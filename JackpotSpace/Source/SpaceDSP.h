#pragma once
// JACKPOT SPACE: rack di riverberi e delay. Motori:
//  PLATE   lastra stile EMT 140 / Valhalla Plate: serbatoio a "8" di Dattorro, modulato, smorzamento in coda
//  HALL    sala stile Lexicon 224: rete di 8 linee (FDN), decadimento separato per bassi e alti, chorus nelle code
//  ROOM    stanza: stessa rete piu' piccola con riflessioni iniziali forti
//  SPRING  molla stile Fender / IK: catene di allpass dispersivi (il "cio-ing"), 1-3 molle, drip
//  TAPE    delay a nastro: wow e flutter, saturazione e filtri nel giro di ritorno, ping-pong, a tempo
//  DIGITAL delay digitale pulito con modulazione, filtri, ping-pong e sfasamento stereo
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <vector>
#include <cmath>

namespace jsp
{
enum Type { OFF = 0, PLATE, HALL, ROOM, SPRING, TAPE, DIGITAL, kNumTypes };
constexpr int kP = 8;   // parametri per motore
struct EngInfo { const char* name; const char* group; const char* desc; int n; const char* pn[kP]; float def[kP]; int main; };
inline const EngInfo& info (int t)
{
    static const EngInfo I[kNumTypes] = {
        { "VUOTO", "-", "Slot vuoto.", 0, {}, {}, 0 },
        { "PLATE", "RIVERBERI", "Lastra stile EMT 140 e Valhalla Plate: densa subito, brillante, coda liscia. Voce, rullante, synth.", 8,
          { "DECAY", "PRE-DELAY", "SIZE", "BRILLO", "MOD", "LO-CUT", "HI-CUT", "WIDTH" }, { 0.40f, 0.08f, 0.55f, 0.60f, 0.30f, 0.30f, 0.80f, 0.67f }, 0 },
        { "HALL 224", "RIVERBERI", "Sala stile Lexicon 224: profonda, con il chorus nelle code e i bassi che durano diversamente dagli alti. Pad, archi, voci lente.", 8,
          { "DECAY", "PRE-DELAY", "SIZE", "BASSI x", "ALTI x", "CHORUS", "DIFFUSIONE", "HI-CUT" }, { 0.45f, 0.10f, 0.60f, 0.50f, 0.45f, 0.35f, 0.70f, 0.75f }, 0 },
        { "ROOM", "RIVERBERI", "Stanza: riflessioni vicine e coda corta, mette lo strumento in un posto vero. Batteria, chitarre, doppie.", 8,
          { "DECAY", "PRE-DELAY", "SIZE", "EARLY", "BRILLO", "MOD", "LO-CUT", "HI-CUT" }, { 0.40f, 0.02f, 0.45f, 0.60f, 0.55f, 0.15f, 0.25f, 0.75f }, 0 },
        { "SPRING", "RIVERBERI", "Molla da amplificatore e da dub: il suono metallico che rimbalza, sgocciola sui colpi. Chitarre, dub, lo-fi.", 8,
          { "DECAY", "TENSIONE", "MOLLE", "DRIP", "TONO", "LO-CUT", "WIDTH", "PRE-DELAY" }, { 0.45f, 0.50f, 0.50f, 0.40f, 0.50f, 0.30f, 0.60f, 0.0f }, 0 },
        { "TAPE ECHO", "DELAY", "Eco a nastro: ripetizioni che si scaldano e si scuriscono, il nastro che oscilla. Voci, dub, chitarre.", 8,
          { "TEMPO", "FEEDBACK", "SYNC", "WOW", "TONO", "DRIVE", "PING-PONG", "LO-CUT" }, { 0.73f, 0.38f, 1.0f, 0.30f, 0.50f, 0.30f, 0.0f, 0.25f }, 0 },
        { "DIGITAL", "DELAY", "Delay digitale pulito: ripetizioni precise, modulazione per allargare, ping-pong. Per tutto.", 8,
          { "TEMPO", "FEEDBACK", "SYNC", "MOD", "HI-CUT", "LO-CUT", "PING-PONG", "OFFSET R" }, { 0.73f, 0.35f, 1.0f, 0.10f, 0.80f, 0.20f, 0.0f, 0.50f }, 0 } };
    return I[juce::jlimit (0, kNumTypes - 1, t)];
}

// divisioni a tempo
constexpr int kDivs = 12;
inline const char* divName (int i) { static const char* n[kDivs] = { "1/32", "1/16T", "1/16", "1/16D", "1/8T", "1/8", "1/8D", "1/4T", "1/4", "1/4D", "1/2", "1/1" }; return n[juce::jlimit (0, kDivs - 1, i)]; }
inline double divBeats (int i) { static const double b[kDivs] = { 0.125, 1.0 / 6, 0.25, 0.375, 1.0 / 3, 0.5, 0.75, 2.0 / 3, 1.0, 1.5, 2.0, 4.0 }; return b[juce::jlimit (0, kDivs - 1, i)]; }

// ---------------------------------------------------------------------------------------------------- mappe 0..1 -> valori
namespace map
{
inline float decay (int t, float n) { return t == ROOM ? 0.1f * std::pow (40.0f, n) : t == SPRING ? 0.4f * std::pow (15.0f, n) : t == HALL ? 0.3f * std::pow (100.0f, n) : 0.2f * std::pow (100.0f, n); }
inline float pre (float n) { return n * 250.0f; }                              // ms
inline float loCut (float n) { return 20.0f * std::pow (25.0f, n); }            // 20..500 Hz
inline float hiCut (float n) { return 1500.0f * std::pow (13.33f, n); }         // 1.5..20 kHz
inline float bright (float n) { return 1000.0f * std::pow (16.0f, n); }         // smorzamento in coda 1..16 kHz
inline float delayMs (float n) { return 20.0f * std::pow (100.0f, n); }         // 20..2000 ms
inline int div (float n) { return juce::jlimit (0, kDivs - 1, (int) std::round (n * (kDivs - 1))); }
inline float bassX (float n) { return 0.5f * std::pow (4.0f, n); }               // 0.5x .. 2x
inline float trebX (float n) { return 0.1f + 0.9f * n; }
inline int springs (float n) { return 1 + juce::jlimit (0, 2, (int) std::round (n * 2)); }
}
inline juce::String fmtS (float s) { return s < 1 ? juce::String (juce::roundToInt (s * 1000)) + " ms" : juce::String (s, s < 10 ? 2 : 1) + " s"; }
inline juce::String fmtHz (float f) { return f >= 1000 ? juce::String (f / 1000.0f, 1) + " kHz" : juce::String (juce::roundToInt (f)) + " Hz"; }
inline juce::String pct (float n) { return juce::String (juce::roundToInt (n * 100)) + "%"; }
inline juce::String paramText (int t, int i, float n, const float* all)
{
    switch (t)
    {
        case PLATE: case HALL: case ROOM: case SPRING:
            if (i == 0) return fmtS (map::decay (t, n));
            if ((i == 1 && t != SPRING) || (i == 7 && t == SPRING)) return juce::String (juce::roundToInt (map::pre (n))) + " ms";
            if (t == PLATE) { if (i == 2) return pct (0.5f + n); if (i == 3) return fmtHz (map::bright (n)); if (i == 5) return fmtHz (map::loCut (n)); if (i == 6) return fmtHz (map::hiCut (n)); if (i == 7) return pct (n * 1.5f); }
            if (t == HALL) { if (i == 2) return pct (0.5f + n); if (i == 3) return juce::String (map::bassX (n), 2) + "x"; if (i == 4) return juce::String (map::trebX (n), 2) + "x"; if (i == 7) return fmtHz (map::hiCut (n)); }
            if (t == ROOM) { if (i == 2) return pct (0.5f + n); if (i == 4) return fmtHz (map::bright (n)); if (i == 6) return fmtHz (map::loCut (n)); if (i == 7) return fmtHz (map::hiCut (n)); }
            if (t == SPRING) { if (i == 2) return juce::String (map::springs (n)); if (i == 4) return fmtHz (2000.0f * std::pow (4.0f, n)); if (i == 5) return fmtHz (map::loCut (n)); }
            return pct (n);
        case TAPE: case DIGITAL:
            if (i == 0) return all != nullptr && all[2] > 0.5f ? juce::String (divName (map::div (n))) : juce::String (juce::roundToInt (map::delayMs (n))) + " ms";
            if (i == 1) return pct (n * 1.1f);
            if (i == 2) return n > 0.5f ? "A TEMPO" : "LIBERO";
            if (t == TAPE) { if (i == 4) return fmtHz (1500.0f * std::pow (6.0f, n)); if (i == 7) return fmtHz (map::loCut (n)); }
            if (t == DIGITAL) { if (i == 4) return fmtHz (map::hiCut (n)); if (i == 5) return fmtHz (map::loCut (n)); if (i == 7) return (n > 0.5f ? "+" : "") + juce::String (juce::roundToInt ((n - 0.5f) * 40)) + " ms"; }
            return pct (n);
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
        const float rt = map::decay (PLATE, p[0]);
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
    void clear() { for (auto& l : d) l.clear(); for (auto& a : diff) a.l.clear(); pre.clear(); er.clear(); for (int i = 0; i < N; ++i) { lp[i] = {}; ph[i] = i * 0.7f; } }
    // room: true = stanza (linee corte e riflessioni forti)
    void process (const float* inL, const float* inR, float* oL, float* oR, int n, int type, const float* p, bool freeze)
    {
        const bool room = type == ROOM;
        static const float base[N] = { 29.7f, 37.1f, 41.1f, 43.7f, 53.3f, 59.3f, 67.9f, 79.3f };   // ms, primi fra loro
        const float sz = (0.5f + p[2]) * (room ? 0.32f : 1.0f);
        const float rt = map::decay (type, p[0]);
        const float rtLo = room ? rt : rt * map::bassX (p[3]), rtHi = room ? rt * (0.15f + 0.85f * p[4]) : rt * map::trebX (p[4]);
        const float xo = coef (room ? 900.0f : 1200.0f, sr);
        const float modD = (room ? p[5] : p[5]) * 0.0009f * (float) sr;   // fino a ~1 ms di escursione: il chorus del 224
        const float diffG = room ? 0.6f : 0.35f + 0.4f * p[6];
        const int preN = (int) (map::pre (p[1]) * 0.001 * sr);
        float len[N], gLo[N], gHi[N];
        for (int i = 0; i < N; ++i)
        {
            len[i] = base[i] * sz * 0.001f * (float) sr;
            gLo[i] = freeze ? 1.0f : juce::jmin (0.9998f, rtGain (len[i], rtLo, sr)); gHi[i] = freeze ? 1.0f : juce::jmin (0.9998f, rtGain (len[i], rtHi, sr));
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
                const float l = lp[i].run (o[i], xo); o[i] = l * gLo[i] + (o[i] - l) * gHi[i];   // bassi e alti decadono diversi
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
    double sr = 48000; Line d[N], pre, er; AP diff[4]; LP1 lp[N]; float ph[N] {};
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

// ---------------------------------------------------------------------------------------------------- DELAY (nastro e digitale)
class Delay
{
public:
    void prepare (double s) { sr = s; for (auto& l : ln) l.prepare ((int) (4.2 * sr) + 64); clear(); }
    void clear() { for (auto& l : ln) l.clear(); for (int c = 0; c < 2; ++c) { lp[c] = {}; hp[c] = 0; } tS[0] = tS[1] = -1; ph = ph2 = 0; }
    void process (const float* inL, const float* inR, float* oL, float* oR, int n, int type, const float* p, double bpm)
    {
        const bool tape = type == TAPE, sync = p[2] > 0.5f;
        float tMs = sync ? (float) (divBeats (map::div (p[0])) * 60000.0 / juce::jmax (30.0, bpm)) : map::delayMs (p[0]);
        tMs = juce::jlimit (5.0f, 4000.0f, tMs);
        const float off = tape ? 0.0f : (p[7] - 0.5f) * 40.0f;
        const float tgt[2] = { tMs * 0.001f * (float) sr, juce::jlimit (5.0f, 4000.0f, tMs + off) * 0.001f * (float) sr };
        const float fbk = p[1] * 1.1f, pp = p[6];
        const float lc = coef (tape ? 1500.0f * std::pow (6.0f, p[4]) : map::hiCut (p[4]), sr), hc = coef (map::loCut (tape ? p[7] : p[5]), sr);
        const float drive = tape ? 1.0f + p[5] * 4.0f : 1.0f;
        const float wow = tape ? p[3] : 0.0f, mod = tape ? 0.0f : p[3];
        for (int c = 0; c < 2; ++c) if (tS[c] < 0) tS[c] = tgt[c];
        // il nastro scivola verso il nuovo tempo (pitch che si piega), il digitale si sposta piu' rapido ma senza click
        const float glide = tape ? 0.00015f : 0.002f;
        const float d1 = (float) (juce::MathConstants<double>::twoPi * 0.6 / sr), d2 = (float) (juce::MathConstants<double>::twoPi * 6.1 / sr);
        for (int i = 0; i < n; ++i)
        {
            ph += d1; ph2 += d2; if (ph > 6.2832f) ph -= 6.2832f; if (ph2 > 6.2832f) ph2 -= 6.2832f;
            const float wobble = (wow * (0.0025f * std::sin (ph) + 0.0004f * std::sin (ph2)) + mod * 0.0015f * std::sin (ph * 1.7f)) * (float) sr;
            float rd[2];
            for (int c = 0; c < 2; ++c)
            {
                tS[c] += (tgt[c] - tS[c]) * glide;
                rd[c] = ln[c].frac (juce::jmax (2.0f, tS[c] + (c ? -wobble : wobble)));
            }
            const float inl = inL[i], inr = inR[i];
            float w[2];
            for (int c = 0; c < 2; ++c)
            {
                float v = lp[c].run (rd[c], lc); hp[c] += hc * (v - hp[c]); v -= hp[c];
                if (tape) v = std::tanh (v * drive) / drive * (1.0f + 0.15f * (drive - 1.0f) / 4.0f);
                w[c] = v;
            }
            // ping-pong: l'ingresso entra da un lato solo e le ripetizioni si incrociano
            const float inA = pp > 0.01f ? 0.5f * (inl + inr) : 0.0f;
            const float xl = (1 - pp) * inl + pp * inA, xr = (1 - pp) * inr;
            ln[0].push (xl + fbk * ((1 - pp) * w[0] + pp * w[1]));
            ln[1].push (xr + fbk * ((1 - pp) * w[1] + pp * w[0]));
            oL[i] = rd[0]; oR[i] = rd[1];
        }
    }
private:
    double sr = 48000; Line ln[2]; LP1 lp[2]; float hp[2] {}, tS[2] { -1, -1 }, ph = 0, ph2 = 0;
};

// ---------------------------------------------------------------------------------------------------- uno slot del rack
class Slot
{
public:
    void prepare (double s, int maxBlock) { sr = s; plate.prepare (s); fdn.prepare (s); spring.prepare (s); dly.prepare (s); wl.assign ((size_t) maxBlock, 0); wr = wl; type = -1; for (int c = 0; c < 2; ++c) { lo[c] = 0; hi[c] = {}; } }
    // scrive solo la parte "bagnata" in oL/oR
    void process (const float* inL, const float* inR, float* oL, float* oR, int n, int t, const float* p, double bpm, bool freeze)
    {
        if (t != type) { plate.clear(); fdn.clear(); spring.clear(); dly.clear(); type = t; }
        switch (t)
        {
            case PLATE: plate.process (inL, inR, oL, oR, n, p, freeze); break;
            case HALL: case ROOM: fdn.process (inL, inR, oL, oR, n, t, p, freeze); break;
            case SPRING: spring.process (inL, inR, oL, oR, n, p, freeze); break;
            case TAPE: case DIGITAL: dly.process (inL, inR, oL, oR, n, t, p, bpm); break;
            default: std::fill (oL, oL + n, 0.0f); std::fill (oR, oR + n, 0.0f); return;
        }
        // filtri d'uscita del riverbero (dove il motore li prevede)
        float lcF = 0, hcF = 0; float wid = 1;
        if (t == PLATE) { lcF = map::loCut (p[5]); hcF = map::hiCut (p[6]); wid = p[7] * 1.5f; }
        if (t == HALL) { hcF = map::hiCut (p[7]); }
        if (t == ROOM) { lcF = map::loCut (p[6]); hcF = map::hiCut (p[7]); }
        const float a = lcF > 0 ? coef (lcF, sr) : 0.0f, b = hcF > 0 ? coef (hcF, sr) : 0.0f;
        for (int i = 0; i < n; ++i)
        {
            float x[2] = { oL[i], oR[i] };
            for (int c = 0; c < 2; ++c) { if (b > 0) x[c] = hi[c].run (x[c], b); if (a > 0) { lo[c] += a * (x[c] - lo[c]); x[c] -= lo[c]; } }
            if (wid != 1) { const float m = 0.5f * (x[0] + x[1]), s = 0.5f * (x[0] - x[1]) * wid; x[0] = m + s; x[1] = m - s; }
            oL[i] = x[0]; oR[i] = x[1];
        }
    }
    int type = -1;
private:
    double sr = 48000; Plate plate; Fdn fdn; Spring spring; Delay dly; std::vector<float> wl, wr; float lo[2] {}; LP1 hi[2];
};
}
