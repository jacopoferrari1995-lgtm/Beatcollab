// JACKPOT SAMPLE DNA: grafica scura come FX e Master.
// In alto il sample con il suo "DNA" (una striscia colorata dalla nota che suona in ogni momento), i tagli, i colpi, i cambi d'armonia.
// Sotto: come tagliare, i pad delle slice, come suonarle e il MIDI da trascinare in Ableton.
#include "DnaEditor.h"

namespace dna
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
static void title (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& t, juce::Colour c)
{ g.setFont (cap (9.5f)); g.setColour (c); g.drawText (t, juce::Rectangle<float> (r.getX() + 18, r.getY() + 12, 400, 14), juce::Justification::centredLeft); }
static juce::String noteOf (int i) { const int n = 36 + i; return juce::String (pcName (n)) + juce::String (n / 12 - 1); }
static int dominantPc (const Analysis& A, int s0, int s1)
{
    std::array<float, 12> t {}; const int f0 = juce::jmax (0, s0 / A.hop), f1 = juce::jmin (A.frames, juce::jmax (f0 + 1, s1 / A.hop));
    for (int f = f0; f < f1; ++f) for (int i = 0; i < 12; ++i) t[(size_t) i] += A.chroma[(size_t) f][(size_t) i] * A.wavePk[(size_t) f];
    int best = 0; for (int i = 1; i < 12; ++i) if (t[(size_t) i] > t[(size_t) best]) best = i; return best;
}

