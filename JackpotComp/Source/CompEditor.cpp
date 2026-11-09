#include "CompEditor.h"

namespace jc
{
juce::Font uf (float size, bool bold) { return jlook::ui (size, bold ? 2 : 1); }
juce::Font mf (float size) { return jlook::mono (size); }

void glowText (juce::Graphics& g, const juce::String& t, juce::Rectangle<float> r, juce::Colour c, juce::Font f, juce::Justification j)
{
    g.setFont (f);
    for (int k = 2; k >= 1; --k) { g.setColour (c.withAlpha (0.12f)); g.drawText (t, r.translated ((float) -k * 0.6f, 0), j); g.drawText (t, r.translated ((float) k * 0.6f, 0), j); }
    g.setColour (c); g.drawText (t, r, j);
}
void lcd (juce::Graphics& g, juce::Rectangle<float> r, float rad)
{
    g.setColour (juce::Colours::black.withAlpha (0.65f)); g.fillRoundedRectangle (r.expanded (1.5f), rad + 1.5f);
    juce::ColourGradient cg (C::lcd1, r.getCentreX(), r.getCentreY(), C::lcd2, r.getX(), r.getY(), true); g.setGradientFill (cg); g.fillRoundedRectangle (r, rad);
    g.setColour (C::phos.withAlpha (0.03f));
    for (float x = r.getX() + 10; x < r.getRight(); x += 12) g.drawVerticalLine ((int) x, r.getY() + 2, r.getBottom() - 2);
    for (float y = r.getY() + 10; y < r.getBottom(); y += 12) g.drawHorizontalLine ((int) y, r.getX() + 2, r.getRight() - 2);
}
static void scan (juce::Graphics& g, juce::Rectangle<float> r)
{
    g.setColour (juce::Colours::black.withAlpha (0.18f)); for (float y = r.getY(); y < r.getBottom(); y += 3) g.drawHorizontalLine ((int) y, r.getX(), r.getRight());
    jlook::glass (g, r, 6);
}

// ======================= Knob =======================
void Knob::mouseDown (const juce::MouseEvent& e)
{
    if (onTouch) onTouch();
    if (! enabled) return;
    if (e.mods.isAltDown() || e.mods.isCommandDown()) { param->setValueNotifyingHost (param->getDefaultValue()); return; }
    start = param->getValue(); param->beginChangeGesture(); drag = true;
}
void Knob::mouseDrag (const juce::MouseEvent& e)
{
    if (! drag) return;
    float v = juce::jlimit (0.0f, 1.0f, start - e.getDistanceFromDragStartY() / (e.mods.isShiftDown() ? 900.0f : 200.0f));
    if (stepped && steps > 1) v = std::round (v * (steps - 1)) / (steps - 1);
    param->setValueNotifyingHost (v);
}
void Knob::paint (juce::Graphics& g)
{
    const float lh = big ? 30.0f : 27.0f;
    const float size = juce::jmin ((float) getWidth(), getHeight() - lh);
    const float cx = getWidth() * 0.5f, cy = size * 0.5f + 1, R = size * 0.5f - 3, r = R - (big ? 11.0f : 8.0f);
    const float v = param->getValue(), a0 = -2.42f, a1 = 2.42f;
    const auto c = enabled ? col : C::faint;
    jlook::knobHalo (g, cx, cy, R, c, ! enabled ? 0.0f : drag ? 1.0f : hover ? 0.6f : 0.0f);
    const int nLed = big ? 31 : 21;
    for (int i = 0; i < nLed; ++i)
    {
        const float t = i / (float) (nLed - 1), a = a0 + t * (a1 - a0);
        const bool lit = enabled && (bipolar ? ((v >= 0.5f && t >= 0.5f && t <= v + 0.001f) || (v < 0.5f && t <= 0.5f && t >= v - 0.001f)) : (t <= v + 0.001f && v > 0.002f));
        const float x = cx + std::sin (a) * R, y = cy - std::cos (a) * R, d = big ? 4.2f : 3.2f;
        if (lit) { g.setColour (c.withAlpha (0.25f)); g.fillEllipse (x - d * 1.3f, y - d * 1.3f, d * 2.6f, d * 2.6f); g.setColour (c.brighter (0.25f)); }
        else g.setColour (juce::Colour (0xff23262e));
        g.fillEllipse (x - d * 0.5f, y - d * 0.5f, d, d);
    }
    g.setColour (juce::Colours::black.withAlpha (0.7f)); g.fillEllipse (cx - r - 1, cy - r + 4, r * 2 + 2, r * 2 + 2);
    juce::ColourGradient sk (juce::Colour (0xff4a4f5a), cx, cy - r, juce::Colour (0xff0c0d11), cx, cy + r, false); g.setGradientFill (sk); g.fillEllipse (cx - r, cy - r, r * 2, r * 2);
    const float r2 = r * 0.8f;
    juce::ColourGradient cap (juce::Colour (0xff343843), cx - r2 * 0.5f, cy - r2, juce::Colour (0xff111318), cx + r2 * 0.4f, cy + r2, false); g.setGradientFill (cap); g.fillEllipse (cx - r2, cy - r2, r2 * 2, r2 * 2);
    for (int i = 0; i < 36; ++i) { const float a = i * juce::MathConstants<float>::twoPi / 36; g.setColour (juce::Colours::black.withAlpha (0.4f)); g.drawLine (cx + std::sin (a) * r2, cy - std::cos (a) * r2, cx + std::sin (a) * r, cy - std::cos (a) * r, 1.0f); }
    juce::ColourGradient spec (juce::Colours::white.withAlpha (0.16f), cx - r2 * 0.4f, cy - r2 * 0.7f, juce::Colours::transparentWhite, cx, cy, true); g.setGradientFill (spec); g.fillEllipse (cx - r2, cy - r2, r2 * 2, r2 * 2);
    g.setColour (c.withAlpha (0.5f)); g.drawEllipse (cx - r2, cy - r2, r2 * 2, r2 * 2, 1.0f);
    const float a = a0 + v * (a1 - a0);
    g.setColour (c.withAlpha (0.4f)); g.drawLine (cx + std::sin (a) * r2 * 0.25f, cy - std::cos (a) * r2 * 0.25f, cx + std::sin (a) * r2 * 0.95f, cy - std::cos (a) * r2 * 0.95f, big ? 6.0f : 4.0f);
    g.setColour (enabled ? juce::Colours::white : C::dim); g.drawLine (cx + std::sin (a) * r2 * 0.3f, cy - std::cos (a) * r2 * 0.3f, cx + std::sin (a) * r2 * 0.92f, cy - std::cos (a) * r2 * 0.92f, big ? 2.6f : 1.8f);
    g.setColour (C::dim); g.setFont (uf (big ? 10.5f : 9.5f)); g.drawText (label, juce::Rectangle<float> (0, size + 1, (float) getWidth(), 13), juce::Justification::centred);
    glowText (g, text ? text() : param->getCurrentValueAsText(), juce::Rectangle<float> (0, size + 14, (float) getWidth(), 13), enabled ? (drag ? c.brighter (0.4f) : c) : C::faint, mf (big ? 11.5f : 10.5f), juce::Justification::centred);
}

// ======================= Segs / Switch =======================
void Segs::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat(); lcd (g, b, 6);
    const auto it = items(); const int n = juce::jmax (1, it.size()), cur = juce::roundToInt (param->convertFrom0to1 (param->getValue()));
    const float w = (b.getWidth() - 6) / n;
    for (int i = 0; i < n; ++i)
    {
        auto r = juce::Rectangle<float> (3 + i * w, 3, w, b.getHeight() - 6).reduced (1, 0);
        if (i == cur) { g.setColour (col.withAlpha (0.18f)); g.fillRoundedRectangle (r, 4); g.setColour (col.withAlpha (0.85f)); g.drawRoundedRectangle (r, 4, 1.0f); glowText (g, it[i], r, col, uf (11.0f), juce::Justification::centred); }
        else { g.setColour (C::dim); g.setFont (uf (11.0f)); g.drawText (it[i], r, juce::Justification::centred); }
    }
    g.setColour (juce::Colours::black.withAlpha (0.15f)); for (float y = b.getY() + 2; y < b.getBottom(); y += 3) g.drawHorizontalLine ((int) y, b.getX() + 2, b.getRight() - 2);
}
void Segs::mouseDown (const juce::MouseEvent& e)
{
    const int n = juce::jmax (1, items().size()); const int k = juce::jlimit (0, n - 1, (int) ((e.position.x - 3) / ((getWidth() - 6) / (float) n)));
    param->setValueNotifyingHost (param->convertTo0to1 ((float) k)); if (onTouch) onTouch(); repaint();
}
void Switch::paint (juce::Graphics& g)
{
    const bool on = param->getValue() > 0.5f; const float h = (float) juce::jmin (getHeight(), 20), w = h * 1.8f;
    auto t = juce::Rectangle<float> (0, (getHeight() - h) / 2, w, h);
    lcd (g, t, h / 2);
    const float d = h - 6, x = on ? t.getRight() - d - 3 : 3.0f;
    if (on) { g.setColour (col.withAlpha (0.35f)); g.fillEllipse (x - 3, t.getY(), d + 6, d + 6); }
    juce::ColourGradient lg (on ? col.brighter (0.6f) : juce::Colour (0xff4a4f5a), x, t.getY() + 3, on ? col : juce::Colour (0xff15171c), x, t.getY() + 3 + d, false); g.setGradientFill (lg); g.fillEllipse (x, t.getY() + 3, d, d);
    if (label.isNotEmpty()) { if (on) glowText (g, label, { w + 8, 0, getWidth() - w - 8, (float) getHeight() }, col, uf (11.0f)); else { g.setColour (C::dim); g.setFont (uf (11.0f)); g.drawText (label, juce::Rectangle<float> (w + 8, 0, getWidth() - w - 8, (float) getHeight()), juce::Justification::centredLeft); } }
}

