// JACKPOT SPACE: stessa grafica di FX e Master. Pannelli arrotondati, temi SCURO / CHIARO / CBRPNK / HUD.
#include "SpaceEditor.h"

namespace jsp
{
using namespace neo;
static juce::String U (const char* s) { return juce::String::fromUTF8 (s); }

static juce::Colour typeCol (int t)
{
    switch (t)
    {
        case PLATE: return K::cyan; case HALL: return K::violet; case ROOM: return K::blue; case SPRING: return K::mint; case TAPE: return K::amber; case DIGITAL: return K::pink;
        case SHIMMER: return K::cyan.interpolatedWith (K::pink, 0.5f); case SPACED: return K::violet.interpolatedWith (K::cyan, 0.4f);
        case MORPH: return K::mint.interpolatedWith (K::cyan, 0.5f); case NEOVERB: return K::blue.interpolatedWith (K::violet, 0.5f);
        case CONV: return K::red; case BBD: return K::amber.interpolatedWith (K::red, 0.45f); case REVERSE: return K::pink.interpolatedWith (K::amber, 0.4f);
        case MULTITAP: return K::amber.interpolatedWith (K::mint, 0.45f);
        case CHAMBER: return K::amber.interpolatedWith (K::violet, 0.5f); case GATED: return K::red.interpolatedWith (K::violet, 0.3f);
        case STUDIO: return K::blue.interpolatedWith (K::mint, 0.4f); case CLOUD: return K::pink.interpolatedWith (K::violet, 0.5f);
        case GRANULAR: return K::mint.interpolatedWith (K::pink, 0.4f);
        default: return K::faint;
    }
}
static juce::Path rr (juce::Rectangle<float> r, float c) { juce::Path p; p.addRoundedRectangle (r, juce::jmin (c * 1.2f, r.getHeight() * 0.5f)); return p; }
static juce::Colour veil (float a) { return K::lo.withAlpha (light() ? a * 0.22f : a); }
static juce::Colour glass (float a) { return K::hi.withAlpha (light() ? juce::jmin (1.0f, a * 1.3f) : a); }
static juce::Colour waveCol() { return light() ? juce::Colour (0xff3f7fd6) : juce::Colour (0xff8cd8ff).interpolatedWith (juce::Colour (0xffb08cff), 0.35f); }
static void panel (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour edge, float c = 14, float glow = 1.0f, bool display = false)
{
    if (skinned()) { neo::plate (g, r, edge); return; }
    const float rad = juce::jmin (20.0f, c * 1.3f);
    g.setColour (K::lo.withAlpha (light() ? 0.10f : 0.35f)); g.fillRoundedRectangle (r.translated (0, 3), rad);
    juce::ColourGradient bgc (display ? K::disp0 : K::panel0, 0, r.getY(), display ? K::disp1 : K::panel1, 0, r.getBottom(), false); g.setGradientFill (bgc); g.fillRoundedRectangle (r, rad);
    juce::ColourGradient tint (edge.withAlpha ((light() ? 0.07f : 0.10f) * glow), 0, r.getY(), edge.withAlpha (0.0f), 0, r.getY() + juce::jmin (140.0f, r.getHeight()), false); g.setGradientFill (tint); g.fillRoundedRectangle (r, rad);
    g.setColour (K::hi.withAlpha (light() ? 0.10f : 0.07f)); g.drawRoundedRectangle (r.reduced (0.5f), rad, 1.0f);
    juce::ColourGradient hl (K::hi.withAlpha (0.0f), r.getX() + rad, 0, K::hi.withAlpha (0.0f), r.getRight() - rad, 0, false); hl.addColour (0.5, (light() ? juce::Colours::white : K::hi).withAlpha (light() ? 0.8f : 0.10f));
    g.setGradientFill (hl); g.fillRect (r.getX() + rad, r.getY() + 1, r.getWidth() - 2 * rad, 1.0f);
}
static std::function<juce::String()> textFor (Processor& P, int k, int i)
{
    return [&P, k, i] { float all[kP]; for (int j = 0; j < kP; ++j) all[j] = P.value (pId (k, j)); return paramText (P.slotType (k), i, all[i], all); };
}

// ======================================================================= scheda
SlotCard::SlotCard (Editor& e, int kk) : ed (e), k (kk) { refresh(); }
void SlotCard::refresh()
{
    auto& P = ed.proc; const int t = P.slotType (k); shownType = t;
    const auto& I = info (t); const auto c = typeCol (t);
    knob.reset(); k2.reset();
    if (t != OFF)
    {
        knob = std::make_unique<neo::Knob> (P.P (pId (k, 0)), juce::String (I.pn[0]), c, c.interpolatedWith (juce::Colours::white, 0.35f));
        knob->text = textFor (P, k, 0); knob->defV = I.def[0];
        knob->setTooltip (juce::String (I.desc));
        addAndMakeVisible (*knob);
        k2 = std::make_unique<neo::Knob> (P.P (pId (k, 1)), juce::String (I.pn[1]), c, c);
        k2->text = textFor (P, k, 1); k2->defV = I.def[1];
        addAndMakeVisible (*k2);
    }
    if (mixK == nullptr)
    {
        auto* mp = P.P (mixId (k));
        mixK = std::make_unique<neo::Knob> (mp, "MIX", K::cyan, K::violet);
        mixK->text = [mp] { return juce::String (juce::roundToInt (mp->getValue() * 100)) + "%"; };
        mixK->setTooltip ("Quanto di questo slot va nel suono. In serie: quanto ne passa allo slot dopo. In parallelo: quanto ne arriva all'uscita.");
        addAndMakeVisible (*mixK);
    }
    mixK->setVisible (t != OFF);
    resized(); repaint();
}
void SlotCard::resized()
{
    auto b = getLocalBounds().toFloat();
    headR = b.withHeight (30);
    powR = { b.getRight() - 30, 8, 18, 18 };
    nameR = { 10, 36, b.getWidth() - 20, 26 };
    routeR = { 10, 68, b.getWidth() - 20, 20 };
    meterR = { 14, b.getHeight() - 14, b.getWidth() - 28, 4 };
    gearR = { 14, b.getHeight() - 50, 26, 26 };
    if (knob) { const int S = 74, w = S + 16; knob->setBounds (juce::Rectangle<int> ((int) b.getCentreX() - w / 2, 100, w, S + 38).expanded (Knob::pad)); }
    const int s2 = 30, w2 = s2 + 26;
    if (k2) k2->setBounds (juce::Rectangle<int> (12, 212, w2, s2 + 38).expanded (Knob::pad));
    if (mixK) mixK->setBounds (juce::Rectangle<int> ((int) b.getRight() - 12 - w2, 212, w2, s2 + 38).expanded (Knob::pad));
}
void SlotCard::paint (juce::Graphics& g)
{
    auto& P = ed.proc; const int t = P.slotType (k); const bool on = P.value (onId (k)) > 0.5f && t != OFF;
    const auto c = typeCol (t); const auto& I = info (t);
    auto b = getLocalBounds().toFloat().reduced (1);
    panel (g, b, on ? c : K::faint, 12, on ? 1.0f : 0.4f);
    if (dropHot) { g.setColour (K::cyan.withAlpha (0.12f)); g.fillPath (rr (b, 12)); g.setColour (K::cyan); g.strokePath (rr (b, 12), juce::PathStrokeType (1.5f)); }
    g.setFont (num (16.0f)); g.setColour (on ? K::ink : K::faint); g.drawText (juce::String (k + 1).paddedLeft ('0', 2), juce::Rectangle<float> (14, 6, 40, 22), juce::Justification::centredLeft);
    g.setFont (cap (7.0f)); g.setColour (K::faint); g.drawText ("< trascina >", juce::Rectangle<float> (42, 8, 80, 18), juce::Justification::centredLeft);
    g.setColour (on ? c.withAlpha (0.25f) : veil (0.4f)); g.fillEllipse (powR.expanded (3));
    g.setColour (on ? c : K::faint); g.drawEllipse (powR, 1.4f);
    { juce::Path pw; pw.addCentredArc (powR.getCentreX(), powR.getCentreY(), 5, 5, 0, 0.6f, 5.68f, true); g.strokePath (pw, juce::PathStrokeType (1.4f)); g.drawLine (powR.getCentreX(), powR.getCentreY() - 6, powR.getCentreX(), powR.getCentreY() - 1, 1.4f); }
    // motore
    g.setColour (c.withAlpha (on ? 0.16f : 0.06f)); g.fillPath (rr (nameR, 6));
    g.setColour (c.withAlpha (on ? 0.7f : 0.3f)); g.strokePath (rr (nameR, 6), juce::PathStrokeType (1.0f));
    g.setFont (cap (10.0f)); g.setColour (on ? K::ink : K::sub); g.drawText (juce::String (I.name) + "  v", nameR, juce::Justification::centred);
    if (t == OFF)
    {
        g.setFont (f (10.0f)); g.setColour (K::faint);
        g.drawFittedText ("clic sul nome per scegliere un riverbero o un delay", b.reduced (20, 0).withTrimmedTop (120).withHeight (60).toNearestInt(), juce::Justification::centred, 3);
        return;
    }
    // serie / parallelo
    {
        const bool par = P.value (rtId (k)) > 0.5f;
        auto l = routeR.withWidth (routeR.getWidth() * 0.5f - 2), r = routeR.withTrimmedLeft (routeR.getWidth() * 0.5f + 2);
        chip (g, l, "SERIE", ! par, K::amber, 7.5f); chip (g, r, "PARALLELO", par, K::cyan, 7.5f);
    }
    g.setFont (cap (6.5f)); g.setColour (K::faint);
    g.drawText (t == CONV && map::pick (P.value (pId (k, 0)), kIRChoices) == kIRs && P.userIR[k].isNotEmpty() ? P.userIRName (k).toUpperCase() : juce::String (I.group), juce::Rectangle<float> (0, 90, b.getWidth(), 10), juce::Justification::centred);
    // ingranaggio
    {
        const bool open = ed.advPanel.isVisible() && ed.advPanel.k == k;
        bool changed = false; for (int i = 2; i < I.n; ++i) if (std::abs (P.value (pId (k, i)) - I.def[i]) > 0.003f) changed = true;
        const auto gc = open ? c : K::sub;
        g.setColour (open ? c.withAlpha (0.22f) : glass (0.05f)); g.fillPath (rr (gearR, 5));
        g.setColour (gc.withAlpha (open ? 1.0f : 0.6f)); g.strokePath (rr (gearR, 5), juce::PathStrokeType (1.0f));
        const float cx = gearR.getCentreX(), cy = gearR.getCentreY();
        juce::Path gp; for (int i = 0; i < 8; ++i) { gp.addRectangle (-1.6f, -8.5f, 3.2f, 4.0f); gp.applyTransform (juce::AffineTransform::rotation (juce::MathConstants<float>::twoPi / 8)); }
        gp.applyTransform (juce::AffineTransform::translation (cx, cy)); g.setColour (gc); g.fillPath (gp);
        g.drawEllipse (cx - 5.5f, cy - 5.5f, 11, 11, 2.0f);
        if (changed) { g.setColour (K::amber); g.fillEllipse (gearR.getRight() - 7, gearR.getY() + 2, 5, 5); }
        g.setFont (cap (6.5f)); g.setColour (K::faint); g.drawText ("TUTTI I PARAMETRI", juce::Rectangle<float> (gearR.getRight() + 6, gearR.getY(), 110, gearR.getHeight()), juce::Justification::centredLeft);
    }
    // livello dello slot
    g.setColour (glass (0.06f)); g.fillRoundedRectangle (meterR, 2);
    const float db = juce::Decibels::gainToDecibels (lvlShown, -60.0f), tt = juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f);
    g.setColour (c.withAlpha (0.85f)); g.fillRoundedRectangle (meterR.withWidth (meterR.getWidth() * tt), 2);
    if (! on) { g.setColour (K::panel1.withAlpha (0.6f)); g.fillRect (b.withTrimmedTop (64).withTrimmedBottom (20)); }
}
void SlotCard::mouseDown (const juce::MouseEvent& e)
{
    auto& P = ed.proc; const int t = P.slotType (k);
    if (powR.expanded (4).contains (e.position)) { auto* p = P.P (onId (k)); p->setValueNotifyingHost (p->getValue() > 0.5f ? 0.0f : 1.0f); repaint(); ed.scope.repaint(); return; }
    if (nameR.contains (e.position) || e.mods.isPopupMenu()) { showMenu(); return; }
    if (t != OFF && routeR.contains (e.position))
    {
        auto* p = P.P (rtId (k)); p->setValueNotifyingHost (e.position.x > routeR.getCentreX() ? 1.0f : 0.0f); repaint(); ed.scope.repaint(); return;
    }
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
    setMouseCursor (juce::MouseCursor::DraggingHandCursor);
}
void SlotCard::mouseUp (const juce::MouseEvent& e)
{
    if (dragging) { dragging = false; setMouseCursor (juce::MouseCursor::NormalCursor); ed.endDrag (e.getEventRelativeTo (&ed).position / ed.scaler.s); }
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
        sp->ed.scope.repaint();
    });
}
void SlotCard::tick()
{
    auto& P = ed.proc;
    if (P.slotType (k) != shownType) refresh();
    for (auto* kn : { knob.get(), k2.get(), mixK.get() }) if (kn != nullptr && kn->animate()) kn->repaint();
    const float l = P.slotLvl[k].load();
    if (std::abs (l - lvlShown) > 1e-4f) { lvlShown = l; repaint (meterR.expanded (2).toNearestInt()); }
}

