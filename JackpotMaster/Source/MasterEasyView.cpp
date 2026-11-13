// Modalita FACILE in stile Master Plan: un display largo in alto (onda, gain, spettro, immagine, metering) con i numeri
// sovrapposti, il LOUD a cavallo fra display e banco, sotto le manopole. Due temi: scuro elegante e chiaro.
#include "MasterEditor.h"

namespace jms
{
using namespace neo;
static juce::String U (const char* s) { return juce::String::fromUTF8 (s); }
static const juce::StringArray& styleNames() { static juce::StringArray a { "MODERN", "AGGRESSIVE", "SMOOTH", "SAFE", "CLASSIC" }; return a; }
static const juce::StringArray& viewNames() { static juce::StringArray a { "ONDA", "GAIN", "SPETTRO", "IMMAGINE", "METERING", "IDEALE", "REFERENCE" }; return a; }
static const double kWins[4] = { 2, 4, 8, 16 };

// zone (coordinate della pagina, 1280 x 736)
static const juce::Rectangle<int> zDisp { 16, 40, 1248, 418 };
static constexpr float kPanelY = 466, kCapY = 478, kKnobY = 504, kS = 64;
static juce::Colour waveCol() { return light() ? juce::Colour (0xff6b3fd6) : juce::Colour (0xffd88cff).interpolatedWith (juce::Colour (0xffff7ab6), 0.35f); }

EasyPage::EasyPage (Editor& e) : ed (e)
{
    auto mk = [this] (const char* id, const char* label, juce::Colour a, juce::Colour b, bool bip, std::function<juce::String()> t) -> neo::Knob*
    {
        auto* k = knobs.add (new neo::Knob (ed.P (id), label, a, b)); k->bipolar = bip; k->text = std::move (t); addAndMakeVisible (k); return k;
    };
    const juce::Colour P0 { 0xffff3d9a }, V0 { 0xff8b5cff }, C0 { 0xff3ee8ff }, M0 { 0xff4dffb4 }, A0 { 0xffffb547 }, R0 { 0xffff4d6a }, B0 { 0xff4d7dff };
    auto db = [this] (const char* id) { return [this, id] { return fmt (ed.proc.value (id)) + " dB"; }; };
    auto pc = [this] (const char* id) { return [this, id] { return juce::String (juce::roundToInt (ed.proc.value (id) * 100)) + "%"; }; };
    low  = mk ("e_low", "BASSI", A0, P0, true, db ("e_low"));   low->setTooltip ("Shelf morbido sotto i 90 Hz: peso e sub.");
    mid  = mk ("e_mid", "MEDI", A0, P0, true, db ("e_mid"));    mid->setTooltip ("Campana larga a 1.2 kHz: presenza di voce e rullante.");
    high = mk ("e_high", "ALTI", A0, P0, true, db ("e_high"));  high->setTooltip ("Shelf d'aria sopra i 9 kHz.");
    thick = mk ("e_thick", "THICK", P0, V0, false, pc ("e_thick")); thick->setTooltip ("Valvola solo sui medio-bassi: spessore e corpo, a volume uguale.");
    warm  = mk ("e_warm", "WARM", P0, V0, false, pc ("e_warm"));    warm->setTooltip ("Nastro da 15 pollici: compatta e arrotonda.");
    silk  = mk ("e_silk", "SILK", P0, V0, false, pc ("e_silk"));    silk->setTooltip ("Armoniche solo sopra i 4.5 kHz: lucentezza senza durezza.");
    loud  = mk ("e_loud", "LOUD", P0, C0, false, [this] { return "+" + juce::String (ed.proc.value ("e_loud"), 1); }); loud->big = true; loud->sub = U ("dB di volume in pi\xc3\xb9");
    loud->setTooltip ("Quanto alzi il volume. La corona esterna mostra quanto lavora il limiter.");
    pres  = mk ("e_pres", "PRESENZA", P0, R0, false, [this] { const float v = ed.proc.value ("e_pres"); return v < 0.3f ? juce::String ("SOFT") : v < 0.7f ? juce::String ("NATURAL") : juce::String ("HARD"); });
    pres->setTooltip ("Come il limiter tratta i picchi: morbido (arrotonda) o duro (clipper, piu volume e grinta).");
    speed = mk ("e_speed", "VELOCITA", V0, C0, false, [this] { const float v = ed.proc.value ("e_speed"); return v < 0.33f ? juce::String ("SMOOTH") : v < 0.66f ? juce::String ("MEDIUM") : juce::String ("FAST"); });
    speed->setTooltip ("Rilascio del limiter: smooth (morbido) o fast (piu volume, piu movimento).");
    glue = mk ("e_glue", "GLUE", V0, B0, false, pc ("e_glue")); glue->setTooltip ("Compressore di bus 2:1 con rilascio automatico: tiene insieme il mix.");
    calm = mk ("e_calm", "CALM", V0, B0, false, pc ("e_calm")); calm->setTooltip ("Abbassa da solo le risonanze che saltano fuori: fango, durezze, sibilanti.");
    wide = mk ("e_wide", "WIDE", B0, C0, true, [this] { const float w = ed.proc.value ("e_wide"); return juce::String (juce::roundToInt (100 + w * (w > 0 ? 80 : 100))) + "%"; });
    wide->setTooltip ("Allarga o stringe lo stereo sopra 120 Hz.");
    ceil = mk ("lm_ceil", "CEILING", M0, C0, false, [this] { return juce::String (ed.proc.value ("lm_ceil"), 1) + " dB"; }); ceil->setTooltip ("Picco massimo in uscita (true peak).");
    trim = mk ("trim", "IN", C0, B0, true, [this] { return fmt (ed.proc.value ("trim")) + " dB"; }); trim->setTooltip ("Livello d'ingresso. AUTO lo porta nella zona ottimale.");
    monoB = std::make_unique<neo::Toggle> (ed.P ("e_monob"), "BASSI MONO", C0); addAndMakeVisible (*monoB);
    addAndMakeVisible (stage); stage.toBack();
    addAndMakeVisible (input);
    addChildComponent (guide);
    trim->toFront (false); input.toFront (false); loud->toFront (false); pres->toFront (false); speed->toFront (false);
}

void EasyPage::resized()
{
    auto kb = [] (neo::Knob* k, float cx, float top, float s) { const float w = juce::jmax (s + 10, 84.0f); k->setBounds (juce::Rectangle<float> (cx - w * 0.5f, top, w, s + 38).expanded ((float) neo::Knob::pad).toNearestInt()); };
    kb (low, 70, kKnobY, kS); kb (mid, 146, kKnobY, kS); kb (high, 222, kKnobY, kS);
    kb (thick, 330, kKnobY, kS); kb (warm, 406, kKnobY, kS); kb (silk, 482, kKnobY, kS);
    loud->setBounds (juce::Rectangle<int> (520, 376, 240, 228).expanded (neo::Knob::pad));
    kb (pres, 560, 616, 42); kb (speed, 720, 616, 42);
    kb (glue, 806, kKnobY, kS); kb (calm, 882, kKnobY, kS); kb (wide, 958, kKnobY, kS);
    kb (ceil, 1064, kKnobY, kS);
    monoB->setBounds (1120, (int) kKnobY + 20, 124, 26);
    kb (trim, (float) zDisp.getX() + 48, (float) zDisp.getY() + 6, 36);
    stage.setBounds (zDisp); input.setBounds (zDisp.getX() + 92, zDisp.getY() + 10, 250, 46); stage.optLeft = 352;
    guide.setBounds (20, 42, 1240, 362); guideR = { 16, 6, 92, 26 };
    tabs.layout ({ 124, 6, 0, 26 }, viewNames(), 8.5f); tabs.ax = -1;
    themeR = { 1182, 6, 82, 26 }; cmpR = { themeR.getX() - 84, 6, 78, 26 }; deltaR = { cmpR.getX() - 72, 6, 66, 26 }; targetR = { deltaR.getX() - 150, 6, 144, 26 };
    { styles.layout ({ 0, 700, 0, 24 }, styleNames(), 8.0f); const float ww = styles.width(), dx = 640 - ww * 0.5f - styles.r[0].getX(); for (int i = 0; i < styles.n; ++i) styles.r[i].translate (dx, 0); styles.ax = -1; }
    for (int b = 0; b < 3; ++b) bandR[b] = { 70.0f + 76.0f * b - 20, kKnobY + kS + 44, 40, 18 };
    loudDR = { 604, 676, 72, 22 };   // DELTA della LOUDNESS: senti solo cosa toglie/sporca il limiter
    bg = {};
}

// ---------------------------------------------------------------------------------------------------- statico (in cache)
void EasyPage::paintStatic (juce::Graphics& g)
{
    // banco delle manopole: una lastra unica, piu chiara del display
    auto pnl = juce::Rectangle<float> (16, kPanelY - 22, 1248, (float) getHeight() - kPanelY + 16);
    g.setColour (K::lo.withAlpha (light() ? 0.10f : 0.35f)); g.fillRoundedRectangle (pnl.translated (0, 4), 22);
    juce::ColourGradient pg (K::panel0, 0, pnl.getY(), K::panel1, 0, pnl.getBottom(), false); g.setGradientFill (pg); g.fillRoundedRectangle (pnl, 22);
    g.setColour (K::hi.withAlpha (light() ? 0.10f : 0.06f)); g.drawRoundedRectangle (pnl.reduced (0.5f), 22, 1.0f);
    { juce::ColourGradient hl (K::hi.withAlpha (0.0f), pnl.getX() + 40, 0, K::hi.withAlpha (0.0f), pnl.getRight() - 40, 0, false); hl.addColour (0.5, (light() ? juce::Colours::white : K::hi).withAlpha (light() ? 0.9f : 0.14f));
      g.setGradientFill (hl); g.fillRect (pnl.getX() + 40, pnl.getY() + 1, pnl.getWidth() - 80, 1.0f); }
    // didascalie dei gruppi: punto colorato + parola
    auto capt = [&] (float x, const char* t, juce::Colour c)
    {
        g.setColour (c.withAlpha (0.3f)); g.fillEllipse (x - 3, kCapY + 1, 10, 10); g.setColour (c); g.fillEllipse (x, kCapY + 4, 4, 4);
        g.setFont (cap (8.5f)); g.setColour (K::sub); g.drawText (t, juce::Rectangle<float> (x + 12, kCapY, 200, 12), juce::Justification::centredLeft);
    };
    capt (36, "EQ", K::amber); capt (296, "COLORE", K::pink); capt (772, "DINAMICA E SPAZIO", K::violet); capt (1030, "USCITA", K::mint);
}
void EasyPage::paint (juce::Graphics& g)
{
    neo::setTheme (ed.proc.uiTheme.load() == 2 ? 2 : 1);
    const float sc = juce::jlimit (1.0f, 4.0f, g.getInternalContext().getPhysicalPixelScaleFactor());
    if (bg.isNull() || std::abs (bgScale - sc) > 0.01f || bgTheme != K::theme)
    {
        bgScale = sc; bgTheme = K::theme; bg = juce::Image (juce::Image::ARGB, juce::roundToInt (getWidth() * sc), juce::roundToInt (getHeight() * sc), true);
        juce::Graphics ig (bg); ig.addTransform (juce::AffineTransform::scale (sc)); paintStatic (ig);
    }
    { g.saveState(); g.addTransform (juce::AffineTransform::scale (1.0f / sc)); g.drawImageAt (bg, 0, 0); g.restoreState(); }
    tabs.paint (g, viewNames(), view, K::violet, 8.5f, hover);
    const auto& T = target (juce::roundToInt (ed.proc.value ("target")));
    chip (g, targetR, juce::String (T.name) + "  " + juce::String (T.lufs, 0) + U ("  \xe2\x96\xbe"), true, K::mint, 8.5f, hover == 10);
    const bool cmp = ed.proc.value ("compare") > 0.5f;
    chip (g, cmpR, cmp ? "ORIGINALE" : "BYPASS", cmp, K::cyan, 8.5f, hover == 11);
    chip (g, deltaR, "DELTA", ed.proc.value ("e_delta") > 0.5f, K::amber, 8.5f, hover == 12);
    chip (g, loudDR, U ("\xce\x94 LOUD"), ed.proc.value ("e_ldelta") > 0.5f, K::amber, 7.5f, hover == 14);
    chip (g, guideR, guide.isVisible() ? U ("CHIUDI") : U ("? GUIDA"), guide.isVisible(), K::violet, 8.5f, hover == 13);
    chip (g, themeR, light() ? U ("CHIARO") : U ("SCURO"), false, K::sub, 8.5f, hover == 14);
    // ascolto delle bande dell'EQ: S = solo la zona, D = solo cosa fa la banda
    const int le = ed.proc.eqListen.load();
    for (int b = 0; b < 3; ++b)
    {
        const bool so = le == b, de = le == b + 3;
        chip (g, bandR[b], so ? "SOLO" : de ? U ("\xce\x94 DELTA") : U ("S / \xce\x94"), so || de, de ? K::amber : K::cyan, 7.5f, hover == 20 + b);
    }
    styles.paint (g, styleNames(), juce::roundToInt (ed.proc.value ("e_style")), K::pink, 8.0f, hover >= 30 ? hover - 30 : -1);
}
void EasyPage::mouseMove (const juce::MouseEvent& e)
{
    int h = tabs.hit (e.position); if (targetR.contains (e.position)) h = 10; if (guideR.contains (e.position)) h = 13; if (cmpR.contains (e.position)) h = 11; if (deltaR.contains (e.position)) h = 12; if (loudDR.contains (e.position)) h = 14;
    if (themeR.contains (e.position)) h = 14;
    for (int b = 0; b < 3; ++b) if (bandR[b].contains (e.position)) h = 20 + b;
    if (const int sh = styles.hit (e.position); sh >= 0) h = 30 + sh;
    if (h != hover) { hover = h; repaint(); }
    setMouseCursor (h >= 0 ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
}
void EasyPage::mouseDown (const juce::MouseEvent& e)
{
    auto& P = ed.proc;
    if (guideR.contains (e.position)) { guide.setVisible (! guide.isVisible()); if (guide.isVisible()) guide.toFront (false); repaint (0, 0, getWidth(), 40); return; }
    if (themeR.contains (e.position)) { P.uiTheme = P.uiTheme.load() == 2 ? 1 : 2; ed.neoBg = {}; ed.repaint(); return; }
    if (const int t = tabs.hit (e.position); t >= 0) { view = t; repaint (0, 0, getWidth(), 40); stage.repaint(); return; }
    if (cmpR.contains (e.position)) { auto* p = ed.P ("compare"); p->setValueNotifyingHost (p->getValue() > 0.5f ? 0.0f : 1.0f); repaint (0, 0, getWidth(), 40); return; }
    if (loudDR.contains (e.position)) { auto* p = ed.P ("e_ldelta"); p->setValueNotifyingHost (p->getValue() > 0.5f ? 0.0f : 1.0f); repaint(); return; }
    if (deltaR.contains (e.position)) { auto* p = ed.P ("e_delta"); p->setValueNotifyingHost (p->getValue() > 0.5f ? 0.0f : 1.0f); repaint (0, 0, getWidth(), 40); return; }
    for (int b = 0; b < 3; ++b)
        if (bandR[b].contains (e.position))
        {   // giro: spento -> SOLO -> DELTA -> spento
            const int le = P.eqListen.load();
            P.eqListen = le == b ? b + 3 : le == b + 3 ? -1 : b; repaint(); return;
        }
    if (const int sh = styles.hit (e.position); sh >= 0) { auto* p = ed.P ("e_style"); p->setValueNotifyingHost (p->convertTo0to1 ((float) sh)); repaint(); return; }
    if (targetR.contains (e.position))
    {
        juce::PopupMenu m; const int cur = juce::roundToInt (P.value ("target"));
        for (int i = 0; i < kNumTargets; ++i) m.addItem (i + 1, juce::String (target (i).name) + "   " + juce::String (target (i).lufs, 0) + " LUFS", true, i == cur);
        auto* tp = ed.P ("target"); juce::Component::SafePointer<EasyPage> sp (this);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [tp, sp] (int r) { if (r > 0) { tp->setValueNotifyingHost (tp->convertTo0to1 ((float) (r - 1))); if (sp) sp->repaint(); } });
    }
}
void EasyPage::tick()
{
    if (! guide.isVisible()) input.repaint();
    loud->activity = juce::jlimit (0.0f, 1.0f, ed.proc.grAvg.load() / 6.0f);
    for (auto* k : knobs) if (k->animate()) k->repaint();
    if (tabs.step (view)) repaint (0, 0, getWidth(), 40);
    if (styles.step (juce::roundToInt (ed.proc.value ("e_style")))) repaint (400, 690, 480, 40);
    monoB->repaint();
}

// ---------------------------------------------------------------------------------------------------- INPUT (in orizzontale)
void EasyInput::paint (juce::Graphics& g)
{
    auto& P = page.ed.proc;
    const float W = (float) getWidth();
    g.setFont (cap (7.5f)); g.setColour (K::sub); g.drawText ("INGRESSO", juce::Rectangle<float> (0, 0, 120, 12), juce::Justification::centredLeft);
    auto m = juce::Rectangle<float> (0, 17, W - 66, 7);
    auto xL = [&m] (float db) { return m.getX() + juce::jlimit (0.0f, 1.0f, (db + 36.0f) / 36.0f) * m.getWidth(); };
    const float lo = Processor::kInOpt - 2.5f, hi = Processor::kInOpt + 2.5f, s = P.inS.load(), a = P.inAvg.load();
    sm = sm < -90 ? s : sm + (s - sm) * 0.35f;
    g.setColour (K::lo.withAlpha (light() ? 0.10f : 0.45f)); g.fillRoundedRectangle (m.expanded (1.5f), 4.5f);
    auto zone = juce::Rectangle<float>::leftTopRightBottom (xL (lo), m.getY() - 4, xL (hi), m.getBottom() + 4);
    g.setColour (K::mint.withAlpha (0.22f)); g.fillRoundedRectangle (zone, 3); g.setColour (K::mint.withAlpha (0.8f)); g.fillRect (zone.getX(), zone.getY(), 1.5f, zone.getHeight()); g.fillRect (zone.getRight() - 1.5f, zone.getY(), 1.5f, zone.getHeight());
    if (sm > -60)
    {
        const auto top = sm > hi ? K::red : (sm >= lo ? K::mint : K::cyan);
        juce::ColourGradient lg (K::blue.withAlpha (0.7f), m.getX(), 0, top, xL (sm), 0, false); g.setGradientFill (lg); g.fillRoundedRectangle (m.withRight (xL (sm)), 3.5f);
    }
    if (a > -60) { g.setColour (K::ink); g.fillRoundedRectangle (xL (a) - 1, m.getY() - 5, 2, m.getHeight() + 10, 1); }
    juce::String st = "PLAY"; juce::Colour c = K::faint;
    if (a > -60) { if (a > hi) { st = "ABBASSA"; c = K::red; } else if (a < lo) { st = "ALZA"; c = K::amber; } else { st = "PERFETTO"; c = K::mint; } }
    g.setFont (cap (7.5f)); g.setColour (c); g.drawText (st, juce::Rectangle<float> (0, 30, 120, 12), juce::Justification::centredLeft);
    g.setFont (num (7.5f)); g.setColour (K::faint); g.drawText (sm > -60 ? juce::String (sm, 1) + " LUFS" : juce::String ("--"), juce::Rectangle<float> (m.getRight() - 90, 30, 90, 12), juce::Justification::centredRight);
    autoR = { W - 58, 10, 56, 22 };
    chip (g, autoR, "AUTO", false, K::cyan, 8.0f, isMouseOver() && autoR.contains (getMouseXYRelative().toFloat()));
}
void EasyInput::mouseDown (const juce::MouseEvent& e) { if (autoR.contains (e.position)) { page.ed.proc.autoInput(); repaint(); } }

// ---------------------------------------------------------------------------------------------------- DISPLAY
EasyStage::EasyStage (EasyPage& p) : page (p)
{
    // ridisegno a ogni refresh dello schermo (massimo ~60 al secondo): scorrimento fluido, niente scatti del timer
    vb = std::make_unique<juce::VBlankAttachment> (this, [this]
    {
        const double now = juce::Time::getMillisecondCounterHiRes();
        if (isShowing() && now - lastFrame > 14.0) { lastFrame = now; repaint(); }
    });
}
int EasyStage::curView() const { return fixedView >= 0 ? fixedView : page.view; }
static bool hasCol (int v) { return v == V_WAVE || v == V_GAIN || v == V_SPEC; }
juce::Rectangle<float> EasyStage::visual()
{
    auto b = getLocalBounds().toFloat();
    b = easy() ? b.withTrimmedTop (62).reduced (18, 0).withTrimmedBottom (10) : b.withTrimmedTop (36).reduced (4, 0).withTrimmedBottom (4);
    if (hasCol (curView())) b.removeFromRight (94);
    return b;
}
double gStageT[6] {};
void EasyStage::paint (juce::Graphics& g)
{
    neo::setTheme (page.ed.proc.uiTheme.load() == 2 ? 2 : 1);
    hots.clear();
    if (bare)
    {
        auto b = getLocalBounds().toFloat(); lcd (g, b, 14);
        auto& P = page.ed.proc; const auto v = b.reduced (12, 10);
        if (curView() == V_IMAGE) image (g, v, v.getWidth() < 420);
        else wave (g, v, curView() != V_GAIN, kWins[juce::jlimit (0, 3, P.uiWin.load())]);
        return;
    }
    double tA = juce::Time::getMillisecondCounterHiRes();
    auto lap = [&tA] (int k) { const double n = juce::Time::getMillisecondCounterHiRes(); gStageT[k] += n - tA; tA = n; };
    panel (g); lap (0);
    const auto v = visual(); const int cv = curView(); auto& P = page.ed.proc;
    switch (cv)
    {
        case V_WAVE:  wave (g, v, true, kWins[juce::jlimit (0, 3, P.uiWin.load())]); break;
        case V_GAIN:  wave (g, v, false, kWins[juce::jlimit (0, 3, P.uiWin.load())]); break;
        case V_SPEC:  spectrum (g, v, P.uiSpec.load()); break;
        case V_IMAGE: image (g, v); break;
        case V_METER: metering (g, v); break;
        case V_IDEAL: ideal (g, v.withTrimmedBottom (easy() ? 70 : 0)); break;
        default:      ref (g, v.withTrimmedBottom (easy() ? 70 : 0)); break;
    }
    lap (1);
    if (hasCol (cv))
    {
        auto c = getLocalBounds().toFloat(); c = easy() ? c.withTrimmedTop (62).withTrimmedBottom (86) : c.withTrimmedTop (36).withTrimmedBottom (4);
        meterCol (g, c.removeFromRight (easy() ? 108 : 92).withTrimmedRight (easy() ? 18 : 4).withTrimmedLeft (4));
    }
    lap (2);
    if (easy() && cv != V_METER) numbers (g);
    lap (3);
    options (g); lap (4);
}
void EasyStage::panel (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    if (! easy()) { g.setColour (K::lo.withAlpha (0.22f)); g.fillRoundedRectangle (b, 10); return; }
    // fondo del display e velo dei numeri: fermi, quindi in cache (a ogni frame solo un'immagine)
    const float sc = juce::jlimit (1.0f, 4.0f, g.getInternalContext().getPhysicalPixelScaleFactor());
    if (panelImg.isNull() || panelKey != juce::String (K::theme) + "/" + juce::String (sc, 2) + "/" + juce::String (getWidth()))
    {
        panelKey = juce::String (K::theme) + "/" + juce::String (sc, 2) + "/" + juce::String (getWidth());
        panelImg = juce::Image (juce::Image::ARGB, juce::roundToInt (getWidth() * sc), juce::roundToInt (getHeight() * sc), true);
        veilImg = juce::Image (juce::Image::ARGB, juce::roundToInt (getWidth() * sc), juce::roundToInt (110 * sc), true);
        { juce::Graphics pg (panelImg); pg.addTransform (juce::AffineTransform::scale (sc)); panelDraw (pg); }
        { juce::Graphics vg2 (veilImg); vg2.addTransform (juce::AffineTransform::translation (0, -(b.getBottom() - 110)).scaled (sc));
          const float y0 = b.getBottom() - 78;
          juce::ColourGradient vg (K::disp1.withAlpha (0.0f), 0, y0 - 30, K::disp1.withAlpha (light() ? 0.92f : 0.88f), 0, y0 + 26, false); vg2.setGradientFill (vg);
          vg2.fillRoundedRectangle (b.getX() + 1, y0 - 30, b.getWidth() - 2, b.getBottom() - y0 + 29, 21); }
    }
    // copia veloce (senza ricampionamento di qualita): prima questa riga da sola pesava piu di meta del fotogramma
    g.setImageResamplingQuality (juce::Graphics::lowResamplingQuality); g.drawImageTransformed (panelImg, juce::AffineTransform::scale (1.0f / sc));
}
void EasyStage::panelDraw (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (K::lo.withAlpha (light() ? 0.10f : 0.4f)); g.fillRoundedRectangle (b.translated (0, 4), 22);
    juce::ColourGradient dg (K::disp0, 0, 0, K::disp1, 0, b.getBottom(), false); g.setGradientFill (dg); g.fillRoundedRectangle (b, 22);
    // velo colorato morbido al centro, come una luce dietro al vetro
    { juce::ColourGradient cg (waveCol().withAlpha (light() ? 0.10f : 0.13f), b.getCentreX(), b.getY() + 40, waveCol().withAlpha (0.0f), b.getCentreX() + b.getWidth() * 0.55f, b.getY() + 40, true); g.setGradientFill (cg); g.fillRoundedRectangle (b, 22); }
    g.setColour (K::hi.withAlpha (light() ? 0.10f : 0.07f)); g.drawRoundedRectangle (b.reduced (0.5f), 22, 1.0f);
    g.setColour (K::hi.withAlpha (light() ? 0.06f : 0.05f)); g.fillRect (b.getX() + 22, 56.0f, b.getWidth() - 44, 1.0f);
}
void EasyStage::opt (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& t, bool on, int id)
{
    chip (g, r, t, on, K::violet, 7.5f, hov == id); hots.push_back ({ r, id });
}
void EasyStage::options (juce::Graphics& g)
{   // opzioni della vista, al centro della testata
    auto& P = page.ed.proc; const int cv = curView();
    const float y = easy() ? 18.0f : 6.0f, h = 22;
    std::vector<std::pair<juce::String, std::pair<bool, int>>> it;
    auto add = [&] (const juce::String& s, bool on, int id) { it.push_back ({ s, { on, id } }); };
    auto gap = [&] { it.push_back ({ "|", { false, 0 } }); };
    if (cv == V_WAVE || cv == V_GAIN)
    {
        add ("SCROLL", P.uiScan.load() == 0, 1); add ("SCAN", P.uiScan.load() == 1, 2); gap();
        for (int k = 0; k < 4; ++k) add (juce::String ((int) kWins[k]) + " s", P.uiWin.load() == k, 10 + k);
    }
    else if (cv == V_SPEC)
    {
        add ("CURVA", P.uiSpec.load() == 0, 20); add ("1/3 OTT", P.uiSpec.load() == 1, 21); add ("BARK", P.uiSpec.load() == 2, 22); gap();
        add ("NORMALIZZA", P.uiNorm.load() == 1, 30); gap();
        add ("VELOCE", P.uiRel.load() == 0, 40); add ("MEDIO", P.uiRel.load() == 1, 41); add ("LENTO", P.uiRel.load() == 2, 42); gap();
        add ("PRIMA", P.uiPrev.load() == 1, 50);
    }
    if (it.empty()) return;
    // i tasti stanno a destra del riquadro INGRESSO; se non ci stanno si stringono (prima si sovrapponevano)
    float fs = 7.5f, pad = 22, gapW = 10;
    auto total = [&] { float t = 0; for (auto& e : it) t += e.first == "|" ? gapW : juce::GlyphArrangement::getStringWidth (cap (fs), e.first) + pad + 4; return t; };
    const float left = juce::jmax (8.0f, optLeft), avail = (float) getWidth() - 10 - left;
    float tot = total();
    if (tot > avail) { pad = 12; gapW = 6; tot = total(); }
    if (tot > avail) { fs = 6.6f; tot = total(); }
    float x = juce::jmax (left, (float) getWidth() * 0.5f - tot * 0.5f);
    if (x + tot > (float) getWidth() - 10) x = juce::jmax (left, (float) getWidth() - 10 - tot);
    for (auto& e : it)
    {
        if (e.first == "|") { g.setColour (K::hi.withAlpha (0.12f)); g.fillRect (x + gapW * 0.5f - 1, y + 4, 1.0f, h - 8); x += gapW; continue; }
        const float w = juce::GlyphArrangement::getStringWidth (cap (fs), e.first) + pad;
        chip (g, { x, y, w, h }, e.first, e.second.first, K::violet, fs, hov == e.second.second); hots.push_back ({ { x, y, w, h }, e.second.second }); x += w + 4;
    }
}
void EasyStage::mouseMove (const juce::MouseEvent& e)
{
    int h = -1; for (auto& t : hots) if (t.r.contains (e.position)) h = t.id;
    if (curView() == V_IDEAL && genreR.contains (e.position)) h = 99;
    if (curView() == V_REF && (loadR.contains (e.position) || listenR.contains (e.position))) h = 98;
    hov = h; setMouseCursor (h >= 0 ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
}
void EasyStage::mouseDown (const juce::MouseEvent& e)
{
    auto& P = page.ed.proc;
    for (auto& t : hots)
        if (t.r.contains (e.position))
        {
            const int id = t.id;
            if (id == 1 || id == 2) { P.uiScan = id - 1; disp = -1; }
            else if (id >= 10 && id < 14) { P.uiWin = id - 10; disp = -1; }
            else if (id >= 20 && id < 23) P.uiSpec = id - 20;
            else if (id == 30) P.uiNorm = 1 - P.uiNorm.load();
            else if (id >= 40 && id < 43) P.uiRel = id - 40;
            else if (id == 50) P.uiPrev = 1 - P.uiPrev.load();
            repaint(); return;
        }
    if (curView() == V_IDEAL && genreR.contains (e.position))
    {
        juce::PopupMenu m; for (int k = 0; k < kGenres; ++k) m.addItem (k + 1, genre (k).name, true, k == genreSel);
        juce::Component::SafePointer<EasyStage> sp (this);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [sp] (int r) { if (r > 0 && sp) { sp->genreSel = r - 1; sp->repaint(); } });
        return;
    }
    if (curView() != V_REF) return;
    if (loadR.contains (e.position))
    {
        page.chooser = std::make_unique<juce::FileChooser> ("Scegli il pezzo di riferimento", juce::File::getSpecialLocation (juce::File::userMusicDirectory), "*.wav;*.aif;*.aiff;*.mp3;*.m4a;*.flac");
        juce::Component::SafePointer<EasyStage> sp (this);
        page.chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [sp] (const juce::FileChooser& c) { if (sp && c.getResult().existsAsFile()) sp->page.ed.proc.loadReference (c.getResult()); });
    }
    else if (listenR.contains (e.position)) P.refListen = ! P.refListen.load();
}

