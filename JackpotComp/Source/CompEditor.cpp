#include "CompEditor.h"

namespace jc
{
juce::Font uf (float size, bool bold) { return jlook::ui (size, bold ? 2 : 1); }
juce::Font mf (float size) { return jlook::mono (size); }

void glowText (juce::Graphics& g, const juce::String& t, juce::Rectangle<float> r, juce::Colour c, juce::Font f, juce::Justification j)
{
    g.setFont (f);
    for (int k = 2; k >= 1; --k) { g.setColour (c.withAlpha (0.12f)); g.drawText (t, r.translated ((float) -k * 0.6f, 0), j); g.drawText (t, r.translated ((float) k * 0.6f, 0), j); }
    g.setColour (c); g.drawText (t, r, j);
}
void lcd (juce::Graphics& g, juce::Rectangle<float> r, float rad)
{
    skin::screen (g, r, rad, C::phos);
    g.setColour (C::phos.withAlpha (0.03f));
    for (float x = r.getX() + 10; x < r.getRight(); x += 12) g.drawVerticalLine ((int) x, r.getY() + 2, r.getBottom() - 2);
    for (float y = r.getY() + 10; y < r.getBottom(); y += 12) g.drawHorizontalLine ((int) y, r.getX() + 2, r.getRight() - 2);
}
static void scan (juce::Graphics& g, juce::Rectangle<float> r)
{
    g.setColour (juce::Colours::black.withAlpha (neo::light() ? 0.03f : 0.18f)); for (float y = r.getY(); y < r.getBottom(); y += 3) g.drawHorizontalLine ((int) y, r.getX(), r.getRight());
    jlook::glass (g, r, 6);
}

// ======================= frontali hardware =======================
// ogni modello ha il suo pannello: colore, scritte, tipo di manopola e targhetta, con cornice e dettagli al neon
struct Face { juce::uint32 p1, p2, ink; int knob; const char* plate; const char* code; };
static const Face& face (int m)
{
    static const Face F[kNumModels] = {
        { 0xffddd5c2, 0xffbfb6a1, 0xff201b14, 0, "LEVELING AMPLIFIER", "T4 OPTICAL / TUBE" },   // OPTO 2A
        { 0xffcfd0cc, 0xffa9aba7, 0xff1c1d1f, 0, "AUDIO LEVELER", "T4 OPTICAL / SOLID STATE" }, // OPTO 3A
        { 0xff1c1d21, 0xff0e0f12, 0xffd9dbe0, 1, "PEAK LIMITER", "FET  CLASS A" },               // FET 76
        { 0xff434a52, 0xff2c3238, 0xffe9ebee, 2, "BUS COMPRESSOR", "VCA  STEREO" },              // VCA BUS
        { 0xff35424a, 0xff212b31, 0xffe6dcc4, 0, "VARIABLE-MU LIMITER", "REMOTE CUTOFF TUBES" }, // VARI-MU
        { 0xff1b1b1d, 0xff0c0c0d, 0xffd6d7da, 1, "COMPRESSOR / LIMITER", "TRUE RMS  OVEREASY" }, // RMS 160
        { 0xff1d2027, 0xff0d0f13, 0xffe8eaef, 1, "COMPRESSOR EMULATOR", "DIST 2  DIST 3  NUKE" },// DISTRESS
        { 0xff2a1440, 0xff150a22, 0xffffd8f6, -1, "UP / DOWN MULTIBAND", "3 BANDS" },            // OTT
        { 0xff57554f, 0xff3a3934, 0xffefe6d0, 0, "VARIABLE-MU TUBE LIMITER", "6386  TWIN CHANNEL" }, // 670
        { 0xff121a22, 0xff0a1016, 0xffdcefff, -1, "MULTIBAND DYNAMICS", "LINKWITZ-RILEY  4 BANDS" }, // MULTI
        { 0xff32363b, 0xff1f2226, 0xffeceef0, 2, "CHANNEL DYNAMICS", "VCA  E SERIES" } };       // SSL
    return F[juce::jlimit (0, kNumModels - 1, m)];
}
static void screw (juce::Graphics& g, float x, float y, float r, float rot)
{
    juce::ColourGradient sg (juce::Colour (0xffd5d7db), x - r * 0.5f, y - r * 0.6f, juce::Colour (0xff55585e), x + r * 0.5f, y + r * 0.7f, false);
    g.setColour (juce::Colours::black.withAlpha (0.45f)); g.fillEllipse (x - r - 1, y - r + 0.5f, 2 * r + 2, 2 * r + 2);
    g.setGradientFill (sg); g.fillEllipse (x - r, y - r, 2 * r, 2 * r);
    g.setColour (juce::Colour (0xff2a2c30)); const float c = std::cos (rot) * r * 0.75f, sn = std::sin (rot) * r * 0.75f;
    g.drawLine (x - c, y - sn, x + c, y + sn, 1.3f); g.drawLine (x + sn, y - c, x - sn, y + c, 1.3f);
}
// cornice cyberpunk: angoli a staffa al neon e piccoli codici
static void cyberFrame (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour acc, float L = 18)
{
    g.setColour (acc.withAlpha (0.18f));
    for (int k = 0; k < 4; ++k)
    {
        const float x = (k & 1) ? r.getRight() : r.getX(), y = (k & 2) ? r.getBottom() : r.getY(), sx = (k & 1) ? -1.0f : 1.0f, sy = (k & 2) ? -1.0f : 1.0f;
        juce::Path p; p.startNewSubPath (x + sx * L, y); p.lineTo (x, y); p.lineTo (x, y + sy * L);
        g.setColour (acc.withAlpha (0.25f)); g.strokePath (p, juce::PathStrokeType (4.0f)); g.setColour (acc); g.strokePath (p, juce::PathStrokeType (1.5f));
    }
}

// ======================= Knob =======================
void Knob::mouseDown (const juce::MouseEvent& e)
{
    if (onTouch) onTouch();
    if (! enabled) return;
    if (e.mods.isAltDown() || e.mods.isCommandDown()) { param->setValueNotifyingHost (param->getDefaultValue()); return; }
    start = param->getValue(); param->beginChangeGesture(); drag = true;
}
void Knob::mouseDrag (const juce::MouseEvent& e)
{
    if (! drag) return;
    float v = juce::jlimit (0.0f, 1.0f, start - e.getDistanceFromDragStartY() / (e.mods.isShiftDown() ? 900.0f : 200.0f));
    if (stepped && steps > 1) v = std::round (v * (steps - 1)) / (steps - 1);
    param->setValueNotifyingHost (v);
}
void Knob::paintHw (juce::Graphics& g)
{
    using namespace juce;
    const float lh = 14, rh = 17;
    const float size = jmin ((float) getWidth(), getHeight() - lh - rh - 2);
    const float cx = getWidth() * 0.5f, cy = lh + size * 0.5f, R = size * 0.5f - 1;
    const float a0 = -2.36f, a1 = 2.36f, v = param->getValue(), av = a0 + v * (a1 - a0);
    auto P = [cx, cy] (float a, float r) { return Point<float> (cx + std::sin (a) * r, cy - std::cos (a) * r); };
    const float al = enabled ? 1.0f : 0.35f;
    // etichetta incisa
    g.setColour (ink.withAlpha (0.92f * al)); g.setFont (uf (big ? 10.5f : 9.5f)); g.drawText (label, Rectangle<float> (-10, 0, getWidth() + 20.0f, lh), Justification::centredTop);
    // scala stampata sul pannello
    for (int i = 0; i <= 10; ++i)
    {
        const float a = a0 + i * (a1 - a0) / 10, l = (i % 5 == 0) ? 5.0f : 3.0f;
        g.setColour (ink.withAlpha ((i % 5 == 0 ? 0.85f : 0.5f) * al)); g.drawLine (Line<float> (P (a, R), P (a, R - l)), i % 5 == 0 ? 1.4f : 1.0f);
    }
    if (lo.isNotEmpty()) { g.setFont (mf (8.0f)); g.setColour (ink.withAlpha (0.65f * al)); auto q0 = P (a0, R + 1), q1 = P (a1, R + 1);
        g.drawText (lo, Rectangle<float> (q0.x - 34, q0.y - 1, 34, 10), Justification::centredRight); g.drawText (hi, Rectangle<float> (q1.x, q1.y - 1, 40, 10), Justification::centredLeft); }
    // arco al neon del valore (dettaglio cyber)
    const float ra = R - 7;
    if (enabled)
    {
        const float s0 = bipolar ? 0.0f : a0;
        Path arc; arc.addCentredArc (cx, cy, ra, ra, 0, jmin (s0, av), jmax (s0, av), true);
        const float live = (hover ? 0.3f : 0.0f) + (drag ? 0.5f : 0.0f);
        g.setColour (col.withAlpha (0.25f + 0.2f * live)); g.strokePath (arc, PathStrokeType (4.5f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour (col.interpolatedWith (Colours::white, 0.15f + 0.3f * live)); g.strokePath (arc, PathStrokeType (1.6f, PathStrokeType::curved, PathStrokeType::rounded));
    }
    // corpo della manopola
    const float rs = R - 10, rc = rs * (hw == 1 ? 0.66f : hw == 2 ? 0.58f : 0.62f);
    g.setColour (Colours::black.withAlpha (0.45f)); g.fillEllipse (cx - rs - 1, cy - rs + 2.5f, 2 * rs + 2, 2 * rs + 2);
    if (hw == 1)
    {   // alluminio con zigrinatura
        ColourGradient sk (Colour (0xffeff0f2), cx - rs * 0.6f, cy - rs, Colour (0xff7d8086), cx + rs * 0.5f, cy + rs, false); g.setGradientFill (sk); g.fillEllipse (cx - rs, cy - rs, 2 * rs, 2 * rs);
        for (int i = 0; i < 40; ++i) { const float a = av + i * MathConstants<float>::twoPi / 40; g.setColour (Colours::black.withAlpha (0.16f)); g.drawLine (Line<float> (P (a, rs), P (a, rs - 2.5f)), 0.8f); }
        ColourGradient cg (Colour (0xffffffff), cx - rc * 0.4f, cy - rc * 0.5f, Colour (0xff9ea2a8), cx + rc * 0.6f, cy + rc, true); g.setGradientFill (cg); g.fillEllipse (cx - rc, cy - rc, 2 * rc, 2 * rc);
        g.setColour (Colours::black.withAlpha (0.25f)); g.drawEllipse (cx - rc, cy - rc, 2 * rc, 2 * rc, 0.8f);
        g.setColour (Colour (0xff111214)); g.drawLine (Line<float> (P (av, rc * 0.25f), P (av, rs - 1)), 2.2f);
    }
    else if (hw == 2)
    {   // gonna scura con alette, cappuccio colorato
        ColourGradient sk (Colour (0xff4a4d53), cx, cy - rs, Colour (0xff17181b), cx, cy + rs, false); g.setGradientFill (sk); g.fillEllipse (cx - rs, cy - rs, 2 * rs, 2 * rs);
        for (int i = 0; i < 24; ++i) { const float a = av + i * MathConstants<float>::twoPi / 24; g.setColour (Colours::white.withAlpha (0.07f)); g.drawLine (Line<float> (P (a, rs - 0.5f), P (a, rc + 1)), 1.0f); }
        const auto cc = enabled ? cap : Colour (0xff55585e);
        ColourGradient cg (cc.brighter (0.45f), cx - rc * 0.5f, cy - rc, cc.darker (0.5f), cx + rc * 0.4f, cy + rc, false); g.setGradientFill (cg); g.fillEllipse (cx - rc, cy - rc, 2 * rc, 2 * rc);
        g.setColour (Colours::white.withAlpha (0.25f)); g.fillEllipse (cx - rc * 0.55f, cy - rc * 0.8f, rc * 0.9f, rc * 0.55f);
        g.setColour (Colours::white.withAlpha (0.95f)); g.drawLine (Line<float> (P (av, rc * 0.2f), P (av, rs - 1.5f)), 2.0f);
    }
    else
    {   // nera grande con zigrinatura, tipo studio anni 60
        ColourGradient sk (Colour (0xff3b3c40), cx - rs * 0.4f, cy - rs, Colour (0xff050506), cx + rs * 0.4f, cy + rs, false); g.setGradientFill (sk); g.fillEllipse (cx - rs, cy - rs, 2 * rs, 2 * rs);
        for (int i = 0; i < 36; ++i) { const float a = av + i * MathConstants<float>::twoPi / 36; g.setColour (Colours::white.withAlpha (0.09f)); g.drawLine (Line<float> (P (a, rs - 0.5f), P (a, rs - 3.5f)), 1.0f); }
        ColourGradient cg (Colour (0xff2c2d31), cx - rc * 0.5f, cy - rc, Colour (0xff020203), cx + rc * 0.5f, cy + rc, false); g.setGradientFill (cg); g.fillEllipse (cx - rc, cy - rc, 2 * rc, 2 * rc);
        g.setColour (Colours::white.withAlpha (0.14f)); g.fillEllipse (cx - rc * 0.6f, cy - rc * 0.85f, rc * 1.0f, rc * 0.6f);
        g.setColour (Colour (0xfff3eee2).withAlpha (al)); g.drawLine (Line<float> (P (av, rc * 0.15f), P (av, rs - 1.5f)), 2.4f);
    }
    // lettura in un piccolo display
    auto rr = Rectangle<float> (4, getHeight() - rh, getWidth() - 8.0f, rh - 1);
    g.setColour (Colour (0xe6050608)); g.fillRoundedRectangle (rr, 4); g.setColour (col.withAlpha (enabled ? 0.45f : 0.15f)); g.drawRoundedRectangle (rr.reduced (0.5f), 4, 1.0f);
    g.setColour (enabled ? (drag ? col.brighter (0.4f) : col) : C::faint); g.setFont (mf (big ? 11.0f : 10.0f)); g.drawText (text ? text() : param->getCurrentValueAsText(), rr, Justification::centred);
}
void Knob::paint (juce::Graphics& g)
{
    if (hw >= 0) { paintHw (g); return; }
    const float lh = big ? 30.0f : 27.0f;
    const float size = juce::jmin ((float) getWidth(), getHeight() - lh);
    const float v = param->getValue();
    const auto c = enabled ? col : C::faint;
    jlook::KnobLook kl; kl.big = big; kl.bipolar = bipolar; kl.enabled = enabled; kl.hover = hover ? 1.0f : 0.0f; kl.active = drag ? 1.0f : 0.0f;
    jlook::drawKnob (g, { (getWidth() - size) * 0.5f, 0, size, size }, v, col, kl);
    g.setColour (C::dim); g.setFont (uf (big ? 10.5f : 9.5f)); g.drawText (label, juce::Rectangle<float> (0, size + 1, (float) getWidth(), 13), juce::Justification::centred);
    glowText (g, text ? text() : param->getCurrentValueAsText(), juce::Rectangle<float> (0, size + 14, (float) getWidth(), 13), enabled ? (drag ? c.brighter (0.4f) : c) : C::faint, mf (big ? 11.5f : 10.5f), juce::Justification::centred);
}

// ======================= Segs / Switch =======================
void Segs::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat(); lcd (g, b, 6);
    const auto it = items(); const int n = juce::jmax (1, it.size()), val = juce::roundToInt (param->convertFrom0to1 (param->getValue()));
    int cur = val; if (map) for (int k = 0; k < n; ++k) if (map (k) == val) cur = k;
    if (unmap) cur = unmap (val);
    const float w = (b.getWidth() - 6) / n;
    for (int i = 0; i < n; ++i)
    {
        auto r = juce::Rectangle<float> (3 + i * w, 3, w, b.getHeight() - 6).reduced (1, 0);
        if (i == cur) { g.setColour (col.withAlpha (0.18f)); g.fillRoundedRectangle (r, 4); g.setColour (col.withAlpha (0.85f)); g.drawRoundedRectangle (r, 4, 1.0f); glowText (g, it[i], r, col, uf (11.0f), juce::Justification::centred); }
        else { g.setColour (C::dim); g.setFont (uf (11.0f)); g.drawText (it[i], r, juce::Justification::centred); }
    }
    g.setColour (juce::Colours::black.withAlpha (neo::light() ? 0.03f : 0.15f)); for (float y = b.getY() + 2; y < b.getBottom(); y += 3) g.drawHorizontalLine ((int) y, b.getX() + 2, b.getRight() - 2);
}
void Segs::mouseDown (const juce::MouseEvent& e)
{
    const int n = juce::jmax (1, items().size()); const int k = juce::jlimit (0, n - 1, (int) ((e.position.x - 3) / ((getWidth() - 6) / (float) n)));
    param->setValueNotifyingHost (param->convertTo0to1 ((float) (map ? map (k) : k))); if (onTouch) onTouch(); repaint();
}
void Switch::paint (juce::Graphics& g)
{
    const bool on = param->getValue() > 0.5f; const float h = (float) juce::jmin (getHeight(), 20), w = h * 1.8f;
    auto t = juce::Rectangle<float> (0, (getHeight() - h) / 2, w, h);
    lcd (g, t, h / 2);
    const float d = h - 6, x = on ? t.getRight() - d - 3 : 3.0f;
    if (on) { g.setColour (col.withAlpha (0.35f)); g.fillEllipse (x - 3, t.getY(), d + 6, d + 6); }
    juce::ColourGradient lg (on ? col.brighter (0.6f) : juce::Colour (0xff4a4f5a), x, t.getY() + 3, on ? col : juce::Colour (0xff15171c), x, t.getY() + 3 + d, false); g.setGradientFill (lg); g.fillEllipse (x, t.getY() + 3, d, d);
    if (label.isNotEmpty()) { if (on) glowText (g, label, { w + 8, 0, getWidth() - w - 8, (float) getHeight() }, col, uf (11.0f)); else { g.setColour (C::dim); g.setFont (uf (11.0f)); g.drawText (label, juce::Rectangle<float> (w + 8, 0, getWidth() - w - 8, (float) getHeight()), juce::Justification::centredLeft); } }
}

// ======================= Rack degli stadi =======================
void Rack::mouseMove (const juce::MouseEvent& e)
{
    int h = -1; for (int s = 0; s < kStages; ++s) if (tile (s).contains (e.position)) h = s;
    if (h != hov) { hov = h; repaint(); }
    setMouseCursor (h >= 0 ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
}
void Rack::mouseDown (const juce::MouseEvent& e)
{
    for (int s = 0; s < kStages; ++s)
    {
        if (! tile (s).contains (e.position)) continue;
        auto* on = ed.P (juce::String (stagePfx (s)) + "on");
        const bool isOn = on->getValue() > 0.5f;
        if (powR (s).expanded (6).contains (e.position)) on->setValueNotifyingHost (isOn ? 0.0f : 1.0f);
        else if (isOn && soloR (s).contains (e.position)) { auto* so = ed.P (juce::String (stagePfx (s)) + "solo"); so->setValueNotifyingHost (so->getValue() > 0.5f ? 0.0f : 1.0f); }
        else if (! isOn) on->setValueNotifyingHost (1.0f);
        ed.select (s); repaint(); return;
    }
}
void Rack::tick()
{
    bool ch = false;
    for (int s = 0; s < kStages; ++s)
    {
        const float g = ed.proc.gr[s].load();
        meter[s] += (g - meter[s]) * (g > meter[s] ? 0.6f : 0.2f);
        if (std::abs (meter[s] - shown[s]) > 0.04f) ch = true;
    }
    if (ch) repaint();
}
void Rack::paint (juce::Graphics& g)
{
    juce::String key; key << skin::cur() << "/" << ed.focus << "/" << hov;
    for (int s = 0; s < kStages; ++s) key << "|" << (int) (ed.sv (s, "on") > 0.5f) << (int) (ed.sv (s, "solo") > 0.5f) << juce::roundToInt (ed.sv (s, "model"));
    bg.draw (g, getLocalBounds(), key, [this] (juce::Graphics& g2)
    {
        for (int s = 0; s < kStages; ++s)
        {
            auto t = tile (s); const bool on = ed.sv (s, "on") > 0.5f, sel = ed.focus == s, solo = on && ed.sv (s, "solo") > 0.5f;
            const int m = juce::roundToInt (ed.sv (s, "model")); const auto c = C::stage[s];
            jlook::softShadow (g2, t, 12, sel ? 20.0f : 12.0f, sel ? 0.7f : 0.5f, sel ? 8.0f : 4.0f);
            jlook::card (g2, t, 12, c, on, sel, false);
            if (on)
            {   // striscia col colore del frontale del modello
                const auto& F = face (m); auto st = juce::Rectangle<float> (t.getX() + 1, t.getY() + 1, t.getWidth() - 2, 5);
                juce::Path cl; cl.addRoundedRectangle (st.getX(), st.getY(), st.getWidth(), st.getHeight(), 4, 4, true, true, false, false);
                g2.setColour (juce::Colour (F.p1)); g2.fillPath (cl);
            }
            if (sel) { g2.setColour (c.withAlpha (0.85f)); g2.drawRoundedRectangle (t.reduced (1), 11, 1.6f); }
            else if (hov == s) { g2.setColour (neo::K::hi.withAlpha (0.18f)); g2.drawRoundedRectangle (t.reduced (1), 11, 1.0f); }
            auto pr = powR (s);
            if (on) { g2.setColour (c.withAlpha (0.35f)); g2.fillEllipse (pr.expanded (4)); }
            juce::ColourGradient lg (on ? c.brighter (0.6f) : juce::Colour (0xff3a3e48), pr.getCentreX(), pr.getY(), on ? c : juce::Colour (0xff15171c), pr.getCentreX(), pr.getBottom(), false);
            g2.setGradientFill (lg); g2.fillEllipse (pr);
            glowText (g2, juce::String (s + 1), { pr.getRight() + 6, pr.getY() - 2, 20, 22 }, on ? C::text : C::dim, mf (13.0f));
            if (! on)
            {
                g2.setColour (C::dim.withAlpha (hov == s ? 0.9f : 0.5f)); g2.setFont (uf (12.0f));
                g2.drawText ("+  AGGIUNGI", t.withTrimmedTop (30), juce::Justification::centred);
                continue;
            }
            glowText (g2, info (m).name, { t.getX() + 10, t.getY() + 30, t.getWidth() - 20, 20 }, neo::acc (juce::Colour (info (m).accent)), uf (13.5f));
            auto mb = juce::Rectangle<float> (t.getX() + 10, t.getY() + 56, t.getWidth() - 20, 8);
            g2.setColour (juce::Colours::black.withAlpha (0.6f)); g2.fillRoundedRectangle (mb, 4);
            jlook::button (g2, soloR (s), C::amber, solo, false, "SOLO", uf (10.0f));
        }
    });
    for (int s = 0; s < kStages; ++s)
    {
        shown[s] = meter[s];
        if (ed.sv (s, "on") < 0.5f) continue;
        auto t = tile (s); const auto c = C::stage[s];
        auto mb = juce::Rectangle<float> (t.getX() + 10, t.getY() + 56, t.getWidth() - 20, 8);
        const float w = juce::jlimit (0.0f, 1.0f, meter[s] / 20.0f) * mb.getWidth();
        if (w > 0.5f) { juce::ColourGradient mg (c, mb.getRight() - w, 0, c.brighter (0.6f), mb.getRight(), 0, false); g.setGradientFill (mg); g.fillRoundedRectangle (mb.withLeft (mb.getRight() - w), 4); }
        g.setColour (C::dim); g.setFont (mf (9.5f)); g.drawText ("GR " + juce::String (meter[s], 1), juce::Rectangle<float> (t.getX() + 10, t.getBottom() - 30, 70, 22), juce::Justification::centredLeft);
    }
}

// ======================= Editor dello stadio =======================
StageEditor::StageEditor (Editor& e) : ed (e)
{
    auto touch = [] {};
    power = std::make_unique<Switch> (ed.P ("a_on"), C::stage[0]);
    ratio = std::make_unique<Segs> (ed.P ("a_ratio"), C::stage[0]);
    ratio->items = [this] { return info (juce::roundToInt (ed.sv (s, "model"))).ratios; };
    auto mk = [&] (const char* id, const char* lab) { auto k = std::make_unique<Knob> (ed.P (juce::String ("a_") + id), C::stage[0]); k->label = lab; k->onTouch = touch; addAndMakeVisible (*k); return k; };
    drive = mk ("drive", "INPUT"); drive->big = true;
    drive2 = mk ("drive2", "INPUT R"); att2 = mk ("att2", "THRESHOLD R");
    drive2->text = [this] { return fmtDrive (VC670, ed.sv (s, "drive2")); };
    att2->text = [this] { return juce::String (t670Thr (ed.sv (s, "att2")), 1) + " dB"; };
    ratio2 = std::make_unique<Segs> (ed.P ("a_ratio2"), C::stage[0]); ratio2->items = [] { return info (VC670).ratios; }; addChildComponent (*ratio2);
    link = std::make_unique<Switch> (ed.P ("a_link"), C::stage[0], "LINK L/R"); addChildComponent (*link);
    olink = std::make_unique<Switch> (ed.P ("a_olink"), C::stage[0], "OUT LEGATO"); addChildComponent (*olink);
    link->onTouch = [this] { layoutKnobs(); repaint(); };
    att = mk ("att", "ATTACK"); rel = mk ("rel", "RELEASE"); out = mk ("out", "OUTPUT"); color = mk ("color", "COLOR"); mix = mk ("mix", "MIX");
    drive->text = [this] { return fmtDrive (juce::roundToInt (ed.sv (s, "model")), ed.sv (s, "drive")); };
    att->text = [this] { const int m = juce::roundToInt (ed.sv (s, "model")); if (m == VC670) return juce::String (t670Thr (ed.sv (s, "thr")), 1) + " dB"; return info (m).attKnob ? fmtTime (attMs (m, ed.sv (s, "att"))) : juce::String (m <= OPTO3A ? "OTTICO" : m == VARIMU ? "TC" : "AUTO"); };
    rel->text = [this] { const int m = juce::roundToInt (ed.sv (s, "model")); if (m == VC670) return juce::String (ed.sv (s, "rel") > 0.5f ? "LAT/VERT" : "L/R"); return info (m).relKnob ? fmtTime (relMs (m, ed.sv (s, "rel"))) : juce::String (m <= OPTO3A ? "OTTICO" : m == VARIMU ? "TC" : "120 dB/s"); };
    out->text = [this] { return fmtDb (ed.sv (s, "out")); };
    color->text = [this]
    {
        const float c = ed.sv (s, "color");
        if (juce::roundToInt (ed.sv (s, "model")) == DISTRESS) return c < 0.01f ? juce::String ("PULITO") : c < 0.5f ? "DIST2 " + juce::String (juce::roundToInt (c / 0.5f * 100)) + "%" : "DIST3 " + juce::String (juce::roundToInt ((c - 0.5f) / 0.5f * 100)) + "%";
        return c < 0.01f ? juce::String ("PULITO") : juce::String (juce::roundToInt (c * 100)) + "%";
    };
    mix->text = [this] { return juce::String (juce::roundToInt (ed.sv (s, "mix") * 100)) + "%"; };
    auto mkb = [&] (const char* lab) { auto k = std::make_unique<Knob> (ed.P ("mb_b1_thr"), C::stage[0]); k->label = lab; addAndMakeVisible (*k); return k; };
    bThr = mkb ("SOGLIA"); bRatio = mkb ("RATIO"); bAtt = mkb ("ATTACCO"); bRel = mkb ("RILASCIO"); bGain = mkb ("GUADAGNO"); bGain->bipolar = true;
    auto bp = [this] (const char* k) { return ed.proc.value (("mb_b" + juce::String (mbSel + 1) + "_" + k).toRawUTF8()); };
    bThr->text = [bp] { return juce::String (bp ("thr"), 1) + " dB"; };
    bRatio->text = [bp] { return juce::String (bp ("ratio"), 1) + ":1"; };
    bAtt->text = [bp] { return fmtTime (bp ("att")); };
    bRel->text = [bp] { return fmtTime (bp ("rel")); };
    bGain->text = [bp] { const float v = bp ("gain"); return (v > 0 ? "+" : "") + juce::String (v, 1) + " dB"; };
    addAndMakeVisible (*power); addAndMakeVisible (*ratio);
    bind (0);
}
bool StageEditor::isMb() const { return juce::roundToInt (ed.sv (s, "model")) == MULTI; }
void StageEditor::bindBand (int b)
{
    mbSel = juce::jlimit (0, 3, b); const juce::String p = "mb_b" + juce::String (mbSel + 1) + "_";
    bThr->param = ed.P (p + "thr"); bRatio->param = ed.P (p + "ratio"); bAtt->param = ed.P (p + "att"); bRel->param = ed.P (p + "rel"); bGain->param = ed.P (p + "gain");
    repaint();
}
void StageEditor::mbBounds (float* e, int& nb) const
{
    nb = 2 + juce::jlimit (0, 2, (int) ed.sv (s, "ratio"));
    const float x1 = ed.proc.value ("mb_x1"), x2 = ed.proc.value ("mb_x2"), x3 = ed.proc.value ("mb_x3");
    e[0] = 20;
    if (nb == 2) { e[1] = x2; e[2] = 20000; }
    else if (nb == 3) { e[1] = x1; e[2] = x3; e[3] = 20000; }
    else { e[1] = x1; e[2] = x2; e[3] = x3; e[4] = 20000; }
}
void StageEditor::layoutKnobs()
{
    drive->setVisible (true); ratio->setVisible (true);
    layoutInner();
    // stadio spento: niente comandi, solo il pannello e l'invito ad accenderlo
    lastOn = ed.sv (s, "on") > 0.5f;
    if (! lastOn) for (auto* c : getChildren()) if (c != power.get()) c->setVisible (false);
}
void StageEditor::layoutInner()
{
    const bool mb = isMb();
    for (auto* k : { bThr.get(), bRatio.get(), bAtt.get(), bRel.get(), bGain.get() }) k->setVisible (mb);
    out->setVisible (! mb); att->setVisible (! mb); rel->setVisible (! mb);
    const int m = juce::roundToInt (ed.sv (s, "model"));
    const bool is670 = m == VC670, split = is670 && ed.sv (s, "link") < 0.5f;
    drive2->setVisible (split); att2->setVisible (split); ratio2->setVisible (split); link->setVisible (is670);
    olink->setVisible (false);   // non serve piu': l'INPUT non alza il volume d'uscita
    link->setBounds (200, 213, 112, 20); olink->setBounds (getWidth() - 396, 13, 120, 22);
    ratio->setBounds (16, split ? 236 : 236, 330, split ? 24 : 30); ratio2->setBounds (16, 264, 330, 24);
    const int x0 = 370;
    // manopole da frontale: un po' piu' larghe del disegno per far stare gli estremi della scala, piu' alte per etichetta e display
    auto place = [] (Knob* k, int cx, int y, int sz) { if (k->hw >= 0) k->setBounds (cx - sz / 2 - 17, y, sz + 34, sz + 33); else k->setBounds (cx - sz / 2, y, sz, sz + 27); };
    if (mb)
    {
        const int cw = (getWidth() - x0 - 14) / 4;
        Knob* r1[4] = { bThr.get(), bRatio.get(), bAtt.get(), bRel.get() }; Knob* r2[4] = { bGain.get(), drive.get(), color.get(), mix.get() };
        for (int i = 0; i < 4; ++i) { place (r1[i], x0 + i * cw + cw / 2, 52, 82); place (r2[i], x0 + i * cw + cw / 2, 180, 76); }
        drive->big = false;
        return;
    }
    drive->big = true;
    const int cw = (getWidth() - x0 - 14) / 3;
    const bool hw = drive->hw >= 0;
    if (hw) place (drive.get(), x0 + cw / 2, 46, 100); else drive->setBounds (x0 + (cw - 104) / 2, 44, 104, 104 + 30);
    place (att.get(), x0 + cw + cw / 2, hw ? 56 : 52, 84); place (rel.get(), x0 + cw * 2 + cw / 2, hw ? 56 : 52, 84);
    place (out.get(), x0 + cw / 2, hw ? 184 : 186, 76); place (color.get(), x0 + cw + cw / 2, hw ? 184 : 186, 76); place (mix.get(), x0 + cw * 2 + cw / 2, hw ? 184 : 186, 76);
    color->setVisible (! split); mix->setVisible (! split);

    if (split)
    {   // 670 a canali separati: in alto il sinistro (o LAT), in basso il destro (o VERT), l'uscita in fondo a destra
        place (drive2.get(), x0 + cw / 2, 184, 76); place (att2.get(), x0 + cw + cw / 2, 184, 76); place (out.get(), x0 + cw * 2 + cw / 2, 184, 76);
    }
}
juce::RangedAudioParameter* StageEditor::thrParam()
{
    const int m = juce::roundToInt (ed.sv (s, "model"));
    if (m == OTT || m == MULTI) return nullptr;
    return ed.P (pfx + (m == VC670 ? "thr" : "drive"));
}
float StageEditor::thrDb() const
{
    const int m = juce::roundToInt (ed.sv (s, "model"));
    if (m == OTT) return -20.0f;
    return stageThrDb (m, ed.sv (s, "drive"), ed.sv (s, "thr"));
}
void StageEditor::mouseMove (const juce::MouseEvent& e)
{
    const bool t = ! isMb() && meterRect().contains (e.position) && juce::roundToInt (ed.sv (s, "model")) != OTT;
    setMouseCursor (t ? juce::MouseCursor::UpDownResizeCursor : (modelRect().contains (e.getPosition()) || soloRect().contains (e.getPosition())) ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
}
void StageEditor::mouseDrag (const juce::MouseEvent& e)
{
    if (thrDrag)
    {   // trascini la linea della soglia: la manopola segue
        const int m = juce::roundToInt (ed.sv (s, "model")); const float db = dbOfY (e.position.y);
        float v = m == VC670 ? (-db - ed.sv (s, "drive") * 18.0f) / 28.0f : -db / (isInputModel (m) ? 48.0f : 40.0f);
        if (auto* p = thrParam()) p->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, v));
        return;
    }
    if (xDrag < 0) return;
    static const char* ids[3] = { "mb_x1", "mb_x2", "mb_x3" };
    auto* pr = ed.P (ids[xDrag]); pr->setValueNotifyingHost (pr->convertTo0to1 (hzOf (e.position.x)));
    repaint();
}
void StageEditor::bind (int st)
{
    s = st; pfx = stagePfx (s); const auto c = C::stage[s];
    power->param = ed.P (pfx + "on"); power->col = c; ratio->param = ed.P (pfx + "ratio"); ratio->col = c;
    for (auto* k : { drive.get(), att.get(), rel.get(), out.get(), color.get(), mix.get() }) k->col = c;
    drive->param = ed.P (pfx + "drive"); att->param = ed.P (pfx + (juce::roundToInt (ed.sv (st, "model")) == VC670 ? "thr" : "att")); rel->param = ed.P (pfx + "rel");
    drive2->param = ed.P (pfx + "drive2"); att2->param = ed.P (pfx + "att2"); ratio2->param = ed.P (pfx + "ratio2"); link->param = ed.P (pfx + "link"); olink->param = ed.P (pfx + "olink");
    for (auto* k : { drive2.get(), att2.get() }) k->col = c; ratio2->col = c; link->col = c; olink->col = c;
    out->param = ed.P (pfx + "out"); color->param = ed.P (pfx + "color"); mix->param = ed.P (pfx + "mix");
    const int m = juce::roundToInt (ed.sv (s, "model")); const auto& I = info (m);
    drive->label = I.driveLabel; att->enabled = I.attKnob; rel->enabled = I.relKnob;
    att->stepped = rel->stepped = m == VCABUS || m == VC670; att->steps = 6; rel->steps = 5;
    if (m == VC670) { att->stepped = false; rel->steps = 2; }
    att->label = m == VC670 ? (ed.sv (s, "link") < 0.5f ? (ed.sv (s, "rel") > 0.5f ? "THRESH LAT" : "THRESHOLD L") : "THRESHOLD") : m == SSLCH ? "FAST ATTACK" : "ATTACK"; rel->label = m == VC670 ? "STEREO" : "RELEASE";
    if (m == VC670 && ed.sv (s, "link") < 0.5f) { drive->label = ed.sv (s, "rel") > 0.5f ? "INPUT LAT" : "INPUT L"; drive2->label = ed.sv (s, "rel") > 0.5f ? "INPUT VERT" : "INPUT R"; att2->label = ed.sv (s, "rel") > 0.5f ? "THRESH VERT" : "THRESHOLD R"; }
    if (m == SSLCH) { att->stepped = true; att->steps = 2; }
    for (auto* k : { bThr.get(), bRatio.get(), bAtt.get(), bRel.get(), bGain.get() }) k->col = c;
    if (! (m == VC670 && ed.sv (s, "link") < 0.5f)) drive->label = m == MULTI ? "SOGLIE +/-" : I.driveLabel;
    if (m == MULTI) { color->label = "WARM"; bindBand (mbSel); }
    layoutKnobs();
    color->label = m == MULTI ? "WARM" : m == DISTRESS ? "DIST" : m == FET76 ? "GRINTA" : (m <= OPTO3A || m == VARIMU || m == VC670) ? "VALVOLA" : "COLOR";
    out->label = m <= OPTO3A ? "GAIN" : (m == VCABUS || m == SSLCH) ? "MAKE-UP" : "OUTPUT";
    if (m == VC670 && ed.sv (s, "link") >= 0.5f) att->label = "THRESHOLD";
    // stile del frontale
    const auto& F = face (m);
    for (auto* k : { drive.get(), att.get(), rel.get(), out.get(), color.get(), mix.get(), drive2.get(), att2.get() }) { k->hw = F.knob; k->ink = juce::Colour (F.ink); k->lo = k->hi = {}; }
    drive->cap = juce::Colour (0xffd23b3b); att->cap = rel->cap = juce::Colour (0xff3f7fd6); out->cap = juce::Colour (0xff9aa0a8); color->cap = juce::Colour (0xffe0a020); mix->cap = juce::Colour (0xff6f8496);
    drive2->cap = drive->cap; att2->cap = att->cap;
    auto ends = [] (Knob* k, const char* a, const char* b) { k->lo = a; k->hi = b; };
    if (m == MULTI) ends (drive.get(), "-12", "+12"); else if (m == OTT) ends (drive.get(), "0", "100%");
    else if (m <= OPTO3A) ends (drive.get(), "0", "100"); else if (m == VC670) { ends (drive.get(), "0", "+18"); ends (drive2.get(), "0", "+18"); ends (att2.get(), "0", "-28"); }
    else if (isInputModel (m)) ends (drive.get(), "0", "+48"); else ends (drive.get(), "0", "-40");
    if (m == FET76) { ends (att.get(), "20us", "0.8"); ends (rel.get(), "50", "1.1s"); }
    else if (m == VCABUS) { ends (att.get(), ".1", "30"); ends (rel.get(), ".1", "AUTO"); }
    else if (m == DISTRESS) { ends (att.get(), ".05", "30"); ends (rel.get(), "50", "3.5s"); }
    else if (m == SSLCH) { ends (att.get(), "AUTO", "FAST"); ends (rel.get(), ".1", "4s"); }
    else if (m == OTT) { ends (att.get(), "x0.25", "x4"); ends (rel.get(), "x0.25", "x4"); }
    else if (m == VC670) { ends (att.get(), "0", "-28"); ends (rel.get(), "L/R", "M/S"); }
    ends (out.get(), "-12", "+24"); ends (color.get(), "0", "100"); ends (mix.get(), "0", "100");
    for (auto* k : { drive.get(), att.get(), rel.get() }) if (! k->enabled) k->lo = k->hi = {};
    layoutKnobs();
    lastModel = m; bg.invalidate(); repaint();
}
void StageEditor::resized()
{
    power->setBounds (16, 13, 38, 22);
    ratio->setBounds (16, 236, 330, 30);
    layoutKnobs();
}
void StageEditor::mouseDown (const juce::MouseEvent& e)
{
    if (isMb() && vuRect().contains (e.position))
    {
        float edg[5]; int nb; mbBounds (edg, nb);
        // tagli trascinabili
        static const int xid2[1] = { 1 }, xid3[2] = { 0, 2 }, xid4[3] = { 0, 1, 2 };
        const int* xid = nb == 2 ? xid2 : nb == 3 ? xid3 : xid4;
        for (int k = 1; k < nb; ++k) if (std::abs (e.position.x - xOf (edg[k])) < 6) { xDrag = xid[k - 1]; return; }
        for (int b = 0; b < nb; ++b)
        {
            const float a = xOf (edg[b]), z = xOf (edg[b + 1]); if (e.position.x < a || e.position.x >= z) continue;
            auto r = vuRect(); const float by = r.getBottom() - 22;
            if (e.position.y > by)
            {
                const juce::String p = "mb_b" + juce::String (b + 1) + "_";
                auto* t = ed.P (p + (e.position.x < (a + z) / 2 ? "solo" : "byp")); t->setValueNotifyingHost (t->getValue() > 0.5f ? 0.0f : 1.0f);
            }
            bindBand (b); return;
        }
        return;
    }
    if (! isMb() && meterRect().contains (e.position))
    {
        if (auto* p = thrParam()) { thrDrag = true; p->beginChangeGesture(); mouseDrag (e); }
        return;
    }
    if (soloRect().contains (e.getPosition())) { auto* so = ed.P (pfx + "solo"); so->setValueNotifyingHost (so->getValue() > 0.5f ? 0.0f : 1.0f); repaint(); ed.rack.repaint(); return; }
    if (modelRect().contains (e.getPosition()))
    {
        juce::PopupMenu m; auto* pr = dynamic_cast<juce::AudioParameterChoice*> (ed.P (pfx + "model"));
        m.addSectionHeader ("MODELLO");
        for (int i = 0; i < kNumModels; ++i) m.addItem (i + 1, juce::String (info (i).name) + "   -   " + info (i).inspired, true, i == pr->getIndex());
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withTargetScreenArea (localAreaToGlobal (modelRect())), [this, pr] (int r)
        {
            if (r <= 0) return;
            const int from = pr->getIndex(), to = r - 1;
            auto set = [this] (const char* k, float v) { if (auto* q = ed.P (pfx + k)) q->setValueNotifyingHost (q->convertTo0to1 (v)); };
            pr->setValueNotifyingHost (pr->convertTo0to1 ((float) to));
            auto* rp = ed.P (pfx + "ratio"); const int n = info (to).ratios.size();
            if (rp->convertFrom0to1 (rp->getValue()) > n - 1) rp->setValueNotifyingHost (rp->convertTo0to1 ((float) (n - 1)));
            // cambiando modello si riparte neutri dove il significato delle manopole cambia
            if (to == MULTI && from != MULTI) set ("drive", 0.3f);                 // SOGLIE +/- al centro
            if (from == MULTI && to != MULTI) set ("drive", 0.0f);
            if (from == OTT && to != OTT) set ("drive", 0.0f);
            if (to == OTT && from != OTT) set ("drive", 0.0f);
            if (from == VC670 && to != VC670) set ("rel", 0.35f);   // la manopola STEREO del 670 e' il RELEASE degli altri
            if (to == DISTRESS && rp->convertFrom0to1 (rp->getValue()) < 0.5f) set ("ratio", 3.0f);   // 1:1 non comprime: si parte da 4:1
            bind (s); ed.rack.repaint();
        });
    }
}
void StageEditor::paintStatic (juce::Graphics& g)
{
    const int m = juce::roundToInt (ed.sv (s, "model"));
    const auto& I = info (m); const auto& F = face (m); const bool on = ed.sv (s, "on") > 0.5f;
    const auto acc = neo::acc (juce::Colour (I.accent)), ink = juce::Colour (F.ink);
    auto b = getLocalBounds().toFloat().reduced (1);
    // cornice esterna scura e pannello del modello
    g.setColour (juce::Colour (0xff060709)); g.fillRoundedRectangle (b, 14);
    auto pl = b.reduced (5);
    { juce::ColourGradient pg (juce::Colour (F.p1), 0, pl.getY(), juce::Colour (F.p2), 0, pl.getBottom(), false); g.setGradientFill (pg); g.fillRoundedRectangle (pl, 10); }
    {   // spazzolatura orizzontale e luce dall'alto
        g.saveState(); g.reduceClipRegion (pl.toNearestInt());
        juce::Random rnd (7 + m);
        for (float y = pl.getY(); y < pl.getBottom(); y += 1.5f) { g.setColour ((rnd.nextBool() ? juce::Colours::white : juce::Colours::black).withAlpha (0.018f + rnd.nextFloat() * 0.02f)); g.drawHorizontalLine ((int) y, pl.getX(), pl.getRight()); }
        juce::ColourGradient sh (juce::Colours::white.withAlpha (0.10f), 0, pl.getY(), juce::Colours::transparentWhite, 0, pl.getY() + 90, false); g.setGradientFill (sh); g.fillRect (pl);
        g.restoreState();
    }
    g.setColour (juce::Colours::black.withAlpha (0.5f)); g.drawRoundedRectangle (pl, 10, 1.2f);
    // binario superiore scuro con i comandi
    auto rail = pl.withHeight (38);
    { juce::Path rp; rp.addRoundedRectangle (rail.getX(), rail.getY(), rail.getWidth(), rail.getHeight(), 10, 10, true, true, false, false);
      g.setColour (juce::Colour (0xe0090a0d)); g.fillPath (rp); g.setColour (acc.withAlpha (0.55f)); g.fillRect (rail.getX() + 10, rail.getBottom() - 1, rail.getWidth() - 20, 1.0f); }
    for (auto x : { pl.getX() + 12, pl.getRight() - 12 }) screw (g, x, pl.getBottom() - 12, 5.0f, 0.4f + x * 0.01f);
    cyberFrame (g, b.reduced (1.5f), acc);
    // targhetta del modello incisa sul pannello
    if (! (m == VC670 && ed.sv (s, "link") < 0.5f))
    {
        const float y = 274; auto r = juce::Rectangle<float> (30, y, 300, 16);
        g.setFont (jlook::ui (13.0f, 2)); g.setColour (juce::Colours::black.withAlpha (0.25f)); g.drawText (juce::String (I.name) + "   " + F.plate, r.translated (0, 1), juce::Justification::centredLeft);
        g.setColour (ink.withAlpha (0.9f)); g.drawText (juce::String (I.name) + "   " + F.plate, r, juce::Justification::centredLeft);
        g.setFont (mf (8.0f)); g.setColour (ink.withAlpha (0.5f)); g.drawText (juce::String ("JC-6 / STAGE 0") + juce::String (s + 1) + "   " + F.code, r.translated (0, 13), juce::Justification::centredLeft);
    }
    glowText (g, "STADIO " + juce::String (s + 1), { 62, 10, 110, 28 }, on ? C::text : C::dim, uf (14.0f));
    g.setColour (C::dim); g.setFont (uf (11.0f, false)); g.drawFittedText (I.inspired, juce::Rectangle<int> (170, 10, (int) b.getWidth() - 470, 28), juce::Justification::centredLeft, 2, 0.8f);
    jlook::button (g, modelRect().toFloat(), acc, true, false, juce::String (I.name) + juce::String::fromUTF8 ("  \xe2\x96\xbe"), uf (13.0f));
    jlook::button (g, soloRect().toFloat(), C::amber, ed.sv (s, "solo") > 0.5f, false, "SOLO", uf (11.5f));
    if (m != MULTI)
    {
        // VU della riduzione: parte fissa
        auto vu = vuRect();
        g.setColour (juce::Colour (0xff0a0b0e)); g.fillRoundedRectangle (vu.expanded (5), 11);
        { juce::ColourGradient fr (juce::Colour (0xff3a3e48), 0, vu.getY() - 5, juce::Colour (0xff121419), 0, vu.getBottom() + 5, false); g.setGradientFill (fr); g.drawRoundedRectangle (vu.expanded (4), 10, 2.0f); }
        const auto fc = juce::Colour (I.face); const bool light = fc.getPerceivedBrightness() > 0.5f;
        juce::ColourGradient fg (fc.brighter (0.15f), vu.getCentreX(), vu.getY(), fc.darker (0.25f), vu.getCentreX(), vu.getBottom(), false); g.setGradientFill (fg); g.fillRoundedRectangle (vu, 8);
        juce::ColourGradient lamp (juce::Colour (0xffffc46b).withAlpha (light ? 0.38f : 0.14f), vu.getCentreX(), vu.getBottom(), juce::Colours::transparentBlack, vu.getCentreX(), vu.getY(), true); g.setGradientFill (lamp); g.fillRoundedRectangle (vu, 8);
        const auto vink = light ? juce::Colour (0xff1b1712) : juce::Colour (0xffe9e4d8);
        const float pcx = vu.getCentreX(), pcy = vu.getBottom() + 44, rad = vu.getHeight() + 10;
        auto angOf = [] (float grDb) { const float t = 1.0f - std::sqrt (juce::jlimit (0.0f, 20.0f, grDb) / 20.0f); return -0.66f + t * 1.32f; };
        juce::Path arc; arc.addCentredArc (pcx, pcy, rad - 22, rad - 22, 0, angOf (20), angOf (0), true);
        g.setColour (vink.withAlpha (0.85f)); g.strokePath (arc, juce::PathStrokeType (1.3f));
        juce::Path red; red.addCentredArc (pcx, pcy, rad - 20, rad - 20, 0, angOf (20), angOf (10), true); g.setColour (juce::Colour (0xffd22b2b).withAlpha (0.85f)); g.strokePath (red, juce::PathStrokeType (3.5f));
        for (int db : { 0, 1, 2, 3, 5, 7, 10, 15, 20 })
        {
            const float a = angOf ((float) db), sx = std::sin (a), cy2 = -std::cos (a);
            g.setColour (vink); g.drawLine (pcx + sx * (rad - 22), pcy + cy2 * (rad - 22), pcx + sx * (rad - 31), pcy + cy2 * (rad - 31), 1.4f);
            if (db == 1 || db == 2 || db == 7 || db == 15) continue;
            g.setFont (mf (10.0f)); g.drawText (juce::String (db == 0 ? 0 : -db), juce::Rectangle<float> (pcx + sx * (rad - 9) - 13, pcy + cy2 * (rad - 9) - 6, 26, 12), juce::Justification::centred);
        }
        g.setColour (vink.withAlpha (0.75f)); g.setFont (uf (10.0f)); g.drawText ("GAIN REDUCTION", vu.withTrimmedBottom (16).withTrimmedTop (vu.getHeight() - 34), juce::Justification::centred);
        g.setFont (mf (10.0f)); g.drawText ("dB", vu.reduced (12, 8), juce::Justification::topLeft);
        // misuratore d'ingresso con la soglia: parte fissa (scala)
        auto mr = meterRect();
        g.setColour (juce::Colour (0xff0a0b0e)); g.fillRoundedRectangle (mr.expanded (5), 11);
        lcd (g, mr, 8);
        g.setFont (mf (8.0f));
        for (int db = 0; db >= -50; db -= 10)
        {
            const float y = yOfDb ((float) db);
            g.setColour (C::phos.withAlpha (0.16f)); g.drawHorizontalLine ((int) y, mr.getX() + 20, mr.getRight() - 4);
            g.setColour (C::dim); g.drawText (juce::String (db), juce::Rectangle<float> (mr.getX() + 2, y - 5, 18, 10), juce::Justification::centredRight);
        }
    }
    g.setColour (ink.withAlpha (0.85f)); g.setFont (uf (10.5f)); g.drawText (I.ratioLabel, juce::Rectangle<float> (16, 214, 200, 20), juce::Justification::centredLeft);
    if (! on)
    {
        g.setColour (juce::Colour (0xd0060709)); g.fillRoundedRectangle (b.withTrimmedTop (44).reduced (5, 0).withTrimmedBottom (5), 10);
        g.setColour (C::text); g.setFont (uf (15.0f)); g.drawText ("STADIO SPENTO", b.withTrimmedTop (44).translated (0, -14), juce::Justification::centred);
        g.setColour (C::dim); g.setFont (uf (11.5f, false)); g.drawText ("accendilo con il pulsante in alto a sinistra: parte neutro, senza comprimere. Poi abbassa la SOGLIA.", b.withTrimmedTop (44).translated (0, 12), juce::Justification::centred);
    }
}
void StageEditor::paintNeedle (juce::Graphics& g)
{
    const int m = juce::roundToInt (ed.sv (s, "model")); const auto& I = info (m);
    auto vu = vuRect(); const auto fc = juce::Colour (I.face); const bool light = fc.getPerceivedBrightness() > 0.5f; const auto c = C::stage[s];
    const float pcx = vu.getCentreX(), pcy = vu.getBottom() + 44, rad = vu.getHeight() + 10;
    auto angOf = [] (float grDb) { const float t = 1.0f - std::sqrt (juce::jlimit (0.0f, 20.0f, grDb) / 20.0f); return -0.66f + t * 1.32f; };
    glowText (g, needle < 0.05f ? juce::String ("0.0") : "-" + juce::String (needle, 1), vu.reduced (12, 8), light ? juce::Colour (0xff8a1e1e) : c, mf (12.0f), juce::Justification::topRight);
    const float a = angOf (needle); g.saveState(); g.reduceClipRegion (vu.toNearestInt());
    g.setColour (juce::Colours::black.withAlpha (0.25f)); g.drawLine (pcx + 4, pcy, pcx + 4 + std::sin (a) * (rad - 16), pcy - std::cos (a) * (rad - 16), 2.2f);
    g.setColour (light ? juce::Colour (0xff111111) : juce::Colour (0xffff4040)); g.drawLine (pcx, pcy, pcx + std::sin (a) * (rad - 16), pcy - std::cos (a) * (rad - 16), 1.7f);
    g.restoreState();
    jlook::glass (g, vu, 8);
}
void StageEditor::paintMeter (juce::Graphics& g)
{
    const int m = juce::roundToInt (ed.sv (s, "model")); const auto c = C::stage[s];
    auto mr = meterRect();
    const float lev = juce::Decibels::gainToDecibels (ed.proc.stIn[s].load(), -60.0f);
    const float thr = thrDb(), knee = m == OTT ? 0.0f : kneeDb (m, juce::roundToInt (ed.sv (s, "ratio")));
    const float top = yOfDb (0), bot = yOfDb (-50);
    auto bar = juce::Rectangle<float> (mr.getX() + 24, top, 12, bot - top);
    g.setColour (juce::Colours::black.withAlpha (0.5f)); g.fillRect (bar);
    if (lev > -50)
    {
        const float yl = yOfDb (lev), yt = yOfDb (thr);
        g.setColour (C::phos.withAlpha (0.85f)); g.fillRect (bar.withTop (juce::jmax (yl, yt)));
        if (yl < yt) { g.setColour (C::red); g.fillRect (bar.withTop (yl).withBottom (yt)); }   // la parte che viene compressa
    }
    // zona del ginocchio e linea della soglia
    const float yt = yOfDb (thr);
    if (knee > 0.6f) { const float yk = yOfDb (thr + knee); g.setColour (c.withAlpha (0.13f)); g.fillRect (juce::Rectangle<float> (mr.getX() + 20, juce::jmin (yk, yt), mr.getWidth() - 24, std::abs (yt - yk))); }
    const bool dragable = m != OTT;
    g.setColour (c.withAlpha (0.35f)); g.fillRect (mr.getX() + 18, yt - 2.0f, mr.getWidth() - 20, 4.0f);
    g.setColour (c); g.fillRect (mr.getX() + 18, yt - 0.75f, mr.getWidth() - 20, 1.5f);
    if (dragable) { juce::Path tri; tri.addTriangle (mr.getRight() - 2, yt - 6, mr.getRight() - 2, yt + 6, mr.getRight() - 11, yt); g.fillPath (tri); }
    if (m == OTT && ed.sv (s, "drive") > 0.01f)
    {   // OTT: sopra -20 dB spinge giu', sotto -40 dB tira su
        const float yu = yOfDb (-40); g.setColour (C::green.withAlpha (0.8f)); g.fillRect (mr.getX() + 18, yu - 0.75f, mr.getWidth() - 20, 1.5f);
        g.setFont (mf (7.5f)); g.drawText ("SU", juce::Rectangle<float> (mr.getX() + 38, yu - 10, 30, 9), juce::Justification::centredLeft);
        g.setColour (c); g.drawText (juce::String::fromUTF8 ("GI\xc3\x99"), juce::Rectangle<float> (mr.getX() + 38, yt - 10, 30, 9), juce::Justification::centredLeft);
    }
    g.setFont (uf (8.5f)); g.setColour (C::dim); g.drawText ("SOGLIA", juce::Rectangle<float> (mr.getX(), mr.getY() + 1, mr.getWidth(), 12), juce::Justification::centred);
    const bool off = m != OTT && thr > -0.05f;
    g.setFont (mf (10.0f)); g.setColour (off ? C::dim : c);
    g.drawText (m == OTT ? juce::String (juce::roundToInt (ed.sv (s, "drive") * 100)) + "%" : off ? juce::String ("0 dB") : juce::String (thr, 1), juce::Rectangle<float> (mr.getX(), mr.getBottom() - 13, mr.getWidth(), 12), juce::Justification::centred);
    jlook::glass (g, mr, 8);
}
void StageEditor::tick()
{
    const float gv = ed.proc.gr[s].load();
    needle += (gv - needle) * (gv > needle ? 0.5f : 0.18f);
    const float lev = juce::Decibels::gainToDecibels (ed.proc.stIn[s].load(), -60.0f);
    if (isMb()) { repaint (vuRect().toNearestInt().expanded (4)); return; }
    if (std::abs (needle - needleShown) > 0.03f) { needleShown = needle; repaint (vuRect().toNearestInt().expanded (2)); }
    if (std::abs (lev - inShown) > 0.25f || thrDrag) { inShown = lev; repaint (meterRect().toNearestInt().expanded (2)); }
}
void StageEditor::paint (juce::Graphics& g)
{
    const int m = juce::roundToInt (ed.sv (s, "model")); if (m != lastModel || (ed.sv (s, "on") > 0.5f) != lastOn) bind (s);
    const bool split = m == VC670 && ed.sv (s, "link") < 0.5f;
    juce::String key; key << skin::cur() << "/" << s << "/" << m << "/" << (int) (ed.sv (s, "on") > 0.5f) << (int) (ed.sv (s, "solo") > 0.5f) << (int) split << "/" << getWidth();
    bg.draw (g, getLocalBounds(), key, [this] (juce::Graphics& g2) { paintStatic (g2); });
    if (ed.sv (s, "on") < 0.5f) return;
    if (m == MULTI) paintMb (g);
    else
    {
        if (g.clipRegionIntersects (vuRect().toNearestInt().expanded (2))) paintNeedle (g);
        if (g.clipRegionIntersects (meterRect().toNearestInt().expanded (2))) paintMeter (g);
    }
}
void StageEditor::paintMb (juce::Graphics& g)
{
        auto r = vuRect(); lcd (g, r, 8);
        float edg[5]; int nb; mbBounds (edg, nb);
        for (float hz : { 100.0f, 1000.0f, 10000.0f }) { g.setColour (C::phos.withAlpha (0.1f)); g.drawVerticalLine ((int) xOf (hz), r.getY(), r.getBottom()); g.setColour (C::faint); g.setFont (mf (8.5f)); g.drawText (hz >= 1000 ? juce::String ((int) hz / 1000) + "k" : juce::String ((int) hz), juce::Rectangle<float> (xOf (hz) + 2, r.getY() + 2, 30, 10), juce::Justification::centredLeft); }
        static const juce::uint32 bc[4] = { 0xffff7a3d, 0xffffd23f, 0xff7dff5a, 0xff5ef2ff };
        for (int b = 0; b < nb; ++b)
        {
            const float a = xOf (edg[b]), z = xOf (edg[b + 1]); const auto col = juce::Colour (bc[b]); const bool sel = b == mbSel;
            const juce::String p = "mb_b" + juce::String (b + 1) + "_";
            const bool so = ed.proc.value ((p + "solo").toRawUTF8()) > 0.5f, by = ed.proc.value ((p + "byp").toRawUTF8()) > 0.5f;
            auto br = juce::Rectangle<float> (a + 1, r.getY() + 14, z - a - 2, r.getHeight() - 38);
            g.setColour (col.withAlpha (sel ? 0.16f : 0.06f)); g.fillRect (br);
            const float grv = ed.proc.mbGr[b].load(), h = juce::jlimit (0.0f, 1.0f, grv / 18.0f) * br.getHeight();
            if (! by) { g.setColour (col.withAlpha (0.75f)); g.fillRect (br.withHeight (h).reduced (br.getWidth() * 0.25f, 0)); }
            g.setColour (sel ? col : col.withAlpha (0.6f)); g.setFont (mf (9.0f));
            g.drawText (by ? juce::String ("BYPASS") : "-" + juce::String (grv, 1), br.withTrimmedTop (br.getHeight() - 14), juce::Justification::centred);
            g.drawText (juce::String (ed.proc.value ((p + "thr").toRawUTF8()), 0) + " / " + juce::String (ed.proc.value ((p + "ratio").toRawUTF8()), 1), br.withHeight (12).translated (0, h > 14 ? h - 13 : 2), juce::Justification::centred);
            auto bb = juce::Rectangle<float> (a + 2, r.getBottom() - 21, (z - a - 4) / 2 - 1, 17);
            jlook::button (g, bb, C::amber, so, false, "S", uf (9.5f)); jlook::button (g, bb.translated (bb.getWidth() + 2, 0), C::red, by, false, "B", uf (9.5f));
            if (sel) { g.setColour (col); g.drawRect (br, 1.5f); }
        }
        for (int k = 1; k < nb; ++k) { const float x = xOf (edg[k]); g.setColour (C::text.withAlpha (0.8f)); g.fillRect (x - 1, r.getY(), 2.0f, r.getHeight() - 24); g.setFont (mf (8.5f)); g.drawText (edg[k] >= 1000 ? juce::String (edg[k] / 1000, 1) + "k" : juce::String ((int) edg[k]), juce::Rectangle<float> (x - 20, r.getY() + 2, 40, 10), juce::Justification::centred); }
        g.setColour (C::dim); g.setFont (uf (10.0f)); g.drawText ("BANDA " + juce::String (mbSel + 1) + ": clic per scegliere, trascina i tagli, S solo, B bypass", juce::Rectangle<float> (16, 202, 340, 12), juce::Justification::centredLeft);
    }

// ======================= Grafico =======================
void Graph::paint (juce::Graphics& g)
{
    auto& P = ed.proc; auto b = getLocalBounds().toFloat().reduced (1);
    auto in = b.reduced (14, 0).withTrimmedTop (32).withTrimmedBottom (14);
    auto tr = in.removeFromRight (in.getHeight()); in.removeFromRight (12);
    auto yLev = [&in] (float db) { return in.getBottom() - juce::jlimit (0.0f, 1.0f, (db + 48.0f) / 48.0f) * in.getHeight(); };
    auto yGr = [&in] (float gr) { return in.getY() + juce::jlimit (0.0f, 1.0f, gr / 24.0f) * in.getHeight() * 0.8f; };
    auto tx = [&tr] (float db) { return tr.getX() + (db + 60.0f) / 60.0f * tr.getWidth(); };
    auto ty = [&tr] (float db) { return tr.getBottom() - (db + 60.0f) / 60.0f * tr.getHeight(); };
    // parte fissa in cache: card, titoli, schermi, griglie
    bg.draw (g, getLocalBounds(), juce::String (skin::cur()) + "/" + juce::String (getWidth()), [&] (juce::Graphics& g2)
    {
        jlook::card (g2, b, 14, C::magenta, true, false, false);
        glowText (g2, "DINAMICA", { 18, 8, 160, 20 }, C::text, uf (12.0f));
        g2.setFont (mf (9.5f)); g2.setColour (C::dim);
        g2.drawText ("grigio = ingresso   ciano = uscita   colori = riduzione di ogni stadio", juce::Rectangle<float> (130, 8, b.getWidth() - 340, 20), juce::Justification::centredLeft);
        g2.drawText ("SOGLIA E CURVA", juce::Rectangle<float> (tr.getX(), 8, tr.getWidth(), 20), juce::Justification::centredRight);
        lcd (g2, in, 6);
        for (int db : { -6, -12, -24, -36 }) { g2.setColour (C::phos.withAlpha (0.06f)); g2.drawHorizontalLine ((int) yLev ((float) db), in.getX(), in.getRight()); g2.setColour (C::faint); g2.setFont (mf (8.5f)); g2.drawText (juce::String (db), juce::Rectangle<float> (in.getX() + 3, yLev ((float) db) - 11, 30, 10), juce::Justification::centredLeft); }
        lcd (g2, tr, 6);
        g2.setColour (C::phos.withAlpha (0.08f)); for (int d = -48; d < 0; d += 12) { g2.drawVerticalLine ((int) tx ((float) d), tr.getY(), tr.getBottom()); g2.drawHorizontalLine ((int) ty ((float) d), tr.getX(), tr.getRight()); }
        g2.setColour (neo::K::hi.withAlpha (0.15f)); g2.drawLine (tr.getX(), tr.getBottom(), tr.getRight(), tr.getY(), 1.0f);
        g2.setColour (C::faint); g2.setFont (mf (8.0f)); g2.drawText ("IN dB", tr.reduced (5, 3), juce::Justification::bottomRight);
    });
    const int NH = Processor::kHist, w0 = P.hW.load();
    auto at = [w0, NH] (int k) { return (w0 + k) % NH; };
    const float dx = in.getWidth() / (NH - 1);
    {
        juce::Path pin; pin.startNewSubPath (in.getX(), in.getBottom());
        for (int k = 0; k < NH; k += 2) pin.lineTo (in.getX() + k * dx, yLev (P.hIn[at (k)].load()));
        pin.lineTo (in.getRight(), in.getBottom()); pin.closeSubPath();
        g.setColour (neo::K::hi.withAlpha (0.12f)); g.fillPath (pin);
    }
    juce::Path pout; for (int k = 0; k < NH; k += 2) { const float y = yLev (P.hOut[at (k)].load()); if (k == 0) pout.startNewSubPath (in.getX(), y); else pout.lineTo (in.getX() + k * dx, y); }
    g.setColour (C::phos.withAlpha (0.3f)); g.strokePath (pout, juce::PathStrokeType (3.0f)); g.setColour (C::phos); g.strokePath (pout, juce::PathStrokeType (1.2f));
    for (int st = 0; st < kStages; ++st)
    {
        if (ed.sv (st, "on") < 0.5f) continue;
        juce::Path pg; pg.startNewSubPath (in.getX(), in.getY()); float mx = 0;
        for (int k = 0; k < NH; k += 2) { const float v = P.hGr[st][at (k)].load(); mx = juce::jmax (mx, v); pg.lineTo (in.getX() + k * dx, yGr (v)); }
        if (mx < 0.05f) continue;
        pg.lineTo (in.getRight(), in.getY()); pg.closeSubPath();
        const auto c = C::stage[st];
        g.setColour (c.withAlpha (st == ed.focus ? 0.22f : 0.08f)); g.fillPath (pg); g.setColour (c.withAlpha (st == ed.focus ? 0.95f : 0.5f)); g.strokePath (pg, juce::PathStrokeType (st == ed.focus ? 1.5f : 1.0f));
    }
    // soglia dello stadio selezionato anche sulla storia del livello
    const auto sp = P.stageParams (ed.focus); const auto c = C::stage[ed.focus];
    const bool hasThr = sp.on && sp.model != MULTI && (sp.model != OTT || sp.drive > 0.01f);
    const float thr = sp.model == OTT ? -20.0f : stageThrDb (sp.model, sp.drive, sp.thr670);
    if (hasThr && thr > -48 && thr < -0.05f)
    {
        const float y = yLev (thr); const float dl[] = { 5, 4 };
        g.setColour (c.withAlpha (0.8f)); g.drawDashedLine ({ in.getX(), y, in.getRight(), y }, dl, 2, 1.0f);
        g.setFont (mf (8.5f)); g.drawText ("SOGLIA " + juce::String (ed.focus + 1) + "  " + juce::String (thr, 1), juce::Rectangle<float> (in.getRight() - 130, y - 12, 124, 11), juce::Justification::centredRight);
    }
    g.setColour (C::magenta); g.fillRect (in.getRight() - 2, in.getY(), 2.0f, in.getHeight());
    // curva di trasferimento dello stadio selezionato
    const float ig = inputGainDb (sp);
    juce::Path cv;
    for (int k = 0; k <= 120; ++k) { const float x = -60.0f + k * 0.5f, o = juce::jlimit (-60.0f, 0.0f, x - gainComputer (sp, x + ig)); if (k == 0) cv.startNewSubPath (tx (x), ty (o)); else cv.lineTo (tx (x), ty (o)); }
    g.setColour (c.withAlpha (0.3f)); g.strokePath (cv, juce::PathStrokeType (4.0f)); g.setColour (c); g.strokePath (cv, juce::PathStrokeType (1.6f));
    if (hasThr && thr > -60 && thr < -0.05f)
    {
        const float x = tx (thr); const float dl[] = { 4, 3 };
        g.setColour (c.withAlpha (0.7f)); g.drawDashedLine ({ x, tr.getY(), x, tr.getBottom() }, dl, 2, 1.0f);
        g.setFont (mf (8.5f)); g.drawText (juce::String (thr, 1), juce::Rectangle<float> (x + 3, tr.getBottom() - 24, 40, 10), juce::Justification::centredLeft);
    }
    const float lin = juce::Decibels::gainToDecibels (P.stIn[ed.focus].load(), -60.0f);
    if (sp.on && lin > -59) { const float o = juce::jlimit (-60.0f, 0.0f, lin - gainComputer (sp, lin + ig)); g.setColour (c.withAlpha (0.3f)); g.fillEllipse (tx (lin) - 7, ty (o) - 7, 14, 14); g.setColour (juce::Colours::white); g.fillEllipse (tx (lin) - 3, ty (o) - 3, 6, 6); }
    g.setColour (C::dim); g.setFont (mf (8.5f)); g.drawText (juce::String (info (sp.model).name) + "  " + juce::String (ed.focus + 1), tr.reduced (6, 4), juce::Justification::topLeft);
    // righe e vetro sopra, anch'essi in cache (trasparenti)
    ov.draw (g, getLocalBounds(), juce::String (skin::cur()) + "/" + juce::String (getWidth()), [&] (juce::Graphics& g2)
    {
        g2.setColour (juce::Colours::black.withAlpha (neo::light() ? 0.03f : 0.16f)); for (float y = in.getY(); y < in.getBottom(); y += 3) g2.drawHorizontalLine ((int) y, in.getX(), in.getRight());
        jlook::glass (g2, in, 6); jlook::glass (g2, tr, 6);
    });
}

// ======================= Guida =======================
void GuidePanel::mouseDown (const juce::MouseEvent& e)
{
    auto& P = ed.proc;
    if (e.position.y < 40 && e.position.x < 16 + 4 * kTabW)
    {
        const int t = juce::jlimit (0, 3, (int) ((e.position.x - 16) / kTabW));
        if (t != tab) { tab = t; ed.resized(); }   // ASCOLTO si allarga su tutta la fascia in basso
        repaint(); return;
    }
    if (tab != 3) return;
    for (int i = 0; i < kNumSrc; ++i)
        if (srcR[i].contains (e.position)) { P.listenSrc = i; if (P.cap.n.load() >= 150 && ! P.cap.on.load()) P.finishListen(); repaint(); return; }
    if (lisR.contains (e.position)) { P.startListen(); listening = true; repaint(); return; }
    if (appR.contains (e.position) && P.plan.valid) { P.applyPlan(); ed.select (0); repaint(); return; }
}
void GuidePanel::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (1);
    jlook::card (g, b, 14, C::amber, true, false, false);
    const char* tabs[] = { "MODELLO", "TECNICA", "CONSIGLI", "ASCOLTO" };
    for (int i = 0; i < 4; ++i)
    {
        auto r = juce::Rectangle<float> (16 + i * kTabW, 10, kTabW - 4, 24);
        jlook::button (g, r, C::amber, i == tab, false, tabs[i], uf (10.5f));
        if (i == 2 && warn > 0) { g.setColour (C::red); g.fillEllipse (r.getRight() - 12, r.getY() + 4, 7, 7); }
    }
    if (tab == 3) { paintListen (g, b); return; }
    auto t = b.reduced (18, 0).withTrimmedTop (44).withTrimmedBottom (12);
    juce::AttributedString as; as.setWordWrap (juce::AttributedString::byWord); as.setLineSpacing (2.0f);
    auto head = [&as] (const juce::String& s, juce::Colour c) { as.append (s + "\n", jlook::ui (11.0f, 2), c); };
    auto body = [&as] (const juce::String& s) { as.append (s + "\n", jlook::ui (12.0f, 0), C::text.withAlpha (0.9f)); };
    if (tab == 0)
    {
        const int m = juce::roundToInt (ed.sv (ed.focus, "model")); const auto& G = guide (m);
        head (juce::String (info (m).name) + "  -  " + info (m).inspired, neo::acc (juce::Colour (info (m).accent))); body (G.carattere);
        head ("PER COSA", C::dim); body (G.usi);
        head ("COME PARTIRE", C::dim); body (G.partenza);
    }
    else if (tab == 1)
    {
        head ("SOGLIA", C::phos); body ("La linea nel misuratore accanto al VU e' dove lo stadio comincia a comprimere; la barra rossa sopra la linea e' la parte del segnale che viene compressa. Trascina la linea o gira la manopola grande. Ogni modello parte con la soglia a 0 dB: acceso non fa niente finche' non la abbassi. FET, VARI-MU e DISTRESS lavorano a INPUT come gli originali: +20 dB di INPUT = soglia a -20 dB, il volume d'uscita resta lo stesso.");
        head ("DELTA", C::phos); body ("DELTA in alto fa sentire solo quello che la compressione toglie, da tutti gli stadi accesi. SOLO fa lo stesso per uno stadio.");
        head ("ROUTING", C::phos); body (routeTip (juce::roundToInt (ed.proc.value ("route"))));
        head ("SOLO", C::phos); body ("SOLO su uno stadio fa sentire solo quello che sta togliendo: se senti il corpo dello strumento e non solo i picchi, stai comprimendo troppo.");
        head ("PRESET", C::phos); body (presets()[(size_t) juce::jlimit (0, (int) presets().size() - 1, ed.proc.preset)].note);
        head ("SIDECHAIN", C::phos); body ("ESTERNO: in Ableton apri il plugin, sezione Sidechain, scegli la traccia (es. la cassa). HPF: il rilevatore ignora i bassi sotto quella frequenza. LISTEN: ascolti cosa sente il rilevatore.");
    }
    else
    {
        if (tips.isEmpty()) body ("Fai suonare qualcosa: i consigli si aggiornano mentre ascolti.");
        for (auto& s : tips) { as.append (juce::String::fromUTF8 ("\xe2\x80\xa2 "), jlook::ui (12.0f, 2), C::amber); body (s); }
    }
    as.draw (g, t);
}

