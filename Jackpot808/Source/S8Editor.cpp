// JACKPOT 808 STUDIO / 808 SYNTH: stessa notte al neon di Vocal Radar.
#include "S8Editor.h"

namespace s8
{
static juce::String U (const char* s) { return juce::String::fromUTF8 (s); }

// ======================================================================= accordatore e nota piu' usata
void TunerView::paint (juce::Graphics& g)
{
    auto& P = ed.proc.ch; auto b = getLocalBounds().toFloat();
    const bool on = P.uiConf.load() > 0.6f && P.uiLevel.load() > -50 && hzS > 20;
    juce::String nm = "--", oct; float cents = 0;
    if (hzS > 20)
    {
        const double mn = 69.0 + 12.0 * std::log2 (hzS / 440.0); const int n = (int) std::round (mn);
        nm = noteName (n); oct = juce::String (n / 12 - 1); cents = (float) ((mn - n) * 100.0);
    }
    // nota grande con glitch
    g.setFont (jlook::logo (96));
    const auto col = ! on ? C::dim : std::abs (centS) < 8 ? C::phos : std::abs (centS) < 20 ? C::yellow : C::red;
    juce::Rectangle<float> nr (b.getX(), 10, b.getWidth(), 110);
    if (on) { g.setColour (C::magenta.withAlpha (0.5f)); g.drawText (nm, nr.translated (-3, 0), juce::Justification::centred); }
    g.setColour (col); g.drawText (nm, nr, juce::Justification::centred);
    g.setFont (jlook::logo (30)); g.setColour (col.withAlpha (0.8f)); g.drawText (oct, juce::Rectangle<float> (b.getCentreX() + 62, 64, 40, 40), juce::Justification::centredLeft);
    g.setFont (s8mono (14)); g.setColour (C::text.withAlpha (0.8f));
    g.drawText (on ? juce::String (hzS, 1) + " Hz   " + (centS >= 0 ? "+" : "") + juce::String (juce::roundToInt (centS)) + " cent" : juce::String ("in attesa dell'808..."), juce::Rectangle<float> (0, 122, b.getWidth(), 18), juce::Justification::centred);
    // barra dei cent
    auto cb = juce::Rectangle<float> (30, 150, b.getWidth() - 60, 16);
    g.setColour (juce::Colour (0xff120e22)); g.fillRect (cb);
    for (int k = -5; k <= 5; ++k) { const float x = cb.getCentreX() + cb.getWidth() * 0.5f * k / 5; g.setColour ((k == 0 ? C::phos : C::dim).withAlpha (k == 0 ? 0.9f : 0.4f)); g.fillRect (x - 0.5f, cb.getY() - (k == 0 ? 4 : 0), 1.0f, cb.getHeight() + (k == 0 ? 8 : 0)); }
    if (on) { const float x = cb.getCentreX() + cb.getWidth() * 0.5f * juce::jlimit (-1.0f, 1.0f, centS / 50.0f); g.setColour (col.withAlpha (0.3f)); g.fillRect (x - 6, cb.getY() - 2, 12.0f, cb.getHeight() + 4); g.setColour (col); g.fillRect (x - 2, cb.getY() - 2, 4.0f, cb.getHeight() + 4); }
    g.setFont (s8mono (10)); g.setColour (C::dim); g.drawText ("-50", cb.withY (cb.getBottom() + 2).withHeight (12), juce::Justification::centredLeft); g.drawText ("+50", cb.withY (cb.getBottom() + 2).withHeight (12), juce::Justification::centredRight);
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
        const auto c = k == top ? C::yellow : C::magenta;
        if (t > 0) { auto f = r.withTrimmedTop (r.getHeight() * (1 - t)); g.setColour (c.withAlpha (0.25f)); g.fillRect (f); g.setColour (c); g.fillRect (f.withHeight (3.0f)); }
        g.setFont (s8lab (11)); g.setColour (k == top ? C::yellow : black ? C::dim : C::text.withAlpha (0.8f)); g.drawText (noteName (k), juce::Rectangle<float> (r.getX(), kb.getBottom() - 18, r.getWidth(), 14), juce::Justification::centred);
    }
    g.setFont (s8lab (12)); g.setColour (top >= 0 ? C::yellow : C::dim);
    g.drawText (top >= 0 ? "NOTA PIU' USATA: " + juce::String (noteName (top)) + "  //  PROBABILE TONICA" : juce::String ("NOTE: suona qualche battuta"), juce::Rectangle<float> (0, 350, b.getWidth(), 16), juce::Justification::centred);
    if (P.pitchN.load() > 20)
    {
        g.setFont (s8mono (11)); g.setColour (C::text.withAlpha (0.75f));
        g.drawText ("accordatura media " + juce::String (P.centsAvg.load() >= 0 ? "+" : "") + juce::String (juce::roundToInt (P.centsAvg.load())) + " cent  //  fondamentale media " + juce::String (juce::roundToInt (P.f0Avg.load())) + " Hz",
                    juce::Rectangle<float> (0, 372, b.getWidth(), 14), juce::Justification::centred);
    }
}

