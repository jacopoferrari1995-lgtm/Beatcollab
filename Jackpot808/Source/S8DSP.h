#pragma once
// JACKPOT 808 STUDIO: centro di controllo delle 808.
//  accordatore e nota piu' usata, transienti (PUNCH / SUSTAIN), saturazione a 3 bande col sub pulito,
//  armoniche esatte (2a e 3a, polinomi di Chebyshev) per farla sentire sulle casse piccole,
//  cassa in sidechain: ducking, fase, sovrapposizioni; mono sui bassi; clipper finale; diagnosi "cosa manca".
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include <array>

namespace s8
{
inline const char* noteName (int pc) { static const char* n[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }; return n[((pc % 12) + 12) % 12]; }

// ------------------------------------------------------------------------------------------------ accordatore (YIN su segnale decimato a ~2 kHz)
class Tuner
{
public:
    static constexpr int WIN = 512;
    void prepare (double sr)
    {
        dec = juce::jmax (1, (int) std::round (sr / 2000.0)); fs = sr / dec;
        auto c = juce::dsp::IIR::Coefficients<float>::makeLowPass (sr, 280.0f, 0.707f);
        lp1.coefficients = c; lp2.coefficients = c; lp1.reset(); lp2.reset();
        buf.fill (0); w = 0; cnt = 0; hop = 0;
    }
    // ritorna true quando ha una nuova misura
    bool push (float x)
    {
        x = lp2.processSample (lp1.processSample (x));
        if (++cnt < dec) return false; cnt = 0;
        buf[(size_t) w] = x; w = (w + 1) % WIN;
        if (++hop < 64) return false; hop = 0;
        return analyse();
    }
    float hz = 0, conf = 0, level = -120;
private:
    bool analyse()
    {
        float x[WIN]; double e = 0; for (int i = 0; i < WIN; ++i) { x[i] = buf[(size_t) ((w + i) % WIN)]; e += (double) x[i] * x[i]; }
        level = (float) (10 * std::log10 (e / WIN + 1e-12));
        if (level < -50) { conf = 0; return true; }
        const int tMin = (int) (fs / 220.0), tMax = juce::jmin (WIN / 2 - 2, (int) (fs / 25.0));
        float d[WIN / 2] {}; float run = 0; d[0] = 1;
        for (int t = 1; t <= tMax; ++t)
        {
            double s = 0; for (int i = 0; i < WIN / 2; ++i) { const float q = x[i] - x[i + t]; s += (double) q * q; }
            run += (float) s; d[t] = run > 0 ? (float) (s * t / run) : 1.0f;
        }
        int best = -1; for (int t = tMin; t <= tMax; ++t) if (d[t] < 0.15f) { while (t + 1 <= tMax && d[t + 1] < d[t]) ++t; best = t; break; }
        if (best < 0) { float mn = 9; for (int t = tMin; t <= tMax; ++t) if (d[t] < mn) { mn = d[t]; best = t; } if (mn > 0.35f) { conf = 0; return true; } }
        float tt = (float) best;
        if (best > 1 && best < tMax) { const float a = d[best - 1], b = d[best], c = d[best + 1], den = a - 2 * b + c; if (std::abs (den) > 1e-9f) tt += 0.5f * (a - c) / den; }
        hz = (float) fs / tt; conf = juce::jlimit (0.0f, 1.0f, 1.0f - d[best]); return true;
    }
    int dec = 24, cnt = 0, w = 0, hop = 0; double fs = 2000; std::array<float, WIN> buf {};
    juce::dsp::IIR::Filter<float> lp1, lp2;
};

// ------------------------------------------------------------------------------------------------ saturazioni
inline float sat (float x, int mode)
{
    switch (mode)
    {
        case 0: return std::tanh (x);                                                   // TAPE
        case 1: { const float y = x + 0.25f * x * x; return std::tanh (y) - 0.25f * std::tanh (0.25f); }   // TUBE (pari)
        case 2: return juce::jlimit (-1.0f, 1.0f, x * (1.0f - 0.1f * x * x * (std::abs (x) < 1.0f ? 1.0f : 0.0f)));   // HARD
        default: { const float a = std::abs (x); return (x < 0 ? -1.0f : 1.0f) * (1.0f - std::exp (-2.5f * a)); }    // FUZZ
    }
}
inline const char* modeName (int m) { static const char* n[4] = { "TAPE", "TUBE", "HARD", "FUZZ" }; return n[juce::jlimit (0, 3, m)]; }
}