void GuidePanel::paintListen (juce::Graphics& g, juce::Rectangle<float> b)
{
    auto& P = ed.proc; const auto& L = P.plan;
    // riga dei comandi: cosa sto ascoltando, ASCOLTA, APPLICA
    const float sw = 92;
    for (int i = 0; i < kNumSrc; ++i)
    {
        srcR[i] = { 16 + i * (sw + 4), 44, sw, 24 };
        jlook::button (g, srcR[i], C::phos, P.listenSrc == i, false, srcName (i), uf (10.0f));
    }
    const bool cap = P.cap.on.load(); const int n = P.cap.n.load();
    lisR = { 510, 44, 150, 24 }; appR = { 668, 44, 150, 24 };
    jlook::button (g, lisR, C::magenta, cap, false, cap ? "ASCOLTO... " + juce::String (n * 100 / Capture::kNeed) + "%" : (L.valid ? juce::String ("RIASCOLTA") : juce::String ("ASCOLTA 8 s")), uf (11.0f));
    jlook::button (g, appR, C::green, P.calib >= 0, false, P.calib >= 0 ? juce::String ("TARATURA...") : juce::String ("APPLICA IL RACK"), uf (11.0f));
    if (P.calibMsg.isNotEmpty()) { g.setColour (P.calib >= 0 ? C::amber : C::green); g.setFont (uf (11.0f)); g.drawFittedText (P.calibMsg, juce::Rectangle<int> (830, 40, (int) b.getWidth() - 846, 32), juce::Justification::centredLeft, 2); }

    auto area = b.reduced (18, 0).withTrimmedTop (80).withTrimmedBottom (10);
    juce::AttributedString a1, a2; for (auto* a : { &a1, &a2 }) { a->setWordWrap (juce::AttributedString::byWord); a->setLineSpacing (1.5f); }
    auto head = [] (juce::AttributedString& a, const juce::String& s, juce::Colour c) { a.append (s + "\n", jlook::ui (11.0f, 2), c); };
    auto body = [] (juce::AttributedString& a, const juce::String& s) { a.append (s + "\n", jlook::ui (11.5f, 0), C::text.withAlpha (0.9f)); };
    if (cap)
    {
        head (a1, "STO ASCOLTANDO", C::magenta);
        body (a1, n == 0 ? juce::String ("Fai partire la traccia: conto solo i momenti in cui suona, i silenzi non valgono.") : juce::String ("Raccolgo picchi, dinamica, attacchi e spettro. Lascia suonare una parte rappresentativa (strofa o ritornello)."));
        a1.draw (g, area); return;
    }
    if (! L.valid)
    {
        head (a1, "COME FUNZIONA", C::amber);
        body (a1, "1. Scegli cosa stai trattando (o lascia AUTO e lo capisco io).");
        body (a1, "2. ASCOLTA: fai suonare 8 secondi della traccia. Misuro picchi, dinamica, quanti attacchi ci sono e dove sta l'energia.");
        body (a1, "3. Ti dico cosa ho sentito con i numeri e ti propongo un rack, stadio per stadio, con il perche' di ogni scelta.");
        body (a1, "4. APPLICA: monto il rack e mentre suona taro da solo le soglie finche' ogni stadio lavora quanto deve.");
        if (n > 0 && n < 150) body (a1, "Ho sentito troppo poco segnale: rifai ASCOLTA con la traccia in play.");
        a1.draw (g, area); return;
    }
    head (a1, "COSA HO SENTITO", C::amber); body (a1, L.srcWhy);
    for (auto& h : L.heard) { a1.append (juce::String::fromUTF8 ("\xe2\x80\xa2 "), jlook::ui (11.5f, 2), C::amber); body (a1, h); }
    if (! L.notes.isEmpty()) { head (a1, "IN PIU'", C::dim); for (auto& t : L.notes) { a1.append (juce::String::fromUTF8 ("\xe2\x80\xa2 "), jlook::ui (11.5f, 2), C::red); body (a1, t); } }
    head (a2, "RACK PROPOSTO  (in serie, AUTO GAIN acceso)", C::green);
    for (size_t k = 0; k < L.stages.size(); ++k)
    {
        const auto& q = L.stages[k];
        a2.append (juce::String ((int) k + 1) + "   " + q.what + "\n", jlook::ui (12.0f, 2), neo::acc (juce::Colour (info (q.model).accent)));
        body (a2, q.why);
    }
    if (L.scHpf > 21) body (a2, "HPF del sidechain a " + juce::String (juce::roundToInt (L.scHpf)) + " Hz.");
    const float half = area.getWidth() * 0.42f;
    a1.draw (g, area.withWidth (half - 12)); a2.draw (g, area.withTrimmedLeft (half + 12));
    g.setColour (C::dim.withAlpha (0.4f)); g.fillRect (area.getX() + half, area.getY(), 1.0f, area.getHeight());
}

