#pragma once
// Generatore: dal giro di accordi e dalle impostazioni a una parte suonabile (note con corda e tasto, curve di bend).
#include "GtrEngine.h"

namespace gtr
{
struct Rng
{
    juce::uint32 s;
    explicit Rng (juce::uint32 seed) : s (seed * 2654435761u + 0x9e3779b9u) { if (s == 0) s = 1; for (int i = 0; i < 4; ++i) next(); }
    juce::uint32 next() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; }
    float f() { return (float) (next() >> 8) * (1.0f / 16777216.0f); }
    int i (int n) { return n <= 0 ? 0 : (int) (next() % (juce::uint32) n); }
    float g() { return (f() + f() + f() + f() - 2.0f) * 1.7320508f; }   // circa normale, deviazione 1
    bool p (float pr) { return f() < pr; }
};

// accordo di uno slot: grado 0..6 diatonico, 10..16 preso in prestito
inline Chord slotChord (const Settings& S, int code)
{
    const int col = S.color < 0 ? genreColor (S.genre) : juce::jlimit (0, 3, (int) S.color);
    if (code >= 10) return borrowedChord (S.key, S.scale, code - 10, col);
    return diatonic (S.key, S.scale, juce::jlimit (0, 6, code), col);
}
// il prestito e' possibile su questo grado?
inline bool canBorrow (int scale, int deg)
{
    if (scale == ARMONICA) return false;
    if (isMinorScale (scale)) return deg == 4 || (scale == MINORE && deg == 3);
    return deg == 3 || (scale == MAGGIORE && deg == 6);
}
inline std::array<bool, 12> scaleSet (int key, int scale) { std::array<bool, 12> a {}; for (int i = 0; i < 7; ++i) a[(size_t) ((key + scaleSteps (scale)[i]) % 12)] = true; return a; }
inline std::array<bool, 12> pentaSet (int key, int scale)
{
    std::array<bool, 12> a {}; const int* st = scaleSteps (scale);
    static const int mi[5] = { 0, 2, 3, 4, 6 }, ma[5] = { 0, 1, 2, 4, 5 };
    for (int i = 0; i < 5; ++i) a[(size_t) ((key + st[isMinorScale (scale) ? mi[i] : ma[i]]) % 12)] = true;
    return a;
}

struct Gen
{
    const Settings& S; Part& P; std::array<int, kStr> tun; Rng hr, mr; double slotLen = 4; int nSlots = 4;
    std::array<bool, 12> inScale {}, penta {};
    std::vector<std::array<bool, 12>> allow;   // note ammesse per ogni slot (scala, adattata all'accordo preso in prestito)
    int grpN = 0;
    Gen (const Settings& s, Part& p) : S (s), P (p), tun (tuning (s.tuning)), hr (s.seed ^ 0x51ed27u), mr (s.seed) {}
    double ms (double m) const { return m / 1000.0 * S.bpm / 60.0; }
    int slotAt (double t) const { return juce::jlimit (0, nSlots - 1, (int) std::floor (t / slotLen + 1e-9)); }
    double slotEnd (int i) const { return (i + 1) * slotLen; }
    // swing sulle sedicesime (o sugli ottavi) in levare
    double sw (double t, double grid) const
    {
        if (S.swing <= 0.001f || grid < 0.2 || std::abs (grid - 1.0 / 3.0) < 0.01) return t;
        const double fr = t - std::floor (t / (grid * 2)) * grid * 2;
        return std::abs (fr - grid) < 1e-6 ? t + S.swing * grid / 3.0 : t;
    }
    double hum() { return (double) hr.g() * S.human * 0.016; }
    float hv() { return hr.g() * S.human * 0.07f; }
    float vAcc (bool acc) const { return acc ? 0.72f + 0.24f * S.dyn : 0.70f - 0.12f * S.dyn; }
    std::vector<int> sounding (int slot) const { std::vector<int> v; for (int s = 0; s < kStr; ++s) if (P.voic[(size_t) slot].fret[(size_t) s] >= 0) v.push_back (s); return v; }
    void add (double t, double len, int s, int fret, float vel, int slot, bool mute = false, bool pm = false, int art = 0)
    {
        Ev e; e.beat = t; e.len = len; e.str = s; e.fret = fret; e.note = tun[(size_t) s] + fret; e.vel = juce::jlimit (0.06f, 1.0f, vel); e.slot = slot; e.mute = mute; e.pm = pm; e.art = art; e.grp = grpN;
        P.ev.push_back (e);
    }
    // pennata: dir +1 giu' (dalla corda grave), -1 su (dalla corda acuta)
    void strum (double t, int slot, int dir, std::vector<int> strs, float vel, double len, int kind = 0)
    {
        if (strs.empty()) return;
        ++grpN;
        if (dir < 0) std::reverse (strs.begin(), strs.end());
        const double total = ms ((dir > 0 ? 7.0 + S.strum * 34.0 : 5.0 + S.strum * 22.0) * (strs.size() - 1) / 5.0) * (kind == 1 ? 0.6 : 1.0);
        const double step = strs.size() > 1 ? total / (double) (strs.size() - 1) : 0.0;
        double tt = t + hum(); const float gv = hv();
        for (size_t k = 0; k < strs.size(); ++k)
        {
            const int s = strs[k], f = P.voic[(size_t) slot].fret[(size_t) s]; if (f < 0) continue;
            float v = vel * (1.0f - 0.035f * (float) k) * (dir < 0 ? 0.84f : 1.0f) + gv + hr.g() * 0.025f;
            if (kind == 1) v *= 0.55f;
            add (juce::jmax (0.0, tt), len, s, f, v, slot, kind == 1, kind == 2);
            tt += step * (0.75 + 0.5 * hr.f());
        }
    }
    void pluck (double t, int slot, int s, float vel, double len, bool mute = false)
    {
        const int f = P.voic[(size_t) slot].fret[(size_t) s]; if (f < 0) return;
        ++grpN; add (juce::jmax (0.0, t + hum()), len, s, f, vel + hv(), slot, mute);
    }
    // ruoli delle corde nell'accordo: B basso, A basso alternato, 1-2-3 le tre corde acute (3 = la piu' alta), 4 la quarta dall'alto
    int role (int slot, char r) const
    {
        const auto v = sounding (slot); const int n = (int) v.size(); if (n == 0) return -1;
        switch (r)
        {
            case 'B': return v[0];
            case 'A':
            {
                if (n < 2) return v[0];
                const auto& ch = P.chords[(size_t) slot];
                for (int i = 1; i < juce::jmin (3, n); ++i) { const int pc = (tun[(size_t) v[(size_t) i]] + P.voic[(size_t) slot].fret[(size_t) v[(size_t) i]]) % 12; if (pc == ch.fifth) return v[(size_t) i]; }
                return v[1];
            }
            case '3': return v[(size_t) (n - 1)];
            case '2': return v[(size_t) juce::jmax (0, n - 2)];
            case '1': return v[(size_t) juce::jmax (0, n - 3)];
            case '4': return v[(size_t) juce::jmax (0, n - 4)];
            default: return -1;
        }
    }
    bool ok (int slot, int midi) const { return allow[(size_t) slot][(size_t) (midi % 12)]; }

