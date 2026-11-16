// JACKPOT VOCAL RADAR: grafica cyberpunk "a manetta". Notte al neon, griglia in prospettiva, palazzi con le finestre accese,
// pannelli a spigoli tagliati con il bordo che brilla, logo glitch, radar circolare con il raggio che gira.
#include "VrEditor.h"

namespace vr
{
using namespace cp;
static juce::String U (const char* s) { return juce::String::fromUTF8 (s); }

// pannello a spigoli tagliati con bordo al neon
static juce::Path cut (juce::Rectangle<float> r, float c)
{
    juce::Path p; p.startNewSubPath (r.getX() + c, r.getY()); p.lineTo (r.getRight(), r.getY()); p.lineTo (r.getRight(), r.getBottom() - c);
    p.lineTo (r.getRight() - c, r.getBottom()); p.lineTo (r.getX(), r.getBottom()); p.lineTo (r.getX(), r.getY() + c); p.closeSubPath(); return p;
}
static void neonStroke (juce::Graphics& g, const juce::Path& p, juce::Colour c, float w = 1.5f)
{
    for (int k = 3; k >= 1; --k) { g.setColour (c.withAlpha (0.08f * (4 - k))); g.strokePath (p, juce::PathStrokeType (w + k * 3.0f)); }
    g.setColour (c); g.strokePath (p, juce::PathStrokeType (w));
}
static void cyberPanel (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c, const juce::String& tag)
{
    const auto p = cut (r, 16);
    g.setColour (panel); g.fillPath (p);
    juce::ColourGradient tint (c.withAlpha (0.10f), r.getX(), r.getY(), c.withAlpha (0.0f), r.getX(), r.getY() + 120, false); g.setGradientFill (tint); g.fillPath (p);
    neonStroke (g, p, c.withAlpha (0.85f), 1.2f);
    // tacche e targhetta
    g.setColour (c); g.fillRect (r.getX() + 22, r.getY() - 2, 46.0f, 4.0f); g.fillRect (r.getRight() - 30, r.getBottom() - 2, 22.0f, 4.0f);
    for (int i = 0; i < 6; ++i) { g.setColour (c.withAlpha (0.35f)); g.fillRect (r.getRight() - 14 - i * 7.0f, r.getY() + 8, 4.0f, 4.0f); }
    g.setFont (fMono (11)); const float tw = juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), tag) + 12;
    g.setColour (bg0); g.fillRect (r.getX() + 72, r.getY() - 8, tw, 14.0f);
    g.setColour (c.withAlpha (0.9f)); g.drawText (tag, juce::Rectangle<float> (r.getX() + 78, r.getY() - 8, tw, 14), juce::Justification::centredLeft);
}
static void chipBtn (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& t, bool on, juce::Colour c, float fs = 12)
{
    const auto p = cut (r, 7);
    g.setColour (on ? c.withAlpha (0.28f) : juce::Colour (0xcc0b0918)); g.fillPath (p);
    if (on) neonStroke (g, p, c, 1.2f); else { g.setColour (c.withAlpha (0.45f)); g.strokePath (p, juce::PathStrokeType (1.0f)); }
    g.setFont (fLab (fs)); g.setColour (on ? ink : c.withAlpha (0.8f)); g.drawText (t, r, juce::Justification::centred);
}

