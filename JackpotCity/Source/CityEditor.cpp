#include "CityEditor.h"

namespace jcity
{
using namespace ui;
juce::Colour ui::sceneColour (int k)
{
    static const juce::Colour c[kScenes] = { AMBER, juce::Colour (0xffff3b3b), ACID, CYAN, MAG, juce::Colour (0xff8f7bff) };
    return c[juce::jlimit (0, kScenes - 1, k)];
}
namespace
{
juce::Path chamfer (juce::Rectangle<float> r, float c)
{
    juce::Path p;
    p.startNewSubPath (r.getX() + c, r.getY()); p.lineTo (r.getRight(), r.getY()); p.lineTo (r.getRight(), r.getBottom() - c);
    p.lineTo (r.getRight() - c, r.getBottom()); p.lineTo (r.getX(), r.getBottom()); p.lineTo (r.getX(), r.getY() + c); p.closeSubPath();
    return p;
}
void text (juce::Graphics& g, const juce::String& t, juce::Rectangle<float> r, juce::Font f, juce::Colour c, juce::Justification j = juce::Justification::centred)
{ g.setFont (f); g.setColour (c); g.drawFittedText (t, r.toNearestInt(), j, 1, 0.6f); }
void glowText (juce::Graphics& g, const juce::String& t, juce::Rectangle<float> r, juce::Font f, juce::Colour c, float on, juce::Justification j = juce::Justification::centred)
{
    g.setFont (f);
    if (on > 0.01f)
        for (int i = 4; i >= 1; --i)
        {
            g.setColour (c.withAlpha (0.07f * on));
            for (int a = 0; a < 8; ++a) { const float an = a * 0.785f; g.drawFittedText (t, r.translated (std::cos (an) * i * 1.3f, std::sin (an) * i * 1.3f).toNearestInt(), j, 1, 0.6f); }
        }
    g.setColour (on > 0.01f ? c.interpolatedWith (juce::Colours::white, 0.35f * on) : c); g.drawFittedText (t, r.toNearestInt(), j, 1, 0.6f);
}
void bracket (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c, float L)
{
    g.setColour (c);
    g.drawLine (r.getX(), r.getY(), r.getX() + L, r.getY(), 2); g.drawLine (r.getX(), r.getY(), r.getX(), r.getY() + L, 2);
    g.drawLine (r.getRight(), r.getBottom(), r.getRight() - L, r.getBottom(), 2); g.drawLine (r.getRight(), r.getBottom(), r.getRight(), r.getBottom() - L, 2);
}
void tube (juce::Graphics& g, const juce::Path& p, juce::Colour c, float on, float w)
{
    if (on > 0.01f) for (int i = 4; i >= 1; --i) { g.setColour (c.withAlpha (0.08f * on)); g.strokePath (p, juce::PathStrokeType (w + i * 4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded)); }
    g.setColour (on > 0.01f ? c.interpolatedWith (juce::Colours::white, 0.45f * on) : c); g.strokePath (p, juce::PathStrokeType (w, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}
constexpr float HORIZON = 292, CONSOLE = 352;
}

// =============================================================================================== manopole
Knob::Knob (Editor& e, const juce::String& id, juce::String l, juce::Colour c, std::function<juce::String (float)> fm)
    : ed (e), prm (e.P (id)), label (std::move (l)), col (c), fmt (std::move (fm))
{ setRepaintsOnMouseActivity (true); setMouseCursor (juce::MouseCursor::UpDownResizeCursor); }
void Knob::paint (juce::Graphics& g)
{
    const float w = (float) getWidth(), h = (float) getHeight(), v = prm->getValue(), a0 = -2.4f, a1 = 2.4f, av = a0 + (a1 - a0) * v;
    const bool hov = isMouseOverOrDragging();
    const juce::String t = fmt ? fmt (prm->convertFrom0to1 (v)) : prm->getCurrentValueAsText();
    if (big)
    {
        const float cx = w * 0.5f, cy = h * 0.53f, R = juce::jmin (w, h) * 0.36f;
        glowText (g, label, { 0, 0, w, 26 }, raj (22.0f, 0.18f), v > 0.001f ? MAG : DIM, v > 0.001f ? 0.6f : 0.0f);
        // anello di 48 segmenti: dal ciano (fuori citta') al magenta (dentro fino al collo)
        for (int i = 0; i < 48; ++i)
        {
            const float f = i / 47.0f, a = a0 + (a1 - a0) * f; const bool lit = f <= v + 1e-4f && v > 0.001f;
            const auto c = CYAN.interpolatedWith (MAG, f);
            juce::Path seg; seg.addCentredArc (cx, cy, R + 20, R + 20, 0, a - 0.045f, a + 0.045f, true);
            if (lit) { g.setColour (c.withAlpha (0.25f)); g.strokePath (seg, juce::PathStrokeType (16.0f)); }
            g.setColour (lit ? c : juce::Colour (0xff24202e)); g.strokePath (seg, juce::PathStrokeType (8.0f));
        }
        g.setColour (juce::Colours::black.withAlpha (0.6f)); g.fillEllipse (cx - R + 4, cy - R + 8, R * 2, R * 2);
        { juce::ColourGradient body (juce::Colour (0xff3b3846), cx, cy - R, juce::Colour (0xff0b0a10), cx, cy + R, false); g.setGradientFill (body); g.fillEllipse (cx - R, cy - R, R * 2, R * 2); }
        for (int i = 0; i < 60; ++i) { const float a = av + i * juce::MathConstants<float>::twoPi / 60; g.setColour (juce::Colours::black.withAlpha (0.4f)); g.drawLine (cx + std::sin (a) * (R - 1), cy - std::cos (a) * (R - 1), cx + std::sin (a) * (R - 6), cy - std::cos (a) * (R - 6), 1.2f); }
        const float rc = R * 0.74f;
        { juce::ColourGradient cap (juce::Colour (0xff221f2b), cx - rc * 0.5f, cy - rc, juce::Colour (0xff07060b), cx + rc * 0.5f, cy + rc, false); g.setGradientFill (cap); g.fillEllipse (cx - rc, cy - rc, rc * 2, rc * 2); }
        for (float r = rc * 0.3f; r < rc; r += 5) { g.setColour (juce::Colours::white.withAlpha (0.025f)); g.drawEllipse (cx - r, cy - r, r * 2, r * 2, 1.0f); }
        const auto pc = CYAN.interpolatedWith (MAG, v);
        { const juce::Point<float> p0 (cx + std::sin (av) * rc * 0.78f, cy - std::cos (av) * rc * 0.78f), p1 (cx + std::sin (av) * (R - 4), cy - std::cos (av) * (R - 4));
          g.setColour (pc.withAlpha (0.35f)); g.drawLine ({ p0, p1 }, 8.0f); g.setColour (juce::Colours::white.interpolatedWith (pc, 0.3f)); g.drawLine ({ p0, p1 }, 3.0f); }
        const juce::String vt = v < 0.001f ? juce::String ("FUORI") : juce::String (juce::roundToInt (v * 100)) + "%";
        glowText (g, vt, { cx - rc, cy - 24, rc * 2, 44 }, raj (40.0f, 0.04f), v < 0.001f ? DIM : pc, v < 0.001f ? 0.0f : 0.7f);
        text (g, v < 0.001f ? "passa identico" : (v < 0.5f ? "ai margini" : (v < 0.9f ? "per strada" : "fino al collo")), { cx - rc, cy + 18, rc * 2, 16 }, mono (12.0f), hov ? INK : DIM);
        return;
    }
    const float lh = 20, vh = 18, S = juce::jmin (w - 30, h - lh - vh - 16), cx = w * 0.5f, cy = lh + 8 + S * 0.5f, R = S * 0.5f;
    text (g, label, { -10, 0, w + 20, lh }, raj (16.0f, 0.12f), hov ? INK : col.interpolatedWith (INK, 0.3f));
    for (int i = 0; i <= 24; ++i)
    {
        const float f = i / 24.0f, a = a0 + (a1 - a0) * f;
        const bool lit = bipolar ? ((v >= 0.5f && f >= 0.5f - 1e-4f && f <= v + 1e-4f) || (v < 0.5f && f <= 0.5f + 1e-4f && f >= v - 1e-4f)) : f <= v + 1e-4f;
        const float r1 = R + 5, r2 = R + (i % 6 == 0 ? 11.0f : 8.0f);
        if (lit) { g.setColour (col.withAlpha (0.25f)); g.drawLine (cx + std::sin (a) * r1, cy - std::cos (a) * r1, cx + std::sin (a) * r2, cy - std::cos (a) * r2, 5.0f); }
        g.setColour (lit ? col : juce::Colour (0xff2a2634)); g.drawLine (cx + std::sin (a) * r1, cy - std::cos (a) * r1, cx + std::sin (a) * r2, cy - std::cos (a) * r2, i % 6 == 0 ? 2.2f : 1.5f);
    }
    g.setColour (juce::Colours::black.withAlpha (0.6f)); g.fillEllipse (cx - R + 2, cy - R + 5, R * 2, R * 2);
    { juce::ColourGradient body (juce::Colour (0xff3a3744), cx, cy - R, juce::Colour (0xff0c0b11), cx, cy + R, false); g.setGradientFill (body); g.fillEllipse (cx - R, cy - R, R * 2, R * 2); }
    for (int i = 0; i < 32; ++i) { const float a = av + i * juce::MathConstants<float>::twoPi / 32; g.setColour (juce::Colours::black.withAlpha (0.4f)); g.drawLine (cx + std::sin (a) * (R - 0.5f), cy - std::cos (a) * (R - 0.5f), cx + std::sin (a) * (R - 4), cy - std::cos (a) * (R - 4), 1.0f); }
    const float rc = R * 0.66f;
    { juce::ColourGradient cap (juce::Colour (0xff272430), cx - rc * 0.4f, cy - rc, juce::Colour (0xff09080d), cx + rc * 0.4f, cy + rc, false); g.setGradientFill (cap); g.fillEllipse (cx - rc, cy - rc, rc * 2, rc * 2); }
    { const juce::Point<float> p0 (cx + std::sin (av) * rc * 0.25f, cy - std::cos (av) * rc * 0.25f), p1 (cx + std::sin (av) * (R - 3), cy - std::cos (av) * (R - 3));
      g.setColour (col.withAlpha (0.35f)); g.drawLine ({ p0, p1 }, 6.0f); g.setColour (juce::Colours::white.interpolatedWith (col, 0.4f)); g.drawLine ({ p0, p1 }, 2.2f); }
    text (g, t, { -10, h - vh, w + 20, vh }, mono (14.0f), hov ? juce::Colours::white : col.interpolatedWith (INK, 0.4f));
}
void Knob::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isAltDown() || e.mods.isCommandDown()) { prm->setValueNotifyingHost (prm->getDefaultValue()); return; }
    start = prm->getValue(); drag = true; prm->beginChangeGesture();
}
void Knob::mouseDrag (const juce::MouseEvent& e)
{
    if (! drag) return;
    prm->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, start - e.getDistanceFromDragStartY() / (e.mods.isShiftDown() ? 900.0f : (big ? 300.0f : 220.0f)))); repaint();
}
void Knob::mouseUp (const juce::MouseEvent&) { if (drag) prm->endChangeGesture(); drag = false; }
void Knob::mouseDoubleClick (const juce::MouseEvent&) { prm->setValueNotifyingHost (prm->getDefaultValue()); }
void Knob::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& wd)
{
    const float dy = wd.isReversed ? -wd.deltaY : wd.deltaY;
    prm->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, prm->getValue() + juce::jlimit (-0.05f, 0.05f, dy * 0.08f)));
}

