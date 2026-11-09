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
}
