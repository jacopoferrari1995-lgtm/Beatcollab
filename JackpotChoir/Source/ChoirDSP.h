#pragma once
// JACKPOT CHOIR: cori e armonie dalla voce registrata.
// La voce viene ascoltata (YIN su un segnale dimezzato) e ricostruita a grani lunghi un periodo (PSOLA): ogni voce del coro
// rimette i grani piu' vicini o piu' lontani, quindi cambia la nota ma non le formanti (niente effetto scoiattolo).
// SCALA: ogni voce sta N gradi sopra o sotto nella scala scelta e segue la melodia. MIDI: le voci cantano le note tenute
// sulla tastiera, con il vibrato naturale del cantante. Umano: ogni voce ha il suo ritardo, la sua intonazione che vaga,
// il suo livello che respira. Con tutte le voci spente esce la voce asciutta, solo ritardata della latenza.
#include "JackpotKit.h"
#include <array>

namespace jcho
{
enum Interval { I_OFF, I_UNI, I_3U, I_4U, I_5U, I_6U, I_8U, I_10U, I_2U, I_3D, I_4D, I_5D, I_6D, I_8D, kIntervals };
inline const char* intervalShort (int i) { static const char* n[kIntervals] = { "OFF", "DOPPIA", "+3", "+4", "+5", "+6", "+8", "+10", "+2", "-3", "-4", "-5", "-6", "-8" }; return n[juce::jlimit (0, kIntervals - 1, i)]; }
inline const char* intervalName (int i)
{
    static const char* n[kIntervals] = { "SPENTA", "DOPPIA (UNISONO)", "TERZA SOPRA", "QUARTA SOPRA", "QUINTA SOPRA", "SESTA SOPRA", "OTTAVA SOPRA", "DECIMA SOPRA", "SECONDA SOPRA",
                                         "TERZA SOTTO", "QUARTA SOTTO", "QUINTA SOTTO", "SESTA SOTTO", "OTTAVA SOTTO" };
    return n[juce::jlimit (0, kIntervals - 1, i)];
}
inline int intervalDeg (int i) { static const int d[kIntervals] = { 0, 0, 2, 3, 4, 5, 7, 9, 1, -2, -3, -4, -5, -7 }; return d[juce::jlimit (0, kIntervals - 1, i)]; }

enum Scale { MAGGIORE, MINORE, DORICA, FRIGIA, MISOLIDIA, ARMONICA, PENTA_MIN, PENTA_MAG, kScales };
inline const char* scaleName (int s) { static const char* n[kScales] = { "MAGGIORE", "MINORE", "DORICA", "FRIGIA", "MISOLIDIA", "ARMONICA", "PENTA MIN", "PENTA MAG" }; return n[juce::jlimit (0, kScales - 1, s)]; }
inline std::vector<int> scaleSteps (int s)
{
    switch (s)
    {
        case MINORE: return { 0, 2, 3, 5, 7, 8, 10 };
        case DORICA: return { 0, 2, 3, 5, 7, 9, 10 };
        case FRIGIA: return { 0, 1, 3, 5, 7, 8, 10 };
        case MISOLIDIA: return { 0, 2, 4, 5, 7, 9, 10 };
        case ARMONICA: return { 0, 2, 3, 5, 7, 8, 11 };
        case PENTA_MIN: return { 0, 3, 5, 7, 10 };
        case PENTA_MAG: return { 0, 2, 4, 7, 9 };
        default: return { 0, 2, 4, 5, 7, 9, 11 };
    }
}
inline const char* noteName (int pc) { static const char* n[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }; return n[((pc % 12) + 12) % 12]; }

struct VoiceSet { int interval = I_OFF; float levelDb = 0, pan = 0, formant = 0; };
struct Settings
{
    int mode = 0, key = 0, scale = MAGGIORE;      // mode 0 SCALA, 1 MIDI
    std::array<VoiceSet, 4> v {};
    float dryDb = 0, choirDb = 0, human = 0.5f, width = 0.7f, cons = 0.35f, lowCut = 120, tone = 0, glide = 0.3f, outDb = 0;
};

// nota di una scala estesa: grado (anche negativo o oltre l'ottava) -> semitoni dalla tonica
struct ScaleMap
{
    std::vector<int> st; int key = 0;
    void set (int k, int s) { key = k; st = scaleSteps (s); }
    int n() const { return (int) st.size(); }
    int noteOfDeg (int d) const { const int N = n(); const int o = (int) std::floor ((double) d / N); return key + st[(size_t) (d - o * N)] + 12 * o; }
    // grado della nota della scala piu' vicina a una nota MIDI (con decimali)
    int nearestDeg (double m) const
    {
        const int N = n(); const int base = (int) std::floor ((m - key) / 12.0) * N; int best = base; double bd = 1e9;
        for (int d = base - N; d <= base + 2 * N; ++d) { const double dd = std::abs (noteOfDeg (d) - m); if (dd < bd) { bd = dd; best = d; } }
        return best;
    }
};

class Engine
{
public:
    static constexpr int kV = 4, kBuf = 1 << 15, kAcc = 1 << 15, kDet = 1 << 13, kTrace = 256;
    void prepare (double s, int)
    {
        sr = s; D = (int) std::ceil (0.045 * sr);
        std::fill (std::begin (x), std::end (x), 0.0f); std::fill (std::begin (dl), std::end (dl), 0.0f); std::fill (std::begin (dr), std::end (dr), 0.0f);
        std::fill (std::begin (det), std::end (det), 0.0f); std::fill (std::begin (xl), std::end (xl), 0.0f); nMarks = 0; lastMark = -1; nextMark = 0;
        for (auto& a : acc) for (auto& z : a) z = 0;
        now = 0; dn = 0; hopCnt = 0; P = sr / 200.0; voiced = false; hang = 0; m0 = 60; m0slow = 60; conf = 0; anchor = 0;
        lp1.lp (sr, 1200); lp2.lp (sr, 1200); lp1.reset(); lp2.reset();
        for (int k = 0; k < kV; ++k) { auto& V = vs[k]; V = {}; V.s = (double) D; V.r = 1; V.g = 0; V.ph1 = 0.13 * (k + 1); V.ph2 = 0.37 * (k + 1); }
        for (auto& b : busF) b.reset();
        last = {}; lastKey = -1; trW = 0; first2 = true; first = true;
        { const double s2 = sr * 0.5; const int Wn = (int) (0.02 * s2), tMax = (int) (s2 / 65.0); buf.assign ((size_t) (Wn + tMax + 2), 0.0f); d.assign ((size_t) (tMax + 2), 0.0f); cmn.assign ((size_t) (tMax + 2), 1.0f); }
        for (auto& t : trLead) t = -1; for (auto& tv : trV) for (auto& t : tv) t = -1;
    }
    int latency() const { return D; }
    // note tenute (modo MIDI), dal processore
    std::array<int, 4> held { -1, -1, -1, -1 }; int nHeld = 0;

