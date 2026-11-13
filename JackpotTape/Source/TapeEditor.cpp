#include "TapeEditor.h"

namespace jt
{
juce::Font tf (float size, int weight, bool mono)
{
    return mono ? jlook::mono (size) : jlook::ui (size, weight == 0 ? 0 : weight == 1 ? 1 : 2);
}
static inline float shapeFn (float x, float c) { x = juce::jlimit (0.0f, 1.0f, x); return c >= 0 ? 1.0f - std::pow (1.0f - x, 1.0f + 3.0f * c) : std::pow (x, 1.0f - 3.0f * c); }
static constexpr float kRowH = 19.0f;
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
        case 7: return juce::String (v, std::abs (v - std::round (v)) < 0.01f ? 0 : 2) + juce::String::fromUTF8 ("\xc3\x97 lento");
        case 8: return v < 0.25f ? juce::String ("SATURA") : v < 0.7f ? juce::String ("DISTORCE") : juce::String ("FUZZ");
        case 9: return v >= 15.9f ? juce::String ("OFF") : juce::String (juce::roundToInt (v)) + " bit";
        case 11: return v >= 0 ? juce::String ("PASSA BASSO") : juce::String ("PASSA ALTO");
        case 12: return std::abs (v) < 0.05f ? juce::String ("0 Hz") : juce::String (v, std::abs (v) < 10 ? 1 : 0) + " Hz";
        case 13: return v < 0.3f ? juce::String ("LENTO") : v > 0.7f ? juce::String ("VELOCE") : juce::String ("IN MEZZO");
        case 14: { const float hz = 150.0f * std::pow (40.0f, v); return hz < 1000 ? juce::String (juce::roundToInt (hz)) + " Hz" : juce::String (hz / 1000.0f, 1) + " kHz"; }
        case 15: return v < 0.34f ? juce::String ("CRUNCH") : v < 0.67f ? juce::String ("DISTORSIONE") : juce::String ("FUZZ");
        case 10: return juce::roundToInt (v) <= 1 ? juce::String ("OFF") : juce::String::fromUTF8 ("\xc3\xb7") + juce::String (juce::roundToInt (v));
        default: return pct (v);
    }
}
static void glow (juce::Graphics& g, const juce::String& t, juce::Rectangle<float> r, juce::Colour c, juce::Font f, juce::Justification j = juce::Justification::centredLeft)
{
    // testo pulito (prima 5 passate per l'alone, a ogni fotogramma)
    g.setFont (f); g.setColour (c); g.drawFittedText (t, r.toNearestInt(), j, 1, 0.8f);
}
// pannello in metallo con ombra, smusso e striscia LED opzionale
static void panel (juce::Graphics& g, juce::Rectangle<float> r, float rad, juce::Colour led = juce::Colours::transparentBlack)
{
    neo::setTheme (3); neo::plate (g, r, led.isTransparent() ? K::amber : led, juce::jmax (10.0f, rad));
}
// schermo LCD incassato
static void inset (juce::Graphics& g, juce::Rectangle<float> r, float rad)
{
    // incavo semplice (colore pieno, niente gradienti: si ridisegna spesso)
    auto p = neo::chamfer (r, juce::jmin (7.0f, rad + 1.0f), 2.0f);
    g.setColour (K::lcd2.withAlpha (0.85f)); g.fillPath (p);
    g.setColour (juce::Colours::white.withAlpha (0.07f)); g.strokePath (p, juce::PathStrokeType (1.0f));
}
static void lcdGrid (juce::Graphics& g, juce::Rectangle<float> r, float step = 10)
{
    g.setColour (K::brand2.withAlpha (0.035f));
    for (float x = r.getX(); x < r.getRight(); x += step) g.drawVerticalLine ((int) x, r.getY(), r.getBottom());
    for (float y = r.getY(); y < r.getBottom(); y += step) g.drawHorizontalLine ((int) y, r.getX(), r.getRight());
}
static void scan (juce::Graphics& g, juce::Rectangle<float> r)
{
    juce::ignoreUnused (g, r);   // le righe di scansione ora sono nel fondo in cache (neo::crt)
}
static void caps (juce::Graphics& g, const juce::String& t, juce::Rectangle<int> r, juce::Colour c = K::dim, juce::Justification j = juce::Justification::centredLeft, float size = 9.5f)
{
    g.setColour (c); g.setFont (neo::cap (size * 0.9f)); g.drawText (t.toUpperCase(), r, j);
}

