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
    add ("density", "DENSITA'", C::amber, pct, "Quante note: a sinistra respira, a destra si riempie. Nella RITMICA sceglie fra le tre versioni del pattern.");
    add ("dyn", "DINAMICA", C::amber, pct, "Differenza fra colpi accentati e leggeri.");
    add ("strum", "PENNATA", C::amber, [] (float v) { return juce::String (juce::roundToInt (7 + v * 34)) + " ms"; }, "Velocita' della pennata: quanto tempo passa dalla prima all'ultima corda.");
    add ("human", "UMANO", C::amber, pct, "Piccole imprecisioni di tempo e di forza, come un chitarrista vero.");
    add ("swing", "SWING", C::amber, pct, "Ritarda i levare (sedicesimi o ottavi): groove R&B, lo-fi, afro.");
    add ("ring", "RISUONA", C::green, pct, "Negli arpeggi: quanto lasciare suonare le corde prima della nota dopo.");
    add ("emb", "ABBELLIM.", C::green, pct, "Hammer-on e scivolate dalle note della scala, negli arpeggi e negli accordi neo soul.");
    add ("tech", "TECNICA", C::green, pct, "Nel LEAD: bend, vibrato, scivolate, legati e doppie note.");
    add ("borrow", "INTERSCAMBIO", C::red, [] (float v) { return v < 0.005f ? juce::String ("OFF") : juce::String (juce::roundToInt (v * 100)) + "%"; }, "Accordi presi in prestito (il V maggiore in minore, il iv minore in maggiore). Spento: la scala si rispetta sempre.");
    add ("pos", "POSIZIONE", C::violet, [] (float v) { return v < 0.5f ? juce::String ("APERTA") : juce::String ("TASTO ") + juce::String (juce::roundToInt (v)); }, "Dove sta la mano sul manico: accordi e lead si cercano attorno a questo tasto.");
    add ("color", "COLORE", C::violet, [] (float v) { static const char* n[5] = { "AUTO", "TRIADI", "SETTIME", "NONE", "SUS/ADD9" }; return juce::String (n[juce::jlimit (0, 4, juce::roundToInt (v))]); }, "Che accordi: AUTO segue il genere.");
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
    genreR = { 44, 178, 150, 26 };
    for (int i = 0; i < 4; ++i) barsR[i] = { 270.0f + i * 38, 178, 34, 26 };
    for (int i = 0; i < 2; ++i) perBarR[i] = { 470.0f + i * 38, 178, 34, 26 };
    remR = { 560, 178, 28, 26 }; addR = { 592, 178, 28, 26 };
    for (int i = 0; i < 8; ++i) slotR[i] = { 44.0f + i * 73, 220, 68, 70 };
    fretR = { 672, 134, 470, 120 }; tuningR = { 1052, 100, 90, 24 };
    for (int i = 0; i < kVStyles; ++i) vstyleR[i] = { 672.0f + i * 95, 270, 90, 24 };
    for (int i = 0; i < kModes; ++i) modeR[i] = { 44, 346.0f + i * 40, 140, 34 };
    for (int i = 0; i < 9; ++i) patR[i] = { 204.0f + (i % 5) * 112, 344.0f + (i / 5) * 30, 106, 26 };
    rollR = { 204, 410, 640, 116 };
    dragR = { 866, 344, 272, 78 };
    genR = { 866, 432, 134, 30 }; varR = { 1004, 432, 134, 30 };
    playR = { 866, 470, 134, 26 }; liveR = { 1004, 470, 134, 26 }; midiR = { 866, 502, 134, 26 }; csR = { 1004, 502, 134, 26 };
    auto put = [this] (const char* id, int x, int y) { if (auto* k = knobById (knobs, id)) k->setBounds (x, y, 74, 100); };
    int x = 44; for (auto* id : { "density", "dyn", "strum", "human", "swing" }) { put (id, x, 582); x += 80; }
    x += 16; for (auto* id : { "ring", "emb", "tech" }) { put (id, x, 582); x += 80; }
    x += 16; for (auto* id : { "borrow", "pos", "color" }) { put (id, x, 582); x += 80; }
    for (int i = 0; i < kBodies; ++i) bodyR[i] = { 44.0f + i * 96, 726, 90, 28 };
    listenR = { 44, 762, 186, 24 };
    x = 450; for (auto* id : { "tone", "bodyAmt", "drive", "space", "width" }) { put (id, x, 694); x += 80; }
    put ("vol", 1050, 694);
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
    panel (g, { 24, 318, 1132, 220 }, C::violet, U ("CLIP"), U (""));
    panel (g, { 24, 554, 1132, 250 }, C::phos, U (""), U ("IL SUONO E' SOLO PER ASCOLTARE: IL MIDI ESCE UGUALE"));
