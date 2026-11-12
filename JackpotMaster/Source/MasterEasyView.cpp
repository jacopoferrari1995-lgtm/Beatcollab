// Modalita FACILE, stile Master Plan 2: onda viva al centro, INPUT con la zona ottimale, barra CRUSH / OTTIMO / SAFE, misure, e un banco di manopole.
#include "MasterEditor.h"

namespace jms
{
using namespace neo;
static const char* kViews[5] = { "ONDA", "SPETTRO", "GAIN", "REFERENCE", "ORECCHIE" };
static juce::String U (const char* s) { return juce::String::fromUTF8 (s); }

// zone della pagina (coordinate locali)
static const juce::Rectangle<float> rIn { 20, 12, 104, 394 }, rStage { 136, 12, 764, 394 }, rRes { 912, 12, 72, 394 }, rMet { 996, 12, 264, 394 }, rDeck { 20, 420, 1240, 302 };
static const float gx[6] = { 20, 300, 580, 900, 1130, 1260 };   // gruppi del banco

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
    pres  = mk ("e_pres", "PRESENZA", K::pink, K::red, false, [this] { const float v = ed.proc.value ("e_pres"); return v < 0.3f ? juce::String ("SOFT") : v < 0.7f ? juce::String ("NATURAL") : juce::String ("HARD"); });
    pres->setTooltip ("Come si comporta il limiter sui picchi: morbido (arrotonda) o duro (clipper, piu volume e grinta).");
    loud  = mk ("e_loud", "LOUD", K::pink, K::cyan, false, [this] { return "+" + juce::String (ed.proc.value ("e_loud"), 1); }); loud->big = true; loud->sub = "dB di volume in piu";
    loud->setTooltip ("Quanto alzi il volume. La corona esterna mostra quanto lavora il limiter.");
    speed = mk ("e_speed", "VELOCITA", K::violet, K::cyan, false, [this] { const float v = ed.proc.value ("e_speed"); return v < 0.33f ? juce::String ("SMOOTH") : v < 0.66f ? juce::String ("MEDIUM") : juce::String ("FAST"); });
    speed->setTooltip ("Rilascio del limiter: smooth (morbido) o fast (piu volume, piu movimento).");
    glue = mk ("e_glue", "GLUE", K::violet, K::blue, false, pc ("e_glue")); glue->setTooltip ("Compressore di bus 2:1 con rilascio automatico: tiene insieme il mix.");
    calm = mk ("e_calm", "CALM", K::violet, K::blue, false, pc ("e_calm")); calm->setTooltip ("Abbassa da solo le risonanze che saltano fuori: fango, durezze, sibilanti.");
    wide = mk ("e_wide", "WIDE", K::blue, K::cyan, true, [this] { const float w = ed.proc.value ("e_wide"); return juce::String (juce::roundToInt (100 + w * (w > 0 ? 80 : 100))) + "%"; });
    wide->setTooltip ("Allarga o stringe lo stereo sopra 120 Hz.");
    ceil = mk ("lm_ceil", "CEILING", K::mint, K::cyan, false, [this] { return juce::String (ed.proc.value ("lm_ceil"), 1) + " dB"; }); ceil->setTooltip ("Picco massimo in uscita (true peak).");
    trim = mk ("trim", "INPUT", K::cyan, K::blue, true, [this] { return fmt (ed.proc.value ("trim")) + " dB"; }); trim->setTooltip ("Livello d'ingresso. AUTO lo porta nella zona ottimale.");
    monoB = std::make_unique<neo::Toggle> (ed.P ("e_monob"), "BASSI MONO", K::cyan); addAndMakeVisible (*monoB);
    for (juce::Component* c : std::initializer_list<juce::Component*> { &stage, &input, &result, &meters }) addAndMakeVisible (c);
}

