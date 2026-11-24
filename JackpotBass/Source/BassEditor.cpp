#include "BassEditor.h"
#include <map>

namespace bss
{
static void chipBtn (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& t, bool on, juce::Colour c, float fs = 10.5f, bool hover = false) { jlook::button (g, r, c, on, hover, t, jkit::uf (fs * 0.92f)); }
static void panel (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c, const juce::String& title, const juce::String& sub)
{
    jlook::softShadow (g, r, 14); jlook::card (g, r, 14, c, true, false, false);
    g.setFont (jkit::uf (11.0f)); g.setColour (c); g.drawText (title, juce::Rectangle<float> (r.getX() + 18, r.getY() + 6, 400, 16), juce::Justification::centredLeft);
    g.setFont (jkit::mf (9.5f)); g.setColour (C::dim); g.drawText (sub, juce::Rectangle<float> (r.getRight() - 618, r.getY() + 6, 600, 16), juce::Justification::centredRight);
}
static void grp (juce::Graphics& g, float x, float y, float w, const juce::String& t)
{
    g.setColour (C::dim.withAlpha (0.4f)); g.fillRect (x, y + 12, w, 1.0f);
    g.setFont (jkit::mf (9.0f)); g.setColour (C::dim); g.drawText (t, juce::Rectangle<float> (x, y, w, 12), juce::Justification::centredLeft);
}
static jkit::Knob* knobById (juce::OwnedArray<jkit::Knob>& K, const char* id) { for (auto* k : K) if (k->getName() == id) return k; return nullptr; }
static juce::Colour modeCol (int m) { return m == B808 ? C::red : m == BASSO ? C::amber : C::violet; }

Editor::Editor (Processor& p) : AudioProcessorEditor (p), proc (p)
{
    skin::init ("bass", { 0xffe96060, 0xffe9a260, 0xffc9b6ff, 0xfff4d07a }, 0xffff5a3c); C::sync();
    undoMark.p = &p; addMouseListener (&undoMark, true); setWantsKeyboardFocus (true);
    auto add = [&] (const char* id, const char* lab, juce::Colour c, std::function<juce::String (float)> f, const char* tip)
    {
        auto* prm = proc.apvts.getParameter (id);
        auto* k = knobs.add (new jkit::Knob (prm, c, lab)); k->text = [prm, f] { return f (prm->convertFrom0to1 (prm->getValue())); };
        k->setName (id); k->setHelpText (U (tip)); addAndMakeVisible (k); return k;
    };
    auto pct = [] (float v) { return v < 0.005f ? juce::String ("OFF") : juce::String (juce::roundToInt (v * 100)) + "%"; };
    add ("density", "DENSITA'", C::amber, pct, "Quante note: ogni tacca cambia il numero. Sotto il 50% restano le piu' importanti (il battere), sopra si aggiungono riempimenti piu' leggeri.");
    add ("dyn", "DINAMICA", C::amber, pct, "Differenza fra battere, accenti e note leggere.");
    add ("human", "UMANO", C::amber, pct, "Piccole imprecisioni di tempo e forza.");
    add ("swing", "SWING", C::amber, pct, "Ritarda i sedicesimi in levare.");
    add ("rest", "PAUSE", C::amber, pct, "A fine frase la linea si ferma o tiene una nota. Bassa: a fine frase c'e' invece un roll (808, synth) o una corsa (basso).");
    add ("len", "LUNGHEZZA", C::amber, pct, "Quanto durano le note: a sinistra staccate, a destra legate fino alla nota dopo.");
    add ("glide", "GLIDE", C::red, pct, "808 e synth: quante note ci arrivano scivolando (la nota prima si sovrappone: i sampler 808 e i synth mono fanno il glide).");
    add ("approach", "PASSAGGI", C::red, pct, "L'ultima nota prima del cambio d'accordo diventa la nota della scala vicina al nuovo accordo: la linea arriva per grado.");
    add ("ghost", "GHOST", C::red, pct, "Basso: note stoppate fra una nota e l'altra (funk, R&B).");
    add ("borrow", "INTERSCAMBIO", C::violet, pct, "Accordi presi in prestito. Spento: la scala si rispetta sempre.");
    add ("vary", "VARIAZIONE", C::red, pct, "Quanto ogni battuta cambia rispetto alla prima della frase: colpi che saltano o si spostano di un sedicesimo, ottave, ripetute, dinamica che respira e una corsa per grado a fine frase. A zero il pattern si ripete identico.");
    add ("color", "COLORE BASE", C::violet, [] (float v) { static const char* n[5] = { "AUTO", "TRIADI", "SETTIME", "NONE", "SUS/ADD9" }; return juce::String (n[juce::jlimit (0, 4, juce::roundToInt (v))]); }, "Colore degli accordi (decide la settima e la terza che il basso puo' usare). Ogni accordo puo' avere il suo: clic sull'accordo.");
    add ("tone", "TONO", C::phos, pct, "808: quanta parte del drive passa. Basso: dita scure o brillanti. Synth: apertura del filtro.");
    add ("drive", "DRIVE", C::phos, pct, "Saturazione.");
    add ("decay", "DECAY", C::phos, pct, "808: quanto dura la coda. Synth: quanto dura l'inviluppo del filtro.");
    add ("punch", "PUNCH", C::phos, pct, "808: il colpo di pitch all'attacco. Synth: risonanza e accento (acid).");
    add ("vol", "VOLUME", C::text, [] (float v) { return v <= -35.9f ? juce::String ("-INF") : juce::String (v, 1) + " dB"; }, "Volume del suono interno (il MIDI non cambia).");
    addAndMakeVisible (sizeChip);
    setupBrowser();
    scaler.setup();
    startTimerHz (30);
}
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
    neckR = { 690, 128, 450, 96 };
    for (int i = 0; i < 3; ++i) octR[i] = { 830.0f + i * 104, 266, 100, 24 };
    for (int i = 0; i < kBModes; ++i) modeR[i] = { 44, 344.0f + i * 44, 140, 38 };
    styleR = { 204, 344, 170, 26 }; figR = { 382, 344, 200, 26 }; figDiceR = { 586, 344, 30, 26 }; ritR = { 624, 344, 186, 26 }; ritDiceR = { 814, 344, 30, 26 };
    rotR = { 520, 376, 96, 24 }; writeR = { 620, 376, 76, 24 }; gridBtnR = { 700, 376, 144, 24 };
    rollR = { 204, 424, 640, 212 };
    dragR = { 866, 344, 272, 70 };
    genR = { 866, 452, 88, 30 }; varR = { 958, 452, 88, 30 }; rndR = { 1050, 452, 88, 30 };
    playR = { 866, 490, 134, 26 }; liveR = { 1004, 490, 134, 26 }; midiR = { 866, 522, 272, 26 };
    snapR = { 866, 568, 134, 24 }; resetEditR = { 1004, 568, 134, 24 };
    auto put = [this] (const char* id, int x, int y) { if (auto* k = knobById (knobs, id)) k->setBounds (x, y, 74, 100); };
    int x = 44; for (auto* id : { "density", "dyn", "human", "swing", "rest", "len" }) { put (id, x, 696); x += 80; }
    x += 16; for (auto* id : { "glide", "approach", "ghost" }) { put (id, x, 696); x += 80; }
    x += 16; for (auto* id : { "borrow", "color", "vary" }) { put (id, x, 696); x += 80; }
    x = 44; for (auto* id : { "tone", "drive", "decay", "punch" }) { put (id, x, 812); x += 80; }
    put ("vol", 1050, 812);
    listenR = { 400, 846, 220, 26 };
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
    jlook::backdrop (g, { 0, 0, Wf, Hf }, C::red, C::amber);
    jlook::header (g, { 0, 0, Wf, 72 }, "BASS", C::red, U ("LINEE DI BASSO  /  808  BASSO  SYNTH"), C::amber, C::violet);
    panel (g, { 24, 88, 604, 214 }, C::amber, U ("GIRO"), U ("TONALITA' / SCALA / GENERE / ACCORDI"));
    panel (g, { 644, 88, 512, 214 }, C::red, U ("MANICO"), U ("LA LINEA SUL BASSO A 4 CORDE"));
    panel (g, { 24, 318, 1132, 334 }, C::violet, U ("LINEA"), U ("PIANO ROLL MODIFICABILE: CLIC SUGLI ACCORDI IN ALTO PER CAMBIARLI, G = GLIDE SULLE NOTE SCELTE"));
    panel (g, { 24, 668, 1132, 250 }, C::phos, U (""), U ("IL SUONO E' SOLO PER ASCOLTARE: IL MIDI ESCE UGUALE"));
    grp (g, 276, 172, 84, U ("BATTUTE")); grp (g, 428, 172, 110, U ("ACC. PER BATT.")); grp (g, 560, 172, 60, U ("ACCORDI")); grp (g, 156, 172, 106, U ("MOOD"));
    grp (g, 44, 172, 106, U ("GENERE")); grp (g, 44, 213, 580, U ("GIRO (CLIC PER CAMBIARE L'ACCORDO E IL SUO COLORE)"));
    grp (g, 830, 252, 308, U ("OTTAVA"));
    grp (g, 44, 682, 474, U ("SUONATA")); grp (g, 540, 682, 232, U ("NOTE")); grp (g, 796, 682, 232, U ("ARMONIA"));
    grp (g, 44, 798, 312, U ("SUONO")); grp (g, 1050, 798, 74, U ("USCITA"));
}
void Editor::drawNeck (juce::Graphics& g, const Part& P, int slot)
{
    const auto r = neckR; const int NF = 15; const auto tun = bassTuning();
    const float fw = (r.getWidth() - 30) / NF, sh = r.getHeight() / 3.0f;
    auto fx = [&] (int f) { return f == 0 ? r.getX() + 10 : r.getX() + 30 + (f - 0.5f) * fw; };
    auto sy = [&] (int s) { return r.getBottom() - s * sh; };
    g.setColour (C::bg.withAlpha (0.55f)); g.fillRoundedRectangle (r.expanded (8, 12), 8);
    for (int f = 0; f <= NF; ++f) { const float x = r.getX() + 30 + f * fw; g.setColour (f == 0 ? C::text.withAlpha (0.7f) : C::dim.withAlpha (0.35f)); g.fillRect (x - (f == 0 ? 1.5f : 0.5f), r.getY(), f == 0 ? 3.0f : 1.0f, r.getHeight()); }
    for (int f : { 3, 5, 7, 9, 15 }) { g.setColour (C::dim.withAlpha (0.25f)); g.fillEllipse (fx (f) - 4, r.getCentreY() - 4, 8, 8); }
    g.setFont (jkit::mf (8.5f)); g.setColour (C::faint);
    for (int f = 1; f <= NF; ++f) g.drawText (juce::String (f), juce::Rectangle<float> (fx (f) - 10, r.getBottom() + 8, 20, 10), juce::Justification::centred);
    for (int s = 0; s < 4; ++s) { g.setColour (C::text.withAlpha (0.3f + 0.05f * (4 - s))); g.fillRect (r.getX() + 30, sy (s) - 0.5f - (4 - s) * 0.3f, r.getWidth() - 30, 1.5f + (4 - s) * 0.5f); g.setColour (C::dim); g.drawText (noteName (tun[(size_t) s]), juce::Rectangle<float> (r.getX() - 16, sy (s) - 6, 14, 12), juce::Justification::centredLeft); }
    if (slot < 0 || slot >= (int) P.chords.size()) return;
    const auto& ch = P.chords[(size_t) slot];
    std::map<std::pair<int, int>, int> seen;
    for (auto& e : P.ev) if (e.slot == slot && ! e.mute && e.fret <= NF) seen[{ e.str, e.fret }]++;
    for (auto& kv : seen)
    {
        const int s = kv.first.first, f = kv.first.second; const int pc = (tun[(size_t) s] + f) % 12; const bool root = pc == ch.root;
        const float x = fx (f), y = sy (s), rad = 8.0f + juce::jmin (4, kv.second);
        g.setColour (root ? C::red : C::amber); g.fillEllipse (x - rad, y - rad, rad * 2, rad * 2);
        g.setColour (C::bg); g.setFont (jkit::uf (7.5f)); g.drawText (noteName (pc), juce::Rectangle<float> (x - 10, y - 6, 20, 12), juce::Justification::centred);
    }
}
void Editor::drawRoll (juce::Graphics& g, const Part& P)
{
    const auto r = rollR; const double L = P.len();
    g.setColour (C::bg.withAlpha (0.6f)); g.fillRoundedRectangle (r.expanded (4), 6);
    if (! editing)
    {
        int lo = 127, hi = 0; for (auto& e : P.ev) { lo = juce::jmin (lo, e.note); hi = juce::jmax (hi, e.note); }
        if (lo > hi) { lo = 28; hi = 52; }
        rollLo = lo - 3; rollHi = hi + 3; if (rollHi - rollLo < 24) { const int c = (rollLo + rollHi) / 2; rollLo = c - 12; rollHi = c + 12; }
    }
    const auto S = proc.settings(); const auto sset = scaleSet (S.key, S.scale);
    const float nh = r.getHeight() / (float) (rollHi - rollLo + 1);
    for (int n = rollLo; n <= rollHi; ++n) if (sset[(size_t) (((n % 12) + 12) % 12)]) { g.setColour (C::text.withAlpha (n % 12 == S.key ? 0.08f : 0.03f)); g.fillRect (r.getX(), ny (n), r.getWidth(), nh); }
    g.setFont (jkit::mf (8)); g.setColour (C::faint);
    for (int n = rollLo; n <= rollHi; ++n) if (n % 12 == S.key) g.drawText (juce::String (noteName (n % 12)) + juce::String (n / 12 - 1), juce::Rectangle<float> (r.getX() + 2, ny (n), 30, nh), juce::Justification::centredLeft);
    const double gd = grid() > 0 ? grid() : 0.25;
    for (double b = 0; b <= L + 1e-6; b += gd) { const bool bar = std::abs (std::fmod (b, 4.0)) < 1e-6, beat = std::abs (b - std::round (b)) < 1e-6; g.setColour (C::dim.withAlpha (bar ? 0.5f : beat ? 0.2f : 0.07f)); g.fillRect (bx (b, L), r.getY(), 1.0f, r.getHeight()); }
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
    const auto mc = modeCol (S.mode);
    for (size_t i = 0; i < P.ev.size(); ++i)
    {
        const auto& e = P.ev[i]; const auto nr = noteRect (e, L);
        // il glide: una linea che entra nella nota da quella prima
        if (e.art == 2 && i > 0) { const auto pr = noteRect (P.ev[i - 1], L); g.setColour (C::text.withAlpha (0.5f)); g.drawLine (nr.getX() - 4, pr.getCentreY(), nr.getX() + 2, nr.getCentreY(), 1.5f); }
        g.setColour ((e.mute ? C::dim : mc).withAlpha (e.mute ? 0.5f : 0.35f + 0.6f * e.vel)); g.fillRect (nr);
        if (isSel[i]) { g.setColour (C::text); g.drawRect (nr.expanded (1), 1.5f); }
    }
    if (editMode == 3) { g.setColour (C::text.withAlpha (0.15f)); g.fillRect (marquee); g.setColour (C::text.withAlpha (0.6f)); g.drawRect (marquee, 1.0f); }
    const double pb = proc.uiBeat.load();
    if (pb >= 0) { g.setColour (C::text); g.fillRect (bx (pb, L), r.getY() - 2, 1.5f, r.getHeight() + 4); }
}
juce::Rectangle<float> Editor::gridCell (int row, int k) const
{
    const float lx = rollR.getX() + 70, cw = (rollR.getWidth() - 74) / 16.0f, ch = (rollR.getHeight() - 20) / 5.0f;
    return { lx + k * cw, rollR.getY() + 10 + row * ch, cw - 3, ch - 4 };
}
void Editor::drawGrid (juce::Graphics& g)
{
    g.setColour (C::bg.withAlpha (0.6f)); g.fillRoundedRectangle (rollR.expanded (4), 6);
    const auto gr = proc.grid(); static const char* lab[5] = { "OTTAVA SU", "QUINTA", "TERZA", "RADICE", "OTTAVA GIU'" };
    for (int r = 0; r < 5; ++r)
    {
        const auto c0 = gridCell (r, 0);
        g.setFont (jkit::uf (9.0f)); g.setColour (r == 3 ? C::red : C::amber); g.drawText (lab[r], juce::Rectangle<float> (rollR.getX() + 6, c0.getY(), 64, c0.getHeight()), juce::Justification::centredLeft);
        for (int k = 0; k < 16; ++k) { const auto c = gridCell (r, k); const bool on = gr[(size_t) r] & (1 << k); g.setColour (on ? (r == 3 ? C::red : C::amber) : C::dim.withAlpha (k % 4 == 0 ? 0.25f : 0.12f)); g.fillRoundedRectangle (c, 3); }
    }
    const double pb = proc.uiBeat.load();
    if (pb >= 0) { const int k = (int) (std::fmod (pb, 4.0) * 4.0); const auto c = gridCell (0, k); g.setColour (C::text.withAlpha (0.5f)); g.fillRect (c.getX(), rollR.getY() + 4, 2.0f, rollR.getHeight() - 8); }
    g.setFont (jkit::mf (9)); g.setColour (C::dim);
    g.drawText (U ("una battuta in sedicesimi, ripetuta. Segue da sola gli accordi del giro."), juce::Rectangle<float> (rollR.getX() + 70, rollR.getBottom() - 12, rollR.getWidth() - 74, 12), juce::Justification::centredLeft);
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
    jkit::lcd (g, presetR, 8, C::red); jlook::glass (g, presetR, 8);
    {
        juce::String gname ("I MIEI"); if (proc.curPreset >= 0) gname = juce::String (presets()[(size_t) proc.curPreset].name).upToFirstOccurrenceOf (" | ", false, false);
        g.setFont (jkit::mf (9)); g.setColour (C::dim); g.drawText (gname, presetR.withHeight (12).translated (0, 2), juce::Justification::centred);
        glowText (g, proc.presetName.toUpperCase(), presetR.withTrimmedTop (10), C::red, mf (12.0f), juce::Justification::centred);
    }
    g.setFont (jkit::uf (16)); g.setColour (C::red); g.drawText ("<", prevR, juce::Justification::centred); g.drawText (">", nextR, juce::Justification::centred);
    chipBtn (g, undoR, U ("\xe2\x86\xb6 ANNULLA"), false, proc.undo.canUndo() ? C::green : C::dim, 9.5f);
    chipBtn (g, redoR, U ("RIPETI \xe2\x86\xb7"), false, proc.undo.canRedo() ? C::green : C::dim, 9.5f);
    skin::chipDraw (g, themeR, hoverId == 900);
    for (int i = 0; i < 12; ++i) chipBtn (g, keyR[i], noteName (i), S.key == i, C::amber, 10.5f, hoverId == 100 + i);
    for (int i = 0; i < kScales; ++i) chipBtn (g, scaleR[i], scaleName (i), S.scale == i, C::amber, 10.0f, hoverId == 120 + i);
    chipBtn (g, genreR, juce::String (genreName (S.genre)) + U (" \xe2\x96\xbe"), true, C::yellow, 9.5f, hoverId == 130);
    chipBtn (g, moodR, juce::String (moodName (S.mood)) + U (" \xe2\x96\xbe"), true, C::yellow, 9.5f, hoverId == 132);
    for (int i = 0; i < 4; ++i) chipBtn (g, barsR[i], juce::String (barsOf (i + 1)), proc.vi ("bars") == i + 1, C::amber, 10.5f, hoverId == 140 + i);
    { static const char* pb[3] = { "1", "2", "1/2" }; for (int i = 0; i < 3; ++i) chipBtn (g, perBarR[i], pb[i], proc.vi ("perBar") == i, C::amber, 10.5f, hoverId == 150 + i); }
    const auto pr = proc.prog();
    chipBtn (g, remR, "-", false, pr.size() > 1 ? C::amber : C::dim, 13, hoverId == 160); chipBtn (g, addR, "+", false, pr.size() < 8 ? C::amber : C::dim, 13, hoverId == 161);
    Settings gs; gs.key = S.key; gs.scale = S.scale; gs.genre = S.genre; gs.color = S.color;
    for (int i = 0; i < 8; ++i)
    {
        const auto r = slotR[i];
        if (i >= (int) pr.size()) { g.setColour (C::dim.withAlpha (0.15f)); g.drawRoundedRectangle (r.reduced (2), 8, 1.0f); continue; }
        const auto c = slotChord (gs, pr[(size_t) i]);
        const bool here = proc.uiPlaying.load() && proc.uiSlot.load() >= 0 && (proc.uiSlot.load() % (int) pr.size()) == i;
        const auto col = c.borrowed ? C::red : C::amber;
        jlook::button (g, r.withTrimmedTop (22), col, here || selSlot == i, hoverSlot == i, c.name, jkit::uf (c.name.length() > 6 ? 10.5f : 13.0f));
        g.setFont (jkit::uf (9.0f)); g.setColour (c.borrowed ? C::red : C::dim); g.drawText (degreeRoman (pr[(size_t) i], S.scale, S.key) + (c.borrowed ? U (" prestito") : U ("")), r.withHeight (20), juce::Justification::centred);
    }
    // MANICO
    {
        const int sl = proc.uiPlaying.load() && proc.uiSlot.load() >= 0 ? proc.uiSlot.load() : (P != nullptr ? juce::jlimit (0, juce::jmax (0, (int) P->chords.size() - 1), selSlot) : 0);
        if (P != nullptr && sl < (int) P->chords.size())
        {
            glowText (g, P->chords[(size_t) sl].name, { 760, 100, 160, 22 }, C::red, jkit::uf (18), juce::Justification::centredLeft);
            drawNeck (g, *P, sl);
        }
        static const char* on[3] = { "-1 OTTAVA", "NORMALE", "+1 OTTAVA" };
        for (int i = 0; i < 3; ++i) chipBtn (g, octR[i], on[i], proc.vi ("octave") == i - 1, C::red, 9, hoverId == 170 + i);
        g.setFont (jkit::mf (9.5f)); g.setColour (C::dim);
        const int lo = regLo (S); g.drawText (U ("fondamentale fra ") + noteName (lo % 12) + juce::String (lo / 12 - 1) + U (" e ") + noteName ((lo + 11) % 12) + juce::String ((lo + 11) / 12 - 1), juce::Rectangle<float> (672, 266, 156, 24), juce::Justification::centredLeft);
    }
    // LINEA
    {
        const int m = S.mode, fg = S.fig;
        for (int i = 0; i < kBModes; ++i) chipBtn (g, modeR[i], bmodeName (i), m == i, modeCol (i), 13.0f, hoverId == 200 + i);
        const auto& sl = bstyles (m);
        chipBtn (g, styleR, juce::String ("STILE: ") + sl[(size_t) juce::jlimit (0, (int) sl.size() - 1, proc.vi ("style"))].name + U (" \xe2\x96\xbe"), false, modeCol (m), 9, hoverId == 280);
        chipBtn (g, figR, juce::String ("FIGURA: ") + bfigOf (fg).name + U (" \xe2\x96\xbe"), true, C::amber, 9.0f, hoverId == 281);
        chipBtn (g, figDiceR, U ("\xe2\x86\xbb"), false, C::amber, 12, hoverId == 282);
        chipBtn (g, ritR, juce::String ("RITMO: ") + britOf (S.rit).name + U (" \xe2\x96\xbe"), true, C::violet, 9.0f, hoverId == 283);
        chipBtn (g, ritDiceR, U ("\xe2\x86\xbb"), false, C::violet, 12, hoverId == 284);
        g.setFont (jkit::mf (10)); g.setColour (C::dim);
        const juce::String fs = fg == kBLibera ? S.figSeq : juce::String (bfigOf (fg).seq);
        g.drawText (fg == kBGrid ? U ("figura disegnata nella griglia") : U ("note: ") + fs + U ("  (R 5 3 7 8 L 2 ^, ~ = glide)"), juce::Rectangle<float> (204, 376, 312, 24), juce::Justification::centredLeft);
        chipBtn (g, rotR, juce::String ("PARTENZA ") + juce::String (proc.vi ("rot") + 1), proc.vi ("rot") > 0, C::amber, 9, hoverId == 286);
        chipBtn (g, writeR, U ("SCRIVI"), fg == kBLibera, C::amber, 9, hoverId == 287);
        chipBtn (g, gridBtnR, gridEdit ? U ("TORNA AL PIANO ROLL") : U ("DISEGNA LA FIGURA"), gridEdit, C::amber, 9, hoverId == 285);
        if (gridEdit) drawGrid (g); else if (P != nullptr) drawRoll (g, editing ? edPart : *P);
        const auto c = dragging ? C::green : C::red;
        jlook::button (g, dragR, c, dragging, hoverId == 230, "", jkit::uf (10));
        g.setFont (jkit::uf (13)); g.setColour (c); g.drawText (U ("TRASCINA IL CLIP MIDI"), dragR.withHeight (30).translated (0, 8), juce::Justification::centred);
        g.setFont (jkit::mf (9.5f)); g.setColour (C::dim); g.drawText (juce::String (barsOf (proc.vi ("bars"))) + U (" battute - nella traccia del tuo basso"), dragR.withTrimmedTop (40).withHeight (14), juce::Justification::centred);
        chipBtn (g, genR, U ("GENERA GIRO"), false, C::yellow, 9.5f, hoverId == 231); chipBtn (g, varR, U ("VARIA"), false, C::yellow, 9.5f, hoverId == 232); chipBtn (g, rndR, U ("A CASO"), false, C::red, 9.5f, hoverId == 237);
        chipBtn (g, playR, proc.v ("play") > 0.5f ? U ("\xe2\x96\xa0 FERMA") : U ("\xe2\x96\xb6 PROVA"), proc.v ("play") > 0.5f, C::green, 10, hoverId == 233);
        chipBtn (g, liveR, U ("GIRO IN PLAY"), proc.v ("live") > 0.5f, C::green, 10, hoverId == 234);
        chipBtn (g, midiR, U ("MIDI OUT"), proc.v ("midiOut") > 0.5f, C::green, 10, hoverId == 235);
        { static const char* sn[4] = { "GRIGLIA 1/16", "GRIGLIA 1/8", "GRIGLIA TERZINE", "GRIGLIA LIBERA" }; chipBtn (g, snapR, sn[snapIdx], false, C::violet, 9, hoverId == 270); }
        const bool ed = proc.edited();
        chipBtn (g, resetEditR, ed ? U ("TORNA AL GENERATO") : U ("NON MODIFICATO"), ed, ed ? C::red : C::dim, 9, hoverId == 271);
    }
    chipBtn (g, listenR, proc.v ("listen") > 0.5f ? juce::String ("SUONO ") + bmodeName (S.mode) + U (" ACCESO") : U ("SUONO SPENTO (SOLO MIDI)"), proc.v ("listen") > 0.5f, C::phos, 9.5f, hoverId == 310);
    g.setFont (jkit::mf (10)); g.setColour (C::dim);
    g.drawText (info.isNotEmpty() ? info : U ("Clic su un accordo per cambiarlo. GENERA GIRO = nuovo giro e nuovo stile, VARIA = stessa armonia suonata in un altro modo. Cmd+Z annulla."), juce::Rectangle<float> (30, (float) H - 24, 1030, 14), juce::Justification::centredLeft);
}
void Editor::repaintAll() { std::function<void (juce::Component*)> all = [&all] (juce::Component* c) { c->repaint(); for (auto* q : c->getChildren()) all (q); }; all (this); }

