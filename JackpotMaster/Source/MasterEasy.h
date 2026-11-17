#pragma once
#include "MasterDyn.h"
#include "MasterColor.h"

// Motore della modalita FACILE: poche manopole, algoritmi che lavorano da soli.
// Ordine: EQ a bande fisse -> CALM (risonanze) -> THICK / WARM / SILK -> GLUE -> WIDE. Poi il processore fa LOUD (maximizer) e il true peak.
namespace jms
{
struct EasySettings { float low = 0, mid = 0, high = 0, calm = 0, thick = 0, warm = 0, silk = 0, glue = 0, wide = 0; bool monoBass = true; int listenBand = -1; bool listenDelta = false; };

class EasyEngine
{
public:
    void prepare (double s)
    {
        sr = s;
        for (auto& c : eq) for (auto& f : c) f.reset();
        for (auto& b : calm) { b.env = b.slow = 0; b.g = 0; for (auto& f : b.det) f.reset(); for (auto& f : b.cut) f.reset(); b.det[0].set (Biquad::BPASS, b.f, 0, 1.4, sr); b.det[1].set (Biquad::BPASS, b.f, 0, 1.4, sr); }
        for (int c = 0; c < 2; ++c) { tLp[c].set (Biquad::HCUT, 1400, 0, 0.707, sr); tLp2[c].set (Biquad::HCUT, 1400, 0, 0.707, sr); tLp[c].reset(); tLp2[c].reset(); sHp[c].set (Biquad::LCUT, 4500, 0, 0.707, sr); sHp[c].reset(); }
        wLp.set (Biquad::HCUT, 120, 0, 0.707, sr); wLp2.set (Biquad::HCUT, 120, 0, 0.707, sr); wLp.reset(); wLp2.reset(); mLp = wLp; mLp2 = wLp2; wM = wS = 0; wSm = 1;
        tape.prepare (s); bus.prepare (s);
        for (int c = 0; c < 2; ++c) { tEnv[c] = tDc[c] = sEnv[c] = 0; sHp3[c].set (Biquad::LCUT, 3500, 0, 0.707, sr); sHp3[c].reset(); }
        sMax = 1024; sBuf.setSize (2, sMax);
        sOs = std::make_unique<juce::dsp::Oversampling<float>> (2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, false, false);
        sOs->initProcessing ((size_t) sMax);
        last = {}; last.low = 99; smInit = false;
    }
    // le manopole scivolano verso il valore nuovo (circa 10 ms, a passi di 32 campioni): niente click quando si muovono
    void process (float* L, float* R, int n, const EasySettings& S)
    {
        if (! smInit) { sm = S; smInit = true; }
        auto same = [&] { return sm.low == S.low && sm.mid == S.mid && sm.high == S.high && sm.thick == S.thick && sm.warm == S.warm && sm.silk == S.silk && sm.glue == S.glue && sm.calm == S.calm && sm.wide == S.wide; };
        sm.monoBass = S.monoBass; sm.listenBand = S.listenBand; sm.listenDelta = S.listenDelta;
        if (same()) { raw (L, R, n, sm); return; }
        const float k = 1.0f - std::exp (-32.0f / (0.010f * (float) sr));
        auto glide = [&] (float& a, float b, float eps) { a += (b - a) * k; if (std::abs (b - a) < eps) a = b; };
        for (int o = 0; o < n; o += 32)
        {
            glide (sm.low, S.low, 0.005f); glide (sm.mid, S.mid, 0.005f); glide (sm.high, S.high, 0.005f);
            glide (sm.thick, S.thick, 0.0005f); glide (sm.warm, S.warm, 0.0005f); glide (sm.silk, S.silk, 0.0005f);
            glide (sm.glue, S.glue, 0.0005f); glide (sm.calm, S.calm, 0.0005f); glide (sm.wide, S.wide, 0.0005f);
            raw (L + o, R + o, juce::jmin (32, n - o), sm);
        }
    }
    void reset() { smInit = false; }
private:
    EasySettings sm; bool smInit = false;
public:
    void raw (float* L, float* R, int n, const EasySettings& S)
    {
        // ---- EQ: shelf morbido sui bassi (Baxandall), campana larga sui medi, shelf d'aria sugli alti
        if (S.low != last.low || S.mid != last.mid || S.high != last.high)
            for (int c = 0; c < 2; ++c)
            {
                eq[c][0].set (Biquad::LSHELF, 90, S.low, 0.55, sr); eq[c][1].set (Biquad::BELL, 1200, S.mid, 0.45, sr); eq[c][2].set (Biquad::HSHELF, 9000, S.high, 0.55, sr);
                for (int b = 0; b < 3; ++b) { auto& f = dq[c][b]; if (b == 0) f.set (Biquad::LSHELF, 90, S.low, 0.55, sr); else if (b == 1) f.set (Biquad::BELL, 1200, S.mid, 0.45, sr); else f.set (Biquad::HSHELF, 9000, S.high, 0.55, sr); }
            }
        // DELTA di una banda: si sente solo cosa aggiunge o toglie quella banda (a zero: silenzio)
        if (S.listenDelta && S.listenBand >= 0 && S.listenBand < 3)
        {
            const int b = S.listenBand;
            for (int i = 0; i < n; ++i) { const float l = L[i], r = R[i]; L[i] = dq[0][b] (l) - l; R[i] = dq[1][b] (r) - r; }
            last = S; return;
        }
        if (std::abs (S.low) > 0.01f || std::abs (S.mid) > 0.01f || std::abs (S.high) > 0.01f)
            for (int i = 0; i < n; ++i) { L[i] = eq[0][2] (eq[0][1] (eq[0][0] (L[i]))); R[i] = eq[1][2] (eq[1][1] (eq[1][0] (R[i]))); }
        // ---- CALM: tre zone critiche (fango, durezza, sibilanti). Ogni zona e confrontata con la sua media lunga:
        // quando "sporge" (risonanza che si accende, nota che rimbomba, s) viene abbassata, fino a 6 dB con CALM al massimo.
        if (S.calm > 0.001f)
        {
            const float at = (float) std::exp (-1.0 / (0.004 * sr)), rl = (float) std::exp (-1.0 / (0.09 * sr)), sl = (float) std::exp (-1.0 / (2.5 * sr));
            for (int i0 = 0; i0 < n; i0 += 32)
            {
                const int m = juce::jmin (32, n - i0);
                for (auto& b : calm)
                {
                    for (int i = i0; i < i0 + m; ++i)
                    {
                        const float x = 0.5f * (std::abs (b.det[0] (L[i])) + std::abs (b.det[1] (R[i])));
                        b.env = x > b.env ? at * b.env + (1 - at) * x : rl * b.env + (1 - rl) * x;
                        b.slow = sl * b.slow + (1 - sl) * b.env;
                    }
                    const float over = 20.0f * std::log10 ((b.env + 1e-9f) / (b.slow + 1e-9f));   // dB sopra la sua media
                    const float tgt = -juce::jlimit (0.0f, b.maxCut * 1.5f, (over - b.knee) * 1.0f) * S.calm;
                    b.g += (tgt - b.g) * 0.25f;
                    b.cut[0].set (Biquad::BELL, b.f, b.g, b.q, sr); b.cut[1].set (Biquad::BELL, b.f, b.g, b.q, sr);
                    for (int i = i0; i < i0 + m; ++i) { L[i] = b.cut[0] (L[i]); R[i] = b.cut[1] (R[i]); }
                }
            }
            calmNow = juce::jmin (calm[0].g, juce::jmin (calm[1].g, calm[2].g));
        }
        else calmNow = 0;
        // ---- THICK: armoniche "da valvola" sui bassi-medi (sotto 1.4 kHz), generate esatte invece che con una curva:
        // 2a armonica dominante (pari, calda), un po' di 3a (corpo), nient'altro -> nessun aliasing, niente pompaggio.
        // Come in una valvola vera, piu' il passaggio e' forte piu' armoniche escono.
        if (S.thick > 0.001f)
        {
            const float aA = (float) std::exp (-1.0 / (0.001 * sr)), aR = (float) std::exp (-1.0 / (0.06 * sr)), dcK = (float) std::exp (-1.0 / (0.015 * sr));
            for (int c = 0; c < 2; ++c)
            {
                float* x = c ? R : L;
                for (int i = 0; i < n; ++i)
                {
                    const float lo = tLp2[c] (tLp[c] (x[i])), a = std::abs (lo);
                    tEnv[c] = a > tEnv[c] ? aA * tEnv[c] + (1 - aA) * a : aR * tEnv[c] + (1 - aR) * a;
                    const float e = juce::jmax (1.0e-5f, tEnv[c] * 1.15f), u = juce::jlimit (-1.0f, 1.0f, lo / e);
                    const float t2 = e * (2 * u * u - 1), t3 = e * (4 * u * u * u - 3 * u);              // Chebyshev: 2a e 3a armonica pure
                    const float drive = S.thick * (0.35f + 0.65f * e / (e + 0.25f));                       // dipende dal livello
                    float h = drive * (0.42f * t2 + 0.14f * t3);
                    tDc[c] = dcK * tDc[c] + (1 - dcK) * h; h -= tDc[c];
                    x[i] += h + S.thick * 0.12f * lo;                                                       // + un filo di corpo
                }
            }
        }
        // ---- WARM: nastro da 15 pollici (head bump, compressione morbida, alti arrotondati)
        // il nastro passa sempre (a miscela zero quando WARM e' a zero): il suo ritardo resta uguale e la DAW lo compensa
        { TapeSettings t; t.model = 1; t.drive = 1.0f * S.warm; t.bias = 0.1f; t.noise = 0; t.mix = S.warm > 0.001f ? juce::jmin (1.0f, 0.5f + S.warm) : 0.0f; tape.process (L, R, n, t); }
        // ---- SILK: aria sopra 4.5 kHz. Parte lineare (un filo di shelf) al rate normale; le armoniche (2a + 3a, generate esatte)
        // a 4x e poi filtrate: quelle oltre i 20 kHz non rientrano come sporco (aliasing)
        if (S.silk > 0.001f && sOs)
        {
            const float k = S.silk; const int m = juce::jmin (n, sMax);
            for (int off = 0; off < n; off += m)
            {
                const int nn = juce::jmin (m, n - off);
                for (int c = 0; c < 2; ++c) { float* x = (c ? R : L) + off; float* hb = sBuf.getWritePointer (c); for (int i = 0; i < nn; ++i) { hb[i] = sHp[c] (x[i]); x[i] += k * 0.55f * hb[i]; } }   // parte lineare, in fase
                juce::dsp::AudioBlock<float> blk (sBuf.getArrayOfWritePointers(), 2, (size_t) nn);
                auto up = sOs->processSamplesUp (blk);
                const double fs4 = sr * 4; const float aA = (float) std::exp (-1.0 / (0.0005 * fs4)), aR = (float) std::exp (-1.0 / (0.03 * fs4));
                for (int c = 0; c < 2; ++c)
                {
                    float* u = up.getChannelPointer ((size_t) c); const int nu = (int) up.getNumSamples();
                    for (int i = 0; i < nu; ++i)
                    {
                        const float h = u[i], a = std::abs (h);
                        sEnv[c] = a > sEnv[c] ? aA * sEnv[c] + (1 - aA) * a : aR * sEnv[c] + (1 - aR) * a;
                        const float e = juce::jmax (1.0e-6f, sEnv[c] * 1.15f), q = juce::jlimit (-1.0f, 1.0f, h / e);
                        u[i] = e * (0.55f * (2 * q * q - 1) + 0.3f * (4 * q * q * q - 3 * q));
                    }
                }
                sOs->processSamplesDown (blk);
                for (int c = 0; c < 2; ++c)
                {   // armoniche: tolgo la continua e le parti sotto i 3.5 kHz che l'inviluppo puo' creare
                    float* x = (c ? R : L) + off; float* hb2 = sBuf.getWritePointer (c);
                    for (int i = 0; i < nn; ++i) x[i] += k * 0.5f * sHp3[c] (hb2[i]);
                }
            }
        }
        // ---- GLUE: compressore di bus 2:1, attacco 10 ms, rilascio auto, filtro sui bassi nel sidechain
        if (S.glue > 0.001f)
        {
            BusSettings b; b.thr = -4.0f - 22.0f * S.glue; b.ratio = S.glue > 0.75f ? 2 : S.glue > 0.35f ? 1 : 0; b.att = 4; b.rel = 4; b.autoMakeup = true; b.makeup = 0; b.mix = 1; b.scHpf = 90; b.range = 9;
            bus.process (L, R, n, b); glueGr = bus.gr;
        }
        else glueGr = 0;
        // ---- WIDE: allarga (o stringe) il side sopra 120 Hz; sotto 120 Hz i bassi restano mono se MONO BASSI e acceso
        if (std::abs (S.wide) > 0.001f || S.monoBass)
        {
            float w = S.wide >= 0 ? 1.0f + 1.3f * S.wide : 1.0f + S.wide;
            // sicurezza mono: l'allargamento si ferma prima che i lati superino il centro (correlazione sotto ~0.1 = fuori fase)
            if (w > 1.0f && wM > 1e-10f)
            {
                const float wMax = std::sqrt (juce::jmax (1.0f, 0.82f * wM / juce::jmax (1e-12f, wS)));
                w = juce::jmin (w, wMax);
            }
            wSm += (w - wSm) * juce::jmin (1.0f, (float) n / (float) (0.3 * sr));
            double am = 0, as = 0;
            for (int i = 0; i < n; ++i)
            {
                const float m = 0.5f * (L[i] + R[i]), s = 0.5f * (L[i] - R[i]);
                const float sLo = wLp2 (wLp (s)), sHi = s - sLo;
                { const float mHi = m - mLp2 (mLp (m)); am += mHi * mHi; as += sHi * sHi; }
                const float s2 = (S.monoBass ? 0.0f : sLo) + sHi * wSm;
                L[i] = m + s2; R[i] = m - s2;
            }
            const float k = (float) std::exp (-(double) n / (0.5 * sr));
            wM = k * wM + (1 - k) * (float) (am / n); wS = k * wS + (1 - k) * (float) (as / n);
        }
        last = S;
    }
    float calmNow = 0, glueGr = 0;
private:
    struct CalmBand { float f, q, knee, maxCut; Biquad det[2], cut[2]; float env = 0, slow = 0, g = 0; };
    double sr = 48000;
    Biquad eq[2][3], dq[2][3], tLp[2], tLp2[2], sHp[2], wLp, wLp2, mLp, mLp2;
    CalmBand calm[3] { { 280, 1.2f, 3.0f, 4.0f }, { 3200, 1.6f, 2.5f, 6.0f }, { 7000, 2.0f, 2.0f, 6.0f } };
    TapeMachine tape; BusComp bus; EasySettings last; float dcAvg[2] {}, eLo[2] {}, eSat[2] {}, tEnv[2] {}, tDc[2] {}, sEnv[2] {}; Biquad sHp3[2]; std::unique_ptr<juce::dsp::Oversampling<float>> sOs; juce::AudioBuffer<float> sBuf; int sMax = 1024; float wM = 0, wS = 0, wSm = 1;
};
}
