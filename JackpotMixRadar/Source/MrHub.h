#pragma once
// JACKPOT MIX RADAR: un SENSORE su ogni traccia importante, un RADAR sul Master.
// I sensori mandano al radar cosa suonano (bande, livello, stereo dei bassi, transienti); il radar incrocia tutto
// e dice quale traccia crea il problema e in quale battuta.
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include <cmath>

namespace mr
{
constexpr int NB = 24, MAXS = 32;
inline float bandHz (int b) { return 30.0f * std::pow (16000.0f / 30.0f, (float) b / (NB - 1)); }
enum Role { KICK = 0, BASS, VOICE, MELODY, PAD, HATS, FXR, OTHER, kRoles };
inline const char* roleName (int r) { static const char* n[kRoles] = { "CASSA", "808 / BASSO", "VOCE", "MELODIA", "PAD / ACCORDI", "HI-HAT / PERC", "FX", "ALTRO" }; return n[juce::jlimit (0, kRoles - 1, r)]; }
inline juce::uint32 nowMs() { return juce::Time::getMillisecondCounter(); }

struct Sensor
{
    std::atomic<juce::int64> owner { 0 };
    std::atomic<juce::uint32> stamp { 0 };
    std::atomic<int> role { OTHER };
    std::atomic<float> bands[NB] {}, lvl { -120 }, peak { -120 }, sideLo { -100 }, crest { 0 };
    juce::SpinLock nameLock; char name[64] {};
    void setName (const juce::String& s) { const juce::SpinLock::ScopedLockType l (nameLock); s.copyToUTF8 (name, sizeof (name) - 1); }
    juce::String getName() { const juce::SpinLock::ScopedLockType l (nameLock); return juce::String::fromUTF8 (name); }
    bool alive() const { const auto s = stamp.load(); return s != 0 && nowMs() - s < 600; }
};
struct Hub { Sensor s[MAXS]; std::atomic<juce::uint32> radarStamp { 0 }; };
inline Hub& hub() { static Hub h; return h; }
inline bool radarAlive() { const auto s = hub().radarStamp.load(); return s != 0 && nowMs() - s < 800; }
inline int claim (juce::int64 id)
{
    auto& H = hub();
    for (int i = 0; i < MAXS; ++i) if (H.s[i].owner.load() == id) return i;
    for (int i = 0; i < MAXS; ++i)
    {
        juce::int64 cur = H.s[i].owner.load();
        if ((cur == 0 || ! H.s[i].alive()) && H.s[i].owner.compare_exchange_strong (cur, id)) { H.s[i].stamp = nowMs(); return i; }
    }
    return -1;
}

// banco di passa-banda per l'analisi
struct Bank
{
    float a1[NB], a2[NB], a3[NB], z1[NB] {}, z2[NB] {}; double acc[NB] {}; int n = 0;
    void prepare (double sr)
    {
        const float k = 1.0f / 2.2f;
        for (int b = 0; b < NB; ++b) { const float g = std::tan (juce::MathConstants<float>::pi * juce::jmin (bandHz (b), 0.45f * (float) sr) / (float) sr); a1[b] = 1.0f / (1.0f + g * (g + k)); a2[b] = g * a1[b]; a3[b] = g * a2[b]; z1[b] = z2[b] = 0; acc[b] = 0; }
        n = 0;
    }
    void push (float x)
    {
        for (int b = 0; b < NB; ++b) { const float v3 = x - z2[b], v1 = a1[b] * z1[b] + a2[b] * v3, v2 = z2[b] + a2[b] * z1[b] + a3[b] * v3; z1[b] = 2 * v1 - z1[b]; z2[b] = 2 * v2 - z2[b]; acc[b] += (double) v1 * v1; }
        ++n;
    }
    void take (float* out) { for (int b = 0; b < NB; ++b) { out[b] = (float) (10 * std::log10 (acc[b] / juce::jmax (1, n) + 1e-12)); acc[b] = 0; } n = 0; }
};
}
