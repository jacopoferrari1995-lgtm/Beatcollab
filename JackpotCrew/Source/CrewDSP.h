#pragma once
// JACKPOT CYBERNET CREW: da una voce sola una crew che canta o grida il ritornello (rinforzi e cori grossi).
// Motore a grani sincronizzati al periodo della voce (come Choir): ogni personaggio ha la sua altezza, la sua formante
// (la "taglia" della voce, indipendente dall'altezza), il suo ritardo d'attacco in ms, la sua stonatura che vaga,
// il suo colore e il suo posto. Il bus della crew ha gridato (compressione), drive, taglio bassi/alti, delay a tempo e riverbero.
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>
#include <cmath>
#include <vector>

namespace jcrew
{
constexpr int kVoices = 6;
struct VoiceSet { bool on = false; float lvlDb = 0, pan = 0, pitch = 0, formant = 0, delayMs = 20, tone = 0; };
struct Settings
{
    std::array<VoiceSet, kVoices> v;
    float loose = 0.35f, drift = 0.4f, spread = 0.8f, shout = 0, drive = 0, verb = 0, size = 0.5f, echo = 0, echoFb = 0.35f, lowCut = 120, highCut = 16000, crewDb = 0, dryDb = 0;
    int echoDiv = 3; double bpm = 120;
};
inline const char* echoDivName (int d) { static const char* n[] = { "1/16", "1/8", "1/8.", "1/4", "1/4.", "1/2" }; return n[juce::jlimit (0, 5, d)]; }
inline double echoBeats (int d) { static const double b[] = { 0.25, 0.5, 0.75, 1.0, 1.5, 2.0 }; return b[juce::jlimit (0, 5, d)]; }
struct Character { const char* name; const char* role; float cents, pitch, formant, delayMs, pan, tone, lvl; };
inline const std::array<Character, kVoices>& characters()
{
    static const std::array<Character, kVoices> c = { {
        { "BIG MO", "il vocione: ottava sotto, petto grosso", -6, -12, -0.45f, 24, -0.35f, -0.3f, -3 },
        { "SPIKE", "il punk che urla: unisono sporco", 11, 0, 0.15f, 14, 0.6f, 0.4f, 0 },
        { "TRIDI", "la voce alta: ottava sopra", 7, 12, 0.2f, 31, 0.25f, 0.2f, -8 },
        { "ASTRO", "dal casco: unisono lontano", -14, 0, -0.1f, 42, -0.75f, 0.6f, -4 },
        { "BANDANA", "l'unisono di dietro, voce larga", 5, 0, -0.3f, 27, -0.15f, -0.1f, -2 },
        { "GRILLZ", "il basso con l'oro: quinta sotto", -9, -7, -0.35f, 19, 0.85f, -0.25f, -4 },
    } };
    return c;
}

struct Biq
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    void set (double sr, double f, bool high)
    {
        const double w = juce::MathConstants<double>::twoPi * juce::jlimit (10.0, sr * 0.45, f) / sr, c = std::cos (w), s = std::sin (w), al = s / (2 * 0.707), a0 = 1 + al;
        if (high) { b0 = (1 + c) / 2 / a0; b1 = -(1 + c) / a0; } else { b0 = (1 - c) / 2 / a0; b1 = (1 - c) / a0; }
        b2 = b0; a1 = -2 * c / a0; a2 = (1 - al) / a0;
    }
    float p (float x) { const double y = b0 * x + z1; z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y; return (float) y; }
    void reset() { z1 = z2 = 0; }
};