// ======================= Knob: manopola 3D con anello LED =======================
void Knob::paint (juce::Graphics& g)
{
    const bool big = getWidth() > 90;
    const float lh = big ? 30.0f : 27.0f;
    const float size = juce::jmin ((float) getWidth(), getHeight() - lh);
    const float v = param->getValue();
    const bool bip = param->convertFrom0to1 (0.0f) < 0 && param->convertFrom0to1 (1.0f) > 0 && std::abs (param->convertFrom0to1 (0.0f) + param->convertFrom0to1 (1.0f)) < 0.01f;
    shown = v; neo::setTheme (3);
    // manopola CYBER: anello di LED a segmenti, disco nero, lancetta del colore dell'effetto
    const float cx = getWidth() * 0.5f, cy = size * 0.5f, R = size * 0.5f - 3.0f, kr = R * (big ? 0.72f : 0.7f);
    const float av = -2.356f + 4.712f * v;
    neo::ledRing (g, cx, cy, R, v, bip, hover || drag ? col.brighter (0.2f) : col, big ? 3.2f : 2.4f, big ? 31 : 23);
    g.setColour (juce::Colours::black.withAlpha (0.45f)); g.fillEllipse (cx - kr, cy - kr + 2, kr * 2, kr * 2);
    juce::ColourGradient bgc (juce::Colour (0xff2a2e3a), cx, cy - kr, juce::Colour (0xff111319), cx, cy + kr, false); g.setGradientFill (bgc); g.fillEllipse (cx - kr, cy - kr, kr * 2, kr * 2);
    g.setColour (juce::Colours::white.withAlpha (0.08f)); g.drawEllipse (cx - kr + 0.5f, cy - kr + 0.5f, kr * 2 - 1, kr * 2 - 1, 1.0f);
    { const float r0 = kr * 0.3f, r1 = kr * 0.88f, sn = std::sin (av), cs = std::cos (av);
      juce::Path pl; pl.startNewSubPath (cx + sn * r0, cy - cs * r0); pl.lineTo (cx + sn * r1, cy - cs * r1);
      g.setColour (col); g.strokePath (pl, juce::PathStrokeType (big ? 2.6f : 2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded)); }
    g.setColour (hover ? K::ink : K::dim); g.setFont (neo::cap (big ? 9.5f : 8.0f)); g.drawFittedText (lab.toUpperCase(), juce::Rectangle<int> (0, (int) size + 2, getWidth(), 12), juce::Justification::centred, 1, 0.7f);
    g.setFont (neo::num (big ? 11.0f : 9.5f)); g.setColour (drag ? col : K::ink.withAlpha (0.9f));
    g.drawFittedText (fmtFn ? fmtFn (param->convertFrom0to1 (v)) : param->getCurrentValueAsText(), juce::Rectangle<int> (0, (int) size + 14, getWidth(), 13), juce::Justification::centred, 1, 0.7f);
}
void Knob::setNorm (float v)
{
    v = juce::jlimit (0.0f, 1.0f, v);
    if (auto* c = dynamic_cast<juce::AudioParameterChoice*> (param)) { const int k = c->choices.size(); v = std::round (v * (k - 1)) / (k - 1); }
    param->setValueNotifyingHost (v); if (onUserChange) onUserChange (param->convertFrom0to1 (v)); repaint();
}
void Knob::mouseDown (const juce::MouseEvent& e) { if (onStart) onStart(); startV = param->getValue(); param->beginChangeGesture(); drag = true; if (e.mods.isAltDown() || e.mods.isCommandDown()) { setNorm (param->getDefaultValue()); drag = false; } }
void Knob::mouseUp (const juce::MouseEvent&) { param->endChangeGesture(); drag = false; repaint(); }
void Knob::mouseDrag (const juce::MouseEvent& e) { if (! drag) return; setNorm (startV - e.getDistanceFromDragStartY() / (e.mods.isShiftDown() ? 700.0f : 160.0f)); }
void Knob::mouseDoubleClick (const juce::MouseEvent&) { if (onStart) onStart(); setNorm (param->getDefaultValue()); }
void Knob::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w)
{
    float step = 1.0f / 60.0f;
    if (auto* c = dynamic_cast<juce::AudioParameterChoice*> (param)) step = 1.0f / (c->choices.size() - 1);
    if (onStart) onStart();
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
            neo::setTheme (3); neo::key (g, r, true, K::amber);
            g.setColour (K::ink); g.setFont (neo::cap (9.0f)); g.drawFittedText (it[i].toUpperCase(), r.toNearestInt().withTrimmedLeft (4), juce::Justification::centred, 1, 0.7f);
        }
        else { g.setColour (K::dim); g.setFont (neo::cap (9.0f)); g.drawFittedText (it[i].toUpperCase(), r.toNearestInt(), juce::Justification::centred, 1, 0.7f); }
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
    inset (g, b, 4);
    const float d = b.getHeight() - 6, x = on ? b.getRight() - d - 3 : 3.0f;
    g.setColour (on ? K::amber : juce::Colour (0xff3a3f4c)); g.fillRect (juce::Rectangle<float> (x, 3, d, d));
    if (on) { g.setColour (K::amber.withAlpha (0.25f)); g.fillRect (juce::Rectangle<float> (3, 3, b.getWidth() - 6, d)); }
}
void TextBtn::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (1);
    if (round)
    {
        g.setColour (hover ? K::ink : K::amber); g.setFont (tf (20.0f, 2)); g.drawText (text, b, juce::Justification::centred);
        return;
    }
    neo::setTheme (3); neo::key (g, b.reduced (2, 1), accent, K::brand1, hover);
    g.setColour (accent || hover ? K::ink : K::ink.withAlpha (0.8f)); g.setFont (neo::cap (9.5f)); g.drawFittedText (text.toUpperCase(), b.reduced (accent ? 10 : 6, 0).toNearestInt(), juce::Justification::centred, 1, 0.7f);
}

// ======================= Lista effetti =======================
void FxList::paint (juce::Graphics& g)
{
    const int am = ed.proc.activeMask.load();
    caps (g, "Effetti", { 6, 0, 100, 20 });
    g.setFont (tf (9.0f, 0, true)); g.setColour (K::faint); g.drawText ("clic = modifica", juce::Rectangle<int> (0, 0, getWidth() - 6, 20), juce::Justification::centredRight);
    for (int f = 0; f < kNumFx; ++f)
    {
        auto r = juce::Rectangle<float> (0, 24.0f + f * kRowH, (float) getWidth(), kRowH - 2);
        const bool s = f == ed.sel, on = (am >> f) & 1, en = ed.proc.fxEnabled (f); const auto c = en ? fxCol (f) : K::faint;
        g.setColour (juce::Colours::white.withAlpha (0.03f)); g.fillRect (r);
        if (s) { g.setColour (c.withAlpha (0.12f)); g.fillRect (r); g.setColour (c); g.fillRect (r.getX(), r.getY(), 2.0f, r.getHeight()); }
        const float dx = r.getX() + 16, dy = r.getCentreY();
        if (on) { g.setColour (c.withAlpha (0.3f)); g.fillRect (dx - 6, dy - 6, 12.0f, 12.0f); }
        g.setColour (on ? c : c.withAlpha (0.25f)); g.fillRect (dx - 4, dy - 4, 8.0f, 8.0f);
        if (s || on) glow (g, juce::String (kFx[f].name).toUpperCase(), r.withTrimmedLeft (30), on ? c : K::ink, tf (10.0f, 2), juce::Justification::centredLeft);
        else { g.setColour (en ? K::dim : K::faint); g.setFont (tf (10.0f, 2)); g.drawText (juce::String (kFx[f].name).toUpperCase() + (en ? "" : "  OFF"), r.withTrimmedLeft (30).toNearestInt(), juce::Justification::centredLeft); }
        auto kk = juce::Rectangle<float> (r.getRight() - 24, dy - 7, 14, 14);
        g.setColour (K::faint); g.drawRoundedRectangle (kk, 3, 1); g.setFont (tf (9.5f, 2, true)); g.drawText (juce::String::charToString (kFx[f].key), kk.toNearestInt(), juce::Justification::centred);
    }
}
void FxList::mouseDown (const juce::MouseEvent& e) { const int f = (int) ((e.position.y - 24) / kRowH); if (f >= 0 && f < kNumFx) ed.select (f); }

