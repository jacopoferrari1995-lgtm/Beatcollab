#pragma once
// JACKPOT FLUX: motore di interazione musicale. 8 corsie MIDI (cassa, rullante, hi-hat, perc, 808, melodia, contro, chop).
// Ogni battuta viene "suonata" dal pattern base trasformato da: 4 macro (DENSITY, ENERGY, MOVEMENT, SPACE),
// regole fra corsie (DIRADA, RISPONDE, SEGUE, ROLL SUL CAMBIO), variazioni con seme salvabile (VARIATE), solo dove autorizzi.
// Tutto deterministico: stesso seme + stesse macro = stesso risultato.
#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include <array>
#include <memory>
#include <cmath>

namespace flux
{
constexpr int NL = 8, BARS = 4, BPB = 4;   // corsie, battute del pattern, battiti per battuta
enum Lane { KICK = 0, SNARE, HATS, PERC, BASS, MEL, COUNTER, CHOP };
inline const char* laneName (int l) { static const char* n[NL] = { "CASSA", "RULLANTE", "HI-HAT", "PERC", "808", "MELODIA", "CONTRO", "CHOP" }; return n[juce::jlimit (0, NL - 1, l)]; }
inline bool isDrum (int l) { return l <= PERC || l == CHOP; }
enum RuleType { DIRADA = 0, RISPONDE, SEGUE, ROLL, kRuleTypes };
inline const char* ruleName (int r) { static const char* n[kRuleTypes] = { "DIRADA", "RISPONDE", "SEGUE ENERGIA", "ROLL SUL CAMBIO" }; return n[juce::jlimit (0, kRuleTypes - 1, r)]; }
inline const char* ruleDesc (int r)
{
    static const char* n[kRuleTypes] = { "quando suona la sorgente, la destinazione si toglie di mezzo",
                                         "la destinazione suona solo negli spazi lasciati dalla sorgente (botta e risposta)",
                                         "la destinazione suona piu' forte dove la sorgente e' piu' fitta",
                                         "quando la sorgente cambia nota, la destinazione fa un roll" };
    return n[juce::jlimit (0, kRuleTypes - 1, r)];
}
constexpr int NSEC = 5;
inline const char* secName (int s) { static const char* n[NSEC] = { "INTRO", "STROFA", "HOOK", "BRIDGE", "OUTRO" }; return n[juce::jlimit (0, NSEC - 1, s)]; }

struct Note { float t, len; int note; int vel; };          // t e len in battiti dall'inizio del pattern (0..16)
struct Rule { int from = BASS, to = MEL, type = DIRADA; bool on = true; };
struct Snap
{
    std::array<std::vector<Note>, NL> lane;
    std::vector<Rule> rules;
    std::array<bool, NL> var {}, mute {};
    int key = 9; bool minor = true;
};
struct Macros { float density = 0.5f, energy = 0.5f, movement = 0.3f, space = 0.0f; };
struct Ev { int lane, note, vel; float t, len; };            // t in battiti dentro la battuta
struct BarEvents { int n = 0; Ev ev[640]; void add (const Ev& e) { if (n < 640) ev[n++] = e; } };

// ------------------------------------------------------------------------------------------------ casualita' ripetibile
inline juce::uint32 hsh (juce::uint32 a, juce::uint32 b, juce::uint32 c = 0, juce::uint32 d = 0)
{ juce::uint32 h = 2166136261u; for (juce::uint32 x : { a, b, c, d }) { h ^= x + 0x9e3779b9u + (h << 6) + (h >> 2); h *= 16777619u; } return h; }
inline float rnd01 (juce::uint32 h) { return (float) (h & 0xffffff) / (float) 0x1000000; }

// ------------------------------------------------------------------------------------------------ scala
inline int scaleNote (int key, bool minor, int degree, int octave)
{
    static const int mn[7] = { 0, 2, 3, 5, 7, 8, 10 }, mj[7] = { 0, 2, 4, 5, 7, 9, 11 };
    const int oct = (int) std::floor (degree / 7.0); const int d = degree - oct * 7;
    return 12 * (octave + 1 + oct) + key + (minor ? mn : mj)[d];
}

// ------------------------------------------------------------------------------------------------ starter
enum Starter { TRAP = 0, DRILL, BOOMBAP, EMPTY, kStarters };
inline const char* starterName (int s) { static const char* n[kStarters] = { "TRAP", "DRILL", "BOOM BAP", "VUOTO" }; return n[juce::jlimit (0, kStarters - 1, s)]; }
inline Snap makeStarter (int s, int key, bool minor)
{
    Snap S; S.key = key; S.minor = minor; for (auto& v : S.var) v = true; S.var[KICK] = false; S.var[SNARE] = false;
    if (s == EMPTY) return S;
    auto add = [&] (int l, float t, float len, int n, int v) { S.lane[(size_t) l].push_back ({ t, len, n, v }); };
    // progressione nella scala: minore i-VI-III-VII, maggiore I-V-vi-IV
    const int prog[4] = { 0, minor ? 5 : 4, minor ? 2 : 5, minor ? 6 : 3 };
    for (int bar = 0; bar < BARS; ++bar)
    {
        const float b0 = (float) bar * BPB; const int deg = prog[bar];
        if (s == TRAP)
        {
            static const int kicks[4][4] = { { 0, 6, 10, -1 }, { 0, 3, 10, 14 }, { 0, 6, 10, -1 }, { 0, 6, 8, 11 } };
            for (int k : kicks[bar]) if (k >= 0) add (KICK, b0 + k * 0.25f, 0.25f, 36, 110);
            add (SNARE, b0 + 2, 0.25f, 38, 112);
            for (int st = 0; st < 16; st += 2) add (HATS, b0 + st * 0.25f, 0.125f, 42, st % 4 == 0 ? 100 : 76);
            if (bar % 2) add (PERC, b0 + 3.5f, 0.25f, 39, 80);
            for (int k = 0; k < 4 && kicks[bar][k] >= 0; ++k)
            {
                const int nx = k + 1 < 4 && kicks[bar][k + 1] >= 0 ? kicks[bar][k + 1] : 16;
                add (BASS, b0 + kicks[bar][k] * 0.25f, (nx - kicks[bar][k]) * 0.25f - 0.05f, scaleNote (key, minor, deg, 1), 105);
            }
        }
        else if (s == DRILL)
        {
            static const float kicks[4][4] = { { 0, 2.75f, -1, -1 }, { 0.75f, 1.5f, 2.75f, -1 }, { 0, 2.75f, 3.5f, -1 }, { 0.75f, 1.5f, 3.25f, -1 } };
            for (float k : kicks[bar]) if (k >= 0) add (KICK, b0 + k, 0.25f, 36, 108);
            add (SNARE, b0 + 2, 0.25f, 38, 112); if (bar % 2) add (SNARE, b0 + 3.75f, 0.2f, 38, 70);
            // hi-hat a terzine con le pause tipiche
            for (int k = 0; k < 12; ++k) if (k % 6 != 4) add (HATS, b0 + k / 3.0f, 0.1f, 42, k % 3 == 0 ? 100 : 70);
            add (PERC, b0 + 1.25f, 0.25f, 75, 75);
            // 808 che scivola: note che si sovrappongono (glide)
            float last = -1; int ln = 0;
            for (float k : kicks[bar]) if (k >= 0)
            {
                const int n = scaleNote (key, minor, deg + (last >= 0 && ln % 2 ? 7 : 0), 1);
                add (BASS, b0 + k, 0.9f, n, 105); last = k; ++ln;
            }
        }
        else   // BOOM BAP
        {
            static const int kicks[4][3] = { { 0, 10, -1 }, { 0, 7, 10 }, { 0, 10, -1 }, { 0, 7, 9 } };
            for (int k : kicks[bar]) if (k >= 0) add (KICK, b0 + k * 0.25f, 0.25f, 36, 108);
            add (SNARE, b0 + 1, 0.25f, 38, 110); add (SNARE, b0 + 3, 0.25f, 38, 110);
            for (int st = 0; st < 16; st += 2) add (HATS, b0 + st * 0.25f + (st % 4 == 2 ? 0.06f : 0.0f), 0.125f, 42, st % 4 == 0 ? 95 : 70);
            add (PERC, b0 + 3.5f, 0.25f, 46, 70);
            for (int k = 0; k < 3 && kicks[bar][k] >= 0; ++k) add (BASS, b0 + kicks[bar][k] * 0.25f, 0.45f, scaleNote (key, minor, deg + (k == 2 ? 4 : 0), 1), 100);
        }
        // melodia: accordo arpeggiato sulla scala, contro-melodia lunga, chop
        const int tri[4] = { 0, 2, 4, 2 };
        for (int k = 0; k < 4; ++k) add (MEL, b0 + k, s == BOOMBAP ? 0.45f : 0.9f, scaleNote (key, minor, deg + tri[k], 4), k == 0 ? 100 : 84);
        if (s != BOOMBAP) add (MEL, b0 + 3.5f, 0.45f, scaleNote (key, minor, deg + 4, 4), 76);
        add (COUNTER, b0, 1.9f, scaleNote (key, minor, deg + 2, 5), 70); add (COUNTER, b0 + 2, 1.9f, scaleNote (key, minor, deg + (bar % 2 ? 1 : 4), 5), 66);
        if (bar % 2) add (CHOP, b0 + 3.25f, 0.25f, 60 + (bar == 3 ? 2 : 0), 90);
    }
    S.rules = { { BASS, MEL, DIRADA, false }, { SNARE, HATS, DIRADA, true }, { BASS, HATS, ROLL, true }, { COUNTER, CHOP, RISPONDE, true } };
    return S;
}

// ------------------------------------------------------------------------------------------------ una battuta suonata
inline float metricWeight (float tInBar)
{
    const float s = tInBar * 4; const bool on16 = std::abs (s - std::round (s)) < 0.05f; const int st = (int) std::round (s) % 16;
    if (! on16) return 0.25f; if (st == 0) return 1.0f; if (st % 4 == 0) return 0.85f; if (st % 2 == 0) return 0.6f; return 0.4f;
}
inline void renderBar (const Snap& S, const Macros& M, juce::uint32 seed, int absBar, BarEvents& out)
{
    out.n = 0;
    const int pb = ((absBar % BARS) + BARS) % BARS, phrase = ((absBar % 8) + 8) % 8; const float b0 = (float) pb * BPB;
    const bool phraseEnd = phrase == 7, barOdd = phrase % 2 == 1;
    BarEvents tmp; tmp.n = 0;
    static const float laneImp[NL] = { 0.35f, 0.4f, 0.0f, 0.0f, 0.3f, 0.15f, 0.0f, 0.0f };
    for (int l = 0; l < NL; ++l)
    {
        if (S.mute[(size_t) l]) continue;
        const bool canVar = S.var[(size_t) l];
        // SPACE: le corsie si tolgono in ordine
        const float sp = M.space;
        if ((l == PERC && sp > 0.2f) || (l == COUNTER && sp > 0.35f) || (l == CHOP && sp > 0.6f) || (l == HATS && sp > 0.5f && barOdd) || (l == HATS && sp > 0.8f)) continue;
        const auto& notes = S.lane[(size_t) l];
        for (size_t i = 0; i < notes.size(); ++i)
        {
            const auto& n = notes[i]; if (n.t < b0 - 1e-4f || n.t >= b0 + BPB - 1e-4f) continue;
            Ev e { l, n.note, n.vel, n.t - b0, n.len };
            const float imp = juce::jlimit (0.0f, 1.0f, metricWeight (e.t) * 0.7f + n.vel / 127.0f * 0.3f + laneImp[l]);
            // DENSITY sotto il centro: toglie le note meno importanti
            if (M.density < 0.5f) { const float keep = 1.0f - (0.5f - M.density) * 2.0f * (1.0f - imp) * 1.6f; if (rnd01 (hsh (seed, (juce::uint32) l, (juce::uint32) phrase, (juce::uint32) i)) > keep) continue; }
            if (sp > 0.7f && l == MEL && metricWeight (e.t) < 0.8f) continue;
            if (sp > 0.85f && l == KICK && metricWeight (e.t) < 0.8f) continue;
            // MOVEMENT: spostamenti, ottave
            if (canVar && M.movement > 0)
            {
                const float r = rnd01 (hsh (seed, 100u + (juce::uint32) l, (juce::uint32) phrase, (juce::uint32) i));
                if (l == KICK && metricWeight (e.t) < 0.8f && r < M.movement * 0.35f) e.t = juce::jlimit (0.0f, 3.75f, e.t + (r < M.movement * 0.17f ? -0.25f : 0.25f));
                if ((l == MEL || l == COUNTER) && r < M.movement * 0.25f) e.note += (r < M.movement * 0.12f ? 12 : -12);
                if (l == BASS && r < M.movement * 0.2f && M.energy > 0.6f) e.note += 12;
            }
            // ENERGY: dinamica e accenti
            const float acc = metricWeight (e.t) >= 0.85f ? 1.1f : 0.95f;
            e.vel = juce::jlimit (1, 127, (int) std::round (n.vel * (0.55f + 0.9f * M.energy) * acc));
            tmp.add (e);
        }
        // DENSITY sopra il centro: aggiunte (ghost, hi-hat sui sedicesimi, ripetizioni)
        if (M.density > 0.5f && canVar)
        {
            const float d = (M.density - 0.5f) * 2.0f;
            for (int st = 0; st < 16; ++st)
            {
                const float t = st * 0.25f; const float r = rnd01 (hsh (seed, 200u + (juce::uint32) l, (juce::uint32) phrase, (juce::uint32) st));
                bool occupied = false; for (int k = 0; k < tmp.n; ++k) if (tmp.ev[k].lane == l && std::abs (tmp.ev[k].t - t) < 0.12f) occupied = true;
                if (occupied) continue;
                if (l == HATS && st % 2 == 1 && r < d * 0.8f) tmp.add ({ HATS, 42, juce::jlimit (1, 127, (int) (60 * (0.6f + 0.6f * M.energy))), t, 0.1f });
                else if (l == KICK && st % 4 != 0 && r < d * 0.10f) tmp.add ({ KICK, 36, 70, t, 0.2f });
                else if (l == PERC && st % 2 == 0 && r < d * 0.25f) tmp.add ({ PERC, 39, 65, t, 0.2f });
            }
        }
        // MOVEMENT: roll di hi-hat in un battito a caso, fill a fine frase
        if (l == HATS && canVar && sp <= 0.8f)
        {
            const float r = rnd01 (hsh (seed, 300u, (juce::uint32) phrase));
            if (r < M.movement * 0.7f || (phraseEnd && M.movement > 0.15f))
            {
                const float beat = phraseEnd ? 3.0f : (float) (hsh (seed, 301u, (juce::uint32) phrase) % 4);
                const bool trip = rnd01 (hsh (seed, 302u, (juce::uint32) phrase)) < 0.5f; const float step = trip ? 1.0f / 6.0f : (M.energy > 0.7f ? 0.125f : 0.25f);
                int k = 0; for (int q = 0; q < tmp.n; ++q) if (! (tmp.ev[q].lane == HATS && tmp.ev[q].t >= beat - 1e-3f && tmp.ev[q].t < beat + 1)) tmp.ev[k++] = tmp.ev[q]; tmp.n = k;
                for (float t = beat; t < beat + 1 - 1e-3f; t += step) tmp.add ({ HATS, 42, juce::jlimit (1, 127, (int) (55 + 50 * (t - beat) * M.energy + 30 * M.energy)), t, step * 0.8f });
            }
        }
        if (l == PERC && canVar && phraseEnd && M.movement > 0.3f && sp <= 0.2f) for (float t = 3.0f; t < 4; t += 0.25f) tmp.add ({ PERC, 39, 70 + (int) ((t - 3) * 40), t, 0.2f });
    }
    // regole fra corsie
    for (const auto& R : S.rules)
    {
        if (! R.on || R.from == R.to) continue;
        auto src = [&] (float t, float before, float after) { for (int k = 0; k < tmp.n; ++k) if (tmp.ev[k].lane == R.from && tmp.ev[k].t >= t - before && tmp.ev[k].t <= t + after) return true; return false; };
        int k = 0;
        for (int q = 0; q < tmp.n; ++q)
        {
            Ev e = tmp.ev[q]; bool keep = true;
            if (e.lane == R.to)
            {
                if (R.type == DIRADA && src (e.t, 0.12f, 0.12f)) keep = false;
                if (R.type == RISPONDE && src (e.t, 0.5f, 0.1f)) keep = false;
                if (R.type == SEGUE)
                {
                    int c = 0; for (int z = 0; z < tmp.n; ++z) if (tmp.ev[z].lane == R.from && std::floor (tmp.ev[z].t) == std::floor (e.t)) ++c;
                    e.vel = juce::jlimit (1, 127, (int) (e.vel * (0.6f + 0.15f * juce::jmin (c, 4))));
                }
            }
            if (keep) tmp.ev[k++] = e;
        }
        tmp.n = k;
        if (R.type == ROLL)
        {
            int prev = -1; const int cnt = tmp.n;
            for (int q = 0; q < cnt; ++q) if (tmp.ev[q].lane == R.from)
            {
                if (prev >= 0 && tmp.ev[q].note != prev) { const float t0 = tmp.ev[q].t; for (int z = 0; z < 4; ++z) tmp.add ({ R.to, R.to == HATS ? 42 : 39, 60 + z * 12, t0 + z * 0.125f, 0.1f }); }
                prev = tmp.ev[q].note;
            }
        }
    }
    // SPACE alto: pausa sull'ultimo battito della frase (tutto si ferma tranne l'808)
    for (int q = 0; q < tmp.n; ++q)
    {
        const auto& e = tmp.ev[q];
        if (M.space > 0.4f && phraseEnd && e.t >= 3.0f && e.lane != BASS) continue;
        if (e.t < 0 || e.t >= BPB) continue;
        out.add (e);
    }
}
}
