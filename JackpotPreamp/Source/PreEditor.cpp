#include "PreEditor.h"

namespace jpre
{
using namespace jkit;
namespace A = alook::A;
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
    // schermo a fosfori verdi in una cornice nera
    auto b = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff111110)); g.fillRoundedRectangle (b, 14);
    auto s = b.reduced (9);
    juce::ColourGradient cg (juce::Colour (0xff132a1b), s.getCentreX(), s.getCentreY(), juce::Colour (0xff050c08), s.getX(), s.getY(), true); g.setGradientFill (cg); g.fillRoundedRectangle (s, 18);
    const auto ph = alook::A::phos;
    const double sr = juce::jmax (8000.0, ed.proc.sampleRate());
    g.setFont (alook::typed (8.5f));
    for (float f : { 30.f, 50.f, 100.f, 200.f, 500.f, 1000.f, 2000.f, 5000.f, 10000.f, 20000.f })
    { g.setColour (ph.withAlpha (0.07f)); g.drawVerticalLine ((int) xOf (f), s.getY() + 6, s.getBottom() - 6); g.setColour (ph.withAlpha (0.35f)); g.drawText (f >= 1000 ? juce::String ((int) (f / 1000)) + "k" : juce::String ((int) f), juce::Rectangle<float> (xOf (f) - 16, s.getBottom() - 16, 32, 12), juce::Justification::centred); }
    for (int db = -18; db <= 18; db += 6)
    { g.setColour (ph.withAlpha (db == 0 ? 0.18f : 0.06f)); g.drawHorizontalLine ((int) yOf ((float) db), 40, s.getRight() - 6); g.setColour (ph.withAlpha (0.35f)); g.drawText ((db > 0 ? "+" : "") + juce::String (db), juce::Rectangle<float> (12, yOf ((float) db) - 6, 24, 12), juce::Justification::centredRight); }
    {
        juce::Path sp; bool st = false;
        for (int x = 40; x < getWidth() - 10; x += 2)
        {
            const float f = fOf ((float) x); const int bin = juce::jlimit (1, 2047, (int) (f / sr * 4096)); const float db = spec[(size_t) bin] + 6.0f * std::log2 (juce::jmax (1.0f, f / 1000.0f)) * 0.5f;
            const float y = juce::jlimit (s.getY() + 4, s.getBottom() - 8, s.getBottom() - 8 - (db + 90.0f) / 90.0f * (s.getHeight() - 16));
            if (! st) { sp.startNewSubPath ((float) x, s.getBottom() - 8); st = true; } sp.lineTo ((float) x, y);
        }
        sp.lineTo (s.getRight() - 10, s.getBottom() - 8); sp.closeSubPath();
        g.setColour (ph.withAlpha (0.07f)); g.fillPath (sp);
    }
    const auto S = ed.proc.settings();
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
    const juce::Colour subC (0xffffb347), eqC (0xff7fd8ff);
    if (S.sOn) { g.setColour (subC.withAlpha (0.55f)); g.strokePath (ps, juce::PathStrokeType (1.2f)); }
    if (S.eOn) { g.setColour (eqC.withAlpha (0.55f)); g.strokePath (pe, juce::PathStrokeType (1.2f)); }
    g.setColour (ph.withAlpha (0.25f)); g.strokePath (pall, juce::PathStrokeType (6.0f)); g.setColour (ph.brighter (0.3f)); g.strokePath (pall, juce::PathStrokeType (1.8f));
    for (int k = 0; k < 6; ++k)
    {
        const auto p = pointOf (k); const bool sel = k == drag || k == hover || (k < 4 && k == ed.band);
        const auto c = S.sOn ? subC : ph.withAlpha (0.4f);
        g.setColour (c.withAlpha (sel ? 0.35f : 0.12f)); g.fillEllipse (p.x - 11, p.y - 11, 22, 22);
        g.setColour (c); g.drawEllipse (p.x - 8, p.y - 8, 16, 16, sel ? 1.8f : 1.1f);
        g.setFont (alook::typed (8.5f)); g.drawText (k < 4 ? juce::String (k + 1) : k == 4 ? juce::String ("HP") : juce::String ("LP"), juce::Rectangle<float> (p.x - 10, p.y - 7, 20, 14), juce::Justification::centred);
    }
    g.setFont (alook::typed (8.5f)); g.setColour (ph.withAlpha (0.5f));
    g.drawText (juce::String::fromUTF8 ("trascina i punti = taglia  \xc2\xb7  rotella = Q  \xc2\xb7  doppio clic = azzera"), juce::Rectangle<float> (48, s.getY() + 6, 500, 14), juce::Justification::centredLeft);
    auto leg = [&] (float x, const char* t, juce::Colour c) { g.setColour (c); g.fillRect (x, s.getY() + 12, 10.0f, 2.0f); g.setColour (ph.withAlpha (0.5f)); g.drawText (t, juce::Rectangle<float> (x + 14, s.getY() + 6, 90, 14), juce::Justification::centredLeft); };
    leg (s.getRight() - 330, "SOTTRATTIVO", subC); leg (s.getRight() - 220, "EQ COLORE", eqC); leg (s.getRight() - 120, "TOTALE", ph);
    // linee di scansione e riflesso del vetro
    g.setColour (juce::Colours::black.withAlpha (0.12f)); for (float y = s.getY(); y < s.getBottom(); y += 3) g.drawHorizontalLine ((int) y, s.getX(), s.getRight());
    juce::ColourGradient gl (juce::Colours::white.withAlpha (0.07f), s.getX(), s.getY(), juce::Colours::transparentWhite, s.getX(), s.getCentreY(), false); g.setGradientFill (gl); g.fillRoundedRectangle (s.withHeight (s.getHeight() * 0.45f), 18);
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
    const juce::Colour black (0xff1d1915), maroonCap (0xff6e1d18), greyCap (0xff5c5952);
    auto knob = [&] (const juce::String& id, const juce::String& label, std::function<juce::String()> txt, int stage, int group, juce::Colour cap = juce::Colour (0xff1d1915)) -> alook::Knob*
    {
        auto k = std::make_unique<alook::Knob> (P (id), label, cap); k->text = std::move (txt);
        k->onTouch = [this, stage] { if (stage >= 0) turnOn (stage); };
        addAndMakeVisible (*k); knobs.push_back ({ std::move (k), stage, group }); return knobs.back().k.get();
    };
    auto val = [this] (const juce::String& id) { return proc.apvts.getRawParameterValue (id)->load(); };
    // sottrattivo
    knob ("hp", "HIGH PASS", [this] { return proc.value ("hp") <= 20.5f ? juce::String ("OUT") : hzT (proc.value ("hp")); }, 0, -1, greyCap);
    knob ("lp", "LOW PASS", [this] { return proc.value ("lp") >= 19900 ? juce::String ("OUT") : hzT (proc.value ("lp")); }, 0, -1, greyCap);
    for (int b = 0; b < 4; ++b)
    {
        const juce::String pp = "b" + juce::String (b + 1);
        knob (pp + "f", "FREQUENCY", [val, pp] { return hzT (val (pp + "f")); }, 0, b, greyCap);
        knob (pp + "g", "CUT", [val, pp] { const float v = val (pp + "g"); return v > -0.05f ? juce::String ("0 dB") : juce::String (v, 1) + " dB"; }, 0, b, maroonCap);
        knob (pp + "q", "SHARP", [val, pp] { return "Q " + juce::String (val (pp + "q"), 1); }, 0, b, greyCap);
    }
    hpSl = std::make_unique<alook::Rotary> (P ("hpSl"), "dB/OCT", juce::StringArray { "12", "24", "48" }, 100); hpSl->knobFrac = 0.34f; hpSl->labelGap = 13; hpSl->fontSize = 8.5f; hpSl->onTouch = [this] { turnOn (0); }; addAndMakeVisible (*hpSl);
    lpSl = std::make_unique<alook::Rotary> (P ("lpSl"), "dB/OCT", juce::StringArray { "12", "24", "48" }, 100); lpSl->knobFrac = 0.34f; lpSl->labelGap = 13; lpSl->fontSize = 8.5f; lpSl->onTouch = [this] { turnOn (0); }; addAndMakeVisible (*lpSl);
    // preamp
    auto* kg = knob ("p_gain", "GAIN", [this] { return dbT (proc.value ("p_gain")); }, 1, -1, maroonCap); kg->big = true;
    knob ("p_out", "TRIM", [this] { return dbT (proc.value ("p_out")); }, 1, -1, black);
    juce::StringArray pm; for (int i = 0; i < kPreModels; ++i) pm.add (preInfo (i).name);
    preRot = std::make_unique<alook::Rotary> (P ("p_model"), "PREAMP", pm, 300); preRot->onTouch = [this] { turnOn (1); }; preRot->fontSize = 9.5f; addAndMakeVisible (*preRot);
    auto tg = [&] (const char* id, const char* l, const char* up, const char* dn) { toggles.push_back (std::make_unique<alook::Toggle> (P (id), l, up, dn)); addAndMakeVisible (*toggles.back()); };
    tg ("p_auto", "GAIN COMPENSATO", "AUTO", "MAN"); tg ("p_phase", "FASE", "180", "0");
    // EQ di colore
    auto num = [val] (const char* id) { return [val, id] { return juce::String (val (id), 1); }; };
    auto choiceTxt = [this] (const char* id) { return [this, id] { return P (id)->getCurrentValueAsText() + " Hz"; }; };
    knob ("pu_lf", "LOW FREQ", choiceTxt ("pu_lf"), 2, 0, black); knob ("pu_lb", "BOOST", num ("pu_lb"), 2, 0, black); knob ("pu_la", "ATTEN", num ("pu_la"), 2, 0, black);
    knob ("pu_hf", "HIGH FREQ", choiceTxt ("pu_hf"), 2, 0, black); knob ("pu_hb", "BOOST", num ("pu_hb"), 2, 0, black); knob ("pu_bw", "BANDWIDTH", [val] { return val ("pu_bw") < 3 ? juce::String ("SHARP") : val ("pu_bw") > 7 ? juce::String ("BROAD") : juce::String ("MID"); }, 2, 0, black);
    knob ("pu_as", "ATTEN SEL", choiceTxt ("pu_as"), 2, 0, black); knob ("pu_ha", "ATTEN", num ("pu_ha"), 2, 0, black);
    knob ("m_lf", "LOW PEAK", choiceTxt ("m_lf"), 2, 1, black); knob ("m_lg", "PEAK", num ("m_lg"), 2, 1, black);
    knob ("m_dip", "DIP", choiceTxt ("m_dip"), 2, 1, black); knob ("m_dg", "DIP", num ("m_dg"), 2, 1, black);
    knob ("m_hf", "HIGH PEAK", choiceTxt ("m_hf"), 2, 1, black); knob ("m_hg", "PEAK", num ("m_hg"), 2, 1, black);
    for (int k = 0; k < 4; ++k)
    {
        const juce::String gi = "a_g" + juce::String (k + 1), fi = "a_f" + juce::String (k + 1);
        knob (gi, "BAND " + juce::String (k + 1), [val, gi] { return dbT (val (gi)); }, 2, 2, maroonCap);
        knob (fi, "FREQ", [this, val, fi, k] { return bandFreqText ((int) proc.value ("e_model"), k, val (fi)); }, 2, 2, greyCap);
    }
    knob ("a_q", "Q", [this] { return pctT (proc.value ("a_q")); }, 2, 2, black);
    knob ("a_drive", "DRIVE", [this] { return pctT (proc.value ("a_drive")); }, 2, 2, black);
    knob ("tube", "TUBE", [this] { return pctT (proc.value ("tube")); }, 2, 3, black);
    juce::StringArray em; for (int i = 0; i < kEqModels; ++i) em.add (eqName (i));
    eqRot = std::make_unique<alook::Rotary> (P ("e_model"), "EQUALIZER", em, 300); eqRot->onTouch = [this] { turnOn (2); }; eqRot->onPick = [this] (int) { layoutVisibility(); }; eqRot->fontSize = 8.5f; addAndMakeVisible (*eqRot);
    knob ("out", "OUTPUT", [this] { return dbT (proc.value ("out")); }, -1, -1, black);
    addAndMakeVisible (graph); addAndMakeVisible (sizeChip);
    scaler.setup();
    startTimerHz (30);
}
void Editor::layoutVisibility()
{
    const int em = (int) proc.value ("e_model"); const int grp = em == E_EQP1A ? 0 : em == E_MEQ5 ? 1 : 2;
    auto c0 = card (0), c2 = card (2);
    std::vector<alook::Knob*> subAlways, subBand, eqVis;
    for (auto& ks : knobs)
    {
        auto* k = ks.k.get();
        if (ks.stage == 0) { const bool v = ks.group < 0 || ks.group == band; k->setVisible (v); if (ks.group < 0) subAlways.push_back (k); else if (v) subBand.push_back (k); }
        if (ks.stage == 2) { const bool v = ks.group == grp || (ks.group == 3 && grp < 2); k->setVisible (v); if (v) eqVis.push_back (k); }
    }
    subAlways[0]->setBounds (juce::Rectangle<float> (c0.getX() + 24, c0.getY() + 64, 86, 86 + 30).toNearestInt());
    subAlways[1]->setBounds (juce::Rectangle<float> (c0.getX() + 196, c0.getY() + 64, 86, 86 + 30).toNearestInt());
    hpSl->setBounds (juce::Rectangle<float> (c0.getX() + 110, c0.getY() + 78, 76, 76).toNearestInt());
    lpSl->setBounds (juce::Rectangle<float> (c0.getX() + 282, c0.getY() + 78, 76, 76).toNearestInt());
    for (size_t i = 0; i < subBand.size(); ++i) subBand[i]->setBounds (juce::Rectangle<float> (c0.getX() + 22 + (float) i * 116, c0.getY() + 290, 96, 96 + 30).toNearestInt());
    const int cols = grp == 2 ? 5 : grp == 0 ? 5 : 4;
    for (size_t i = 0; i < eqVis.size(); ++i)
    {
        const int r = (int) i / cols, c = (int) i % cols; const float cw = (c2.getWidth() - 28) / cols;
        eqVis[i]->setBounds (juce::Rectangle<float> (c2.getX() + 14 + c * cw + (cw - 70) / 2, c2.getY() + 216 + r * 104, 70, 70 + 30).toNearestInt());
    }
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
    graph.setBounds (52, 116, 1076, 268);
    auto c1 = card (1), c2 = card (2);
    preRot->setBounds (juce::Rectangle<float> (c1.getX() + 14, c1.getY() + 60, 210, 210).toNearestInt());
    for (auto& ks : knobs)
    {
        if (ks.k->param == P ("p_gain")) ks.k->setBounds (juce::Rectangle<float> (c1.getX() + 228, c1.getY() + 60, 130, 130 + 30).toNearestInt());
        if (ks.k->param == P ("p_out")) ks.k->setBounds (juce::Rectangle<float> (c1.getX() + 250, c1.getY() + 236, 86, 86 + 30).toNearestInt());
        if (ks.k->param == P ("out")) ks.k->setBounds (juce::Rectangle<float> (1062, 12, 56, 56 + 30).toNearestInt());
    }
    toggles[0]->setBounds (juce::Rectangle<float> (c1.getX() + 40, c1.getY() + 276, 180, 50).toNearestInt());
    toggles[1]->setBounds (juce::Rectangle<float> (c1.getX() + 40, c1.getY() + 330, 150, 50).toNearestInt());
    eqRot->setBounds (juce::Rectangle<float> (c2.getX() + 8, c2.getY() + 54, 236, 160).toNearestInt());
    sizeChip.setBounds (W - 76, H - 16, 58, 14);
    layoutVisibility();
}
void Editor::paint (juce::Graphics& g)
{
    g.addTransform (juce::AffineTransform::scale (scaler.s));
    // armadio rack: fondo scuro e montanti con i fori
    g.fillAll (A::rack2);
    juce::ColourGradient bg (A::rack, 0, 0, A::rack2, 0, (float) H, false); g.setGradientFill (bg); g.fillRect (0, 0, W, H);
    jlook::grain (g, { 0, 0, W, H }, 0.05f);
    for (float x : { 4.0f, (float) W - 12.0f }) for (float y = 20; y < H; y += 44) { g.setColour (juce::Colours::black.withAlpha (0.6f)); g.fillRoundedRectangle (x, y, 8, 14, 3); }
    // unita 1: targa, preset, A/B, VU
    alook::faceplate (g, { 16, 8, (float) W - 32, 92 });
    alook::engrave (g, "JACKPOT", { 40, 18, 260, 34 }, alook::wide (24.0f), juce::Justification::centredLeft);
    alook::engrave (g, "PREAMP  &  EQUALIZER", { 42, 50, 300, 16 }, alook::serif (11.0f), juce::Justification::centredLeft, A::maroon);
    alook::engrave (g, juce::String::fromUTF8 ("MODEL JP-73  \xc2\xb7  CLASS A  \xc2\xb7  TUBE"), { 42, 66, 300, 14 }, alook::typed (8.5f), juce::Justification::centredLeft, A::inkSoft);
    // nastro etichettatrice con il nome del preset
    {
        auto r = presetR; g.setColour (juce::Colours::black.withAlpha (0.4f)); g.fillRoundedRectangle (r.translated (0, 2), 3);
        juce::ColourGradient tg (juce::Colour (0xff27221e), r.getX(), r.getY(), juce::Colour (0xff15120f), r.getX(), r.getBottom(), false); g.setGradientFill (tg); g.fillRoundedRectangle (r, 3);
        g.setFont (alook::typed (13.0f)); g.setColour (juce::Colour (0xfff2ede0).withAlpha (0.9f));
        g.drawText (proc.getProgramName (proc.preset).toUpperCase(), r, juce::Justification::centred);
        alook::engrave (g, "PRESET", { r.getX(), r.getY() - 15, r.getWidth(), 12 }, alook::serif (8.0f));
        for (auto b : { prevR, nextR })
        {
            juce::ColourGradient cg (juce::Colour (0xffeeebe4), b.getX(), b.getY(), juce::Colour (0xff7a766e), b.getRight(), b.getBottom(), false); g.setGradientFill (cg); g.fillEllipse (b);
            g.setColour (A::ink); g.setFont (alook::serif (16.0f)); g.drawText (b == prevR ? juce::String::fromUTF8 ("\xe2\x80\xb9") : juce::String::fromUTF8 ("\xe2\x80\xba"), b, juce::Justification::centred);
        }
        alook::jewel (g, abA.getCentreX(), abA.getCentreY(), 8, A::amberLamp, proc.ab.cur == 0);
        alook::jewel (g, abB.getCentreX(), abB.getCentreY(), 8, A::amberLamp, proc.ab.cur == 1);
        alook::engrave (g, "A", { abA.getX(), abA.getY() - 14, abA.getWidth(), 12 }, alook::serif (9.0f)); alook::engrave (g, "B", { abB.getX(), abB.getY() - 14, abB.getWidth(), 12 }, alook::serif (9.0f));
        alook::engrave (g, "MEMORIA", { abA.getX() - 10, abA.getBottom() + 2, abB.getRight() - abA.getX() + 20, 12 }, alook::serif (7.5f));
    }
    alook::vuMeter (g, { 900, 20, 66, 68 }, vuIn, "IN");
    alook::vuMeter (g, { 976, 20, 66, 68 }, vuOut, "OUT");
    // unita 2: schermo
    alook::faceplate (g, { 16, 106, (float) W - 32, 290 });
    // unita 3: moduli
    const char* names[3] = { "SUBTRACTIVE EQ", "MIC / LINE PREAMP", "PROGRAM EQUALIZER" };
    const char* subs[3] = { "pulisci", "colora", "scolpisci" };
    for (int k = 0; k < 3; ++k)
    {
        const auto c = card (k); alook::faceplate (g, c);
        alook::jewel (g, c.getX() + 38, c.getY() + 32, 7, k == 0 ? A::red : k == 1 ? A::amberLamp : A::greenLamp, stageOn (k));
        alook::engrave (g, juce::String (k + 1) + "   " + names[k], { c.getX() + 56, c.getY() + 22, c.getWidth() - 80, 20 }, alook::serif (12.5f), juce::Justification::centredLeft);
        alook::engrave (g, subs[k], { c.getX() + 56, c.getY() + 40, 200, 12 }, alook::typed (8.5f), juce::Justification::centredLeft, A::inkSoft);
        g.setColour (A::ink.withAlpha (0.5f)); g.drawHorizontalLine ((int) c.getY() + 56, c.getX() + 24, c.getRight() - 24);
        // freccia del segnale stampata
        if (k < 2) { juce::Path a; a.addTriangle (c.getRight() + 1, c.getCentreY() - 6, c.getRight() + 7, c.getCentreY(), c.getRight() + 1, c.getCentreY() + 6); g.setColour (A::chrome); g.fillPath (a); }
    }
    // pulsanti delle bande del sottrattivo
    for (int k = 0; k < 4; ++k)
    {
        const auto r = bandBtn (k); const bool sel = k == band;
        const float gdb = proc.apvts.getRawParameterValue ("b" + juce::String (k + 1) + "g")->load();
        g.setColour (juce::Colours::black.withAlpha (0.35f)); g.fillRoundedRectangle (r.translated (0, 2), 5);
        juce::ColourGradient bgc (sel ? juce::Colour (0xff3a2e26) : juce::Colour (0xfff4efe2), r.getX(), r.getY(), sel ? juce::Colour (0xff15100c) : juce::Colour (0xffcfc6b0), r.getX(), r.getBottom(), false);
        g.setGradientFill (bgc); g.fillRoundedRectangle (r, 5); g.setColour (A::ink.withAlpha (0.6f)); g.drawRoundedRectangle (r, 5, 1.0f);
        g.setColour (sel ? A::cream : A::ink); g.setFont (alook::serif (11.0f)); g.drawText ("CUT " + juce::String (k + 1), r.withTrimmedRight (12), juce::Justification::centred);
        alook::jewel (g, r.getRight() - 9, r.getCentreY(), 3.5f, A::red, gdb < -0.05f && stageOn (0));
    }
    auto c0 = card (0), c1 = card (1), c2 = card (2);
    alook::engrave (g, "BAND", { c0.getX() + 24, c0.getY() + 216, 200, 14 }, alook::serif (9.0f), juce::Justification::centredLeft);
    g.setColour (A::inkSoft); g.setFont (alook::serif (9.5f, false));
    g.drawFittedText ("Togli quello che non serve: rimbombi, fango, risonanze. I punti si trascinano anche sullo schermo.", juce::Rectangle<float> (c0.getX() + 24, c0.getY() + 182, c0.getWidth() - 48, 28).toNearestInt(), juce::Justification::topLeft, 2, 1.0f);
    g.drawFittedText (preInfo ((int) proc.value ("p_model")).desc, juce::Rectangle<float> (c1.getX() + 24, c1.getY() + 384, c1.getWidth() - 48, 40).toNearestInt(), juce::Justification::topLeft, 3, 1.0f);
    g.drawFittedText (eqDesc ((int) proc.value ("e_model")), juce::Rectangle<float> (c2.getX() + 248, c2.getY() + 70, c2.getWidth() - 270, 130).toNearestInt(), juce::Justification::topLeft, 9, 1.0f);
}
void Editor::mouseDown (const juce::MouseEvent& e0)
{
    const auto e = e0.withNewPosition (e0.position / scaler.s);
    for (int k = 0; k < 3; ++k) if (juce::Rectangle<float> (card (k).getX() + 20, card (k).getY() + 14, 200, 36).contains (e.position)) { auto* p = P (k == 0 ? "s_on" : k == 1 ? "p_on" : "e_on"); p->setValueNotifyingHost (p->getValue() > 0.5f ? 0.0f : 1.0f); repaint(); return; }
    for (int k = 0; k < 4; ++k) if (bandBtn (k).contains (e.position)) { selectBand (k); return; }
    const int n = (int) presets().size();
    if (abA.expanded (4).contains (e.position)) { if (proc.ab.cur != 0) proc.ab.toggle (proc); repaint(); return; }
    if (abB.expanded (4).contains (e.position)) { if (proc.ab.cur != 1) proc.ab.toggle (proc); repaint(); return; }
    if (prevR.contains (e.position)) { proc.loadPreset ((proc.preset + n - 1) % n); repaint(); return; }
    if (nextR.contains (e.position)) { proc.loadPreset ((proc.preset + 1) % n); repaint(); return; }
    if (presetR.contains (e.position))
    {
        jkit::PresetBar pb; pb.preset = presetR.toNearestInt(); pb.ab = {}; pb.prev = {}; pb.next = {};
        pb.click (*this, proc, presets(), e.getPosition(), localAreaToGlobal (scaler.map (presetR.toNearestInt())));
    }
}
void Editor::timerCallback()
{
    graph.tick();
    if ((int) proc.value ("e_model") != lastEqModel) { layoutVisibility(); repaint(); }
    for (auto& ks : knobs) { const bool lit = ks.stage < 0 || stageOn (ks.stage); const float v = ks.k->param->getValue() + (lit ? 0.0f : 9.0f); if (lit != ks.k->lit || v != ks.k->shown) { ks.k->lit = lit; ks.k->shown = v; ks.k->repaint(); } }
    preRot->lit = stageOn (1); eqRot->lit = stageOn (2); hpSl->lit = lpSl->lit = stageOn (0);
    // VU: 0 VU = -18 dBFS
    auto vu = [] (float pk) { return juce::Decibels::gainToDecibels (pk, -60.0f) + 18.0f - 3.0f; };
    vuIn += (vu (proc.eng.inPk.load()) - vuIn) * 0.3f; vuOut += (vu (proc.eng.outPk.load()) - vuOut) * 0.3f;
    if (++tick % 2 == 0) repaint (scaler.map ({ 890, 10, 160, 86 }));
    if (tick % 3 == 0) { preRot->repaint(); eqRot->repaint(); hpSl->repaint(); lpSl->repaint(); for (auto& t : toggles) t->repaint(); }
    if (tick % 15 == 0) repaint();
}
}
