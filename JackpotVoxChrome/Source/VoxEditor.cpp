#include "VoxEditor.h"

namespace jvox
{
using namespace ui;
namespace
{
juce::Path chamfer (juce::Rectangle<float> r, float c)
{
    juce::Path p;
    p.startNewSubPath (r.getX() + c, r.getY()); p.lineTo (r.getRight() - c, r.getY()); p.lineTo (r.getRight(), r.getY() + c); p.lineTo (r.getRight(), r.getBottom());
    p.lineTo (r.getX() + c, r.getBottom()); p.lineTo (r.getX(), r.getBottom() - c); p.lineTo (r.getX(), r.getY() + c); p.closeSubPath();
    return p;
}
void text (juce::Graphics& g, const juce::String& t, juce::Rectangle<float> r, juce::Font f, juce::Colour c, juce::Justification j = juce::Justification::centred)
{ g.setFont (f); g.setColour (c); g.drawFittedText (t, r.toNearestInt(), j, 1, 0.6f); }
void glowText (juce::Graphics& g, const juce::String& t, juce::Rectangle<float> r, juce::Font f, juce::Colour c, float on, juce::Justification j = juce::Justification::centred)
{
    g.setFont (f);
    for (int i = 3; i >= 1 && on > 0.01f; --i) { g.setColour (c.withAlpha (0.08f * on)); for (int a = 0; a < 8; ++a) g.drawFittedText (t, r.translated (std::cos (a * 0.785f) * i * 1.4f, std::sin (a * 0.785f) * i * 1.4f).toNearestInt(), j, 1, 0.6f); }
    g.setColour (c); g.drawFittedText (t, r.toNearestInt(), j, 1, 0.6f);
}
void rivet (juce::Graphics& g, float x, float y, float r = 3.0f)
{
    g.setColour (juce::Colours::black.withAlpha (0.7f)); g.fillEllipse (x - r + 0.8f, y - r + 1.2f, r * 2, r * 2);
    juce::ColourGradient cg (juce::Colour (0xff6d6f77), x - r, y - r, juce::Colour (0xff1b1c20), x + r, y + r, false); g.setGradientFill (cg); g.fillEllipse (x - r, y - r, r * 2, r * 2);
}
void hazard (juce::Graphics& g, juce::Rectangle<float> r)
{
    g.saveState(); g.reduceClipRegion (r.toNearestInt());
    g.setColour (WARN.withAlpha (0.75f)); g.fillRect (r);
    g.setColour (BLACK); for (float x = r.getX() - r.getHeight(); x < r.getRight(); x += 14) { juce::Path p; p.addQuadrilateral (x, r.getBottom(), x + 7, r.getBottom(), x + 7 + r.getHeight(), r.getY(), x + r.getHeight(), r.getY()); g.fillPath (p); }
    g.restoreState();
}
const juce::Rectangle<float> HEAD { 420, 84, 340, 384 }, MON { 392, 476, 396, 132 };
}

// =============================================================================================== manopole: acciaio, gonna cromata, anello di led rossi
Knob::Knob (Editor& e, const juce::String& id, juce::String l, std::function<juce::String (float)> fm)
    : ed (e), prm (e.P (id)), label (std::move (l)), fmt (std::move (fm))
{ setRepaintsOnMouseActivity (true); setMouseCursor (juce::MouseCursor::UpDownResizeCursor); }
void Knob::paint (juce::Graphics& g)
{
    const float w = (float) getWidth(), h = (float) getHeight(), v = prm->getValue(), a0 = -2.35f, a1 = 2.35f, av = a0 + (a1 - a0) * v;
    const bool hov = isMouseOverOrDragging();
    const juce::String t = fmt ? fmt (prm->convertFrom0to1 (v)) : prm->getCurrentValueAsText();
    const bool compact = h < 130;
    const float lh = 20, vh = 20, S = compact ? h - 30 : juce::jmin (w - 34, h - lh - vh - 18), cx = compact ? w - S * 0.5f - 16 : w * 0.5f, cy = compact ? h * 0.5f : lh + 9 + S * 0.5f, R = S * 0.5f;
    if (compact) text (g, label, { 0, h * 0.5f - 22, w - S - 30, 20 }, raj (17.0f, 0.16f), hov ? juce::Colours::white : INK.withAlpha (0.85f), juce::Justification::centredLeft);
    else text (g, label, { -10, 0, w + 20, lh }, raj (17.0f, 0.16f), hov ? juce::Colours::white : INK.withAlpha (0.85f));
    const bool live = bipolar ? std::abs (v - 0.5f) > 0.004f : v > 0.002f;
    for (int i = 0; i <= 26; ++i)
    {
        const float f = i / 26.0f, a = a0 + (a1 - a0) * f;
        const bool lit = live && (bipolar ? (f >= juce::jmin (0.5f, v) - 1e-4f && f <= juce::jmax (0.5f, v) + 1e-4f) : f <= v + 1e-4f);
        const float r1 = R + 6, r2 = R + 11;
        if (lit) { g.setColour (RED.withAlpha (0.22f)); g.drawLine (cx + std::sin (a) * r1, cy - std::cos (a) * r1, cx + std::sin (a) * r2, cy - std::cos (a) * r2, 6.0f); }
        g.setColour (lit ? RED : juce::Colour (0xff24161a)); g.drawLine (cx + std::sin (a) * r1, cy - std::cos (a) * r1, cx + std::sin (a) * r2, cy - std::cos (a) * r2, 2.6f);
    }
    // gonna cromata
    g.setColour (juce::Colours::black.withAlpha (0.7f)); g.fillEllipse (cx - R + 3, cy - R + 6, R * 2, R * 2);
    { juce::ColourGradient sk (juce::Colour (0xffd7dae0), cx, cy - R, juce::Colour (0xff1c1d21), cx, cy + R, false); sk.addColour (0.42, juce::Colour (0xff4a4d55)); sk.addColour (0.55, juce::Colour (0xffa4a8b1)); sk.addColour (0.7, juce::Colour (0xff2b2d33)); g.setGradientFill (sk); g.fillEllipse (cx - R, cy - R, R * 2, R * 2); }
    for (int i = 0; i < 40; ++i) { const float a = av + i * juce::MathConstants<float>::twoPi / 40; g.setColour (juce::Colours::black.withAlpha (0.45f)); g.drawLine (cx + std::sin (a) * (R - 0.5f), cy - std::cos (a) * (R - 0.5f), cx + std::sin (a) * (R - 5), cy - std::cos (a) * (R - 5), 1.1f); }
    // testa in acciaio brunito
    const float rc = R * 0.7f;
    { juce::ColourGradient cap (juce::Colour (0xff34363d), cx - rc * 0.6f, cy - rc, juce::Colour (0xff08080a), cx + rc * 0.5f, cy + rc, false); g.setGradientFill (cap); g.fillEllipse (cx - rc, cy - rc, rc * 2, rc * 2); }
    g.setColour (juce::Colours::white.withAlpha (0.1f)); g.drawEllipse (cx - rc, cy - rc, rc * 2, rc * 2, 1.0f);
    { const juce::Point<float> p0 (cx + std::sin (av) * rc * 0.15f, cy - std::cos (av) * rc * 0.15f), p1 (cx + std::sin (av) * (rc - 2), cy - std::cos (av) * (rc - 2));
      g.setColour (RED.withAlpha (live ? 0.4f : 0.15f)); g.drawLine ({ p0, p1 }, 7.0f); g.setColour (live ? juce::Colour (0xffff6a72) : juce::Colour (0xff8a8c93)); g.drawLine ({ p0, p1 }, 2.6f); }
    if (compact) text (g, t, { 0, h * 0.5f + 2, w - S - 30, 20 }, mono (15.0f), hov ? juce::Colours::white : (live ? RED.interpolatedWith (INK, 0.35f) : DIM), juce::Justification::centredLeft);
    else text (g, t, { -10, h - vh, w + 20, vh }, mono (15.0f), hov ? juce::Colours::white : (live ? RED.interpolatedWith (INK, 0.35f) : DIM));
}
void Knob::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isAltDown() || e.mods.isCommandDown()) { prm->setValueNotifyingHost (prm->getDefaultValue()); return; }
    start = prm->getValue(); drag = true; prm->beginChangeGesture();
}
void Knob::mouseDrag (const juce::MouseEvent& e)
{
    if (! drag) return;
    float nv = start - e.getDistanceFromDragStartY() / (e.mods.isShiftDown() ? 900.0f : 220.0f);
    if (steps > 1) nv = std::round (nv * (steps - 1)) / (steps - 1);
    prm->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, nv)); repaint();
}
void Knob::mouseUp (const juce::MouseEvent&) { if (drag) prm->endChangeGesture(); drag = false; }
void Knob::mouseDoubleClick (const juce::MouseEvent&) { prm->setValueNotifyingHost (prm->getDefaultValue()); }
void Knob::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& wd)
{
    const float dy = wd.isReversed ? -wd.deltaY : wd.deltaY;
    if (steps > 1) { const float st = 1.0f / (steps - 1); prm->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, prm->getValue() + (dy > 0 ? st : -st))); }
    else prm->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, prm->getValue() + juce::jlimit (-0.05f, 0.05f, dy * 0.08f)));
}

