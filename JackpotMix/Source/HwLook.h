#pragma once
// Jackpot Mixology: hardware vero. Pannelli 500-series (vernice, alluminio spazzolato, anodizzato nero, smalto crema,
// pedali), manopole renderizzate in 3D con Blender (una striscia di 64 fotogrammi per stile), serigrafie e scale.
#include "AnalogLook.h"

namespace hw
{
using alook::A::ink;
enum KnobStyle { KS_BIGBLACK, KS_CHICKEN, KS_DAVIES, KS_FET, KS_IVORY, KS_NEVE_BLUE, KS_NEVE_GREY, KS_NEVE_RED, KS_PEDAL, KS_SILVER,
                 KS_SSL_BLUE, KS_SSL_GREEN, KS_SSL_RED, KS_SSL_WHITE, KS_SSL_YELLOW, kKnobStyles };
inline juce::Image knobArt (int s)
{
    auto G = [] (const char* d, int n) { return juce::ImageCache::getFromMemory (d, n); };
    switch (s)
    {
        case KS_BIGBLACK:   return G (PreArt::k2_bigblack_png, PreArt::k2_bigblack_pngSize);
        case KS_CHICKEN:    return G (PreArt::k2_chicken_png, PreArt::k2_chicken_pngSize);
        case KS_DAVIES:     return G (PreArt::k2_davies_png, PreArt::k2_davies_pngSize);
        case KS_FET:        return G (PreArt::k2_fet_png, PreArt::k2_fet_pngSize);
        case KS_IVORY:      return G (PreArt::k2_ivory_png, PreArt::k2_ivory_pngSize);
        case KS_NEVE_BLUE:  return G (PreArt::k2_neve_blue_png, PreArt::k2_neve_blue_pngSize);
        case KS_NEVE_GREY:  return G (PreArt::k2_neve_grey_png, PreArt::k2_neve_grey_pngSize);
        case KS_NEVE_RED:   return G (PreArt::k2_neve_red_png, PreArt::k2_neve_red_pngSize);
        case KS_PEDAL:      return G (PreArt::k2_pedal_png, PreArt::k2_pedal_pngSize);
        case KS_SILVER:     return G (PreArt::k2_silver_png, PreArt::k2_silver_pngSize);
        case KS_SSL_BLUE:   return G (PreArt::k2_ssl_blue_png, PreArt::k2_ssl_blue_pngSize);
        case KS_SSL_GREEN:  return G (PreArt::k2_ssl_green_png, PreArt::k2_ssl_green_pngSize);
        case KS_SSL_RED:    return G (PreArt::k2_ssl_red_png, PreArt::k2_ssl_red_pngSize);
        case KS_SSL_WHITE:  return G (PreArt::k2_ssl_white_png, PreArt::k2_ssl_white_pngSize);
        default:            return G (PreArt::k2_ssl_yellow_png, PreArt::k2_ssl_yellow_pngSize);
    }
}
inline void frame (juce::Graphics& g, const juce::Image& strip, int idx, juce::Rectangle<float> dst)
{
    const int w = strip.getWidth(), n = juce::jmax (1, strip.getHeight() / juce::jmax (1, w)); idx = juce::jlimit (0, n - 1, idx);
    g.setImageResamplingQuality (juce::Graphics::highResamplingQuality); g.setOpacity (1.0f);
    g.drawImage (strip, dst.getX(), dst.getY(), dst.getWidth(), dst.getHeight(), 0, idx * w, w, w);
}

// ----------------------------------------------------------------------- pannelli
enum Face { FACE_PAINT, FACE_BRUSHED, FACE_BLACK, FACE_CREAM, FACE_PEDAL, FACE_WOOD };
inline void brushed (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour base, int seed, float amt)
{
    juce::Random rnd (seed);
    for (float y = r.getY(); y < r.getBottom(); y += 1.0f)
    {
        const float v = (rnd.nextFloat() - 0.5f) * amt;
        g.setColour (v > 0 ? juce::Colours::white.withAlpha (v) : juce::Colours::black.withAlpha (-v));
        g.fillRect (r.getX(), y, r.getWidth(), 1.0f);
    }
    juce::ignoreUnused (base);
}
inline void speckle (juce::Graphics& g, juce::Rectangle<float> r, int seed, float amt)
{
    juce::Random rnd (seed); const int n = (int) (r.getWidth() * r.getHeight() / 30);
    for (int i = 0; i < n; ++i)
    {
        const float x = r.getX() + rnd.nextFloat() * r.getWidth(), y = r.getY() + rnd.nextFloat() * r.getHeight();
        g.setColour (rnd.nextBool() ? juce::Colours::white.withAlpha (amt * rnd.nextFloat()) : juce::Colours::black.withAlpha (amt * rnd.nextFloat()));
        g.fillRect (x, y, 1.0f, 1.0f);
    }
}
inline void screw (juce::Graphics& g, float x, float y, float r, float rot)
{
    g.setColour (juce::Colours::black.withAlpha (0.45f)); g.fillEllipse (x - r - 1, y - r - 1, r * 2 + 2, r * 2 + 2);
    juce::ColourGradient sg (juce::Colour (0xfff0eee8), x - r * 0.6f, y - r * 0.7f, juce::Colour (0xff6b6862), x + r * 0.7f, y + r * 0.8f, false);
    g.setGradientFill (sg); g.fillEllipse (x - r, y - r, r * 2, r * 2);
    g.setColour (juce::Colour (0xff3d3a35));
    g.drawLine (x - std::cos (rot) * r * 0.7f, y - std::sin (rot) * r * 0.7f, x + std::cos (rot) * r * 0.7f, y + std::sin (rot) * r * 0.7f, 1.4f);
    g.drawLine (x - std::cos (rot + 1.5708f) * r * 0.7f, y - std::sin (rot + 1.5708f) * r * 0.7f, x + std::cos (rot + 1.5708f) * r * 0.7f, y + std::sin (rot + 1.5708f) * r * 0.7f, 1.4f);
}
// frontale di un modulo: ombra nel telaio, superficie, smusso, viti in alto e in basso (500-series)
inline void faceplate (juce::Graphics& g, juce::Rectangle<float> r, int face, juce::Colour c, int seed)
{
    g.setColour (juce::Colours::black.withAlpha (0.6f)); g.fillRoundedRectangle (r.translated (0, 3).expanded (1.5f), 4);
    const float rad = face == FACE_PEDAL ? 12.0f : 3.0f;
    juce::Path shape; shape.addRoundedRectangle (r, rad);
    g.saveState(); g.reduceClipRegion (shape);
    switch (face)
    {
        case FACE_BRUSHED:
        {
            juce::ColourGradient gr (c.brighter (0.12f), r.getX(), r.getY(), c.darker (0.12f), r.getRight(), r.getBottom(), false); g.setGradientFill (gr); g.fillRect (r);
            brushed (g, r, c, seed, 0.09f); break;
        }
        case FACE_BLACK:
        {
            juce::ColourGradient gr (c.brighter (0.08f), r.getX(), r.getY(), c.darker (0.3f), r.getX(), r.getBottom(), false); g.setGradientFill (gr); g.fillRect (r);
            brushed (g, r, c, seed, 0.035f); break;
        }
        case FACE_CREAM:
        {
            juce::ColourGradient gr (c.brighter (0.05f), r.getX(), r.getY(), c.darker (0.12f), r.getRight(), r.getBottom(), false); g.setGradientFill (gr); g.fillRect (r);
            speckle (g, r, seed, 0.06f);
            juce::Random rnd (seed);
            for (int i = 0; i < 10; ++i) { const float x = r.getX() + rnd.nextFloat() * r.getWidth(), y = r.getY() + rnd.nextFloat() * r.getHeight(), s = 20 + rnd.nextFloat() * 60;
                juce::ColourGradient m (juce::Colour (0xff7a5a2a).withAlpha (0.035f), x, y, juce::Colours::transparentBlack, x + s, y, true); g.setGradientFill (m); g.fillEllipse (x - s, y - s, s * 2, s * 2); }
            break;
        }
        case FACE_PEDAL:
        {
            juce::ColourGradient gr (c.brighter (0.25f), r.getX(), r.getY(), c.darker (0.35f), r.getRight(), r.getBottom(), false); g.setGradientFill (gr); g.fillRect (r);
            speckle (g, r, seed, 0.05f);
            juce::ColourGradient sh (juce::Colours::white.withAlpha (0.22f), r.getX(), r.getY(), juce::Colours::transparentWhite, r.getX() + r.getWidth() * 0.5f, r.getY() + r.getHeight() * 0.35f, false);
            g.setGradientFill (sh); g.fillRect (r);
            break;
        }
        case FACE_WOOD:
        {
            g.setColour (c); g.fillRect (r);
            juce::Random rnd (seed);
            for (float x = r.getX(); x < r.getRight(); x += 1.0f) { const float v = 0.5f + 0.5f * std::sin (x * 0.21f + std::sin (x * 0.05f) * 3.0f); g.setColour (juce::Colours::black.withAlpha (0.05f + 0.1f * v + rnd.nextFloat() * 0.03f)); g.fillRect (x, r.getY(), 1.0f, r.getHeight()); }
            break;
        }
        default:
        {   // vernice a polvere martellata
            juce::ColourGradient gr (c.brighter (0.08f), r.getX(), r.getY(), c.darker (0.15f), r.getRight(), r.getBottom(), false); g.setGradientFill (gr); g.fillRect (r);
            speckle (g, r, seed, 0.09f);
        }
    }
    g.restoreState();
    // smusso: luce sul bordo alto e sinistro, ombra sotto
    g.setColour (juce::Colours::white.withAlpha (face == FACE_BLACK ? 0.12f : 0.3f)); g.drawLine (r.getX() + rad, r.getY() + 0.8f, r.getRight() - rad, r.getY() + 0.8f, 1.2f);
    g.drawLine (r.getX() + 0.8f, r.getY() + rad, r.getX() + 0.8f, r.getBottom() - rad, 1.0f);
    g.setColour (juce::Colours::black.withAlpha (0.5f)); g.drawRoundedRectangle (r, rad, 1.0f);
    if (face != FACE_PEDAL)
    {
        screw (g, r.getCentreX(), r.getY() + 9, 4.6f, 0.3f + (seed % 7) * 0.4f);
        screw (g, r.getCentreX(), r.getBottom() - 9, 4.6f, 1.1f + (seed % 5) * 0.5f);
    }
}
// serigrafia
inline void silk (juce::Graphics& g, const juce::String& t, juce::Rectangle<float> r, juce::Font f, juce::Colour c, juce::Justification j = juce::Justification::centred)
{
    g.setFont (f);
    const bool light = c.getPerceivedBrightness() > 0.55f;
    g.setColour (light ? juce::Colours::black.withAlpha (0.5f) : juce::Colours::white.withAlpha (0.3f)); g.drawText (t, r.translated (0, light ? 1.0f : 0.7f), j);
    g.setColour (c); g.drawText (t, r, j);
}
// spia (LED a cupola)
inline void led (juce::Graphics& g, float cx, float cy, float r, juce::Colour c, bool on)
{
    if (on) { juce::ColourGradient hg (c.withAlpha (0.45f), cx, cy, c.withAlpha (0.0f), cx + r * 4.5f, cy, true); g.setGradientFill (hg); g.fillEllipse (cx - r * 4.5f, cy - r * 4.5f, r * 9, r * 9); }
    g.setColour (juce::Colour (0xff121110)); g.fillEllipse (cx - r - 1.5f, cy - r - 1.5f, r * 2 + 3, r * 2 + 3);
    juce::ColourGradient lg (on ? c.brighter (0.9f) : c.darker (0.85f), cx - r * 0.3f, cy - r * 0.4f, on ? c : c.darker (1.4f), cx + r, cy + r, true);
    g.setGradientFill (lg); g.fillEllipse (cx - r, cy - r, r * 2, r * 2);
    g.setColour (juce::Colours::white.withAlpha (on ? 0.7f : 0.25f)); g.fillEllipse (cx - r * 0.5f, cy - r * 0.6f, r * 0.55f, r * 0.4f);
}

// ----------------------------------------------------------------------- manopola
class Knob : public juce::Component, public juce::SettableTooltipClient
{
public:
    Knob (juce::RangedAudioParameter* p, juce::String l, int st) : param (p), label (std::move (l)), style (st) { setRepaintsOnMouseActivity (true); }
    void paint (juce::Graphics& g) override
    {
        const float lh = labelH, vh = valueH;
        const float S = juce::jmin ((float) getWidth() - 8.0f, getHeight() - lh - vh);
        const float cx = getWidth() * 0.5f, cy = lh + S * 0.5f, R = S * 0.5f;
        const float a0 = -2.356f, a1 = 2.356f, v = param->getValue();
        silk (g, label, { -6, 0, getWidth() + 12.0f, lh - 1 }, alook::serif (labelSize), inkC);
        // scala serigrafata
        const int n = ticks;
        for (int i = 0; i <= n; ++i)
        {
            const float a = a0 + (a1 - a0) * i / n, major = (i == 0 || i == n || i == n / 2);
            const float ro = R, ri = R - (major ? 5.0f : 3.0f);
            g.setColour (inkC.withAlpha (major ? 0.95f : 0.65f));
            g.drawLine (cx + std::sin (a) * ri, cy - std::cos (a) * ri, cx + std::sin (a) * ro, cy - std::cos (a) * ro, major ? 1.3f : 0.9f);
        }
        if (bipolar) { g.setColour (inkC); g.fillEllipse (cx - 1.6f, cy - R - 4.5f, 3.2f, 3.2f); }
        const float rk = R - 6.0f, fs = rk * 2.0f * frameMul;
        // ombra di contatto
        for (int i = 5; i >= 1; --i) { const float e = rk * 0.04f * i; g.setColour (juce::Colours::black.withAlpha (0.07f)); g.fillEllipse (cx - rk - e + rk * 0.08f, cy - rk - e + rk * 0.14f, (rk + e) * 2, (rk + e) * 2); }
        frame (g, knobArt (style), juce::roundToInt (v * 63), { cx - fs / 2, cy - fs / 2, fs, fs });
        if (isMouseOverOrDragging())
        {   // arco di valore discreto mentre lo tocchi
            juce::Path arc; const float va = bipolar ? 0.0f : a0;
            arc.addCentredArc (cx, cy, R + 2.5f, R + 2.5f, 0, va, a0 + (a1 - a0) * v, true);
            g.setColour (hot.withAlpha (0.9f)); g.strokePath (arc, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        const juce::String t = text ? text() : param->getCurrentValueAsText();
        g.setFont (alook::typed (valueSize)); g.setColour (isMouseOverOrDragging() ? hot : inkC.withAlpha (0.78f));
        g.drawText (t, juce::Rectangle<float> (-12, (float) getHeight() - vh, getWidth() + 24.0f, vh), juce::Justification::centred);
    }
    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.mods.isAltDown() || e.mods.isCommandDown()) { param->setValueNotifyingHost (param->getDefaultValue()); return; }
        start = param->getValue(); param->beginChangeGesture(); drag = true;
    }
    void mouseUp (const juce::MouseEvent&) override { if (drag) param->endChangeGesture(); drag = false; repaint(); }
    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (! drag) return;
        float nv = start - e.getDistanceFromDragStartY() / (e.mods.isShiftDown() ? 900.0f : 200.0f);
        if (stepsN > 1) nv = std::round (nv * (stepsN - 1)) / (stepsN - 1);
        param->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, nv)); repaint();
    }
    void mouseDoubleClick (const juce::MouseEvent&) override { param->setValueNotifyingHost (param->getDefaultValue()); }
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w) override
    {
        const float st = stepsN > 1 ? 1.0f / (stepsN - 1) : 0.01f;
        param->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, param->getValue() + (w.deltaY > 0 ? st : -st))); repaint();
    }
    juce::RangedAudioParameter* param; juce::String label; int style;
    std::function<juce::String()> text;
    juce::Colour inkC { 0xff1e1b17 }, hot { 0xffd8452f };
    float labelH = 15, valueH = 14, labelSize = 9.5f, valueSize = 9.0f, frameMul = 1.0f;
    int ticks = 10, stepsN = 0; bool bipolar = false;
