#include "AlterEditor.h"

namespace ja
{
juce::Font uf (float size, bool bold) { return jlook::ui (size, bold ? 2 : 1); }
juce::Font mf (float size) { return jlook::mono (size); }

void glowText (juce::Graphics& g, const juce::String& t, juce::Rectangle<float> r, juce::Colour c, juce::Font f, juce::Justification j)
{
    g.setFont (f);
    for (int k = 2; k >= 1; --k) { g.setColour (c.withAlpha (0.12f)); g.drawText (t, r.translated ((float) -k * 0.6f, 0), j); g.drawText (t, r.translated ((float) k * 0.6f, 0), j); }
    g.setColour (c); g.drawText (t, r, j);
}
void lcd (juce::Graphics& g, juce::Rectangle<float> r, float rad)
{
    if (jlook::skinHooks().on && jlook::skinHooks().screen) { jlook::skinHooks().screen (g, r, rad, C::phos); return; }
    g.setColour (juce::Colours::black.withAlpha (0.65f)); g.fillRoundedRectangle (r.expanded (1.5f), rad + 1.5f);
    juce::ColourGradient cg (C::lcd1, r.getCentreX(), r.getCentreY(), C::lcd2, r.getX(), r.getY(), true); g.setGradientFill (cg); g.fillRoundedRectangle (r, rad);
    g.setColour (C::phos.withAlpha (0.03f));
    for (float x = r.getX() + 10; x < r.getRight(); x += 12) g.drawVerticalLine ((int) x, r.getY() + 2, r.getBottom() - 2);
    for (float y = r.getY() + 10; y < r.getBottom(); y += 12) g.drawHorizontalLine ((int) y, r.getX() + 2, r.getRight() - 2);
}
static void scanlines (juce::Graphics& g, juce::Rectangle<float> r)
{
    g.setColour (juce::Colours::black.withAlpha (0.16f)); for (float y = r.getY(); y < r.getBottom(); y += 3) g.drawHorizontalLine ((int) y, r.getX(), r.getRight());
    jlook::glass (g, r, 6);
}
static juce::String hzText (float f) { return f >= 1000 ? juce::String (f / 1000.0f, f >= 10000 ? 1 : 2) + " kHz" : juce::String (juce::roundToInt (f)) + " Hz"; }
static juce::Colour modelCol (int m) { return juce::Colour (modelInfo (m).colour); }

// ======================================================================= controlli
void Knob::mouseDown (const juce::MouseEvent& e)
{
    if (! enabled) return;
    if (e.mods.isAltDown() || e.mods.isCommandDown()) { param->setValueNotifyingHost (param->getDefaultValue()); return; }
    start = param->getValue(); param->beginChangeGesture(); drag = true;
}
void Knob::mouseDrag (const juce::MouseEvent& e)
{
    if (! drag) return;
    float v = juce::jlimit (0.0f, 1.0f, start - e.getDistanceFromDragStartY() / (e.mods.isShiftDown() ? 900.0f : 220.0f));
    if (snap) v = param->convertTo0to1 (snap (param->convertFrom0to1 (v), e.mods.isShiftDown()));
    param->setValueNotifyingHost (v);
}
void Knob::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    if (! enabled) return;
    float v = juce::jlimit (0.0f, 1.0f, param->getValue() + (w.deltaY > 0 ? 0.01f : -0.01f));
    if (snap && ! e.mods.isShiftDown()) { const float cur = param->convertFrom0to1 (param->getValue()); float nv = snap (param->convertFrom0to1 (v), false); if (std::abs (nv - cur) < 1e-4f) nv = snap (cur + (w.deltaY > 0 ? 1.0f : -1.0f), false); v = param->convertTo0to1 (nv); }
    param->setValueNotifyingHost (v);
}
void Knob::paint (juce::Graphics& g)
{
    const float lh = big ? 32.0f : 27.0f;
    const float size = juce::jmin ((float) getWidth(), getHeight() - lh);
    const auto c = enabled ? col : C::faint;
    jlook::KnobLook kl; kl.big = big; kl.bipolar = bipolar; kl.enabled = enabled; kl.hover = hover ? 1.0f : 0.0f; kl.active = drag ? 1.0f : 0.0f; kl.activity = activity;
    jlook::drawKnob (g, { (getWidth() - size) * 0.5f, 0, size, size }, param->getValue(), c, kl);
    g.setColour (C::dim); g.setFont (uf (big ? 11.5f : 9.5f)); g.drawText (label, juce::Rectangle<float> (0, size + 1, (float) getWidth(), 14), juce::Justification::centred);
    glowText (g, text ? text() : param->getCurrentValueAsText(), juce::Rectangle<float> (0, size + 15, (float) getWidth(), 14), enabled ? (drag ? c.brighter (0.4f) : c) : C::faint, mf (big ? 12.5f : 10.5f), juce::Justification::centred);
}

void Segs::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat(); lcd (g, b, 6);
    const int n = juce::jmax (1, items.size()), cur = juce::roundToInt (param->convertFrom0to1 (param->getValue()));
    const float w = (b.getWidth() - 6) / n;
    for (int i = 0; i < n; ++i)
    {
        auto r = juce::Rectangle<float> (3 + i * w, 3, w, b.getHeight() - 6).reduced (1, 0);
        if (i == cur && enabled) { g.setColour (col.withAlpha (0.18f)); g.fillRoundedRectangle (r, 4); g.setColour (col.withAlpha (0.85f)); g.drawRoundedRectangle (r, 4, 1.0f); glowText (g, items[i], r, col, uf (font), juce::Justification::centred); }
        else { g.setColour (i == cur ? C::dim : (enabled ? C::dim : C::faint)); g.setFont (uf (font)); g.drawText (items[i], r, juce::Justification::centred); }
    }
    g.setColour (juce::Colours::black.withAlpha (0.15f)); for (float y = b.getY() + 2; y < b.getBottom(); y += 3) g.drawHorizontalLine ((int) y, b.getX() + 2, b.getRight() - 2);
}
void Segs::mouseDown (const juce::MouseEvent& e)
{
    const int n = juce::jmax (1, items.size()); const int k = juce::jlimit (0, n - 1, (int) ((e.position.x - 3) / ((getWidth() - 6) / (float) n)));
    param->setValueNotifyingHost (param->convertTo0to1 ((float) k)); repaint();
}
void Switch::paint (juce::Graphics& g)
{
    const bool on = param->getValue() > 0.5f; const float h = (float) juce::jmin (getHeight(), 20), w = h * 1.8f;
    auto t = juce::Rectangle<float> (0, (getHeight() - h) / 2, w, h);
    lcd (g, t, h / 2);
    const float d = h - 6, x = on ? t.getRight() - d - 3 : 3.0f;
    if (on) { g.setColour (col.withAlpha (0.35f)); g.fillEllipse (x - 3, t.getY(), d + 6, d + 6); }
    juce::ColourGradient lg (on ? col.brighter (0.6f) : juce::Colour (0xff4a4f5a), x, t.getY() + 3, on ? col : juce::Colour (0xff15171c), x, t.getY() + 3 + d, false); g.setGradientFill (lg); g.fillEllipse (x, t.getY() + 3, d, d);
    auto lr = juce::Rectangle<float> (w + 8, 0, getWidth() - w - 8, (float) getHeight());
    if (on) glowText (g, label, lr, col, uf (11.0f)); else { g.setColour (C::dim); g.setFont (uf (11.0f)); g.drawText (label, lr, juce::Justification::centredLeft); }
}

