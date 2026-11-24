// JACKPOT FLUX: grafica scura come FX e Master.
// A sinistra le 8 corsie con le 4 battute che stanno suonando (in chiaro le note vere, in trasparenza il pattern di partenza),
// a destra la mappa delle relazioni e le regole, sotto le 4 macro, le sezioni e la memoria.
#include "FluxEditor.h"

namespace flux
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
{ g.setFont (cap (9.5f)); g.setColour (c); g.drawText (t, juce::Rectangle<float> (r.getX() + 18, r.getY() + 12, 500, 14), juce::Justification::centredLeft); }
juce::Colour Editor::laneCol (int l)
{
    static const juce::Colour* c[NL] = { &K::amber, &K::pink, &K::mint, &K::cyan, &K::red, &K::violet, &K::blue, &K::sub };
    return *c[juce::jlimit (0, NL - 1, l)];
}
static const char* keyNames[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

Editor::Editor (Processor& p) : AudioProcessorEditor (p), proc (p)
{
    skin::init ("flux", { 0xffe58aa0, 0xffb9a6e0, 0xffe9a260, 0xffa8bea0 }, 0xff3ee8ff);
    undoMark.p = &p; addMouseListener (&undoMark, true); setWantsKeyboardFocus (true);
    const char* ids[4] = { "density", "energy", "movement", "space" };
    const char* labs[4] = { "DENSITY", "ENERGY", "MOVEMENT", "SPACE" };
    const char* tips[4] = { "Quante note: a sinistra toglie le meno importanti, a destra aggiunge ghost, hi-hat sui sedicesimi, ripetizioni.",
                            "Intensita': dinamica, accenti, roll piu' fitti, 808 che sale d'ottava.",
                            "Quanto cambia da una battuta all'altra: roll, spostamenti di cassa, ottave, fill a fine frase.",
                            "Sottrazione: toglie corsie in ordine (perc, contro, chop, hi-hat...) e fa la pausa a fine frase." };
    const juce::Colour cs[4] = { K::cyan, K::amber, K::violet, K::mint };
    for (int i = 0; i < 4; ++i)
    {
        auto* prm = proc.apvts.getParameter (ids[i]); auto* k = knobs.add (new neo::Knob (prm, labs[i], cs[i], cs[i].interpolatedWith (juce::Colours::white, 0.3f)));
        k->text = [prm] { return juce::String (juce::roundToInt (prm->getValue() * 100)) + "%"; }; k->defV = prm->getDefaultValue(); k->setTooltip (U (tips[i])); addAndMakeVisible (k);
    }
    addAndMakeVisible (sizeChip);
    scaler.setup();
    startTimerHz (30);
}
void Editor::resized()
{
    const float s = scaler.update();
    for (int i = 0; i < kStarters; ++i) starterR[i] = { 330.0f + i * 84, 20, 80, 30 };
    keyR = { 672, 20, 96, 30 }; playR = { 774, 20, 104, 30 }; varR = { 884, 16, 120, 38 }; abR = { 1010, 20, 66, 30 }; midiR = { 1082, 20, 74, 30 };
    for (int l = 0; l < NL; ++l) { laneR[l] = { 180.0f, 112.0f + l * 51, 528, 44 }; muteR[l] = { 112.0f, 114.0f + l * 51, 28, 18 }; varLR[l] = { 144.0f, 114.0f + l * 51, 32, 18 }; }
    drumR = { 40, 524, 180, 22 };
    for (int i = 0; i < 4; ++i) knobs[i]->setBounds (juce::Rectangle<int> (52 + i * 112, 600, 92, 120).expanded (Knob::pad));
    for (int s2 = 0; s2 < NSEC; ++s2) secR[s2] = { 540.0f + s2 * 74, 612, 70, 38 };
    for (int m = 0; m < 8; ++m) memR[m] = { 540.0f + m * 46, 680, 42, 34 };
    addR = { 752, 506, 120, 26 }; themeR = { 1060, 575, 86, 24 };
    soundR = { 920, 612, 108, 26 }; volR = { 1034, 612, 112, 26 }; followR = { 920, 646, 226, 26 };
    sizeChip.setBounds (W - 76, H - 18, 58, 15);
    bg = {};
    jlook::scaleChildren (*this, s);
}
void Editor::paintStatic (juce::Graphics& g)
{
    if (skin::cur() != 3) { jlook::backdrop (g, { 0, 0, (float) W, (float) H }, K::cyan, K::violet); jlook::header (g, { 0, 0, (float) W, 64 }, "FLUX", K::cyan, "", K::violet, K::amber); }
    else
    {
    juce::ColourGradient base (K::bg1, 0, 0, K::bg0, 0, (float) H, false); g.setGradientFill (base); g.fillAll();
    // logo: onda che scorre
    juce::Path w; for (int i = 0; i <= 30; ++i) { const float x = 22 + i, y = 34 + std::sin (i * 0.42f) * 8 * std::sin (i * 0.1f + 0.4f); if (i == 0) w.startNewSubPath (x, y); else w.lineTo (x, y); }
    g.setColour (K::cyan); g.strokePath (w, juce::PathStrokeType (2.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setFont (num (15.0f)); g.setColour (K::ink); g.drawText ("JACKPOT", juce::Rectangle<float> (62, 18, 100, 20), juce::Justification::centredLeft);
    g.setFont (big (15.0f)); g.setColour (K::sub); g.drawText ("FLUX", juce::Rectangle<float> (139, 18, 160, 20), juce::Justification::centredLeft);
    g.setFont (cap (7.5f)); g.setColour (K::faint); g.drawText (U ("IL BEAT CHE REAGISCE A SE STESSO"), juce::Rectangle<float> (62, 38, 260, 12), juce::Justification::centredLeft);
    }
    panel (g, { 24, 72, 700, 482 }, K::cyan, true); title (g, { 24, 72, 700, 482 }, U ("CORSIE  //  le 4 battute che stanno suonando"), K::cyan);
    panel (g, { 736, 72, 420, 482 }, K::violet); title (g, { 736, 72, 420, 482 }, "RELAZIONI", K::violet);
    panel (g, { 24, 566, 476, 180 }, K::amber); title (g, { 24, 566, 476, 180 }, "MACRO", K::amber);
    panel (g, { 512, 566, 644, 180 }, K::pink); title (g, { 512, 566, 644, 180 }, "SEZIONI  //  MEMORIA", K::pink);
    g.setFont (cap (7.0f)); g.setColour (K::faint);
    g.drawText ("MUTE", juce::Rectangle<float> (108, 96, 36, 10), juce::Justification::centred); g.drawText ("VARIA", juce::Rectangle<float> (142, 96, 36, 10), juce::Justification::centred);
}
void Editor::paint (juce::Graphics& g)
{
    g.addTransform (juce::AffineTransform::scale (scaler.s));
    const float sc = juce::jlimit (1.0f, 4.0f, g.getInternalContext().getPhysicalPixelScaleFactor());
    if (bg.isNull() || std::abs (bgScale - sc) > 0.01f || bgTheme != skin::cur()) { bgTheme = skin::cur(); bgScale = sc; bg = juce::Image (juce::Image::RGB, juce::roundToInt (W * sc), juce::roundToInt (H * sc), false); juce::Graphics ig (bg); ig.addTransform (juce::AffineTransform::scale (sc)); paintStatic (ig); }
    { g.saveState(); g.addTransform (juce::AffineTransform::scale (1.0f / sc)); g.drawImageAt (bg, 0, 0); g.restoreState(); }
    auto S = proc.snap(); const auto M = proc.macros();
    skin::chipDraw (g, themeR, false);
    // ---------------- testata
    for (int i = 0; i < kStarters; ++i) chip (g, starterR[i], starterName (i), proc.starter == i, K::cyan, 8.5f);
    chip (g, keyR, juce::String (keyNames[S->key]) + (S->minor ? " min" : " magg"), false, K::violet, 9.0f);
    chip (g, playR, proc.v ("play") > 0.5f ? "ANTEPRIMA ON" : "ANTEPRIMA", proc.v ("play") > 0.5f, K::mint, 8.5f);
    chip (g, varR, "VARIATE", flash > 0, K::amber, 12.0f);
    g.setFont (num (8.0f)); g.setColour (K::faint); g.drawText ("seme " + juce::String ((int) (proc.seed.load() % 100000)), juce::Rectangle<float> (884, 56, 120, 10), juce::Justification::centred);
    chip (g, abR, proc.abOriginal.load() ? "ORIG." : "A / B", proc.abOriginal.load(), K::red, 8.5f);
    chip (g, midiR, "MIDI >", dragging, K::mint, 8.5f);
    // ---------------- corsie
    const double ppq = proc.uiPpq.load(); const int curBar = (int) std::floor (ppq / BPB), base = (curBar / BARS) * BARS;
    BarEvents be;
    for (int l = 0; l < NL; ++l)
    {
        auto r = laneR[l]; const auto c = laneCol (l);
        g.setFont (f (11.0f, 1)); g.setColour (S->mute[(size_t) l] ? K::faint : K::ink); g.drawText (laneName (l), juce::Rectangle<float> (40, r.getY() + 4, 70, 16), juce::Justification::centredLeft);
        const int ch = proc.v ("drum1") > 0.5f && isDrum (l) ? 1 : l + 1;
        g.setFont (num (8.0f)); g.setColour (c); g.drawText ("CH " + juce::String (ch), juce::Rectangle<float> (40, r.getY() + 22, 60, 12), juce::Justification::centredLeft);
        chip (g, muteR[l], "M", S->mute[(size_t) l], K::red, 8.0f); chip (g, varLR[l], "V", S->var[(size_t) l], K::amber, 8.0f);
        g.setColour (K::hi.withAlpha (0.035f)); g.fillRoundedRectangle (r, 5);
        for (int b = 1; b < BARS * BPB; ++b) { g.setColour (K::hi.withAlpha (b % BPB == 0 ? 0.14f : 0.04f)); g.fillRect (r.getX() + r.getWidth() * b / (BARS * BPB), r.getY(), 1.0f, r.getHeight()); }
        // pattern di partenza (trasparente)
        int lo = 127, hi = 0; for (auto& n : S->lane[(size_t) l]) { lo = juce::jmin (lo, n.note); hi = juce::jmax (hi, n.note); }
        auto yOf = [&] (int note) { return hi > lo ? r.getBottom() - 5 - (r.getHeight() - 12) * (note - lo) / (float) juce::jmax (1, hi - lo + 12) - 6 : r.getCentreY() - 3; };
        for (auto& n : S->lane[(size_t) l]) { g.setColour (c.withAlpha (0.18f)); g.fillRect (r.getX() + r.getWidth() * n.t / (BARS * BPB), yOf (n.note), juce::jmax (2.0f, r.getWidth() * n.len / (BARS * BPB)), 5.0f); }
        // quello che suona davvero nelle 4 battute della frase corrente
        for (int b = 0; b < BARS; ++b)
        {
            Macros mm = M; juce::uint32 sd = proc.seed.load(); if (proc.abOriginal.load()) { mm = { 0.5f, 0.5f, 0.0f, 0.0f }; sd = 1; }
            renderBar (*S, mm, sd, base + b, be);
            for (int k = 0; k < be.n; ++k) if (be.ev[k].lane == l)
            {
                const auto& e = be.ev[k]; const float x = r.getX() + r.getWidth() * (b * BPB + e.t) / (BARS * BPB);
                g.setColour (c.withAlpha (0.35f + 0.6f * e.vel / 127.0f)); g.fillRect (x, yOf (e.note), juce::jmax (2.0f, r.getWidth() * e.len / (BARS * BPB) - 1), 5.0f);
            }
        }
    }
    if (proc.uiPlaying.load()) { const float x = laneR[0].getX() + laneR[0].getWidth() * (float) std::fmod (ppq, (double) BARS * BPB) / (BARS * BPB); g.setColour (K::ink.withAlpha (0.7f)); g.fillRect (x, laneR[0].getY() - 4, 1.5f, laneR[NL - 1].getBottom() - laneR[0].getY() + 8); }
    g.setFont (num (8.0f)); g.setColour (K::faint); for (int b = 0; b < BARS; ++b) g.drawText (juce::String (base + b + 1), juce::Rectangle<float> (laneR[0].getX() + laneR[0].getWidth() * b / BARS + 3, 98, 40, 10), juce::Justification::centredLeft);
    chip (g, drumR, proc.v ("drum1") > 0.5f ? "BATTERIA TUTTA SU CH 1" : "OGNI CORSIA SUL SUO CANALE", proc.v ("drum1") > 0.5f, K::cyan, 7.5f);
    g.setFont (f (9.0f)); g.setColour (K::faint); g.drawText (U ("trascina un .mid su una corsia per usarlo come pattern"), juce::Rectangle<float> (230, 524, 480, 22), juce::Justification::centredLeft);
    // ---------------- mappa delle relazioni
    {
        const float cx = 946, cy = 210, R = 92;
        auto nodePos = [&] (int l) { const float a = -juce::MathConstants<float>::halfPi + l * juce::MathConstants<float>::twoPi / NL; return juce::Point<float> (cx + std::cos (a) * R, cy + std::sin (a) * R); };
        static const juce::Colour* rc[kRuleTypes] = { &K::red, &K::cyan, &K::amber, &K::mint };
        for (auto& rr : S->rules)
        {
            if (rr.from == rr.to) continue;
            auto a = nodePos (rr.from), b = nodePos (rr.to); const auto col = (*rc[rr.type]).withAlpha (rr.on ? 0.9f : 0.2f);
            juce::Path p; const auto mid = (a + b) * 0.5f + (juce::Point<float> (cx, cy) - (a + b) * 0.5f) * 0.35f; p.startNewSubPath (a); p.quadraticTo (mid, b);
            g.setColour (col); g.strokePath (p, juce::PathStrokeType (rr.on ? 2.0f : 1.0f));
            const auto dir = (b - mid); const float L = dir.getDistanceFromOrigin(); if (L > 0) { const auto u = dir / L; const auto tip = b - u * 16.0f; juce::Path ar; ar.addTriangle (tip + u * 8.0f, tip + juce::Point<float> (-u.y, u.x) * 5.0f, tip - juce::Point<float> (-u.y, u.x) * 5.0f); g.fillPath (ar); }
        }
        for (int l = 0; l < NL; ++l)
        {
            const auto p = nodePos (l); const auto c = laneCol (l); const float hit = juce::jlimit (0.0f, 1.0f, (float) proc.uiHits[l].load() * 0.0f);
            (void) hit;
            g.setColour (c.withAlpha (S->mute[(size_t) l] ? 0.1f : 0.25f)); g.fillEllipse (p.x - 15, p.y - 15, 30, 30); g.setColour (c); g.drawEllipse (p.x - 15, p.y - 15, 30, 30, 1.4f);
            g.setFont (cap (6.5f)); g.setColour (K::ink); g.drawText (laneName (l), juce::Rectangle<float> (p.x - 34, p.y - 5, 68, 10), juce::Justification::centred);
        }
        // elenco delle regole
        ruleUI.clear(); float y = 330;
        g.setFont (cap (7.5f)); g.setColour (K::faint); g.drawText ("REGOLE  (clic per cambiare)", juce::Rectangle<float> (752, 314, 300, 12), juce::Justification::centredLeft);
        for (size_t i = 0; i < S->rules.size() && i < 7; ++i)
        {
            const auto& rr = S->rules[i]; RuleUI u;
            u.on = { 752, y, 26, 20 }; u.from = { 782, y, 84, 20 }; u.to = { 884, y, 84, 20 }; u.type = { 972, y, 140, 20 }; u.del = { 1116, y, 22, 20 };
            chip (g, u.on, rr.on ? "ON" : "-", rr.on, K::mint, 7.0f);
            chip (g, u.from, laneName (rr.from), false, laneCol (rr.from), 7.5f);
            g.setFont (f (11.0f)); g.setColour (K::sub); g.drawText (">", juce::Rectangle<float> (866, y, 18, 20), juce::Justification::centred);
            chip (g, u.to, laneName (rr.to), false, laneCol (rr.to), 7.5f);
            chip (g, u.type, ruleName (rr.type), true, *rc[rr.type], 7.0f);
            chip (g, u.del, "X", false, K::red, 7.5f);
            ruleUI.push_back (u); y += 24;
        }
        chip (g, addR, "+ REGOLA", false, K::violet, 8.0f);
        g.setFont (f (9.0f)); g.setColour (K::faint);
        g.drawFittedText (info.isNotEmpty() ? info : U ("Le regole dicono come le corsie reagiscono fra loro. Passa sopra il tipo di regola per sapere cosa fa."), juce::Rectangle<int> (880, 504, 264, 30), juce::Justification::topLeft, 2);
    }
    // ---------------- sezioni e memoria
    const int cs = juce::roundToInt (proc.v ("section"));
    g.setFont (cap (7.5f)); g.setColour (K::faint); g.drawText (U ("SEZIONI: ognuna ricorda le sue macro (automatizzabile da Ableton)"), juce::Rectangle<float> (540, 596, 420, 12), juce::Justification::centredLeft);
    for (int s2 = 0; s2 < NSEC; ++s2) chip (g, secR[s2], secName (s2), cs == s2, K::pink, 8.5f);
    g.setFont (cap (7.5f)); g.setColour (K::faint); g.drawText (U ("MEMORIA: clic = richiama, shift + clic = salva (seme, macro, regole)"), juce::Rectangle<float> (540, 664, 440, 12), juce::Justification::centredLeft);
    for (int m = 0; m < 8; ++m) chip (g, memR[m], juce::String (m + 1), proc.mem[(size_t) m].used, K::amber, 9.0f);
    // suono interno e tastiera
    chip (g, soundR, proc.v ("sound") > 0.5f ? U ("SUONO ACCESO") : U ("SUONO SPENTO"), proc.v ("sound") > 0.5f, K::mint, 8.0f);
    {
        const float vv = (proc.v ("vol") + 30.0f) / 36.0f; g.setColour (K::lo.withAlpha (0.5f)); g.fillRoundedRectangle (volR, 6);
        g.setColour (K::mint.withAlpha (0.35f)); g.fillRoundedRectangle (volR.withWidth (volR.getWidth() * juce::jlimit (0.0f, 1.0f, vv)), 6);
        g.setFont (f (9.0f)); g.setColour (K::ink); g.drawText ("VOL " + juce::String (proc.v ("vol"), 1) + " dB", volR, juce::Justification::centred);
    }
    {
        const int rt = proc.uiFollowRoot.load(); const bool fo = proc.v ("follow") > 0.5f;
        chip (g, followR, fo ? (rt >= 0 ? U ("SEGUE LA TASTIERA: ") + keyNames[rt] : U ("SEGUI LA TASTIERA: ACCESO")) : U ("SEGUI LA TASTIERA: SPENTO"), fo, K::violet, 8.0f);
    }
    g.setFont (f (8.5f)); g.setColour (K::sub);
    g.drawFittedText (U ("Tieni una nota sulla tastiera: 808 e melodie cambiano tonalita'. Per i tuoi strumenti: MIDI From = FLUX, canale della corsia (CH 1-8)."), juce::Rectangle<int> (920, 678, 226, 44), juce::Justification::topLeft, 3);
}
void Editor::mouseDown (const juce::MouseEvent& e)
{
    const auto p = e.position / scaler.s; dragging = false; auto S = proc.snap();
    if (themeR.contains (p)) { skin::cycle(); bg = {}; std::function<void (juce::Component*)> all = [&all] (juce::Component* c) { c->repaint(); for (auto* q : c->getChildren()) all (q); }; all (this); return; }
    for (int i = 0; i < kStarters; ++i) if (starterR[i].contains (p)) { proc.loadStarter (i, S->key, S->minor); repaint(); return; }
    if (keyR.contains (p))
    {
        juce::PopupMenu m, mn, mj; for (int k = 0; k < 12; ++k) { mn.addItem (1 + k, juce::String (keyNames[k]) + " minore", true, S->minor && S->key == k); mj.addItem (101 + k, juce::String (keyNames[k]) + " maggiore", true, ! S->minor && S->key == k); }
        m.addSectionHeader (U ("TONALITA' (ricarica lo starter nella scala)")); m.addSubMenu ("Minore", mn); m.addSubMenu ("Maggiore", mj);
        juce::Component::SafePointer<Editor> sp (this);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [sp] (int r) { if (sp == nullptr || r <= 0) return; sp->proc.loadStarter (sp->proc.starter, (r - 1) % 100, r < 100); sp->repaint(); });
        return;
    }
    if (playR.contains (p)) { proc.setV ("play", proc.v ("play") > 0.5f ? 0.0f : 1.0f); repaint(); return; }
    if (varR.contains (p)) { proc.variate(); flash = 8; repaint(); return; }
    if (abR.contains (p)) { proc.abOriginal = ! proc.abOriginal.load(); repaint(); return; }
    if (soundR.contains (p)) { proc.setV ("sound", proc.v ("sound") > 0.5f ? 0.0f : 1.0f); repaint(); return; }
    if (followR.contains (p)) { proc.setV ("follow", proc.v ("follow") > 0.5f ? 0.0f : 1.0f); repaint(); return; }
    if (volR.contains (p)) { volStart = proc.v ("vol"); if (e.getNumberOfClicks() > 1) proc.setV ("vol", -6); return; }
    if (drumR.contains (p)) { proc.setV ("drum1", proc.v ("drum1") > 0.5f ? 0.0f : 1.0f); repaint(); return; }
    for (int l = 0; l < NL; ++l)
    {
        if (muteR[l].contains (p)) { proc.edit ([l] (Snap& s) { s.mute[(size_t) l] = ! s.mute[(size_t) l]; }); const int cs = juce::roundToInt (proc.v ("section")); proc.sections[(size_t) cs].mute = proc.snap()->mute; repaint(); return; }
        if (varLR[l].contains (p)) { proc.edit ([l] (Snap& s) { s.var[(size_t) l] = ! s.var[(size_t) l]; }); repaint(); return; }
    }
    for (int s2 = 0; s2 < NSEC; ++s2) if (secR[s2].contains (p)) { proc.selectSection (s2); repaint(); return; }
    for (int m = 0; m < 8; ++m) if (memR[m].contains (p)) { if (e.mods.isShiftDown()) proc.storeMem (m); else proc.recallMem (m); repaint(); return; }
    if (addR.contains (p)) { proc.edit ([] (Snap& s) { if (s.rules.size() < 7) s.rules.push_back ({ KICK, PERC, DIRADA, true }); }); repaint(); return; }
    juce::Component::SafePointer<Editor> sp (this);
    for (size_t i = 0; i < ruleUI.size(); ++i)
    {
        const auto& u = ruleUI[i];
        if (u.on.contains (p)) { proc.edit ([i] (Snap& s) { s.rules[i].on = ! s.rules[i].on; }); repaint(); return; }
        if (u.del.contains (p)) { proc.edit ([i] (Snap& s) { s.rules.erase (s.rules.begin() + (long) i); }); repaint(); return; }
        if (u.from.contains (p) || u.to.contains (p))
        {
            const bool from = u.from.contains (p); juce::PopupMenu m; for (int l = 0; l < NL; ++l) m.addItem (1 + l, laneName (l));
            m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [sp, i, from] (int r) { if (sp == nullptr || r <= 0) return; sp->proc.edit ([i, from, r] (Snap& s) { if (i < s.rules.size()) (from ? s.rules[i].from : s.rules[i].to) = r - 1; }); sp->repaint(); });
            return;
        }
        if (u.type.contains (p))
        {
            juce::PopupMenu m; for (int t = 0; t < kRuleTypes; ++t) m.addItem (1 + t, juce::String (ruleName (t)) + "  -  " + U (ruleDesc (t)));
            m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [sp, i] (int r) { if (sp == nullptr || r <= 0) return; sp->proc.edit ([i, r] (Snap& s) { if (i < s.rules.size()) s.rules[i].type = r - 1; }); sp->repaint(); });
            return;
        }
    }
}
void Editor::mouseDrag (const juce::MouseEvent& e)
{
    const auto p = e.getMouseDownPosition().toFloat() / scaler.s;
    if (volR.contains (p)) { proc.setV ("vol", juce::jlimit (-30.0f, 6.0f, volStart + e.getDistanceFromDragStartX() / scaler.s * 0.25f - e.getDistanceFromDragStartY() / scaler.s * 0.25f)); repaint(); return; }
    if (! midiR.contains (p) || dragging || e.getDistanceFromDragStart() < 6) return;
    auto f = proc.exportMidi (8); if (! f.existsAsFile()) return;
    dragging = true; repaint();
    juce::Component::SafePointer<Editor> sp (this);
    performExternalDragDropOfFiles ({ f.getFullPathName() }, false, this, [sp] { if (sp != nullptr) { sp->dragging = false; sp->repaint(); } });
}
void Editor::filesDropped (const juce::StringArray& files, int x, int y)
{
    const auto p = juce::Point<float> ((float) x, (float) y) / scaler.s;
    for (int l = 0; l < NL; ++l) if (laneR[l].expanded (0, 4).contains (p) || juce::Rectangle<float> (40, laneR[l].getY(), 140, laneR[l].getHeight()).contains (p))
        for (auto& fn : files) if (proc.importLane (l, juce::File (fn))) { info = "Pattern caricato in " + juce::String (laneName (l)) + "."; repaint(); return; }
}
void Editor::timerCallback()
{
    if (bgTheme != skin::cur() && bgTheme >= 0) { skin::apply(); bg = {}; repaint(); }
    for (auto* k : knobs) if (k->animate()) k->repaint();
    if (flash > 0) --flash;
    // spiegazione del tipo di regola sotto il mouse
    const auto mp = getMouseXYRelative().toFloat() / scaler.s; juce::String ni;
    auto S = proc.snap();
    for (size_t i = 0; i < ruleUI.size() && i < S->rules.size(); ++i) if (ruleUI[i].type.contains (mp)) ni = juce::String (ruleName (S->rules[i].type)) + ": " + U (ruleDesc (S->rules[i].type));
    if (ni != info && (ni.isNotEmpty() || info.startsWith ("DIRADA") || info.startsWith ("RISPONDE") || info.startsWith ("SEGUE") || info.startsWith ("ROLL"))) info = ni;
    repaint();
}
}
