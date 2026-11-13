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
    skin::screen (g, r, rad, C::phos);
    g.setColour (C::phos.withAlpha (0.03f));
    for (float x = r.getX() + 10; x < r.getRight(); x += 12) g.drawVerticalLine ((int) x, r.getY() + 2, r.getBottom() - 2);
    for (float y = r.getY() + 10; y < r.getBottom(); y += 12) g.drawHorizontalLine ((int) y, r.getX() + 2, r.getRight() - 2);
}
static void scan (juce::Graphics& g, juce::Rectangle<float> r)
{
    g.setColour (juce::Colours::black.withAlpha (neo::light() ? 0.03f : 0.18f)); for (float y = r.getY(); y < r.getBottom(); y += 3) g.drawHorizontalLine ((int) y, r.getX(), r.getRight());
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
    const float v = param->getValue();
    const auto c = enabled ? col : C::faint;
    jlook::KnobLook kl; kl.big = big; kl.bipolar = bipolar; kl.enabled = enabled; kl.hover = hover ? 1.0f : 0.0f; kl.active = drag ? 1.0f : 0.0f;
    jlook::drawKnob (g, { (getWidth() - size) * 0.5f, 0, size, size }, v, col, kl);
    g.setColour (C::dim); g.setFont (uf (big ? 10.5f : 9.5f)); g.drawText (label, juce::Rectangle<float> (0, size + 1, (float) getWidth(), 13), juce::Justification::centred);
    glowText (g, text ? text() : param->getCurrentValueAsText(), juce::Rectangle<float> (0, size + 14, (float) getWidth(), 13), enabled ? (drag ? c.brighter (0.4f) : c) : C::faint, mf (big ? 11.5f : 10.5f), juce::Justification::centred);
}