// =============================================================================================== pioggia
void Rain::step()
{
    juce::Random& r = juce::Random::getSystemRandom();
    const int want = (int) (30 + amount * 220);
    while ((int) drops.size() < want) drops.push_back ({ r.nextFloat() * getWidth(), r.nextFloat() * getHeight(), 8 + r.nextFloat() * 16, 14 + r.nextFloat() * 12, 0.08f + r.nextFloat() * 0.22f });
    if ((int) drops.size() > want) drops.resize ((size_t) want);
    for (auto& d : drops) { d.y += d.sp; d.x -= d.sp * 0.18f; if (d.y > getHeight()) { d.y = -d.len; d.x = r.nextFloat() * (getWidth() + 60); } }
}
void Rain::paint (juce::Graphics& g)
{
    for (auto& d : drops) { g.setColour (juce::Colour (0xffbfe6ff).withAlpha (d.a)); g.drawLine (d.x, d.y, d.x - d.len * 0.18f, d.y + d.len, 1.0f); }
}

// =============================================================================================== sfondo
juce::Rectangle<float> Editor::signRect (int k)
{
    static const float ys[kScenes] = { 98, 124, 92, 118, 96, 122 };
    return { 20.0f + k * 192.0f, ys[k], 178, 96 };
}
void Editor::drawStatic (juce::Graphics& g)
{
    // cielo
    { juce::ColourGradient sky (SKY0, 0, 56, SKY1, 0, HORIZON, false); sky.addColour (0.7, juce::Colour (0xff140a24)); g.setGradientFill (sky); g.fillRect (0.0f, 0.0f, (float) W, HORIZON); }
    { juce::ColourGradient haze (MAG.withAlpha (0.22f), W * 0.5f, HORIZON, MAG.withAlpha (0.0f), W * 0.5f, HORIZON - 170, false); g.setGradientFill (haze); g.fillRect (0.0f, HORIZON - 170, (float) W, 170.0f); }
    juce::Random r (2077);
    // i palazzi lontani, poi quelli vicini
    for (int layer = 0; layer < 3; ++layer)
    {
        const juce::Colour body = layer == 0 ? juce::Colour (0xff1a1029) : layer == 1 ? juce::Colour (0xff100a1a) : juce::Colour (0xff08060d);
        float x = -20;
        while (x < W + 20)
        {
            const float bw = 28 + r.nextFloat() * (layer == 2 ? 90.0f : 60.0f);
            const float top = layer == 0 ? 120 + r.nextFloat() * 110 : layer == 1 ? 80 + r.nextFloat() * 150 : 60 + r.nextFloat() * 190;
            juce::Rectangle<float> b (x, top, bw, HORIZON - top);
            g.setColour (body); g.fillRect (b);
            if (r.nextFloat() < 0.35f) { g.fillRect (b.getCentreX() - 1, top - 18 - r.nextFloat() * 20, 2.0f, 40.0f); if (layer > 0) { g.setColour (juce::Colour (0xffff3b3b).withAlpha (0.8f)); g.fillEllipse (b.getCentreX() - 2, top - 40, 4, 4); } }
            // finestre
            const float wa = layer == 0 ? 0.25f : layer == 1 ? 0.4f : 0.55f;
            for (float wy = top + 8; wy < HORIZON - 6; wy += 7)
                for (float wx = x + 4; wx < x + bw - 4; wx += 6)
                    if (r.nextFloat() < 0.16f)
                    {
                        const float pick = r.nextFloat(); const auto c = pick < 0.55f ? juce::Colour (0xffffd9a0) : pick < 0.8f ? CYAN : MAG;
                        g.setColour (c.withAlpha (wa * (0.4f + 0.6f * r.nextFloat()))); g.fillRect (wx, wy, 2.0f, 3.0f);
                    }
            // strisce verticali al neon sui palazzi vicini
            if (layer == 2 && r.nextFloat() < 0.3f)
            {
                const auto c = r.nextBool() ? CYAN : MAG; const float sx = x + 3 + r.nextFloat() * (bw - 6);
                g.setColour (c.withAlpha (0.12f)); g.fillRect (sx - 3, top + 10, 7.0f, HORIZON - top - 20); g.setColour (c.withAlpha (0.7f)); g.fillRect (sx, top + 10, 1.5f, HORIZON - top - 20);
            }
            x += bw + (layer == 2 ? r.nextFloat() * 14 : 0);
        }
        if (layer == 1) { juce::ColourGradient fog (juce::Colour (0x0018101f), 0, 150, juce::Colour (0x8818101f), 0, HORIZON, false); g.setGradientFill (fog); g.fillRect (0.0f, 150.0f, (float) W, HORIZON - 150); }
    }
    // la strada bagnata
    { juce::ColourGradient st (juce::Colour (0xff0d0a14), 0, HORIZON, juce::Colour (0xff040308), 0, (float) H, false); g.setGradientFill (st); g.fillRect (0.0f, HORIZON, (float) W, H - HORIZON); }
    g.setColour (MAG.withAlpha (0.35f)); g.drawHorizontalLine ((int) HORIZON, 0, (float) W);
    for (float y = HORIZON + 4; y < CONSOLE; y += 4) { g.setColour (juce::Colours::black.withAlpha (0.25f)); g.drawHorizontalLine ((int) y, 0, (float) W); }
    // barra in alto
    { juce::ColourGradient bar (juce::Colour (0xf2050409), 0, 0, juce::Colour (0xe00a0712), 0, 56, false); g.setGradientFill (bar); g.fillRect (0, 0, W, 56); }
    g.setColour (MAG.withAlpha (0.6f)); g.drawHorizontalLine (56, 0, (float) W);
    g.setColour (CYAN.withAlpha (0.25f)); g.drawHorizontalLine (58, 0, (float) W);
    glowText (g, "EDGERUNNER", { 20, 6, 300, 40 }, neon (34.0f), MAG, 0.8f, juce::Justification::centredLeft);
    text (g, "JACKPOT CYBERNET  //  CITY", { 22, 40, 300, 14 }, mono (11.0f), CYAN.withAlpha (0.8f), juce::Justification::centredLeft);
    { auto pb = hotR[H_NAME].expanded (44, 4); auto p = chamfer (pb, 10); g.setColour (juce::Colour (0xff07060c)); g.fillPath (p); g.setColour (CYAN.withAlpha (0.5f)); g.strokePath (p, juce::PathStrokeType (1.2f)); bracket (g, pb.reduced (3), MAG.withAlpha (0.8f), 9); }
    // il chiosco di vetro
    auto con = juce::Rectangle<float> (26, CONSOLE + 6, W - 52, H - CONSOLE - 20);
    auto cp = chamfer (con, 20);
    g.setColour (juce::Colours::black.withAlpha (0.5f)); g.fillPath (cp, juce::AffineTransform::translation (6, 8));
    { juce::ColourGradient gl (juce::Colour (0xe8100d1a), 0, con.getY(), juce::Colour (0xf206050b), 0, con.getBottom(), false); g.setGradientFill (gl); g.fillPath (cp); }
    g.saveState(); g.reduceClipRegion (cp);
    g.setColour (CYAN.withAlpha (0.035f)); for (float x = con.getX(); x < con.getRight(); x += 24) g.drawVerticalLine ((int) x, con.getY(), con.getBottom());
    for (float y = con.getY(); y < con.getBottom(); y += 24) g.drawHorizontalLine ((int) y, con.getX(), con.getRight());
    g.setColour (juce::Colours::black.withAlpha (0.16f)); for (float y = con.getY(); y < con.getBottom(); y += 3) g.drawHorizontalLine ((int) y, con.getX(), con.getRight());
    // graffi e sporco sul vetro
    for (int i = 0; i < 70; ++i) { g.setColour (juce::Colours::white.withAlpha (0.02f + r.nextFloat() * 0.04f)); const float x0 = con.getX() + r.nextFloat() * con.getWidth(), y0 = con.getY() + r.nextFloat() * con.getHeight(); g.drawLine (x0, y0, x0 + r.nextFloat() * 40 - 20, y0 + r.nextFloat() * 10 - 5, 0.7f); }
    g.restoreState();
    g.setColour (MAG.withAlpha (0.8f)); g.strokePath (cp, juce::PathStrokeType (1.5f));
    bracket (g, con.reduced (8), CYAN.withAlpha (0.85f), 16);
    // striscia HUD
    auto hud = juce::Rectangle<float> (con.getX() + 20, con.getY() + 14, con.getWidth() - 40, 40);
    g.setColour (juce::Colour (0xff07060c)); g.fillRect (hud); g.setColour (CYAN.withAlpha (0.3f)); g.drawRect (hud, 1.0f);
    // titoli dei gruppi
    auto group = [&] (juce::Rectangle<float> rr, const juce::String& t, juce::Colour c)
    {
        text (g, t, rr.withHeight (16), mono (12.0f), c.withAlpha (0.9f), juce::Justification::centredLeft);
        g.setColour (c.withAlpha (0.4f)); g.drawHorizontalLine ((int) rr.getY() + 18, rr.getX(), rr.getRight());
    };
    group ({ 60, 432, 352, 20 }, "// LA SCENA", CYAN);
    group ({ 768, 432, 352, 20 }, "// L'USCITA", ACID);
    text (g, "clicca un'insegna per cambiare scena  -  doppio clic su una manopola: torna a posto", { 60, 668, 600, 14 }, mono (11.0f), DIM, juce::Justification::centredLeft);
    text (g, "jackpot.wav // night city", { 900, 668, 220, 14 }, mono (11.0f), DIM, juce::Justification::centredRight);
}
void Editor::drawCity (juce::Graphics& g)
{
    // insegne e riflessi (strato trasparente sopra lo sfondo)
    const int sc = scene();
    for (int k = 0; k < kScenes; ++k)
    {
        const auto s = signRect (k); const auto col = sceneColour (k);
        const bool cur = k == sc; const float on = cur ? (flick > 0 ? 0.25f : 1.0f) : 0.0f;
        // cavi e staffe
        g.setColour (juce::Colour (0xff2b2735)); g.drawLine (s.getX() + 22, s.getY() - 40, s.getX() + 22, s.getY(), 1.5f); g.drawLine (s.getRight() - 22, s.getY() - 40, s.getRight() - 22, s.getY(), 1.5f);
        auto plate = chamfer (s, 12);
        g.setColour (juce::Colours::black.withAlpha (0.55f)); g.fillPath (plate, juce::AffineTransform::translation (4, 6));
        { juce::ColourGradient pg (juce::Colour (0xff120f1a).interpolatedWith (col, 0.12f * on), s.getX(), s.getY(), juce::Colour (0xff06050a), s.getX(), s.getBottom(), false); g.setGradientFill (pg); g.fillPath (plate); }
        if (on > 0.01f) { juce::ColourGradient halo (col.withAlpha (0.22f * on), s.getCentreX(), s.getCentreY(), col.withAlpha (0.0f), s.getCentreX(), s.getY() - 40, true); g.setGradientFill (halo); g.fillEllipse (s.expanded (40, 30)); }
        juce::Path tb; tb.addRoundedRectangle (s.reduced (7), 9.0f);
        tube (g, tb, on > 0.01f ? col : col.withMultipliedSaturation (0.35f).withMultipliedBrightness (0.42f), on, 2.2f);
        const auto& S = scenes()[(size_t) k];
        glowText (g, S.sign, s.reduced (14, 0).withTrimmedTop (16).withHeight (44), neon (juce::String (S.sign).length() > 7 ? 22.0f : 28.0f), on > 0.01f ? col : col.withMultipliedSaturation (0.4f).withMultipliedBrightness (0.58f), on);
        text (g, S.name, s.reduced (12, 0).withTrimmedTop (64).withHeight (16), mono (10.5f), on > 0.01f ? INK.withAlpha (0.85f) : DIM.withAlpha (0.7f));
        g.setColour (juce::Colours::white.withAlpha (0.08f)); g.strokePath (plate, juce::PathStrokeType (1.0f));
        // riflesso sulla strada bagnata
        if (on > 0.01f)
        {
            juce::ColourGradient rf (col.withAlpha (0.35f * on), s.getCentreX(), HORIZON + 2, col.withAlpha (0.0f), s.getCentreX(), CONSOLE + 4, false);
            g.setGradientFill (rf); g.fillRect (s.getX() + 18, HORIZON + 2, s.getWidth() - 36, CONSOLE - HORIZON + 2);
            for (float y = HORIZON + 5; y < CONSOLE; y += 4) { g.setColour (juce::Colours::black.withAlpha (0.35f)); g.drawHorizontalLine ((int) y, s.getX() + 18, s.getRight() - 18); }
        }
        else { g.setColour (col.withAlpha (0.05f)); g.fillRect (s.getX() + 30, HORIZON + 2, s.getWidth() - 60, 30.0f); }
    }
}
void Editor::drawHud (juce::Graphics& g)
{
    const int sc = scene(); const auto col = sceneColour (sc); const auto& S = scenes()[(size_t) sc];
    const float c = proc.value ("city");
    auto hud = juce::Rectangle<float> (46, CONSOLE + 20, W - 92, 40);
    text (g, "SCENA //", { hud.getX() + 12, hud.getY(), 80, hud.getHeight() }, mono (12.0f), DIM, juce::Justification::centredLeft);
    glowText (g, S.name, { hud.getX() + 90, hud.getY(), 330, hud.getHeight() }, raj (24.0f, 0.1f), col, c > 0.001f ? 0.6f : 0.0f, juce::Justification::centredLeft);
    text (g, S.line, { hud.getX() + 420, hud.getY(), 420, hud.getHeight() }, mono (12.0f), INK.withAlpha (0.65f), juce::Justification::centredLeft);
    // segnale: barre che salgono col suono, e lampeggiano se il segnale cade
    const bool lost = gate < 0.6f && c > 0.001f;
    text (g, lost ? "SEGNALE PERSO" : (c > 0.001f ? "SEGNALE" : "OFFLINE"), { hud.getRight() - 290, hud.getY(), 120, hud.getHeight() }, mono (12.0f), lost ? juce::Colour (0xffff3b3b) : (c > 0.001f ? CYAN : DIM), juce::Justification::centredRight);
    const float lv = juce::jlimit (0.0f, 1.0f, std::sqrt (outLvl) * 1.1f) * (c > 0.001f ? 1.0f : 0.0f);
    for (int i = 0; i < 14; ++i)
    {
        const float f = (i + 1) / 14.0f; const bool lit = f <= lv + 0.02f;
        auto b = juce::Rectangle<float> (hud.getRight() - 160 + i * 11.0f, hud.getBottom() - 8 - (6 + i * 1.6f), 7, 6 + i * 1.6f);
        g.setColour (lit ? (lost ? juce::Colour (0xffff3b3b) : CYAN.interpolatedWith (MAG, f)) : juce::Colour (0xff1f1b29)); g.fillRect (b);
    }
}

