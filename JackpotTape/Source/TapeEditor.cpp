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
        default: return pct (v);
    }
}
static void panel (juce::Graphics& g, juce::Rectangle<float> r, float rad, juce::Colour top = K::p1, juce::Colour bot = juce::Colour (0xff11141a))
{
    juce::ColourGradient cg (top, r.getX(), r.getY(), bot, r.getX(), r.getBottom(), false);
    g.setGradientFill (cg); g.fillRoundedRectangle (r, rad);
    g.setColour (K::hair); g.drawRoundedRectangle (r.reduced (0.5f), rad, 1.0f);
}
static void caps (juce::Graphics& g, const juce::String& t, juce::Rectangle<int> r, juce::Colour c = K::faint, juce::Justification j = juce::Justification::centredLeft, float size = 9.5f)
{
    g.setColour (c); g.setFont (tf (size, 2)); g.drawText (t.toUpperCase(), r, j);
}

// ======================= Knob =======================
void Knob::paint (juce::Graphics& g)
{
    const float size = (float) juce::jmin (getWidth(), getHeight() - 18);
    const float R = size * 0.5f - 5, r = R - 7, cx = getWidth() * 0.5f, cy = size * 0.5f;
    // ombra e corpo
    g.setColour (juce::Colours::black.withAlpha (0.5f)); g.fillEllipse (cx - r, cy - r + 5, r * 2, r * 2);
    juce::ColourGradient body (juce::Colour (0xff2b3140), cx - r * 0.4f, cy - r * 0.6f, juce::Colour (0xff14171f), cx + r * 0.6f, cy + r, true);
    g.setGradientFill (body); g.fillEllipse (cx - r, cy - r, r * 2, r * 2);
    g.setColour (juce::Colours::white.withAlpha (0.07f)); g.drawEllipse (cx - r + 0.5f, cy - r + 0.5f, r * 2 - 1, r * 2 - 1, 1.0f);
    const float a0 = -2.356f, a1 = 2.356f, n = param->getValue(), a = a0 + n * (a1 - a0);
    juce::Path tr; tr.addCentredArc (cx, cy, R, R, 0, a0, a1, true);
    g.setColour (juce::Colour (0xff232836)); g.strokePath (tr, juce::PathStrokeType (3.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    if (n > 0.004f)
    {
        juce::Path va; va.addCentredArc (cx, cy, R, R, 0, a0, a, true);
        g.setColour (col.withAlpha (0.25f)); g.strokePath (va, juce::PathStrokeType (7.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (col); g.strokePath (va, juce::PathStrokeType (3.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    g.setColour (juce::Colours::white); g.fillEllipse (cx + std::sin (a) * R - 2.6f, cy - std::cos (a) * R - 2.6f, 5.2f, 5.2f);
    const float v = param->convertFrom0to1 (n);
    g.setColour (K::ink); g.setFont (tf (size > 90 ? 13.0f : 10.5f, 0, true));
    g.drawText (fmtFn ? fmtFn (v) : param->getCurrentValueAsText(), juce::Rectangle<float> (cx - r, cy - 8, r * 2, 16), juce::Justification::centred);
    caps (g, lab, juce::Rectangle<int> (0, (int) size + 2, getWidth(), 14), K::dim, juce::Justification::centred);
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
    g.setColour (K::p1); g.fillRoundedRectangle (b, 9); g.setColour (K::hair); g.drawRoundedRectangle (b.reduced (0.5f), 9, 1);
    const int cur = getter(); float x = 3;
    for (int i = 0; i < it.size(); ++i)
    {
        const float w = juce::GlyphArrangement::getStringWidth (tf (11.0f, 1), it[i]) + 22;
        auto r = juce::Rectangle<float> (x, 3, w, b.getHeight() - 6);
        if (i == cur)
        {
            g.setColour (juce::Colours::black.withAlpha (0.4f)); g.fillRoundedRectangle (r.translated (0, 2), 7);
            g.setColour (K::p3); g.fillRoundedRectangle (r, 7); g.setColour (K::hair2); g.drawRoundedRectangle (r.reduced (0.5f), 7, 1);
        }
        g.setColour (i == cur ? K::ink : K::dim); g.setFont (tf (11.0f, 1)); g.drawText (it[i], r.toNearestInt(), juce::Justification::centred);
        x += w;
    }
}
void Seg::mouseDown (const juce::MouseEvent& e)
{
    float x = 3;
    for (int i = 0; i < it.size(); ++i) { const float w = juce::GlyphArrangement::getStringWidth (tf (11.0f, 1), it[i]) + 22; if (e.position.x >= x && e.position.x < x + w) { setter (i); repaint(); return; } x += w; }
}
void Toggle::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat(); const bool on = get();
    if (on) { juce::ColourGradient cg (K::brand1, b.getX(), b.getY(), K::brand2, b.getRight(), b.getBottom(), false); g.setGradientFill (cg); }
    else g.setColour (K::p3);
    g.fillRoundedRectangle (b, b.getHeight() / 2); g.setColour (K::hair2); g.drawRoundedRectangle (b.reduced (0.5f), b.getHeight() / 2, 1);
    const float d = b.getHeight() - 6; g.setColour (on ? juce::Colours::white : juce::Colour (0xff9aa1b2));
    g.fillEllipse (on ? b.getRight() - d - 3 : 3.0f, 3, d, d);
}
void TextBtn::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    if (accent) { juce::ColourGradient cg (K::brand1, b.getX(), b.getY(), K::brand2, b.getRight(), b.getBottom(), false); g.setGradientFill (cg); g.fillRoundedRectangle (b, 9); }
    else if (! round) { g.setColour (hover ? K::p2 : K::p1); g.fillRoundedRectangle (b, 9); g.setColour (hover ? K::hair2 : K::hair); g.drawRoundedRectangle (b.reduced (0.5f), 9, 1); }
    g.setColour (accent ? juce::Colours::white : (hover || round ? K::ink : K::dim)); g.setFont (tf (round ? 16.0f : 11.0f, 1)); g.drawText (text, getLocalBounds(), juce::Justification::centred);
}

// ======================= Lista effetti =======================
void FxList::paint (juce::Graphics& g)
{
    const int am = ed.proc.activeMask.load();
    caps (g, "Effetti", { 6, 0, 100, 20 });
    g.setFont (tf (10.0f)); g.setColour (K::dim); g.drawText ("clic per modificare", juce::Rectangle<int> (0, 0, getWidth() - 6, 20), juce::Justification::centredRight);
    for (int f = 0; f < kNumFx; ++f)
    {
        auto r = juce::Rectangle<float> (0, 30.0f + f * 40.0f, (float) getWidth(), 38);
        const bool s = f == ed.sel, on = (am >> f) & 1;
        if (s) { g.setColour (K::p2); g.fillRoundedRectangle (r, 10); g.setColour (K::hair); g.drawRoundedRectangle (r.reduced (0.5f), 10, 1); g.setColour (fxCol (f)); g.fillRoundedRectangle (r.getX() - 10 + 10, r.getY() + 9, 3, 20, 1.5f); }
        const float dx = r.getX() + 16, dy = r.getCentreY();
        if (on) { g.setColour (fxCol (f).withAlpha (0.18f)); g.fillEllipse (dx - 8, dy - 8, 16, 16); g.setColour (fxCol (f).withAlpha (0.5f)); g.fillEllipse (dx - 6, dy - 6, 12, 12); }
        g.setColour (on ? fxCol (f) : fxCol (f).interpolatedWith (juce::Colour (0xff1a1d24), 0.65f)); g.fillEllipse (dx - 4, dy - 4, 8, 8);
        g.setColour (s || on ? K::ink : K::dim); g.setFont (tf (12.5f, 1)); g.drawText (kFx[f].name, r.withTrimmedLeft (30).toNearestInt(), juce::Justification::centredLeft);
        auto kk = juce::Rectangle<float> (r.getRight() - 28, dy - 9, 18, 18);
        g.setColour (K::hair2); g.drawRoundedRectangle (kk, 5, 1); g.setColour (K::faint); g.setFont (tf (10.0f, 0, true)); g.drawText (juce::String::charToString (kFx[f].key), kk.toNearestInt(), juce::Justification::centred);
    }
}
void FxList::mouseDown (const juce::MouseEvent& e) { const int f = (int) ((e.position.y - 30) / 40); if (f >= 0 && f < kNumFx) ed.select (f); }

// ======================= Display =======================
void Display::paint (juce::Graphics& g)
{
    auto& P = ed.proc; auto b = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff0a0c10)); g.fillRoundedRectangle (b, 14);
    juce::ColourGradient glow (K::brand1.withAlpha (0.10f), b.getX(), b.getY(), juce::Colours::transparentBlack, b.getX(), b.getY() + b.getHeight() * 0.6f, false);
    g.setGradientFill (glow); g.fillRoundedRectangle (b, 14);
    const int steps = ed.steps(); const double beats = steps / 4.0;
    auto hdr = b.removeFromTop (34).reduced (14, 0);
    // intestazione
    const double beat = P.uiBeat.load(); const double pb = std::fmod (beat, beats);
    juce::String pos; pos << "BAR " << ((int) (pb / 4) + 1) << "." << ((int) std::fmod (pb, 4.0) + 1);
    g.setFont (tf (12.0f, 2, true)); g.setColour (K::ink); g.drawText (pos, hdr.toNearestInt(), juce::Justification::centredLeft);
    const float spd = P.uiSpeed.load();
    g.setFont (tf (11.0f, 0, true)); g.setColour (K::dim);
    g.drawText ("VELOCITA " + juce::String (spd, 2) + juce::String::fromUTF8 ("\xc3\x97") + "   " + juce::String (P.uiBpm.load(), 1) + " BPM" + (P.uiHost.load() ? "  HOST" : ""), hdr.toNearestInt(), juce::Justification::centredRight);
    // bobine
    reelA += spd * 0.12f;
    for (int k = 0; k < 2; ++k)
    {
        const float cx = hdr.getCentreX() + (k ? 22.0f : -22.0f), cy = hdr.getCentreY(), r = 11;
        g.setColour (juce::Colours::white.withAlpha (0.22f)); g.drawEllipse (cx - r, cy - r, r * 2, r * 2, 2.0f);
        for (int s = 0; s < 3; ++s) { const float a = reelA + s * 2.094f; g.drawLine (cx + std::cos (a) * 3, cy + std::sin (a) * 3, cx + std::cos (a) * (r - 2), cy + std::sin (a) * (r - 2), 2.0f); }
    }
    auto wave = b.reduced (14, 6).withTrimmedBottom (56);
    auto curve = b.reduced (14, 0).removeFromBottom (54).withTrimmedBottom (8);
    // griglia battute
    for (int q = 0; q <= (int) beats; ++q)
    {
        const float x = wave.getX() + wave.getWidth() * (float) (q / beats);
        g.setColour (juce::Colours::white.withAlpha (q % 4 == 0 ? 0.10f : 0.035f)); g.drawVerticalLine ((int) x, wave.getY(), curve.getBottom());
        if (q % 4 == 0 && q < beats) { g.setColour (K::faint); g.setFont (tf (9.5f, 1, true)); g.drawText (juce::String (q / 4 + 1), juce::Rectangle<float> (x + 4, wave.getY(), 30, 12), juce::Justification::centredLeft); }
    }
    // onda: secco sotto, effetto a colori sopra
    const int W = (int) wave.getWidth(); const float cy = wave.getCentreY(), hh = wave.getHeight() * 0.46f;
    for (int x = 0; x < W; ++x)
    {
        const int c = juce::jlimit (0, kCols - 1, x * kCols / W);
        const float d = juce::jmin (1.0f, P.dispDry[c].load() * 1.2f), w = juce::jmin (1.0f, P.dispWet[c].load() * 1.2f); const int m = P.dispMask[c].load();
        g.setColour (juce::Colours::white.withAlpha (0.08f)); g.drawVerticalLine ((int) wave.getX() + x, cy - d * hh, cy + d * hh + 1);
        juce::Colour wc = juce::Colour (0xffcfd6e6);
        for (int f = 0; f < kNumFx; ++f) if ((m >> f) & 1) { wc = fxCol (f); break; }
        g.setColour (wc.withAlpha (m ? 0.95f : 0.55f)); g.drawVerticalLine ((int) wave.getX() + x, cy - w * hh, cy + w * hh + 1);
    }
    // curva di velocità
    g.setColour (juce::Colours::white.withAlpha (0.05f)); g.drawHorizontalLine ((int) (curve.getY() + curve.getHeight() * 0.5f), curve.getX(), curve.getRight());
    juce::Path sp; for (int x = 0; x < W; x += 2)
    {
        const int c = juce::jlimit (0, kCols - 1, x * kCols / W); const float v = juce::jlimit (-2.0f, 2.0f, P.dispSpd[c].load());
        const float y = curve.getCentreY() - v * curve.getHeight() * 0.24f;
        if (x == 0) sp.startNewSubPath (curve.getX() + x, y); else sp.lineTo (curve.getX() + x, y);
    }
    juce::ColourGradient cg (K::brand1, curve.getX(), 0, K::brand2, curve.getRight(), 0, false);
    g.setGradientFill (cg); g.strokePath (sp, juce::PathStrokeType (1.8f));
    caps (g, "velocita", curve.toNearestInt().removeFromTop (12), K::faint, juce::Justification::centredLeft, 8.5f);
    // testina
    const float px = wave.getX() + wave.getWidth() * (float) (pb / beats);
    g.setColour (juce::Colours::white.withAlpha (0.85f)); g.drawLine (px, wave.getY(), px, curve.getBottom(), 1.4f);
    g.setColour (juce::Colours::white.withAlpha (0.12f)); g.fillRect (px - 6, wave.getY(), 6.0f, curve.getBottom() - wave.getY());
    g.setColour (K::hair); g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (0.5f), 14, 1);
}

// ======================= Pad =======================
void PadBtn::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat(); const bool on = down || ((ed.proc.activeMask.load() >> fx) & 1); const auto c = fxCol (fx);
    if (on) b = b.reduced (b.getWidth() * 0.015f);
    if (on)
    {
        g.setColour (c.withAlpha (0.25f)); g.fillRoundedRectangle (b.translated (0, 6).reduced (4, 0), 11);
        juce::ColourGradient cg (c.interpolatedWith (juce::Colour (0xff141821), 0.68f), 0, b.getY(), c.interpolatedWith (juce::Colour (0xff11141b), 0.88f), 0, b.getBottom(), false);
        g.setGradientFill (cg);
    }
    else g.setColour (K::p1);
    g.fillRoundedRectangle (b, 11);
    g.setColour (on ? c.withAlpha (0.7f) : K::hair); g.drawRoundedRectangle (b.reduced (0.5f), 11, 1);
    g.saveState(); juce::Path clip; clip.addRoundedRectangle (b, 11); g.reduceClipRegion (clip);
    g.setColour (c.withAlpha (on ? 1.0f : 0.55f)); g.fillRect (b.withHeight (2));
    g.restoreState();
    g.setColour (K::ink); g.setFont (tf (11.0f, 2)); g.drawText (kFx[fx].name, b.reduced (9, 8).toNearestInt(), juce::Justification::topLeft);
    const bool shot = ed.proc.value ("mode") > 0.5f;
    g.setColour (K::faint); g.setFont (tf (9.5f, 0, true)); g.drawText (juce::String::charToString (kFx[fx].key) + juce::String::fromUTF8 (" \xc2\xb7 ") + (shot ? "one-shot" : "hold"), b.reduced (9, 8).toNearestInt(), juce::Justification::bottomLeft);
}
void PadBtn::mouseDown (const juce::MouseEvent&) { down = true; ed.select (fx); ed.proc.uiPad[fx] = true; repaint(); }
void PadBtn::mouseUp (const juce::MouseEvent&) { down = false; ed.proc.uiPad[fx] = false; repaint(); }

// ======================= Editor effetto =======================
juce::Rectangle<int> FxEditor::curveArea() const { return { 16, 44, getWidth() * 52 / 100 - 16, getHeight() - 60 }; }
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
    auto a = getLocalBounds().withTrimmedLeft (getWidth() * 52 / 100 + 8).reduced (6, 14);
    const int n = knobs.size(); if (n == 0) return;
    const int s = n > 3 ? 70 : 84, w = a.getWidth() / n;
    for (int i = 0; i < n; ++i) knobs[i]->setBounds (a.getX() + i * w + (w - s) / 2, a.getCentreY() - (s + 18) / 2, s, s + 18);
}
void FxEditor::paint (juce::Graphics& g)
{
    panel (g, getLocalBounds().toFloat(), 14);
    const auto c = fxCol (ed.sel);
    g.setColour (c.withAlpha (0.3f)); g.fillEllipse (14, 14, 14, 14); g.setColour (c); g.fillEllipse (17, 17, 8, 8);
    g.setColour (K::ink); g.setFont (tf (17.0f, 2)); g.drawText (kFx[ed.sel].name, 34, 8, 200, 26, juce::Justification::centredLeft);
    const float nw = juce::GlyphArrangement::getStringWidth (tf (17.0f, 2), kFx[ed.sel].name);
    g.setColour (K::dim); g.setFont (tf (11.5f)); g.drawText (kFx[ed.sel].desc, (int) (44 + nw), 10, 360, 24, juce::Justification::centredLeft);
    // anteprima della curva di velocità dell'effetto
    auto ca = curveArea().toFloat();
    g.setColour (juce::Colour (0xff0a0c10)); g.fillRoundedRectangle (ca, 10); g.setColour (K::hair); g.drawRoundedRectangle (ca.reduced (0.5f), 10, 1);
    auto in = ca.reduced (12, 12);
    for (int k = 0; k <= 4; ++k) { g.setColour (juce::Colours::white.withAlpha (0.04f)); g.drawHorizontalLine ((int) (in.getY() + in.getHeight() * k / 4), in.getX(), in.getRight()); }
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
    g.setColour (juce::Colour (0xff0a0c10)); g.fillRoundedRectangle (b, 12); g.setColour (K::hair); g.drawRoundedRectangle (b.reduced (0.5f), 12, 1);
    auto gr = grid(); const int S = ed.steps(); const float cw = gr.getWidth() / S, lh = gr.getHeight() / kNumFx;
    const int cur = ed.proc.uiStep.load(); const bool on = ed.proc.value ("seqOn") > 0.5f;
    for (int f = 0; f < kNumFx; ++f)
    {
        const float y = gr.getY() + f * lh; const auto c = fxCol (f);
        g.setColour (c.withAlpha (0.9f)); g.fillRoundedRectangle (16, y + lh * 0.5f - 4, 8, 8, 4);
        g.setColour (K::ink.withAlpha (0.85f)); g.setFont (tf (11.5f, 1)); g.drawText (kFx[f].name, juce::Rectangle<float> (32, y, 130, lh), juce::Justification::centredLeft);
        if (f % 2 == 0) { g.setColour (juce::Colours::white.withAlpha (0.015f)); g.fillRect (gr.getX(), y, gr.getWidth(), lh); }
        int s = 0;
        while (s < S)
        {
            if (! ed.proc.seq[f][s].load()) { ++s; continue; }
            int e = s; while (e < S && ed.proc.seq[f][e].load()) ++e;
            auto r = juce::Rectangle<float> (gr.getX() + s * cw + 1, y + 3, (e - s) * cw - 2, lh - 6);
            const bool playing = on && cur >= s && cur < e;
            if (playing) { g.setColour (c.withAlpha (0.35f)); g.fillRoundedRectangle (r.expanded (2), 6); }
            juce::ColourGradient cg (c.brighter (0.15f), 0, r.getY(), c.darker (0.25f), 0, r.getBottom(), false); g.setGradientFill (cg);
            g.fillRoundedRectangle (r, 5);
            g.setColour (juce::Colours::white.withAlpha (0.35f)); g.drawHorizontalLine ((int) r.getY() + 1, r.getX() + 3, r.getRight() - 3);
            if (r.getWidth() > 60) { g.setColour (juce::Colour (0xff0a0c10).withAlpha (0.85f)); g.setFont (tf (10.0f, 2)); g.drawText (kFx[f].name, r.reduced (7, 0).toNearestInt(), juce::Justification::centredLeft); }
            s = e;
        }
    }
    for (int s = 0; s <= S; ++s)
    {
        const float x = gr.getX() + s * cw;
        g.setColour (juce::Colours::white.withAlpha (s % 16 == 0 ? 0.16f : s % 4 == 0 ? 0.07f : 0.025f)); g.drawVerticalLine ((int) x, gr.getY(), gr.getBottom());
        if (s % 16 == 0 && s < S) { g.setColour (K::dim); g.setFont (tf (9.5f, 1, true)); g.drawText ("BAR " + juce::String (s / 16 + 1), juce::Rectangle<float> (x + 4, gr.getY() - 10, 60, 10), juce::Justification::centredLeft); }
    }
    if (on && cur >= 0 && cur < S) { g.setColour (juce::Colours::white.withAlpha (0.08f)); g.fillRect (gr.getX() + cur * cw, gr.getY(), cw, gr.getHeight()); }
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
    for (int c = 0; c < 2; ++c)
    {
        auto r = juce::Rectangle<float> (b.getX(), b.getY() + c * (b.getHeight() / 2 + 2), b.getWidth(), b.getHeight() / 2 - 2);
        g.setColour (juce::Colour (0xff151922)); g.fillRoundedRectangle (r, 4); g.setColour (K::hair); g.drawRoundedRectangle (r, 4, 1);
        const float db = juce::Decibels::gainToDecibels (v[c], -60.0f), n = juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 63.0f);
        juce::ColourGradient cg (K::brand2, r.getX(), 0, juce::Colour (0xfff472b6), r.getRight(), 0, false); cg.addColour (0.7, K::brand1);
        g.setGradientFill (cg); g.fillRoundedRectangle (r.withWidth (r.getWidth() * n), 4);
    }
}