// ======================= Segs / Switch =======================
void Segs::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat(); lcd (g, b, 6);
    const auto it = items(); const int n = juce::jmax (1, it.size()), val = juce::roundToInt (param->convertFrom0to1 (param->getValue()));
    int cur = val; if (map) for (int k = 0; k < n; ++k) if (map (k) == val) cur = k;
    const float w = (b.getWidth() - 6) / n;
    for (int i = 0; i < n; ++i)
    {
        auto r = juce::Rectangle<float> (3 + i * w, 3, w, b.getHeight() - 6).reduced (1, 0);
        if (i == cur) { g.setColour (col.withAlpha (0.18f)); g.fillRoundedRectangle (r, 4); g.setColour (col.withAlpha (0.85f)); g.drawRoundedRectangle (r, 4, 1.0f); glowText (g, it[i], r, col, uf (11.0f), juce::Justification::centred); }
        else { g.setColour (C::dim); g.setFont (uf (11.0f)); g.drawText (it[i], r, juce::Justification::centred); }
    }
    g.setColour (juce::Colours::black.withAlpha (neo::light() ? 0.03f : 0.15f)); for (float y = b.getY() + 2; y < b.getBottom(); y += 3) g.drawHorizontalLine ((int) y, b.getX() + 2, b.getRight() - 2);
}
void Segs::mouseDown (const juce::MouseEvent& e)
{
    const int n = juce::jmax (1, items().size()); const int k = juce::jlimit (0, n - 1, (int) ((e.position.x - 3) / ((getWidth() - 6) / (float) n)));
    param->setValueNotifyingHost (param->convertTo0to1 ((float) (map ? map (k) : k))); if (onTouch) onTouch(); repaint();
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

// ======================= Rack degli stadi =======================
void Rack::mouseMove (const juce::MouseEvent& e)
{
    int h = -1; for (int s = 0; s < kStages; ++s) if (tile (s).contains (e.position)) h = s;
    if (h != hov) { hov = h; repaint(); }
    setMouseCursor (h >= 0 ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
}
void Rack::mouseDown (const juce::MouseEvent& e)
{
    for (int s = 0; s < kStages; ++s)
    {
        if (! tile (s).contains (e.position)) continue;
        auto* on = ed.P (juce::String (stagePfx (s)) + "on");
        const bool isOn = on->getValue() > 0.5f;
        if (powR (s).expanded (6).contains (e.position)) on->setValueNotifyingHost (isOn ? 0.0f : 1.0f);
        else if (isOn && soloR (s).contains (e.position)) { auto* so = ed.P (juce::String (stagePfx (s)) + "solo"); so->setValueNotifyingHost (so->getValue() > 0.5f ? 0.0f : 1.0f); }
        else if (! isOn) on->setValueNotifyingHost (1.0f);
        ed.select (s); repaint(); return;
    }
}
void Rack::paint (juce::Graphics& g)
{
    for (int s = 0; s < kStages; ++s)
    {
        auto t = tile (s); const bool on = ed.sv (s, "on") > 0.5f, sel = ed.focus == s, solo = on && ed.sv (s, "solo") > 0.5f;
        const int m = (int) ed.sv (s, "model"); const auto c = C::stage[s];
        jlook::softShadow (g, t, 12, sel ? 20.0f : 12.0f, sel ? 0.7f : 0.5f, sel ? 8.0f : 4.0f);
        jlook::card (g, t, 12, c, on, sel, false);
        if (sel) { g.setColour (c.withAlpha (0.85f)); g.drawRoundedRectangle (t.reduced (1), 11, 1.6f); }
        else if (hov == s) { g.setColour (neo::K::hi.withAlpha (0.18f)); g.drawRoundedRectangle (t.reduced (1), 11, 1.0f); }
        // accensione
        auto pr = powR (s);
        if (on) { g.setColour (c.withAlpha (0.35f)); g.fillEllipse (pr.expanded (4)); }
        juce::ColourGradient lg (on ? c.brighter (0.6f) : juce::Colour (0xff3a3e48), pr.getCentreX(), pr.getY(), on ? c : juce::Colour (0xff15171c), pr.getCentreX(), pr.getBottom(), false);
        g.setGradientFill (lg); g.fillEllipse (pr);
        glowText (g, juce::String (s + 1), { pr.getRight() + 6, pr.getY() - 2, 20, 22 }, on ? C::text : C::dim, mf (13.0f));
        if (! on)
        {
            g.setColour (C::dim.withAlpha (hov == s ? 0.9f : 0.5f)); g.setFont (uf (12.0f));
            g.drawText ("+  AGGIUNGI", t.withTrimmedTop (30), juce::Justification::centred);
            continue;
        }
        glowText (g, info (m).name, { t.getX() + 10, t.getY() + 30, t.getWidth() - 20, 20 }, neo::acc (juce::Colour (info (m).accent)), uf (13.5f));
        // misuratore di riduzione orizzontale
        meter[s] += (ed.proc.gr[s].load() - meter[s]) * (ed.proc.gr[s].load() > meter[s] ? 0.6f : 0.2f);
        auto mb = juce::Rectangle<float> (t.getX() + 10, t.getY() + 56, t.getWidth() - 20, 8);
        g.setColour (juce::Colours::black.withAlpha (0.6f)); g.fillRoundedRectangle (mb, 4);
        const float w = juce::jlimit (0.0f, 1.0f, meter[s] / 20.0f) * mb.getWidth();
        if (w > 0.5f) { juce::ColourGradient mg (c, mb.getRight() - w, 0, c.brighter (0.6f), mb.getRight(), 0, false); g.setGradientFill (mg); g.fillRoundedRectangle (mb.withLeft (mb.getRight() - w), 4); }
        g.setColour (C::dim); g.setFont (mf (9.5f)); g.drawText ("GR " + juce::String (meter[s], 1), juce::Rectangle<float> (t.getX() + 10, t.getBottom() - 30, 70, 22), juce::Justification::centredLeft);
        jlook::button (g, soloR (s), C::amber, solo, false, "SOLO", uf (10.0f));
    }
}

// ======================= Editor dello stadio =======================
StageEditor::StageEditor (Editor& e) : ed (e)
{
    auto touch = [] {};
    power = std::make_unique<Switch> (ed.P ("a_on"), C::stage[0]);
    ratio = std::make_unique<Segs> (ed.P ("a_ratio"), C::stage[0]);
    ratio->items = [this] { return info ((int) ed.sv (s, "model")).ratios; };
    auto mk = [&] (const char* id, const char* lab) { auto k = std::make_unique<Knob> (ed.P (juce::String ("a_") + id), C::stage[0]); k->label = lab; k->onTouch = touch; addAndMakeVisible (*k); return k; };
    drive = mk ("drive", "INPUT"); drive->big = true;
    att = mk ("att", "ATTACK"); rel = mk ("rel", "RELEASE"); out = mk ("out", "OUTPUT"); color = mk ("color", "COLOR"); mix = mk ("mix", "MIX");
    drive->text = [this] { return fmtDrive ((int) ed.sv (s, "model"), ed.sv (s, "drive")); };
    att->text = [this] { const int m = (int) ed.sv (s, "model"); if (m == VC670) return juce::String (t670Thr (ed.sv (s, "att")), 1) + " dB"; return info (m).attKnob ? fmtTime (attMs (m, ed.sv (s, "att"))) : juce::String (m <= OPTO3A ? "OTTICO" : m == VARIMU ? "TC" : "AUTO"); };
    rel->text = [this] { const int m = (int) ed.sv (s, "model"); if (m == VC670) return juce::String (ed.sv (s, "rel") > 0.5f ? "LAT/VERT" : "L/R"); return info (m).relKnob ? fmtTime (relMs (m, ed.sv (s, "rel"))) : juce::String (m <= OPTO3A ? "OTTICO" : m == VARIMU ? "TC" : "120 dB/s"); };
    out->text = [this] { const float v = ed.sv (s, "out"); return (v > 0 ? "+" : "") + juce::String (v, 1) + " dB"; };
    color->text = [this] { return juce::String (juce::roundToInt (ed.sv (s, "color") * 100)) + "%"; };
    mix->text = [this] { return juce::String (juce::roundToInt (ed.sv (s, "mix") * 100)) + "%"; };
    auto mkb = [&] (const char* lab) { auto k = std::make_unique<Knob> (ed.P ("mb_b1_thr"), C::stage[0]); k->label = lab; addAndMakeVisible (*k); return k; };
    bThr = mkb ("SOGLIA"); bRatio = mkb ("RATIO"); bAtt = mkb ("ATTACCO"); bRel = mkb ("RILASCIO"); bGain = mkb ("GUADAGNO"); bGain->bipolar = true;
    auto bp = [this] (const char* k) { return ed.proc.value (("mb_b" + juce::String (mbSel + 1) + "_" + k).toRawUTF8()); };
    bThr->text = [bp] { return juce::String (bp ("thr"), 1) + " dB"; };
    bRatio->text = [bp] { return juce::String (bp ("ratio"), 1) + ":1"; };
    bAtt->text = [bp] { return fmtTime (bp ("att")); };
    bRel->text = [bp] { return fmtTime (bp ("rel")); };
    bGain->text = [bp] { const float v = bp ("gain"); return (v > 0 ? "+" : "") + juce::String (v, 1) + " dB"; };
    addAndMakeVisible (*power); addAndMakeVisible (*ratio);
    bind (0);
}
bool StageEditor::isMb() const { return (int) ed.sv (s, "model") == MULTI; }
void StageEditor::bindBand (int b)
{
    mbSel = juce::jlimit (0, 3, b); const juce::String p = "mb_b" + juce::String (mbSel + 1) + "_";
    bThr->param = ed.P (p + "thr"); bRatio->param = ed.P (p + "ratio"); bAtt->param = ed.P (p + "att"); bRel->param = ed.P (p + "rel"); bGain->param = ed.P (p + "gain");
    repaint();
}
void StageEditor::mbBounds (float* e, int& nb) const
{
    nb = 2 + juce::jlimit (0, 2, (int) ed.sv (s, "ratio"));
    const float x1 = ed.proc.value ("mb_x1"), x2 = ed.proc.value ("mb_x2"), x3 = ed.proc.value ("mb_x3");
    e[0] = 20;
    if (nb == 2) { e[1] = x2; e[2] = 20000; }
    else if (nb == 3) { e[1] = x1; e[2] = x3; e[3] = 20000; }
    else { e[1] = x1; e[2] = x2; e[3] = x3; e[4] = 20000; }
}
void StageEditor::layoutKnobs()
{
    const bool mb = isMb();
    for (auto* k : { bThr.get(), bRatio.get(), bAtt.get(), bRel.get(), bGain.get() }) k->setVisible (mb);
    out->setVisible (! mb); att->setVisible (! mb); rel->setVisible (! mb);
    const int x0 = 370;
    if (mb)
    {
        const int cw = (getWidth() - x0 - 14) / 4;
        Knob* r1[4] = { bThr.get(), bRatio.get(), bAtt.get(), bRel.get() }; Knob* r2[4] = { bGain.get(), drive.get(), color.get(), mix.get() };
        for (int i = 0; i < 4; ++i) { r1[i]->setBounds (x0 + i * cw + (cw - 82) / 2, 52, 82, 82 + 27); r2[i]->setBounds (x0 + i * cw + (cw - 76) / 2, 180, 76, 76 + 27); }
        drive->big = false;
        return;
    }
    drive->big = true;
    const int cw = (getWidth() - x0 - 14) / 3;
    drive->setBounds (x0 + (cw - 104) / 2, 44, 104, 104 + 30);
    att->setBounds (x0 + cw + (cw - 84) / 2, 52, 84, 84 + 27); rel->setBounds (x0 + cw * 2 + (cw - 84) / 2, 52, 84, 84 + 27);
    out->setBounds (x0 + (cw - 76) / 2, 186, 76, 76 + 27); color->setBounds (x0 + cw + (cw - 76) / 2, 186, 76, 76 + 27); mix->setBounds (x0 + cw * 2 + (cw - 76) / 2, 186, 76, 76 + 27);
}
void StageEditor::mouseDrag (const juce::MouseEvent& e)
{
    if (xDrag < 0) return;
    static const char* ids[3] = { "mb_x1", "mb_x2", "mb_x3" };
    auto* pr = ed.P (ids[xDrag]); pr->setValueNotifyingHost (pr->convertTo0to1 (hzOf (e.position.x)));
    repaint();
}
void StageEditor::bind (int st)
{
    s = st; pfx = stagePfx (s); const auto c = C::stage[s];
    power->param = ed.P (pfx + "on"); power->col = c; ratio->param = ed.P (pfx + "ratio"); ratio->col = c;
    for (auto* k : { drive.get(), att.get(), rel.get(), out.get(), color.get(), mix.get() }) k->col = c;
    drive->param = ed.P (pfx + "drive"); att->param = ed.P (pfx + "att"); rel->param = ed.P (pfx + "rel");
    out->param = ed.P (pfx + "out"); color->param = ed.P (pfx + "color"); mix->param = ed.P (pfx + "mix");
    const int m = (int) ed.sv (s, "model"); const auto& I = info (m);
    drive->label = I.driveLabel; att->enabled = I.attKnob; rel->enabled = I.relKnob;
    att->stepped = rel->stepped = m == VCABUS || m == VC670; att->steps = 6; rel->steps = 5;
    if (m == VC670) { att->stepped = false; rel->steps = 2; }
    att->label = m == VC670 ? "THRESHOLD" : "ATTACK"; rel->label = m == VC670 ? "STEREO" : "RELEASE";
    for (auto* k : { bThr.get(), bRatio.get(), bAtt.get(), bRel.get(), bGain.get() }) k->col = c;
    drive->label = m == MULTI ? "SOGLIE +/-" : I.driveLabel;
    if (m == MULTI) { color->label = "WARM"; bindBand (mbSel); }
    layoutKnobs();
    color->label = m == MULTI ? "WARM" : m == DISTRESS ? (ed.sv (s, "color") < 0.5f ? "DIST 2" : "DIST 3") : m == FET76 ? "GRINTA" : (m <= OPTO3A || m == VARIMU) ? "VALVOLA" : "COLOR";
    lastModel = m; repaint();
}
void StageEditor::resized()
{
    power->setBounds (16, 13, 38, 22);
    ratio->setBounds (16, 236, 330, 30);
    layoutKnobs();
}
void StageEditor::mouseDown (const juce::MouseEvent& e)
{
    if (isMb() && vuRect().contains (e.position))
    {
        float edg[5]; int nb; mbBounds (edg, nb);
        // tagli trascinabili
        static const int xid2[1] = { 1 }, xid3[2] = { 0, 2 }, xid4[3] = { 0, 1, 2 };
        const int* xid = nb == 2 ? xid2 : nb == 3 ? xid3 : xid4;
        for (int k = 1; k < nb; ++k) if (std::abs (e.position.x - xOf (edg[k])) < 6) { xDrag = xid[k - 1]; return; }
        for (int b = 0; b < nb; ++b)
        {
            const float a = xOf (edg[b]), z = xOf (edg[b + 1]); if (e.position.x < a || e.position.x >= z) continue;
            auto r = vuRect(); const float by = r.getBottom() - 22;
            if (e.position.y > by)
            {
                const juce::String p = "mb_b" + juce::String (b + 1) + "_";
                auto* t = ed.P (p + (e.position.x < (a + z) / 2 ? "solo" : "byp")); t->setValueNotifyingHost (t->getValue() > 0.5f ? 0.0f : 1.0f);
            }
            bindBand (b); return;
        }
        return;
    }
    if (soloRect().contains (e.getPosition())) { auto* so = ed.P (pfx + "solo"); so->setValueNotifyingHost (so->getValue() > 0.5f ? 0.0f : 1.0f); repaint(); ed.rack.repaint(); return; }
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
            bind (s); ed.rack.repaint();
        });
    }
}
void StageEditor::paint (juce::Graphics& g)
{
    const int m = (int) ed.sv (s, "model"); if (m != lastModel) bind (s);
    const auto& I = info (m); const bool on = ed.sv (s, "on") > 0.5f; const auto c = C::stage[s];
    auto b = getLocalBounds().toFloat().reduced (1);
    jlook::card (g, b, 14, neo::acc (juce::Colour (I.accent)), on, false, false);
    glowText (g, "STADIO " + juce::String (s + 1), { 62, 10, 110, 28 }, on ? C::text : C::dim, uf (14.0f));
    g.setColour (C::dim); g.setFont (uf (11.0f, false)); g.drawText (I.inspired, juce::Rectangle<float> (170, 10, b.getWidth() - 470, 28), juce::Justification::centredLeft);
    jlook::button (g, modelRect().toFloat(), neo::acc (juce::Colour (I.accent)), true, false, juce::String (I.name) + juce::String::fromUTF8 ("  \xe2\x96\xbe"), uf (13.0f));
    jlook::button (g, soloRect().toFloat(), C::amber, ed.sv (s, "solo") > 0.5f, false, "SOLO", uf (11.5f));
    if (m == MULTI)
    {
        auto r = vuRect(); lcd (g, r, 8);
        float edg[5]; int nb; mbBounds (edg, nb);
        for (float hz : { 100.0f, 1000.0f, 10000.0f }) { g.setColour (C::phos.withAlpha (0.1f)); g.drawVerticalLine ((int) xOf (hz), r.getY(), r.getBottom()); g.setColour (C::faint); g.setFont (mf (8.5f)); g.drawText (hz >= 1000 ? juce::String ((int) hz / 1000) + "k" : juce::String ((int) hz), juce::Rectangle<float> (xOf (hz) + 2, r.getY() + 2, 30, 10), juce::Justification::centredLeft); }
        static const juce::uint32 bc[4] = { 0xffff7a3d, 0xffffd23f, 0xff7dff5a, 0xff5ef2ff };
        for (int b = 0; b < nb; ++b)
        {
            const float a = xOf (edg[b]), z = xOf (edg[b + 1]); const auto col = juce::Colour (bc[b]); const bool sel = b == mbSel;
            const juce::String p = "mb_b" + juce::String (b + 1) + "_";
            const bool so = ed.proc.value ((p + "solo").toRawUTF8()) > 0.5f, by = ed.proc.value ((p + "byp").toRawUTF8()) > 0.5f;
            auto br = juce::Rectangle<float> (a + 1, r.getY() + 14, z - a - 2, r.getHeight() - 38);
            g.setColour (col.withAlpha (sel ? 0.16f : 0.06f)); g.fillRect (br);
            const float grv = ed.proc.mbGr[b].load(), h = juce::jlimit (0.0f, 1.0f, grv / 18.0f) * br.getHeight();
            if (! by) { g.setColour (col.withAlpha (0.75f)); g.fillRect (br.withHeight (h).reduced (br.getWidth() * 0.25f, 0)); }
            g.setColour (sel ? col : col.withAlpha (0.6f)); g.setFont (mf (9.0f));
            g.drawText (by ? juce::String ("BYPASS") : "-" + juce::String (grv, 1), br.withTrimmedTop (br.getHeight() - 14), juce::Justification::centred);
            g.drawText (juce::String (ed.proc.value ((p + "thr").toRawUTF8()), 0) + " / " + juce::String (ed.proc.value ((p + "ratio").toRawUTF8()), 1), br.withHeight (12).translated (0, h > 14 ? h - 13 : 2), juce::Justification::centred);
            auto bb = juce::Rectangle<float> (a + 2, r.getBottom() - 21, (z - a - 4) / 2 - 1, 17);
            jlook::button (g, bb, C::amber, so, false, "S", uf (9.5f)); jlook::button (g, bb.translated (bb.getWidth() + 2, 0), C::red, by, false, "B", uf (9.5f));
            if (sel) { g.setColour (col); g.drawRect (br, 1.5f); }
        }
        for (int k = 1; k < nb; ++k) { const float x = xOf (edg[k]); g.setColour (C::text.withAlpha (0.8f)); g.fillRect (x - 1, r.getY(), 2.0f, r.getHeight() - 24); g.setFont (mf (8.5f)); g.drawText (edg[k] >= 1000 ? juce::String (edg[k] / 1000, 1) + "k" : juce::String ((int) edg[k]), juce::Rectangle<float> (x - 20, r.getY() + 2, 40, 10), juce::Justification::centred); }
        g.setColour (C::dim); g.setFont (uf (10.0f)); g.drawText ("BANDA " + juce::String (mbSel + 1) + ": clic per scegliere, trascina i tagli, S solo, B bypass", juce::Rectangle<float> (16, 202, 340, 12), juce::Justification::centredLeft);
    }
    else
    {
    // VU analogico
    auto vu = vuRect();
    jlook::softShadow (g, vu, 8, 12, 0.65f, 4);
    g.setColour (juce::Colour (0xff0a0b0e)); g.fillRoundedRectangle (vu.expanded (5), 11);   // cornice
    { juce::ColourGradient fr (juce::Colour (0xff3a3e48), 0, vu.getY() - 5, juce::Colour (0xff121419), 0, vu.getBottom() + 5, false); g.setGradientFill (fr); g.drawRoundedRectangle (vu.expanded (4), 10, 2.0f); }
    const auto face = juce::Colour (I.face); const bool light = face.getPerceivedBrightness() > 0.5f;
    juce::ColourGradient fg (face.brighter (0.15f), vu.getCentreX(), vu.getY(), face.darker (0.25f), vu.getCentreX(), vu.getBottom(), false); g.setGradientFill (fg); g.fillRoundedRectangle (vu, 8);
    juce::ColourGradient lamp (juce::Colour (0xffffc46b).withAlpha (light ? 0.38f : 0.14f), vu.getCentreX(), vu.getBottom(), juce::Colours::transparentBlack, vu.getCentreX(), vu.getY(), true); g.setGradientFill (lamp); g.fillRoundedRectangle (vu, 8);
    const auto ink = light ? juce::Colour (0xff1b1712) : juce::Colour (0xffe9e4d8);
    const float pcx = vu.getCentreX(), pcy = vu.getBottom() + 44, rad = vu.getHeight() + 10;
    auto angOf = [] (float grDb) { const float t = 1.0f - std::sqrt (juce::jlimit (0.0f, 20.0f, grDb) / 20.0f); return -0.72f + t * 1.44f; };
    juce::Path arc; arc.addCentredArc (pcx, pcy, rad - 22, rad - 22, 0, angOf (20), angOf (0), true);
    g.setColour (ink.withAlpha (0.85f)); g.strokePath (arc, juce::PathStrokeType (1.3f));
    juce::Path red; red.addCentredArc (pcx, pcy, rad - 20, rad - 20, 0, angOf (20), angOf (10), true); g.setColour (juce::Colour (0xffd22b2b).withAlpha (0.85f)); g.strokePath (red, juce::PathStrokeType (3.5f));
    for (int db : { 0, 1, 2, 3, 5, 7, 10, 15, 20 })
    {
        const float a = angOf ((float) db), sx = std::sin (a), cy2 = -std::cos (a);
        g.setColour (ink); g.drawLine (pcx + sx * (rad - 22), pcy + cy2 * (rad - 22), pcx + sx * (rad - 31), pcy + cy2 * (rad - 31), 1.4f);
        if (db == 1 || db == 2 || db == 7 || db == 15) continue;
        g.setFont (mf (10.0f)); g.drawText (juce::String (db == 0 ? 0 : -db), juce::Rectangle<float> (pcx + sx * (rad - 9) - 13, pcy + cy2 * (rad - 9) - 6, 26, 12), juce::Justification::centred);
    }
    g.setColour (ink.withAlpha (0.75f)); g.setFont (uf (10.5f)); g.drawText ("GAIN REDUCTION", vu.withTrimmedBottom (16).withTrimmedTop (vu.getHeight() - 34), juce::Justification::centred);
    g.setFont (mf (10.0f)); g.drawText ("dB", vu.reduced (12, 8), juce::Justification::topLeft);
    glowText (g, "-" + juce::String (needle, 1), vu.reduced (12, 8), light ? juce::Colour (0xff8a1e1e) : c, mf (12.0f), juce::Justification::topRight);
    {
        const float a = angOf (needle); g.saveState(); g.reduceClipRegion (vu.toNearestInt());
        g.setColour (juce::Colours::black.withAlpha (0.25f)); g.drawLine (pcx + 4, pcy, pcx + 4 + std::sin (a) * (rad - 16), pcy - std::cos (a) * (rad - 16), 2.2f);
        g.setColour (light ? juce::Colour (0xff111111) : juce::Colour (0xffff4040)); g.drawLine (pcx, pcy, pcx + std::sin (a) * (rad - 16), pcy - std::cos (a) * (rad - 16), 1.7f);
        g.restoreState();
    }
    jlook::glass (g, vu, 8);
    }
    g.setColour (C::dim); g.setFont (uf (10.5f)); g.drawText (I.ratioLabel, juce::Rectangle<float> (16, 214, 200, 20), juce::Justification::centredLeft);
    if (! on) { g.setColour (neo::K::bg0.withAlpha (0.6f)); g.fillRoundedRectangle (b.withTrimmedTop (46), 14); g.setColour (C::dim); g.setFont (uf (13.0f)); g.drawText ("stadio spento: accendilo con il pulsante in alto a sinistra", b.withTrimmedTop (46), juce::Justification::centred); }
}