private:
    float start = 0; bool drag = false;
};

// deviatore a leva (IN/OUT, ON/OFF)
class Switch : public juce::Component, public juce::SettableTooltipClient
{
public:
    Switch (juce::RangedAudioParameter* p, juce::String l, juce::String up, juce::String dn, bool inv = false) : param (p), label (std::move (l)), upT (std::move (up)), dnT (std::move (dn)), invert (inv) {}
    bool isUp() const { return (param->getValue() > 0.5f) != invert; }
    void paint (juce::Graphics& g) override
    {
        const float w = (float) getWidth(), h = (float) getHeight();
        silk (g, label, { -6, 0, w + 12, 13 }, alook::serif (9.0f), inkC);
        const float s = juce::jmin (w, h - 26) * 1.9f;
        silk (g, upT, { 0, 13, w, 11 }, alook::serif (7.5f), inkC);
        alook::Art::frame (g, alook::Art::toggle (isUp()), 0, { w * 0.5f - s / 2, 13 + (h - 26) * 0.5f - s / 2 + 6, s, s });
        silk (g, dnT, { 0, h - 11, w, 11 }, alook::serif (7.5f), inkC);
    }
    void mouseDown (const juce::MouseEvent&) override { param->setValueNotifyingHost (param->getValue() > 0.5f ? 0.0f : 1.0f); if (onChange) onChange(); repaint(); }
    juce::RangedAudioParameter* param; juce::String label, upT, dnT; bool invert; juce::Colour inkC { 0xff1e1b17 }; std::function<void()> onChange;
};

