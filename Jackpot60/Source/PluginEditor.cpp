#include "PluginEditor.h"

namespace j60
{
juce::Font fnt (float size, bool bold, bool mono)
{
    auto o = juce::FontOptions (size);
    if (mono) o = o.withName (juce::Font::getDefaultMonospacedFontName());
    if (bold) o = o.withStyle ("Bold");
    return juce::Font (o);
}

static void screw (juce::Graphics& g, float x, float y)
{
    juce::ColourGradient cg (juce::Colour (0xfff4efe4), x - 3, y - 3, juce::Colour (0xff8f8471), x + 4, y + 4, false);
    g.setGradientFill (cg); g.fillEllipse (x - 5, y - 5, 10, 10);
    g.setColour (juce::Colour (0xff6d6252)); g.drawLine (x - 3.5f, y + 1.5f, x + 3.5f, y - 1.5f, 1.4f);
}

static void engrave (juce::Graphics& g, const juce::String& t, juce::Rectangle<int> r, float size = 11.0f, juce::Justification j = juce::Justification::centredLeft)
{
    g.setFont (fnt (size, true)); g.setColour (juce::Colours::white.withAlpha (0.55f)); g.drawText (t, r.translated (0, 1), j);
    g.setColour (C::ink); g.drawText (t, r, j);
}

// ======================= Look and feel =======================
VintageLAF::VintageLAF()
{
    setColour (juce::Slider::textBoxTextColourId, C::amber);
    setColour (juce::Slider::textBoxBackgroundColourId, C::lcdBg2);
    setColour (juce::Slider::textBoxHighlightColourId, C::amberDim);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
}
juce::Label* VintageLAF::createSliderTextBox (juce::Slider& s)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (s); l->setFont (fnt (11.0f, true, true)); l->setColour (juce::Label::textColourId, C::amber);
    l->setColour (juce::Label::backgroundColourId, C::lcdBg2); l->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
    return l;
}
void VintageLAF::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider&)
{
    const auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4);
    const float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f, cx = b.getCentreX(), cy = b.getCentreY();
    // scala incisa
    g.setColour (C::inkSoft);
    for (int i = 0; i <= 10; ++i) { const float a = a0 + (a1 - a0) * i / 10.0f; const float r1 = r - 1, r2 = r - (i % 5 == 0 ? 7.0f : 4.5f);
        g.drawLine (cx + std::sin (a) * r1, cy - std::cos (a) * r1, cx + std::sin (a) * r2, cy - std::cos (a) * r2, i % 5 == 0 ? 1.6f : 1.0f); }
    const float kr = r - 9;
    // ombra e gonna metallica
    g.setColour (juce::Colours::black.withAlpha (0.28f)); g.fillEllipse (cx - kr + 2, cy - kr + 4, kr * 2, kr * 2);
    juce::ColourGradient skirt (juce::Colour (0xffe6e1d6), cx - kr, cy - kr, juce::Colour (0xff7d756a), cx + kr, cy + kr, false);
    g.setGradientFill (skirt); g.fillEllipse (cx - kr, cy - kr, kr * 2, kr * 2);
    // corpo zigrinato
    const float br = kr * 0.78f;
    juce::ColourGradient body (juce::Colour (0xff4a4038), cx - br, cy - br, juce::Colour (0xff1d1814), cx + br, cy + br, false);
    g.setGradientFill (body); g.fillEllipse (cx - br, cy - br, br * 2, br * 2);
    g.setColour (juce::Colours::white.withAlpha (0.06f));
    for (int i = 0; i < 36; ++i) { const float a = i * juce::MathConstants<float>::twoPi / 36; g.drawLine (cx + std::sin (a) * br * 0.86f, cy - std::cos (a) * br * 0.86f, cx + std::sin (a) * br, cy - std::cos (a) * br, 1.0f); }
    g.setColour (juce::Colours::white.withAlpha (0.12f)); g.drawEllipse (cx - br * 0.7f, cy - br * 0.7f, br * 1.4f, br * 1.4f, 1.0f);
    const float a = a0 + pos * (a1 - a0);
    g.setColour (C::chassisHi);
    g.drawLine (cx + std::sin (a) * br * 0.25f, cy - std::cos (a) * br * 0.25f, cx + std::sin (a) * br * 0.92f, cy - std::cos (a) * br * 0.92f, 2.6f);
}
void VintageLAF::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float, juce::Slider::SliderStyle, juce::Slider&)
{
    const float cx = x + w * 0.5f;
    // fessura
    g.setColour (juce::Colour (0xff1e1914)); g.fillRoundedRectangle (cx - 3, (float) y + 6, 6, (float) h - 12, 3);
    g.setColour (juce::Colours::white.withAlpha (0.5f)); g.drawLine (cx + 3.5f, (float) y + 8, cx + 3.5f, (float) y + h - 8, 1.0f);
    g.setColour (C::inkSoft);
    for (int i = 0; i <= 10; ++i) { const float yy = y + 8 + (h - 16) * i / 10.0f; g.drawLine (cx - 18, yy, cx - (i % 5 == 0 ? 10.0f : 13.0f), yy, 1.0f); }
    // cappuccio
    const float cw = 34, ch = 22, cy = pos;
    g.setColour (juce::Colours::black.withAlpha (0.3f)); g.fillRoundedRectangle (cx - cw / 2 + 1, cy - ch / 2 + 3, cw, ch, 3);
    juce::ColourGradient cg (juce::Colour (0xfff6f0e2), cx, cy - ch / 2, juce::Colour (0xffb8ac94), cx, cy + ch / 2, false);
    g.setGradientFill (cg); g.fillRoundedRectangle (cx - cw / 2, cy - ch / 2, cw, ch, 3);
    g.setColour (C::orange); g.fillRect (cx - cw / 2 + 3, cy - 1.0f, cw - 6, 2.0f);
}

// ======================= Pulsante =======================
void HwButton::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (1.5f);
    const bool lit = isLit ? isLit() : false;
    g.setColour (juce::Colours::black.withAlpha (0.25f)); g.fillRoundedRectangle (b.translated (0, down ? 0.5f : 2.0f), 4);
    auto face = b.translated (0, down ? 1.5f : 0.0f);
    juce::ColourGradient cg (down ? juce::Colour (0xffcfc5b1) : juce::Colour (0xfff2ebdc), face.getX(), face.getY(), juce::Colour (0xffbdb19a), face.getX(), face.getBottom(), false);
    g.setGradientFill (cg); g.fillRoundedRectangle (face, 4);
    g.setColour (juce::Colour (0xff9c907a)); g.drawRoundedRectangle (face, 4, 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.7f)); g.drawLine (face.getX() + 4, face.getY() + 1.2f, face.getRight() - 4, face.getY() + 1.2f, 1.0f);
    auto txt = face.toNearestInt();
    if (ledColour != juce::Colours::transparentBlack)
    {
        const float lx = face.getX() + 9, ly = face.getCentreY();
        if (lit) { g.setColour (ledColour.withAlpha (0.35f)); g.fillEllipse (lx - 7, ly - 7, 14, 14); }
        g.setColour (lit ? ledColour.brighter (0.4f) : ledColour.darker (0.9f).withAlpha (0.7f)); g.fillEllipse (lx - 3.5f, ly - 3.5f, 7, 7);
        txt = txt.withTrimmedLeft (14);
    }
    g.setFont (fnt (big ? 13.0f : 11.0f, true)); g.setColour (C::ink); g.drawText (label, txt, juce::Justification::centred);
}