// ======================= Grafico =======================
void Graph::paint (juce::Graphics& g)
{
    auto& P = ed.proc; auto b = getLocalBounds().toFloat().reduced (1);
    jlook::card (g, b, 14, C::magenta, true, false, false);
    glowText (g, "DINAMICA", { 18, 8, 160, 20 }, C::text, uf (12.0f));
    g.setFont (mf (9.5f)); g.setColour (C::dim);
    g.drawText ("grigio = ingresso   ciano = uscita   colori = riduzione di ogni stadio", juce::Rectangle<float> (130, 8, b.getWidth() - 340, 20), juce::Justification::centredLeft);
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
    g.setColour (neo::K::hi.withAlpha (0.12f)); g.fillPath (pin);
    juce::Path pout; for (int k = 0; k < NH; ++k) { const float y = yLev (P.hOut[at (k)].load()); if (k == 0) pout.startNewSubPath (in.getX(), y); else pout.lineTo (in.getX() + k * dx, y); }
    g.setColour (C::phos.withAlpha (0.3f)); g.strokePath (pout, juce::PathStrokeType (3.5f)); g.setColour (C::phos); g.strokePath (pout, juce::PathStrokeType (1.3f));
    for (int st = 0; st < kStages; ++st)
    {
        if (ed.sv (st, "on") < 0.5f) continue;
        juce::Path pg; pg.startNewSubPath (in.getX(), in.getY()); float mx = 0;
        for (int k = 0; k < NH; ++k) { const float v = P.hGr[st][at (k)].load(); mx = juce::jmax (mx, v); pg.lineTo (in.getX() + k * dx, yGr (v)); }
        if (mx < 0.05f) continue;
        pg.lineTo (in.getRight(), in.getY()); pg.closeSubPath();
        const auto c = C::stage[st];
        g.setColour (c.withAlpha (st == ed.focus ? 0.22f : 0.08f)); g.fillPath (pg); g.setColour (c.withAlpha (st == ed.focus ? 0.95f : 0.5f)); g.strokePath (pg, juce::PathStrokeType (st == ed.focus ? 1.5f : 1.0f));
    }
    g.setColour (C::magenta); g.fillRect (in.getRight() - 2, in.getY(), 2.0f, in.getHeight());
    g.setColour (juce::Colours::black.withAlpha (neo::light() ? 0.03f : 0.16f)); for (float y = in.getY(); y < in.getBottom(); y += 3) g.drawHorizontalLine ((int) y, in.getX(), in.getRight());
    jlook::glass (g, in, 6);
    // curva di trasferimento dello stadio selezionato
    lcd (g, tr, 6);
    const auto sp = P.stageParams (ed.focus); const auto c = C::stage[ed.focus];
    auto tx = [&tr] (float db) { return tr.getX() + (db + 60.0f) / 60.0f * tr.getWidth(); };
    auto ty = [&tr] (float db) { return tr.getBottom() - (db + 60.0f) / 60.0f * tr.getHeight(); };
    g.setColour (C::phos.withAlpha (0.08f)); for (int d = -48; d < 0; d += 12) { g.drawVerticalLine ((int) tx ((float) d), tr.getY(), tr.getBottom()); g.drawHorizontalLine ((int) ty ((float) d), tr.getX(), tr.getRight()); }
    g.setColour (neo::K::hi.withAlpha (0.15f)); g.drawLine (tr.getX(), tr.getBottom(), tr.getRight(), tr.getY(), 1.0f);
    const float ig = inputGainDb (sp);
    juce::Path cv;
    for (int k = 0; k <= 120; ++k) { const float x = -60.0f + k * 0.5f, l = x + ig, o = juce::jlimit (-60.0f, 0.0f, x - gainComputer (sp, l)); if (k == 0) cv.startNewSubPath (tx (x), ty (o)); else cv.lineTo (tx (x), ty (o)); }
    g.setColour (c.withAlpha (0.3f)); g.strokePath (cv, juce::PathStrokeType (4.0f)); g.setColour (c); g.strokePath (cv, juce::PathStrokeType (1.6f));
    const float lin = juce::Decibels::gainToDecibels (juce::jmax (P.inPk[0].load(), P.inPk[1].load()), -60.0f);
    if (lin > -59) { const float o = juce::jlimit (-60.0f, 0.0f, lin - gainComputer (sp, lin + ig)); g.setColour (c.withAlpha (0.3f)); g.fillEllipse (tx (lin) - 7, ty (o) - 7, 14, 14); g.setColour (juce::Colours::white); g.fillEllipse (tx (lin) - 3, ty (o) - 3, 6, 6); }
    g.setColour (C::dim); g.setFont (mf (8.5f)); g.drawText (juce::String (info (sp.model).name) + "  " + juce::String (ed.focus + 1), tr.reduced (6, 4), juce::Justification::topLeft);
    jlook::glass (g, tr, 6);
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
        jlook::button (g, r, C::amber, i == tab, false, tabs[i], uf (11.0f));
        if (i == 2 && warn > 0) { g.setColour (C::red); g.fillEllipse (r.getRight() - 12, r.getY() + 4, 7, 7); }
    }
    auto t = b.reduced (18, 0).withTrimmedTop (44).withTrimmedBottom (12);
    juce::AttributedString as; as.setWordWrap (juce::AttributedString::byWord); as.setLineSpacing (2.0f);
    auto head = [&as] (const juce::String& s, juce::Colour c) { as.append (s + "\n", jlook::ui (11.0f, 2), c); };
    auto body = [&as] (const juce::String& s) { as.append (s + "\n", jlook::ui (12.0f, 0), C::text.withAlpha (0.9f)); };
    if (tab == 0)
    {
        const int m = (int) ed.sv (ed.focus, "model"); const auto& G = guide (m);
        head (juce::String (info (m).name) + "  -  " + info (m).inspired, neo::acc (juce::Colour (info (m).accent))); body (G.carattere);
        head ("PER COSA", C::dim); body (G.usi);
        head ("COME PARTIRE", C::dim); body (G.partenza);
    }
    else if (tab == 1)
    {
        head ("ROUTING", C::phos); body (routeTip ((int) ed.proc.value ("route")));
        head ("SOLO", C::phos); body ("SOLO su uno stadio fa sentire solo quello che sta togliendo: se senti il corpo dello strumento e non solo i picchi, stai comprimendo troppo.");
        head ("PRESET", C::phos); body (presets()[(size_t) juce::jlimit (0, (int) presets().size() - 1, ed.proc.preset)].note);
        head ("SIDECHAIN", C::phos); body ("ESTERNO: in Ableton apri il plugin, sezione Sidechain, scegli la traccia (es. la cassa). HPF: il rilevatore ignora i bassi sotto quella frequenza. LISTEN: ascolti cosa sente il rilevatore.");
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
    const float gr = proc.avgGr[f].load();
    const float inDb = juce::Decibels::gainToDecibels (std::sqrt (proc.inRms.load()), -100.0f), outDb = juce::Decibels::gainToDecibels (std::sqrt (proc.outRms.load()), -100.0f);
    const juce::String st = juce::String (f + 1);
    if (inDb < -60) { guide.tips = {}; guide.warn = 0; return; }
    if (proc.anySolo()) { t.add ("SOLO attivo: stai ascoltando solo quello che gli stadi in solo tolgono. Spegnilo per sentire il risultato."); ++w; }
    if (! sp.on) t.add ("Lo stadio " + st + " e spento: clicca la sua casella nella fila in alto per accenderlo.");
    else
    {
        if (gr < 0.5f && sp.model != OTT) { t.add ("Stadio " + st + ": quasi nessuna riduzione. Alza " + juce::String (info (sp.model).driveLabel) + " finche il VU scende di 2-6 dB."); ++w; }
        else if (gr > 12) { t.add ("Stadio " + st + ": oltre 12 dB di riduzione. Va bene solo in parallelo (MIX 20-40%), altrimenti stai schiacciando."); ++w; }
        else if (gr > 6) t.add ("Stadio " + st + ": riduzione forte (" + juce::String (gr, 1) + " dB). Ok per voce rap, room e smash; tanto per bus e master.");
        else t.add ("Stadio " + st + ": " + juce::String (gr, 1) + " dB di riduzione media, zona giusta per quasi tutto.");
        if (info (sp.model).relKnob && relMs (sp.model, sp.rel) > 0 && relMs (sp.model, sp.rel) < 90 && gr > 6) { t.add ("Rilascio molto corto con tanta riduzione: rischi distorsione sui bassi e pumping. Allunga il RELEASE."); ++w; }
        if (info (sp.model).attKnob && attMs (sp.model, sp.att) < 0.1f && gr > 3) t.add ("Attacco velocissimo: smussa i transienti. Per una batteria che picchia allunga l'attacco.");
        if (sp.model == OTT && sp.drive > 0.6f && sp.mix > 0.8f) t.add ("OTT molto profondo al 100% di MIX: suono famoso ma estremo. Prova MIX 30-50%.");
        if (proc.value ("scHpf") < 25 && gr > 3 && (sp.model == VCABUS || sp.model == VARIMU)) t.add ("Su bus e master alza l'HPF del sidechain a 80-150 Hz: il compressore smette di reagire alla cassa e non pompa.");
    }
    int busy = 0; for (int s = 0; s < kStages; ++s) if (sv (s, "on") > 0.5f && proc.avgGr[s].load() > 6) ++busy;
    if (busy >= 2) { t.add (juce::String (busy) + " stadi lavorano tanto: con piu stadi suona meglio 1-4 dB ciascuno."); ++w; }
    const float d = outDb - inDb;
    if (d > 2) { t.add ("L'uscita e piu forte dell'ingresso di " + juce::String (d, 1) + " dB: il piu forte sembra sempre meglio. Attiva AUTO GAIN o abbassa OUTPUT per un confronto onesto."); ++w; }
    else if (d < -3) t.add ("L'uscita e piu bassa di " + juce::String (-d, 1) + " dB: recupera con OUTPUT o AUTO GAIN.");
    if (proc.value ("scSrc") > 0.5f && ! proc.hasExtSidechain()) { t.add ("Sidechain ESTERNO selezionato ma nessuna traccia collegata: in Ableton scegli la sorgente nella sezione Sidechain del plugin."); ++w; }
    if (juce::jmax (proc.outPk[0].load(), proc.outPk[1].load()) > 0.99f) { t.add ("L'uscita tocca 0 dBFS: abbassa OUTPUT."); ++w; }
    guide.tips = t; guide.warn = w;
}