// ======================================================================= modelli
juce::Rectangle<float> ModelGrid::cell (int m) const
{
    const int perRow = 7; const float gap = 5, w = (getWidth() - gap * (perRow - 1)) / perRow, h = (getHeight() - gap) / 2;
    const int row = m / perRow, col = m % perRow;
    const float x0 = row == 1 ? (w + gap) * 0.5f : 0.0f;   // seconda fila sfalsata, a nido d'ape
    return { x0 + col * (w + gap), row * (h + gap), w, h };
}
void ModelGrid::paint (juce::Graphics& g)
{
    const int cur = (int) ed.bv ("model");
    for (int m = 0; m < kNumModels; ++m)
    {
        auto r = cell (m); const auto c = modelCol (m); const bool on = m == cur;
        g.setColour (juce::Colours::black.withAlpha (0.55f)); g.fillRoundedRectangle (r.expanded (1), 6);
        juce::ColourGradient cg (on ? c.withAlpha (0.32f) : juce::Colour (0xff1c1f26), r.getX(), r.getY(), on ? c.withAlpha (0.10f) : juce::Colour (0xff111318), r.getX(), r.getBottom(), false);
        g.setGradientFill (cg); g.fillRoundedRectangle (r, 6);
        g.setColour (on ? c : (hov == m ? c.withAlpha (0.6f) : neo::K::hi.withAlpha (0.08f))); g.drawRoundedRectangle (r.reduced (0.5f), 6, on ? 1.4f : 1.0f);
        // led del modello
        auto led = juce::Rectangle<float> (r.getX() + 7, r.getCentreY() - 3, 6, 6);
        if (on) { g.setColour (c.withAlpha (0.4f)); g.fillEllipse (led.expanded (3)); }
        g.setColour (on ? c : c.withAlpha (0.35f)); g.fillEllipse (led);
        auto tr = r.withTrimmedLeft (16);
        g.setFont (uf (10.5f)); g.setColour (on ? c.brighter (0.3f) : (hov == m ? C::text : C::dim));
        g.drawFittedText (modelInfo (m).name, tr.reduced (2, 0).toNearestInt(), juce::Justification::centred, 1, 0.65f);
    }
}
void ModelGrid::mouseDown (const juce::MouseEvent& e)
{
    for (int m = 0; m < kNumModels; ++m) if (cell (m).contains (e.position)) { auto* p = ed.P (ed.bp ("model")); p->setValueNotifyingHost (p->convertTo0to1 ((float) m)); ed.repaint(); repaint(); return; }
}
void ModelGrid::mouseMove (const juce::MouseEvent& e)
{
    int h = -1; for (int m = 0; m < kNumModels; ++m) if (cell (m).contains (e.position)) h = m;
    if (h != hov) { hov = h; repaint(); }
    setMouseCursor (h >= 0 ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
}

// ======================================================================= display pitch
void PitchView::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat(); lcd (g, b, 8);
    auto& P = ed.proc; const int mode = juce::roundToInt (P.value ("mode"));
    const float semi = P.value ("pitch"), form = P.value ("formant");
    const float inHz = P.pitch.inHz.load(), outHz = P.pitch.outHz.load(), corr = P.pitch.corrSemi.load(), drift = P.pitch.driftSemi.load();
    auto r = b.reduced (12, 8);
    g.setColour (C::dim); g.setFont (mf (9.0f)); g.drawText (mode == TRASPONI ? "TRASPOSIZIONE" : mode == QUANTIZZA ? "QUANTIZZA SULLA SCALA" : "ROBOT: UNA NOTA", r.removeFromTop (14), juce::Justification::centredLeft);
    // spostamento totale grande
    const float tot = semi + corr + drift;
    juce::String big = (tot >= 0 ? "+" : juce::String::fromUTF8 ("\xe2\x88\x92")) + juce::String (std::abs (tot), std::abs (tot - std::round (tot)) < 0.005f ? 0 : 2);
    glowText (g, big, r.removeFromTop (44), P.pitch.active.load() ? C::phos : C::faint, jlook::logo (30.0f), juce::Justification::centredLeft);
    g.setColour (C::dim); g.setFont (mf (9.0f)); g.drawText ("semitoni", juce::Rectangle<float> (r.getX() + 110, r.getY() - 30, 80, 14), juce::Justification::centredLeft);
    auto row = [&] (const juce::String& k, const juce::String& v, juce::Colour c)
    { auto rr = r.removeFromTop (16); g.setColour (C::dim); g.setFont (mf (9.5f)); g.drawText (k, rr.removeFromLeft (64), juce::Justification::centredLeft); glowText (g, v, rr, c, mf (11.0f)); };
    if (mode == TRASPONI) { row ("FORMANTI", (form > 0 ? "+" : "") + juce::String (form, 1) + (P.value ("link") > 0.5f ? "  + legate" : ""), C::violet); row ("DERIVA", juce::String (drift, 2) + " st", C::amber); }
    else
    {
        row ("ENTRA", inHz > 0 ? noteOf (inHz) + "  " + juce::String (inHz, 1) + " Hz" : juce::String ("-"), C::text);
        row ("ESCE", outHz > 0 ? noteOf (outHz) + "  " + juce::String (outHz, 1) + " Hz" : juce::String ("-"), C::phos);
    }
    // traccia
    auto tr = r.withTrimmedTop (4);
    g.setColour (C::phos.withAlpha (0.12f)); g.drawHorizontalLine ((int) tr.getCentreY(), tr.getX(), tr.getRight());
    juce::Path p; const int n = (int) std::size (trace);
    for (int i = 0; i < n; ++i)
    {
        const float v = trace[(tw + i) % n], x = tr.getX() + tr.getWidth() * i / (n - 1), y = tr.getCentreY() - juce::jlimit (-1.0f, 1.0f, v / 12.0f) * tr.getHeight() * 0.5f;
        if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
    }
    g.setColour (C::phos.withAlpha (0.25f)); g.strokePath (p, juce::PathStrokeType (3.0f));
    g.setColour (C::phos); g.strokePath (p, juce::PathStrokeType (1.2f));
    scanlines (g, b);
}

