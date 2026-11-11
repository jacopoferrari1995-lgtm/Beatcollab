#include "MixEditor.h"

namespace jmx
{
namespace A = alook::A;
static const juce::Colour kBg (0xff151514), kPanel (0xff1e1d1b), kLine (0xff34322e), kTxt (0xffe9e4d8), kDim (0xff8f897c), kPhos (0xff9cf2a8);
static juce::Colour bandCol (int b) { static const juce::uint32 c[kBands] = { 0xffff6b6b, 0xffffa94d, 0xffffd43b, 0xffa9e34b, 0xff51cf66, 0xff38d9a9, 0xff3bc9db, 0xff4dabf7, 0xff748ffc, 0xffb197fc, 0xfff783ac, 0xffe599f7 }; return juce::Colour (c[b % kBands]); }
static void darkBox (juce::Graphics& g, juce::Rectangle<float> r, float rad = 6)
{
    g.setColour (juce::Colours::black.withAlpha (0.5f)); g.fillRoundedRectangle (r.translated (0, 2), rad);
    g.setColour (kPanel); g.fillRoundedRectangle (r, rad); g.setColour (kLine); g.drawRoundedRectangle (r.reduced (0.5f), rad, 1.0f);
}
static void title (juce::Graphics& g, const juce::String& t, juce::Rectangle<float> r, juce::Colour c = kDim) { g.setFont (alook::serif (9.5f)); g.setColour (c); g.drawText (t, r, juce::Justification::centredLeft); }
static void button (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& t, bool on, juce::Colour c = juce::Colour (0xffffb347))
{
    g.setColour (on ? c.withAlpha (0.22f) : juce::Colour (0xff262522)); g.fillRoundedRectangle (r, 4);
    g.setColour (on ? c : kLine.brighter (0.2f)); g.drawRoundedRectangle (r.reduced (0.5f), 4, on ? 1.4f : 1.0f);
    g.setFont (alook::serif (10.0f)); g.setColour (on ? c : kTxt.withAlpha (0.8f)); g.drawText (t, r, juce::Justification::centred);
}
static juce::String hzT (float f) { return f >= 1000 ? juce::String (f / 1000.0f, f >= 10000 ? 1 : 2) + " kHz" : juce::String (juce::roundToInt (f)) + " Hz"; }

// ======================================================================= meter
void MeterPanel::paint (juce::Graphics& g)
{
    auto& P = ed.proc; const auto& T = targets()[(size_t) juce::jlimit (0, (int) targets().size() - 1, (int) P.value ("target"))];
    darkBox (g, getLocalBounds().toFloat());
    title (g, "METER  \xc2\xb7  USCITA", { 12, 8, 160, 18 }); button (g, resetR(), "RESET", false);
    // target per strumento (clic = cambia)
    auto tr = targetR(); g.setColour (juce::Colour (0xff11100f)); g.fillRoundedRectangle (tr, 4); g.setColour (kLine.brighter (0.3f)); g.drawRoundedRectangle (tr, 4, 1);
    g.setFont (alook::serif (10.0f)); g.setColour (juce::Colour (0xffffb347)); g.drawText (juce::String ("TARGET: ") + T.name + juce::String::fromUTF8 ("  \xe2\x96\xbe"), tr.reduced (8, 0), juce::Justification::centredLeft);
    const float S = P.lufsS.load(), M = P.lufsM.load(), I = P.lufsI.load(), tp = P.tp.load(), tpH = P.tpHold.load();
    auto num = [&] (float x, float y, float w, const juce::String& lab, float v, float big, juce::Colour c)
    {
        g.setFont (alook::serif (8.5f)); g.setColour (kDim); g.drawText (lab, juce::Rectangle<float> (x, y, w, 12), juce::Justification::centredLeft);
        g.setFont (alook::typed (big)); g.setColour (c); g.drawText (v <= -69 ? juce::String ("--") : juce::String (v, 1), juce::Rectangle<float> (x, y + 12, w, big + 4), juce::Justification::centredLeft);
    };
    // stato rispetto al target
    juce::Colour sc = kPhos; juce::String status = "nella zona giusta";
    if (S > -69) { if (S > T.lufsHi) { sc = juce::Colour (0xffff6b5e); status = "troppo forte di " + juce::String (S - T.lufsHi, 1) + " dB"; } else if (S < T.lufsLo) { sc = juce::Colour (0xff7fb8ff); status = "piano di " + juce::String (T.lufsLo - S, 1) + " dB"; } }
    else status = "nessun segnale";
    num (14, 62, 140, "LUFS SHORT-TERM", S, 30, sc);
    num (160, 62, 70, "MOMENTARY", M, 15, kTxt); num (232, 62, 70, "INTEGRATO", I, 15, kTxt);
    num (160, 96, 70, "TRUE PEAK", tp, 15, tp > T.peak ? juce::Colour (0xffff6b5e) : kTxt); num (232, 96, 70, "PICCO MAX", tpH, 15, tpH > T.peak ? juce::Colour (0xffff6b5e) : kTxt);
    // barra LUFS con la zona del target
    auto bar = juce::Rectangle<float> (14, 140, getWidth() - 28.0f, 16);
    auto xL = [&] (float db) { return bar.getX() + bar.getWidth() * juce::jlimit (0.0f, 1.0f, (db + 40.0f) / 40.0f); };
    g.setColour (juce::Colour (0xff0f0e0d)); g.fillRoundedRectangle (bar, 3);
    g.setColour (kPhos.withAlpha (0.22f)); g.fillRect (juce::Rectangle<float>::leftTopRightBottom (xL (T.lufsLo), bar.getY(), xL (T.lufsHi), bar.getBottom()));
    if (S > -69) { g.setColour (sc); g.fillRoundedRectangle (juce::Rectangle<float> (bar.getX(), bar.getY() + 4, xL (S) - bar.getX(), 8), 2); }
    g.setFont (alook::typed (8.0f)); g.setColour (kDim);
    for (int d = -40; d <= 0; d += 10) { g.drawVerticalLine ((int) xL ((float) d), bar.getBottom(), bar.getBottom() + 4); g.drawText (juce::String (d), juce::Rectangle<float> (xL ((float) d) - 12, bar.getBottom() + 4, 24, 10), juce::Justification::centred); }
    // barra del picco
    auto pb = juce::Rectangle<float> (14, 178, getWidth() - 28.0f, 10);
    auto xP = [&] (float db) { return pb.getX() + pb.getWidth() * juce::jlimit (0.0f, 1.0f, (db + 30.0f) / 30.0f); };
    g.setColour (juce::Colour (0xff0f0e0d)); g.fillRoundedRectangle (pb, 2);
    if (tp > -69) { g.setColour (tp > T.peak ? juce::Colour (0xffff6b5e) : juce::Colour (0xffffd166)); g.fillRoundedRectangle (juce::Rectangle<float> (pb.getX(), pb.getY() + 2, xP (tp) - pb.getX(), 6), 2); }
    g.setColour (juce::Colours::white); g.fillRect (xP (T.peak) - 1, pb.getY() - 3, 2.0f, pb.getHeight() + 6);
    g.setFont (alook::serif (8.0f)); g.setColour (kDim); g.drawText ("PICCO CONSIGLIATO " + juce::String (T.peak, 0) + " dBFS", juce::Rectangle<float> (14, 190, 200, 12), juce::Justification::centredLeft);
    g.setFont (alook::serif (10.0f)); g.setColour (sc); g.drawText (status, juce::Rectangle<float> (14, 206, getWidth() - 28.0f, 14), juce::Justification::centredLeft);
    g.setFont (alook::serif (8.0f, false)); g.setColour (kDim); g.drawFittedText (T.note, juce::Rectangle<int> (14, 220, getWidth() - 28, 16), juce::Justification::centredLeft, 1, 0.8f);
}
void MeterPanel::mouseDown (const juce::MouseEvent& e)
{
    if (resetR().contains (e.position)) { ed.proc.resetMeter = true; return; }
    if (targetR().contains (e.position))
    {
        juce::PopupMenu m; const int cur = (int) ed.proc.value ("target");
        for (int i = 0; i < (int) targets().size(); ++i) m.addItem (i + 1, targets()[(size_t) i].name, true, i == cur);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [this] (int r) { if (r > 0) { ed.proc.setP ("target", (float) (r - 1)); repaint(); } });
    }
}

