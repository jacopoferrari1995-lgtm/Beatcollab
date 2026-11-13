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

// --------------------------------------------------------------------------- PULTEC (EQ passivo a valvole, stile EQP-1A)
// Bassi: BOOST e ATTEN sulla stessa frequenza agiscono su curve diverse (l'attenuazione ha l'angolo piu' in alto),
// quindi usati insieme gonfiano il sub e scavano sopra: il famoso trucco. Alti: campana con larghezza (BANDWIDTH) e
// attenuazione a shelf separata. Le manopole 0..10 come sull'hardware.
struct PultecSettings { int lf = 2, hf = 2, as = 1; float lb = 0, la = 0, hb = 0, bw = 5, ha = 0, tube = 0; };
inline float pultecLf (int i) { static const float f[4] = { 20, 30, 60, 100 }; return f[juce::jlimit (0, 3, i)]; }
inline float pultecHf (int i) { static const float f[7] = { 3000, 4000, 5000, 8000, 10000, 12000, 16000 }; return f[juce::jlimit (0, 6, i)]; }
inline float pultecAs (int i) { static const float f[3] = { 5000, 10000, 20000 }; return f[juce::jlimit (0, 2, i)]; }
// le curve: in un posto solo, cosi' disegno e suono coincidono
inline void pultecDesign (const PultecSettings& S, double sr, Biquad* f /* 4 filtri */, bool* act)
{
    const float lf = pultecLf (S.lf);
    act[0] = S.lb > 0.01f; act[1] = S.la > 0.01f; act[2] = S.hb > 0.01f; act[3] = S.ha > 0.01f;
    // boost bassi: shelf con un filo di risonanza, fino a +14 dB
    f[0].set (Biquad::LSHELF, lf, S.lb * 1.4, 0.8, sr);
    // da solo arriva a ~ -17 dB come l'EQP-1A; col boost attivo la rete passiva interagisce e il taglio si ammorbidisce (trucco Pultec)
    f[1].set (Biquad::LSHELF, lf * 2.2, -S.la * 1.75 * (1.0f - 0.045f * S.lb), 0.35, sr);
    // boost alti: campana, BANDWIDTH 0 = stretta (Q 2.2), 10 = larga (Q 0.35), fino a +16 dB
    const double q = 2.2 * std::pow (0.35 / 2.2, S.bw / 10.0);
    f[2].set (Biquad::BELL, pultecHf (S.hf), S.hb * 1.6, q, sr);
    // atten alti: shelf verso il basso, fino a -16 dB
    f[3].set (Biquad::HSHELF, pultecAs (S.as) * 0.7, -S.ha * 1.6, 0.6, sr);
}
class Pultec
{
public:
    void prepare (double s) { sr = s; for (auto& c : f) for (auto& x : c) x.reset(); key = -1; for (auto& a : sat) a = {}; for (auto& v : dc) v = 0; }
    void process (float* L, float* R, int n, const PultecSettings& S)
    {
        const float k = S.lf * 1.0f + S.hf * 10 + S.as * 100 + S.lb * 1e3f + S.la * 1.3e4f + S.hb * 1.7e5f + S.bw * 2.3e6f + S.ha * 3.1e7f;
        if (k != key)
        {
            key = k; Biquad nf[4];
            pultecDesign (S, sr, nf, act);
            for (int c = 0; c < 2; ++c) for (int b = 0; b < 4; ++b) { const double z1 = f[c][b].z1, z2 = f[c][b].z2; f[c][b] = nf[b]; f[c][b].z1 = z1; f[c][b].z2 = z2; }
        }
        const bool any = act[0] || act[1] || act[2] || act[3];
        const float tube = S.tube, drv = 0.45f + 0.45f * tube, bias = 0.08f * tube, nrm = 1.0f / (1.0f - std::tanh (bias) * std::tanh (bias));
        const float dA = 1.0f - (float) (juce::MathConstants<double>::twoPi * 8.0 / sr);
        float* io[2] = { L, R };
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < n; ++i)
            {
                float y = io[c][i];
                if (any) for (int b = 0; b < 4; ++b) if (act[b]) y = f[c][b] (y);
                if (tube > 0.001f)
                {   // stadio d'uscita a valvola (12AX7 + 12AU7): armoniche pari morbide, livello invariato
                    const float v = sat[c] (y * drv, bias) * nrm / drv;
                    const float o = v - dc[c * 2] + dA * dc[c * 2 + 1]; dc[c * 2] = v; dc[c * 2 + 1] = o;
                    y = y + (o - y) * tube;
                }
                io[c][i] = y;
            }
    }
private:
    double sr = 48000; Biquad f[2][4]; bool act[4] {}; float key = -1; AdaaTanh sat[2]; float dc[4] {};
};