// ======================================================================= display drive
void DriveView::update (int model, float drive, float tone)
{
    if (model == lastM && std::abs (drive - lastD) < 1e-4f && std::abs (tone - lastT) < 1e-4f) return;
    lastM = model; lastD = drive; lastT = tone;
    const double fs = 192000; DriveChannel dc; dc.prepare (fs, 4); dc.setModel (model); dc.setDrive (drive);
    const float g = juce::Decibels::decibelsToGain (drive * modelInfo (model).maxDb);
    const int order = 14, N = 1 << order; const int cyc = 64; const double f0 = fs * cyc / N;   // 750 Hz esatti sul bin
    std::vector<float> y ((size_t) N); const int settle = 8192;
    double ein = 0, eout = 0;
    for (int i = 0; i < settle + N; ++i)
    {
        const float x = 0.25f * (float) std::sin (2 * juce::MathConstants<double>::pi * f0 * i / fs);
        const float o = dc.process (x, g, drive);
        if (i >= settle) { y[(size_t) (i - settle)] = o; ein += x * x; eout += o * o; }
    }
    const float norm = (float) std::sqrt (ein / juce::jmax (1e-12, eout));
    // due cicli per la forma d'onda
    const int per = N / cyc; waveIn.resize ((size_t) (2 * per)); waveOut.resize ((size_t) (2 * per));
    for (int i = 0; i < 2 * per; ++i) { waveIn[(size_t) i] = 0.25f * (float) std::sin (2 * juce::MathConstants<double>::pi * i / per); waveOut[(size_t) i] = y[(size_t) i] * norm; }
    juce::dsp::FFT f (order); std::vector<float> d ((size_t) (2 * N), 0);
    for (int i = 0; i < N; ++i) d[(size_t) i] = y[(size_t) i];
    f.performFrequencyOnlyForwardTransform (d.data(), true);
    const float h1 = juce::jmax (1e-9f, d[(size_t) cyc]); double hs = 0;
    for (int h = 1; h <= 12; ++h)
    {
        float a = 0; for (int k = -1; k <= 1; ++k) a = juce::jmax (a, d[(size_t) (h * cyc + k)]);
        harm[h - 1] = juce::Decibels::gainToDecibels (a / h1, -90.0f); if (h > 1) hs += (a / h1) * (a / h1);
    }
    thd = (float) std::sqrt (hs) * 100.0f;
    juce::ignoreUnused (tone);
}
void DriveView::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat(); lcd (g, b, 8);
    const int m = (int) ed.bv ("model"); const auto c = modelCol (m);
    auto top = b.reduced (10, 8); auto wv = top.removeFromTop (top.getHeight() * 0.5f); top.removeFromTop (6);
    g.setColour (C::dim); g.setFont (mf (8.5f)); g.drawText ("FORMA D'ONDA", wv.getX(), wv.getY(), 120, 12, juce::Justification::centredLeft);
    g.drawText ("THD " + (thd < 10 ? juce::String (thd, 1) : juce::String (juce::roundToInt (thd))) + "%", juce::Rectangle<float> (wv.getRight() - 90, wv.getY(), 90, 12), juce::Justification::centredRight);
    auto plot = wv.withTrimmedTop (12);
    g.setColour (C::phos.withAlpha (0.1f)); g.drawHorizontalLine ((int) plot.getCentreY(), plot.getX(), plot.getRight());
    auto drawW = [&] (const std::vector<float>& w, juce::Colour col, float th)
    {
        if (w.empty()) return; juce::Path p; float mx = 0.3f; for (auto v : waveOut) mx = juce::jmax (mx, std::abs (v));
        for (size_t i = 0; i < w.size(); ++i) { const float x = plot.getX() + plot.getWidth() * i / (w.size() - 1), y = plot.getCentreY() - w[i] / mx * plot.getHeight() * 0.46f; if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y); }
        g.setColour (col.withAlpha (0.2f)); g.strokePath (p, juce::PathStrokeType (th * 3)); g.setColour (col); g.strokePath (p, juce::PathStrokeType (th));
    };
    drawW (waveIn, C::dim.withAlpha (0.6f), 1.0f); drawW (waveOut, c, 1.6f);
    // armoniche
    g.setColour (C::dim); g.setFont (mf (8.5f)); g.drawText ("ARMONICHE", top.getX(), top.getY(), 120, 12, juce::Justification::centredLeft);
    auto hb = top.withTrimmedTop (14);
    const float bw = hb.getWidth() / 12;
    for (int h = 0; h < 12; ++h)
    {
        const float n = juce::jlimit (0.0f, 1.0f, (harm[h] + 80) / 80);
        auto r = juce::Rectangle<float> (hb.getX() + h * bw + 3, hb.getY(), bw - 6, hb.getHeight() - 12);
        g.setColour (neo::K::hi.withAlpha (0.04f)); g.fillRoundedRectangle (r, 2);
        auto f = r.withTop (r.getBottom() - r.getHeight() * n);
        const auto hc = h == 0 ? C::text : ((h + 1) % 2 == 0 ? C::amber : c);   // pari in ambra, dispari col colore del modello
        g.setColour (hc.withAlpha (0.25f)); g.fillRoundedRectangle (f.expanded (1.5f), 3); g.setColour (hc); g.fillRoundedRectangle (f, 2);
        g.setColour (C::dim); g.setFont (mf (8.0f)); g.drawText (juce::String (h + 1), juce::Rectangle<float> (r.getX(), r.getBottom() + 1, r.getWidth(), 11), juce::Justification::centred);
    }
    scanlines (g, b);
}