// ======================= Stadio =======================
StageCard::StageCard (Editor& e, int st) : ed (e), s (st), pfx (st == 0 ? "a_" : "b_")
{
    auto touch = [this] { ed.focus = s; ed.repaint(); };
    const auto c = s == 0 ? C::phos : C::amber;
    power = std::make_unique<Switch> (ed.P (pfx + "on"), c); power->onTouch = touch;
    ratio = std::make_unique<Segs> (ed.P (pfx + "ratio"), c); ratio->onTouch = touch;
    ratio->items = [this] { return info ((int) ed.proc.value ((pfx + "model").toRawUTF8())).ratios; };
    auto mk = [&] (const char* id, const char* lab) { auto k = std::make_unique<Knob> (ed.P (pfx + id), c); k->label = lab; k->onTouch = touch; addAndMakeVisible (*k); return k; };
    drive = mk ("drive", "INPUT"); drive->big = true;
    att = mk ("att", "ATTACK"); rel = mk ("rel", "RELEASE"); out = mk ("out", "OUTPUT"); color = mk ("color", "COLOR"); mix = mk ("mix", "MIX");
    auto M = [this] { return (int) ed.proc.value ((pfx + "model").toRawUTF8()); };
    drive->text = [this, M] { return fmtDrive (M(), ed.proc.value ((pfx + "drive").toRawUTF8())); };
    att->text = [this, M] { const int m = M(); return info (m).attKnob ? fmtTime (attMs (m, ed.proc.value ((pfx + "att").toRawUTF8()))) : juce::String (m <= OPTO3A ? "OTTICO" : m == VARIMU ? "TC" : "AUTO"); };
    rel->text = [this, M] { const int m = M(); return info (m).relKnob ? fmtTime (relMs (m, ed.proc.value ((pfx + "rel").toRawUTF8()))) : juce::String (m <= OPTO3A ? "OTTICO" : m == VARIMU ? "TC" : "120 dB/s"); };
    out->text = [this] { const float v = ed.proc.value ((pfx + "out").toRawUTF8()); return (v > 0 ? "+" : "") + juce::String (v, 1) + " dB"; };
    color->text = [this] { return juce::String (juce::roundToInt (ed.proc.value ((pfx + "color").toRawUTF8()) * 100)) + "%"; };
    mix->text = [this] { return juce::String (juce::roundToInt (ed.proc.value ((pfx + "mix").toRawUTF8()) * 100)) + "%"; };
    addAndMakeVisible (*power); addAndMakeVisible (*ratio);
    refresh();
}
void StageCard::refresh()
{
    const int m = (int) ed.proc.value ((pfx + "model").toRawUTF8()); const auto& I = info (m);
    drive->label = I.driveLabel; att->enabled = I.attKnob; rel->enabled = I.relKnob;
    att->stepped = rel->stepped = m == VCABUS; att->steps = 6; rel->steps = 5;
    if (m == DISTRESS) color->label = ed.proc.value ((pfx + "color").toRawUTF8()) < 0.5f ? "DIST 2" : "DIST 3"; else color->label = m == FET76 ? "GRINTA" : m <= OPTO3A || m == VARIMU ? "VALVOLA" : "COLOR";
    lastModel = m; repaint();
}
void StageCard::resized()
{
    power->setBounds (14, 12, 38, 20);
    ratio->setBounds (16, 186, getWidth() - 32, 28);
    const int cw = (getWidth() - 20) / 3;
    drive->setBounds (10 + (cw - 90) / 2, 224, 90, 90 + 30);
    att->setBounds (10 + cw + (cw - 72) / 2, 232, 72, 72 + 27); rel->setBounds (10 + cw * 2 + (cw - 72) / 2, 232, 72, 72 + 27);
    out->setBounds (10 + (cw - 66) / 2, 350, 66, 66 + 27); color->setBounds (10 + cw + (cw - 66) / 2, 350, 66, 66 + 27); mix->setBounds (10 + cw * 2 + (cw - 66) / 2, 350, 66, 66 + 27);
}
void StageCard::mouseDown (const juce::MouseEvent& e)
{
    ed.focus = s; ed.repaint();
    if (modelRect().contains (e.getPosition()))
    {
        juce::PopupMenu m; auto* pr = dynamic_cast<juce::AudioParameterChoice*> (ed.P (pfx + "model"));
        m.addSectionHeader ("MODELLO");
        for (int i = 0; i < kNumModels; ++i) m.addItem (i + 1, juce::String (info (i).name) + "   -   " + info (i).inspired, true, i == pr->getIndex());
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withTargetScreenArea (localAreaToGlobal (modelRect())), [this, pr] (int r)
        {
            if (r <= 0) return;
            pr->setValueNotifyingHost (pr->convertTo0to1 ((float) (r - 1)));
            auto* rp = ed.P (pfx + "ratio"); const int n = info (r - 1).ratios.size();
            if (rp->convertFrom0to1 (rp->getValue()) > n - 1) rp->setValueNotifyingHost (rp->convertTo0to1 ((float) (n - 1)));
            refresh();
        });
    }
}
void StageCard::paint (juce::Graphics& g)
{
    const int m = (int) ed.proc.value ((pfx + "model").toRawUTF8()); if (m != lastModel) refresh();
    const auto& I = info (m); const bool on = power->param->getValue() > 0.5f && (s == 0 || ed.proc.value ("route") > 0.5f);
    const auto c = s == 0 ? C::phos : C::amber;
    auto b = getLocalBounds().toFloat().reduced (1);
    jlook::card (g, b, 14, juce::Colour (I.accent), on, false, false);
    if (ed.focus == s) { g.setColour (c.withAlpha (0.5f)); g.drawRoundedRectangle (b.reduced (2), 12, 1.0f); }
    glowText (g, s == 0 ? "STADIO A" : "STADIO B", { 60, 9, 120, 26 }, on ? C::text : C::dim, uf (13.0f));
    // modello
    auto mr = modelRect().toFloat(); jlook::button (g, mr, juce::Colour (I.accent), true, false, juce::String (I.name) + juce::String::fromUTF8 ("  \xe2\x96\xbe"), uf (12.5f));
    g.setColour (C::dim); g.setFont (uf (10.5f, false)); g.drawText (I.inspired, juce::Rectangle<float> (16, 38, b.getWidth() - 32, 18), juce::Justification::centredRight);
    if (s == 1 && ed.proc.value ("route") < 0.5f) { g.setColour (C::amber.withAlpha (0.8f)); g.setFont (uf (10.5f)); g.drawText ("attivo in SERIE o PARALLELO", juce::Rectangle<float> (16, 38, 220, 18), juce::Justification::centredLeft); }
    // VU analogico di riduzione
    auto vu = vuRect();
    jlook::softShadow (g, vu, 8, 10, 0.6f, 3);
    const auto face = juce::Colour (I.face); const bool light = face.getPerceivedBrightness() > 0.5f;
    juce::ColourGradient fg (face.brighter (0.15f), vu.getCentreX(), vu.getY(), face.darker (0.25f), vu.getCentreX(), vu.getBottom(), false); g.setGradientFill (fg); g.fillRoundedRectangle (vu, 8);
    // lampadina calda dietro
    juce::ColourGradient lamp (juce::Colour (0xffffc46b).withAlpha (light ? 0.35f : 0.12f), vu.getCentreX(), vu.getBottom(), juce::Colours::transparentBlack, vu.getCentreX(), vu.getY(), true); g.setGradientFill (lamp); g.fillRoundedRectangle (vu, 8);
    const auto ink = light ? juce::Colour (0xff1b1712) : juce::Colour (0xffe9e4d8);
    const float pcx = vu.getCentreX(), pcy = vu.getBottom() + 40, rad = vu.getHeight() + 6;
    // scala: 0 a destra, -20 a sinistra
    auto angOf = [] (float grDb) { const float t = 1.0f - std::sqrt (juce::jlimit (0.0f, 20.0f, grDb) / 20.0f); return -0.75f + t * 1.5f; };
    juce::Path arc; arc.addCentredArc (pcx, pcy, rad - 20, rad - 20, 0, angOf (20), angOf (0), true);
    g.setColour (ink.withAlpha (0.8f)); g.strokePath (arc, juce::PathStrokeType (1.2f));
    juce::Path red; red.addCentredArc (pcx, pcy, rad - 18, rad - 18, 0, angOf (20), angOf (10), true); g.setColour (juce::Colour (0xffd22b2b).withAlpha (0.8f)); g.strokePath (red, juce::PathStrokeType (3.0f));
    for (int db : { 0, 1, 2, 3, 5, 7, 10, 15, 20 })
    {
        const float a = angOf ((float) db), sx = std::sin (a), cy2 = -std::cos (a);
        g.setColour (ink); g.drawLine (pcx + sx * (rad - 20), pcy + cy2 * (rad - 20), pcx + sx * (rad - 28), pcy + cy2 * (rad - 28), 1.3f);
        if (db == 1 || db == 2 || db == 7 || db == 15) continue;
        g.setFont (mf (9.0f)); g.drawText (juce::String (db == 0 ? 0 : -db), juce::Rectangle<float> (pcx + sx * (rad - 8) - 12, pcy + cy2 * (rad - 8) - 6, 24, 12), juce::Justification::centred);
    }
    g.setColour (ink.withAlpha (0.7f)); g.setFont (uf (10.0f)); g.drawText ("GAIN REDUCTION", vu.withTrimmedBottom (14).withTrimmedTop (vu.getHeight() - 30), juce::Justification::centred);
    g.setFont (mf (9.5f)); g.drawText ("dB", vu.reduced (10, 6), juce::Justification::topLeft);
    glowText (g, "-" + juce::String (needle, 1), vu.reduced (10, 6), light ? juce::Colour (0xff8a1e1e) : c, mf (11.0f), juce::Justification::topRight);
    { // ago
        const float a = angOf (needle); g.saveState(); g.reduceClipRegion (vu.toNearestInt());
        g.setColour (juce::Colours::black.withAlpha (0.25f)); g.drawLine (pcx + 3, pcy, pcx + 3 + std::sin (a) * (rad - 14), pcy - std::cos (a) * (rad - 14), 2.0f);
        g.setColour (light ? juce::Colour (0xff111111) : juce::Colour (0xffff4040)); g.drawLine (pcx, pcy, pcx + std::sin (a) * (rad - 14), pcy - std::cos (a) * (rad - 14), 1.6f);
        g.restoreState();
    }
    jlook::glass (g, vu, 8); g.setColour (juce::Colours::black.withAlpha (0.6f)); g.drawRoundedRectangle (vu, 8, 2.0f);
    // etichetta ratio
    g.setColour (C::dim); g.setFont (uf (10.0f)); g.drawText (I.ratioLabel, juce::Rectangle<float> (16, 166, 140, 18), juce::Justification::centredLeft);
    if (! on) { g.setColour (juce::Colour (0xff101216).withAlpha (0.55f)); g.fillRoundedRectangle (b.withTrimmedTop (40), 14); }
}

