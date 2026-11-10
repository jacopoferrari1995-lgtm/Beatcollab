#pragma once
#include "MasterDSP.h"
#include <complex>

// Jackpot Master, fase 2: BASS CONTROL, EQ DINAMICO, IMAGER (tutti a latenza zero)
namespace jms
{
// --------------------------------------------------------------------------- biquad (RBJ, doppia precisione per i bassi)
struct Biquad
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    inline float operator() (float x) { const double y = b0 * x + z1; z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y; return (float) y; }
    void reset() { z1 = z2 = 0; }
    enum Type { BELL, LSHELF, HSHELF, LCUT, HCUT, BPASS };
    void set (int type, double f, double gDb, double q, double sr)
    {
        f = juce::jlimit (10.0, sr * 0.49, f); q = juce::jmax (0.1, q);
        const double A = std::pow (10.0, gDb / 40.0), w = juce::MathConstants<double>::twoPi * f / sr, cs = std::cos (w), sn = std::sin (w), al = sn / (2 * q);
        double B0, B1, B2, A0, A1, A2;
        switch (type)
        {
            case LSHELF: { const double s2 = 2 * std::sqrt (A) * al; B0 = A * ((A + 1) - (A - 1) * cs + s2); B1 = 2 * A * ((A - 1) - (A + 1) * cs); B2 = A * ((A + 1) - (A - 1) * cs - s2); A0 = (A + 1) + (A - 1) * cs + s2; A1 = -2 * ((A - 1) + (A + 1) * cs); A2 = (A + 1) + (A - 1) * cs - s2; break; }
            case HSHELF: { const double s2 = 2 * std::sqrt (A) * al; B0 = A * ((A + 1) + (A - 1) * cs + s2); B1 = -2 * A * ((A - 1) + (A + 1) * cs); B2 = A * ((A + 1) + (A - 1) * cs - s2); A0 = (A + 1) - (A - 1) * cs + s2; A1 = 2 * ((A - 1) - (A + 1) * cs); A2 = (A + 1) - (A - 1) * cs - s2; break; }
            case LCUT: B0 = (1 + cs) / 2; B1 = -(1 + cs); B2 = (1 + cs) / 2; A0 = 1 + al; A1 = -2 * cs; A2 = 1 - al; break;
            case HCUT: B0 = (1 - cs) / 2; B1 = 1 - cs; B2 = (1 - cs) / 2; A0 = 1 + al; A1 = -2 * cs; A2 = 1 - al; break;
            case BPASS: B0 = al; B1 = 0; B2 = -al; A0 = 1 + al; A1 = -2 * cs; A2 = 1 - al; break;
            default: B0 = 1 + al * A; B1 = -2 * cs; B2 = 1 - al * A; A0 = 1 + al / A; A1 = -2 * cs; A2 = 1 - al / A; break;
        }
        b0 = B0 / A0; b1 = B1 / A0; b2 = B2 / A0; a1 = A1 / A0; a2 = A2 / A0;
    }
    double magDb (double f, double sr) const
    {
        const std::complex<double> z = std::polar (1.0, -juce::MathConstants<double>::twoPi * f / sr);
        const auto h = (b0 + b1 * z + b2 * z * z) / (1.0 + a1 * z + a2 * z * z);
        return 20 * std::log10 (std::abs (h) + 1e-12);
    }
};

// inviluppo a un polo (attacco / rilascio)
struct Env
{
    float a = 0, r = 0, v = 0;
    void set (double sr, double attMs, double relMs) { a = (float) std::exp (-1.0 / (attMs * 0.001 * sr)); r = (float) std::exp (-1.0 / (relMs * 0.001 * sr)); }
    inline float operator() (float x) { v = x > v ? a * v + (1 - a) * x : r * v + (1 - r) * x; return v; }
};

