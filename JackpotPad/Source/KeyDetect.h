#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

namespace jp
{
// tonalita: 0..11 maggiore (C..B), 12..23 minore (Cm..Bm), 24..35 nota singola (one shot), -1 sconosciuta, -2 analizzata senza esito
inline juce::String keyName (int k)
{
    static const char* n[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    if (k < 0 || k > 35) return "--";
    return juce::String (n[k % 12]) + (k >= 12 && k < 24 ? "m" : "");
}

// "Loop Dark 140 Bm", "piano_F#min_90bpm", "Vox C Minor", "Melody 85 G#", "keys_Ebmaj"
inline int keyFromName (const juce::String& name)
{
    const auto clean = name.replaceCharacters ("_-.()[],+", "         ");
    auto toks = juce::StringArray::fromTokens (clean, " ", "");
    toks.removeEmptyStrings();
    bool hasNumber = false; for (auto& t : toks) if (t.containsOnly ("0123456789") && t.getIntValue() >= 50 && t.getIntValue() <= 220) hasNumber = true;
    auto mode = [] (const juce::String& s) -> int   // 0 maggiore, 1 minore, -1 non e un suffisso
    {
        const auto l = s.toLowerCase();
        if (l.isEmpty() || l == "maj" || l == "major" || l == "ma" || l == "dur") return 0;
        if (l == "min" || l == "minor" || l == "mi" || l == "moll") return 1;
        if (s == "m") return 1;
        return -1;
    };
    for (int i = toks.size() - 1; i >= 0; --i)
    {
        auto t = toks[i];
        if (t.toLowerCase().startsWith ("key")) { t = t.substring (3); if (t.isEmpty()) continue; }
        const auto c0 = t[0];
        if (! ((c0 >= 'A' && c0 <= 'G') || (c0 >= 'a' && c0 <= 'g'))) continue;
        static const int base[] = { 9, 11, 0, 2, 4, 5, 7 };   // A B C D E F G
        int pc = base[juce::CharacterFunctions::toUpperCase (c0) - 'A'];
        auto rest = t.substring (1); bool acc = false;
        const auto sharpU = juce::String::fromUTF8 ("\xe2\x99\xaf"), flatU = juce::String::fromUTF8 ("\xe2\x99\xad");
        if (rest.startsWith ("#")) { pc += 1; acc = true; rest = rest.substring (1); }
        else if (rest.startsWith (sharpU)) { pc += 1; acc = true; rest = rest.substring (1); }
        else if (rest.toLowerCase().startsWith ("sharp")) { pc += 1; acc = true; rest = rest.substring (5); }
        else if (rest.startsWith (flatU)) { pc -= 1; acc = true; rest = rest.substring (1); }
        else if (rest.toLowerCase().startsWith ("flat")) { pc -= 1; acc = true; rest = rest.substring (4); }
        else if (rest.startsWith ("b") && mode (rest.substring (1)) >= 0) { pc -= 1; acc = true; rest = rest.substring (1); }   // Bb, Ebm, ebmaj
        // nota con ottava: "808 C1", "Piano_F#3"
        if (rest.length() == 1 && rest.containsOnly ("01234567") && (c0 >= 'A' && c0 <= 'G')) return 24 + (pc + 12) % 12;
        while (rest.isNotEmpty() && juce::CharacterFunctions::isDigit (rest.getLastCharacter())) rest = rest.dropLastCharacters (1);   // Am7, Cmaj9
        int m = mode (rest);
        if (m < 0) continue;
        const bool upper = c0 >= 'A' && c0 <= 'G';
        if (rest.isEmpty() && i + 1 < toks.size())
        {   // "C Minor", "F# major"
            const auto nx = toks[i + 1].toLowerCase();
            if (nx == "minor" || nx == "min" || nx == "moll") { m = 1; rest = "min"; }
            else if (nx == "major" || nx == "maj" || nx == "dur") { m = 0; rest = "maj"; }
        }
        const bool explicitSuffix = rest.isNotEmpty() && rest != "m";
        bool ok = false;
        if (explicitSuffix) ok = true;                       // Cmin, F#major, ebmaj
        else if (rest == "m") ok = upper || acc || (hasNumber && c0 != 'a');   // Am, F#m, "loop 140 bm" (non "am")
        else if (acc) ok = upper || t.length() <= 2;         // C#, Bb, f#
        else ok = upper && t.length() == 1 && (hasNumber || i == toks.size() - 1) && toks.size() > 1;   // "Loop 90 C"
        if (! ok) continue;
        pc = (pc + 12) % 12;
        return pc + (m == 1 ? 12 : 0);
    }
    return -1;
}

// stima dall'ascolto: cromagramma e profili di Krumhansl-Schmuckler
inline int keyFromAudio (const juce::AudioBuffer<float>& b, double sr, float& confidence)
{
    confidence = 0;
    const int n = b.getNumSamples(); if (n < (int) (sr * 0.25)) return -1;
    constexpr int order = 13, N = 1 << order;
    juce::dsp::FFT fft (order); std::vector<float> buf ((size_t) N * 2), win ((size_t) N);
    for (int i = 0; i < N; ++i) win[(size_t) i] = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * i / N);
    double chroma[12] = {};
    const float* d0 = b.getReadPointer (0); const float* d1 = b.getReadPointer (juce::jmin (1, b.getNumChannels() - 1));
    const int span = juce::jmin (n, (int) (sr * 40)), frames = juce::jlimit (1, 60, span / (N / 2));
    std::vector<int> binPc ((size_t) N / 2, -1); std::vector<float> binW ((size_t) N / 2, 0.0f);
    for (int k = 1; k < N / 2; ++k)
    {
        const double f = k * sr / N; if (f < 50 || f > 2500) continue;
        const double midi = 69 + 12 * std::log2 (f / 440.0);
        binPc[(size_t) k] = ((int) std::round (midi) % 12 + 12) % 12;
        binW[(size_t) k] = (float) (1.0 - std::abs (midi - std::round (midi)));   // piu peso vicino al semitono
    }
    for (int fr = 0; fr < frames; ++fr)
    {
        const int st = frames == 1 ? 0 : (int) ((double) fr / frames * juce::jmax (0, span - N));
        std::fill (buf.begin(), buf.end(), 0.0f);
        for (int i = 0; i < N && st + i < n; ++i) buf[(size_t) i] = 0.5f * (d0[st + i] + d1[st + i]) * win[(size_t) i];
        fft.performFrequencyOnlyForwardTransform (buf.data(), true);
        double fc[12] = {}, tot = 0;
        for (int k = 1; k < N / 2; ++k) if (binPc[(size_t) k] >= 0) { const double m = buf[(size_t) k], e = m * m * binW[(size_t) k]; fc[binPc[(size_t) k]] += e; tot += e; }
        if (tot <= 1e-9) continue;
        for (int c = 0; c < 12; ++c) chroma[c] += std::sqrt (fc[c] / tot);
    }
    double s = 0, top = 0; int topPc = 0; for (int c = 0; c < 12; ++c) { s += chroma[c]; if (chroma[c] > top) { top = chroma[c]; topPc = c; } }
    if (s <= 0) return -1;
    // one shot corto con una nota dominante: e una nota, non una tonalita
    if (n < sr * 2.5 && top / s > 0.26) { confidence = (float) (top / s); return 24 + topPc; }
    static const double maj[] = { 6.35, 2.23, 3.48, 2.33, 4.38, 4.09, 2.52, 5.19, 2.39, 3.66, 2.29, 2.88 };
    static const double mnr[] = { 6.33, 2.68, 3.52, 5.38, 2.60, 3.53, 2.54, 4.75, 3.98, 2.69, 3.34, 3.17 };
    auto corr = [&] (const double* prof, int rot)
    {
        double mx = 0, my = 0; for (int i = 0; i < 12; ++i) { mx += chroma[(i + rot) % 12]; my += prof[i]; } mx /= 12; my /= 12;
        double sxy = 0, sxx = 0, syy = 0;
        for (int i = 0; i < 12; ++i) { const double x = chroma[(i + rot) % 12] - mx, y = prof[i] - my; sxy += x * y; sxx += x * x; syy += y * y; }
        return sxy / std::sqrt (sxx * syy + 1e-12);
    };
    double best = -2, second = -2; int bk = -1;
    for (int k = 0; k < 24; ++k)
    {
        const double c = corr (k < 12 ? maj : mnr, k % 12);
        if (c > best) { second = best; best = c; bk = k; } else if (c > second) second = c;
    }
    confidence = (float) juce::jmax (0.0, best - second) + (float) juce::jmax (0.0, best - 0.5);
    return best > 0.45 ? bk : -1;
}

// semitoni (-6..+5) per portare la tonalita "from" in "to"; maggiore e minore relativi contano uguali
inline int keyShift (int from, int to)
{
    if (from < 0 || to < 0) return 0;
    if (from >= 24 || to >= 24) { int d = (((to % 12) - (from % 12)) % 12 + 12) % 12; if (d > 5) d -= 12; return d; }   // nota: tonica su tonica
    int tpc = to % 12; const bool fm = from >= 12, tm = to >= 12;
    if (fm && ! tm) tpc = (tpc + 9) % 12;      // maggiore -> relativo minore
    if (! fm && tm) tpc = (tpc + 3) % 12;      // minore -> relativo maggiore
    int d = ((tpc - from % 12) % 12 + 12) % 12; if (d > 5) d -= 12;
    return d;
}
}
