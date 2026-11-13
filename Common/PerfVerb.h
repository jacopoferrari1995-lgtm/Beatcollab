#pragma once
// Riverbero degli effetti da performance: rete di ritardi con retroazione (FDN a 8 linee), diffusione in ingresso,
// coda modulata, prime riflessioni per le stanze e una molla vera (catena di allpass dispersivi dentro un anello).
// Tipi tarati sul comportamento dei modelli classici: HALL (sala grande, densa, coda lunga e morbida), ROOM (stanza,
// prime riflessioni forti), PLATE (piastra: diffusione immediata, brillante, niente riflessioni), SPRING (molla: "boing",
// dispersione e banda limitata), CHAMBER (camera d'eco: densa e calda), AMBIENCE (solo spazio, coda cortissima).
#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>
#include <vector>

namespace pfx
{
class Verb
{
public:
    enum { HALL, ROOM, PLATE, SPRING, CHAMBER, AMBIENCE, kTypes };
    static const char* typeName (int t) { static const char* n[kTypes] = { "HALL", "ROOM", "PLATE", "SPRING", "CHAMBER", "AMBIENCE" }; return n[juce::jlimit (0, kTypes - 1, t)]; }

    // tempo e pre-delay tipici di ogni tipo: quando si cambia TIPO si parte da qui (come i preset dei riverberi hardware)
    static void typeDefaults (int t, float& decaySec, float& preMs)
    {
        static const float d[kTypes] = { 2.8f, 0.9f, 1.8f, 2.0f, 1.4f, 0.6f }, p[kTypes] = { 25, 6, 10, 0, 14, 0 };
        const int i = juce::jlimit (0, kTypes - 1, t); decaySec = d[i]; preMs = p[i];
    }
    struct Params { int type = HALL; float decay = 2.5f, preMs = 0, lowHz = 0, highHz = 0, color = 0.5f, rate = 0.4f, depth = 0.35f, width = 1.0f; };