// ======================================================================= il sample
void WaveView::paint (juce::Graphics& g)
{
    auto& P = ed.proc; auto k = P.kit(); auto a = P.analysis(); auto b = getLocalBounds().toFloat();
    auto strip = juce::Rectangle<float> (18, 40, b.getWidth() - 36, 18), wv = juce::Rectangle<float> (18, 66, b.getWidth() - 36, b.getHeight() - 92);
    if (k == nullptr)
    {
        g.setColour (K::hi.withAlpha (0.08f)); g.drawRoundedRectangle (wv.reduced (2), 12, 1.5f);
        g.setFont (f (16.0f, 1)); g.setColour (K::sub); g.drawText (U ("TRASCINA QUI UN SAMPLE"), wv, juce::Justification::centred);
        g.setFont (f (11.0f)); g.setColour (K::faint); g.drawText (U ("wav, aiff, flac, mp3  //  oppure CARICA in alto"), wv.translated (0, 24), juce::Justification::centred);
        return;
    }
    const int n = k->audio.getNumSamples(); auto xOf = [&] (double s) { return wv.getX() + wv.getWidth() * (float) (s / juce::jmax (1, n)); };
    // griglia dei battiti
    if (a != nullptr && a->bpm > 0)
    {
        const double spb = 60.0 / a->bpm * k->sr;
        for (int i = 0; ; ++i) { const double s = a->firstBeat * k->sr + i * spb; if (s > n) break; g.setColour (K::hi.withAlpha (i % 4 == 0 ? 0.14f : 0.05f)); g.fillRect (xOf (s), wv.getY(), 1.0f, wv.getHeight()); if (i % 4 == 0) { g.setFont (num (8.5f)); g.setColour (K::faint); g.drawText (juce::String (i / 4 + 1), juce::Rectangle<float> (xOf (s) + 2, wv.getBottom() + 2, 30, 11), juce::Justification::centredLeft); } }
    }
    // slice: fondo alternato colorato dalla nota dominante
    const auto& st = k->starts; const int ns = (int) st.size();
    for (int i = 0; i < ns; ++i)
    {
        const int s0 = st[(size_t) i], s1 = i + 1 < ns ? st[(size_t) i + 1] : n;
        const int pc = a != nullptr ? dominantPc (*a, s0, s1) : 0;
        const bool on = i < 32 && P.playing[i].load() > 0;
        g.setColour (ed.pcCol (pc).withAlpha (on ? 0.30f : (i % 2 ? 0.07f : 0.12f))); g.fillRect (juce::Rectangle<float> (xOf (s0), wv.getY(), xOf (s1) - xOf (s0), wv.getHeight()));
    }
    // forma d'onda
    const int cols = (int) wv.getWidth(); const float mid = wv.getCentreY(), hh = wv.getHeight() * 0.46f;
    const float* d0 = k->audio.getReadPointer (0);
    float gmax = 1e-6f; for (int i = 0; i < n; i += 64) gmax = juce::jmax (gmax, std::abs (d0[i]));
    g.setColour (K::ink.withAlpha (0.75f));
    for (int x = 0; x < cols; ++x)
    {
        const int s0 = (int) ((double) x / cols * n), s1 = juce::jmin (n, (int) ((double) (x + 1) / cols * n) + 1);
        float mn = 0, mx = 0; for (int s = s0; s < s1; s += juce::jmax (1, (s1 - s0) / 64)) { mn = juce::jmin (mn, d0[s]); mx = juce::jmax (mx, d0[s]); }
        g.fillRect (wv.getX() + x, mid - mx / gmax * hh, 1.0f, juce::jmax (1.0f, (mx - mn) / gmax * hh));
    }
    // DNA: la nota che suona in ogni momento
    if (a != nullptr)
    {
        float pkMax = 1e-6f; for (float v : a->wavePk) pkMax = juce::jmax (pkMax, v);
        for (int x = 0; x < (int) strip.getWidth(); x += 2)
        {
            const int f = juce::jlimit (0, a->frames - 1, (int) ((float) x / strip.getWidth() * a->frames));
            const auto& c = a->chroma[(size_t) f]; int best = 0; for (int i = 1; i < 12; ++i) if (c[(size_t) i] > c[(size_t) best]) best = i;
            const float e = a->wavePk[(size_t) f] / pkMax;
            g.setColour (ed.pcCol (best, e).withAlpha (0.25f + 0.75f * juce::jlimit (0.0f, 1.0f, e * 1.5f))); g.fillRect (strip.getX() + x, strip.getY(), 2.0f, strip.getHeight());
        }
        // colpi (tacche) e cambi d'armonia (righe viola)
        for (size_t i = 0; i < a->onsets.size(); ++i) { const float x = xOf (a->onsets[i] * (double) a->hop); g.setColour (K::amber.withAlpha (0.3f + 0.6f * a->onsetStr[i])); g.fillRect (x - 0.5f, wv.getY(), 1.0f, 6.0f * (0.5f + a->onsetStr[i])); }
        for (int c : a->changes) { const float x = xOf (c * (double) a->hop); g.setColour (K::violet.withAlpha (0.8f)); for (float y = wv.getY(); y < wv.getBottom(); y += 8) g.fillRect (x - 0.5f, y, 1.0f, 4.0f); }
    }
    // tagli con numero e nota MIDI
    for (int i = 0; i < ns; ++i)
    {
        const float x = xOf (st[(size_t) i]);
        g.setColour (K::cyan); g.fillRect (x - 0.75f, wv.getY(), 1.5f, wv.getHeight());
        juce::Path tri; tri.addTriangle (x, wv.getY() + 10, x - 5, wv.getY(), x + 5, wv.getY()); g.fillPath (tri);
        g.setFont (num (9.0f)); g.setColour (K::ink); g.drawText (juce::String (i + 1), juce::Rectangle<float> (x + 3, wv.getY() + 2, 30, 11), juce::Justification::centredLeft);
        g.setFont (num (8.0f)); g.setColour (K::faint); g.drawText (noteOf (i), juce::Rectangle<float> (x + 3, wv.getY() + 13, 30, 10), juce::Justification::centredLeft);
    }
    const float pp = P.playPos.load(); if (pp >= 0) { g.setColour (K::mint); g.fillRect (xOf (pp) - 1, wv.getY(), 2.0f, wv.getHeight()); }
    g.setFont (cap (8.0f)); g.setColour (K::faint);
    g.drawText (U ("DNA = la nota che suona (colore)"), juce::Rectangle<float> (strip.getX(), strip.getY() - 13, 300, 11), juce::Justification::centredLeft);
    g.drawText (U ("arancio = colpi  //  viola = cambi d'armonia  //  azzurro = tagli  //  clic = ascolta la slice"), juce::Rectangle<float> (wv.getX(), wv.getBottom() + 12, wv.getWidth(), 11), juce::Justification::centredRight);
}
void WaveView::mouseDown (const juce::MouseEvent& e)
{
    auto k = ed.proc.kit(); if (k == nullptr) return;
    const float x0 = 18, w = (float) getWidth() - 36; const double s = (e.position.x - x0) / w * k->audio.getNumSamples();
    int sl = 0; for (int i = 0; i < (int) k->starts.size(); ++i) if (k->starts[(size_t) i] <= s) sl = i;
    ed.proc.preview = sl;
}