// ======================= Display =======================
void Display::paint (juce::Graphics& g)
{
    auto& P = ed.proc; auto full = getLocalBounds().toFloat();
    auto b = full.reduced (10);
    // fondo, vetro e griglia: una volta sola in cache (prima ogni fotogramma)
    base.draw (g, full, 1, [&] (juce::Graphics& bg) { bg.fillAll (K::win); neo::setTheme (3); neo::plate (bg, full.reduced (1), K::brand2, 12.0f); neo::crt (bg, b, K::brand2, 9.0f); lcdGrid (bg, b.reduced (4), 12); });
    const int steps = ed.steps(); const double beats = steps / 4.0;
    auto hdr = b.removeFromTop (30).reduced (12, 0);
    const double beat = P.uiBeat.load(); const double pb = std::fmod (beat, beats);
    juce::String pos; pos << "BAR " << ((int) (pb / 4) + 1) << "." << ((int) std::fmod (pb, 4.0) + 1);
    g.setFont (jlook::mono (13.0f)); g.setColour (K::amber); g.drawText (pos, hdr, juce::Justification::centredLeft);
    const float spd = P.uiSpeed.load();
    g.setFont (jlook::mono (11.0f)); g.setColour (K::brand2); g.drawText ("SPEED " + juce::String (spd, 2) + juce::String::fromUTF8 ("\xc3\x97") + "   " + juce::String (P.uiBpm.load(), 1) + " BPM" + (P.uiHost.load() ? "  HOST" : ""), hdr, juce::Justification::centredRight);
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
    auto b = getLocalBounds().toFloat().reduced (2); const bool en = ed.proc.fxEnabled (fx); const bool on = en && (down || ((ed.proc.activeMask.load() >> fx) & 1)); const auto c = en ? fxCol (fx) : K::faint;
    shownOn = on ? 1 : 0;
    // tasto CYBER: nero con un angolo tagliato, si accende pieno del colore dell'effetto
    auto pth = neo::chamfer (b, 6.0f, 2.0f);
    g.setColour (on ? c : juce::Colour (0xff1b1e27)); g.fillPath (pth);
    if (! on) { g.setColour (c.withAlpha (0.8f)); g.fillRect (b.getX() + 2, b.getY() + 6, 2.0f, b.getHeight() - 12); }
    g.setColour (on ? c.brighter (0.4f) : juce::Colours::white.withAlpha (0.09f)); g.strokePath (pth, juce::PathStrokeType (1.0f));
    static const char* shortN[] = { "STOP", "START", "SPIN", "HALF", "REV", "STUTTER", "ZONA", "GLITCH", "GATE", "CRUSH", "SWEEP", "ECO", "REVERB", "RISER", "TREMOLO", "ROTARY", "AUTOFILT", "RING", "SHIFT", "DIST" };
    g.setColour (on ? juce::Colour (0xff0b0c10) : K::ink); g.setFont (tf (9.5f, 2)); g.drawText (shortN[fx], b.reduced (6, 0).toNearestInt(), juce::Justification::centredLeft);
    const bool shot = ed.proc.value ("mode") > 0.5f;
    g.setColour (on ? juce::Colour (0xff0b0c10).withAlpha (0.7f) : c.withAlpha (0.8f)); g.setFont (tf (8.5f, 2, true));
    g.drawText (juce::String::charToString (kFx[fx].key) + (! en ? juce::String (" OFF") : shot ? juce::String (" 1x") : juce::String()), b.reduced (5, 0).toNearestInt(), juce::Justification::centredRight);
}
void PadBtn::mouseDown (const juce::MouseEvent&) { down = true; ed.select (fx); ed.proc.uiPad[fx] = true; repaint(); }
void PadBtn::mouseUp (const juce::MouseEvent&) { down = false; ed.proc.uiPad[fx] = false; repaint(); }

