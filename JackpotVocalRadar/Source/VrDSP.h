#pragma once
// JACKPOT VOCAL RADAR: la voce manda la sua impronta, il beat le fa spazio.
// Le copie del plugin si parlano direttamente (stessa memoria del programma): una sulla voce in modo VOCE,
// una o piu' sul beat / sulle melodie / sul bus in modo BEAT. In alternativa la voce arriva dal sidechain.
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include <cmath>
#include <array>

namespace vr
{
constexpr int NB = 20;            // bande di analisi e di intervento, 80 Hz .. 12 kHz
constexpr int HOP = 64;           // un fotogramma ogni 64 campioni
constexpr int NCH = 4;            // canali A B C D (voce principale, ad-lib, doppie...)
constexpr int RING = 1024;
constexpr int SLOTS = 64;         // ricevitori BEAT conteggiati per canale
inline float bandHz (int b) { return 80.0f * std::pow (150.0f, (float) b / (NB - 1)); }
inline const char* chName (int c) { static const char* n[NCH] = { "A", "B", "C", "D" }; return n[juce::jlimit (0, NCH - 1, c)]; }

// ------------------------------------------------------------------------------------------------ il collegamento fra le copie
struct Frame { double pos = -1; float band[NB] {}; float lvl = -120; };
struct Chan
{
    std::atomic<int> w { 0 };
    Frame f[RING];
    std::atomic<juce::uint32> stamp { 0 };                    // ultimo invio della voce (ms)
    std::atomic<juce::uint32> beatStamp[SLOTS] {};            // ultimi ascolti dei BEAT
    std::atomic<float> lvl { -120 }, bands[NB] {};             // per la grafica
};
struct Hub { Chan ch[NCH]; };
inline Hub& hub() { static Hub h; return h; }
inline juce::uint32 nowMs() { return juce::Time::getMillisecondCounter(); }
inline bool voiceAlive (int c) { const auto s = hub().ch[c].stamp.load(); return s != 0 && nowMs() - s < 400; }
inline int beatsListening (int c) { int n = 0; const auto t = nowMs(); for (auto& s : hub().ch[c].beatStamp) { const auto v = s.load(); if (v != 0 && t - v < 1000) ++n; } return n; }

// ------------------------------------------------------------------------------------------------ banco di filtri passa-banda (SVF)
struct Bank
{
    struct S { float ic1 = 0, ic2 = 0; };
    float a1[NB], a2[NB], a3[NB]; S s[NB]; double acc[NB] {}; double accAll = 0; int n = 0;
    void prepare (double sr)
    {
        const float Q = 2.6f, k = 1.0f / Q;
        for (int b = 0; b < NB; ++b) { const float g = std::tan (juce::MathConstants<float>::pi * juce::jmin (bandHz (b), 0.45f * (float) sr) / (float) sr); a1[b] = 1.0f / (1.0f + g * (g + k)); a2[b] = g * a1[b]; a3[b] = g * a2[b]; s[b] = {}; }
        reset();
    }
    void reset() { for (auto& a : acc) a = 0; accAll = 0; n = 0; }
    void push (float x)
    {
        for (int b = 0; b < NB; ++b)
        {
            auto& z = s[b]; const float v3 = x - z.ic2, v1 = a1[b] * z.ic1 + a2[b] * v3, v2 = z.ic2 + a2[b] * z.ic1 + a3[b] * v3;
            z.ic1 = 2 * v1 - z.ic1; z.ic2 = 2 * v2 - z.ic2; acc[b] += (double) v1 * v1;
        }
        accAll += (double) x * x; ++n;
    }
    // chiude il fotogramma: livelli in dB
    float take (float* out)
    {
        const double inv = 1.0 / juce::jmax (1, n);
        for (int b = 0; b < NB; ++b) out[b] = (float) (10.0 * std::log10 (acc[b] * inv + 1e-12));
        const float all = (float) (10.0 * std::log10 (accAll * inv + 1e-12)); reset(); return all;
    }
};

// ------------------------------------------------------------------------------------------------ EQ a campana (RBJ)
struct Peak
{
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    void set (float f, float q, float db, double sr)
    {
        if (std::abs (db) < 0.01f) { b0 = 1; b1 = b2 = a1 = a2 = 0; return; }
        const float A = std::pow (10.0f, db / 40.0f), w = juce::MathConstants<float>::twoPi * f / (float) sr, al = std::sin (w) / (2 * q), c = std::cos (w);
        const float a0 = 1 + al / A; b0 = (1 + al * A) / a0; b1 = -2 * c / a0; b2 = (1 - al * A) / a0; a1 = -2 * c / a0; a2 = (1 - al / A) / a0;
    }
    float run (float x) { const float y = b0 * x + z1; z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y; return y; }
};
}