    // --------------------------------------------------------------------------------------------- ACCORDI
    void accordi()
    {
        const int pat = S.pattern;
        for (int i = 0; i < nSlots; ++i)
        {
            const double t0 = i * slotLen, L = slotLen, end = slotEnd (i);
            auto all = sounding (i); if (all.empty()) continue;
            auto top = [&] (int k) { std::vector<int> v; for (int j = juce::jmax (0, (int) all.size() - k); j < (int) all.size(); ++j) v.push_back (all[(size_t) j]); return v; };
            switch (pat)
            {
                case 0:   // TENUTI
                    strum (t0, i, 1, all, vAcc (true), L);
                    if (S.density > 0.66f && L >= 2) strum (t0 + L / 2, i, 1, all, vAcc (false) * 0.85f, L / 2);
                    break;
                case 1:   // COLPI 1 E 3
                    for (double b = 0; b < L - 1e-6; b += 2)
                    {
                        strum (sw (t0 + b, 0.5), i, 1, all, vAcc (b == 0), 1.6);
                        if (S.density > 0.45f && b + 1.5 < L) strum (sw (t0 + b + 1.5, 0.5), i, -1, top (3), vAcc (false), 0.45);
                    }
                    if (S.density > 0.8f) P.chokes.push_back ({ t0 + L - 0.25 });
                    break;
                case 2:   // STAB R&B: colpi corti e sincopati, pizzicati insieme
                {
                    static const double pos[] = { 0, 0.75, 1.5, 2.5, 3.0, 3.75 }; static const float pr[] = { 1.0f, 0.5f, 0.8f, 0.6f, 0.35f, 0.5f };
                    for (int k = 0; k < 6; ++k)
                    {
                        if (pos[k] >= L - 1e-6) break;
                        if (k > 0 && ! mr.p (pr[k] * (0.35f + S.density))) continue;
                        const double t = sw (t0 + pos[k], 0.25); ++grpN; const double h = hum(); const float gv = hv();
                        for (size_t q = 0; q < all.size(); ++q) add (juce::jmax (0.0, t + h + ms (1.5 * (double) q)), 0.32, all[q], P.voic[(size_t) i].fret[(size_t) all[q]], vAcc (k == 0) + gv, i);
                    }
                    break;
                }
                case 3:   // NEO SOUL: accordo sfogliato, la nota alta arriva con hammer-on o scivolata
                {
                    ++grpN; const double h = hum();
                    const int topS = all.back(), topF = P.voic[(size_t) i].fret[(size_t) topS];
                    int grace = -1;
                    if (topF >= 2 && mr.p (0.35f + S.emb * 0.6f)) for (int d : { 2, 1 }) if (ok (i, tun[(size_t) topS] + topF - d)) { grace = topF - d; break; }
                    for (size_t q = 0; q < all.size(); ++q)
                    {
                        const int s = all[q]; const double t = juce::jmax (0.0, t0 + h + ms (14.0 * (double) q));
                        if (s == topS && grace >= 0) { add (t, 0.14, s, grace, vAcc (false), i); add (t + 0.125, L, s, topF, vAcc (false) * 0.8f, i, false, false, 1); }
                        else add (t, L, s, P.voic[(size_t) i].fret[(size_t) s], vAcc (q == 0), i);
                    }
                    if (S.density > 0.5f && L >= 4)
                    {   // risposta sulle due corde alte a meta' battuta, con scivolata se c'e' spazio
                        const double t = sw (t0 + 2.5, 0.5); const auto tp = top (2);
                        for (int s : tp)
                        {
                            const int f = P.voic[(size_t) i].fret[(size_t) s];
                            int gr = -1; if (s == topS && f >= 3 && S.emb > 0.3f) for (int d : { 2, 1 }) if (ok (i, tun[(size_t) s] + f - d)) { gr = f - d; break; }
                            if (gr >= 0 && mr.p (S.emb)) { add (t + hum(), 0.1, s, gr, vAcc (false) * 0.8f, i); add (t + 0.1, L - 2.6, s, f, vAcc (false) * 0.7f, i, false, false, 2); }
                            else add (t + hum(), L - 2.5, s, f, vAcc (false) * 0.8f, i);
                        }
                    }
                    break;
                }
                case 4:   // ROLL LENTO
                {
                    ++grpN; const double h = hum(); const double st = ms (55.0 + 70.0 * (1.0 - S.density));
                    for (size_t q = 0; q < all.size(); ++q) add (juce::jmax (0.0, t0 + h + st * (double) q), L, all[q], P.voic[(size_t) i].fret[(size_t) all[q]], vAcc (q == 0) * (1.0f - 0.03f * (float) q), i);
                    if (S.density > 0.6f && L >= 4) { ++grpN; for (size_t q = 0; q < all.size(); ++q) { const size_t r = all.size() - 1 - q; add (t0 + 2.5 + st * 0.7 * (double) q, L - 2.5, all[r], P.voic[(size_t) i].fret[(size_t) all[r]], vAcc (false) * 0.75f, i); } }
                    break;
                }
                default:  // SPINTE: l'accordo arriva un ottavo prima della battuta
                {
                    const double t = i == 0 ? 0.0 : t0 - 0.5;
                    strum (t, i, 1, all, vAcc (true), end - t);
                    if (S.density > 0.35f && L >= 2) strum (sw (t0 + 1.5, 0.5), i, -1, top (3), vAcc (false), 0.5);
                    if (S.density > 0.7f && L >= 4) strum (sw (t0 + 2.5, 0.5), i, 1, all, vAcc (false), 0.8);
                    break;
                }
            }
        }
    }

