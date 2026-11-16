#pragma once
// Grafica cyberpunk comune (Vocal Radar, 808 Studio): palette, pannelli a spigoli tagliati, neon, manopole ad anello.
#include <juce_audio_processors/juce_audio_processors.h>
#include "Neo.h"
namespace cyber
{
namespace cp   // palette cyberpunk
{
const juce::Colour bg0 (0xff05040c), bg1 (0xff140626), mag (0xffff2e88), cya (0xff21f3ff), yel (0xfff4ff5e), pur (0xff8a3bff), red (0xffff3355), ink (0xffeef0ff), dim (0xff7d7aa6), panel (0xe60a0816);
}

// font
inline juce::Font fTitle (float s) { return juce::Font (juce::FontOptions (neo::tf (4)).withHeight (s)); }
inline juce::Font fLab (float s) { return juce::Font (juce::FontOptions (neo::tf (9)).withHeight (s)).withExtraKerningFactor (0.10f); }
inline juce::Font fMono (float s) { return juce::Font (juce::FontOptions (neo::tf (7)).withHeight (s)); }
inline juce::Font fWide (float s) { return juce::Font (juce::FontOptions (neo::tf (8)).withHeight (s)).withExtraKerningFactor (0.12f); }

using namespace cp;
class CyberKnob : public juce::Slider
{
public:
    CyberKnob (juce::RangedAudioParameter* p, juce::String label, juce::Colour c, bool big = false);
    void paint (juce::Graphics&) override;
    std::function<juce::String (float)> fmt;
    juce::String label; juce::Colour col; bool big; bool dimmed = false;
    std::unique_ptr<juce::SliderParameterAttachment> att;
};

// pannello a spigoli tagliati con bordo al neon
inline juce::Path cut (juce::Rectangle<float> r, float c)
{
    juce::Path p; p.startNewSubPath (r.getX() + c, r.getY()); p.lineTo (r.getRight(), r.getY()); p.lineTo (r.getRight(), r.getBottom() - c);
    p.lineTo (r.getRight() - c, r.getBottom()); p.lineTo (r.getX(), r.getBottom()); p.lineTo (r.getX(), r.getY() + c); p.closeSubPath(); return p;
}
inline void neonStroke (juce::Graphics& g, const juce::Path& p, juce::Colour c, float w = 1.5f)
{
    for (int k = 3; k >= 1; --k) { g.setColour (c.withAlpha (0.08f * (4 - k))); g.strokePath (p, juce::PathStrokeType (w + k * 3.0f)); }
    g.setColour (c); g.strokePath (p, juce::PathStrokeType (w));
}
inline void cyberPanel (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c, const juce::String& tag)
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
inline void chipBtn (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& t, bool on, juce::Colour c, float fs = 12)
{
    const auto p = cut (r, 7);
    g.setColour (on ? c.withAlpha (0.28f) : juce::Colour (0xcc0b0918)); g.fillPath (p);
    if (on) neonStroke (g, p, c, 1.2f); else { g.setColour (c.withAlpha (0.45f)); g.strokePath (p, juce::PathStrokeType (1.0f)); }
    g.setFont (fLab (fs)); g.setColour (on ? ink : c.withAlpha (0.8f)); g.drawText (t, r, juce::Justification::centred);
}

// ======================================================================= manopola
inline CyberKnob::CyberKnob (juce::RangedAudioParameter* p, juce::String l, juce::Colour c, bool b) : label (std::move (l)), col (c), big (b)
{
    setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag); setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    setMouseDragSensitivity (220); setVelocityBasedMode (false);
    att = std::make_unique<juce::SliderParameterAttachment> (*p, *this);
    setDoubleClickReturnValue (true, p->convertFrom0to1 (p->getDefaultValue()));
}
inline void CyberKnob::paint (juce::Graphics& g)
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

}