// ======================================================================= pad
void PadView::paint (juce::Graphics& g)
{
    auto& P = ed.proc; auto k = P.kit(); auto a = P.analysis(); auto b = getLocalBounds().toFloat().reduced (16, 0).withTrimmedTop (38).withTrimmedBottom (14);
    const int ns = k != nullptr ? (int) k->starts.size() : 0; const float pw = b.getWidth() / 8, ph = b.getHeight() / 4;
    for (int i = 0; i < 32; ++i)
    {
        const int row = 3 - i / 8, col = i % 8; auto r = juce::Rectangle<float> (b.getX() + col * pw, b.getY() + row * ph, pw, ph).reduced (3);
        const bool has = i < ns, on = has && P.playing[i].load() > 0;
        int pc = 0; if (has && a != nullptr) pc = dominantPc (*a, k->starts[(size_t) i], i + 1 < ns ? k->starts[(size_t) i + 1] : k->audio.getNumSamples());
        const auto c = has ? ed.pcCol (pc) : K::faint;
        g.setColour (has ? c.withAlpha (on ? 0.55f : 0.16f) : K::lo.withAlpha (0.25f)); g.fillRoundedRectangle (r, 6);
        g.setColour (has ? c.withAlpha (on ? 1.0f : 0.55f) : K::hi.withAlpha (0.06f)); g.drawRoundedRectangle (r.reduced (0.5f), 6, on ? 1.6f : 1.0f);
        g.setFont (num (12.0f)); g.setColour (has ? K::ink : K::faint); g.drawText (juce::String (i + 1), r.withTrimmedBottom (r.getHeight() * 0.45f), juce::Justification::centred);
        g.setFont (num (8.5f)); g.setColour (has ? K::sub : K::faint); g.drawText (noteOf (i) + (has ? "  " + juce::String (pcName (pc)) : juce::String()), r.withTrimmedTop (r.getHeight() * 0.5f), juce::Justification::centred);
    }
}
void PadView::mouseDown (const juce::MouseEvent& e)
{
    auto b = getLocalBounds().toFloat().reduced (16, 0).withTrimmedTop (38).withTrimmedBottom (14);
    if (! b.contains (e.position)) return;
    const int col = (int) ((e.position.x - b.getX()) / (b.getWidth() / 8)), row = 3 - (int) ((e.position.y - b.getY()) / (b.getHeight() / 4));
    ed.proc.preview = row * 8 + col;
}