void Editor::computeTips()
{
    juce::StringArray t; int w = 0;
    const int f = focus; const auto sp = proc.stageParams (f);
    const float gr = proc.avgGr[f].load();
    const float inDb = juce::Decibels::gainToDecibels (std::sqrt (proc.inRms.load()), -100.0f), outDb = juce::Decibels::gainToDecibels (std::sqrt (proc.outRms.load()), -100.0f);
    const juce::String st = juce::String (f + 1);
    if (inDb < -60) { guide.setTips ({}, 0); return; }
    if (proc.deltaOn()) { t.add ("DELTA acceso: senti solo quello che la compressione toglie. Spegnilo per sentire il risultato."); ++w; }
    if (proc.anySolo()) { t.add ("SOLO attivo: stai ascoltando solo quello che gli stadi in solo tolgono. Spegnilo per sentire il risultato."); ++w; }
    if (! sp.on) t.add ("Lo stadio " + st + " e spento: clicca la sua casella nella fila in alto per accenderlo.");
    else
    {
        if (gr < 0.5f && sp.model != OTT && sp.model != MULTI) { t.add ("Stadio " + st + ": quasi nessuna riduzione. Alza " + juce::String (info (sp.model).driveLabel) + " (o trascina la linea SOGLIA) finche' la barra rossa sopra la soglia c'e' sui picchi e il VU scende di 2-6 dB."); ++w; }
        else if (gr > 12) { t.add ("Stadio " + st + ": oltre 12 dB di riduzione. Va bene solo in parallelo (MIX 20-40%), altrimenti stai schiacciando."); ++w; }
        else if (gr > 6) t.add ("Stadio " + st + ": riduzione forte (" + juce::String (gr, 1) + " dB). Ok per voce rap, room e smash; tanto per bus e master.");
        else t.add ("Stadio " + st + ": " + juce::String (gr, 1) + " dB di riduzione media, zona giusta per quasi tutto.");
        if (info (sp.model).relKnob && relMs (sp.model, sp.rel) > 0 && relMs (sp.model, sp.rel) < 90 && gr > 6) { t.add ("Rilascio molto corto con tanta riduzione: rischi distorsione sui bassi e pumping. Allunga il RELEASE."); ++w; }
        if (info (sp.model).attKnob && attMs (sp.model, sp.att) < 0.1f && gr > 3) t.add ("Attacco velocissimo: smussa i transienti. Per una batteria che picchia allunga l'attacco.");
        if (sp.model == OTT && sp.drive > 0.6f && sp.mix > 0.8f) t.add ("OTT molto profondo al 100% di MIX: suono famoso ma estremo. Prova MIX 30-50%.");
        if (proc.value ("scHpf") < 25 && gr > 3 && (sp.model == VCABUS || sp.model == VARIMU)) t.add ("Su bus e master alza l'HPF del sidechain a 80-150 Hz: il compressore smette di reagire alla cassa e non pompa.");
    }
    int busy = 0; for (int s = 0; s < kStages; ++s) if (sv (s, "on") > 0.5f && proc.avgGr[s].load() > 6) ++busy;
    if (busy >= 2) { t.add (juce::String (busy) + " stadi lavorano tanto: con piu stadi suona meglio 1-4 dB ciascuno."); ++w; }
    const float d = outDb - inDb;
    if (d > 2) { t.add ("L'uscita e piu forte dell'ingresso di " + juce::String (d, 1) + " dB: il piu forte sembra sempre meglio. Attiva AUTO GAIN o abbassa OUTPUT per un confronto onesto."); ++w; }
    else if (d < -3) t.add ("L'uscita e piu bassa di " + juce::String (-d, 1) + " dB: recupera con OUTPUT o AUTO GAIN.");
    if (proc.value ("scSrc") > 0.5f && ! proc.hasExtSidechain()) { t.add ("Sidechain ESTERNO selezionato ma nessuna traccia collegata: in Ableton scegli la sorgente nella sezione Sidechain del plugin."); ++w; }
    if (juce::jmax (proc.outPk[0].load(), proc.outPk[1].load()) > 0.99f) { t.add ("L'uscita tocca 0 dBFS: abbassa OUTPUT."); ++w; }
    guide.setTips (t, w);
}

