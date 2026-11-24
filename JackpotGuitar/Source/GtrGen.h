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
// codice dello slot: grado (0..6, +10 se preso in prestito) + 100 * (colore + 1) se il colore e' scelto per quell'accordo
inline int codeDeg (int code) { return code % 100; }
inline int codeCol (int code) { return code >= 100 ? code / 100 - 1 : -1; }
inline int withCol (int code, int col) { return codeDeg (code) + (col >= 0 ? 100 * (col + 1) : 0); }
inline Chord slotChord (const Settings& S, int code)
{
    int col = codeCol (code);
    if (col < 0) col = S.color < 0 ? genreColor (S.genre) : juce::jlimit (0, kColors - 1, (int) S.color);
    const int d = codeDeg (code);
    if (d >= 10) return borrowedChord (S.key, S.scale, d - 10, col);
    return diatonic (S.key, S.scale, juce::jlimit (0, 6, d), col);
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
    MoodK mk; float dens = 0.5f, space = 0.3f, ring = 0.7f;   // manopole corrette dal mood
    Gen (const Settings& s, Part& p) : S (s), P (p), tun (tuning (s.tuning)), hr (s.seed ^ 0x51ed27u), mr (s.seed)
    {
        mk = moodK (S.mood == M_AUTO ? moodOfGenre (S.genre) : S.mood);
        dens = juce::jlimit (0.0f, 1.0f, S.density + mk.dens); space = juce::jlimit (0.0f, 1.0f, S.space + mk.space); ring = juce::jlimit (0.0f, 1.0f, S.ring + mk.ring);
    }
    // forma: ogni 4 battute una frase. 0 motivo, 1 risposta (variazione), 2 motivo ripetuto, 3 chiusura. Nel giro di 8 la seconda frase sale un po'
    int role (int b) const { if (P.bars == 1) return 0; if (P.bars == 2) return b == 0 ? 0 : 3; return b % 4; }
    bool lifted (int b) const { return P.bars >= 8 && b >= 4; }
    float barVel (int b) const { static const float v[4] = { 1.0f, 0.93f, 1.03f, 0.95f }; return v[b % 4] * (lifted (b) ? 1.0f + 0.05f * mk.lift : 1.0f); }
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
    int roleStr (int slot, char r) const
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
                    if (dens > 0.66f && L >= 2) strum (t0 + L / 2, i, 1, all, vAcc (false) * 0.85f, L / 2);
                    break;
                case 1:   // COLPI 1 E 3
                    for (double b = 0; b < L - 1e-6; b += 2)
                    {
                        strum (sw (t0 + b, 0.5), i, 1, all, vAcc (b == 0), 1.6);
                        if (dens > 0.45f && b + 1.5 < L) strum (sw (t0 + b + 1.5, 0.5), i, -1, top (3), vAcc (false), 0.45);
                    }
                    if (dens > 0.8f) P.chokes.push_back ({ t0 + L - 0.25 });
                    break;
                case 2:   // STAB R&B: colpi corti e sincopati, pizzicati insieme
                {
                    static const double pos[] = { 0, 0.75, 1.5, 2.5, 3.0, 3.75 }; static const float pr[] = { 1.0f, 0.5f, 0.8f, 0.6f, 0.35f, 0.5f };
                    for (int k = 0; k < 6; ++k)
                    {
                        if (pos[k] >= L - 1e-6) break;
                        if (k > 0 && ! mr.p (pr[k] * (0.35f + dens))) continue;
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
                    if (dens > 0.5f && L >= 4)
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
                    ++grpN; const double h = hum(); const double st = ms (55.0 + 70.0 * (1.0 - dens));
                    for (size_t q = 0; q < all.size(); ++q) add (juce::jmax (0.0, t0 + h + st * (double) q), L, all[q], P.voic[(size_t) i].fret[(size_t) all[q]], vAcc (q == 0) * (1.0f - 0.03f * (float) q), i);
                    if (dens > 0.6f && L >= 4) { ++grpN; for (size_t q = 0; q < all.size(); ++q) { const size_t r = all.size() - 1 - q; add (t0 + 2.5 + st * 0.7 * (double) q, L - 2.5, all[r], P.voic[(size_t) i].fret[(size_t) all[r]], vAcc (false) * 0.75f, i); } }
                    break;
                }
                case 6:   // PIZZICATI: accordo pizzicato tutto insieme (dita), ritmo trap 1 - 2.5(e) - 4
                {
                    static const double pos[] = { 0, 1.5, 3.0, 2.5, 3.75 }; static const float pr[] = { 1.0f, 0.9f, 0.7f, 0.4f, 0.3f };
                    for (int k = 0; k < 5; ++k)
                    {
                        if (pos[k] >= L - 1e-6) continue;
                        Rng kr (S.seed * 41u + (juce::uint32) k * 7u + (juce::uint32) (i % 2) * 3u);
                        if (k > 0 && ! kr.p (pr[k] * (0.4f + dens))) continue;
                        const double t = sw (t0 + pos[k], 0.25); ++grpN; const double h = hum(); const float gv = hv();
                        const auto use = k == 0 ? all : top (3);
                        for (size_t q = 0; q < use.size(); ++q) add (juce::jmax (0.0, t + h + ms (2.0 * (double) q)), k == 0 ? L : 1.2, use[q], P.voic[(size_t) i].fret[(size_t) use[q]], vAcc (k == 0) * (k == 0 ? 1.0f : 0.85f) + gv, i);
                    }
                    break;
                }
                default:  // SPINTE: l'accordo arriva un ottavo prima della battuta
                {
                    const double t = i == 0 ? 0.0 : t0 - 0.5;
                    strum (t, i, 1, all, vAcc (true), end - t);
                    if (dens > 0.35f && L >= 2) strum (sw (t0 + 1.5, 0.5), i, -1, top (3), vAcc (false), 0.5);
                    if (dens > 0.7f && L >= 4) strum (sw (t0 + 2.5, 0.5), i, 1, all, vAcc (false), 0.8);
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
            { 16, { "b---D---B---D---", "b---D-U-B---D-U-", "b---D-UuB-u-D-U-" } },          // FOLK
            { 16, { "D-------D-----u-", "D-----u--g-U----", "D--u--g-D-u-gU--" } },          // TRAP STRUM: pochi colpi, spazio, un levare
            { 16, { "D---d-u---u-d-u-", "D-uUd-u-D-u-d-uU", "D-uUd-uUD-uUd-uU" } } };        // R&B 16: sedicesimi leggeri
        return R[juce::jlimit (0, 10, p)];
    }
    void ritmica()
    {
        const auto& R = rpat (S.pattern); const double st = 4.0 / R.steps;
        const bool tripl = R.steps == 12;
        for (int b = 0; b < P.bars; ++b)
        {
            int var = dens < 0.34f ? 0 : dens < 0.67f ? 1 : 2;
            const int rl = role (b); Rng br (S.seed * 7919u + (juce::uint32) b * 104729u);
            const bool same = br.p (mk.repeat);
            if (rl == 1 && ! same) var = juce::jlimit (0, 2, var + (br.p (0.5f) ? 1 : -1));                 // risposta: un'altra versione del pattern
            if (lifted (b) && rl != 3 && br.p (mk.lift * 0.6f)) var = juce::jmin (2, var + 1);              // seconda frase del giro di 8: un po' piu' piena
            double breath = 1e9;                                                                             // fine frase: o si riempie o si respira
            if (rl == 3 && P.bars > 1 && ! br.p (mk.repeat - 0.4f)) { if (S.space > 0.35f && br.p (S.space)) breath = S.space > 0.7f ? 3.0 : 3.5; else var = juce::jmin (2, var + 1); }
            const char* pat = R.v[var];
            if (breath < 1e8) P.chokes.push_back ({ b * 4.0 + breath + ms (6) });
            for (int k = 0; k < R.steps; ++k)
            {
                const char c = pat[k]; if (c == '-') continue;
                if (k * st >= breath - 1e-6) continue;
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
                    case 'b': pluck (t, i, roleStr (i, 'B'), vAcc (true), rest); break;
                    case 'B': pluck (t, i, roleStr (i, 'A'), vAcc (true) * 0.92f, rest); break;
                    case '.': P.chokes.push_back ({ t + ms (8) }); break;
                    default: break;
                }
            }
        }
    }

    // --------------------------------------------------------------------------------------------- ARPEGGIO
    // voce dell'accordo -> corda: 1 la piu' acuta, 2 la seconda dall'alto, 3, 4; B il basso, A il basso alternato
    int voiceStr (int slot, char c) const
    {
        const auto v = sounding (slot); const int n = (int) v.size(); if (n == 0) return -1;
        if (c == 'B') return v[0];
        if (c == 'A') return roleStr (slot, 'A');
        if (c >= '1' && c <= '4') { const int k = c - '0'; return v[(size_t) juce::jlimit (n > 1 ? 1 : 0, n - 1, n - k)]; }
        return -1;
    }
    // importanza di un passo: battere > ottavo > sedicesimo (per togliere o aggiungere note con la DENSITA')
    static int importance (int k, int steps) { if (steps == 12) return k % 6 == 0 ? 4 : k % 3 == 0 ? 3 : 1; return k % 8 == 0 ? 4 : k % 4 == 0 ? 3 : k % 2 == 0 ? 2 : 1; }
    // il ritmo con la densita': sotto il 50% toglie le note meno importanti, sopra aggiunge riempimenti (note fantasma) nei posti piu' importanti.
    // Ogni tacca della manopola cambia il numero di note. 0 = niente, 1 = nota, 2 = accento, 3 = aggiunta
    std::vector<int> rhythmMask (const char* pat, int steps, float d, juce::uint32 salt) const
    {
        std::vector<int> m ((size_t) steps, 0); int n0 = 0;
        for (int k = 0; k < steps; ++k) { m[(size_t) k] = pat[k] == 'X' ? 2 : pat[k] == 'x' ? 1 : 0; n0 += m[(size_t) k] > 0; }
        const float f = d < 0.5f ? 0.3f + 1.4f * d : 1.0f + 2.6f * (d - 0.5f);
        const int target = juce::jlimit (1, steps, (int) std::round ((float) n0 * f));
        Rng r (S.seed * 131u + salt);
        std::vector<std::pair<float, int>> rank;
        if (target < n0)
        {
            for (int k = 0; k < steps; ++k) if (m[(size_t) k] > 0 && k != 0) rank.push_back ({ (float) importance (k, steps) + (m[(size_t) k] == 2 ? 2.0f : 0.0f) + r.f() * 0.9f, k });
            std::sort (rank.begin(), rank.end());
            for (int q = 0; q < n0 - target && q < (int) rank.size(); ++q) m[(size_t) rank[(size_t) q].second] = 0;
        }
        else if (target > n0)
        {
            for (int k = 0; k < steps; ++k) if (m[(size_t) k] == 0) rank.push_back ({ -((float) importance (k, steps) + r.f() * 0.9f), k });
            std::sort (rank.begin(), rank.end());
            for (int q = 0; q < target - n0 && q < (int) rank.size(); ++q) m[(size_t) rank[(size_t) q].second] = 3;
        }
        return m;
    }
    void arpeggio()
    {
        const bool useGrid = S.arpFig == kFigs - 1;
        const auto& R = ritOf (S.arpRit);
        const int steps = useGrid ? 16 : (int) std::strlen (R.steps);
        const double st = 4.0 / steps; const bool tripl = steps == 12;
        auto fig = juce::StringArray::fromTokens (S.arpFig == kFigLibera ? S.figSeq.toUpperCase() : juce::String (figOf (S.arpFig).seq), " ", "");
        for (auto& t : fig) t = t.retainCharacters ("1234BA");
        fig.removeEmptyStrings();
        if (fig.isEmpty()) fig.add ("B");
        // la battuta-tipo: decisa una volta, poi ripetuta (il motivo si riconosce); la seconda frase del giro di 8 un po' piu' piena
        std::vector<int> base, lift;
        if (useGrid)
        {
            base.assign (16, 0); for (int k = 0; k < 16; ++k) for (int rr = 0; rr < 5; ++rr) if (S.grid[(size_t) rr] & (1 << k)) base[(size_t) k] = k % 4 == 0 ? 2 : 1;
            lift = base;
        }
        else { base = rhythmMask (R.steps, steps, S.density, 1); lift = rhythmMask (R.steps, steps, juce::jmin (1.0f, S.density + 0.12f * mk.lift), 2); }
        auto gridTok = [&] (int k) { juce::String t; static const char rc[5] = { '1', '2', '3', '4', 'B' }; for (int rr = 0; rr < 5; ++rr) if (S.grid[(size_t) rr] & (1 << k)) t += juce::String::charToString (rc[rr]); return t; };
        int lastSlot = -1, fi = 0;
        for (int b = 0; b < P.bars; ++b)
        {
            const int rl = role (b); Rng br (S.seed * 7919u + (juce::uint32) b * 104729u);
            const auto& m = lifted (b) && rl != 3 ? lift : base;
            const bool same = br.p (mk.repeat);
            const bool reverseHalf = ! useGrid && rl == 1 && ! same;              // risposta: la seconda meta' della battuta va al contrario
            const bool fill = ! useGrid && rl == 3 && P.bars > 1 && S.space <= 0.45f && dens > 0.3f && ! br.p (mk.repeat - 0.3f);   // fine frase: corsa dal basso all'acuto
            double breath = 1e9;
            if (rl == 3 && S.space > 0.45f && br.p ((S.space - 0.2f) * (1.0f - mk.repeat * 0.6f))) breath = S.space > 0.75f ? 2.0 : 3.0;
            int lastK = -1, fillIdx = 0; for (int k = 0; k < steps; ++k) if (m[(size_t) k] && k * st < breath - 1e-6) lastK = k;
            for (int k = 0; k < steps; ++k)
            {
                const double pos = k * st;
                const bool inFill = fill && pos >= 3.0 - 1e-6;
                if (! inFill && m[(size_t) k] == 0) continue;
                if (pos >= breath - 1e-6) continue;
                const double tb = b * 4.0 + pos, t = sw (tb, tripl ? 1.0 / 3.0 : 0.25);
                const int i = slotAt (tb + 1e-6);
                if (i != lastSlot) { fi = 0; lastSlot = i; }                       // la figura ricomincia al cambio d'accordo
                juce::String tk;
                if (useGrid) tk = gridTok (k);
                else if (inFill) { static const char* run[4] = { "B", "3", "2", "1" }; tk = run[juce::jmin (3, fillIdx++)]; }
                else
                {
                    const int n = fig.size(); int idx = (fi + S.arpRot) % n; if (reverseHalf && pos >= 2.0) idx = n - 1 - idx;   // PARTENZA: da quale nota della figura si comincia
                    tk = fig[idx]; ++fi;
                }
                // distanza dalla nota dopo: le corde suonano fino a li' (piu' a lungo con RISUONA)
                int nk = k + 1; while (nk < steps && m[(size_t) nk] == 0 && ! (fill && nk * st >= 3.0)) ++nk;
                const double gap = (nk - k) * st;
                double len = gap * (0.9 + ring * 6.0);
                if (k == lastK && breath < 1e8) len = juce::jmax (len, 4.0 - pos);
                // dinamica: battere piu' forte, pollice sul basso, nota alta sui tempi, piccolo crescendo nella battuta, note aggiunte piu' leggere
                const int imp = importance (k, steps);
                float v = 0.55f + (imp >= 4 ? 0.15f : imp == 3 ? 0.10f : imp == 2 ? 0.05f : 0.0f) * (0.4f + S.dyn) + 0.05f * (float) (pos / 4.0) * S.dyn;
                if (m[(size_t) k] == 2) v += 0.05f * S.dyn;
                if (m[(size_t) k] == 3 || inFill) v *= 0.78f;
                ++grpN; const double h = hum(); const float gv = hv();
                Rng er (S.seed * 2654435u + (juce::uint32) k * 97u + (juce::uint32) (lifted (b) ? 1 : 0) * 7919u + (juce::uint32) (reverseHalf ? 3 : 0));
                int q = 0;
                for (auto c : tk)
                {
                    const int s = voiceStr (i, (char) c); if (s < 0) continue;
                    const int f = P.voic[(size_t) i].fret[(size_t) s];
                    const bool bass = c == 'B' || c == 'A';
                    float vv = v + gv + (bass ? 0.07f : c == '1' && imp >= 3 ? 0.04f : 0.0f);
                    const double tt = juce::jmax (0.0, t + h + ms (4.0 * q++));   // pizzicate insieme: appena sfalsate
                    if (! bass && f >= 1 && S.emb > 0.01f && er.p (S.emb * 0.22f))
                    {   // abbellimento: hammer-on da una nota della scala sotto, sulla stessa corda
                        int g = -1; for (int dd : { 2, 1 }) if (f - dd >= 0 && ok (i, tun[(size_t) s] + f - dd)) { g = f - dd; break; }
                        if (g >= 0) { const double gl = juce::jmin (0.125, gap * 0.5); add (tt, gl + 0.02, s, g, vv * 0.95f, i); add (tt + gl, len, s, f, vv * 0.78f, i, false, false, 1); continue; }
                    }
                    add (tt, bass ? len * 1.5 : len, s, f, vv, i);
                }
            }
        }
    }

    // --------------------------------------------------------------------------------------------- LEAD
    // --- melodia cantabile: frasi di 4 battute (domanda e risposta), motivo che torna, note d'arrivo sugli accordi, passi e salti piccoli, respiri
    using Cell = std::vector<std::pair<double, double>>;   // (attacco, durata) in battiti dentro la battuta
    struct CellSet { std::vector<Cell> M, R, E; };
    static CellSet cellSet (int style)
    {
        const double t = 1.0 / 3.0;
        switch (style)
        {
            case 1: return { { { { 0, 1.5 }, { 1.5, 0.5 }, { 2, 2 } }, { { 0, 0.75 }, { 0.75, 0.75 }, { 1.5, 2.5 } }, { { 0, t }, { t, t }, { 2 * t, t }, { 1, 2 }, { 3, 1 } }, { { 0.5, 0.5 }, { 1, 1 }, { 2, 2 } } },
                             { { { 0, 3 } }, { { 0.5, 0.5 }, { 1, 2.5 } }, { { 0, 2 }, { 2, 0.5 }, { 2.5, 1 } } },
                             { { { 0, 3 } }, { { 0, 1 }, { 1, 2.5 } }, { { 0, 0.5 }, { 0.5, 0.5 }, { 1, 2.5 } } } };                                      // TRAP SCURO
            case 2: return { { { { 0, 2 * t }, { 2 * t, 2 * t }, { 4 * t, 2 * t }, { 2, 1.5 } }, { { 0, t }, { t, t }, { 2 * t, t }, { 1, 1 }, { 2, 2 * t }, { 8 * t, 2 * t }, { 10 * t, 2 * t } }, { { 0, 1 }, { 1, 2 * t }, { 5 * t, t }, { 2, 2 } } },
                             { { { 0, 1 }, { 1, 2 * t }, { 5 * t, t }, { 2, 1.5 } }, { { 0, 2 }, { 8 * t, 2 * t }, { 10 * t, 2 * t } } },
                             { { { 0, 2 * t }, { 2 * t, 2 * t }, { 4 * t, 2.5 } }, { { 0, 3 } } } };                                                         // DRILL
            case 3: return { { { { 0, 1.5 }, { 1.5, 0.25 }, { 1.75, 0.25 }, { 2, 0.25 }, { 2.25, 0.25 }, { 2.5, 1.5 } }, { { 0, 0.5 }, { 0.5, 0.5 }, { 1, 0.5 }, { 1.5, 0.25 }, { 1.75, 0.25 }, { 2, 2 } } },
                             { { { 0, 2 }, { 2.5, 0.25 }, { 2.75, 0.25 }, { 3, 1 } }, { { 0, 1 }, { 1, 0.5 }, { 1.5, 2 } } },
                             { { { 0, 0.25 }, { 0.25, 0.25 }, { 0.5, 0.25 }, { 0.75, 0.25 }, { 1, 2.5 } }, { { 0, 3 } } } };                                   // R&B RUN
            case 4: return { { { { 0, 0.5 }, { 0.5, 0.5 }, { 1, 1.5 }, { 2.5, 0.5 }, { 3, 1 } }, { { 0, t }, { t, t }, { 2 * t, t }, { 1, 1 }, { 2, 1.5 } } },
                             { { { 0, 1.5 }, { 1.5, 0.5 }, { 2, 1.5 } }, { { 0, 3 } } },
                             { { { 0, 0.5 }, { 0.5, 0.5 }, { 1, 2.5 } }, { { 0, 3 } } } };                                                                   // BLUES
            case 5: return { { { { 0, 0.75 }, { 0.75, 0.75 }, { 1.5, 0.5 }, { 2, 0.75 }, { 2.75, 0.75 }, { 3.5, 0.5 } }, { { 0, 0.5 }, { 0.5, 0.5 }, { 1, 0.5 }, { 1.5, 0.5 }, { 2, 1.5 } } },
                             { { { 0, 0.75 }, { 0.75, 0.75 }, { 1.5, 2 } }, { { 0.5, 0.25 }, { 0.75, 0.75 }, { 1.5, 0.5 }, { 2, 1.5 } } },
                             { { { 0, 0.75 }, { 0.75, 0.75 }, { 1.5, 2 } }, { { 0, 3 } } } };                                                                 // LATIN
            default: return { { { { 0, 1 }, { 1, 0.5 }, { 1.5, 0.5 }, { 2, 1.5 } }, { { 0, 0.5 }, { 0.5, 0.5 }, { 1, 1 }, { 2, 0.5 }, { 2.5, 0.5 }, { 3, 1 } },
                                { { 0.5, 0.5 }, { 1, 0.5 }, { 1.5, 1 }, { 2.5, 0.5 }, { 3, 1 } }, { { 0, 0.75 }, { 0.75, 0.75 }, { 1.5, 0.5 }, { 2, 2 } } },
                              { { { 0, 2 }, { 2.5, 0.5 }, { 3, 1 } }, { { 0, 1.5 }, { 1.5, 0.5 }, { 2, 1.5 } }, { { 0, 1 }, { 1, 1 }, { 2, 1.5 } }, { { 0.5, 0.5 }, { 1, 2.5 } } },
                              { { { 0, 1 }, { 1, 1 }, { 2, 1.5 } }, { { 0, 0.5 }, { 0.5, 0.5 }, { 1, 2.5 } }, { { 0, 3 } }, { { 0, 1.5 }, { 1.5, 0.5 }, { 2, 1.5 } } } };   // MELODIA
        }
    }
    // sceglie una cella: con il mood "note lunghe" alto vince quella con meno note
    Cell pickCell (const std::vector<Cell>& L, Rng& r) const
    {
        Cell a = L[(size_t) r.i ((int) L.size())], b = L[(size_t) r.i ((int) L.size())];
        if (mk.longN > 0.55f) return a.size() <= b.size() ? a : b;
        if (mk.longN < 0.35f) return a.size() >= b.size() ? a : b;
        return a;
    }
    // densita' e pause applicate alla cella (una volta: la cella ripetuta resta uguale)
    Cell shape (Cell c, Rng& r, bool isEnd) const
    {
        Cell d;
        for (size_t k = 0; k < c.size(); ++k)
        {
            auto n = c[k];
            if (k > 0 && dens < 0.4f && n.first != std::floor (n.first) && r.p ((0.4f - dens) * 1.6f)) { d.back().second += n.second; continue; }
            if (dens > 0.7f && n.second >= 1.0 && n.first + n.second < 3.9 && ! (isEnd && k + 1 == c.size()) && r.p ((dens - 0.7f) * 2.0f)) { d.push_back ({ n.first, n.second * 0.5 }); d.push_back ({ n.first + n.second * 0.5, n.second * 0.5 }); continue; }
            d.push_back (n);
        }
        if (isEnd) { d.back().second = juce::jmax (d.back().second, (space > 0.5f ? 3.0 : 3.5) - d.back().first); }   // l'ultima nota della frase dura
        else if (d.size() > 2 && space > 0.35f && r.p (space * 0.5f)) d.pop_back();                                       // un respiro in piu'
        return d;
    }
    void lead()
    {
        const int style = S.pattern;
        const int lp = juce::jlimit (0, kFrets - 5, S.pos + mk.reg);
        // estensione cantabile: poco piu' di un'ottava attorno alla corda di Sol nella posizione
        const int centre = tun[3] + lp + (style == 2 ? 0 : 3);
        const int lo = centre - 7, hi = centre + 9;
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
        auto nearestIn = [] (const std::vector<int>& v, int m) { int best = v.empty() ? m : v.front(); for (int x : v) if (std::abs (x - m) < std::abs (best - m)) best = x; return best; };
        auto idxIn = [] (const std::vector<int>& v, int m) { int bi = 0; for (int q = 0; q < (int) v.size(); ++q) if (std::abs (v[(size_t) q] - m) < std::abs (v[(size_t) bi] - m)) bi = q; return bi; };

        // 1. ritmo: motivo M, risposta R, chiusura E (scelti una volta), ripetuti secondo la forma
        Rng cr (S.seed * 3u + 17u); const auto CS = cellSet (style);
        const Cell M = shape (pickCell (CS.M, cr), cr, false), R1 = shape (pickCell (CS.R, cr), cr, false), R2 = shape (pickCell (CS.R, cr), cr, false);
        const Cell E1 = shape (pickCell (CS.E, cr), cr, true), E2 = shape (pickCell (CS.E, cr), cr, true);
        const bool hyp = mk.repeat > 0.85f;
        struct LN { double t, d; int m; bool strong; int bar, k; };
        std::vector<LN> notes;
        std::vector<int> barStart;
        for (int b = 0; b < P.bars; ++b)
        {
            const int rl = role (b);
            const Cell* c = &M;
            if (rl == 1) c = hyp ? &M : (lifted (b) ? &R2 : &R1);
            if (rl == 3) c = (b == P.bars - 1) ? &E2 : &E1;
            Cell cc = *c;
            if (hyp && rl == 3) { cc = M; while (cc.size() > 1 && cc.back().first > 2.01) cc.pop_back(); cc.back().second = 4.0 - cc.back().first - 0.5; }
            barStart.push_back ((int) notes.size());
            for (size_t k = 0; k < cc.size(); ++k)
            {
                const double on = cc[k].first;
                const bool strong = std::abs (on - std::round (on)) < 1e-6 && ((int) std::round (on) % 2 == 0);
                notes.push_back ({ b * 4.0 + on, cc[k].second, 0, strong, b, (int) k });
            }
        }
        if (notes.empty()) return;
        // 2. note d'arrivo: la prima di ogni battuta e' una nota dell'accordo che segue la curva della frase
        static const float C[4][4] = { { 0.45f, 0.65f, 0.8f, 0.3f }, { 0.75f, 0.6f, 0.5f, 0.25f }, { 0.35f, 0.5f, 0.7f, 0.55f }, { 0.5f, 0.55f, 0.5f, 0.45f } };
        auto curve = [&] (int b) { float v = C[juce::jlimit (0, 3, mk.contour)][role (b)]; if (lifted (b) && role (b) != 3) v += 0.15f * mk.lift; return juce::jlimit (0.05f, 1.0f, v); };
        int prevT = -1;
        std::vector<int> target ((size_t) P.bars, 0);
        for (int b = 0; b < P.bars; ++b)
        {
            const auto& first = notes[(size_t) barStart[(size_t) b]];
            const int slot = slotAt (first.t + 1e-6); const auto ct = poolFor (slot, true);
            const int want = lo + juce::roundToInt (curve (b) * (float) (hi - lo));
            int best = nearestIn (ct, want);
            if (prevT >= 0)
            {   // dalla nota d'arrivo precedente: meglio entro una quarta, mai oltre la sesta
                float bc = 1e9f;
                for (int x : ct) { const int iv = std::abs (x - prevT); float c = (float) std::abs (x - want) * 0.6f + (iv > 9 ? 50.0f : iv > 5 ? (float) (iv - 5) * 1.5f : 0.0f) + (iv == 6 ? 6.0f : 0.0f) + (iv == 0 ? 1.2f : 0.0f); if (c < bc) { bc = c; best = x; } }
            }
            target[(size_t) b] = best; prevT = best;
        }
        // 3. le note di ogni battuta: il motivo ripetuto conserva la sua forma (gli intervalli), il resto va per gradi verso la nota d'arrivo successiva
        std::vector<int> motifShape;   // intervalli del motivo, in gradi della scala
        int prev = target[0];
        for (int b = 0; b < P.bars; ++b)
        {
            const int s0 = barStart[(size_t) b], s1 = b + 1 < P.bars ? barStart[(size_t) b + 1] : (int) notes.size();
            const int rl = role (b), nextT = b + 1 < P.bars ? target[(size_t) b + 1] : target[0];
            const bool isMotif = rl == 0 || rl == 2 || (hyp && rl == 1);
            const bool useShape = isMotif && ! motifShape.empty() && (b > 0);
            Rng nr (S.seed * 613u + (juce::uint32) b * 31u);
            for (int q = s0; q < s1; ++q)
            {
                auto& n = notes[(size_t) q];
                const int slot = slotAt (n.t + 1e-6); const auto pool = poolFor (slot, false), ct = poolFor (slot, true);
                if (pool.empty()) { n.m = prev; continue; }
                int m;
                const bool last = q == s1 - 1;
                const bool cadence = rl == 3 && last;
                if (q == s0 && ! cadence) m = target[(size_t) b];
                else if (cadence)
                {   // chiusura: prima frase "domanda" (quinta o terza), alla fine "risposta" (fondamentale)
                    const bool answer = b == P.bars - 1 || (b % 8) == 7 || P.bars <= 4;
                    const auto& ch = P.chords[(size_t) slot];
                    std::vector<int> cand; for (int x : ct) { const int pc = x % 12; if (answer ? pc == ch.root : (pc == ch.fifth || pc == ch.third)) cand.push_back (x); }
                    m = nearestIn (cand.empty() ? ct : cand, prev);
                    if (std::abs (m - prev) == 6)
                    {   // la fondamentale sarebbe a un tritono: si chiude sulla terza o sulla quinta, che arrivano per grado o salto piccolo
                        int bm = m, bc = 99; for (int x : ct) { const int iv = std::abs (x - prev); if (iv != 6 && iv <= 5 && iv < bc) { bc = iv; bm = x; } }
                        m = bm;
                    }
                }
                else if (useShape && q - s0 - 1 < (int) motifShape.size())
                {
                    m = pool[(size_t) juce::jlimit (0, (int) pool.size() - 1, idxIn (pool, prev) + motifShape[(size_t) (q - s0 - 1)])];
                    // nella seconda volta una nota cambia (variazione), se il mood non e' ipnotico
                    if (! hyp && rl == 2 && q == s0 + 1 + nr.i (juce::jmax (1, s1 - s0 - 1)) && nr.p (0.6f)) m = pool[(size_t) juce::jlimit (0, (int) pool.size() - 1, idxIn (pool, m) + (nr.p (0.5f) ? 1 : -1))];
                }
                else
                {   // cammino verso la prossima nota d'arrivo: gradi congiunti, ogni tanto una nota di volta, salti solo se serve
                    const int remaining = s1 - q;   // note che mancano (questa compresa) prima della battuta dopo
                    const int ci = idxIn (pool, prev), ti = idxIn (pool, nextT); const int d = ti - ci;
                    int stp;
                    if (std::abs (d) > remaining + 1) stp = (d > 0 ? 1 : -1) * juce::jmin (3, (std::abs (d) + remaining) / (remaining + 1));
                    else if (std::abs (d) <= 1 && remaining >= 2) stp = nr.p (0.8f) ? (nr.p (0.5f) ? 1 : -1) : 0;   // nota di volta
                    else stp = d > 0 ? 1 : d < 0 ? -1 : 0;
                    if (stp == 0 && nr.p (0.75f)) stp = ci + 1 < (int) pool.size() && (ci == 0 || nr.p (0.5f)) ? 1 : -1;
                    m = pool[(size_t) juce::jlimit (0, (int) pool.size() - 1, ci + stp)];
                }
                // tempo forte e nota lunga: nota dell'accordo (la piu' vicina)
                if (! cadence && q != s0 && n.strong && n.d >= 1.0 && ! ct.empty()) { bool isCt = false; for (int x : ct) isCt |= x == m; if (! isCt) m = nearestIn (ct, m); }
                // niente tritono e niente salti oltre la sesta
                if (q > 0 && ! cadence)
                {
                    int iv = std::abs (m - prev);
                    if (iv > 9) { const int o = m > prev ? m - 12 : m + 12; if (o >= lo && o <= hi) m = o; iv = std::abs (m - prev); }
                    if (iv == 6 || iv > 9) m = pool[(size_t) juce::jlimit (0, (int) pool.size() - 1, idxIn (pool, m) + (m > prev ? -1 : 1))];
                }
                else if (q > 0 && cadence && std::abs (m - prev) > 9) { const int o = m > prev ? m - 12 : m + 12; if (o >= lo - 2 && o <= hi + 2) m = o; }
                // al massimo due note uguali di fila (fuori dal motivo ripetuto): la terza si muove di un grado
                if (! cadence && q >= 2 && m == prev && notes[(size_t) q - 2].m == prev && ! (useShape && ! hyp)) m = pool[(size_t) juce::jlimit (0, (int) pool.size() - 1, idxIn (pool, m) + (m > centre ? -1 : 1))];
                if (b == 0 && q > s0) motifShape.push_back (idxIn (pool, m) - idxIn (pool, prev));
                n.m = m; prev = m;
            }
        }
        // durate: legato cantabile fino alla nota dopo, salvo le pause volute
        for (size_t k = 0; k < notes.size(); ++k)
        {
            const double nt = k + 1 < notes.size() ? notes[k + 1].t : P.len();
            if (nt - (notes[k].t + notes[k].d) < 0.26) notes[k].d = nt - notes[k].t;
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
            if (! legato && n.d >= 0.7 && T > 0.01f && mr.p (T * (style == 4 || style == 1 ? 0.6f : style == 0 ? 0.15f : 0.3f)))
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
        for (auto& e : P.ev) e.vel = juce::jlimit (0.06f, 1.0f, e.vel * barVel ((int) (e.beat / 4.0)));   // dinamica della frase
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
        // la stessa altezza su due corde (unisono): nel MIDI e' la stessa nota, quindi la prima si chiude prima che parta la seconda
        { std::array<Ev*, 128> lastOf {}; for (auto& e : P.ev) { auto*& pv = lastOf[(size_t) juce::jlimit (0, 127, e.note)]; if (pv != nullptr && pv->beat + pv->len > e.beat - 0.004) pv->len = juce::jmax (0.01, e.beat - pv->beat - 0.004); pv = &e; } }
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
    G.slotLen = juce::jmin (P.len(), S.perBar >= 2 ? 2.0 : S.perBar == 0 ? 8.0 : 4.0); G.nSlots = juce::jmax (1, (int) std::round (P.len() / G.slotLen));
    G.inScale = scaleSet (S.key, S.scale); G.penta = pentaSet (S.key, S.scale);
    std::vector<int> prog = S.prog; if (prog.empty()) prog = progressions (S.genre).front();
    juce::Random vr ((juce::int64) S.seed * 31 + 7);
    const auto tun = tuning (S.tuning);
    // colore dal mood quando COLORE e' AUTO
    Settings S2 = S; if (S2.color < 0 && G.mk.color >= 0) S2.color = (float) G.mk.color;
    // linea alta: una nota per accordo che canta (passi piccoli, note comuni tenute, curva della frase); i voicing la mettono in cima
    int topPrev = -1; Rng tr (S.seed * 5u + 99u);
    const int tLo = tun[4] + juce::jmax (0, S.pos + G.mk.reg), tHi = tun[5] + S.pos + G.mk.reg + 4;
    const bool useTop = S.mode != LEAD && S.vstyle != V_POWER;
    for (int i = 0; i < G.nSlots; ++i)
    {
        int code = prog[(size_t) (i % (int) prog.size())];
        // interscambio automatico (solo se alzato): il V maggiore in minore, il iv minore in maggiore
        if (codeDeg (code) < 10 && S.borrow > 0.001f && canBorrow (S.scale, codeDeg (code)))
        {
            const int nxt = codeDeg (prog[(size_t) ((i + 1) % (int) prog.size())]) % 10;
            const float pr = S.borrow * (nxt == 0 ? 0.9f : 0.45f);
            if (G.mr.p (pr)) code += 10;
        }
        Chord c = slotChord (S2, code);
        const Voicing* pv = i > 0 ? &P.voic.back() : nullptr;
        int top = -1;
        if (useTop)
        {
            const double x = (double) i / juce::jmax (1, G.nSlots - 1);
            static const float K[4][3] = { { 0.4f, 0.75f, 0.45f }, { 0.75f, 0.55f, 0.3f }, { 0.3f, 0.5f, 0.75f }, { 0.5f, 0.5f, 0.5f } };
            const auto& k = K[juce::jlimit (0, 3, G.mk.contour)];
            const double cv = x < 0.5 ? k[0] + (k[1] - k[0]) * x * 2 : k[1] + (k[2] - k[1]) * (x - 0.5) * 2;
            const int want = tLo + (int) std::round (cv * (tHi - tLo));
            float bc = 1e9f;
            for (int m = tLo; m <= tHi; ++m)
            {
                bool ct = false; for (int t : c.tones) ct |= t == m % 12; if (! ct) continue;
                float cost = (float) std::abs (m - want) * 0.35f;
                if (topPrev >= 0) { const int iv = std::abs (m - topPrev); cost += iv == 0 ? (tr.p (G.mk.pedal) ? -2.0f : 0.8f) : iv <= 2 ? 0.0f : iv <= 4 ? 0.6f : (float) iv * 0.5f; }
                cost += tr.f() * 0.6f;
                if (cost < bc) { bc = cost; top = m; }
            }
            topPrev = top;
        }
        Voicing v; bool reused = false;
        if (i > 0 && prog[(size_t) (i % (int) prog.size())] == prog[(size_t) ((i - 1) % (int) prog.size())] && ! c.borrowed && ! P.chords.back().borrowed) { v = P.voic.back(); reused = true; }   // stesso accordo: stessa forma
        // arpeggio e ritmica: lo stesso accordo torna sempre con la stessa forma, cosi' il pattern resta riconoscibile giro dopo giro
        if (! reused && (S.mode == ARPEGGIO || S.mode == RITMICA))
            for (size_t j = 0; j < P.chords.size(); ++j) if (P.chords[j].name == c.name && P.chords[j].borrowed == c.borrowed) { v = P.voic[j]; reused = true; break; }
        if (reused) {}
        else v = findVoicing (c, tun, S.vstyle, S.pos, pv, vr, 0.0f, top);
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

// --------------------------------------------------------------------------------------------- COMPOSIZIONE
// tre chitarre sullo stesso giro: BASE (accordi o ritmica, a sinistra), ARPEGGIO (seconda chitarra in alto, a destra), LEAD (al centro).
// Si ascoltano fra loro: l'arpeggio sta sopra la base, si dirada quando il lead canta e lascia libere le note vicine alla melodia;
// la base si alleggerisce quando c'e' l'arpeggio.
inline Part generateComposition (const Settings& S)
{
    const bool hasB = S.lyBase >= 0, hasA = S.lyArp >= 0, hasL = S.lyLead >= 0;
    if (! hasB && ! hasA && ! hasL) { Settings a = S; a.mode = ARPEGGIO; return generate (a); }
    Part out; out.bars = juce::jlimit (1, 8, S.bars);
    Part pb, pa, pl;
    // l'armonia e' una sola per tutte e tre le chitarre: i prestiti si decidono qui, una volta
    Settings H = S;
    {
        const double slotLen = juce::jmin ((double) out.len(), S.perBar >= 2 ? 2.0 : S.perBar == 0 ? 8.0 : 4.0);
        const int n = juce::jmax (1, (int) std::round (out.len() / slotLen));
        std::vector<int> prog = S.prog; if (prog.empty()) prog = progressions (S.genre).front();
        Rng br (S.seed * 97u + 5u); std::vector<int> res;
        for (int i = 0; i < n; ++i)
        {
            int code = prog[(size_t) (i % (int) prog.size())];
            if (codeDeg (code) < 10 && S.borrow > 0.001f && canBorrow (S.scale, codeDeg (code)))
            {
                const int nxt = codeDeg (prog[(size_t) ((i + 1) % (int) prog.size())]) % 10;
                if (br.p (S.borrow * (nxt == 0 ? 0.9f : 0.45f))) code += 10;
            }
            res.push_back (code);
        }
        H.prog = res; H.borrow = 0.0f;
    }
    if (hasB)
    {
        Settings b = H; b.mode = S.lyBase >= 100 ? RITMICA : ACCORDI; b.pattern = S.lyBase >= 100 ? S.lyBase - 100 : S.lyBase;
        b.pattern = juce::jlimit (0, patternNames (b.mode).size() - 1, b.pattern);
        b.seed = S.seed * 7u + S.seedB * 13u; if (hasA) b.density = juce::jmax (0.0f, S.density - 0.2f);
        pb = generate (b);
    }
    if (hasA)
    {
        Settings a = H; a.mode = ARPEGGIO; a.pattern = juce::jlimit (0, patternNames (ARPEGGIO).size() - 1, S.lyArp);
        a.seed = S.seed * 11u + S.seedA * 17u;
        if (hasB) { a.vstyle = V_TRIAD; a.pos = juce::jmin (12, S.pos + 5); }   // doppia traccia: la seconda chitarra suona le triadi in alto
        pa = generate (a);
    }
    if (hasL)
    {
        Settings l = H; l.mode = LEAD; l.pattern = juce::jlimit (0, patternNames (LEAD).size() - 1, S.lyLead);
        l.seed = S.seed * 19u + S.seedL * 23u; l.pos = juce::jmin (12, S.pos + (hasA ? 6 : 4));   // la melodia sta sopra gli accordi
        pl = generate (l);
    }
    // l'arpeggio lascia spazio alla melodia
    if (hasA && hasL)
    {
        Rng r (S.seed * 29u + S.seedA);
        std::vector<Ev> keep;
        for (auto& e : pa.ev)
        {
            bool drop = false;
            const bool bass = e.str <= 1;
            for (auto& m : pl.ev)
            {
                const bool overlap = m.beat < e.beat + 0.25 && m.beat + m.len > e.beat;
                if (! overlap) continue;
                if (! bass && std::abs (m.note - e.note) <= 2 && m.note != e.note) { drop = true; break; }        // seconda contro la melodia
                if (! bass && e.note > m.note) { drop = true; break; }                                             // mai sopra la melodia
            }
            // dove il lead e' fitto (due attacchi nel battito) l'arpeggio si dirada
            if (! drop && ! bass)
            {
                int on = 0; for (auto& m : pl.ev) if (m.beat >= std::floor (e.beat) && m.beat < std::floor (e.beat) + 1) ++on;
                if (on >= 2 && r.p (0.55f)) drop = true;
            }
            if (! drop) keep.push_back (e);
        }
        pa.ev = keep;
    }
    auto addAll = [&] (Part& p, int layer, float vel)
    {
        for (auto e : p.ev) { e.layer = layer; e.vel = juce::jlimit (0.06f, 1.0f, e.vel * vel); out.ev.push_back (e); }
        for (auto c : p.chokes) out.chokes.push_back (c);
    };
    if (hasB) addAll (pb, 0, 0.88f);
    if (hasA) addAll (pa, 1, 0.82f);
    if (hasL) { addAll (pl, 2, 1.0f); out.bend = pl.bend; }
    const Part& ref = hasB ? pb : hasA ? pa : pl;
    out.chords = ref.chords; out.voic = ref.voic; out.chordAt = ref.chordAt;
    std::stable_sort (out.ev.begin(), out.ev.end(), [] (const Ev& a, const Ev& b) { return a.beat < b.beat; });
    return out;
}
inline Part generateAny (const Settings& S) { return S.mode == COMPOSIZIONE ? generateComposition (S) : generate (S); }

// giri per funzione armonica: catena di accordi con le mosse tipiche (tonica -> predominante -> dominante -> tonica),
// pesata dal genere; l'ultimo accordo deve tornare bene sul primo (il giro gira)
inline std::vector<int> markovProg (bool minor, int genre, int mood, int len, Rng& r)
{
    // pesi di passaggio fra i gradi 0..6 (righe: da, colonne: a)
    static const float MI[7][7] = {
        { 0.3f, 0.2f, 1.0f, 2.0f, 1.0f, 3.0f, 2.0f },   // i
        { 1.0f, 0.0f, 0.5f, 0.5f, 2.0f, 0.5f, 1.0f },   // ii dim
        { 1.0f, 0.0f, 0.0f, 1.5f, 0.5f, 2.0f, 3.0f },   // III
        { 2.0f, 0.3f, 0.5f, 0.0f, 1.5f, 1.5f, 2.0f },   // iv
        { 3.0f, 0.0f, 0.3f, 1.0f, 0.0f, 2.0f, 0.5f },   // v
        { 1.5f, 0.3f, 2.0f, 1.5f, 0.5f, 0.0f, 3.0f },   // VI
        { 3.0f, 0.0f, 2.0f, 0.5f, 0.5f, 1.5f, 0.0f } }; // VII
    static const float MA[7][7] = {
        { 0.3f, 1.0f, 0.7f, 2.5f, 2.5f, 2.0f, 0.0f },   // I
        { 0.5f, 0.0f, 0.5f, 1.0f, 3.0f, 0.5f, 0.0f },   // ii
        { 0.5f, 0.5f, 0.0f, 2.0f, 0.5f, 3.0f, 0.0f },   // iii
        { 2.5f, 1.0f, 0.5f, 0.0f, 2.5f, 1.0f, 0.0f },   // IV
        { 3.0f, 0.3f, 0.5f, 1.5f, 0.0f, 2.5f, 0.0f },   // V
        { 1.0f, 2.0f, 1.0f, 3.0f, 1.5f, 0.0f, 0.0f },   // vi
        { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f } };
    // gradi preferiti dal genere (piu' peso in arrivo)
    auto fav = [&] (int d)
    {
        switch (genre)
        {
            case TRAP: case EMO: case DRILL: return d == 5 || d == 6 || d == 2 ? 1.4f : 1.0f;
            case RNB: case LOFI: case BOOMBAP: return d == 1 || d == 4 || d == 3 ? 1.5f : 1.0f;
            case POP: case FOLK: return d == 0 || d == 3 || d == 4 || d == 5 ? 1.3f : 1.0f;
            case AFRO: case REGGAETON: case LATIN: return d == 3 || d == 4 || d == 5 ? 1.3f : 1.0f;
            default: return 1.0f;
        }
    };
    const auto& T = minor ? MI : MA;
    std::vector<int> best;
    for (int attempt = 0; attempt < 40; ++attempt)
    {
        std::vector<int> p; int cur = r.p (0.7f) ? 0 : (minor ? (r.p (0.5f) ? 5 : 3) : (r.p (0.5f) ? 5 : 3));
        p.push_back (cur);
        for (int k = 1; k < len; ++k)
        {
            float w[7], sum = 0;
            for (int d = 0; d < 7; ++d) { w[d] = T[cur][d] * fav (d); if (mood == TESO && d == 1 && minor) w[d] *= 2.0f; if (k == len - 1) w[d] *= 0.3f + T[d][p[0]]; sum += w[d]; }
            float x = r.f() * sum; int nx = 0; for (int d = 0; d < 7; ++d) { x -= w[d]; if (x <= 0) { nx = d; break; } }
            p.push_back (nx); cur = nx;
        }
        if (T[(size_t) p.back()][(size_t) p[0]] >= 1.0f || p.back() == p[0]) return p;
        best = p;
    }
    return best;
}

// nuova progressione: dal genere o dal mood, sempre dentro la scala; nel giro di 8 a volte la seconda meta' cambia la chiusura
struct NewProg { std::vector<int> prog; int perBar = 1; };
inline NewProg newProgression (int genre, int mood, int scale, int bars, juce::uint32 seed)
{
    Rng r (seed ^ 0xabcdu); NewProg out;
    const int md = mood == M_AUTO ? moodOfGenre (genre) : mood;
    using L = std::vector<std::vector<int>>;
    static const L mi[kMoods] = { {},
        { { 0, 5, 2, 6 }, { 0, 3, 5, 6 }, { 5, 6, 0, 0 }, { 0, 2, 5, 6 }, { 0, 5, 3, 4 } },     // MALINCONICO
        { { 2, 6, 5, 0 }, { 0, 6, 5, 6 }, { 5, 2, 6, 0 }, { 2, 6, 0, 0 } },                    // APERTO
        { { 0, 5 }, { 0, 0, 5, 5 }, { 0, 3 }, { 0, 6 }, { 0, 0, 0, 6 } },                       // IPNOTICO
        { { 5, 3, 0, 0 }, { 0, 5 }, { 3, 6, 2, 5 }, { 5, 2, 3, 0 } },                          // SOGNANTE
        { { 0, 5, 0, 4 }, { 0, 0, 5, 4 }, { 0, 6, 5, 4 }, { 0, 1, 0, 6 } },                    // TESO
        { { 3, 6, 2, 5 }, { 0, 3, 6, 2 }, { 1, 4, 0, 0 } },                                    // CALDO
        { { 0, 6, 5, 6 }, { 0, 5, 6, 0 }, { 0, 2, 6, 5 } } };                                  // ENERGICO
    static const L ma[kMoods] = { {},
        { { 5, 3, 0, 4 }, { 5, 4, 3, 3 }, { 0, 2, 5, 3 } },
        { { 0, 4, 5, 3 }, { 3, 0, 4, 4 }, { 0, 3, 0, 4 }, { 0, 4, 3, 3 } },
        { { 0, 3 }, { 0, 4 }, { 5, 3 }, { 0, 0, 3, 3 } },
        { { 3, 2, 1, 0 }, { 0, 2, 3, 3 }, { 3, 4, 2, 5 } },
        { { 5, 4, 5, 4 }, { 1, 4, 5, 5 } },
        { { 1, 4, 0, 5 }, { 3, 2, 1, 0 }, { 0, 5, 1, 4 } },
        { { 0, 4, 5, 3 }, { 5, 3, 0, 4 }, { 0, 3, 4, 4 } } };
    const bool minor = isMinorScale (scale);
    const auto& ML = minor ? mi[md] : ma[md];
    const auto& GL = progressions (genre);
    const float pick = r.f();
    std::vector<int> p;
    if (pick < 0.3f && ! ML.empty()) p = ML[(size_t) r.i ((int) ML.size())];
    else if (pick < 0.6f) p = GL[(size_t) r.i ((int) GL.size())];
    else p = markovProg (minor, genre, md, md == IPNOTICO ? (r.p (0.5f) ? 2 : 4) : 4, r);
    static const int sub[7] = { 5, 3, 4, 1, 2, 0, 4 };   // sostituto con la stessa funzione
    if (r.p (0.25f) && p.size() >= 4) { const int k = 1 + r.i ((int) p.size() - 1); if (sub[p[(size_t) k]] != p[(size_t) k - 1]) p[(size_t) k] = sub[p[(size_t) k]]; }
    // gradi scomodi nelle scale speciali: III aumentato e vii diminuito dell'armonica, v diminuito della frigia, vii diminuito del maggiore
    for (auto& x : p)
    {
        if (scale == ARMONICA && x == 2) x = 5;
        if (scale == ARMONICA && x == 6) x = 4;
        if (scale == FRIGIA && x == 4) x = 6;
        if (! minor && x == 6) x = 4;
    }
    // ritmo armonico: ipnotico e sognante a volte un accordo ogni 2 battute; teso ed energico a volte due per battuta
    out.perBar = 1;
    if ((md == IPNOTICO || md == SOGNANTE || md == MALINCONICO) && bars >= 8 && p.size() >= 4 && r.p (0.45f)) out.perBar = 0;
    else if ((md == ENERGICO || md == TESO) && p.size() == 2 && bars >= 2 && r.p (0.3f)) out.perBar = 2;
    // giro di 8 con 4 accordi: la seconda volta cambia la chiusura (turnaround)
    if (bars >= 8 && out.perBar == 1 && p.size() == 4 && md != IPNOTICO && r.p (0.5f))
    {
        auto q = p; const int k = 3; int nv = sub[q[(size_t) k]];
        if (scale == ARMONICA && (nv == 2 || nv == 6)) nv = 4;
        if (! minor && nv == 6) nv = 4;
        if (nv != q[2]) q[(size_t) k] = nv;
        p.insert (p.end(), q.begin(), q.end());
    }
    out.prog = p;
    return out;
}
inline juce::String degreeRoman (int code, int scale, int key)
{
    static const char* R[7] = { "I", "II", "III", "IV", "V", "VI", "VII" };
    code = codeDeg (code);
    const auto c = code >= 10 ? borrowedChord (key, scale, code - 10, 0) : diatonic (key, scale, code, 0);
    juce::String s (R[code % 10]); const int i3 = c.third >= 0 ? (c.third - c.root + 12) % 12 : 4;
    if (i3 == 3) s = s.toLowerCase();
    if (c.name.endsWith ("dim")) s += juce::String::fromUTF8 ("\xc2\xb0");
    return s;
}
}