    // --------------------------------------------------------------------------------------------- RITMICA
    struct RPat { int steps; const char* v[3]; };
    static const RPat& rpat (int p)
    {
        static const RPat R[] = {
            { 16, { "D---D---D---D-U-", "D---D-U---U-D-U-", "D-u-D-U-g-U-D-U-" } },          // POP 8
            { 16, { "D-------D-------", "D-----U-D---D-U-", "D-u-D-U-d-u-D-U-" } },          // BALLATA
            { 16, { "----D.------D.--", "----D.--g---D.-u", "--U.--U.--U.--U." } },          // REGGAE
            { 16, { "D--U----D--U----", "D--U--D.D--U--D.", "D-gU-gD-D-gU-gD-" } },          // REGGAETON (dembow)
            { 16, { "D.g-g-g-D.g-g-g-", "D.ggggU.ggggD.gg", "D.gUggU.gUgDg.gU" } },          // FUNK 16
            { 16, { "p-p-p-p-p-p-p-p-", "p-p-p-p-D---p-p-", "p-ppp-ppp-ppD---" } },          // PALM MUTE
            { 16, { "D--U----D--U----", "D--U--U-D-U--U--", "D-gU-gU-D-UgdU-g" } },          // AFRO
            { 12, { "D-----D-----", "D--D-UD--D-U", "D-uDuUD-uDuU" } },                      // 6/8
            { 16, { "b---D---B---D---", "b---D-U-B---D-U-", "b---D-UuB-u-D-U-" } } };        // FOLK
        return R[juce::jlimit (0, 8, p)];
    }
    void ritmica()
    {
        const auto& R = rpat (S.pattern); const double st = 4.0 / R.steps;
        const bool tripl = R.steps == 12;
        for (int b = 0; b < P.bars; ++b)
        {
            int var = S.density < 0.34f ? 0 : S.density < 0.67f ? 1 : 2;
            if (P.bars >= 4 && b % 4 == 3 && S.density > 0.25f) var = juce::jmin (2, var + 1);   // ultima battuta della frase: un po' piu' piena
            const char* pat = R.v[var];
            for (int k = 0; k < R.steps; ++k)
            {
                const char c = pat[k]; if (c == '-') continue;
                const double t = sw (b * 4.0 + k * st, tripl ? 1.0 / 3.0 : 0.25);
                const int i = slotAt (b * 4.0 + k * st + 1e-6); auto all = sounding (i); if (all.empty()) continue;
                const double rest = slotEnd (i) - t;
                const bool beat = (k * st) == std::floor (k * st);
                auto top = [&] (int n) { std::vector<int> v; for (int j = juce::jmax (0, (int) all.size() - n); j < (int) all.size(); ++j) v.push_back (all[(size_t) j]); return v; };
                auto low = [&] (int n) { std::vector<int> v; for (int j = 0; j < juce::jmin (n, (int) all.size()); ++j) v.push_back (all[(size_t) j]); return v; };
                switch (c)
                {
                    case 'D': strum (t, i, 1, all, vAcc (true), rest); break;
                    case 'd': strum (t, i, 1, all.size() >= 5 ? top ((int) all.size() - 1) : all, vAcc (beat) * 0.92f, rest); break;
                    case 'U': strum (t, i, -1, top (4), vAcc (true) * 0.95f, rest); break;
                    case 'u': strum (t, i, -1, top (3), vAcc (false), rest); break;
                    case 'g': { const bool down = (k % 2) == 0; strum (t, i, down ? 1 : -1, down ? all : top (4), vAcc (false), 0.06, 1); break; }
                    case 'p': strum (t, i, 1, low (S.vstyle == V_POWER ? 3 : 2), vAcc (beat) * 0.85f, 0.2, 2); break;
                    case 'b': pluck (t, i, role (i, 'B'), vAcc (true), rest); break;
                    case 'B': pluck (t, i, role (i, 'A'), vAcc (true) * 0.92f, rest); break;
                    case '.': P.chokes.push_back ({ t + ms (8) }); break;
                    default: break;
                }
            }
        }
    }