// ======================= Root (fondi e titoli) =======================
void Root::paint (juce::Graphics& g)
{
    const int W = Editor::W, H = Editor::H;
    g.fillAll (K::win);
    juce::ColourGradient g1 (K::brand1.withAlpha (0.10f), 180, -100, juce::Colours::transparentBlack, 900, 500, true); g.setGradientFill (g1); g.fillRect (0, 0, W, H);
    // intestazione
    juce::ColourGradient hg (juce::Colour (0xff141821), 0, 0, juce::Colour (0xff10131a), 0, 58, false); g.setGradientFill (hg); g.fillRect (0, 0, W, 58);
    g.setColour (K::hair); g.drawHorizontalLine (58, 0, (float) W);
    juce::ColourGradient lg (K::brand1, 16, 16, K::brand2, 42, 42, false); g.setGradientFill (lg); g.fillRoundedRectangle (16, 16, 26, 26, 8);
    g.setColour (K::win); g.fillEllipse (24, 24, 10, 10);
    g.setColour (K::ink); g.setFont (tf (13.0f, 2)); g.drawText ("JACKPOT", 52, 0, 80, 58, juce::Justification::centredLeft);
    g.setColour (K::dim); g.setFont (tf (13.0f)); g.drawText ("TAPE", 124, 0, 60, 58, juce::Justification::centredLeft);
    // colonne
    auto side = ed.sidePanel, right = ed.rightPanel;
    juce::ColourGradient sg (juce::Colour (0xff0f1218), 0, 58, juce::Colour (0xff0d0f14), 0, (float) side.getBottom(), false);
    g.setGradientFill (sg); g.fillRect (side); g.fillRect (right);
    g.setColour (K::hair); g.drawVerticalLine (side.getRight(), (float) side.getY(), (float) side.getBottom()); g.drawVerticalLine (right.getX(), (float) right.getY(), (float) right.getBottom());
    caps (g, "Macro", { right.getX() + 18, 70, 120, 16 });
    g.setColour (K::hair); g.drawHorizontalLine (322, (float) right.getX() + 14, (float) right.getRight() - 14); g.drawHorizontalLine (506, (float) right.getX() + 14, (float) right.getRight() - 14);
    caps (g, "Nastro", { right.getX() + 18, 332, 120, 16 });
    caps (g, "Uscita", { right.getX() + 18, 516, 120, 16 });
    // riga sotto il display
    caps (g, "Pad", { 226, 316, 40, 28 }, K::faint, juce::Justification::centredLeft, 10.0f);
    caps (g, "Quantizza", { 404, 316, 80, 28 }, K::faint, juce::Justification::centredLeft, 10.0f);
    g.setColour (K::faint); g.setFont (tf (11.0f)); g.drawText (juce::String::fromUTF8 ("tieni premuto \xc2\xb7 tasti 1-9 \xc2\xb7 pi\xc3\xb9 pad insieme si sommano"), juce::Rectangle<int> (640, 316, 366, 28), juce::Justification::centredRight);
    // coreografia
    juce::ColourGradient tg (juce::Colour (0xff0f1218), 0, 596, juce::Colour (0xff0c0e13), 0, (float) H - 32, false); g.setGradientFill (tg); g.fillRect (0, 596, W, H - 628);
    g.setColour (K::hair); g.drawHorizontalLine (596, 0, (float) W);
    caps (g, "Coreografia", { 16, 606, 110, 30 }, K::faint, juce::Justification::centredLeft, 10.0f);
    caps (g, "Lunghezza", { 186, 606, 80, 30 }, K::faint, juce::Justification::centredLeft, 10.0f);
    caps (g, "Griglia", { 498, 606, 60, 30 }, K::faint, juce::Justification::centredLeft, 10.0f);
    // piede
    g.setColour (K::hair); g.drawHorizontalLine (H - 32, 0, (float) W);
    g.setColour (K::faint); g.setFont (tf (10.0f));
    g.drawText (juce::String::fromUTF8 ("JACKPOT.WAV \xc2\xb7 TAPE MOTION \xc2\xb7 AU"), juce::Rectangle<int> (16, H - 32, 400, 32), juce::Justification::centredLeft);
    g.drawText (ed.proc.uiHost.load() ? "BPM sincronizzati con l'host" : (ed.proc.value ("sync") > 0.5f ? "in attesa del play dell'host" : "BPM manuali"), juce::Rectangle<int> (W - 416, H - 32, 400, 32), juce::Justification::centredRight);
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
    barsSeg = std::make_unique<Seg> (juce::StringArray { "1", "2", "4", "8 battute" }, choice ("bars"), setC ("bars"));
    resSeg = std::make_unique<Seg> (juce::StringArray { "1/16", "1/8", "1/4" }, [this] { return res; }, [this] (int i) { res = i; });
    seqTog = std::make_unique<Toggle> ([this] { return proc.value ("seqOn") > 0.5f; }, [this] { auto* pr = proc.apvts.getParameter ("seqOn"); pr->setValueNotifyingHost (pr->getValue() > 0.5f ? 0.0f : 1.0f); });
    clearBtn = std::make_unique<TextBtn> ("Svuota", [this] { for (auto& r : proc.seq) for (auto& s : r) s = false; tl.repaint(); });
    for (auto* c : std::initializer_list<juce::Component*> { modeSeg.get(), qSeg.get(), barsSeg.get(), resSeg.get(), seqTog.get(), clearBtn.get() }) root.addAndMakeVisible (c);
    // preset
    auto showName = [this] { const auto n = proc.getProgramName (proc.presetIndex); pName->text = n.isEmpty() ? juce::String ("Init") : n; pName->repaint(); };
    pPrev = std::make_unique<TextBtn> (juce::String::fromUTF8 ("\xe2\x80\xb9"), [this, showName] { proc.loadPreset ((proc.presetIndex - 1 + proc.getNumPrograms()) % proc.getNumPrograms()); showName(); fxEd.rebuild(); tl.repaint(); });
    pNext = std::make_unique<TextBtn> (juce::String::fromUTF8 ("\xe2\x80\xba"), [this, showName] { proc.loadPreset ((proc.presetIndex + 1) % proc.getNumPrograms()); showName(); fxEd.rebuild(); tl.repaint(); });
    pPrev->round = pNext->round = true;
    pName = std::make_unique<TextBtn> ("", [this, showName]
    {
        juce::PopupMenu m;
        for (int i = 0; i < proc.getNumPrograms(); ++i) m.addItem (i + 1, proc.getProgramName (i), true, i == proc.presetIndex);
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
    list.setBounds (14, 72, 182, 400);
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
    seqTog->setBounds (128, 611, 36, 20);
    barsSeg->setBounds (264, 607, barsSeg->preferredWidth(), 28);
    resSeg->setBounds (556, 607, resSeg->preferredWidth(), 28);
    clearBtn->setBounds (W - 90, 607, 74, 28);
    tl.setBounds (14, 644, W - 28, H - 32 - 644 - 10);
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
