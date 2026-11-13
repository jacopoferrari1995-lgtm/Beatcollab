// JACKPOT FX: scena scura con taglio cyberpunk. Pannelli a spigoli tagliati, luci al neon sottili, manopole 3D.
#include "FxEditor.h"

namespace jfx
{
using namespace neo;
static juce::String U (const char* s) { return juce::String::fromUTF8 (s); }

// colore per famiglia di effetti
static juce::Colour groupCol (int t)
{
    const juce::String g (pfx::info (t).group);
    if (g == "FILTRI") return K::cyan; if (g == "SPAZIO") return K::violet; if (g == "TEMPO") return K::amber;
    if (g == "SPORCO") return K::pink; if (g == "MODULAZIONE") return K::mint;
    if (g == "TRANSIZIONI") return K::red; if (g == "TONO") return K::blue; return K::faint;
}
// pannello a spigoli tagliati (in alto a sinistra e in basso a destra)
static juce::Path chamfer (juce::Rectangle<float> r, float c)
{
    juce::Path p; p.startNewSubPath (r.getX() + c, r.getY()); p.lineTo (r.getRight(), r.getY()); p.lineTo (r.getRight(), r.getBottom() - c);
    p.lineTo (r.getRight() - c, r.getBottom()); p.lineTo (r.getX(), r.getBottom()); p.lineTo (r.getX(), r.getY() + c); p.closeSubPath(); return p;
}
static void panel (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour edge, float c = 14, float glow = 1.0f)
{
    const auto p = chamfer (r, c);
    for (int k = 3; k >= 1; --k) { g.setColour (juce::Colours::black.withAlpha (0.16f)); g.fillPath (chamfer (r.translated (0, 2.0f * k).expanded ((float) k), c)); }
    juce::ColourGradient bgc (juce::Colour (0xff11131e), r.getX(), r.getY(), juce::Colour (0xff07080e), r.getX(), r.getBottom(), false); g.setGradientFill (bgc); g.fillPath (p);
    juce::ColourGradient tint (edge.withAlpha (0.08f * glow), r.getX(), r.getY(), edge.withAlpha (0.0f), r.getX(), r.getY() + juce::jmin (120.0f, r.getHeight()), false); g.setGradientFill (tint); g.fillPath (p);
    g.setColour (juce::Colours::white.withAlpha (0.06f)); g.strokePath (p, juce::PathStrokeType (1.0f));
    // filo di neon solo sugli spigoli tagliati
    g.setColour (edge.withAlpha (0.8f * glow));
    g.drawLine (r.getX(), r.getY() + c, r.getX() + c, r.getY(), 1.6f); g.drawLine (r.getRight() - c, r.getBottom(), r.getRight(), r.getBottom() - c, 1.6f);
    g.setColour (edge.withAlpha (0.5f * glow)); g.fillRect (r.getX() + c + 6, r.getY(), 28.0f, 1.5f); g.fillRect (r.getRight() - c - 34, r.getBottom() - 1.5f, 28.0f, 1.5f);
}

// ======================================================================= scheda di uno slot
SlotCard::SlotCard (Editor& e, int kk) : ed (e), k (kk) { refresh(); }

void SlotCard::refresh()
{
    const int t = ed.proc.slotType (k);
    shownType = t;
    const auto& I = pfx::info (t); const auto c = groupCol (t);
    knob = std::make_unique<neo::Knob> (ed.proc.P (amtId (k)), juce::String (I.name), c, c.interpolatedWith (juce::Colours::white, 0.35f));
    knob->bipolar = I.bipolar;
    auto* pr = ed.proc.P (amtId (k));
    knob->text = [pr, t] { return pfx::valueText (t, pr->getValue()); };
    if (I.spring) knob->onRelease = [pr, t] { pr->setValueNotifyingHost (pfx::info (t).neutral); };
    knob->setTooltip (I.tip);
    addAndMakeVisible (*knob);
    resized(); repaint();
}
void SlotCard::resized()
{
    auto b = getLocalBounds().toFloat();
    headR = b.withHeight (30);
    powR = { b.getRight() - 30, 8, 18, 18 };
    nameR = { 10, 36, b.getWidth() - 20, 26 };
    ccR = { 10, b.getHeight() - 34, b.getWidth() - 20, 22 };
    if (knob) { const int S = 104, w = S + 16; knob->setBounds (juce::Rectangle<int> ((int) b.getCentreX() - w / 2, 82, w, S + 38).expanded (Knob::pad)); }
}
void SlotCard::paint (juce::Graphics& g)
{
    auto& P = ed.proc; const int t = P.slotType (k); const bool on = P.value (onId (k)) > 0.5f;
    const auto c = groupCol (t); const auto& I = pfx::info (t);
    auto b = getLocalBounds().toFloat().reduced (1);
    panel (g, b, on ? c : K::faint, 12, on ? 1.0f : 0.4f);
    if (dropHot) { g.setColour (K::cyan.withAlpha (0.12f)); g.fillPath (chamfer (b, 12)); g.setColour (K::cyan); g.strokePath (chamfer (b, 12), juce::PathStrokeType (1.5f)); }
    // numero e acceso/spento
    g.setFont (num (16.0f)); g.setColour (on ? K::ink : K::faint); g.drawText (juce::String (k + 1).paddedLeft ('0', 2), juce::Rectangle<float> (14, 6, 40, 22), juce::Justification::centredLeft);
    g.setFont (cap (7.5f)); g.setColour (K::faint); g.drawText (U ("\xe2\x87\x84 trascina"), juce::Rectangle<float> (42, 8, 70, 18), juce::Justification::centredLeft);
    g.setColour (on ? c.withAlpha (0.25f) : juce::Colours::black.withAlpha (0.4f)); g.fillEllipse (powR.expanded (3));
    g.setColour (on ? c : K::faint); g.drawEllipse (powR, 1.4f);
    { juce::Path pw; pw.addCentredArc (powR.getCentreX(), powR.getCentreY(), 5, 5, 0, 0.6f, 5.68f, true); g.strokePath (pw, juce::PathStrokeType (1.4f)); g.drawLine (powR.getCentreX(), powR.getCentreY() - 6, powR.getCentreX(), powR.getCentreY() - 1, 1.4f); }
    // nome dell'effetto: si apre il menu
    g.setColour (c.withAlpha (on ? 0.16f : 0.06f)); g.fillPath (chamfer (nameR, 6));
    g.setColour (c.withAlpha (on ? 0.7f : 0.3f)); g.strokePath (chamfer (nameR, 6), juce::PathStrokeType (1.0f));
    g.setFont (cap (10.0f)); g.setColour (on ? K::ink : K::sub); g.drawText (juce::String (I.name) + U ("  \xe2\x96\xbe"), nameR, juce::Justification::centred);
    if (I.spring) { g.setFont (cap (6.5f)); g.setColour (K::amber.withAlpha (0.8f)); g.drawText ("TORNA A ZERO", juce::Rectangle<float> (0, b.getBottom() - 50, b.getWidth(), 10), juce::Justification::centred); }
    // controller
    const int cc = P.ccMap[k].load(); const bool learn = P.learnSlot.load() == k;
    const bool blink = learn && (juce::Time::getMillisecondCounter() / 300) % 2 == 0;
    auto cr = ccR;
    auto badge = cr.removeFromLeft (cr.getWidth() * 0.5f - 2); cr.removeFromLeft (4);
    g.setColour (juce::Colours::white.withAlpha (0.04f + 0.25f * flash)); g.fillRoundedRectangle (badge, 4);
    g.setColour (c.withAlpha (0.25f + 0.6f * flash)); g.drawRoundedRectangle (badge, 4, 1.0f);
    g.setFont (num (9.5f)); g.setColour (cc >= 0 ? K::ink.withAlpha (0.85f) : K::faint); g.drawText (cc >= 0 ? "CC " + juce::String (cc) : juce::String ("--"), badge, juce::Justification::centred);
    g.setColour (learn ? K::red.withAlpha (blink ? 0.35f : 0.15f) : juce::Colours::white.withAlpha (0.04f)); g.fillRoundedRectangle (cr, 4);
    g.setColour (learn ? K::red : juce::Colours::white.withAlpha (0.12f)); g.drawRoundedRectangle (cr, 4, 1.0f);
    g.setFont (cap (8.0f)); g.setColour (learn ? K::ink : K::sub); g.drawText (learn ? "MUOVI..." : "LEARN", cr, juce::Justification::centred);
    if (! on) { g.setColour (juce::Colour (0xff07080e).withAlpha (0.55f)); g.fillRect (b.withTrimmedTop (64).withTrimmedBottom (40)); }
}
void SlotCard::mouseDown (const juce::MouseEvent& e)
{
    auto& P = ed.proc;
    if (powR.expanded (4).contains (e.position)) { auto* p = P.P (onId (k)); p->setValueNotifyingHost (p->getValue() > 0.5f ? 0.0f : 1.0f); repaint(); return; }
    if (nameR.contains (e.position) || e.mods.isPopupMenu()) { showMenu(); return; }
    auto cr = ccR; auto badge = cr.removeFromLeft (cr.getWidth() * 0.5f - 2); cr.removeFromLeft (4);
    if (cr.contains (e.position)) { P.learnSlot = P.learnSlot.load() == k ? -1 : k; for (auto* c : ed.cards) c->repaint(); return; }
    if (badge.contains (e.position)) { showMenu(); return; }
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
    auto& P = ed.proc; const int ty = P.slotType (k); const int cc = P.ccMap[k].load();
    juce::PopupMenu m; m.addSectionHeader ("SLOT " + juce::String (k + 1) + (cc >= 0 ? "  -  CC " + juce::String (cc) : juce::String ("  -  nessun CC")));
    std::map<juce::String, juce::PopupMenu> groups; juce::StringArray order;
    for (int t = 1; t < pfx::kNumTypes; ++t)
    {
        const juce::String gname (pfx::info (t).group); if (! order.contains (gname)) order.add (gname);
        groups[gname].addItem (100 + t, juce::String (pfx::info (t).name) + (pfx::info (t).spring ? "   (torna a zero)" : ""), true, t == ty);
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
    if (knob && knob->animate()) knob->repaint();
    const int seen = P.ccSeen[k].load();
    if (seen != lastSeen) { lastSeen = seen; flash = 1.0f; }
    if (flash > 0.01f || P.learnSlot.load() == k) { flash *= 0.85f; repaint (ccR.expanded (2).toNearestInt()); }
}

// ======================================================================= display
void Scope::paint (juce::Graphics& g)
{
    auto& P = ed.proc; auto b = getLocalBounds().toFloat();
    auto p = b.reduced (16, 14).withTrimmedTop (30).withTrimmedBottom (40);
    // griglia
    g.setColour (K::cyan.withAlpha (0.05f));
    for (int i = 1; i < 16; ++i) g.fillRect (p.getX() + p.getWidth() * i / 16, p.getY(), 1.0f, p.getHeight());
    for (int i = 1; i < 6; ++i) g.fillRect (p.getX(), p.getY() + p.getHeight() * i / 6, p.getWidth(), 1.0f);
    const int Nh = Processor::kHist, w0 = P.hW.load();
    const float cy = p.getCentreY(), h = p.getHeight() * 0.48f;
    auto lin = [] (float db) { return juce::jlimit (0.0f, 1.0f, juce::Decibels::decibelsToGain (db)); };
    auto body = [&] (std::atomic<float>* hist)
    {
        juce::Path pt; const int NX = Nh;
        for (int x = 0; x < NX; ++x) { const float v = lin (hist[(w0 + x) % Nh].load()); const float xx = p.getX() + p.getWidth() * x / (NX - 1); if (x == 0) pt.startNewSubPath (xx, cy - v * h); else pt.lineTo (xx, cy - v * h); }
        for (int x = NX - 1; x >= 0; --x) { const float v = lin (hist[(w0 + x) % Nh].load()); pt.lineTo (p.getX() + p.getWidth() * x / (NX - 1), cy + v * h); }
        pt.closeSubPath(); return pt;
    };
    const auto pin = body (P.hIn), pout = body (P.hOut);
    juce::ColourGradient og (K::pink.withAlpha (0.7f), 0, cy - h, K::pink.withAlpha (0.7f), 0, cy + h, false); og.addColour (0.5, K::violet.withAlpha (0.25f));
    g.setGradientFill (og); g.fillPath (pout);
    g.setColour (K::pink.brighter (0.4f)); g.strokePath (pout, juce::PathStrokeType (0.8f));
    g.setColour (juce::Colour (0xffd8dbe8).withAlpha (0.18f)); g.fillPath (pin);
    g.setColour (juce::Colour (0xffd8dbe8).withAlpha (0.55f)); g.strokePath (pin, juce::PathStrokeType (0.7f));
    g.setColour (K::cyan.withAlpha (0.6f)); g.fillRect (p.getRight() - 1, p.getY(), 1.5f, p.getHeight());
    // intestazione del display
    g.setFont (cap (9.0f)); g.setColour (K::cyan.withAlpha (0.8f)); g.drawText ("SEGNALE", b.reduced (18, 12).withHeight (16), juce::Justification::centredLeft);
    g.setFont (f (9.0f)); g.setColour (K::faint);
    g.drawText (U ("grigio = entra  \xc2\xb7  rosa = esce dagli effetti  \xc2\xb7  ultimi 4 s"), b.reduced (18, 12).withHeight (16).withTrimmedLeft (70), juce::Justification::centredLeft);
    g.setFont (num (10.0f)); g.setColour (P.uiPlay.load() ? K::mint : K::sub);
    g.drawText (juce::String (P.uiBpm.load(), 1) + " BPM" + (P.uiPlay.load() ? U ("  \xe2\x96\xb6") : juce::String ("  ||")), b.reduced (18, 12).withHeight (16), juce::Justification::centredRight);
    // effetti in azione: chip accese per gli slot che stanno lavorando
    auto row = b.reduced (16, 0).withTrimmedTop (b.getHeight() - 36).withHeight (24);
    float x = row.getX();
    for (int k = 0; k < kSlots; ++k)
    {
        const int t = P.slotType (k); if (t == pfx::OFF) continue;
        const float v = P.P (amtId (k))->getValue(), nu = pfx::info (t).neutral;
        const bool active = P.value (onId (k)) > 0.5f && std::abs (v - nu) > 0.02f && t != pfx::SCRATCHHAND;
        const juce::String s = juce::String (k + 1) + " " + pfx::info (t).name;
        const float wch = juce::GlyphArrangement::getStringWidth (cap (8.0f), s) + 18;
        auto r = juce::Rectangle<float> (x, row.getY(), wch, row.getHeight()); x += wch + 6;
        const auto c = groupCol (t);
        g.setColour (active ? c.withAlpha (0.22f) : juce::Colours::white.withAlpha (0.03f)); g.fillPath (chamfer (r, 5));
        g.setColour (active ? c : juce::Colours::white.withAlpha (0.1f)); g.strokePath (chamfer (r, 5), juce::PathStrokeType (1.0f));
        g.setFont (cap (8.0f)); g.setColour (active ? K::ink : K::faint); g.drawText (s, r, juce::Justification::centred);
    }
}

// ======================================================================= editor
Editor::Editor (Processor& p) : AudioProcessorEditor (p), proc (p)
{
    addAndMakeVisible (scope);
    for (int k = 0; k < kSlots; ++k) addAndMakeVisible (cards.add (new SlotCard (*this, k)));
    mixK = std::make_unique<neo::Knob> (proc.P ("mix"), "MIX", K::cyan, K::violet); mixK->text = [this] { return juce::String (juce::roundToInt (proc.value ("mix") * 100)) + "%"; };
    mixK->setTooltip ("Quanto degli effetti si sente: 100% tutto, 0% suono pulito.");
    outK = std::make_unique<neo::Knob> (proc.P ("out"), "OUTPUT", K::mint, K::cyan); outK->bipolar = true; outK->text = [this] { const float v = proc.value ("out"); return (v > 0 ? "+" : "") + juce::String (v, 1) + " dB"; };
    addAndMakeVisible (*mixK); addAndMakeVisible (*outK);
    addAndMakeVisible (sizeChip);
    scaler.setup();
    startTimerHz (30);
}
void Editor::resized()
{
    const float s = scaler.update();
    presetR = { 470, 16, 300, 34 }; prevR = { 436, 16, 30, 34 }; nextR = { 774, 16, 30, 34 };
    saveR = { 812, 18, 74, 30 }; resetR = { 892, 18, 86, 30 }; bypR = { 984, 18, 92, 30 }; ctrlR = { 1082, 18, 96, 30 }; menuR = presetR;
    scope.setBounds (24, 80, 872, 252);
    const int S = 72;
    mixK->setBounds (juce::Rectangle<int> (934, 110, S + 16, S + 38).expanded (Knob::pad));
    outK->setBounds (juce::Rectangle<int> (1050, 110, S + 16, S + 38).expanded (Knob::pad));
    const float cw = (W - 48 - 7 * 8) / (float) kSlots;
    for (int k = 0; k < kSlots; ++k) cards[k]->setBounds (juce::Rectangle<float> (24 + k * (cw + 8), 350, cw, 300).toNearestInt());
    sizeChip.setBounds (W - 76, H - 18, 58, 15);
    bg = {};
    jlook::scaleChildren (*this, s);
}
void Editor::paintStatic (juce::Graphics& g)
{
    const float Wf = (float) W, Hf = (float) H;
    juce::ColourGradient base (juce::Colour (0xff0b0b16), 0, 0, juce::Colour (0xff040409), 0, Hf, false); g.setGradientFill (base); g.fillAll();
    juce::Image au (juce::Image::ARGB, W / 8, H / 8, true);
    { juce::Graphics ag (au); ag.addTransform (juce::AffineTransform::scale (1.0f / 8));
      auto blob = [&ag] (float x, float y, float rx, float ry, juce::Colour c, float a) { ag.setColour (c.withAlpha (a)); ag.fillEllipse (x - rx, y - ry, rx * 2, ry * 2); };
      blob (250, 200, 360, 150, K::pink, 0.16f); blob (900, 160, 320, 140, K::violet, 0.2f); blob (600, 620, 640, 140, K::cyan, 0.08f); blob (1150, 500, 200, 200, K::pink, 0.07f); }
    blur (au, 10, 3); g.drawImage (au, juce::Rectangle<float> (0, 0, Wf, Hf));
    // griglia prospettica sul pavimento (cyberpunk)
    for (int i = 0; i <= 24; ++i)
    {
        const float x0 = Wf * 0.5f + (i - 12) * 40.0f, x1 = Wf * 0.5f + (i - 12) * 140.0f;
        g.setColour (K::cyan.withAlpha (0.035f)); g.drawLine (x0, 345, x1, Hf, 1.0f);
    }
    for (int j = 0; j < 8; ++j) { const float y = 345 + std::pow ((float) j / 7.0f, 1.8f) * (Hf - 345); g.setColour (K::cyan.withAlpha (0.03f)); g.fillRect (0.0f, y, Wf, 1.0f); }
    // linee di scansione e grana
    for (float y = 0; y < Hf; y += 3) { g.setColour (juce::Colours::black.withAlpha (0.12f)); g.fillRect (0.0f, y, Wf, 1.0f); }
    juce::Random rnd (11); for (int i = 0; i < 9000; ++i) { g.setColour (juce::Colours::white.withAlpha (rnd.nextFloat() * 0.03f)); g.fillRect (rnd.nextFloat() * Wf, rnd.nextFloat() * Hf, 1.0f, 1.0f); }
    // testata
    juce::ColourGradient hl (K::pink.withAlpha (0.0f), 0, 63, K::cyan.withAlpha (0.0f), Wf, 63, false); hl.addColour (0.2, K::pink.withAlpha (0.6f)); hl.addColour (0.8, K::cyan.withAlpha (0.5f));
    g.setGradientFill (hl); g.fillRect (0.0f, 63.0f, Wf, 1.0f);
    { juce::Path lg; lg.startNewSubPath (26, 44); lg.lineTo (38, 18); lg.lineTo (50, 44); lg.startNewSubPath (31, 34); lg.lineTo (45, 34);
      g.setColour (K::pink.withAlpha (0.3f)); g.strokePath (lg, juce::PathStrokeType (6.0f, juce::PathStrokeType::mitered)); g.setColour (K::pink); g.strokePath (lg, juce::PathStrokeType (2.2f, juce::PathStrokeType::mitered)); }
    glowText (g, "JACKPOT", { 62, 14, 120, 22 }, K::ink, num (17.0f));
    glowText (g, "FX", { 152, 12, 60, 26 }, K::pink, big (22.0f));
    g.setFont (cap (7.5f)); g.setColour (K::faint); g.drawText ("PERFORMANCE RACK  //  8 SLOT", juce::Rectangle<float> (62, 36, 260, 12), juce::Justification::centredLeft);
    // pannelli fissi
    panel (g, { 24, 80, 872, 252 }, K::pink, 16);
    panel (g, { 912, 80, 264, 252 }, K::cyan, 16);
    g.setFont (cap (9.0f)); g.setColour (K::cyan.withAlpha (0.8f)); g.drawText ("USCITA", juce::Rectangle<float> (930, 90, 100, 16), juce::Justification::centredLeft);
    // frecce della catena fra le schede
    const float cw = (W - 48 - 7 * 8) / (float) kSlots;
    for (int k = 0; k < kSlots - 1; ++k)
    {
        const float x = 24 + (k + 1) * (cw + 8) - 4, y = 350 + 150;
        juce::Path a; a.addTriangle (x - 3, y - 5, x - 3, y + 5, x + 4, y); g.setColour (K::pink.withAlpha (0.7f)); g.fillPath (a);
    }
    g.setFont (f (9.0f)); g.setColour (K::faint);
    g.drawText (U ("il suono passa da 1 a 8  \xc2\xb7  trascina una scheda dal numero per cambiare l'ordine  \xc2\xb7  clic sul nome = scegli l'effetto  \xc2\xb7  doppio clic sulla manopola = neutro"),
                juce::Rectangle<float> (24, 662, 1000, 16), juce::Justification::centredLeft);
}
void Editor::paint (juce::Graphics& g)
{
    g.addTransform (juce::AffineTransform::scale (scaler.s));
    const float sc = juce::jlimit (1.0f, 4.0f, g.getInternalContext().getPhysicalPixelScaleFactor());
    if (bg.isNull() || std::abs (bgScale - sc) > 0.01f)
    {
        bgScale = sc; bg = juce::Image (juce::Image::RGB, juce::roundToInt (W * sc), juce::roundToInt (H * sc), false);
        juce::Graphics ig (bg); ig.addTransform (juce::AffineTransform::scale (sc)); paintStatic (ig);
    }
    { g.saveState(); g.addTransform (juce::AffineTransform::scale (1.0f / sc)); g.drawImageAt (bg, 0, 0); g.restoreState(); }
    // preset
    { auto pr = presetR.toFloat(); g.setColour (juce::Colours::black.withAlpha (0.45f)); g.fillPath (chamfer (pr, 8)); g.setColour (K::pink.withAlpha (0.6f)); g.strokePath (chamfer (pr, 8), juce::PathStrokeType (1.0f));
      g.setFont (cap (7.0f)); g.setColour (K::faint); g.drawText ("PRESET", pr.withHeight (12).translated (0, 2), juce::Justification::centred);
      g.setFont (f (13.0f, 1)); g.setColour (K::ink); g.drawFittedText (proc.presetName, pr.withTrimmedTop (11).toNearestInt(), juce::Justification::centred, 1, 0.7f);
      g.setFont (f (18.0f)); g.setColour (K::sub); g.drawText (U ("\xe2\x80\xb9"), prevR, juce::Justification::centred); g.drawText (U ("\xe2\x80\xba"), nextR, juce::Justification::centred); }
    chip (g, saveR.toFloat(), "SALVA", false, K::pink, 8.5f);
    chip (g, resetR.toFloat(), "AZZERA", false, K::amber, 8.5f);
    chip (g, bypR.toFloat(), "BYPASS", proc.value ("bypass") > 0.5f, K::red, 8.5f);
    chip (g, ctrlR.toFloat(), proc.ctrlOn.load() ? "MIDI ON" : "MIDI OFF", proc.ctrlOn.load(), K::mint, 8.5f);
    // meter d'uscita
    for (int c = 0; c < 2; ++c)
    {
        auto r = juce::Rectangle<float> (934, 262 + c * 14.0f, 222, 6);
        g.setColour (juce::Colours::white.withAlpha (0.06f)); g.fillRect (r);
        const float db = juce::Decibels::gainToDecibels (proc.pkOut[c].load(), -48.0f), t = juce::jlimit (0.0f, 1.0f, (db + 48.0f) / 48.0f);
        juce::ColourGradient mg (K::cyan, r.getX(), 0, db > -0.5f ? K::red : K::pink, r.getRight(), 0, false); g.setGradientFill (mg); g.fillRect (r.withWidth (r.getWidth() * t));
        g.setFont (cap (7.0f)); g.setColour (K::faint); g.drawText (c ? "R" : "L", juce::Rectangle<float> (922, r.getY() - 3, 10, 12), juce::Justification::centredLeft);
    }
    g.setFont (num (9.0f)); g.setColour (K::sub);
    g.drawText ("OUT " + juce::String (juce::Decibels::gainToDecibels (juce::jmax (proc.pkOut[0].load(), proc.pkOut[1].load()), -60.0f), 1) + " dB", juce::Rectangle<float> (934, 292, 222, 14), juce::Justification::centredRight);
    // scheda trascinata
    if (dragFrom >= 0)
    {
        auto r = juce::Rectangle<float> (dragPos.x - 50, dragPos.y - 14, 100, 28);
        g.setColour (K::cyan.withAlpha (0.25f)); g.fillPath (chamfer (r, 6)); g.setColour (K::cyan); g.strokePath (chamfer (r, 6), juce::PathStrokeType (1.2f));
        g.setFont (cap (9.0f)); g.setColour (K::ink); g.drawText (juce::String (dragFrom + 1) + " " + pfx::info (proc.slotType (dragFrom)).name, r, juce::Justification::centred);
    }
}
void Editor::mouseDown (const juce::MouseEvent& e0)
{
    const auto p = e0.position / scaler.s;
    const int n = (int) factoryPresets().size();
    auto curF = [this, n] { for (int i = 0; i < n; ++i) if (proc.presetName == factoryPresets()[(size_t) i].name) return i; return -1; };
    if (prevR.toFloat().contains (p)) { proc.loadFactory ((curF() + n - 1) % n); for (auto* c : cards) c->refresh(); repaint(); return; }
    if (nextR.toFloat().contains (p)) { proc.loadFactory ((curF() + 1) % n); for (auto* c : cards) c->refresh(); repaint(); return; }
    if (presetR.toFloat().contains (p)) { presetMenu(); return; }
    if (resetR.toFloat().contains (p)) { proc.resetAmounts(); repaint(); return; }
    if (ctrlR.toFloat().contains (p)) { proc.ctrlOn = ! proc.ctrlOn.load(); repaint(); return; }
    if (bypR.toFloat().contains (p)) { auto* b = proc.P ("bypass"); b->setValueNotifyingHost (b->getValue() > 0.5f ? 0.0f : 1.0f); repaint(); return; }
    if (saveR.toFloat().contains (p))
    {
        chooser = std::make_unique<juce::FileChooser> ("Salva il preset", Processor::presetFolder().getChildFile (proc.presetName + ".jfx"), "*.jfx");
        juce::Component::SafePointer<Editor> sp (this);
        chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting, [sp] (const juce::FileChooser& c)
        { if (sp == nullptr) return; auto f = c.getResult(); if (f != juce::File()) { sp->proc.savePreset (f.withFileExtension ("jfx")); sp->repaint(); } });
    }
}
void Editor::presetMenu()
{
    juce::PopupMenu m, fac, mine; const auto& fp = factoryPresets();
    for (int i = 0; i < (int) fp.size(); ++i) fac.addItem (1 + i, fp[(size_t) i].name, true, proc.presetName == fp[(size_t) i].name);
    m.addSectionHeader ("PRESET");
    m.addSubMenu ("Di fabbrica", fac);
    auto files = Processor::presetFolder().findChildFiles (juce::File::findFiles, false, "*.jfx");
    files.sort();
    for (int i = 0; i < files.size(); ++i) mine.addItem (1000 + i, files[i].getFileNameWithoutExtension(), true, proc.presetName == files[i].getFileNameWithoutExtension());
    m.addSubMenu ("I miei", mine, files.size() > 0);
    m.addSeparator(); m.addItem (2000, "Apri la cartella dei preset");
    juce::Component::SafePointer<Editor> sp (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withTargetScreenArea (localAreaToGlobal (scaler.map (presetR))), [sp, files] (int r)
    {
        if (sp == nullptr || r <= 0) return;
        if (r < 1000) sp->proc.loadFactory (r - 1);
        else if (r < 2000) sp->proc.loadPreset (files[r - 1000]);
        else Processor::presetFolder().revealToUser();
        for (auto* c : sp->cards) c->refresh(); sp->repaint();
    });
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
    scope.repaint();
    for (auto* c : cards) c->tick();
    for (auto* k : { mixK.get(), outK.get() }) if (k->animate()) k->repaint();
    repaint (scaler.map ({ 912, 250, 264, 70 }));
    repaint (scaler.map ({ 430, 10, 760, 46 }));
    if (dragFrom >= 0) repaint();
}
}
