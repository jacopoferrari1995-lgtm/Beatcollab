#include "TapeEditor.h"

namespace jt
{
juce::Font tf (float size, int weight, bool mono)
{
    return mono ? jlook::mono (size) : jlook::ui (size, weight == 0 ? 0 : weight == 1 ? 1 : 2);
}
static inline float shapeFn (float x, float c) { x = juce::jlimit (0.0f, 1.0f, x); return c >= 0 ? 1.0f - std::pow (1.0f - x, 1.0f + 3.0f * c) : std::pow (x, 1.0f - 3.0f * c); }
static juce::Colour fxCol (int f) { return juce::Colour (kFx[f].colour); }
static juce::String pct (float v) { return juce::String (juce::roundToInt (v * 100)) + "%"; }
static juce::String fmtSpec (const PSpec& p, float v)
{
    if (p.table) { const Step* t = p.table == 1 ? kDur : p.table == 2 ? kSize : kRate; return t[juce::jlimit (0, p.table == 3 ? 4 : 5, juce::roundToInt (v))].label; }
    switch (p.fmt)
    {
        case 1: return (v > 0 ? "+" : "") + juce::String (v, 2);
        case 2: return juce::String (v, 2) + juce::String::fromUTF8 ("\xc3\x97");
        case 3: return juce::String (juce::roundToInt (v)) + "ms";
        case 4: return (v > 0 ? "+" : "") + juce::String (v, 1) + "st";
        case 5: return juce::String (v, 1) + juce::String::fromUTF8 ("\xc3\x97");
        case 6: return (v > 0 ? "+" : "") + juce::String (v, 1) + "dB";
        default: return pct (v);
    }
}
static void glow (juce::Graphics& g, const juce::String& t, juce::Rectangle<float> r, juce::Colour c, juce::Font f, juce::Justification j = juce::Justification::centredLeft)
{
    g.setFont (f);
    for (int k = 2; k >= 1; --k) { g.setColour (c.withAlpha (0.12f)); g.drawText (t, r.translated ((float) -k * 0.6f, 0), j); g.drawText (t, r.translated ((float) k * 0.6f, 0), j); }
    g.setColour (c); g.drawText (t, r, j);
}
// pannello in metallo con ombra, smusso e striscia LED opzionale
static void panel (juce::Graphics& g, juce::Rectangle<float> r, float rad, juce::Colour led = juce::Colours::transparentBlack)
{
    jlook::card (g, r, rad, led.isTransparent() ? juce::Colour (0xff7d8494) : led, ! led.isTransparent(), false, false);
}
// schermo LCD incassato
static void inset (juce::Graphics& g, juce::Rectangle<float> r, float rad)
{
    g.setColour (juce::Colours::black.withAlpha (0.65f)); g.fillRoundedRectangle (r.expanded (1.5f), rad + 1.5f);
    juce::ColourGradient cg (K::lcd1, r.getCentreX(), r.getCentreY(), K::lcd2, r.getX(), r.getY(), true); g.setGradientFill (cg); g.fillRoundedRectangle (r, rad);
    g.setColour (juce::Colours::white.withAlpha (0.05f)); g.drawLine (r.getX() + rad, r.getBottom() + 1.5f, r.getRight() - rad, r.getBottom() + 1.5f);
}
static void lcdGrid (juce::Graphics& g, juce::Rectangle<float> r, float step = 10)
{
    g.setColour (K::brand2.withAlpha (0.035f));
    for (float x = r.getX(); x < r.getRight(); x += step) g.drawVerticalLine ((int) x, r.getY(), r.getBottom());
    for (float y = r.getY(); y < r.getBottom(); y += step) g.drawHorizontalLine ((int) y, r.getX(), r.getRight());
}
static void scan (juce::Graphics& g, juce::Rectangle<float> r)
{
    g.setColour (juce::Colours::black.withAlpha (0.2f)); for (float y = r.getY(); y < r.getBottom(); y += 3) g.drawHorizontalLine ((int) y, r.getX(), r.getRight());
    juce::ColourGradient gl (juce::Colours::white.withAlpha (0.045f), r.getX(), r.getY(), juce::Colours::transparentWhite, r.getX() + r.getWidth() * 0.4f, r.getY() + r.getHeight() * 0.7f, false);
    g.setGradientFill (gl); g.fillRect (r);
}
static void caps (juce::Graphics& g, const juce::String& t, juce::Rectangle<int> r, juce::Colour c = K::dim, juce::Justification j = juce::Justification::centredLeft, float size = 9.5f)
{
    g.setColour (c); g.setFont (tf (size, 2)); g.drawText (t.toUpperCase(), r, j);
}

// ======================= Knob: manopola 3D con anello LED =======================
void Knob::paint (juce::Graphics& g)
{
    const bool big = getWidth() > 90;
    const float lh = big ? 30.0f : 27.0f;
    const float size = juce::jmin ((float) getWidth(), getHeight() - lh);
    const float v = param->getValue();
    const bool bip = param->convertFrom0to1 (0.0f) < 0 && param->convertFrom0to1 (1.0f) > 0 && std::abs (param->convertFrom0to1 (0.0f) + param->convertFrom0to1 (1.0f)) < 0.01f;
    jlook::KnobLook kl; kl.big = big; kl.bipolar = bip; kl.hover = hover ? 1.0f : 0.0f; kl.active = drag ? 1.0f : 0.0f;
    jlook::drawKnob (g, { (getWidth() - size) * 0.5f, 0, size, size }, v, col, kl);
    g.setColour (K::dim); g.setFont (tf (big ? 10.5f : 9.0f, 2)); g.drawText (lab.toUpperCase(), juce::Rectangle<float> (0, size + 2, (float) getWidth(), 12), juce::Justification::centred);
    glow (g, fmtFn ? fmtFn (param->convertFrom0to1 (v)) : param->getCurrentValueAsText(), juce::Rectangle<float> (0, size + 14, (float) getWidth(), 13), col, tf (big ? 11.5f : 10.0f, 2, true), juce::Justification::centred);
}
void Knob::setNorm (float v)
{
    v = juce::jlimit (0.0f, 1.0f, v);
    if (auto* c = dynamic_cast<juce::AudioParameterChoice*> (param)) { const int k = c->choices.size(); v = std::round (v * (k - 1)) / (k - 1); }
    param->setValueNotifyingHost (v); if (onUserChange) onUserChange (param->convertFrom0to1 (v)); repaint();
}
void Knob::mouseDown (const juce::MouseEvent& e) { startV = param->getValue(); param->beginChangeGesture(); drag = true; if (e.mods.isAltDown() || e.mods.isCommandDown()) { setNorm (param->getDefaultValue()); drag = false; } }
void Knob::mouseUp (const juce::MouseEvent&) { param->endChangeGesture(); drag = false; repaint(); }
void Knob::mouseDrag (const juce::MouseEvent& e) { if (! drag) return; setNorm (startV - e.getDistanceFromDragStartY() / (e.mods.isShiftDown() ? 700.0f : 160.0f)); }
void Knob::mouseDoubleClick (const juce::MouseEvent&) { setNorm (param->getDefaultValue()); }
void Knob::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w)
{
    float step = 1.0f / 60.0f;
    if (auto* c = dynamic_cast<juce::AudioParameterChoice*> (param)) step = 1.0f / (c->choices.size() - 1);
    setNorm (param->getValue() + (w.deltaY > 0 ? step : -step));
}