void EasyPage::resized()
{
    auto kb = [] (neo::Knob* k, float cx, float y, float s) { k->setBounds (juce::Rectangle<float> (cx - s * 0.5f - 8, y, s + 16, s + 34).toNearestInt()); };
    const float ky = kDeckY + 100, ks = 80;
    kb (low, 76, ky, ks); kb (mid, 160, ky, ks); kb (high, 244, ky, ks);
    kb (thick, 356, ky, ks); kb (warm, 440, ky, ks); kb (silk, 524, ky, ks);
    kb (pres, 602, ky + 34, 62); kb (speed, 878, ky + 34, 62);
    loud->setBounds (juce::Rectangle<float> (650, kDeckY + 44, 180, 180 + 30).toNearestInt());
    kb (glue, 945, ky + 6, 68); kb (calm, 1015, ky + 6, 68); kb (wide, 1085, ky + 6, 68);
    kb (ceil, 1195, ky + 6, 68);
    monoB->setBounds (juce::Rectangle<float> (1146, kDeckY + 230, 110, 24).toNearestInt());
    trim->setBounds (juce::Rectangle<float> (rIn.getX() + 10, rIn.getBottom() - 108, 84, 102).toNearestInt());
    stage.setBounds (rStage.withTrimmedTop (48).reduced (12, 0).withTrimmedBottom (12).toNearestInt());
    input.setBounds (juce::Rectangle<float> (rIn.getX(), rIn.getY() + 40, rIn.getWidth(), rIn.getHeight() - 152).toNearestInt());
    result.setBounds (rRes.withTrimmedTop (36).withTrimmedBottom (10).toNearestInt());
    meters.setBounds (rMet.withTrimmedTop (42).reduced (14, 0).withTrimmedBottom (12).toNearestInt());
    bg = {};
}

// ---------------------------------------------------------------------------------------------------- statico (in cache)
void EasyPage::paintStatic (juce::Graphics& g)
{
    glass (g, rIn, 20, K::cyan, 0.8f); glass (g, rStage, 22, K::violet); glass (g, rRes, 20, K::pink, 0.6f); glass (g, rMet, 20, K::mint, 0.7f); glass (g, rDeck, 24, K::pink, 0.6f);
    auto head = [&] (juce::Rectangle<float> r, const char* t) { g.setFont (f (10.5f, 2)); g.setColour (K::sub); g.drawText (t, r.withHeight (36).withTrimmedLeft (16), juce::Justification::centredLeft); };
    head (rIn, "INPUT"); head (rMet, "MISURE");
    g.setFont (f (9.0f, 2)); g.setColour (K::sub); g.drawText ("RISULTATO", rRes.withHeight (34), juce::Justification::centred);
    well (g, rStage.withTrimmedTop (48).reduced (12, 0).withTrimmedBottom (12), 14);
    // banco: gruppi con titolo, sottotitolo e divisori sfumati
    static const char* gt[5] = { "EQ", "COLORE", "VOLUME", "DINAMICA E SPAZIO", "USCITA" };
    static const char* gs[5] = { "bande fisse, curve da master", "valvola  \xc2\xb7  nastro  \xc2\xb7  seta", "limiter intelligente", "colla, risonanze, stereo", "picco massimo" };
    static const juce::Colour gc[5] = { K::amber, K::pink, K::cyan, K::violet, K::mint };
    for (int i = 0; i < 5; ++i)
    {
        const float x0 = gx[i], x1 = gx[i + 1];
        g.setColour (gc[i]); g.fillRoundedRectangle (x0 + 22, kDeckY + 22, 3, 26, 1.5f);
        g.setFont (f (12.0f, 2)); g.setColour (K::ink); g.drawText (gt[i], juce::Rectangle<float> (x0 + 32, kDeckY + 19, x1 - x0 - 40, 16), juce::Justification::centredLeft);
        g.setFont (f (9.0f)); g.setColour (K::faint); g.drawText (U (gs[i]), juce::Rectangle<float> (x0 + 32, kDeckY + 35, x1 - x0 - 40, 14), juce::Justification::centredLeft);
        if (i > 0) { juce::ColourGradient dv (juce::Colours::white.withAlpha (0.0f), 0, kDeckY + 30, juce::Colours::white.withAlpha (0.0f), 0, kDeckY + kDeckH - 30, false); dv.addColour (0.5, juce::Colours::white.withAlpha (0.08f)); g.setGradientFill (dv); g.fillRect (x0, kDeckY + 30, 1.0f, kDeckH - 60); }
    }
    // alone dietro il LOUD
    { const float cx = 740, cy = kDeckY + 44 + 90; juce::ColourGradient hg (K::violet.withAlpha (0.18f), cx, cy, K::violet.withAlpha (0.0f), cx + 160, cy, true); g.setGradientFill (hg); g.fillEllipse (cx - 160, cy - 160, 320, 320); }
    g.setFont (f (9.0f)); g.setColour (K::faint);
    g.drawText (U ("doppio clic: torna a zero  \xc2\xb7  shift: regolazione fine  \xc2\xb7  passa sopra una manopola per sapere cosa fa"), juce::Rectangle<float> (42, kDeckY + kDeckH - 30, 600, 14), juce::Justification::centredLeft);
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
    // schede delle viste e pillole in cima al palco
    float x = rStage.getX() + 14;
    for (int i = 0; i < 5; ++i) { const float w = i == 3 ? 96.0f : i == 4 ? 90.0f : 76.0f; viewR[i] = { x, rStage.getY() + 12, w, 26 }; x += w + 4; pill (g, viewR[i], kViews[i], view == i, K::violet, 9.5f); }
    const auto& T = target ((int) ed.proc.value ("target"));
    cmpR = { rStage.getRight() - 106, rStage.getY() + 12, 92, 26 }; targetR = { cmpR.getX() - 150, rStage.getY() + 12, 144, 26 };
    pill (g, targetR, juce::String (T.name) + "  " + juce::String (T.lufs, 0) + U (" \xe2\x96\xbe"), true, K::mint, 9.5f);
    pill (g, cmpR, ed.proc.value ("compare") > 0.5f ? "ORIGINALE" : "BYPASS", ed.proc.value ("compare") > 0.5f, K::cyan, 9.5f);
}
void EasyPage::mouseDown (const juce::MouseEvent& e)
{
    auto& P = ed.proc;
    for (int i = 0; i < 5; ++i) if (viewR[i].contains (e.position)) { view = i; repaint(); return; }
    if (cmpR.contains (e.position)) { auto* p = ed.P ("compare"); p->setValueNotifyingHost (p->getValue() > 0.5f ? 0.0f : 1.0f); repaint(); return; }
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
    for (auto* k : knobs) { const float v = k->param->getValue() + (k == loud ? k->activity * 3 : 0); if (v != k->shown) { k->shown = v; k->repaint(); } }
    monoB->repaint();
}

