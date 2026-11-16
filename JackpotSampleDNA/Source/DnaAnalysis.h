#pragma once
// SAMPLE DNA: analisi di un sample. BPM, tonalita', transienti, cambi d'armonia, colore (nota) di ogni momento,
// e quattro modi di tagliarlo: SMART (tagli che hanno senso musicale), TRANSIENTI, GRIGLIA, ARMONIA.
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <array>
#include <algorithm>
#include <cmath>

namespace dna
{
inline const char* pcName (int pc) { static const char* n[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }; return n[((pc % 12) + 12) % 12]; }
enum Mode { SMART = 0, TRANSIENTS, GRID, HARMONY, kModes };
inline const char* modeName (int m) { static const char* n[kModes] = { "SMART", "TRANSIENTI", "GRIGLIA", "ARMONIA" }; return n[juce::jlimit (0, kModes - 1, m)]; }

struct Analysis
{
    double sr = 44100; int hop = 512; int frames = 0; double dur = 0;
    std::vector<float> onset;                 // inviluppo dei transienti (per frame)
    std::vector<std::array<float, 12>> chroma; // note per frame (normalizzate)
    std::vector<float> novelty;               // cambio d'armonia (per frame)
    std::vector<int> onsets;                  // frame dei transienti trovati
    std::vector<float> onsetStr;
    std::vector<int> onsetSmp;                // campione preciso dell'attacco di ogni colpo
    std::vector<int> changes;                 // frame dei cambi d'armonia
    std::vector<float> wavePk;                // forma d'onda ridotta (per frame)
    float bpm = 0, bpmRaw = 0; double firstBeat = 0; int beats = 0; bool loopFit = false;
    int key = -1; bool minor = false; float keyConf = 0; std::array<float, 12> keyProfile {};
    double fSec (int f) const { return (double) f * hop / sr; }
};

inline float cosSim (const std::array<float, 12>& a, const std::array<float, 12>& b)
{ double d = 0, x = 0, y = 0; for (int i = 0; i < 12; ++i) { d += a[i] * b[i]; x += a[i] * a[i]; y += b[i] * b[i]; } return (float) (d / std::sqrt (x * y + 1e-12)); }

inline Analysis analyse (const juce::AudioBuffer<float>& buf, double sr, float sens = 0.5f)
{
    Analysis A; A.sr = sr; A.hop = 512; const int N = 2048, n = buf.getNumSamples(); A.dur = n / sr;
    std::vector<float> mono ((size_t) n);
    for (int i = 0; i < n; ++i) { float s = 0; for (int c = 0; c < buf.getNumChannels(); ++c) s += buf.getSample (c, i); mono[(size_t) i] = s / (float) juce::jmax (1, buf.getNumChannels()); }
    A.frames = juce::jmax (1, (n - N) / A.hop + 1);
    juce::dsp::FFT fft (11); std::vector<float> win ((size_t) N), tmp ((size_t) N * 2), prev ((size_t) N / 2, 0.0f);
    for (int i = 0; i < N; ++i) win[(size_t) i] = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * i / (N - 1));
    // nota di ogni bin (80 Hz .. 5 kHz)
    std::vector<int> binPc ((size_t) N / 2, -1);
    for (int k = 1; k < N / 2; ++k) { const double hz = k * sr / N; if (hz >= 110 && hz <= 5000) binPc[(size_t) k] = ((int) std::round (12 * std::log2 (hz / 440.0) + 69)) % 12; }
    A.onset.assign ((size_t) A.frames, 0); A.chroma.assign ((size_t) A.frames, {}); A.wavePk.assign ((size_t) A.frames, 0);
    for (int f = 0; f < A.frames; ++f)
    {
        const int o = f * A.hop; float pk = 0;
        for (int i = 0; i < N; ++i) { const float x = o + i < n ? mono[(size_t) (o + i)] : 0.0f; tmp[(size_t) i] = x * win[(size_t) i]; if (i < A.hop) pk = juce::jmax (pk, std::abs (x)); }
        std::fill (tmp.begin() + N, tmp.end(), 0.0f);
        fft.performFrequencyOnlyForwardTransform (tmp.data());
        double flux = 0; auto& ch = A.chroma[(size_t) f];
        for (int k = 1; k < N / 2; ++k)
        {
            const float m = std::log1p (tmp[(size_t) k] * 10.0f); const float d = m - prev[(size_t) k]; if (d > 0) flux += d * (k * sr / N < 200 ? 6.0 : 1.0); prev[(size_t) k] = m;   // i bassi (cassa, 808) contano di piu' per il tempo
        }
        // note: solo i picchi dello spettro, con la frequenza affinata (sui bassi i bin vicini cadono su note diverse)
        for (int k = 2; k < N / 2 - 1; ++k)
        {
            const float m0 = tmp[(size_t) k - 1], m1 = tmp[(size_t) k], m2 = tmp[(size_t) k + 1];
            if (m1 <= m0 || m1 < m2 || binPc[(size_t) k] < 0) continue;
            const float a = std::log (m0 + 1e-9f), b = std::log (m1 + 1e-9f), c = std::log (m2 + 1e-9f), den = a - 2 * b + c;
            const float off = std::abs (den) > 1e-9f ? 0.5f * (a - c) / den : 0.0f;
            const double hz = (k + off) * sr / N;
            ch[(size_t) (((int) std::round (12 * std::log2 (hz / 440.0) + 69)) % 12)] += m1;
        }
        float cs = 0; for (auto& c : ch) cs += c; if (cs > 1e-9f) for (auto& c : ch) c /= cs;
        A.onset[(size_t) f] = (float) flux; A.wavePk[(size_t) f] = pk;
    }
    // normalizza l'inviluppo e trova i picchi (soglia adattiva)
    float omax = 1e-9f; for (float v : A.onset) omax = juce::jmax (omax, v); for (auto& v : A.onset) v /= omax;
    const int W = 8, minGap = (int) std::ceil (0.05 * sr / A.hop);
    const float delta = 0.04f + 0.16f * (1.0f - sens);
    int last = -minGap;
    for (int f = 1; f < A.frames - 1; ++f)
    {
        float mean = 0; int c = 0; for (int k = juce::jmax (0, f - W); k <= juce::jmin (A.frames - 1, f + W); ++k) { mean += A.onset[(size_t) k]; ++c; } mean /= (float) c;
        const float v = A.onset[(size_t) f];
        if (v > mean + delta && v >= A.onset[(size_t) f - 1] && v > A.onset[(size_t) f + 1] && f - last >= minGap) { A.onsets.push_back (f); A.onsetStr.push_back (v); last = f; }
    }
    // il sample che parte gia' con un colpo
    { float pm = 0; for (float v : A.wavePk) pm = juce::jmax (pm, v); if (! A.wavePk.empty() && A.wavePk[0] > 0.2f * pm && (A.onsets.empty() || A.onsets[0] > minGap)) { A.onsets.insert (A.onsets.begin(), 0); A.onsetStr.insert (A.onsetStr.begin(), 1.0f); } }
    // attacco preciso: picco vicino, poi indietro fino a dove il suono comincia a salire
    for (int f : A.onsets)
    {
        const int c = f * A.hop + 768, a0 = juce::jmax (0, c - 1400), a1 = juce::jmin (n - 1, c + 1400);
        int pk = a0; float m = 0; for (int i = a0; i <= a1; ++i) if (std::abs (mono[(size_t) i]) > m) { m = std::abs (mono[(size_t) i]); pk = i; }
        int st = pk; const int lim = juce::jmax (0, pk - (int) (0.03 * sr));
        // inviluppo corto: il punto in cui l'energia scende sotto il 15% del picco
        float e = 0; for (int i = pk; i > lim; --i) { e = 0.9f * e + 0.1f * std::abs (mono[(size_t) i]); if (e < 0.15f * m && std::abs (mono[(size_t) i]) < 0.15f * m) { st = i; break; } st = i; }
        A.onsetSmp.push_back (juce::jmax (0, st - (int) (0.001 * sr)));
    }
    // BPM: autocorrelazione dell'inviluppo + somma dei multipli
    const double fps = sr / A.hop; std::vector<float> oc (A.onset); float om = 0; for (float v : oc) om += v; om /= (float) juce::jmax (1, (int) oc.size()); for (auto& v : oc) v -= om;
    auto ac = [&] (int lag) { double s = 0; for (size_t i = (size_t) lag; i < oc.size(); ++i) s += oc[i] * oc[i - (size_t) lag]; return s / juce::jmax (1.0, (double) oc.size() - lag); };
    const int lagMin = (int) (fps * 60 / 180), lagMax = juce::jmin ((int) oc.size() / 2, (int) (fps * 60 / 60));
    double best = -1e9; int bestLag = 0;
    for (int lag = lagMin; lag <= lagMax; ++lag)
    {
        double s = ac (lag) + (2 * lag < (int) oc.size() ? 0.5 * ac (2 * lag) : 0) + (4 * lag < (int) oc.size() ? 0.25 * ac (4 * lag) : 0);
        const double bpm = 60 * fps / lag; s *= 1.0 + 0.15 * std::exp (-std::pow ((bpm - 110) / 40, 2));   // leggera preferenza per il range centrale
        if (s > best) { best = s; bestLag = lag; }
    }
    if (bestLag > 0) A.bpmRaw = (float) (60 * fps / bestLag);
    {
        std::vector<double> cand;
        auto addC = [&] (double c) { while (c > 0 && c < 60) c *= 2; while (c > 180) c *= 0.5; if (c >= 60 && c <= 180) cand.push_back (c); };
        // i picchi dell'autocorrelazione
        std::vector<std::pair<double, int>> peaks;
        for (int lag = lagMin + 1; lag < lagMax; ++lag) { const double a0 = ac (lag - 1), a1 = ac (lag), a2 = ac (lag + 1); if (a1 > a0 && a1 >= a2) peaks.push_back ({ a1, lag }); }
        std::sort (peaks.begin(), peaks.end(), [] (auto& x, auto& y) { return x.first > y.first; });
        for (size_t i = 0; i < peaks.size() && i < 6; ++i) { const double c = 60 * fps / peaks[i].second; for (double m : { 1.0, 2.0, 0.5, 1.5, 2.0 / 3.0 }) addC (c * m); }
        for (int beats = 4; beats <= 64; ++beats) addC (60.0 * beats / A.dur);
        // colpi forti
        float smax = 0; for (float v : A.onsetStr) smax = juce::jmax (smax, v);
        const double t0 = A.onsetSmp.empty() ? 0.0 : A.onsetSmp[0] / sr;
        double bestS = -1;
        for (double c : cand)
        {
            const double spb = 60.0 / c; double sc = 0, tot = 0;
            for (size_t i = 0; i < A.onsetSmp.size(); ++i)
            {
                const float w = A.onsetStr[i]; if (w < 0.25f * smax) continue;
                const double bt = (A.onsetSmp[i] / sr - t0) / spb, d4 = std::abs (bt - std::round (bt)) * spb, d8 = std::abs (bt * 2 - std::round (bt * 2)) * spb / 2;
                sc += w * w * (d4 < 0.03 ? 1.0 : d8 < 0.03 ? 0.45 : 0.0); tot += w * w;
            }
            if (tot <= 0) continue;
            sc /= tot;
            const double beats = A.dur / spb; const bool fit = std::abs (beats - std::round (beats)) < 0.04 * 4 / juce::jmax (4.0, beats) * 4 && (int) std::round (beats) % 4 == 0;
            sc += (fit ? 0.15 : 0.0) + 0.4 * std::exp (-std::pow ((c - 100) / 35, 2));   // trap/rap/hip hop: 70-140
            if (sc > bestS) { bestS = sc; A.bpm = (float) c; A.loopFit = fit; A.beats = (int) std::round (beats); }
        }
        if (A.bpm <= 0) A.bpm = A.bpmRaw;
        if (A.loopFit) A.bpm = (float) (60.0 * A.beats / A.dur);   // loop: il BPM esatto dalla durata
    }
    A.firstBeat = A.onsetSmp.empty() ? 0.0 : A.onsetSmp[0] / sr;
    if (A.loopFit && A.firstBeat < 0.06) A.firstBeat = 0;
    // tonalita' (Krumhansl-Schmuckler)
    static const float maj[12] = { 6.35f, 2.23f, 3.48f, 2.33f, 4.38f, 4.09f, 2.52f, 5.19f, 2.39f, 3.66f, 2.29f, 2.88f };
    static const float mnr[12] = { 6.33f, 2.68f, 3.52f, 5.38f, 2.60f, 3.53f, 2.54f, 4.75f, 3.98f, 2.69f, 3.34f, 3.17f };
    std::array<float, 12> tot {}; for (auto& c : A.chroma) for (int i = 0; i < 12; ++i) tot[(size_t) i] += c[(size_t) i];
    A.keyProfile = tot;
    float b1 = -2, b2 = -2;
    for (int k = 0; k < 24; ++k)
    {
        std::array<float, 12> prof {}; for (int i = 0; i < 12; ++i) prof[(size_t) ((i + k % 12) % 12)] = (k < 12 ? maj : mnr)[i];
        double mx = 0, my = 0; for (int i = 0; i < 12; ++i) { mx += tot[(size_t) i]; my += prof[(size_t) i]; } mx /= 12; my /= 12;
        double sxy = 0, sxx = 0, syy = 0; for (int i = 0; i < 12; ++i) { const double a = tot[(size_t) i] - mx, b = prof[(size_t) i] - my; sxy += a * b; sxx += a * a; syy += b * b; }
        const float r = (float) (sxy / std::sqrt (sxx * syy + 1e-12));
        if (r > b1) { b2 = b1; b1 = r; A.key = k % 12; A.minor = k >= 12; } else if (r > b2) b2 = r;
    }
    A.keyConf = juce::jlimit (0.0f, 1.0f, (b1 - b2) * 5.0f + (b1 - 0.5f));
    // cambi d'armonia: differenza fra la mezza battuta prima e quella dopo
    const int hw = juce::jmax (4, (int) (0.4 * fps)); A.novelty.assign ((size_t) A.frames, 0);
    for (int f = hw; f < A.frames - hw; ++f)
    {
        std::array<float, 12> a {}, b {}; for (int k = 0; k < hw; ++k) for (int i = 0; i < 12; ++i) { a[(size_t) i] += A.chroma[(size_t) (f - 1 - k)][(size_t) i]; b[(size_t) i] += A.chroma[(size_t) (f + k)][(size_t) i]; }
        A.novelty[(size_t) f] = 1.0f - cosSim (a, b);
    }
    float nmax = 1e-6f; for (float v : A.novelty) nmax = juce::jmax (nmax, v); for (auto& v : A.novelty) v /= nmax;
    for (int f = 1, lastC = -hw; f < A.frames - 1; ++f)
        if (A.novelty[(size_t) f] > 0.35f && A.novelty[(size_t) f] >= A.novelty[(size_t) f - 1] && A.novelty[(size_t) f] > A.novelty[(size_t) f + 1] && f - lastC > hw) { A.changes.push_back (f); lastC = f; }
    return A;
}