// ======================= Editor =======================
Editor::Editor (Processor& p) : AudioProcessorEditor (&p), proc (p)
{
    // tema MACCHINA: rack da studio anni 70, alluminio scuro, scritte crema, VU ambra
    skin::init ("comp", "STUDIO", { 0xff14161a, 0xff262a31, 0xffece6d8, 0xffa8a294, 0xff6e6b64, 0xff3b3f47, 0xff2b2e35, 0xff1b1812, 0xff0e0c09, 0xffe8a33d },
                { 0xffd2694f, 0xff9a86c0, 0xff86c0b6, 0xff9cc472, 0xffe8a33d, 0xffd8483c, 0xff7b9bd0 }, false, 0);
    C::sync();
    setLookAndFeel (&lnf);
    addAndMakeVisible (rack); addAndMakeVisible (stage); addAndMakeVisible (graph); addAndMakeVisible (guide);
    route = std::make_unique<Segs> (P ("route"), C::magenta); route->items = [] { return juce::StringArray { "SERIE", "PARALLELO" }; };
    route->map = [] (int k) { return k == 0 ? 0 : 2; };
    route->onTouch = [this] { repaint(); };
    scSrc = std::make_unique<Segs> (P ("scSrc"), C::green); scSrc->items = [] { return juce::StringArray { "INTERNO", "ESTERNO" }; };
    scTo = std::make_unique<Segs> (P ("scTo"), C::green); scTo->items = [] { return juce::StringArray { "TUTTI", "1", "2", "3", "4", "5", "6" }; };
    scHpf = std::make_unique<Knob> (P ("scHpf"), C::green); scHpf->label = "SC HPF";
    scHpf->text = [this] { const float f = proc.value ("scHpf"); return f < 21 ? juce::String ("OFF") : juce::String (juce::roundToInt (f)) + " Hz"; };
    listen = std::make_unique<Switch> (P ("scListen"), C::green, "LISTEN");
    autoG = std::make_unique<Switch> (P ("auto"), C::phos, "AUTO GAIN");
    gMix = std::make_unique<Knob> (P ("mix"), C::phos); gMix->label = "MIX"; gMix->text = [this] { return juce::String (juce::roundToInt (proc.value ("mix") * 100)) + "%"; };
    gOut = std::make_unique<Knob> (P ("out"), C::phos); gOut->label = "OUTPUT"; gOut->bipolar = true; gOut->text = [this] { const float v = proc.value ("out"); return (v > 0 ? "+" : "") + juce::String (v, 1) + " dB"; };
    for (juce::Component* c : std::initializer_list<juce::Component*> { route.get(), scSrc.get(), scTo.get(), scHpf.get(), listen.get(), autoG.get(), gMix.get(), gOut.get() }) addAndMakeVisible (c);
    addAndMakeVisible (sizeChip);
    recolor();
    scaler.setup();
    startTimerHz (30);
}
void Editor::recolor()
{
    C::sync();
    route->col = C::magenta; for (auto* c : { scSrc.get(), scTo.get() }) c->col = C::green; scHpf->col = C::green; listen->col = C::green;
    autoG->col = C::phos; gMix->col = C::phos; gOut->col = C::phos;
    stage.bind (focus);
    for (auto* c : getChildren()) c->repaint();
    repaint();
}
void Editor::select (int s) { focus = s; stage.bind (s); rack.repaint(); guide.repaint(); graph.repaint(); }

