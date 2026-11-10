#pragma once
#include "MasterModules.h"

// Jackpot Master, fase 3: DE-ESSER, BUS COMP, MULTIBANDA (con OTT). Latenza zero.
namespace jms
{
// --------------------------------------------------------------------------- DE-ESSER (relativo, su un intervallo scelto)
// Abbassa solo l'intervallo DA..A (o da DA in su) quando sporge rispetto al resto del mix.
struct DeEssSettings { float lo = 5000, hi = 20000, thr = -12, amount = 0.5f, soft = 0.5f; int ch = 0; bool delta = false; };
class DeEsser
{
public:
    void prepare (double s)
    {
        sr = s; juce::dsp::ProcessSpec sp { sr, 512, 2 };
        for (auto* x : { &x1, &x2 }) { x->prepare (sp); x->setType (juce::dsp::LinkwitzRileyFilterType::lowpass); }
        apLo.prepare (sp); apLo.setType (juce::dsp::LinkwitzRileyFilterType::allpass);
        eF.set (sr, 1.0, 80); lastLo = lastHi = -1; grSm = 0; bm = bs = 0;
    }
    void process (float* L, float* R, int n, const DeEssSettings& S)
    {
        const bool open = S.hi >= 19000;
        if (std::abs (S.lo - lastLo) > 1) { lastLo = S.lo; x1.setCutoffFrequency (S.lo); }
        if (! open && std::abs (S.hi - lastHi) > 1) { lastHi = S.hi; x2.setCutoffFrequency (S.hi); apLo.setCutoffFrequency (S.hi); }
        const float ratio = 1.0f + S.amount * 5.0f, maxRed = 2.0f + S.amount * 8.0f, knee = 2.0f + S.soft * 10.0f;
        const float ak = (float) std::exp (-1.0 / ((0.0005 + 0.002 * S.soft) * sr)), rk = (float) std::exp (-1.0 / ((0.04 + 0.16 * S.soft) * sr));
        const float bk = (float) std::exp (-1.0 / (0.004 * sr));
        float grMaxB = 0, relMax = -60;
        for (int i = 0; i < n; ++i)
        {
            const float M = 0.5f * (L[i] + R[i]), Sd = 0.5f * (L[i] - R[i]);
            float lo[2], band[2], top[2]; const float in[2] = { M, Sd };
            for (int c = 0; c < 2; ++c)
            {
                float l, h; x1.processSample (c, in[c], l, h);
                if (open) { lo[c] = l; band[c] = h; top[c] = 0; }
                else { float b2, t2; x2.processSample (c, h, b2, t2); band[c] = b2; top[c] = t2; lo[c] = apLo.processSample (c, l); }
            }
            // livello dell'intervallo rispetto al mix, sul canale scelto
            const float ab = S.ch == 1 ? std::abs (band[0]) : S.ch == 2 ? std::abs (band[1]) : juce::jmax (std::abs (band[0]), std::abs (band[1]));
            const float af = S.ch == 1 ? std::abs (M) : S.ch == 2 ? std::abs (Sd) : juce::jmax (std::abs (M), std::abs (Sd));
            bm = ab > bm ? ab : bk * bm + (1 - bk) * ab;
            const float f = eF (af);
            const float rel = gToDb (bm + 1e-7f) - gToDb (f + 1e-6f);
            relMax = juce::jmax (relMax, rel);
            float gr = gToDb (bm) < -60 ? 0.0f : juce::jmax (-maxRed, downGain (rel, S.thr, ratio, knee));
            grSm = gr < grSm ? ak * grSm + (1 - ak) * gr : rk * grSm + (1 - rk) * gr;
            grMaxB = juce::jmin (grMaxB, grSm);
            const float g = dbToG (grSm);
            const float gM = S.ch != 2 ? g : 1.0f, gS = S.ch != 1 ? g : 1.0f;
            float Mo, So;
            if (S.delta) { Mo = band[0] * (gM - 1); So = band[1] * (gS - 1); }
            else { Mo = lo[0] + band[0] * gM + top[0]; So = lo[1] + band[1] * gS + top[1]; }
            L[i] = Mo + So; R[i] = Mo - So;
        }
        gr = -grMaxB; relNow = relMax;
    }
    float gr = 0, relNow = -60;
private:
    double sr = 48000; float lastLo = -1, lastHi = -1, grSm = 0, bm = 0, bs = 0;
    juce::dsp::LinkwitzRileyFilter<float> x1, x2, apLo; Env eF;
};

// --------------------------------------------------------------------------- BUS COMP (stile console, morbido come i "glue" migliori)
// Rivelatore RMS (non di picco), ginocchio largo, RANGE che limita la riduzione massima. Collegato in stereo.
struct BusSettings { float thr = 0, makeup = 0, mix = 1, scHpf = 0, range = 12; int ratio = 1, att = 4, rel = 4; bool autoMakeup = true; };
inline float busRatio (int i) { static const float r[] = { 1.5f, 2, 4, 10 }; return r[juce::jlimit (0, 3, i)]; }
inline float busAtt (int i) { static const float a[] = { 0.1f, 0.3f, 1, 3, 10, 30 }; return a[juce::jlimit (0, 5, i)]; }
inline float busRel (int i) { static const float r[] = { 100, 300, 600, 1200, -1 }; return r[juce::jlimit (0, 4, i)]; }
class BusComp
{
public:
    void prepare (double s) { sr = s; for (auto& h : hp) h.reset(); lastHp = -1; ms = 0; gdb = 0; slow = 0; }
    void process (float* L, float* R, int n, const BusSettings& S)
    {
        if (std::abs (S.scHpf - lastHp) > 0.5f) { lastHp = S.scHpf; for (auto& h : hp) { h.set (Biquad::LCUT, juce::jmax (20.0f, S.scHpf), 0, 0.7, sr); h.reset(); } }
        const float ratio = busRatio (S.ratio), attMs = busAtt (S.att), relMs = busRel (S.rel);
        const float aK = (float) std::exp (-1.0 / (juce::jmax (0.3f, attMs) * 0.001 * sr));
        const float rFast = (float) std::exp (-1.0 / ((relMs > 0 ? relMs : 150.0f) * 0.001 * sr)), rSlow = (float) std::exp (-1.0 / (1.2 * sr));
        const float msK = (float) std::exp (-1.0 / (0.012 * sr));                 // finestra RMS 12 ms
        const float knee = 10.0f;
        const float mk = dbToG (S.makeup + (S.autoMakeup ? juce::jmin (S.range, -downGain (-6.0f, S.thr, ratio, knee)) * 0.6f : 0.0f));
        float grMaxB = 0;
        for (int i = 0; i < n; ++i)
        {
            float dl = L[i], dr = R[i];
            if (S.scHpf >= 21) { dl = hp[0] (dl); dr = hp[1] (dr); }
            ms = msK * ms + (1 - msK) * 0.5f * (dl * dl + dr * dr);
            const float lvl = 10.0f * std::log10 (ms * 2.0f + 1e-12f);              // RMS, riferito al picco di una sinusoide
            const float target = juce::jmax (-S.range, downGain (lvl, S.thr, ratio, knee));
            if (target < gdb) gdb = aK * gdb + (1 - aK) * target;
            else if (relMs > 0) gdb = rFast * gdb + (1 - rFast) * target;
            else
            {   // AUTO: veloce sui colpi isolati, lento quando la compressione e costante
                slow = rSlow * slow + (1 - rSlow) * gdb;
                const float k = gdb < slow - 1.5f ? rFast : (float) std::exp (-1.0 / (0.7 * sr));
                gdb = k * gdb + (1 - k) * target;
            }
            grMaxB = juce::jmin (grMaxB, gdb);
            const float g = dbToG (gdb) * mk;
            L[i] = L[i] + (L[i] * g - L[i]) * S.mix; R[i] = R[i] + (R[i] * g - R[i]) * S.mix;
        }
        gr = -grMaxB;
    }
    float gr = 0;
private:
    double sr = 48000; Biquad hp[2]; float lastHp = -1, ms = 0, gdb = 0, slow = 0;
};

// --------------------------------------------------------------------------- MULTIBANDA (3 bande, giu + su, stile OTT)
constexpr int kMbBands = 3;
struct MbBand { float thr = -18, ratio = 3, up = 0, gain = 0; bool solo = false; };
struct MbSettings { float x1 = 120, x2 = 2500, speed = 0.5f, mix = 1; MbBand b[kMbBands]; bool delta = false; };
class Multiband
{
public:
    void prepare (double s)
    {
        sr = s; juce::dsp::ProcessSpec sp { sr, 512, 2 };
        for (auto* f : { &x1, &x2 }) { f->prepare (sp); f->setType (juce::dsp::LinkwitzRileyFilterType::lowpass); }
        ap.prepare (sp); ap.setType (juce::dsp::LinkwitzRileyFilterType::allpass);
        for (auto& e : env) e = 0; for (auto& g : gdb) g = 0; lx1 = lx2 = -1;
    }
    void process (float* L, float* R, int n, const MbSettings& S)
    {
        if (std::abs (S.x1 - lx1) > 0.5f) { lx1 = S.x1; x1.setCutoffFrequency (S.x1); }
        if (std::abs (S.x2 - lx2) > 0.5f) { lx2 = S.x2; x2.setCutoffFrequency (S.x2); ap.setCutoffFrequency (S.x2); }
        // tempi: bassi piu lenti, alti piu veloci; VELOCITA li scala
        const float sp = std::pow (4.0f, 0.5f - S.speed);   // x2 .. x0.5
        static const float baseAtt[kMbBands] = { 12, 6, 3 }, baseRel[kMbBands] = { 220, 140, 90 };
        float aK[kMbBands], rK[kMbBands];
        for (int b = 0; b < kMbBands; ++b) { aK[b] = (float) std::exp (-1.0 / (baseAtt[b] * sp * 0.001 * sr)); rK[b] = (float) std::exp (-1.0 / (baseRel[b] * sp * 0.001 * sr)); }
        bool anySolo = false; for (auto& b : S.b) anySolo |= b.solo;
        float lvlMax[kMbBands] { -100, -100, -100 }, gMin[kMbBands] {}, gMaxB[kMbBands] {};
        for (int i = 0; i < n; ++i)
        {
            float bl[kMbBands], br[kMbBands], lo, hi;
            x1.processSample (0, L[i], lo, hi); x2.processSample (0, hi, bl[1], bl[2]); bl[0] = ap.processSample (0, lo);
            x1.processSample (1, R[i], lo, hi); x2.processSample (1, hi, br[1], br[2]); br[0] = ap.processSample (1, lo);
            float ol = 0, orr = 0;
            for (int b = 0; b < kMbBands; ++b)
            {
                const auto& B = S.b[b];
                const float a = juce::jmax (std::abs (bl[b]), std::abs (br[b]));
                env[b] = a > env[b] ? aK[b] * env[b] + (1 - aK[b]) * a : rK[b] * env[b] + (1 - rK[b]) * a;
                const float lvl = gToDb (env[b] * 1.2f + 1e-9f);
                // giu sopra soglia, su sotto (soglia - 18 dB), mai sotto -60 dBFS (il rumore non si alza)
                float t = downGain (lvl, B.thr, B.ratio, 6.0f);
                if (B.up > 1e-3f && lvl > -60)
                {
                    const float ut = B.thr - 18.0f, ur = 1.0f + B.up * 3.0f;
                    if (lvl < ut) t += juce::jmin (18.0f * B.up + 2.0f, (ut - lvl) * (1 - 1 / ur));
                }
                gdb[b] += (t - gdb[b]) * (t < gdb[b] ? (1 - aK[b]) : (1 - rK[b])) * 4.0f;
                const float g = dbToG (gdb[b] + B.gain);
                lvlMax[b] = juce::jmax (lvlMax[b], lvl); gMin[b] = juce::jmin (gMin[b], gdb[b]); gMaxB[b] = juce::jmax (gMaxB[b], gdb[b]);
                if (anySolo && ! B.solo) continue;
                ol += bl[b] * g; orr += br[b] * g;
            }
            // MIX: parallelo col segnale gia passato dagli allpass (somma piatta)
            const float dl = bl[0] + bl[1] + bl[2], dr = br[0] + br[1] + br[2];
            if (S.delta) { L[i] = (ol - dl) * S.mix; R[i] = (orr - dr) * S.mix; }
            else { L[i] = anySolo ? ol : dl + (ol - dl) * S.mix; R[i] = anySolo ? orr : dr + (orr - dr) * S.mix; }
        }
        for (int b = 0; b < kMbBands; ++b) { level[b] = lvlMax[b]; down[b] = -gMin[b]; up[b] = gMaxB[b]; }
    }
    float level[kMbBands] {}, down[kMbBands] {}, up[kMbBands] {};
private:
    double sr = 48000; float lx1 = -1, lx2 = -1, env[kMbBands] {}, gdb[kMbBands] {};
    juce::dsp::LinkwitzRileyFilter<float> x1, x2, ap;
};
}
