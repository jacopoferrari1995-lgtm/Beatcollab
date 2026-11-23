// JACKPOT SPACE: frontale come Dust, Alter e FX. Testata e preset nella pelle comune, piastre per slot, manopole da hardware,
// onda SCAN del Master con le corsie degli slot e lo schema serie / parallelo.
#include "SpaceEditor.h"

namespace jsp
{
static juce::String U (const char* s) { return juce::String::fromUTF8 (s); }
static juce::Font uf (float size, bool bold = true) { return jlook::ui (size, bold ? 2 : 1); }
static juce::Font mf (float size) { return jlook::mono (size); }
static void glowText (juce::Graphics& g, const juce::String& t, juce::Rectangle<float> r, juce::Colour c, juce::Font f, juce::Justification j = juce::Justification::centredLeft)
{
    g.setFont (f);
    g.setColour (c.withAlpha (0.2f)); g.drawText (t, r.translated (-0.7f, 0.3f), j); g.drawText (t, r.translated (0.7f, 0.3f), j);
    g.setColour (c); g.drawText (t, r, j);
}
static void lcd (juce::Graphics& g, juce::Rectangle<float> r, float rad)
{
    if (jlook::skinHooks().on && jlook::skinHooks().screen) { jlook::skinHooks().screen (g, r, rad, C::cyan); return; }
    g.setColour (juce::Colours::black.withAlpha (0.65f)); g.fillRoundedRectangle (r.expanded (1.5f), rad + 1.5f);
    juce::ColourGradient cg (C::lcd1, r.getCentreX(), r.getCentreY(), C::lcd2, r.getX(), r.getY(), true); g.setGradientFill (cg); g.fillRoundedRectangle (r, rad);
}
juce::Colour typeCol (int t)
{
    switch (t)
    {
        case PLATE: return C::cyan; case HALL: return C::violet; case ROOM: return C::blue; case SPRING: return C::mint; case TAPE: return C::amber; case DIGITAL: return C::pink;
        case SHIMMER: return C::cyan.interpolatedWith (C::pink, 0.5f); case SPACED: return C::violet.interpolatedWith (C::cyan, 0.4f);
        case MORPH: return C::mint.interpolatedWith (C::cyan, 0.5f); case NEOVERB: return C::blue.interpolatedWith (C::violet, 0.5f);
        case CONV: return C::red; case BBD: return C::amber.interpolatedWith (C::red, 0.45f); case REVERSE: return C::pink.interpolatedWith (C::amber, 0.4f);
        case MULTITAP: return C::amber.interpolatedWith (C::mint, 0.45f);
        case CHAMBER: return C::amber.interpolatedWith (C::violet, 0.5f); case GATED: return C::red.interpolatedWith (C::violet, 0.3f);
        case STUDIO: return C::blue.interpolatedWith (C::mint, 0.4f); case CLOUD: return C::pink.interpolatedWith (C::violet, 0.5f);
        case GRANULAR: return C::mint.interpolatedWith (C::pink, 0.4f);
        default: return C::faint;
    }
}
static juce::Colour plate1 (int t) { return typeCol (t).interpolatedWith (juce::Colour (0xff202228), 0.84f); }
static juce::Colour plate2 (int t) { return typeCol (t).interpolatedWith (juce::Colour (0xff0d0e11), 0.9f); }
static std::function<juce::String()> textFor (Processor& P, int k, int i)
{
    return [&P, k, i] { float all[kP]; for (int j = 0; j < kP; ++j) all[j] = P.value (pId (k, j)); return paramText (P.slotType (k), i, all[i], all); };
}

// ======================================================================= manopola
void Knob::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isAltDown() || e.mods.isCommandDown()) { mouseDoubleClick (e); return; }
    start = param->getValue(); param->beginChangeGesture(); drag = true; repaint();
}
void Knob::mouseUp (const juce::MouseEvent&) { if (drag) param->endChangeGesture(); drag = false; repaint(); }
void Knob::mouseDrag (const juce::MouseEvent& e)
{
    if (drag) param->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, start - e.getDistanceFromDragStartY() / (e.mods.isShiftDown() ? 900.0f : 220.0f)));
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
    const float laneH = 5.0f, laneGap = 2.0f, lanesH = kSlots * (laneH + laneGap) + 6;
    auto area = b.reduced (12, 10).withTrimmedBottom (lanesH + 14);
    auto lanes = juce::Rectangle<float> (area.getX(), area.getBottom() + 14, area.getWidth(), lanesH - 6);
    const long tot = P.hTot.load(); if (tot <= 2) return;
    const int N = Processor::kHist; const long newest = tot - 1;
    auto ring = [N] (long a) { return (int) (((a % N) + N) % N); };
    const float lastQ = P.hPpq[ring (newest)].load();
    const bool beats = lastQ >= 0;
    // 2 battute quando la DAW suona (agganciate alla battuta), 4 secondi da ferma
    const double winB = 8.0, winCols = beats ? winB * 60.0 / juce::jmax (40.0, P.uiBpm.load()) / Processor::kColSec : 4.0 / Processor::kColSec;
    const double cursor = beats ? std::fmod ((double) lastQ, winB) / winB : std::fmod ((double) newest, winCols) / winCols;
    const float cy = area.getCentreY(), hh = area.getHeight() * 0.5f - 2;
    if (beats) for (int q = 0; q <= 8; ++q) { const float x = area.getX() + area.getWidth() * q / 8.0f; g.setColour (C::text.withAlpha (q % 4 == 0 ? 0.12f : 0.05f)); g.fillRect (x, area.getY(), 1.0f, area.getHeight()); }
    else for (int s = 0; s <= 4; ++s) { const float x = area.getX() + area.getWidth() * s / 4.0f; g.setColour (C::text.withAlpha (0.06f)); g.fillRect (x, area.getY(), 1.0f, area.getHeight()); }
    g.setColour (C::text.withAlpha (0.06f)); g.fillRect (area.getX(), cy, area.getWidth(), 1.0f);
    const int npx = juce::jmax (16, (int) area.getWidth());
    std::vector<float> pin ((size_t) npx, -1.0f), pout ((size_t) npx, -1.0f); std::vector<int> pm ((size_t) npx, 0);
    const int back = (int) juce::jmin ((double) N - 2, winCols * 1.02);
    for (int j = 0; j < back && newest - j >= 0; ++j)
    {
        const long a = newest - j; const int r = ring (a);
        double rel;
        if (beats) { const float q = P.hPpq[r].load(); if (q < 0 || lastQ - q > winB - 0.02) break; rel = std::fmod ((double) q, winB) / winB; }
        else { if (j > winCols - 2) break; rel = std::fmod ((double) a, winCols) / winCols; }
        double ahead = rel - cursor; if (ahead < 0) ahead += 1.0;
        if (j > 0 && ahead > 0.0 && ahead < 0.012) continue;
        const int x = juce::jlimit (0, npx - 1, (int) (rel * npx));
        pin[(size_t) x] = juce::jmax (pin[(size_t) x], P.hIn[r].load()); pout[(size_t) x] = juce::jmax (pout[(size_t) x], P.hOut[r].load()); pm[(size_t) x] |= P.hMask[r].load();
    }
    for (int x = 1; x + 1 < npx; ++x) if (pin[(size_t) x] < 0 && pin[(size_t) x - 1] >= 0 && pin[(size_t) x + 1] >= 0) { pin[(size_t) x] = pin[(size_t) x - 1]; pout[(size_t) x] = pout[(size_t) x - 1]; pm[(size_t) x] = pm[(size_t) x - 1]; }
    auto shape = [&] (const std::vector<float>& v)
    {
        juce::Path path; int x = 0; const float dx = area.getWidth() / npx;
        auto lin = [] (float a) { return juce::jlimit (0.0f, 1.05f, std::sqrt (a)); };   // radice: le code basse si vedono
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
    const auto wc = C::cyan.interpolatedWith (C::violet, 0.35f);
    const auto po = shape (pout), pi = shape (pin);
    g.setColour (C::text.withAlpha (0.13f)); g.fillPath (pi);
    g.setColour (wc.withAlpha (0.72f)); g.fillPath (po);
    g.setColour (wc.brighter (0.5f).withAlpha (0.9f)); g.strokePath (po, juce::PathStrokeType (1.0f));
    { const float x = area.getX() + (float) cursor * area.getWidth(); g.setColour (C::text.withAlpha (0.55f)); g.fillRect (x, area.getY(), 1.0f, area.getHeight()); }
    const float dx = lanes.getWidth() / npx;
    for (int k = 0; k < kSlots; ++k)
    {
        const float y = lanes.getY() + k * (laneH + laneGap);
        g.setColour (C::text.withAlpha (0.04f)); g.fillRect (lanes.getX(), y, lanes.getWidth(), laneH);
        g.setColour (typeCol (P.slotType (k)).withAlpha (0.9f));
        for (int x = 0; x < npx; )
        {
            if (! (pm[(size_t) x] & (1 << k))) { ++x; continue; }
            const int s0 = x; while (x < npx && (pm[(size_t) x] & (1 << k))) ++x;
            g.fillRect (lanes.getX() + s0 * dx, y, (x - s0) * dx, laneH);
        }
    }
}

