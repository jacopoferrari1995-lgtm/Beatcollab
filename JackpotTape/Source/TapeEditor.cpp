#include "TapeEditor.h"

namespace jt
{
juce::Font tf (float size, int weight, bool mono)
{
    auto o = juce::FontOptions (size);
    if (mono) o = o.withName (juce::Font::getDefaultMonospacedFontName());
    if (weight > 0) o = o.withStyle ("Bold");
    return juce::Font (o).withExtraKerningFactor (weight == 1 ? 0.01f : 0.0f);
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
    g.setColour (juce::Colours::black.withAlpha (0.5f)); g.fillRoundedRectangle (r.translated (0, 4), rad);
    juce::ColourGradient cg (K::p1, 0, r.getY(), K::win, 0, r.getBottom(), false); g.setGradientFill (cg); g.fillRoundedRectangle (r, rad);
    juce::Random rr ((int) r.getX() * 7 + (int) r.getY()); for (float y = r.getY() + 3; y < r.getBottom() - 3; y += 2) { g.setColour (juce::Colours::white.withAlpha (rr.nextFloat() * 0.014f)); g.drawHorizontalLine ((int) y, r.getX() + 4, r.getRight() - 4); }
    juce::ColourGradient bev (juce::Colours::white.withAlpha (0.07f), 0, r.getY(), juce::Colours::transparentWhite, 0, r.getY() + 26, false); g.setGradientFill (bev); g.fillRoundedRectangle (r.withHeight (26), rad);
    g.setColour (! led.isTransparent() ? led.withAlpha (0.5f) : juce::Colours::white.withAlpha (0.08f)); g.drawRoundedRectangle (r, rad, 1.2f);
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
    const float cx = getWidth() * 0.5f, cy = size * 0.5f + 1, R = size * 0.5f - 3, r = R - (big ? 11.0f : 7.5f);
    const float v = param->getValue(), a0 = -2.42f, a1 = 2.42f;
    const bool bip = param->convertFrom0to1 (0.0f) < 0 && param->convertFrom0to1 (1.0f) > 0 && std::abs (param->convertFrom0to1 (0.0f) + param->convertFrom0to1 (1.0f)) < 0.01f;
    const int nLed = big ? 31 : 21;
    for (int i = 0; i < nLed; ++i)
    {
        const float t = i / (float) (nLed - 1), a = a0 + t * (a1 - a0);
        const bool lit = bip ? ((v >= 0.5f && t >= 0.5f && t <= v + 0.001f) || (v < 0.5f && t <= 0.5f && t >= v - 0.001f)) : (t <= v + 0.001f && v > 0.002f);
        const float x = cx + std::sin (a) * R, y = cy - std::cos (a) * R, d = big ? 4.2f : 3.2f;
        if (lit) { g.setColour (col.withAlpha (0.25f)); g.fillEllipse (x - d * 1.3f, y - d * 1.3f, d * 2.6f, d * 2.6f); g.setColour (col.brighter (0.25f)); }
        else g.setColour (juce::Colour (0xff23262e));
        g.fillEllipse (x - d * 0.5f, y - d * 0.5f, d, d);
    }
    g.setColour (juce::Colours::black.withAlpha (0.7f)); g.fillEllipse (cx - r - 1, cy - r + 4, r * 2 + 2, r * 2 + 2);
    juce::ColourGradient sk (juce::Colour (0xff4a4f5a), cx, cy - r, juce::Colour (0xff0c0d11), cx, cy + r, false);
    g.setGradientFill (sk); g.fillEllipse (cx - r, cy - r, r * 2, r * 2);
    const float r2 = r * 0.8f;
    juce::ColourGradient cap (juce::Colour (0xff343843), cx - r2 * 0.5f, cy - r2, juce::Colour (0xff111318), cx + r2 * 0.4f, cy + r2, false);
    g.setGradientFill (cap); g.fillEllipse (cx - r2, cy - r2, r2 * 2, r2 * 2);
    for (int i = 0; i < 36; ++i) { const float a = i * juce::MathConstants<float>::twoPi / 36; g.setColour (juce::Colours::black.withAlpha (0.4f)); g.drawLine (cx + std::sin (a) * r2, cy - std::cos (a) * r2, cx + std::sin (a) * r, cy - std::cos (a) * r, 1.0f); }
    juce::ColourGradient spec (juce::Colours::white.withAlpha (0.16f), cx - r2 * 0.4f, cy - r2 * 0.7f, juce::Colours::transparentWhite, cx, cy, true);
    g.setGradientFill (spec); g.fillEllipse (cx - r2, cy - r2, r2 * 2, r2 * 2);
    g.setColour (col.withAlpha (0.55f)); g.drawEllipse (cx - r2, cy - r2, r2 * 2, r2 * 2, 1.0f);
    const float a = a0 + v * (a1 - a0);
    g.setColour (col.withAlpha (0.4f)); g.drawLine (cx + std::sin (a) * r2 * 0.25f, cy - std::cos (a) * r2 * 0.25f, cx + std::sin (a) * r2 * 0.95f, cy - std::cos (a) * r2 * 0.95f, big ? 6.0f : 4.0f);
    g.setColour (juce::Colours::white); g.drawLine (cx + std::sin (a) * r2 * 0.3f, cy - std::cos (a) * r2 * 0.3f, cx + std::sin (a) * r2 * 0.92f, cy - std::cos (a) * r2 * 0.92f, big ? 2.6f : 1.8f);
    g.setColour (K::dim); g.setFont (tf (big ? 10.5f : 9.0f, 2)); g.drawText (lab.toUpperCase(), juce::Rectangle<float> (0, size + 2, (float) getWidth(), 12), juce::Justification::centred);
    glow (g, fmtFn ? fmtFn (param->convertFrom0to1 (v)) : param->getCurrentValueAsText(), juce::Rectangle<float> (0, size + 14, (float) getWidth(), 13), col, tf (big ? 11.5f : 10.0f, 2, true), juce::Justification::centred);
}
void Knob::setNorm (float v)
{
    v = juce::jlimit (0.0f, 1.0f, v);
    if (auto* c = dynamic_cast<juce::AudioParameterChoice*> (param)) { const int k = c->choices.size(); v = std::round (v * (k - 1)) / (k - 1); }
    param->setValueNotifyingHost (v); if (onUserChange) onUserChange (param->convertFrom0to1 (v)); repaint();
}
void Knob::mouseDown (const juce::MouseEvent&) { startV = param->getValue(); param->beginChangeGesture(); }
void Knob::mouseUp (const juce::MouseEvent&) { param->endChangeGesture(); }
void Knob::mouseDrag (const juce::MouseEvent& e) { setNorm (startV - e.getDistanceFromDragStartY() / (e.mods.isShiftDown() ? 700.0f : 160.0f)); }
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
    g.setColour (juce::Colours::black.withAlpha (0.5f)); g.fillRoundedRectangle (b.translated (0, 2), 7);
    juce::ColourGradient rg (hover ? juce::Colour (0xff32363f) : K::p3, 0, b.getY(), juce::Colour (0xff16181d), 0, b.getBottom(), false); g.setGradientFill (rg); g.fillRoundedRectangle (b, 7);
    g.setColour (accent ? K::brand1.withAlpha (0.9f) : juce::Colours::white.withAlpha (0.12f)); g.drawRoundedRectangle (b, 7, accent ? 1.4f : 1.0f);
    if (accent) { g.setColour (K::brand1.withAlpha (0.12f)); g.drawRoundedRectangle (b.expanded (1.5f), 8, 3.0f); }
    glow (g, text.toUpperCase(), b, accent ? K::brand1 : (hover ? K::ink : K::dim), tf (11.0f, 2), juce::Justification::centred);
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
juce::Rectangle<float> Timeline::grid() const { return getLocalBounds().toFloat().reduced (10, 10).withTrimmedLeft (170); }
bool Timeline::cellAt (juce::Point<float> p, int& lane, int& step) const
{
    auto gr = grid(); if (! gr.contains (p)) return false;
    lane = juce::jlimit (0, kNumFx - 1, (int) ((p.y - gr.getY()) / (gr.getHeight() / kNumFx)));
    step = juce::jlimit (0, ed.steps() - 1, (int) ((p.x - gr.getX()) / (gr.getWidth() / ed.steps())));
    return true;
}
void Timeline::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    inset (g, b, 8);
    auto gr = grid(); const int S = ed.steps(); const float cw = gr.getWidth() / S, lh = gr.getHeight() / kNumFx;
    const int cur = ed.proc.uiStep.load(); const bool on = ed.proc.value ("seqOn") > 0.5f;
    for (int f = 0; f < kNumFx; ++f)
    {
        const float y = gr.getY() + f * lh; const auto c = fxCol (f);
        g.setColour (c.withAlpha (0.3f)); g.fillEllipse (14, y + lh * 0.5f - 6, 12, 12); g.setColour (c); g.fillEllipse (17, y + lh * 0.5f - 3, 6, 6);
        g.setColour (K::ink.withAlpha (0.85f)); g.setFont (tf (10.0f, 2)); g.drawText (juce::String (kFx[f].name).toUpperCase(), juce::Rectangle<float> (32, y, 130, lh), juce::Justification::centredLeft);
        if (f % 2 == 0) { g.setColour (K::brand2.withAlpha (0.025f)); g.fillRect (gr.getX(), y, gr.getWidth(), lh); }
        int s = 0;
        while (s < S)
        {
            if (! ed.proc.seq[f][s].load()) { ++s; continue; }
            int e = s; while (e < S && ed.proc.seq[f][e].load()) ++e;
            auto r = juce::Rectangle<float> (gr.getX() + s * cw + 1, y + 3, (e - s) * cw - 2, lh - 6);
            const bool playing = on && cur >= s && cur < e;
            if (playing) { g.setColour (c.withAlpha (0.45f)); g.fillRoundedRectangle (r.expanded (3), 6); }
            else { g.setColour (c.withAlpha (0.15f)); g.fillRoundedRectangle (r.expanded (2), 5); }
            juce::ColourGradient cg (playing ? c.brighter (0.5f) : c.brighter (0.1f), 0, r.getY(), c.darker (0.35f), 0, r.getBottom(), false); g.setGradientFill (cg);
            g.fillRoundedRectangle (r, 4);
            g.setColour (juce::Colours::white.withAlpha (0.35f)); g.drawHorizontalLine ((int) r.getY() + 1, r.getX() + 3, r.getRight() - 3);
            if (r.getWidth() > 60) { g.setColour (juce::Colour (0xff0a0c10).withAlpha (0.85f)); g.setFont (tf (10.0f, 2)); g.drawText (kFx[f].name, r.reduced (7, 0).toNearestInt(), juce::Justification::centredLeft); }
            s = e;
        }
    }
    for (int s = 0; s <= S; ++s)
    {
        const float x = gr.getX() + s * cw;
        g.setColour (K::brand2.withAlpha (s % 16 == 0 ? 0.22f : s % 4 == 0 ? 0.09f : 0.03f)); g.drawVerticalLine ((int) x, gr.getY(), gr.getBottom());
        if (s % 16 == 0 && s < S) { g.setColour (K::brand2.withAlpha (0.7f)); g.setFont (tf (9.0f, 2, true)); g.drawText ("BAR " + juce::String (s / 16 + 1), juce::Rectangle<float> (x + 4, gr.getY() - 10, 60, 10), juce::Justification::centredLeft); }
    }
    if (on && cur >= 0 && cur < S) { g.setColour (K::amber.withAlpha (0.18f)); g.fillRect (gr.getX() + cur * cw, gr.getY(), cw, gr.getHeight()); g.setColour (K::amber); g.drawVerticalLine ((int) (gr.getX() + cur * cw), gr.getY(), gr.getBottom()); }
    scan (g, b.reduced (2));
}
void Timeline::mouseDown (const juce::MouseEvent& e)
{
    int l, s; if (! cellAt (e.position, l, s)) return;
    const int span = ed.res == 0 ? 1 : ed.res == 1 ? 2 : 4; s = s / span * span;
    paintLane = l; paintVal = ! ed.proc.seq[l][s].load();
    for (int k = 0; k < span; ++k) ed.proc.seq[l][s + k] = paintVal;
    ed.select (l); repaint();
}
void Timeline::mouseDrag (const juce::MouseEvent& e)
{
    int l, s; if (paintLane < 0 || ! cellAt (e.position, l, s)) return;
    const int span = ed.res == 0 ? 1 : ed.res == 1 ? 2 : 4; s = s / span * span;
    for (int k = 0; k < span && s + k < kMaxSteps; ++k) ed.proc.seq[paintLane][s + k] = paintVal;
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
    juce::ColourGradient bg (juce::Colour (0xff181a20), 0, 0, K::bg, 0, Hf, false); g.setGradientFill (bg); g.fillAll();
    juce::Random rr (7); for (int y = 0; y < H; y += 2) { g.setColour (juce::Colours::white.withAlpha (rr.nextFloat() * 0.014f)); g.drawHorizontalLine (y, 0, Wf); }
    juce::ColourGradient amb (K::brand1.withAlpha (0.07f), Wf * 0.85f, 200, juce::Colours::transparentBlack, Wf * 0.85f + 500, 200, true); g.setGradientFill (amb); g.fillRect (0.0f, 0.0f, Wf, Hf);
    juce::ColourGradient amb2 (K::brand2.withAlpha (0.05f), 300, 700, juce::Colours::transparentBlack, 900, 700, true); g.setGradientFill (amb2); g.fillRect (0.0f, 0.0f, Wf, Hf);
    // intestazione
    juce::ColourGradient hg (juce::Colour (0xff23262e), 0, 0, juce::Colour (0xff121419), 0, 58, false); g.setGradientFill (hg); g.fillRect (0.0f, 0.0f, Wf, 58.0f);
    g.setColour (K::brand1.withAlpha (0.8f)); g.fillRect (0.0f, 56.0f, Wf * 0.45f, 2.0f); g.setColour (K::brand2.withAlpha (0.8f)); g.fillRect (Wf * 0.45f, 56.0f, Wf * 0.55f, 2.0f);
    glow (g, "JACKPOT", { 18, 8, 140, 34 }, K::ink, tf (25.0f, 2));
    glow (g, "TAPE", { 148, 8, 120, 34 }, K::amber, tf (25.0f, 2));
    g.setFont (tf (9.0f, 2, true)); g.setColour (K::dim); g.drawText ("JT-9  TAPE MOTION", juce::Rectangle<float> (18, 38, 200, 14), juce::Justification::centredLeft);
    for (int i = 0; i < 8; ++i) { auto r = juce::Rectangle<float> (268 + i * 11.0f, 16, 5, 26); g.setColour (juce::Colours::black.withAlpha (0.8f)); g.fillRoundedRectangle (r, 2.5f); }
    // colonne
    panel (g, ed.sidePanel.toFloat().reduced (6, 8), 12, K::brand2);
    panel (g, ed.rightPanel.toFloat().reduced (6, 8), 12, K::amber);
    auto right = ed.rightPanel;
    glow (g, "MACRO", juce::Rectangle<float> ((float) right.getX() + 18, 72, 120, 16), K::ink, tf (11.0f, 2));
    g.setColour (K::amber); g.fillRoundedRectangle ((float) right.getX() + 18, 90, (float) right.getWidth() - 36, 2.0f, 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.08f)); g.drawHorizontalLine (322, (float) right.getX() + 18, (float) right.getRight() - 18); g.drawHorizontalLine (506, (float) right.getX() + 18, (float) right.getRight() - 18);
    caps (g, "Nastro", { right.getX() + 18, 332, 120, 16 }, K::amber);
    caps (g, "Uscita", { right.getX() + 18, 516, 120, 16 }, K::amber);
    // riga sotto il display
    caps (g, "Pad", { 226, 316, 40, 28 }, K::dim, juce::Justification::centredLeft, 10.0f);
    caps (g, "Quantizza", { 404, 316, 80, 28 }, K::dim, juce::Justification::centredLeft, 10.0f);
    g.setColour (K::faint); g.setFont (tf (10.5f)); g.drawText (juce::String::fromUTF8 ("tieni premuto \xc2\xb7 tasti 1-9 \xc2\xb7 pi\xc3\xb9 pad insieme si sommano"), juce::Rectangle<int> (640, 316, 366, 28), juce::Justification::centredRight);
    // coreografia
    panel (g, juce::Rectangle<float> (8, 598, Wf - 16, Hf - 32 - 604), 12, K::brand1);
    glow (g, "COREOGRAFIA", juce::Rectangle<float> (20, 606, 110, 30), K::ink, tf (10.5f, 2));
    caps (g, "Battute", { 172, 606, 60, 30 }, K::dim, juce::Justification::centredLeft, 9.5f);
    caps (g, "Griglia", { 432, 606, 50, 30 }, K::dim, juce::Justification::centredLeft, 9.5f);
    caps (g, "Stile", { 652, 606, 46, 30 }, K::brand1, juce::Justification::centredLeft, 9.5f);
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
    addAndMakeVisible (root);
    sidePanel = { 0, 59, 210, 537 }; rightPanel = { 1020, 59, 220, 537 };
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
Editor::~Editor() {}