// ======================= Editor effetto =======================
juce::Rectangle<int> FxEditor::curveArea() const { return { 16, 44, getWidth() * 36 / 100 - 16, getHeight() - 58 }; }
float FxEditor::paramSig() const
{
    float sg = (float) ed.sel * 1000.0f + (ed.proc.fxEnabled (ed.sel) ? 0.5f : 0.0f);
    for (int i = 0; i < kNumParams; ++i) if (kParams[i].fx == ed.sel) sg += ed.proc.apvts.getParameter (kParams[i].id)->getValue() * (float) (i + 3);
    return sg;
}
void FxEditor::rebuild()
{
    knobs.clear();
    const auto c = fxCol (ed.sel);
    for (int i = 0; i < kNumParams; ++i)
        if (kParams[i].fx == ed.sel)
        {
            const auto spec = kParams[i];
            auto* k = knobs.add (new Knob (ed.proc.apvts.getParameter (spec.id), spec.label, c, [spec] (float v) { return fmtSpec (spec, v); }));
            k->onStart = [this] { ed.pushUndo(); };
            k->onUserChange = [this] (float) { repaint (curveArea()); };
            addAndMakeVisible (k);
        }
    const juce::String onId = juce::String (kFx[ed.sel].id) + "_on";
    onTog = std::make_unique<Toggle> ([this, onId] { return ed.proc.value (onId.toRawUTF8()) > 0.5f; },
                                      [this, onId] { ed.pushUndo(); auto* pr = ed.proc.apvts.getParameter (onId); pr->setValueNotifyingHost (pr->getValue() > 0.5f ? 0.0f : 1.0f); repaint(); ed.refreshAll(); });
    addAndMakeVisible (*onTog);
    resized(); repaint();
}
void FxEditor::resized()
{
    if (onTog) onTog->setBounds (getWidth() - 56, 11, 38, 20);
    auto a = getLocalBounds().withTrimmedLeft (getWidth() * 36 / 100 + 6).reduced (6, 10).withTrimmedTop (28);
    const int n = knobs.size(); if (n == 0) return;
    const int w = a.getWidth() / n, s = juce::jmin (78, w - 4);
    for (int i = 0; i < n; ++i) knobs[i]->setBounds (a.getX() + i * w + (w - s) / 2, a.getCentreY() - (s + 27) / 2, s, s + 27);
}
void FxEditor::paint (juce::Graphics& g)
{
    const bool en = ed.proc.fxEnabled (ed.sel);
    const auto c = en ? fxCol (ed.sel) : K::faint;
    base.draw (g, getLocalBounds().toFloat(), (int) c.getARGB(), [&] (juce::Graphics& bg) { panel (bg, getLocalBounds().toFloat().reduced (1), 12, c); });
    g.setColour (c.withAlpha (0.3f)); g.fillRect (14.0f, 13.0f, 14.0f, 14.0f); g.setColour (c.brighter (0.3f)); g.fillRect (17.0f, 16.0f, 8.0f, 8.0f);
    g.setFont (tf (15.0f, 2)); g.setColour (K::ink); g.drawText (juce::String (kFx[ed.sel].name).toUpperCase(), juce::Rectangle<float> (36, 8, 220, 24), juce::Justification::centredLeft);
    const float nw = juce::GlyphArrangement::getStringWidth (tf (15.0f, 2), juce::String (kFx[ed.sel].name).toUpperCase());
    g.setColour (K::dim); g.setFont (tf (11.0f)); g.drawText (kFx[ed.sel].desc, (int) (48 + nw), 9, 400, 24, juce::Justification::centredLeft);
    caps (g, en ? "Attivo" : "Spento", { getWidth() - 130, 9, 68, 24 }, en ? c : K::dim, juce::Justification::centredRight);
    g.setColour (c); g.fillRoundedRectangle (14.0f, 36.0f, getWidth() - 28.0f, 2.0f, 1.0f);
    auto ca = curveArea().toFloat().withTrimmedTop (6);
    inset (g, ca, 6);
    auto in = ca.reduced (10, 10);
    lcdGrid (g, in, 10);
    auto& P = ed.proc; const int f = ed.sel; juce::Path p; const int M = 160;
    // asse x: 2 battute per gli effetti a durata, cosi la lunghezza si vede
    const bool timed = f == STOP || f == START || f == SPIN || f == HALF || f == SWEEP || f == ECHO || f == THROW || f == RISER;
    if (timed) for (int q = 1; q < 8; ++q) { g.setColour (K::brand2.withAlpha (q == 4 ? 0.25f : 0.08f)); g.drawVerticalLine ((int) (in.getX() + in.getWidth() * q / 8.0f), in.getY(), in.getBottom()); }
    const double bpm = juce::jmax (40.0, (double) P.uiBpm.load());
    float marker = -1;
    for (int i = 0; i <= M; ++i)
    {
        const float x = i / (float) M, beats = x * 8.0f; float y = 1;
        switch (f)
        {
            case STOP:  { const float L = P.fxValue (STOP, "len"); marker = L / 8; y = beats < L ? 1 - shapeFn (beats / L, P.fxValue (STOP, "curve")) : 0; break; }
            case START: { const float L = P.fxValue (START, "len"); marker = L / 8; y = beats < L ? shapeFn (beats / L, -P.fxValue (START, "curve")) : 1; break; }
            case SPIN:  { const float L = P.fxValue (SPIN, "len"), e = beats / L, a = juce::jmin (1.0f, e * 10); marker = L / 8; y = e < 1 ? -P.fxValue (SPIN, "force") * a * std::pow (1 - e, 1.6f) / 5.0f : 0; break; }
            case HALF:  { const float sp = 1.0f / P.fxValue (HALF, "div"), gl = (float) (P.fxValue (HALF, "glide") / 1000.0 * bpm / 60.0);
                          const float t = beats - 1.0f; y = t < 0 ? 1 : (gl > 0.001f ? sp + (1 - sp) * std::exp (-3.0f * t / gl) : sp); if (t > 4) y = 1; break; }
            case REV:   y = x < 0.1f || x > 0.9f ? 1.0f : -P.fxValue (REV, "speed") / 1.5f; break;
            case STUT:  { const float sz = P.fxValue (STUT, "size"), roll = P.fxValue (STUT, "roll"); float t = x * 2.0f, len = sz, acc = 0; int k = 0;
                          while (acc + len < t && k < 64) { acc += len; ++k; len = juce::jmax (0.125f, sz * std::pow (1 - roll * 0.22f, (float) k)); }
                          const float w = (t - acc) / len, dec = std::pow (1 - P.fxValue (STUT, "decay") * 0.18f, (float) k);
                          y = w < P.fxValue (STUT, "gate") ? std::pow (2.0f, P.fxValue (STUT, "pitch") * x / 12.0f) * 0.5f * dec : 0.05f; break; }
            case LOOP:  { const float z = P.fxValue (LOOP, "size") / 8.0f; y = 0.2f + 0.7f * std::fmod (x, z) / z; break; }
            case GLITCH:{ const float gs = P.fxValue (GLITCH, "size"); juce::Random r (int (x * 2.0f / gs) * 7 + 3); const float d = P.fxValue (GLITCH, "dens"), ch = P.fxValue (GLITCH, "chaos");
                          y = r.nextFloat() < d ? (r.nextFloat() * (0.8f + ch) - 0.5f * ch) : 1; break; }
            case GATE:  { const float rt = P.fxValue (GATE, "rate") * 2.0f, ph = std::fmod (x * 4, rt) / rt, m = P.fxValue (GATE, "mix"); y = ph < P.fxValue (GATE, "duty") ? 1 : 1 - P.fxValue (GATE, "depth") * m; break; }
            case CRUSH: { const int rr = juce::jmax (1, juce::roundToInt (P.fxValue (CRUSH, "rate"))); const int ii = i - i % juce::jmin (rr, 40);
                          const float xin = std::sin ((float) ii / M * juce::MathConstants<float>::twoPi * 2.0f) * 0.7f;
                          const float wet = crushBits (crushShape (xin, P.fxValue (CRUSH, "drive"), P.fxValue (CRUSH, "char")), P.fxValue (CRUSH, "bits"));
                          const float m = P.fxValue (CRUSH, "mix"); y = (xin + (wet * juce::Decibels::decibelsToGain (P.fxValue (CRUSH, "vol")) - xin) * m) * 0.75f + 0.15f; break; }
            case SWEEP: { const float L = P.fxValue (SWEEP, "len"); marker = L / 8; const float pr = juce::jmin (1.0f, beats / L) * P.fxValue (SWEEP, "depth");
                          y = P.fxValue (SWEEP, "dir") >= 0 ? 1.0f - pr * 0.9f : 0.1f + pr * 0.9f; break; }
            case ECHO:  { const float tm = P.fxValue (ECHO, "time"), fb = P.fxValue (ECHO, "fb"); const float b2 = beats - 1.0f;
                          if (b2 < 0) y = 1.0f;
                          else { const int k = (int) (b2 / tm); const float ph = b2 / tm - k; y = ph < 0.25f ? std::pow (fb, (float) k) * P.fxValue (ECHO, "mix") : 0.0f; } break; }
            case THROW: { const float b2 = beats - 1.0f; y = b2 < 0 ? 1.0f : std::exp (-b2 / (0.6f + P.fxValue (THROW, "size") * 3.0f)) * P.fxValue (THROW, "mix"); break; }
            case TREM:  { const float rt = P.fxValue (TREM, "rate"), k = 1 + P.fxValue (TREM, "shape") * 12; const float s = std::cos (beats / rt * juce::MathConstants<float>::twoPi);
                          y = 1.0f - P.fxValue (TREM, "depth") * (0.5f - 0.5f * std::tanh (s * k) / std::tanh (k)); break; }
            case ROTARY:{ const float sp = 0.8f + P.fxValue (ROTARY, "speed") * 5.9f; y = 0.6f + 0.35f * P.fxValue (ROTARY, "depth") * std::sin (x * 4.0f * sp); break; }
            case AUTOFILT: { const float rt = P.fxValue (AUTOFILT, "rate"); y = 0.15f + P.fxValue (AUTOFILT, "freq") * 0.5f + 0.4f * P.fxValue (AUTOFILT, "depth") * (0.5f - 0.5f * std::cos (beats / rt * juce::MathConstants<float>::twoPi)); break; }
            case RINGMOD: { const float f0 = P.fxValue (RINGMOD, "freq"); const float fr = f0 * std::pow (0.25f, juce::jmin (1.0f, beats / 4.0f) * P.fxValue (RINGMOD, "glide")); y = 0.2f + 0.8f * std::log (fr / 20.0f) / std::log (100.0f); break; }
            case FSHIFT:{ y = 0.6f + 0.35f * std::sin (x * juce::MathConstants<float>::twoPi * (1.0f + std::abs (P.fxValue (FSHIFT, "hz")) / 40.0f)); break; }
            case DIST:  { const float xin = std::sin (x * juce::MathConstants<float>::twoPi * 2.0f) * 0.8f, gd = 1 + P.fxValue (DIST, "drive") * 12; y = 0.25f + std::tanh (xin * gd) * 0.6f; break; }
            case RISER: { const float L = P.fxValue (RISER, "len"); marker = L / 8; y = 0.15f + 0.85f * shapeFn (juce::jmin (1.0f, beats / L), P.fxValue (RISER, "curve")) * P.fxValue (RISER, "pitch") / 24.0f; break; }
            default: break;
        }
        // il mix avvicina la curva alla velocita normale (segnale originale)
        if (! isOverlay (f)) { const float m = P.fxValue (f, "mix"); y = 1 + (y - 1) * m; }
        const float px = in.getX() + x * in.getWidth(), py = in.getCentreY() + in.getHeight() * 0.5f - juce::jlimit (-1.0f, 1.3f, y) * in.getHeight() * 0.42f - in.getHeight() * 0.08f;
        if (i == 0) p.startNewSubPath (px, py); else p.lineTo (px, py);
    }
    juce::Path fill (p); fill.lineTo (in.getRight(), in.getBottom()); fill.lineTo (in.getX(), in.getBottom()); fill.closeSubPath();
    juce::ColourGradient fg (c.withAlpha (0.28f), 0, in.getY(), c.withAlpha (0.0f), 0, in.getBottom(), false); g.setGradientFill (fg); g.fillPath (fill);
    g.setColour (c.withAlpha (0.25f)); g.strokePath (p, juce::PathStrokeType (5.0f)); g.setColour (c); g.strokePath (p, juce::PathStrokeType (2.0f));
    if (marker > 0 && marker <= 1) { const float mx = in.getX() + marker * in.getWidth(); g.setColour (K::amber.withAlpha (0.7f)); g.drawVerticalLine ((int) mx, in.getY(), in.getBottom()); }
    if (timed) { g.setColour (K::dim); g.setFont (tf (8.5f, 2, true)); g.drawText ("2 BATTUTE", in.reduced (4, 2).toNearestInt(), juce::Justification::topRight); }
    scan (g, ca.reduced (2));
}