// ======================================================================= spettro e storia
void SpecView::compute()
{
    auto& P = ed.proc.ch; const int w0 = P.ringW.load();
    for (int i = 0; i < NF; ++i) buf[(size_t) i] = P.ring[(w0 - NF + i) & (Chain::kRing - 1)] * win[(size_t) i];
    std::fill (buf.begin() + NF, buf.end(), 0.0f);
    fft.performFrequencyOnlyForwardTransform (buf.data());
    for (int i = 0; i < NF / 2; ++i) { const float db = juce::Decibels::gainToDecibels (buf[(size_t) i] / (NF * 0.25f) + 1e-9f); spm[(size_t) i] += (db > spm[(size_t) i] ? 0.6f : 0.15f) * (db - spm[(size_t) i]); }
}
void SpecView::paint (juce::Graphics& g)
{
    auto& P = ed.proc.ch; auto b = getLocalBounds().toFloat();
    const bool hist = b.getHeight() > 300;
    auto sp = juce::Rectangle<float> (16, 26, b.getWidth() - 32, hist ? 210.0f : b.getHeight() - 46);
    const double sr = P.sampleRate();
    auto xOf = [&] (float hz) { return sp.getX() + sp.getWidth() * std::log (hz / 20.0f) / std::log (100.0f); };   // 20 Hz .. 2 kHz
    auto yOf = [&] (float db) { return sp.getBottom() - sp.getHeight() * juce::jlimit (0.0f, 1.0f, (db + 84.0f) / 78.0f); };
    // zone
    struct Z { float a, b; juce::Colour c; const char* t; } zones[3] = { { 20, 90, C::phos, "SUB" }, { 90, 400, C::magenta, "CORPO" }, { 400, 2000, C::yellow, "ARMONICHE / GRINTA" } };
    for (auto& z : zones) { g.setColour (z.c.withAlpha (0.06f)); g.fillRect (juce::Rectangle<float> (xOf (z.a), sp.getY(), xOf (z.b) - xOf (z.a), sp.getHeight())); g.setFont (s8mono (10)); g.setColour (z.c.withAlpha (0.8f)); g.drawText (z.t, juce::Rectangle<float> (xOf (z.a) + 4, sp.getY() + 2, 160, 12), juce::Justification::centredLeft); }
    for (float hz : { 30.f, 50.f, 100.f, 200.f, 500.f, 1000.f }) { g.setColour (C::phos.withAlpha (0.08f)); g.fillRect (xOf (hz), sp.getY(), 1.0f, sp.getHeight()); g.setFont (s8mono (9.5f)); g.setColour (C::dim); g.drawText (hz >= 1000 ? "1k" : juce::String ((int) hz), juce::Rectangle<float> (xOf (hz) - 15, sp.getBottom() + 2, 30, 11), juce::Justification::centred); }
    // nota rilevata: righe della fondamentale e delle armoniche
    const float f0 = P.uiHz.load();
    if (P.uiConf.load() > 0.6f && f0 > 20) for (int k = 1; k <= 6; ++k) { const float x = xOf (f0 * k); if (x > sp.getRight()) break; g.setColour ((k == 1 ? C::phos : C::yellow).withAlpha (k == 1 ? 0.6f : 0.3f)); g.fillRect (x - 0.5f, sp.getY(), 1.0f, sp.getHeight()); }
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
        juce::ColourGradient gg (C::magenta.withAlpha (0.45f), 0, sp.getY(), C::magenta.withAlpha (0.05f), 0, sp.getBottom(), false); g.setGradientFill (gg); g.fillPath (f);
        neonStroke (g, p, C::magenta, 1.3f);
    }
    if (! hist) return;
    // storia: livello dell'808, colpi di cassa, ducking
    auto hb = juce::Rectangle<float> (16, 270, b.getWidth() - 32, 130);
    g.setColour (juce::Colour (0xff0b0816)); g.fillRect (hb);
    const int N = Chain::kHist, w0 = P.hW.load();
    for (int x = 0; x < N; ++x)
    {
        const float l = P.hLvl[(w0 + x) % N].load(), k = P.hKick[(w0 + x) % N].load(), d = P.hDuck[(w0 + x) % N].load();
        const float xx = hb.getX() + hb.getWidth() * x / N, ww = hb.getWidth() / N + 0.6f, h = hb.getHeight() * 0.5f * juce::jlimit (0.0f, 1.0f, l);
        g.setColour (C::phos.withAlpha (0.75f)); g.fillRect (xx, hb.getCentreY() - h, ww, h * 2);
        if (k > 0.6f) { g.setColour (C::yellow); g.fillRect (xx, hb.getY(), ww, 6.0f); }
        if (d < 0.98f) { g.setColour (C::red.withAlpha (0.8f)); g.fillRect (xx, hb.getBottom() - 4 - (1 - d) * 30, ww, 2.0f); }
    }
    g.setFont (s8mono (10)); g.setColour (C::dim);
    g.drawText (U ("808 IN USCITA  //  GIALLO = CASSA  //  ROSSO = DUCKING  //  ULTIMI 6 s"), juce::Rectangle<float> (hb.getX(), hb.getBottom() + 4, hb.getWidth(), 12), juce::Justification::centredLeft);
}

// ======================================================================= diagnosi
void Editor::diagnose()
{
    auto& P = proc.ch; diags.clear();
    const bool sound = P.pitchN.load() > 15 || P.eSub.load() > -70;
    if (! sound) { diags.push_back ({ 0, "IN ASCOLTO", "Fai suonare l'808 per qualche battuta: la diagnosi si costruisce mentre ascolta." }); return; }
    const float f0 = P.f0Avg.load(), cents = P.centsAvg.load(); const int pn = P.pitchN.load();
    if (P.inPeak.load() > -0.1f) diags.push_back ({ 2, "INGRESSO TROPPO ALTO", "L'808 arriva a 0 dB prima del plugin: abbassa il volume del campione o del clip di 3-6 dB." });
    if (pn > 20 && std::abs (cents) > 15) diags.push_back ({ 2, juce::String ("808 ") + (cents < 0 ? "CALANTE" : "CRESCENTE") + " DI " + juce::String (juce::roundToInt (std::abs (cents))) + " CENT",
                                                         "Rispetto all'accordatura a 440 Hz. Nel campionatore metti Detune " + juce::String (-juce::roundToInt (cents)) + " cent, oppure riaccorda le melodie." });
    if (pn > 20 && f0 > 0 && f0 < 45) diags.push_back ({ 2, "FONDAMENTALE A " + juce::String (juce::roundToInt (f0)) + " Hz", "Su telefono, laptop e cuffiette sotto i 50-60 Hz non si sente niente: alza ARMONICHE (40-60%) cosi' si sente la nota anche li'." });
    const float bodyRel = P.eBody.load() - P.eSub.load();
    if (bodyRel < -22 && proc.v ("harm") < 0.2f && proc.v ("drive") < 0.2f && proc.v ("subPlus") < 0.05f) diags.push_back ({ 1, "SOLO SUB", "L'808 vive solo sotto i 90 Hz (corpo " + juce::String (juce::roundToInt (bodyRel)) + " dB). Dai un po' di DRIVE o ARMONICHE per farle avere una voce." });
    else if (bodyRel > -4) diags.push_back ({ 1, "TANTO CORPO", "Molto medio-basso (90-400 Hz): rischia di impastarsi con la voce e le melodie. Meno DRIVE, oppure usa Vocal Radar sul bus." });
    if (P.sideRatio.load() > -15) diags.push_back ({ 2, "BASSI LARGHI IN STEREO", "Sotto i 120 Hz c'e' tanto stereo (" + juce::String (juce::roundToInt (P.sideRatio.load())) + " dB): in club e su mono si perde. Attiva MONO SUB a 120 Hz." });
    if (P.crest.load() > 0 && P.crest.load() < 7 && proc.v ("punch") < 0.3f) diags.push_back ({ 1, "ATTACCO PIATTO", "L'808 non ha un colpo all'inizio (crest " + juce::String (P.crest.load(), 1) + " dB): alza PUNCH per farla uscire sulle casse piccole." });
    if (P.eSub.load() < -45 && P.eBody.load() > P.eSub.load() + 3 && proc.v ("subPlus") < 0.2f) diags.push_back ({ 1, "SUB DEBOLE", "L'808 ha poco sotto i 90 Hz: alza SUB+ (40-70%), aggiunge una sinusoide pulita agganciata alla nota." });
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
        const auto c = d.sev == 2 ? C::red : d.sev == 1 ? C::yellow : C::phos;
        g.setFont (s8lab (12.5f)); const int lines = juce::jmax (1, (int) std::ceil (juce::GlyphArrangement::getStringWidth (s8lab (11), d.text) / (b.getWidth() - 14)));
        const float h = 20.0f + lines * 14.0f + 8;
        if (y + h > (float) getHeight() - 6) break;
        g.setColour (c.withAlpha (0.10f)); g.fillPath (cut ({ b.getX(), y, b.getWidth(), h - 6 }, 6)); g.setColour (c); g.fillRect (b.getX(), y, 3.0f, h - 6);
        g.drawText ((d.sev == 2 ? "!! " : d.sev == 1 ? "> " : "OK ") + d.title, juce::Rectangle<float> (b.getX() + 10, y + 3, b.getWidth() - 14, 16), juce::Justification::centredLeft);
        g.setFont (s8lab (11)); g.setColour (C::text.withAlpha (0.85f));
        g.drawFittedText (d.text, juce::Rectangle<int> ((int) b.getX() + 10, (int) y + 20, (int) b.getWidth() - 14, lines * 14 + 2), juce::Justification::topLeft, lines + 1);
        y += h;
    }
}

