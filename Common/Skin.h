#pragma once
// PELLE della serie: i plugin costruiti su jlook/jkit si ridisegnano coi temi.
// CYBER (3), ROSSO (4), RETRO (8), CBRPNK (9: schede pastello, nero pesante), HUD (10: nero, linee bianche, rosso), SCURO (1), CHIARO (2).
// Il tema e' uno per plugin su questo Mac (tutte le istanze uguali), salvato nelle preferenze.
// Ogni plugin ha i suoi colori nel CBRPNK, il suo colore acceso nell'HUD e un emblema suo (la macchina che emula).
#include "Neo.h"

namespace skin
{
inline int& cur() { static int t = -1; return t; }
inline juce::String& plug() { static juce::String p; return p; }
inline juce::File prefsFile() { return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory).getChildFile ("Jackpot").getChildFile ("temi.xml"); }
inline int load (const juce::String& id, int def)
{
    if (auto x = juce::XmlDocument::parse (prefsFile())) return x->getIntAttribute ("t_" + id, def);
    return def;
}
inline void save (const juce::String& id, int t)
{
    auto f = prefsFile(); f.getParentDirectory().createDirectory();
    std::unique_ptr<juce::XmlElement> x (juce::XmlDocument::parse (f).release()); if (x == nullptr) x = std::make_unique<juce::XmlElement> ("JACKPOT_TEMI");
    x->setAttribute ("t_" + id, t); x->writeTo (f);
}
inline juce::String label (int t) { return t == 3 ? "CYBER" : t == 4 ? "ROSSO" : t == 8 ? "RETRO" : t == 9 ? "CBRPNK" : t == 10 ? "HUD" : t == 2 ? "CHIARO" : "SCURO"; }
inline int next (int t) { return t == 3 ? 4 : t == 4 ? 8 : t == 8 ? 9 : t == 9 ? 10 : t == 10 ? 1 : t == 1 ? 2 : 3; }

using namespace neo;
inline bool cb() { return K::theme == 9; }
inline bool hud() { return K::theme == 10; }
inline std::vector<juce::Colour>& cards() { static std::vector<juce::Colour> c; return c; }
inline juce::Colour card0() { return cards().empty() ? juce::Colour (0xffe9a260) : cards()[0]; }
inline juce::Colour cardFor (juce::Rectangle<float> r)
{
    if (cards().empty()) return K::panel0;
    const int k = (int) (std::abs ((int) (r.getX() * 7 + r.getY() * 3 + r.getWidth())) / 11) % (int) cards().size();
    return cards()[(size_t) k];
}
inline const juce::Colour INK { 0xff121212 }, CBRED { 0xffe24834 };
inline juce::Colour shade (juce::Colour c, float k) { return juce::Colour::fromFloatRGBA (c.getFloatRed() * k, c.getFloatGreen() * k, c.getFloatBlue() * k, c.getFloatAlpha()); }
inline juce::Colour lift (juce::Colour c, float k) { return c.interpolatedWith (juce::Colours::white, k); }

// ------------------------------------------------------------------------------------------------ forme
inline juce::Path cham (juce::Rectangle<float> r, float c, bool tl = true, bool tr = false, bool br = true, bool bl = false)
{
    juce::Path p; const float x = r.getX(), y = r.getY(), w = r.getWidth(), h = r.getHeight(); c = juce::jmin (c, w * 0.4f, h * 0.4f);
    p.startNewSubPath (x + (tl ? c : 0), y); p.lineTo (x + w - (tr ? c : 0), y); if (tr) p.lineTo (x + w, y + c);
    p.lineTo (x + w, y + h - (br ? c : 0)); if (br) p.lineTo (x + w - c, y + h); p.lineTo (x + (bl ? c : 0), y + h); if (bl) p.lineTo (x, y + h - c);
    p.lineTo (x, y + (tl ? c : 0)); p.closeSubPath(); return p;
}
inline void hatch (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c, float step = 7.0f, float th = 2.0f)
{
    g.saveState(); g.reduceClipRegion (r.toNearestInt()); g.setColour (c);
    for (float x = r.getX() - r.getHeight(); x < r.getRight(); x += step) g.drawLine (x, r.getBottom(), x + r.getHeight(), r.getY(), th);
    g.restoreState();
}
inline void barcode (juce::Graphics& g, float x, float y, float w, float h, juce::Colour c)
{
    juce::Random rr ((juce::int64) (x * 3 + y)); float k = x; g.setColour (c);
    while (k < x + w) { const float b = (float) (1 + rr.nextInt (3)); g.fillRect (k, y, b, h); k += b + 1.0f + rr.nextFloat(); }
}
inline void circles (juce::Graphics& g, float x, float y, float r, juce::Colour c)
{
    g.setColour (c); g.drawEllipse (x - r, y - r, 2 * r, 2 * r, 1.4f); g.fillEllipse (x + 1.6f * r, y - r, 2 * r, 2 * r);
    g.drawEllipse (x + 4.2f * r, y - r, 2 * r, 2 * r, 1.4f); juce::Path h; h.addPieSegment (x + 4.2f * r, y - r, 2 * r, 2 * r, juce::MathConstants<float>::pi, juce::MathConstants<float>::twoPi, 0); g.fillPath (h);
}
inline void arrow (juce::Graphics& g, float cx, float cy, float s, juce::Colour c)
{
    g.setColour (c); g.drawLine (cx - s * 0.75f, cy + s * 0.7f, cx + s * 0.5f, cy - s * 0.55f, s * 0.28f);
    juce::Path hd; hd.addTriangle (cx + s * 0.9f, cy - s * 0.95f, cx - s * 0.1f, cy - s * 0.82f, cx + s * 0.78f, cy + s * 0.05f); g.fillPath (hd);
}
inline void sphere (juce::Graphics& g, float cx, float cy, float R, juce::Colour c, float w = 1.0f, int n = 7)
{
    g.setColour (c);
    for (int k = 1; k < n; ++k)
    {
        const float a = k / (float) n * juce::MathConstants<float>::pi, q = R * std::abs (std::cos (a));
        g.drawEllipse (cx - R, cy - q, 2 * R, 2 * q, w); g.drawEllipse (cx - q, cy - R, 2 * q, 2 * R, w);
    }
    g.drawEllipse (cx - R, cy - R, 2 * R, 2 * R, w);
}
inline void torus (juce::Graphics& g, float cx, float cy, float R, float r, float tilt, float rot, juce::Colour c, float w = 1.0f, int nu = 18, int nv = 10)
{
    const float ct = std::cos (tilt), st = std::sin (tilt), cr = std::cos (rot), sr = std::sin (rot);
    auto pt = [&] (float u, float v)
    {
        const float x = (R + r * std::cos (v)) * std::cos (u), y = (R + r * std::cos (v)) * std::sin (u), z = r * std::sin (v);
        const float y2 = y * ct - z * st; return juce::Point<float> (cx + x * cr - y2 * sr, cy + x * sr + y2 * cr);
    };
    g.setColour (c); const float tp = juce::MathConstants<float>::twoPi;
    for (int i = 0; i < nu; ++i) { juce::Path p; const float u = i * tp / nu; p.startNewSubPath (pt (u, 0)); for (int j = 1; j <= 20; ++j) p.lineTo (pt (u, j * tp / 20)); g.strokePath (p, juce::PathStrokeType (w)); }
    for (int j = 0; j < nv; ++j) { juce::Path p; const float v = j * tp / nv; p.startNewSubPath (pt (0, v)); for (int i = 1; i <= 40; ++i) p.lineTo (pt (i * tp / 40, v)); g.strokePath (p, juce::PathStrokeType (w)); }
}
inline void tunnel (juce::Graphics& g, juce::Rectangle<float> r, juce::Point<float> vp, juce::Colour c, int n = 9)
{
    g.setColour (c);
    for (int k = 0; k < n; ++k)
    {
        const float t = std::pow (k / (float) n, 1.6f);
        g.drawRect (juce::Rectangle<float>::leftTopRightBottom (r.getX() + (vp.x - r.getX()) * t, r.getY() + (vp.y - r.getY()) * t, r.getRight() - (r.getRight() - vp.x) * t, r.getBottom() - (r.getBottom() - vp.y) * t), 1.0f);
    }
    for (int k = 0; k <= 8; ++k)
    {
        const float f = k / 8.0f; g.drawLine (r.getX() + r.getWidth() * f, r.getY(), vp.x, vp.y, 1.0f); g.drawLine (r.getX() + r.getWidth() * f, r.getBottom(), vp.x, vp.y, 1.0f);
    }
}