// ======================================================================= FOCUS
void FocusView::analyse()
{
    const int order = 12, N = 1 << order; if (! fft) fft = std::make_unique<juce::dsp::FFT> (order);
    std::vector<float> d ((size_t) (2 * N)); const double sr = ed.proc.sampleRate();
    auto spec = [&] (const Ring& rg, float* out)
    {
        rg.read (d.data(), N); for (int i = 0; i < N; ++i) d[(size_t) i] *= 0.5f - 0.5f * std::cos (kTwoPi * i / N);
        std::fill (d.begin() + N, d.end(), 0.0f); fft->performFrequencyOnlyForwardTransform (d.data(), true);
        for (int k = 0; k < 256; ++k)
        {
            const float f0 = 20.0f * std::pow (1000.0f, k / 256.0f), f1 = 20.0f * std::pow (1000.0f, (k + 1) / 256.0f);
            int a = (int) (f0 * N / sr), bb = juce::jmax (a + 1, (int) (f1 * N / sr)); float mx = 0;
            for (int j = a; j < bb && j < N / 2; ++j) mx = juce::jmax (mx, d[(size_t) j]);
            const float db = juce::Decibels::gainToDecibels (mx * 4.0f / N, -100.0f) + 4.5f * std::log2 (juce::jmax (20.0f, f0) / 1000.0f);   // inclinazione 4.5 dB/ott
            out[k] = out[k] * 0.7f + db * 0.3f;
        }
    };
    spec (ed.proc.ringIn, spIn); spec (ed.proc.ringOut, spOut);
}
static const juce::uint32 kBandCol[4] = { 0xffff7a3d, 0xffffd23f, 0xff5ef2ff, 0xffa66bff };
void FocusView::edges (float* e, int& nb) const
{
    auto& P = ed.proc; nb = juce::jlimit (1, kBandsMax, juce::roundToInt (P.value ("bands")));
    float x[3] = { P.value ("x1"), P.value ("x2"), P.value ("x3") }; for (int k = 1; k < 3; ++k) x[k] = juce::jmax (x[k], x[k - 1] * 1.15f);
    e[0] = 20; for (int k = 0; k < nb - 1; ++k) e[k + 1] = x[k]; e[nb] = 20000;
}
int FocusView::xParam (int k, int) const { return k; }
int FocusView::bandAt (float x) const { float e[5]; int nb; edges (e, nb); for (int b = 0; b < nb; ++b) if (x >= xOf (e[b]) && x < xOf (e[b + 1])) return b; return nb - 1; }
void FocusView::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat(); lcd (g, b, 8);
    auto& P = ed.proc; float e[5]; int nb; edges (e, nb);
    const float h = b.getHeight() - 16;
    g.setFont (mf (8.0f));
    for (float f : { 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f })
    {
        const float x = xOf (f); g.setColour (C::phos.withAlpha (0.07f)); g.drawVerticalLine ((int) x, 4, b.getBottom() - 14);
        g.setColour (C::dim.withAlpha (0.7f)); g.drawText (f >= 1000 ? juce::String ((int) (f / 1000)) + "k" : juce::String ((int) f), juce::Rectangle<float> (x - 20, b.getBottom() - 13, 40, 11), juce::Justification::centred);
    }
    // bande
    for (int k = 0; k < nb; ++k)
    {
        const juce::String p = "b" + juce::String (k + 1) + "_";
        const int m = (int) P.value ((p + "model").toRawUTF8()); const float dr = P.value ((p + "drive").toRawUTF8());
        const bool sel = k == ed.band, byp = P.value ((p + "byp").toRawUTF8()) > 0.5f, so = P.value ((p + "solo").toRawUTF8()) > 0.5f;
        const auto c = byp || dr < 0.001f ? C::faint : modelCol (m);
        auto r = juce::Rectangle<float> (xOf (e[k]), 2, xOf (e[k + 1]) - xOf (e[k]), h - 2);
        juce::ColourGradient bg (c.withAlpha (sel ? 0.26f : 0.12f), 0, r.getY(), c.withAlpha (0.02f), 0, r.getBottom(), false); g.setGradientFill (bg); g.fillRect (r);
        const float lv = juce::jmin (1.0f, ed.proc.drive.bandLev[k].load());
        g.setColour (c.withAlpha (0.5f)); g.fillRect (r.getX() + 3, r.getBottom() - 4 - lv * 30, 4.0f, lv * 30);
        if (sel) { g.setColour (juce::Colour (kBandCol[k])); g.drawRect (r, 1.6f); }
        auto lab = r.reduced (6, 4).withHeight (30);
        glowText (g, juce::String (k + 1) + "  " + (byp ? juce::String ("BYPASS") : dr < 0.001f ? juce::String ("PULITA") : juce::String (modelInfo (m).name)), lab.withHeight (14), sel ? C::text : c, uf (10.0f));
        g.setColour (C::dim); g.setFont (mf (9.0f)); g.drawText (dr < 0.001f ? juce::String ("drive 0") : "drive " + juce::String (juce::roundToInt (dr * 100)) + "%" + (so ? "  SOLO" : ""), lab.withTrimmedTop (15), juce::Justification::centredLeft);
    }
    // spettri
    auto curve = [&] (const float* sp, juce::Colour c, bool fill)
    {
        juce::Path pth;
        for (int k = 0; k < 256; ++k) { const float x = b.getWidth() * (k + 0.5f) / 256, y = juce::jlimit (2.0f, h, h * (1 - (sp[k] + 84) / 84)); if (k == 0) pth.startNewSubPath (x, y); else pth.lineTo (x, y); }
        if (fill) { juce::Path f = pth; f.lineTo (b.getWidth(), h); f.lineTo (0, h); f.closeSubPath(); juce::ColourGradient cg (c.withAlpha (0.18f), 0, 0, c.withAlpha (0.02f), 0, h, false); g.setGradientFill (cg); g.fillPath (f); }
        g.setColour (c.withAlpha (0.25f)); g.strokePath (pth, juce::PathStrokeType (3.0f)); g.setColour (c); g.strokePath (pth, juce::PathStrokeType (1.1f));
    };
    curve (spIn, C::dim.withAlpha (0.55f), false);
    curve (spOut, C::phos, true);
    // tagli trascinabili
    for (int k = 1; k < nb; ++k)
    {
        const float x = xOf (e[k]); const bool hh = hovH == k - 1 || drag == k - 1;
        g.setColour (C::text.withAlpha (hh ? 1.0f : 0.7f)); g.fillRect (x - (hh ? 1.5f : 1.0f), 2.0f, hh ? 3.0f : 2.0f, h - 2);
        auto knob = juce::Rectangle<float> (x - 7, h * 0.5f - 11, 14, 22);
        g.setColour (juce::Colours::black.withAlpha (0.7f)); g.fillRoundedRectangle (knob.expanded (1), 4);
        g.setColour (C::text); g.drawRoundedRectangle (knob, 4, 1.2f);
        glowText (g, hzText (e[k]), juce::Rectangle<float> (x + 9, h * 0.5f - 7, 70, 14), C::text, mf (9.5f));
    }
    g.setColour (C::dim); g.setFont (uf (9.5f, false));
    g.drawText (nb < kBandsMax ? "doppio clic = nuova banda   clic = scegli   trascina i tagli   doppio clic sul taglio = toglilo" : "4 bande   doppio clic sul taglio = toglilo",
                juce::Rectangle<float> (8, h - 16, b.getWidth() - 16, 14), juce::Justification::centredRight);
    scanlines (g, b);
}
void FocusView::mouseMove (const juce::MouseEvent& e)
{
    float ed2[5]; int nb; edges (ed2, nb);
    int hh = -1; for (int k = 1; k < nb; ++k) if (std::abs (e.position.x - xOf (ed2[k])) < 8) hh = k - 1;
    if (hh != hovH) { hovH = hh; repaint(); }
    setMouseCursor (hh >= 0 ? juce::MouseCursor::LeftRightResizeCursor : juce::MouseCursor::PointingHandCursor);
}
void FocusView::mouseDown (const juce::MouseEvent& e)
{
    mouseMove (e); drag = hovH;
    if (drag >= 0) { ed.P ("x" + juce::String (drag + 1))->beginChangeGesture(); return; }
    const int b = bandAt (e.position.x);
    if (e.mods.isPopupMenu())
    {
        juce::PopupMenu m; const juce::String p = "b" + juce::String (b + 1) + "_";
        m.addItem (1, "Solo", true, ed.proc.value ((p + "solo").toRawUTF8()) > 0.5f); m.addItem (2, "Bypass", true, ed.proc.value ((p + "byp").toRawUTF8()) > 0.5f);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [this, p] (int r) { if (r <= 0) return; auto* q = ed.P (p + (r == 1 ? "solo" : "byp")); q->setValueNotifyingHost (q->getValue() > 0.5f ? 0.0f : 1.0f); });
    }
    ed.selectBand (b);
}
void FocusView::mouseDrag (const juce::MouseEvent& e)
{
    if (drag < 0) return;
    float ed2[5]; int nb; edges (ed2, nb);
    const float lo = ed2[drag] * 1.15f, hi = ed2[drag + 2] / 1.15f;
    const float f = juce::jlimit (juce::jmax (30.0f, lo), juce::jmin (16000.0f, hi), hzOf (e.position.x));
    auto* p = ed.P ("x" + juce::String (drag + 1)); p->setValueNotifyingHost (p->convertTo0to1 (f)); repaint();
}
void FocusView::mouseUp (const juce::MouseEvent&) { if (drag >= 0) ed.P ("x" + juce::String (drag + 1))->endChangeGesture(); drag = -1; }
void FocusView::mouseDoubleClick (const juce::MouseEvent& e)
{
    auto& P = ed.proc; float ed2[5]; int nb; edges (ed2, nb);
    auto setP = [this] (const juce::String& id, float v) { auto* q = ed.P (id); q->setValueNotifyingHost (q->convertTo0to1 (v)); };
    // doppio clic su un taglio: lo toglie (le bande sopra scendono di uno)
    for (int k = 1; k < nb; ++k)
        if (std::abs (e.position.x - xOf (ed2[k])) < 8)
        {
            for (int j = k; j < nb - 1; ++j) setP ("x" + juce::String (j), P.value (("x" + juce::String (j + 1)).toRawUTF8()));
            for (int bb = k; bb < nb - 1; ++bb)
                for (auto key : { "model", "drive", "tone", "mix", "lvl", "solo", "byp" })
                    setP ("b" + juce::String (bb + 1) + "_" + key, P.value (("b" + juce::String (bb + 2) + "_" + key).toRawUTF8()));
            setP ("bands", (float) (nb - 1)); ed.selectBand (juce::jmin (ed.band, nb - 2)); repaint(); return;
        }
    // doppio clic dentro una banda: la divide in due in quel punto (la nuova eredita le impostazioni)
    if (nb >= kBandsMax) return;
    const float f = hzOf (e.position.x); const int b = bandAt (e.position.x);
    // sposta tagli e bande sopra di uno
    for (int j = nb - 1; j > b; --j) setP ("x" + juce::String (j + 1), P.value (("x" + juce::String (j)).toRawUTF8()));
    for (int bb = nb - 1; bb > b; --bb)
        for (auto key : { "model", "drive", "tone", "mix", "lvl", "solo", "byp" })
            setP ("b" + juce::String (bb + 2) + "_" + key, P.value (("b" + juce::String (bb + 1) + "_" + key).toRawUTF8()));
    setP ("x" + juce::String (b + 1), juce::jlimit (30.0f, 16000.0f, f));
    for (auto key : { "model", "drive", "tone", "mix", "lvl" }) setP ("b" + juce::String (b + 2) + "_" + key, P.value (("b" + juce::String (b + 1) + "_" + key).toRawUTF8()));
    setP ("b" + juce::String (b + 2) + "_solo", 0); setP ("b" + juce::String (b + 2) + "_byp", 0);
    setP ("bands", (float) (nb + 1)); ed.selectBand (b + 1); repaint();
}

