#pragma once
#include <juce_dsp/juce_dsp.h>
#include "JackpotKit.h"

namespace jtbx
{
// ======================================================================= impostazioni
struct Settings
{
    // sezione valvolare (stile HG-2): pentodo e triodo in serie, saturazione parallela 12AT7
    bool hgOn = false; float inDb = 0, pentode = 5, triode = 5, density = 0, sat = 0, air = 0, mix = 1;
    int satFreq = 1, calib = 1; bool satIn = false, alt = false, airIn = false, ms = false;
    // enhancer multibanda
    bool mbOn = false; float xo[4] { 120, 600, 2500, 8000 }, harm[5] {}, lvl[5] {}, width[5] { 1, 1, 1, 1, 1 }; int character = 0; float mbMix = 1;
    float outDb = 0;
};

// ----------------------------------------------------------------------- forme delle valvole
// pentodo 6U8A: ginocchio piu netto, armoniche dispari con un filo di pari
inline float pentodeShape (float x)
{
    // sigmoide algebrica: ginocchio piu netto della tanh, economica
    const float b = 0.06f, u = x + b;
    const float y = u / std::sqrt (1.0f + u * u);
    constexpr float y0 = 0.06f / 1.0017982f;
    return (y - y0) * 1.0018f;
}
// triodo: seconda armonica forte, la griglia conduce sul lato positivo (si schiaccia prima)
inline float triodeShape (float x, float bias)
{
    const float u = x + bias;
    const float y = u > 0 ? std::tanh (u * 1.25f) / 1.25f : std::tanh (u * 0.85f) / 0.85f;
    const float y0 = bias > 0 ? std::tanh (bias * 1.25f) / 1.25f : std::tanh (bias * 0.85f) / 0.85f;
    return y - y0;
}

class Engine
{
public:
    void prepare (double s, int maxBlock)
    {
        sr = s; sro = s * 2;
        os = std::make_unique<juce::dsp::Oversampling<float>> (2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false);
        os->initProcessing ((size_t) maxBlock);
        for (int c = 0; c < 2; ++c) { for (auto& f : satF[c]) f.reset(); airF[c].reset(); calF[c].reset(); inLp[c] = 0; dc1[c] = dc2[c] = 0; dcx1[c] = dcx2[c] = 0; for (auto& x : mbx[c]) x.reset(); for (auto& a : mbap[c]) a.reset(); hdc[c][0] = hdc[c][1] = hdc[c][2] = hdc[c][3] = hdc[c][4] = 0; }
        lastKey = -1; lastXo[0] = -1; vu = 0; erel = (float) std::exp (-1.0 / (0.02 * sro)); for (auto& r : henv) for (auto& v : r) v = 0;
        dry.setSize (2, maxBlock); dl[0].assign (1 << 12, 0.0f); dl[1].assign (1 << 12, 0.0f); dw = 0;
    }
    int latency() const { return os ? (int) std::round (os->getLatencyInSamples()) : 0; }

