#pragma once
#include <juce_dsp/juce_dsp.h>
#include "JackpotKit.h"
#include "ButterAnalog.h"   // EQ analogici e Pultec, condivisi con il Butter Rack

namespace jpre
{
// ======================================================================= preamp
enum PreModel { P_73, P_81, P_V76, P_610, P_API, P_SSL, P_TG2, P_737, P_1272, P_HELIOS, P_TRIDENT, kPreModels };
struct PreInfo
{
    const char* name; const char* desc;
    float lfSat;      // il trasformatore d'ingresso satura le basse
    float bias;       // asimmetria = armoniche pari (seconda)
    float knee;       // durezza dello stadio = armoniche dispari
    bool tube;        // valvola: compressione morbida e ginocchio largo
    float headroom;   // dB prima che inizi a saturare
    float hpHz, lpHz; // banda del trasformatore
    float lowHz, lowDb, midHz, midDb, midQ, airHz, airDb;   // impronta in frequenza
};
inline const PreInfo& preInfo (int m)
{
    static const PreInfo I[kPreModels] = {
        { "73", "Neve 1073: classe A e trasformatori Marinair. Bassi spessi, medi in faccia, seconda e terza armonica. Il suono dei dischi.",
          1.0f, 0.16f, 1.0f, false, 18, 16, 24000, 70, 0.7f, 1600, 0.4f, 0.8f, 18000, -0.4f },
        { "81", "Neve 1081 (Lindell 81): stessa famiglia del 73 con piu margine. Rotondo ma piu pulito e aperto.",
          0.65f, 0.11f, 0.95f, false, 22, 14, 30000, 60, 0.4f, 2500, 0.2f, 0.7f, 20000, 0.0f },
        { "V76", "Telefunken V76 a valvole: tanta seconda armonica, medio-bassi pieni e alti morbidi. Caldo da broadcast tedesco.",
          0.7f, 0.42f, 0.7f, true, 14, 25, 17000, 220, 0.9f, 3500, -0.3f, 0.6f, 12000, -1.2f },
        { "610", "UA 610 a valvole: grasso nei bassi e nei medi bassi, si comprime dolcemente quando lo spingi. Voci e bassi.",
          0.85f, 0.34f, 0.75f, true, 15, 22, 19000, 140, 1.0f, 800, 0.3f, 0.7f, 14000, -0.6f },
        { "API 312", "API 312 con l'op-amp 2520: punch e medi davanti, terza armonica, basso stretto. Batterie e chitarre.",
          0.5f, 0.05f, 1.45f, false, 20, 28, 26000, 100, 0.3f, 2800, 0.9f, 0.9f, 15000, 0.2f },
        { "SSL E", "Pre della console SSL E: pulito e brillante, non colora finche non lo spingi forte, poi morde.",
          0.18f, 0.03f, 1.7f, false, 24, 18, 30000, 80, 0.0f, 4000, 0.3f, 0.8f, 12000, 0.6f },
        { "TG2", "EMI TG2 di Abbey Road: trasformatori pesanti, si rompe presto in una saturazione grossa e granulosa.",
          1.35f, 0.2f, 1.25f, false, 12, 20, 18000, 90, 0.6f, 1200, 0.7f, 0.7f, 16000, -0.6f },
        { "737", "Avalon 737 a valvole in classe A: pulito, arioso, alti setosi. Voci moderne.",
          0.3f, 0.24f, 0.8f, true, 22, 12, 32000, 120, 0.2f, 3000, 0.0f, 0.7f, 14000, 0.9f },
        { "1272", "Neve 1272: il pre da broadcast con il trasformatore Marinair spinto. Piu crudo e mediaso del 73, si rompe prima.",
          1.15f, 0.2f, 1.15f, false, 15, 18, 22000, 110, 0.9f, 1000, 0.9f, 0.7f, 16000, -0.8f },
        { "HELIOS", "Helios 69 (Island, Stones): medi alti in avanti e bassi asciutti, saturazione con tanta terza. Rock e batterie.",
          0.75f, 0.08f, 1.35f, false, 17, 35, 22000, 100, -0.6f, 3200, 1.3f, 0.8f, 14000, -0.4f },
        { "TRIDENT", "Trident A-Range: medi morbidi e aperti, alti dolci e un po' di grasso sotto. Il suono dei 70 inglesi.",
          0.6f, 0.14f, 0.9f, false, 20, 20, 24000, 90, 0.8f, 2200, -0.3f, 0.6f, 11000, 0.8f } };
    return I[juce::jlimit (0, kPreModels - 1, m)];
}

// ======================================================================= EQ di colore
enum EqModel { E_EQP1A, E_MEQ5, E_MAAG, E_SSL, E_NEVE84, E_LIND81, E_73, E_PE1C, kEqModels };
inline const char* eqName (int m) { static const char* n[] = { "EQP-1A", "MEQ-5", "MAAG EQ4", "SSL E", "NEVE 1084", "LINDELL 81", "73 EQ", "PE 1C" }; return n[juce::jlimit (0, kEqModels - 1, m)]; }
inline const char* eqDesc (int m)
{
    static const char* d[] = {
        "Pultec EQP-1A a valvole: boost e attenuazione dei bassi insieme (il trucco), alti dolci. Colore della valvola di uscita.",
        "Pultec MEQ-5 a valvole: tre bande sui medi. Picco basso, DIP per pulire, picco alto per la presenza.",
        "Maag EQ4: pulitissimo, AIR BAND fino a 40 kHz. Brillantezza senza durezza.",
        "SSL E: chirurgico, Q che si stringe con il guadagno, colore mordente.",
        "Neve 1084: classe A, bassi rotondi, medi larghi e musicali.",
        "Lindell 81 (1081): quattro bande, Neve ma piu pulito.",
        "Neve 1073: il piu colorato, trasformatore che spinge in faccia.",
        "Tube-Tech PE 1C stile Pultec: boost e cut sui bassi, alti morbidissimi." };
    return d[juce::jlimit (0, kEqModels - 1, m)];
}
inline int aeOf (int m) { return m - E_MAAG; }   // modelli condivisi con il Butter
inline const float* meqLpF() { static const float f[5] = { 200, 300, 500, 700, 1000 }; return f; }
inline const float* meqDipF() { static const float f[11] = { 200, 300, 500, 700, 1000, 1500, 2000, 3000, 4000, 5000, 7000 }; return f; }
inline const float* meqHpF() { static const float f[5] = { 1500, 2000, 3000, 4000, 5000 }; return f; }

// ======================================================================= moduli del rack
// tipo 0 = slot vuoto, 1..kPreModels = preamp, poi gli EQ di colore
constexpr int kSlots = 6, kSlotParams = 10;
constexpr int kModTypes = 1 + kPreModels + kEqModels;
inline bool isPre (int t) { return t >= 1 && t <= kPreModels; }
inline bool isEq (int t) { return t > kPreModels && t < kModTypes; }
inline int preOf (int t) { return t - 1; }
inline int eqOf (int t) { return t - 1 - kPreModels; }
inline juce::String modName (int t) { return t == 0 ? juce::String ("VUOTO") : isPre (t) ? juce::String (preInfo (preOf (t)).name) : juce::String (eqName (eqOf (t))); }
inline juce::String modDesc (int t) { return t == 0 ? juce::String ("Scegli un preamp o un EQ dal menu.") : isPre (t) ? juce::String (preInfo (preOf (t)).desc) : juce::String (eqDesc (eqOf (t))); }

// un comando di un modulo: valore reale = lo + v * (hi - lo); con items e a scatti
enum PFmt { F_NUM, F_DB, F_PCT, F_BOOL, F_ITEMS, F_AEF, F_HZ, F_MS, F_CUSTOM, F_BIP };   // HZ/MS: scala logaritmica; BIP: -100..+100
struct PSpec { const char* label = nullptr; float lo = 0, hi = 1, def = 0; int fmt = F_NUM; juce::StringArray items; int aeBand = -1; int cap = 2; };
inline bool hidden (const PSpec& s) { return s.label == nullptr; }
inline int steps (const PSpec& s) { return s.fmt == F_BOOL ? 2 : s.fmt == F_ITEMS ? s.items.size() : s.fmt == F_AEF ? 99 : 0; }
inline std::vector<PSpec> modSpec (int t)
{
    std::vector<PSpec> v ((size_t) kSlotParams);
    auto S = [&v] (int i, const char* l, float lo, float hi, float def, int fmt, juce::StringArray it = {}, int cap = 2) { auto& s = v[(size_t) i]; s.label = l; s.lo = lo; s.hi = hi; s.def = def; s.fmt = fmt; s.items = std::move (it); s.cap = cap; };
    if (isPre (t))
    {
        S (0, "GAIN", -10, 30, 0, F_DB, {}, 0); S (1, "TRIM", -24, 12, 0, F_DB); S (2, "LOW CUT", 0, 4, 0, F_ITEMS, { "OFF", "50", "80", "160", "300" }, 1);
        S (3, "MIX", 0, 1, 1, F_PCT, {}, 1); S (4, "AUTO", 0, 1, 1, F_BOOL); S (5, "FASE", 0, 1, 0, F_BOOL);
    }
    else if (isEq (t))
    {
        const int e = eqOf (t);
        if (e == E_EQP1A)
        {
            S (0, "LOW FREQ", 0, 3, 1, F_ITEMS, { "20", "30", "60", "100" }); S (1, "BOOST", 0, 10, 0, F_NUM); S (2, "ATTEN", 0, 10, 0, F_NUM);
            S (3, "HIGH FREQ", 0, 6, 3, F_ITEMS, { "3k", "4k", "5k", "8k", "10k", "12k", "16k" }); S (4, "BOOST", 0, 10, 0, F_NUM); S (5, "BANDWIDTH", 0, 10, 5, F_NUM);
            S (6, "ATTEN SEL", 0, 2, 1, F_ITEMS, { "5k", "10k", "20k" }); S (7, "ATTEN", 0, 10, 0, F_NUM); S (8, "TUBE", 0, 1, 0.3f, F_PCT, {}, 0);
        }
        else if (e == E_MEQ5)
        {
            S (0, "LOW PEAK", 0, 4, 1, F_ITEMS, { "200", "300", "500", "700", "1k" }); S (1, "PEAK", 0, 10, 0, F_NUM);
            S (2, "DIP", 0, 10, 4, F_ITEMS, { "200", "300", "500", "700", "1k", "1.5k", "2k", "3k", "4k", "5k", "7k" }); S (3, "DIP", 0, 10, 0, F_NUM);
            S (4, "HIGH PEAK", 0, 4, 2, F_ITEMS, { "1.5k", "2k", "3k", "4k", "5k" }); S (5, "PEAK", 0, 10, 0, F_NUM); S (6, "TUBE", 0, 1, 0.3f, F_PCT, {}, 0);
        }
        else
        {
            const auto& I = jb::aeInfo (aeOf (e));
            for (int k = 0; k < 4; ++k)
            {
                const auto& B = I.b[k];
                if (B.type != jb::Bq::HP) S (k, B.name, -15, 15, 0, F_DB, {}, 0);
                const bool sharedF = aeOf (e) == jb::AE_PE1C && k == 1;   // il taglio dei bassi del PE 1C segue la frequenza del boost
                if (! sharedF && (B.freqs.size() > 1 || B.type == jb::Bq::HP)) { S (4 + k, B.type == jb::Bq::HP ? B.name : "FREQ", 0, 1, 0.5f, F_AEF, {}, 1); v[(size_t) (4 + k)].aeBand = k; if (B.type == jb::Bq::HP) v[(size_t) (4 + k)].def = 0; }
            }
            if (e == E_SSL || e == E_NEVE84 || e == E_LIND81 || e == E_PE1C) S (8, e == E_PE1C ? "BANDWIDTH" : e == E_NEVE84 ? "HI-Q" : "Q", 0, 1, 0.5f, F_PCT, {}, 1);
            S (9, "DRIVE", 0, 1, 0.3f, F_PCT, {}, 0);
        }
    }
    return v;
}
// ordine in cui disporre i comandi nel modulo (due colonne: coppie che vanno lette insieme)
inline std::vector<int> displayOrder (int t)
{
    if (isEq (t) && eqOf (t) == E_EQP1A) return { 0, 3, 1, 4, 2, 5, 6, 7, 8 };
    if (isEq (t) && eqOf (t) != E_MEQ5) return { 0, 4, 1, 5, 2, 6, 3, 7, 8, 9 };
    std::vector<int> v; for (int i = 0; i < kSlotParams; ++i) v.push_back (i); return v;
}
inline float specReal (const PSpec& s, float v)
{
    if (s.fmt == F_BOOL) return v > 0.5f ? 1.0f : 0.0f;
    if (s.fmt == F_ITEMS) return (float) juce::roundToInt (v * (s.items.size() - 1));
    if (s.fmt == F_AEF) return v;
    if ((s.fmt == F_HZ || s.fmt == F_MS) && s.lo > 0) return s.lo * std::pow (s.hi / s.lo, v);
    return s.lo + v * (s.hi - s.lo);
}
inline float specNorm (const PSpec& s, float real)
{
    if (s.fmt == F_BOOL) return real > 0.5f ? 1.0f : 0.0f;
    if (s.fmt == F_ITEMS) return s.items.size() > 1 ? juce::jlimit (0.0f, 1.0f, real / (float) (s.items.size() - 1)) : 0.0f;
    if (s.fmt == F_AEF) return juce::jlimit (0.0f, 1.0f, real);
    if ((s.fmt == F_HZ || s.fmt == F_MS) && s.lo > 0) return juce::jlimit (0.0f, 1.0f, std::log (juce::jmax (s.lo, real) / s.lo) / std::log (s.hi / s.lo));
    return juce::jlimit (0.0f, 1.0f, (real - s.lo) / (s.hi - s.lo));
}
inline juce::String specText (int t, int i, float v)
{
    const auto sp = modSpec (t); const auto& s = sp[(size_t) i]; const float r = specReal (s, v);
    switch (s.fmt)
    {
        case F_DB: return (r > 0.05f ? "+" : "") + juce::String (r, 1) + " dB";
        case F_PCT: return juce::String (juce::roundToInt (r * 100)) + "%";
        case F_BOOL: return r > 0.5f ? "ON" : "OFF";
        case F_ITEMS: { const auto it = s.items[(int) r]; return it == "OFF" ? it : it + " Hz"; }
        case F_AEF:
        {
            const auto& B = jb::aeInfo (aeOf (eqOf (t))).b[s.aeBand]; const float f = B.freqs[(size_t) jb::aeIndex (B, v)];
            if (B.type == jb::Bq::HP && f < 1) return "OFF";
            return f >= 1000 ? juce::String (f / 1000.0f, f >= 10000 ? 0 : 1) + " kHz" : juce::String (juce::roundToInt (f)) + " Hz";
        }
        default: return juce::String (r, 1);
    }
}

struct SlotSet { int type = 0; bool byp = false; float v[kSlotParams] {}; };
struct Band { float f = 1000, g = 0, q = 4; bool dyn = false, solo = false; float thr = -24; int ms = 0; };
struct Settings
{
    // centro: EQ dinamico sottrattivo o soppressore di risonanze
    bool cOn = false; int cMode = 0; int cPos = 3;
    float hpHz = 20, lpHz = 20000; int hpSlope = 1, lpSlope = 1; Band b[4]; float att = 10, rel = 120;
    float rDepth = 0.5f, rSharp = 0.5f, rLo = 200, rHi = 12000, rSpeed = 0.5f; int rMs = 0; bool rDelta = false;
    SlotSet slot[kSlots];
    float outDb = 0;
};

// ----------------------------------------------------------------------- stadio preamp
struct Preamp
{
    jkit::Bq hp[2], lp[2], low[2], mid[2], air[2]; float lfLp[2] {}, dc[2] {}, env[2] {};
    int lastModel = -1; double sr = 44100;
    void prepare (double s) { sr = s; lastModel = -1; for (int c = 0; c < 2; ++c) { hp[c].reset(); lp[c].reset(); low[c].reset(); mid[c].reset(); air[c].reset(); lfLp[c] = dc[c] = env[c] = 0; } }
    void setModel (int m)
    {
        if (m == lastModel) return; lastModel = m; const auto& I = preInfo (m);
        for (int c = 0; c < 2; ++c)
        {
            hp[c].hp (sr, I.hpHz, 0.6); lp[c].lp (sr, juce::jmin ((double) I.lpHz, sr * 0.45), 0.6);
            low[c].lowShelf (sr, I.lowHz, I.lowDb, 0.7); mid[c].peak (sr, I.midHz, I.midQ, I.midDb); air[c].highShelf (sr, I.airHz, I.airDb, 0.7);
        }
    }
    // x gia sovracampionato e moltiplicato per il gain del pre
    inline float run (float x, int c, const PreInfo& I, float sre)
    {
        const float H = juce::Decibels::decibelsToGain (I.headroom) * 0.5f;   // livello a cui lo stadio inizia a piegare
        // trasformatore d'ingresso: le basse saturano prima (nucleo che si satura)
        const float aLf = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 110.0f / sre);
        lfLp[c] += (x - lfLp[c]) * aLf; const float lo = lfLp[c], Hl = H * 0.35f;
        float y = x + (Hl * std::tanh (lo / Hl) - lo) * juce::jmin (1.0f, I.lfSat * 0.8f);
        // valvola: comprime dolcemente seguendo l'inviluppo (sag dell'alimentazione)
        if (I.tube)
        {
            const float a = std::abs (y) / H; env[c] += (a - env[c]) * (a > env[c] ? 0.0015f : 0.00015f);
            y /= 1.0f + 0.35f * env[c] * env[c];
        }
        // stadio di guadagno: asimmetria (seconda armonica) e ginocchio (terza)
        const float u = y / H, k = I.knee, b = I.bias;
        const float nb = std::tanh (b);
        y = H * (std::tanh (k * u + b) - nb) / (k * (1.0f - nb * nb));
        // trasformatore d'uscita
        const float v = y / H; y -= 0.04f * I.lfSat * v * v * v * H;
        return y;
    }
    inline float tone (float y, int c) { y = hp[c].p (y); y = lp[c].p (y); y = low[c].p (y); y = mid[c].p (y); return air[c].p (y); }
    inline float dcBlock (float y, int c) { dc[c] += (y - dc[c]) * 0.0003f; return y - dc[c]; }
};