// ======================================================================= SYNTH: sample
void SampleView::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat(); auto& P = ed.proc;
    auto s = P.sampleRef; auto wr = juce::Rectangle<float> (0, 18, b.getWidth(), b.getHeight() - 52);
    g.setColour (juce::Colour (0xff0b0816)); g.fillPath (cut (wr, 8));
    g.setColour ((ed.dragOver ? C::yellow : C::phos).withAlpha (ed.dragOver ? 0.9f : 0.35f)); g.strokePath (cut (wr, 8), juce::PathStrokeType (ed.dragOver ? 2.0f : 1.0f));
    g.setFont (s8mono (10)); g.setColour (C::dim); g.drawText ("SAMPLE 808 (TRASCINA QUI WAV / AIFF / FLAC)", juce::Rectangle<float> (0, 0, b.getWidth(), 14), juce::Justification::centredLeft);
    if (s != nullptr)
    {
        if (waveOf != s.get())
        {   // forma d'onda calcolata una volta per sample
            waveOf = s.get(); wave.clear(); const auto& B = s->buf; const int n = B.getNumSamples(), W = (int) wr.getWidth() - 8;
            for (int x = 0; x < W; ++x) { const int a = (int) ((juce::int64) n * x / W), e = juce::jmax (a + 1, (int) ((juce::int64) n * (x + 1) / W)); float mx = 0; for (int i = a; i < e; i += 4) mx = juce::jmax (mx, std::abs (B.getSample (0, i))); const float h = mx * (wr.getHeight() * 0.45f); wave.addRectangle (wr.getX() + 4 + x, wr.getCentreY() - h, 1.0f, h * 2 + 0.5f); }
        }
        g.setColour (C::phos.withAlpha (0.75f)); g.fillPath (wave);
        const float st = P.v ("s_sstart"); const float xs = wr.getX() + 4 + (wr.getWidth() - 8) * st;
        g.setColour (C::yellow); g.fillRect (xs, wr.getY() + 2, 2.0f, wr.getHeight() - 4);
        g.setFont (s8lab (12)); g.setColour (C::text); g.drawFittedText (s->name, juce::Rectangle<int> (6, (int) wr.getY() + 4, (int) wr.getWidth() - 12, 16), juce::Justification::centredLeft, 1);
        const int rn = juce::roundToInt (s->root); const float rc = (s->root - rn) * 100;
        g.setFont (s8lab (12)); g.setColour (C::yellow);
        g.drawText ("NOTA DEL SAMPLE: " + juce::String (noteName (rn)) + juce::String (rn / 12 - 1) + (std::abs (rc) > 3 ? "  " + juce::String (rc >= 0 ? "+" : "") + juce::String (juce::roundToInt (rc)) + " c" : juce::String()), juce::Rectangle<float> (0, b.getBottom() - 30, b.getWidth() - 130, 26), juce::Justification::centredLeft);
        chipBtn (g, rootDn, "-", false, C::yellow, 12); chipBtn (g, rootUp, "+", false, C::yellow, 12);
    }
    else
    {
        g.setFont (s8lab (13)); g.setColour (C::dim.brighter (0.3f)); g.drawText (msg.isNotEmpty() ? msg : juce::String ("NESSUN SAMPLE: SUONA SOLO LA SINTESI"), wr, juce::Justification::centred);
        if (msg.isNotEmpty()) { g.setColour (C::red); g.drawText (msg, wr, juce::Justification::centred); }
    }
    chipBtn (g, loadR, "CARICA", false, C::phos, 11); chipBtn (g, clearR, "TOGLI", false, C::red, 11);
}
void SampleView::mouseDown (const juce::MouseEvent& e)
{
    auto& P = ed.proc;
    if (loadR.contains (e.position)) { ed.openSample(); return; }
    if (clearR.contains (e.position)) { P.clearSample(); waveOf = nullptr; repaint(); return; }
    if (P.sampleRef != nullptr && rootDn.contains (e.position)) { P.setSampleRoot (std::round (P.sampleRef->root) - 1); repaint(); return; }
    if (P.sampleRef != nullptr && rootUp.contains (e.position)) { P.setSampleRoot (std::round (P.sampleRef->root) + 1); repaint(); return; }
}

// ======================================================================= SYNTH: inviluppi d'ampiezza e di pitch
void EnvView::paint (juce::Graphics& g)
{
    auto& P = ed.proc; auto b = getLocalBounds().toFloat(); auto r = b.reduced (4, 20).withTrimmedBottom (4);
    g.setColour (juce::Colour (0xff0b0816)); g.fillRect (r);
    const float A = P.v ("s_att") * 0.001f, D = P.v ("s_dec"), S = P.v ("s_sus"), Rl = P.v ("s_rel") * 0.001f, pA = P.v ("s_pamt"), pT = P.v ("s_ptime") * 0.001f;
    const float hold = juce::jmax (0.4f, juce::jmin (D * 0.8f, 3.0f)), T = A + hold + Rl + 0.05f;
    auto xOf = [&] (float t) { return r.getX() + r.getWidth() * t / T; };
    for (float t = 0.5f; t < T; t += 0.5f) { g.setColour (C::dim.withAlpha (0.15f)); g.fillRect (xOf (t), r.getY(), 1.0f, r.getHeight()); }
    juce::Path pa, pp;
    for (int px = 0; px <= (int) r.getWidth(); px += 2)
    {
        const float t = T * px / r.getWidth(); float a;
        if (t < A) a = t / juce::jmax (1e-4f, A);
        else if (t < A + hold) a = S + (1 - S) * std::exp (-(t - A) * 6.9f / D);
        else { const float a0 = S + (1 - S) * std::exp (-hold * 6.9f / D); a = a0 * std::exp (-(t - A - hold) * 6.9f / juce::jmax (0.01f, Rl)); }
        const float y = r.getBottom() - r.getHeight() * a * 0.92f, yp = r.getBottom() - r.getHeight() * juce::jlimit (0.0f, 1.0f, pA * std::exp (-t / juce::jmax (0.005f, pT)) / 24.0f) * 0.92f;
        if (px == 0) { pa.startNewSubPath (r.getX(), y); pp.startNewSubPath (r.getX(), yp); } else { pa.lineTo (r.getX() + px, y); pp.lineTo (r.getX() + px, yp); }
    }
    juce::Path f = pa; f.lineTo (r.getRight(), r.getBottom()); f.lineTo (r.getX(), r.getBottom()); f.closeSubPath();
    g.setColour (C::phos.withAlpha (0.12f)); g.fillPath (f); neonStroke (g, pa, C::phos, 1.3f);
    if (pA > 0.05f) neonStroke (g, pp, C::yellow, 1.0f);
    g.setColour (C::magenta.withAlpha (0.5f)); g.fillRect (xOf (A + hold), r.getY(), 1.0f, r.getHeight());
    g.setFont (s8mono (10)); g.setColour (C::dim);
    g.drawText ("AZZURRO = VOLUME   GIALLO = PITCH   ROSA = RILASCI IL TASTO", juce::Rectangle<float> (b.getX() + 4, 2, b.getWidth(), 14), juce::Justification::centredLeft);
    const int n = P.syn.uiNote.load();
    g.setFont (s8lab (12)); g.setColour (n >= 0 ? C::yellow : C::dim);
    g.drawText (n >= 0 ? juce::String (noteName (n)) + juce::String (n / 12 - 1) + "   VOCI " + juce::String (P.syn.uiVoices.load()) : juce::String ("NESSUNA NOTA"), juce::Rectangle<float> (b.getRight() - 160, 2, 156, 14), juce::Justification::centredRight);
    const float e = P.syn.uiEnv.load(); g.setColour (C::phos); g.fillRect (r.getRight() - 6, r.getBottom() - r.getHeight() * juce::jlimit (0.0f, 1.0f, e), 4.0f, r.getHeight() * juce::jlimit (0.0f, 1.0f, e));
}