// ======================================================================= deriva
void DriftView::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat(); lcd (g, b, 8);
    const float amt = ed.proc.value ("drift"); const float range = juce::jmax (0.25f, amt * amt * 3.2f);
    g.setColour (C::amber.withAlpha (0.12f)); g.drawHorizontalLine ((int) b.getCentreY(), b.getX() + 4, b.getRight() - 4);
    juce::Path p; const int n = (int) std::size (trace);
    for (int i = 0; i < n; ++i)
    {
        const float x = b.getX() + 6 + (b.getWidth() - 12) * i / (n - 1), y = b.getCentreY() - juce::jlimit (-1.0f, 1.0f, trace[(tw + i) % n] / range) * (b.getHeight() * 0.4f);
        if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
    }
    g.setColour (C::amber.withAlpha (0.25f)); g.strokePath (p, juce::PathStrokeType (3.5f)); g.setColour (C::amber); g.strokePath (p, juce::PathStrokeType (1.3f));
    g.setColour (C::dim); g.setFont (mf (8.5f));
    g.drawText (juce::String::fromUTF8 ("\xc2\xb1") + juce::String (range, 2) + " st", b.reduced (8, 4), juce::Justification::topRight);
    scanlines (g, b);
}

// ======================================================================= Editor
Editor::Editor (Processor& p) : AudioProcessorEditor (&p), proc (p)
{
    skin::init ("alter", { 0xffe58aa0, 0xffb9a6e0, 0xffe9a260, 0xffcfcfca }, 0xffff2e88); C::sync(); themeRect = { 952, 15, 100, 36 };
    setLookAndFeel (&lnf);
    auto semi = [] (float v, bool fine) { return fine ? v : std::round (v); };
    kPMix = std::make_unique<Knob> (P ("pmix"), C::phos); kPMix->label = "MIX PITCH"; kPMix->text = [this] { return juce::String (juce::roundToInt (proc.value ("pmix") * 100)) + "%"; };
    kPitch = std::make_unique<Knob> (P ("pitch"), C::phos); kPitch->label = "PITCH"; kPitch->big = true; kPitch->bipolar = true; kPitch->snap = semi;
    kPitch->text = [this] { const float v = proc.value ("pitch"); return (v > 0 ? "+" : "") + juce::String (v, std::abs (v - std::round (v)) < 0.005f ? 0 : 2) + " st"; };
    kFormant = std::make_unique<Knob> (P ("formant"), C::violet); kFormant->label = "FORMANTI"; kFormant->big = true; kFormant->bipolar = true; kFormant->snap = [] (float v, bool fine) { return fine ? v : std::round (v * 2) / 2; };
    kFormant->text = [this] { const float v = proc.value ("formant"); return (v > 0 ? "+" : "") + juce::String (v, 1); };
    kDrift = std::make_unique<Knob> (P ("drift"), C::amber); kDrift->label = "DERIVA"; kDrift->text = [this] { return juce::String (juce::roundToInt (proc.value ("drift") * 100)) + "%"; };
    kRate = std::make_unique<Knob> (P ("driftRate"), C::amber); kRate->label = "VELOCITA";
    kRate->text = [this] { const float v = proc.value ("driftRate"); return juce::String (v, v < 1 ? 2 : 1) + " Hz"; };
    kDrive = std::make_unique<Knob> (P ("b1_drive"), C::magenta); kDrive->label = "DRIVE"; kDrive->big = true;
    kDrive->text = [this] { return juce::String (juce::roundToInt (bv ("drive") * 100)) + "%"; };
    kBMix = std::make_unique<Knob> (P ("b1_mix"), C::magenta); kBMix->label = "MIX BANDA"; kBMix->text = [this] { return juce::String (juce::roundToInt (bv ("mix") * 100)) + "%"; };
    kBLvl = std::make_unique<Knob> (P ("b1_lvl"), C::magenta); kBLvl->label = "LIVELLO"; kBLvl->bipolar = true; kBLvl->text = [this] { const float v = bv ("lvl"); return (v > 0 ? "+" : "") + juce::String (v, 1) + " dB"; };
    kTone = std::make_unique<Knob> (P ("b1_tone"), C::magenta); kTone->label = "TONO"; kTone->bipolar = true;
    kTone->text = [this] { const float v = bv ("tone"); return std::abs (v) < 0.02f ? juce::String ("PIATTO") : (v > 0 ? "BRILLANTE " : "SCURO ") + juce::String (juce::roundToInt (std::abs (v) * 100)); };
    kMix = std::make_unique<Knob> (P ("mix"), C::phos); kMix->label = "MIX"; kMix->big = true; kMix->text = [this] { return juce::String (juce::roundToInt (proc.value ("mix") * 100)) + "%"; };
    kOut = std::make_unique<Knob> (P ("out"), C::phos); kOut->label = "OUTPUT"; kOut->bipolar = true;
    kOut->text = [this] { const float v = proc.value ("out"); return (v > 0 ? "+" : "") + juce::String (v, 1) + " dB"; };
    sMode = std::make_unique<Segs> (P ("mode"), C::phos, juce::StringArray { "TRASPONI", "QUANTIZZA", "ROBOT" });
    sKey = std::make_unique<Segs> (P ("key"), C::green, noteNames()); sKey->font = 10.5f;
    sScale = std::make_unique<Segs> (P ("scale"), C::green, juce::StringArray { "CROMATICA", "MAGGIORE", "MINORE", "ARMONICA", "PENTA", "FRIGIA" }); sScale->font = 10.0f;
    sDriftT = std::make_unique<Segs> (P ("driftType"), C::amber, juce::StringArray { "WOW", "VIBRATO", "CAOS" });
    sOrder = std::make_unique<Segs> (P ("order"), C::phos, juce::StringArray { juce::String::fromUTF8 ("PITCH \xe2\x80\xba DRIVE"), juce::String::fromUTF8 ("DRIVE \xe2\x80\xba PITCH") });
    wLink = std::make_unique<Switch> (P ("link"), C::violet, "FORMANTI LEGATE");
    wSolo = std::make_unique<Switch> (P ("b1_solo"), C::green, "SOLO BANDA");
    wByp = std::make_unique<Switch> (P ("b1_byp"), C::red, "BYPASS BANDA");
    // echo
    auto ek = [this] (std::unique_ptr<Knob>& k, const char* id, const char* l, std::function<juce::String()> t) { k = std::make_unique<Knob> (P (id), C::green); k->label = l; k->text = std::move (t); };
    ek (kEDiv, "eDiv", "TEMPO", [this] { return echoDivs()[juce::roundToInt (proc.value ("eDiv"))]; });
    ek (kEMs, "eMs", "TEMPO", [this] { return juce::String (juce::roundToInt (proc.value ("eMs"))) + " ms"; });
    ek (kEFb, "eFb", "FEEDBACK", [this] { return juce::String (juce::roundToInt (proc.value ("eFb") * 100)) + "%"; });
    ek (kEMix, "eMix", "MIX ECHO", [this] { return juce::String (juce::roundToInt (proc.value ("eMix") * 100)) + "%"; });
    ek (kELo, "eLo", "TAGLIO BASSI", [this] { return hzText (proc.value ("eLo")); });
    ek (kEHi, "eHi", "TAGLIO ALTI", [this] { return hzText (proc.value ("eHi")); });
    ek (kESat, "eSat", "SATURAZIONE", [this] { return juce::String (juce::roundToInt (proc.value ("eSat") * 100)) + "%"; });
    wEOn = std::make_unique<Switch> (P ("eOn"), C::green, "ECHO"); wESync = std::make_unique<Switch> (P ("eSync"), C::green, "A TEMPO"); wEPing = std::make_unique<Switch> (P ("ePing"), C::green, "PING-PONG");
    wAuto = std::make_unique<Switch> (P ("auto"), C::phos, "AUTO GAIN");
    // voce pitch a parte
    kVWide = std::make_unique<Knob> (P ("vWide"), C::violet); kVWide->label = "LARGHEZZA"; kVWide->text = [this] { return juce::String (juce::roundToInt (proc.value ("vWide") * 100)) + "%"; };
    kVPan = std::make_unique<Knob> (P ("vPan"), C::violet); kVPan->label = "POSIZIONE"; kVPan->bipolar = true;
    kVPan->text = [this] { const float v = proc.value ("vPan"); return std::abs (v) < 0.02f ? juce::String ("CENTRO") : (v < 0 ? "SX " : "DX ") + juce::String (juce::roundToInt (std::abs (v) * 100)); };
    wVEcho = std::make_unique<Switch> (P ("vEcho"), C::violet, "ECHO SOLO SU LEI");
    for (juce::Component* c : std::initializer_list<juce::Component*> { kPitch.get(), kFormant.get(), kPMix.get(), kDrift.get(), kRate.get(), kDrive.get(), kTone.get(), kBMix.get(), kBLvl.get(), kMix.get(), kOut.get(),
                                                                        kEDiv.get(), kEMs.get(), kEFb.get(), kEMix.get(), kELo.get(), kEHi.get(), kESat.get(), wEOn.get(), wESync.get(), wEPing.get(),
                                                                        sMode.get(), sKey.get(), sScale.get(), sDriftT.get(), sOrder.get(), wLink.get(), wSolo.get(), wByp.get(), wAuto.get(),
                                                                        kVWide.get(), kVPan.get(), wVEcho.get(),
                                                                        &grid, &pitchView, &driveView, &focusView, &driftView })
        addAndMakeVisible (c);
    addAndMakeVisible (sizeChip);
    scaler.setup();
    selectBand (0);
    startTimerHz (30);
}
void Editor::selectBand (int b)
{
    band = juce::jlimit (0, kBandsMax - 1, b);
    kDrive->param = P (bp ("drive")); kTone->param = P (bp ("tone")); kBMix->param = P (bp ("mix")); kBLvl->param = P (bp ("lvl"));
    wSolo->param = P (bp ("solo")); wByp->param = P (bp ("byp"));
    repaint();
}

