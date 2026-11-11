#include "ClipEditor.h"

namespace jclp
{
void WaveView::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat(); lcd (g, b, 8, ed.acc);
    auto r = b.reduced (10, 10);
    const float ceil = juce::Decibels::decibelsToGain (ed.proc.value ("ceil"));
    const auto& H = ed.proc.hist; const int w = H.w.load(), N = History::kCols;
    // scala: 0 dBFS a 85% dell'altezza (meta sopra e sotto)
    const float half = r.getHeight() * 0.5f, cy = r.getCentreY(), sc = half * 0.85f;
    auto Y = [&] (float v) { return juce::jmin (half, v * sc); };
    // linee del tetto
    g.setColour (ed.acc.withAlpha (0.5f)); const float yc = Y (ceil);
    for (float x = r.getX(); x < r.getRight(); x += 8) { g.drawHorizontalLine ((int) (cy - yc), x, x + 4); g.drawHorizontalLine ((int) (cy + yc), x, x + 4); }
    g.setFont (mf (9.0f)); g.drawText ("TETTO " + dbT (ed.proc.value ("ceil")), juce::Rectangle<float> (r.getRight() - 120, cy - yc - 14, 118, 12), juce::Justification::centredRight);
    const float cw = r.getWidth() / N;
    for (int k = 0; k < N; ++k)
    {
        const int i = (w + k) % N; const float vi = H.in[i].load(), vo = H.out[i].load(); const float x = r.getX() + k * cw;
        const float hi = Y (vi), ho = Y (vo * ceil);
        g.setColour (C::dim.withAlpha (0.35f)); g.fillRect (x, cy - hi, juce::jmax (1.0f, cw - 0.5f), 2 * hi);
        g.setColour (ed.acc2.withAlpha (0.75f)); g.fillRect (x, cy - ho, juce::jmax (1.0f, cw - 0.5f), 2 * ho);
        if (vi > ceil * 1.001f) { g.setColour (C::red); g.fillRect (x, cy - hi, juce::jmax (1.0f, cw - 0.5f), juce::jmax (1.5f, hi - ho)); g.fillRect (x, cy + ho, juce::jmax (1.0f, cw - 0.5f), juce::jmax (1.5f, hi - ho)); }
    }
}
void CurveView::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat(); lcd (g, b, 8, ed.acc);
    auto r = b.reduced (14, 14).withTrimmedBottom (16);
    const int m = (int) ed.proc.value ("mode"); const float drive = juce::Decibels::decibelsToGain (ed.proc.value ("drive"));
    g.setColour (C::dim.withAlpha (0.25f)); g.drawLine (r.getX(), r.getBottom(), r.getRight(), r.getY(), 1.0f);
    g.drawHorizontalLine ((int) r.getCentreY(), r.getX(), r.getRight()); g.drawVerticalLine ((int) r.getCentreX(), r.getY(), r.getBottom());
    // ingresso da -2 a +2 volte il tetto
    juce::Path p;
    for (int i = 0; i <= 200; ++i)
    {
        const float x = -2.0f + 4.0f * i / 200.0f; const float y = juce::jlimit (-1.0f, 1.0f, shapeMode (m == CRISP ? CRISP : m, x * drive));
        const float px = r.getX() + r.getWidth() * i / 200.0f, py = r.getCentreY() - y * r.getHeight() * 0.25f * 1.6f;
        if (i == 0) p.startNewSubPath (px, py); else p.lineTo (px, py);
    }
    g.setColour (ed.acc.withAlpha (0.3f)); g.strokePath (p, juce::PathStrokeType (5.0f)); g.setColour (ed.acc); g.strokePath (p, juce::PathStrokeType (1.8f));
    glowText (g, modeName (m), b.withTrimmedLeft (14).withHeight (24), ed.acc, uf (12.0f));
    g.setColour (C::dim); g.setFont (mf (9.0f)); g.drawText ("ingresso -> uscita", b.withTrimmedRight (14).withHeight (24), juce::Justification::centredRight);
    const float red = ed.proc.eng.clipDb.load();
    glowText (g, "TAGLIO  -" + juce::String (red, 1) + " dB", b.withTrimmedLeft (14).removeFromBottom (24), red > 0.05f ? C::red : C::dim, mf (11.0f));
}