// ======================================================================= SYNTH: tastiera a schermo (3 ottave)
int KeysView::keyAt (juce::Point<float> p) const
{
    auto b = getLocalBounds().toFloat(); const int nW = kN / 12 * 7; const float ww = b.getWidth() / nW;
    static const int wSemi[7] = { 0, 2, 4, 5, 7, 9, 11 }; static const int bAfter[7] = { 1, 1, 0, 1, 1, 1, 0 };
    if (p.y < b.getHeight() * 0.6f)
        for (int w = 0; w < nW; ++w) if (bAfter[w % 7]) { const float x = (w + 1) * ww - ww * 0.3f; if (p.x >= x && p.x < x + ww * 0.6f) return kLo + (w / 7) * 12 + wSemi[w % 7] + 1; }
    const int w = juce::jlimit (0, nW - 1, (int) (p.x / ww)); return kLo + (w / 7) * 12 + wSemi[w % 7];
}
void KeysView::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat(); const int nW = kN / 12 * 7; const float ww = b.getWidth() / nW;
    static const int wSemi[7] = { 0, 2, 4, 5, 7, 9, 11 }; static const int bAfter[7] = { 1, 1, 0, 1, 1, 1, 0 };
    const int on = ed.proc.syn.uiNote.load();
    for (int w = 0; w < nW; ++w)
    {
        const int n = kLo + (w / 7) * 12 + wSemi[w % 7]; auto r = juce::Rectangle<float> (w * ww + 1, 0, ww - 2, b.getHeight());
        g.setColour (n == on ? C::phos.withAlpha (0.6f) : juce::Colour (0xff1b1630)); g.fillRect (r);
        g.setColour (C::phos.withAlpha (0.25f)); g.drawRect (r, 1.0f);
        if (wSemi[w % 7] == 0) { g.setFont (s8mono (9)); g.setColour (C::dim); g.drawText ("C" + juce::String (n / 12 - 1), r.withTop (r.getBottom() - 14), juce::Justification::centred); }
    }
    for (int w = 0; w < nW; ++w) if (bAfter[w % 7])
    {
        const int n = kLo + (w / 7) * 12 + wSemi[w % 7] + 1; auto r = juce::Rectangle<float> ((w + 1) * ww - ww * 0.3f, 0, ww * 0.6f, b.getHeight() * 0.6f);
        g.setColour (n == on ? C::magenta : juce::Colour (0xff07050d)); g.fillRect (r); g.setColour (C::magenta.withAlpha (0.4f)); g.drawRect (r, 1.0f);
    }
}
void KeysView::mouseDown (const juce::MouseEvent& e) { down = keyAt (e.position); ed.proc.syn.uiOn = down; }
void KeysView::mouseDrag (const juce::MouseEvent& e) { const int k = keyAt (e.position); if (k != down) { ed.proc.syn.uiOff = down; down = k; ed.proc.syn.uiOn = k; } }
void KeysView::mouseUp (const juce::MouseEvent&) { if (down >= 0) ed.proc.syn.uiOff = down; down = -1; }