// =============================================================================================== la testa cromata
void Editor::drawHead (juce::Graphics& g, juce::Rectangle<float> b)
{
    const float cx = b.getCentreX(), top = b.getY() + 6, W2 = b.getWidth() * 0.36f;
    auto chrome = [] (float y0, float y1, float x) {
        juce::ColourGradient cg (juce::Colour (0xfff1f3f7), x, y0, juce::Colour (0xffa9adb6), x, y1, false);
        cg.addColour (0.16, juce::Colour (0xff6e727b)); cg.addColour (0.3, juce::Colour (0xffd5d8de)); cg.addColour (0.43, juce::Colour (0xff1a1b20));
        cg.addColour (0.5, juce::Colour (0xff3d4048)); cg.addColour (0.62, juce::Colour (0xff9aa0aa)); cg.addColour (0.82, juce::Colour (0xff2c2e34));
        return cg; };
    // collo e spalle: piastre scure, cavi
    {
        juce::Path sh; sh.startNewSubPath (b.getX() + 10, b.getBottom()); sh.lineTo (b.getX() + 40, b.getBottom() - 70); sh.lineTo (cx - 70, b.getBottom() - 92);
        sh.lineTo (cx + 70, b.getBottom() - 92); sh.lineTo (b.getRight() - 40, b.getBottom() - 70); sh.lineTo (b.getRight() - 10, b.getBottom()); sh.closeSubPath();
        juce::ColourGradient sg (juce::Colour (0xff2a2b31), cx, b.getBottom() - 92, juce::Colour (0xff07070a), cx, b.getBottom(), false); g.setGradientFill (sg); g.fillPath (sh);
        g.setColour (juce::Colours::white.withAlpha (0.12f)); g.strokePath (sh, juce::PathStrokeType (1.0f));
        for (int i = 0; i < 5; ++i) { const float x = cx - 48 + i * 24; juce::ColourGradient cg (juce::Colour (0xff3a3c42), x, 0, juce::Colour (0xff0c0c0f), x + 10, 0, false); g.setGradientFill (cg); g.fillRoundedRectangle (x - 5, b.getBottom() - 128, 10, 60, 4); }
        for (int s = -1; s <= 1; s += 2) for (int i = 0; i < 3; ++i) rivet (g, cx + s * (90 + i * 28), b.getBottom() - 70 + i * 9, 3.2f);
        g.setColour (RED.withAlpha (0.8f)); g.setFont (mono (10.0f)); g.drawText ("UNITA' 7-C  //  PROPRIETA' DEL SISTEMA", juce::Rectangle<float> (cx - 120, b.getBottom() - 34, 240, 14), juce::Justification::centred);
    }
    // calotta
    juce::Path head;
    head.startNewSubPath (cx, top);
    head.cubicTo (cx + W2 * 1.02f, top, cx + W2 * 1.1f, top + 110, cx + W2 * 1.02f, top + 170);
    head.cubicTo (cx + W2 * 0.98f, top + 230, cx + W2 * 0.75f, top + 262, cx + W2 * 0.42f, top + 286);
    head.lineTo (cx - W2 * 0.42f, top + 286);
    head.cubicTo (cx - W2 * 0.75f, top + 262, cx - W2 * 0.98f, top + 230, cx - W2 * 1.02f, top + 170);
    head.cubicTo (cx - W2 * 1.1f, top + 110, cx - W2 * 1.02f, top, cx, top); head.closeSubPath();
    g.setColour (juce::Colours::black.withAlpha (0.65f)); g.fillPath (head, juce::AffineTransform::translation (8, 12));
    { auto cg = chrome (top, top + 286, cx); g.setGradientFill (cg); g.fillPath (head); }
    g.saveState(); g.reduceClipRegion (head);
    { juce::ColourGradient side (juce::Colours::black.withAlpha (0.75f), cx - W2 * 1.1f, 0, juce::Colours::transparentBlack, cx - W2 * 0.35f, 0, false); g.setGradientFill (side); g.fillRect (b); }
    { juce::ColourGradient side (juce::Colours::black.withAlpha (0.8f), cx + W2 * 1.1f, 0, juce::Colours::transparentBlack, cx + W2 * 0.3f, 0, false); g.setGradientFill (side); g.fillRect (b); }
    // riflesso rosso degli stendardi sui fianchi
    { juce::ColourGradient rr (RED.withAlpha (0.28f), cx - W2 * 1.05f, top + 150, RED.withAlpha (0.0f), cx - W2 * 0.55f, top + 150, false); g.setGradientFill (rr); g.fillRect (b); }
    { juce::ColourGradient rr (RED.withAlpha (0.22f), cx + W2 * 1.05f, top + 150, RED.withAlpha (0.0f), cx + W2 * 0.6f, top + 150, false); g.setGradientFill (rr); g.fillRect (b); }
    // placca della fronte e cuciture
    juce::Path brow; brow.startNewSubPath (cx - W2 * 0.9f, top + 118); brow.cubicTo (cx - W2 * 0.6f, top + 60, cx + W2 * 0.6f, top + 60, cx + W2 * 0.9f, top + 118); brow.lineTo (cx + W2 * 0.82f, top + 132); brow.lineTo (cx - W2 * 0.82f, top + 132); brow.closeSubPath();
    g.setColour (juce::Colours::black.withAlpha (0.45f)); g.strokePath (brow, juce::PathStrokeType (2.2f)); g.setColour (juce::Colours::white.withAlpha (0.35f)); g.strokePath (brow, juce::PathStrokeType (0.8f), juce::AffineTransform::translation (0, 1.5f));
    g.setColour (juce::Colours::black.withAlpha (0.5f)); g.drawLine (cx, top + 4, cx, top + 70, 1.6f); g.setColour (juce::Colours::white.withAlpha (0.3f)); g.drawLine (cx + 1.2f, top + 4, cx + 1.2f, top + 70, 0.7f);
    // fessura degli occhi (dentro e' nera: la luce la accende il disegno dal vivo)
    juce::Path slit; slit.addRoundedRectangle (cx - W2 * 0.78f, top + 138, W2 * 1.56f, 26, 6);
    g.setColour (juce::Colour (0xff030304)); g.fillPath (slit); g.setColour (juce::Colours::white.withAlpha (0.4f)); g.strokePath (slit, juce::PathStrokeType (1.0f), juce::AffineTransform::translation (0, 1.5f));
    // guance a lamelle
    for (int s = -1; s <= 1; s += 2)
        for (int i = 0; i < 5; ++i)
        {
            const float x0 = cx + s * (W2 * 0.48f + i * 9), y0 = top + 186 + i * 3;
            g.setColour (juce::Colours::black.withAlpha (0.55f)); g.drawLine (x0, y0, x0 + s * 18, y0 + 40, 2.4f);
            g.setColour (juce::Colours::white.withAlpha (0.25f)); g.drawLine (x0 + 1.5f, y0, x0 + s * 18 + 1.5f, y0 + 40, 0.8f);
        }
    // griglia della bocca
    auto grill = juce::Rectangle<float> (cx - 44, top + 206, 88, 56);
    { juce::ColourGradient gg (juce::Colour (0xff0b0b0e), cx, grill.getY(), juce::Colour (0xff1e1f24), cx, grill.getBottom(), false); g.setGradientFill (gg); g.fillRoundedRectangle (grill, 6); }
    for (float x = grill.getX() + 8; x < grill.getRight() - 4; x += 8) { g.setColour (juce::Colours::black); g.fillRect (x, grill.getY() + 6, 4.0f, grill.getHeight() - 12); g.setColour (juce::Colours::white.withAlpha (0.18f)); g.fillRect (x + 4, grill.getY() + 6, 1.0f, grill.getHeight() - 12); }
    g.setColour (juce::Colours::white.withAlpha (0.35f)); g.drawRoundedRectangle (grill, 6, 1.0f);
    // codice a barre del soggetto sulla fronte
    { juce::Random br (77); float x = cx - 34; while (x < cx + 34) { const float bw = 1.0f + br.nextInt (3); g.setColour (juce::Colours::black.withAlpha (0.75f)); g.fillRect (x, top + 92, bw, 16.0f); x += bw + 1.0f + br.nextInt (2); } }
    g.setColour (juce::Colours::black.withAlpha (0.8f)); g.setFont (mono (9.0f)); g.drawText ("SOGG. 7-C / 0451", juce::Rectangle<float> (cx - 50, top + 110, 100, 10), juce::Justification::centred);
    // graffio profondo sulla calotta (danno)
    { juce::Path cr; cr.startNewSubPath (cx + W2 * 0.25f, top + 30); cr.lineTo (cx + W2 * 0.42f, top + 64); cr.lineTo (cx + W2 * 0.36f, top + 78); cr.lineTo (cx + W2 * 0.6f, top + 120);
      g.setColour (juce::Colours::black.withAlpha (0.8f)); g.strokePath (cr, juce::PathStrokeType (2.0f)); g.setColour (juce::Colours::white.withAlpha (0.6f)); g.strokePath (cr, juce::PathStrokeType (0.7f), juce::AffineTransform::translation (1.2f, 0.8f)); }
    g.restoreState();
    // tubi del respiratore: dalla griglia al collare
    for (int s2 = -1; s2 <= 1; s2 += 2)
    {
        juce::Path hose; hose.startNewSubPath (cx + s2 * 40, top + 246); hose.cubicTo (cx + s2 * 70, top + 280, cx + s2 * 96, top + 290, cx + s2 * 112, top + 330);
        g.setColour (juce::Colours::black); g.strokePath (hose, juce::PathStrokeType (14.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (juce::Colour (0xff2b2c31)); g.strokePath (hose, juce::PathStrokeType (11.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        juce::PathStrokeType (1.0f).createDashedStroke (hose, hose, std::array<float, 2> { 2.0f, 4.0f }.data(), 2);
        g.setColour (juce::Colours::black.withAlpha (0.6f)); g.strokePath (hose, juce::PathStrokeType (9.0f));
        g.setColour (juce::Colours::white.withAlpha (0.12f)); g.strokePath (hose, juce::PathStrokeType (3.0f), juce::AffineTransform::translation (-1.5f, -1.5f));
    }
    // orecchie: cilindri con bulloni
    for (int s = -1; s <= 1; s += 2)
    {
        auto ear = juce::Rectangle<float> (cx + s * W2 * 1.02f - 16, top + 118, 32, 74);
        juce::ColourGradient eg (juce::Colour (0xff15161a), ear.getX(), 0, juce::Colour (0xff15161a), ear.getRight(), 0, false); eg.addColour (0.4, juce::Colour (0xffb9bdc6)); eg.addColour (0.55, juce::Colour (0xff4d5058));
        g.setGradientFill (eg); g.fillRoundedRectangle (ear, 8); rivet (g, ear.getCentreX(), ear.getY() + 14); rivet (g, ear.getCentreX(), ear.getBottom() - 14);
        g.setColour (RED2); g.fillRect (ear.getCentreX() - 1.5f, ear.getY() + 26, 3.0f, 22.0f);
    }
    g.setColour (juce::Colours::white.withAlpha (0.55f)); g.strokePath (head, juce::PathStrokeType (1.0f));
}

// =============================================================================================== sfondo
void Editor::drawStatic (juce::Graphics& g)
{
    juce::Random r (1984);
    g.fillAll (BLACK);
    // muro di cemento a lastre
    for (float x = 0; x < W; x += 118)
        for (float y = 56; y < H; y += 164)
        {
            const auto c = CONC.interpolatedWith (CONC2, r.nextFloat());
            g.setColour (c); g.fillRect (x + 1, y + 1, 116.0f, 162.0f);
            g.setColour (juce::Colours::black.withAlpha (0.6f)); g.drawRect (x, y, 118.0f, 164.0f, 1.0f);
            for (int k = 0; k < 4; ++k) g.setColour (juce::Colours::black.withAlpha (0.25f)), g.fillEllipse (x + 10 + (k % 2) * 92, y + 10 + (k / 2) * 140, 5, 5);
            for (int k = 0; k < 40; ++k) { g.setColour (juce::Colours::white.withAlpha (r.nextFloat() * 0.025f)); g.fillRect (x + r.nextFloat() * 116, y + r.nextFloat() * 162, 1.5f, 1.5f); }
            if (r.nextFloat() < 0.5f) { g.setColour (juce::Colours::black.withAlpha (0.35f)); const float sx = x + r.nextFloat() * 110; g.fillRect (sx, y + 2, 2.0f + r.nextFloat() * 3, 40 + r.nextFloat() * 120); }   // colature
        }
    // luce rossa dall'alto, smog
    { juce::ColourGradient lr (RED.withAlpha (0.16f), W * 0.5f, 56, RED.withAlpha (0.0f), W * 0.5f, 420, true); g.setGradientFill (lr); g.fillRect (0, 56, W, 420); }
    { juce::ColourGradient smog (juce::Colours::transparentBlack, 0, 380, juce::Colours::black.withAlpha (0.65f), 0, (float) H, false); g.setGradientFill (smog); g.fillRect (0, 380, W, H - 380); }
    // stendardi del regime ai lati della testa
    for (int s = 0; s < 2; ++s)
    {
        auto bn = juce::Rectangle<float> (s == 0 ? 378.0f : 746.0f, 64, 56, 400);
        juce::Path p; p.startNewSubPath (bn.getX(), bn.getY()); p.lineTo (bn.getRight(), bn.getY()); p.lineTo (bn.getRight(), bn.getBottom()); p.lineTo (bn.getCentreX(), bn.getBottom() - 22); p.lineTo (bn.getX(), bn.getBottom()); p.closeSubPath();
        { juce::ColourGradient bg2 (RED2.brighter (0.2f), bn.getX(), 0, RED2.darker (0.6f), bn.getRight(), 0, false); bg2.addColour (0.5, RED2.brighter (0.5f)); g.setGradientFill (bg2); g.fillPath (p); }
        for (float y = bn.getY(); y < bn.getBottom(); y += 3) { g.setColour (juce::Colours::black.withAlpha (0.08f)); g.drawHorizontalLine ((int) y, bn.getX(), bn.getRight()); }
        g.setColour (juce::Colours::black.withAlpha (0.8f)); g.fillRect (bn.getX() - 4, bn.getY() - 4, bn.getWidth() + 8, 6.0f);
        // simbolo: occhio dentro un cerchio
        const float ey = bn.getY() + 44; g.setColour (BLACK); g.drawEllipse (bn.getCentreX() - 18, ey - 18, 36, 36, 3); juce::Path eye; eye.startNewSubPath (bn.getCentreX() - 13, ey); eye.quadraticTo (bn.getCentreX(), ey - 11, bn.getCentreX() + 13, ey); eye.quadraticTo (bn.getCentreX(), ey + 11, bn.getCentreX() - 13, ey); g.fillPath (eye); g.setColour (RED2.brighter (0.3f)); g.fillEllipse (bn.getCentreX() - 4, ey - 4, 8, 8);
        const juce::String word = s == 0 ? "OBBEDISCI" : "ASCOLTA";
        g.setFont (heavy (22.0f, 0.0f)); g.setColour (BLACK.withAlpha (0.9f));
        for (int i = 0; i < word.length(); ++i) g.drawText (word.substring (i, i + 1), juce::Rectangle<float> (bn.getX(), bn.getY() + 80 + i * 30.0f, bn.getWidth(), 30), juce::Justification::centred);
    }
    // telecamere di sorveglianza
    for (int s = 0; s < 2; ++s)
    {
        const float x = s == 0 ? 330.0f : 850.0f, y = 74;
        g.setColour (juce::Colour (0xff1d1e22)); g.fillRect (x - 2, y - 14, 4.0f, 18.0f);
        juce::Path cam; cam.addRoundedRectangle (s == 0 ? x - 4 : x - 44, y, 48, 20, 4);
        juce::ColourGradient cg (juce::Colour (0xff3d3f46), x, y, juce::Colour (0xff111215), x, y + 20, false); g.setGradientFill (cg); g.fillPath (cam);
        g.setColour (BLACK); g.fillEllipse (s == 0 ? x + 34 : x - 42, y + 4, 12, 12); g.setColour (RED); g.fillEllipse (s == 0 ? x + 38 : x - 38, y + 8, 4, 4);
    }
    drawHead (g, HEAD);
    // pannelli di acciaio
    auto plate = [&] (juce::Rectangle<float> pr, const juce::String& title, const juce::String& code)
    {
        auto p = chamfer (pr, 16);
        g.setColour (juce::Colours::black.withAlpha (0.7f)); g.fillPath (p, juce::AffineTransform::translation (6, 8));
        { juce::ColourGradient pg (juce::Colour (0xff24252a), pr.getX(), pr.getY(), juce::Colour (0xff0d0d10), pr.getRight(), pr.getBottom(), false); g.setGradientFill (pg); g.fillPath (p); }
        g.saveState(); g.reduceClipRegion (p);
        for (int k = 0; k < 220; ++k) { g.setColour ((r.nextBool() ? juce::Colours::white : juce::Colours::black).withAlpha (0.03f + r.nextFloat() * 0.04f)); const float x0 = pr.getX() + r.nextFloat() * pr.getWidth(), y0 = pr.getY() + r.nextFloat() * pr.getHeight(); g.drawLine (x0, y0, x0 + r.nextFloat() * 26 - 13, y0 + r.nextFloat() * 4 - 2, 0.6f); }
        g.restoreState();
        g.setColour (juce::Colours::white.withAlpha (0.16f)); g.strokePath (p, juce::PathStrokeType (1.2f));
        g.setColour (RED.withAlpha (0.7f)); g.drawLine (pr.getX() + 16, pr.getY() + 1, pr.getRight() - 16, pr.getY() + 1, 2.0f);
        for (auto pt : { juce::Point<float> (pr.getX() + 12, pr.getY() + 26), juce::Point<float> (pr.getRight() - 12, pr.getY() + 26), juce::Point<float> (pr.getX() + 12, pr.getBottom() - 14), juce::Point<float> (pr.getRight() - 26, pr.getBottom() - 12) }) rivet (g, pt.x, pt.y);
        text (g, title, { pr.getX() + 26, pr.getY() + 12, pr.getWidth() - 52, 26 }, heavy (17.0f, 0.06f), INK, juce::Justification::centredLeft);
        text (g, code, { pr.getX() + 26, pr.getY() + 38, pr.getWidth() - 52, 14 }, mono (11.0f), RED.withAlpha (0.8f), juce::Justification::centredLeft);
        hazard (g, { pr.getRight() - 96, pr.getY() + 18, 64, 10 });
        g.setColour (juce::Colours::black.withAlpha (0.6f)); g.drawHorizontalLine ((int) pr.getY() + 58, pr.getX() + 20, pr.getRight() - 20); g.setColour (juce::Colours::white.withAlpha (0.08f)); g.drawHorizontalLine ((int) pr.getY() + 59, pr.getX() + 20, pr.getRight() - 20);
    };
    plate ({ 18, 80, 350, 620 }, "UNITA' CROMO", "MOD. 7-C // MODIFICA DELLA VOCE");
    plate ({ 812, 80, 350, 620 }, "PROTOCOLLO THROW", "AUTO // SI ATTIVA A FINE FRASE");
    // monitor
    { auto m = MON; g.setColour (juce::Colours::black.withAlpha (0.7f)); g.fillRect (m.translated (5, 7)); g.setColour (juce::Colour (0xff1a1b20)); g.fillRect (m.expanded (6)); g.setColour (juce::Colour (0xff050608)); g.fillRect (m);
      g.setColour (juce::Colours::white.withAlpha (0.14f)); g.drawRect (m.expanded (6), 1.0f); rivet (g, m.getX() - 1, m.getY() - 1, 2.4f); rivet (g, m.getRight() + 1, m.getY() - 1, 2.4f); }
    // console sotto
    { auto c = juce::Rectangle<float> (392, 622, 396, 82); auto p = chamfer (c, 10); { juce::ColourGradient pg (juce::Colour (0xff202126), 0, c.getY(), juce::Colour (0xff0b0b0e), 0, c.getBottom(), false); g.setGradientFill (pg); g.fillPath (p); } g.setColour (juce::Colours::white.withAlpha (0.14f)); g.strokePath (p, juce::PathStrokeType (1.0f)); }
    // barra in alto
    g.setColour (juce::Colour (0xff030304)); g.fillRect (0, 0, W, 56); g.setColour (RED.withAlpha (0.85f)); g.fillRect (0.0f, 55.0f, (float) W, 2.0f);
    glowText (g, "VOX CHROME", { 20, 6, 300, 34 }, heavy (26.0f, 0.02f), INK, 0.3f, juce::Justification::centredLeft);
    text (g, "JACKPOT CYBERNET  //  UNITA' VOCALE", { 22, 38, 300, 14 }, mono (11.0f), RED, juce::Justification::centredLeft);
    { auto pb = hotR[H_NAME].expanded (44, 4); auto p = chamfer (pb, 8); g.setColour (juce::Colour (0xff0b0b0d)); g.fillPath (p); g.setColour (RED.withAlpha (0.55f)); g.strokePath (p, juce::PathStrokeType (1.2f)); }
}
void Editor::drawLive (juce::Graphics& g)
{
    const float cx = HEAD.getCentreX(), top = HEAD.getY() + 6, W2 = HEAD.getWidth() * 0.36f;
    // occhi: la luce segue la voce, un punto rosso scorre come uno scanner
    auto slit = juce::Rectangle<float> (cx - W2 * 0.78f, top + 138, W2 * 1.56f, 26);
    const float e = juce::jlimit (0.0f, 1.0f, 0.22f + eye * 1.4f + flash);
    g.saveState(); g.reduceClipRegion (slit.toNearestInt());
    g.setColour (juce::Colour (0xff030304)); g.fillRect (slit);
    { juce::ColourGradient ig (RED.withAlpha (0.25f + 0.6f * e), slit.getCentreX(), slit.getCentreY(), RED.withAlpha (0.05f), slit.getX(), slit.getCentreY(), true); g.setGradientFill (ig); g.fillRect (slit); }
    const float sx = slit.getCentreX() + std::sin (tick * 0.07f) * slit.getWidth() * 0.4f;
    { juce::ColourGradient dot (juce::Colours::white.withAlpha (0.9f * e), sx, slit.getCentreY(), RED.withAlpha (0.0f), sx + 40, slit.getCentreY(), true); g.setGradientFill (dot); g.fillEllipse (sx - 40, slit.getY() - 6, 80, slit.getHeight() + 12); }
    for (float y = slit.getY(); y < slit.getBottom(); y += 3) { g.setColour (juce::Colours::black.withAlpha (0.35f)); g.drawHorizontalLine ((int) y, slit.getX(), slit.getRight()); }
    g.restoreState();
    { juce::ColourGradient halo (RED.withAlpha (0.18f * e), slit.getCentreX(), slit.getCentreY(), RED.withAlpha (0.0f), slit.getCentreX(), slit.getY() - 50, true); g.setGradientFill (halo); g.fillEllipse (slit.expanded (40, 40)); }
    // throw: la testa glitcha
    if (flash > 0.05f && ! root.bg.isNull())
    {
        juce::Random r (tick); const float s = root.bgScale;
        for (int i = 0; i < 6; ++i)
        {
            const float y = HEAD.getY() + r.nextFloat() * HEAD.getHeight(), hh = 4 + r.nextFloat() * 16, dx = (r.nextFloat() - 0.5f) * 40 * flash;
            g.drawImage (root.bg, (int) (HEAD.getX() + dx), (int) y, (int) HEAD.getWidth(), (int) hh, (int) (HEAD.getX() * s), (int) (y * s), (int) (HEAD.getWidth() * s), (int) (hh * s));
            g.setColour (RED.withAlpha (0.25f * flash)); g.fillRect (HEAD.getX() + dx, y, HEAD.getWidth(), hh);
        }
    }
    // monitor: frasi e throw
    auto m = MON.reduced (8, 6);
    g.saveState(); g.reduceClipRegion (MON.toNearestInt());
    g.setColour (juce::Colour (0xff050608)); g.fillRect (MON);
    g.setColour (RED.withAlpha (0.07f)); for (float x = m.getX(); x < m.getRight(); x += 22) g.drawVerticalLine ((int) x, m.getY(), m.getBottom()); for (float y = m.getY(); y < m.getBottom(); y += 22) g.drawHorizontalLine ((int) y, m.getX(), m.getRight());
    text (g, "MONITOR FRASI", { m.getX(), m.getY(), 160, 14 }, mono (11.0f), DIM, juce::Justification::centredLeft);
    const int N = (int) envHist.size();
    const float thr = juce::Decibels::decibelsToGain (-18.0f - 30.0f * proc.value ("sens"));
    auto yOf = [&] (float v) { const float db = juce::jlimit (-60.0f, 0.0f, juce::Decibels::gainToDecibels (v, -60.0f)); return m.getBottom() - (db + 60.0f) / 60.0f * (m.getHeight() - 18); };
    juce::Path pth; pth.startNewSubPath (m.getX(), m.getBottom());
    for (int i = 0; i < N; ++i) { const float x = m.getX() + m.getWidth() * i / (float) (N - 1); pth.lineTo (x, yOf (envHist[(size_t) i])); }
    pth.lineTo (m.getRight(), m.getBottom()); pth.closeSubPath();
    g.setColour (INK.withAlpha (0.14f)); g.fillPath (pth); g.setColour (INK.withAlpha (0.7f)); g.strokePath (pth, juce::PathStrokeType (1.1f));
    { const float ty = yOf (thr); juce::Path d; for (float x = m.getX(); x < m.getRight(); x += 8) { d.startNewSubPath (x, ty); d.lineTo (x + 4, ty); } g.setColour (RED.withAlpha (0.8f)); g.strokePath (d, juce::PathStrokeType (1.0f)); text (g, "SOGLIA", { m.getRight() - 60, ty - 14, 58, 12 }, mono (10.0f), RED, juce::Justification::centredRight); }
    for (int i = 0; i < N; ++i) if (throwHist[(size_t) i])
    {
        const float x = m.getX() + m.getWidth() * i / (float) (N - 1);
        g.setColour (RED.withAlpha (0.25f)); g.fillRect (x - 3, m.getY() + 14, 6.0f, m.getHeight() - 14); g.setColour (RED); g.fillRect (x - 0.75f, m.getY() + 14, 1.5f, m.getHeight() - 14);
        text (g, "THROW", { x - 24, m.getY() + 14, 48, 12 }, mono (10.0f), RED);
    }
    const bool any = proc.value ("eco") + proc.value ("space") + proc.value ("glitch") > 0.001f;
    text (g, ! any ? "THROW SPENTI" : (gateNow > 0.5f ? "THROW APERTO" : "IN ASCOLTO"), { m.getRight() - 160, m.getY(), 160, 14 }, mono (11.0f), ! any ? DIM : (gateNow > 0.5f ? RED : INK), juce::Justification::centredRight);
    for (float y = MON.getY(); y < MON.getBottom(); y += 3) { g.setColour (juce::Colours::black.withAlpha (0.22f)); g.drawHorizontalLine ((int) y, MON.getX(), MON.getRight()); }
    g.restoreState();
    // striscia degli avvisi che scorre
    {
        auto tk = juce::Rectangle<float> (0, 57, (float) W, 16);
        g.setColour (juce::Colour (0xff120305)); g.fillRect (tk);
        const juce::String msg = "AVVISO: OGNI VOCE E' REGISTRATA   ///   COPRIFUOCO ALLE 23:00   ///   LA FREQUENZA E' PROPRIETA' DEL SISTEMA   ///   SEGNALA OGNI TRASMISSIONE NON AUTORIZZATA   ///   ";
        g.setFont (mono (11.0f)); g.setColour (RED.withAlpha (0.85f));
        const float tw = juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), msg);
        for (float x = -std::fmod (ticker, tw); x < W; x += tw) g.drawText (msg, juce::Rectangle<float> (x, tk.getY(), tw, tk.getHeight()), juce::Justification::centredLeft);
    }
}

// =============================================================================================== radice
void Root::paint (juce::Graphics& g)
{
    const float s = g.getInternalContext().getPhysicalPixelScaleFactor();
    if (bg.isNull() || std::abs (bgScale - s) > 0.01f)
    {
        bgScale = s; bg = juce::Image (juce::Image::RGB, juce::jmax (1, juce::roundToInt (Editor::W * s)), juce::jmax (1, juce::roundToInt (Editor::H * s)), true);
        juce::Graphics ig (bg); ig.addTransform (juce::AffineTransform::scale (s)); ed.drawStatic (ig);
    }
    g.drawImage (bg, getLocalBounds().toFloat());
    auto R = [this] (int h) { return ed.hotR[h]; };
    text (g, "<", R (Editor::H_PREV), raj (26.0f), hot == Editor::H_PREV ? juce::Colours::white : RED);
    text (g, ">", R (Editor::H_NEXT), raj (26.0f), hot == Editor::H_NEXT ? juce::Colours::white : RED);
    glowText (g, ed.proc.currentName().toUpperCase(), R (Editor::H_NAME), raj (24.0f, 0.16f), hot == Editor::H_NAME ? juce::Colours::white : INK, 0.0f);
    text (g, "SALVA", R (Editor::H_SAVE), raj (17.0f, 0.14f), hot == Editor::H_SAVE ? juce::Colours::white : RED);
    text (g, "ANNULLA", R (Editor::H_UNDO), raj (16.0f, 0.1f), ed.proc.undo.canUndo() ? (hot == Editor::H_UNDO ? juce::Colours::white : INK) : DIM.withAlpha (0.6f));
    text (g, "RIPETI", R (Editor::H_REDO), raj (16.0f, 0.1f), ed.proc.undo.canRedo() ? (hot == Editor::H_REDO ? juce::Colours::white : INK) : DIM.withAlpha (0.6f));
    ed.drawLive (g);
}
void Root::mouseDown (const juce::MouseEvent& e) { for (int h = 0; h < Editor::H_COUNT; ++h) if (ed.hotR[h].contains (e.position)) { ed.click (h); return; } }
void Root::mouseMove (const juce::MouseEvent& e)
{
    int h = -1; for (int i = 0; i < Editor::H_COUNT; ++i) if (ed.hotR[i].contains (e.position)) { h = i; break; }
    if (h != hot) { hot = h; repaint (0, 0, Editor::W, 56); setMouseCursor (h >= 0 ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor); }
}

// =============================================================================================== editor
Editor::Editor (Processor& p) : juce::AudioProcessorEditor (p), proc (p)
{
    setWantsKeyboardFocus (true);
    addAndMakeVisible (root);
    envHist.assign (160, 0.0f); throwHist.assign (160, false);
    auto pct = [] (float v) { return v < 0.002f ? juce::String ("SPENTO") : juce::String (juce::roundToInt (v * 100)) + "%"; };
    struct K { const char* id; const char* l; std::function<juce::String (float)> f; const char* tip; bool bip; int steps; };
    const K ks[] = {
        { "cromo", "CROMO", pct, "Risonanze metalliche: la voce suona dentro tubi di cromo", false, 0 },
        { "robot", "ROBOT", pct, "Modulatore ad anello: la voce diventa macchina", false, 0 },
        { "shift", "SPOSTA", [] (float v) { return std::abs (v) < 0.004f ? juce::String ("ZERO") : juce::String (v > 0 ? "+" : "") + juce::String (juce::roundToInt (v * 260)) + " Hz"; }, "Sposta tutte le frequenze (non e' un pitch: suona inumano)", true, 0 },
        { "bit", "BIT", pct, "Riduce bit e campioni: voce digitale e corrotta", false, 0 },
        { "drive", "DRIVE", pct, "Saturazione sporca", false, 0 },
        { "band", "BANDA", pct, "Stringe la voce come un altoparlante del regime", false, 0 },
        { "eco", "ECO", pct, "Eco a tempo che parte da sola sull'ultima parola di ogni frase", false, 0 },
        { "space", "SPAZIO", pct, "Riverbero scuro che si apre a fine frase", false, 0 },
        { "glitch", "GLITCH", pct, "L'ultima sillaba si ripete e rallenta, come una macchina che si spegne", false, 0 },
        { "div", "TEMPO", [] (float v) { return juce::String (divName (juce::roundToInt (v))); }, "Tempo dell'eco (segue il BPM)", false, 6 },
        { "fb", "RIPETIZ.", [] (float v) { return juce::String (juce::roundToInt (v / 0.9f * 100)) + "%"; }, "Quante volte l'eco si ripete", false, 0 },
        { "sens", "SOGLIA", [] (float v) { return juce::String (juce::roundToInt (-18 - 30 * v)) + " dB"; }, "Sotto questo livello la frase e' finita e partono i throw. Piu' basso = piu' sensibile", false, 0 },
        { "width", "LARGHEZZA", pct, "Doppi cyborg ai lati della voce", false, 0 },
        { "out", "USCITA", [] (float v) { return juce::String (v, 1) + " dB"; }, "Volume d'uscita", false, 0 },
    };
    for (auto& k : ks)
    {
        auto kn = std::make_unique<Knob> (*this, k.id, k.l, k.f); kn->setTooltip (k.tip); kn->bipolar = k.bip; kn->steps = k.steps;
        root.addAndMakeVisible (*kn); knobs.push_back (std::move (kn));
    }
    for (int j = 0; j < 6; ++j) knobs[(size_t) j]->setBounds (36 + (j % 2) * 160, 150 + (j / 2) * 180, 150, 172);
    for (int j = 0; j < 6; ++j) knobs[(size_t) (6 + j)]->setBounds (830 + (j % 2) * 160, 150 + (j / 2) * 180, 150, 172);
    knobs[12]->setBounds (410, 624, 176, 80); knobs[13]->setBounds (598, 624, 176, 80);
    root.addAndMakeVisible (sizeChip);
    auto userFiles = [] { auto f = Processor::presetFolder().findChildFiles (juce::File::findFiles, false, "*.voxchrome"); f.sort(); return f; };
    browser.entries = [userFiles]
    {
        std::vector<jcyber::Entry> v;
        for (int i = 0; i < (int) presets().size(); ++i) v.push_back ({ presets()[(size_t) i].cat, presets()[(size_t) i].name, presets()[(size_t) i].note, i, false });
        auto files = userFiles(); for (int i = 0; i < files.size(); ++i) v.push_back ({ "I MIEI", files[i].getFileNameWithoutExtension(), "preset salvato da te", 1000 + i, true });
        return v;
    };
    browser.current = [this, userFiles] { if (proc.userName.isEmpty()) return proc.preset; auto files = userFiles(); for (int i = 0; i < files.size(); ++i) if (files[i].getFileNameWithoutExtension() == juce::File::createLegalFileName (proc.userName)) return 1000 + i; return -1; };
    browser.load = [this, userFiles] (const jcyber::Entry& e)
    {
        if (e.user) { auto files = userFiles(); if (juce::isPositiveAndBelow (e.id - 1000, files.size())) proc.loadUserPreset (files[e.id - 1000]); }
        else proc.loadPreset (e.id);
        root.repaint();
    };
    browser.save = [this] { askSave(); };
    browser.canOverwrite = [this] { return proc.userName.isNotEmpty(); };
    browser.overwrite = [this] { proc.saveUserPreset (proc.userName); };
    browser.folder = [] { Processor::presetFolder().revealToUser(); };
    browser.remove = [userFiles] (const jcyber::Entry& e) { auto files = userFiles(); if (juce::isPositiveAndBelow (e.id - 1000, files.size())) files[e.id - 1000].moveToTrash(); };
    browser.onClose = [this] { root.repaint(); };
    browser.title = "ARCHIVIO // VOX"; browser.sub = "PROFILI VOCALI AUTORIZZATI";
    browser.titleFont = heavy (26.0f); browser.nameFont = raj (26.0f, 0.1f); browser.monoFont = mono (12.0f);
    browser.accent = RED; browser.accent2 = INK; browser.ink = INK; browser.dim = DIM;
    browser.bg0 = juce::Colour (0xf6040405); browser.bg1 = juce::Colour (0xf6141416);
    root.addChildComponent (browser);
    hotR[H_PREV] = { 420, 12, 30, 32 }; hotR[H_NEXT] = { 766, 12, 30, 32 }; hotR[H_NAME] = { 452, 10, 312, 36 };
    hotR[H_SAVE] = { 828, 16, 70, 24 }; hotR[H_UNDO] = { 904, 16, 84, 24 }; hotR[H_REDO] = { 990, 16, 74, 24 };
    sizeChip.setBounds (1104, 20, 58, 15);
    browser.setBounds (0, 0, W, H);
    scaler.setup();
    startTimerHz (30);
}
void Editor::resized()
{
    const float s = scaler.update();
    root.setBounds (0, 0, W, H); root.setTransform (juce::AffineTransform::scale (s));
}
bool Editor::keyPressed (const juce::KeyPress& k)
{
    if (k.getModifiers().isCommandDown() && (k.getKeyCode() == 'Z' || k.getKeyCode() == 'z')) { if (k.getModifiers().isShiftDown()) proc.undo.redo(); else proc.undo.undo(); root.repaint(); return true; }
    return false;
}
void Editor::click (int h)
{
    switch (h)
    {
        case H_PREV: case H_NEXT: { const int n = (int) presets().size(); proc.loadPreset ((juce::jmax (0, proc.preset) + (h == H_NEXT ? 1 : n - 1)) % n); break; }
        case H_NAME: browser.open(); break;
        case H_SAVE: askSave(); break;
        case H_UNDO: proc.undo.undo(); break;
        case H_REDO: proc.undo.redo(); break;
        default: break;
    }
    root.repaint(); for (auto& k : knobs) k->repaint();
}
void Editor::askSave()
{
    auto* w = new juce::AlertWindow ("SALVA IL PRESET", "Nome del preset", juce::MessageBoxIconType::NoIcon, this);
    w->addTextEditor ("n", proc.currentName()); w->addButton ("SALVA", 1, juce::KeyPress (juce::KeyPress::returnKey)); w->addButton ("ANNULLA", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    juce::Component::SafePointer<Editor> safe (this);
    w->enterModalState (true, juce::ModalCallbackFunction::create ([safe, w] (int r)
    {
        if (safe != nullptr && r == 1) { const auto n = w->getTextEditorContents ("n").trim(); if (n.isNotEmpty()) safe->proc.saveUserPreset (n); safe->root.repaint(); safe->browser.refresh(); }
    }), true);
}
void Editor::timerCallback()
{
    ++tick; ticker += 1.4f;
    const float ev = proc.eng.uiEnv.load(); eye = juce::jmax (std::sqrt (ev), eye * 0.85f);
    const int th = proc.eng.uiThrows.load(); const bool fired = th != lastThrows; lastThrows = th;
    if (fired) flash = 1.0f; else flash *= 0.8f;
    gateNow = proc.eng.uiGate.load();
    envHist.erase (envHist.begin()); envHist.push_back (ev);
    throwHist.erase (throwHist.begin()); throwHist.push_back (fired);
    if (! browser.isVisible())
    {
        root.repaint (HEAD.expanded (30, 0).toNearestInt());
        root.repaint (MON.toNearestInt());
        root.repaint (0, 57, W, 16);
    }
    if (proc.preset != lastPreset || proc.userName != lastUser) { lastPreset = proc.preset; lastUser = proc.userName; root.repaint (0, 0, W, 56); }
    if (tick % 8 == 0) root.repaint (hotR[H_UNDO].getUnion (hotR[H_REDO]).toNearestInt());
    for (auto& k : knobs) { const float v = k->prm->getValue(); if (v != k->last) { k->last = v; k->repaint(); } }
}
}
