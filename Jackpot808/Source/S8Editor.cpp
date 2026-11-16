// JACKPOT 808 STUDIO: stessa notte al neon di Vocal Radar. Accordatore, spettro con le zone, diagnosi "cosa manca".
#include "S8Editor.h"

namespace s8
{
static juce::String U (const char* s) { return juce::String::fromUTF8 (s); }

// ======================================================================= accordatore e nota piu' usata
void TunerView::paint (juce::Graphics& g)
{
    auto& P = ed.proc; auto b = getLocalBounds().toFloat();
    const bool on = P.uiConf.load() > 0.6f && P.uiLevel.load() > -50 && hzS > 20;
    juce::String nm = "--", oct; float cents = 0;
    if (hzS > 20)
    {
        const double mn = 69.0 + 12.0 * std::log2 (hzS / 440.0); const int n = (int) std::round (mn);
        nm = noteName (n); oct = juce::String (n / 12 - 1); cents = (float) ((mn - n) * 100.0);
    }
    // nota grande con glitch
    g.setFont (fTitle (96));
    const auto col = ! on ? dim : std::abs (centS) < 8 ? cya : std::abs (centS) < 20 ? yel : red;
    juce::Rectangle<float> nr (b.getX(), 10, b.getWidth(), 110);
    if (on) { g.setColour (mag.withAlpha (0.5f)); g.drawText (nm, nr.translated (-3, 0), juce::Justification::centred); }
    g.setColour (col); g.drawText (nm, nr, juce::Justification::centred);
    g.setFont (fTitle (30)); g.setColour (col.withAlpha (0.8f)); g.drawText (oct, juce::Rectangle<float> (b.getCentreX() + 62, 64, 40, 40), juce::Justification::centredLeft);
    g.setFont (fMono (14)); g.setColour (ink.withAlpha (0.8f));
    g.drawText (on ? juce::String (hzS, 1) + " Hz   " + (centS >= 0 ? "+" : "") + juce::String (juce::roundToInt (centS)) + " cent" : juce::String ("in attesa dell'808..."), juce::Rectangle<float> (0, 122, b.getWidth(), 18), juce::Justification::centred);
    // barra dei cent
    auto cb = juce::Rectangle<float> (30, 150, b.getWidth() - 60, 16);
    g.setColour (juce::Colour (0xff120e22)); g.fillRect (cb);
    for (int k = -5; k <= 5; ++k) { const float x = cb.getCentreX() + cb.getWidth() * 0.5f * k / 5; g.setColour ((k == 0 ? cya : dim).withAlpha (k == 0 ? 0.9f : 0.4f)); g.fillRect (x - 0.5f, cb.getY() - (k == 0 ? 4 : 0), 1.0f, cb.getHeight() + (k == 0 ? 8 : 0)); }
    if (on) { const float x = cb.getCentreX() + cb.getWidth() * 0.5f * juce::jlimit (-1.0f, 1.0f, centS / 50.0f); g.setColour (col.withAlpha (0.3f)); g.fillRect (x - 6, cb.getY() - 2, 12.0f, cb.getHeight() + 4); g.setColour (col); g.fillRect (x - 2, cb.getY() - 2, 4.0f, cb.getHeight() + 4); }
    g.setFont (fMono (10)); g.setColour (dim); g.drawText ("-50", cb.withY (cb.getBottom() + 2).withHeight (12), juce::Justification::centredLeft); g.drawText ("+50", cb.withY (cb.getBottom() + 2).withHeight (12), juce::Justification::centredRight);
    // le 12 note: quanto suona ciascuna
    float mx = 0; for (auto& h : P.pcHist) mx = juce::jmax (mx, h.load());
    int top = -1; float topV = 0; for (int k = 0; k < 12; ++k) if (P.pcHist[k].load() > topV) { topV = P.pcHist[k].load(); top = k; }
    auto kb = juce::Rectangle<float> (24, 214, b.getWidth() - 48, 130);
    const float kw = kb.getWidth() / 12;
    for (int k = 0; k < 12; ++k)
    {
        const bool black = k == 1 || k == 3 || k == 6 || k == 8 || k == 10; const float t = mx > 0 ? P.pcHist[k].load() / mx : 0;
        auto r = juce::Rectangle<float> (kb.getX() + k * kw + 2, kb.getY(), kw - 4, kb.getHeight() - 22);
        g.setColour (black ? juce::Colour (0xff0d0a18) : juce::Colour (0xff1b1630)); g.fillPath (cut (r, 4));
        const auto c = k == top ? yel : mag;
        if (t > 0) { auto f = r.withTrimmedTop (r.getHeight() * (1 - t)); g.setColour (c.withAlpha (0.25f)); g.fillRect (f); g.setColour (c); g.fillRect (f.withHeight (3.0f)); }
        g.setFont (fLab (11)); g.setColour (k == top ? yel : black ? dim : ink.withAlpha (0.8f)); g.drawText (noteName (k), juce::Rectangle<float> (r.getX(), kb.getBottom() - 18, r.getWidth(), 14), juce::Justification::centred);
    }
    g.setFont (fLab (12)); g.setColour (top >= 0 ? yel : dim);
    g.drawText (top >= 0 ? "NOTA PIU' USATA: " + juce::String (noteName (top)) + "  //  PROBABILE TONICA" : juce::String ("NOTE: suona qualche battuta"), juce::Rectangle<float> (0, 350, b.getWidth(), 16), juce::Justification::centred);
    if (P.pitchN.load() > 20)
    {
        g.setFont (fMono (11)); g.setColour (ink.withAlpha (0.75f));
        g.drawText ("accordatura media " + juce::String (P.centsAvg.load() >= 0 ? "+" : "") + juce::String (juce::roundToInt (P.centsAvg.load())) + " cent  //  fondamentale media " + juce::String (juce::roundToInt (P.f0Avg.load())) + " Hz",
                    juce::Rectangle<float> (0, 372, b.getWidth(), 14), juce::Justification::centred);
    }
}

// ======================================================================= spettro e storia
void SpecView::compute()
{
    auto& P = ed.proc; const int w0 = P.ringW.load();
    for (int i = 0; i < NF; ++i) buf[(size_t) i] = P.ring[(w0 - NF + i) & (Processor::kRing - 1)] * win[(size_t) i];
    std::fill (buf.begin() + NF, buf.end(), 0.0f);
    fft.performFrequencyOnlyForwardTransform (buf.data());
    for (int i = 0; i < NF / 2; ++i) { const float db = juce::Decibels::gainToDecibels (buf[(size_t) i] / (NF * 0.25f) + 1e-9f); spm[(size_t) i] += (db > spm[(size_t) i] ? 0.6f : 0.15f) * (db - spm[(size_t) i]); }
}
void SpecView::paint (juce::Graphics& g)
{
    auto& P = ed.proc; auto b = getLocalBounds().toFloat();
    auto sp = juce::Rectangle<float> (16, 26, b.getWidth() - 32, 210);
    const double sr = P.sampleRate();
    auto xOf = [&] (float hz) { return sp.getX() + sp.getWidth() * std::log (hz / 20.0f) / std::log (100.0f); };   // 20 Hz .. 2 kHz
    auto yOf = [&] (float db) { return sp.getBottom() - sp.getHeight() * juce::jlimit (0.0f, 1.0f, (db + 84.0f) / 78.0f); };
    // zone
    struct Z { float a, b; juce::Colour c; const char* t; } zones[3] = { { 20, 90, cya, "SUB" }, { 90, 400, mag, "CORPO" }, { 400, 2000, yel, "ARMONICHE / GRINTA" } };
    for (auto& z : zones) { g.setColour (z.c.withAlpha (0.06f)); g.fillRect (juce::Rectangle<float> (xOf (z.a), sp.getY(), xOf (z.b) - xOf (z.a), sp.getHeight())); g.setFont (fMono (10)); g.setColour (z.c.withAlpha (0.8f)); g.drawText (z.t, juce::Rectangle<float> (xOf (z.a) + 4, sp.getY() + 2, 160, 12), juce::Justification::centredLeft); }
    for (float hz : { 30.f, 50.f, 100.f, 200.f, 500.f, 1000.f }) { g.setColour (cya.withAlpha (0.08f)); g.fillRect (xOf (hz), sp.getY(), 1.0f, sp.getHeight()); g.setFont (fMono (9.5f)); g.setColour (dim); g.drawText (hz >= 1000 ? "1k" : juce::String ((int) hz), juce::Rectangle<float> (xOf (hz) - 15, sp.getBottom() + 2, 30, 11), juce::Justification::centred); }
    // nota rilevata: righe della fondamentale e delle armoniche
    const float f0 = P.uiHz.load();
    if (P.uiConf.load() > 0.6f && f0 > 20) for (int k = 1; k <= 6; ++k) { const float x = xOf (f0 * k); if (x > sp.getRight()) break; g.setColour ((k == 1 ? cya : yel).withAlpha (k == 1 ? 0.6f : 0.3f)); g.fillRect (x - 0.5f, sp.getY(), 1.0f, sp.getHeight()); }
    juce::Path p; bool st = false;
    for (int px = 0; px < (int) sp.getWidth(); px += 2)
    {
        const float hz = 20.0f * std::pow (100.0f, px / sp.getWidth()); const float bin = hz / (float) sr * NF;
        const int b0 = (int) bin; if (b0 + 1 >= NF / 2) break; const float fr = bin - b0;
        const float db = spm[(size_t) b0] * (1 - fr) + spm[(size_t) b0 + 1] * fr;
        if (! st) { p.startNewSubPath (sp.getX() + px, yOf (db)); st = true; } else p.lineTo (sp.getX() + px, yOf (db));
    }
    if (st)
    {
        juce::Path f = p; f.lineTo (sp.getRight(), sp.getBottom()); f.lineTo (sp.getX(), sp.getBottom()); f.closeSubPath();
        juce::ColourGradient gg (mag.withAlpha (0.45f), 0, sp.getY(), mag.withAlpha (0.05f), 0, sp.getBottom(), false); g.setGradientFill (gg); g.fillPath (f);
        neonStroke (g, p, mag, 1.3f);
    }
    // storia: livello dell'808, colpi di cassa, ducking
    auto hb = juce::Rectangle<float> (16, 270, b.getWidth() - 32, 130);
    g.setColour (juce::Colour (0xff0b0816)); g.fillRect (hb);
    const int N = Processor::kHist, w0 = P.hW.load();
    for (int x = 0; x < N; ++x)
    {
        const float l = P.hLvl[(w0 + x) % N].load(), k = P.hKick[(w0 + x) % N].load(), d = P.hDuck[(w0 + x) % N].load();
        const float xx = hb.getX() + hb.getWidth() * x / N, ww = hb.getWidth() / N + 0.6f, h = hb.getHeight() * 0.5f * juce::jlimit (0.0f, 1.0f, l);
        g.setColour (cya.withAlpha (0.75f)); g.fillRect (xx, hb.getCentreY() - h, ww, h * 2);
        if (k > 0.6f) { g.setColour (yel); g.fillRect (xx, hb.getY(), ww, 6.0f); }
        if (d < 0.98f) { g.setColour (red.withAlpha (0.8f)); g.fillRect (xx, hb.getBottom() - 4 - (1 - d) * 30, ww, 2.0f); }
    }
    g.setFont (fMono (10)); g.setColour (dim);
    g.drawText (U ("808 IN USCITA  //  GIALLO = CASSA  //  ROSSO = DUCKING  //  ULTIMI 6 s"), juce::Rectangle<float> (hb.getX(), hb.getBottom() + 4, hb.getWidth(), 12), juce::Justification::centredLeft);
}

// ======================================================================= diagnosi
void Editor::diagnose()
{
    auto& P = proc; diags.clear();
    const bool sound = P.pitchN.load() > 15 || P.eSub.load() > -70;
    if (! sound) { diags.push_back ({ 0, "IN ASCOLTO", "Fai suonare l'808 per qualche battuta: la diagnosi si costruisce mentre ascolta." }); return; }
    const float f0 = P.f0Avg.load(), cents = P.centsAvg.load(); const int pn = P.pitchN.load();
    if (P.inPeak.load() > -0.1f) diags.push_back ({ 2, "INGRESSO TROPPO ALTO", "L'808 arriva a 0 dB prima del plugin: abbassa il volume del campione o del clip di 3-6 dB." });
    if (pn > 20 && std::abs (cents) > 15) diags.push_back ({ 2, juce::String ("808 ") + (cents < 0 ? "CALANTE" : "CRESCENTE") + " DI " + juce::String (juce::roundToInt (std::abs (cents))) + " CENT",
                                                         "Rispetto all'accordatura a 440 Hz. Nel campionatore metti Detune " + juce::String (-juce::roundToInt (cents)) + " cent, oppure riaccorda le melodie." });
    if (pn > 20 && f0 > 0 && f0 < 45) diags.push_back ({ 2, "FONDAMENTALE A " + juce::String (juce::roundToInt (f0)) + " Hz", "Su telefono, laptop e cuffiette sotto i 50-60 Hz non si sente niente: alza ARMONICHE (40-60%) cosi' si sente la nota anche li'." });
    const float bodyRel = P.eBody.load() - P.eSub.load();
    if (bodyRel < -22 && P.v ("harm") < 0.2f && P.v ("drive") < 0.2f) diags.push_back ({ 1, "SOLO SUB", "L'808 vive solo sotto i 90 Hz (corpo " + juce::String (juce::roundToInt (bodyRel)) + " dB). Dai un po' di DRIVE o ARMONICHE per farle avere una voce." });
    else if (bodyRel > -4) diags.push_back ({ 1, "TANTO CORPO", "Molto medio-basso (90-400 Hz): rischia di impastarsi con la voce e le melodie. Meno DRIVE, oppure usa Vocal Radar sul bus." });
    if (P.sideRatio.load() > -15) diags.push_back ({ 2, "BASSI LARGHI IN STEREO", "Sotto i 120 Hz c'e' tanto stereo (" + juce::String (juce::roundToInt (P.sideRatio.load())) + " dB): in club e su mono si perde. Attiva MONO SUB a 120 Hz." });
    if (P.crest.load() > 0 && P.crest.load() < 7 && P.v ("punch") < 0.3f) diags.push_back ({ 1, "ATTACCO PIATTO", "L'808 non ha un colpo all'inizio (crest " + juce::String (P.crest.load(), 1) + " dB): alza PUNCH per farla uscire sulle casse piccole." });
    if (P.clips.load() > 0) diags.push_back ({ 1, "CLIPPER AL LAVORO", "Il clipper ha tagliato " + juce::String (P.clips.load()) + " volte: se senti distorsione che non vuoi, abbassa DRIVE o alza il CEILING." });
    if (P.scLive.load() && P.kickHits.load() > 3)
    {
        if (P.kickCorr.load() < -0.2f) diags.push_back ({ 2, "CASSA E 808 IN CONTROFASE", "Nei bassi si cancellano a vicenda quando suonano insieme (" + juce::String (P.kickCorr.load(), 2) + "). Premi INVERTI FASE e senti se il colpo torna pieno." });
        if (P.overlapDb.load() > -6) diags.push_back ({ 1, "CODA SOPRA LA CASSA", "Quando batte la cassa l'808 e' ancora forte (" + juce::String (juce::roundToInt (P.overlapDb.load())) + " dB rispetto alla cassa). Alza DUCKING (30-50%)." });
        if (P.kickCorr.load() >= -0.2f && P.overlapDb.load() <= -6) diags.push_back ({ 0, "CASSA E 808 OK", "Fase a posto e la cassa ha spazio quando colpisce." });
    }
    else diags.push_back ({ 0, "SENZA CASSA", "Manda la cassa nel sidechain del plugin: controllo la fase, la sovrapposizione e attivo il ducking." });
    if (pn > 20 && std::abs (cents) <= 15) diags.push_back ({ 0, "INTONATA", "Scarto medio " + juce::String (juce::roundToInt (cents)) + " cent: va bene." });
    std::stable_sort (diags.begin(), diags.end(), [] (const Diag& a, const Diag& b) { return a.sev > b.sev; });
}
void DocView::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (14, 0); float y = 20;
    for (auto& d : ed.diags)
    {
        const auto c = d.sev == 2 ? red : d.sev == 1 ? yel : cya;
        g.setFont (fLab (12.5f)); const int lines = juce::jmax (1, (int) std::ceil (juce::GlyphArrangement::getStringWidth (fLab (11), d.text) / (b.getWidth() - 14)));
        const float h = 20.0f + lines * 14.0f + 8;
        if (y + h > (float) getHeight() - 6) break;
        g.setColour (c.withAlpha (0.10f)); g.fillPath (cut ({ b.getX(), y, b.getWidth(), h - 6 }, 6)); g.setColour (c); g.fillRect (b.getX(), y, 3.0f, h - 6);
        g.drawText ((d.sev == 2 ? "!! " : d.sev == 1 ? "> " : "OK ") + d.title, juce::Rectangle<float> (b.getX() + 10, y + 3, b.getWidth() - 14, 16), juce::Justification::centredLeft);
        g.setFont (fLab (11)); g.setColour (ink.withAlpha (0.85f));
        g.drawFittedText (d.text, juce::Rectangle<int> ((int) b.getX() + 10, (int) y + 20, (int) b.getWidth() - 14, lines * 14 + 2), juce::Justification::topLeft, lines + 1);
        y += h;
    }
}