// ======================= ParamBox =======================
void ParamBox::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setFont (fnt (9.5f, true)); g.setColour (C::inkSoft); g.drawText (cap, b.removeFromTop (13).toNearestInt(), juce::Justification::centredLeft);
    g.setColour (C::lcdBg2); g.fillRoundedRectangle (b, 3);
    g.setColour (juce::Colours::black.withAlpha (0.4f)); g.drawRoundedRectangle (b, 3, 1.0f);
    g.setFont (fnt (15.0f, true, true)); g.setColour (C::amber); g.drawText (param->getCurrentValueAsText(), b.toNearestInt(), juce::Justification::centred);
}
void ParamBox::mouseDown (const juce::MouseEvent& e)
{
    startV = param->getValue();
    if (auto* c = dynamic_cast<juce::AudioParameterChoice*> (param))
    {
        const int n = c->choices.size(); int i = c->getIndex() + (e.mods.isRightButtonDown() || e.mods.isShiftDown() ? -1 : 1); i = (i + n) % n;
        c->beginChangeGesture(); c->setValueNotifyingHost (c->convertTo0to1 ((float) i)); c->endChangeGesture(); repaint();
    }
}
void ParamBox::mouseDrag (const juce::MouseEvent& e)
{
    if (dynamic_cast<juce::AudioParameterChoice*> (param)) return;
    const float v = juce::jlimit (0.0f, 1.0f, startV - e.getDistanceFromDragStartY() / (e.mods.isShiftDown() ? 1500.0f : 300.0f));
    param->setValueNotifyingHost (v); repaint();
}
void ParamBox::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w)
{
    if (auto* c = dynamic_cast<juce::AudioParameterChoice*> (param)) { const int n = c->choices.size(); int i = (c->getIndex() + (w.deltaY > 0 ? 1 : -1) + n) % n; c->setValueNotifyingHost (c->convertTo0to1 ((float) i)); }
    else param->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, param->getValue() + (w.deltaY > 0 ? 0.004f : -0.004f)));
    repaint();
}

// ======================= Pad =======================
void PadComp::paint (juce::Graphics& g)
{
    const int p = ed.bank * kPadsPerBank + slot;
    auto b = getLocalBounds().toFloat().reduced (3);
    const bool sel = p == ed.selPad;
    // incavo
    g.setColour (juce::Colour (0xff221d18)); g.fillRoundedRectangle (b.expanded (2), 9);
    auto face = b.reduced (held ? 2.5f : 2.0f).translated (0, held ? 1.0f : 0.0f);
    juce::ColourGradient rub (C::padHi, face.getCentreX(), face.getY(), C::padLo, face.getCentreX(), face.getBottom(), false);
    g.setGradientFill (rub); g.fillRoundedRectangle (face, 7);
    // grana della gomma
    juce::Random rr (p * 31 + 7); g.setColour (juce::Colours::black.withAlpha (0.05f));
    for (int i = 0; i < 90; ++i) g.fillRect (face.getX() + rr.nextFloat() * face.getWidth(), face.getY() + rr.nextFloat() * face.getHeight(), 1.2f, 1.2f);
    g.setColour (juce::Colours::white.withAlpha (0.35f)); g.drawRoundedRectangle (face.reduced (1.5f), 6, 1.0f);
    if (flash > 0.01f)
    {
        juce::ColourGradient glow (C::amber.withAlpha (0.85f * flash), face.getCentreX(), face.getCentreY(), C::orange.withAlpha (0.0f), face.getX(), face.getY(), true);
        g.setGradientFill (glow); g.fillRoundedRectangle (face, 7);
    }
    if (sel) { g.setColour (C::orange); g.drawRoundedRectangle (face.expanded (1.5f), 8, 2.2f); }
    g.setFont (fnt (11.0f, true, true)); g.setColour (C::ink.withAlpha (0.8f)); g.drawText (padName (p), face.reduced (8, 6).toNearestInt(), juce::Justification::topLeft);
    auto s = ed.proc.getSample (p);
    g.setFont (fnt (10.5f, false)); g.setColour (s ? C::ink : C::inkSoft.withAlpha (0.6f));
    g.drawFittedText (s ? s->name : juce::String ("vuoto"), face.reduced (8, 6).toNearestInt(), juce::Justification::bottomLeft, 2);
}
void PadComp::mouseDown (const juce::MouseEvent& e)
{
    const int p = ed.bank * kPadsPerBank + slot;
    ed.selectPad (p);
    if (ed.proc.erase)
    {
        const juce::SpinLock::ScopedLockType l (ed.proc.seqLock);
        auto& ev = ed.proc.seq[ed.proc.curSeq.load()].ev;
        ev.erase (std::remove_if (ev.begin(), ev.end(), [p] (const Event& x) { return x.pad == p; }), ev.end());
        ed.setStatus ("CANCELLATO " + padName (p));
        return;
    }
    const float y = juce::jlimit (0.0f, 1.0f, e.position.y / (float) getHeight());
    const int vel = juce::jlimit (1, 127, (int) juce::jmap (y, 0.0f, 1.0f, 127.0f, 35.0f));
    held = true; ed.proc.uiPadDown (p, vel); repaint();
}
void PadComp::mouseUp (const juce::MouseEvent&)
{
    if (! held) return; held = false;
    ed.proc.uiPadUp (ed.bank * kPadsPerBank + slot); repaint();
}

