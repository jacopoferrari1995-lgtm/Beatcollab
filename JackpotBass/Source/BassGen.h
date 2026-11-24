#pragma once
// JACKPOT BASS: generatore di linee di basso (808, basso elettrico, synth bass) sullo stesso giro di Guitar.
// Come l'arpeggio di Guitar la linea nasce da due cose separate: la FIGURA (quali note dell'accordo: fondamentale, quinta,
// ottava, terza, settima, passaggi) e il RITMO. Sopra: densita' a tacche, glide (808 e synth), note di passaggio verso
// l'accordo dopo, ghost note (basso), forma a frasi di 4 battute, mood. La scala dichiarata si rispetta sempre.
#include "GtrGen.h"

namespace bss
{
using namespace gtr;

enum BMode { B808, BASSO, SYNTH, kBModes };
inline const char* bmodeName (int m) { static const char* n[kBModes] = { "808", "BASSO", "SYNTH" }; return n[juce::jlimit (0, kBModes - 1, m)]; }

// figura: R fondamentale, 5 quinta, 3 terza, 7 settima, 2 nona (grado sopra), 8 ottava sopra, L ottava sotto,
// ^ nota di passaggio verso la fondamentale dell'accordo dopo. "~" davanti = ci si arriva scivolando (glide)
struct BFig { const char* name; const char* seq; };
constexpr int kBFigs = 17;
inline const BFig& bfigOf (int i)
{
    static const BFig F[kBFigs] = {
        { "SOLO RADICE", "R" }, { "RADICE E OTTAVA", "R 8" }, { "RADICE E QUINTA", "R 5" }, { "OTTAVE 808", "R R ~8 R" },
        { "QUINTA E OTTAVA", "R 5 8 5" }, { "ARPEGGIO", "R 3 5 8" }, { "DISCESA", "8 5 3 R" }, { "WALKING", "R 3 5 ^" },
        { "FUNK", "R 8 R 5 7 8" }, { "DRILL SLIDE", "R ~8 5 ~R" }, { "OTTAVA SOTTO", "R L" }, { "ACID", "R 8 R 3 R ~5 R 8" },
        { "TERZE", "R 3 R 5" }, { "PASSAGGIO", "R R 2 3" }, { "SALTO DI QUINTA", "R ~5 R ~L" }, { "LIBERA", "" }, { "GRIGLIA", "" } };
    return F[juce::jlimit (0, kBFigs - 1, i)];
}
constexpr int kBLibera = kBFigs - 2, kBGrid = kBFigs - 1;

// ritmo: X accento, x nota, o ghost (nota stoppata), - niente. 16 passi o 12 (terzine)
struct BRit { const char* name; const char* steps; };
constexpr int kBRits = 14;
inline const BRit& britOf (int i)
{
    static const BRit R[kBRits] = {
        { "TRAP 808", "X------x--X-----" }, { "TRAP ROLL", "X------x--X-x-xx" }, { "DRILL", "X--x---x--X-x---" }, { "SEMIBREVE", "X---------------" },
        { "MINIME", "X-------X-------" }, { "SEMIMINIME", "X---x---X---x---" }, { "OTTAVI", "X-x-x-x-X-x-x-x-" }, { "SEDICESIMI", "Xxxxxxxxxxxxxxxx" },
        { "TRESILLO 3+3+2", "X--x--x-X--x--x-" }, { "DEMBOW", "X---X--xX---X--x" }, { "FUNK", "X-oxo-x-X-oxo-xo" }, { "AFRO", "X--x--x---x-X---" },
        { "R&B", "X--x-x--X-----x-" }, { "TERZINE", "X--x-xX--x-x" } };
    return R[juce::jlimit (0, kBRits - 1, i)];
}
// stili pronti per ogni modo: (figura, ritmo, nome)
struct BStyle { const char* name; int fig, rit; float glide, len; };
inline const std::vector<BStyle>& bstyles (int mode)
{
    static const std::vector<BStyle> S[kBModes] = {
        { { "TRAP LUNGO", 0, 0, 0.2f, 0.95f }, { "TRAP ROLL", 3, 1, 0.4f, 0.9f }, { "DRILL SLIDE", 9, 2, 0.7f, 0.9f }, { "OTTAVE", 1, 8, 0.4f, 0.8f },
          { "EMO 808", 2, 4, 0.2f, 0.95f }, { "SALTO DI QUINTA", 14, 0, 0.6f, 0.9f }, { "REGGAETON", 0, 9, 0.1f, 0.7f }, { "AFRO", 2, 11, 0.2f, 0.8f } },
        { { "POP OTTAVI", 0, 6, 0, 0.75f }, { "ROCK", 1, 6, 0, 0.6f }, { "FOLK R-5", 2, 5, 0, 0.7f }, { "R&B", 13, 12, 0, 0.8f },
          { "WALKING", 7, 5, 0, 0.85f }, { "FUNK", 8, 10, 0, 0.45f }, { "REGGAETON", 2, 9, 0, 0.6f }, { "AFRO", 12, 11, 0, 0.6f } },
        { { "SYNTHWAVE", 0, 6, 0, 0.55f }, { "ACID", 11, 7, 0.35f, 0.5f }, { "REESE", 0, 4, 0.2f, 1.0f }, { "HOUSE OTTAVE", 1, 6, 0, 0.4f },
          { "DISCO", 4, 6, 0, 0.45f }, { "TRAP SYNTH", 3, 0, 0.4f, 0.9f }, { "PULSE", 0, 7, 0, 0.35f }, { "ARPEGGIO", 5, 7, 0.1f, 0.45f } } };
    return S[juce::jlimit (0, kBModes - 1, mode)];
}
// figura a caso: 2-8 note, la fondamentale sempre presente
inline juce::String randomBassFigure (juce::Random& r)
{
    static const char* v[8] = { "R", "5", "8", "3", "7", "L", "2", "^" };
    const int n = 2 + r.nextInt (7); juce::StringArray t; t.add ("R");
    for (int i = 1; i < n; ++i) { juce::String x = v[r.nextInt (8)]; if (r.nextInt (5) == 0 && x != "^") x = "~" + x; t.add (x); }
    return t.joinIntoString (" ");
}

struct BSettings
{
    int key = 9, scale = MINORE, genre = TRAP, mood = 0, mode = B808;
    int fig = 0, rit = 0, rot = 0; juce::String figSeq { "R 8 5 R" };
    std::array<juce::uint16, 5> grid { 0x0010, 0x0400, 0, 0x8081, 0 };   // righe: 8, 5, 3, R, L; un bit per sedicesimo
    int bars = 4, perBar = 1; float color = -1; int octave = 0;
    float density = 0.5f, dyn = 0.6f, human = 0.3f, swing = 0.0f, space = 0.2f, len = 0.9f;
    float glide = 0.3f, approach = 0.3f, ghost = 0.3f, borrow = 0.0f, vary = 0.45f;
    std::vector<int> prog; juce::uint32 seed = 1; double bpm = 120;
};
// registro della fondamentale per ogni modo (piu' l'ottava scelta)
inline int regLo (const BSettings& S) { static const int lo[kBModes] = { 24, 28, 36 }; return juce::jmax (21, lo[juce::jlimit (0, kBModes - 1, S.mode)] + 12 * juce::jlimit (-1, 1, S.octave)); }

// basso a 4 corde (MI LA RE SOL), per mostrare la linea sul manico e scrivere la corda
inline std::array<int, 4> bassTuning() { return { 28, 33, 38, 43 }; }

struct BGen
{
    const BSettings& S; Part& P; Rng hr, mr; MoodK mk; float dens, space;
    double slotLen = 4; int nSlots = 4;
    std::vector<std::array<bool, 12>> allow; std::array<bool, 12> inScale {};
    BGen (const BSettings& s, Part& p) : S (s), P (p), hr (s.seed ^ 0x51ed27u), mr (s.seed)
    {
        mk = moodK (S.mood == M_AUTO ? moodOfGenre (S.genre) : S.mood);
        dens = juce::jlimit (0.0f, 1.0f, S.density); space = juce::jlimit (0.0f, 1.0f, S.space + mk.space * 0.5f);
    }
    int role (int b) const { if (P.bars == 1) return 0; if (P.bars == 2) return b == 0 ? 0 : 3; return b % 4; }
    bool lifted (int b) const { return P.bars >= 8 && (b / 4) % 2 == 1; }
    int slotAt (double t) const { return juce::jlimit (0, nSlots - 1, (int) std::floor (t / slotLen + 1e-9)); }
    double sw (double t, double grid) const
    {
        if (S.swing <= 0.001f || std::abs (grid - 1.0 / 3.0) < 0.01) return t;
        const double fr = t - std::floor (t / (grid * 2)) * grid * 2;
        return std::abs (fr - grid) < 1e-6 ? t + S.swing * grid / 3.0 : t;
    }
    double hum() { return (double) hr.g() * S.human * 0.012; }
    bool ok (int slot, int m) const { return allow[(size_t) slot][(size_t) (((m % 12) + 12) % 12)]; }
    // nota della scala vicina (sopra o sotto) a una nota data
    int scaleNeighbor (int slot, int m, int dir) const { int n = m; for (int k = 0; k < 12; ++k) { n += dir; if (ok (slot, n)) return n; } return m; }
    int rootPitch (int slot) const { const int lo = regLo (S); const int pc = P.chords[(size_t) slot].root; int m = lo + ((pc - lo) % 12 + 12) % 12; return m; }
    // token della figura -> nota
    int noteOf (int slot, const juce::String& tk) const
    {
        const auto& c = P.chords[(size_t) slot]; const int R = rootPitch (slot);
        auto above = [&] (int pc) { int m = R + ((pc - R) % 12 + 12) % 12; return m; };
        const juce::String t = tk.removeCharacters ("~");
        if (t == "R") return R;
        if (t == "8") return R + 12;
        if (t == "L") return R - 12 >= 21 ? R - 12 : R;
        if (t == "5") { const int f = c.fifth >= 0 ? c.fifth : (c.root + 7) % 12; return above (f); }
        if (t == "3") { const int th = c.third >= 0 ? c.third : (c.tones.size() > 1 ? c.tones[1] : (c.root + 2) % 12); return above (th); }
        if (t == "7") { if (c.tones.size() >= 4) return above (c.tones[3]); return scaleNeighbor (slot, R + 12, -1); }
        if (t == "2") return scaleNeighbor (slot, R, 1);
        if (t == "^")
        {   // passaggio: la nota della scala appena sotto (o sopra) la fondamentale dell'accordo dopo
            const int nx = juce::jmin (nSlots - 1, slot + 1); const int target = rootPitch (nx == slot ? 0 : nx);
            return scaleNeighbor (slot, target, R <= target ? -1 : 1);
        }
        return R;
    }
    static int importance (int k, int steps) { if (steps == 12) return k % 6 == 0 ? 4 : k % 3 == 0 ? 3 : 1; return k % 8 == 0 ? 4 : k % 4 == 0 ? 3 : k % 2 == 0 ? 2 : 1; }
    // stesso metodo dell'arpeggio di Guitar: sotto il 50% toglie le note meno importanti, sopra aggiunge riempimenti
    std::vector<int> rhythmMask (const char* pat, int steps, float d, juce::uint32 salt) const
    {
        std::vector<int> m ((size_t) steps, 0); int n0 = 0;
        for (int k = 0; k < steps; ++k) { m[(size_t) k] = pat[k] == 'X' ? 2 : pat[k] == 'x' ? 1 : pat[k] == 'o' ? 4 : 0; n0 += m[(size_t) k] > 0; }
        const float f = d < 0.5f ? 0.3f + 1.4f * d : 1.0f + 2.6f * (d - 0.5f);
        const int target = juce::jlimit (1, steps, (int) std::round ((float) n0 * f));
        Rng r (S.seed * 131u + salt); std::vector<std::pair<float, int>> rank;
        if (target < n0)
        {
            for (int k = 1; k < steps; ++k) if (m[(size_t) k] > 0) rank.push_back ({ (float) importance (k, steps) + (m[(size_t) k] == 2 ? 2.0f : 0.0f) - (m[(size_t) k] == 4 ? 1.0f : 0.0f) + r.f() * 0.9f, k });
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
    void add (double t, double len, int note, float vel, int slot, bool mute, int art)
    {
        while (note < 21) note += 12; while (note > 96) note -= 12;   // per ottave: resta nella scala
        Ev e; e.beat = juce::jmax (0.0, t); e.len = len; e.note = note; e.vel = juce::jlimit (0.06f, 1.0f, vel); e.slot = slot; e.mute = mute; e.art = art;
        P.ev.push_back (e);
    }
    void line()
    {
        const bool useGrid = S.fig == kBGrid;
        const auto& R = britOf (S.rit);
        const int steps = useGrid ? 16 : (int) std::strlen (R.steps);
        const double st = 4.0 / steps; const bool tripl = steps == 12;
        auto fig = juce::StringArray::fromTokens (S.fig == kBLibera ? S.figSeq.toUpperCase() : juce::String (bfigOf (S.fig).seq), " ", "");
        for (auto& t : fig) t = t.retainCharacters ("~R5378L2^");
        fig.removeEmptyStrings(); if (fig.isEmpty()) fig.add ("R");
        std::vector<int> base, lift;
        if (useGrid) { base.assign (16, 0); for (int k = 0; k < 16; ++k) for (int rr = 0; rr < 5; ++rr) if (S.grid[(size_t) rr] & (1 << k)) base[(size_t) k] = k % 4 == 0 ? 2 : 1; lift = base; }
        else { base = rhythmMask (R.steps, steps, dens, 1); lift = rhythmMask (R.steps, steps, juce::jmin (1.0f, dens + 0.12f * mk.lift), 2); }
        // ghost note (basso): sedicesimi stoppati in piu' sui posti vuoti, solo nel modo BASSO
        auto gridTok = [&] (int k) { static const char* rc[5] = { "8", "5", "3", "R", "L" }; for (int rr = 0; rr < 5; ++rr) if (S.grid[(size_t) rr] & (1 << k)) return juce::String (rc[rr]); return juce::String ("R"); };
        int lastSlot = -1, fi = 0;
        for (int b = 0; b < P.bars; ++b)
        {
            const int rl = role (b); Rng br (S.seed * 7919u + (juce::uint32) b * 104729u);
            auto m = lifted (b) && rl != 3 ? lift : base;
            if (S.mode == BASSO && S.ghost > 0.01f)
            {
                Rng gr (S.seed * 53u + (juce::uint32) (rl == 1 ? b : 0));
                for (int k = 1; k < steps; ++k) if (m[(size_t) k] == 0 && importance (k, steps) <= 2 && gr.p (S.ghost * 0.35f)) m[(size_t) k] = 4;
            }
            const bool same = br.p (mk.repeat);
            const bool reverseHalf = ! useGrid && rl == 1 && ! same && fig.size() > 1;
            // fine frase: 808 e synth fanno un roll (terzine di sedicesimi che salgono di ottava), il basso una corsa di passaggio; con PAUSE alta si respira
            const bool fill = ! useGrid && rl == 3 && P.bars > 1 && space <= 0.45f && dens > 0.25f && ! br.p (mk.repeat - 0.3f);
            double breath = 1e9;
            if (rl == 3 && space > 0.45f && br.p (space - 0.15f)) breath = space > 0.75f ? 2.0 : 3.0;
            std::vector<std::pair<double, int>> onsets;   // posizione, tipo
            for (int k = 0; k < steps; ++k) { const double pos = k * st; if (pos >= breath - 1e-6) break; if (fill && pos >= 3.0 - 1e-6) break; if (m[(size_t) k]) onsets.push_back ({ pos, m[(size_t) k] }); }
            if (fill)
            {
                if (S.mode == BASSO) { for (int q = 0; q < 4; ++q) onsets.push_back ({ 3.0 + q * 0.25, 10 + q }); }
                else { const int n = dens > 0.6f ? 6 : 3; for (int q = 0; q < n; ++q) onsets.push_back ({ 3.0 + q * (1.0 / n), 20 + q }); }
            }
            for (size_t q = 0; q < onsets.size(); ++q)
            {
                const double pos = onsets[q].first; const int type = onsets[q].second;
                const double tb = b * 4.0 + pos, t = sw (tb, tripl ? 1.0 / 3.0 : 0.25) + hum();
                const int i = slotAt (tb + 1e-6);
                if (i != lastSlot) { fi = 0; lastSlot = i; }
                juce::String tk; bool glide = false, ghost = type == 4;
                int note;
                if (type >= 20) { note = noteOf (i, (type - 20) % 2 == 0 ? "R" : "8"); glide = type > 20; }               // roll 808
                else if (type >= 10)
                {   // corsa del basso verso l'accordo dopo: 4 note della scala che arrivano per grado
                    const int nx = juce::jmin (nSlots - 1, i + 1); const int target = rootPitch (nx == i ? 0 : nx);
                    note = target; for (int z = 0; z < 4 - (type - 10); ++z) note = scaleNeighbor (i, note, target >= rootPitch (i) ? -1 : 1);
                }
                else if (ghost) note = noteOf (i, "R");
                else
                {
                    if (useGrid) tk = gridTok ((int) std::round (pos / st));
                    else { const int n = fig.size(); int idx = (fi + S.rot) % n; if (reverseHalf && pos >= 2.0) idx = n - 1 - idx; tk = fig[idx]; ++fi; }
                    glide = tk.startsWith ("~");
                    note = noteOf (i, tk);
                }
                // passaggio verso l'accordo dopo: l'ultima nota prima del cambio diventa una nota della scala vicina alla nuova fondamentale
                const double nextOn = q + 1 < onsets.size() ? b * 4.0 + onsets[q + 1].first : (b + 1) * 4.0;
                const int ni = slotAt (nextOn + 1e-6);
                if (type < 10 && ! ghost && ni != i && S.approach > 0.01f && br.p (S.approach * 0.8f))
                {
                    const int target = rootPitch (ni); const int a = scaleNeighbor (i, target, note <= target ? -1 : 1);
                    if (std::abs (a - note) <= 7) note = a;
                }
                // glide anche a caso (808 e synth) sui salti di almeno una terza minore
                if (S.mode != BASSO && ! P.ev.empty() && ! ghost && std::abs (note - P.ev.back().note) >= 3 && mr.p (S.glide * 0.5f)) glide = true;
                if (S.mode == BASSO) glide = glide && mr.p (S.glide);   // sul basso il glide e' una scivolata vera: piu' rara
                // durata: fino alla nota dopo, accorciata da LUNGHEZZA; ghost cortissime
                const double gap = nextOn - tb;
                double len = ghost ? 0.06 : juce::jmax (0.08, gap * (0.2 + 0.8 * S.len));
                if (breath < 1e8 && q + 1 == onsets.size()) len = juce::jmax (len, b * 4.0 + 4.0 - tb);
                // dinamica: battere forte, accenti del ritmo, ghost piano, roll in crescendo
                const int imp = type < 10 ? importance ((int) std::round (pos / st), steps) : 2;
                float v = 0.62f + (imp >= 4 ? 0.14f : imp == 3 ? 0.08f : imp == 2 ? 0.03f : 0.0f) * (0.4f + S.dyn);
                if (type == 2) v += 0.05f * S.dyn;
                if (type == 3) v *= 0.85f;
                if (ghost) v = 0.32f + 0.1f * hr.f();
                if (type >= 20) v = 0.6f + 0.06f * (float) (type - 20);
                v += hr.g() * S.human * 0.05f;
                // glide: la nota prima si allunga e si sovrappone (i campionatori 808 e i synth mono scivolano)
                if (glide && ! P.ev.empty() && P.ev.back().beat < t) { auto& pv = P.ev.back(); pv.len = juce::jmax (pv.len, t - pv.beat + 0.06); }
                add (t, len, note, v, i, ghost, glide ? 2 : 0);
            }
        }
    }
    void finish()
    {
        const double L = P.len();
        std::stable_sort (P.ev.begin(), P.ev.end(), [] (const Ev& a, const Ev& b) { return a.beat < b.beat; });
        // linea monofonica: ogni nota chiude la precedente (tranne il glide, che si sovrappone appena)
        for (size_t k = 0; k + 1 < P.ev.size(); ++k)
        {
            auto& a = P.ev[k]; const auto& n = P.ev[k + 1];
            if (n.art == 2) a.len = juce::jmax (0.02, n.beat - a.beat + 0.06);
            else if (a.beat + a.len > n.beat - 0.004) a.len = juce::jmax (0.02, n.beat - a.beat - 0.004);
        }
        static const float bv[4] = { 1.0f, 0.94f, 1.03f, 0.96f };
        for (auto& e : P.ev) { e.vel = juce::jlimit (0.06f, 1.0f, e.vel * (e.mute ? 1.0f : bv[((int) (e.beat / 4.0)) % 4])); e.len = juce::jmin (e.len, L - 0.005 - e.beat); e.len = juce::jmax (0.02, e.len); }
        P.ev.erase (std::remove_if (P.ev.begin(), P.ev.end(), [L] (const Ev& e) { return e.beat >= L || e.beat < 0; }), P.ev.end());
        // corda e tasto sul basso a 4 corde (per il manico e per chi usa un basso campionato a corde)
        const auto tun = bassTuning(); double hand = 3;
        for (auto& e : P.ev)
        {
            int bs = 0; double bc = 1e9;
            for (int s = 0; s < 4; ++s) { const int f = e.note - tun[(size_t) s]; if (f < 0 || f > 20) continue; const double c = std::abs (f - hand) + s * 0.1; if (c < bc) { bc = c; bs = s; } }
            e.str = bs; e.fret = juce::jmax (0, e.note - tun[(size_t) bs]); hand += (e.fret - hand) * 0.3;
        }
    }
};

inline Part generateBass (const BSettings& S)
{
    Part P; P.bars = juce::jlimit (1, 16, S.bars);
    BGen G (S, P);
    G.slotLen = juce::jmin (P.len(), S.perBar >= 2 ? 2.0 : S.perBar == 0 ? 8.0 : 4.0); G.nSlots = juce::jmax (1, (int) std::round (P.len() / G.slotLen));
    G.inScale = scaleSet (S.key, S.scale);
    std::vector<int> prog = S.prog; if (prog.empty()) prog = progressions (S.genre).front();
    Settings gs; gs.key = S.key; gs.scale = S.scale; gs.genre = S.genre; gs.color = S.color;
    for (int i = 0; i < G.nSlots; ++i)
    {
        int code = prog[(size_t) (i % (int) prog.size())];
        if (codeDeg (code) < 10 && S.borrow > 0.001f && canBorrow (S.scale, codeDeg (code)))
        {
            const int nxt = codeDeg (prog[(size_t) ((i + 1) % (int) prog.size())]) % 10;
            if (G.mr.p (S.borrow * (nxt == 0 ? 0.9f : 0.45f))) code += 10;
        }
        const Chord c = slotChord (gs, code);
        P.chords.push_back (c); P.voic.push_back ({}); P.chordAt.push_back (i * G.slotLen);
        auto a = G.inScale;
        if (c.borrowed) for (int t : c.tones) if (! a[(size_t) t]) { a[(size_t) t] = true; a[(size_t) ((t + 11) % 12)] = false; }
        for (int t : c.tones) a[(size_t) t] = true;
        G.allow.push_back (a);
    }
    G.line();
    varyPart (P, S.vary, S.seed, G.inScale, true, regLo (S), regLo (S) + 24);
    G.finish();
    return P;
}
}
