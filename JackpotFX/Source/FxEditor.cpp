// JACKPOT FX: frontale come Dust e Alter. Testata e preset nella pelle comune, piastre per slot, manopole da hardware,
// onda SCAN del Master con le corsie degli slot.
#include "FxEditor.h"

namespace jfx
{
using namespace pfx;
static juce::String U (const char* s) { return juce::String::fromUTF8 (s); }
juce::Font uf (float size, bool bold) { return jlook::ui (size, bold ? 2 : 1); }
juce::Font mf (float size) { return jlook::mono (size); }
void glowText (juce::Graphics& g, const juce::String& t, juce::Rectangle<float> r, juce::Colour c, juce::Font f, juce::Justification j)
{
    g.setFont (f);
    g.setColour (c.withAlpha (0.2f)); g.drawText (t, r.translated (-0.7f, 0.3f), j); g.drawText (t, r.translated (0.7f, 0.3f), j);
    g.setColour (c); g.drawText (t, r, j);
}
void lcd (juce::Graphics& g, juce::Rectangle<float> r, float rad)
{
    if (jlook::skinHooks().on && jlook::skinHooks().screen) { jlook::skinHooks().screen (g, r, rad, C::cyan); return; }
    g.setColour (juce::Colours::black.withAlpha (0.65f)); g.fillRoundedRectangle (r.expanded (1.5f), rad + 1.5f);
    juce::ColourGradient cg (C::lcd1, r.getCentreX(), r.getCentreY(), C::lcd2, r.getX(), r.getY(), true); g.setGradientFill (cg); g.fillRoundedRectangle (r, rad);
}
juce::Colour groupCol (int t)
{
    const juce::String g (info (t).group);
    if (g == "FILTRI") return C::cyan; if (g == "SPAZIO") return C::violet; if (g == "TEMPO") return C::amber;
    if (g == "SPORCO") return C::pink; if (g == "MODULAZIONE") return C::mint;
    if (g == "TRANSIZIONI") return C::red; if (g == "TONO") return C::blue; return C::faint;
}
// piastra dello slot: scura, con una punta del colore della famiglia dell'effetto
static juce::Colour plate1 (int t) { return groupCol (t).interpolatedWith (juce::Colour (0xff202228), 0.84f); }
static juce::Colour plate2 (int t) { return groupCol (t).interpolatedWith (juce::Colour (0xff0d0e11), 0.9f); }

// ======================================================================= manopola
void Knob::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isAltDown() || e.mods.isCommandDown()) { mouseDoubleClick (e); return; }
    start = param->getValue(); param->beginChangeGesture(); drag = true; repaint();
}
void Knob::mouseUp (const juce::MouseEvent&)
{
    if (drag) param->endChangeGesture();
    drag = false; repaint();
    if (onRelease) onRelease();
}
void Knob::mouseDrag (const juce::MouseEvent& e)
{
    if (! drag) return;
    param->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, start - e.getDistanceFromDragStartY() / (e.mods.isShiftDown() ? 900.0f : 220.0f)));
}
void Knob::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w)
{
    param->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, param->getValue() + (w.deltaY > 0 ? 0.01f : -0.01f)));
}
void Knob::paint (juce::Graphics& g)
{
    auto s = spec; s.enabled = lit; s.hover = hover; s.drag = drag; s.text = text ? text() : param->getCurrentValueAsText();
    hw::knob (g, getLocalBounds().toFloat(), param->getValue(), s);
}