// ======================================================================= imager
void ImagerPanel::paint (juce::Graphics& g)
{
    darkBox (g, getLocalBounds().toFloat());
    title (g, "IMAGER", { 12, 8, 100, 18 });
    auto sq = juce::Rectangle<float> (14, 28, getWidth() - 28.0f, getWidth() - 28.0f);
    g.setColour (juce::Colour (0xff0c0f0d)); g.fillEllipse (sq); g.setColour (kLine); g.drawEllipse (sq, 1);
    const float cx = sq.getCentreX(), cy = sq.getCentreY(), R = sq.getWidth() * 0.5f;
    g.setColour (kLine); g.drawLine (cx, sq.getY(), cx, sq.getBottom(), 0.6f); g.drawLine (sq.getX(), cy, sq.getRight(), cy, 0.6f);
    g.setFont (alook::typed (7.5f)); g.setColour (kDim); g.drawText ("L", juce::Rectangle<float> (sq.getX() + R * 0.25f - 6, sq.getY() + R * 0.25f - 6, 12, 12), juce::Justification::centred); g.drawText ("R", juce::Rectangle<float> (sq.getRight() - R * 0.25f - 6, sq.getY() + R * 0.25f - 6, 12, 12), juce::Justification::centred);
    ed.proc.post.read (bl, br, 1024);
    float pk = 1e-4f; for (int i = 0; i < 1024; ++i) pk = juce::jmax (pk, std::abs (bl[i]), std::abs (br[i]));
    const float sc = R * 0.9f / juce::jmax (0.25f, pk);
    for (int i = 0; i < 1024; i += 2)
    {
        const float m = (bl[i] + br[i]) * 0.7071f, s = (bl[i] - br[i]) * 0.7071f;
        const float x = cx - s * sc, y = cy - m * sc;
        g.setColour (kPhos.withAlpha (0.35f)); g.fillRect (x, y, 1.5f, 1.5f);
    }
    const float c = ed.proc.corr.load(), w = ed.proc.width.load();
    auto cb = juce::Rectangle<float> (14, sq.getBottom() + 10, getWidth() - 28.0f, 8);
    g.setColour (juce::Colour (0xff0f0e0d)); g.fillRoundedRectangle (cb, 2);
    const float xc = cb.getCentreX() + c * cb.getWidth() * 0.5f;
    g.setColour (c < 0 ? juce::Colour (0xffff6b5e) : kPhos); g.fillRect (juce::jmin (cb.getCentreX(), xc), cb.getY() + 1, std::abs (xc - cb.getCentreX()), cb.getHeight() - 2);
    g.setFont (alook::serif (8.0f)); g.setColour (kDim);
    g.drawText ("CORRELAZIONE " + juce::String (c, 2), juce::Rectangle<float> (14, cb.getBottom() + 2, 140, 12), juce::Justification::centredLeft);
    g.drawText ("LARGHEZZA " + juce::String (juce::roundToInt (w * 200)) + "%", juce::Rectangle<float> (14, cb.getBottom() + 2, getWidth() - 28.0f, 12), juce::Justification::centredRight);
}