// ======================= Grafico =======================
void Graph::paint (juce::Graphics& g)
{
    auto& P = ed.proc; auto b = getLocalBounds().toFloat().reduced (1);
    jlook::card (g, b, 14, C::magenta, true, false, false);
    glowText (g, "DINAMICA", { 18, 8, 160, 20 }, C::text, uf (12.0f));
    g.setFont (mf (9.5f)); g.setColour (C::dim);
    g.drawText ("grigio = ingresso   ciano = uscita   magenta = GR A   ambra = GR B", juce::Rectangle<float> (150, 8, b.getWidth() - 360, 20), juce::Justification::centredLeft);
    auto in = b.reduced (14, 0).withTrimmedTop (32).withTrimmedBottom (14);
    auto tr = in.removeFromRight (in.getHeight()); in.removeFromRight (12);
    lcd (g, in, 6);
    const int NH = Processor::kHist, w0 = P.hW.load();
    auto at = [w0, NH] (int k) { return (w0 + k) % NH; };
    auto yLev = [&in] (float db) { return in.getBottom() - juce::jlimit (0.0f, 1.0f, (db + 48.0f) / 48.0f) * in.getHeight(); };
    auto yGr = [&in] (float gr) { return in.getY() + juce::jlimit (0.0f, 1.0f, gr / 24.0f) * in.getHeight() * 0.8f; };
    for (int db : { -6, -12, -24, -36 }) { g.setColour (C::phos.withAlpha (0.06f)); g.drawHorizontalLine ((int) yLev ((float) db), in.getX(), in.getRight()); g.setColour (C::faint); g.setFont (mf (8.5f)); g.drawText (juce::String (db), juce::Rectangle<float> (in.getX() + 3, yLev ((float) db) - 11, 30, 10), juce::Justification::centredLeft); }
    const float dx = in.getWidth() / (NH - 1);
    juce::Path pin; pin.startNewSubPath (in.getX(), in.getBottom());
    for (int k = 0; k < NH; ++k) pin.lineTo (in.getX() + k * dx, yLev (P.hIn[at (k)].load()));
    pin.lineTo (in.getRight(), in.getBottom()); pin.closeSubPath();
    g.setColour (juce::Colours::white.withAlpha (0.12f)); g.fillPath (pin);
    juce::Path pout; for (int k = 0; k < NH; ++k) { const float y = yLev (P.hOut[at (k)].load()); if (k == 0) pout.startNewSubPath (in.getX(), y); else pout.lineTo (in.getX() + k * dx, y); }
    g.setColour (C::phos.withAlpha (0.3f)); g.strokePath (pout, juce::PathStrokeType (3.5f)); g.setColour (C::phos); g.strokePath (pout, juce::PathStrokeType (1.3f));
    for (int st = 0; st < 2; ++st)
    {
        juce::Path pg; pg.startNewSubPath (in.getX(), in.getY());
        float mx = 0;
        for (int k = 0; k < NH; ++k) { const float v = (st == 0 ? P.hGrA : P.hGrB)[at (k)].load(); mx = juce::jmax (mx, v); pg.lineTo (in.getX() + k * dx, yGr (v)); }
        if (mx < 0.05f) continue;
        pg.lineTo (in.getRight(), in.getY()); pg.closeSubPath();
        const auto c = st == 0 ? C::magenta : C::amber;
        g.setColour (c.withAlpha (0.18f)); g.fillPath (pg); g.setColour (c.withAlpha (0.9f)); g.strokePath (pg, juce::PathStrokeType (1.2f));
    }
    g.setColour (C::magenta); g.fillRect (in.getRight() - 2, in.getY(), 2.0f, in.getHeight());
    scan (g, in);
    // curva di trasferimento dello stadio selezionato
    lcd (g, tr, 6);
    const auto sp = P.stageParams (ed.focus); const auto c = ed.focus == 0 ? C::phos : C::amber;
    auto tx = [&tr] (float db) { return tr.getX() + (db + 60.0f) / 60.0f * tr.getWidth(); };
    auto ty = [&tr] (float db) { return tr.getBottom() - (db + 60.0f) / 60.0f * tr.getHeight(); };
    g.setColour (C::phos.withAlpha (0.08f)); for (int d = -48; d < 0; d += 12) { g.drawVerticalLine ((int) tx ((float) d), tr.getY(), tr.getBottom()); g.drawHorizontalLine ((int) ty ((float) d), tr.getX(), tr.getRight()); }
    g.setColour (juce::Colours::white.withAlpha (0.15f)); g.drawLine (tr.getX(), tr.getBottom(), tr.getRight(), tr.getY(), 1.0f);
    const float ig = inputGainDb (sp);
    juce::Path cv;
    for (int k = 0; k <= 120; ++k) { const float x = -60.0f + k * 0.5f, l = x + ig, o = juce::jlimit (-60.0f, 0.0f, x - gainComputer (sp, l)); if (k == 0) cv.startNewSubPath (tx (x), ty (o)); else cv.lineTo (tx (x), ty (o)); }
    g.setColour (c.withAlpha (0.3f)); g.strokePath (cv, juce::PathStrokeType (4.0f)); g.setColour (c); g.strokePath (cv, juce::PathStrokeType (1.6f));
    const float lin = juce::Decibels::gainToDecibels (juce::jmax (P.inPk[0].load(), P.inPk[1].load()), -60.0f);
    if (lin > -59) { const float o = juce::jlimit (-60.0f, 0.0f, lin - gainComputer (sp, lin + ig)); g.setColour (c.withAlpha (0.3f)); g.fillEllipse (tx (lin) - 7, ty (o) - 7, 14, 14); g.setColour (juce::Colours::white); g.fillEllipse (tx (lin) - 3, ty (o) - 3, 6, 6); }
    g.setColour (C::dim); g.setFont (mf (8.5f)); g.drawText (juce::String (info (sp.model).name) + (ed.focus == 0 ? "  A" : "  B"), tr.reduced (6, 4), juce::Justification::topLeft);
    scan (g, tr);
}