    void process (float* L, float* R, int n, const Settings& S)
    {
        if (S.key * 31 + S.scale != lastKey) { lastKey = S.key * 31 + S.scale; map.set (S.key, S.scale); }
        bool any = false; for (auto& v : S.v) any |= v.interval != I_OFF;
        const float kS = (float) (1.0 - std::exp (-1.0 / (0.02 * sr)));
        const float dryT = S.dryDb <= -59.9f ? 0.0f : juce::Decibels::decibelsToGain (S.dryDb), choirT = juce::Decibels::decibelsToGain (S.choirDb), outT = juce::Decibels::decibelsToGain (S.outDb);
        if (first) { dryG = dryT; choirG = choirT; outG = outT; first = false; }
        updateBus (S);
        for (int i = 0; i < n; ++i)
        {
            const float inL = L[i], inR = R[i], mono = 0.5f * (inL + inR);
            const int w = (int) (now & (kBuf - 1));
            x[w] = mono; dl[w] = inL; dr[w] = inR;
            // rilevatore a meta' frequenza
            const float lpv = lp2.p (lp1.p (mono)); xl[w] = lpv;
            if ((now & 1) == 0) { det[dn & (kDet - 1)] = lpv; ++dn; if (++hopCnt >= 128) { hopCnt = 0; detect (S); } }
            // segni d'analisi: uno per periodo, messo sul colpo della voce (il massimo del segnale filtrato) cosi' i grani sono allineati
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
            if (any || active)
            {
                active = false;
                for (int k = 0; k < kV; ++k) voice (k, S, cl, cr);
            }
            const int rd = (int) ((now - D) & (kBuf - 1));
            dryG += (dryT - dryG) * kS; choirG += (choirT - choirG) * kS; outG += (outT - outG) * kS;
            float oL = dl[rd] * dryG, oR = dr[rd] * dryG;
            if (cl != 0.0f || cr != 0.0f)
            {
                cl = busF[1].p (busF[0].p (cl)); cr = busF[3].p (busF[2].p (cr));
                cl = busF[5].p (busF[4].p (cl)); cr = busF[7].p (busF[6].p (cr));
                oL += cl * choirG; oR += cr * choirG;
            }
            else if (any) { for (int f = 0; f < 8; f += 2) { busF[f].p (0.0f); busF[f + 1].p (0.0f); } }
            L[i] = oL * outG; R[i] = oR * outG;
            ++now;
        }
    }