// ======================================================================= onda SCAN
void Scope::paint (juce::Graphics& g)
{
    auto& P = ed.proc; const auto b = getLocalBounds().toFloat();
    lcd (g, b, 8);
    const int nl = kSlots; const float laneH = 5.0f, laneGap = 2.0f, lanesH = nl * (laneH + laneGap) + 6;
    auto area = b.reduced (12, 10).withTrimmedBottom (lanesH + 14);
    auto lanes = juce::Rectangle<float> (area.getX(), area.getBottom() + 14, area.getWidth(), lanesH - 6);
    const long tot = P.hTot.load(); if (tot <= 2) return;
    const int N = Processor::kHist; const long newest = tot - 1;
    auto ring = [N] (long a) { return (int) (((a % N) + N) % N); };
    const float lastQ = P.hPpq[ring (newest)].load();
    const bool beats = lastQ >= 0;
    // la finestra: 2 battute quando la DAW suona (agganciata alla battuta), 4 secondi da ferma
    const double winB = 8.0, winCols = beats ? winB * 60.0 / juce::jmax (40.0, P.uiBpm.load()) / Processor::kColSec : 4.0 / Processor::kColSec;
    const double cursor = beats ? std::fmod ((double) lastQ, winB) / winB : std::fmod ((double) newest, winCols) / winCols;
    const float cy = area.getCentreY(), hh = area.getHeight() * 0.5f - 2;
    // griglia: battiti o secondi
    if (beats) for (int q = 0; q <= 8; ++q) { const float x = area.getX() + area.getWidth() * q / 8.0f; g.setColour (C::text.withAlpha (q % 4 == 0 ? 0.12f : 0.05f)); g.fillRect (x, area.getY(), 1.0f, area.getHeight()); }
    else for (int s = 0; s <= 4; ++s) { const float x = area.getX() + area.getWidth() * s / 4.0f; g.setColour (C::text.withAlpha (0.06f)); g.fillRect (x, area.getY(), 1.0f, area.getHeight()); }
    g.setColour (C::text.withAlpha (0.06f)); g.fillRect (area.getX(), cy, area.getWidth(), 1.0f);
    // colonne: per ogni pixel il picco delle colonne che ci cadono (stessa idea del Master)
    const int npx = juce::jmax (16, (int) area.getWidth());
    std::vector<float> pin ((size_t) npx, -1.0f), pout ((size_t) npx, -1.0f); std::vector<int> pm ((size_t) npx, 0);
    const int back = (int) juce::jmin ((double) N - 2, winCols * 1.02);
    for (int j = 0; j < back && newest - j >= 0; ++j)
    {
        const long a = newest - j; const int r = ring (a);
        double rel;
        if (beats) { const float q = P.hPpq[r].load(); if (q < 0 || lastQ - q > winB - 0.02) break; rel = std::fmod ((double) q, winB) / winB; }
        else { if (j > winCols - 2) break; rel = std::fmod ((double) a, winCols) / winCols; }
        // subito dopo il cursore un piccolo vuoto: si vede dove sta scrivendo
        double ahead = rel - cursor; if (ahead < 0) ahead += 1.0;
        if (j > 0 && ahead > 0.0 && ahead < 0.012) continue;
        const int x = juce::jlimit (0, npx - 1, (int) (rel * npx));
        pin[(size_t) x] = juce::jmax (pin[(size_t) x], P.hIn[r].load()); pout[(size_t) x] = juce::jmax (pout[(size_t) x], P.hOut[r].load()); pm[(size_t) x] |= P.hMask[r].load();
    }
    // i pixel vuoti fra due colonne (finestra lunga, zoom) prendono il vicino
    for (int x = 1; x < npx; ++x) if (pin[(size_t) x] < 0 && pin[(size_t) x - 1] >= 0 && x + 1 < npx && pin[(size_t) x + 1] >= 0) { pin[(size_t) x] = pin[(size_t) x - 1]; pout[(size_t) x] = pout[(size_t) x - 1]; pm[(size_t) x] = pm[(size_t) x - 1]; }
    auto shape = [&] (const std::vector<float>& v)
    {
        juce::Path path; int x = 0; const float dx = area.getWidth() / npx;
        auto lin = [] (float a) { return juce::jlimit (0.0f, 1.05f, a); };
        while (x < npx)
        {
            while (x < npx && v[(size_t) x] < 0) ++x; if (x >= npx) break;
            const int s0 = x; while (x < npx && v[(size_t) x] >= 0) ++x; const int s1 = x;
            path.startNewSubPath (area.getX() + s0 * dx, cy);
            for (int k = s0; k < s1; ++k) path.lineTo (area.getX() + (k + 0.5f) * dx, cy - lin (v[(size_t) k]) * hh);
            path.lineTo (area.getX() + s1 * dx, cy);
            for (int k = s1 - 1; k >= s0; --k) path.lineTo (area.getX() + (k + 0.5f) * dx, cy + lin (v[(size_t) k]) * hh);
            path.closeSubPath();
        }
        return path;
    };
    const auto wc = C::pink.interpolatedWith (C::violet, 0.35f);
    const auto po = shape (pout), pi = shape (pin);
    g.setColour (C::text.withAlpha (0.13f)); g.fillPath (pi);                                    // quello che entra, dietro e scuro
    g.setColour (wc.withAlpha (0.72f)); g.fillPath (po);                                         // quello che esce dagli effetti, sopra
    g.setColour (wc.brighter (0.5f).withAlpha (0.9f)); g.strokePath (po, juce::PathStrokeType (1.0f));
    // cursore
    { const float x = area.getX() + (float) cursor * area.getWidth(); g.setColour (C::text.withAlpha (0.55f)); g.fillRect (x, area.getY(), 1.0f, area.getHeight()); }
    // corsie degli slot: dove ogni effetto ha lavorato
    const float dx = lanes.getWidth() / npx;
    for (int k = 0; k < nl; ++k)
    {
        const float y = lanes.getY() + k * (laneH + laneGap);
        g.setColour (C::text.withAlpha (0.04f)); g.fillRect (lanes.getX(), y, lanes.getWidth(), laneH);
        const auto c = groupCol (P.slotType (k));
        g.setColour (c.withAlpha (0.9f));
        for (int x = 0; x < npx; )
        {
            if (! (pm[(size_t) x] & (1 << k))) { ++x; continue; }
            const int s0 = x; while (x < npx && (pm[(size_t) x] & (1 << k))) ++x;
            g.fillRect (lanes.getX() + s0 * dx, y, (x - s0) * dx, laneH);
        }
    }
    g.setFont (mf (8.0f)); g.setColour (C::dim);
    if (beats) for (int q = 0; q < 2; ++q) g.drawText (juce::String ((int) std::floor (lastQ / 4.0) - ((int) std::floor (std::fmod ((double) lastQ, winB) / 4.0)) + q + 1), juce::Rectangle<float> (area.getX() + area.getWidth() * q / 2.0f + 4, area.getY() + 2, 30, 10), juce::Justification::centredLeft);
}