// ------------------------------------------------------------------------------------------------ emblema: la macchina di ogni plugin
inline void emblem (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c, float w = 1.6f)
{
    const auto id = plug(); const float cx = r.getCentreX(), cy = r.getCentreY(), s = juce::jmin (r.getWidth(), r.getHeight()) * 0.5f;
    g.setColour (c);
    if (id == "comp")
    {   // strumento a lancetta
        juce::Path a; a.addCentredArc (cx, cy + s * 0.55f, s, s, 0, -1.1f, 1.1f, true); g.strokePath (a, juce::PathStrokeType (w));
        for (int k = 0; k <= 6; ++k) { const float t = -1.1f + 2.2f * k / 6; g.drawLine (cx + std::sin (t) * s * 0.82f, cy + s * 0.55f - std::cos (t) * s * 0.82f, cx + std::sin (t) * s, cy + s * 0.55f - std::cos (t) * s, w * 0.8f); }
        g.drawLine (cx, cy + s * 0.55f, cx + s * 0.55f, cy - s * 0.25f, w * 1.3f); g.fillEllipse (cx - 3, cy + s * 0.55f - 3, 6, 6);
    }
    else if (id == "tube")
    {   // valvola con filamento
        juce::Path b; b.addRoundedRectangle (cx - s * 0.45f, cy - s * 0.95f, s * 0.9f, s * 1.5f, s * 0.42f); g.strokePath (b, juce::PathStrokeType (w));
        g.fillRect (cx - s * 0.5f, cy + s * 0.55f, s, s * 0.35f);
        juce::Path f; f.startNewSubPath (cx - s * 0.2f, cy + s * 0.4f); f.lineTo (cx - s * 0.2f, cy - s * 0.3f); f.lineTo (cx, cy - s * 0.05f); f.lineTo (cx + s * 0.2f, cy - s * 0.3f); f.lineTo (cx + s * 0.2f, cy + s * 0.4f); g.strokePath (f, juce::PathStrokeType (w));
    }
    else if (id == "clip")
    {   // onda tagliata
        juce::Path p; for (int i = 0; i <= 40; ++i) { const float t = i / 40.0f; const float y = juce::jlimit (-0.55f, 0.55f, std::sin (t * 12.566f)); const juce::Point<float> q (cx - s + 2 * s * t, cy - y * s); if (i == 0) p.startNewSubPath (q); else p.lineTo (q); }
        g.strokePath (p, juce::PathStrokeType (w)); g.drawLine (cx - s, cy - s * 0.62f, cx + s, cy - s * 0.62f, w * 0.6f); g.drawLine (cx - s, cy + s * 0.62f, cx + s, cy + s * 0.62f, w * 0.6f);
    }
    else if (id == "meter")
    {   // colonne di misura
        const float h[5] = { 0.5f, 0.9f, 0.7f, 1.0f, 0.4f };
        for (int k = 0; k < 5; ++k) g.fillRect (cx - s + k * s * 0.42f, cy + s - 2 * s * h[k], s * 0.28f, 2 * s * h[k]);
    }
    else if (id == "mix")
    {   // tre fader
        const float p[3] = { 0.3f, -0.4f, 0.1f };
        for (int k = 0; k < 3; ++k) { const float x = cx - s * 0.7f + k * s * 0.7f; g.drawLine (x, cy - s, x, cy + s, w * 0.8f); g.fillRoundedRectangle (x - s * 0.2f, cy + p[k] * s - s * 0.12f, s * 0.4f, s * 0.24f, 1.5f); }
    }
    else if (id == "motion")
    {   // LFO con freccia
        juce::Path p; for (int i = 0; i <= 40; ++i) { const float t = i / 40.0f; const juce::Point<float> q (cx - s + 2 * s * t, cy - std::sin (t * 6.283f) * s * 0.6f); if (i == 0) p.startNewSubPath (q); else p.lineTo (q); }
        g.strokePath (p, juce::PathStrokeType (w)); juce::Path hd; hd.addTriangle (cx + s, cy, cx + s * 0.7f, cy - s * 0.25f, cx + s * 0.7f, cy + s * 0.25f); g.fillPath (hd);
    }
    else if (id == "phase")
    {   // due cerchi sfasati
        g.drawEllipse (cx - s, cy - s * 0.7f, s * 1.4f, s * 1.4f, w); g.drawEllipse (cx - s * 0.4f, cy - s * 0.7f, s * 1.4f, s * 1.4f, w);
        g.drawLine (cx - s * 0.3f, cy, cx + s * 0.3f, cy, w);
    }
    else if (id == "rewind")
    {   // cassetta e riavvolgi
        g.drawRoundedRectangle (cx - s, cy - s * 0.65f, 2 * s, s * 1.3f, 3, w);
        g.drawEllipse (cx - s * 0.62f, cy - s * 0.32f, s * 0.5f, s * 0.5f, w); g.drawEllipse (cx + s * 0.12f, cy - s * 0.32f, s * 0.5f, s * 0.5f, w);
        juce::Path t; t.addTriangle (cx - s * 0.2f, cy + s * 0.42f, cx + s * 0.05f, cy + s * 0.28f, cx + s * 0.05f, cy + s * 0.56f); t.addTriangle (cx + s * 0.05f, cy + s * 0.42f, cx + s * 0.3f, cy + s * 0.28f, cx + s * 0.3f, cy + s * 0.56f); g.fillPath (t);
    }
    else if (id == "alter")
    {   // fulmine di distorsione
        juce::Path p; p.startNewSubPath (cx + s * 0.2f, cy - s); p.lineTo (cx - s * 0.45f, cy + s * 0.1f); p.lineTo (cx + s * 0.05f, cy + s * 0.1f); p.lineTo (cx - s * 0.25f, cy + s); p.lineTo (cx + s * 0.5f, cy - s * 0.2f); p.lineTo (cx, cy - s * 0.2f); p.closeSubPath();
        g.fillPath (p);
    }
    else if (id == "dust")
    {   // disco con polvere
        g.drawEllipse (cx - s, cy - s, 2 * s, 2 * s, w); g.drawEllipse (cx - s * 0.6f, cy - s * 0.6f, s * 1.2f, s * 1.2f, w * 0.6f); g.fillEllipse (cx - s * 0.15f, cy - s * 0.15f, s * 0.3f, s * 0.3f);
        juce::Random rr (7); for (int k = 0; k < 9; ++k) g.fillEllipse (cx - s * 1.2f + rr.nextFloat() * s * 2.4f, cy - s * 1.2f + rr.nextFloat() * s * 2.4f, 2.2f, 2.2f);
    }
    else sphere (g, cx, cy, s, c, w * 0.6f, 6);
}

