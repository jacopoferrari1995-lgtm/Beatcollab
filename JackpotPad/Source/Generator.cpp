#include "Generator.h"
#include "Library.h"
#include "KeyDetect.h"
#include <map>
#include <set>

namespace jp
{
// ======================================================================= ruoli
const char* roleName (int r)
{
    static const char* n[] = { "-", "CASSA", "RULLANTE", "CLAP", "HAT", "OPEN HAT", "PERC", "TOM", "PIATTO", "808", "MELODICO", "LOOP", "VOCE", "FX", "TEXTURE", "CHOP" };
    return n[juce::jlimit (0, kRoles - 1, r)];
}
Role roleOfCategory (const juce::String& c)
{
    if (c == "KICK") return R_KICK;        if (c == "808") return R_808;          if (c == "SNARE") return R_SNARE;
    if (c == "CLAP / SNAP") return R_CLAP; if (c == "HAT") return R_HAT;          if (c == "OPEN HAT") return R_OPEN;
    if (c == "PERC") return R_PERC;        if (c == "TOM") return R_TOM;          if (c == "CYMBAL") return R_CYM;
    if (c == "MELODICI") return R_MEL;     if (c == "TEXTURE") return R_TEX;      if (c == "FX") return R_FX;
    if (c == "VOCI") return R_VOX;         if (c == "LOOP") return R_LOOP;        if (c == "CHOP") return R_CHOP;
    return R_NONE;
}
static Role roleFromName (const juce::String& n0)
{
    const auto n = " " + n0.toLowerCase().replaceCharacters ("_-.()[]", "       ") + " ";
    auto has = [&n] (std::initializer_list<const char*> l) { for (auto* w : l) if (n.contains (w)) return true; return false; };
    if (has ({ "open", " oh ", "apert" })) return R_OPEN;
    if (has ({ "808", " sub", "bass" })) return R_808;
    if (has ({ "kick", " bd ", " kik", "cassa" })) return R_KICK;
    if (has ({ "snare", " snr", " sd ", "rullante" })) return R_SNARE;
    if (has ({ "clap", "snap", " clp" })) return R_CLAP;
    if (has ({ "hat", " hh ", "hihat" })) return R_HAT;
    if (has ({ "crash", "ride", "cymbal", "splash", "piatto" })) return R_CYM;
    if (has ({ " tom" })) return R_TOM;
    if (has ({ "rim", "perc", "conga", "bongo", "shaker", "cowbell", "clave", "tamb", "bell", "wood" })) return R_PERC;
    if (has ({ "vox", "vocal", "voce", "voice", "chant" })) return R_VOX;
    if (has ({ " fx", "riser", "sweep", "impact", "whoosh", "swoosh" })) return R_FX;
    if (has ({ "vinyl", "vinile", "noise", "hiss", "texture", "crackle" })) return R_TEX;
    if (has ({ "loop" })) return R_LOOP;
    if (has ({ "keys", "piano", "chord", "stab", "pad ", "pluck", "synth", "guitar", "chitarra", "rhodes", "organ", "string" })) return R_MEL;
    return R_NONE;
}
Role padRole (const Processor& P, int p)
{
    auto s = P.getSample (p); if (s == nullptr) return R_NONE;
    if (P.pad[p].get (SLICE) > 0.5f) return R_CHOP;
    const auto rc = roleOfCategory (P.padCat[p]); if (rc != R_NONE) return rc;
    const auto rn = roleFromName (s->name); if (rn != R_NONE) return rn;
    const double dur = s->buf.getNumSamples() / s->sr;
    if (P.pad[p].get (SKEY) >= 0) return dur > 2.5 ? R_LOOP : R_MEL;
    return dur > 2.5 ? R_LOOP : R_PERC;
}

// ======================================================================= stili
const char* genCatName (int c) { static const char* n[] = { "DRUM", "MPC", "PROGRESSIONI", "PROGRESSIONI MPC" }; return n[juce::jlimit (0, kGenCats - 1, c)]; }

// ----------------------------------------------------------------------- banca di progressioni (accordi veri, come le Progressions della MPC)
// qualita degli accordi: semitoni sopra la fondamentale (massimo 4 note, voicing compatti)
static const std::vector<int>& qualIv (int q)
{
    static const std::vector<int> Q[] = { { 0, 3, 7 }, { 0, 4, 7 }, { 0, 3, 7, 10 }, { 0, 4, 7, 11 }, { 0, 4, 7, 10 }, { 0, 3, 10, 14 }, { 0, 4, 11, 14 }, { 0, 4, 10, 14 },
                                          { 0, 2, 7 }, { 0, 5, 7 }, { 0, 3, 6 }, { 0, 3, 6, 10 }, { 0, 4, 7, 14 }, { 0, 3, 7, 14 } };
    return Q[juce::jlimit (0, 13, q)];
}
static const char* qualName (int q) { static const char* n[] = { "m", "", "m7", "maj7", "7", "m9", "maj9", "9", "sus2", "sus4", "dim", "m7b5", "add9", "m(add9)" }; return n[juce::jlimit (0, 13, q)]; }
enum { Qm, QM, Qm7, QM7, Q7, Qm9, QM9, Q9, Qs2, Qs4, Qdim, Qhd, QA9, QmA9 };
struct Chd { int root; int q; };
struct ProgDef { const char* name; const char* genre; bool minor; std::vector<Chd> ch; };
const std::vector<ProgDef>& progBank()
{
    static const std::vector<ProgDef> B = {
        { "Trap Minore", "Trap", true, { { 0, Qm }, { 8, QM }, { 3, QM }, { 10, QM } } },
        { "Trap Scura", "Trap", true, { { 0, Qm }, { 5, Qm }, { 8, QM }, { 7, QM } } },
        { "Trap Epica", "Trap", true, { { 0, Qm }, { 10, QM }, { 8, QM }, { 10, QM } } },
        { "Trap Frigia", "Trap", true, { { 0, Qm }, { 1, QM }, { 0, Qm }, { 10, QM } } },
        { "Drill Notte", "Drill", true, { { 0, Qm }, { 8, QM }, { 5, Qm }, { 7, QM } } },
        { "Drill Armonica", "Drill", true, { { 0, Qm }, { 0, Qm }, { 8, QM }, { 7, Q7 } } },
        { "Neo Soul 1", "R&B", false, { { 2, Qm9 }, { 7, Q9 }, { 0, QM9 }, { 9, Qm9 } } },
        { "Neo Soul 2", "R&B", false, { { 0, QM7 }, { 4, Qm7 }, { 9, Qm7 }, { 5, QM7 } } },
        { "R&B Anni 90", "R&B", false, { { 9, Qm9 }, { 2, Qm9 }, { 7, Q7 }, { 0, QM7 } } },
        { "R&B Lento", "R&B", false, { { 5, QM7 }, { 4, Qm7 }, { 2, Qm7 }, { 0, QM9 } } },
        { "Lo-fi Jazz", "Lo-fi", false, { { 2, Qm7 }, { 7, Q7 }, { 0, QM7 }, { 9, Qm7 } } },
        { "Lo-fi Notte", "Lo-fi", true, { { 0, Qm9 }, { 5, Qm7 }, { 10, Q7 }, { 3, QM7 } } },
        { "Lo-fi Pioggia", "Lo-fi", false, { { 5, QM9 }, { 4, Qm7 }, { 9, Qm9 }, { 0, QM7 } } },
        { "Boom Bap Soul", "Boom Bap", true, { { 0, Qm9 }, { 5, Qm9 }, { 0, Qm9 }, { 7, Qm7 } } },
        { "Boom Bap Minore", "Boom Bap", true, { { 0, Qm7 }, { 8, QM7 }, { 5, Qm7 }, { 7, Qm7 } } },
        { "Jazz Turnaround", "Boom Bap", false, { { 0, QM7 }, { 9, Qm7 }, { 2, Qm7 }, { 7, Q7 } } },
        { "Gospel", "R&B", false, { { 5, QM7 }, { 7, Q7 }, { 4, Qm7 }, { 9, Qm7 } } },
        { "Pop Classico", "Pop", false, { { 0, QM }, { 7, QM }, { 9, Qm }, { 5, QM } } },
        { "Pop Malinconico", "Pop", false, { { 9, Qm }, { 5, QM }, { 0, QM }, { 7, QM } } },
        { "Doo-wop", "Pop", false, { { 0, QM }, { 9, Qm }, { 5, QM }, { 7, QM } } },
        { "Andalusa", "Afro / Latin", true, { { 0, Qm }, { 10, QM }, { 8, QM }, { 7, QM } } },
        { "Afro", "Afro / Latin", true, { { 0, Qm7 }, { 5, Qm7 }, { 10, QM }, { 3, QM7 } } },
        { "Reggaeton", "Afro / Latin", true, { { 0, Qm }, { 8, QM }, { 3, QM }, { 10, QM } } },
        { "Sospesa", "Ambient", false, { { 0, Qs2 }, { 5, Qs2 }, { 9, QmA9 }, { 7, Qs4 } } },
        { "Cinematica", "Ambient", true, { { 0, QmA9 }, { 8, QM7 }, { 3, QA9 }, { 10, QA9 } } } };
    return B;
}
static juce::String chordLabel (int keyPc, const Chd& c)
{
    static const char* nn[] = { "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B" };
    return juce::String (nn[(keyPc + c.root) % 12]) + qualName (c.q);
}

namespace
{
// righe da 16 passi: X forte, x normale, o ghost, - debole, . vuoto
struct DStyle
{
    const char* name; const char* desc; const char* bankGenre;
    std::vector<const char*> kick, back, hat, open, perc;
    int backInst;            // 0 rullante, 1 clap, 2 rim/perc
    float layer, ghost;      // probabilita di raddoppio clap+rullante, ghost note
    int m808;                // 0 niente, 1 segue la cassa, 2 linea sua
    std::vector<const char*> b808;
    int hatMode;             // 0 righe, 1 trap (rulli), 2 drill (terzine)
    float roll, swing;       // rulli, swing (-1 = non toccare)
    int bars; float mel; bool vinyl; int late;   // battute, stab melodico, vinile, ritardo del rullante (tick)
};
const std::vector<DStyle>& dstyles()
{
    static const std::vector<DStyle> S = {
        { "Boom Bap", "Anni 90, cassa e rullante pesanti, hat in ottavi swingati", "Boom Bap",
          { "X......x..X.....", "X.........X..x..", "X..x....X.x.....", "X......X..x...x.", "X.x.......X.....", "X......x.xX....." },
          { "....X.......X..." }, { "x.x.x.x.x.x.x.x.", "x.xox.xox.xox.xo", "X.x.X.x.X.x.X.x." }, { "", "..............x.", "......x........." }, { "", "...x......x..x..", "..o...o...o...o." },
          0, 0.0f, 0.35f, 0, {}, 0, 0.0f, 57, 4, 0.25f, false, 0 },
        { "Boom Bap Dusty", "Polveroso, cassa che inciampa, ghost sul rullante", "Boom Bap",
          { "X..X......X.....", "X......x.xX.....", "X.....X...X..x..", "X.......x.x..X.." },
          { "....X.......X..." }, { "x.x.x.x.x.x.x.x.", "x.x.x.x.x.x.xxx." }, { "......o.........", "" }, { "xoxoxoxoxoxoxoxo", "" },
          0, 0.0f, 0.6f, 0, {}, 0, 0.0f, 60, 4, 0.3f, true, 0 },
        { "Jazz Hop", "Morbido, ride e hat swingati, rullante con spazzola", "Lo-fi",
          { "X.........x.....", "X......x...x....", "X.......x.x....." },
          { "....X.......X..." }, { "x..x.xx..x.xx..x", "x.xxx.xxx.xxx.xx" }, { "" }, { "......o.......o.", "" },
          0, 0.0f, 0.7f, 0, {}, 0, 0.0f, 62, 4, 0.5f, true, 2 },
        { "Lo-fi", "Pigro e caldo, vinile sotto, cassa leggera", "Lo-fi",
          { "X.......x.x.....", "X.........X.x...", "X......x..X.....", "X.......X......." },
          { "....X.......X..." }, { "x.x.x.x.x.x.x.x.", "x...x...x...x..." }, { "" }, { "..o...o...o...o.", "", "......x.......x." },
          0, 0.0f, 0.3f, 0, {}, 0, 0.0f, 58, 4, 0.45f, true, 0 },
        { "Trap", "Cassa e 808 insieme, clap a meta battuta, hat con rulli", "Trap",
          { "X......X..X.....", "X.....X...X.....", "X......X.X......", "X..X......X...X.", "X.......X.X....." },
          { "........X......." }, {}, { "..............x.", "", "......x........." }, { "", "............x..." },
          1, 0.25f, 0.0f, 1, {}, 1, 0.7f, -1, 4, 0.15f, false, 0 },
        { "Trap Dark", "Lento e pieno di spazio, 808 lunghi", "Trap",
          { "X..............X", "X.........X.....", "X.............X." },
          { "........X......." }, {}, { "", "......x........." }, { "" },
          1, 0.15f, 0.0f, 2, { "X...............", "X.........X.....", "X.............X." }, 1, 0.45f, -1, 4, 0.1f, false, 0 },
        { "Plugg", "Rimbalzante e melodico, clap secco", "Trap",
          { "X.......X..X....", "X.....X.X.......", "X..X....X......." },
          { "........X......." }, { "x.x.x.x.x.x.x.x." }, { "......x.......x." }, { "" },
          1, 0.0f, 0.0f, 1, {}, 0, 0.35f, -1, 4, 0.4f, false, 0 },
        { "Rage", "Aggressivo, cassa doppia e 808 che spinge", "Trap",
          { "X.X.....X.X.....", "X.X...X...X.X...", "XX......X.X....." },
          { "....X.......X..." }, {}, { "" }, { "" },
          0, 0.5f, 0.0f, 1, {}, 1, 0.8f, -1, 4, 0.0f, false, 0 },
        { "Drill UK", "Rullante spostato, hat a terzine, 808 che scivola", "Drill",
          { "X.........X.....", "X.........X...X.", "X.......X.....X.", "X.........X.X..." },
          { "........X.......", "........X..x...." }, {}, { "" }, { "", "......x........." },
          0, 0.2f, 0.25f, 2, { "X.........X.....", "X.......X.....X.", "X.........X...X." }, 2, 0.5f, -1, 4, 0.1f, false, 0 },
        { "Drill NY", "Piu sporco e pieno, cassa che rimbalza", "Drill",
          { "X.......X.....X.", "X.....X...X.....", "X.......X.X...X." },
          { "........X......." }, {}, { "" }, { "......x.........", "" },
          0, 0.35f, 0.2f, 1, {}, 2, 0.5f, -1, 4, 0.1f, false, 0 },
        { "Jersey Club", "Cassa a cinque colpi, clap e voci tagliate", "Old School",
          { "X..X..X...X.X...", "X..X..X.X...X...", "X..X..X...X.X.X." },
          { "....X.......X...", "....X..X....X..." }, { "x.x.x.x.x.x.x.x." }, { "" }, { "..x...x...x...x.", "" },
          1, 0.0f, 0.0f, 0, {}, 0, 0.2f, -1, 2, 0.1f, false, 0 },
        { "Phonk", "Cowbell, cassa piena e hat fitti", "Phonk",
          { "X.....X...X.....", "X..x..X...X..x..", "X.......X.X....." },
          { "....X.......X..." }, { "x.x.x.x.x.x.x.x.", "xxx.xxx.xxx.xxx." }, { "......x........." }, { "X..X......X.....", "X.x...X.x.x....." },
          0, 0.3f, 0.0f, 1, {}, 0, 0.2f, 52, 4, 0.0f, false, 0 },
        { "Memphis", "Anni 90, hat in sedicesimi, cowbell", "Phonk",
          { "X..x..X...X..x..", "X.....X...X..X.." },
          { "....X.......X..." }, { "X.x.X.x.X.x.X.x.", "XxxxXxxxXxxxXxxx" }, { "" }, { "x.x...x.x...x.x.", "..x...x...x...x." },
          1, 0.0f, 0.0f, 1, {}, 0, 0.0f, 54, 4, 0.0f, true, 0 },
        { "Electro", "808 old school, hat in sedicesimi, cowbell", "Old School",
          { "X.....X...X.....", "X..X..X...X..X..", "X.....X...X..X.." },
          { "....X.......X..." }, { "XxxxXxxxXxxxXxxx", "x.x.x.x.x.x.x.x." }, { "..x...x...x...x.", "" }, { "..x.....x.x.....", "" },
          1, 0.0f, 0.0f, 0, {}, 0, 0.0f, -1, 4, 0.0f, false, 0 },
        { "Miami Bass", "Cassa veloce e 808 lungo", "Old School",
          { "X..X..X...X..X..", "X..X..X.....X..." },
          { "....X.......X..." }, { "x.x.x.x.x.x.x.x." }, { "..x...x...x...x." }, { "x..x..x...x.x...", "" },
          0, 0.0f, 0.0f, 2, { "X.......X.......", "X.....X.X......." }, 0, 0.0f, -1, 4, 0.0f, false, 0 },
        { "R&B", "Morbido, snap e shaker, sub sotto", "R&B",
          { "X.......x.x.....", "X......x..X.....", "X.........X....." },
          { "....X.......X..." }, { "x.x.x.x.x.x.x.x.", "x.xox.xox.xox.xo" }, { "" }, { "xoxoxoxoxoxoxoxo", "....X..x....X...", "" },
          1, 0.0f, 0.2f, 2, { "X.......x.x.....", "X.........X....." }, 0, 0.15f, 54, 4, 0.5f, false, 0 },
        { "Neo Soul", "Rullante in ritardo, tutto un po' fuori griglia", "R&B",
          { "X.........x..x..", "X......x...x....", "X.......x.....x." },
          { "....X.......X..." }, { "x.xox.xox.xox.xo", "x.x.x.x.x.x.x.x." }, { "" }, { "......o.......o.", "" },
          0, 0.0f, 0.5f, 2, { "X.........x.....", "X......x........" }, 0, 0.0f, 63, 4, 0.55f, false, 6 },
        { "Afrobeats", "Cassa morbida, rim a 3-3-2, shaker continuo", "R&B",
          { "X.......X.......", "X.....X.X.......", "X..x....X......." },
          { "....X.......X..." }, { "xoxoxoxoxoxoxoxo", "x.x.x.x.x.x.x.x." }, { "" }, { "...x..x....x..x.", "x.x...x.x.x...x." },
          1, 0.0f, 0.0f, 0, {}, 0, 0.0f, 54, 4, 0.3f, false, 0 },
        { "Amapiano", "Cassa dritta leggera, log drum sul 808, shaker", "R&B",
          { "X...X...X...X...", "X.......X.......", "X...X...X...X..." },
          { "....X.......X..." }, { "xoxoxoxoxoxoxoxo" }, { "" }, { "x..x..x...x..x..", "" },
          1, 0.0f, 0.0f, 2, { "..X...X.X.....X.", "X..X..X...X...X.", "..X.X...X.X....." }, 0, 0.0f, 55, 4, 0.3f, false, 0 },
        { "Reggaeton", "Dembow: cassa in quarti e rullante 3-3-2", "Old School",
          { "X...X...X...X..." }, { "...X..X....X..X." }, { "x.x.x.x.x.x.x.x." }, { "" }, { "", "x.x.x.x.x.x.x.x." },
          0, 0.3f, 0.0f, 0, {}, 0, 0.0f, -1, 4, 0.1f, false, 0 },
        { "Dancehall", "Cassa che salta, rim in levare", "Old School",
          { "X..X..X.X..X..X.", "X..x..X...X..X.." }, { "....X.......X..." }, { "x.x.x.x.x.x.x.x." }, { "" }, { "..x...x...x...x.", "...x..x....x..x." },
          0, 0.0f, 0.0f, 0, {}, 0, 0.0f, -1, 4, 0.1f, false, 0 },
        { "UK Garage", "2-step: niente cassa sul 2 e 4, hat swingati", "Old School",
          { "X.........X.....", "X.......X.X.....", "X..........X.X.." }, { "....X.......X..." }, { "..x...x...x...x.", "x.xxx.xxx.xxx.xx" }, { "..x...x...x...x.", "" }, { "...x..x....x..x.", "" },
          1, 0.0f, 0.2f, 0, {}, 0, 0.0f, 64, 4, 0.2f, false, 0 },
        { "House", "Cassa dritta, clap sul 2 e 4, open hat in levare", "Old School",
          { "X...X...X...X..." }, { "....X.......X..." }, { "x.x.x.x.x.x.x.x.", "xxxxxxxxxxxxxxxx" }, { "..x...x...x...x." }, { "", "...x..x....x..x." },
          1, 0.0f, 0.0f, 0, {}, 0, 0.0f, -1, 4, 0.15f, false, 0 },
        { "Boom Bap Premier", "Cassa doppia secca, rullante che schiocca, spazio per gli scratch", "Boom Bap",
          { "X.x.......X.....", "X.x....x..X.....", "X.x.......X..x.." }, { "....X.......X..." }, { "x.x.x.x.x.x.x.x." }, { "", "......x........." }, { "", "..........o....." },
          0, 0.0f, 0.3f, 0, {}, 0, 0.0f, 56, 4, 0.2f, true, 0 },
        { "Golden Era", "Hat swingati con ghost, cassa morbida, shaker sotto", "Boom Bap",
          { "X......x..x.....", "X.........xx....", "X......x.x......" }, { "....X.......X..." }, { "x.xox.xox.xox.xo", "x.x.x.x.x.x.x.x." }, { "..............x." }, { "xoxoxoxoxoxoxoxo", "" },
          0, 0.0f, 0.45f, 0, {}, 0, 0.0f, 59, 4, 0.35f, true, 0 },
        { "Dilla Swing", "Cassa ubriaca, rullante in ritardo, swing alto", "Lo-fi",
          { "X.....x...x..x..", "X.......x..x.x..", "X..x......x....." }, { "....X.......X..." }, { "x.x.x.x.x.x.x.x.", "x..xx..xx..xx..x" }, { "" }, { "", "......o.......o." },
          0, 0.0f, 0.5f, 0, {}, 0, 0.0f, 66, 4, 0.3f, true, 8 },
        { "Lo-fi Bedroom", "Cassa sottile, rim e hat chiusi, swing morbido", "Lo-fi",
          { "X.......x.......", "X.....x...x....." }, { "....X.......X..." }, { "x.x.x.x.x.x.x.x.", "x..x..x.x..x..x." }, { "" }, { "..o...o...o...o.", "" },
          2, 0.0f, 0.4f, 0, {}, 0, 0.0f, 60, 4, 0.5f, true, 3 },
        { "G-Funk", "West coast anni 90: clap, hat in sedicesimi, basso che rimbalza", "Old School",
          { "X.....X...X.....", "X.....X.X.......", "X..x..X...X....." }, { "....X.......X..." }, { "xxxxxxxxxxxxxxxx", "x.xxx.xxx.xxx.xx" }, { "......x.......x." }, { "" },
          1, 0.4f, 0.0f, 2, { "X.....X...X.x...", "X..X..X...X....." }, 0, 0.0f, 56, 4, 0.4f, false, 0 },
        { "Crunk", "Dirty South: cassa pesante, clap doppi, 808 lungo", "Trap",
          { "X.......X.X.....", "X..X....X.......", "X.....X.X......." }, { "....X.......X...", "....X..X....X..." }, { "x.x.x.x.x.x.x.x." }, { "" }, { "..x...x...x...x." },
          1, 0.5f, 0.0f, 2, { "X.......X.X.....", "X..X....X......." }, 0, 0.1f, -1, 4, 0.1f, false, 0 },
        { "Trap Bounce", "Rimbalzo stile Atlanta, cassa sincopata, hat con rulli", "Trap",
          { "X..X......X..X..", "X.....X..X......", "X..X.....XX....." }, { "........X......." }, {}, { "" }, { "", "......x.....x..." },
          1, 0.3f, 0.0f, 1, {}, 1, 0.6f, -1, 4, 0.15f, false, 0 },
        { "Trap Terzine", "Hat a terzine che corrono, clap pieno", "Trap",
          { "X.........X.....", "X......X..X.....", "X.........X..X.." }, { "........X......." }, {}, { "" }, { "" },
          1, 0.2f, 0.0f, 1, {}, 2, 0.6f, -1, 4, 0.1f, false, 0 },
        { "Detroit", "Hat velocissimi e cassa che corre, clap secco", "Trap",
          { "X.X...X.X.X.....", "X..X.X..X.X.....", "X.X.....X..X.X.." }, { "....X.......X..." }, {}, { "" }, { "" },
          1, 0.0f, 0.0f, 1, {}, 1, 0.85f, -1, 4, 0.05f, false, 0 },
        { "Cloud", "Lento e arioso, rullante lontano e tanto spazio", "Trap",
          { "X...............", "X.........X....." }, { "........X......." }, { "x...x...x...x...", "x.x.x.x.x.x.x.x." }, { "..............x." }, { "" },
          0, 0.0f, 0.0f, 2, { "X...............", "X.........X....." }, 0, 0.2f, -1, 4, 0.6f, false, 0 },
        { "Pluggnb", "Plugg morbido e melodico, snap e 808 cantato", "R&B",
          { "X.......X.......", "X.....X.X......." }, { "........X......." }, { "x.x.x.x.x.x.x.x.", "x...x...x...x.x." }, { "......x........." }, { "" },
          1, 0.0f, 0.0f, 2, { "X.......X.....X.", "X.....X.X......." }, 0, 0.25f, -1, 4, 0.6f, false, 0 },
        { "Hyperpop", "Veloce, cassa che martella, rullante squillante", "Trap",
          { "X.X...X.X.X...X.", "X.X.X...X.X.X..." }, { "....X.......X..." }, { "xxxxxxxxxxxxxxxx" }, { "" }, { "" },
          1, 0.6f, 0.0f, 1, {}, 0, 0.3f, -1, 2, 0.2f, false, 0 },
        { "Afro Drill", "Drill con percussioni afro e shaker", "Drill",
          { "X.........X.....", "X.......X.....X." }, { "........X......." }, {}, { "" }, { "..x..x....x..x..", "x..x..x...x..x.." },
          0, 0.2f, 0.2f, 2, { "X.........X.....", "X.......X.....X." }, 2, 0.4f, -1, 4, 0.1f, false, 0 },
        { "Baile Funk", "Tamborzao brasiliano: cassa e conga che rimbalzano", "Old School",
          { "X..X..X...X..X..", "X..X..X.X.X....." }, { "....X.......X...", "...X..X....X..X." }, { "x.x.x.x.x.x.x.x." }, { "" }, { "..X..X....X..X..", "x..x..x.x..x..x." },
          1, 0.0f, 0.0f, 0, {}, 0, 0.0f, -1, 2, 0.1f, false, 0 },
        { "Phonk Brasiliano", "Funk e phonk: cowbell, cassa dura, tutto in faccia", "Phonk",
          { "X..X..X...X..X..", "X.X...X.X.X....." }, { "....X.......X..." }, { "x.x.x.x.x.x.x.x." }, { "" }, { "X..X..X...X..X..", "X.X.X..XX.X.X..." },
          1, 0.3f, 0.0f, 1, {}, 0, 0.1f, -1, 2, 0.0f, false, 0 },
        { "Breakbeat", "Spezzato anni 90: cassa e rullante che si inseguono", "Old School",
          { "X.X.......X.....", "X.X.......XX....", "X.........X..X.." }, { "....X..o.o..X..o", "....X.......X..." }, { "x.x.x.x.x.x.x.x." }, { "..............x." }, { "" },
          0, 0.0f, 0.5f, 0, {}, 0, 0.0f, 54, 4, 0.1f, true, 0 },
        { "Drum & Bass", "Due colpi: cassa sull'1 e sul 3 in levare, rullante sul 2 e 4", "Old School",
          { "X.........X.....", "X.....X...X....." }, { "....X.......X...", "....X..x....X..." }, { "x.x.x.x.x.x.x.x.", "xxxxxxxxxxxxxxxx" }, { "..x...x...x...x.", "" }, { "" },
          0, 0.0f, 0.3f, 0, {}, 0, 0.05f, -1, 4, 0.1f, false, 0 },
        { "Footwork", "Chicago: cassa a terzine e clap fitti", "Old School",
          { "X..X..X..X..X...", "X..X..X...X..X.." }, { "....X.......X...", "..X...X...X...X." }, { "x.x.x.x.x.x.x.x." }, { "" }, { "" },
          1, 0.0f, 0.0f, 2, { "X..X..X..X..X..." }, 0, 0.0f, -1, 2, 0.1f, false, 0 },
        { "Trap Half Time", "Meta tempo: cassa che rimbalza prima del 3, clap pieno sul 3, hat in ottavi con rulli, 808 lunghi", "Trap",
          { "X.........X.....", "X......X..X.....", "X.........X...X.", "X..X......X.....", "X.....X...X..X.." }, { "........X......." },
          { "x.x.x.x.x.x.x.x.", "x.x.x.x.x.xxx.x.", "x...x.x.x...x.x." }, { "..............x.", "", "......x........." }, { "", "...........x....", "...x............" },
          1, 0.35f, 0.0f, 2, { "X.........X.....", "X......X..X.....", "X.........X...X.", "X.....X...X..X.." }, 1, 0.45f, -1, 4, 0.2f, false, 0 },
        { "Half Time Boom Bap", "Boom bap a meta tempo: rullante pesante sul 3, cassa con ghost, hat swingati", "Boom Bap",
          { "X......x..x.....", "X.......xX......", "X..x......x..x..", "X......X.x......" }, { "........X.......", "........X.....o." },
          { "x.x.x.x.x.x.x.x.", "x.xox.xox.xox.xo" }, { "..............x.", "" }, { "", "....o.......o..." },
          0, 0.2f, 0.45f, 0, {}, 0, 0.0f, 58, 4, 0.3f, true, 0 },
        { "Half Time Lo-fi", "Meta tempo lento e caldo: cassa morbida, rim sul 3, vinile", "Lo-fi",
          { "X.........x.....", "X.......x.....x.", "X.....x...x....." }, { "........X......." }, { "x..x.x..x..x.x..", "x.x.x.x.x.x.x.x." }, { "" }, { "....o.......o...", "" },
          2, 0.0f, 0.5f, 0, {}, 0, 0.0f, 60, 4, 0.5f, true, 3 },
        { "Half Time R&B", "Meta tempo morbido: snap sul 3 in ritardo, cassa sincopata, sub che segue", "R&B",
          { "X.........x..x..", "X......x...x....", "X.........X....x" }, { "........X.......", "........X......o" }, { "x.x.x.x.x.x.x.x.", "x.xox.xox.xox.xo" }, { "" }, { "xoxoxoxoxoxoxoxo", "" },
          1, 0.0f, 0.3f, 2, { "X.........x..x..", "X......x...x...." }, 0, 0.0f, 56, 4, 0.5f, false, 5 },
        { "Half Time Drill", "Drill lenta: rullante sul 3 e nel levare, hat a terzine radi, 808 che scivola", "Drill",
          { "X.........X.....", "X.......X.....X.", "X..........X...." }, { "........X.......", "........X..x...." }, {}, { "" }, { "" },
          0, 0.25f, 0.2f, 2, { "X.........X.....", "X.......X.....X.", "X..........X..X." }, 2, 0.35f, -1, 4, 0.15f, false, 0 },
        { "Trap Terzine Lente", "Hat e rullante a terzine su un groove lento", "Trap",
          { "X.........X.....", "X..........X...." }, { "........X......." }, {}, { "" }, { "" },
          1, 0.0f, 0.0f, 2, { "X.........X.....", "X.....X...X....." }, 2, 0.75f, -1, 4, 0.15f, false, 0 },
        { "Rap Vuoto", "Minimale: cassa, rullante e quasi niente altro, spazio per la voce", "Boom Bap",
          { "X.........X.....", "X.......X.......", "X..........X...." }, { "....X.......X..." }, { "x.......x.......", "........x......." }, { "" }, { "" },
          0, 0.0f, 0.15f, 0, {}, 0, 0.0f, 55, 4, 0.15f, false, 0 },
        { "Sperimentale", "Colpi spostati e silenzi: niente e' dove te lo aspetti", "Lo-fi",
          { "X....X.....X..x.", "X......X.X......", "X..x.....X....x." }, { "......X.....X...", "....X.....X....X", ".....X......X..." }, { "x..x....x.x.....", "....x..x....x..x" }, { "...........x....", "" }, { "..x.......x..x..", ".x....x......x.." },
          2, 0.0f, 0.2f, 0, {}, 0, 0.0f, 58, 4, 0.35f, true, 4 },
        { "Jazz Complesso", "Ride swing, cassa che accompagna, rullante che comp a ghost", "Lo-fi",
          { "X..x...x.x....x.", "X.....x...x.x...", "X..x.x.....x..x." }, { "....X.......X...", "..o.X..o.o..X.o." }, { "x..x.xx..x.xx..x", "x.xxx.xxx.xxx.xx", "x..xx..xx..xx..x" }, { "" }, { "..o..o.o..o..o.o", "" },
          0, 0.0f, 0.85f, 0, {}, 0, 0.0f, 66, 4, 0.6f, true, 4 },
        { "Amapiano Deep", "Log drum che parla, cassa leggera, shaker e percussioni in levare", "R&B",
          { "X.......X.......", "X...X...X...X..." }, { "....X.......X...", "........X......." }, { "xoxoxoxoxoxoxoxo", "x.xxx.xxx.xxx.xx" }, { "..x...x...x...x." }, { "...x..x...x..x..", "x..x..x...x..x.." },
          1, 0.0f, 0.1f, 2, { "..X..X..X.X...X.", "X..X..X.X...X.X.", "..X.X.X...X..X.." }, 0, 0.0f, 56, 4, 0.35f, false, 0 },
        { "Riddim", "140 a meta tempo: cassa sull'1, rullante sul 3, basso che martella in terzine", "Trap",
          { "X...............", "X.........X....." }, { "........X......." }, { "x.x.x.x.x.x.x.x.", "x...x...x...x..." }, { "" }, { "" },
          0, 0.3f, 0.0f, 2, { "X..X..X..X..X...", "X.X.X.X.X...X..." }, 0, 0.1f, -1, 4, 0.0f, false, 0 },
        { "Jersey Club Bounce", "Cassa a cinque colpi che rimbalza, clap fitti, tutto a tempo", "Old School",
          { "X..X..X.X..X..X.", "X..X..X...X..X.X", "X.X..X..X..X..X." }, { "....X..X....X..X", "....X.......X..X" }, { "x.x.x.x.x.x.x.x." }, { "" }, { "..x...x...x...x." },
          1, 0.2f, 0.0f, 0, {}, 0, 0.2f, -1, 2, 0.15f, false, 0 },
        { "Indie", "Batteria suonata: cassa e rullante dritti, hat in ottavi, aperto a fine giro", "Old School",
          { "X.......X.X.....", "X.....X.X.......", "X.......X...X..." }, { "....X.......X...", "....X.......X..x" }, { "x.x.x.x.x.x.x.x.", "X.x.X.x.X.x.X.x." }, { "..............x.", "......x.......x." }, { "" },
          0, 0.0f, 0.25f, 0, {}, 0, 0.0f, -1, 4, 0.2f, false, 0 },
        { "Indie Dream", "Lento e sognante: cassa morbida, rim, ride lunghi", "Lo-fi",
          { "X.........X.....", "X.......X......." }, { "....X.......X..." }, { "x...x...x...x...", "x.x.x.x.x.x.x.x." }, { "........x.......", "" }, { "" },
          2, 0.0f, 0.2f, 0, {}, 0, 0.0f, 54, 4, 0.5f, true, 2 },
        { "Trap Latina", "Dembow e trap insieme: rullante 3-3-2, hat con rulli", "Trap",
          { "X...X...X...X...", "X......X..X....." }, { "...X..X....X..X." }, {}, { "" }, { "", "..x...x...x...x." },
          1, 0.2f, 0.0f, 1, {}, 1, 0.5f, -1, 4, 0.15f, false, 0 },
        { "Bossa Hop", "Bossa nova sotto il rap: rim in clave, cassa morbida, shaker", "Lo-fi",
          { "X..X....X..X....", "X......XX......." }, { "X..X..X...X..X..", "..X..X...X..X..." }, { "xoxoxoxoxoxoxoxo" }, { "" }, { "" },
          2, 0.0f, 0.0f, 0, {}, 0, 0.0f, 58, 4, 0.5f, false, 0 },
        { "Drill Terzine", "Drill con rullanti e hat a terzine, 808 che scivola", "Drill",
          { "X.........X.....", "X.......X.....X." }, { "........X.......", "........X..x...." }, {}, { "" }, { "" },
          0, 0.2f, 0.3f, 2, { "X.........X.....", "X.......X.X....." }, 2, 0.8f, -1, 4, 0.1f, false, 0 } };
    return S;
}
struct MStyle { const char* name; const char* desc; int drum; };
const std::vector<MStyle>& mstyles()
{
    static const std::vector<MStyle> M = {
        { "Flip classico", "Chop a quarti in un ordine nuovo, cassa e rullante boom bap", 0 },
        { "Dilla", "Chop fuori griglia, batteria ubriaca e swing alto", 2 },
        { "Stab", "Lo stesso pezzetto ripetuto a colpi corti", 1 },
        { "Riff AAAB", "Tre battute uguali e la quarta che risponde", 0 },
        { "Lo-fi chop", "Chop lunghi a meta battuta, vinile sotto", 3 },
        { "Chop trap", "Chop lenti sopra batteria trap", 4 } };
    return M;
}
struct PStyle { const char* name; const char* desc; };
juce::String progMpc (Processor& P, int bankIdx, juce::uint32 seed, int sel);
const std::vector<PStyle>& pstyles()
{
    static const std::vector<PStyle> Pp = {
        { "Soul lento", "Un accordo per battuta, ripreso sul levare del 3" },
        { "Trap scura", "Accordi lunghi, uno per battuta" },
        { "Drill", "Accordi sul ritmo drill (3-3-2)" },
        { "Lo-fi jazz", "Accordi in levare, swing" },
        { "R&B", "Il cambio arriva in anticipo, prima della battuta" },
        { "Boom bap stab", "Colpi corti sincopati" },
        { "Ballad", "Due accordi per battuta" },
        { "Riff (nota singola)", "Per i suoni a nota singola: un giro di note nella scala" } };
    return Pp;
}

juce::int64 mix (juce::uint32 s)
{   // i seed vicini (1, 2, 3...) danno sequenze casuali molto diverse
    juce::uint64 z = (juce::uint64) s + 0x9E3779B97F4A7C15ull;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull; z = (z ^ (z >> 27)) * 0x94D049BB133111EBull; z ^= z >> 31;
    return (juce::int64) (z & 0x7fffffffffffull);
}
int velOf (char c) { return c == 'X' ? 118 : c == 'x' ? 96 : c == 'o' ? 62 : c == '-' ? 48 : 0; }

struct Ctx
{
    Processor& P; juce::Random r; std::vector<PatEv> ev; int bars = 4;
    std::vector<int> pads[kRoles];
    Ctx (Processor& p, juce::uint32 seed, int maxPad = kPads) : P (p), r (mix (seed))
    {
        for (int k = 0; k < maxPad; ++k) { const int ro = padRole (p, k); if (ro != R_NONE) pads[ro].push_back (k); }
    }
    int pad (Role ro, int i = 0) const { return (int) pads[ro].size() > i ? pads[ro][(size_t) i] : -1; }
    void hit (int p, double tick, int vel)
    {
        if (p < 0 || vel <= 0) return;
        vel = juce::jlimit (20, 127, vel + r.nextInt ({ -5, 6 }));
        ev.push_back ({ juce::jmax (0.0, tick), p, vel });
    }
    void lane (int p, const char* row, int bar, float vs = 1.0f, double shift = 0)
    {
        if (p < 0 || row == nullptr) return; const int n = (int) std::strlen (row);
        for (int i = 0; i < juce::jmin (16, n); ++i) if (const int v = velOf (row[i])) hit (p, bar * kBar + i * 24.0 + shift, (int) (v * vs));
    }
    template <class T> const T& pick (const std::vector<T>& v) { return v[(size_t) r.nextInt ((int) v.size())]; }
    void finish()
    {   // un solo colpo per pad e per istante (tiene il piu forte)
        std::map<std::pair<int, long long>, int> best;
        for (auto& e : ev) { auto key = std::make_pair (e.pad, (long long) std::llround (e.tick * 10)); auto it = best.find (key); if (it == best.end() || it->second < e.vel) best[key] = e.vel; }
        ev.clear(); for (auto& [k, v] : best) ev.push_back ({ k.second / 10.0, k.first, v });
        std::sort (ev.begin(), ev.end(), [] (const PatEv& a, const PatEv& b) { return a.tick < b.tick; });
    }
};

// cassa: variazione di una riga (toglie o aggiunge un colpo)
juce::String mutate (juce::Random& r, const juce::String& row, float amt)
{
    juce::String s = row.paddedRight ('.', 16);
    for (int i = 1; i < 16; ++i)
    {
        if (s[i] != '.' && r.nextFloat() < amt * 0.5f) s = s.replaceSection (i, 1, ".");
        else if (s[i] == '.' && i % 2 == 0 && r.nextFloat() < amt * 0.25f) s = s.replaceSection (i, 1, "x");
    }
    return s;
}

void drums (Ctx& c, const DStyle& S, bool allowMel = true)
{
    auto& r = c.r;
    const int kick = c.pad (R_KICK) >= 0 ? c.pad (R_KICK) : c.pad (R_808);
    const bool kickIs808 = c.pad (R_KICK) < 0;
    // il colpo sul 2 e 4: uno strumento principale, raddoppio solo se lo stile lo vuole
    int back = -1, layerPad = -1;
    {
        const int sn = c.pad (R_SNARE), cl = c.pad (R_CLAP), rim = c.pad (R_PERC);
        if (S.backInst == 1) back = cl >= 0 ? cl : sn; else if (S.backInst == 2) back = rim >= 0 ? rim : sn; else back = sn >= 0 ? sn : cl;
        if (back < 0) back = rim;
        if (r.nextFloat() < S.layer && sn >= 0 && cl >= 0) layerPad = back == sn ? cl : sn;
    }
    const int hat = c.pad (R_HAT) >= 0 ? c.pad (R_HAT) : (S.hatMode == 0 ? -1 : -1);
    const int open = c.pad (R_OPEN), perc = c.pad (R_PERC) != back ? c.pad (R_PERC) : c.pad (R_PERC, 1), perc2 = c.pad (R_PERC, 1) != back ? c.pad (R_PERC, 1) : -1;
    const int p808 = (! kickIs808 && S.m808 > 0) ? c.pad (R_808) : -1;
    // righe scelte una volta: il pattern resta riconoscibile, le battute 2 e 4 variano
    const juce::String kA = c.pick (S.kick), kB = r.nextBool() ? juce::String (c.pick (S.kick)) : mutate (r, kA, 0.5f);
    const char* bk = c.pick (S.back);
    const char* ht = S.hat.empty() ? nullptr : c.pick (S.hat);
    const char* op = S.open.empty() ? nullptr : c.pick (S.open);
    const char* pc = S.perc.empty() ? nullptr : c.pick (S.perc);
    const char* pc2 = S.perc.size() > 1 && perc2 >= 0 && r.nextBool() ? c.pick (S.perc) : nullptr;
    const char* o8 = S.b808.empty() ? nullptr : c.pick (S.b808);
    const bool fill = r.nextFloat() < 0.75f;
    int fillKind = r.nextInt (12);
    if (S.hatMode == 1 && r.nextFloat() < 0.5f) { static const int tf[] = { 2, 4, 5, 11, 8 }; fillKind = tf[r.nextInt (5)]; }        // trap: rulli, terzine, stacchi
    if (S.hatMode == 2 && r.nextFloat() < 0.5f) { static const int df[] = { 4, 5, 6 }; fillKind = df[r.nextInt (3)]; }               // drill
    if (S.swing > 0 && r.nextFloat() < 0.5f) { static const int bf[] = { 0, 1, 3, 9, 10 }; fillKind = bf[r.nextInt (5)]; }          // boom bap, jazz, lo-fi
    for (int b = 0; b < c.bars; ++b)
    {
        const bool last = b == c.bars - 1 && fill;
        const juce::String kr = (b % 2 == 0) ? kA : kB;
        c.lane (kick, kr.toRawUTF8(), b);
        // 808: segue la cassa o ha la sua linea; mai insieme alla cassa se la cassa e gia l'808
        if (p808 >= 0)
        {
            if (S.m808 == 1) { for (int i = 0; i < 16; ++i) if (kr[i] != '.' && r.nextFloat() < 0.88f) c.hit (p808, b * kBar + i * 24.0, 112); if (r.nextFloat() < 0.4f) c.hit (p808, b * kBar + (r.nextBool() ? 14 : 7) * 24.0, 100); }
            else if (o8 != nullptr) c.lane (p808, (b % 2 == 1 && r.nextBool()) ? c.pick (S.b808) : o8, b, 0.95f);
        }
        c.lane (back, bk, b, 1.0f, S.late);
        if (layerPad >= 0) c.lane (layerPad, bk, b, 0.8f, S.late);
        // ghost note sul rullante
        const int sn = c.pad (R_SNARE);
        if (sn >= 0 && S.ghost > 0) for (int i : { 3, 7, 9, 10, 14, 15 }) if (r.nextFloat() < S.ghost * 0.35f) c.hit (sn, b * kBar + i * 24.0, 52);
        // hat
        if (hat >= 0)
        {
            if (S.hatMode == 0 && ht != nullptr) c.lane (hat, ht, b, 0.85f);
            else if (S.hatMode == 1)
            {   // trap: base in ottavi o sedicesimi, con rulli in terzine o trentaduesimi
                const bool sixteen = r.nextFloat() < 0.45f;
                std::set<int> rolled; for (int q = 0; q < 4; ++q) if (r.nextFloat() < S.roll * 0.35f) rolled.insert (q * 4 + 2 * r.nextInt (2));
                for (int i = 0; i < 16; ++i)
                {
                    if (rolled.count (i)) { const bool trip = r.nextBool(); const int nn = trip ? 3 : 4; const double sp = trip ? 16.0 : 12.0; for (int q = 0; q < nn; ++q) c.hit (hat, b * kBar + i * 24.0 + q * sp, 60 + q * 14); i += 1; continue; }
                    if (sixteen || i % 2 == 0) c.hit (hat, b * kBar + i * 24.0, i % 4 == 0 ? 100 : 78);
                }
            }
            else
            {   // drill: hat che saltano, con terzine
                static const char* dr[] = { "x..x..x.x..x..x.", "x..x..x.x..x.x.x", "x..x..x.x..x..xx" };
                c.lane (hat, dr[r.nextInt (3)], b, 0.85f);
                if (r.nextFloat() < S.roll * 0.5f) { const int st = 12 + r.nextInt (3); for (int q = 0; q < 3; ++q) c.hit (hat, b * kBar + st * 24.0 + q * 16.0, 64 + q * 12); }
            }
        }
        if (open >= 0 && op != nullptr) c.lane (open, op, b, 0.85f);
        if (perc >= 0 && pc != nullptr) c.lane (perc, pc, b, 0.8f);
        if (pc2 != nullptr) c.lane (perc2, pc2, b, 0.75f);
        // riempimento nell'ultima battuta: tanti tipi diversi, scelti in base allo stile
        if (last)
        {
            const double bar0 = b * kBar, t0 = bar0 + 12 * 24.0, t8 = bar0 + 8 * 24.0, bar1 = bar0 + kBar;
            const int tom = c.pad (R_TOM), cl = c.pad (R_CLAP);
            auto clear = [&] (double from, std::initializer_list<int> keep)
            {
                c.ev.erase (std::remove_if (c.ev.begin(), c.ev.end(), [&] (const PatEv& e)
                {
                    if (e.tick < from || e.tick >= bar1) return false;
                    for (int k : keep) if (e.pad == k) return false;
                    return true;
                }), c.ev.end());
            };
            switch (fillKind)
            {
                case 0: if (sn >= 0) for (int q = 0; q < 4; ++q) c.hit (sn, t0 + q * 24.0, 70 + q * 14); break;                                // rullante in sedicesimi, crescendo
                case 1: if (tom >= 0) { for (int q = 0; q < 4; ++q) c.hit (c.pad (R_TOM, q % juce::jmax (1, (int) c.pads[R_TOM].size())), t0 + q * 24.0, 100); break; } [[fallthrough]];
                case 2: if (hat >= 0) for (int q = 0; q < 8; ++q) c.hit (hat, t0 + q * 12.0, 56 + q * 8); break;                                // rullata di hat in trentaduesimi
                case 3: if (kick >= 0) { c.hit (kick, t0 + 2 * 24.0, 100); c.hit (kick, t0 + 3 * 24.0, 90); } break;                          // cassa che rientra
                case 4: if (sn >= 0) for (int q = 0; q < 6; ++q) c.hit (sn, t8 + q * 32.0, 64 + q * 10); break;                                 // terzine di rullante sugli ultimi due battiti
                case 5: clear (t0, { back }); break;                                                                                              // stacco: silenzio sull'ultimo battito
                case 6: if (sn >= 0)                                                                                                              // rullata che accelera
                        { for (int q = 0; q < 2; ++q) c.hit (sn, t8 + q * 48.0, 72); for (int q = 0; q < 4; ++q) c.hit (sn, t0 - 48 + q * 24.0, 80 + q * 6); for (int q = 0; q < 4; ++q) c.hit (sn, t0 + 48 + q * 12.0, 96 + q * 6); }
                        break;
                case 7: if (open >= 0) c.hit (open, t0 + 2 * 24.0, 110); if (c.pad (R_CYM) >= 0) c.hit (c.pad (R_CYM), t0 + 2 * 24.0, 100); break;     // accento aperto sul levare del 4
                case 8: { const int f = cl >= 0 ? cl : sn; if (f >= 0) for (int q : { 1, 2, 3 }) { c.hit (f, t0 + q * 24.0, 90); c.hit (f, t0 + q * 24.0 + 6, 70); } } break;   // flam di clap
                case 9: clear (t8, { -1 }); if (kick >= 0 && sn >= 0) for (int q : { 0, 3, 6, 10, 12 }) { c.hit (kick, t8 + q * 24.0, 110); c.hit (sn, t8 + q * 24.0, 100); } break;   // stab insieme, stile break
                case 10: if (sn >= 0) { for (int q = 0; q < 8; ++q) { const bool tq = tom >= 0 && q % 2; c.hit (tq ? tom : sn, t8 + q * 24.0, 70 + q * 6); } } break;   // giro misto rullante e tom
                default: if (hat >= 0) { for (int q = 0; q < 16; ++q) c.hit (hat, t0 + q * 6.0, 40 + q * 4); } break;                             // buzz di hat in sessantaquattresimi
            }
        }
        // piccolo fill a meta giro (battuta 2 di 4), piu leggero
        if (c.bars >= 4 && b == 1 && r.nextFloat() < 0.35f)
        {
            const double t = b * kBar + 14 * 24.0;
            if (sn >= 0 && r.nextBool()) { c.hit (sn, t, 60); c.hit (sn, t + 24, 72); }
            else if (hat >= 0) for (int q = 0; q < 4; ++q) c.hit (hat, t + 24 + q * 6.0, 50 + q * 8);
        }
    }
    // piatto all'inizio, una volta
    if (c.pad (R_CYM) >= 0 && c.bars >= 4 && r.nextFloat() < 0.35f) c.hit (c.pad (R_CYM), 0, 96);
    // vinile sotto, una volta ogni due battute
    if (S.vinyl && c.pad (R_TEX) >= 0) for (int b = 0; b < c.bars; b += 2) c.hit (c.pad (R_TEX), b * kBar, 70);
    // uno stab melodico, un solo pad melodico
    if (allowMel && c.pad (R_MEL) >= 0 && r.nextFloat() < S.mel)
    {
        const int m = c.pads[R_MEL][(size_t) r.nextInt ((int) c.pads[R_MEL].size())];
        const int every = r.nextBool() ? 1 : 2;
        for (int b = 0; b < c.bars; b += every) c.hit (m, b * kBar, 96);
    }
}

void applySwing (Processor& P, float sw) { if (sw > 0) if (auto* p = P.apvts.getParameter ("swing")) p->setValueNotifyingHost (p->convertTo0to1 (sw)); }

// ----------------------------------------------------------------------- MPC
juce::String mpc (Processor& P, const MStyle& S, juce::uint32 seed, int sel)
{
    // trova un gruppo di chop (stesso campione su piu pad); se non c'e, taglia un campione in 8
    std::map<SampleData*, std::vector<int>> groups;
    for (int p = 0; p < kPads; ++p) if (auto s = P.getSample (p)) if (P.pad[p].get (SLICE) > 0.5f) groups[s.get()].push_back (p);
    std::vector<int> chops; for (auto& [k, v] : groups) if (v.size() > chops.size()) chops = v;
    juce::String msg;
    if (chops.size() < 3)
    {
        int src = -1;
        auto len = [&] (int p) { auto s = P.getSample (p); return s ? s->buf.getNumSamples() / s->sr : 0.0; };
        // da tagliare ha senso un loop o una frase: uno stab corto in 8 pezzi suona male
        auto okSrc = [&] (int p) { const auto ro = padRole (P, p); return (ro == R_LOOP && len (p) > 1.5) || (ro == R_VOX && len (p) > 1.5) || (ro == R_MEL && len (p) > 3.0); };
        if (sel >= 0 && P.getSample (sel) && len (sel) > 1.5 && padRole (P, sel) != R_KICK && padRole (P, sel) != R_808) src = sel;   // il pad selezionato vince
        for (int p = kPads - 1; p >= 0 && src < 0; --p) if (padRole (P, p) == R_LOOP && okSrc (p)) src = p;
        for (int p = kPads - 1; p >= 0 && src < 0; --p) if (okSrc (p)) src = p;
        if (src < 0) return "MPC: serve un loop o una frase (oltre 1,5 s) da tagliare. Caricalo su un pad, selezionalo e riprova";
        std::vector<float> pts; for (int k = 0; k <= 8; ++k) pts.push_back (k / 8.0f);
        P.chopToPads (src, pts, 8);
        chops.clear(); for (int p = 8; p < 16; ++p) { chops.push_back (p); P.padCat[p] = "CHOP"; }
        msg = "Ho tagliato " + P.getSample (8)->name + " in 8 chop sui pad 9-16. ";
    }
    for (int p : chops) P.pad[p].v[CHOKE] = 8;   // i chop si tagliano tra loro, come in mono sulla MPC
    // batteria sui pad che non sono chop
    Ctx c (P, seed);
    for (auto& v : c.pads) v.erase (std::remove_if (v.begin(), v.end(), [&] (int p) { return std::find (chops.begin(), chops.end(), p) != chops.end(); }), v.end());
    c.bars = 4;
    static const int drumStyle[] = { 0, 2, 1, 3, 4 };   // boom bap, jazz hop, dusty, lo-fi, trap
    const auto& ds = dstyles()[(size_t) drumStyle[juce::jlimit (0, 4, S.drum)]];
    drums (c, ds, false);
    auto& r = c.r; const int n = (int) chops.size();
    auto ch = [&] (int k) { return chops[(size_t) (((k % n) + n) % n)]; };
    const int idx = (int) (&S - &mstyles()[0]);
    // frase di 2 battute
    std::vector<int> order; for (int k = 0; k < n; ++k) order.push_back (k);
    for (int k = n - 1; k > 0; --k) std::swap (order[(size_t) k], order[(size_t) r.nextInt (k + 1)]);
    for (int b = 0; b < c.bars; ++b)
    {
        const bool answer = (idx == 3 && b == 3);
        switch (idx)
        {
            case 0: case 3:   // a quarti, ordine nuovo; la quarta battuta del riff risponde
                for (int q = 0; q < 4; ++q) { const int k = answer ? order[(size_t) ((q + 4) % n)] : order[(size_t) ((b % 2) * 4 + q) % (size_t) n]; c.hit (ch (k), b * kBar + q * 96.0, 104); }
                if (r.nextFloat() < 0.5f) c.hit (ch (order[0]), b * kBar + 14 * 24.0, 84);
                break;
            case 1:           // Dilla: sincopi e tempi ubriachi
            {
                static const int pos[] = { 0, 3, 6, 10, 12, 15 };
                for (int q = 0; q < 6; ++q) if (q == 0 || r.nextFloat() < 0.75f) c.hit (ch (order[(size_t) ((b * 3 + q) % n)]), b * kBar + pos[q] * 24.0 + r.nextInt ({ -9, 10 }), 96);
                break;
            }
            case 2:           // stab: lo stesso chop corto
            {
                const int k = order[(size_t) (b % 2)];
                for (int i : { 0, 3, 6, 10, 11, 14 }) if (i == 0 || r.nextFloat() < 0.7f) c.hit (ch (k), b * kBar + i * 24.0, 100);
                break;
            }
            case 4:           // lo-fi: due chop lunghi per battuta
                c.hit (ch (order[(size_t) ((b * 2) % n)]), b * kBar, 96); c.hit (ch (order[(size_t) ((b * 2 + 1) % n)]), b * kBar + 192.0, 90);
                break;
            default:          // trap: un chop per battuta, lungo
                c.hit (ch (order[(size_t) (b % n)]), b * kBar, 100); if (r.nextBool()) c.hit (ch (order[(size_t) ((b + 2) % n)]), b * kBar + 10 * 24.0, 88);
                break;
        }
    }
    c.finish();
    { const juce::SpinLock::ScopedLockType l (P.recLock); P.events = c.ev; P.patternBars = c.bars; }
    applySwing (P, idx == 1 ? 62.0f : ds.swing);
    return msg + "MPC " + S.name + " su " + juce::String (n) + " chop";
}

// ----------------------------------------------------------------------- progressioni
// sorgente melodica: il pad selezionato (se melodico), poi quello usato l'ultima volta, poi il primo melodico.
// I pad accordo/intonati creati dalle progressioni non diventano mai la sorgente (si ripartirebbe dalle copie).
static int melSource (Processor& P, int sel, bool preferNote)
{
    auto isMel = [&] (int p) { return p >= 0 && p < kPads && P.getSample (p) != nullptr && padRole (P, p) == R_MEL && P.chordN[p].load() == 0 && P.chordName[p].isEmpty(); };
    auto isNote = [&] (int p) { return isMel (p) && P.pad[p].get (SKEY) >= 24; };
    if (isMel (sel) && (! preferNote || isNote (sel))) return sel;
    if (P.progSrc >= 0 && P.getSample (P.progSrc) != nullptr && (! preferNote || P.pad[P.progSrc].get (SKEY) >= 24)) return P.progSrc;
    if (preferNote) for (int p = 0; p < kPads; ++p) if (isNote (p)) return p;
    for (int p = 0; p < kPads; ++p) if (isMel (p)) return p;
    return -1;
}
// suono a nota singola di riserva (pluck sintetico in Do) se nel kit non ce n'e' uno
static int ensureNote (Processor& P, int pad)
{
    DrumDesc d { S_PLUCK, 60.0f, 2.4f, 0.55f, 0, 0, "Keys pluck C" };
    auto s = makeDrum (d); if (s == nullptr) return -1;
    P.pad[pad].reset(); P.setSample (pad, s); P.setLayer (pad, nullptr); P.padCat[pad] = "MELODICI"; P.pad[pad].v[SKEY] = 24; P.pad[pad].colour = 5;
    return pad;
}
// tonica su cui costruire: tonalita fissata, altrimenti quella del kit, altrimenti la nota del suono
static int tonicPc (Processor& P, int src)
{
    if (P.keyLock() >= 0) return P.keyLock() % 12;
    const int k = P.kitEffectiveKey (src); if (k >= 0) return k % 12;
    const int sk = juce::roundToInt (P.pad[src].get (SKEY)); return sk >= 24 ? (sk - 24) % 12 : (sk >= 0 ? sk % 12 : 0);
}
// trasforma i pad in pad accordo: stesso suono a nota singola, intonato sulla fondamentale, piu voci sopra
static void makeChordPads (Processor& P, int src, int tonic, const std::vector<std::pair<int, Chd>>& at)
{
    float pv[kNumPadParams]; for (int i = 0; i < kNumPadParams; ++i) pv[i] = P.pad[src].get (i);
    const int col = P.pad[src].colour.load(); auto s = P.getSample (src), l2 = P.getLayer (src);
    const int sp = juce::roundToInt (pv[SKEY]) >= 24 ? (juce::roundToInt (pv[SKEY]) - 24) % 12 : 0;
    for (auto& [p, c] : at)
    {
        for (int i = 0; i < kNumPadParams; ++i) P.pad[p].v[i] = pv[i];
        P.pad[p].colour = col; P.setSample (p, s); P.setLayer (p, l2); P.padCat[p] = "MELODICI";
        int d = ((tonic + c.root - sp) % 12 + 12) % 12; if (d > 6) d -= 12;
        P.pad[p].v[TUNE] = (float) d; P.pad[p].v[CHOKE] = 7; P.pad[p].v[MODE] = 0;
        std::vector<int> iv; for (int v : qualIv (c.q)) iv.push_back (d + v > 12 ? v - 12 : v);
        std::sort (iv.begin(), iv.end()); iv.erase (std::unique (iv.begin(), iv.end()), iv.end());
        P.setChord (p, iv); P.chordName[p] = chordLabel (tonic, c);
        P.pad[p].v[TUNE] = (float) d;
    }
    P.progSrc = src;
}
static int progFor (juce::Random& r, std::initializer_list<const char*> genres)
{
    std::vector<int> ok; const auto& B = progBank();
    for (int i = 0; i < (int) B.size(); ++i) for (auto* g : genres) if (juce::String (B[(size_t) i].genre) == g) ok.push_back (i);
    if (ok.empty()) for (int i = 0; i < (int) B.size(); ++i) ok.push_back (i);
    return ok[(size_t) r.nextInt ((int) ok.size())];
}
// ritmo degli accordi: un grado per battuta con il carattere dello stile
static void chordRhythm (std::vector<PatEv>& ev, juce::Random& r, int style, const int* padOfChord, int nCh, int bars)
{
    auto add = [&] (int k, double t, int v) { const int p = padOfChord[((k % nCh) + nCh) % nCh]; if (p >= 0) ev.push_back ({ t, p, juce::jlimit (30, 127, v + r.nextInt ({ -4, 5 })) }); };
    for (int b = 0; b < bars; ++b)
    {
        const int k = b % nCh; const double t = b * (double) kBar;
        switch (style)
        {
            case 0: add (k, t, 104); add (k, t + 10 * 24.0, 82); break;                                    // soul: ripreso sul levare del 3
            case 1: add (k, t, 110); break;                                                                 // trap: lungo
            case 2: for (int i : { 0, 6, 10 }) add (k, t + i * 24.0, i == 0 ? 108 : 90); break;             // drill 3-3-2
            case 3: add (k, t, 96); for (int i : { 3, 7, 11 }) if (r.nextFloat() < 0.7f) add (k, t + i * 24.0, 80); break;   // lo-fi in levare
            case 4: add (k, t, 100); add (k, t + 8 * 24.0, 86); add (k + 1, t + 14 * 24.0, 96); break;      // r&b: il cambio in anticipo
            case 5: for (int i : { 0, 6, 8, 14 }) if (i == 0 || r.nextFloat() < 0.75f) add (k, t + i * 24.0, 100); break;   // stab
            case 6: add (k * 2, t, 104); add (k * 2 + 1, t + 192.0, 98); break;                            // ballad: due per battuta
            default: add (k, t, 104); break;
        }
    }
}
juce::String prog (Processor& P, const PStyle& S, juce::uint32 seed, int sel)
{
    juce::Random r (mix (seed));
    const int idx = (int) (&S - &pstyles()[0]);
    int src = melSource (P, sel, true);
    const bool noteSrc = src >= 0 && P.pad[src].get (SKEY) >= 24;
    if (idx != 7 && (noteSrc || src < 0))
    {   // accordi veri: una progressione della banca, scelta a ogni giro (ogni GENERA ne prova una diversa)
        if (src < 0) src = ensureNote (P, 8);
        if (src < 0) return "Per le progressioni serve un suono melodico";
        static const std::vector<std::vector<const char*>> gen = { { "R&B", "Boom Bap" }, { "Trap" }, { "Drill" }, { "Lo-fi" }, { "R&B" }, { "Boom Bap" }, { "Pop", "R&B", "Ambient" } };
        const auto& G = gen[(size_t) juce::jlimit (0, 6, idx)];
        std::vector<int> pool; for (int i = 0; i < (int) progBank().size(); ++i) for (auto* g : G) if (juce::String (progBank()[(size_t) i].genre) == g) pool.push_back (i);
        // non ripetere la stessa di prima
        int bi = pool[(size_t) r.nextInt ((int) pool.size())];
        if (pool.size() > 1 && bi == P.lastProg) bi = pool[(size_t) ((std::find (pool.begin(), pool.end(), bi) - pool.begin() + 1) % (long) pool.size())];
        P.lastProg = bi;
        const auto& B = progBank()[(size_t) bi];
        const int tonic = tonicPc (P, src);
        std::vector<std::pair<int, Chd>> at; int padOf[4];
        for (int k = 0; k < 4; ++k) { padOf[k] = 12 + k; at.push_back ({ 12 + k, B.ch[(size_t) k % B.ch.size()] }); }
        makeChordPads (P, src, tonic, at);
        std::vector<PatEv> ev; { const juce::SpinLock::ScopedLockType l (P.recLock); for (auto& e : P.events) if (e.pad < 12 && e.pad != src) ev.push_back (e); }
        chordRhythm (ev, r, idx, padOf, 4, 4);
        std::sort (ev.begin(), ev.end(), [] (const PatEv& a, const PatEv& b) { return a.tick < b.tick; });
        { const juce::SpinLock::ScopedLockType l (P.recLock); P.events = ev; P.patternBars = juce::jmax (P.patternBars.load(), 4); }
        if (idx == 3) applySwing (P, 58);
        juce::String deg; for (int k = 0; k < 4; ++k) deg << (k ? " - " : "") << P.chordName[12 + k];
        return juce::String (S.name) + ": " + B.name + "  " + deg + "  (accordi sui pad 13-16)";
    }
    if (src < 0) src = melSource (P, sel, false);
    if (src < 0) return "Per le progressioni serve un suono melodico (accordo o nota): caricane uno su un pad e selezionalo";
    auto s = P.getSample (src);
    P.progSrc = src;
    // qualita del suono: dalla tonalita rilevata o dal nome
    const int key = juce::roundToInt (P.pad[src].get (SKEY));
    const auto nm = s->name.toLowerCase();
    enum Q { MINOR, MAJOR, MAJ7, SUS, DOM, NOTE } q = MINOR;
    if (key >= 24) q = NOTE; else if (key >= 12) q = MINOR; else if (key >= 0) q = MAJOR;
    if (nm.contains ("maj7") || nm.contains ("maj9") || nm.contains ("maj 7")) q = MAJ7;
    else if (nm.contains ("sus")) q = SUS;
    else if (q == MAJOR && (nm.contains ("7") || nm.contains ("9")) && ! nm.contains ("maj")) q = DOM;
    if (idx == 7) q = NOTE;
    // l'accordo campionato si sposta intero: solo gradi che restano nella scala con la stessa qualita
    std::vector<std::vector<int>> seqs; juce::String what;
    switch (q)
    {
        case MINOR: seqs = { { 0, 0, 5, 7 }, { 0, 5, 0, 7 }, { 0, 7, 5, 0 }, { 0, 5, 5, 7 }, { 0, 0, 7, 5 }, { 0, 5, 7, 5 }, { 0, 7, 0, 5 }, { 0, 5, 7, 7 }, { 7, 5, 0, 0 }, { 0, 0, 5, 5 } }; what = "minore: i, iv, v"; break;
        case MAJOR: seqs = { { 0, 5, 7, 5 }, { 0, 7, 5, 0 }, { 0, 0, 5, 7 }, { 0, 5, 0, 7 }, { 0, 7, 7, 5 }, { 5, 0, 7, 0 }, { 0, 5, 5, 0 } }; what = "maggiore: I, IV, V"; break;
        case MAJ7:  seqs = { { 0, 5, 0, 5 }, { 0, 0, 5, 5 }, { 0, 5, 5, 0 }, { 0, 5, 0, 0 }, { 5, 0, 5, 0 } }; what = "maj7: I e IV"; break;
        case SUS:   seqs = { { 0, 9, 5, 7 }, { 0, 5, 9, 7 }, { 0, 7, 9, 5 }, { 0, 2, 5, 7 }, { 0, 5, 7, 9 } }; what = "sus: gradi della scala maggiore"; break;
        case DOM:   seqs = { { 0, 0, 0, 0 }, { 0, 5, 0, 0 } }; what = "dominante"; break;
        case NOTE:  seqs = { { 0, 3, 5, 7 }, { 0, 7, 5, 3 }, { 0, 5, 3, 7 }, { 0, 3, 7, 10 }, { 0, 10, 7, 5 }, { 0, 3, 10, 7 } }; what = "nota singola: giro nella scala minore"; break;
    }
    int pick = r.nextInt ((int) seqs.size());
    if (seqs.size() > 1 && pick == P.lastProg - 1000) pick = (pick + 1) % (int) seqs.size();
    P.lastProg = 1000 + pick;
    const auto seq = seqs[(size_t) pick];
    std::vector<int> ivs; for (int v : seq) if (std::find (ivs.begin(), ivs.end(), v) == ivs.end()) ivs.push_back (v);
    std::map<int, int> padOf; const float baseTune = P.pad[src].get (TUNE);
    float pv[kNumPadParams]; for (int i = 0; i < kNumPadParams; ++i) pv[i] = P.pad[src].get (i);
    for (size_t k = 0; k < ivs.size() && k < 4; ++k)
    {
        const int p = 12 + (int) k; if (p == src) continue;
        for (int i = 0; i < kNumPadParams; ++i) P.pad[p].v[i] = pv[i];
        P.pad[p].colour = P.pad[src].colour.load();
        const int iv = ivs[k]; const int shift = iv > 7 ? iv - 12 : iv;
        P.pad[p].v[TUNE] = juce::jlimit (-24.0f, 24.0f, baseTune + shift);
        P.pad[p].v[CHOKE] = 7;
        P.setSample (p, s); P.setLayer (p, P.getLayer (src)); P.padCat[p] = "MELODICI"; P.setChord (p, {});
        static const char* rom[] = { "I", "bII", "II", "bIII", "III", "IV", "#IV", "V", "bVI", "VI", "bVII", "VII" };
        P.chordName[p] = q == MINOR || q == NOTE ? juce::String (rom[iv]).toLowerCase() : juce::String (rom[iv]);
        padOf[iv] = p;
    }
    if (padOf.count (0) == 0) padOf[0] = src;
    if (src >= 12) P.pad[src].v[CHOKE] = 7;
    std::vector<PatEv> keep;
    { const juce::SpinLock::ScopedLockType l (P.recLock); for (auto& e : P.events) if (e.pad < 12 && e.pad != src) keep.push_back (e); }
    std::vector<PatEv> ev = keep; const int bars = 4;
    if (idx == 7)
    {
        auto add = [&] (int iv, double t, int v) { auto it = padOf.find (iv); if (it != padOf.end()) ev.push_back ({ t, it->second, juce::jlimit (30, 127, v + r.nextInt ({ -4, 5 })) }); };
        static const char* rows[] = { "X..x..x...x.x...", "X.x...x.x...x.x.", "X...x.x...x...x." };
        for (int b = 0; b < bars; ++b) { const char* row = rows[r.nextInt (3)]; for (int i = 0; i < 16; ++i) if (row[i] != '.') add (ivs[(size_t) r.nextInt ((int) ivs.size())], b * kBar + i * 24.0, row[i] == 'X' ? 108 : 88); }
    }
    else
    {
        int po[8]; for (int k = 0; k < 8; ++k) po[k] = padOf[seq[(size_t) k % seq.size()]];
        chordRhythm (ev, r, idx, po, (int) seq.size(), bars);
    }
    std::sort (ev.begin(), ev.end(), [] (const PatEv& a, const PatEv& b) { return a.tick < b.tick; });
    { const juce::SpinLock::ScopedLockType l (P.recLock); P.events = ev; P.patternBars = juce::jmax (P.patternBars.load(), bars); }
    if (idx == 3) applySwing (P, 58);
    juce::String deg; static const char* rom[] = { "I", "bII", "II", "bIII", "III", "IV", "#IV", "V", "bVI", "VI", "bVII", "VII" };
    for (int v : seq) deg << (deg.isEmpty() ? "" : " - ") << (q == MINOR || q == NOTE ? juce::String (rom[v]).toLowerCase() : juce::String (rom[v]));
    return S.name + juce::String (": ") + deg + "  (" + what + ", pad 13-16)";
}
// PROGRESSIONI MPC: la progressione sui pad 13-16 e altri accordi della stessa tonalita sui pad 10-12, da suonare a mano.
// Sotto: batteria del genere e gli accordi uno per battuta.
juce::String progMpc (Processor& P, int bankIdx, juce::uint32 seed, int sel)
{
    juce::Random r (mix (seed));
    const auto& B = progBank()[(size_t) juce::jlimit (0, (int) progBank().size() - 1, bankIdx)];
    int src = melSource (P, sel, true);
    if (src < 0 || P.pad[src].get (SKEY) < 24) src = ensureNote (P, 9);   // serve una nota singola: se non c'e' uso un pluck
    if (src < 0) return "Serve un suono a nota singola";
    const int tonic = tonicPc (P, src);
    std::vector<std::pair<int, Chd>> at;
    for (int k = 0; k < 4; ++k) at.push_back ({ 12 + k, B.ch[(size_t) k % B.ch.size()] });
    // accordi in piu della stessa scala (quelli che la progressione non usa)
    const std::vector<Chd> dia = B.minor ? std::vector<Chd> { { 0, Qm7 }, { 3, QM7 }, { 5, Qm7 }, { 7, Qm7 }, { 8, QM7 }, { 10, Q7 }, { 2, Qhd } }
                                         : std::vector<Chd> { { 0, QM7 }, { 2, Qm7 }, { 4, Qm7 }, { 5, QM7 }, { 7, Q7 }, { 9, Qm7 }, { 11, Qhd } };
    int extra = 0;
    for (auto& c : dia)
    {
        if (extra >= 3) break; bool used = false; for (auto& x : B.ch) if (x.root == c.root) used = true;
        if (used) continue; const int p = 9 + extra; if (p == src) continue; at.push_back ({ p, c }); ++extra;
    }
    makeChordPads (P, src, tonic, at);
    // batteria del genere sui pad 1-9, accordi sopra
    static const std::pair<const char*, const char*> gmap[] = { { "Trap", "Trap" }, { "Drill", "Drill UK" }, { "R&B", "R&B" }, { "Lo-fi", "Lo-fi" }, { "Boom Bap", "Boom Bap" }, { "Pop", "R&B" }, { "Afro / Latin", "Afrobeats" }, { "Ambient", "Lo-fi Bedroom" } };
    int dsi = 0; for (auto& [g, d] : gmap) if (juce::String (B.genre) == g) for (int i = 0; i < (int) dstyles().size(); ++i) if (juce::String (dstyles()[(size_t) i].name) == d) dsi = i;
    Ctx c (P, seed);
    for (auto& v : c.pads) v.erase (std::remove_if (v.begin(), v.end(), [] (int p) { return p >= 9; }), v.end());
    c.bars = 4; drums (c, dstyles()[(size_t) dsi], false);
    const int st = juce::String (B.genre) == "Trap" ? 1 : juce::String (B.genre) == "Drill" ? 2 : juce::String (B.genre) == "Lo-fi" ? 3 : juce::String (B.genre) == "R&B" ? 4 : juce::String (B.genre) == "Boom Bap" ? 0 : 1;
    int po[4] = { 12, 13, 14, 15 };
    chordRhythm (c.ev, r, st, po, 4, 4);
    c.finish();
    { const juce::SpinLock::ScopedLockType l (P.recLock); P.events = c.ev; P.patternBars = 4; }
    applySwing (P, dstyles()[(size_t) dsi].swing);
    juce::String deg; for (int k = 0; k < 4; ++k) deg << (k ? " - " : "") << P.chordName[12 + k];
    return juce::String ("Progressione ") + B.name + ": " + deg + "  (pad 13-16, altri accordi su 10-12)";
}
}
const std::vector<GenStyle>& genStyles()
{
    static const std::vector<GenStyle> all = []
    {
        std::vector<GenStyle> v;
        for (int i = 0; i < (int) dstyles().size(); ++i) v.push_back ({ dstyles()[(size_t) i].name, GC_DRUM, i, dstyles()[(size_t) i].desc });
        for (int i = 0; i < (int) mstyles().size(); ++i) v.push_back ({ mstyles()[(size_t) i].name, GC_MPC, i, mstyles()[(size_t) i].desc });
        for (int i = 0; i < (int) pstyles().size(); ++i) v.push_back ({ pstyles()[(size_t) i].name, GC_PROG, i, pstyles()[(size_t) i].desc });
        for (int i = 0; i < (int) progBank().size(); ++i)
        {
            juce::String d = juce::String (progBank()[(size_t) i].genre) + ": ";
            for (size_t k = 0; k < progBank()[(size_t) i].ch.size(); ++k) d << (k ? " - " : "") << chordLabel (progBank()[(size_t) i].minor ? 0 : 0, progBank()[(size_t) i].ch[k]);
            v.push_back ({ progBank()[(size_t) i].name, GC_PROGMPC, i, d + " (in Do)" });
        }
        return v;
    }();
    return all;
}
int stylesInCat (int cat, std::vector<int>& out) { out.clear(); for (int i = 0; i < (int) genStyles().size(); ++i) if (genStyles()[(size_t) i].cat == cat) out.push_back (i); return (int) out.size(); }

std::vector<GenPreset> genPresets (int style)
{
    std::vector<GenPreset> v; const auto& st = genStyles()[(size_t) juce::jlimit (0, (int) genStyles().size() - 1, style)];
    if (st.cat == GC_DRUM)
    {
        const auto& ds = dstyles()[(size_t) st.idx];
        // i loop scritti a mano dello stesso genere (solo per il primo stile di quel genere)
        bool firstOfGenre = true; for (int i = 0; i < st.idx; ++i) if (juce::String (dstyles()[(size_t) i].bankGenre) == ds.bankGenre) firstOfGenre = false;
        if (firstOfGenre) for (int i = 0; i < (int) patternBank().size(); ++i) if (juce::String (patternBank()[(size_t) i].genre) == ds.bankGenre) v.push_back ({ juce::String ("Scritto | ") + patternBank()[(size_t) i].name, i, 0 });
    }
    static const char* adj[] = { "Classico", "Rimbalzo", "Notte", "Spezzato", "Pieno", "Minimale", "Sporco", "Strada", "Fumo", "Pesante", "Leggero", "Nervoso", "Lento", "Caldo", "Freddo", "Ultimo giro",
                                 "Seconda strofa", "Ritornello", "Intro", "Bridge", "Dopo mezzanotte", "Cantina", "Tetto", "Periferia" };
    for (int k = 0; k < 24; ++k) v.push_back ({ juce::String (k + 1).paddedLeft ('0', 2) + "  " + adj[k], -1, (juce::uint32) (style * 7919 + k * 104729 + 17) });
    return v;
}

juce::String generate (Processor& P, int style, juce::uint32 seed, int sel)
{
    const auto& st = genStyles()[(size_t) juce::jlimit (0, (int) genStyles().size() - 1, style)];
    if (st.cat == GC_MPC) return mpc (P, mstyles()[(size_t) st.idx], seed, sel);
    if (st.cat == GC_PROG) return prog (P, pstyles()[(size_t) st.idx], seed, sel);
    if (st.cat == GC_PROGMPC) return progMpc (P, st.idx, seed, sel);
    const auto& ds = dstyles()[(size_t) st.idx];
    Ctx c (P, seed);
    c.bars = c.r.nextFloat() < 0.2f ? 2 : ds.bars;
    drums (c, ds);
    c.finish();
    { const juce::SpinLock::ScopedLockType l (P.recLock); P.events = c.ev; P.patternBars = c.bars; }
    applySwing (P, ds.swing);
    juce::String miss;
    if (c.ev.empty()) return "Il kit e vuoto: carica dei suoni o un kit e riprova";
    if (c.pad (R_KICK) < 0 && c.pad (R_808) < 0) miss << " (nel kit manca una cassa)";
    if (c.pad (R_SNARE) < 0 && c.pad (R_CLAP) < 0) miss << " (manca rullante o clap)";
    return st.name + ": " + juce::String (c.bars) + " battute" + miss;
}

juce::String regenRow (Processor& P, int style, juce::uint32 seed, int pad)
{
    // serve uno stile di batteria: se quello scelto e' MPC o PROGRESSIONI si usa il primo di batteria
    int st = juce::jlimit (0, (int) genStyles().size() - 1, style);
    if (genStyles()[(size_t) st].cat != GC_DRUM) for (int i = 0; i < (int) genStyles().size(); ++i) if (genStyles()[(size_t) i].cat == GC_DRUM) { st = i; break; }
    const auto& ds = dstyles()[(size_t) genStyles()[(size_t) st].idx];
    const int bars = P.patternBars.load();
    std::vector<PatEv> got;
    for (int t = 0; t < 8 && got.empty(); ++t)
    {
        Ctx c (P, seed + (juce::uint32) t * 7919u); c.bars = bars;
        drums (c, ds); c.finish();
        for (auto& e : c.ev) if (e.pad == pad && e.tick < bars * kBar) got.push_back (e);
    }
    if (got.empty()) return "Pad " + juce::String (pad + 1) + ": nello stile " + genStyles()[(size_t) st].name + " questo suono non ha una parte di batteria";
    { const juce::SpinLock::ScopedLockType l (P.recLock);
      P.events.erase (std::remove_if (P.events.begin(), P.events.end(), [pad] (const PatEv& e) { return e.pad == pad; }), P.events.end());
      for (auto& e : got) P.events.push_back (e); }
    return "Pad " + juce::String (pad + 1) + " rigenerato (" + genStyles()[(size_t) st].name + "), il resto e' rimasto com'era";
}

void loadBankByRoles (Processor& P, int index)
{
    P.loadPattern (index);   // scrive sulla disposizione standard dei kit di fabbrica
    // disposizione standard -> ruolo
    static const Role lay[kPads] = { R_KICK, R_SNARE, R_HAT, R_OPEN, R_CLAP, R_PERC, R_PERC, R_CYM, R_808, R_MEL, R_MEL, R_MEL, R_MEL, R_PERC, R_FX, R_PERC };
    Ctx c (P, 1);
    std::vector<PatEv> out;
    { const juce::SpinLock::ScopedLockType l (P.recLock);
      for (auto e : P.events)
      {
          const Role ro = lay[juce::jlimit (0, kPads - 1, e.pad)];
          int p = padRole (P, e.pad) == ro ? e.pad : c.pad (ro);
          if (p < 0 && ro == R_CLAP) p = c.pad (R_SNARE);
          if (p < 0 && ro == R_SNARE) p = c.pad (R_CLAP);
          if (p < 0 && ro == R_OPEN) continue;
          if (p < 0 && ro == R_808) continue;
          if (p < 0 && ro == R_KICK) p = c.pad (R_808);
          if (p < 0 && ro == R_HAT) p = c.pad (R_PERC);
          if (p < 0) continue;
          e.pad = p; out.push_back (e);
      } }
    c.ev = out; c.r = juce::Random (2); c.finish();
    const juce::SpinLock::ScopedLockType l (P.recLock); P.events = c.ev;
}
}

