#include "PhantomEditor.h"

namespace jphn
{
using namespace ui;
namespace
{
void text (juce::Graphics& g, const juce::String& t, juce::Rectangle<float> r, juce::Font f, juce::Colour c, juce::Justification j = juce::Justification::centred)
{ g.setFont (f); g.setColour (c); g.drawFittedText (t, r.toNearestInt(), j, 1, 0.6f); }
void glowText (juce::Graphics& g, const juce::String& t, juce::Rectangle<float> r, juce::Font f, juce::Colour c, float on, juce::Justification j = juce::Justification::centred)
{
    g.setFont (f);
    for (int i = 3; i >= 1 && on > 0.01f; --i) { g.setColour (c.withAlpha (0.09f * on)); for (int a = 0; a < 8; ++a) g.drawFittedText (t, r.translated (std::cos (a * 0.785f) * i * 1.4f, std::sin (a * 0.785f) * i * 1.4f).toNearestInt(), j, 1, 0.6f); }
    g.setColour (on > 0.01f ? c.interpolatedWith (juce::Colours::white, 0.3f * on) : c); g.drawFittedText (t, r.toNearestInt(), j, 1, 0.6f);
}
constexpr float GROUND = 330, BOTTOM = 418;
const juce::Colour GC[kGhosts] = { juce::Colour (0xffb8f6ff), juce::Colour (0xff7b4dff), juce::Colour (0xffff2e97), juce::Colour (0xff22f3dc) };
}

// =============================================================================================== manopole: disco scuro, anello rosa, lancetta ciano
Knob::Knob (Editor& e, const juce::String& id, juce::String l, std::function<juce::String (float)> fm)
    : ed (e), prm (e.P (id)), label (std::move (l)), fmt (std::move (fm))
{ setRepaintsOnMouseActivity (true); setMouseCursor (juce::MouseCursor::UpDownResizeCursor); }
void Knob::paint (juce::Graphics& g)
{
    const float w = (float) getWidth(), h = (float) getHeight(), v = prm->getValue(), a0 = -2.4f, a1 = 2.4f, av = a0 + (a1 - a0) * v;
    const bool hov = isMouseOverOrDragging();
    const juce::String t = fmt ? fmt (prm->convertFrom0to1 (v)) : prm->getCurrentValueAsText();
    const float lh = 18, vh = 20, S = juce::jmin (w - 26, h - lh - vh - 12), cx = w * 0.5f, cy = lh + 6 + S * 0.5f, R = S * 0.5f;
    text (g, label, { -10, 0, w + 20, lh }, raj (15.0f, 0.14f), hov ? juce::Colours::white : INK.withAlpha (0.85f));
    { juce::Path bgArc; bgArc.addCentredArc (cx, cy, R + 5, R + 5, 0, a0, a1, true); g.setColour (juce::Colour (0xff281a3a)); g.strokePath (bgArc, juce::PathStrokeType (3.5f)); }
    if (v > 0.002f) { juce::Path arc; arc.addCentredArc (cx, cy, R + 5, R + 5, 0, a0, av, true); g.setColour (col.withAlpha (0.3f)); g.strokePath (arc, juce::PathStrokeType (9.0f)); g.setColour (col); g.strokePath (arc, juce::PathStrokeType (3.5f)); }
    g.setColour (juce::Colours::black.withAlpha (0.6f)); g.fillEllipse (cx - R + 2, cy - R + 5, R * 2, R * 2);
    { juce::ColourGradient body (juce::Colour (0xff3a2a52), cx, cy - R, juce::Colour (0xff0c0816), cx, cy + R, false); g.setGradientFill (body); g.fillEllipse (cx - R, cy - R, R * 2, R * 2); }
    g.setColour (TEAL.withAlpha (0.18f)); g.drawEllipse (cx - R, cy - R, R * 2, R * 2, 1.0f);
    const float rc = R * 0.68f;
    { juce::ColourGradient cap (juce::Colour (0xff261b36), cx - rc * 0.5f, cy - rc, juce::Colour (0xff07050c), cx + rc * 0.5f, cy + rc, false); g.setGradientFill (cap); g.fillEllipse (cx - rc, cy - rc, rc * 2, rc * 2); }
    { const juce::Point<float> p0 (cx + std::sin (av) * rc * 0.2f, cy - std::cos (av) * rc * 0.2f), p1 (cx + std::sin (av) * (R - 2), cy - std::cos (av) * (R - 2));
      g.setColour (TEAL.withAlpha (0.35f)); g.drawLine ({ p0, p1 }, 6.0f); g.setColour (juce::Colours::white.interpolatedWith (TEAL, 0.5f)); g.drawLine ({ p0, p1 }, 2.2f); }
    text (g, t, { -10, h - vh, w + 20, vh }, vt (20.0f), hov ? juce::Colours::white : (v > 0.002f ? col.interpolatedWith (INK, 0.35f) : DIM));
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
    prm->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, prm->getValue() + juce::jlimit (-0.05f, 0.05f, dy * 0.08f)));
}