    // --------------------------------------------------------------------------------------------- ARPEGGIO
    struct APat { int steps; const char* t; };
    static const APat& apat (int p)
    {
        static const APat A[] = {
            { 16, "B A 1 2 3 2 1 A B A 1 2 3 2 1 A" },      // SALITA
            { 8,  "B 1 2 3 2 1 3 2" },                      // TRAP TRISTE
            { 8,  "B3 2 A 1 B 3 A 2" },                     // TRAVIS (pollice alternato sui tempi, dita in levare)
            { 16, "B 1 2 3 4 1 2 3 A 1 2 3 4 1 2 3" },      // PIMA
            { 12, "B 1 2 3 2 1 A 1 2 3 2 1" },              // 6/8 EMO
            { 12, "B 3 2 - 3 1 A 3 2 - 3 1" },              // TERZINE DRILL
            { 16, "B 1 2 3 - - 3 2 - - 1 - A - 2 -" },      // R&B ROLL
            { 16, "B - 3 2 - 3 1 - A - 3 2 - 3 1 -" },      // HIGHLIFE
            { 16, "B 2 3 2 A 2 3 2 B 2 3 2 A 2 3 2" } };    // CLASSICO
        return A[juce::jlimit (0, 8, p)];
    }
    void arpeggio()
    {
        const auto& A = apat (S.pattern); const auto tok = juce::StringArray::fromTokens (A.t, " ", "");
        const double st = 4.0 / A.steps; const bool tripl = A.steps == 12;
        const double ringLen = st * (1.0 + S.ring * 14.0);
        for (int b = 0; b < P.bars; ++b)
            for (int k = 0; k < tok.size(); ++k)
            {
                const auto& tk = tok[k]; if (tk == "-") continue;
                const double tb = b * 4.0 + k * st, t = sw (tb, tripl ? 1.0 / 3.0 : st);
                const int i = slotAt (tb + 1e-6);
                const bool hasBass = tk.containsChar ('B') || tk.containsChar ('A');
                // densita' bassa: si tolgono note acute (mai il basso); alta: qualche sedicesimo in piu'
                if (! hasBass && S.density < 0.5f && mr.p ((0.5f - S.density) * 1.3f)) continue;
                ++grpN; const double h = hum(); const float gv = hv();
                for (auto c : tk)
                {
                    const int s = role (i, (char) c); if (s < 0) continue;
                    const int f = P.voic[(size_t) i].fret[(size_t) s];
                    const bool bass = c == 'B' || c == 'A';
                    const float v = (bass ? vAcc (k == 0) : vAcc (false) * (0.92f + 0.08f * (float) (c - '0') / 3.0f)) + gv;
                    const double tt = juce::jmax (0.0, t + h);
                    // abbellimento: hammer-on da una nota della scala sotto, sulla stessa corda
                    if (! bass && f >= 1 && S.emb > 0.01f && mr.p (S.emb * 0.22f))
                    {
                        int g = -1; for (int d : { 2, 1 }) if (f - d >= 0 && ok (i, tun[(size_t) s] + f - d)) { g = f - d; break; }
                        if (g >= 0) { const double gl = juce::jmin (0.125, st * 0.5); add (tt, gl + 0.02, s, g, v * 0.95f, i); add (tt + gl, ringLen, s, f, v * 0.78f, i, false, false, 1); continue; }
                    }
                    add (tt, bass ? ringLen * 1.5 : ringLen, s, f, v, i);
                }
                if (S.density > 0.75f && A.steps == 8 && mr.p ((S.density - 0.75f) * 1.6f))
                {   // sedicesimo di passaggio sulla corda vicina
                    const int s = role (i, mr.p (0.5f) ? '2' : '3'); if (s >= 0) pluck (t + st * 0.5, i, s, vAcc (false) * 0.8f, ringLen * 0.6);
                }
            }
    }