// ======================================================================= editor
Editor::Editor (Processor& p) : AudioProcessorEditor (p), proc (p)
{
    skin::init ("sampledna", { 0xffe58aa0, 0xffb9a6e0, 0xffe9a260, 0xffa8bea0 }, 0xff3ee8ff);
    addAndMakeVisible (wave); addAndMakeVisible (pads);
    auto add = [&] (const char* id, const char* lab, juce::Colour a, juce::Colour b2, std::function<juce::String (float)> fm, const char* tip)
    {
        auto* prm = proc.apvts.getParameter (id); auto* k = knobs.add (new neo::Knob (prm, lab, a, b2));
        k->text = [prm, fm] { return fm (prm->convertFrom0to1 (prm->getValue())); }; k->defV = prm->getDefaultValue(); k->setTooltip (juce::String::fromUTF8 (tip)); addAndMakeVisible (k);
    };
    add ("count", "SLICE", K::cyan, K::violet, [] (float v) { return juce::String (juce::roundToInt (v)); }, "Quante slice al massimo.");
    add ("sens", "SENSIBILITA'", K::cyan, K::violet, [] (float v) { return juce::String (juce::roundToInt (v * 100)) + "%"; }, "Quanto deve essere forte un colpo per contare. Piu' alta = trova anche i colpi deboli.");
    add ("pitch", "PITCH", K::amber, K::pink, [] (float v) { return (v > 0 ? "+" : "") + juce::String (juce::roundToInt (v)) + " st"; }, "Trasposizione di tutte le slice.");
    add ("att", "ATTACCO", K::amber, K::pink, [] (float v) { return juce::String (v, v < 10 ? 1 : 0) + " ms"; }, "Dissolvenza all'inizio della slice.");
    add ("rel", "RILASCIO", K::amber, K::pink, [] (float v) { return juce::String (juce::roundToInt (v)) + " ms"; }, "Dissolvenza alla fine (o quando lasci il tasto).");
    add ("gain", "VOLUME", K::mint, K::cyan, [] (float v) { return (v > 0 ? "+" : "") + juce::String (v, 1) + " dB"; }, "Volume d'uscita.");
    addAndMakeVisible (sizeChip);
    scaler.setup();
    startTimerHz (30);
}
void Editor::resized()
{
    const float s = scaler.update();
    wave.setBounds (24, 76, 1052, 300); pads.setBounds (380, 390, 448, 318);
    const int S = 50, w = S + 24;
    knobs[0]->setBounds (juce::Rectangle<int> (60, 560, w, S + 38).expanded (Knob::pad)); knobs[1]->setBounds (juce::Rectangle<int> (200, 560, w, S + 38).expanded (Knob::pad));
    knobs[2]->setBounds (juce::Rectangle<int> (858, 420, w, S + 38).expanded (Knob::pad)); knobs[3]->setBounds (juce::Rectangle<int> (970, 420, w, S + 38).expanded (Knob::pad));
    knobs[4]->setBounds (juce::Rectangle<int> (858, 524, w, S + 38).expanded (Knob::pad)); knobs[5]->setBounds (juce::Rectangle<int> (970, 524, w, S + 38).expanded (Knob::pad));
    loadR = { 330, 20, 110, 32 }; themeR = { 1006, 22, 76, 28 }; halfR = { 640, 22, 30, 28 }; dblR = { 674, 22, 30, 28 };
    for (int m = 0; m < kModes; ++m) modeR[m] = { 40.0f + (m % 2) * 160, 430.0f + (m / 2) * 36, 152, 30 };
    varR = { 40, 664, 312, 32 };
    playR[0] = { 852, 628, 108, 26 }; playR[1] = { 964, 628, 100, 26 }; chokeR = { 852, 658, 212, 22 };
    midiR = { 852, 684 - 4, 212, 26 };
    sizeChip.setBounds (W - 76, H - 18, 58, 15);
    bg = {};
    jlook::scaleChildren (*this, s);
}
void Editor::paintStatic (juce::Graphics& g)
{
    if (skin::cur() != 3) { jlook::backdrop (g, { 0, 0, (float) W, (float) H }, K::cyan, K::pink); jlook::header (g, { 0, 0, (float) W, 66 }, "DNA", K::cyan, "SAMPLE", K::pink, K::amber); }
    else
    {
    juce::ColourGradient base (K::bg1, 0, 0, K::bg0, 0, (float) H, false); g.setGradientFill (base); g.fillAll();
    // logo: doppia elica
    for (int i = 0; i < 14; ++i) { const float y = 18 + i * 2.4f, a = i * 0.55f; g.setColour (K::cyan.withAlpha (0.8f)); g.fillEllipse (36 + std::sin (a) * 10 - 2, y, 4, 2.5f); g.setColour (K::pink.withAlpha (0.8f)); g.fillEllipse (36 - std::sin (a) * 10 - 2, y, 4, 2.5f); }
    g.setFont (num (15.0f)); g.setColour (K::ink); g.drawText ("JACKPOT", juce::Rectangle<float> (60, 18, 100, 20), juce::Justification::centredLeft);
    g.setFont (big (15.0f)); g.setColour (K::sub); g.drawText ("SAMPLE DNA", juce::Rectangle<float> (137, 18, 160, 20), juce::Justification::centredLeft);
    g.setFont (cap (7.5f)); g.setColour (K::faint); g.drawText (U ("CAPISCE IL SAMPLE E LO TAGLIA CON SENSO"), juce::Rectangle<float> (60, 38, 300, 12), juce::Justification::centredLeft);
    }
    panel (g, { 24, 76, 1052, 300 }, K::cyan, true);
    panel (g, { 24, 390, 344, 318 }, K::cyan);
    panel (g, { 380, 390, 448, 318 }, K::violet);
    panel (g, { 840, 390, 236, 318 }, K::amber);
    title (g, { 24, 76, 1052, 300 }, "IL SAMPLE", K::cyan);
    title (g, { 24, 390, 344, 318 }, "COME TAGLIARE", K::cyan);
    title (g, { 380, 390, 448, 318 }, "SLICE SUI PAD  (DA C1 IN SU)", K::violet);
    title (g, { 840, 390, 236, 318 }, "SUONA", K::amber);
}
void Editor::paint (juce::Graphics& g)
{
    g.addTransform (juce::AffineTransform::scale (scaler.s));
    const float sc = juce::jlimit (1.0f, 4.0f, g.getInternalContext().getPhysicalPixelScaleFactor());
    if (bg.isNull() || std::abs (bgScale - sc) > 0.01f || bgTheme != skin::cur()) { bgTheme = skin::cur(); bgScale = sc; bg = juce::Image (juce::Image::RGB, juce::roundToInt (W * sc), juce::roundToInt (H * sc), false); juce::Graphics ig (bg); ig.addTransform (juce::AffineTransform::scale (sc)); paintStatic (ig); }
    { g.saveState(); g.addTransform (juce::AffineTransform::scale (1.0f / sc)); g.drawImageAt (bg, 0, 0); g.restoreState(); }
    auto k = proc.kit(); auto a = proc.analysis();
    chip (g, loadR, "CARICA...", false, K::cyan, 9.0f);
    skin::chipDraw (g, themeR, false);
    // letture
    auto readout = [&] (float x, float w, const juce::String& lab, const juce::String& val, juce::Colour c)
    { g.setFont (cap (7.5f)); g.setColour (K::faint); g.drawText (lab, juce::Rectangle<float> (x, 16, w, 12), juce::Justification::centredLeft); g.setFont (num (17.0f)); g.setColour (c); g.drawText (val, juce::Rectangle<float> (x, 28, w, 24), juce::Justification::centredLeft); };
    if (proc.busy.load()) { g.setFont (f (12.0f, 1)); g.setColour (K::amber); g.drawText (U ("ANALISI IN CORSO..."), juce::Rectangle<float> (470, 20, 300, 32), juce::Justification::centredLeft); }
    else if (a != nullptr && k != nullptr)
    {
        readout (470, 160, "BPM" + juce::String (a->loopFit ? "  (LOOP DI " + juce::String (a->beats) + " BATTITI)" : ""), juce::String (a->bpm, 1), K::cyan);
        chip (g, halfR, "/2", false, K::cyan, 8.5f); chip (g, dblR, "x2", false, K::cyan, 8.5f);
        readout (724, 150, juce::String ("TONALITA'") + (a->keyConf < 0.35f ? "  (INCERTA)" : ""), a->key >= 0 ? juce::String (pcName (a->key)) + (a->minor ? " minore" : " maggiore") : "--", a->keyConf < 0.35f ? K::sub : K::pink);
        readout (880, 122, "DURATA  //  " + k->name.substring (0, 10).toUpperCase(), juce::String (a->dur, 1) + "s " + juce::String (a->dur * a->bpm / 240.0, 1) + "bt", K::ink);
    }
    // come tagliare
    static const char* desc[kModes] = { "Sceglie i tagli che contano: colpi forti, cambi d'armonia e posizioni a tempo. Prova ALTRA PROPOSTA per altre idee.",
                                        "Taglia sui colpi piu' forti. Per batterie e campioni ritmici.",
                                        "Taglia a tempo (ottavi, quarti, battute) e aggancia ogni taglio al colpo piu' vicino. Per loop da riordinare.",
                                        "Taglia dove cambia l'accordo. Per sample melodici, soul, piano." };
    const int md = juce::roundToInt (proc.v ("chop"));
    for (int m = 0; m < kModes; ++m) chip (g, modeR[m], modeName (m), md == m, K::cyan, 9.0f);
    g.setFont (f (10.0f)); g.setColour (K::sub); g.drawFittedText (U (desc[md]), juce::Rectangle<int> (40, 504, 312, 44), juce::Justification::topLeft, 3);
    chip (g, varR, "ALTRA PROPOSTA  (" + juce::String (proc.variant.load() + 1) + ")", false, K::amber, 9.5f);
    if (k != nullptr) { g.setFont (num (9.0f)); g.setColour (K::faint); g.drawText (juce::String ((int) k->starts.size()) + " slice trovate", juce::Rectangle<float> (40, 648, 312, 12), juce::Justification::centredLeft); }
    // suona
    chip (g, playR[0], "ONE SHOT", proc.v ("play") < 0.5f, K::amber, 8.5f); chip (g, playR[1], "TIENI", proc.v ("play") > 0.5f, K::amber, 8.5f);
    chip (g, chokeR, proc.v ("choke") > 0.5f ? "CHOKE: UNA ALLA VOLTA" : "CHOKE: SI SOVRAPPONGONO", proc.v ("choke") > 0.5f, K::amber, 8.0f);
    chip (g, midiR, U ("TRASCINA IL MIDI >"), dragging, K::mint, 8.5f);
}
void Editor::mouseDown (const juce::MouseEvent& e)
{
    const auto p = e.position / scaler.s; dragging = false;
    if (themeR.contains (p)) { skin::cycle(); bg = {}; std::function<void (juce::Component*)> all = [&all] (juce::Component* c) { c->repaint(); for (auto* q : c->getChildren()) all (q); }; all (this); return; }
    if (loadR.contains (p))
    {
        chooser = std::make_unique<juce::FileChooser> ("Scegli un sample", juce::File::getSpecialLocation (juce::File::userMusicDirectory), "*.wav;*.aif;*.aiff;*.flac;*.mp3;*.m4a");
        juce::Component::SafePointer<Editor> sp (this);
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [sp] (const juce::FileChooser& c) { if (sp != nullptr && c.getResult().existsAsFile()) { sp->proc.loadFile (c.getResult()); sp->repaint(); } });
        return;
    }
    if (halfR.contains (p)) { proc.halveBpm(); repaint(); return; }
    if (dblR.contains (p)) { proc.doubleBpm(); repaint(); return; }
    for (int m = 0; m < kModes; ++m) if (modeR[m].contains (p)) { proc.setV ("chop", (float) m); proc.variant = 0; repaint(); return; }
    if (varR.contains (p)) { proc.nextVariant(); repaint(); return; }
    if (playR[0].contains (p)) { proc.setV ("play", 0); repaint(); return; }
    if (playR[1].contains (p)) { proc.setV ("play", 1); repaint(); return; }
    if (chokeR.contains (p)) { proc.setV ("choke", proc.v ("choke") > 0.5f ? 0.0f : 1.0f); repaint(); return; }
}
void Editor::mouseDrag (const juce::MouseEvent& e)
{
    const auto p = e.getMouseDownPosition().toFloat() / scaler.s;
    if (! midiR.contains (p) || dragging || e.getDistanceFromDragStart() < 6) return;
    auto f = proc.writeMidi(); if (! f.existsAsFile()) return;
    dragging = true; repaint();
    juce::Component::SafePointer<Editor> sp (this);
    performExternalDragDropOfFiles ({ f.getFullPathName() }, false, this, [sp] { if (sp != nullptr) { sp->dragging = false; sp->repaint(); } });
}
void Editor::timerCallback()
{
    if (bgTheme != skin::cur() && bgTheme >= 0) { skin::apply(); bg = {}; repaint(); }
    for (auto* k : knobs) if (k->animate()) k->repaint();
    static juce::uint32 last = 0; auto k = proc.kit(); auto a = proc.analysis();
    const juce::uint32 sig = (juce::uint32) (k != nullptr ? k->starts.size() * 31 + (size_t) k->audio.getNumSamples() : 0) ^ (juce::uint32) (a != nullptr ? (int) (a->bpm * 10) : 0) ^ (proc.busy.load() ? 77u : 0u);
    if (sig != last) { last = sig; repaint(); }
    wave.repaint(); pads.repaint();
}
}
