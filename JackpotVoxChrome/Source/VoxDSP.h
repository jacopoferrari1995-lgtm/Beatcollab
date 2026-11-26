#pragma once
// VOX CHROME: la voce diventa cyborg, e a fine frase partono da soli i "throw" (eco, spazio, glitch).
// Unita' CROMO: risonanze metalliche, robot (modulatore ad anello), spostamento di frequenza, bit, drive, banda.
// Protocollo THROW: un rivelatore ascolta la voce; quando una frase finisce apre l'eco, il riverbero e il glitch
// solo sull'ultima parola, e li richiude quando riparti a cantare. Tutto a zero: la voce passa identica.
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <cmath>
#include <vector>

namespace jvox
{
struct Settings
{
    float cromo = 0, robot = 0, shift = 0, bit = 0, drive = 0, band = 0, width = 0;
    float eco = 0, space = 0, glitch = 0, fb = 0.4f, sens = 0.5f; int div = 3;
    float outDb = 0; double bpm = 120;
};
inline const char* divName (int i) { static const char* n[] = { "1/16", "1/8", "1/8.", "1/4", "1/4.", "1/2" }; return n[juce::jlimit (0, 5, i)]; }
inline float divBeats (int i) { static const float b[] = { 0.25f, 0.5f, 0.75f, 1.0f, 1.5f, 2.0f }; return b[juce::jlimit (0, 5, i)]; }

struct Svf
{
    float ic1 = 0, ic2 = 0, a1 = 0, a2 = 0, a3 = 0, k = 1.41f;
    void set (float fc, float sr, float q = 0.707f) { const float g = std::tan (juce::MathConstants<float>::pi * juce::jlimit (10.0f, sr * 0.45f, fc) / sr); k = 1.0f / q; a1 = 1.0f / (1.0f + g * (g + k)); a2 = g * a1; a3 = g * a2; }
    float lp (float x) { float l, b, h; tick (x, l, b, h); return l; }
    float hp (float x) { float l, b, h; tick (x, l, b, h); return h; }
    void tick (float x, float& l, float& b, float& h) { const float v3 = x - ic2, v1 = a1 * ic1 + a2 * v3, v2 = ic2 + a2 * ic1 + a3 * v3; ic1 = 2 * v1 - ic1; ic2 = 2 * v2 - ic2; l = v2; b = v1; h = x - k * v1 - v2; }
};
// rete a 90 gradi (Niemitalo): due catene di passa-tutto la cui differenza di fase resta ~90 gradi su quasi tutta la banda
struct Hilbert
{
    std::array<float, 4> c1 { 0.6923878f, 0.9360654322959f, 0.9882295226860f, 0.9987488452737f }, c2 { 0.4021921162426f, 0.8561710882420f, 0.9722909545651f, 0.9952884791278f };
    std::array<float, 4> x1a {}, x2a {}, y1a {}, y2a {}, x1b {}, x2b {}, y1b {}, y2b {}; float d = 0;
    void process (float x, float& re, float& im)
    {
        float a = x, b = x;
        for (int i = 0; i < 4; ++i) { const float c = c1[(size_t) i] * c1[(size_t) i]; const float y = c * (a + y2a[(size_t) i]) - x2a[(size_t) i]; x2a[(size_t) i] = x1a[(size_t) i]; x1a[(size_t) i] = a; y2a[(size_t) i] = y1a[(size_t) i]; y1a[(size_t) i] = y; a = y; }
        for (int i = 0; i < 4; ++i) { const float c = c2[(size_t) i] * c2[(size_t) i]; const float y = c * (b + y2b[(size_t) i]) - x2b[(size_t) i]; x2b[(size_t) i] = x1b[(size_t) i]; x1b[(size_t) i] = b; y2b[(size_t) i] = y1b[(size_t) i]; y1b[(size_t) i] = y; b = y; }
        re = d; d = a; im = b;
    }
};

class Engine
{
public:
    void prepare (double s, int maxBlock)
    {
        sr = (float) s; mb = juce::jmax (32, maxBlock);
        for (auto& c : ch) { c = {}; for (auto& v : c.comb) v.assign (kComb, 0.0f); c.dbl.assign (kDbl, 0.0f); }
        hist.assign (kHist, 0.0f); for (auto& v : dl) v.assign (kDl, 0.0f); pre.assign (kPre, 0.0f);
        for (auto* v : { &vL, &vR, &og }) v->assign ((size_t) mb, 0.0f);
        rev.setSampleRate (s); rev.reset(); juce::Reverb::Parameters p; p.roomSize = 0.88f; p.damping = 0.55f; p.wetLevel = 1; p.dryLevel = 0; p.width = 1; rev.setParameters (p);
        env = 0; active = false; activeT = 0; lowT = 0; gate = 0; gateT = 0; hpos = dpos = ppos = 0; glLeft = 0; sm = {}; first = true;
        for (auto& c : ch) for (int k = 0; k < 3; ++k) c.combLp[(size_t) k] = 0;
        hpOut[0].set (30, sr); hpOut[1].set (30, sr);
    }
    // per l'interfaccia: inviluppo della voce, momenti in cui parte un throw
    std::atomic<float> uiEnv { 0 }, uiGate { 0 }; std::atomic<int> uiThrows { 0 };