// ritardo intero per tenere costante la latenza (bypass e mix in parallelo)
struct DelayLine
{
    std::vector<float> d[2]; int w = 0, n = 0;
    void prepare (int len) { n = juce::jmax (0, len); for (auto& x : d) x.assign ((size_t) juce::jmax (1, n), 0.0f); w = 0; }
    inline void run (float& l, float& r) { if (n <= 0) return; const float ol = d[0][(size_t) w], orr = d[1][(size_t) w]; d[0][(size_t) w] = l; d[1][(size_t) w] = r; w = (w + 1) % n; l = ol; r = orr; }
};

// ----------------------------------------------------------------------- un modulo del rack
class Slot
{
public:
    void prepare (double s, int maxBlock)
    {
        sr = s; maxB = maxBlock;
        os = std::make_unique<juce::dsp::Oversampling<float>> (2, 2, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, true);
        os->initProcessing ((size_t) maxBlock);
        osLat = juce::roundToInt (os->getLatencyInSamples());
        pre.prepare (sr); dly.prepare (osLat); dry.assign ((size_t) maxBlock * 2, 0.0f);
        for (auto& f : eq) { f.z1[0] = f.z1[1] = f.z2[0] = f.z2[1] = 0; }
        for (auto& c : lc) c.reset();
        lastType = -1; eqInit = false; lastLc = -1;
    }
    int latency (int type) const { return isPre (type) ? osLat : 0; }
    // curva in frequenza del modulo (per lo schermo)
    static double curveDb (const SlotSet& S, double f, double sr)
    {
        if (S.byp || S.type == 0) return 0;
        const auto sp = modSpec (S.type); double d = 0;
        if (isPre (S.type))
        {
            const auto& I = preInfo (preOf (S.type)); jkit::Bq pt[5]; pt[0].hp (sr, I.hpHz, 0.6); pt[1].lp (sr, juce::jmin ((double) I.lpHz, sr * 0.45), 0.6); pt[2].lowShelf (sr, I.lowHz, I.lowDb, 0.7); pt[3].peak (sr, I.midHz, I.midQ, I.midDb); pt[4].highShelf (sr, I.airHz, I.airDb, 0.7);
            const float mix = specReal (sp[3], S.v[3]);
            for (auto& q : pt) d += q.magDb (f, sr) * mix;
            const int lci = (int) specReal (sp[2], S.v[2]); static const float lcf[] = { 0, 50, 80, 160, 300 };
            if (lci > 0) { jkit::Bq h; h.hp (sr, lcf[lci], 0.7071); d += 2 * h.magDb (f, sr); }
            return d;
        }
        jb::Bq c[4]; bool act[4]; int n = 4; eqFilters (S, sr, c, act, n);
        for (int k = 0; k < n; ++k) if (act[k]) d += c[k].magDb (f, sr);
        return d;
    }
    static void eqFilters (const SlotSet& S, double sr, jb::Bq* f, bool* act, int& n)
    {
        const int e = eqOf (S.type); const auto sp = modSpec (S.type); auto R = [&] (int i) { return specReal (sp[(size_t) i], S.v[i]); };
        n = 4;
        if (e == E_EQP1A)
        {
            jb::PultecSettings pu; pu.lf = (int) R (0); pu.lb = R (1); pu.la = R (2); pu.hf = (int) R (3); pu.hb = R (4); pu.bw = R (5); pu.as = (int) R (6); pu.ha = R (7);
            jb::pultecFilters (pu, sr, f); act[0] = pu.lb > 0.01f; act[1] = pu.la > 0.01f; act[2] = pu.hb > 0.01f; act[3] = pu.ha > 0.01f; return;
        }
        if (e == E_MEQ5)
        {
            f[0].set (jb::Bq::PEAK, sr, meqLpF()[juce::jlimit (0, 4, (int) R (0))], R (1), 0.75); act[0] = R (1) > 0.01f;
            f[1].set (jb::Bq::PEAK, sr, meqDipF()[juce::jlimit (0, 10, (int) R (2))], -R (3), 0.9); act[1] = R (3) > 0.01f;
            f[2].set (jb::Bq::PEAK, sr, meqHpF()[juce::jlimit (0, 4, (int) R (4))], R (5), 1.1); act[2] = R (5) > 0.01f;
            f[3].set (jb::Bq::PEAK, sr, 1000, 0, 1); act[3] = false; n = 3; return;
        }
        jb::AeSettings a; a.model = aeOf (e);
        for (int k = 0; k < 4; ++k) { a.g[k] = hidden (sp[(size_t) k]) ? 0.0f : R (k); a.f[k] = S.v[4 + k]; }
        a.q = hidden (sp[8]) ? 0.5f : S.v[8]; a.drive = R (9);
        for (int k = 0; k < 4; ++k) jb::aeBandFilter (a, k, sr, f[k], act[k]);
    }
    void process (float* L, float* R, int n, const SlotSet& S)
    {
        if (S.type != lastType) { lastType = S.type; eqInit = false; dly.prepare (latency (S.type)); for (auto& c : lc) c.reset(); lastLc = -1; }
        if (S.type == 0) return;
        if (S.byp) { if (isPre (S.type)) for (int i = 0; i < n; ++i) dly.run (L[i], R[i]); return; }   // stessa latenza anche in bypass
        const auto sp = modSpec (S.type); auto Rv = [&] (int i) { return specReal (sp[(size_t) i], S.v[i]); };
        float* ch[2] = { L, R };
        if (isPre (S.type))
        {
            const int m = preOf (S.type); pre.setModel (m); const auto& I = preInfo (m);
            const float gdb = Rv (0), g = juce::Decibels::decibelsToGain (gdb), mix = Rv (3);
            const int lci = (int) Rv (2); static const float lcf[] = { 0, 50, 80, 160, 300 };
            if (lci != lastLc) { lastLc = lci; if (lci > 0) for (int c = 0; c < 4; ++c) lc[c].hp (sr, lcf[lci], c % 2 ? 1.3066 : 0.5412); }
            // copia pulita (per il mix in parallelo), ritardata come il sovracampionamento
            for (int i = 0; i < n; ++i) { float l = L[i], r = R[i]; dly.run (l, r); dry[(size_t) i] = l; dry[(size_t) (maxB + i)] = r; }
            juce::dsp::AudioBlock<float> blk (ch, 2, (size_t) n);
            auto up = os->processSamplesUp (blk);
            const float sre = (float) sr * 4;
            for (int c = 0; c < 2; ++c) { float* d = up.getChannelPointer ((size_t) c); const int N = (int) up.getNumSamples(); for (int i = 0; i < N; ++i) d[i] = pre.run (d[i] * g, c, I, sre); }
            os->processSamplesDown (blk);
            const float comp = Rv (4) > 0.5f ? 1.0f / g : 1.0f;   // AUTO: il gain si sente come colore, non come volume
            const float og = juce::Decibels::decibelsToGain (Rv (1)) * comp * (Rv (5) > 0.5f ? -1.0f : 1.0f);
            for (int c = 0; c < 2; ++c)
                for (int i = 0; i < n; ++i)
                {
                    float y = pre.dcBlock (pre.tone (ch[c][i], c), c) * og;
                    if (lci > 0) y = lc[c * 2 + 1].p (lc[c * 2].p (y));
                    const float d = dry[(size_t) (c * maxB + i)] * (Rv (5) > 0.5f ? -1.0f : 1.0f) * juce::Decibels::decibelsToGain (Rv (1));
                    ch[c][i] = mix >= 0.999f ? y : d + (y - d) * mix;
                }
            return;
        }
        // EQ di colore
        float sig[kSlotParams + 1]; sig[0] = (float) S.type; for (int i = 0; i < kSlotParams; ++i) sig[i + 1] = S.v[i];
        if (! eqInit || std::memcmp (sig, lastSig, sizeof (sig)) != 0)
        {
            jb::Bq nf[4]; int nb = 4; eqFilters (S, sr, nf, act, nb); nEq = nb;
            for (int k = 0; k < 4; ++k) { auto keep = eq[k]; eq[k] = nf[k]; std::memcpy (eq[k].z1, keep.z1, sizeof (keep.z1)); std::memcpy (eq[k].z2, keep.z2, sizeof (keep.z2)); }
            std::memcpy (lastSig, sig, sizeof (sig)); eqInit = true;
        }
        const int e = eqOf (S.type); const bool tubeEq = e == E_EQP1A || e == E_MEQ5;
        const int colModel = tubeEq ? jb::AE_PE1C : aeOf (e);
        const float drive = tubeEq ? Rv (e == E_EQP1A ? 8 : 6) : Rv (9);
        for (int c = 0; c < 2; ++c) for (int i = 0; i < n; ++i)
        {
            float x = ch[c][i];
            for (int k = 0; k < nEq; ++k) if (act[k]) x = eq[k].run (x, c);
            ch[c][i] = colour.run (x, c, colModel, drive, sr);
        }
    }
    Preamp pre;
private:
    double sr = 44100; int maxB = 2048, osLat = 0, lastType = -1, lastLc = -1, nEq = 4;
    std::unique_ptr<juce::dsp::Oversampling<float>> os;
    DelayLine dly; std::vector<float> dry; jkit::Bq lc[4];
    jb::Bq eq[4]; bool act[4] {}; bool eqInit = false; float lastSig[kSlotParams + 1] {};
    jb::AeColour colour;
};