// ---------------------------------------------------------------------------------------------------- INPUT
void EasyInput::paint (juce::Graphics& g)
{
    auto& P = page.ed.proc;
    auto m = juce::Rectangle<float> (20, 4, 20, (float) getHeight() - 74);
    well (g, m.expanded (4), 9);
    auto yL = [&m] (float db) { return m.getBottom() - juce::jlimit (0.0f, 1.0f, (db + 36.0f) / 36.0f) * m.getHeight(); };
    const float lo = Processor::kInOpt - 2.5f, hi = Processor::kInOpt + 2.5f, s = P.inS.load(), a = P.inAvg.load();
    auto zone = juce::Rectangle<float>::leftTopRightBottom (m.getX() - 5, yL (hi), m.getRight() + 5, yL (lo));
    g.setColour (K::mint.withAlpha (0.12f)); g.fillRoundedRectangle (zone, 4); g.setColour (K::mint.withAlpha (0.6f)); g.drawRoundedRectangle (zone, 4, 1.0f);
    if (s > -60)
    {
        auto lv = juce::Rectangle<float>::leftTopRightBottom (m.getX() + 3, yL (s), m.getRight() - 3, m.getBottom() - 1);
        juce::ColourGradient lg (K::blue, 0, lv.getBottom(), s > hi ? K::red : K::cyan, 0, lv.getY(), false); g.setGradientFill (lg); g.fillRoundedRectangle (lv, 4);
    }
    if (a > -60) { g.setColour (juce::Colours::white.withAlpha (0.3f)); g.fillRoundedRectangle (m.getX() - 9, yL (a) - 3, m.getWidth() + 18, 6, 3); g.setColour (juce::Colours::white); g.fillRoundedRectangle (m.getX() - 7, yL (a) - 1.5f, m.getWidth() + 14, 3, 1.5f); }
    g.setFont (num (8.5f)); g.setColour (K::faint);
    for (int d : { -30, -24, -18, -12, -6, 0 }) g.drawText (juce::String (d), juce::Rectangle<float> (m.getRight() + 10, yL ((float) d) - 5, 30, 10), juce::Justification::centredLeft);
    g.setFont (f (8.5f, 2)); g.setColour (K::mint); g.drawText ("OK", juce::Rectangle<float> (m.getRight() + 10, yL (Processor::kInOpt) - 6, 30, 12), juce::Justification::centredLeft);
    juce::String st = "PLAY"; juce::Colour c = K::faint;
    if (a > -60) { if (a > hi) { st = "ABBASSA"; c = K::red; } else if (a < lo) { st = "ALZA"; c = K::amber; } else { st = "OK"; c = K::mint; } }
    glowText (g, st, { 0, (float) getHeight() - 64, (float) getWidth(), 18 }, c, f (13.0f, 2), juce::Justification::centred);
    autoR = { 14, (float) getHeight() - 38, (float) getWidth() - 28, 26 };
    pill (g, autoR, "AUTO", false, K::cyan, 10.0f, isMouseOver());
}
void EasyInput::mouseDown (const juce::MouseEvent& e) { if (autoR.contains (e.position)) { page.ed.proc.autoInput(); repaint(); } }