// ======================= Seg / Toggle / bottoni =======================
int Seg::preferredWidth() const { int w = 6; for (auto& s : it) w += (int) juce::GlyphArrangement::getStringWidth (tf (11.0f, 1), s) + 22; return w; }
void Seg::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    inset (g, b, 6);
    const int cur = getter(); float x = 3;
    for (int i = 0; i < it.size(); ++i)
    {
        const float w = juce::GlyphArrangement::getStringWidth (tf (11.0f, 1), it[i]) + 22;
        auto r = juce::Rectangle<float> (x, 3, w, b.getHeight() - 6);
        if (i == cur)
        {
            g.setColour (K::brand2.withAlpha (0.18f)); g.fillRoundedRectangle (r, 4);
            g.setColour (K::brand2.withAlpha (0.8f)); g.drawRoundedRectangle (r, 4, 1.0f);
            glow (g, it[i].toUpperCase(), r, K::brand2, tf (10.5f, 2, true), juce::Justification::centred);
        }
        else { g.setColour (K::dim); g.setFont (tf (10.5f, 2, true)); g.drawText (it[i].toUpperCase(), r.toNearestInt(), juce::Justification::centred); }
        x += w;
    }
    scan (g, b.reduced (2));
}
void Seg::mouseDown (const juce::MouseEvent& e)
{
    float x = 3;
    for (int i = 0; i < it.size(); ++i) { const float w = juce::GlyphArrangement::getStringWidth (tf (11.0f, 1), it[i]) + 22; if (e.position.x >= x && e.position.x < x + w) { setter (i); repaint(); return; } x += w; }
}
void Toggle::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat(); const bool on = get();
    inset (g, b, b.getHeight() / 2);
    const float d = b.getHeight() - 6, x = on ? b.getRight() - d - 3 : 3.0f;
    if (on) { g.setColour (K::brand1.withAlpha (0.35f)); g.fillEllipse (x - 3, 0, d + 6, d + 6); }
    juce::ColourGradient lg (on ? K::brand1.brighter (0.6f) : juce::Colour (0xff4a4f5a), x, 3, on ? K::brand1 : juce::Colour (0xff15171c), x, 3 + d, false);
    g.setGradientFill (lg); g.fillEllipse (x, 3, d, d);
}
void TextBtn::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (1);
    if (round)
    {
        glow (g, text, b, hover ? K::ink : K::brand2, tf (20.0f, 2), juce::Justification::centred);
        return;
    }
    jlook::button (g, b.reduced (2, 1), K::brand1, accent, hover, text.toUpperCase(), tf (11.5f, 2));
}

// ======================= Lista effetti =======================
void FxList::paint (juce::Graphics& g)
{
    const int am = ed.proc.activeMask.load();
    caps (g, "Effetti", { 6, 0, 100, 20 });
    g.setFont (tf (9.0f, 0, true)); g.setColour (K::faint); g.drawText ("clic = modifica", juce::Rectangle<int> (0, 0, getWidth() - 6, 20), juce::Justification::centredRight);
    for (int f = 0; f < kNumFx; ++f)
    {
        auto r = juce::Rectangle<float> (0, 30.0f + f * 40.0f, (float) getWidth(), 36);
        const bool s = f == ed.sel, on = (am >> f) & 1; const auto c = fxCol (f);
        inset (g, r, 6);
        if (s) { g.setColour (c.withAlpha (0.12f)); g.fillRoundedRectangle (r, 6); g.setColour (c.withAlpha (0.7f)); g.drawRoundedRectangle (r, 6, 1.0f); }
        const float dx = r.getX() + 16, dy = r.getCentreY();
        if (on) { g.setColour (c.withAlpha (0.3f)); g.fillEllipse (dx - 8, dy - 8, 16, 16); }
        juce::ColourGradient lg (on ? c.brighter (0.6f) : c.withAlpha (0.25f), dx, dy - 5, on ? c : c.withAlpha (0.1f), dx, dy + 5, false);
        g.setGradientFill (lg); g.fillEllipse (dx - 5, dy - 5, 10, 10);
        if (s || on) glow (g, juce::String (kFx[f].name).toUpperCase(), r.withTrimmedLeft (30), on ? c : K::ink, tf (11.0f, 2), juce::Justification::centredLeft);
        else { g.setColour (K::dim); g.setFont (tf (11.0f, 2)); g.drawText (juce::String (kFx[f].name).toUpperCase(), r.withTrimmedLeft (30).toNearestInt(), juce::Justification::centredLeft); }
        auto kk = juce::Rectangle<float> (r.getRight() - 26, dy - 8, 16, 16);
        g.setColour (K::faint); g.drawRoundedRectangle (kk, 3, 1); g.setFont (tf (9.5f, 2, true)); g.drawText (juce::String::charToString (kFx[f].key), kk.toNearestInt(), juce::Justification::centred);
    }
}
void FxList::mouseDown (const juce::MouseEvent& e) { const int f = (int) ((e.position.y - 30) / 40); if (f >= 0 && f < kNumFx) ed.select (f); }