// ======================= Guida =======================
void GuidePanel::mouseDown (const juce::MouseEvent& e)
{
    if (e.position.y < 40) { tab = juce::jlimit (0, 2, (int) ((e.position.x - 16) / ((getWidth() - 32) / 3.0f))); repaint(); }
}
void GuidePanel::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (1);
    jlook::card (g, b, 14, C::amber, true, false, false);
    const char* tabs[] = { "MODELLO", "TECNICA", "CONSIGLI LIVE" };
    const float tw = (b.getWidth() - 32) / 3;
    for (int i = 0; i < 3; ++i)
    {
        auto r = juce::Rectangle<float> (16 + i * tw, 10, tw - 4, 24);
        const bool sel = i == tab; jlook::button (g, r, C::amber, sel, false, tabs[i], uf (11.0f));
        if (i == 2 && warn > 0) { g.setColour (C::red); g.fillEllipse (r.getRight() - 12, r.getY() + 4, 7, 7); }
    }
    auto t = b.reduced (18, 0).withTrimmedTop (44).withTrimmedBottom (12);
    juce::AttributedString as; as.setWordWrap (juce::AttributedString::byWord); as.setLineSpacing (2.0f);
    auto head = [&as] (const juce::String& s, juce::Colour c) { as.append (s + "\n", jlook::ui (11.0f, 2), c); };
    auto body = [&as] (const juce::String& s) { as.append (s + "\n", jlook::ui (12.0f, 0), C::text.withAlpha (0.9f)); };
    if (tab == 0)
    {
        const int m = (int) ed.proc.value (ed.focus == 0 ? "a_model" : "b_model"); const auto& G = guide (m);
        head (juce::String (info (m).name) + "  -  " + info (m).inspired, juce::Colour (info (m).accent)); body (G.carattere);
        head ("PER COSA", C::dim); body (G.usi);
        head ("COME PARTIRE", C::dim); body (G.partenza);
    }
    else if (tab == 1)
    {
        head ("ROUTING", C::phos); body (routeTip ((int) ed.proc.value ("route")));
        head ("PRESET", C::phos); body (presets()[(size_t) juce::jlimit (0, (int) presets().size() - 1, ed.proc.preset)].note);
        head ("SIDECHAIN", C::phos); body ("ESTERNO: in Ableton apri il plugin, sezione Sidechain, scegli la traccia (es. la cassa). HPF: il compressore ignora i bassi sotto quella frequenza. LISTEN: ascolti cosa sente il rilevatore.");
    }
    else
    {
        if (tips.isEmpty()) body ("Fai suonare qualcosa: i consigli si aggiornano mentre ascolti.");
        for (auto& s : tips) { as.append (juce::String::fromUTF8 ("\xe2\x80\xa2 "), jlook::ui (12.0f, 2), C::amber); body (s); }
    }
    as.draw (g, t);
}