Editor::Editor (Processor& p) : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);
    auto knob = [&] (const char* id, const char* label, std::function<juce::String()> txt, juce::Colour c, bool bip = false) -> Knob*
    {
        auto k = std::make_unique<Knob> (P (id), c, label); k->text = std::move (txt); k->bipolar = bip;
        addAndMakeVisible (*k); knobs.push_back (std::move (k)); return knobs.back().get();
    };
    knob ("drive", "DRIVE", [this] { return "+" + juce::String (proc.value ("drive"), 1) + " dB"; }, acc)->big = true;
    knob ("ceil", "TETTO", [this] { return dbT (proc.value ("ceil"), 2); }, acc);
    knob ("punch", "PUNCH", [this] { const float v = proc.value ("punch"); return std::abs (v) < 0.01f ? juce::String ("0") : (v > 0 ? "+" : "") + juce::String (juce::roundToInt (v * 100)); }, acc2, true);
    knob ("mix", "MIX", [this] { return pctT (proc.value ("mix")); }, acc2);
    knob ("out", "OUTPUT", [this] { return dbT (proc.value ("out")); }, acc2, true);
    kSplit = knob ("splitHz", "FREQ SPLIT", [this] { return hzT (proc.value ("splitHz")); }, C::phos);
    juce::StringArray ms; for (int i = 0; i < kModes; ++i) ms.add (modeName (i));
    modeSeg = std::make_unique<Segs> (P ("mode"), acc, ms); modeSeg->rows = 2; modeSeg->font = 11.0f; addAndMakeVisible (*modeSeg);
    osSeg = std::make_unique<Segs> (P ("os"), C::phos, juce::StringArray { "OFF", "2x", "4x", "8x" }); addAndMakeVisible (*osSeg);
    auto sw = [&] (const char* id, juce::Colour c, const char* l) { switches.push_back (std::make_unique<Switch> (P (id), c, l)); addAndMakeVisible (*switches.back()); };
    sw ("split", C::phos, "SPLIT: BASSI SEPARATI"); sw ("link", acc2, "VOLUME COMPENSATO"); sw ("delta", C::red, "DELTA: ASCOLTA IL TAGLIATO");
    addAndMakeVisible (wave); addAndMakeVisible (curve); addAndMakeVisible (sizeChip);
    scaler.setup();
    startTimerHz (30);
}
void Editor::resized()
{
    const float sc = scaler.update();
    wave.setBounds (30, 114, 676, 250);
    curve.setBounds (742, 114, 288, 250);
    modeSeg->setBounds (30, 426, 676, 64);
    for (int i = 0; i < 5; ++i) knobs[(size_t) i]->setBounds (i == 0 ? 56 : 196 + (i - 1) * 128, i == 0 ? 534 : 548, i == 0 ? 88 : 66, i == 0 ? 88 + 32 : 66 + 27);
    osSeg->setBounds (742, 446, 288, 28);
    switches[0]->setBounds (742, 486, 220, 22);
    kSplit->setBounds (966, 480, 56, 56 + 27);
    switches[1]->setBounds (742, 520, 220, 22);
    switches[2]->setBounds (742, 554, 260, 22);
    sizeChip.setBounds (W - 76, H - 18, 58, 15);
    jlook::scaleChildren (*this, sc);
}
void Editor::paint (juce::Graphics& g)
{
    g.addTransform (juce::AffineTransform::scale (scaler.s));
    jlook::backdrop (g, { 0, 0, (float) W, (float) H }, acc, acc2);
    jlook::header (g, { 0, 0, (float) W, 64 }, "CLIP", acc, "JC-9  CLIPPER A 9 CURVE", acc, acc2);
    bar.paint (g, proc.getProgramName (proc.preset), acc, proc.ab.cur == 1, proc.ab.label());
    titledCard (g, { 18, 76, 700, 300 }, "FORMA D'ONDA", "grigio entra, arancio esce, rosso tagliato", acc);
    titledCard (g, { 730, 76, 312, 300 }, "CURVA", "", acc);
    titledCard (g, { 18, 388, 700, 274 }, "MODO DI CLIPPING", "", acc);
    titledCard (g, { 730, 388, 312, 274 }, "OPZIONI", "", C::phos);
    g.setColour (C::dim); g.setFont (mf (9.0f)); g.drawText ("SOVRACAMPIONAMENTO", juce::Rectangle<int> (742, 426, 200, 16), juce::Justification::centredLeft);
    g.setColour (C::text.withAlpha (0.7f)); g.setFont (uf (10.0f, false));
    g.drawFittedText (modeDesc ((int) proc.value ("mode")), juce::Rectangle<int> (32, 494, 672, 32), juce::Justification::topLeft, 2, 1.0f);
    // misuratori
    auto mt = [&] (float x, const char* t, float v, juce::Colour c) { vMeter (g, { x, 588, 120, 10 }, 0, c); 
        const float db = juce::Decibels::gainToDecibels (v, -60.0f); const float tt = juce::jlimit (0.0f, 1.0f, (db + 48) / 48);
        g.setColour (c); g.fillRect (juce::Rectangle<float> (x + 2, 590, 116 * tt, 6)); g.setColour (C::dim); g.setFont (mf (8.5f)); g.drawText (juce::String (t) + " " + juce::String (db, 1), juce::Rectangle<float> (x, 600, 120, 12), juce::Justification::centredLeft); };
    mt (742, "IN", proc.eng.inPk.load(), C::dim); mt (900, "OUT", proc.eng.outPk.load(), acc2);
}
void Editor::mouseMove (const juce::MouseEvent& e0) { const auto e = e0.withNewPosition (e0.position / scaler.s); const bool h = bar.ab.contains (e.getPosition()); if (h != bar.abHover) { bar.abHover = h; repaint(); } }
void Editor::mouseDown (const juce::MouseEvent& e0)
{
    const auto e = e0.withNewPosition (e0.position / scaler.s);
    bar.click (*this, proc, presets(), e.getPosition(), localAreaToGlobal (scaler.map (bar.preset)));
}
void Editor::timerCallback()
{
    wave.repaint(); curve.repaint();
    kSplit->lit = proc.value ("split") > 0.5f;
    for (auto& k : knobs) { const float v = k->param->getValue() + (k->lit ? 0.0f : 9.0f); if (v != k->shown) { k->shown = v; k->repaint(); } }
    if (++tick % 3 == 0) { modeSeg->repaint(); osSeg->repaint(); for (auto& s : switches) s->repaint(); repaint (scaler.map ({ 18, 480, 1030, 140 })); }
    if (tick % 15 == 0) repaint (scaler.map ({ 0, 0, W, 64 }));
}
}
