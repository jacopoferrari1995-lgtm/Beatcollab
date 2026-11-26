#pragma once
// PHANTOM: dalla tua voce escono quattro fantasmi che ti seguono.
//  SUSSURRO  la tua voce rifatta solo di fiato (vocoder a rumore)
//  OMBRA     un'altra te, piu' bassa e scura (pitch giu')
//  RIFLESSO  la voce al contrario, a pezzi a tempo (eco inversa)
//  ALONE     un'aura alta che si scioglie nello spazio (pitch su + coda lunga)
// Tutti i fantasmi passano da RITARDO, DERIVA (si spostano e ondeggiano), BUIO e SPAZIO.
// Livelli dei fantasmi a zero: la voce passa identica.
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <cmath>
#include <vector>

namespace jphn
{
constexpr int kGhosts = 4, kBands = 14;
struct Settings
{
    std::array<float, kGhosts> lvl {}, chr { 0.5f, 0.0f, 0.5f, 1.0f };
    float delayMs = 0, drift = 0.3f, dark = 0.3f, space = 0.4f, width = 0.7f, ghostDb = 0, dryDb = 0; double bpm = 120;
};
inline const char* ghostName (int k) { static const char* n[] = { "SUSSURRO", "OMBRA", "RIFLESSO", "ALONE" }; return n[juce::jlimit (0, kGhosts - 1, k)]; }

struct Svf
{
    float ic1 = 0, ic2 = 0, a1 = 0, a2 = 0, a3 = 0, k = 1.41f;
    void set (float fc, float sr, float q = 0.707f) { const float g = std::tan (juce::MathConstants<float>::pi * juce::jlimit (10.0f, sr * 0.45f, fc) / sr); k = 1.0f / q; a1 = 1.0f / (1.0f + g * (g + k)); a2 = g * a1; a3 = g * a2; }
    void tick (float x, float& l, float& b, float& h) { const float v3 = x - ic2, v1 = a1 * ic1 + a2 * v3, v2 = ic2 + a2 * ic1 + a3 * v3; ic1 = 2 * v1 - ic1; ic2 = 2 * v2 - ic2; l = v2; b = v1; h = x - k * v1 - v2; }
    float lp (float x) { float l, b, h; tick (x, l, b, h); return l; }
    float bp (float x) { float l, b, h; tick (x, l, b, h); return b; }
    float hp (float x) { float l, b, h; tick (x, l, b, h); return h; }
};
// pitch a due testine che scorrono (economico, un po' sporco: per i fantasmi va benissimo)
struct Shifter
{
    std::vector<float> buf; int w = 0, N = 1 << 13; double ph = 0; float win = 2400;
    void prepare (float sr) { buf.assign ((size_t) N, 0.0f); w = 0; ph = 0; win = 0.06f * sr; }
    float process (float x, float ratio)
    {
        buf[(size_t) w] = x;
        ph += (1.0 - ratio) / win; ph -= std::floor (ph);
        float o = 0;
        for (int t = 0; t < 2; ++t)
        {
            const double p = std::fmod (ph + t * 0.5, 1.0); const float d = (float) p * win + 2;
            const float rp = (float) w - d; const int i0 = (int) std::floor (rp); const float fr = rp - (float) i0;
            const float s = buf[(size_t) (i0 & (N - 1))] * (1 - fr) + buf[(size_t) ((i0 + 1) & (N - 1))] * fr;
            o += s * std::sin (juce::MathConstants<float>::pi * (float) p);
        }
        w = (w + 1) & (N - 1);
        return o * 0.75f;
    }
};

class Engine
{
public:
    void prepare (double s, int maxBlock)
    {
        sr = (float) s; mb = juce::jmax (32, maxBlock);
        for (auto* v : { &gL, &gR, &hL, &hR, &dg }) v->assign ((size_t) mb, 0.0f);
        for (int b = 0; b < kBands; ++b) { const float f = 180.0f * std::pow (9000.0f / 180.0f, b / (float) (kBands - 1)); an[(size_t) b].set (f, sr, 4.0f); syn[(size_t) b].set (f, sr, 4.0f); env[(size_t) b] = 0; }
        shOmbra.prepare (sr); shAlone.prepare (sr); shAlone2.prepare (sr);
        rv.assign (kRev, 0.0f); rw = 0; rph = 0;
        for (auto& d : gdl) d.assign (kDl, 0.0f); dw = 0;
        verb.setSampleRate (s); verb.reset(); halo.setSampleRate (s); halo.reset();
        { juce::Reverb::Parameters p; p.roomSize = 0.97f; p.damping = 0.25f; p.wetLevel = 1; p.dryLevel = 0; p.width = 1; halo.setParameters (p); }
        sm = {}; first = true; for (auto& f : dk) f = {}; for (auto& f : hpG) f = {}; hpG[0].set (220.0f, sr); hpG[1].set (220.0f, sr);
        for (auto* v : { &tmpL, &tmpR, &haloL, &haloR }) v->reserve ((size_t) mb); revTail = 0;
        rng.setSeed (1234);
    }
    std::array<std::atomic<float>, kGhosts> meter {};
    float lvlOf (int k) const { return meter[(size_t) k].load(); }