// ----------------------------------------------------------------------- soppressore di risonanze (STFT)
class Resonance
{
public:
    static constexpr int kView = 256;
    void prepare (double s)
    {
        sr = s; order = sr > 60000 ? 12 : 11; N = 1 << order; H = N / 4;
        fft = std::make_unique<juce::dsp::FFT> (order);
        win.resize ((size_t) N); for (int i = 0; i < N; ++i) win[(size_t) i] = std::sqrt (0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) i / (float) N));
        for (int c = 0; c < 2; ++c) { inQ[c].assign ((size_t) N, 0.0f); outQ[c].assign ((size_t) N, 0.0f); }
        work.assign ((size_t) (2 * N), 0.0f); work2 = work; spec[0].assign ((size_t) N, {}); spec[1] = spec[0]; cbuf = spec[0];
        red.assign ((size_t) (N / 2 + 1), 0.0f); det.assign (red.size(), 0.0f); env = det; pre = det; tmp = det; w = 0;
        for (auto& v : view) v = 0;
    }
    int latency() const { return N; }
    // x0/x1: due canali (L/R oppure M/S); target 0 = entrambi, 1 = solo il primo, 2 = solo il secondo
    void process (float* x0, float* x1, int n, const Settings& S, int target)
    {
        float* ch[2] = { x0, x1 };
        for (int i = 0; i < n; ++i)
        {
            for (int c = 0; c < 2; ++c) { inQ[c][(size_t) (N - H + w)] = ch[c][i]; ch[c][i] = outQ[c][(size_t) w]; }
            if (++w >= H) { w = 0; frame (S, target); }
        }
    }
    std::atomic<float> view[kView];   // riduzione in dB su scala logaritmica 20 Hz - 20 kHz
    std::atomic<float> total { 0 };