// ======================= Coreografia =======================
juce::Rectangle<float> Timeline::grid() const { return getLocalBounds().toFloat().reduced (12, 0).withTrimmedTop (22).withTrimmedBottom (10).withTrimmedLeft (150); }
int Timeline::span() const { return ed.res == 0 ? 1 : ed.res == 1 ? 2 : 4; }
int Timeline::rawStep (float x) const { auto gr = grid(); return juce::jlimit (0, ed.steps() - 1, (int) ((x - gr.getX()) / (gr.getWidth() / ed.steps()))); }
bool Timeline::cellAt (juce::Point<float> p, int& lane, int& step) const
{
    auto gr = grid(); if (! gr.contains (p)) return false;
    lane = juce::jlimit (0, kNumFx - 1, (int) ((p.y - gr.getY()) / (gr.getHeight() / kNumFx)));
    step = rawStep (p.x) / span() * span();
    return true;
}
bool Timeline::blockAt (int lane, int step, int& s, int& e) const
{
    if (lane < 0 || step < 0 || ! ed.proc.seq[lane][step].load()) return false;
    s = step; while (s > 0 && ed.proc.seq[lane][s - 1].load()) --s;
    e = step; while (e < ed.steps() && ed.proc.seq[lane][e].load()) ++e;
    return true;
}
void Timeline::setRange (int lane, int a, int b, bool v) { for (int k = juce::jmax (0, a); k < juce::jmin (b, kMaxSteps); ++k) ed.proc.seq[lane][k] = v; }
void Timeline::deleteSelected()
{
    if (selLane < 0) return;
    ed.pushUndo(); setRange (selLane, selS, selE, false); selLane = -1; repaint();
}
juce::int64 Timeline::sig() const
{
    juce::int64 h = ed.proc.uiStep.load() * 1000003LL + (ed.proc.value ("seqOn") > 0.5f) * 7 + ed.steps() * 131 + ed.res * 17 + ed.sel * 29 + hLane * 37 + hStep * 41 + selLane * 43 + selS * 47 + selE * 53;
    for (int f = 0; f < kNumFx; ++f) { h = h * 31 + (ed.proc.fxEnabled (f) ? 1 : 0); for (int k = 0; k < ed.steps(); ++k) if (ed.proc.seq[f][k].load()) h = h * 1099511628211LL + f * 977 + k; }
    return h;
}
void Timeline::paint (juce::Graphics& g)
{
    lastSig = sig();
    auto b = getLocalBounds().toFloat();
    inset (g, b, 8);
    auto gr = grid(); const int S = ed.steps(), sp = span(); const float cw = gr.getWidth() / S, lh = gr.getHeight() / kNumFx;
    const int cur = ed.proc.uiStep.load(); const bool on = ed.proc.value ("seqOn") > 0.5f;
    for (int s = 0; s < S; s += 4)
    {
        const float x = gr.getX() + s * cw;
        if (s % 16 == 0) { g.setColour (K::amber.withAlpha (0.9f)); g.setFont (tf (10.0f, 2, true)); g.drawText ("BAR " + juce::String (s / 16 + 1), juce::Rectangle<float> (x + 3, 4, 60, 14), juce::Justification::centredLeft); }
        else if (cw * 4 > 26) { g.setColour (K::dim); g.setFont (tf (9.0f, 1, true)); g.drawText (juce::String (s / 16 + 1) + "." + juce::String ((s % 16) / 4 + 1), juce::Rectangle<float> (x + 3, 4, 40, 14), juce::Justification::centredLeft); }
    }
    g.setColour (K::faint); g.setFont (tf (8.5f, 1));
    g.drawText (selLane >= 0 ? juce::String ("zona selezionata: CANC per cancellare") : juce::String ("trascina per creare una zona"), juce::Rectangle<float> (14, 4, 134, 14), juce::Justification::centredLeft);
    for (int f = 0; f < kNumFx; ++f)
    {
        const float y = gr.getY() + f * lh; const bool en = ed.proc.fxEnabled (f); const auto c = en ? fxCol (f) : K::faint;
        const bool selL = f == ed.sel, hovL = f == hLane;
        if (selL || hovL) { g.setColour (c.withAlpha (selL ? 0.08f : 0.05f)); g.fillRect (12.0f, y, b.getWidth() - 24, lh); }
        g.setColour (c.withAlpha (0.3f)); g.fillRect (17.0f, y + lh * 0.5f - 6, 12.0f, 12.0f); g.setColour (c); g.fillRect (20.0f, y + lh * 0.5f - 3, 6.0f, 6.0f);
        g.setColour (selL ? c : K::ink.withAlpha (en ? 0.85f : 0.4f)); g.setFont (tf (11.0f, 2)); g.drawText (juce::String (kFx[f].name).toUpperCase() + (en ? "" : " OFF"), juce::Rectangle<float> (36, y, 110, lh), juce::Justification::centredLeft);
        for (int s = 0; s < S; s += sp)
        {
            auto r = juce::Rectangle<float> (gr.getX() + s * cw + 1.5f, y + 2.5f, sp * cw - 3, lh - 5);
            const bool beat = (s / 4) % 2 == 0;
            g.setColour (juce::Colours::white.withAlpha (beat ? 0.045f : 0.025f)); g.fillRect (r);
        }
        int s = 0;
        while (s < S)
        {
            if (! ed.proc.seq[f][s].load()) { ++s; continue; }
            int e = s; while (e < S && ed.proc.seq[f][e].load()) ++e;
            auto r = juce::Rectangle<float> (gr.getX() + s * cw + 1.5f, y + 2.5f, (e - s) * cw - 3, lh - 5);
            const bool playing = on && en && cur >= s && cur < e;
            if (playing) { g.setColour (c.withAlpha (0.35f)); g.fillRect (r.expanded (2.0f)); }
            g.setColour (playing ? c.brighter (0.35f) : c.withMultipliedBrightness (0.85f)); g.fillPath (neo::chamfer (r, juce::jmin (5.0f, r.getHeight() * 0.3f), 1.5f));
            g.setColour (juce::Colours::white.withAlpha (0.4f)); g.drawHorizontalLine ((int) r.getY() + 1, r.getX() + 3, r.getRight() - 3);
            // maniglie ai bordi
            g.setColour (juce::Colours::black.withAlpha (0.35f)); g.fillRoundedRectangle (r.getX() + 2, r.getCentreY() - 6, 3, 12, 1.5f); g.fillRoundedRectangle (r.getRight() - 5, r.getCentreY() - 6, 3, 12, 1.5f);
            if (r.getWidth() > 70) { g.setColour (juce::Colour (0xff0a0c10).withAlpha (0.85f)); g.setFont (tf (10.0f, 2)); g.drawText (juce::String (kFx[f].name).toUpperCase(), r.reduced (9, 0).toNearestInt(), juce::Justification::centredLeft); }
            if (f == selLane && s == selS && e == selE) { neo::brackets (g, r.expanded (2.5f), juce::Colours::white, 6.0f, 1.6f); }
            s = e;
        }
    }
    for (int s = 0; s <= S; s += 4)
    {
        const float x = gr.getX() + s * cw;
        g.setColour (s % 16 == 0 ? K::amber.withAlpha (0.45f) : K::brand2.withAlpha (0.14f)); g.fillRect (x - (s % 16 == 0 ? 1.0f : 0.5f), gr.getY(), s % 16 == 0 ? 2.0f : 1.0f, gr.getHeight());
    }
    for (int f = 1; f < kNumFx; ++f) { g.setColour (juce::Colours::white.withAlpha (0.04f)); g.drawHorizontalLine ((int) (gr.getY() + f * lh), 12.0f, gr.getRight()); }
    if (hLane >= 0 && hStep >= 0 && mode == NONE)
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
    auto cur = juce::MouseCursor::NormalCursor;
    if (l >= 0)
    {
        int bs, be; const int rs = rawStep (e.position.x);
        if (blockAt (l, rs, bs, be))
        {
            auto gr = grid(); const float cw = gr.getWidth() / ed.steps(), x0 = gr.getX() + bs * cw, x1 = gr.getX() + be * cw;
            cur = (e.position.x - x0 < 8 || x1 - e.position.x < 8) ? juce::MouseCursor::LeftRightResizeCursor : juce::MouseCursor::DraggingHandCursor;
        }
        else cur = juce::MouseCursor::CrosshairCursor;
    }
    setMouseCursor (cur);
}
void Timeline::mouseDown (const juce::MouseEvent& e)
{
    auto gr = grid();
    if (e.position.x < gr.getX() && e.position.y >= gr.getY())   // clic sul nome della corsia: seleziona l'effetto
    {
        const int l = juce::jlimit (0, kNumFx - 1, (int) ((e.position.y - gr.getY()) / (gr.getHeight() / kNumFx)));
        ed.select (l); repaint(); return;
    }
    int l, s; if (! cellAt (e.position, l, s)) { selLane = -1; repaint(); return; }
    grabKeyboardFocus();
    const int rs = rawStep (e.position.x); int bs, be;
    ed.pushUndo();
    for (int k = 0; k < kMaxSteps; ++k) before[k] = ed.proc.seq[l][k].load();
    dLane = l; ed.select (l);
    if (blockAt (l, rs, bs, be))
    {
        if (e.mods.isPopupMenu() || e.mods.isAltDown()) { setRange (l, bs, be, false); selLane = -1; mode = NONE; repaint(); return; }   // tasto destro: cancella la zona
        selLane = l; selS = bs; selE = be; bS = bs; bE = be;
        const float cw = gr.getWidth() / ed.steps(), x0 = gr.getX() + bs * cw, x1 = gr.getX() + be * cw;
        mode = e.position.x - x0 < 8 ? RESL : x1 - e.position.x < 8 ? RESR : MOVE;
        dOff = rs - bs;
    }
    else
    {
        mode = (e.mods.isPopupMenu() || e.mods.isAltDown()) ? ERASE : CREATE; anchor = s;
        setRange (l, s, s + span(), mode == CREATE);
        if (mode == CREATE) { selLane = l; selS = s; selE = s + span(); int a2, b2; if (blockAt (l, s, a2, b2)) { selS = a2; selE = b2; } }
        else selLane = -1;
    }
    repaint();
}
void Timeline::mouseDrag (const juce::MouseEvent& e)
{
    if (mode == NONE || dLane < 0) return;
    const int S = ed.steps(), sp = span(); const int rs = rawStep (e.position.x), st = rs / sp * sp;
    for (int k = 0; k < kMaxSteps; ++k) ed.proc.seq[dLane][k] = before[k];
    if (mode == CREATE || mode == ERASE)
    {
        const int a = juce::jmin (anchor, st), b = juce::jmin (S, juce::jmax (anchor, st) + sp);
        setRange (dLane, a, b, mode == CREATE);
        if (mode == CREATE) { int a2, b2; if (blockAt (dLane, a, a2, b2)) { selLane = dLane; selS = a2; selE = b2; } }
    }
    else
    {
        int ns = bS, ne = bE;
        if (mode == MOVE) { ns = juce::jlimit (0, S - (bE - bS), (rs - dOff) / sp * sp); ne = ns + (bE - bS); }
        if (mode == RESL) { ns = juce::jlimit (0, bE - 1, st); }
        if (mode == RESR) { ne = juce::jlimit (bS + 1, S, st + sp); }
        setRange (dLane, bS, bE, false); setRange (dLane, ns, ne, true);
        int a2, b2; if (blockAt (dLane, ns, a2, b2)) { selLane = dLane; selS = a2; selE = b2; }
    }
    hLane = dLane; hStep = st;
    repaint();
}
void Timeline::mouseDoubleClick (const juce::MouseEvent& e)
{
    auto gr = grid();
    if (e.position.x >= gr.getX() || e.position.y < gr.getY()) return;
    const int l = juce::jlimit (0, kNumFx - 1, (int) ((e.position.y - gr.getY()) / (gr.getHeight() / kNumFx)));
    ed.pushUndo();
    for (int s = 0; s < kMaxSteps; ++s) ed.proc.seq[l][s] = false;
    selLane = -1; repaint();
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
    // tutto lo sfondo sta in un'immagine: prima veniva ridisegnato (grana, ombre sfocate, gradienti) dietro a ogni elemento che si aggiornava, 30 volte al secondo
    const int W = Editor::W, H = Editor::H;
    base.draw (g, { 0, 0, (float) W, (float) H }, 3, [this] (juce::Graphics& bg) { paintStatic (bg); });
    g.setColour (K::faint); g.setFont (neo::num (8.5f));
    g.drawText (ed.proc.uiHost.load() ? "BPM SINCRONIZZATI CON L'HOST" : (ed.proc.value ("sync") > 0.5f ? "IN ATTESA DEL PLAY DELL'HOST" : "BPM MANUALI"), juce::Rectangle<int> (W - 500, H - 30, 410, 30), juce::Justification::centredRight);
}
void Root::paintStatic (juce::Graphics& g)
{
    const int W = Editor::W, H = Editor::H; const float Wf = (float) W, Hf = (float) H;
    neo::setTheme (3);
    neo::cyberBackdrop (g, Wf, Hf, 0.6f);
    neo::cyberHeader (g, Wf, 58, "TPE-9", "TAPE MOTION // STOP  SPIN  STUTTER  COREOGRAFIA");
    neo::plate (g, ed.sidePanel.toFloat().reduced (6, 8), K::brand2);
    neo::plate (g, ed.rightPanel.toFloat().reduced (6, 8), K::amber);
    auto right = ed.rightPanel;
    caps (g, "Macro", { right.getX() + 22, 72, 120, 16 }, K::ink);
    g.setColour (K::amber); g.fillRoundedRectangle ((float) right.getX() + 18, 90, (float) right.getWidth() - 36, 2.0f, 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.08f)); g.drawHorizontalLine (290, (float) right.getX() + 18, (float) right.getRight() - 18); g.drawHorizontalLine (458, (float) right.getX() + 18, (float) right.getRight() - 18);
    caps (g, "Nastro", { right.getX() + 18, 296, 120, 16 }, K::amber);
    caps (g, "Uscita", { right.getX() + 18, 464, 120, 16 }, K::amber);
    // riga sotto il display
    caps (g, "Pad", { 226, 264, 40, 28 }, K::dim, juce::Justification::centredLeft, 10.0f);
    caps (g, "Quantizza", { 404, 264, 80, 28 }, K::dim, juce::Justification::centredLeft, 10.0f);
    g.setColour (K::faint); g.setFont (tf (10.5f)); g.drawText (juce::String::fromUTF8 ("tieni premuto \xc2\xb7 tasti 1-0 e Q-P \xc2\xb7 pi\xc3\xb9 pad insieme si sommano"), juce::Rectangle<int> (640, 264, 366, 28), juce::Justification::centredRight);
    // coreografia
    neo::plate (g, juce::Rectangle<float> (8, 552, Wf - 16, Hf - 32 - 558), K::brand1);
    caps (g, "Coreografia", { 24, 560, 110, 30 }, K::ink);
    caps (g, "Battute", { 172, 560, 60, 30 }, K::dim, juce::Justification::centredLeft, 9.5f);
    caps (g, "Griglia", { 432, 560, 50, 30 }, K::dim, juce::Justification::centredLeft, 9.5f);
    caps (g, "Stile", { 652, 560, 46, 30 }, K::brand1, juce::Justification::centredLeft, 9.5f);
    // piede
    g.setColour (juce::Colours::white.withAlpha (0.06f)); g.drawHorizontalLine (H - 30, 0, Wf);
    g.setColour (K::faint); g.setFont (tf (9.5f, 2, true));
    g.drawText (juce::String::fromUTF8 ("JACKPOT.WAV \xc2\xb7 TAPE MOTION \xc2\xb7 AU"), juce::Rectangle<int> (16, H - 30, 400, 30), juce::Justification::centredLeft);
    g.setFont (tf (9.0f, 1)); g.drawText (juce::String::fromUTF8 ("coreografia: trascina = crea zona \xc2\xb7 trascina la zona = sposta \xc2\xb7 bordi = allunga \xc2\xb7 tasto dx o CANC = cancella \xc2\xb7 cmd+Z annulla"),
                juce::Rectangle<int> (330, H - 30, W - 660, 30), juce::Justification::centred);
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
    auto setC = [this] (const char* id) { return [this, id] (int i) { pushUndo(); auto* pr = proc.apvts.getParameter (id); pr->setValueNotifyingHost (pr->convertTo0to1 ((float) i)); fxEd.repaint(); for (auto* pd : pads) pd->repaint(); tl.repaint(); }; };
    modeSeg = std::make_unique<Seg> (juce::StringArray { "Hold", "One-shot" }, choice ("mode"), setC ("mode"));
    qSeg = std::make_unique<Seg> (juce::StringArray { "Off", "1/16", "1/4", "Bar" }, choice ("quant"), setC ("quant"));
    barsSeg = std::make_unique<Seg> (juce::StringArray { "1", "2", "4", "8" }, choice ("bars"), setC ("bars"));
    resSeg = std::make_unique<Seg> (juce::StringArray { "1/16", "1/8", "1/4" }, [this] { return res; }, [this] (int i) { res = i; });
    seqTog = std::make_unique<Toggle> ([this] { return proc.value ("seqOn") > 0.5f; }, [this] { auto* pr = proc.apvts.getParameter ("seqOn"); pr->setValueNotifyingHost (pr->getValue() > 0.5f ? 0.0f : 1.0f); });
    clearBtn = std::make_unique<TextBtn> ("Svuota", [this] { pushUndo(); tl.selLane = -1; for (auto& r : proc.seq) for (auto& s : r) s = false; tl.repaint(); });
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
    genBtn = std::make_unique<TextBtn> ("Genera", [this] { pushUndo(); tl.selLane = -1; proc.generate(); fxEd.rebuild(); tl.repaint(); });
    genBtn->accent = true;
    abBtn = std::make_unique<TextBtn> ("A / B", [this] { pushUndo(); proc.ab.toggle (proc); abBtn->text = juce::String ("MEMORIA ") + proc.ab.label(); fxEd.rebuild(); tl.repaint(); root.repaint(); });
    abBtn->text = "MEMORIA A"; root.addAndMakeVisible (*abBtn);
    for (auto* c : std::initializer_list<juce::Component*> { modeSeg.get(), qSeg.get(), barsSeg.get(), resSeg.get(), seqTog.get(), clearBtn.get(), styleBtn.get(), genSeg.get(), genBtn.get() }) root.addAndMakeVisible (c);
    // preset
    auto showName = [this] { const auto n = proc.getProgramName (proc.presetIndex); pName->text = n.isEmpty() ? juce::String ("Init") : n; pName->repaint(); };
    pPrev = std::make_unique<TextBtn> (juce::String::fromUTF8 ("\xe2\x80\xb9"), [this, showName] { pushUndo(); proc.loadPreset ((proc.presetIndex - 1 + proc.getNumPrograms()) % proc.getNumPrograms()); showName(); fxEd.rebuild(); tl.repaint(); });
    pNext = std::make_unique<TextBtn> (juce::String::fromUTF8 ("\xe2\x80\xba"), [this, showName] { pushUndo(); proc.loadPreset ((proc.presetIndex + 1) % proc.getNumPrograms()); showName(); fxEd.rebuild(); tl.repaint(); });
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
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (pName.get()), [this, showName] (int r) { if (r > 0) { pushUndo(); proc.loadPreset (r - 1); showName(); fxEd.rebuild(); tl.repaint(); } });
    });
    bpmPill = std::make_unique<TextBtn> ("", [this] { auto* pr = proc.apvts.getParameter ("sync"); pr->setValueNotifyingHost (pr->getValue() > 0.5f ? 0.0f : 1.0f); });
    undoBtn = std::make_unique<TextBtn> ("Annulla", [this] { undo(); });
    redoBtn = std::make_unique<TextBtn> ("Rifai", [this] { redo(); });
    for (auto* c : std::initializer_list<juce::Component*> { pPrev.get(), pNext.get(), pName.get(), bpmPill.get(), undoBtn.get(), redoBtn.get() }) root.addAndMakeVisible (c);
    showName();
    // manopole del nastro
    const auto vio = K::brand1;
    auto add = [this] (const char* id, const char* lab, juce::Colour c, std::function<juce::String (float)> f) { auto* k = tapeKnobs.add (new Knob (proc.apvts.getParameter (id), lab, c, std::move (f))); root.addAndMakeVisible (k); return k; };
    auto* age = add ("age", "Age", juce::Colour (0xffa78bfa), pct); age->onUserChange = [this] (float a) { proc.applyAge (a); for (auto* k : tapeKnobs) k->repaint(); };
    add ("mix", "Mix", vio, pct); add ("out", "Output", vio, fmtOut);
    add ("wow", "Wow", vio, pct); add ("flut", "Flutter", vio, pct); add ("tone", "Tono", vio, pct);
    add ("drive", "Drive", vio, pct); add ("hiss", "Hiss", vio, pct); add ("drop", "Dropout", vio, pct);
    auto* bpmK = add ("bpm", "BPM man.", K::brand2, [] (float v) { return juce::String (v, 1); }); juce::ignoreUnused (bpmK);
    for (auto* k : tapeKnobs) k->onStart = [this] { pushUndo(); };

    fxEd.rebuild();
    setWantsKeyboardFocus (true);
    root.addAndMakeVisible (sizeChip);
    scaler.setup();
    startTimerHz (30);
}
Editor::~Editor() { setLookAndFeel (nullptr); }