    void process (float* L, float* R, int n, const Settings& S)
    {
        for (int o = 0; o < n; o += mb) chunk (L + o, R + o, juce::jmin (mb, n - o), S);
    }
private:
    static constexpr int kRev = 1 << 18, kDl = 1 << 16;
    struct Sm { std::array<float, kGhosts> lvl {}, chr {}; float delay = 0, drift = 0, dark = 0, space = 0, width = 0, ghost = 1, dry = 1; };
    float sr = 44100; int mb = 512; bool first = true; Sm sm;
    std::array<Svf, kBands> an, syn; std::array<float, kBands> env {};
    Shifter shOmbra, shAlone, shAlone2;
    std::vector<float> rv; int rw = 0; double rph = 0;
    std::array<std::vector<float>, 2> gdl; int dw = 0;
    std::vector<float> gL, gR, hL, hR, dg;
    juce::Reverb verb, halo; std::array<Svf, 2> dk, hpG;
    juce::Random rng; double lfo = 0; int revTail = 0;
    std::array<float, kGhosts> pk {};

    static float soft (float x) { const float a = std::abs (x); return a < 0.6f ? x : std::copysign (0.6f + 0.6f * std::tanh ((a - 0.6f) / 0.6f), x); }
    static float sm1 (float& s, float t, float k) { s += (t - s) * k; return s; }

