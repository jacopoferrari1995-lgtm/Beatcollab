#include "TubeEditor.h"

namespace jtbx
{
void VuView::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    // quadrante color crema come sull'hardware
    g.setColour (juce::Colours::black.withAlpha (0.7f)); g.fillRoundedRectangle (b.expanded (2), 8);
    juce::ColourGradient cg (juce::Colour (0xfff3e2b8), b.getCentreX(), b.getY(), juce::Colour (0xffc9a96a), b.getCentreX(), b.getBottom(), false); g.setGradientFill (cg); g.fillRoundedRectangle (b, 6);
    const bool on = ed.proc.value ("hg_on") > 0.5f;
    if (on) { g.setColour (juce::Colour (0x30ffb040)); g.fillRoundedRectangle (b, 6); }
    const float cx = b.getCentreX(), cy = b.getBottom() + b.getHeight() * 0.35f, R = b.getHeight() * 1.05f;
    // scala da -20 a +3 VU
    auto angOf = [] (float db) { return juce::jmap (juce::jlimit (-20.0f, 3.0f, db), -20.0f, 3.0f, -0.75f, 0.75f); };
    for (float db : { -20.f, -10.f, -7.f, -5.f, -3.f, 0.f, 3.f })
    {
        const float a = angOf (db);
        g.setColour (db > 0 ? juce::Colour (0xffb3261e) : juce::Colour (0xff2a2016)); g.drawLine (cx + std::sin (a) * R * 0.88f, cy - std::cos (a) * R * 0.88f, cx + std::sin (a) * R * 0.96f, cy - std::cos (a) * R * 0.96f, 1.5f);
        g.setFont (mf (8.0f)); g.drawText (db > 0 ? "+" + juce::String ((int) db) : juce::String ((int) db), juce::Rectangle<float> (cx + std::sin (a) * R * 0.78f - 12, cy - std::cos (a) * R * 0.78f - 6, 24, 12), juce::Justification::centred);
    }
    { juce::Path arc; arc.addCentredArc (cx, cy, R * 0.92f, R * 0.92f, 0, angOf (0), angOf (3), true); g.setColour (juce::Colour (0xffb3261e)); g.strokePath (arc, juce::PathStrokeType (3.0f)); }
    g.setColour (juce::Colour (0xff2a2016)); g.setFont (uf (11.0f)); g.drawText ("VU", juce::Rectangle<float> (cx - 20, b.getBottom() - 26, 40, 16), juce::Justification::centred);
    // ago: 0 VU = -14 dBFS RMS
    const float lv = ed.proc.eng.vuUi.load(), db = juce::Decibels::gainToDecibels (lv * 1.414f, -60.0f) + 14.0f;
    needle += (angOf (db) - needle) * 0.25f;
    g.saveState(); g.reduceClipRegion (b.toNearestInt());
    g.setColour (juce::Colour (0xff15110c)); g.drawLine (cx, cy, cx + std::sin (needle) * R, cy - std::cos (needle) * R, 1.6f);
    g.restoreState();
    g.setColour (juce::Colours::white.withAlpha (0.15f)); g.drawRoundedRectangle (b, 6, 1.0f);
}
void BandView::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat(); lcd (g, b, 8, ed.acc2);
    const bool on = ed.proc.value ("mb_on") > 0.5f;
    float xs[6] = { 10 }; for (int k = 0; k < 4; ++k) xs[k + 1] = xOf (ed.proc.valueS ("x" + juce::String (k + 1))); xs[5] = b.getRight() - 10;
    const char* names[] = { "SUB", "BASSI", "MEDI", "PRESENZA", "ARIA" };
    for (int k = 0; k < 5; ++k)
    {
        const float h = ed.proc.valueS ("h" + juce::String (k + 1)), lv = ed.proc.valueS ("l" + juce::String (k + 1)), w = ed.proc.valueS ("w" + juce::String (k + 1));
        auto r = juce::Rectangle<float> (xs[k] + 2, b.getY() + 6, xs[k + 1] - xs[k] - 4, b.getHeight() - 12);
        const bool hot = ed.hotBand == k;
        const auto c = on ? Editor::bandCol (k) : C::faint;
        g.setColour (c.withAlpha (hot ? 0.16f : 0.06f)); g.fillRoundedRectangle (r, 4);
        if (hot) { g.setColour (c.withAlpha (0.8f)); g.drawRoundedRectangle (r, 4, 1.5f); }
        // barra delle armoniche e livello
        const float lvH = r.getHeight() * 0.5f * juce::jlimit (-1.0f, 1.0f, lv / 12.0f);
        g.setColour (c.withAlpha (0.12f + h * 0.4f)); g.fillRoundedRectangle (r.withTop (r.getCentreY() - lvH - h * r.getHeight() * 0.35f).withBottom (r.getBottom() - 4), 4);
        const float act = juce::jlimit (0.0f, 1.0f, ed.proc.eng.bandUi[k].load() * 6.0f);
        g.setColour (ed.gold.withAlpha (0.25f + 0.5f * act)); g.fillRect (r.withTop (r.getBottom() - 4));
        g.setColour (on ? c : C::dim); g.setFont (uf (10.5f)); g.drawText (names[k], r.withHeight (16).translated (0, 22), juce::Justification::centred);
        g.setColour (C::dim); g.setFont (mf (8.5f));
        g.drawText ("H " + pctT (h) + "  W " + pctT (w), r.withTop (r.getBottom() - 22).withHeight (14), juce::Justification::centred);
    }
    for (int k = 1; k <= 4; ++k)
    {
        const bool sel = drag == k - 1 || hover == k - 1;
        g.setColour (ed.gold.withAlpha (sel ? 0.9f : 0.5f)); g.drawLine (xs[k], b.getY() + 4, xs[k], b.getBottom() - 4, sel ? 2.0f : 1.2f);
        g.setFont (mf (8.5f)); g.drawText (hzT (ed.proc.valueS ("x" + juce::String (k))), juce::Rectangle<float> (xs[k] - 30, b.getY() + 4, 60, 12), juce::Justification::centred);
    }
}
void BandView::mouseMove (const juce::MouseEvent& e)
{
    int h = -1; for (int k = 0; k < 4; ++k) if (std::abs (e.position.x - xOf (ed.proc.valueS ("x" + juce::String (k + 1)))) < 8) h = k;
    if (h != hover) { hover = h; setMouseCursor (h >= 0 ? juce::MouseCursor::LeftRightResizeCursor : juce::MouseCursor::NormalCursor); repaint(); }
}
void BandView::mouseDown (const juce::MouseEvent& e) { drag = -1; for (int k = 0; k < 4; ++k) if (std::abs (e.position.x - xOf (ed.proc.valueS ("x" + juce::String (k + 1)))) < 8) drag = k; }
void BandView::mouseDrag (const juce::MouseEvent& e)
{
    if (drag < 0) return;
    auto* p = ed.P ("x" + juce::String (drag + 1)); p->setValueNotifyingHost (p->convertTo0to1 (fOf (e.position.x))); repaint();
}