    void process (float* L, float* R, int n, const Settings& S)
    {
        for (int o = 0; o < n; o += mb) chunk (L + o, R + o, juce::jmin (mb, n - o), S);
    }
private:
    static constexpr int kComb = 4096, kDbl = 4096, kHist = 1 << 17, kDl = 1 << 18, kPre = 1 << 15;
    struct Ch
    {
        std::array<std::vector<float>, 3> comb; std::array<float, 3> combLp {}; int cpos = 0;
        std::vector<float> dbl; Hilbert hil; Svf bhp1, bhp2, blp1, blp2, echoLp, echoHp; float hold = 0;
    };
    struct Sm { float cromo = 0, robot = 0, shift = 0, bit = 0, drive = 0, band = 0, width = 0, eco = 0, space = 0, glitch = 0, fb = 0, out = 1, dtime = 0; };
    float sr = 44100; int mb = 512; bool first = true;
    std::array<Ch, 2> ch; Sm sm;
    std::vector<float> hist, pre, vL, vR, og; int revTail = 0; std::array<std::vector<float>, 2> dl; int hpos = 0, dpos = 0, ppos = 0;
    juce::Reverb rev; std::array<Svf, 2> hpOut;
    double phR = 0, phS = 0, phD = 0, holdPh = 0;
    float env = 0, thr = 0.03f; bool active = false; int activeT = 0, lowT = 0, onT = 0; float gate = 0, gateT = 0;
    int glStart = 0, glLen = 1, glLeft = 0, glRep = 0; double glPos = 0, glRate = 1; float glAmp = 0;

    static float sm1 (float& s, float t, float k) { s += (t - s) * k; return s; }

