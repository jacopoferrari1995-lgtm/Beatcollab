#pragma once
// Effetti da performance: 8 slot, in ogni slot un effetto a scelta, una manopola sola (0..1).
// Lo stesso codice serve al Sampler e servira' al plugin Jackpot FX: e' il "motore" degli effetti.
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <cmath>

namespace pfx
{
enum Type { OFF, FILTER, GAIN, REVERB, DELAY, SCRATCH, TAPESTOP, STUTTER, GLITCH, GATE, CRUSH, LOFI, TAPE, VINYL,
            FLANGER, PHASER, CHORUS, DIMENSION, TREMOLO, AUTOPAN, WIDE, kNumTypes };

struct Info { const char* name; float neutral; bool spring; bool bipolar; const char* group; const char* tip; };
inline const Info& info (int t)
{
    static const Info I[kNumTypes] = {
        { "OFF",        0.0f, false, false, "",          "Slot vuoto." },
        { "FILTRO",     0.5f, false, true,  "FILTRI",    "Filtro da DJ: a sinistra passa basso, a destra passa alto, al centro spento." },
        { "GAIN",       0.5f, false, true,  "FILTRI",    "Volume da -12 a +12 dB, al centro 0." },
        { "REVERB",     0.0f, false, false, "SPAZIO",    "Riverbero grande sopra al suono." },
        { "DELAY",      0.0f, false, false, "SPAZIO",    "Delay ping pong a 1/8 puntato, a tempo con la DAW." },
        { "SCRATCH",    0.5f, true,  true,  "TEMPO",     "La manopola e' la mano sul vinile: muovila avanti e indietro. Ferma, il disco riparte." },
        { "TAPE STOP",  0.0f, true,  false, "TEMPO",     "Il nastro rallenta fino a fermarsi. Torna a zero e riparte." },
        { "STUTTER",    0.0f, true,  false, "TEMPO",     "Ripete l'ultimo pezzetto: 1/8, 1/16, 1/32 secondo quanto giri." },
        { "GLITCH",     0.0f, true,  false, "TEMPO",     "Salti, ripetizioni e pezzi al contrario a tempo. Piu giri, piu succede." },
        { "GATE",       0.0f, true,  false, "TEMPO",     "Taglia il suono a tempo: 1/8, 1/16, 1/32 secondo quanto giri." },
        { "CRUSH",      0.0f, false, false, "SPORCO",    "Meno bit: da granuloso a distrutto." },
        { "LO-FI",      0.0f, false, false, "SPORCO",    "Meno bit, meno campioni, meno alti: campionatore vecchio." },
        { "TAPE",       0.0f, false, false, "SPORCO",    "Nastro: saturazione morbida, wow e flutter." },
        { "VINYL",      0.0f, false, false, "SPORCO",    "Disco: fruscio, crepitio e alti arrotondati." },
        { "FLANGER",    0.0f, false, false, "MODULAZIONE", "Flanger: il jet che sale e scende." },
        { "PHASER",     0.0f, false, false, "MODULAZIONE", "Phaser: il suono che gira." },
        { "CHORUS",     0.0f, false, false, "MODULAZIONE", "Chorus: piu voci leggermente stonate, piu largo." },
        { "DIMENSION",  0.0f, false, false, "MODULAZIONE", "Dimension: allarga e addolcisce senza far ondeggiare." },
        { "TREMOLO",    0.0f, false, false, "MODULAZIONE", "Volume che pulsa a tempo (1/8)." },
        { "AUTOPAN",    0.0f, false, false, "MODULAZIONE", "Il suono va da sinistra a destra a tempo (1/4)." },
        { "WIDE",       0.5f, false, true,  "SPAZIO",    "A sinistra verso il mono, a destra piu largo." } };
    return I[juce::jlimit (0, kNumTypes - 1, t)];
}

// ritardo modulabile, piccolo (fino a ~60 ms)
struct ModDelay
{
    std::vector<float> b[2]; int w = 0, n = 1;
    void prepare (double sr) { n = (int) (sr * 0.06) + 4; for (auto& x : b) x.assign ((size_t) n, 0.0f); w = 0; }
    void push (float l, float r) { b[0][(size_t) w] = l; b[1][(size_t) w] = r; w = (w + 1) % n; }
    float read (int c, float d) const   // d in campioni dietro all'ultimo scritto
    {
        float p = (float) w - 1 - d; while (p < 0) p += (float) n;
        const int a = (int) p; const float f = p - (float) a; const int a1 = (a + 1) % n;
        return b[c][(size_t) a] + (b[c][(size_t) a1] - b[c][(size_t) a]) * f;
    }
};

class Slot
{
public:
    void prepare (double s)
    {
        sr = s;
        hist.setSize (2, (int) (sr * 3)); hist.clear(); hw = 0;
        dly.setSize (2, (int) (sr * 2)); dly.clear(); dw = 0;
        md.prepare (sr);
        rev.setSampleRate (sr); rev.reset();
        juce::Reverb::Parameters rp; rp.roomSize = 0.82f; rp.damping = 0.45f; rp.wetLevel = 1; rp.dryLevel = 0; rp.width = 1; rev.setParameters (rp);
        reset();
    }
    void reset()
    {
        stOn = stuOn = scOn = false; stX = stuX = scX = 0; stSpeed = 1; scV = -1; glX = 0;
        for (int c = 0; c < 2; ++c) { ic1[c] = ic2[c] = 0; lfHold[c] = lfLp[c] = 0; tLp[c] = 0; vLp[c] = 0; for (auto& a : ap[c]) a = 0; dlLp[c] = 0; phFb[c] = 0; }
        djMode = 0; fxG = 1; amtS = -1; phLfo = 0; free = 0;
    }
    void setType (int t) { if (t != type) { type = t; reset(); } }
    int type = OFF;
    // amt: valore della manopola (0..1). bpm, ppq e playing: per gli effetti a tempo
    void process (float* L, float* R, int N, float amt, double bpm, double ppq, bool playing)
    {
        // storia per gli effetti che rileggono il passato (sempre scritta, cosi' entrano senza buchi)
        if (type == OFF) { pushHist (L, R, N); return; }
        if (amtS < 0) amtS = amt;
        const double spb = 60.0 / juce::jmax (40.0, bpm) * sr;   // campioni per battito
        const double beat0 = playing ? ppq : free / spb; free += N;
        switch (type)
        {
            case FILTER:    filter (L, R, N, amt); pushHist (L, R, N); break;
            case GAIN:      { const float tg = juce::Decibels::decibelsToGain ((amt - 0.5f) * 24.0f); for (int i = 0; i < N; ++i) { fxG += (tg - fxG) * 0.002f; L[i] *= fxG; R[i] *= fxG; } pushHist (L, R, N); break; }
            case REVERB:    reverb (L, R, N, amt); pushHist (L, R, N); break;
            case DELAY:     delay (L, R, N, amt, spb); pushHist (L, R, N); break;
            case SCRATCH:   scratch (L, R, N, amt); break;
            case TAPESTOP:  tapeStop (L, R, N, amt); break;
            case STUTTER:   stutter (L, R, N, amt, spb); break;
            case GLITCH:    glitch (L, R, N, amt, spb, beat0); break;
            case GATE:      gate (L, R, N, amt, spb, beat0); pushHist (L, R, N); break;
            case CRUSH:     crush (L, R, N, amt); pushHist (L, R, N); break;
            case LOFI:      lofi (L, R, N, amt); pushHist (L, R, N); break;
            case TAPE:      tape (L, R, N, amt); pushHist (L, R, N); break;
            case VINYL:     vinyl (L, R, N, amt); pushHist (L, R, N); break;
            case FLANGER:   flanger (L, R, N, amt); pushHist (L, R, N); break;
            case PHASER:    phaser (L, R, N, amt); pushHist (L, R, N); break;
            case CHORUS:    chorus (L, R, N, amt, false); pushHist (L, R, N); break;
            case DIMENSION: chorus (L, R, N, amt, true); pushHist (L, R, N); break;
            case TREMOLO:   tremolo (L, R, N, amt, spb, beat0, false); pushHist (L, R, N); break;
            case AUTOPAN:   tremolo (L, R, N, amt, spb, beat0, true); pushHist (L, R, N); break;
            case WIDE:      wide (L, R, N, amt); pushHist (L, R, N); break;
            default:        pushHist (L, R, N); break;
        }
    }
private:
    double sr = 48000, free = 0;
    juce::AudioBuffer<float> hist, dly; int hw = 0, dw = 0;
    ModDelay md; juce::Reverb rev; juce::Random rnd { 7 };
    static constexpr int kChunk = 512; float wl[kChunk] {}, wr[kChunk] {}, phFb[2] {};
    // stati
    bool stOn = false, stuOn = false, scOn = false; float stX = 0, stuX = 0, scX = 0, stSpeed = 1; double stR = 0, stuStart = 0, stuPos = 0, scAnchor = 0; int stuLen = 0, scIdle = 0; float scV = -1, scGrab = 0;
    float ic1[2] {}, ic2[2] {}, djMode = 0, djFc = 1000, fxG = 1, amtS = -1; float lfHold[2] {}, lfLp[2] {}; int lfCnt = 0;
    float tLp[2] {}, vLp[2] {}, ap[2][6] {}, dlLp[2] {}; double phLfo = 0;
    int glStep = -1, glMode = 0; double glStart = 0; float glX = 0;