// ======================================================================= rack
std::vector<RackStrip::Item> RackStrip::items() const
{
    std::vector<Item> v; const int n = ed.proc.chainLen(), ep = juce::jlimit (0, n, (int) ed.proc.value ("eq_pos"));
    const int count = n + 1 + (n < kSlots ? 1 : 0);
    const float tw = juce::jmin (118.0f, (getWidth() - 20.0f - (count - 1) * 6.0f) / juce::jmax (1, count));
    float x = 10;
    auto push = [&] (int k, int s) { v.push_back ({ k, s, { x, 32, k == 1 ? tw * 0.8f : tw, getHeight() - 42.0f } }); x += (k == 1 ? tw * 0.8f : tw) + 6; };
    for (int s = 0; s <= n; ++s) { if (s == ep) push (1, -1); if (s < n) push (0, s); }
    if (n < kSlots) push (2, n);
    return v;
}
int RackStrip::insertIndex (juce::Point<float> p) const
{
    int idx = 0; for (auto& it : items()) if (it.kind == 0 && p.x > it.r.getCentreX()) idx = it.slot + 1;
    return juce::jlimit (0, ed.proc.chainLen(), idx);
}
void RackStrip::paint (juce::Graphics& g)
{
    darkBox (g, getLocalBounds().toFloat());
    title (g, juce::String::fromUTF8 ("RACK  \xc2\xb7  ") + juce::String (ed.proc.chainLen()) + "/10 moduli  \xc2\xb7  clic = apri  \xc2\xb7  trascina = sposta  \xc2\xb7  tasto destro = menu", { 12, 8, getWidth() - 24.0f, 18 });
    auto& P = ed.proc;
    for (auto& it : items())
    {
        auto r = it.r;
        if (it.kind == 2)
        {
            g.setColour (juce::Colour (0xff1a1917)); g.fillRoundedRectangle (r, 5);
            g.setColour (kLine.brighter (0.4f)); juce::Path d; d.addRoundedRectangle (r.reduced (1), 5); juce::PathStrokeType (1.2f).createDashedStroke (d, d, std::array<float, 2> { 5.0f, 4.0f }.data(), 2); g.fillPath (d);
            g.setColour (kTxt.withAlpha (0.7f)); g.setFont (alook::serif (34.0f)); g.drawText ("+", r.withTrimmedBottom (r.getHeight() * 0.25f), juce::Justification::centred);
            g.setFont (alook::serif (9.0f)); g.drawText ("AGGIUNGI", r.withTrimmedTop (r.getHeight() * 0.55f).withHeight (16), juce::Justification::centred);
            continue;
        }
        if (it.kind == 1)
        {   // l'EQ centrale nella catena
            const bool on = P.value ("eq_on") > 0.5f;
            g.setColour (juce::Colour (0xff10202a)); g.fillRoundedRectangle (r, 5); g.setColour (juce::Colour (0xff4dabf7).withAlpha (on ? 0.9f : 0.3f)); g.drawRoundedRectangle (r.reduced (0.5f), 5, 1.4f);
            alook::jewel (g, r.getCentreX(), r.getY() + 16, 6, juce::Colour (0xff4dabf7), on);
            g.saveState(); g.addTransform (juce::AffineTransform::rotation (-juce::MathConstants<float>::halfPi, r.getCentreX(), r.getCentreY()));
            g.setFont (alook::serif (11.0f)); g.setColour (juce::Colour (0xff9fd4ff)); g.drawText ("EQ CENTRALE", juce::Rectangle<float> (r.getCentreX() - 70, r.getCentreY() - 8, 140, 16), juce::Justification::centred);
            g.restoreState();
            continue;
        }
        const int t = P.slotType (it.slot); const auto& d = mod (t); const bool byp = P.valueS (Processor::sid (it.slot, "byp")) > 0.5f;
        const juce::Colour plate (d.look.plate); const juce::Colour ink (d.look.dark ? 0xffeeeae0 : 0xff1e1b17);
        if (it.slot == dragFrom) g.setOpacity (0.5f);
        alook::faceplate (g, r, false, plate);
        g.setColour (catColour (d.cat)); g.fillRect (r.getX() + 3, r.getY() + 3, r.getWidth() - 6, 4.0f);
        alook::jewel (g, r.getX() + 13, r.getY() + 20, 5, juce::Colour (d.look.accent), ! byp);
        g.setFont (alook::typed (8.0f)); g.setColour (ink.withAlpha (0.6f)); g.drawText (juce::String (it.slot + 1), juce::Rectangle<float> (r.getRight() - 22, r.getY() + 12, 16, 14), juce::Justification::centredRight);
        g.setFont (alook::serif (11.0f)); g.setColour (ink); g.drawFittedText (d.name, r.reduced (6, 0).withY (r.getY() + 30).withHeight (30).toNearestInt(), juce::Justification::centred, 2, 0.8f);
        g.setFont (alook::serif (7.5f, false)); g.setColour (ink.withAlpha (0.6f)); g.drawFittedText (catName (d.cat), r.reduced (4, 0).withY (r.getY() + 60).withHeight (12).toNearestInt(), juce::Justification::centred, 1, 0.7f);
        // attivita: riduzione (rosso) o livello (verde)
        auto mr = juce::Rectangle<float> (r.getX() + 10, r.getY() + 80, r.getWidth() - 20, r.getHeight() - 112);
        g.setColour (juce::Colours::black.withAlpha (0.55f)); g.fillRoundedRectangle (mr, 3);
        if (auto* e = P.engine (it.slot))
        {
            const float gr = e->gr.load(), lv = juce::Decibels::gainToDecibels (e->out.load(), -60.0f);
            const float hl = mr.getHeight() * juce::jlimit (0.0f, 1.0f, (lv + 48.0f) / 48.0f);
            g.setColour (juce::Colour (0xff7dde5a).withAlpha (0.8f)); g.fillRect (mr.getX() + 3, mr.getBottom() - hl, mr.getWidth() * 0.5f - 4, hl);
            if (gr < -0.05f && d.cat != C_DYN) { const float hg = mr.getHeight() * juce::jlimit (0.0f, 1.0f, -gr / 20.0f); g.setColour (juce::Colour (0xffff6b5e)); g.fillRect (mr.getCentreX() + 1, mr.getY(), mr.getWidth() * 0.5f - 4, hg); }
        }
        const float mix = P.valueS (Processor::sid (it.slot, "mix"));
        g.setFont (alook::typed (8.0f)); g.setColour (ink.withAlpha (0.75f)); g.drawText ("MIX " + juce::String (juce::roundToInt (mix * 100)) + "%", juce::Rectangle<float> (r.getX(), r.getBottom() - 28, r.getWidth(), 12), juce::Justification::centred);
        if (byp) { g.setColour (juce::Colours::black.withAlpha (0.45f)); g.fillRoundedRectangle (r, 4); g.setColour (juce::Colours::white.withAlpha (0.8f)); g.setFont (alook::serif (9.0f)); g.drawText ("BYPASS", r, juce::Justification::centred); }
        if (it.slot == ed.sel) { g.setColour (juce::Colour (0xffffb347)); g.drawRoundedRectangle (r.expanded (2), 6, 2.0f); }
        g.setOpacity (1.0f);
        g.setColour (ink.withAlpha (0.5f)); g.setFont (alook::serif (11.0f)); g.drawText (juce::String::fromUTF8 ("\xe2\x8b\xae"), juce::Rectangle<float> (r.getRight() - 16, r.getBottom() - 16, 14, 14), juce::Justification::centred);
    }
    if (dropAt >= 0)
    {   // punto di inserimento
        float x = 8; for (auto& it : items()) if (it.kind == 0 && it.slot < dropAt) x = it.r.getRight() + 3; else if (it.kind == 1 && dropAt > 0 && x < it.r.getX()) {}
        g.setColour (juce::Colour (0xffffb347)); g.fillRoundedRectangle (x - 2, 30, 4, getHeight() - 38.0f, 2);
    }
}
juce::PopupMenu Editor::moduleMenu (int base, int cur)
{
    juce::PopupMenu m;
    for (int c = 0; c < kCats; ++c)
    {
        juce::PopupMenu sub; bool any = false;
        for (int t = 1; t < (int) mods().size(); ++t) if (mod (t).cat == c) { sub.addItem (base + t, mod (t).name + "   -   " + mod (t).inspired, true, t == cur); any = true; }
        if (any) m.addSubMenu (catName (c), sub);
    }
    return m;
}
void RackStrip::openMenu (int s, bool replace)
{
    auto& P = ed.proc;
    if (! replace) { auto m = Editor::moduleMenu (1, -1); m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [this] (int r) { if (r > 0) { ed.insertAt (ed.proc.chainLen(), r - 1); repaint(); } }); return; }
    juce::PopupMenu m; const bool byp = P.valueS (Processor::sid (s, "byp")) > 0.5f;
    m.addSectionHeader (juce::String (s + 1) + "  " + mod (P.slotType (s)).name);
    m.addSubMenu ("Sostituisci con...", Editor::moduleMenu (1000, P.slotType (s)));
    m.addItem (1, byp ? "Riattiva" : "Bypass"); m.addItem (2, "Copia modulo"); m.addItem (3, "Incolla qui", P.hasClip()); m.addItem (4, "Duplica");
    m.addSeparator(); m.addItem (5, "Rimuovi");
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [this, s] (int r)
    {
        auto& P = ed.proc;
        if (r == 1) { auto* p = ed.P (Processor::sid (s, "byp")); p->setValueNotifyingHost (p->getValue() > 0.5f ? 0.0f : 1.0f); }
        else if (r == 2) P.copySlot (s);
        else if (r == 3) P.pasteSlot (s);
        else if (r == 4) { P.copySlot (s); if (P.chainLen() < kSlots) { P.insertModule (s + 1, P.slotType (s)); P.pasteSlot (s + 1); } }
        else if (r == 5) { P.removeSlot (s); ed.selectSlot (juce::jmin (s, P.chainLen() - 1)); }
        else if (r >= 1000) P.setModule (s, r - 1000);
        ed.panel.rebuild(); repaint();
    });
}
void RackStrip::mouseDown (const juce::MouseEvent& e)
{
    downAt = e.position; dragFrom = -2;
    for (auto& it : items())
    {
        if (! it.r.contains (e.position)) continue;
        if (it.kind == 2) { openMenu (-1, false); return; }
        if (it.kind == 1)
        {
            if (e.position.y < it.r.getY() + 30) { auto* p = ed.P ("eq_on"); p->setValueNotifyingHost (p->getValue() > 0.5f ? 0.0f : 1.0f); repaint(); return; }
            dragFrom = -1; return;   // si puo trascinare anche l'EQ
        }
        if (e.mods.isPopupMenu() || (e.position.x > it.r.getRight() - 18 && e.position.y > it.r.getBottom() - 18)) { ed.selectSlot (it.slot); openMenu (it.slot, true); return; }
        if (e.position.y < it.r.getY() + 30 && e.position.x < it.r.getX() + 26) { auto* p = ed.P (Processor::sid (it.slot, "byp")); p->setValueNotifyingHost (p->getValue() > 0.5f ? 0.0f : 1.0f); repaint(); return; }
        ed.selectSlot (it.slot); dragFrom = it.slot; return;
    }
}
void RackStrip::mouseDrag (const juce::MouseEvent& e)
{
    if (dragFrom == -2 || e.position.getDistanceFrom (downAt) < 6) return;
    if (! ed.isDragAndDropActive()) ed.startDragging (dragFrom == -1 ? juce::String ("eq") : "slot:" + juce::String (dragFrom), this);
}
void RackStrip::itemDropped (const SourceDetails& d)
{
    const juce::String s = d.description.toString(); const int at = insertIndex (d.localPosition.toFloat()); dropAt = -1;
    auto& P = ed.proc;
    if (s.startsWith ("mod:")) { if (P.chainLen() < kSlots) ed.insertAt (at, s.fromFirstOccurrenceOf (":", false, false).getIntValue()); }
    else if (s.startsWith ("slot:"))
    {
        const int from = s.fromFirstOccurrenceOf (":", false, false).getIntValue(); int to = at > from ? at - 1 : at;
        to = juce::jlimit (0, P.chainLen() - 1, to); P.moveSlot (from, to); ed.selectSlot (to);
    }
    else if (s == "eq") P.setP ("eq_pos", (float) at);
    dragFrom = -2; repaint();
}