// ======================= LCD =======================
juce::Rectangle<int> LcdComp::waveArea() const { return getLocalBounds().reduced (16).withTrimmedTop (34).withWidth ((getWidth() - 32) * 56 / 100); }
juce::Rectangle<int> LcdComp::listArea() const { auto w = waveArea(); return getLocalBounds().reduced (16).withTrimmedTop (34).withTrimmedLeft (w.getWidth() + 18); }
int LcdComp::rowAt (juce::Point<int> p) const
{
    auto a = listArea(); if (! a.contains (p)) return -1;
    const int col = p.x < a.getX() + a.getWidth() / 2 ? 0 : 1, rows = 10, rh = a.getHeight() / rows;
    const int r = (p.y - a.getY()) / rh, idx = col * rows + r;
    return idx < kNumPadParams ? idx : -1;
}
static float getPP (PadParams& P, int i)
{
    switch (i) { case 0: return P.level; case 1: return P.pan; case 2: return P.tune; case 3: return P.fine; case 4: return P.start; case 5: return P.end; case 6: return P.attack; case 7: return P.decay;
        case 8: return P.release; case 9: return P.cutoff; case 10: return P.reso; case 11: return P.velSens; case 12: return (float) P.mode.load(); case 13: return (float) P.reverse.load(); case 14: return (float) P.choke.load();
        case 15: return (float) P.loop.load(); case 16: return (float) P.sync.load(); case 17: return P.bpm; case 18: return P.sendDly; default: return P.sendRev; }
}
static void setPP (PadParams& P, int i, float v)
{
    const auto& in = kPadParamInfo[i]; v = juce::jlimit (in.min, in.max, v);
    switch (i) { case 0: P.level = v; break; case 1: P.pan = v; break; case 2: P.tune = std::round (v); break; case 3: P.fine = std::round (v); break; case 4: P.start = juce::jmin (v, P.end.load() - 0.001f); break;
        case 5: P.end = juce::jmax (v, P.start.load() + 0.001f); break; case 6: P.attack = v; break; case 7: P.decay = v; break; case 8: P.release = v; break; case 9: P.cutoff = v; break;
        case 10: P.reso = v; break; case 11: P.velSens = v; break; case 12: P.mode = (int) std::round (v); break; case 13: P.reverse = (int) std::round (v); break; case 14: P.choke = (int) std::round (v); break;
        case 15: P.loop = (int) std::round (v); break; case 16: P.sync = (int) std::round (v); break; case 17: P.bpm = v < 40.0f ? 0.0f : std::round (v * 10.0f) / 10.0f; break;
        case 18: P.sendDly = v; break; default: P.sendRev = v; }
}
static juce::String fmtPP (int i, float v)
{
    switch (i)
    {
        case 0: case 10: case 11: return juce::String (juce::roundToInt (v * 100)) + "%";
        case 1: return std::abs (v) < 0.02f ? juce::String ("C") : (v < 0 ? "L" : "R") + juce::String (juce::roundToInt (std::abs (v) * 50));
        case 2: return (v > 0 ? "+" : "") + juce::String ((int) v) + " st";
        case 3: return (v > 0 ? "+" : "") + juce::String ((int) v) + " ct";
        case 4: case 5: return juce::String (v * 100.0f, 1) + "%";
        case 6: case 8: return juce::String (juce::roundToInt (v)) + " ms";
        case 7: return v >= 4990 ? juce::String ("FULL") : juce::String (juce::roundToInt (v)) + " ms";
        case 9: return v >= 19000 ? juce::String ("OPEN") : (v >= 1000 ? juce::String (v / 1000.0f, 1) + " kHz" : juce::String (juce::roundToInt (v)) + " Hz");
        case 12: return v > 0.5f ? "NOTE ON" : "ONE SHOT";
        case 13: case 15: case 16: return v > 0.5f ? "ON" : "OFF";
        case 14: return v < 0.5f ? juce::String ("OFF") : juce::String ((int) v);
        case 17: return v < 40.0f ? juce::String ("--") : juce::String (v, 1);
        default: return juce::String (juce::roundToInt (v * 100)) + "%";
    }
}
void LcdComp::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    // cornice
    g.setColour (C::darkPanel2); g.fillRoundedRectangle (b, 10);
    auto scr = b.reduced (8);
    juce::ColourGradient bg (C::lcdBg, scr.getX(), scr.getY(), C::lcdBg2, scr.getX(), scr.getBottom(), false);
    g.setGradientFill (bg); g.fillRoundedRectangle (scr, 6);
    // righe di scansione
    g.setColour (juce::Colours::black.withAlpha (0.18f)); for (float y = scr.getY(); y < scr.getBottom(); y += 3) g.drawHorizontalLine ((int) y, scr.getX(), scr.getRight());
    auto& P = ed.proc; const int p = ed.selPad; auto smp = P.getSample (p);
    // intestazione
    auto hdr = getLocalBounds().reduced (16).removeFromTop (24);
    g.setColour (C::amber.withAlpha (0.12f)); g.fillRect (hdr.toFloat());
    g.setFont (fnt (13.0f, true, true)); g.setColour (C::amber);
    const double tk = P.uiTick.load(); const int bar = (int) (tk / kTicksPerBar) + 1, beat = (int) std::fmod (tk / PPQ, 4.0) + 1, tt = (int) std::fmod (tk, (double) PPQ);
    int bars; { const juce::SpinLock::ScopedLockType l (P.seqLock); bars = P.seq[P.curSeq.load()].bars; }
    juce::String h; h << "SEQ " << juce::String (P.curSeq.load() + 1).paddedLeft ('0', 2) << "  " << bars << " BAR  "
                      << juce::String (bar).paddedLeft ('0', 3) << "." << beat << "." << juce::String (tt).paddedLeft ('0', 2) << "  "
                      << juce::String (P.uiBpm.load(), 1) << " BPM";
    g.drawText (h, hdr.reduced (8, 0), juce::Justification::centredLeft);
    juce::String st = P.recording ? "REC" : (P.uiRunning ? "PLAY" : "STOP");
    g.setColour (P.recording ? juce::Colour (0xffff6a4a) : C::amber); g.drawText (st + (P.syncHost ? "  HOST" : "  INT"), hdr.reduced (8, 0), juce::Justification::centredRight);
    // forma d'onda
    auto wa = waveArea(); auto wf = wa.toFloat();
    g.setColour (C::amber.withAlpha (0.25f)); g.drawRect (wf, 1.0f);
    g.setFont (fnt (11.0f, true, true)); g.setColour (C::amber);
    auto wh = wa.removeFromTop (18).reduced (6, 0);
    g.drawText ("PAD " + padName (p) + "  " + (smp ? smp->name.toUpperCase() : juce::String ("VUOTO")), wh, juce::Justification::centredLeft);
    if (smp)
    {
        juce::String inf = juce::String (smp->buf.getNumSamples() / smp->sr, 2) + "s";
        const float pb = P.pad[p].bpm.load();
        if (pb > 40.0f) inf << "  " << juce::String (pb, 1) << (P.pad[p].sync.load() ? juce::String (" > ") + juce::String (P.uiBpm.load(), 1) : juce::String (" BPM"));
        if (P.pad[p].loop.load()) inf << "  LOOP";
        g.setFont (fnt (10.0f, true, true)); g.setColour (P.pad[p].sync.load() ? C::amber : C::amberDim.brighter (0.3f)); g.drawText (inf, wh, juce::Justification::centredRight);
    }
    auto plot = wf.withTrimmedTop (20).reduced (4, 6);
    if (smp)
    {
        const int n = smp->buf.getNumSamples(); const float* d = smp->buf.getReadPointer (0); const int W = (int) plot.getWidth();
        const float s0 = P.pad[p].start, s1 = P.pad[p].end;
        for (int x = 0; x < W; ++x)
        {
            const int a = (int) ((double) x / W * n), e = juce::jmin (n, (int) ((double) (x + 1) / W * n) + 1);
            float mx = 0; for (int i = a; i < e; i += 4) mx = juce::jmax (mx, std::abs (d[i]));
            const float fx = (float) x / W; const bool in = fx >= s0 && fx <= s1;
            g.setColour (in ? C::amber : C::amberDim.withAlpha (0.5f));
            g.drawVerticalLine ((int) plot.getX() + x, plot.getCentreY() - mx * plot.getHeight() * 0.48f, plot.getCentreY() + mx * plot.getHeight() * 0.48f + 1);
        }
        for (int k = 0; k < 2; ++k)
        {
            const float x = plot.getX() + (k ? s1 : s0) * plot.getWidth();
            g.setColour (juce::Colour (0xffffe0a8)); g.drawLine (x, plot.getY() - 2, x, plot.getBottom() + 2, 1.5f);
            juce::Path tri; tri.addTriangle (x - 5, plot.getY() - 2, x + 5, plot.getY() - 2, x, plot.getY() + 6); g.fillPath (tri);
            g.setFont (fnt (9.0f, true, true)); g.drawText (k ? "E" : "S", juce::Rectangle<float> (x + (k ? -14.0f : 3.0f), plot.getBottom() - 12, 12, 12), juce::Justification::centred);
        }
    }
    else
    {
        g.setFont (fnt (12.0f, false, true)); g.setColour (C::amberDim);
        g.drawFittedText ("TRASCINA QUI UN FILE AUDIO\nO PREMI LOAD", plot.toNearestInt(), juce::Justification::centred, 2);
    }
    // parametri del pad
    auto la = listArea(); const int rows = 10, rh = la.getHeight() / rows, cw = la.getWidth() / 2;
    for (int i = 0; i < kNumPadParams; ++i)
    {
        const int col = i / rows, r = i % rows; juce::Rectangle<int> rc (la.getX() + col * cw, la.getY() + r * rh, cw - 6, rh);
        if (i == dragRow) { g.setColour (C::amber.withAlpha (0.18f)); g.fillRect (rc); }
        g.setFont (fnt (11.0f, false, true)); g.setColour (C::amberDim.brighter (0.6f)); g.drawText (kPadParamInfo[i].label, rc.reduced (4, 0), juce::Justification::centredLeft);
        g.setFont (fnt (12.0f, true, true)); g.setColour (C::amber); g.drawText (fmtPP (i, getPP (P.pad[p], i)), rc.reduced (4, 0), juce::Justification::centredRight);
    }
    // stato temporaneo
    if (ed.status.isNotEmpty() && juce::Time::getMillisecondCounter() - ed.statusTime < 1800)
    {
        auto sb = getLocalBounds().reduced (16).removeFromBottom (22);
        g.setColour (C::amber); g.fillRect (sb); g.setColour (C::lcdBg2); g.setFont (fnt (12.0f, true, true)); g.drawText (ed.status, sb, juce::Justification::centred);
    }
    // vetro
    juce::ColourGradient gl (juce::Colours::white.withAlpha (0.07f), scr.getX(), scr.getY(), juce::Colours::transparentWhite, scr.getX(), scr.getY() + scr.getHeight() * 0.45f, false);
    g.setGradientFill (gl); g.fillRoundedRectangle (scr, 6);
}
void LcdComp::mouseDown (const juce::MouseEvent& e)
{
    downPos = e.getPosition(); auto& P = ed.proc.pad[ed.selPad];
    auto plot = waveArea().toFloat().withTrimmedTop (20).reduced (4, 6);
    if (plot.expanded (6).contains (e.position))
    {
        const float fx = (e.position.x - plot.getX()) / plot.getWidth();
        dragMarker = std::abs (fx - P.start.load()) < std::abs (fx - P.end.load()) ? 0 : 1;
        setPP (P, dragMarker ? 5 : 4, juce::jlimit (0.0f, 1.0f, fx)); repaint(); return;
    }
    dragRow = rowAt (e.getPosition());
    if (dragRow >= 0)
    {
        const auto& in = kPadParamInfo[dragRow];
        if (in.kind == 2) { setPP (P, dragRow, getPP (P, dragRow) > 0.5f ? 0.0f : 1.0f); dragRow = -1; }
        else dragStart = getPP (P, dragRow);
        ed.refreshQLink(); repaint();
    }
}
void LcdComp::mouseDrag (const juce::MouseEvent& e)
{
    auto& P = ed.proc.pad[ed.selPad];
    if (dragMarker >= 0)
    {
        auto plot = waveArea().toFloat().withTrimmedTop (20).reduced (4, 6);
        setPP (P, dragMarker ? 5 : 4, juce::jlimit (0.0f, 1.0f, (e.position.x - plot.getX()) / plot.getWidth())); repaint(); return;
    }
    if (dragRow < 0) return;
    const auto& in = kPadParamInfo[dragRow]; const float dy = (float) -e.getDistanceFromDragStartY() / (e.mods.isShiftDown() ? 900.0f : 180.0f);
    float v;
    if (in.kind == 3) v = (float) (std::exp (std::log (juce::jmax (80.0f, dragStart)) + dy * std::log (20000.0 / 80.0)));
    else if (in.kind == 4) v = (float) (std::exp (std::log (juce::jmax (1.0f, dragStart)) + dy * 7.0));
    else v = dragStart + dy * (in.max - in.min);
    setPP (P, dragRow, v); ed.refreshQLink(); repaint();
}
void LcdComp::mouseDoubleClick (const juce::MouseEvent& e)
{
    const int r = rowAt (e.getPosition()); if (r < 0) return;
    setPP (ed.proc.pad[ed.selPad], r, kPadParamInfo[r].def); ed.refreshQLink(); repaint();
}
void LcdComp::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    const int r = rowAt (e.getPosition()); if (r < 0) return; auto& P = ed.proc.pad[ed.selPad]; const auto& in = kPadParamInfo[r];
    const float step = (in.kind == 1 || in.kind == 5 || in.kind == 2) ? 1.0f : (in.max - in.min) / 100.0f;
    setPP (P, r, getPP (P, r) + (w.deltaY > 0 ? step : -step)); ed.refreshQLink(); repaint();
}