void Editor::resized()
{
    const float sc = scaler.update();
    presetRect = { 470, 15, 330, 36 }; prevRect = { 432, 15, 36, 36 }; nextRect = { 802, 15, 36, 36 }; abRect = { 850, 15, 110, 36 };
    rack.setBounds (18, 78, 792, 100);
    stage.setBounds (18, 190, 792, 300);
    const int rx = 818, rw = 404;
    route->setBounds (rx + 16, 112, rw - 32, 30);
    scSrc->setBounds (rx + 16, 236, 180, 28); scTo->setBounds (rx + 16, 272, 220, 28);
    scHpf->setBounds (rx + 246, 222, 72, 72 + 27); listen->setBounds (rx + 320, 250, 80, 22);
    autoG->setBounds (rx + 16, 380, 130, 22);
    gMix->setBounds (rx + 168, 366, 72, 72 + 27); gOut->setBounds (rx + 250, 366, 72, 72 + 27);
    graph.setBounds (18, 502, 792, 284); guide.setBounds (818, 502, 404, 284);
    sizeChip.setBounds (W - 76, H - 18, 58, 15);
    jlook::scaleChildren (*this, sc);
}

void Editor::paint (juce::Graphics& g)
{
    skin::apply(); C::sync();
    g.addTransform (juce::AffineTransform::scale (scaler.s));
    const float Wf = (float) W, Hf = (float) H;
    jlook::backdrop (g, { 0, 0, Wf, Hf }, C::magenta, C::phos);
    jlook::header (g, { 0, 0, Wf, 64 }, "STUDIO COMP", C::phos, "JC-6  MULTI COMPRESSOR", C::magenta, C::phos);
    for (auto* c : { (juce::Component*) &stage, (juce::Component*) &graph, (juce::Component*) &guide }) jlook::softShadow (g, c->getBounds().toFloat(), 14);
    lcd (g, presetRect.toFloat(), 8); jlook::glass (g, presetRect.toFloat(), 8);
    glowText (g, proc.getProgramName (proc.preset).toUpperCase(), presetRect.toFloat(), neo::light() ? C::text : C::amber, mf (12.5f), juce::Justification::centred);
    g.setColour (C::phos); g.setFont (uf (20.0f));
    g.drawText (juce::String::fromUTF8 ("\xe2\x80\xb9"), prevRect, juce::Justification::centred); g.drawText (juce::String::fromUTF8 ("\xe2\x80\xba"), nextRect, juce::Justification::centred);
    jlook::button (g, abRect.toFloat(), C::phos, proc.ab.cur == 1, abHover, "MEMORIA " + proc.ab.label(), uf (11.5f));
    if (proc.anySolo()) { auto sr = juce::Rectangle<float> (972, 18, 128, 30); jlook::button (g, sr, C::amber, true, false, "SOLO ATTIVO", uf (11.0f)); }
    skin::chipDraw (g, themeRect.toFloat());
    const float rx = 818, rw = 404;
    auto cardR = [&] (juce::Rectangle<float> r, juce::Colour c, const char* t) { jlook::softShadow (g, r, 14); jlook::card (g, r, 14, c, true, false, false); glowText (g, t, { r.getX() + 16, r.getY() + 6, 200, 22 }, C::text, uf (12.0f)); };
    cardR ({ rx, 78, rw, 112 }, C::magenta, "ROUTING");
    { // catena
        const bool par = (int) proc.value ("route") == 2; auto d = juce::Rectangle<float> (rx + 16, 150, rw - 32, 32);
        g.setColour (C::dim); g.setFont (mf (9.5f)); g.drawText ("IN", juce::Rectangle<float> (d.getX(), d.getY(), 24, 32), juce::Justification::centredLeft); g.drawText ("OUT", juce::Rectangle<float> (d.getRight() - 28, d.getY(), 28, 32), juce::Justification::centredRight);
        juce::Array<int> on; for (int s = 0; s < kStages; ++s) if (sv (s, "on") > 0.5f) on.add (s);
        const float x0 = d.getX() + 26, x1 = d.getRight() - 30, cy = d.getCentreY();
        g.setColour (C::dim.withAlpha (0.6f)); g.drawLine (x0, cy, x1, cy);
        const int n = juce::jmax (1, on.size()); const float bw = juce::jmin (34.0f, (x1 - x0) / n - 6);
        for (int i = 0; i < on.size(); ++i)
        {
            const int s = on[i]; auto r = par ? juce::Rectangle<float> ((x0 + x1) / 2 - bw / 2 + (i - (on.size() - 1) / 2.0f) * (bw + 4), cy - 9, bw, 18)
                                              : juce::Rectangle<float> (x0 + (x1 - x0) * (i + 0.5f) / n - bw / 2, cy - 9, bw, 18);
            g.setColour (C::stage[s].withAlpha (0.25f)); g.fillRoundedRectangle (r, 4); g.setColour (C::stage[s]); g.drawRoundedRectangle (r, 4, 1.0f);
            g.setFont (uf (11.0f)); g.drawText (juce::String (s + 1), r, juce::Justification::centred);
        }
        if (par && on.size() > 1) { g.setColour (C::dim); g.setFont (mf (8.5f)); g.drawText ("in parallelo, sommati", juce::Rectangle<float> (d.getX() + 30, d.getBottom() - 4, 200, 12), juce::Justification::centredLeft); }
    }
    cardR ({ rx, 202, rw, 140 }, C::green, "SIDECHAIN");
    g.setFont (mf (9.5f)); g.setColour (proc.hasExtSidechain() ? C::green : C::dim);
    g.drawText (proc.hasExtSidechain() ? "esterno collegato" : "esterno non collegato", juce::Rectangle<float> (rx + 200, 208, rw - 216, 18), juce::Justification::centredRight);
    g.setColour (C::dim); g.setFont (uf (9.5f)); g.drawText ("su", juce::Rectangle<float> (rx + 16, 300, 30, 14), juce::Justification::centredLeft);
    cardR ({ rx, 354, rw, 136 }, C::phos, "USCITA");
    auto meter = [&] (juce::Rectangle<float> r, float l, float rr, const char* t)
    {
        g.setColour (C::dim); g.setFont (mf (8.5f)); g.drawText (t, r.removeFromBottom (12), juce::Justification::centred);
        lcd (g, r, 3);
        for (int c = 0; c < 2; ++c)
        {
            auto m = juce::Rectangle<float> (r.getX() + 3 + c * (r.getWidth() / 2 - 1), r.getY() + 3, r.getWidth() / 2 - 5, r.getHeight() - 6);
            const float db = juce::Decibels::gainToDecibels (c ? rr : l, -60.0f), n = juce::jlimit (0.0f, 1.0f, (db + 60) / 63);
            const int seg = 22;
            for (int k = 0; k < seg; ++k) { const float t2 = k / (float) seg; g.setColour (t2 <= n ? (t2 > 0.93f ? C::red : t2 > 0.8f ? C::amber : C::phos) : neo::K::hi.withAlpha (0.05f)); g.fillRect (m.getX(), m.getBottom() - (k + 1) * m.getHeight() / seg + 1, m.getWidth(), m.getHeight() / seg - 1.5f); }
        }
    };
    meter ({ rx + 336, 384, 24, 100 }, proc.inPk[0].load(), proc.inPk[1].load(), "IN");
    meter ({ rx + 366, 384, 24, 100 }, proc.outPk[0].load(), proc.outPk[1].load(), "OUT");
    g.setColour (C::dim); g.setFont (uf (10.0f, false));
    g.drawFittedText ("AUTO GAIN pareggia il volume: confronta ad orecchio onesto.", juce::Rectangle<int> ((int) rx + 16, 412, 140, 60), juce::Justification::topLeft, 4);
}