grp (g, 270, 164, 150, U ("BATTUTE")); grp (g, 470, 164, 72, U ("ACC./BATT.")); grp (g, 560, 164, 60, U ("ACCORDI"));
    grp (g, 44, 164, 150, U ("GENERE")); grp (g, 44, 206, 580, U ("GIRO (CLIC PER CAMBIARE L'ACCORDO)"));
    grp (g, 44, 568, 392, U ("SUONATA")); grp (g, 456, 568, 232, U ("CORDE")); grp (g, 708, 568, 232, U ("ARMONIA E MANO"));
    grp (g, 44, 708, 380, U ("CHITARRA")); grp (g, 450, 680, 392, U ("SUONO")); grp (g, 1050, 680, 74, U ("USCITA"));
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
void Editor::drawRoll (juce::Graphics& g, const Part& P)
{
    const auto r = rollR; const double L = P.len();
    g.setColour (C::bg.withAlpha (0.6f)); g.fillRoundedRectangle (r.expanded (4), 6);
    int lo = 127, hi = 0; for (auto& e : P.ev) { lo = juce::jmin (lo, e.note); hi = juce::jmax (hi, e.note); }
    if (lo > hi) { lo = 40; hi = 76; }
    lo -= 2; hi += 2; const float nh = r.getHeight() / (float) (hi - lo + 1);
    auto bx = [&] (double b) { return r.getX() + (float) (b / L) * r.getWidth(); };
    for (int b = 0; b <= (int) L; ++b) { g.setColour (C::dim.withAlpha (b % 4 == 0 ? 0.45f : 0.15f)); g.fillRect (bx (b), r.getY(), 1.0f, r.getHeight()); }
    g.setFont (jkit::mf (9));
    for (size_t i = 0; i < P.chords.size(); ++i)
    {
        const float x = bx (P.chordAt[i]); g.setColour (P.chords[i].borrowed ? C::red : C::amber); g.drawText (P.chords[i].name, juce::Rectangle<float> (x + 3, r.getY() - 15, 70, 12), juce::Justification::centredLeft);
    }
    for (auto& e : P.ev)
    {
        const float x = bx (e.beat), w = juce::jmax (2.0f, bx (e.beat + e.len) - x), y = r.getBottom() - (e.note - lo + 1) * nh;
        g.setColour (strCol (e.str).withAlpha (e.mute ? 0.35f : 0.35f + 0.6f * e.vel)); g.fillRect (x, y, w, juce::jmax (2.0f, nh - 1));
    }
    if (! P.bend.empty())
    {   // curva dei bend sotto
        juce::Path pb; bool st = false; for (auto& b : P.bend) { const float x = bx (b.beat), y = r.getBottom() - 2 - b.semis * 6; if (! st) { pb.startNewSubPath (x, y); st = true; } else pb.lineTo (x, y); }
        g.setColour (C::green.withAlpha (0.7f)); g.strokePath (pb, juce::PathStrokeType (1.2f));
    }
    const double pb = proc.uiBeat.load();
    if (pb >= 0) { g.setColour (C::text); g.fillRect (bx (pb), r.getY() - 2, 1.5f, r.getHeight() + 4); }
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
    chipBtn (g, genreR, juce::String (genreName (S.genre)) + U ("  \xe2\x96\xbe"), true, C::yellow, 10.5f, hoverId == 130);
    for (int i = 0; i < 4; ++i) chipBtn (g, barsR[i], juce::String (barsOf (i)), proc.vi ("bars") == i, C::amber, 10.5f, hoverId == 140 + i);
    for (int i = 0; i < 2; ++i) chipBtn (g, perBarR[i], juce::String (i + 1), proc.vi ("perBar") == i, C::amber, 10.5f, hoverId == 150 + i);
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
        for (int i = 0; i < kVStyles; ++i) chipBtn (g, vstyleR[i], vstyleName (i), proc.vi ("vstyle") == i, C::green, 9.5f, hoverId == 180 + i);
    }
    // CLIP
    {
        const int m = proc.vi ("mode");
        for (int i = 0; i < kModes; ++i) chipBtn (g, modeR[i], modeName (i), m == i, C::violet, 12.5f, hoverId == 200 + i);
        const auto& pn = patternNames (m); const int pat = juce::jlimit (0, pn.size() - 1, proc.vi ("pat"));
        for (int i = 0; i < 9; ++i) if (i < pn.size()) chipBtn (g, patR[i], pn[i], pat == i, C::violet, 9.5f, hoverId == 210 + i);
        if (P != nullptr) drawRoll (g, *P);
        // trascina
        {
            const auto c = dragging ? C::green : C::amber;
            jlook::button (g, dragR, c, dragging, hoverId == 230, "", jkit::uf (10));
            g.setFont (jkit::uf (13)); g.setColour (c); g.drawText (U ("TRASCINA IL CLIP MIDI"), dragR.withHeight (30).translated (0, 10), juce::Justification::centred);
            g.setFont (jkit::mf (9.5f)); g.setColour (C::dim);
            const int nb = barsOf (proc.vi ("bars"));
            g.drawText (juce::String (nb) + (nb == 1 ? " battuta" : " battute") + U (" - nella traccia di Ableton"), dragR.withTrimmedTop (42).withHeight (14), juce::Justification::centred);
        }
        chipBtn (g, genR, U ("GENERA GIRO"), false, C::yellow, 10.5f, hoverId == 231); chipBtn (g, varR, U ("VARIA"), false, C::yellow, 10.5f, hoverId == 232);
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
void Editor::slotMenu (int i)
{
    const auto S = proc.settings(); const auto pr = proc.prog(); if (i >= (int) pr.size()) return;
    juce::PopupMenu m; m.addSectionHeader (U ("ACCORDI DELLA SCALA"));
    for (int d = 0; d < 7; ++d) { const auto c = slotChord (S, d); m.addItem (1 + d, degreeRoman (d, S.scale, S.key) + "   " + c.name, true, pr[(size_t) i] == d); }
    bool any = false; for (int d = 0; d < 7; ++d) any |= canBorrow (S.scale, d);
    if (any)
    {
        m.addSectionHeader (U ("PRESI IN PRESTITO (CON CAUTELA)"));
        for (int d = 0; d < 7; ++d) if (canBorrow (S.scale, d)) { const auto c = slotChord (S, 10 + d); m.addItem (11 + d, degreeRoman (10 + d, S.scale, S.key) + "   " + c.name, true, pr[(size_t) i] == 10 + d); }
    }
    juce::Component::SafePointer<Editor> sp (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withTargetScreenArea (localAreaToGlobal (scaler.map (slotR[i].toNearestInt()))), [sp, i] (int r)
    {
        if (sp == nullptr || r <= 0) return;
        auto p = sp->proc.prog(); if (i >= (int) p.size()) return;
        sp->proc.undo.beginNewTransaction(); p[(size_t) i] = r <= 7 ? r - 1 : 10 + (r - 11); sp->proc.setProg (p); sp->proc.curPreset = -1; sp->selSlot = i; sp->repaint();
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
void Editor::mouseDown (const juce::MouseEvent& e)
{
    const auto p = e.position / scaler.s;
    auto tog = [this] (const char* id) { proc.setV (id, proc.v (id) > 0.5f ? 0.0f : 1.0f); repaint(); };
    for (int i = 0; i < 12; ++i) if (keyR[i].contains (p)) { proc.setV ("key", (float) i); proc.curPreset = -1; repaint(); return; }
    for (int i = 0; i < kScales; ++i) if (scaleR[i].contains (p)) { proc.setV ("scale", (float) i); proc.curPreset = -1; repaint(); return; }
    if (genreR.contains (p)) { genreMenu(); return; }
    for (int i = 0; i < 4; ++i) if (barsR[i].contains (p)) { proc.setV ("bars", (float) i); repaint(); return; }
    for (int i = 0; i < 2; ++i) if (perBarR[i].contains (p)) { proc.setV ("perBar", (float) i); repaint(); return; }
    if (addR.contains (p) || remR.contains (p))
    {
        auto pr = proc.prog();
        if (addR.contains (p) && pr.size() < 8) pr.push_back (pr.size() % 2 == 0 ? 0 : pr.back());
        else if (remR.contains (p) && pr.size() > 1) pr.pop_back();
        proc.undo.beginNewTransaction(); proc.setProg (pr); proc.curPreset = -1; repaint(); return;
    }
    for (int i = 0; i < 8; ++i) if (slotR[i].contains (p)) { selSlot = i; slotMenu (i); repaint(); return; }
    if (tuningR.contains (p)) { proc.setV ("tuning", (float) ((proc.vi ("tuning") + 1) % kTunings)); repaint(); return; }
    for (int i = 0; i < kVStyles; ++i) if (vstyleR[i].contains (p)) { proc.setV ("vstyle", (float) i); repaint(); return; }
    for (int i = 0; i < kModes; ++i) if (modeR[i].contains (p)) { proc.setV ("mode", (float) i); proc.setV ("pat", 0); proc.curPreset = -1; repaint(); return; }
    for (int i = 0; i < 9; ++i) if (patR[i].contains (p) && i < patternNames (proc.vi ("mode")).size()) { proc.setV ("pat", (float) i); proc.curPreset = -1; repaint(); return; }
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
    const auto p = e.getMouseDownPosition().toFloat() / scaler.s;
    if (! dragR.contains (p) || dragging || e.getDistanceFromDragStart() < 6) return;
    auto f = proc.exportMidi(); if (! f.existsAsFile()) return;
    dragging = true; repaint();
    juce::Component::SafePointer<Editor> sp (this);
    performExternalDragDropOfFiles ({ f.getFullPathName() }, false, this, [sp] { if (sp != nullptr) { sp->dragging = false; sp->repaint(); } });
}
void Editor::mouseMove (const juce::MouseEvent& e)
{
    const auto p = e.position / scaler.s; int h = -1, hs = -1;
    auto chk = [&] (const juce::Rectangle<float>& r, int id) { if (r.contains (p)) h = id; };
    for (int i = 0; i < 12; ++i) chk (keyR[i], 100 + i);
    for (int i = 0; i < kScales; ++i) chk (scaleR[i], 120 + i);
    chk (genreR, 130); for (int i = 0; i < 4; ++i) chk (barsR[i], 140 + i); for (int i = 0; i < 2; ++i) chk (perBarR[i], 150 + i); chk (remR, 160); chk (addR, 161);
    chk (tuningR, 170); for (int i = 0; i < kVStyles; ++i) chk (vstyleR[i], 180 + i);
    for (int i = 0; i < kModes; ++i) chk (modeR[i], 200 + i); for (int i = 0; i < 9; ++i) chk (patR[i], 210 + i);
    chk (dragR, 230); chk (genR, 231); chk (varR, 232); chk (playR, 233); chk (liveR, 234); chk (midiR, 235); chk (csR, 236);
    for (int i = 0; i < kBodies; ++i) chk (bodyR[i], 300 + i); chk (listenR, 310); chk (themeR, 900);
    for (int i = 0; i < 8; ++i) if (slotR[i].contains (p)) hs = i;
    if (h != hoverId || hs != hoverSlot) { hoverId = h; hoverSlot = hs; repaint(); }
    static const std::map<int, const char*> tips = {
        { 130, "Il genere sceglie i giri tipici e il colore degli accordi. Cambiarlo genera un giro nuovo." },
        { 160, "Toglie l'ultimo accordo del giro." }, { 161, "Aggiunge un accordo al giro (fino a 8)." },
        { 170, "Accordatura: STANDARD, DROP D (sesta corda in Re), -1/2 TONO." },
        { 180, "APERTI: accordi con le corde a vuoto, il suono delle acustiche." }, { 181, "BARRE': forme chiuse e piene, si spostano sul manico." },
        { 182, "TRIADI ALTE: tre o quattro corde acute, per funk, R&B, afro." }, { 183, "POWER: fondamentale e quinta, per rock, punk e palm mute." }, { 184, "JAZZ: voicing di settima e nona anche senza fondamentale al basso." },
        { 200, "ACCORDI: accordi tenuti, colpi, stab, neo soul." }, { 201, "RITMICA: pennate su e giu' corda per corda, stoppate, palm mute." },
        { 202, "ARPEGGIO: fingerpicking, una corda alla volta, le note si lasciano suonare." }, { 203, "LEAD: melodia con bend, vibrato, scivolate e legati nel box della POSIZIONE." },
        { 230, "Trascina nella traccia: il clip MIDI e' gia' in tempo e lungo quanto il giro." }, { 231, "Nuovo giro dal genere e nuova suonata." }, { 232, "Stesso giro, suonato in un altro modo." },
        { 233, "Ascolta il giro anche a trasporto fermo." }, { 234, "Acceso: il giro suona con il play della DAW. Spento: la chitarra suona solo il MIDI che arriva (per esempio il clip trascinato)." },
        { 235, "Manda le note fuori dal plugin (per suonare un'altra chitarra o un sampler)." }, { 236, "Ogni corda su un canale (2-7): i bend restano sulla corda giusta in un'altra chitarra virtuale." },
        { 310, "Spenta: nessun suono, esce solo il MIDI." } };
    juce::String ni; auto it = tips.find (h); if (it != tips.end()) ni = U (it->second);
    if (hs >= 0) ni = U ("Clic per scegliere l'accordo: sopra quelli della scala, sotto quelli presi in prestito.");
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
    if (k.getKeyCode() == juce::KeyPress::spaceKey) { proc.setV ("play", proc.v ("play") > 0.5f ? 0.0f : 1.0f); repaint(); return true; }
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
