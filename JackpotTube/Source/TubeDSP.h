#pragma once
// JACKPOT TUBE BOX: sezione valvolare stile Black Box HG-2 (pentodo 6U8A e triodo in serie, saturazione parallela 12AT7,
// trasformatori) + harmonic enhancer a 4 comandi. Tutto a 4x con filtri a fase lineare: la latenza dichiarata e' quella vera.
// Spento: passa solo un ritardo uguale (neutro al bit). Ogni comando scivola, niente click.
#include <juce_dsp/juce_dsp.h>
#include "JackpotKit.h"

namespace jtbx
{
struct Settings
{
    bool hgOn = false; float inDb = 0, pentode = 5, triode = 5, density = 0, sat = 0, air = 0, mix = 1;
    int satFreq = 1, calib = 1; bool satIn = false, alt = false, airIn = false, ms = false;
    // enhancer: quantita', carattere, dove (fuoco), larghezza
    bool mbOn = false; float amount = 0, focus = 0.5f, width = 1; int character = 0;
    float outDb = 0;
};
// fuoco dell'enhancer: 0 = 60 Hz (sub), 1 = 9 kHz (aria)
inline float focusHz (float f) { return 60.0f * std::pow (150.0f, juce::jlimit (0.0f, 1.0f, f)); }

// ----------------------------------------------------------------------- antiderivata del primo ordine (in double)
template <typename F, typename Fi>
struct Adaa
{
    double x1 = 0, F1 = 0; bool init = false;
    void reset() { init = false; }
    inline float run (float xf, const F& f, const Fi& Fint)
    {
        const double x = xf, Fx = Fint (x);
        if (! init) { init = true; x1 = x; F1 = Fx; return (float) f (x); }
        const double d = x - x1; double y;
        if (std::abs (d) < 1e-6) y = f (0.5 * (x + x1)); else y = (Fx - F1) / d;
        x1 = x; F1 = Fx; return (float) y;
    }
};
inline double lcosh (double x) { const double a = std::abs (x); return a + std::log1p (std::exp (-2.0 * a)) - 0.6931471805599453; }
// pentodo 6U8A: ginocchio netto (sigmoide algebrica), un filo di asimmetria
struct PentF  { double operator() (double u) const { const double v = u + 0.06; return v / std::sqrt (1.0 + v * v) - 0.0598924; } };
struct PentFi { double operator() (double u) const { const double v = u + 0.06; return std::sqrt (1.0 + v * v) - 0.0598924 * u; } };
// triodo: la griglia conduce sul lato positivo (si schiaccia prima): tanh a due pendenze, con la polarizzazione b
struct TriF  { double b; double operator() (double x) const { const double u = x + b; return (u > 0 ? std::tanh (u * 1.25) / 1.25 : std::tanh (u * 0.85) / 0.85) - y0(); }
               double y0() const { return b > 0 ? std::tanh (b * 1.25) / 1.25 : std::tanh (b * 0.85) / 0.85; } };
struct TriFi { double b; double operator() (double x) const { const double u = x + b; const double y0 = b > 0 ? std::tanh (b * 1.25) / 1.25 : std::tanh (b * 0.85) / 0.85;
               return (u > 0 ? lcosh (u * 1.25) / 1.5625 : lcosh (u * 0.85) / 0.7225) - y0 * x; } };

class Engine
{
public:
    static constexpr int kOs = 4;
    void prepare (double s, int maxBlock)
    {
        sr = s; sro = s * kOs; mb = juce::jmax (64, maxBlock);
        os = std::make_unique<juce::dsp::Oversampling<float>> (2, 2, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, true);
        os->initProcessing ((size_t) mb);
        // passa-basso a 16 poli a 21 kHz dentro il 4x (quello che le valvole creano sopra non ripiega); il suo ritardo si conta
        static const double Q8[8] = { 0.5024, 0.5225, 0.5669, 0.6468, 0.7882, 1.0607, 1.7224, 5.1011 }; double sq = 0; for (double q : Q8) sq += 1.0 / q;
        const double fc = juce::jmin (21000.0, s * 0.45);
        for (int c = 0; c < 2; ++c) for (int j = 0; j < 8; ++j) { aaLp[c][j].lp (sro, fc, Q8[j]); aaLp[c][j].reset(); }
        lat = juce::jmax (1, (int) std::round (os->getLatencyInSamples() + sq / (juce::MathConstants<double>::twoPi * fc) * s));
        for (int c = 0; c < 2; ++c) { ring[c].assign ((size_t) lat, 0.0f); dryT[c].assign ((size_t) mb, 0.0f); }
        rw = 0; running = false; warm = 0; gate = 0;
        resetState(); sm = {}; smInit = false;
    }
    int latency() const { return lat; }