void Editor::computeTips()
{
    juce::StringArray t; int w = 0;
    const int f = focus; const auto sp = proc.stageParams (f);
    const bool stageOn = sp.on && (f == 0 || proc.value ("route") > 0.5f);
    const float gr = f == 0 ? proc.avgGrA.load() : proc.avgGrB.load();
    const float inDb = juce::Decibels::gainToDecibels (std::sqrt (proc.inRms.load()), -100.0f), outDb = juce::Decibels::gainToDecibels (std::sqrt (proc.outRms.load()), -100.0f);
    const juce::String st = f == 0 ? "A" : "B";
    if (inDb < -60) { guide.tips = {}; guide.warn = 0; return; }
    if (! stageOn) t.add ("Lo stadio " + st + " e spento" + juce::String (f == 1 && proc.value ("route") < 0.5f ? ": scegli SERIE o PARALLELO per usarlo." : "."));
    else
    {
        if (gr < 0.5f) { t.add ("Stadio " + st + ": quasi nessuna riduzione. Alza " + juce::String (info (sp.model).driveLabel) + " finche il VU scende di 2-6 dB."); ++w; }
        else if (gr > 12) { t.add ("Stadio " + st + ": oltre 12 dB di riduzione. Va bene solo in parallelo (MIX 20-40%), altrimenti stai schiacciando."); ++w; }
        else if (gr > 6) t.add ("Stadio " + st + ": riduzione forte (" + juce::String (gr, 1) + " dB). Ok per voce rap, room e smash; tanto per bus e master.");
        else t.add ("Stadio " + st + ": " + juce::String (gr, 1) + " dB di riduzione media, zona giusta per quasi tutto.");
        if (info (sp.model).relKnob && relMs (sp.model, sp.rel) > 0 && relMs (sp.model, sp.rel) < 90 && gr > 6) { t.add ("Rilascio molto corto con tanta riduzione: rischi distorsione sui bassi e pumping. Allunga il RELEASE."); ++w; }
        if (info (sp.model).attKnob && attMs (sp.model, sp.att) < 0.1f && gr > 3) t.add ("Attacco velocissimo: smussa i transienti. Per una batteria che picchia allunga l'attacco.");
        if (proc.value ("scHpf") < 25 && gr > 3 && (sp.model == VCABUS || sp.model == VARIMU)) t.add ("Su bus e master alza l'HPF del sidechain a 80-150 Hz: il compressore smette di reagire alla cassa e non pompa.");
    }
    const float d = outDb - inDb;
    if (d > 2) { t.add ("L'uscita e piu forte dell'ingresso di " + juce::String (d, 1) + " dB: il piu forte sembra sempre meglio. Attiva AUTO GAIN o abbassa OUTPUT per un confronto onesto."); ++w; }
    else if (d < -3) t.add ("L'uscita e piu bassa di " + juce::String (-d, 1) + " dB: recupera con OUTPUT o AUTO GAIN.");
    if (proc.value ("scSrc") > 0.5f && ! proc.hasExtSidechain()) { t.add ("Sidechain ESTERNO selezionato ma nessuna traccia collegata: in Ableton scegli la sorgente nella sezione Sidechain del plugin."); ++w; }
    if (proc.value ("route") > 0.5f && proc.avgGrA.load() > 6 && proc.avgGrB.load() > 6) { t.add ("Entrambi gli stadi lavorano tanto: in serie suona meglio con 2-4 dB ciascuno."); ++w; }
    if (juce::jmax (proc.outPk[0].load(), proc.outPk[1].load()) > 0.99f) { t.add ("L'uscita tocca 0 dBFS: abbassa OUTPUT."); ++w; }
    guide.tips = t; guide.warn = w;
}

