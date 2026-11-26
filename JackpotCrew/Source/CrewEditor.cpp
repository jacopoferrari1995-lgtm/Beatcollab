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
void flat (juce::Graphics& g, const Path& p, juce::Colour c, bool on) { if (! on) return; g.setColour (c.darker (0.18f)); g.fillPath (p, juce::AffineTransform::translation (3.0f, 2.0f)); }
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
const juce::Colour LITE (0xfff1ece2);
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
            both (g, poly ({ { 124, 134 }, { 156, 134 }, { 140, 168 } }), LITE, on, s + 1, 1.8f);
            ink (g, poly ({ { 112, 136 }, { 140, 172 }, { 168, 136 } }), s + 2, 2.0f, on);
            both (g, neck, skin.darker (0.15f), on, s + 3);
            both (g, ell (94, 70, 12, 20), skin, on, s + 4, 1.6f); both (g, ell (174, 70, 12, 20), skin, on, s + 5, 1.6f);
            both (g, rr (102, 36, 76, 90, 28), skin, on, s + 6);
            both (g, rr (126, 20, 28, 30, 10), DARKF, on, s + 7);
            both (g, poly ({ { 100, 70 }, { 180, 70 }, { 176, 86 }, { 148, 89 }, { 140, 82 }, { 132, 89 }, { 104, 86 } }), DARKF, on, s + 8);
            if (on) { g.setColour (LITE.withAlpha (0.8f)); g.drawLine (110, 74, 126, 74, 2.0f); }
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
            both (g, rr (104, 68, 72, 22, 4), LITE, on, s + 6, 1.8f);
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
            both (g, ell (112, 70, 24, 24), DARKF, on, s + 5, 2.0f); both (g, ell (144, 70, 24, 24), DARKF, on, s + 6, 2.0f);
            if (on) { g.setColour (juce::Colour (0xffe8b83a)); g.drawEllipse (112, 70, 24, 24, 1.4f); g.drawEllipse (144, 70, 24, 24, 1.4f); g.setColour (LITE.withAlpha (0.8f)); g.drawLine (118, 76, 124, 74, 1.6f); }
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
        if (on) { g.setColour (LITE.withAlpha (0.7f)); g.drawLine (112, 58, 126, 56, 2.5f); }
    }
    else if (k == 4)
    {   // bandana: la stoffa si gonfia
        Path b; const float bulge = 6 + open * 10;
        b.startNewSubPath (104, 84); b.lineTo (176, 84); b.cubicTo (176, 100, 160, 118 + bulge * 0.4f, 140, 128 + bulge * 0.3f); b.cubicTo (120, 118 + bulge * 0.4f, 104, 100, 104, 84); b.closeSubPath();
        both (g, b, juce::Colour (0xff3567b5), on, 400 + tk / 6 % 3);
        if (on) { g.setColour (LITE); for (int i = 0; i < 9; ++i) g.fillEllipse (112.0f + (i % 5) * 12, 90.0f + (i / 5) * 12 + (i % 2) * 3, 3, 3); }
    }
    else
    {
        if (open < 0.08f || ! on) { Path l; l.startNewSubPath (m.x - 9, m.y); l.quadraticTo (m.x, m.y + 2, m.x + 9, m.y); ink (g, l, 300 + k, 2.0f, on); }
        else
        {
            const float w = 14 + 10 * open + (k == 1 ? 6 : 0), h = 4 + 16 * open;
            Path mo; mo.addEllipse (m.x - w * 0.5f, m.y - h * 0.3f, w, h);
            g.setColour (DARKF); g.fillPath (mo);
            g.saveState(); g.reduceClipRegion (mo);
            g.setColour (k == 5 ? juce::Colour (0xffe8b83a) : LITE); g.fillRect (m.x - w * 0.5f, m.y - h * 0.3f, w, juce::jmin (h * 0.3f, 5.0f));
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

juce::Colour sk::holoTint (int k)
{
    static const juce::Colour c[] = { juce::Colour (0xffffb347), juce::Colour (0xff5dff8a), juce::Colour (0xffff7a3d), juce::Colour (0xff38f2ff), juce::Colour (0xff6f9bff), juce::Colour (0xffffd84a) };
    return c[juce::jlimit (0, kVoices - 1, k)];
}
// posto di ogni ologramma nella stanza: tre a sinistra, tre a destra, quelli in mezzo piu' vicini al centro
juce::Rectangle<float> Editor::holoSlot (int k)
{
    static const int side[] = { 0, 1, 1, 0, 0, 1 }, row[] = { 0, 0, 1, 1, 2, 2 };
    const float y = 66.0f + row[k] * 212.0f, inset = row[k] == 1 ? 50.0f : 12.0f;
    return { side[k] == 0 ? inset : (float) W - inset - 374, y, 374, 206 };
}
namespace
{
bool leftSide (int k) { return k == 0 || k == 3 || k == 4; }
void glowText (juce::Graphics& g, const juce::String& t, juce::Rectangle<float> r, juce::Font f, juce::Colour c, float amt, juce::Justification j = juce::Justification::centred)
{
    g.setFont (f);
    for (int i = 4; i >= 1; --i) { g.setColour (c.withAlpha (0.07f * amt)); for (auto o : { juce::Point<float> (-1, 0), juce::Point<float> (1, 0), juce::Point<float> (0, -1), juce::Point<float> (0, 1) }) g.drawText (t, r.translated (o.x * i * 1.6f, o.y * i * 1.6f), j, false); }
    g.setColour (c.withAlpha (amt)); g.drawText (t, r, j, false);
    g.setColour (juce::Colours::white.withAlpha (0.55f * amt)); g.drawText (t, r.translated (0, -0.5f), j, false);
}
void bracket (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c, float len = 14, float w = 2)
{
    g.setColour (c);
    g.fillRect (r.getX(), r.getY(), len, w); g.fillRect (r.getX(), r.getY(), w, len);
    g.fillRect (r.getRight() - len, r.getY(), len, w); g.fillRect (r.getRight() - w, r.getY(), w, len);
    g.fillRect (r.getX(), r.getBottom() - w, len, w); g.fillRect (r.getX(), r.getBottom() - len, w, len);
    g.fillRect (r.getRight() - len, r.getBottom() - w, len, w); g.fillRect (r.getRight() - w, r.getBottom() - len, w, len);
}
void grime (juce::Graphics& g, juce::Rectangle<float> r, int seed, float amt)
{   // sporco: puntini, graffi, colature
    juce::Random rnd (seed);
    for (int i = 0; i < (int) (r.getWidth() * r.getHeight() / 90); ++i) { g.setColour ((rnd.nextBool() ? juce::Colours::black : juce::Colours::white).withAlpha (0.05f * amt + rnd.nextFloat() * 0.04f * amt)); g.fillRect (r.getX() + rnd.nextFloat() * r.getWidth(), r.getY() + rnd.nextFloat() * r.getHeight(), 1.3f, 1.3f); }
    for (int i = 0; i < 26; ++i)
    {
        const float x = r.getX() + rnd.nextFloat() * r.getWidth(), y = r.getY() + rnd.nextFloat() * r.getHeight(), l = 8 + rnd.nextFloat() * 60, a = rnd.nextFloat() * 3.14f;
        g.setColour (juce::Colours::white.withAlpha (0.04f * amt)); g.drawLine (x, y, x + std::cos (a) * l, y + std::sin (a) * l, 0.7f);
    }
    for (int i = 0; i < 10; ++i)
    {
        const float x = r.getX() + rnd.nextFloat() * r.getWidth(), y = r.getY() + rnd.nextFloat() * r.getHeight() * 0.4f, l = 30 + rnd.nextFloat() * 120;
        juce::ColourGradient cg (juce::Colours::black.withAlpha (0.28f * amt), x, y, juce::Colours::transparentBlack, x, y + l, false); g.setGradientFill (cg); g.fillRect (x, y, 1.5f + rnd.nextFloat() * 2.5f, l);
    }
}
}

// =============================================================================================== carta / proiezione del personaggio
void Card::paint (juce::Graphics& g)
{
    const bool on = ed.proc.value (Processor::vid (idx, "on")) > 0.5f;
    if (ed.holoTheme()) paintHolo (g, on); else paintSketch (g, on);
}
void Card::paintSketch (juce::Graphics& g, bool on)
{
    const float sc = juce::jlimit (1.0f, 4.0f, g.getInternalContext().getPhysicalPixelScaleFactor());
    if (cache.isNull() || std::abs (cacheScale - sc) > 0.01f || cacheKey != (int) on)
    {
        cacheScale = sc; cacheKey = on;
        cache = juce::Image (juce::Image::ARGB, juce::roundToInt (getWidth() * sc), juce::roundToInt (getHeight() * sc), true);
        juce::Graphics ig (cache); ig.addTransform (juce::AffineTransform::scale (sc));
        sheet (ig, getLocalBounds().toFloat().reduced (4), 50 + idx * 17);
        tape (ig, getWidth() * 0.5f, 6, (idx % 2 ? 0.05f : -0.06f));
        if (on) { ig.setColour (accent (idx).withAlpha (0.16f)); ig.fillRect (juce::Rectangle<float> (60, 24, 160, 124)); }
        { ig.saveState(); ig.addTransform (juce::AffineTransform::scale (0.8f).translated (28, 6)); Editor::drawCharacter (ig, idx, { 0, 0, 280, 176 }, on); ig.restoreState(); }
        const auto& c = characters()[(size_t) idx];
        if (on) { ig.setColour (accent (idx).darker (0.2f)); ig.fillRect (juce::Rectangle<float> (16, 150, 116, 26).translated (3, 2)); }
        text (ig, c.name, { 16, 146, 160, 32 }, beb (30.0f), on ? INK : PENCIL, juce::Justification::centredLeft);
        ig.setFont (mon (8.5f)); ig.setColour (on ? INK.withAlpha (0.75f) : PENCIL);
        ig.drawFittedText (c.role, juce::Rectangle<int> (140, 150, getWidth() - 152, 26), juce::Justification::centredRight, 2, 0.8f);
        if (! on) text (ig, "clicca per farlo entrare", { 0, 124, (float) getWidth(), 18 }, mon (10.0f), PENCIL);
        grime (ig, getLocalBounds().toFloat().reduced (4), 900 + idx, 1.3f);
    }
    g.drawImageTransformed (cache, juce::AffineTransform::scale (1.0f / sc));
    { g.saveState(); g.addTransform (juce::AffineTransform::scale (0.8f).translated (28, 6)); Editor::drawMouth (g, idx, { 0, 0, 280, 176 }, on, mouth, ed.tick); g.restoreState(); }
    if (hover) { g.setColour (RED.withAlpha (0.7f)); g.drawRect (getLocalBounds().toFloat().reduced (6), 1.5f); }
}
void Card::paintHolo (juce::Graphics& g, bool on)
{
    const float w = (float) getWidth(), h = (float) getHeight(); const auto tint = holoTint (idx);
    const float sc = juce::jlimit (1.0f, 4.0f, g.getInternalContext().getPhysicalPixelScaleFactor());
    const auto fig = juce::Rectangle<float> (0, 32, w, 150);
    if (holo.isNull() || std::abs (holoScale - sc) > 0.01f)
    {   // il personaggio disegnato e poi trasformato in luce: i tratti brillano, i colori diventano velo, righe di scansione e alone
        holoScale = sc;
        const int iw = juce::roundToInt (fig.getWidth() * sc), ih = juce::roundToInt (fig.getHeight() * sc);
        juce::Image src (juce::Image::ARGB, iw, ih, true);
        { juce::Graphics ig (src); const float s0 = fig.getWidth() / 220.0f; ig.addTransform (juce::AffineTransform::translation (-30, 0).scaled (s0 * sc)); Editor::drawCharacter (ig, idx, { 0, 0, 280, 176 }, true); }
        {
            juce::Image::BitmapData bd (src, juce::Image::BitmapData::readWrite);
            for (int y = 0; y < ih; ++y)
            {
                const float scan = (y % 3 == 0) ? 0.45f : 1.0f;
                for (int x = 0; x < iw; ++x)
                {
                    const auto c = bd.getPixelColour (x, y); const float a = c.getFloatAlpha(); if (a <= 0.001f) continue;
                    const float lum = (0.3f * c.getFloatRed() + 0.59f * c.getFloatGreen() + 0.11f * c.getFloatBlue());
                    const float out = a * (0.16f + 0.84f * std::pow (lum, 1.4f)) * scan;
                    bd.setPixelColour (x, y, tint.interpolatedWith (juce::Colours::white, std::pow (lum, 3.0f) * 0.55f).withAlpha (juce::jlimit (0.0f, 1.0f, out)));
                }
            }
        }
        juce::Image glow = src.createCopy();
        juce::ImageConvolutionKernel k (9); k.createGaussianBlur (3.5f * sc / 2.0f + 1.5f); k.applyToImage (glow, glow, glow.getBounds());
        holo = juce::Image (juce::Image::ARGB, iw, ih, true);
        { juce::Graphics hg (holo); hg.setOpacity (0.9f); hg.drawImageAt (glow, 0, 0); hg.setOpacity (1.0f); hg.drawImageAt (src, 0, 0); }
    }
    // proiettore
    const float pcx = w * 0.5f, pcy = h - 18;
    {
        g.setColour (juce::Colours::black.withAlpha (0.6f)); g.fillEllipse (pcx - 80, pcy - 8, 160, 30);
        juce::ColourGradient mg (juce::Colour (0xff2b2e3a), pcx, pcy - 12, juce::Colour (0xff0b0c12), pcx, pcy + 14, false); g.setGradientFill (mg); g.fillEllipse (pcx - 74, pcy - 13, 148, 26);
        g.setColour (juce::Colour (0xff3c4050)); g.drawEllipse (pcx - 74, pcy - 13, 148, 26, 1.5f);
        g.setColour ((on ? tint : juce::Colour (0xffff3a3a)).withAlpha (on ? 0.85f : 0.35f)); g.drawEllipse (pcx - 52, pcy - 8, 104, 16, 2.0f);
        if (on)
        {
            const float pulse = 0.35f + 0.65f * mouth;
            juce::ColourGradient beam (tint.withAlpha (0.18f + 0.15f * pulse), pcx, pcy, tint.withAlpha (0.0f), pcx, fig.getY(), false);
            g.setGradientFill (beam); juce::Path b; b.startNewSubPath (pcx - 50, pcy); b.lineTo (pcx + 50, pcy); b.lineTo (pcx + 86, fig.getY() + 10); b.lineTo (pcx - 86, fig.getY() + 10); b.closeSubPath(); g.fillPath (b);
            // anello che pulsa con la voce
            for (int i = 0; i < 24; ++i) { const float a = i / 24.0f * juce::MathConstants<float>::twoPi, l = 3 + 10 * mouth * (0.5f + 0.5f * std::sin (a * 3 + ed.tick * 0.4f)); g.setColour (tint.withAlpha (0.8f)); g.drawLine (pcx + std::cos (a) * 52, pcy + std::sin (a) * 8, pcx + std::cos (a) * (52 + l), pcy + std::sin (a) * (8 + l * 0.15f), 1.6f); }
        }
    }
    // figura
    {
        float alpha = on ? (0.78f + 0.22f * mouth) * (glitch > 0.5f ? 0.6f : 1.0f) : 0.07f;
        if (on && ed.tick % 47 == idx * 7) alpha *= 0.7f;
        if (glitch > 0.01f && on)
        {   // sfarfallio: fette orizzontali spostate
            juce::Random r (glitchSeed);
            const int slices = 5;
            for (int s = 0; s < slices; ++s)
            {
                const float y0 = fig.getY() + fig.getHeight() * s / slices, hh = fig.getHeight() / slices, dx = (r.nextFloat() - 0.5f) * 14 * glitch;
                g.saveState(); g.reduceClipRegion (juce::Rectangle<float> (0, y0, w, hh).toNearestInt()); g.setOpacity (alpha);
                g.drawImageTransformed (holo, juce::AffineTransform::scale (1.0f / holoScale).translated (fig.getX() + dx, fig.getY())); g.restoreState();
            }
        }
        else { g.setOpacity (alpha); g.drawImageTransformed (holo, juce::AffineTransform::scale (1.0f / holoScale).translated (fig.getX(), fig.getY())); g.setOpacity (1.0f); }
        if (! on)
        {   // proiettore spento: solo rumore
            juce::Random r (ed.tick / 3 + idx * 31);
            for (int i = 0; i < 16; ++i) { g.setColour (juce::Colours::white.withAlpha (0.05f + r.nextFloat() * 0.07f)); g.fillRect (r.nextFloat() * w, fig.getY() + r.nextFloat() * fig.getHeight(), 6 + r.nextFloat() * 40, 1.0f); }
            glowText (g, "OFFLINE", { 0, pcy - 52, w, 20 }, mon (12.0f), juce::Colour (0xffff3a3a), 0.8f);
            text (g, "clicca per proiettarlo", { 0, pcy - 34, w, 14 }, mon (9.0f), juce::Colour (0xff8a8fa3));
        }
        else if (mouth > 0.2f)
        {   // segni del suono
            const float mx = fig.getX() + (140 - 30) * fig.getWidth() / 220.0f, my = fig.getY() + 106 * fig.getWidth() / 220.0f;
            for (int i = 0; i < 3; ++i) { juce::Path a; const float r = 10 + i * 8 + (ed.tick % 6); a.addCentredArc (mx + 24, my, r, r, 0, 0.9f, 2.2f, true); g.setColour (tint.withAlpha (0.8f - i * 0.22f)); g.strokePath (a, juce::PathStrokeType (1.8f)); }
            g.setColour (tint.withAlpha (0.9f)); g.fillEllipse (mx - 4 - 5 * mouth, my - 2, 8 + 10 * mouth, 3 + 9 * mouth);
        }
    }
    // nome e ruolo
    const auto& c = characters()[(size_t) idx];
    const bool L = leftSide (idx);
    glowText (g, c.name, { 6, 2, w - 12, 26 }, beb (28.0f), on ? tint : juce::Colour (0xff4b4f60), on ? 1.0f : 0.6f, L ? juce::Justification::centredLeft : juce::Justification::centredRight);
    text (g, c.role, { 6, 24, w - 12, 12 }, mon (8.5f), on ? tint.withAlpha (0.7f) : juce::Colour (0xff4b4f60), L ? juce::Justification::centredLeft : juce::Justification::centredRight);
    if (hover) bracket (g, getLocalBounds().toFloat().reduced (2), tint.withAlpha (0.9f), 12, 1.5f);
}
void Card::mouseDown (const juce::MouseEvent& e)
{
    if (! ed.holoTheme() && e.position.y > 146) return;
    ed.proc.undo.beginNewTransaction();
    const auto id = Processor::vid (idx, "on");
    ed.proc.setP (id, ed.proc.value (id) > 0.5f ? 0.0f : 1.0f);
    glitch = 1.0f; glitchSeed = ed.tick;
    repaint();
}

// =============================================================================================== manopola (gesso o metallo con neon)
Knob::Knob (Editor& e, const juce::String& id, juce::String l, juce::Colour f, std::function<juce::String (float)> fm, int sd)
    : ed (e), prm (e.P (id)), label (std::move (l)), fill (f), fmt (std::move (fm)), seed (sd)
{ setRepaintsOnMouseActivity (true); setMouseCursor (juce::MouseCursor::UpDownResizeCursor); }
void Knob::paint (juce::Graphics& g)
{
    const bool holoT = ed.holoTheme();
    const bool live = voice < 0 || ed.proc.value (Processor::vid (voice, "on")) > 0.5f;
    const bool small = getHeight() < 70 || getWidth() < 62;
    const float w = (float) getWidth(), h = (float) getHeight(), lh = holoT ? 13.0f : (small ? 13.0f : 17.0f), vh = small ? 12.0f : 14.0f;
    const float S = juce::jmin (w - 20, h - lh - vh - (small ? 6.0f : 10.0f)), cx = w * 0.5f, cy = lh + (small ? 3.0f : 5.0f) + S * 0.5f, R = S * 0.5f;
    const float v = prm->getValue(), a0 = -2.36f, a1 = 2.36f, av = a0 + (a1 - a0) * v;
    const juce::String t = fmt ? fmt (prm->convertFrom0to1 (v)) : prm->getCurrentValueAsText();
    const bool hov = isMouseOverOrDragging();
    if (holoT)
    {
        const auto c = live ? fill : juce::Colour (0xff4b4f60);
        text (g, label, { -10, 0, w + 20, lh }, mon (w < 60 ? 8.0f : 9.0f), c.withAlpha (0.9f));
        for (int i = 0; i <= 20; ++i)
        {
            const float a = a0 + (a1 - a0) * i / 20.0f, r1 = R + 4, r2 = R + (i % 5 == 0 ? 9.0f : 6.5f); const bool lit = live && i / 20.0f <= v + 1e-4f;
            g.setColour (lit ? c : juce::Colour (0xff2c3040)); g.drawLine (cx + std::sin (a) * r1, cy - std::cos (a) * r1, cx + std::sin (a) * r2, cy - std::cos (a) * r2, i % 5 == 0 ? 2.0f : 1.4f);
        }
        if (live) { juce::Path p; const float s0 = bipolar ? 0.0f : a0; p.addCentredArc (cx, cy, R + 2, R + 2, 0, juce::jmin (s0, av), juce::jmax (s0, av), true); g.setColour (c.withAlpha (hov ? 0.5f : 0.3f)); g.strokePath (p, juce::PathStrokeType (5.0f)); g.setColour (c); g.strokePath (p, juce::PathStrokeType (1.6f)); }
        g.setColour (juce::Colours::black.withAlpha (0.6f)); g.fillEllipse (cx - R + 2, cy - R + 4, R * 2, R * 2);
        { juce::ColourGradient body (juce::Colour (0xff3a3e4c), cx, cy - R, juce::Colour (0xff0e1017), cx, cy + R, false); g.setGradientFill (body); g.fillEllipse (cx - R, cy - R, R * 2, R * 2); }
        for (int i = 0; i < 36; ++i) { const float a = av + i * juce::MathConstants<float>::twoPi / 36; g.setColour (juce::Colours::black.withAlpha (0.35f)); g.drawLine (cx + std::sin (a) * (R - 0.5f), cy - std::cos (a) * (R - 0.5f), cx + std::sin (a) * (R - 3.5f), cy - std::cos (a) * (R - 3.5f), 1.0f); }
        const float rc = R * 0.66f; { juce::ColourGradient cap (juce::Colour (0xff2a2d38), cx - rc * 0.4f, cy - rc, juce::Colour (0xff0a0b10), cx + rc * 0.4f, cy + rc, false); g.setGradientFill (cap); g.fillEllipse (cx - rc, cy - rc, rc * 2, rc * 2); }
        g.setColour (juce::Colours::white.withAlpha (0.12f)); g.drawEllipse (cx - rc, cy - rc, rc * 2, rc * 2, 1.0f);
        { const auto p0 = juce::Point<float> (cx + std::sin (av) * rc * 0.2f, cy - std::cos (av) * rc * 0.2f), p1 = juce::Point<float> (cx + std::sin (av) * (R - 2), cy - std::cos (av) * (R - 2));
          g.setColour (c.withAlpha (0.35f)); g.drawLine ({ p0, p1 }, 5.0f); g.setColour (live ? juce::Colours::white.interpolatedWith (c, 0.4f) : c); g.drawLine ({ p0, p1 }, 2.0f); }
        text (g, t, { -10, h - vh, w + 20, vh }, mon (w < 60 ? 8.5f : 9.5f), hov ? juce::Colours::white : (live ? c.interpolatedWith (juce::Colours::white, 0.35f) : c));
        return;
    }
    text (g, label, { -10, 0, w + 20, lh }, beb (small ? 14.0f : 17.0f), INK);
    for (int i = 0; i <= 10; ++i)
    {
        const float a = a0 + (a1 - a0) * i / 10.0f, r1 = R + 3, r2 = R + (i % 5 == 0 ? 8.0f : 6.0f);
        g.setColour (PENCIL); g.drawLine (cx + std::sin (a) * r1, cy - std::cos (a) * r1, cx + std::sin (a) * r2, cy - std::cos (a) * r2, i % 5 == 0 ? 1.6f : 1.0f);
    }
    {
        const float s0 = bipolar ? 0.0f : a0; Path p; p.addCentredArc (cx, cy, R + 4.5f, R + 4.5f, 0, juce::jmin (s0, av), juce::jmax (s0, av), true);
        g.setColour (fill.withAlpha (hov ? 0.95f : 0.75f)); g.strokePath (p, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    Path body; body.addEllipse (cx - R, cy - R, R * 2, R * 2);
    g.setColour (fill.darker (0.35f)); g.fillPath (body, juce::AffineTransform::translation (2.5f, 2.0f));
    g.setColour (INK.withAlpha (0.25f)); g.fillEllipse (cx - R * 0.55f, cy - R * 0.7f, R * 0.6f, R * 0.35f);
    ink (g, body, seed, 2.0f);
    Path ptr; ptr.startNewSubPath (cx + std::sin (av) * R * 0.2f, cy - std::cos (av) * R * 0.2f); ptr.lineTo (cx + std::sin (av) * (R + 1), cy - std::cos (av) * (R + 1));
    g.setColour (INK); g.strokePath (ptr, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    text (g, t, { -10, h - vh, w + 20, vh }, mon (small ? 9.0f : 10.5f), hov ? RED : INK);
    if (! live) { g.setColour (PAPER.withAlpha (0.45f)); g.fillRect (getLocalBounds()); }
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

// =============================================================================================== quaderno sporco
void Editor::drawSketchStatic (juce::Graphics& g)
{
    g.fillAll (PAPER);
    {
        juce::Random r (5);
        for (int i = 0; i < 14000; ++i) { g.setColour ((r.nextBool() ? juce::Colours::black : juce::Colours::white).withAlpha (0.05f)); g.fillRect (r.nextFloat() * W, r.nextFloat() * H, 1.2f, 1.2f); }
        g.setColour (BLUE.withAlpha (0.08f)); for (float x = 12; x < W; x += 24) g.drawVerticalLine ((int) x, 0, (float) H); for (float y = 12; y < H; y += 24) g.drawHorizontalLine ((int) y, 0, (float) H > 0 ? (float) W : 0.0f);
        g.setColour (RED.withAlpha (0.4f)); g.drawVerticalLine (14, 0, (float) H);
    }
    for (int k = 0; k < 3; ++k) { g.setColour (juce::Colour (0xff6b4a2b).withAlpha (0.14f + k * 0.04f)); g.drawEllipse (840.0f - k, 640.0f + k * 0.5f, 120.0f + k * 2, 112.0f, 3.0f - k); }
    grime (g, { 0, 0, (float) W, (float) H }, 3, 1.6f);
    // titolo: neon fuori registro
    glowText (g, "CREW", { 28, 13, 300, 70 }, bun (64.0f), BLUE, 0.7f, juce::Justification::centredLeft);
    glowText (g, "CREW", { 22, 9, 300, 70 }, bun (64.0f), RED, 1.0f, juce::Justification::centredLeft);
    text (g, "JACKPOT CYBERNET  //  GANG VOCALS DA UNA VOCE", { 26, 74, 420, 16 }, mon (10.0f), INK.withAlpha (0.7f), juce::Justification::centredLeft);
    {
        g.setColour (PENCIL);
        Path star; star.addStar ({ 352, 44 }, 5, 6, 15, 0.2f); g.strokePath (wob (star, 3, 0.8f), juce::PathStrokeType (1.6f));
    }
    {   // la cartellina
        auto b = juce::Rectangle<float> (904, 96, 256, 612);
        g.setColour (juce::Colours::black.withAlpha (0.4f)); g.fillRoundedRectangle (b.translated (5, 6), 10);
        g.setColour (BOARD); g.fillRoundedRectangle (b, 10); g.setColour (BOARD.darker (0.5f)); g.drawRoundedRectangle (b, 10, 2.0f);
        sheet (g, b.reduced (12).withTrimmedTop (26), 77);
        auto clip = juce::Rectangle<float> (b.getCentreX() - 58, b.getY() - 8, 116, 38);
        g.setColour (STEEL.darker (0.5f)); g.fillRoundedRectangle (clip.translated (0, 2), 8); g.setColour (STEEL.darker (0.2f)); g.fillRoundedRectangle (clip, 8);
        g.setColour (STEEL.darker (0.7f)); g.fillEllipse (clip.getCentreX() - 6, clip.getY() + 12, 12, 12);
        text (g, "LA CREW", { b.getX(), b.getY() + 34, b.getWidth(), 34 }, beb (32.0f), INK);
        grime (g, b, 11, 1.4f);
    }
    text (g, "jackpot.wav '26", { 760, 694, 130, 16 }, mon (9.5f), PENCIL, juce::Justification::centredRight);
}

// =============================================================================================== la stanza degli ologrammi
void Editor::drawMain (juce::Graphics& g, float lvl)
{
    juce::ignoreUnused (lvl);
    const float cx = 590;
    // tu: cappuccio scuro, visiera, respiratore, microfono in cuffia, cuciture che brillano; luce magenta a sinistra e ciano a destra
    Path hood; hood.startNewSubPath (cx - 168, 532); hood.cubicTo (cx - 160, 440, cx - 120, 392, cx - 96, 370); hood.cubicTo (cx - 118, 300, cx - 104, 210, cx - 58, 176);
    hood.cubicTo (cx - 30, 156, cx + 30, 156, cx + 58, 176); hood.cubicTo (cx + 104, 210, cx + 118, 300, cx + 96, 370); hood.cubicTo (cx + 120, 392, cx + 160, 440, cx + 168, 532); hood.closeSubPath();
    g.setColour (juce::Colours::black.withAlpha (0.5f)); g.fillPath (hood, juce::AffineTransform::translation (0, 8));
    { juce::ColourGradient hg (juce::Colour (0xff1c1f2c), cx, 170, juce::Colour (0xff07080d), cx, 532, false); g.setGradientFill (hg); g.fillPath (hood); }
    g.saveState(); g.reduceClipRegion (hood);
    { juce::ColourGradient rl (MAG.withAlpha (0.35f), cx - 170, 300, MAG.withAlpha (0.0f), cx - 90, 300, false); g.setGradientFill (rl); g.fillRect (cx - 170, 150.0f, 90.0f, 390.0f); }
    { juce::ColourGradient rr2 (CYAN.withAlpha (0.3f), cx + 170, 300, CYAN.withAlpha (0.0f), cx + 90, 300, false); g.setGradientFill (rr2); g.fillRect (cx + 80, 150.0f, 90.0f, 390.0f); }
    // cuciture e cinghie
    g.setColour (CYAN.withAlpha (0.45f)); { Path s; s.startNewSubPath (cx - 70, 400); s.cubicTo (cx - 40, 440, cx - 20, 480, cx - 18, 532); g.strokePath (s, juce::PathStrokeType (1.4f, juce::PathStrokeType::curved, juce::PathStrokeType::butt)); }
    g.setColour (MAG.withAlpha (0.45f)); { Path s; s.startNewSubPath (cx + 70, 400); s.cubicTo (cx + 40, 440, cx + 20, 480, cx + 18, 532); g.strokePath (s, juce::PathStrokeType (1.4f)); }
    g.setColour (juce::Colour (0xff0b0c11)); g.fillRect (cx - 120, 448.0f, 240.0f, 16.0f); g.setColour (juce::Colour (0xff3a3e4c)); g.fillRect (cx - 14, 446.0f, 28.0f, 20.0f);
    g.restoreState();
    g.setColour (MAG.withAlpha (0.8f)); { Path e; e.startNewSubPath (cx - 168, 532); e.cubicTo (cx - 160, 440, cx - 120, 392, cx - 96, 370); e.cubicTo (cx - 118, 300, cx - 104, 210, cx - 58, 176); g.strokePath (e, juce::PathStrokeType (2.0f)); }
    g.setColour (CYAN.withAlpha (0.8f)); { Path e; e.startNewSubPath (cx + 168, 532); e.cubicTo (cx + 160, 440, cx + 120, 392, cx + 96, 370); e.cubicTo (cx + 118, 300, cx + 104, 210, cx + 58, 176); g.strokePath (e, juce::PathStrokeType (2.0f)); }
    // il buio dentro il cappuccio
    Path face; face.startNewSubPath (cx - 62, 270); face.cubicTo (cx - 64, 210, cx - 30, 192, cx, 192); face.cubicTo (cx + 30, 192, cx + 64, 210, cx + 62, 270);
    face.cubicTo (cx + 60, 330, cx + 34, 376, cx, 384); face.cubicTo (cx - 34, 376, cx - 60, 330, cx - 62, 270); face.closeSubPath();
    g.setColour (juce::Colour (0xff030305)); g.fillPath (face);
    // respiratore
    { Path m; m.startNewSubPath (cx - 40, 300); m.lineTo (cx + 40, 300); m.lineTo (cx + 30, 350); m.lineTo (cx + 12, 368); m.lineTo (cx - 12, 368); m.lineTo (cx - 30, 350); m.closeSubPath();
      juce::ColourGradient mg (juce::Colour (0xff2a2e3a), cx, 300, juce::Colour (0xff101218), cx, 368, false); g.setGradientFill (mg); g.fillPath (m);
      g.setColour (juce::Colour (0xff4a5062)); g.strokePath (m, juce::PathStrokeType (1.2f));
      g.setColour (juce::Colour (0xff07080b)); for (int i = 0; i < 5; ++i) g.fillRect (cx - 18 + i * 8.0f, 318.0f, 4.0f, 34.0f); }
    // cuffia col microfono
    g.setColour (juce::Colour (0xff1a1c25)); g.fillRoundedRectangle (cx - 84, 248, 26, 58, 9); g.fillRoundedRectangle (cx + 58, 248, 26, 58, 9);
    g.setColour (MAG.withAlpha (0.7f)); g.drawRoundedRectangle (cx - 84, 248, 26, 58, 9, 1.2f); g.setColour (CYAN.withAlpha (0.7f)); g.drawRoundedRectangle (cx + 58, 248, 26, 58, 9, 1.2f);
    { Path boom; boom.startNewSubPath (cx + 70, 300); boom.cubicTo (cx + 70, 350, cx + 40, 362, cx + 18, 360); g.setColour (juce::Colour (0xff2a2d38)); g.strokePath (boom, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded)); }
    g.setColour (juce::Colour (0xff15161d)); g.fillEllipse (cx + 6, 352, 16, 16);
    // targhetta
    { auto p = juce::Rectangle<float> (cx + 40, 470, 64, 26); g.setColour (juce::Colour (0xff0b0c11)); g.fillRect (p); g.setColour (ACID); g.drawRect (p, 1.2f); text (g, "TU", p, bun (16.0f), ACID); }
}
void Editor::drawHoloStatic (juce::Graphics& g)
{
    { juce::ColourGradient bg (ROOM1, 590, 40, ROOM0, 590, (float) H, false); g.setGradientFill (bg); g.fillAll(); }
    // parete: pannelli, tubi, cavi
    g.setColour (juce::Colour (0xff141726)); for (float x = 0; x < W; x += 64) g.drawVerticalLine ((int) x, 56, 470);
    g.setColour (juce::Colour (0xff1b1f33)); g.drawHorizontalLine (180, 0, (float) W); g.drawHorizontalLine (182, 0, (float) W);
    for (int i = 0; i < 5; ++i) { Path c; const float x0 = 120.0f + i * 230, x1 = x0 + 140 + i * 10; c.startNewSubPath (x0, 56); c.quadraticTo ((x0 + x1) * 0.5f, 140.0f + i * 12, x1, 56); g.setColour (juce::Colour (0xff05060a)); g.strokePath (c, juce::PathStrokeType (3.0f)); }
    // foschia colorata
    { juce::ColourGradient hz (MAG.withAlpha (0.16f), 140, 360, MAG.withAlpha (0.0f), 520, 360, true); g.setGradientFill (hz); g.fillAll(); }
    { juce::ColourGradient hz (CYAN.withAlpha (0.13f), 1040, 360, CYAN.withAlpha (0.0f), 660, 360, true); g.setGradientFill (hz); g.fillAll(); }
    // pavimento a griglia in prospettiva, bagnato
    {
        const float vx = 590, vy = 360, fy = 470;
        g.saveState(); g.reduceClipRegion (juce::Rectangle<int> (0, (int) fy, W, H - (int) fy));
        g.setColour (CYAN.withAlpha (0.12f));
        for (int i = -14; i <= 14; ++i) g.drawLine (vx, vy, vx + i * 120.0f, (float) H + 200, 1.0f);
        for (float t = 0.06f; t < 1.2f; t *= 1.35f) { const float y = fy + (H - fy) * t; g.drawHorizontalLine ((int) y, 0, (float) W); }
        juce::Random r (4); for (int i = 0; i < 14; ++i) { const float x = r.nextFloat() * W, y = fy + 20 + r.nextFloat() * (H - fy - 20); g.setColour ((r.nextBool() ? MAG : CYAN).withAlpha (0.08f)); g.fillRect (x, y, 30 + r.nextFloat() * 80, 1.5f); }
        g.restoreState();
    }
    grime (g, { 0, 50, (float) W, (float) H - 50 }, 7, 1.8f);
    drawMain (g, 0);
    // pannelli di vetro dei personaggi
    for (int k = 0; k < kVoices; ++k)
    {
        const auto s = holoSlot (k); const bool L = leftSide (k);
        auto panel = juce::Rectangle<float> (L ? s.getX() + 188 : s.getX(), s.getY() + 12, 186, 186);
        g.setColour (GLASS); g.fillRect (panel); g.setColour (holoTint (k).withAlpha (0.18f)); g.drawRect (panel, 1.0f);
        bracket (g, panel, holoTint (k).withAlpha (0.75f));
        text (g, "0" + juce::String (k + 1), panel.removeFromBottom (14).translated (0, -2), mon (8.5f), holoTint (k).withAlpha (0.5f), L ? juce::Justification::centredRight : juce::Justification::centredLeft);
        // cavo dal pannello al proiettore
        g.setColour (juce::Colour (0xff05060a)); g.drawLine (L ? panel.getX() : panel.getRight(), panel.getBottom() - 20, L ? s.getX() + 140 : s.getRight() - 140, s.getBottom() - 10, 2.5f);
    }
    // consolle della crew davanti a te
    {
        auto c = juce::Rectangle<float> (398, 530, 384, 182);
        Path d; d.startNewSubPath (c.getX() + 18, c.getY()); d.lineTo (c.getRight() - 18, c.getY()); d.lineTo (c.getRight(), c.getBottom()); d.lineTo (c.getX(), c.getBottom()); d.closeSubPath();
        g.setColour (juce::Colours::black.withAlpha (0.6f)); g.fillPath (d, juce::AffineTransform::translation (0, 6));
        { juce::ColourGradient cg (juce::Colour (0xff20232f), c.getX(), c.getY(), juce::Colour (0xff0c0d13), c.getX(), c.getBottom(), false); g.setGradientFill (cg); g.fillPath (d); }
        g.setColour (juce::Colour (0xff3a3f52)); g.strokePath (d, juce::PathStrokeType (1.5f));
        g.setColour (MAG.withAlpha (0.6f)); g.drawLine (c.getX() + 18, c.getY() + 1, c.getRight() - 18, c.getY() + 1, 2.0f);
        for (auto p : { juce::Point<float> (c.getX() + 14, c.getBottom() - 10), juce::Point<float> (c.getRight() - 14, c.getBottom() - 10) }) { g.setColour (juce::Colour (0xff4a4f60)); g.fillEllipse (p.x - 4, p.y - 4, 8, 8); g.setColour (juce::Colours::black); g.drawLine (p.x - 3, p.y, p.x + 3, p.y, 1.2f); }
        // nastro adesivo con scritto a pennarello
        g.saveState(); g.addTransform (juce::AffineTransform::rotation (-0.03f, c.getCentreX(), c.getY() - 2));
        g.setColour (juce::Colour (0xffd9d4c6)); g.fillRect (c.getCentreX() - 60, c.getY() - 13, 120.0f, 20.0f);
        text (g, "LA CREW", { c.getCentreX() - 60, c.getY() - 13, 120, 20 }, beb (20.0f), juce::Colour (0xff141418));
        g.restoreState();
        grime (g, c, 13, 1.2f);
    }
    // barra in alto
    { g.setColour (juce::Colour (0xf0040509)); g.fillRect (0, 0, W, 56); g.setColour (MAG.withAlpha (0.5f)); g.drawHorizontalLine (56, 0, (float) W); }
    glowText (g, "CREW", { 21, 2, 200, 50 }, bun (40.0f), CYAN, 0.55f, juce::Justification::centredLeft);
    glowText (g, "CREW", { 18, 0, 200, 50 }, bun (40.0f), MAG, 1.0f, juce::Justification::centredLeft);
    text (g, "JACKPOT CYBERNET", { 150, 12, 200, 14 }, mon (10.0f), CYAN.withAlpha (0.8f), juce::Justification::centredLeft);
    text (g, "GANG VOCALS // OLOGRAMMI", { 150, 28, 220, 14 }, mon (9.0f), juce::Colour (0xff6f7590), juce::Justification::centredLeft);
    { auto lcd = juce::Rectangle<float> (398, 8, 384, 40); g.setColour (juce::Colour (0xff06080e)); g.fillRect (lcd); g.setColour (CYAN.withAlpha (0.35f)); g.drawRect (lcd, 1.0f); bracket (g, lcd, CYAN, 8, 1.5f); }
    // righe di scansione su tutto
    g.setColour (juce::Colours::black.withAlpha (0.10f)); for (int y = 0; y < H; y += 3) g.drawHorizontalLine (y, 0, (float) W);
    { juce::ColourGradient vg (juce::Colours::transparentBlack, 590, 380, juce::Colours::black.withAlpha (0.55f), 0, 0, true); g.setGradientFill (vg); g.fillAll(); }
}

void Root::paint (juce::Graphics& g)
{
    const float sc = juce::jlimit (1.0f, 4.0f, g.getInternalContext().getPhysicalPixelScaleFactor());
    const int th = ed.proc.uiTheme;
    if (bg.isNull() || std::abs (bgScale - sc) > 0.01f || bgTheme != th)
    {
        bgScale = sc; bgTheme = th; bg = juce::Image (juce::Image::RGB, juce::roundToInt (Editor::W * sc), juce::roundToInt (Editor::H * sc), false);
        juce::Graphics ig (bg); ig.addTransform (juce::AffineTransform::scale (sc));
        if (th == 0) ed.drawHoloStatic (ig); else ed.drawSketchStatic (ig);
    }
    g.drawImageTransformed (bg, juce::AffineTransform::scale (1.0f / sc));
    using H = Editor::Hot; auto R = [this] (int i) { return ed.hotR[i]; };
    const auto name = ed.proc.currentName().toUpperCase();
    if (th == 0)
    {
        // insegna al neon dietro di te (sfarfalla ogni tanto)
        glowText (g, "C R E W", { 460, 70, 260, 70 }, bun (46.0f), MAG, 0.55f + 0.45f * ed.signFlicker);
        // visiera: le barre seguono la tua voce
        {
            const float cx = 590, l = juce::jlimit (0.0f, 1.0f, ed.inLvl * 1.5f);
            auto vz = juce::Rectangle<float> (cx - 52, 246, 104, 30);
            g.setColour (juce::Colour (0xff020205)); g.fillRoundedRectangle (vz, 6);
            for (int i = 0; i < 13; ++i)
            {
                const float f = std::abs (i - 6) / 6.0f, hh = 3 + 22 * l * (1.0f - 0.6f * f) * (0.7f + 0.3f * std::sin (ed.tick * 0.5f + i * 1.7f));
                g.setColour ((i < 6 ? MAG : i > 6 ? CYAN : juce::Colours::white).withAlpha (0.35f + 0.65f * l)); g.fillRect (vz.getX() + 6 + i * 7.3f, vz.getCentreY() - hh * 0.5f, 4.5f, hh);
            }
            g.setColour (CYAN.withAlpha (0.25f + 0.5f * l)); g.drawRoundedRectangle (vz.expanded (2), 7, 1.5f);
            g.setColour (juce::Colour (0xffff3030).withAlpha (0.4f + 0.6f * l)); g.fillEllipse (cx + 10, 356, 8, 8);
        }
        glowText (g, "<", R (H::H_PREV), bun (22.0f), hot == H::H_PREV ? MAG : CYAN, 1.0f);
        glowText (g, ">", R (H::H_NEXT), bun (22.0f), hot == H::H_NEXT ? MAG : CYAN, 1.0f);
        glowText (g, name, R (H::H_NAME), beb (30.0f), hot == H::H_NAME ? MAG : CYAN, 1.0f);
        text (g, "SALVA", R (H::H_SAVE), mon (11.0f), hot == H::H_SAVE ? MAG : ACID);
        text (g, "ANNULLA", R (H::H_UNDO), mon (10.0f), ed.proc.undo.canUndo() ? (hot == H::H_UNDO ? MAG : juce::Colour (0xffb7bdd6)) : juce::Colour (0xff3d4256));
        text (g, "RIPETI", R (H::H_REDO), mon (10.0f), ed.proc.undo.canRedo() ? (hot == H::H_REDO ? MAG : juce::Colour (0xffb7bdd6)) : juce::Colour (0xff3d4256));
        g.setColour (hot == H::H_THEME ? MAG : CYAN.withAlpha (0.6f)); g.drawRect (R (H::H_THEME), 1.0f);
        text (g, "TEMA: OLOGRAMMI", R (H::H_THEME), mon (10.0f), hot == H::H_THEME ? MAG : CYAN);
        return;
    }
    {
        auto n = juce::Rectangle<float> (390, 16, 430, 64);
        g.setColour (juce::Colours::black.withAlpha (0.4f)); g.fillRect (n.translated (4, 5));
        g.setColour (YEL.darker (0.15f)); g.fillRect (n); tape (g, n.getCentreX(), n.getY() + 2, 0.04f, 80);
        grime (g, n, 21, 2.0f);
        const auto ink2 = juce::Colour (0xff141418);
        text (g, "PRESET", n.withHeight (16).translated (14, 6), mon (9.0f), ink2.withAlpha (0.6f), juce::Justification::centredLeft);
        text (g, "<", R (H::H_PREV), bun (24.0f), hot == H::H_PREV ? RED : ink2);
        text (g, ">", R (H::H_NEXT), bun (24.0f), hot == H::H_NEXT ? RED : ink2);
        text (g, name, R (H::H_NAME), beb (34.0f), hot == H::H_NAME ? RED : ink2);
    }
    text (g, "SALVA", R (H::H_SAVE), beb (22.0f), hot == H::H_SAVE ? RED : INK);
    text (g, "ANNULLA", R (H::H_UNDO), beb (18.0f), ed.proc.undo.canUndo() ? (hot == H::H_UNDO ? RED : INK) : PENCIL.withAlpha (0.6f));
    text (g, "RIPETI", R (H::H_REDO), beb (18.0f), ed.proc.undo.canRedo() ? (hot == H::H_REDO ? RED : INK) : PENCIL.withAlpha (0.6f));
    text (g, "TEMA: QUADERNO", R (H::H_THEME), beb (18.0f), hot == H::H_THEME ? RED : INK);
    {
        auto m = juce::Rectangle<float> (922, 686, 220, 8); const float l = juce::jlimit (0.0f, 1.0f, ed.inLvl * 1.4f);
        g.setColour (INK.withAlpha (0.12f)); g.fillRect (m); g.setColour (RED); g.fillRect (m.withWidth (m.getWidth() * l));
        g.setColour (INK.withAlpha (0.6f)); g.drawRect (m, 1.2f); text (g, "LA TUA VOCE", m.translated (0, -16).withHeight (14), mon (9.0f), INK.withAlpha (0.7f), juce::Justification::centredLeft);
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
    addAndMakeVisible (root);
    auto pct = [] (float v) { return juce::String (juce::roundToInt (v * 100)) + "%"; };
    auto db = [] (float v) { return v <= -59.9f ? juce::String ("MUTO") : (v > 0 ? "+" : "") + juce::String (v, 1) + " dB"; };
    auto pan = [] (float v) { const int q = juce::roundToInt (v * 100); return q == 0 ? juce::String ("C") : (q < 0 ? "L" : "R") + juce::String (std::abs (q)); };
    auto st = [] (float v) { const int q = juce::roundToInt (v); return q == 0 ? juce::String ("UNISONO") : (q > 0 ? "+" : "") + juce::String (q) + " st"; };
    auto tone = [] (float v) { return std::abs (v) < 0.05f ? juce::String ("NEUTRO") : v < 0 ? "SCURO " + juce::String (juce::roundToInt (-v * 100)) : "CHIARO " + juce::String (juce::roundToInt (v * 100)); };
    auto form = [] (float v) { const int q = juce::roundToInt (v * 6); return q == 0 ? juce::String ("UGUALE") : (q < 0 ? "GROSSA " : "PICCOLA ") + juce::String (std::abs (q)); };
    auto ms = [] (float v) { return juce::String (juce::roundToInt (v)) + " ms"; };
    for (int k = 0; k < kVoices; ++k)
    {
        auto c = std::make_unique<Card> (*this, k);
        c->setTooltip (juce::String (characters()[(size_t) k].name) + ": " + characters()[(size_t) k].role + " (clicca il personaggio per accenderlo)");
        root.addAndMakeVisible (*c); cards.push_back (std::move (c));
        struct D { const char* w; const char* l; std::function<juce::String (float)> f; int steps; bool bip; const char* tip; };
        const D d[] = { { "lvl", "VOLUME", db, 0, false, "Volume del personaggio" }, { "pan", "POSTO", pan, 0, true, "Dove sta nello stereo" },
                        { "pitch", "ALTEZZA", st, 25, true, "Altezza in semitoni (ottava, quinta o unisono)" },
                        { "formant", "FORMANTE", form, 0, true, "La taglia della voce, senza cambiare la nota: giu' piu' grossa, su piu' piccola" },
                        { "delay", "RITARDO", ms, 0, false, "Quanti millisecondi dopo di te entra (piu' alto = piu' indietro e piu' largo)" },
                        { "tone", "COLORE", tone, 0, true, "Scuro (meno acuti) o chiaro (meno bassi, piu' presenza)" } };
        for (int j = 0; j < 6; ++j)
        {
            auto kn = std::make_unique<Knob> (*this, Processor::vid (k, d[j].w), d[j].l, sk::accent (k), d[j].f, k * 10 + j);
            kn->steps = d[j].steps; kn->bipolar = d[j].bip; kn->voice = k; kn->setTooltip (d[j].tip);
            root.addAndMakeVisible (*kn); knobs.push_back (std::move (kn));
        }
    }
    struct G { const char* id; const char* l; std::function<juce::String (float)> f; const char* tip; int steps; };
    auto hz = [] (float v) { return v >= 1000 ? juce::String (v / 1000.0f, 1) + " k" : juce::String (juce::roundToInt (v)) + " Hz"; };
    const G gl[] = {
        { "loose", "FUORI TEMPO", pct, "Quanto la crew esce dal tempo e respira: ognuno entra un po' prima o dopo, e cambia di continuo (0 = tutti perfetti)", 0 },
        { "drift", "STONATI", pct, "Quanto ognuno stona di qualche cent e ondeggia: piu' alto = piu' gente vera", 0 },
        { "spread", "LARGHEZZA", pct, "Quanto la crew si apre nello stereo", 0 },
        { "shout", "GRIDATO", pct, "Compressione veloce: la crew viene avanti, da cantato a urlato", 0 },
        { "drive", "DRIVE", pct, "Saturazione della crew (sporca, aggressiva)", 0 },
        { "lowcut", "TAGLIO BASSI", hz, "Taglia i bassi della crew per non impastare la tua voce", 0 },
        { "highcut", "TAGLIO ALTI", hz, "Taglia gli acuti della crew (piu' dietro, meno sibilanti)", 0 },
        { "verb", "RIVERBERO", pct, "Quanto riverbero sulla crew", 0 },
        { "size", "STANZA", pct, "Grandezza della stanza", 0 },
        { "echo", "DELAY", pct, "Delay a tempo sulla crew (ping-pong)", 0 },
        { "echoDiv", "TEMPO", [] (float v) { return juce::String (echoDivName (juce::roundToInt (v))); }, "Tempo del delay, a tempo col BPM", 6 },
        { "echoFb", "RIPETIZ.", pct, "Quante ripetizioni del delay", 0 },
        { "crew", "CREW", db, "Volume di tutta la crew", 0 },
        { "dry", "TU", db, "Volume della tua voce (la principale)", 0 },
    };
    for (int j = 0; j < 14; ++j)
    {
        auto kn = std::make_unique<Knob> (*this, gl[j].id, gl[j].l, j < 12 ? RED : BLUE, gl[j].f, 200 + j);
        kn->setTooltip (gl[j].tip); kn->steps = gl[j].steps;
        root.addAndMakeVisible (*kn); knobs.push_back (std::move (kn));
    }
    // scheda dei preset
    browser.entries = [this]
    {
        std::vector<jcyber::Entry> v;
        for (int i = 0; i < (int) presets().size(); ++i) v.push_back ({ presets()[(size_t) i].cat, presets()[(size_t) i].name, presets()[(size_t) i].note, i, false });
        auto files = Processor::presetFolder().findChildFiles (juce::File::findFiles, false, "*.crew"); files.sort();
        for (int i = 0; i < files.size(); ++i) v.push_back ({ "I MIEI", files[i].getFileNameWithoutExtension(), "preset salvato da te", 1000 + i, true });
        return v;
    };
    browser.current = [this] { if (proc.userName.isEmpty()) return proc.preset; auto files = Processor::presetFolder().findChildFiles (juce::File::findFiles, false, "*.crew"); files.sort(); for (int i = 0; i < files.size(); ++i) if (files[i].getFileNameWithoutExtension() == juce::File::createLegalFileName (proc.userName)) return 1000 + i; return -1; };
    browser.load = [this] (const jcyber::Entry& e)
    {
        if (e.user) { auto files = Processor::presetFolder().findChildFiles (juce::File::findFiles, false, "*.crew"); files.sort(); if (juce::isPositiveAndBelow (e.id - 1000, files.size())) proc.loadUserPreset (files[e.id - 1000]); }
        else proc.loadPreset (e.id);
        root.repaint(); for (auto& c : cards) c->repaint();
    };
    browser.save = [this] { askSave(); };
    browser.canOverwrite = [this] { return proc.userName.isNotEmpty(); };
    browser.overwrite = [this] { proc.saveUserPreset (proc.userName); };
    browser.folder = [] { Processor::presetFolder().revealToUser(); };
    browser.remove = [] (const jcyber::Entry& e) { auto files = Processor::presetFolder().findChildFiles (juce::File::findFiles, false, "*.crew"); files.sort(); if (juce::isPositiveAndBelow (e.id - 1000, files.size())) files[e.id - 1000].moveToTrash(); };
    browser.onClose = [this] { root.repaint(); };
    browser.title = "CREW // PRESET"; browser.sub = "ARCHIVIO CREW";
    browser.titleFont = bun (30.0f); browser.nameFont = beb (24.0f); browser.monoFont = mon (11.0f);
    root.addChildComponent (browser);
    root.addAndMakeVisible (sizeChip);
    setWantsKeyboardFocus (true);
    layout();
    scaler.setup();
    startTimerHz (30);
}
void Editor::layout()
{
    const bool ho = holoTheme();
    for (int k = 0; k < kVoices; ++k)
    {
        juce::Rectangle<float> card;
        if (ho)
        {
            const auto s = holoSlot (k); const bool L = leftSide (k);
            card = { L ? s.getX() : s.getX() + 188, s.getY(), 186, s.getHeight() };
            const float px = L ? s.getX() + 190 : s.getX() + 2, py = s.getY() + 16;
            for (int j = 0; j < 6; ++j) knobs[(size_t) (k * 6 + j)]->setBounds (juce::Rectangle<float> (px + (j % 3) * 61.0f, py + (j / 3) * 88.0f, 61, 86).toNearestInt());
        }
        else
        {
            card = { 24 + (k % 3) * 292.0f, 100 + (k / 3) * 304.0f, 280, 296 };
            for (int j = 0; j < 6; ++j) knobs[(size_t) (k * 6 + j)]->setBounds (juce::Rectangle<float> (card.getX() + 10 + (j % 3) * 87.0f, card.getY() + 180 + (j / 3) * 56.0f, 86, 56).toNearestInt());
        }
        cards[(size_t) k]->setBounds (card.toNearestInt()); cards[(size_t) k]->cache = {}; cards[(size_t) k]->holo = {};
        for (int j = 0; j < 6; ++j) knobs[(size_t) (k * 6 + j)]->fill = ho ? holoTint (k) : sk::accent (k);
    }
    for (int j = 0; j < 14; ++j)
    {
        auto& kn = knobs[(size_t) (kVoices * 6 + j)];
        if (ho) { kn->fill = j < 7 ? MAG : (j < 12 ? CYAN : ACID); kn->setBounds (juce::Rectangle<float> (401 + (j % 7) * 54.6f, 540 + (j / 7) * 84.0f, 54, 82).toNearestInt()); }
        else { kn->fill = j < 7 ? RED : (j < 12 ? BLUE : YEL); kn->setBounds (juce::Rectangle<float> (926 + (j % 2) * 106.0f, 166 + (j / 2) * 70.0f, 104, 68).toNearestInt()); }
    }
    if (ho)
    {
        hotR[H_PREV] = { 404, 12, 30, 32 }; hotR[H_NEXT] = { 746, 12, 30, 32 }; hotR[H_NAME] = { 436, 8, 308, 40 };
        hotR[H_SAVE] = { 792, 14, 56, 28 }; hotR[H_UNDO] = { 850, 16, 70, 24 }; hotR[H_REDO] = { 920, 16, 62, 24 }; hotR[H_THEME] = { 990, 14, 128, 28 };
        sizeChip.setBounds (1124, 20, 50, 15);
        browser.accent = MAG; browser.accent2 = CYAN; browser.ink = juce::Colour (0xffe8ecf7); browser.dim = juce::Colour (0xff6f7590);
        browser.bg0 = juce::Colour (0xf4060810); browser.bg1 = juce::Colour (0xf40d1020);
    }
    else
    {
        hotR[H_PREV] = { 400, 34, 34, 40 }; hotR[H_NEXT] = { 776, 34, 34, 40 }; hotR[H_NAME] = { 436, 30, 338, 46 };
        hotR[H_SAVE] = { 836, 50, 66, 28 }; hotR[H_UNDO] = { 908, 50, 84, 26 }; hotR[H_REDO] = { 996, 50, 76, 26 }; hotR[H_THEME] = { 836, 12, 160, 28 };
        sizeChip.setBounds (1100, 18, 58, 15);
        browser.accent = RED; browser.accent2 = YEL; browser.ink = INK; browser.dim = PENCIL.brighter (0.3f);
        browser.bg0 = juce::Colour (0xf6121116); browser.bg1 = juce::Colour (0xf6221f27);
    }
    browser.setBounds (0, 0, W, H);
    root.bg = {}; root.repaint();
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
        case H_NAME: presetMenu(); break;
        case H_SAVE: askSave(); break;
        case H_UNDO: proc.undo.undo(); break;
        case H_REDO: proc.undo.redo(); break;
        case H_THEME: proc.uiTheme = 1 - proc.uiTheme; layout(); break;
        default: break;
    }
    root.repaint(); for (auto& c : cards) c->repaint(); for (auto& k : knobs) k->repaint();
}
void Editor::presetMenu() { browser.open(); }
void Editor::presetMenuOld()
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
    const bool ho = holoTheme();
    juce::Random rnd;
    for (auto& c : cards)
    {
        const bool on = proc.value (Processor::vid (c->idx, "on")) > 0.5f;
        const float target = on ? juce::jlimit (0.0f, 1.0f, proc.uiMeter[(size_t) c->idx].load() * 3.0f) : 0.0f;
        c->mouth += (target - c->mouth) * (target > c->mouth ? 0.6f : 0.25f);
        if (ho && on && rnd.nextFloat() < 0.012f) { c->glitch = 1.0f; c->glitchSeed = tick; }
        const bool gl = c->glitch > 0.01f; c->glitch *= 0.75f;
        if (on != c->lastOn) { c->lastOn = on; c->repaint(); for (int j = 0; j < 6; ++j) knobs[(size_t) (c->idx * 6 + j)]->repaint(); }
        else if (gl || std::abs (c->mouth - c->lastMouth) > 0.02f || (on && c->mouth > 0.2f && tick % 2 == 0)) { c->lastMouth = c->mouth; if (ho) c->repaint(); else c->repaint (juce::Rectangle<int> (0, 0, c->getWidth(), 150)); }
    }
    if (proc.preset != lastPreset || proc.userName != lastUser) { lastPreset = proc.preset; lastUser = proc.userName; root.repaint(); }
    const float il = proc.uiIn.load(); inLvl = juce::jmax (il, inLvl * 0.85f);
    if (std::abs (inLvl - lastIn) > 0.01f || (ho && inLvl > 0.05f)) { lastIn = inLvl; if (ho) root.repaint (juce::Rectangle<int> (530, 240, 120, 130)); else root.repaint (juce::Rectangle<int> (918, 648, 230, 34)); }
    if (ho)
    {   // l'insegna sfarfalla ogni tanto
        const float old = signFlicker;
        signFlicker = rnd.nextFloat() < 0.04f ? 0.15f + rnd.nextFloat() * 0.5f : juce::jmin (1.0f, signFlicker + 0.25f);
        if (std::abs (old - signFlicker) > 0.01f) root.repaint (juce::Rectangle<int> (450, 64, 280, 82));
    }
    if (tick % 8 == 0) root.repaint (hotR[H_UNDO].getUnion (hotR[H_REDO]).toNearestInt());
    for (auto& k : knobs) { const float v = k->prm->getValue(); if (v != k->last) { k->last = v; k->repaint(); } }
}
}
