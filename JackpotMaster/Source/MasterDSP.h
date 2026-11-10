#pragma once
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <cmath>

// Jackpot Master: blocchi DSP di base (fase 1)
namespace jms
{
inline float dbToG (float db) { return std::pow (10.0f, db * 0.05f); }
inline float gToDb (float g) { return 20.0f * std::log10 (juce::jmax (1.0e-9f, g)); }

// --------------------------------------------------------------------------- ritardo
struct Delay
{
    std::vector<float> b; int p = 0, n = 0;
    void prepare (int len) { n = juce::jmax (0, len); b.assign ((size_t) juce::jmax (1, n), 0.0f); p = 0; }
    void reset() { std::fill (b.begin(), b.end(), 0.0f); p = 0; }
    inline float operator() (float x) { if (n == 0) return x; const float y = b[(size_t) p]; b[(size_t) p] = x; if (++p >= n) p = 0; return y; }
};

// --------------------------------------------------------------------------- motore di guadagno con lookahead
// minimo su finestra (L+1) -> rilascio adattivo in dB -> media mobile su L: il guadagno arriva
// in modo liscio e garantito sotto il richiesto prima del picco (nessun overshoot).
class GainEngine
{
public:
    void prepare (double fs_, int maxL)
    {
        fs = fs_; cap = maxL + 2;
        q.assign ((size_t) cap * 2, 0); qv.assign ((size_t) cap * 2, 1.0f);
        ring.assign ((size_t) cap, 0.0f); setLookahead (maxL); reset();
    }
    void setLookahead (int l)
    {
        l = juce::jlimit (1, cap - 2, l);
        if (l == L) return;
        L = l; reset();
    }
    void setRelease (float fastMs, float slowMs) { relF = coef (fastMs); relS = coef (slowMs); }
    void reset()
    {
        head = tail = 0; t = 0; hr = 0; sum = 0; rp = 0; avg = 0; std::fill (ring.begin(), ring.end(), 0.0f); grNow = 0;
        sustain = 0; sustK = coef (450.0f);
    }
    // r: guadagno richiesto (lineare, <= 1). Ritorna il guadagno da applicare al campione ritardato di L.
    inline float tick (float r)
    {
        const float rdb = gToDb (juce::jmin (1.0f, r));
        // minimo sulla finestra [t-L, t] con coda monotona
        while (head != tail && qv[(size_t) ((tail - 1 + 2 * cap) % (2 * cap))] >= rdb) tail = (tail - 1 + 2 * cap) % (2 * cap);
        q[(size_t) tail] = t; qv[(size_t) tail] = rdb; tail = (tail + 1) % (2 * cap);
        while (q[(size_t) head] < t - L) head = (head + 1) % (2 * cap);
        const float h = qv[(size_t) head];
        ++t;
        // rilascio adattivo: veloce sui picchi isolati, lento quando la riduzione e sostenuta
        sustain = sustK * sustain + (1 - sustK) * (-h);
        if (h < hr) hr = h;
        else { const float s = juce::jlimit (0.0f, 1.0f, sustain / 3.0f); const float k = relF + (relS - relF) * s; hr = h + (hr - h) * k; }
        // media mobile su L
        sum += hr - ring[(size_t) rp]; ring[(size_t) rp] = hr; rp = (rp + 1) % L;
        if (++resync >= 65536) { resync = 0; double s2 = 0; for (int i = 0; i < L; ++i) s2 += ring[(size_t) i]; sum = s2; }
        const float gdb = (float) (sum / L);
        grNow = -gdb;
        return dbToG (gdb);
    }
    int lookahead() const { return L; }
    float grNow = 0;
private:
    float coef (float ms) const { return (float) std::exp (-1.0 / (juce::jmax (0.05, (double) ms) * 0.001 * fs)); }
    double fs = 48000; int cap = 8, L = -1, head = 0, tail = 0, rp = 0, resync = 0; long long t = 0;
    std::vector<long long> q; std::vector<float> qv, ring;
    float hr = 0, relF = 0.99f, relS = 0.999f, sustain = 0, sustK = 0.999f, avg = 0; double sum = 0;
};

// --------------------------------------------------------------------------- stima del true peak (interpolazione 4x polifase, 12 prese per fase)
class TruePeak
{
public:
    static constexpr int kTaps = 48, kPh = 8, kDelay = kTaps / 2;   // il picco restituito riguarda il campione di kDelay campioni fa
    float center = 0;                                                // picco del solo campione (senza interpolazione)
    TruePeak()
    {
        for (int p = 1; p < kPh; ++p)
            for (int k = 0; k < kTaps; ++k)
            {
                const double x = (k - kTaps / 2 + 1) - p / (double) kPh;   // posizione relativa all'uscita
                const double sinc = std::abs (x) < 1e-9 ? 1.0 : std::sin (juce::MathConstants<double>::pi * x) / (juce::MathConstants<double>::pi * x);
                const double w = kaiser (x, kTaps * 0.5, 8.0);
                h[p][k] = (float) (sinc * w);
            }
        for (int p = 1; p < kPh; ++p) { double sm = 0; for (int k = 0; k < kTaps; ++k) sm += h[p][k]; for (int k = 0; k < kTaps; ++k) h[p][k] = (float) (h[p][k] / sm); }
        reset();
    }
    void reset() { std::fill (std::begin (z), std::end (z), 0.0f); zp = 0; }
    // spinge un campione, ritorna il picco (lineare) attorno al campione di kDelay-1 campioni fa
    inline float push (float x)
    {
        z[zp] = x; z[zp + kTaps] = x; zp = (zp + 1) % kTaps;
        const float* w = z + zp;          // w[0] = piu vecchio ... w[kTaps-1] = piu nuovo
        float pk = center = std::abs (w[kTaps / 2 - 1]);
        for (int p = 1; p < kPh; ++p) { float s = 0; for (int k = 0; k < kTaps; ++k) s += h[p][k] * w[k]; pk = juce::jmax (pk, std::abs (s)); }
        return pk;
    }
private:
    static double kaiser (double x, double half, double beta)
    {
        const double r = x / (half + 0.5); if (std::abs (r) >= 1) return 0;
        auto i0 = [] (double v) { double s = 1, t = 1; for (int k = 1; k < 25; ++k) { t *= (v / (2 * k)) * (v / (2 * k)); s += t; } return s; };
        return i0 (beta * std::sqrt (1 - r * r)) / i0 (beta);
    }
    float h[kPh][kTaps] {}; float z[2 * kTaps] {}; int zp = 0;
};

// --------------------------------------------------------------------------- LUFS (ITU BS.1770-4 / EBU R128)
class Loudness
{
public:
    void prepare (double sr)
    {
        double f0 = 1681.974450955533, G = 3.999843853973347, Q = 0.7071752369554196, K = std::tan (juce::MathConstants<double>::pi * f0 / sr);
        const double Vh = std::pow (10.0, G / 20.0), Vb = std::pow (Vh, 0.4996667741545416), a0 = 1.0 + K / Q + K * K;
        kb[0][0] = (Vh + Vb * K / Q + K * K) / a0; kb[0][1] = 2.0 * (K * K - Vh) / a0; kb[0][2] = (Vh - Vb * K / Q + K * K) / a0;
        ka[0][1] = 2.0 * (K * K - 1.0) / a0; ka[0][2] = (1.0 - K / Q + K * K) / a0;
        f0 = 38.13547087602444; Q = 0.5003270373238773; K = std::tan (juce::MathConstants<double>::pi * f0 / sr);
        kb[1][0] = 1; kb[1][1] = -2; kb[1][2] = 1;
        ka[1][1] = 2.0 * (K * K - 1.0) / (1.0 + K / Q + K * K); ka[1][2] = (1.0 - K / Q + K * K) / (1.0 + K / Q + K * K);
        blockLen = juce::jmax (1, (int) (sr * 0.1));
        resetAll();
    }
    void resetAll()
    {
        std::fill (&kz[0][0][0], &kz[0][0][0] + 8, 0.0); blockE = 0; blockN = 0; blocks.assign (30, 0.0); bi = 0; seen = 0;
        resetIntegrated();
    }
    void resetIntegrated() { histI.assign (760, 0); histS.assign (760, 0); nI = nS = 0; I = -100; lra = 0; dirty = 0; }
    inline void push (float l, float r)
    {
        double e = 0;
        for (int c = 0; c < 2; ++c)
        {
            double x = c ? r : l;
            for (int s = 0; s < 2; ++s)
            {
                const double y = kb[s][0] * x + kz[s][c][0];
                kz[s][c][0] = kb[s][1] * x - ka[s][1] * y + kz[s][c][1];
                kz[s][c][1] = kb[s][2] * x - ka[s][2] * y;
                x = y;
            }
            e += x * x;
        }
        blockE += e;
        if (++blockN >= blockLen) block();
    }
    float M = -100, S = -100, I = -100, lra = 0;
private:
    static int bin (double l) { return juce::jlimit (0, 759, (int) std::round ((l + 70.0) * 10.0)); }
    static double pw (int k) { return std::pow (10.0, (k / 10.0 - 70.0 + 0.691) / 10.0); }
    void block()
    {
        blocks[(size_t) bi] = blockE / blockN; bi = (bi + 1) % 30; blockE = 0; blockN = 0;
        auto mean = [this] (int n) { double s = 0; for (int k = 1; k <= n; ++k) s += blocks[(size_t) ((bi - k + 30) % 30)]; return s / n; };
        const double m = -0.691 + 10 * std::log10 (mean (4) + 1e-12), st = -0.691 + 10 * std::log10 (mean (30) + 1e-12);
        M = (float) m; S = (float) st;
        if (++seen >= 4 && m > -70) { histI[(size_t) bin (m)]++; ++nI; }
        if (seen >= 30 && st > -70) { histS[(size_t) bin (st)]++; ++nS; }
        if (++dirty >= 5) { dirty = 0; integrate(); }
    }
    void integrate()
    {
        if (nI > 0)
        {
            double s = 0; for (int k = 0; k < 760; ++k) if (histI[(size_t) k]) s += histI[(size_t) k] * pw (k);
            const double gate = -0.691 + 10 * std::log10 (s / nI) - 10;
            double s2 = 0; int n2 = 0; for (int k = bin (gate); k < 760; ++k) if (histI[(size_t) k]) { s2 += histI[(size_t) k] * pw (k); n2 += histI[(size_t) k]; }
            if (n2 > 0) I = (float) (-0.691 + 10 * std::log10 (s2 / n2));
        }
        if (nS > 10)
        {
            double s = 0; for (int k = 0; k < 760; ++k) if (histS[(size_t) k]) s += histS[(size_t) k] * pw (k);
            const int g0 = bin (-0.691 + 10 * std::log10 (s / nS) - 20);
            int tot = 0; for (int k = g0; k < 760; ++k) tot += histS[(size_t) k];
            if (tot > 0) { int acc = 0, lo = -1, hi = -1; for (int k = g0; k < 760; ++k) { acc += histS[(size_t) k]; if (lo < 0 && acc >= tot * 0.10) lo = k; if (hi < 0 && acc >= tot * 0.95) { hi = k; break; } } if (lo >= 0 && hi >= 0) lra = (hi - lo) / 10.0f; }
        }
    }
    double kb[2][3] {}, ka[2][3] {}, kz[2][2][2] {}, blockE = 0; int blockN = 0, blockLen = 4800, bi = 0, seen = 0, nI = 0, nS = 0, dirty = 0;
    std::vector<double> blocks; std::vector<int> histI, histS;
};

// --------------------------------------------------------------------------- MAXIMIZER
// 4x sovracampionato: spinta -> (saturazione morbida per lo stile) -> clipper morbido -> limitatore lookahead.
enum MaxStyle { PULITO, PUNCH, AGGRESSIVO, MORBIDO, kNumStyles };
struct StyleInfo { const char* name; const char* what; float lookMs, relFast, relSlow, knee; };
inline const StyleInfo& styleInfo (int s)
{
    static const StyleInfo S[kNumStyles] = {
        { "PULITO", "Trasparente: lookahead lungo, rilascio che si adatta al materiale. Per streaming e mix gia buoni.", 5.0f, 60, 700, 0.6f },
        { "PUNCH", "Lascia passare il colpo della cassa e del rullante e recupera veloce: batteria che picchia anche forte.", 1.6f, 22, 260, 0.75f },
        { "AGGRESSIVO", "Rilascio rapido e clipper piu deciso: volume da trap e drill, un po' di grinta voluta.", 1.0f, 10, 140, 0.85f },
        { "MORBIDO", "Saturazione leggera e rilascio lento: arrotonda, suono vintage per boom bap, lo-fi e R&B.", 4.0f, 120, 1100, 0.45f } };
    return S[juce::jlimit (0, kNumStyles - 1, s)];
}

struct MaxSettings { float gainDb = 0, ceilDb = -1, clip = 0.2f, character = 0.5f, link = 1; int style = PULITO; };

class Maximizer
{
public:
    static constexpr float kMaxLookMs = 5.0f;
    void prepare (double sr_, int maxBlock)
    {
        sr = sr_;
        os = std::make_unique<juce::dsp::Oversampling<float>> (2, 2, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, true);
        os->initProcessing ((size_t) maxBlock);
        osLat = juce::roundToInt (os->getLatencyInSamples());
        const double fs = sr * 4;
        maxL4 = ((int) std::ceil (kMaxLookMs * 0.001 * fs) + 3) / 4 * 4;
        for (auto& e : eng) e.prepare (fs, maxL4);
        for (auto& d : dl) d.prepare (maxL4);
        for (auto& d : pad) d.prepare (0);
        lastStyle = -1; maxB = maxBlock;
    }
    // latenza totale in campioni al rate base (costante)
    int latency() const { return osLat + maxL4 / 4; }
    void process (float* L, float* R, int n, const MaxSettings& S)
    {
        if (S.style != lastStyle)
        {
            lastStyle = S.style; const auto& si = styleInfo (S.style);
            const int L4 = juce::jlimit (4, maxL4, (int) std::round (si.lookMs * 0.001 * sr * 4) / 4 * 4);
            for (auto& e : eng) e.setLookahead (L4);
            for (auto& d : dl) d.prepare (L4);
            for (auto& d : pad) d.prepare (maxL4 - L4);
        }
        const auto& si = styleInfo (S.style);
        const float ch = juce::jlimit (0.0f, 1.0f, S.character);           // 0 = lento e pulito, 1 = veloce e aggressivo
        const float fScale = std::pow (2.0f, (0.5f - ch) * 2.0f);          // x2 .. x0.5
        for (auto& e : eng) e.setRelease (si.relFast * fScale, si.relSlow * fScale);
        const float g = dbToG (S.gainDb), ceil = dbToG (S.ceilDb);
        // il clipper morbido lavora sopra una soglia che scende col CLIP
        const float clipT = ceil * dbToG (6.0f * (1.0f - S.clip)), knee = si.knee;
        for (int off = 0; off < n; off += maxB)
        {
            const int m = juce::jmin (maxB, n - off);
            float* ch2[2] = { L + off, R + off };
            juce::dsp::AudioBlock<float> blk (ch2, 2, (size_t) m);
            auto up = os->processSamplesUp (blk);
            float* a = up.getChannelPointer (0); float* b = up.getChannelPointer (1);
            const int nu = (int) up.getNumSamples();
            float grMax = 0;
            for (int i = 0; i < nu; ++i)
            {
                float x = a[i] * g, y = b[i] * g;
                if (S.style == MORBIDO) { x = soft (x, ceil * 2.0f); y = soft (y, ceil * 2.0f); }   // un filo di saturazione prima
                if (S.clip > 0.001f) { x = clip (x, clipT, knee); y = clip (y, clipT, knee); }
                // guadagno richiesto per canale, poi collegamento stereo
                float rl = juce::jmin (1.0f, ceil / juce::jmax (1e-9f, std::abs (x))), rr = juce::jmin (1.0f, ceil / juce::jmax (1e-9f, std::abs (y)));
                const float lk = juce::jmin (rl, rr);
                rl = rl + (lk - rl) * S.link; rr = rr + (lk - rr) * S.link;
                const float gl = eng[0].tick (rl), gr = eng[1].tick (rr);
                float ox = dl[0] (x) * gl, oy = dl[1] (y) * gr;
                ox = pad[0] (ox); oy = pad[1] (oy);
                a[i] = ox; b[i] = oy;
                grMax = juce::jmax (grMax, juce::jmax (eng[0].grNow, eng[1].grNow));
            }
            os->processSamplesDown (blk);
            gr = grMax;
        }
    }
    float gr = 0;   // riduzione (dB) dell'ultimo blocco
private:
    static inline float soft (float x, float c) { return c * std::tanh (x / c); }
    // clipper morbido: lineare fino a t*knee, poi curva che tende a t
    static inline float clip (float x, float t, float knee)
    {
        const float a = std::abs (x), k = t * knee;
        if (a <= k) return x;
        const float r = t - k; const float y = k + r * std::tanh ((a - k) / r);
        return std::copysign (y, x);
    }
    double sr = 48000; int osLat = 0, maxL4 = 960, lastStyle = -1, maxB = 512;
    std::unique_ptr<juce::dsp::Oversampling<float>> os;
    GainEngine eng[2]; Delay dl[2], pad[2];
};

// --------------------------------------------------------------------------- LIMITER finale (true peak)
struct LimSettings { float ceilDb = -1, relMs = 80; bool autoRel = true, truePeak = true; };
class Limiter
{
public:
    void prepare (double sr_)
    {
        sr = sr_; L = juce::jmax (8, (int) std::round (0.0015 * sr));
        for (auto& e : eng) { e.prepare (sr, L); e.setLookahead (L); }
        for (auto& d : dl) d.prepare (L + TruePeak::kDelay);
        for (auto& t : tp) t.reset();
    }
    int latency() const { return L + TruePeak::kDelay; }
    void process (float* Lc, float* Rc, int n, const LimSettings& S)
    {
        const float ceil = dbToG (S.ceilDb - (S.truePeak ? 0.1f : 0.0f));   // piccolo margine per i picchi oltre i 20 kHz
        if (S.autoRel) for (auto& e : eng) e.setRelease (25, 600); else for (auto& e : eng) e.setRelease (S.relMs, S.relMs);
        float grMax = 0;
        for (int i = 0; i < n; ++i)
        {
            float pl = tp[0].push (Lc[i]), pr = tp[1].push (Rc[i]);
            if (! S.truePeak) { pl = tp[0].center; pr = tp[1].center; }
            // collegamento stereo pieno: niente spostamenti dell'immagine
            const float req = juce::jmin (1.0f, ceil / juce::jmax (1e-9f, juce::jmax (pl, pr)));
            const float g = eng[0].tick (req);
            float x = dl[0] (Lc[i]) * g, y = dl[1] (Rc[i]) * g;
            // sicurezza finale sul campione
            x = juce::jlimit (-ceil, ceil, x); y = juce::jlimit (-ceil, ceil, y);
            Lc[i] = x; Rc[i] = y;
            grMax = juce::jmax (grMax, eng[0].grNow);
        }
        gr = grMax;
    }
    float gr = 0;
private:
    double sr = 48000; int L = 72;
    GainEngine eng[1]; Delay dl[2]; TruePeak tp[2];
};

// --------------------------------------------------------------------------- DITHER (TPDF, opzionale noise shaping)
class Dither
{
public:
    void reset() { std::fill (std::begin (e[0]), std::end (e[0]), 0.0f); std::fill (std::begin (e[1]), std::end (e[1]), 0.0f); }
    // mode: 0 off, 1 = 24 bit, 2 = 16 bit TPDF, 3 = 16 bit con noise shaping
    void process (float* L, float* R, int n, int mode)
    {
        if (mode <= 0) return;
        const float q = mode == 1 ? 1.0f / 8388608.0f : 1.0f / 32768.0f;
        static const float c[9] = { 2.412f, -3.370f, 3.937f, -4.174f, 3.353f, -2.205f, 1.281f, -0.569f, 0.0847f };   // pesatura E (Lipshitz)
        for (int ch = 0; ch < 2; ++ch)
        {
            float* x = ch ? R : L; float* h = e[ch];
            for (int i = 0; i < n; ++i)
            {
                float v = x[i];
                if (mode == 3) { float s = 0; for (int k = 0; k < 9; ++k) s += c[k] * h[k]; v -= s; }
                const float d = (rng.nextFloat() - rng.nextFloat()) * q;
                const float y = std::round ((v + d) / q) * q;
                if (mode == 3) { for (int k = 8; k > 0; --k) h[k] = h[k - 1]; h[0] = juce::jlimit (-8 * q, 8 * q, y - v); }
                x[i] = y;
            }
        }
    }
private:
    float e[2][9] {}; juce::Random rng { 12345 };
};
}
