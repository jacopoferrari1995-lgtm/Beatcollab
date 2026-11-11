#pragma once
// Jackpot Mixology: effetti nuovi (riverberi, delay, de-esser, gate, transienti, wider, utility)
#include <juce_dsp/juce_dsp.h>
#include "JackpotKit.h"

namespace jmx
{
inline float db2g (float d) { return std::pow (10.0f, d * 0.05f); }
inline float g2db (float g) { return 20.0f * std::log10 (juce::jmax (1.0e-9f, g)); }
struct OP { float a = 0, z = 0; void lp (double sr, double f) { a = (float) std::exp (-juce::MathConstants<double>::twoPi * juce::jlimit (1.0, sr * 0.49, f) / sr); } inline float run (float x) { z = x + a * (z - x); return z; } void reset() { z = 0; } };
struct Line
{
    std::vector<float> b; int w = 0, mask = 0;
    void prepare (int maxLen) { int n = 1; while (n < maxLen + 4) n <<= 1; b.assign ((size_t) n, 0.0f); mask = n - 1; w = 0; }
    inline void push (float x) { b[(size_t) w] = x; w = (w + 1) & mask; }
    inline float tap (int d) const { return b[(size_t) ((w - 1 - d) & mask)]; }
    inline float frac (float d) const { const int i = (int) d; const float f = d - (float) i; const float a = tap (i), c = tap (i + 1); return a + (c - a) * f; }
    void clear() { std::fill (b.begin(), b.end(), 0.0f); }
};
struct AP { Line l; int d = 1; float g = 0.6f; inline float run (float x) { const float y = l.tap (d - 1); const float v = x + g * y; l.push (v); return y - g * v; } };
// divisioni a tempo (in battiti)
inline const juce::StringArray& divNames() { static const juce::StringArray n { "1/32", "1/16T", "1/16", "1/16D", "1/8T", "1/8", "1/8D", "1/4T", "1/4", "1/4D", "1/2", "1/2D", "1/1" }; return n; }
inline double divBeats (int i) { static const double b[] = { 0.125, 1.0 / 6, 0.25, 0.375, 1.0 / 3, 0.5, 0.75, 2.0 / 3, 1, 1.5, 2, 3, 4 }; return b[juce::jlimit (0, 12, i)]; }

// ======================================================================= riverbero a rete di ritardi (FDN 8 linee)
struct RevSettings
{
    float preMs = 10, decay = 1.8f, size = 0.6f, damp = 6000, lowMul = 1.0f, diff = 0.7f, mod = 0.3f, width = 1, loCut = 120, hiCut = 12000, er = 0.3f, duck = 0, shimmer = 0;
    int kind = 0;   // 0 plate, 1 hall, 2 room, 3 chamber, 4 grande (shimmer)
};
class Reverb
{
public:
    void prepare (double s)
    {
        sr = s;
        pre[0].prepare ((int) (sr * 0.3)); pre[1].prepare ((int) (sr * 0.3));
        for (auto& l : fdn) l.prepare ((int) (sr * 0.35));
        for (int c = 0; c < 2; ++c) for (int k = 0; k < 4; ++k) { inAp[c][k].l.prepare ((int) (sr * 0.03)); }
        er[0].prepare ((int) (sr * 0.12)); er[1].prepare ((int) (sr * 0.12));
        shim.prepare ((int) (sr * 0.2));
        for (auto& f : dmp) f.reset(); for (auto& f : lowS) f.reset(); for (auto& c : hp) c.reset(); for (auto& c : lp) c.reset();
        lastKey = -1; env = 0; ph = 0; shPh = 0;
    }
    void process (float* L, float* R, int n, const RevSettings& S)
    {
        configure (S);
        const float g8 = 0.25f;  // matrice di Householder: I - 2/N
        const float aDuck = 1.0f - std::exp (-1.0f / (0.25f * (float) sr));
        for (int i = 0; i < n; ++i)
        {
            const float inL = L[i], inR = R[i];
            pre[0].push (inL); pre[1].push (inR);
            float xl = pre[0].tap (preD), xr = pre[1].tap (preD);
            for (int k = 0; k < 4; ++k) { xl = inAp[0][k].run (xl); xr = inAp[1][k].run (xr); }
            // riflessioni primarie (stanza, chamber)
            er[0].push (inL); er[1].push (inR);
            float el = 0, erv = 0; for (int k = 0; k < 6; ++k) { el += er[0].tap (erT[k]) * erG[k]; erv += er[1].tap (erT[(k + 3) % 6]) * erG[k]; }
            // rete
            float o[8], s = 0;
            ph += modHz / sr; if (ph > 1) ph -= 1;
            for (int k = 0; k < 8; ++k)
            {
                const float m = modDepth * std::sin ((float) juce::MathConstants<double>::twoPi * (float) (ph + k * 0.137));
                o[k] = fdn[k].frac (dl[k] + m);
                o[k] = dmp[k].run (o[k]);                                  // gli alti muoiono prima
                const float lo = lowS[k].run (o[k]); o[k] = (o[k] - lo) + lo * lowMulG;   // i bassi durano LOW volte
                s += o[k];
            }
            s *= g8;
            float shimIn = 0;
            if (S.shimmer > 0.001f)
            {   // un'ottava sopra nel ritorno (due prese a dente di sega incrociate)
                shim.push (0.5f * (o[0] + o[1]));
                const float N2 = (float) (sr * 0.08); shPh += 1.0 / N2; if (shPh >= 1) shPh -= 1;
                const float p1 = (float) shPh, p2 = std::fmod (p1 + 0.5f, 1.0f);
                const float d1 = (1 - p1) * N2 + 2, d2 = (1 - p2) * N2 + 2, w1 = std::sin (juce::MathConstants<float>::pi * p1), w2 = std::sin (juce::MathConstants<float>::pi * p2);
                shimIn = (shim.frac (d1) * w1 * w1 + shim.frac (d2) * w2 * w2) * S.shimmer * 0.6f;
            }
            for (int k = 0; k < 8; ++k)
            {
                const float in = (k & 1 ? xr : xl) * 0.5f + (k < 2 ? shimIn : 0.0f);
                fdn[k].push (std::tanh ((o[k] - s * 2.0f) * fbG[k] + in));
            }
            float wl = o[0] + o[2] + o[4] + o[6] * 0.7f - o[5] * 0.4f, wr = o[1] + o[3] + o[5] + o[7] * 0.7f - o[4] * 0.4f;
            wl = wl * 0.42f + el * S.er; wr = wr * 0.42f + erv * S.er;
            // uscita: taglio basso e alto, larghezza, ducking
            wl = lpF[0].run (wl - hpF[0].run (wl)); wr = lpF[1].run (wr - hpF[1].run (wr));
            const float m = 0.5f * (wl + wr), sd = 0.5f * (wl - wr) * S.width;
            wl = m + sd; wr = m - sd;
            if (S.duck > 0.001f) { const float a = std::abs (inL) + std::abs (inR); env += (a - env) * (a > env ? 0.02f : aDuck); const float d = 1.0f / (1.0f + S.duck * 8.0f * env); wl *= d; wr *= d; }
            L[i] = wl; R[i] = wr;
        }
    }
private:
    void configure (const RevSettings& S)
    {
        const int key = juce::roundToInt (S.size * 1000) * 7 + S.kind * 100003 + juce::roundToInt (S.decay * 100) * 31 + juce::roundToInt (S.damp) * 3 + juce::roundToInt (S.diff * 100) * 11 + juce::roundToInt (S.preMs * 10) * 13
                        + juce::roundToInt (S.loCut) * 17 + juce::roundToInt (S.hiCut) * 19 + juce::roundToInt (S.lowMul * 100) * 23 + juce::roundToInt (S.mod * 100) * 29;
        if (key == lastKey) return; lastKey = key;
        static const float base[8] = { 0.0297f, 0.0371f, 0.0411f, 0.0437f, 0.0531f, 0.0593f, 0.0671f, 0.0773f };
        const float sc = S.kind == 2 ? 0.25f + 0.55f * S.size : S.kind == 4 ? 1.0f + 2.5f * S.size : 0.45f + 1.4f * S.size;
        for (int k = 0; k < 8; ++k)
        {
            dl[k] = (float) (base[k] * sc * sr);
            fbG[k] = std::pow (10.0f, -3.0f * (dl[k] / (float) sr) / juce::jmax (0.1f, S.decay));
            dmp[k].lp (sr, S.damp); lowS[k].lp (sr, 250);
        }
        lowMulG = juce::jlimit (0.5f, 1.25f, 1.0f + (S.lowMul - 1.0f) * 0.12f);
        static const float ad[4] = { 0.0047f, 0.0036f, 0.0127f, 0.0093f };
        const float diffScale = S.kind == 0 ? 1.4f : S.kind == 1 ? 1.0f : 0.6f;
        for (int c = 0; c < 2; ++c) for (int k = 0; k < 4; ++k) { inAp[c][k].d = juce::jmax (2, (int) (ad[k] * diffScale * (1 + 0.07f * c) * sr)); inAp[c][k].g = 0.35f + 0.4f * S.diff; }
        preD = juce::jlimit (0, (int) (sr * 0.29), (int) (S.preMs * 0.001 * sr));
        static const float ert[6] = { 0.0043f, 0.0089f, 0.0137f, 0.0211f, 0.0293f, 0.0371f };
        for (int k = 0; k < 6; ++k) { erT[k] = (int) (ert[k] * (0.4f + S.size) * sr) + preD; erG[k] = 0.5f * std::pow (0.72f, (float) k) * (k & 1 ? -1.0f : 1.0f); }
        modHz = 0.25 + S.mod * 0.9; modDepth = (float) (S.mod * 0.0018 * sr * (S.kind == 4 ? 2.5 : 1.0));
        for (int c = 0; c < 2; ++c) { hpF[c].lp (sr, S.loCut); lpF[c].lp (sr, S.hiCut); }
    }
    double sr = 44100, ph = 0, shPh = 0, modHz = 0.5; float modDepth = 0, lowMulG = 1, env = 0;
    Line pre[2], fdn[8], er[2], shim; AP inAp[2][4]; OP dmp[8], lowS[8], hp[2], lp[2], hpF[2], lpF[2];
    float dl[8] {}, fbG[8] {}; int preD = 0, erT[6] {}; float erG[6] {}; int lastKey = -1;
};

// ======================================================================= molla (spring): catena di allpass dispersivi + ritorno
struct SpringSettings { float decay = 2.0f, tension = 0.5f, tone = 0.5f, drip = 0.4f, loCut = 150, hiCut = 6000, width = 0.7f; };
class Spring
{
public:
    void prepare (double s)
    {
        sr = s;
        for (int c = 0; c < 2; ++c) { for (auto& a : disp[c]) a.l.prepare (64); loop[c].prepare ((int) (sr * 0.2)); hpF[c].reset(); lpF[c].reset(); dmp[c].reset(); }
        last = -1;
    }
    void process (float* L, float* R, int n, const SpringSettings& S)
    {
        const int key = juce::roundToInt (S.tension * 100) + juce::roundToInt (S.decay * 100) * 101 + juce::roundToInt (S.tone * 100) * 10007 + juce::roundToInt (S.loCut) * 7 + juce::roundToInt (S.hiCut) * 3;
        if (key != last)
        {
            last = key;
            for (int c = 0; c < 2; ++c)
            {
                for (int k = 0; k < kD; ++k) { disp[c][k].d = 1 + (k % 3); disp[c][k].g = 0.55f + 0.25f * S.drip + 0.01f * c; }
                ld[c] = (int) ((0.033 + 0.03 * S.tension + 0.004 * c) * sr);
                fb[c] = std::pow (10.0f, -3.0f * (ld[c] / (float) sr) / juce::jmax (0.2f, S.decay));
                hpF[c].lp (sr, S.loCut); lpF[c].lp (sr, S.hiCut); dmp[c].lp (sr, 2500 + 6000 * S.tone);
            }
        }
        for (int i = 0; i < n; ++i)
        {
            const float in[2] = { L[i], R[i] }; float out[2];
            for (int c = 0; c < 2; ++c)
            {
                float x = loop[c].tap (ld[c]) * fb[c] + 0.5f * (in[c] + in[1 - c] * 0.3f);
                for (int k = 0; k < kD; ++k) x = disp[c][k].run (x);   // dispersione: gli alti arrivano prima (il "boing")
                x = dmp[c].run (x);
                loop[c].push (std::tanh (x));
                float y = x - hpF[c].run (x); y = lpF[c].run (y);
                out[c] = y;
            }
            const float m = 0.5f * (out[0] + out[1]), sd = 0.5f * (out[0] - out[1]) * S.width;
            L[i] = (m + sd) * 0.8f; R[i] = (m - sd) * 0.8f;
        }
    }
private:
    static constexpr int kD = 48;
    double sr = 44100; AP disp[2][kD]; Line loop[2]; OP hpF[2], lpF[2], dmp[2]; int ld[2] {}; float fb[2] {}; int last = -1;
};

// ======================================================================= riverbero a convoluzione con risposte generate
inline const juce::StringArray& irNames() { static const juce::StringArray n { "STUDIO A", "VOCAL BOOTH", "LIVE ROOM", "CHAMBER", "PLATE EMT", "SALA CONCERTI", "CATTEDRALE", "PARCHEGGIO", "MOLLA", "TUO FILE" }; return n; }
inline juce::AudioBuffer<float> makeIr (int kind, double sr, float decayMul)
{
    struct K { float t60, preMs, hiDamp, density, erAmt, bright; };
    static const K k[] = { { 0.55f, 4, 6000, 1, 0.6f, 0.4f }, { 0.22f, 1, 8000, 1, 0.8f, 0.5f }, { 0.9f, 8, 7000, 1, 0.5f, 0.6f }, { 1.4f, 12, 5000, 0.9f, 0.4f, 0.4f },
                           { 2.2f, 0, 9000, 1, 0.0f, 0.9f }, { 2.6f, 25, 4500, 0.8f, 0.3f, 0.35f }, { 6.0f, 45, 3500, 0.6f, 0.2f, 0.3f }, { 2.0f, 18, 7000, 0.7f, 0.7f, 0.7f }, { 1.8f, 2, 4000, 0.5f, 0.0f, 0.5f } };
    const auto& P = k[juce::jlimit (0, 8, kind)];
    const float t60 = P.t60 * decayMul;
    const int len = juce::jlimit (256, (int) (sr * 10), (int) (sr * (t60 * 1.1f + P.preMs * 0.001f)));
    juce::AudioBuffer<float> ir (2, len); ir.clear();
    juce::Random r (1234 + kind);
    const int pre = (int) (P.preMs * 0.001 * sr);
    for (int c = 0; c < 2; ++c)
    {
        float lp = 0; float* d = ir.getWritePointer (c);
        for (int i = pre; i < len; ++i)
        {
            const float t = (float) (i - pre) / (float) sr;
            const float env = std::pow (10.0f, -3.0f * t / t60);
            // piu scuro col tempo: il filtro si chiude man mano
            const float fc = P.hiDamp * (1.0f + 2.0f * P.bright) / (1.0f + t * 3.0f);
            const float a = std::exp (-juce::MathConstants<float>::twoPi * fc / (float) sr);
            const float nz = r.nextFloat() < P.density ? r.nextFloat() * 2 - 1 : 0.0f;
            lp = nz + a * (lp - nz);
            d[i] = lp * env;
            if (kind == 8) d[i] *= 0.6f + 0.4f * std::sin (t * 2 * 3.14159f * (9.0f + c));   // molla: battimento metallico
        }
        // riflessioni primarie discrete
        for (int e = 0; e < 8 && P.erAmt > 0; ++e) { const int at = pre + (int) ((0.003 + e * 0.0047 + c * 0.0011) * sr * (0.5 + P.t60 * 0.3)); if (at < len) d[at] += P.erAmt * std::pow (0.7f, (float) e) * (e & 1 ? -1.0f : 1.0f); }
    }
    // normalizza l'energia
    double e = 0; for (int c = 0; c < 2; ++c) for (int i = 0; i < len; ++i) e += (double) ir.getSample (c, i) * ir.getSample (c, i);
    ir.applyGain ((float) (0.35 / std::sqrt (juce::jmax (1e-9, e / 2))));
    return ir;
}

// ======================================================================= delay analogici
struct DlySettings
{
    double beats = 0.5; float ms = 375; bool sync = true; float fb = 0.35f, loCut = 120, hiCut = 6000, drive = 0.2f, mod = 0.2f, width = 1, duck = 0, spread = 0;
    int kind = 0;   // 0 nastro, 1 BBD, 2 digitale, 3 ping-pong, 4 dub, 5 slap
};
class Delay
{
public:
    void prepare (double s) { sr = s; for (auto& l : ln) l.prepare ((int) (sr * 4.2)); for (auto& f : lp) f.reset(); for (auto& f : hp) f.reset(); tSm = -1; env = 0; ph = 0; ph2 = 0; nz = 0; }
    void process (float* L, float* R, int n, const DlySettings& S, double bpm)
    {
        double t = S.sync ? S.beats * 60.0 / juce::jmax (30.0, bpm) : S.ms * 0.001;
        t = juce::jlimit (0.002, 4.0, t);
        const float target = (float) (t * sr);
        if (tSm < 0) tSm = target;
        // nastro: wow e flutter; BBD: chorus lento; digitale: fermo
        const float wowD = S.kind == 0 ? 0.0035f : S.kind == 1 ? 0.0025f : S.kind == 4 ? 0.003f : 0.0f;
        const float hiC = S.kind == 1 ? juce::jmin (S.hiCut, 2500.0f + 9000.0f / (1.0f + (float) t * 4.0f)) : S.kind == 2 ? S.hiCut : S.hiCut;
        for (int c = 0; c < 2; ++c) { lp[c].lp (sr, hiC); hp[c].lp (sr, S.loCut); }
        const float aDuck = 1.0f - std::exp (-1.0f / (0.3f * (float) sr));
        const bool ping = S.kind == 3;
        for (int i = 0; i < n; ++i)
        {
            tSm += (target - tSm) * (S.kind == 2 ? 0.002f : 0.00015f);   // cambiando tempo il nastro "scivola"
            ph += (0.6 + S.mod * 1.4) / sr; if (ph > 1) ph -= 1; ph2 += 5.7 / sr; if (ph2 > 1) ph2 -= 1;
            const float m = (float) ((std::sin (ph * juce::MathConstants<double>::twoPi) * wowD + std::sin (ph2 * juce::MathConstants<double>::twoPi) * wowD * 0.15) * S.mod * sr);
            float y[2];
            for (int c = 0; c < 2; ++c)
            {
                const float d = juce::jlimit (2.0f, (float) (sr * 4.1), tSm * (c == 1 && S.spread > 0 ? 1.0f + S.spread * 0.25f : 1.0f) + m * (c ? -1.0f : 1.0f));
                y[c] = ln[c].frac (d);
            }
            const float inL = L[i], inR = R[i];
            float fl, fr;
            if (ping) { fl = 0.5f * (inL + inR) + y[1] * S.fb; fr = y[0] * S.fb; }
            else { fl = inL + y[0] * S.fb; fr = inR + y[1] * S.fb; }
            // il ritorno passa nei tagli e nella saturazione del modello
            auto colour = [&] (float x, int c)
            {
                x = lp[c].run (x); x = x - hp[c].run (x);
                const float dr = 1.0f + S.drive * 4.0f;
                if (S.kind == 0 || S.kind == 4) x = std::tanh (x * dr) / dr * (1.0f + 0.3f * S.drive);
                else if (S.kind == 1) { x = juce::jlimit (-1.0f, 1.0f, x * dr) / dr; nz = nz * 0.97f + (rnd.nextFloat() - 0.5f) * 0.0004f * S.drive; x += nz; }
                else if (S.kind == 5) x = std::tanh (x * dr) / dr;
                return x;
            };
            ln[0].push (colour (fl, 0)); ln[1].push (colour (fr, 1));
            float wl = y[0], wr = y[1];
            const float mm = 0.5f * (wl + wr), sd = 0.5f * (wl - wr) * S.width; wl = mm + sd; wr = mm - sd;
            if (S.duck > 0.001f) { const float a = std::abs (inL) + std::abs (inR); env += (a - env) * (a > env ? 0.01f : aDuck); const float g = 1.0f / (1.0f + S.duck * 10.0f * env); wl *= g; wr *= g; }
            L[i] = wl; R[i] = wr;
        }
    }
private:
    double sr = 44100, ph = 0, ph2 = 0; float tSm = -1, env = 0, nz = 0; Line ln[2]; OP lp[2], hp[2]; juce::Random rnd;
};

// ======================================================================= de-esser
struct DsSettings { float freq = 6000, thr = -24, range = 8, hiLim = 16000; bool wide = false, listen = false, relative = true; int look = 0; };
class DeEsser
{
public:
    void prepare (double s) { sr = s; for (auto& c : st) c = {}; env = 0; gr = 0; last = -1; for (int c = 0; c < 2; ++c) dl[c].prepare ((int) (sr * 0.006)); fullEnv = 0; }
    void process (float* L, float* R, int n, const DsSettings& S)
    {
        const int key = juce::roundToInt (S.freq) * 3 + juce::roundToInt (S.hiLim) * 7;
        if (key != last) { last = key; for (auto& c : st) { c.h1.hp (sr, S.freq, 0.7071); c.h2.hp (sr, S.freq, 0.7071); c.l1.lp (sr, juce::jmin ((double) S.hiLim, sr * 0.45), 0.7071); c.x1.hp (sr, S.freq, 0.7071); c.x2.hp (sr, S.freq, 0.7071); } }
        const int la = S.look ? (int) (0.003 * sr) : 0;
        const float aA = 1.0f - std::exp (-1.0f / (0.0005f * (float) sr)), aR = 1.0f - std::exp (-1.0f / (0.06f * (float) sr)), aF = 1.0f - std::exp (-1.0f / (0.02f * (float) sr));
        float grMax = 0;
        for (int i = 0; i < n; ++i)
        {
            float det = 0, full = 0;
            for (int c = 0; c < 2; ++c) { const float x = c ? R[i] : L[i]; const float h = st[c].l1.p (st[c].h2.p (st[c].h1.p (x))); det = juce::jmax (det, std::abs (h)); full = juce::jmax (full, std::abs (x)); }
            env += (det - env) * (det > env ? aA : aR); fullEnv += (full - fullEnv) * aF;
            // RELATIVO: guarda quanto le sibilanti sporgono sul resto della voce (indipendente dal volume)
            const float lev = S.relative ? g2db (env) - g2db (fullEnv) + 12.0f + S.thr * 0.0f : g2db (env);
            const float th = S.relative ? 12.0f + S.thr * 0.5f : S.thr;
            const float over = lev - th;
            const float target = over > 0 ? -juce::jmin (S.range, over * 0.8f) : 0.0f;
            gr += (target - gr) * (target < gr ? 0.3f : 0.002f);
            grMax = juce::jmin (grMax, gr);
            const float g = db2g (gr);
            for (int c = 0; c < 2; ++c)
            {
                float& x = c ? R[i] : L[i]; dl[c].push (x); const float xd = la ? dl[c].tap (la) : x;
                if (S.wide) { const float hh = st[c].x2.p (st[c].x1.p (xd)); x = S.listen ? hh * (1 - g) : xd * g; }
                else
                {   // split: abbassa solo la parte alta
                    const float hh = st[c].x2.p (st[c].x1.p (xd));
                    x = S.listen ? hh * (1 - g) : xd - hh * (1 - g);
                }
            }
        }
        grUi = grMax;
    }
    int latency (const DsSettings& S) const { return S.look ? (int) (0.003 * sr) : 0; }
    float grUi = 0;
private:
    struct Ch { jkit::Bq h1, h2, l1, x1, x2; } st[2]; Line dl[2];
    double sr = 44100; float env = 0, fullEnv = 0, gr = 0; int last = -1;
};

// ======================================================================= gate / expander
struct GateSettings { float thr = -40, range = 40, attMs = 1, holdMs = 20, relMs = 120, hpHz = 20, lpHz = 20000; bool listen = false; };
class Gate
{
public:
    void prepare (double s) { sr = s; g = 0; hold = 0; for (auto& c : f) c = {}; last = -1; open = false; }
    void process (float* L, float* R, int n, const float* sL, const float* sR, const GateSettings& S)
    {
        const int key = juce::roundToInt (S.hpHz) * 3 + juce::roundToInt (S.lpHz) * 7;
        if (key != last) { last = key; for (auto& c : f) { c.h.hp (sr, S.hpHz, 0.7071); c.l.lp (sr, juce::jmin ((double) S.lpHz, sr * 0.45), 0.7071); } }
        const float aA = 1.0f - std::exp (-1.0f / (0.001f * juce::jmax (0.05f, S.attMs) * (float) sr)), aR = 1.0f - std::exp (-1.0f / (0.001f * S.relMs * (float) sr));
        const float rangeG = db2g (-S.range); const float thOpen = db2g (S.thr), thClose = db2g (S.thr - 4.0f);   // isteresi: si chiude 4 dB sotto
        const int holdN = (int) (S.holdMs * 0.001 * sr); float minG = 1;
        for (int i = 0; i < n; ++i)
        {
            const float dl = f[0].l.p (f[0].h.p (sL[i])), dr = f[1].l.p (f[1].h.p (sR[i]));
            const float a = juce::jmax (std::abs (dl), std::abs (dr));
            if (a > thOpen) { open = true; hold = holdN; } else if (a < thClose) { if (hold > 0) --hold; else open = false; }
            const float t = open ? 1.0f : rangeG;
            g += (t - g) * (t > g ? aA : aR);
            minG = juce::jmin (minG, g);
            if (S.listen) { L[i] = dl; R[i] = dr; } else { L[i] *= g; R[i] *= g; }
        }
        grUi = g2db (minG);
    }
    float grUi = 0;
private:
    struct C { jkit::Bq h, l; } f[2]; double sr = 44100; float g = 0; int hold = 0, last = -1; bool open = false;
};

// ======================================================================= transienti
struct TrSettings { float attack = 0, sustain = 0, speed = 0.5f, outDb = 0; bool clip = false; };
class Transient
{
public:
    void prepare (double s) { sr = s; ef = es = 0; ev = 0; }
    void process (float* L, float* R, int n, const TrSettings& S)
    {
        const float fA = 1.0f - std::exp (-1.0f / (0.0007f * (float) sr)), sA = 1.0f - std::exp (-1.0f / ((0.006f + 0.04f * S.speed) * (float) sr));
        const float rel = 1.0f - std::exp (-1.0f / ((0.05f + 0.35f * S.speed) * (float) sr));
        const float og = db2g (S.outDb); float act = 0;
        for (int i = 0; i < n; ++i)
        {
            const float a = juce::jmax (std::abs (L[i]), std::abs (R[i]));
            ef += (a - ef) * (a > ef ? fA : rel * 4); es += (a - es) * (a > es ? sA : rel);
            ev += (a - ev) * (a > ev ? fA : rel * 0.25f);
            const float att = juce::jlimit (0.0f, 1.0f, (g2db (ef + 1e-6f) - g2db (es + 1e-6f)) / 12.0f);     // c'e un colpo
            const float sus = juce::jlimit (0.0f, 1.0f, (g2db (ev + 1e-6f) - g2db (ef + 1e-6f) + 6) / 12.0f);  // sta suonando la coda
            const float gdb = att * S.attack * 0.15f + sus * S.sustain * 0.15f;
            act = juce::jmax (act, std::abs (gdb));
            const float g = db2g (gdb) * og;
            L[i] *= g; R[i] *= g;
            if (S.clip) { L[i] = std::tanh (L[i]); R[i] = std::tanh (R[i]); }
        }
        actUi = act;
    }
    float actUi = 0;
private:
    double sr = 44100; float ef = 0, es = 0, ev = 0;
};

// ======================================================================= wider
struct WideSettings { float width = 1, fromHz = 300, outDb = 0; bool monoBass = false; float haas = 0; };
class Wider
{
public:
    void prepare (double s) { sr = s; for (auto& f : xo) f.reset(); last = -1; hd.prepare ((int) (sr * 0.03)); }
    void process (float* L, float* R, int n, const WideSettings& S)
    {
        if (std::abs (S.fromHz - last) > 0.5f) { last = S.fromHz; for (int k = 0; k < 4; ++k) { if (k < 2) xo[k].lp (sr, juce::jmax (20.0f, S.fromHz), 0.7071); else xo[k].lp (sr, juce::jmax (20.0f, S.fromHz), 0.7071); } }
        const float og = db2g (S.outDb); const int hD = (int) (S.haas * 0.012 * sr);
        for (int i = 0; i < n; ++i)
        {
            float m = 0.5f * (L[i] + R[i]), s = 0.5f * (L[i] - R[i]);
            // lato diviso in sotto/sopra la frequenza (crossover a due poli, somma piatta in ampiezza)
            const float sLo = xo[1].p (xo[0].p (s)), sHi = s - sLo;
            float sOut = (S.monoBass ? 0.0f : sLo) + sHi * S.width;
            if (hD > 0) { hd.push (m); sOut += (m - hd.tap (hD)) * 0.35f * juce::jmin (1.0f, S.width - 1.0f + 0.3f); }
            // un filo di compensazione: allargando molto il centro resta presente
            const float mg = S.width > 1 ? 1.0f / std::sqrt (1.0f + (S.width - 1.0f) * 0.25f) : 1.0f;
            L[i] = (m * mg + sOut) * og; R[i] = (m * mg - sOut) * og;
        }
    }
private:
    double sr = 44100; jkit::Bq xo[4]; float last = -1; Line hd;
};

// ======================================================================= utility
struct UtilSettings { float gainDb = 0, pan = 0, loHz = 10, hiHz = 22000; bool phL = false, phR = false, mono = false, swap = false; };
class Utility
{
public:
    void prepare (double s) { sr = s; for (auto& f : hp) f.reset(); for (auto& f : lp) f.reset(); lh = ll = -1; }
    void process (float* L, float* R, int n, const UtilSettings& S)
    {
        if (S.loHz != lh) { lh = S.loHz; for (int c = 0; c < 4; ++c) hp[c].hp (sr, juce::jmax (10.0f, S.loHz), c % 2 ? 1.3066 : 0.5412); }
        if (S.hiHz != ll) { ll = S.hiHz; for (int c = 0; c < 4; ++c) lp[c].lp (sr, juce::jmin ((double) S.hiHz, sr * 0.45), c % 2 ? 1.3066 : 0.5412); }
        const float g = db2g (S.gainDb); const float pa = (S.pan + 1) * juce::MathConstants<float>::pi * 0.25f; const float gl = std::cos (pa) * 1.41421f, gr = std::sin (pa) * 1.41421f;
        const bool hpOn = S.loHz > 10.5f, lpOn = S.hiHz < 21900;
        for (int i = 0; i < n; ++i)
        {
            float l = L[i] * (S.phL ? -1.0f : 1.0f), r = R[i] * (S.phR ? -1.0f : 1.0f);
            if (S.swap) std::swap (l, r);
            if (S.mono) l = r = 0.5f * (l + r);
            if (hpOn) { l = hp[1].p (hp[0].p (l)); r = hp[3].p (hp[2].p (r)); }
            if (lpOn) { l = lp[1].p (lp[0].p (l)); r = lp[3].p (lp[2].p (r)); }
            L[i] = l * g * gl; R[i] = r * g * gr;
        }
    }
private:
    double sr = 44100; jkit::Bq hp[4], lp[4]; float lh = -1, ll = -1;
};
}
