#include "MotionEditor.h"
#include <complex>

namespace jmo
{
juce::Font uf (float size, bool bold) { return jlook::ui (size, bold ? 2 : 1); }
juce::Font mf (float size) { return jlook::mono (size); }

void glowText (juce::Graphics& g, const juce::String& t, juce::Rectangle<float> r, juce::Colour c, juce::Font f, juce::Justification j)
{
    g.setFont (f);
    g.setColour (c.withAlpha (0.2f)); g.drawText (t, r.translated (-0.7f, 0.3f), j); g.drawText (t, r.translated (0.7f, 0.3f), j);
    g.setColour (c); g.drawText (t, r, j);
}
void lcd (juce::Graphics& g, juce::Rectangle<float> r, float rad)
{
    g.setColour (juce::Colours::black.withAlpha (0.65f)); g.fillRoundedRectangle (r.expanded (1.5f), rad + 1.5f);
    juce::ColourGradient cg (C::lcd1, r.getCentreX(), r.getCentreY(), C::lcd2, r.getX(), r.getY(), true); g.setGradientFill (cg); g.fillRoundedRectangle (r, rad);
    g.setColour (C::phos.withAlpha (0.03f));
    for (float x = r.getX() + 10; x < r.getRight(); x += 12) g.drawVerticalLine ((int) x, r.getY() + 2, r.getBottom() - 2);
    for (float y = r.getY() + 10; y < r.getBottom(); y += 12) g.drawHorizontalLine ((int) y, r.getX() + 2, r.getRight() - 2);
}
static juce::Colour fxCol (int f) { return juce::Colour (fxInfo (f).colour); }
static juce::String hzT (float f) { return f >= 1000 ? juce::String (f / 1000.0f, 1) + " kHz" : juce::String (juce::roundToInt (f)) + " Hz"; }

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
    float v = juce::jlimit (0.0f, 1.0f, start - e.getDistanceFromDragStartY() / (e.mods.isShiftDown() ? 900.0f : (steps > 0 ? 400.0f : 220.0f)));
    if (steps > 0) v = std::round (v * steps) / steps;
    param->setValueNotifyingHost (v);
}
void Knob::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w)
{
    if (! enabled) return;
    const float st = steps > 0 ? 1.0f / steps : 0.01f;
    param->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, param->getValue() + (w.deltaY > 0 ? st : -st)));
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
        else { g.setColour (enabled ? C::dim : C::faint); g.setFont (uf (font)); g.drawText (items[i], r, juce::Justification::centred); }
    }
}
void Segs::mouseDown (const juce::MouseEvent& e)
{
    if (! enabled) return;
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

// ======================================================================= scelta effetto
void FxGrid::paint (juce::Graphics& g)
{
    const int cur = (int) ed.proc.value ("fx");
    for (int m = 0; m < kNumFx; ++m)
    {
        auto r = cell (m); const auto c = fxCol (m); const bool on = m == cur;
        g.setColour (juce::Colours::black.withAlpha (0.55f)); g.fillRoundedRectangle (r.expanded (1), 7);
        juce::ColourGradient cg (on ? c.withAlpha (0.34f) : juce::Colour (0xff1c1f26), r.getX(), r.getY(), on ? c.withAlpha (0.10f) : juce::Colour (0xff111318), r.getX(), r.getBottom(), false);
        g.setGradientFill (cg); g.fillRoundedRectangle (r, 7);
        g.setColour (on ? c : (hov == m ? c.withAlpha (0.6f) : juce::Colours::white.withAlpha (0.08f))); g.drawRoundedRectangle (r.reduced (0.5f), 7, on ? 1.5f : 1.0f);
        auto led = juce::Rectangle<float> (r.getCentreX() - 3, r.getY() + 8, 6, 6);
        if (on) { g.setColour (c.withAlpha (0.4f)); g.fillEllipse (led.expanded (3)); }
        g.setColour (on ? c : c.withAlpha (0.35f)); g.fillEllipse (led);
        g.setFont (uf (11.5f)); g.setColour (on ? c.brighter (0.3f) : (hov == m ? C::text : C::dim));
        g.drawFittedText (fxInfo (m).name, r.withTrimmedTop (16).withHeight (18).toNearestInt(), juce::Justification::centred, 1, 0.7f);
        g.setFont (mf (8.5f)); g.setColour (on ? c.withAlpha (0.8f) : C::faint);
        g.drawFittedText (fxInfo (m).sub, r.withTrimmedTop (34).reduced (4, 0).toNearestInt(), juce::Justification::centredTop, 2, 0.7f);
    }
}
void FxGrid::mouseDown (const juce::MouseEvent& e)
{
    for (int m = 0; m < kNumFx; ++m) if (cell (m).contains (e.position)) { auto* p = ed.P ("fx"); p->setValueNotifyingHost (p->convertTo0to1 ((float) m)); ed.repaint(); return; }
}
void FxGrid::mouseMove (const juce::MouseEvent& e)
{
    int h = -1; for (int m = 0; m < kNumFx; ++m) if (cell (m).contains (e.position)) h = m;
    if (h != hov) { hov = h; repaint(); }
    setMouseCursor (h >= 0 ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
}

// ======================================================================= display dell'effetto
void FxView::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat(); lcd (g, b, 10);
    auto& P = ed.proc; const int fx = (int) P.value ("fx"); const auto c = fxCol (fx);
    const float lfo = P.eng.lfoNow.load(), depth = P.value ("depth"), ch = P.value ("char"), fb = P.value ("fb"), mix = P.value ("mix");
    auto plot = b.reduced (16, 14).withTrimmedBottom (46);
    g.setColour (C::dim); g.setFont (mf (9.0f));
    // risposta in frequenza animata per phaser e flanger
    auto curve = [&] (std::function<float (float)> magDb, float range)
    {
        juce::Path p; const int N = 160;
        for (int i = 0; i <= N; ++i)
        {
            const float f = 20.0f * std::pow (1000.0f, i / (float) N);
            const float db = juce::jlimit (-range, 6.0f, magDb (f));
            const float x = plot.getX() + plot.getWidth() * i / N, y = plot.getY() + plot.getHeight() * (6.0f - db) / (6.0f + range);
            if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
        }
        juce::Path f2 = p; f2.lineTo (plot.getRight(), plot.getBottom()); f2.lineTo (plot.getX(), plot.getBottom()); f2.closeSubPath();
        juce::ColourGradient cg (c.withAlpha (0.25f), 0, plot.getY(), c.withAlpha (0.02f), 0, plot.getBottom(), false); g.setGradientFill (cg); g.fillPath (f2);
        g.setColour (c.withAlpha (0.3f)); g.strokePath (p, juce::PathStrokeType (4.0f)); g.setColour (c); g.strokePath (p, juce::PathStrokeType (1.6f));
        for (float hz : { 100.0f, 1000.0f, 10000.0f }) { const float x = plot.getX() + plot.getWidth() * std::log (hz / 20.0f) / std::log (1000.0f); g.setColour (C::phos.withAlpha (0.12f)); g.drawVerticalLine ((int) x, plot.getY(), plot.getBottom()); g.setColour (C::dim); g.drawText (hzT (hz), juce::Rectangle<float> (x + 3, plot.getBottom() - 12, 60, 12), juce::Justification::centredLeft); }
    };
    const double sr = juce::jmax (8000.0, P.sampleRate());
    if (fx == PHASER)
    {
        const int stages = 2 + 2 * juce::jlimit (0, 5, (int) (ch * 5.99f));
        const float fc = 900.0f * std::pow (2.0f, lfo * (0.4f + 3.1f * depth) * 0.5f + 0.2f);
        const float t = std::tan (juce::MathConstants<float>::pi * fc / (float) sr), a = (t - 1) / (t + 1);
        curve ([&] (float f)
        {
            const std::complex<float> z = std::polar (1.0f, -kTwoPi * f / (float) sr);
            std::complex<float> ap (1.0f, 0.0f); const auto one = (a + z) / (1.0f + a * z);
            for (int k = 0; k < stages; ++k) ap *= one;
            const auto h = 0.5f * (1.0f + ap / (1.0f - fb * ap)) * 1.4f;
            return juce::Decibels::gainToDecibels (std::abs (h) * mix + (1 - mix), -60.0f);
        }, 30.0f);
        glowText (g, juce::String (stages) + " STADI  " + juce::String (juce::roundToInt (fc)) + " Hz", plot.withHeight (14), c, mf (10.0f), juce::Justification::centredRight);
    }
    else if (fx == FLANGER)
    {
        const float T = 0.0001f + ch * 0.006f + 0.004f * depth * (0.5f + 0.5f * lfo);
        curve ([&] (float f)
        {
            const auto z = std::polar (1.0f, -kTwoPi * f * T);
            const auto h = 0.5f * (1.0f + z / (1.0f - fb * z)) * 1.3f;
            return juce::Decibels::gainToDecibels (std::abs (h) * mix + (1 - mix), -60.0f);
        }, 30.0f);
        glowText (g, juce::String (T * 1000.0f, 2) + " ms", plot.withHeight (14), c, mf (10.0f), juce::Justification::centredRight);
    }
    else if (fx == TREMOLO || fx == VIBRATO)
    {
        // due cicli dell'LFO, sinistra e destra, con il punto che scorre
        const int shape = (int) P.value ("shape"); const float st = P.value ("stereo") / 360.0f, ph = P.eng.lfoPh.load();
        Lfo l; for (int side = 0; side < 2; ++side)
        {
            juce::Path p;
            for (int i = 0; i <= 200; ++i)
            {
                const double pp = i / 100.0 + (side ? st : 0);
                float v = l.at (pp, shape == RANDOM || shape == SH ? SINE : shape, fx == TREMOLO ? ch : 0.0f);
                v = fx == TREMOLO ? 1.0f - depth * (0.5f - 0.5f * v) : 0.5f + 0.5f * v * depth;
                const float x = plot.getX() + plot.getWidth() * i / 200.0f, y = plot.getBottom() - plot.getHeight() * (fx == TREMOLO ? v : v);
                if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
            }
            g.setColour ((side ? C::phos : c).withAlpha (0.85f)); g.strokePath (p, juce::PathStrokeType (side ? 1.2f : 2.0f));
        }
        const float x = plot.getX() + plot.getWidth() * ph * 0.5f;
        g.setColour (C::amber); g.fillRect (x - 1, plot.getY(), 2.0f, plot.getHeight());
        g.setColour (C::dim); g.drawText (fx == TREMOLO ? "VOLUME   (colore = sinistra, azzurro = destra)" : "INTONAZIONE   (colore = sinistra, azzurro = destra)", plot.withHeight (14), juce::Justification::centredLeft);
    }
    else
    {
        // campo stereo: ogni voce e un punto (sinistra-destra = posizione, altezza = ritardo)
        const float cx = plot.getCentreX(), base = plot.getBottom() - 8, rad = juce::jmin (plot.getWidth() * 0.48f, plot.getHeight() - 16);
        g.setColour (C::phos.withAlpha (0.12f));
        for (int k = 1; k <= 3; ++k) { juce::Path a; a.addCentredArc (cx, base, rad * k / 3, rad * k / 3, 0, -juce::MathConstants<float>::halfPi, juce::MathConstants<float>::halfPi, true); g.strokePath (a, juce::PathStrokeType (1.0f)); }
        g.drawLine (cx, base, cx, base - rad); g.setColour (C::dim); g.drawText ("L", juce::Rectangle<float> (cx - rad - 14, base - 14, 12, 12), juce::Justification::centred); g.drawText ("R", juce::Rectangle<float> (cx + rad + 2, base - 14, 12, 12), juce::Justification::centred);
        struct V { float pan, dist, size; };
        std::vector<V> vs;
        const float width = P.value ("width");
        auto vp = [&] (int i) { return P.eng.voicePh[i].load(); };
        if (fx == CHORUS) { const int n = 1 + juce::jlimit (0, 3, (int) (ch * 3.99f)); for (int s = 0; s < 2; ++s) for (int v = 0; v < n; ++v) { const float m = std::sin (kTwoPi * (P.eng.lfoPh.load() + v / (float) n + s * 0.25f)) * depth; vs.push_back ({ (s ? 0.75f : -0.75f) * width * (0.6f + 0.4f * v / juce::jmax (1, n - 1)), 0.4f + 0.1f * v + 0.12f * m, 1.0f }); } }
        else if (fx == DIMENSION) { const float t = lfo * (0.3f + depth); vs.push_back ({ -0.6f * width, 0.35f + 0.1f * t, 1.2f }); vs.push_back ({ 0.6f * width, 0.35f - 0.1f * t, 1.2f }); }
        else if (fx == OMNIVOX) { static const float pans[6] = { -1.0f, 1.0f, -0.55f, 0.55f, -0.15f, 0.15f }; for (int v = 0; v < 6; ++v) vs.push_back ({ pans[v] * 0.9f * width, 0.25f + 0.1f * v * (0.4f + ch) + 0.08f * vp (v) * depth, 0.9f }); }
        else if (fx == DOUBLER) { vs.push_back ({ -0.85f * width, 0.3f + 0.4f * ch + 0.05f * vp (0) * depth, 1.1f }); vs.push_back ({ 0.85f * width, 0.3f + 0.55f * ch + 0.05f * vp (1) * depth, 1.1f }); }
        else if (fx == TRIVOX) { vs.push_back ({ -0.8f * width, 0.3f + 0.4f * ch, 1.1f }); vs.push_back ({ 0.8f * width, 0.3f + 0.5f * ch, 1.1f }); vs.push_back ({ 0.0f, 0.2f + 0.2f * ch, 0.8f }); }
        // dry al centro
        g.setColour (C::text.withAlpha (0.5f + 0.5f * (1 - mix))); g.fillEllipse (cx - 6, base - 6, 12, 12);
        for (auto& v : vs)
        {
            const float ang = juce::jlimit (-1.0f, 1.0f, v.pan) * juce::MathConstants<float>::halfPi * 0.95f, r = rad * juce::jlimit (0.1f, 1.0f, v.dist);
            const float x = cx + std::sin (ang) * r, y = base - std::cos (ang) * r, s = 9.0f * v.size * (0.4f + 0.6f * mix);
            g.setColour (c.withAlpha (0.25f * mix + 0.05f)); g.fillEllipse (x - s * 1.8f, y - s * 1.8f, s * 3.6f, s * 3.6f);
            g.setColour (c.withAlpha (0.4f + 0.6f * mix)); g.fillEllipse (x - s * 0.5f, y - s * 0.5f, s, s);
            g.setColour (c.withAlpha (0.15f)); g.drawLine (cx, base, x, y, 1.0f);
        }
        g.setColour (C::dim);
        g.drawText (fx == TRIVOX ? "+/- " + juce::String (3.0f + 27.0f * depth, 0) + " cent" : fx == DOUBLER ? juce::String (10.0f + 45.0f * ch, 0) + " ms" : juce::String ((int) vs.size()) + " voci",
                    plot.withHeight (14), juce::Justification::centredRight);
        g.drawText ("distanza dal centro = ritardo", plot.withHeight (14), juce::Justification::centredLeft);
    }
    // striscia LFO in basso
    auto st = b.reduced (16, 0).withTrimmedTop (b.getHeight() - 40).withHeight (26);
    g.setColour (juce::Colours::black.withAlpha (0.35f)); g.fillRoundedRectangle (st, 4);
    juce::Path tp; const int n = (int) std::size (trace);
    for (int i = 0; i < n; ++i) { const float v = trace[(tw + i) % n]; const float x = st.getX() + st.getWidth() * i / (n - 1), y = st.getCentreY() - v * st.getHeight() * 0.42f; if (i == 0) tp.startNewSubPath (x, y); else tp.lineTo (x, y); }
    g.setColour (c.withAlpha (0.8f)); g.strokePath (tp, juce::PathStrokeType (1.3f));
    const bool sync = P.value ("sync") > 0.5f;
    glowText (g, juce::String ("LFO  ") + (sync ? divNames()[(int) P.value ("div")] + juce::String (" a ") + juce::String (P.uiBpm.load(), 0) + " BPM" : juce::String (P.value ("rate"), 2) + " Hz"),
              st.reduced (8, 0), C::text.withAlpha (0.75f), mf (9.5f), juce::Justification::centredRight);
    if (mix < 0.001f) glowText (g, "ALZA MIX PER SENTIRE L'EFFETTO", b.withHeight (b.getHeight() - 50), C::amber, uf (14.0f), juce::Justification::centred);
}

// ======================================================================= autopan
void PanView::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat(); lcd (g, b, 10);
    auto& P = ed.proc; const bool on = P.value ("pan") > 0.5f;
    const float cx = b.getCentreX(), base = b.getBottom() - 22, rad = juce::jmin (b.getWidth() * 0.44f, b.getHeight() - 40);
    juce::Path arc; arc.addCentredArc (cx, base, rad, rad, 0, -juce::MathConstants<float>::halfPi, juce::MathConstants<float>::halfPi, true);
    g.setColour (C::phos.withAlpha (0.15f)); g.strokePath (arc, juce::PathStrokeType (6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    const float dp = P.value ("pandepth");
    juce::Path span; span.addCentredArc (cx, base, rad, rad, 0, -juce::MathConstants<float>::halfPi * dp, juce::MathConstants<float>::halfPi * dp, true);
    g.setColour ((on ? C::magenta : C::faint).withAlpha (0.5f)); g.strokePath (span, juce::PathStrokeType (6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    const int n = (int) std::size (trail);
    for (int i = 0; i < n; ++i)
    {
        const float v = trail[(tw + i) % n], a = v * juce::MathConstants<float>::halfPi, al = (float) i / n;
        g.setColour (C::magenta.withAlpha (on ? 0.25f * al : 0.0f)); g.fillEllipse (cx + std::sin (a) * rad - 4, base - std::cos (a) * rad - 4, 8, 8);
    }
    const float a = P.eng.panNow.load() * juce::MathConstants<float>::halfPi;
    const float x = cx + std::sin (a) * rad, y = base - std::cos (a) * rad;
    if (on) { g.setColour (C::magenta.withAlpha (0.35f)); g.fillEllipse (x - 14, y - 14, 28, 28); }
    g.setColour (on ? C::magenta : C::faint); g.fillEllipse (x - 7, y - 7, 14, 14);
    g.setColour (C::dim); g.setFont (mf (10.0f));
    g.drawText ("L", juce::Rectangle<float> (cx - rad - 16, base - 8, 14, 14), juce::Justification::centred); g.drawText ("R", juce::Rectangle<float> (cx + rad + 2, base - 8, 14, 14), juce::Justification::centred);
    const float pv = P.eng.panNow.load();
    glowText (g, on ? (std::abs (pv) < 0.03f ? juce::String ("C") : (pv < 0 ? "L" : "R") + juce::String (juce::roundToInt (std::abs (pv) * 100))) : juce::String ("SPENTO"), { cx - 50, base - 30, 100, 20 }, on ? C::text : C::dim, mf (12.0f), juce::Justification::centred);
}

// ======================================================================= editor
Editor::Editor (Processor& p) : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);
    auto knob = [this] (std::unique_ptr<Knob>& k, const char* id, juce::Colour c, const char* label, bool big, std::function<juce::String()> txt = nullptr, bool bip = false)
    { k = std::make_unique<Knob> (P (id), c); k->label = label; k->big = big; k->bipolar = bip; k->text = std::move (txt); addAndMakeVisible (*k); };
    auto pct = [this] (const char* id) { return [this, id] { return juce::String (juce::roundToInt (proc.value (id) * 100)) + "%"; }; };
    knob (kRate, "rate", C::phos, "RATE", true, [this] { return juce::String (proc.value ("rate"), proc.value ("rate") < 1 ? 2 : 1) + " Hz"; });
    knob (kDiv, "div", C::phos, "RATE A TEMPO", true, [this] { return divNames()[(int) proc.value ("div")]; }); kDiv->steps = divNames().size() - 1;
    knob (kDepth, "depth", C::magenta, "DEPTH", true, pct ("depth"));
    knob (kChar, "char", C::amber, "CARATTERE", true);
    kChar->text = [this]
    {
        const int fx = (int) proc.value ("fx"); const float c = proc.value ("char");
        switch (fx)
        {
            case PHASER: return juce::String (2 + 2 * juce::jlimit (0, 5, (int) (c * 5.99f))) + " stadi";
            case CHORUS: return juce::String (1 + juce::jlimit (0, 3, (int) (c * 3.99f))) + " voci";
            case DIMENSION: return "modo " + juce::String (1 + juce::jlimit (0, 3, (int) (c * 3.99f)));
            case FLANGER: return juce::String (0.1f + c * 6.0f, 1) + " ms";
            case DOUBLER: return juce::String (10.0f + 45.0f * c, 0) + " ms";
            case TRIVOX: return juce::String (c * 30.0f, 0) + " ms";
            default: return juce::String (juce::roundToInt (c * 100)) + "%";
        }
    };
    knob (kFb, "fb", C::violet, "FEEDBACK", false, [this] { return juce::String (juce::roundToInt (proc.value ("fb") * 100)) + "%"; }, true);
    knob (kStereo, "stereo", C::phos, "STEREO", false, [this] { return juce::String (juce::roundToInt (proc.value ("stereo"))) + juce::String::fromUTF8 ("\xc2\xb0"); });
    knob (kMix, "mix", C::green, "MIX", true, pct ("mix"));
    knob (kTone, "tone", C::amber, "TONO", false, [this] { return proc.value ("tone") >= 19900 ? juce::String ("APERTO") : hzT (proc.value ("tone")); });
    knob (kWidth, "width", C::phos, "LARGHEZZA", false, pct ("width"));
    knob (kBass, "bassmono", C::magenta, "BASSI MONO", false, [this] { return proc.value ("bassmono") <= 21 ? juce::String ("OFF") : juce::String::fromUTF8 ("< ") + hzT (proc.value ("bassmono")); });
    knob (kOut, "out", C::text, "OUTPUT", false, [this] { return juce::String (proc.value ("out"), 1) + " dB"; }, true);
    knob (kPanRate, "panrate", C::magenta, "RATE", false, [this] { return juce::String (proc.value ("panrate"), 2) + " Hz"; });
    knob (kPanDiv, "pandiv", C::magenta, "RATE A TEMPO", false, [this] { return divNames()[(int) proc.value ("pandiv")]; }); kPanDiv->steps = divNames().size() - 1;
    knob (kPanDepth, "pandepth", C::magenta, "AMPIEZZA", false, pct ("pandepth"));
    sShape = std::make_unique<Segs> (P ("shape"), C::phos, juce::StringArray { "SINE", "TRI", "QUADRA", "CASUALE", "S&H" }); sShape->font = 10.0f; addAndMakeVisible (*sShape);
    sPanShape = std::make_unique<Segs> (P ("panshape"), C::magenta, juce::StringArray { "SINE", "TRI", "QUADRA", "CAS.", "S&H" }); sPanShape->font = 9.5f; addAndMakeVisible (*sPanShape);
    wSync = std::make_unique<Switch> (P ("sync"), C::phos, "A TEMPO CON LA DAW"); addAndMakeVisible (*wSync);
    wPan = std::make_unique<Switch> (P ("pan"), C::magenta, "AUTOPAN"); addAndMakeVisible (*wPan);
    wPanSync = std::make_unique<Switch> (P ("pansync"), C::magenta, "A TEMPO"); addAndMakeVisible (*wPanSync);
    addAndMakeVisible (grid); addAndMakeVisible (view); addAndMakeVisible (panView);
    addAndMakeVisible (sizeChip);
    scaler.setup();
    startTimerHz (30);
}

void Editor::resized()
{
    const float sc = scaler.update();
    presetRect = { 450, 15, 330, 36 }; prevRect = { 412, 15, 36, 36 }; nextRect = { 782, 15, 36, 36 }; abRect = { 830, 15, 110, 36 };
    grid.setBounds (34, 108, 1132, 62);
    kRate->setBounds (34, 186, 130, 130 + 32); kDiv->setBounds (34, 186, 130, 130 + 32);
    kDepth->setBounds (180, 186, 130, 130 + 32); kChar->setBounds (326, 186, 130, 130 + 32);
    kFb->setBounds (40, 360, 84, 84 + 27); kStereo->setBounds (140, 360, 84, 84 + 27);
    wSync->setBounds (246, 362, 210, 22); sShape->setBounds (246, 394, 230, 28);
    view.setBounds (496, 186, 670, 262);
    // movimento
    wPan->setBounds (34, 532, 140, 22); wPanSync->setBounds (186, 532, 110, 22);
    kPanRate->setBounds (34, 566, 96, 96 + 27); kPanDiv->setBounds (34, 566, 96, 96 + 27); kPanDepth->setBounds (146, 566, 96, 96 + 27);
    sPanShape->setBounds (34, 700, 240, 28);
    panView.setBounds (288, 524, 276, 232);
    // uscita
    kMix->setBounds (606, 530, 130, 130 + 32);
    kTone->setBounds (752, 548, 86, 86 + 27); kWidth->setBounds (846, 548, 86, 86 + 27); kBass->setBounds (940, 548, 86, 86 + 27); kOut->setBounds (1034, 548, 76, 76 + 27);
    sizeChip.setBounds (W - 76, H - 18, 58, 15);
    jlook::scaleChildren (*this, sc);
}

void Editor::paint (juce::Graphics& g)
{
    g.addTransform (juce::AffineTransform::scale (scaler.s));
    const float Wf = (float) W, Hf = (float) H;
    const int fx = (int) proc.value ("fx"); const auto fc = fxCol (fx);
    jlook::backdrop (g, { 0, 0, Wf, Hf }, C::phos, C::magenta);
    jlook::header (g, { 0, 0, Wf, 64 }, "MOTION", C::phos, "JM-2  MODULAZIONE / PAN / CHORUS / DOUBLER", C::magenta, C::phos);
    lcd (g, presetRect.toFloat(), 8); jlook::glass (g, presetRect.toFloat(), 8);
    glowText (g, proc.getProgramName (proc.preset).toUpperCase(), presetRect.toFloat(), C::amber, mf (12.5f), juce::Justification::centred);
    g.setColour (C::phos); g.setFont (uf (20.0f));
    g.drawText (juce::String::fromUTF8 ("\xe2\x80\xb9"), prevRect, juce::Justification::centred); g.drawText (juce::String::fromUTF8 ("\xe2\x80\xba"), nextRect, juce::Justification::centred);
    jlook::button (g, abRect.toFloat(), C::phos, proc.ab.cur == 1, abHover, "CONFRONTO " + proc.ab.label(), uf (11.5f));
    auto card = [&] (juce::Rectangle<float> r, juce::Colour c, const juce::String& t, const juce::String& sub)
    {
        jlook::softShadow (g, r, 14); jlook::card (g, r, 14, c, true, false, false);
        glowText (g, t, { r.getX() + 16, r.getY() + 6, 400, 22 }, C::text, uf (13.0f));
        if (sub.isNotEmpty()) { g.setColour (C::dim); g.setFont (mf (9.0f)); g.drawText (sub, juce::Rectangle<float> (r.getX() + 16, r.getY() + 8, r.getWidth() - 32, 18), juce::Justification::centredRight); }
    };
    card ({ 18, 78, 1164, 402 }, fc, juce::String ("EFFETTO  ") + juce::String::fromUTF8 ("\xc2\xb7") + "  " + fxInfo (fx).name, fxInfo (fx).sub);
    card ({ 18, 492, 562, 274 }, C::magenta, "MOVIMENTO", "autopan, si somma all'effetto");
    card ({ 590, 492, 592, 274 }, C::green, "USCITA", "");
    // descrizione dell'effetto
    g.setColour (C::text.withAlpha (0.85f)); g.setFont (uf (10.5f, false));
    g.drawFittedText (fxInfo (fx).what, juce::Rectangle<int> (246, 428, 238, 44), juce::Justification::topLeft, 3);
    g.setColour (fc); g.setFont (uf (10.0f));
    g.drawFittedText (juce::String ("SU: ") + fxInfo (fx).use, juce::Rectangle<int> (500, 452, 666, 22), juce::Justification::centredLeft, 1);
    // uscita: note e correlazione
    g.setColour (C::dim); g.setFont (uf (10.0f, false));
    g.drawFittedText ("BASSI MONO tiene al centro le frequenze sotto il valore scelto: chorus e doubler allargano senza svuotare cassa e 808. TONO scurisce solo l'effetto.",
                      juce::Rectangle<int> (606, 712, 500, 44), juce::Justification::topLeft, 3);
    const float co = proc.corr.load();
    auto cb = juce::Rectangle<float> (752, 690, 358, 10);
    g.setColour (C::dim); g.setFont (mf (9.0f)); g.drawText ("CORRELAZIONE  " + juce::String (co, 2) + (co < 0 ? "  (attento al mono)" : ""), juce::Rectangle<float> (752, 674, 358, 14), juce::Justification::centredLeft);
    lcd (g, cb, 3); g.setColour (co < 0 ? C::red : C::green); g.fillRect (juce::Rectangle<float> (cb.getCentreX(), cb.getY() + 2, cb.getWidth() * 0.5f * co, cb.getHeight() - 4).withWidth (std::abs (cb.getWidth() * 0.5f * co)).withX (co < 0 ? cb.getCentreX() + cb.getWidth() * 0.5f * co : cb.getCentreX()));
    auto meter = [&] (juce::Rectangle<float> r, float l, float rr, const char* t)
    {
        g.setColour (C::dim); g.setFont (mf (8.5f)); g.drawText (t, r.removeFromBottom (12), juce::Justification::centred);
        lcd (g, r, 3);
        for (int c = 0; c < 2; ++c)
        {
            auto m = juce::Rectangle<float> (r.getX() + 3 + c * (r.getWidth() / 2 - 1), r.getY() + 3, r.getWidth() / 2 - 5, r.getHeight() - 6);
            const float db = juce::Decibels::gainToDecibels (c ? rr : l, -60.0f), n = juce::jlimit (0.0f, 1.0f, (db + 60) / 63);
            for (int k = 0; k < 24; ++k) { const float t2 = k / 24.0f; g.setColour (t2 <= n ? (t2 > 0.93f ? C::red : t2 > 0.8f ? C::amber : C::phos) : juce::Colours::white.withAlpha (0.05f)); g.fillRect (m.getX(), m.getBottom() - (k + 1) * m.getHeight() / 24 + 1, m.getWidth(), m.getHeight() / 24 - 1.5f); }
        }
    };
    meter ({ 1124, 530, 24, 200 }, proc.inPk[0].load(), proc.inPk[1].load(), "IN");
    meter ({ 1152, 530, 24, 200 }, proc.outPk[0].load(), proc.outPk[1].load(), "OUT");
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
    const auto e = e0.withNewPosition (e0.position / scaler.s);
    const int n = (int) presets().size();
    if (abRect.contains (e.getPosition())) { proc.ab.toggle (proc); repaint(); }
    else if (prevRect.contains (e.getPosition())) { proc.loadPreset ((proc.preset + n - 1) % n); repaint(); }
    else if (nextRect.contains (e.getPosition())) { proc.loadPreset ((proc.preset + 1) % n); repaint(); }
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
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withTargetScreenArea (localAreaToGlobal (scaler.map (presetRect))), [this] (int r) { if (r > 0) { proc.loadPreset (r - 1); repaint(); } });
    }
}