// ======================================================================= display
void Scope::paint (juce::Graphics& g)
{
    auto& P = ed.proc; auto b = getLocalBounds().toFloat();
    auto p = b.reduced (16, 14).withTrimmedTop (30).withTrimmedBottom (54);
    g.setColour (K::hi.withAlpha (0.045f));
    for (int i = 1; i < 8; ++i) g.fillRect (p.getX() + p.getWidth() * i / 8, p.getY(), 1.0f, p.getHeight());
    g.fillRect (p.getX(), p.getCentreY(), p.getWidth(), 1.0f);
    const int Nh = Processor::kHist, w0 = P.hW.load();
    const float cy = p.getCentreY(), h = p.getHeight() * 0.48f;
    auto lin = [] (float db) { return juce::jlimit (0.0f, 1.0f, std::sqrt (juce::Decibels::decibelsToGain (db))); };
    auto body = [&] (std::atomic<float>* hist)
    {
        juce::Path pt;
        for (int x = 0; x < Nh; ++x) { const float v = lin (hist[(w0 + x) % Nh].load()); const float xx = p.getX() + p.getWidth() * x / (Nh - 1); if (x == 0) pt.startNewSubPath (xx, cy - v * h); else pt.lineTo (xx, cy - v * h); }
        for (int x = Nh - 1; x >= 0; --x) { const float v = lin (hist[(w0 + x) % Nh].load()); pt.lineTo (p.getX() + p.getWidth() * x / (Nh - 1), cy + v * h); }
        pt.closeSubPath(); return pt;
    };
    const auto pin = body (P.hIn), pout = body (P.hOut);
    const auto wc = waveCol();
    g.setColour (wc.withAlpha (0.75f)); g.fillPath (pout);
    g.setColour (K::ink.withAlpha (0.5f)); g.strokePath (pin, juce::PathStrokeType (1.0f));
    g.setFont (cap (9.0f)); g.setColour (K::sub); g.drawText ("CODA", b.reduced (18, 12).withHeight (16), juce::Justification::centredLeft);
    g.setFont (f (9.0f)); g.setColour (K::faint);
    juce::String note = U ("contorno = suono che entra  \xc2\xb7  pieno = riverberi e delay  \xc2\xb7  ultimi 4 s");
    for (auto& fp : factoryPresets()) if (fp.name == P.presetName) note = fp.note;
    g.drawFittedText (note, b.reduced (18, 12).withHeight (16).withTrimmedLeft (56).withTrimmedRight (110).toNearestInt(), juce::Justification::centredLeft, 1, 0.8f);
    g.setFont (num (10.0f)); g.setColour (K::sub);
    g.drawText (juce::String (P.uiBpm.load(), 1) + " BPM" + (P.value ("freeze") > 0.5f ? "   FREEZE" : ""), b.reduced (18, 12).withHeight (16), juce::Justification::centredRight);
    // schema della catena: riga alta = serie (uno dopo l'altro), riga bassa = parallelo (dal pulito)
    auto row = b.reduced (16, 0).withTrimmedTop (b.getHeight() - 48).withHeight (36);
    g.setFont (cap (7.0f)); g.setColour (K::faint);
    g.drawText ("IN", juce::Rectangle<float> (row.getX(), row.getY(), 20, 16), juce::Justification::centredLeft);
    g.drawText ("OUT", juce::Rectangle<float> (row.getRight() - 26, row.getY(), 26, 16), juce::Justification::centredRight);
    const float x0 = row.getX() + 26, x1 = row.getRight() - 32, sw = (x1 - x0) / kSlots;
    g.setColour (K::sub.withAlpha (0.35f)); g.fillRect (x0 - 6, row.getY() + 8, x1 - x0 + 12, 1.0f);
    for (int k = 0; k < kSlots; ++k)
    {
        const int t = P.slotType (k); const bool on = t != OFF && P.value (onId (k)) > 0.5f, par = P.value (rtId (k)) > 0.5f;
        auto r = juce::Rectangle<float> (x0 + k * sw + 3, par ? row.getY() + 20 : row.getY(), sw - 6, 16);
        const auto c = typeCol (t);
        if (par && t != OFF) { g.setColour (c.withAlpha (on ? 0.5f : 0.15f)); g.fillRect (r.getCentreX(), row.getY() + 9, 1.0f, 11.0f); }
        g.setColour (on ? c.withAlpha (0.22f) : glass (0.03f)); g.fillPath (rr (r, 4));
        g.setColour (on ? c : glass (0.1f)); g.strokePath (rr (r, 4), juce::PathStrokeType (1.0f));
        g.setFont (cap (7.0f)); g.setColour (on ? K::ink : K::faint);
        g.drawText (juce::String (k + 1) + " " + (t == OFF ? juce::String ("-") : juce::String (info (t).name)), r, juce::Justification::centred);
    }
}