// ======================================================================= scheda
SlotCard::SlotCard (Editor& e, int kk) : ed (e), k (kk)
{
    auto* mp = ed.proc.P (mixId (k));
    mixK = std::make_unique<Knob> (mp); mixK->spec.label = "MIX"; mixK->spec.style = 1; mixK->defV = 0.0f;
    mixK->text = [mp] { return juce::String (juce::roundToInt (mp->getValue() * 100)) + "%"; };
    mixK->setTooltip ("Quanto di questo slot va nel suono. In serie: quanto ne passa allo slot dopo. In parallelo: quanto ne arriva all'uscita. Doppio clic = 0.");
    addAndMakeVisible (*mixK);
    refresh();
}
void SlotCard::refresh()
{
    auto& P = ed.proc; const int t = P.slotType (k); shownType = t;
    const auto& I = info (t); const auto c = typeCol (t);
    knob.reset(); k2.reset();
    if (t != OFF)
    {
        knob = std::make_unique<Knob> (P.P (pId (k, 0))); knob->spec.label = I.pn[0]; knob->spec.col = c; knob->spec.big = true;
        knob->text = textFor (P, k, 0); knob->defV = I.def[0]; knob->setTooltip (juce::String (I.desc) + "  Doppio clic = valore tipico.");
        addAndMakeVisible (*knob);
        k2 = std::make_unique<Knob> (P.P (pId (k, 1))); k2->spec.label = I.pn[1]; k2->spec.col = c; k2->spec.style = 1;
        k2->text = textFor (P, k, 1); k2->defV = I.def[1];
        addAndMakeVisible (*k2);
    }
    mixK->spec.col = c; mixK->setVisible (t != OFF);
    resized(); repaint();
}
void SlotCard::resized()
{
    auto b = getLocalBounds().toFloat();
    headR = b.withHeight (32); powR = { b.getWidth() - 34, 6, 24, 24 }; soloR = { b.getWidth() - 66, 8, 26, 18 };
    nameR = { 10, 42, b.getWidth() - 20, 26 };
    serR = { 10, 74, (b.getWidth() - 26) * 0.5f, 22 }; parR = { serR.getRight() + 6, 74, serR.getWidth(), 22 };
    if (knob) knob->setBounds (juce::Rectangle<float> (14, 110, b.getWidth() - 28, 132).toNearestInt());
    if (k2) k2->setBounds (juce::Rectangle<float> (8, 252, b.getWidth() * 0.5f - 10, 82).toNearestInt());
    mixK->setBounds (juce::Rectangle<float> (b.getWidth() * 0.5f + 2, 252, b.getWidth() * 0.5f - 10, 82).toNearestInt());
    gearR = { 12, b.getHeight() - 42, 30, 30 };
    meterR = { 50, b.getHeight() - 30, b.getWidth() - 64, 6 };
}
juce::String SlotCard::key() const
{
    auto& P = ed.proc; const int t = P.slotType (k); juce::String s;
    s << skin::cur() << "/" << t << "/" << (int) (P.value (onId (k)) > 0.5f) << "/" << (int) (P.value (rtId (k)) > 0.5f) << "/" << (int) dropHot << "/" << (int) (ed.advPanel.isVisible() && ed.advPanel.k == k) << "/" << P.userIRName (k);
    const auto& I = info (t); for (int i = 2; i < I.n; ++i) s << (std::abs (P.value (pId (k, i)) - I.def[i]) > 0.003f ? "1" : "0");
    if (t == CONV) s << "/" << map::pick (P.value (pId (k, 0)), kIRChoices);
    return s;
}
void SlotCard::paint (juce::Graphics& g)
{
    auto& P = ed.proc; const int t = P.slotType (k); const bool on = P.value (onId (k)) > 0.5f && t != OFF;
    const auto c = typeCol (t); const auto& I = info (t);
    const auto bnd = getLocalBounds();
    base.draw (g, bnd, key(), [&] (juce::Graphics& bg)
    {
        auto b = bnd.toFloat();
        hw::faceplate (bg, b, plate1 (t), plate2 (t), on ? c : C::faint, 32, 50 + k, 12);
        bg.setColour (C::text.withAlpha (on ? 1.0f : 0.55f)); bg.setFont (mf (15.0f)); bg.drawText (juce::String (k + 1).paddedLeft ('0', 2), juce::Rectangle<float> (14, 6, 34, 22), juce::Justification::centredLeft);
        bg.setColour (C::dim); bg.setFont (uf (8.0f, false)); bg.drawText (U ("\xe2\x87\x84 trascina"), juce::Rectangle<float> (44, 8, 70, 18), juce::Justification::centredLeft);
        hw::jewel (bg, powR.getCentreX(), powR.getCentreY(), 5.0f, on, c);
        lcd (bg, nameR, 6);
        glowText (bg, juce::String (I.name) + U ("  \xe2\x96\xbe"), nameR, on ? c : C::dim, uf (10.5f), juce::Justification::centred);
        if (t == OFF)
        {
            bg.setFont (uf (10.0f, false)); bg.setColour (C::dim);
            bg.drawFittedText ("clic sul nome per scegliere un riverbero o un delay", b.reduced (18, 0).withTrimmedTop (140).withHeight (60).toNearestInt(), juce::Justification::centred, 3);
            return;
        }
        const bool par = P.value (rtId (k)) > 0.5f;
        jlook::button (bg, serR, C::amber, ! par, false, "SERIE", uf (8.0f));
        jlook::button (bg, parR, C::cyan, par, false, "PARALLELO", uf (8.0f));
        bg.setFont (uf (7.5f)); bg.setColour (C::dim);
        bg.drawText (t == CONV && map::pick (P.value (pId (k, 0)), kIRChoices) == kIRs && P.userIR[k].isNotEmpty() ? P.userIRName (k).toUpperCase() : juce::String (I.group), juce::Rectangle<float> (0, 98, b.getWidth(), 10), juce::Justification::centred);
        const bool open = ed.advPanel.isVisible() && ed.advPanel.k == k;
        bool changed = false; for (int i = 2; i < I.n; ++i) if (std::abs (P.value (pId (k, i)) - I.def[i]) > 0.003f) changed = true;
        jlook::button (bg, gearR, c, open, false, {}, uf (8.0f));
        const float cx = gearR.getCentreX(), cy = gearR.getCentreY();
        juce::Path gp; for (int i = 0; i < 8; ++i) { gp.addRectangle (-1.4f, -7.5f, 2.8f, 3.5f); gp.applyTransform (juce::AffineTransform::rotation (juce::MathConstants<float>::twoPi / 8)); }
        gp.applyTransform (juce::AffineTransform::translation (cx, cy)); bg.setColour (open ? c : C::text.withAlpha (0.8f)); bg.fillPath (gp); bg.drawEllipse (cx - 5, cy - 5, 10, 10, 1.8f);
        if (changed) { bg.setColour (C::amber); bg.fillEllipse (gearR.getRight() - 8, gearR.getY() + 3, 5, 5); }
        lcd (bg, meterR.expanded (2, 2), 3);
        if (! on) { bg.setColour (juce::Colours::black.withAlpha (0.32f)); bg.fillRoundedRectangle (b.reduced (5).withTrimmedTop (70).withTrimmedBottom (8), 8); }
        if (dropHot) { bg.setColour (C::cyan.withAlpha (0.12f)); bg.fillRoundedRectangle (b.reduced (2), 12); bg.setColour (C::cyan); bg.drawRoundedRectangle (b.reduced (2), 12, 1.5f); }
    });
    if (t != OFF)
    {   // SOLO dello slot
        const int so = P.soloSlot.load(); const bool me = so == k;
        g.setColour (me ? C::amber : juce::Colour (0xff15161a)); g.fillRoundedRectangle (soloR, 3);
        g.setColour (me ? C::amber.brighter (0.4f) : C::dim.withAlpha (0.6f)); g.drawRoundedRectangle (soloR.reduced (0.5f), 3, 1);
        g.setFont (uf (8.5f)); g.setColour (me ? juce::Colours::black : C::text.withAlpha (0.8f)); g.drawText ("S", soloR, juce::Justification::centred);
        if (so >= 0 && ! me) { g.setColour (juce::Colours::black.withAlpha (0.45f)); g.fillRoundedRectangle (getLocalBounds().toFloat().reduced (5).withTrimmedTop (34), 8); g.setColour (C::amber.withAlpha (0.8f)); g.setFont (uf (8.5f)); g.drawText ("ESCLUSO DAL SOLO", getLocalBounds().toFloat().withTrimmedTop (36).withHeight (14), juce::Justification::centred); }
    }
    if (t != OFF)
    {
        const float db = juce::Decibels::gainToDecibels (lvlShown, -60.0f), tt = juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f);
        g.setColour (c.withAlpha (on ? 0.9f : 0.3f)); g.fillRoundedRectangle (meterR.withWidth (meterR.getWidth() * tt), 2);
    }
}
void SlotCard::mouseMove (const juce::MouseEvent& e)
{
    const bool hand = powR.contains (e.position) || soloR.contains (e.position) || nameR.contains (e.position) || gearR.contains (e.position) || serR.contains (e.position) || parR.contains (e.position);
    setMouseCursor (hand ? juce::MouseCursor::PointingHandCursor : headR.contains (e.position) ? juce::MouseCursor::DraggingHandCursor : juce::MouseCursor::NormalCursor);
}
void SlotCard::mouseDown (const juce::MouseEvent& e)
{
    auto& P = ed.proc; const int t = P.slotType (k);
    if (t != OFF && soloR.expanded (2).contains (e.position)) { P.soloSlot = P.soloSlot.load() == k ? -1 : k; for (auto* c : ed.cards) c->repaint(); return; }
    if (powR.expanded (4).contains (e.position)) { auto* p = P.P (onId (k)); p->setValueNotifyingHost (p->getValue() > 0.5f ? 0.0f : 1.0f); repaint(); return; }
    if (nameR.contains (e.position) || e.mods.isPopupMenu()) { showMenu(); return; }
    if (t != OFF && (serR.contains (e.position) || parR.contains (e.position))) { P.P (rtId (k))->setValueNotifyingHost (parR.contains (e.position) ? 1.0f : 0.0f); repaint(); return; }
    if (t != OFF && gearR.expanded (3).contains (e.position))
    {
        if (ed.advPanel.isVisible() && ed.advPanel.k == k) ed.advPanel.setVisible (false); else ed.advPanel.open (k);
        for (auto* c : ed.cards) c->repaint(); return;
    }
    if (headR.contains (e.position)) { dragging = true; ed.dragFrom = k; }
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
    auto& P = ed.proc; const int ty = P.slotType (k);
    juce::PopupMenu m; m.addSectionHeader ("SLOT " + juce::String (k + 1));
    std::map<juce::String, juce::PopupMenu> groups; juce::StringArray order;
    for (int t = 1; t < kNumTypes; ++t)
    {
        const juce::String gname (info (t).group); if (! order.contains (gname)) order.add (gname);
        groups[gname].addItem (100 + t, info (t).name, true, t == ty);
    }
    for (auto& gname : order) m.addSubMenu (gname, groups[gname]);
    m.addItem (100, "Vuoto", true, ty == 0);
    juce::Component::SafePointer<SlotCard> sp (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [sp] (int r)
    {
        if (sp == nullptr || r < 100) return;
        sp->ed.proc.setSlot (sp->k, r - 100); sp->refresh();
        if (sp->ed.advPanel.isVisible() && sp->ed.advPanel.k == sp->k) sp->ed.advPanel.open (sp->k);
    });
}
void SlotCard::tick()
{
    auto& P = ed.proc;
    if (P.slotType (k) != shownType) refresh();
    const bool on = P.value (onId (k)) > 0.5f;
    for (auto* kn : { knob.get(), k2.get(), mixK.get() }) if (kn != nullptr) kn->refresh (on);
    const float l = P.slotLvl[k].load();
    if (std::abs (l - lvlShown) > 1e-4f) { lvlShown = l; repaint (meterR.expanded (2).toNearestInt()); }
    if (base.key != key()) repaint();
}