// ======================================================================= browser
BrowserList::BrowserList (Editor& e) : ed (e) { open.assign (kCats, false); open[C_PRE] = true; rebuildRows(); }
void BrowserList::rebuildRows()
{
    rows.clear();
    for (int c = 0; c < kCats; ++c)
    {
        rows.push_back ({ c, -1 });
        if (open[(size_t) c]) for (int t = 1; t < (int) mods().size(); ++t) if (mod (t).cat == c) rows.push_back ({ c, t });
    }
    setSize (getWidth() > 0 ? getWidth() : 200, (int) rows.size() * 20 + 4);
    repaint();
}
void BrowserList::paint (juce::Graphics& g)
{
    for (int i = 0; i < (int) rows.size(); ++i)
    {
        auto r = juce::Rectangle<float> (0, i * 20.0f, (float) getWidth(), 20);
        const auto& row = rows[(size_t) i];
        if (row.type < 0)
        {
            g.setColour (catColour (row.cat).withAlpha (i == hover ? 0.28f : 0.16f)); g.fillRect (r.reduced (2, 1));
            g.setColour (catColour (row.cat)); g.fillRect (r.getX() + 2, r.getY() + 1, 3.0f, r.getHeight() - 2);
            g.setFont (alook::serif (9.5f)); g.setColour (kTxt); g.drawText ((open[(size_t) row.cat] ? juce::String::fromUTF8 ("\xe2\x96\xbe  ") : juce::String::fromUTF8 ("\xe2\x96\xb8  ")) + catName (row.cat), r.withTrimmedLeft (10), juce::Justification::centredLeft);
        }
        else
        {
            if (i == hover) { g.setColour (juce::Colours::white.withAlpha (0.06f)); g.fillRect (r); }
            g.setColour (juce::Colour (mod (row.type).look.plate)); g.fillRoundedRectangle (r.getX() + 12, r.getY() + 5, 10, 10, 2);
            g.setFont (alook::serif (9.5f)); g.setColour (kTxt.withAlpha (0.92f)); g.drawText (mod (row.type).name, r.withTrimmedLeft (28), juce::Justification::centredLeft);
        }
    }
}
void BrowserList::mouseMove (const juce::MouseEvent& e)
{
    const int h = rowAt (e.position.y); if (h != hover) { hover = h; repaint(); }
    if (juce::isPositiveAndBelow (h, (int) rows.size()) && rows[(size_t) h].type > 0) { const auto& d = mod (rows[(size_t) h].type); setTooltip (d.name + " (" + d.inspired + ")\n" + d.desc); }
    else setTooltip ({});
}
void BrowserList::mouseDown (const juce::MouseEvent& e)
{
    pressed = rowAt (e.position.y); dragged = false;
    if (! juce::isPositiveAndBelow (pressed, (int) rows.size())) return;
    if (rows[(size_t) pressed].type < 0) { const int c = rows[(size_t) pressed].cat; open[(size_t) c] = ! open[(size_t) c]; rebuildRows(); pressed = -1; }
}
void BrowserList::mouseDrag (const juce::MouseEvent& e)
{
    if (! juce::isPositiveAndBelow (pressed, (int) rows.size()) || rows[(size_t) pressed].type <= 0 || e.getDistanceFromDragStart() < 5) return;
    if (! ed.isDragAndDropActive()) { dragged = true; ed.startDragging ("mod:" + juce::String (rows[(size_t) pressed].type), this); }
}

// ======================================================================= modulo selezionato
void ModulePanel::rebuild()
{
    ctl.clear(); mixK.reset(); removeAllChildren();
    auto& P = ed.proc; const int s = ed.sel;
    shownSlot = s; shownType = s >= 0 && s < P.chainLen() ? P.slotType (s) : 0;
    if (shownType > 0)
    {
        const auto& d = mod (shownType); const juce::Colour ink (d.look.dark ? 0xffeeeae0 : 0xff241f19), soft (d.look.dark ? 0xffbdb6a6 : 0xff5a4a38), hot (d.look.accent);
        for (int i = 0; i < kP; ++i)
        {
            const auto& sp = d.spec[(size_t) i]; if (hidden (sp)) continue;
            auto* prm = ed.P (Processor::sid (s, ("p" + juce::String (i + 1)).toRawUTF8()));
            juce::Component* c = nullptr;
            if (sp.fmt == F_BOOL) { auto t = std::make_unique<alook::Toggle> (prm, sp.label, "ON", "OFF"); t->ink = ink; c = t.get(); ctl.push_back (std::move (t)); }
            else
            {
                auto k = std::make_unique<alook::Knob> (prm, sp.label, sp.cap >= 0 ? sp.cap : d.look.knob);
                k->ink = ink; k->soft = soft; k->hot = hot.getPerceivedBrightness() > 0.2f ? hot : ink;
                const int st = shownType;
                k->text = [st, i, prm] { return paramText (st, i, prm->getValue()); };
                k->numbers = sp.fmt != F_ITEMS && sp.fmt != F_AEF; k->ticks = sp.fmt == F_ITEMS ? juce::jmax (1, sp.items.size() - 1) : 10;
                c = k.get(); ctl.push_back (std::move (k));
            }
            addAndMakeVisible (c);
            c->setComponentID (juce::String (i));
        }
        mixK = std::make_unique<alook::Knob> (ed.P (Processor::sid (s, "mix")), d.send ? "MANDATA" : "MIX", d.look.knob);
        mixK->ink = ink; mixK->soft = soft; mixK->numbers = false; mixK->text = [this, s] { return juce::String (juce::roundToInt (ed.proc.valueS (Processor::sid (s, "mix")) * 100)) + "%"; };
        addAndMakeVisible (*mixK);
    }
    layout(); repaint();
}
void ModulePanel::layout()
{
    if (shownType <= 0) return;
    const auto& d = mod (shownType);
    auto area = getLocalBounds().toFloat().reduced (16).withTrimmedTop (62);
    meterR = {};
    if (d.look.meter > 0) { meterR = juce::Rectangle<float> (area.getX() + 4, area.getY() + 22, 170, 128); area.removeFromLeft (190); }
    mixK->setBounds (juce::Rectangle<float> (getWidth() - 100.0f, 10, 80, 50 + 30).toNearestInt());
    const int n = (int) ctl.size(); if (n == 0) return;
    const int rows = n > 7 ? 2 : 1; const int perRow = (n + rows - 1) / rows;
    const float cw = juce::jmin (140.0f, area.getWidth() / perRow), ch = area.getHeight() / rows;
    for (int k = 0; k < n; ++k)
    {
        const int r = k / perRow, c = k % perRow;
        auto cell = juce::Rectangle<float> (area.getX() + c * cw, area.getY() + r * ch, cw, ch).reduced (6, 4);
        auto* comp = ctl[(size_t) k].get();
        if (dynamic_cast<alook::Toggle*> (comp)) comp->setBounds (cell.withSizeKeepingCentre (cell.getWidth(), 50).toNearestInt());
        else { const float sz = juce::jmin (cell.getWidth(), cell.getHeight() - 30); comp->setBounds (cell.withSizeKeepingCentre (sz, sz + 30).toNearestInt()); }
    }
}
void ModulePanel::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    if (shownType <= 0)
    {
        darkBox (g, b);
        g.setFont (alook::serif (14.0f)); g.setColour (kDim);
        g.drawText (ed.proc.chainLen() == 0 ? "Il rack e vuoto: premi + nel rack o trascina un modulo dal browser a sinistra" : "Clicca un modulo nel rack per aprirlo qui", b, juce::Justification::centred);
        return;
    }
    const auto& d = mod (shownType); const auto& L = d.look; const juce::Colour ink (L.dark ? 0xffeeeae0 : 0xff241f19);
    alook::faceplate (g, b, true, juce::Colour (L.plate));
    g.setColour (juce::Colour (L.accent).withAlpha (0.85f)); g.fillRect (b.getX() + 30, b.getY() + 54, b.getWidth() - 60, 1.5f);
    const bool byp = ed.proc.valueS (Processor::sid (shownSlot, "byp")) > 0.5f;
    alook::jewel (g, powerR.getCentreX() + 20, powerR.getCentreY() + 4, 8, juce::Colour (L.accent), ! byp);
    alook::engrave (g, d.name, { 72, 10, 420, 30 }, alook::wide (22.0f), juce::Justification::centredLeft, ink);
    alook::engrave (g, juce::String ("ispirato a ") + d.inspired + "   \xc2\xb7   slot " + juce::String (shownSlot + 1), { 74, 36, 520, 14 }, alook::typed (9.0f), juce::Justification::centredLeft, ink.withAlpha (0.7f));
    g.setFont (alook::serif (10.0f, false)); g.setColour (ink.withAlpha (0.85f));
    g.drawFittedText (d.desc, juce::Rectangle<int> (560, 12, (int) b.getWidth() - 690, 40), juce::Justification::centredLeft, 2, 0.9f);
    if (L.meter > 0 && ! meterR.isEmpty())
    {
        float db = -40; juce::String lab;
        if (auto* e = ed.proc.engine (shownSlot))
        {
            if (L.meter == 2) { db = juce::Decibels::gainToDecibels (e->out.load(), -60.0f) + 18.0f - 6.0f; lab = "OUTPUT"; }
            else { db = e->gr.load(); lab = "GAIN REDUCTION"; }
        }
        vu += ((L.meter == 2 ? db : 3 + db * 1.15f) - vu) * 0.35f;   // riduzione: ago che scende da 0 (e qui +3) verso sinistra
        alook::vuMeter (g, meterR, vu, lab);
        if (L.meter != 2) { g.setFont (alook::typed (10.0f)); g.setColour (ink); g.drawText (juce::String (db, 1) + " dB", meterR.translated (0, meterR.getHeight() + 6).withHeight (14), juce::Justification::centred); }
    }
    if (d.kind == K_CONV)
    {
        irR = { b.getRight() - 230, 18, 120, 26 };
        button (g, irR, "CARICA IR...", false, juce::Colour (L.accent));
        const auto nm = ed.proc.userIrName (shownSlot);
        if (nm.isNotEmpty()) { g.setFont (alook::typed (8.5f)); g.setColour (ink); g.drawText (nm, irR.translated (0, 28).withWidth (200), juce::Justification::centredLeft); }
    }
    if (byp) { g.setColour (juce::Colours::black.withAlpha (0.25f)); g.fillRoundedRectangle (b.withTrimmedTop (60), 4); }
}
void ModulePanel::mouseDown (const juce::MouseEvent& e)
{
    if (shownType <= 0) return;
    if (juce::Rectangle<float> (14, 10, 50, 40).contains (e.position)) { auto* p = ed.P (Processor::sid (shownSlot, "byp")); p->setValueNotifyingHost (p->getValue() > 0.5f ? 0.0f : 1.0f); repaint(); ed.rack.repaint(); return; }
    if (mod (shownType).kind == K_CONV && irR.contains (e.position))
    {
        auto fc = std::make_shared<juce::FileChooser> ("Scegli una risposta all'impulso (WAV/AIFF)", juce::File::getSpecialLocation (juce::File::userMusicDirectory), "*.wav;*.aif;*.aiff");
        const int s = shownSlot;
        fc->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [this, fc, s] (const juce::FileChooser& c)
        {
            const auto f = c.getResult(); if (! f.existsAsFile()) return;
            ed.proc.setUserIr (s, f); ed.proc.setP (Processor::sid (s, "p1"), 1.0f); repaint();
        });
    }
}