// ======================================================================= manopola
CyberKnob::CyberKnob (juce::RangedAudioParameter* p, juce::String l, juce::Colour c, bool b) : label (std::move (l)), col (c), big (b)
{
    setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag); setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    setMouseDragSensitivity (220); setVelocityBasedMode (false);
    att = std::make_unique<juce::SliderParameterAttachment> (*p, *this);
    setDoubleClickReturnValue (true, p->convertFrom0to1 (p->getDefaultValue()));
}
void CyberKnob::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    const float labH = big ? 40.0f : 34.0f, d = juce::jmin (b.getWidth() - 8, b.getHeight() - labH);
    const float cx = b.getCentreX(), cy = 4 + d * 0.5f, R = d * 0.5f;
    const float t = (float) valueToProportionOfLength (getValue());
    const float a0 = juce::MathConstants<float>::pi * 1.25f, a1 = juce::MathConstants<float>::pi * 2.75f;
    const auto c = dimmed ? dim.withAlpha (0.4f) : col;
    // anello a segmenti
    const int seg = big ? 40 : 28;
    for (int i = 0; i < seg; ++i)
    {
        const float u = (i + 0.5f) / seg, a = a0 + (a1 - a0) * u; const bool on = u <= t + 1e-4f;
        juce::Path s; s.addCentredArc (cx, cy, R - 2, R - 2, 0, a - (a1 - a0) / seg * 0.36f, a + (a1 - a0) / seg * 0.36f, true);
        const auto sc = on ? c.interpolatedWith (mag, u * 0.6f) : juce::Colour (0xff221d38);
        if (on) { g.setColour (sc.withAlpha (0.25f)); g.strokePath (s, juce::PathStrokeType (big ? 9.0f : 7.0f)); }
        g.setColour (sc); g.strokePath (s, juce::PathStrokeType (big ? 4.0f : 3.0f));
    }
    // corpo metallico con zigrinatura
    const float r2 = R - (big ? 13.0f : 10.0f);
    g.setColour (juce::Colours::black.withAlpha (0.6f)); g.fillEllipse (cx - r2 - 1, cy - r2 + 3, r2 * 2 + 2, r2 * 2 + 2);
    juce::ColourGradient body (juce::Colour (0xff3a3650), cx - r2 * 0.6f, cy - r2 * 0.8f, juce::Colour (0xff09070f), cx + r2 * 0.6f, cy + r2, true);
    g.setGradientFill (body); g.fillEllipse (cx - r2, cy - r2, r2 * 2, r2 * 2);
    g.setColour (juce::Colours::white.withAlpha (0.05f));
    for (int i = 0; i < 36; ++i) { const float a = i * juce::MathConstants<float>::twoPi / 36; g.drawLine (cx + std::cos (a) * r2 * 0.82f, cy + std::sin (a) * r2 * 0.82f, cx + std::cos (a) * r2, cy + std::sin (a) * r2, 1.0f); }
    g.setColour (juce::Colours::white.withAlpha (0.10f)); g.drawEllipse (cx - r2, cy - r2, r2 * 2, r2 * 2, 1.0f);
    const float ri = r2 * 0.62f; juce::ColourGradient cap (juce::Colour (0xff1c1a28), cx, cy - ri, juce::Colour (0xff0a0912), cx, cy + ri, false); g.setGradientFill (cap); g.fillEllipse (cx - ri, cy - ri, ri * 2, ri * 2);
    // indice al neon
    const float a = a0 + (a1 - a0) * t, sx = std::sin (a), sy = -std::cos (a);
    juce::Line<float> ln (cx + sx * r2 * 0.25f, cy + sy * r2 * 0.25f, cx + sx * r2 * 0.92f, cy + sy * r2 * 0.92f);
    g.setColour (c.withAlpha (0.3f)); g.drawLine (ln, 6.0f); g.setColour (c.brighter (0.4f)); g.drawLine (ln, 2.2f);
    // scritte
    g.setFont (fLab (big ? 13.0f : 10.5f)); g.setColour (dimmed ? dim.withAlpha (0.5f) : ink.withAlpha (0.85f));
    g.drawText (label, juce::Rectangle<float> (0, cy + R + 2, b.getWidth(), 14), juce::Justification::centred);
    g.setFont (fMono (big ? 17.0f : 13.0f)); g.setColour (c);
    g.drawText (fmt ? fmt ((float) getValue()) : juce::String (getValue(), 1), juce::Rectangle<float> (0, cy + R + (big ? 18.0f : 16.0f), b.getWidth(), 18), juce::Justification::centred);
}