// ======================= Editor =======================
Editor::Editor (Processor& p) : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (a); addAndMakeVisible (b); addAndMakeVisible (graph); addAndMakeVisible (guide);
    route = std::make_unique<Segs> (P ("route"), C::magenta); route->items = [] { return juce::StringArray { "SINGOLO", "SERIE", "PARALLELO" }; };
    route->onTouch = [this] { a.repaint(); b.repaint(); repaint(); };
    scSrc = std::make_unique<Segs> (P ("scSrc"), C::green); scSrc->items = [] { return juce::StringArray { "INTERNO", "ESTERNO" }; };
    scTo = std::make_unique<Segs> (P ("scTo"), C::green); scTo->items = [] { return juce::StringArray { "A + B", "SOLO A", "SOLO B" }; };
    scHpf = std::make_unique<Knob> (P ("scHpf"), C::green); scHpf->label = "SC HPF";
    scHpf->text = [this] { const float f = proc.value ("scHpf"); return f < 21 ? juce::String ("OFF") : juce::String (juce::roundToInt (f)) + " Hz"; };
    listen = std::make_unique<Switch> (P ("scListen"), C::green, "LISTEN");
    autoG = std::make_unique<Switch> (P ("auto"), C::phos, "AUTO GAIN");
    gMix = std::make_unique<Knob> (P ("mix"), C::phos); gMix->label = "MIX"; gMix->text = [this] { return juce::String (juce::roundToInt (proc.value ("mix") * 100)) + "%"; };
    gOut = std::make_unique<Knob> (P ("out"), C::phos); gOut->label = "OUTPUT"; gOut->bipolar = true; gOut->text = [this] { const float v = proc.value ("out"); return (v > 0 ? "+" : "") + juce::String (v, 1) + " dB"; };
    for (juce::Component* c : std::initializer_list<juce::Component*> { route.get(), scSrc.get(), scTo.get(), scHpf.get(), listen.get(), autoG.get(), gMix.get(), gOut.get() }) addAndMakeVisible (c);
    setSize (W, H);
    startTimerHz (30);
}