// ======================================================================= editor
Editor::Editor (Processor& p) : AudioProcessorEditor (p), proc (p)
{
    addAndMakeVisible (tuner); addAndMakeVisible (spec); addAndMakeVisible (doc);
    for (int i = 0; i < SpecView::NF; ++i) spec.win[(size_t) i] = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * i / (SpecView::NF - 1));
    auto add = [&] (const char* id, const char* lab, juce::Colour c, std::function<juce::String (float)> f, const char* tip)
    { auto* k = knobs.add (new CyberKnob (proc.apvts.getParameter (id), lab, c, false)); k->fmt = std::move (f); k->setTooltip (juce::String::fromUTF8 (tip)); addAndMakeVisible (k); return k; };
    auto pct = [] (float v) { return v < 0.005f ? juce::String ("OFF") : juce::String (juce::roundToInt (v * 100)) + "%"; };
    add ("punch", "PUNCH", yel, pct, "Alza il colpo all'inizio di ogni nota: l'808 esce di piu' sulle casse piccole.");
    add ("sustain", "SUSTAIN", yel, [] (float v) { return std::abs (v) < 0.01f ? juce::String ("0") : (v > 0 ? "+" : "") + juce::String (juce::roundToInt (v * 100)) + "%"; }, "A destra allunga la coda, a sinistra la accorcia.");
    add ("drive", "DRIVE", mag, pct, "Satura il corpo e gli alti. Il sub sotto i 90 Hz resta pulito.");
    add ("harm", "ARMONICHE", mag, pct, "Aggiunge la seconda e la terza armonica esatte della nota: l'orecchio sente la nota anche dove il sub non arriva.");
    add ("duck", "DUCKING", cya, pct, "Abbassa l'808 quando batte la cassa (serve la cassa nel sidechain).");
    add ("rel", "RILASCIO", cya, [] (float v) { return juce::String (juce::roundToInt (v)) + " ms"; }, "Quanto ci mette l'808 a tornare su dopo la cassa.");
    add ("mono", "MONO SUB", pur, [] (float v) { static const char* n[5] = { "OFF", "60 Hz", "90 Hz", "120 Hz", "150 Hz" }; return juce::String (n[juce::jlimit (0, 4, juce::roundToInt (v))]); }, "Sotto questa frequenza lo stereo diventa mono.");
    add ("ceil", "CEILING", red, [] (float v) { return juce::String (v, 1) + " dB"; }, "Clipper morbido: oltre questo livello l'808 viene arrotondata.");
    add ("out", "OUTPUT", ink, [] (float v) { return (v > 0 ? "+" : "") + juce::String (v, 1) + " dB"; }, "Volume d'uscita.");
    addAndMakeVisible (sizeChip);
    scaler.setup();
    startTimerHz (30);
}
void Editor::resized()
{
    const float s = scaler.update();
    tuner.setBounds (24, 96, 372, 404); spec.setBounds (412, 96, 452, 412); doc.setBounds (880, 100, 276, 404);
    const int y = 560, w = 92;
    const int xs[9] = { 44, 136, 248, 470, 586, 678, 790, 902, 994 };
    for (int i = 0; i < knobs.size(); ++i) knobs[i]->setBounds (xs[i], y, w, 118);
    for (int m = 0; m < 4; ++m) modeR[m] = { 346.0f, 562.0f + m * 27, 76, 22 };
    invR = { 700, 22, 150, 32 }; resetR = { 856, 22, 120, 32 };
    prevR = { 316, 22, 28, 32 }; presetR = { 346, 22, 316, 32 }; nextR = { 664, 22, 28, 32 };
    sizeChip.setBounds (W - 76, H - 16, 58, 14);
    bg = {};
    jlook::scaleChildren (*this, s);
}
void Editor::paintStatic (juce::Graphics& g)
{
    const float Wf = (float) W, Hf = (float) H;
    juce::ColourGradient sky (bg1, 0, 0, bg0, 0, Hf, false); g.setGradientFill (sky); g.fillAll();
    juce::Random rnd (12);
    for (int k = 0; k < 40; ++k)
    {
        const float w = 20 + rnd.nextFloat() * 60, h = 60 + rnd.nextFloat() * 280, x = rnd.nextFloat() * Wf, y = 540 - h;
        g.setColour (juce::Colour (0xff0d0820).interpolatedWith (juce::Colour (0xff1d0a2e), rnd.nextFloat())); g.fillRect (x, y, w, h);
        for (float wy = y + 6; wy < 530; wy += 9) for (float wx = x + 3; wx < x + w - 4; wx += 7)
            if (rnd.nextFloat() < 0.08f) { g.setColour ((rnd.nextBool() ? mag : yel).withAlpha (0.2f + rnd.nextFloat() * 0.4f)); g.fillRect (wx, wy, 3.0f, 4.0f); }
    }
    juce::ColourGradient haze (yel.withAlpha (0.0f), 0, 380, mag.withAlpha (0.16f), 0, 540, false); g.setGradientFill (haze); g.fillRect (0.0f, 380.0f, Wf, 160.0f);
    g.setColour (juce::Colour (0xff07040d)); g.fillRect (0.0f, 540.0f, Wf, Hf - 540);
    for (int i = -20; i <= 20; ++i) { g.setColour (cya.withAlpha (0.18f)); g.drawLine (Wf / 2 + i * 18.0f, 540, Wf / 2 + i * 140.0f, Hf, 1.0f); }
    for (int j = 1; j < 14; ++j) { const float t = std::pow (j / 14.0f, 2.0f), y = 540 + (Hf - 540) * t; g.setColour (cya.withAlpha (0.08f + 0.18f * t)); g.drawLine (0, y, Wf, y, 1.0f); }
    cyberPanel (g, { 24, 84, 372, 428 }, yel, "TUNER//KEY");
    cyberPanel (g, { 412, 84, 452, 428 }, mag, "SPECTRUM//808");
    cyberPanel (g, { 880, 84, 288, 428 }, red, "DOCTOR//COSA MANCA");
    cyberPanel (g, { 24, 532, 1144, 176 }, cya, "CONTROL//DECK");
    auto grp = [&] (float x, float w, const char* t) { g.setColour (cya.withAlpha (0.45f)); g.fillRect (x, 548.0f, w, 1.0f); g.setFont (fMono (10)); g.setColour (dim); g.drawText (t, juce::Rectangle<float> (x, 536, w, 12), juce::Justification::centred); };
    grp (44, 184, "TRANSIENTI"); grp (248, 180, "SATURAZIONE (SUB PULITO)"); grp (470, 92, "TELEFONI"); grp (586, 184, "CASSA IN SIDECHAIN"); grp (790, 92, "STEREO"); grp (902, 184, "USCITA");
    for (float y = 0; y < Hf; y += 3) { g.setColour (juce::Colours::black.withAlpha (0.10f)); g.fillRect (0.0f, y, Wf, 1.0f); }
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
    // logo
    {
        const float gx = glitchOff;
        g.setFont (fTitle (40));
        g.setColour (cya.withAlpha (0.8f)); g.drawText ("808", juce::Rectangle<float> (22 - 3 + gx, 8, 120, 44), juce::Justification::centredLeft);
        g.setColour (red.withAlpha (0.8f)); g.drawText ("808", juce::Rectangle<float> (22 + 3 - gx, 10, 120, 44), juce::Justification::centredLeft);
        g.setColour (yel); g.drawText ("808", juce::Rectangle<float> (22, 9, 120, 44), juce::Justification::centredLeft);
        g.setFont (fTitle (30)); g.setColour (ink); g.drawText ("STUDIO", juce::Rectangle<float> (118 + gx * 0.4f, 13, 200, 40), juce::Justification::centredLeft);
        g.setFont (fMono (11)); g.setColour (dim); g.drawText ("JACKPOT // 808 CONTROL CENTER // TRAP DRILL HIP HOP", juce::Rectangle<float> (24, 52, 340, 14), juce::Justification::centredLeft);
    }
    g.setColour (juce::Colour (0xcc0b0918)); g.fillPath (cut (presetR, 8)); g.setColour (pur.withAlpha (0.7f)); g.strokePath (cut (presetR, 8), juce::PathStrokeType (1.0f));
    g.setFont (fMono (9)); g.setColour (dim); g.drawText ("PRESET", presetR.withHeight (12).translated (0, 1), juce::Justification::centred);
    g.setFont (fLab (13)); g.setColour (ink); g.drawFittedText (proc.presetName, presetR.withTrimmedTop (10).toNearestInt(), juce::Justification::centred, 1);
    g.setFont (fLab (16)); g.setColour (pur); g.drawText ("<", prevR, juce::Justification::centred); g.drawText (">", nextR, juce::Justification::centred);
    chipBtn (g, invR, "INVERTI FASE", proc.v ("inv") > 0.5f, red, 11.5f);
    chipBtn (g, resetR, "NUOVA ANALISI", false, cya, 11.5f);
    // stato cassa
    {
        const bool k = proc.scLive.load(); const auto c = k ? cya : dim;
        g.setColour (c); g.fillEllipse (990, 33, 9, 9);
        g.setFont (fMono (11)); g.setColour (c); g.drawFittedText (k ? "CASSA NEL SIDECHAIN" : "NESSUNA CASSA IN SIDECHAIN", juce::Rectangle<int> (1004, 22, 170, 32), juce::Justification::centredLeft, 2);
    }
    for (int m = 0; m < 4; ++m) chipBtn (g, modeR[m], modeName (m), juce::roundToInt (proc.v ("mode")) == m, mag, 10.5f);
    // clip
    {
        const float gr = proc.uiGrClip.load(); auto r = juce::Rectangle<float> (1100, 572, 50, 90);
        g.setColour (juce::Colour (0xff0b0816)); g.fillRect (r);
        g.setColour (red); g.fillRect (r.withTrimmedTop (r.getHeight() * (1 - juce::jlimit (0.0f, 1.0f, gr / 6.0f))));
        g.setFont (fMono (10)); g.setColour (gr > 0.05f ? red : dim); g.drawText ("CLIP", r.withY (r.getBottom() + 4).withHeight (12), juce::Justification::centred);
        g.drawText ("-" + juce::String (gr, 1), r.withY (r.getBottom() + 18).withHeight (12), juce::Justification::centred);
    }
}
void Editor::mouseDown (const juce::MouseEvent& e)
{
    const auto p = e.position / scaler.s;
    for (int m = 0; m < 4; ++m) if (modeR[m].contains (p)) { proc.setV ("mode", (float) m); repaint(); return; }
    if (invR.contains (p)) { proc.setV ("inv", proc.v ("inv") > 0.5f ? 0.0f : 1.0f); repaint(); return; }
    if (resetR.contains (p)) { proc.resetStats(); repaint(); return; }
    if (prevR.contains (p) || nextR.contains (p))
    {
        const int n = (int) presets().size(); int cur = 0; for (int i = 0; i < n; ++i) if (proc.presetName == presets()[(size_t) i].name) cur = i;
        proc.loadPreset ((cur + (nextR.contains (p) ? 1 : n - 1)) % n); repaint(); return;
    }
    if (presetR.contains (p)) presetMenu();
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
    const bool on = proc.uiConf.load() > 0.6f && proc.uiLevel.load() > -50;
    if (on)
    {
        const float hz = proc.uiHz.load(); tuner.hzS = tuner.hzS < 20 ? hz : tuner.hzS + 0.4f * (hz - tuner.hzS);
        const double mn = 69.0 + 12.0 * std::log2 (tuner.hzS / 440.0); tuner.centS += 0.4f * ((float) ((mn - std::round (mn)) * 100.0) - tuner.centS);
    }
    spec.compute();
    if (glitchT > 0) { --glitchT; glitchOff = (float) juce::Random::getSystemRandom().nextInt (9) - 4.0f; } else { glitchOff = 0; if (juce::Random::getSystemRandom().nextInt (90) == 0) glitchT = 6; }
    if (frame % 10 == 0) { diagnose(); doc.repaint(); }
    tuner.repaint(); spec.repaint();
    repaint (scaler.map (juce::Rectangle<int> (0, 0, 380, 70)));
    repaint (scaler.map (juce::Rectangle<int> (984, 18, 190, 40)));
    repaint (scaler.map (juce::Rectangle<int> (1096, 566, 60, 130)));
}
}
