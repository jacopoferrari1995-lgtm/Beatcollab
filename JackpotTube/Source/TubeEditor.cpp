// JACKPOT TUBE BOX: frontale da hardware come Dust, FX e Space. Sopra le valvole HG con le due valvole a vista e il VU,
// sotto l'enhancer a quattro comandi con il disegno di dove nascono le armoniche.
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
static juce::Colour charCol (int c) { return c == 0 ? C::amber : c == 1 ? juce::Colour (0xffd9a066).interpolatedWith (C::cream, 0.2f) : C::cyan; }
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
static juce::String clock (float v) { const float h = 7.0f + v; const int hh = (int) std::floor (h), mm = juce::roundToInt ((h - hh) * 60) % 60; return juce::String (hh) + ":" + juce::String (mm).paddedLeft ('0', 2); }

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
    auto win = b.withWidth (b.getWidth() * 0.4f);
    lcd (g, win, 10);
    const bool on = ed.on ("hg_on");
    const float heat = ed.proc.eng.heat.load(), tgt = on ? jlimit (0.25f, 1.0f, 0.35f + heat * 12.0f) : 0.0f;
    if (glow < 0) glow = tgt;
    glow += (tgt - glow) * (tgt > glow ? 0.08f : 0.04f);
    flick = 0.5f + 0.5f * std::sin ((float) (Time::getMillisecondCounter() % 100000) * 0.013f) * std::sin ((float) (Time::getMillisecondCounter() % 100000) * 0.0071f);
    const float tw = win.getWidth() * 0.34f;
    drawTube (g, Rectangle<float> (win.getX() + win.getWidth() * 0.12f, win.getY() + 14, tw, win.getHeight() - 26), glow, flick, "6U8A");
    drawTube (g, Rectangle<float> (win.getRight() - win.getWidth() * 0.12f - tw, win.getY() + 14, tw, win.getHeight() - 26), glow * 0.9f, 1.0f - flick, "12AT7");
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
    g.setFont (mf (8.0f)); g.drawText ("TUBE BOX  /  2 STAGE", Rectangle<float> (vb.getX(), vb.getY() + 36, vb.getWidth(), 12), Justification::centred);
    
    const float lv = ed.proc.eng.vuUi.load(), db = Decibels::gainToDecibels (lv * 1.414f, -60.0f) + 14.0f;   // 0 VU = -14 dBFS RMS
    needle += (angOf (db) - needle) * 0.22f;
    g.saveState(); g.reduceClipRegion (vb.toNearestInt());
    g.setColour (Colour (0xff15110c)); g.drawLine (cx + std::sin (needle) * R * 0.25f, cy - std::cos (needle) * R * 0.25f, cx + std::sin (needle) * R * 0.97f, cy - std::cos (needle) * R * 0.97f, 1.6f);
    g.fillEllipse (cx - 6, cy - 6, 12, 12);
    g.restoreState();
    g.setColour (Colours::white.withAlpha (0.12f)); g.drawRoundedRectangle (vb, 6, 1.0f);
}