// ======================================================================= EQ centrale
EqGraph::EqGraph (Editor& e) : ed (e)
{
    win.resize (4096); buf.resize (8192); specPre.assign (2048, -100.0f); specPost.assign (2048, -100.0f);
    for (int i = 0; i < 4096; ++i) win[(size_t) i] = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * i / 4095.0f);
}
float EqGraph::xOf (float f) const { return 36.0f + (getWidth() - 46.0f) * std::log (juce::jmax (10.0f, f) / 10.0f) / std::log (2200.0f); }
float EqGraph::fOf (float x) const { return 10.0f * std::pow (2200.0f, juce::jlimit (0.0f, 1.0f, (x - 36.0f) / (getWidth() - 46.0f))); }
float EqGraph::yOf (float db) const { return 24.0f + (getHeight() - 46.0f) * (0.5f - db / 60.0f); }
float EqGraph::dbOf (float y) const { return (0.5f - (y - 24.0f) / (getHeight() - 46.0f)) * 60.0f; }
juce::Point<float> EqGraph::nodeOf (int b) const
{
    auto& P = ed.proc; const int t = (int) P.valueS (Processor::bid (b, "type"));
    return { xOf (P.valueS (Processor::bid (b, "f"))), hasGain (t) ? yOf (P.valueS (Processor::bid (b, "g"))) : yOf (0) };
}
int EqGraph::hit (juce::Point<float> p) const
{
    int best = -1; float bd = 12;
    for (int b = 0; b < kBands; ++b) if (ed.proc.valueS (Processor::bid (b, "on")) > 0.5f) { const float d = p.getDistanceFrom (nodeOf (b)); if (d < bd) { bd = d; best = b; } }
    return best;
}
void EqGraph::tick()
{
    auto spec = [this] (const Ring& r, std::vector<float>& out)
    {
        std::vector<float> l (4096), rr (4096); r.read (l.data(), rr.data(), 4096);
        for (int i = 0; i < 4096; ++i) buf[(size_t) i] = 0.5f * (l[(size_t) i] + rr[(size_t) i]) * win[(size_t) i];
        std::fill (buf.begin() + 4096, buf.end(), 0.0f); fft.performFrequencyOnlyForwardTransform (buf.data());
        for (int i = 0; i < 2048; ++i) { const float db = juce::Decibels::gainToDecibels (buf[(size_t) i] / 1024.0f, -100.0f); out[(size_t) i] = db > out[(size_t) i] ? db : out[(size_t) i] * 0.85f + db * 0.15f; }
    };
    spec (ed.proc.pre, specPre); spec (ed.proc.post, specPost);
    repaint();
}
void EqGraph::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat(); darkBox (g, b);
    auto s = b.reduced (6);
    g.setColour (juce::Colour (0xff0d0e10)); g.fillRoundedRectangle (s, 5);
    const double sr = juce::jmax (8000.0, ed.proc.sampleRate());
    g.setFont (alook::typed (8.0f));
    for (float f : { 20.f, 50.f, 100.f, 200.f, 500.f, 1000.f, 2000.f, 5000.f, 10000.f, 20000.f })
    { g.setColour (juce::Colours::white.withAlpha (0.05f)); g.drawVerticalLine ((int) xOf (f), s.getY() + 4, s.getBottom() - 16); g.setColour (kDim); g.drawText (f >= 1000 ? juce::String ((int) (f / 1000)) + "k" : juce::String ((int) f), juce::Rectangle<float> (xOf (f) - 14, s.getBottom() - 14, 28, 12), juce::Justification::centred); }
    for (int db = -24; db <= 24; db += 6)
    { g.setColour (juce::Colours::white.withAlpha (db == 0 ? 0.12f : 0.04f)); g.drawHorizontalLine ((int) yOf ((float) db), 36, s.getRight() - 4); g.setColour (kDim); g.drawText ((db > 0 ? "+" : "") + juce::String (db), juce::Rectangle<float> (8, yOf ((float) db) - 6, 24, 12), juce::Justification::centredRight); }
    auto specPath = [&] (const std::vector<float>& sp, bool fill) -> juce::Path
    {
        juce::Path p; bool st = false;
        for (int x = 36; x < getWidth() - 10; x += 2)
        {
            const float f = fOf ((float) x); const int bin = juce::jlimit (1, 2047, (int) (f / sr * 4096));
            const float db = sp[(size_t) bin] + 4.5f * std::log2 (juce::jmax (1.0f, f / 1000.0f));
            const float y = juce::jlimit (s.getY() + 2, s.getBottom() - 16, s.getBottom() - 16 - (db + 96.0f) / 96.0f * (s.getHeight() - 20));
            if (! st) { p.startNewSubPath ((float) x, fill ? s.getBottom() - 16 : y); st = true; if (fill) p.lineTo ((float) x, y); } else p.lineTo ((float) x, y);
        }
        if (fill) { p.lineTo (s.getRight() - 10, s.getBottom() - 16); p.closeSubPath(); }
        return p;
    };
    g.setColour (juce::Colours::white.withAlpha (0.12f)); g.strokePath (specPath (specPre, false), juce::PathStrokeType (1.0f));
    g.setColour (juce::Colour (0xff4dabf7).withAlpha (0.12f)); g.fillPath (specPath (specPost, true));
    const auto E = ed.proc.eqSettings(); const bool eqOn = ed.proc.value ("eq_on") > 0.5f;
    // riduzione spettrale
    if (ed.proc.eq.spectralOn (E))
    {
        juce::Path rp; rp.startNewSubPath (xOf (20), yOf (0));
        for (int v = 0; v < ProEq::kView; ++v) { const float f = 20.0f * std::pow (1000.0f, (float) v / (ProEq::kView - 1)); rp.lineTo (xOf (f), yOf (juce::jlimit (-30.0f, 18.0f, ed.proc.eq.specView[v].load()))); }
        rp.lineTo (xOf (20000), yOf (0)); rp.closeSubPath(); g.setColour (juce::Colour (0xffff6b5e).withAlpha (0.3f)); g.fillPath (rp);
    }
    // curve per banda e totale
    juce::Path tot; std::vector<double> sum ((size_t) (getWidth() / 2 + 2), 0.0);
    for (int k = 0; k < kBands; ++k)
    {
        const auto& B = E.b[k]; if (! B.on) continue;
        BQ f[8]; const float gNow = B.g + (B.dyn ? ed.proc.eq.dynUi[k].load() : 0.0f); const int n = designBand (B, sr, gNow, f);
        juce::Path p; int i = 0; bool st = false;
        for (int x = 36; x < getWidth() - 10; x += 2, ++i)
        {
            double d = 0; const float fr = fOf ((float) x); for (int q = 0; q < n; ++q) d += f[q].magDb (fr, sr);
            if (B.ch == 0 || B.ch == 1) sum[(size_t) i] += d;
            const float y = yOf ((float) juce::jlimit (-30.0, 30.0, d));
            if (! st) { p.startNewSubPath ((float) x, y); st = true; } else p.lineTo ((float) x, y);
        }
        const auto c = bandCol (k);
        if (k == ed.band || k == hover) { juce::Path fp = p; fp.lineTo ((float) getWidth() - 12, yOf (0)); fp.lineTo (36, yOf (0)); fp.closeSubPath(); g.setColour (c.withAlpha (0.12f)); g.fillPath (fp); }
        g.setColour (c.withAlpha (k == ed.band ? 0.9f : 0.45f)); g.strokePath (p, juce::PathStrokeType (k == ed.band ? 1.6f : 1.0f));
    }
    { int i = 0; bool st = false; for (int x = 36; x < getWidth() - 10; x += 2, ++i) { const float y = yOf ((float) juce::jlimit (-30.0, 30.0, sum[(size_t) i])); if (! st) { tot.startNewSubPath ((float) x, y); st = true; } else tot.lineTo ((float) x, y); } }
    g.setColour (juce::Colours::white.withAlpha (eqOn ? 0.95f : 0.3f)); g.strokePath (tot, juce::PathStrokeType (2.0f));
    for (int k = 0; k < kBands; ++k)
    {
        const auto& B = E.b[k]; if (! B.on) continue;
        const auto p = nodeOf (k); const auto c = bandCol (k); const bool selB = k == ed.band;
        if (B.dyn && hasGain (B.type)) { const float y1 = yOf (B.g + B.range); g.setColour (c.withAlpha (0.5f)); g.drawLine (p.x, p.y, p.x, y1, 2.0f); g.fillEllipse (p.x - 3, y1 - 3, 6, 6); }
        g.setColour (c.withAlpha (selB ? 0.4f : 0.18f)); g.fillEllipse (p.x - 10, p.y - 10, 20, 20);
        g.setColour (c); g.drawEllipse (p.x - 8, p.y - 8, 16, 16, selB ? 2.0f : 1.2f);
        g.setFont (alook::typed (8.5f)); g.setColour (juce::Colours::white); g.drawText (juce::String (k + 1), juce::Rectangle<float> (p.x - 8, p.y - 7, 16, 14), juce::Justification::centred);
        juce::String tags; if (B.ch > 0) tags << juce::String ("MSLR")[B.ch - 1]; if (B.dyn) tags << "D"; if (B.spec) tags << "S"; if (B.solo) tags << "!";
        if (tags.isNotEmpty()) { g.setFont (alook::typed (7.5f)); g.setColour (c); g.drawText (tags, juce::Rectangle<float> (p.x + 9, p.y - 18, 30, 10), juce::Justification::centredLeft); }
    }
    g.setFont (alook::serif (9.0f)); g.setColour (kDim);
    g.drawText (juce::String::fromUTF8 ("EQ CENTRALE  \xc2\xb7  doppio clic = nuova banda  \xc2\xb7  trascina = frequenza e gain  \xc2\xb7  rotella = Q  \xc2\xb7  tasto destro = opzioni  \xc2\xb7  doppio clic sul punto = elimina"),
                juce::Rectangle<float> (44, 8, getWidth() - 60.0f, 14), juce::Justification::centredLeft);
    if (! eqOn) { g.setFont (alook::serif (12.0f)); g.setColour (juce::Colour (0xffffb347)); g.drawText ("EQ SPENTO (accendilo dalla spia nel rack)", s, juce::Justification::centred); }
}
void EqGraph::mouseMove (const juce::MouseEvent& e) { const int h = hit (e.position); if (h != hover) { hover = h; repaint(); } }
void EqGraph::mouseDown (const juce::MouseEvent& e)
{
    drag = hit (e.position);
    if (drag >= 0) { ed.selectBand (drag); if (e.mods.isPopupMenu()) { bandMenu (drag); drag = -1; return; } }
}
void EqGraph::mouseDrag (const juce::MouseEvent& e)
{
    if (drag < 0) return;
    auto& P = ed.proc; const int t = (int) P.valueS (Processor::bid (drag, "type"));
    P.setP (Processor::bid (drag, "f"), juce::jlimit (10.0f, 22000.0f, fOf (e.position.x)));
    if (hasGain (t)) P.setP (Processor::bid (drag, "g"), juce::jlimit (-30.0f, 30.0f, dbOf (e.position.y)));
}
void EqGraph::mouseDoubleClick (const juce::MouseEvent& e)
{
    auto& P = ed.proc; const int h = hit (e.position);
    if (h >= 0) { P.setP (Processor::bid (h, "on"), 0); P.setP (Processor::bid (h, "solo"), 0); ed.selectBand (-1); return; }
    for (int b = 0; b < kBands; ++b)
        if (P.valueS (Processor::bid (b, "on")) < 0.5f)
        {
            const float f = fOf (e.position.x); const int t = f < 40 ? BT_LCUT : f > 16000 ? BT_HCUT : BT_BELL;
            for (auto* id : { "dyn", "spec", "solo" }) P.setP (Processor::bid (b, id), 0);
            P.setP (Processor::bid (b, "type"), (float) t); P.setP (Processor::bid (b, "f"), f); P.setP (Processor::bid (b, "q"), 1);
            P.setP (Processor::bid (b, "g"), t == BT_BELL ? juce::jlimit (-30.0f, 30.0f, dbOf (e.position.y)) : 0.0f); P.setP (Processor::bid (b, "ch"), 0);
            P.setP (Processor::bid (b, "on"), 1); ed.selectBand (b); return;
        }
}
void EqGraph::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    const int h = hit (e.position); if (h < 0) return;
    auto* p = ed.P (Processor::bid (h, "q")); p->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, p->getValue() + (w.deltaY > 0 ? 0.025f : -0.025f)));
}
void EqGraph::bandMenu (int b)
{
    auto& P = ed.proc; juce::PopupMenu m, tm, cm, sm; const int t = (int) P.valueS (Processor::bid (b, "type")), ch = (int) P.valueS (Processor::bid (b, "ch")), sl = (int) P.valueS (Processor::bid (b, "slope"));
    for (int i = 0; i < kBandTypes; ++i) tm.addItem (100 + i, bandTypeNames()[i], true, i == t);
    for (int i = 0; i < 5; ++i) cm.addItem (200 + i, bandChNames()[i], true, i == ch);
    for (int i = 0; i < 4; ++i) sm.addItem (300 + i, slopeNames()[i] + " dB/ott", true, i == sl);
    m.addSectionHeader ("BANDA " + juce::String (b + 1));
    m.addSubMenu ("Tipo", tm); m.addSubMenu ("Canale", cm); m.addSubMenu ("Pendenza (tagli)", sm, t == BT_LCUT || t == BT_HCUT);
    m.addItem (1, "Dinamica", hasGain (t), P.valueS (Processor::bid (b, "dyn")) > 0.5f);
    m.addItem (2, "Spettrale", hasGain (t), P.valueS (Processor::bid (b, "spec")) > 0.5f);
    m.addItem (3, "Solo", true, P.valueS (Processor::bid (b, "solo")) > 0.5f);
    m.addSeparator(); m.addItem (4, "Elimina");
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [this, b] (int r)
    {
        auto& P = ed.proc; auto tog = [&] (const char* id) { P.setP (Processor::bid (b, id), P.valueS (Processor::bid (b, id)) > 0.5f ? 0.0f : 1.0f); };
        if (r >= 300) P.setP (Processor::bid (b, "slope"), (float) (r - 300));
        else if (r >= 200) P.setP (Processor::bid (b, "ch"), (float) (r - 200));
        else if (r >= 100) P.setP (Processor::bid (b, "type"), (float) (r - 100));
        else if (r == 1) tog ("dyn"); else if (r == 2) tog ("spec");
        else if (r == 3) { const bool on = P.valueS (Processor::bid (b, "solo")) < 0.5f; for (int k = 0; k < kBands; ++k) P.setP (Processor::bid (k, "solo"), 0); P.setP (Processor::bid (b, "solo"), on ? 1.0f : 0.0f); }
        else if (r == 4) { P.setP (Processor::bid (b, "on"), 0); ed.selectBand (-1); }
        ed.bandPanel.rebuild(); repaint();
    });
}

