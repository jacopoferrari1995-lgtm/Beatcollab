#include "PreEditor.h"

namespace jpre
{
using namespace jkit;
namespace A = alook::A;

// tasto quadrato crema con cornice cromata (premuto = piu basso)
static void pushKey (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& t, bool sel, bool lamp = false, float font = 10.5f)
{
    g.setColour (juce::Colours::black.withAlpha (0.5f)); g.fillRect (r.expanded (3).translated (0, 1));
    juce::ColourGradient bz (juce::Colour (0xffe9e6df), r.getX(), r.getY() - 3, juce::Colour (0xff6b6861), r.getX(), r.getBottom() + 3, false); g.setGradientFill (bz); g.fillRect (r.expanded (3));
    auto face = r.reduced (sel ? 3.0f : 1.5f).translated (0, sel ? 1.5f : 0.0f);
    g.setColour (juce::Colour (0xff1a1714)); g.fillRect (r);
    juce::ColourGradient fc (sel ? juce::Colour (0xffd8cfb8) : juce::Colour (0xfff5f0e2), face.getX(), face.getY(), sel ? juce::Colour (0xffb9ae94) : juce::Colour (0xffd9d0b8), face.getX(), face.getBottom(), false);
    g.setGradientFill (fc); g.fillRect (face);
    if (! sel) { g.setColour (juce::Colours::white.withAlpha (0.6f)); g.drawHorizontalLine ((int) face.getY(), face.getX(), face.getRight()); g.setColour (juce::Colours::black.withAlpha (0.25f)); g.drawHorizontalLine ((int) face.getBottom() - 1, face.getX(), face.getRight()); }
    alook::engrave (g, t, lamp ? face.withTrimmedRight (12) : face, alook::serif (font), juce::Justification::centred, sel ? A::maroon : A::ink);
    if (lamp) alook::jewelImg (g, face.getRight() - 8, face.getCentreY(), 4.0f, 0, true);
}

// ======================================================================= grafico
Graph::Graph (Editor& e) : ed (e) { win.resize (4096); buf.resize (8192); spec.assign (2048, -100.0f); for (int i = 0; i < 4096; ++i) win[(size_t) i] = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * i / 4095.0f); }
float Graph::xOf (float f) const { return 40.0f + (getWidth() - 50.0f) * std::log (f / 20.0f) / std::log (1000.0f); }
float Graph::fOf (float x) const { return 20.0f * std::pow (1000.0f, juce::jlimit (0.0f, 1.0f, (x - 40.0f) / (getWidth() - 50.0f))); }
float Graph::yOf (float db) const { return 18.0f + (getHeight() - 36.0f) * (0.5f - db / 48.0f); }
float Graph::dbOf (float y) const { return (0.5f - (y - 18.0f) / (getHeight() - 36.0f)) * 48.0f; }
juce::Point<float> Graph::pointOf (int k) const
{
    auto& P = ed.proc;
    if (k < 4) { const juce::String p = "b" + juce::String (k + 1); return { xOf (P.valueS (p + "f")), yOf (P.valueS (p + "g")) }; }
    if (k == 4) return { xOf (P.value ("hp")), yOf (0) };
    return { xOf (P.value ("lp")), yOf (0) };
}
int Graph::hit (juce::Point<float> p) const
{
    if (ed.coreMode() != 0) return -1;
    int best = -1; float bd = 14; for (int k = 0; k < 6; ++k) { const float d = p.getDistanceFrom (pointOf (k)); if (d < bd) { bd = d; best = k; } } return best;
}
void Graph::tick()
{
    ed.proc.ring.read (buf.data(), 4096);
    for (int i = 0; i < 4096; ++i) buf[(size_t) i] *= win[(size_t) i];
    std::fill (buf.begin() + 4096, buf.end(), 0.0f);
    fft.performFrequencyOnlyForwardTransform (buf.data());
    for (int i = 0; i < 2048; ++i) { const float db = juce::Decibels::gainToDecibels (buf[(size_t) i] / 1024.0f, -100.0f); spec[(size_t) i] = db > spec[(size_t) i] ? db : spec[(size_t) i] * 0.85f + db * 0.15f; }
    repaint();
}
void Graph::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff111110)); g.fillRoundedRectangle (b, 14);
    auto s = b.reduced (9);
    juce::ColourGradient cg (juce::Colour (0xff132a1b), s.getCentreX(), s.getCentreY(), juce::Colour (0xff050c08), s.getX(), s.getY(), true); g.setGradientFill (cg); g.fillRoundedRectangle (s, 18);
    const auto ph = A::phos;
    const double sr = juce::jmax (8000.0, ed.proc.sampleRate());
    g.setFont (alook::typed (8.5f));
    for (float f : { 30.f, 50.f, 100.f, 200.f, 500.f, 1000.f, 2000.f, 5000.f, 10000.f, 20000.f })
    { g.setColour (ph.withAlpha (0.07f)); g.drawVerticalLine ((int) xOf (f), s.getY() + 6, s.getBottom() - 6); g.setColour (ph.withAlpha (0.35f)); g.drawText (f >= 1000 ? juce::String ((int) (f / 1000)) + "k" : juce::String ((int) f), juce::Rectangle<float> (xOf (f) - 16, s.getBottom() - 16, 32, 12), juce::Justification::centred); }
    for (int db = -18; db <= 18; db += 6)
    { g.setColour (ph.withAlpha (db == 0 ? 0.18f : 0.06f)); g.drawHorizontalLine ((int) yOf ((float) db), 40, s.getRight() - 6); g.setColour (ph.withAlpha (0.35f)); g.drawText ((db > 0 ? "+" : "") + juce::String (db), juce::Rectangle<float> (12, yOf ((float) db) - 6, 24, 12), juce::Justification::centredRight); }
    {   // spettro in uscita
        juce::Path sp; bool st = false;
        for (int x = 40; x < getWidth() - 10; x += 2)
        {
            const float f = fOf ((float) x); const int bin = juce::jlimit (1, 2047, (int) (f / sr * 4096)); const float db = spec[(size_t) bin] + 6.0f * std::log2 (juce::jmax (1.0f, f / 1000.0f)) * 0.5f;
            const float y = juce::jlimit (s.getY() + 4, s.getBottom() - 8, s.getBottom() - 8 - (db + 90.0f) / 90.0f * (s.getHeight() - 16));
            if (! st) { sp.startNewSubPath ((float) x, s.getBottom() - 8); st = true; } sp.lineTo ((float) x, y);
        }
        sp.lineTo (s.getRight() - 10, s.getBottom() - 8); sp.closeSubPath();
        g.setColour (ph.withAlpha (0.07f)); g.fillPath (sp);
    }
    const auto S = ed.proc.settings();
    const bool eqMode = S.cMode == 0;
    jkit::Bq hs[4], ls[4]; Core::staticFilters (S, sr, hs, ls);
    jkit::Bq bandF[4]; for (int k = 0; k < 4; ++k) { const float cut = S.b[k].dyn ? ed.proc.eng.core.gr[k].load() : S.b[k].g; if (cut < -0.05f) bandF[k].peak (sr, S.b[k].f, S.b[k].q, cut); else bandF[k].flat(); }
    juce::Path pc, pr, pall; bool first = true;
    for (int x = 40; x < getWidth() - 10; x += 2)
    {
        const double f = fOf ((float) x);
        double dc = 0, dr = 0;
        if (S.cOn && eqMode)
        {
            if (S.hpHz > 20.5f) for (auto& q : hs) dc += q.magDb (f, sr);
            if (S.lpHz < 19900) for (auto& q : ls) dc += q.magDb (f, sr);
            for (int k = 0; k < 4; ++k) if (S.b[k].ms != 2) dc += bandF[k].magDb (f, sr);   // la curva mostra il mid (o lo stereo)
        }
        for (int k = 0; k < kSlots; ++k) dr += Slot::curveDb (S.slot[k], f, sr);
        const float yc = yOf ((float) juce::jlimit (-30.0, 30.0, dc)), yr = yOf ((float) juce::jlimit (-30.0, 30.0, dr)), ya = yOf ((float) juce::jlimit (-30.0, 30.0, dc + dr));
        if (first) { pc.startNewSubPath ((float) x, yc); pr.startNewSubPath ((float) x, yr); pall.startNewSubPath ((float) x, ya); first = false; }
        else { pc.lineTo ((float) x, yc); pr.lineTo ((float) x, yr); pall.lineTo ((float) x, ya); }
    }
    const juce::Colour subC (0xffffb347), rackC (0xff7fd8ff), resC (0xffff6b5e);
    if (S.cOn && ! eqMode)
    {   // riduzione delle risonanze: area rossa che scende dallo zero
        juce::Path rp; rp.startNewSubPath (xOf (20), yOf (0));
        for (int v = 0; v < Resonance::kView; ++v) { const float f = 20.0f * std::pow (1000.0f, (float) v / (Resonance::kView - 1)); rp.lineTo (xOf (f), yOf (-juce::jmin (24.0f, ed.proc.eng.core.res.view[v].load()))); }
        rp.lineTo (xOf (20000), yOf (0)); rp.closeSubPath();
        g.setColour (resC.withAlpha (0.28f)); g.fillPath (rp); g.setColour (resC); g.strokePath (rp, juce::PathStrokeType (1.2f));
        // zona di lavoro
        g.setColour (resC.withAlpha (0.06f)); g.fillRect (juce::Rectangle<float>::leftTopRightBottom (xOf (S.rLo), s.getY() + 22, xOf (S.rHi), s.getBottom() - 20));
        g.setColour (resC.withAlpha (0.5f)); for (float f : { S.rLo, S.rHi }) g.drawVerticalLine ((int) xOf (f), s.getY() + 22, s.getBottom() - 20);
    }
    if (S.cOn && eqMode) { g.setColour (subC.withAlpha (0.55f)); g.strokePath (pc, juce::PathStrokeType (1.2f)); }
    g.setColour (rackC.withAlpha (0.55f)); g.strokePath (pr, juce::PathStrokeType (1.2f));
    g.setColour (ph.withAlpha (0.25f)); g.strokePath (pall, juce::PathStrokeType (6.0f)); g.setColour (ph.brighter (0.3f)); g.strokePath (pall, juce::PathStrokeType (1.8f));
    if (eqMode)
        for (int k = 0; k < 6; ++k)
        {
            const auto p = pointOf (k); const bool sel = k == drag || k == hover || (k < 4 && k == ed.band) || (k >= 4 && ed.band == 4);
            const auto c = S.cOn ? subC : ph.withAlpha (0.4f);
            g.setColour (c.withAlpha (sel ? 0.35f : 0.12f)); g.fillEllipse (p.x - 11, p.y - 11, 22, 22);
            g.setColour (c); g.drawEllipse (p.x - 8, p.y - 8, 16, 16, sel ? 1.8f : 1.1f);
            if (k < 4 && S.b[k].dyn) { g.drawEllipse (p.x - 12, p.y - 12, 24, 24, 0.8f); const float gr = ed.proc.eng.core.gr[k].load(); if (gr < -0.1f) { g.setColour (resC); g.drawLine (p.x, yOf (0), p.x, yOf (gr), 2.0f); } }
            g.setFont (alook::typed (8.5f)); g.setColour (c);
            g.drawText (k < 4 ? juce::String (k + 1) : k == 4 ? juce::String ("HP") : juce::String ("LP"), juce::Rectangle<float> (p.x - 10, p.y - 7, 20, 14), juce::Justification::centred);
            if (k < 4 && S.b[k].ms > 0) g.drawText (S.b[k].ms == 1 ? "M" : "S", juce::Rectangle<float> (p.x + 8, p.y - 18, 14, 12), juce::Justification::centred);
        }
    g.setFont (alook::typed (8.5f)); g.setColour (ph.withAlpha (0.5f));
    g.drawText (eqMode ? juce::String::fromUTF8 ("trascina i punti = taglia  \xc2\xb7  rotella = Q  \xc2\xb7  doppio clic = azzera")
                       : juce::String ("rosso = quanto viene tolto, frequenza per frequenza, in tempo reale  (media " + juce::String (ed.proc.eng.core.res.total.load(), 1) + " dB)"),
                juce::Rectangle<float> (48, s.getY() + 6, 520, 14), juce::Justification::centredLeft);
    auto leg = [&] (float x, const char* t, juce::Colour c) { g.setColour (c); g.fillRect (x, s.getY() + 12, 10.0f, 2.0f); g.setColour (ph.withAlpha (0.5f)); g.drawText (t, juce::Rectangle<float> (x + 14, s.getY() + 6, 90, 14), juce::Justification::centredLeft); };
    leg (s.getRight() - 240, "CENTRO", eqMode ? subC : resC); leg (s.getRight() - 160, "RACK", rackC); leg (s.getRight() - 90, "TOTALE", ph);
    g.setColour (juce::Colours::black.withAlpha (0.12f)); for (float y = s.getY(); y < s.getBottom(); y += 3) g.drawHorizontalLine ((int) y, s.getX(), s.getRight());
    juce::ColourGradient gl (juce::Colours::white.withAlpha (0.07f), s.getX(), s.getY(), juce::Colours::transparentWhite, s.getX(), s.getCentreY(), false); g.setGradientFill (gl); g.fillRoundedRectangle (s.withHeight (s.getHeight() * 0.45f), 18);
}
void Graph::mouseMove (const juce::MouseEvent& e) { const int h = hit (e.position); if (h != hover) { hover = h; repaint(); } }
void Graph::mouseDown (const juce::MouseEvent& e)
{
    drag = hit (e.position); if (drag < 0) return;
    ed.turnOnCore(); ed.selectBand (drag < 4 ? drag : 4);
    mouseDrag (e);
}
void Graph::mouseDrag (const juce::MouseEvent& e)
{
    if (drag < 0) return;
    auto set = [this] (const juce::String& id, float v) { auto* p = ed.P (id); p->setValueNotifyingHost (p->convertTo0to1 (v)); };
    const float f = fOf (e.position.x);
    if (drag < 4) { const juce::String p = "b" + juce::String (drag + 1); set (p + "f", f); set (p + "g", juce::jlimit (-24.0f, 0.0f, dbOf (e.position.y))); }
    else if (drag == 4) set ("hp", juce::jlimit (20.0f, 1000.0f, f));
    else set ("lp", juce::jlimit (1000.0f, 20000.0f, f));
}
void Graph::mouseDoubleClick (const juce::MouseEvent& e)
{
    const int k = hit (e.position); if (k < 0) return;
    if (k < 4) ed.P ("b" + juce::String (k + 1) + "g")->setValueNotifyingHost (ed.P ("b" + juce::String (k + 1) + "g")->getDefaultValue());
    else if (k == 4) ed.P ("hp")->setValueNotifyingHost (0.0f); else ed.P ("lp")->setValueNotifyingHost (1.0f);
}
void Graph::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    const int k = hit (e.position); if (k < 0 || k >= 4) return;
    auto* p = ed.P ("b" + juce::String (k + 1) + "q"); p->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, p->getValue() + (w.deltaY > 0 ? 0.03f : -0.03f)));
}

