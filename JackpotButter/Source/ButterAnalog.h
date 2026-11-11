#pragma once
// Jackpot Butter Rack: EQ analogici (modelli con le loro frequenze e il loro colore), Pultec EQP-1A, formanti, MAXX
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <cmath>

namespace jb
{
// ---------------------------------------------------------------- biquad in double per gli EQ
struct Bq
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1[2] {}, z2[2] {};
    enum { LSH = 0, PEAK, HSH, HP, LP };
    void set (int type, double sr, double f, double gDb, double q)
    {
        f = juce::jlimit (5.0, sr * 0.45, f); q = juce::jmax (0.1, q);
        const double A = std::pow (10.0, gDb / 40.0), w = juce::MathConstants<double>::twoPi * f / sr, cs = std::cos (w), sn = std::sin (w), al = sn / (2 * q);
        double B0, B1, B2, A0, A1, A2;
        switch (type)
        {
            case LSH: { const double s = 2 * std::sqrt (A) * al; B0 = A * ((A + 1) - (A - 1) * cs + s); B1 = 2 * A * ((A - 1) - (A + 1) * cs); B2 = A * ((A + 1) - (A - 1) * cs - s); A0 = (A + 1) + (A - 1) * cs + s; A1 = -2 * ((A - 1) + (A + 1) * cs); A2 = (A + 1) + (A - 1) * cs - s; break; }
            case HSH: { const double s = 2 * std::sqrt (A) * al; B0 = A * ((A + 1) + (A - 1) * cs + s); B1 = -2 * A * ((A - 1) + (A + 1) * cs); B2 = A * ((A + 1) + (A - 1) * cs - s); A0 = (A + 1) - (A - 1) * cs + s; A1 = 2 * ((A - 1) - (A + 1) * cs); A2 = (A + 1) - (A - 1) * cs - s; break; }
            case HP: B0 = (1 + cs) / 2; B1 = -(1 + cs); B2 = B0; A0 = 1 + al; A1 = -2 * cs; A2 = 1 - al; break;
            case LP: B0 = (1 - cs) / 2; B1 = 1 - cs; B2 = B0; A0 = 1 + al; A1 = -2 * cs; A2 = 1 - al; break;
            default: B0 = 1 + al * A; B1 = -2 * cs; B2 = 1 - al * A; A0 = 1 + al / A; A1 = -2 * cs; A2 = 1 - al / A; break;
        }
        b0 = B0 / A0; b1 = B1 / A0; b2 = B2 / A0; a1 = A1 / A0; a2 = A2 / A0;
    }
    inline float run (float x, int c) { const double y = b0 * x + z1[c]; z1[c] = b1 * x - a1 * y + z2[c]; z2[c] = b2 * x - a2 * y; return (float) y; }
    double magDb (double hz, double sr) const
    {
        const double w = juce::MathConstants<double>::twoPi * hz / sr;
        const double cr = std::cos (w), ci = -std::sin (w), c2r = std::cos (2 * w), c2i = -std::sin (2 * w);
        const double nr = b0 + b1 * cr + b2 * c2r, ni = b1 * ci + b2 * c2i, dr = 1 + a1 * cr + a2 * c2r, di = a1 * ci + a2 * c2i;
        return 10 * std::log10 ((nr * nr + ni * ni) / (dr * dr + di * di) + 1e-24);
    }
};