void BandPanel::rebuild()
{
    knobs.clear(); removeAllChildren(); shown = ed.band;
    if (shown < 0 || ed.proc.valueS (Processor::bid (shown, "on")) < 0.5f) { repaint(); return; }
    auto mk = [this] (const char* id, const char* lab, std::function<juce::String()> txt, juce::Rectangle<int> r)
    {
        auto k = std::make_unique<alook::Knob> (ed.P (Processor::bid (shown, id)), lab, 2);
        k->ink = kTxt; k->soft = kDim; k->hot = bandCol (shown); k->numbers = false; k->text = std::move (txt);
        addAndMakeVisible (*k); k->setBounds (r); knobs.push_back (std::move (k));
    };
    const int b = shown; auto& P = ed.proc;
    mk ("f", "FREQ", [&P, b] { return hzT (P.valueS (Processor::bid (b, "f"))); }, { 8, 94, 96, 106 });
    mk ("g", "GAIN", [&P, b] { const float v = P.valueS (Processor::bid (b, "g")); return (v > 0 ? "+" : "") + juce::String (v, 1) + " dB"; }, { 106, 94, 96, 106 });
    mk ("q", "Q", [&P, b] { return juce::String (P.valueS (Processor::bid (b, "q")), 2); }, { 204, 94, 96, 106 });
    mk ("thr", "SOGLIA", [&P, b] { return juce::String (P.valueS (Processor::bid (b, "thr")), 1) + " dB"; }, { 12, 232, 92, 82 });
    mk ("range", "RANGE", [&P, b] { const float v = P.valueS (Processor::bid (b, "range")); return (v > 0 ? "+" : "") + juce::String (v, 1) + " dB"; }, { 108, 232, 92, 82 });
    repaint();
}
void BandPanel::paint (juce::Graphics& g)
{
    darkBox (g, getLocalBounds().toFloat());
    if (shown != ed.band) rebuild();
    if (shown < 0 || ed.proc.valueS (Processor::bid (shown, "on")) < 0.5f)
    {
        title (g, "BANDA", { 12, 8, 200, 18 });
        g.setFont (alook::serif (10.5f, false)); g.setColour (kDim);
        g.drawFittedText ("Doppio clic sul grafico per creare una banda, clic su un punto per modificarla.\n\nDINAMICA: la banda si muove solo quando la sua zona supera la SOGLIA, fino al RANGE.\n\nSPETTRALE: agisce frequenza per frequenza solo sulle risonanze che superano la soglia (aggiunge latenza; spento in LIVE).",
                          getLocalBounds().reduced (14, 30), juce::Justification::topLeft, 12);
        return;
    }
    auto& P = ed.proc; const int b = shown; auto v = [&P, b] (const char* id) { return P.valueS (Processor::bid (b, id)); };
    g.setColour (bandCol (b)); g.setFont (alook::serif (12.0f)); g.drawText ("BANDA " + juce::String (b + 1), juce::Rectangle<float> (12, 8, 200, 20), juce::Justification::centredLeft);
    button (g, typeR, bandTypeNames()[(int) v ("type")] + juce::String::fromUTF8 ("  \xe2\x96\xbe"), true, bandCol (b));
    button (g, chR, bandChNames()[(int) v ("ch")] + juce::String::fromUTF8 ("  \xe2\x96\xbe"), v ("ch") > 0.5f, bandCol (b));
    const int t = (int) v ("type");
    button (g, slopeR, (t == BT_LCUT || t == BT_HCUT) ? slopeNames()[(int) v ("slope")] + " dB/OTT" + juce::String::fromUTF8 ("  \xe2\x96\xbe") : juce::String ("PENDENZA -"), false, bandCol (b));
    button (g, onR, "ATTIVA", v ("on") > 0.5f, bandCol (b));
    button (g, dynR, "DINAMICA", v ("dyn") > 0.5f, bandCol (b)); button (g, specR, "SPETTRALE", v ("spec") > 0.5f, bandCol (b)); button (g, soloR, "SOLO", v ("solo") > 0.5f, juce::Colour (0xffffd43b));
    button (g, delR, "ELIMINA", false, juce::Colour (0xffff6b5e));
    const bool dynLike = v ("dyn") > 0.5f || v ("spec") > 0.5f;
    for (auto& k : knobs) if (k->label == "SOGLIA" || k->label == "RANGE") k->lit = dynLike;
}
void BandPanel::mouseDown (const juce::MouseEvent& e)
{
    if (shown < 0) return; auto& P = ed.proc; const int b = shown;
    auto tog = [&] (const char* id) { P.setP (Processor::bid (b, id), P.valueS (Processor::bid (b, id)) > 0.5f ? 0.0f : 1.0f); repaint(); ed.graph.repaint(); };
    auto menu = [&] (const juce::StringArray& items, const char* id, juce::Rectangle<float> r)
    {
        juce::PopupMenu m; const int cur = (int) P.valueS (Processor::bid (b, id)); for (int i = 0; i < items.size(); ++i) m.addItem (i + 1, items[i], true, i == cur);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea (localAreaToGlobal (r.toNearestInt())), [this, b, id] (int x) { if (x > 0) { ed.proc.setP (Processor::bid (b, id), (float) (x - 1)); repaint(); ed.graph.repaint(); } });
    };
    if (typeR.contains (e.position)) menu (bandTypeNames(), "type", typeR);
    else if (chR.contains (e.position)) menu (bandChNames(), "ch", chR);
    else if (slopeR.contains (e.position)) menu (slopeNames(), "slope", slopeR);
    else if (onR.contains (e.position)) tog ("on");
    else if (dynR.contains (e.position)) tog ("dyn");
    else if (specR.contains (e.position)) tog ("spec");
    else if (soloR.contains (e.position)) { const bool on = P.valueS (Processor::bid (b, "solo")) < 0.5f; for (int k = 0; k < kBands; ++k) P.setP (Processor::bid (k, "solo"), 0); P.setP (Processor::bid (b, "solo"), on ? 1.0f : 0.0f); repaint(); }
    else if (delR.contains (e.position)) { P.setP (Processor::bid (b, "on"), 0); P.setP (Processor::bid (b, "solo"), 0); ed.selectBand (-1); }
}

