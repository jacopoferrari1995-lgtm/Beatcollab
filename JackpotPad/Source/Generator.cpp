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
const char* genCatName (int c) { static const char* n[] = { "DRUM", "MPC", "PROGRESSIONI" }; return n[juce::jlimit (0, kGenCats - 1, c)]; }

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
          { "........X......." }, {}, { "" }, { "......x...........", "" },
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
          1, 0.0f, 0.0f, 0, {}, 0, 0.0f, -1, 4, 0.15f, false, 0 } };
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
    const bool fill = r.nextFloat() < 0.65f;
    const int fillKind = r.nextInt (4);
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
        // riempimento nell'ultima battuta
        if (last)
        {
            const double t0 = b * kBar + 12 * 24.0;
            if (fillKind == 0 && sn >= 0) for (int q = 0; q < 4; ++q) c.hit (sn, t0 + q * 24.0, 70 + q * 14);
            else if (fillKind == 1 && c.pad (R_TOM) >= 0) { for (int q = 0; q < 4; ++q) c.hit (c.pad (R_TOM, q % juce::jmax (1, (int) c.pads[R_TOM].size())), t0 + q * 24.0, 100); }
            else if (fillKind == 2 && hat >= 0) for (int q = 0; q < 8; ++q) c.hit (hat, t0 + q * 12.0, 56 + q * 8);
            else if (kick >= 0) { c.hit (kick, t0 + 2 * 24.0, 100); c.hit (kick, t0 + 3 * 24.0, 90); }
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
juce::String prog (Processor& P, const PStyle& S, juce::uint32 seed, int sel)
{
    juce::Random r (mix (seed));
    const int idx = (int) (&S - &pstyles()[0]);
    int src = -1;
    if (sel >= 0 && padRole (P, sel) == R_MEL) src = sel;
    for (int p = 0; p < kPads && src < 0; ++p) if (padRole (P, p) == R_MEL && p < 12) src = p;
    for (int p = 0; p < kPads && src < 0; ++p) if (padRole (P, p) == R_MEL) src = p;
    if (src < 0) return "Per le progressioni serve un suono melodico (accordo o nota): caricane uno su un pad e selezionalo";
    auto s = P.getSample (src);
    // qualita del suono: dalla tonalita rilevata o dal nome
    const int key = juce::roundToInt (P.pad[src].get (SKEY));
    const auto nm = s->name.toLowerCase();
    enum Q { MINOR, MAJOR, MAJ7, SUS, DOM, NOTE } q = MINOR;
    if (key >= 24) q = NOTE; else if (key >= 12) q = MINOR; else if (key >= 0) q = MAJOR;
    if (nm.contains ("maj7") || nm.contains ("maj9") || nm.contains ("maj 7")) q = MAJ7;
    else if (nm.contains ("sus")) q = SUS;
    else if (q == MAJOR && (nm.contains ("7") || nm.contains ("9")) && ! nm.contains ("maj")) q = DOM;
    if (idx == 7) q = NOTE;
    // gradi ammessi: solo accordi della scala con la stessa qualita del campione (niente note fuori scala)
    std::vector<std::vector<int>> seqs; juce::String what;
    switch (q)
    {
        case MINOR: seqs = { { 0, 0, 5, 7 }, { 0, 5, 0, 7 }, { 0, 7, 5, 0 }, { 0, 5, 5, 7 }, { 0, 0, 7, 5 }, { 0, 5, 7, 5 } }; what = "minore: i, iv, v (minore naturale)"; break;
        case MAJOR: seqs = { { 0, 5, 7, 5 }, { 0, 7, 5, 0 }, { 0, 0, 5, 7 }, { 0, 5, 0, 7 } }; what = "maggiore: I, IV, V"; break;
        case MAJ7:  seqs = { { 0, 5, 0, 5 }, { 0, 0, 5, 5 }, { 0, 5, 5, 0 }, { 0, 5, 0, 0 } }; what = "maj7: I e IV"; break;
        case SUS:   seqs = { { 0, 9, 5, 7 }, { 0, 5, 9, 7 }, { 0, 7, 9, 5 }, { 0, 2, 5, 7 } }; what = "sus: gradi della scala maggiore"; break;
        case DOM:   seqs = { { 0, 0, 0, 0 } }; what = "dominante: un accordo solo, ritmo"; break;
        case NOTE:  seqs = { { 0, 3, 5, 7 }, { 0, 7, 5, 3 }, { 0, 5, 3, 7 }, { 0, 3, 7, 10 } }; what = "nota singola: giro nella scala minore"; break;
    }
    const auto seq = seqs[(size_t) r.nextInt ((int) seqs.size())];
    // un pad per ogni intervallo (pad 13-16), stesso suono intonato sul grado giusto
    std::vector<int> ivs; for (int v : seq) if (std::find (ivs.begin(), ivs.end(), v) == ivs.end()) ivs.push_back (v);
    std::map<int, int> padOf; const float baseTune = P.pad[src].get (TUNE);
    for (size_t k = 0; k < ivs.size() && k < 4; ++k)
    {
        const int p = 12 + (int) k; if (p == src) continue;
        for (int i = 0; i < kNumPadParams; ++i) P.pad[p].v[i] = P.pad[src].get (i);
        P.pad[p].colour = P.pad[src].colour.load();
        const int iv = ivs[k]; const int shift = iv > 7 ? iv - 12 : iv;
        P.pad[p].v[TUNE] = juce::jlimit (-24.0f, 24.0f, baseTune + shift);
        P.pad[p].v[CHOKE] = 7;   // l'accordo nuovo taglia il precedente
        P.setSample (p, s); P.setLayer (p, P.getLayer (src)); P.padCat[p] = "MELODICI";
        padOf[iv] = p;
    }
    if (padOf.count (0) == 0) padOf[0] = src;
    if (src >= 12) P.pad[src].v[CHOKE] = 7;
    // ritmo: 4 battute, un grado per battuta (la ballad ne fa due)
    std::vector<PatEv> keep;
    { const juce::SpinLock::ScopedLockType l (P.recLock); for (auto& e : P.events) if (e.pad < 12 && e.pad != src) keep.push_back (e); }
    std::vector<PatEv> ev = keep; const int bars = 4;
    auto add = [&] (int iv, double t, int v) { auto it = padOf.find (iv); if (it != padOf.end()) ev.push_back ({ t, it->second, juce::jlimit (30, 127, v + r.nextInt ({ -4, 5 })) }); };
    for (int b = 0; b < bars; ++b)
    {
        const int iv = seq[(size_t) b % seq.size()], nx = seq[(size_t) (b + 1) % seq.size()]; const double t = b * kBar;
        switch (idx)
        {
            case 0: add (iv, t, 104); add (iv, t + 10 * 24.0, 82); break;
            case 1: add (iv, t, 110); break;
            case 2: for (int i : { 0, 6, 10 }) add (iv, t + i * 24.0, i == 0 ? 108 : 90); break;
            case 3: add (iv, t, 96); for (int i : { 3, 7, 11 }) if (r.nextFloat() < 0.7f) add (iv, t + i * 24.0, 80); break;
            case 4: add (iv, t, 100); add (iv, t + 8 * 24.0, 86); if (b < bars - 1 || true) add (nx, t + 14 * 24.0, 96); break;
            case 5: for (int i : { 0, 6, 8, 14 }) if (i == 0 || r.nextFloat() < 0.75f) add (iv, t + i * 24.0, 100); break;
            case 6: add (iv, t, 104); add (seq[(size_t) (b * 2 + 1) % seq.size()], t + 192.0, 98); break;
            default:
            {   // riff di note nella scala, sedicesimi scelti
                static const char* rows[] = { "X..x..x...x.x...", "X.x...x.x...x.x.", "X...x.x...x...x." };
                const char* row = rows[r.nextInt (3)];
                for (int i = 0; i < 16; ++i) if (row[i] != '.') add (ivs[(size_t) r.nextInt ((int) ivs.size())], t + i * 24.0, row[i] == 'X' ? 108 : 88);
            }
        }
    }
    std::sort (ev.begin(), ev.end(), [] (const PatEv& a, const PatEv& b) { return a.tick < b.tick; });
    { const juce::SpinLock::ScopedLockType l (P.recLock); P.events = ev; P.patternBars = juce::jmax (P.patternBars.load(), bars); }
    if (idx == 3) applySwing (P, 58);
    juce::String deg; static const char* rom[] = { "I", "bII", "II", "bIII", "III", "IV", "#IV", "V", "bVI", "VI", "bVII", "VII" };
    for (int v : seq) deg << (deg.isEmpty() ? "" : " - ") << (q == MINOR || q == NOTE ? juce::String (rom[v]).toLowerCase() : juce::String (rom[v]));
    return S.name + juce::String (": ") + deg + "  (" + what + ", pad 13-16)";
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
