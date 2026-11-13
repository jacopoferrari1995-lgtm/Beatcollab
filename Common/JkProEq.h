#pragma once
// EQ a bande libere (stile Pro-Q), condiviso da Mixology e Master: solo, stereo/mid/side/L/R, dinamica e spettrale
#include <juce_dsp/juce_dsp.h>
#include <complex>

namespace jmx
{
constexpr int kBands = 12;
enum BandType { BT_BELL, BT_LSHELF, BT_HSHELF, BT_LCUT, BT_HCUT, BT_NOTCH, BT_BPASS, BT_TILT, kBandTypes };
inline const juce::StringArray& bandTypeNames() { static const juce::StringArray n { "BELL", "LOW SHELF", "HIGH SHELF", "LOW CUT", "HIGH CUT", "NOTCH", "BAND PASS", "TILT" }; return n; }
inline const juce::StringArray& bandChNames() { static const juce::StringArray n { "STEREO", "MID", "SIDE", "LEFT", "RIGHT" }; return n; }
inline const juce::StringArray& slopeNames() { static const juce::StringArray n { "12", "24", "48", "96" }; return n; }

struct BandSet { bool on = false, dyn = false, spec = false, solo = false; int type = BT_BELL, slope = 1, ch = 0; float f = 1000, g = 0, q = 1, thr = -30, range = -6; };
struct EqSet { BandSet b[kBands]; bool live = false; float att = 5, rel = 80; };

// biquad RBJ con risposta in frequenza per il grafico
struct BQ
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0; float z1[2] {}, z2[2] {};
    inline float run (float x, int c) { const float y = (float) b0 * x + z1[c]; z1[c] = (float) b1 * x - (float) a1 * y + z2[c]; z2[c] = (float) b2 * x - (float) a2 * y; return y; }
    void reset() { z1[0] = z1[1] = z2[0] = z2[1] = 0; }
    void flat() { b0 = 1; b1 = b2 = a1 = a2 = 0; }
    void design (int t, double sr, double f, double gDb, double q)
    {
        f = juce::jlimit (10.0, sr * 0.49, f); q = juce::jmax (0.05, q);
        const double w = juce::MathConstants<double>::twoPi * f / sr, cs = std::cos (w), sn = std::sin (w), al = sn / (2 * q), A = std::pow (10.0, gDb / 40.0);
        double B0 = 1, B1 = 0, B2 = 0, A0 = 1, A1 = 0, A2 = 0;
        switch (t)
        {
            case 0: B0 = 1 + al * A; B1 = -2 * cs; B2 = 1 - al * A; A0 = 1 + al / A; A1 = -2 * cs; A2 = 1 - al / A; break;                       // bell
            case 1: { const double s = 2 * std::sqrt (A) * al; B0 = A * ((A + 1) - (A - 1) * cs + s); B1 = 2 * A * ((A - 1) - (A + 1) * cs); B2 = A * ((A + 1) - (A - 1) * cs - s);
                      A0 = (A + 1) + (A - 1) * cs + s; A1 = -2 * ((A - 1) + (A + 1) * cs); A2 = (A + 1) + (A - 1) * cs - s; break; }                  // low shelf
            case 2: { const double s = 2 * std::sqrt (A) * al; B0 = A * ((A + 1) + (A - 1) * cs + s); B1 = -2 * A * ((A - 1) + (A + 1) * cs); B2 = A * ((A + 1) + (A - 1) * cs - s);
                      A0 = (A + 1) - (A - 1) * cs + s; A1 = 2 * ((A - 1) - (A + 1) * cs); A2 = (A + 1) - (A - 1) * cs - s; break; }                   // high shelf
            case 3: B0 = (1 + cs) / 2; B1 = -(1 + cs); B2 = (1 + cs) / 2; A0 = 1 + al; A1 = -2 * cs; A2 = 1 - al; break;                         // passa alto
            case 4: B0 = (1 - cs) / 2; B1 = 1 - cs; B2 = (1 - cs) / 2; A0 = 1 + al; A1 = -2 * cs; A2 = 1 - al; break;                             // passa basso
            case 5: B0 = 1; B1 = -2 * cs; B2 = 1; A0 = 1 + al; A1 = -2 * cs; A2 = 1 - al; break;                                                 // notch
            case 6: B0 = al; B1 = 0; B2 = -al; A0 = 1 + al; A1 = -2 * cs; A2 = 1 - al; break;                                                     // passa banda
        }
        b0 = B0 / A0; b1 = B1 / A0; b2 = B2 / A0; a1 = A1 / A0; a2 = A2 / A0;
    }
    double magDb (double f, double sr) const
    {
        const double w = juce::MathConstants<double>::twoPi * f / sr; const std::complex<double> z = std::polar (1.0, -w), z2 = z * z;
        return 20.0 * std::log10 (std::abs ((b0 + b1 * z + b2 * z2) / (1.0 + a1 * z + a2 * z2)) + 1e-12);
    }
};
inline int cutStages (int slope) { return slope == 0 ? 1 : slope == 1 ? 2 : slope == 2 ? 4 : 8; }
inline double cutQ (int slope, int k)
{   // Butterworth: Q delle sezioni
    const int n = cutStages (slope) * 2; return 1.0 / (2.0 * std::cos (juce::MathConstants<double>::pi * (2 * k + 1) / (2.0 * n)));
}
// filtri di una banda (fino a 8 sezioni per i tagli a 96 dB/ott)
inline int designBand (const BandSet& B, double sr, float gDb, BQ* out)
{
    switch (B.type)
    {
        case BT_LCUT: case BT_HCUT: { const int n = cutStages (B.slope); for (int k = 0; k < n; ++k) out[k].design (B.type == BT_LCUT ? 3 : 4, sr, B.f, 0, n == 1 ? juce::jlimit (0.3, 3.0, B.q * 0.7071) : cutQ (B.slope, k) * (k == n - 1 ? juce::jlimit (0.5, 2.0, (double) B.q) : 1.0)); return n; }
        case BT_TILT: out[0].design (1, sr, B.f, -gDb * 0.5, 0.5); out[1].design (2, sr, B.f, gDb * 0.5, 0.5); return 2;
        case BT_LSHELF: out[0].design (1, sr, B.f, gDb, juce::jmin (2.0f, B.q)); return 1;
        case BT_HSHELF: out[0].design (2, sr, B.f, gDb, juce::jmin (2.0f, B.q)); return 1;
        case BT_NOTCH: out[0].design (5, sr, B.f, 0, B.q); return 1;
        case BT_BPASS: out[0].design (6, sr, B.f, 0, B.q); return 1;
        default: out[0].design (0, sr, B.f, gDb, B.q); return 1;
    }
}
inline bool hasGain (int t) { return t == BT_BELL || t == BT_LSHELF || t == BT_HSHELF || t == BT_TILT; }