// ======================================================================= scheda di uno slot
SlotCard::SlotCard (Editor& e, int kk) : ed (e), k (kk)
{
    auto* mp = ed.proc.P (mixId (k));
    mixK = std::make_unique<Knob> (mp); mixK->spec.label = "MIX"; mixK->spec.style = 1; mixK->defV = 1.0f;
    mixK->text = [mp] { return juce::String (juce::roundToInt (mp->getValue() * 100)) + "%"; };
    mixK->setTooltip ("Mix di questo slot: quanto del suo effetto si sente. Doppio clic = 100%.");
    addAndMakeVisible (*mixK);
    refresh();
}
void SlotCard::refresh()
{
    const int t = ed.proc.slotType (k); shownType = t;
    const auto& I = info (t); const auto c = groupCol (t);
    auto* pr = ed.proc.P (amtId (k));
    knob = std::make_unique<Knob> (pr);
    knob->spec.label = I.name; knob->spec.col = c; knob->spec.style = 0; knob->spec.big = true; knob->spec.bipolar = I.bipolar;
    knob->defV = I.neutral;
    knob->text = [pr, t] { return valueText (t, pr->getValue()); };
    if (I.spring) knob->onRelease = [pr, t] { pr->setValueNotifyingHost (info (t).neutral); };
    knob->setTooltip (juce::String (I.tip) + "  Doppio clic = neutro.");
    addAndMakeVisible (*knob);
    mixK->spec.col = c;
    resized(); repaint();
}
void SlotCard::resized()
{
    auto b = getLocalBounds().toFloat();
    railR = b.withHeight (32);
    nameR = { 10, 42, b.getWidth() - 20, 26 };
    if (knob) knob->setBounds (juce::Rectangle<float> (8, 84, b.getWidth() - 16, 150).toNearestInt());
    gearR = { 12, 262, 40, 40 };
    if (mixK) mixK->setBounds (juce::Rectangle<float> (b.getWidth() - 86, 246, 76, 92).toNearestInt());
    ccR = { 10, b.getHeight() - 40, (b.getWidth() - 26) * 0.5f, 26 };
    learnR = { ccR.getRight() + 6, ccR.getY(), ccR.getWidth(), 26 };
}
juce::String SlotCard::key() const
{
    auto& P = ed.proc; juce::String s; s << skin::cur() << "/" << P.slotType (k) << "/" << (int) (P.value (onId (k)) > 0.5f) << "/" << (int) dropHot << "/" << (int) (ed.advPanel.isVisible() && ed.advPanel.k == k);
    const auto& sp = adv (P.slotType (k)); for (int i = 0; i < sp.n; ++i) s << (std::abs (P.value (advId (k, i)) - sp.def[i]) > 0.003f ? "1" : "0");
    return s;
}
void SlotCard::paint (juce::Graphics& g)
{
    auto& P = ed.proc; const int t = P.slotType (k); const bool on = P.value (onId (k)) > 0.5f;
    const auto c = groupCol (t); const auto& I = info (t);
    const auto bnd = getLocalBounds();
    base.draw (g, bnd, key(), [&] (juce::Graphics& bg)
    {
        auto b = bnd.toFloat();
        hw::faceplate (bg, b, plate1 (t), plate2 (t), on ? c : C::faint, 32, 40 + k, 12);
        bg.setColour (C::text.withAlpha (on ? 1.0f : 0.55f)); bg.setFont (mf (15.0f)); bg.drawText (juce::String (k + 1).paddedLeft ('0', 2), juce::Rectangle<float> (14, 6, 34, 22), juce::Justification::centredLeft);
        bg.setColour (C::dim); bg.setFont (uf (8.0f, false)); bg.drawText (U ("\xe2\x87\x84 trascina"), juce::Rectangle<float> (44, 8, 70, 18), juce::Justification::centredLeft);
        hw::jewel (bg, b.getRight() - 22, 19, 5.0f, on, c);
        // effetto scelto: si apre il menu
        lcd (bg, nameR, 6);
        glowText (bg, juce::String (I.name) + U ("  \xe2\x96\xbe"), nameR, on ? c : C::dim, uf (10.5f), juce::Justification::centred);
        if (I.spring) { bg.setColour (C::amber.withAlpha (0.85f)); bg.setFont (uf (7.5f)); bg.drawText ("TORNA A ZERO", juce::Rectangle<float> (0, 70, b.getWidth(), 12), juce::Justification::centred); }
        else if (familyOfMix (t) >= 0) { bg.setColour (C::dim); bg.setFont (uf (7.5f)); bg.drawText ("SECONDA MANOPOLA", juce::Rectangle<float> (0, 70, b.getWidth(), 12), juce::Justification::centred); }
        // ingranaggio: parametri avanzati
        const auto& sp = adv (t); const bool has = sp.n > 0, open = ed.advPanel.isVisible() && ed.advPanel.k == k;
        bool changed = false; for (int i = 0; i < sp.n; ++i) if (std::abs (P.value (advId (k, i)) - sp.def[i]) > 0.003f) changed = true;
        jlook::button (bg, gearR, c, open, false, {}, uf (8.0f));
        const auto gc = has ? (open ? c : C::text.withAlpha (0.8f)) : C::faint.withAlpha (0.5f);
        const float cx = gearR.getCentreX(), cy = gearR.getCentreY();
        juce::Path gp; for (int i = 0; i < 8; ++i) { gp.addRectangle (-1.6f, -9.0f, 3.2f, 4.0f); gp.applyTransform (juce::AffineTransform::rotation (juce::MathConstants<float>::twoPi / 8)); }
        gp.applyTransform (juce::AffineTransform::translation (cx, cy)); bg.setColour (gc); bg.fillPath (gp); bg.drawEllipse (cx - 6, cy - 6, 12, 12, 2.0f);
        if (changed) { bg.setColour (C::amber); bg.fillEllipse (gearR.getRight() - 9, gearR.getY() + 4, 5, 5); }
        bg.setColour (has ? C::dim : C::faint); bg.setFont (uf (7.5f)); bg.drawText ("AVANZATO", juce::Rectangle<float> (gearR.getX() - 10, gearR.getBottom() + 3, gearR.getWidth() + 20, 10), juce::Justification::centred);
        if (! on) { bg.setColour (juce::Colours::black.withAlpha (0.32f)); bg.fillRoundedRectangle (b.reduced (5).withTrimmedTop (32).withTrimmedBottom (48), 8); }
        if (dropHot) { bg.setColour (C::cyan.withAlpha (0.12f)); bg.fillRoundedRectangle (b.reduced (2), 12); bg.setColour (C::cyan); bg.drawRoundedRectangle (b.reduced (2), 12, 1.5f); }
    });
    // controller: si ridisegna a parte (lampeggia quando arriva il CC)
    const int cc = P.ccMap[k].load(); const bool learn = P.learnSlot.load() == k;
    const bool blink = learn && (juce::Time::getMillisecondCounter() / 300) % 2 == 0;
    lcd (g, ccR, 5);
    if (flash > 0.02f) { g.setColour (c.withAlpha (0.35f * flash)); g.fillRoundedRectangle (ccR, 5); }
    glowText (g, cc >= 0 ? "CC " + juce::String (cc) : juce::String ("--"), ccR, cc >= 0 ? c : C::dim, mf (10.0f), juce::Justification::centred);
    jlook::button (g, learnR, C::red, learn && blink, false, learn ? "MUOVI..." : "LEARN", uf (8.5f));
}
void SlotCard::mouseMove (const juce::MouseEvent& e)
{
    const bool hand = railR.contains (e.position) || nameR.contains (e.position) || gearR.contains (e.position) || ccR.contains (e.position) || learnR.contains (e.position);
    setMouseCursor (railR.withTrimmedRight (40).contains (e.position) ? juce::MouseCursor::DraggingHandCursor : hand ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
}
void SlotCard::mouseDown (const juce::MouseEvent& e)
{
    auto& P = ed.proc;
    if (juce::Rectangle<float> (getWidth() - 40.0f, 0, 40, 34).contains (e.position)) { auto* p = P.P (onId (k)); p->setValueNotifyingHost (p->getValue() > 0.5f ? 0.0f : 1.0f); repaint(); return; }
    if (gearR.contains (e.position))
    {
        if (adv (P.slotType (k)).n == 0) return;
        if (ed.advPanel.isVisible() && ed.advPanel.k == k) ed.advPanel.setVisible (false); else ed.advPanel.open (k);
        for (auto* c : ed.cards) c->repaint(); return;
    }
    if (nameR.contains (e.position) || e.mods.isPopupMenu() || ccR.contains (e.position)) { showMenu(); return; }
    if (learnR.contains (e.position)) { P.learnSlot = P.learnSlot.load() == k ? -1 : k; for (auto* c : ed.cards) c->repaint(); return; }
    if (railR.contains (e.position)) { dragging = true; ed.dragFrom = k; }
}
void SlotCard::mouseDrag (const juce::MouseEvent& e)
{
    if (! dragging) return;
    ed.dragPos = e.getEventRelativeTo (&ed).position / ed.scaler.s;
    for (auto* c : ed.cards) { const bool hot = c != this && c->getBoundsInParent().toFloat().contains (ed.dragPos); if (hot != c->dropHot) { c->dropHot = hot; c->repaint(); } }
    ed.repaint();
}
void SlotCard::mouseUp (const juce::MouseEvent& e)
{
    if (dragging) { dragging = false; ed.endDrag (e.getEventRelativeTo (&ed).position / ed.scaler.s); }
}
void SlotCard::showMenu()
{
    auto& P = ed.proc; const int ty = P.slotType (k); const int cc = P.ccMap[k].load();
    juce::PopupMenu m; m.addSectionHeader ("SLOT " + juce::String (k + 1) + (cc >= 0 ? "  -  CC " + juce::String (cc) : juce::String ("  -  nessun CC")));
    std::map<juce::String, juce::PopupMenu> groups; juce::StringArray order;
    for (int t = 1; t < kNumTypes; ++t)
    {
        const juce::String gname (info (t).group); if (! order.contains (gname)) order.add (gname);
        groups[gname].addItem (100 + t, juce::String (info (t).name) + (info (t).spring ? "   (torna a zero)" : ""), true, t == ty);
    }
    for (auto& gname : order) m.addSubMenu (gname, groups[gname]);
    m.addItem (100, "Vuoto (OFF)", true, ty == 0);
    m.addSeparator();
    m.addItem (1, "MIDI LEARN: muovi una manopola, un fader o un pad in modo CC");
    m.addItem (2, "Scollega dal controller");
    m.addItem (3, "Tutti come di fabbrica (CC 70-77, Akai MPK)");
    juce::Component::SafePointer<SlotCard> sp (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [sp] (int r)
    {
        if (sp == nullptr || r <= 0) return;
        auto& PP = sp->ed.proc;
        if (r >= 100) { PP.setSlot (sp->k, r - 100, -1); sp->refresh(); }
        else if (r == 1) PP.learnSlot = sp->k;
        else if (r == 2) PP.ccMap[sp->k] = -1;
        else if (r == 3) for (int j = 0; j < kSlots; ++j) PP.ccMap[j] = 70 + j;
        for (auto* c : sp->ed.cards) c->repaint();
    });
}
void SlotCard::tick()
{
    auto& P = ed.proc;
    if (P.slotType (k) != shownType) refresh();
    const bool on = P.value (onId (k)) > 0.5f;
    if (knob) knob->refresh (on);
    mixK->refresh (on);
    const int seen = P.ccSeen[k].load();
    if (seen != lastSeen) { lastSeen = seen; flash = 1.0f; }
    if (flash > 0.01f || P.learnSlot.load() == k) { flash *= 0.85f; repaint (ccR.getUnion (learnR).expanded (2).toNearestInt()); }
    if (base.key != key()) repaint();
}

// ======================================================================= pannello avanzato
void AdvPanel::open (int kk)
{
    k = kk; shownType = ed.proc.slotType (k); verbZone = -1;
    knobs.clear();
    const auto& sp = adv (shownType); const auto c = groupCol (shownType);
    for (int i = 0; i < sp.n; ++i)
    {
        auto* pr = ed.proc.P (advId (k, i)); const int t = shownType;
        auto kn = std::make_unique<Knob> (pr);
        kn->spec.label = sp.name[i]; kn->spec.col = c; kn->spec.style = 1; kn->defV = sp.def[i];
        kn->text = [pr, t, i] { return advText (t, i, pr->getValue()); };
        kn->setTooltip ("Doppio clic = valore base dell'effetto. Shift mentre trascini = movimento fine.");
        addAndMakeVisible (*kn); knobs.push_back (std::move (kn));
    }
    resized(); setVisible (true); toFront (false); repaint();
}
void AdvPanel::resized()
{
    auto b = getLocalBounds().toFloat();
    closeR = { b.getRight() - 52, 6, 36, 22 }; resetR = { b.getRight() - 172, 6, 112, 22 };
    const int n = (int) knobs.size(); if (n == 0) return;
    const int per = n <= 6 ? n : (n + 1) / 2, rows = n <= 6 ? 1 : 2;
    const float kw = n <= 6 ? 120.0f : 96.0f, kh = n <= 6 ? 150.0f : 108.0f, gap = n <= 6 ? 26.0f : 18.0f;
    const float tot = per * kw + (per - 1) * gap, x0 = b.getCentreX() - tot / 2, y0 = rows == 1 ? 70.0f : 46.0f;
    for (int i = 0; i < n; ++i) { const int r = i / per, c = i % per; knobs[(size_t) i]->setBounds (juce::Rectangle<float> (x0 + c * (kw + gap), y0 + r * (kh + 6), kw, kh).toNearestInt()); }
}
void AdvPanel::paint (juce::Graphics& g)
{
    if (k < 0) return;
    auto b = getLocalBounds().toFloat(); const int t = ed.proc.slotType (k); const auto c = groupCol (t);
    hw::faceplate (g, b, plate1 (t), plate2 (t), c, 32, 60 + k, 14);
    g.setColour (C::text); g.setFont (uf (12.0f)); g.drawText ("AVANZATO  " + juce::String (k + 1).paddedLeft ('0', 2) + U ("  \xc2\xb7  ") + info (t).name, juce::Rectangle<float> (16, 4, 400, 26), juce::Justification::centredLeft);
    g.setColour (C::dim); g.setFont (uf (9.0f, false));
    g.drawText (U ("solo con il mouse  \xc2\xb7  la manopola del controller resta quella grande  \xc2\xb7  doppio clic = valore base"), juce::Rectangle<float> (30, b.getBottom() - 24, b.getWidth() - 60, 16), juce::Justification::centredLeft);
    jlook::button (g, resetR, C::amber, false, false, "RIPRISTINA", uf (9.0f));
    jlook::button (g, closeR, C::pink, false, false, U ("\xc3\x97"), uf (13.0f));
}
void AdvPanel::mouseDown (const juce::MouseEvent& e)
{
    if (closeR.contains (e.position)) { setVisible (false); for (auto* c : ed.cards) c->repaint(); return; }
    if (resetR.contains (e.position)) { ed.proc.resetAdv (k); repaint(); if (k >= 0) ed.cards[k]->repaint(); }
}
void AdvPanel::tick()
{
    if (! isVisible() || k < 0) return;
    if (ed.proc.slotType (k) != shownType) { if (adv (ed.proc.slotType (k)).n == 0) setVisible (false); else open (k); return; }
    if (shownType == REVERB)
    {   // cambiando TIPO il tempo e il pre-delay vanno a quelli tipici del tipo (poi si regolano a piacere)
        const int z = zoneOf (ed.proc.P (advId (k, 0))->getValue(), Verb::kTypes);
        if (verbZone >= 0 && z != verbZone)
        {
            float dec, pre; Verb::typeDefaults (z, dec, pre);
            ed.proc.setAdv (k, 1, (float) (std::log (dec / 0.2) / std::log (100.0))); ed.proc.setAdv (k, 2, pre / 250.0f);
        }
        verbZone = z;
    }
    for (auto& kn : knobs) kn->refresh (true);
}

// ======================================================================= editor
Editor::Editor (Processor& p) : AudioProcessorEditor (p), proc (p)
{
    skin::init ("fx", { 0xffe58aa0, 0xffb9a6e0, 0xffe9a260, 0xffa8bea0 }, 0xffff2e88); C::sync();
    setLookAndFeel (&lnf);
    addAndMakeVisible (scope);
    for (int k = 0; k < kSlots; ++k) addAndMakeVisible (cards.add (new SlotCard (*this, k)));
    addChildComponent (advPanel);
    mixK = std::make_unique<Knob> (proc.P ("mix")); mixK->spec.label = "MIX"; mixK->spec.style = 1; mixK->spec.col = C::pink; mixK->defV = 1.0f;
    mixK->text = [this] { return juce::String (juce::roundToInt (proc.value ("mix") * 100)) + "%"; };
    outK = std::make_unique<Knob> (proc.P ("out")); outK->spec.label = "OUTPUT"; outK->spec.style = 1; outK->spec.col = C::pink; outK->spec.bipolar = true;
    outK->text = [this] { const float v = proc.value ("out"); return (v > 0.05f ? "+" : "") + juce::String (v, 1) + " dB"; };
    addAndMakeVisible (*mixK); addAndMakeVisible (*outK);
    addAndMakeVisible (sizeChip);
    setupBrowser();
    scaler.setup();
    startTimerHz (30);
}
void Editor::resized()
{
    const float s = scaler.update();
    presetR = { 372, 15, 330, 36 }; prevR = { 334, 15, 36, 36 }; nextR = { 704, 15, 36, 36 };
    resetR = { 752, 15, 100, 36 }; bypR = { 858, 15, 100, 36 }; ctrlR = { 964, 15, 100, 36 }; themeR = { 1078, 19, 100, 28 };
    const auto sc = scopeCard(); scope.setBounds (juce::Rectangle<float> (sc.getX() + 16, sc.getY() + 42, sc.getWidth() - 32, sc.getHeight() - 56).toNearestInt());
    advPanel.setBounds (sc.toNearestInt());
    const auto oc = outCard();
    mixK->setBounds (juce::Rectangle<float> (oc.getX() + 24, oc.getY() + 44, 104, 130).toNearestInt());
    outK->setBounds (juce::Rectangle<float> (oc.getRight() - 128, oc.getY() + 44, 104, 130).toNearestInt());
    for (int k = 0; k < kSlots; ++k) cards[k]->setBounds (card (k).toNearestInt());
    sizeChip.setBounds (W - 76, H - 16, 58, 14);
    browser.setBounds (0, 0, W, H);
    jlook::scaleChildren (*this, s);
}
juce::String Editor::staticKey()
{
    juce::String k; k << skin::cur() << "/" << proc.presetName << "/" << (int) (proc.value ("bypass") > 0.5f) << "/" << (int) proc.ctrlOn.load() << "/" << hoverBtn;
    return k;
}
void Editor::paintStatic (juce::Graphics& g)
{
    const float Wf = (float) W, Hf = (float) H;
    jlook::backdrop (g, { 0, 0, Wf, Hf }, C::pink, C::violet);
    jlook::header (g, { 0, 0, Wf, 64 }, "FX", C::pink, "JFX-8  PERFORMANCE RACK / 8 SLOT / CONTROLLER", C::violet, C::pink);
    lcd (g, presetR.toFloat(), 8); jlook::glass (g, presetR.toFloat(), 8);
    {
        const bool grp = proc.presetName.contains (" | ");
        const auto nm = grp ? proc.presetName.fromFirstOccurrenceOf (" | ", false, false) : proc.presetName;
        const auto gr = grp ? proc.presetName.upToFirstOccurrenceOf (" | ", false, false) : juce::String ("I MIEI");
        auto pr = presetR.toFloat();
        g.setColour (C::dim); g.setFont (uf (7.5f)); g.drawText (gr, pr.withWidth (70).translated (12, 0), juce::Justification::centredLeft);
        glowText (g, nm.toUpperCase(), pr, C::pink, mf (12.5f), juce::Justification::centred);
    }
    g.setColour (C::pink); g.setFont (uf (20.0f));
    g.drawText (U ("\xe2\x80\xb9"), prevR, juce::Justification::centred); g.drawText (U ("\xe2\x80\xba"), nextR, juce::Justification::centred);
    jlook::button (g, resetR.toFloat(), C::amber, false, hoverBtn == 0, "AZZERA", uf (11.0f));
    jlook::button (g, bypR.toFloat(), C::red, proc.value ("bypass") > 0.5f, hoverBtn == 1, "BYPASS", uf (11.0f));
    jlook::button (g, ctrlR.toFloat(), C::mint, proc.ctrlOn.load(), hoverBtn == 2, proc.ctrlOn.load() ? "MIDI ON" : "MIDI OFF", uf (11.0f));
    skin::chipDraw (g, themeR.toFloat(), hoverBtn == 3);
    // display
    {
        const auto sc = scopeCard();
        hw::faceplate (g, sc, juce::Colour (0xff24222b), juce::Colour (0xff111015), C::pink, 30, 3, 12);
        g.setColour (C::text); g.setFont (uf (12.0f)); g.drawText ("SEGNALE", juce::Rectangle<float> (sc.getX() + 16, sc.getY() + 4, 100, 24), juce::Justification::centredLeft);
        g.setColour (C::dim); g.setFont (uf (9.0f, false));
        g.drawText (U ("scuro = entra  \xc2\xb7  colore = esce dagli effetti  \xc2\xb7  sotto: dove ha lavorato ogni slot"), juce::Rectangle<float> (sc.getX() + 96, sc.getY() + 4, 520, 24), juce::Justification::centredLeft);
        hw::bezel (g, juce::Rectangle<float> (sc.getX() + 16, sc.getY() + 42, sc.getWidth() - 32, sc.getHeight() - 56), 8);
    }
    // uscita
    {
        const auto oc = outCard();
        hw::faceplate (g, oc, juce::Colour (0xff2a2a2e), juce::Colour (0xff121214), C::pink, 30, 23, 12);
        g.setColour (C::text); g.setFont (uf (12.0f)); g.drawText ("USCITA", juce::Rectangle<float> (oc.getX() + 16, oc.getY() + 4, 150, 24), juce::Justification::centredLeft);
        for (int i = 0; i < 2; ++i) { auto mr = juce::Rectangle<float> (oc.getX() + 22, oc.getY() + 196 + i * 34.0f, oc.getWidth() - 44, 20); lcd (g, mr, 4); g.setColour (C::dim); g.setFont (mf (8.5f)); g.drawText (i ? "OUT" : "IN", mr.translated (0, -11).withHeight (10), juce::Justification::centredLeft); }
    }
    g.setColour (C::dim); g.setFont (uf (9.0f, false));
    g.drawText (U ("il suono passa da 1 a 8  \xc2\xb7  trascina la testata di uno slot per cambiare l'ordine  \xc2\xb7  clic sul nome = scegli l'effetto  \xc2\xb7  doppio clic sulla manopola = neutro  \xc2\xb7  ingranaggio = avanzati"),
                juce::Rectangle<float> (20, Hf - 26, 1060, 16), juce::Justification::centredLeft);
}
void Editor::paint (juce::Graphics& g)
{
    skin::apply(); C::sync();
    g.addTransform (juce::AffineTransform::scale (scaler.s));
    bg.draw (g, { 0, 0, W, H }, staticKey(), [this] (juce::Graphics& g2) { paintStatic (g2); });
    // misuratori
    const auto oc = outCard();
    for (int i = 0; i < 2; ++i)
    {
        auto mr = juce::Rectangle<float> (oc.getX() + 22, oc.getY() + 196 + i * 34.0f, oc.getWidth() - 44, 20).reduced (4, 4);
        for (int c = 0; c < 2; ++c)
        {
            auto m = mr.withHeight (mr.getHeight() / 2 - 1).translated (0, c * mr.getHeight() / 2);
            const float v = i ? proc.pkOut[c].load() : proc.pkIn[c].load(), dbv = juce::Decibels::gainToDecibels (v, -100.0f), n = juce::jlimit (0.0f, 1.0f, (dbv + 60) / 63);
            for (int q = 0; q < 36; ++q) { const float t2 = q / 36.0f; g.setColour (t2 <= n ? (t2 > 0.93f ? C::red : t2 > 0.8f ? C::amber : C::mint) : juce::Colours::white.withAlpha (0.05f)); g.fillRect (m.getX() + q * m.getWidth() / 36, m.getY(), m.getWidth() / 36 - 1.2f, m.getHeight()); }
        }
    }
    g.setColour (proc.uiPlay.load() ? C::mint : C::dim); g.setFont (mf (9.5f));
    g.drawText (juce::String (proc.uiBpm.load(), 1) + " BPM" + (proc.uiPlay.load() ? U ("  \xe2\x96\xb6") : juce::String ("  ||")), juce::Rectangle<float> (oc.getX() + 16, oc.getBottom() - 30, oc.getWidth() - 32, 16), juce::Justification::centredRight);
    // scheda trascinata
    if (dragFrom >= 0)
    {
        auto r = juce::Rectangle<float> (dragPos.x - 60, dragPos.y - 14, 120, 28);
        lcd (g, r, 6); g.setColour (C::cyan); g.drawRoundedRectangle (r, 6, 1.2f);
        glowText (g, juce::String (dragFrom + 1) + " " + info (proc.slotType (dragFrom)).name, r, C::cyan, uf (10.0f), juce::Justification::centred);
    }
}
void Editor::mouseMove (const juce::MouseEvent& e0)
{
    const auto p = (e0.position / scaler.s).toInt();
    const int h = resetR.contains (p) ? 0 : bypR.contains (p) ? 1 : ctrlR.contains (p) ? 2 : themeR.contains (p) ? 3 : -1;
    if (h != hoverBtn) { hoverBtn = h; repaint(); }
    setMouseCursor (h >= 0 || presetR.contains (p) || prevR.contains (p) || nextR.contains (p) ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
}
void Editor::mouseDown (const juce::MouseEvent& e0)
{
    const auto p = (e0.position / scaler.s).toInt();
    const int n = (int) factoryPresets().size();
    auto curF = [this, n] { for (int i = 0; i < n; ++i) if (proc.presetName == factoryPresets()[(size_t) i].name) return i; return -1; };
    if (themeR.contains (p)) { skin::cycle(); C::sync(); for (auto* c : cards) c->repaint(); advPanel.repaint(); repaint(); return; }
    if (prevR.contains (p)) { proc.loadFactory ((juce::jmax (0, curF()) + n - 1) % n); refreshCards(); repaint(); return; }
    if (nextR.contains (p)) { proc.loadFactory ((curF() + 1) % n); refreshCards(); repaint(); return; }
    if (presetR.contains (p)) { browser.open(); return; }
    if (resetR.contains (p)) { proc.resetAmounts(); repaint(); return; }
    if (ctrlR.contains (p)) { proc.ctrlOn = ! proc.ctrlOn.load(); repaint(); return; }
    if (bypR.contains (p)) { auto* b = proc.P ("bypass"); b->setValueNotifyingHost (b->getValue() > 0.5f ? 0.0f : 1.0f); repaint(); return; }
}
void Editor::setupBrowser()
{
    addChildComponent (browser);
    browser.accent = C::pink; browser.title = "Preset";
    auto userFiles = [] { auto f = Processor::presetFolder().findChildFiles (juce::File::findFiles, false, "*.jfx"); f.sort(); return f; };
    browser.entries = [userFiles]
    {
        std::vector<jbrowse::Entry> v;
        for (int i = 0; i < (int) factoryPresets().size(); ++i)
        {
            const juce::String nm (factoryPresets()[(size_t) i].name);
            jbrowse::Entry e; e.id = i; e.note = factoryPresets()[(size_t) i].note;
            e.group = nm.upToFirstOccurrenceOf (" | ", false, false); e.name = nm.fromFirstOccurrenceOf (" | ", false, false);
            v.push_back (e);
        }
        const auto fs = userFiles();
        for (int i = 0; i < fs.size(); ++i) { jbrowse::Entry e; e.id = 100000 + i; e.user = true; e.group = "I MIEI"; e.name = fs[i].getFileNameWithoutExtension(); e.note = "Preset tuo, salvato in Musica / Jackpot / FX Preset."; v.push_back (e); }
        return v;
    };
    browser.current = [this, userFiles]
    {
        for (int i = 0; i < (int) factoryPresets().size(); ++i) if (proc.presetName == factoryPresets()[(size_t) i].name) return i;
        const auto fs = userFiles(); for (int i = 0; i < fs.size(); ++i) if (fs[i].getFileNameWithoutExtension() == proc.presetName) return 100000 + i;
        return -1;
    };
    browser.load = [this] (const jbrowse::Entry& e)
    {
        if (e.user) proc.loadPreset (Processor::presetFolder().getChildFile (e.name + ".jfx")); else proc.loadFactory (e.id);
        refreshCards(); repaint();
    };
    browser.save = [this] { askPresetName(); };
    browser.canOverwrite = [this] { return Processor::presetFolder().getChildFile (proc.presetName + ".jfx").existsAsFile(); };
    browser.overwrite = [this] { proc.savePreset (Processor::presetFolder().getChildFile (proc.presetName + ".jfx")); repaint(); };
    browser.folder = [] { Processor::presetFolder().startAsProcess(); };
    browser.remove = [this] (const jbrowse::Entry& e) { Processor::presetFolder().getChildFile (e.name + ".jfx").moveToTrash(); repaint(); };
    browser.onClose = [this] { repaint(); };
}
void Editor::askPresetName()
{
    auto* w = new juce::AlertWindow ("Salva preset", "Nome del preset (finisce in Musica / Jackpot / FX Preset):", juce::MessageBoxIconType::NoIcon, this);
    w->addTextEditor ("name", proc.presetName.replace (" | ", " - "));
    w->addButton ("SALVA", 1, juce::KeyPress (juce::KeyPress::returnKey)); w->addButton ("ANNULLA", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    juce::Component::SafePointer<Editor> sp (this);
    w->enterModalState (true, juce::ModalCallbackFunction::create ([sp, w] (int r)
    {
        if (sp != nullptr && r == 1)
        {
            const auto nm = juce::File::createLegalFileName (w->getTextEditorContents ("name").trim());
            if (nm.isNotEmpty()) { sp->proc.savePreset (Processor::presetFolder().getChildFile (nm + ".jfx")); sp->browser.refresh(); sp->repaint(); }
        }
    }), true);
}
void Editor::endDrag (juce::Point<float> pos)
{
    const int from = dragFrom; dragFrom = -1;
    for (auto* c : cards) { if (c->dropHot) { c->dropHot = false; c->repaint(); } }
    for (int k = 0; k < kSlots; ++k) if (k != from && cards[k]->getBoundsInParent().toFloat().contains (pos)) { proc.swapSlots (from, k); cards[from]->refresh(); cards[k]->refresh(); break; }
    repaint();
}
void Editor::timerCallback()
{
    if (browser.isVisible()) return;
    if (! advPanel.isVisible()) scope.repaint();
    advPanel.tick();
    for (auto* c : cards) c->tick();
    mixK->refresh (true); outK->refresh (true);
    if (bg.key != staticKey()) repaint();
    repaint (scaler.map (outCard().toNearestInt().withTrimmedTop (180)));
    if (dragFrom >= 0) repaint();
}
}