// ---------------------------------------------------------------------------------------------- piano roll
void Editor::storeSig (const Part& P) { selSig.clear(); for (int i : sel) if (i >= 0 && i < (int) P.ev.size()) selSig.push_back ({ P.ev[(size_t) i].beat, P.ev[(size_t) i].note }); }
void Editor::resolveSel (const Part& P)
{
    sel.clear();
    for (auto& sg : selSig) for (int i = 0; i < (int) P.ev.size(); ++i) { const auto& e = P.ev[(size_t) i]; if (std::abs (e.beat - sg.beat) < 1e-6 && e.note == sg.note) { sel.push_back (i); break; } }
}
int Editor::scaleShift (int note, int steps) const
{
    const auto S = proc.settings(); const auto ss = scaleSet (S.key, S.scale);
    int n = note; const int dir = steps > 0 ? 1 : -1;
    for (int k = 0; k < std::abs (steps); ++k) { do { n += dir; } while (n > 0 && n < 127 && ! ss[(size_t) (n % 12)]); }
    return juce::jlimit (12, 96, n);
}
int Editor::stepsBetween (int a, int b) const
{
    const auto S = proc.settings(); const auto ss = scaleSet (S.key, S.scale);
    int k = 0; const int dir = b > a ? 1 : -1; for (int n = a; n != b; n += dir) if (ss[(size_t) ((n + dir) % 12)]) k += dir;
    return k;
}
int Editor::snapScale (int n) const { const auto S = proc.settings(); const auto ss = scaleSet (S.key, S.scale); for (int k = 0; k < 12 && ! ss[(size_t) (((n % 12) + 12) % 12)]; ++k) n += (k % 2 == 0 ? k + 1 : -(k + 1)); return n; }
static void fixBassString (Ev& e) { const auto tun = bassTuning(); int bs = 0; for (int s = 3; s >= 0; --s) if (e.note - tun[(size_t) s] >= 0 && e.note - tun[(size_t) s] <= 12) { bs = s; break; } e.str = bs; e.fret = juce::jmax (0, e.note - tun[(size_t) bs]); }
void Editor::commitEdit()
{
    storeSig (edPart);
    std::stable_sort (edPart.ev.begin(), edPart.ev.end(), [] (const Ev& a, const Ev& b) { return a.beat < b.beat; });
    proc.commitEdit (edPart); resolveSel (edPart); repaint();
}
void Editor::deleteSelected()
{
    auto P = proc.part(); if (P == nullptr) return;
    edPart = *P; resolveSel (edPart); if (sel.empty()) return;
    std::vector<bool> del (edPart.ev.size(), false); for (int i : sel) del[(size_t) i] = true;
    std::vector<Ev> keep; for (size_t i = 0; i < edPart.ev.size(); ++i) if (! del[i]) keep.push_back (edPart.ev[i]);
    edPart.ev = keep; sel.clear(); selSig.clear(); commitEdit();
}