// ======================= Display =======================
void Display::paint (juce::Graphics& g)
{
    auto& P = ed.proc; auto full = getLocalBounds().toFloat();
    panel (g, full, 12, K::brand2);
    auto b = full.reduced (10);
    inset (g, b, 8);
    lcdGrid (g, b.reduced (4), 12);
    const int steps = ed.steps(); const double beats = steps / 4.0;
    auto hdr = b.removeFromTop (30).reduced (12, 0);
    const double beat = P.uiBeat.load(); const double pb = std::fmod (beat, beats);
    juce::String pos; pos << "BAR " << ((int) (pb / 4) + 1) << "." << ((int) std::fmod (pb, 4.0) + 1);
    glow (g, pos, hdr, K::amber, tf (13.0f, 2, true), juce::Justification::centredLeft);
    const float spd = P.uiSpeed.load();
    glow (g, "SPEED " + juce::String (spd, 2) + juce::String::fromUTF8 ("\xc3\x97") + "   " + juce::String (P.uiBpm.load(), 1) + " BPM" + (P.uiHost.load() ? "  HOST" : ""), hdr, K::brand2, tf (11.0f, 2, true), juce::Justification::centredRight);
    // bobine 3D
    reelA += spd * 0.12f;
    for (int k = 0; k < 2; ++k)
    {
        const float cx = hdr.getCentreX() + (k ? 24.0f : -24.0f), cy = hdr.getCentreY() + 1, r = 12;
        juce::ColourGradient rg (juce::Colour (0xff3a3f4a), cx, cy - r, juce::Colour (0xff0e1014), cx, cy + r, false); g.setGradientFill (rg); g.fillEllipse (cx - r, cy - r, r * 2, r * 2);
        g.setColour (K::amber.withAlpha (0.7f)); g.drawEllipse (cx - r, cy - r, r * 2, r * 2, 1.2f);
        for (int s2 = 0; s2 < 3; ++s2) { const float a = reelA + s2 * 2.094f; g.setColour (K::amber.withAlpha (0.8f)); g.drawLine (cx + std::cos (a) * 3, cy + std::sin (a) * 3, cx + std::cos (a) * (r - 2), cy + std::sin (a) * (r - 2), 2.0f); }
        g.setColour (juce::Colours::black); g.fillEllipse (cx - 2.5f, cy - 2.5f, 5, 5);
    }
    g.setColour (K::amber.withAlpha (0.35f)); g.drawLine (hdr.getCentreX() - 24, hdr.getCentreY() + 13, hdr.getCentreX() + 24, hdr.getCentreY() + 13, 1.0f);
    auto wave = b.reduced (12, 4).withTrimmedBottom (56);
    auto curve = b.reduced (12, 0).removeFromBottom (54).withTrimmedBottom (8);
    for (int q = 0; q <= (int) beats; ++q)
    {
        const float x = wave.getX() + wave.getWidth() * (float) (q / beats);
        g.setColour (K::brand2.withAlpha (q % 4 == 0 ? 0.16f : 0.05f)); g.drawVerticalLine ((int) x, wave.getY(), curve.getBottom());
        if (q % 4 == 0 && q < beats) { g.setColour (K::brand2.withAlpha (0.6f)); g.setFont (tf (9.0f, 2, true)); g.drawText (juce::String (q / 4 + 1), juce::Rectangle<float> (x + 4, wave.getY(), 30, 12), juce::Justification::centredLeft); }
    }
    const int W = (int) wave.getWidth(); const float cy = wave.getCentreY(), hh = wave.getHeight() * 0.46f;
    for (int x = 0; x < W; ++x)
    {
        const int c = juce::jlimit (0, kCols - 1, x * kCols / W);
        const float d = juce::jmin (1.0f, P.dispDry[c].load() * 1.2f), w = juce::jmin (1.0f, P.dispWet[c].load() * 1.2f); const int m = P.dispMask[c].load();
        g.setColour (juce::Colours::white.withAlpha (0.07f)); g.drawVerticalLine ((int) wave.getX() + x, cy - d * hh, cy + d * hh + 1);
        juce::Colour wc = K::brand2;
        for (int f = 0; f < kNumFx; ++f) if ((m >> f) & 1) { wc = fxCol (f); break; }
        g.setColour (wc.withAlpha (m ? 0.95f : 0.6f)); g.drawVerticalLine ((int) wave.getX() + x, cy - w * hh, cy + w * hh + 1);
    }
    g.setColour (K::brand2.withAlpha (0.08f)); g.drawHorizontalLine ((int) (curve.getY() + curve.getHeight() * 0.5f), curve.getX(), curve.getRight());
    juce::Path sp; for (int x = 0; x < W; x += 2)
    {
        const int c = juce::jlimit (0, kCols - 1, x * kCols / W); const float v = juce::jlimit (-2.0f, 2.0f, P.dispSpd[c].load());
        const float y = curve.getCentreY() - v * curve.getHeight() * 0.24f;
        if (x == 0) sp.startNewSubPath (curve.getX() + x, y); else sp.lineTo (curve.getX() + x, y);
    }
    g.setColour (K::brand1.withAlpha (0.3f)); g.strokePath (sp, juce::PathStrokeType (4.5f));
    g.setColour (K::brand1); g.strokePath (sp, juce::PathStrokeType (1.6f));
    caps (g, "speed", curve.toNearestInt().removeFromTop (12), K::brand1.withAlpha (0.7f), juce::Justification::centredLeft, 8.5f);
    const float px = wave.getX() + wave.getWidth() * (float) (pb / beats);
    g.setColour (K::amber.withAlpha (0.18f)); g.fillRect (px - 7, wave.getY(), 7.0f, curve.getBottom() - wave.getY());
    g.setColour (K::amber); g.drawLine (px, wave.getY(), px, curve.getBottom(), 1.4f);
    scan (g, b.withY (full.getY() + 10).withHeight (full.getHeight() - 20));
}

// ======================= Pad =======================
void PadBtn::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (2); const bool on = down || ((ed.proc.activeMask.load() >> fx) & 1); const auto c = fxCol (fx);
    // pad in gomma retroilluminato
    g.setColour (juce::Colours::black.withAlpha (0.75f)); g.fillRoundedRectangle (b.expanded (2).translated (0, 2), 9);
    if (on) { g.setColour (c.withAlpha (0.35f)); g.fillRoundedRectangle (b.expanded (4), 11); b = b.translated (0, 1.5f); }
    juce::ColourGradient body (on ? c.interpolatedWith (juce::Colours::white, 0.15f) : juce::Colour (0xff2c2f37), b.getCentreX(), b.getCentreY(),
                               on ? c.interpolatedWith (juce::Colour (0xff15171c), 0.45f) : juce::Colour (0xff17191e), b.getX(), b.getBottom(), true);
    g.setGradientFill (body); g.fillRoundedRectangle (b, 8);
    if (! on) { g.setColour (c.withAlpha (0.12f)); g.fillRoundedRectangle (b.reduced (b.getWidth() * 0.3f, b.getHeight() * 0.3f), 6); }
    juce::ColourGradient spec (juce::Colours::white.withAlpha (on ? 0.25f : 0.08f), b.getX(), b.getY(), juce::Colours::transparentWhite, b.getX(), b.getY() + b.getHeight() * 0.5f, false);
    g.setGradientFill (spec); g.fillRoundedRectangle (b, 8);
    g.setColour (on ? c.brighter (0.4f) : juce::Colours::white.withAlpha (0.1f)); g.drawRoundedRectangle (b, 8, 1.0f);
    g.setColour (on ? juce::Colour (0xff0b0c10) : K::ink); g.setFont (tf (10.5f, 2)); g.drawText (juce::String (kFx[fx].name).toUpperCase(), b.reduced (8, 7).toNearestInt(), juce::Justification::topLeft);
    const bool shot = ed.proc.value ("mode") > 0.5f;
    g.setColour (on ? juce::Colour (0xff0b0c10).withAlpha (0.7f) : c.withAlpha (0.8f)); g.setFont (tf (9.0f, 2, true));
    g.drawText (juce::String::charToString (kFx[fx].key) + juce::String::fromUTF8 (" \xc2\xb7 ") + (shot ? "SHOT" : "HOLD"), b.reduced (8, 7).toNearestInt(), juce::Justification::bottomLeft);
}
void PadBtn::mouseDown (const juce::MouseEvent&) { down = true; ed.select (fx); ed.proc.uiPad[fx] = true; repaint(); }
void PadBtn::mouseUp (const juce::MouseEvent&) { down = false; ed.proc.uiPad[fx] = false; repaint(); }