    // per la grafica
    std::atomic<float> uiNote { -1 }, uiConf { 0 }; std::array<std::atomic<float>, kV> uiVoice {};
    float trLead[kTrace] {}; float trV[kV][kTrace] {}; std::atomic<int> trW { 0 };

private:
    struct VoiceState { double s = 0; float r = 1, g = 0, dly = 0, pan = 0, fr = 1; double ph1 = 0, ph2 = 0; float semis = 0; bool on = false; };
    double sr = 48000; int D = 2048; juce::int64 now = 0, dn = 0; int hopCnt = 0;
    float x[kBuf] {}, xl[kBuf] {}, dl[kBuf] {}, dr[kBuf] {}, det[kDet] {};
    static constexpr int kMarks = 2048; juce::int64 marks[kMarks] {}; juce::int64 nMarks = 0, lastMark = -1; double nextMark = 0;
    juce::int64 nearestMark (double c) const
    {
        juce::int64 best = (juce::int64) std::llround (c); double bd = 1e18;
        for (juce::int64 k = nMarks - 1; k >= 0 && k >= nMarks - kMarks; --k) { const double d = std::abs ((double) marks[(size_t) (k & (kMarks - 1))] - c); if (d < bd) { bd = d; best = marks[(size_t) (k & (kMarks - 1))]; } else if ((double) marks[(size_t) (k & (kMarks - 1))] < c) break; }
        return best;
    }
    float acc[kV][kAcc] {};
    VoiceState vs[kV];
    std::vector<float> buf, d, cmn;
    double P = 240, anchor = 0; bool voiced = false; int hang = 0; double m0 = 60, m0slow = 60, pendM = -100; float conf = 0;
    jkit::Bq lp1, lp2, busF[8]; ScaleMap map; int lastKey = -1; Settings last; bool first = true, active = false;
    float dryG = 1, choirG = 1, outG = 1;

    void updateBus (const Settings& S)
    {
        if (S.lowCut == last.lowCut && S.tone == last.tone && ! first2) return;
        first2 = false; last.lowCut = S.lowCut; last.tone = S.tone;
        for (int c = 0; c < 2; ++c)
        {
            busF[c * 2].hp (sr, S.lowCut, 0.7071); busF[c * 2 + 1].hp (sr, S.lowCut, 0.7071);
            busF[4 + c * 2].lowShelf (sr, 250, -S.tone * 5.0); busF[4 + c * 2 + 1].highShelf (sr, 4500, S.tone * 6.0);
        }
    }
    bool first2 = true;