// ---------------------------------------------------------------- modelli di EQ analogico
enum AeModel { AE_MAAG = 0, AE_SSL, AE_NEVE84, AE_LIND81, AE_NEVE73, AE_PE1C, kAeModels };
struct AeBand { const char* name; int type; std::vector<float> freqs; float q; bool boostOnly = false, cutOnly = false; };
struct AeInfo { const char* name; const char* desc; AeBand b[4]; };
inline const AeInfo& aeInfo (int m)
{
    static const AeInfo I[kAeModels] = {
        { "MAAG EQ4", "pulitissimo, AIR BAND aperta fino a 40 kHz: brillantezza senza durezza",
          { { "SUB", Bq::PEAK, { 10, 40 }, 0.45f }, { "LOW MID", Bq::PEAK, { 160, 650 }, 0.6f }, { "MID", Bq::PEAK, { 2500 }, 0.6f }, { "AIR", Bq::HSH, { 2500, 5000, 10000, 20000, 40000 }, 0.35f, true } } },
        { "SSL E", "chirurgico e diretto: Q che si stringe con il guadagno, colore leggero e mordente",
          { { "LF", Bq::LSH, { 30, 50, 80, 120, 200, 300, 450 }, 0.7f }, { "LMF", Bq::PEAK, { 200, 300, 500, 800, 1200, 1800, 2500 }, 1.0f }, { "HMF", Bq::PEAK, { 600, 1000, 1500, 2500, 4000, 6000, 7000 }, 1.0f }, { "HF", Bq::HSH, { 1500, 3000, 5000, 8000, 10000, 12000, 16000 }, 0.7f } } },
        { "NEVE 1084", "trasformatori e classe A: bassi rotondi, medi larghi musicali, alti setosi",
          { { "LF", Bq::LSH, { 35, 60, 110, 220 }, 1.1f }, { "MID", Bq::PEAK, { 360, 700, 1600, 3200, 4800, 7200 }, 0.8f }, { "HF", Bq::HSH, { 10000, 12000, 16000 }, 0.7f }, { "HPF", Bq::HP, { 0, 45, 70, 160, 360 }, 0.6f } } },
        { "LINDELL 81", "stile 1081: quattro bande con piu frequenze, Neve ma un po' piu pulito",
          { { "LF", Bq::LSH, { 33, 56, 100, 180, 270, 390 }, 1.0f }, { "LMF", Bq::PEAK, { 220, 270, 390, 560, 820, 1200 }, 0.9f }, { "HMF", Bq::PEAK, { 1500, 2200, 3300, 4700, 6800, 10000 }, 0.9f }, { "HF", Bq::HSH, { 6800, 8200, 10000, 12000, 15000, 18000 }, 0.7f } } },
        { "73", "il 1073: il piu colorato, trasformatore che spinge i bassi e medi in faccia",
          { { "LF", Bq::LSH, { 35, 60, 110, 220 }, 1.3f }, { "MID", Bq::PEAK, { 360, 700, 1600, 3200, 4800, 7200 }, 0.7f }, { "HF", Bq::HSH, { 12000 }, 0.7f }, { "HPF", Bq::HP, { 0, 50, 80, 160, 300 }, 0.6f } } },
        { "PE 1C", "Tube-Tech a valvole stile Pultec: boost e cut sui bassi insieme, alti morbidissimi",
          { { "LF BOOST", Bq::LSH, { 20, 30, 60, 100 }, 0.9f, true }, { "LF CUT", Bq::LSH, { 20, 30, 60, 100 }, 0.5f, false, true }, { "HF BOOST", Bq::PEAK, { 3000, 4000, 5000, 8000, 10000, 12000, 16000 }, 1.0f, true }, { "HF CUT", Bq::HSH, { 5000, 10000, 20000 }, 0.6f, false, true } } } };
    return I[juce::jlimit (0, kAeModels - 1, m)];
}
inline int aeIndex (const AeBand& b, float v) { return juce::jlimit (0, (int) b.freqs.size() - 1, juce::roundToInt (v * (float) (b.freqs.size() - 1))); }

struct AeSettings { int model = 0; float g[4] {}, f[4] {}, q = 0.5f, drive = 0.3f; };