class Engine
{
public:
    static constexpr int kBuf = 1 << 16, kAcc = 1 << 16, kDet = 1 << 13;
    void prepare (double s, int maxBlock)
    {
        sr = s; D = (int) std::ceil (0.045 * sr);
        x.assign ((size_t) kBuf, 0.0f); xl.assign ((size_t) kBuf, 0.0f); dl.assign ((size_t) kBuf, 0.0f); dr.assign ((size_t) kBuf, 0.0f); det.assign ((size_t) kDet, 0.0f);
        for (auto& a : acc) a.assign ((size_t) kAcc, 0.0f);
        nMarks = 0; lastMark = -1; nextMark = 0; now = 0; dn = 0; hopCnt = 0; P = sr / 200.0; voiced = false; hang = 0; pendM = -100; active = false;
        lp1.set (sr, 1200, false); lp2.set (sr, 1200, false); lp1.reset(); lp2.reset();
        for (int k = 0; k < kVoices; ++k)
        {
            auto& V = vs[(size_t) k]; V = {}; V.s = (double) D; V.r = 1; V.g = 0; V.ph = 0.13 * (k + 1); V.dly = -1;
            juce::Random r (k * 977 + 13); V.seed = r.nextInt(); V.next = 0;
        }
        { const double s2 = sr * 0.5; const int Wn = (int) (0.02 * s2), tMax = (int) (s2 / 65.0); yb.assign ((size_t) (Wn + tMax + 2), 0.0f); yd.assign ((size_t) (tMax + 2), 0.0f); cmn.assign ((size_t) (tMax + 2), 1.0f); }
        for (auto* f : { &hpL, &hpR, &hp2L, &hp2R, &lpL, &lpR, &lp2L, &lp2R }) f->reset();
        lastLow = lastHigh = -1; env = 0; dryG = crewG = -1;
        juce::Reverb::Parameters p; p.roomSize = 0.5f; p.damping = 0.45f; p.width = 1; p.wetLevel = 1; p.dryLevel = 0; rv.setParameters (p); rv.setSampleRate (s); rv.reset(); lastSize = -1;
        echoL.assign ((size_t) (sr * 2.2) + 8, 0.0f); echoR.assign ((size_t) (sr * 2.2) + 8, 0.0f); ew = 0; eD = -1;
        echoLp[0].set (sr, 5000, false); echoLp[1].set (sr, 5000, false); echoHp[0].set (sr, 250, true); echoHp[1].set (sr, 250, true);
        for (auto& f : echoLp) f.reset();
        for (auto& f : echoHp) f.reset();
        const size_t mb = (size_t) juce::jmax (32, maxBlock);
        busL.assign (mb, 0.0f); busR.assign (mb, 0.0f); tL.assign (mb, 0.0f); tR.assign (mb, 0.0f);
        driveS = shoutS = verbS = echoS = fbS = -1; busTail = 0;
    }
    int latency() const { return D; }
    float meter (int k) const { return vs[(size_t) k].meter; }

