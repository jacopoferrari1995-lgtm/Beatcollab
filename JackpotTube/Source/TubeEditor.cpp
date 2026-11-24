// JACKPOT TUBE BOX 2: sopra le valvole a vista e il VU, il DRIVE grande e i sei caratteri;
// sotto CALORE, ARIA, ENHANCER, MIX e OUTPUT. Niente altro: il resto lo fa il carattere.
#include "TubeEditor.h"

namespace jtbx
{
static juce::String U (const char* s) { return juce::String::fromUTF8 (s); }
static juce::Font uf (float size, bool bold = true) { return jlook::ui (size, bold ? 2 : 1); }
static juce::Font mf (float size) { return jlook::mono (size); }
namespace C
{
    inline juce::Colour text { 0xffe9e2d4 }, dim { 0xff8a8172 }, faint { 0xff514a40 }, amber { 0xffffb347 }, orange { 0xffff7a2e }, red { 0xffff3b30 }, cream { 0xfff3e2b8 }, cyan { 0xff5ef2ff }, mint { 0xff5effc1 }, pink { 0xffff4fa3 };
    inline juce::Colour lcd1 { 0xff1a120c }, lcd2 { 0xff0b0806 };
    inline void sync()
    {
        if (! jlook::skinHooks().on) return;
        const auto& p = jlook::skinPal();
        text = p.text; dim = p.dim; faint = p.faint; amber = p.amber; red = p.red; cyan = p.cyan; pink = p.pink; lcd1 = p.lcd0; lcd2 = p.lcd1;
    }
}
static const juce::Colour plateInk { 0xffe9e2d4 }, plateDim { 0xff9a9080 };   // scritte sui frontali (sempre scuri)
static juce::Colour charCol (int c) { static const juce::uint32 k[kChars] = { 0xffffb347, 0xffff7a2e, 0xffe6cfa0, 0xff9fe8ff, 0xffff3b30, 0xffd98a4a }; return juce::Colour (k[juce::jlimit (0, kChars - 1, c)]); }
static void glowText (juce::Graphics& g, const juce::String& t, juce::Rectangle<float> r, juce::Colour c, juce::Font f, juce::Justification j = juce::Justification::centredLeft)
{
    g.setFont (f); g.setColour (c.withAlpha (0.2f)); g.drawText (t, r.translated (-0.7f, 0.3f), j); g.drawText (t, r.translated (0.7f, 0.3f), j);
    g.setColour (c); g.drawText (t, r, j);
}
static void lcd (juce::Graphics& g, juce::Rectangle<float> r, float rad)
{
    if (jlook::skinHooks().on && jlook::skinHooks().screen) { jlook::skinHooks().screen (g, r, rad, C::amber); return; }
    g.setColour (juce::Colours::black.withAlpha (0.65f)); g.fillRoundedRectangle (r.expanded (1.5f), rad + 1.5f);
    juce::ColourGradient cg (C::lcd1, r.getCentreX(), r.getCentreY(), C::lcd2, r.getX(), r.getY(), true); g.setGradientFill (cg); g.fillRoundedRectangle (r, rad);
}

// ======================================================================= manopola
void Knob::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isAltDown() || e.mods.isCommandDown()) { mouseDoubleClick (e); return; }
    start = param->getValue(); param->beginChangeGesture(); drag = true; repaint();
}
void Knob::mouseUp (const juce::MouseEvent&) { if (drag) param->endChangeGesture(); drag = false; repaint(); }
void Knob::mouseDrag (const juce::MouseEvent& e)
{
    if (drag) param->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, start - e.getDistanceFromDragStartY() / (e.mods.isShiftDown() ? 900.0f : 220.0f)));
}
void Knob::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w)
{
    param->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, param->getValue() + (w.deltaY > 0 ? 0.01f : -0.01f)));
}
void Knob::paint (juce::Graphics& g)
{
    auto s = spec; s.enabled = lit; s.hover = hover; s.drag = drag; s.text = text ? text() : param->getCurrentValueAsText();
    hw::knob (g, getLocalBounds().toFloat(), param->getValue(), s);
}