    void process (float* L, float* R, int n, const Settings& S)
    {
        float* ch[2] = { L, R };
        for (int c = 0; c < 2; ++c) dry.copyFrom (c, 0, ch[c], n);
        if (! S.hgOn && ! S.mbOn)
        {   // spento: solo il ritardo di compensazione, cosi la latenza non cambia
            delayDry (n); for (int c = 0; c < 2; ++c) juce::FloatVectorOperations::copy (ch[c], dry.getReadPointer (c), n);
            applyOut (L, R, n, S); return;
        }
        // filtri (alla frequenza sovracampionata)
        const int key = S.satFreq * 100 + S.calib * 10 + (int) (S.air * 10) + (S.airIn ? 1000 : 0);
        if (key != lastKey)
        {
            lastKey = key;
            for (int c = 0; c < 2; ++c)
            {
                // frequenza della saturazione parallela: LOW pesa i bassi, HIGH gli alti, FLAT piena
                if (S.satFreq == 0) { satF[c][0].lowShelf (sro, 300, 6, 0.7); satF[c][1].lp (sro, 4000, 0.6); }
                else if (S.satFreq == 2) { satF[c][0].highShelf (sro, 2500, 6, 0.7); satF[c][1].hp (sro, 250, 0.6); }
                else { satF[c][0].flat(); satF[c][1].hp (sro, 20, 0.6); }
                // aria: sale da 10 kHz verso 20 kHz
                airF[c].highShelf (sro, 14000, S.airIn ? S.air * 0.7 : 0.0, 0.6);
                // calibrazione: dark, normal, bright
                calF[c].highShelf (sro, 7000, S.calib == 0 ? -2.5 : S.calib == 2 ? 1.8 : 0.0, 0.6);
            }
        }
        if (S.xo[0] != lastXo[0] || S.xo[1] != lastXo[1] || S.xo[2] != lastXo[2] || S.xo[3] != lastXo[3])
        {
            for (int k = 0; k < 4; ++k) lastXo[k] = S.xo[k];
            for (int c = 0; c < 2; ++c) for (int k = 0; k < 4; ++k) { mbx[c][k].set (sro, S.xo[k]); for (int j = 0; j < 4; ++j) mbap[c][k * 4 + j].set (sro, S.xo[j]); }
        }
        juce::dsp::AudioBlock<float> blk (ch, 2, (size_t) n);
        auto up = os->processSamplesUp (blk);
        const int N = (int) up.getNumSamples();
        float* u[2] = { up.getChannelPointer (0), up.getChannelPointer (1) };
        const float inG = juce::Decibels::decibelsToGain (S.inDb);
        const float dens = juce::Decibels::decibelsToGain (S.density * 1.2f);      // fino a +12 dB in entrambe le valvole
        const float gP = juce::Decibels::decibelsToGain (-12.0f + 2.4f * S.pentode) * dens, gT = juce::Decibels::decibelsToGain (-12.0f + 2.4f * S.triode);
        const float satAmt = S.satIn ? std::pow (S.sat / 10.0f, 1.5f) * 0.9f : 0.0f;
        const float altG = S.alt ? 2.0f : 1.0f, altB = S.alt ? 0.32f : 0.2f;
        const float aLf = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 90.0f / (float) sro);
        const float dcA = 1.0f - (float) (juce::MathConstants<double>::twoPi * 6.0 / sro);
        for (int i = 0; i < N; ++i)
        {
            float x[2] = { u[0][i], u[1][i] };
            if (S.hgOn)
            {
                if (S.ms) { const float m = 0.5f * (x[0] + x[1]), sd = 0.5f * (x[0] - x[1]); x[0] = m; x[1] = sd; }
                for (int c = 0; c < 2; ++c)
                {
                    float v = x[c] * inG;
                    // trasformatore d'ingresso: nucleo che satura sulle basse
                    inLp[c] += (v - inLp[c]) * aLf; const float lo = inLp[c];
                    v += (0.9f * std::tanh (lo / 0.9f) - lo) * 0.6f;
                    // saturazione parallela 12AT7, filtrata dal SAT FREQ, sommata prima del pentodo
                    if (satAmt > 0.0005f)
                    {
                        float sv = satF[c][1].p (satF[c][0].p (v));
                        sv = triodeShape (sv * 4.0f * altG, altB) / (2.0f * altG);
                        v += sv * satAmt;
                    }
                    // pentodo
                    v = pentodeShape (v * gP / 1.4f) * 1.4f;
                    // aria inserita allo stadio del triodo
                    v = airF[c].p (v);
                    // triodo
                    v = triodeShape (v * gT / 1.6f, 0.18f) * 1.6f / dens;
                    // via la componente continua delle valvole
                    const float y = v - dcx1[c] + dcA * dc1[c]; dcx1[c] = v; dc1[c] = y; v = y;
                    // calibrazione e trasformatore d'uscita
                    v = calF[c].p (v);
                    v = v - 0.015f * v * v * v;
                    x[c] = v;
                }
                if (S.ms) { const float m = x[0], sd = x[1]; x[0] = m + sd; x[1] = m - sd; }
            }
            if (S.mbOn) enhance (x, S, dcA);
            u[0][i] = x[0]; u[1][i] = x[1];
            vu += (std::abs (x[0]) + std::abs (x[1]) - vu) * 0.00004f;
        }
        os->processSamplesDown (blk);
        // mix con il dry ritardato
        delayDry (n);
        const float mx = S.hgOn ? S.mix : 1.0f;
        if (mx < 0.999f) for (int c = 0; c < 2; ++c) for (int i = 0; i < n; ++i) ch[c][i] = ch[c][i] * mx + dry.getSample (c, i) * (1 - mx);
        applyOut (L, R, n, S);
        vuUi = vu * 0.5f;
    }
    std::atomic<float> vuUi { 0 }, outPk { 0 }, bandUi[5] {};
private:
    void delayDry (int n)
    {
        const int lat = latency(), M = (int) dl[0].size() - 1;
        for (int c = 0; c < 2; ++c) { int w = dw; for (int i = 0; i < n; ++i) { dl[c][(size_t) w] = dry.getSample (c, i); dry.setSample (c, i, dl[c][(size_t) ((w - lat) & M)]); w = (w + 1) & M; } }
        dw = (dw + n) & M;
    }
    void applyOut (float* L, float* R, int n, const Settings& S)
    {
        const float og = juce::Decibels::decibelsToGain (S.outDb); float pk = 0;
        for (int i = 0; i < n; ++i) { L[i] *= og; R[i] *= og; pk = juce::jmax (pk, std::abs (L[i]), std::abs (R[i])); }
        outPk = juce::jmax (pk, outPk.load() * 0.9f);
    }
    // enhancer: 5 bande, armoniche in parallelo per banda, livello e larghezza per banda
    inline void enhance (float* x, const Settings& S, float dcA)
    {
        float band[2][5];
        for (int c = 0; c < 2; ++c)
        {
            float rest = x[c];
            for (int k = 0; k < 4; ++k)
            {
                float lo, hi; mbx[c][k].split (rest, lo, hi);
                // la banda bassa passa per i passa-tutto dei tagli successivi: cosi la somma e piatta
                for (int j = k + 1; j < 4; ++j) lo = mbap[c][k * 4 + j].p (lo);
                band[c][k] = lo; rest = hi;
            }
            band[c][4] = rest;
        }
        float out[2] = { 0, 0 };
        for (int k = 0; k < 5; ++k)
        {
            float b[2] = { band[0][k], band[1][k] };
            if (S.harm[k] > 0.001f)
                for (int c = 0; c < 2; ++c)
                {
                    // armoniche pure (polinomi di Chebyshev sul segnale normalizzato dal suo inviluppo): niente fondamentale
                    const float a = std::abs (b[c]); auto& e = henv[c][k];
                    e = a > e ? a : e * erel + a * (1 - erel);
                    const float en = e + 1e-6f, xx = juce::jlimit (-1.0f, 1.0f, b[c] / en), x2 = xx * xx;
                    const float t2 = 2 * x2 - 1, t3 = xx * (4 * x2 - 3), t5 = xx * (16 * x2 * x2 - 20 * x2 + 5);
                    float h;
                    switch (S.character)
                    {
                        case 0:  h = 0.8f * t2 + 0.25f * t3; break;               // TUBO: seconda, un po' di terza
                        case 1:  h = 0.15f * t2 + 0.7f * t3; break;               // NASTRO: terza morbida
                        default: h = 0.6f * t3 + 0.35f * t5; break;               // TRANSISTOR: dispari alte
                    }
                    h *= en;
                    hdc[c][k] = hdc[c][k] * dcA + h * (1 - dcA); h -= hdc[c][k];
                    b[c] += h * S.harm[k] * 0.6f;
                }
            // larghezza
            if (std::abs (S.width[k] - 1.0f) > 0.001f) { const float m = 0.5f * (b[0] + b[1]), sd = 0.5f * (b[0] - b[1]) * S.width[k]; b[0] = m + sd; b[1] = m - sd; }
            const float g = juce::Decibels::decibelsToGain (S.lvl[k]);
            out[0] += b[0] * g; out[1] += b[1] * g;
            bandUi[k] = bandUi[k].load() * 0.9999f + std::abs (b[0] + b[1]) * 0.0001f;
        }
        const float m = S.mbMix;
        x[0] = out[0] * m + bandSum (band[0]) * (1 - m); x[1] = out[1] * m + bandSum (band[1]) * (1 - m);
    }
    static float bandSum (const float* b) { return b[0] + b[1] + b[2] + b[3] + b[4]; }
    double sr = 44100, sro = 176400;
    std::unique_ptr<juce::dsp::Oversampling<float>> os;
    jkit::Bq satF[2][2], airF[2], calF[2];
    float inLp[2] {}, dc1[2] {}, dc2[2] {}, dcx1[2] {}, dcx2[2] {}, hdc[2][5] {}, henv[2][5] {}; float erel = 0.9999f;
    jkit::LR4 mbx[2][4]; jkit::LRAp mbap[2][16];
    int lastKey = -1; float lastXo[4] { -1, -1, -1, -1 }; float vu = 0;
    juce::AudioBuffer<float> dry; std::vector<float> dl[2]; int dw = 0;
};
}