// ---------------------------------------------------------------------------------------------- menu
void Editor::slotMenu (int i, juce::Rectangle<float> at)
{
    const auto S = proc.settings(); const auto pr = proc.prog(); if (i >= (int) pr.size()) return;
    Settings gs; gs.key = S.key; gs.scale = S.scale; gs.genre = S.genre; gs.color = S.color;
    const int cur = pr[(size_t) i], curDeg = codeDeg (cur), curCol = codeCol (cur);
    juce::PopupMenu m; m.addSectionHeader (U ("ACCORDI DELLA SCALA"));
    for (int d = 0; d < 7; ++d) { const auto c = slotChord (gs, withCol (d, curCol)); m.addItem (1 + d, degreeRoman (d, S.scale, S.key) + "   " + c.name, true, curDeg == d); }
    bool any = false; for (int d = 0; d < 7; ++d) any |= canBorrow (S.scale, d);
    if (any)
    {
        m.addSectionHeader (U ("PRESI IN PRESTITO (CON CAUTELA)"));
        for (int d = 0; d < 7; ++d) if (canBorrow (S.scale, d)) { const auto c = slotChord (gs, withCol (10 + d, curCol)); m.addItem (11 + d, degreeRoman (10 + d, S.scale, S.key) + "   " + c.name, true, curDeg == 10 + d); }
    }
    m.addSectionHeader (U ("COLORE DI QUESTO ACCORDO"));
    m.addItem (99, juce::String ("AUTO (colore base)"), true, curCol < 0);
    for (int c = 0; c < kColors; ++c) if (c != 3) { const auto ch = slotChord (gs, withCol (curDeg, c)); m.addItem (100 + c, juce::String (colorName (c)) + "   " + ch.name, true, curCol == c); }
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
void Editor::genreMenu()
{
    juce::PopupMenu m; m.addSectionHeader (U ("GENERE (GIRI E STILE DEL BASSO)"));
    for (int g = 0; g < kGenres; ++g) m.addItem (1 + g, genreName (g), true, proc.vi ("genre") == g);
    juce::Component::SafePointer<Editor> sp (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withTargetScreenArea (localAreaToGlobal (scaler.map (genreR.toNearestInt()))), [sp] (int r)
    { if (sp == nullptr || r <= 0) return; sp->proc.undo.beginNewTransaction(); sp->proc.setV ("genre", (float) (r - 1)); sp->proc.generaGiro(); sp->repaint(); });
}
void Editor::moodMenu()
{
    juce::PopupMenu m; m.addSectionHeader (U ("MOOD"));
    for (int i = 0; i < kMoods; ++i) m.addItem (1 + i, i == 0 ? juce::String ("AUTO (dal genere: ") + moodName (moodOfGenre (proc.vi ("genre"))) + ")" : juce::String (moodName (i)), true, proc.vi ("mood") == i);
    juce::Component::SafePointer<Editor> sp (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withTargetScreenArea (localAreaToGlobal (scaler.map (moodR.toNearestInt()))), [sp] (int r)
    { if (sp == nullptr || r <= 0) return; sp->proc.undo.beginNewTransaction(); sp->proc.setV ("mood", (float) (r - 1)); sp->proc.curPreset = -1; sp->repaint(); });
}
void Editor::styleMenu()
{
    const int md = proc.vi ("mode"); const auto& L = bstyles (md);
    juce::PopupMenu m; m.addSectionHeader (juce::String ("STILI ") + bmodeName (md) + U (" (FIGURA + RITMO + GLIDE + LUNGHEZZA)"));
    for (int i = 0; i < (int) L.size(); ++i) m.addItem (1 + i, juce::String (L[(size_t) i].name) + "   (" + bfigOf (L[(size_t) i].fig).name + " / " + britOf (L[(size_t) i].rit).name + ")", true, proc.vi ("style") == i);
    juce::Component::SafePointer<Editor> sp (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withTargetScreenArea (localAreaToGlobal (scaler.map (styleR.toNearestInt()))), [sp] (int r)
    { if (sp == nullptr || r <= 0) return; sp->proc.undo.beginNewTransaction(); sp->proc.applyStyle (r - 1); sp->proc.curPreset = -1; sp->gridEdit = false; sp->repaint(); });
}
void Editor::figMenu()
{
    juce::PopupMenu m; m.addSectionHeader (U ("FIGURA: QUALI NOTE (R RADICE, 5 QUINTA, 3 TERZA, 7 SETTIMA, 8 OTTAVA SU, L OTTAVA GIU', 2 NONA, ^ PASSAGGIO, ~ GLIDE)"));
    for (int i = 0; i < kBFigs; ++i)
    {
        const juce::String d = i == kBGrid ? U ("   (la disegni nella griglia)") : i == kBLibera ? U ("   (la scrivi tu: ") + proc.settings().figSeq + ")" : juce::String ("   ") + bfigOf (i).seq;
        if (i == kBLibera) m.addSeparator();
        m.addItem (1 + i, juce::String (bfigOf (i).name) + d, true, proc.vi ("fig") == i);
    }
    m.addItem (900, U ("FIGURA A CASO (NUOVA)"));
    juce::Component::SafePointer<Editor> sp (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withTargetScreenArea (localAreaToGlobal (scaler.map (figR.toNearestInt()))), [sp] (int r)
    {
        if (sp == nullptr || r <= 0) return;
        if (r == 900) { sp->randomFig(); return; }
        if (r - 1 == kBLibera) { sp->askFigure(); return; }
        sp->proc.undo.beginNewTransaction(); sp->proc.setV ("fig", (float) (r - 1)); sp->proc.setV ("rot", 0); sp->gridEdit = r - 1 == kBGrid; sp->proc.curPreset = -1; sp->repaint();
    });
}
void Editor::ritMenu()
{
    juce::PopupMenu m; m.addSectionHeader (U ("RITMO DELLA LINEA (DENSITA' TOGLIE O AGGIUNGE NOTE)"));
    for (int i = 0; i < kBRits; ++i) m.addItem (1 + i, britOf (i).name, true, proc.vi ("rit") == i);
    juce::Component::SafePointer<Editor> sp (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withTargetScreenArea (localAreaToGlobal (scaler.map (ritR.toNearestInt()))), [sp] (int r)
    { if (sp == nullptr || r <= 0) return; sp->proc.undo.beginNewTransaction(); sp->proc.setV ("rit", (float) (r - 1)); sp->proc.curPreset = -1; sp->repaint(); });
}
void Editor::randomFig()
{
    auto& R = juce::Random::getSystemRandom(); proc.undo.beginNewTransaction();
    if (R.nextBool()) { int f = proc.vi ("fig"), n = f; while (n == f) n = R.nextInt (kBLibera); proc.setV ("fig", (float) n); }
    else proc.setFigSeq (randomBassFigure (R));
    proc.setV ("rot", 0); proc.curPreset = -1; gridEdit = false; repaint();
}
void Editor::askFigure()
{
    auto* w = new juce::AlertWindow (U ("Scrivi la figura"), U ("Le note dell'accordo nell'ordine, separate da spazi.\nR radice, 5 quinta, 3 terza, 7 settima, 8 ottava sopra, L ottava sotto, 2 nona, ^ passaggio verso l'accordo dopo.\n~ davanti = ci arrivi scivolando (glide). Esempi: R 8 5 R    R ~8 R 5    R R 3 ^"), juce::MessageBoxIconType::NoIcon, this);
    w->addTextEditor ("fig", proc.settings().figSeq);
    w->addButton ("OK", 1, juce::KeyPress (juce::KeyPress::returnKey)); w->addButton ("ANNULLA", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    juce::Component::SafePointer<Editor> sp (this);
    w->enterModalState (true, juce::ModalCallbackFunction::create ([sp, w] (int r)
    {
        if (sp == nullptr || r != 1) return;
        juce::StringArray t = juce::StringArray::fromTokens (w->getTextEditorContents ("fig").toUpperCase(), " ,", "");
        for (auto& x : t) x = x.retainCharacters ("~R5378L2^");
        t.removeEmptyStrings(); if (t.isEmpty()) return;
        sp->proc.undo.beginNewTransaction(); sp->proc.setFigSeq (t.joinIntoString (" ")); sp->proc.setV ("rot", 0); sp->proc.curPreset = -1; sp->gridEdit = false; sp->repaint();
    }), true);
}

// ---------------------------------------------------------------------------------------------- mouse
void Editor::mouseDown (const juce::MouseEvent& e)
{
    const auto p = e.position / scaler.s;
    auto tog = [this] (const char* id) { proc.setV (id, proc.v (id) > 0.5f ? 0.0f : 1.0f); repaint(); };
    for (int i = 0; i < 12; ++i) if (keyR[i].contains (p)) { proc.setV ("key", (float) i); proc.curPreset = -1; repaint(); return; }
    for (int i = 0; i < kScales; ++i) if (scaleR[i].contains (p)) { proc.setV ("scale", (float) i); proc.curPreset = -1; repaint(); return; }
    if (genreR.contains (p)) { genreMenu(); return; }
    if (moodR.contains (p)) { moodMenu(); return; }
    for (int i = 0; i < 4; ++i) if (barsR[i].contains (p)) { proc.setV ("bars", (float) (i + 1)); repaint(); return; }
    if (rndR.contains (p)) { proc.aCaso(); repaint(); return; }
    for (int i = 0; i < 3; ++i) if (perBarR[i].contains (p)) { proc.setV ("perBar", (float) i); repaint(); return; }
    if (addR.contains (p) || remR.contains (p))
    {
        auto pr = proc.prog();
        if (addR.contains (p) && pr.size() < 8) pr.push_back (pr.size() % 2 == 0 ? 0 : pr.back());
        else if (remR.contains (p) && pr.size() > 1) pr.pop_back();
        proc.undo.beginNewTransaction(); proc.setProg (pr); proc.curPreset = -1; repaint(); return;
    }
    for (int i = 0; i < 8; ++i) if (slotR[i].contains (p)) { selSlot = i; slotMenu (i); repaint(); return; }
    for (int i = 0; i < 3; ++i) if (octR[i].contains (p)) { proc.setV ("octave", (float) (i - 1)); repaint(); return; }
    for (int i = 0; i < kBModes; ++i) if (modeR[i].contains (p)) { proc.undo.beginNewTransaction(); proc.setV ("mode", (float) i); proc.applyStyle (0); proc.curPreset = -1; sel.clear(); selSig.clear(); repaint(); return; }
    if (styleR.contains (p)) { styleMenu(); return; }
    if (figR.contains (p)) { figMenu(); return; }
    if (ritR.contains (p)) { ritMenu(); return; }
    if (figDiceR.contains (p)) { randomFig(); return; }
    if (ritDiceR.contains (p)) { int f = proc.vi ("rit"), n = f; while (n == f) n = juce::Random::getSystemRandom().nextInt (kBRits); proc.setV ("rit", (float) n); proc.curPreset = -1; repaint(); return; }
    if (rotR.contains (p))
    {
        const auto S = proc.settings(); const auto seq = S.fig == kBLibera ? S.figSeq : juce::String (bfigOf (S.fig).seq);
        const int n = juce::jlimit (1, 8, juce::StringArray::fromTokens (seq, " ", "").size());
        proc.setV ("rot", (float) ((proc.vi ("rot") + 1) % n)); proc.curPreset = -1; repaint(); return;
    }
    if (writeR.contains (p)) { askFigure(); return; }
    if (gridBtnR.contains (p)) { gridEdit = ! gridEdit; if (gridEdit) proc.setV ("fig", (float) kBGrid); repaint(); return; }
    if (genR.contains (p)) { proc.generaGiro(); gridEdit = false; repaint(); return; }
    if (varR.contains (p)) { proc.varia(); repaint(); return; }
    if (playR.contains (p)) { tog ("play"); return; }
    if (liveR.contains (p)) { tog ("live"); return; }
    if (midiR.contains (p)) { tog ("midiOut"); return; }
    if (listenR.contains (p)) { tog ("listen"); return; }
    if (snapR.contains (p)) { snapIdx = (snapIdx + 1) % 4; repaint(); return; }
    if (resetEditR.contains (p)) { if (proc.edited()) { proc.clearEdit(); sel.clear(); selSig.clear(); } repaint(); return; }
    if (undoR.contains (p)) { doUndo (false); return; }
    if (redoR.contains (p)) { doUndo (true); return; }
    if (prevR.contains (p) || nextR.contains (p)) { const int n = (int) presets().size(); proc.loadPreset ((juce::jmax (0, proc.curPreset) + (nextR.contains (p) ? 1 : n - 1)) % n); gridEdit = false; repaintAll(); return; }
    if (themeR.contains (p)) { skin::cycle(); C::sync(); bg = {}; repaintAll(); return; }
    if (presetR.contains (p)) { browser.open(); return; }
    for (auto& cl : chordLabels) if (cl.first.contains (p) && ! gridEdit) { const auto pr = proc.prog(); slotMenu (cl.second % (int) pr.size(), cl.first); return; }
    if (gridEdit && rollR.contains (p))
    {
        auto gr = proc.grid();
        for (int r = 0; r < 5; ++r) for (int k = 0; k < 16; ++k) if (gridCell (r, k).expanded (1.5f).contains (p))
        { gridPaint = (gr[(size_t) r] & (1 << k)) ? 0 : 1; gr[(size_t) r] = (juce::uint16) (gridPaint ? gr[(size_t) r] | (1 << k) : gr[(size_t) r] & ~(1 << k)); proc.setGrid (gr); repaint(); }
        return;
    }
    if (rollR.contains (p))
    {
        auto P = proc.part(); if (P == nullptr) return;
        grabKeyboardFocus();
        edPart = *P; resolveSel (edPart); orig = edPart.ev;
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
        editing = true; storeSig (edPart); repaint();
    }
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
        {   // sposta: tempo sulla griglia, altezza per gradi della scala
            const double nb = snapB (a.beat + xb (p.x, L) - xb (d0.x, L)), db = juce::jlimit (-a.beat, L - 0.05 - a.beat, nb - a.beat);
            const int target = snapScale (yn (p.y)); const int steps = stepsBetween (a.note, target);
            const auto S = proc.settings(); const auto ss = scaleSet (S.key, S.scale);
            for (int i : sel)
            {
                auto& ev = edPart.ev[(size_t) i]; const auto& o = orig[(size_t) i];
                ev.beat = juce::jlimit (0.0, L - 0.02, o.beat + db);
                ev.note = ss[(size_t) (o.note % 12)] ? scaleShift (o.note, steps) : o.note + (target - a.note);
                fixBassString (ev);
            }
        }
        else
        {
            const double end = snapB (a.beat + a.len + xb (p.x, L) - xb (d0.x, L)), dl = end - (a.beat + a.len);
            for (int i : sel) { auto& ev = edPart.ev[(size_t) i]; const auto& o = orig[(size_t) i]; ev.len = juce::jlimit (0.03, L - o.beat, o.len + dl); }
        }
        storeSig (edPart); repaint(); return;
    }
    if (dragging || e.getDistanceFromDragStart() < 6 || ! dragR.contains (d0)) return;
    auto f = proc.exportMidi(); if (! f.existsAsFile()) return;
    dragging = true; repaint();
    juce::Component::SafePointer<Editor> sp (this);
    performExternalDragDropOfFiles ({ f.getFullPathName() }, false, this, [sp] { if (sp != nullptr) { sp->dragging = false; sp->repaint(); } });
}
void Editor::mouseUp (const juce::MouseEvent& e)
{
    gridPaint = -1;
    if (! editing) return;
    editing = false;
    if ((editMode == 1 || editMode == 2) && e.getDistanceFromDragStart() > 2) commitEdit();
    editMode = 0; repaint();
}
void Editor::mouseDoubleClick (const juce::MouseEvent& e)
{
    const auto p = e.position / scaler.s; if (! rollR.contains (p) || gridEdit) return;
    auto P = proc.part(); if (P == nullptr) return;
    editing = false; editMode = 0;
    edPart = *P; const int h = hitNote (edPart, p);
    if (h >= 0) { sel = { h }; storeSig (edPart); deleteSelected(); return; }
    Ev ev; ev.beat = juce::jlimit (0.0, edPart.len() - 0.1, grid() > 0 ? std::floor (xb (p.x, edPart.len()) / grid()) * grid() : xb (p.x, edPart.len()));
    ev.len = grid() > 0 ? grid() * 2 : 0.5; ev.note = snapScale (yn (p.y)); ev.vel = 0.75f;
    ev.slot = edPart.chordAt.empty() ? 0 : juce::jlimit (0, (int) edPart.chordAt.size() - 1, (int) std::floor (ev.beat / (edPart.len() / (double) edPart.chordAt.size())));
    fixBassString (ev);
    edPart.ev.push_back (ev); sel = { (int) edPart.ev.size() - 1 }; commitEdit();
}
void Editor::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    const auto p = e.position / scaler.s; if (! rollR.contains (p) || gridEdit) return;
    auto P = proc.part(); if (P == nullptr) return;
    edPart = *P; resolveSel (edPart); if (sel.empty()) return;
    for (int i : sel) edPart.ev[(size_t) i].vel = juce::jlimit (0.06f, 1.0f, edPart.ev[(size_t) i].vel + (w.deltaY > 0 ? 0.04f : -0.04f));
    commitEdit();
}
void Editor::mouseMove (const juce::MouseEvent& e)
{
    const auto p = e.position / scaler.s; int h = -1, hs = -1;
    auto chk = [&] (const juce::Rectangle<float>& r, int id) { if (r.contains (p)) h = id; };
    for (int i = 0; i < 12; ++i) chk (keyR[i], 100 + i);
    for (int i = 0; i < kScales; ++i) chk (scaleR[i], 120 + i);
    chk (genreR, 130); chk (moodR, 132); for (int i = 0; i < 4; ++i) chk (barsR[i], 140 + i); chk (rndR, 237); for (int i = 0; i < 3; ++i) chk (perBarR[i], 150 + i); chk (remR, 160); chk (addR, 161);
    for (int i = 0; i < 3; ++i) chk (octR[i], 170 + i);
    for (int i = 0; i < kBModes; ++i) chk (modeR[i], 200 + i);
    chk (styleR, 280); chk (figR, 281); chk (figDiceR, 282); chk (ritR, 283); chk (ritDiceR, 284); chk (gridBtnR, 285); chk (rotR, 286); chk (writeR, 287);
    chk (dragR, 230); chk (genR, 231); chk (varR, 232); chk (playR, 233); chk (liveR, 234); chk (midiR, 235); chk (snapR, 270); chk (resetEditR, 271); chk (listenR, 310); chk (themeR, 900);
    for (int i = 0; i < 8; ++i) if (slotR[i].contains (p)) hs = i;
    if (h != hoverId || hs != hoverSlot) { hoverId = h; hoverSlot = hs; repaint(); }
    static const std::map<int, const char*> tips = {
        { 130, "Il genere sceglie i giri tipici e lo stile del basso. Cambiarlo genera un giro nuovo." },
        { 132, "Mood: il carattere del giro (malinconico, aperto, ipnotico...)." },
        { 150, "Un accordo per battuta." }, { 151, "Due accordi per battuta." }, { 152, "Un accordo ogni due battute." },
        { 160, "Toglie l'ultimo accordo del giro." }, { 161, "Aggiunge un accordo al giro (fino a 8)." },
        { 170, "La linea un'ottava sotto." }, { 171, "Registro normale del modo." }, { 172, "La linea un'ottava sopra." },
        { 200, "808: note lunghe, glide, roll. Il MIDI va nel tuo 808 (sampler mono con glide)." }, { 201, "BASSO: basso elettrico, passaggi, ghost note, walking." }, { 202, "SYNTH: synth bass, ottave, acid con slide, reese." },
        { 280, "Stili pronti per il modo: figura, ritmo, glide e lunghezza." },
        { 281, "FIGURA: quali note dell'accordo e in che ordine. Ricomincia a ogni accordo." }, { 282, "Un'altra figura (a volte una nuova, mai sentita)." },
        { 283, "RITMO della linea, indipendente dalla figura." }, { 284, "Un altro ritmo, stessa figura." },
        { 285, "Disegna la figura: radice, quinta, terza, ottava su e giu' per 16 sedicesimi." },
        { 286, "PARTENZA: la stessa figura cominciata da un'altra nota." }, { 287, "Scrivi la tua figura, per esempio R ~8 5 R." },
        { 230, "Trascina nella traccia: il clip e' gia' in tempo, i glide sono note sovrapposte." }, { 231, "Nuovo giro, nuovo stile adatto al genere, nuova suonata." }, { 237, "Tutto nuovo e a caso (genere, mood, modo di suonare, giro, groove) tenendo solo tonalita', scala e lunghezza." }, { 232, "Stesso giro e stesso stile, suonati in un altro modo." },
        { 233, "Ascolta anche a trasporto fermo." }, { 234, "Acceso: la linea suona con il play. Spento: suona solo il MIDI che arriva." },
        { 235, "Manda le note fuori dal plugin, per suonare il tuo 808 o il tuo synth." },
        { 270, "Griglia del piano roll." }, { 271, "Le modifiche a mano restano finche' non cambi il giro o le manopole. Qui le togli." },
        { 310, "Spento: nessun suono, esce solo il MIDI." } };
    juce::String ni; auto it = tips.find (h); if (it != tips.end()) ni = U (it->second);
    if (hs >= 0) ni = U ("Clic per scegliere l'accordo e il suo colore.");
    for (auto& cl : chordLabels) if (cl.first.contains (p)) ni = U ("Clic per cambiare questo accordo o il suo colore.");
    if (rollR.contains (p) && ! gridEdit) ni = U ("Clic seleziona (Maiusc aggiunge), trascina sposta, bordo destro allunga, doppio clic aggiunge o toglie, rotella = forza, frecce = grado / griglia, G = glide, Canc cancella.");
    if (ni != info) { info = ni; repaint(); }
    setMouseCursor (h >= 0 || hs >= 0 || presetR.contains (p) || prevR.contains (p) || nextR.contains (p) || undoR.contains (p) || redoR.contains (p) ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
}

// ---------------------------------------------------------------------------------------------- preset, tasti
void Editor::setupBrowser()
{
    addChildComponent (browser);
    browser.accent = C::red; browser.title = "Preset";
    auto userFiles = [] { auto f = presetFolder().findChildFiles (juce::File::findFiles, false, "*.jbas"); f.sort(); return f; };
    browser.entries = [userFiles]
    {
        std::vector<jbrowse::Entry> v;
        for (int i = 0; i < (int) presets().size(); ++i)
        {
            const juce::String nm (presets()[(size_t) i].name); jbrowse::Entry e; e.id = i;
            e.group = nm.upToFirstOccurrenceOf (" | ", false, false); e.name = nm.fromFirstOccurrenceOf (" | ", false, false); e.note = U (presets()[(size_t) i].note); v.push_back (e);
        }
        const auto fs = userFiles();
        for (int i = 0; i < fs.size(); ++i) { jbrowse::Entry e; e.id = 100000 + i; e.user = true; e.group = "I MIEI"; e.name = fs[i].getFileNameWithoutExtension(); e.note = "Preset tuo, in Musica / Jackpot / Bass Preset."; v.push_back (e); }
        return v;
    };
    browser.current = [this, userFiles]
    {
        if (proc.curPreset >= 0) return proc.curPreset;
        const auto fs = userFiles(); for (int i = 0; i < fs.size(); ++i) if (fs[i].getFileNameWithoutExtension() == proc.presetName) return 100000 + i;
        return -1;
    };
    browser.load = [this] (const jbrowse::Entry& e) { if (e.user) proc.loadPresetFile (presetFolder().getChildFile (e.name + ".jbas")); else proc.loadPreset (e.id); gridEdit = false; repaintAll(); };
    browser.save = [this] { askPresetName(); };
    browser.canOverwrite = [this] { return proc.curPreset < 0 && presetFolder().getChildFile (proc.presetName + ".jbas").existsAsFile(); };
    browser.overwrite = [this] { proc.savePresetFile (presetFolder().getChildFile (proc.presetName + ".jbas")); repaint(); };
    browser.folder = [] { presetFolder().startAsProcess(); };
    browser.remove = [] (const jbrowse::Entry& e) { presetFolder().getChildFile (e.name + ".jbas").moveToTrash(); };
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
        if (nm.isNotEmpty()) { sp->proc.savePresetFile (presetFolder().getChildFile (nm + ".jbas")); sp->browser.refresh(); sp->repaint(); }
    }), true);
}
bool Editor::keyPressed (const juce::KeyPress& k)
{
    if (k.getModifiers().isCommandDown() && (k.getKeyCode() == 'Z' || k.getKeyCode() == 'z')) { doUndo (k.getModifiers().isShiftDown()); return true; }
    if (k.getModifiers().isCommandDown() && (k.getKeyCode() == 'Y' || k.getKeyCode() == 'y')) { doUndo (true); return true; }
    if (k.getKeyCode() == juce::KeyPress::deleteKey || k.getKeyCode() == juce::KeyPress::backspaceKey) { if (! selSig.empty()) { deleteSelected(); return true; } return false; }
    if (k.getModifiers().isCommandDown() && (k.getKeyCode() == 'A' || k.getKeyCode() == 'a')) { if (auto P = proc.part()) { sel.clear(); for (int i = 0; i < (int) P->ev.size(); ++i) sel.push_back (i); storeSig (*P); repaint(); } return true; }
    const int kc = k.getKeyCode();
    if (! selSig.empty() && ! k.getModifiers().isCommandDown() && (kc == 'G' || kc == 'g'))
    {   // G: glide sulle note scelte (la nota prima si allunga fino a sovrapporsi), di nuovo G lo toglie
        auto P = proc.part(); if (P == nullptr) return false;
        edPart = *P; resolveSel (edPart);
        for (int i : sel)
        {
            auto& ev = edPart.ev[(size_t) i]; ev.art = ev.art == 2 ? 0 : 2;
            if (i > 0) { auto& pv = edPart.ev[(size_t) i - 1]; pv.len = ev.art == 2 ? ev.beat - pv.beat + 0.06 : juce::jmax (0.03, juce::jmin (pv.len, ev.beat - pv.beat - 0.004)); }
        }
        commitEdit(); return true;
    }
    if (! selSig.empty() && (kc == juce::KeyPress::upKey || kc == juce::KeyPress::downKey || kc == juce::KeyPress::leftKey || kc == juce::KeyPress::rightKey))
    {
        auto P = proc.part(); if (P == nullptr) return false;
        edPart = *P; resolveSel (edPart);
        const double g = grid() > 0 ? grid() : 0.25;
        for (int i : sel)
        {
            auto& ev = edPart.ev[(size_t) i];
            if (kc == juce::KeyPress::upKey || kc == juce::KeyPress::downKey) { ev.note = scaleShift (ev.note, kc == juce::KeyPress::upKey ? 1 : -1); fixBassString (ev); }
            else ev.beat = juce::jlimit (0.0, edPart.len() - 0.02, ev.beat + (kc == juce::KeyPress::rightKey ? g : -g));
        }
        commitEdit(); return true;
    }
    return false;
}
void Editor::doUndo (bool redo) { proc.undo.beginNewTransaction(); if (redo ? proc.undo.redo() : proc.undo.undo()) repaintAll(); repaint(); }
void Editor::timerCallback()
{
    if (browser.isVisible()) return;
    if (bgTheme != skin::cur() && bgTheme >= 0) { skin::apply(); C::sync(); bg = {}; repaintAll(); }
    for (auto* k : knobs) if (k->isMouseOverOrDragging()) { if (info != k->getHelpText()) info = k->getHelpText(); }
    repaint();
}
}