// ======================= Griglia =======================
juce::Rectangle<float> GridComp::cellArea() const { return getLocalBounds().toFloat().reduced (10, 8).withTrimmedLeft (54).withTrimmedTop (18); }
void GridComp::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (C::darkPanel2); g.fillRoundedRectangle (b, 10);
    g.setColour (C::lcdBg2); g.fillRoundedRectangle (b.reduced (5), 7);
    auto& P = ed.proc; int bars; std::vector<Event> ev;
    { const juce::SpinLock::ScopedLockType l (P.seqLock); bars = P.seq[P.curSeq.load()].bars; ev = P.seq[P.curSeq.load()].ev; }
    const int pagesN = (bars + 1) / 2; page = juce::jlimit (0, pagesN - 1, page);
    const int barsOnPage = juce::jmin (2, bars - page * 2), steps = barsOnPage * 16, t0 = page * 2 * kTicksPerBar;
    auto ca = cellArea(); const float cw = ca.getWidth() / 32.0f, rh = ca.getHeight() / 16.0f;
    g.setFont (fnt (10.0f, true, true));
    for (int bIdx = 0; bIdx < barsOnPage; ++bIdx) { g.setColour (C::amber); g.drawText ("BAR " + juce::String (page * 2 + bIdx + 1), juce::Rectangle<float> (ca.getX() + bIdx * 16 * cw + 2, ca.getY() - 16, 80, 14), juce::Justification::centredLeft); }
    g.setColour (C::amberDim); g.drawText ("PAGINA " + juce::String (page + 1) + "/" + juce::String (pagesN), juce::Rectangle<float> (ca.getRight() - 170, ca.getY() - 16, 90, 14), juce::Justification::centredRight);
    for (int r = 0; r < 16; ++r)
    {
        const int pad = ed.bank * kPadsPerBank + r; const float y = ca.getBottom() - (r + 1) * rh;
        g.setColour (pad == ed.selPad ? C::amber : C::amberDim.brighter (0.4f)); g.setFont (fnt (10.0f, pad == ed.selPad, true));
        g.drawText (padName (pad), juce::Rectangle<float> (ca.getX() - 50, y, 46, rh), juce::Justification::centredRight);
        for (int s = 0; s < 32; ++s)
        {
            auto c = juce::Rectangle<float> (ca.getX() + s * cw, y, cw, rh).reduced (1.0f);
            if (s >= steps) { g.setColour (juce::Colours::black.withAlpha (0.35f)); g.fillRect (c); continue; }
            g.setColour (s % 4 == 0 ? C::amber.withAlpha (0.10f) : C::amber.withAlpha (0.05f)); g.fillRect (c);
        }
    }
    for (int s = 0; s <= 32; s += 4) { g.setColour (C::amber.withAlpha (s % 16 == 0 ? 0.35f : 0.12f)); g.drawVerticalLine ((int) (ca.getX() + s * cw), ca.getY(), ca.getBottom()); }
    // eventi
    for (const auto& e : ev)
    {
        const int r = e.pad - ed.bank * kPadsPerBank; if (r < 0 || r >= 16) continue;
        const int rel = e.tick - t0; if (rel < 0 || rel >= steps * kTick16) continue;
        const float x = ca.getX() + rel / (float) kTick16 * cw, y = ca.getBottom() - (r + 1) * rh;
        const float v = e.vel / 127.0f;
        auto c = juce::Rectangle<float> (x, y, cw, rh).reduced (1.5f);
        g.setColour (C::amber.withAlpha (0.25f + 0.75f * v)); g.fillRoundedRectangle (c, 2);
        g.setColour (C::lcdBg2.withAlpha (0.8f)); g.fillRect (c.withTrimmedTop (c.getHeight() * v).withHeight (1.5f).withY (c.getBottom() - c.getHeight() * v));
    }
    // testina
    if (P.uiRunning)
    {
        const double tk = P.uiTick.load() - t0;
        if (tk >= 0 && tk < steps * kTick16) { const float x = ca.getX() + (float) (tk / kTick16) * cw; g.setColour (juce::Colour (0xffffe0a8)); g.drawLine (x, ca.getY(), x, ca.getBottom(), 1.6f); }
    }
}
void GridComp::mouseDown (const juce::MouseEvent& e)
{
    auto ca = cellArea(); if (! ca.contains (e.position)) return;
    const float cw = ca.getWidth() / 32.0f, rh = ca.getHeight() / 16.0f;
    const int s = (int) ((e.position.x - ca.getX()) / cw), r = 15 - (int) ((e.position.y - ca.getY()) / rh);
    auto& P = ed.proc; const int pad = ed.bank * kPadsPerBank + juce::jlimit (0, 15, r);
    const int tick = page * 2 * kTicksPerBar + s * kTick16;
    ed.selectPad (pad);
    const juce::SpinLock::ScopedLockType l (P.seqLock);
    auto& sq = P.seq[P.curSeq.load()];
    if (s >= juce::jmin (2, sq.bars - page * 2) * 16) return;
    auto it = std::find_if (sq.ev.begin(), sq.ev.end(), [&] (const Event& x) { return x.pad == pad && x.tick >= tick && x.tick < tick + kTick16; });
    if (e.mods.isShiftDown() && it != sq.ev.end()) { velEdit = 1; velTick = it->tick; velPad = pad; velStart = it->vel; paintMode = false; return; }
    velEdit = -1; paintMode = true;
    if (it != sq.ev.end()) { paintVal = false; sq.ev.erase (std::remove_if (sq.ev.begin(), sq.ev.end(), [&] (const Event& x) { return x.pad == pad && x.tick >= tick && x.tick < tick + kTick16; }), sq.ev.end()); }
    else { paintVal = true; sq.ev.push_back ({ tick, pad, P.fullLevel ? 127 : 100 }); }
    repaint();
}
void GridComp::mouseDrag (const juce::MouseEvent& e)
{
    auto& P = ed.proc; const juce::SpinLock::ScopedLockType l (P.seqLock); auto& sq = P.seq[P.curSeq.load()];
    if (velEdit > 0)
    {
        for (auto& x : sq.ev) if (x.pad == velPad && x.tick == velTick) x.vel = juce::jlimit (1, 127, velStart - e.getDistanceFromDragStartY());
        repaint(); return;
    }
    if (! paintMode) return;
    auto ca = cellArea(); if (! ca.contains (e.position)) return;
    const float cw = ca.getWidth() / 32.0f, rh = ca.getHeight() / 16.0f;
    const int s = (int) ((e.position.x - ca.getX()) / cw), r = 15 - (int) ((e.position.y - ca.getY()) / rh);
    const int pad = ed.bank * kPadsPerBank + juce::jlimit (0, 15, r), tick = page * 2 * kTicksPerBar + s * kTick16;
    if (s >= juce::jmin (2, sq.bars - page * 2) * 16) return;
    const bool has = std::any_of (sq.ev.begin(), sq.ev.end(), [&] (const Event& x) { return x.pad == pad && x.tick >= tick && x.tick < tick + kTick16; });
    if (paintVal && ! has) sq.ev.push_back ({ tick, pad, P.fullLevel ? 127 : 100 });
    if (! paintVal && has) sq.ev.erase (std::remove_if (sq.ev.begin(), sq.ev.end(), [&] (const Event& x) { return x.pad == pad && x.tick >= tick && x.tick < tick + kTick16; }), sq.ev.end());
    repaint();
}