// ======================================================================= dove nascono le armoniche
static float xOfHz (juce::Rectangle<float> r, float f) { return r.getX() + r.getWidth() * std::log (juce::jlimit (20.0f, 20000.0f, f) / 20.0f) / std::log (1000.0f); }
void HarmView::paint (juce::Graphics& g)
{
    using namespace juce;
    auto b = getLocalBounds().toFloat(); lcd (g, b, 8);
    auto r = b.reduced (16, 14).withTrimmedBottom (16);
    const bool on = ed.on ("mb_on"); const int ch = roundToInt (ed.proc.value ("char")); const auto cc = charCol (ch);
    // griglia
    for (float f : { 50.f, 100.f, 200.f, 500.f, 1000.f, 2000.f, 5000.f, 10000.f })
    {
        const float x = xOfHz (r, f); g.setColour (C::text.withAlpha (f == 100 || f == 1000 || f == 10000 ? 0.1f : 0.05f)); g.fillRect (x, r.getY(), 1.0f, r.getHeight());
        g.setColour (C::dim); g.setFont (mf (8.0f)); g.drawText (f >= 1000 ? String (f / 1000, 0) + "k" : String ((int) f), Rectangle<float> (x - 16, r.getBottom() + 2, 32, 12), Justification::centred);
    }
    g.setColour (C::text.withAlpha (0.08f)); g.fillRect (r.getX(), r.getBottom() - 1, r.getWidth(), 1.0f);
    const float f0 = focusHz (ed.proc.value ("e_focus")), amt = ed.proc.value ("e_amt") / 10.0f;
    // zona del fuoco: da qui nascono le armoniche
    {
        const float x0 = xOfHz (r, f0 * 0.7f), x1 = xOfHz (r, f0 * 1.6f);
        ColourGradient zg (cc.withAlpha (on ? 0.35f : 0.12f), (x0 + x1) * 0.5f, r.getBottom(), cc.withAlpha (0.0f), (x0 + x1) * 0.5f, r.getY(), false);
        g.setGradientFill (zg); g.fillRect (Rectangle<float> (x0, r.getY(), x1 - x0, r.getHeight()));
        g.setColour (cc.withAlpha (on ? 0.9f : 0.35f)); g.fillRect (xOfHz (r, f0) - 1, r.getY(), 2.0f, r.getHeight());
        g.setFont (uf (9.0f)); g.drawText ("FUOCO " + (f0 >= 1000 ? String (f0 / 1000, 1) + " kHz" : String (roundToInt (f0)) + " Hz"), Rectangle<float> (xOfHz (r, f0) + 6, r.getY() + 2, 140, 14), Justification::centredLeft);
    }
    // armoniche: barre a 2x, 3x, 4x, 5x della zona; altezza = quanto ne nasce (si muove con la musica)
    const float lvl = ed.proc.eng.harmUi[4].load(), live = jlimit (0.15f, 1.0f, 0.15f + std::sqrt (lvl) * 3.0f);
    static const char* hn[4] = { "2a", "3a", "4a", "5a" };
    float k[4];
    switch (ch) { case 0: k[0] = 0.75f; k[1] = 0.25f; k[2] = 0.12f; k[3] = 0.0f; break; case 1: k[0] = 0.2f; k[1] = 0.65f; k[2] = 0.0f; k[3] = 0.1f; break; default: k[0] = 0.0f; k[1] = 0.55f; k[2] = 0.0f; k[3] = 0.4f; }
    for (int j = 0; j < 4; ++j)
    {
        const float f = f0 * (j + 2); if (f > 20000) continue;
        const float x = xOfHz (r, f), hgt = r.getHeight() * 0.92f * jlimit (0.0f, 1.0f, k[j] * (0.25f + 0.75f * amt) * (on ? live : 0.3f) * 1.3f);
        if (k[j] <= 0) { g.setColour (C::dim.withAlpha (0.3f)); g.fillRect (x - 1, r.getBottom() - 4, 2.0f, 4.0f); continue; }
        ColourGradient bg (cc.withAlpha (on ? 0.95f : 0.35f), x, r.getBottom(), cc.brighter (0.4f).withAlpha (on ? 0.7f : 0.2f), x, r.getBottom() - hgt, false);
        g.setGradientFill (bg); g.fillRoundedRectangle (x - 5, r.getBottom() - hgt, 10, hgt, 3);
        if (on) { g.setColour (cc.withAlpha (0.18f)); g.fillRoundedRectangle (x - 10, r.getBottom() - hgt - 4, 20, hgt + 4, 6); }
        g.setColour (C::text.withAlpha (0.8f)); g.setFont (mf (8.0f)); g.drawText (hn[j], Rectangle<float> (x - 12, r.getBottom() - hgt - 14, 24, 12), Justification::centred);
    }
    static const char* cn[3] = { "TUBO: pari, caldo", "NASTRO: terza, morbido", "TRANSISTOR: dispari, grinta" };
    g.setColour (cc.withAlpha (on ? 1.0f : 0.5f)); g.setFont (uf (9.0f)); g.drawText (cn[jlimit (0, 2, ch)], r.withHeight (14).translated (0, 2), Justification::centredRight);
    if (! on) { g.setColour (C::dim); g.setFont (uf (10.0f, false)); g.drawText ("accendi ENHANCER: trascina qui per spostare il fuoco", b, Justification::centred); }
}
void HarmView::mouseDrag (const juce::MouseEvent& e)
{
    auto r = getLocalBounds().toFloat().reduced (16, 14);
    const float f = 20.0f * std::pow (1000.0f, juce::jlimit (0.0f, 1.0f, (e.position.x - r.getX()) / r.getWidth()));
    auto* p = ed.P ("e_focus"); if (! dragging) { p->beginChangeGesture(); dragging = true; }
    p->setValueNotifyingHost (p->convertTo0to1 (juce::jlimit (0.0f, 1.0f, std::log (f / 60.0f) / std::log (150.0f))));
    repaint();
}
void HarmView::mouseUp (const juce::MouseEvent&) { if (dragging) ed.P ("e_focus")->endChangeGesture(); dragging = false; }