// ======================================================================= pannello di tutti i parametri
void AdvPanel::open (int kk)
{
    k = kk; shownType = ed.proc.slotType (k); knobs.clear();
    const auto& I = info (shownType); const auto c = typeCol (shownType);
    for (int i = 0; i < I.n; ++i)
    {
        auto* kn = knobs.add (new neo::Knob (ed.proc.P (pId (k, i)), I.pn[i], c, c.interpolatedWith (juce::Colours::white, 0.35f)));
        kn->text = textFor (ed.proc, k, i); kn->defV = I.def[i];
        kn->setTooltip ("Doppio clic = valore tipico del motore. Shift mentre trascini = movimento fine.");
        addAndMakeVisible (kn);
    }
    resized(); setVisible (shownType != OFF); repaint();
}
void AdvPanel::resized()
{
    auto b = getLocalBounds().toFloat();
    closeR = { b.getRight() - 46, 14, 30, 24 }; resetR = { b.getRight() - 160, 14, 106, 24 }; irR = { b.getRight() - 290, 14, 122, 24 };
    const int n = knobs.size(); if (n == 0) return;
    const int S = 58, w = S + 26, gap = 18, tot = n * w + (n - 1) * gap, x0 = (int) b.getCentreX() - tot / 2;
    for (int i = 0; i < n; ++i) knobs[i]->setBounds (juce::Rectangle<int> (x0 + i * (w + gap), 82, w, S + 38).expanded (Knob::pad));
}
void AdvPanel::paint (juce::Graphics& g)
{
    if (k < 0) return;
    auto b = getLocalBounds().toFloat(); const int t = ed.proc.slotType (k); const auto c = typeCol (t);
    panel (g, b, c, 16);
    g.setFont (cap (9.0f)); g.setColour (c); g.drawText ("PARAMETRI", juce::Rectangle<float> (20, 14, 90, 24), juce::Justification::centredLeft);
    g.setFont (f (14.0f, 1)); g.setColour (K::ink);
    g.drawText ("SLOT " + juce::String (k + 1) + U ("  \xc2\xb7  ") + info (t).name, juce::Rectangle<float> (110, 14, 400, 24), juce::Justification::centredLeft);
    g.setFont (f (10.0f)); g.setColour (K::sub);
    g.drawFittedText (info (t).desc, juce::Rectangle<int> (20, 44, (int) b.getWidth() - 40, 28), juce::Justification::centredLeft, 2);
    g.setFont (f (9.0f)); g.setColour (K::faint);
    g.drawText (U ("doppio clic = valore tipico  \xc2\xb7  shift = fine  \xc2\xb7  le prime due sono anche sulla scheda"), juce::Rectangle<float> (20, b.getBottom() - 30, b.getWidth() - 40, 16), juce::Justification::centredLeft);
    chip (g, resetR, "RIPRISTINA", false, K::amber, 8.0f);
    chip (g, closeR, "X", false, K::pink, 10.0f);
    if (t == CONV)
    {
        chip (g, irR, "CARICA IR...", ed.proc.userIR[k].isNotEmpty(), K::red, 8.0f);
        g.setFont (f (9.0f)); g.setColour (K::sub);
        const auto nm = ed.proc.userIRName (k);
        g.drawText (nm.isEmpty() ? juce::String ("nessun file: trascina un .wav sulla scheda") : "file: " + nm, juce::Rectangle<float> (irR.getX() - 280, 14, 270, 24), juce::Justification::centredRight);
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
    bool any = false; for (auto* kn : knobs) if (kn->animate()) { kn->repaint(); any = true; }
    if (any) ed.cards[k]->repaint (ed.cards[k]->gearR.expanded (4).toNearestInt());
}

// ======================================================================= editor
Editor::Editor (Processor& p) : AudioProcessorEditor (p), proc (p)
{
    skin::init ("space", { 0xff9fc3e0, 0xffb9a6e0, 0xffa8bea0, 0xffd8cdb8 }, 0xff3ee8ff);
    skin::useTheme (proc.uiTheme.load());
    addAndMakeVisible (scope);
    addChildComponent (advPanel);
    for (int k = 0; k < kSlots; ++k) addAndMakeVisible (cards.add (new SlotCard (*this, k)));
    dryK = std::make_unique<neo::Knob> (proc.P ("dry"), "DRY", K::ink, K::sub); dryK->text = [this] { return juce::String (juce::roundToInt (proc.value ("dry") * 100)) + "%"; }; dryK->defV = 1.0f;
    dryK->setTooltip ("Il suono pulito. Su una traccia di mandata (return) mettilo a 0%.");
    wetK = std::make_unique<neo::Knob> (proc.P ("wet"), "WET", K::cyan, K::violet); wetK->text = [this] { const float v = proc.value ("wet"); return (v > 0 ? "+" : "") + juce::String (v, 1) + " dB"; }; wetK->defV = 0.8f;
    wetK->setTooltip ("Volume di tutti i riverberi e delay insieme.");
    duckK = std::make_unique<neo::Knob> (proc.P ("duck"), "DUCKING", K::amber, K::pink); duckK->text = [this] { return juce::String (juce::roundToInt (proc.value ("duck") * 100)) + "%"; }; duckK->defV = 0.0f;
    duckK->setTooltip ("Abbassa le code mentre suona lo strumento e le fa salire nelle pause: voce chiara anche con tanto spazio.");
    outK = std::make_unique<neo::Knob> (proc.P ("out"), "OUTPUT", K::mint, K::cyan); outK->bipolar = true; outK->text = [this] { const float v = proc.value ("out"); return (v > 0 ? "+" : "") + juce::String (v, 1) + " dB"; }; outK->defV = 0.5f;
    for (auto* kn : { dryK.get(), wetK.get(), duckK.get(), outK.get() }) addAndMakeVisible (*kn);
    addAndMakeVisible (sizeChip);
    scaler.setup();
    startTimerHz (30);
}
void Editor::resized()
{
    const float s = scaler.update();
    themeR = { 330, 18, 88, 30 }; prevR = { 436, 16, 30, 34 }; presetR = { 470, 16, 300, 34 }; nextR = { 774, 16, 30, 34 };
    saveR = { 812, 18, 74, 30 }; frzR = { 892, 18, 86, 30 }; bypR = { 984, 18, 92, 30 };
    scope.setBounds (24, 80, 872, 252); advPanel.setBounds (24, 80, 872, 252);
    const int S = 50, w = S + 22;
    const int xs[2] = { 944, 1060 }, ys[2] = { 104, 200 };
    dryK->setBounds (juce::Rectangle<int> (xs[0], ys[0], w, S + 38).expanded (Knob::pad));
    wetK->setBounds (juce::Rectangle<int> (xs[1], ys[0], w, S + 38).expanded (Knob::pad));
    duckK->setBounds (juce::Rectangle<int> (xs[0], ys[1], w, S + 38).expanded (Knob::pad));
    outK->setBounds (juce::Rectangle<int> (xs[1], ys[1], w, S + 38).expanded (Knob::pad));
    for (int k = 0; k < kSlots; ++k) cards[k]->setBounds (juce::Rectangle<float> (24 + k * (cardW() + 10), 350, cardW(), 344).toNearestInt());
    sizeChip.setBounds (W - 76, H - 18, 58, 15);
    bg = {};
    jlook::scaleChildren (*this, s);
}
void Editor::paintStatic (juce::Graphics& g)
{
    const float Wf = (float) W, Hf = (float) H;
    if (skinned()) neo::cyberBackdrop (g, Wf, Hf);
    else
    {
        juce::ColourGradient base (K::bg1, 0, 0, K::bg0, 0, Hf, false); g.setGradientFill (base); g.fillAll();
        juce::Image au (juce::Image::ARGB, W / 8, H / 8, true);
        { juce::Graphics ag (au); ag.addTransform (juce::AffineTransform::scale (1.0f / 8)); const float la = light() ? 0.35f : 0.6f;
          auto blob = [&ag] (float x, float y, float rx, float ry, juce::Colour c, float a) { ag.setColour (c.withAlpha (a)); ag.fillEllipse (x - rx, y - ry, rx * 2, ry * 2); };
          blob (250, 200, 360, 150, K::cyan, 0.16f * la); blob (900, 160, 320, 140, K::violet, 0.18f * la); blob (600, 620, 640, 140, K::blue, 0.12f * la); }
        blur (au, 10, 3); g.drawImage (au, juce::Rectangle<float> (0, 0, Wf, Hf));
        juce::Random rnd (17); for (int i = 0; i < 9000; ++i) { g.setColour (K::hi.withAlpha (rnd.nextFloat() * 0.025f)); g.fillRect (rnd.nextFloat() * Wf, rnd.nextFloat() * Hf, 1.0f, 1.0f); }
    }
    if (skinned()) neo::cyberHeader (g, Wf, 64, "SPACE", U ("RIVERBERI E DELAY  \xc2\xb7  6 SLOT"));
    else
    {
    // logo: anelli concentrici (onde che si allargano)
    for (int i = 0; i < 3; ++i)
    {
        juce::Path ring; const float r = 5.0f + i * 5.0f; ring.addCentredArc (38, 32, r, r, 0, 0.4f, 6.2832f - 0.4f, true);
        g.setColour ((i == 0 ? K::cyan : i == 1 ? K::violet : K::pink).withAlpha (1.0f - i * 0.25f)); g.strokePath (ring, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    g.setFont (num (15.0f)); g.setColour (K::ink); g.drawText ("JACKPOT", juce::Rectangle<float> (62, 15, 100, 20), juce::Justification::centredLeft);
    g.setFont (big (15.0f)); g.setColour (K::sub); g.drawText ("SPACE", juce::Rectangle<float> (139, 15, 90, 20), juce::Justification::centredLeft);
    g.setFont (cap (7.5f)); g.setColour (K::faint); g.drawText (U ("RIVERBERI E DELAY  \xc2\xb7  6 SLOT  \xc2\xb7  SERIE E PARALLELO"), juce::Rectangle<float> (62, 35, 270, 12), juce::Justification::centredLeft);
    }
    panel (g, { 24, 80, 872, 252 }, waveCol(), 16, 1.0f, true);
    panel (g, { 912, 80, 264, 252 }, K::violet, 16);
    g.setFont (cap (9.0f)); g.setColour (K::sub); g.drawText ("USCITA", juce::Rectangle<float> (930, 90, 100, 16), juce::Justification::centredLeft);
    const float cw = cardW();
    for (int k = 0; k < kSlots - 1; ++k)
    {
        const float x = 24 + (k + 1) * (cw + 10) - 5, y = 350 + 150;
        juce::Path a; a.addTriangle (x - 3, y - 5, x - 3, y + 5, x + 4, y); g.setColour (K::sub.withAlpha (0.7f)); g.fillPath (a);
    }
    g.setFont (f (9.0f)); g.setColour (K::faint);
    g.drawText (U ("SERIE = prende quello che esce dallo slot prima  \xc2\xb7  PARALLELO = prende il suono pulito  \xc2\xb7  trascina dal numero per cambiare l'ordine  \xc2\xb7  ingranaggio = tutti i parametri"),
                juce::Rectangle<float> (24, 706, 1100, 16), juce::Justification::centredLeft);
}
void Editor::paint (juce::Graphics& g)
{
    skin::useTheme (proc.uiTheme.load());
    g.addTransform (juce::AffineTransform::scale (scaler.s));
    const float sc = juce::jlimit (1.0f, 4.0f, g.getInternalContext().getPhysicalPixelScaleFactor());
    if (bg.isNull() || std::abs (bgScale - sc) > 0.01f || bgTheme != K::theme)
    {
        bgScale = sc; bgTheme = K::theme; bg = juce::Image (juce::Image::RGB, juce::roundToInt (W * sc), juce::roundToInt (H * sc), false);
        juce::Graphics ig (bg); ig.addTransform (juce::AffineTransform::scale (sc)); paintStatic (ig);
    }
    { g.saveState(); g.addTransform (juce::AffineTransform::scale (1.0f / sc)); g.drawImageAt (bg, 0, 0); g.restoreState(); }
    chip (g, themeR.toFloat(), skin::label (proc.uiTheme.load()), false, K::sub, 8.5f);
    { auto pr = presetR.toFloat(); g.setColour (veil (0.3f)); g.fillPath (rr (pr, 8)); g.setColour (K::hi.withAlpha (0.1f)); g.strokePath (rr (pr, 8), juce::PathStrokeType (1.0f));
      g.setFont (cap (7.0f)); g.setColour (K::faint); g.drawText ("PRESET", pr.withHeight (12).translated (0, 2), juce::Justification::centred);
      g.setFont (f (13.0f, 1)); g.setColour (K::ink); g.drawFittedText (proc.presetName, pr.withTrimmedTop (11).toNearestInt(), juce::Justification::centred, 1, 0.7f);
      g.setFont (f (18.0f)); g.setColour (K::sub); g.drawText ("<", prevR, juce::Justification::centred); g.drawText (">", nextR, juce::Justification::centred); }
    chip (g, saveR.toFloat(), "SALVA", false, K::pink, 8.5f);
    chip (g, frzR.toFloat(), "FREEZE", proc.value ("freeze") > 0.5f, K::cyan, 8.5f);
    chip (g, bypR.toFloat(), "BYPASS", proc.value ("bypass") > 0.5f, K::red, 8.5f);
    for (int c = 0; c < 2; ++c)
    {
        auto r = juce::Rectangle<float> (944, 300 + c * 12.0f, 212, 5);
        g.setColour (glass (0.06f)); g.fillRect (r);
        const float db = juce::Decibels::gainToDecibels (proc.pkOut[c].load(), -48.0f), t = juce::jlimit (0.0f, 1.0f, (db + 48.0f) / 48.0f);
        g.setColour (db > -0.5f ? K::red : waveCol()); g.fillRoundedRectangle (r.withWidth (r.getWidth() * t), 2.5f);
        g.setFont (cap (7.0f)); g.setColour (K::faint); g.drawText (c ? "R" : "L", juce::Rectangle<float> (932, r.getY() - 3, 10, 11), juce::Justification::centredLeft);
    }
    if (dragFrom >= 0)
    {
        auto r = juce::Rectangle<float> (dragPos.x - 50, dragPos.y - 14, 100, 28);
        g.setColour (K::cyan.withAlpha (0.25f)); g.fillPath (rr (r, 6)); g.setColour (K::cyan); g.strokePath (rr (r, 6), juce::PathStrokeType (1.2f));
        g.setFont (cap (9.0f)); g.setColour (K::ink); g.drawText (juce::String (dragFrom + 1) + " " + info (proc.slotType (dragFrom)).name, r, juce::Justification::centred);
    }
}
void Editor::mouseDown (const juce::MouseEvent& e0)
{
    const auto p = e0.position / scaler.s;
    const int n = (int) factoryPresets().size();
    auto curF = [this, n] { for (int i = 0; i < n; ++i) if (proc.presetName == factoryPresets()[(size_t) i].name) return i; return -1; };
    if (prevR.toFloat().contains (p)) { proc.loadFactory ((curF() + n - 1) % n); refreshAll(); return; }
    if (nextR.toFloat().contains (p)) { proc.loadFactory ((curF() + 1) % n); refreshAll(); return; }
    if (presetR.toFloat().contains (p)) { presetMenu(); return; }
    if (themeR.toFloat().contains (p)) { proc.uiTheme = skin::nextIn (proc.uiTheme.load(), { 1, 2, 9, 10 }); bg = {}; for (auto* c : cards) c->repaint(); advPanel.repaint(); scope.repaint(); repaint(); return; }
    if (frzR.toFloat().contains (p)) { auto* b = proc.P ("freeze"); b->setValueNotifyingHost (b->getValue() > 0.5f ? 0.0f : 1.0f); repaint(); return; }
    if (bypR.toFloat().contains (p)) { auto* b = proc.P ("bypass"); b->setValueNotifyingHost (b->getValue() > 0.5f ? 0.0f : 1.0f); repaint(); return; }
    if (saveR.toFloat().contains (p))
    {
        chooser = std::make_unique<juce::FileChooser> ("Salva il preset", Processor::presetFolder().getChildFile (proc.presetName + ".jsp"), "*.jsp");
        juce::Component::SafePointer<Editor> sp (this);
        chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting, [sp] (const juce::FileChooser& c)
        { if (sp == nullptr) return; auto f = c.getResult(); if (f != juce::File()) { sp->proc.savePreset (f.withFileExtension ("jsp")); sp->repaint(); } });
    }
}
void Editor::presetMenu()
{
    juce::PopupMenu m, mine; const auto& fp = factoryPresets();
    m.addSectionHeader ("PRESET");
    std::map<juce::String, juce::PopupMenu> cats; juce::StringArray order;
    for (int i = 0; i < (int) fp.size(); ++i)
    {
        const auto& c = fp[(size_t) i].cat; if (! order.contains (c)) order.add (c);
        juce::PopupMenu::Item it (fp[(size_t) i].name); it.itemID = 1 + i; it.isTicked = proc.presetName == fp[(size_t) i].name;
        cats[c].addItem (it);
    }
    for (auto& c : order) m.addSubMenu (c, cats[c]);
    m.addSeparator();
    auto files = Processor::presetFolder().findChildFiles (juce::File::findFiles, false, "*.jsp"); files.sort();
    for (int i = 0; i < files.size(); ++i) mine.addItem (1000 + i, files[i].getFileNameWithoutExtension(), true, proc.presetName == files[i].getFileNameWithoutExtension());
    m.addSubMenu ("I miei", mine, files.size() > 0);
    m.addSeparator(); m.addItem (2000, "Apri la cartella dei preset");
    juce::Component::SafePointer<Editor> sp (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withTargetScreenArea (localAreaToGlobal (scaler.map (presetR))), [sp, files] (int r)
    {
        if (sp == nullptr || r <= 0) return;
        if (r < 1000) sp->proc.loadFactory (r - 1); else if (r < 2000) sp->proc.loadPreset (files[r - 1000]); else Processor::presetFolder().revealToUser();
        sp->refreshAll();
    });
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
    scope.repaint(); repaint();
}
void Editor::timerCallback()
{
    if (! advPanel.isVisible()) scope.repaint();
    advPanel.tick();
    for (auto* c : cards) c->tick();
    for (auto* k : { dryK.get(), wetK.get(), duckK.get(), outK.get() }) if (k->animate()) k->repaint();
    repaint (scaler.map ({ 920, 290, 250, 36 }));
    if (dragFrom >= 0) repaint();
}
}
