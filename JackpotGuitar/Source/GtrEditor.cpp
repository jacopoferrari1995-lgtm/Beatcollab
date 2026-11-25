#include "GtrEditor.h"
#include <set>
#include <map>

namespace gtr
{
static juce::Colour strCol (int s)
{
    const juce::Colour c[kStr] = { C::red, C::rust, C::yellow, C::green, C::phos, C::violet };
    return c[juce::jlimit (0, kStr - 1, s)];
}
static void chipBtn (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& t, bool on, juce::Colour c, float fs = 10.5f, bool hover = false) { jlook::button (g, r, c, on, hover, t, jkit::uf (fs * 0.92f)); }
static void panel (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c, const juce::String& title, const juce::String& sub)
{
    jlook::softShadow (g, r, 14); jlook::card (g, r, 14, c, true, false, false);
    g.setFont (jkit::uf (11.0f)); g.setColour (c); g.drawText (title, juce::Rectangle<float> (r.getX() + 18, r.getY() + 6, 400, 16), juce::Justification::centredLeft);
    g.setFont (jkit::mf (9.5f)); g.setColour (C::dim); g.drawText (sub, juce::Rectangle<float> (r.getRight() - 418, r.getY() + 6, 400, 16), juce::Justification::centredRight);
}
static void grp (juce::Graphics& g, float x, float y, float w, const juce::String& t)
{
    g.setColour (C::dim.withAlpha (0.4f)); g.fillRect (x, y + 12, w, 1.0f);
    g.setFont (jkit::mf (9.0f)); g.setColour (C::dim); g.drawText (t, juce::Rectangle<float> (x, y, w, 12), juce::Justification::centredLeft);
}

Editor::Editor (Processor& p) : AudioProcessorEditor (p), proc (p)
{
    skin::init ("guitar", { 0xffe9a260, 0xff8fd8b8, 0xffc9b6ff, 0xfff4d07a }, 0xffffb02e); C::sync();
    undoMark.p = &p; addMouseListener (&undoMark, true); setWantsKeyboardFocus (true);
    auto add = [&] (const char* id, const char* lab, juce::Colour c, std::function<juce::String (float)> f, const char* tip)
    {
        auto* prm = proc.apvts.getParameter (id);
        auto* k = knobs.add (new jkit::Knob (prm, c, lab)); k->text = [prm, f] { return f (prm->convertFrom0to1 (prm->getValue())); };
        k->setName (id); k->setHelpText (U (tip)); addAndMakeVisible (k); return k;
    };
    auto pct = [] (float v) { return v < 0.005f ? juce::String ("OFF") : juce::String (juce::roundToInt (v * 100)) + "%"; };
    add ("density", "DENSITA'", C::amber, pct, "Quante note. Nell'ARPEGGIO ogni tacca cambia il numero di note: sotto il 50% toglie quelle meno importanti (resta il battere), sopra aggiunge riempimenti leggeri fino ai sedicesimi pieni.");
    add ("dyn", "DINAMICA", C::amber, pct, "Differenza fra colpi accentati e leggeri.");
    add ("strum", "PENNATA", C::amber, [] (float v) { return juce::String (juce::roundToInt (7 + v * 34)) + " ms"; }, "Velocita' della pennata: quanto tempo passa dalla prima all'ultima corda.");
    add ("human", "UMANO", C::amber, pct, "Piccole imprecisioni di tempo e di forza, come un chitarrista vero.");
    add ("swing", "SWING", C::amber, pct, "Ritarda i levare (sedicesimi o ottavi): groove R&B, lo-fi, afro.");
    add ("rest", "RESPIRI", C::amber, pct, "Spazi e vuoti: negli arpeggi alcuni battiti diventano una nota lunga che suona, un vuoto o una ripresa verso il battito dopo; a fine frase la parte si ferma o resta una nota lunga. In sedicesimi c'e' sempre almeno un respiro per battuta.");
    add ("ring", "RISUONA", C::green, pct, "Negli arpeggi: quanto lasciare suonare le corde prima della nota dopo.");
    add ("emb", "ABBELLIM.", C::green, pct, "Hammer-on e scivolate dalle note della scala, negli arpeggi e negli accordi neo soul.");
    add ("tech", "TECNICA", C::green, pct, "Nel LEAD: bend, vibrato, scivolate, legati e doppie note.");
    add ("borrow", "INTERSCAMBIO", C::red, [] (float v) { return v < 0.005f ? juce::String ("OFF") : juce::String (juce::roundToInt (v * 100)) + "%"; }, "Accordi presi in prestito (il V maggiore in minore, il iv minore in maggiore). Spento: la scala si rispetta sempre.");
    add ("vary", "VARIAZIONE", C::red, [] (float v) { return v < 0.005f ? juce::String ("OFF") : juce::String (juce::roundToInt (v * 100)) + "%"; }, "Quanto ogni battuta cambia rispetto alla prima della frase: colpi che saltano o si spostano, ottave, ripetute, dinamica che respira e un'anacrusi a fine frase. A zero il pattern si ripete identico.");
    add ("pos", "POSIZIONE", C::violet, [] (float v) { return v < 0.5f ? juce::String ("APERTA") : juce::String ("TASTO ") + juce::String (juce::roundToInt (v)); }, "Dove sta la mano sul manico: accordi e lead si cercano attorno a questo tasto.");
    add ("color", "COLORE BASE", C::violet, [] (float v) { static const char* n[5] = { "AUTO", "TRIADI", "SETTIME", "NONE", "SUS/ADD9" }; return juce::String (n[juce::jlimit (0, 4, juce::roundToInt (v))]); }, "Colore di partenza per tutti gli accordi (AUTO segue genere e mood). Ogni accordo puo' avere il suo: clic sull'accordo, voce COLORE.");
    add ("tone", "TONO", C::phos, pct, "Brillantezza della corda e del suono.");
    add ("bodyAmt", "CORPO", C::phos, pct, "Quanto si sente la cassa (acustiche) o il carattere del pickup (elettriche).");
    add ("drive", "DRIVE", C::phos, pct, "Distorsione del CRUNCH.");
    add ("space", "SPAZIO", C::phos, pct, "Stanza (acustiche) o plate (elettriche).");
    add ("width", "LARGHEZZA", C::phos, pct, "Raddoppio: una seconda chitarra dall'altra parte, come nei dischi.");
    add ("vol", "VOLUME", C::text, [] (float v) { return v <= -35.9f ? juce::String ("-INF") : juce::String (v, 1) + " dB"; }, "Volume della chitarra interna (il MIDI non cambia).");
    addAndMakeVisible (sizeChip);
    setupBrowser();
    scaler.setup();
    startTimerHz (30);
}
static jkit::Knob* knobById (juce::OwnedArray<jkit::Knob>& K, const char* id) { for (auto* k : K) if (k->getName() == id) return k; return nullptr; }
void Editor::resized()
{
    const float s = scaler.update();
    for (int i = 0; i < 12; ++i) keyR[i] = { 44.0f + i * 47, 114, 43, 24 };
    for (int i = 0; i < kScales; ++i) scaleR[i] = { 44.0f + i * 94, 146, 90, 24 };
    genreR = { 44, 186, 106, 24 }; moodR = { 156, 186, 106, 24 };
    for (int i = 0; i < 4; ++i) barsR[i] = { 276.0f + i * 36, 186, 33, 24 };
    for (int i = 0; i < 3; ++i) perBarR[i] = { 428.0f + i * 38, 186, 34, 24 };
    remR = { 560, 186, 28, 24 }; addR = { 592, 186, 28, 24 };
    for (int i = 0; i < 8; ++i) slotR[i] = { 44.0f + i * 73, 228, 68, 66 };
    fretR = { 672, 134, 470, 120 }; tuningR = { 1052, 100, 90, 24 };
    for (int i = 0; i < 5; ++i) octR[i] = { 882.0f + i * 33, 100, 30, 24 };
    for (int i = 0; i < kVStyles; ++i) vstyleR[i] = { 672.0f + i * 95, 270, 90, 24 };
    // modo come schede in alto nel pannello, il piano roll si prende tutta la larghezza
    for (int i = 0; i < kModes; ++i) modeR[i] = { 112.0f + i * 122, 324, 116, 26 };
    for (int i = 0; i < 12; ++i) patR[i] = { 84.0f + (i % 6) * 127, 360.0f + (i / 6) * 30, 121, 26 };
    for (int i = 0; i < 3; ++i) { layR[i] = { 84.0f + i * 254, 360, 210, 26 }; diceR[i] = { 298.0f + i * 254, 360, 32, 26 }; editLayR[i] = { 180.0f + i * 80, 392, 76, 22 }; }
    rollR = { 84, 440, 760, 206 };
    styleR = { 84, 360, 170, 26 }; figR = { 262, 360, 236, 26 }; figDiceR = { 502, 360, 30, 26 }; ritR = { 540, 360, 266, 26 }; ritDiceR = { 810, 360, 34, 26 };
    gridBtnR = { 700, 392, 144, 24 }; rotR = { 520, 392, 96, 24 }; writeR = { 620, 392, 76, 24 };
    dragR = { 866, 360, 272, 56 };
    for (int i = 0; i < 3; ++i) layDragR[i] = { 866.0f + i * 92, 420, 88, 24 };
    genR = { 866, 452, 88, 30 }; varR = { 958, 452, 88, 30 }; rndR = { 1050, 452, 88, 30 };
    playR = { 866, 490, 134, 26 }; liveR = { 1004, 490, 134, 26 }; midiR = { 866, 522, 134, 26 }; csR = { 1004, 522, 134, 26 };
    snapR = { 866, 568, 134, 24 }; resetEditR = { 1004, 568, 134, 24 };
    // in grande quello che cambia la musica (FEEL), piu' piccoli i dettagli della mano, piccolissimo il suono d'ascolto
    auto put = [this] (const char* id, int x, int y, int w, int h) { if (auto* k = knobById (knobs, id)) k->setBounds (x, y, w, h); };
    int x = 44; for (auto* id : { "density", "rest", "vary", "human", "swing", "dyn" }) { put (id, x, 744, 78, 104); x += 84; }
    x = 566; for (auto* id : { "strum", "ring", "emb", "tech", "borrow", "pos", "color" }) { put (id, x, 748, 60, 82); x += 66; }
    for (int i = 0; i < kBodies; ++i) bodyR[i] = { 566.0f + i * 70, 862, 66, 22 };
    listenR = { 566, 890, 130, 20 };
    x = 862; for (auto* id : { "tone", "bodyAmt", "drive", "space", "width", "vol" }) { put (id, x, 846, 46, 66); x += 47; }
    undoR = { 700, 22, 70, 32 }; redoR = { 774, 22, 70, 32 }; themeR = { 1066, 24, 90, 28 };
    prevR = { 316, 22, 28, 32 }; presetR = { 346, 22, 316, 32 }; nextR = { 664, 22, 28, 32 };
    browser.setBounds (0, 0, W, H);
    sizeChip.setBounds (W - 76, H - 16, 58, 14);
    bg = {};
    jlook::scaleChildren (*this, s);
}
void Editor::paintStatic (juce::Graphics& g)
{
    const float Wf = (float) W, Hf = (float) H;
    jlook::backdrop (g, { 0, 0, Wf, Hf }, C::amber, C::green);
    jlook::header (g, { 0, 0, Wf, 72 }, "GUITAR", C::amber, U ("GIRI DI CHITARRA  /  ACCORDI RITMICA ARPEGGIO LEAD"), C::green, C::violet);
    panel (g, { 24, 88, 604, 214 }, C::amber, U ("GIRO"), U ("TONALITA' / SCALA / GENERE / ACCORDI"));
    panel (g, { 644, 88, 512, 214 }, C::green, U ("TASTIERA"), U (""));
    panel (g, { 24, 318, 1132, 386 }, C::violet, U ("CLIP"), U (""));
    panel (g, { 24, 716, 1132, 202 }, C::phos, U (""), U (""));
grp (g, 276, 172, 84, U ("BATTUTE")); grp (g, 428, 172, 110, U ("ACC. PER BATT.")); grp (g, 560, 172, 60, U ("ACCORDI")); grp (g, 156, 172, 106, U ("MOOD"));
    grp (g, 44, 172, 106, U ("GENERE")); grp (g, 44, 213, 580, U ("GIRO (CLIC PER CAMBIARE L'ACCORDO)"));
    grp (g, 44, 728, 498, U ("FEEL: QUANTE NOTE, RESPIRI, VARIAZIONE, MANO UMANA"));
    grp (g, 566, 728, 456, U ("DETTAGLI: PENNATA, CORDE, ABBELLIMENTI, ARMONIA, MANO"));
    grp (g, 566, 846, 280, U ("SUONO D'ASCOLTO (IL MIDI NON CAMBIA)")); grp (g, 862, 834, 280, U (""));
}
void Editor::drawFretboard (juce::Graphics& g, const Part& P, int slot)
{
    const auto r = fretR; const int NF = 15; const auto tun = tuning (proc.vi ("tuning"));
    const float fw = (r.getWidth() - 34) / NF, sh = r.getHeight() / (kStr - 1);
    auto fx = [&] (int f) { return f == 0 ? r.getX() + 12 : r.getX() + 34 + (f - 0.5f) * fw; };
    auto sy = [&] (int s) { return r.getBottom() - s * sh; };   // corda grave in basso, come la si guarda suonando
    g.setColour (C::bg.withAlpha (0.55f)); g.fillRoundedRectangle (r.expanded (8, 10), 8);
    for (int f = 0; f <= NF; ++f) { const float x = r.getX() + 34 + f * fw; g.setColour (f == 0 ? C::text.withAlpha (0.7f) : C::dim.withAlpha (0.35f)); g.fillRect (x - (f == 0 ? 1.5f : 0.5f), r.getY(), f == 0 ? 3.0f : 1.0f, r.getHeight()); }
    for (int f : { 3, 5, 7, 9, 15 }) { g.setColour (C::dim.withAlpha (0.25f)); g.fillEllipse (fx (f) - 4, r.getCentreY() - 4, 8, 8); }
    g.setColour (C::dim.withAlpha (0.25f)); g.fillEllipse (fx (12) - 4, r.getY() + sh * 1.5f - 4, 8, 8); g.fillEllipse (fx (12) - 4, r.getBottom() - sh * 1.5f - 4, 8, 8);
    g.setFont (jkit::mf (8.5f)); g.setColour (C::faint);
    for (int f = 1; f <= NF; ++f) g.drawText (juce::String (f), juce::Rectangle<float> (fx (f) - 10, r.getBottom() + 6, 20, 10), juce::Justification::centred);
    for (int s = 0; s < kStr; ++s) { g.setColour (C::text.withAlpha (0.25f + 0.04f * (kStr - s))); g.fillRect (r.getX() + 34, sy (s) - 0.5f - (kStr - s) * 0.12f, r.getWidth() - 34, 1.0f + (kStr - s) * 0.25f); g.setColour (C::dim); g.drawText (noteName (tun[(size_t) s]), juce::Rectangle<float> (r.getX() - 14, sy (s) - 6, 14, 12), juce::Justification::centredLeft); }
    if (slot < 0 || slot >= (int) P.voic.size()) return;
    const auto& ch = P.chords[(size_t) slot];
    if (proc.vi ("mode") == LEAD)
    {   // nel lead: le note della frase in questo accordo
        std::set<std::pair<int, int>> seen;
        for (auto& e : P.ev) if (e.slot == slot && e.fret <= NF && seen.insert ({ e.str, e.fret }).second)
        {
            const float x = fx (e.fret), y = sy (e.str); bool ct = false; for (int t : ch.tones) ct |= t == e.note % 12;
            g.setColour (ct ? C::amber : C::green.withAlpha (0.8f)); g.fillEllipse (x - 8, y - 8, 16, 16);
            g.setColour (C::bg); g.setFont (jkit::uf (7.5f)); g.drawText (noteName (e.note % 12), juce::Rectangle<float> (x - 9, y - 6, 18, 12), juce::Justification::centred);
        }
        return;
    }
    const auto& v = P.voic[(size_t) slot];
    if (v.barre && v.lo > 0)
    {
        int s0 = -1, s1 = -1; for (int s = 0; s < kStr; ++s) if (v.fret[(size_t) s] == v.lo) { if (s0 < 0) s0 = s; s1 = s; }
        for (int s = s0; s < kStr; ++s) if (v.fret[(size_t) s] >= v.lo) s1 = s;
        g.setColour (C::amber.withAlpha (0.45f)); g.fillRoundedRectangle (fx (v.lo) - 7, sy (s1) - 8, 14, sy (s0) - sy (s1) + 16, 7);
    }
    for (int s = 0; s < kStr; ++s)
    {
        const int f = v.fret[(size_t) s]; const float y = sy (s);
        if (f < 0) { g.setColour (C::red.withAlpha (0.8f)); g.setFont (jkit::uf (11)); g.drawText ("x", juce::Rectangle<float> (r.getX() + 4, y - 7, 16, 14), juce::Justification::centred); continue; }
        const int pc = (tun[(size_t) s] + f) % 12; const bool root = pc == ch.root;
        const float x = fx (f);
        if (f == 0) { g.setColour (root ? C::amber : C::text); g.drawEllipse (x - 6, y - 6, 12, 12, 2.0f); continue; }
        g.setColour (root ? C::amber : strCol (s)); g.fillEllipse (x - 9, y - 9, 18, 18);
        g.setColour (C::bg); g.setFont (jkit::uf (7.5f)); g.drawText (noteName (pc), juce::Rectangle<float> (x - 10, y - 6, 20, 12), juce::Justification::centred);
    }
}
static juce::Colour layerCol (int l) { return l == 0 ? C::amber : l == 1 ? C::green : C::violet; }
void Editor::drawRoll (juce::Graphics& g, const Part& P)
{
    const auto r = rollR; const double L = P.len(); const bool comp = proc.vi ("mode") == COMPOSIZIONE;
    g.setColour (C::bg.withAlpha (0.75f)); g.fillRoundedRectangle (r.expanded (4).withLeft (44).withBottom (r.getBottom() + 50), 8);
    if (! editing)
    {   // estensione verticale: si fissa durante una modifica
        int lo = 127, hi = 0; for (auto& e : P.ev) { lo = juce::jmin (lo, e.note); hi = juce::jmax (hi, e.note); }
        if (lo > hi) { lo = 48; hi = 72; }
        rollLo = lo - 3; rollHi = hi + 3; if (rollHi - rollLo < 18) { const int c = (rollLo + rollHi) / 2; rollLo = c - 9; rollHi = c + 9; }
    }
    // righe: con un'estensione ampia restano solo le note della scala (piu' quelle usate fuori scala), cosi' le note restano grandi
    const auto S = proc.settings(); const auto sset = scaleSet (S.key, S.scale);
    if (! editing || rows.empty())
    {
        rows.clear(); folded = r.getHeight() / (float) (rollHi - rollLo + 1) < 9.0f;
        std::array<bool, 128> used {}; for (auto& e : P.ev) used[(size_t) juce::jlimit (0, 127, e.note)] = true;
        for (int n = rollLo; n <= rollHi; ++n) if (! folded || sset[(size_t) (n % 12)] || used[(size_t) n]) rows.push_back (n);
    }
    const float nh = rowH();
    for (size_t i = 0; i < rows.size(); ++i)
    {
        const int n = rows[i]; const float y = ny (n);
        if (folded ? (i % 2 == 0) : sset[(size_t) (n % 12)]) { g.setColour (C::text.withAlpha (n % 12 == S.key ? 0.075f : 0.035f)); g.fillRect (juce::Rectangle<float> ((float) r.getX(), y, (float) r.getWidth(), nh).toNearestInt()); }
        if (n % 12 == S.key) { g.setColour (C::amber.withAlpha (0.10f)); g.fillRect (r.getX(), (float) juce::roundToInt (y + nh) - 1.0f, r.getWidth(), 1.0f); }
    }
    for (int b = 0; b * 4.0 < L - 1e-6; ++b) { if (b % 2) { g.setColour (C::text.withAlpha (0.018f)); g.fillRect (juce::Rectangle<float> (bx (b * 4.0, L), (float) r.getY(), bx ((b + 1) * 4.0, L) - bx (b * 4.0, L), (float) r.getHeight()).toNearestInt()); } g.setFont (jkit::mf (9.0f)); g.setColour (C::faint); g.drawText (juce::String (b + 1), juce::Rectangle<float> (bx (b * 4.0, L) + 4, r.getY() + 1, 20, 11), juce::Justification::centredLeft); }
    // a sinistra: tastiera, o il nome delle note quando le righe sono solo quelle della scala
    {
        const float kx = 46, kw = r.getX() - kx - 4;
        static const char* nn[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        for (int n : rows)
        {
            const int pc = n % 12; const bool blk = pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10; const float y = ny (n);
            const auto kr = juce::Rectangle<float> (kx, y + 0.5f, kw, juce::jmax (1.0f, nh - 1.0f));
            if (folded)
            {
                g.setColour (pc == S.key ? C::amber.withAlpha (0.22f) : C::text.withAlpha (0.06f)); g.fillRoundedRectangle (kr, 2.0f);
                if (nh >= 7) { g.setFont (jkit::mf (juce::jmin (10.0f, nh))); g.setColour (pc == S.key ? C::amber : sset[(size_t) pc] ? C::dim : C::red);
                               g.drawText (juce::String (nn[pc]) + juce::String (n / 12 - 1), kr.reduced (4, 0), juce::Justification::centredRight); }
                continue;
            }
            g.setColour (blk ? juce::Colour (0xff15161a) : juce::Colour (0xffd9d6cf).withAlpha (0.85f)); g.fillRect (kx, y + 0.5f, blk ? kw * 0.62f : kw, juce::jmax (1.0f, nh - 1.0f));
            if (pc == S.key) { g.setColour (C::amber.withAlpha (0.9f)); g.fillRect (kx + kw - 4, y + 0.5f, 4.0f, juce::jmax (1.0f, nh - 1.0f)); }
            if (pc == 0 && nh >= 6) { g.setColour (juce::Colour (0xff2a2a2e)); g.setFont (jkit::mf (juce::jmin (9.0f, nh + 1))); g.drawText ("C" + juce::String (n / 12 - 1), juce::Rectangle<float> (kx + 2, y - 1, kw - 6, nh + 2), juce::Justification::centredRight); }
        }
    }
    const double gd = grid() > 0 ? grid() : 0.25;
    for (double b = 0; b <= L + 1e-6; b += gd) { const bool bar = std::abs (std::fmod (b, 4.0)) < 1e-6, beat = std::abs (b - std::round (b)) < 1e-6; g.setColour (C::dim.withAlpha (bar ? 0.5f : beat ? 0.2f : 0.07f)); g.fillRect ((float) juce::roundToInt (bx (b, L)), (float) r.getY(), bar ? 1.2f : 1.0f, (float) r.getHeight()); }
    // accordi sopra: si cliccano
    chordLabels.clear(); g.setFont (jkit::uf (9.5f));
    for (size_t i = 0; i < P.chords.size(); ++i)
    {
        const float x = bx (P.chordAt[i], L), x2 = i + 1 < P.chords.size() ? bx (P.chordAt[i + 1], L) : r.getRight();
        juce::Rectangle<float> cr (x + 1, r.getY() - 20, juce::jmax (20.0f, x2 - x - 2), 16);
        const bool hov = cr.contains (getMouseXYRelative().toFloat() / scaler.s);
        g.setColour ((P.chords[i].borrowed ? C::red : C::amber).withAlpha (hov ? 0.35f : 0.12f)); g.fillRoundedRectangle (cr, 4);
        g.setColour (P.chords[i].borrowed ? C::red : C::amber); g.drawText (P.chords[i].name, cr.reduced (4, 0), juce::Justification::centredLeft);
        chordLabels.push_back ({ cr, (int) i });
    }
    resolveSel (P);
    std::vector<bool> isSel (P.ev.size(), false); for (int i : sel) if (i >= 0 && i < (int) P.ev.size()) isSel[(size_t) i] = true;
    for (size_t i = 0; i < P.ev.size(); ++i)
    {
        const auto& e = P.ev[i]; const auto nr = noteRect (e, L);
        const auto c = comp ? layerCol (e.layer) : strCol (e.str);
        const float rr = juce::jmin (3.0f, nr.getHeight() * 0.4f);
        g.setColour (c.withAlpha (e.mute ? 0.25f : 0.35f + 0.6f * e.vel)); g.fillRoundedRectangle (nr, rr);
        g.setColour (c.brighter (0.5f).withAlpha (e.mute ? 0.3f : 0.9f)); g.fillRect (nr.getX() + 1, nr.getY(), juce::jmin (3.0f, nr.getWidth() - 1), nr.getHeight());   // l'attacco si vede
        g.setColour (juce::Colours::black.withAlpha (0.35f)); g.drawRoundedRectangle (nr, rr, 0.8f);
        if (isSel[i]) { g.setColour (C::text); g.drawRoundedRectangle (nr.expanded (1.5f), rr + 1, 1.5f); }
        // corsia della forza sotto il piano roll
        const float vx = nr.getX(), vh = 38.0f * e.vel, vy = r.getBottom() + 46;
        g.setColour (c.withAlpha (0.75f)); g.fillRect (vx, vy - vh, 2.0f, vh); g.fillEllipse (vx - 1.5f, vy - vh - 2, 5, 5);
    }
    if (! P.bend.empty())
    {   // curva dei bend
        juce::Path pb; bool st = false; float py = 0; for (auto& b : P.bend) { const float x = bx (b.beat, L), y = r.getBottom() - 8 - b.semis * 6; if (! st) { pb.startNewSubPath (x, y); st = true; } else { pb.lineTo (x, py); pb.lineTo (x, y); } py = y; }
        g.setColour (C::green.withAlpha (0.6f)); g.strokePath (pb, juce::PathStrokeType (1.2f));
    }
    if (editMode == 3) { g.setColour (C::text.withAlpha (0.15f)); g.fillRect (marquee); g.setColour (C::text.withAlpha (0.6f)); g.drawRect (marquee, 1.0f); }
    const double pb = proc.uiBeat.load();
    if (pb >= 0) { const float px = bx (pb, L); g.setColour (C::text.withAlpha (0.15f)); g.fillRect (px - 3, r.getY(), 7.0f, r.getHeight()); g.setColour (C::text); g.fillRect (px, r.getY() - 2, 1.5f, r.getHeight() + 50); }
    g.setColour (C::dim.withAlpha (0.25f)); g.fillRect (r.getX(), r.getBottom() + 7, r.getWidth(), 1.0f);
    g.setFont (jkit::mf (7.5f)); g.setColour (C::faint); g.drawText (U ("FORZA"), juce::Rectangle<float> (46, r.getBottom() + 18, 34, 12), juce::Justification::centredLeft);
}
juce::Rectangle<float> Editor::gridCell (int row, int k) const
{
    const float lx = rollR.getX() + 70, cw = (rollR.getWidth() - 74) / 16.0f, ch = (rollR.getHeight() - 20) / 5.0f;
    return { lx + k * cw, rollR.getY() + 10 + row * ch, cw - 3, ch - 4 };
}
void Editor::drawGrid (juce::Graphics& g)
{
    g.setColour (C::bg.withAlpha (0.6f)); g.fillRoundedRectangle (rollR.expanded (4), 6);
    const auto gr = proc.grid(); static const char* lab[5] = { "1 ACUTA", "2", "3", "4", "BASSO" };
    for (int r = 0; r < 5; ++r)
    {
        const auto c0 = gridCell (r, 0);
        g.setFont (jkit::uf (9.5f)); g.setColour (r == 4 ? C::amber : C::green); g.drawText (lab[r], juce::Rectangle<float> (rollR.getX() + 6, c0.getY(), 60, c0.getHeight()), juce::Justification::centredLeft);
        for (int k = 0; k < 16; ++k)
        {
            const auto c = gridCell (r, k); const bool on = gr[(size_t) r] & (1 << k);
            g.setColour (on ? (r == 4 ? C::amber : C::green) : C::dim.withAlpha (k % 4 == 0 ? 0.25f : 0.12f));
            g.fillRoundedRectangle (c, 3);
        }
    }
    const double pb = proc.uiBeat.load();
    if (pb >= 0) { const int k = (int) (std::fmod (pb, 4.0) * 4.0); const auto c = gridCell (0, k); g.setColour (C::text.withAlpha (0.5f)); g.fillRect (c.getX(), rollR.getY() + 4, 2.0f, rollR.getHeight() - 8); }
    g.setFont (jkit::mf (9)); g.setColour (C::dim);
    g.drawText (U ("una battuta in sedicesimi, ripetuta. La figura segue da sola gli accordi del giro."), juce::Rectangle<float> (rollR.getX() + 70, rollR.getBottom() - 12, rollR.getWidth() - 74, 12), juce::Justification::centredLeft);
}
void Editor::styleMenu()
{
    juce::PopupMenu m; m.addSectionHeader (U ("STILI (FIGURA + RITMO PRONTI)"));
    const auto& pn = patternNames (ARPEGGIO);
    for (int i = 0; i < pn.size(); ++i) { const auto fr = arpStyle (i); m.addItem (1 + i, pn[i] + "   (" + figOf (fr.first).name + " / " + ritOf (fr.second).name + ")"); }
    juce::Component::SafePointer<Editor> sp (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withTargetScreenArea (localAreaToGlobal (scaler.map (styleR.toNearestInt()))), [sp] (int r)
    { if (sp == nullptr || r <= 0) return; sp->proc.undo.beginNewTransaction(); sp->proc.setV ("pat", (float) (r - 1)); sp->proc.applyArpStyle (r - 1); sp->proc.curPreset = -1; sp->repaint(); });
}
void Editor::figMenu()
{
    juce::PopupMenu m; m.addSectionHeader (U ("FIGURA: L'ORDINE DELLE CORDE (1 = LA PIU' ACUTA, B = BASSO)"));
    for (int i = 0; i < kFigs; ++i)
    {
        const juce::String d = i == kFigs - 1 ? U ("   (la disegni nella griglia)") : i == kFigLibera ? U ("   (la scrivi tu: ") + proc.apvts.state.getProperty ("figSeq", "1 3 4 2").toString() + ")" : juce::String ("   ") + figOf (i).seq;
        if (i == kFigLibera) m.addSeparator();
        m.addItem (1 + i, juce::String (figOf (i).name) + d, true, proc.vi ("arpFig") == i);
    }
    m.addItem (900, U ("FIGURA A CASO (NUOVA)"));
    juce::Component::SafePointer<Editor> sp (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withTargetScreenArea (localAreaToGlobal (scaler.map (figR.toNearestInt()))), [sp] (int r)
    {
        if (sp == nullptr || r <= 0) return;
        if (r == 900) { sp->randomFig(); return; }
        if (r - 1 == kFigLibera) { sp->askFigure(); return; }
        sp->proc.undo.beginNewTransaction(); sp->proc.setV ("arpFig", (float) (r - 1)); sp->proc.setV ("arpRot", 0); if (r - 1 == kFigs - 1) sp->gridEdit = true; sp->proc.curPreset = -1; sp->repaint();
    });
}
void Editor::randomFig()
{
    // meta' delle volte una figura della lista, meta' una figura nuova (puo' partire da qualsiasi corda)
    auto& R = juce::Random::getSystemRandom(); proc.undo.beginNewTransaction();
    if (R.nextBool()) { int f = proc.vi ("arpFig"), n = f; while (n == f) n = R.nextInt (kFigLibera); proc.setV ("arpFig", (float) n); }
    else proc.setFigSeq (randomFigure (R));
    proc.setV ("arpRot", 0); proc.curPreset = -1; gridEdit = false; repaint();
}
void Editor::askFigure()
{
    auto* w = new juce::AlertWindow (U ("Scrivi la figura"), U ("Le corde dell'accordo nell'ordine in cui le suoni, separate da spazi.\n1 = la piu' acuta, 2, 3, 4 scendendo, B = basso, A = basso alternato.\nDue cifre attaccate = insieme (es. B1). Esempi: 1 3 4 2   3 B 2 1   2 4 1 3 B"), juce::MessageBoxIconType::NoIcon, this);
    w->addTextEditor ("fig", proc.apvts.state.getProperty ("figSeq", "1 3 4 2").toString());
    w->addButton ("OK", 1, juce::KeyPress (juce::KeyPress::returnKey)); w->addButton ("ANNULLA", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    juce::Component::SafePointer<Editor> sp (this);
    w->enterModalState (true, juce::ModalCallbackFunction::create ([sp, w] (int r)
    {
        if (sp == nullptr || r != 1) return;
        juce::StringArray t = juce::StringArray::fromTokens (w->getTextEditorContents ("fig").toUpperCase(), " ,-", "");
        for (auto& x : t) x = x.retainCharacters ("1234BA");
        t.removeEmptyStrings(); if (t.isEmpty()) return;
        sp->proc.undo.beginNewTransaction(); sp->proc.setFigSeq (t.joinIntoString (" ")); sp->proc.setV ("arpRot", 0); sp->proc.curPreset = -1; sp->gridEdit = false; sp->repaint();
    }), true);
}
void Editor::ritMenu()
{
    juce::PopupMenu m; m.addSectionHeader (U ("RITMO DELL'ARPEGGIO (DENSITA' TOGLIE O AGGIUNGE NOTE)"));
    for (int i = 0; i < kRits; ++i) m.addItem (1 + i, ritOf (i).name, true, proc.vi ("arpRit") == i);
    juce::Component::SafePointer<Editor> sp (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withTargetScreenArea (localAreaToGlobal (scaler.map (ritR.toNearestInt()))), [sp] (int r)
    { if (sp == nullptr || r <= 0) return; sp->proc.undo.beginNewTransaction(); sp->proc.setV ("arpRit", (float) (r - 1)); sp->proc.curPreset = -1; sp->repaint(); });
}
void Editor::storeSig (const Part& P) { selSig.clear(); for (int i : sel) if (i >= 0 && i < (int) P.ev.size()) selSig.push_back ({ P.ev[(size_t) i].beat, P.ev[(size_t) i].note, P.ev[(size_t) i].layer }); }
void Editor::resolveSel (const Part& P)
{
    sel.clear();
    for (auto& sg : selSig) for (int i = 0; i < (int) P.ev.size(); ++i) { const auto& e = P.ev[(size_t) i]; if (std::abs (e.beat - sg.beat) < 1e-6 && e.note == sg.note && e.layer == sg.layer) { sel.push_back (i); break; } }
}
void Editor::fixString (Ev& e) const
{
    const auto tun = tuning (proc.vi ("tuning")); const int pos = proc.vi ("pos");
    int f = e.note - tun[(size_t) e.str];
    if (f >= 0 && f <= 20) { e.fret = f; return; }
    int best = 0; float bc = 1e9f;
    for (int s = 0; s < kStr; ++s) { const int ff = e.note - tun[(size_t) s]; if (ff < 0 || ff > 20) continue; const float c = std::abs ((float) ff - (float) pos - 2.0f); if (c < bc) { bc = c; best = s; } }
    e.str = best; e.fret = juce::jmax (0, e.note - tun[(size_t) best]);
}
int Editor::scaleShift (int note, int steps) const
{
    const auto S = proc.settings(); const auto ss = scaleSet (S.key, S.scale);
    int n = note; const int dir = steps > 0 ? 1 : -1;
    for (int k = 0; k < std::abs (steps); ++k) { do { n += dir; } while (n > 0 && n < 127 && ! ss[(size_t) (n % 12)]); }
    return juce::jlimit (0, 127, n);
}
int Editor::stepsBetween (int a, int b) const
{
    const auto S = proc.settings(); const auto ss = scaleSet (S.key, S.scale);
    int k = 0; const int dir = b > a ? 1 : -1; for (int n = a; n != b; n += dir) if (ss[(size_t) ((n + dir) % 12)]) k += dir;
    return k;
}
void Editor::commitEdit()
{
    storeSig (edPart);
    std::stable_sort (edPart.ev.begin(), edPart.ev.end(), [] (const Ev& a, const Ev& b) { return a.beat < b.beat; });
    proc.commitEdit (edPart);
    resolveSel (edPart);
    repaint();
}
void Editor::deleteSelected()
{
    auto P = proc.part(); if (P == nullptr) return;
    edPart = *P; resolveSel (edPart); if (sel.empty()) return;
    std::vector<bool> del (edPart.ev.size(), false); for (int i : sel) del[(size_t) i] = true;
    std::vector<Ev> keep; for (size_t i = 0; i < edPart.ev.size(); ++i)
    {
        if (! del[i]) { keep.push_back (edPart.ev[i]); continue; }
        const auto& e = edPart.ev[i];   // via anche i bend di quella nota
        if (e.lead) edPart.bend.erase (std::remove_if (edPart.bend.begin(), edPart.bend.end(), [&] (const Bend& b) { return b.beat > e.beat - 0.02 && b.beat < e.beat + e.len && b.str == e.str; }), edPart.bend.end());
    }
    edPart.ev = keep; sel.clear(); selSig.clear(); commitEdit();
}

void Editor::paint (juce::Graphics& g)
{
    g.addTransform (juce::AffineTransform::scale (scaler.s));
    const float sc = juce::jlimit (1.0f, 4.0f, g.getInternalContext().getPhysicalPixelScaleFactor());
    if (bg.isNull() || std::abs (bgScale - sc) > 0.01f || bgTheme != skin::cur())
    {
        bgTheme = skin::cur(); bgScale = sc; bg = juce::Image (juce::Image::RGB, juce::roundToInt (W * sc), juce::roundToInt (H * sc), false);
        juce::Graphics ig (bg); ig.addTransform (juce::AffineTransform::scale (sc)); paintStatic (ig);
    }
    { g.saveState(); g.addTransform (juce::AffineTransform::scale (1.0f / sc)); g.drawImageAt (bg, 0, 0); g.restoreState(); }
    auto P = proc.part(); const auto S = proc.settings();
    // testata: preset, annulla / ripeti, tema
    jkit::lcd (g, presetR, 8, C::amber); jlook::glass (g, presetR, 8);
    {
        juce::String gname ("I MIEI"); if (proc.curPreset >= 0) gname = juce::String (presets()[(size_t) proc.curPreset].name).upToFirstOccurrenceOf (" | ", false, false);
        g.setFont (jkit::mf (9)); g.setColour (C::dim); g.drawText (gname, presetR.withHeight (12).translated (0, 2), juce::Justification::centred);
        glowText (g, proc.presetName.toUpperCase(), presetR.withTrimmedTop (10), C::amber, mf (12.0f), juce::Justification::centred);
    }
    g.setFont (jkit::uf (16)); g.setColour (C::amber); g.drawText ("<", prevR, juce::Justification::centred); g.drawText (">", nextR, juce::Justification::centred);
    chipBtn (g, undoR, U ("\xe2\x86\xb6 ANNULLA"), false, proc.undo.canUndo() ? C::green : C::dim, 9.5f);
    chipBtn (g, redoR, U ("RIPETI \xe2\x86\xb7"), false, proc.undo.canRedo() ? C::green : C::dim, 9.5f);
    skin::chipDraw (g, themeR, hoverId == 900);
    // GIRO
    for (int i = 0; i < 12; ++i) chipBtn (g, keyR[i], noteName (i), S.key == i, C::amber, 10.5f, hoverId == 100 + i);
    for (int i = 0; i < kScales; ++i) chipBtn (g, scaleR[i], scaleName (i), S.scale == i, C::amber, 10.0f, hoverId == 120 + i);
    chipBtn (g, genreR, juce::String (genreName (S.genre)) + U (" \xe2\x96\xbe"), true, C::yellow, 9.5f, hoverId == 130);
    chipBtn (g, moodR, juce::String (moodName (S.mood)) + U (" \xe2\x96\xbe"), true, C::yellow, 9.5f, hoverId == 132);
    for (int i = 0; i < 4; ++i) chipBtn (g, barsR[i], juce::String (barsOf (i + 1)), proc.vi ("bars") == i + 1, C::amber, 10.5f, hoverId == 140 + i);
    { static const char* pb[3] = { "1", "2", "1/2" }; for (int i = 0; i < 3; ++i) chipBtn (g, perBarR[i], pb[i], proc.vi ("perBar") == i, C::amber, 10.5f, hoverId == 150 + i); }
    const auto pr = proc.prog();
    chipBtn (g, remR, "-", false, pr.size() > 1 ? C::amber : C::dim, 13, hoverId == 160); chipBtn (g, addR, "+", false, pr.size() < 8 ? C::amber : C::dim, 13, hoverId == 161);
    for (int i = 0; i < 8; ++i)
    {
        const auto r = slotR[i];
        if (i >= (int) pr.size()) { g.setColour (C::dim.withAlpha (0.15f)); g.drawRoundedRectangle (r.reduced (2), 8, 1.0f); continue; }
        const auto c = slotChord (S, pr[(size_t) i]);
        const bool playingHere = P != nullptr && proc.uiSlot.load() >= 0 && (proc.uiSlot.load() % (int) pr.size()) == i && proc.uiPlaying.load();
        const auto col = c.borrowed ? C::red : C::amber;
        // il nome lo disegna il bottone (colore giusto in ogni tema, anche acceso); sopra il grado
        jlook::button (g, r.withTrimmedTop (22), col, playingHere || selSlot == i, hoverSlot == i, c.name, jkit::uf (c.name.length() > 6 ? 10.5f : 13.0f));
        g.setFont (jkit::uf (9.0f)); g.setColour (c.borrowed ? C::red : C::dim); g.drawText (degreeRoman (pr[(size_t) i], S.scale, S.key) + (c.borrowed ? U (" prestito") : U ("")), r.withHeight (20), juce::Justification::centred);
    }
    // TASTIERA
    {
        const int sl = proc.uiPlaying.load() && proc.uiSlot.load() >= 0 ? proc.uiSlot.load() : (P != nullptr ? juce::jlimit (0, (int) P->chords.size() - 1, selSlot) : 0);
        if (P != nullptr && sl < (int) P->chords.size())
        {
            const auto& c = P->chords[(size_t) sl];
            glowText (g, c.name, { 672, 106, 120, 24 }, c.borrowed ? C::red : C::amber, jkit::uf (18), juce::Justification::centredLeft);
            juce::String tones; for (int t : c.tones) tones << noteName (t) << "  ";
            g.setFont (jkit::mf (10)); g.setColour (C::dim); g.drawText (tones.trim() + (c.borrowed ? U ("   (preso in prestito)") : U ("")), juce::Rectangle<float> (790, 109, 250, 18), juce::Justification::centredLeft);
            drawFretboard (g, *P, sl);
        }
        chipBtn (g, tuningR, tuningName (proc.vi ("tuning")), true, C::green, 9.5f, hoverId == 170);
        { g.setFont (jkit::mf (9.0f)); g.setColour (C::dim); g.drawText (U ("OTTAVA"), juce::Rectangle<float> (826, 100, 52, 24), juce::Justification::centredRight);
          static const char* on[5] = { "-2", "-1", "0", "+1", "+2" }; for (int i = 0; i < 5; ++i) chipBtn (g, octR[i], on[i], proc.vi ("octave") == i - 2, C::green, 9.5f, hoverId == 190 + i); }
        for (int i = 0; i < kVStyles; ++i) chipBtn (g, vstyleR[i], vstyleName (i), proc.vi ("vstyle") == i, C::green, 9.5f, hoverId == 180 + i);
    }
    // CLIP
    {
        const int m = proc.vi ("mode");
        for (int i = 0; i < kModes; ++i) chipBtn (g, modeR[i], modeName (i), m == i, C::violet, 12.5f, hoverId == 200 + i);
        if (m == COMPOSIZIONE)
        {   // tre strati: BASE, ARPEGGIO, LEAD (menu + dado per rifare solo quello strato)
            static const char* ids[3] = { "lyBase", "lyArp", "lyLead" }; static const char* nm[3] = { "BASE", "ARPEGGIO", "LEAD" };
            for (int i = 0; i < 3; ++i)
            {
                auto* prm = dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter (ids[i]));
                juce::String cur = prm != nullptr ? prm->getCurrentChoiceName() : juce::String();
                if (i == 1 && cur != "OFF") cur = juce::String (figOf (proc.vi ("arpFig")).name) + " / " + ritOf (proc.vi ("arpRit")).name;
                chipBtn (g, layR[i], juce::String (nm[i]) + ": " + cur + U (" \xe2\x96\xbe"), cur != "OFF", layerCol (i), 8.8f, hoverId == 240 + i);
                chipBtn (g, diceR[i], U ("\xe2\x86\xbb"), false, layerCol (i), 12, hoverId == 243 + i);
                chipBtn (g, editLayR[i], nm[i], editLayer == i, layerCol (i), 8.5f, hoverId == 250 + i);
            }
            g.setFont (jkit::mf (9)); g.setColour (C::dim); g.drawText (U ("NOTE NUOVE IN:"), juce::Rectangle<float> (84, 392, 94, 22), juce::Justification::centredLeft);
        }
        else if (m == ARPEGGIO)
        {   // figura delle corde e ritmo, separati; lo stile e' una combinazione pronta
            const int fg = proc.vi ("arpFig"), rt = proc.vi ("arpRit");
            chipBtn (g, styleR, U ("STILI \xe2\x96\xbe"), false, C::violet, 9.5f, hoverId == 280);
            chipBtn (g, figR, juce::String ("FIGURA: ") + figOf (fg).name + U (" \xe2\x96\xbe"), true, C::green, 9.0f, hoverId == 281);
            chipBtn (g, figDiceR, U ("\xe2\x86\xbb"), false, C::green, 12, hoverId == 282);
            chipBtn (g, ritR, juce::String ("RITMO: ") + ritOf (rt).name + U (" \xe2\x96\xbe"), true, C::amber, 9.0f, hoverId == 283);
            chipBtn (g, ritDiceR, U ("\xe2\x86\xbb"), false, C::amber, 12, hoverId == 284);
            g.setFont (jkit::mf (10)); g.setColour (C::dim);
            const juce::String fs = fg == kFigLibera ? proc.apvts.state.getProperty ("figSeq", "1 3 4 2").toString() : juce::String (figOf (fg).seq);
            const juce::String seq = fg == kFigs - 1 ? U ("figura disegnata nella griglia") : U ("corde: ") + fs + U ("  (1 = acuta, B = basso)");
            g.drawText (seq, juce::Rectangle<float> (204, 376, 312, 24), juce::Justification::centredLeft);
            chipBtn (g, rotR, juce::String ("PARTENZA ") + juce::String (proc.vi ("arpRot") + 1), proc.vi ("arpRot") > 0, C::green, 9, hoverId == 286);
            chipBtn (g, writeR, U ("SCRIVI"), fg == kFigLibera, C::green, 9, hoverId == 287);
            chipBtn (g, gridBtnR, gridEdit ? U ("TORNA AL PIANO ROLL") : U ("DISEGNA LA FIGURA"), gridEdit, C::green, 9, hoverId == 285);
        }
        else
        {
            const auto& pn = patternNames (m); const int pat = juce::jlimit (0, pn.size() - 1, proc.vi ("pat"));
            for (int i = 0; i < 12; ++i) if (i < pn.size()) chipBtn (g, patR[i], pn[i], pat == i, C::violet, 9.0f, hoverId == 210 + i);
        }
        if (gridEdit && (m == ARPEGGIO || m == COMPOSIZIONE)) drawGrid (g);
        else if (P != nullptr) drawRoll (g, editing ? edPart : *P);
        // trascina
        {
            const auto c = dragging ? C::green : C::amber;
            jlook::button (g, dragR, c, dragging, hoverId == 230, "", jkit::uf (10));
            g.setFont (jkit::uf (13)); g.setColour (c); g.drawText (m == COMPOSIZIONE ? U ("TRASCINA TUTTO") : U ("TRASCINA IL CLIP MIDI"), dragR.withHeight (30).translated (0, 8), juce::Justification::centred);
            g.setFont (jkit::mf (9.5f)); g.setColour (C::dim);
            const int nb = barsOf (proc.vi ("bars"));
            g.drawText (juce::String (nb) + " battute" + (m == COMPOSIZIONE ? U (" - una traccia per chitarra") : U (" - nella traccia di Ableton")), dragR.withTrimmedTop (40).withHeight (14), juce::Justification::centred);
            if (m == COMPOSIZIONE) { static const char* nm[3] = { "BASE", "ARPEGGIO", "LEAD" }; for (int i = 0; i < 3; ++i) chipBtn (g, layDragR[i], U ("\xe2\x87\xa2 ") + nm[i], false, layerCol (i), 8.5f, hoverId == 260 + i); }
        }
        { static const char* sn[4] = { "GRIGLIA 1/16", "GRIGLIA 1/8", "GRIGLIA TERZINE", "GRIGLIA LIBERA" }; chipBtn (g, snapR, sn[snapIdx], false, C::violet, 9, hoverId == 270); }
        const bool ed = proc.edited();
        chipBtn (g, resetEditR, ed ? U ("TORNA AL GENERATO") : U ("NON MODIFICATO"), ed, ed ? C::red : C::dim, 9, hoverId == 271);
        chipBtn (g, genR, U ("GENERA GIRO"), false, C::yellow, 9.5f, hoverId == 231); chipBtn (g, varR, U ("VARIA"), false, C::yellow, 9.5f, hoverId == 232); chipBtn (g, rndR, U ("A CASO"), false, C::red, 9.5f, hoverId == 237);
        chipBtn (g, playR, proc.v ("play") > 0.5f ? U ("\xe2\x96\xa0 FERMA") : U ("\xe2\x96\xb6 PROVA"), proc.v ("play") > 0.5f, C::green, 10, hoverId == 233);
        chipBtn (g, liveR, U ("GIRO IN PLAY"), proc.v ("live") > 0.5f, C::green, 10, hoverId == 234);
        chipBtn (g, midiR, U ("MIDI OUT"), proc.v ("midiOut") > 0.5f, C::green, 10, hoverId == 235);
        chipBtn (g, csR, U ("CANALE x CORDA"), proc.v ("chStr") > 0.5f, C::green, 9.5f, hoverId == 236);
    }
    // SUONO
    for (int i = 0; i < kBodies; ++i) chipBtn (g, bodyR[i], bodyName (i), proc.vi ("body") == i, C::phos, 10, hoverId == 300 + i);
    chipBtn (g, listenR, proc.v ("listen") > 0.5f ? U ("CHITARRA ACCESA") : U ("CHITARRA SPENTA (SOLO MIDI)"), proc.v ("listen") > 0.5f, C::phos, 9.5f, hoverId == 310);
    // riga d'aiuto
    g.setFont (jkit::mf (10)); g.setColour (C::dim);
    g.drawText (info.isNotEmpty() ? info : U ("Clic su un accordo per cambiarlo. GENERA GIRO = nuovo giro dal genere, VARIA = stessa armonia suonata in un altro modo. Cmd+Z annulla."), juce::Rectangle<float> (30, (float) H - 24, 1030, 14), juce::Justification::centredLeft);
}
void Editor::repaintAll() { std::function<void (juce::Component*)> all = [&all] (juce::Component* c) { c->repaint(); for (auto* q : c->getChildren()) all (q); }; all (this); }
void Editor::slotMenu (int i, juce::Rectangle<float> at)
{
    const auto S = proc.settings(); const auto pr = proc.prog(); if (i >= (int) pr.size()) return;
    const int cur = pr[(size_t) i], curDeg = codeDeg (cur), curCol = codeCol (cur);
    juce::PopupMenu m; m.addSectionHeader (U ("ACCORDI DELLA SCALA"));
    for (int d = 0; d < 7; ++d) { const auto c = slotChord (S, withCol (d, curCol)); m.addItem (1 + d, degreeRoman (d, S.scale, S.key) + "   " + c.name, true, curDeg == d); }
    bool any = false; for (int d = 0; d < 7; ++d) any |= canBorrow (S.scale, d);
    if (any)
    {
        m.addSectionHeader (U ("PRESI IN PRESTITO (CON CAUTELA)"));
        for (int d = 0; d < 7; ++d) if (canBorrow (S.scale, d)) { const auto c = slotChord (S, withCol (10 + d, curCol)); m.addItem (11 + d, degreeRoman (10 + d, S.scale, S.key) + "   " + c.name, true, curDeg == 10 + d); }
    }
    // colore di questo accordo (solo note della scala: se il colore non c'e', resta la triade)
    m.addSectionHeader (U ("COLORE DI QUESTO ACCORDO"));
    m.addItem (99, juce::String ("AUTO (colore base)"), true, curCol < 0);
    for (int c = 0; c < kColors; ++c) if (c != 3) { const auto ch = slotChord (S, withCol (curDeg, c)); m.addItem (100 + c, juce::String (colorName (c)) + "   " + ch.name, true, curCol == c); }
    juce::Component::SafePointer<Editor> sp (this);
    const auto area = at.isEmpty() ? slotR[i] : at;
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withTargetScreenArea (localAreaToGlobal (scaler.map (area.toNearestInt()))), [sp, i] (int r)
    {
        if (sp == nullptr || r <= 0) return;
        auto p = sp->proc.prog(); if (i >= (int) p.size()) return;
        sp->proc.undo.beginNewTransaction();
        const int col = codeCol (p[(size_t) i]);
        if (r >= 99) p[(size_t) i] = withCol (p[(size_t) i], r == 99 ? -1 : r - 100);
        else p[(size_t) i] = withCol (r <= 7 ? r - 1 : 10 + (r - 11), col);
        sp->proc.setProg (p); sp->proc.curPreset = -1; sp->selSlot = i; sp->repaint();
    });
}
void Editor::layerMenu (int ly)
{
    static const char* ids[3] = { "lyBase", "lyArp", "lyLead" };
    auto* prm = dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter (ids[ly])); if (prm == nullptr) return;
    juce::PopupMenu m; m.addSectionHeader (ly == 0 ? U ("BASE (A SINISTRA)") : ly == 1 ? U ("ARPEGGIO (SECONDA CHITARRA, A DESTRA)") : U ("LEAD (AL CENTRO)"));
    if (ly == 1) m.addSectionHeader (U ("STILI"));
    for (int k = 0; k < prm->choices.size(); ++k) m.addItem (1 + k, prm->choices[k], true, prm->getIndex() == k);
    if (ly == 1)
    {
        juce::PopupMenu f, r;
        for (int i = 0; i < kFigs; ++i) f.addItem (500 + i, juce::String (figOf (i).name) + "   " + figOf (i).seq, true, proc.vi ("arpFig") == i);
        for (int i = 0; i < kRits; ++i) r.addItem (600 + i, ritOf (i).name, true, proc.vi ("arpRit") == i);
        m.addSeparator(); m.addSubMenu (U ("FIGURA (ORDINE DELLE CORDE)"), f); m.addSubMenu (U ("RITMO"), r);
        m.addItem (700, gridEdit ? U ("TORNA AL PIANO ROLL") : U ("DISEGNA LA FIGURA (GRIGLIA)"));
    }
    juce::Component::SafePointer<Editor> sp (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withTargetScreenArea (localAreaToGlobal (scaler.map (layR[ly].toNearestInt()))), [sp, ly] (int r)
    {
        if (sp == nullptr || r <= 0) return;
        static const char* ids2[3] = { "lyBase", "lyArp", "lyLead" };
        sp->proc.undo.beginNewTransaction();
        if (r == 700) { sp->gridEdit = ! sp->gridEdit; if (sp->gridEdit) sp->proc.setV ("arpFig", (float) (kFigs - 1)); }
        else if (r >= 600) sp->proc.setV ("arpRit", (float) (r - 600));
        else if (r >= 500) { if (r - 500 == kFigLibera) { sp->askFigure(); return; } sp->proc.setV ("arpFig", (float) (r - 500)); sp->proc.setV ("arpRot", 0); if (r - 500 == kFigs - 1) sp->gridEdit = true; }
        else { sp->proc.setV (ids2[ly], (float) (r - 1)); if (ly == 1 && r > 1) sp->proc.applyArpStyle (r - 2); }
        sp->proc.curPreset = -1; sp->repaint();
    });
}
void Editor::genreMenu()
{
    juce::PopupMenu m; m.addSectionHeader (U ("GENERE (GIRI E COLORE DEGLI ACCORDI)"));
    for (int g = 0; g < kGenres; ++g) m.addItem (1 + g, genreName (g), true, proc.vi ("genre") == g);
    juce::Component::SafePointer<Editor> sp (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withTargetScreenArea (localAreaToGlobal (scaler.map (genreR.toNearestInt()))), [sp] (int r)
    {
        if (sp == nullptr || r <= 0) return;
        sp->proc.undo.beginNewTransaction(); sp->proc.setV ("genre", (float) (r - 1)); sp->proc.generaGiro(); sp->repaint();
    });
}
void Editor::moodMenu()
{
    juce::PopupMenu m; m.addSectionHeader (U ("MOOD (CARATTERE DEL GIRO E DELLA MELODIA)"));
    for (int i = 0; i < kMoods; ++i) m.addItem (1 + i, i == 0 ? juce::String ("AUTO (dal genere: ") + moodName (moodOfGenre (proc.vi ("genre"))) + ")" : juce::String (moodName (i)), true, proc.vi ("mood") == i);
    juce::Component::SafePointer<Editor> sp (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withTargetScreenArea (localAreaToGlobal (scaler.map (moodR.toNearestInt()))), [sp] (int r)
    {
        if (sp == nullptr || r <= 0) return;
        sp->proc.undo.beginNewTransaction(); sp->proc.setV ("mood", (float) (r - 1)); sp->proc.curPreset = -1; sp->repaint();
    });
}
void Editor::mouseDown (const juce::MouseEvent& e)
{
    const auto p = e.position / scaler.s;
    auto tog = [this] (const char* id) { proc.setV (id, proc.v (id) > 0.5f ? 0.0f : 1.0f); repaint(); };
    for (int i = 0; i < 12; ++i) if (keyR[i].contains (p)) { proc.setV ("key", (float) i); proc.curPreset = -1; repaint(); return; }
    for (int i = 0; i < kScales; ++i) if (scaleR[i].contains (p)) { proc.setV ("scale", (float) i); proc.curPreset = -1; repaint(); return; }
    if (genreR.contains (p)) { genreMenu(); return; }
    for (int i = 0; i < 4; ++i) if (barsR[i].contains (p)) { proc.setV ("bars", (float) (i + 1)); repaint(); return; }
    if (rndR.contains (p)) { proc.aCaso(); repaint(); return; }
    for (int i = 0; i < 3; ++i) if (perBarR[i].contains (p)) { proc.setV ("perBar", (float) i); repaint(); return; }
    if (moodR.contains (p)) { moodMenu(); return; }
    if (addR.contains (p) || remR.contains (p))
    {
        auto pr = proc.prog();
        if (addR.contains (p) && pr.size() < 8) pr.push_back (pr.size() % 2 == 0 ? 0 : pr.back());
        else if (remR.contains (p) && pr.size() > 1) pr.pop_back();
        proc.undo.beginNewTransaction(); proc.setProg (pr); proc.curPreset = -1; repaint(); return;
    }
    for (int i = 0; i < 8; ++i) if (slotR[i].contains (p)) { selSlot = i; slotMenu (i); repaint(); return; }
    for (int i = 0; i < 5; ++i) if (octR[i].contains (p)) { proc.setV ("octave", (float) (i - 2)); repaint(); return; }
    if (tuningR.contains (p)) { proc.setV ("tuning", (float) ((proc.vi ("tuning") + 1) % kTunings)); repaint(); return; }
    for (int i = 0; i < kVStyles; ++i) if (vstyleR[i].contains (p)) { proc.setV ("vstyle", (float) i); repaint(); return; }
    for (int i = 0; i < kModes; ++i) if (modeR[i].contains (p)) { proc.setV ("mode", (float) i); if (i != COMPOSIZIONE) proc.setV ("pat", 0); proc.curPreset = -1; sel.clear(); selSig.clear(); if (i != ARPEGGIO && i != COMPOSIZIONE) gridEdit = false; repaint(); return; }
    const bool comp = proc.vi ("mode") == COMPOSIZIONE;
    if (comp)
    {
        for (int i = 0; i < 3; ++i) if (layR[i].contains (p)) { layerMenu (i); return; }
        for (int i = 0; i < 3; ++i) if (diceR[i].contains (p)) { proc.rerollLayer (i); repaint(); return; }
        for (int i = 0; i < 3; ++i) if (editLayR[i].contains (p)) { editLayer = i; repaint(); return; }
        for (int i = 0; i < 3; ++i) if (layDragR[i].contains (p)) return;   // trascinamento in mouseDrag
    }
    else if (proc.vi ("mode") == ARPEGGIO)
    {
        if (styleR.contains (p)) { styleMenu(); return; }
        if (figR.contains (p)) { figMenu(); return; }
        if (ritR.contains (p)) { ritMenu(); return; }
        if (figDiceR.contains (p)) { randomFig(); return; }
        if (rotR.contains (p))
        {   // PARTENZA: la stessa figura cominciata da un'altra corda
            const int fg = proc.vi ("arpFig");
            const auto seq = fg == kFigLibera ? proc.apvts.state.getProperty ("figSeq", "1").toString() : juce::String (figOf (fg).seq);
            const int n = juce::jlimit (1, 8, juce::StringArray::fromTokens (seq, " ", "").size());
            proc.setV ("arpRot", (float) ((proc.vi ("arpRot") + 1) % n)); proc.curPreset = -1; repaint(); return;
        }
        if (writeR.contains (p)) { askFigure(); return; }
        if (ritDiceR.contains (p)) { int f = proc.vi ("arpRit"), n = f; while (n == f) n = juce::Random::getSystemRandom().nextInt (kRits); proc.setV ("arpRit", (float) n); proc.curPreset = -1; repaint(); return; }
        if (gridBtnR.contains (p)) { gridEdit = ! gridEdit; if (gridEdit) proc.setV ("arpFig", (float) (kFigs - 1)); repaint(); return; }
    }
    else for (int i = 0; i < 12; ++i) if (patR[i].contains (p) && i < patternNames (proc.vi ("mode")).size()) { proc.setV ("pat", (float) i); proc.curPreset = -1; repaint(); return; }
    if (gridEdit && (proc.vi ("mode") == ARPEGGIO || proc.vi ("mode") == COMPOSIZIONE) && rollR.contains (p))
    {   // griglia: clic accende o spegne, trascinando si dipinge
        auto gr = proc.grid();
        for (int r = 0; r < 5; ++r) for (int k = 0; k < 16; ++k) if (gridCell (r, k).expanded (1.5f).contains (p))
        { gridPaint = (gr[(size_t) r] & (1 << k)) ? 0 : 1; gr[(size_t) r] = (juce::uint16) (gridPaint ? gr[(size_t) r] | (1 << k) : gr[(size_t) r] & ~(1 << k)); proc.setGrid (gr); repaint(); }
        return;
    }
    if (snapR.contains (p)) { snapIdx = (snapIdx + 1) % 4; repaint(); return; }
    if (resetEditR.contains (p)) { if (proc.edited()) { proc.clearEdit(); sel.clear(); selSig.clear(); } repaint(); return; }
    for (auto& cl : chordLabels) if (cl.first.contains (p)) { const auto pr = proc.prog(); slotMenu (cl.second % (int) pr.size(), cl.first); return; }
    if (rollR.contains (p))
    {   // piano roll: seleziona, sposta, allunga, rettangolo di selezione
        auto P = proc.part(); if (P == nullptr) return;
        grabKeyboardFocus();
        edPart = *P; resolveSel (edPart); downPos = p; orig = edPart.ev;
        hitIdx = hitNote (edPart, p);
        if (hitIdx >= 0)
        {
            const bool isSel = std::find (sel.begin(), sel.end(), hitIdx) != sel.end();
            if (e.mods.isShiftDown()) { if (isSel) sel.erase (std::find (sel.begin(), sel.end(), hitIdx)); else sel.push_back (hitIdx); }
            else if (! isSel) sel = { hitIdx };
            const auto nr = noteRect (edPart.ev[(size_t) hitIdx], edPart.len());
            editMode = p.x > nr.getRight() - 6 ? 2 : 1;
        }
        else { if (! e.mods.isShiftDown()) sel.clear(); editMode = 3; marquee = { p, p }; }
        editing = true; storeSig (edPart); repaint(); return;
    }
    if (genR.contains (p)) { proc.generaGiro(); repaint(); return; }
    if (varR.contains (p)) { proc.varia(); repaint(); return; }
    if (playR.contains (p)) { tog ("play"); return; }
    if (liveR.contains (p)) { tog ("live"); return; }
    if (midiR.contains (p)) { tog ("midiOut"); return; }
    if (csR.contains (p)) { tog ("chStr"); return; }
    for (int i = 0; i < kBodies; ++i) if (bodyR[i].contains (p)) { proc.setV ("body", (float) i); repaint(); return; }
    if (listenR.contains (p)) { tog ("listen"); return; }
    if (undoR.contains (p)) { doUndo (false); return; }
    if (redoR.contains (p)) { doUndo (true); return; }
    if (prevR.contains (p) || nextR.contains (p)) { const int n = (int) presets().size(); proc.loadPreset ((juce::jmax (0, proc.curPreset) + (nextR.contains (p) ? 1 : n - 1)) % n); repaintAll(); return; }
    if (themeR.contains (p)) { skin::cycle(); C::sync(); bg = {}; repaintAll(); return; }
    if (presetR.contains (p)) { browser.open(); return; }
}
void Editor::mouseDrag (const juce::MouseEvent& e)
{
    const auto p = e.position / scaler.s, d0 = e.getMouseDownPosition().toFloat() / scaler.s;
    if (gridPaint >= 0)
    {
        auto gr = proc.grid(); bool ch = false;
        for (int r = 0; r < 5; ++r) for (int k = 0; k < 16; ++k) if (gridCell (r, k).contains (p))
        { const bool on = gr[(size_t) r] & (1 << k); if (on != (gridPaint == 1)) { gr[(size_t) r] = (juce::uint16) (gridPaint ? gr[(size_t) r] | (1 << k) : gr[(size_t) r] & ~(1 << k)); ch = true; } }
        if (ch) { proc.setGrid (gr); repaint(); }
        return;
    }
    if (editing)
    {
        const double L = edPart.len();
        if (editMode == 3)
        {
            marquee = juce::Rectangle<float> (d0, p);
            sel.clear(); for (int i = 0; i < (int) edPart.ev.size(); ++i) if (noteRect (edPart.ev[(size_t) i], L).intersects (marquee)) sel.push_back (i);
            storeSig (edPart); repaint(); return;
        }
        if (hitIdx < 0 || sel.empty()) return;
        const auto& a = orig[(size_t) hitIdx];
        if (editMode == 1)
        {   // sposta: tempo sulla griglia, altezza per gradi della scala (gli intervalli fra le note selezionate restano quelli della scala)
            const double nb = snapB (a.beat + xb (p.x, L) - xb (d0.x, L)), db = juce::jlimit (-a.beat, L - 0.05 - a.beat, nb - a.beat);
            int target = yn (p.y); const auto S = proc.settings(); const auto ss = scaleSet (S.key, S.scale);
            for (int k = 0; k < 12 && ! ss[(size_t) (((target % 12) + 12) % 12)]; ++k) target += (k % 2 == 0 ? k + 1 : -(k + 1));
            const int steps = stepsBetween (a.note, target);
            for (int i : sel)
            {
                auto& ev = edPart.ev[(size_t) i]; const auto& o = orig[(size_t) i];
                ev.beat = juce::jlimit (0.0, L - 0.02, o.beat + db);
                ev.note = ss[(size_t) (o.note % 12)] ? scaleShift (o.note, steps) : o.note + (target - a.note);
                ev.str = o.str; fixString (ev);
            }
        }
        else
        {   // allunga / accorcia dal bordo destro
            const double end = snapB (a.beat + a.len + xb (p.x, L) - xb (d0.x, L)), dl = end - (a.beat + a.len);
            for (int i : sel) { auto& ev = edPart.ev[(size_t) i]; const auto& o = orig[(size_t) i]; ev.len = juce::jlimit (0.03, L - o.beat, o.len + dl); }
        }
        storeSig (edPart); repaint(); return;
    }
    if (dragging || e.getDistanceFromDragStart() < 6) return;
    int layer = -2;
    if (dragR.contains (d0)) layer = -1;
    if (proc.vi ("mode") == COMPOSIZIONE) for (int i = 0; i < 3; ++i) if (layDragR[i].contains (d0)) layer = i;
    if (layer < -1) return;
    auto f = proc.exportMidi (layer); if (! f.existsAsFile()) return;
    dragging = true; repaint();
    juce::Component::SafePointer<Editor> sp (this);
    performExternalDragDropOfFiles ({ f.getFullPathName() }, false, this, [sp] { if (sp != nullptr) { sp->dragging = false; sp->repaint(); } });
}
void Editor::mouseUp (const juce::MouseEvent& e)
{
    gridPaint = -1;
    if (! editing) return;
    editing = false;
    if ((editMode == 1 || editMode == 2) && e.getDistanceFromDragStart() > 2)
    {
        // i bend delle note del lead spostate non hanno piu' senso: si tolgono
        for (int i : sel) { const auto& o = orig[(size_t) i]; if (o.lead) edPart.bend.erase (std::remove_if (edPart.bend.begin(), edPart.bend.end(), [&] (const Bend& b) { return b.beat > o.beat - 0.02 && b.beat < o.beat + o.len && b.str == o.str; }), edPart.bend.end()); }
        commitEdit();
    }
    editMode = 0; repaint();
}
void Editor::mouseDoubleClick (const juce::MouseEvent& e)
{
    const auto p = e.position / scaler.s; if (! rollR.contains (p) || gridEdit) return;
    auto P = proc.part(); if (P == nullptr) return;
    editing = false; editMode = 0;
    edPart = *P; const int h = hitNote (edPart, p);
    if (h >= 0) { sel = { h }; storeSig (edPart); deleteSelected(); return; }
    // nota nuova: sulla griglia, sulla nota della scala piu' vicina
    const auto S = proc.settings(); const auto ss = scaleSet (S.key, S.scale);
    int n = yn (p.y); for (int k = 0; k < 12 && ! ss[(size_t) (((n % 12) + 12) % 12)]; ++k) n += (k % 2 == 0 ? k + 1 : -(k + 1));
    Ev ev; ev.beat = juce::jlimit (0.0, edPart.len() - 0.1, grid() > 0 ? std::floor (xb (p.x, edPart.len()) / grid()) * grid() : xb (p.x, edPart.len()));
    ev.len = grid() > 0 ? grid() * 2 : 0.5; ev.note = n; ev.vel = 0.7f; ev.str = 3;
    const bool comp = S.mode == COMPOSIZIONE; ev.layer = comp ? editLayer : 0; ev.lead = S.mode == LEAD || (comp && editLayer == 2);
    ev.slot = edPart.chordAt.empty() ? 0 : juce::jlimit (0, (int) edPart.chordAt.size() - 1, (int) std::floor (ev.beat / (edPart.len() / (double) edPart.chordAt.size())));
    fixString (ev);
    edPart.ev.push_back (ev); sel = { (int) edPart.ev.size() - 1 }; commitEdit();
}
void Editor::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    const auto p = e.position / scaler.s; if (! rollR.contains (p)) return;
    auto P = proc.part(); if (P == nullptr) return;
    edPart = *P; resolveSel (edPart); if (sel.empty()) return;
    for (int i : sel) edPart.ev[(size_t) i].vel = juce::jlimit (0.06f, 1.0f, edPart.ev[(size_t) i].vel + (w.deltaY > 0 ? 0.04f : -0.04f));   // rotella = forza
    commitEdit();
}
void Editor::mouseMove (const juce::MouseEvent& e)
{
    const auto p = e.position / scaler.s; int h = -1, hs = -1;
    auto chk = [&] (const juce::Rectangle<float>& r, int id) { if (r.contains (p)) h = id; };
    for (int i = 0; i < 12; ++i) chk (keyR[i], 100 + i);
    for (int i = 0; i < kScales; ++i) chk (scaleR[i], 120 + i);
    chk (genreR, 130); for (int i = 0; i < 4; ++i) chk (barsR[i], 140 + i); chk (rndR, 237); for (int i = 0; i < 3; ++i) chk (perBarR[i], 150 + i); chk (moodR, 132); chk (remR, 160); chk (addR, 161);
    chk (tuningR, 170); for (int i = 0; i < 5; ++i) chk (octR[i], 190 + i); for (int i = 0; i < kVStyles; ++i) chk (vstyleR[i], 180 + i);
    for (int i = 0; i < kModes; ++i) chk (modeR[i], 200 + i);
    if (proc.vi ("mode") == COMPOSIZIONE) for (int i = 0; i < 3; ++i) { chk (layR[i], 240 + i); chk (diceR[i], 243 + i); chk (editLayR[i], 250 + i); chk (layDragR[i], 260 + i); }
    else if (proc.vi ("mode") == ARPEGGIO) { chk (styleR, 280); chk (figR, 281); chk (figDiceR, 282); chk (ritR, 283); chk (ritDiceR, 284); chk (gridBtnR, 285); chk (rotR, 286); chk (writeR, 287); }
    else for (int i = 0; i < 12; ++i) chk (patR[i], 210 + i);
    chk (snapR, 270); chk (resetEditR, 271);
    chk (dragR, 230); chk (genR, 231); chk (varR, 232); chk (playR, 233); chk (liveR, 234); chk (midiR, 235); chk (csR, 236);
    for (int i = 0; i < kBodies; ++i) chk (bodyR[i], 300 + i); chk (listenR, 310); chk (themeR, 900);
    for (int i = 0; i < 8; ++i) if (slotR[i].contains (p)) hs = i;
    if (h != hoverId || hs != hoverSlot) { hoverId = h; hoverSlot = hs; repaint(); }
    static const std::map<int, const char*> tips = {
        { 130, "Il genere sceglie i giri tipici e il colore degli accordi. Cambiarlo genera un giro nuovo." },
        { 132, "Mood: il carattere del giro. MALINCONICO scende e respira, APERTO corde a vuoto e note comuni, IPNOTICO ripete il motivo, SOGNANTE none e note lunghe, TESO stretto e basso, CALDO settime, ENERGICO pieno e in salita." },
        { 150, "Un accordo per battuta." }, { 151, "Due accordi per battuta." }, { 152, "Un accordo ogni due battute: giri lenti, ipnotici." },
        { 160, "Toglie l'ultimo accordo del giro." }, { 161, "Aggiunge un accordo al giro (fino a 8)." },
        { 170, "Accordatura: STANDARD, DROP D (sesta corda in Re), -1/2 TONO." },
        { 180, "APERTI: accordi con le corde a vuoto, il suono delle acustiche." }, { 181, "BARRE': forme chiuse e piene, si spostano sul manico." },
        { 182, "TRIADI ALTE: tre o quattro corde acute, per funk, R&B, afro." }, { 183, "POWER: fondamentale e quinta, per rock, punk e palm mute." }, { 184, "JAZZ: voicing di settima e nona anche senza fondamentale al basso." },
        { 200, "ACCORDI: accordi tenuti, colpi, stab, neo soul." }, { 201, "RITMICA: pennate su e giu' corda per corda, stoppate, palm mute." },
        { 202, "ARPEGGIO: fingerpicking, una corda alla volta, le note si lasciano suonare." }, { 203, "LEAD: melodia con bend, vibrato, scivolate e legati nel box della POSIZIONE." },
        { 230, "Trascina nella traccia: il clip MIDI e' gia' in tempo e lungo quanto il giro. In composizione: un file con una traccia per chitarra." }, { 231, "Nuovo giro (per funzione armonica, dal genere e dal mood), nuovo pattern adatto al genere, nuova suonata." }, { 237, "Tutto nuovo e a caso (genere, mood, modo di suonare, giro, groove) tenendo solo tonalita', scala e lunghezza." }, { 232, "Stesso giro e stessi pattern, suonati in un altro modo." },
        { 233, "Ascolta il giro anche a trasporto fermo." }, { 234, "Acceso: il giro suona con il play della DAW. Spento: la chitarra suona solo il MIDI che arriva (per esempio il clip trascinato)." },
        { 235, "Manda le note fuori dal plugin (per suonare un'altra chitarra o un sampler)." }, { 236, "Ogni corda su un canale (2-7): i bend restano sulla corda giusta in un'altra chitarra virtuale." },
        { 310, "Spenta: nessun suono, esce solo il MIDI." },
        { 204, "COMPOSIZIONE: tre chitarre sullo stesso giro. BASE a sinistra, ARPEGGIO (seconda chitarra, in alto) a destra, LEAD al centro. Si ascoltano: l'arpeggio lascia spazio alla melodia." },
        { 240, "Come suona la base (accordi o ritmica). OFF la toglie." }, { 241, "Che arpeggio fa la seconda chitarra. OFF lo toglie." }, { 242, "Che melodia fa il lead. OFF lo toglie." },
        { 243, "Rifa' solo la base, il resto resta." }, { 244, "Rifa' solo l'arpeggio." }, { 245, "Rifa' solo la melodia." },
        { 250, "Le note aggiunte con il doppio clic vanno alla base." }, { 251, "Le note aggiunte vanno all'arpeggio." }, { 252, "Le note aggiunte vanno al lead." },
        { 260, "Trascina solo la base." }, { 261, "Trascina solo l'arpeggio." }, { 262, "Trascina solo il lead." },
        { 280, "Stili pronti: una figura e un ritmo che stanno bene insieme." },
        { 281, "FIGURA: l'ordine in cui la mano prende le corde dell'accordo. 1 = la corda piu' acuta, B = il basso. Segue ogni accordo da sola." },
        { 282, "Un'altra figura, stesso ritmo: a volte una della lista, a volte una nuova che parte da qualsiasi corda." },
        { 286, "PARTENZA: la stessa figura cominciata da un'altra nota (1 3 4 2 diventa 3 4 2 1, poi 4 2 1 3...)." },
        { 287, "Scrivi la tua figura: per esempio 1 3 4 2, oppure 3 B 2 1. 1 = corda piu' acuta, B = basso." }, { 283, "RITMO dell'arpeggio, indipendente dalla figura. Se la figura ha 3 note e il ritmo 4, la figura scivola sul tempo." },
        { 284, "Un altro ritmo, stessa figura." }, { 285, "Disegna la tua figura: 5 righe (le corde dell'accordo dall'acuta al basso) per 16 sedicesimi." },
        { 270, "Griglia del piano roll per spostare e aggiungere note." }, { 271, "Le modifiche a mano restano finche' non cambi il giro o le manopole della parte. Qui le togli." } };
    juce::String ni; auto it = tips.find (h); if (it != tips.end()) ni = U (it->second);
    if (hs >= 0) ni = U ("Clic per scegliere l'accordo e il suo colore: sopra quelli della scala, poi quelli presi in prestito, in fondo il colore.");
    for (auto& cl : chordLabels) if (cl.first.contains (p)) ni = U ("Clic per cambiare questo accordo o il suo colore.");
    if (rollR.contains (p)) ni = U ("Clic seleziona (Maiusc aggiunge), trascina sposta, bordo destro allunga, doppio clic aggiunge o toglie, rotella = forza, frecce = grado / griglia, Canc cancella.");
    if (ni != info) { info = ni; repaint(); }
    setMouseCursor (h >= 0 || hs >= 0 || presetR.contains (p) || prevR.contains (p) || nextR.contains (p) || undoR.contains (p) || redoR.contains (p) ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
}
void Editor::setupBrowser()
{
    addChildComponent (browser);
    browser.accent = C::amber; browser.title = "Preset";
    auto userFiles = [] { auto f = presetFolder().findChildFiles (juce::File::findFiles, false, "*.jgtr"); f.sort(); return f; };
    browser.entries = [userFiles]
    {
        std::vector<jbrowse::Entry> v;
        for (int i = 0; i < (int) presets().size(); ++i)
        {
            const juce::String nm (presets()[(size_t) i].name); jbrowse::Entry e; e.id = i;
            e.group = nm.upToFirstOccurrenceOf (" | ", false, false); e.name = nm.fromFirstOccurrenceOf (" | ", false, false); e.note = U (presets()[(size_t) i].note); v.push_back (e);
        }
        const auto fs = userFiles();
        for (int i = 0; i < fs.size(); ++i) { jbrowse::Entry e; e.id = 100000 + i; e.user = true; e.group = "I MIEI"; e.name = fs[i].getFileNameWithoutExtension(); e.note = "Preset tuo, in Musica / Jackpot / Guitar Preset."; v.push_back (e); }
        return v;
    };
    browser.current = [this, userFiles]
    {
        if (proc.curPreset >= 0) return proc.curPreset;
        const auto fs = userFiles(); for (int i = 0; i < fs.size(); ++i) if (fs[i].getFileNameWithoutExtension() == proc.presetName) return 100000 + i;
        return -1;
    };
    browser.load = [this] (const jbrowse::Entry& e) { if (e.user) proc.loadPresetFile (presetFolder().getChildFile (e.name + ".jgtr")); else proc.loadPreset (e.id); repaintAll(); };
    browser.save = [this] { askPresetName(); };
    browser.canOverwrite = [this] { return proc.curPreset < 0 && presetFolder().getChildFile (proc.presetName + ".jgtr").existsAsFile(); };
    browser.overwrite = [this] { proc.savePresetFile (presetFolder().getChildFile (proc.presetName + ".jgtr")); repaint(); };
    browser.folder = [] { presetFolder().startAsProcess(); };
    browser.remove = [] (const jbrowse::Entry& e) { presetFolder().getChildFile (e.name + ".jgtr").moveToTrash(); };
    browser.onClose = [this] { repaint(); };
}
void Editor::askPresetName()
{
    auto* w = new juce::AlertWindow ("Salva preset", "Nome del preset:", juce::MessageBoxIconType::NoIcon, this);
    w->addTextEditor ("name", proc.presetName);
    w->addButton ("SALVA", 1, juce::KeyPress (juce::KeyPress::returnKey)); w->addButton ("ANNULLA", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    juce::Component::SafePointer<Editor> sp (this);
    w->enterModalState (true, juce::ModalCallbackFunction::create ([sp, w] (int r)
    {
        if (sp == nullptr || r != 1) return;
        const auto nm = juce::File::createLegalFileName (w->getTextEditorContents ("name").trim());
        if (nm.isNotEmpty()) { sp->proc.savePresetFile (presetFolder().getChildFile (nm + ".jgtr")); sp->browser.refresh(); sp->repaint(); }
    }), true);
}
bool Editor::keyPressed (const juce::KeyPress& k)
{
    if (k.getModifiers().isCommandDown() && (k.getKeyCode() == 'Z' || k.getKeyCode() == 'z')) { doUndo (k.getModifiers().isShiftDown()); return true; }
    if (k.getModifiers().isCommandDown() && (k.getKeyCode() == 'Y' || k.getKeyCode() == 'y')) { doUndo (true); return true; }
    if (k.getKeyCode() == juce::KeyPress::deleteKey || k.getKeyCode() == juce::KeyPress::backspaceKey) { if (! selSig.empty()) { deleteSelected(); return true; } return false; }
    if (k.getModifiers().isCommandDown() && (k.getKeyCode() == 'A' || k.getKeyCode() == 'a')) { if (auto P = proc.part()) { sel.clear(); for (int i = 0; i < (int) P->ev.size(); ++i) sel.push_back (i); storeSig (*P); repaint(); } return true; }
    const int kc = k.getKeyCode();
    if (! selSig.empty() && (kc == juce::KeyPress::upKey || kc == juce::KeyPress::downKey || kc == juce::KeyPress::leftKey || kc == juce::KeyPress::rightKey))
    {   // frecce: su / giu' di un grado della scala, sinistra / destra di un passo di griglia
        auto P = proc.part(); if (P == nullptr) return false;
        edPart = *P; resolveSel (edPart);
        const double g = grid() > 0 ? grid() : 0.25;
        for (int i : sel)
        {
            auto& ev = edPart.ev[(size_t) i];
            if (kc == juce::KeyPress::upKey || kc == juce::KeyPress::downKey) { ev.note = scaleShift (ev.note, kc == juce::KeyPress::upKey ? 1 : -1); fixString (ev); }
            else ev.beat = juce::jlimit (0.0, edPart.len() - 0.02, ev.beat + (kc == juce::KeyPress::rightKey ? g : -g));
        }
        commitEdit(); return true;
    }
    return false;
}
void Editor::doUndo (bool redo)
{
    proc.undo.beginNewTransaction();
    if (redo ? proc.undo.redo() : proc.undo.undo()) repaintAll();
    repaint();
}
void Editor::timerCallback()
{
    if (browser.isVisible()) return;
    if (bgTheme != skin::cur() && bgTheme >= 0) { skin::apply(); C::sync(); bg = {}; repaintAll(); }
    for (auto* k : knobs) if (k->isMouseOverOrDragging()) { if (info != k->getHelpText()) info = k->getHelpText(); }
    repaint();
}
}