void Editor::resized()
{
    presetRect = { 470, 15, 330, 36 }; prevRect = { 432, 15, 36, 36 }; nextRect = { 802, 15, 36, 36 }; abRect = { 850, 15, 110, 36 };
    a.setBounds (18, 80, 392, 452); b.setBounds (418, 80, 392, 452);
    const int rx = 818, rw = 404;
    route->setBounds (rx + 16, 112, rw - 32, 30);
    scSrc->setBounds (rx + 16, 236, 180, 28); scTo->setBounds (rx + 16, 272, 180, 28);
    scHpf->setBounds (rx + 224, 222, 72, 72 + 27); listen->setBounds (rx + 306, 250, 90, 22);
    autoG->setBounds (rx + 16, 400, 130, 22);
    gMix->setBounds (rx + 168, 384, 72, 72 + 27); gOut->setBounds (rx + 250, 384, 72, 72 + 27);
    graph.setBounds (18, 546, 792, 230); guide.setBounds (818, 546, 404, 230);
}

void Editor::paint (juce::Graphics& g)
{
    const float Wf = (float) W, Hf = (float) H;
    jlook::backdrop (g, { 0, 0, Wf, Hf }, C::magenta, C::phos);
    jlook::header (g, { 0, 0, Wf, 64 }, "STUDIO COMP", C::phos, "JC-2  DUAL COMPRESSOR", C::magenta, C::phos);
    for (auto* c : { (juce::Component*) &a, (juce::Component*) &b, (juce::Component*) &graph, (juce::Component*) &guide }) jlook::softShadow (g, c->getBounds().toFloat(), 14);
    // preset
    lcd (g, presetRect.toFloat(), 8); jlook::glass (g, presetRect.toFloat(), 8);
    glowText (g, proc.getProgramName (proc.preset).toUpperCase(), presetRect.toFloat(), C::amber, mf (12.5f), juce::Justification::centred);
    g.setColour (C::phos); g.setFont (uf (20.0f));
    g.drawText (juce::String::fromUTF8 ("\xe2\x80\xb9"), prevRect, juce::Justification::centred); g.drawText (juce::String::fromUTF8 ("\xe2\x80\xba"), nextRect, juce::Justification::centred);
    jlook::button (g, abRect.toFloat(), C::phos, proc.ab.cur == 1, abHover, "CONFRONTO " + proc.ab.label(), uf (11.5f));
    for (int i = 0; i < 9; ++i) { auto r = juce::Rectangle<float> (Wf - 120 + i * 11.0f, 18, 5, 26); g.setColour (juce::Colours::black.withAlpha (0.8f)); g.fillRoundedRectangle (r, 2.5f); }
    // colonna destra
    const float rx = 818, rw = 404;
    auto cardR = [&] (juce::Rectangle<float> r, juce::Colour c, const char* t) { jlook::softShadow (g, r, 14); jlook::card (g, r, 14, c, true, false, false); glowText (g, t, { r.getX() + 16, r.getY() + 6, 200, 22 }, C::text, uf (12.0f)); };
    cardR ({ rx, 80, rw, 112 }, C::magenta, "ROUTING");
    { // schema del routing
        const int rt = (int) proc.value ("route"); auto d = juce::Rectangle<float> (rx + 16, 150, rw - 32, 32);
        auto box = [&] (float x, float y, const char* t, juce::Colour c, bool on) { auto r = juce::Rectangle<float> (x, y, 40, 18); g.setColour (c.withAlpha (on ? 0.25f : 0.05f)); g.fillRoundedRectangle (r, 4); g.setColour (c.withAlpha (on ? 0.9f : 0.25f)); g.drawRoundedRectangle (r, 4, 1.0f); g.setFont (uf (11.0f)); g.drawText (t, r, juce::Justification::centred); };
        g.setColour (C::dim); g.setFont (mf (9.5f)); g.drawText ("IN", juce::Rectangle<float> (d.getX(), d.getY(), 30, 32), juce::Justification::centredLeft); g.drawText ("OUT", juce::Rectangle<float> (d.getRight() - 30, d.getY(), 30, 32), juce::Justification::centredRight);
        const float x0 = d.getX() + 34, x1 = d.getRight() - 34, cy = d.getCentreY();
        g.setColour (C::dim.withAlpha (0.6f));
        if (rt == 2)
        {
            const float xm = (x0 + x1) / 2;
            g.drawLine (x0, cy, xm - 50, cy - 8); g.drawLine (x0, cy, xm - 50, cy + 8); g.drawLine (xm + 50, cy - 8, x1, cy); g.drawLine (xm + 50, cy + 8, x1, cy);
            g.drawLine (xm - 50, cy - 8, xm + 50, cy - 8); g.drawLine (xm - 50, cy + 8, xm + 50, cy + 8);
            box (xm - 20, cy - 17, "A", C::phos, true); box (xm - 20, cy - 1, "B", C::amber, true);
        }
        else
        {
            g.drawLine (x0, cy, x1, cy);
            if (rt == 0) box ((x0 + x1) / 2 - 20, cy - 9, "A", C::phos, true);
            else { box ((x0 + x1) / 2 - 60, cy - 9, "A", C::phos, true); box ((x0 + x1) / 2 + 20, cy - 9, "B", C::amber, true); }
        }
    }
    cardR ({ rx, 204, rw, 136 }, C::green, "SIDECHAIN");
    g.setFont (mf (9.5f)); g.setColour (proc.hasExtSidechain() ? C::green : C::dim);
    g.drawText (proc.hasExtSidechain() ? "esterno collegato" : "esterno non collegato", juce::Rectangle<float> (rx + 200, 210, rw - 216, 18), juce::Justification::centredRight);
    cardR ({ rx, 352, rw, 180 }, C::phos, "USCITA");
    // meter in/out
    auto meter = [&] (juce::Rectangle<float> r, float l, float rr, const char* t)
    {
        g.setColour (C::dim); g.setFont (mf (8.5f)); g.drawText (t, r.removeFromBottom (12), juce::Justification::centred);
        lcd (g, r, 3);
        for (int c = 0; c < 2; ++c)
        {
            auto m = juce::Rectangle<float> (r.getX() + 3 + c * (r.getWidth() / 2 - 1), r.getY() + 3, r.getWidth() / 2 - 5, r.getHeight() - 6);
            const float db = juce::Decibels::gainToDecibels (c ? rr : l, -60.0f), n = juce::jlimit (0.0f, 1.0f, (db + 60) / 63);
            const int seg = 24;
            for (int k = 0; k < seg; ++k) { const float t2 = k / (float) seg; g.setColour (t2 <= n ? (t2 > 0.93f ? C::red : t2 > 0.8f ? C::amber : C::phos) : juce::Colours::white.withAlpha (0.05f)); g.fillRect (m.getX(), m.getBottom() - (k + 1) * m.getHeight() / seg + 1, m.getWidth(), m.getHeight() / seg - 1.5f); }
        }
    };
    meter ({ rx + 336, 386, 24, 136 }, proc.inPk[0].load(), proc.inPk[1].load(), "IN");
    meter ({ rx + 366, 386, 24, 136 }, proc.outPk[0].load(), proc.outPk[1].load(), "OUT");
    g.setColour (C::dim); g.setFont (uf (10.0f, false));
    g.drawFittedText ("AUTO GAIN compensa la riduzione: confronta a volume pari.", juce::Rectangle<int> ((int) rx + 16, 432, 140, 60), juce::Justification::topLeft, 4);
}

