// Modalita FACILE: una scena unica, senza scatole. Onda viva al centro, INPUT a sinistra, barra del risultato e misure a destra,
// sotto un banco di manopole 3D con al centro il LOUD.
#include "MasterEditor.h"

namespace jms
{
using namespace neo;
static juce::String U (const char* s) { return juce::String::fromUTF8 (s); }
static const juce::StringArray& styleNames() { static juce::StringArray a { "MODERN", "AGGRESSIVE", "SMOOTH", "SAFE", "CLASSIC" }; return a; }
static const juce::StringArray& viewNames() { static juce::StringArray a { "ONDA", "SPETTRO", "GAIN", "IDEALE", "REFERENCE" }; return a; }

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
    addChildComponent (guide);
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
    guide.setBounds (20, 42, 1240, 362); guideR = { 24, 8, 104, 28 };
    tabs.layout ({ (float) zStage.getX() + 4, 8, 0, 28 }, viewNames(), 9.0f); tabs.ax = -1;
    cmpR = { (float) zStage.getRight() - 84, 8, 80, 28 }; deltaR = { cmpR.getX() - 76, 8, 70, 28 }; targetR = { deltaR.getX() - 154, 8, 146, 28 };
    { const float w = 330; styles.layout ({ 640 - w * 0.5f, 700, 0, 24 }, styleNames(), 8.0f); const float ww = styles.width(), dx = 640 - ww * 0.5f - styles.r[0].getX(); for (int i = 0; i < styles.n; ++i) styles.r[i].translate (dx, 0); styles.ax = -1; }
    for (int b = 0; b < 3; ++b) bandR[b] = { 58.0f + 78.0f * b - 20, kKnobY + kS + 44, 40, 18 };
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
    chip (g, deltaR, "DELTA", ed.proc.value ("e_delta") > 0.5f, K::amber, 9.0f, hover == 12);
    chip (g, guideR, guide.isVisible() ? U ("CHIUDI GUIDA") : U ("? GUIDA"), guide.isVisible(), K::violet, 9.0f, hover == 13);
    // ascolto delle bande dell'EQ: S = solo la zona, D = solo cosa fa la banda
    const int le = ed.proc.eqListen.load();
    for (int b = 0; b < 3; ++b)
    {
        const bool so = le == b, de = le == b + 3;
        chip (g, bandR[b], so ? "SOLO" : de ? U ("\xce\x94 DELTA") : U ("S / \xce\x94"), so || de, de ? K::amber : K::cyan, 7.5f, hover == 20 + b);
    }
    styles.paint (g, styleNames(), (int) ed.proc.value ("e_style"), K::pink, 8.0f, hover >= 30 ? hover - 30 : -1);
}
void EasyPage::mouseMove (const juce::MouseEvent& e)
{
    int h = tabs.hit (e.position); if (targetR.contains (e.position)) h = 10; if (guideR.contains (e.position)) h = 13; if (cmpR.contains (e.position)) h = 11; if (deltaR.contains (e.position)) h = 12;
    for (int b = 0; b < 3; ++b) if (bandR[b].contains (e.position)) h = 20 + b;
    if (const int sh = styles.hit (e.position); sh >= 0) h = 30 + sh;
    if (h != hover) { hover = h; repaint(); }
    setMouseCursor (h >= 0 ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
}
void EasyPage::mouseDown (const juce::MouseEvent& e)
{
    auto& P = ed.proc;
    if (guideR.contains (e.position)) { guide.setVisible (! guide.isVisible()); if (guide.isVisible()) guide.toFront (false); repaint (0, 0, getWidth(), 44); return; }
    if (const int t = tabs.hit (e.position); t >= 0) { view = t; repaint (0, 0, getWidth(), 44); stage.repaint(); return; }
    if (cmpR.contains (e.position)) { auto* p = ed.P ("compare"); p->setValueNotifyingHost (p->getValue() > 0.5f ? 0.0f : 1.0f); repaint (0, 0, getWidth(), 44); return; }
    if (deltaR.contains (e.position)) { auto* p = ed.P ("e_delta"); p->setValueNotifyingHost (p->getValue() > 0.5f ? 0.0f : 1.0f); repaint (0, 0, getWidth(), 44); return; }
    for (int b = 0; b < 3; ++b)
        if (bandR[b].contains (e.position))
        {   // giro: spento -> SOLO -> DELTA -> spento
            const int le = P.eqListen.load();
            P.eqListen = le == b ? b + 3 : le == b + 3 ? -1 : b; repaint(); return;
        }
    if (const int sh = styles.hit (e.position); sh >= 0) { auto* p = ed.P ("e_style"); p->setValueNotifyingHost (p->convertTo0to1 ((float) sh)); repaint(); return; }
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
    if (! guide.isVisible()) { stage.repaint(); result.repaint(); meters.repaint(); input.repaint(); }
    loud->activity = juce::jlimit (0.0f, 1.0f, ed.proc.grAvg.load() / 6.0f);
    for (auto* k : knobs) if (k->animate()) k->repaint();
    if (tabs.step (view)) repaint (0, 0, getWidth(), 44);
    if (styles.step ((int) ed.proc.value ("e_style"))) repaint (400, 690, 480, 40);
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
    else if (d > 3 && cr > 0.7f) { adv = "+" + juce::String (d, 1) + " dB sopra il target: guida ?"; ac = K::red; }
    else if (cr > 0.7f) { adv = "troppo schiacciato: abbassa LOUD"; ac = K::red; }
    else if (cr < 0.3f) { adv = "c'e margine: puoi alzare LOUD"; ac = K::cyan; }
    else { adv = "nel punto giusto"; ac = K::mint; }
    neo::glowText (g, adv, { 0, 322, W, 18 }, ac, f (10.5f, 1));
}

// ---------------------------------------------------------------------------------------------------- PALCO
int EasyStage::curView() const { return fixedView >= 0 ? fixedView : page.view; }
void EasyStage::paint (juce::Graphics& g)
{
    auto p = getLocalBounds().toFloat();
    // colonna del livello sempre visibile a destra, in tutte le viste
    meterCol (g, p.removeFromRight (52).reduced (4, 10)); p.removeFromRight (4);
    switch (curView()) { case 1: spectrum (g, p.reduced (0, 8)); break; case 2: gain (g, p.reduced (0, 8)); break; case 3: ideal (g, p.reduced (0, 8)); break; case 4: ref (g, p.reduced (0, 8)); break; default: wave (g, p); break; }
}
// scorrimento morbido: fra una colonna e la successiva l'onda si sposta di frazioni di pixel, invece di saltare
float EasyStage::scrollFrac()
{
    const int tot = page.ed.proc.hTot.load(); const double now = juce::Time::getMillisecondCounterHiRes();
    if (tot != lastTot)
    {
        if (lastTot >= 0 && tot > lastTot) colMs = juce::jlimit (8.0, 80.0, colMs * 0.8 + 0.2 * (now - lastMs) / (tot - lastTot));
        lastTot = tot; lastMs = now;
    }
    if (now - lastMs > colMs * 3) return 0.0f;   // nessuna colonna nuova: fermo
    return (float) juce::jlimit (0.0, 1.0, (now - lastMs) / colMs);
}
void EasyStage::meterCol (juce::Graphics& g, juce::Rectangle<float> r)
{   // ampiezza del segnale in uscita: picco (barra chiara + tacca che resta) e medio (barra piena), sinistro e destro
    auto& P = page.ed.proc;
    g.setColour (juce::Colours::black.withAlpha (0.3f)); g.fillRoundedRectangle (r, 6);
    auto bars = r.reduced (4).withTrimmedTop (14).withTrimmedBottom (16);
    auto yOf = [&bars] (float db) { return bars.getBottom() - juce::jlimit (0.0f, 1.0f, (db + 48.0f) / 48.0f) * bars.getHeight(); };
    g.setFont (num (7.5f));
    for (int d : { 0, -6, -12, -24, -36 }) { g.setColour (juce::Colours::white.withAlpha (0.08f)); g.fillRect (bars.getX(), yOf ((float) d), bars.getWidth(), 1.0f); }
    const float bw = (bars.getWidth() - 4) / 2;
    for (int c = 0; c < 2; ++c)
    {
        const float pk = juce::Decibels::gainToDecibels (P.outPk[c].load(), -100.0f), rm = juce::Decibels::gainToDecibels (std::sqrt (P.outRms[c].load()), -100.0f);
        if (pk >= pkHold[c]) { pkHold[c] = pk; holdT[c] = 45; } else if (--holdT[c] <= 0) pkHold[c] -= 0.6f;
        auto col = juce::Rectangle<float> (bars.getX() + c * (bw + 4), bars.getY(), bw, bars.getHeight());
        g.setColour (juce::Colours::white.withAlpha (0.05f)); g.fillRect (col);
        const auto cc = pk > -1.0f ? K::red : pk > -6.0f ? K::amber : K::cyan;
        g.setColour (cc.withAlpha (0.35f)); g.fillRect (col.withTop (yOf (pk)));
        g.setColour (cc); g.fillRect (col.withTop (yOf (rm)));
        g.setColour (pkHold[c] > -1.0f ? K::red : K::ink); g.fillRect (col.getX(), yOf (pkHold[c]) - 1, col.getWidth(), 2.0f);
    }
    g.setColour (K::sub); g.drawText ("L  R", r.withHeight (12).translated (0, 2), juce::Justification::centred);
    const float mx = juce::jmax (pkHold[0], pkHold[1]);
    g.setColour (mx > -1.0f ? K::red : K::ink); g.drawText (mx < -90 ? juce::String ("-inf") : juce::String (mx, 1), juce::Rectangle<float> (r.getX(), r.getBottom() - 14, r.getWidth(), 12), juce::Justification::centred);
}
void EasyStage::ideal (juce::Graphics& g, juce::Rectangle<float> p)
{   // IDEALE contro MIO: la fascia del genere (verde) e lo spettro medio del tuo master (ambra), come nel Metering Hub
    auto& E = page.ed;
    if (genreSel < 0) { const int t = (int) E.proc.value ("target"); genreSel = t == 5 || t == 6 ? 0 : 0; }
    const auto& G = genre (genreSel);
    p = p.withTrimmedLeft (34).withTrimmedRight (8).withTrimmedBottom (18).withTrimmedTop (28);
    g.setColour (juce::Colours::black.withAlpha (0.28f)); g.fillRoundedRectangle (p.expanded (4), 8);
    auto xOf = [&p] (float f) { return p.getX() + p.getWidth() * (float) (std::log (f / 20.0) / std::log (1000.0)); };
    auto yOf = [&p] (float d) { return p.getCentreY() - juce::jlimit (-1.0f, 1.0f, d / 15.0f) * p.getHeight() * 0.5f; };
    g.setFont (num (8.0f));
    for (float f : { 50.f, 100.f, 200.f, 500.f, 1000.f, 2000.f, 5000.f, 10000.f })
    { g.setColour (juce::Colours::white.withAlpha (0.05f)); g.fillRect (xOf (f), p.getY(), 1.0f, p.getHeight()); g.setColour (K::faint); g.drawText (f >= 1000 ? juce::String ((int) (f / 1000)) + "k" : juce::String ((int) f), juce::Rectangle<float> (xOf (f) - 14, p.getBottom() + 3, 28, 11), juce::Justification::centred); }
    for (int d : { -10, -5, 0, 5, 10 }) { g.setColour (juce::Colours::white.withAlpha (d == 0 ? 0.1f : 0.04f)); g.fillRect (p.getX(), yOf ((float) d), p.getWidth(), 1.0f); g.setColour (K::faint); g.drawText ((d > 0 ? "+" : "") + juce::String (d), juce::Rectangle<float> (p.getX() - 32, yOf ((float) d) - 6, 26, 12), juce::Justification::centredRight); }
    juce::Path band; const int n = 90;
    for (int i = 0; i <= n; ++i) { const float f = 20.0f * std::pow (1000.0f, i / (float) n); const float y = yOf (targetAt (G.off, f) + 2.5f); if (i == 0) band.startNewSubPath (xOf (f), y); else band.lineTo (xOf (f), y); }
    for (int i = n; i >= 0; --i) { const float f = 20.0f * std::pow (1000.0f, i / (float) n); band.lineTo (xOf (f), yOf (targetAt (G.off, f) - 2.5f)); }
    band.closeSubPath(); g.setColour (K::mint.withAlpha (0.14f)); g.fillPath (band); g.setColour (K::mint.withAlpha (0.5f)); g.strokePath (band, juce::PathStrokeType (1.0f));
    if (E.live.hasData())
    {
        juce::Path my;
        for (int k = 0; k < kThirds; ++k) { const float x = xOf (thirdHz (k)), y = yOf (E.liveThirds[k]); if (k == 0) my.startNewSubPath (x, y); else my.lineTo (x, y); }
        g.setColour (K::amber.withAlpha (0.3f)); g.strokePath (my, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (K::amber); g.strokePath (my, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        // scarti per regione sotto il grafico
        float reg[kRegions]; ThirdAnalyzer::regions (E.liveThirds, reg); float m = 0; for (float o : G.off) m += o; m /= kRegions;
        g.setFont (cap (7.5f));
        for (int k = 0; k < kRegions; ++k)
        {
            const float d = reg[k] - (G.off[k] - m), x0 = xOf ((float) regionEdge (k)), x1 = xOf ((float) regionEdge (k + 1));
            const auto c = std::abs (d) < 2.5f ? K::mint : d > 0 ? K::amber : K::cyan;
            g.setColour (c); g.drawText (juce::String (regionName (k)) + " " + (d > 0 ? "+" : "") + juce::String (d, 1), juce::Rectangle<float> (x0, p.getY() + 4, x1 - x0, 12), juce::Justification::centred);
        }
    }
    else { g.setFont (f (12.0f)); g.setColour (K::sub); g.drawText ("premi play: misuro lo spettro medio del tuo master", p, juce::Justification::centred); }
    genreR = { p.getX(), p.getY() - 26, 220, 20 };
    chip (g, genreR, juce::String ("GENERE: ") + G.name + juce::String::fromUTF8 (" \xe2\x96\xbe"), true, K::mint, 8.0f);
    g.setFont (f (9.0f)); g.setColour (K::faint);
    g.drawText (juce::String::fromUTF8 ("verde = ideale per il genere (\xc2\xb1 2.5 dB)  \xc2\xb7  ambra = il tuo master"), juce::Rectangle<float> (p.getX() + 230, p.getY() - 26, p.getWidth() - 230, 20), juce::Justification::centredRight);
}
// tempo sull'asse orizzontale: gli ultimi 20 s, "ora" a destra
static void timeAxis (juce::Graphics& g, juce::Rectangle<float> p, float y)
{
    g.setFont (num (8.5f));
    for (int s = 0; s <= 20; s += 2)
    {
        const float x = p.getRight() - p.getWidth() * s / 20.0f;
        g.setColour (juce::Colours::white.withAlpha (s % 10 == 0 ? 0.08f : 0.04f)); g.fillRect (x, p.getY(), 1.0f, y - p.getY());
        g.setColour (K::faint); g.drawText (s == 0 ? juce::String ("ora") : "-" + juce::String (s) + "s", juce::Rectangle<float> (x - 18, y + 2, 36, 12), juce::Justification::centred);
    }
}
// una colonna di storia = 25 ms, disegnata sempre allo stesso passo: l'onda scorre senza ricampionare, quindi non vibra
struct Cols
{
    int N, w0; float x0, dx, fr;
    Cols (int n, int w, juce::Rectangle<float> p, float frac = 0) : N (n), w0 (w), x0 (p.getX()), dx (p.getWidth() / (float) n), fr (frac) {}
    int idx (int x) const { return (w0 + x) % N; }          // x = 0 la colonna piu vecchia, N-1 la piu nuova
    float X (int x) const { return x0 + dx * ((float) x - fr); }
};
// onda simmetrica in stile DAW: picco (chiaro) e RMS (pieno) dalla stessa linea di centro
static void drawWav (juce::Graphics& g, const Cols& C, juce::Rectangle<float> lane, std::atomic<float>* pk, std::atomic<float>* rm, juce::Colour c, float scale)
{
    const float cy = lane.getCentreY(), h = lane.getHeight() * 0.5f - 2;
    auto lin = [scale] (float db) { return db < -80 ? 0.0f : juce::jlimit (0.0f, 1.0f, juce::Decibels::decibelsToGain (db) * scale); };
    juce::RectangleList<float> rp, rr;
    for (int x = 0; x < C.N; ++x)
    {
        const int i = C.idx (x); const float a = lin (pk[i].load()) * h, r = juce::jmin (a, lin (rm[i].load()) * h);
        if (a > 0.3f) rp.addWithoutMerging ({ C.X (x), cy - a, C.dx + 0.35f, a * 2 });
        if (r > 0.3f) rr.addWithoutMerging ({ C.X (x), cy - r, C.dx + 0.35f, r * 2 });
    }
    g.setColour (c.withAlpha (0.45f)); g.fillRectList (rp);
    g.setColour (c); g.fillRectList (rr);
}
void EasyStage::wave (juce::Graphics& g, juce::Rectangle<float> p)
{   // due corsie, stessa scala: sopra il wav cosi' com'e', sotto dopo il master
    auto& P = page.ed.proc;
    p = p.withTrimmedLeft (8).withTrimmedRight (8).withTrimmedBottom (18).withTrimmedTop (6);
    const Cols C (Processor::kHist, P.hW.load(), p, scrollFrac());
    timeAxis (g, p, p.getBottom());
    g.saveState(); g.reduceClipRegion (p.toNearestInt());
    auto top = p.removeFromTop (p.getHeight() * 0.5f - 3); p.removeFromTop (6); auto bot = p;
    for (auto* l : { &top, &bot })
    {
        g.setColour (juce::Colours::black.withAlpha (0.28f)); g.fillRoundedRectangle (*l, 6);
        g.setColour (juce::Colours::white.withAlpha (0.07f)); g.fillRect (l->getX(), l->getCentreY() - 0.5f, l->getWidth(), 1.0f);
    }
    drawWav (g, C, top, P.hRaw, P.hRawR, juce::Colour (0xffc9cede), 1.0f);
    drawWav (g, C, bot, P.hOut, P.hOutR, juce::Colour (0xff3d9df0), 1.0f);
    // ceiling sulla corsia del master
    const float cl = juce::Decibels::decibelsToGain (P.value ("lm_ceil")), hh = bot.getHeight() * 0.5f - 2;
    g.setColour (K::red.withAlpha (0.55f)); g.fillRect (bot.getX(), bot.getCentreY() - cl * hh - 0.5f, bot.getWidth(), 1.0f); g.fillRect (bot.getX(), bot.getCentreY() + cl * hh - 0.5f, bot.getWidth(), 1.0f);
    g.restoreState();
    g.setFont (cap (8.5f));
    g.setColour (K::sub); g.drawText ("SENZA MASTER", top.reduced (10, 6), juce::Justification::topLeft);
    g.setColour (juce::Colour (0xff8fd3ff)); g.drawText ("CON IL MASTER", bot.reduced (10, 6), juce::Justification::topLeft);
    g.setFont (f (9.0f)); g.setColour (K::faint);
    g.drawText (U ("chiaro = picchi  \xc2\xb7  pieno = volume medio (RMS)  \xc2\xb7  linea rossa = ceiling"), bot.reduced (10, 6), juce::Justification::bottomRight);
}
void EasyStage::spectrum (juce::Graphics& g, juce::Rectangle<float> p)
{
    const int NP = 300; const double sr = page.ed.proc.sampleRate();
    anIn.run (page.ed.proc.ringIn, specIn, NP, 20, 20000, sr); anOut.run (page.ed.proc.ringOut, specOut, NP, 20, 20000, sr);
    p = p.withTrimmedBottom (16).withTrimmedLeft (40).withTrimmedRight (10);
    auto xOf = [&p] (float fr) { return p.getX() + p.getWidth() * std::log (fr / 20.0f) / std::log (1000.0f); };
    auto yOf = [&p] (float db) { return p.getBottom() - juce::jlimit (0.0f, 1.0f, (db + 96.0f) / 90.0f) * p.getHeight(); };
    g.setFont (num (8.5f));
    for (int d = -90; d <= -6; d += 12) { g.setColour (juce::Colours::white.withAlpha (0.04f)); g.fillRect (p.getX(), yOf ((float) d), p.getWidth(), 1.0f); g.setColour (K::faint); g.drawText (juce::String (d), juce::Rectangle<float> (p.getX() - 38, yOf ((float) d) - 6, 32, 12), juce::Justification::centredRight); }
    for (float fr : { 50.f, 100.f, 200.f, 500.f, 1000.f, 2000.f, 5000.f, 10000.f })
    { g.setColour (juce::Colours::white.withAlpha (0.04f)); g.fillRect (xOf (fr), p.getY(), 1.0f, p.getHeight()); g.setColour (K::faint); g.drawText (fr >= 1000 ? juce::String ((int) fr / 1000) + "k" : juce::String ((int) fr), juce::Rectangle<float> (xOf (fr) - 14, p.getBottom() + 2, 28, 12), juce::Justification::centred); }
    auto path = [&] (const float* s, bool fill)
    {
        juce::Path pt; if (fill) pt.startNewSubPath (p.getX(), p.getBottom());
        for (int i = 0; i < NP; ++i) { const float x = p.getX() + p.getWidth() * (i + 0.5f) / NP, y = yOf (s[i]); if (i == 0 && ! fill) pt.startNewSubPath (x, y); else pt.lineTo (x, y); }
        if (fill) { pt.lineTo (p.getRight(), p.getBottom()); pt.closeSubPath(); }
        return pt;
    };
    g.setColour (juce::Colour (0xff2f8fd8).withAlpha (0.35f)); g.fillPath (path (specOut, true));
    g.setColour (juce::Colour (0xffc9cede).withAlpha (0.5f)); g.strokePath (path (specIn, false), juce::PathStrokeType (1.0f));
    g.setColour (juce::Colour (0xff8fd3ff)); g.strokePath (path (specOut, false), juce::PathStrokeType (1.3f));
    g.setFont (f (9.0f)); g.setColour (K::faint); g.drawText (U ("grigio = prima   \xc2\xb7   azzurro = dopo il master"), p.reduced (10, 6), juce::Justification::topRight);
}
void EasyStage::gain (juce::Graphics& g, juce::Rectangle<float> p)
{   // come nei limiter da master: mezza onda in dB dal basso, il taglio del limiter in rosso sopra i picchi,
    // e in alto la riduzione appesa alla linea dello zero. Sotto i picchi piu forti, quanti dB sta togliendo.
    auto& P = page.ed.proc;
    p = p.withTrimmedLeft (40).withTrimmedRight (8).withTrimmedBottom (18).withTrimmedTop (4);
    const Cols C (Processor::kHist, P.hW.load(), p, scrollFrac());
    timeAxis (g, p, p.getBottom());
    g.saveState(); g.reduceClipRegion (p.toNearestInt());
    auto grLane = p.removeFromTop (64); p.removeFromTop (8);
    // sopra lo zero c'e' spazio: il rosso mostra dove arrivava il picco prima del limiter
    auto yD = [&p] (float db) { return p.getBottom() - juce::jlimit (0.0f, 1.0f, (db + 30.0f) / 39.0f) * p.getHeight(); };
    g.setFont (num (8.5f));
    g.setColour (juce::Colours::white.withAlpha (0.12f)); g.fillRect (p.getX(), yD (0), p.getWidth(), 1.0f);
    for (int d = -30; d <= 6; d += 6) { g.setColour (juce::Colours::white.withAlpha (0.05f)); g.fillRect (p.getX(), yD ((float) d), p.getWidth(), 1.0f); g.setColour (K::faint); g.drawText (juce::String (d), juce::Rectangle<float> (p.getX() - 38, yD ((float) d) - 6, 32, 12), juce::Justification::centredRight); }
    juce::RectangleList<float> ro, rr, rc; juce::RectangleList<float> rg;
    auto yG = [&grLane] (float d) { return grLane.getY() + juce::jlimit (0.0f, 1.0f, d / 12.0f) * grLane.getHeight(); };
    float grMax = 0;
    for (int x = 0; x < C.N; ++x)
    {
        const int i = C.idx (x); const float o = P.hOut[i].load(), r = P.hOutR[i].load(), gr = P.hGrM[i].load() + P.hGrL[i].load();
        const float xx = C.X (x), w = C.dx + 0.35f;
        if (o > -30) ro.addWithoutMerging (juce::Rectangle<float>::leftTopRightBottom (xx, yD (o), xx + w, p.getBottom()));
        if (r > -30) rr.addWithoutMerging (juce::Rectangle<float>::leftTopRightBottom (xx, yD (r), xx + w, p.getBottom()));
        if (o > -30 && gr > 0.05f) rc.addWithoutMerging (juce::Rectangle<float>::leftTopRightBottom (xx, yD (o + gr), xx + w, yD (o)));
        if (gr > 0.05f) { rg.addWithoutMerging (juce::Rectangle<float>::leftTopRightBottom (xx, grLane.getY(), xx + w, yG (gr))); grMax = juce::jmax (grMax, gr); }
    }
    g.setColour (juce::Colour (0xff3d9df0).withAlpha (0.45f)); g.fillRectList (ro);
    g.setColour (juce::Colour (0xff3d9df0)); g.fillRectList (rr);
    g.setColour (K::red.withAlpha (0.85f)); g.fillRectList (rc);
    // corsia della riduzione
    g.setColour (juce::Colours::black.withAlpha (0.28f)); g.fillRoundedRectangle (grLane, 6);
    for (float d : { 3.0f, 6.0f, 9.0f }) { g.setColour (juce::Colours::white.withAlpha (0.06f)); g.fillRect (grLane.getX(), yG (d), grLane.getWidth(), 1.0f); g.setColour (K::faint); g.drawText ("-" + juce::String ((int) d), juce::Rectangle<float> (grLane.getX() - 38, yG (d) - 6, 32, 12), juce::Justification::centredRight); }
    g.setColour (K::red.withAlpha (0.8f)); g.fillRectList (rg);
    g.setFont (cap (8.0f)); g.setColour (K::sub); g.drawText ("RIDUZIONE DEL LIMITER", grLane.reduced (8, 4), juce::Justification::bottomRight);
    // numeri sotto i picchi: a blocchi fissi di 1 s (40 colonne), ancorati al tempo, cosi' scorrono con l'onda
    const int tot = P.hTot.load(), B = 40;
    g.setFont (num (9.0f));
    for (int x = 0; x < C.N; )
    {
        const int abs0 = tot - C.N + x; int len = B - (((abs0 % B) + B) % B); len = juce::jmin (len, C.N - x);
        float bg = 0, bo = -100; int bx = x;
        for (int k = x; k < x + len; ++k) { const int i = C.idx (k); const float gr = P.hGrM[i].load() + P.hGrL[i].load(); if (gr > bg) { bg = gr; bx = k; bo = P.hOut[i].load(); } }
        if (bg >= 0.5f && len == B && x > 0 && x + len < C.N && bo > -30)
        {
            const auto t = "-" + juce::String (bg, 1);
            const float tx = C.X (bx), ty = yD (bo) + 6;
            g.setColour (juce::Colours::black.withAlpha (0.55f)); g.fillRoundedRectangle (tx - 16, ty, 32, 14, 3);
            g.setColour (bg > 6 ? K::red : K::ink); g.drawText (t, juce::Rectangle<float> (tx - 16, ty, 32, 14), juce::Justification::centred);
        }
        x += len;
    }
    g.restoreState();
    const float cl = P.value ("lm_ceil");
    g.setColour (K::red.withAlpha (0.5f)); g.fillRect (p.getX(), yD (cl), p.getWidth(), 1.0f);
    g.setFont (f (9.0f)); g.setColour (K::faint);
    g.drawText (U ("azzurro = livello che esce (pieno = medio)  \xc2\xb7  rosso = quanto il limiter toglie a quel picco  \xc2\xb7  max ") + juce::String (grMax, 1) + " dB",
                p.reduced (8, 4), juce::Justification::bottomLeft);
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
    g.setColour (K::amber); g.strokePath (pr, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    if (live) { g.setColour (K::cyan); g.strokePath (pm, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded)); }
    g.setFont (num (8.0f)); g.setColour (K::faint);
    for (int k = 0; k < kThirds; k += 3) { const float fr = thirdHz (k); g.drawText (fr >= 1000 ? juce::String (fr / 1000, 1) + "k" : juce::String ((int) fr), juce::Rectangle<float> (xOf (k) - 16, pl.getBottom() - 4, 32, 12), juce::Justification::centred); }
}
void EasyStage::mouseDown (const juce::MouseEvent& e)
{
    if (curView() == 3 && genreR.contains (e.position))
    {
        juce::PopupMenu m; for (int k = 0; k < kGenres; ++k) m.addItem (k + 1, genre (k).name, true, k == genreSel);
        juce::Component::SafePointer<EasyStage> sp (this);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [sp] (int r) { if (r > 0 && sp) { sp->genreSel = r - 1; sp->repaint(); } });
        return;
    }
    if (curView() != 4) return;
    auto& P = page.ed.proc;
    if (loadR.contains (e.position))
    {
        page.chooser = std::make_unique<juce::FileChooser> ("Scegli il pezzo di riferimento", juce::File::getSpecialLocation (juce::File::userMusicDirectory), "*.wav;*.aif;*.aiff;*.mp3;*.m4a;*.flac");
        juce::Component::SafePointer<EasyStage> sp (this);
        page.chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [sp] (const juce::FileChooser& c) { if (sp && c.getResult().existsAsFile()) sp->page.ed.proc.loadReference (c.getResult()); });
    }
    else if (listenR.contains (e.position)) P.refListen = ! P.refListen.load();
}

// ---------------------------------------------------------------------------------------------------- GUIDA
namespace
{
struct Topic { const char* title; const char* body; };
const Topic kTopics[] = {
    { "LUFS E dB",
      "I dB (dBFS) misurano l'ampiezza del segnale in un istante: \xc3\xa8 il picco. 0 dBFS \xc3\xa8 il massimo che il file digitale pu\xc3\xb2 contenere.\n\n"
      "I LUFS misurano quanto forte il pezzo SEMBRA all'orecchio, in media nel tempo. Pesano di pi\xc3\xb9 le frequenze a cui l'orecchio \xc3\xa8 sensibile (medie e medio-alte) e meno i bassissimi: un 808 enorme ha picchi alti ma pochi LUFS, una voce brillante tanti LUFS.\n\n"
      "1 LU di differenza = 1 dB di volume.\n\n"
      "MOMENTARY: ultimi 0.4 s (segue i colpi).   SHORT-TERM: ultimi 3 s (il numero grande, per lavorare).   INTEGRATO: tutto il pezzo dall'inizio (\xc3\xa8 quello che usano Spotify, Apple, YouTube). Per leggere l'integrato giusto fai suonare il brano intero." },
    { "SPOTIFY E LA NORMALIZZAZIONE",
      "Spotify misura il loudness INTEGRATO di tutto il brano e lo porta a -14 LUFS.\n\n"
      "Se il tuo master \xc3\xa8 a -8.4 LUFS, Spotify lo abbassa di 5.6 dB. Solo volume gi\xc3\xb9: niente limiter, niente distorsione. Quindi su Spotify suona forte quanto gli altri, ma con la dinamica che hai tolto per arrivare a -8.4. Se \xc3\xa8 sotto -14 lo alza solo se c'\xc3\xa8 margine sui picchi (true peak).\n\n"
      "Apple Music normalizza a -16, YouTube a -14 (solo abbassa), Tidal a -14. SoundCloud non normalizza.\n\n"
      "In pratica: nell'hip hop e nella trap \xc3\xa8 normale stare tra -9 e -7 LUFS integrati, lo fanno quasi tutti, perch\xc3\xa9 il carattere schiacciato fa parte del genere e nei club e DJ set non c'\xc3\xa8 normalizzazione. Basta sapere che sopra -14 quel volume in pi\xc3\xb9 su Spotify non lo senti: conta solo come suona il pezzo a pari volume. Usa il tasto confronto (BYPASS) a pari volume per giudicare." },
    { "TRUE PEAK E CEILING",
      "Il picco dei campioni non \xc3\xa8 il picco vero: fra un campione e l'altro l'onda ricostruita dal convertitore pu\xc3\xb2 salire sopra lo 0 (inter-sample peak).\n\n"
      "Quando Spotify o YouTube convertono il wav in OGG / AAC / MP3 i picchi crescono ancora. Per questo per lo streaming si tiene il CEILING a -1 dBTP (true peak). Per club e CD si pu\xc3\xb2 stare a -0.3.\n\n"
      "Il TRUE PEAK nelle misure deve restare sotto il tuo ceiling: se diventa rosso, abbassa il ceiling o il LOUD." },
    { "DINAMICA: PLR E LRA",
      "PLR (Peak to Loudness Ratio) = true peak meno LUFS integrati. Dice quanto spazio hanno i colpi sopra il corpo del pezzo.\n\n"
      "  sopra 10: aperto e dinamico   \xc2\xb7   8-10: moderno, pieno   \xc2\xb7   6-8: molto schiacciato (trap forte, EDM)   \xc2\xb7   sotto 6: i colpi non respirano, la cassa si appiattisce.\n\n"
      "LRA (Loudness Range) = quanto cambia il volume fra le parti del pezzo (strofa, ritornello). Nel rap 4-7 LU \xc3\xa8 normale.\n\n"
      "La barra RISULTATO (SAFE - CRUSH) mette insieme: quanto sei sopra il target scelto, quanto lavora il limiter e il PLR. In mezzo (menta) \xc3\xa8 il punto giusto per quel target. Se punti alla trap forte scegli il target TRAP FORTE, non SPOTIFY: altrimenti ti dira' sempre CRUSH, giustamente." },
    { "CORRELAZIONE E FASE",
      "La correlazione confronta canale sinistro e destro.\n\n"
      "  +1 = identici (mono)   \xc2\xb7   fra +1 e 0 = stereo sano   \xc2\xb7   0 = molto largo   \xc2\xb7   sotto 0 = i due lati si cancellano quando il pezzo va in mono (telefono, casse bluetooth, club in mono).\n\n"
      "Che gli ALTI stiano vicino a 0 \xc3\xa8 normale: hi-hat, riverberi, pad e cori sono larghi. Il problema \xc3\xa8 se restano SOTTO 0. Cause tipiche: widener o chorus spinti, effetto Haas (un lato ritardato), sample stereo con un canale invertito di fase, doppie registrazioni troppo simili.\n\n"
      "Come si corregge: ascolta in mono e trova la traccia che sparisce o diventa sottile (mettila in solo). Su quella riduci il widener o togli l'Haas; se \xc3\xa8 un sample, mettilo mono o inverti un canale. Sul master puoi stringere solo gli alti con l'imager (WIDE sotto il 100%). I bassi sotto 120 Hz devono stare mono: BASSI MONO acceso." },
    { "COME SI FA UN MASTER",
      "1. Parti da un mix con margine: picchi intorno a -6 / -3 dBFS, niente limiter sul bus del mix.\n"
      "2. Scegli un riferimento (REFERENCE) di un pezzo del tuo genere e ascoltalo allo stesso volume.\n"
      "3. Porta l'ingresso nella zona ottimale (INPUT, tasto AUTO).\n"
      "4. EQ con mano leggera: 1-2 dB bastano. Prima togli quello che da' fastidio (CALM), poi aggiungi.\n"
      "5. Colore (THICK / WARM / SILK) e GLUE: pochi punti, devi sentirlo appena.\n"
      "6. LOUD fino al target scelto. Guarda il GAIN: se il limiter toglie spesso pi\xc3\xb9 di 4-6 dB ai colpi, torna indietro o lavora sul mix (cassa e 808).\n"
      "7. Ceiling -1 dBTP per lo streaming.\n"
      "8. Confronta sempre a pari volume (BYPASS): pi\xc3\xb9 forte sembra sempre meglio, \xc3\xa8 un inganno.\n"
      "9. Controlla in mono, su cuffie, telefono e auto. Fai suonare tutto il brano per leggere l'integrato.\n"
      "10. Esporta in wav 24 bit (o 16 bit con dither attivo) alla stessa frequenza del progetto." },
    { "LEGGERE QUESTO PLUGIN",
      "ONDA: sopra il wav cos\xc3\xac com'\xc3\xa8 entra, sotto dopo il master, stessa scala. Chiaro = picchi, pieno = volume medio. Se sotto il pieno diventa un blocco piatto contro la linea rossa, stai schiacciando.\n\n"
      "GAIN: come nei limiter da master. Azzurro = livello in uscita in dB, rosso sopra = quanto il limiter ha tolto a quel picco, in alto la riduzione nel tempo. I numeri sotto i picchi dicono quanti dB toglie sul colpo pi\xc3\xb9 forte di ogni secondo.\n\n"
      "SPETTRO: grigio prima, azzurro dopo. REFERENCE: il tuo pezzo contro quello caricato.\n\n"
      "INPUT a sinistra: quanto entra. RISULTATO: SAFE - CRUSH. A destra le misure: il numero grande \xc3\xa8 lo short-term, sotto la distanza dal target." } };
constexpr int kNumTopics = (int) (sizeof (kTopics) / sizeof (kTopics[0]));
}
void GuidePanel::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xf20a0b14)); g.fillRoundedRectangle (b, 12);
    g.setColour (K::violet.withAlpha (0.5f)); g.drawRoundedRectangle (b.reduced (0.5f), 12, 1.0f);
    auto left = b.reduced (16).removeFromLeft (250);
    g.setFont (cap (9.0f)); g.setColour (K::violet); g.drawText ("GUIDA RAPIDA", left.removeFromTop (20), juce::Justification::centredLeft);
    left.removeFromTop (6);
    for (int i = 0; i < kNumTopics && i < 8; ++i)
    {
        topicR[i] = left.removeFromTop (36).reduced (0, 3);
        const bool on = i == topic;
        g.setColour (on ? K::violet.withAlpha (0.2f) : juce::Colours::white.withAlpha (hover == i ? 0.06f : 0.02f)); g.fillRoundedRectangle (topicR[i], 6);
        if (on) { g.setColour (K::violet); g.fillRoundedRectangle (topicR[i].withWidth (3), 1.5f); }
        g.setFont (cap (9.5f)); g.setColour (on ? K::ink : K::sub); g.drawText (kTopics[i].title, topicR[i].reduced (14, 0), juce::Justification::centredLeft);
    }
    closeR = { b.getRight() - 44, 12, 30, 26 };
    chip (g, closeR, U ("\xc3\x97"), false, K::violet, 11.0f);
    auto r = b.reduced (16).withTrimmedLeft (280).withTrimmedRight (40);
    g.setFont (f (17.0f, 1)); g.setColour (K::ink); g.drawText (kTopics[topic].title, r.removeFromTop (28), juce::Justification::centredLeft);
    r.removeFromTop (8);
    juce::AttributedString as; as.setJustification (juce::Justification::topLeft); as.setLineSpacing (3.0f);
    as.append (juce::String::fromUTF8 (kTopics[topic].body), f (12.5f), K::ink.withAlpha (0.86f));
    juce::TextLayout tl; tl.createLayout (as, r.getWidth()); tl.draw (g, r);
}
void GuidePanel::mouseMove (const juce::MouseEvent& e)
{
    int h = -1; for (int i = 0; i < kNumTopics && i < 8; ++i) if (topicR[i].contains (e.position)) h = i;
    if (h != hover) { hover = h; repaint(); }
}
void GuidePanel::mouseDown (const juce::MouseEvent& e)
{
    if (closeR.contains (e.position)) { setVisible (false); if (auto* p = getParentComponent()) p->repaint (0, 0, p->getWidth(), 44); return; }
    for (int i = 0; i < kNumTopics && i < 8; ++i) if (topicR[i].contains (e.position)) { topic = i; repaint(); }
}
}