int Editor::steps() const { static const int b[] = { 1, 2, 4, 8 }; return b[juce::jlimit (0, 3, (int) proc.value ("bars"))] * 16; }
void Editor::select (int f) { if (f != sel) { sel = f; fxEd.rebuild(); } list.repaint(); }

void Editor::pushUndo()
{
    auto cur = snap();
    if (! undoS.empty() && undoS.back().seq == cur.seq && undoS.back().st.isEquivalentTo (cur.st)) return;
    undoS.push_back (std::move (cur)); if (undoS.size() > 120) undoS.erase (undoS.begin());
    redoS.clear();
}
void Editor::restore (const Snap& sn)
{
    // ripristina solo i parametri (non i pad premuti)
    for (int i = 0; i < sn.st.getNumChildren(); ++i)
    {
        auto c = sn.st.getChild (i); const auto id = c.getProperty ("id").toString();
        if (id.startsWith ("pad")) continue;
        if (auto* pr = proc.apvts.getParameter (id)) pr->setValueNotifyingHost (pr->convertTo0to1 ((float) (double) c.getProperty ("value")));
    }
    proc.setSeqString (sn.seq);
    refreshAll();
}
void Editor::undo() { if (undoS.empty()) return; redoS.push_back (snap()); auto s = undoS.back(); undoS.pop_back(); restore (s); }
void Editor::redo() { if (redoS.empty()) return; undoS.push_back (snap()); auto s = redoS.back(); redoS.pop_back(); restore (s); }
void Editor::refreshAll()
{
    tl.selLane = -1; fxEd.rebuild(); tl.repaint(); list.repaint();
    for (auto* p : pads) p->repaint(); for (auto* k : tapeKnobs) k->repaint();
    for (auto* c : std::initializer_list<juce::Component*> { modeSeg.get(), qSeg.get(), barsSeg.get(), resSeg.get(), seqTog.get(), genSeg.get() }) c->repaint();
}