// ======================= Editor =======================
Editor::Editor (Processor& p) : AudioProcessorEditor (&p), proc (p)
{
    // tema MACCHINA: rack da studio anni 70, alluminio scuro, scritte crema, VU ambra
    // CBRPNK: arancio, salmone, salvia, grigio. HUD: rosso
    skin::init ("comp", { 0xffe9a260, 0xffde6c5c, 0xffa8bea0, 0xffccccc7 }, 0xffff2828);
    C::sync();
    setLookAndFeel (&lnf);
    addAndMakeVisible (rack); addAndMakeVisible (stage); addAndMakeVisible (graph); addAndMakeVisible (guide);
    route = std::make_unique<Segs> (P ("route"), C::magenta); route->items = [] { return juce::StringArray { "SERIE", "PARALLELO" }; };
    route->map = [] (int k) { return k == 0 ? 0 : 2; }; route->unmap = [] (int v) { return v == 2 ? 1 : 0; };
    route->onTouch = [this] { repaint(); };
    scSrc = std::make_unique<Segs> (P ("scSrc"), C::green); scSrc->items = [] { return juce::StringArray { "INTERNO", "ESTERNO" }; };
    scTo = std::make_unique<Segs> (P ("scTo"), C::green); scTo->items = [] { return juce::StringArray { "TUTTI", "1", "2", "3", "4", "5", "6" }; };
    scHpf = std::make_unique<Knob> (P ("scHpf"), C::green); scHpf->label = "SC HPF";
    scHpf->text = [this] { const float f = proc.value ("scHpf"); return f < 21 ? juce::String ("OFF") : juce::String (juce::roundToInt (f)) + " Hz"; };
    listen = std::make_unique<Switch> (P ("scListen"), C::green, "LISTEN");
    autoG = std::make_unique<Switch> (P ("auto"), C::phos, "AUTO GAIN");
    gMix = std::make_unique<Knob> (P ("mix"), C::phos); gMix->label = "MIX"; gMix->text = [this] { return juce::String (juce::roundToInt (proc.value ("mix") * 100)) + "%"; };
    gOut = std::make_unique<Knob> (P ("out"), C::phos); gOut->label = "OUTPUT"; gOut->bipolar = true; gOut->text = [this] { const float v = proc.value ("out"); return (v > 0 ? "+" : "") + juce::String (v, 1) + " dB"; };
    for (juce::Component* c : std::initializer_list<juce::Component*> { route.get(), scSrc.get(), scTo.get(), scHpf.get(), listen.get(), autoG.get(), gMix.get(), gOut.get() }) addAndMakeVisible (c);
    addAndMakeVisible (sizeChip);
    recolor();
    scaler.setup();
    startTimerHz (30);
}
void Editor::recolor()
{
    C::sync();
    route->col = C::magenta; for (auto* c : { scSrc.get(), scTo.get() }) c->col = C::green; scHpf->col = C::green; listen->col = C::green;
    autoG->col = C::phos; gMix->col = C::phos; gOut->col = C::phos;
    stage.bind (focus);
    for (auto* c : getChildren()) c->repaint();
    repaint();
}
void Editor::select (int s) { focus = s; stage.bind (s); rack.repaint(); guide.repaint(); graph.repaint(); }