class ProEq
{
public:
    static constexpr int kView = 256;
    void prepare (double s, int maxBlock)
    {
        sr = s; for (auto& b : st) { for (auto& f : b.f) f.reset(); for (auto& f : b.det) f.reset(); b.solo.reset(); b.env = 0; b.dyn = 0; b.key = -1; }
        order = sr > 60000 ? 12 : 11; N = 1 << order; H = N / 4; fft = std::make_unique<juce::dsp::FFT> (order);
        win.resize ((size_t) N); for (int i = 0; i < N; ++i) win[(size_t) i] = std::sqrt (0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) i / (float) N));
        for (int c = 0; c < 2; ++c) { inQ[c].assign ((size_t) N, 0.0f); outQ[c].assign ((size_t) N, 0.0f); spec[c].assign ((size_t) N, {}); }
        cb.assign ((size_t) N, {}); red.assign ((size_t) (N / 2 + 1), 0.0f); w = 0; (void) maxBlock;
        for (auto& v : specView) v = 0;
    }
    bool spectralOn (const EqSet& S) const { if (S.live) return false; for (auto& b : S.b) if (b.on && b.spec && hasGain (b.type)) return true; return false; }
    int latency (const EqSet& S) const { return spectralOn (S) ? N : 0; }
    void process (float* L, float* R, int n, const EqSet& S)
    {
        int solo = -1; for (int k = 0; k < kBands; ++k) if (S.b[k].on && S.b[k].solo) solo = k;
        const float aA = 1.0f - std::exp (-1.0f / (0.001f * S.att * (float) sr)), aR = 1.0f - std::exp (-1.0f / (0.001f * S.rel * (float) sr));
        for (int k = 0; k < kBands; ++k)
        {
            const auto& B = S.b[k]; auto& T = st[k];
            const bool dynOn = B.dyn && hasGain (B.type) && ! (B.spec && ! S.live);
            const int key = (int) B.type * 7 + B.slope * 131 + juce::roundToInt (B.f * 10) * 3 + juce::roundToInt (B.q * 1000) * 11 + juce::roundToInt ((dynOn ? T.dyn + B.g : B.g) * 20) * 13;
            if (key != T.key) { T.key = key; T.n = designBand (B, sr, dynOn ? B.g + T.dyn : B.g, T.f); }
            const int dk = juce::roundToInt (B.f) + juce::roundToInt (B.q * 100) * 100003;
            if (dk != T.dk) { T.dk = dk; for (auto& d : T.det) d.design (6, sr, B.f, 0, juce::jmax (0.5f, B.q)); T.solo.design (6, sr, B.f, 0, (B.type == BT_BELL || B.type == BT_NOTCH || B.type == BT_BPASS) ? juce::jmax (0.4f, B.q) : 0.7); }
        }
        for (int i = 0; i < n; ++i)
        {
            float l = L[i], r = R[i];
            for (int k = 0; k < kBands; ++k)
            {
                const auto& B = S.b[k]; if (! B.on) continue; auto& T = st[k];
                if (k == solo) continue;
                const bool dynOn = B.dyn && hasGain (B.type) && ! (B.spec && ! S.live);
                if (dynOn)
                {   // la banda si muove solo quando la sua zona supera la soglia
                    float a[2]; get (B.ch, l, r, a); const float d0 = T.det[0].run (a[0], 0), d1 = B.ch == 0 ? T.det[1].run (a[1], 0) : 0.0f;
                    const float lev = juce::jmax (std::abs (d0), std::abs (d1));
                    T.env += (lev - T.env) * (lev > T.env ? aA : aR);
                    const float over = juce::Decibels::gainToDecibels (T.env, -100.0f) - B.thr;
                    const float amt = juce::jlimit (0.0f, 1.0f, over / 10.0f);
                    const float tgt = B.range * amt;
                    T.dyn += (tgt - T.dyn) * 0.01f;
                    if ((i & 15) == 0 && std::abs (T.dyn - T.dynC) > 0.05f) { T.dynC = T.dyn; designBand (B, sr, B.g + T.dyn, T.f); }
                }
                else T.dyn = 0;
                float a[2]; get (B.ch, l, r, a);
                for (int s = 0; s < T.n; ++s) { a[0] = T.f[s].run (a[0], 0); if (B.ch == 0) a[1] = T.f[s].run (a[1], 1); }
                put (B.ch, l, r, a);
            }
            if (solo >= 0)
            {   // ascolta solo la zona della banda
                const auto& B = S.b[solo]; auto& T = st[solo]; float a[2]; get (B.ch, l, r, a);
                a[0] = T.solo.run (a[0], 0); if (B.ch == 0) a[1] = T.solo.run (a[1], 1);
                if (B.ch == 0) { l = a[0]; r = a[1]; } else { float z[2] = { 0, 0 }; l = r = 0; put (B.ch, l, r, a); (void) z; }
            }
            L[i] = l; R[i] = r;
        }
        for (int k = 0; k < kBands; ++k) dynUi[k] = st[k].dyn;
        if (spectralOn (S)) spectral (L, R, n, S);
        else for (auto& v : specView) v = v * 0.9f;
    }
    std::atomic<float> dynUi[kBands] {};
    std::atomic<float> specView[kView] {};   // riduzione spettrale in dB (scala log 20 Hz - 20 kHz)