    void process (float* L, float* R, int n, const Settings& S)
    {
        if ((int) busL.size() < n) { busL.resize ((size_t) n); busR.resize ((size_t) n); tL.resize ((size_t) n); tR.resize ((size_t) n); }
        bool any = false; for (auto& v : S.v) any |= v.on;
        const float kS = (float) (1.0 - std::exp (-1.0 / (0.03 * sr)));
        const float dryT = S.dryDb <= -59.9f ? 0.0f : juce::Decibels::decibelsToGain (S.dryDb), crewT = juce::Decibels::decibelsToGain (S.crewDb);
        if (dryG < 0) { dryG = dryT; crewG = crewT; driveS = S.drive; shoutS = S.shout; verbS = S.verb; echoS = S.echo; fbS = S.echoFb; }
        if (S.lowCut != lastLow) { lastLow = S.lowCut; hpL.set (sr, S.lowCut, true); hpR.set (sr, S.lowCut, true); hp2L.set (sr, S.lowCut, true); hp2R.set (sr, S.lowCut, true); }
        if (S.highCut != lastHigh) { lastHigh = S.highCut; lpL.set (sr, S.highCut, false); lpR.set (sr, S.highCut, false); lp2L.set (sr, S.highCut, false); lp2R.set (sr, S.highCut, false); }
        if (std::abs (S.size - lastSize) > 0.002f) { lastSize = S.size; auto p = rv.getParameters(); p.roomSize = 0.35f + 0.62f * S.size; p.damping = 0.6f - 0.3f * S.size; rv.setParameters (p); }
        const int eTarget = juce::jlimit (16, (int) echoL.size() - 4, (int) (sr * 60.0 / juce::jmax (40.0, S.bpm) * echoBeats (S.echoDiv)));
        if (eD < 0) eD = (float) eTarget;
        for (int i = 0; i < n; ++i)
        {
            const float inL = L[i], inR = R[i], mono = 0.5f * (inL + inR);
            const int w = (int) (now & (kBuf - 1));
            x[(size_t) w] = mono; dl[(size_t) w] = inL; dr[(size_t) w] = inR;
            const float lpv = lp2.p (lp1.p (mono)); xl[(size_t) w] = lpv;
            if ((now & 1) == 0) { det[(size_t) (dn & (kDet - 1))] = lpv; ++dn; if (++hopCnt >= 128) { hopCnt = 0; detect(); } }
            while (nextMark + 0.3 * P <= (double) now)
            {
                juce::int64 mk = (juce::int64) std::llround (nextMark);
                if (voiced)
                {
                    const juce::int64 a = (juce::int64) (nextMark - 0.3 * P), b = (juce::int64) (nextMark + 0.3 * P); float bv = -1e9f;
                    for (juce::int64 q = juce::jmax ((juce::int64) 0, a); q <= b; ++q) { const float v = xl[(size_t) (q & (kBuf - 1))]; if (v > bv) { bv = v; mk = q; } }
                }
                if (mk <= lastMark) mk = lastMark + 1;
                marks[(size_t) (nMarks & (kMarks - 1))] = mk; ++nMarks; lastMark = mk;
                nextMark = (double) mk + (voiced ? P : 0.004 * sr);
            }
            float cl = 0, cr = 0;
            if (any || active) { active = false; for (int k = 0; k < kVoices; ++k) voice (k, S, cl, cr); }
            busL[(size_t) i] = cl; busR[(size_t) i] = cr;
            ++now;
        }
        const bool busOn = any || busTail > 0;
        if (busOn)
        {
            for (int i = 0; i < n; ++i)
            {
                driveS += (S.drive - driveS) * kS; shoutS += (S.shout - shoutS) * kS;
                float l = lp2L.p (lpL.p (hp2L.p (hpL.p (busL[(size_t) i])))), r = lp2R.p (lpR.p (hp2R.p (hpR.p (busR[(size_t) i]))));
                if (shoutS > 0.0005f)
                {   // gridato: compressione veloce che porta la crew in faccia
                    const float lv = juce::jmax (std::abs (l), std::abs (r)); env += (lv - env) * (lv > env ? 0.02f : 0.0006f);
                    const float comp = 1.0f / (1.0f + shoutS * 6.0f * env), mk = 1.0f + shoutS * 1.2f;
                    l *= comp * mk; r *= comp * mk;
                }
                if (driveS > 0.0005f)
                {   // drive: saturazione morbida a volume compensato
                    const float g = 1.0f + driveS * 9.0f, nrm = 1.0f / std::tanh (g * 0.6f) * 0.6f;
                    l = l * (1 - driveS) + std::tanh (l * g * 0.6f) * nrm * driveS; r = r * (1 - driveS) + std::tanh (r * g * 0.6f) * nrm * driveS;
                }
                busL[(size_t) i] = l; busR[(size_t) i] = r;
            }
            // delay a tempo, ping-pong, filtrato
            const int EN = (int) echoL.size();
            for (int i = 0; i < n; ++i)
            {
                echoS += (S.echo - echoS) * kS; fbS += (S.echoFb - fbS) * kS; eD += ((float) eTarget - eD) * 0.0004f;
                float rp = (float) ew - eD; while (rp < 0) rp += (float) EN;
                const int i0 = (int) rp % EN; const float t = rp - std::floor (rp); const int i1 = (i0 + 1) % EN;
                const float el = echoL[(size_t) i0] + (echoL[(size_t) i1] - echoL[(size_t) i0]) * t, er = echoR[(size_t) i0] + (echoR[(size_t) i1] - echoR[(size_t) i0]) * t;
                const float inE = 0.5f * (busL[(size_t) i] + busR[(size_t) i]) * juce::jmin (1.0f, echoS * 50.0f);
                echoL[(size_t) ew] = echoHp[0].p (echoLp[0].p (inE + er * fbS)); echoR[(size_t) ew] = echoHp[1].p (echoLp[1].p (el * fbS));
                ew = (ew + 1) % EN;
                tL[(size_t) i] = el * echoS * 1.3f; tR[(size_t) i] = er * echoS * 1.3f;
            }
            for (int i = 0; i < n; ++i) { busL[(size_t) i] += tL[(size_t) i]; busR[(size_t) i] += tR[(size_t) i]; }
            std::copy (busL.begin(), busL.begin() + n, tL.begin()); std::copy (busR.begin(), busR.begin() + n, tR.begin());
            rv.processStereo (tL.data(), tR.data(), n);
            for (int i = 0; i < n; ++i) { verbS += (S.verb - verbS) * kS; const float vg = verbS * 0.8f * (1.0f - 0.45f * lastSize); busL[(size_t) i] += tL[(size_t) i] * vg; busR[(size_t) i] += tR[(size_t) i] * vg; }
            busTail = any ? (int) (sr * 6) : juce::jmax (0, busTail - n);
        }
        // uscita: la tua voce (in ritardo della latenza dichiarata, cosi' resta allineata) + la crew
        const juce::int64 t0 = now - n;
        for (int i = 0; i < n; ++i)
        {
            dryG += (dryT - dryG) * kS; crewG += (crewT - crewG) * kS;
            const int rdx = (int) ((t0 + i - D) & (kBuf - 1));
            L[i] = dl[(size_t) rdx] * dryG + (busOn ? busL[(size_t) i] * crewG : 0.0f);
            R[i] = dr[(size_t) rdx] * dryG + (busOn ? busR[(size_t) i] * crewG : 0.0f);
        }
    }

private:
    struct V
    {
        double s = 0, ph = 0; float r = 1, g = 0, dly = -1, fr = 1, wob = 0, target = 0, lp = 0, hz = 0, meter = 0, panL = 0.707f, panR = 0.707f;
        int next = 0, seed = 1;
    };
    double sr = 48000; int D = 2048; juce::int64 now = 0, dn = 0; int hopCnt = 0;
    std::vector<float> x, xl, dl, dr, det;
    static constexpr int kMarks = 2048; juce::int64 marks[kMarks] {}; juce::int64 nMarks = 0, lastMark = -1; double nextMark = 0;
    std::array<std::vector<float>, kVoices> acc;
    std::array<V, kVoices> vs;
    std::vector<float> yb, yd, cmn;
    double P = 240; bool voiced = false, active = false; int hang = 0; double m0 = 60, pendM = -100;
    Biq lp1, lp2, hpL, hpR, hp2L, hp2R, lpL, lpR, lp2L, lp2R, echoLp[2], echoHp[2];
    float lastLow = -1, lastHigh = -1, lastSize = -1, env = 0, dryG = -1, crewG = -1, driveS = -1, shoutS = -1, verbS = -1, echoS = -1, fbS = -1, eD = -1;
    juce::Reverb rv; std::vector<float> echoL, echoR, busL, busR, tL, tR; int ew = 0, busTail = 0;