private:
    void frame (const Settings& S, int target)
    {
        const int nb = N / 2 + 1;
        for (int c = 0; c < 2; ++c)
        {
            for (int i = 0; i < N; ++i) { work[(size_t) i] = inQ[c][(size_t) i] * win[(size_t) i]; work[(size_t) (N + i)] = 0; }
            for (int i = 0; i < N; ++i) cbuf[(size_t) i] = { work[(size_t) i], 0.0f };
            fft->perform (cbuf.data(), spec[c].data(), false);
        }
        // rilevazione: potenza dei canali coinvolti
        for (int k = 0; k < nb; ++k)
        {
            const float p0 = std::norm (spec[0][(size_t) k]), p1 = std::norm (spec[1][(size_t) k]);
            const float p = target == 1 ? p0 : target == 2 ? p1 : 0.5f * (p0 + p1);
            det[(size_t) k] = 10.0f * std::log10 (p + 1e-14f);
        }
        // inviluppo dello spettro: media in frequenza su una finestra proporzionale (in ottave); SHARP la stringe
        const float oct = 1.0f - 0.8f * S.rSharp;   // da 1 ottava a 1/5 di ottava
        const float wFac = std::pow (2.0f, oct * 0.5f) - 1.0f;
        pre[0] = det[0]; for (int k = 1; k < nb; ++k) pre[(size_t) k] = pre[(size_t) (k - 1)] + det[(size_t) k];
        for (int k = 1; k < nb; ++k)
        {
            const int hw = juce::jlimit (1, nb / 4, (int) (k * wFac) + 1); const int a = juce::jmax (0, k - hw), b = juce::jmin (nb - 1, k + hw);
            env[(size_t) k] = (pre[(size_t) b] - (a > 0 ? pre[(size_t) (a - 1)] : 0.0f)) / (float) (b - a + 1);
        }
        // riduzione: quanto il picco sporge sull'inviluppo, solo nella zona scelta, con attacco veloce e rilascio da SPEED
        const float maxRed = 3.0f + S.rDepth * 15.0f, slope = 0.4f + S.rDepth * 1.1f;
        const float hop = (float) H / (float) sr, relT = 0.03f + (1.0f - S.rSpeed) * 0.45f;
        const float aAtt = 1.0f - std::exp (-hop / 0.006f), aRel = 1.0f - std::exp (-hop / relT);
        const float binHz = (float) sr / (float) N; float tot = 0, totW = 0;
        for (int k = 1; k < nb; ++k)
        {
            const float f = k * binHz;
            float zone = 1.0f;
            if (f < S.rLo) zone = juce::jlimit (0.0f, 1.0f, 1.0f + 3.0f * std::log2 (f / S.rLo));
            if (f > S.rHi) zone = juce::jmin (zone, juce::jlimit (0.0f, 1.0f, 1.0f - 3.0f * std::log2 (f / S.rHi)));
            const float loud = det[(size_t) k] > -100.0f ? 1.0f : 0.0f;   // niente lavoro sul silenzio
            const float ex = det[(size_t) k] - env[(size_t) k] - 2.5f;
            const float t = ex > 0 ? juce::jmin (maxRed, ex * slope) * zone * loud : 0.0f;
            tmp[(size_t) k] = t;
        }
        tmp[0] = 0;
        for (int k = 1; k < nb - 1; ++k)
        {   // un filo di lisciatura tra bin vicini: meno artefatti "metallici"
            const float t = 0.25f * tmp[(size_t) (k - 1)] + 0.5f * tmp[(size_t) k] + 0.25f * tmp[(size_t) (k + 1)];
            red[(size_t) k] += (t - red[(size_t) k]) * (t > red[(size_t) k] ? aAtt : aRel);
            tot += red[(size_t) k] * std::exp (det[(size_t) k] * 0.05f); totW += std::exp (det[(size_t) k] * 0.05f);
        }
        total = totW > 0 ? tot / totW : 0.0f;
        // applica (o DELTA: fai sentire solo cio che viene tolto)
        for (int c = 0; c < 2; ++c)
        {
            const bool apply = target == 0 || (target == 1 && c == 0) || (target == 2 && c == 1);
            for (int k = 0; k < nb; ++k)
            {
                float g = apply ? juce::Decibels::decibelsToGain (-red[(size_t) k]) : 1.0f;
                if (S.rDelta) g = apply ? 1.0f - g : 0.0f;
                spec[c][(size_t) k] *= g; if (k > 0 && k < N / 2) spec[c][(size_t) (N - k)] *= g;
            }
            auto& tout = cbuf; fft->perform (spec[c].data(), tout.data(), true);
            // sovrapposizione: finestra di sintesi e scala per il 75% (somma delle finestre = 2)
            auto& oq = outQ[c];
            std::memmove (oq.data(), oq.data() + H, sizeof (float) * (size_t) (N - H)); std::fill (oq.begin() + (N - H), oq.end(), 0.0f);
            for (int i = 0; i < N; ++i) oq[(size_t) i] += tout[(size_t) i].real() * win[(size_t) i] * 0.5f;
            std::memmove (inQ[c].data(), inQ[c].data() + H, sizeof (float) * (size_t) (N - H));
        }
        for (int v = 0; v < kView; ++v)
        {
            const float f = 20.0f * std::pow (1000.0f, (float) v / (kView - 1)); const int k = juce::jlimit (1, nb - 2, (int) (f / binHz));
            view[v] = red[(size_t) k];
        }
    }
    double sr = 44100; int order = 11, N = 2048, H = 512, w = 0;
    std::unique_ptr<juce::dsp::FFT> fft;
    std::vector<float> win, inQ[2], outQ[2], work, work2, red, det, env, pre, tmp;
    std::vector<std::complex<float>> spec[2], cbuf;
};