void Editor::resized()
{
    const float sc = scaler.update();
    presetRect = { 470, 15, 330, 36 }; prevRect = { 432, 15, 36, 36 }; nextRect = { 802, 15, 36, 36 }; abRect = { 850, 15, 110, 36 };
    rack.setBounds (18, 78, 792, 100);
    stage.setBounds (18, 190, 792, 300);
    const int rx = 818, rw = 404;
    route->setBounds (rx + 16, 112, rw - 32, 30);
    scSrc->setBounds (rx + 16, 236, 180, 28); scTo->setBounds (rx + 16, 272, 220, 28);
    scHpf->setBounds (rx + 246, 222, 72, 72 + 27); listen->setBounds (rx + 320, 250, 80, 22);
    autoG->setBounds (rx + 16, 380, 130, 22);
    gMix->setBounds (rx + 168, 366, 72, 72 + 27); gOut->setBounds (rx + 250, 366, 72, 72 + 27);
    if (guide.tab == 3) { graph.setVisible (false); guide.setBounds (18, 502, 1204, 284); }
    else { graph.setVisible (true); graph.setBounds (18, 502, 792, 284); guide.setBounds (818, 502, 404, 284); }
    sizeChip.setBounds (W - 76, H - 18, 58, 15);
    jlook::scaleChildren (*this, sc);
}