// ======================= Editor effetto =======================
juce::Rectangle<int> FxEditor::curveArea() const { return { 16, 44, getWidth() * 36 / 100 - 16, getHeight() - 58 }; }
void FxEditor::rebuild()
{
    knobs.clear();
    const auto c = fxCol (ed.sel);
    for (int i = 0; i < kNumParams; ++i)
        if (kParams[i].fx == ed.sel)
        {
            const auto spec = kParams[i];
            auto* k = knobs.add (new Knob (ed.proc.apvts.getParameter (spec.id), spec.label, c, [spec] (float v) { return fmtSpec (spec, v); }));
            addAndMakeVisible (k);
        }
    resized(); repaint();
}
void FxEditor::resized()
{
    auto a = getLocalBounds().withTrimmedLeft (getWidth() * 36 / 100 + 6).reduced (6, 10).withTrimmedTop (28);
    const int n = knobs.size(); if (n == 0) return;
    const int w = a.getWidth() / n, s = juce::jmin (78, w - 4);
    for (int i = 0; i < n; ++i) knobs[i]->setBounds (a.getX() + i * w + (w - s) / 2, a.getCentreY() - (s + 27) / 2, s, s + 27);
}
void FxEditor::paint (juce::Graphics& g)
{
    const auto c = fxCol (ed.sel);
    panel (g, getLocalBounds().toFloat(), 12, c);
    g.setColour (c.withAlpha (0.3f)); g.fillEllipse (12, 11, 18, 18); g.setColour (c.brighter (0.4f)); g.fillEllipse (16, 15, 10, 10);
    glow (g, juce::String (kFx[ed.sel].name).toUpperCase(), { 36, 8, 220, 24 }, K::ink, tf (15.0f, 2));
    const float nw = juce::GlyphArrangement::getStringWidth (tf (15.0f, 2), juce::String (kFx[ed.sel].name).toUpperCase());
    g.setColour (K::dim); g.setFont (tf (11.0f)); g.drawText (kFx[ed.sel].desc, (int) (48 + nw), 9, 360, 24, juce::Justification::centredLeft);
    g.setColour (c); g.fillRoundedRectangle (14.0f, 36.0f, getWidth() - 28.0f, 2.0f, 1.0f);
    auto ca = curveArea().toFloat().withTrimmedTop (6);
    inset (g, ca, 6);
    auto in = ca.reduced (10, 10);
    lcdGrid (g, in, 10);
    auto& P = ed.proc; const int f = ed.sel; juce::Path p; const int M = 120;
    for (int i = 0; i <= M; ++i)
    {
        const float x = i / (float) M; float y = 1;
        switch (f)
        {
            case STOP:  y = 1 - shapeFn (x, P.fxValue (STOP, "curve")); break;
            case START: y = shapeFn (x, -P.fxValue (START, "curve")); break;
            case SPIN:  { const float e = x, a = juce::jmin (1.0f, e * 10); y = -P.fxValue (SPIN, "force") * a * std::pow (1 - e, 1.6f) / 5.0f; break; }
            case HALF:  y = x < 0.15f ? 1 - (1 - P.fxValue (HALF, "speed")) * (x / 0.15f) : P.fxValue (HALF, "speed"); break;
            case REV:   y = -P.fxValue (REV, "speed") / 1.5f; break;
            case STUT:  { const float sz = P.fxValue (STUT, "size"), roll = P.fxValue (STUT, "roll"); float t = x * 2.0f, len = sz, acc = 0; int k = 0;
                          while (acc + len < t && k < 64) { acc += len; ++k; len = juce::jmax (0.125f, sz * std::pow (1 - roll * 0.22f, (float) k)); }
                          const float w = (t - acc) / len; y = w < P.fxValue (STUT, "gate") ? std::pow (2.0f, P.fxValue (STUT, "pitch") * x / 12.0f) * 0.5f : 0.05f; break; }
            case LOOP:  { const float z = P.fxValue (LOOP, "size") / 8.0f; y = 0.2f + 0.7f * std::fmod (x, z) / z; break; }
            case GLITCH:{ juce::Random r (int (x * 16) * 7 + 3); const float d = P.fxValue (GLITCH, "dens"); y = r.nextFloat() < d ? (r.nextFloat() * 1.6f - 0.5f) : 1; break; }
            case GATE:  { const float rt = P.fxValue (GATE, "rate") * 2.0f, ph = std::fmod (x * 4, rt) / rt; y = ph < P.fxValue (GATE, "duty") ? 1 : 1 - P.fxValue (GATE, "depth"); break; }
            default: break;
        }
        const float px = in.getX() + x * in.getWidth(), py = in.getCentreY() + in.getHeight() * 0.5f - juce::jlimit (-1.0f, 1.3f, y) * in.getHeight() * 0.42f - in.getHeight() * 0.08f;
        if (i == 0) p.startNewSubPath (px, py); else p.lineTo (px, py);
    }
    juce::Path fill (p); fill.lineTo (in.getRight(), in.getBottom()); fill.lineTo (in.getX(), in.getBottom()); fill.closeSubPath();
    juce::ColourGradient fg (c.withAlpha (0.28f), 0, in.getY(), c.withAlpha (0.0f), 0, in.getBottom(), false); g.setGradientFill (fg); g.fillPath (fill);
    g.setColour (c.withAlpha (0.3f)); g.strokePath (p, juce::PathStrokeType (6.0f)); g.setColour (c); g.strokePath (p, juce::PathStrokeType (2.0f));
    scan (g, ca.reduced (2));
}