void Editor::resized()
{
    const float s = scaler.update();
    sizeChip.setBounds (W - 76, H - 18, 58, 15);
    root.setBounds (0, 0, W, H); root.setTransform (juce::AffineTransform::scale (s));
    list.setBounds (20, 74, 172, 404);
    disp.setBounds (224, 72, 782, 182);
    modeSeg->setBounds (256, 264, modeSeg->preferredWidth(), 28);
    qSeg->setBounds (484, 264, qSeg->preferredWidth(), 28);
    {   // due file di pad: nastro sopra, effetti sovrapposti sotto
        const int perRow = (kNumFx + 1) / 2, pw = (782 - (perRow - 1) * 5) / perRow;
        for (int f = 0; f < kNumFx; ++f) pads[f]->setBounds (224 + (f % perRow) * (pw + 5), 298 + (f / perRow) * 30, pw, 27);
    }
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
    pPrev->setBounds (380, 11, 34, 36); pName->setBounds (414, 11, 320, 36); pNext->setBounds (734, 11, 34, 36);
    bpmPill->setBounds (1060, 11, 164, 36);
    abBtn->setBounds (776, 11, 104, 36);
    undoBtn->setBounds (888, 11, 86, 36); redoBtn->setBounds (974, 11, 80, 36);
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
    // si ridisegna solo cio' che e' cambiato (prima tutto, 30 volte al secondo)
    disp.repaint(); meter.repaint(); bpmPill->repaint();
    { const int ls = proc.activeMask.load() * 64 + sel; int en = 0; for (int f = 0; f < kNumFx; ++f) en = en * 2 + (proc.fxEnabled (f) ? 1 : 0); if (ls * 7919 + en != lastListSig) { lastListSig = ls * 7919 + en; list.repaint(); } }
    if (tl.sig() != tl.lastSig) tl.repaint();
    for (auto* p : pads) { const bool on = proc.fxEnabled (p->fx) && (p->down || ((proc.activeMask.load() >> p->fx) & 1)); if ((on ? 1 : 0) != p->shownOn) p->repaint(); }
    for (auto* k : fxEd.knobs) k->refresh();
    { const float sg = fxEd.paramSig(); if (sg != fxEd.lastSig) { fxEd.lastSig = sg; fxEd.repaint(); } }
    if (proc.uiHost.load() && (sel == HALF)) fxEd.repaint (fxEd.curveArea());
    undoBtn->accent = false; undoBtn->setAlpha (undoS.empty() ? 0.45f : 1.0f); redoBtn->setAlpha (redoS.empty() ? 0.45f : 1.0f);
    { const auto st = proc.apvts.getParameter ("genStyle")->getCurrentValueAsText(); if (styleBtn->text != st) { styleBtn->text = st; styleBtn->repaint(); } }
    for (auto* k : tapeKnobs) k->refresh();
    { const juce::String ft = proc.uiHost.load() ? "h" : (proc.value ("sync") > 0.5f ? "s" : "m"); if (ft != lastFoot) { lastFoot = ft; root.repaint (W - 504, H - 32, 504, 32); } }
}