Editor::Editor (Processor& p) : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);
    auto knob = [&] (const juce::String& id, const juce::String& label, std::function<juce::String()> txt, int sec, juce::Colour c, bool big = false) -> Knob*
    {
        auto k = std::make_unique<Knob> (P (id), c, label); k->text = std::move (txt); k->big = big;
        k->onTouch = [this, sec] { if (sec >= 0) { auto* pp = P (sec == 0 ? "hg_on" : "mb_on"); if (pp->getValue() < 0.5f) pp->setValueNotifyingHost (1.0f); } };
        addAndMakeVisible (*k); knobs.push_back ({ std::move (k), sec }); return knobs.back().k.get();
    };
    auto ten = [this] (const char* id) { return [this, id] { return juce::String (proc.value (id), 1); }; };
    knob ("in", "INPUT", [this] { return dbT (proc.value ("in")); }, 0, gold)->bipolar = true;
    knob ("sat", "SATURATION", ten ("sat"), 0, gold, true);
    knob ("pentode", "PENTODE", ten ("pentode"), 0, gold, true);
    knob ("density", "DENSITY", ten ("density"), 0, gold);
    knob ("triode", "TRIODE", ten ("triode"), 0, gold, true);
    knob ("air", "AIR", ten ("air"), 0, gold);
    knob ("mix", "MIX", [this] { return pctT (proc.value ("mix")); }, 0, gold);
    knob ("out", "OUTPUT", [this] { return dbT (proc.value ("out")); }, -1, C::text)->bipolar = true;
    for (int k = 1; k <= 5; ++k)
    {
        const juce::String s = juce::String (k);
        const auto bc = bandCol (k - 1);
        knob ("h" + s, "ARMONICHE", [this, s] { return pctT (proc.valueS ("h" + s)); }, 1, bc);
        knob ("l" + s, "LIVELLO", [this, s] { return dbT (proc.valueS ("l" + s)); }, 1, bc)->bipolar = true;
        knob ("w" + s, "LARGHEZZA", [this, s] { return pctT (proc.valueS ("w" + s)); }, 1, bc)->bipolar = true;
    }
    knob ("mb_mix", "MIX", [this] { return pctT (proc.value ("mb_mix")); }, 1, acc2);
    auto sw = [&] (const char* id, juce::Colour c, const char* l) { switches.push_back (std::make_unique<Switch> (P (id), c, l)); addAndMakeVisible (*switches.back()); };
    sw ("hg_on", C::red, "POWER"); sw ("sat_in", gold, "SATURATION IN"); sw ("alt", gold, "ALT TUBE"); sw ("air_in", gold, "AIR IN"); sw ("ms", gold, "MID / SIDE"); sw ("mb_on", acc2, "ENHANCER ON");
    auto seg = [&] (const char* id, juce::StringArray it, juce::Colour c) { segs.push_back (std::make_unique<Segs> (P (id), c, it)); segs.back()->font = 9.5f; addAndMakeVisible (*segs.back()); };
    seg ("satFreq", { "LOW", "FLAT", "HIGH" }, gold); seg ("calib", { "DARK", "NORMAL", "BRIGHT" }, gold); seg ("char", { "TUBO", "NASTRO", "TRANSISTOR" }, acc2);
    addAndMakeVisible (vu); addAndMakeVisible (bands); addAndMakeVisible (sizeChip);
    scaler.setup(); startTimerHz (30);
}
void Editor::resized()
{
    const float sc = scaler.update();
    auto K = [this] (const char* id) -> Knob* { for (auto& ks : knobs) if (ks.k->param == P (id)) return ks.k.get(); return nullptr; };
    // valvole
    vu.setBounds (36, 120, 220, 128);
    switches[0]->setBounds (40, 262, 110, 22); switches[4]->setBounds (40, 292, 150, 22);
    K ("in")->setBounds (178, 296, 62, 62 + 27);
    switches[1]->setBounds (292, 116, 160, 22);
    K ("sat")->setBounds (300, 146, 110, 110 + 32);
    segs[0]->setBounds (286, 312, 140, 26); switches[2]->setBounds (292, 348, 140, 22);
    K ("pentode")->setBounds (470, 122, 130, 130 + 32);
    K ("density")->setBounds (500, 300, 70, 70 + 27);
    K ("triode")->setBounds (650, 122, 130, 130 + 32);
    switches[3]->setBounds (818, 116, 110, 22); K ("air")->setBounds (826, 146, 70, 70 + 27);
    segs[1]->setBounds (630, 300, 170, 26);
    K ("mix")->setBounds (960, 140, 70, 70 + 27); K ("out")->setBounds (1060, 140, 70, 70 + 27);
    // enhancer
    switches[5]->setBounds (30, 476, 170, 22);
    bands.setBounds (30, 506, 1120, 110);
    for (int k = 0; k < 5; ++k)
    {
        const float x0 = 30 + k * 186.0f;
        K (("h" + juce::String (k + 1)).toRawUTF8())->setBounds ((int) x0 + 4, 664, 56, 56 + 27);
        K (("l" + juce::String (k + 1)).toRawUTF8())->setBounds ((int) x0 + 64, 664, 56, 56 + 27);
        K (("w" + juce::String (k + 1)).toRawUTF8())->setBounds ((int) x0 + 124, 664, 56, 56 + 27);
    }
    segs[2]->setBounds (970, 652, 180, 26);
    K ("mb_mix")->setBounds (1025, 686, 62, 62 + 27);
    sizeChip.setBounds (W - 76, H - 18, 58, 15);
    jlook::scaleChildren (*this, sc);
}
void Editor::paint (juce::Graphics& g)
{
    g.addTransform (juce::AffineTransform::scale (scaler.s));
    jlook::backdrop (g, { 0, 0, (float) W, (float) H }, gold, acc2);
    jlook::header (g, { 0, 0, (float) W, 64 }, "TUBE BOX", gold, "JB-2  PENTODE / TRIODE  +  ENHANCER MULTIBANDA", gold, acc2);
    bar.preset = { 470, 15, 330, 36 }; bar.prev = { 432, 15, 36, 36 }; bar.next = { 802, 15, 36, 36 }; bar.ab = { 850, 15, 110, 36 };
    bar.paint (g, proc.getProgramName (proc.preset), gold, proc.ab.cur == 1, proc.ab.label());
    const bool on1 = proc.value ("hg_on") > 0.5f, on2 = proc.value ("mb_on") > 0.5f;
    titledCard (g, hgCard, "VALVOLE  HG", "6U8A pentode + triode, saturazione parallela 12AT7", gold, on1);
    titledCard (g, mbCard, "ENHANCER MULTIBANDA", "armoniche, livello e larghezza per banda", acc2, on2);
    // pannello frontale nero con serigrafia oro
    auto face = hgCard.reduced (10, 0).withTrimmedTop (34).withTrimmedBottom (10);
    g.setColour (juce::Colour (0xff0d0d0f)); g.fillRoundedRectangle (face.withTrimmedLeft (250).withTrimmedRight (210), 8);
    g.setColour (gold.withAlpha (0.25f)); g.drawRoundedRectangle (face.withTrimmedLeft (250).withTrimmedRight (210), 8, 1.0f);
    g.setColour (gold.withAlpha (0.18f));
    for (float x : { 444.f, 622.f, 806.f }) g.drawVerticalLine ((int) x, face.getY() + 14, face.getBottom() - 14);
    g.setColour (gold.withAlpha (0.7f)); g.setFont (mf (8.5f));
    g.drawText ("PARALLELO 12AT7", juce::Rectangle<float> (276, 376, 170, 14), juce::Justification::centred);
    g.drawText ("SAT FREQ", juce::Rectangle<float> (286, 296, 140, 14), juce::Justification::centred);
    g.drawText ("CALIBRATION", juce::Rectangle<float> (630, 284, 170, 14), juce::Justification::centred);
    g.drawText ("STADI IN SERIE", juce::Rectangle<float> (460, 404, 330, 14), juce::Justification::centred);
    g.setColour (C::dim); g.setFont (uf (9.5f, false));
    g.drawFittedText ("PENTODE spinge e satura per primo (dispari), TRIODE aggiunge la seconda armonica e regola il volume. SATURATION somma in parallelo una valvola filtrata dal SAT FREQ. DENSITY guida entrambe senza alzare il volume.",
                      juce::Rectangle<int> (946, 252, 196, 100), juce::Justification::topLeft, 7, 1.0f);
    g.drawFittedText ("Trascina le linee per spostare i tagli. ARMONICHE aggiunge solo armoniche in parallelo, LIVELLO alza o abbassa la banda, LARGHEZZA stringe (bassi mono) o allarga.",
                      juce::Rectangle<int> (220, 474, 760, 30), juce::Justification::topLeft, 2, 1.0f);
    g.setColour (gold.withAlpha (0.8f)); g.setFont (mf (8.5f)); g.drawText ("CARATTERE", juce::Rectangle<float> (970, 636, 180, 14), juce::Justification::centred);
    // ogni gruppo di manopole collegato alla sua zona del grafico: imbuto colorato + nome e frequenze
    {
        const char* names[] = { "SUB", "BASSI", "MEDI", "PRESENZA", "ARIA" };
        float xs[6]; xs[0] = (float) bands.getX() + 10; for (int k = 0; k < 4; ++k) xs[k + 1] = (float) bands.getX() + bands.xOf (proc.valueS ("x" + juce::String (k + 1))); xs[5] = (float) bands.getRight() - 10;
        const float yTop = (float) bands.getBottom() + 1, yBot = 634;
        for (int k = 0; k < 5; ++k)
        {
            const auto c = on2 ? bandCol (k) : C::faint; const bool hot = hotBand == k;
            const float gx0 = 30 + k * 186.0f + 2, gx1 = gx0 + 180;
            juce::Path p; p.startNewSubPath (xs[k] + 2, yTop); p.lineTo (xs[k + 1] - 2, yTop); p.lineTo (gx1, yBot); p.lineTo (gx0, yBot); p.closeSubPath();
            g.setColour (c.withAlpha (hot ? 0.28f : 0.10f)); g.fillPath (p);
            auto hr = juce::Rectangle<float> (gx0, yBot, 180, 22);
            g.setColour (c.withAlpha (hot ? 0.35f : 0.18f)); g.fillRoundedRectangle (hr, 4);
            g.setColour (c); g.drawRoundedRectangle (hr, 4, hot ? 1.6f : 1.0f);
            const juce::String lo = k == 0 ? juce::String ("20 Hz") : hzT (proc.valueS ("x" + juce::String (k))), hi = k == 4 ? juce::String ("20 kHz") : hzT (proc.valueS ("x" + juce::String (k + 1)));
            g.setFont (uf (10.5f)); g.drawText (names[k], hr.reduced (8, 0), juce::Justification::centredLeft);
            g.setColour (c.withAlpha (0.8f)); g.setFont (mf (8.5f)); g.drawText (lo + " - " + hi, hr.reduced (8, 0), juce::Justification::centredRight);
            g.setColour (c.withAlpha (0.25f)); g.drawRoundedRectangle (juce::Rectangle<float> (gx0, yBot + 24, 180, 92), 6, 1.0f);
        }
    }
}
void Editor::mouseMove (const juce::MouseEvent& e0) { const auto e = e0.withNewPosition (e0.position / scaler.s); const bool h = bar.ab.contains (e.getPosition()); if (h != bar.abHover) { bar.abHover = h; repaint(); } }
void Editor::mouseDown (const juce::MouseEvent& e0)
{
    const auto e = e0.withNewPosition (e0.position / scaler.s);
    if (hgCard.withHeight (32).contains (e.position)) { auto* p = P ("hg_on"); p->setValueNotifyingHost (p->getValue() > 0.5f ? 0.0f : 1.0f); repaint(); return; }
    if (mbCard.withHeight (32).contains (e.position)) { auto* p = P ("mb_on"); p->setValueNotifyingHost (p->getValue() > 0.5f ? 0.0f : 1.0f); repaint(); return; }
    bar.click (*this, proc, presets(), e.getPosition(), localAreaToGlobal (scaler.map (bar.preset)));
}
void Editor::timerCallback()
{
    const bool on1 = proc.value ("hg_on") > 0.5f, on2 = proc.value ("mb_on") > 0.5f;
    for (auto& ks : knobs) { const bool lit = ks.sec < 0 || (ks.sec == 0 ? on1 : on2); const float v = ks.k->param->getValue() + (lit ? 0.0f : 9.0f); if (lit != ks.k->lit || v != ks.k->shown) { ks.k->lit = lit; ks.k->shown = v; ks.k->repaint(); } }
    vu.repaint();
    {   // banda "calda": mouse su una delle sue manopole o sulla sua zona del grafico
        int hb = -1;
        for (int k = 1; k <= 5 && hb < 0; ++k) for (const char* t : { "h", "l", "w" }) if (auto* pp = P (t + juce::String (k))) for (auto& ks : knobs) if (ks.k->param == pp && ks.k->isMouseOverOrDragging()) hb = k - 1;
        if (hb < 0 && bands.isMouseOver())
        {
            const float mx = (float) bands.getMouseXYRelative().x; hb = 0;
            for (int k = 0; k < 4; ++k) if (mx > bands.xOf (proc.valueS ("x" + juce::String (k + 1)))) hb = k + 1;
        }
        if (hb != hotBand) { hotBand = hb; bands.repaint(); repaint (scaler.map ({ 18, 610, 940, 160 })); }
    }
    if (++tick % 3 == 0) { bands.repaint(); for (auto& s : switches) s->repaint(); for (auto& s : segs) { s->lit = (s->param == P ("char")) ? on2 : on1; s->repaint(); } }
    if (tick % 15 == 0) repaint (scaler.map ({ 0, 76, W, 40 }));
}
}
