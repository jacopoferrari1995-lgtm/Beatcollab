#pragma once
// Stile NEO della modalita FACILE: vetro scuro, anelli di luce, gradienti magenta -> viola -> ciano, tipografia pulita.
#include "JackpotLook.h"

namespace neo
{
namespace K
{
    const juce::Colour bg0 { 0xff05060b }, bg1 { 0xff0b0d17 }, ink { 0xffeef1f8 }, sub { 0xff8a91a6 }, faint { 0xff4a5064 }, line { 0x14ffffff };
    const juce::Colour pink { 0xffff3d9a }, violet { 0xff8b5cff }, cyan { 0xff3ee8ff }, mint { 0xff4dffb4 }, amber { 0xffffb547 }, red { 0xffff4d6a }, blue { 0xff4d8dff };
}
inline juce::Font f (float s, int w = 1) { return jlook::ui (s, w); }
inline juce::Font num (float s) { return jlook::mono (s); }

// pannello di vetro: riempimento traslucido, filo di luce in alto, bordo sottile
inline void glass (juce::Graphics& g, juce::Rectangle<float> r, float rad = 18, juce::Colour tint = juce::Colours::white, float strength = 1.0f)
{
    for (int k = 4; k >= 1; --k) { g.setColour (juce::Colours::black.withAlpha (0.10f)); g.fillRoundedRectangle (r.translated (0, 2.0f * k).expanded (k * 1.5f), rad + k); }
    juce::ColourGradient body (juce::Colour (0xff141826).withAlpha (0.92f), r.getX(), r.getY(), juce::Colour (0xff0b0d16).withAlpha (0.95f), r.getX(), r.getBottom(), false);
    g.setGradientFill (body); g.fillRoundedRectangle (r, rad);
    juce::ColourGradient tg (tint.withAlpha (0.07f * strength), r.getX(), r.getY(), tint.withAlpha (0.0f), r.getX(), r.getY() + juce::jmin (160.0f, r.getHeight()), false);
    g.setGradientFill (tg); g.fillRoundedRectangle (r, rad);
    g.setColour (juce::Colours::white.withAlpha (0.07f)); g.drawRoundedRectangle (r.reduced (0.5f), rad, 1.0f);
    juce::ColourGradient hl (juce::Colours::white.withAlpha (0.0f), r.getX() + rad, r.getY(), juce::Colours::white.withAlpha (0.0f), r.getRight() - rad, r.getY(), false);
    hl.addColour (0.5, juce::Colours::white.withAlpha (0.22f)); g.setGradientFill (hl); g.fillRect (r.getX() + rad, r.getY() + 0.5f, r.getWidth() - 2 * rad, 1.0f);
}
// incavo (display)
inline void well (juce::Graphics& g, juce::Rectangle<float> r, float rad = 12)
{
    juce::ColourGradient b (juce::Colour (0xff04050a), r.getX(), r.getY(), juce::Colour (0xff080a12), r.getX(), r.getBottom(), false);
    g.setGradientFill (b); g.fillRoundedRectangle (r, rad);
    g.setColour (juce::Colours::black.withAlpha (0.6f)); g.drawRoundedRectangle (r.translated (0, 1), rad, 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.05f)); g.drawRoundedRectangle (r.reduced (0.5f), rad, 1.0f);
}
// tratto con alone (3 passate)
inline void glowStroke (juce::Graphics& g, const juce::Path& p, juce::Colour c, float w, float glow = 1.0f)
{
    g.setColour (c.withAlpha (0.07f * glow)); g.strokePath (p, juce::PathStrokeType (w * 5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (c.withAlpha (0.16f * glow)); g.strokePath (p, juce::PathStrokeType (w * 2.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (c); g.strokePath (p, juce::PathStrokeType (w, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}
inline void glowText (juce::Graphics& g, const juce::String& t, juce::Rectangle<float> r, juce::Colour c, juce::Font fo, juce::Justification j = juce::Justification::centredLeft)
{
    g.setFont (fo);
    g.setColour (c.withAlpha (0.18f)); for (float d : { -1.2f, 1.2f }) { g.drawText (t, r.translated (d, 0), j); g.drawText (t, r.translated (0, d), j); }
    g.setColour (c); g.drawText (t, r, j);
}
// pillola (bottone / segmento)
inline void pill (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& t, bool on, juce::Colour c, float fs = 10.5f, bool hover = false)
{
    const float rad = r.getHeight() * 0.5f;
    if (on)
    {
        g.setColour (c.withAlpha (0.18f)); g.fillRoundedRectangle (r.expanded (3), rad + 3);
        juce::ColourGradient bg (c.withAlpha (0.42f), r.getX(), r.getY(), c.withAlpha (0.22f), r.getX(), r.getBottom(), false); g.setGradientFill (bg); g.fillRoundedRectangle (r, rad);
        g.setColour (c.brighter (0.4f).withAlpha (0.9f)); g.drawRoundedRectangle (r.reduced (0.5f), rad, 1.0f);
    }
    else
    {
        g.setColour (juce::Colours::white.withAlpha (hover ? 0.08f : 0.04f)); g.fillRoundedRectangle (r, rad);
        g.setColour (juce::Colours::white.withAlpha (0.09f)); g.drawRoundedRectangle (r.reduced (0.5f), rad, 1.0f);
    }
    g.setFont (f (fs, 2)); g.setColour (on ? juce::Colours::white : K::sub); g.drawText (t, r, juce::Justification::centred);
}
inline juce::String fmt (float v, int d = 1, bool sign = true) { return (sign && v > 0 ? "+" : "") + juce::String (v, d); }

// ------------------------------------------------------------------------------------------------ manopola ad anello
class Knob : public juce::Component, public juce::SettableTooltipClient
{
public:
    Knob (juce::RangedAudioParameter* p, juce::String l, juce::Colour a, juce::Colour b) : param (p), label (std::move (l)), c1 (a), c2 (b) { setRepaintsOnMouseActivity (true); }
    void paint (juce::Graphics& g) override
    {
        const float W = (float) getWidth(), H = (float) getHeight();
        const float S = big ? juce::jmin (W, H - 26) : juce::jmin (W, H - 34);
        const float cx = W * 0.5f, cy = (big ? 0 : 0) + S * 0.5f, r = S * 0.5f - (big ? 14 : 8);
        const float a0 = -2.356f, a1 = 2.356f, v = param->getValue(), av = a0 + (a1 - a0) * v;
        const float tw = big ? 9.0f : 5.0f; const bool hov = isMouseOverOrDragging();
        // traccia
        { juce::Path t; t.addCentredArc (cx, cy, r, r, 0, a0, a1, true); g.setColour (juce::Colours::white.withAlpha (0.06f)); g.strokePath (t, juce::PathStrokeType (tw, juce::PathStrokeType::curved, juce::PathStrokeType::rounded)); }
        // valore
        const float from = bipolar ? 0.0f : a0;
        if (std::abs (av - from) > 0.01f)
        {
            juce::Path arc; arc.addCentredArc (cx, cy, r, r, 0, juce::jmin (from, av), juce::jmax (from, av), true);
            juce::ColourGradient cg (c1, cx - r, cy + r, c2, cx + r, cy - r, false);
            g.setGradientFill (cg); g.setOpacity (0.12f); g.strokePath (arc, juce::PathStrokeType (tw * 4.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setOpacity (0.25f); g.strokePath (arc, juce::PathStrokeType (tw * 2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setOpacity (1.0f); g.strokePath (arc, juce::PathStrokeType (tw, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        // attivita (per LOUD: quanto lavora il limiter) come corona esterna
        if (activity > 0.01f)
        {
            const float ro = r + tw + 5; const int nS = 36;
            for (int i = 0; i < nS; ++i)
            {
                const float t = (i + 0.5f) / nS; if (t > activity) break;
                const float a = a0 + (a1 - a0) * t; juce::Path s; s.addCentredArc (cx, cy, ro, ro, 0, a - 0.03f, a + 0.03f, true);
                g.setColour ((t > 0.75f ? K::red : K::pink).withAlpha (0.85f)); g.strokePath (s, juce::PathStrokeType (3.0f));
            }
        }
        // disco
        const float rd = r - tw - (big ? 8 : 4);
        for (int k = 3; k >= 1; --k) { g.setColour (juce::Colours::black.withAlpha (0.18f)); g.fillEllipse (cx - rd - k, cy - rd + k * 1.5f, (rd + k) * 2, (rd + k) * 2); }
        juce::ColourGradient dg (juce::Colour (0xff222739), cx - rd * 0.4f, cy - rd, juce::Colour (0xff0b0d15), cx + rd * 0.3f, cy + rd, false);
        g.setGradientFill (dg); g.fillEllipse (cx - rd, cy - rd, rd * 2, rd * 2);
        g.setColour (juce::Colours::white.withAlpha (hov ? 0.16f : 0.08f)); g.drawEllipse (cx - rd + 0.5f, cy - rd + 0.5f, rd * 2 - 1, rd * 2 - 1, 1.0f);
        // indicatore: punto luminoso
        const auto pc = c1.interpolatedWith (c2, v);
        const float pr = rd * 0.72f, px = cx + std::sin (av) * pr, py = cy - std::cos (av) * pr, ds = big ? 5.0f : 3.0f;
        g.setColour (pc.withAlpha (0.25f)); g.fillEllipse (px - ds * 2.2f, py - ds * 2.2f, ds * 4.4f, ds * 4.4f);
        g.setColour (pc.brighter (0.3f)); g.fillEllipse (px - ds, py - ds, ds * 2, ds * 2);
        const juce::String t = text ? text() : param->getCurrentValueAsText();
        if (big)
        {
            glowText (g, t, { cx - rd, cy - 22, rd * 2, 34 }, K::ink, num (30.0f), juce::Justification::centred);
            g.setFont (f (11.0f, 2)); g.setColour (K::sub); g.drawText (label, juce::Rectangle<float> (cx - rd, cy + 14, rd * 2, 16), juce::Justification::centred);
            if (sub.isNotEmpty()) { g.setFont (f (9.0f)); g.setColour (K::faint); g.drawText (sub, juce::Rectangle<float> (0, H - 18, W, 14), juce::Justification::centred); }
        }
        else
        {
            g.setFont (f (9.5f, 2)); g.setColour (hov ? K::ink : K::sub); g.drawText (label, juce::Rectangle<float> (-10, S - 2, W + 20, 14), juce::Justification::centred);
            g.setFont (num (11.0f)); g.setColour (hov ? pc.brighter (0.4f) : K::ink.withAlpha (0.85f)); g.drawText (t, juce::Rectangle<float> (-10, S + 12, W + 20, 16), juce::Justification::centred);
        }
    }
    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.mods.isAltDown() || e.mods.isCommandDown()) { param->setValueNotifyingHost (param->getDefaultValue()); return; }
        start = param->getValue(); param->beginChangeGesture(); drag = true;
    }
    void mouseDrag (const juce::MouseEvent& e) override { if (drag) { param->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, start - e.getDistanceFromDragStartY() / (e.mods.isShiftDown() ? 900.0f : (big ? 260.0f : 200.0f)))); repaint(); } }
    void mouseUp (const juce::MouseEvent&) override { if (drag) param->endChangeGesture(); drag = false; repaint(); }
    void mouseDoubleClick (const juce::MouseEvent&) override { param->setValueNotifyingHost (param->getDefaultValue()); repaint(); }
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w) override { param->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, param->getValue() + (w.deltaY > 0 ? 0.01f : -0.01f))); repaint(); }
    juce::RangedAudioParameter* param; juce::String label, sub; juce::Colour c1, c2; std::function<juce::String()> text;
    bool big = false, bipolar = false; float activity = 0, shown = -1;
private:
    float start = 0; bool drag = false;
};

// interruttore a pillola
class Toggle : public juce::Component
{
public:
    Toggle (juce::RangedAudioParameter* p, juce::String l, juce::Colour c) : param (p), label (std::move (l)), col (c) { setRepaintsOnMouseActivity (true); }
    void paint (juce::Graphics& g) override
    {
        const bool on = param->getValue() > 0.5f; auto b = getLocalBounds().toFloat();
        auto sw = b.removeFromLeft (38).withSizeKeepingCentre (38, 20);
        g.setColour (on ? col.withAlpha (0.35f) : juce::Colours::white.withAlpha (0.07f)); g.fillRoundedRectangle (sw, 10);
        g.setColour (on ? col : juce::Colours::white.withAlpha (0.15f)); g.drawRoundedRectangle (sw.reduced (0.5f), 10, 1.0f);
        const float x = on ? sw.getRight() - 17 : sw.getX() + 3;
        if (on) { g.setColour (col.withAlpha (0.3f)); g.fillEllipse (x - 4, sw.getY() - 1, 22, 22); }
        g.setColour (on ? juce::Colours::white : K::sub); g.fillEllipse (x, sw.getY() + 3, 14, 14);
        g.setFont (f (10.0f, 2)); g.setColour (on ? K::ink : K::sub); g.drawText (label, b.withTrimmedLeft (8), juce::Justification::centredLeft);
    }
    void mouseDown (const juce::MouseEvent&) override { param->setValueNotifyingHost (param->getValue() > 0.5f ? 0.0f : 1.0f); repaint(); }
    juce::RangedAudioParameter* param; juce::String label; juce::Colour col;
};
}