void GainChip::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff11100f)); g.fillRoundedRectangle (b, 4); g.setColour (kLine.brighter (0.3f)); g.drawRoundedRectangle (b.reduced (0.5f), 4, 1);
    const float v = param->convertFrom0to1 (param->getValue());
    g.setFont (alook::serif (8.0f)); g.setColour (kDim); g.drawText (label, b.withHeight (13).translated (0, 2), juce::Justification::centred);
    g.setFont (alook::typed (11.0f)); g.setColour (std::abs (v) > 0.05f ? juce::Colour (0xffffb347) : kTxt); g.drawText ((v > 0 ? "+" : "") + juce::String (v, 1) + " dB", b.withTrimmedTop (13), juce::Justification::centred);
}

// ======================================================================= editor
Editor::Editor (Processor& p) : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);
    for (auto* c : std::initializer_list<juce::Component*> { &meter, &imager, &rack, &panel, &graph, &bandPanel }) addAndMakeVisible (c);
    browserView.setViewedComponent (&browser, false); browserView.setScrollBarsShown (true, false); browserView.setScrollBarThickness (8); addAndMakeVisible (browserView);
    inChip = std::make_unique<GainChip> (P ("in"), "INPUT"); outChip = std::make_unique<GainChip> (P ("out"), "OUTPUT");
    addAndMakeVisible (*inChip); addAndMakeVisible (*outChip);
    addAndMakeVisible (sizeChip);
    if (proc.chainLen() > 0) sel = 0;
    scaler.setup();
    panel.rebuild();
    startTimerHz (30);
}
void Editor::resized()
{
    scaler.update();
    meter.setBounds (12, 62, 300, 240); imager.setBounds (318, 62, 200, 240); rack.setBounds (524, 62, 964, 240);
    browserView.setBounds (12, 330, 220, 286); browser.setSize (208, browser.getHeight());
    panel.setBounds (238, 308, 1250, 308);
    graph.setBounds (12, 622, 1160, 318); bandPanel.setBounds (1178, 622, 310, 318);
    inChip->setBounds (1130, 12, 72, 34); outChip->setBounds (1208, 12, 72, 34);
    sizeChip.setBounds (W - 76, H - 12, 58, 11);
    jlook::scaleChildren (*this, scaler.s);
}
void Editor::paint (juce::Graphics& g)
{
    g.addTransform (juce::AffineTransform::scale (scaler.s));
    juce::ColourGradient bg (juce::Colour (0xff22211f), 0, 0, kBg, 0, (float) H, false); g.setGradientFill (bg); g.fillRect (0, 0, W, H);
    jlook::grain (g, { 0, 0, W, H }, 0.04f);
    alook::engrave (g, "JACKPOT", { 18, 8, 160, 22 }, alook::wide (17.0f), juce::Justification::centredLeft, juce::Colour (0xffffb347));
    alook::engrave (g, "MIXOLOGY", { 18, 28, 300, 24 }, alook::wide (21.0f), juce::Justification::centredLeft, kTxt);
    {   // preset
        auto r = presetR; g.setColour (juce::Colour (0xff0f0e0d)); g.fillRoundedRectangle (r, 4); g.setColour (kLine.brighter (0.3f)); g.drawRoundedRectangle (r, 4, 1);
        g.setFont (alook::typed (12.5f)); g.setColour (kTxt); g.drawFittedText (proc.presetName.toUpperCase(), r.reduced (10, 0).toNearestInt(), juce::Justification::centred, 1, 0.7f);
        for (auto bb : { prevR, nextR }) { g.setColour (juce::Colour (0xff2a2926)); g.fillEllipse (bb); g.setColour (kTxt); g.setFont (alook::serif (16.0f)); g.drawText (bb == prevR ? juce::String::fromUTF8 ("\xe2\x80\xb9") : juce::String::fromUTF8 ("\xe2\x80\xba"), bb, juce::Justification::centred); }
        alook::jewel (g, abA.getCentreX(), abA.getCentreY(), 7, juce::Colour (0xffffb347), proc.ab.cur == 0); alook::jewel (g, abB.getCentreX(), abB.getCentreY(), 7, juce::Colour (0xffffb347), proc.ab.cur == 1);
        g.setFont (alook::serif (8.0f)); g.setColour (kDim); g.drawText ("A", abA.translated (0, 18), juce::Justification::centred); g.drawText ("B", abB.translated (0, 18), juce::Justification::centred);
    }
    button (g, liveR, "LIVE", proc.value ("live") > 0.5f, juce::Colour (0xffff6b5e));
    button (g, cmpR, "CONFRONTA", proc.value ("cmp") > 0.5f, juce::Colour (0xff4dabf7));
    {   // stato del sidechain
        const bool sc = proc.scConnected.load();
        g.setColour (juce::Colour (0xff0f0e0d)); g.fillRoundedRectangle (scR, 4); g.setColour (kLine); g.drawRoundedRectangle (scR, 4, 1);
        alook::jewel (g, scR.getX() + 14, scR.getCentreY(), 5, juce::Colour (0xff7dde5a), sc);
        g.setFont (alook::serif (9.0f)); g.setColour (sc ? kTxt : kDim); g.drawText (sc ? "SIDECHAIN COLLEGATO" : "SIDECHAIN NON COLLEGATO", scR.withTrimmedLeft (26), juce::Justification::centredLeft);
    }
    title (g, "BROWSER  \xc2\xb7  clic = aggiungi, trascina nel rack", { 14, 308, 220, 20 });
}
void Editor::presetMenu()
{
    juce::PopupMenu m; std::map<juce::String, juce::PopupMenu> cats; juce::StringArray order;
    const auto& F = factoryPresets();
    for (int i = 0; i < (int) F.size(); ++i) { if (! order.contains (F[(size_t) i].cat)) order.add (F[(size_t) i].cat); cats[F[(size_t) i].cat].addItem (1 + i, F[(size_t) i].name + "   -   " + F[(size_t) i].note.upToFirstOccurrenceOf (":", false, false).substring (0, 60), true, i == proc.preset); }
    for (auto& c : order) m.addSubMenu (c, cats[c]);
    m.addSeparator();
    juce::PopupMenu mine; auto files = Processor::userPresetDir().findChildFiles (juce::File::findFiles, false, "*.xml"); files.sort();
    for (int i = 0; i < files.size(); ++i) mine.addItem (5000 + i, files[i].getFileNameWithoutExtension());
    if (files.isEmpty()) mine.addItem (4999, "(nessuno)", false);
    m.addSubMenu ("I MIEI PRESET", mine);
    m.addItem (4000, "Salva preset...");
    m.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea (localAreaToGlobal (scaler.map (presetR.toNearestInt()))), [this, files] (int r)
    {
        if (r <= 0) return;
        if (r == 4000)
        {
            auto* aw = new juce::AlertWindow ("Salva preset", "Nome del preset:", juce::MessageBoxIconType::NoIcon);
            aw->addTextEditor ("n", proc.presetName.fromFirstOccurrenceOf ("| ", false, false)); aw->addButton ("Salva", 1, juce::KeyPress (juce::KeyPress::returnKey)); aw->addButton ("Annulla", 0);
            aw->enterModalState (true, juce::ModalCallbackFunction::create ([this, aw] (int res) { if (res == 1) { proc.saveUserPreset (aw->getTextEditorContents ("n")); repaint(); } }), true);
            return;
        }
        if (r >= 5000) { proc.loadUserPreset (files[r - 5000]); }
        else proc.loadPreset (r - 1);
        sel = proc.chainLen() > 0 ? 0 : -1; band = -1; panel.rebuild(); bandPanel.rebuild(); repaint();
    });
}
void Editor::mouseDown (const juce::MouseEvent& e0)
{
    const auto e = e0.withNewPosition (e0.position / scaler.s);
    const int n = (int) factoryPresets().size();
    auto after = [this] { sel = proc.chainLen() > 0 ? 0 : -1; band = -1; panel.rebuild(); bandPanel.rebuild(); repaint(); };
    if (prevR.contains (e.position)) { proc.loadPreset ((proc.preset + n - 1) % n); after(); return; }
    if (nextR.contains (e.position)) { proc.loadPreset ((proc.preset + 1) % n); after(); return; }
    if (presetR.contains (e.position)) { presetMenu(); return; }
    if (abA.expanded (4).contains (e.position)) { if (proc.ab.cur != 0) proc.ab.toggle (proc); after(); return; }
    if (abB.expanded (4).contains (e.position)) { if (proc.ab.cur != 1) proc.ab.toggle (proc); after(); return; }
    if (liveR.contains (e.position)) { auto* p = P ("live"); p->setValueNotifyingHost (p->getValue() > 0.5f ? 0.0f : 1.0f); repaint(); return; }
    if (cmpR.contains (e.position)) { auto* p = P ("cmp"); p->setValueNotifyingHost (p->getValue() > 0.5f ? 0.0f : 1.0f); repaint(); return; }
}
void Editor::timerCallback()
{
    ++tick;
    meter.repaint(); imager.repaint();
    if (tick % 2 == 0) graph.tick();
    rack.repaint();
    const int s = sel; const int t = s >= 0 && s < proc.chainLen() ? proc.slotType (s) : 0;
    if (s != panel.shownSlot || t != panel.shownType) panel.rebuild();
    if (panel.shownType > 0) { panel.repaint (juce::Rectangle<int> (0, 60, 240, 248)); if (tick % 3 == 0) for (auto& c : panel.ctl) c->repaint(); if (panel.mixK) panel.mixK->repaint(); }
    if (tick % 3 == 0) { for (auto& k : bandPanel.knobs) k->repaint(); bandPanel.repaint(); inChip->repaint(); outChip->repaint(); }
    if (proc.chainLen() != lastN) { lastN = proc.chainLen(); if (sel >= lastN) selectSlot (lastN - 1); }
    if (tick % 15 == 0) repaint (scaler.map ({ 300, 0, 1200, 56 }));
}
}