// ======================================================================= radar
void Radar::paint (juce::Graphics& g)
{
    auto& P = ed.proc; const bool beat = P.beatMode();
    auto b = getLocalBounds().toFloat();
    const float cx = b.getCentreX(), cy = b.getCentreY() + 6, R = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f - 22;
    // anelli e raggi
    for (int k = 1; k <= 4; ++k) { const float r = R * k / 4; g.setColour (cya.withAlpha (k == 4 ? 0.35f : 0.12f)); g.drawEllipse (cx - r, cy - r, r * 2, r * 2, k == 4 ? 1.4f : 1.0f); }
    const float A0 = juce::MathConstants<float>::pi * 0.75f, span = juce::MathConstants<float>::pi * 1.5f;   // 270 gradi: dai bassi (in basso a sinistra) agli alti
    auto ang = [&] (float u) { return A0 + span * u; };
    auto pt = [&] (float u, float r) { const float a = ang (u); return juce::Point<float> (cx + std::cos (a) * r, cy + std::sin (a) * r); };
    g.setFont (fMono (10));
    const float marks[6] = { 100, 300, 1000, 3000, 6000, 10000 };
    for (float hz : marks)
    {
        const float u = std::log (hz / 80.0f) / std::log (150.0f);
        g.setColour (cya.withAlpha (0.10f)); g.drawLine (juce::Line<float> (pt (u, R * 0.18f), pt (u, R)), 1.0f);
        const auto q = pt (u, R + 13); g.setColour (dim); g.drawText (hz >= 1000 ? juce::String (hz / 1000, 0) + "k" : juce::String ((int) hz), juce::Rectangle<float> (q.x - 18, q.y - 7, 36, 14), juce::Justification::centred);
    }
    // raggio che gira con la scia
    {
        const float a = sweep;
        for (int k = 0; k < 24; ++k)
        {
            juce::Path w; const float a1 = a - k * 0.035f; w.addPieSegment (cx - R, cy - R, R * 2, R * 2, a1 + juce::MathConstants<float>::halfPi, a1 + juce::MathConstants<float>::halfPi + 0.035f, 0.0f);
            g.setColour (cya.withAlpha (0.10f * (1.0f - k / 24.0f))); g.fillPath (w);
        }
        g.setColour (cya.withAlpha (0.7f)); g.drawLine (cx, cy, cx + std::cos (a) * R, cy + std::sin (a) * R, 1.2f);
    }
    auto rad = [&] (float db) { return R * (0.18f + 0.82f * juce::jlimit (0.0f, 1.0f, (db + 78.0f) / 66.0f)); };
    auto shape = [&] (const float* lv)
    {
        juce::Path p; p.startNewSubPath (pt (0, R * 0.18f));
        for (int i = 0; i <= 60; ++i)
        {
            const float u = i / 60.0f, fb = u * (NB - 1); const int b0 = (int) fb; const int b1 = juce::jmin (NB - 1, b0 + 1); const float fr = fb - b0;
            p.lineTo (pt (u, rad (lv[b0] * (1 - fr) + lv[b1] * fr)));
        }
        p.lineTo (pt (1, R * 0.18f)); p.addCentredArc (cx, cy, R * 0.18f, R * 0.18f, 0, ang (1) + juce::MathConstants<float>::halfPi, ang (0) + juce::MathConstants<float>::halfPi); p.closeSubPath(); return p;
    };
    // beat (ciano, contorno) e voce (magenta, pieno)
    if (beat)
    {
        const auto pb = shape (smB);
        g.setColour (cya.withAlpha (0.10f)); g.fillPath (pb); neonStroke (g, pb, cya.withAlpha (0.9f), 1.2f);
    }
    const auto pv = shape (smV);
    juce::ColourGradient vg (mag.withAlpha (0.55f), cx, cy, mag.withAlpha (0.15f), cx + R, cy, true); g.setGradientFill (vg); g.fillPath (pv);
    neonStroke (g, pv, mag, 1.4f);
    // tagli: spicchi gialli che entrano dal bordo
    if (beat)
        for (int b0 = 0; b0 < NB; ++b0)
        {
            const float c = smC[b0]; if (c < 0.15f) continue;
            const float u = (float) b0 / (NB - 1), w = 0.55f / NB, depth = R * 0.5f * juce::jlimit (0.0f, 1.0f, c / 12.0f);
            juce::Path s; s.addPieSegment (cx - R, cy - R, R * 2, R * 2, ang (u) - w * span * 0.5f + juce::MathConstants<float>::halfPi, ang (u) + w * span * 0.5f + juce::MathConstants<float>::halfPi, (R - depth) / R);
            g.setColour (yel.withAlpha (0.35f)); g.fillPath (s); g.setColour (yel); g.strokePath (s, juce::PathStrokeType (1.0f));
        }
    // centro: stato
    const float act = P.uiAct.load();
    g.setColour (juce::Colour (0xff07060e)); g.fillEllipse (cx - R * 0.17f, cy - R * 0.17f, R * 0.34f, R * 0.34f);
    g.setColour ((act > 0.4f ? mag : dim).withAlpha (0.4f + 0.6f * act)); g.drawEllipse (cx - R * 0.17f, cy - R * 0.17f, R * 0.34f, R * 0.34f, 2.0f);
    g.setFont (fLab (11)); g.setColour (act > 0.4f ? mag : dim); g.drawText (act > 0.4f ? "VOCE" : "PAUSA", juce::Rectangle<float> (cx - 40, cy - 16, 80, 14), juce::Justification::centred);
    float mx = 0; for (auto c : smC) mx = juce::jmax (mx, c);
    g.setFont (fMono (15)); g.setColour (beat ? yel : ink);
    g.drawText (beat ? "-" + juce::String (mx, 1) + " dB" : juce::String (juce::roundToInt (P.uiVoiceLvl.load())) + " dB", juce::Rectangle<float> (cx - 50, cy, 100, 18), juce::Justification::centred);
    // leggenda
    g.setFont (fMono (11));
    g.setColour (mag); g.drawText ("VOCE", juce::Rectangle<float> (b.getX() + 18, b.getBottom() - 46, 80, 14), juce::Justification::centredLeft);
    if (beat) { g.setColour (cya); g.drawText ("BEAT", juce::Rectangle<float> (b.getX() + 18, b.getBottom() - 32, 80, 14), juce::Justification::centredLeft);
                g.setColour (yel); g.drawText ("SPAZIO APERTO", juce::Rectangle<float> (b.getX() + 18, b.getBottom() - 18, 140, 14), juce::Justification::centredLeft); }
}