    // --------------------------------------------------------------------------------------------- LEAD
    struct Cell { std::vector<std::pair<double, double>> n; };
    std::vector<Cell> cells (int style)
    {
        auto C = [] (std::initializer_list<std::pair<double, double>> l) { Cell c; c.n = l; return c; };
        switch (style)
        {
            case 1: return { C ({ { 0, 1.5 }, { 1.5, 0.5 }, { 2, 2 } }), C ({ { 0, 0.75 }, { 0.75, 0.75 }, { 1.5, 2.5 } }), C ({ { 0.5, 0.5 }, { 1, 3 } }),
                             C ({ { 0, 1.0 / 3 }, { 1.0 / 3, 1.0 / 3 }, { 2.0 / 3, 1.0 / 3 }, { 1, 3 } }), C ({ { 0, 2 }, { 2, 0.5 }, { 2.5, 0.5 }, { 3, 1 } }) };   // TRAP SCURO
            case 2: return { C ({ { 0, 2.0 / 3 }, { 2.0 / 3, 2.0 / 3 }, { 4.0 / 3, 2.0 / 3 }, { 2, 2 } }), C ({ { 0, 1.0 / 3 }, { 1.0 / 3, 1.0 / 3 }, { 2.0 / 3, 1.0 / 3 }, { 1, 1.0 / 3 }, { 4.0 / 3, 2.0 / 3 }, { 2, 1 }, { 3, 1 } }),
                             C ({ { 0, 1 }, { 1, 2.0 / 3 }, { 5.0 / 3, 1.0 / 3 }, { 2, 2 } }), C ({ { 0.5, 0.5 }, { 1, 2.0 / 3 }, { 5.0 / 3, 2.0 / 3 }, { 7.0 / 3, 5.0 / 3 } }) };   // DRILL
            case 3: return { C ({ { 0, 1.5 }, { 1.5, 0.25 }, { 1.75, 0.25 }, { 2, 0.25 }, { 2.25, 0.25 }, { 2.5, 1.5 } }), C ({ { 0, 0.5 }, { 0.5, 0.25 }, { 0.75, 0.25 }, { 1, 1 }, { 2, 0.25 }, { 2.25, 0.25 }, { 2.5, 0.25 }, { 2.75, 0.25 }, { 3, 1 } }),
                             C ({ { 0, 2 }, { 2.5, 0.25 }, { 2.75, 0.25 }, { 3, 0.25 }, { 3.25, 0.25 }, { 3.5, 0.5 } }), C ({ { 0.5, 0.5 }, { 1, 0.5 }, { 1.5, 0.5 }, { 2, 2 } }) };   // R&B RUN
            case 4: return { C ({ { 0, 0.5 }, { 0.5, 0.5 }, { 1, 1.5 }, { 2.5, 0.5 }, { 3, 1 } }), C ({ { 0, 1.0 / 3 }, { 1.0 / 3, 1.0 / 3 }, { 2.0 / 3, 1.0 / 3 }, { 1, 1 }, { 2, 2 } }),
                             C ({ { 0, 1.5 }, { 1.5, 0.5 }, { 2, 0.5 }, { 2.5, 1.5 } }), C ({ { 0.5, 0.5 }, { 1, 0.5 }, { 1.5, 0.5 }, { 2, 1 }, { 3, 1 } }) };   // BLUES
            case 5: return { C ({ { 0, 0.75 }, { 0.75, 0.75 }, { 1.5, 0.5 }, { 2, 0.75 }, { 2.75, 0.75 }, { 3.5, 0.5 } }), C ({ { 0, 0.75 }, { 0.75, 0.75 }, { 1.5, 2.5 } }),
                             C ({ { 0, 0.5 }, { 0.5, 0.5 }, { 1, 0.5 }, { 1.5, 0.5 }, { 2, 2 } }), C ({ { 0.5, 0.25 }, { 0.75, 0.75 }, { 1.5, 0.5 }, { 2, 2 } }) };   // LATIN
            default: return { C ({ { 0, 1 }, { 1, 0.5 }, { 1.5, 0.5 }, { 2, 1.5 } }), C ({ { 0, 0.5 }, { 0.5, 0.5 }, { 1, 1 }, { 2.5, 0.5 }, { 3, 1 } }),
                              C ({ { 0.5, 0.5 }, { 1, 0.5 }, { 1.5, 1.5 }, { 3, 0.5 }, { 3.5, 0.5 } }), C ({ { 0, 1.5 }, { 1.5, 0.5 }, { 2, 0.5 }, { 2.5, 0.5 }, { 3, 1 } }) };   // MELODIA
        }
    }
    void lead()
    {
        const int style = S.pattern;
        const int lp = juce::jlimit (0, kFrets - 5, S.pos);
        // note nel box: corde 1..5 (la sesta solo per il DRILL, scuro), tasti lp-1 .. lp+4
        int lo = 999, hi = 0;
        for (int s = style == 2 ? 0 : 1; s < kStr; ++s) { lo = juce::jmin (lo, tun[(size_t) s] + juce::jmax (0, lp - 1)); hi = juce::jmax (hi, tun[(size_t) s] + lp + 4); }
        const bool pent = style == 1 || style == 4 || style == 2;
        auto poolFor = [&] (int slot, bool chordOnly)
        {
            std::vector<int> v; const auto& ch = P.chords[(size_t) slot];
            for (int m = lo; m <= hi; ++m)
            {
                const int pc = m % 12; if (! ok (slot, m)) continue;
                bool ct = false; for (int t : ch.tones) ct |= t == pc;
                if (chordOnly && ! ct) continue;
                if (! chordOnly && pent && ! penta[(size_t) pc] && ! ct) continue;
                v.push_back (m);
            }
            return v;
        };
        const auto cl = cells (style);
        // motivo: ritmo A e risposta B; contorno di A salvato e ripetuto a sequenza
        const int cA = mr.i ((int) cl.size()); int cB = mr.i ((int) cl.size()); if (cB == cA) cB = (cB + 1) % (int) cl.size();
        std::vector<int> contour; int prev = (lo + hi) / 2 + 2;
        struct LN { double t, d; int m; bool strong; };
        std::vector<LN> notes;
        for (int b = 0; b < P.bars; ++b)
        {
            const int ph = b % 4; const bool last = b == P.bars - 1;
            Cell c = cl[(size_t) (ph == 1 ? cB : (ph == 3 || last) ? (cA + 2) % (int) cl.size() : cA)];
            if (last || (P.bars >= 4 && ph == 3))
            {   // chiusura: l'ultima nota cade su un tempo forte e dura
                while (c.n.size() > 1 && c.n.back().first > 2.01) c.n.pop_back();
                c.n.back().second = 4.0 - c.n.back().first;
            }
            // densita': bassa toglie note di passaggio, alta divide le lunghe
            Cell d;
            for (auto& n : c.n)
            {
                if (S.density < 0.4f && n.first != std::floor (n.first) && d.n.size() > 0 && mr.p ((0.4f - S.density) * 1.6f)) { d.n.back().second += n.second; continue; }
                if (S.density > 0.7f && n.second >= 1.0 && n.first + n.second < 3.9 && mr.p ((S.density - 0.7f) * 2.0f)) { d.n.push_back ({ n.first, n.second * 0.5 }); d.n.push_back ({ n.first + n.second * 0.5, n.second * 0.5 }); continue; }
                d.n.push_back (n);
            }
            const bool repeatA = (ph == 2) && ! contour.empty();
            int ci = 0;
            for (size_t k = 0; k < d.n.size(); ++k)
            {
                const double t = b * 4.0 + d.n[k].first; const int slot = slotAt (t + 1e-6);
                const bool strong = std::abs (d.n[k].first - std::round (d.n[k].first)) < 1e-6 && ((int) std::round (d.n[k].first) % 2 == 0);
                const bool endN = k + 1 == d.n.size() && (last || ph == 3);
                const auto pool = poolFor (slot, false), ct = poolFor (slot, true);
                if (pool.empty()) continue;
                auto nearestIn = [] (const std::vector<int>& v, int m) { int best = v.front(); for (int x : v) if (std::abs (x - m) < std::abs (best - m)) best = x; return best; };
                auto idxOf = [&] (int m) { int bi = 0; for (int q = 0; q < (int) pool.size(); ++q) if (std::abs (pool[(size_t) q] - m) < std::abs (pool[(size_t) bi] - m)) bi = q; return bi; };
                int m;
                if (endN)
                {   // fine frase: fondamentale (o terza) dell'accordo vicino all'ultima nota
                    std::vector<int> rt; for (int x : ct) if (x % 12 == P.chords[(size_t) slot].root || (x % 12 == P.chords[(size_t) slot].third && mr.p (0.3f))) rt.push_back (x);
                    m = nearestIn (rt.empty() ? ct : rt, prev);
                }
                else if (repeatA && ci < (int) contour.size())
                {
                    m = pool[(size_t) juce::jlimit (0, (int) pool.size() - 1, idxOf (prev) + contour[(size_t) ci])]; ++ci;
                    if (strong && ! ct.empty()) m = nearestIn (ct, m);
                }
                else if (notes.empty() || (strong && ! ct.empty() && mr.p (0.8f)) || d.n[k].second >= 1.5)
                    m = ct.empty() ? pool[(size_t) idxOf (prev)] : nearestIn (ct, prev + (mr.p (0.5f) ? 2 : -2));
                else
                {   // passo: per grado congiunto quasi sempre, salti piccoli, attrazione verso il centro
                    const float r = mr.f(); int stp = r < 0.62f ? 1 : r < 0.88f ? 2 : 3;
                    const int centre = (lo + hi) / 2 + 3; int dir = prev > centre + 5 ? -1 : prev < centre - 5 ? 1 : (mr.p (0.5f) ? 1 : -1);
                    if (notes.size() >= 2) { const int last2 = notes[notes.size() - 2].m; if (std::abs (prev - last2) > 4) dir = prev > last2 ? -1 : 1; }   // dopo un salto si torna indietro
                    m = pool[(size_t) juce::jlimit (0, (int) pool.size() - 1, idxOf (prev) + dir * stp)];
                }
                if (m == prev && ! endN && ! notes.empty() && mr.p (0.7f))
                {   // niente note ribattute a vuoto: si va alla nota vicina (dell'accordo sui tempi forti)
                    const int dir = prev > (lo + hi) / 2 + 3 ? -1 : 1; const auto& src = strong && ct.size() > 1 ? ct : pool;
                    int alt = m; for (int x : src) if ((dir > 0 && x > m && (alt == m || x < alt)) || (dir < 0 && x < m && (alt == m || x > alt))) alt = x;
                    m = alt;
                }
                if (ph == 0 && ! notes.empty() && b < 4) contour.push_back (idxOf (m) - idxOf (prev));
                else if (ph == 0 && notes.empty()) {}
                notes.push_back ({ t, d.n[k].second, m, strong });
                prev = m;
            }
        }
        // corde e tasti: la mano resta nel box e si sposta poco
        double hand = lp + 1.5; int lastS = -1;
        struct Pos { int s, f; }; std::vector<Pos> pos;
        for (auto& n : notes)
        {
            Pos best { -1, 0 }; double bc = 1e9;
            for (int s = 0; s < kStr; ++s)
            {
                const int f = n.m - tun[(size_t) s]; if (f < 0 || f > kFrets) continue;
                double c = std::abs (f - hand) * 1.0 + (f == 0 ? 1.5 : 0.0) + (lastS >= 0 ? std::abs (s - lastS) * 0.35 : 0.0);
                if (f < lp - 1 || f > lp + 4) c += 4.0;
                if (c < bc) { bc = c; best = { s, f }; }
            }
            pos.push_back (best); if (best.s >= 0) { hand += (best.f - hand) * 0.25; lastS = best.s; }
        }
        // tecniche
        const double bendT = 0.14;
        for (size_t k = 0; k < notes.size(); ++k)
        {
            auto n = notes[k]; auto ps = pos[k]; if (ps.s < 0) continue;
            const int slot = slotAt (n.t + 1e-6); ++grpN;
            const double t = juce::jmax (0.0, n.t + hum()); const double nextT = k + 1 < notes.size() ? notes[k + 1].t : P.len();
            double len = juce::jmin (n.d, nextT - n.t) - 0.02;
            float v = (n.strong ? 0.74f : 0.62f) + 0.12f * S.dyn * (n.strong ? 1.0f : -0.5f) + hv();
            bool bent = false, legato = false;
            const float T = S.tech;
            // legato (hammer-on / pull-off) o scivolata fra note vicine sulla stessa corda
            if (k > 0 && pos[k - 1].s == ps.s && notes[k].t - (notes[k - 1].t + notes[k - 1].d) < 0.01)
            {
                const int iv = std::abs (n.m - notes[k - 1].m);
                const bool prevBent = std::any_of (P.bend.begin(), P.bend.end(), [&] (const Bend& b) { return b.beat > notes[k - 1].t && b.beat < n.t && b.semis != 0; });
                if (! prevBent && iv >= 1 && iv <= 3 && n.d <= 0.5 && mr.p (T * (style == 3 ? 0.8f : 0.45f))) legato = true;
                else if (! prevBent && iv >= 3 && iv <= 5 && mr.p (T * (style == 2 ? 0.5f : 0.2f))) legato = true;
            }
            // bend verso la nota (da una nota della scala sotto), solo sulle note lunghe e con la corda gia' premuta
            if (! legato && n.d >= 0.7 && T > 0.01f && mr.p (T * (style == 4 || style == 1 ? 0.6f : 0.3f)))
            {
                for (int amt : { 2, 1 })
                {
                    const int from = n.m - amt, f = ps.f - amt;
                    if (f >= 1 && ok (slot, from))
                    {
                        if (style == 1 && mr.p (0.4f) && ok (slot, n.m + amt) && ps.f + amt <= kFrets && amt == 2) break;   // sotto: pre-bend e rilascio
                        add (t, len, ps.s, f, v, slot); P.ev.back().lead = true;
                        const double b0 = t + 0.03 + 0.03 * hr.f(), bl = bendT + 0.06 * hr.f();
                        for (int q = 0; q <= 8; ++q) { const double x = q / 8.0; P.bend.push_back ({ b0 + bl * x, (float) (amt * (0.5 - 0.5 * std::cos (x * juce::MathConstants<double>::pi))), ps.s }); }
                        if (n.d >= 1.0 && mr.p (0.4f + T * 0.5f)) vibrato (b0 + bl + 0.1, t + len - 0.05, (float) amt, ps.s, 0.35f);
                        P.bend.push_back ({ t + len + 0.005, 0.0f, ps.s });
                        bent = true; break;
                    }
                }
                if (! bent && style == 1 && ps.f + 2 <= kFrets && ok (slot, n.m + 2) && n.d >= 1.0)
                {   // pre-bend: si sente prima la nota sopra, poi scende sulla nota
                    add (t, len, ps.s, ps.f, v, slot); P.ev.back().lead = true;
                    P.bend.push_back ({ t - 0.01, 2.0f, ps.s });
                    const double b0 = t + 0.25, bl = 0.25;
                    for (int q = 1; q <= 8; ++q) { const double x = q / 8.0; P.bend.push_back ({ b0 + bl * x, (float) (2.0 * (0.5 + 0.5 * std::cos (x * juce::MathConstants<double>::pi))), ps.s }); }
                    P.bend.push_back ({ t + len + 0.005, 0.0f, ps.s });
                    bent = true;
                }
            }
            if (bent) continue;
            // scivolata d'entrata (dopo una pausa), con il pitch bend: si arriva da un tono sotto
            if (! legato && (k == 0 || n.t - (notes[k - 1].t + notes[k - 1].d) > 0.2) && ps.f >= 2 && mr.p (T * (style == 2 || style == 5 ? 0.45f : 0.15f)))
            {
                add (t, len, ps.s, ps.f, v, slot); P.ev.back().lead = true;
                P.bend.push_back ({ t - 0.005, -2.0f, ps.s });
                for (int q = 1; q <= 6; ++q) { const double x = q / 6.0; P.bend.push_back ({ t + 0.1 * x, (float) (-2.0 * (1.0 - x)), ps.s }); }
                if (n.d >= 1.0 && mr.p (T)) { vibrato (t + 0.35, t + len - 0.05, 0.0f, ps.s, 0.25f); P.bend.push_back ({ t + len + 0.005, 0.0f, ps.s }); }
                continue;
            }
            if (legato) { auto& pe = P.ev.back(); if (pe.lead && pe.str == ps.s) pe.len = (t - pe.beat) + 0.03; }
            add (t, len, ps.s, ps.f, legato ? v * 0.8f : v, slot, false, false, legato ? (std::abs (n.m - notes[k - 1].m) >= 3 ? 2 : 1) : 0); P.ev.back().lead = true;
            if (n.d >= 1.0 && mr.p (T * 0.8f + 0.1f)) { vibrato (t + n.d * 0.35, t + len - 0.05, 0.0f, ps.s, 0.22f); P.bend.push_back ({ t + len + 0.005, 0.0f, ps.s }); }
            // doppie note: BLUES quarta sopra sulla corda vicina, LATIN terza della scala sotto
            if ((style == 4 || style == 5) && n.d >= 0.5 && mr.p ((T + 0.2f) * 0.45f))
            {
                if (style == 4 && ps.s < kStr - 1)
                {
                    const int s2 = ps.s + 1; for (int f2 = ps.f - 1; f2 <= ps.f + 1; ++f2) { const int m2 = tun[(size_t) s2] + f2; if (f2 >= 0 && m2 - n.m >= 3 && m2 - n.m <= 5 && ok (slot, m2)) { add (t + ms (3), len, s2, f2, v * 0.85f, slot); P.ev.back().lead = true; break; } }
                }
                else if (style == 5 && ps.s > 0)
                {
                    const int s2 = ps.s - 1; for (int f2 = ps.f - 1; f2 <= ps.f + 3; ++f2) { const int m2 = tun[(size_t) s2] + f2; if (f2 >= 0 && n.m - m2 >= 3 && n.m - m2 <= 4 && ok (slot, m2)) { add (t + ms (2), len, s2, f2, v * 0.85f, slot); P.ev.back().lead = true; break; } }
                }
            }
        }
    }
    void vibrato (double a, double b, float centre, int s, float depth)
    {
        if (b - a < 0.15) return;
        const double per = S.bpm / 60.0 / 5.6;   // 5.6 Hz in battiti
        for (double x = a; x < b; x += per / 12.0)
        {
            const double env = juce::jmin (1.0, (x - a) / (per * 2.0));
            const double ph = juce::MathConstants<double>::twoPi * (x - a) / per;
            // sul bend la vibrazione scende dalla nota (si molla un po' la corda), altrimenti oscilla attorno alla nota
            const float off = centre > 0 ? -(float) (depth * env * (0.5 - 0.5 * std::cos (ph))) : (float) (depth * 0.6 * env * std::sin (ph));
            P.bend.push_back ({ x, centre + off, s });
        }
        P.bend.push_back ({ b, centre, s });
    }