juce::String Editor::staticKey()
{
    juce::String k; k << skin::cur() << "/" << proc.preset << proc.getProgramName (proc.preset) << "/" << juce::roundToInt (proc.value ("route")) << "/" << (int) proc.hasExtSidechain();
    for (int s = 0; s < kStages; ++s) k << (int) (sv (s, "on") > 0.5f);
    return k;
}
void Editor::paint (juce::Graphics& g)
{
    skin::apply(); C::sync();
    g.addTransform (juce::AffineTransform::scale (scaler.s));
    const float Wf = (float) W, Hf = (float) H; const float rx = 818, rw = 404;
    // tutto cio' che non si muove sta in un'immagine: si ridisegna solo quando cambia (preset, tema, routing...)
    bg.draw (g, { 0, 0, W, H }, staticKey(), [&] (juce::Graphics& g)
    {
    jlook::backdrop (g, { 0, 0, Wf, Hf }, C::magenta, C::phos);
    jlook::header (g, { 0, 0, Wf, 64 }, "STUDIO COMP", C::phos, "JC-6  MULTI COMPRESSOR", C::magenta, C::phos);
    for (auto* c : { (juce::Component*) &stage, (juce::Component*) &graph, (juce::Component*) &guide }) jlook::softShadow (g, c->getBounds().toFloat(), 14);
    lcd (g, presetRect.toFloat(), 8); jlook::glass (g, presetRect.toFloat(), 8);
    glowText (g, proc.getProgramName (proc.preset).toUpperCase(), presetRect.toFloat(), neo::light() ? C::text : C::amber, mf (12.5f), juce::Justification::centred);
    g.setColour (C::phos); g.setFont (uf (20.0f));
    g.drawText (juce::String::fromUTF8 ("\xe2\x80\xb9"), prevRect, juce::Justification::centred); g.drawText (juce::String::fromUTF8 ("\xe2\x80\xba"), nextRect, juce::Justification::centred);
    skin::chipDraw (g, themeRect.toFloat());
    auto cardR = [&] (juce::Rectangle<float> r, juce::Colour c, const char* t) { jlook::softShadow (g, r, 14); jlook::card (g, r, 14, c, true, false, false); glowText (g, t, { r.getX() + 16, r.getY() + 6, 200, 22 }, C::text, uf (12.0f)); };
    cardR ({ rx, 78, rw, 112 }, C::magenta, "ROUTING");
    { // catena
        const bool par = juce::roundToInt (proc.value ("route")) == 2; auto d = juce::Rectangle<float> (rx + 16, 150, rw - 32, 32);
        g.setColour (C::dim); g.setFont (mf (9.5f)); g.drawText ("IN", juce::Rectangle<float> (d.getX(), d.getY(), 24, 32), juce::Justification::centredLeft); g.drawText ("OUT", juce::Rectangle<float> (d.getRight() - 28, d.getY(), 28, 32), juce::Justification::centredRight);
        juce::Array<int> on; for (int s = 0; s < kStages; ++s) if (sv (s, "on") > 0.5f) on.add (s);
        const float x0 = d.getX() + 26, x1 = d.getRight() - 30, cy = d.getCentreY();
        g.setColour (C::dim.withAlpha (0.6f)); g.drawLine (x0, cy, x1, cy);
        const int n = juce::jmax (1, on.size()); const float bw = juce::jmin (34.0f, (x1 - x0) / n - 6);
        for (int i = 0; i < on.size(); ++i)
        {
            const int s = on[i]; auto r = par ? juce::Rectangle<float> ((x0 + x1) / 2 - bw / 2 + (i - (on.size() - 1) / 2.0f) * (bw + 4), cy - 9, bw, 18)
                                              : juce::Rectangle<float> (x0 + (x1 - x0) * (i + 0.5f) / n - bw / 2, cy - 9, bw, 18);
            g.setColour (C::stage[s].withAlpha (0.25f)); g.fillRoundedRectangle (r, 4); g.setColour (C::stage[s]); g.drawRoundedRectangle (r, 4, 1.0f);
            g.setFont (uf (11.0f)); g.drawText (juce::String (s + 1), r, juce::Justification::centred);
        }
        if (par && on.size() > 1) { g.setColour (C::dim); g.setFont (mf (8.5f)); g.drawText ("in parallelo, sommati", juce::Rectangle<float> (d.getX() + 30, d.getBottom() - 4, 200, 12), juce::Justification::centredLeft); }
    }
    cardR ({ rx, 202, rw, 140 }, C::green, "SIDECHAIN");
    g.setFont (mf (9.5f)); g.setColour (proc.hasExtSidechain() ? C::green : C::dim);
    g.drawText (proc.hasExtSidechain() ? "esterno collegato" : "esterno non collegato", juce::Rectangle<float> (rx + 200, 208, rw - 216, 18), juce::Justification::centredRight);
    g.setColour (C::dim); g.setFont (uf (9.5f)); g.drawText ("su", juce::Rectangle<float> (rx + 16, 300, 30, 14), juce::Justification::centredLeft);
    cardR ({ rx, 354, rw, 136 }, C::phos, "USCITA");
    g.setColour (C::dim); g.setFont (uf (10.0f, false));
    g.drawFittedText ("AUTO GAIN pareggia il volume: confronta ad orecchio onesto.", juce::Rectangle<int> ((int) rx + 16, 412, 140, 60), juce::Justification::topLeft, 4);
    for (auto mrr : { juce::Rectangle<float> (rx + 336, 384, 24, 100), juce::Rectangle<float> (rx + 366, 384, 24, 100) })
    { g.setColour (C::dim); g.setFont (mf (8.5f)); g.drawText (mrr.getX() < rx + 360 ? "IN" : "OUT", mrr.removeFromBottom (12), juce::Justification::centred); lcd (g, mrr, 3); }
    });
    // parti vive
    jlook::button (g, abRect.toFloat(), C::phos, proc.ab.cur == 1, abHover, "MEMORIA " + proc.ab.label(), uf (11.5f));
    {
        const bool so = proc.anySolo(), dl = proc.deltaOn();
        jlook::button (g, dltRect.toFloat(), so ? C::amber : C::magenta, so || dl, dltHover, so ? juce::String ("SOLO ATTIVO  x") : dl ? juce::String ("DELTA ACCESO") : juce::String ("DELTA"), uf (11.0f));
    }
    auto meter = [&] (juce::Rectangle<float> r, float l, float rr)
    {
        r.removeFromBottom (12);
        for (int c = 0; c < 2; ++c)
        {
            auto m = juce::Rectangle<float> (r.getX() + 3 + c * (r.getWidth() / 2 - 1), r.getY() + 3, r.getWidth() / 2 - 5, r.getHeight() - 6);
            const float db = juce::Decibels::gainToDecibels (c ? rr : l, -60.0f), n = juce::jlimit (0.0f, 1.0f, (db + 60) / 63);
            const int seg = 22;
            for (int k = 0; k < seg; ++k) { const float t2 = k / (float) seg; g.setColour (t2 <= n ? (t2 > 0.93f ? C::red : t2 > 0.8f ? C::amber : C::phos) : neo::K::hi.withAlpha (0.05f)); g.fillRect (m.getX(), m.getBottom() - (k + 1) * m.getHeight() / seg + 1, m.getWidth(), m.getHeight() / seg - 1.5f); }
        }
    };
    meter ({ rx + 336, 384, 24, 100 }, proc.inPk[0].load(), proc.inPk[1].load());
    meter ({ rx + 366, 384, 24, 100 }, proc.outPk[0].load(), proc.outPk[1].load());
}