private:
    // canali: 0 stereo (l, r), 1 mid, 2 side, 3 left, 4 right
    static inline void get (int ch, float l, float r, float* a)
    {
        switch (ch) { case 1: a[0] = 0.5f * (l + r); break; case 2: a[0] = 0.5f * (l - r); break; case 3: a[0] = l; break; case 4: a[0] = r; break; default: a[0] = l; a[1] = r; }
    }
    static inline void put (int ch, float& l, float& r, const float* a)
    {
        switch (ch)
        {
            case 1: { const float s = 0.5f * (l - r); l = a[0] + s; r = a[0] - s; break; }
            case 2: { const float m = 0.5f * (l + r); l = m + a[0]; r = m - a[0]; break; }
            case 3: l = a[0]; break; case 4: r = a[0]; break; default: l = a[0]; r = a[1];
        }
    }
    void spectral (float* L, float* R, int n, const EqSet& S)
    {
        float* ch[2] = { L, R };
        for (int i = 0; i < n; ++i)
        {
            for (int c = 0; c < 2; ++c) { inQ[c][(size_t) (N - H + w)] = ch[c][i]; ch[c][i] = outQ[c][(size_t) w]; }
            if (++w >= H) { w = 0; frame (S); }
        }
    }
    void frame (const EqSet& S)
    {
        const int nb = N / 2 + 1; const float binHz = (float) sr / (float) N;
        for (int c = 0; c < 2; ++c) { for (int i = 0; i < N; ++i) cb[(size_t) i] = { inQ[c][(size_t) i] * win[(size_t) i], 0.0f }; fft->perform (cb.data(), spec[c].data(), false); }
        const float hop = (float) H / (float) sr; const float aA = 1.0f - std::exp (-hop / 0.005f), aR = 1.0f - std::exp (-hop / juce::jmax (0.02f, S.rel * 0.001f));
        // per ogni bin: riduzione (o spinta) data dalle bande spettrali che lo coprono, solo se quel bin supera la soglia
        for (int k = 1; k < nb; ++k)
        {
            const float f = k * binHz;
            const float p = 0.5f * (std::norm (spec[0][(size_t) k]) + std::norm (spec[1][(size_t) k]));
            const float lev = 10.0f * std::log10 (p + 1e-14f) - 20.0f * std::log10 ((float) N * 0.25f);   // circa dBFS per bin
            float t = 0;
            for (auto& B : S.b)
            {
                if (! (B.on && B.spec && hasGain (B.type))) continue;
                float wgt;
                const float oct = std::log2 (f / B.f);
                if (B.type == BT_BELL) wgt = std::exp (-0.5f * juce::square (oct * B.q * 1.2f));
                else if (B.type == BT_LSHELF) wgt = 1.0f / (1.0f + std::exp (oct * 4.0f));
                else if (B.type == BT_HSHELF) wgt = 1.0f / (1.0f + std::exp (-oct * 4.0f));
                else wgt = 1.0f;
                if (wgt < 0.01f) continue;
                const float over = lev - B.thr;
                if (over > 0) t += B.range * juce::jlimit (0.0f, 1.0f, over / 10.0f) * wgt;
            }
            red[(size_t) k] += (t - red[(size_t) k]) * (std::abs (t) > std::abs (red[(size_t) k]) ? aA : aR);
        }
        for (int c = 0; c < 2; ++c)
        {
            for (int k = 0; k < nb; ++k) { const float g = juce::Decibels::decibelsToGain (juce::jlimit (-30.0f, 18.0f, red[(size_t) k])); spec[c][(size_t) k] *= g; if (k > 0 && k < N / 2) spec[c][(size_t) (N - k)] *= g; }
            fft->perform (spec[c].data(), cb.data(), true);
            auto& oq = outQ[c];
            std::memmove (oq.data(), oq.data() + H, sizeof (float) * (size_t) (N - H)); std::fill (oq.begin() + (N - H), oq.end(), 0.0f);
            for (int i = 0; i < N; ++i) oq[(size_t) i] += cb[(size_t) i].real() * win[(size_t) i] * 0.5f;
            std::memmove (inQ[c].data(), inQ[c].data() + H, sizeof (float) * (size_t) (N - H));
        }
        for (int v = 0; v < kView; ++v) { const float f = 20.0f * std::pow (1000.0f, (float) v / (kView - 1)); specView[v] = red[(size_t) juce::jlimit (1, nb - 1, (int) (f / binHz))]; }
    }
    struct St { BQ f[8]; BQ det[2]; BQ solo; int n = 1, key = -1, dk = -1; float env = 0, dyn = 0, dynC = 0; } st[kBands];
    double sr = 44100; int order = 11, N = 2048, H = 512, w = 0;
    std::unique_ptr<juce::dsp::FFT> fft; std::vector<float> win, inQ[2], outQ[2], red; std::vector<std::complex<float>> spec[2], cb;
};
}