// ----------------------------------------------------------------------- centro: EQ dinamico sottrattivo
class Core
{
public:
    void prepare (double s)
    {
        sr = s; res.prepare (s); resDly.prepare (res.latency());
        for (auto& b : st) for (auto& f : b) f.reset();
        for (auto& b : hpF) for (auto& f : b) f.reset();
        for (auto& b : lpF) for (auto& f : b) f.reset();
        for (auto& d : bd) { for (auto& f : d.det) f.reset(); d.env = 0; d.cut = 0; d.coefCut = 999; for (auto& f : d.flt) f.reset(); for (auto& f : d.solo) f.reset(); }
        lastKey = {}; init = false;
    }
    int latency (const Settings& S) const { return S.cMode == 1 ? res.latency() : 0; }
    static void staticFilters (const Settings& S, double sr, jkit::Bq (&h)[4], jkit::Bq (&l)[4])
    {
        static const double q2[1] = { 0.7071 }, q4[2] = { 0.5412, 1.3066 }, q8[4] = { 0.5098, 0.6013, 0.9000, 2.5629 };
        auto make = [&] (jkit::Bq (&f)[4], bool hi, double fc, int slope)
        {
            const double* q = slope == 0 ? q2 : slope == 1 ? q4 : q8; const int n = slope == 0 ? 1 : slope == 1 ? 2 : 4;
            for (int k = 0; k < 4; ++k) { if (k < n) { if (hi) f[k].hp (sr, fc, q[k]); else f[k].lp (sr, fc, q[k]); } else f[k].flat(); }
        };
        make (h, true, S.hpHz, S.hpSlope); make (l, false, S.lpHz, S.lpSlope);
    }
    void process (float* L, float* R, int n, const Settings& S)
    {
        if (S.cMode == 1)
        {   // anti-risonanze: lavora su L/R (stereo) o su M/S
            if (! S.cOn) { for (int i = 0; i < n; ++i) resDly.run (L[i], R[i]); for (auto& v : res.view) v = 0; res.total = 0; return; }
            if (S.rMs == 0) { res.process (L, R, n, S, 0); return; }
            for (int i = 0; i < n; ++i) { const float m = 0.5f * (L[i] + R[i]), s = 0.5f * (L[i] - R[i]); L[i] = m; R[i] = s; }
            res.process (L, R, n, S, S.rMs);
            for (int i = 0; i < n; ++i) { const float m = L[i], s = R[i]; L[i] = m + s; R[i] = m - s; }
            return;
        }
        for (auto& d : bd) d.grUi = 0;
        if (! S.cOn) return;
        const auto key = std::make_tuple (S.hpHz, S.lpHz, S.hpSlope, S.lpSlope);
        if (! init || key != lastKey) { jkit::Bq h[4], l[4]; staticFilters (S, sr, h, l); for (int c = 0; c < 2; ++c) for (int k = 0; k < 4; ++k) { cp (hpF[c][k], h[k]); cp (lpF[c][k], l[k]); } lastKey = key; init = true; }
        const bool hpOn = S.hpHz > 20.5f, lpOn = S.lpHz < 19900;
        int soloB = -1; for (int k = 0; k < 4; ++k) if (S.b[k].solo) soloB = k;
        const float aA = 1.0f - std::exp (-1.0f / (0.001f * S.att * (float) sr)), aR = 1.0f - std::exp (-1.0f / (0.001f * S.rel * (float) sr));
        // filtri dei rilevatori e del solo (cambiano solo con frequenza e Q)
        for (int k = 0; k < 4; ++k)
        {
            auto& D = bd[k]; const auto& B = S.b[k];
            if (B.f != D.lf || B.q != D.lq) { for (int c = 0; c < 2; ++c) { bp (D.det[c], B.f, B.q); bp (D.solo[c], B.f, B.q); } D.lf = B.f; D.lq = B.q; D.coefCut = 999; }
        }
        for (int i = 0; i < n; ++i)
        {
            float m = 0.5f * (L[i] + R[i]), s = 0.5f * (L[i] - R[i]);
            if (hpOn) for (int k = 0; k < 4; ++k) { m = hpF[0][k].p (m); s = hpF[1][k].p (s); }
            if (lpOn) for (int k = 0; k < 4; ++k) { m = lpF[0][k].p (m); s = lpF[1][k].p (s); }
            float om = m, os2 = s;
            if (soloB >= 0)
            {   // ascolta solo la zona della banda (sul canale su cui lavora)
                const auto& B = S.b[soloB]; auto& D = bd[soloB];
                om = B.ms == 2 ? 0.0f : D.solo[0].p (m); os2 = B.ms == 1 ? 0.0f : D.solo[1].p (s);
            }
            for (int k = 0; k < 4; ++k)
            {
                const auto& B = S.b[k]; auto& D = bd[k];
                if (B.g > -0.05f) { D.cut = 0; continue; }
                float target = B.g;
                if (B.dyn)
                {   // dinamico: taglia solo quando la zona supera la soglia, fino al massimo impostato
                    const float a = juce::jmax (B.ms == 2 ? 0.0f : std::abs (D.det[0].p (m)), B.ms == 1 ? 0.0f : std::abs (D.det[1].p (s)));
                    D.env += (a - D.env) * (a > D.env ? aA : aR);
                    const float over = juce::Decibels::gainToDecibels (D.env, -100.0f) - B.thr;
                    target = over > 0 ? juce::jmax (B.g, -over * 0.75f) : 0.0f;
                }
                D.cut += (target - D.cut) * 0.002f;
                if ((i & 15) == 0 && std::abs (D.cut - D.coefCut) > 0.05f) { for (int c = 0; c < 2; ++c) D.flt[c].peak (sr, B.f, B.q, D.cut); D.coefCut = D.cut; }
                if (soloB < 0)
                {
                    if (B.ms != 2) om = D.flt[0].p (om);
                    if (B.ms != 1) os2 = D.flt[1].p (os2);
                }
                D.grUi = juce::jmin (D.grUi, D.cut);
            }
            L[i] = om + os2; R[i] = om - os2;
        }
        for (int k = 0; k < 4; ++k) gr[k] = bd[k].grUi;
    }
    std::atomic<float> gr[4] {};
    Resonance res;
private:
    static void cp (jkit::Bq& d, const jkit::Bq& s) { d.b0 = s.b0; d.b1 = s.b1; d.b2 = s.b2; d.a1 = s.a1; d.a2 = s.a2; }
    void bp (jkit::Bq& f, float fc, float q)
    {   // passa banda a guadagno unitario al centro
        const double w = juce::MathConstants<double>::twoPi * juce::jlimit (10.0, sr * 0.45, (double) fc) / sr, cs = std::cos (w), a = std::sin (w) / (2 * juce::jmax (0.3, (double) q));
        f.set (a, 0, -a, 1 + a, -2 * cs, 1 - a);
    }
    double sr = 44100;
    jkit::Bq hpF[2][4], lpF[2][4], st[2][4];
    struct BandDyn { jkit::Bq det[2], flt[2], solo[2]; float env = 0, cut = 0, coefCut = 999, lf = -1, lq = -1, grUi = 0; } bd[4];
    std::tuple<float, float, int, int> lastKey {}; bool init = false;
    DelayLine resDly;
};