// =============================================================================================== la figura col cappuccio
juce::Path Editor::figure (juce::Rectangle<float> b, bool flip)
{
    // coordinate 0..1 nella scatola: cappuccio, spalle larghe, giacca, gambe
    juce::Path p;
    auto P = [&] (float x, float y) { return juce::Point<float> (b.getX() + (flip ? 1 - x : x) * b.getWidth(), b.getY() + y * b.getHeight()); };
    auto M = [&] (float x, float y) { p.startNewSubPath (P (x, y)); };
    auto Lt = [&] (float x, float y) { p.lineTo (P (x, y)); };
    auto Q = [&] (float cx, float cy, float x, float y) { p.quadraticTo (P (cx, cy), P (x, y)); };
    M (0.5f, 0.0f);
    Q (0.66f, 0.0f, 0.67f, 0.12f); Q (0.68f, 0.2f, 0.62f, 0.24f);           // cappuccio
    Q (0.8f, 0.26f, 0.9f, 0.31f); Q (0.97f, 0.36f, 0.95f, 0.5f);            // spalla
    Lt (0.9f, 0.6f); Lt (0.84f, 0.62f); Lt (0.82f, 0.56f);                   // braccio, mano in tasca
    Lt (0.78f, 0.64f); Lt (0.74f, 0.66f);
    Lt (0.7f, 0.98f); Lt (0.56f, 1.0f); Lt (0.53f, 0.72f);                   // gamba destra
    Lt (0.47f, 0.72f); Lt (0.44f, 1.0f); Lt (0.3f, 0.98f);                   // gamba sinistra
    Lt (0.26f, 0.66f); Lt (0.22f, 0.64f); Lt (0.18f, 0.56f); Lt (0.16f, 0.62f); Lt (0.1f, 0.6f);
    Lt (0.05f, 0.5f); Q (0.03f, 0.36f, 0.1f, 0.31f); Q (0.2f, 0.26f, 0.38f, 0.24f);
    Q (0.32f, 0.2f, 0.33f, 0.12f); Q (0.34f, 0.0f, 0.5f, 0.0f);
    p.closeSubPath();
    return p;
}
juce::Rectangle<float> Editor::ghostBox (int k)
{
    static const juce::Rectangle<float> r[kGhosts] = { { 330, 146, 120, 204 }, { 770, 128, 140, 232 }, { 240, 186, 86, 150 }, { 600, 64, 150, 250 } };
    return r[juce::jlimit (0, kGhosts - 1, k)];
}
juce::Rectangle<float> Editor::tvRect (int k) { return { 14.0f + k * 192.0f, BOTTOM + 10, 184, 284 }; }