// ---------------------------------------------------------------------------------------------------- tempo
// la posizione mostrata avanza col tempo reale e insegue piano i dati, che arrivano a pacchetti dalla DAW
double EasyStage::clock()
{
    auto& P = page.ed.proc;
    const int tot = P.hTot.load(); const double now = juce::Time::getMillisecondCounterHiRes();
    const double colLen = Processor::kColSec * juce::jmax (8000.0, P.sampleRate());
    const double lag = std::ceil (P.lastBlock.load() / colLen) + 1.0;
    const double target = tot - 1 - lag;
    if (disp < 0 || lastT <= 0 || std::abs (target - disp) > 400) { disp = target; lastT = now; return disp; }
    const double dt = juce::jlimit (0.0, 200.0, now - lastT); lastT = now;
    disp += dt / (Processor::kColSec * 1000.0);
    disp += (target - disp) * 0.05;
    disp = juce::jmin (disp, (double) tot - 1);
    return disp;
}
void EasyStage::envelope (PxEnv& e, int npx, double winSec)
{
    auto& P = page.ed.proc; const int N = Processor::kHist, tot = P.hTot.load(), w0 = P.hW.load();
    e.size (npx);
    const double pos = clock(), cols = winSec / Processor::kColSec, cpp = cols / npx;
    const bool scan = P.uiScan.load() == 1;
    const double sweep0 = std::floor (pos / cols) * cols;
    e.shift = scan ? 0.0f : (float) ((pos - std::floor (pos / cpp) * cpp) / cpp);
    e.g0 = (long) std::floor (pos / cpp);
    if (scan) e.cursor = (float) ((pos - sweep0) / cpp);
    auto ok = [&] (long a) { return a <= tot - 1 && a > tot - N + 2; };
    auto ring = [&] (long a) { const long back = tot - a; return (int) (((w0 - back) % N + N) % N); };
    const int num = juce::jmax (1, P.hostNum.load());
    for (int x = 0; x < npx; ++x)
    {
        double a0;
        if (scan)
        {
            const double k = x * cpp; a0 = sweep0 + k;
            if (a0 > pos) { if (a0 - pos < cols * 0.012) continue; a0 -= cols; }   // poco dopo il cursore: vuoto
        }
        // SCROLL rigido: ogni pixel e' agganciato a una griglia fissa nel tempo, cosi' il suo valore non cambia mentre scorre;
        // il movimento fine (meno di un pixel) lo fa il disegno con e.shift. Prima la forma si ricalcolava a ogni fotogramma e tremava
        else a0 = std::floor (pos / cpp) * cpp - (npx - x) * cpp;
        const double a1 = a0 + cpp;
        float pk = -200, rm = -200, gr = 0, in = -200;
        if (cpp >= 1.0)
        {
            for (long a = (long) std::ceil (a0); a < (long) std::ceil (a1); ++a)
            {
                if (! ok (a)) continue; const int i = ring (a);
                pk = juce::jmax (pk, P.hOut[i].load()); rm = juce::jmax (rm, P.hOutR[i].load()); in = juce::jmax (in, P.hRaw[i].load()); gr = juce::jmax (gr, P.hGrM[i].load() + P.hGrL[i].load());
            }
        }
        else
        {   // zoom: fra due colonne interpolo, cosi' la linea resta morbida
            const double c = 0.5 * (a0 + a1); const long a = (long) std::floor (c); const float t = (float) (c - a);
            if (ok (a) && ok (a + 1))
            {
                const int i = ring (a), j = ring (a + 1);
                pk = juce::jmap (t, P.hOut[i].load(), P.hOut[j].load()); in = juce::jmap (t, P.hRaw[i].load(), P.hRaw[j].load()); rm = juce::jmap (t, P.hOutR[i].load(), P.hOutR[j].load());
                gr = juce::jmap (t, P.hGrM[i].load() + P.hGrL[i].load(), P.hGrM[j].load() + P.hGrL[j].load());
            }
        }
        e.pk[(size_t) x] = pk; e.rm[(size_t) x] = rm; e.in[(size_t) x] = in; e.gr[(size_t) x] = gr;
        // battito che cade in questo pixel
        const long ac = (long) std::floor (a1);
        if (ok (ac) && ok (ac - 1) && cpp > 0)
        {
            const float q1 = P.hPpq[ring (ac)].load(), q0 = P.hPpq[ring ((long) std::floor (a0))].load();
            if (q1 >= 0 && q0 >= 0 && q1 >= q0 && std::floor (q1) != std::floor (q0)) { const int beat = (int) std::floor (q1); e.beat[(size_t) x] = beat % num == 0 ? 100000 + beat / num + 1 : 1; }
        }
    }
}