// ======================================================================= valvole a vista + VU
static void drawTube (juce::Graphics& g, juce::Rectangle<float> r, float glow, float flick, const juce::String& name)
{
    using namespace juce;
    const float cx = r.getCentreX();
    auto glass = r.withTrimmedBottom (r.getHeight() * 0.16f);
    auto base = r.withTop (glass.getBottom() - 4);
    // alone caldo dietro il vetro
    const float gl = jlimit (0.0f, 1.0f, glow) * (0.92f + 0.08f * flick);
    if (gl > 0.01f)
    {
        ColourGradient halo (Colour (0xffff8a2a).withAlpha (0.55f * gl), cx, glass.getY() + glass.getHeight() * 0.55f, Colour (0x00ff6a00), cx + glass.getWidth() * 1.2f, glass.getY() + glass.getHeight() * 0.55f, true);
        g.setGradientFill (halo); g.fillEllipse (glass.expanded (glass.getWidth() * 0.9f, glass.getHeight() * 0.25f));
    }
    // zoccolo e piedini
    g.setColour (Colour (0xff141210)); g.fillRoundedRectangle (base, 5);
    g.setColour (Colour (0xff2c2924)); g.drawRoundedRectangle (base.reduced (1), 5, 1.0f);
    g.setColour (Colour (0xff9a8d72)); for (int i = 0; i < 5; ++i) g.fillRect (base.getX() + base.getWidth() * (0.2f + i * 0.15f) - 1, base.getBottom() - 2, 2.0f, 6.0f);
    // vetro: corpo e cupola
    Path env; env.addRoundedRectangle (glass.getX(), glass.getY() + glass.getWidth() * 0.35f, glass.getWidth(), glass.getHeight() - glass.getWidth() * 0.35f, 6);
    env.addEllipse (glass.getX(), glass.getY(), glass.getWidth(), glass.getWidth() * 0.7f);
    g.setColour (Colour (0x332a3540)); g.fillPath (env);
    // placche interne
    auto in = glass.reduced (glass.getWidth() * 0.2f, glass.getHeight() * 0.18f).translated (0, glass.getHeight() * 0.08f);
    g.setColour (Colour (0xff3a3a3e)); g.fillRect (in.withWidth (in.getWidth() * 0.32f)); g.fillRect (in.withTrimmedLeft (in.getWidth() * 0.68f));
    g.setColour (Colour (0xff55555c)); g.fillRect (in.getX(), in.getY() - 3, in.getWidth(), 3.0f); g.fillRect (in.getX(), in.getBottom(), in.getWidth(), 3.0f);
    // filamento che si accende
    {
        auto fil = in.withSizeKeepingCentre (in.getWidth() * 0.2f, in.getHeight() * 0.86f);
        g.setColour (Colour (0xff201a14)); g.fillRect (fil);
        if (gl > 0.01f)
        {
            ColourGradient fg (Colour (0xffffd08a).withAlpha (gl), fil.getCentreX(), fil.getCentreY(), Colour (0xffff5a10).withAlpha (0.0f), fil.getCentreX() + fil.getWidth() * 2.5f, fil.getCentreY(), true);
            g.setGradientFill (fg); g.fillRect (fil.expanded (fil.getWidth() * 2.0f, 4));
            g.setColour (Colour (0xffffe0a8).withAlpha (gl)); g.fillRect (fil.reduced (fil.getWidth() * 0.32f, 2));
        }
    }
    // getter (argentatura in cima) e riflesso del vetro
    g.setColour (Colour (0xffb8bcc4).withAlpha (0.35f)); g.fillEllipse (glass.getX() + glass.getWidth() * 0.18f, glass.getY() + 3, glass.getWidth() * 0.64f, glass.getWidth() * 0.3f);
    g.setColour (Colours::white.withAlpha (0.22f)); g.fillRoundedRectangle (glass.getX() + glass.getWidth() * 0.12f, glass.getY() + glass.getWidth() * 0.35f, 3.0f, glass.getHeight() * 0.55f, 1.5f);
    g.setColour (Colours::white.withAlpha (0.18f)); g.strokePath (env, PathStrokeType (1.0f));
    g.setColour (Colour (0xffd8ccb0)); g.setFont (mf (8.5f)); g.drawText (name, base.withTrimmedTop (2), Justification::centred);
}
void TubeView::paint (juce::Graphics& g)
{
    using namespace juce;
    auto b = getLocalBounds().toFloat();
    // finestra scura con le valvole
    auto win = b.withWidth (b.getWidth() * 0.5f);
    lcd (g, win, 10);
    const bool on = ed.active();
    const float heat = ed.proc.eng.heat.load(), tgt = on ? jlimit (0.2f, 1.0f, 0.2f + ed.proc.value ("drive") * 0.05f + heat * 10.0f) : 0.0f;
    const auto& K = charOf (ed.character());
    if (glow < 0) glow = tgt;
    glow += (tgt - glow) * (tgt > glow ? 0.08f : 0.04f);
    flick = 0.5f + 0.5f * std::sin ((float) (Time::getMillisecondCounter() % 100000) * 0.013f) * std::sin ((float) (Time::getMillisecondCounter() % 100000) * 0.0071f);
    const float tw = win.getWidth() * 0.34f;
    drawTube (g, Rectangle<float> (win.getX() + win.getWidth() * 0.12f, win.getY() + 14, tw, win.getHeight() - 26), glow, flick, K.tube);
    drawTube (g, Rectangle<float> (win.getRight() - win.getWidth() * 0.12f - tw, win.getY() + 14, tw, win.getHeight() - 26), glow * 0.9f, 1.0f - flick, K.stages > 1 || K.curve == TRAFO ? juce::String (K.curve == TRAFO ? "OUT TRAFO" : "12AX7") : juce::String (K.tube));
    // VU crema come sull'hardware
    auto vb = b.withTrimmedLeft (win.getWidth() + 12).reduced (0, 6);
    g.setColour (Colours::black.withAlpha (0.7f)); g.fillRoundedRectangle (vb.expanded (2), 8);
    ColourGradient cg (Colour (0xfff3e2b8), vb.getCentreX(), vb.getY(), Colour (0xffc9a96a), vb.getCentreX(), vb.getBottom(), false); g.setGradientFill (cg); g.fillRoundedRectangle (vb, 6);
    if (on) { g.setColour (Colour (0xffffb040).withAlpha (0.12f + 0.2f * glow)); g.fillRoundedRectangle (vb, 6); }
    const float R = juce::jmin (vb.getWidth() * 0.62f, vb.getHeight() * 0.72f), cx = vb.getCentreX(), cy = vb.getBottom() - 34;
    auto angOf = [] (float db) { return jmap (jlimit (-20.0f, 3.0f, db), -20.0f, 3.0f, -0.78f, 0.78f); };
    for (float db : { -20.f, -10.f, -7.f, -5.f, -3.f, -1.f, 0.f, 1.f, 2.f, 3.f })
    {
        const float a = angOf (db);
        g.setColour (db > 0 ? Colour (0xffb3261e) : Colour (0xff2a2016)); g.drawLine (cx + std::sin (a) * R * 0.86f, cy - std::cos (a) * R * 0.86f, cx + std::sin (a) * R * 0.95f, cy - std::cos (a) * R * 0.95f, 1.4f);
        if (db != -1 && db != 1 && db != 2) { g.setFont (mf (8.0f)); g.drawText (db > 0 ? "+" + String ((int) db) : String ((int) db), Rectangle<float> (cx + std::sin (a) * R * 0.76f - 12, cy - std::cos (a) * R * 0.76f - 6, 24, 12), Justification::centred); }
    }
    { Path arc; arc.addCentredArc (cx, cy, R * 0.9f, R * 0.9f, 0, angOf (0), angOf (3), true); g.setColour (Colour (0xffb3261e)); g.strokePath (arc, PathStrokeType (3.0f)); }
    g.setColour (Colour (0xff2a2016)); g.setFont (uf (13.0f)); g.drawText ("VU", Rectangle<float> (cx - 20, cy - R * 0.45f, 40, 16), Justification::centred);
    g.setFont (uf (15.0f)); g.drawText ("JACKPOT", Rectangle<float> (vb.getX(), vb.getY() + 16, vb.getWidth(), 18), Justification::centred);
    g.setFont (mf (8.0f)); g.drawText (juce::String ("TUBE BOX  /  ") + K.name, Rectangle<float> (vb.getX(), vb.getY() + 36, vb.getWidth(), 12), Justification::centred);
    
    const float lv = ed.proc.eng.vuUi.load(), db = Decibels::gainToDecibels (lv * 1.414f, -60.0f) + 14.0f;   // 0 VU = -14 dBFS RMS
    needle += (angOf (db) - needle) * 0.22f;
    g.saveState(); g.reduceClipRegion (vb.toNearestInt());
    g.setColour (Colour (0xff15110c)); g.drawLine (cx + std::sin (needle) * R * 0.25f, cy - std::cos (needle) * R * 0.25f, cx + std::sin (needle) * R * 0.97f, cy - std::cos (needle) * R * 0.97f, 1.6f);
    g.fillEllipse (cx - 6, cy - 6, 12, 12);
    g.restoreState();
    g.setColour (Colours::white.withAlpha (0.12f)); g.drawRoundedRectangle (vb, 6, 1.0f);
}

