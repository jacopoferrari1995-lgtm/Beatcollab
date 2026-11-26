#include "TagEditor.h"

namespace jtag
{
using namespace ink;
namespace
{
// rettangolo con gli angoli tagliati (adesivo / nastro)
juce::Path chamfer (juce::Rectangle<float> r, float c)
{
    juce::Path p; p.startNewSubPath (r.getX() + c, r.getY()); p.lineTo (r.getRight(), r.getY()); p.lineTo (r.getRight(), r.getBottom() - c);
    p.lineTo (r.getRight() - c, r.getBottom()); p.lineTo (r.getX(), r.getBottom()); p.lineTo (r.getX(), r.getY() + c); p.closeSubPath(); return p;
}
// adesivo da fumetto: ombra piena spostata, corpo, contorno a inchiostro
void sticker (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour body, juce::Colour shadow, float off = 5, float c = 8, float line = 3)
{
    auto p = chamfer (r, c);
    g.setColour (shadow); g.fillPath (p, juce::AffineTransform::translation (off, off));
    g.setColour (body); g.fillPath (p);
    g.setColour (K); g.strokePath (p, juce::PathStrokeType (line, juce::PathStrokeType::mitered));
}
void halftone (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c, float step, float maxR, bool fromTopLeft)
{
    g.setColour (c);
    for (float y = r.getY(); y < r.getBottom(); y += step)
        for (float x = r.getX() + (((int) ((y - r.getY()) / step)) % 2 ? step * 0.5f : 0.0f); x < r.getRight(); x += step)
        {
            const float tx = (x - r.getX()) / r.getWidth(), ty = (y - r.getY()) / r.getHeight();
            const float f = fromTopLeft ? 1.0f - (tx + ty) * 0.5f : (tx + ty) * 0.5f;
            const float rr = maxR * juce::jlimit (0.0f, 1.0f, f * 1.3f - 0.15f);
            if (rr > 0.35f) g.fillEllipse (x - rr, y - rr, rr * 2, rr * 2);
        }
}
// schizzo di vernice: macchia, gocce che colano, spruzzi
void splat (juce::Graphics& g, float cx, float cy, float s, juce::Colour c, int seed)
{
    juce::Random r (seed);
    g.setColour (c);
    g.fillEllipse (cx - s, cy - s * 0.85f, s * 2, s * 1.7f);
    for (int i = 0; i < 9; ++i)
    {
        const float a = r.nextFloat() * juce::MathConstants<float>::twoPi, d = s * (0.7f + r.nextFloat() * 0.6f), rr = s * (0.2f + r.nextFloat() * 0.35f);
        g.fillEllipse (cx + std::cos (a) * d - rr, cy + std::sin (a) * d * 0.85f - rr, rr * 2, rr * 2);
    }
    for (int i = 0; i < 3; ++i)
    {   // gocce
        const float x = cx - s * 0.6f + r.nextFloat() * s * 1.2f, len = s * (0.8f + r.nextFloat() * 1.8f), w = s * (0.12f + r.nextFloat() * 0.1f);
        g.fillRoundedRectangle (x - w * 0.5f, cy, w, len, w * 0.5f); g.fillEllipse (x - w * 0.8f, cy + len - w * 0.6f, w * 1.6f, w * 1.6f);
    }
    for (int i = 0; i < 26; ++i)
    {   // spruzzi
        const float a = r.nextFloat() * juce::MathConstants<float>::twoPi, d = s * (1.3f + r.nextFloat() * 1.6f), rr = 0.8f + r.nextFloat() * s * 0.09f;
        g.fillEllipse (cx + std::cos (a) * d - rr, cy + std::sin (a) * d - rr, rr * 2, rr * 2);
    }
}
// nuvola di fumo viola a contorno pieno (come nei poster)
void smoke (juce::Graphics& g, std::initializer_list<juce::Point<float>> c, std::initializer_list<float> rad, juce::Colour fill, juce::Colour edge)
{
    auto ri = rad.begin();
    std::vector<std::pair<juce::Point<float>, float>> v; for (auto p : c) v.push_back ({ p, *ri++ });
    g.setColour (edge); for (auto& e : v) g.fillEllipse (e.first.x - e.second - 3.5f, e.first.y - e.second - 3.5f, (e.second + 3.5f) * 2, (e.second + 3.5f) * 2);
    g.setColour (fill); for (auto& e : v) g.fillEllipse (e.first.x - e.second, e.first.y - e.second, e.second * 2, e.second * 2);
    g.setColour (edge.withAlpha (0.35f)); for (auto& e : v) g.drawEllipse (e.first.x - e.second * 0.55f, e.first.y - e.second * 0.3f, e.second * 0.9f, e.second * 0.7f, 1.5f);
}
void text (juce::Graphics& g, const juce::String& t, juce::Rectangle<float> r, juce::Font f, juce::Colour c, juce::Justification j = juce::Justification::centred)
{ g.setFont (f); g.setColour (c); g.drawText (t, r, j, false); }
juce::String noteName (int n) { static const char* nn[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }; return n < 0 ? juce::String ("TUTTE") : juce::String (nn[n % 12]) + juce::String (n / 12 - 2); }
struct CardDef { const char* title; float x, w; juce::Colour body, label, title2; };
const CardDef cards[] = { { "VOCE", 456, 164, P2, K, Y }, { "SPORCO", 632, 164, M, K, Y }, { "GLITCH", 808, 164, K, Y, M }, { "SPAZIO", 984, 180, WH, K, Y } };
constexpr float cardY = 398, cardH = 292;
}

// =============================================================================================== manopola
Knob::Knob (Editor& e, const char* id, juce::String l, juce::Colour c, juce::Colour o, std::function<juce::String (float)> f)
    : prm (e.P (id)), label (std::move (l)), cap (c), on (o), fmt (std::move (f))
{ setRepaintsOnMouseActivity (true); setMouseCursor (juce::MouseCursor::UpDownResizeCursor); }
void Knob::paint (juce::Graphics& g)
{
    const float w = (float) getWidth(), h = (float) getHeight(), lh = 15, vh = 19;
    const float S = juce::jmin (w - 14, h - lh - vh - 8), cx = w * 0.5f, cy = lh + 4 + S * 0.5f, R = S * 0.5f;
    const float v = prm->getValue(), a0 = -2.36f, a1 = 2.36f, av = a0 + (a1 - a0) * v;
    text (g, label, { -8, 0, w + 16, lh }, arc (11.0f, 0.06f), labelInk);
    // tacche di scala e arco del valore
    for (int i = 0; i <= 10; ++i)
    {
        const float a = a0 + (a1 - a0) * i / 10.0f, r1 = R + 3, r2 = R + (i % 5 == 0 ? 8.0f : 6.0f);
        g.setColour (labelInk.withAlpha (i % 5 == 0 ? 1.0f : 0.7f)); g.drawLine (cx + std::sin (a) * r1, cy - std::cos (a) * r1, cx + std::sin (a) * r2, cy - std::cos (a) * r2, i % 5 == 0 ? 2.2f : 1.4f);
    }
    {
        const float s0 = bipolar ? 0.0f : a0; juce::Path p; p.addCentredArc (cx, cy, R + 3, R + 3, 0, juce::jmin (s0, av), juce::jmax (s0, av), true);
        g.setColour (on); g.strokePath (p, juce::PathStrokeType (isMouseOverOrDragging() ? 5.0f : 4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::butt));
    }
    // corpo: ombra piena da fumetto, anello nero, cappuccio piatto con ombra a meta' e riflesso a falce
    g.setColour (K); g.fillEllipse (cx - R + 3, cy - R + 4, R * 2, R * 2);
    g.setColour (K); g.fillEllipse (cx - R, cy - R, R * 2, R * 2);
    const float rc = R - 4.5f;
    g.setColour (cap); g.fillEllipse (cx - rc, cy - rc, rc * 2, rc * 2);
    {   // ombra piatta in basso a destra
        juce::Path sh; sh.addEllipse (cx - rc, cy - rc, rc * 2, rc * 2); juce::Path cut; cut.addEllipse (cx - rc - rc * 0.28f, cy - rc - rc * 0.32f, rc * 2, rc * 2);
        g.saveState(); g.reduceClipRegion (sh); g.setColour (cap.darker (0.45f)); g.fillPath (sh); g.setColour (cap); g.fillPath (cut); g.restoreState();
    }
    {   // riflesso
        juce::Path hl; hl.addCentredArc (cx, cy, rc * 0.72f, rc * 0.72f, 0, -1.9f, -0.6f, true);
        g.setColour (juce::Colours::white.withAlpha (cap == K ? 0.35f : 0.75f)); g.strokePath (hl, juce::PathStrokeType (rc * 0.13f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    // lancetta spessa a inchiostro
    {
        const auto p0 = juce::Point<float> (cx + std::sin (av) * rc * 0.15f, cy - std::cos (av) * rc * 0.15f), p1 = juce::Point<float> (cx + std::sin (av) * (R - 1.5f), cy - std::cos (av) * (R - 1.5f));
        g.setColour (K); g.drawLine ({ p0, p1 }, juce::jmax (4.0f, R * 0.16f));
        g.setColour (cap == K ? WH : (cap == Y ? K : WH)); g.drawLine ({ p0, p1 }, juce::jmax (1.6f, R * 0.06f));
        g.setColour (K); g.fillEllipse (cx - 3, cy - 3, 6, 6);
    }
    // valore in un cartellino
    const juce::String t = fmt ? fmt (prm->convertFrom0to1 (v)) : prm->getCurrentValueAsText();
    g.setFont (mon (11.0f));
    const float tw = juce::jlimit (34.0f, w + 6, juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), t) + 12);
    auto vr = juce::Rectangle<float> (cx - tw * 0.5f, h - vh, tw, vh - 2);
    g.setColour (valueBox); g.fillRect (vr); g.setColour (K); g.drawRect (vr, 1.5f);
    g.setColour (valueInk); g.drawText (t, vr, juce::Justification::centred);
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

// =============================================================================================== schermo
juce::Rectangle<float> Screen::srcR() const { auto b = getLocalBounds().toFloat().reduced (14); b.removeFromTop (34); return b.removeFromTop (b.getHeight() * 0.42f); }
juce::Rectangle<float> Screen::tagR() const { auto b = getLocalBounds().toFloat().reduced (14); b.removeFromTop (34); b.removeFromTop (b.getHeight() * 0.42f + 10); b.removeFromBottom (16); return b; }
void Screen::refresh()
{
    const int gn = ed.proc.renderGen.load();
    if (gn == gen) return;
    gen = gn; ed.proc.waveSnapshot (src, tag, 300, tagSec, srcSec); repaint();
}
void Screen::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (K); g.fillRect (b);
    g.setColour (Y); g.drawRect (b.reduced (6), 2.0f);
    // griglia sottile da monitor
    g.setColour (P2.withAlpha (0.16f)); for (float x = b.getX() + 20; x < b.getRight() - 10; x += 24) g.drawVerticalLine ((int) x, b.getY() + 34, b.getBottom() - 10);
    auto& p = ed.proc; const bool rec = p.recording();
    {   // riga di stato
        juce::String s; s << "FRASE " << juce::String (srcSec, 2) << " s   //   TAG " << juce::String (tagSec, 2) << " s";
        text (g, s, { 18, 12, b.getWidth() - 200, 18 }, mon (12.0f), Y, juce::Justification::centredLeft);
        text (g, rec ? juce::String ("REC  ") + juce::String (p.recSeconds(), 1) + " s" : juce::String ("PRONTO"), { b.getRight() - 190, 12, 172, 18 }, mon (12.0f), rec ? M : P3, juce::Justification::centredRight);
    }
    const bool empty = src.empty() || srcSec <= 0.0f || ! p.hasSource();
    if (rec || empty)
    {
        if (rec)
        {   // durante la registrazione: un'onda che pulsa col livello
            auto r = b.reduced (30, 50).translated (0, 10);
            g.setColour (M.withAlpha (0.2f + 0.3f * ed.recPulse)); g.fillRect (r.withWidth (r.getWidth() * juce::jlimit (0.0f, 1.0f, p.recSeconds() / 12.0f)));
            text (g, "STO REGISTRANDO: PARLA ORA", r, bun (26.0f), M);
            text (g, "premi di nuovo REC per chiudere la frase (massimo 12 s)", r.translated (0, 34), mon (12.0f), WH);
        }
        else
        {
            text (g, "REGISTRA O TRASCINA QUI", b.withTrimmedTop (34).translated (0, -14), bun (28.0f), Y);
            text (g, "la tua frase: arma la traccia in Ableton, premi REC e parla", b.withTrimmedTop (34).translated (0, 24), mon (12.0f), WH);
            text (g, "(oppure trascina un file audio)", b.withTrimmedTop (34).translated (0, 44), mon (11.0f), P3);
        }
    }
    else
    {
        const float s0 = p.value ("start"), s1 = p.value ("end");
        {   // frase con il taglio
            auto r = srcR(); const int N = (int) src.size();
            text (g, "FRASE", r.withHeight (12).translated (0, -13), arc (9.0f, 0.1f), P3, juce::Justification::centredLeft);
            for (int i = 0; i < N; ++i)
            {
                const float f = (float) i / N, x = r.getX() + f * r.getWidth(), hh = juce::jmin (1.0f, src[(size_t) i] * 1.1f) * r.getHeight() * 0.5f;
                const bool in = f >= s0 && f <= s1;
                g.setColour (in ? P3 : PU.brighter (0.2f)); g.fillRect (x, r.getCentreY() - hh, juce::jmax (1.0f, r.getWidth() / N - 0.6f), juce::jmax (1.0f, hh * 2));
            }
            for (int k = 0; k < 2; ++k)
            {   // maniglie del taglio
                const float x = r.getX() + (k == 0 ? s0 : s1) * r.getWidth(); const bool hv = hoverH == k || handle == k;
                g.setColour (hv ? WH : Y); g.fillRect (x - 1.5f, r.getY() - 2, 3.0f, r.getHeight() + 4);
                juce::Path t; t.addTriangle (x - 7, r.getY() - 2, x + 7, r.getY() - 2, x, r.getY() + 7); g.fillPath (t);
            }
        }
        {   // tag forgiato
            auto r = tagR(); const int N = (int) tag.size();
            text (g, "TAG FORGIATO", r.withHeight (12).translated (0, -13), arc (9.0f, 0.1f), M, juce::Justification::centredLeft);
            if (N == 0) text (g, "frase troppo corta: allarga il taglio", r, mon (12.0f), WH);
            const float pos = p.uiPlayPos.load();
            for (int i = 0; i < N; ++i)
            {
                const float f = (float) i / N, x = r.getX() + f * r.getWidth(), hh = juce::jmin (1.0f, tag[(size_t) i] * 1.05f) * r.getHeight() * 0.5f;
                g.setColour (pos >= 0 && f <= pos ? Y : M); g.fillRect (x, r.getCentreY() - hh, juce::jmax (1.0f, r.getWidth() / N - 0.6f), juce::jmax (1.0f, hh * 2));
            }
            if (pos >= 0) { const float x = r.getX() + pos * r.getWidth(); g.setColour (WH); g.fillRect (x - 1, r.getY() - 4, 2.0f, r.getHeight() + 8); }
        }
    }
    if (ed.flashT > 0) text (g, ed.flash, { 18, b.getBottom() - 26, b.getWidth() - 36, 16 }, mon (11.0f), T, juce::Justification::centredLeft);
    if (dropHover) { g.setColour (M.withAlpha (0.35f)); g.fillRect (b); text (g, "MOLLA QUI LA FRASE", b, bun (30.0f), WH); }
}
void Screen::mouseMove (const juce::MouseEvent& e)
{
    int h = -1; auto r = srcR();
    if (r.expanded (0, 10).contains (e.position)) for (int k = 0; k < 2; ++k) if (std::abs (e.position.x - (r.getX() + ed.proc.value (k == 0 ? "start" : "end") * r.getWidth())) < 8) h = k;
    if (h != hoverH) { hoverH = h; setMouseCursor (h >= 0 ? juce::MouseCursor::LeftRightResizeCursor : juce::MouseCursor::NormalCursor); repaint(); }
}
void Screen::mouseDown (const juce::MouseEvent& e)
{
    handle = hoverH;
    if (handle >= 0) { ed.proc.undo.beginNewTransaction(); ed.P (handle == 0 ? "start" : "end")->beginChangeGesture(); return; }
    if (tagR().contains (e.position) || srcR().contains (e.position)) ed.proc.play();
}
void Screen::mouseDrag (const juce::MouseEvent& e)
{
    if (handle < 0) return;
    auto r = srcR(); float f = juce::jlimit (0.0f, 1.0f, (e.position.x - r.getX()) / r.getWidth());
    if (handle == 0) f = juce::jmin (f, ed.proc.value ("end") - 0.02f); else f = juce::jmax (f, ed.proc.value ("start") + 0.02f);
    ed.proc.setP (handle == 0 ? "start" : "end", f); repaint();
}
bool Screen::isInterestedInFileDrag (const juce::StringArray& f)
{
    for (auto& s : f) if (juce::File (s).hasFileExtension ("wav;aif;aiff;flac;mp3;ogg;m4a")) return true;
    return false;
}
void Screen::filesDropped (const juce::StringArray& f, int, int)
{
    dropHover = false;
    for (auto& s : f) if (ed.proc.loadFile (juce::File (s))) { ed.flash = "CARICATA: " + juce::File (s).getFileName(); ed.flashT = 90; break; }
    repaint();
}

// =============================================================================================== il personaggio
void Editor::drawCharacter (juce::Graphics& g)
{
    using juce::Path; const auto stroke4 = juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
    // cappuccio giallo
    Path hood;
    hood.startNewSubPath (26, 720); hood.cubicTo (30, 600, 58, 520, 94, 470); hood.cubicTo (70, 380, 82, 250, 132, 190);
    hood.cubicTo (166, 142, 206, 118, 238, 110); hood.cubicTo (292, 120, 340, 160, 362, 216); hood.cubicTo (394, 290, 394, 400, 368, 470);
    hood.cubicTo (402, 520, 430, 600, 436, 720); hood.closeSubPath();
    g.setColour (K); g.fillPath (hood, juce::AffineTransform::translation (8, 6));   // ombra piena
    g.setColour (Y); g.fillPath (hood);
    {   // ombra piatta sul lato destro e pieghe
        Path sh; sh.startNewSubPath (300, 132); sh.cubicTo (346, 170, 380, 260, 380, 360); sh.cubicTo (380, 420, 372, 450, 368, 470); sh.cubicTo (404, 520, 430, 600, 436, 720);
        sh.lineTo (380, 720); sh.cubicTo (370, 600, 340, 520, 318, 470); sh.cubicTo (346, 400, 350, 260, 300, 132); sh.closeSubPath();
        g.saveState(); g.reduceClipRegion (hood); g.setColour (Y2); g.fillPath (sh);
        g.setColour (K.withAlpha (0.55f)); for (int i = 0; i < 9; ++i) g.drawLine (352 + i * 3.0f, 300 + i * 22.0f, 372 + i * 3.0f, 286 + i * 22.0f, 1.6f);   // tratteggio a inchiostro
        g.setColour (K); g.drawLine (70, 640, 104, 520, 3.0f); g.drawLine (96, 700, 120, 600, 2.5f); g.drawLine (400, 700, 384, 600, 2.5f);
        g.restoreState();
    }
    g.setColour (K); g.strokePath (hood, stroke4);
    // giacca viola davanti
    Path jacket; jacket.startNewSubPath (160, 452); jacket.lineTo (234, 506); jacket.lineTo (308, 452); jacket.lineTo (350, 720); jacket.lineTo (116, 720); jacket.closeSubPath();
    g.setColour (PU); g.fillPath (jacket);
    g.saveState(); g.reduceClipRegion (jacket);
    g.setColour (P2); g.fillRect (240.0f, 500.0f, 120.0f, 230.0f);   // luce a destra della zip
    g.setColour (K.withAlpha (0.5f)); for (int i = 0; i < 12; ++i) g.drawLine (124 + i * 6.0f, 720, 150 + i * 6.0f, 640, 1.4f);
    g.restoreState();
    g.setColour (K); g.strokePath (jacket, stroke4);
    g.setColour (K); g.drawLine (234, 506, 234, 720, 3.0f);
    g.setColour (WH.withAlpha (0.8f)); for (float y = 516; y < 716; y += 8) g.drawLine (230, y, 238, y, 1.5f);
    {   // baveri
        Path l; l.addTriangle (160, 452, 234, 506, 194, 548); Path r; r.addTriangle (308, 452, 234, 506, 274, 548);
        g.setColour (P2); g.fillPath (l); g.setColour (P3); g.fillPath (r); g.setColour (K); g.strokePath (l, juce::PathStrokeType (3.0f)); g.strokePath (r, juce::PathStrokeType (3.0f));
    }
    {   // tracolla e fibbia
        Path s; s.startNewSubPath (112, 580); s.lineTo (350, 700); g.setColour (K); g.strokePath (s, juce::PathStrokeType (20.0f));
        g.setColour (P2.withAlpha (0.8f)); g.strokePath (s, juce::PathStrokeType (12.0f));
        g.saveState(); g.addTransform (juce::AffineTransform::rotation (0.46f, 200, 624)); g.setColour (K); g.fillRect (184.0f, 610.0f, 34.0f, 28.0f); g.setColour (WH); g.fillRect (188.0f, 614.0f, 26.0f, 20.0f); g.setColour (K); g.fillRect (196.0f, 618.0f, 10.0f, 12.0f); g.restoreState();
    }
    {   // toppa sul petto
        g.saveState(); g.addTransform (juce::AffineTransform::rotation (-0.14f, 296, 590));
        auto pr = juce::Rectangle<float> (258, 568, 82, 46);
        g.setColour (K); g.fillRect (pr.translated (3, 3)); g.setColour (M); g.fillRect (pr); g.setColour (K); g.drawRect (pr, 3.0f);
        text (g, "TAG", pr.withTrimmedBottom (14).translated (0, 2), bun (22.0f), K);
        text (g, "FORGE // 001", pr.withTrimmedTop (30), mon (8.5f), WH);
        g.restoreState();
    }
    // il buio dentro il cappuccio
    Path face; face.startNewSubPath (150, 300); face.cubicTo (148, 215, 205, 178, 234, 178); face.cubicTo (264, 178, 320, 212, 320, 300);
    face.cubicTo (320, 392, 282, 468, 236, 474); face.cubicTo (190, 468, 150, 392, 150, 300); face.closeSubPath();
    g.setColour (Y2); g.strokePath (face, juce::PathStrokeType (14.0f)); g.setColour (K); g.fillPath (face); g.strokePath (face, juce::PathStrokeType (3.5f));
    // cuffie da studio (archetto sopra il cappuccio, padiglioni magenta)
    {
        Path band; band.addCentredArc (236, 318, 156, 196, 0, -1.25f, 1.25f, true);
        g.setColour (K); g.strokePath (band, juce::PathStrokeType (16.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (M); g.strokePath (band, juce::PathStrokeType (7.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        for (int s = 0; s < 2; ++s)
        {
            auto cup = juce::Rectangle<float> (s == 0 ? 62.0f : 368.0f, 270, 44, 94);
            g.setColour (K); g.fillRoundedRectangle (cup.translated (4, 4), 14); g.setColour (M); g.fillRoundedRectangle (cup, 14);
            g.setColour (M2); g.fillRoundedRectangle (cup.withTrimmedLeft (cup.getWidth() * 0.55f), 12);
            g.setColour (K); g.drawRoundedRectangle (cup, 14, 4.0f);
            g.setColour (K); g.fillRoundedRectangle (cup.reduced (12, 22), 5);
            g.setColour (WH.withAlpha (0.85f)); g.drawLine (cup.getX() + 9, cup.getY() + 18, cup.getX() + 9, cup.getY() + 40, 3.0f);
        }
    }
    // maschera antigas
    {
        Path strap; strap.startNewSubPath (152, 300); strap.lineTo (318, 300); g.setColour (G); g.strokePath (strap, juce::PathStrokeType (12.0f)); g.setColour (K); g.strokePath (strap, juce::PathStrokeType (2.0f));
        Path plate; plate.startNewSubPath (180, 322); plate.lineTo (290, 322); plate.lineTo (284, 404); plate.lineTo (258, 448); plate.lineTo (212, 448); plate.lineTo (186, 404); plate.closeSubPath();
        g.setColour (G); g.fillPath (plate); g.setColour (G.brighter (0.35f)); g.fillRect (190.0f, 328.0f, 46.0f, 8.0f);
        g.setColour (K); g.strokePath (plate, juce::PathStrokeType (3.5f));
        // tubi verso le spalle
        for (int s = 0; s < 2; ++s)
        {
            Path h; const float sx = s == 0 ? 192.0f : 278.0f, dir = s == 0 ? -1.0f : 1.0f;
            h.startNewSubPath (sx, 446); h.cubicTo (sx + dir * 30, 500, sx + dir * 84, 520, sx + dir * 100, 600);
            g.setColour (K); g.strokePath (h, juce::PathStrokeType (15.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setColour (P2); g.strokePath (h, juce::PathStrokeType (8.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            juce::PathFlatteningIterator it (h); int k = 0;
            while (it.next()) if (++k % 2 == 0) { const float dx = it.x2 - it.x1, dy = it.y2 - it.y1, l = std::sqrt (dx * dx + dy * dy) + 1e-3f; g.setColour (K); g.drawLine (it.x2 - dy / l * 7, it.y2 + dx / l * 7, it.x2 + dy / l * 7, it.y2 - dx / l * 7, 1.8f); }
        }
        // filtri
        for (int s = 0; s < 2; ++s)
        {
            const float fx = s == 0 ? 188.0f : 280.0f, fy = 424;
            g.setColour (K); g.fillEllipse (fx - 27, fy - 29, 54, 60);
            g.setColour (P2); g.fillEllipse (fx - 24, fy - 26, 48, 54);
            g.setColour (PU); g.fillEllipse (fx - 24 + (s == 0 ? 6.0f : 0.0f), fy - 20, 42, 48);
            g.setColour (K); for (float r = 18; r > 4; r -= 5) g.drawEllipse (fx - r, fy - r * 1.1f, r * 2, r * 2.2f, 1.6f);
            g.setColour (WH.withAlpha (0.7f)); g.drawLine (fx - 14, fy - 16, fx - 6, fy - 22, 2.5f);
        }
        // griglia della bocca
        g.setColour (K); g.fillRoundedRectangle (220, 400, 30, 46, 5);
        g.setColour (WH.withAlpha (0.35f)); for (float y = 408; y < 442; y += 6) g.drawLine (225, y, 245, y, 1.6f);
        // occhiali: anelli (le lenti si accendono dal vivo)
        for (int s = 0; s < 2; ++s)
        {
            const float ex = s == 0 ? 204.0f : 266.0f, ey = 312;
            g.setColour (K); g.fillEllipse (ex - 30, ey - 30, 60, 60);
            g.setColour (juce::Colour (0xff6b6475)); g.fillEllipse (ex - 26, ey - 26, 52, 52);
            g.setColour (K); g.fillEllipse (ex - 21, ey - 21, 42, 42);
        }
    }
    // luci a fumetto sul bordo del cappuccio
    g.setColour (WH.withAlpha (0.9f));
    { Path hl; hl.startNewSubPath (120, 230); hl.cubicTo (140, 180, 180, 148, 222, 132); g.strokePath (hl, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded)); }
    { Path hl; hl.startNewSubPath (84, 420); hl.cubicTo (80, 380, 82, 340, 86, 300); g.strokePath (hl, juce::PathStrokeType (2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded)); }
}

// =============================================================================================== fondo fisso
void Editor::drawStatic (juce::Graphics& g)
{
    g.fillAll (Y);
    // retino e tagli a inchiostro
    halftone (g, { 0, 0, 300, 230 }, K.withAlpha (0.85f), 9, 3.6f, true);
    halftone (g, { 900, 0, 280, 90 }, M, 8, 2.6f, false);
    halftone (g, { 440, 600, 380, 120 }, K.withAlpha (0.2f), 10, 3.0f, false);
    {
        juce::Path s1; s1.addQuadrilateral (0, 470, 160, 380, 200, 410, 0, 560); g.setColour (K); g.fillPath (s1);
        juce::Path s2; s2.addQuadrilateral (360, 60, 450, 0, 470, 0, 380, 80); g.setColour (M); g.fillPath (s2);
        juce::Path s3; s3.addQuadrilateral (400, 90, 452, 40, 460, 52, 410, 104); g.setColour (K); g.fillPath (s3);
    }
    // fumo viola dietro al personaggio
    smoke (g, { { 70, 210 }, { 40, 300 }, { 110, 150 }, { 30, 400 } }, { 58, 50, 40, 44 }, P3, K);
    smoke (g, { { 400, 180 }, { 430, 260 }, { 360, 120 }, { 438, 360 } }, { 52, 44, 36, 40 }, P3, K);
    // schizzi magenta
    splat (g, 64, 120, 26, M, 3); splat (g, 418, 420, 22, M, 9); splat (g, 30, 560, 18, M, 17); splat (g, 330, 70, 12, M, 23);
    // TAG enorme dietro
    g.saveState(); g.addTransform (juce::AffineTransform::rotation (-0.12f, 220, 120));
    text (g, "TAG", { -30, -20, 520, 280 }, bun (196.0f), K);
    g.restoreState();
    // scritta verticale sul bordo
    g.saveState(); g.addTransform (juce::AffineTransform::rotation (-juce::MathConstants<float>::halfPi, 16, 700));
    text (g, "JACKPOT CYBERNET  //  SOUND DIVISION 01  //  TAG FORGE", { 16, 690, 640, 16 }, mon (10.0f), K, juce::Justification::centredLeft);
    g.restoreState();
    drawCharacter (g);
    // FORGE su nastro nero
    g.saveState(); g.addTransform (juce::AffineTransform::rotation (0.06f, 180, 650));
    {
        auto r = juce::Rectangle<float> (14, 618, 330, 66);
        g.setColour (M); g.fillRect (r.translated (6, 6)); g.setColour (K); g.fillRect (r);
        text (g, "FORGE", r.translated (3, 3), bun (52.0f), WH); text (g, "FORGE", r, bun (52.0f), Y);
    }
    g.restoreState();
    // adesivo del marchio
    {
        auto r = juce::Rectangle<float> (24, 20, 150, 46);
        g.saveState(); g.addTransform (juce::AffineTransform::rotation (-0.05f, r.getCentreX(), r.getCentreY()));
        sticker (g, r, K, M, 4, 7, 2.5f);
        text (g, "JACKPOT", r.withTrimmedBottom (20).translated (0, 4), arc (15.0f, 0.12f), Y);
        text (g, "CYBERNET // 01", r.withTrimmedTop (26), mon (9.5f), WH);
        g.restoreState();
    }
    // strisce di pericolo
    {
        g.saveState(); auto hz = juce::Rectangle<float> (300, 520, 120, 26); g.reduceClipRegion (hz.toNearestInt());
        g.setColour (K); g.fillRect (hz); g.setColour (Y);
        for (float x = hz.getX() - 30; x < hz.getRight(); x += 16) { juce::Path p; p.addQuadrilateral (x, hz.getBottom(), x + 8, hz.getBottom(), x + 34, hz.getY(), x + 26, hz.getY()); g.fillPath (p); }
        g.restoreState(); g.setColour (K); g.drawRect (hz, 2.5f);
    }
    // ----------------------------------------------------------------- parte destra
    {   // testata: nastro nero
        auto r = juce::Rectangle<float> (456, 16, 708, 62);
        sticker (g, r, K, M, 6, 12, 3);
        text (g, "PRESET", { 470, 20, 80, 12 }, arc (9.0f, 0.15f), P3, juce::Justification::centredLeft);
    }
    {   // schermo
        auto r = juce::Rectangle<float> (456, 190, 708, 194);
        g.setColour (M); g.fillRect (r.translated (8, 8)); g.setColour (K); g.fillRect (r.expanded (3));
    }
    // carte dei moduli
    for (auto& c : cards)
    {
        auto r = juce::Rectangle<float> (c.x, cardY, c.w, cardH);
        sticker (g, r, c.body, K, 7, 10, 3.5f);
        auto t = r.withHeight (34).reduced (3, 3);
        g.setColour (c.body == K ? M : K); g.fillRect (t);
        text (g, c.title, t.translated (2, 1), bun (19.0f), c.body == K ? K : c.title2);
        // nastro adesivo agli angoli
        for (int k = 0; k < 2; ++k)
        {
            g.saveState(); const float tx = k == 0 ? r.getX() + 4 : r.getRight() - 40, ty = r.getBottom() - 12;
            g.addTransform (juce::AffineTransform::rotation (k == 0 ? -0.5f : 0.5f, tx + 18, ty + 6));
            g.setColour (WH.withAlpha (0.75f)); g.fillRect (tx, ty, 36.0f, 13.0f); g.setColour (K.withAlpha (0.25f)); g.drawRect (tx, ty, 36.0f, 13.0f, 1.0f);
            g.restoreState();
        }
        if (c.body == WH) halftone (g, r.reduced (6).withTrimmedTop (34), P3.withAlpha (0.5f), 9, 1.6f, false);
        if (c.body == M) splat (g, r.getRight() - 30, r.getY() + 70, 10, M2, 41);
    }
    text (g, "OUT", { 1066, 92, 92, 12 }, arc (10.0f, 0.18f), K, juce::Justification::centred);
}

// =============================================================================================== radice: fondo, comandi, personaggio vivo
void Root::paint (juce::Graphics& g)
{
    const float sc = juce::jlimit (1.0f, 4.0f, g.getInternalContext().getPhysicalPixelScaleFactor());
    if (bg.isNull() || std::abs (bgScale - sc) > 0.01f)
    {
        bgScale = sc; bg = juce::Image (juce::Image::RGB, juce::roundToInt (Editor::W * sc), juce::roundToInt (Editor::H * sc), false);
        juce::Graphics ig (bg); ig.addTransform (juce::AffineTransform::scale (sc)); ed.drawStatic (ig);
    }
    g.drawImageTransformed (bg, juce::AffineTransform::scale (1.0f / sc));
    auto& p = ed.proc;
    // occhi accesi col tag (e col livello in registrazione)
    {
        const float l = juce::jlimit (0.0f, 1.0f, ed.lvl * 1.6f + (p.recording() ? 0.35f + 0.3f * ed.recPulse : 0.0f));
        for (int s = 0; s < 2; ++s)
        {
            const float ex = s == 0 ? 204.0f : 266.0f, ey = 312;
            juce::ColourGradient gl (M.withAlpha (0.25f + 0.55f * l), ex, ey, M.withAlpha (0.0f), ex + 40, ey, true); g.setGradientFill (gl); g.fillEllipse (ex - 40, ey - 40, 80, 80);
            g.setColour (M.interpolatedWith (juce::Colour (0xffff7fc0), l)); g.fillEllipse (ex - 19, ey - 19, 38, 38);
            g.setColour (M2); g.fillEllipse (ex - 19 + 9, ey - 19 + 9, 30, 30); g.setColour (M.interpolatedWith (WH, l * 0.6f)); g.fillEllipse (ex - 15, ey - 15, 26, 26);
            g.setColour (WH.withAlpha (0.9f)); g.fillEllipse (ex - 11, ey - 12, 9, 7);
            g.setColour (K); g.drawEllipse (ex - 19, ey - 19, 38, 38, 2.0f);
        }
        if (p.recording())
        {   // registrazione: onde dalle cuffie
            for (int s = 0; s < 2; ++s) for (int k = 0; k < 3; ++k)
            {
                const float ph = std::fmod (ed.recPulse * 0.5f + k / 3.0f + (float) ed.tick * 0.02f, 1.0f), cx = s == 0 ? 84.0f : 390.0f, r = 50 + ph * 50;
                juce::Path a; a.addCentredArc (cx, 317, r, r, 0, s == 0 ? -2.4f : 0.7f, s == 0 ? -0.7f : 2.4f, true);
                g.setColour (M.withAlpha (1.0f - ph)); g.strokePath (a, juce::PathStrokeType (4.0f));
            }
        }
    }
    // sbuffi dai filtri
    for (auto& pf : ed.puffs) { g.setColour (K.withAlpha (pf.a)); g.fillEllipse (pf.x - pf.r - 2, pf.y - pf.r - 2, pf.r * 2 + 4, pf.r * 2 + 4); g.setColour (P3.withAlpha (pf.a)); g.fillEllipse (pf.x - pf.r, pf.y - pf.r, pf.r * 2, pf.r * 2); }

    using H = Editor::Hot; auto R = [this] (int i) { return ed.hotR[i]; };
    // testata
    text (g, "<", R (H::H_PREV), bun (24.0f), hot == H::H_PREV ? M : Y);
    text (g, ">", R (H::H_NEXT), bun (24.0f), hot == H::H_NEXT ? M : Y);
    text (g, p.currentName().toUpperCase(), R (H::H_NAME), bun (24.0f), hot == H::H_NAME ? WH : Y);
    text (g, "SALVA", R (H::H_SAVE), arc (11.0f, 0.12f), hot == H::H_SAVE ? M : P3);
    text (g, "ANNULLA", R (H::H_UNDO), arc (9.5f, 0.1f), p.undo.canUndo() ? (hot == H::H_UNDO ? M : WH) : WH.withAlpha (0.3f));
    text (g, "RIPETI", R (H::H_REDO), arc (9.5f, 0.1f), p.undo.canRedo() ? (hot == H::H_REDO ? M : WH) : WH.withAlpha (0.3f));
    // REC
    {
        auto r = R (H::H_REC); const bool rec = p.recording();
        g.setColour (K); g.fillEllipse (r.translated (5, 5)); g.fillEllipse (r);
        auto in = r.reduced (6); g.setColour (rec ? M.interpolatedWith (WH, 0.25f * ed.recPulse) : (hot == H::H_REC ? M : M2)); g.fillEllipse (in);
        g.setColour (WH.withAlpha (0.8f)); juce::Path hl; hl.addCentredArc (in.getCentreX(), in.getCentreY(), in.getWidth() * 0.36f, in.getWidth() * 0.36f, 0, -1.9f, -0.7f, true); g.strokePath (hl, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        if (rec) text (g, juce::String (p.recSeconds(), 1), in, bun (20.0f), WH);
        else { g.setColour (WH); g.fillEllipse (in.withSizeKeepingCentre (18, 18).translated (0, -8)); text (g, "REC", in.withTrimmedTop (in.getHeight() * 0.5f).translated (0, -2), bun (14.0f), WH); }
    }
    // PLAY
    {
        auto r = R (H::H_PLAY); const bool ok = p.hasSource();
        g.setColour (K); g.fillEllipse (r.translated (4, 4)); g.fillEllipse (r);
        auto in = r.reduced (5); g.setColour (ok ? (hot == H::H_PLAY ? WH : Y) : Y.withAlpha (0.35f)); g.fillEllipse (in);
        juce::Path t; const float c = in.getCentreX() + 3, m = in.getCentreY(), s = in.getWidth() * 0.22f; t.addTriangle (c - s, m - s * 1.2f, c - s, m + s * 1.2f, c + s * 1.3f, m);
        g.setColour (K); g.fillPath (t);
    }
    // CARICA / ESPORTA
    for (int h : { (int) H::H_LOAD, (int) H::H_EXPORT })
    {
        auto r = R (h); sticker (g, r, hot == h ? M : WH, K, 4, 6, 2.5f);
        text (g, h == H::H_LOAD ? "CARICA" : "ESPORTA", r, arc (13.0f, 0.12f), K);
    }
    // TRASCINA NELLA DAW
    {
        auto r = R (H::H_DRAG); const bool ok = p.hasSource();
        sticker (g, r, hot == H::H_DRAG || dragOut ? M : K, M2, 5, 10, 3);
        text (g, "TRASCINA", r.withTrimmedBottom (r.getHeight() * 0.42f).translated (0, 6), bun (22.0f), ok ? Y : Y.withAlpha (0.35f));
        text (g, "IL TAG NELLA DAW", r.withTrimmedTop (r.getHeight() * 0.56f).translated (0, -4), mon (11.0f), WH.withAlpha (ok ? 1.0f : 0.35f));
        juce::Path ar; const float ax = r.getRight() - 22, ay = r.getY() + 16; ar.startNewSubPath (ax - 10, ay + 10); ar.lineTo (ax + 6, ay - 6); ar.startNewSubPath (ax - 4, ay - 6); ar.lineTo (ax + 6, ay - 6); ar.lineTo (ax + 6, ay + 4);
        g.setColour (Y); g.strokePath (ar, juce::PathStrokeType (3.0f));
    }
    // NOTA
    {
        auto r = R (H::H_NOTE); sticker (g, r, hot == H::H_NOTE ? Y : P2, K, 4, 8, 2.5f);
        text (g, "NOTA MIDI", r.withHeight (22).translated (0, 8), arc (10.0f, 0.1f), K);
        text (g, noteName (juce::roundToInt (p.value ("note"))), r.withTrimmedTop (28), bun (20.0f), K);
    }
    // AL CONTRARIO (nella carta GLITCH)
    {
        auto r = R (H::H_REV); const bool on = p.value ("reverse") > 0.5f;
        g.setColour (on ? M : K); g.fillRect (r); g.setColour (on ? K : (hot == H::H_REV ? M : Y)); g.drawRect (r, 2.5f);
        text (g, on ? "AL CONTRARIO: SI" : "AL CONTRARIO: NO", r, arc (11.0f, 0.06f), on ? K : Y);
    }
}
void Root::paintOverChildren (juce::Graphics&) {}
void Root::mouseMove (const juce::MouseEvent& e)
{
    int h = -1; for (int i = 0; i < Editor::H_COUNT; ++i) if (ed.hotR[i].contains (e.position)) h = i;
    if (h != hot) { hot = h; setMouseCursor (h >= 0 ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor); repaint(); }
}
void Root::mouseDown (const juce::MouseEvent& e)
{
    for (int i = 0; i < Editor::H_COUNT; ++i) if (ed.hotR[i].contains (e.position)) { ed.click (i, e); repaint(); return; }
}
void Root::mouseDrag (const juce::MouseEvent& e)
{
    if (dragOut || ! ed.hotR[Editor::H_DRAG].contains (e.mouseDownPosition) || e.getDistanceFromDragStart() < 6 || ! ed.proc.hasSource()) return;
    dragOut = true;
    const auto f = ed.proc.exportTag();
    if (f.existsAsFile()) { ed.flash = "TRASCINA: " + f.getFileName(); ed.flashT = 90; juce::DragAndDropContainer::performExternalDragDropOfFiles ({ f.getFullPathName() }, false, this); }
    dragOut = false; repaint();
}

// =============================================================================================== editor
Editor::Editor (Processor& p) : juce::AudioProcessorEditor (p), proc (p)
{
    using H = Hot;
    hotR[H_PREV] = { 470, 32, 36, 40 }; hotR[H_NEXT] = { 948, 32, 36, 40 }; hotR[H_NAME] = { 510, 30, 434, 42 };
    hotR[H_SAVE] = { 990, 26, 60, 22 }; hotR[H_UNDO] = { 990, 50, 76, 20 }; hotR[H_REDO] = { 1074, 50, 76, 20 };
    hotR[H_REC] = { 466, 92, 84, 84 }; hotR[H_PLAY] = { 566, 102, 64, 64 };
    hotR[H_LOAD] = { 646, 96, 108, 34 }; hotR[H_EXPORT] = { 646, 138, 108, 34 };
    hotR[H_DRAG] = { 770, 96, 170, 78 }; hotR[H_NOTE] = { 954, 96, 100, 78 };
    hotR[H_REV] = { 822, 636, 136, 30 };
    addAndMakeVisible (root);
    root.addAndMakeVisible (screen);
    auto pct = [] (float v) { return juce::String (juce::roundToInt (v * 100)) + "%"; };
    auto st = [] (float v) { return (v > 0 ? "+" : "") + juce::String (juce::roundToInt (v)) + " st"; };
    auto st1 = [] (float v) { return std::abs (v) < 0.05f ? juce::String ("0.0 st") : (v > 0 ? "+" : "") + juce::String (v, 1) + " st"; };
    auto db = [] (float v) { return v <= -59.9f ? juce::String ("MUTO") : (v > 0 ? "+" : "") + juce::String (v, 1) + " dB"; };
    struct KD { const char* id; const char* l; int card; float x, y, w, h; std::function<juce::String (float)> f; int steps; bool bip; const char* tip; };
    const std::vector<KD> kd = {
        { "pitch", "PITCH", 0, 470, 440, 72, 116, st, 25, true, "Altezza della voce senza cambiare la durata" },
        { "tape", "NASTRO", 0, 550, 440, 72, 116, st1, 0, true, "Nastro rallentato o accelerato: cambia altezza e durata insieme (gigante o vocina)" },
        { "robot", "ROBOT", 0, 506, 560, 84, 120, pct, 0, false, "Voce cyborg: modulazione ad anello e pettine metallico" },
        { "radio", "RADIO", 1, 646, 440, 72, 116, pct, 0, false, "Radio pirata / telefono: banda stretta, saturata, con fruscio" },
        { "drive", "DRIVE", 1, 726, 440, 72, 116, pct, 0, false, "Saturazione" },
        { "crush", "CRUSH", 1, 682, 560, 84, 120, pct, 0, false, "Bit e campionamento ridotti" },
        { "stutter", "STUTTER", 2, 822, 440, 72, 116, [] (float v) { return v < 0.5f ? juce::String ("OFF") : "x" + juce::String (juce::roundToInt (v)); }, 9, false, "J-J-J-Jackpot: il primo sedicesimo ripetuto prima della frase" },
        { "swell", "SWELL", 2, 902, 440, 72, 116, pct, 0, false, "Riverbero al contrario che sale fino all'inizio del tag" },
        { "echo", "ECO", 3, 996, 432, 78, 84, pct, 0, false, "Eco a tempo (lanciata)" },
        { "echoDiv", "TEMPO", 3, 1076, 432, 78, 84, [] (float v) { return juce::String (echoDivName (juce::roundToInt (v))); }, 5, false, "Tempo dell'eco, a tempo col BPM della sessione" },
        { "echoFb", "RIPETIZ.", 3, 996, 516, 78, 84, pct, 0, false, "Quante ripetizioni" },
        { "verb", "RIVERBERO", 3, 1076, 516, 78, 84, pct, 0, false, "Riverbero" },
        { "size", "STANZA", 3, 996, 600, 78, 84, pct, 0, false, "Grandezza della stanza" },
        { "width", "LARGO", 3, 1076, 600, 78, 84, pct, 0, false, "Larghezza stereo" },
        { "tag", "TAG", -1, 1060, 104, 50, 78, db, 0, false, "Volume del tag" },
        { "dry", "IN", -1, 1112, 104, 52, 78, db, 0, false, "Volume della traccia che entra (la voce passa sempre)" },
    };
    for (auto& k : kd)
    {
        juce::Colour cap = Y, on = Y;
        if (k.card == 1) { cap = K; on = Y; } else if (k.card == 2) { cap = M; on = M; } else if (k.card == 3) { cap = P2; on = M; } else if (k.card < 0) { cap = M; on = M; }
        auto kn = std::make_unique<Knob> (*this, k.id, k.l, cap, on, k.f);
        kn->steps = k.steps; kn->bipolar = k.bip; kn->setTooltip (k.tip);
        kn->labelInk = k.card == 2 ? Y : K; kn->valueBox = k.card == 2 ? Y : K; kn->valueInk = k.card == 2 ? K : Y;
        kn->setBounds (juce::Rectangle<float> (k.x, k.y, k.w, k.h).toNearestInt());
        root.addAndMakeVisible (*kn); knobs.push_back (std::move (kn));
    }
    root.addAndMakeVisible (sizeChip);
    {
        auto userFiles = [] { auto f = Processor::presetFolder().findChildFiles (juce::File::findFiles, false, "*.tagforge"); f.sort(); return f; };
        browser.entries = [this, userFiles]
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
        browser.title = "TAG // ARCHIVIO"; browser.sub = "FIRME IN MEMORIA";
        browser.titleFont = bun (30.0f); browser.nameFont = arc (22.0f); browser.monoFont = mon (11.0f);
        browser.accent = Y; browser.accent2 = M; browser.ink = WH; browser.dim = juce::Colour (0xff8a8296);
        browser.bg0 = juce::Colour (0xf50a0810); browser.bg1 = juce::Colour (0xf5161020);
        root.addChildComponent (browser);
    }
    setWantsKeyboardFocus (true);
    scaler.setup();
    startTimerHz (30);
}
Editor::~Editor() { stopTimer(); }
void Editor::resized()
{
    const float s = scaler.update();
    root.setBounds (0, 0, W, H); root.setTransform (juce::AffineTransform::scale (s));
    screen.setBounds (456, 190, 708, 194);
    sizeChip.setBounds (1104, 701, 58, 15);
    browser.setBounds (0, 0, W, H);
}
bool Editor::keyPressed (const juce::KeyPress& k)
{
    if (k.getModifiers().isCommandDown() && (k.getKeyCode() == 'Z' || k.getKeyCode() == 'z')) { if (k.getModifiers().isShiftDown()) proc.undo.redo(); else proc.undo.undo(); root.repaint(); return true; }
    return false;
}
void Editor::click (int h, const juce::MouseEvent& e)
{
    juce::ignoreUnused (e);
    switch (h)
    {
        case H_PREV: case H_NEXT:
        {
            const int n = (int) presets().size(); proc.loadPreset ((juce::jmax (0, proc.preset) + (h == H_NEXT ? 1 : n - 1)) % n); break;
        }
        case H_NAME: presetMenu(); break;
        case H_SAVE: askSave(); break;
        case H_UNDO: proc.undo.undo(); break;
        case H_REDO: proc.undo.redo(); break;
        case H_REC: if (proc.recording()) proc.stopRec(); else proc.startRec(); break;
        case H_PLAY: proc.play(); break;
        case H_LOAD:
        {
            chooser = std::make_unique<juce::FileChooser> ("Scegli la frase", juce::File::getSpecialLocation (juce::File::userMusicDirectory), "*.wav;*.aif;*.aiff;*.flac;*.mp3;*.ogg;*.m4a");
            juce::Component::SafePointer<Editor> safe (this);
            chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [safe] (const juce::FileChooser& fc)
            {
                if (safe == nullptr || fc.getResult() == juce::File()) return;
                if (safe->proc.loadFile (fc.getResult())) { safe->flash = "CARICATA: " + fc.getResult().getFileName(); safe->flashT = 90; }
            });
            break;
        }
        case H_EXPORT:
        {
            const auto f = proc.exportTag();
            if (f.existsAsFile()) { flash = "ESPORTATO: " + f.getFullPathName(); flashT = 150; f.revealToUser(); }
            else { flash = "niente da esportare: registra prima una frase"; flashT = 90; }
            break;
        }
        case H_NOTE:
        {
            juce::PopupMenu m; m.addSectionHeader ("QUALE NOTA FA PARTIRE IL TAG");
            const int cur = juce::roundToInt (proc.value ("note"));
            m.addItem (1000, "TUTTE le note", true, cur < 0);
            for (int oct = 0; oct < 9; ++oct) { juce::PopupMenu sub; for (int k = 0; k < 12; ++k) { const int n = (oct + 1) * 12 + k; if (n < 128) sub.addItem (1001 + n, noteName (n), true, cur == n); } m.addSubMenu ("Ottava " + juce::String (oct - 1), sub); }
            juce::Component::SafePointer<Editor> safe (this);
            m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&root), [safe] (int r) { if (safe == nullptr || r < 1000) return; safe->proc.setP ("note", r == 1000 ? -1.0f : (float) (r - 1001)); safe->root.repaint(); });
            break;
        }
        case H_REV: proc.undo.beginNewTransaction(); proc.setP ("reverse", proc.value ("reverse") > 0.5f ? 0.0f : 1.0f); break;
        default: break;
    }
    root.repaint(); screen.repaint();
}
void Editor::presetMenu() { browser.open(); }
void Editor::askSave()
{
    auto* w = new juce::AlertWindow ("SALVA IL PRESET", "Nome del preset (solo il carattere del tag, non la frase)", juce::MessageBoxIconType::NoIcon, this);
    w->addTextEditor ("n", proc.currentName()); w->addButton ("SALVA", 1, juce::KeyPress (juce::KeyPress::returnKey)); w->addButton ("ANNULLA", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    juce::Component::SafePointer<Editor> safe (this);
    w->enterModalState (true, juce::ModalCallbackFunction::create ([safe, w] (int r)
    {
        if (safe != nullptr && r == 1) { const auto n = w->getTextEditorContents ("n").trim(); if (n.isNotEmpty() && safe->proc.saveUserPreset (n)) { safe->flash = "PRESET SALVATO: " + n; safe->flashT = 90; } safe->root.repaint(); }
    }), true);
}
void Editor::timerCallback()
{
    ++tick;
    const float l = proc.uiLevel.load();
    const float nl = juce::jmax (l, lvl * 0.88f);
    const bool rec = proc.recording();
    if (rec)
    {
        recPulse = 0.5f + 0.5f * std::sin ((float) tick * 0.25f);
        if (proc.recSeconds() >= 12.0f) proc.stopRec();
    }
    if (std::abs (nl - lvl) > 0.004f || rec) { lvl = nl; root.repaint (150, 260, 180, 110); }
    if (rec || recPulse > 0) { root.repaint (hotR[H_REC].expanded (8).toNearestInt()); root.repaint (40, 220, 400, 200); if (! rec) recPulse = 0; screen.repaint(); }
    // sbuffi dai filtri mentre il tag suona
    if (lvl > 0.06f && tick % 3 == 0 && puffs.size() < 24)
        for (int s = 0; s < 2; ++s) { juce::Random r; puffs.push_back ({ s == 0 ? 170.0f : 298.0f, 440.0f, (s == 0 ? -1.0f : 1.0f) * (0.6f + r.nextFloat()), -0.4f - r.nextFloat() * 0.8f, 5 + lvl * 10, 0.9f }); }
    if (! puffs.empty())
    {
        for (auto& p : puffs) { p.x += p.vx; p.y += p.vy; p.r += 0.35f; p.a -= 0.035f; }
        puffs.erase (std::remove_if (puffs.begin(), puffs.end(), [] (const Puff& p) { return p.a <= 0; }), puffs.end());
        root.repaint (40, 340, 400, 160);
    }
    screen.refresh();
    static float lastPos = -2; const float pos = proc.uiPlayPos.load();
    if (pos != lastPos) { lastPos = pos; screen.repaint(); }
    if (flashT > 0 && --flashT == 0) screen.repaint();
    if (proc.preset != lastPreset || proc.userName != lastUser) { lastPreset = proc.preset; lastUser = proc.userName; root.repaint (hotR[H_NAME].expanded (4).toNearestInt()); }
    if (tick % 6 == 0) { root.repaint (hotR[H_UNDO].getUnion (hotR[H_REDO]).toNearestInt()); root.repaint (hotR[H_NOTE].expanded (6).toNearestInt()); root.repaint (hotR[H_REV].expanded (4).toNearestInt()); root.repaint (hotR[H_PLAY].expanded (6).toNearestInt()); root.repaint (hotR[H_DRAG].expanded (6).toNearestInt()); }
    for (auto& k : knobs) { const float v = k->prm->getValue(); if (v != k->last) { k->last = v; k->repaint(); } }
}
}