// ======================================================================= linea del tempo
void Timeline::paint (juce::Graphics& g)
{
    auto& P = ed.proc; auto b = getLocalBounds().toFloat().reduced (14, 10);
    const int N = Processor::kHist, w0 = P.hW.load();
    g.setColour (cya.withAlpha (0.08f)); for (int i = 1; i < 12; ++i) g.fillRect (b.getX() + b.getWidth() * i / 12, b.getY(), 1.0f, b.getHeight());
    const float mid = b.getY() + b.getHeight() * 0.45f;
    // frasi della voce (barre magenta in alto) e spazio aperto (giallo in basso)
    for (int x = 0; x < N; ++x)
    {
        const float a = P.hAct[(w0 + x) % N].load(), c = P.hCut[(w0 + x) % N].load();
        const float xx = b.getX() + b.getWidth() * x / N, ww = b.getWidth() / N + 0.6f;
        if (a > 0.05f) { g.setColour (mag.withAlpha (0.25f + 0.65f * a)); g.fillRect (xx, mid - (b.getHeight() * 0.42f) * a, ww, (b.getHeight() * 0.42f) * a); }
        if (c > 0.05f) { const float h = (b.getBottom() - mid - 4) * juce::jlimit (0.0f, 1.0f, c / 12.0f); g.setColour (yel.withAlpha (0.75f)); g.fillRect (xx, mid + 4, ww, h); }
    }
    g.setColour (cya.withAlpha (0.4f)); g.fillRect (b.getX(), mid + 1, b.getWidth(), 1.0f);
    // testina che scorre
    g.setColour (cya); g.fillRect (b.getRight() - 2, b.getY(), 2.0f, b.getHeight());
    g.setFont (fMono (10));
    auto tagAt = [&] (juce::Rectangle<float> r, const juce::String& t, juce::Colour c) { g.setColour (juce::Colour (0xd0050409)); g.fillRect (r.withWidth (juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), t) + 8)); g.setColour (c); g.drawText (t, r.translated (4, 0), juce::Justification::centredLeft); };
    tagAt ({ b.getX(), b.getY(), 200, 13 }, "FRASI DELLA VOCE", mag);
    if (P.beatMode()) tagAt ({ b.getX(), b.getBottom() - 13, 200, 13 }, "SPAZIO APERTO NEL BEAT", yel);
    g.setColour (dim); g.drawText ("-6 s", juce::Rectangle<float> (b.getX(), b.getY(), b.getWidth() - 6, 12), juce::Justification::centredRight);
}

