#include "PreEditor.h"

namespace jpre
{
static juce::String bandFreqText (int model, int k, float v)
{
    const auto& B = jb::aeInfo (aeOf (model)).b[k]; const float f = B.freqs[(size_t) jb::aeIndex (B, v)];
    if (B.type == jb::Bq::HP && f < 1) return "OFF";
    return hzT (f);
}
// ======================================================================= grafico
Graph::Graph (Editor& e) : ed (e) { win.resize (4096); buf.resize (8192); spec.assign (2048, -100.0f); for (int i = 0; i < 4096; ++i) win[(size_t) i] = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * i / 4095.0f); }
float Graph::xOf (float f) const { return 40.0f + (getWidth() - 50.0f) * std::log (f / 20.0f) / std::log (1000.0f); }
float Graph::fOf (float x) const { return 20.0f * std::pow (1000.0f, juce::jlimit (0.0f, 1.0f, (x - 40.0f) / (getWidth() - 50.0f))); }
float Graph::yOf (float db) const { return 14.0f + (getHeight() - 28.0f) * (0.5f - db / 48.0f); }
float Graph::dbOf (float y) const { return (0.5f - (y - 14.0f) / (getHeight() - 28.0f)) * 48.0f; }
juce::Point<float> Graph::pointOf (int k) const
{
    auto& P = ed.proc;
    if (k < 4) { const juce::String p = "b" + juce::String (k + 1); return { xOf (P.apvts.getRawParameterValue (p + "f")->load()), yOf (P.apvts.getRawParameterValue (p + "g")->load()) }; }
    if (k == 4) return { xOf (P.value ("hp")), yOf (0) };
    return { xOf (P.value ("lp")), yOf (0) };
}
int Graph::hit (juce::Point<float> p) const { int best = -1; float bd = 14; for (int k = 0; k < 6; ++k) { const float d = p.getDistanceFrom (pointOf (k)); if (d < bd) { bd = d; best = k; } } return best; }
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
    auto b = getLocalBounds().toFloat(); lcd (g, b, 8, ed.colEq);
    const double sr = juce::jmax (8000.0, ed.proc.sampleRate());
    // griglia
    g.setFont (mf (8.5f));
    for (float f : { 30.f, 50.f, 100.f, 200.f, 500.f, 1000.f, 2000.f, 5000.f, 10000.f, 20000.f })
    { g.setColour (juce::Colours::white.withAlpha (0.05f)); g.drawVerticalLine ((int) xOf (f), 8, b.getBottom() - 8); g.setColour (C::faint); g.drawText (f >= 1000 ? juce::String ((int) (f / 1000)) + "k" : juce::String ((int) f), juce::Rectangle<float> (xOf (f) - 16, b.getBottom() - 14, 32, 12), juce::Justification::centred); }
    for (int db = -18; db <= 18; db += 6)
    { g.setColour (juce::Colours::white.withAlpha (db == 0 ? 0.12f : 0.04f)); g.drawHorizontalLine ((int) yOf ((float) db), 40, b.getRight() - 10); g.setColour (C::faint); g.drawText ((db > 0 ? "+" : "") + juce::String (db), juce::Rectangle<float> (4, yOf ((float) db) - 6, 30, 12), juce::Justification::centredRight); }
    // analizzatore (uscita)
    {
        juce::Path sp; bool st = false;
        for (int x = 40; x < getWidth() - 10; x += 2)
        {
            const float f = fOf ((float) x); const int bin = juce::jlimit (1, 2047, (int) (f / sr * 4096)); const float db = spec[(size_t) bin] + 6.0f * std::log2 (juce::jmax (1.0f, f / 1000.0f)) * 0.5f;
            const float y = juce::jlimit (8.0f, b.getBottom() - 8, b.getBottom() - 8 - (db + 90.0f) / 90.0f * (b.getHeight() - 16));
            if (! st) { sp.startNewSubPath ((float) x, b.getBottom() - 8); st = true; } sp.lineTo ((float) x, y);
        }
        sp.lineTo (b.getRight() - 10, b.getBottom() - 8); sp.closeSubPath();
        g.setColour (juce::Colours::white.withAlpha (0.05f)); g.fillPath (sp);
    }
    const auto S = ed.proc.settings();
    // curve
    jkit::Bq bs[4], hs[4], ls[4]; Engine::subFilters (S, sr, bs, hs, ls);
    jb::Bq eqf[4]; bool act[4]; int nb = 4; Engine::eqFilters (S, sr, eqf, act, nb);
    const auto& I = preInfo (S.pModel); jkit::Bq pt[5]; pt[0].hp (sr, I.hpHz, 0.6); pt[1].lp (sr, juce::jmin ((double) I.lpHz, sr * 0.45), 0.6); pt[2].lowShelf (sr, I.lowHz, I.lowDb, 0.7); pt[3].peak (sr, I.midHz, I.midQ, I.midDb); pt[4].highShelf (sr, I.airHz, I.airDb, 0.7);
    juce::Path ps, pe, pall; bool first = true;
    for (int x = 40; x < getWidth() - 10; x += 2)
    {
        const double f = fOf ((float) x);
        double ds = 0, dp = 0, de = 0;
        if (S.sOn) { for (int k = 0; k < 4; ++k) if (S.bG[k] < -0.05f) ds += bs[k].magDb (f, sr); if (S.hpHz > 20.5f) for (auto& q : hs) ds += q.magDb (f, sr); if (S.lpHz < 19900) for (auto& q : ls) ds += q.magDb (f, sr); }
        if (S.pOn) for (auto& q : pt) dp += q.magDb (f, sr);
        if (S.eOn) for (int k = 0; k < nb; ++k) if (act[k]) de += eqf[k].magDb (f, sr);
        const float ys = yOf ((float) juce::jlimit (-30.0, 30.0, ds)), ye = yOf ((float) juce::jlimit (-30.0, 30.0, de)), ya = yOf ((float) juce::jlimit (-30.0, 30.0, ds + dp + de));
        if (first) { ps.startNewSubPath ((float) x, ys); pe.startNewSubPath ((float) x, ye); pall.startNewSubPath ((float) x, ya); first = false; }
        else { ps.lineTo ((float) x, ys); pe.lineTo ((float) x, ye); pall.lineTo ((float) x, ya); }
    }
    if (S.sOn) { g.setColour (ed.colSub.withAlpha (0.6f)); g.strokePath (ps, juce::PathStrokeType (1.3f)); }
    if (S.eOn) { g.setColour (ed.colEq.withAlpha (0.6f)); g.strokePath (pe, juce::PathStrokeType (1.3f)); }
    g.setColour (C::text.withAlpha (0.25f)); g.strokePath (pall, juce::PathStrokeType (5.0f)); g.setColour (C::text); g.strokePath (pall, juce::PathStrokeType (1.8f));
    // punti del sottrattivo
    for (int k = 0; k < 6; ++k)
    {
        const auto p = pointOf (k); const bool sel = k == drag || k == hover || (k < 4 && k == ed.band);
        const auto c = S.sOn ? ed.colSub : C::faint;
        g.setColour (c.withAlpha (sel ? 0.35f : 0.15f)); g.fillEllipse (p.x - 11, p.y - 11, 22, 22);
        g.setColour (c); g.drawEllipse (p.x - 8, p.y - 8, 16, 16, sel ? 2.0f : 1.2f);
        g.setFont (uf (9.5f)); g.drawText (k < 4 ? juce::String (k + 1) : k == 4 ? juce::String ("HP") : juce::String ("LP"), juce::Rectangle<float> (p.x - 10, p.y - 7, 20, 14), juce::Justification::centred);
    }
    g.setFont (mf (9.0f)); g.setColour (C::dim);
    g.drawText (juce::String::fromUTF8 ("trascina i punti = taglia  \xc2\xb7  rotella = Q  \xc2\xb7  doppio clic = azzera"), juce::Rectangle<float> (46, 6, 500, 14), juce::Justification::centredLeft);
    auto leg = [&] (float x, const char* t, juce::Colour c) { g.setColour (c); g.fillRect (x, 11.0f, 10.0f, 2.0f); g.setColour (C::dim); g.drawText (t, juce::Rectangle<float> (x + 14, 5, 90, 14), juce::Justification::centredLeft); };
    leg (b.getRight() - 330, "SOTTRATTIVO", ed.colSub); leg (b.getRight() - 220, "EQ COLORE", ed.colEq); leg (b.getRight() - 120, "TOTALE", C::text);
}
void Graph::mouseMove (const juce::MouseEvent& e) { const int h = hit (e.position); if (h != hover) { hover = h; repaint(); } }
void Graph::mouseDown (const juce::MouseEvent& e)
{
    drag = hit (e.position); if (drag < 0) return;
    ed.turnOn (0); if (drag < 4) ed.selectBand (drag);
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

// ======================================================================= editor
Editor::Editor (Processor& p) : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);
    auto knob = [&] (const juce::String& id, const juce::String& label, std::function<juce::String()> txt, int stage, int group, juce::Colour c, bool bip = false) -> Knob*
    {
        auto k = std::make_unique<Knob> (P (id), c, label); k->text = std::move (txt); k->bipolar = bip;
        k->onTouch = [this, stage] { turnOn (stage); };
        addAndMakeVisible (*k); knobs.push_back ({ std::move (k), stage, group }); return knobs.back().k.get();
    };
    auto val = [this] (const juce::String& id) { return proc.apvts.getRawParameterValue (id)->load(); };
    // sottrattivo
    knob ("hp", "PASSA ALTO", [this] { return proc.value ("hp") <= 20.5f ? juce::String ("OFF") : hzT (proc.value ("hp")); }, 0, -1, colSub);
    knob ("lp", "PASSA BASSO", [this] { return proc.value ("lp") >= 19900 ? juce::String ("OFF") : hzT (proc.value ("lp")); }, 0, -1, colSub);
    for (int b = 0; b < 4; ++b)
    {
        const juce::String pp = "b" + juce::String (b + 1);
        knob (pp + "f", "FREQUENZA", [val, pp] { return hzT (val (pp + "f")); }, 0, b, colSub);
        knob (pp + "g", "TAGLIO", [val, pp] { const float v = val (pp + "g"); return v > -0.05f ? juce::String ("0 dB") : juce::String (v, 1) + " dB"; }, 0, b, colSub);
        knob (pp + "q", "Q", [val, pp] { return juce::String (val (pp + "q"), 1); }, 0, b, colSub);
    }
    hpSl = std::make_unique<Segs> (P ("hpSl"), colSub, juce::StringArray { "12", "24", "48" }); hpSl->font = 9.0f; addAndMakeVisible (*hpSl);
    lpSl = std::make_unique<Segs> (P ("lpSl"), colSub, juce::StringArray { "12", "24", "48" }); lpSl->font = 9.0f; addAndMakeVisible (*lpSl);
    // preamp
    knob ("p_gain", "GAIN", [this] { return dbT (proc.value ("p_gain")); }, 1, -1, colPre)->big = true;
    knob ("p_out", "USCITA", [this] { return dbT (proc.value ("p_out")); }, 1, -1, colPre, true);
    juce::StringArray pm; for (int i = 0; i < kPreModels; ++i) pm.add (preInfo (i).name);
    preSeg = std::make_unique<Segs> (P ("p_model"), colPre, pm); preSeg->rows = 2; preSeg->font = 10.5f; preSeg->onPick = [this] (int) { turnOn (1); }; addAndMakeVisible (*preSeg);
    auto sw = [&] (const char* id, juce::Colour c, const char* l) { switches.push_back (std::make_unique<Switch> (P (id), c, l)); addAndMakeVisible (*switches.back()); };
    sw ("p_auto", colPre, "GAIN COMPENSATO"); sw ("p_phase", colPre, "INVERTI FASE");
    // EQ di colore: Pultec (gruppo 0), MEQ (1), analogici (2)
    auto num = [val] (const char* id) { return [val, id] { return juce::String (val (id), 1); }; };
    auto choiceTxt = [this] (const char* id) { return [this, id] { return P (id)->getCurrentValueAsText() + " Hz"; }; };
    knob ("pu_lf", "LOW FREQ", choiceTxt ("pu_lf"), 2, 0, colEq); knob ("pu_lb", "BOOST", num ("pu_lb"), 2, 0, colEq); knob ("pu_la", "ATTEN", num ("pu_la"), 2, 0, colEq);
    knob ("pu_hf", "HIGH FREQ", choiceTxt ("pu_hf"), 2, 0, colEq); knob ("pu_hb", "BOOST", num ("pu_hb"), 2, 0, colEq); knob ("pu_bw", "BANDA", [val] { return val ("pu_bw") < 3 ? juce::String ("STRETTA") : val ("pu_bw") > 7 ? juce::String ("LARGA") : juce::String ("MEDIA"); }, 2, 0, colEq);
    knob ("pu_as", "ATTEN SEL", choiceTxt ("pu_as"), 2, 0, colEq); knob ("pu_ha", "ATTEN", num ("pu_ha"), 2, 0, colEq);
    knob ("m_lf", "LOW PEAK", choiceTxt ("m_lf"), 2, 1, colEq); knob ("m_lg", "PICCO", num ("m_lg"), 2, 1, colEq);
    knob ("m_dip", "DIP", choiceTxt ("m_dip"), 2, 1, colEq); knob ("m_dg", "TAGLIO", num ("m_dg"), 2, 1, colEq);
    knob ("m_hf", "HIGH PEAK", choiceTxt ("m_hf"), 2, 1, colEq); knob ("m_hg", "PICCO", num ("m_hg"), 2, 1, colEq);
    for (int k = 0; k < 4; ++k)
    {
        const juce::String gi = "a_g" + juce::String (k + 1), fi = "a_f" + juce::String (k + 1);
        knob (gi, "BANDA " + juce::String (k + 1), [val, gi] { return dbT (val (gi)); }, 2, 2, colEq, true);
        knob (fi, "FREQ", [this, val, fi, k] { return bandFreqText ((int) proc.value ("e_model"), k, val (fi)); }, 2, 2, colEq);
    }
    knob ("a_q", "Q", [this] { return pctT (proc.value ("a_q")); }, 2, 2, colEq);
    knob ("a_drive", "COLORE", [this] { return pctT (proc.value ("a_drive")); }, 2, 2, colEq);
    knob ("tube", "VALVOLA", [this] { return pctT (proc.value ("tube")); }, 2, 3, colEq);   // pultec e meq
    juce::StringArray em; for (int i = 0; i < kEqModels; ++i) em.add (eqName (i));
    eqSeg = std::make_unique<Segs> (P ("e_model"), colEq, em); eqSeg->rows = 2; eqSeg->font = 9.5f; eqSeg->onPick = [this] (int) { turnOn (2); layoutVisibility(); }; addAndMakeVisible (*eqSeg);
    knob ("out", "OUTPUT", [this] { return dbT (proc.value ("out")); }, -1, -1, C::text, true);
    addAndMakeVisible (graph); addAndMakeVisible (sizeChip);
    scaler.setup();
    startTimerHz (30);
}
void Editor::layoutVisibility()
{
    const int em = (int) proc.value ("e_model"); const int grp = em == E_EQP1A ? 0 : em == E_MEQ5 ? 1 : 2;
    // sottrattivo
    auto c0 = card (0), c1 = card (1), c2 = card (2);
    std::vector<Knob*> subAlways, subBand, eqVis;
    for (auto& ks : knobs)
    {
        auto* k = ks.k.get();
        if (ks.stage == 0) { const bool v = ks.group < 0 || ks.group == band; k->setVisible (v); if (ks.group < 0) subAlways.push_back (k); else if (v) subBand.push_back (k); }
        if (ks.stage == 2) { const bool v = ks.group == grp || (ks.group == 3 && grp < 2); k->setVisible (v); if (v) eqVis.push_back (k); }
    }
    subAlways[0]->setBounds (juce::Rectangle<float> (c0.getX() + 30, c0.getY() + 44, 66, 66 + 27).toNearestInt());
    subAlways[1]->setBounds (juce::Rectangle<float> (c0.getX() + 200, c0.getY() + 44, 66, 66 + 27).toNearestInt());
    hpSl->setBounds (juce::Rectangle<float> (c0.getX() + 104, c0.getY() + 70, 84, 24).toNearestInt());
    lpSl->setBounds (juce::Rectangle<float> (c0.getX() + 274, c0.getY() + 70, 84, 24).toNearestInt());
    for (size_t i = 0; i < subBand.size(); ++i) subBand[i]->setBounds (juce::Rectangle<float> (c0.getX() + 34 + (float) i * 112, c0.getY() + 240, 72, 72 + 27).toNearestInt());
    bandTabs = { c0.getX() + 12, c0.getY() + 194, c0.getWidth() - 24, 30 };
    // EQ: griglia a 5 colonne
    const int cols = grp == 2 ? 5 : grp == 0 ? 5 : 4;
    for (size_t i = 0; i < eqVis.size(); ++i)
    {
        const int r = (int) i / cols, c = (int) i % cols; const float cw = (c2.getWidth() - 24) / cols;
        eqVis[i]->setBounds (juce::Rectangle<float> (c2.getX() + 12 + c * cw + (cw - 58) / 2, c2.getY() + 152 + r * 100, 58, 58 + 27).toNearestInt());
    }
    juce::ignoreUnused (c1);
    if (grp == 2)
        for (auto& ks : knobs)
            for (int k = 0; k < 4; ++k)
            {
                const auto& B = jb::aeInfo (aeOf (em)).b[k];
                if (ks.k->param == P ("a_g" + juce::String (k + 1))) { ks.k->label = juce::String (B.name) + (B.type == jb::Bq::HP ? " (-)" : ""); ks.k->repaint(); }
                if (ks.k->param == P ("a_f" + juce::String (k + 1))) { ks.k->label = juce::String (B.name) + " FREQ"; ks.k->repaint(); }
            }
    jlook::scaleChildren (*this, scaler.s);
    lastEqModel = em;
}
void Editor::resized()
{
    scaler.update();
    graph.setBounds (30, 106, 1120, 278);
    auto c1 = card (1), c2 = card (2);
    preSeg->setBounds (juce::Rectangle<float> (c1.getX() + 12, c1.getY() + 40, c1.getWidth() - 24, 60).toNearestInt());
    for (auto& ks : knobs)
    {
        if (ks.k->param == P ("p_gain")) ks.k->setBounds (juce::Rectangle<float> (c1.getX() + 40, c1.getY() + 186, 100, 100 + 32).toNearestInt());
        if (ks.k->param == P ("p_out")) ks.k->setBounds (juce::Rectangle<float> (c1.getX() + 170, c1.getY() + 200, 70, 70 + 27).toNearestInt());
        if (ks.k->param == P ("out")) ks.k->setBounds (juce::Rectangle<float> (c1.getX() + 280, c1.getY() + 200, 64, 64 + 27).toNearestInt());
    }
    switches[0]->setBounds (juce::Rectangle<float> (c1.getX() + 20, c1.getY() + 324, 170, 22).toNearestInt());
    switches[1]->setBounds (juce::Rectangle<float> (c1.getX() + 200, c1.getY() + 324, 150, 22).toNearestInt());
    eqSeg->setBounds (juce::Rectangle<float> (c2.getX() + 12, c2.getY() + 40, c2.getWidth() - 24, 60).toNearestInt());
    sizeChip.setBounds (W - 76, H - 18, 58, 15);
    layoutVisibility();
}
void Editor::paint (juce::Graphics& g)
{
    g.addTransform (juce::AffineTransform::scale (scaler.s));
    jlook::backdrop (g, { 0, 0, (float) W, (float) H }, colPre, colEq);
    jlook::header (g, { 0, 0, (float) W, 64 }, "PREAMP EQ", colPre, "JP-73  PREAMP / EQ SOTTRATTIVO / EQ VALVOLARI", colPre, colEq);
    bar.preset = { 500, 15, 330, 36 }; bar.prev = { 462, 15, 36, 36 }; bar.next = { 832, 15, 36, 36 }; bar.ab = { 880, 15, 110, 36 };
    bar.paint (g, proc.getProgramName (proc.preset), colPre, proc.ab.cur == 1, proc.ab.label());
    titledCard (g, { 18, 76, 1144, 320 }, "RISPOSTA", "pulisci  >  colora  >  scolpisci", C::text);
    const char* names[3] = { "1  EQ SOTTRATTIVO", "2  PREAMP", "3  EQ DI COLORE" };
    const char* subs[3] = { "pulito, senza colore", "trasformatori e valvole", "valvole e console" };
    const juce::Colour cs[3] = { colSub, colPre, colEq };
    for (int k = 0; k < 3; ++k) titledCard (g, card (k), names[k], subs[k], cs[k], stageOn (k));
    // flusso
    for (int k = 0; k < 2; ++k) { const auto c = card (k); g.setColour (colPre.withAlpha (0.4f)); juce::Path a; a.addTriangle (c.getRight() + 1, c.getCentreY() - 6, c.getRight() + 8, c.getCentreY(), c.getRight() + 1, c.getCentreY() + 6); g.fillPath (a); }
    // linguette delle bande del sottrattivo
    {
        lcd (g, bandTabs, 6, colSub); const float w = bandTabs.getWidth() / 4;
        for (int k = 0; k < 4; ++k)
        {
            auto r = juce::Rectangle<float> (bandTabs.getX() + k * w, bandTabs.getY(), w, bandTabs.getHeight()).reduced (3, 3);
            const float gdb = proc.apvts.getRawParameterValue ("b" + juce::String (k + 1) + "g")->load();
            const juce::String t = "TAGLIO " + juce::String (k + 1) + (gdb < -0.05f ? juce::String::fromUTF8 (" \xe2\x97\x8f") : juce::String());
            if (k == band) { g.setColour (colSub.withAlpha (0.2f)); g.fillRoundedRectangle (r, 4); g.setColour (colSub); g.drawRoundedRectangle (r, 4, 1); glowText (g, t, r, colSub, uf (10.0f), juce::Justification::centred); }
            else { g.setColour (C::dim); g.setFont (uf (10.0f)); g.drawText (t, r, juce::Justification::centred); }
        }
    }
    auto c0 = card (0), c1 = card (1), c2 = card (2);
    g.setColour (C::dim); g.setFont (uf (9.5f, false));
    g.drawFittedText ("Togli prima quello che non serve: rimbombi, fango, risonanze, fischi. Trascina i punti sul grafico.", juce::Rectangle<float> (c0.getX() + 14, c0.getY() + 146, c0.getWidth() - 28, 40).toNearestInt(), juce::Justification::topLeft, 3, 1.0f);
    g.drawFittedText ("Pendenza", juce::Rectangle<float> (c0.getX() + 104, c0.getY() + 52, 84, 16).toNearestInt(), juce::Justification::centredLeft, 1);
    g.drawFittedText ("Pendenza", juce::Rectangle<float> (c0.getX() + 274, c0.getY() + 52, 84, 16).toNearestInt(), juce::Justification::centredLeft, 1);
    g.setColour (C::text.withAlpha (0.7f));
    g.drawFittedText (preInfo ((int) proc.value ("p_model")).desc, juce::Rectangle<float> (c1.getX() + 14, c1.getY() + 106, c1.getWidth() - 28, 74).toNearestInt(), juce::Justification::topLeft, 5, 1.0f);
    g.drawFittedText (eqDesc ((int) proc.value ("e_model")), juce::Rectangle<float> (c2.getX() + 14, c2.getY() + 106, c2.getWidth() - 28, 44).toNearestInt(), juce::Justification::topLeft, 3, 1.0f);
    // misuratori
    auto mt = [&] (float y, const char* t, float v, juce::Colour c) { const float db = juce::Decibels::gainToDecibels (v, -60.0f); const float tt = juce::jlimit (0.0f, 1.0f, (db + 48) / 48);
        lcd (g, { 1060, y, 100, 8 }, 3, c); g.setColour (c); g.fillRect (juce::Rectangle<float> (1062, y + 2, 96 * tt, 4)); g.setColour (C::dim); g.setFont (mf (8.5f)); g.drawText (juce::String (t) + " " + juce::String (db, 1), juce::Rectangle<float> (996, y - 3, 60, 14), juce::Justification::centredRight); };
    mt (21, "IN", proc.eng.inPk.load(), C::dim); mt (37, "OUT", proc.eng.outPk.load(), colPre);
}
void Editor::mouseMove (const juce::MouseEvent& e0) { const auto e = e0.withNewPosition (e0.position / scaler.s); const bool h = bar.ab.contains (e.getPosition()); if (h != bar.abHover) { bar.abHover = h; repaint(); } }
void Editor::mouseDown (const juce::MouseEvent& e0)
{
    const auto e = e0.withNewPosition (e0.position / scaler.s);
    for (int k = 0; k < 3; ++k) if (card (k).withHeight (32).contains (e.position)) { auto* p = P (k == 0 ? "s_on" : k == 1 ? "p_on" : "e_on"); p->setValueNotifyingHost (p->getValue() > 0.5f ? 0.0f : 1.0f); repaint(); return; }
    if (bandTabs.contains (e.position)) { selectBand ((int) ((e.position.x - bandTabs.getX()) / (bandTabs.getWidth() / 4))); return; }
    bar.click (*this, proc, presets(), e.getPosition(), localAreaToGlobal (scaler.map (bar.preset)));
}
void Editor::timerCallback()
{
    graph.tick();
    if ((int) proc.value ("e_model") != lastEqModel) { layoutVisibility(); repaint(); }
    for (auto& ks : knobs) { const bool lit = ks.stage < 0 || stageOn (ks.stage); const float v = ks.k->param->getValue() + (lit ? 0.0f : 9.0f); if (lit != ks.k->lit || v != ks.k->shown) { ks.k->lit = lit; ks.k->shown = v; ks.k->repaint(); } }
    if (++tick % 3 == 0) { preSeg->repaint(); eqSeg->repaint(); hpSl->repaint(); lpSl->repaint(); for (auto& s : switches) s->repaint(); repaint (scaler.map ({ 990, 10, 180, 44 })); }
    if (tick % 15 == 0) repaint();
}
}