// --------------------------------------------------------------------------- BASS CONTROL
struct BassSettings { float freq = 120, monoBelow = 0, punch = 0, tail = 0, harm = 0, levelDb = 0, control = 0; };
class BassControl
{
public:
    void prepare (double s)
    {
        sr = s; juce::dsp::ProcessSpec sp { sr, 512, 2 };
        for (auto* x : { &xo, &mxM, &mxS }) { x->prepare (sp); x->setType (juce::dsp::LinkwitzRileyFilterType::lowpass); }
        eAf.set (sr, 0.4, 35); eAs.set (sr, 18, 35); eSf.set (sr, 1, 45); eSs.set (sr, 1, 320);
        eC.set (sr, 8, 140); eLong.set (sr, 1500, 2500);
        hHp.set (Biquad::LCUT, 90, 0, 0.7, sr); hHp2.set (Biquad::LCUT, 90, 0, 0.7, sr); hLp.set (Biquad::HCUT, 450, 0, 0.7, sr);
        lastF = lastM = -1;
    }
    void process (float* L, float* R, int n, const BassSettings& S)
    {
        if (std::abs (S.freq - lastF) > 0.5f) { xo.setCutoffFrequency (S.freq); lastF = S.freq; }
        const bool mono = S.monoBelow >= 20;
        if (mono && std::abs (S.monoBelow - lastM) > 0.5f) { mxM.setCutoffFrequency (S.monoBelow); mxS.setCutoffFrequency (S.monoBelow); lastM = S.monoBelow; }
        const float lvl = dbToG (S.levelDb), kP = S.punch * 1.6f, kT = S.tail * 1.4f, kC = S.control * 0.75f;
        float gMin = 1, gMax = 1; double eLo = 0;
        for (int i = 0; i < n; ++i)
        {
            float lo0, hi0, lo1, hi1;
            xo.processSample (0, L[i], lo0, hi0); xo.processSample (1, R[i], lo1, hi1);
            const float lm = 0.5f * (lo0 + lo1), a = std::abs (lm);
            // forma del transiente sulla sola banda bassa (collegata in stereo)
            float g = 1;
            if (std::abs (kP) > 1e-3f) { const float f = eAf (a), s = eAs (a); g *= std::pow (juce::jlimit (0.25f, 8.0f, (f + 1e-6f) / (s + 1e-6f)), kP); }
            if (std::abs (kT) > 1e-3f) { const float f = eSf (a), s = eSs (a); g *= std::pow (juce::jlimit (0.25f, 8.0f, (s + 1e-6f) / (f + 1e-6f)), kT); }
            // stabilizza: abbassa i colpi di basso che superano la media lunga
            if (kC > 1e-3f) { const float e = eC (a), lg = eLong (a); if (e > lg * 1.05f && lg > 1e-5f) g *= std::pow (lg * 1.05f / e, kC); }
            g = juce::jlimit (0.125f, 4.0f, g) * lvl;
            lo0 *= g; lo1 *= g;
            gMin = juce::jmin (gMin, g); gMax = juce::jmax (gMax, g);
            // armoniche: raddrizzatore + saturazione sul basso mono, filtrate 90-450 Hz
            float h = 0;
            if (S.harm > 1e-3f)
            {
                const float m = 0.5f * (lo0 + lo1);
                h = std::tanh (4.0f * std::abs (m)) * 0.5f + std::tanh (3.0f * m) * 0.5f;
                h = hLp (hHp2 (hHp (h))) * S.harm * 0.9f;
            }
            float l = lo0 + hi0 + h, r = lo1 + hi1 + h;
            eLo += (double) (lo0 * lo0 + lo1 * lo1);
            // mono sotto la frequenza scelta: si toglie il lato (S) sotto, il medio passa per lo stesso allpass
            if (mono)
            {
                const float M = 0.5f * (l + r), Sd = 0.5f * (l - r);
                float mlo, mhi, slo, shi;
                mxM.processSample (0, M, mlo, mhi); mxS.processSample (0, Sd, slo, shi);
                l = mlo + mhi + shi; r = mlo + mhi - shi;
            }
            L[i] = l; R[i] = r;
        }
        gainDb = gToDb (std::abs (gMin - 1) > std::abs (gMax - 1) ? gMin : gMax);
        lowRms = (float) std::sqrt (eLo / juce::jmax (1, 2 * n));
    }
    float gainDb = 0, lowRms = 0;
private:
    double sr = 48000; float lastF = -1, lastM = -1;
    juce::dsp::LinkwitzRileyFilter<float> xo, mxM, mxS;
    Env eAf, eAs, eSf, eSs, eC, eLong; Biquad hHp, hHp2, hLp;
};