// ======================= Coreografia =======================
juce::Rectangle<float> Timeline::grid() const { return getLocalBounds().toFloat().reduced (12, 0).withTrimmedTop (22).withTrimmedBottom (10).withTrimmedLeft (150); }
int Timeline::span() const { return ed.res == 0 ? 1 : ed.res == 1 ? 2 : 4; }
bool Timeline::cellAt (juce::Point<float> p, int& lane, int& step) const
{
    auto gr = grid(); if (! gr.contains (p)) return false;
    lane = juce::jlimit (0, kNumFx - 1, (int) ((p.y - gr.getY()) / (gr.getHeight() / kNumFx)));
    step = juce::jlimit (0, ed.steps() - 1, (int) ((p.x - gr.getX()) / (gr.getWidth() / ed.steps())));
    step = step / span() * span();
    return true;
}
void Timeline::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    inset (g, b, 8);
    auto gr = grid(); const int S = ed.steps(), sp = span(); const float cw = gr.getWidth() / S, lh = gr.getHeight() / kNumFx;
    const int cur = ed.proc.uiStep.load(); const bool on = ed.proc.value ("seqOn") > 0.5f;
    // intestazione con numeri di battuta e movimenti
    for (int s = 0; s < S; s += 4)
    {
        const float x = gr.getX() + s * cw;
        if (s % 16 == 0) { g.setColour (K::amber.withAlpha (0.9f)); g.setFont (tf (10.0f, 2, true)); g.drawText ("BAR " + juce::String (s / 16 + 1), juce::Rectangle<float> (x + 3, 4, 60, 14), juce::Justification::centredLeft); }
        else { g.setColour (K::dim); g.setFont (tf (9.0f, 1, true)); g.drawText (juce::String (s / 16 + 1) + "." + juce::String ((s % 16) / 4 + 1), juce::Rectangle<float> (x + 3, 4, 40, 14), juce::Justification::centredLeft); }
    }
    g.setColour (K::faint); g.setFont (tf (8.5f, 1));
    g.drawText ("clic disegna, tasto dx cancella", juce::Rectangle<float> (14, 0, 134, 11), juce::Justification::centredLeft);
    g.drawText ("doppio clic sul nome svuota", juce::Rectangle<float> (14, 10, 134, 11), juce::Justification::centredLeft);
    for (int f = 0; f < kNumFx; ++f)
    {
        const float y = gr.getY() + f * lh; const auto c = fxCol (f);
        const bool selL = f == ed.sel, hovL = f == hLane;
        if (selL || hovL) { g.setColour (c.withAlpha (selL ? 0.08f : 0.05f)); g.fillRect (12.0f, y, b.getWidth() - 24, lh); }
        g.setColour (c.withAlpha (0.3f)); g.fillEllipse (16, y + lh * 0.5f - 7, 14, 14); g.setColour (c); g.fillEllipse (19.5f, y + lh * 0.5f - 3.5f, 7, 7);
        g.setColour (selL ? c : K::ink.withAlpha (0.85f)); g.setFont (tf (11.0f, 2)); g.drawText (juce::String (kFx[f].name).toUpperCase(), juce::Rectangle<float> (36, y, 110, lh), juce::Justification::centredLeft);
        // celle vuote alla risoluzione scelta
        for (int s = 0; s < S; s += sp)
        {
            auto r = juce::Rectangle<float> (gr.getX() + s * cw + 1.5f, y + 2.5f, sp * cw - 3, lh - 5);
            const bool beat = (s / 4) % 2 == 0;
            g.setColour (juce::Colours::white.withAlpha (beat ? 0.045f : 0.025f)); g.fillRoundedRectangle (r, 3);
            g.setColour (juce::Colours::white.withAlpha (0.05f)); g.drawRoundedRectangle (r, 3, 1.0f);
        }
        // blocchi attivi
        int s = 0;
        while (s < S)
        {
            if (! ed.proc.seq[f][s].load()) { ++s; continue; }
            int e = s; while (e < S && ed.proc.seq[f][e].load()) ++e;
            auto r = juce::Rectangle<float> (gr.getX() + s * cw + 1.5f, y + 2.5f, (e - s) * cw - 3, lh - 5);
            const bool playing = on && cur >= s && cur < e;
            g.setColour (c.withAlpha (playing ? 0.5f : 0.2f)); g.fillRoundedRectangle (r.expanded (playing ? 3.0f : 1.5f), 5);
            juce::ColourGradient cg (playing ? c.brighter (0.5f) : c.brighter (0.15f), 0, r.getY(), c.darker (0.3f), 0, r.getBottom(), false); g.setGradientFill (cg);
            g.fillRoundedRectangle (r, 4);
            g.setColour (juce::Colours::white.withAlpha (0.4f)); g.drawHorizontalLine ((int) r.getY() + 1, r.getX() + 3, r.getRight() - 3);
            for (int k = s + 1; k < e; ++k) { g.setColour (juce::Colours::black.withAlpha (0.12f)); g.drawVerticalLine ((int) (gr.getX() + k * cw), r.getY() + 3, r.getBottom() - 3); }
            if (r.getWidth() > 70) { g.setColour (juce::Colour (0xff0a0c10).withAlpha (0.85f)); g.setFont (tf (10.0f, 2)); g.drawText (juce::String (kFx[f].name).toUpperCase(), r.reduced (7, 0).toNearestInt(), juce::Justification::centredLeft); }
            s = e;
        }
    }
    // linee di battuta e di movimento
    for (int s = 0; s <= S; s += 4)
    {
        const float x = gr.getX() + s * cw;
        g.setColour (s % 16 == 0 ? K::amber.withAlpha (0.45f) : K::brand2.withAlpha (0.14f)); g.fillRect (x - (s % 16 == 0 ? 1.0f : 0.5f), gr.getY(), s % 16 == 0 ? 2.0f : 1.0f, gr.getHeight());
    }
    for (int f = 1; f < kNumFx; ++f) { g.setColour (juce::Colours::white.withAlpha (0.04f)); g.drawHorizontalLine ((int) (gr.getY() + f * lh), 12.0f, gr.getRight()); }
    // cella sotto il mouse
    if (hLane >= 0 && hStep >= 0)
    {
        auto r = juce::Rectangle<float> (gr.getX() + hStep * cw + 1.5f, gr.getY() + hLane * lh + 2.5f, sp * cw - 3, lh - 5);
        g.setColour (fxCol (hLane).withAlpha (0.9f)); g.drawRoundedRectangle (r, 4, 1.6f);
    }
    if (on && cur >= 0 && cur < S) { g.setColour (K::amber.withAlpha (0.16f)); g.fillRect (gr.getX() + cur * cw, gr.getY(), cw, gr.getHeight()); g.setColour (K::amber); g.fillRect (gr.getX() + cur * cw - 1, gr.getY(), 2.0f, gr.getHeight()); }
    scan (g, b.reduced (2));
}
void Timeline::mouseMove (const juce::MouseEvent& e)
{
    int l = -1, s = -1;
    if (! cellAt (e.position, l, s)) { l = -1; s = -1; }
    if (l != hLane || s != hStep) { hLane = l; hStep = s; repaint(); }
    setMouseCursor (l >= 0 ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
}
void Timeline::mouseDown (const juce::MouseEvent& e)
{
    auto gr = grid();
    if (e.position.x < gr.getX() && e.position.y >= gr.getY())   // clic sul nome della corsia: seleziona l'effetto
    {
        const int l = juce::jlimit (0, kNumFx - 1, (int) ((e.position.y - gr.getY()) / (gr.getHeight() / kNumFx)));
        ed.select (l); repaint(); return;
    }
    int l, s; if (! cellAt (e.position, l, s)) return;
    const bool erase = e.mods.isPopupMenu() || e.mods.isAltDown();
    paintLane = l; paintVal = erase ? false : ! ed.proc.seq[l][s].load();
    for (int k = 0; k < span() && s + k < kMaxSteps; ++k) ed.proc.seq[l][s + k] = paintVal;
    ed.select (l); repaint();
}
void Timeline::mouseDrag (const juce::MouseEvent& e)
{
    int l, s; if (paintLane < 0 || ! cellAt (e.position, l, s)) return;
    hLane = paintLane; hStep = s;
    for (int k = 0; k < span() && s + k < kMaxSteps; ++k) ed.proc.seq[paintLane][s + k] = paintVal;
    repaint();
}
void Timeline::mouseDoubleClick (const juce::MouseEvent& e)
{
    auto gr = grid();
    if (e.position.x >= gr.getX() || e.position.y < gr.getY()) return;
    const int l = juce::jlimit (0, kNumFx - 1, (int) ((e.position.y - gr.getY()) / (gr.getHeight() / kNumFx)));
    for (int s = 0; s < kMaxSteps; ++s) ed.proc.seq[l][s] = false;
    repaint();
}

// ======================= Meter =======================
void Meter::paint (juce::Graphics& g)
{
    const float v[2] = { proc.meterL.load(), proc.meterR.load() };
    auto b = getLocalBounds().toFloat();
    inset (g, b, 4);
    b = b.reduced (4, 3);
    for (int c = 0; c < 2; ++c)
    {
        auto r = juce::Rectangle<float> (b.getX(), b.getY() + c * (b.getHeight() / 2 + 1), b.getWidth(), b.getHeight() / 2 - 1);
        const float db = juce::Decibels::gainToDecibels (v[c], -60.0f), n = juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 63.0f);
        const int seg = 28;
        for (int k = 0; k < seg; ++k) { const float t = k / (float) seg; g.setColour (t <= n ? (t > 0.92f ? K::brand1 : t > 0.78f ? K::amber : K::brand2) : juce::Colours::white.withAlpha (0.05f)); g.fillRect (r.getX() + k * r.getWidth() / seg, r.getY(), r.getWidth() / seg - 1.5f, r.getHeight()); }
    }
}