// filtri effettivi per una banda (anche per disegnare la curva)
inline void aeBandFilter (const AeSettings& S, int k, double sr, Bq& out, bool& active)
{
    const auto& I = aeInfo (S.model); const auto& B = I.b[k];
    int idx = aeIndex (B, S.f[k]);
    if (S.model == AE_PE1C && k == 1) idx = aeIndex (I.b[0], S.f[0]);   // il taglio dei bassi usa la stessa frequenza del boost
    float f = B.freqs[(size_t) idx], g = S.g[k], q = B.q;
    if (B.boostOnly) g = juce::jmax (0.0f, g);
    if (B.cutOnly) g = juce::jmin (0.0f, g);
    active = B.type == Bq::HP ? f > 1 : std::abs (g) > 0.05f;
    switch (S.model)
    {
        case AE_SSL: if (B.type == Bq::PEAK) q = (0.5f + S.q * 2.5f) * (1.0f + std::abs (g) / 12.0f); break;   // Q proporzionale (serie E)
        case AE_NEVE84: if (B.type == Bq::PEAK && S.q > 0.5f) q = 2.0f; break;                                  // HI-Q
        case AE_LIND81: if (B.type == Bq::PEAK) q = 0.6f + S.q * 1.6f; break;
        case AE_PE1C:
            if (k == 1) f *= 2.2f;                                   // il taglio lavora piu in alto del boost: il famoso "dip"
            if (k == 2) q = 0.4f + (1.0f - S.q) * 2.4f;              // BANDWIDTH: largo -> stretto
            break;
        case AE_MAAG: if (k == 3) g *= 1.0f; break;
        default: break;
    }
    out.set (B.type, sr, f, g, q);
}

// colore: armoniche diverse per ogni modello. drive = quanto si entra caldi nel circuito
struct AeColour
{
    float lp[2] {}, dc[2] {};
    inline float run (float x, int c, int model, float drive, double sr)
    {
        if (drive < 0.001f) return x;
        const float aLf = (float) (1.0 - std::exp (-juce::MathConstants<double>::twoPi * 90.0 / sr));
        auto asym = [] (float v, float d, float b) { const float n = d * (1 - std::tanh (b) * std::tanh (b)); return (std::tanh (d * v + b) - std::tanh (b)) / n; };
        float y = x;
        switch (model)
        {
            case AE_MAAG: y = x + 0.015f * drive * x * x; break;                                               // quasi nulla, un filo di 2a
            case AE_SSL: { const float d = 1.0f + drive * 0.9f; y = std::tanh (d * x) / d; y += 0.02f * drive * (x - y * 0.0f); break; }   // dispari, mordente
            case AE_NEVE84: case AE_NEVE73: case AE_LIND81:
            {
                const float k = model == AE_NEVE73 ? 1.6f : model == AE_LIND81 ? 0.7f : 1.0f;
                lp[c] += (x - lp[c]) * aLf; const float lo = lp[c], kd = 1.0f + drive * 2.2f * k;
                y = x + (std::tanh (lo * kd) / kd - lo);                                                         // il trasformatore satura i bassi
                y = asym (y, 1.0f + drive * 0.9f * k, 0.12f * drive * k);                                        // 2a e 3a armonica
                break;
            }
            case AE_PE1C:
            {
                lp[c] += (x - lp[c]) * aLf; const float lo = lp[c], kd = 1.0f + drive * 1.5f;
                y = x + (std::tanh (lo * kd) / kd - lo);
                y = asym (y, 1.0f + drive * 1.3f, 0.25f * drive);                                                // valvola: 2a armonica
                break;
            }
            default: break;
        }
        dc[c] += (y - dc[c]) * 0.0004f; return y - dc[c];
    }
};

// ---------------------------------------------------------------- Pultec EQP-1A
struct PultecSettings { float lb = 0, la = 0, hb = 0, bw = 5, ha = 0; int lf = 1, hf = 3, as = 1; };
inline const float* pultecLf() { static const float f[4] = { 20, 30, 60, 100 }; return f; }
inline const float* pultecHf() { static const float f[7] = { 3000, 4000, 5000, 8000, 10000, 12000, 16000 }; return f; }
inline const float* pultecAs() { static const float f[3] = { 5000, 10000, 20000 }; return f; }
// i quattro filtri del Pultec (boost bassi, attenuazione bassi, boost alti, attenuazione alti)
inline void pultecFilters (const PultecSettings& P, double sr, Bq* f)
{
    const float lf = pultecLf()[juce::jlimit (0, 3, P.lf)];
    f[0].set (Bq::LSH, sr, lf * 1.25f, P.lb / 10.0f * 13.5f, 1.05f);                      // boost con il piccolo "panciotto"
    f[1].set (Bq::LSH, sr, lf * 2.6f, -P.la / 10.0f * 17.5f, 0.5f);                       // attenuazione piu in alto: il trucco del Pultec
    f[2].set (Bq::PEAK, sr, pultecHf()[juce::jlimit (0, 6, P.hf)], P.hb / 10.0f * 16.0f, 0.45f + (10.0f - P.bw) / 10.0f * 2.6f);
    f[3].set (Bq::HSH, sr, pultecAs()[juce::jlimit (0, 2, P.as)], -P.ha / 10.0f * 16.0f, 0.55f);
}
inline bool pultecActive (const PultecSettings& P) { return P.lb > 0.01f || P.la > 0.01f || P.hb > 0.01f || P.ha > 0.01f; }