void Editor::mouseMove (const juce::MouseEvent& e0)
{
    const auto e = e0.withNewPosition (e0.position / scaler.s);
    const bool h = abRect.contains (e.getPosition()), hd = dltRect.contains (e.getPosition());
    if (h != abHover) { abHover = h; repaint (scaler.map (abRect.expanded (8))); }
    if (hd != dltHover) { dltHover = hd; repaint (scaler.map (dltRect.expanded (8))); }
    setMouseCursor (h || hd || presetRect.contains (e.getPosition()) || prevRect.contains (e.getPosition()) || nextRect.contains (e.getPosition()) ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
}

void Editor::mouseDown (const juce::MouseEvent& e0)
{
    const auto e = e0.withNewPosition (e0.position / scaler.s);
    const int n = (int) presets().size();
    auto after = [this] { stage.bind (focus); rack.repaint(); repaint(); };
    if (themeRect.contains (e.getPosition())) { skin::cycle(); recolor(); return; }
    if (dltRect.contains (e.getPosition()))
    {   // con un solo acceso il pulsante spegne i solo; altrimenti accende/spegne il DELTA
        if (proc.anySolo()) { for (int s = 0; s < kStages; ++s) if (auto* q = P (juce::String (stagePfx (s)) + "solo")) q->setValueNotifyingHost (0.0f); }
        else if (auto* q = P ("delta")) q->setValueNotifyingHost (q->getValue() > 0.5f ? 0.0f : 1.0f);
        stage.repaint(); rack.repaint(); repaint (scaler.map (dltRect.expanded (8))); return;
    }
    if (abRect.contains (e.getPosition())) { proc.ab.toggle (proc); after(); }
    else if (prevRect.contains (e.getPosition())) { proc.loadPreset ((proc.preset + n - 1) % n); after(); }
    else if (nextRect.contains (e.getPosition())) { proc.loadPreset ((proc.preset + 1) % n); after(); }
    else if (presetRect.contains (e.getPosition()))
    {
        juce::PopupMenu m; std::vector<std::pair<juce::String, juce::PopupMenu>> groups;
        for (int i = 0; i < n; ++i)
        {
            const juce::String name = presets()[(size_t) i].name;
            if (! name.contains (" | ")) { m.addItem (i + 1, name, true, i == proc.preset); continue; }
            const auto gn = name.upToFirstOccurrenceOf (" | ", false, false), it = name.fromFirstOccurrenceOf (" | ", false, false);
            bool found = false; for (auto& gr : groups) if (gr.first == gn) { gr.second.addItem (i + 1, it, true, i == proc.preset); found = true; }
            if (! found) { juce::PopupMenu pm; pm.addItem (i + 1, it, true, i == proc.preset); groups.push_back ({ gn, pm }); }
        }
        for (auto& gr : groups) m.addSubMenu (gr.first, gr.second);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withTargetScreenArea (localAreaToGlobal (scaler.map (presetRect))), [this, after] (int r) { if (r > 0) { proc.loadPreset (r - 1); after(); } });
    }
}