// ======================= Root (fondi e titoli) =======================
void Root::paint (juce::Graphics& g)
{
    const int W = Editor::W, H = Editor::H; const float Wf = (float) W, Hf = (float) H;
    jlook::backdrop (g, { 0, 0, Wf, Hf }, K::brand1, K::brand2);
    jlook::header (g, { 0, 0, Wf, 58 }, "TAPE", K::amber, "JT-9  TAPE MOTION", K::brand1, K::brand2);
    for (auto* c : { (juce::Component*) &ed.disp, (juce::Component*) &ed.fxEd }) jlook::softShadow (g, c->getBounds().toFloat(), 12);
    // colonne
    for (auto r : { ed.sidePanel.toFloat().reduced (6, 8), ed.rightPanel.toFloat().reduced (6, 8), juce::Rectangle<float> (8, 552, Wf - 16, Hf - 32 - 558) }) jlook::softShadow (g, r, 12);
    jlook::card (g, ed.sidePanel.toFloat().reduced (6, 8), 12, K::brand2, true, false, false);
    jlook::card (g, ed.rightPanel.toFloat().reduced (6, 8), 12, K::amber, true, false, false);
    auto right = ed.rightPanel;
    glow (g, "MACRO", juce::Rectangle<float> ((float) right.getX() + 18, 72, 120, 16), K::ink, tf (11.0f, 2));
    g.setColour (K::amber); g.fillRoundedRectangle ((float) right.getX() + 18, 90, (float) right.getWidth() - 36, 2.0f, 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.08f)); g.drawHorizontalLine (290, (float) right.getX() + 18, (float) right.getRight() - 18); g.drawHorizontalLine (458, (float) right.getX() + 18, (float) right.getRight() - 18);
    caps (g, "Nastro", { right.getX() + 18, 296, 120, 16 }, K::amber);
    caps (g, "Uscita", { right.getX() + 18, 464, 120, 16 }, K::amber);
    // riga sotto il display
    caps (g, "Pad", { 226, 264, 40, 28 }, K::dim, juce::Justification::centredLeft, 10.0f);
    caps (g, "Quantizza", { 404, 264, 80, 28 }, K::dim, juce::Justification::centredLeft, 10.0f);
    g.setColour (K::faint); g.setFont (tf (10.5f)); g.drawText (juce::String::fromUTF8 ("tieni premuto \xc2\xb7 tasti 1-9 \xc2\xb7 pi\xc3\xb9 pad insieme si sommano"), juce::Rectangle<int> (640, 264, 366, 28), juce::Justification::centredRight);
    // coreografia
    jlook::card (g, juce::Rectangle<float> (8, 552, Wf - 16, Hf - 32 - 558), 12, K::brand1, true, false, false);
    glow (g, "COREOGRAFIA", juce::Rectangle<float> (20, 560, 110, 30), K::ink, tf (10.5f, 2));
    caps (g, "Battute", { 172, 560, 60, 30 }, K::dim, juce::Justification::centredLeft, 9.5f);
    caps (g, "Griglia", { 432, 560, 50, 30 }, K::dim, juce::Justification::centredLeft, 9.5f);
    caps (g, "Stile", { 652, 560, 46, 30 }, K::brand1, juce::Justification::centredLeft, 9.5f);
    // piede
    g.setColour (juce::Colours::white.withAlpha (0.06f)); g.drawHorizontalLine (H - 30, 0, Wf);
    g.setColour (K::faint); g.setFont (tf (9.5f, 2, true));
    g.drawText (juce::String::fromUTF8 ("JACKPOT.WAV \xc2\xb7 TAPE MOTION \xc2\xb7 AU"), juce::Rectangle<int> (16, H - 30, 400, 30), juce::Justification::centredLeft);
    g.drawText (ed.proc.uiHost.load() ? "BPM SINCRONIZZATI CON L'HOST" : (ed.proc.value ("sync") > 0.5f ? "IN ATTESA DEL PLAY DELL'HOST" : "BPM MANUALI"), juce::Rectangle<int> (W - 416, H - 30, 400, 30), juce::Justification::centredRight);
}

