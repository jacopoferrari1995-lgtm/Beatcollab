#pragma once
// Stile NEO della modalita FACILE: niente scatole. Una scena unica e profonda, luce vera (bloom sfocato), manopole 3D renderizzate,
// tipografia moderna (Manrope + Sora), movimenti morbidi.
#include "JackpotLook.h"
#include "NeoData.h"

namespace neo
{
namespace K
{
    const juce::Colour bg0 { 0xff04050a }, bg1 { 0xff0a0b16 }, ink { 0xfff2f3f9 }, sub { 0xff9096ad }, faint { 0xff545a70 }, line { 0x14ffffff };
    const juce::Colour pink { 0xffff3d9a }, violet { 0xff8b5cff }, cyan { 0xff3ee8ff }, mint { 0xff4dffb4 }, amber { 0xffffb547 }, red { 0xffff4d6a }, blue { 0xff4d7dff };
}

// ------------------------------------------------------------------------------------------------ font
struct Fonts
{
    juce::Typeface::Ptr t[4];
    Fonts()
    {
        t[0] = juce::Typeface::createSystemTypefaceFor (NeoData::Manrope500_ttf, NeoData::Manrope500_ttfSize);
        t[1] = juce::Typeface::createSystemTypefaceFor (NeoData::Manrope700_ttf, NeoData::Manrope700_ttfSize);
        t[2] = juce::Typeface::createSystemTypefaceFor (NeoData::Sora300_ttf, NeoData::Sora300_ttfSize);
        t[3] = juce::Typeface::createSystemTypefaceFor (NeoData::Sora600_ttf, NeoData::Sora600_ttfSize);
    }
};
inline juce::Typeface::Ptr tf (int i) { return juce::SharedResourcePointer<Fonts>()->t[i]; }
// testo: w 0 normale, 1/2 grassetto. Le maiuscole piccole hanno un po' di spaziatura.
inline juce::Font f (float s, int w = 0) { return juce::Font (juce::FontOptions (tf (w > 0 ? 1 : 0)).withHeight (s * 1.18f)).withExtraKerningFactor (w > 0 ? 0.08f : 0.02f); }
inline juce::Font cap (float s) { return juce::Font (juce::FontOptions (tf (1)).withHeight (s * 1.18f)).withExtraKerningFactor (0.16f); }
inline juce::Font big (float s) { return juce::Font (juce::FontOptions (tf (2)).withHeight (s * 1.25f)).withExtraKerningFactor (-0.01f); }
inline juce::Font num (float s) { return juce::Font (juce::FontOptions (tf (3)).withHeight (s * 1.2f)); }
inline juce::String fmt (float v, int d = 1, bool sign = true) { return (sign && v > 0 ? "+" : "") + juce::String (v, d); }

// ------------------------------------------------------------------------------------------------ luce: sfocatura e bloom
inline void blur (juce::Image& img, int r, int passes = 3)
{
    if (r < 1 || img.isNull()) return;
    juce::Image::BitmapData d (img, juce::Image::BitmapData::readWrite);
    const int w = d.width, h = d.height, ps = d.pixelStride;
    std::vector<int> line ((size_t) juce::jmax (w, h) * 4);
    auto run = [&] (juce::uint8* base, int n, int step)
    {
        for (int i = 0; i < n; ++i) for (int c = 0; c < 4; ++c) line[(size_t) (i * 4 + c)] = base[i * step + c];
        const int win = 2 * r + 1;
        for (int c = 0; c < 4; ++c)
        {
            int s = 0;
            for (int k = -r; k <= r; ++k) s += line[(size_t) (juce::jlimit (0, n - 1, k) * 4 + c)];
            for (int i = 0; i < n; ++i)
            {
                base[i * step + c] = (juce::uint8) (s / win);
                s += line[(size_t) (juce::jmin (n - 1, i + r + 1) * 4 + c)] - line[(size_t) (juce::jmax (0, i - r) * 4 + c)];
            }
        }
    };
    for (int p = 0; p < passes; ++p)
    {
        for (int y = 0; y < h; ++y) run (d.getLinePointer (y), w, ps);
        for (int x = 0; x < w; ++x) run (d.getPixelPointer (x, 0), h, d.lineStride);
    }
}
// disegna in piccolo, sfoca, e poi stende sopra: e' l'alone vero della luce
struct Bloom
{
    juce::Image img; float ds = 4;
    template <typename F> void make (juce::Rectangle<float> area, float down, int radius, F&& draw)
    {
        ds = down; const int w = juce::jmax (1, (int) std::ceil (area.getWidth() / ds)), h = juce::jmax (1, (int) std::ceil (area.getHeight() / ds));
        if (img.getWidth() != w || img.getHeight() != h) img = juce::Image (juce::Image::ARGB, w, h, true); else img.clear (img.getBounds());
        { juce::Graphics g (img); g.addTransform (juce::AffineTransform::translation (-area.getX(), -area.getY()).scaled (1.0f / ds)); draw (g); }
        blur (img, radius);
        at = area;
    }
    void draw (juce::Graphics& g, float alpha = 1.0f) const
    {
        if (img.isNull()) return;
        g.setOpacity (alpha); g.drawImage (img, at, juce::RectanglePlacement::stretchToFit); g.setOpacity (1.0f);
    }
    juce::Rectangle<float> at;
};

inline void glowText (juce::Graphics& g, const juce::String& t, juce::Rectangle<float> r, juce::Colour c, juce::Font fo, juce::Justification j = juce::Justification::centredLeft, float glow = 1.0f)
{
    g.setFont (fo);
    g.setColour (c.withAlpha (0.10f * glow)); for (float d : { -2.0f, 2.0f }) { g.drawText (t, r.translated (d, 0), j); g.drawText (t, r.translated (0, d), j); }
    g.setColour (c.withAlpha (0.16f * glow)); for (float d : { -1.0f, 1.0f }) { g.drawText (t, r.translated (d, 0), j); g.drawText (t, r.translated (0, d), j); }
    g.setColour (c); g.drawText (t, r, j);
}
inline void glowStroke (juce::Graphics& g, const juce::Path& p, juce::Colour c, float w, float glow = 1.0f)
{
    g.setColour (c.withAlpha (0.10f * glow)); g.strokePath (p, juce::PathStrokeType (w * 4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (c); g.strokePath (p, juce::PathStrokeType (w, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

// controllo a segmenti con cursore che scorre (anim: posizione animata del cursore, aggiornata da chi lo usa)
struct Segments
{
    juce::Rectangle<float> r[8]; int n = 0; float ax = -1, aw = 0;
    void layout (juce::Rectangle<float> area, const juce::StringArray& labels, float fs)
    {
        n = labels.size(); float tot = 0, w[8];
        for (int i = 0; i < n; ++i) { w[i] = juce::GlyphArrangement::getStringWidth (cap (fs), labels[i]) + 28; tot += w[i]; }
        float x = area.getX();
        for (int i = 0; i < n; ++i) { r[i] = { x, area.getY(), w[i], area.getHeight() }; x += w[i]; }
        juce::ignoreUnused (tot);
    }
    float width() const { return n ? r[n - 1].getRight() - r[0].getX() : 0; }
    bool step (int sel) { if (sel < 0 || sel >= n) return false; const float tx = r[sel].getX(), tw = r[sel].getWidth(); if (ax < 0) { ax = tx; aw = tw; return false; } const float nx = ax + (tx - ax) * 0.3f, nw = aw + (tw - aw) * 0.3f; const bool moving = std::abs (nx - tx) > 0.3f; ax = std::abs (nx - tx) < 0.3f ? tx : nx; aw = std::abs (nw - tw) < 0.3f ? tw : nw; return moving; }
    void paint (juce::Graphics& g, const juce::StringArray& labels, int sel, juce::Colour c, float fs, int hover = -1) const
    {
        if (! n) return;
        auto all = juce::Rectangle<float>::leftTopRightBottom (r[0].getX(), r[0].getY(), r[n - 1].getRight(), r[0].getBottom());
        const float rad = all.getHeight() * 0.5f;
        g.setColour (juce::Colours::black.withAlpha (0.35f)); g.fillRoundedRectangle (all.expanded (3), rad + 3);
        g.setColour (juce::Colours::white.withAlpha (0.05f)); g.drawRoundedRectangle (all.expanded (2.5f), rad + 2.5f, 1.0f);
        if (ax >= 0)
        {
            auto th = juce::Rectangle<float> (ax, all.getY(), aw, all.getHeight());
            g.setColour (c.withAlpha (0.25f)); g.fillRoundedRectangle (th.expanded (4, 3), rad + 3);
            juce::ColourGradient cg (c.brighter (0.25f), th.getX(), th.getY(), c.darker (0.35f), th.getX(), th.getBottom(), false); g.setGradientFill (cg); g.fillRoundedRectangle (th, rad);
            g.setColour (juce::Colours::white.withAlpha (0.35f)); g.drawRoundedRectangle (th.reduced (0.5f), rad, 0.8f);
        }
        g.setFont (cap (fs));
        for (int i = 0; i < n; ++i) { g.setColour (i == sel ? juce::Colours::white : (i == hover ? K::ink : K::sub)); g.drawText (labels[i], r[i], juce::Justification::centred); }
    }
    int hit (juce::Point<float> p) const { for (int i = 0; i < n; ++i) if (r[i].contains (p)) return i; return -1; }
};

// pillola di testo (bottone leggero)
inline void chip (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& t, bool on, juce::Colour c, float fs = 9.5f, bool hover = false)
{
    const float rad = r.getHeight() * 0.5f;
    if (on)
    {
        g.setColour (c.withAlpha (0.16f)); g.fillRoundedRectangle (r.expanded (3), rad + 3);
        g.setColour (c.withAlpha (0.22f)); g.fillRoundedRectangle (r, rad);
        g.setColour (c.withAlpha (0.9f)); g.drawRoundedRectangle (r.reduced (0.5f), rad, 1.0f);
    }
    else
    {
        g.setColour (juce::Colours::white.withAlpha (hover ? 0.09f : 0.045f)); g.fillRoundedRectangle (r, rad);
        g.setColour (juce::Colours::white.withAlpha (hover ? 0.18f : 0.08f)); g.drawRoundedRectangle (r.reduced (0.5f), rad, 1.0f);
    }
    g.setFont (cap (fs)); g.setColour (on ? juce::Colours::white : (hover ? K::ink : K::sub)); g.drawText (t, r, juce::Justification::centred);
}

// ------------------------------------------------------------------------------------------------ manopola 3D
inline const juce::Image& knobBody() { static juce::Image im = juce::ImageCache::getFromMemory (NeoData::neo_knob_png, NeoData::neo_knob_pngSize); return im; }
// il render ha la manopola (raggio 1) in un quadro largo 2.8: la manopola occupa il 71.4% dell'immagine
constexpr float kBodyFrac = 2.0f / 2.8f;

class Knob : public juce::Component, public juce::SettableTooltipClient
{
public:
    Knob (juce::RangedAudioParameter* p, juce::String l, juce::Colour a, juce::Colour b) : param (p), label (std::move (l)), c1 (a), c2 (b) { setRepaintsOnMouseActivity (true); }
    // geometria: S = diametro dell'anello
    // pad: margine trasparente intorno, cosi' l'alone della luce non viene tagliato dai bordi
    static constexpr int pad = 18;
    float S() const { const float w = (float) getWidth() - 2 * pad, h = (float) getHeight() - 2 * pad; return big ? juce::jmin (w, h - 22) : juce::jmin (w, h - 38); }
    bool hitTest (int x, int y) override { return getLocalBounds().reduced (pad).contains (x, y); }
    void paint (juce::Graphics& g) override
    {
        g.addTransform (juce::AffineTransform::translation ((float) pad, (float) pad));
        const float W = (float) getWidth() - 2 * pad, s = S(), cx = W * 0.5f, cy = s * 0.5f;
        const float R = s * 0.5f - (big ? 20.0f : 5.0f), tw = big ? 4.0f : 2.6f, kr = R - (big ? 26.0f : 11.0f);
        const float a0 = -2.356f, a1 = 2.356f, v = shownV < 0 ? param->getValue() : shownV, av = a0 + (a1 - a0) * v;
        const bool hov = isMouseOverOrDragging();
        const float from = bipolar ? 0.0f : a0;
        g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
        // tacche (solo grande)
        if (big)
            for (int i = 0; i <= 40; ++i)
            {
                const float a = a0 + (a1 - a0) * i / 40.0f, r0 = R + 7, r1 = R + (i % 5 == 0 ? 13.0f : 10.0f);
                g.setColour (juce::Colours::white.withAlpha (i % 5 == 0 ? 0.22f : 0.1f));
                g.drawLine (cx + std::sin (a) * r0, cy - std::cos (a) * r0, cx + std::sin (a) * r1, cy - std::cos (a) * r1, 1.0f);
            }
        // binario
        { juce::Path t; t.addCentredArc (cx, cy, R, R, 0, a0, a1, true); g.setColour (juce::Colours::white.withAlpha (0.07f)); g.strokePath (t, juce::PathStrokeType (tw, juce::PathStrokeType::curved, juce::PathStrokeType::rounded)); }
        // arco di valore con alone vero (in cache finche' il valore non cambia)
        juce::Path arc; const bool hasArc = std::abs (av - from) > 0.01f;
        if (hasArc) arc.addCentredArc (cx, cy, R, R, 0, juce::jmin (from, av), juce::jmax (from, av), true);
        juce::ColourGradient cg (c1, cx - R, cy + R, c2, cx + R, cy - R, false);
        if (std::abs (bloomV - v) > 0.002f || bloomS != s)
        {
            bloomV = v; bloomS = s;
            bloom.make (getLocalBounds().toFloat().translated ((float) -pad, (float) -pad), big ? 3.0f : 2.0f, big ? 5 : 3, [&] (juce::Graphics& bg)
            {
                if (hasArc) { bg.setGradientFill (cg); bg.strokePath (arc, juce::PathStrokeType (tw * 3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded)); }
                const auto pc = c1.interpolatedWith (c2, v); const float pr = kr * 0.62f;
                bg.setColour (pc); bg.fillEllipse (cx + std::sin (av) * pr - 5, cy - std::cos (av) * pr - 5, 10, 10);
            });
        }
        bloom.draw (g, hov ? 1.0f : 0.85f);
        if (hasArc) { g.setGradientFill (cg); g.strokePath (arc, juce::PathStrokeType (tw, juce::PathStrokeType::curved, juce::PathStrokeType::rounded)); }
        // corona di attivita (LOUD: quanto lavora il limiter)
        if (activity > 0.01f)
        {
            const float ro = R + 17; const int nS = 40;
            for (int i = 0; i < nS; ++i)
            {
                const float t = (i + 0.5f) / nS; if (t > activity) break;
                const float a = a0 + (a1 - a0) * t; juce::Path sg; sg.addCentredArc (cx, cy, ro, ro, 0, a - 0.025f, a + 0.025f, true);
                g.setColour ((t > 0.75f ? K::red : K::pink).withAlpha (0.9f)); g.strokePath (sg, juce::PathStrokeType (4.0f));
            }
        }
        // corpo 3D
        const float is = kr * 2.0f / kBodyFrac;
        g.drawImage (knobBody(), { cx - is * 0.5f, cy - is * 0.5f, is, is }, juce::RectanglePlacement::stretchToFit);
        if (hov) { g.setColour (juce::Colours::white.withAlpha (0.04f)); g.fillEllipse (cx - kr, cy - kr, kr * 2, kr * 2); }
        // indicatore: tacca luminosa sul cappello
        {
            const auto pc = c1.interpolatedWith (c2, v).brighter (0.3f);
            const float r0 = kr * (big ? 0.66f : 0.42f), r1 = kr * 0.8f, sn = std::sin (av), cs = std::cos (av);
            juce::Path pl; pl.startNewSubPath (cx + sn * r0, cy - cs * r0); pl.lineTo (cx + sn * r1, cy - cs * r1);
            g.setColour (pc.withAlpha (0.35f)); g.strokePath (pl, juce::PathStrokeType (big ? 7.0f : 5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setColour (juce::Colours::white); g.strokePath (pl, juce::PathStrokeType (big ? 2.4f : 1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        const juce::String t = text ? text() : param->getCurrentValueAsText();
        if (big)
        {
            g.setFont (big_ (40.0f)); g.setColour (K::ink); g.drawText (t, juce::Rectangle<float> (cx - kr, cy - 30, kr * 2, 46), juce::Justification::centred);
            g.setFont (cap (10.0f)); g.setColour (K::sub); g.drawText (label, juce::Rectangle<float> (cx - kr, cy + 16, kr * 2, 16), juce::Justification::centred);
            if (sub.isNotEmpty()) { g.setFont (f (9.5f)); g.setColour (K::faint); g.drawText (sub, juce::Rectangle<float> (0, (float) getHeight() - 2 * pad - 16, W, 14), juce::Justification::centred); }
        }
        else
        {
            g.setFont (cap (9.0f)); g.setColour (hov ? K::ink : K::sub); g.drawText (label, juce::Rectangle<float> (-20, s + 3, W + 40, 14), juce::Justification::centred);
            g.setFont (num (11.0f)); g.setColour (hov ? c1.interpolatedWith (c2, v).brighter (0.5f) : K::ink.withAlpha (0.9f)); g.drawText (t, juce::Rectangle<float> (-20, s + 18, W + 40, 16), juce::Justification::centred);
        }
    }
    static juce::Font big_ (float s) { return neo::big (s); }
    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.mods.isAltDown() || e.mods.isCommandDown()) { param->setValueNotifyingHost (param->getDefaultValue()); return; }
        start = param->getValue(); param->beginChangeGesture(); drag = true;
    }
    void mouseDrag (const juce::MouseEvent& e) override { if (drag) { param->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, start - e.getDistanceFromDragStartY() / (e.mods.isShiftDown() ? 900.0f : (big ? 260.0f : 200.0f)))); shownV = param->getValue(); repaint(); } }
    void mouseUp (const juce::MouseEvent&) override { if (drag) { param->endChangeGesture(); if (onRelease) onRelease(); } drag = false; repaint(); }
    void mouseDoubleClick (const juce::MouseEvent&) override { param->setValueNotifyingHost (defV >= 0 ? defV : param->getDefaultValue()); }
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w) override { param->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, param->getValue() + (w.deltaY > 0 ? 0.01f : -0.01f))); }
    // movimento morbido verso il valore vero (cambi di preset, doppio clic): ritorna true se va ridisegnata
    bool animate()
    {
        const float target = param->getValue(); bool ch = false;
        if (shownV < 0 || drag) { if (shownV != target) ch = true; shownV = target; }
        else if (std::abs (shownV - target) > 0.0005f) { shownV += (target - shownV) * 0.28f; ch = true; }
        else if (shownV != target) { shownV = target; ch = true; }
        if (std::abs (activity - shownAct) > 0.01f) { shownAct = activity; ch = true; }
        return ch;
    }
    juce::RangedAudioParameter* param; juce::String label, sub; juce::Colour c1, c2; std::function<juce::String()> text;
    std::function<void()> onRelease;   // per gli effetti che tornano a zero quando molli
    bool big = false, bipolar = false; float activity = 0;
    float defV = -1;   // valore del doppio clic (se diverso da quello del parametro)
private:
    float start = 0, shownV = -1, shownAct = 0, bloomV = -1, bloomS = 0; bool drag = false; Bloom bloom;
};

// interruttore
class Toggle : public juce::Component
{
public:
    Toggle (juce::RangedAudioParameter* p, juce::String l, juce::Colour c) : param (p), label (std::move (l)), col (c) { setRepaintsOnMouseActivity (true); }
    void paint (juce::Graphics& g) override
    {
        const bool on = param->getValue() > 0.5f; auto b = getLocalBounds().toFloat();
        auto sw = b.removeFromLeft (36).withSizeKeepingCentre (36, 18);
        g.setColour (on ? col.withAlpha (0.3f) : juce::Colours::black.withAlpha (0.4f)); g.fillRoundedRectangle (sw, 9);
        g.setColour (on ? col.withAlpha (0.8f) : juce::Colours::white.withAlpha (0.12f)); g.drawRoundedRectangle (sw.reduced (0.5f), 9, 1.0f);
        const float x = on ? sw.getRight() - 16 : sw.getX() + 2;
        if (on) { g.setColour (col.withAlpha (0.25f)); g.fillEllipse (x - 5, sw.getY() - 3, 24, 24); }
        juce::ColourGradient kg (juce::Colours::white, x, sw.getY() + 2, juce::Colour (0xffb9bccb), x, sw.getBottom() - 2, false); g.setGradientFill (kg); g.fillEllipse (x, sw.getY() + 2, 14, 14);
        g.setFont (cap (9.0f)); g.setColour (on ? K::ink : K::sub); g.drawText (label, b.withTrimmedLeft (10), juce::Justification::centredLeft);
    }
    void mouseDown (const juce::MouseEvent&) override { param->setValueNotifyingHost (param->getValue() > 0.5f ? 0.0f : 1.0f); repaint(); }
    juce::RangedAudioParameter* param; juce::String label; juce::Colour col;
};
}