// ======================================================================= editor
std::unique_ptr<Knob> Editor::mk (const char* id, const char* label, int style, juce::Colour col, std::function<juce::String()> txt, const char* tip, bool big, bool bip)
{
    auto k = std::make_unique<Knob> (P (id)); k->spec.label = label; k->spec.style = style; k->spec.col = col; k->spec.big = big; k->spec.bipolar = bip; k->text = std::move (txt); k->setTooltip (tip);
    addAndMakeVisible (*k); return k;
}
Editor::Editor (Processor& p) : AudioProcessorEditor (p), proc (p)
{
    skin::init ("tubebox", { 0xffe0b07a, 0xffd99a70, 0xffc9b08a, 0xffe6cfa0 }, 0xffff8a2a); C::sync();
    setLookAndFeel (&lnf);
    addAndMakeVisible (tubes); addAndMakeVisible (harm);
    auto v = [this] (const char* id) { return proc.value (id); };
    auto dbTxt = [v] (const char* id) { return [v, id] { const float x = v (id); return (x > 0.05f ? "+" : "") + juce::String (x, 1) + " dB"; }; };
    auto clkTxt = [v] (const char* id) { return [v, id] { return juce::String (v (id), 1) + "  (" + clock (v (id)) + ")"; }; };
    hgKnobs.push_back (mk ("sat", "SATURATION", 0, C::amber, clkTxt ("sat"), "Saturazione parallela 12AT7, sommata prima del pentodo. Si sente solo con SAT IN acceso.", true));
    hgKnobs.push_back (mk ("pentode", "PENTODE", 0, C::amber, clkTxt ("pentode"), "Il primo stadio: spinge e satura per primo (armoniche dispari, ginocchio netto). Ore 12 = colore senza distorsione.", true));
    hgKnobs.push_back (mk ("triode", "TRIODE", 0, C::amber, clkTxt ("triode"), "Il secondo stadio: seconda armonica e volume d'uscita delle valvole.", true));
    hgKnobs.push_back (mk ("density", "DENSITY", 1, C::orange, [v] { return juce::String (v ("density"), 1); }, "Spinge entrambe le valvole senza alzare il volume: piu' densita' e compressione."));
    hgKnobs.push_back (mk ("air", "AIR", 1, C::cream, [v] { return juce::String (v ("air"), 1); }, "Aria sopra i 10 kHz allo stadio del triodo (con AIR IN acceso)."));
    hgKnobs.push_back (mk ("in", "INPUT", 1, C::cream, dbTxt ("in"), "Quanto entra nelle valvole.", false, true));
    hgKnobs.push_back (mk ("mix", "MIX", 1, C::amber, [v] { return juce::String (juce::roundToInt (v ("mix") * 100)) + "%"; }, "Valvole in parallelo col suono pulito."));
    hgKnobs.push_back (mk ("out", "OUTPUT", 1, C::cream, dbTxt ("out"), "Volume finale (anche dell'enhancer).", false, true));
    for (auto& k : hgKnobs) k->defV = k->param->getDefaultValue();
    enKnobs.push_back (mk ("e_amt", "QUANTITA'", 0, C::amber, [v] { return juce::String (v ("e_amt"), 1); }, "Quante armoniche nascono dalla zona del FUOCO. Si sommano al suono: il pulito non cambia.", true));
    enKnobs.push_back (mk ("e_focus", "FUOCO", 0, C::amber, [v] { const float f = focusHz (v ("e_focus")); return f >= 1000 ? juce::String (f / 1000, 1) + " kHz" : juce::String (juce::roundToInt (f)) + " Hz"; }, "Dove nascono le armoniche: a sinistra i bassi (l'808 si sente sul telefono), a destra l'aria.", true));
    enKnobs.push_back (mk ("e_width", "LARGHEZZA", 1, C::cyan, [v] { return juce::String (juce::roundToInt (v ("e_width") * 100)) + "%"; }, "Larghezza delle armoniche: 0 = al centro, 100% = come il suono, 200% = larghe."));
    for (auto& k : enKnobs) k->defV = k->param->getDefaultValue();
    btns = {
        { {}, "hg_on", -1, "POWER", C::red }, { {}, "ms", -1, "MID / SIDE", C::cyan },
        { {}, "sat_in", -1, "SAT IN", C::amber }, { {}, "alt", -1, "ALT TUBE", C::orange },
        { {}, "satFreq", 0, "LOW", C::amber }, { {}, "satFreq", 1, "FLAT", C::amber }, { {}, "satFreq", 2, "HIGH", C::amber },
        { {}, "calib", 0, "DARK", C::amber }, { {}, "calib", 1, "NORMAL", C::amber }, { {}, "calib", 2, "BRIGHT", C::amber },
        { {}, "air_in", -1, "AIR IN", C::cream },
        { {}, "mb_on", -1, "ENHANCER", C::red },
        { {}, "char", 0, "TUBO", charCol (0) }, { {}, "char", 1, "NASTRO", charCol (1) }, { {}, "char", 2, "TRANSISTOR", charCol (2) } };
    addAndMakeVisible (sizeChip);
    setupBrowser();
    scaler.setup();
    startTimerHz (30);
}
void Editor::resized()
{
    const float s = scaler.update();
    presetR = { 412, 15, 360, 36 }; prevR = { 374, 15, 36, 36 }; nextR = { 774, 15, 36, 36 }; abR = { 850, 15, 120, 36 }; themeR = { 1060, 19, 100, 28 };
    const auto hc = hgCard(), ec = enCard();
    tubes.setBounds (juce::Rectangle<float> (hc.getX() + 18, hc.getY() + 46, 330, 270).toNearestInt());
    // stadi in serie: SAT -> PENTODE -> TRIODE -> AIR
    const float x0 = hc.getX() + 372;
    hgKnobs[0]->setBounds (juce::Rectangle<float> (x0, hc.getY() + 82, 140, 168).toNearestInt());
    hgKnobs[1]->setBounds (juce::Rectangle<float> (x0 + 150, hc.getY() + 46, 150, 190).toNearestInt());
    hgKnobs[2]->setBounds (juce::Rectangle<float> (x0 + 310, hc.getY() + 46, 150, 190).toNearestInt());
    hgKnobs[3]->setBounds (juce::Rectangle<float> (x0 + 175, hc.getY() + 252, 100, 110).toNearestInt());
    hgKnobs[4]->setBounds (juce::Rectangle<float> (x0 + 472, hc.getY() + 82, 110, 130).toNearestInt());
    hgKnobs[5]->setBounds (juce::Rectangle<float> (x0 + 472, hc.getY() + 252, 110, 110).toNearestInt());
    hgKnobs[6]->setBounds (juce::Rectangle<float> (hc.getRight() - 196, hc.getY() + 46, 88, 112).toNearestInt());
    hgKnobs[7]->setBounds (juce::Rectangle<float> (hc.getRight() - 104, hc.getY() + 46, 88, 112).toNearestInt());
    auto B = [this] (const juce::String& id, int val) -> juce::Rectangle<float>& { for (auto& b : btns) if (b.id == id && b.value == val) return b.r; return btns[0].r; };
    B ("hg_on", -1) = { hc.getX() + 18, hc.getY() + 330, 160, 36 }; B ("ms", -1) = { hc.getX() + 188, hc.getY() + 330, 160, 36 };
    B ("sat_in", -1) = { x0 + 10, hc.getY() + 46, 120, 28 };
    for (int i = 0; i < 3; ++i) B ("satFreq", i) = { x0 + i * 47.0f, hc.getY() + 262, 44, 26 };
    B ("alt", -1) = { x0 + 10, hc.getY() + 300, 120, 28 };
    for (int i = 0; i < 3; ++i) B ("calib", i) = { x0 + 300 + i * 57.0f, hc.getY() + 262, 54, 26 };
    B ("air_in", -1) = { x0 + 477, hc.getY() + 46, 100, 28 };
    B ("mb_on", -1) = { ec.getX() + 18, ec.getY() + 46, 170, 36 };
    for (int i = 0; i < 3; ++i) B ("char", i) = { ec.getX() + 204, ec.getY() + 46 + i * 44.0f, 136, 36 };
    enKnobs[0]->setBounds (juce::Rectangle<float> (ec.getX() + 18, ec.getY() + 92, 170, 186).toNearestInt());
    harm.setBounds (juce::Rectangle<float> (ec.getX() + 354, ec.getY() + 46, 520, 228).toNearestInt());
    enKnobs[1]->setBounds (juce::Rectangle<float> (ec.getX() + 888, ec.getY() + 46, 130, 170).toNearestInt());
    enKnobs[2]->setBounds (juce::Rectangle<float> (ec.getX() + 1024, ec.getY() + 70, 104, 130).toNearestInt());
    sizeChip.setBounds (W - 76, H - 16, 58, 14);
    browser.setBounds (0, 0, W, H);
    jlook::scaleChildren (*this, s);
}
juce::String Editor::staticKey()
{
    juce::String k; k << skin::cur() << "/" << proc.preset << "/" << proc.ab.label() << "/" << hoverBtn << "/" << hoverTop;
    for (auto& b : btns) k << (proc.value (b.id.toRawUTF8()) > (b.value < 0 ? 0.5f : -1.0f) ? (b.value < 0 ? "1" : juce::String (juce::roundToInt (proc.value (b.id.toRawUTF8())))) : "0");
    return k;
}
void Editor::paintStatic (juce::Graphics& g)
{
    const float Wf = (float) W, Hf = (float) H;
    jlook::backdrop (g, { 0, 0, Wf, Hf }, C::orange, C::amber);
    jlook::header (g, { 0, 0, Wf, 64 }, "TUBE BOX", C::orange, U ("JTB-2  PENTODE / TRIODE / HARMONIC ENHANCER"), C::amber, C::orange);
    lcd (g, presetR.toFloat(), 8); jlook::glass (g, presetR.toFloat(), 8);
    {
        const auto& pr = presets()[(size_t) juce::jlimit (0, (int) presets().size() - 1, proc.preset)];
        const juce::String nm (pr.name); const bool grp = nm.contains (" | ");
        g.setColour (C::dim); g.setFont (uf (7.5f)); g.drawText (grp ? nm.upToFirstOccurrenceOf (" | ", false, false).toUpperCase() : juce::String ("BASE"), presetR.toFloat().withWidth (80).translated (12, 0), juce::Justification::centredLeft);
        glowText (g, (grp ? nm.fromFirstOccurrenceOf (" | ", false, false) : nm).toUpperCase(), presetR.toFloat().withTrimmedLeft (60).withTrimmedRight (20), C::amber, mf (12.5f), juce::Justification::centred);
    }
    g.setColour (C::amber); g.setFont (uf (20.0f));
    g.drawText (U ("\xe2\x80\xb9"), prevR, juce::Justification::centred); g.drawText (U ("\xe2\x80\xba"), nextR, juce::Justification::centred);
    jlook::button (g, abR.toFloat(), C::cyan, proc.ab.cur == 1, hoverTop == 0, "MEMORIA " + proc.ab.label(), uf (11.0f));
    skin::chipDraw (g, themeR.toFloat(), hoverTop == 1);
    // ---- valvole
    {
        const auto hc = hgCard();
        hw::faceplate (g, hc, juce::Colour (0xff1d1a17), juce::Colour (0xff0d0c0b), on ("hg_on") ? C::orange : C::faint, 32, 7, 12);
        g.setColour (C::text); g.setFont (uf (12.0f)); g.drawText ("VALVOLE HG", juce::Rectangle<float> (hc.getX() + 16, hc.getY() + 4, 160, 24), juce::Justification::centredLeft);
        g.setColour (C::dim); g.setFont (uf (9.0f, false));
        g.drawText (U ("6U8A pentodo + triodo in serie  \xc2\xb7  saturazione parallela 12AT7  \xc2\xb7  trasformatori  \xc2\xb7  4x a fase lineare"), juce::Rectangle<float> (hc.getX() + 150, hc.getY() + 4, 700, 24), juce::Justification::centredLeft);
        hw::jewel (g, hc.getRight() - 22, hc.getY() + 16, 5.0f, on ("hg_on"), C::red);
        const float x0 = hc.getX() + 372;
        // la linea degli stadi in serie, come la serigrafia dell'hardware
        g.setColour (C::amber.withAlpha (0.35f)); g.fillRect (x0, hc.getY() + 372.0f, 582.0f, 1.0f);
        g.setFont (uf (8.0f)); g.setColour (C::dim);
        g.drawText (U ("SAT  \xe2\x86\x92  PENTODE  \xe2\x86\x92  AIR  \xe2\x86\x92  TRIODE  \xe2\x86\x92  CALIBRATION"), juce::Rectangle<float> (x0, hc.getY() + 376.0f, 582.0f, 14), juce::Justification::centred);
        g.drawText ("SAT FREQ", juce::Rectangle<float> (x0, hc.getY() + 248.0f, 140, 12), juce::Justification::centred);
        g.drawText ("CALIBRATION", juce::Rectangle<float> (x0 + 300, hc.getY() + 248.0f, 171, 12), juce::Justification::centred);
        for (float dx : { 145.0f, 305.0f, 466.0f }) { g.setColour (C::text.withAlpha (0.06f)); g.fillRect (x0 + dx, hc.getY() + 50.0f, 1.0f, 310.0f); }
        g.setColour (C::dim); g.setFont (uf (9.5f, false));
        g.drawFittedText (U ("PENTODE spinge e satura per primo, TRIODE da' la seconda armonica e il volume. Ore 12 = colore senza distorsione, oltre le 14 si sente il crunch.\n\nSATURATION aggiunge in parallelo una valvola colorata dal SAT FREQ. DENSITY spinge tutto senza alzare il volume."),
                          juce::Rectangle<int> ((int) hc.getRight() - 196, (int) hc.getY() + 170, 180, 190), juce::Justification::topLeft, 12);
    }
    // ---- enhancer
    {
        const auto ec = enCard();
        hw::faceplate (g, ec, juce::Colour (0xff1c1a20), juce::Colour (0xff0c0b0e), on ("mb_on") ? C::amber : C::faint, 32, 17, 12);
        g.setColour (C::text); g.setFont (uf (12.0f)); g.drawText ("HARMONIC ENHANCER", juce::Rectangle<float> (ec.getX() + 16, ec.getY() + 4, 220, 24), juce::Justification::centredLeft);
        g.setColour (C::dim); g.setFont (uf (9.0f, false));
        g.drawText (U ("armoniche nuove dalla zona che scegli, sommate al suono: il pulito non cambia  \xc2\xb7  trascina nel grafico per spostare il fuoco"), juce::Rectangle<float> (ec.getX() + 220, ec.getY() + 4, 760, 24), juce::Justification::centredLeft);
        hw::jewel (g, ec.getRight() - 22, ec.getY() + 16, 5.0f, on ("mb_on"), C::red);
        g.setColour (C::dim); g.setFont (uf (8.0f)); g.drawText ("CARATTERE", juce::Rectangle<float> (ec.getX() + 204, ec.getY() + 182, 136, 12), juce::Justification::centred);
        hw::bezel (g, juce::Rectangle<float> (ec.getX() + 354, ec.getY() + 46, 520, 228), 8);
    }
    hw::bezel (g, juce::Rectangle<float> (hgCard().getX() + 18, hgCard().getY() + 46, 330, 270), 10);
    for (int i = 0; i < (int) btns.size(); ++i)
    {
        const auto& b = btns[(size_t) i]; const float pv = proc.value (b.id.toRawUTF8());
        const bool lit = b.value < 0 ? pv > 0.5f : juce::roundToInt (pv) == b.value;
        jlook::button (g, b.r, b.col, lit, hoverBtn == i, b.label, uf (b.value < 0 && (b.id == "hg_on" || b.id == "mb_on") ? 12.0f : 9.5f));
    }
    g.setColour (C::dim); g.setFont (uf (9.0f, false));
    g.drawText (U ("doppio clic su una manopola = valore di partenza  \xc2\xb7  shift = movimento fine  \xc2\xb7  spento = suono intatto, latenza compensata"), juce::Rectangle<float> (20, Hf - 24, 900, 16), juce::Justification::centredLeft);
}
void Editor::paint (juce::Graphics& g)
{
    skin::apply(); C::sync();
    g.addTransform (juce::AffineTransform::scale (scaler.s));
    bg.draw (g, { 0, 0, W, H }, staticKey(), [this] (juce::Graphics& g2) { paintStatic (g2); });
    // uscita
    const auto hc = hgCard();
    auto mr = juce::Rectangle<float> (hc.getRight() - 196, hc.getY() + 160, 180, 6);
    const float db = juce::Decibels::gainToDecibels (proc.eng.outPk.load(), -60.0f), n = juce::jlimit (0.0f, 1.0f, (db + 48) / 48);
    g.setColour (juce::Colours::white.withAlpha (0.06f)); g.fillRoundedRectangle (mr, 3);
    g.setColour (db > -0.5f ? C::red : C::amber); g.fillRoundedRectangle (mr.withWidth (mr.getWidth() * n), 3);
}
void Editor::mouseMove (const juce::MouseEvent& e0)
{
    const auto p = e0.position / scaler.s;
    int h = -1; for (int i = 0; i < (int) btns.size(); ++i) if (btns[(size_t) i].r.contains (p)) h = i;
    const int t = abR.toFloat().contains (p) ? 0 : themeR.toFloat().contains (p) ? 1 : -1;
    if (h != hoverBtn || t != hoverTop) { hoverBtn = h; hoverTop = t; repaint(); }
    setMouseCursor (h >= 0 || t >= 0 || presetR.toFloat().contains (p) || prevR.toFloat().contains (p) || nextR.toFloat().contains (p) ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
}
void Editor::mouseDown (const juce::MouseEvent& e0)
{
    const auto p = e0.position / scaler.s;
    const int n = (int) presets().size();
    for (auto& b : btns)
        if (b.r.contains (p))
        {
            auto* rp = P (b.id);
            if (b.value < 0) rp->setValueNotifyingHost (rp->getValue() > 0.5f ? 0.0f : 1.0f);
            else rp->setValueNotifyingHost (rp->convertTo0to1 ((float) b.value));
            repaint(); return;
        }
    if (themeR.toFloat().contains (p)) { skin::cycle(); C::sync(); repaint(); for (auto* c : getChildren()) c->repaint(); return; }
    if (abR.toFloat().contains (p)) { proc.ab.toggle (proc); repaint(); return; }
    if (prevR.toFloat().contains (p)) { proc.loadPreset ((juce::jmax (0, proc.preset) + n - 1) % n); repaint(); return; }
    if (nextR.toFloat().contains (p)) { proc.loadPreset ((proc.preset + 1) % n); repaint(); return; }
    if (presetR.toFloat().contains (p)) { browser.open(); return; }
}
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
    tubes.repaint(); harm.repaint();
    const bool hon = on ("hg_on"), eon = on ("mb_on");
    for (int i = 0; i < (int) hgKnobs.size(); ++i) hgKnobs[(size_t) i]->refresh (i >= 6 || hon);
    for (auto& k : enKnobs) k->refresh (eon);
    if (bg.key != staticKey()) repaint();
    repaint (scaler.map (juce::Rectangle<int> ((int) hgCard().getRight() - 200, (int) hgCard().getY() + 156, 190, 14)));
}
}
