#include "PhaseEditor.h"

namespace jphs
{
void WaveView::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat(); lcd (g, b, 8, ed.acc);
    auto r = b.reduced (12, 26); const auto& S = ed.proc.snap; const int N = Snapshot::kLen;
    g.setColour (neo::K::hi.withAlpha (0.06f)); g.drawHorizontalLine ((int) r.getCentreY(), r.getX(), r.getRight());
    if (S.serial.load() == 0)
    {
        g.setColour (C::dim); g.setFont (uf (13.0f));
        g.drawFittedText (ed.proc.eng.scUi.load() ? "In attesa di un colpo di cassa..." : "Manda la cassa nel SIDECHAIN di questo plugin\n(in Ableton: menu Sidechain del dispositivo, scegli la traccia della cassa)", r.toNearestInt(), juce::Justification::centred, 3, 1.0f);
        return;
    }
    auto path = [&] (const float* d) { juce::Path p; for (int i = 0; i < N; ++i) { const float x = r.getX() + r.getWidth() * i / (N - 1), y = r.getCentreY() - juce::jlimit (-1.1f, 1.1f, d[i]) * r.getHeight() * 0.46f; if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y); } return p; };
    // cassa: riempita chiara
    { auto p = path (S.kick); juce::Path f = p; f.lineTo (r.getRight(), r.getCentreY()); f.lineTo (r.getX(), r.getCentreY()); f.closeSubPath(); g.setColour (C::text.withAlpha (0.08f)); g.fillPath (f); g.setColour (C::text.withAlpha (0.8f)); g.strokePath (p, juce::PathStrokeType (1.6f)); }
    // basso prima: tratteggiato grigio
    { auto p = path (S.before); juce::Path d; const float dash[] = { 5, 4 }; juce::PathStrokeType (1.3f).createDashedStroke (d, p, dash, 2); g.setColour (C::dim.withAlpha (0.8f)); g.fillPath (d); }
    // basso dopo: colore dello stato
    const auto c = Editor::corrCol (S.corrAfter);
    if ((int) ed.proc.value ("mode") != M_OFF) { auto p = path (S.after); g.setColour (c.withAlpha (0.25f)); g.strokePath (p, juce::PathStrokeType (6.0f)); g.setColour (c); g.strokePath (p, juce::PathStrokeType (2.2f)); }
    // legenda
    auto leg = [&] (float x, const char* t, juce::Colour cc) { g.setColour (cc); g.fillRect (x, b.getY() + 13, 14.0f, 2.5f); g.setColour (C::dim); g.setFont (mf (9.0f)); g.drawText (t, juce::Rectangle<float> (x + 18, b.getY() + 6, 150, 16), juce::Justification::centredLeft); };
    leg (b.getX() + 14, "CASSA (sidechain)", C::text); leg (b.getX() + 160, "BASSO PRIMA", C::dim); leg (b.getX() + 290, "BASSO DOPO", c);
    g.setColour (C::dim); g.setFont (mf (9.0f));
    g.drawText ("ultimo colpo, ~90 ms, sotto " + hzT (ed.proc.value ("freq")), juce::Rectangle<float> (b.getRight() - 260, b.getY() + 6, 248, 16), juce::Justification::centredRight);
}
void StatusView::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat(); lcd (g, b, 8, ed.acc);
    const bool sc = ed.proc.eng.scUi.load();
    const float ra = ed.proc.eng.caUi.load(), rb = ed.proc.eng.cbUi.load();
    shown += (ra - shown) * 0.2f; shownB += (rb - shownB) * 0.2f;
    const auto c = sc ? Editor::corrCol (shown) : C::faint;
    // spia grande
    auto led = juce::Rectangle<float> (b.getCentreX() - 46, b.getY() + 18, 92, 92);
    if (sc) { juce::ColourGradient hg (c.withAlpha (0.45f), led.getCentreX(), led.getCentreY(), c.withAlpha (0.0f), led.getCentreX() + 80, led.getCentreY(), true); g.setGradientFill (hg); g.fillEllipse (led.expanded (34)); }
    juce::ColourGradient lg (c.brighter (0.6f), led.getCentreX() - 15, led.getY() + 10, c.darker (0.6f), led.getCentreX(), led.getBottom(), true); g.setGradientFill (lg); g.fillEllipse (led);
    g.setColour (juce::Colours::black.withAlpha (0.5f)); g.drawEllipse (led, 2.0f);
    glowText (g, ! sc ? "NESSUNA CASSA" : shown > 0.2f ? "IN FASE" : shown > -0.05f ? "A META" : "FUORI FASE", { b.getX(), led.getBottom() + 10, b.getWidth(), 26 }, c, uf (19.0f), juce::Justification::centred);
    // barre di correlazione prima / dopo
    auto bar = [&] (float y, const char* t, float v)
    {
        auto r = juce::Rectangle<float> (b.getX() + 20, y, b.getWidth() - 40, 12); lcd (g, r, 4, C::faint);
        const float cx = r.getCentreX(), x = cx + juce::jlimit (-1.0f, 1.0f, v) * r.getWidth() * 0.5f;
        g.setColour (Editor::corrCol (v)); g.fillRect (juce::Rectangle<float>::leftTopRightBottom (juce::jmin (cx, x), r.getY() + 2, juce::jmax (cx, x), r.getBottom() - 2));
        g.setColour (C::text.withAlpha (0.4f)); g.drawVerticalLine ((int) cx, r.getY() - 2, r.getBottom() + 2);
        g.setColour (C::dim); g.setFont (mf (9.0f)); g.drawText (juce::String (t) + "  " + juce::String (v, 2), juce::Rectangle<float> (r.getX(), y - 15, r.getWidth(), 13), juce::Justification::centredLeft);
        g.setFont (mf (8.0f)); g.drawText ("-1  fuori", juce::Rectangle<float> (r.getX(), r.getBottom() + 1, r.getWidth(), 11), juce::Justification::centredLeft);
        g.drawText ("in fase  +1", juce::Rectangle<float> (r.getX(), r.getBottom() + 1, r.getWidth(), 11), juce::Justification::centredRight);
    };
    bar (b.getY() + 182, "CASSA / BASSO PRIMA", sc ? shownB : 0.0f);
    bar (b.getY() + 224, "CASSA / BASSO DOPO", sc ? shown : 0.0f);
    // quadrante dell'angolo
    const float th = ed.proc.eng.thetaUi.load(); const int m = (int) ed.proc.value ("mode");
    auto dial = juce::Rectangle<float> (b.getCentreX() - 44, b.getY() + 254, 88, 88);
    g.setColour (neo::K::hi.withAlpha (0.08f)); g.drawEllipse (dial, 1.5f);
    for (int k = 0; k < 4; ++k) { const float a = k * juce::MathConstants<float>::halfPi; g.drawLine (dial.getCentreX() + std::sin (a) * 40, dial.getCentreY() - std::cos (a) * 40, dial.getCentreX() + std::sin (a) * 44, dial.getCentreY() - std::cos (a) * 44, 1.0f); }
    const float ang = juce::degreesToRadians (m == M_INV ? 180.0f : th);
    g.setColour (ed.acc2); g.drawLine (dial.getCentreX(), dial.getCentreY(), dial.getCentreX() + std::sin (ang) * 38, dial.getCentreY() - std::cos (ang) * 38, 2.5f);
    g.fillEllipse (dial.getCentreX() - 4, dial.getCentreY() - 4, 8, 8);
    glowText (g, (m == M_INV ? juce::String ("180") : juce::String (juce::roundToInt (th))) + juce::String::fromUTF8 ("\xc2\xb0"), { dial.getRight() + 8, dial.getCentreY() - 10, 70, 20 }, ed.acc2, mf (13.0f));
    g.setColour (C::dim); g.setFont (mf (8.5f)); g.drawText ("ROTAZIONE", juce::Rectangle<float> (dial.getX() - 84, dial.getCentreY() - 8, 80, 16), juce::Justification::centredRight);
}