// ======================================================================= editor
Editor::Editor (Processor& p) : AudioProcessorEditor (p), proc (p)
{
    addAndMakeVisible (radar); addAndMakeVisible (timeline);
    auto add = [&] (const char* id, const char* lab, juce::Colour c, bool big, std::function<juce::String (float)> f)
    { auto* k = knobs.add (new CyberKnob (proc.apvts.getParameter (id), lab, c, big)); k->fmt = std::move (f); k->setComponentID (id); addAndMakeVisible (k); return k; };
    auto pct = [] (float v) { return juce::String (juce::roundToInt (v * 100)) + "%"; };
    amountK = add ("amount", "AMOUNT", yel, true, [] (float v) { return v < 0.05f ? juce::String ("OFF") : "-" + juce::String (v, 1) + " dB"; });
    add ("pres", "PRESENZA", mag, false, pct); add ("body", "CORPO", mag, false, pct); add ("air", "ARIA", mag, false, pct);
    add ("vol", "VOLUME", yel, false, [] (float v) { return v < 0.05f ? juce::String ("OFF") : "-" + juce::String (v, 1) + " dB"; });
    add ("stereo", "STEREO", cya, false, [] (float v) { return v > 0.95f ? juce::String ("SOLO CENTRO") : juce::String (juce::roundToInt (v * 100)) + "% CTR"; });
    add ("rel", "RILASCIO", cya, false, [] (float v) { return juce::String (juce::roundToInt (v)) + " ms"; });
    add ("ant", "ANTICIPO", cya, false, [] (float v) { return juce::String (juce::roundToInt (v)) + " ms"; });
    sensK = add ("sens", "SOGLIA VOCE", pur, false, [] (float v) { return juce::String (juce::roundToInt (v)) + " dB"; });
    add ("mix", "MIX", ink, false, pct);
    add ("out", "OUTPUT", ink, false, [] (float v) { return (v > 0 ? "+" : "") + juce::String (v, 1) + " dB"; });
    knobs[1]->setTooltip ("Quanto aprire la zona 2-5 kHz, dove si capiscono le parole.");
    knobs[2]->setTooltip ("Quanto aprire il corpo della voce, 150-500 Hz, dove si scontra con 808 e melodie.");
    knobs[3]->setTooltip ("Quanto aprire l'aria, 7-12 kHz.");
    knobs[4]->setTooltip ("Abbassa tutto il beat di qualche dB mentre la voce canta (oltre alle bande).");
    knobs[5]->setTooltip ("100% = i tagli solo al centro, i lati del beat restano larghi. 0% = tagli su tutto lo stereo.");
    knobs[6]->setTooltip ("Quanto ci mette il beat a richiudersi a fine frase. Corto per il rap, lungo per le voci melodiche.");
    knobs[7]->setTooltip ("Di quanto il beat si apre prima che arrivi la voce (consonanti).");
    sensK->setTooltip ("Sotto questa soglia la voce e' considerata in pausa.");
    addAndMakeVisible (sizeChip);
    scaler.setup();
    startTimerHz (30);
}
void Editor::resized()
{
    const float s = scaler.update();
    radar.setBounds (24, 92, 520, 412);
    timeline.setBounds (572, 112, 584, 168);
    // fila delle manopole
    amountK->setBounds (40, 548, 130, 150);
    const int x0 = 196, w = 86;
    for (int i = 1; i < knobs.size(); ++i) { const int gap = i >= 4 ? (i >= 6 ? (i >= 8 ? (i >= 9 ? 48 : 36) : 24) : 12) : 0; knobs[i]->setBounds (x0 + (i - 1) * w + gap, 560, w, 118); }
    sizeChip.setBounds (W - 76, H - 16, 58, 14);
    // pulsanti dipinti
    modeR[0] = { 690, 22, 84, 32 }; modeR[1] = { 778, 22, 84, 32 };
    for (int c = 0; c < NCH; ++c) chanR[c] = { 890.0f + c * 38, 22, 34, 32 };
    srcR[0] = { 646, 470, 86, 26 }; srcR[1] = { 736, 470, 112, 26 };
    deltaR = { 1044, 470, 112, 26 };
    clickR = { 588, 316, 230, 54 }; applyR = { 836, 316, 150, 26 }; discardR = { 836, 344, 150, 26 };
    prevR = { 316, 22, 28, 32 }; presetR = { 346, 22, 296, 32 }; nextR = { 644, 22, 28, 32 };
    bg = {};
    jlook::scaleChildren (*this, s);
}