// strumento a ago (VU) con cornice nera e luce calda
inline void vu (juce::Graphics& g, juce::Rectangle<float> b, float db, const juce::String& title, bool grMode)
{
    g.setColour (juce::Colour (0xff111010)); g.fillRoundedRectangle (b.expanded (3), 5);
    g.setColour (juce::Colour (0xff3a3833)); g.drawRoundedRectangle (b.expanded (3), 5, 1.0f);
    juce::ColourGradient cg (juce::Colour (0xfff8eccb), b.getCentreX(), b.getY(), juce::Colour (0xffd8c08c), b.getCentreX(), b.getBottom(), false); g.setGradientFill (cg); g.fillRoundedRectangle (b, 2);
    juce::ColourGradient lamp (juce::Colour (0x55ffb347), b.getCentreX(), b.getBottom(), juce::Colours::transparentBlack, b.getCentreX(), b.getY(), false); g.setGradientFill (lamp); g.fillRoundedRectangle (b, 2);
    const float cx = b.getCentreX(), cy = b.getBottom() + b.getHeight() * 0.55f, R = b.getHeight() * 1.28f;
    auto ang = [] (float d) { return juce::jmap (juce::jlimit (-20.0f, 3.0f, d), -20.0f, 3.0f, -0.74f, 0.74f); };
    const juce::Colour in (0xff241b12), red (0xffc0392b);
    { juce::Path arc; arc.addCentredArc (cx, cy, R * 0.82f, R * 0.82f, 0, ang (-20), ang (0), true); g.setColour (in); g.strokePath (arc, juce::PathStrokeType (1.0f)); }
    { juce::Path arc; arc.addCentredArc (cx, cy, R * 0.82f, R * 0.82f, 0, ang (0), ang (3), true); g.setColour (red); g.strokePath (arc, juce::PathStrokeType (3.0f)); }
    for (float d : { -20.f, -10.f, -7.f, -5.f, -3.f, -2.f, -1.f, 0.f, 1.f, 2.f, 3.f })
    {
        const float a = ang (d); const bool lab = d == -20 || d == -10 || d == -7 || d == -5 || d == -3 || d == 0 || d == 3;
        g.setColour (d > 0 ? red : in);
        g.drawLine (cx + std::sin (a) * R * 0.82f, cy - std::cos (a) * R * 0.82f, cx + std::sin (a) * R * (lab ? 0.92f : 0.88f), cy - std::cos (a) * R * (lab ? 0.92f : 0.88f), 1.0f);
        if (lab) { g.setFont (alook::typed (7.0f)); g.drawText (juce::String (std::abs ((int) d)), juce::Rectangle<float> (cx + std::sin (a) * R * 0.98f - 9, cy - std::cos (a) * R * 0.98f - 5, 18, 10), juce::Justification::centred); }
    }
    g.setFont (alook::serif (8.0f)); g.setColour (in); g.drawText (title, juce::Rectangle<float> (b.getX(), b.getBottom() - 17, b.getWidth(), 11), juce::Justification::centred);
    g.setFont (alook::serif (10.0f)); g.drawText ("VU", juce::Rectangle<float> (b.getX(), b.getBottom() - 30, b.getWidth(), 12), juce::Justification::centred);
    juce::ignoreUnused (grMode);
    g.saveState(); g.reduceClipRegion (b.toNearestInt());
    const float a = ang (db);
    g.setColour (juce::Colours::black.withAlpha (0.25f)); g.drawLine (cx + 2, cy + 2, cx + 2 + std::sin (a) * R * 0.95f, cy + 2 - std::cos (a) * R * 0.95f, 1.6f);
    g.setColour (juce::Colour (0xff15110c)); g.drawLine (cx, cy, cx + std::sin (a) * R * 0.95f, cy - std::cos (a) * R * 0.95f, 1.3f);
    g.restoreState();
    // vetro
    juce::ColourGradient gl (juce::Colours::white.withAlpha (0.28f), b.getX(), b.getY(), juce::Colours::transparentWhite, b.getX(), b.getY() + b.getHeight() * 0.45f, false);
    g.setGradientFill (gl); g.fillRoundedRectangle (b.withHeight (b.getHeight() * 0.45f), 2);
}
// barra di LED verticale (riduzione dall'alto verso il basso o livello dal basso)
inline void ledBar (juce::Graphics& g, juce::Rectangle<float> b, float frac, bool fromTop, juce::Colour c)
{
    g.setColour (juce::Colour (0xff0b0b0a)); g.fillRoundedRectangle (b.expanded (2), 3);
    const int n = juce::jmax (4, (int) (b.getHeight() / 6)); const float h = b.getHeight() / n;
    for (int i = 0; i < n; ++i)
    {
        const float f = (i + 0.5f) / n; const bool on = f <= frac;
        const float y = fromTop ? b.getY() + i * h : b.getBottom() - (i + 1) * h;
        juce::Colour col = c; if (! fromTop && f > 0.85f) col = juce::Colour (0xffff3b2f); else if (! fromTop && f > 0.7f) col = juce::Colour (0xffffc23d);
        g.setColour (on ? col : col.withAlpha (0.12f)); g.fillRoundedRectangle (b.getX(), y + 1, b.getWidth(), h - 2, 1.0f);
    }
}
}