// ======================= FX rack =======================
FxRack::FxRack (Processor& p) : proc (p)
{
    struct K { const char* id; const char* lab; int mod; };
    const K ks[] = { { "bComp", "COMP", 0 }, { "bTrans", "TRANSIENT", 0 }, { "bCrunch", "CRUNCH", 0 }, { "bBoom", "BOOM", 0 }, { "bFreq", "FREQ", 0 }, { "bDecay", "DECAY", 0 }, { "bDamp", "DAMP", 0 },
                     { "dFb", "FEEDBACK", 1 }, { "dTone", "TONE", 1 }, { "dRet", "RETURN", 1 },
                     { "rSize", "SIZE", 2 }, { "rDamp", "DAMP", 2 }, { "rRet", "RETURN", 2 } };
    for (auto& k : ks)
    {
        auto* s = knobs.add (new juce::Slider());
        s->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag); s->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 58, 15);
        s->setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
        s->setColour (juce::Slider::textBoxBackgroundColourId, C::lcdBg2); s->setColour (juce::Slider::textBoxTextColourId, C::amber);
        s->setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        addAndMakeVisible (s); atts.add (new juce::AudioProcessorValueTreeState::SliderAttachment (proc.apvts, k.id, *s));
        labels.add (k.lab); mods.add (k.mod);
    }
    knobs[1]->textFromValueFunction = [] (double v) { return std::abs (v) < 0.01 ? juce::String ("0") : (v > 0 ? "+" : "") + juce::String (juce::roundToInt (v * 100)); };
    knobs[4]->textFromValueFunction = [] (double v) { return juce::String (v, 1) + " Hz"; };
    knobs[6]->textFromValueFunction = [] (double v) { return v >= 19000 ? juce::String ("OFF") : juce::String (v / 1000.0, 1) + "k"; };
    for (int i : { 0, 2, 3, 5, 7, 8, 9, 10, 11, 12 }) knobs[i]->textFromValueFunction = [] (double v) { return juce::String (juce::roundToInt (v * 100)) + "%"; };
    for (auto* k : knobs) k->updateText();
    dTime = std::make_unique<ParamBox> (proc.apvts.getParameter ("dTime"), "TIME"); addAndMakeVisible (*dTime);
}
juce::Rectangle<int> FxRack::module (int m) const
{
    auto b = getLocalBounds().reduced (10, 10);
    const int w0 = 360, w1 = 176;
    if (m == 0) return b.removeFromLeft (w0);
    b.removeFromLeft (w0 + 8);
    if (m == 1) return b.removeFromLeft (w1);
    b.removeFromLeft (w1 + 8);
    return b;
}
void FxRack::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (C::darkPanel2); g.fillRoundedRectangle (b, 10);
    const char* names[] = { "DRUM BUS", "DELAY  \xc2\xb7  MANDATA", "REVERB  \xc2\xb7  MANDATA" };
    const juce::Colour accents[] = { C::orange, C::amber, juce::Colour (0xffe8a93a) };
    for (int m = 0; m < 3; ++m)
    {
        auto r = module (m).toFloat();
        juce::ColourGradient cg (C::darkPanel, r.getX(), r.getY(), C::darkPanel2.darker (0.2f), r.getX(), r.getBottom(), false);
        g.setGradientFill (cg); g.fillRoundedRectangle (r, 7);
        g.setColour (juce::Colours::white.withAlpha (0.05f)); g.drawRoundedRectangle (r, 7, 1.0f);
        g.setColour (accents[m]); g.fillRoundedRectangle (r.getX() + 10, r.getY() + 12, 4, 14, 2);
        g.setFont (fnt (12.0f, true)); g.setColour (C::chassisHi); g.drawText (juce::String::fromUTF8 (names[m]), r.toNearestInt().withTrimmedLeft (20).removeFromTop (38), juce::Justification::centredLeft);
    }
    g.setFont (fnt (9.5f, true));
    for (int i = 0; i < knobs.size(); ++i)
    {
        auto kb = knobs[i]->getBounds();
        g.setColour (C::chassisHi.withAlpha (0.6f)); g.drawText (labels[i], kb.withY (kb.getY() - 15).withHeight (14), juce::Justification::centred);
    }
    auto m0 = module (0);
    juce::ignoreUnused (m0);
}
void FxRack::resized()
{
    auto place = [this] (int idx, juce::Rectangle<int> r) { knobs[idx]->setBounds (r); };
    auto m0 = module (0); const int kw = 56, kh = 78;
    // drum bus: 4 sopra, 3 sotto
    for (int i = 0; i < 4; ++i) place (i, { m0.getX() + 8 + i * 87, m0.getY() + 50, 76, kh });
    for (int i = 0; i < 3; ++i) place (4 + i, { m0.getX() + 51 + i * 87, m0.getY() + 148, 76, kh });
    auto m1 = module (1);
    dTime->setBounds (m1.getX() + 16, m1.getY() + 54, 64, 42);
    place (7, { m1.getX() + 94, m1.getY() + 50, 72, kh });
    place (8, { m1.getX() + 12, m1.getY() + 148, 72, kh }); place (9, { m1.getX() + 94, m1.getY() + 148, 72, kh });
    auto m2 = module (2);
    place (10, { m2.getX() + 10, m2.getY() + 50, 72, kh }); place (11, { m2.getX() + 90, m2.getY() + 50, 72, kh });
    place (12, { m2.getX() + 50, m2.getY() + 148, 72, kh });
}