void Editor::resized()
{
    const float sc = scaler.update();
    presetRect = { 450, 15, 330, 36 }; prevRect = { 412, 15, 36, 36 }; nextRect = { 782, 15, 36, 36 }; abRect = { 830, 15, 110, 36 };
    // ALTER
    sMode->setBounds (34, 112, 330, 30); wLink->setBounds (390, 116, 176, 22);
    kPitch->setBounds (30, 156, 120, 120 + 32); kFormant->setBounds (154, 156, 120, 120 + 32); kPMix->setBounds (278, 180, 76, 76 + 27);
    pitchView.setBounds (362, 156, 204, 172);
    sKey->setBounds (34, 344, 532, 26); sScale->setBounds (34, 376, 532, 26);
    // DRIVE
    grid.setBounds (606, 110, 576, 62);
    kDrive->setBounds (606, 186, 140, 140 + 32); kTone->setBounds (752, 186, 70, 70 + 27); kBMix->setBounds (826, 186, 70, 70 + 27); kBLvl->setBounds (789, 270, 70, 70 + 27);
    driveView.setBounds (906, 186, 276, 168);
    // DERIVA
    sDriftT->setBounds (34, 488, 236, 28);
    kDrift->setBounds (40, 526, 96, 96 + 27); kRate->setBounds (168, 526, 96, 96 + 27);
    driftView.setBounds (34, 684, 236, 76);
    // FOCUS
    wSolo->setBounds (470, 459, 150, 22); wByp->setBounds (640, 459, 170, 22);
    focusView.setBounds (316, 492, 548, 236);
    // USCITA
    sOrder->setBounds (892, 488, 290, 28);
    wAuto->setBounds (892, 528, 130, 22);
    kMix->setBounds (892, 562, 120, 120 + 32); kOut->setBounds (1022, 580, 90, 90 + 27);
    // ECHO
    wEOn->setBounds (34, 806, 110, 22); wESync->setBounds (34, 840, 110, 22); wEPing->setBounds (34, 874, 120, 22);
    kEDiv->setBounds (176, 796, 86, 86 + 27); kEMs->setBounds (176, 796, 86, 86 + 27);
    { int x = 280; for (auto* k : { kEFb.get(), kEMix.get(), kELo.get(), kEHi.get(), kESat.get() }) { k->setBounds (x, 796, 86, 86 + 27); x += 100; } }
    kVWide->setBounds (800, 796, 86, 86 + 27); kVPan->setBounds (894, 796, 86, 86 + 27); wVEcho->setBounds (992, 806, 178, 22);
    sizeChip.setBounds (W - 76, H - 18, 58, 15);
    jlook::scaleChildren (*this, sc);
}