void Editor::paintStatic (juce::Graphics& g)
{
    const float Wf = (float) W, Hf = (float) H;
    juce::ColourGradient sky (bg1, 0, 0, bg0, 0, Hf, false); sky.addColour (0.55, juce::Colour (0xff0b0418)); g.setGradientFill (sky); g.fillAll();
    juce::Random rnd (7);
    // palazzi in controluce con le finestre accese
    for (int k = 0; k < 46; ++k)
    {
        const float w = 18 + rnd.nextFloat() * 50, h = 80 + rnd.nextFloat() * 260, x = rnd.nextFloat() * Wf, y = 540 - h;
        g.setColour (juce::Colour (0xff0d0820).interpolatedWith (juce::Colour (0xff1a0a33), rnd.nextFloat())); g.fillRect (x, y, w, h);
        for (float wy = y + 6; wy < 530; wy += 9) for (float wx = x + 3; wx < x + w - 4; wx += 7)
            if (rnd.nextFloat() < 0.10f) { g.setColour ((rnd.nextBool() ? mag : rnd.nextBool() ? cya : juce::Colour (0xffffc46b)).withAlpha (0.25f + rnd.nextFloat() * 0.4f)); g.fillRect (wx, wy, 3.0f, 4.0f); }
    }
    // foschia rosa sull'orizzonte
    juce::ColourGradient haze (mag.withAlpha (0.0f), 0, 380, mag.withAlpha (0.18f), 0, 540, false); g.setGradientFill (haze); g.fillRect (0.0f, 380.0f, Wf, 160.0f);
    // griglia in prospettiva sul pavimento
    g.setColour (juce::Colour (0xff07040d)); g.fillRect (0.0f, 540.0f, Wf, Hf - 540);
    const float vx = Wf * 0.5f, vy = 540;
    for (int i = -20; i <= 20; ++i) { g.setColour (mag.withAlpha (0.22f)); g.drawLine (vx + i * 18.0f, vy, vx + i * 140.0f, Hf, 1.0f); }
    for (int j = 1; j < 14; ++j) { const float t = std::pow (j / 14.0f, 2.0f), y = vy + (Hf - vy) * t; g.setColour (mag.withAlpha (0.10f + 0.2f * t)); g.drawLine (0, y, Wf, y, 1.0f); }
    // insegne verticali ai lati (decorazione)
    for (int s = 0; s < 2; ++s)
    {
        const float x = s ? Wf - 14 : 4; const auto c = s ? cya : mag;
        g.setColour (c.withAlpha (0.12f)); g.fillRect (x, 90.0f, 10.0f, 420.0f);
        for (int k = 0; k < 12; ++k) { g.setColour (c.withAlpha (k % 3 ? 0.25f : 0.7f)); g.fillRect (x + 2, 96.0f + k * 34, 6.0f, 20.0f); }
    }
    // pannelli
    cyberPanel (g, { 24, 84, 520, 428 }, mag, "SYS//RADAR  VOCE x BEAT");
    cyberPanel (g, { 560, 96, 608, 196 }, cya, "TIMELINE//FRASI");
    cyberPanel (g, { 560, 304, 608, 208 }, yel, "ONE CLICK//ANALISI");
    cyberPanel (g, { 24, 532, 1144, 176 }, pur, "CONTROL//DECK");
    // separatori della fila manopole
    g.setFont (fMono (10)); g.setColour (dim);
    auto grp = [&] (float x, float w, const char* t) { g.setColour (pur.withAlpha (0.5f)); g.fillRect (x, 548.0f, w, 1.0f); g.setColour (dim); g.drawText (t, juce::Rectangle<float> (x, 536, w, 12), juce::Justification::centred); };
    grp (196, 258, "DOVE APRIRE"); grp (466, 172, "QUANTO"); grp (650, 172, "TEMPI"); grp (834, 86, "VOCE"); grp (918, 182, "USCITA");
    // scanline
    for (float y = 0; y < Hf; y += 3) { g.setColour (juce::Colours::black.withAlpha (0.10f)); g.fillRect (0.0f, y, Wf, 1.0f); }
    // vignetta
    juce::ColourGradient vg (juce::Colours::transparentBlack, Wf / 2, Hf / 2, juce::Colours::black.withAlpha (0.55f), 0, 0, true); g.setGradientFill (vg); g.fillAll();
}

