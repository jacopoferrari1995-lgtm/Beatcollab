#pragma once
// Look condiviso della serie Jackpot: font incorporati e piccoli effetti grafici.
#include <juce_gui_basics/juce_gui_basics.h>
#include "JackpotFontsData.h"

namespace jlook
{
inline juce::Typeface::Ptr face (int which)
{
    static juce::Typeface::Ptr f[5];
    if (f[which] == nullptr)
    {
        switch (which)
        {
            case 0: f[0] = juce::Typeface::createSystemTypefaceFor (JackpotFontsData::RajdhaniMedium_ttf, JackpotFontsData::RajdhaniMedium_ttfSize); break;
            case 1: f[1] = juce::Typeface::createSystemTypefaceFor (JackpotFontsData::RajdhaniSemiBold_ttf, JackpotFontsData::RajdhaniSemiBold_ttfSize); break;
            case 2: f[2] = juce::Typeface::createSystemTypefaceFor (JackpotFontsData::RajdhaniBold_ttf, JackpotFontsData::RajdhaniBold_ttfSize); break;
            case 3: f[3] = juce::Typeface::createSystemTypefaceFor (JackpotFontsData::ShareTechMonoRegular_ttf, JackpotFontsData::ShareTechMonoRegular_ttfSize); break;
            default: f[4] = juce::Typeface::createSystemTypefaceFor (JackpotFontsData::MichromaRegular_ttf, JackpotFontsData::MichromaRegular_ttfSize); break;
        }
    }
    return f[which];
}
// testo dell'interfaccia (Rajdhani): weight 0 medio, 1 semibold, 2 bold
inline juce::Font ui (float size, int weight = 1) { return juce::Font (juce::FontOptions (face (juce::jlimit (0, 2, weight))).withHeight (size * 1.24f)); }
// display LCD (Share Tech Mono)
inline juce::Font mono (float size) { return juce::Font (juce::FontOptions (face (3)).withHeight (size * 1.12f)); }
// logo (Michroma)
inline juce::Font logo (float size) { return juce::Font (juce::FontOptions (face (4)).withHeight (size)).withExtraKerningFactor (0.06f); }

// LookAndFeel per menu e finestre native
struct LnF : juce::LookAndFeel_V4
{
    LnF()
    {
        setDefaultSansSerifTypeface (face (1));
        setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xf0121419));
        setColour (juce::PopupMenu::textColourId, juce::Colour (0xffdfe3ea));
        setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xff2a2d36));
        setColour (juce::PopupMenu::highlightedTextColourId, juce::Colour (0xff5ef2ff));
        setColour (juce::PopupMenu::headerTextColourId, juce::Colour (0xffffb000));
    }
    juce::Font getPopupMenuFont() override { return ui (15.0f, 1); }
    void drawPopupMenuBackground (juce::Graphics& g, int w, int h) override
    {
        g.fillAll (findColour (juce::PopupMenu::backgroundColourId));
        g.setColour (juce::Colour (0x40ff2e88)); g.drawRect (0, 0, w, h, 1);
    }
};

