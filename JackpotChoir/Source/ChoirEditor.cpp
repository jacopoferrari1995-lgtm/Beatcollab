#include "ChoirEditor.h"

namespace jcho
{
juce::Colour Editor::voiceCol (int k) { const juce::Colour c[4] = { C::phos, C::amber, C::magenta, C::green }; return c[k & 3]; }

void PitchView::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat(); lcd (g, b, 8, ed.acc);
    auto r = b.reduced (12, 12).withTrimmedLeft (26);
    auto& E = *ed.proc.eng; const auto S = ed.proc.settings();
    // la finestra segue la voce: centro sulle ultime note sentite
    float sum = 0; int cnt = 0; for (int i = 0; i < Engine::kTrace; ++i) if (E.trLead[i] > 0) { sum += E.trLead[i]; ++cnt; }
    if (cnt > 8) { const float c = sum / cnt; const float tlo = c - 14, thi = c + 14; lo += (tlo - lo) * 0.05f; hi += (thi - hi) * 0.05f; }
    auto Y = [&] (float m) { return r.getBottom() - (m - lo) / (hi - lo) * r.getHeight(); };
    // righe: note della scala piu' chiare, la tonica evidenziata
    ScaleMap map; map.set (S.key, S.scale); std::array<bool, 12> in {}; for (int d = 0; d < map.n(); ++d) in[(size_t) ((S.key + scaleSteps (S.scale)[(size_t) d]) % 12)] = true;
    g.setFont (mf (8.0f));
    for (int m = (int) std::ceil (lo); m <= (int) hi; ++m)
    {
        const int pc = ((m % 12) + 12) % 12; const float y = Y ((float) m);
        if (S.mode == 0 && ! in[(size_t) pc]) continue;
        g.setColour ((pc == S.key ? ed.acc : C::dim).withAlpha (pc == S.key ? 0.35f : 0.12f)); g.drawHorizontalLine ((int) y, r.getX(), r.getRight());
        g.setColour (pc == S.key ? ed.acc : C::dim); g.drawText (juce::String (noteName (pc)) + juce::String (m / 12 - 1), juce::Rectangle<float> (b.getX() + 6, y - 6, 30, 12), juce::Justification::centredLeft);
    }
    const int w = E.trW.load(); const float dx = r.getWidth() / (Engine::kTrace - 1);
    auto trace = [&] (const float* t, juce::Colour c, float th)
    {
        juce::Path p; bool st = false;
        for (int k = 0; k < Engine::kTrace; ++k)
        {
            const float v = t[(w + k) % Engine::kTrace];
            if (v <= 0 || v < lo - 2 || v > hi + 2) { st = false; continue; }
            const float x = r.getX() + k * dx, y = juce::jlimit (r.getY(), r.getBottom(), Y (v));
            if (! st) { p.startNewSubPath (x, y); st = true; } else p.lineTo (x, y);
        }
        g.setColour (c.withAlpha (0.25f)); g.strokePath (p, juce::PathStrokeType (th + 3, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (c); g.strokePath (p, juce::PathStrokeType (th, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    };
    for (int k = 0; k < 4; ++k) trace (E.trV[k], Editor::voiceCol (k), 1.6f);
    trace (E.trLead, C::text, 2.2f);
    // nota di adesso
    const float n = E.uiNote.load();
    const juce::String t = n > 0 ? juce::String (noteName (juce::roundToInt (n))) + juce::String (juce::roundToInt (n) / 12 - 1) + "  " + (n - std::round (n) >= 0 ? "+" : "") + juce::String (juce::roundToInt ((n - std::round (n)) * 100)) + " cent" : juce::String ("--");
    glowText (g, t, b.withTrimmedRight (16).withHeight (30).translated (0, 6), n > 0 ? C::text : C::dim, mf (15.0f), juce::Justification::centredRight);
    g.setColour (C::dim); g.setFont (mf (9.0f));
    g.drawText (S.mode == 0 ? juce::String ("SCALA ") + noteName (S.key) + " " + scaleName (S.scale) : juce::String ("MIDI: tieni le note sulla tastiera"), b.withTrimmedRight (16).withTrimmedTop (30).withHeight (16), juce::Justification::centredRight);
}

Editor::Editor (Processor& p) : AudioProcessorEditor (&p), proc (p)
{
    skin::init ("choir", { 0xffc9b6ff, 0xff8fd8e8, 0xfff4d07a, 0xffe9a2c8 }, 0xffb388ff); C::sync(); themeRect = { 990, 15, 100, 36 };
    setLookAndFeel (&lnf); setWantsKeyboardFocus (true);
    undoMark.p = &p; addMouseListener (&undoMark, true);
    auto knob = [&] (const juce::String& id, const char* label, std::function<juce::String()> txt, juce::Colour c, bool bip = false) -> Knob*
    {
        auto k = std::make_unique<Knob> (P (id), c, label); k->text = std::move (txt); k->bipolar = bip;
        addAndMakeVisible (*k); knobs.push_back (std::move (k)); return knobs.back().get();
    };
    // quattro voci: livello, pan, formante (0..11)
    for (int v = 0; v < 4; ++v)
    {
        const juce::String pf = "v" + juce::String (v + 1) + "_"; const auto c = voiceCol (v);
        knob (pf + "lvl", "LIVELLO", [this, pf] { return dbT (proc.value (pf + "lvl")); }, c);
        knob (pf + "pan", "PAN", [this, pf] { const float x = proc.value (pf + "pan"); return std::abs (x) < 0.02f ? juce::String ("C") : (x < 0 ? "L " : "R ") + juce::String (juce::roundToInt (std::abs (x) * 100)); }, c, true);
        knob (pf + "form", "FORMANTE", [this, pf] { const float x = proc.value (pf + "form"); return std::abs (x) < 0.02f ? juce::String ("0") : (x > 0 ? juce::String ("CHIARA ") : juce::String ("SCURA ")) + juce::String (juce::roundToInt (std::abs (x) * 100)); }, c, true);
    }
    // globali (12..20)
    knob ("dry", "VOCE", [this] { return proc.value ("dry") <= -59.9f ? juce::String ("OFF") : dbT (proc.value ("dry")); }, C::text);
    knob ("choir", "CORO", [this] { return dbT (proc.value ("choir")); }, acc);
    knob ("human", "UMANO", [this] { return pctT (proc.value ("human")); }, acc);
    knob ("width", "LARGHEZZA", [this] { return pctT (proc.value ("width")); }, acc);
    knob ("cons", "CONSONANTI", [this] { return pctT (proc.value ("cons")); }, acc2);
    knob ("glide", "GLIDE", [this] { return juce::String (juce::roundToInt (20 + 180 * proc.value ("glide"))) + " ms"; }, acc2);
    knob ("lowcut", "TAGLIO BASSI", [this] { return hzT (proc.value ("lowcut")); }, acc2);
    knob ("tone", "TONO", [this] { const float x = proc.value ("tone"); return std::abs (x) < 0.02f ? juce::String ("0") : (x > 0 ? "ARIA " : "BUIO ") + juce::String (juce::roundToInt (std::abs (x) * 100)); }, acc2, true);
    knob ("out", "OUTPUT", [this] { return dbT (proc.value ("out")); }, C::text, true);
    const char* tips[] = { "Volume della voce principale (OFF = solo il coro).", "Volume di tutto il coro.", "Quanto le voci sono diverse fra loro: ritardo, intonazione che vaga, respiro. A zero sono perfette (effetto).",
                           "Quanto le voci si aprono ai lati.", "Quanto si sentono le consonanti (S, T, respiri) nelle voci del coro.", "Quanto ci mette una voce a cambiare nota.",
                           "Taglio dei bassi sul coro, cosi' non impasta la voce.", "Coro piu' scuro o piu' arioso.", "Volume finale." };
    for (int i = 0; i < 9; ++i) knobs[(size_t) (12 + i)]->setHelpText (tips[i]);
    // selettori
    juce::StringArray iv; for (int i = 0; i < kIntervals; ++i) iv.add (intervalShort (i));
    for (int v = 0; v < 4; ++v) { auto s = std::make_unique<Segs> (P ("v" + juce::String (v + 1) + "_int"), voiceCol (v), iv); s->rows = 2; s->font = 9.5f; addAndMakeVisible (*s); segs.push_back (std::move (s)); }
    { auto s = std::make_unique<Segs> (P ("mode"), acc, juce::StringArray { "SCALA", "MIDI" }); s->font = 11.0f; addAndMakeVisible (*s); segs.push_back (std::move (s)); }
    { juce::StringArray k; for (int i = 0; i < 12; ++i) k.add (noteName (i)); auto s = std::make_unique<Segs> (P ("key"), acc, k); s->rows = 2; s->font = 10.0f; addAndMakeVisible (*s); segs.push_back (std::move (s)); }
    { juce::StringArray k; for (int i = 0; i < kScales; ++i) k.add (scaleName (i)); auto s = std::make_unique<Segs> (P ("scale"), acc, k); s->rows = 2; s->font = 9.0f; addAndMakeVisible (*s); segs.push_back (std::move (s)); }
    addAndMakeVisible (pitch); addAndMakeVisible (sizeChip);
    setupBrowser();
    scaler.setup();
    startTimerHz (30);
}
void Editor::resized()
{
    const float sc = scaler.update();
    pitch.setBounds (30, 112, 676, 230);
    segs[4]->setBounds (742, 120, 330, 34);
    segs[5]->setBounds (742, 186, 330, 56);
    segs[6]->setBounds (742, 272, 330, 60);
    for (int v = 0; v < 4; ++v)
    {
        const int x = 30 + v * 266;
        segs[(size_t) v]->setBounds (x, 400, 246, 58);
        for (int k = 0; k < 3; ++k) knobs[(size_t) (v * 3 + k)]->setBounds (x + 8 + k * 82, 472, 66, 66 + 27);
    }
    for (int i = 0; i < 9; ++i) knobs[(size_t) (12 + i)]->setBounds (44 + i * 116, 622, 66, 66 + 27);
    sizeChip.setBounds (W - 76, H - 18, 58, 15);
    browser.setBounds (0, 0, W, H);
    jlook::scaleChildren (*this, sc);
}
void Editor::paint (juce::Graphics& g)
{
    skin::apply(); C::sync();
    g.addTransform (juce::AffineTransform::scale (scaler.s));
    jlook::backdrop (g, { 0, 0, (float) W, (float) H }, acc, acc2);
    jlook::header (g, { 0, 0, (float) W, 64 }, "CHOIR", acc, "", acc, acc2);
    bar.paint (g, proc.currentName(), acc, proc.ab.cur == 1, proc.ab.label());
    jlook::button (g, undoR.toFloat(), acc, false, false, juce::String::fromUTF8 ("\xe2\x86\xb6"), uf (13.0f));
    jlook::button (g, redoR.toFloat(), acc, false, false, juce::String::fromUTF8 ("\xe2\x86\xb7"), uf (13.0f));
    titledCard (g, { 18, 76, 700, 278 }, "LA TUA VOCE E IL CORO", "bianco: quello che canti  -  colori: le voci", acc);
    titledCard (g, { 730, 76, 352, 278 }, "COSA CANTANO LE VOCI", "", acc);
    g.setColour (C::dim); g.setFont (mf (9.0f));
    g.drawText ("TONALITA'", juce::Rectangle<int> (742, 166, 200, 16), juce::Justification::centredLeft);
    g.drawText ("SCALA", juce::Rectangle<int> (742, 252, 200, 16), juce::Justification::centredLeft);
    if (juce::roundToInt (proc.value ("mode")) == 1)
    {
        juce::String h; for (int k = 0; k < 4; ++k) { const int n = proc.uiHeld[(size_t) k].load(); if (n >= 0) h << noteName (n) << (n / 12 - 1) << "  "; }
        g.setColour (acc); g.drawText (h.isEmpty() ? juce::String ("nessuna nota tenuta: le voci tacciono") : "TENUTE: " + h, juce::Rectangle<int> (742, 334, 330, 14), juce::Justification::centredLeft);
    }
    for (int v = 0; v < 4; ++v)
    {
        const int x = 18 + v * 266; const auto c = voiceCol (v); const int iv = juce::roundToInt (proc.value ("v" + juce::String (v + 1) + "_int"));
        titledCard (g, { (float) x, 366, 258, 200 }, "VOCE " + juce::String (v + 1), iv == I_OFF ? juce::String ("spenta") : juce::String (intervalName (iv)).toLowerCase(), c, iv != I_OFF);
    }
    titledCard (g, { 18, 576, 1064, 152 }, "CORO", "", acc2);
    skin::chipDraw (g, themeRect.toFloat());
}
void Editor::mouseMove (const juce::MouseEvent& e0) { const auto e = e0.withNewPosition (e0.position / scaler.s); const bool h = bar.ab.contains (e.getPosition()); if (h != bar.abHover) { bar.abHover = h; repaint(); } }
void Editor::mouseDown (const juce::MouseEvent& e0)
{
    { const auto tp = e0.position / scaler.s; if (themeRect.toFloat().contains (tp)) { skin::cycle(); C::sync(); for (auto* ch : getChildren()) ch->repaint(); repaint(); return; } }
    const auto e = e0.withNewPosition (e0.position / scaler.s);
    if (browser.isVisible()) return;
    if (undoR.contains (e.getPosition())) { doUndo (false); return; }
    if (redoR.contains (e.getPosition())) { doUndo (true); return; }
    if (bar.preset.contains (e.getPosition())) { browser.open(); return; }
    if (proc.preset < 0 && (bar.prev.contains (e.getPosition()) || bar.next.contains (e.getPosition()))) { proc.loadPreset (0); repaint(); return; }
    bar.click (*this, proc, presets(), e.getPosition(), localAreaToGlobal (scaler.map (bar.preset)));
}
bool Editor::keyPressed (const juce::KeyPress& k)
{
    if (k.getModifiers().isCommandDown() && (k.getKeyCode() == 'Z' || k.getKeyCode() == 'z')) { doUndo (k.getModifiers().isShiftDown()); return true; }
    return false;
}
void Editor::doUndo (bool redo)
{
    proc.undo.beginNewTransaction();
    if (redo ? proc.undo.redo() : proc.undo.undo()) { for (auto* ch : getChildren()) ch->repaint(); repaint(); }
}
void Editor::setupBrowser()
{
    addChildComponent (browser);
    browser.accent = acc; browser.title = "Preset";
    auto userFiles = [] { auto f = Processor::presetFolder().findChildFiles (juce::File::findFiles, false, "*.jchoir"); f.sort(); return f; };
    browser.entries = [userFiles]
    {
        std::vector<jbrowse::Entry> v;
        for (int i = 0; i < (int) presets().size(); ++i)
        {
            const juce::String n (presets()[(size_t) i].name);
            jbrowse::Entry e; e.id = i; e.note = presets()[(size_t) i].note;
            e.group = n.contains (" | ") ? n.upToFirstOccurrenceOf (" | ", false, false) : juce::String ("BASE");
            e.name = n.contains (" | ") ? n.fromFirstOccurrenceOf (" | ", false, false) : n;
            v.push_back (e);
        }
        const auto fs = userFiles();
        for (int i = 0; i < fs.size(); ++i) { jbrowse::Entry e; e.id = 100000 + i; e.user = true; e.group = "I MIEI"; e.name = fs[i].getFileNameWithoutExtension(); e.note = "Preset tuo, salvato in Musica / Jackpot / Choir Preset."; v.push_back (e); }
        return v;
    };
    browser.current = [this, userFiles]
    {
        if (proc.userName.isEmpty()) return proc.preset;
        const auto fs = userFiles(); for (int i = 0; i < fs.size(); ++i) if (fs[i].getFileNameWithoutExtension() == proc.userName) return 100000 + i;
        return -1;
    };
    browser.load = [this] (const jbrowse::Entry& e)
    {
        if (e.user) proc.loadUserPreset (Processor::presetFolder().getChildFile (e.name + ".jchoir")); else proc.loadPreset (e.id);
        repaint();
    };
    browser.save = [this] { askPresetName(); };
    browser.canOverwrite = [this] { return proc.userName.isNotEmpty(); };
    browser.overwrite = [this] { proc.saveUserPreset (proc.userName); repaint(); };
    browser.folder = [] { Processor::presetFolder().startAsProcess(); };
    browser.remove = [this] (const jbrowse::Entry& e) { Processor::presetFolder().getChildFile (e.name + ".jchoir").moveToTrash(); if (proc.userName == e.name) proc.userName = {}; repaint(); };
    browser.onClose = [this] { repaint(); };
}
void Editor::askPresetName()
{
    auto* w = new juce::AlertWindow ("Salva preset", "Nome del preset (finisce in Musica / Jackpot / Choir Preset):", juce::MessageBoxIconType::NoIcon, this);
    w->addTextEditor ("name", proc.userName.isNotEmpty() ? proc.userName : proc.getProgramName (proc.preset).replace (" | ", " - "));
    w->addButton ("SALVA", 1, juce::KeyPress (juce::KeyPress::returnKey)); w->addButton ("ANNULLA", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    juce::Component::SafePointer<Editor> sp (this);
    w->enterModalState (true, juce::ModalCallbackFunction::create ([sp, w] (int r)
    {
        if (sp != nullptr && r == 1) { const auto n = w->getTextEditorContents ("name").trim(); if (n.isNotEmpty()) { sp->proc.saveUserPreset (n); sp->browser.refresh(); sp->repaint(); } }
    }), true);
}
void Editor::timerCallback()
{
    if (browser.isVisible()) return;
    pitch.repaint();
    for (auto& k : knobs) { const float v = k->param->getValue(); if (v != k->shown) { k->shown = v; k->repaint(); } }
    if (++tick % 3 == 0)
    {
        for (auto& s : segs) s->repaint();
        for (int v = 0; v < 4; ++v) { const bool on = juce::roundToInt (proc.value ("v" + juce::String (v + 1) + "_int")) != I_OFF; for (int k = 0; k < 3; ++k) knobs[(size_t) (v * 3 + k)]->lit = on; }
        repaint (scaler.map ({ 18, 330, 1064, 60 }));
    }
    if (tick % 15 == 0) repaint (scaler.map ({ 0, 0, W, 64 }));
}
}