// =============================================================================================== radice
void Root::paint (juce::Graphics& g)
{
    const float s = g.getInternalContext().getPhysicalPixelScaleFactor();
    auto cacheImg = [s] (juce::Image& img, float& sc, bool transparent, std::function<void (juce::Graphics&)> fn)
    {
        if (img.isNull() || std::abs (sc - s) > 0.01f)
        {
            sc = s; img = juce::Image (transparent ? juce::Image::ARGB : juce::Image::RGB, juce::jmax (1, juce::roundToInt (Editor::W * s)), juce::jmax (1, juce::roundToInt (Editor::H * s)), true);
            juce::Graphics ig (img); ig.addTransform (juce::AffineTransform::scale (s)); fn (ig);
        }
    };
    cacheImg (bg, bgScale, false, [this] (juce::Graphics& ig) { ed.drawStatic (ig); });
    const int key = ed.scene() * 2 + (ed.flick > 0 ? 1 : 0);
    if (key != cityKey) { cityKey = key; city = {}; }
    cacheImg (city, cityScale, true, [this] (juce::Graphics& ig) { ed.drawCity (ig); });
    g.drawImage (bg, getLocalBounds().toFloat()); g.drawImage (city, getLocalBounds().toFloat());
    // insegna sotto il mouse
    if (hot >= Editor::H_SCENE0) { const auto s2 = Editor::signRect (hot - Editor::H_SCENE0); g.setColour (juce::Colours::white.withAlpha (0.06f)); g.fillPath (chamfer (s2, 12)); }
    // barra preset
    auto R = [this] (int h) { return ed.hotR[h]; };
    glowText (g, "<", R (Editor::H_PREV), raj (26.0f), hot == Editor::H_PREV ? juce::Colours::white : CYAN, 0.5f);
    glowText (g, ">", R (Editor::H_NEXT), raj (26.0f), hot == Editor::H_NEXT ? juce::Colours::white : CYAN, 0.5f);
    glowText (g, ed.proc.currentName().toUpperCase(), R (Editor::H_NAME), raj (24.0f, 0.12f), hot == Editor::H_NAME ? juce::Colours::white : MAG, 0.5f);
    text (g, "SALVA", R (Editor::H_SAVE), raj (17.0f, 0.12f), hot == Editor::H_SAVE ? juce::Colours::white : ACID);
    text (g, "ANNULLA", R (Editor::H_UNDO), raj (16.0f, 0.1f), ed.proc.undo.canUndo() ? (hot == Editor::H_UNDO ? juce::Colours::white : INK) : DIM.withAlpha (0.6f));
    text (g, "RIPETI", R (Editor::H_REDO), raj (16.0f, 0.1f), ed.proc.undo.canRedo() ? (hot == Editor::H_REDO ? juce::Colours::white : INK) : DIM.withAlpha (0.6f));
    ed.drawHud (g);
}
void Root::mouseDown (const juce::MouseEvent& e)
{
    for (int h = 0; h < Editor::H_COUNT; ++h)
        if (ed.hotR[h].contains (e.position)) { ed.click (h); return; }
}
void Root::mouseMove (const juce::MouseEvent& e)
{
    int h = -1; for (int i = 0; i < Editor::H_COUNT; ++i) if (ed.hotR[i].contains (e.position)) { h = i; break; }
    if (h != hot) { hot = h; repaint(); setMouseCursor (h >= 0 ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor); }
}

