#pragma once
// Moduli nuovi del Master: IMPACT (transienti per banda), SPETTRALE (risonanze + clarity), STABILIZER (equilibrio tonale verso il genere)
#include "MasterModules.h"
#include "MasterAnalysis.h"

namespace jms
{
// ---------------------------------------------------------------------------------------------------- IMPACT
// Quattro bande (tagli 120 Hz, 1 kHz, 6 kHz). In ogni banda un inviluppo veloce e uno lento: la differenza e' l'attacco.
// Valore positivo = piu' botta (si alza l'attacco), negativo = piu' morbido. Zero latenza, somma delle bande piatta.
struct ImpactSettings { float amt[4] {}; float speed = 0.5f; bool delta = false; };
class Impact
{
public:
    static constexpr int kB = 4;
    void prepare (double s, int maxBlock)
    {
        sr = s; const juce::dsp::ProcessSpec sp { s, (juce::uint32) juce::jmax (16, maxBlock), 2 };
        const float xo[3] = { 120.0f, 1000.0f, 6000.0f };
        for (int k = 0; k < 3; ++k) { lp[k].prepare (sp); lp[k].setType (juce::dsp::LinkwitzRileyFilterType::lowpass); lp[k].setCutoffFrequency (xo[k]); }
        for (int k = 0; k < 3; ++k) { apA[k].prepare (sp); apA[k].setType (juce::dsp::LinkwitzRileyFilterType::allpass); }
        apA[0].setCutoffFrequency (1000.0f); apA[1].setCutoffFrequency (6000.0f); apA[2].setCutoffFrequency (6000.0f);
        for (auto& b : st) b = {};
    }
    void process (float* L, float* R, int n, const ImpactSettings& S)
    {
        const float sp = juce::jlimit (0.0f, 1.0f, S.speed);
        const float fA = co (0.4f), fR = co (8.0f + 22.0f * (1 - sp)), sA = co (12.0f + 30.0f * (1 - sp)), sR = co (120.0f + 300.0f * (1 - sp)), gS = co (2.0f);
        for (int i = 0; i < n; ++i)
        {
            float in[2] = { L[i], R[i] }, out[2] = { 0, 0 }, band[2][kB];
            for (int c = 0; c < 2; ++c)
            {
                float b1, r1; lp[0].processSample (c, in[c], b1, r1);
                float b2, r2; lp[1].processSample (c, r1, b2, r2);
                float b3, b4; lp[2].processSample (c, r2, b3, b4);
                b1 = apA[1].processSample (c, apA[0].processSample (c, b1));   // la banda bassa prende la fase dei tagli sopra
                b2 = apA[2].processSample (c, b2);
                band[c][0] = b1; band[c][1] = b2; band[c][2] = b3; band[c][3] = b4;
            }
            for (int b = 0; b < kB; ++b)
            {
                auto& s = st[b]; const float a = juce::jmax (std::abs (band[0][b]), std::abs (band[1][b]));
                s.fast = a > s.fast ? fA * s.fast + (1 - fA) * a : fR * s.fast + (1 - fR) * a;
                s.slow = a > s.slow ? sA * s.slow + (1 - sA) * a : sR * s.slow + (1 - sR) * a;
                const float tr = juce::jlimit (0.0f, 12.0f, 20.0f * std::log10 ((s.fast + 1e-7f) / (s.slow + 1e-7f)));   // quanto l'attacco sporge
                const float amt = juce::jlimit (-1.0f, 1.0f, S.amt[b]);
                const float tgt = juce::jlimit (-9.0f, 6.0f, amt * tr * (amt > 0 ? 0.55f : 0.75f));
                s.gDb = gS * s.gDb + (1 - gS) * tgt;
                const float g = std::pow (10.0f, s.gDb * 0.05f);
                for (int c = 0; c < 2; ++c) out[c] += S.delta ? band[c][b] * (g - 1.0f) : band[c][b] * g;
                ui[b] = s.gDb;
            }
            L[i] = out[0]; R[i] = out[1];
        }
    }
    float ui[kB] {};
private:
    float co (float ms) const { return (float) std::exp (-1.0 / (0.001 * ms * sr)); }
    struct St { float fast = 0, slow = 0, gDb = 0; } st[kB];
    double sr = 48000;
    juce::dsp::LinkwitzRileyFilter<float> lp[3], apA[3];
};

// ---------------------------------------------------------------------------------------------------- SPETTRALE
// Banco di 28 bande (circa 1/3 d'ottava). Ogni banda misura il suo livello e lo confronta con la media delle vicine:
//  RISONANZE: dove una banda sporge oltre la SENSIBILITA', la abbassa solo finche' sporge (stile soothe / Spectral Shaper)
//  CLARITY:   avvicina le bande coperte alle vicine (le alza piano), cosi' i dettagli escono senza alzare il volume
// Filtri a campana a fase minima: latenza zero.
struct SpectralSettings { float res = 0, sens = 0.5f, clar = 0, lo = 120, hi = 12000; bool delta = false; };
class Spectral
{
public:
    static constexpr int kB = 28;
    void prepare (double s)
    {
        sr = s;
        for (int b = 0; b < kB; ++b)
        {
            fc[b] = 60.0f * std::pow (2.0f, b / 3.0f * 1.02f);
            for (int c = 0; c < 2; ++c) { det[b][c].set (Biquad::BPASS, fc[b], 0, 4.3, sr); det[b][c].reset(); eq[b][c].set (Biquad::BELL, fc[b], 0, 4.3, sr); eq[b][c].reset(); }
            env[b] = 0; gDb[b] = 0; ui[b] = 0;
        }
        cnt = 0;
    }
    void process (float* L, float* R, int n, const SpectralSettings& S)
    {
        const float eA = (float) std::exp (-1.0 / (0.004 * sr)), eR = (float) std::exp (-1.0 / (0.06 * sr));
        for (int i0 = 0; i0 < n; i0 += kSub)
        {
            const int m = juce::jmin (kSub, n - i0);
            // misura (sui campioni del sottoblocco)
            for (int b = 0; b < kB; ++b)
            {
                if (fc[b] > sr * 0.45) continue;
                float e = env[b];
                for (int i = i0; i < i0 + m; ++i)
                {
                    const float v = juce::jmax (std::abs (det[b][0] (L[i])), std::abs (det[b][1] (R[i])));
                    e = v > e ? eA * e + (1 - eA) * v : eR * e + (1 - eR) * v;
                }
                env[b] = e; lev[b] = 20.0f * std::log10 (e + 1e-7f) + 3.0f * std::log2 (fc[b] / 1000.0f);   // compensazione: i mix scendono verso gli alti
            }
            // confronto con le vicine e guadagni
            const float thr = 2.0f + 8.0f * (1.0f - S.sens);
            for (int b = 0; b < kB; ++b)
            {
                float tgt = 0;
                if (fc[b] >= S.lo && fc[b] <= S.hi && fc[b] < sr * 0.45)
                {
                    float s = 0; int c = 0; for (int k = b - 3; k <= b + 3; ++k) if (k != b && k >= 0 && k < kB && fc[k] < sr * 0.45) { s += lev[k]; ++c; }
                    const float ex = lev[b] - (c ? s / c : lev[b]);
                    if (ex > thr) tgt -= S.res * juce::jmin (9.0f, (ex - thr) * 0.8f);
                    if (env[b] > 1.0e-4f) tgt += S.clar * juce::jlimit (-2.0f, 4.0f, -ex * 0.45f);   // clarity: verso le vicine, piu' su che giu'
                }
                const float k = tgt < gDb[b] ? 0.35f : 0.06f;   // giu' veloce, su lento
                gDb[b] += (tgt - gDb[b]) * k; ui[b] = gDb[b];
                for (int c = 0; c < 2; ++c) eq[b][c].set (Biquad::BELL, fc[b], gDb[b] * 0.6f, 4.3, sr);
            }
            // applico
            for (int i = i0; i < i0 + m; ++i)
            {
                float x = L[i], y = R[i]; const float x0 = x, y0 = y;
                for (int b = 0; b < kB; ++b) if (std::abs (gDb[b]) > 0.02f && fc[b] < sr * 0.45) { x = eq[b][0] (x); y = eq[b][1] (y); }
                L[i] = S.delta ? x - x0 : x; R[i] = S.delta ? y - y0 : y;
            }
        }
    }
    float ui[kB] {}, fc[kB] {};
private:
    static constexpr int kSub = 32;
    double sr = 48000; int cnt = 0;
    Biquad det[kB][2], eq[kB][2]; float env[kB] {}, lev[kB] {}, gDb[kB] {};
};

// ---------------------------------------------------------------------------------------------------- STABILIZER
// Ascolta lo spettro medio (ultimi secondi), lo confronta con la curva del genere scelto e corregge piano le sei zone
// (sub, bassi, medio-bassi, medi, presenza, aria) con filtri larghi. Si muove lento: segue la canzone, non il colpo.
struct StabSettings { float amt = 0, speed = 0.5f; int genre = 0; bool delta = false; };
class Stabilizer
{
public:
    void prepare (double s)
    {
        sr = s; ana.reset(); fill = 0; for (auto& g : gDb) g = 0; for (auto& g : ui) g = 0; for (auto& d : diff) d = 0;
        setFilters (true);
    }
    void process (float* L, float* R, int n, const StabSettings& S)
    {
        for (int i = 0; i < n; ++i)
        {
            ring[(size_t) fill++] = 0.5f * (L[i] + R[i]);
            if (fill == 4096)
            {   // spettro in terzi, media mobile lenta, poi le sei zone
                const double k = 0.02 + 0.12 * S.speed; ana.addFrame (ring.data(), sr, k);
                std::copy (ring.begin() + 2048, ring.end(), ring.begin()); fill = 2048;
                if (ana.frames > 4)
                {
                    float th[kThirds], reg[kRegions]; ana.result (th); ThirdAnalyzer::regions (th, reg);
                    const auto& G = genre (S.genre);
                    for (int r = 0; r < kRegions; ++r) diff[r] = G.off[r] - reg[r];
                }
            }
        }
        // guadagni: verso la curva, al massimo +/- 4 dB, lentissimi
        const float k = (float) (n / sr) / (0.6f + 4.0f * (1 - S.speed));
        for (int r = 0; r < kRegions; ++r) { const float tgt = juce::jlimit (-4.0f, 4.0f, diff[r] * 0.7f) * S.amt; gDb[r] += (tgt - gDb[r]) * juce::jmin (1.0f, k); ui[r] = gDb[r]; }
        setFilters (false);
        for (int i = 0; i < n; ++i)
        {
            float x = L[i], y = R[i]; const float x0 = x, y0 = y;
            for (int r = 0; r < kRegions; ++r) { x = f[r][0] (x); y = f[r][1] (y); }
            L[i] = S.delta ? x - x0 : x; R[i] = S.delta ? y - y0 : y;
        }
    }
    float ui[kRegions] {}, diff[kRegions] {};
private:
    void setFilters (bool reset)
    {
        static const float fr[kRegions] = { 45, 120, 450, 1400, 3900, 9000 };
        for (int r = 0; r < kRegions; ++r) for (int c = 0; c < 2; ++c)
        {
            f[r][c].set (r == 0 ? Biquad::LSHELF : r == kRegions - 1 ? Biquad::HSHELF : Biquad::BELL, fr[r], gDb[r], r == 0 || r == kRegions - 1 ? 0.6 : 0.8, sr);
            if (reset) f[r][c].reset();
        }
    }
    double sr = 48000; ThirdAnalyzer ana; std::vector<float> ring = std::vector<float> (4096, 0.0f); int fill = 0;
    float gDb[kRegions] {}; Biquad f[kRegions][2];
};
}