bool Editor::keyPressed (const juce::KeyPress& k)
{
    const int c = k.getKeyCode();
    if (k.getModifiers().isCommandDown() && (c == 'z' || c == 'Z')) { if (k.getModifiers().isShiftDown()) redo(); else undo(); return true; }
    if (k.getModifiers().isCommandDown() && (c == 'y' || c == 'Y')) { redo(); return true; }
    if (c == juce::KeyPress::deleteKey || c == juce::KeyPress::backspaceKey) { tl.deleteSelected(); return true; }
    for (int f = 0; f < kNumFx; ++f) if (juce::CharacterFunctions::toUpperCase (c) == kFx[f].key) return true;
    return false;
}
bool Editor::keyStateChanged (bool)
{
    bool used = false;
    for (int f = 0; f < kNumFx; ++f)
    {
        const bool d = juce::KeyPress::isKeyCurrentlyDown (kFx[f].key) || juce::KeyPress::isKeyCurrentlyDown (juce::CharacterFunctions::toLowerCase ((juce::juce_wchar) kFx[f].key));
        if (d && ! keysDown.count (f)) { keysDown.insert (f); proc.uiPad[f] = true; select (f); used = true; }
        else if (! d && keysDown.count (f)) { keysDown.erase (f); proc.uiPad[f] = false; used = true; }
    }
    return used;
}
}