// ---------------------------------------------------------------------------------------------------- RISULTATO
void EasyResult::paint (juce::Graphics& g)
{
    auto& P = page.ed.proc;
    auto b = juce::Rectangle<float> (getWidth() * 0.5f - 7, 30, 14, (float) getHeight() - 70);
    juce::ColourGradient cg (K::red, 0, b.getY(), K::cyan, 0, b.getBottom(), false); cg.addColour (0.5, K::mint); cg.addColour (0.22, K::amber);
    g.setGradientFill (cg); g.setOpacity (0.18f); g.fillRoundedRectangle (b.expanded (5, 0), 10); g.setOpacity (0.6f); g.fillRoundedRectangle (b, 7); g.setOpacity (1.0f);
    g.setFont (f (8.5f, 2));
    g.setColour (K::red); g.drawText ("CRUSH", juce::Rectangle<float> (0, 2, (float) getWidth(), 14), juce::Justification::centred);
    g.setColour (K::cyan); g.drawText ("SAFE", juce::Rectangle<float> (0, (float) getHeight() - 22, (float) getWidth(), 14), juce::Justification::centred);
    g.setColour (K::mint.withAlpha (0.8f)); g.fillRect (b.getX() - 10, b.getCentreY() - 0.5f, 6.0f, 1.0f); g.fillRect (b.getRight() + 4, b.getCentreY() - 0.5f, 6.0f, 1.0f);
    sm += (P.crush.load() - sm) * 0.12f;
    const float y = b.getBottom() - sm * b.getHeight();
    const auto c = sm > 0.7f ? K::red : sm < 0.3f ? K::cyan : K::mint;
    g.setColour (c.withAlpha (0.16f)); g.fillEllipse (b.getCentreX() - 24, y - 24, 48, 48);
    g.setColour (c.withAlpha (0.35f)); g.fillEllipse (b.getCentreX() - 14, y - 14, 28, 28);
    g.setColour (juce::Colours::white); g.fillEllipse (b.getCentreX() - 6, y - 6, 12, 12);
    g.setColour (c); g.drawEllipse (b.getCentreX() - 8, y - 8, 16, 16, 2.0f);
}