Editor::Editor (Processor& p) : AudioProcessorEditor (&p), proc (p)
{
    skin::init ("phase", { 0xffa9b8c9, 0xffa8bea0, 0xffe9a260, 0xffccccc7 }, 0xff9b6bff); C::sync(); themeRect = { 872, 15, 100, 36 };
    setLookAndFeel (&lnf);
    auto knob = [&] (const char* id, const char* label, std::function<juce::String()> txt, juce::Colour c) -> Knob*
    { auto k = std::make_unique<Knob> (P (id), c, label); k->text = std::move (txt); addAndMakeVisible (*k); knobs.push_back (std::move (k)); return knobs.back().get(); };
    kFreq = knob ("freq", "FREQUENZA", [this] { return "< " + hzT (proc.value ("freq")); }, acc);
    kAngle = knob ("angle", "ANGOLO", [this] { return juce::String (juce::roundToInt (proc.value ("angle"))) + juce::String::fromUTF8 ("\xc2\xb0"); }, acc2);
    kSpeed = knob ("speed", "VELOCITA", [this] { return juce::String (juce::roundToInt (proc.value ("speed"))) + " ms"; }, acc2);
    kAmount = knob ("amount", "QUANTO", [this] { return pctT (proc.value ("amount")); }, acc2);
    knob ("out", "OUTPUT", [this] { return dbT (proc.value ("out")); }, C::text)->bipolar = true;
    const char* mn[] = { "SPENTO", "INVERTI FASE", "RUOTA", "ADATTA DINAMICA" };
    for (int k = 0; k < kModes; ++k)
    {
        auto pl = std::make_unique<Pill> (P ("mode"), k == M_ADAPT ? acc : k == M_INV ? C::red : k == M_ROT ? acc2 : C::dim, [k, mn] { return juce::String (mn[k]); });
        pl->lit = [this, k] { return (int) proc.value ("mode") == k; };
        pl->click = [this, k] { P ("mode")->setValueNotifyingHost (P ("mode")->convertTo0to1 ((float) k)); };
        pl->fontSize = 12.0f; addAndMakeVisible (*pl); modes.push_back (std::move (pl));
    }
    listen = std::make_unique<Switch> (P ("listen"), C::amber, "ASCOLTA CON LA CASSA"); addAndMakeVisible (*listen);
    addAndMakeVisible (wave); addAndMakeVisible (status); addAndMakeVisible (sizeChip);
    scaler.setup(); startTimerHz (30);
}
void Editor::resized()
{
    const float sc = scaler.update();
    wave.setBounds (30, 108, 616, 336); status.setBounds (680, 108, 290, 336);
    for (int k = 0; k < kModes; ++k) modes[(size_t) k]->setBounds (30 + (k % 2) * 172, 508 + (k / 2) * 52, 164, 44);
    for (size_t i = 0; i < knobs.size(); ++i) knobs[i]->setBounds (390 + (int) i * 92, 506, 66, 66 + 27);
    listen->setBounds (392, 610, 240, 22);
    sizeChip.setBounds (W - 76, H - 18, 58, 15);
    jlook::scaleChildren (*this, sc);
}
void Editor::paint (juce::Graphics& g)
{
    skin::apply(); C::sync();
    g.addTransform (juce::AffineTransform::scale (scaler.s));
    jlook::backdrop (g, { 0, 0, (float) W, (float) H }, acc, acc2);
    jlook::header (g, { 0, 0, (float) W, 64 }, "PHASE", acc, "JF-1  FASE BASSO / 808 CONTRO CASSA", acc, acc2);
    bar.preset = { 400, 15, 300, 36 }; bar.prev = { 362, 15, 36, 36 }; bar.next = { 702, 15, 36, 36 }; bar.ab = { 750, 15, 110, 36 };
    bar.paint (g, proc.getProgramName (proc.preset), acc, proc.ab.cur == 1, proc.ab.label());
    titledCard (g, { 18, 76, 640, 380 }, "ONDA", "cassa e basso nella banda bassa", acc);
    titledCard (g, { 668, 76, 314, 380 }, "FASE", "", acc);
    titledCard (g, { 18, 468, 964, 176 }, "CORREZIONE", "", acc2);
    const int m = (int) proc.value ("mode");
    const char* desc[] = { "Il suono passa com'e. Guarda la spia: se e rossa, basso e cassa si cancellano nei bassi.",
                           "Polarita invertita su tutto il segnale, sempre. Per quando la spia e sempre rossa.",
                           "Ruota la fase del basso sotto la FREQUENZA di un angolo fisso (ANGOLO). Sopra non cambia niente.",
                           "A ogni colpo di cassa trova l'angolo che mette il basso in fase con lei (sotto la FREQUENZA) e ci arriva con la VELOCITA scelta. Tra un colpo e l'altro tiene l'ultimo angolo." };
    g.setColour (C::text.withAlpha (0.7f)); g.setFont (uf (10.0f, false));
    g.drawFittedText (desc[juce::jlimit (0, 3, m)], juce::Rectangle<int> (650, 604, 320, 34), juce::Justification::topLeft, 3, 1.0f);
    skin::chipDraw (g, themeRect.toFloat());
}
void Editor::mouseMove (const juce::MouseEvent& e0) { const auto e = e0.withNewPosition (e0.position / scaler.s); const bool h = bar.ab.contains (e.getPosition()); if (h != bar.abHover) { bar.abHover = h; repaint(); } }
void Editor::mouseDown (const juce::MouseEvent& e0) {
    { const auto tp = e0.position / scaler.s; if (themeRect.toFloat().contains (tp)) { skin::cycle(); C::sync(); for (auto* ch : getChildren()) ch->repaint(); repaint(); return; } } const auto e = e0.withNewPosition (e0.position / scaler.s); bar.click (*this, proc, presets(), e.getPosition(), localAreaToGlobal (scaler.map (bar.preset))); }
void Editor::timerCallback()
{
    const int m = (int) proc.value ("mode");
    kAngle->lit = m == M_ROT; kSpeed->lit = m == M_ROT || m == M_ADAPT; kAmount->lit = m == M_ADAPT; kFreq->lit = m == M_ROT || m == M_ADAPT;
    for (auto& k : knobs) { const float v = k->param->getValue() + (k->lit ? 0.0f : 9.0f); if (v != k->shown) { k->shown = v; k->repaint(); } }
    status.repaint();
    if (proc.snap.serial.load() != lastSerial) { lastSerial = proc.snap.serial.load(); wave.repaint(); }
    if (++tick % 3 == 0) { for (auto& p : modes) p->repaint(); listen->repaint(); }
    if (tick % 10 == 0) { repaint (scaler.map ({ 640, 600, 340, 44 })); wave.repaint(); }
}
}
