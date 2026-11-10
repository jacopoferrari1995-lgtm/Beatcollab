#pragma once
#include "MasterModules.h"

// Jackpot Master, fase 3: DE-ESSER, BUS COMP, MULTIBANDA (con OTT). Latenza zero.
namespace jms
{
// guadagno con ginocchio morbido (dB): riduzione per compressione verso il basso
inline float downGain (float lvl, float thr, float ratio, float knee)
{
    const float over = lvl - thr;
    if (over <= -knee * 0.5f) return 0.0f;
    if (over < knee * 0.5f) { const float x = over + knee * 0.5f; return -(1 - 1 / ratio) * x * x / (2 * knee); }
    return -(1 - 1 / ratio) * over;
}

// --------------------------------------------------------------------------- DE-ESSER (relativo: guarda quanto le sibilanti superano il resto)
struct DeEssSettings { float freq = 6500, thr = -12, range = 6; int ch = 0; };
class DeEsser
{
public:
    void prepare (double s)
    {
        sr = s; juce::dsp::ProcessSpec sp { sr, 512, 2 };
        xo.prepare (sp); xo.setType (juce::dsp::LinkwitzRileyFilterType::lowpass);
        eH.set (sr, 0.8, 70); eF.set (sr, 0.8, 70); lastF = -1; grSm = 0;
        rk = (float) std::exp (-1.0 / (0.06 * sr)); ak = (float) std::exp (-1.0 / (0.0008 * sr));
    }
    void process (float* L, float* R, int n, const DeEssSettings& S)
    {
        if (std::abs (S.freq - lastF) > 1)
        {
            lastF = S.freq; xo.setCutoffFrequency (S.freq * 0.8f);
            for (auto& d : det) d.set (Biquad::BPASS, S.freq, 0, 1.2, sr);
        }
        float grMaxB = 0; float relMax = -60;
        for (int i = 0; i < n; ++i)
        {
            const float M = 0.5f * (L[i] + R[i]), Sd = 0.5f * (L[i] - R[i]);
            const float bm = std::abs (det[0] (M)), bs = std::abs (det[1] (Sd));
            const float h = eH (S.ch == 1 ? bm : S.ch == 2 ? bs : juce::jmax (bm, bs));
            const float f = eF (S.ch == 1 ? std::abs (M) : S.ch == 2 ? std::abs (Sd) : juce::jmax (std::abs (M), std::abs (Sd)));
            const float rel = gToDb (h + 1e-7f) - gToDb (f + 1e-6f);
            relMax = juce::jmax (relMax, rel);
            float gr = juce::jlimit (-S.range, 0.0f, downGain (rel, S.thr, 4.0f, 4.0f));
            if (gToDb (h) < -60) gr = 0;
            grSm = gr < grSm ? ak * grSm + (1 - ak) * gr : rk * grSm + (1 - rk) * gr;
            grMaxB = juce::jmin (grMaxB, grSm);
            const float g = dbToG (grSm);
            // si abbassa solo la banda sopra la frequenza, sul canale scelto
            float mlo, mhi, slo, shi;
            xo.processSample (0, M, mlo, mhi); xo.processSample (1, Sd, slo, shi);
            const float Mo = mlo + mhi * (S.ch != 2 ? g : 1.0f), So = slo + shi * (S.ch != 1 ? g : 1.0f);
            L[i] = Mo + So; R[i] = Mo - So;
        }
        gr = -grMaxB; relNow = relMax;
    }
    float gr = 0, relNow = -60;
private:
    double sr = 48000; float lastF = -1, grSm = 0, ak = 0.9f, rk = 0.999f;
    juce::dsp::LinkwitzRileyFilter<float> xo; Biquad det[2]; Env eH, eF;
};

// --------------------------------------------------------------------------- BUS COMP (VCA da console, collegato in stereo)
struct BusSettings { float thr = -10, makeup = 0, mix = 1, scHpf = 0; int ratio = 1, att = 3, rel = 4; bool autoMakeup = true; };
inline float busRatio (int i) { static const float r[] = { 1.5f, 2, 4, 10 }; return r[juce::jlimit (0, 3, i)]; }
inline float busAtt (int i) { static const float a[] = { 0.1f, 0.3f, 1, 3, 10, 30 }; return a[juce::jlimit (0, 5, i)]; }
inline float busRel (int i) { static const float r[] = { 100, 300, 600, 1200, -1 }; return r[juce::jlimit (0, 4, i)]; }
class BusComp
{
public:
    void prepare (double s) { sr = s; for (auto& h : hp) h.reset(); lastHp = -1; env = 0; gdb = 0; slow = 0; colour[0] = colour[1] = 0; }
    void process (float* L, float* R, int n, const BusSettings& S)
    {
        if (std::abs (S.scHpf - lastHp) > 0.5f) { lastHp = S.scHpf; for (auto& h : hp) { h.set (Biquad::LCUT, juce::jmax (20.0f, S.scHpf), 0, 0.7, sr); h.reset(); } }
        const float ratio = busRatio (S.ratio), attMs = busAtt (S.att), relMs = busRel (S.rel);
        const float aK = (float) std::exp (-1.0 / (attMs * 0.001 * sr));
        const float rFast = (float) std::exp (-1.0 / ((relMs > 0 ? relMs : 120.0f) * 0.001 * sr)), rSlow = (float) std::exp (-1.0 / (1.5 * sr));
        const float dK = (float) std::exp (-1.0 / (0.002 * sr));   // rivelatore: picco smussato 2 ms
        // compensazione automatica: meta della riduzione alla soglia scelta (come fanno le console)
        const float mk = dbToG (S.makeup + (S.autoMakeup ? -downGain (0.0f, S.thr, ratio, 6.0f) * 0.5f : 0.0f));
        float grMaxB = 0;
        for (int i = 0; i < n; ++i)
        {
            float dl = L[i], dr = R[i];
            if (S.scHpf >= 21) { dl = hp[0] (dl); dr = hp[1] (dr); }
            const float a = juce::jmax (std::abs (dl), std::abs (dr));
            env = a > env ? a : dK * env + (1 - dK) * a;
            const float target = downGain (gToDb (env + 1e-9f), S.thr, ratio, 6.0f);
            if (target < gdb) gdb = aK * gdb + (1 - aK) * target;
            else if (relMs > 0) gdb = rFast * gdb + (1 - rFast) * target;
            else
            {   // AUTO: doppio rilascio, veloce sui picchi e lento sulla riduzione media
                slow = rSlow * slow + (1 - rSlow) * gdb;
                const float k = gdb < slow - 2.0f ? rFast : (float) std::exp (-1.0 / (0.6 * sr));
                gdb = k * gdb + (1 - k) * target;
            }
            grMaxB = juce::jmin (grMaxB, gdb);
            const float g = dbToG (gdb) * mk;
            const float xl = L[i] * g, xr = R[i] * g;
            L[i] = L[i] + (xl - L[i]) * S.mix; R[i] = R[i] + (xr - R[i]) * S.mix;
        }
        gr = -grMaxB; grNow = -gdb;
    }
    float gr = 0, grNow = 0;
private:
    double sr = 48000; Biquad hp[2]; float lastHp = -1, env = 0, gdb = 0, slow = 0, colour[2] {};
};

// --------------------------------------------------------------------------- MULTIBANDA (3 bande, giu + su, stile OTT)
constexpr int kMbBands = 3;
struct MbBand { float thr = -18, ratio = 3, up = 0, gain = 0; bool solo = false; };
struct MbSettings { float x1 = 120, x2 = 2500, speed = 0.5f, mix = 1; MbBand b[kMbBands]; };
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
            L[i] = anySolo ? ol : dl + (ol - dl) * S.mix; R[i] = anySolo ? orr : dr + (orr - dr) * S.mix;
        }
        for (int b = 0; b < kMbBands; ++b) { level[b] = lvlMax[b]; down[b] = -gMin[b]; up[b] = gMaxB[b]; }
    }
    float level[kMbBands] {}, down[kMbBands] {}, up[kMbBands] {};
private:
    double sr = 48000; float lx1 = -1, lx2 = -1, env[kMbBands] {}, gdb[kMbBands] {};
    juce::dsp::LinkwitzRileyFilter<float> x1, x2, ap;
};
}
