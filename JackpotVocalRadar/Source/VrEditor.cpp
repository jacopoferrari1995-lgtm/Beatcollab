// JACKPOT VOCAL RADAR: grafica scura di FX e Master. Tutto quello che fa deve vedersi:
// chi e' questa copia (voce o beat) e se e' collegata, dove il beat si abbassa (curva sulle frequenze), quando (corsie nel tempo),
// e una riga che dice a parole cosa sta succedendo adesso.
#include "VrEditor.h"

namespace vr
{
using namespace neo;
static juce::String U (const char* s) { return juce::String::fromUTF8 (s); }
static void panel (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour edge, bool display = false)
{
    if (skin::cur() != 3) { jlook::softShadow (g, r, 14); jlook::card (g, r, 14, edge, true, false, false); if (display) { g.setColour (K::lo.withAlpha (skin::light() ? 0.06f : 0.25f)); g.fillRoundedRectangle (r.reduced (6), 10); } return; }
    const float rad = 16;
    g.setColour (K::lo.withAlpha (0.35f)); g.fillRoundedRectangle (r.translated (0, 3), rad);
    juce::ColourGradient bgc (display ? K::disp0 : K::panel0, 0, r.getY(), display ? K::disp1 : K::panel1, 0, r.getBottom(), false); g.setGradientFill (bgc); g.fillRoundedRectangle (r, rad);
    juce::ColourGradient tint (edge.withAlpha (0.10f), 0, r.getY(), edge.withAlpha (0.0f), 0, r.getY() + juce::jmin (140.0f, r.getHeight()), false); g.setGradientFill (tint); g.fillRoundedRectangle (r, rad);
    g.setColour (K::hi.withAlpha (0.07f)); g.drawRoundedRectangle (r.reduced (0.5f), rad, 1.0f);
}
static void title (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& t, juce::Colour c, const juce::String& sub = {})
{
    g.setFont (cap (9.5f)); g.setColour (c); g.drawText (t, juce::Rectangle<float> (r.getX() + 18, r.getY() + 12, 400, 14), juce::Justification::centredLeft);
    if (sub.isNotEmpty()) { g.setFont (f (9.5f)); g.setColour (K::faint); g.drawText (sub, juce::Rectangle<float> (r.getX() + 18, r.getY() + 27, r.getWidth() - 36, 13), juce::Justification::centredLeft); }
}
struct Zone { float lo, hi; const char* name; const char* id; };
static const Zone zones[3] = { { 160, 480, "CORPO", "body" }, { 1800, 5500, "PAROLE", "pres" }, { 7000, 12000, "ARIA", "air" } };

// ======================================================================= cosa sta facendo
void SpaceView::paint (juce::Graphics& g)
{
    auto& P = ed.proc; const bool beat = P.beatMode();
    auto b = getLocalBounds().toFloat();
    auto top = juce::Rectangle<float> (54, 64, b.getWidth() - 74, beat ? 126 : 196);
    auto bot = juce::Rectangle<float> (54, top.getBottom() + 22, b.getWidth() - 74, b.getHeight() - top.getBottom() - 44);
    const float axisY = beat ? bot.getBottom() : top.getBottom();
    auto xOf = [&] (float hz) { return top.getX() + top.getWidth() * std::log (hz / 60.0f) / std::log (14000.0f / 60.0f); };
    auto yS = [&] (float db) { return top.getBottom() - top.getHeight() * juce::jlimit (0.0f, 1.0f, (db + 84.0f) / 72.0f); };
    for (auto& z : zones)
    {
        const float w = beat ? P.v (z.id) : 0.5f, x0 = xOf (z.lo), x1 = xOf (z.hi);
        g.setColour (K::amber.withAlpha (0.03f + 0.10f * w)); g.fillRect (juce::Rectangle<float> (x0, top.getY(), x1 - x0, axisY - top.getY()));
        g.setFont (cap (8.5f)); g.setColour (K::amber.withAlpha (0.45f + 0.5f * w));
        g.drawText (juce::String (z.name) + (beat ? "  " + juce::String (juce::roundToInt (w * 100)) + "%" : juce::String()), juce::Rectangle<float> (x0 - 20, top.getY() - 14, x1 - x0 + 40, 12), juce::Justification::centred);
    }
    for (float hz : { 100.f, 200.f, 500.f, 1000.f, 2000.f, 5000.f, 10000.f })
    {
        g.setColour (K::hi.withAlpha (0.05f)); g.fillRect (xOf (hz), top.getY(), 1.0f, axisY - top.getY());
        g.setFont (num (8.5f)); g.setColour (K::faint); g.drawText (hz >= 1000 ? juce::String (hz / 1000, 0) + "k" : juce::String ((int) hz), juce::Rectangle<float> (xOf (hz) - 16, axisY + 3, 32, 11), juce::Justification::centred);
    }
    auto curve = [&] (const float* lv, auto yf, const juce::Rectangle<float>& area, bool close)
    {
        juce::Path p;
        for (int i = 0; i <= 120; ++i)
        {
            const float u = i / 120.0f, hz = 60.0f * std::pow (14000.0f / 60.0f, u);
            const float fb = juce::jlimit (0.0f, (float) (NB - 1), std::log (hz / 80.0f) / std::log (150.0f) * (NB - 1));
            const int b0 = (int) fb, b1 = juce::jmin (NB - 1, b0 + 1); const float fr = fb - b0, t = fr * fr * (3 - 2 * fr);
            const float y = yf (lv[b0] * (1 - t) + lv[b1] * t);
            if (i == 0) p.startNewSubPath (area.getX() + area.getWidth() * u, y); else p.lineTo (area.getX() + area.getWidth() * u, y);
        }
        if (close) { p.lineTo (area.getRight(), area.getBottom()); p.lineTo (area.getX(), area.getBottom()); p.closeSubPath(); }
        return p;
    };
    if (beat) { auto pb = curve (smB, yS, top, true); g.setColour (K::cyan.withAlpha (0.16f)); g.fillPath (pb); g.setColour (K::cyan.withAlpha (0.6f)); g.strokePath (curve (smB, yS, top, false), juce::PathStrokeType (1.2f)); }
    { auto pv = curve (smV, yS, top, false); g.setColour (K::pink.withAlpha (0.25f)); g.strokePath (pv, juce::PathStrokeType (5.0f)); g.setColour (K::pink); g.strokePath (pv, juce::PathStrokeType (1.8f)); }
    g.setFont (cap (8.5f));
    g.setColour (K::pink); g.fillEllipse (top.getRight() - 150, top.getY() + 6, 7, 7); g.drawText ("VOCE", juce::Rectangle<float> (top.getRight() - 138, top.getY() + 2, 50, 14), juce::Justification::centredLeft);
    if (beat) { g.setColour (K::cyan); g.fillEllipse (top.getRight() - 84, top.getY() + 6, 7, 7); g.drawText ("BEAT", juce::Rectangle<float> (top.getRight() - 72, top.getY() + 2, 50, 14), juce::Justification::centredLeft); }
    g.setFont (num (8.5f)); g.setColour (K::faint); g.drawText ("SPETTRO", juce::Rectangle<float> (4, top.getY() + 2, 48, 12), juce::Justification::centredLeft);
    if (! beat) return;
    // quanto il beat si abbassa, banda per banda (0 dB in alto, -12 in basso)
    auto yC = [&] (float c) { return bot.getY() + bot.getHeight() * juce::jlimit (0.0f, 1.0f, c / 12.0f); };
    for (int db = 0; db <= 12; db += 3) { g.setColour (K::hi.withAlpha (db == 0 ? 0.25f : 0.06f)); g.fillRect (bot.getX(), yC ((float) db), bot.getWidth(), 1.0f); g.setFont (num (8.5f)); g.setColour (K::faint); g.drawText (db == 0 ? "0 dB" : "-" + juce::String (db), juce::Rectangle<float> (4, yC ((float) db) - 6, 44, 12), juce::Justification::centredLeft); }
    auto pc = curve (smC, yC, bot, false);
    juce::Path fill = pc; fill.lineTo (bot.getRight(), bot.getY()); fill.lineTo (bot.getX(), bot.getY()); fill.closeSubPath();
    g.setColour (K::amber.withAlpha (0.28f)); g.fillPath (fill);
    g.setColour (K::amber.withAlpha (0.3f)); g.strokePath (pc, juce::PathStrokeType (5.0f)); g.setColour (K::amber); g.strokePath (pc, juce::PathStrokeType (2.0f));
    g.setFont (cap (8.5f)); g.setColour (K::amber); g.drawText ("QUANTO SI ABBASSA IL BEAT", juce::Rectangle<float> (bot.getX() + 6, bot.getBottom() - 16, 300, 12), juce::Justification::centredLeft);
    const float vol = P.uiVol.load();
    if (vol > 0.05f) { g.setColour (K::amber.withAlpha (0.7f)); g.drawText ("+ VOLUME -" + juce::String (vol, 1) + " dB", juce::Rectangle<float> (bot.getRight() - 200, bot.getBottom() - 16, 196, 12), juce::Justification::centredRight); }
}

// ======================================================================= nel tempo
void LaneView::paint (juce::Graphics& g)
{
    auto& P = ed.proc; const bool beat = P.beatMode(); auto b = getLocalBounds().toFloat();
    const int N = Processor::kHist, w0 = P.hW.load();
    auto l1 = juce::Rectangle<float> (70, 30, b.getWidth() - 90, beat ? 30 : 60), l2 = juce::Rectangle<float> (70, l1.getBottom() + 8, b.getWidth() - 90, 30);
    g.setFont (cap (8.5f)); g.setColour (K::pink); g.drawText ("VOCE", juce::Rectangle<float> (18, l1.getY(), 50, l1.getHeight()), juce::Justification::centredLeft);
    g.setColour (K::hi.withAlpha (0.04f)); g.fillRoundedRectangle (l1, 4);
    if (beat) { g.setColour (K::amber); g.drawText ("BEAT", juce::Rectangle<float> (18, l2.getY(), 50, l2.getHeight()), juce::Justification::centredLeft); g.setColour (K::hi.withAlpha (0.04f)); g.fillRoundedRectangle (l2, 4); }
    for (int x = 0; x < N; ++x)
    {
        const float a = P.hAct[(w0 + x) % N].load(), c = P.hCut[(w0 + x) % N].load();
        const float xx = l1.getX() + l1.getWidth() * x / N, ww = l1.getWidth() / N + 0.6f;
        if (a > 0.05f) { g.setColour (K::pink.withAlpha (0.25f + 0.7f * a)); g.fillRect (xx, l1.getBottom() - l1.getHeight() * a, ww, l1.getHeight() * a); }
        if (beat && c > 0.05f) { g.setColour (K::amber.withAlpha (0.85f)); g.fillRect (xx, l2.getY(), ww, l2.getHeight() * juce::jlimit (0.0f, 1.0f, c / 12.0f)); }
    }
    g.setFont (num (8.5f)); g.setColour (K::faint);
    g.drawText ("-6 s", juce::Rectangle<float> (l1.getX(), b.getBottom() - 14, 60, 12), juce::Justification::centredLeft);
    g.drawText ("ORA", juce::Rectangle<float> (l1.getRight() - 60, b.getBottom() - 14, 60, 12), juce::Justification::centredRight);
    if (beat) g.drawText (U ("arancio verso il basso = il beat si abbassa (fino a -12 dB)"), juce::Rectangle<float> (l1.getX() + 60, b.getBottom() - 14, l1.getWidth() - 120, 12), juce::Justification::centred);
}

// ======================================================================= editor
Editor::Editor (Processor& p) : AudioProcessorEditor (p), proc (p)
{
    skin::init ("vocalradar", { 0xffe58aa0, 0xffb9a6e0, 0xffe9a260, 0xffa8bea0 }, 0xffff2e88);
    addAndMakeVisible (space); addAndMakeVisible (lanes);
    auto add = [&] (const char* id, const char* lab, juce::Colour a, juce::Colour b2, std::function<juce::String (float)> fm, const char* tip, bool big = false)
    {
        auto* prm = proc.apvts.getParameter (id);
        auto* k = knobs.add (new neo::Knob (prm, lab, a, b2)); k->big = big;
        k->text = [prm, fm] { return fm (prm->convertFrom0to1 (prm->getValue())); }; k->defV = prm->getDefaultValue();
        tips.add (juce::String::fromUTF8 (tip)); addAndMakeVisible (k);
    };
    auto pct = [] (float v) { return juce::String (juce::roundToInt (v * 100)) + "%"; };
    add ("body", "CORPO", K::amber, K::pink, pct, "Quanto aprire il corpo della voce (160-480 Hz), dove si scontra con 808, bassi e melodie piene. Alzalo se la voce suona impastata o lontana.");
    add ("pres", "PAROLE", K::amber, K::pink, pct, "Quanto aprire 1.8-5.5 kHz, dove si capiscono le parole. Il comando piu' importante: alzalo se non si capisce il testo.");
    add ("air", "ARIA", K::amber, K::pink, pct, "Quanto aprire 7-12 kHz, il respiro e le sibilanti. Utile se hi-hat e piatti coprono la voce.");
    add ("amount", "AMOUNT", K::amber, K::red, [] (float v) { return v < 0.05f ? juce::String ("OFF") : "-" + juce::String (v, 1) + " dB"; }, "Quanto puo' scendere al massimo il beat nelle zone scelte. 3-6 dB e' gia' tanto: deve sentirsi la voce piu' chiara, non il beat che respira.");
    add ("vol", "VOLUME", K::amber, K::red, [] (float v) { return v < 0.05f ? juce::String ("OFF") : "-" + juce::String (v, 1) + " dB"; }, "Oltre alle zone, abbassa tutto il beat di qualche dB mentre la voce canta. Per parlato e voci deboli; sul rap di solito 0-1 dB.");
    add ("stereo", "STEREO", K::cyan, K::violet, [] (float v) { return v > 0.97f ? juce::String ("SOLO CENTRO") : juce::String (juce::roundToInt (v * 100)) + "%"; }, "100% = il beat si abbassa solo al centro, dove sta la voce, e i lati restano larghi. 0% = si abbassa tutto lo stereo.");
    add ("rel", "RILASCIO", K::cyan, K::violet, [] (float v) { return juce::String (juce::roundToInt (v)) + " ms"; }, "Quanto ci mette il beat a tornare pieno a fine frase. Corto (100-150) per il rap, lungo (300-500) per il cantato.");
    add ("ant", "ANTICIPO", K::cyan, K::violet, [] (float v) { return juce::String (juce::roundToInt (v)) + " ms"; }, "Di quanto il beat comincia a scendere PRIMA che arrivi la voce: le consonanti a inizio frase restano pulite.");
    add ("sens", "SOGLIA VOCE", K::violet, K::pink, [] (float v) { return juce::String (juce::roundToInt (v)) + " dB"; }, "Sotto questa soglia la voce e' in pausa e il beat torna pieno. Se il beat si abbassa anche sui respiri o sul rientro delle cuffie, alzala.");
    add ("mix", "MIX", K::ink, K::sub, pct, "Quanto dell'effetto arriva in uscita. 100% = tutto.");
    add ("out", "OUTPUT", K::mint, K::cyan, [] (float v) { return (v > 0 ? "+" : "") + juce::String (v, 1) + " dB"; }, "Volume d'uscita del beat.");
    addAndMakeVisible (sizeChip);
    setupBrowser();
    undoMark.p = &p; addMouseListener (&undoMark, true); setWantsKeyboardFocus (true);
    scaler.setup();
    startTimerHz (30);
}
void Editor::resized()
{
    const float s = scaler.update();
    space.setBounds (24, 178, 712, 300); lanes.setBounds (24, 488, 712, 112);
    const int Sx = 50, w = Sx + 24, y = 642;
    const int xs[11] = { 40, 118, 196, 296, 414, 492, 596, 674, 782, 890, 968 };
    for (int i = 0; i < knobs.size(); ++i)
    {
        if (i == 3) { knobs[i]->setBounds (juce::Rectangle<int> (xs[i] + 4, 636, 92, 112).expanded (Knob::pad)); continue; }
        knobs[i]->setBounds (juce::Rectangle<int> (xs[i], y, w, Sx + 38).expanded (Knob::pad));
    }
    roleR[0] = { 182, 92, 196, 62 }; roleR[1] = { 386, 92, 196, 62 };
    for (int c = 0; c < NCH; ++c) chanR[c] = { 608.0f + c * 38, 104, 34, 38 };
    srcR[0] = { 962, 130, 50, 22 }; srcR[1] = { 1014, 130, 56, 22 };
    clickR = { 764, 274, 296, 40 }; applyR = { 764, 516, 144, 32 }; discardR = { 916, 516, 144, 32 };
    prevR = { 318, 18, 28, 34 }; presetR = { 348, 18, 260, 34 }; nextR = { 610, 18, 28, 34 };
    undoR = { 648, 21, 32, 28 }; redoR = { 682, 21, 32, 28 };
    deltaR = { 726, 18, 140, 34 }; bypR = { 872, 18, 112, 34 }; themeR = { 992, 21, 86, 28 };
    browser.setBounds (0, 0, W, H);
    sizeChip.setBounds (W - 76, H - 18, 58, 15);
    bg = {};
    jlook::scaleChildren (*this, s);
}
void Editor::paintStatic (juce::Graphics& g)
{
    const float Hf = (float) H;
    if (skin::cur() != 3)
    {   // temi della serie: sfondo e testata della pelle
        jlook::backdrop (g, { 0, 0, (float) W, Hf }, K::pink, K::amber);
        jlook::header (g, { 0, 0, (float) W, 66 }, "VOCAL", K::pink, "RADAR", K::amber, K::cyan);
    }
    else
    {
    juce::ColourGradient base (K::bg1, 0, 0, K::bg0, 0, Hf, false); g.setGradientFill (base); g.fillAll();
    for (int i = 0; i < 3; ++i) { juce::Path a; const float r = 5.0f + i * 5.5f; a.addCentredArc (36, 36, r, r, 0, -1.2f, 1.2f, true); g.setColour ((i == 0 ? K::pink : i == 1 ? K::amber : K::cyan).withAlpha (1.0f - i * 0.2f)); g.strokePath (a, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded)); }
    g.setColour (K::pink); g.fillEllipse (33, 33, 6, 6);
    g.setFont (num (15.0f)); g.setColour (K::ink); g.drawText ("JACKPOT", juce::Rectangle<float> (60, 18, 100, 20), juce::Justification::centredLeft);
    g.setFont (big (15.0f)); g.setColour (K::sub); g.drawText ("VOCAL RADAR", juce::Rectangle<float> (137, 18, 160, 20), juce::Justification::centredLeft);
    g.setFont (cap (7.5f)); g.setColour (K::faint); g.drawText (U ("LA VOCE PARLA, IL BEAT FA SPAZIO"), juce::Rectangle<float> (60, 38, 300, 12), juce::Justification::centredLeft);
    }
    panel (g, { 24, 72, 1052, 94 }, K::pink);
    panel (g, { 24, 178, 712, 300 }, K::amber, true);
    panel (g, { 24, 488, 712, 112 }, K::pink, true);
    panel (g, { 748, 178, 328, 422 }, K::amber);
    panel (g, { 24, 612, 1052, 146 }, K::violet);
    title (g, { 24, 72, 160, 94 }, "QUESTA COPIA STA", K::pink, U ("sulla traccia di..."));
    title (g, { 24, 178, 712, 300 }, "COSA STA FACENDO", K::amber);
    title (g, { 24, 488, 712, 112 }, "QUANDO", K::pink);
    auto grp = [&] (float x, float w, const char* t) { g.setFont (cap (8.5f)); g.setColour (K::sub); g.drawText (t, juce::Rectangle<float> (x, 620, w, 14), juce::Justification::centred); g.setColour (K::hi.withAlpha (0.08f)); g.fillRect (x + 6, 636.0f, w - 12, 1.0f); };
    grp (36, 236, "DOVE APRIRE LO SPAZIO"); grp (288, 278, "QUANTO"); grp (588, 172, "TEMPI"); grp (774, 92, "VOCE"); grp (880, 176, "USCITA");
}
juce::String Editor::liveSentence() const
{
    if (! proc.beatMode())
    {
        const int n = beatsListening (proc.channel());
        return proc.uiAct.load() > 0.4f ? U ("la voce sta cantando: la mando sul canale ") + chName (proc.channel()) + (n > 0 ? "." : U (", ma nessun beat la sta ascoltando."))
                                        : U ("voce in pausa (sotto la SOGLIA).");
    }
    if (proc.v ("byp") > 0.5f) return U ("CONFRONTA acceso: senti il beat originale.");
    if (proc.v ("amount") < 0.05f) return U ("AMOUNT e' a OFF: il plugin guarda ma non tocca il beat. Scegli un preset o usa ONE CLICK.");
    if (! proc.uiLinked.load() && ! proc.uiSide.load()) return U ("non arriva nessuna voce: guarda il collegamento in alto.");
    if (proc.uiAct.load() < 0.15f) return U ("la voce e' in pausa: il beat suona pieno.");
    float best = 0, body = 0, words = 0, air = 0;
    for (int b = 0; b < NB; ++b) { const float c = proc.uiCut[b].load(), hz = bandHz (b); if (hz < 700) body = juce::jmax (body, c); else if (hz < 6000) words = juce::jmax (words, c); else air = juce::jmax (air, c); best = juce::jmax (best, c); }
    if (best < 0.3f) return U ("la voce canta ma il beat non la copre: non serve abbassare niente.");
    juce::StringArray parts;
    if (words > 0.3f) parts.add ("-" + juce::String (words, 1) + " dB sulle parole");
    if (body > 0.3f) parts.add ("-" + juce::String (body, 1) + " dB sul corpo");
    if (air > 0.3f) parts.add ("-" + juce::String (air, 1) + " dB sull'aria");
    return U ("la voce canta, il beat scende ") + parts.joinIntoString (", ") + (proc.uiVol.load() > 0.05f ? U (", volume -") + juce::String (proc.uiVol.load(), 1) + " dB." : juce::String ("."));
}
void Editor::paint (juce::Graphics& g)
{
    g.addTransform (juce::AffineTransform::scale (scaler.s));
    const float sc = juce::jlimit (1.0f, 4.0f, g.getInternalContext().getPhysicalPixelScaleFactor());
    if (bg.isNull() || std::abs (bgScale - sc) > 0.01f || bgTheme != skin::cur())
    {
        bgTheme = skin::cur(); bgScale = sc; bg = juce::Image (juce::Image::RGB, juce::roundToInt (W * sc), juce::roundToInt (H * sc), false);
        juce::Graphics ig (bg); ig.addTransform (juce::AffineTransform::scale (sc)); paintStatic (ig);
    }
    { g.saveState(); g.addTransform (juce::AffineTransform::scale (1.0f / sc)); g.drawImageAt (bg, 0, 0); g.restoreState(); }
    const bool beat = proc.beatMode(); const int ch = proc.channel();
    // ---------------- testata
    { auto pr = presetR; g.setColour (K::lo.withAlpha (0.3f)); g.fillRoundedRectangle (pr, 8); g.setColour (K::hi.withAlpha (0.1f)); g.drawRoundedRectangle (pr, 8, 1.0f);
      g.setFont (cap (7.0f)); g.setColour (K::faint); g.drawText ("PRESET", pr.withHeight (12).translated (0, 2), juce::Justification::centred);
      g.setFont (f (13.0f, 1)); g.setColour (K::ink); g.drawFittedText (proc.presetName, pr.withTrimmedTop (11).toNearestInt(), juce::Justification::centred, 1);
      g.setFont (f (18.0f)); g.setColour (K::sub); g.drawText ("<", prevR, juce::Justification::centred); g.drawText (">", nextR, juce::Justification::centred); }
    chip (g, undoR, U ("\xe2\x86\xb6"), false, proc.undo.canUndo() ? K::mint : K::faint, 12.0f, hoverTop == 1);
    chip (g, redoR, U ("\xe2\x86\xb7"), false, proc.undo.canRedo() ? K::mint : K::faint, 12.0f, hoverTop == 2);
    skin::chipDraw (g, themeR, hoverTop == 3);
    if (beat) { chip (g, deltaR, "DELTA: COSA TOGLIE", proc.v ("delta") > 0.5f, K::red, 8.5f); chip (g, bypR, "CONFRONTA (OFF)", proc.v ("byp") > 0.5f, K::amber, 8.5f); }
    // ---------------- collegamento
    {
        const char* rn[2] = { "LA VOCE", "IL BEAT" }; const char* rd[2] = { "manda la voce agli altri", "si abbassa sotto la voce" };
        for (int r = 0; r < 2; ++r)
        {
            const bool on = (r == 1) == beat; const auto c = r ? K::amber : K::pink; auto rr = roleR[r];
            g.setColour (on ? c.withAlpha (0.18f) : K::lo.withAlpha (0.25f)); g.fillRoundedRectangle (rr, 10);
            g.setColour (on ? c : K::hi.withAlpha (0.12f)); g.drawRoundedRectangle (rr.reduced (0.5f), 10, on ? 1.6f : 1.0f);
            g.setFont (f (15.0f, 1)); g.setColour (on ? K::ink : K::sub); g.drawText (rn[r], rr.withTrimmedBottom (24).translated (0, 6), juce::Justification::centred);
            g.setFont (f (10.0f)); g.setColour (on ? c : K::faint); g.drawText (rd[r], rr.withTrimmedTop (34), juce::Justification::centred);
        }
        g.setFont (cap (8.0f)); g.setColour (K::sub); g.drawText ("CANALE", juce::Rectangle<float> (608, 86, 150, 14), juce::Justification::centredLeft);
        for (int c = 0; c < NCH; ++c) chip (g, chanR[c], chName (c), ch == c, K::violet, 11.0f);
        const bool sc = beat && proc.v ("src") > 0.5f;
        const bool vAlive = sc ? proc.uiSide.load() : voiceAlive (ch); const int nb = beatsListening (ch);
        const bool bAlive = beat || nb > 0;
        auto box = [&] (juce::Rectangle<float> r, const juce::String& t, juce::Colour c, bool alive, bool me)
        {
            g.setColour (alive ? c.withAlpha (0.16f) : K::lo.withAlpha (0.2f)); g.fillRoundedRectangle (r, 8);
            g.setColour (alive ? c : K::red.withAlpha (0.7f)); g.drawRoundedRectangle (r.reduced (0.5f), 8, me ? 2.0f : 1.0f);
            g.setFont (cap (8.5f)); g.setColour (alive ? K::ink : K::red); g.drawText (t, r, juce::Justification::centred);
            if (me) { g.setFont (cap (6.5f)); g.setColour (K::faint); g.drawText ("QUESTA COPIA", r.translated (0, -14).withHeight (12), juce::Justification::centred); }
        };
        auto vb = juce::Rectangle<float> (790, 96, 96, 28), bb = juce::Rectangle<float> (952, 96, 96, 28);
        box (vb, sc ? "SIDECHAIN" : "VOCE " + juce::String (chName (ch)), K::pink, vAlive, ! beat);
        box (bb, "BEAT " + juce::String (chName (ch)), K::amber, bAlive, beat);
        const bool flow = vAlive && bAlive;
        g.setColour (flow ? K::pink : K::red.withAlpha (0.5f)); g.drawLine (vb.getRight() + 4, vb.getCentreY(), bb.getX() - 8, bb.getCentreY(), 1.4f);
        { juce::Path ar; ar.addTriangle (bb.getX() - 8, bb.getCentreY() - 4, bb.getX() - 8, bb.getCentreY() + 4, bb.getX() - 2, bb.getCentreY()); g.fillPath (ar); }
        if (flow) for (int k = 0; k < 3; ++k) { const float t = std::fmod (frame * 0.025f + k / 3.0f, 1.0f); const float x = vb.getRight() + 4 + (bb.getX() - vb.getRight() - 12) * t; g.setColour (K::pink.withAlpha (std::sin (t * 3.1416f))); g.fillEllipse (x - 3, vb.getCentreY() - 3, 6, 6); }
        g.setFont (f (9.5f)); g.setColour (flow ? K::mint : K::red);
        juce::String st;
        if (beat) st = sc ? (proc.uiSide.load() ? U ("voce dal sidechain: ok") : U ("sidechain vuoto: scegli la voce nel menu sidechain"))
                          : (vAlive ? U ("collegato: la voce arriva") : U ("manca la voce: copia LA VOCE, canale ") + chName (ch));
        else st = nb > 0 ? juce::String (nb) + U (" beat in ascolto: tutto ok") : U ("nessun beat in ascolto: metti una copia IL BEAT sul beat, canale ") + chName (ch);
        g.drawFittedText (st, juce::Rectangle<int> (790, 130, beat ? 166 : 280, 28), juce::Justification::topLeft, 2);
        if (beat) { chip (g, srcR[0], "LINK", ! sc, K::pink, 7.5f); chip (g, srcR[1], "SIDECHAIN", sc, K::pink, 7.5f); }
    }
    // ---------------- adesso
    g.setFont (f (11.0f, 1)); g.setColour (K::ink);
    g.drawFittedText ("ADESSO: " + liveSentence(), juce::Rectangle<int> (42, 204, 676, 16), juce::Justification::centredLeft, 1);
    // ---------------- pannello destro
    const juce::Rectangle<float> rp (748, 178, 328, 422);
    if (beat)
    {
        title (g, rp, "ONE CLICK", K::amber, U ("il plugin ascolta voce e beat e propone le regolazioni"));
        g.setFont (f (10.5f)); g.setColour (K::sub);
        g.drawFittedText (U ("1  Fai suonare il pezzo con la voce dentro\n2  Premi ASCOLTA e lascia andare 10-20 secondi\n3  Premi STOP: leggi la proposta, poi APPLICA o SCARTA"), juce::Rectangle<int> (764, 222, 300, 44), juce::Justification::topLeft, 3);
        const bool lr = proc.learning.load();
        chip (g, clickR, lr ? "STOP E PROPONI  (" + juce::String (proc.learnSec.load(), 0) + " s)" : "ASCOLTA", lr, K::amber, 11.0f);
        const auto& pr = proc.proposal;
        juce::StringArray lines = pr.lines; juce::Colour lc = K::ink.withAlpha (0.88f);
        if (lr) { lines.clear(); lines.add (U ("Sto ascoltando... la voce deve essere collegata (vedi in alto).")); }
        else if (pr.state == 0) { lines.clear(); lines.add (U ("Nessuna proposta ancora.")); lc = K::faint; }
        else if (pr.state == 2) { lines.clear(); lines.add (U ("Proposta applicata ai comandi. Se non ti convince premi TORNA A PRIMA.")); lc = K::mint; }
        else if (pr.state == 3) lc = K::red;
        g.setFont (f (10.5f)); g.setColour (lc); float yy = 328;
        for (auto& l : lines) { const int h = l.length() > 52 ? (l.length() > 104 ? 42 : 28) : 14; if (yy + h > 508) break; g.drawFittedText (l, juce::Rectangle<int> (764, (int) yy, 300, h), juce::Justification::topLeft, 3); yy += h + 6; }
        chip (g, applyR, "APPLICA", pr.state == 1, K::mint, 9.5f);
        chip (g, discardR, pr.state == 2 ? "TORNA A PRIMA" : "SCARTA", pr.state == 2, K::red, 9.5f);
        g.setFont (f (8.5f)); g.setColour (K::faint);
        g.drawText (pr.state == 1 ? U ("APPLICA mette questi valori sui comandi") : pr.state == 2 ? U ("TORNA A PRIMA rimette i valori di prima") : U ("APPLICA e SCARTA si accendono dopo STOP"), juce::Rectangle<float> (764, 554, 300, 12), juce::Justification::centredLeft);
        g.drawFittedText (U ("DELTA (in alto) = senti solo quello che toglie al beat.  CONFRONTA = beat originale, per sentire la differenza."), juce::Rectangle<int> (764, 570, 300, 26), juce::Justification::topLeft, 2);
    }
    else
    {
        title (g, rp, "PROSSIMO PASSO", K::pink);
        g.setFont (f (11.0f)); g.setColour (K::ink.withAlpha (0.9f));
        g.drawFittedText (U ("Questa copia sta sulla VOCE: non cambia il suono, dice solo alle altre copie quando e come canti.\n\n"
                             "Ora vai sul beat (meglio: raggruppa le tracce del beat con Cmd+G e usa il gruppo), metti un'altra copia di Vocal Radar, scegli IL BEAT e lo stesso canale ") + chName (ch) + U (".\n\n"
                             "Tutte le copie dello stesso tipo: tutte AU o tutte VST3.\n\n"
                             "Ad-lib o seconda voce: un'altra copia LA VOCE sul canale B."), juce::Rectangle<int> (764, 210, 296, 360), juce::Justification::topLeft, 22);
    }
}
void Editor::paintOverChildren (juce::Graphics& g)
{
    g.addTransform (juce::AffineTransform::scale (scaler.s));
    if (! proc.beatMode())
    {
        g.setColour (K::bg0.withAlpha (0.78f));
        g.fillRect (juce::Rectangle<float> (30, 640, 738, 112)); g.fillRect (juce::Rectangle<float> (874, 640, 196, 112));
        g.setFont (cap (9.0f)); g.setColour (K::pink); g.drawText (U ("SULLA VOCE CONTA SOLO LA SOGLIA: GLI ALTRI COMANDI SONO SULLA COPIA DEL BEAT"), juce::Rectangle<float> (30, 686, 738, 16), juce::Justification::centred);
    }
    if (hoverKnob >= 0)
    {
        auto tr = juce::Rectangle<float> (44, 566, 676, 28);
        g.setColour (K::lo.withAlpha (0.92f)); g.fillRoundedRectangle (tr.expanded (8, 4), 8); g.setColour (K::amber.withAlpha (0.5f)); g.drawRoundedRectangle (tr.expanded (8, 4), 8, 1.0f);
        g.setFont (f (10.0f)); g.setColour (K::ink); g.drawFittedText (knobs[hoverKnob]->label + ": " + tips[hoverKnob], tr.toNearestInt(), juce::Justification::centredLeft, 2);
    }
}
void Editor::mouseDown (const juce::MouseEvent& e)
{
    const auto p = e.position / scaler.s;
    auto set = [this] (const char* id, float v) { proc.setV (id, v); repaint(); };
    if (roleR[0].contains (p)) { set ("mode", 0); return; }
    if (roleR[1].contains (p)) { set ("mode", 1); return; }
    for (int c = 0; c < NCH; ++c) if (chanR[c].contains (p)) { set ("chan", (float) c); return; }
    if (prevR.contains (p) || nextR.contains (p))
    {
        const int n = (int) presets().size(); const int cur = juce::jmax (0, proc.curPreset);
        proc.loadPreset ((cur + (nextR.contains (p) ? 1 : n - 1)) % n); repaint(); return;
    }
    if (presetR.contains (p)) { browser.open(); return; }
    if (undoR.contains (p)) { doUndo (false); return; }
    if (redoR.contains (p)) { doUndo (true); return; }
    if (themeR.contains (p)) { skin::cycle(); bg = {}; std::function<void (juce::Component*)> all = [&all] (juce::Component* c) { c->repaint(); for (auto* q : c->getChildren()) all (q); }; all (this); return; }
    if (! proc.beatMode()) return;
    if (deltaR.contains (p)) { set ("delta", proc.v ("delta") > 0.5f ? 0.0f : 1.0f); return; }
    if (bypR.contains (p)) { set ("byp", proc.v ("byp") > 0.5f ? 0.0f : 1.0f); return; }
    if (srcR[0].contains (p)) { set ("src", 0); return; }
    if (srcR[1].contains (p)) { set ("src", 1); return; }
    if (clickR.contains (p)) { if (proc.learning.load()) proc.learnStop(); else proc.learnStart(); repaint(); return; }
    if (applyR.contains (p) && proc.proposal.state == 1) { proc.learnApply(); repaint(); return; }
    if (discardR.contains (p) && proc.proposal.state != 0) { proc.learnDiscard(); repaint(); return; }
}
void Editor::mouseMove (const juce::MouseEvent& e)
{
    const auto p = e.position / scaler.s;
    const int h = undoR.contains (p) ? 1 : redoR.contains (p) ? 2 : themeR.contains (p) ? 3 : -1;
    if (h != hoverTop) { hoverTop = h; repaint (scaler.map (juce::Rectangle<int> (600, 10, 490, 50))); }
    setMouseCursor (h >= 0 || presetR.contains (p) || prevR.contains (p) || nextR.contains (p) || deltaR.contains (p) || bypR.contains (p) || clickR.contains (p) ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
}
bool Editor::keyPressed (const juce::KeyPress& k)
{
    if (k.getModifiers().isCommandDown() && (k.getKeyCode() == 'Z' || k.getKeyCode() == 'z')) { doUndo (k.getModifiers().isShiftDown()); return true; }
    if (k.getModifiers().isCommandDown() && (k.getKeyCode() == 'Y' || k.getKeyCode() == 'y')) { doUndo (true); return true; }
    return false;
}
void Editor::doUndo (bool redo) { proc.undo.beginNewTransaction(); if (redo) proc.undo.redo(); else proc.undo.undo(); repaint(); }
void Editor::setupBrowser()
{
    addChildComponent (browser);
    browser.accent = K::pink; browser.title = "Preset";
    auto userFiles = [] { auto f = Processor::presetFolder().findChildFiles (juce::File::findFiles, false, "*.jvr"); f.sort(); return f; };
    browser.entries = [userFiles]
    {
        std::vector<jbrowse::Entry> v;
        for (int i = 0; i < (int) presets().size(); ++i)
        {
            const juce::String nm (presets()[(size_t) i].name); jbrowse::Entry e; e.id = i; e.note = juce::String::fromUTF8 (presets()[(size_t) i].note);
            e.group = nm.contains (" | ") ? nm.upToFirstOccurrenceOf (" | ", false, false) : juce::String ("BASE"); e.name = nm.contains (" | ") ? nm.fromFirstOccurrenceOf (" | ", false, false) : nm;
            v.push_back (e);
        }
        const auto fs = userFiles();
        for (int i = 0; i < fs.size(); ++i) { jbrowse::Entry e; e.id = 100000 + i; e.user = true; e.group = "I MIEI"; e.name = fs[i].getFileNameWithoutExtension(); e.note = "Preset tuo, salvato in Musica / Jackpot / Vocal Radar Preset."; v.push_back (e); }
        return v;
    };
    browser.current = [this, userFiles]
    {
        if (proc.curPreset < 0) { const auto fs = userFiles(); for (int i = 0; i < fs.size(); ++i) if (fs[i].getFileNameWithoutExtension() == proc.presetName) return 100000 + i; return -1; }
        return proc.curPreset;
    };
    browser.load = [this] (const jbrowse::Entry& e) { if (e.user) proc.loadPresetFile (Processor::presetFolder().getChildFile (e.name + ".jvr")); else proc.loadPreset (e.id); repaint(); };
    browser.save = [this] { askPresetName(); };
    browser.canOverwrite = [this] { return proc.curPreset < 0 && Processor::presetFolder().getChildFile (proc.presetName + ".jvr").existsAsFile(); };
    browser.overwrite = [this] { proc.savePresetFile (Processor::presetFolder().getChildFile (proc.presetName + ".jvr")); repaint(); };
    browser.folder = [] { Processor::presetFolder().startAsProcess(); };
    browser.remove = [] (const jbrowse::Entry& e) { Processor::presetFolder().getChildFile (e.name + ".jvr").moveToTrash(); };
    browser.onClose = [this] { repaint(); };
}
void Editor::askPresetName()
{
    auto* w = new juce::AlertWindow ("Salva preset", "Nome del preset (finisce in Musica / Jackpot / Vocal Radar Preset):", juce::MessageBoxIconType::NoIcon, this);
    w->addTextEditor ("name", proc.curPreset < 0 ? proc.presetName : juce::String());
    w->addButton ("SALVA", 1, juce::KeyPress (juce::KeyPress::returnKey)); w->addButton ("ANNULLA", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    juce::Component::SafePointer<Editor> sp (this);
    w->enterModalState (true, juce::ModalCallbackFunction::create ([sp, w] (int r)
    {
        if (sp != nullptr && r == 1)
        {
            const auto nm = juce::File::createLegalFileName (w->getTextEditorContents ("name").trim());
            if (nm.isNotEmpty()) { sp->proc.savePresetFile (Processor::presetFolder().getChildFile (nm + ".jvr")); sp->browser.refresh(); sp->repaint(); }
        }
    }), true);
}
void Editor::timerCallback()
{
    if (browser.isVisible()) return;
    if (bgTheme != skin::cur() && bgTheme >= 0) { skin::apply(); bg = {}; repaint(); }
    ++frame;
    for (int b = 0; b < NB; ++b)
    {
        space.smV[b] += 0.35f * (proc.uiVoice[b].load() - space.smV[b]); space.smB[b] += 0.35f * (proc.uiBeat[b].load() - space.smB[b]); space.smC[b] += 0.4f * (proc.uiCut[b].load() - space.smC[b]);
    }
    const bool beat = proc.beatMode();
    int hov = -1;
    for (int i = 0; i < knobs.size(); ++i)
    {
        auto* k = knobs[i]; const bool en = beat || i == 8; if (k->isEnabled() != en) k->setEnabled (en);
        if (k->animate()) k->repaint();
        if (k->isMouseOverOrDragging()) hov = i;
    }
    if (hov != hoverKnob) { hoverKnob = hov; repaint (scaler.map (juce::Rectangle<int> (24, 556, 712, 48))); }
    space.repaint(); lanes.repaint();
    repaint (scaler.map (juce::Rectangle<int> (24, 72, 1052, 94))); repaint (scaler.map (juce::Rectangle<int> (640, 16, 80, 36)));
    repaint (scaler.map (juce::Rectangle<int> (30, 196, 700, 28)));
    if (proc.learning.load() || frame % 15 == 0) repaint (scaler.map (juce::Rectangle<int> (748, 178, 328, 422)));
}
}