// =============================================================================================== editor
Editor::Editor (Processor& p) : juce::AudioProcessorEditor (p), proc (p)
{
    setWantsKeyboardFocus (true);
    addAndMakeVisible (root);
    auto pct = [] (float v) { return juce::String (juce::roundToInt (v * 100)) + "%"; };
    struct K { const char* id; const char* l; juce::Colour c; std::function<juce::String (float)> f; const char* tip; bool bip; };
    const K ks[] = {
        { "city", "DENTRO LA CITTA'", MAG, pct, "Quanto sei dentro la scena. A zero la voce passa identica.", false },
        { "distance", "DISTANZA", CYAN, pct, "Quanto e' lontana la voce: meno acuti, piu' stanza e piu' eco", false },
        { "disturb", "DISTURBO", CYAN, pct, "Quanto si rompe il segnale: saturazione, bit, fruscio, cadute (dipende dalla scena)", false },
        { "ambience", "AMBIENTE", CYAN, pct, "Quanto si sente la citta' intorno: pioggia, treni, palazzi, stanza", false },
        { "width", "LARGHEZZA", ACID, pct, "Larghezza stereo (le scene radio e telefono partono mono)", false },
        { "tone", "TONO", ACID, [] (float v) { return std::abs (v) < 0.03f ? juce::String ("NEUTRO") : (v < 0 ? "SCURO " : "CHIARO ") + juce::String (juce::roundToInt (std::abs (v) * 100)); }, "Sposta la banda della scena verso lo scuro o il chiaro", true },
        { "out", "USCITA", ACID, [] (float v) { return juce::String (v, 1) + " dB"; }, "Volume dell'effetto (solo la parte dentro la citta')", false },
    };
    for (auto& k : ks)
    {
        auto kn = std::make_unique<Knob> (*this, k.id, k.l, k.c, k.f); kn->setTooltip (k.tip); kn->bipolar = k.bip;
        root.addAndMakeVisible (*kn); knobs.push_back (std::move (kn));
    }
    knobs[0]->big = true;
    rain.setBounds (0, 60, W, (int) CONSOLE - 60);
    root.addAndMakeVisible (rain);
    root.addAndMakeVisible (sizeChip);
    // scheda dei preset
    auto userFiles = [] { auto f = Processor::presetFolder().findChildFiles (juce::File::findFiles, false, "*.city"); f.sort(); return f; };
    browser.entries = [userFiles]
    {
        std::vector<jcyber::Entry> v;
        for (int i = 0; i < (int) presets().size(); ++i) v.push_back ({ presets()[(size_t) i].cat, presets()[(size_t) i].name, presets()[(size_t) i].note, i, false });
        auto files = userFiles();
        for (int i = 0; i < files.size(); ++i) v.push_back ({ "I MIEI", files[i].getFileNameWithoutExtension(), "preset salvato da te", 1000 + i, true });
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
    browser.title = "CITY // ARCHIVIO"; browser.sub = "LUOGHI SALVATI";
    browser.titleFont = neon (30.0f); browser.nameFont = raj (26.0f, 0.08f); browser.monoFont = mono (12.0f);
    browser.accent = MAG; browser.accent2 = CYAN; browser.ink = INK; browser.dim = DIM;
    browser.bg0 = juce::Colour (0xf505040a); browser.bg1 = juce::Colour (0xf5120a1e);
    root.addChildComponent (browser);

    hotR[H_PREV] = { 420, 12, 30, 32 }; hotR[H_NEXT] = { 766, 12, 30, 32 }; hotR[H_NAME] = { 452, 10, 312, 36 };
    hotR[H_SAVE] = { 828, 16, 70, 24 }; hotR[H_UNDO] = { 904, 16, 84, 24 }; hotR[H_REDO] = { 990, 16, 74, 24 };
    for (int k = 0; k < kScenes; ++k) hotR[H_SCENE0 + k] = signRect (k);
    knobs[0]->setBounds (430, 418, 320, 250);
    for (int j = 0; j < 3; ++j) knobs[(size_t) (1 + j)]->setBounds (60 + j * 118, 462, 112, 150);
    for (int j = 0; j < 3; ++j) knobs[(size_t) (4 + j)]->setBounds (768 + j * 118, 462, 112, 150);
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
    if (h >= H_SCENE0) { proc.undo.beginNewTransaction(); proc.setP ("scene", (float) (h - H_SCENE0)); root.repaint(); return; }
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
    juce::Random& rnd = juce::Random::getSystemRandom();
    const int sc = scene();
    const float c = proc.value ("city");
    // pioggia: forte davanti al club, leggera altrove; ferma se sei fuori dalla citta'
    rain.amount = c < 0.001f ? 0.15f : (sc == 4 ? 0.4f + 0.6f * proc.value ("ambience") : 0.25f + 0.2f * proc.value ("ambience"));
    if (! browser.isVisible()) { rain.step(); rain.repaint(); }
    // l'insegna accesa sfarfalla ogni tanto (piu' spesso se il segnale e' disturbato)
    const int oldF = flick;
    if (flick > 0) --flick; else if (c > 0.001f && rnd.nextFloat() < 0.008f + 0.03f * proc.value ("disturb") * (sc == 3 ? 1.0f : 0.3f)) flick = 1 + rnd.nextInt (3);
    if ((oldF > 0) != (flick > 0) || sc != lastScene) { lastScene = sc; root.repaint (signRect (sc).expanded (60, 50).getUnion ({ 0, 56, (float) W, CONSOLE - 56 }).toNearestInt()); root.repaint ({ 40, (int) CONSOLE + 16, W - 80, 48 }); }
    outLvl = juce::jmax (proc.uiOut.load(), outLvl * 0.82f); gate = proc.uiGate.load();
    if (std::abs (outLvl - lastOut) > 0.01f || std::abs (gate - lastGate) > 0.05f) { lastOut = outLvl; lastGate = gate; root.repaint ({ W - 360, (int) CONSOLE + 16, 320, 48 }); }
    if (proc.preset != lastPreset || proc.userName != lastUser) { lastPreset = proc.preset; lastUser = proc.userName; root.repaint (hotR[H_NAME].expanded (6).toNearestInt()); }
    if (tick % 8 == 0) root.repaint (hotR[H_UNDO].getUnion (hotR[H_REDO]).toNearestInt());
    for (auto& k : knobs) { const float v = k->prm->getValue(); if (v != k->last) { if (k->big && (k->last < 0.001f) != (v < 0.001f)) root.repaint ({ 40, (int) CONSOLE + 16, W - 80, 48 }); k->last = v; k->repaint(); } }
}
}