// ======================= Editor =======================
static juce::String fmtOut (float v) { return juce::String (v, 1) + "dB"; }

Editor::Editor (Processor& p) : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (root);
    sidePanel = { 0, 59, 210, 489 }; rightPanel = { 1020, 59, 220, 489 };
    root.addAndMakeVisible (list); root.addAndMakeVisible (disp); root.addAndMakeVisible (fxEd); root.addAndMakeVisible (tl); root.addAndMakeVisible (meter);
    for (int f = 0; f < kNumFx; ++f) root.addAndMakeVisible (pads.add (new PadBtn (*this, f)));
    auto choice = [this] (const char* id) { return [this, id] { return (int) proc.value (id); }; };
    auto setC = [this] (const char* id) { return [this, id] (int i) { auto* pr = proc.apvts.getParameter (id); pr->setValueNotifyingHost (pr->convertTo0to1 ((float) i)); fxEd.repaint(); for (auto* pd : pads) pd->repaint(); tl.repaint(); }; };
    modeSeg = std::make_unique<Seg> (juce::StringArray { "Hold", "One-shot" }, choice ("mode"), setC ("mode"));
    qSeg = std::make_unique<Seg> (juce::StringArray { "Off", "1/16", "1/4", "Bar" }, choice ("quant"), setC ("quant"));
    barsSeg = std::make_unique<Seg> (juce::StringArray { "1", "2", "4", "8" }, choice ("bars"), setC ("bars"));
    resSeg = std::make_unique<Seg> (juce::StringArray { "1/16", "1/8", "1/4" }, [this] { return res; }, [this] (int i) { res = i; });
    seqTog = std::make_unique<Toggle> ([this] { return proc.value ("seqOn") > 0.5f; }, [this] { auto* pr = proc.apvts.getParameter ("seqOn"); pr->setValueNotifyingHost (pr->getValue() > 0.5f ? 0.0f : 1.0f); });
    clearBtn = std::make_unique<TextBtn> ("Svuota", [this] { for (auto& r : proc.seq) for (auto& s : r) s = false; tl.repaint(); });
    // generatore di coreografia
    styleBtn = std::make_unique<TextBtn> ("", [this]
    {
        juce::PopupMenu m; auto* pr = dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter ("genStyle"));
        for (int i = 0; i < pr->choices.size(); ++i) m.addItem (i + 1, pr->choices[i], true, i == pr->getIndex());
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (styleBtn.get()), [this, pr] (int r) { if (r > 0) { pr->setValueNotifyingHost (pr->convertTo0to1 ((float) (r - 1))); styleBtn->text = pr->getCurrentChoiceName(); styleBtn->repaint(); } });
    });
    styleBtn->text = proc.apvts.getParameter ("genStyle")->getCurrentValueAsText();
    genSeg = std::make_unique<Seg> (juce::StringArray { "Soft", "Med", "Hard" }, [this] { const float a = proc.value ("genAmt"); return a < 0.35f ? 0 : a < 0.7f ? 1 : 2; },
                                    [this] (int i) { proc.apvts.getParameter ("genAmt")->setValueNotifyingHost (i == 0 ? 0.2f : i == 1 ? 0.5f : 0.9f); });
    genBtn = std::make_unique<TextBtn> ("Genera", [this] { proc.generate(); fxEd.rebuild(); tl.repaint(); });
    genBtn->accent = true;
    abBtn = std::make_unique<TextBtn> ("A / B", [this] { proc.ab.toggle (proc); abBtn->text = juce::String ("CONFRONTO ") + proc.ab.label(); fxEd.rebuild(); tl.repaint(); root.repaint(); });
    abBtn->text = "CONFRONTO A"; root.addAndMakeVisible (*abBtn);
    for (auto* c : std::initializer_list<juce::Component*> { modeSeg.get(), qSeg.get(), barsSeg.get(), resSeg.get(), seqTog.get(), clearBtn.get(), styleBtn.get(), genSeg.get(), genBtn.get() }) root.addAndMakeVisible (c);
    // preset
    auto showName = [this] { const auto n = proc.getProgramName (proc.presetIndex); pName->text = n.isEmpty() ? juce::String ("Init") : n; pName->repaint(); };
    pPrev = std::make_unique<TextBtn> (juce::String::fromUTF8 ("\xe2\x80\xb9"), [this, showName] { proc.loadPreset ((proc.presetIndex - 1 + proc.getNumPrograms()) % proc.getNumPrograms()); showName(); fxEd.rebuild(); tl.repaint(); });
    pNext = std::make_unique<TextBtn> (juce::String::fromUTF8 ("\xe2\x80\xba"), [this, showName] { proc.loadPreset ((proc.presetIndex + 1) % proc.getNumPrograms()); showName(); fxEd.rebuild(); tl.repaint(); });
    pPrev->round = pNext->round = true;
    pName = std::make_unique<TextBtn> ("", [this, showName]
    {
        juce::PopupMenu m; std::vector<std::pair<juce::String, juce::PopupMenu>> groups;
        for (int i = 0; i < proc.getNumPrograms(); ++i)
        {
            const auto name = proc.getProgramName (i);
            if (! name.contains (" - ")) { m.addItem (i + 1, name, true, i == proc.presetIndex); continue; }
            const auto gname = name.upToFirstOccurrenceOf (" - ", false, false), item = name.fromFirstOccurrenceOf (" - ", false, false);
            bool found = false; for (auto& gr : groups) if (gr.first == gname) { gr.second.addItem (i + 1, item, true, i == proc.presetIndex); found = true; }
            if (! found) { juce::PopupMenu pm; pm.addItem (i + 1, item, true, i == proc.presetIndex); groups.push_back ({ gname, pm }); }
        }
        for (auto& gr : groups) m.addSubMenu (gr.first, gr.second);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (pName.get()), [this, showName] (int r) { if (r > 0) { proc.loadPreset (r - 1); showName(); fxEd.rebuild(); tl.repaint(); } });
    });
    bpmPill = std::make_unique<TextBtn> ("", [this] { auto* pr = proc.apvts.getParameter ("sync"); pr->setValueNotifyingHost (pr->getValue() > 0.5f ? 0.0f : 1.0f); });
    for (auto* c : std::initializer_list<juce::Component*> { pPrev.get(), pNext.get(), pName.get(), bpmPill.get() }) root.addAndMakeVisible (c);
    showName();
    // manopole del nastro
    const auto vio = K::brand1;
    auto add = [this] (const char* id, const char* lab, juce::Colour c, std::function<juce::String (float)> f) { auto* k = tapeKnobs.add (new Knob (proc.apvts.getParameter (id), lab, c, std::move (f))); root.addAndMakeVisible (k); return k; };
    auto* age = add ("age", "Age", juce::Colour (0xffa78bfa), pct); age->onUserChange = [this] (float a) { proc.applyAge (a); for (auto* k : tapeKnobs) k->repaint(); };
    add ("mix", "Mix", vio, pct); add ("out", "Output", vio, fmtOut);
    add ("wow", "Wow", vio, pct); add ("flut", "Flutter", vio, pct); add ("tone", "Tono", vio, pct);
    add ("drive", "Drive", vio, pct); add ("hiss", "Hiss", vio, pct); add ("drop", "Dropout", vio, pct);
    auto* bpmK = add ("bpm", "BPM man.", K::brand2, [] (float v) { return juce::String (v, 1); }); juce::ignoreUnused (bpmK);

    select (STUT);
    setWantsKeyboardFocus (true);
    setResizable (true, true);
    setResizeLimits (W * 6 / 10, H * 6 / 10, W * 3 / 2, H * 3 / 2);
    if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) W / H);
    setSize (W, H);
    startTimerHz (30);
}
Editor::~Editor() { setLookAndFeel (nullptr); }