// ======================= Drag MIDI =======================
void DragMidi::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (1.5f);
    juce::ColourGradient cg (C::orange.brighter (0.25f), b.getX(), b.getY(), C::orange.darker (0.25f), b.getX(), b.getBottom(), false);
    g.setGradientFill (cg); g.fillRoundedRectangle (b, 4);
    g.setColour (juce::Colours::white); g.setFont (fnt (11.0f, true)); g.drawText (dragging ? "RILASCIA IN ABLETON" : "DRAG MIDI", b.toNearestInt(), juce::Justification::centred);
}
void DragMidi::mouseDrag (const juce::MouseEvent& e)
{
    if (dragging || e.getDistanceFromDragStart() < 6) return;
    dragging = true; repaint();
    const auto f = ed.proc.writeMidiFile();
    juce::DragAndDropContainer::performExternalDragDropOfFiles ({ f.getFullPathName() }, false, this, [this] { dragging = false; repaint(); });
}

// ======================= Editor =======================
Editor::Editor (Processor& p) : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&laf);
    setWantsKeyboardFocus (true);
    lcd = std::make_unique<LcdComp> (*this); addAndMakeVisible (*lcd);
    grid = std::make_unique<GridComp> (*this); addAndMakeVisible (*grid);
    dragMidi = std::make_unique<DragMidi> (*this); addAndMakeVisible (*dragMidi);
    fxRack = std::make_unique<FxRack> (proc); addChildComponent (*fxRack);
    for (int i = 0; i < 16; ++i) addAndMakeVisible (pads.add (new PadComp (*this, i)));

    auto mk = [this] (const juce::String& t, juce::Colour led = juce::Colours::transparentBlack) { auto* b = buttons.add (new HwButton (t, led)); addAndMakeVisible (b); return b; };
    // tasti funzione sotto il display
    auto* fLoad = mk ("LOAD"); fLoad->onClick = [this] { loadDialog(); };
    auto* fC8 = mk ("CHOP 8"); fC8->onClick = [this] { proc.chopPad (selPad, 8, false); bank = (selPad / 16 + 1) % 4; setStatus ("CHOP 8 -> BANCO " + juce::String::charToString ((juce::juce_wchar) ('A' + bank))); repaint(); };
    auto* fC16 = mk ("CHOP 16"); fC16->onClick = [this] { proc.chopPad (selPad, 16, false); bank = (selPad / 16 + 1) % 4; setStatus ("CHOP 16 -> BANCO " + juce::String::charToString ((juce::juce_wchar) ('A' + bank))); repaint(); };
    auto* fCA = mk ("CHOP AUTO"); fCA->onClick = [this] { proc.chopPad (selPad, 16, true); bank = (selPad / 16 + 1) % 4; setStatus ("CHOP SUI TRANSIENTI -> BANCO " + juce::String::charToString ((juce::juce_wchar) ('A' + bank))); repaint(); };
    auto* fClr = mk ("CLEAR PAD"); fClr->onClick = [this] { proc.clearPad (selPad); setStatus ("PAD " + padName (selPad) + " SVUOTATO"); repaint(); };
    bResample = mk ("RESAMPLE", C::red);
    bResample->isLit = [this] { const int st = proc.rsState.load(); return st == 2 || (st == 1 && (juce::Time::getMillisecondCounter() / 250) % 2 == 0); };
    bResample->onClick = [this] { proc.toggleResample(); const int st = proc.rsState.load();
        setStatus (st == 1 ? "RESAMPLE: PARTE ALLA PROSSIMA BATTUTA" : st == 0 ? "RESAMPLE ANNULLATO" : "RESAMPLE: STOP"); };
    bFx = mk ("FX RACK", C::amber); bFx->isLit = [this] { return fxRack->isVisible(); };
    bFx->onClick = [this] { const bool show = ! fxRack->isVisible(); fxRack->setVisible (show); grid->setVisible (! show); bPgDn->setVisible (! show); bPgUp->setVisible (! show); };
    // trasporto
    bRec = mk ("REC", C::red); bRec->big = true; bRec->isLit = [this] { return proc.recording.load(); }; bRec->onClick = [this] { proc.recording = ! proc.recording; };
    bStop = mk ("STOP"); bStop->big = true; bStop->onClick = [this] { proc.requestStop(); proc.recording = false; };
    bPlay = mk ("PLAY", C::green); bPlay->big = true; bPlay->isLit = [this] { return proc.uiRunning.load(); }; bPlay->onClick = [this] { proc.requestPlay(); };
    bStart = mk ("PLAY START"); bStart->big = true; bStart->onClick = [this] { proc.requestPlayStart(); };
    bSeqDn = mk ("<"); bSeqDn->onClick = [this] { proc.curSeq = (proc.curSeq.load() + kSeqs - 1) % kSeqs; };
    bSeqUp = mk (">"); bSeqUp->onClick = [this] { proc.curSeq = (proc.curSeq.load() + 1) % kSeqs; };
    bBarDn = mk ("-"); bBarDn->onClick = [this] { const juce::SpinLock::ScopedLockType l (proc.seqLock); auto& s = proc.seq[proc.curSeq.load()]; s.bars = juce::jmax (1, s.bars - 1); };
    bBarUp = mk ("+"); bBarUp->onClick = [this] { const juce::SpinLock::ScopedLockType l (proc.seqLock); auto& s = proc.seq[proc.curSeq.load()]; s.bars = juce::jmin (kMaxBars, s.bars + 1); };
    bPgDn = mk ("<"); bPgDn->onClick = [this] { grid->page = juce::jmax (0, grid->page - 1); };
    bPgUp = mk (">"); bPgUp->onClick = [this] { grid->page += 1; };
    bSync = mk ("HOST", C::green); bSync->isLit = [this] { return proc.syncHost.load(); }; bSync->onClick = [this] { proc.syncHost = ! proc.syncHost; };
    bClick = mk ("CLICK", C::amber); bClick->isLit = [this] { return *proc.apvts.getRawParameterValue ("click") > 0.5f; };
    bClick->onClick = [this] { auto* pr = proc.apvts.getParameter ("click"); pr->setValueNotifyingHost (pr->getValue() > 0.5f ? 0.0f : 1.0f); };
    // banchi e funzioni dei pad
    for (int i = 0; i < 4; ++i)
    {
        bBank[i] = mk ("BANK " + juce::String::charToString ((juce::juce_wchar) ('A' + i)), C::amber);
        bBank[i]->isLit = [this, i] { return bank == i; };
        bBank[i]->onClick = [this, i] { bank = i; selectPad (i * 16 + selPad % 16); };
    }
    bRepeat = mk ("NOTE REPEAT", C::amber); bRepeat->isLit = [this] { return proc.noteRepeat.load(); }; bRepeat->onClick = [this] { proc.noteRepeat = ! proc.noteRepeat; };
    bFull = mk ("FULL LEVEL", C::amber); bFull->isLit = [this] { return proc.fullLevel.load(); }; bFull->onClick = [this] { proc.fullLevel = ! proc.fullLevel; };
    bErase = mk ("ERASE", C::red); bErase->isLit = [this] { return proc.erase.load(); }; bErase->onClick = [this] { proc.erase = ! proc.erase; setStatus (proc.erase ? "ERASE: TOCCA UN PAD PER CANCELLARLO" : ""); };

    pbBpm = std::make_unique<ParamBox> (proc.apvts.getParameter ("bpm"), "BPM (INT)"); addAndMakeVisible (*pbBpm);
    pbTc = std::make_unique<ParamBox> (proc.apvts.getParameter ("tc"), "TIMING"); addAndMakeVisible (*pbTc);
    pbRpt = std::make_unique<ParamBox> (proc.apvts.getParameter ("rpt"), ""); addAndMakeVisible (*pbRpt);
    pbVin = std::make_unique<ParamBox> (proc.apvts.getParameter ("vintage"), "VINTAGE"); addAndMakeVisible (*pbVin);

    auto knob = [this] (juce::Slider& s, const char* id, std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>& a)
    {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag); s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 16);
        s.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
        addAndMakeVisible (s); a = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, s);
    };
    auto styleBox = [] (juce::Slider& s)
    {
        s.setColour (juce::Slider::textBoxBackgroundColourId, C::lcdBg2); s.setColour (juce::Slider::textBoxTextColourId, C::amber);
        s.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    };
    for (auto* s : { &swing, &drive, &tone, &out, &qTune, &qDecay, &qFilter, &qLevel }) styleBox (*s);
    knob (swing, "swing", aSwing); swing.setTextValueSuffix ("%");
    knob (drive, "drive", aDrive); knob (tone, "tone", aTone); knob (out, "out", aOut); out.setTextValueSuffix (" dB");
    for (auto* k : { &drive, &tone, &out }) k->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 54, 16);
    auto fader = [this] (juce::Slider& s, double mn, double mx, std::function<void (double)> fn)
    {
        s.setSliderStyle (juce::Slider::LinearVertical); s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 64, 16); s.setRange (mn, mx);
        s.onValueChange = [&s, fn] { fn (s.getValue()); }; addAndMakeVisible (s);
    };
    fader (qTune, -36, 36, [this] (double v) { proc.pad[selPad].tune = (float) std::round (v); lcd->repaint(); }); qTune.setNumDecimalPlacesToDisplay (0);
    fader (qDecay, 10, 5000, [this] (double v) { proc.pad[selPad].decay = (float) v; lcd->repaint(); }); qDecay.setSkewFactorFromMidPoint (400); qDecay.setNumDecimalPlacesToDisplay (0);
    fader (qFilter, 80, 20000, [this] (double v) { proc.pad[selPad].cutoff = (float) v; lcd->repaint(); }); qFilter.setSkewFactorFromMidPoint (1500); qFilter.setNumDecimalPlacesToDisplay (0);
    fader (qLevel, 0, 1, [this] (double v) { proc.pad[selPad].level = (float) v; lcd->repaint(); }); qLevel.setNumDecimalPlacesToDisplay (2);

    setSize (1200, 820);
    refreshQLink();
    startTimerHz (30);
}
Editor::~Editor() { setLookAndFeel (nullptr); }