void Editor::timerCallback()
{
    // si ridisegna solo cio' che e' cambiato e solo la sua zona
    stage.tick();
    for (auto* k : { stage.drive.get(), stage.att.get(), stage.rel.get(), stage.out.get(), stage.color.get(), stage.mix.get(), stage.drive2.get(), stage.att2.get(), scHpf.get(), gMix.get(), gOut.get(), stage.bThr.get(), stage.bRatio.get(), stage.bAtt.get(), stage.bRel.get(), stage.bGain.get() }) if (k->isVisible()) k->refresh();
    rack.tick();
    if (graph.isVisible() && (tick & 1) == 0) graph.repaint();
    {   // misuratori IN/OUT
        const float v[4] = { proc.inPk[0].load(), proc.inPk[1].load(), proc.outPk[0].load(), proc.outPk[1].load() };
        bool ch = false; for (int i = 0; i < 4; ++i) if (std::abs (v[i] - mShown[i]) > 0.003f) { mShown[i] = v[i]; ch = true; }
        if (ch) repaint (scaler.map ({ 1150, 380, 68, 106 }));
    }
    proc.calibTick();
    if (guide.tab == 3)
    {
        if (guide.listening && ! proc.cap.on.load()) { guide.listening = false; proc.finishListen(); guide.repaint(); }
        if (tick % 6 == 0) guide.repaint();
    }
    if (++tick % 15 == 0)
    {
        computeTips();
        static juce::String lastK; const auto k = staticKey() + (proc.anySolo() ? "S" : "") + (proc.deltaOn() ? "D" : "");
        if (k != lastK) { lastK = k; repaint(); stage.repaint(); rack.repaint(); }   // cambi arrivati dall'automazione o dal DAW
    }
}
}