void Editor::paint (juce::Graphics& g)
{
    skin::apply(); C::sync();
    g.addTransform (juce::AffineTransform::scale (scaler.s));
    const float Wf = (float) W, Hf = (float) H;
    const int model = (int) bv ("model"); const auto mc = modelCol (model);
    jlook::backdrop (g, { 0, 0, Wf, Hf }, C::magenta, C::phos);
    jlook::header (g, { 0, 0, Wf, 64 }, "ALTER DRIVE", C::magenta, "JA-1  PITCH + FORMANTI + SATURAZIONE", C::phos, C::magenta);
    lcd (g, presetRect.toFloat(), 8); jlook::glass (g, presetRect.toFloat(), 8);
    glowText (g, proc.getProgramName (proc.preset).toUpperCase(), presetRect.toFloat(), C::amber, mf (12.5f), juce::Justification::centred);
    g.setColour (C::phos); g.setFont (uf (20.0f));
    g.drawText (juce::String::fromUTF8 ("\xe2\x80\xb9"), prevRect, juce::Justification::centred); g.drawText (juce::String::fromUTF8 ("\xe2\x80\xba"), nextRect, juce::Justification::centred);
    jlook::button (g, abRect.toFloat(), C::phos, proc.ab.cur == 1, abHover, "MEMORIA " + proc.ab.label(), uf (11.5f));
    // latenza
    g.setColour (C::dim); g.setFont (mf (9.0f));
    g.drawText ("latenza " + juce::String (proc.getLatencySamples() * 1000.0 / juce::jmax (1.0, proc.sampleRate()), 1) + " ms (compensata)", juce::Rectangle<float> (Wf - 250, 20, 230, 14), juce::Justification::centredRight);
    for (int i = 0; i < 9; ++i) { auto r = juce::Rectangle<float> (Wf - 120 + i * 11.0f, 38, 5, 14); g.setColour (juce::Colours::black.withAlpha (0.8f)); g.fillRoundedRectangle (r, 2.5f); }

    auto card = [&] (juce::Rectangle<float> r, juce::Colour c, const juce::String& t, const juce::String& sub)
    {
        jlook::softShadow (g, r, 14); jlook::card (g, r, 14, c, true, false, false);
        glowText (g, t, { r.getX() + 16, r.getY() + 6, 300, 22 }, C::text, uf (13.0f));
        if (sub.isNotEmpty()) { g.setColour (C::dim); g.setFont (mf (9.0f)); g.drawText (sub, juce::Rectangle<float> (r.getX() + 16, r.getY() + 8, r.getWidth() - 32, 18), juce::Justification::centredRight); }
    };
    card ({ 18, 78, 562, 362 }, C::phos, "ALTER", "pitch e formanti indipendenti");
    card ({ 590, 78, 592, 362 }, mc, juce::String ("DRIVE  ") + juce::String::fromUTF8 ("\xc2\xb7") + "  BANDA " + juce::String (band + 1) + "  " + juce::String::fromUTF8 ("\xc2\xb7") + "  " + modelInfo (model).name, modelInfo (model).sub);
    card ({ 18, 452, 268, 318 }, C::amber, "DERIVA", "pitch che ondeggia");
    card ({ 298, 452, 582, 318 }, C::magenta, "", "");
    glowText (g, "MULTIBANDA", { 314, 458, 150, 22 }, C::text, uf (13.0f));
    card ({ 876 + 4, 452, 302, 318 }, C::phos, "USCITA", "");
    card ({ 18, 782, 1164, 136 }, C::green, "", "");
    g.setColour (C::dim); g.setFont (uf (10.0f, false));
    g.setColour (C::violet.withAlpha (0.25f)); g.drawVerticalLine (790, 792.0f, 910.0f);
    glowText (g, "VOCE PITCH", { 800, 784, 200, 14 }, C::violet, uf (10.5f));
    g.drawFittedText ("Solo la voce spostata (quella che esce con MIX PITCH): larghezza, posizione, e l'echo puo andare solo su di lei. La voce principale resta asciutta e al centro.",
                      juce::Rectangle<int> (992, 836, 180, 72), juce::Justification::topLeft, 5);

    // spiegazioni brevi
    const int mode = juce::roundToInt (proc.value ("mode"));
    g.setColour (C::dim); g.setFont (uf (10.0f, false));
    g.drawFittedText (mode == TRASPONI ? "KEY e SCALA servono per QUANTIZZA e ROBOT. PITCH a scatti di semitono, SHIFT per i centesimi."
                                       : mode == QUANTIZZA ? "Ogni nota cantata viene portata sulla nota piu vicina della scala. PITCH sposta sopra o sotto."
                                                           : "Tutta la voce su una sola nota (KEY), all'ottava piu vicina. PITCH per spostarla.",
                      juce::Rectangle<int> (34, 408, 532, 26), juce::Justification::centredLeft, 2);
    // descrizione del modello
    auto dr = juce::Rectangle<float> (606, 364, 576, 70);
    g.setColour (C::text.withAlpha (0.85f)); g.setFont (uf (11.0f, false));
    g.drawFittedText (modelInfo (model).what, dr.removeFromTop (34).toNearestInt(), juce::Justification::topLeft, 2);
    g.setColour (mc); g.setFont (uf (11.0f));
    g.drawFittedText (juce::String ("SU COSA: ") + modelInfo (model).use, dr.toNearestInt(), juce::Justification::topLeft, 2);
    // focus: nota sotto lo spettro
    g.setColour (C::dim); g.setFont (uf (10.0f, false));
    g.drawFittedText ("Come Saturn: ogni banda ha il suo modello, drive, tono, mix e livello. Il modello, DRIVE e le manopole a destra cambiano solo la banda scelta. Tasto destro: solo o bypass.",
                      juce::Rectangle<int> (316, 732, 548, 30), juce::Justification::centredLeft, 2);
    // uscita: auto gain e misure
    g.setColour (C::dim); g.setFont (mf (9.5f));
    if (proc.value ("auto") > 0.5f) g.drawText ("compensa " + juce::String (proc.drive.autoDb.load(), 1) + " dB", juce::Rectangle<float> (1030, 528, 140, 22), juce::Justification::centredRight);
    auto meter = [&] (juce::Rectangle<float> r, float l, float rr, const char* t)
    {
        g.setColour (C::dim); g.setFont (mf (8.5f)); g.drawText (t, r.removeFromBottom (12), juce::Justification::centred);
        lcd (g, r, 3);
        for (int c = 0; c < 2; ++c)
        {
            auto m = juce::Rectangle<float> (r.getX() + 3 + c * (r.getWidth() / 2 - 1), r.getY() + 3, r.getWidth() / 2 - 5, r.getHeight() - 6);
            const float db = juce::Decibels::gainToDecibels (c ? rr : l, -60.0f), n = juce::jlimit (0.0f, 1.0f, (db + 60) / 63);
            const int seg = 24;
            for (int k = 0; k < seg; ++k) { const float t2 = k / (float) seg; g.setColour (t2 <= n ? (t2 > 0.93f ? C::red : t2 > 0.8f ? C::amber : C::phos) : neo::K::hi.withAlpha (0.05f)); g.fillRect (m.getX(), m.getBottom() - (k + 1) * m.getHeight() / seg + 1, m.getWidth(), m.getHeight() / seg - 1.5f); }
        }
    };
    meter ({ 1124, 560, 24, 196 }, proc.inPk[0].load(), proc.inPk[1].load(), "IN");
    meter ({ 1154, 560, 24, 196 }, proc.outPk[0].load(), proc.outPk[1].load(), "OUT");
    g.setColour (C::dim); g.setFont (uf (10.0f, false));
    g.drawFittedText ("PITCH > DRIVE: satura la voce gia spostata (piu naturale). DRIVE > PITCH: sposti anche la distorsione.", juce::Rectangle<int> (892, 722, 222, 44), juce::Justification::topLeft, 3);
    skin::chipDraw (g, themeRect.toFloat());
}