// ---------------------------------------------------------------------------------------------------- MISURE
void EasyMeters::paint (juce::Graphics& g)
{
    auto& P = page.ed.proc; const auto& T = target ((int) P.value ("target"));
    const float W = (float) getWidth();
    const float S = P.lufsS.load(), I = P.lufsI.load(), M = P.lufsM.load(), tp = P.tpMax.load(), gr = P.grAvg.load();
    const float d = S - T.lufs; const auto c = S < -69 ? K::faint : std::abs (d) <= 1 ? K::mint : d > 1 ? K::red : K::amber;
    glowText (g, S < -69 ? juce::String ("--.-") : juce::String (S, 1), { 0, 0, W, 50 }, c, num (44.0f), juce::Justification::centredLeft);
    g.setFont (f (9.0f, 2)); g.setColour (K::sub); g.drawText ("LUFS SHORT", juce::Rectangle<float> (0, 6, W, 14), juce::Justification::centredRight);
    g.setFont (f (9.5f)); g.setColour (c); g.drawText (S < -69 ? juce::String ("in attesa di audio") : "target " + juce::String (T.lufs, 0) + "   " + fmt (d) + " dB", juce::Rectangle<float> (0, 24, W, 16), juce::Justification::centredRight);
    auto bar = juce::Rectangle<float> (0, 58, W, 8); well (g, bar.expanded (0, 2), 5);
    auto xL = [&bar] (float l) { return bar.getX() + juce::jlimit (0.0f, 1.0f, (l + 30.0f) / 30.0f) * bar.getWidth(); };
    g.setColour (K::mint.withAlpha (0.35f)); g.fillRect (juce::Rectangle<float>::leftTopRightBottom (xL (T.lufs - 1), bar.getY() - 2, xL (T.lufs + 1), bar.getBottom() + 2));
    if (S > -60) { juce::ColourGradient lg (K::blue, bar.getX(), 0, c, xL (S), 0, false); g.setGradientFill (lg); g.fillRoundedRectangle (bar.withRight (xL (S)), 4); }
    struct V { const char* n; juce::String v; juce::Colour c; };
    const float plr = (tp > -60 && I > -60) ? tp - I : -99;
    V vals[] = { { "INTEGRATO", I < -69 ? juce::String ("--") : juce::String (I, 1), K::cyan }, { "MOMENTARY", M < -69 ? juce::String ("--") : juce::String (M, 1), K::cyan },
                 { "TRUE PEAK", tp < -60 ? juce::String ("--") : juce::String (tp, 1), tp > T.tp ? K::red : K::mint }, { "PLR", plr < -90 ? juce::String ("--") : juce::String (plr, 1), plr > 0 && plr < 7 ? K::red : K::amber },
                 { "LIMITER", gr < 0.05f ? juce::String ("0.0") : "-" + juce::String (gr, 1), gr > 6 ? K::red : K::pink }, { "LRA", juce::String (P.lra.load(), 1), K::violet } };
    const float tw = (W - 8) / 2, th = 44;
    for (int i = 0; i < 6; ++i)
    {
        auto r = juce::Rectangle<float> ((i % 2) * (tw + 8), 80 + (i / 2) * (th + 8), tw, th);
        g.setColour (juce::Colours::white.withAlpha (0.035f)); g.fillRoundedRectangle (r, 10); g.setColour (juce::Colours::white.withAlpha (0.06f)); g.drawRoundedRectangle (r.reduced (0.5f), 10, 1);
        g.setFont (f (8.0f, 2)); g.setColour (K::faint); g.drawText (vals[i].n, r.reduced (11, 5).withHeight (12), juce::Justification::centredLeft);
        g.setFont (num (16.0f)); g.setColour (vals[i].c); g.drawText (vals[i].v, r.reduced (11, 4).withTrimmedTop (13), juce::Justification::centredLeft);
    }
    for (int ch = 0; ch < 2; ++ch)
    {
        pk[ch] = juce::jmax (P.outPk[ch].load(), pk[ch] * 0.92f);
        auto r = juce::Rectangle<float> (16, 244 + ch * 12.0f, W - 16, 6);
        g.setColour (juce::Colours::white.withAlpha (0.05f)); g.fillRoundedRectangle (r, 3);
        const float db = juce::Decibels::gainToDecibels (pk[ch], -60.0f), t = juce::jlimit (0.0f, 1.0f, (db + 48.0f) / 48.0f);
        juce::ColourGradient pg (K::blue, r.getX(), 0, db > T.tp ? K::red : K::cyan, r.getX() + r.getWidth() * t, 0, false); g.setGradientFill (pg); g.fillRoundedRectangle (r.withWidth (juce::jmax (0.0f, r.getWidth() * t)), 3);
        g.setFont (f (8.0f, 2)); g.setColour (K::faint); g.drawText (ch ? "R" : "L", juce::Rectangle<float> (0, r.getY() - 3, 12, 12), juce::Justification::centredLeft);
    }
    const float cr = P.crush.load(); juce::String adv; juce::Colour ac = K::faint;
    if (S < -60) adv = "premi play";
    else if (cr > 0.7f) { adv = "troppo schiacciato: abbassa LOUD"; ac = K::red; }
    else if (cr < 0.3f) { adv = "c'e margine: puoi alzare LOUD"; ac = K::cyan; }
    else { adv = "nel punto giusto"; ac = K::mint; }
    glowText (g, adv, { 0, 276, W, 18 }, ac, f (11.0f, 2));
    g.setFont (f (8.5f)); g.setColour (K::faint);
    g.drawText ("latenza " + juce::String (P.getLatencySamples() * 1000.0 / juce::jmax (1.0, P.sampleRate()), 1) + " ms compensata", juce::Rectangle<float> (0, 298, W, 14), juce::Justification::centredLeft);
}