void Editor::selectPad (int p) { selPad = p; refreshQLink(); repaint(); }
void Editor::refreshQLink()
{
    auto& P = proc.pad[selPad];
    qTune.setValue (P.tune.load(), juce::dontSendNotification); qDecay.setValue (P.decay.load(), juce::dontSendNotification);
    qFilter.setValue (P.cutoff.load(), juce::dontSendNotification); qLevel.setValue (P.level.load(), juce::dontSendNotification);
}

void Editor::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    juce::ColourGradient body (C::chassisHi, 0, 0, C::chassisLo, 0, b.getHeight(), false);
    g.setGradientFill (body); g.fillAll();
    // grana della plastica
    juce::Random rr (99); g.setColour (juce::Colours::black.withAlpha (0.025f));
    for (int i = 0; i < 2500; ++i) g.fillRect (rr.nextFloat() * b.getWidth(), rr.nextFloat() * b.getHeight(), 1.0f, 1.0f);
    // fascia superiore
    auto top = b.removeFromTop (62);
    juce::ColourGradient tg (juce::Colour (0xff4a4036), 0, 0, juce::Colour (0xff2e2721), 0, 62, false); g.setGradientFill (tg); g.fillRect (top);
    g.setColour (C::orange); g.fillRect (top.removeFromBottom (4));
    g.setFont (fnt (30.0f, true)); g.setColour (C::chassisHi); g.drawText ("JACKPOT", juce::Rectangle<int> (26, 10, 200, 40), juce::Justification::centredLeft);
    g.setColour (C::orange); g.drawText ("60", juce::Rectangle<int> (166, 10, 60, 40), juce::Justification::centredLeft);
    g.setFont (fnt (11.0f, true)); g.setColour (C::chassisHi.withAlpha (0.65f));
    g.drawText (juce::String::fromUTF8 ("MIDI PRODUCTION CENTER  \xc2\xb7  64 PAD SAMPLER  \xc2\xb7  12-BIT CHARACTER"), juce::Rectangle<int> (232, 18, 520, 24), juce::Justification::centredLeft);
    // strisce colorate vintage
    const juce::Colour stripes[] = { juce::Colour (0xffe8a93a), juce::Colour (0xffd4602c), juce::Colour (0xffb73a2c) };
    for (int i = 0; i < 3; ++i) { g.setColour (stripes[i]); g.fillRect (getWidth() - 160 + i * 16, 12, 10, 36); }
    for (auto pt : { juce::Point<float> (12, 76), juce::Point<float> ((float) getWidth() - 12, 76), juce::Point<float> (12, (float) getHeight() - 12), juce::Point<float> ((float) getWidth() - 12, (float) getHeight() - 12) }) screw (g, pt.x, pt.y);
    // incavo dei pad
    auto padWell = juce::Rectangle<float> (782, 80, 400, 400);
    g.setColour (juce::Colour (0xff4a4036)); g.fillRoundedRectangle (padWell.expanded (8), 12);
    g.setColour (juce::Colours::black.withAlpha (0.35f)); g.drawRoundedRectangle (padWell.expanded (8), 12, 2.0f);
    // etichette incise
    engrave (g, "SEQUENCE", { 356, 694, 90, 14 }, 10.0f); engrave (g, "BARS", { 452, 694, 60, 14 }, 10.0f);
    engrave (g, juce::String::fromUTF8 ("Q-LINK  \xc2\xb7  PAD SELEZIONATO"), { 784, 556, 220, 14 }, 10.0f);
    engrave (g, "MASTER", { 1010, 556, 100, 14 }, 10.0f);
    const char* q[] = { "TUNE", "DECAY", "FILTER", "LEVEL" };
    for (int i = 0; i < 4; ++i) engrave (g, q[i], { 784 + i * 52, 576, 48, 14 }, 9.5f, juce::Justification::centred);
    const char* mk[] = { "SWING", "DRIVE", "TONE", "OUTPUT" };
    const juce::Rectangle<int> mr[] = { { 692, 694, 72, 14 }, { 1008, 636, 60, 14 }, { 1068, 636, 60, 14 }, { 1128, 636, 60, 14 } };
    for (int i = 0; i < 4; ++i) engrave (g, mk[i], mr[i], 9.5f, juce::Justification::centred);
    engrave (g, "SYNC", { 1010, 752, 100, 14 }, 9.5f);
    // separatore trasporto
    g.setColour (C::chassisLine); g.drawHorizontalLine (688, 20, 764);
    g.setFont (fnt (12.0f, true, true)); g.setColour (C::ink);
    g.drawText (juce::String (proc.curSeq.load() + 1).paddedLeft ('0', 2), juce::Rectangle<int> (382, 716, 30, 30), juce::Justification::centred);
    int bars; { const juce::SpinLock::ScopedLockType l (proc.seqLock); bars = proc.seq[proc.curSeq.load()].bars; }
    g.drawText (juce::String (bars), juce::Rectangle<int> (478, 716, 26, 30), juce::Justification::centred);
    g.setColour (C::inkSoft); g.setFont (fnt (10.0f));
    g.drawText (juce::String::fromUTF8 ("TASTIERA  1234 / QWER / ASDF / ZXCV = PAD  \xc2\xb7  SPAZIO = PLAY/STOP  \xc2\xb7  TRASCINA FILE AUDIO SUI PAD  \xc2\xb7  SHIFT+DRAG SULLA GRIGLIA = VELOCITY"), juce::Rectangle<int> (26, getHeight() - 24, 1000, 16), juce::Justification::centredLeft);
}