void Editor::paint (juce::Graphics& g)
{
    g.addTransform (juce::AffineTransform::scale (scaler.s));
    const float sc = juce::jlimit (1.0f, 4.0f, g.getInternalContext().getPhysicalPixelScaleFactor());
    if (bg.isNull() || std::abs (bgScale - sc) > 0.01f)
    {
        bgScale = sc; bg = juce::Image (juce::Image::RGB, juce::roundToInt (W * sc), juce::roundToInt (H * sc), false);
        juce::Graphics ig (bg); ig.addTransform (juce::AffineTransform::scale (sc)); paintStatic (ig);
    }
    { g.saveState(); g.addTransform (juce::AffineTransform::scale (1.0f / sc)); g.drawImageAt (bg, 0, 0); g.restoreState(); }
    const bool beat = proc.beatMode();
    // logo glitch
    {
        const float gx = glitchOff;
        g.setFont (fTitle (34));
        g.setColour (cya.withAlpha (0.8f)); g.drawText ("VOCAL", juce::Rectangle<float> (22 - 3 + gx, 10, 200, 40), juce::Justification::centredLeft);
        g.setColour (red.withAlpha (0.8f)); g.drawText ("VOCAL", juce::Rectangle<float> (22 + 3 - gx, 12, 200, 40), juce::Justification::centredLeft);
        g.setColour (ink); g.drawText ("VOCAL", juce::Rectangle<float> (22, 11, 200, 40), juce::Justification::centredLeft);
        g.setColour (mag); g.drawText ("RADAR", juce::Rectangle<float> (150 + gx * 0.5f, 11, 200, 40), juce::Justification::centredLeft);
        if (glitchT > 0) { g.setColour (cya.withAlpha (0.5f)); g.fillRect (22.0f, 26.0f + (frame % 7) * 2, 270.0f, 2.0f); }
        g.setFont (fMono (11)); g.setColour (dim); g.drawText ("JACKPOT // VOICE x BEAT INTERACTION ENGINE", juce::Rectangle<float> (24, 50, 300, 14), juce::Justification::centredLeft);
    }
    // preset
    {
        g.setColour (juce::Colour (0xcc0b0918)); g.fillPath (cut (presetR, 8)); g.setColour (pur.withAlpha (0.7f)); g.strokePath (cut (presetR, 8), juce::PathStrokeType (1.0f));
        g.setFont (fMono (9)); g.setColour (dim); g.drawText ("PRESET", presetR.withHeight (12).translated (0, 1), juce::Justification::centred);
        g.setFont (fLab (13)); g.setColour (ink); g.drawFittedText (proc.presetName, presetR.withTrimmedTop (10).toNearestInt(), juce::Justification::centred, 1);
        g.setFont (fLab (16)); g.setColour (pur); g.drawText ("<", prevR, juce::Justification::centred); g.drawText (">", nextR, juce::Justification::centred);
    }
    chipBtn (g, modeR[0], "VOCE", ! beat, mag, 12); chipBtn (g, modeR[1], "BEAT", beat, cya, 12);
    for (int c = 0; c < NCH; ++c) chipBtn (g, chanR[c], chName (c), proc.channel() == c, yel, 13);
    // stato del collegamento
    {
        const int c = proc.channel(); juce::String st; juce::Colour col = dim;
        if (! beat) { const int n = beatsListening (c); st = "TRASMETTE SU " + juce::String (chName (c)) + "  //  " + juce::String (n) + " BEAT IN ASCOLTO"; col = n > 0 ? mag : dim; }
        else if (proc.v ("src") > 0.5f) { st = proc.uiSide.load() ? "VOCE DAL SIDECHAIN" : "SIDECHAIN VUOTO"; col = proc.uiSide.load() ? cya : red; }
        else { const bool ok = proc.uiLinked.load(); st = ok ? "AGGANCIATO ALLA VOCE " + juce::String (chName (c)) : "NESSUNA VOCE SUL CANALE " + juce::String (chName (c)); col = ok ? cya : red; }
        const bool blink = (frame / 15) % 2 == 0;
        g.setColour (col.withAlpha (blink || col == cya || col == mag ? 1.0f : 0.4f)); g.fillEllipse (1048, 33, 9, 9);
        g.setFont (fMono (11)); g.setColour (col); g.drawFittedText (st, juce::Rectangle<int> (1062, 22, 112, 32), juce::Justification::centredLeft, 2);
    }
    // timeline: titolo
    g.setFont (fLab (12)); g.setColour (cya); g.drawText ("COSA SUCCEDE NEL TEMPO", juce::Rectangle<float> (900, 96 + 4, 300, 14), juce::Justification::centredLeft);
    // ONE CLICK
    if (beat)
    {
        const bool lr = proc.learning.load();
        chipBtn (g, clickR, lr ? "STOP E PROPONI  " + juce::String (proc.learnSec.load(), 0) + " s" : "ONE CLICK // ASCOLTA", lr, yel, 14);
        const auto& pr = proc.proposal;
        chipBtn (g, applyR, "APPLICA", pr.ready, cya, 11); chipBtn (g, discardR, proc.hasBackup ? "TORNA A PRIMA" : "SCARTA", false, red, 11);
        g.setFont (fLab (11.5f)); g.setColour (ink.withAlpha (0.88f));
        juce::StringArray lines = pr.lines;
        if (lines.isEmpty()) lines.add (lr ? "Sto ascoltando voce e beat insieme: lascia suonare almeno 10-20 secondi con la voce che canta, poi premi STOP." : "Premi ONE CLICK con voce e beat che suonano: il plugin ascolta, misura dove il beat copre la voce e propone le regolazioni. Tu applichi o scarti.");
        float y = 382;
        for (auto& l : lines) { const int h = l.length() > 90 ? 30 : 16; g.drawFittedText (l, juce::Rectangle<int> (588, (int) y, 560, h), juce::Justification::topLeft, 2); y += h + 3; }
        chipBtn (g, srcR[0], "LINK", proc.v ("src") < 0.5f, cya, 11); chipBtn (g, srcR[1], "SIDECHAIN", proc.v ("src") > 0.5f, cya, 11);
        chipBtn (g, deltaR, "ASCOLTA TOLTO", proc.v ("delta") > 0.5f, red, 11);
        g.setFont (fMono (10)); g.setColour (dim); g.drawText ("VOCE DA:", juce::Rectangle<float> (580, 470, 66, 26), juce::Justification::centredLeft);
    }
    else
    {
        g.setFont (fLab (15)); g.setColour (mag); g.drawText ("MODO VOCE  //  TRASMETTITORE", juce::Rectangle<float> (588, 320, 560, 20), juce::Justification::centredLeft);
        g.setFont (fLab (12)); g.setColour (ink.withAlpha (0.88f));
        g.drawFittedText (U ("Questa copia sta sulla traccia della voce: non tocca l'audio, manda solo la sua impronta sul canale scelto.\n"
                             "Sul beat (o sul bus del beat, o sulle singole melodie e sull'808) metti un'altra copia in modo BEAT sullo stesso canale.\n"
                             "Usa per tutte le copie lo stesso formato (tutte AU o tutte VST3), altrimenti non si vedono.\n"
                             "Canali diversi = voci diverse: A voce principale, B ad-lib, C doppie..."), juce::Rectangle<int> (588, 348, 560, 150), juce::Justification::topLeft, 8);
    }
}
void Editor::paintOverChildren (juce::Graphics& g)
{
    // pannello comandi spento in modo VOCE
    if (! proc.beatMode())
    {
        g.addTransform (juce::AffineTransform::scale (scaler.s));
        g.setColour (juce::Colour (0xc0050409)); g.fillRect (juce::Rectangle<float> (180, 552, 650, 150)); g.fillRect (juce::Rectangle<float> (40, 548, 130, 150)); g.fillRect (juce::Rectangle<float> (916, 552, 240, 150));
        g.setFont (fLab (13)); g.setColour (mag); g.drawText ("IN MODO VOCE CONTA SOLO LA SOGLIA", juce::Rectangle<float> (180, 610, 650, 20), juce::Justification::centred);
    }
}
void Editor::mouseDown (const juce::MouseEvent& e)
{
    const auto p = e.position / scaler.s;
    auto set = [this] (const char* id, float v) { proc.setV (id, v); repaint(); };
    if (modeR[0].contains (p)) { set ("mode", 0); return; }
    if (modeR[1].contains (p)) { set ("mode", 1); return; }
    for (int c = 0; c < NCH; ++c) if (chanR[c].contains (p)) { set ("chan", (float) c); return; }
    if (prevR.contains (p) || nextR.contains (p))
    {
        const int n = (int) presets().size(); int cur = 0; for (int i = 0; i < n; ++i) if (proc.presetName == presets()[(size_t) i].name) cur = i;
        proc.loadPreset ((cur + (nextR.contains (p) ? 1 : n - 1)) % n); repaint(); return;
    }
    if (presetR.contains (p)) { presetMenu(); return; }
    if (! proc.beatMode()) return;
    if (srcR[0].contains (p)) { set ("src", 0); return; }
    if (srcR[1].contains (p)) { set ("src", 1); return; }
    if (deltaR.contains (p)) { set ("delta", proc.v ("delta") > 0.5f ? 0.0f : 1.0f); return; }
    if (clickR.contains (p)) { if (proc.learning.load()) proc.learnStop(); else proc.learnStart(); repaint(); return; }
    if (applyR.contains (p)) { proc.learnApply(); repaint(); return; }
    if (discardR.contains (p)) { proc.learnDiscard(); repaint(); return; }
}
void Editor::presetMenu()
{
    juce::PopupMenu m; m.addSectionHeader ("PRESET");
    for (int i = 0; i < (int) presets().size(); ++i) m.addItem (1 + i, presets()[(size_t) i].name, true, proc.presetName == presets()[(size_t) i].name);
    juce::Component::SafePointer<Editor> sp (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withTargetScreenArea (localAreaToGlobal (scaler.map (presetR.toNearestInt()))), [sp] (int r) { if (sp != nullptr && r > 0) { sp->proc.loadPreset (r - 1); sp->repaint(); } });
}
void Editor::timerCallback()
{
    ++frame;
    radar.sweep += 0.07f; if (radar.sweep > juce::MathConstants<float>::twoPi) radar.sweep -= juce::MathConstants<float>::twoPi;
    for (int b = 0; b < NB; ++b)
    {
        radar.smV[b] += 0.35f * (proc.uiVoice[b].load() - radar.smV[b]); radar.smB[b] += 0.35f * (proc.uiBeat[b].load() - radar.smB[b]); radar.smC[b] += 0.4f * (proc.uiCut[b].load() - radar.smC[b]);
    }
    // glitch del logo ogni tanto
    if (glitchT > 0) { --glitchT; glitchOff = (float) juce::Random::getSystemRandom().nextInt (9) - 4.0f; }
    else { glitchOff = 0; if (juce::Random::getSystemRandom().nextInt (90) == 0) glitchT = 6; }
    const bool beat = proc.beatMode();
    for (auto* k : knobs) { const bool d = ! beat && k != sensK; if (d != k->dimmed) { k->dimmed = d; k->setEnabled (! d); k->repaint(); } }
    radar.repaint(); timeline.repaint();
    repaint (scaler.map (juce::Rectangle<int> (0, 0, 680, 70)));
    repaint (scaler.map (juce::Rectangle<int> (1040, 18, 140, 40)));
    if (proc.learning.load() || frame % 15 == 0) repaint (scaler.map (juce::Rectangle<int> (572, 310, 600, 196)));
}
}