// ======================================================================= modulo del rack
SlotStrip::SlotStrip (Editor& e, int s) : ed (e), slot (s)
{
    for (int i = 0; i < kSlotParams; ++i)
    {
        auto* p = ed.P (sid (s, ("p" + juce::String (i + 1)).toRawUTF8()));
        knobs[i] = std::make_unique<alook::Knob> (p, "", 2); knobs[i]->numbers = false; knobs[i]->ticks = 10;
        knobs[i]->text = [this, i] { return specText (type(), i, knobs[i]->param->getValue()); };
        toggles[i] = std::make_unique<alook::Toggle> (p, "", "ON", "OFF");
        addChildComponent (*knobs[i]); addChildComponent (*toggles[i]);
    }
    refresh();
}
int SlotStrip::type() const { return juce::jlimit (0, kModTypes - 1, (int) ed.proc.valueS (sid (slot, "type"))); }
void SlotStrip::refresh()
{
    const int t = type(); shownType = t;
    const auto sp = modSpec (t);
    for (int i = 0; i < kSlotParams; ++i)
    {
        const auto& s = sp[(size_t) i]; const bool vis = t != 0 && ! hidden (s);
        knobs[i]->setVisible (vis && s.fmt != F_BOOL); toggles[i]->setVisible (vis && s.fmt == F_BOOL);
        if (vis) { knobs[i]->label = s.label; knobs[i]->style = s.cap; toggles[i]->label = s.label; knobs[i]->ticks = s.fmt == F_ITEMS ? s.items.size() - 1 : 10; }
    }
    setTooltip (modDesc (t));
    layout(); repaint();
}
void SlotStrip::layout()
{
    const int t = type(); if (t == 0) return;
    const auto sp = modSpec (t);
    const float cw = (getWidth() - 12) / 2.0f; const float top = 64, rh = (getHeight() - top - 8) / 5.0f;
    int n = 0;
    for (int i : displayOrder (t))
    {
        if (hidden (sp[(size_t) i])) { ++n; continue; }   // il buco tiene allineate le coppie
        const int r = n / 2, c = n % 2; ++n;
        auto cell = juce::Rectangle<float> (6 + c * cw, top + r * rh, cw, rh);
        if (sp[(size_t) i].fmt == F_BOOL) toggles[i]->setBounds (cell.reduced (2, rh * 0.2f).toNearestInt());
        else knobs[i]->setBounds (cell.reduced (2, 0).toNearestInt());
    }
}
void SlotStrip::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    const int t = type(); const bool byp = ed.proc.valueS (sid (slot, "byp")) > 0.5f;
    if (t == 0)
    {   // slot vuoto: un buco nel rack con le guide
        g.setColour (juce::Colour (0xff141312)); g.fillRoundedRectangle (b, 4);
        g.setColour (dropHover ? A::amberLamp.withAlpha (0.6f) : juce::Colours::white.withAlpha (0.08f)); g.drawRoundedRectangle (b.reduced (1), 4, dropHover ? 2.0f : 1.0f);
        for (float y : { 14.0f, b.getBottom() - 14 }) for (float x : { 12.0f, b.getRight() - 12 }) { g.setColour (juce::Colours::black.withAlpha (0.7f)); g.fillRoundedRectangle (x - 4, y - 3, 8, 6, 2); }
        g.setColour (A::chrome.withAlpha (0.55f)); g.setFont (alook::serif (30.0f)); g.drawText ("+", b.withHeight (b.getHeight() * 0.5f).translated (0, b.getHeight() * 0.18f), juce::Justification::centred);
        g.setFont (alook::serif (10.0f)); g.drawText ("SCEGLI MODULO", b.withTrimmedTop (b.getHeight() * 0.5f).withHeight (20), juce::Justification::centred);
        g.setFont (alook::typed (8.0f)); g.setColour (A::chrome.withAlpha (0.35f)); g.drawText ("SLOT " + juce::String (slot + 1), b.withTrimmedTop (b.getHeight() * 0.5f + 22).withHeight (14), juce::Justification::centred);
        return;
    }
    const bool pre = isPre (t);
    alook::faceplate (g, b, false, pre ? A::cream : juce::Colour (0xffd9d3c4));
    if (dropHover) { g.setColour (A::amberLamp); g.drawRoundedRectangle (b.reduced (1), 4, 2.5f); }
    // intestazione: spia (attivo/bypass), numero, maniglia, targa con il nome
    alook::jewelImg (g, jewelR().getCentreX(), jewelR().getCentreY(), 7, pre ? 1 : 2, ! byp);
    alook::engrave (g, juce::String (slot + 1) + (pre ? "  PREAMP" : "  EQ"), { 34, 6, b.getWidth() - 60, 20 }, alook::typed (8.5f), juce::Justification::centredLeft, A::inkSoft);
    g.setColour (A::ink.withAlpha (0.4f)); for (int k = 0; k < 3; ++k) g.fillRect (b.getRight() - 24.0f, 11.0f + k * 4.0f, 14.0f, 1.5f);   // maniglia per trascinare
    auto nr = nameR();
    g.setColour (juce::Colour (0xff1d1915)); g.fillRoundedRectangle (nr, 3);
    g.setFont (alook::typed (12.0f)); g.setColour (juce::Colour (0xfff2ede0).withAlpha (byp ? 0.4f : 0.92f)); g.drawText (modName (t), nr.withTrimmedRight (14), juce::Justification::centred);
    g.setColour (A::chrome.withAlpha (0.6f)); g.setFont (alook::serif (8.0f)); g.drawText (juce::String::fromUTF8 ("\xe2\x96\xbc"), nr.withTrimmedRight (5), juce::Justification::centredRight);
    if (byp) { g.setColour (A::cream.withAlpha (0.45f)); g.fillRect (b.withTrimmedTop (60)); alook::engrave (g, "BYPASS", b.withTrimmedTop (60).withHeight (30), alook::serif (11.0f), juce::Justification::centred, A::maroon); }
}
void SlotStrip::openMenu()
{
    juce::PopupMenu m, pm, em; const int cur = type();
    for (int i = 0; i < kPreModels; ++i) pm.addItem (1 + i, preInfo (i).name, true, cur == 1 + i);
    for (int i = 0; i < kEqModels; ++i) em.addItem (1 + kPreModels + i, eqName (i), true, cur == 1 + kPreModels + i);
    m.addSectionHeader ("SLOT " + juce::String (slot + 1));
    m.addSubMenu ("PREAMP", pm, true, nullptr, isPre (cur)); m.addSubMenu ("EQUALIZZATORI", em, true, nullptr, isEq (cur));
    m.addSeparator();
    if (cur != 0) { m.addItem (1000, ed.proc.valueS (sid (slot, "byp")) > 0.5f ? "Riattiva" : "Bypass"); m.addItem (1001, "Svuota lo slot"); }
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [this] (int r)
    {
        if (r <= 0) return;
        if (r == 1000) { auto* p = ed.P (sid (slot, "byp")); p->setValueNotifyingHost (p->getValue() > 0.5f ? 0.0f : 1.0f); }
        else if (r == 1001) ed.proc.setModule (slot, 0);
        else ed.proc.setModule (slot, r);
        refresh(); ed.repaint();
    });
}
void SlotStrip::mouseDown (const juce::MouseEvent& e)
{
    dragging = false;
    if (type() == 0 || nameR().contains (e.position)) { openMenu(); return; }
    if (jewelR().expanded (4).contains (e.position)) { auto* p = ed.P (sid (slot, "byp")); p->setValueNotifyingHost (p->getValue() > 0.5f ? 0.0f : 1.0f); repaint(); return; }
    if (e.position.y < 28) dragging = true;   // maniglia: trascina su un altro slot per scambiarli
}
void SlotStrip::mouseDrag (const juce::MouseEvent& e)
{
    if (! dragging) return;
    setMouseCursor (juce::MouseCursor::DraggingHandCursor);
    const int over = ed.slotAt (ed.getLocalPoint (this, e.getPosition()));
    for (auto& s : ed.strips) { const bool h = s->slot == over && over != slot; if (h != s->dropHover) { s->dropHover = h; s->repaint(); } }
}
void SlotStrip::mouseUp (const juce::MouseEvent& e)
{
    setMouseCursor (juce::MouseCursor::NormalCursor);
    for (auto& s : ed.strips) if (s->dropHover) { s->dropHover = false; s->repaint(); }
    if (! dragging) return; dragging = false;
    const int over = ed.slotAt (ed.getLocalPoint (this, e.getPosition()));
    if (over >= 0 && over != slot) { ed.proc.swapSlots (slot, over); for (auto& s : ed.strips) s->refresh(); ed.repaint(); }
}