    juce::int64 nearestMark (double c) const
    {
        juce::int64 best = (juce::int64) std::llround (c); double bd = 1e18;
        for (juce::int64 k = nMarks - 1; k >= 0 && k >= nMarks - kMarks; --k) { const double d = std::abs ((double) marks[(size_t) (k & (kMarks - 1))] - c); if (d < bd) { bd = d; best = marks[(size_t) (k & (kMarks - 1))]; } else if ((double) marks[(size_t) (k & (kMarks - 1))] < c) break; }
        return best;
    }
    float rd (const std::vector<float>& b, double pos) const
    {
        const double fl = std::floor (pos); const float t = (float) (pos - fl); const juce::int64 i0 = (juce::int64) fl;
        const float a = b[(size_t) (i0 & (kBuf - 1))], c = b[(size_t) ((i0 + 1) & (kBuf - 1))];
        return a + t * (c - a);
    }
    void detect()
    {
        const double s2 = sr * 0.5; const int Wn = (int) (0.02 * s2), tMin = (int) (s2 / 1000.0), tMax = (int) (s2 / 65.0);
        const juce::int64 st = dn - (juce::int64) yb.size(); double e = 0;
        for (size_t j = 0; j < yb.size(); ++j) yb[j] = det[(size_t) ((st + (juce::int64) j) & (kDet - 1))];
        for (int j = 0; j < Wn; ++j) e += (double) yb[(size_t) j] * yb[(size_t) j];
        const float rms = (float) std::sqrt (e / Wn);
        for (int t = 1; t <= tMax; ++t) { float s = 0; for (int j = 0; j < Wn; ++j) { const float q = yb[(size_t) j] - yb[(size_t) (j + t)]; s += q * q; } yd[(size_t) t] = s; }
        float run = 0; int best = -1; float bv = 1e9f;
        for (int t = 1; t <= tMax; ++t) { run += yd[(size_t) t]; cmn[(size_t) t] = run > 0 ? yd[(size_t) t] * t / run : 1.0f; }
        for (int t = tMin; t < tMax; ++t)
        {
            if (cmn[(size_t) t] < 0.15f) { while (t + 1 < tMax && cmn[(size_t) (t + 1)] < cmn[(size_t) t]) ++t; best = t; bv = cmn[(size_t) t]; break; }
            if (cmn[(size_t) t] < bv) { bv = cmn[(size_t) t]; best = t; }
        }
        if (best > 0 && bv < 0.3f && rms > 0.0015f)
        {
            double tt = best;
            if (best > 1 && best < tMax) { const double a = cmn[(size_t) best - 1], b = cmn[(size_t) best], c = cmn[(size_t) best + 1]; const double den = a - 2 * b + c; if (std::abs (den) > 1e-9) tt += 0.5 * (a - c) / den; }
            const double f = s2 / tt, Pn = sr / f, m = 69.0 + 12.0 * std::log2 (f / 440.0);
            if (voiced && std::abs (m - m0) > 6.5 && std::abs (m - pendM) > 1.0) { pendM = m; return; }
            pendM = -100;
            P = std::abs (Pn - P) / P > 0.12 ? Pn : P * 0.4 + Pn * 0.6;
            m0 = voiced ? m0 * 0.35 + m * 0.65 : m; voiced = true; hang = 4;
        }
        else if (hang > 0) --hang;
        else { voiced = false; P = sr / 200.0; }
    }
    void voice (int k, const Settings& S, float& outL, float& outR)
    {
        auto& Vv = vs[(size_t) k]; const auto& VS = S.v[(size_t) k]; const auto& ch = characters()[(size_t) k];
        auto& A = acc[(size_t) k]; const int ai = (int) (now & (kAcc - 1));
        const float y = A[(size_t) ai]; A[(size_t) ai] = 0;
        const bool on = VS.on;
        // fuori tempo: ogni ~150 ms un nuovo scarto, raggiunto morbido (mai tutti insieme)
        if (--Vv.next <= 0) { juce::Random r (Vv.seed += 7919); Vv.target = (r.nextFloat() * 2 - 1) * S.loose * (float) sr * 0.022f; Vv.next = (int) (sr * (0.12 + 0.1 * r.nextFloat())); }
        Vv.wob += (Vv.target - Vv.wob) * (float) (1.0 / (sr * 0.12));
        Vv.ph += (0.15 + 0.05 * k) / sr; if (Vv.ph > 1) Vv.ph -= 1;
        const float sw = (float) std::sin (juce::MathConstants<double>::twoPi * Vv.ph);
        const float dT = juce::jmax (0.0f, (float) sr * VS.delayMs * 0.001f + Vv.wob);
        if (Vv.dly < 0) Vv.dly = dT;
        Vv.dly += (dT - Vv.dly) * 0.0004f;
        const float cents = ch.cents * S.drift * 1.6f + sw * 5.0f * S.drift;
        const float gT = on ? juce::Decibels::decibelsToGain (VS.lvlDb) * (voiced ? 1.0f : 0.85f) : 0.0f;
        Vv.g += (gT - Vv.g) * (float) (1.0 - std::exp (-1.0 / (0.02 * sr)));
        const float pan = juce::jlimit (-1.0f, 1.0f, VS.pan * S.spread), a = (pan + 1.0f) * 0.25f * juce::MathConstants<float>::pi;
        Vv.panL += (std::cos (a) - Vv.panL) * 0.001f; Vv.panR += (std::sin (a) - Vv.panR) * 0.001f;
        Vv.fr = std::pow (2.0f, VS.formant * 0.5f);   // formante su: grano letto piu' in fretta (voce piu' piccola); giu': piu' grossa
        if (on || Vv.g > 1e-4f) active = true;
        int guard = 0;
        while ((double) now >= Vv.s + Vv.dly - (voiced ? P : 0.004 * sr) && guard++ < 8)
        {
            const double Pg = voiced ? P : 0.004 * sr;
            const float rT = voiced ? std::pow (2.0f, (VS.pitch + cents * 0.01f) / 12.0f) : 1.0f;
            Vv.r += (rT - Vv.r) * (float) (1.0 - std::exp (-Pg / (0.03 * sr)));
            double c = Vv.s - D;
            if (voiced) c = (double) nearestMark (c);
            const double w0 = Vv.s + Vv.dly; const int half = (int) Pg; const float fr = voiced ? Vv.fr : 1.0f;
            if (Vv.g > 1e-5f || on)
                for (int j = -half; j <= half; ++j)
                {
                    const float win = 0.5f + 0.5f * std::cos (juce::MathConstants<float>::pi * (float) j / (float) (half + 1));
                    const juce::int64 wp = (juce::int64) std::llround (w0) + j; if (wp < now) continue;
                    A[(size_t) (wp & (kAcc - 1))] += win * rd (x, c + j * fr);
                }
            Vv.s += voiced ? Pg / juce::jmax (0.25f, Vv.r) : Pg;
        }
        if (guard >= 8) Vv.s = (double) now;
        float s = y * Vv.g;
        if (VS.tone < -0.01f) { const float lpA = (float) std::exp (-juce::MathConstants<double>::twoPi * (18000.0 * std::pow (0.08, (double) -VS.tone)) / sr); Vv.lp = s + lpA * (Vv.lp - s); s = Vv.lp; }
        else Vv.lp = s;
        if (VS.tone > 0.01f) { const float hpA = (float) std::exp (-juce::MathConstants<double>::twoPi * (40.0 + 900.0 * VS.tone) / sr); const float h = s - Vv.hz; Vv.hz = s + hpA * (Vv.hz - s); s = h * 1.15f + s * 0.15f; }
        else Vv.hz = s;
        Vv.meter = juce::jmax (std::abs (s), Vv.meter * 0.9995f);
        outL += s * Vv.panL * 1.2f; outR += s * Vv.panR * 1.2f;
    }
};
}