    void prepare (double s)
    {
        sr = s; const double k = sr / 44100.0;
        const int maxLine = (int) (0.16 * 1.5 * sr) + 64;
        for (auto& l : line) l.assign ((size_t) maxLine, 0.0f);
        for (int c = 0; c < 2; ++c) for (int i = 0; i < 4; ++i) { const int n = (int) (kApLen[c][i] * k) + 1; ap[c][i].assign ((size_t) n, 0.0f); apW[c][i] = 0; }
        pre.assign ((size_t) (0.26 * sr) + 4, 0.0f); preW = 0;
        er.assign ((size_t) (0.09 * sr) + 4, 0.0f); erW = 0;
        for (int c = 0; c < 2; ++c) { spLine[c].assign ((size_t) (0.06 * sr) + 4, 0.0f); spW[c] = 0; for (auto& d : spAp[c]) d = 0; spHp[c] = spLp[c] = 0; }
        lastType = -1; reset();
    }
    void reset()
    {
        for (auto& l : line) std::fill (l.begin(), l.end(), 0.0f);
        for (auto& c : ap) for (auto& a : c) std::fill (a.begin(), a.end(), 0.0f);
        std::fill (pre.begin(), pre.end(), 0.0f); std::fill (er.begin(), er.end(), 0.0f);
        for (auto& s2 : spLine) std::fill (s2.begin(), s2.end(), 0.0f);
        for (auto& d : damp) d = 0; for (auto& x : lc) x = 0; for (auto& x : hc) x = 0; for (auto& x : hc2) x = 0; for (auto& x : tl) x = 0;
        w = 0; lfoC = 1; lfoS = 0;
    }
    // in: L/R originali; out: solo la coda (wet), da sommare fuori
    void process (const float* inL, const float* inR, float* outL, float* outR, int n, const Params& P)
    {
        if (P.type != lastType) { lastType = P.type; setType (P.type); }
        const double k = sr / 44100.0;
        // guadagni di linea dal tempo di decadimento (RT60): ogni linea perde 60 dB in "decay" secondi
        float T = juce::jlimit (0.1f, 30.0f, P.decay * decayK);
        if (type == SPRING) T = juce::jmin (12.0f, 1.0f / juce::jmax (0.02f, 1.0f / T - 0.15f));   // la molla perde energia nei filtri: si compensa
        float g[8], len[8];
        for (int i = 0; i < 8; ++i) { len[i] = (float) (kLen[i] * size * k); g[i] = (float) std::pow (10.0, -3.0 * len[i] / (T * sr)); }
        // colore della coda: quanto in fretta muoiono gli alti rispetto ai bassi (0 scuro, 1 brillante)
        const float dA = juce::jlimit (0.0f, 0.92f, (1.0f - P.color) * 0.85f * dampK + (type == PLATE ? -0.12f : type == HALL ? 0.08f : 0.0f));
        const int preN = juce::jlimit (0, (int) pre.size() - 2, (int) (P.preMs * 0.001 * sr));
        const float tlA = tlHz > 1 ? (float) std::exp (-juce::MathConstants<double>::twoPi * tlHz / sr) : -1.0f;
        const float lcA = P.lowHz > 1 ? (float) std::exp (-juce::MathConstants<double>::twoPi * P.lowHz / sr) : -1.0f;
        const float hcA = P.highHz > 1 ? (float) std::exp (-juce::MathConstants<double>::twoPi * P.highHz / sr) : -1.0f;
        const float modAmt = P.depth * modK * (float) k * size;    // in campioni
        const float dw = (float) (juce::MathConstants<double>::twoPi * P.rate / sr), cw = std::cos (dw), sw = std::sin (dw);
        const int LN = (int) line[0].size(), PN = (int) pre.size(), EN = (int) er.size();
        const float wid = juce::jlimit (0.0f, 1.5f, P.width), oc = outCal[type];
        for (int s = 0; s < n; ++s)
        {
            // pre-delay
            pre[(size_t) preW] = 0.5f * (inL[s] + inR[s]); const float side = 0.5f * (inL[s] - inR[s]);
            int rp = preW - preN; if (rp < 0) rp += PN; const float x = pre[(size_t) rp]; if (++preW >= PN) preW = 0;
            // prime riflessioni (stanze)
            float eL = 0, eR = 0;
            if (erN > 0)
            {
                er[(size_t) erW] = x;
                for (int t = 0; t < erN; ++t) { int q = erW - erTap[t]; if (q < 0) q += EN; const float v = er[(size_t) q] * erG[t]; if (t & 1) eR += v; else eL += v; }
                if (++erW >= EN) erW = 0;
            }
            // diffusione in ingresso: 4 allpass per lato
            float d[2] = { x + side * 0.3f, x - side * 0.3f };
            for (int c = 0; c < 2; ++c)
                for (int i = 0; i < 4; ++i)
                {
                    auto& b = ap[c][i]; const int N = (int) b.size(); int& wp = apW[c][i];
                    const float z = b[(size_t) wp], v = d[c] + diff * z; b[(size_t) wp] = v; d[c] = z - diff * v; if (++wp >= N) wp = 0;
                }
            float yL = 0, yR = 0;
            if (type == SPRING)
            {   // molla: anello con dispersione (allpass del primo ordine in serie) e banda stretta, due molle leggermente diverse
                for (int c = 0; c < 2; ++c)
                {
                    auto& b = spLine[(size_t) c]; const int N = (int) b.size(); const int L1 = juce::jmin (N - 2, (int) ((c ? 0.0413 : 0.0347) * sr));
                    int q = spW[c] - L1; if (q < 0) q += N; float v = b[(size_t) q];
                    for (auto& st : spAp[c]) { const float o = -0.62f * v + st; st = v + 0.62f * o; v = o; }   // dispersione: il "boing"
                    spHp[c] = v + 0.985f * (spHp[c] - v); v -= spHp[c];                                         // via i bassissimi
                    spLp[c] += (v - spLp[c]) * (0.35f + 0.4f * P.color);                                        // e gli acuti
                    const float gS = (float) std::pow (10.0, -3.0 * L1 / (T * sr));
                    b[(size_t) spW[c]] = d[c] * 0.6f + std::tanh (spLp[c] * gS); if (++spW[c] >= N) spW[c] = 0;
                    (c ? yR : yL) = spLp[c];
                }
            }
            else
            {
                // LFO a quadratura (una sola sinusoide che ruota), fasi diverse per linea
                const float nc = lfoC * cw - lfoS * sw; lfoS = lfoS * cw + lfoC * sw; lfoC = nc;
                float o[8];
                for (int i = 0; i < 8; ++i)
                {
                    const float m = modAmt * (lfoS * kPhC[i] + lfoC * kPhS[i]);
                    float p = (float) w - len[i] - m; while (p < 0) p += (float) LN;
                    const int i0 = juce::jmin (LN - 1, (int) p); const float fr = p - (float) i0; const int i1 = i0 + 1 >= LN ? 0 : i0 + 1;
                    float v = line[i][(size_t) i0] + (line[i][(size_t) i1] - line[i][(size_t) i0]) * fr;
                    damp[i] = v + dA * (damp[i] - v); o[i] = damp[i] * g[i];
                }
                // matrice di Householder: mescola tutte le linee senza perdere energia
                float sum = 0; for (float v : o) sum += v; sum *= 0.25f;
                for (int i = 0; i < 8; ++i)
                {
                    const float inj = (i & 1) ? d[1] : d[0];
                    line[i][(size_t) w] = o[i] - sum + inj * 0.35f;
                }
                if (++w >= LN) w = 0;
                yL = o[0] - o[2] + o[4] - o[6] + 0.5f * (o[1] + o[5]);
                yR = o[1] - o[3] + o[5] - o[7] + 0.5f * (o[2] + o[6]);
                yL *= 0.42f; yR *= 0.42f;
            }
            yL += eL * erMix; yR += eR * erMix;
            // larghezza
            const float m = 0.5f * (yL + yR), sd = 0.5f * (yL - yR) * wid; yL = m + sd; yR = m - sd;
            // taglia bassi / alti sulla coda (12 dB/ott in alto)
            float y[2] = { yL, yR };
            for (int c = 0; c < 2; ++c)
            {
                if (tlA >= 0) { tl[c] = y[c] + tlA * (tl[c] - y[c]); y[c] -= tl[c]; }   // corpo tipico del tipo (piastra e molla sono sottili)
                if (lcA >= 0) { lc[c] = y[c] + lcA * (lc[c] - y[c]); y[c] -= lc[c]; }
                if (hcA >= 0) { hc[c] = y[c] + hcA * (hc[c] - y[c]); hc2[c] = hc[c] + hcA * (hc2[c] - hc[c]); y[c] = hc2[c]; }
            }
            outL[s] = y[0] * outK * oc; outR[s] = y[1] * outK * oc;
        }
        // la sinusoide non deve allontanarsi da ampiezza 1 (errori di arrotondamento)
        const float a = 1.0f / std::sqrt (lfoC * lfoC + lfoS * lfoS); lfoC *= a; lfoS *= a;
    }
private:
public:
    float outCal[kTypes] = { 1, 1, 1, 1, 1, 1 };
private:
    void setType (int t)
    {
        type = t; erN = 0; erMix = 0; tlHz = 0;
        switch (t)
        {
            case HALL:     size = 1.35f; diff = 0.62f; dampK = 0.8f; decayK = 1.15f; modK = 9; outK = 3.02f; break;
            case ROOM:     size = 0.42f; diff = 0.55f; dampK = 1.0f; decayK = 1.2f; modK = 3; outK = 2.09f; setEr (0); tlHz = 70; break;
            case PLATE:    size = 0.62f; diff = 0.75f; dampK = 0.45f; decayK = 1.15f; modK = 6; outK = 4.78f; tlHz = 220; break;
            case SPRING:   size = 1.0f; diff = 0.5f; dampK = 1.0f; decayK = 1.0f; modK = 0; outK = 2.67f; break;
            case CHAMBER:  size = 0.75f; diff = 0.7f; dampK = 1.1f; decayK = 1.19f; modK = 4; outK = 3.23f; setEr (1); tlHz = 110; break;
            default:       size = 0.28f; diff = 0.6f; dampK = 0.9f; decayK = 1.24f; modK = 2; outK = 1.65f; setEr (2); tlHz = 90; break;
        }
        reset();
    }
    void setEr (int which)
    {   // prime riflessioni: ritardi (ms) e guadagni, alternati sinistra / destra
        static const float ms[3][8] = { { 4.3f, 7.9f, 11.2f, 15.7f, 19.1f, 23.6f, 29.4f, 34.9f }, { 6.1f, 10.4f, 15.3f, 21.7f, 27.2f, 34.6f, 41.9f, 52.3f }, { 2.1f, 3.7f, 5.9f, 8.3f, 11.1f, 13.9f, 17.2f, 21.4f } };
        static const float gn[3][8] = { { 0.8f, 0.72f, 0.6f, 0.52f, 0.43f, 0.37f, 0.29f, 0.23f }, { 0.6f, 0.55f, 0.48f, 0.42f, 0.36f, 0.31f, 0.26f, 0.2f }, { 0.9f, 0.8f, 0.7f, 0.6f, 0.5f, 0.42f, 0.34f, 0.26f } };
        erN = 8; erMix = which == 2 ? 0.9f : which == 0 ? 0.65f : 0.35f;
        for (int i = 0; i < 8; ++i) { erTap[i] = juce::jlimit (1, (int) er.size() - 1, (int) (ms[which][i] * 0.001 * sr)); erG[i] = gn[which][i]; }
    }
    static constexpr float kLen[8] = { 1307, 1637, 1811, 1931, 2357, 2641, 2987, 3229 };   // campioni a 44.1k (circa 30-73 ms), primi fra loro
    static constexpr float kApLen[2][4] = { { 142, 107, 379, 277 }, { 151, 113, 397, 263 } };
    static constexpr float kPhC[8] = { 1.0f, 0.707f, 0.0f, -0.707f, -1.0f, -0.707f, 0.0f, 0.707f };
    static constexpr float kPhS[8] = { 0.0f, 0.707f, 1.0f, 0.707f, 0.0f, -0.707f, -1.0f, -0.707f };
    double sr = 48000; int type = HALL, lastType = -1;
    float size = 1, diff = 0.6f, dampK = 1, decayK = 1, modK = 6, outK = 1;
    std::vector<float> line[8]; int w = 0; float damp[8] {};
    std::vector<float> ap[2][4]; int apW[2][4] {};
    std::vector<float> pre; int preW = 0;
    std::vector<float> er; int erW = 0, erN = 0, erTap[8] {}; float erG[8] {}, erMix = 0;
    std::vector<float> spLine[2]; int spW[2] {}; float spAp[2][16] {}, spHp[2] {}, spLp[2] {};
    float lc[2] {}, hc[2] {}, hc2[2] {}, tl[2] {}, tlHz = 0, lfoC = 1, lfoS = 0;
};
}