void Editor::mouseMove (const juce::MouseEvent& e0)
{
    const auto e = e0.withNewPosition (e0.position / scaler.s);
    const bool h = abRect.contains (e.getPosition());
    if (h != abHover) { abHover = h; repaint (scaler.map (abRect.expanded (8))); }
    setMouseCursor (h || presetRect.contains (e.getPosition()) || prevRect.contains (e.getPosition()) || nextRect.contains (e.getPosition()) ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
}

void Editor::mouseDown (const juce::MouseEvent& e0)
{
    const auto e = e0.withNewPosition (e0.position / scaler.s);
    const int n = (int) presets().size();
    auto after = [this] { stage.bind (focus); rack.repaint(); repaint(); };
    if (themeRect.contains (e.getPosition())) { skin::cycle(); recolor(); return; }
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
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withTargetScreenArea (localAreaToGlobal (scaler.map (presetRect))), [this, after] (int r) { if (r > 0) { proc.loadPreset (r - 1); after(); } });
    }
}

void Editor::timerCallback()
{
    const float g = proc.gr[focus].load();
    stage.needle += (g - stage.needle) * (g > stage.needle ? 0.5f : 0.18f);
    stage.repaint (stage.vuRect().toNearestInt().expanded (8));
    for (auto* k : { stage.drive.get(), stage.att.get(), stage.rel.get(), stage.out.get(), stage.color.get(), stage.mix.get(), scHpf.get(), gMix.get(), gOut.get(), stage.bThr.get(), stage.bRatio.get(), stage.bAtt.get(), stage.bRel.get(), stage.bGain.get() }) if (k->isVisible()) k->repaint();
    if (stage.isMb()) stage.repaint (stage.vuRect().toNearestInt().expanded (4));
    rack.repaint(); graph.repaint();
    repaint (scaler.map ({ 818, 354, 404, 136 }));
    if (++tick % 15 == 0) { computeTips(); guide.repaint(); repaint (scaler.map ({ W - 260, 10, 250, 44 })); }
}
}