void Editor::timerCallback()
{
    const int fx = (int) proc.value ("fx");
    const bool sync = proc.value ("sync") > 0.5f, psync = proc.value ("pansync") > 0.5f, pan = proc.value ("pan") > 0.5f;
    kRate->setVisible (! sync); kDiv->setVisible (sync); kPanRate->setVisible (! psync); kPanDiv->setVisible (psync);
    const bool usesFb = fx == PHASER || fx == FLANGER;
    kFb->enabled = usesFb; kFb->label = usesFb ? fxInfo (fx).fbName : "FEEDBACK";
    kChar->label = fxInfo (fx).charName; kChar->col = fxCol (fx); kDepth->col = fxCol (fx);
    kStereo->enabled = fx == TREMOLO || fx == PHASER || fx == FLANGER || fx == CHORUS || fx == VIBRATO;
    kPanRate->enabled = kPanDiv->enabled = kPanDepth->enabled = sPanShape->enabled = pan;
    view.trace[view.tw] = proc.eng.lfoNow.load(); view.tw = (view.tw + 1) % (int) std::size (view.trace);
    panView.trail[panView.tw] = proc.eng.panNow.load(); panView.tw = (panView.tw + 1) % (int) std::size (panView.trail);
    view.repaint(); panView.repaint();
    for (Knob* k : { kRate.get(), kDiv.get(), kDepth.get(), kChar.get(), kFb.get(), kStereo.get(), kMix.get(), kTone.get(), kWidth.get(), kBass.get(), kOut.get(), kPanRate.get(), kPanDiv.get(), kPanDepth.get() })
    {
        const float v = k->param->getValue() + (k->enabled ? 0.0f : 7.0f) + (float) fx * 13.0f;
        if (v != k->shown) { k->shown = v; k->repaint(); }
    }
    if (++tick % 4 == 0)
    {
        for (juce::Component* c : std::initializer_list<juce::Component*> { &grid, sShape.get(), sPanShape.get(), wSync.get(), wPan.get(), wPanSync.get() }) c->repaint();
        repaint (scaler.map ({ 18, 78, 1164, 30 })); repaint (scaler.map ({ 240, 420, 250, 60 })); repaint (scaler.map ({ 496, 448, 670, 30 })); repaint (scaler.map ({ 740, 640, 440, 120 }));
    }
}
}