// ======================================================================= editor
Editor::Editor (Processor& p) : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);
    const int black = 2, maroonCap = 0, greyCap = 1;
    auto knob = [&] (const juce::String& id, const juce::String& label, std::function<juce::String()> txt, int mode, int group, int cap = 2) -> alook::Knob*
    {
        auto k = std::make_unique<alook::Knob> (P (id), label, cap); k->text = std::move (txt);
        k->onTouch = [this] { turnOnCore(); };
        addAndMakeVisible (*k); knobs.push_back ({ std::move (k), mode, group }); return knobs.back().k.get();
    };
    auto val = [this] (const juce::String& id) { return proc.valueS (id); };
    // centro: EQ dinamico sottrattivo
    for (int b = 0; b < 4; ++b)
    {
        const juce::String pp = "b" + juce::String (b + 1);
        knob (pp + "f", "FREQUENCY", [val, pp] { return hzT (val (pp + "f")); }, 0, b, greyCap);
        knob (pp + "g", "CUT", [val, pp] { const float v = val (pp + "g"); return v > -0.05f ? juce::String ("0 dB") : juce::String (v, 1) + " dB"; }, 0, b, maroonCap);
        knob (pp + "q", "SHARP", [val, pp] { return "Q " + juce::String (val (pp + "q"), 1); }, 0, b, greyCap);
        knob (pp + "thr", "SOGLIA", [val, pp] { return juce::String (val (pp + "thr"), 1) + " dB"; }, 0, b, black);
        bandDyn.push_back (std::make_unique<alook::Toggle> (P (pp + "dyn"), "DINAMICO", "DYN", "FISSO")); addChildComponent (*bandDyn.back());
        bandDyn.back()->onPick = [this] { turnOnCore(); };
    }
    knob ("hp", "HIGH PASS", [this] { return proc.value ("hp") <= 20.5f ? juce::String ("OUT") : hzT (proc.value ("hp")); }, 0, 4, greyCap);
    knob ("lp", "LOW PASS", [this] { return proc.value ("lp") >= 19900 ? juce::String ("OUT") : hzT (proc.value ("lp")); }, 0, 4, greyCap);
    knob ("c_att", "ATTACCO", [this] { return juce::String (proc.value ("c_att"), 1) + " ms"; }, 0, 4, black);
    knob ("c_rel", "RILASCIO", [this] { return juce::String (juce::roundToInt (proc.value ("c_rel"))) + " ms"; }, 0, 4, black);
    auto rot = [this] (std::unique_ptr<alook::Rotary>& r, const char* id, const char* l, juce::StringArray it, float span)
    { r = std::make_unique<alook::Rotary> (P (id), l, it, span); r->knobFrac = 0.34f; r->labelGap = 13; r->fontSize = 8.5f; r->onTouch = [this] { turnOnCore(); }; addAndMakeVisible (*r); };
    rot (hpSl, "hpSl", "dB/OCT", { "12", "24", "48" }, 100); rot (lpSl, "lpSl", "dB/OCT", { "12", "24", "48" }, 100);
    rot (msRot, "b1ms", "CANALE", { "ST", "MID", "SIDE" }, 100);
    // centro: anti-risonanze
    knob ("r_depth", "PROFONDITA", [this] { return juce::String (juce::roundToInt (3 + proc.value ("r_depth") * 15)) + " dB max"; }, 1, -1, maroonCap)->big = true;
    knob ("r_sharp", "PRECISIONE", [this] { const float v = proc.value ("r_sharp"); return v < 0.33f ? juce::String ("LARGA") : v < 0.66f ? juce::String ("MEDIA") : juce::String ("STRETTA"); }, 1, -1, greyCap);
    knob ("r_speed", "VELOCITA", [this] { const float v = proc.value ("r_speed"); return v < 0.33f ? juce::String ("LENTA") : v < 0.66f ? juce::String ("MEDIA") : juce::String ("VELOCE"); }, 1, -1, greyCap);
    knob ("r_lo", "DA", [this] { return hzT (proc.value ("r_lo")); }, 1, -1, black);
    knob ("r_hi", "A", [this] { return hzT (proc.value ("r_hi")); }, 1, -1, black);
    rot (rMsRot, "r_ms", "CANALE", { "ST", "MID", "SIDE" }, 100);
    deltaTog = std::make_unique<alook::Toggle> (P ("r_delta"), "DELTA", "SOLO", "MIX"); addAndMakeVisible (*deltaTog);
    knob ("out", "OUTPUT", [this] { return dbT (proc.value ("out")); }, -1, -2, black);
    addAndMakeVisible (graph);
    for (int s = 0; s < kSlots; ++s) { strips.push_back (std::make_unique<SlotStrip> (*this, s)); addAndMakeVisible (*strips.back()); }
    addAndMakeVisible (sizeChip);
    scaler.setup();
    startTimerHz (30);
}
juce::Rectangle<float> Editor::chipR() const
{
    const int cp = (int) proc.value ("c_pos"); const float x0 = rackR.getX() + 12, sw = (rackR.getWidth() - 24 - 40 - 6 * 6) / 6.0f;
    return { x0 + cp * (sw + 6), rackR.getY() + 10, 40, rackR.getHeight() - 20 };
}
void Editor::layoutRack()
{
    const int cp = (int) proc.value ("c_pos"); const float x0 = rackR.getX() + 12, sw = (rackR.getWidth() - 24 - 40 - 6 * 6) / 6.0f;
    for (int s = 0; s < kSlots; ++s)
    {
        const float x = x0 + s * (sw + 6) + (s >= cp ? 46.0f : 0.0f);
        strips[(size_t) s]->setBounds (juce::Rectangle<float> (x, rackR.getY() + 10, sw, rackR.getHeight() - 20).toNearestInt());
    }
    lastPos = cp;
}
int Editor::slotAt (juce::Point<int> p0) const
{
    const auto p = (p0.toFloat() / scaler.s).toInt();   // i figli sono disposti in coordinate non scalate
    for (auto& s : strips) if (s->getBounds().contains (p)) return s->slot;
    return -1;
}
void Editor::layoutCore()
{
    const int mode = coreMode();
    for (auto& ks : knobs)
    {
        if (ks.group == -2) continue;
        const bool v = ks.mode == mode && (mode == 1 || ks.group == band);
        ks.k->setVisible (v);
    }
    for (int k = 0; k < 4; ++k) bandDyn[(size_t) k]->setVisible (mode == 0 && band == k);
    hpSl->setVisible (mode == 0 && band == 4); lpSl->setVisible (mode == 0 && band == 4); msRot->setVisible (mode == 0 && band < 4);
    rMsRot->setVisible (mode == 1); deltaTog->setVisible (mode == 1);
    auto K = [this] (const juce::String& id) -> alook::Knob* { for (auto& ks : knobs) if (ks.k->param == P (id)) return ks.k.get(); return nullptr; };
    auto at = [] (juce::Component* c, float x, float y, float w, float h) { c->setBounds (juce::Rectangle<float> (x, y, w, h).toNearestInt()); };
    if (mode == 0 && band < 4)
    {
        const juce::String pp = "b" + juce::String (band + 1);
        at (K (pp + "f"), 838, 198, 92, 92 + 30); at (K (pp + "g"), 940, 198, 92, 92 + 30); at (K (pp + "q"), 1042, 198, 92, 92 + 30);
        at (K (pp + "thr"), 838, 326, 76, 76 + 30); at (bandDyn[(size_t) band].get(), 914, 340, 100, 46);
        msRot->param = P (pp + "ms"); at (msRot.get(), 1012, 322, 74, 74);
    }
    else if (mode == 0)
    {
        at (K ("hp"), 836, 198, 84, 84 + 30); at (hpSl.get(), 918, 210, 66, 66); at (K ("lp"), 990, 198, 84, 84 + 30); at (lpSl.get(), 1072, 210, 66, 66);
        at (K ("c_att"), 870, 326, 76, 76 + 30); at (K ("c_rel"), 970, 326, 76, 76 + 30);
    }
    else
    {
        at (K ("r_depth"), 836, 160, 104, 104 + 30); at (K ("r_sharp"), 950, 168, 90, 90 + 30); at (K ("r_speed"), 1050, 168, 90, 90 + 30);
        at (K ("r_lo"), 838, 304, 80, 80 + 30); at (K ("r_hi"), 926, 304, 80, 80 + 30); at (rMsRot.get(), 1010, 310, 70, 70); at (deltaTog.get(), 1080, 322, 72, 44);
    }
    jlook::scaleChildren (*this, scaler.s);
    lastMode = mode;
}
void Editor::resized()
{
    scaler.update();
    graph.setBounds (28, 118, 794, 306);
    for (auto& ks : knobs) if (ks.k->param == P ("out")) ks.k->setBounds (1062, 12, 56, 56 + 30);
    sizeChip.setBounds (W - 76, H - 16, 58, 14);
    layoutRack();
    layoutCore();
}
void Editor::paint (juce::Graphics& g)
{
    g.addTransform (juce::AffineTransform::scale (scaler.s));
    g.fillAll (A::rack2);
    juce::ColourGradient bg (A::rack, 0, 0, A::rack2, 0, (float) H, false); g.setGradientFill (bg); g.fillRect (0, 0, W, H);
    jlook::grain (g, { 0, 0, W, H }, 0.05f);
    for (float x : { 4.0f, (float) W - 12.0f }) for (float y = 20; y < H; y += 44) { g.setColour (juce::Colours::black.withAlpha (0.6f)); g.fillRoundedRectangle (x, y, 8, 14, 3); }
    // unita 1: targa, preset, A/B, VU
    alook::faceplate (g, headR);
    alook::engrave (g, "JACKPOT", { 40, 18, 260, 34 }, alook::wide (24.0f), juce::Justification::centredLeft);
    alook::engrave (g, "PREAMP  &  EQ  HUB", { 42, 50, 300, 16 }, alook::serif (11.0f), juce::Justification::centredLeft, A::maroon);
    alook::engrave (g, juce::String::fromUTF8 ("MODEL JP-RACK  \xc2\xb7  6 SLOT  \xc2\xb7  CENTRO DINAMICO"), { 42, 66, 340, 14 }, alook::typed (8.5f), juce::Justification::centredLeft, A::inkSoft);
    {
        auto r = presetR; g.setColour (juce::Colours::black.withAlpha (0.4f)); g.fillRoundedRectangle (r.translated (0, 2), 3);
        juce::ColourGradient tg (juce::Colour (0xff27221e), r.getX(), r.getY(), juce::Colour (0xff15120f), r.getX(), r.getBottom(), false); g.setGradientFill (tg); g.fillRoundedRectangle (r, 3);
        g.setFont (alook::typed (13.0f)); g.setColour (juce::Colour (0xfff2ede0).withAlpha (0.9f));
        g.drawText (proc.getProgramName (proc.preset).toUpperCase(), r, juce::Justification::centred);
        alook::engrave (g, "PRESET", { r.getX(), r.getY() - 15, r.getWidth(), 12 }, alook::serif (8.0f));
        for (auto b : { prevR, nextR })
        {
            juce::ColourGradient cg (juce::Colour (0xffeeebe4), b.getX(), b.getY(), juce::Colour (0xff7a766e), b.getRight(), b.getBottom(), false); g.setGradientFill (cg); g.fillEllipse (b);
            g.setColour (A::ink); g.setFont (alook::serif (16.0f)); g.drawText (b == prevR ? juce::String::fromUTF8 ("\xe2\x80\xb9") : juce::String::fromUTF8 ("\xe2\x80\xba"), b, juce::Justification::centred);
        }
        alook::jewelImg (g, abA.getCentreX(), abA.getCentreY(), 10, 1, proc.ab.cur == 0);
        alook::jewelImg (g, abB.getCentreX(), abB.getCentreY(), 10, 1, proc.ab.cur == 1);
        alook::engrave (g, "A", { abA.getX(), abA.getY() - 14, abA.getWidth(), 12 }, alook::serif (9.0f)); alook::engrave (g, "B", { abB.getX(), abB.getY() - 14, abB.getWidth(), 12 }, alook::serif (9.0f));
        alook::engrave (g, "MEMORIA", { abA.getX() - 10, abA.getBottom() + 2, abB.getRight() - abA.getX() + 20, 12 }, alook::serif (7.5f));
    }
    alook::vuMeter (g, { 900, 20, 66, 68 }, vuIn, "IN");
    alook::vuMeter (g, { 976, 20, 66, 68 }, vuOut, "OUT");
    // unita 2: il centro
    alook::faceplate (g, coreR);
    const int mode = coreMode();
    alook::jewelImg (g, 852, 135, 10, 0, coreOn());
    alook::engrave (g, "CENTRO", { 836, 148, 40, 12 }, alook::serif (7.5f));
    pushKey (g, modeR (0), "EQ DINAMICO", mode == 0, false, 10.0f); pushKey (g, modeR (1), "ANTI-RISONANZE", mode == 1, false, 10.0f);
    g.setColour (A::ink.withAlpha (0.35f)); g.drawVerticalLine (828, coreR.getY() + 14, coreR.getBottom() - 14);
    if (mode == 0)
    {
        for (int k = 0; k < 5; ++k)
        {
            const bool act = k < 4 ? proc.valueS ("b" + juce::String (k + 1) + "g") < -0.05f && coreOn() : (proc.value ("hp") > 20.5f || proc.value ("lp") < 19900) && coreOn();
            pushKey (g, tabR (k), k < 4 ? juce::String (k + 1) : juce::String ("HP/LP"), band == k, act, 10.5f);
        }
        if (band < 4)
        {
            const bool solo = proc.valueS ("b" + juce::String (band + 1) + "solo") > 0.5f;
            pushKey (g, soloR(), "SOLO", solo, false, 10.0f);
            g.setColour (A::inkSoft); g.setFont (alook::serif (8.0f, false));
            g.drawFittedText ("In DYN taglia fino a CUT solo quando la zona supera la SOGLIA.", juce::Rectangle<int> (840, 412, 310, 14), juce::Justification::centredLeft, 1, 1.0f);
        }
        else
        {
            g.setColour (A::inkSoft); g.setFont (alook::serif (8.5f, false));
            g.drawFittedText ("Attacco e rilascio valgono per tutti i tagli dinamici.", juce::Rectangle<int> (1056, 330, 96, 80), juce::Justification::topLeft, 5, 1.0f);
        }
    }
    else
    {
        g.setColour (A::inkSoft); g.setFont (alook::serif (8.5f, false));
        g.drawFittedText ("Trova da solo le frequenze che fischiano o rimbombano e le abbassa solo quando escono. DELTA = senti solo cio che toglie.",
                          juce::Rectangle<int> (838, 410, 310, 24), juce::Justification::topLeft, 2, 1.0f);
    }
    // unita 3: il rack
    g.setColour (juce::Colour (0xff141312)); g.fillRoundedRectangle (rackR, 6);
    g.setColour (juce::Colours::white.withAlpha (0.06f)); g.drawRoundedRectangle (rackR, 6, 1.0f);
    {   // piastra del centro nel punto della catena
        const auto c = chipR(); alook::faceplate (g, c, false, juce::Colour (0xffb8ad94));
        alook::jewelImg (g, c.getCentreX(), c.getY() + 22, 7, 0, coreOn());
        g.saveState(); g.addTransform (juce::AffineTransform::rotation (-juce::MathConstants<float>::halfPi, c.getCentreX(), c.getCentreY()));
        alook::engrave (g, juce::String ("CENTRO  ") + (mode == 0 ? "EQ DINAMICO" : "ANTI-RISONANZE"), juce::Rectangle<float> (c.getCentreX() - 150, c.getCentreY() - 8, 300, 16), alook::serif (10.0f));
        g.restoreState();
        for (int k = 0; k < 2; ++k)
        {
            auto r = juce::Rectangle<float> (c.getX() + 6, k == 0 ? c.getBottom() - 70 : c.getBottom() - 36, 28, 28);
            juce::ColourGradient cg (juce::Colour (0xffeeebe4), r.getX(), r.getY(), juce::Colour (0xff7a766e), r.getRight(), r.getBottom(), false); g.setGradientFill (cg); g.fillEllipse (r);
            g.setColour (A::ink); g.setFont (alook::serif (14.0f)); g.drawText (k == 0 ? juce::String::fromUTF8 ("\xe2\x80\xb9") : juce::String::fromUTF8 ("\xe2\x80\xba"), r, juce::Justification::centred);
        }
    }
    // frecce del segnale tra i moduli
    for (auto& s : strips)
    {
        const auto r = s->getBounds().toFloat(); juce::Path a; a.addTriangle (r.getRight() + 1, rackR.getCentreY() - 5, r.getRight() + 6, rackR.getCentreY(), r.getRight() + 1, rackR.getCentreY() + 5);
        g.setColour (A::chrome.withAlpha (0.5f)); g.fillPath (a);
    }
    alook::engrave (g, juce::String::fromUTF8 ("RACK  \xc2\xb7  clic sulla targa = scegli il modulo  \xc2\xb7  trascina la maniglia = cambia ordine  \xc2\xb7  spia = bypass"),
                    { rackR.getX(), rackR.getBottom() + 0, rackR.getWidth(), 10 }, alook::typed (7.5f), juce::Justification::centred, A::chrome.withAlpha (0.5f));
}
void Editor::mouseDown (const juce::MouseEvent& e0)
{
    const auto e = e0.withNewPosition (e0.position / scaler.s);
    if (juce::Rectangle<float> (836, 120, 34, 40).contains (e.position)) { auto* p = P ("c_on"); p->setValueNotifyingHost (p->getValue() > 0.5f ? 0.0f : 1.0f); repaint(); return; }
    for (int k = 0; k < 2; ++k) if (modeR (k).contains (e.position)) { auto* p = P ("c_mode"); p->setValueNotifyingHost (p->convertTo0to1 ((float) k)); turnOnCore(); layoutCore(); repaint(); return; }
    if (coreMode() == 0)
    {
        for (int k = 0; k < 5; ++k) if (tabR (k).contains (e.position)) { selectBand (k); return; }
        if (band < 4 && soloR().contains (e.position)) { auto* p = P ("b" + juce::String (band + 1) + "solo"); const bool on = p->getValue() < 0.5f; for (int k = 0; k < 4; ++k) P ("b" + juce::String (k + 1) + "solo")->setValueNotifyingHost (0.0f); p->setValueNotifyingHost (on ? 1.0f : 0.0f); turnOnCore(); repaint(); return; }
    }
    {   // sposta il centro nella catena
        const auto c = chipR();
        auto* pp = P ("c_pos"); const int cp = (int) proc.value ("c_pos");
        if (juce::Rectangle<float> (c.getX(), c.getBottom() - 72, c.getWidth(), 32).contains (e.position) && cp > 0) { pp->setValueNotifyingHost (pp->convertTo0to1 ((float) (cp - 1))); layoutRack(); repaint(); return; }
        if (juce::Rectangle<float> (c.getX(), c.getBottom() - 38, c.getWidth(), 32).contains (e.position) && cp < kSlots) { pp->setValueNotifyingHost (pp->convertTo0to1 ((float) (cp + 1))); layoutRack(); repaint(); return; }
        if (c.contains (e.position)) { auto* p = P ("c_on"); p->setValueNotifyingHost (p->getValue() > 0.5f ? 0.0f : 1.0f); repaint(); return; }
    }
    const int n = (int) presets().size();
    if (abA.expanded (4).contains (e.position)) { if (proc.ab.cur != 0) proc.ab.toggle (proc); for (auto& s : strips) s->refresh(); repaint(); return; }
    if (abB.expanded (4).contains (e.position)) { if (proc.ab.cur != 1) proc.ab.toggle (proc); for (auto& s : strips) s->refresh(); repaint(); return; }
    if (prevR.contains (e.position)) { proc.loadPreset ((proc.preset + n - 1) % n); repaint(); return; }
    if (nextR.contains (e.position)) { proc.loadPreset ((proc.preset + 1) % n); repaint(); return; }
    if (presetR.contains (e.position))
    {
        jkit::PresetBar pb; pb.preset = presetR.toNearestInt(); pb.ab = {}; pb.prev = {}; pb.next = {};
        pb.click (*this, proc, presets(), e.getPosition(), localAreaToGlobal (scaler.map (presetR.toNearestInt())));
    }
}
void Editor::timerCallback()
{
    graph.tick();
    if (coreMode() != lastMode) { layoutCore(); repaint(); }
    if ((int) proc.value ("c_pos") != lastPos) { layoutRack(); jlook::scaleChildren (*this, scaler.s); repaint(); }
    for (auto& s : strips) if (s->type() != s->shownType) { s->refresh(); jlook::scaleChildren (*this, scaler.s); }
    for (auto& ks : knobs) { const bool lit = ks.group == -2 || coreOn(); const float v = ks.k->param->getValue() + (lit ? 0.0f : 9.0f); if (lit != ks.k->lit || v != ks.k->shown) { ks.k->lit = lit; ks.k->shown = v; ks.k->repaint(); } }
    hpSl->lit = lpSl->lit = msRot->lit = rMsRot->lit = coreOn();
    auto vu = [] (float pk) { return juce::Decibels::gainToDecibels (pk, -60.0f) + 18.0f - 3.0f; };
    vuIn += (vu (proc.eng.inPk.load()) - vuIn) * 0.3f; vuOut += (vu (proc.eng.outPk.load()) - vuOut) * 0.3f;
    if (++tick % 2 == 0) repaint (scaler.map ({ 890, 10, 160, 86 }));
    if (tick % 3 == 0)
    {
        hpSl->repaint(); lpSl->repaint(); msRot->repaint(); rMsRot->repaint(); deltaTog->repaint(); for (auto& t : bandDyn) t->repaint();
        for (auto& s : strips) { for (auto& k : s->knobs) if (k->isVisible()) k->repaint(); for (auto& t : s->toggles) if (t->isVisible()) t->repaint(); }
    }
    if (tick % 15 == 0) repaint();
}
}