void Editor::resized()
{
    lcd->setBounds (20, 78, 744, 300);
    for (int i = 0; i < 7; ++i) buttons[i]->setBounds (20 + i * 93, 386, 87, 32);
    dragMidi->setBounds (20 + 7 * 93, 386, 93, 32);
    grid->setBounds (20, 426, 744, 254);
    fxRack->setBounds (20, 426, 744, 254);
    bPgDn->setBounds (702, 432, 26, 18); bPgUp->setBounds (732, 432, 26, 18);
    // trasporto
    bRec->setBounds (20, 694, 72, 58); bStop->setBounds (96, 694, 72, 58); bPlay->setBounds (172, 694, 72, 58); bStart->setBounds (248, 694, 96, 58);
    bSeqDn->setBounds (356, 716, 26, 30); bSeqUp->setBounds (412, 716, 26, 30);
    bBarDn->setBounds (452, 716, 26, 30); bBarUp->setBounds (504, 716, 26, 30);
    pbBpm->setBounds (542, 694, 70, 58); pbTc->setBounds (618, 694, 66, 58);
    swing.setBounds (692, 706, 72, 74);
    // pad 4x4 (pad 1 in basso a sinistra)
    for (int i = 0; i < 16; ++i) { const int r = 3 - i / 4, c = i % 4; pads[i]->setBounds (782 + c * 100, 80 + r * 100, 100, 100); }
    for (int i = 0; i < 4; ++i) bBank[i]->setBounds (782 + i * 100, 494, 96, 26);
    bRepeat->setBounds (782, 526, 112, 24); pbRpt->setBounds (898, 513, 56, 37); bFull->setBounds (958, 526, 110, 24); bErase->setBounds (1072, 526, 110, 24);
    qTune.setBounds (784, 590, 48, 196); qDecay.setBounds (836, 590, 48, 196); qFilter.setBounds (888, 590, 48, 196); qLevel.setBounds (940, 590, 48, 196);
    pbVin->setBounds (1010, 574, 172, 50);
    drive.setBounds (1008, 650, 60, 84); tone.setBounds (1068, 650, 60, 84); out.setBounds (1128, 650, 60, 84);
    bSync->setBounds (1010, 766, 84, 24); bClick->setBounds (1098, 766, 84, 24);
}

void Editor::timerCallback()
{
    if (auto s = proc.takeResample())
    {
        int target = -1;
        for (int i = 0; i < 16 && target < 0; ++i) if (proc.getSample (bank * 16 + i) == nullptr) target = bank * 16 + i;
        if (target < 0) target = selPad;
        auto& P = proc.pad[target];
        P.start = 0; P.end = 1; P.tune = 0; P.fine = 0; P.decay = 5000; P.attack = 0; P.mode = 0; P.reverse = 0; P.loop = 0; P.choke = 0; P.cutoff = 20000;
        P.bpm = (float) proc.rsBpm; P.sync = 1; P.level = 0.8f; P.sendDly = 0; P.sendRev = 0;
        proc.setSample (target, s);
        selectPad (target); setStatus ("RESAMPLE IN " + padName (target));
    }
    for (int i = 0; i < 16; ++i)
    {
        auto* pc = pads[i]; const int p = bank * 16 + i; const uint32_t h = proc.hitCount[p].load();
        if (h != pc->lastHit) { pc->lastHit = h; pc->flash = 1.0f; }
        if (pc->flash > 0.0f) { pc->flash = juce::jmax (0.0f, pc->flash - 0.12f); pc->repaint(); }
    }
    lcd->repaint(); grid->repaint();
    for (auto* b : buttons) if (b->isLit) b->repaint();
    pbBpm->repaint(); pbTc->repaint(); pbRpt->repaint(); pbVin->repaint();
    repaint (350, 710, 190, 40);
}

void Editor::loadDialog()
{
    chooser = std::make_unique<juce::FileChooser> ("Carica un campione nel pad " + padName (selPad), juce::File(), proc.formats.getWildcardForAllFormats());
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [this] (const juce::FileChooser& fc)
    {
        const auto f = fc.getResult();
        if (f.existsAsFile()) { proc.pad[selPad].start = 0; proc.pad[selPad].end = 1; setStatus (proc.loadFileToPad (f, selPad) ? "CARICATO " + f.getFileName().toUpperCase() : "FORMATO NON SUPPORTATO"); repaint(); }
    });
}

void Editor::filesDropped (const juce::StringArray& files, int x, int y)
{
    int target = selPad;
    for (int i = 0; i < 16; ++i) if (pads[i]->getBounds().contains (x, y)) target = bank * 16 + i;
    int n = 0;
    for (const auto& path : files)
    {
        const int p = target + n; if (p >= kPads) break;
        proc.pad[p].start = 0; proc.pad[p].end = 1;
        if (proc.loadFileToPad (juce::File (path), p)) ++n;
    }
    setStatus (n > 0 ? juce::String (n) + (n == 1 ? " CAMPIONE CARICATO" : " CAMPIONI CARICATI") : "FORMATO NON SUPPORTATO");
    selectPad (target);
}

static int keyToSlot (int key)
{
    static const char* rows[] = { "zxcv", "asdf", "qwer", "1234" };
    const auto lower = juce::CharacterFunctions::toLowerCase ((juce::juce_wchar) key);
    for (int r = 0; r < 4; ++r) for (int c = 0; c < 4; ++c) if (rows[r][c] == lower) return r * 4 + c;
    return -1;
}
bool Editor::keyPressed (const juce::KeyPress& k)
{
    if (k == juce::KeyPress::spaceKey) { if (proc.uiRunning) { proc.requestStop(); proc.recording = false; } else proc.requestPlay(); return true; }
    return keyToSlot (k.getKeyCode()) >= 0;
}
bool Editor::keyStateChanged (bool)
{
    bool used = false;
    static const char* keys = "zxcvasdfqwer1234";
    for (int i = 0; i < 16; ++i)
    {
        const int code = keys[i]; const bool down = juce::KeyPress::isKeyCurrentlyDown (code) || juce::KeyPress::isKeyCurrentlyDown (juce::CharacterFunctions::toUpperCase ((juce::juce_wchar) code));
        const int slot = keyToSlot (code), p = bank * 16 + slot;
        if (down && ! keysDown.count (slot)) { keysDown.insert (slot); proc.uiPadDown (p, proc.fullLevel ? 127 : 105); selPad = p; used = true; }
        else if (! down && keysDown.count (slot)) { keysDown.erase (slot); proc.uiPadUp (p); used = true; }
    }
    return used;
}
}