// --------------------------------------------------------------------------- VARI-MU (compressore valvolare a pendenza variabile)
// Nel vari-mu il rapporto non e' fisso: piu' il segnale supera la soglia, piu' la valvola comprime (ginocchio larghissimo
// che continua a salire). Attacco e rilascio lenti e "rotondi", rilascio che dipende dal programma (piu' compressione = ritorno
// piu' lento). COMPRESS = curva dolce (~1.5:1 che sale), LIMIT = piu' ripida. TUBE = colore della valvola.
struct VariMuSettings { float thr = -18, makeup = 0, mix = 1, tube = 0.3f; int mode = 0, att = 2, rel = 2, hpf = 1; bool autoMakeup = true, delta = false, ms = false; };
inline float vmAtt (int i) { static const float a[5] = { 1, 5, 15, 30, 70 }; return a[juce::jlimit (0, 4, i)]; }
inline float vmRel (int i) { static const float r[7] = { 200, 400, 600, 800, 1200, 2000, -1 }; return r[juce::jlimit (0, 6, i)]; }
inline float vmHpf (int i) { static const float h[4] = { 0, 60, 100, 150 }; return h[juce::jlimit (0, 3, i)]; }
// riduzione statica (dB) per un livello sopra soglia: pendenza che cresce col superamento
inline float vmCurve (float over, int mode)
{
    const float W = 12.0f;                                                   // ginocchio di 12 dB centrato sulla soglia
    const float r0 = mode == 0 ? 1.3f : 2.0f, r1 = mode == 0 ? 0.08f : 0.25f;   // il rapporto parte da r0 e sale di r1 per ogni dB sopra soglia
    const float r = r0 + r1 * juce::jmax (0.0f, over);
    if (over <= -W * 0.5f) return 0.0f;
    if (over < W * 0.5f) { const float t = over + W * 0.5f; return (1.0f - 1.0f / r) * t * t / (2.0f * W); }
    return (1.0f - 1.0f / r) * over;
}
class VariMu
{
public:
    void prepare (double s) { sr = s; env = 0; gdb = 0; slow = 0; lastHpf = -1; for (auto& h : hp) h.reset(); for (auto& a : sat) a = {}; for (auto& v : dc) v = 0; avgGr = 0; mkG = -1; }
    void process (float* L, float* R, int n, const VariMuSettings& S)
    {
        const float hz = vmHpf (S.hpf);
        if (hz != lastHpf) { lastHpf = hz; for (auto& h : hp) { if (hz > 0) h.set (Biquad::LCUT, hz, 0, 0.707, sr); else h = Biquad(); h.reset(); } }
        const float aK = (float) std::exp (-1.0 / (vmAtt (S.att) * 0.001 * sr));
        const float rMs = vmRel (S.rel);
        const float eK = (float) std::exp (-1.0 / (0.010 * sr));             // detector RMS 10 ms
        const float avK = (float) std::exp (-(double) n / (3.0 * sr));
        const float mkDb = S.makeup + (S.autoMakeup ? avgGr * 1.0f : 0.0f);
        const float mk0 = mkG < 0 ? dbToG (mkDb) : mkG, mk1 = dbToG (mkDb);
        const float tube = S.tube, drv = 0.45f + 0.45f * tube, bias = 0.08f * tube, nrm = 1.0f / (1.0f - std::tanh (bias) * std::tanh (bias));
        const float dA = 1.0f - (float) (juce::MathConstants<double>::twoPi * 8.0 / sr);
        float grMaxB = 0; double grSum = 0;
        for (int i = 0; i < n; ++i)
        {
            float l = L[i], r = R[i];
            if (S.ms) { const float m = 0.5f * (l + r), sd = 0.5f * (l - r); l = m; r = sd; }
            const float dl = hp[0] (l), dr = hp[1] (r);
            env = eK * env + (1 - eK) * 0.5f * (dl * dl + dr * dr);
            const float lev = 10.0f * std::log10 (env + 1e-12f);
            const float tgt = -vmCurve (lev - S.thr, S.mode);              // dB (<= 0)
            // rilascio: fisso, oppure AUTO (veloce dopo picchi brevi, lento dopo compressione lunga)
            float relMs = rMs;
            if (rMs < 0) { slow = 0.9995f * slow + 0.0005f * (-gdb); relMs = 300.0f + 250.0f * slow; }
            // la valvola stessa "ritarda": un altro filtro morbido sopra a quello di rilascio
            const float rK = (float) std::exp (-1.0 / (relMs * 0.001 * sr * (1.0f + 0.04f * -gdb)));
            gdb = tgt < gdb ? aK * gdb + (1 - aK) * tgt : rK * gdb + (1 - rK) * tgt;
            grMaxB = juce::jmin (grMaxB, gdb); grSum += gdb;
            const float g = dbToG (gdb) * (mk0 + (mk1 - mk0) * (float) (i + 1) / (float) n);
            float yl = l * g, yr = r * g;
            if (tube > 0.001f)
            {   // le valvole lavorano di piu' quando comprimono: colore che segue la riduzione
                const float d2 = drv * (1.0f + 0.03f * -gdb);
                float* ys[2] = { &yl, &yr };
                for (int c = 0; c < 2; ++c)
                {
                    const float v = sat[c] (*ys[c] * d2, bias) * nrm / d2;
                    const float o = v - dc[c * 2] + dA * dc[c * 2 + 1]; dc[c * 2] = v; dc[c * 2 + 1] = o;
                    *ys[c] = *ys[c] + (o - *ys[c]) * tube;
                }
            }
            if (S.ms) { const float m = yl, sd = yr; yl = m + sd; yr = m - sd; }
            const float xl = L[i], xr = R[i];
            yl = xl + (yl - xl) * S.mix; yr = xr + (yr - xr) * S.mix;
            L[i] = S.delta ? yl - xl : yl; R[i] = S.delta ? yr - xr : yr;
        }
        gr = -grMaxB; mkG = mk1;
        avgGr = avK * avgGr + (1 - avK) * (float) (-grSum / juce::jmax (1, n));
    }
    float gr = 0;
private:
    double sr = 48000; Biquad hp[2]; float env = 0, gdb = 0, slow = 0, lastHpf = -1, avgGr = 0, mkG = -1; AdaaTanh sat[2]; float dc[4] {};
};
}
