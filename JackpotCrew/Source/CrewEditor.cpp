#include "CrewEditor.h"

namespace jcrew
{
using namespace sk;
using juce::Path;
juce::Colour sk::accent (int k)
{
    static const juce::Colour c[] = { juce::Colour (0xffc9853f), juce::Colour (0xff57c66a), juce::Colour (0xfff07a2e), juce::Colour (0xff3fc6c0), juce::Colour (0xff4f7fd0), juce::Colour (0xffe2b43a) };
    return c[juce::jlimit (0, kVoices - 1, k)];
}
namespace
{
// tratto a mano: il contorno trema un poco, ed e' ripassato due volte
Path wob (const Path& src, int seed, float amp)
{
    Path out; juce::PathFlatteningIterator it (src, {}, 0.8f); int sub = -1; float t = 0;
    const float s = (float) (seed % 97) * 0.37f;
    auto n = [&] (float tt) { return amp * (0.6f * std::sin (tt * 0.09f + s) + 0.4f * std::sin (tt * 0.031f + s * 1.7f)); };
    while (it.next())
    {
        t += std::hypot (it.x2 - it.x1, it.y2 - it.y1);
        if (it.subPathIndex != sub) { sub = it.subPathIndex; out.startNewSubPath (it.x1 + n (t + 11), it.y1 + n (t + 37)); }
        out.lineTo (it.x2 + n (t + 11), it.y2 + n (t + 37));
        if (it.closesSubPath) out.closeSubPath();
    }
    return out;
}
void ink (juce::Graphics& g, const Path& p, int seed, float w, bool on = true)
{
    const auto c = on ? INK : PENCIL;
    g.setColour (c); g.strokePath (wob (p, seed, 0.8f), juce::PathStrokeType (w, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (c.withAlpha (0.55f)); g.strokePath (wob (p, seed + 13, 1.1f), juce::PathStrokeType (w * 0.45f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}
// colore piatto fuori registro (come una stampa a due passaggi)
void flat (juce::Graphics& g, const Path& p, juce::Colour c, bool on) { if (! on) return; g.setColour (c); g.fillPath (p, juce::AffineTransform::translation (3.0f, 2.0f)); }
void both (juce::Graphics& g, const Path& p, juce::Colour c, bool on, int seed, float w = 2.2f) { flat (g, p, c, on); ink (g, p, seed, w, on); }
Path ell (float x, float y, float w, float h) { Path p; p.addEllipse (x, y, w, h); return p; }
Path rr (float x, float y, float w, float h, float r) { Path p; p.addRoundedRectangle (x, y, w, h, r); return p; }
Path poly (std::initializer_list<juce::Point<float>> pts) { Path p; bool f = true; for (auto q : pts) { if (f) p.startNewSubPath (q); else p.lineTo (q); f = false; } p.closeSubPath(); return p; }
void text (juce::Graphics& g, const juce::String& t, juce::Rectangle<float> r, juce::Font f, juce::Colour c, juce::Justification j = juce::Justification::centred) { g.setFont (f); g.setColour (c); g.drawText (t, r, j, false); }
// foglio di carta con il bordo un po' storto e il nastro adesivo
void sheet (juce::Graphics& g, juce::Rectangle<float> r, int seed, juce::Colour c = SHEET)
{
    g.setColour (juce::Colours::black.withAlpha (0.12f)); g.fillRect (r.translated (4, 5));
    g.setColour (c); g.fillRect (r);
    Path e; e.startNewSubPath (r.getTopLeft()); e.lineTo (r.getTopRight()); e.startNewSubPath (r.getTopRight()); e.lineTo (r.getBottomRight());
    e.startNewSubPath (r.getBottomRight()); e.lineTo (r.getBottomLeft()); e.startNewSubPath (r.getBottomLeft()); e.lineTo (r.getTopLeft());
    g.setColour (PENCIL.withAlpha (0.5f)); g.strokePath (wob (e, seed + 5, 1.0f), juce::PathStrokeType (1.0f));
}
void tape (juce::Graphics& g, float cx, float cy, float ang, float w = 64)
{
    g.saveState(); g.addTransform (juce::AffineTransform::rotation (ang, cx, cy));
    g.setColour (juce::Colour (0xccf2ead2)); g.fillRect (cx - w * 0.5f, cy - 10, w, 20.0f);
    g.setColour (juce::Colours::black.withAlpha (0.08f)); for (float x = cx - w * 0.5f; x < cx + w * 0.5f; x += 4) g.drawVerticalLine ((int) x, cy - 10, cy - 7);
    g.restoreState();
}
juce::Point<float> mouthAt (int k) { return k == 5 ? juce::Point<float> (140, 110) : k == 1 ? juce::Point<float> (140, 109) : juce::Point<float> (140, 106); }
}

// =============================================================================================== personaggi
void Editor::drawCharacter (juce::Graphics& g, int k, juce::Rectangle<float> box, bool on)
{
    g.saveState(); g.addTransform (juce::AffineTransform::translation (box.getX(), box.getY()));
    const int s = 100 + k * 31;
    auto shoulders = [] () { Path p; p.startNewSubPath (30, 176); p.cubicTo (40, 150, 80, 138, 108, 134); p.lineTo (172, 134); p.cubicTo (200, 138, 240, 150, 250, 176); p.closeSubPath(); return p; };
    auto neck = rr (122, 108, 36, 34, 6);
    switch (k)
    {
        case 0:
        {   // BIG MO: completo scuro, cresta corta, visiera nera, orecchino d'oro
            const juce::Colour skin (0xff8a5a3c);
            both (g, shoulders(), juce::Colour (0xff3b3f46), on, s);
            both (g, poly ({ { 124, 134 }, { 156, 134 }, { 140, 168 } }), SHEET, on, s + 1, 1.8f);
            ink (g, poly ({ { 112, 136 }, { 140, 172 }, { 168, 136 } }), s + 2, 2.0f, on);
            both (g, neck, skin.darker (0.15f), on, s + 3);
            both (g, ell (94, 70, 12, 20), skin, on, s + 4, 1.6f); both (g, ell (174, 70, 12, 20), skin, on, s + 5, 1.6f);
            both (g, rr (102, 36, 76, 90, 28), skin, on, s + 6);
            both (g, rr (126, 20, 28, 30, 10), INK, on, s + 7);
            both (g, poly ({ { 100, 70 }, { 180, 70 }, { 176, 86 }, { 148, 89 }, { 140, 82 }, { 132, 89 }, { 104, 86 } }), INK, on, s + 8);
            if (on) { g.setColour (SHEET.withAlpha (0.8f)); g.drawLine (110, 74, 126, 74, 2.0f); }
            both (g, ell (96, 92, 7, 7), juce::Colour (0xffe8b83a), on, s + 9, 1.2f);
            ink (g, poly ({ { 136, 92 }, { 144, 92 }, { 140, 98 } }), s + 10, 1.4f, on);
            break;
        }
        case 1:
        {   // SPIKE: cresta verde, occhiali rosa a punta, giubbotto di pelle con borchie, sciarpa rossa
            const juce::Colour skin (0xfff1c7a3);
            both (g, shoulders(), juce::Colour (0xff2a2a2e), on, s);
            both (g, rr (116, 120, 48, 16, 6), RED, on, s + 1, 1.8f);
            both (g, neck, skin.darker (0.1f), on, s + 2);
            both (g, ell (106, 40, 68, 86), skin, on, s + 3);
            both (g, poly ({ { 116, 50 }, { 110, 14 }, { 126, 38 }, { 130, 2 }, { 140, 34 }, { 150, 4 }, { 154, 38 }, { 168, 14 }, { 164, 50 }, { 140, 42 } }), juce::Colour (0xff57c66a), on, s + 4);
            both (g, poly ({ { 106, 66 }, { 136, 66 }, { 126, 86 } }), juce::Colour (0xffff5fa8), on, s + 5, 1.8f);
            both (g, poly ({ { 144, 66 }, { 174, 66 }, { 154, 86 } }), juce::Colour (0xffff5fa8), on, s + 6, 1.8f);
            { Path b; b.startNewSubPath (136, 68); b.lineTo (144, 68); ink (g, b, s + 7, 2.0f, on); }
            ink (g, ell (137, 93, 6, 6), s + 8, 1.3f, on);
            for (int i = 0; i < 7; ++i) { const float x = 60 + i * 26.0f, y = 158 - std::sin ((float) i / 6 * 3.14f) * 18; both (g, ell (x, y, 7, 7), STEEL, on, s + 20 + i, 1.0f); }
            both (g, poly ({ { 104, 136 }, { 92, 120 }, { 116, 132 } }), STEEL, on, s + 30, 1.4f); both (g, poly ({ { 176, 136 }, { 188, 120 }, { 164, 132 } }), STEEL, on, s + 31, 1.4f);
            break;
        }
        case 2:
        {   // TRIDI: caschetto biondo, occhiali 3D, giacca arancione a collo alto, lentiggini
            const juce::Colour skin (0xfff6d3b5), hair (0xfff4d36b);
            both (g, shoulders(), juce::Colour (0xfff07a2e), on, s);
            both (g, ell (92, 30, 96, 104), hair, on, s + 1);
            both (g, neck, skin.darker (0.08f), on, s + 2);
            both (g, poly ({ { 108, 124 }, { 172, 124 }, { 178, 152 }, { 102, 152 } }), juce::Colour (0xffd9601e), on, s + 3);
            both (g, ell (108, 42, 64, 82), skin, on, s + 4);
            { Path b; b.startNewSubPath (104, 68); b.cubicTo (106, 28, 174, 28, 176, 68); b.cubicTo (162, 58, 150, 62, 140, 54); b.cubicTo (128, 62, 116, 58, 104, 68); b.closeSubPath(); both (g, b, hair, on, s + 5); }
            both (g, rr (104, 68, 72, 22, 4), SHEET, on, s + 6, 1.8f);
            both (g, rr (108, 71, 28, 15, 2), juce::Colour (0xffe0453c), on, s + 7, 1.2f); both (g, rr (144, 71, 28, 15, 2), juce::Colour (0xff4cc6d6), on, s + 8, 1.2f);
            if (on) { g.setColour (juce::Colour (0xffc0704a)); for (int i = 0; i < 6; ++i) g.fillEllipse (116.0f + (i % 3) * 5 + (i > 2 ? 34 : 0), 94.0f + (i % 2) * 3, 2.2f, 2.2f); }
            break;
        }
        case 3:
        {   // ASTRO: casco con visiera (si accende quando canta), antenna, anello arancione
            both (g, shoulders(), juce::Colour (0xffd8cdb6), on, s);
            both (g, rr (100, 126, 80, 16, 7), juce::Colour (0xfff07a2e), on, s + 1);
            { Path a; a.startNewSubPath (170, 34); a.lineTo (188, 8); ink (g, a, s + 2, 2.0f, on); both (g, ell (184, 2, 9, 9), RED, on, s + 3, 1.4f); }
            both (g, ell (88, 20, 104, 112), juce::Colour (0xffecebe6), on, s + 4);
            if (on) { Path sh; sh.addPieSegment (88, 20, 104, 112, 0.6f, 2.6f, 0.75f); g.setColour (juce::Colour (0xffb9b7b0)); g.fillPath (sh, juce::AffineTransform::translation (3, 2)); }
            both (g, rr (102, 48, 76, 58, 22), juce::Colour (0xff2c3e55), on, s + 5);
            both (g, rr (120, 112, 40, 10, 3), STEEL, on, s + 6, 1.2f);
            break;
        }
        case 4:
        {   // BANDANA: felpa col cappuccio, berretto rosso, bandana blu a pois sulla bocca
            const juce::Colour skin (0xffc98e6a);
            both (g, shoulders(), juce::Colour (0xff6c6f78), on, s);
            both (g, ell (90, 28, 100, 116), juce::Colour (0xff5d6068), on, s + 1);
            both (g, neck, skin.darker (0.12f), on, s + 2);
            both (g, ell (108, 42, 64, 82), skin, on, s + 3);
            { Path b; b.startNewSubPath (104, 64); b.cubicTo (106, 20, 174, 20, 176, 64); b.closeSubPath(); both (g, b, juce::Colour (0xffc8463c), on, s + 4); }
            both (g, rr (102, 54, 76, 12, 4), juce::Colour (0xff9e3029), on, s + 5, 1.6f);
            { Path e; e.startNewSubPath (118, 76); e.lineTo (132, 74); e.startNewSubPath (148, 74); e.lineTo (162, 76); ink (g, e, s + 6, 2.4f, on); }
            { Path e; e.startNewSubPath (116, 68); e.lineTo (132, 66); e.startNewSubPath (148, 66); e.lineTo (164, 68); ink (g, e, s + 7, 1.6f, on); }
            { Path st; st.startNewSubPath (128, 140); st.lineTo (124, 170); st.startNewSubPath (152, 140); st.lineTo (156, 170); ink (g, st, s + 8, 1.6f, on); }
            break;
        }
        default:
        {   // GRILLZ: cappello a secchiello, occhiali tondi, catena d'oro, denti d'oro quando canta
            const juce::Colour skin (0xff5b3a29);
            both (g, shoulders(), juce::Colour (0xff222226), on, s);
            { Path c; c.addCentredArc (140, 128, 34, 30, 0, 2.2f, 4.1f, true); g.setColour (on ? juce::Colour (0xffe8b83a) : PENCIL); g.strokePath (wob (c, s, 0.6f), juce::PathStrokeType (4.0f)); }
            both (g, neck, skin.darker (0.1f), on, s + 1);
            both (g, ell (108, 44, 64, 82), skin, on, s + 2);
            both (g, ell (92, 44, 96, 20), juce::Colour (0xffe8dcc0), on, s + 3);
            both (g, rr (112, 22, 56, 34, 12), juce::Colour (0xffe8dcc0), on, s + 4);
            both (g, ell (112, 70, 24, 24), INK, on, s + 5, 2.0f); both (g, ell (144, 70, 24, 24), INK, on, s + 6, 2.0f);
            if (on) { g.setColour (juce::Colour (0xffe8b83a)); g.drawEllipse (112, 70, 24, 24, 1.4f); g.drawEllipse (144, 70, 24, 24, 1.4f); g.setColour (SHEET.withAlpha (0.8f)); g.drawLine (118, 76, 124, 74, 1.6f); }
            { Path b; b.startNewSubPath (136, 80); b.lineTo (144, 80); ink (g, b, s + 7, 1.6f, on); }
            break;
        }
    }
    g.restoreState();
}
void Editor::drawMouth (juce::Graphics& g, int k, juce::Rectangle<float> box, bool on, float open, int tk)
{
    g.saveState(); g.addTransform (juce::AffineTransform::translation (box.getX(), box.getY()));
    const auto m = mouthAt (k); open = juce::jlimit (0.0f, 1.0f, open);
    if (k == 3)
    {   // il casco: la visiera si accende e ne escono onde
        g.setColour (juce::Colour (0xff3fd3c6).withAlpha (on ? 0.12f + 0.65f * open : 0.0f)); g.fillRoundedRectangle (106, 52, 70, 52, 20);
        if (on) { g.setColour (SHEET.withAlpha (0.7f)); g.drawLine (112, 58, 126, 56, 2.5f); }
    }
    else if (k == 4)
    {   // bandana: la stoffa si gonfia
        Path b; const float bulge = 6 + open * 10;
        b.startNewSubPath (104, 84); b.lineTo (176, 84); b.cubicTo (176, 100, 160, 118 + bulge * 0.4f, 140, 128 + bulge * 0.3f); b.cubicTo (120, 118 + bulge * 0.4f, 104, 100, 104, 84); b.closeSubPath();
        both (g, b, juce::Colour (0xff3567b5), on, 400 + tk / 6 % 3);
        if (on) { g.setColour (SHEET); for (int i = 0; i < 9; ++i) g.fillEllipse (112.0f + (i % 5) * 12, 90.0f + (i / 5) * 12 + (i % 2) * 3, 3, 3); }
    }
    else
    {
        if (open < 0.08f || ! on) { Path l; l.startNewSubPath (m.x - 9, m.y); l.quadraticTo (m.x, m.y + 2, m.x + 9, m.y); ink (g, l, 300 + k, 2.0f, on); }
        else
        {
            const float w = 14 + 10 * open + (k == 1 ? 6 : 0), h = 4 + 16 * open;
            Path mo; mo.addEllipse (m.x - w * 0.5f, m.y - h * 0.3f, w, h);
            g.setColour (INK); g.fillPath (mo);
            g.saveState(); g.reduceClipRegion (mo);
            g.setColour (k == 5 ? juce::Colour (0xffe8b83a) : SHEET); g.fillRect (m.x - w * 0.5f, m.y - h * 0.3f, w, juce::jmin (h * 0.3f, 5.0f));
            g.setColour (juce::Colour (0xffd9443a)); g.fillEllipse (m.x - w * 0.3f, m.y + h * 0.35f, w * 0.6f, h * 0.4f);
            g.restoreState();
            ink (g, mo, 300 + k + tk / 4 % 3, 1.8f, on);
        }
    }
    if (on && open > 0.25f)
    {   // segni del suono
        const float cx = k == 3 ? 196.0f : m.x + 34, cy = k == 3 ? 78.0f : m.y;
        for (int i = 0; i < 3; ++i) { Path a; const float r = 8 + i * 8 + (tk % 6) * 0.8f; a.addCentredArc (cx - 12, cy, r, r, 0, 0.9f, 2.2f, true); g.setColour (INK.withAlpha (0.9f - i * 0.25f)); g.strokePath (wob (a, i + tk / 3, 0.5f), juce::PathStrokeType (2.0f)); }
    }
    g.restoreState();
}

// =============================================================================================== carta del personaggio
void Card::paint (juce::Graphics& g)
{
    const bool on = ed.proc.value (Processor::vid (idx, "on")) > 0.5f;
    const float sc = juce::jlimit (1.0f, 4.0f, g.getInternalContext().getPhysicalPixelScaleFactor());
    if (cache.isNull() || std::abs (cacheScale - sc) > 0.01f || cacheOn != (int) on)
    {
        cacheScale = sc; cacheOn = on;
        cache = juce::Image (juce::Image::ARGB, juce::roundToInt (getWidth() * sc), juce::roundToInt (getHeight() * sc), true);
        juce::Graphics ig (cache); ig.addTransform (juce::AffineTransform::scale (sc));
        sheet (ig, getLocalBounds().toFloat().reduced (4), 50 + idx * 17);
        tape (ig, getWidth() * 0.5f, 6, (idx % 2 ? 0.05f : -0.06f));
        // fondo colorato dietro al busto (come un timbro)
        if (on) { ig.setColour (accent (idx).withAlpha (0.28f)); ig.fillRect (juce::Rectangle<float> (40, 30, 200, 146)); }
        Editor::drawCharacter (ig, idx, { 0, 0, (float) getWidth(), 176 }, on);
        const auto& c = characters()[(size_t) idx];
        if (on) { ig.setColour (accent (idx)); ig.fillRect (juce::Rectangle<float> (16, 182, 132, 30).translated (3, 2)); }
        text (ig, c.name, { 16, 178, 160, 36 }, beb (34.0f), on ? INK : PENCIL, juce::Justification::centredLeft);
        ig.setFont (mon (8.5f)); ig.setColour (on ? INK.withAlpha (0.75f) : PENCIL);
        ig.drawFittedText (c.role, juce::Rectangle<int> (156, 182, getWidth() - 168, 28), juce::Justification::centredRight, 2, 0.8f);
        if (! on) text (ig, "clicca per farlo entrare", { 0, 156, (float) getWidth(), 18 }, mon (10.0f), PENCIL);
    }
    g.drawImageTransformed (cache, juce::AffineTransform::scale (1.0f / sc));
    Editor::drawMouth (g, idx, { 0, 0, (float) getWidth(), 176 }, on, mouth, ed.tick);
    if (hover) { g.setColour (INK.withAlpha (0.5f)); g.drawRect (getLocalBounds().toFloat().reduced (6), 1.5f); }
}
void Card::mouseDown (const juce::MouseEvent& e)
{
    if (e.position.y > 176) return;
    ed.proc.undo.beginNewTransaction();
    const auto id = Processor::vid (idx, "on");
    ed.proc.setP (id, ed.proc.value (id) > 0.5f ? 0.0f : 1.0f);
    repaint();
}

// =============================================================================================== manopola a matita
Knob::Knob (Editor& e, const juce::String& id, juce::String l, juce::Colour f, std::function<juce::String (float)> fm, int sd)
    : prm (e.P (id)), label (std::move (l)), fill (f), fmt (std::move (fm)), seed (sd)
{ setRepaintsOnMouseActivity (true); setMouseCursor (juce::MouseCursor::UpDownResizeCursor); }
void Knob::paint (juce::Graphics& g)
{
    const float w = (float) getWidth(), h = (float) getHeight(), lh = 17, vh = 15;
    const float S = juce::jmin (w - 16, h - lh - vh - 8), cx = w * 0.5f, cy = lh + 4 + S * 0.5f, R = S * 0.5f;
    const float v = prm->getValue(), a0 = -2.36f, a1 = 2.36f, av = a0 + (a1 - a0) * v;
    text (g, label, { -10, 0, w + 20, lh }, beb (17.0f), INK);
    for (int i = 0; i <= 10; ++i)
    {
        const float a = a0 + (a1 - a0) * i / 10.0f, r1 = R + 3, r2 = R + (i % 5 == 0 ? 8.0f : 6.0f);
        g.setColour (PENCIL); g.drawLine (cx + std::sin (a) * r1, cy - std::cos (a) * r1, cx + std::sin (a) * r2, cy - std::cos (a) * r2, i % 5 == 0 ? 1.6f : 1.0f);
    }
    {   // il valore come un tratto di evidenziatore
        const float s0 = bipolar ? 0.0f : a0; Path p; p.addCentredArc (cx, cy, R + 4.5f, R + 4.5f, 0, juce::jmin (s0, av), juce::jmax (s0, av), true);
        g.setColour (fill.withAlpha (isMouseOverOrDragging() ? 0.95f : 0.75f)); g.strokePath (p, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    Path body; body.addEllipse (cx - R, cy - R, R * 2, R * 2);
    g.setColour (fill); g.fillPath (body, juce::AffineTransform::translation (2.5f, 2.0f));
    g.setColour (SHEET.withAlpha (0.55f)); g.fillEllipse (cx - R * 0.55f, cy - R * 0.7f, R * 0.6f, R * 0.35f);
    ink (g, body, seed, 2.0f);
    Path ptr; ptr.startNewSubPath (cx + std::sin (av) * R * 0.2f, cy - std::cos (av) * R * 0.2f); ptr.lineTo (cx + std::sin (av) * (R + 1), cy - std::cos (av) * (R + 1));
    g.setColour (INK); g.strokePath (ptr, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    const juce::String t = fmt ? fmt (prm->convertFrom0to1 (v)) : prm->getCurrentValueAsText();
    text (g, t, { -10, h - vh, w + 20, vh }, mon (10.5f), isMouseOverOrDragging() ? RED : INK);
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
void Knob::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w)
{
    const float dy = w.isReversed ? -w.deltaY : w.deltaY;
    if (steps > 1) { const float st = 1.0f / (steps - 1); prm->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, prm->getValue() + (dy > 0 ? st : -st))); }
    else prm->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, prm->getValue() + juce::jlimit (-0.05f, 0.05f, dy * 0.08f)));
}

// =============================================================================================== quaderno
void Editor::drawStatic (juce::Graphics& g)
{
    g.fillAll (PAPER);
    {   // grana della carta e quadretti
        juce::Random r (5);
        for (int i = 0; i < 9000; ++i) { g.setColour ((r.nextBool() ? juce::Colours::black : juce::Colours::white).withAlpha (0.035f)); g.fillRect (r.nextFloat() * W, r.nextFloat() * H, 1.2f, 1.2f); }
        g.setColour (BLUE.withAlpha (0.22f)); for (float x = 12; x < W; x += 24) g.drawVerticalLine ((int) x, 0, (float) H); for (float y = 12; y < H; y += 24) g.drawHorizontalLine ((int) y, 0, (float) W);
        g.setColour (RED.withAlpha (0.35f)); g.drawVerticalLine (14, 0, (float) H);
    }
    {   // macchia di caffe'
        for (int k = 0; k < 3; ++k) { g.setColour (juce::Colour (0xff8a5a2b).withAlpha (0.08f + k * 0.03f)); g.drawEllipse (840.0f - k, 640.0f + k * 0.5f, 120.0f + k * 2, 112.0f, 3.0f - k); }
    }
    // titolo a mano, stampato due volte fuori registro
    text (g, "CREW", { 27, 13, 300, 70 }, bun (64.0f), RED, juce::Justification::centredLeft);
    text (g, "CREW", { 22, 9, 300, 70 }, bun (64.0f), INK, juce::Justification::centredLeft);
    text (g, "JACKPOT CYBERNET  //  GANG VOCALS DA UNA VOCE", { 26, 74, 420, 16 }, mon (10.0f), INK.withAlpha (0.7f), juce::Justification::centredLeft);
    {   // scarabocchi a matita
        g.setColour (PENCIL.withAlpha (0.8f));
        Path star; star.addStar ({ 352, 44 }, 5, 6, 15, 0.2f); g.strokePath (wob (star, 3, 0.8f), juce::PathStrokeType (1.6f));
        Path ar; ar.startNewSubPath (842, 30); ar.quadraticTo (870, 10, 896, 34); ar.startNewSubPath (888, 24); ar.lineTo (896, 34); ar.lineTo (884, 38); g.strokePath (wob (ar, 9, 0.8f), juce::PathStrokeType (1.6f));
        g.saveState(); g.addTransform (juce::AffineTransform::rotation (-0.12f, 1100, 40)); text (g, "tutti insieme!!", { 1010, 28, 160, 24 }, beb (22.0f), PENCIL); g.restoreState();
    }
    // la cartellina della crew (comandi di tutti)
    {
        auto b = juce::Rectangle<float> (904, 96, 256, 612);
        g.setColour (juce::Colours::black.withAlpha (0.18f)); g.fillRoundedRectangle (b.translated (5, 6), 10);
        g.setColour (BOARD); g.fillRoundedRectangle (b, 10); g.setColour (BOARD.darker (0.4f)); g.drawRoundedRectangle (b, 10, 2.0f);
        sheet (g, b.reduced (12).withTrimmedTop (26), 77);
        auto clip = juce::Rectangle<float> (b.getCentreX() - 58, b.getY() - 8, 116, 38);
        g.setColour (STEEL.darker (0.3f)); g.fillRoundedRectangle (clip.translated (0, 2), 8); g.setColour (STEEL); g.fillRoundedRectangle (clip, 8);
        g.setColour (juce::Colours::white.withAlpha (0.6f)); g.drawLine (clip.getX() + 10, clip.getY() + 6, clip.getRight() - 10, clip.getY() + 6, 2.0f);
        g.setColour (STEEL.darker (0.5f)); g.fillEllipse (clip.getCentreX() - 6, clip.getY() + 12, 12, 12);
        text (g, "LA CREW", { b.getX(), b.getY() + 40, b.getWidth(), 34 }, beb (34.0f), INK);
        text (g, "come cantano tutti insieme", { b.getX(), b.getY() + 70, b.getWidth(), 14 }, mon (9.0f), INK.withAlpha (0.65f));
    }
    text (g, "jackpot.wav '26", { 760, 694, 130, 16 }, mon (9.5f), PENCIL, juce::Justification::centredRight);
}
void Root::paint (juce::Graphics& g)
{
    const float sc = juce::jlimit (1.0f, 4.0f, g.getInternalContext().getPhysicalPixelScaleFactor());
    if (bg.isNull() || std::abs (bgScale - sc) > 0.01f)
    {
        bgScale = sc; bg = juce::Image (juce::Image::RGB, juce::roundToInt (Editor::W * sc), juce::roundToInt (Editor::H * sc), false);
        juce::Graphics ig (bg); ig.addTransform (juce::AffineTransform::scale (sc)); ed.drawStatic (ig);
    }
    g.drawImageTransformed (bg, juce::AffineTransform::scale (1.0f / sc));
    using H = Editor::Hot; auto R = [this] (int i) { return ed.hotR[i]; };
    // il preset su un post-it
    {
        auto n = juce::Rectangle<float> (390, 16, 430, 64);
        g.setColour (juce::Colours::black.withAlpha (0.15f)); g.fillRect (n.translated (4, 5));
        g.setColour (YEL); g.fillRect (n); tape (g, n.getCentreX(), n.getY() + 2, 0.04f, 80);
        text (g, "PRESET", n.withHeight (16).translated (14, 6), mon (9.0f), INK.withAlpha (0.6f), juce::Justification::centredLeft);
        text (g, "<", R (H::H_PREV), bun (24.0f), hot == H::H_PREV ? RED : INK);
        text (g, ">", R (H::H_NEXT), bun (24.0f), hot == H::H_NEXT ? RED : INK);
        text (g, ed.proc.currentName().toUpperCase(), R (H::H_NAME), beb (34.0f), hot == H::H_NAME ? RED : INK);
    }
    text (g, "SALVA", R (H::H_SAVE), beb (22.0f), hot == H::H_SAVE ? RED : INK);
    text (g, "ANNULLA", R (H::H_UNDO), beb (18.0f), ed.proc.undo.canUndo() ? (hot == H::H_UNDO ? RED : INK) : PENCIL.withAlpha (0.5f));
    text (g, "RIPETI", R (H::H_REDO), beb (18.0f), ed.proc.undo.canRedo() ? (hot == H::H_REDO ? RED : INK) : PENCIL.withAlpha (0.5f));
    for (int i = (int) H::H_SAVE; i <= (int) H::H_REDO; ++i) if (hot == i) { Path u; u.startNewSubPath (R (i).getX() + 6, R (i).getBottom() - 2); u.lineTo (R (i).getRight() - 6, R (i).getBottom() - 3); g.setColour (RED); g.strokePath (wob (u, i, 0.6f), juce::PathStrokeType (2.0f)); }
    {   // ingresso: quanto canti forte
        auto m = juce::Rectangle<float> (922, 668, 220, 10); const float l = juce::jlimit (0.0f, 1.0f, ed.proc.uiIn.load() * 1.4f);
        g.setColour (INK.withAlpha (0.15f)); g.fillRect (m); g.setColour (RED); g.fillRect (m.withWidth (m.getWidth() * l));
        g.setColour (INK); g.drawRect (m, 1.2f); text (g, "LA TUA VOCE", m.translated (0, -16).withHeight (14), mon (9.0f), INK.withAlpha (0.7f), juce::Justification::centredLeft);
    }
}
void Root::mouseMove (const juce::MouseEvent& e)
{
    int h = -1; for (int i = 0; i < Editor::H_COUNT; ++i) if (ed.hotR[i].contains (e.position)) h = i;
    if (h != hot) { hot = h; setMouseCursor (h >= 0 ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor); repaint(); }
}
void Root::mouseDown (const juce::MouseEvent& e) { for (int i = 0; i < Editor::H_COUNT; ++i) if (ed.hotR[i].contains (e.position)) { ed.click (i); return; } }

// =============================================================================================== editor
Editor::Editor (Processor& p) : juce::AudioProcessorEditor (p), proc (p)
{
    hotR[H_PREV] = { 400, 34, 34, 40 }; hotR[H_NEXT] = { 776, 34, 34, 40 }; hotR[H_NAME] = { 436, 30, 338, 46 };
    hotR[H_SAVE] = { 836, 46, 70, 30 }; hotR[H_UNDO] = { 916, 50, 84, 26 }; hotR[H_REDO] = { 1004, 50, 84, 26 };
    addAndMakeVisible (root);
    auto pct = [] (float v) { return juce::String (juce::roundToInt (v * 100)) + "%"; };
    auto db = [] (float v) { return v <= -59.9f ? juce::String ("MUTO") : (v > 0 ? "+" : "") + juce::String (v, 1) + " dB"; };
    auto pan = [] (float v) { const int q = juce::roundToInt (v * 100); return q == 0 ? juce::String ("C") : (q < 0 ? "L" : "R") + juce::String (std::abs (q)); };
    auto st = [] (float v) { const int q = juce::roundToInt (v); return q == 0 ? juce::String ("UNISONO") : (q > 0 ? "+" : "") + juce::String (q) + " st"; };
    auto tone = [] (float v) { return std::abs (v) < 0.05f ? juce::String ("NEUTRO") : v < 0 ? "SCURO " + juce::String (juce::roundToInt (-v * 100)) : "CHIARO " + juce::String (juce::roundToInt (v * 100)); };
    for (int k = 0; k < kVoices; ++k)
    {
        const float cx = 24 + (k % 3) * 292.0f, cy = 100 + (k / 3) * 304.0f;
        auto c = std::make_unique<Card> (*this, k); c->setBounds (juce::Rectangle<float> (cx, cy, 280, 296).toNearestInt());
        c->setTooltip (juce::String (characters()[(size_t) k].name) + ": " + characters()[(size_t) k].role + " (clicca il personaggio per accenderlo)");
        root.addAndMakeVisible (*c); cards.push_back (std::move (c));
        struct D { const char* w; const char* l; std::function<juce::String (float)> f; int steps; bool bip; };
        const D d[] = { { "lvl", "VOLUME", db, 0, false }, { "pan", "POSTO", pan, 0, true }, { "pitch", "ALTEZZA", st, 25, true }, { "tone", "COLORE", tone, 0, true } };
        for (int j = 0; j < 4; ++j)
        {
            auto kn = std::make_unique<Knob> (*this, Processor::vid (k, d[j].w), d[j].l, sk::accent (k), d[j].f, k * 10 + j);
            kn->steps = d[j].steps; kn->bipolar = d[j].bip;
            kn->setBounds (juce::Rectangle<float> (cx + 8 + j * 66.0f, cy + 214, 66, 78).toNearestInt());
            root.addAndMakeVisible (*kn); knobs.push_back (std::move (kn));
        }
    }
    struct G { const char* id; const char* l; std::function<juce::String (float)> f; const char* tip; };
    const G gl[] = {
        { "loose", "SCIOLTI", pct, "Quanto la crew entra in ritardo e respira: mai tutti perfettamente a tempo" },
        { "drift", "STONATI", pct, "Quanto ognuno stona di qualche cent: piu' alto = piu' gente vera" },
        { "spread", "LARGHEZZA", pct, "Quanto la crew si allarga nello stereo" },
        { "shout", "GRIDATO", pct, "Compressione e saturazione: da cantato a urlato" },
        { "room", "STANZA", pct, "La stanza in cui grida la crew" },
        { "lowcut", "TAGLIO BASSI", [] (float v) { return juce::String (juce::roundToInt (v)) + " Hz"; }, "Taglia i bassi della crew per non impastare la voce principale" },
        { "crew", "CREW", db, "Volume di tutta la crew" },
        { "dry", "TU", db, "Volume della tua voce (la principale)" },
    };
    for (int j = 0; j < 8; ++j)
    {
        auto kn = std::make_unique<Knob> (*this, gl[j].id, gl[j].l, j < 5 ? RED : juce::Colour (0xff4f7fd0), gl[j].f, 200 + j);
        kn->setTooltip (gl[j].tip);
        kn->setBounds (juce::Rectangle<float> (930 + (j % 2) * 104.0f, 186 + (j / 2) * 116.0f, 100, 108).toNearestInt());
        root.addAndMakeVisible (*kn); knobs.push_back (std::move (kn));
    }
    root.addAndMakeVisible (sizeChip);
    setWantsKeyboardFocus (true);
    scaler.setup();
    startTimerHz (30);
}
void Editor::resized()
{
    const float s = scaler.update();
    root.setBounds (0, 0, W, H); root.setTransform (juce::AffineTransform::scale (s));
    sizeChip.setBounds (1100, 6, 58, 15);
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
        case H_NAME: presetMenu(); break;
        case H_SAVE: askSave(); break;
        case H_UNDO: proc.undo.undo(); break;
        case H_REDO: proc.undo.redo(); break;
        default: break;
    }
    root.repaint(); for (auto& c : cards) c->repaint();
}
void Editor::presetMenu()
{
    juce::PopupMenu m; juce::StringArray cats;
    for (auto& p : presets()) if (! cats.contains (p.cat)) cats.add (p.cat);
    for (auto& c : cats)
    {
        juce::PopupMenu sub;
        for (int i = 0; i < (int) presets().size(); ++i) if (presets()[(size_t) i].cat == c) sub.addItem (i + 1, presets()[(size_t) i].name, true, i == proc.preset && proc.userName.isEmpty());
        m.addSubMenu (c, sub);
    }
    auto files = Processor::presetFolder().findChildFiles (juce::File::findFiles, false, "*.crew"); files.sort();
    if (! files.isEmpty()) { juce::PopupMenu u; for (int i = 0; i < files.size(); ++i) u.addItem (500 + i, files[i].getFileNameWithoutExtension()); m.addSubMenu ("I MIEI", u); }
    m.addSeparator(); m.addItem (900, "Salva il preset...");
    juce::Component::SafePointer<Editor> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&root), [safe, files] (int r)
    {
        if (safe == nullptr || r <= 0) return;
        if (r == 900) safe->askSave(); else if (r >= 500 && r < 900) safe->proc.loadUserPreset (files[r - 500]); else safe->proc.loadPreset (r - 1);
        safe->root.repaint(); for (auto& c : safe->cards) c->repaint();
    });
}
void Editor::askSave()
{
    auto* w = new juce::AlertWindow ("SALVA IL PRESET", "Nome del preset", juce::MessageBoxIconType::NoIcon, this);
    w->addTextEditor ("n", proc.currentName()); w->addButton ("SALVA", 1, juce::KeyPress (juce::KeyPress::returnKey)); w->addButton ("ANNULLA", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    juce::Component::SafePointer<Editor> safe (this);
    w->enterModalState (true, juce::ModalCallbackFunction::create ([safe, w] (int r)
    { if (safe != nullptr && r == 1) { const auto n = w->getTextEditorContents ("n").trim(); if (n.isNotEmpty()) safe->proc.saveUserPreset (n); safe->root.repaint(); } }), true);
}
void Editor::timerCallback()
{
    ++tick;
    for (auto& c : cards)
    {
        const bool on = proc.value (Processor::vid (c->idx, "on")) > 0.5f;
        const float target = on ? juce::jlimit (0.0f, 1.0f, proc.uiMeter[(size_t) c->idx].load() * 3.0f) : 0.0f;
        c->mouth += (target - c->mouth) * (target > c->mouth ? 0.6f : 0.25f);
        if (on != c->lastOn) { c->lastOn = on; c->repaint(); }
        else if (std::abs (c->mouth - c->lastMouth) > 0.02f || (on && c->mouth > 0.25f && tick % 2 == 0)) { c->lastMouth = c->mouth; c->repaint (juce::Rectangle<int> (0, 0, c->getWidth(), 178)); }
    }
    if (proc.preset != lastPreset || proc.userName != lastUser) { lastPreset = proc.preset; lastUser = proc.userName; root.repaint(); }
    if (tick % 3 == 0) root.repaint (juce::Rectangle<int> (918, 648, 230, 34));
    if (tick % 8 == 0) root.repaint (hotR[H_UNDO].getUnion (hotR[H_REDO]).toNearestInt());
    for (auto& k : knobs) { const float v = k->prm->getValue(); if (v != k->last) { k->last = v; k->repaint(); } }
}
}