// --------------------------------------------------------------------------- EQ DINAMICO (6 bande, stereo / mid / side)
constexpr int kEqBands = 6;
struct EqBand { bool on = true, dyn = false; int type = 0, ch = 0; float f = 1000, g = 0, q = 1, thr = -24, rng = -6; };
class DynEq
{
public:
    void prepare (double s)
    {
        sr = s;
        for (int b = 0; b < kEqBands; ++b) { for (auto& f : flt[b]) f.reset(); det[b].reset(); det2[b].reset(); env[b].set (sr, 4, 70); env[b].v = 0; dynNow[b] = 0; last[b] = {}; last[b].f = -1; }
    }
    void process (float* L, float* R, int n, const EqBand* B)
    {
        constexpr int kSub = 16;
        for (int off = 0; off < n; off += kSub)
        {
            const int m = juce::jmin (kSub, n - off);
            // aggiorna i coefficienti (il guadagno dinamico cambia ogni 16 campioni)
            for (int b = 0; b < kEqBands; ++b)
            {
                const auto& e = B[b];
                if (! e.on) continue;
                const bool cut = e.type == Biquad::LCUT || e.type == Biquad::HCUT;
                float gain = e.g + (e.dyn && ! cut ? dynNow[b] : 0.0f);
                if (std::abs (gain - lastG[b]) > 0.02f || e.f != last[b].f || e.q != last[b].q || e.type != last[b].type)
                {
                    for (int c = 0; c < 2; ++c) { const double z1 = flt[b][c].z1, z2 = flt[b][c].z2; flt[b][c].set (e.type, e.f, gain, cut ? 0.707 * e.q : e.q, sr); flt[b][c].z1 = z1; flt[b][c].z2 = z2; }
                    if (e.f != last[b].f || e.q != last[b].q) { det[b].set (Biquad::BPASS, e.f, 0, juce::jmax (0.7f, e.q), sr); det2[b] = det[b]; det2[b].reset(); }
                    last[b] = e; lastG[b] = gain;
                }
            }
            for (int i = off; i < off + m; ++i)
            {
                float M = 0.5f * (L[i] + R[i]), S = 0.5f * (L[i] - R[i]);
                for (int b = 0; b < kEqBands; ++b)
                {
                    const auto& e = B[b];
                    if (! e.on) continue;
                    if (e.dyn)
                    {
                        const float a = e.ch == 2 ? 0.0f : std::abs (det[b] (M)), c = e.ch == 1 ? 0.0f : std::abs (det2[b] (S));
                        env[b] (juce::jmax (a, c));
                    }
                    const bool active = e.type == Biquad::LCUT || e.type == Biquad::HCUT || std::abs (lastG[b]) > 0.01f;
                    if (! active) continue;
                    if (e.ch != 2) M = flt[b][0] (M);
                    if (e.ch != 1) S = flt[b][1] (S);
                }
                L[i] = M + S; R[i] = M - S;
            }
            // guadagno dinamico: sopra soglia si muove verso RANGE (pieno a 12 dB oltre)
            for (int b = 0; b < kEqBands; ++b)
            {
                const auto& e = B[b];
                if (! e.on || ! e.dyn) { dynNow[b] = 0; continue; }
                const float lvl = gToDb (env[b].v * 1.414f), over = juce::jlimit (0.0f, 1.0f, (lvl - e.thr) / 12.0f);
                dynNow[b] = e.rng * over;
                levelDb[b] = lvl;
            }
        }
    }
    float dynNow[kEqBands] {}, levelDb[kEqBands] {};
    double sampleRate() const { return sr; }
private:
    double sr = 48000;
    Biquad flt[kEqBands][2], det[kEqBands], det2[kEqBands]; Env env[kEqBands]; EqBand last[kEqBands]; float lastG[kEqBands] { 99, 99, 99, 99, 99, 99 };
};