void Editor::mouseMove (const juce::MouseEvent& e0)
{
    const auto e = e0.withNewPosition (e0.position / scaler.s);
    const bool h = abRect.contains (e.getPosition());
    if (h != abHover) { abHover = h; repaint (scaler.map (abRect.expanded (8))); }
    setMouseCursor (h || presetRect.contains (e.getPosition()) || prevRect.contains (e.getPosition()) || nextRect.contains (e.getPosition()) ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
}

void Editor::mouseDown (const juce::MouseEvent& e0)
{
    { const auto tp = e0.position / scaler.s; if (themeRect.toFloat().contains (tp)) { skin::cycle(); C::sync(); for (auto* ch : getChildren()) ch->repaint(); repaint(); return; } }
    const auto e = e0.withNewPosition (e0.position / scaler.s);
    const int n = (int) presets().size();
    if (abRect.contains (e.getPosition())) { proc.ab.toggle (proc); selectBand (band); }
    else if (prevRect.contains (e.getPosition())) { proc.loadPreset ((proc.preset + n - 1) % n); selectBand (0); }
    else if (nextRect.contains (e.getPosition())) { proc.loadPreset ((proc.preset + 1) % n); selectBand (0); }
    else if (presetRect.contains (e.getPosition()))
    {
        juce::PopupMenu m; std::vector<std::pair<juce::String, juce::PopupMenu>> groups;
        for (int i = 0; i < n; ++i)
        {
            const juce::String name = presets()[(size_t) i].name;
            if (! name.contains (" | ")) { m.addItem (i + 1, name, true, i == proc.preset); continue; }
            const auto gn = name.upToFirstOccurrenceOf (" | ", false, false), it = name.fromFirstOccurrenceOf (" | ", false, false);
            bool found = false; for (auto& gr : groups) if (gr.first == gn) { gr.second.addItem (i + 1, it, true, i == proc.preset); found = true; }
            if (! found) { juce::PopupMenu pm; pm.addItem (i + 1, it, true, i == proc.preset); groups.push_back ({ gn, pm }); }
        }
        for (auto& gr : groups) m.addSubMenu (gr.first, gr.second);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withTargetScreenArea (localAreaToGlobal (scaler.map (presetRect))), [this] (int r) { if (r > 0) { proc.loadPreset (r - 1); selectBand (0); } });
    }
}

void Editor::timerCallback()
{
    const int mode = juce::roundToInt (proc.value ("mode"));
    sKey->enabled = sScale->enabled = mode != TRASPONI; sScale->enabled = mode == QUANTIZZA;
    pitchView.trace[pitchView.tw] = proc.value ("pitch") + proc.pitch.corrSemi.load() + proc.pitch.driftSemi.load(); pitchView.tw = (pitchView.tw + 1) % (int) std::size (pitchView.trace);
    driftView.trace[driftView.tw] = proc.pitch.driftSemi.load(); driftView.tw = (driftView.tw + 1) % (int) std::size (driftView.trace);
    driveView.update ((int) bv ("model"), bv ("drive"), bv ("tone"));
    { const bool sy = proc.value ("eSync") > 0.5f; kEDiv->setVisible (sy); kEMs->setVisible (! sy); }
    // toccare una manopola dell'echo lo accende (prima serviva anche il tasto ECHO, e sembrava rotto)
    if (proc.value ("eOn") < 0.5f && juce::ModifierKeys::currentModifiers.isAnyMouseButtonDown())
        for (auto* k : { kEDiv.get(), kEMs.get(), kEFb.get(), kEMix.get() })
            if (k->isMouseButtonDown()) { if (auto* p = proc.apvts.getParameter ("eOn")) p->setValueNotifyingHost (1.0f); break; }
    focusView.analyse();
    kDrive->activity = juce::jlimit (0.0f, 1.0f, juce::jmax (proc.outPk[0].load(), proc.outPk[1].load()) * bv ("drive") * 1.5f);
    kDrive->activity = juce::jlimit (0.0f, 1.0f, kDrive->activity);
    for (juce::Component* c : std::initializer_list<juce::Component*> { &pitchView, &driftView, &driveView, &focusView, &grid, sKey.get(), sScale.get(), sMode.get(), sDriftT.get(), sOrder.get(),
                                                                        wLink.get(), wSolo.get(), wByp.get(), wAuto.get(), kPitch.get(), kFormant.get(), kPMix.get(), kDrift.get(), kRate.get(), kDrive.get(), kTone.get(), kBMix.get(), kBLvl.get(), kMix.get(), kOut.get(),
                                                                        kEDiv.get(), kEMs.get(), kEFb.get(), kEMix.get(), kELo.get(), kEHi.get(), kESat.get(), wEOn.get(), wESync.get(), wEPing.get(), kVWide.get(), kVPan.get(), wVEcho.get() })
        c->repaint();
    if (++tick % 3 == 0) { repaint (scaler.map ({ 590, 78, 592, 362 }).withTrimmedLeft (0).withHeight (30)); repaint (scaler.map ({ 880, 452, 302, 318 })); repaint (scaler.map ({ 590, 360, 592, 80 })); repaint (scaler.map ({ 18, 400, 562, 40 })); }
}
}
