// Modalita FACILE: onda del brano al centro, INPUT con la zona ottimale, barra SAFE / OTTIMALE / CRUSH, meter, e poche manopole.
#include "MasterEditor.h"

namespace jms
{
static juce::String dbS1 (float v) { return (v > 0 ? "+" : "") + juce::String (v, 1); }
static const char* kViews[5] = { "ONDA", "SPETTRO", "GAIN", "REFERENCE", "SOLO ORECCHIE" };

EasyPage::EasyPage (Editor& e) : ed (e)
{
    auto mk = [this] (const char* id, juce::Colour c, const char* label, bool bip, std::function<juce::String()> t = {}) -> Knob*
    {
        auto* k = knobs.add (new Knob (ed.P (id), c)); k->label = label; k->bipolar = bip;
        if (t) k->text = std::move (t);
        addAndMakeVisible (k); return k;
    };
    auto db = [this] (const char* id) { return [this, id] { return dbS1 (ed.proc.value (id)) + " dB"; }; };
    auto pc = [this] (const char* id) { return [this, id] { return juce::String (juce::roundToInt (ed.proc.value (id) * 100)) + "%"; }; };
    low = mk ("e_low", C::amber, "BASSI", true, db ("e_low")); mid = mk ("e_mid", C::amber, "MEDI", true, db ("e_mid")); high = mk ("e_high", C::amber, "ALTI", true, db ("e_high"));
    thick = mk ("e_thick", C::magenta, "THICK", false, pc ("e_thick")); warm = mk ("e_warm", C::magenta, "WARM", false, pc ("e_warm")); silk = mk ("e_silk", C::magenta, "SILK", false, pc ("e_silk"));
    pres = mk ("e_pres", C::red, "PRESENZA", false, [this] { const float v = ed.proc.value ("e_pres"); return v < 0.3f ? juce::String ("MORBIDO") : v < 0.7f ? juce::String ("NATURALE") : juce::String ("DURO"); });
    loud = mk ("e_loud", C::red, "LOUD", false, [this] { return "+" + juce::String (ed.proc.value ("e_loud"), 1) + " dB"; }); loud->big = true;
    speed = mk ("e_speed", C::red, "VELOCITA", false, [this] { const float v = ed.proc.value ("e_speed"); return v < 0.33f ? juce::String ("SMOOTH") : v < 0.66f ? juce::String ("MEDIA") : juce::String ("FAST"); });
    glue = mk ("e_glue", C::violet, "GLUE", false, pc ("e_glue")); calm = mk ("e_calm", C::violet, "CALM", false, pc ("e_calm"));
    wide = mk ("e_wide", C::phos, "WIDE", true, [this] { return juce::String (juce::roundToInt (100 + ed.proc.value ("e_wide") * (ed.proc.value ("e_wide") > 0 ? 80 : 100))) + "%"; });
    ceil = mk ("lm_ceil", C::green, "CEILING", false, [this] { return juce::String (ed.proc.value ("lm_ceil"), 1) + " dB"; });
    trim = mk ("trim", C::phos, "INPUT", true, [this] { return dbS1 (ed.proc.value ("trim")) + " dB"; });
    monoB = std::make_unique<Switch> (ed.P ("e_monob"), C::phos, "BASSI MONO"); addAndMakeVisible (*monoB);
    static const char* tips[][2] = { { "e_low", "Shelf morbido sotto i 90 Hz: peso e sub." }, { "e_mid", "Campana larga a 1.2 kHz: presenza della voce e del rullante." }, { "e_high", "Shelf d'aria sopra i 9 kHz." },
                                     { "e_thick", "Saturazione a valvola solo sui medio-bassi: spessore e corpo." }, { "e_warm", "Nastro da 15 pollici: compatta e arrotonda." }, { "e_silk", "Armoniche solo sugli alti: lucentezza senza durezza." },
                                     { "e_glue", "Compressore di bus 2:1 con rilascio automatico: tiene insieme il mix." }, { "e_calm", "Abbassa da solo le risonanze che saltano fuori su fango, durezze e sibilanti." },
                                     { "e_wide", "Allarga (o stringe) lo stereo sopra 120 Hz." } };
    juce::ignoreUnused (tips);
}

void EasyPage::resized()
{
    const float x0 = 8, y0 = 412;
    auto place = [] (Knob* k, float x, float y, float s) { k->setBounds (juce::Rectangle<float> (x, y, s, s + 27).toNearestInt()); };
    // EQ
    place (low, x0 + 16, y0 + 70, 70); place (mid, x0 + 100, y0 + 70, 70); place (high, x0 + 184, y0 + 70, 70);
    // colore
    place (thick, x0 + 296, y0 + 70, 70); place (warm, x0 + 380, y0 + 70, 70); place (silk, x0 + 464, y0 + 70, 70);
    // LOUD al centro con presenza e velocita ai lati
    place (pres, x0 + 568, y0 + 100, 64); loud->setBounds (juce::Rectangle<float> (x0 + 642, y0 + 40, 160, 160 + 32).toNearestInt()); place (speed, x0 + 812, y0 + 100, 64);
    // dinamica e spazio
    place (glue, x0 + 908, y0 + 70, 70); place (calm, x0 + 992, y0 + 70, 70); place (wide, x0 + 1076, y0 + 70, 70);
    place (ceil, x0 + 1150, y0 + 70, 70);
    monoB->setBounds (juce::Rectangle<float> (x0 + 1066, y0 + 190, 120, 22).toNearestInt());
    place (trim, 24, 300, 64);
}

// ----------------------------------------------------------------------- disegno
static void section (juce::Graphics& g, juce::Rectangle<float> r, const char* t, juce::Colour c)
{
    jlook::softShadow (g, r, 12); jlook::card (g, r, 12, c, true, false, false);
    glowText (g, t, r.withHeight (28).withTrimmedLeft (14).translated (0, 4), C::text, uf (11.5f));
}
void EasyPage::paint (juce::Graphics& g)
{
    auto& P = ed.proc;
    // ---- selettore delle viste
    float x = 128;
    for (int i = 0; i < 5; ++i) { const float w = i == 4 ? 130.0f : i == 3 ? 100.0f : 80.0f; viewR[i] = { x, 2, w, 28 }; x += w + 6; jlook::button (g, viewR[i], C::phos, view == i, false, kViews[i], uf (10.5f)); }
    const auto& T = target ((int) P.value ("target"));
    targetR = { 940, 2, 150, 28 }; cmpR = { 1096, 2, 140, 28 };
    jlook::button (g, targetR, C::green, true, false, juce::String ("TARGET: ") + T.name + juce::String::fromUTF8 (" \xe2\x96\xbe"), uf (10.0f));
    jlook::button (g, cmpR, C::phos, P.value ("compare") > 0.5f, false, P.value ("compare") > 0.5f ? "ORIGINALE (VOL. UGUALE)" : "CONFRONTA", uf (9.5f));

    // ---- INPUT con la zona ottimale
    auto inR = juce::Rectangle<float> (0, 38, 112, 360); section (g, inR, "INPUT", C::phos);
    {
        auto m = juce::Rectangle<float> (16, 72, 30, 158); lcd (g, m, 5);
        auto yL = [&m] (float db) { return m.getBottom() - juce::jlimit (0.0f, 1.0f, (db + 36.0f) / 36.0f) * m.getHeight(); };
        const float lo = Processor::kInOpt - 2.5f, hi = Processor::kInOpt + 2.5f;
        g.setColour (C::green.withAlpha (0.18f)); g.fillRect (juce::Rectangle<float>::leftTopRightBottom (m.getX(), yL (hi), m.getRight(), yL (lo)));
        const float s = P.inS.load(), a = P.inAvg.load();
        if (s > -60) { g.setColour (C::phos.withAlpha (0.8f)); g.fillRect (juce::Rectangle<float>::leftTopRightBottom (m.getX() + 4, yL (s), m.getRight() - 4, m.getBottom())); }
        if (a > -60) { g.setColour (juce::Colours::white); g.fillRect (m.getX() - 3, yL (a) - 1, m.getWidth() + 6, 2.0f); }
        g.setFont (mf (8.0f)); g.setColour (C::faint); for (int d : { -30, -24, -18, -12, -6, 0 }) g.drawText (juce::String (d), juce::Rectangle<float> (m.getRight() + 4, yL ((float) d) - 5, 26, 10), juce::Justification::centredLeft);
        g.setFont (uf (7.5f)); g.setColour (C::green.withAlpha (0.8f)); g.drawText ("ZONA OK", juce::Rectangle<float> (m.getRight() + 26, yL (Processor::kInOpt) - 5, 40, 10), juce::Justification::centredLeft);
        juce::String st = "suona un pezzo"; juce::Colour c = C::dim;
        if (a > -60) { if (a > hi) { st = "ABBASSA"; c = C::red; } else if (a < lo) { st = "ALZA"; c = C::amber; } else { st = "OK"; c = C::green; } }
        autoR = { 14, 236, 84, 24 }; jlook::button (g, autoR, C::phos, false, false, "AUTO", uf (10.0f));
        glowText (g, st, { 6, 264, 100, 16 }, c, uf (12.0f), juce::Justification::centred);
        g.setFont (mf (8.5f)); g.setColour (C::dim); g.drawText (a > -60 ? juce::String (a, 1) + " LUFS medi" : juce::String ("--"), juce::Rectangle<float> (6, 280, 100, 12), juce::Justification::centred);
    }
    // ---- vista centrale
    auto vR = juce::Rectangle<float> (120, 38, 780, 360); jlook::softShadow (g, vR, 12); jlook::card (g, vR, 12, C::phos, true, false, false);
    auto plot = vR.reduced (12, 12); lcd (g, plot, 6);
    switch (view)
    {
        case 1: paintSpectrum (g, plot); break;
        case 2: paintGain (g, plot); break;
        case 3: paintRef (g, plot); break;
        case 4: { glowText (g, "ASCOLTA", plot.withSizeKeepingCentre (400, 60), C::text.withAlpha (0.5f), uf (34.0f), juce::Justification::centred);
                  g.setFont (uf (11.0f, false)); g.setColour (C::dim); g.drawText ("niente grafici: giudica solo con le orecchie", plot.withTrimmedTop (plot.getHeight() * 0.5f + 34).withHeight (20), juce::Justification::centred); break; }
        default: paintWave (g, plot); break;
    }
    jlook::glass (g, plot, 6);
    // ---- barra del risultato: SAFE in basso, OTTIMALE al centro, CRUSH in alto
    auto rR = juce::Rectangle<float> (908, 38, 84, 360); jlook::softShadow (g, rR, 12); jlook::card (g, rR, 12, C::amber, true, false, false);
    {
        auto b = juce::Rectangle<float> (920, 60, 18, 318);
        juce::ColourGradient cg (C::red, 0, b.getY(), C::phos, 0, b.getBottom(), false); cg.addColour (0.5, C::green); cg.addColour (0.25, C::amber);
        g.setGradientFill (cg); g.setOpacity (0.35f); g.fillRoundedRectangle (b, 6); g.setOpacity (1.0f);
        g.setColour (juce::Colours::white.withAlpha (0.2f)); g.drawRoundedRectangle (b, 6, 1);
        auto lab = [&] (const char* t, float y, juce::Colour c) { g.setColour (c.withAlpha (0.6f)); g.drawHorizontalLine ((int) y, b.getRight() + 1, b.getRight() + 6); g.setFont (uf (8.5f)); g.setColour (c); g.drawText (t, juce::Rectangle<float> (b.getRight() + 8, y - 6, 44, 12), juce::Justification::centredLeft); };
        lab ("CRUSH", b.getY() + 10, C::red); lab ("OTTIMO", b.getCentreY(), C::green); lab ("SAFE", b.getBottom() - 10, C::phos);
        crushSm += (P.crush.load() - crushSm) * 0.15f;
        const float y = b.getBottom() - crushSm * b.getHeight();
        const auto c = crushSm > 0.7f ? C::red : crushSm < 0.3f ? C::phos : C::green;
        g.setColour (c.withAlpha (0.35f)); g.fillRoundedRectangle (b.getX() - 6, y - 6, b.getWidth() + 12, 12, 4);
        g.setColour (c); g.fillRoundedRectangle (b.getX() - 4, y - 3, b.getWidth() + 8, 6, 3);
    }
    // ---- meter
    auto mR = juce::Rectangle<float> (1000, 38, 244, 360); section (g, mR, "MISURE", C::green);
    {
        const float S = P.lufsS.load(), I = P.lufsI.load(), M = P.lufsM.load(), tp = P.tpMax.load(), gr = P.grAvg.load();
        auto big = juce::Rectangle<float> (1012, 72, 220, 74); lcd (g, big, 6);
        const float d = S - T.lufs; const auto c = S < -69 ? C::dim : std::abs (d) <= 1 ? C::green : d > 1 ? C::red : C::amber;
        glowText (g, S < -69 ? juce::String ("--.-") : juce::String (S, 1), big.reduced (12, 6).withTrimmedBottom (20), c, mf (32.0f));
        g.setFont (uf (9.0f)); g.setColour (C::dim); g.drawText ("LUFS SHORT-TERM", big.reduced (12, 6), juce::Justification::topRight);
        g.setColour (c); g.drawText (S < -69 ? juce::String ("in attesa") : juce::String ("target ") + juce::String (T.lufs, 0) + "  (" + dbS1 (d) + ")", big.reduced (12, 6), juce::Justification::bottomLeft);
        struct V { const char* n; juce::String v; juce::Colour c; };
        const float plr = (tp > -60 && I > -60) ? tp - I : -99;
        V vals[] = { { "INTEGRATO", I < -69 ? juce::String ("--") : juce::String (I, 1) + " LUFS", C::phos }, { "MOMENTARY", M < -69 ? juce::String ("--") : juce::String (M, 1), C::phos },
                     { "TRUE PEAK", tp < -60 ? juce::String ("--") : juce::String (tp, 1) + " dBTP", tp > T.tp ? C::red : C::green }, { "PLR", plr < -90 ? juce::String ("--") : juce::String (plr, 1) + " dB", plr > 0 && plr < 7 ? C::red : C::amber },
                     { "LRA", juce::String (P.lra.load(), 1) + " LU", C::violet }, { "RIDUZIONE", gr < 0.05f ? juce::String ("0.0 dB") : "-" + juce::String (gr, 1) + " dB", gr > 6 ? C::red : C::amber },
                     { "COLLA", P.glueGr.load() < 0.05f ? juce::String ("0.0 dB") : "-" + juce::String (P.glueGr.load(), 1) + " dB", C::violet }, { "CALM", P.calmNow.load() > -0.05f ? juce::String ("0.0 dB") : juce::String (P.calmNow.load(), 1) + " dB", C::violet } };
        for (int i = 0; i < 8; ++i)
        {
            auto r = juce::Rectangle<float> (1012 + (i % 2) * 112.0f, 154 + (i / 2) * 42.0f, 108, 38); lcd (g, r, 5);
            g.setFont (uf (8.0f)); g.setColour (C::dim); g.drawText (vals[i].n, r.reduced (8, 3).withHeight (12), juce::Justification::centredLeft);
            glowText (g, vals[i].v, r.reduced (8, 3).withTrimmedTop (13), vals[i].c, mf (12.0f));
        }
        juce::String adv; juce::Colour ac = C::dim;
        if (S < -60) adv = "premi play";
        else if (crushSm > 0.7f) { adv = "troppo schiacciato: abbassa LOUD"; ac = C::red; }
        else if (crushSm < 0.3f) { adv = "c'e margine: puoi alzare LOUD"; ac = C::phos; }
        else { adv = "nel punto giusto"; ac = C::green; }
        glowText (g, adv, { 1012, 330, 220, 16 }, ac, uf (10.5f));
        g.setFont (uf (8.5f, false)); g.setColour (C::dim); g.drawText ("ceiling " + juce::String (P.value ("lm_ceil"), 1) + juce::String::fromUTF8 (" dBTP  \xc2\xb7  latenza ") + juce::String (P.getLatencySamples() * 1000.0 / juce::jmax (1.0, P.sampleRate()), 1) + " ms", juce::Rectangle<float> (1012, 350, 220, 14), juce::Justification::centredLeft);
    }
    // ---- manopole
    section (g, { 8, 412, 270, 290 }, "EQ", C::amber);
    section (g, { 288, 412, 270, 290 }, "COLORE", C::magenta);
    section (g, { 568, 412, 330, 290 }, "VOLUME", C::red);
    section (g, { 908, 412, 336, 290 }, "DINAMICA E SPAZIO", C::violet);
    g.setFont (uf (9.0f, false)); g.setColour (C::dim);
    g.drawFittedText ("Bande fisse con curve da master: bassi sotto 90 Hz, medi a 1.2 kHz, aria sopra 9 kHz.", juce::Rectangle<int> (24, 590, 240, 40), juce::Justification::topLeft, 3);
    g.drawFittedText ("THICK: spessore a valvola sui medio-bassi.  WARM: nastro.  SILK: lucentezza sugli alti.", juce::Rectangle<int> (304, 590, 240, 40), juce::Justification::topLeft, 3);
    g.drawFittedText ("LOUD alza il volume. PRESENZA: morbido o duro (clipper). VELOCITA: rilascio smooth o fast.", juce::Rectangle<int> (584, 640, 300, 40), juce::Justification::topLeft, 3);
    g.drawFittedText ("GLUE: colla di bus.  CALM: doma da solo le risonanze.  WIDE: stereo sopra 120 Hz.", juce::Rectangle<int> (924, 620, 300, 40), juce::Justification::topLeft, 3);
}

void EasyPage::paintWave (juce::Graphics& g, juce::Rectangle<float> p)
{
    auto& P = ed.proc; const int N = Processor::kHist, w0 = P.hW.load();
    const float cy = p.getCentreY(), h = p.getHeight() * 0.5f - 8;
    auto lin = [] (float db) { return juce::jlimit (0.0f, 1.0f, juce::Decibels::decibelsToGain (db)); };
    g.setColour (C::phos.withAlpha (0.08f)); g.drawHorizontalLine ((int) cy, p.getX(), p.getRight());
    const float ceilL = lin (P.value ("lm_ceil"));
    g.setColour (C::red.withAlpha (0.35f)); g.drawHorizontalLine ((int) (cy - ceilL * h), p.getX(), p.getRight()); g.drawHorizontalLine ((int) (cy + ceilL * h), p.getX(), p.getRight());
    const float W = p.getWidth();
    for (int x = 0; x < (int) W; ++x)
    {
        const int a = (int) (x / W * N), b = juce::jmax (a + 1, (int) ((x + 1) / W * N));
        float in = -100, out = -100, gr = 0;
        for (int k = a; k < b; ++k) { const int i = (w0 + k) % N; in = juce::jmax (in, P.hIn[i].load()); out = juce::jmax (out, P.hOut[i].load()); gr = juce::jmax (gr, P.hGrM[i].load() + P.hGrL[i].load()); }
        const float xi = p.getX() + x;
        if (in > -60) { g.setColour (juce::Colours::white.withAlpha (0.13f)); g.drawVerticalLine ((int) xi, cy - lin (in) * h, cy + lin (in) * h); }
        if (out > -60) { g.setColour (C::phos.withAlpha (0.75f)); g.drawVerticalLine ((int) xi, cy - lin (out) * h, cy + lin (out) * h); }
        if (gr > 0.05f) { g.setColour (C::red.withAlpha (0.7f)); g.drawVerticalLine ((int) xi, p.getY(), p.getY() + juce::jmin (1.0f, gr / 12.0f) * h * 0.6f); }
    }
    g.setFont (uf (9.0f)); g.setColour (C::dim);
    g.drawText (juce::String::fromUTF8 ("ultimi 20 s  \xc2\xb7  grigio = ingresso  \xc2\xb7  ciano = uscita  \xc2\xb7  rosso dall'alto = quanto lavora il limiter"), p.reduced (10, 6), juce::Justification::bottomLeft);
}
void EasyPage::paintSpectrum (juce::Graphics& g, juce::Rectangle<float> p)
{
    const int NP = 300; const double sr = ed.proc.sampleRate();
    anIn.run (ed.proc.ringIn, specIn, NP, 20, 20000, sr); anOut.run (ed.proc.ringOut, specOut, NP, 20, 20000, sr);
    auto xOf = [&p] (float f) { return p.getX() + p.getWidth() * std::log (f / 20.0f) / std::log (1000.0f); };
    for (float f : { 50.f, 100.f, 200.f, 500.f, 1000.f, 2000.f, 5000.f, 10000.f }) { g.setColour (C::phos.withAlpha (0.06f)); g.drawVerticalLine ((int) xOf (f), p.getY(), p.getBottom()); g.setColour (C::faint); g.setFont (mf (8.0f)); g.drawText (f >= 1000 ? juce::String ((int) f / 1000) + "k" : juce::String ((int) f), juce::Rectangle<float> (xOf (f) - 14, p.getBottom() - 14, 28, 12), juce::Justification::centred); }
    auto path = [&] (const float* s, bool fill)
    {
        juce::Path pt;
        if (fill) pt.startNewSubPath (p.getX(), p.getBottom());
        for (int i = 0; i < NP; ++i)
        {
            const float x = p.getX() + p.getWidth() * (i + 0.5f) / NP, y = p.getBottom() - juce::jlimit (0.0f, 1.0f, (s[i] + 84.0f) / 84.0f) * p.getHeight();
            if (i == 0 && ! fill) pt.startNewSubPath (x, y); else pt.lineTo (x, y);
        }
        if (fill) { pt.lineTo (p.getRight(), p.getBottom()); pt.closeSubPath(); }
        return pt;
    };
    g.setColour (juce::Colours::white.withAlpha (0.35f)); g.strokePath (path (specIn, false), juce::PathStrokeType (1.0f));
    juce::ColourGradient sg (C::phos.withAlpha (0.3f), 0, p.getY(), C::phos.withAlpha (0.03f), 0, p.getBottom(), false); g.setGradientFill (sg); g.fillPath (path (specOut, true));
    g.setColour (C::phos); g.strokePath (path (specOut, false), juce::PathStrokeType (1.4f));
    g.setFont (uf (9.0f)); g.setColour (C::dim); g.drawText (juce::String::fromUTF8 ("bianco = prima  \xc2\xb7  ciano = dopo il master"), p.reduced (10, 6), juce::Justification::topLeft);
}
void EasyPage::paintGain (juce::Graphics& g, juce::Rectangle<float> p)
{
    auto& P = ed.proc; const int N = Processor::kHist, w0 = P.hW.load(); const auto& T = target ((int) P.value ("target"));
    auto yS = [&p] (float l) { return p.getY() + 10 + (1.0f - juce::jlimit (0.0f, 1.0f, (l + 30.0f) / 30.0f)) * (p.getHeight() * 0.55f); };
    auto yG = [&p] (float gr) { return p.getBottom() - 16 - juce::jlimit (0.0f, 1.0f, gr / 12.0f) * (p.getHeight() * 0.35f); };
    g.setColour (C::green.withAlpha (0.5f)); g.drawHorizontalLine ((int) yS (T.lufs), p.getX(), p.getRight());
    g.setFont (mf (8.5f)); g.setColour (C::green); g.drawText ("target " + juce::String (T.lufs, 0) + " LUFS", juce::Rectangle<float> (p.getX() + 8, yS (T.lufs) - 14, 160, 12), juce::Justification::centredLeft);
    juce::Path ls, gm, gl; bool s1 = false;
    for (int k = 0; k < N; ++k)
    {
        const int i = (w0 + k) % N; const float x = p.getX() + p.getWidth() * k / (N - 1);
        const float s = P.hS[i].load(); if (s > -60) { if (! s1) { ls.startNewSubPath (x, yS (s)); s1 = true; } else ls.lineTo (x, yS (s)); }
        if (k == 0) { gm.startNewSubPath (x, yG (0)); gl.startNewSubPath (x, yG (0)); }
        gm.lineTo (x, yG (P.hGrM[i].load())); gl.lineTo (x, yG (P.hGrM[i].load() + P.hGrL[i].load()));
    }
    gl.lineTo (p.getRight(), yG (0)); gl.closeSubPath(); gm.lineTo (p.getRight(), yG (0)); gm.closeSubPath();
    g.setColour (C::red.withAlpha (0.3f)); g.fillPath (gl); g.setColour (C::magenta.withAlpha (0.55f)); g.fillPath (gm);
    g.setColour (C::amber.withAlpha (0.35f)); g.strokePath (ls, juce::PathStrokeType (4.0f)); g.setColour (C::amber); g.strokePath (ls, juce::PathStrokeType (1.5f));
    g.setFont (uf (9.0f)); g.setColour (C::dim);
    g.drawText (juce::String::fromUTF8 ("ambra = LUFS short-term  \xc2\xb7  in basso: riduzione (magenta = LOUD, rosso = true peak), scala 12 dB"), p.reduced (10, 6), juce::Justification::topLeft);
}
void EasyPage::paintRef (juce::Graphics& g, juce::Rectangle<float> p)
{
    auto& P = ed.proc; auto rt = P.reference();
    loadR = { p.getX() + 10, p.getY() + 8, 150, 26 }; listenR = { p.getX() + 166, p.getY() + 8, 170, 26 };
    jlook::button (g, loadR, C::amber, false, false, P.refLoading.load() ? "ANALISI..." : "CARICA PEZZO...", uf (10.0f));
    jlook::button (g, listenR, C::phos, P.refListen.load(), false, P.refListen.load() ? "ASCOLTI IL RIFERIMENTO" : "ASCOLTA RIFERIMENTO", uf (10.0f));
    if (! rt) { g.setFont (uf (11.0f, false)); g.setColour (C::dim); g.drawFittedText ("Carica un master che ti piace: lo confronti con il tuo (spettro e loudness) e lo ascolti allo stesso volume.", p.reduced (40).toNearestInt(), juce::Justification::centred, 3); return; }
    g.setFont (uf (10.0f)); g.setColour (C::amber); g.drawText (rt->name + "   " + juce::String (rt->lufs, 1) + " LUFS   picco " + juce::String (rt->tp, 1) + " dBTP", juce::Rectangle<float> (p.getX() + 350, p.getY() + 8, p.getWidth() - 360, 26), juce::Justification::centredLeft);
    auto pl = p.withTrimmedTop (44).reduced (10, 10);
    auto xOf = [&pl] (int k) { return pl.getX() + pl.getWidth() * (k + 0.5f) / kThirds; };
    auto yOf = [&pl] (float d) { return pl.getCentreY() - juce::jlimit (-15.0f, 15.0f, d) / 15.0f * pl.getHeight() * 0.5f; };
    g.setColour (C::phos.withAlpha (0.1f)); g.drawHorizontalLine ((int) yOf (0), pl.getX(), pl.getRight());
    juce::Path pr, pm;
    for (int k = 0; k < kThirds; ++k) { const float x = xOf (k); if (k == 0) { pr.startNewSubPath (x, yOf (rt->thirds[k])); pm.startNewSubPath (x, yOf (ed.liveThirds[k])); } else { pr.lineTo (x, yOf (rt->thirds[k])); pm.lineTo (x, yOf (ed.liveThirds[k])); } }
    g.setColour (C::amber); g.strokePath (pr, juce::PathStrokeType (2.0f));
    if (ed.live.hasData()) { g.setColour (C::phos); g.strokePath (pm, juce::PathStrokeType (2.0f)); }
    g.setFont (mf (8.0f)); g.setColour (C::faint);
    for (int k = 0; k < kThirds; k += 3) { const float f = thirdHz (k); g.drawText (f >= 1000 ? juce::String (f / 1000, 1) + "k" : juce::String ((int) f), juce::Rectangle<float> (xOf (k) - 16, pl.getBottom() - 12, 32, 12), juce::Justification::centred); }
    g.setFont (uf (9.0f)); g.setColour (C::dim); g.drawText (juce::String::fromUTF8 ("ambra = riferimento  \xc2\xb7  ciano = il tuo master (forma, volume allineato)"), pl, juce::Justification::topLeft);
}

void EasyPage::mouseDown (const juce::MouseEvent& e)
{
    auto& P = ed.proc;
    for (int i = 0; i < 5; ++i) if (viewR[i].contains (e.position)) { view = i; repaint(); return; }
    if (autoR.contains (e.position)) { P.autoInput(); repaint(); return; }
    if (cmpR.contains (e.position)) { auto* p = ed.P ("compare"); p->setValueNotifyingHost (p->getValue() > 0.5f ? 0.0f : 1.0f); repaint(); return; }
    if (targetR.contains (e.position))
    {
        juce::PopupMenu m; const int cur = (int) P.value ("target");
        for (int i = 0; i < kNumTargets; ++i) m.addItem (i + 1, juce::String (target (i).name) + "   " + juce::String (target (i).lufs, 0) + " LUFS", true, i == cur);
        auto* tp = ed.P ("target");
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [tp] (int r) { if (r > 0) tp->setValueNotifyingHost (tp->convertTo0to1 ((float) (r - 1))); });
        return;
    }
    if (view == 3 && loadR.contains (e.position))
    {
        chooser = std::make_unique<juce::FileChooser> ("Scegli il pezzo di riferimento", juce::File::getSpecialLocation (juce::File::userMusicDirectory), "*.wav;*.aif;*.aiff;*.mp3;*.m4a;*.flac");
        juce::Component::SafePointer<EasyPage> sp (this);
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [sp] (const juce::FileChooser& c) { if (sp && c.getResult().existsAsFile()) sp->ed.proc.loadReference (c.getResult()); });
        return;
    }
    if (view == 3 && listenR.contains (e.position)) { P.refListen = ! P.refListen.load(); repaint(); return; }
}
}