// --------------------------------------------------------------------------- IMAGER (4 bande, M/S)
constexpr int kImBands = 4;
struct ImagerSettings { float x[3] { 120, 900, 5000 }; float w[kImBands] { 1, 1, 1, 1 }; float stereoize = 0; };
class Imager
{
public:
    void prepare (double s)
    {
        sr = s; juce::dsp::ProcessSpec sp { sr, 512, 2 };
        for (auto& f : xs) { f.prepare (sp); f.setType (juce::dsp::LinkwitzRileyFilterType::lowpass); }
        for (auto& f : ap) { f.prepare (sp); f.setType (juce::dsp::LinkwitzRileyFilterType::allpass); }
        // decorrelatore: catena di allpass di Schroeder
        static const float d[4] = { 3.1f, 4.7f, 7.3f, 11.1f };
        for (int k = 0; k < 4; ++k) { dl[k].assign ((size_t) juce::jmax (2, (int) (d[k] * 0.001 * sr)), 0.0f); dp[k] = 0; }
        dHp.set (Biquad::LCUT, 300, 0, 0.7, sr);
        for (auto& v : eM) v = 0; for (auto& v : eS) v = 0;
        for (auto& v : last) v = -1;
    }
    void process (float* L, float* R, int n, const ImagerSettings& S)
    {
        for (int k = 0; k < 3; ++k)
            if (std::abs (S.x[k] - last[k]) > 0.5f)
            {
                last[k] = S.x[k]; xs[k].setCutoffFrequency (S.x[k]);
                // allpass di compensazione: banda 1 passa per gli allpass dei tagli 2 e 3, banda 2 per il 3
                ap[0].setCutoffFrequency (S.x[1]); ap[1].setCutoffFrequency (S.x[2]); ap[2].setCutoffFrequency (S.x[2]);
            }
        double em[kImBands] {}, es[kImBands] {};
        for (int i = 0; i < n; ++i)
        {
            const float M = 0.5f * (L[i] + R[i]), Sd = 0.5f * (L[i] - R[i]);
            float sb[kImBands], mb[kImBands];
            split (M, Sd, sb, mb);
            float so = 0, mo = 0;
            for (int b = 0; b < kImBands; ++b) { sb[b] *= S.w[b]; so += sb[b]; mo += mb[b]; em[b] += mb[b] * mb[b]; es[b] += sb[b] * sb[b]; }
            // stereoize: aggiunge al lato una versione decorrelata del medio sopra 300 Hz (sparisce in mono)
            if (S.stereoize > 1e-3f) so += decor (dHp (mo)) * S.stereoize * 0.6f;
            L[i] = mo + so; R[i] = mo - so;
        }
        for (int b = 0; b < kImBands; ++b) { const float k = 0.85f; eM[b] = k * eM[b] + (1 - k) * (float) em[b]; eS[b] = k * eS[b] + (1 - k) * (float) es[b]; }
    }
    // correlazione stimata per banda (+1 mono, 0 largo, <0 fuori fase)
    float corr (int b) const { const float t = eM[b] + eS[b]; return t < 1e-12f ? 1.0f : (eM[b] - eS[b]) / t; }
private:
    void split (float M, float Sd, float* sb, float* mb)
    {
        float lo, hi, lo2, hi2, lo3, hi3;
        // lato (S)
        xs[0].processSample (0, Sd, lo, hi); xs[1].processSample (0, hi, lo2, hi2); xs[2].processSample (0, hi2, lo3, hi3);
        sb[0] = ap[1].processSample (0, ap[0].processSample (0, lo)); sb[1] = ap[2].processSample (0, lo2); sb[2] = lo3; sb[3] = hi3;
        // medio (M): stessa struttura, canale 1 dei filtri
        xs[0].processSample (1, M, lo, hi); xs[1].processSample (1, hi, lo2, hi2); xs[2].processSample (1, hi2, lo3, hi3);
        mb[0] = ap[1].processSample (1, ap[0].processSample (1, lo)); mb[1] = ap[2].processSample (1, lo2); mb[2] = lo3; mb[3] = hi3;
    }
    inline float decor (float x)
    {
        for (int k = 0; k < 4; ++k)
        {
            auto& b = dl[k]; const float d = b[(size_t) dp[k]], y = -0.5f * x + d;
            b[(size_t) dp[k]] = x + 0.5f * y; dp[k] = (dp[k] + 1) % (int) b.size(); x = y;
        }
        return x;
    }
    double sr = 48000; float last[3] { -1, -1, -1 };
    juce::dsp::LinkwitzRileyFilter<float> xs[3], ap[3];
    std::vector<float> dl[4]; int dp[4] {}; Biquad dHp; float eM[kImBands] {}, eS[kImBands] {};
};
}