    void chunk (float* L, float* R, int n, const Settings& S)
    {
        const float k = 1.0f - std::exp (-1.0f / (0.025f * sr));
        const float outT = juce::Decibels::decibelsToGain (S.outDb);
        const float dtimeT = juce::jlimit (0.02f, 2.5f, (float) (60.0 / juce::jlimit (40.0, 300.0, S.bpm)) * divBeats (S.div)) * sr;
        if (first) { sm.dtime = dtimeT; sm.out = outT; first = false; }
        thr = juce::Decibels::decibelsToGain (-18.0f - 30.0f * S.sens);
        const float aE = 1.0f - std::exp (-1.0f / (0.002f * sr)), rE = 1.0f - std::exp (-1.0f / (0.025f * sr));
        const int holdN = (int) (0.2f * sr), minAct = (int) (0.15f * sr), onN = (int) (0.03f * sr);
        float envPk = 0; int throws = 0;
        for (int i = 0; i < n; ++i)
        {
            const float cromo = sm1 (sm.cromo, S.cromo, k), robot = sm1 (sm.robot, S.robot, k), shift = sm1 (sm.shift, S.shift, k), bit = sm1 (sm.bit, S.bit, k),
                        drive = sm1 (sm.drive, S.drive, k), band = sm1 (sm.band, S.band, k), width = sm1 (sm.width, S.width, k), eco = sm1 (sm.eco, S.eco, k),
                        space = sm1 (sm.space, S.space, k), glitch = sm1 (sm.glitch, S.glitch, k), fb = sm1 (sm.fb, S.fb, k), out = sm1 (sm.out, outT, k);
            sm.dtime += (dtimeT - sm.dtime) * 0.0005f;
            if ((i & 15) == 0) for (auto& c : ch)
            {
                c.bhp1.set (20.0f + band * 480.0f, sr); c.bhp2.set (20.0f + band * 480.0f, sr);
                c.blp1.set (20000.0f * std::pow (2.0f, -3.0f * band), sr); c.blp2.set (20000.0f * std::pow (2.0f, -3.0f * band), sr);
                c.echoLp.set (4200.0f, sr); c.echoHp.set (250.0f, sr);
            }
            // ---------------- rivelatore di frase (sulla voce in ingresso)
            const float in = juce::jmax (std::abs (L[i]), std::abs (R[i]));
            env += (in - env) * (in > env ? aE : rE); envPk = juce::jmax (envPk, env);
            if (env > thr) { if (++onT > onN && ! active) { active = true; activeT = 0; } lowT = 0; }
            else onT = 0;
            if (active)
            {
                ++activeT;
                if (env < thr * 0.5f) { if (++lowT > holdN) { active = false; if (activeT > minAct) { gateT = 1; trigger(); ++throws; } } }
                else lowT = 0;
            }
            if (active && activeT > onN) gateT = 0;   // ricanti: i throw si chiudono
            gate += (gateT - gate) * (gateT > gate ? 0.02f : 1.0f / (0.25f * sr));
            // ---------------- unita' cromo
            phR += 92.0 / sr; if (phR > 1) phR -= 1;
            phS += (double) (shift * 260.0f) / sr; if (phS > 1) phS -= 1; if (phS < 0) phS += 1;
            phD += 0.31 / sr; if (phD > 1) phD -= 1;
            holdPh += 1.0 / (1.0 + bit * 14.0); const bool grab = holdPh >= 1.0; if (grab) holdPh -= 1.0;
            float y[2];
            for (int c = 0; c < 2; ++c)
            {
                auto& C = ch[(size_t) c];
                const float x = c == 0 ? L[i] : R[i];
                float v = x;
                if (cromo > 1e-5f)
                {   // tre risonatori metallici (tubi di cromo)
                    static const float hz[3] = { 214.0f, 331.0f, 509.0f };
                    const float g = 0.55f + 0.42f * cromo; float acc = 0;
                    for (int q = 0; q < 3; ++q)
                    {
                        const float d = sr / (hz[q] * (c == 0 ? 1.0f : 1.013f));
                        auto& b = C.comb[(size_t) q]; const float rp = (float) C.cpos - d; const int i0 = (int) std::floor (rp); const float fr = rp - (float) i0;
                        const float o = b[(size_t) (i0 & (kComb - 1))] * (1 - fr) + b[(size_t) ((i0 + 1) & (kComb - 1))] * fr;
                        C.combLp[(size_t) q] += (o - C.combLp[(size_t) q]) * 0.55f;
                        b[(size_t) C.cpos] = v + C.combLp[(size_t) q] * g; acc += o;
                    }
                    v += cromo * (acc * (1 - g) * 1.6f - v * 0.35f);
                }
                if (robot > 1e-5f) v += robot * (v * (float) std::sin (juce::MathConstants<double>::twoPi * phR) * 2.0f - v);
                float re, im; C.hil.process (v, re, im);
                if (std::abs (shift) > 1e-4f)
                {
                    const float a = juce::jmin (1.0f, std::abs (shift) * 12.0f);
                    const float sh = re * (float) std::cos (juce::MathConstants<double>::twoPi * phS) - im * (float) std::sin (juce::MathConstants<double>::twoPi * phS);
                    v += a * (sh - v);
                }
                if (bit > 1e-5f)
                {
                    if (grab) C.hold = v;
                    const float q = std::pow (2.0f, 15.0f - bit * 11.0f);
                    v += juce::jmin (1.0f, bit * 4.0f) * (std::round (C.hold * q) / q - v);
                }
                if (drive > 1e-5f) { const float gd = 1.0f + drive * 18.0f; v += juce::jmin (1.0f, drive * 6.0f) * (std::tanh (v * gd + 0.08f * drive) / std::tanh (gd * 0.5f) * 0.5f - 0.04f * drive - v); }
                if (band > 1e-5f) { float b = C.bhp2.hp (C.bhp1.hp (v)); b = C.blp2.lp (C.blp1.lp (b)); v += juce::jmin (1.0f, band * 5.0f) * (b * (1.0f + band * 2.2f) - v); }
                else { C.bhp2.hp (C.bhp1.hp (v)); }
                // doppio cyborg ai lati
                C.dbl[(size_t) C.cpos] = v;
                if (width > 1e-5f)
                {
                    const float d = (0.009f + 0.004f * (float) std::sin (juce::MathConstants<double>::twoPi * (phD + c * 0.37))) * sr * (c == 0 ? 1.0f : 1.31f);
                    const float rp = (float) C.cpos - d; const int i0 = (int) std::floor (rp); const float fr = rp - (float) i0;
                    const float o = C.dbl[(size_t) (i0 & (kDbl - 1))] * (1 - fr) + C.dbl[(size_t) ((i0 + 1) & (kDbl - 1))] * fr;
                    v += width * 0.55f * o;
                }
                C.cpos = (C.cpos + 1) & (kComb - 1);
                y[c] = v;
            }
            // la voce cromata va in memoria (per il glitch) e nelle linee dei throw
            const float mono = 0.5f * (y[0] + y[1]);
            hist[(size_t) hpos] = mono; hpos = (hpos + 1) & (kHist - 1);
            pre[(size_t) ppos] = mono;
            // ---------------- eco a tempo: la linea si riempie sempre, ma si sente e si ripete solo quando il throw e' aperto
            float e[2];
            for (int c = 0; c < 2; ++c)
            {
                const float d = sm.dtime * (c == 0 ? 1.0f : 2.0f);
                const float rp = (float) dpos - d; const int i0 = (int) std::floor (rp); const float fr = rp - (float) i0;
                auto& b = dl[(size_t) c];
                e[c] = b[(size_t) (i0 & (kDl - 1))] * (1 - fr) + b[(size_t) ((i0 + 1) & (kDl - 1))] * fr;
                e[c] = ch[(size_t) c].echoHp.hp (ch[(size_t) c].echoLp.lp (e[c]));
            }
            const float lagged = pre[(size_t) ((ppos - (int) (0.21f * sr) + kPre) & (kPre - 1))];
            for (int c = 0; c < 2; ++c) dl[(size_t) c][(size_t) dpos] = juce::jlimit (-2.0f, 2.0f, lagged * (1.0f - gate) + (e[c] * 0.9f + e[1 - c] * 0.1f) * fb * gate);
            dpos = (dpos + 1) & (kDl - 1);
            // ---------------- spazio: il riverbero riceve la coda dell'ultima parola (pre-delay di 250 ms) solo a throw aperto
            const int pd = (int) (0.32f * sr);
            const float pv = pre[(size_t) ((ppos - pd + kPre) & (kPre - 1))]; ppos = (ppos + 1) & (kPre - 1);
            vL[(size_t) i] = (pv * gate + (e[0] + e[1]) * 0.25f * gate) * space;
            vR[(size_t) i] = 0;
            // ---------------- glitch: ripete l'ultima sillaba sempre piu' lenta e rotta
            float gl = 0;
            if (glLeft > 0 && glitch > 1e-4f)
            {
                const int pos = (int) glPos; const float w = std::sin (juce::MathConstants<float>::pi * juce::jlimit (0.0f, 1.0f, (float) glPos / (float) glLen));
                gl = hist[(size_t) ((glStart + pos) & (kHist - 1))] * w * glAmp;
                gl = std::round (gl * 64.0f) / 64.0f;
                glPos += glRate;
                if (glPos >= glLen) { glPos = 0; glAmp *= 0.78f; glRate *= 0.9; if (--glRep <= 0) glLeft = 0; }
            }
            // ---------------- uscita
            const float duck = 1.0f;
            const float oL = y[0] + e[0] * gate * eco * 1.2f * duck + gl * glitch * (glRep % 2 ? 1.0f : 0.45f);
            const float oR = y[1] + e[1] * gate * eco * 1.2f * duck + gl * glitch * (glRep % 2 ? 0.45f : 1.0f);
            L[i] = oL; R[i] = oR; og[(size_t) i] = out;
        }
        // riverbero dei throw (a blocchi), poi volume d'uscita
        if (sm.space > 1e-5f || S.space > 0) revTail = (int) (6.0f * sr); else revTail -= n;
        if (revTail > 0)
        {
            std::copy (vL.begin(), vL.begin() + n, vR.begin());
            rev.processStereo (vL.data(), vR.data(), n);
            for (int i = 0; i < n; ++i) { L[i] += hpOut[0].hp (vL[(size_t) i]) * 0.9f; R[i] += hpOut[1].hp (vR[(size_t) i]) * 0.9f; }
        }
        for (int i = 0; i < n; ++i) if (og[(size_t) i] != 1.0f) { L[i] *= og[(size_t) i]; R[i] *= og[(size_t) i]; }
        uiEnv.store (envPk); uiGate.store (gate); if (throws) uiThrows.fetch_add (throws);
    }
    void trigger()
    {   // l'ultima sillaba: finisce ~holdN fa; prendila lunga 1/16
        const int hold = (int) (0.2f * sr);
        glLen = juce::jlimit (512, (int) (0.3f * sr), (int) (sm.dtime / divBeats (3) * 0.25f));
        glStart = (hpos - hold - glLen + kHist) & (kHist - 1);
        glPos = 0; glRate = 1.0; glRep = 6; glLeft = 1; glAmp = 1.0f;
    }
};
}
