#pragma once
// Disegno "hardware" condiviso: manopole tridimensionali, viti, cornici cyber (da Studio Comp)
#include "JackpotLook.h"
namespace jlook { inline juce::Font ui2 (float s) { return ui (s, 2); } }
namespace hw
{
struct KnobSpec
{
    int style = 0;   // 0 bakelite nera, 1 alluminio, 2 cappuccio colorato
    juce::Colour col { 0xff5ef2ff }, ink { 0xffe8e4d8 }, cap { 0xff8b8f96 };
    juce::String label, text, lo, hi;
    bool enabled = true, bipolar = false, hover = false, drag = false, big = false;
};
// area: etichetta in alto (14 px), manopola, display in basso (17 px)
inline void knob (juce::Graphics& g, juce::Rectangle<float> area, float v0, const KnobSpec& k)
{
    g.saveState(); g.addTransform (juce::AffineTransform::translation (area.getX(), area.getY()));
    const float W = area.getWidth(), H = area.getHeight();

    using namespace juce;
    const float lh = 14, rh = 17;
    const float size = jmin ((float) W, H - lh - rh - 2);
    const float cx = W * 0.5f, cy = lh + size * 0.5f, R = size * 0.5f - 1;
    const float a0 = -2.36f, a1 = 2.36f, v = v0, av = a0 + v * (a1 - a0);
    auto P = [cx, cy] (float a, float r) { return Point<float> (cx + std::sin (a) * r, cy - std::cos (a) * r); };
    const float al = k.enabled ? 1.0f : 0.35f;
    // etichetta incisa
    g.setColour (k.ink.withAlpha (0.92f * al)); g.setFont (jlook::ui2 (k.big ? 10.5f : 9.5f)); g.drawText (k.label, Rectangle<float> (-10, 0, W + 20.0f, lh), Justification::centredTop);
    // scala stampata sul pannello
    for (int i = 0; i <= 10; ++i)
    {
        const float a = a0 + i * (a1 - a0) / 10, l = (i % 5 == 0) ? 5.0f : 3.0f;
        g.setColour (k.ink.withAlpha ((i % 5 == 0 ? 0.85f : 0.5f) * al)); g.drawLine (Line<float> (P (a, R), P (a, R - l)), i % 5 == 0 ? 1.4f : 1.0f);
    }
    if (k.lo.isNotEmpty()) { g.setFont (jlook::mono (8.0f)); g.setColour (k.ink.withAlpha (0.65f * al)); auto q0 = P (a0, R + 1), q1 = P (a1, R + 1);
        g.drawText (k.lo, Rectangle<float> (q0.x - 34, q0.y - 1, 34, 10), Justification::centredRight); g.drawText (k.hi, Rectangle<float> (q1.x, q1.y - 1, 40, 10), Justification::centredLeft); }
    // arco al neon del valore (dettaglio cyber)
    const float ra = R - 7;
    if (k.enabled)
    {
        const float s0 = k.bipolar ? 0.0f : a0;
        Path arc; arc.addCentredArc (cx, cy, ra, ra, 0, jmin (s0, av), jmax (s0, av), true);
        const float live = (k.hover ? 0.3f : 0.0f) + (k.drag ? 0.5f : 0.0f);
        g.setColour (k.col.withAlpha (0.25f + 0.2f * live)); g.strokePath (arc, PathStrokeType (4.5f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour (k.col.interpolatedWith (Colours::white, 0.15f + 0.3f * live)); g.strokePath (arc, PathStrokeType (1.6f, PathStrokeType::curved, PathStrokeType::rounded));
    }
    // corpo della manopola: cilindro con ombra morbida, fianco, faccia superiore e riflesso fisso (la luce non gira con la manopola)
    const float rs = R - 10, rc = rs * (k.style == 1 ? 0.7f : k.style == 2 ? 0.6f : 0.64f), depth = rs * 0.16f;
    for (int i = 6; i >= 1; --i) { const float e = i * rs * 0.07f; g.setColour (Colours::black.withAlpha (0.07f)); g.fillEllipse (cx - rs - e * 0.6f + depth * 0.5f, cy - rs + depth * 1.4f - e * 0.4f, 2 * rs + e * 1.2f, 2 * rs + e * 1.2f); }
    auto disc = [&] (float r, float dy) { return Rectangle<float> (cx - r, cy - r + dy, 2 * r, 2 * r); };
    if (k.style == 1)
    {   // alluminio tornito: fianco zigrinato, faccia con anelli concentrici e riflesso conico
        { ColourGradient sd (Colour (0xff9a9ea5), cx - rs, cy, Colour (0xff3d4045), cx + rs, cy, false); sd.addColour (0.35, Colour (0xffd8dbe0)); g.setGradientFill (sd); g.fillEllipse (disc (rs, depth)); }
        { ColourGradient sk (Colour (0xfff2f3f5), cx - rs * 0.6f, cy - rs, Colour (0xff70747a), cx + rs * 0.5f, cy + rs, false); g.setGradientFill (sk); g.fillEllipse (disc (rs, 0)); }
        for (int i = 0; i < 48; ++i) { const float a = av + i * MathConstants<float>::twoPi / 48; g.setColour (Colours::black.withAlpha (0.18f)); g.drawLine (Line<float> (P (a, rs - 0.3f), P (a, rs - 3.0f)), 0.9f); }
        g.setColour (Colours::black.withAlpha (0.35f)); g.drawEllipse (disc (rc + 1.2f, 0), 1.2f);
        { ColourGradient cg (Colour (0xffe9ebee), cx, cy - rc, Colour (0xffa2a6ad), cx, cy + rc, false); g.setGradientFill (cg); g.fillEllipse (disc (rc, 0)); }
        for (float r2 = rc - 1.5f; r2 > 2; r2 -= 1.6f) { g.setColour ((((int) (r2 * 3)) & 1 ? Colours::white : Colours::black).withAlpha (0.05f)); g.drawEllipse (disc (r2, 0), 0.8f); }
        {   // riflesso conico: due spicchi chiari opposti
            for (float base : { -0.9f, 2.24f })
            { Path sp; sp.addPieSegment (disc (rc, 0), base - 0.35f, base + 0.35f, 0.0f); ColourGradient hg (Colours::white.withAlpha (0.55f), cx, cy, Colours::white.withAlpha (0.0f), P (base, rc).x, P (base, rc).y, false); g.setGradientFill (hg); g.fillPath (sp); }
        }
        g.setColour (Colour (0xff15161a)); g.drawLine (Line<float> (P (av, rc * 0.3f), P (av, rs - 1.5f)), 2.4f);
        g.setColour (Colours::white.withAlpha (0.3f)); g.drawLine (Line<float> (P (av + 0.06f, rc * 0.3f), P (av + 0.04f, rc * 0.95f)), 0.8f);
    }
    else if (k.style == 2)
    {   // manopola da console: gonna scura con alette, cappuccio colorato lucido
        { ColourGradient sd (Colour (0xff2a2c31), cx - rs, cy, Colour (0xff0b0c0e), cx + rs, cy, false); g.setGradientFill (sd); g.fillEllipse (disc (rs, depth)); }
        { ColourGradient sk (Colour (0xff50545b), cx, cy - rs, Colour (0xff15161a), cx, cy + rs, false); g.setGradientFill (sk); g.fillEllipse (disc (rs, 0)); }
        for (int i = 0; i < 24; ++i) { const float a = av + i * MathConstants<float>::twoPi / 24; g.setColour (Colours::white.withAlpha (0.08f)); g.drawLine (Line<float> (P (a, rs - 0.5f), P (a, rc + 2)), 1.1f); g.setColour (Colours::black.withAlpha (0.25f)); g.drawLine (Line<float> (P (a + 0.05f, rs - 0.5f), P (a + 0.05f, rc + 2)), 0.8f); }
        const auto cc = k.enabled ? k.cap : Colour (0xff55585e);
        { ColourGradient cs (cc.darker (0.8f), cx - rc, cy, cc.darker (1.6f), cx + rc, cy, false); g.setGradientFill (cs); g.fillEllipse (disc (rc, depth * 0.7f)); }
        { ColourGradient cg (cc.brighter (0.5f), cx - rc * 0.5f, cy - rc, cc.darker (0.45f), cx + rc * 0.4f, cy + rc, true); g.setGradientFill (cg); g.fillEllipse (disc (rc, 0)); }
        { ColourGradient hl (Colours::white.withAlpha (0.55f), cx - rc * 0.2f, cy - rc * 0.75f, Colours::white.withAlpha (0.0f), cx, cy, false); g.setGradientFill (hl); g.fillEllipse (cx - rc * 0.7f, cy - rc * 0.92f, rc * 1.3f, rc * 0.85f); }
        g.setColour (Colours::white.withAlpha (0.95f)); g.drawLine (Line<float> (P (av, rc * 0.25f), P (av, rs - 1.5f)), 2.1f);
    }
    else
    {   // bakelite nera grande, zigrinata, con punta crema (anni 60)
        { ColourGradient sd (Colour (0xff202124), cx - rs, cy, Colour (0xff030304), cx + rs, cy, false); g.setGradientFill (sd); g.fillEllipse (disc (rs, depth)); }
        { ColourGradient sk (Colour (0xff3f4045), cx - rs * 0.4f, cy - rs, Colour (0xff060607), cx + rs * 0.4f, cy + rs, false); g.setGradientFill (sk); g.fillEllipse (disc (rs, 0)); }
        for (int i = 0; i < 40; ++i) { const float a = av + i * MathConstants<float>::twoPi / 40; g.setColour (Colours::white.withAlpha (0.1f)); g.drawLine (Line<float> (P (a, rs - 0.5f), P (a, rs - 4.0f)), 1.0f); }
        { ColourGradient cs (Colour (0xff1b1c1f), cx, cy - rc, Colour (0xff000000), cx, cy + rc, false); g.setGradientFill (cs); g.fillEllipse (disc (rc + 1.5f, 1.0f)); }
        { ColourGradient cg (Colour (0xff35363b), cx - rc * 0.5f, cy - rc, Colour (0xff040405), cx + rc * 0.5f, cy + rc, true); g.setGradientFill (cg); g.fillEllipse (disc (rc, 0)); }
        { ColourGradient hl (Colours::white.withAlpha (0.28f), cx - rc * 0.25f, cy - rc * 0.8f, Colours::white.withAlpha (0.0f), cx, cy - rc * 0.1f, false); g.setGradientFill (hl); g.fillEllipse (cx - rc * 0.75f, cy - rc * 0.95f, rc * 1.4f, rc * 0.9f); }
        g.setColour (Colours::black.withAlpha (0.6f)); g.drawLine (Line<float> (P (av, rc * 0.12f), P (av, rs - 1.0f)), 4.0f);
        g.setColour (Colour (0xfff3eee2).withAlpha (al)); g.drawLine (Line<float> (P (av, rc * 0.15f), P (av, rs - 1.5f)), 2.4f);
    }
    // lettura in un piccolo display
    auto rr = Rectangle<float> (4, H - rh, W - 8.0f, rh - 1);
    g.setColour (Colour (0xe6050608)); g.fillRoundedRectangle (rr, 4); g.setColour (k.col.withAlpha (k.enabled ? 0.45f : 0.15f)); g.drawRoundedRectangle (rr.reduced (0.5f), 4, 1.0f);
    g.setColour (k.enabled ? (k.drag ? k.col.brighter (0.4f) : k.col) : juce::Colour (0xff4f5562)); g.setFont (jlook::mono (k.big ? 11.0f : 10.0f)); g.drawText (k.text, rr, Justification::centred);

    g.restoreState();
}
inline void screw (juce::Graphics& g, float x, float y, float r, float rot)
{
    juce::ColourGradient sg (juce::Colour (0xffd5d7db), x - r * 0.5f, y - r * 0.6f, juce::Colour (0xff55585e), x + r * 0.5f, y + r * 0.7f, false);
    g.setColour (juce::Colours::black.withAlpha (0.45f)); g.fillEllipse (x - r - 1, y - r + 0.5f, 2 * r + 2, 2 * r + 2);
    g.setGradientFill (sg); g.fillEllipse (x - r, y - r, 2 * r, 2 * r);
    g.setColour (juce::Colour (0xff2a2c30)); const float c = std::cos (rot) * r * 0.75f, sn = std::sin (rot) * r * 0.75f;
    g.drawLine (x - c, y - sn, x + c, y + sn, 1.3f); g.drawLine (x + sn, y - c, x - sn, y + c, 1.3f);
}
inline void cyberFrame (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour acc, float L = 18)
{
    g.setColour (acc.withAlpha (0.18f));
    for (int k = 0; k < 4; ++k)
    {
        const float x = (k & 1) ? r.getRight() : r.getX(), y = (k & 2) ? r.getBottom() : r.getY(), sx = (k & 1) ? -1.0f : 1.0f, sy = (k & 2) ? -1.0f : 1.0f;
        juce::Path p; p.startNewSubPath (x + sx * L, y); p.lineTo (x, y); p.lineTo (x, y + sy * L);
        g.setColour (acc.withAlpha (0.25f)); g.strokePath (p, juce::PathStrokeType (4.0f)); g.setColour (acc); g.strokePath (p, juce::PathStrokeType (1.5f));
    }
}

// spia a gemma (vetro colorato)
inline void jewel (juce::Graphics& g, float lx, float ly, float lr, bool on, juce::Colour c = juce::Colour (0xffff3b30))
{
    g.setColour (juce::Colour (0xff9aa0a8)); g.fillEllipse (lx - lr - 2, ly - lr - 2, 2 * lr + 4, 2 * lr + 4);
    g.setColour (juce::Colour (0xff2a2c30)); g.drawEllipse (lx - lr - 2, ly - lr - 2, 2 * lr + 4, 2 * lr + 4, 1.0f);
    const auto lc = on ? c : c.darker (2.2f);
    if (on) { juce::ColourGradient glow (lc.withAlpha (0.45f), lx, ly, lc.withAlpha (0.0f), lx + lr * 2.8f, ly, true); g.setGradientFill (glow); g.fillEllipse (lx - lr * 2.8f, ly - lr * 2.8f, lr * 5.6f, lr * 5.6f); }
    juce::ColourGradient jg (lc.brighter (0.6f), lx - lr * 0.4f, ly - lr * 0.5f, lc.darker (0.8f), lx + lr, ly + lr, true); g.setGradientFill (jg); g.fillEllipse (lx - lr, ly - lr, 2 * lr, 2 * lr);
    for (int k = 0; k < 6; ++k) { const float a = k * 1.047f; g.setColour (juce::Colours::white.withAlpha (0.12f)); g.drawLine (lx, ly, lx + std::cos (a) * lr, ly + std::sin (a) * lr, 0.7f); }
    g.setColour (juce::Colours::white.withAlpha (0.7f)); g.fillEllipse (lx - lr * 0.5f, ly - lr * 0.6f, lr * 0.5f, lr * 0.35f);
}
// cornice cromata incassata con 4 viti (per VU e display)
inline void bezel (juce::Graphics& g, juce::Rectangle<float> r, float rad = 9)
{
    g.setColour (juce::Colours::black.withAlpha (0.55f)); g.fillRoundedRectangle (r.expanded (8).translated (0, 2), rad + 4);
    juce::ColourGradient ch (juce::Colour (0xffd9dce2), 0, r.getY() - 7, juce::Colour (0xff3b3e44), 0, r.getBottom() + 7, false); ch.addColour (0.5, juce::Colour (0xff7d8189));
    g.setGradientFill (ch); g.fillRoundedRectangle (r.expanded (7), rad + 3);
    g.setColour (juce::Colour (0xff0a0b0e)); g.fillRoundedRectangle (r.expanded (3), rad);
    g.setColour (juce::Colours::white.withAlpha (0.35f)); g.drawRoundedRectangle (r.expanded (6.5f), rad + 2.5f, 0.8f);
    for (auto pt : { r.getTopLeft().translated (-3.5f, -3.5f), r.getTopRight().translated (3.5f, -3.5f), r.getBottomLeft().translated (-3.5f, 3.5f), r.getBottomRight().translated (3.5f, 3.5f) })
    { g.setColour (juce::Colour (0xff23252a)); g.fillEllipse (pt.x - 2.2f, pt.y - 2.2f, 4.4f, 4.4f); g.setColour (juce::Colours::white.withAlpha (0.4f)); g.drawLine (pt.x - 1.5f, pt.y, pt.x + 1.5f, pt.y, 0.7f); }
}
// frontale: telaio scuro con bisello, pannello incassato spazzolato con curvatura, binario scuro in alto, viti, cornice cyber
inline juce::Rectangle<float> faceplate (juce::Graphics& g, juce::Rectangle<float> b, juce::Colour p1, juce::Colour p2, juce::Colour acc, float railH = 34, int seed = 1, float rad = 14)
{
    { juce::ColourGradient fr (juce::Colour (0xff30333a), 0, b.getY(), juce::Colour (0xff050607), 0, b.getBottom(), false); g.setGradientFill (fr); g.fillRoundedRectangle (b, rad); }
    g.setColour (juce::Colours::white.withAlpha (0.10f)); g.drawRoundedRectangle (b.reduced (0.5f), rad, 1.0f);
    auto pl = b.reduced (5);
    g.setColour (juce::Colours::black.withAlpha (0.8f)); g.fillRoundedRectangle (pl.expanded (1.5f).translated (0, 1.5f), rad - 3);
    { juce::ColourGradient pg (p1, 0, pl.getY(), p2, 0, pl.getBottom(), false); g.setGradientFill (pg); g.fillRoundedRectangle (pl, rad - 4); }
    {
        g.saveState(); g.reduceClipRegion (pl.toNearestInt());
        juce::Random rnd (seed);
        for (float y = pl.getY(); y < pl.getBottom(); y += 1.5f) { g.setColour ((rnd.nextBool() ? juce::Colours::white : juce::Colours::black).withAlpha (0.018f + rnd.nextFloat() * 0.02f)); g.drawHorizontalLine ((int) y, pl.getX(), pl.getRight()); }
        juce::ColourGradient vg (juce::Colours::white.withAlpha (0.06f), pl.getCentreX(), pl.getY() + pl.getHeight() * 0.35f, juce::Colours::black.withAlpha (0.22f), pl.getX(), pl.getBottom(), true);
        g.setGradientFill (vg); g.fillRect (pl);
        g.restoreState();
    }
    { juce::ColourGradient bv (juce::Colours::white.withAlpha (0.35f), 0, pl.getY(), juce::Colours::black.withAlpha (0.55f), 0, pl.getBottom(), false); g.setGradientFill (bv); g.drawRoundedRectangle (pl.reduced (0.5f), rad - 4, 1.4f); }
    if (railH > 0)
    {
        juce::Path rp; rp.addRoundedRectangle (pl.getX(), pl.getY(), pl.getWidth(), railH, rad - 4, rad - 4, true, true, false, false);
        g.setColour (juce::Colour (0xe0090a0d)); g.fillPath (rp); g.setColour (acc.withAlpha (0.55f)); g.fillRect (pl.getX() + 10, pl.getY() + railH - 1, pl.getWidth() - 20, 1.0f);
    }
    for (auto x : { pl.getX() + 11, pl.getRight() - 11 }) screw (g, x, pl.getBottom() - 11, 4.5f, 0.4f + x * 0.01f);
    {
        juce::ColourGradient ug (acc.withAlpha (0.0f), b.getX() + 30, 0, acc.withAlpha (0.0f), b.getRight() - 30, 0, false); ug.addColour (0.5, acc.withAlpha (0.55f));
        g.setGradientFill (ug); g.fillRect (b.getX() + 30, b.getBottom() - 2.5f, b.getWidth() - 60, 1.5f);
    }
    cyberFrame (g, b.reduced (1.5f), acc, 14);
    return pl;
}
}