// ---------------------------------------------------------------------------------------------------- ONDA / GAIN
void EasyStage::wave (juce::Graphics& g, juce::Rectangle<float> p, bool bipolar, double win, bool small)
{
    auto& P = page.ed.proc;
    // scala in dB fissa a lato (non scorre): 0 in alto, verso il basso il silenzio
    juce::Rectangle<float> scaleR; if (! small) scaleR = p.removeFromLeft (30);
    const int npx = juce::jmax (16, (int) p.getWidth());
    PxEnv e; envelope (e, npx, win);
    const float dx = p.getWidth() / (float) npx;
    const bool scan = P.uiScan.load() == 1;
    // limiter e maximizer: rosso/rosa sul tema scuro, blu sul chiaro (il rosso sul chiaro stancava)
    const auto wc = tint.isTransparent() ? waveCol() : (light() ? juce::Colour (0xff3a63c4) : tint), lineC = light() ? K::ink : juce::Colours::white;
    // due onde: l'ENTRATA scura e intera dietro, l'USCITA sopra; dove il master alza, l'uscita sborda e si vede la differenza
    const auto outC = wc.withAlpha (light() ? 0.88f : 0.92f);
    const auto inC = light() ? wc.interpolatedWith (juce::Colour (0xff4a5266), 0.6f).withAlpha (0.92f) : wc.withMultipliedSaturation (0.55f).interpolatedWith (juce::Colours::black, 0.38f).withAlpha (1.0f);
    const auto capC = light() ? wc.darker (0.35f) : wc.brighter (0.45f);
    const bool over = easy() && ! small;   // nel FACILE l'onda scende fin sotto i numeri, le etichette del tempo vanno in alto
    const float bottomPad = small ? 2.0f : over ? (bipolar ? 30.0f : 0.0f) : 16.0f;
    auto area = p.withTrimmedBottom (bottomPad);
    if (! small) scaleR = scaleR.withY (area.getY()).withHeight (area.getHeight());
    const float labY = over ? area.getY() + 2 : area.getBottom() + 2;
    const float cy = area.getCentreY(), hh = area.getHeight() * 0.5f - 2;
    auto lin = [] (float db) { return db < -80 ? 0.0f : juce::jlimit (0.0f, 1.05f, juce::Decibels::decibelsToGain (db)); };
    auto yD = [&] (float db) { return area.getY() + juce::jlimit (0.0f, 1.0f, -db / 36.0f) * area.getHeight(); };
    // righe fisse della scala
    static const float bipDb[] = { 0, -3, -6, -12 }, uniDb[] = { 0, -6, -12, -18, -24, -30 };
    if (! small)
    {
        if (bipolar) for (float d : bipDb) for (int sd = -1; sd <= 1; sd += 2) { const float y = cy + sd * lin (d) * hh; g.setColour (K::hi.withAlpha (d == 0 ? 0.07f : 0.04f)); g.fillRect (area.getX(), y, area.getWidth(), 1.0f); }
        else for (float d : uniDb) { const float y = yD (d); g.setColour (K::hi.withAlpha (d == 0 ? 0.07f : 0.05f)); g.fillRect (area.getX(), y, area.getWidth(), 1.0f); }
    }
    // griglia del tempo: battute del pezzo se la DAW suona, altrimenti secondi (le battute scorrono insieme all'onda)
    bool beats = false; for (int b : e.beat) if (b >= 0) { beats = true; break; }
    g.setFont (num (7.5f));
    if (beats)
        for (int x = 0; x < npx; ++x)
        {
            const int b = e.beat[(size_t) x]; if (b < 0) continue; const bool bar = b >= 100000; const float xx = p.getX() + (x - e.shift) * dx;
            if (xx < area.getX()) continue;
            g.setColour (K::hi.withAlpha (bar ? 0.11f : 0.04f)); g.fillRect (xx, area.getY(), 1.0f, area.getHeight());
            if (bar && ! small) { g.setColour (K::faint); g.drawText (juce::String (b - 100000), juce::Rectangle<float> (xx + 3, labY, 28, 12), juce::Justification::centredLeft); }
        }
    else if (! small)
    {
        const int step = win <= 4 ? 1 : win <= 8 ? 2 : 4;
        for (int s = 0; s <= (int) win; s += step)
        {
            const float xx = scan ? p.getX() + p.getWidth() * s / (float) win : p.getRight() - p.getWidth() * s / (float) win;
            g.setColour (K::hi.withAlpha (0.05f)); g.fillRect (xx, area.getY(), 1.0f, area.getHeight());
            g.setColour (K::faint); g.drawText (scan ? juce::String (s) + "s" : (s == 0 ? juce::String ("ora") : "-" + juce::String (s) + "s"), juce::Rectangle<float> (over ? xx + 3 : xx - 16, labY, 32, 12), over ? juce::Justification::centredLeft : juce::Justification::centred);
        }
    }
    g.saveState(); g.reduceClipRegion (area.toNearestInt());
    g.addTransform (juce::AffineTransform::translation (-e.shift * dx, 0));   // scorrimento rigido al sotto-pixel
    const float cl = P.value ("lm_ceil");
    if (! scan && ! small)
    {
        // SCROLL: anello di colonne. Si disegnano solo le colonne nuove, il resto e' una copia dell'immagine
        auto& R = ring;
        const int cw = juce::jmax (1, juce::roundToInt (g.getInternalContext().getPhysicalPixelScaleFactor()));
        const int W = npx + 8, H = juce::jmax (4, juce::roundToInt (area.getHeight() * cw));
        const juce::String key = juce::String (W) + "|" + juce::String (H) + "|" + juce::String (cw) + "|" + (bipolar ? "b" : "u") + "|" + juce::String (win) + "|" + outC.toString() + inC.toString();
        if (R.key != key || R.img.isNull()) { R.img = juce::Image (juce::Image::ARGB, W * cw, H, true); R.key = key; R.valid = false; R.W = W; R.cw = cw; R.H = H; }
        const long oldest = e.g0 - npx, newest = e.g0 - 1;
        // una colonna si congela solo quando tutti i suoi dati sono arrivati (l'ultima colonna dell'audio e' ancora a meta'):
        // prima congelavo anche quelle a meta' o vuote, e l'onda compariva e spariva a scatti, al ritmo dei blocchi audio
        const double cppR = (win / Processor::kColSec) / npx;
        const long ready = juce::jmin (newest, (long) std::floor ((P.hTot.load() - 3) / cppR) - 1);
        long from = oldest;
        if (R.valid && R.last >= oldest - 1 && R.last <= newest) from = R.last + 1;
        for (long G = juce::jmax (from, ready + 1); G <= newest; ++G) { const int ix = (int) (((G % W) + W) % W) * cw; R.img.clear ({ ix, 0, cw, H }); }
        if (from <= ready)
        {
            juce::Graphics ig (R.img);
            const float c = H * 0.5f, hp = H * 0.5f - 2.0f * cw, cap = (float) cw;
            for (long G = from; G <= ready; ++G)
            {
                const int x = (int) (G - oldest); const int ix = (int) (((G % W) + W) % W) * cw;
                R.img.clear ({ ix, 0, cw, H });
                const float o = e.pk[(size_t) x], i = e.in[(size_t) x];
                if (bipolar)
                {
                    if (o >= -80) { const float a = lin (o) * hp; ig.setColour (outC); ig.fillRect (juce::Rectangle<float> ((float) ix, c - a, (float) cw, 2 * a)); }
                    if (i >= -80) { const float a = lin (i) * hp; ig.setColour (inC); ig.fillRect (juce::Rectangle<float> ((float) ix, c - a, (float) cw, 2 * a)); }
                    if (o >= -80) { const float a = lin (o) * hp; ig.setColour (capC); ig.fillRect (juce::Rectangle<float> ((float) ix, c - a, (float) cw, cap)); ig.fillRect (juce::Rectangle<float> ((float) ix, c + a - cap, (float) cw, cap)); }
                }
                else
                {
                    auto yy = [&] (float db) { return juce::jlimit (0.0f, 1.0f, -db / 36.0f) * H; };
                    if (o >= -80) { ig.setColour (outC); ig.fillRect (juce::Rectangle<float> ((float) ix, yy (o), (float) cw, H - yy (o))); }
                    if (i >= -80) { ig.setColour (inC); ig.fillRect (juce::Rectangle<float> ((float) ix, yy (i), (float) cw, H - yy (i))); }
                    if (o >= -80) { ig.setColour (capC); ig.fillRect (juce::Rectangle<float> ((float) ix, yy (o), (float) cw, cap)); }
                }
            }
            R.last = ready; R.valid = true;
        }
        g.setImageResamplingQuality (juce::Graphics::lowResamplingQuality);
        auto blit = [&] (int col, int n, float x0)
        {
            if (n <= 0) return;
            auto sub = R.img.getClippedImage ({ col * cw, 0, n * cw, H });
            g.drawImageTransformed (sub, juce::AffineTransform::scale (dx / cw, area.getHeight() / (float) H).translated (x0, area.getY()));
        };
        const int io = (int) (((oldest % W) + W) % W), n1 = juce::jmin (npx, W - io);
        blit (io, n1, p.getX()); blit (0, npx - n1, p.getX() + n1 * dx);
    }
    else
    {
        // SCAN e miniature: forme vettoriali
        auto shape = [&] (const std::vector<float>& v)
        {
            juce::Path path; int x = 0;
            while (x < npx)
            {
                while (x < npx && v[(size_t) x] < -80) ++x;
                if (x >= npx) break;
                const int s0 = x; while (x < npx && v[(size_t) x] >= -80) ++x; const int s1 = x;
                if (bipolar)
                {
                    path.startNewSubPath (p.getX() + s0 * dx, cy);
                    for (int k = s0; k < s1; ++k) path.lineTo (p.getX() + (k + 0.5f) * dx, cy - lin (v[(size_t) k]) * hh);
                    path.lineTo (p.getX() + s1 * dx, cy);
                    for (int k = s1 - 1; k >= s0; --k) path.lineTo (p.getX() + (k + 0.5f) * dx, cy + lin (v[(size_t) k]) * hh);
                    path.closeSubPath();
                }
                else
                {
                    path.startNewSubPath (p.getX() + s0 * dx, area.getBottom());
                    for (int k = s0; k < s1; ++k) path.lineTo (p.getX() + (k + 0.5f) * dx, yD (v[(size_t) k]));
                    path.lineTo (p.getX() + s1 * dx, area.getBottom()); path.closeSubPath();
                }
            }
            return path;
        };
        const auto po = shape (e.pk);
        g.setColour (outC); g.fillPath (po);
        g.setColour (inC); g.fillPath (shape (e.in));
        if (! small) { g.setColour (capC.withAlpha (0.9f)); g.strokePath (po, juce::PathStrokeType (1.0f)); }
    }
    // l'intervento del master: la linea del limiter. Ferma e tenue quando non lavora, si piega (e si accende) quando toglie
    {
        auto yLine = [&] (float gr, bool top)
        {
            if (bipolar) { const float cy = area.getCentreY(), h = area.getHeight() * 0.5f - 2, a = juce::Decibels::decibelsToGain (cl - gr) * h; return top ? cy - a : cy + a; }
            return area.getY() + juce::jlimit (0.0f, 1.0f, gr / 36.0f) * area.getHeight();
        };
        for (int side = 0; side < (bipolar && ! over ? 2 : 1); ++side)
        {
            juce::Path base, act; bool inAct = false;
            for (int x = 0; x < npx; ++x)
            {
                const float gr = e.gr[(size_t) x], xx = p.getX() + (x + 0.5f) * dx, y = yLine (gr, side == 0);
                if (x == 0) base.startNewSubPath (xx, y); else base.lineTo (xx, y);
                if (gr > 0.15f) { if (! inAct) { act.startNewSubPath (xx, yLine (0, side == 0)); inAct = true; } act.lineTo (xx, y); }
                else if (inAct) { act.lineTo (xx, y); inAct = false; }
            }
            g.setColour (lineC.withAlpha (light() ? 0.22f : 0.18f)); g.strokePath (base, juce::PathStrokeType (1.0f));
            g.setColour (lineC.withAlpha (0.25f)); g.strokePath (act, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setColour (lineC); g.strokePath (act, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        // quanto toglie: un numero per ogni battuta (o per secondo), sopra il punto piu' profondo
        if (! small)
        {
            const int segPx = juce::jmax (44, (int) (npx / juce::jmax (1.0, win)));
            float lastX = -100;
            g.setFont (num (8.5f));
            for (int s0 = 0; s0 < npx; )
            {
                int s1 = s0 + segPx;
                if (beats) { s1 = s0 + 1; while (s1 < npx && e.beat[(size_t) s1] < 100000) ++s1; }
                s1 = juce::jmin (s1, npx);
                float m = 0; int mx = s0; for (int x = s0; x < s1; ++x) if (e.gr[(size_t) x] > m) { m = e.gr[(size_t) x]; mx = x; }
                const float xx = p.getX() + (mx + 0.5f) * dx;
                if (m >= 0.5f && xx - lastX > 36)
                {
                    lastX = xx; const auto t = "-" + juce::String (m, 1);
                    const float ty = bipolar ? yLine (0, true) - 14 : yLine (m, true) + 4;
                    auto tr = juce::Rectangle<float> (xx - 17, juce::jmax (area.getY(), ty), 34, 13);
                    g.setColour (K::lo.withAlpha (light() ? 0.12f : 0.55f)); g.fillRoundedRectangle (tr, 4);
                    g.setColour (m > 6 ? K::red : lineC); g.drawText (t, tr, juce::Justification::centred);
                }
                s0 = juce::jmax (s1, s0 + 1);
            }
        }
    }
    if (e.cursor >= 0) { const float xx = p.getX() + e.cursor * dx; g.setColour (lineC.withAlpha (0.5f)); g.fillRect (xx, area.getY(), 1.0f, area.getHeight()); }
    g.restoreState();
    if (! small)
    {   // la scala fissa
        g.setColour (K::hi.withAlpha (0.06f)); g.fillRect (scaleR.getRight() - 1, area.getY(), 1.0f, area.getHeight());
        g.setFont (num (7.5f)); g.setColour (K::faint);
        auto lab = [&] (float y, float d) { g.drawText (d == 0 ? juce::String ("0") : juce::String ((int) d), juce::Rectangle<float> (scaleR.getX(), juce::jlimit (area.getY(), area.getBottom() - 10, y - 5), scaleR.getWidth() - 5, 10), juce::Justification::centredRight); };
        if (bipolar) { for (float d : bipDb) { lab (cy - lin (d) * hh, d); lab (cy + lin (d) * hh, d); } g.setColour (K::faint.withAlpha (0.6f)); g.drawText ("dB", juce::Rectangle<float> (scaleR.getX(), cy - 5, scaleR.getWidth() - 5, 10), juce::Justification::centredRight); }
        else for (float d : uniDb) lab (yD (d), d);
    }
    if (! small && P.lufsS.load() < -69 && P.hTot.load() < 10) { g.setFont (f (12.0f)); g.setColour (K::sub); g.drawText ("premi play", area, juce::Justification::centred); }
}

// ---------------------------------------------------------------------------------------------------- SPETTRO
static int bandsFor (int mode, float* edges)
{
    if (mode == 1) { int n = 0; for (int k = 0; k <= 30; ++k) { const float c = 1000.0f * std::pow (2.0f, (k - 17) / 3.0f); if (k == 0) edges[n++] = c * std::pow (2.0f, -1.0f / 6.0f); edges[n++] = c * std::pow (2.0f, 1.0f / 6.0f); } return n - 1; }
    static const float bk[] = { 20, 100, 200, 300, 400, 510, 630, 770, 920, 1080, 1270, 1480, 1720, 2000, 2320, 2700, 3150, 3700, 4400, 5300, 6400, 7700, 9500, 12000, 15500, 20000 };
    for (int i = 0; i < 26; ++i) edges[i] = bk[i]; return 25;
}
void EasyStage::spectrum (juce::Graphics& g, juce::Rectangle<float> p, int mode, bool small)
{
    auto& P = page.ed.proc;
    const int NP = 300; const double sr = P.sampleRate();
    static const float rel[3] = { 0.55f, 0.8f, 0.93f };
    const float sm = rel[juce::jlimit (0, 2, P.uiRel.load())];
    anOut.run (P.ringOut, specOut, NP, 20, 20000, sr, sm);
    const bool prev = P.uiPrev.load() == 1 && ! small; if (prev) anIn.run (P.ringIn, specIn, NP, 20, 20000, sr, sm);
    float mx = -200; for (int i = 0; i < NP; ++i) mx = juce::jmax (mx, specOut[i]);
    const float wantOff = (P.uiNorm.load() == 1 && mx > -100) ? -12.0f - mx : 0.0f; normOff += (wantOff - normOff) * 0.15f;
    if (! small) p = p.withTrimmedBottom (16).withTrimmedLeft (30);
    auto xOf = [&p] (float fr) { return p.getX() + p.getWidth() * std::log (fr / 20.0f) / std::log (1000.0f); };
    auto yOf = [&p, this] (float db) { return p.getBottom() - juce::jlimit (0.0f, 1.0f, (db + normOff + 92.0f) / 86.0f) * p.getHeight(); };
    auto ptOf = [] (float fr) { return juce::jlimit (0, 299, (int) (std::log (fr / 20.0f) / std::log (1000.0f) * 300)); };
    // picchi grigi: restano fermi 0.25 s, poi scendono a 60 dB al secondo (prima 0.35 dB per fotogramma: lenti)
    const double nowT = juce::Time::getMillisecondCounterHiRes(); const float dtS = (float) juce::jlimit (0.0, 0.1, (nowT - lastSpecT) / 1000.0); lastSpecT = nowT;
    auto holdStep = [dtS] (float v, float& hold, float& age) { if (v >= hold) { hold = v; age = 0; } else { age += dtS; if (age > 0.25f) hold = juce::jmax (v, hold - 60.0f * dtS); } };
    const auto wc = waveCol();
    if (! small)
    {
        g.setFont (num (7.5f));
        for (int d = -84; d <= -12; d += 12) { g.setColour (K::hi.withAlpha (0.04f)); g.fillRect (p.getX(), yOf ((float) d - normOff), p.getWidth(), 1.0f); g.setColour (K::faint); g.drawText (juce::String (d), juce::Rectangle<float> (p.getX() - 30, yOf ((float) d - normOff) - 6, 26, 12), juce::Justification::centredRight); }
        for (float fr : { 50.f, 100.f, 200.f, 500.f, 1000.f, 2000.f, 5000.f, 10000.f })
        { g.setColour (K::hi.withAlpha (0.045f)); g.fillRect (xOf (fr), p.getY(), 1.0f, p.getHeight()); g.setColour (K::faint); g.drawText (fr >= 1000 ? juce::String ((int) fr / 1000) + "k" : juce::String ((int) fr), juce::Rectangle<float> (xOf (fr) - 14, p.getBottom() + 2, 28, 12), juce::Justification::centred); }
    }
    if (mode == 0)
    {
        juce::Path fill, line;
        fill.startNewSubPath (p.getX(), p.getBottom());
        for (int i = 0; i < NP; ++i)
        {
            const float x = p.getX() + p.getWidth() * (i + 0.5f) / NP, y = yOf (specOut[i]);
            fill.lineTo (x, y); if (i == 0) line.startNewSubPath (x, y); else line.lineTo (x, y);
            holdStep (specOut[i], specHold[i], specAge[i]);
        }
        fill.lineTo (p.getRight(), p.getBottom()); fill.closeSubPath();
        juce::ColourGradient cg (wc.withAlpha (light() ? 0.55f : 0.6f), 0, p.getY(), wc.withAlpha (0.06f), 0, p.getBottom(), false); g.setGradientFill (cg); g.fillPath (fill);
        g.setColour (wc); g.strokePath (line, juce::PathStrokeType (small ? 1.0f : 1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        if (! small)
        {
            juce::Path hold; for (int i = 0; i < NP; ++i) { const float x = p.getX() + p.getWidth() * (i + 0.5f) / NP, y = yOf (specHold[i]); if (i == 0) hold.startNewSubPath (x, y); else hold.lineTo (x, y); }
            g.setColour (K::ink.withAlpha (0.28f)); g.strokePath (hold, juce::PathStrokeType (0.8f));
        }
    }
    else
    {
        float edges[40]; const int nb = bandsFor (mode, edges);
        for (int b = 0; b < nb; ++b)
        {
            const float lo = juce::jmax (20.0f, edges[b]), hi = juce::jmin (20000.0f, edges[b + 1]); if (hi <= lo) continue;
            float v = -200; for (int i = ptOf (lo); i <= ptOf (hi); ++i) v = juce::jmax (v, specOut[i]);
            holdStep (v, barHold[b], barAge[b]);
            const float x0 = xOf (lo) + 1.5f, x1 = xOf (hi) - 1.5f, y = yOf (v);
            juce::ColourGradient cg (wc.withAlpha (light() ? 0.9f : 0.95f), 0, y, wc.withAlpha (0.35f), 0, p.getBottom(), false); g.setGradientFill (cg);
            g.fillRect (juce::Rectangle<float>::leftTopRightBottom (x0, y, x1, p.getBottom()));
            g.setColour (K::ink.withAlpha (0.75f)); g.fillRect (x0, yOf (barHold[b]) - 1, x1 - x0, 1.6f);
        }
    }
    if (prev)
    {
        juce::Path pl; for (int i = 0; i < NP; ++i) { const float x = p.getX() + p.getWidth() * (i + 0.5f) / NP, y = yOf (specIn[i]); if (i == 0) pl.startNewSubPath (x, y); else pl.lineTo (x, y); }
        g.setColour (K::sub.withAlpha (0.8f)); g.strokePath (pl, juce::PathStrokeType (1.0f));
    }
}

// ---------------------------------------------------------------------------------------------------- IMMAGINE STEREO
void EasyStage::image (juce::Graphics& g, juce::Rectangle<float> p, bool small)
{
    auto& P = page.ed.proc;
    const int n = 2048; float l[n], r[n]; P.ringOut.read (l, r, n);
    double em = 0, es = 0, el = 0, er = 0, lr = 0; float pkv = 1e-4f;
    for (int i = 0; i < n; ++i) { const float m = 0.5f * (l[i] + r[i]), s = 0.5f * (l[i] - r[i]); em += m * m; es += s * s; el += l[i] * l[i]; er += r[i] * r[i]; lr += l[i] * r[i]; pkv = juce::jmax (pkv, std::abs (m) + std::abs (s)); }
    const float corr = (el > 1e-9 && er > 1e-9) ? (float) (lr / std::sqrt (el * er)) : 1.0f; crS += (corr - crS) * 0.08f;
    const float width = em > 1e-9 ? (float) std::sqrt (es / em) : 0.0f; wS += (width - wS) * 0.08f;
    const float bal = (el > 1e-9 && er > 1e-9) ? (float) (10 * std::log10 (el / er)) : 0.0f;
    {   // correlazione per banda misurata qui (non dipende dall'imager acceso): tagli a 150 Hz, 1 kHz, 5 kHz
        const double sr = juce::jmax (8000.0, P.sampleRate()); static const float fc[3] = { 150, 1000, 5000 };
        float a[3]; for (int k = 0; k < 3; ++k) a[k] = (float) std::exp (-juce::MathConstants<double>::twoPi * fc[k] / sr);
        double xy[4] {}, xx[4] {}, yy[4] {}; float sl[3][2] {}, sr2[3][2] {};
        for (int i = 0; i < n; ++i)
        {
            float lp[3], rp[3];
            for (int k = 0; k < 3; ++k)
            {   // passa basso a due poli per ogni taglio
                sl[k][0] = l[i] + a[k] * (sl[k][0] - l[i]); sl[k][1] = sl[k][0] + a[k] * (sl[k][1] - sl[k][0]); lp[k] = sl[k][1];
                sr2[k][0] = r[i] + a[k] * (sr2[k][0] - r[i]); sr2[k][1] = sr2[k][0] + a[k] * (sr2[k][1] - sr2[k][0]); rp[k] = sr2[k][1];
            }
            const float bl[4] = { lp[0], lp[1] - lp[0], lp[2] - lp[1], l[i] - lp[2] }, br[4] = { rp[0], rp[1] - rp[0], rp[2] - rp[1], r[i] - rp[2] };
            if (i < 256) continue;   // lascia assestare i filtri
            for (int b = 0; b < 4; ++b) { xy[b] += bl[b] * br[b]; xx[b] += bl[b] * bl[b]; yy[b] += br[b] * br[b]; }
        }
        for (int b = 0; b < 4; ++b) { const float c = (xx[b] > 1e-10 && yy[b] > 1e-10) ? (float) (xy[b] / std::sqrt (xx[b] * yy[b])) : 1.0f; bandCr[b] += (c - bandCr[b]) * 0.1f; }
    }
    // vettorscopio con scia
    const float S = small ? juce::jmin (p.getWidth(), p.getHeight()) : juce::jmin (p.getHeight(), p.getWidth() * 0.5f);
    auto sq = small ? p.withSizeKeepingCentre (S, S) : juce::Rectangle<float> (p.getX() + 20, p.getY() + (p.getHeight() - S) * 0.5f, S, S);
    const int is = juce::jmax (8, (int) S);
    if (scope.getWidth() != is) scope = juce::Image (juce::Image::ARGB, is, is, true);
    scope.multiplyAllAlphas (0.78f);
    {
        juce::Graphics sg (scope); const float c = is * 0.5f, k = is * 0.46f / juce::jmax (0.05f, pkv);
        sg.setColour (waveCol().withAlpha (0.55f));
        for (int i = 0; i < n; i += 2) { const float m = 0.5f * (l[i] + r[i]), s = 0.5f * (l[i] - r[i]); sg.fillRect (c + s * k, c - m * k, 1.3f, 1.3f); }
    }
    juce::Path dm; dm.addTriangle (sq.getCentreX(), sq.getY(), sq.getRight(), sq.getCentreY(), sq.getCentreX(), sq.getBottom()); dm.addTriangle (sq.getCentreX(), sq.getY(), sq.getX(), sq.getCentreY(), sq.getCentreX(), sq.getBottom());
    g.setColour (K::hi.withAlpha (0.08f)); g.strokePath (dm, juce::PathStrokeType (1.0f));
    g.fillRect (sq.getCentreX(), sq.getY(), 1.0f, sq.getHeight()); g.fillRect (sq.getX(), sq.getCentreY(), sq.getWidth(), 1.0f);
    g.setOpacity (1.0f); g.setColour (juce::Colours::white); g.drawImage (scope, sq);
    g.setFont (cap (7.5f)); g.setColour (K::faint);
    g.drawText ("M", juce::Rectangle<float> (sq.getCentreX() - 10, sq.getY() - 2, 20, 10), juce::Justification::centred);
    g.drawText ("L", juce::Rectangle<float> (sq.getX() + 4, sq.getY() + 4, 12, 10), juce::Justification::centredLeft);
    g.drawText ("R", juce::Rectangle<float> (sq.getRight() - 16, sq.getY() + 4, 12, 10), juce::Justification::centredRight);
    if (small) return;
    // misure a destra: correlazione, larghezza, bilanciamento, correlazione per banda
    auto rr = juce::Rectangle<float> (sq.getRight() + 40, p.getY() + 10, p.getRight() - sq.getRight() - 60, p.getHeight() - 20);
    auto meter = [&] (float y, const char* name, juce::String val, float pos01, juce::Colour c, bool centre)
    {
        g.setFont (cap (8.0f)); g.setColour (K::sub); g.drawText (name, juce::Rectangle<float> (rr.getX(), y, 260, 12), juce::Justification::centredLeft);
        g.setFont (num (11.0f)); g.setColour (K::ink); g.drawText (val, juce::Rectangle<float> (rr.getRight() - 120, y - 2, 120, 14), juce::Justification::centredRight);
        auto bar = juce::Rectangle<float> (rr.getX(), y + 18, rr.getWidth(), 6);
        g.setColour (K::lo.withAlpha (light() ? 0.10f : 0.4f)); g.fillRoundedRectangle (bar, 3);
        const float xv = bar.getX() + juce::jlimit (0.0f, 1.0f, pos01) * bar.getWidth();
        if (centre) { const float xc = bar.getCentreX(); g.setColour (c); g.fillRoundedRectangle (juce::Rectangle<float>::leftTopRightBottom (juce::jmin (xc, xv), bar.getY(), juce::jmax (xc, xv), bar.getBottom()), 3); g.setColour (K::hi.withAlpha (0.25f)); g.fillRect (xc, bar.getY() - 3, 1.0f, bar.getHeight() + 6); }
        else { g.setColour (c); g.fillRoundedRectangle (bar.withRight (xv), 3); }
        g.setColour (K::ink); g.fillRoundedRectangle (xv - 1.5f, bar.getY() - 3, 3, bar.getHeight() + 6, 1.5f);
    };
    const auto cc = crS < 0 ? K::red : crS < 0.3f ? K::amber : K::mint;
    meter (rr.getY(), "CORRELAZIONE", juce::String (crS, 2), (crS + 1) * 0.5f, cc, true);
    meter (rr.getY() + 46, "LARGHEZZA (S / M)", juce::String (juce::roundToInt (wS * 100)) + "%", juce::jlimit (0.0f, 1.0f, wS / 1.2f), K::violet, false);
    meter (rr.getY() + 92, "BILANCIAMENTO L / R", (std::abs (bal) < 0.05f ? juce::String ("C") : juce::String (std::abs (bal), 1) + " dB " + (bal > 0 ? "L" : "R")), 0.5f - juce::jlimit (-6.0f, 6.0f, bal) / 12.0f, K::cyan, true);
    static const char* bn[kImBands] = { "SOTTO 150 Hz", "150 Hz - 1 kHz", "1 - 5 kHz", "SOPRA 5 kHz" };
    g.setFont (cap (8.0f)); g.setColour (K::sub); g.drawText ("CORRELAZIONE PER BANDA", juce::Rectangle<float> (rr.getX(), rr.getY() + 140, 260, 12), juce::Justification::centredLeft);
    for (int b = 0; b < kImBands; ++b)
    {
        const float c = bandCr[b], y = rr.getY() + 162 + b * 26.0f;
        auto bar = juce::Rectangle<float> (rr.getX() + 120, y + 3, rr.getWidth() - 180, 6);
        g.setFont (cap (7.5f)); g.setColour (K::faint); g.drawText (bn[b], juce::Rectangle<float> (rr.getX(), y, 116, 12), juce::Justification::centredLeft);
        g.setColour (K::lo.withAlpha (light() ? 0.10f : 0.4f)); g.fillRoundedRectangle (bar, 3);
        const float xv = bar.getX() + juce::jlimit (0.0f, 1.0f, (c + 1) * 0.5f) * bar.getWidth(), xc = bar.getCentreX();
        g.setColour (c < -0.1f ? K::red : c < 0.3f ? K::amber : K::mint); g.fillRoundedRectangle (juce::Rectangle<float>::leftTopRightBottom (juce::jmin (xc, xv), bar.getY(), juce::jmax (xc, xv), bar.getBottom()), 3);
        g.setFont (num (9.0f)); g.setColour (K::ink); g.drawText (juce::String (c, 2), juce::Rectangle<float> (bar.getRight() + 6, y - 1, 50, 14), juce::Justification::centredRight);
    }
    g.setFont (f (9.0f)); g.setColour (K::faint);
    g.drawFittedText (U ("+1 = mono, 0 = molto largo, sotto zero = fasi opposte: in mono qualcosa sparisce. I bassi vanno tenuti vicino a +1."), juce::Rectangle<int> ((int) rr.getX(), (int) (rr.getY() + 274), (int) rr.getWidth(), 30), juce::Justification::topLeft, 2);
}

// ---------------------------------------------------------------------------------------------------- METERING COMPLETO
void EasyStage::metering (juce::Graphics& g, juce::Rectangle<float> p)
{
    auto& P = page.ed.proc; const auto& T = target (juce::roundToInt (P.value ("target")));
    p = p.withTrimmedBottom (easy() ? 4 : 0);
    const float W = p.getWidth();
    auto colA = juce::Rectangle<float> (p.getX(), p.getY(), W * 0.33f, p.getHeight());
    auto colB = juce::Rectangle<float> (colA.getRight() + 16, p.getY(), W * 0.17f, p.getHeight());
    auto colC = juce::Rectangle<float> (colB.getRight() + 16, p.getY(), W * 0.19f, p.getHeight());
    auto colD = juce::Rectangle<float>::leftTopRightBottom (colC.getRight() + 20, p.getY(), p.getRight(), p.getBottom());
    auto title = [&] (juce::Rectangle<float> r, const char* t) { g.setFont (cap (8.0f)); g.setColour (K::sub); g.drawText (t, r.withHeight (12), juce::Justification::centred); };
    // A: spettro a terzi e gain sotto
    { auto a = colA; auto top = a.removeFromTop (a.getHeight() * 0.58f); title (top, "SPETTRO 1/3 OTT"); spectrum (g, top.withTrimmedTop (16), 1, true);
      a.removeFromTop (10); title (a, "GAIN"); wave (g, a.withTrimmedTop (16), true, 4.0, true); }
    // B: immagine
    { title (colB, "IMMAGINE"); auto sq = colB.withTrimmedTop (16).removeFromTop (juce::jmin (colB.getWidth(), colB.getHeight() * 0.62f)); image (g, sq, true);
      const auto cc = crS < 0 ? K::red : crS < 0.3f ? K::amber : K::mint;
      g.setFont (cap (7.5f)); g.setColour (K::faint); g.drawText ("CORRELAZIONE", juce::Rectangle<float> (colB.getX(), sq.getBottom() + 10, colB.getWidth(), 12), juce::Justification::centred);
      g.setFont (num (16.0f)); g.setColour (cc); g.drawText (juce::String (crS, 2), juce::Rectangle<float> (colB.getX(), sq.getBottom() + 24, colB.getWidth(), 22), juce::Justification::centred); }
    // C: loudness
    {
        const float I = P.lufsI.load(), S2 = P.lufsS.load(), M = P.lufsM.load(), tp = P.tpMax.load(), plr = (tp > -60 && I > -60) ? tp - I : -99;
        const auto dash = U ("\xe2\x80\x93");
        title (colC, "LUFS-I");
        const float d = I - T.lufs; const auto c = I < -69 ? K::faint : std::abs (d) <= 1 ? K::mint : d > 1 ? K::red : K::amber;
        g.setFont (neo::big (40.0f)); g.setColour (c); g.drawText (I < -69 ? dash + dash : juce::String (I, 1), juce::Rectangle<float> (colC.getX(), colC.getY() + 14, colC.getWidth(), 48), juce::Justification::centred);
        // crush
        crushS += (P.crush.load() - crushS) * 0.1f;
        auto bar = juce::Rectangle<float> (colC.getX() + 6, colC.getY() + 92, colC.getWidth() - 12, 6);
        g.setFont (cap (6.5f)); g.setColour (K::faint);
        g.drawText ("CRUSH", bar.withY (bar.getY() - 14).withHeight (10), juce::Justification::centredLeft); g.drawText ("OPTIMAL", bar.withY (bar.getY() - 14).withHeight (10), juce::Justification::centred); g.drawText ("SAFE", bar.withY (bar.getY() - 14).withHeight (10), juce::Justification::centredRight);
        g.setColour (K::lo.withAlpha (light() ? 0.10f : 0.4f)); g.fillRoundedRectangle (bar, 3);
        g.setColour (K::mint.withAlpha (0.35f)); g.fillRoundedRectangle (bar.withSizeKeepingCentre (bar.getWidth() * 0.3f, bar.getHeight()), 3);
        const float xv = bar.getX() + (1.0f - crushS) * bar.getWidth(); const auto cc = crushS > 0.7f ? K::red : crushS < 0.3f ? K::cyan : K::mint;
        g.setColour (cc); g.fillRoundedRectangle (xv - 2, bar.getY() - 4, 4, bar.getHeight() + 8, 2);
        struct V { const char* n; juce::String v; };
        V vs[3] = { { "LUFS-S", S2 < -69 ? dash : juce::String (S2, 1) }, { "LUFS-M", M < -69 ? dash : juce::String (M, 1) }, { "PLR", plr < -90 ? dash : juce::String (plr, 1) } };
        for (int i = 0; i < 3; ++i)
        {
            auto r = juce::Rectangle<float> (colC.getX() + colC.getWidth() * i / 3.0f, colC.getY() + 124, colC.getWidth() / 3.0f, 40);
            g.setFont (cap (7.0f)); g.setColour (K::faint); g.drawText (vs[i].n, r.withHeight (12), juce::Justification::centred);
            g.setFont (num (13.0f)); g.setColour (K::ink); g.drawText (vs[i].v, r.withTrimmedTop (14), juce::Justification::centred);
        }
        g.setFont (f (9.5f)); g.setColour (c); g.drawFittedText (I < -69 ? U ("premi play") : fmt (d) + " LU dal target " + juce::String (T.name), juce::Rectangle<int> ((int) colC.getX(), (int) colC.getY() + 178, (int) colC.getWidth(), 30), juce::Justification::centred, 2);
    }
    // D: colonne RMS, PEAK, CREST
    {
        const double now = juce::Time::getMillisecondCounterHiRes(); const float dt = (float) juce::jlimit (0.0, 0.2, (now - mLast) / 1000.0); mLast = now;
        float pk[2], rm[2];
        for (int c = 0; c < 2; ++c)
        {
            const float pn = juce::Decibels::gainToDecibels (P.outPk[c].load(), -100.0f);
            if (pn >= mtPk[c]) { mtPk[c] = pn; mtHold[c] = 1.0f; } else if ((mtHold[c] -= dt) <= 0) mtPk[c] = juce::jmax (pn, mtPk[c] - 8.0f * dt);   // picco: resta un secondo, poi scende piano
            pk[c] = mtPk[c];
            const float rn = juce::Decibels::gainToDecibels (std::sqrt (P.outRms[c].load()), -100.0f); mRms[c] += (rn - mRms[c]) * juce::jmin (1.0f, dt * 8.0f); rm[c] = mRms[c];
        }
        const float crest = juce::jmax (pk[0], pk[1]) - juce::jmax (rm[0], rm[1]);
        auto grp = [&] (juce::Rectangle<float> r, const char* name, const float* v, int nv, float lo, float hi, juce::Colour c, const char* unit)
        {
            title (r, name);
            auto bars = r.withTrimmedTop (18).withTrimmedBottom (34);
            auto yOf = [&] (float x) { return bars.getBottom() - juce::jlimit (0.0f, 1.0f, (x - lo) / (hi - lo)) * bars.getHeight(); };
            const float bw = juce::jmin (16.0f, (bars.getWidth() - 4 * (nv - 1)) / nv), x0 = bars.getCentreX() - (bw * nv + 6 * (nv - 1)) * 0.5f;
            for (int i = 0; i < nv; ++i)
            {
                auto col = juce::Rectangle<float> (x0 + i * (bw + 6), bars.getY(), bw, bars.getHeight());
                g.setColour (K::lo.withAlpha (light() ? 0.08f : 0.35f)); g.fillRoundedRectangle (col, 3);
                juce::ColourGradient cg (c, 0, yOf (v[i]), c.withAlpha (0.45f), 0, col.getBottom(), false); g.setGradientFill (cg);
                g.fillRoundedRectangle (col.withTop (yOf (v[i])), 3);
                g.setFont (num (8.5f)); g.setColour (K::ink); g.drawText (v[i] < lo ? U ("\xe2\x80\x93") : juce::String (v[i], 1), juce::Rectangle<float> (col.getCentreX() - 22, bars.getBottom() + 6, 44, 12), juce::Justification::centred);
            }
            g.setFont (cap (6.5f)); g.setColour (K::faint); g.drawText (unit, juce::Rectangle<float> (r.getX(), r.getBottom() - 12, r.getWidth(), 10), juce::Justification::centred);
        };
        const float cw = colD.getWidth() / 3.0f;
        grp (colD.withWidth (cw), "RMS", rm, 2, -60, 0, K::violet, "dBFS");
        grp (colD.withX (colD.getX() + cw).withWidth (cw), "PEAK", pk, 2, -60, 0, pk[0] > -1 || pk[1] > -1 ? K::red : K::cyan, "dBFS");
        grp (colD.withX (colD.getX() + 2 * cw).withWidth (cw), "CREST", &crest, 1, 0, 24, K::mint, "dB");
    }
}

// ---------------------------------------------------------------------------------------------------- numeri sul display (FACILE)
void EasyStage::numbers (juce::Graphics& g)
{
    auto& P = page.ed.proc; const auto& T = target (juce::roundToInt (P.value ("target")));
    auto b = getLocalBounds().toFloat();
    const float y0 = b.getBottom() - 78;
    // velo in basso per leggere i numeri sopra l'onda
    g.setImageResamplingQuality (juce::Graphics::lowResamplingQuality);
    if (veilImg.isValid()) g.drawImageTransformed (veilImg, juce::AffineTransform::scale ((float) getWidth() / (float) veilImg.getWidth()).translated (0, b.getBottom() - 110));
    const auto dash = U ("\xe2\x80\x93");
    // CRUSH / OPTIMAL / SAFE + LUFS-I
    crushS += (P.crush.load() - crushS) * 0.1f;
    auto bar = juce::Rectangle<float> (40, y0 + 22, 250, 7);
    g.setFont (cap (7.5f)); g.setColour (K::sub);
    g.drawText ("CRUSH", bar.withY (y0 + 4).withHeight (12), juce::Justification::centredLeft); g.drawText ("OPTIMAL", bar.withY (y0 + 4).withHeight (12), juce::Justification::centred); g.drawText ("SAFE", bar.withY (y0 + 4).withHeight (12), juce::Justification::centredRight);
    g.setColour (K::lo.withAlpha (light() ? 0.10f : 0.45f)); g.fillRoundedRectangle (bar, 3.5f);
    g.setColour (K::mint.withAlpha (0.35f)); g.fillRoundedRectangle (bar.withSizeKeepingCentre (bar.getWidth() * 0.3f, bar.getHeight()), 3.5f);
    for (int k = 1; k < 10; ++k) { g.setColour (K::hi.withAlpha (0.08f)); g.fillRect (bar.getX() + bar.getWidth() * k / 10.0f, bar.getY() + 1, 1.0f, bar.getHeight() - 2); }
    const float xv = bar.getX() + (1.0f - crushS) * bar.getWidth(); const auto cc = crushS > 0.7f ? K::red : crushS < 0.3f ? K::cyan : K::mint;
    g.setColour (cc.withAlpha (0.3f)); g.fillRoundedRectangle (xv - 5, bar.getY() - 6, 10, bar.getHeight() + 12, 4);
    g.setColour (cc); g.fillRoundedRectangle (xv - 2, bar.getY() - 4, 4, bar.getHeight() + 8, 2);
    const float S = P.lufsS.load(), I = P.lufsI.load(), tp = P.tpMax.load();
    juce::String adv; juce::Colour ac = K::faint; const float d = S - T.lufs;
    if (S < -60) adv = "premi play";
    else if (d > 3 && crushS > 0.7f) { adv = "+" + juce::String (d, 1) + " dB sopra il target"; ac = K::red; }
    else if (crushS > 0.7f) { adv = "troppo schiacciato: abbassa LOUD"; ac = K::red; }
    else if (crushS < 0.3f) { adv = U ("c'\xc3\xa8 margine: puoi alzare LOUD"); ac = K::cyan; }
    else { adv = "nel punto giusto"; ac = K::mint; }
    g.setFont (f (9.0f, 1)); g.setColour (ac); g.drawText (adv, juce::Rectangle<float> (bar.getX(), bar.getBottom() + 8, 260, 14), juce::Justification::centredLeft);
    auto cell = [&] (float x, float w, const char* n, const juce::String& v, juce::Colour c)
    {
        g.setFont (cap (7.5f)); g.setColour (K::sub); g.drawText (n, juce::Rectangle<float> (x, y0 + 4, w, 12), juce::Justification::centred);
        g.setFont (num (17.0f)); g.setColour (c); g.drawText (v, juce::Rectangle<float> (x, y0 + 20, w, 26), juce::Justification::centred);
    };
    const auto ci = I < -69 ? K::faint : std::abs (I - T.lufs) <= 1 ? K::mint : I > T.lufs + 1 ? K::red : K::ink;
    cell (308, 96, "LUFS-I", I < -69 ? dash + dash : juce::String (I, 1), ci);
    // a destra: LUFS-S, PEAK, RMS, PLR
    const float rmsDb = juce::Decibels::gainToDecibels (std::sqrt (0.5f * (P.outRms[0].load() + P.outRms[1].load())), -100.0f);
    const float plr = (tp > -60 && I > -60) ? tp - I : -99;
    const float x0 = b.getRight() - 24 - 4 * 92;
    const auto cs = S < -69 ? K::faint : std::abs (d) <= 1.5f ? K::mint : d > 0 ? K::red : K::ink;
    cell (x0, 92, "LUFS-S", S < -69 ? dash + dash : juce::String (S, 1), cs);
    cell (x0 + 92, 92, "PEAK", tp < -60 ? dash + dash : juce::String (tp, 1), tp > T.tp ? K::red : K::ink);
    cell (x0 + 184, 92, "RMS", rmsDb < -69 ? dash + dash : juce::String (rmsDb, 1), K::ink);
    cell (x0 + 276, 92, "PLR", plr < -90 ? dash + dash : juce::String (plr, 1), plr > 0 && plr < 7 ? K::red : K::ink);
}

// ---------------------------------------------------------------------------------------------------- colonna del livello
void EasyStage::meterCol (juce::Graphics& g, juce::Rectangle<float> r)
{   // tre colonne: L e R (picco chiaro, volume medio pieno, tacca del picco che resta) e LU = loudness short-term contro il target
    auto& P = page.ed.proc;
    const double now = juce::Time::getMillisecondCounterHiRes(); const float dt = (float) juce::jlimit (0.0, 0.2, (now - mLast) / 1000.0); mLast = now;
    g.setColour (K::lo.withAlpha (light() ? 0.06f : 0.28f)); g.fillRoundedRectangle (r, 8);
    auto bars = r.reduced (6).withTrimmedTop (16).withTrimmedBottom (34);
    auto yOf = [&bars] (float db) { return bars.getBottom() - juce::jlimit (0.0f, 1.0f, (db + 48.0f) / 48.0f) * bars.getHeight(); };
    for (int d : { 0, -6, -12, -24, -36 }) { g.setColour (K::hi.withAlpha (0.07f)); g.fillRect (bars.getX(), yOf ((float) d), bars.getWidth(), 1.0f); }
    const float bw = (bars.getWidth() - 8) / 3;
    const auto wc = waveCol();
    for (int c = 0; c < 2; ++c)
    {
        const float pkNow = juce::Decibels::gainToDecibels (P.outPk[c].load(), -100.0f), rm = juce::Decibels::gainToDecibels (std::sqrt (P.outRms[c].load()), -100.0f);
        mPk[c] = pkNow >= mPk[c] ? pkNow : juce::jmax (pkNow, mPk[c] - 26.0f * dt);
        if (pkNow >= pkHold[c]) { pkHold[c] = pkNow; holdS[c] = 1.5f; } else if ((holdS[c] -= dt) <= 0) pkHold[c] -= 12.0f * dt;
        auto col = juce::Rectangle<float> (bars.getX() + c * (bw + 4), bars.getY(), bw, bars.getHeight());
        g.setColour (K::hi.withAlpha (0.05f)); g.fillRoundedRectangle (col, 2);
        const auto cc = mPk[c] > -1.0f ? K::red : wc;
        g.setColour (cc.withAlpha (0.4f)); g.fillRoundedRectangle (col.withTop (yOf (mPk[c])), 2);
        g.setColour (cc); g.fillRoundedRectangle (col.withTop (yOf (rm)), 2);
        g.setColour (pkHold[c] > -1.0f ? K::red : K::ink); g.fillRect (col.getX(), yOf (pkHold[c]) - 1, col.getWidth(), 2.0f);
    }
    {   // loudness short-term contro il target della piattaforma
        const float tg = target (juce::roundToInt (P.value ("target"))).lufs, lu = P.lufsS.load();
        mLu = lu < -70 ? -100.0f : (mLu < -70 ? lu : mLu + (lu - mLu) * juce::jmin (1.0f, dt * 6.0f));
        auto yL = [&bars] (float l) { return bars.getBottom() - juce::jlimit (0.0f, 1.0f, (l + 36.0f) / 36.0f) * bars.getHeight(); };
        auto col = juce::Rectangle<float> (bars.getX() + 2 * (bw + 4), bars.getY(), bw, bars.getHeight());
        g.setColour (K::hi.withAlpha (0.05f)); g.fillRoundedRectangle (col, 2);
        const float dd = mLu - tg; const auto lc = mLu < -70 ? K::faint : std::abs (dd) <= 1.5f ? K::mint : dd > 0 ? K::red : K::amber;
        if (mLu > -70) { juce::ColourGradient lg (lc, 0, yL (mLu), lc.withAlpha (0.35f), 0, col.getBottom(), false); g.setGradientFill (lg); g.fillRoundedRectangle (col.withTop (yL (mLu)), 2); }
        g.setColour (K::mint.withAlpha (0.25f)); g.fillRect (col.getX() - 2, yL (tg + 1.5f), col.getWidth() + 4, yL (tg - 1.5f) - yL (tg + 1.5f));
        g.setColour (K::mint); g.fillRect (col.getX() - 3, yL (tg) - 1, col.getWidth() + 6, 2.0f);
        g.setFont (num (8.5f)); g.setColour (lc);
        g.drawText (mLu < -70 ? juce::String ("--") : juce::String (mLu, 1), juce::Rectangle<float> (r.getX(), r.getBottom() - 30, r.getWidth(), 13), juce::Justification::centred);
        g.setFont (cap (6.5f)); g.setColour (K::faint);
        g.drawText (mLu < -70 ? juce::String ("LUFS") : (std::abs (dd) <= 1.5f ? juce::String ("SUL TARGET") : (dd > 0 ? "+" : "") + juce::String (dd, 1) + " LU"), juce::Rectangle<float> (r.getX(), r.getBottom() - 16, r.getWidth(), 11), juce::Justification::centred);
    }
    g.setFont (cap (7.0f)); g.setColour (K::sub);
    for (int c = 0; c < 3; ++c) g.drawText (c == 0 ? "L" : c == 1 ? "R" : "LU", juce::Rectangle<float> (bars.getX() + c * (bw + 4) - 4, r.getY() + 3, bw + 8, 11), juce::Justification::centred);
}

void EasyStage::ideal (juce::Graphics& g, juce::Rectangle<float> p)
{   // IDEALE contro MIO: la fascia del genere (verde) e lo spettro medio del tuo master (ambra), come nel Metering Hub
    auto& E = page.ed;
    if (genreSel < 0) { const int t = (int) E.proc.value ("target"); genreSel = t == 5 || t == 6 ? 0 : 0; }
    const auto& G = genre (genreSel);
    p = p.withTrimmedLeft (34).withTrimmedRight (8).withTrimmedBottom (18).withTrimmedTop (28);
    g.setColour (K::lo.withAlpha (0.28f)); g.fillRoundedRectangle (p.expanded (4), 8);
    auto xOf = [&p] (float f) { return p.getX() + p.getWidth() * (float) (std::log (f / 20.0) / std::log (1000.0)); };
    auto yOf = [&p] (float d) { return p.getCentreY() - juce::jlimit (-1.0f, 1.0f, d / 15.0f) * p.getHeight() * 0.5f; };
    g.setFont (num (8.0f));
    for (float f : { 50.f, 100.f, 200.f, 500.f, 1000.f, 2000.f, 5000.f, 10000.f })
    { g.setColour (K::hi.withAlpha (0.05f)); g.fillRect (xOf (f), p.getY(), 1.0f, p.getHeight()); g.setColour (K::faint); g.drawText (f >= 1000 ? juce::String ((int) (f / 1000)) + "k" : juce::String ((int) f), juce::Rectangle<float> (xOf (f) - 14, p.getBottom() + 3, 28, 11), juce::Justification::centred); }
    for (int d : { -10, -5, 0, 5, 10 }) { g.setColour (K::hi.withAlpha (d == 0 ? 0.1f : 0.04f)); g.fillRect (p.getX(), yOf ((float) d), p.getWidth(), 1.0f); g.setColour (K::faint); g.drawText ((d > 0 ? "+" : "") + juce::String (d), juce::Rectangle<float> (p.getX() - 32, yOf ((float) d) - 6, 26, 12), juce::Justification::centredRight); }
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
    g.setColour (K::hi.withAlpha (0.06f)); g.fillRect (pl.getX(), yOf (0), pl.getWidth(), 1.0f);
    juce::Path pr, pm;
    for (int k = 0; k < kThirds; ++k) { const float x = xOf (k); if (k == 0) { pr.startNewSubPath (x, yOf (rt->thirds[k])); pm.startNewSubPath (x, yOf (page.ed.liveThirds[k])); } else { pr.lineTo (x, yOf (rt->thirds[k])); pm.lineTo (x, yOf (page.ed.liveThirds[k])); } }
    const bool live = page.ed.live.hasData();
    g.setColour (K::amber); g.strokePath (pr, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    if (live) { g.setColour (K::cyan); g.strokePath (pm, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded)); }
    g.setFont (num (8.0f)); g.setColour (K::faint);
    for (int k = 0; k < kThirds; k += 3) { const float fr = thirdHz (k); g.drawText (fr >= 1000 ? juce::String (fr / 1000, 1) + "k" : juce::String ((int) fr), juce::Rectangle<float> (xOf (k) - 16, pl.getBottom() - 4, 32, 12), juce::Justification::centred); }
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