// ---------------------------------------------------------------------------------------------------- PALCO
void EasyStage::paint (juce::Graphics& g)
{
    auto p = getLocalBounds().toFloat().reduced (2);
    g.saveState(); juce::Path clip; clip.addRoundedRectangle (getLocalBounds().toFloat(), 14); g.reduceClipRegion (clip);
    switch (page.view) { case 1: spectrum (g, p); break; case 2: gain (g, p); break; case 3: ref (g, p); break; case 4: ears (g, p); break; default: wave (g, p); break; }
    g.restoreState();
}
void EasyStage::wave (juce::Graphics& g, juce::Rectangle<float> p)
{
    auto& P = page.ed.proc; const int N = Processor::kHist, w0 = P.hW.load();
    const float cy = p.getCentreY(), h = p.getHeight() * 0.5f - 14;
    auto lin = [] (float db) { return juce::jlimit (0.0f, 1.0f, juce::Decibels::decibelsToGain (db)); };
    for (int k = 1; k < 8; ++k) { g.setColour (juce::Colours::white.withAlpha (0.025f)); g.drawVerticalLine ((int) (p.getX() + p.getWidth() * k / 8), p.getY(), p.getBottom()); }
    g.setColour (juce::Colours::white.withAlpha (0.06f)); g.drawHorizontalLine ((int) cy, p.getX(), p.getRight());
    const float cl = lin (P.value ("lm_ceil"));
    {
        juce::Path cpth; cpth.startNewSubPath (p.getX(), cy - cl * h); cpth.lineTo (p.getRight(), cy - cl * h); cpth.startNewSubPath (p.getX(), cy + cl * h); cpth.lineTo (p.getRight(), cy + cl * h);
        juce::Path dash; const float dl[] = { 4.0f, 6.0f }; juce::PathStrokeType (1.0f).createDashedStroke (dash, cpth, dl, 2); g.setColour (K::red.withAlpha (0.4f)); g.fillPath (dash);
    }
    const int NX = (int) p.getWidth() / 2;
    std::vector<float> in ((size_t) NX), out ((size_t) NX), gr ((size_t) NX);
    for (int x = 0; x < NX; ++x)
    {
        const int a = x * N / NX, b = juce::jmax (a + 1, (x + 1) * N / NX);
        float vi = -100, vo = -100, vg = 0;
        for (int k = a; k < b; ++k) { const int i = (w0 + k) % N; vi = juce::jmax (vi, P.hIn[i].load()); vo = juce::jmax (vo, P.hOut[i].load()); vg = juce::jmax (vg, P.hGrM[i].load() + P.hGrL[i].load()); }
        in[(size_t) x] = vi > -60 ? lin (vi) : 0; out[(size_t) x] = vo > -60 ? lin (vo) : 0; gr[(size_t) x] = vg;
    }
    auto smooth = [] (std::vector<float>& v) { auto c = v; for (size_t i = 1; i + 1 < v.size(); ++i) v[i] = 0.25f * c[i - 1] + 0.5f * c[i] + 0.25f * c[i + 1]; };
    smooth (out); smooth (in);
    auto mirror = [&] (const std::vector<float>& v)
    {
        juce::Path pt; pt.startNewSubPath (p.getX(), cy);
        for (int x = 0; x < NX; ++x) pt.lineTo (p.getX() + x * 2.0f, cy - v[(size_t) x] * h);
        for (int x = NX - 1; x >= 0; --x) pt.lineTo (p.getX() + x * 2.0f, cy + v[(size_t) x] * h);
        pt.closeSubPath(); return pt;
    };
    auto edge = [&] (const std::vector<float>& v, float sgn)
    {
        juce::Path pt; for (int x = 0; x < NX; ++x) { const float xx = p.getX() + x * 2.0f, yy = cy - sgn * v[(size_t) x] * h; if (x == 0) pt.startNewSubPath (xx, yy); else pt.lineTo (xx, yy); } return pt;
    };
    g.setColour (juce::Colours::white.withAlpha (0.07f)); g.fillPath (mirror (in));
    {
        juce::ColourGradient og (K::cyan.withAlpha (0.95f), 0, cy - h, K::cyan.withAlpha (0.95f), 0, cy + h, false);
        og.addColour (0.5, K::violet.withAlpha (0.35f)); og.addColour (0.3, K::blue.withAlpha (0.7f)); og.addColour (0.7, K::blue.withAlpha (0.7f));
        g.setGradientFill (og); g.fillPath (mirror (out));
        glowStroke (g, edge (out, 1), K::cyan.withAlpha (0.9f), 1.2f, 0.8f); glowStroke (g, edge (out, -1), K::cyan.withAlpha (0.9f), 1.2f, 0.8f);
    }
    {
        juce::Path gp; gp.startNewSubPath (p.getX(), p.getY());
        for (int x = 0; x < NX; ++x) gp.lineTo (p.getX() + x * 2.0f, p.getY() + juce::jmin (1.0f, gr[(size_t) x] / 12.0f) * h * 0.7f);
        gp.lineTo (p.getRight(), p.getY()); gp.closeSubPath();
        juce::ColourGradient gg (K::pink.withAlpha (0.75f), 0, p.getY(), K::pink.withAlpha (0.05f), 0, p.getY() + h * 0.7f, false); g.setGradientFill (gg); g.fillPath (gp);
    }
    { juce::ColourGradient ng (K::cyan.withAlpha (0.0f), p.getRight() - 40, 0, K::cyan.withAlpha (0.18f), p.getRight(), 0, false); g.setGradientFill (ng); g.fillRect (p.withLeft (p.getRight() - 40)); }
    g.setFont (f (9.0f)); g.setColour (K::faint);
    g.drawText (U ("ultimi 20 s   \xc2\xb7   velo = ingresso   \xc2\xb7   ciano = uscita   \xc2\xb7   rosa dall'alto = limiter"), p.reduced (12, 8), juce::Justification::bottomLeft);
}
void EasyStage::spectrum (juce::Graphics& g, juce::Rectangle<float> p)
{
    const int NP = 300; const double sr = page.ed.proc.sampleRate();
    anIn.run (page.ed.proc.ringIn, specIn, NP, 20, 20000, sr); anOut.run (page.ed.proc.ringOut, specOut, NP, 20, 20000, sr);
    auto xOf = [&p] (float fr) { return p.getX() + p.getWidth() * std::log (fr / 20.0f) / std::log (1000.0f); };
    for (float fr : { 50.f, 100.f, 200.f, 500.f, 1000.f, 2000.f, 5000.f, 10000.f })
    { g.setColour (juce::Colours::white.withAlpha (0.04f)); g.drawVerticalLine ((int) xOf (fr), p.getY(), p.getBottom()); g.setColour (K::faint); g.setFont (num (8.5f)); g.drawText (fr >= 1000 ? juce::String ((int) fr / 1000) + "k" : juce::String ((int) fr), juce::Rectangle<float> (xOf (fr) - 14, p.getBottom() - 16, 28, 12), juce::Justification::centred); }
    auto path = [&] (const float* s, bool fill)
    {
        juce::Path pt; if (fill) pt.startNewSubPath (p.getX(), p.getBottom());
        for (int i = 0; i < NP; ++i) { const float x = p.getX() + p.getWidth() * (i + 0.5f) / NP, y = p.getBottom() - juce::jlimit (0.0f, 1.0f, (s[i] + 96.0f) / 90.0f) * p.getHeight(); if (i == 0 && ! fill) pt.startNewSubPath (x, y); else pt.lineTo (x, y); }
        if (fill) { pt.lineTo (p.getRight(), p.getBottom()); pt.closeSubPath(); }
        return pt;
    };
    g.setColour (juce::Colours::white.withAlpha (0.3f)); g.strokePath (path (specIn, false), juce::PathStrokeType (1.0f));
    juce::ColourGradient sg (K::violet.withAlpha (0.45f), 0, p.getY(), K::cyan.withAlpha (0.04f), 0, p.getBottom(), false); g.setGradientFill (sg); g.fillPath (path (specOut, true));
    glowStroke (g, path (specOut, false), K::cyan, 1.4f);
    g.setFont (f (9.0f)); g.setColour (K::faint); g.drawText (U ("bianco = prima   \xc2\xb7   ciano = dopo il master"), p.reduced (12, 8), juce::Justification::topLeft);
}
void EasyStage::gain (juce::Graphics& g, juce::Rectangle<float> p)
{
    auto& P = page.ed.proc; const int N = Processor::kHist, w0 = P.hW.load(); const auto& T = target ((int) P.value ("target"));
    auto yS = [&p] (float l) { return p.getY() + 16 + (1.0f - juce::jlimit (0.0f, 1.0f, (l + 30.0f) / 30.0f)) * (p.getHeight() * 0.55f); };
    auto yG = [&p] (float gr) { return p.getBottom() - 20 - juce::jlimit (0.0f, 1.0f, gr / 12.0f) * (p.getHeight() * 0.3f); };
    g.setColour (K::mint.withAlpha (0.35f)); g.drawHorizontalLine ((int) yS (T.lufs), p.getX(), p.getRight());
    g.setFont (num (8.5f)); g.setColour (K::mint); g.drawText ("target " + juce::String (T.lufs, 0), juce::Rectangle<float> (p.getRight() - 90, yS (T.lufs) - 14, 80, 12), juce::Justification::centredRight);
    juce::Path ls, gl; bool s1 = false;
    for (int k = 0; k < N; ++k)
    {
        const int i = (w0 + k) % N; const float x = p.getX() + p.getWidth() * k / (N - 1);
        const float s = P.hS[i].load(); if (s > -60) { if (! s1) { ls.startNewSubPath (x, yS (s)); s1 = true; } else ls.lineTo (x, yS (s)); }
        if (k == 0) gl.startNewSubPath (x, yG (0));
        gl.lineTo (x, yG (P.hGrM[i].load() + P.hGrL[i].load()));
    }
    gl.lineTo (p.getRight(), yG (0)); gl.closeSubPath();
    juce::ColourGradient gg (K::pink.withAlpha (0.1f), 0, yG (0), K::pink.withAlpha (0.7f), 0, yG (12), false); g.setGradientFill (gg); g.fillPath (gl);
    glowStroke (g, ls, K::amber, 1.6f);
    g.setFont (f (9.0f)); g.setColour (K::faint); g.drawText (U ("ambra = LUFS short nel tempo   \xc2\xb7   rosa = quanto lavora il limiter (12 dB)"), p.reduced (12, 8), juce::Justification::topLeft);
}
void EasyStage::ref (juce::Graphics& g, juce::Rectangle<float> p)
{
    auto& P = page.ed.proc; auto rt = P.reference();
    loadR = { p.getX() + 12, p.getY() + 10, 140, 26 }; listenR = { p.getX() + 158, p.getY() + 10, 170, 26 };
    pill (g, loadR, P.refLoading.load() ? "ANALISI..." : "CARICA PEZZO", false, K::amber, 9.5f);
    pill (g, listenR, P.refListen.load() ? "ASCOLTI IL RIFERIMENTO" : "ASCOLTA RIFERIMENTO", P.refListen.load(), K::cyan, 9.5f);
    if (! rt) { g.setFont (f (12.0f)); g.setColour (K::sub); g.drawFittedText ("Carica un master che ti piace: confronti spettro e loudness e lo ascolti allo stesso volume.", p.reduced (60).toNearestInt(), juce::Justification::centred, 3); return; }
    g.setFont (f (10.0f, 2)); g.setColour (K::amber); g.drawText (rt->name + "   " + juce::String (rt->lufs, 1) + " LUFS   picco " + juce::String (rt->tp, 1), juce::Rectangle<float> (p.getX() + 340, p.getY() + 10, p.getWidth() - 350, 26), juce::Justification::centredLeft);
    auto pl = p.withTrimmedTop (48).reduced (14, 12);
    auto xOf = [&pl] (int k) { return pl.getX() + pl.getWidth() * (k + 0.5f) / kThirds; };
    auto yOf = [&pl] (float d) { return pl.getCentreY() - juce::jlimit (-15.0f, 15.0f, d) / 15.0f * pl.getHeight() * 0.5f; };
    g.setColour (juce::Colours::white.withAlpha (0.06f)); g.drawHorizontalLine ((int) yOf (0), pl.getX(), pl.getRight());
    juce::Path pr, pm;
    for (int k = 0; k < kThirds; ++k) { const float x = xOf (k); if (k == 0) { pr.startNewSubPath (x, yOf (rt->thirds[k])); pm.startNewSubPath (x, yOf (page.ed.liveThirds[k])); } else { pr.lineTo (x, yOf (rt->thirds[k])); pm.lineTo (x, yOf (page.ed.liveThirds[k])); } }
    glowStroke (g, pr, K::amber, 1.8f);
    if (page.ed.live.hasData()) glowStroke (g, pm, K::cyan, 1.8f);
    g.setFont (num (8.0f)); g.setColour (K::faint);
    for (int k = 0; k < kThirds; k += 3) { const float fr = thirdHz (k); g.drawText (fr >= 1000 ? juce::String (fr / 1000, 1) + "k" : juce::String ((int) fr), juce::Rectangle<float> (xOf (k) - 16, pl.getBottom() - 12, 32, 12), juce::Justification::centred); }
}
void EasyStage::ears (juce::Graphics& g, juce::Rectangle<float> p)
{
    const float t = (float) (juce::Time::getMillisecondCounter() % 4000) / 4000.0f, cx = p.getCentreX(), cy = p.getCentreY();
    for (int k = 0; k < 3; ++k) { const float ph = std::fmod (t + k / 3.0f, 1.0f), r = 30 + ph * 160; g.setColour (K::violet.withAlpha ((1 - ph) * 0.25f)); g.drawEllipse (cx - r, cy - r, r * 2, r * 2, 1.5f); }
    glowText (g, "ASCOLTA", { cx - 200, cy - 22, 400, 44 }, K::ink.withAlpha (0.8f), f (30.0f, 2), juce::Justification::centred);
    g.setFont (f (11.0f)); g.setColour (K::sub); g.drawText ("niente grafici: giudica solo con le orecchie", juce::Rectangle<float> (cx - 200, cy + 26, 400, 18), juce::Justification::centred);
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