    float rd (const float* buf, double pos) const
    {
        const double fl = std::floor (pos); const float t = (float) (pos - fl); const juce::int64 i0 = (juce::int64) fl;
        const float a = buf[(size_t) (i0 & (kBuf - 1))], b = buf[(size_t) ((i0 + 1) & (kBuf - 1))];
        return a + t * (b - a);
    }
    // YIN sull'ultimo pezzo del segnale dimezzato (sr/2)
    void detect (const Settings&)
    {
        const double s2 = sr * 0.5; const int Wn = (int) (0.02 * s2), tMin = (int) (s2 / 1000.0), tMax = (int) (s2 / 65.0);
        if ((int) buf.size() != Wn + tMax + 2) { buf.assign ((size_t) (Wn + tMax + 2), 0.0f); d.assign ((size_t) (tMax + 2), 0.0f); cmn.assign ((size_t) (tMax + 2), 1.0f); }
        const juce::int64 st = dn - (juce::int64) buf.size(); double e = 0;
        for (size_t j = 0; j < buf.size(); ++j) { buf[j] = det[(size_t) ((st + (juce::int64) j) & (kDet - 1))]; }
        for (int j = 0; j < Wn; ++j) e += (double) buf[(size_t) j] * buf[(size_t) j];
        const float rms = (float) std::sqrt (e / Wn);
        for (int t = 1; t <= tMax; ++t) { float s = 0; for (int j = 0; j < Wn; ++j) { const float q = buf[(size_t) j] - buf[(size_t) (j + t)]; s += q * q; } d[(size_t) t] = s; }
        float run = 0; int best = -1; float bv = 1e9f;
        for (int t = 1; t <= tMax; ++t) { run += d[(size_t) t]; cmn[(size_t) t] = run > 0 ? d[(size_t) t] * t / run : 1.0f; }
        for (int t = tMin; t < tMax; ++t)
        {
            if (cmn[(size_t) t] < 0.15f) { while (t + 1 < tMax && cmn[(size_t) (t + 1)] < cmn[(size_t) t]) ++t; best = t; bv = cmn[(size_t) t]; break; }
            if (cmn[(size_t) t] < bv) { bv = cmn[(size_t) t]; best = t; }
        }
        const bool v = best > 0 && bv < 0.3f && rms > 0.0015f;
        if (v)
        {
            double tt = best;
            if (best > 1 && best < tMax) { const double a = cmn[(size_t) best - 1], b = cmn[(size_t) best], c = cmn[(size_t) best + 1]; const double den = a - 2 * b + c; if (std::abs (den) > 1e-9) tt += 0.5 * (a - c) / den; }
            const double f = s2 / tt, Pn = sr / f;
            const double m = 69.0 + 12.0 * std::log2 (f / 440.0);
            // salti di oltre mezza ottava: veri solo se confermati al giro dopo (niente errori d'ottava di un attimo)
            if (voiced && std::abs (m - m0) > 6.5 && std::abs (m - pendM) > 1.0) { pendM = m; return; }
            pendM = -100;
            P = std::abs (Pn - P) / P > 0.12 ? Pn : P * 0.4 + Pn * 0.6;
            m0 = voiced ? m0 * 0.35 + m * 0.65 : m; m0slow = voiced ? m0slow * 0.85 + m * 0.15 : m;
            voiced = true; hang = 4; conf = 1.0f - bv;
        }
        else if (hang > 0) --hang;
        else { voiced = false; conf = 0; P = sr / 200.0; }
        uiNote = voiced ? (float) m0 : -1.0f; uiConf = conf;
        const int tw = trW.load(); trLead[tw] = voiced ? (float) m0 : -1.0f;
        for (int k = 0; k < kV; ++k) trV[k][tw] = vs[k].on && voiced ? (float) (m0 + vs[k].semis) : -1.0f;
        trW = (tw + 1) % kTrace;
    }
    float targetSemis (int k, const Settings& S, bool& on) const
    {
        const auto& V = S.v[(size_t) k]; on = V.interval != I_OFF;
        if (! on) return 0;
        if (S.mode == 1)
        {   // MIDI: la voce k canta la k-esima nota tenuta (dal basso), col vibrato del cantante
            if (k >= nHeld || held[(size_t) k] < 0) { on = false; return 0; }
            return (float) (held[(size_t) k] - m0slow);
        }
        if (V.interval == I_UNI) return 0;
        const int q = map.nearestDeg (m0);
        return (float) (map.noteOfDeg (q + intervalDeg (V.interval)) - map.noteOfDeg (q));
    }
    void voice (int k, const Settings& S, float& outL, float& outR)
    {
        auto& V = vs[k]; const auto& VS = S.v[(size_t) k];
        float* A = acc[k]; const int ai = (int) (now & (kAcc - 1));
        const float y = A[ai]; A[ai] = 0;
        bool on = false; const float semis = targetSemis (k, S, on);
        // umano: ogni voce ha il suo ritardo, la sua stonatura che vaga piano e il suo respiro
        const float hu = S.human;
        static const float baseD[4] = { 0.011f, 0.018f, 0.024f, 0.031f }, baseC[4] = { 7.0f, -9.0f, 5.0f, -6.0f };
        V.ph1 += (0.17 + 0.05 * k) / sr; V.ph2 += (0.29 + 0.07 * k) / sr; if (V.ph1 > 1) V.ph1 -= 1; if (V.ph2 > 1) V.ph2 -= 1;
        const float wob = (float) std::sin (juce::MathConstants<double>::twoPi * V.ph1), wob2 = (float) std::sin (juce::MathConstants<double>::twoPi * V.ph2);
        const bool uni = VS.interval == I_UNI && S.mode == 0;
        const float cents = (uni ? baseC[k] * 1.4f : baseC[k] * 0.6f) * hu + wob * 4.0f * hu;
        const float dlyT = (float) sr * (baseD[k] * hu * (uni ? 1.3f : 1.0f) + 0.003f * hu * wob2);
        V.dly += (dlyT - V.dly) * 0.0005f;
        const float gT = (on ? juce::Decibels::decibelsToGain (VS.levelDb) * (1.0f + 0.08f * hu * wob2) : 0.0f) * (voiced ? 1.0f : S.cons);
        V.g += (gT - V.g) * (float) (1.0 - std::exp (-1.0 / (0.025 * sr)));
        V.pan = VS.pan * S.width; V.fr = std::pow (2.0f, VS.formant * 0.3f);
        V.semis = semis; V.on = on;
        if (on || V.g > 1e-4f) active = true;
        // grani: quando l'uscita arriva all'inizio del prossimo grano, lo si scrive tutto
        int guard = 0;
        while ((double) now >= V.s + V.dly - (voiced ? P : 0.004 * sr) && guard++ < 8)
        {
            const double Pg = voiced ? P : 0.004 * sr;
            const float rT = voiced && on ? std::pow (2.0f, (semis + cents * 0.01f) / 12.0f) : 1.0f;
            const float ga = (float) (1.0 - std::exp (-Pg / ((0.02 + 0.18 * S.glide) * sr)));
            V.r += (rT - V.r) * ga;
            double c = V.s - D;
            if (voiced) c = (double) nearestMark (c);
            const double w0 = V.s + V.dly; const int half = (int) Pg; const float fr = voiced ? V.fr : 1.0f;
            if (V.g > 1e-5f || on)
                for (int j = -half; j <= half; ++j)
                {
                    const float win = 0.5f + 0.5f * std::cos (juce::MathConstants<float>::pi * (float) j / (float) (half + 1));
                    const juce::int64 wp = (juce::int64) std::llround (w0) + j; if (wp < now) continue;
                    A[(size_t) (wp & (kAcc - 1))] += win * rd (x, c + j * fr);
                }
            V.s += voiced ? Pg / juce::jmax (0.25f, V.r) : Pg;
        }
        if (guard >= 8) V.s = (double) now;   // non deve mai restare indietro
        const float s = y * V.g;
        const float a = (V.pan + 1.0f) * 0.25f * juce::MathConstants<float>::pi;
        outL += s * std::cos (a) * 1.4142f; outR += s * std::sin (a) * 1.4142f;
    }
};
}