namespace jp
{
// Allunga il pattern (es. 2 -> 4 o 4 -> 8 battute) tenendo quello che c'e: le battute si ripetono e
// l'ultima di ogni giro da 4 cambia un po' (fill di rullante, rullata di hat, stacco, cassa spostata),
// come si fa a mano. Se oltre la lunghezza attuale c'erano gia note (pattern accorciato prima) non tocca nulla.
void extendPattern (Processor& P, int newBars, juce::uint32 seed)
{
    int cur = juce::jmax (1, P.patternBars.load());
    if (newBars <= cur) { P.patternBars = newBars; return; }
    std::vector<PatEv> ev; { const juce::SpinLock::ScopedLockType l (P.recLock); ev = P.events; }
    if (ev.empty()) { P.patternBars = newBars; return; }
    // quanto e' lungo davvero il materiale (es. da 4 a 2 e poi a 8: le battute 3-4 c'erano ancora)
    double last = 0; for (auto& e : ev) last = juce::jmax (last, e.tick);
    int have = juce::jlimit (1, 8, (int) std::floor (last / kBar) + 1);
    for (int b : { 1, 2, 4, 8 }) if (b >= have) { have = b; break; }   // si ripete a blocchi di 1, 2, 4 battute
    if (have >= newBars) { P.patternBars = newBars; return; }   // c'e' gia' tutto
    cur = juce::jmax (cur, have);
    ev.erase (std::remove_if (ev.begin(), ev.end(), [cur] (const PatEv& e) { return e.tick >= cur * (double) kBar; }), ev.end());
    juce::Random r ((juce::int64) seed);
    constexpr double st = kBar / 16.0;
    int padOf[kRoles]; for (auto& x : padOf) x = -1;
    for (int p = 0; p < kPads; ++p) { const int ro = padRole (P, p); if (padOf[ro] < 0) padOf[ro] = p; }
    auto used = [&ev] (int p) { if (p < 0) return false; for (auto& e : ev) if (e.pad == p) return true; return false; };
    const int snare = used (padOf[R_SNARE]) ? padOf[R_SNARE] : used (padOf[R_CLAP]) ? padOf[R_CLAP] : padOf[R_SNARE];
    const int hat = used (padOf[R_HAT]) ? padOf[R_HAT] : -1, kick = padOf[R_KICK], b808 = padOf[R_808], cym = padOf[R_CYM], tom = padOf[R_TOM];
    std::vector<PatEv> out;
    for (int b = 0; b < newBars; ++b)
        for (auto& e : ev) if ((int) (e.tick / kBar) == b % cur) out.push_back ({ e.tick + (b - b % cur) * (double) kBar, e.pad, e.vel });
    auto isDrum = [&] (int p) { const int ro = padRole (P, p); return ro >= R_KICK && ro <= R_CYM; };
    auto clear = [&out] (double t0, double t1, std::function<bool (int)> which) { out.erase (std::remove_if (out.begin(), out.end(), [&] (const PatEv& e) { return e.tick >= t0 && e.tick < t1 && which (e.pad); }), out.end()); };
    for (int b = 3; b < newBars; b += 4)
    {
        const bool big = b == newBars - 1 && newBars >= 8;   // l'ottava cambia di piu della quarta
        const double t0 = b * (double) kBar;
        int kind = r.nextInt (big ? 5 : 4);
        if (snare < 0 && (kind == 0)) kind = 2;
        if (hat < 0 && kind == 1) kind = 2;
        switch (kind)
        {
            case 0:   // fill di rullante sull'ultimo mezzo giro (o sull'ultimo quarto), crescendo
            {
                const int from = big ? 8 : 12;
                clear (t0 + from * st, t0 + kBar, [&] (int p) { return p == snare || p == hat || p == tom; });
                for (int s = from; s < 16; ++s) if (big || s % 2 == 0 || s >= 14) out.push_back ({ t0 + s * st, (big && tom >= 0 && s >= 12 && s % 2) ? tom : snare, juce::jlimit (40, 127, 60 + (s - from) * 8) });
                break;
            }
            case 1:   // rullata di hat in trentaduesimi sull'ultimo movimento
            {
                clear (t0 + 12 * st, t0 + kBar, [&] (int p) { return p == hat; });
                for (int k = 0; k < 8; ++k) out.push_back ({ t0 + 12 * st + k * st * 0.5, hat, 50 + k * 8 });
                break;
            }
            case 2:   // stacco: si ferma tutto sull'ultimo movimento (resta solo l'ultimo colpo di rullante se c'e)
            {
                clear (t0 + 12 * st, t0 + kBar, [&] (int p) { return isDrum (p) || p == b808; });
                if (snare >= 0) out.push_back ({ t0 + 15 * st, snare, 96 });
                break;
            }
            case 3:   // groove: la cassa (e l'808 con lei) cambia nella seconda meta della battuta
            {
                clear (t0 + 8 * st, t0 + kBar, [&] (int p) { return p == kick || p == b808; });
                static const char* alt[] = { "X..X..X.", "X.X...X.", "..X.X..X", "X...X.X." };
                const juce::String a = alt[r.nextInt (4)];
                for (int s = 0; s < 8; ++s) if (a[s] == 'X') { if (kick >= 0) out.push_back ({ t0 + (8 + s) * st, kick, 110 }); if (used (b808)) out.push_back ({ t0 + (8 + s) * st, b808, 104 }); }
                break;
            }
            default:  // svuota la seconda meta (solo hat e melodia) e riparte col piatto
            {
                clear (t0 + 8 * st, t0 + kBar, [&] (int p) { return p == kick || p == b808 || p == snare; });
                if (snare >= 0) for (int s : { 12, 13, 14, 15 }) out.push_back ({ t0 + s * st, snare, 70 + (s - 12) * 15 });
                break;
            }
        }
        if (cym >= 0 && b + 1 < newBars) out.push_back ({ (b + 1) * (double) kBar, cym, 100 });   // piatto alla ripartenza
    }
    std::sort (out.begin(), out.end(), [] (const PatEv& a, const PatEv& b) { return a.tick < b.tick; });
    { const juce::SpinLock::ScopedLockType l (P.recLock); P.events = out; P.patternBars = newBars; }
}
}