// ======================================================================= tutti i parametri
void AdvPanel::open (int kk)
{
    k = kk; shownType = ed.proc.slotType (k); knobs.clear();
    const auto& I = info (shownType); const auto c = typeCol (shownType);
    for (int i = 0; i < I.n; ++i)
    {
        auto kn = std::make_unique<Knob> (ed.proc.P (pId (k, i)));
        kn->spec.label = I.pn[i]; kn->spec.col = c; kn->spec.style = 1; kn->defV = I.def[i]; kn->text = textFor (ed.proc, k, i);
        kn->setTooltip ("Doppio clic = valore tipico del motore. Shift mentre trascini = movimento fine.");
        addAndMakeVisible (*kn); knobs.push_back (std::move (kn));
    }
    resized(); setVisible (shownType != OFF); toFront (false); repaint();
}
void AdvPanel::resized()
{
    auto b = getLocalBounds().toFloat();
    closeR = { b.getRight() - 52, 6, 36, 22 }; resetR = { b.getRight() - 172, 6, 112, 22 }; irR = { b.getRight() - 300, 6, 120, 22 };
    const int n = (int) knobs.size(); if (n == 0) return;
    const float kw = 92, kh = 122, gap = 12, tot = n * kw + (n - 1) * gap, x0 = b.getCentreX() - tot / 2;
    for (int i = 0; i < n; ++i) knobs[(size_t) i]->setBounds (juce::Rectangle<float> (x0 + i * (kw + gap), 120, kw, kh).toNearestInt());
}
void AdvPanel::paint (juce::Graphics& g)
{
    if (k < 0) return;
    auto b = getLocalBounds().toFloat(); const int t = ed.proc.slotType (k); const auto c = typeCol (t);
    hw::faceplate (g, b, plate1 (t), plate2 (t), c, 32, 70 + k, 14);
    g.setColour (C::text); g.setFont (uf (12.0f));
    g.drawText ("PARAMETRI  " + juce::String (k + 1).paddedLeft ('0', 2) + U ("  \xc2\xb7  ") + info (t).name, juce::Rectangle<float> (16, 4, 400, 26), juce::Justification::centredLeft);
    g.setColour (C::text.withAlpha (0.85f)); g.setFont (uf (10.5f, false));
    g.drawFittedText (info (t).desc, juce::Rectangle<int> (24, 46, (int) b.getWidth() - 48, 40), juce::Justification::centredLeft, 2);
    g.setColour (C::dim); g.setFont (uf (9.0f, false));
    g.drawText (U ("doppio clic = valore tipico  \xc2\xb7  shift = fine  \xc2\xb7  le prime due sono anche sulla scheda"), juce::Rectangle<float> (30, b.getBottom() - 24, b.getWidth() - 60, 16), juce::Justification::centredLeft);
    jlook::button (g, resetR, C::amber, false, false, "RIPRISTINA", uf (9.0f));
    jlook::button (g, closeR, C::pink, false, false, U ("\xc3\x97"), uf (13.0f));
    if (t == CONV)
    {
        jlook::button (g, irR, C::red, ed.proc.userIR[k].isNotEmpty(), false, "CARICA IR...", uf (9.0f));
        const auto nm = ed.proc.userIRName (k);
        g.setColour (C::dim); g.setFont (uf (9.0f, false));
        g.drawText (nm.isEmpty() ? juce::String ("nessun file: trascina un .wav sulla scheda") : "file: " + nm, juce::Rectangle<float> (irR.getX() - 290, 6, 280, 22), juce::Justification::centredRight);
    }
}
void AdvPanel::mouseDown (const juce::MouseEvent& e)
{
    if (closeR.contains (e.position)) { setVisible (false); for (auto* c : ed.cards) c->repaint(); return; }
    if (resetR.contains (e.position)) { ed.proc.resetP (k); repaint(); if (k >= 0) ed.cards[k]->repaint(); return; }
    if (k >= 0 && ed.proc.slotType (k) == CONV && irR.contains (e.position))
    {
        chooser = std::make_unique<juce::FileChooser> ("Scegli una risposta all'impulso", juce::File::getSpecialLocation (juce::File::userMusicDirectory), "*.wav;*.aif;*.aiff;*.flac");
        juce::Component::SafePointer<AdvPanel> sp (this);
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [sp] (const juce::FileChooser& c)
        { if (sp == nullptr) return; auto f = c.getResult(); if (f.existsAsFile() && sp->ed.proc.setUserIR (sp->k, f)) sp->ed.refreshAll(); });
    }
}
void AdvPanel::tick()
{
    if (! isVisible() || k < 0) return;
    if (ed.proc.slotType (k) != shownType) { open (k); return; }
    for (auto& kn : knobs) kn->refresh (true);
}