// ======================================================================= editor
std::unique_ptr<Knob> Editor::mk (const char* id, const char* label, int style, juce::Colour col, std::function<juce::String()> txt, const char* tip, bool big, bool bip)
{
    auto k = std::make_unique<Knob> (P (id)); k->spec.label = label; k->spec.style = style; k->spec.col = col; k->spec.big = big; k->spec.bipolar = bip; k->text = std::move (txt); k->setTooltip (tip);
    k->defV = k->param->getDefaultValue(); addAndMakeVisible (*k); return k;
}
Editor::Editor (Processor& p) : AudioProcessorEditor (p), proc (p)
{
    skin::init ("tubebox", { 0xffe0b07a, 0xffd99a70, 0xffc9b08a, 0xffe6cfa0 }, 0xffff8a2a); C::sync();
    setLookAndFeel (&lnf);
    addAndMakeVisible (tubes);
    auto v = [this] (const char* id) { return proc.value (id); };
    auto one = [v] (const char* id) { return [v, id] { return juce::String (v (id), 1); }; };
    knobs.push_back (mk ("drive", "DRIVE", 0, C::orange, one ("drive"), "Quanto spingi la valvola. A zero e' spenta; il volume resta uguale, cambia solo il carattere.", true));
    knobs.push_back (mk ("warmth", "CALORE", 0, C::amber, one ("warmth"), "Armoniche sui bassi: corpo e peso, l'808 e la voce si sentono anche sul telefono.", true));
    knobs.push_back (mk ("air", "ARIA", 0, C::cream, one ("air"), "Apre la parte alta con dolcezza, con un velo di armoniche sopra gli 11 kHz.", true));
    knobs.push_back (mk ("enh", "ENHANCER", 0, C::amber, one ("enh"), "Armoniche nuove nella zona della presenza (la sceglie il carattere): il suono viene avanti senza alzarlo.", true));
    knobs.push_back (mk ("mix", "MIX", 1, C::amber, [v] { return juce::String (juce::roundToInt (v ("mix") * 100)) + "%"; }, "Il suono trattato in parallelo col pulito."));
    knobs.push_back (mk ("out", "OUTPUT", 1, C::cream, [v] { const float x = v ("out"); return (x > 0.05f ? "+" : "") + juce::String (x, 1) + " dB"; }, "Volume finale.", false, true));
    addAndMakeVisible (sizeChip);
    setupBrowser();
    undoMark.p = &p; addMouseListener (&undoMark, true); setWantsKeyboardFocus (true);
    scaler.setup();
    startTimerHz (30);
}
void Editor::resized()
{
    const float s = scaler.update();
    presetR = { 372, 15, 340, 36 }; prevR = { 334, 15, 36, 36 }; nextR = { 714, 15, 36, 36 }; abR = { 760, 15, 120, 36 };
    undoR = { 890, 19, 30, 28 }; redoR = { 922, 19, 30, 28 }; themeR = { 980, 19, 100, 28 };
    const auto mc = mainCard(), lc = lowCard();
    tubes.setBounds (juce::Rectangle<float> (mc.getX() + 18, mc.getY() + 42, 400, 280).toNearestInt());
    knobs[0]->setBounds (juce::Rectangle<float> (mc.getX() + 436, mc.getY() + 36, 250, 290).toNearestInt());
    for (int i = 0; i < kChars; ++i) charR[i] = { mc.getX() + 706 + (i % 2) * 172.0f, mc.getY() + 46 + (i / 2) * 58.0f, 164, 50 };
    for (int i = 0; i < 3; ++i) knobs[(size_t) i + 1]->setBounds (juce::Rectangle<float> (lc.getX() + 24 + i * 200.0f, lc.getY() + 36, 180, 176).toNearestInt());
    knobs[4]->setBounds (juce::Rectangle<float> (lc.getX() + 650, lc.getY() + 56, 120, 140).toNearestInt());
    knobs[5]->setBounds (juce::Rectangle<float> (lc.getX() + 780, lc.getY() + 56, 120, 140).toNearestInt());
    autoR = { lc.getX() + 914, lc.getY() + 160, 132, 30 };
    sizeChip.setBounds (W - 76, H - 16, 58, 14);
    browser.setBounds (0, 0, W, H);
    jlook::scaleChildren (*this, s);
}
juce::String Editor::staticKey()
{
    juce::String k; k << skin::cur() << "/" << proc.preset << "/" << proc.ab.label() << "/" << hoverChar << "/" << hoverTop << "/" << character() << "/" << (int) active() << "/" << (int) on ("auto")
                      << "/" << (int) proc.undo.canUndo() << (int) proc.undo.canRedo();
    return k;
}
void Editor::paintStatic (juce::Graphics& g)
{
    const float Wf = (float) W, Hf = (float) H;
    const int ch = character(); const auto& K = charOf (ch); const auto cc = charCol (ch);
    jlook::backdrop (g, { 0, 0, Wf, Hf }, C::orange, C::amber);
    jlook::header (g, { 0, 0, Wf, 64 }, "TUBE BOX", C::orange, U ("JTB-3  SEI VALVOLE"), C::amber, C::orange);
    lcd (g, presetR.toFloat(), 8); jlook::glass (g, presetR.toFloat(), 8);
    {
        const auto& pr = presets()[(size_t) juce::jlimit (0, (int) presets().size() - 1, proc.preset)];
        const juce::String nm (proc.userName.isNotEmpty() ? proc.userName : juce::String (pr.name)); const bool grp = nm.contains (" | ");
        g.setColour (C::dim); g.setFont (uf (7.5f)); g.drawText (proc.userName.isNotEmpty() ? juce::String ("MIO") : grp ? nm.upToFirstOccurrenceOf (" | ", false, false).toUpperCase() : juce::String ("BASE"), presetR.toFloat().withWidth (80).translated (12, 0), juce::Justification::centredLeft);
        glowText (g, (grp ? nm.fromFirstOccurrenceOf (" | ", false, false) : nm).toUpperCase(), presetR.toFloat().withTrimmedLeft (60).withTrimmedRight (20), C::amber, mf (12.5f), juce::Justification::centred);
    }
    g.setColour (C::amber); g.setFont (uf (20.0f));
    g.drawText (U ("\xe2\x80\xb9"), prevR, juce::Justification::centred); g.drawText (U ("\xe2\x80\xba"), nextR, juce::Justification::centred);
    jlook::button (g, abR.toFloat(), C::cyan, proc.ab.cur == 1, hoverTop == 0, "MEMORIA " + proc.ab.label(), uf (11.0f));
    jlook::button (g, undoR.toFloat(), C::amber, false, hoverTop == 2, U ("\xe2\x86\xb6"), uf (13.0f));
    jlook::button (g, redoR.toFloat(), C::amber, false, hoverTop == 3, U ("\xe2\x86\xb7"), uf (13.0f));
    if (! proc.undo.canUndo()) { g.setColour (juce::Colours::black.withAlpha (0.45f)); g.fillRoundedRectangle (undoR.toFloat(), 5); }
    if (! proc.undo.canRedo()) { g.setColour (juce::Colours::black.withAlpha (0.45f)); g.fillRoundedRectangle (redoR.toFloat(), 5); }
    skin::chipDraw (g, themeR.toFloat(), hoverTop == 1);
    // ---- valvola e caratteri
    {
        const auto mc = mainCard();
        hw::faceplate (g, mc, juce::Colour (0xff1d1a17), juce::Colour (0xff0d0c0b), active() ? cc : C::faint, 32, 7, 12);
        g.setColour (plateInk); g.setFont (uf (12.0f)); g.drawText ("VALVOLA", juce::Rectangle<float> (mc.getX() + 16, mc.getY() + 4, 160, 24), juce::Justification::centredLeft);
        g.setColour (plateDim); g.setFont (uf (9.0f, false));
        g.drawText (U ("4x senza aliasing  \xc2\xb7  volume compensato  \xc2\xb7  a zero e' trasparente"), juce::Rectangle<float> (mc.getX() + 100, mc.getY() + 4, 500, 24), juce::Justification::centredLeft);
        hw::jewel (g, mc.getRight() - 22, mc.getY() + 16, 5.0f, active(), C::red);
        hw::bezel (g, juce::Rectangle<float> (mc.getX() + 18, mc.getY() + 42, 400, 280), 10);
        g.setColour (plateDim); g.setFont (uf (8.0f)); g.drawText ("CARATTERE", juce::Rectangle<float> (mc.getX() + 706, mc.getY() + 30, 336, 12), juce::Justification::centredLeft);
        for (int i = 0; i < kChars; ++i)
        {
            const auto r = charR[i]; const bool lit = i == ch, hv = i == hoverChar; const auto col = charCol (i);
            jlook::button (g, r, col, lit, hv, "", uf (10.0f));
            glowText (g, charOf (i).name, r.withTrimmedLeft (14).withTrimmedBottom (18), lit ? col : C::text.withAlpha (0.8f), uf (15.0f));
            g.setColour (lit ? col.withAlpha (0.8f) : C::dim); g.setFont (mf (8.5f)); g.drawText (charOf (i).tube, r.withTrimmedLeft (14).withTrimmedTop (28), juce::Justification::centredLeft);
        }
        // descrizione del carattere scelto
        auto dr = juce::Rectangle<float> (mc.getX() + 706, mc.getY() + 226, 336, 96);
        lcd (g, dr, 8);
        glowText (g, juce::String (K.name) + "  \xc2\xb7  " + K.tube, dr.reduced (14, 10).withHeight (18), cc, mf (12.0f));
        g.setColour (C::text.withAlpha (0.85f)); g.setFont (uf (11.0f, false));
        g.drawFittedText (U (K.desc), dr.reduced (14, 10).withTrimmedTop (24).toNearestInt(), juce::Justification::topLeft, 3);
    }
    // ---- colore
    {
        const auto lc = lowCard();
        hw::faceplate (g, lc, juce::Colour (0xff1c1a20), juce::Colour (0xff0c0b0e), active() ? C::amber : C::faint, 32, 17, 12);
        g.setColour (plateInk); g.setFont (uf (12.0f)); g.drawText ("COLORE", juce::Rectangle<float> (lc.getX() + 16, lc.getY() + 4, 160, 24), juce::Justification::centredLeft);
        g.setColour (plateDim); g.setFont (uf (9.0f, false));
        g.drawText (U ("CALORE sui bassi  \xc2\xb7  ARIA in alto  \xc2\xb7  ENHANCER sulla presenza del carattere"), juce::Rectangle<float> (lc.getX() + 100, lc.getY() + 4, 600, 24), juce::Justification::centredLeft);
        for (float dx : { 620.0f }) { g.setColour (plateInk.withAlpha (0.06f)); g.fillRect (lc.getX() + dx, lc.getY() + 40.0f, 1.0f, 170.0f); }
        jlook::button (g, autoR, C::cyan, on ("auto"), hoverTop == 4, "VOLUME AUTO", uf (9.5f));
        g.setColour (plateDim); g.setFont (uf (8.0f)); g.drawText ("USCITA", juce::Rectangle<float> (autoR.getX(), lc.getY() + 56, autoR.getWidth(), 12), juce::Justification::centredLeft);
    }
    g.setColour (C::dim); g.setFont (uf (9.0f, false));
    g.drawText (U ("doppio clic su una manopola = zero  \xc2\xb7  shift = movimento fine  \xc2\xb7  cmd+Z annulla"), juce::Rectangle<float> (20, Hf - 24, 900, 16), juce::Justification::centredLeft);
}
void Editor::paint (juce::Graphics& g)
{
    skin::apply(); C::sync();
    g.addTransform (juce::AffineTransform::scale (scaler.s));
    bg.draw (g, { 0, 0, W, H }, staticKey(), [this] (juce::Graphics& g2) { paintStatic (g2); });
    // uscita: picco
    const auto lc = lowCard();
    auto mr = juce::Rectangle<float> (lc.getX() + 914, lc.getY() + 76, 132, 8);
    const float db = juce::Decibels::gainToDecibels (proc.eng.outPk.load(), -60.0f), n = juce::jlimit (0.0f, 1.0f, (db + 48) / 48);
    g.setColour (juce::Colours::white.withAlpha (0.06f)); g.fillRoundedRectangle (mr, 4);
    g.setColour (db > -0.5f ? C::red : C::amber); g.fillRoundedRectangle (mr.withWidth (mr.getWidth() * n), 4);
    g.setColour (plateDim); g.setFont (mf (9.0f));
    g.drawText (db > -59 ? juce::String (db, 1) + " dB" : juce::String ("-inf"), juce::Rectangle<float> (mr.getX(), mr.getBottom() + 4, mr.getWidth(), 12), juce::Justification::centredLeft);
    // compensazione automatica in corso
    if (on ("auto") && active())
    {
        const float ag = juce::Decibels::gainToDecibels (proc.eng.agUi.load());
        g.drawText ("auto " + juce::String (ag >= 0 ? "+" : "") + juce::String (ag, 1) + " dB", juce::Rectangle<float> (mr.getX(), mr.getBottom() + 22, mr.getWidth(), 12), juce::Justification::centredLeft);
    }
}
void Editor::mouseMove (const juce::MouseEvent& e0)
{
    const auto p = e0.position / scaler.s;
    int h = -1; for (int i = 0; i < kChars; ++i) if (charR[i].contains (p)) h = i;
    const int t = abR.toFloat().contains (p) ? 0 : themeR.toFloat().contains (p) ? 1 : undoR.toFloat().contains (p) ? 2 : redoR.toFloat().contains (p) ? 3 : autoR.contains (p) ? 4 : -1;
    if (h != hoverChar || t != hoverTop) { hoverChar = h; hoverTop = t; repaint(); }
    setMouseCursor (h >= 0 || t >= 0 || presetR.toFloat().contains (p) || prevR.toFloat().contains (p) || nextR.toFloat().contains (p) ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
}
void Editor::mouseDown (const juce::MouseEvent& e0)
{
    const auto p = e0.position / scaler.s;
    const int n = (int) presets().size();
    for (int i = 0; i < kChars; ++i)
        if (charR[i].contains (p)) { auto* rp = P ("char"); rp->beginChangeGesture(); rp->setValueNotifyingHost (rp->convertTo0to1 ((float) i)); rp->endChangeGesture(); repaint(); return; }
    if (autoR.contains (p)) { auto* rp = P ("auto"); rp->setValueNotifyingHost (rp->getValue() > 0.5f ? 0.0f : 1.0f); repaint(); return; }
    if (themeR.toFloat().contains (p)) { skin::cycle(); C::sync(); repaint(); for (auto* c : getChildren()) c->repaint(); return; }
    if (undoR.toFloat().contains (p)) { doUndo (false); return; }
    if (redoR.toFloat().contains (p)) { doUndo (true); return; }
    if (abR.toFloat().contains (p)) { proc.ab.toggle (proc); repaint(); return; }
    if (prevR.toFloat().contains (p)) { proc.loadPreset ((juce::jmax (0, proc.preset) + n - 1) % n); repaint(); return; }
    if (nextR.toFloat().contains (p)) { proc.loadPreset ((proc.preset + 1) % n); repaint(); return; }
    if (presetR.toFloat().contains (p)) { browser.open(); return; }
}
bool Editor::keyPressed (const juce::KeyPress& k)
{
    if (k.getModifiers().isCommandDown() && (k.getKeyCode() == 'Z' || k.getKeyCode() == 'z')) { doUndo (k.getModifiers().isShiftDown()); return true; }
    if (k.getModifiers().isCommandDown() && (k.getKeyCode() == 'Y' || k.getKeyCode() == 'y')) { doUndo (true); return true; }
    return false;
}
void Editor::doUndo (bool redo) { proc.undo.beginNewTransaction(); if (redo) proc.undo.redo(); else proc.undo.undo(); repaint(); }
void Editor::setupBrowser()
{
    addChildComponent (browser);
    browser.accent = C::orange; browser.title = "Preset";
    auto userFiles = [] { auto f = Processor::presetFolder().findChildFiles (juce::File::findFiles, false, "*.jtb"); f.sort(); return f; };
    browser.entries = [userFiles]
    {
        std::vector<jbrowse::Entry> v;
        for (int i = 0; i < (int) presets().size(); ++i)
        {
            const juce::String nm (presets()[(size_t) i].name); jbrowse::Entry e; e.id = i; e.note = presets()[(size_t) i].note;
            e.group = nm.contains (" | ") ? nm.upToFirstOccurrenceOf (" | ", false, false) : juce::String ("BASE"); e.name = nm.contains (" | ") ? nm.fromFirstOccurrenceOf (" | ", false, false) : nm;
            v.push_back (e);
        }
        const auto fs = userFiles();
        for (int i = 0; i < fs.size(); ++i) { jbrowse::Entry e; e.id = 100000 + i; e.user = true; e.group = "I MIEI"; e.name = fs[i].getFileNameWithoutExtension(); e.note = "Preset tuo, salvato in Musica / Jackpot / Tube Box Preset."; v.push_back (e); }
        return v;
    };
    browser.current = [this, userFiles]
    {
        if (proc.userName.isNotEmpty()) { const auto fs = userFiles(); for (int i = 0; i < fs.size(); ++i) if (fs[i].getFileNameWithoutExtension() == proc.userName) return 100000 + i; return -1; }
        return proc.preset;
    };
    browser.load = [this] (const jbrowse::Entry& e)
    {
        if (e.user) proc.loadUser (Processor::presetFolder().getChildFile (e.name + ".jtb")); else proc.loadPreset (e.id);
        repaint();
    };
    browser.save = [this] { askPresetName(); };
    browser.canOverwrite = [this] { return proc.userName.isNotEmpty() && Processor::presetFolder().getChildFile (proc.userName + ".jtb").existsAsFile(); };
    browser.overwrite = [this] { proc.saveUser (Processor::presetFolder().getChildFile (proc.userName + ".jtb")); repaint(); };
    browser.folder = [] { Processor::presetFolder().startAsProcess(); };
    browser.remove = [] (const jbrowse::Entry& e) { Processor::presetFolder().getChildFile (e.name + ".jtb").moveToTrash(); };
    browser.onClose = [this] { repaint(); };
}
void Editor::askPresetName()
{
    auto* w = new juce::AlertWindow ("Salva preset", "Nome del preset (finisce in Musica / Jackpot / Tube Box Preset):", juce::MessageBoxIconType::NoIcon, this);
    w->addTextEditor ("name", proc.userName);
    w->addButton ("SALVA", 1, juce::KeyPress (juce::KeyPress::returnKey)); w->addButton ("ANNULLA", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    juce::Component::SafePointer<Editor> sp (this);
    w->enterModalState (true, juce::ModalCallbackFunction::create ([sp, w] (int r)
    {
        if (sp != nullptr && r == 1)
        {
            const auto nm = juce::File::createLegalFileName (w->getTextEditorContents ("name").trim());
            if (nm.isNotEmpty()) { sp->proc.saveUser (Processor::presetFolder().getChildFile (nm + ".jtb")); sp->browser.refresh(); sp->repaint(); }
        }
    }), true);
}
void Editor::timerCallback()
{
    if (browser.isVisible()) return;
    tubes.repaint();
    for (auto& k : knobs) { k->spec.col = k.get() == knobs[0].get() ? charCol (character()) : k->spec.col; k->refresh (true); }
    if (bg.key != staticKey()) repaint();
    repaint (scaler.map (juce::Rectangle<int> ((int) lowCard().getX() + 910, (int) lowCard().getY() + 70, 140, 64)));
}
}
