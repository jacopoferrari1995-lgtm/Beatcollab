#pragma once
// JACKPOT SPACE: rack di riverberi e delay.
//  RIVERBERI   PLATE (EMT 140 / Valhalla Plate), HALL 224 (Lexicon 224), ROOM, SPRING (molla stile IK)
//  SPAZIALI    SHIMMER (Crystalline: coda che sale di ottava), SPACED (Spaced Out: eco dentro il riverbero, sparkle, ducking),
//              MORPH (passa da stanza a lastra a sala), NEOVERB (stile iZotope: tre motori miscelati + unmask sulla voce)
//  CONVOLUZIONE  8 ambienti veri ricostruiti (studio, chamber, chiesa, cattedrale, plate d'epoca, tunnel, legno, cisterna), anche al contrario
//  DELAY       TAPE ECHO, BBD (pedale analogico), DIGITAL, REVERSE, MULTITAP
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <vector>
#include <cmath>

namespace jsp
{
enum Type { OFF = 0, PLATE, HALL, ROOM, SPRING, TAPE, DIGITAL, SHIMMER, SPACED, MORPH, NEOVERB, CONV, BBD, REVERSE, MULTITAP, kNumTypes };
constexpr int kP = 8;   // parametri per motore
// come si legge ogni parametro
enum Kind { KP, KDEC, KPRE, KLO, KHI, KBR, KSIZE, KBX, KTX, KSPR, KSPT, KTIME, KFB, KSYNC, KTT, KOFF, KWID, KPITCH, KIR, KPAT, KLEN, KDIV, KMORPH, KONOFF, KBIP };
struct EngInfo { const char* name; const char* group; const char* desc; int n; const char* pn[kP]; float def[kP]; Kind kind[kP]; };
inline const EngInfo& info (int t)
{
    static const EngInfo I[kNumTypes] = {
        { "VUOTO", "-", "Slot vuoto.", 0, {}, {}, {} },
        { "PLATE", "RIVERBERI", "Lastra stile EMT 140 e Valhalla Plate: densa subito, brillante, coda liscia. Voce, rullante, synth.", 8,
          { "DECAY", "PRE-DELAY", "SIZE", "BRILLO", "MOD", "LO-CUT", "HI-CUT", "WIDTH" }, { 0.40f, 0.08f, 0.55f, 0.60f, 0.30f, 0.30f, 0.80f, 0.67f },
          { KDEC, KPRE, KSIZE, KBR, KP, KLO, KHI, KWID } },
        { "HALL 224", "RIVERBERI", "Sala stile Lexicon 224: profonda, con il chorus nelle code e i bassi che durano diversamente dagli alti. Pad, archi, voci lente.", 8,
          { "DECAY", "PRE-DELAY", "SIZE", "BASSI x", "ALTI x", "CHORUS", "DIFFUSIONE", "HI-CUT" }, { 0.45f, 0.10f, 0.60f, 0.50f, 0.45f, 0.35f, 0.70f, 0.75f },
          { KDEC, KPRE, KSIZE, KBX, KTX, KP, KP, KHI } },
        { "ROOM", "RIVERBERI", "Stanza: riflessioni vicine e coda corta, mette lo strumento in un posto vero. Batteria, chitarre, doppie.", 8,
          { "DECAY", "PRE-DELAY", "SIZE", "EARLY", "BRILLO", "MOD", "LO-CUT", "HI-CUT" }, { 0.40f, 0.02f, 0.45f, 0.60f, 0.55f, 0.15f, 0.25f, 0.75f },
          { KDEC, KPRE, KSIZE, KP, KBR, KP, KLO, KHI } },
        { "SPRING", "RIVERBERI", "Molla da amplificatore e da dub: il suono metallico che rimbalza, sgocciola sui colpi. Chitarre, dub, lo-fi.", 8,
          { "DECAY", "TENSIONE", "MOLLE", "DRIP", "TONO", "LO-CUT", "WIDTH", "PRE-DELAY" }, { 0.45f, 0.50f, 0.50f, 0.40f, 0.50f, 0.30f, 0.60f, 0.0f },
          { KDEC, KP, KSPR, KP, KSPT, KLO, KP, KPRE } },
        { "TAPE ECHO", "DELAY", "Eco a nastro: ripetizioni che si scaldano e si scuriscono, il nastro che oscilla. Voci, dub, chitarre.", 8,
          { "TEMPO", "FEEDBACK", "SYNC", "WOW", "TONO", "DRIVE", "PING-PONG", "LO-CUT" }, { 0.73f, 0.38f, 1.0f, 0.30f, 0.50f, 0.30f, 0.0f, 0.25f },
          { KTIME, KFB, KSYNC, KP, KTT, KP, KP, KLO } },
        { "DIGITAL", "DELAY", "Delay digitale pulito: ripetizioni precise, modulazione per allargare, ping-pong. Per tutto.", 8,
          { "TEMPO", "FEEDBACK", "SYNC", "MOD", "HI-CUT", "LO-CUT", "PING-PONG", "OFFSET R" }, { 0.73f, 0.35f, 1.0f, 0.10f, 0.80f, 0.20f, 0.0f, 0.50f },
          { KTIME, KFB, KSYNC, KP, KHI, KLO, KP, KOFF } },
        { "SHIMMER", "SPAZIALI", "Stile Crystalline: la coda sale di ottava (o di quinta) e diventa luce. Pad, voci lente, intro.", 8,
          { "DECAY", "PRE-DELAY", "SHIMMER", "PITCH", "SIZE", "TONO", "MOD", "LO-CUT" }, { 0.55f, 0.10f, 0.45f, 0.60f, 0.70f, 0.60f, 0.40f, 0.35f },
          { KDEC, KPRE, KP, KPITCH, KSIZE, KHI, KP, KLO } },
        { "SPACED", "SPAZIALI", "Stile Spaced Out: eco a tempo che finisce dentro un riverbero enorme, sparkle all'ottava e ducking. Voci, chitarre, synth.", 8,
          { "SPACE", "ECHO", "TEMPO", "BLEND", "MOD", "SPARKLE", "TONO", "DUCK" }, { 0.55f, 0.45f, 0.727f, 0.50f, 0.40f, 0.20f, 0.60f, 0.30f },
          { KDEC, KFB, KDIV, KP, KP, KP, KHI, KP } },
        { "MORPH", "SPAZIALI", "Uno spazio che si trasforma: da stanza a lastra a sala con una manopola (da automatizzare nei passaggi).", 8,
          { "DECAY", "PRE-DELAY", "MORPH", "SIZE", "BRILLO", "MOD", "LO-CUT", "HI-CUT" }, { 0.45f, 0.10f, 0.50f, 0.55f, 0.60f, 0.30f, 0.30f, 0.80f },
          { KDEC, KPRE, KMORPH, KSIZE, KBR, KP, KLO, KHI } },
        { "NEOVERB", "SPAZIALI", "Stile iZotope: tre motori (lastra, stanza, sala) miscelati e l'UNMASK che toglie dalla coda le frequenze dove sta cantando la voce.", 8,
          { "DECAY", "PRE-DELAY", "PLATE", "ROOM", "HALL", "UNMASK", "LO-CUT", "HI-CUT" }, { 0.45f, 0.10f, 0.60f, 0.30f, 0.50f, 0.50f, 0.30f, 0.75f },
          { KDEC, KPRE, KP, KP, KP, KP, KLO, KHI } },
        { "CONVOLUTION", "CONVOLUZIONE", "Ambienti veri ricostruiti: studio, chamber, chiesa, cattedrale, plate d'epoca, tunnel, legno, cisterna. Anche al contrario.", 8,
          { "SPAZIO", "LUNGHEZZA", "PRE-DELAY", "SCURO", "LO-CUT", "HI-CUT", "WIDTH", "REVERSE" }, { 0.15f, 0.50f, 0.0f, 0.30f, 0.25f, 0.85f, 0.67f, 0.0f },
          { KIR, KLEN, KPRE, KP, KLO, KHI, KWID, KONOFF } },
        { "BBD", "DELAY", "Delay analogico a secchielli (Memory Man, Carbon Copy): piu' lungo = piu' scuro, chorus e un filo di sporco.", 8,
          { "TEMPO", "FEEDBACK", "SYNC", "MOD", "TONO", "DRIVE", "PING-PONG", "LO-CUT" }, { 0.73f, 0.38f, 1.0f, 0.35f, 0.55f, 0.30f, 0.0f, 0.20f },
          { KTIME, KFB, KSYNC, KP, KTT, KP, KP, KLO } },
        { "REVERSE", "DELAY", "Delay al contrario: ogni pezzo torna indietro e sale verso il colpo. Intro, transizioni, voci sognanti.", 6,
          { "TEMPO", "FEEDBACK", "SYNC", "HI-CUT", "LO-CUT", "WIDTH" }, { 0.909f, 0.30f, 1.0f, 0.70f, 0.30f, 0.50f },
          { KTIME, KFB, KSYNC, KHI, KLO, KP } },
        { "MULTITAP", "DELAY", "Quattro ripetizioni a ritmo (dritto, puntato, terzine, rampa, swing, cascata) sparse nello stereo.", 8,
          { "TEMPO", "FEEDBACK", "SYNC", "PATTERN", "SPREAD", "HI-CUT", "LO-CUT", "CODA" }, { 0.727f, 0.20f, 1.0f, 0.0f, 0.70f, 0.70f, 0.25f, 0.50f },
          { KTIME, KFB, KSYNC, KPAT, KP, KHI, KLO, KP } } };
    return I[juce::jlimit (0, kNumTypes - 1, t)];
}
inline bool isDelay (int t) { return t == TAPE || t == DIGITAL || t == BBD || t == REVERSE || t == MULTITAP; }

// divisioni a tempo
constexpr int kDivs = 12;
inline const char* divName (int i) { static const char* n[kDivs] = { "1/32", "1/16T", "1/16", "1/16D", "1/8T", "1/8", "1/8D", "1/4T", "1/4", "1/4D", "1/2", "1/1" }; return n[juce::jlimit (0, kDivs - 1, i)]; }
inline double divBeats (int i) { static const double b[kDivs] = { 0.125, 1.0 / 6, 0.25, 0.375, 1.0 / 3, 0.5, 0.75, 2.0 / 3, 1.0, 1.5, 2.0, 4.0 }; return b[juce::jlimit (0, kDivs - 1, i)]; }
constexpr int kPitches = 6;
inline const char* pitchName (int i) { static const char* n[kPitches] = { "-12", "+5", "+7", "+12", "+19", "+24" }; return n[juce::jlimit (0, kPitches - 1, i)]; }
inline float pitchRatio (int i) { static const float s[kPitches] = { -12, 5, 7, 12, 19, 24 }; return std::pow (2.0f, s[juce::jlimit (0, kPitches - 1, i)] / 12.0f); }
constexpr int kIRs = 8;
inline const char* irName (int i) { static const char* n[kIRs] = { "STUDIO A", "CHAMBER", "CHIESA", "CATTEDRALE", "PLATE '70", "TUNNEL", "LEGNO", "CISTERNA" }; return n[juce::jlimit (0, kIRs - 1, i)]; }
constexpr int kPats = 6;
inline const char* patName (int i) { static const char* n[kPats] = { "DRITTO", "PUNTATO", "TERZINE", "RAMPA", "SWING", "CASCATA" }; return n[juce::jlimit (0, kPats - 1, i)]; }

// ---------------------------------------------------------------------------------------------------- mappe 0..1 -> valori
namespace map
{
inline float decay (int t, float n) { return t == ROOM ? 0.1f * std::pow (40.0f, n) : t == SPRING ? 0.4f * std::pow (15.0f, n) : t == PLATE ? 0.2f * std::pow (100.0f, n) : 0.3f * std::pow (100.0f, n); }
inline float pre (float n) { return n * 250.0f; }                              // ms
inline float loCut (float n) { return 20.0f * std::pow (25.0f, n); }            // 20..500 Hz
inline float hiCut (float n) { return 1500.0f * std::pow (13.33f, n); }         // 1.5..20 kHz
inline float bright (float n) { return 1000.0f * std::pow (16.0f, n); }         // smorzamento in coda 1..16 kHz
inline float delayMs (float n) { return 20.0f * std::pow (100.0f, n); }         // 20..2000 ms
inline int div (float n) { return juce::jlimit (0, kDivs - 1, (int) std::round (n * (kDivs - 1))); }
inline float bassX (float n) { return 0.5f * std::pow (4.0f, n); }               // 0.5x .. 2x
inline float trebX (float n) { return 0.1f + 0.9f * n; }
inline int springs (float n) { return 1 + juce::jlimit (0, 2, (int) std::round (n * 2)); }
inline int pick (float n, int count) { return juce::jlimit (0, count - 1, (int) std::round (n * (count - 1))); }
inline float len (float n) { return 0.5f * std::pow (4.0f, n); }                // 0.5x .. 2x
inline float tapeTone (float n) { return 1500.0f * std::pow (6.0f, n); }
// dal valore vero allo 0..1 (per i preset)
inline float nDecay (int t, float sec) { const float a = t == ROOM ? 0.1f : t == SPRING ? 0.4f : t == PLATE ? 0.2f : 0.3f, b = t == ROOM ? 40.0f : t == SPRING ? 15.0f : 100.0f; return juce::jlimit (0.0f, 1.0f, std::log (sec / a) / std::log (b)); }
inline float nPre (float ms) { return juce::jlimit (0.0f, 1.0f, ms / 250.0f); }
inline float nLo (float hz) { return juce::jlimit (0.0f, 1.0f, std::log (hz / 20.0f) / std::log (25.0f)); }
inline float nHi (float hz) { return juce::jlimit (0.0f, 1.0f, std::log (hz / 1500.0f) / std::log (13.33f)); }
inline float nBr (float hz) { return juce::jlimit (0.0f, 1.0f, std::log (hz / 1000.0f) / std::log (16.0f)); }
inline float nMs (float ms) { return juce::jlimit (0.0f, 1.0f, std::log (ms / 20.0f) / std::log (100.0f)); }
inline float nDiv (const char* d) { for (int i = 0; i < kDivs; ++i) if (juce::String (divName (i)) == d) return (float) i / (kDivs - 1); return 8.0f / 11; }
inline float nTT (float hz) { return juce::jlimit (0.0f, 1.0f, std::log (hz / 1500.0f) / std::log (6.0f)); }
inline float nPick (int i, int count) { return (float) i / (count - 1); }
}
inline juce::String fmtS (float s) { return s < 1 ? juce::String (juce::roundToInt (s * 1000)) + " ms" : juce::String (s, s < 10 ? 2 : 1) + " s"; }
inline juce::String fmtHz (float f) { return f >= 1000 ? juce::String (f / 1000.0f, 1) + " kHz" : juce::String (juce::roundToInt (f)) + " Hz"; }
inline juce::String pct (float n) { return juce::String (juce::roundToInt (n * 100)) + "%"; }
inline juce::String paramText (int t, int i, float n, const float* all)
{
    switch (info (t).kind[juce::jlimit (0, kP - 1, i)])
    {
        case KDEC: return fmtS (map::decay (t, n));
        case KPRE: return juce::String (juce::roundToInt (map::pre (n))) + " ms";
        case KLO: return fmtHz (map::loCut (n));
        case KHI: return fmtHz (map::hiCut (n));
        case KBR: return fmtHz (map::bright (n));
        case KSIZE: return pct (0.5f + n);
        case KBX: return juce::String (map::bassX (n), 2) + "x";
        case KTX: return juce::String (map::trebX (n), 2) + "x";
        case KSPR: return juce::String (map::springs (n));
        case KSPT: return fmtHz (2000.0f * std::pow (4.0f, n));
        case KTIME: return all != nullptr && all[2] > 0.5f ? juce::String (divName (map::div (n))) : juce::String (juce::roundToInt (map::delayMs (n))) + " ms";
        case KDIV: return divName (map::div (n));
        case KFB: return pct (n * 1.1f);
        case KSYNC: return n > 0.5f ? "A TEMPO" : "LIBERO";
        case KTT: return fmtHz (map::tapeTone (n));
        case KOFF: return (n > 0.5f ? "+" : "") + juce::String (juce::roundToInt ((n - 0.5f) * 40)) + " ms";
        case KWID: return pct (n * 1.5f);
        case KPITCH: return juce::String (pitchName (map::pick (n, kPitches))) + " st";
        case KIR: return irName (map::pick (n, kIRs));
        case KPAT: return patName (map::pick (n, kPats));
        case KLEN: return pct (map::len (n));
        case KONOFF: return n > 0.5f ? "SI" : "NO";
        case KMORPH: return n < 0.25f ? "ROOM" : n < 0.45f ? "ROOM > PLATE" : n < 0.55f ? "PLATE" : n < 0.75f ? "PLATE > HALL" : "HALL";
        case KBIP: return (n > 0.5f ? "+" : "") + juce::String (juce::roundToInt ((n - 0.5f) * 200)) + "%";
        default: return pct (n);
    }
}

// ---------------------------------------------------------------------------------------------------- mattoni
struct Line
{
    std::vector<float> b; int w = 0, mask = 0;
    void prepare (int maxN) { int n = 1; while (n < maxN + 4) n <<= 1; b.assign ((size_t) n, 0.0f); mask = n - 1; w = 0; }
    void clear() { std::fill (b.begin(), b.end(), 0.0f); }
    void push (float x) { b[(size_t) w] = x; w = (w + 1) & mask; }
    float at (int d) const { return b[(size_t) ((w - 1 - d) & mask)]; }
    float frac (float d) const   // lettura frazionaria (Hermite)
    {
        const int i = (int) d; const float f = d - (float) i;
        const float y0 = at (i - 1), y1 = at (i), y2 = at (i + 1), y3 = at (i + 2);
        const float c1 = 0.5f * (y2 - y0), c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3, c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
        return ((c3 * f + c2) * f + c1) * f + y1;
    }
};
struct AP   // allpass di Schroeder su una linea
{
    Line l; int d = 1;
    void prepare (int maxN) { l.prepare (maxN); }
    float run (float x, float g) { const float v = l.at (d - 1); const float u = x + g * v; l.push (u); return v - g * u; }
    float runMod (float x, float g, float dd) { const float v = l.frac (dd - 1); const float u = x + g * v; l.push (u); return v - g * u; }
};
struct LP1 { float z = 0; float run (float x, float a) { z += a * (x - z); return z; } };
inline float coef (float hz, double sr) { return 1.0f - std::exp (-juce::MathConstants<float>::twoPi * juce::jlimit (5.0f, 0.49f * (float) sr, hz) / (float) sr); }
inline float rtGain (float lenSamples, float rt, double sr) { return std::pow (10.0f, -3.0f * lenSamples / (juce::jmax (0.05f, rt) * (float) sr)); }

// ---------------------------------------------------------------------------------------------------- PLATE (Dattorro)
class Plate
{
public:
    void prepare (double s)
    {
        sr = s; k = (float) (sr / 29761.0) * 2.0f;   // spazio per SIZE fino a 1.5x
        for (int i = 0; i < 4; ++i) inAp[i].prepare ((int) (380 * k) + 8);
        for (int c = 0; c < 2; ++c) { modAp[c].prepare ((int) (910 * k) + 64); d1[c].prepare ((int) (4460 * k) + 8); ap2[c].prepare ((int) (2660 * k) + 8); d2[c].prepare ((int) (3730 * k) + 8); }
        pre.prepare ((int) (0.26 * sr) + 8); clear();
    }
    void clear() { for (auto& a : inAp) a.l.clear(); for (int c = 0; c < 2; ++c) { modAp[c].l.clear(); d1[c].clear(); ap2[c].l.clear(); d2[c].clear(); damp[c] = {}; fb[c] = 0; } bw = {}; ph = 0; }
    void process (const float* inL, const float* inR, float* oL, float* oR, int n, const float* p, bool freeze)
    {
        const float sz = 0.5f + p[2], kk = (float) (sr / 29761.0) * sz;
        static const int inLen[4] = { 142, 107, 379, 277 };
        for (int i = 0; i < 4; ++i) inAp[i].d = juce::jmax (1, (int) (inLen[i] * kk));
        const float L1[2] = { 672 * kk, 908 * kk }; const int D1[2] = { (int) (4453 * kk), (int) (4217 * kk) }, A2[2] = { (int) (1800 * kk), (int) (2656 * kk) }, D2[2] = { (int) (3720 * kk), (int) (3163 * kk) };
        for (int c = 0; c < 2; ++c) { ap2[c].d = A2[c]; }
        const float rt = juce::jmax (0.08f, (map::decay (PLATE, p[0]) - 0.24f) / 0.943f);   // taratura: diffusori e prese d'uscita allungano le code corte
        const float loopLen = (L1[0] + D1[0] + A2[0] + D2[0]);   // giro di mezzo serbatoio
        const float dec = freeze ? 1.0f : juce::jmin (0.9995f, rtGain (loopLen * 0.5f, rt, sr));   // due guadagni per mezzo serbatoio
        const float dmp = coef (map::bright (p[3]), sr), bwc = coef (14000.0f, sr);
        const float exc = 16.0f * (float) (sr / 29761.0) * (0.2f + 2.0f * p[4]), dphi = (float) (juce::MathConstants<double>::twoPi * 0.9 / sr);
        const int preN = juce::jmax (0, (int) (map::pre (p[1]) * 0.001 * sr));
        const float sc = 0.6f;
        for (int i = 0; i < n; ++i)
        {
            pre.push (freeze ? 0.0f : 0.5f * (inL[i] + inR[i]));
            float x = bw.run (pre.at (preN), bwc);
            x = inAp[0].run (x, 0.75f); x = inAp[1].run (x, 0.75f); x = inAp[2].run (x, 0.625f); x = inAp[3].run (x, 0.625f);
            ph += dphi; if (ph > juce::MathConstants<float>::twoPi) ph -= juce::MathConstants<float>::twoPi;
            const float m[2] = { exc * std::sin (ph), exc * std::cos (ph * 0.87f) };
            float y[2];
            for (int c = 0; c < 2; ++c)
            {
                float t = x + dec * fb[1 - c];
                t = modAp[c].runMod (t, -0.70f, L1[c] + m[c] + exc + 2);
                d1[c].push (t); t = d1[c].at (D1[c]);
                t = damp[c].run (t, dmp) * dec;
                t = ap2[c].run (t, 0.5f);
                d2[c].push (t); y[c] = d2[c].at (D2[c]);
            }
            fb[0] = y[0]; fb[1] = y[1];
            // uscite: prese multiple dal serbatoio (tabella di Dattorro), sinistra e destra decorrelate
            const float l = d1[1].at ((int) (266 * kk)) + d1[1].at ((int) (2974 * kk)) - ap2[1].l.at ((int) (1913 * kk)) + d2[1].at ((int) (1996 * kk)) - d1[0].at ((int) (1990 * kk)) - ap2[0].l.at ((int) (187 * kk)) - d2[0].at ((int) (1066 * kk));
            const float r = d1[0].at ((int) (353 * kk)) + d1[0].at ((int) (3627 * kk)) - ap2[0].l.at ((int) (1228 * kk)) + d2[0].at ((int) (2673 * kk)) - d1[1].at ((int) (2111 * kk)) - ap2[1].l.at ((int) (335 * kk)) - d2[1].at ((int) (121 * kk));
            oL[i] = l * sc; oR[i] = r * sc;
        }
    }
private:
    double sr = 48000; float k = 1, ph = 0, fb[2] {};
    AP inAp[4], modAp[2], ap2[2]; Line d1[2], d2[2], pre; LP1 damp[2], bw;
};

// ---------------------------------------------------------------------------------------------------- FDN a 8 linee (HALL e ROOM)
class Fdn
{
public:
    static constexpr int N = 8;
    void prepare (double s)
    {
        sr = s;
        for (int i = 0; i < N; ++i) d[i].prepare ((int) (0.2 * sr) + 64);
        for (int i = 0; i < 4; ++i) diff[i].prepare ((int) (0.03 * sr) + 8);
        pre.prepare ((int) (0.26 * sr) + 8); er.prepare ((int) (0.12 * sr) + 8);
        clear();
    }
    void clear() { for (auto& l : d) l.clear(); for (auto& a : diff) a.l.clear(); pre.clear(); er.clear(); for (int i = 0; i < N; ++i) { lp[i] = {}; lp2[i] = {}; ph[i] = i * 0.7f; } }
    // room: true = stanza (linee corte e riflessioni forti)
    void process (const float* inL, const float* inR, float* oL, float* oR, int n, int type, const float* p, bool freeze)
    {
        const bool room = type == ROOM;
        static const float base[N] = { 29.7f, 37.1f, 41.1f, 43.7f, 53.3f, 59.3f, 67.9f, 79.3f };   // ms, primi fra loro
        const float sz = (0.5f + p[2]) * (room ? 0.32f : 1.0f);
        const float rt = map::decay (type, p[0]) * 1.065f;   // taratura: la modulazione e i diffusori accorciano un poco la coda
        const float rtLo = room ? rt : rt * map::bassX (p[3]), rtHi = room ? rt * (0.15f + 0.85f * p[4]) : rt * map::trebX (p[4]);
        const float xoL = coef (350.0f, sr), xoH = coef (room ? 3000.0f : 4000.0f, sr);   // tre bande: bassi x, medi = DECAY, alti x (come il 224)
        const float modD = (room ? p[5] : p[5]) * 0.0009f * (float) sr;   // fino a ~1 ms di escursione: il chorus del 224
        const float diffG = room ? 0.6f : 0.35f + 0.4f * p[6];
        const int preN = (int) (map::pre (p[1]) * 0.001 * sr);
        float len[N], gLo[N], gMid[N], gHi[N];
        for (int i = 0; i < N; ++i)
        {
            len[i] = base[i] * sz * 0.001f * (float) sr;
            gLo[i] = freeze ? 1.0f : juce::jmin (0.9998f, rtGain (len[i], rtLo, sr)); gMid[i] = freeze ? 1.0f : juce::jmin (0.9998f, rtGain (len[i], rt, sr)); gHi[i] = freeze ? 1.0f : juce::jmin (0.9998f, rtGain (len[i], rtHi, sr));
        }
        static const int dl[4] = { 113, 167, 271, 383 };   // diffusori d'ingresso (campioni a 48k)
        for (int i = 0; i < 4; ++i) diff[i].d = juce::jmax (1, (int) (dl[i] * sr / 48000.0 * (room ? 0.6f : 1.0f)));
        const float erG = room ? 0.9f * p[3] : 0.25f;
        static const float erT[8] = { 4.3f, 7.9f, 11.3f, 17.1f, 23.9f, 31.3f, 41.9f, 53.7f };
        const float dph = (float) (juce::MathConstants<double>::twoPi / sr);
        for (int s = 0; s < n; ++s)
        {
            const float inM = freeze ? 0.0f : 0.5f * (inL[s] + inR[s]), inS = freeze ? 0.0f : 0.5f * (inL[s] - inR[s]);
            pre.push (inM); er.push (inM);
            float x = pre.at (preN);
            for (int i = 0; i < 4; ++i) x = diff[i].run (x, diffG);
            // lettura con modulazione lenta (ogni linea con il suo giro)
            float o[N];
            for (int i = 0; i < N; ++i)
            {
                ph[i] += dph * (0.25f + 0.11f * i); if (ph[i] > 6.2832f) ph[i] -= 6.2832f;
                o[i] = d[i].frac (len[i] + modD * (1 + std::sin (ph[i])) + 1);
                const float l = lp[i].run (o[i], xoL), m = lp2[i].run (o[i], xoH); o[i] = l * gLo[i] + (m - l) * gMid[i] + (o[i] - m) * gHi[i];   // bassi, medi e alti decadono diversi
            }
            // matrice di Hadamard 8x8 (energia conservata)
            float h[N]; for (int i = 0; i < N; ++i) h[i] = o[i];
            for (int st = 1; st < N; st <<= 1) for (int i = 0; i < N; i += st * 2) for (int j = i; j < i + st; ++j) { const float a = h[j], b = h[j + st]; h[j] = a + b; h[j + st] = a - b; }
            const float nrm = 0.35355339f;
            for (int i = 0; i < N; ++i) d[i].push (h[i] * nrm + (i & 4 ? -x : x) * 0.5f + ((i ^ (i >> 1)) & 1 ? -inS : inS) * 0.3f);
            float l = 0, r = 0; for (int i = 0; i < N; ++i) { l += i & 1 ? -o[i] : o[i]; r += i & 2 ? -o[i] : o[i]; }
            // riflessioni iniziali (dal segnale prima del pre-delay della coda)
            float el = 0, erv = 0; for (int k = 0; k < 8; ++k) { const float v = er.at ((int) (erT[k] * sz * (room ? 2.2f : 1.0f) * 0.001f * (float) sr)) * (1.0f - k * 0.09f); if (k & 1) erv += v; else el += v; }
            oL[s] = 0.52f * l + erG * el * 0.6f; oR[s] = 0.52f * r + erG * erv * 0.6f;
        }
    }
private:
    double sr = 48000; Line d[N], pre, er; AP diff[4]; LP1 lp[N], lp2[N]; float ph[N] {};
};

// ---------------------------------------------------------------------------------------------------- SPRING
class Spring
{
public:
    static constexpr int kAp = 48;
    void prepare (double s) { sr = s; for (int c = 0; c < 3; ++c) { loop[c].prepare ((int) (0.12 * sr) + 8); } pre.prepare ((int) (0.26 * sr) + 8); clear(); }
    void clear() { for (int c = 0; c < 3; ++c) { loop[c].clear(); for (auto& z : apz[c]) z = 0; for (auto& z : apx[c]) z = 0; lp[c] = {}; hp[c] = 0; fb[c] = 0; } pre.clear(); }
    void process (const float* inL, const float* inR, float* oL, float* oR, int n, const float* p, bool freeze)
    {
        const int ns = map::springs (p[2]);
        const float rt = map::decay (SPRING, p[0]);
        const float a = -0.55f - 0.3f * p[1];                       // dispersione: piu' tensione, piu' "cio-ing"
        static const float lenMs[3] = { 37.0f, 43.5f, 51.3f };
        const float tone = coef (2000.0f * std::pow (4.0f, p[4]), sr), hpc = coef (map::loCut (p[5]) + 60.0f, sr);
        const int preN = (int) (map::pre (p[7]) * 0.001 * sr);
        for (int i = 0; i < n; ++i)
        {
            pre.push (freeze ? 0.0f : 0.5f * (inL[i] + inR[i]));
            float x = pre.at (preN);
            // drip: i transienti eccitano la molla in modo non lineare (il "plic" del riverbero a molla)
            const float drip = p[3] * 3.0f; x = x + drip * (x - std::tanh (x * 2.5f) / 2.5f) * 4.0f;
            float out[3] = { 0, 0, 0 };
            for (int c = 0; c < ns; ++c)
            {
                const float L = lenMs[c] * (1.0f + 0.15f * (p[1] - 0.5f)) * 0.001f * (float) sr;
                const float g = freeze ? 0.999f : juce::jmin (0.995f, rtGain (L, rt, sr));
                float v = x * 0.6f + fb[c] * g;
                for (int k = 0; k < kAp; ++k) { const float y = a * v + apx[c][k] - a * apz[c][k]; apx[c][k] = v; apz[c][k] = y; v = y; }   // allpass del 1o ordine in cascata
                v = lp[c].run (v, tone);
                hp[c] += hpc * (v - hp[c]); v -= hp[c];
                loop[c].push (v); fb[c] = loop[c].frac (L);
                out[c] = fb[c];
            }
            const float w = p[6];
            const float m = (out[0] + out[1] + out[2]) / (float) ns;
            const float side = ns > 1 ? (out[0] - out[1] + (ns > 2 ? 0.5f * out[2] : 0.0f)) * 0.5f : out[0] * 0.0f;
            oL[i] = (m + side * w) * 0.56f; oR[i] = (m - side * w) * 0.56f;
        }
    }
private:
    double sr = 48000; Line loop[3], pre; float apz[3][kAp] {}, apx[3][kAp] {}, hp[3] {}, fb[3] {}; LP1 lp[3];
};

// ---------------------------------------------------------------------------------------------------- DELAY a linea (nastro, BBD, digitale)
class Delay
{
public:
    void prepare (double s) { sr = s; for (auto& l : ln) l.prepare ((int) (4.2 * sr) + 64); clear(); }
    void clear() { for (auto& l : ln) l.clear(); for (int c = 0; c < 2; ++c) { lp[c] = {}; lp2[c] = {}; hp[c] = 0; } tS[0] = tS[1] = -1; ph = ph2 = 0; }
    void process (const float* inL, const float* inR, float* oL, float* oR, int n, int type, const float* p, double bpm)
    {
        const bool tape = type == TAPE, bbd = type == BBD, warm = tape || bbd, sync = p[2] > 0.5f;
        float tMs = sync ? (float) (divBeats (map::div (p[0])) * 60000.0 / juce::jmax (30.0, bpm)) : map::delayMs (p[0]);
        tMs = juce::jlimit (5.0f, 4000.0f, tMs);
        const float off = warm ? 0.0f : (p[7] - 0.5f) * 40.0f;
        const float tgt[2] = { tMs * 0.001f * (float) sr, juce::jlimit (5.0f, 4000.0f, tMs + off) * 0.001f * (float) sr };
        const float fbk = p[1] * 1.1f, pp = p[6];
        // il BBD: piu' lungo il ritardo, piu' basso il clock, piu' scuro il suono (come i pedali veri)
        float hiHz = warm ? map::tapeTone (p[4]) : map::hiCut (p[4]);
        if (bbd) hiHz = juce::jmin (hiHz * 1.3f, juce::jlimit (1600.0f, 12000.0f, 10000.0f * std::pow (60.0f / tMs, 0.55f)));
        const float lc = coef (hiHz, sr), hc = coef (map::loCut (warm ? p[7] : p[5]), sr);
        const float drive = tape ? 1.0f + p[5] * 4.0f : bbd ? 1.0f + p[5] * 3.0f : 1.0f;
        const float wow = tape ? p[3] : 0.0f, mod = tape ? 0.0f : p[3];
        for (int c = 0; c < 2; ++c) if (tS[c] < 0) tS[c] = tgt[c];
        const float glide = warm ? 0.00015f : 0.002f;
        const float d1 = (float) (juce::MathConstants<double>::twoPi * (bbd ? 0.9 : 0.6) / sr), d2 = (float) (juce::MathConstants<double>::twoPi * 6.1 / sr);
        for (int i = 0; i < n; ++i)
        {
            ph += d1; ph2 += d2; if (ph > 6.2832f) ph -= 6.2832f; if (ph2 > 6.2832f) ph2 -= 6.2832f;
            const float wobble = (wow * (0.0025f * std::sin (ph) + 0.0004f * std::sin (ph2)) + mod * (bbd ? 0.003f : 0.0015f) * std::sin (ph * 1.7f)) * (float) sr;
            float rd[2];
            for (int c = 0; c < 2; ++c) { tS[c] += (tgt[c] - tS[c]) * glide; rd[c] = ln[c].frac (juce::jmax (2.0f, tS[c] + (c ? -wobble : wobble))); }
            float w[2];
            for (int c = 0; c < 2; ++c)
            {
                float v = lp[c].run (rd[c], lc); if (bbd) v = lp2[c].run (v, lc);
                hp[c] += hc * (v - hp[c]); v -= hp[c];
                if (warm) v = std::tanh (v * drive) / drive * (1.0f + 0.15f * (drive - 1.0f) / 4.0f);
                w[c] = v;
            }
            const float inl = inL[i], inr = inR[i];
            const float inA = pp > 0.01f ? 0.5f * (inl + inr) : 0.0f;
            const float xl = (1 - pp) * inl + pp * inA, xr = (1 - pp) * inr;
            const float hiss = bbd ? (rnd.nextFloat() - 0.5f) * 3e-4f : 0.0f;
            ln[0].push (xl + fbk * ((1 - pp) * w[0] + pp * w[1]) + hiss);
            ln[1].push (xr + fbk * ((1 - pp) * w[1] + pp * w[0]) + hiss);
            const float mk = bbd ? 1.4f : 1.0f; oL[i] = w[0] * mk; oR[i] = w[1] * mk;
        }
    }
private:
    double sr = 48000; Line ln[2]; LP1 lp[2], lp2[2]; float hp[2] {}, tS[2] { -1, -1 }, ph = 0, ph2 = 0; juce::Random rnd { 5 };
};

// ---------------------------------------------------------------------------------------------------- REVERSE
class Reverse
{
public:
    void prepare (double s) { sr = s; for (auto& l : ln) l.prepare ((int) (8.5 * sr) + 64); clear(); }
    void clear() { for (auto& l : ln) l.clear(); ph = 0; for (int c = 0; c < 2; ++c) { lp[c] = {}; hp[c] = 0; fb[c] = 0; } }
    void process (const float* inL, const float* inR, float* oL, float* oR, int n, const float* p, double bpm)
    {
        float tMs = p[2] > 0.5f ? (float) (divBeats (map::div (p[0])) * 60000.0 / juce::jmax (30.0, bpm)) : map::delayMs (p[0]);
        const float T = juce::jlimit (0.03f, 4.0f, tMs * 0.001f) * (float) sr;
        const float fbk = p[1] * 0.95f, lc = coef (map::hiCut (p[3]), sr), hc = coef (map::loCut (p[4]), sr), wd = p[5] * 0.5f;
        const float* in[2] = { inL, inR }; float* out[2] = { oL, oR };
        for (int i = 0; i < n; ++i)
        {
            ph += 1.0f / T; if (ph >= 1.0f) ph -= 1.0f;
            for (int c = 0; c < 2; ++c)
            {
                ln[c].push (in[c][i] + fbk * fb[c]);
                float y = 0;
                for (int h = 0; h < 2; ++h)
                {
                    float f = ph + 0.5f * h + (c ? wd : 0.0f); f -= std::floor (f);
                    const float s = std::sin (juce::MathConstants<float>::pi * f);
                    y += ln[c].frac (2.0f * f * T + 2.0f) * s * s;   // legge all'indietro: due teste sfasate, finestre che sommano a 1
                }
                y = lp[c].run (y, lc); hp[c] += hc * (y - hp[c]); y -= hp[c];
                fb[c] = y; out[c][i] = y;
            }
        }
    }
private:
    double sr = 48000; Line ln[2]; float ph = 0, hp[2] {}, fb[2] {}; LP1 lp[2];
};

// ---------------------------------------------------------------------------------------------------- MULTITAP
class Multitap
{
public:
    void prepare (double s) { sr = s; ln.prepare ((int) (4.2 * sr) + 64); clear(); }
    void clear() { ln.clear(); fb = 0; for (int c = 0; c < 2; ++c) { lp[c] = {}; hp[c] = 0; } }
    void process (const float* inL, const float* inR, float* oL, float* oR, int n, const float* p, double bpm)
    {
        static const float mul[kPats][4] = { { 1, 2, 3, 4 }, { 0.75f, 1.5f, 2.25f, 3 }, { 0.667f, 1.333f, 2, 2.667f }, { 0.25f, 0.5f, 1, 2 }, { 0.5f, 0.833f, 1.5f, 1.833f }, { 0.5f, 1, 1.5f, 2 } };
        const int pat = map::pick (p[3], kPats);
        float tMs = p[2] > 0.5f ? (float) (divBeats (map::div (p[0])) * 60000.0 / juce::jmax (30.0, bpm)) : map::delayMs (p[0]);
        float T = tMs * 0.001f * (float) sr; const float last = mul[pat][3] * T, cap = 4.0f * (float) sr; if (last > cap) T *= cap / last;
        const float k = 0.3f + 0.65f * p[7], sp = p[4];
        static const float pan[4] = { -1.0f, 1.0f, -0.6f, 0.6f };
        float gl[4], gr[4], d[4];
        for (int j = 0; j < 4; ++j)
        {
            const float gg = pat == 5 ? std::pow (k, (float) (3 - j)) : std::pow (k, (float) j);
            const float a = (pan[j] * sp + 1.0f) * 0.25f * juce::MathConstants<float>::pi;
            gl[j] = gg * std::cos (a) * 1.2f; gr[j] = gg * std::sin (a) * 1.2f; d[j] = juce::jmax (2.0f, mul[pat][j] * T);
        }
        const float fbk = p[1] * 0.9f, lc = coef (map::hiCut (p[5]), sr), hc = coef (map::loCut (p[6]), sr);
        for (int i = 0; i < n; ++i)
        {
            ln.push (0.5f * (inL[i] + inR[i]) + fbk * fb);
            float l = 0, r = 0;
            for (int j = 0; j < 4; ++j) { const float v = ln.frac (d[j]); l += v * gl[j]; r += v * gr[j]; if (j == 3) fb = v; }
            l = lp[0].run (l, lc); r = lp[1].run (r, lc);
            hp[0] += hc * (l - hp[0]); l -= hp[0]; hp[1] += hc * (r - hp[1]); r -= hp[1];
            fb = std::tanh (fb);
            oL[i] = l; oR[i] = r;
        }
    }
private:
    double sr = 48000; Line ln; float fb = 0, hp[2] {}; LP1 lp[2];
};

// ---------------------------------------------------------------------------------------------------- pitch shifter a due teste (per shimmer e sparkle)
struct Shifter
{
    Line l; float ph = 0;
    void prepare (int maxN) { l.prepare (maxN); }
    void clear() { l.clear(); ph = 0; }
    float run (float x, float ratio, float win)
    {
        l.push (x);
        ph += (1.0f - ratio) / win; ph -= std::floor (ph);
        float y = 0;
        for (int h = 0; h < 2; ++h) { float f = ph + 0.5f * h; f -= std::floor (f); const float s = std::sin (juce::MathConstants<float>::pi * f); y += l.frac (f * win + 2.0f) * s * s; }
        return y;
    }
};

// ---------------------------------------------------------------------------------------------------- unmask (stile Neoverb): toglie dalla coda le bande dove suona il segnale
struct Unmask
{
    LP1 d[3], w[2][3]; float de[4] {}, we[4] {}, g[4] { 1, 1, 1, 1 };
    void clear() { *this = Unmask(); }
    void run (float dry, float& l, float& r, float amt, double sr)
    {
        const float c[3] = { coef (250, sr), coef (1500, sr), coef (5000, sr) };
        float db[4], lb[4], rb[4];
        auto split = [&] (float x, LP1* f, float* b) { const float a = f[0].run (x, c[0]), bb = f[1].run (x, c[1]), cc = f[2].run (x, c[2]); b[0] = a; b[1] = bb - a; b[2] = cc - bb; b[3] = x - cc; };
        split (dry, d, db); split (l, w[0], lb); split (r, w[1], rb);
        const float at = 1.0f - std::exp (-1.0f / (0.005f * (float) sr)), rl = 1.0f - std::exp (-1.0f / (0.15f * (float) sr));
        l = r = 0;
        for (int b = 0; b < 4; ++b)
        {
            const float dv = std::abs (db[b]), wv = 0.5f * (std::abs (lb[b]) + std::abs (rb[b]));
            de[b] += (dv > de[b] ? at : rl) * (dv - de[b]); we[b] += (wv > we[b] ? at : rl) * (wv - we[b]);
            const float tg = juce::jmax (0.2f, 1.0f / (1.0f + amt * 3.0f * de[b] / (we[b] + 1e-5f)));
            g[b] += (tg - g[b]) * 0.002f;
            l += lb[b] * g[b]; r += rb[b] * g[b];
        }
    }
};

// ---------------------------------------------------------------------------------------------------- risposte all'impulso costruite (convoluzione)
inline juce::AudioBuffer<float> makeIR (int which, float lenX, float dark, bool rev, double sr)
{
    struct Spec { float lo, mid, hi, erMs; int erN; float erG, onMs, flutMs, lpHz; };
    static const Spec S[kIRs] = {
        { 0.45f, 0.40f, 0.28f, 25, 8, 0.70f, 4, 0, 14000 },     // STUDIO A: stanza di ripresa trattata
        { 1.50f, 1.40f, 0.90f, 40, 14, 0.50f, 8, 0, 11000 },    // CHAMBER: camera d'eco in muratura (Abbey Road, Capitol)
        { 3.60f, 3.00f, 1.60f, 80, 18, 0.45f, 20, 0, 9000 },    // CHIESA
        { 7.00f, 6.00f, 3.00f, 140, 24, 0.40f, 40, 0, 8000 },   // CATTEDRALE
        { 2.20f, 2.40f, 2.20f, 0, 0, 0.0f, 1, 0, 16000 },        // PLATE '70: densa da subito, alti lunghi
        { 3.00f, 2.60f, 1.20f, 60, 6, 0.50f, 15, 23, 7000 },    // TUNNEL: con l'eco a sfarfallio
        { 0.80f, 0.60f, 0.35f, 20, 10, 0.60f, 5, 0, 8000 },     // LEGNO: sala calda e corta
        { 5.00f, 4.20f, 1.20f, 30, 12, 0.60f, 10, 0, 5000 } };  // CISTERNA: cemento, scura e lunghissima
    const auto& s = S[juce::jlimit (0, kIRs - 1, which)];
    const float rl = s.lo * lenX, rm = s.mid * lenX, rh = s.hi * lenX * (1.0f - 0.75f * dark), lpHz = s.lpHz * (1.0f - 0.6f * dark);
    const int N = juce::jlimit (256, (int) (8.0 * sr), (int) (juce::jmax (rl, rm, rh) * 1.1f * (float) sr));
    juce::AudioBuffer<float> b (2, N); b.clear();
    for (int c = 0; c < 2; ++c)
    {
        juce::Random rnd (which * 10 + c + 1); float* x = b.getWritePointer (c);
        LP1 a, m, f1, f2; const float ca = coef (300, sr), cm = coef (3000, sr), cf = coef (lpHz, sr);
        for (int i = 0; i < N; ++i)
        {
            const float t = (float) i / (float) sr, w = rnd.nextFloat() * 2 - 1;
            const float lo = a.run (w, ca), mi = m.run (w, cm) - lo, hi = w - lo - mi;
            float v = lo * std::exp (-6.9078f * t / rl) + mi * std::exp (-6.9078f * t / rm) + hi * std::exp (-6.9078f * t / rh);
            v *= 1.0f - std::exp (-t * 1000.0f / s.onMs);
            x[i] = f2.run (f1.run (v, cf), cf);
        }
        for (int j = 0; j < s.erN; ++j)
        {
            const int at = (int) ((1.0f + rnd.nextFloat() * s.erMs) * 0.001f * (float) sr);
            if (at < N) x[at] += s.erG * (1.0f - (float) j / s.erN) * (rnd.nextBool() ? 1.0f : -1.0f) * 0.8f;
        }
        if (s.flutMs > 0)
            for (int j = 1; ; ++j) { const float t = j * s.flutMs * 0.001f; const int at = (int) (t * (float) sr) + c * 7; if (at >= N) break; x[at] += 0.5f * std::exp (-6.9078f * t / (rm * 0.6f)) * ((j + c) % 2 ? 1.0f : -1.0f); }
        double e = 0; for (int i = 0; i < N; ++i) e += (double) x[i] * x[i];
        const float g = (float) std::sqrt (0.5 / juce::jmax (1e-12, e)); for (int i = 0; i < N; ++i) x[i] *= g;
        if (rev) std::reverse (x, x + N);
    }
    return b;
}

// ---------------------------------------------------------------------------------------------------- uno slot del rack
class Slot
{
public:
    static constexpr int kChunk = 32;   // passo del giro di ritorno dello shimmer
    void prepare (double s, int maxBlock)
    {
        sr = s; maxB = juce::jmax (64, maxBlock);
        plate.prepare (s); fdnA.prepare (s); fdnB.prepare (s); spring.prepare (s); dly.prepare (s); rev.prepare (s); mt.prepare (s);
        for (int c = 0; c < 2; ++c) { sh[c].prepare ((int) (0.2 * sr) + 64); conPre[c].prepare ((int) (0.26 * sr) + 8); }
        for (auto& v : buf) v.assign ((size_t) maxB, 0.0f);
        conv.reset(); conv.prepare ({ s, (juce::uint32) maxB, 2 }); convBuf.setSize (2, maxB);
        type = -1; irKey = -1; clearAll();
    }
    void clearAll()
    {
        plate.clear(); fdnA.clear(); fdnB.clear(); spring.clear(); dly.clear(); rev.clear(); mt.clear(); um.clear(); conv.reset();
        for (int c = 0; c < 2; ++c) { sh[c].clear(); conPre[c].clear(); lo[c] = 0; hi[c] = {}; tl[c] = {}; shFb[c].fill (0.0f); }
        env = 0; duckG = 1; shEnv = 0;
    }
    // scrive solo la parte "bagnata" in oL/oR
    void process (const float* inL, const float* inR, float* oL, float* oR, int n, int t, const float* p, double bpm, bool freeze)
    {
        if (t != type) { clearAll(); type = t; }
        for (int o = 0; o < n; o += maxB) { const int m = juce::jmin (maxB, n - o); run (inL + o, inR + o, oL + o, oR + o, m, t, p, bpm, freeze); }
    }
    // la convoluzione si carica dal thread dei messaggi (Processor::updateIRs)
    int irWanted (const float* p) const { return map::pick (p[0], kIRs) * 100000 + juce::roundToInt (p[1] * 40) * 1000 + juce::roundToInt (p[3] * 20) * 10 + (p[7] > 0.5f ? 1 : 0); }
    void loadIR (int key, const float* p)
    {
        irKey = key;
        conv.loadImpulseResponse (makeIR (map::pick (p[0], kIRs), map::len (p[1]), p[3], p[7] > 0.5f, sr), sr, juce::dsp::Convolution::Stereo::yes, juce::dsp::Convolution::Trim::no, juce::dsp::Convolution::Normalise::no);
    }
    int type = -1, irKey = -1;
private:
    float* B (int i) { return buf[(size_t) i].data(); }
    void run (const float* inL, const float* inR, float* oL, float* oR, int n, int t, const float* p, double bpm, bool freeze)
    {
        switch (t)
        {
            case PLATE: plate.process (inL, inR, oL, oR, n, p, freeze); break;
            case HALL: case ROOM: fdnA.process (inL, inR, oL, oR, n, t, p, freeze); break;
            case SPRING: spring.process (inL, inR, oL, oR, n, p, freeze); break;
            case TAPE: case DIGITAL: case BBD: dly.process (inL, inR, oL, oR, n, t, p, bpm); break;
            case REVERSE: rev.process (inL, inR, oL, oR, n, p, bpm); break;
            case MULTITAP: mt.process (inL, inR, oL, oR, n, p, bpm); break;
            case SHIMMER: shimmer (inL, inR, oL, oR, n, p, freeze); break;
            case SPACED: spaced (inL, inR, oL, oR, n, p, bpm, freeze); break;
            case MORPH: case NEOVERB: blend (inL, inR, oL, oR, n, t, p, freeze); break;
            case CONV: convolve (inL, inR, oL, oR, n, p); break;
            default: std::fill (oL, oL + n, 0.0f); std::fill (oR, oR + n, 0.0f); return;
        }
        // filtri e larghezza d'uscita (dove il motore li prevede)
        int lcI = -1, hcI = -1, wI = -1;
        switch (t)
        {
            case PLATE: lcI = 5; hcI = 6; wI = 7; break;
            case HALL: hcI = 7; break;
            case ROOM: case MORPH: case NEOVERB: lcI = 6; hcI = 7; break;
            case SHIMMER: lcI = 7; break;
            case SPACED: hcI = 6; break;
            case CONV: lcI = 4; hcI = 5; wI = 6; break;
            default: return;
        }
        const float a = lcI >= 0 ? coef (map::loCut (p[lcI]), sr) : 0.0f, b = hcI >= 0 ? coef (map::hiCut (p[hcI]), sr) : 0.0f, wid = wI >= 0 ? p[wI] * 1.5f : 1.0f;
        for (int i = 0; i < n; ++i)
        {
            float x[2] = { oL[i], oR[i] };
            for (int c = 0; c < 2; ++c) { if (b > 0) x[c] = hi[c].run (x[c], b); if (a > 0) { lo[c] += a * (x[c] - lo[c]); x[c] -= lo[c]; } }
            if (wid != 1) { const float m = 0.5f * (x[0] + x[1]), s = 0.5f * (x[0] - x[1]) * wid; x[0] = m + s; x[1] = m - s; }
            oL[i] = x[0]; oR[i] = x[1];
        }
    }
    // SHIMMER: sala a cui torna dentro la propria coda trasposta (giro chiuso, autolimitato)
    void shimmer (const float* inL, const float* inR, float* oL, float* oR, int n, const float* p, bool freeze)
    {
        const float amt = freeze ? 0.0f : p[2];
        const float hp[kP] = { map::nDecay (HALL, map::decay (SHIMMER, p[0]) * (1.0f - 0.25f * amt * p[0])), p[1], p[4], 0.45f, 0.75f, 0.35f + 0.5f * p[6], 0.8f, 1.0f };   // il giro dello shimmer allunga la coda: si compensa
        const float ratio = pitchRatio (map::pick (p[3], kPitches)), win = 0.06f * (float) sr, tc = coef (map::hiCut (p[5]), sr);
        const float er = 1.0f - std::exp (-1.0f / (0.2f * (float) sr));
        float* xl = B (0); float* xr = B (1);
        for (int o = 0; o < n; o += kChunk)
        {
            const int m = juce::jmin (kChunk, n - o);
            for (int i = 0; i < m; ++i) { xl[i] = inL[o + i] + shFb[0][(size_t) i]; xr[i] = inR[o + i] + shFb[1][(size_t) i]; }
            fdnA.process (xl, xr, oL + o, oR + o, m, HALL, hp, freeze);
            for (int i = 0; i < m; ++i)
            {
                float* out[2] = { oL + o, oR + o };
                const float lv = 0.5f * (std::abs (out[0][i]) + std::abs (out[1][i])); shEnv += er * (lv - shEnv);
                for (int c = 0; c < 2; ++c) { const float s = tl[c].run (sh[c].run (out[c][i], ratio, win), tc); shFb[c][(size_t) i] = amt * 0.5f * s / (1.0f + 12.0f * shEnv); }
            }
        }
    }
    // SPACED: eco a tempo che finisce in una sala enorme, sparkle all'ottava, ducking interno
    void spaced (const float* inL, const float* inR, float* oL, float* oR, int n, const float* p, double bpm, bool freeze)
    {
        const float dp[kP] = { p[2], p[1], 1.0f, 0.3f + 0.4f * p[4], p[6], map::nLo (220.0f), 1.0f, 0.5f };
        float* el = B (0); float* er = B (1); float* xl = B (2); float* xr = B (3);
        dly.process (inL, inR, el, er, n, DIGITAL, dp, bpm);
        const float bl = p[3];
        for (int i = 0; i < n; ++i) { xl[i] = inL[i] * (1.0f - 0.5f * bl) + el[i] * bl * 1.2f; xr[i] = inR[i] * (1.0f - 0.5f * bl) + er[i] * bl * 1.2f; }
        const float hp[kP] = { p[0], 0.0f, 1.0f, 0.45f, 0.6f, p[4], 0.9f, 1.0f };
        fdnA.process (xl, xr, oL, oR, n, HALL, hp, freeze);
        const float spk = p[5] * 0.8f, win = 0.05f * (float) sr, duck = p[7];
        const float at = 1.0f - std::exp (-1.0f / (0.005f * (float) sr)), rl = 1.0f - std::exp (-1.0f / (0.3f * (float) sr));
        for (int i = 0; i < n; ++i)
        {
            const float lv = 0.5f * (std::abs (inL[i]) + std::abs (inR[i])); env += (lv > env ? at : rl) * (lv - env);
            const float tg = 1.0f - duck * 0.85f * juce::jlimit (0.0f, 1.0f, env * 5.0f); duckG += (tg - duckG) * 0.003f;
            const float sl = spk > 0 ? sh[0].run (oL[i], 2.0f, win) : 0.0f, sr2 = spk > 0 ? sh[1].run (oR[i], 2.0f, win) : 0.0f;
            const float echo = 0.8f * (1.0f - 0.5f * bl);
            oL[i] = (oL[i] + sl * spk + el[i] * echo) * duckG; oR[i] = (oR[i] + sr2 * spk + er[i] * echo) * duckG;
        }
    }
    // MORPH e NEOVERB: lastra + stanza + sala insieme
    void blend (const float* inL, const float* inR, float* oL, float* oR, int n, int t, const float* p, bool freeze)
    {
        const float sec = map::decay (t, p[0]);
        const float pp[kP] = { map::nDecay (PLATE, sec), p[1], t == MORPH ? p[3] : 0.55f, t == MORPH ? p[4] : 0.6f, t == MORPH ? p[5] : 0.3f, 0, 0, 0 };
        const float hp[kP] = { p[0], p[1], t == MORPH ? p[3] : 0.6f, 0.5f, t == MORPH ? 0.2f + 0.6f * p[4] : 0.45f, t == MORPH ? p[5] : 0.35f, 0.7f, 1.0f };
        const float rp[kP] = { map::nDecay (ROOM, juce::jmin (4.0f, sec * 0.4f)), p[1], t == MORPH ? p[3] * 0.8f : 0.45f, 0.6f, t == MORPH ? p[4] : 0.55f, t == MORPH ? p[5] * 0.5f : 0.15f, 0, 0 };
        float wP, wR, wH;
        if (t == MORPH) { const float m = p[2]; wR = std::sqrt (juce::jmax (0.0f, 1.0f - 2.0f * m)); wP = std::sqrt (juce::jmax (0.0f, 1.0f - std::abs (2.0f * m - 1.0f))); wH = std::sqrt (juce::jmax (0.0f, 2.0f * m - 1.0f)); }
        else { wP = p[2]; wR = p[3]; wH = p[4]; const float sum = juce::jmax (0.6f, wP + wR + wH); wP /= sum; wR /= sum; wH /= sum; }
        float* pl = B (0); float* pr = B (1); float* rl = B (2); float* rr = B (3);
        plate.process (inL, inR, pl, pr, n, pp, freeze);
        fdnB.process (inL, inR, rl, rr, n, ROOM, rp, freeze);
        fdnA.process (inL, inR, oL, oR, n, HALL, hp, freeze);
        const float uAmt = t == NEOVERB ? p[5] : 0.0f;
        for (int i = 0; i < n; ++i)
        {
            float l = pl[i] * wP + rl[i] * wR + oL[i] * wH, r = pr[i] * wP + rr[i] * wR + oR[i] * wH;
            if (uAmt > 0.001f) um.run (0.5f * (inL[i] + inR[i]), l, r, uAmt, sr);
            const float mk = t == NEOVERB ? 1.8f : 1.0f; oL[i] = l * mk; oR[i] = r * mk;
        }
    }
    void convolve (const float* inL, const float* inR, float* oL, float* oR, int n, const float* p)
    {
        const int preN = (int) (map::pre (p[2]) * 0.001 * sr);
        float* cl = convBuf.getWritePointer (0); float* cr = convBuf.getWritePointer (1);
        for (int i = 0; i < n; ++i) { conPre[0].push (inL[i]); conPre[1].push (inR[i]); cl[i] = conPre[0].at (preN); cr[i] = conPre[1].at (preN); }
        juce::dsp::AudioBlock<float> blk (convBuf.getArrayOfWritePointers(), 2, (size_t) n);
        conv.process (juce::dsp::ProcessContextReplacing<float> (blk));
        for (int i = 0; i < n; ++i) { oL[i] = cl[i]; oR[i] = cr[i]; }
    }
    double sr = 48000; int maxB = 512;
    Plate plate; Fdn fdnA, fdnB; Spring spring; Delay dly; Reverse rev; Multitap mt; Shifter sh[2]; Unmask um; Line conPre[2];
    juce::dsp::Convolution conv; juce::AudioBuffer<float> convBuf;
    std::array<std::vector<float>, 4> buf; std::array<float, kChunk> shFb[2] {};
    float lo[2] {}, env = 0, duckG = 1, shEnv = 0; LP1 hi[2], tl[2];
};
}