// ombra morbida sotto un rettangolo arrotondato
inline void softShadow (juce::Graphics& g, juce::Rectangle<float> r, float rad, float spread = 14.0f, float alpha = 0.55f, float dy = 6.0f)
{
    juce::Path p; p.addRoundedRectangle (r.translated (0, dy), rad);
    juce::DropShadow (juce::Colours::black.withAlpha (alpha), (int) spread, {}).drawForPath (g, p);
}
// grana fine (texture cacheata) per togliere l'effetto "piatto"
inline void grain (juce::Graphics& g, juce::Rectangle<int> r, float alpha = 0.035f)
{
    static juce::Image img;
    if (img.isNull())
    {
        img = juce::Image (juce::Image::ARGB, 256, 256, true); juce::Random rr (42);
        for (int y = 0; y < 256; ++y) for (int x = 0; x < 256; ++x) { const auto v = (juce::uint8) rr.nextInt (256); img.setPixelAt (x, y, juce::Colour (v, v, v, (juce::uint8) 255)); }
    }
    g.saveState(); g.reduceClipRegion (r); g.setOpacity (alpha);
    for (int y = r.getY(); y < r.getBottom(); y += 256) for (int x = r.getX(); x < r.getRight(); x += 256) g.drawImageAt (img, x, y);
    g.restoreState();
}
// vignettatura ai bordi
inline void vignette (juce::Graphics& g, juce::Rectangle<float> r, float amt = 0.45f)
{
    juce::ColourGradient vg (juce::Colours::transparentBlack, r.getCentreX(), r.getCentreY(), juce::Colours::black.withAlpha (amt), r.getX(), r.getY(), true);
    vg.addColour (0.6, juce::Colours::transparentBlack);
    g.setGradientFill (vg); g.fillRect (r);
}
// riflesso "vetro" su un display
inline void glass (juce::Graphics& g, juce::Rectangle<float> r, float rad)
{
    juce::Path p; p.addRoundedRectangle (r, rad);
    g.saveState(); g.reduceClipRegion (p);
    juce::ColourGradient gl (juce::Colours::white.withAlpha (0.07f), r.getX(), r.getY(), juce::Colours::transparentWhite, r.getX(), r.getY() + r.getHeight() * 0.45f, false);
    g.setGradientFill (gl); g.fillRect (r.withHeight (r.getHeight() * 0.45f));
    g.restoreState();
    g.setColour (juce::Colours::white.withAlpha (0.06f)); g.drawRoundedRectangle (r.reduced (0.5f), rad, 1.0f);
}
// valore animato (inseguimento esponenziale), da chiamare nel timer
struct Smooth
{
    float v = 0;
    float step (float target, float k = 0.25f) { v += (target - v) * k; if (std::abs (target - v) < 1.0e-4f) v = target; return v; }
};
// scheda/pannello: ombra morbida, metallo spazzolato, bordo che sfuma dall'accento in alto
inline void card (juce::Graphics& g, juce::Rectangle<float> r, float rad, juce::Colour accent, bool active = true, bool lifted = false, bool shadow = true)
{
    if (shadow) softShadow (g, r, rad, lifted ? 26.0f : 16.0f, lifted ? 0.75f : 0.6f, lifted ? 12.0f : 6.0f);
    juce::ColourGradient body (juce::Colour (0xff22252d), r.getX(), r.getY(), juce::Colour (0xff121419), r.getX(), r.getBottom(), false);
    body.addColour (0.12, juce::Colour (0xff1d2027));
    g.setGradientFill (body); g.fillRoundedRectangle (r, rad);
    // luce dall'alto colorata dall'accento
    if (active)
    {
        juce::ColourGradient top (accent.withAlpha (0.11f), r.getCentreX(), r.getY(), accent.withAlpha (0.0f), r.getCentreX(), r.getY() + juce::jmin (140.0f, r.getHeight() * 0.5f), false);
        g.setGradientFill (top); g.fillRoundedRectangle (r, rad);
    }
    grain (g, r.toNearestInt().reduced ((int) rad / 2), 0.03f);
    // bisello superiore
    juce::ColourGradient bev (juce::Colours::white.withAlpha (0.09f), 0, r.getY(), juce::Colours::transparentWhite, 0, r.getY() + 18, false);
    g.setGradientFill (bev); g.drawRoundedRectangle (r.reduced (0.5f), rad, 1.0f);
    // bordo: accento in alto che sfuma verso il basso
    const juce::Colour a0 = active ? accent.withAlpha (0.75f) : juce::Colours::white.withAlpha (0.12f);
    juce::ColourGradient rim (a0, 0, r.getY(), a0.withAlpha (active ? 0.12f : 0.04f), 0, r.getBottom(), false);
    g.setGradientFill (rim); g.drawRoundedRectangle (r, rad, 1.2f);
    if (active)
    {
        juce::ColourGradient halo (accent.withAlpha (0.22f), 0, r.getY(), accent.withAlpha (0.0f), 0, r.getY() + 60, false);
        g.setGradientFill (halo); g.drawRoundedRectangle (r.expanded (1.5f), rad + 1.5f, 3.0f);
    }
}
// sfondo della finestra: gradiente, luci ambientali, grana, vignettatura
inline void backdrop (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour a1, juce::Colour a2)
{
    juce::ColourGradient bg (juce::Colour (0xff1a1c22), r.getX(), r.getY(), juce::Colour (0xff08090c), r.getX(), r.getBottom(), false);
    g.setGradientFill (bg); g.fillRect (r);
    juce::ColourGradient l1 (a1.withAlpha (0.10f), r.getRight() * 0.82f, r.getY() + 120, juce::Colours::transparentBlack, r.getRight() * 0.82f + 620, r.getY() + 120, true); g.setGradientFill (l1); g.fillRect (r);
    juce::ColourGradient l2 (a2.withAlpha (0.07f), r.getX() + r.getWidth() * 0.22f, r.getBottom() - 120, juce::Colours::transparentBlack, r.getX() + r.getWidth() * 0.22f + 700, r.getBottom() - 120, true); g.setGradientFill (l2); g.fillRect (r);
    grain (g, r.toNearestInt(), 0.045f);
    vignette (g, r, 0.5f);
}
// barra superiore con logo
inline void header (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& product, juce::Colour pc, const juce::String& model, juce::Colour a1, juce::Colour a2)
{
    juce::ColourGradient hg (juce::Colour (0xff25282f), 0, r.getY(), juce::Colour (0xff111317), 0, r.getBottom(), false); g.setGradientFill (hg); g.fillRect (r);
    grain (g, r.toNearestInt(), 0.04f);
    g.setColour (juce::Colours::white.withAlpha (0.06f)); g.drawHorizontalLine ((int) r.getY() + 1, r.getX(), r.getRight());
    // filo luminoso sotto: magenta -> ciano, con alone
    juce::ColourGradient line (a1, r.getX(), 0, a2, r.getRight(), 0, false); line.addColour (0.5, a1.interpolatedWith (a2, 0.5f));
    g.setGradientFill (line); g.fillRect (r.getX(), r.getBottom() - 2, r.getWidth(), 2.0f);
    juce::ColourGradient halo (a1.withAlpha (0.18f), 0, r.getBottom(), juce::Colours::transparentBlack, 0, r.getBottom() + 14, false); g.setGradientFill (halo); g.fillRect (r.getX(), r.getBottom(), r.getWidth(), 14.0f);
    // logo
    auto lf = logo (r.getHeight() * 0.36f);
    const float x0 = r.getX() + 22, cy = r.getCentreY() - 6;
    const float w1 = juce::GlyphArrangement::getStringWidth (lf, "JACKPOT");
    g.setFont (lf);
    g.setColour (juce::Colours::black.withAlpha (0.6f)); g.drawText ("JACKPOT", juce::Rectangle<float> (x0 + 1, cy - 14 + 2, w1 + 10, 28), juce::Justification::centredLeft);
    juce::ColourGradient chrome (juce::Colours::white, 0, cy - 12, juce::Colour (0xff9aa2b1), 0, cy + 12, false); g.setGradientFill (chrome);
    for (float o : { 0.0f, 0.5f, 1.0f }) g.drawText ("JACKPOT", juce::Rectangle<float> (x0 + o, cy - 14, w1 + 10, 28), juce::Justification::centredLeft);
    const float px = x0 + w1 + 12, w2 = juce::GlyphArrangement::getStringWidth (lf, product);
    for (int k = 3; k >= 1; --k) { g.setColour (pc.withAlpha (0.10f)); g.drawText (product, juce::Rectangle<float> (px - k, cy - 14, w2 + 20, 28), juce::Justification::centredLeft); g.drawText (product, juce::Rectangle<float> (px + k, cy - 14, w2 + 20, 28), juce::Justification::centredLeft); }
    g.setColour (pc); for (float o : { 0.0f, 0.5f, 1.0f }) g.drawText (product, juce::Rectangle<float> (px + o, cy - 14, w2 + 20, 28), juce::Justification::centredLeft);
    g.setFont (mono (9.5f)); g.setColour (juce::Colour (0xff7d8494));
    g.drawText (model, juce::Rectangle<float> (x0, cy + 13, 300, 14), juce::Justification::centredLeft);
}
// pulsante in rilievo morbido
inline void button (juce::Graphics& g, juce::Rectangle<float> b, juce::Colour accent, bool lit, bool hover, const juce::String& text, juce::Font f)
{
    softShadow (g, b, 7, 8.0f, 0.5f, 3.0f);
    juce::ColourGradient rg (hover ? juce::Colour (0xff353944) : juce::Colour (0xff2b2e37), 0, b.getY(), juce::Colour (0xff15171c), 0, b.getBottom(), false); g.setGradientFill (rg); g.fillRoundedRectangle (b, 7);
    if (lit) { juce::ColourGradient lg (accent.withAlpha (0.22f), 0, b.getY(), accent.withAlpha (0.05f), 0, b.getBottom(), false); g.setGradientFill (lg); g.fillRoundedRectangle (b, 7); }
    g.setColour (juce::Colours::white.withAlpha (0.08f)); g.drawHorizontalLine ((int) b.getY() + 1, b.getX() + 6, b.getRight() - 6);
    g.setColour (lit ? accent.withAlpha (0.85f) : juce::Colours::white.withAlpha (hover ? 0.2f : 0.1f)); g.drawRoundedRectangle (b, 7, lit ? 1.3f : 1.0f);
    if (lit) { g.setColour (accent.withAlpha (0.14f)); g.drawRoundedRectangle (b.expanded (2), 9, 3.0f); }
    g.setFont (f);
    if (lit) { g.setColour (accent.withAlpha (0.18f)); g.drawText (text, b.translated (-0.7f, 0), juce::Justification::centred); g.drawText (text, b.translated (0.7f, 0), juce::Justification::centred); }
    g.setColour (lit ? accent : (hover ? juce::Colour (0xffdfe3ea) : juce::Colour (0xffa9afbd))); g.drawText (text, b, juce::Justification::centred);
}
// alone dietro una manopola quando ci passi sopra o la giri
inline void knobHalo (juce::Graphics& g, float cx, float cy, float R, juce::Colour c, float amt)
{
    if (amt <= 0.01f) return;
    juce::ColourGradient hg (c.withAlpha (0.22f * amt), cx, cy, c.withAlpha (0.0f), cx + R * 1.25f, cy, true);
    g.setGradientFill (hg); g.fillEllipse (cx - R * 1.25f, cy - R * 1.25f, R * 2.5f, R * 2.5f);
}
// fumetto col valore durante il trascinamento
inline void valueBubble (juce::Graphics& g, juce::Rectangle<float> area, const juce::String& t, juce::Colour c)
{
    auto f = mono (11.0f); const float w = juce::GlyphArrangement::getStringWidth (f, t) + 14;
    auto r = juce::Rectangle<float> (area.getCentreX() - w / 2, area.getY(), w, 18);
    g.setColour (juce::Colours::black.withAlpha (0.8f)); g.fillRoundedRectangle (r, 5);
    g.setColour (c.withAlpha (0.8f)); g.drawRoundedRectangle (r, 5, 1.0f);
    g.setFont (f); g.setColour (c); g.drawText (t, r, juce::Justification::centred);
}
// confronto A/B: due istantanee dello stato del plugin
struct ABCompare
{
    juce::MemoryBlock slot[2]; int cur = 0;
    template <class P> void toggle (P& proc)
    {
        proc.getStateInformation (slot[cur]);
        cur ^= 1;
        if (slot[cur].getSize() > 0) proc.setStateInformation (slot[cur].getData(), (int) slot[cur].getSize());
        else slot[cur] = slot[cur ^ 1];
    }
    template <class P> void copyToOther (P& proc) { proc.getStateInformation (slot[cur ^ 1]); }
    juce::String label() const { return cur == 0 ? "A" : "B"; }
};
// manopola 3D della serie: incavo con arco luminoso, gonna zigrinata, cappello metallico spazzolato
struct KnobLook { bool big = false, bipolar = false; float hover = 0, active = 0, activity = 0; bool enabled = true; };
inline void drawKnob (juce::Graphics& g, juce::Rectangle<float> area, float v, juce::Colour col, const KnobLook& k)
{
    using namespace juce;
    const float size = jmin (area.getWidth(), area.getHeight());
    const float cx = area.getCentreX(), cy = area.getY() + size * 0.5f;
    const float R = size * 0.5f - (k.big ? 5.0f : 3.5f), tw = k.big ? 5.0f : 3.4f;
    const float a0 = -2.36f, a1 = 2.36f, av = a0 + jlimit (0.0f, 1.0f, v) * (a1 - a0);
    const auto c = k.enabled ? col : Colour (0xff4f5562);
    const float live = jlimit (0.0f, 1.0f, jmax (k.hover * 0.6f, k.active, k.activity * 0.8f));
    // alone
    if (live > 0.01f) { ColourGradient hg (c.withAlpha (0.2f * live), cx, cy, c.withAlpha (0.0f), cx + R * 1.3f, cy, true); g.setGradientFill (hg); g.fillEllipse (cx - R * 1.3f, cy - R * 1.3f, R * 2.6f, R * 2.6f); }
    // incavo
    { ColourGradient gr (Colour (0xff050608), cx, cy - R, Colour (0xff1c1f26), cx, cy + R, false); g.setGradientFill (gr); g.fillEllipse (cx - R - tw * 0.6f, cy - R - tw * 0.6f, (R + tw * 0.6f) * 2, (R + tw * 0.6f) * 2); }
    g.setColour (Colours::white.withAlpha (0.06f)); { Path lip; lip.addCentredArc (cx, cy, R + tw * 0.6f, R + tw * 0.6f, 0, 1.9f, 4.4f, true); g.strokePath (lip, PathStrokeType (1.0f)); }
    // tacche
    for (int i = 0; i <= 10; ++i)
    {
        const float a = a0 + i / 10.0f * (a1 - a0), r1 = R + tw * 0.9f, r2 = R + tw * 0.9f + (i % 5 == 0 ? 3.5f : 2.0f);
        g.setColour (Colours::white.withAlpha (i % 5 == 0 ? 0.28f : 0.14f)); g.drawLine (cx + std::sin (a) * r1, cy - std::cos (a) * r1, cx + std::sin (a) * r2, cy - std::cos (a) * r2, 1.0f);
    }
    // pista e arco del valore
    Path track; track.addCentredArc (cx, cy, R, R, 0, a0, a1, true);
    g.setColour (Colour (0xff0b0c10)); g.strokePath (track, PathStrokeType (tw, PathStrokeType::curved, PathStrokeType::rounded));
    g.setColour (Colours::white.withAlpha (0.035f)); g.strokePath (track, PathStrokeType (tw * 0.4f, PathStrokeType::curved, PathStrokeType::rounded));
    const float from = k.bipolar ? 0.0f : a0;
    if (k.enabled && std::abs (av - from) > 0.015f)
    {
        Path val; val.addCentredArc (cx, cy, R, R, 0, jmin (from, av), jmax (from, av), true);
        g.setColour (c.withAlpha (0.14f + 0.28f * live)); g.strokePath (val, PathStrokeType (tw * 3.0f, PathStrokeType::curved, PathStrokeType::rounded));
        ColourGradient vg (c.darker (0.35f), cx - R, cy + R, c.brighter (0.3f + 0.4f * live), cx + std::sin (av) * R, cy - std::cos (av) * R, false);
        g.setGradientFill (vg); g.strokePath (val, PathStrokeType (tw, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour (Colours::white.withAlpha (0.35f)); g.strokePath (val, PathStrokeType (tw * 0.28f, PathStrokeType::curved, PathStrokeType::rounded));
    }
    // corpo
    const float rb = R - tw - (k.big ? 5.0f : 3.5f), rc = rb * 0.76f;
    { Path body; body.addEllipse (cx - rb, cy - rb, rb * 2, rb * 2); DropShadow (Colours::black.withAlpha (0.75f), (int) (rb * 0.55f), { 0, (int) (rb * 0.22f) }).drawForPath (g, body); }
    { ColourGradient sk (Colour (0xff545a66), cx, cy - rb, Colour (0xff0c0d11), cx, cy + rb, false); sk.addColour (0.45, Colour (0xff2a2e36)); g.setGradientFill (sk); g.fillEllipse (cx - rb, cy - rb, rb * 2, rb * 2); }
    // zigrinatura della gonna
    const int nk = k.big ? 60 : 40;
    for (int i = 0; i < nk; ++i)
    {
        const float a = i * MathConstants<float>::twoPi / nk, lit = 0.5f + 0.5f * -std::cos (a);   // piu chiara in alto
        g.setColour ((i % 2 ? Colours::black : Colours::white).withAlpha (i % 2 ? 0.28f : 0.05f + 0.08f * lit));
        g.drawLine (cx + std::sin (a) * rc * 1.02f, cy - std::cos (a) * rc * 1.02f, cx + std::sin (a) * rb * 0.98f, cy - std::cos (a) * rb * 0.98f, 1.0f);
    }
    { Path rim; rim.addCentredArc (cx, cy, rb - 0.5f, rb - 0.5f, 0, -1.2f, 1.2f, true); g.setColour (Colours::white.withAlpha (0.22f)); g.strokePath (rim, PathStrokeType (1.0f)); }
    // cappello spazzolato
    { ColourGradient cg (Colour (0xff3e434e), cx - rc * 0.35f, cy - rc * 0.6f, Colour (0xff121419), cx + rc * 0.5f, cy + rc, true); g.setGradientFill (cg); g.fillEllipse (cx - rc, cy - rc, rc * 2, rc * 2); }
    for (float rr = rc * 0.95f; rr > 2; rr -= 1.6f) { g.setColour (Colours::white.withAlpha (0.018f)); g.drawEllipse (cx - rr, cy - rr, rr * 2, rr * 2, 0.6f); }
    { ColourGradient sp (Colours::white.withAlpha (0.22f), cx - rc * 0.25f, cy - rc * 0.75f, Colours::transparentWhite, cx - rc * 0.25f, cy, true); g.setGradientFill (sp); g.fillEllipse (cx - rc, cy - rc, rc * 2, rc * 2); }
    g.setColour (Colours::black.withAlpha (0.5f)); g.drawEllipse (cx - rc, cy - rc, rc * 2, rc * 2, 1.0f);
    g.setColour (Colours::white.withAlpha (0.08f)); { Path cr; cr.addCentredArc (cx, cy, rc - 1, rc - 1, 0, -1.4f, 1.4f, true); g.strokePath (cr, PathStrokeType (1.0f)); }
    // indicatore
    const float sx = std::sin (av), sy = -std::cos (av);
    g.setColour (c.withAlpha (k.enabled ? 0.35f : 0.1f)); g.drawLine (cx + sx * rc * 0.3f, cy + sy * rc * 0.3f, cx + sx * rc * 0.9f, cy + sy * rc * 0.9f, k.big ? 6.0f : 4.5f);
    g.setColour (k.enabled ? c.brighter (0.7f) : Colour (0xff6b7180)); g.drawLine (cx + sx * rc * 0.34f, cy + sy * rc * 0.34f, cx + sx * rc * 0.88f, cy + sy * rc * 0.88f, k.big ? 2.4f : 1.8f);
    g.setColour (c.brighter (0.4f).withAlpha (k.enabled ? 0.9f : 0.2f)); g.fillEllipse (cx + sx * (rb - 3) - 1.6f, cy + sy * (rb - 3) - 1.6f, 3.2f, 3.2f);
}
}