int Editor::steps() const { static const int b[] = { 1, 2, 4, 8 }; return b[juce::jlimit (0, 3, (int) proc.value ("bars"))] * 16; }
void Editor::select (int f) { sel = f; fxEd.rebuild(); list.repaint(); }

void Editor::resized()
{
    const float s = getWidth() / (float) W;
    root.setBounds (0, 0, W, H); root.setTransform (juce::AffineTransform::scale (s));
    list.setBounds (20, 74, 172, 400);
    disp.setBounds (224, 72, 782, 232);
    modeSeg->setBounds (256, 316, modeSeg->preferredWidth(), 28);
    qSeg->setBounds (484, 316, qSeg->preferredWidth(), 28);
    const int pw = (782 - 8 * 6) / 9;
    for (int f = 0; f < kNumFx; ++f) pads[f]->setBounds (224 + f * (pw + 6), 354, pw, 60);
    fxEd.setBounds (224, 424, 782, 160);
    // destra
    const int rx = rightPanel.getX();
    tapeKnobs[0]->setBounds (rx + 55, 90, 110, 128);
    tapeKnobs[1]->setBounds (rx + 30, 228, 66, 84); tapeKnobs[2]->setBounds (rx + 124, 228, 66, 84);
    for (int i = 0; i < 6; ++i) tapeKnobs[3 + i]->setBounds (rx + 14 + (i % 3) * 66, 352 + (i / 3) * 76, 60, 76);
    tapeKnobs[9]->setBounds (rx + 14, 536, 54, 54);
    tapeKnobs[9]->setVisible (proc.value ("sync") < 0.5f);
    meter.setBounds (rx + (tapeKnobs[9]->isVisible() ? 80 : 18), 544, tapeKnobs[9]->isVisible() ? 124 : 186, 30);
    // intestazione
    pPrev->setBounds (420, 11, 34, 36); pName->setBounds (454, 11, 332, 36); pNext->setBounds (786, 11, 34, 36);
    bpmPill->setBounds (1060, 11, 164, 36);
    // coreografia
    seqTog->setBounds (124, 611, 38, 20);
    barsSeg->setBounds (232, 607, barsSeg->preferredWidth(), 28);
    resSeg->setBounds (484, 607, resSeg->preferredWidth(), 28);
    styleBtn->setBounds (700, 607, 120, 28);
    genSeg->setBounds (828, 607, genSeg->preferredWidth(), 28);
    genBtn->setBounds (828 + genSeg->preferredWidth() + 8, 607, 100, 28);
    clearBtn->setBounds (W - 104, 607, 84, 28);
    tl.setBounds (18, 644, W - 36, H - 32 - 644 - 14);
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