// ------------------------------------------------------------------------------------------------ caratteri
inline bool fonts (int kind, float size, int weight, juce::Font& out)
{
    if (K::theme == 9)
    {
        const int t = kind == 2 ? 4 : kind == 1 ? 5 : weight >= 2 ? 4 : weight == 1 ? 6 : 5;
        out = juce::Font (juce::FontOptions (tf (t)).withHeight (size * (t == 4 ? 1.0f : 1.02f))); return true;
    }
    if (K::theme == 10)
    {
        if (kind == 0) { out = juce::Font (juce::FontOptions (tf (9)).withHeight (size * 1.12f)); return true; }
        return false;   // numeri e logo: Share Tech Mono e Michroma di serie
    }
    return false;
}

// ------------------------------------------------------------------------------------------------ disegno
inline void fakeShadow (juce::Graphics& g, const juce::Path& p, float dx, float dy, float a)
{
    for (int k = 3; k >= 1; --k) { g.setColour (juce::Colours::black.withAlpha (a / 3.0f)); g.fillPath (p, juce::AffineTransform::translation (dx * k / 3.0f + k * 0.5f, dy * k / 3.0f + k * 0.8f)); }
}
inline void panel (juce::Graphics& g, juce::Rectangle<float> r, float rad, juce::Colour accent, bool active, bool lifted)
{
    juce::ignoreUnused (lifted);
    const auto tag = active ? accent : K::faint;
    if (cb())
    {   // scheda di plastica: spessore sotto, luce sul bordo
        const auto c = cardFor (r); const float rr = juce::jlimit (8.0f, 18.0f, rad + 4);
        juce::Path p; p.addRoundedRectangle (r, rr);
        fakeShadow (g, p, 2, 7, 0.55f);
        g.setColour (shade (c, 0.6f)); g.fillPath (p, juce::AffineTransform::translation (0, 5));
        juce::ColourGradient cg (lift (c, 0.12f), 0, r.getY(), shade (c, 0.94f), 0, r.getBottom(), false); g.setGradientFill (cg); g.fillPath (p);
        g.setColour (lift (c, 0.45f)); g.drawRoundedRectangle (r.reduced (1.2f), rr - 1, 1.2f);
        if (r.getWidth() > 180 && r.getHeight() > 60)
        {
            const float y = r.getBottom() - 10, x0 = r.getRight() - 42;
            circles (g, x0, y, 3.5f, INK); barcode (g, x0 - 64, y - 4, 56, 7, INK);
        }
        if (active) { g.setColour (CBRED); g.fillRoundedRectangle (r.getX() + rr, r.getY() + 3, 20.0f, 4.0f, 1.5f); }
        return;
    }
    if (hud())
    {   // pannello tecnico: angoli tagliati, spessore, filo di luce, crocini
        auto p = cham (r, 12);
        fakeShadow (g, p, 4, 7, 0.8f);
        g.setColour (juce::Colour (0xff17171b)); g.fillPath (p, juce::AffineTransform::translation (3, 4));
        juce::ColourGradient cg (juce::Colour (0xee1d1d22), 0, r.getY(), juce::Colour (0xf0070709), 0, r.getBottom(), false); g.setGradientFill (cg); g.fillPath (p);
        g.setColour (juce::Colour (0xffe8e8e8)); g.strokePath (p, juce::PathStrokeType (1.1f));
        g.setColour (juce::Colour (0xff36363c)); g.strokePath (cham (r.reduced (4), 9), juce::PathStrokeType (0.8f));
        g.setColour (juce::Colours::white); g.fillRect (r.getX() + 14, r.getY() + 1, r.getWidth() * 0.4f, 1.6f);
        g.setColour (active ? accent : K::accent); g.fillRect (r.getX() + 14, r.getY() - 1.5f, 22.0f, 3.0f);
        if (r.getWidth() > 120 && r.getHeight() > 50)
        {
            g.setColour (juce::Colour (0xff6e6e74)); for (int k = 0; k < 9; ++k) g.fillRect (r.getRight() - 56 + k * 5.0f, r.getBottom() - 8, 2.0f, 4.0f);
            for (auto q : { juce::Point<float> (r.getX() - 5, r.getBottom() + 5), juce::Point<float> (r.getRight() + 5, r.getY() - 5) }) { g.drawLine (q.x - 4, q.y, q.x + 4, q.y, 1.0f); g.drawLine (q.x, q.y - 4, q.x, q.y + 4, 1.0f); }
        }
        return;
    }
    if (cyber()) { plate (g, r, tag, juce::jlimit (8.0f, 14.0f, rad)); return; }
    rad = juce::jmin (rad, 12.0f);
    g.setColour (K::lo.withAlpha (light() ? 0.10f : 0.45f)); g.fillRoundedRectangle (r.translated (0, 3), rad);
    juce::ColourGradient cg (K::panel0, 0, r.getY(), K::panel1, 0, r.getBottom(), false); g.setGradientFill (cg); g.fillRoundedRectangle (r, rad);
    g.setColour (K::hi.withAlpha (light() ? 0.10f : 0.07f)); g.drawRoundedRectangle (r.reduced (0.5f), rad, 1.0f);
    g.setColour (K::hi.withAlpha (light() ? 0.0f : 0.06f)); g.drawHorizontalLine ((int) r.getY() + 1, r.getX() + rad, r.getRight() - rad);
    if (active) { g.setColour (tag.withAlpha (0.9f)); g.fillRoundedRectangle (r.getX() + rad + 4, r.getY() - 1, juce::jmin (46.0f, r.getWidth() * 0.25f), 2.5f, 1.2f); }
}
// sfondo: calcolato una volta e tenuto in memoria (e' pieno di dettagli)
inline void backdrop (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour, juce::Colour)
{
    const float W = r.getWidth(), H = r.getHeight();
    if (K::theme == 8) { memphisBackdrop (g, W, H); return; }
    if (K::theme == 3) { cyberBackdrop (g, W, H, 0.6f); return; }
    if (cb() || hud())
    {
        static juce::Image img; static juce::String key;
        const juce::String k = juce::String (K::theme) + "|" + juce::String ((int) W) + "x" + juce::String ((int) H) + "|" + plug() + juce::String::toHexString ((int) K::hudAcc);
        if (img.isNull() || key != k)
        {
            key = k; img = juce::Image (juce::Image::RGB, juce::jmax (1, (int) W), juce::jmax (1, (int) H), true);
            juce::Graphics bg (img);
            if (cb())
            {
                bg.fillAll (juce::Colour (0xff101010));
                bg.setColour (juce::Colours::white.withAlpha (0.08f)); for (float y = 7; y < H; y += 14) for (float x = 7; x < W; x += 14) bg.fillRect (x, y, 1.4f, 1.4f);
                torus (bg, W * 0.52f, H * 0.6f, W * 0.27f, H * 0.14f, 1.15f, 0.3f, CBRED.withAlpha (0.16f), 1.0f, 22, 14);
                sphere (bg, W * 0.08f, H * 0.9f, 70, juce::Colours::white.withAlpha (0.06f), 1.0f, 7);
            }
            else
            {
                bg.fillAll (juce::Colour (0xff060608));
                bg.setColour (juce::Colours::white.withAlpha (0.035f)); for (float x = 0; x < W; x += 40) bg.fillRect (x, 0.0f, 1.0f, H); for (float y = 0; y < H; y += 40) bg.fillRect (0.0f, y, W, 1.0f);
                tunnel (bg, { 0, 60, W, H - 60 }, { W * 0.5f, H * 0.56f }, juce::Colours::white.withAlpha (0.06f), 10);
                juce::Random rr (3); bg.setFont (jlook::mono (7.5f)); bg.setColour (K::accent.withAlpha (0.13f));
                for (float x = 6; x < W; x += 92) for (float y = 70; y < H; y += 13) if (rr.nextBool()) bg.drawText (juce::String::toHexString (rr.nextInt (256)).paddedLeft ('0', 2).toUpperCase() + " " + juce::String::toHexString (rr.nextInt (256)).paddedLeft ('0', 2).toUpperCase(), juce::Rectangle<float> (x, y, 60, 12), juce::Justification::centredLeft);
                sphere (bg, W * 0.13f, H * 0.86f, 110, juce::Colours::white.withAlpha (0.07f), 1.0f, 9);
                torus (bg, W * 0.88f, H * 0.86f, 150, 55, 1.2f, 0.5f, K::accent.withAlpha (0.14f), 1.0f, 20, 12);
                bg.setColour (juce::Colours::black.withAlpha (0.18f)); for (float y = 0; y < H; y += 3) bg.fillRect (0.0f, y, W, 1.0f);
            }
        }
        g.drawImageAt (img, (int) r.getX(), (int) r.getY());
        return;
    }
    juce::ColourGradient base (K::bg1, 0, 0, K::bg0, 0, H, false); g.setGradientFill (base); g.fillRect (r);
    if (K::theme == 4) { g.setColour (juce::Colours::black.withAlpha (0.10f)); for (float x = -H; x < W; x += 9) g.drawLine (x, H, x + H, 0, 1.0f); }
    juce::ColourGradient vg (juce::Colours::transparentBlack, r.getCentreX(), r.getCentreY(), juce::Colours::black.withAlpha (light() ? 0.10f : 0.35f), 0, 0, true);
    vg.addColour (0.6, juce::Colours::transparentBlack); g.setGradientFill (vg); g.fillRect (r);
}
inline void header (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& product, juce::Colour pc, const juce::String& model)
{
    juce::ignoreUnused (pc);
    const float W = r.getWidth(), h = r.getHeight(), cy = r.getCentreY();
    if (cb())
    {   // scheda grande: nome nero pesante col puntino rosso, emblema, freccia, tratteggio
        auto c = r.reduced (8, 6); const auto col = card0();
        juce::Path p; p.addRoundedRectangle (c, 14);
        g.setColour (shade (col, 0.6f)); g.fillPath (p, juce::AffineTransform::translation (0, 4));
        juce::ColourGradient cg (lift (col, 0.12f), 0, c.getY(), shade (col, 0.94f), 0, c.getBottom(), false); g.setGradientFill (cg); g.fillPath (p);
        g.setColour (lift (col, 0.45f)); g.drawRoundedRectangle (c.reduced (1.2f), 13, 1.2f);
        const auto nf = juce::Font (juce::FontOptions (tf (4)).withHeight (juce::jmin (30.0f, c.getHeight() * 0.62f)));
        const auto name = product.toUpperCase(); const float wn = juce::GlyphArrangement::getStringWidth (nf, name);
        g.setFont (nf); g.setColour (INK); g.drawText (name, juce::Rectangle<float> (c.getX() + 16, c.getY() - 2, wn + 8, c.getHeight() - 8), juce::Justification::centredLeft);
        g.setColour (CBRED); g.fillEllipse (c.getX() + 20 + wn, c.getY() + 7, 7, 7);
        g.setFont (juce::Font (juce::FontOptions (tf (5)).withHeight (8.5f))); g.setColour (INK);
        g.drawText ("JACKPOT // " + model.toUpperCase(), juce::Rectangle<float> (c.getX() + 18, c.getBottom() - 15, 360, 11), juce::Justification::centredLeft);
        const float ex = c.getX() + 46 + wn; emblem (g, { ex, c.getY() + 8, 34, c.getHeight() - 16 }, INK, 1.8f);
        arrow (g, ex + 64, c.getCentreY(), c.getHeight() * 0.32f, INK);
        hatch (g, { c.getRight() - 40, c.getY() + 7, 26, c.getHeight() - 14 }, INK, 6.0f, 2.2f);
        return;
    }
    if (hud())
    {
        juce::ColourGradient hg (juce::Colour (0xff17171b), 0, 0, juce::Colour (0xff040406), 0, h, false); g.setGradientFill (hg); g.fillRect (r);
        g.setColour (juce::Colours::white); g.fillRect (0.0f, r.getBottom() - 2, W, 1.0f); g.setColour (K::accent); g.fillRect (0.0f, r.getBottom(), W, 1.0f);
        const auto nf = jlook::logo (juce::jmin (19.0f, h * 0.32f)); const auto name = product.toUpperCase(); const float wn = juce::GlyphArrangement::getStringWidth (nf, name);
        g.setFont (nf); g.setColour (juce::Colours::white); g.drawText (name, juce::Rectangle<float> (22, cy - 20, wn + 8, 24), juce::Justification::centredLeft);
        const auto tf2 = jlook::mono (8.0f); const juce::String tag = "WARNING // " + model.toUpperCase(); const float wt = juce::GlyphArrangement::getStringWidth (tf2, tag) + 14;
        g.setColour (K::accent); g.fillRect (22.0f, cy + 7, wt, 12.0f); g.setFont (tf2); g.setColour (juce::Colours::black); g.drawText (tag, juce::Rectangle<float> (28, cy + 7, wt, 12), juce::Justification::centredLeft);
        const float ex = 40 + juce::jmax (wn, wt); emblem (g, { ex, cy - 16, 34, 32 }, K::accent, 1.4f);
        hatch (g, { ex + 50, cy - 14, 54, 28 }, juce::Colours::white, 7.0f, 2.0f);
        return;
    }
    if (K::theme == 8)
    {
        g.setColour (juce::Colour (0xffefe2cc)); g.fillRect (r);
        static const juce::uint32 st[4] = { 0xffe0663d, 0xffe8b448, 0xff5f9c93, 0xff6d58a6 };
        for (int i = 0; i < 4; ++i) { g.setColour (juce::Colour (st[i])); g.fillRect (0.0f, r.getBottom() - 10 + i * 2.5f, W, 2.5f); }
    }
    else if (cyber()) { g.setColour (K::lo.withAlpha (0.5f)); g.fillRect (r); g.setColour (K::accent.withAlpha (0.35f)); g.fillRect (0.0f, h - 1, W, 1.0f); }
    else
    {
        juce::ColourGradient hg (K::panel0, 0, 0, K::panel1, 0, h, false); g.setGradientFill (hg); g.fillRect (r);
        g.setColour (K::hi.withAlpha (0.08f)); g.fillRect (0.0f, h - 1, W, 1.0f); g.setColour (K::accent); g.fillRect (0.0f, h - 2, 120.0f, 2.0f);
    }
    const bool cream = K::theme == 8;
    const auto ink = cream ? juce::Colour (0xff2a1f3a) : K::ink, acc = cream ? juce::Colour (0xffe0663d) : K::accent;
    { auto lr = juce::Rectangle<float> (22, cy - 13, 26, 26);
      if (cyber()) { g.setColour (acc); g.strokePath (chamfer (lr, 7.0f, 1.5f), juce::PathStrokeType (1.6f)); g.setColour (cream ? juce::Colour (0xff6d58a6) : K::pink); g.fillRect (lr.reduced (8)); }
      else { g.setColour (acc); g.drawEllipse (lr.reduced (1), 1.6f); g.fillEllipse (lr.reduced (8)); } }
    g.setFont (cyber() ? jlook::logo (15.0f) : big (15.0f)); g.setColour (ink);
    const float w1 = juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), "JACKPOT");
    g.drawText ("JACKPOT", juce::Rectangle<float> (60, cy - 15, w1 + 4, 20), juce::Justification::centredLeft);
    g.setColour (acc); g.drawText (product, juce::Rectangle<float> (68 + w1, cy - 15, 260, 20), juce::Justification::centredLeft);
    g.setFont (num (8.5f)); g.setColour (cream ? juce::Colour (0xff6a5a7d) : K::faint); g.drawText (model.toUpperCase(), juce::Rectangle<float> (60, cy + 6, 380, 12), juce::Justification::centredLeft);
}
inline void button (juce::Graphics& g, juce::Rectangle<float> b, juce::Colour c, bool lit, bool hover, const juce::String& text, juce::Font fo)
{
    if (cb())
    {   // pillola in rilievo; accesa = nera premuta
        const float rad = juce::jmin (b.getHeight() * 0.5f, 14.0f); const auto base = card0();
        g.setColour (lit ? juce::Colours::black : shade (base, 0.55f)); g.fillRoundedRectangle (b.translated (0, 2.5f), rad);
        if (lit) { g.setColour (INK); g.fillRoundedRectangle (b, rad); g.setColour (juce::Colour (0xff2c2c2c)); g.fillRoundedRectangle (b.reduced (3, 2).withHeight (b.getHeight() * 0.45f), rad); }
        else { g.setColour (hover ? lift (base, 0.3f) : lift (base, 0.14f)); g.fillRoundedRectangle (b, rad); g.setColour (INK); g.drawRoundedRectangle (b.reduced (0.7f), rad, 1.4f); }
        g.setFont (juce::Font (juce::FontOptions (tf (4)).withHeight (juce::jmin (11.5f, b.getHeight() * 0.44f))));
        g.setColour (lit ? base : INK); g.drawFittedText (text, b.toNearestInt().reduced (5, 0), juce::Justification::centred, 1, 0.7f); juce::ignoreUnused (c, fo);
        return;
    }
    if (hud())
    {
        auto p = cham (b, juce::jmin (6.0f, b.getHeight() * 0.3f));
        g.setColour (juce::Colours::black); g.fillPath (p, juce::AffineTransform::translation (2, 3));
        if (lit)
        {
            g.setColour (K::accent.withAlpha (0.25f)); g.strokePath (p, juce::PathStrokeType (5.0f));
            juce::ColourGradient lg (K::accent.interpolatedWith (juce::Colours::white, 0.35f), 0, b.getY(), K::accent, 0, b.getCentreY(), false); g.setGradientFill (lg); g.fillPath (p);
        }
        else { juce::ColourGradient ug (juce::Colour (0xff26262b), 0, b.getY(), juce::Colour (0xff0e0e10), 0, b.getBottom(), false); g.setGradientFill (ug); g.fillPath (p); g.setColour (juce::Colours::white.withAlpha (hover ? 1.0f : 0.85f)); g.strokePath (p, juce::PathStrokeType (1.0f)); }
        g.setFont (juce::Font (juce::FontOptions (tf (9)).withHeight (juce::jmin (12.5f, b.getHeight() * 0.5f))));
        g.setColour (lit ? juce::Colours::black : juce::Colours::white); g.drawFittedText (text, b.toNearestInt().reduced (4, 0), juce::Justification::centred, 1, 0.7f); juce::ignoreUnused (c, fo);
        return;
    }
    if (cyber()) key (g, b, lit, c, hover);
    else
    {
        const float rad = juce::jmin (7.0f, b.getHeight() * 0.35f);
        if (lit) { g.setColour (c.withAlpha (0.20f)); g.fillRoundedRectangle (b, rad); g.setColour (c.withAlpha (0.85f)); g.drawRoundedRectangle (b.reduced (0.5f), rad, 1.0f); }
        else { g.setColour (K::hi.withAlpha (hover ? 0.09f : 0.045f)); g.fillRoundedRectangle (b, rad); g.setColour (K::hi.withAlpha (hover ? 0.2f : 0.1f)); g.drawRoundedRectangle (b.reduced (0.5f), rad, 1.0f); }
    }
    g.setFont (cap (juce::jmin (10.0f, fo.getHeight() * 0.72f)));
    g.setColour (lit ? (light() ? c.darker (0.45f) : K::ink) : (hover ? K::ink : K::sub));
    g.drawFittedText (text, b.toNearestInt().reduced (lit && cyber() ? 7 : 4, 0), juce::Justification::centred, 1, 0.7f);
}
inline void knob (juce::Graphics& g, juce::Rectangle<float> area, float v, juce::Colour col, const jlook::KnobLook& k)
{
    v = juce::jlimit (0.0f, 1.0f, v);
    const float size = juce::jmin (area.getWidth(), area.getHeight());
    const float cx = area.getCentreX(), cy = area.getY() + size * 0.5f, R = size * 0.5f - 3.0f, kr = R * (k.big ? 0.72f : 0.7f);
    const auto c = k.enabled ? col : K::faint;
    const float av = -2.356f + 4.712f * v;
    if (cb())
    {   // fader verticale in una finestra incavata, col cappuccio bianco (come le colonne ADSR)
        const float fw = juce::jmax (14.0f, size * 0.38f), top = cy - R + 2, bot = cy + R - 2;
        auto well = juce::Rectangle<float> (cx - fw * 0.5f, top, fw, bot - top);
        g.setColour (INK.withAlpha (0.13f)); g.fillRoundedRectangle (well, 4); g.setColour (INK.withAlpha (0.16f)); g.fillRoundedRectangle (well.withHeight (5), 4);
        g.setColour (INK); g.drawRoundedRectangle (well, 4, 1.4f);
        for (int t = 1; t < 10; ++t) g.fillRect (well.getX() - 7, top + t * (bot - top) / 10, 4.0f, 1.2f);
        const float ih = well.getHeight() - 8, mid = well.getBottom() - 4 - (k.bipolar ? ih * 0.5f : 0.0f), yv = well.getBottom() - 4 - ih * v;
        g.setColour (k.active > 0.5f || k.hover > 0.5f ? CBRED : INK);
        g.fillRoundedRectangle (juce::Rectangle<float>::leftTopRightBottom (well.getX() + 3.5f, juce::jmin (mid, yv), well.getRight() - 3.5f, juce::jmax (mid, yv)), 2);
        const float cyc = juce::jlimit (top + 6, bot - 6, yv);
        auto capR = juce::Rectangle<float> (well.getX() - 4, cyc - 6, well.getWidth() + 8, 11);
        g.setColour (juce::Colours::black); g.fillRoundedRectangle (capR.translated (0, 2), 2.5f);
        g.setColour (juce::Colour (0xffe8e4dc)); g.fillRoundedRectangle (capR, 2.5f); g.setColour (juce::Colour (0xfffaf8f2)); g.fillRoundedRectangle (capR.withHeight (4), 2.5f);
        g.setColour (INK); g.fillRect (capR.getX() + 4, capR.getCentreY() - 0.5f, capR.getWidth() - 8, 1.6f);
        juce::ignoreUnused (c, kr, av);
        return;
    }
    if (hud())
    {   // metallo scuro con riflesso, anelli, scala a tacche e arco acceso che brilla (sempre il colore del tema)
        const auto c = k.enabled ? K::accent : K::faint;
        for (int t = 0; t <= 27; ++t)
        {
            const float a = -2.356f + 4.712f * t / 27.0f; g.setColour (t % 3 == 0 ? juce::Colour (0xffd8d8d8) : juce::Colour (0xff5c5c62));
            g.drawLine (cx + std::sin (a) * (R + 1), cy - std::cos (a) * (R + 1), cx + std::sin (a) * (R + 4), cy - std::cos (a) * (R + 4), 1.0f);
        }
        juce::Path tr; tr.addCentredArc (cx, cy, R - 3, R - 3, 0, -2.356f, 2.356f, true); g.setColour (juce::Colour (0xff2a2a2e)); g.strokePath (tr, juce::PathStrokeType (3.0f));
        juce::Path va; va.addCentredArc (cx, cy, R - 3, R - 3, 0, k.bipolar ? 0.0f : -2.356f, av, true);
        g.setColour (c.withAlpha (0.22f)); g.strokePath (va, juce::PathStrokeType (8.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (c); g.strokePath (va, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        const float br = R - 8;
        g.setColour (juce::Colours::black.withAlpha (0.7f)); g.fillEllipse (cx - br + 2, cy - br + 4, br * 2, br * 2);
        juce::ColourGradient mg (juce::Colour (0xff6a6a72), cx - br * 0.4f, cy - br * 0.5f, juce::Colour (0xff0e0e10), cx + br * 0.6f, cy + br, true); g.setGradientFill (mg); g.fillEllipse (cx - br, cy - br, br * 2, br * 2);
        g.setColour (juce::Colours::white.withAlpha (0.55f)); g.drawEllipse (cx - br * 0.78f, cy - br * 0.78f, br * 1.56f, br * 1.56f, 0.8f);
        g.setColour (juce::Colour (0xff3a3a40)); g.drawEllipse (cx - br * 0.55f, cy - br * 0.55f, br * 1.1f, br * 1.1f, 0.8f);
        juce::ColourGradient cp (juce::Colour (0xff2c2c30), cx, cy - br * 0.4f, juce::Colour (0xff060607), cx, cy + br * 0.4f, false); g.setGradientFill (cp); g.fillEllipse (cx - br * 0.42f, cy - br * 0.42f, br * 0.84f, br * 0.84f);
        const float sn = std::sin (av), cs = std::cos (av);
        g.setColour (c.withAlpha (0.3f)); g.drawLine (cx + sn * br * 0.2f, cy - cs * br * 0.2f, cx + sn * (br - 2), cy - cs * (br - 2), 5.0f);
        g.setColour (c); g.drawLine (cx + sn * br * 0.2f, cy - cs * br * 0.2f, cx + sn * (br - 2), cy - cs * (br - 2), 2.2f);
        juce::ignoreUnused (kr);
        return;
    }
    const float live = juce::jmax (k.hover * 0.6f, k.active);
    if (cyber() || K::theme == 4) ledRing (g, cx, cy, R, v, k.bipolar, live > 0.01f ? c.brighter (0.2f) : c, k.big ? 3.2f : 2.4f, k.big ? 31 : 23);
    else
    {
        juce::Path tr; tr.addCentredArc (cx, cy, R - 1, R - 1, 0, -2.356f, 2.356f, true); g.setColour (K::hi.withAlpha (light() ? 0.14f : 0.09f)); g.strokePath (tr, juce::PathStrokeType (2.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        juce::Path va; va.addCentredArc (cx, cy, R - 1, R - 1, 0, k.bipolar ? 0.0f : -2.356f, av, true);
        g.setColour (c); g.strokePath (va, juce::PathStrokeType (2.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    g.setColour (K::lo.withAlpha (light() ? 0.18f : 0.45f)); g.fillEllipse (cx - kr, cy - kr + 2, kr * 2, kr * 2);
    const auto top = light() ? juce::Colour (0xfffbf9f6) : juce::Colour (0xff2a2e3a), bot = light() ? juce::Colour (0xffd9d4cd) : juce::Colour (0xff111319);
    juce::ColourGradient bgc (top, cx, cy - kr, bot, cx, cy + kr, false); g.setGradientFill (bgc); g.fillEllipse (cx - kr, cy - kr, kr * 2, kr * 2);
    g.setColour (K::hi.withAlpha (light() ? 0.12f : 0.08f)); g.drawEllipse (cx - kr + 0.5f, cy - kr + 0.5f, kr * 2 - 1, kr * 2 - 1, 1.0f);
    const float r0 = kr * 0.3f, r1 = kr * 0.88f, sn = std::sin (av), cs = std::cos (av);
    juce::Path pl; pl.startNewSubPath (cx + sn * r0, cy - cs * r0); pl.lineTo (cx + sn * r1, cy - cs * r1);
    g.setColour (c); g.strokePath (pl, juce::PathStrokeType (k.big ? 2.6f : 2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}
inline void glass (juce::Graphics& g, juce::Rectangle<float> r, float rad)
{
    if (cb() || hud()) return;
    if (cyber()) { brackets (g, r.reduced (3), K::accent.withAlpha (0.45f), 6.0f, 1.0f); return; }
    g.setColour (K::hi.withAlpha (light() ? 0.10f : 0.06f)); g.drawRoundedRectangle (r.reduced (0.5f), rad, 1.0f);
}
inline void bubble (juce::Graphics& g, juce::Rectangle<float> area, const juce::String& t, juce::Colour c)
{
    auto fo = num (10.5f); const float w = juce::GlyphArrangement::getStringWidth (fo, t) + 14;
    auto r = juce::Rectangle<float> (area.getCentreX() - w / 2, area.getY(), w, 18);
    if (cb()) { g.setColour (INK); g.fillRoundedRectangle (r, 9); g.setFont (jlook::mono (10.0f)); g.setColour (card0()); g.drawText (t, r, juce::Justification::centred); return; }
    if (hud()) { g.setColour (K::accent); g.fillPath (cham (r, 4)); g.setFont (jlook::mono (10.0f)); g.setColour (juce::Colours::black); g.drawText (t, r, juce::Justification::centred); return; }
    g.setColour (K::panel1); g.fillRoundedRectangle (r, 4); g.setColour (c.withAlpha (0.8f)); g.drawRoundedRectangle (r, 4, 1.0f);
    g.setFont (fo); g.setColour (light() ? c.darker (0.4f) : c); g.drawText (t, r, juce::Justification::centred);
}
// schermo: sostituisce gli LCD dei vecchi plugin
inline void screen (juce::Graphics& g, juce::Rectangle<float> r, float rad, juce::Colour edge)
{
    if (cb())
    {   // finestra incavata nella scheda: carta chiara, ombra interna in alto, bordo nero
        const float rr = juce::jmin (rad + 2, 9.0f); const auto base = cardFor (r);
        g.setColour (shade (lift (base, 0.35f), 0.9f)); g.fillRoundedRectangle (r, rr);
        g.setColour (shade (base, 0.72f).withAlpha (0.8f)); g.fillRoundedRectangle (r.withHeight (5), rr);
        g.setColour (INK); g.drawRoundedRectangle (r.reduced (0.7f), rr, 1.4f); juce::ignoreUnused (edge); return;
    }
    if (hud())
    {
        auto p = cham (r, juce::jmin (9.0f, rad + 4), true, true, true, true);
        juce::ColourGradient cg (K::disp0, 0, r.getY(), K::disp1, 0, r.getBottom(), false); g.setGradientFill (cg); g.fillPath (p);
        g.setColour (juce::Colour (0xff4a4a50)); g.strokePath (p, juce::PathStrokeType (1.0f)); juce::ignoreUnused (edge); return;
    }
    if (cyber())
    {
        auto p = chamfer (r, juce::jmin (8.0f, rad + 2), 2.0f);
        g.setColour (K::lo.withAlpha (0.7f)); g.fillPath (chamfer (r.expanded (1.5f), juce::jmin (9.0f, rad + 3), 2.5f));
        juce::ColourGradient cg (K::disp0, r.getCentreX(), r.getY() + r.getHeight() * 0.35f, K::disp1, r.getX(), r.getBottom(), true); g.setGradientFill (cg); g.fillPath (p);
        g.setColour (edge.withAlpha (0.18f)); g.strokePath (p, juce::PathStrokeType (1.0f));
        return;
    }
    g.setColour (K::lo.withAlpha (light() ? 0.10f : 0.5f)); g.fillRoundedRectangle (r.expanded (1.5f), rad + 1.5f);
    juce::ColourGradient cg (K::disp0, r.getCentreX(), r.getY(), K::disp1, r.getCentreX(), r.getBottom(), false); g.setGradientFill (cg); g.fillRoundedRectangle (r, rad);
    g.setColour (K::hi.withAlpha (light() ? 0.08f : 0.05f)); g.drawRoundedRectangle (r.reduced (0.5f), rad, 1.0f);
}
// pillola del tema nella testata
inline void chipDraw (juce::Graphics& g, juce::Rectangle<float> r, bool hover = false)
{
    if (cb() || hud()) { button (g, r, K::accent, false, hover, label (cur()), jlook::ui (11.0f)); return; }
    chip (g, r, label (cur()), false, K::accent, 8.5f, hover);
}

// ------------------------------------------------------------------------------------------------ uso
inline void apply()
{
    if (cur() < 0) cur() = 3;
    setTheme (cur());
    auto& h = jlook::skinHooks(); h.on = true;
    h.card = panel; h.backdrop = backdrop; h.header = header; h.button = button; h.knob = knob; h.glass = glass; h.bubble = bubble; h.screen = screen;
    jlook::fontHook().fn = fonts;
    auto& p = jlook::skinPal();
    p.bg = K::bg0; p.text = K::ink; p.dim = K::sub; p.faint = K::faint; p.amber = K::amber; p.red = K::red; p.green = K::mint; p.cyan = K::cyan; p.pink = K::pink;
    p.violet = K::violet; p.blue = K::blue; p.lcd0 = K::disp0; p.lcd1 = K::disp1; p.hi = K::hi; p.light = light();
}
// nel costruttore dell'editor: id del plugin (preferenze + emblema), le 4 schede del CBRPNK e il colore acceso dell'HUD
inline void init (const juce::String& id, std::initializer_list<juce::uint32> cardCols, juce::uint32 hudColour = 0xffff2828)
{
    plug() = id; cards().clear(); for (auto c : cardCols) cards().push_back (juce::Colour (c));
    const auto c0 = card0();
    const juce::uint32 pal[10] = { 0xff101010, 0xff1a1a1a, 0xff121212, 0xff3a3733, 0xff6a6762, c0.getARGB(), shade (c0, 0.9f).getARGB(), lift (c0, 0.45f).getARGB(), lift (c0, 0.3f).getARGB(), 0xffe24834 };
    for (int i = 0; i < 10; ++i) K::custom[i] = pal[i];
    const juce::uint32 ac[7] = { 0xffd8402c, 0xff3b2f7a, 0xff121212, 0xff2f5a2a, 0xff8a4b0c, 0xffd8402c, 0xff1f3f7a };
    for (int i = 0; i < 7; ++i) K::customAcc[i] = ac[i];
    K::customLight = true; K::customStyle = 3; K::hudAcc = hudColour;
    if (cur() < 0) cur() = load (id, 3);
    K::theme = -1; apply();
}
inline void cycle() { cur() = next (cur()); save (plug(), cur()); K::theme = -1; apply(); }
}