int Editor::steps() const { static const int b[] = { 1, 2, 4, 8 }; return b[juce::jlimit (0, 3, (int) proc.value ("bars"))] * 16; }
void Editor::select (int f) { sel = f; fxEd.rebuild(); list.repaint(); }

void Editor::resized()
{
    const float s = getWidth() / (float) W;
    root.setBounds (0, 0, W, H); root.setTransform (juce::AffineTransform::scale (s));
    list.setBounds (20, 74, 172, 400);
    disp.setBounds (224, 72, 782, 182);
    modeSeg->setBounds (256, 264, modeSeg->preferredWidth(), 28);
    qSeg->setBounds (484, 264, qSeg->preferredWidth(), 28);
    const int pw = (782 - 8 * 6) / 9;
    for (int f = 0; f < kNumFx; ++f) pads[f]->setBounds (224 + f * (pw + 6), 300, pw, 56);
    fxEd.setBounds (224, 366, 782, 168);
    // destra
    const int rx = rightPanel.getX();
    tapeKnobs[0]->setBounds (rx + 64, 96, 92, 106);
    tapeKnobs[1]->setBounds (rx + 30, 204, 66, 84); tapeKnobs[2]->setBounds (rx + 124, 204, 66, 84);
    for (int i = 0; i < 6; ++i) tapeKnobs[3 + i]->setBounds (rx + 14 + (i % 3) * 66, 314 + (i / 3) * 72, 60, 72);
    tapeKnobs[9]->setBounds (rx + 14, 482, 54, 54);
    tapeKnobs[9]->setVisible (proc.value ("sync") < 0.5f);
    meter.setBounds (rx + (tapeKnobs[9]->isVisible() ? 80 : 18), 490, tapeKnobs[9]->isVisible() ? 124 : 186, 30);
    // intestazione
    pPrev->setBounds (420, 11, 34, 36); pName->setBounds (454, 11, 332, 36); pNext->setBounds (786, 11, 34, 36);
    bpmPill->setBounds (1060, 11, 164, 36);
    abBtn->setBounds (850, 11, 92, 36);
    // coreografia
    seqTog->setBounds (124, 565, 38, 20);
    barsSeg->setBounds (232, 561, barsSeg->preferredWidth(), 28);
    resSeg->setBounds (484, 561, resSeg->preferredWidth(), 28);
    styleBtn->setBounds (700, 561, 120, 28);
    genSeg->setBounds (828, 561, genSeg->preferredWidth(), 28);
    genBtn->setBounds (828 + genSeg->preferredWidth() + 8, 561, 100, 28);
    clearBtn->setBounds (W - 104, 561, 84, 28);
    tl.setBounds (18, 598, W - 36, H - 32 - 598 - 12);
}

void Editor::timerCallback()
{
    const bool host = proc.uiHost.load(), sync = proc.value ("sync") > 0.5f;
    bpmPill->text = (sync ? (host ? juce::String ("HOST  ") : juce::String ("AUTO  ")) : juce::String ("MAN  ")) + juce::String (proc.uiBpm.load(), 1) + " BPM";
    bpmPill->accent = sync && host;
    if (tapeKnobs[9]->isVisible() == sync) resized();
    disp.repaint(); list.repaint(); tl.repaint(); meter.repaint(); bpmPill->repaint();
    for (auto* p : pads) p->repaint();
    for (auto* k : fxEd.knobs) k->repaint();
    { const auto st = proc.apvts.getParameter ("genStyle")->getCurrentValueAsText(); if (styleBtn->text != st) { styleBtn->text = st; styleBtn->repaint(); } }
    for (auto* k : tapeKnobs) k->repaint();
    static int n = 0; if (++n % 15 == 0) root.repaint (0, H - 32, W, 32);
}

bool Editor::keyPressed (const juce::KeyPress& k) { const int c = k.getKeyCode(); return c >= '1' && c <= '9'; }
bool Editor::keyStateChanged (bool)
{
    bool used = false;
    for (int f = 0; f < kNumFx; ++f)
    {
        const bool d = juce::KeyPress::isKeyCurrentlyDown ('1' + f);
        if (d && ! keysDown.count (f)) { keysDown.insert (f); proc.uiPad[f] = true; select (f); used = true; }
        else if (! d && keysDown.count (f)) { keysDown.erase (f); proc.uiPad[f] = false; used = true; }
    }
    return used;
}
}