    void process (float* L, float* R, int n, const Settings& S)
    {
        const bool act = S.hgOn || S.mbOn || hgS > 1e-4f || enS > 1e-4f;
        if ((S.hgOn || S.mbOn) && ! running) { running = true; warm = lat + 64; gate = 0; os->reset(); resetState(); }
        float* ch[2] = { L, R };
        for (int off = 0; off < n; off += mb)
        {
            const int m = juce::jmin (mb, n - off);
            float* io[2] = { L + off, R + off };
            if (running) { for (int c = 0; c < 2; ++c) std::copy (io[c], io[c] + m, dryT[c].begin()); runOs (io, m, S); }
            // fuori dal 4x quando e' tutto spento: solo un ritardo uguale alla latenza (neutro al bit)
            const float step = 1.0f / 1024.0f;
            for (int i = 0; i < m; ++i)
            {
                if (warm > 0) --warm; else if (running) gate = act ? juce::jmin (1.0f, gate + step) : juce::jmax (0.0f, gate - step);
                for (int c = 0; c < 2; ++c)
                {
                    const float in = running ? dryT[c][(size_t) i] : io[c][i], d = ring[c][(size_t) rw]; ring[c][(size_t) rw] = in;
                    io[c][i] = running ? d + (io[c][i] - d) * gate : d;
                }
                if (++rw >= lat) rw = 0;
            }
            if (! act && gate <= 0.0f && warm == 0) running = false;
        }
        // uscita (scivola)
        const float og = juce::Decibels::decibelsToGain (S.outDb), ok = 1.0f - std::exp (-1.0f / (0.02f * (float) sr));
        if (outS < 0) outS = og;
        float pk = 0;
        for (int i = 0; i < n; ++i) { outS += (og - outS) * ok; if (outS != 1.0f) { ch[0][i] *= outS; ch[1][i] *= outS; } pk = juce::jmax (pk, std::abs (ch[0][i]), std::abs (ch[1][i])); }
        if (std::abs (outS - og) < 1e-6f) outS = og;
        outPk = juce::jmax (pk, outPk.load() * 0.9f);
        vuUi = vu; heat = heatS; for (int k = 0; k < 6; ++k) harmUi[k] = hUi[k];
        if (! running) { vu *= 0.9f; heatS *= 0.9f; }
    }
    std::atomic<float> vuUi { 0 }, outPk { 0 }, heat { 0 }, harmUi[6] {};
private:
    struct Sm { float inG = 1, gP = 1, gT = 1, dens = 1, sat = 0, air = 0, mix = 1, amt = 0, foc = 0.5f, wid = 1; };
    void resetState()
    {
        for (int c = 0; c < 2; ++c)
        {
            for (auto& f : satF[c]) f.reset(); airF[c].reset(); calF[c].reset(); inLp[c] = 0; dcx[c] = dcy[c] = 0;
            aPent[c].reset(); aTri[c].reset(); aSat[c].reset(); aIn[c].reset();
            enHp[c].reset(); enLp[c].reset(); enHp2[c].reset(); hEnv[c] = 0; hDc[c] = 0; for (auto& f : aaLp[c]) f.reset();
        }
        lastKey = -1; lastAir = -1; lastFoc = -1; hgS = 0; enS = 0; mixS = -1;
    }
    void runOs (float* const* io, int m, const Settings& S)
    {
        // mono (sinistro = destro: bassi, 808, voci): si calcola un canale solo e si copia, stato compreso
        const bool mono = ! S.ms && std::memcmp (io[0], io[1], sizeof (float) * (size_t) m) == 0; const int nc = mono ? 1 : 2;
        juce::dsp::AudioBlock<float> blk (io, 2, (size_t) m);
        auto up = os->processSamplesUp (blk);
        const int N = (int) up.getNumSamples();
        float* u[2] = { up.getChannelPointer (0), up.getChannelPointer (1) };
        // bersagli dei comandi
        Sm t;
        t.inG = juce::Decibels::decibelsToGain (S.inDb);
        t.dens = juce::Decibels::decibelsToGain (S.density * 1.2f);                 // fino a +12 dB in entrambe le valvole, il volume resta
        t.gP = juce::Decibels::decibelsToGain (-12.0f + 2.4f * S.pentode) * t.dens;
        t.gT = juce::Decibels::decibelsToGain (-12.0f + 2.4f * S.triode);
        t.sat = S.satIn ? std::pow (S.sat / 10.0f, 1.5f) * 0.9f : 0.0f;
        t.air = S.airIn ? S.air * 0.7f : 0.0f;
        t.mix = S.mix; t.amt = S.mbOn ? S.amount / 10.0f : 0.0f; t.foc = S.focus; t.wid = S.width;
        if (! smInit) { sm = t; smInit = true; }
        const float k4 = 1.0f - std::exp (-1.0f / (0.02f * (float) sro));             // 20 ms
        const float onK = 1.0f / (0.015f * (float) sro);                               // accensione delle sezioni in 15 ms
        // filtri che non scivolano (selettori): subito
        const int key = S.satFreq * 10 + S.calib;
        if (key != lastKey)
        {
            lastKey = key;
            for (int c = 0; c < 2; ++c)
            {
                if (S.satFreq == 0) { satF[c][0].lowShelf (sro, 300, 6, 0.7); satF[c][1].lp (sro, 4000, 0.6); }
                else if (S.satFreq == 2) { satF[c][0].highShelf (sro, 2500, 6, 0.7); satF[c][1].hp (sro, 250, 0.6); }
                else { satF[c][0].flat(); satF[c][1].hp (sro, 20, 0.6); }
                calF[c].highShelf (sro, 7000, S.calib == 0 ? -2.5 : S.calib == 2 ? 1.8 : 0.0, 0.6);
            }
        }
        const float aLf = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 90.0f / (float) sro);
        const float dcA = 1.0f - (float) (juce::MathConstants<double>::twoPi * 5.0 / sro);
        const float eRel = (float) std::exp (-1.0 / (0.02 * sro));
        const float altG = S.alt ? 2.0f : 1.0f; const double altB = S.alt ? 0.32 : 0.2;
        const TriF triSat { altB }; const TriFi triSatI { altB }; const TriF triT { 0.18 }; const TriFi triTI { 0.18 };
        const PentF pf; const PentFi pfi;
        float vuAcc = vu, heatAcc = heatS; float hu[6] = { 0, 0, 0, 0, 0, 0 }; int hn = 0;
        for (int i = 0; i < N; ++i)
        {
            sm.inG += (t.inG - sm.inG) * k4; sm.gP += (t.gP - sm.gP) * k4; sm.gT += (t.gT - sm.gT) * k4; sm.dens += (t.dens - sm.dens) * k4;
            sm.sat += (t.sat - sm.sat) * k4; sm.air += (t.air - sm.air) * k4; sm.mix += (t.mix - sm.mix) * k4;
            sm.amt += (t.amt - sm.amt) * k4; sm.foc += (t.foc - sm.foc) * k4; sm.wid += (t.wid - sm.wid) * k4;
            hgS = S.hgOn ? juce::jmin (1.0f, hgS + onK) : juce::jmax (0.0f, hgS - onK);
            enS = S.mbOn ? juce::jmin (1.0f, enS + onK) : juce::jmax (0.0f, enS - onK);
            if ((i & 31) == 0)
            {   // filtri che scivolano, aggiornati a piccoli passi
                if (std::abs (sm.air - lastAir) > 0.002f) { lastAir = sm.air; for (int c = 0; c < 2; ++c) airF[c].highShelf (sro, 14000, sm.air, 0.6); }
                if (std::abs (sm.foc - lastFoc) > 0.0005f)
                {
                    lastFoc = sm.foc; const float f = focusHz (sm.foc);
                    for (int c = 0; c < 2; ++c) { enHp[c].hp (sro, f * 0.7f, 0.6); enLp[c].lp (sro, f * 1.6f, 0.6); enHp2[c].hp (sro, f * 1.4f, 0.7); }
                }
            }
            float x[2] = { u[0][i], u[1][i] };
            // ---- valvole HG
            if (hgS > 0.0f)
            {
                float y[2] = { x[0], x[1] };
                if (S.ms) { const float mm = 0.5f * (y[0] + y[1]), sd = 0.5f * (y[0] - y[1]); y[0] = mm; y[1] = sd; }
                for (int c = 0; c < nc; ++c)
                {
                    float v = y[c] * sm.inG;
                    // trasformatore d'ingresso: il nucleo satura sulle basse
                    inLp[c] += (v - inLp[c]) * aLf; const float lo = inLp[c];
                    v += (aIn[c].run (lo / 0.9f, TanhF(), TanhFi()) * 0.9f - lo) * 0.6f;
                    // saturazione parallela 12AT7, colorata dal SAT FREQ, sommata prima del pentodo
                    if (sm.sat > 0.0005f)
                    {
                        const float sv = satF[c][1].p (satF[c][0].p (v));
                        v += aSat[c].run (sv * 4.0f * altG, triSat, triSatI) / (2.0f * altG) * sm.sat;
                    }
                    // pentodo, aria allo stadio del triodo, triodo
                    v = aPent[c].run (v * sm.gP / 1.4f, pf, pfi) * 1.4f;
                    v = airF[c].p (v);
                    v = aTri[c].run (v * sm.gT / 1.6f, triT, triTI) * 1.6f / sm.dens;
                    // via la continua, calibrazione, trasformatore d'uscita
                    const float yy = v - dcx[c] + dcA * dcy[c]; dcx[c] = v; dcy[c] = yy; v = yy;
                    v = calF[c].p (v);
                    v = v - 0.015f * v * v * v;
                    y[c] = v;
                }
                if (mono) y[1] = y[0];
                if (S.ms) { const float mm = y[0], sd = y[1]; y[0] = mm + sd; y[1] = mm - sd; }
                const float g = hgS * sm.mix;
                for (int c = 0; c < 2; ++c) x[c] += (y[c] - x[c]) * g;
                heatAcc += (std::abs (y[0] - u[0][i]) + std::abs (y[1] - u[1][i]) - heatAcc) * 0.00002f;
            }
            // ---- enhancer: armoniche nate dalla zona del FUOCO, sommate al suono (il diretto non si tocca)
            if (enS > 0.0f && sm.amt > 0.0005f)
            {
                float h[2];
                for (int c = 0; c < nc; ++c)
                {
                    const float b = enLp[c].p (enHp[c].p (x[c]));
                    const float a = std::abs (b); hEnv[c] = a > hEnv[c] ? a : hEnv[c] * eRel + a * (1 - eRel);
                    const float en = hEnv[c] + 1e-6f, xx = juce::jlimit (-1.0f, 1.0f, b / en), x2 = xx * xx;
                    const float t2 = 2 * x2 - 1, t3 = xx * (4 * x2 - 3), t4 = 8 * x2 * x2 - 8 * x2 + 1, t5 = xx * (16 * x2 * x2 - 20 * x2 + 5);
                    float hh;
                    switch (S.character)
                    {
                        case 0:  hh = 0.75f * t2 + 0.25f * t3 + 0.12f * t4; break;          // TUBO: seconda, un filo di terza e quarta
                        case 1:  hh = 0.2f * t2 + 0.65f * t3 + 0.1f * t5; break;            // NASTRO: terza morbida
                        default: hh = 0.55f * t3 + 0.4f * t5; break;                          // TRANSISTOR: dispari alte, grinta
                    }
                    // il livello delle armoniche segue il segnale (sotto la soglia di rumore non nasce niente)
                    hh *= en * (en / (en + 0.003f));
                    hh = enHp2[c].p (hh);   // solo sopra la zona: niente intermodulazione sotto
                    h[c] = hh;
                    if (c == 0 && (i & 15) == 0) { hu[0] += std::abs (b); hu[1] += std::abs (t2) * en; hu[2] += std::abs (t3) * en; hu[3] += std::abs (t4) * en; hu[4] += std::abs (t5) * en; ++hn; }
                }
                if (mono) h[1] = h[0];
                const float hm = 0.5f * (h[0] + h[1]), hs = 0.5f * (h[0] - h[1]) * sm.wid;
                const float g = enS * sm.amt * 0.7f;
                x[0] += (hm + hs) * g; x[1] += (hm - hs) * g;
            }
            for (int c = 0; c < nc; ++c) { float y = x[c]; for (auto& f : aaLp[c]) y = f.p (y); x[c] = y; }
            if (mono) x[1] = x[0];
            u[0][i] = x[0]; u[1][i] = x[1];
            vuAcc += (std::abs (x[0]) + std::abs (x[1]) - vuAcc) * 0.00003f;
        }
        if (mono)
        {
            for (int j = 0; j < 2; ++j) satF[1][j] = satF[0][j];
            airF[1] = airF[0]; calF[1] = calF[0]; enHp[1] = enHp[0]; enLp[1] = enLp[0]; enHp2[1] = enHp2[0]; for (int j = 0; j < 8; ++j) aaLp[1][j] = aaLp[0][j];
            aPent[1] = aPent[0]; aTri[1] = aTri[0]; aSat[1] = aSat[0]; aIn[1] = aIn[0];
            inLp[1] = inLp[0]; dcx[1] = dcx[0]; dcy[1] = dcy[0]; hEnv[1] = hEnv[0];
        }
        os->processSamplesDown (blk);
        vu = vuAcc * 0.5f; heatS = heatAcc;
        {   // per il disegno: quanto pesa ogni armonica (2a..5a) e quanto suona la zona del fuoco
            float k[4];
            switch (S.character) { case 0: k[0] = 0.75f; k[1] = 0.25f; k[2] = 0.12f; k[3] = 0.0f; break; case 1: k[0] = 0.2f; k[1] = 0.65f; k[2] = 0.0f; k[3] = 0.1f; break; default: k[0] = 0.0f; k[1] = 0.55f; k[2] = 0.0f; k[3] = 0.4f; }
            const float on = sm.amt * enS;
            for (int j = 0; j < 4; ++j) hUi[j] += (juce::jlimit (0.0f, 1.0f, k[j] * on * 1.4f) - hUi[j]) * 0.3f;
            hUi[4] += ((hn > 0 ? hu[0] / (float) hn : 0.0f) - hUi[4]) * 0.3f;
        }
    }
    struct TanhF  { double operator() (double x) const { return std::tanh (x); } };
    struct TanhFi { double operator() (double x) const { return lcosh (x); } };
    double sr = 48000, sro = 192000; int mb = 512, lat = 1;
    std::unique_ptr<juce::dsp::Oversampling<float>> os;
    std::vector<float> ring[2], dryT[2]; int rw = 0, warm = 0; float gate = 0; bool running = false;
    jkit::Bq satF[2][2], airF[2], calF[2], enHp[2], enLp[2], enHp2[2], aaLp[2][8];
    Adaa<PentF, PentFi> aPent[2]; Adaa<TriF, TriFi> aTri[2], aSat[2]; Adaa<TanhF, TanhFi> aIn[2];
    float inLp[2] {}, dcx[2] {}, dcy[2] {}, hEnv[2] {}, hDc[2] {};
    int lastKey = -1; float lastAir = -1, lastFoc = -1, hgS = 0, enS = 0, mixS = -1, outS = -1, vu = 0, heatS = 0, hUi[6] {};
    Sm sm; bool smInit = false;
};
}