// ----------------------------------------------------------------------- motore: rack con il centro in mezzo
class Engine
{
public:
    void prepare (double s, int maxBlock) { sr = s; for (auto& sl : slots) sl.prepare (s, maxBlock); core.prepare (s); }
    int latency (const Settings& S) const { int l = core.latency (S); for (int k = 0; k < kSlots; ++k) l += slots[k].latency (S.slot[k].type); return l; }
    void process (float* L, float* R, int n, const Settings& S)
    {
        for (int i = 0; i < n; ++i) inPk = juce::jmax (inPk.load() * 0.99995f, juce::jmax (std::abs (L[i]), std::abs (R[i])));
        const int cp = juce::jlimit (0, kSlots, S.cPos);
        for (int k = 0; k <= kSlots; ++k)
        {
            if (k == cp) core.process (L, R, n, S);
            if (k < kSlots) slots[k].process (L, R, n, S.slot[k]);
        }
        const float og = juce::Decibels::decibelsToGain (S.outDb);
        for (int i = 0; i < n; ++i) { L[i] *= og; R[i] *= og; outPk = juce::jmax (outPk.load() * 0.99995f, juce::jmax (std::abs (L[i]), std::abs (R[i]))); }
    }
    std::atomic<float> inPk { 0 }, outPk { 0 };
    Slot slots[kSlots];
    Core core;
private:
    double sr = 44100;
};
}