    void chunk (float* L, float* R, int n, const Settings& S)
    {
        const float k = 1.0f - std::exp (-1.0f / (0.03f * sr));
        const float ghostT = juce::Decibels::decibelsToGain (S.ghostDb), dryT = juce::Decibels::decibelsToGain (S.dryDb);
        if (first) { sm.chr = S.chr; sm.delay = S.delayMs; sm.ghost = ghostT; sm.dry = dryT; sm.drift = S.drift; sm.dark = S.dark; sm.space = S.space; sm.width = S.width; first = false; }
        float lvlSum = 0; for (float v : S.lvl) lvlSum += v; for (float v : sm.lvl) lvlSum += v;
        // lunghezza dei pezzi al contrario: da 1/16 a 1/2, a tempo
        const float beat = 60.0f / (float) juce::jlimit (40.0, 300.0, S.bpm) * sr;
        std::fill (pk.begin(), pk.end(), 0.0f);
        for (int i = 0; i < n; ++i)
        {
            for (int g = 0; g < kGhosts; ++g) { sm1 (sm.lvl[(size_t) g], S.lvl[(size_t) g], k); sm.chr[(size_t) g] += (S.chr[(size_t) g] - sm.chr[(size_t) g]) * 0.0008f; }
            sm.delay += (S.delayMs - sm.delay) * 0.0006f;
            const float drift = sm1 (sm.drift, S.drift, k), dark = sm1 (sm.dark, S.dark, k), space = sm1 (sm.space, S.space, k), width = sm1 (sm.width, S.width, k);
            const float gG = sm1 (sm.ghost, ghostT, k), dry = sm1 (sm.dry, dryT, k);
            dg[(size_t) i] = dry;
            const float x = 0.5f * (L[i] + R[i]);
            lfo += 0.11 / sr; if (lfo > 1) lfo -= 1;
            const float drf = (float) std::sin (juce::MathConstants<double>::twoPi * lfo);
            float gs[kGhosts] = {};
            // SUSSURRO: inviluppi a bande -> rumore filtrato nelle stesse bande
            if (sm.lvl[0] > 1e-5f || S.lvl[0] > 0)
            {
                const float air = sm.chr[0];
                float o = 0;
                for (int b = 0; b < kBands; ++b)
                {
                    const float e = std::abs (an[(size_t) b].bp (x));
                    env[(size_t) b] += (e - env[(size_t) b]) * (e > env[(size_t) b] ? 0.01f : 0.0015f) * (44100.0f / sr);
                    const float tilt = std::pow (2.0f, (b / (float) (kBands - 1) - 0.5f) * (air * 2 - 1) * 3.0f);
                    o += syn[(size_t) b].bp (rng.nextFloat() * 2 - 1) * env[(size_t) b] * tilt;
                }
                gs[0] = o * 1.9f;
            }
            // OMBRA: da un'ottava sotto (0) a una quarta sotto (1)
            if (sm.lvl[1] > 1e-5f || S.lvl[1] > 0)
            {
                const float semi = -12.0f + sm.chr[1] * 7.0f + drf * drift * 0.25f;
                gs[1] = shOmbra.process (x, std::pow (2.0f, semi / 12.0f));
            }
            else shOmbra.process (x, 1.0f);
            // RIFLESSO: pezzi a tempo suonati al contrario, due testine sfalsate di mezzo pezzo
            rv[(size_t) rw] = x;
            {
                static const float lens[5] = { 0.25f, 0.5f, 1.0f, 1.5f, 2.0f };
                const float segLen = juce::jlimit (512.0f, (float) kRev / 3, beat * lens[juce::jlimit (0, 4, juce::roundToInt (sm.chr[2] * 4))]);
                rph += 1.0 / segLen; if (rph >= 1) rph -= 1;
                float o = 0;
                if (sm.lvl[2] > 1e-5f || S.lvl[2] > 0)
                    for (int t = 0; t < 2; ++t)
                    {
                        const double p = std::fmod (rph + t * 0.5, 1.0);
                        const float back = (float) (2.0 * p) * segLen;   // all'indietro: la distanza cresce del doppio
                        const int idx = (rw - (int) back + kRev * 4) & (kRev - 1);
                        o += rv[(size_t) idx] * std::sin (juce::MathConstants<float>::pi * (float) p);
                    }
                gs[2] = o * 0.8f;
            }
            rw = (rw + 1) & (kRev - 1);
            // ALONE: quinta (0) -> ottava (1) sopra, va solo nella coda lunga
            if (sm.lvl[3] > 1e-5f || S.lvl[3] > 0)
            {
                const float semi = 7.0f + sm.chr[3] * 5.0f;
                gs[3] = shAlone.process (x, std::pow (2.0f, semi / 12.0f)) * 0.8f + shAlone2.process (x, 2.0f * std::pow (2.0f, semi / 12.0f)) * 0.2f;
            }
            for (int g = 0; g < kGhosts; ++g) pk[(size_t) g] = juce::jmax (pk[(size_t) g], std::abs (gs[g] * sm.lvl[(size_t) g]));
            // dove stanno: sussurro e ombra ai lati, riflesso che gira, alone ovunque
            const float p0 = -0.6f * width + drf * drift * 0.3f, p1 = 0.5f * width - drf * drift * 0.3f, p2 = (float) std::sin (juce::MathConstants<double>::twoPi * lfo * 3) * width * (0.3f + 0.7f * drift);
            auto panL = [] (float p) { return std::cos ((p + 1) * juce::MathConstants<float>::pi * 0.25f); };
            auto panR = [] (float p) { return std::sin ((p + 1) * juce::MathConstants<float>::pi * 0.25f); };
            const float l0 = gs[0] * sm.lvl[0], l1 = gs[1] * sm.lvl[1], l2 = gs[2] * sm.lvl[2];
            float a = l0 * panL (p0) + l1 * panL (p1) + l2 * panL (p2), b = l0 * panR (p0) + l1 * panR (p1) + l2 * panR (p2);
            // ritardo dei fantasmi (ti seguono)
            gdl[0][(size_t) dw] = a; gdl[1][(size_t) dw] = b;
            const float dd = juce::jlimit (0.0f, (float) kDl - 4, sm.delay * 0.001f * sr * (1.0f + 0.04f * drf * drift));
            for (int c = 0; c < 2; ++c)
            {
                const float rp = (float) dw - dd - (c == 1 ? 0.004f * sr * width : 0.0f); const int i0 = (int) std::floor (rp); const float fr = rp - (float) i0;
                const float v = gdl[(size_t) c][(size_t) (i0 & (kDl - 1))] * (1 - fr) + gdl[(size_t) c][(size_t) ((i0 + 1) & (kDl - 1))] * fr;
                (c == 0 ? a : b) = v;
            }
            dw = (dw + 1) & (kDl - 1);
            // buio: i fantasmi perdono luce
            if ((i & 15) == 0) { dk[0].set (16000.0f * std::pow (2.0f, -4.5f * dark), sr); dk[1].set (16000.0f * std::pow (2.0f, -4.5f * dark), sr); }
            a = dk[0].lp (a); b = dk[1].lp (b);
            gL[(size_t) i] = a * gG; gR[(size_t) i] = b * gG;
            hL[(size_t) i] = gs[3] * sm.lvl[3] * gG * 1.6f; hR[(size_t) i] = space * 0.6f;   // hR: quanto spazio (letto sotto)
        }
        // spazio: i fantasmi in una stanza, l'alone in una coda infinita
        if (lvlSum > 1e-5f) revTail = (int) (8.0f * sr); else revTail -= n;
        std::vector<float>& sL = gL; std::vector<float>& sR = gR;
        float sp = sm.space;
        {
            juce::Reverb::Parameters p; p.roomSize = 0.6f + 0.35f * sp; p.damping = 0.3f + 0.5f * sm.dark; p.wetLevel = 1; p.dryLevel = 0; p.width = 1; verb.setParameters (p);
        }
        if (revTail > 0)
        {
            tmpL.assign (sL.begin(), sL.begin() + n); tmpR.assign (sR.begin(), sR.begin() + n);
            for (int i = 0; i < n; ++i) { tmpL[(size_t) i] += hL[(size_t) i] * 0.3f; tmpR[(size_t) i] += hL[(size_t) i] * 0.3f; }
            verb.processStereo (tmpL.data(), tmpR.data(), n);
            haloL.assign (hL.begin(), hL.begin() + n); haloR.assign (hL.begin(), hL.begin() + n);
            halo.processStereo (haloL.data(), haloR.data(), n);
            for (int i = 0; i < n; ++i)
            {
                const float s2 = hR[(size_t) i];
                const float a = sL[(size_t) i] * (1.0f - 0.5f * s2) + tmpL[(size_t) i] * s2 * 0.45f + hpG[0].hp (haloL[(size_t) i]) * 0.22f;
                const float b = sR[(size_t) i] * (1.0f - 0.5f * s2) + tmpR[(size_t) i] * s2 * 0.45f + hpG[1].hp (haloR[(size_t) i]) * 0.22f;
                // i fantasmi non superano mai la voce di troppo: limitatore morbido sul loro bus
                L[i] = L[i] * dg[(size_t) i] + soft (a); R[i] = R[i] * dg[(size_t) i] + soft (b);
            }
        }
        else for (int i = 0; i < n; ++i) if (dg[(size_t) i] != 1.0f) { L[i] *= dg[(size_t) i]; R[i] *= dg[(size_t) i]; }
        for (int g = 0; g < kGhosts; ++g) meter[(size_t) g].store (juce::jmax (pk[(size_t) g], meter[(size_t) g].load() * 0.85f));
    }
    std::vector<float> tmpL, tmpR, haloL, haloR;
};
}
