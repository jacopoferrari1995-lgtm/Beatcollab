#pragma once
// JACKPOT GUITAR: generatore di parti di chitarra suonabili davvero.
// Tutto passa dalla tastiera: ogni accordo e' un voicing cercato sulle 6 corde (una nota per corda, al massimo 4 dita,
// barre' riconosciuto, corde a vuoto), ogni nota del lead ha la sua corda e il suo tasto. Da li' nascono quattro modi:
//   ACCORDI  accordi tenuti o colpi (con abbellimenti neo soul: hammer-on e scivolate verso la nota alta)
//   RITMICA  pennate su/giu' sfalsate corda per corda, stoppate, palm mute, anticipi
//   ARPEGGIO fingerpicking (PIMA, Travis, arpeggi trap/emo, 6/8): le corde suonano finche' non vengono ripizzicate
//   LEAD     frasi con motivo e risposta, note dell'accordo sui tempi forti, bend, scivolate, legati, vibrato
// La scala dichiarata si rispetta sempre; gli accordi presi in prestito entrano solo con INTERSCAMBIO alzato.
#include <juce_core/juce_core.h>
#include <vector>
#include <array>
#include <cmath>
#include <algorithm>
#include <functional>

namespace gtr
{
// ------------------------------------------------------------------------------------------------ teoria
inline const char* noteName (int pc) { static const char* n[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }; return n[((pc % 12) + 12) % 12]; }
enum Scale { MINORE, ARMONICA, DORICA, FRIGIA, MAGGIORE, MISOLIDIA, kScales };
inline const char* scaleName (int s) { static const char* n[kScales] = { "MINORE", "ARMONICA", "DORICA", "FRIGIA", "MAGGIORE", "MISOLIDIA" }; return n[juce::jlimit (0, kScales - 1, s)]; }
inline const int* scaleSteps (int s)
{
    static const int st[kScales][7] = { { 0, 2, 3, 5, 7, 8, 10 }, { 0, 2, 3, 5, 7, 8, 11 }, { 0, 2, 3, 5, 7, 9, 10 }, { 0, 1, 3, 5, 7, 8, 10 }, { 0, 2, 4, 5, 7, 9, 11 }, { 0, 2, 4, 5, 7, 9, 10 } };
    return st[juce::jlimit (0, kScales - 1, s)];
}
inline bool isMinorScale (int s) { return s <= FRIGIA; }

struct Chord
{
    int deg = 0;                 // grado 0..6 nella scala (o -1 se preso in prestito)
    int root = 0;                // classe di altezza
    std::vector<int> tones;      // classi di altezza, la prima e' la fondamentale
    int third = -1, fifth = -1;  // per il controllo dei voicing
    juce::String name;
    bool borrowed = false;
};
// accordo diatonico per terze sul grado; color 0 triade, 1 settima, 2 nona (senza quinta se serve), 3 sus / add9
inline Chord diatonic (int key, int scale, int deg, int color)
{
    const int* st = scaleSteps (scale); Chord c; c.deg = deg;
    auto pcOf = [&] (int d) { return (key + st[((d % 7) + 7) % 7]) % 12; };
    c.root = pcOf (deg); const int t3 = pcOf (deg + 2), t5 = pcOf (deg + 4), t7 = pcOf (deg + 6), t9 = pcOf (deg + 1);
    c.tones = { c.root, t3, t5 }; c.third = t3; c.fifth = t5;
    const int i3 = (t3 - c.root + 12) % 12, i5 = (t5 - c.root + 12) % 12, i7 = (t7 - c.root + 12) % 12, i9 = (t9 - c.root + 12) % 12;
    juce::String q = i3 == 3 ? (i5 == 6 ? "dim" : "m") : (i5 == 8 ? "aug" : "");
    if (color == 1 || color == 2) { c.tones.push_back (t7); q = q == "m" ? (i7 == 10 ? "m7" : "m(maj7)") : q == "dim" ? (i7 == 9 ? "dim7" : "m7b5") : (i7 == 11 ? "maj7" : "7"); }
    if (color == 2 && i9 == 2) { c.tones.push_back (t9); q = q.replace ("7", "9"); if (q == "maj9" || q == "m9" || q == "9") {} else q += "(9)"; }
    const int s4 = pcOf (deg + 3), t6 = pcOf (deg + 5), i4 = (s4 - c.root + 12) % 12, i6 = (t6 - c.root + 12) % 12;
    if (color == 3)
    {
        if (i9 == 2 && i5 == 7) { c.tones.push_back (t9); q += "add9"; }
        else if (i5 == 7 && i4 == 5) { c.tones = { c.root, s4, t5 }; c.third = -1; q = "sus4"; }
    }
    // colori scelti accordo per accordo; se la nota non e' nella scala resta la triade
    if (color == 4 && i9 == 2 && i5 == 7) { c.tones = { c.root, t9, t5 }; c.third = -1; q = "sus2"; }
    if (color == 5 && i4 == 5 && i5 == 7) { c.tones = { c.root, s4, t5 }; c.third = -1; q = "sus4"; }
    if (color == 6 && i9 == 2 && i5 == 7) { c.tones.push_back (t9); q = q == "m" ? "m(add9)" : q + "add9"; }
    if (color == 7 && i6 == 9 && i5 == 7) { c.tones.push_back (t6); q += "6"; }
    c.name = juce::String (noteName (c.root)) + q;
    return c;
}
// accordi presi in prestito (interscambio modale), usati solo con INTERSCAMBIO > 0
inline Chord borrowedChord (int key, int scale, int deg, int color)
{
    // in minore: il V maggiore (dall'armonica) e il IV maggiore (dalla dorica); in maggiore: il iv minore e il bVII (dalla minore)
    if (isMinorScale (scale)) { Chord c = diatonic (key, deg == 4 ? ARMONICA : DORICA, deg, color); c.borrowed = true; c.deg = deg; return c; }
    Chord c = diatonic (key, MINORE, deg, color); c.borrowed = true; return c;
}

// progressioni per genere (gradi 0..6). Sono giri tipici, tutti diatonici; il prestito si aggiunge a parte
enum Genre { TRAP, DRILL, EMO, RNB, POP, REGGAETON, AFRO, BOOMBAP, LOFI, ROCK, FUNK, FOLK, LATIN, kGenres };
inline const char* genreName (int g) { static const char* n[kGenres] = { "TRAP", "DRILL", "EMO RAP", "R&B", "POP", "REGGAETON", "AFRO", "BOOM BAP", "LO-FI", "ROCK", "FUNK", "FOLK", "LATIN" }; return n[juce::jlimit (0, kGenres - 1, g)]; }
inline const std::vector<std::vector<int>>& progressions (int g)
{
    static const std::vector<std::vector<int>> P[kGenres] = {
        /* TRAP */      { { 0, 5, 2, 6 }, { 0, 6, 5, 6 }, { 0, 5 }, { 0, 3, 5, 4 }, { 0, 0, 5, 5 }, { 0, 2, 5, 6 }, { 5, 6, 0, 0 } },
        /* DRILL */     { { 0, 5, 4, 0 }, { 0, 1, 0, 1 }, { 0, 6, 5, 4 }, { 0, 3, 4, 0 }, { 0, 5, 6, 4 } },
        /* EMO */       { { 5, 6, 0, 0 }, { 0, 5, 2, 6 }, { 0, 3, 5, 6 }, { 5, 2, 6, 0 } },
        /* R&B */       { { 1, 4, 0, 0 }, { 3, 2, 5, 0 }, { 0, 5, 1, 4 }, { 3, 4, 2, 5 }, { 0, 3, 0, 3 } },
        /* POP */       { { 0, 4, 5, 3 }, { 5, 3, 0, 4 }, { 0, 5, 3, 4 }, { 3, 0, 4, 5 } },
        /* REGGAETON */ { { 0, 5, 2, 6 }, { 0, 3, 6, 2 }, { 0, 6, 5, 6 }, { 0, 3, 4, 0 } },
        /* AFRO */      { { 0, 3, 5, 4 }, { 1, 4, 0, 0 }, { 0, 4, 5, 3 }, { 5, 3, 0, 4 } },
        /* BOOM BAP */  { { 1, 4, 0, 5 }, { 0, 3, 0, 3 }, { 5, 1, 4, 0 }, { 0, 5, 1, 4 } },
        /* LO-FI */     { { 1, 4, 0, 5 }, { 3, 2, 1, 0 }, { 0, 5, 3, 4 }, { 3, 4, 2, 5 } },
        /* ROCK */      { { 0, 6, 3, 0 }, { 0, 3, 4, 3 }, { 0, 5, 3, 4 }, { 0, 4, 3, 4 } },
        /* FUNK */      { { 0, 3, 0, 3 }, { 1, 4, 1, 4 }, { 0, 0, 3, 3 } },
        /* FOLK */      { { 0, 3, 0, 4 }, { 0, 4, 5, 3 }, { 5, 3, 0, 4 }, { 0, 3, 4, 4 } },
        /* LATIN */     { { 0, 6, 5, 4 }, { 0, 3, 4, 0 }, { 3, 2, 1, 0 }, { 0, 5, 3, 4 } } };
    return P[juce::jlimit (0, kGenres - 1, g)];
}
// colore tipico del genere (0 triadi, 1 settime, 2 none, 3 sus/add9)
inline int genreColor (int g) { static const int c[kGenres] = { 0, 0, 3, 2, 3, 0, 1, 1, 2, 0, 1, 0, 0 }; return c[juce::jlimit (0, kGenres - 1, g)]; }

// ------------------------------------------------------------------------------------------------ mood
// il mood cambia il carattere della parte: giri, colore, registro, quanto si ripete, quanto respira, come va la melodia
enum Mood { M_AUTO, MALINCONICO, APERTO, IPNOTICO, SOGNANTE, TESO, CALDO, ENERGICO, kMoods };
inline const char* moodName (int m) { static const char* n[kMoods] = { "AUTO", "MALINCONICO", "APERTO", "IPNOTICO", "SOGNANTE", "TESO", "CALDO", "ENERGICO" }; return n[juce::jlimit (0, kMoods - 1, m)]; }
inline int moodOfGenre (int g) { static const int m[kGenres] = { MALINCONICO, TESO, MALINCONICO, CALDO, APERTO, CALDO, APERTO, CALDO, SOGNANTE, ENERGICO, ENERGICO, APERTO, CALDO }; return m[juce::jlimit (0, kGenres - 1, g)]; }
struct MoodK
{
    float dens = 0, space = 0, ring = 0;  // si sommano alle manopole
    int color = -1;                       // colore degli accordi quando COLORE e' AUTO (-1 = quello del genere)
    float repeat = 0.5f;                  // quanto si ripete il motivo (ostinato)
    float pedal = 0.2f;                   // nota comune tenuta in cima agli accordi
    float lift = 0.5f;                    // quanto sale la seconda meta' del giro di 8
    int contour = 0;                      // 0 arco, 1 scende, 2 sale, 3 stretto (attorno a una nota)
    float longN = 0.5f;                   // note lunghe nella melodia
    int reg = 0;                          // registro (tasti in piu' o in meno)
};
inline MoodK moodK (int mood)
{
    MoodK k;
    switch (mood)
    {
        case MALINCONICO: k.dens = -0.08f; k.space = 0.15f; k.ring = 0.1f; k.repeat = 0.6f; k.pedal = 0.3f; k.contour = 1; k.longN = 0.65f; break;
        case APERTO:      k.space = 0.1f; k.ring = 0.2f; k.color = 3; k.repeat = 0.5f; k.pedal = 0.65f; k.contour = 0; k.longN = 0.6f; k.lift = 0.8f; break;
        case IPNOTICO:    k.space = 0.0f; k.ring = 0.15f; k.repeat = 0.92f; k.pedal = 0.85f; k.contour = 3; k.longN = 0.4f; k.lift = 0.15f; break;
        case SOGNANTE:    k.dens = -0.12f; k.space = 0.2f; k.ring = 0.25f; k.color = 2; k.repeat = 0.55f; k.pedal = 0.45f; k.contour = 0; k.longN = 0.75f; k.reg = 3; break;
        case TESO:        k.dens = 0.05f; k.space = 0.1f; k.ring = -0.3f; k.color = 0; k.repeat = 0.75f; k.pedal = 0.5f; k.contour = 3; k.longN = 0.3f; k.reg = -1; break;
        case CALDO:       k.color = 1; k.repeat = 0.5f; k.pedal = 0.25f; k.contour = 0; k.longN = 0.5f; break;
        case ENERGICO:    k.dens = 0.15f; k.space = -0.15f; k.ring = -0.1f; k.repeat = 0.6f; k.pedal = 0.2f; k.contour = 2; k.longN = 0.3f; k.lift = 0.7f; break;
        default: break;
    }
    return k;
}

// ------------------------------------------------------------------------------------------------ tastiera
constexpr int kStr = 6;
enum Tuning { STANDARD, DROPD, MEZZOTONO, kTunings };
inline const char* tuningName (int t) { static const char* n[kTunings] = { "STANDARD", "DROP D", "-1/2 TONO" }; return n[juce::jlimit (0, kTunings - 1, t)]; }
inline std::array<int, kStr> tuning (int t)
{
    switch (t) { case DROPD: return { 38, 45, 50, 55, 59, 64 }; case MEZZOTONO: return { 39, 44, 49, 54, 58, 63 }; default: return { 40, 45, 50, 55, 59, 64 }; }
}
constexpr int kFrets = 17;
struct Voicing { std::array<int, kStr> fret { -1, -1, -1, -1, -1, -1 }; float score = -1e9f; int lo = 0, hi = 0; bool barre = false; int count() const { int n = 0; for (int f : fret) n += f >= 0; return n; } };
enum VStyle { V_OPEN, V_BARRE, V_TRIAD, V_POWER, V_JAZZ, kVStyles };
inline const char* vstyleName (int v) { static const char* n[kVStyles] = { "APERTI", "BARRE'", "TRIADI ALTE", "POWER", "JAZZ" }; return n[juce::jlimit (0, kVStyles - 1, v)]; }

// cerca il voicing migliore: una nota per corda, al massimo 4 dita (il barre' conta uno), apertura di 4 tasti al massimo
inline Voicing findVoicing (const Chord& ch, const std::array<int, kStr>& tun, int style, int pos, const Voicing* prev, juce::Random& rnd, float rndAmt = 0.0f, int topTarget = -1)
{
    Voicing best;
    const bool power = style == V_POWER, triad = style == V_TRIAD, jazz = style == V_JAZZ, open = style == V_OPEN;
    std::vector<int> want = ch.tones; if (power) want = { ch.root, ch.fifth >= 0 ? ch.fifth : (ch.root + 7) % 12 };
    auto inChord = [&] (int pc) { for (int t : want) if (t == pc) return true; return false; };
    const int wLo = power ? juce::jmax (0, pos - 3) : 0, wHi = juce::jmin (kFrets - 4, pos + 5);
    for (int w = wLo; w <= wHi; ++w)
    {
        // per ogni corda le scelte possibili in questa finestra: muta, a vuoto (se e' nota dell'accordo), tasti w..w+3
        std::array<std::vector<int>, kStr> opt;
        for (int s = 0; s < kStr; ++s)
        {
            opt[(size_t) s].push_back (-1);
            if (inChord (tun[(size_t) s] % 12) && ! jazz) opt[(size_t) s].push_back (0);
            for (int f = juce::jmax (1, w); f <= w + 3; ++f) if (inChord ((tun[(size_t) s] + f) % 12)) opt[(size_t) s].push_back (f);
        }
        std::array<int, kStr> cur {};
        std::function<void (int)> rec = [&] (int s)
        {
            if (s == kStr)
            {
                // corde che suonano: per gli accordi pennati devono essere vicine (niente corda muta in mezzo, tranne il jazz)
                int first = -1, last = -1, n = 0, holes = 0;
                for (int i = 0; i < kStr; ++i) if (cur[(size_t) i] >= 0) { if (first < 0) first = i; last = i; ++n; }
                if (n < (power ? 2 : triad ? 3 : 3)) return;
                for (int i = first; i <= last; ++i) if (cur[(size_t) i] < 0) ++holes;
                if (holes > (jazz ? 1 : 0)) return;
                if (power && (n > 3 || first > 2)) return;
                if ((open || style == V_BARRE) && n < 4) return;
                if (triad && (n != 3 && n != 4)) return;
                if (triad && first < 2) return;
                // dita: tasti premuti distinti (barre' = la stessa nota piu' bassa su piu' corde, dalla corda piu' bassa)
                int minF = 99, maxF = 0; for (int i = 0; i < kStr; ++i) if (cur[(size_t) i] > 0) { minF = juce::jmin (minF, cur[(size_t) i]); maxF = juce::jmax (maxF, cur[(size_t) i]); }
                if (maxF > 0 && maxF - minF > 3) return;
                // dita: una per nota premuta; se servono piu' di 4 dita, il barre' (indice steso sul tasto piu' basso) ne copre piu' d'una,
                // ma allora dalla corda del barre' in su non ci possono essere corde a vuoto
                int pressed = 0, atMin = 0; for (int i = 0; i < kStr; ++i) if (cur[(size_t) i] > 0) { ++pressed; if (cur[(size_t) i] == minF) ++atMin; }
                bool barre = false;
                if (pressed > 4)
                {
                    if (atMin < 2 || pressed - atMin + 1 > 4) return;
                    int firstMin = -1; for (int i = 0; i < kStr; ++i) if (cur[(size_t) i] == minF) { firstMin = i; break; }
                    for (int i = firstMin; i < kStr; ++i) if (cur[(size_t) i] == 0) return;
                    barre = true;
                }
                // note: la piu' bassa deve essere la fondamentale (rivolti solo nel jazz e nelle triadi alte)
                int lowPc = -1; for (int i = 0; i < kStr; ++i) if (cur[(size_t) i] >= 0) { lowPc = (tun[(size_t) i] + cur[(size_t) i]) % 12; break; }
                if (! jazz && ! triad && lowPc != ch.root) return;
                std::array<bool, 12> has {}; for (int i = 0; i < kStr; ++i) if (cur[(size_t) i] >= 0) has[(size_t) ((tun[(size_t) i] + cur[(size_t) i]) % 12)] = true;
                if (! has[(size_t) ch.root] && ! (jazz || triad)) return;
                if (ch.third >= 0 && ! power && ! has[(size_t) ch.third]) return;
                int cover = 0; for (int t : want) cover += has[(size_t) t];
                if (jazz && want.size() >= 4 && cover < 3) return;
                // punteggio
                float sc = (float) cover * 3.0f + (float) n * (triad ? 0.0f : jazz ? 0.3f : 0.55f);
                sc -= (float) (barre ? pressed - atMin + 1 : pressed) * 0.4f;   // meno dita = piu' facile
                if (cover < (int) want.size()) sc -= (float) ((int) want.size() - cover) * (jazz ? 1.0f : 2.5f);
                const float centre = maxF > 0 ? 0.5f * (minF + maxF) : 0.0f;
                { const float d = std::abs (centre - (float) pos); sc -= d * (open ? 0.6f : 0.9f) + juce::jmax (0.0f, d - 3.0f) * juce::jmax (0.0f, d - 3.0f) * 0.35f; }
                int opens = 0; for (int i = 0; i < kStr; ++i) opens += cur[(size_t) i] == 0;
                sc += open ? opens * 1.2f : -opens * (style == V_BARRE ? 1.0f : 0.6f);
                if (style == V_BARRE) sc += barre ? 3.0f : -1.0f;
                if (jazz && lowPc == ch.root) sc += 2.0f;
                if (maxF > 0) { const int sp = maxF - minF; sc -= sp == 3 ? 3.0f : sp == 2 ? 0.6f : 0.0f; }
                if (prev != nullptr && prev->count() > 0)
                {   // condotta delle voci: poco movimento della mano e della nota alta
                    float mv = 0; int k = 0; for (int i = 0; i < kStr; ++i) if (cur[(size_t) i] >= 0 && prev->fret[(size_t) i] >= 0) { mv += (float) std::abs (cur[(size_t) i] - prev->fret[(size_t) i]); ++k; }
                    sc -= (k ? mv / (float) k : 3.0f) * 0.8f;
                    int topA = -1, topB = -1; for (int i = kStr - 1; i >= 0; --i) { if (topA < 0 && cur[(size_t) i] >= 0) topA = tun[(size_t) i] + cur[(size_t) i]; if (topB < 0 && prev->fret[(size_t) i] >= 0) topB = tun[(size_t) i] + prev->fret[(size_t) i]; }
                    if (topA >= 0 && topB >= 0) sc -= (float) std::abs (topA - topB) * 0.25f;
                }
                if (topTarget >= 0)
                {   // la nota alta canta la linea: si premia il voicing che la mette in cima
                    int top = -1; for (int i = kStr - 1; i >= 0; --i) if (cur[(size_t) i] >= 0) { top = tun[(size_t) i] + cur[(size_t) i]; break; }
                    if (top == topTarget) sc += 1.8f; else if (top % 12 == topTarget % 12) sc += 0.5f; else sc -= (float) juce::jmin (6, std::abs (top - topTarget)) * 0.12f;
                }
                if (rndAmt > 0) sc += rnd.nextFloat() * rndAmt;
                if (sc > best.score) { best.fret = cur; best.score = sc; best.lo = minF == 99 ? 0 : minF; best.hi = maxF; best.barre = barre; }
                return;
            }
            for (int f : opt[(size_t) s]) { cur[(size_t) s] = f; rec (s + 1); }
        };
        rec (0);
    }
    return best;
}

// ------------------------------------------------------------------------------------------------ uscita: note e bend
struct Ev
{
    double beat = 0, len = 0.25;    // in battiti (semiminime)
    int note = 60; float vel = 0.7f; int str = 0, fret = 0;
    bool mute = false;              // stoppata (nota morta, cortissima)
    bool pm = false;                // palm mute
    int art = 0;                    // 0 pizzicata, 1 legato (hammer-on / pull-off), 2 scivolata
    bool lead = false; int slot = 0, grp = 0;
    int layer = 0;                  // composizione: 0 base, 1 arpeggio, 2 lead
};
struct Bend { double beat = 0; float semis = 0; int str = 0; };   // curva di pitch bend del lead (canale unico)
struct Part
{
    std::vector<Ev> ev; std::vector<Bend> bend; std::vector<Chord> chords; std::vector<Voicing> voic;
    std::vector<double> chordAt;    // battito d'inizio di ogni accordo
    struct Choke { double beat; };  std::vector<Choke> chokes;
    int bars = 4; double beatsPerBar = 4; double len() const { return bars * beatsPerBar; }
    juce::String error;
};

// ------------------------------------------------------------------------------------------------ impostazioni
enum Mode { ACCORDI, RITMICA, ARPEGGIO, LEAD, COMPOSIZIONE, kModes };
inline const char* modeName (int m) { static const char* n[kModes] = { "ACCORDI", "RITMICA", "ARPEGGIO", "LEAD", "COMPOSIZIONE" }; return n[juce::jlimit (0, kModes - 1, m)]; }
// ------------------------------------------------------------------------------------------------ arpeggio: figura e ritmo
// figura = l'ordine in cui la mano prende le voci dell'accordo. 1 = la voce piu' acuta, 2 la seconda dall'alto, 3, 4; B = basso, A = basso alternato.
// Due cifre insieme = pizzicate insieme. La figura segue qualsiasi voicing e ricomincia a ogni cambio d'accordo.
struct Fig { const char* name; const char* seq; };
constexpr int kFigs = 15;
inline const Fig& figOf (int i)
{
    static const Fig F[kFigs] = {
        { "SALITA", "B 3 2 1" }, { "DISCESA", "1 2 3 B" }, { "SU E GIU'", "B 3 2 1 2 3" }, { "PEDALE ALTO", "B 1 3 1 2 1 3 1" },
        { "1-3-1-B", "1 3 1 B" }, { "1-2-4-1", "1 2 4 1" }, { "SALTI", "B 1 3 2" }, { "TRAVIS", "B 2 A 1" },
        { "ROLL A TRE", "B 2 1" }, { "PINZA", "B1 3 2 3" }, { "DIADI", "B 12 3 12" }, { "CASCATA", "1 2 3 4 B" },
        { "ALTERNATO", "B 1 A 1" }, { "INTRECCIO", "B 2 1 2 A 2 1 3" }, { "GRIGLIA", "" } };
    return F[juce::jlimit (0, kFigs - 1, i)];
}
// ritmo: x nota, X nota accentata, - niente. 16 passi (sedicesimi) o 12 (terzine)
struct Rit { const char* name; const char* steps; };
constexpr int kRits = 10;
inline const Rit& ritOf (int i)
{
    static const Rit R[kRits] = {
        { "OTTAVI", "X-x-x-x-X-x-x-x-" }, { "SEDICESIMI", "Xxxxxxxxxxxxxxxx" }, { "PUNTATO 3+3+2", "X--x--x-X--x--x-" }, { "OTTAVO + 2 SEDIC.", "X-xxx-xxX-xxx-xx" },
        { "TERZINE", "XxxxxxXxxxxx" }, { "SINCOPATO", "X-x--x-xX-x--x-x" }, { "SEMIMINIME", "X---x---X---x---" }, { "LUNGA E CORSA", "X---x---X-xxxxx-" },
        { "TRAP SPARSO", "X--x--x---x-x---" }, { "SHUFFLE", "X-xx-xX-xx-x" } };
    return R[juce::jlimit (0, kRits - 1, i)];
}
// gli stili dell'arpeggio sono combinazioni pronte di figura e ritmo
inline std::pair<int, int> arpStyle (int style)
{
    static const std::pair<int, int> M[12] = { { 2, 1 }, { 6, 0 }, { 7, 0 }, { 0, 1 }, { 2, 4 }, { 3, 4 }, { 11, 5 }, { 13, 5 }, { 12, 1 }, { 11, 1 }, { 3, 1 }, { 10, 0 } };
    return M[juce::jlimit (0, 11, style)];
}

// colori dell'accordo (nel menu di ogni accordo)
constexpr int kColors = 8;
inline const char* colorName (int c) { static const char* n[kColors] = { "TRIADE", "SETTIMA", "NONA", "SUS / ADD9", "SUS2", "SUS4", "ADD9", "SESTA" }; return n[juce::jlimit (0, kColors - 1, c)]; }
struct Settings
{
    int key = 9, scale = MINORE, genre = TRAP, mode = ARPEGGIO, pattern = 0, vstyle = V_OPEN, tuning = STANDARD;
    int bars = 4, perBar = 1;            // battute del giro; accordi per battuta: 1, 2, oppure 0 = un accordo ogni 2 battute
    float density = 0.5f, color = -1.0f; // color < 0 = quello del genere
    int pos = 3;                         // tasto di riferimento per la mano
    float human = 0.35f, swing = 0.0f, strum = 0.35f, dyn = 0.6f, borrow = 0.0f, ring = 0.7f, emb = 0.3f, tech = 0.5f;
    double bpm = 120;
    int mood = 0;                        // 0 = dal genere
    int arpFig = 6, arpRit = 0;              // arpeggio: figura delle corde e ritmo (separati)
    std::array<juce::uint16, 5> grid { 0x4444, 0, 0x1010, 0, 0x0101 };   // figura disegnata: righe 1 (acuta), 2, 3, 4, BASSO; un bit per sedicesimo
    int lyBase = 1, lyArp = 1, lyLead = 0;   // composizione: pattern di ogni strato (-1 spento; base: 0..5 accordi, 100+ ritmica)
    juce::uint32 seedB = 1, seedA = 2, seedL = 3;
    float space = 0.3f;                  // pause: quanto respira la parte
    std::vector<int> prog;               // gradi scelti a mano (vuoto = dal genere)
    juce::uint32 seed = 1;
};
// nomi dei pattern di ogni modo
inline const juce::StringArray& patternNames (int mode)
{
    static const juce::StringArray P[kModes] = {
        { "TENUTI", "COLPI 1 E 3", "STAB R&B", "NEO SOUL", "ROLL LENTO", "SPINTE", "PIZZICATI" },
        { "POP 8", "BALLATA", "REGGAE", "REGGAETON", "FUNK 16", "PALM MUTE", "AFRO", "6/8", "FOLK", "TRAP STRUM", "R&B 16" },
        { "SALITA", "TRAP TRISTE", "TRAVIS", "PIMA", "6/8 EMO", "TERZINE DRILL", "R&B ROLL", "HIGHLIFE", "CLASSICO", "CASCATA", "PEDALE", "DIADI" },
        { "MELODIA", "TRAP SCURO", "DRILL", "R&B RUN", "BLUES", "LATIN" },
        { "-" } };
    return P[juce::jlimit (0, kModes - 1, mode)];
}
}