    void pushHist (const float* L, const float* R, int N)
    {
        const int len = hist.getNumSamples(); float* a = hist.getWritePointer (0); float* b = hist.getWritePointer (1);
        for (int i = 0; i < N; ++i) { a[hw] = L[i]; b[hw] = R[i]; if (++hw >= len) hw = 0; }
    }
    float rd (int c, double p) const
    {
        const int len = hist.getNumSamples(); while (p < 0) p += len; while (p >= len) p -= len;
        const int a = (int) p; const float f = (float) (p - a); const float* d = hist.getReadPointer (c);
        return d[a] + (d[(a + 1) % len] - d[a]) * f;
    }
    // ---------------------------------------------------------------- effetti
    void filter (float* L, float* R, int N, float amt)
    {
        const float x = (amt - 0.5f) * 2.0f;
        if (std::abs (x) <= 0.03f) { djMode = 0; for (int c = 0; c < 2; ++c) ic1[c] = ic2[c] = 0; return; }
        const float mode = x < 0 ? -1.0f : 1.0f, t = (std::abs (x) - 0.03f) / 0.97f;
        const float fc = x < 0 ? (float) (20000.0 * std::pow (0.008, (double) t)) : (float) (20.0 * std::pow (150.0, (double) t));
        if (mode != djMode) { djMode = mode; djFc = fc; for (int c = 0; c < 2; ++c) ic1[c] = ic2[c] = 0; }
        const float k = 2.0f - 2.0f * 0.3f;
        for (int i = 0; i < N; ++i)
        {
            djFc += (fc - djFc) * 0.002f;
            const float g = (float) std::tan (juce::MathConstants<double>::pi * juce::jmin ((double) djFc, sr * 0.45) / sr);
            const float a1 = 1.0f / (1.0f + g * (g + k)), a2 = g * a1, a3 = g * a2;
            float* io[2] = { L + i, R + i };
            for (int c = 0; c < 2; ++c)
            {
                const float v0 = *io[c], v3 = v0 - ic2[c], v1 = a1 * ic1[c] + a2 * v3, v2 = ic2[c] + a2 * ic1[c] + a3 * v3;
                ic1[c] = 2 * v1 - ic1[c]; ic2[c] = 2 * v2 - ic2[c];
                *io[c] = mode < 0 ? v2 : v0 - k * v1 - v2;
            }
        }
    }
    void reverb (float* L, float* R, int N, float amt)
    {
        if (amt < 0.005f && amtS < 0.005f) { amtS = amt; return; }
        for (int off = 0; off < N; off += kChunk)
        {
            const int m = juce::jmin (kChunk, N - off);
            for (int i = 0; i < m; ++i) { wl[i] = L[off + i]; wr[i] = R[off + i]; }
            rev.processStereo (wl, wr, m);
            for (int i = 0; i < m; ++i) { amtS += (amt - amtS) * 0.002f; L[off + i] += wl[i] * amtS * 0.45f; R[off + i] += wr[i] * amtS * 0.45f; }
        }
    }
    void delay (float* L, float* R, int N, float amt, double spb)
    {
        const int len = dly.getNumSamples(); const double dT = juce::jlimit (16.0, (double) len - 4, spb * 0.75);   // 1/8 puntato
        float* a = dly.getWritePointer (0); float* b = dly.getWritePointer (1);
        const float lpA = (float) std::exp (-juce::MathConstants<double>::twoPi * 5000.0 / sr);
        for (int i = 0; i < N; ++i)
        {
            amtS += (amt - amtS) * 0.002f;
            double rp = dw - dT; if (rp < 0) rp += len;
            const int i0 = (int) rp; const float fr = (float) (rp - i0); const int i1 = (i0 + 1) % len;
            const float yL = a[i0] + (a[i1] - a[i0]) * fr, yR = b[i0] + (b[i1] - b[i0]) * fr;
            dlLp[0] = yR + lpA * (dlLp[0] - yR); dlLp[1] = yL + lpA * (dlLp[1] - yL);   // ping pong
            const float sendIn = amtS;
            a[dw] = 0.5f * (L[i] + R[i]) * sendIn + dlLp[0] * 0.5f; b[dw] = dlLp[1] * 0.5f;
            if (++dw >= len) dw = 0;
            L[i] += yL * amtS; R[i] += yR * amtS;
        }
    }
    void scratch (float* L, float* R, int N, float amt)
    {
        const int len = hist.getNumSamples(); float* b0 = hist.getWritePointer (0); float* b1 = hist.getWritePointer (1);
        if (scV < 0) scV = amt;
        const float sm = (float) (1.0 - std::exp (-1.0 / (0.012 * sr))); const double range = 0.9 * sr;
        for (int i = 0; i < N; ++i)
        {
            b0[hw] = L[i]; b1[hw] = R[i];
            const float prev = scV; scV += (amt - scV) * sm;
            if (std::abs (scV - prev) > 2e-6f || std::abs (amt - scV) > 1e-4f)
            { if (! scOn) { scOn = true; scAnchor = hw - 0.12 * sr; if (scAnchor < 0) scAnchor += len; scGrab = scV; } scIdle = 0; }
            else if (scOn && ++scIdle > (int) (0.25 * sr)) scOn = false;
            if (scOn || scX > 0)
            {
                double p = scAnchor + (scV - scGrab) * range; double lag = hw - p; while (lag < 0) lag += len; while (lag >= len) lag -= len; if (lag < 2) p = hw - 2;
                const float tl = rd (0, p), tr = rd (1, p);
                scX = juce::jlimit (0.0f, 1.0f, scX + (scOn ? 1.0f : -1.0f) / (float) (0.02 * sr));
                L[i] += (tl - L[i]) * scX; R[i] += (tr - R[i]) * scX;
            }
            if (++hw >= len) hw = 0;
        }
    }
    void tapeStop (float* L, float* R, int N, float amt)
    {
        const int len = hist.getNumSamples(); float* b0 = hist.getWritePointer (0); float* b1 = hist.getWritePointer (1);
        const float target = 1.0f - juce::jlimit (0.0f, 1.0f, amt);
        if (amt > 0.01f && ! stOn) { stOn = true; stR = hw; stSpeed = 1; }
        const float sc = (float) (1.0 - std::exp (-1.0 / (0.06 * sr)));
        for (int i = 0; i < N; ++i)
        {
            b0[hw] = L[i]; b1[hw] = R[i];
            if (stOn || stX > 0)
            {
                stSpeed += ((stOn ? target : 1.0f) - stSpeed) * sc;
                stR += stSpeed; if (stR >= len) stR -= len;
                double lag = hw - stR; if (lag < 0) lag += len; if (lag > len - 4) stR = hw;
                const float g = juce::jmin (1.0f, stSpeed * 3.0f);
                const float tl = rd (0, stR) * g, tr = rd (1, stR) * g;
                stX = juce::jlimit (0.0f, 1.0f, stX + (stOn ? 1.0f : -1.0f) / (float) (0.03 * sr));
                L[i] += (tl - L[i]) * stX; R[i] += (tr - R[i]) * stX;
            }
            if (++hw >= len) hw = 0;
        }
        if (amt <= 0.01f) stOn = false;
    }
    void stutter (float* L, float* R, int N, float amt, double spb)
    {
        const int len = hist.getNumSamples(); float* b0 = hist.getWritePointer (0); float* b1 = hist.getWritePointer (1);
        const double beats = amt < 0.34f ? 0.5 : amt < 0.67f ? 0.25 : 0.125;
        const int want = juce::jlimit (64, len - 8, (int) (beats * spb));
        if (amt > 0.01f && ! stuOn) { stuOn = true; stuLen = want; stuStart = hw - want; if (stuStart < 0) stuStart += len; stuPos = 0; }
        if (stuOn) stuLen = want;
        const float fade = (float) (0.003 * sr);
        for (int i = 0; i < N; ++i)
        {
            b0[hw] = L[i]; b1[hw] = R[i];
            if (stuOn || stuX > 0)
            {
                const float w = juce::jmin (1.0f, juce::jmin ((float) stuPos / fade, (float) (stuLen - stuPos) / fade));
                const float tl = rd (0, stuStart + stuPos) * w, tr = rd (1, stuStart + stuPos) * w;
                stuX = juce::jlimit (0.0f, 1.0f, stuX + (stuOn ? 1.0f : -1.0f) / (float) (0.005 * sr));
                L[i] += (tl - L[i]) * stuX; R[i] += (tr - R[i]) * stuX;
                stuPos += 1; if (stuPos >= stuLen) stuPos = 0;
            }
            if (++hw >= len) hw = 0;
        }
        if (amt <= 0.01f) stuOn = false;
    }
    void glitch (float* L, float* R, int N, float amt, double spb, double beat0)
    {   // a ogni sedicesimo, con probabilita' = manopola: ripeti 1/32, pezzo al contrario, o salto di ottava giu
        const int len = hist.getNumSamples(); float* b0 = hist.getWritePointer (0); float* b1 = hist.getWritePointer (1);
        const double step = spb * 0.25;
        for (int i = 0; i < N; ++i)
        {
            b0[hw] = L[i]; b1[hw] = R[i];
            const double beat = beat0 + i / spb; const int st = (int) std::floor (beat * 4.0);
            if (st != glStep)
            {
                glStep = st;
                glMode = (amt > 0.01f && rnd.nextFloat() < amt * 0.85f) ? 1 + rnd.nextInt (3) : 0;
                glStart = hw;
            }
            const double inStep = (beat * 4.0 - st) * step;
            float tl = L[i], tr = R[i];
            if (glMode == 1) { const double sl = step * 0.5; const double p = glStart - sl + std::fmod (inStep, sl); tl = rd (0, p); tr = rd (1, p); }
            else if (glMode == 2) { const double p = glStart - inStep; tl = rd (0, p); tr = rd (1, p); }
            else if (glMode == 3) { const double p = glStart - step + inStep * 0.5; tl = rd (0, p); tr = rd (1, p); }
            const float target = glMode ? 1.0f : 0.0f;
            glX += (target - glX) * (float) (1.0 - std::exp (-1.0 / (0.002 * sr)));
            L[i] += (tl - L[i]) * glX; R[i] += (tr - R[i]) * glX;
            if (++hw >= len) hw = 0;
        }
    }
    void gate (float* L, float* R, int N, float amt, double spb, double beat0)
    {
        if (amt <= 0.01f) return;
        const double div = amt < 0.34f ? 2.0 : amt < 0.67f ? 4.0 : 8.0;   // colpi per battito
        const float edge = 0.08f;
        for (int i = 0; i < N; ++i)
        {
            const double ph = std::fmod ((beat0 + i / spb) * div, 1.0);
            float g = ph < 0.5 ? 1.0f : 0.0f;
            if (ph < edge) g = (float) (ph / edge); else if (ph >= 0.5 && ph < 0.5 + edge) g = 1.0f - (float) ((ph - 0.5) / edge);
            L[i] *= g; R[i] *= g;
        }
    }
    void crush (float* L, float* R, int N, float amt)
    {
        if (amt <= 0.01f) return;
        const float bits = 16.0f - amt * 13.0f, q = std::pow (2.0f, bits - 1);
        for (int i = 0; i < N; ++i) { L[i] = std::round (L[i] * q) / q; R[i] = std::round (R[i] * q) / q; }
    }
    void lofi (float* L, float* R, int N, float amt)
    {
        if (amt <= 0.01f) return;
        const float bits = 16.0f - amt * 10.0f, q = std::pow (2.0f, bits - 1); const int hold = 1 + (int) (amt * 7.0f);
        const float a = (float) std::exp (-juce::MathConstants<double>::twoPi * (16000.0 * std::pow (0.22, (double) amt)) / sr);
        for (int i = 0; i < N; ++i)
        {
            if (++lfCnt >= hold) { lfCnt = 0; lfHold[0] = std::round (L[i] * q) / q; lfHold[1] = std::round (R[i] * q) / q; }
            lfLp[0] = lfHold[0] + a * (lfLp[0] - lfHold[0]); lfLp[1] = lfHold[1] + a * (lfLp[1] - lfHold[1]);
            L[i] = lfLp[0]; R[i] = lfLp[1];
        }
    }
    void tape (float* L, float* R, int N, float amt)
    {
        if (amt <= 0.01f) { for (int i = 0; i < N; ++i) md.push (L[i], R[i]); return; }
        const float drive = 1.0f + amt * 4.0f, mk = 1.0f / drive;
        const float a = (float) std::exp (-juce::MathConstants<double>::twoPi * (18000.0 - 9000.0 * amt) / sr);
        for (int i = 0; i < N; ++i)
        {
            md.push (L[i], R[i]);
            phLfo += 1.0 / sr;
            const float wow = (float) (std::sin (phLfo * juce::MathConstants<double>::twoPi * 0.6) * 0.0018 + std::sin (phLfo * juce::MathConstants<double>::twoPi * 7.3) * 0.00025) * amt;
            const float d = (float) (0.004 * sr) + wow * (float) sr;
            float o[2] = { md.read (0, d), md.read (1, d) };
            for (int c = 0; c < 2; ++c) { const float s = std::tanh (o[c] * drive) * mk; tLp[c] = s + a * (tLp[c] - s); o[c] = tLp[c]; }
            L[i] += (o[0] - L[i]) * juce::jmin (1.0f, amt * 2.0f); R[i] += (o[1] - R[i]) * juce::jmin (1.0f, amt * 2.0f);
        }
    }
    void vinyl (float* L, float* R, int N, float amt)
    {
        if (amt <= 0.01f) return;
        const float a = (float) std::exp (-juce::MathConstants<double>::twoPi * (16000.0 - 10000.0 * amt) / sr);
        for (int i = 0; i < N; ++i)
        {
            float n = (rnd.nextFloat() * 2 - 1) * 0.004f * amt;
            if (rnd.nextFloat() < 0.0004f * amt) n += (rnd.nextFloat() * 2 - 1) * 0.35f * amt;   // crepitio
            for (int c = 0; c < 2; ++c) { float* x = c ? R : L; vLp[c] = x[i] + a * (vLp[c] - x[i]); x[i] = vLp[c] + n; }
        }
    }
    void flanger (float* L, float* R, int N, float amt)
    {
        for (int i = 0; i < N; ++i)
        {
            phLfo += 0.22 / sr; const float l = (float) (0.5 + 0.5 * std::sin (phLfo * juce::MathConstants<double>::twoPi));
            const float d0 = (float) ((0.0006 + 0.004 * l) * sr), d1 = (float) ((0.0006 + 0.004 * (1 - l)) * sr);
            const float y0 = md.read (0, d0), y1 = md.read (1, d1);
            md.push (L[i] + y0 * 0.6f * amt, R[i] + y1 * 0.6f * amt);
            L[i] = L[i] * (1 - 0.5f * amt) + y0 * amt * 0.7f; R[i] = R[i] * (1 - 0.5f * amt) + y1 * amt * 0.7f;
        }
    }
    void phaser (float* L, float* R, int N, float amt)
    {
        if (amt <= 0.01f) return;
        for (int i = 0; i < N; ++i)
        {
            phLfo += 0.35 / sr; const float l = (float) (0.5 + 0.5 * std::sin (phLfo * juce::MathConstants<double>::twoPi));
            const float fc = 200.0f * std::pow (20.0f, l); const float t = (float) std::tan (juce::MathConstants<double>::pi * fc / sr), co = (t - 1) / (t + 1);
            float* io[2] = { L + i, R + i };
            for (int c = 0; c < 2; ++c)
            {
                float y = *io[c] + phFb[c] * 0.5f * amt;
                for (int k = 0; k < 6; ++k) { const float o = co * y + ap[c][k]; ap[c][k] = y - co * o; y = o; }
                phFb[c] = y;
                *io[c] = *io[c] * (1 - 0.5f * amt) + y * 0.5f * amt * 1.4f;
            }
        }
    }
    void chorus (float* L, float* R, int N, float amt, bool dim)
    {
        for (int i = 0; i < N; ++i)
        {
            md.push (L[i], R[i]);
            phLfo += (dim ? 0.25 : 0.8) / sr; const double ph = phLfo * juce::MathConstants<double>::twoPi;
            const float dep = (float) ((dim ? 0.0012 : 0.003) * sr), base = (float) ((dim ? 0.009 : 0.016) * sr);
            const float yl = md.read (0, base + dep * (float) std::sin (ph)), yr = md.read (1, base + dep * (float) std::sin (ph + juce::MathConstants<double>::pi));
            if (dim) { const float m = 0.5f * (yl + yr); L[i] += (0.5f * (yl - yr) + m * 0.3f) * amt * 0.8f; R[i] += (0.5f * (yr - yl) + m * 0.3f) * amt * 0.8f; }
            else { L[i] = L[i] * (1 - 0.35f * amt) + yl * amt * 0.6f; R[i] = R[i] * (1 - 0.35f * amt) + yr * amt * 0.6f; }
        }
    }
    void tremolo (float* L, float* R, int N, float amt, double spb, double beat0, bool pan)
    {
        if (amt <= 0.01f) return;
        for (int i = 0; i < N; ++i)
        {
            const double beat = beat0 + i / spb;
            if (pan) { const float p = (float) std::sin (beat * juce::MathConstants<double>::pi) * amt; L[i] *= std::sqrt (0.5f * (1 - p)) * 1.414f; R[i] *= std::sqrt (0.5f * (1 + p)) * 1.414f; }
            else { const float g = 1.0f - amt * (float) (0.5 + 0.5 * std::cos (beat * 2.0 * juce::MathConstants<double>::twoPi)); L[i] *= g; R[i] *= g; }
        }
    }
    void wide (float* L, float* R, int N, float amt)
    {
        const float w = amt < 0.5f ? amt * 2.0f : 1.0f + (amt - 0.5f) * 2.0f;
        if (std::abs (w - 1.0f) < 0.01f) return;
        for (int i = 0; i < N; ++i) { const float m = 0.5f * (L[i] + R[i]), s = 0.5f * (L[i] - R[i]) * w; L[i] = m + s; R[i] = m - s; }
    }
};

class Engine
{
public:
    static constexpr int kSlots = 8;
    Slot slot[kSlots];
    void prepare (double sr) { for (auto& s : slot) s.prepare (sr); }
    void process (float* L, float* R, int N, const int* types, const float* amts, double bpm, double ppq, bool playing)
    {
        for (int k = 0; k < kSlots; ++k) { slot[k].setType (types[k]); slot[k].process (L, R, N, amts[k], bpm, ppq, playing); }
    }
};

// testo da mostrare sotto la manopola
inline juce::String valueText (int t, float v)
{
    switch (t)
    {
        case FILTER: { const float x = (v - 0.5f) * 2; return std::abs (x) <= 0.03f ? juce::String ("OFF") : x < 0 ? "LP " + juce::String (juce::roundToInt (-x * 100)) + "%" : "HP " + juce::String (juce::roundToInt (x * 100)) + "%"; }
        case GAIN: { const float d = (v - 0.5f) * 24; return (d > 0 ? "+" : "") + juce::String (d, 1) + " dB"; }
        case WIDE: return juce::String (juce::roundToInt (v * 200)) + "%";
        case SCRATCH: return juce::String ("MUOVI");
        case STUTTER: case GATE: return v <= 0.01f ? juce::String ("OFF") : v < 0.34f ? juce::String ("1/8") : v < 0.67f ? juce::String ("1/16") : juce::String ("1/32");
        case OFF: return juce::String ("-");
        default: return v <= 0.01f ? juce::String ("OFF") : juce::String (juce::roundToInt (v * 100)) + "%";
    }
}
}