// =============================================================================================== sfondo: la strada magenta
void Editor::drawStatic (juce::Graphics& g)
{
    juce::Random r (1999);
    const float vx = W * 0.5f, vy = 228;
    // cielo rosa che sfuma nel viola
    { juce::ColourGradient sky (juce::Colour (0xff2a0a3c), 0, 56, MAG, 0, vy + 20, false); sky.addColour (0.55, juce::Colour (0xffc0207a)); g.setGradientFill (sky); g.fillRect (0.0f, 56.0f, (float) W, GROUND - 56); }
    { juce::ColourGradient sun (PINK.withAlpha (0.55f), vx, vy, PINK.withAlpha (0.0f), vx, vy - 140, true); g.setGradientFill (sun); g.fillEllipse (vx - 220, vy - 140, 440, 280); }
    // marciapiedi e asfalto sotto i palazzi
    { juce::Path gr; gr.startNewSubPath (0, GROUND - 6); gr.lineTo (vx - 60, vy + 8); gr.lineTo (vx + 60, vy + 8); gr.lineTo ((float) W, GROUND - 6); gr.lineTo ((float) W, BOTTOM); gr.lineTo (0, BOTTOM); gr.closeSubPath();
      juce::ColourGradient gg (juce::Colour (0xff2a0c36), 0, vy, juce::Colour (0xff0b0614), 0, BOTTOM, false); g.setGradientFill (gg); g.fillPath (gr); }
    // palazzi in prospettiva, ai lati, che vanno verso il punto di fuga
    for (int side = 0; side < 2; ++side)
    {
        const float sgn = side == 0 ? -1.0f : 1.0f;
        for (int i = 0; i < 9; ++i)
        {
            const float t0 = i / 9.0f, t1 = (i + 1) / 9.0f;
            const float xNear0 = vx + sgn * (60 + 560 * (1 - t0)), xNear1 = vx + sgn * (60 + 560 * (1 - t1));
            const float topY0 = vy - 30 - (1 - t0) * (140 + r.nextFloat() * 60), topY1 = vy - 30 - (1 - t1) * (140 + r.nextFloat() * 60);
            const float botY0 = vy + (GROUND - vy) * (1 - t0) * 0.9f + 8, botY1 = vy + (GROUND - vy) * (1 - t1) * 0.9f + 8;
            juce::Path fac; fac.startNewSubPath (xNear0, juce::jmax (40.0f, topY0 - 120)); fac.lineTo (xNear1, juce::jmax (40.0f, topY1 - 120)); fac.lineTo (xNear1, botY1); fac.lineTo (xNear0, botY0); fac.closeSubPath();
            const auto base = juce::Colour (0xff0b1a2e).interpolatedWith (juce::Colour (0xff3a0c40), t0 * 0.6f);
            g.setColour (base); g.fillPath (fac);
            // finestre ciano / rosa
            for (int k = 0; k < 26; ++k)
            {
                const float u = r.nextFloat(), vv = r.nextFloat();
                const float x = xNear0 + (xNear1 - xNear0) * u, yt = juce::jmax (40.0f, topY0 - 120) + (juce::jmax (40.0f, topY1 - 120) - juce::jmax (40.0f, topY0 - 120)) * u, yb = botY0 + (botY1 - botY0) * u;
                const float y = yt + (yb - yt) * vv, s = 1.5f + (1 - t0) * 3.5f;
                g.setColour ((r.nextFloat() < 0.6f ? TEAL : PINK).withAlpha (0.25f + r.nextFloat() * 0.5f)); g.fillRect (x, y, s, s * 1.4f);
            }
            g.setColour (juce::Colours::black.withAlpha (0.35f)); g.strokePath (fac, juce::PathStrokeType (1.0f));
            // insegne verticali ciano
            if (r.nextFloat() < 0.35f)
            {
                const float x = xNear0 + (xNear1 - xNear0) * 0.5f, y = vy - 40 - (1 - t0) * 60, hh = 30 + (1 - t0) * 70, ww = 4 + (1 - t0) * 8;
                const auto c = r.nextBool() ? TEAL : PINK;
                g.setColour (c.withAlpha (0.18f)); g.fillRect (x - ww, y - 6, ww * 3, hh + 12); g.setColour (c); g.fillRect (x, y, ww, hh);
            }
        }
    }
    // cavi elettrici che attraversano la strada
    for (int i = 0; i < 6; ++i)
    {
        const float y0 = 70 + i * 22 + r.nextFloat() * 10, y1 = 60 + r.nextFloat() * 120;
        juce::Path w; w.startNewSubPath (0, y0); w.quadraticTo (W * 0.5f, y0 + 30 + r.nextFloat() * 40, (float) W, y1);
        g.setColour (juce::Colours::black.withAlpha (0.75f)); g.strokePath (w, juce::PathStrokeType (1.3f));
    }
    // strada bagnata con riflessi
    { juce::ColourGradient st (juce::Colour (0xff3a0f45), 0, vy + 10, juce::Colour (0xff0a0614), 0, BOTTOM, false); g.setGradientFill (st);
      juce::Path road; road.startNewSubPath (vx - 40, vy + 10); road.lineTo (vx + 40, vy + 10); road.lineTo ((float) W, BOTTOM); road.lineTo (0, BOTTOM); road.closeSubPath(); g.fillPath (road); }
    for (int i = 0; i < 40; ++i) { const float x = vx + (r.nextFloat() - 0.5f) * 900, y = GROUND - 30 + r.nextFloat() * (BOTTOM - GROUND + 30); g.setColour ((r.nextBool() ? MAG : TEAL).withAlpha (0.08f + r.nextFloat() * 0.15f)); g.fillRect (x, y, 2.0f + r.nextFloat() * 4, 10 + r.nextFloat() * 40); }
    for (int i = 0; i < 6; ++i) { const float t = i / 6.0f; const float y = vy + 20 + (BOTTOM - vy) * t * t; g.setColour (PINK.withAlpha (0.12f)); g.drawLine (vx - 4 - 600 * t * t * 0.2f, y, vx + 4 + 600 * t * t * 0.2f, y, 1.0f + t * 2); }
    // macchina sportiva a sinistra (solo sagoma con i neon ciano)
    {
        juce::Path car; const float cx = 14, cy = 396;
        car.startNewSubPath (cx, cy); car.lineTo (cx + 20, cy - 22); car.lineTo (cx + 90, cy - 30); car.lineTo (cx + 130, cy - 52); car.lineTo (cx + 200, cy - 54); car.lineTo (cx + 240, cy - 30);
        car.lineTo (cx + 270, cy - 24); car.lineTo (cx + 272, cy); car.closeSubPath();
        g.setColour (juce::Colour (0xff0a0612)); g.fillPath (car);
        g.setColour (TEAL.withAlpha (0.25f)); g.strokePath (car, juce::PathStrokeType (6.0f)); g.setColour (TEAL); g.strokePath (car, juce::PathStrokeType (1.6f));
        g.setColour (MAG); g.fillRect (cx + 6, cy - 16, 40.0f, 4.0f); g.setColour (MAG.withAlpha (0.3f)); g.fillRect (cx - 4, cy - 22, 60.0f, 16.0f);
        for (float wx : { cx + 52, cx + 214 }) { g.setColour (juce::Colours::black); g.fillEllipse (wx - 18, cy - 14, 36, 36); g.setColour (TEAL.withAlpha (0.6f)); g.drawEllipse (wx - 12, cy - 8, 24, 24, 1.5f); }
        g.setColour (TEAL.withAlpha (0.12f)); g.fillRect (cx, cy + 20, 272.0f, 24.0f);
    }
    // scanline su tutta la scena
    for (float y = 56; y < BOTTOM; y += 3) { g.setColour (juce::Colours::black.withAlpha (0.12f)); g.drawHorizontalLine ((int) y, 0, (float) W); }
    // stanza in basso: scrivania buia
    { juce::ColourGradient dk (juce::Colour (0xff0e0818), 0, BOTTOM, juce::Colour (0xff040308), 0, (float) H, false); g.setGradientFill (dk); g.fillRect (0.0f, BOTTOM, (float) W, H - BOTTOM); }
    g.setColour (MAG.withAlpha (0.7f)); g.fillRect (0.0f, BOTTOM, (float) W, 2.0f);
    // televisori
    for (int k = 0; k < kGhosts; ++k)
    {
        auto tv = tvRect (k);
        g.setColour (juce::Colours::black.withAlpha (0.6f)); g.fillRoundedRectangle (tv.translated (4, 6), 12);
        { juce::ColourGradient cg (juce::Colour (0xff2c2236), tv.getX(), tv.getY(), juce::Colour (0xff120c1a), tv.getX(), tv.getBottom(), false); g.setGradientFill (cg); g.fillRoundedRectangle (tv, 12); }
        g.setColour (juce::Colours::white.withAlpha (0.1f)); g.drawRoundedRectangle (tv, 12, 1.2f);
        auto scr = juce::Rectangle<float> (tv.getX() + 12, tv.getY() + 12, tv.getWidth() - 24, 112);
        g.setColour (juce::Colours::black); g.fillRoundedRectangle (scr.expanded (4), 16);
        g.setColour (juce::Colour (0xff1b1226)); g.drawRoundedRectangle (scr.expanded (4), 16, 3.0f);
        // targhetta e griglia dell'altoparlante
        for (int i = 0; i < 5; ++i) { g.setColour (juce::Colours::black.withAlpha (0.5f)); g.fillRoundedRectangle (tv.getX() + 14 + i * 7.0f, tv.getBottom() - 16, 4, 8, 2); }
        g.setColour (DIM); g.setFont (vt (15.0f)); g.drawText ("CH-0" + juce::String (k + 1), juce::Rectangle<float> (tv.getRight() - 70, tv.getBottom() - 20, 60, 14), juce::Justification::centredRight);
    }
    // impianto a destra
    {
        auto pr = juce::Rectangle<float> (786, BOTTOM + 10, 380, 284);
        g.setColour (juce::Colours::black.withAlpha (0.6f)); g.fillRoundedRectangle (pr.translated (4, 6), 10);
        { juce::ColourGradient cg (juce::Colour (0xff1e1630), pr.getX(), pr.getY(), juce::Colour (0xff0b0712), pr.getX(), pr.getBottom(), false); g.setGradientFill (cg); g.fillRoundedRectangle (pr, 10); }
        g.setColour (TEAL.withAlpha (0.35f)); g.drawRoundedRectangle (pr, 10, 1.2f);
        text (g, "IMPIANTO  //  TUTTI I FANTASMI", { pr.getX() + 16, pr.getY() + 8, pr.getWidth() - 32, 18 }, vt (19.0f), TEAL, juce::Justification::centredLeft);
        g.setColour (TEAL.withAlpha (0.25f)); g.drawHorizontalLine ((int) pr.getY() + 30, pr.getX() + 14, pr.getRight() - 14);
    }
    // barra in alto
    { juce::ColourGradient bar (juce::Colour (0xff0b0614), 0, 0, juce::Colour (0xff160a24), 0, 56, false); g.setGradientFill (bar); g.fillRect (0, 0, W, 56); }
    g.setColour (MAG); g.fillRect (0.0f, 54.0f, (float) W, 2.0f);
    glowText (g, "PHANTOM", { 20, 6, 260, 34 }, krona (26.0f, 0.12f), PINK, 0.6f, juce::Justification::centredLeft);
    text (g, "JACKPOT CYBERNET  //  VOCI FANTASMA", { 22, 38, 300, 14 }, vt (16.0f), TEAL, juce::Justification::centredLeft);
    { auto pb = hotR[H_NAME].expanded (44, 4); g.setColour (juce::Colour (0xff07040c)); g.fillRoundedRectangle (pb, 6); g.setColour (TEAL.withAlpha (0.5f)); g.drawRoundedRectangle (pb, 6, 1.2f); }
}

