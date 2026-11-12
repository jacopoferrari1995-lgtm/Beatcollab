// Modalita FACILE: una scena unica, senza scatole. Onda viva al centro, INPUT a sinistra, barra del risultato e misure a destra,
// sotto un banco di manopole 3D con al centro il LOUD.
#include "MasterEditor.h"

namespace jms
{
using namespace neo;
static juce::String U (const char* s) { return juce::String::fromUTF8 (s); }
static const juce::StringArray& viewNames() { static juce::StringArray a { "ONDA", "SPETTRO", "GAIN", "REFERENCE", "ORECCHIE" }; return a; }

// zone (coordinate della pagina, 1280 x 736)
static const juce::Rectangle<int> zStage { 150, 46, 850, 350 }, zIn { 24, 46, 104, 262 }, zRes { 1018, 46, 44, 350 }, zMet { 1084, 46, 176, 350 };
static constexpr float kFloor = 414, kCapY = 440, kKnobY = 486, kS = 70;

EasyPage::EasyPage (Editor& e) : ed (e)
{
    auto mk = [this] (const char* id, const char* label, juce::Colour a, juce::Colour b, bool bip, std::function<juce::String()> t) -> neo::Knob*
    {
        auto* k = knobs.add (new neo::Knob (ed.P (id), label, a, b)); k->bipolar = bip; k->text = std::move (t); addAndMakeVisible (k); return k;
    };
    auto db = [this] (const char* id) { return [this, id] { return fmt (ed.proc.value (id)) + " dB"; }; };
    auto pc = [this] (const char* id) { return [this, id] { return juce::String (juce::roundToInt (ed.proc.value (id) * 100)) + "%"; }; };
    low  = mk ("e_low", "BASSI", K::amber, K::pink, true, db ("e_low"));   low->setTooltip ("Shelf morbido sotto i 90 Hz: peso e sub.");
    mid  = mk ("e_mid", "MEDI", K::amber, K::pink, true, db ("e_mid"));    mid->setTooltip ("Campana larga a 1.2 kHz: presenza di voce e rullante.");
    high = mk ("e_high", "ALTI", K::amber, K::pink, true, db ("e_high"));  high->setTooltip ("Shelf d'aria sopra i 9 kHz.");
    thick = mk ("e_thick", "THICK", K::pink, K::violet, false, pc ("e_thick")); thick->setTooltip ("Valvola solo sui medio-bassi: spessore e corpo, a volume uguale.");
    warm  = mk ("e_warm", "WARM", K::pink, K::violet, false, pc ("e_warm"));    warm->setTooltip ("Nastro da 15 pollici: compatta e arrotonda.");
    silk  = mk ("e_silk", "SILK", K::pink, K::violet, false, pc ("e_silk"));    silk->setTooltip ("Armoniche solo sopra i 4.5 kHz: lucentezza senza durezza.");
    loud  = mk ("e_loud", "LOUD", K::pink, K::cyan, false, [this] { return "+" + juce::String (ed.proc.value ("e_loud"), 1); }); loud->big = true; loud->sub = U ("dB di volume in pi\xc3\xb9");
    loud->setTooltip ("Quanto alzi il volume. La corona esterna mostra quanto lavora il limiter.");
    pres  = mk ("e_pres", "PRESENZA", K::pink, K::red, false, [this] { const float v = ed.proc.value ("e_pres"); return v < 0.3f ? juce::String ("SOFT") : v < 0.7f ? juce::String ("NATURAL") : juce::String ("HARD"); });
    pres->setTooltip ("Come il limiter tratta i picchi: morbido (arrotonda) o duro (clipper, piu volume e grinta).");
    speed = mk ("e_speed", "VELOCITA", K::violet, K::cyan, false, [this] { const float v = ed.proc.value ("e_speed"); return v < 0.33f ? juce::String ("SMOOTH") : v < 0.66f ? juce::String ("MEDIUM") : juce::String ("FAST"); });
    speed->setTooltip ("Rilascio del limiter: smooth (morbido) o fast (piu volume, piu movimento).");
    glue = mk ("e_glue", "GLUE", K::violet, K::blue, false, pc ("e_glue")); glue->setTooltip ("Compressore di bus 2:1 con rilascio automatico: tiene insieme il mix.");
    calm = mk ("e_calm", "CALM", K::violet, K::blue, false, pc ("e_calm")); calm->setTooltip ("Abbassa da solo le risonanze che saltano fuori: fango, durezze, sibilanti.");
    wide = mk ("e_wide", "WIDE", K::blue, K::cyan, true, [this] { const float w = ed.proc.value ("e_wide"); return juce::String (juce::roundToInt (100 + w * (w > 0 ? 80 : 100))) + "%"; });
    wide->setTooltip ("Allarga o stringe lo stereo sopra 120 Hz.");
    ceil = mk ("lm_ceil", "CEILING", K::mint, K::cyan, false, [this] { return juce::String (ed.proc.value ("lm_ceil"), 1) + " dB"; }); ceil->setTooltip ("Picco massimo in uscita (true peak).");
    trim = mk ("trim", "GAIN", K::cyan, K::blue, true, [this] { return fmt (ed.proc.value ("trim")) + " dB"; }); trim->setTooltip ("Livello d'ingresso. AUTO lo porta nella zona ottimale.");
    monoB = std::make_unique<neo::Toggle> (ed.P ("e_monob"), "BASSI MONO", K::cyan); addAndMakeVisible (*monoB);
    for (juce::Component* c : std::initializer_list<juce::Component*> { &stage, &input, &result, &meters }) addAndMakeVisible (c);
    pres->toFront (false); speed->toFront (false);
}

void EasyPage::resized()
{
    auto kb = [] (neo::Knob* k, float cx, float top, float s) { const float w = juce::jmax (s + 10, 84.0f); k->setBounds (juce::Rectangle<float> (cx - w * 0.5f, top, w, s + 38).expanded ((float) neo::Knob::pad).toNearestInt()); };
    kb (low, 58, kKnobY, kS); kb (mid, 136, kKnobY, kS); kb (high, 214, kKnobY, kS);
    kb (thick, 314, kKnobY, kS); kb (warm, 392, kKnobY, kS); kb (silk, 470, kKnobY, kS);
    loud->setBounds (juce::Rectangle<int> (510, 432, 260, 236).expanded (neo::Knob::pad));
    kb (pres, 548, 598, 50); kb (speed, 732, 598, 50);
    kb (glue, 836, kKnobY, kS); kb (calm, 914, kKnobY, kS); kb (wide, 992, kKnobY, kS);
    kb (ceil, 1100, kKnobY, kS);
    monoB->setBounds (1150, (int) kKnobY + 22, 116, 26);
    kb (trim, (float) zIn.getCentreX(), 306, 48);
    stage.setBounds (zStage); input.setBounds (zIn); result.setBounds (zRes); meters.setBounds (zMet);
    tabs.layout ({ (float) zStage.getX() + 4, 8, 0, 28 }, viewNames(), 9.0f); tabs.ax = -1;
    cmpR = { (float) zStage.getRight() - 88, 8, 84, 28 }; targetR = { cmpR.getX() - 156, 8, 148, 28 };
    bg = {};
}

// ---------------------------------------------------------------------------------------------------- statico (in cache)
void EasyPage::paintStatic (juce::Graphics& g)
{
    const float W = (float) getWidth();
    // pavimento: una lastra di luce che sfuma ai lati, nessun angolo
    {
        juce::ColourGradient fl (juce::Colours::white.withAlpha (0.035f), 0, kFloor, juce::Colours::white.withAlpha (0.0f), 0, kFloor + 180, false);
        g.setGradientFill (fl); g.fillRect (0.0f, kFloor, W, (float) getHeight() - kFloor);
        juce::ColourGradient hl (juce::Colours::white.withAlpha (0.0f), 40, kFloor, juce::Colours::white.withAlpha (0.0f), W - 40, kFloor, false);
        hl.addColour (0.2, juce::Colours::white.withAlpha (0.10f)); hl.addColour (0.5, K::violet.withAlpha (0.45f)); hl.addColour (0.8, juce::Colours::white.withAlpha (0.10f));
        g.setGradientFill (hl); g.fillRect (40.0f, kFloor, W - 80, 1.0f);
        juce::ColourGradient pl (K::violet.withAlpha (0.16f), 640, kFloor, K::violet.withAlpha (0.0f), 640 + 520, kFloor, true); g.setGradientFill (pl); g.fillEllipse (120, kFloor - 60, 1040, 120);
    }
    // luce dietro il LOUD
    { const float cx = 640, cy = 432 + 107; juce::ColourGradient hg (K::pink.withAlpha (0.13f), cx, cy, K::pink.withAlpha (0.0f), cx + 200, cy, true); g.setGradientFill (hg); g.fillEllipse (cx - 200, cy - 200, 400, 400); }
    // pozza di luce sotto l'onda
    { juce::ColourGradient wg (K::blue.withAlpha (0.10f), 575, 221, K::blue.withAlpha (0.0f), 575 + 460, 221, true); g.setGradientFill (wg); g.fillEllipse (115, 40, 920, 362); }
    // didascalie dei gruppi: punto colorato + parola, niente riquadri
    auto capt = [&] (float x, const char* t, const char* s, juce::Colour c)
    {
        g.setColour (c.withAlpha (0.3f)); g.fillEllipse (x - 3, kCapY + 2, 10, 10); g.setColour (c); g.fillEllipse (x, kCapY + 5, 4, 4);
        g.setFont (cap (10.0f)); g.setColour (K::ink.withAlpha (0.9f)); g.drawText (t, juce::Rectangle<float> (x + 12, kCapY, 240, 14), juce::Justification::centredLeft);
        g.setFont (f (9.5f)); g.setColour (K::faint); g.drawText (U (s), juce::Rectangle<float> (x + 12, kCapY + 15, 240, 14), juce::Justification::centredLeft);
    };
    capt (24, "EQ", "bande fisse, curve da master", K::amber);
    capt (280, "COLORE", "valvola \xc2\xb7 nastro \xc2\xb7 seta", K::pink);
    capt (802, "DINAMICA E SPAZIO", "colla, risonanze, stereo", K::violet);
    capt (1066, "USCITA", "picco massimo", K::mint);
    g.setFont (f (9.5f)); g.setColour (K::faint.withAlpha (0.8f));
    g.drawText (U ("doppio clic: torna a zero   \xc2\xb7   shift: regolazione fine   \xc2\xb7   passa sopra una manopola per sapere cosa fa"), juce::Rectangle<float> (24, (float) getHeight() - 30, 640, 14), juce::Justification::centredLeft);
    g.setFont (cap (9.0f)); g.setColour (K::sub);
    g.drawText ("RISULTATO", juce::Rectangle<float> ((float) zRes.getX() - 20, 14, (float) zRes.getWidth() + 40, 14), juce::Justification::centred);
}
void EasyPage::paint (juce::Graphics& g)
{
    const float sc = juce::jlimit (1.0f, 4.0f, g.getInternalContext().getPhysicalPixelScaleFactor());
    if (bg.isNull() || std::abs (bgScale - sc) > 0.01f)
    {
        bgScale = sc; bg = juce::Image (juce::Image::ARGB, juce::roundToInt (getWidth() * sc), juce::roundToInt (getHeight() * sc), true);
        juce::Graphics ig (bg); ig.addTransform (juce::AffineTransform::scale (sc)); paintStatic (ig);
    }
    { g.saveState(); g.addTransform (juce::AffineTransform::scale (1.0f / sc)); g.drawImageAt (bg, 0, 0); g.restoreState(); }
    tabs.paint (g, viewNames(), view, K::violet, 9.0f, hover);
    const auto& T = target ((int) ed.proc.value ("target"));
    chip (g, targetR, juce::String (T.name) + "  " + juce::String (T.lufs, 0) + U ("  \xe2\x96\xbe"), true, K::mint, 9.0f, hover == 10);
    const bool cmp = ed.proc.value ("compare") > 0.5f;
    chip (g, cmpR, cmp ? "ORIGINALE" : "BYPASS", cmp, K::cyan, 9.0f, hover == 11);
}
void EasyPage::mouseMove (const juce::MouseEvent& e)
{
    int h = tabs.hit (e.position); if (targetR.contains (e.position)) h = 10; if (cmpR.contains (e.position)) h = 11;
    if (h != hover) { hover = h; repaint (0, 0, getWidth(), 44); }
    setMouseCursor (h >= 0 ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
}
void EasyPage::mouseDown (const juce::MouseEvent& e)
{
    auto& P = ed.proc;
    if (const int t = tabs.hit (e.position); t >= 0) { view = t; repaint (0, 0, getWidth(), 44); stage.repaint(); return; }
    if (cmpR.contains (e.position)) { auto* p = ed.P ("compare"); p->setValueNotifyingHost (p->getValue() > 0.5f ? 0.0f : 1.0f); repaint (0, 0, getWidth(), 44); return; }
    if (targetR.contains (e.position))
    {
        juce::PopupMenu m; const int cur = (int) P.value ("target");
        for (int i = 0; i < kNumTargets; ++i) m.addItem (i + 1, juce::String (target (i).name) + "   " + juce::String (target (i).lufs, 0) + " LUFS", true, i == cur);
        auto* tp = ed.P ("target"); juce::Component::SafePointer<EasyPage> sp (this);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [tp, sp] (int r) { if (r > 0) { tp->setValueNotifyingHost (tp->convertTo0to1 ((float) (r - 1))); if (sp) sp->repaint(); } });
    }
}
void EasyPage::tick()
{
    stage.repaint(); result.repaint(); meters.repaint(); input.repaint();
    loud->activity = juce::jlimit (0.0f, 1.0f, ed.proc.grAvg.load() / 6.0f);
    for (auto* k : knobs) if (k->animate()) k->repaint();
    if (tabs.step (view)) repaint (0, 0, getWidth(), 44);
    monoB->repaint();
}

// ---------------------------------------------------------------------------------------------------- INPUT
void EasyInput::paint (juce::Graphics& g)
{
    auto& P = page.ed.proc;
    const float W = (float) getWidth();
    g.setFont (cap (9.0f)); g.setColour (K::sub); g.drawText ("INPUT", juce::Rectangle<float> (0, 0, W, 14), juce::Justification::centred);
    auto m = juce::Rectangle<float> (W * 0.5f - 18, 26, 10, 170);
    auto yL = [&m] (float db) { return m.getBottom() - juce::jlimit (0.0f, 1.0f, (db + 36.0f) / 36.0f) * m.getHeight(); };
    const float lo = Processor::kInOpt - 2.5f, hi = Processor::kInOpt + 2.5f, s = P.inS.load(), a = P.inAvg.load();
    sm = sm < -90 ? s : sm + (s - sm) * 0.35f;
    g.setColour (juce::Colours::black.withAlpha (0.5f)); g.fillRoundedRectangle (m.expanded (2), 7);
    g.setColour (juce::Colours::white.withAlpha (0.06f)); g.drawRoundedRectangle (m.expanded (2.5f), 7, 1.0f);
    // zona ottimale: fascia di luce menta
    auto zone = juce::Rectangle<float>::leftTopRightBottom (m.getX() - 10, yL (hi), m.getRight() + 10, yL (lo));
    { juce::ColourGradient zg (K::mint.withAlpha (0.0f), zone.getX(), 0, K::mint.withAlpha (0.0f), zone.getRight(), 0, false); zg.addColour (0.5, K::mint.withAlpha (0.25f)); g.setGradientFill (zg); g.fillRect (zone); }
    g.setColour (K::mint.withAlpha (0.7f)); g.fillRect (zone.getX(), zone.getY(), 3.0f, zone.getHeight()); g.fillRect (zone.getRight() - 3, zone.getY(), 3.0f, zone.getHeight());
    if (sm > -60)
    {
        auto lv = juce::Rectangle<float>::leftTopRightBottom (m.getX(), yL (sm), m.getRight(), m.getBottom());
        const auto top = sm > hi ? K::red : (sm >= lo ? K::mint : K::cyan);
        g.setColour (top.withAlpha (0.18f)); g.fillRoundedRectangle (lv.expanded (5, 3), 8);
        juce::ColourGradient lg (K::blue, 0, lv.getBottom(), top, 0, lv.getY(), false); g.setGradientFill (lg); g.fillRoundedRectangle (lv, 5);
    }
    if (a > -60) { const float y = yL (a); g.setColour (juce::Colours::white.withAlpha (0.25f)); g.fillRoundedRectangle (m.getX() - 8, y - 3, m.getWidth() + 16, 6, 3); g.setColour (juce::Colours::white); g.fillRoundedRectangle (m.getX() - 6, y - 1, m.getWidth() + 12, 2, 1); }
    g.setFont (num (8.5f)); g.setColour (K::faint);
    for (int d : { -30, -24, -18, -12, -6, 0 }) g.drawText (juce::String (d), juce::Rectangle<float> (m.getRight() + 14, yL ((float) d) - 5, 30, 10), juce::Justification::centredLeft);
    juce::String st = "PLAY"; juce::Colour c = K::faint;
    if (a > -60) { if (a > hi) { st = "ABBASSA"; c = K::red; } else if (a < lo) { st = "ALZA"; c = K::amber; } else { st = "PERFETTO"; c = K::mint; } }
    neo::glowText (g, st, { 0, 204, W, 18 }, c, cap (10.5f), juce::Justification::centred);
    autoR = { 18, 230, W - 36, 24 };
    chip (g, autoR, "AUTO", false, K::cyan, 9.0f, isMouseOver() && autoR.contains (getMouseXYRelative().toFloat()));
}
void EasyInput::mouseDown (const juce::MouseEvent& e) { if (autoR.contains (e.position)) { page.ed.proc.autoInput(); repaint(); } }

// ---------------------------------------------------------------------------------------------------- RISULTATO
void EasyResult::paint (juce::Graphics& g)
{
    auto& P = page.ed.proc;
    const float W = (float) getWidth();
    auto b = juce::Rectangle<float> (W * 0.5f - 5, 26, 10, (float) getHeight() - 52);
    juce::ColourGradient cg (K::red, 0, b.getY(), K::cyan, 0, b.getBottom(), false); cg.addColour (0.5, K::mint); cg.addColour (0.25, K::amber); cg.addColour (0.75, K::mint.interpolatedWith (K::cyan, 0.5f));
    g.setColour (juce::Colours::black.withAlpha (0.5f)); g.fillRoundedRectangle (b.expanded (3), 8);
    g.setGradientFill (cg); g.setOpacity (0.12f); g.fillRoundedRectangle (b.expanded (8, 0), 12); g.setOpacity (0.55f); g.fillRoundedRectangle (b, 5); g.setOpacity (1.0f);
    g.setFont (cap (8.0f));
    g.setColour (K::red); g.drawText ("CRUSH", juce::Rectangle<float> (-10, 6, W + 20, 14), juce::Justification::centred);
    g.setColour (K::cyan); g.drawText ("SAFE", juce::Rectangle<float> (-10, (float) getHeight() - 20, W + 20, 14), juce::Justification::centred);
    g.setColour (K::mint.withAlpha (0.8f)); g.fillRect (b.getX() - 9, b.getCentreY() - 0.5f, 5.0f, 1.0f); g.fillRect (b.getRight() + 4, b.getCentreY() - 0.5f, 5.0f, 1.0f);
    sm += (P.crush.load() - sm) * 0.12f;
    const float y = b.getBottom() - sm * b.getHeight();
    const auto c = sm > 0.7f ? K::red : sm < 0.3f ? K::cyan : K::mint;
    g.setColour (c.withAlpha (0.12f)); g.fillEllipse (b.getCentreX() - 20, y - 20, 40, 40);
    g.setColour (c.withAlpha (0.3f)); g.fillEllipse (b.getCentreX() - 12, y - 12, 24, 24);
    juce::ColourGradient bg (juce::Colours::white, b.getCentreX(), y - 7, c.brighter (0.4f), b.getCentreX(), y + 7, false); g.setGradientFill (bg); g.fillEllipse (b.getCentreX() - 7, y - 7, 14, 14);
}

// ---------------------------------------------------------------------------------------------------- MISURE
void EasyMeters::paint (juce::Graphics& g)
{
    auto& P = page.ed.proc; const auto& T = target ((int) P.value ("target"));
    const float W = (float) getWidth();
    const float S = P.lufsS.load(), I = P.lufsI.load(), M = P.lufsM.load(), tp = P.tpMax.load(), gr = P.grAvg.load();
    big = (big < -90 || S < -69) ? S : big + (S - big) * 0.25f;
    const float d = big - T.lufs; const auto c = S < -69 ? K::faint : std::abs (d) <= 1 ? K::mint : d > 1 ? K::red : K::amber;
    const auto dash = U ("\xe2\x80\x93");
    neo::glowText (g, S < -69 ? dash + dash : juce::String (big, 1), { -3, -6, W + 6, 66 }, c, neo::big (54.0f), juce::Justification::centredLeft, 1.2f);
    g.setFont (cap (8.5f)); g.setColour (K::sub); g.drawText ("LUFS SHORT-TERM", juce::Rectangle<float> (0, 60, W, 12), juce::Justification::centredLeft);
    g.setFont (f (10.0f)); g.setColour (c); g.drawText (S < -69 ? juce::String ("in attesa di audio") : (fmt (d) + " dB dal target " + juce::String (T.lufs, 0)), juce::Rectangle<float> (0, 74, W, 16), juce::Justification::centredLeft);
    auto bar = juce::Rectangle<float> (0, 98, W, 4);
    auto xL = [&bar] (float l) { return bar.getX() + juce::jlimit (0.0f, 1.0f, (l + 30.0f) / 30.0f) * bar.getWidth(); };
    g.setColour (juce::Colours::white.withAlpha (0.07f)); g.fillRoundedRectangle (bar, 2);
    g.setColour (K::mint.withAlpha (0.45f)); g.fillRoundedRectangle (juce::Rectangle<float>::leftTopRightBottom (xL (T.lufs - 1), bar.getY() - 3, xL (T.lufs + 1), bar.getBottom() + 3), 2);
    if (S > -60) { juce::ColourGradient lg (K::blue.withAlpha (0.4f), bar.getX(), 0, c, xL (big), 0, false); g.setGradientFill (lg); g.fillRoundedRectangle (bar.withRight (xL (big)), 2); g.setColour (juce::Colours::white); g.fillEllipse (xL (big) - 4, bar.getCentreY() - 4, 8, 8); }
    struct V { const char* n; juce::String v; juce::Colour c; };
    const float plr = (tp > -60 && I > -60) ? tp - I : -99;
    V vals[] = { { "INTEGRATO", I < -69 ? dash : juce::String (I, 1) + " LUFS", K::ink }, { "MOMENTARY", M < -69 ? dash : juce::String (M, 1) + " LUFS", K::ink },
                 { "TRUE PEAK", tp < -60 ? dash : juce::String (tp, 1) + " dB", tp > T.tp ? K::red : K::ink }, { "DINAMICA (PLR)", plr < -90 ? dash : juce::String (plr, 1) + " dB", plr > 0 && plr < 7 ? K::red : K::ink },
                 { "LIMITER", gr < 0.05f ? juce::String ("0.0 dB") : "-" + juce::String (gr, 1) + " dB", gr > 6 ? K::red : K::pink }, { "LRA", juce::String (P.lra.load(), 1) + " LU", K::ink } };
    for (int i = 0; i < 6; ++i)
    {
        const float y = 120 + i * 28.0f;
        juce::ColourGradient sep (juce::Colours::white.withAlpha (0.08f), 0, y, juce::Colours::white.withAlpha (0.0f), W, y, false); g.setGradientFill (sep); g.fillRect (0.0f, y, W, 1.0f);
        g.setFont (cap (8.0f)); g.setColour (K::faint); g.drawText (vals[i].n, juce::Rectangle<float> (0, y + 1, W, 27), juce::Justification::centredLeft);
        g.setFont (num (12.5f)); g.setColour (vals[i].c); g.drawText (vals[i].v, juce::Rectangle<float> (0, y + 1, W, 27), juce::Justification::centredRight);
    }
    for (int ch = 0; ch < 2; ++ch)
    {
        pk[ch] = juce::jmax (P.outPk[ch].load(), pk[ch] * 0.92f);
        auto r = juce::Rectangle<float> (14, 298 + ch * 10.0f, W - 14, 4);
        g.setColour (juce::Colours::white.withAlpha (0.06f)); g.fillRoundedRectangle (r, 2);
        const float db = juce::Decibels::gainToDecibels (pk[ch], -60.0f), t = juce::jlimit (0.0f, 1.0f, (db + 48.0f) / 48.0f);
        juce::ColourGradient pg (K::blue.withAlpha (0.5f), r.getX(), 0, db > T.tp ? K::red : K::cyan, r.getX() + r.getWidth() * t, 0, false); g.setGradientFill (pg); g.fillRoundedRectangle (r.withWidth (juce::jmax (0.0f, r.getWidth() * t)), 2);
        g.setFont (cap (7.5f)); g.setColour (K::faint); g.drawText (ch ? "R" : "L", juce::Rectangle<float> (0, r.getY() - 4, 10, 12), juce::Justification::centredLeft);
    }
    const float cr = P.crush.load(); juce::String adv; juce::Colour ac = K::faint;
    if (S < -60) adv = "premi play";
    else if (cr > 0.7f) { adv = "troppo schiacciato: abbassa LOUD"; ac = K::red; }
    else if (cr < 0.3f) { adv = "c'e margine: puoi alzare LOUD"; ac = K::cyan; }
    else { adv = "nel punto giusto"; ac = K::mint; }
    neo::glowText (g, adv, { 0, 322, W, 18 }, ac, f (10.5f, 1));
}

// ---------------------------------------------------------------------------------------------------- PALCO
void EasyStage::paint (juce::Graphics& g)
{
    auto p = getLocalBounds().toFloat();
    switch (page.view) { case 1: spectrum (g, p.reduced (0, 8)); break; case 2: gain (g, p.reduced (0, 8)); break; case 3: ref (g, p.reduced (0, 8)); break; case 4: ears (g, p); break; default: wave (g, p); break; }
}
void EasyStage::wave (juce::Graphics& g, juce::Rectangle<float> p)
{
    auto& P = page.ed.proc; const int N = Processor::kHist, w0 = P.hW.load();
    const auto full = p; p = p.withTrimmedRight (40);
    const float cy = p.getCentreY(), h = p.getHeight() * 0.5f - 40;
    auto lin = [] (float db) { return juce::jlimit (0.0f, 1.0f, juce::Decibels::decibelsToGain (db)); };
    const int NX = (int) p.getWidth() / 3 + 1;
    std::vector<float> in ((size_t) NX), out ((size_t) NX), gr ((size_t) NX);
    for (int x = 0; x < NX; ++x)
    {
        const int a = x * N / NX, b = juce::jmax (a + 1, (x + 1) * N / NX);
        float vi = -100, vo = -100, vg = 0;
        for (int k = a; k < b; ++k) { const int i = (w0 + k) % N; vi = juce::jmax (vi, P.hIn[i].load()); vo = juce::jmax (vo, P.hOut[i].load()); vg = juce::jmax (vg, P.hGrM[i].load() + P.hGrL[i].load()); }
        in[(size_t) x] = vi > -60 ? lin (vi) : 0; out[(size_t) x] = vo > -60 ? lin (vo) : 0; gr[(size_t) x] = vg;
    }
    auto smooth = [] (std::vector<float>& v) { auto c = v; for (size_t i = 1; i + 1 < v.size(); ++i) v[i] = 0.25f * c[i - 1] + 0.5f * c[i] + 0.25f * c[i + 1]; };
    smooth (out); smooth (in); smooth (gr);
    // la storia nasce dal silenzio a sinistra: niente cornice, l'onda si assottiglia e sparisce
    for (int x = 0; x < NX; ++x)
    {
        const float u = (float) x / (NX - 1), t = juce::jlimit (0.0f, 1.0f, u / 0.16f), e = t * t * (3 - 2 * t);
        out[(size_t) x] *= e; in[(size_t) x] *= e; gr[(size_t) x] *= e;
    }
    auto X = [&] (int x) { return p.getX() + p.getWidth() * x / (float) (NX - 1); };
    // curva morbida: quadratiche fra i punti medi
    auto curve = [&] (juce::Path& pt, const std::vector<float>& v, float sgn, bool reverse, bool start)
    {
        auto I = [&] (int k) { return reverse ? NX - 1 - k : k; };
        auto Y = [&] (int k) { return cy - sgn * v[(size_t) I (k)] * h; };
        if (start) pt.startNewSubPath (X (I (0)), Y (0)); else pt.lineTo (X (I (0)), Y (0));
        for (int k = 1; k < NX - 1; ++k) pt.quadraticTo (X (I (k)), Y (k), (X (I (k)) + X (I (k + 1))) * 0.5f, (Y (k) + Y (k + 1)) * 0.5f);
        pt.lineTo (X (I (NX - 1)), Y (NX - 1));
    };
    auto body = [&] (const std::vector<float>& v) { juce::Path pt; curve (pt, v, 1, false, true); curve (pt, v, -1, true, false); pt.closeSubPath(); return pt; };
    juce::Path top, bot; curve (top, out, 1, false, true); curve (bot, out, -1, false, true);
    const juce::Path ob = body (out);
    { juce::ColourGradient cg (juce::Colours::white.withAlpha (0.0f), p.getX(), 0, juce::Colours::white.withAlpha (0.07f), p.getX() + p.getWidth() * 0.3f, 0, false); g.setGradientFill (cg); g.fillRect (p.getX(), cy - 0.5f, p.getWidth(), 1.0f); }
    const float cl = lin (P.value ("lm_ceil"));
    for (float s : { -1.0f, 1.0f })
    {
        juce::Path cp; cp.startNewSubPath (p.getX(), cy - s * cl * h); cp.lineTo (p.getRight(), cy - s * cl * h);
        juce::Path dsh; const float dl[] = { 2.0f, 6.0f }; juce::PathStrokeType (1.0f).createDashedStroke (dsh, cp, dl, 2);
        juce::ColourGradient dg (K::pink.withAlpha (0.0f), p.getX(), 0, K::pink.withAlpha (0.4f), p.getX() + p.getWidth() * 0.3f, 0, false); g.setGradientFill (dg); g.fillPath (dsh);
    }
    juce::Path grp; grp.startNewSubPath (p.getX(), cy - h - 10);
    for (int x = 0; x < NX; ++x) grp.lineTo (X (x), cy - h - 10 + juce::jmin (1.0f, gr[(size_t) x] / 12.0f) * h * 0.75f);
    grp.lineTo (p.getRight(), cy - h - 10); grp.closeSubPath();
    // alone vero (bloom)
    bloom.make (full, 4.0f, 3, [&] (juce::Graphics& bg)
    {
        bg.setColour (K::blue.withAlpha (0.22f)); bg.fillPath (ob);
        bg.setColour (K::cyan); bg.strokePath (top, juce::PathStrokeType (6.0f)); bg.strokePath (bot, juce::PathStrokeType (6.0f));
        bg.setColour (K::pink); bg.fillPath (grp);
    });
    bloom.draw (g, 0.9f);
    g.setColour (juce::Colours::white.withAlpha (0.06f)); g.fillPath (body (in));
    {
        juce::ColourGradient og (K::cyan.withAlpha (0.8f), 0, cy - h, K::cyan.withAlpha (0.8f), 0, cy + h, false);
        og.addColour (0.5, K::violet.withAlpha (0.05f)); og.addColour (0.3, K::blue.withAlpha (0.32f)); og.addColour (0.7, K::blue.withAlpha (0.32f));
        g.setGradientFill (og); g.fillPath (ob);
        g.setColour (juce::Colours::white.withAlpha (0.85f)); g.strokePath (top, juce::PathStrokeType (1.1f)); g.strokePath (bot, juce::PathStrokeType (1.1f));
    }
    { juce::ColourGradient gg (K::pink.withAlpha (0.8f), 0, cy - h - 10, K::pink.withAlpha (0.0f), 0, cy - h * 0.25f, false); g.setGradientFill (gg); g.fillPath (grp); }
    { const float nx = p.getRight(); juce::ColourGradient ng (K::cyan.withAlpha (0.35f), nx, cy, K::cyan.withAlpha (0.0f), nx + 34, cy, true); g.setGradientFill (ng); g.fillEllipse (nx - 34, cy - h - 20, 68, 2 * h + 40);
      g.setColour (K::cyan.withAlpha (0.5f)); g.fillRect (nx, cy - h - 6, 1.0f, 2 * h + 12); }
    g.setFont (f (9.0f)); g.setColour (K::faint);
    g.drawText (U ("ultimi 20 s  \xc2\xb7  velo = prima  \xc2\xb7  ciano = dopo  \xc2\xb7  rosa dall'alto = limiter  \xc2\xb7  tratteggio = ceiling"), full.reduced (150, 4), juce::Justification::bottomLeft);
}
void EasyStage::spectrum (juce::Graphics& g, juce::Rectangle<float> p)
{
    const int NP = 300; const double sr = page.ed.proc.sampleRate();
    anIn.run (page.ed.proc.ringIn, specIn, NP, 20, 20000, sr); anOut.run (page.ed.proc.ringOut, specOut, NP, 20, 20000, sr);
    p = p.withTrimmedBottom (16).withTrimmedLeft (90).withTrimmedRight (30);
    auto xOf = [&p] (float fr) { return p.getX() + p.getWidth() * std::log (fr / 20.0f) / std::log (1000.0f); };
    for (float fr : { 50.f, 100.f, 200.f, 500.f, 1000.f, 2000.f, 5000.f, 10000.f })
    { g.setColour (juce::Colours::white.withAlpha (0.035f)); g.fillRect (xOf (fr), p.getY(), 1.0f, p.getHeight()); g.setColour (K::faint); g.setFont (num (8.5f)); g.drawText (fr >= 1000 ? juce::String ((int) fr / 1000) + "k" : juce::String ((int) fr), juce::Rectangle<float> (xOf (fr) - 14, p.getBottom() + 2, 28, 12), juce::Justification::centred); }
    auto path = [&] (const float* s, bool fill)
    {
        juce::Path pt; if (fill) pt.startNewSubPath (p.getX(), p.getBottom());
        for (int i = 0; i < NP; ++i) { const float x = p.getX() + p.getWidth() * (i + 0.5f) / NP, y = p.getBottom() - juce::jlimit (0.0f, 1.0f, (s[i] + 96.0f) / 90.0f) * p.getHeight(); if (i == 0 && ! fill) pt.startNewSubPath (x, y); else pt.lineTo (x, y); }
        if (fill) { pt.lineTo (p.getRight(), p.getBottom()); pt.closeSubPath(); }
        return pt;
    };
    const auto po = path (specOut, false), pf = path (specOut, true);
    bloom.make (p, 4.0f, 4, [&] (juce::Graphics& bg) { bg.setColour (K::cyan); bg.strokePath (po, juce::PathStrokeType (8.0f)); });
    juce::ColourGradient sg (K::violet.withAlpha (0.0f), p.getX(), 0, K::cyan.withAlpha (0.0f), p.getRight(), 0, false);
    sg.addColour (0.12, K::violet.withAlpha (0.38f)); sg.addColour (0.6, K::blue.withAlpha (0.32f)); sg.addColour (0.94, K::cyan.withAlpha (0.25f));
    g.setGradientFill (sg); g.fillPath (pf);
    bloom.draw (g, 0.7f);
    g.setColour (juce::Colours::white.withAlpha (0.35f)); g.strokePath (path (specIn, false), juce::PathStrokeType (1.0f));
    g.setColour (juce::Colours::white); g.strokePath (po, juce::PathStrokeType (1.3f));
    g.setFont (f (9.0f)); g.setColour (K::faint); g.drawText (U ("linea sottile = prima   \xc2\xb7   luce = dopo il master"), p.reduced (10, 8), juce::Justification::bottomLeft);
}
void EasyStage::gain (juce::Graphics& g, juce::Rectangle<float> p)
{
    auto& P = page.ed.proc; const int N = Processor::kHist, w0 = P.hW.load(); const auto& T = target ((int) P.value ("target"));
    p = p.withTrimmedLeft (90).withTrimmedRight (30);
    auto yS = [&p] (float l) { return p.getY() + 20 + (1.0f - juce::jlimit (0.0f, 1.0f, (l + 30.0f) / 30.0f)) * (p.getHeight() * 0.55f); };
    auto yG = [&p] (float gr) { return p.getBottom() - 10 - juce::jlimit (0.0f, 1.0f, gr / 12.0f) * (p.getHeight() * 0.3f); };
    { juce::ColourGradient tl (K::mint.withAlpha (0.0f), p.getX(), 0, K::mint.withAlpha (0.5f), p.getRight(), 0, false); g.setGradientFill (tl); g.fillRect (p.getX(), yS (T.lufs), p.getWidth(), 1.0f); }
    g.setFont (num (9.0f)); g.setColour (K::mint); g.drawText ("target " + juce::String (T.lufs, 0), juce::Rectangle<float> (p.getRight() - 100, yS (T.lufs) - 16, 92, 14), juce::Justification::centredRight);
    juce::Path ls, gl; bool s1 = false;
    for (int k = 0; k < N; k += 2)
    {
        const int i = (w0 + k) % N; const float x = p.getX() + p.getWidth() * k / (N - 1);
        const float s = P.hS[i].load(); if (s > -60) { if (! s1) { ls.startNewSubPath (x, yS (s)); s1 = true; } else ls.lineTo (x, yS (s)); }
        if (k == 0) gl.startNewSubPath (x, yG (0));
        gl.lineTo (x, yG (P.hGrM[i].load() + P.hGrL[i].load()));
    }
    gl.lineTo (p.getRight(), yG (0)); gl.closeSubPath();
    bloom.make (p, 4.0f, 4, [&] (juce::Graphics& bg) { bg.setColour (K::amber); bg.strokePath (ls, juce::PathStrokeType (8.0f)); bg.setColour (K::pink); bg.fillPath (gl); });
    bloom.draw (g, 0.8f);
    juce::ColourGradient gg (K::pink.withAlpha (0.05f), 0, yG (0), K::pink.withAlpha (0.75f), 0, yG (12), false); g.setGradientFill (gg); g.fillPath (gl);
    g.setColour (K::amber.brighter (0.3f)); g.strokePath (ls, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setFont (f (9.0f)); g.setColour (K::faint); g.drawText (U ("ambra = LUFS short nel tempo   \xc2\xb7   rosa = quanto lavora il limiter (fino a 12 dB)"), p.reduced (10, 4), juce::Justification::topLeft);
}
void EasyStage::ref (juce::Graphics& g, juce::Rectangle<float> p)
{
    auto& P = page.ed.proc; auto rt = P.reference();
    loadR = { p.getX() + 140, p.getY() + 6, 130, 26 }; listenR = { p.getX() + 278, p.getY() + 6, 180, 26 };
    chip (g, loadR, P.refLoading.load() ? "ANALISI..." : "CARICA PEZZO", false, K::amber, 9.0f);
    chip (g, listenR, P.refListen.load() ? "ASCOLTI IL RIFERIMENTO" : "ASCOLTA RIFERIMENTO", P.refListen.load(), K::cyan, 9.0f);
    if (! rt) { g.setFont (f (13.0f)); g.setColour (K::sub); g.drawFittedText ("Carica un master che ti piace: confronti spettro e loudness e lo ascolti allo stesso volume.", p.reduced (180, 60).toNearestInt(), juce::Justification::centred, 3); return; }
    g.setFont (f (10.5f, 1)); g.setColour (K::amber); g.drawText (rt->name + "   " + juce::String (rt->lufs, 1) + " LUFS   picco " + juce::String (rt->tp, 1), juce::Rectangle<float> (p.getX() + 470, p.getY() + 6, p.getWidth() - 480, 26), juce::Justification::centredLeft);
    auto pl = p.withTrimmedTop (44).reduced (0, 12).withTrimmedLeft (120);
    auto xOf = [&pl] (int k) { return pl.getX() + pl.getWidth() * (k + 0.5f) / kThirds; };
    auto yOf = [&pl] (float d) { return pl.getCentreY() - juce::jlimit (-15.0f, 15.0f, d) / 15.0f * pl.getHeight() * 0.5f; };
    g.setColour (juce::Colours::white.withAlpha (0.06f)); g.fillRect (pl.getX(), yOf (0), pl.getWidth(), 1.0f);
    juce::Path pr, pm;
    for (int k = 0; k < kThirds; ++k) { const float x = xOf (k); if (k == 0) { pr.startNewSubPath (x, yOf (rt->thirds[k])); pm.startNewSubPath (x, yOf (page.ed.liveThirds[k])); } else { pr.lineTo (x, yOf (rt->thirds[k])); pm.lineTo (x, yOf (page.ed.liveThirds[k])); } }
    const bool live = page.ed.live.hasData();
    bloom.make (p, 4.0f, 4, [&] (juce::Graphics& bg) { bg.setColour (K::amber); bg.strokePath (pr, juce::PathStrokeType (8.0f)); if (live) { bg.setColour (K::cyan); bg.strokePath (pm, juce::PathStrokeType (8.0f)); } });
    bloom.draw (g, 0.7f);
    g.setColour (K::amber); g.strokePath (pr, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    if (live) { g.setColour (K::cyan); g.strokePath (pm, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded)); }
    g.setFont (num (8.0f)); g.setColour (K::faint);
    for (int k = 0; k < kThirds; k += 3) { const float fr = thirdHz (k); g.drawText (fr >= 1000 ? juce::String (fr / 1000, 1) + "k" : juce::String ((int) fr), juce::Rectangle<float> (xOf (k) - 16, pl.getBottom() - 4, 32, 12), juce::Justification::centred); }
}
void EasyStage::ears (juce::Graphics& g, juce::Rectangle<float> p)
{
    const float t = (float) (juce::Time::getMillisecondCounter() % 5000) / 5000.0f, cx = p.getCentreX() + 40, cy = p.getCentreY();
    for (int k = 0; k < 4; ++k) { const float ph = std::fmod (t + k / 4.0f, 1.0f), r = 40 + ph * 200; g.setColour (K::violet.withAlpha ((1 - ph) * 0.28f)); g.drawEllipse (cx - r, cy - r, r * 2, r * 2, 1.5f); }
    g.setFont (neo::big (34.0f)); g.setColour (K::ink.withAlpha (0.9f)); g.drawText ("Ascolta", juce::Rectangle<float> (cx - 200, cy - 26, 400, 44), juce::Justification::centred);
    g.setFont (f (11.0f)); g.setColour (K::sub); g.drawText ("niente grafici: giudica solo con le orecchie", juce::Rectangle<float> (cx - 200, cy + 22, 400, 18), juce::Justification::centred);
}
void EasyStage::mouseDown (const juce::MouseEvent& e)
{
    if (page.view != 3) return;
    auto& P = page.ed.proc;
    if (loadR.contains (e.position))
    {
        page.chooser = std::make_unique<juce::FileChooser> ("Scegli il pezzo di riferimento", juce::File::getSpecialLocation (juce::File::userMusicDirectory), "*.wav;*.aif;*.aiff;*.mp3;*.m4a;*.flac");
        juce::Component::SafePointer<EasyStage> sp (this);
        page.chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [sp] (const juce::FileChooser& c) { if (sp && c.getResult().existsAsFile()) sp->page.ed.proc.loadReference (c.getResult()); });
    }
    else if (listenR.contains (e.position)) P.refListen = ! P.refListen.load();
}
}