    // --------------------------------------------------------------------------------------------- pulizia finale
    void finish()
    {
        const double L = P.len();
        // inizio di ogni accordo (primo colpo dello slot): le note dell'accordo prima si chiudono li' (la mano cambia posizione)
        std::vector<double> first ((size_t) nSlots, 1e9);
        for (auto& e : P.ev) if (! e.lead) first[(size_t) e.slot] = juce::jmin (first[(size_t) e.slot], e.beat);
        for (auto& e : P.ev)
        {
            double end = e.beat + e.len;
            if (! e.lead)
                for (int j = e.slot + 1; j < nSlots; ++j)
                    if (first[(size_t) j] < 1e8) { end = juce::jmin (end, first[(size_t) j] - 0.01); break; }
            for (auto& c : P.chokes) if (c.beat > e.beat + 0.02 && c.beat < end) end = c.beat;
            end = juce::jmin (end, L - 0.005);
            e.len = juce::jmax (0.02, end - e.beat);
        }
        // una corda suona una nota alla volta: la nuova pizzicata ferma la precedente (il legato si sovrappone appena)
        std::stable_sort (P.ev.begin(), P.ev.end(), [] (const Ev& a, const Ev& b) { return a.beat < b.beat; });
        for (int s = 0; s < kStr; ++s)
        {
            Ev* prev = nullptr;
            for (auto& e : P.ev)
            {
                if (e.str != s) continue;
                if (prev != nullptr && e.art != 0 && e.beat - (prev->beat + prev->len) < 0.15) prev->len = e.beat - prev->beat + 0.03;
                else if (prev != nullptr && prev->beat + prev->len > e.beat - 0.004) prev->len = juce::jmax (0.015, e.beat - prev->beat - 0.004);
                prev = &e;
            }
        }
        P.ev.erase (std::remove_if (P.ev.begin(), P.ev.end(), [L] (const Ev& e) { return e.beat >= L || e.beat < 0; }), P.ev.end());
        std::stable_sort (P.bend.begin(), P.bend.end(), [] (const Bend& a, const Bend& b) { return a.beat < b.beat; });
        for (auto& b : P.bend) b.beat = juce::jlimit (0.0, L - 0.001, b.beat);
        if (! P.bend.empty() && P.bend.back().semis != 0) P.bend.push_back ({ L - 0.001, 0.0f, P.bend.back().str });
    }
};

