#pragma once
#include "MasterDyn.h"

// Jackpot Master, colore: TAPE (macchine a nastro da master) ed EXCITER multibanda.
// Le curve sono antialiasate con ADAA di primo ordine (integrale della non linearita): niente latenza.
namespace jms
{
// tanh con polarizzazione, antialiasata (F = log cosh)
struct AdaaTanh
{
    float x1 = 0;
    static inline double F (double x) { const double a = std::abs (x); return a > 15 ? a - 0.6931471805599453 : std::log (std::cosh (x)); }
    inline float operator() (float x, float b = 0.0f)
    {
        const double d = (double) x - x1; float y;
        if (std::abs (d) < 1e-5) y = std::tanh (0.5f * (x + x1) + b);
        else y = (float) ((F (x + b) - F (x1 + b)) / d);
        x1 = x; return y - std::tanh (b);
    }
};

// --------------------------------------------------------------------------- TAPE
enum TapeModel { T30, T15, CASSETTA, kTapeModels };
struct TapeInfo { const char* name; const char* what; float bumpF, bumpDb, lpF, preDb, satK, noise; };
inline const TapeInfo& tapeInfo (int m)
{
    static const TapeInfo I[kTapeModels] = {
        { "30 IPS", "Macchina da master a 30 pollici al secondo: bassi profondi, alti aperti, saturazione molto fine. Colla trasparente.", 38, 1.2f, 22000, 3, 0.8f, 0.4f },
        { "15 IPS", "La velocita classica: head bump sui 60 Hz, alti arrotondati, la saturazione piu musicale. Calore e spessore.", 62, 2.4f, 17500, 4.5f, 1.0f, 0.7f },
        { "CASSETTA", "Nastro piccolo: bump alto, alti chiusi, saturazione evidente e fruscio. Lo-fi e vintage.", 95, 2.0f, 11000, 6, 1.4f, 1.6f } };
    return I[juce::jlimit (0, kTapeModels - 1, m)];
}
struct TapeSettings { int model = T15; float drive = 0, bias = 0, noise = 0, mix = 1; bool delta = false; };
class TapeMachine
{
public:
    void prepare (double s) { sr = s; lastModel = -1; lastBias = 99; for (auto& c : ch) c = {}; }
    void process (float* L, float* R, int n, const TapeSettings& S)
    {
        const auto& ti = tapeInfo (S.model);
        if (S.model != lastModel || std::abs (S.bias - lastBias) > 1e-3f)
        {
            lastModel = S.model; lastBias = S.bias;
            // BIAS: sotto = piu acuti e piu distorsione, sopra = piu scuro e pulito
            for (auto& c : ch)
            {
                c.pre.set (Biquad::HSHELF, 3000, ti.preDb, 0.6, sr); c.de.set (Biquad::HSHELF, 3000, -ti.preDb, 0.6, sr);
                c.bump.set (Biquad::BELL, ti.bumpF, ti.bumpDb, 0.9, sr); c.dip.set (Biquad::BELL, ti.bumpF * 2.1, -ti.bumpDb * 0.35, 1.2, sr);
                c.lp.set (Biquad::HCUT, ti.lpF * (1.0f - 0.25f * S.bias), 0, 0.6, sr);
                c.air.set (Biquad::HSHELF, 9000, -2.5 * S.bias, 0.7, sr);
                c.nlp.set (Biquad::HCUT, 9000, 0, 0.7, sr); c.nhp.set (Biquad::LCUT, 400, 0, 0.7, sr);
            }
        }
        const float g = dbToG (12.0f * std::pow (S.drive, 1.5f) * ti.satK), inv = 1.0f / g;
        const float asym = 0.06f + 0.10f * juce::jmax (0.0f, -S.bias);       // poco bias = piu seconda armonica
        const float hiss = S.noise * 0.0025f * ti.noise;
        float* io[2] = { L, R };
        for (int c = 0; c < 2; ++c)
        {
            auto& st = ch[c]; float* x = io[c];
            for (int i = 0; i < n; ++i)
            {
                const float in = x[i];
                float y = st.pre (in);
                y = S.drive > 1e-4f ? st.sat (y * g, asym) * inv : y;
                y = st.de (y); y = st.dip (st.bump (y)); y = st.air (y);
                if (ti.lpF < 21000 || S.bias > 0.01f) y = st.lp (y);
                if (hiss > 0) y += st.nlp (st.nhp ((rng.nextFloat() * 2 - 1) * hiss));
                y = in + (y - in) * S.mix;
                x[i] = S.delta ? y - in : y;
            }
        }
    }
private:
    struct Ch { Biquad pre, de, bump, dip, lp, air, nlp, nhp; AdaaTanh sat; };
    double sr = 48000; int lastModel = -1; float lastBias = 99; Ch ch[2]; juce::Random rng { 77 };
};

// --------------------------------------------------------------------------- EXCITER multibanda
enum ExMode { VALVOLA, NASTRO, TRIODO, CALDO, RETRO, BRILLANTE, kExModes };
inline const char* exName (int m) { static const char* n[kExModes] = { "VALVOLA", "NASTRO", "TRIODO", "CALDO", "RETRO", "BRILLANTE" }; return n[juce::jlimit (0, kExModes - 1, m)]; }
inline const char* exWhat (int m)
{
    static const char* w[kExModes] = { "Armoniche pari morbide: piu pieno e caldo.", "Saturazione simmetrica e dolce: densita senza durezza.", "Asimmetria forte: tanta seconda armonica, presenza.",
                                       "Satura e arrotonda gli alti: spessore vintage.", "Armoniche dispari decise: grinta e mordente.", "Esalta le armoniche alte: aria e luccichio." };
    return w[juce::jlimit (0, kExModes - 1, m)];
}
constexpr int kExBands = 3;
struct ExSettings { float x1 = 250, x2 = 4000, mix = 1; float amt[kExBands] {}; int mode[kExBands] { VALVOLA, NASTRO, BRILLANTE }; bool solo[kExBands] {}; bool delta = false; };
class Exciter
{
public:
    void prepare (double s)
    {
        sr = s; juce::dsp::ProcessSpec sp { sr, 512, 2 };
        for (auto* f : { &x1, &x2 }) { f->prepare (sp); f->setType (juce::dsp::LinkwitzRileyFilterType::lowpass); }
        ap.prepare (sp); ap.setType (juce::dsp::LinkwitzRileyFilterType::allpass);
        for (auto& b : st) for (auto& c : b) c = {};
        lx1 = lx2 = -1;
        for (int b = 0; b < kExBands; ++b) for (int c = 0; c < 2; ++c)
        {
            st[b][c].warm.set (Biquad::HCUT, 9000, 0, 0.7, sr); st[b][c].bright.set (Biquad::LCUT, 3000, 0, 0.7, sr);
            static const double hpHz[kExBands] = { 60, 400, 3000 };   // sotto queste frequenze le "armoniche" sono solo intermodulazione: via
            st[b][c].hpA = (float) std::exp (-juce::MathConstants<double>::twoPi * hpHz[b] / sr);
            st[b][c].eK = (float) std::exp (-1.0 / (0.08 * sr));
        }
    }
    void process (float* L, float* R, int n, const ExSettings& S)
    {
        if (std::abs (S.x1 - lx1) > 0.5f) { lx1 = S.x1; x1.setCutoffFrequency (S.x1); }
        if (std::abs (S.x2 - lx2) > 0.5f) { lx2 = S.x2; x2.setCutoffFrequency (S.x2); ap.setCutoffFrequency (S.x2); }
        bool anySolo = false; for (bool b : S.solo) anySolo |= b;
        float* io[2] = { L, R };
        for (int c = 0; c < 2; ++c)
        {
            float* x = io[c];
            for (int i = 0; i < n; ++i)
            {
                float lo, hi, b[kExBands];
                x1.processSample (c, x[i], lo, hi); x2.processSample (c, hi, b[1], b[2]); b[0] = ap.processSample (c, lo);
                float out = 0, dry = 0;
                for (int k = 0; k < kExBands; ++k)
                {
                    float y = b[k];
                    if (S.amt[k] > 1e-4f) y = excite (st[k][c], b[k], S.mode[k], S.amt[k]);
                    if (anySolo && ! S.solo[k]) continue;
                    out += y; dry += b[k];
                }
                const float res = dry + (out - dry) * S.mix;
                x[i] = S.delta ? res - dry : res;
                if (c == 0) { const float e = std::abs (out - dry); meter = juce::jmax (meter * 0.9999f, e); }
            }
        }
    }
    float meter = 0;
private:
    struct St { AdaaTanh s; Biquad warm, bright; float hpS = 0, hpA = 0.99f, eIn = 0, eOut = 0, eK = 0.999f, gS = 1; };
    // aggiunge armoniche: in parallelo alla banda, la parte lineare si cancella e restano le armoniche
    inline float excite (St& t, float x, int mode, float amt)
    {
        const float g = 1.0f + amt * amt * 6.0f;
        float h;
        switch (mode)
        {
            case VALVOLA:   h = t.s (x * g, 0.35f) / g; break;
            case NASTRO:    h = t.s (x * g, 0.0f) / g; break;
            case TRIODO:    h = t.s (x * g * 1.3f, 0.7f) / (g * 1.3f); break;
            case CALDO:     h = t.warm (t.s (x * g, 0.2f) / g); break;
            case RETRO:     h = t.s (x * g * 1.8f, 0.0f) / (g * 1.8f); break;
            default:        { const float hp = t.bright (x); h = x + t.s (hp * g * 1.5f, 0.5f) / (g * 1.5f) - hp; break; }   // armoniche solo della parte alta
        }
        // armoniche = banda saturata meno banda pulita, ripulite dalle intermodulazioni sotto la banda (passa alto) e aggiunte dosate.
        // La saturazione abbassa un po' la fondamentale: un inviluppo lento rimette il livello della banda dov'era (si aggiunge brillantezza, non volume giu').
        float harm = h - x;
        t.hpS = harm + t.hpA * (t.hpS - harm); harm -= t.hpS;
        const float y = x + harm * amt * amt * 1.6f;   // curva morbida: 50% = colore, 100% = deciso
        t.eIn = t.eK * t.eIn + (1 - t.eK) * std::abs (x); t.eOut = t.eK * t.eOut + (1 - t.eK) * std::abs (y);
        const float gc = t.eOut > 1e-6f ? juce::jlimit (0.7f, 1.4f, t.eIn / t.eOut) : 1.0f;
        t.gS += (gc - t.gS) * 0.001f;
        return y * t.gS;
    }
    double sr = 48000; float lx1 = -1, lx2 = -1;
    juce::dsp::LinkwitzRileyFilter<float> x1, x2, ap; St st[kExBands][2];
};

// --------------------------------------------------------------------------- EQ 432 (ispirato al classico parametrico da mastering a 5 bande)
// Curve larghe e "musicali", bande sovrapposte, gamma di guadagno commutabile (6 / 9 / 12 dB come le tre versioni hardware),
// BASSI e ALTI campana o shelf. COLORE aggiunge l'impronta dello stadio discreto in classe A (armoniche pari leggerissime).
constexpr int k432Bands = 5;
struct Eq432Settings { float f[k432Bands] { 60, 250, 1000, 4000, 12000 }, g[k432Bands] {}, q[k432Bands] { 0.7f, 0.8f, 0.8f, 0.8f, 0.7f }; bool shelf[2] { true, true }; int range = 1; float colour = 0; bool delta = false; };
inline float eq432Range (int r) { return r == 0 ? 6.0f : r == 2 ? 12.0f : 9.0f; }
inline juce::NormalisableRange<float> eq432Freq (int b)
{
    static const float lo[k432Bands] = { 20, 60, 200, 800, 2000 }, hi[k432Bands] = { 400, 1200, 4000, 12000, 20000 };
    return { lo[b], hi[b], 0, 0.4f };
}
class Eq432
{
public:
    void prepare (double s) { sr = s; for (auto& c : f) for (auto& x : c) x.reset(); for (auto& k : key) k = -1; for (auto& a : sat) a = {}; dcX[0] = dcX[1] = dcY[0] = dcY[1] = 0; }
    void process (float* L, float* R, int n, const Eq432Settings& S)
    {
        const float rng = eq432Range (S.range);
        bool act[k432Bands];
        for (int b = 0; b < k432Bands; ++b)
        {
            const float gdb = juce::jlimit (-1.0f, 1.0f, S.g[b]) * rng;
            act[b] = std::abs (gdb) > 0.01f;
            const int type = (b == 0 && S.shelf[0]) ? Biquad::LSHELF : (b == k432Bands - 1 && S.shelf[1]) ? Biquad::HSHELF : Biquad::BELL;
            // Q proporzionale: con poco guadagno la campana si allarga (comportamento tipico dei parametrici passivi/discreti d'epoca)
            const float q = type == Biquad::BELL ? S.q[b] * (0.6f + 0.4f * std::abs (gdb) / rng) : juce::jmin (1.0f, S.q[b]);
            const float k = (float) type * 100000.0f + S.f[b] + gdb * 1000.0f + q * 10.0f;
            if (k != key[b]) { key[b] = k; for (int c = 0; c < 2; ++c) { const double z1 = f[c][b].z1, z2 = f[c][b].z2; f[c][b].set (type, S.f[b], gdb, q, sr); f[c][b].z1 = z1; f[c][b].z2 = z2; } }
        }
        const float col = S.colour, drv = 1.0f + col * 2.5f;
        const float dA = 1.0f - (float) (juce::MathConstants<double>::twoPi * 8.0 / sr);
        float* io[2] = { L, R };
        for (int c = 0; c < 2; ++c)
        {
            float* x = io[c];
            for (int i = 0; i < n; ++i)
            {
                const float in = x[i]; float y = in;
                for (int b = 0; b < k432Bands; ++b) if (act[b]) y = f[c][b] (y);
                if (col > 0.001f)
                {   // classe A: un filo di asimmetria (pari) che cresce col livello, livello d'uscita invariato
                    float v = sat[c] (y * drv, 0.12f * col) / drv;
                    const float o = v - dcX[c] + dA * dcY[c]; dcX[c] = v; dcY[c] = o;
                    y = y + (o - y) * col;
                }
                x[i] = S.delta ? y - in : y;
            }
        }
    }
private:
    double sr = 48000; Biquad f[2][k432Bands]; float key[k432Bands] { -1, -1, -1, -1, -1 }; AdaaTanh sat[2]; float dcX[2] {}, dcY[2] {};
};
}