void Editor::drawLive (juce::Graphics& g)
{
    // fantasmi: trasparenti, doppio contorno rosa/ciano spostato, righe, e si rompono quando suonano
    for (int k = 0; k < kGhosts; ++k)
    {
        const float lv = proc.value ("g" + juce::String (k + 1));
        if (lv < 0.002f)
        {   // spento: solo un'impronta tratteggiata dove starebbe
            auto f = figure (ghostBox (k), k == 2);
            juce::Path d; juce::PathStrokeType (1.0f).createDashedStroke (d, f, std::array<float, 2> { 3.0f, 6.0f }.data(), 2);
            g.setColour (INK.withAlpha (0.12f)); g.fillPath (d);
            continue;
        }
        const float a = juce::jlimit (0.0f, 1.0f, 0.18f + lv * 0.5f + glow[(size_t) k] * 0.6f);
        auto box = ghostBox (k);
        juce::Random rr (tick * 7 + k);
        const float jit = glow[(size_t) k] * 6.0f;
        auto f = figure (box.translated ((rr.nextFloat() - 0.5f) * jit, 0), k == 2);
        if (k == 3) { juce::ColourGradient aura (GC[3].withAlpha (0.25f * a), box.getCentreX(), box.getCentreY(), GC[3].withAlpha (0.0f), box.getCentreX(), box.getY() - 60, true); g.setGradientFill (aura); g.fillEllipse (box.expanded (80, 40)); }
        g.saveState(); g.reduceClipRegion (f);
        { juce::ColourGradient body (GC[k].withAlpha (0.35f * a), box.getCentreX(), box.getY(), GC[k].withAlpha (0.05f * a), box.getCentreX(), box.getBottom(), false); g.setGradientFill (body); g.fillRect (box.expanded (10)); }
        for (float y = box.getY(); y < box.getBottom(); y += 3) { g.setColour (juce::Colours::black.withAlpha (0.25f)); g.drawHorizontalLine ((int) y, box.getX() - 10, box.getRight() + 10); }
        g.restoreState();
        g.setColour (MAG.withAlpha (0.6f * a)); g.strokePath (f, juce::PathStrokeType (1.6f), juce::AffineTransform::translation (-2.5f - jit, 0));
        g.setColour (TEAL.withAlpha (0.6f * a)); g.strokePath (f, juce::PathStrokeType (1.6f), juce::AffineTransform::translation (2.5f + jit, 0));
        g.setColour (juce::Colours::white.withAlpha (0.5f * a)); g.strokePath (f, juce::PathStrokeType (1.0f));
        // occhi vuoti
        const float ey = box.getY() + box.getHeight() * 0.13f;
        g.setColour (GC[k].withAlpha (0.9f * a)); g.fillRect (box.getCentreX() - box.getWidth() * 0.12f, ey, box.getWidth() * 0.24f, juce::jmax (2.0f, box.getHeight() * 0.018f));
        // fette spostate quando suona forte
        if (glow[(size_t) k] > 0.25f)
            for (int s = 0; s < 3; ++s) { const float y = box.getY() + rr.nextFloat() * box.getHeight(); g.setColour (GC[k].withAlpha (0.35f)); g.fillRect (box.getX() + (rr.nextFloat() - 0.5f) * 30, y, box.getWidth(), 2.0f + rr.nextFloat() * 4); }
        text (g, ghostName (k), { box.getX() - 30, box.getBottom() + 2, box.getWidth() + 60, 16 }, vt (17.0f), GC[k].withAlpha (0.4f + 0.6f * a));
    }
    // tu: solido, al centro, occhiali ciano che si accendono con la voce
    {
        auto box = juce::Rectangle<float> (530, 120, 150, 268);
        auto f = figure (box);
        g.setColour (juce::Colours::black.withAlpha (0.5f)); g.fillEllipse (box.getX() - 10, box.getBottom() - 12, box.getWidth() + 20, 22);
        { juce::ColourGradient body (juce::Colour (0xff1a1026), box.getCentreX(), box.getY(), juce::Colour (0xff07040c), box.getCentreX(), box.getBottom(), false); g.setGradientFill (body); g.fillPath (f); }
        // controluce rosa sui bordi
        g.setColour (PINK.withAlpha (0.65f)); g.strokePath (f, juce::PathStrokeType (1.6f));
        g.saveState(); g.reduceClipRegion (f);
        { juce::ColourGradient rim (MAG.withAlpha (0.35f), box.getX(), 0, juce::Colours::transparentBlack, box.getX() + 40, 0, false); g.setGradientFill (rim); g.fillRect (box); }
        { juce::ColourGradient rim (TEAL.withAlpha (0.25f), box.getRight(), 0, juce::Colours::transparentBlack, box.getRight() - 40, 0, false); g.setGradientFill (rim); g.fillRect (box); }
        // faccia in ombra dentro il cappuccio
        g.setColour (juce::Colour (0xff030206)); g.fillEllipse (box.getX() + box.getWidth() * 0.38f, box.getY() + box.getHeight() * 0.06f, box.getWidth() * 0.24f, box.getHeight() * 0.17f);
        // zip e strisce luminose sui pantaloni
        g.setColour (TEAL.withAlpha (0.5f)); g.drawLine (box.getCentreX(), box.getY() + box.getHeight() * 0.3f, box.getCentreX(), box.getY() + box.getHeight() * 0.7f, 1.0f);
        g.setColour (TEAL); g.fillRect (box.getX() + box.getWidth() * 0.33f, box.getY() + box.getHeight() * 0.92f, box.getWidth() * 0.12f, 3.0f); g.fillRect (box.getX() + box.getWidth() * 0.56f, box.getY() + box.getHeight() * 0.92f, box.getWidth() * 0.12f, 3.0f);
        g.restoreState();
        const float e = juce::jlimit (0.0f, 1.0f, 0.35f + inLvl * 1.5f);
        auto vis = juce::Rectangle<float> (box.getX() + box.getWidth() * 0.39f, box.getY() + box.getHeight() * 0.1f, box.getWidth() * 0.22f, 6);
        g.setColour (TEAL.withAlpha (0.3f * e)); g.fillRoundedRectangle (vis.expanded (6, 4), 4); g.setColour (TEAL.interpolatedWith (juce::Colours::white, 0.4f * e)); g.fillRoundedRectangle (vis, 3);
        text (g, "TU", { box.getX(), box.getBottom() + 4, box.getWidth(), 16 }, vt (18.0f), PINK);
    }
    // schermi dei televisori: nome, segnale che scorre, neve se spento
    for (int k = 0; k < kGhosts; ++k)
    {
        auto tv = tvRect (k); auto scr = juce::Rectangle<float> (tv.getX() + 12, tv.getY() + 12, tv.getWidth() - 24, 112);
        const float lv = proc.value ("g" + juce::String (k + 1));
        juce::Path sp; sp.addRoundedRectangle (scr, 14);
        g.saveState(); g.reduceClipRegion (sp);
        { juce::ColourGradient cg (GC[k].withAlpha (lv > 0.002f ? 0.22f : 0.05f), scr.getCentreX(), scr.getCentreY(), juce::Colour (0xff050308), scr.getX(), scr.getY(), true); g.setGradientFill (cg); g.fillRect (scr); }
        if (lv < 0.002f)
        {
            juce::Random rr (tick + k * 31);
            for (int i = 0; i < 160; ++i) { g.setColour (juce::Colours::white.withAlpha (rr.nextFloat() * 0.18f)); g.fillRect (scr.getX() + rr.nextFloat() * scr.getWidth(), scr.getY() + rr.nextFloat() * scr.getHeight(), 2.0f, 2.0f); }
            text (g, "NESSUN SEGNALE", scr.withTrimmedTop (60), vt (20.0f), DIM);
        }
        else
        {
            auto& sc = scope[(size_t) k]; juce::Path w; const float mid = scr.getY() + 78;
            for (int i = 0; i < (int) sc.size(); ++i) { const float x = scr.getX() + scr.getWidth() * i / (float) (sc.size() - 1), y = mid - sc[(size_t) i] * 34 * (i % 2 ? 1.0f : -1.0f); if (i == 0) w.startNewSubPath (x, y); else w.lineTo (x, y); }
            g.setColour (GC[k].withAlpha (0.35f)); g.strokePath (w, juce::PathStrokeType (4.0f)); g.setColour (GC[k]); g.strokePath (w, juce::PathStrokeType (1.4f));
        }
        glowText (g, ghostName (k), { scr.getX(), scr.getY() + 8, scr.getWidth(), 30 }, vt (30.0f), lv > 0.002f ? GC[k] : DIM, lv > 0.002f ? 0.7f : 0.0f);
        static const char* sub[kGhosts] = { "fiato dalla tua voce", "un'altra te, piu' bassa", "al contrario, a tempo", "aura alta che si scioglie" };
        text (g, sub[k], { scr.getX(), scr.getY() + 36, scr.getWidth(), 16 }, vt (16.0f), INK.withAlpha (0.55f));
        for (float y = scr.getY(); y < scr.getBottom(); y += 3) { g.setColour (juce::Colours::black.withAlpha (0.28f)); g.drawHorizontalLine ((int) y, scr.getX(), scr.getRight()); }
        { juce::ColourGradient vg (juce::Colours::transparentBlack, scr.getCentreX(), scr.getCentreY(), juce::Colours::black.withAlpha (0.6f), scr.getX(), scr.getY(), true); g.setGradientFill (vg); g.fillRect (scr); }
        g.restoreState();
        g.setColour (juce::Colours::white.withAlpha (0.06f)); g.fillRoundedRectangle (scr.getX() + 8, scr.getY() + 6, scr.getWidth() * 0.5f, 10, 5);
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
    text (g, "<", R (Editor::H_PREV), raj (26.0f), hot == Editor::H_PREV ? juce::Colours::white : TEAL);
    text (g, ">", R (Editor::H_NEXT), raj (26.0f), hot == Editor::H_NEXT ? juce::Colours::white : TEAL);
    glowText (g, ed.proc.currentName().toUpperCase(), R (Editor::H_NAME), raj (24.0f, 0.14f), hot == Editor::H_NAME ? juce::Colours::white : PINK, 0.5f);
    text (g, "SALVA", R (Editor::H_SAVE), raj (17.0f, 0.14f), hot == Editor::H_SAVE ? juce::Colours::white : TEAL);
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
    for (auto& s : scope) s.assign (48, 0.0f);
    auto pct = [] (float v) { return v < 0.002f ? juce::String ("SPENTO") : juce::String (juce::roundToInt (v * 100)) + "%"; };
    auto db = [] (float v) { return v <= -39.9f ? juce::String ("MUTO") : juce::String (v, 1) + " dB"; };
    struct K { const char* id; const char* l; std::function<juce::String (float)> f; const char* tip; juce::Colour c; };
    const K ks[] = {
        { "g1", "LIVELLO", pct, "Quanto si sente il sussurro", GC[0] },
        { "c1", "ARIA", [] (float v) { return v < 0.35f ? juce::String ("CUPA") : v > 0.65f ? juce::String ("CHIARA") : juce::String ("MEDIA"); }, "Il fiato piu' cupo o piu' chiaro", GC[0] },
        { "g2", "LIVELLO", pct, "Quanto si sente l'ombra", GC[1] },
        { "c2", "SOTTO", [] (float v) { return "-" + juce::String (juce::roundToInt (12 - v * 7)) + " st"; }, "Da un'ottava sotto (-12) a una quarta sotto (-5)", GC[1] },
        { "g3", "LIVELLO", pct, "Quanto si sente il riflesso", GC[2] },
        { "c3", "PEZZI", [] (float v) { static const char* n[] = { "1/16", "1/8", "1/4", "1/4.", "1/2" }; return juce::String (n[juce::jlimit (0, 4, juce::roundToInt (v * 4))]); }, "Lunghezza dei pezzi al contrario, a tempo", GC[2] },
        { "g4", "LIVELLO", pct, "Quanto si sente l'alone", GC[3] },
        { "c4", "SOPRA", [] (float v) { return "+" + juce::String (juce::roundToInt (7 + v * 5)) + " st"; }, "Da una quinta (+7) a un'ottava (+12) sopra", GC[3] },
        { "delay", "RITARDO", [] (float v) { return juce::String (juce::roundToInt (v)) + " ms"; }, "Quanto i fantasmi ti seguono in ritardo", TEAL },
        { "drift", "DERIVA", pct, "Quanto si muovono intorno a te e ondeggiano", TEAL },
        { "dark", "BUIO", pct, "Quanta luce perdono (meno acuti)", TEAL },
        { "space", "SPAZIO", pct, "La stanza dove stanno i fantasmi", TEAL },
        { "width", "LARGHEZZA", pct, "Quanto si aprono ai lati", MAG },
        { "ghost", "FANTASMI", [] (float v) { return juce::String (v, 1) + " dB"; }, "Volume di tutti i fantasmi", MAG },
        { "dry", "TU", db, "Volume della tua voce (al minimo restano solo i fantasmi)", MAG },
    };
    for (auto& k : ks)
    {
        auto kn = std::make_unique<Knob> (*this, k.id, k.l, k.f); kn->setTooltip (k.tip); kn->col = k.c;
        root.addAndMakeVisible (*kn); knobs.push_back (std::move (kn));
    }
    for (int gI = 0; gI < kGhosts; ++gI)
    {
        auto tv = tvRect (gI);
        knobs[(size_t) (gI * 2)]->setBounds ((int) tv.getX() + 4, (int) tv.getY() + 136, 88, 120);
        knobs[(size_t) (gI * 2 + 1)]->setBounds ((int) tv.getX() + 92, (int) tv.getY() + 136, 88, 120);
    }
    for (int j = 0; j < 7; ++j) knobs[(size_t) (8 + j)]->setBounds (j < 4 ? 798 + j * 90 : 843 + (j - 4) * 90, j < 4 ? (int) BOTTOM + 44 : (int) BOTTOM + 170, 86, 118);
    root.addAndMakeVisible (sizeChip);
    auto userFiles = [] { auto f = Processor::presetFolder().findChildFiles (juce::File::findFiles, false, "*.phantom"); f.sort(); return f; };
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
    browser.title = "PHANTOM // NASTRI"; browser.sub = "FANTASMI REGISTRATI";
    browser.titleFont = krona (24.0f, 0.1f); browser.nameFont = vt (32.0f); browser.monoFont = vt (16.0f);
    browser.accent = MAG; browser.accent2 = TEAL; browser.ink = INK; browser.dim = DIM;
    browser.bg0 = juce::Colour (0xf5080410); browser.bg1 = juce::Colour (0xf5200a2e);
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
    ++tick;
    inLvl = juce::jmax (proc.uiIn.load(), inLvl * 0.85f);
    for (int k = 0; k < kGhosts; ++k)
    {
        const float m = proc.eng.lvlOf (k);
        glow[(size_t) k] = juce::jmax (juce::jlimit (0.0f, 1.0f, std::sqrt (m) * 1.3f), glow[(size_t) k] * 0.82f);
        auto& sc = scope[(size_t) k]; sc.erase (sc.begin()); sc.push_back (glow[(size_t) k] * (0.6f + 0.4f * juce::Random::getSystemRandom().nextFloat()));
    }
    if (! browser.isVisible())
    {
        root.repaint (150, 60, 800, (int) BOTTOM - 60);
        root.repaint (0, (int) BOTTOM + 18, 780, 132);
    }
    if (proc.preset != lastPreset || proc.userName != lastUser) { lastPreset = proc.preset; lastUser = proc.userName; root.repaint (0, 0, W, 56); }
    if (tick % 8 == 0) root.repaint (hotR[H_UNDO].getUnion (hotR[H_REDO]).toNearestInt());
    for (auto& k : knobs) { const float v = k->prm->getValue(); if (v != k->last) { k->last = v; k->repaint(); } }
}
}