inline Part generate (const Settings& S)
{
    Part P; P.bars = juce::jlimit (1, 8, S.bars);
    Gen G (S, P);
    G.slotLen = S.perBar >= 2 ? 2.0 : 4.0; G.nSlots = (int) std::round (P.len() / G.slotLen);
    G.inScale = scaleSet (S.key, S.scale); G.penta = pentaSet (S.key, S.scale);
    std::vector<int> prog = S.prog; if (prog.empty()) prog = progressions (S.genre).front();
    juce::Random vr ((juce::int64) S.seed * 31 + 7);
    const auto tun = tuning (S.tuning);
    for (int i = 0; i < G.nSlots; ++i)
    {
        int code = prog[(size_t) (i % (int) prog.size())];
        // interscambio automatico (solo se alzato): il V maggiore in minore, il iv minore in maggiore
        if (code < 10 && S.borrow > 0.001f && canBorrow (S.scale, code))
        {
            const int nxt = prog[(size_t) ((i + 1) % (int) prog.size())] % 10;
            const float pr = S.borrow * (nxt == 0 ? 0.9f : 0.45f);
            if (G.mr.p (pr)) code += 10;
        }
        Chord c = slotChord (S, code);
        const Voicing* pv = i > 0 ? &P.voic.back() : nullptr;
        Voicing v = findVoicing (c, tun, S.vstyle, S.pos, pv, vr);
        if (v.score < -1e8) v = findVoicing (c, tun, V_JAZZ, S.pos, pv, vr);
        if (v.score < -1e8) { Chord t = c; t.tones.resize (3); v = findVoicing (t, tun, V_OPEN, S.pos, pv, vr); }
        if (v.score < -1e8) v = findVoicing (c, tun, V_POWER, S.pos, pv, vr);
        P.chords.push_back (c); P.voic.push_back (v); P.chordAt.push_back (i * G.slotLen);
        // note ammesse: la scala; sopra un accordo preso in prestito entrano le sue note e si tolgono quelle che ci stridono contro
        auto a = G.inScale;
        if (c.borrowed)
            for (int t : c.tones) if (! a[(size_t) t]) { a[(size_t) t] = true; a[(size_t) ((t + 11) % 12)] = false; a[(size_t) ((t + 1) % 12)] = a[(size_t) ((t + 1) % 12)] && [&] { for (int u : c.tones) if (u == (t + 1) % 12) return true; return false; }(); }
        for (int t : c.tones) a[(size_t) t] = true;
        G.allow.push_back (a);
    }
    switch (S.mode) { case ACCORDI: G.accordi(); break; case RITMICA: G.ritmica(); break; case ARPEGGIO: G.arpeggio(); break; default: G.lead(); break; }
    G.finish();
    return P;
}