void Editor::mouseMove (const juce::MouseEvent& e)
{
    const bool h = abRect.contains (e.getPosition());
    if (h != abHover) { abHover = h; repaint (abRect.expanded (8)); }
    setMouseCursor (h || presetRect.contains (e.getPosition()) || prevRect.contains (e.getPosition()) || nextRect.contains (e.getPosition()) ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
}

void Editor::mouseDown (const juce::MouseEvent& e)
{
    const int n = (int) presets().size();
    auto after = [this] { a.refresh(); b.refresh(); repaint(); };
    if (abRect.contains (e.getPosition())) { proc.ab.toggle (proc); after(); }
    else if (prevRect.contains (e.getPosition())) { proc.loadPreset ((proc.preset + n - 1) % n); after(); }
    else if (nextRect.contains (e.getPosition())) { proc.loadPreset ((proc.preset + 1) % n); after(); }
    else if (presetRect.contains (e.getPosition()))
    {
        juce::PopupMenu m; std::vector<std::pair<juce::String, juce::PopupMenu>> groups;
        for (int i = 0; i < n; ++i)
        {
            const juce::String name = presets()[(size_t) i].name;
            if (! name.contains (" | ")) { m.addItem (i + 1, name, true, i == proc.preset); continue; }
            const auto gn = name.upToFirstOccurrenceOf (" | ", false, false), it = name.fromFirstOccurrenceOf (" | ", false, false);
            bool found = false; for (auto& gr : groups) if (gr.first == gn) { gr.second.addItem (i + 1, it, true, i == proc.preset); found = true; }
            if (! found) { juce::PopupMenu pm; pm.addItem (i + 1, it, true, i == proc.preset); groups.push_back ({ gn, pm }); }
        }
        for (auto& gr : groups) m.addSubMenu (gr.first, gr.second);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withTargetScreenArea (localAreaToGlobal (presetRect)), [this, after] (int r) { if (r > 0) { proc.loadPreset (r - 1); after(); } });
    }
}

void Editor::timerCallback()
{
    a.needle += (proc.grA.load() - a.needle) * (proc.grA.load() > a.needle ? 0.5f : 0.18f);
    b.needle += (proc.grB.load() - b.needle) * (proc.grB.load() > b.needle ? 0.5f : 0.18f);
    a.repaint (a.vuRect().toNearestInt().expanded (4)); b.repaint (b.vuRect().toNearestInt().expanded (4));
    for (auto* k : { a.drive.get(), a.att.get(), a.rel.get(), a.out.get(), a.color.get(), a.mix.get(), b.drive.get(), b.att.get(), b.rel.get(), b.out.get(), b.color.get(), b.mix.get(), scHpf.get(), gMix.get(), gOut.get() }) k->repaint();
    graph.repaint();
    repaint (818, 352, 404, 180);
    if (++tick % 15 == 0) { computeTips(); guide.repaint(); }
}
}