// ======================================================================= editor
Editor::Editor (Processor& p) : AudioProcessorEditor (p), proc (p)
{
    // temi della serie (CYBER = la notte al neon dell'808, ROSSO, RETRO, CBRPNK, HUD, SCURO, CHIARO); colori delle schede e accento HUD dell'808
    skin::init (kSynth ? "808synth" : "808studio", { 0xfff4e04d, 0xffff8fb8, 0xff8fd8e8, 0xffc9b6ff }, 0xffff2e88); C::sync();
    undoMark.p = &p; addMouseListener (&undoMark, true); setWantsKeyboardFocus (true);
    addAndMakeVisible (spec);
    if (kSynth) { addAndMakeVisible (smp); addAndMakeVisible (env); addAndMakeVisible (keys); }
    else { addAndMakeVisible (tuner); addAndMakeVisible (doc); }
    for (int i = 0; i < SpecView::NF; ++i) spec.win[(size_t) i] = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * i / (SpecView::NF - 1));
    auto add = [&] (const char* id, const char* lab, juce::Colour c, std::function<juce::String (float)> f, const char* tip)
    {
        auto* prm = proc.apvts.getParameter (id);
        auto* k = knobs.add (new jkit::Knob (prm, c, lab)); k->text = [prm, f] { return f (prm->convertFrom0to1 (prm->getValue())); };
        k->setName (id); k->setHelpText (juce::String::fromUTF8 (tip)); addAndMakeVisible (k); return k;
    };
    auto pct = [] (float v) { return v < 0.005f ? juce::String ("OFF") : juce::String (juce::roundToInt (v * 100)) + "%"; };
    auto db = [] (float v) { return std::abs (v) < 0.05f ? juce::String ("0 dB") : (v > 0 ? "+" : "") + juce::String (v, 1) + " dB"; };
    auto hz = [] (float v) { return v >= 1000 ? juce::String (v / 1000, v >= 10000 ? 0 : 1) + "k" : juce::String (juce::roundToInt (v)) + " Hz"; };
    auto ms = [] (float v) { return v >= 1000 ? juce::String (v / 1000, 2) + " s" : juce::String (juce::roundToInt (v)) + " ms"; };
    // catena (tutti e due)
    add ("punch", "PUNCH", C::yellow, pct, "Alza il colpo all'inizio di ogni nota: l'808 esce di piu' sulle casse piccole.");
    add ("sustain", "SUSTAIN", C::yellow, [] (float v) { return std::abs (v) < 0.01f ? juce::String ("0") : (v > 0 ? "+" : "") + juce::String (juce::roundToInt (v * 100)) + "%"; }, "A destra allunga la coda, a sinistra la accorcia.");
    if (! kSynth) add ("subPlus", "SUB+", C::phos, pct, "Aggiunge sotto una sinusoide pulita agganciata alla nota e al volume dell'808: rinforza le 808 sottili.");
    add ("subDb", "SUB", C::phos, db, "Scaffale sotto i 70 Hz: piu' o meno sub.");
    add ("knock", "KNOCK", C::magenta, [] (float v) { return v < 0.05f ? juce::String ("OFF") : "+" + juce::String (v, 1) + " dB"; }, "Il colpo del corpo: una campana sulla frequenza a destra.");
    add ("knockHz", "FREQ", C::magenta, hz, "Dove colpisce il KNOCK (100-250 Hz e' il corpo dell'808).");
    add ("topHz", "TOP", C::magenta, [hz] (float v) { return v > 19500 ? juce::String ("APERTO") : hz (v); }, "Taglia gli alti: 808 piu' scura e morbida.");
    add ("drive", "DRIVE", C::magenta, pct, "Satura il corpo e gli alti. Il sub sotto i 90 Hz resta pulito.");
    add ("harm", "ARMONICHE", C::magenta, pct, "Aggiunge la seconda e la terza armonica esatte della nota: l'orecchio sente la nota anche dove il sub non arriva.");
    add ("duck", "DUCKING", C::phos, pct, "Abbassa l'808 quando batte la cassa (serve la cassa nel sidechain).");
    add ("rel", "RILASCIO", C::phos, ms, "Quanto ci mette l'808 a tornare su dopo la cassa.");
    add ("mono", "MONO SUB", C::violet, [] (float v) { static const char* n[5] = { "OFF", "60 Hz", "90 Hz", "120 Hz", "150 Hz" }; return juce::String (n[juce::jlimit (0, 4, juce::roundToInt (v))]); }, "Sotto questa frequenza lo stereo diventa mono.");
    add ("ceil", "CEILING", C::red, [] (float v) { return juce::String (v, 1) + " dB"; }, "Clipper morbido: oltre questo livello l'808 viene arrotondata.");
    add ("out", "OUTPUT", C::text, db, "Volume d'uscita.");
    if (kSynth)
    {
        add ("s_tb", "TIMBRO", C::phos, pct, "Da sinusoide pura verso un'onda quasi quadra: piu' presenza e armoniche.");
        add ("s_reese", "REESE", C::phos, pct, "Due denti di sega leggermente stonati sopra: l'808 si muove e si allarga.");
        add ("s_sub", "SUB OTT.", C::phos, pct, "Una sinusoide un'ottava sotto.");
        add ("s_click", "CLICK", C::yellow, pct, "Il colpetto d'attacco: si sente sulle casse piccole.");
        add ("s_dist", "DIST", C::red, pct, "Distorsione su tutta l'808, sub compreso (la rotella del controller ne aggiunge).");
        add ("s_pamt", "PITCH", C::yellow, [] (float v) { return v < 0.05f ? juce::String ("OFF") : "+" + juce::String (v, 1) + " st"; }, "Quanto parte piu' alta la nota prima di scendere: il colpo dell'808.");
        add ("s_ptime", "P.TEMPO", C::yellow, ms, "Quanto ci mette il pitch a scendere sulla nota.");
        add ("s_att", "ATTACCO", C::violet, [] (float v) { return juce::String (v, 1) + " ms"; }, "Quanto ci mette a salire.");
        add ("s_dec", "CODA", C::violet, [] (float v) { return juce::String (v, 2) + " s"; }, "Quanto dura la coda dell'808 (fino al silenzio).");
        add ("s_sus", "TENUTA", C::violet, pct, "Livello che resta finche' tieni il tasto (0 = 808 classica che si spegne da sola).");
        add ("s_rel", "RILASCIO", C::violet, ms, "Quanto si spegne dopo che lasci il tasto.");
        add ("s_vel", "VELOCITY", C::violet, pct, "Quanto il volume segue la forza con cui suoni.");
        add ("s_lvl", "LIVELLO", C::text, db, "Volume del synth prima della catena.");
        add ("s_glide", "GLIDE", C::magenta, ms, "Tempo di scivolamento fra le note.");
        add ("s_tune", "TUNE", C::phos, [] (float v) { return (v > 0 ? "+" : "") + juce::String (juce::roundToInt (v)) + " st"; }, "Accordatura in semitoni.");
        add ("s_fine", "FINE", C::phos, [] (float v) { return (v > 0 ? "+" : "") + juce::String (juce::roundToInt (v)) + " c"; }, "Accordatura fine in cent.");
        add ("s_bend", "BEND", C::phos, [] (float v) { return juce::String (juce::roundToInt (v)) + " st"; }, "Escursione della rotella del pitch bend.");
        add ("s_smix", "SAMPLE", C::yellow, [] (float v) { return v < 0.005f ? juce::String ("SINTESI") : v > 0.995f ? juce::String ("SAMPLE") : juce::String (juce::roundToInt (v * 100)) + "%"; }, "Miscela fra la sintesi e il tuo sample.");
        add ("s_sstart", "INIZIO", C::yellow, [] (float v) { return juce::String (juce::roundToInt (v * 200)) + "%"; }, "Da dove parte il sample (salta l'attacco).");
    }
    addAndMakeVisible (sizeChip);
    setupBrowser();
    scaler.setup();
    startTimerHz (30);
}
static jkit::Knob* knobById (juce::OwnedArray<jkit::Knob>& K, const char* id) { for (auto* k : K) if (k->getName() == id) return k; return nullptr; }
void Editor::resized()
{
    const float s = scaler.update();
    deckY = kSynth ? 622 : 532;
    auto put = [this] (const char* id, int x, int y) { if (auto* k = knobById (knobs, id)) k->setBounds (x, y, 74, 100); };
    const int a = deckY + 26, b = deckY + 152;
    put ("punch", 44, a); put ("sustain", 118, a);
    if (! kSynth) { put ("subPlus", 214, a); put ("subDb", 288, a); } else put ("subDb", 251, a);
    put ("knock", 384, a); put ("knockHz", 458, a); put ("topHz", 532, a);
    put ("drive", 628, a);
    for (int m = 0; m < 6; ++m) modeR[m] = { 708.0f + (m % 3) * 64, (float) a + 16 + (m / 3) * 30, 60, 24 };
    put ("harm", 44, b); put ("duck", 140, b); put ("rel", 214, b); put ("mono", 310, b);
    put ("ceil", 940, b); put ("out", 1014, b);
    if (kSynth)
    {
        const int r1 = 104, r2 = 214, r3 = 324;
        int x = 44; for (auto* id : { "s_tb", "s_reese", "s_sub", "s_click", "s_dist" }) { put (id, x, r1); x += 74; }
        x += 18; for (auto* id : { "s_pamt", "s_ptime" }) { put (id, x, r1); x += 74; }
        x = 44; for (auto* id : { "s_att", "s_dec", "s_sus", "s_rel", "s_vel" }) { put (id, x, r2); x += 74; }
        x += 18; put ("s_lvl", x, r2);
        put ("s_glide", 44, r3); for (int m = 0; m < 3; ++m) gmodeR[m] = { 124.0f, (float) r3 + 6 + m * 28, 84, 24 };
        x = 236; for (auto* id : { "s_tune", "s_fine", "s_bend" }) { put (id, x, r3); x += 74; }
        polyR = { 470, (float) r3 + 20, 120, 30 };
        put ("s_smix", 1002, 112); put ("s_sstart", 1076, 112);
        smp.setBounds (668, 98, 326, 146); smp.loadR = { 326 - 126, 146 - 28, 60, 24 }; smp.clearR = { 326 - 62, 146 - 28, 60, 24 };
        smp.rootDn = { 326 - 190, 146 - 28, 26, 24 }; smp.rootUp = { 326 - 160, 146 - 28, 26, 24 };
        env.setBounds (668, 284, 476, 140); keys.setBounds (40, 476, 584, 116); spec.setBounds (656, 456, 500, 150);
    }
    else
    {
        tuner.setBounds (24, 92, 372, 388); spec.setBounds (412, 96, 452, 412); doc.setBounds (880, 100, 276, 404);
        resetR = { 130, 482, 160, 22 };
    }
    undoR = { 700, 22, 66, 32 }; redoR = { 770, 22, 66, 32 }; invR = { 842, 22, 112, 32 }; themeR = { 1074, 24, 90, 28 };
    browser.setBounds (0, 0, W, H);
    prevR = { 316, 22, 28, 32 }; presetR = { 346, 22, 316, 32 }; nextR = { 664, 22, 28, 32 };
    sizeChip.setBounds (W - 76, H - 16, 58, 14);
    bg = {};
    jlook::scaleChildren (*this, s);
}
static void panel808 (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c, const juce::String& tag)
{
    if (skin::cur() != 3)
    {   // temi della serie: la scheda del tema, la targhetta in alto a destra (a sinistra stanno i titoli dei gruppi)
        jlook::softShadow (g, r, 14); jlook::card (g, r, 14, c, true, false, false);
        g.setFont (jkit::uf (9.0f)); const auto t = tag.replace ("//", " / ");
        const float tw = juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), t) + 26;
        auto tr = juce::Rectangle<float> (r.getRight() - tw - 14, r.getY() - 9, tw, 18);
        jlook::button (g, tr, c, true, false, t, jkit::uf (8.5f));
        return;
    }
    // CYBER: lo stile proprio dell'808, spigoli tagliati e neon
    const auto p = cut (r, 16);
    g.setColour (C::bg.withAlpha (0.9f)); g.fillPath (p);
    juce::ColourGradient tint (c.withAlpha (0.10f), r.getX(), r.getY(), c.withAlpha (0.0f), r.getX(), r.getY() + 120, false); g.setGradientFill (tint); g.fillPath (p);
    neonStroke (g, p, c.withAlpha (0.85f), 1.2f);
    g.setColour (c); g.fillRect (r.getX() + 22, r.getY() - 2, 46.0f, 4.0f); g.fillRect (r.getRight() - 30, r.getBottom() - 2, 22.0f, 4.0f);
    g.setFont (s8mono (11)); const float tw = juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), tag) + 12;
    g.setColour (C::bg); g.fillRect (r.getX() + 72, r.getY() - 8, tw, 14.0f);
    g.setColour (c.withAlpha (0.9f)); g.drawText (tag, juce::Rectangle<float> (r.getX() + 78, r.getY() - 8, tw, 14), juce::Justification::centredLeft);
}
void Editor::paintStatic (juce::Graphics& g)
{
    const float Wf = (float) W, Hf = (float) H, hz = (float) deckY + 8;
    const bool cyberT = skin::cur() == 3;
    if (! cyberT) { jlook::backdrop (g, { 0, 0, Wf, Hf }, C::magenta, C::phos); jlook::header (g, { 0, 0, Wf, 72 }, "808", C::yellow, kSynth ? "SYNTH" : "STUDIO", C::magenta, C::phos); }
    else
    {
    juce::ColourGradient sky (juce::Colour (0xff140626), 0, 0, juce::Colour (0xff05040c), 0, Hf, false); g.setGradientFill (sky); g.fillAll();
    juce::Random rnd (12);
    for (int k = 0; k < 40; ++k)
    {
        const float w = 20 + rnd.nextFloat() * 60, h = 60 + rnd.nextFloat() * 280, x = rnd.nextFloat() * Wf, y = hz - h;
        g.setColour (juce::Colour (0xff0d0820).interpolatedWith (juce::Colour (0xff1d0a2e), rnd.nextFloat())); g.fillRect (x, y, w, h);
        for (float wy = y + 6; wy < hz - 10; wy += 9) for (float wx = x + 3; wx < x + w - 4; wx += 7)
            if (rnd.nextFloat() < 0.08f) { g.setColour ((rnd.nextBool() ? C::magenta : C::yellow).withAlpha (0.2f + rnd.nextFloat() * 0.4f)); g.fillRect (wx, wy, 3.0f, 4.0f); }
    }
    juce::ColourGradient haze (C::yellow.withAlpha (0.0f), 0, hz - 160, C::magenta.withAlpha (0.16f), 0, hz, false); g.setGradientFill (haze); g.fillRect (0.0f, hz - 160, Wf, 160.0f);
    g.setColour (juce::Colour (0xff07040d)); g.fillRect (0.0f, hz, Wf, Hf - hz);
    for (int i = -20; i <= 20; ++i) { g.setColour (C::phos.withAlpha (0.18f)); g.drawLine (Wf / 2 + i * 18.0f, hz, Wf / 2 + i * 140.0f, Hf, 1.0f); }
    for (int j = 1; j < 14; ++j) { const float t = std::pow (j / 14.0f, 2.0f), y = hz + (Hf - hz) * t; g.setColour (C::phos.withAlpha (0.08f + 0.18f * t)); g.drawLine (0, y, Wf, y, 1.0f); }
    }
    if (kSynth)
    {
        panel808 (g, { 24, 84, 616, 352 }, C::phos, "SYNTH//808");
        panel808 (g, { 656, 84, 500, 176 }, C::yellow, "SAMPLE//ACCORDATO");
        panel808 (g, { 656, 272, 500, 164 }, C::violet, "INVILUPPI");
        panel808 (g, { 24, 452, 616, 154 }, C::magenta, "PROVA//TASTIERA");
        panel808 (g, { 656, 452, 500, 154 }, C::magenta, "SPECTRUM//808");
        auto grp = [&] (float x, float y, float w, const char* t) { g.setColour (C::phos.withAlpha (0.45f)); g.fillRect (x, y + 12, w, 1.0f); g.setFont (s8mono (10)); g.setColour (C::dim); g.drawText (t, juce::Rectangle<float> (x, y, w, 12), juce::Justification::centred); };
        grp (44, 90, 370, "OSCILLATORE"); grp (432, 90, 148, "COLPO DI PITCH");
        grp (44, 204, 370, "VOLUME (808: CODA; TENUTA = BASSO SUONATO)"); grp (432, 204, 74, "SYNTH");
        grp (44, 314, 170, "GLIDE"); grp (236, 314, 222, "ACCORDATURA"); grp (470, 314, 120, "VOCI");
    }
    else
    {
        panel808 (g, { 24, 84, 372, 428 }, C::yellow, "TUNER//KEY");
        panel808 (g, { 412, 84, 452, 428 }, C::magenta, "SPECTRUM//808");
        panel808 (g, { 880, 84, 288, 428 }, C::red, "DOCTOR//COSA MANCA");
    }
    panel808 (g, { 24, (float) deckY, 1144, (float) (H - deckY - 22) }, C::phos, "CATENA//808 STUDIO");
    auto grp = [&] (float x, float y, float w, const char* t) { g.setColour (C::phos.withAlpha (0.45f)); g.fillRect (x, y + 12, w, 1.0f); g.setFont (s8mono (10)); g.setColour (C::dim); g.drawText (t, juce::Rectangle<float> (x, y, w, 12), juce::Justification::centred); };
    const float a = (float) deckY + 10, b = (float) deckY + 138;
    grp (44, a, 148, "TRANSIENTI"); grp (214, a, 148, kSynth ? "SUB" : "SUB (RINFORZO)"); grp (384, a, 222, "TONO"); grp (628, a, 270, "SATURAZIONE (SUB PULITO)");
    grp (44, b, 74, "TELEFONI"); grp (140, b, 148, "CASSA IN SIDECHAIN"); grp (310, b, 74, "STEREO"); grp (940, b, 210, "USCITA");
    if (cyberT)
    {
        for (float y = 0; y < Hf; y += 3) { g.setColour (juce::Colours::black.withAlpha (0.10f)); g.fillRect (0.0f, y, Wf, 1.0f); }
        juce::ColourGradient vg (juce::Colours::transparentBlack, Wf / 2, Hf / 2, juce::Colours::black.withAlpha (0.55f), 0, 0, true); g.setGradientFill (vg); g.fillAll();
    }
}
void Editor::paint (juce::Graphics& g)
{
    g.addTransform (juce::AffineTransform::scale (scaler.s));
    const float sc = juce::jlimit (1.0f, 4.0f, g.getInternalContext().getPhysicalPixelScaleFactor());
    if (bg.isNull() || std::abs (bgScale - sc) > 0.01f || bgTheme != skin::cur())
    {
        bgTheme = skin::cur();
        bgScale = sc; bg = juce::Image (juce::Image::RGB, juce::roundToInt (W * sc), juce::roundToInt (H * sc), false);
        juce::Graphics ig (bg); ig.addTransform (juce::AffineTransform::scale (sc)); paintStatic (ig);
    }
    { g.saveState(); g.addTransform (juce::AffineTransform::scale (1.0f / sc)); g.drawImageAt (bg, 0, 0); g.restoreState(); }
    if (skin::cur() != 3) {}   // testata della pelle (nello sfondo)
    else
    {
        const float gx = glitchOff;
        g.setFont (jlook::logo (40));
        if (skin::cur() == 3)
        {
            g.setColour (C::phos.withAlpha (0.8f)); g.drawText ("808", juce::Rectangle<float> (22 - 3 + gx, 8, 120, 44), juce::Justification::centredLeft);
            g.setColour (C::red.withAlpha (0.8f)); g.drawText ("808", juce::Rectangle<float> (22 + 3 - gx, 10, 120, 44), juce::Justification::centredLeft);
        }
        g.setColour (C::yellow); g.drawText ("808", juce::Rectangle<float> (22, 9, 120, 44), juce::Justification::centredLeft);
        g.setFont (jlook::logo (30)); g.setColour (C::text); g.drawText (kSynth ? "SYNTH" : "STUDIO", juce::Rectangle<float> (118 + gx * 0.4f, 13, 200, 40), juce::Justification::centredLeft);
        g.setFont (s8mono (11)); g.setColour (C::dim); g.drawText (kSynth ? "JACKPOT // 808 HUB // SINTESI + SAMPLE + CATENA STUDIO" : "JACKPOT // 808 HUB // TRAP DRILL HIP HOP", juce::Rectangle<float> (24, 52, 360, 14), juce::Justification::centredLeft);
    }
    jkit::lcd (g, presetR, 8, C::violet); jlook::glass (g, presetR, 8);
    {
        juce::String grp ("I MIEI"); if (proc.curPreset >= 0) grp = juce::String (presets()[(size_t) proc.curPreset].name).upToFirstOccurrenceOf (" | ", false, false);
        g.setFont (s8mono (9)); g.setColour (C::dim); g.drawText (grp, presetR.withHeight (12).translated (0, 2), juce::Justification::centred);
        glowText (g, proc.presetName.toUpperCase(), presetR.withTrimmedTop (10), C::violet, mf (12.0f), juce::Justification::centred);
    }
    g.setFont (s8lab (16)); g.setColour (C::violet); g.drawText ("<", prevR, juce::Justification::centred); g.drawText (">", nextR, juce::Justification::centred);
    chipBtn (g, undoR, juce::String::fromUTF8 ("\xe2\x86\xb6 ANNULLA"), false, proc.undo.canUndo() ? C::phos : C::dim, 9.5f);
    chipBtn (g, redoR, juce::String::fromUTF8 ("RIPETI \xe2\x86\xb7"), false, proc.undo.canRedo() ? C::phos : C::dim, 9.5f);
    chipBtn (g, invR, "INVERTI FASE", proc.v ("inv") > 0.5f, C::red, 10.5f);
    if (! kSynth) chipBtn (g, resetR, "NUOVA ANALISI", false, C::phos, 10.5f);
    {
        const bool k = proc.ch.scLive.load(); const auto c = k ? C::phos : C::dim;
        g.setColour (c); g.fillEllipse (962, 33, 9, 9);
        g.setFont (s8mono (10)); g.setColour (c); g.drawFittedText (k ? "CASSA IN SIDECHAIN" : "NESSUNA CASSA IN SIDECHAIN", juce::Rectangle<int> (975, 20, 92, 36), juce::Justification::centredLeft, 2);
    }
    skin::chipDraw (g, themeR, hoverBtn == 9);
    for (int m = 0; m < 6; ++m) chipBtn (g, modeR[m], modeName6 (m), juce::roundToInt (proc.v ("mode")) == m, C::magenta, 10);
    if (kSynth)
    {
        static const char* gm[3] = { "OFF", "LEGATO", "SEMPRE" };
        for (int m = 0; m < 3; ++m) chipBtn (g, gmodeR[m], gm[m], juce::roundToInt (proc.v ("s_gmode")) == m, C::magenta, 10);
        chipBtn (g, polyR, proc.v ("s_poly") > 0.5f ? "POLY (6 VOCI)" : "MONO", proc.v ("s_poly") > 0.5f, C::violet, 10.5f);
    }
    {   // clipper e uscita
        const float gr = proc.ch.uiGrClip.load(); auto r = juce::Rectangle<float> (1100, (float) deckY + 30, 44, (float) (H - deckY - 96));
        g.setColour (juce::Colour (0xff0b0816)); g.fillRect (r);
        const float op = juce::jlimit (0.0f, 1.0f, (proc.ch.outPeak.load() + 36.0f) / 36.0f);
        g.setColour (C::phos.withAlpha (0.7f)); g.fillRect (r.withTrimmedTop (r.getHeight() * (1 - op)).withWidth (18));
        g.setColour (C::red); g.fillRect (r.withTrimmedTop (r.getHeight() * (1 - juce::jlimit (0.0f, 1.0f, gr / 6.0f))).withTrimmedLeft (24));
        g.setFont (s8mono (10)); g.setColour (gr > 0.05f ? C::red : C::dim); g.drawText ("OUT  CLIP", r.withY (r.getBottom() + 4).withHeight (12).expanded (8, 0), juce::Justification::centred);
    }
    if (! kSynth && proc.v ("subPlus") > 0.01f && proc.ch.uiSubPlus.load() > 20)
    {
        g.setFont (s8mono (10)); g.setColour (C::phos);
        g.drawText ("SUB+ AGGANCIATO A " + juce::String (proc.ch.uiSubPlus.load(), 1) + " Hz", juce::Rectangle<float> (204, (float) deckY + 124, 168, 12), juce::Justification::centred);
    }
}
void Editor::mouseDown (const juce::MouseEvent& e)
{
    const auto p = e.position / scaler.s;
    for (int m = 0; m < 6; ++m) if (modeR[m].contains (p)) { proc.setV ("mode", (float) m); repaint(); return; }
    if (kSynth)
    {
        for (int m = 0; m < 3; ++m) if (gmodeR[m].contains (p)) { proc.setV ("s_gmode", (float) m); repaint(); return; }
        if (polyR.contains (p)) { proc.setV ("s_poly", proc.v ("s_poly") > 0.5f ? 0.0f : 1.0f); repaint(); return; }
    }
    if (undoR.contains (p)) { doUndo (false); return; }
    if (redoR.contains (p)) { doUndo (true); return; }
    if (invR.contains (p)) { proc.setV ("inv", proc.v ("inv") > 0.5f ? 0.0f : 1.0f); repaint(); return; }
    if (! kSynth && resetR.contains (p)) { proc.resetStats(); repaint(); return; }
    if (prevR.contains (p) || nextR.contains (p))
    {
        const int n = (int) presets().size(); proc.loadPreset ((proc.curPreset + (nextR.contains (p) ? 1 : n - 1)) % n); repaint(); return;
    }
    if (themeR.contains (p)) { skin::cycle(); C::sync(); bg = {}; std::function<void (juce::Component*)> all = [&all] (juce::Component* c) { c->repaint(); for (auto* q : c->getChildren()) all (q); }; all (this); return; }
    if (presetR.contains (p)) { browser.open(); return; }
}
void Editor::mouseMove (const juce::MouseEvent& e)
{
    const auto p = e.position / scaler.s; const int h = themeR.contains (p) ? 9 : -1;
    if (h != hoverBtn) { hoverBtn = h; repaint (scaler.map (themeR.toNearestInt().expanded (4))); }
    setMouseCursor (themeR.contains (p) || presetR.contains (p) || prevR.contains (p) || nextR.contains (p) || undoR.contains (p) || redoR.contains (p) || invR.contains (p) ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
}
void Editor::setupBrowser()
{
    addChildComponent (browser);
    browser.accent = C::violet; browser.title = "Preset";
    auto userFiles = [] { auto f = presetFolder().findChildFiles (juce::File::findFiles, false, "*.j808"); f.sort(); return f; };
    browser.entries = [userFiles]
    {
        std::vector<jbrowse::Entry> v;
        for (int i = 0; i < (int) presets().size(); ++i)
        {
            const juce::String nm (presets()[(size_t) i].name); jbrowse::Entry e; e.id = i;
            e.group = nm.upToFirstOccurrenceOf (" | ", false, false); e.name = nm.fromFirstOccurrenceOf (" | ", false, false); e.note = juce::String::fromUTF8 (presets()[(size_t) i].note); v.push_back (e);
        }
        const auto fs = userFiles();
        for (int i = 0; i < fs.size(); ++i) { jbrowse::Entry e; e.id = 100000 + i; e.user = true; e.group = "I MIEI"; e.name = fs[i].getFileNameWithoutExtension(); e.note = juce::String ("Preset tuo, in Musica / Jackpot / ") + (kSynth ? "808 Synth Preset." : "808 Studio Preset."); v.push_back (e); }
        return v;
    };
    browser.current = [this, userFiles]
    {
        if (proc.curPreset >= 0) return proc.curPreset;
        const auto fs = userFiles(); for (int i = 0; i < fs.size(); ++i) if (fs[i].getFileNameWithoutExtension() == proc.presetName) return 100000 + i;
        return -1;
    };
    browser.load = [this] (const jbrowse::Entry& e) { if (e.user) proc.loadPresetFile (presetFolder().getChildFile (e.name + ".j808")); else proc.loadPreset (e.id); repaint(); };
    browser.save = [this] { askPresetName(); };
    browser.canOverwrite = [this] { return proc.curPreset < 0 && presetFolder().getChildFile (proc.presetName + ".j808").existsAsFile(); };
    browser.overwrite = [this] { proc.savePresetFile (presetFolder().getChildFile (proc.presetName + ".j808")); repaint(); };
    browser.folder = [] { presetFolder().startAsProcess(); };
    browser.remove = [] (const jbrowse::Entry& e) { presetFolder().getChildFile (e.name + ".j808").moveToTrash(); };
    browser.onClose = [this] { repaint(); };
}
void Editor::askPresetName()
{
    auto* w = new juce::AlertWindow ("Salva preset", "Nome del preset:", juce::MessageBoxIconType::NoIcon, this);
    w->addTextEditor ("name", proc.presetName);
    w->addButton ("SALVA", 1, juce::KeyPress (juce::KeyPress::returnKey)); w->addButton ("ANNULLA", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    juce::Component::SafePointer<Editor> sp (this);
    w->enterModalState (true, juce::ModalCallbackFunction::create ([sp, w] (int r)
    {
        if (sp == nullptr || r != 1) return;
        const auto nm = juce::File::createLegalFileName (w->getTextEditorContents ("name").trim());
        if (nm.isNotEmpty()) { sp->proc.savePresetFile (presetFolder().getChildFile (nm + ".j808")); sp->browser.refresh(); sp->repaint(); }
    }), true);
}
bool Editor::keyPressed (const juce::KeyPress& k)
{
    if (k.getModifiers().isCommandDown() && (k.getKeyCode() == 'Z' || k.getKeyCode() == 'z')) { doUndo (k.getModifiers().isShiftDown()); return true; }
    if (k.getModifiers().isCommandDown() && (k.getKeyCode() == 'Y' || k.getKeyCode() == 'y')) { doUndo (true); return true; }
    return false;
}
void Editor::doUndo (bool redo)
{
    proc.undo.beginNewTransaction();
    if (redo ? proc.undo.redo() : proc.undo.undo()) { std::function<void (juce::Component*)> all = [&all] (juce::Component* c) { c->repaint(); for (auto* q : c->getChildren()) all (q); }; all (this); }
    repaint();
}
bool Editor::isInterestedInFileDrag (const juce::StringArray& f)
{
    if (! kSynth) return false;
    for (auto& s : f) if (juce::File (s).hasFileExtension ("wav;aif;aiff;flac")) return true;
    return false;
}
void Editor::filesDropped (const juce::StringArray& f, int, int)
{
    dragOver = false;
    for (auto& s : f) { juce::String err; if (proc.loadSample (juce::File (s), err)) { smp.msg.clear(); break; } smp.msg = err; }
    smp.waveOf = nullptr; repaint();
}
void Editor::openSample()
{
    chooser = std::make_unique<juce::FileChooser> ("Scegli un sample 808", juce::File::getSpecialLocation (juce::File::userMusicDirectory), "*.wav;*.aif;*.aiff;*.flac");
    juce::Component::SafePointer<Editor> sp (this);
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [sp] (const juce::FileChooser& fc)
    {
        if (sp == nullptr) return; const auto r = fc.getResult(); if (! r.existsAsFile()) return;
        juce::String err; if (sp->proc.loadSample (r, err)) sp->smp.msg.clear(); else sp->smp.msg = err;
        sp->smp.waveOf = nullptr; sp->repaint();
    });
}
void Editor::presetMenu()
{
    juce::PopupMenu m; m.addSectionHeader ("PRESET");
    for (int i = 0; i < (int) presets().size(); ++i) m.addItem (1 + i, presets()[(size_t) i].name, true, proc.curPreset == i);
    juce::Component::SafePointer<Editor> sp (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withTargetScreenArea (localAreaToGlobal (scaler.map (presetR.toNearestInt()))), [sp] (int r) { if (sp != nullptr && r > 0) { sp->proc.loadPreset (r - 1); sp->repaint(); } });
}
void Editor::timerCallback()
{
    ++frame;
    if (browser.isVisible()) return;
    if (bgTheme != skin::cur() && bgTheme >= 0) { skin::apply(); C::sync(); repaint(); }
    auto& C = proc.ch;
    if (! kSynth)
    {
        const bool on = C.uiConf.load() > 0.6f && C.uiLevel.load() > -50;
        if (on)
        {
            const float hz = C.uiHz.load(); tuner.hzS = tuner.hzS < 20 ? hz : tuner.hzS + 0.4f * (hz - tuner.hzS);
            const double mn = 69.0 + 12.0 * std::log2 (tuner.hzS / 440.0); tuner.centS += 0.4f * ((float) ((mn - std::round (mn)) * 100.0) - tuner.centS);
        }
        if (frame % 10 == 0) { diagnose(); doc.repaint(); }
        tuner.repaint();
    }
    else { env.repaint(); keys.repaint(); if (frame % 15 == 0) smp.repaint(); }
    spec.compute(); spec.repaint();
    if (glitchT > 0) { --glitchT; glitchOff = (float) juce::Random::getSystemRandom().nextInt (9) - 4.0f; } else { glitchOff = 0; if (juce::Random::getSystemRandom().nextInt (90) == 0) glitchT = 6; }
    repaint (scaler.map (juce::Rectangle<int> (0, 0, 380, 70)));
    if (frame % 10 == 0) repaint (scaler.map (juce::Rectangle<int> (690, 16, 490, 44)));
    repaint (scaler.map (juce::Rectangle<int> (1090, deckY + 24, 70, H - deckY - 40)));
    if (! kSynth && frame % 8 == 0) repaint (scaler.map (juce::Rectangle<int> (200, deckY + 122, 176, 16)));
}
}