// nuova progressione dal genere, con piccole varianti sempre diatoniche
inline std::vector<int> newProgression (int genre, juce::uint32 seed)
{
    Rng r (seed ^ 0xabcdu); const auto& L = progressions (genre);
    auto p = L[(size_t) r.i ((int) L.size())];
    if (r.p (0.3f) && p.size() >= 4) { const int k = 1 + r.i ((int) p.size() - 1); static const int sub[7] = { 5, 3, 4, 1, 2, 0, 4 }; if (sub[p[(size_t) k]] != p[(size_t) k - 1]) p[(size_t) k] = sub[p[(size_t) k]]; }   // sostituto (stessa funzione)
    return p;
}
inline juce::String degreeRoman (int code, int scale, int key)
{
    static const char* R[7] = { "I", "II", "III", "IV", "V", "VI", "VII" };
    const auto c = code >= 10 ? borrowedChord (key, scale, code - 10, 0) : diatonic (key, scale, code, 0);
    juce::String s (R[code % 10]); const int i3 = c.third >= 0 ? (c.third - c.root + 12) % 12 : 4;
    if (i3 == 3) s = s.toLowerCase();
    if (c.name.endsWith ("dim")) s += juce::String::fromUTF8 ("\xc2\xb0");
    return s;
}
}