// ======================================================================= editor
Editor::Editor (Processor& p) : AudioProcessorEditor (p), proc (p)
{
    skin::init ("space", { 0xff9fc3e0, 0xffb9a6e0, 0xffa8bea0, 0xffd8cdb8 }, 0xff3ee8ff); C::sync();
    undoMark.p = &p; addMouseListener (&undoMark, true); setWantsKeyboardFocus (true);
    setLookAndFeel (&lnf);
    addAndMakeVisible (scope);
    for (int k = 0; k < kSlots; ++k) addAndMakeVisible (cards.add (new SlotCard (*this, k)));
    addChildComponent (advPanel);
    auto mk = [this] (const char* id, const char* label, bool bip, float def, std::function<juce::String()> txt, const char* tip)
    {
        auto kn = std::make_unique<Knob> (proc.P (id)); kn->spec.label = label; kn->spec.style = 1; kn->spec.col = C::cyan; kn->spec.bipolar = bip; kn->defV = def; kn->text = std::move (txt); kn->setTooltip (tip);
        addAndMakeVisible (*kn); return kn;
    };
    dryK = mk ("dry", "DRY", false, 1.0f, [this] { return juce::String (juce::roundToInt (proc.value ("dry") * 100)) + "%"; }, "Il suono pulito. Su una traccia di mandata (return) mettilo a 0%.");
    wetK = mk ("wet", "WET", false, 0.8f, [this] { const float v = proc.value ("wet"); return (v > 0.05f ? "+" : "") + juce::String (v, 1) + " dB"; }, "Volume di tutti i riverberi e delay insieme.");
    duckK = mk ("duck", "DUCKING", false, 0.0f, [this] { return juce::String (juce::roundToInt (proc.value ("duck") * 100)) + "%"; }, "Abbassa le code mentre suona lo strumento e le fa salire nelle pause.");
    outK = mk ("out", "OUTPUT", true, 0.5f, [this] { const float v = proc.value ("out"); return (v > 0.05f ? "+" : "") + juce::String (v, 1) + " dB"; }, "Volume finale.");
    addAndMakeVisible (sizeChip);
    setupBrowser();
    scaler.setup();
    startTimerHz (30);
}
void Editor::resized()
{
    const float s = scaler.update();
    presetR = { 372, 15, 282, 36 }; prevR = { 334, 15, 36, 36 }; nextR = { 656, 15, 36, 36 };
    undoR = { 700, 15, 72, 36 }; redoR = { 776, 15, 72, 36 };
    frzR = { 852, 15, 100, 36 }; bypR = { 958, 15, 100, 36 }; themeR = { 1078, 19, 100, 28 };
    const auto sc = scopeCard(); scope.setBounds (juce::Rectangle<float> (sc.getX() + 16, sc.getY() + 42, sc.getWidth() - 32, sc.getHeight() - 100).toNearestInt());
    advPanel.setBounds (sc.toNearestInt());
    const auto oc = outCard();
    dryK->setBounds (juce::Rectangle<float> (oc.getX() + 30, oc.getY() + 36, 96, 100).toNearestInt());
    wetK->setBounds (juce::Rectangle<float> (oc.getRight() - 126, oc.getY() + 36, 96, 100).toNearestInt());
    duckK->setBounds (juce::Rectangle<float> (oc.getX() + 30, oc.getY() + 140, 96, 100).toNearestInt());
    outK->setBounds (juce::Rectangle<float> (oc.getRight() - 126, oc.getY() + 140, 96, 100).toNearestInt());
    for (int k = 0; k < kSlots; ++k) cards[k]->setBounds (card (k).toNearestInt());
    sizeChip.setBounds (W - 76, H - 16, 58, 14);
    browser.setBounds (0, 0, W, H);
    jlook::scaleChildren (*this, s);
}
juce::String Editor::staticKey()
{
    juce::String k; k << skin::cur() << "/" << proc.presetName << "/" << (int) (proc.value ("bypass") > 0.5f) << "/" << (int) (proc.value ("freeze") > 0.5f) << "/" << hoverBtn;
    for (int i = 0; i < kSlots; ++i) k << "/" << proc.slotType (i) << (proc.value (onId (i)) > 0.5f ? "a" : "b") << (proc.value (rtId (i)) > 0.5f ? "p" : "s");
    return k;
}
void Editor::paintStatic (juce::Graphics& g)
{
    const float Wf = (float) W, Hf = (float) H;
    jlook::backdrop (g, { 0, 0, Wf, Hf }, C::cyan, C::violet);
    jlook::header (g, { 0, 0, Wf, 64 }, "SPACE", C::cyan, U ("JSP-6  RIVERBERI E DELAY / 6 SLOT / SERIE E PARALLELO"), C::violet, C::cyan);
    lcd (g, presetR.toFloat(), 8); jlook::glass (g, presetR.toFloat(), 8);
    {
        auto pr = presetR.toFloat(); juce::String cat ("I MIEI");
        for (auto& fp : factoryPresets()) if (fp.name == proc.presetName) cat = fp.cat;
        g.setColour (C::dim); g.setFont (uf (7.5f)); g.drawText (cat, pr.withWidth (80).translated (12, 0), juce::Justification::centredLeft);
        glowText (g, proc.presetName.toUpperCase(), pr.withTrimmedLeft (60).withTrimmedRight (20), C::cyan, mf (12.5f), juce::Justification::centred);
    }
    g.setColour (C::cyan); g.setFont (uf (20.0f));
    g.drawText (U ("\xe2\x80\xb9"), prevR, juce::Justification::centred); g.drawText (U ("\xe2\x80\xba"), nextR, juce::Justification::centred);
    jlook::button (g, undoR.toFloat(), proc.undo.canUndo() ? C::cyan : C::dim, false, hoverBtn == 3, U ("\xe2\x86\xb6 ANNULLA"), uf (9.0f));
    jlook::button (g, redoR.toFloat(), proc.undo.canRedo() ? C::cyan : C::dim, false, hoverBtn == 4, U ("RIPETI \xe2\x86\xb7"), uf (9.0f));
    jlook::button (g, frzR.toFloat(), C::cyan, proc.value ("freeze") > 0.5f, hoverBtn == 0, "FREEZE", uf (11.0f));
    jlook::button (g, bypR.toFloat(), C::red, proc.value ("bypass") > 0.5f, hoverBtn == 1, "BYPASS", uf (11.0f));
    skin::chipDraw (g, themeR.toFloat(), hoverBtn == 2);
    {
        const auto sc = scopeCard();
        hw::faceplate (g, sc, juce::Colour (0xff22242b), juce::Colour (0xff101115), C::cyan, 30, 3, 12);
        g.setColour (C::text); g.setFont (uf (12.0f)); g.drawText ("CODA", juce::Rectangle<float> (sc.getX() + 16, sc.getY() + 4, 100, 24), juce::Justification::centredLeft);
        g.setColour (C::dim); g.setFont (uf (9.0f, false));
        juce::String note = U ("scuro = entra  \xc2\xb7  colore = solo riverberi e delay  \xc2\xb7  sotto: dove suona ogni slot");
        g.drawText (note, juce::Rectangle<float> (sc.getX() + 80, sc.getY() + 4, 560, 24), juce::Justification::centredLeft);
        hw::bezel (g, juce::Rectangle<float> (sc.getX() + 16, sc.getY() + 42, sc.getWidth() - 32, sc.getHeight() - 100), 8);
        // schema della catena: riga alta = serie (uno dopo l'altro), riga bassa = parallelo (dal pulito)
        auto row = juce::Rectangle<float> (sc.getX() + 16, sc.getBottom() - 50, sc.getWidth() - 32, 38);
        g.setFont (uf (8.0f)); g.setColour (C::dim);
        g.drawText ("IN", juce::Rectangle<float> (row.getX(), row.getY(), 20, 16), juce::Justification::centredLeft);
        g.drawText ("OUT", juce::Rectangle<float> (row.getRight() - 26, row.getY(), 26, 16), juce::Justification::centredRight);
        const float x0 = row.getX() + 26, x1 = row.getRight() - 32, sw = (x1 - x0) / kSlots;
        g.setColour (C::dim.withAlpha (0.5f)); g.fillRect (x0 - 6, row.getY() + 8, x1 - x0 + 12, 1.0f);
        for (int k = 0; k < kSlots; ++k)
        {
            const int t = proc.slotType (k); const bool on = t != OFF && proc.value (onId (k)) > 0.5f, par = proc.value (rtId (k)) > 0.5f;
            auto r = juce::Rectangle<float> (x0 + k * sw + 3, par ? row.getY() + 20 : row.getY(), sw - 6, 17);
            const auto c = typeCol (t);
            if (par && t != OFF) { g.setColour (c.withAlpha (on ? 0.6f : 0.2f)); g.fillRect (r.getCentreX(), row.getY() + 9, 1.0f, 11.0f); }
            lcd (g, r, 4);
            if (on) { g.setColour (c.withAlpha (0.7f)); g.drawRoundedRectangle (r, 4, 1.0f); }
            glowText (g, juce::String (k + 1) + " " + (t == OFF ? juce::String ("-") : juce::String (info (t).name)), r, on ? c : C::faint, uf (8.0f), juce::Justification::centred);
        }
    }
    {
        const auto oc = outCard();
        hw::faceplate (g, oc, juce::Colour (0xff2a2a2e), juce::Colour (0xff121214), C::cyan, 30, 23, 12);
        g.setColour (C::text); g.setFont (uf (12.0f)); g.drawText ("USCITA", juce::Rectangle<float> (oc.getX() + 16, oc.getY() + 4, 150, 24), juce::Justification::centredLeft);
        auto mr = juce::Rectangle<float> (oc.getX() + 22, oc.getY() + 256, oc.getWidth() - 44, 20); lcd (g, mr, 4);
        g.setColour (C::dim); g.setFont (mf (8.5f)); g.drawText ("OUT", mr.translated (0, -11).withHeight (10), juce::Justification::centredLeft);
    }
    g.setColour (C::dim); g.setFont (uf (9.0f, false));
    g.drawText (U ("SERIE = prende quello che esce dallo slot prima  \xc2\xb7  PARALLELO = prende il suono pulito  \xc2\xb7  trascina la testata per cambiare l'ordine  \xc2\xb7  ingranaggio = tutti i parametri  \xc2\xb7  trascina un .wav su una scheda = la tua IR"),
                juce::Rectangle<float> (20, Hf - 26, 1080, 16), juce::Justification::centredLeft);
}
void Editor::paint (juce::Graphics& g)
{
    skin::apply(); C::sync();
    g.addTransform (juce::AffineTransform::scale (scaler.s));
    bg.draw (g, { 0, 0, W, H }, staticKey(), [this] (juce::Graphics& g2) { paintStatic (g2); });
    const auto oc = outCard();
    auto mr = juce::Rectangle<float> (oc.getX() + 22, oc.getY() + 256, oc.getWidth() - 44, 20).reduced (4, 4);
    for (int c = 0; c < 2; ++c)
    {
        auto m = mr.withHeight (mr.getHeight() / 2 - 1).translated (0, c * mr.getHeight() / 2);
        const float dbv = juce::Decibels::gainToDecibels (proc.pkOut[c].load(), -100.0f), n = juce::jlimit (0.0f, 1.0f, (dbv + 60) / 63);
        for (int q = 0; q < 36; ++q) { const float t2 = q / 36.0f; g.setColour (t2 <= n ? (t2 > 0.93f ? C::red : t2 > 0.8f ? C::amber : C::mint) : juce::Colours::white.withAlpha (0.05f)); g.fillRect (m.getX() + q * m.getWidth() / 36, m.getY(), m.getWidth() / 36 - 1.2f, m.getHeight()); }
    }
    g.setColour (proc.uiPlay.load() ? C::mint : C::dim); g.setFont (mf (9.5f));
    g.drawText (juce::String (proc.uiBpm.load(), 1) + " BPM" + (proc.uiPlay.load() ? U ("  \xe2\x96\xb6") : juce::String ("  ||")), juce::Rectangle<float> (oc.getX() + 16, oc.getY() + 4, oc.getWidth() - 32, 24), juce::Justification::centredRight);
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
    const int h = frzR.contains (p) ? 0 : bypR.contains (p) ? 1 : themeR.contains (p) ? 2 : undoR.contains (p) ? 3 : redoR.contains (p) ? 4 : -1;
    if (h != hoverBtn) { hoverBtn = h; repaint(); }
    setMouseCursor (h >= 0 || presetR.contains (p) || prevR.contains (p) || nextR.contains (p) ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
}
void Editor::mouseDown (const juce::MouseEvent& e0)
{
    const auto p = (e0.position / scaler.s).toInt();
    const int n = (int) factoryPresets().size();
    auto curF = [this, n] { for (int i = 0; i < n; ++i) if (proc.presetName == factoryPresets()[(size_t) i].name) return i; return -1; };
    if (themeR.contains (p)) { skin::cycle(); C::sync(); for (auto* c : cards) c->repaint(); advPanel.repaint(); repaint(); return; }
    if (prevR.contains (p)) { proc.loadFactory ((juce::jmax (0, curF()) + n - 1) % n); refreshAll(); return; }
    if (nextR.contains (p)) { proc.loadFactory ((curF() + 1) % n); refreshAll(); return; }
    if (presetR.contains (p)) { browser.open(); return; }
    if (undoR.contains (p)) { doUndo (false); return; }
    if (redoR.contains (p)) { doUndo (true); return; }
    if (frzR.contains (p)) { auto* b = proc.P ("freeze"); b->setValueNotifyingHost (b->getValue() > 0.5f ? 0.0f : 1.0f); repaint(); return; }
    if (bypR.contains (p)) { auto* b = proc.P ("bypass"); b->setValueNotifyingHost (b->getValue() > 0.5f ? 0.0f : 1.0f); repaint(); return; }
}
bool Editor::keyPressed (const juce::KeyPress& k)
{
    if (k.getModifiers().isCommandDown() && (k.getKeyCode() == 'Z' || k.getKeyCode() == 'z')) { doUndo (k.getModifiers().isShiftDown()); return true; }
    if (k.getModifiers().isCommandDown() && (k.getKeyCode() == 'Y' || k.getKeyCode() == 'y')) { doUndo (true); return true; }
    return false;
}
void Editor::doUndo (bool redo)
{
    proc.undo.beginNewTransaction();
    if (redo ? proc.undo.redo() : proc.undo.undo()) { proc.soloSlot = -1; refreshAll(); }
    repaint();
}
void Editor::setupBrowser()
{
    addChildComponent (browser);
    browser.accent = C::cyan; browser.title = "Preset";
    auto userFiles = [] { auto f = Processor::presetFolder().findChildFiles (juce::File::findFiles, false, "*.jsp"); f.sort(); return f; };
    browser.entries = [userFiles]
    {
        std::vector<jbrowse::Entry> v;
        for (int i = 0; i < (int) factoryPresets().size(); ++i)
        { const auto& fp = factoryPresets()[(size_t) i]; jbrowse::Entry e; e.id = i; e.group = fp.cat; e.name = fp.name; e.note = fp.note; v.push_back (e); }
        const auto fs = userFiles();
        for (int i = 0; i < fs.size(); ++i) { jbrowse::Entry e; e.id = 100000 + i; e.user = true; e.group = "I MIEI"; e.name = fs[i].getFileNameWithoutExtension(); e.note = "Preset tuo, salvato in Musica / Jackpot / Space Preset."; v.push_back (e); }
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
        if (e.user) proc.loadPreset (Processor::presetFolder().getChildFile (e.name + ".jsp")); else proc.loadFactory (e.id);
        refreshAll();
    };
    browser.save = [this] { askPresetName(); };
    browser.canOverwrite = [this] { return Processor::presetFolder().getChildFile (proc.presetName + ".jsp").existsAsFile(); };
    browser.overwrite = [this] { proc.savePreset (Processor::presetFolder().getChildFile (proc.presetName + ".jsp")); repaint(); };
    browser.folder = [] { Processor::presetFolder().startAsProcess(); };
    browser.remove = [] (const jbrowse::Entry& e) { Processor::presetFolder().getChildFile (e.name + ".jsp").moveToTrash(); };
    browser.onClose = [this] { repaint(); };
}
void Editor::askPresetName()
{
    auto* w = new juce::AlertWindow ("Salva preset", "Nome del preset (finisce in Musica / Jackpot / Space Preset):", juce::MessageBoxIconType::NoIcon, this);
    w->addTextEditor ("name", proc.presetName);
    w->addButton ("SALVA", 1, juce::KeyPress (juce::KeyPress::returnKey)); w->addButton ("ANNULLA", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    juce::Component::SafePointer<Editor> sp (this);
    w->enterModalState (true, juce::ModalCallbackFunction::create ([sp, w] (int r)
    {
        if (sp != nullptr && r == 1)
        {
            const auto nm = juce::File::createLegalFileName (w->getTextEditorContents ("name").trim());
            if (nm.isNotEmpty()) { sp->proc.savePreset (Processor::presetFolder().getChildFile (nm + ".jsp")); sp->browser.refresh(); sp->repaint(); }
        }
    }), true);
}
void Editor::filesDropped (const juce::StringArray& files, int x, int y)
{
    const auto pos = juce::Point<float> ((float) x, (float) y) / scaler.s;
    for (int k = 0; k < kSlots; ++k)
        if (cards[k]->getBoundsInParent().toFloat().contains (pos))
            for (auto& fn : files) if (proc.setUserIR (k, juce::File (fn))) { refreshAll(); advPanel.open (k); return; }
}
void Editor::endDrag (juce::Point<float> pos)
{
    const int from = dragFrom; dragFrom = -1;
    for (auto* c : cards) if (c->dropHot) { c->dropHot = false; c->repaint(); }
    for (int k = 0; k < kSlots; ++k) if (k != from && cards[k]->getBoundsInParent().toFloat().contains (pos)) { proc.swapSlots (from, k); cards[from]->refresh(); cards[k]->refresh(); if (advPanel.isVisible()) advPanel.open (advPanel.k); break; }
    repaint();
}
void Editor::timerCallback()
{
    if (browser.isVisible()) return;
    if (! advPanel.isVisible()) scope.repaint();
    advPanel.tick();
    for (auto* c : cards) c->tick();
    for (auto* k : { dryK.get(), wetK.get(), duckK.get(), outK.get() }) k->refresh (true);
    if (bg.key != staticKey()) repaint();
    repaint (scaler.map (outCard().toNearestInt().withTrimmedTop (240)));
    repaint (scaler.map (outCard().toNearestInt().withHeight (32)));
    if (dragFrom >= 0) repaint();
    if (++undoTick % 10 == 0) repaint (scaler.map (juce::Rectangle<int> (696, 12, 156, 42)));   // ANNULLA / RIPETI accesi o spenti
}
}