// ------------------------------------------------------------------------------------------------ tagli
inline int snapToOnset (const Analysis& A, int f, int within)
{ int best = f, bd = within + 1; for (int o : A.onsets) { const int d = std::abs (o - f); if (d < bd) { bd = d; best = o; } } return best; }
inline int onsetSample (const Analysis& A, int f) { for (size_t i = 0; i < A.onsets.size(); ++i) if (A.onsets[i] == f) return A.onsetSmp[i]; return f * A.hop; }

// restituisce i punti d'inizio delle slice in campioni (il primo e' sempre 0)
inline std::vector<int> chop (const Analysis& A, int mode, int count, int variant, const juce::AudioBuffer<float>& buf)
{
    std::vector<int> fr { 0 };
    const double fps = A.sr / A.hop, spb = A.bpm > 0 ? 60.0 / A.bpm : 0.5;
    const int minGap = juce::jmax ((int) std::ceil (0.06 * fps), (int) (A.frames / (count * 3.0)));
    auto gridScore = [&] (int f)
    {
        if (A.bpm <= 0) return 0.0f;
        const double t = onsetSample (A, f) / A.sr - A.firstBeat, b = t / spb;
        const double d8 = std::abs (b * 2 - std::round (b * 2)) * spb / 2, d16 = std::abs (b * 4 - std::round (b * 4)) * spb / 4;
        return d8 < 0.025 ? 1.0f : d16 < 0.02 ? 0.5f : 0.0f;
    };
    auto novAt = [&] (int f) { float m = 0; for (int k = juce::jmax (0, f - 6); k <= juce::jmin (A.frames - 1, f + 6); ++k) m = juce::jmax (m, A.novelty[(size_t) k]); return m; };
    auto pickTop = [&] (std::vector<std::pair<float, int>> cand)
    {
        std::sort (cand.begin(), cand.end(), [] (auto& a, auto& b) { return a.first > b.first; });
        for (auto& c : cand)
        {
            if ((int) fr.size() >= count) break;
            bool ok = true; for (int x : fr) if (std::abs (x - c.second) < minGap) { ok = false; break; }
            if (ok) fr.push_back (c.second);
        }
    };
    juce::Random rnd (1234 + variant * 97);
    if (mode == TRANSIENTS) { std::vector<std::pair<float, int>> c; for (size_t i = 0; i < A.onsets.size(); ++i) c.push_back ({ A.onsetStr[i] + 0.15f * rnd.nextFloat() * (variant > 0), A.onsets[i] }); pickTop (c); }
    else if (mode == HARMONY) { std::vector<std::pair<float, int>> c; for (int f : A.changes) c.push_back ({ A.novelty[(size_t) f] + 0.1f * rnd.nextFloat() * (variant > 0), snapToOnset (A, f, (int) (0.12 * fps)) }); pickTop (c); }
    else if (mode == GRID)
    {
        const double totalBeats = A.dur / spb; double div = 0.25; for (double d : { 0.25, 0.5, 1.0, 2.0, 4.0, 8.0 }) { div = d; if (totalBeats / d <= count + 0.01) break; }
        if (variant % 2) div *= 0.5;
        std::vector<int> smp { 0 };
        if (A.firstBeat > 0.03) smp.push_back ((int) (A.firstBeat * A.sr));
        for (int k = 1; ; ++k)
        {
            const double t = A.firstBeat + k * div * spb; if (t >= A.dur - 0.02 || (int) smp.size() >= count) break;
            int s = (int) (t * A.sr); for (size_t i = 0; i < A.onsetSmp.size(); ++i) if (std::abs (A.onsetSmp[i] - s) < (int) (0.03 * A.sr)) { s = A.onsetSmp[i]; break; }
            smp.push_back (s);
        }
        std::sort (smp.begin(), smp.end()); return smp;
    }
    else
    {   // SMART: forza del colpo + cambio d'armonia + allineamento alla griglia
        std::vector<std::pair<float, int>> c;
        for (size_t i = 0; i < A.onsets.size(); ++i) { const int f = A.onsets[i]; c.push_back ({ 0.5f * A.onsetStr[i] + 0.35f * novAt (f) + 0.25f * gridScore (f) + (variant > 0 ? 0.2f * rnd.nextFloat() : 0.0f), f }); }
        pickTop (c);
    }
    std::sort (fr.begin(), fr.end()); fr.erase (std::unique (fr.begin(), fr.end()), fr.end());
    // in campioni, con il taglio portato sullo zero piu' vicino (prima del colpo)
    std::vector<int> out; const int n = buf.getNumSamples();
    for (int f : fr)
    {
        int s = juce::jlimit (0, n - 1, onsetSample (A, f));
        if (f == 0) s = 0;
        else for (int k = 0; k < (int) (0.003 * A.sr) && s > 1; ++k, --s) { const float a = buf.getSample (0, s - 1), b = buf.getSample (0, s); if ((a <= 0 && b > 0) || (a >= 0 && b < 0)) break; }
        if (out.empty() || s - out.back() > 64) out.push_back (s);
    }
    return out;
}
}