// ---------------------------------------------------------------- formanti: LPC a traliccio (latenza zero)
// sbianca il segnale con il suo inviluppo spettrale e lo ricolora con l'inviluppo del segnale originale spostato
class Formant
{
public:
    static constexpr int P = 14, WIN = 1024, HOP = 256;
    void prepare (double s) { sr = s; histX.assign (WIN, 0.0f); histY.assign (WIN, 0.0f); w = cnt = 0; for (auto& k : kA) k = 0; for (auto& k : kS) k = 0; for (auto& k : kA0) k = 0; for (auto& k : kS0) k = 0; reset(); }
    void reset() { for (auto& c : fa) for (auto& v : c) v = 0; for (auto& c : fs) for (auto& v : c) v = 0; eY = eO = 1e-6f; gSm = 1; }
    // x = segnale prima dello shift (inviluppo di riferimento), y = segnale shiftato; ratio = spostamento dell'inviluppo
    inline void push (float x, float y)
    {
        histX[(size_t) w] = x; histY[(size_t) w] = y; w = (w + 1) % WIN;
        if (++cnt >= HOP) { cnt = 0; std::copy (std::begin (kA), std::end (kA), std::begin (kA0)); std::copy (std::begin (kS), std::end (kS), std::begin (kS0)); analyse(); }
    }
    float ratio = 1;
    // y del canale c -> uscita con formanti spostate
    inline float process (float y, int c)
    {
        const float t = (float) cnt / HOP;
        float k1[P], k2[P]; for (int m = 0; m < P; ++m) { k1[m] = kA0[m] + (kA[m] - kA0[m]) * t; k2[m] = kS0[m] + (kS[m] - kS0[m]) * t; }
        // analisi (FIR a traliccio): residuo
        float f = y, bPrev = y; auto& A = fa[c];
        for (int m = 0; m < P; ++m) { const float fn = f + k1[m] * A[m]; const float bn = k1[m] * f + A[m]; A[m] = bPrev; f = fn; bPrev = bn; }
        const float e = f;
        // sintesi (IIR a traliccio) con l'inviluppo spostato
        auto& S = fs[c]; float fv = e;
        for (int m = P - 1; m >= 0; --m) { fv = fv - k2[m] * S[m]; if (m + 1 < P) S[m + 1] = k2[m] * fv + S[m]; }
        S[0] = fv;
        float out = juce::jlimit (-8.0f, 8.0f, fv);
        // stessa energia del segnale shiftato (inseguimento veloce) e tetto morbido sui picchi
        eY = eY * 0.998f + y * y * 0.002f; eO = eO * 0.998f + out * out * 0.002f;
        gSm += (juce::jlimit (0.05f, 2.5f, std::sqrt ((eY + 1e-10f) / (eO + 1e-10f))) - gSm) * 0.01f;
        const float o = out * gSm, lim = juce::jmax (0.05f, 2.0f * std::sqrt (eY) * 1.414f);
        return std::abs (o) > lim ? std::copysign (lim + (std::abs (o) - lim) * 0.2f, o) : o;
    }
private:
    double sr = 44100; std::vector<float> histX, histY; int w = 0, cnt = 0;
    float kA[P] {}, kS[P] {}, kA0[P] {}, kS0[P] {}, fa[2][P] {}, fs[2][P] {}, eY = 1e-6f, eO = 1e-6f, gSm = 1;
    static void levinson (const double* r, float* k)
    {
        double a[P + 1] {}, tmp[P + 1]; a[0] = 1; double E = r[0];
        for (int m = 1; m <= P; ++m)
        {
            double acc = r[m]; for (int i = 1; i < m; ++i) acc += a[i] * r[m - i];
            double km = E > 1e-12 ? -acc / E : 0.0; km = juce::jlimit (-0.995, 0.995, km);
            for (int i = 0; i <= m; ++i) tmp[i] = a[i];
            for (int i = 1; i < m; ++i) a[i] = tmp[i] + km * tmp[m - i];
            a[m] = km; E *= (1 - km * km); k[m - 1] = (float) km;
        }
    }
    void autocorr (const std::vector<float>& h, double* r) const
    {
        static float win[WIN]; static bool init = false;
        if (! init) { for (int i = 0; i < WIN; ++i) win[i] = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * i / (WIN - 1)); init = true; }
        float x[WIN]; for (int i = 0; i < WIN; ++i) x[i] = h[(size_t) ((w + i) % WIN)] * win[i];
        for (int l = 0; l <= P; ++l) { double s = 0; for (int i = l; i < WIN; ++i) s += (double) x[i] * x[i - l]; r[l] = s; }
        r[0] = r[0] * 1.003 + 1e-9;
        for (int l = 1; l <= P; ++l) r[l] *= std::exp (-0.5 * std::pow (2 * juce::MathConstants<double>::pi * 120.0 * l / sr, 2.0));   // finestra di lag
    }
    void analyse()
    {
        double rx[P + 1], ry[P + 1]; autocorr (histX, rx); autocorr (histY, ry);
        levinson (ry, kA);
        // inviluppo del riferimento -> spettro -> spostato di "ratio" -> autocorrelazione -> Levinson
        float kx[P]; levinson (rx, kx);
        double a[P + 1] {}, tmp[P + 1]; a[0] = 1;
        for (int m = 1; m <= P; ++m) { for (int i = 0; i <= m; ++i) tmp[i] = a[i]; for (int i = 1; i < m; ++i) a[i] = tmp[i] + kx[m - 1] * tmp[m - i]; a[m] = kx[m - 1]; }
        constexpr int M = 192; double pw[M];
        for (int j = 0; j < M; ++j)
        {
            const double om = juce::MathConstants<double>::pi * (j + 0.5) / M / ratio;
            if (om >= juce::MathConstants<double>::pi) { pw[j] = j > 0 ? pw[j - 1] * 0.5 : 1e-6; continue; }
            double re = 0, im = 0; for (int i = 0; i <= P; ++i) { re += a[i] * std::cos (om * i); im -= a[i] * std::sin (om * i); }
            pw[j] = 1.0 / (re * re + im * im + 1e-9);
        }
        double rw[P + 1];
        for (int l = 0; l <= P; ++l) { double s = 0; for (int j = 0; j < M; ++j) s += pw[j] * std::cos (juce::MathConstants<double>::pi * (j + 0.5) / M * l); rw[l] = s; }
        rw[0] *= 1.003; for (int l = 1; l <= P; ++l) rw[l] *= std::exp (-0.5 * std::pow (2 * juce::MathConstants<double>::pi * 120.0 * l / sr, 2.0));
        levinson (rw, kS);
    }
};

// ---------------------------------------------------------------- MAXX: massimizzatore (limitatore di picco morbido, zero latenza)
struct Maxx
{
    float env = 0, gr = 1;
    inline void run (float& l, float& r, double sr, float ceilingLin)
    {
        const float pk = juce::jmax (std::abs (l), std::abs (r));
        const float tgt = pk > ceilingLin ? ceilingLin / pk : 1.0f;
        const float rel = (float) std::exp (-1.0 / ((0.04 + 0.12 * (1.0f - gr)) * sr));   // rilascio che si allunga quando lavora tanto
        gr = tgt < gr ? tgt + (gr - tgt) * 0.25f : tgt + (gr - tgt) * rel;
        l *= gr; r *= gr;
        // sicurezza morbida sopra il tetto
        auto sc = [ceilingLin] (float x) { const float a = std::abs (x); return a <= ceilingLin * 0.9f ? x : std::copysign (ceilingLin * 0.9f + ceilingLin * 0.1f * std::tanh ((a - ceilingLin * 0.9f) / (ceilingLin * 0.1f)), x); };
        l = sc (l); r = sc (r);
    }
};
}
