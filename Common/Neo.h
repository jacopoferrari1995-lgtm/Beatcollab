#pragma once
// Stile NEO della modalita FACILE: niente scatole. Una scena unica e profonda, luce vera (bloom sfocato), manopole 3D renderizzate,
// tipografia moderna (Manrope + Sora), movimenti morbidi.
#include "JackpotLook.h"
#include "NeoData.h"

namespace neo
{
// tavolozza: i neutri e gli accenti cambiano col tema. 0 = notte (FX), 1 = scuro elegante, 2 = chiaro.
// Ogni editor imposta il suo tema all'inizio del proprio paint (i plugin nella stessa DAW condividono queste variabili).
namespace K
{
    inline juce::Colour bg0 { 0xff04050a }, bg1 { 0xff0a0b16 }, ink { 0xfff2f3f9 }, sub { 0xff9096ad }, faint { 0xff545a70 }, line { 0x14ffffff };
    inline juce::Colour pink { 0xffff3d9a }, violet { 0xff8b5cff }, cyan { 0xff3ee8ff }, mint { 0xff4dffb4 }, amber { 0xffffb547 }, red { 0xffff4d6a }, blue { 0xff4d7dff };
    inline juce::Colour hi { 0xffffffff }, lo { 0xff000000 }, panel0 { 0xff221d2c }, panel1 { 0xff1a1622 }, disp0 { 0xff2b2238 }, disp1 { 0xff1b1724 };
    inline juce::Colour accent { 0xffc9a0f0 };
    inline int theme = 0;
}
inline bool light() { return K::theme == 2; }
// un accento neon reso leggibile sul chiaro (sul tema scuro resta com'e')
inline juce::Colour acc (juce::Colour c) { return light() ? c.withMultipliedSaturation (1.0f).withMultipliedBrightness (0.66f) : K::theme == 1 ? c.withMultipliedSaturation (0.68f).withMultipliedBrightness (0.95f) : c; }
// testo sopra un bottone acceso
inline juce::Colour onText (juce::Colour c) { return light() ? c.darker (0.5f) : juce::Colours::white; }
inline void setTheme (int t)
{
    if (t == K::theme) return;
    K::theme = t; using namespace K;
    const juce::Colour P0 { 0xffff3d9a }, V0 { 0xff8b5cff }, C0 { 0xff3ee8ff }, M0 { 0xff4dffb4 }, A0 { 0xffffb547 }, R0 { 0xffff4d6a }, B0 { 0xff4d7dff };
    if (t == 0)      { bg0 = juce::Colour (0xff04050a); bg1 = juce::Colour (0xff0a0b16); ink = juce::Colour (0xfff2f3f9); sub = juce::Colour (0xff9096ad); faint = juce::Colour (0xff545a70); line = juce::Colour (0x14ffffff); hi = juce::Colours::white; lo = juce::Colours::black; }
    else if (t == 1) { bg0 = juce::Colour (0xff1b1823); bg1 = juce::Colour (0xff272231); ink = juce::Colour (0xfff5f3f9); sub = juce::Colour (0xffaaa5ba); faint = juce::Colour (0xff716c84); line = juce::Colour (0x18ffffff); hi = juce::Colours::white; lo = juce::Colour (0xff07060b);
                       panel0 = juce::Colour (0xff2d2938); panel1 = juce::Colour (0xff23202b); disp0 = juce::Colour (0xff3b2e4b); disp1 = juce::Colour (0xff261f32); }
    else if (t == 3) { bg0 = juce::Colour (0xff06070c); bg1 = juce::Colour (0xff0f1119); ink = juce::Colour (0xffe9edf3); sub = juce::Colour (0xff8c95a9); faint = juce::Colour (0xff4f576b); line = juce::Colour (0x16ffffff); hi = juce::Colour (0xffdfe8ff); lo = juce::Colour (0xff020306);
                       panel0 = juce::Colour (0xff1a1d28); panel1 = juce::Colour (0xff12141d); disp0 = juce::Colour (0xff0b161a); disp1 = juce::Colour (0xff050a0d); }
    else             { bg0 = juce::Colour (0xffd9d7de); bg1 = juce::Colour (0xffeceaef); ink = juce::Colour (0xff1c1a23); sub = juce::Colour (0xff5d596a); faint = juce::Colour (0xff97929f); line = juce::Colour (0x18000000); hi = juce::Colour (0xff15131c); lo = juce::Colour (0xff3a3448);
                       panel0 = juce::Colour (0xfff1f0f3); panel1 = juce::Colour (0xffe2e0e7); disp0 = juce::Colour (0xfff7f2ea); disp1 = juce::Colour (0xffe9e1d5); }
    accent = t == 2 ? juce::Colour (0xff6b45c9) : t == 3 ? juce::Colour (0xffffb84a) : juce::Colour (0xffc7a3ee);
    if (t == 3) { pink = juce::Colour (0xffff3f8e); violet = juce::Colour (0xff9a6bff); cyan = juce::Colour (0xff38e8d8); mint = juce::Colour (0xff5dff9e); amber = juce::Colour (0xffffb84a); red = juce::Colour (0xffff4a5c); blue = juce::Colour (0xff4f8bff); return; }
    pink = acc (P0); violet = acc (V0); cyan = acc (C0); mint = acc (M0); amber = acc (A0); red = acc (R0); blue = acc (B0);
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
inline bool cyber() { return K::theme == 3; }
// nel tema CYBER le scritte sono da strumento: Rajdhani stretto per le etichette, Share Tech Mono per i numeri, Michroma per i titoli
inline juce::Font f (float s, int w = 0) { if (cyber()) return jlook::ui (s * 1.1f, w > 0 ? 1 : 0).withExtraKerningFactor (0.03f); return juce::Font (juce::FontOptions (tf (w > 0 ? 1 : 0)).withHeight (s * 1.18f)).withExtraKerningFactor (w > 0 ? 0.08f : 0.02f); }
inline juce::Font cap (float s) { if (cyber()) return jlook::ui (s * 1.16f, 1).withExtraKerningFactor (0.09f); return juce::Font (juce::FontOptions (tf (1)).withHeight (s * 1.18f)).withExtraKerningFactor (0.16f); }
inline juce::Font big (float s) { if (cyber()) return jlook::logo (s * 0.92f); return juce::Font (juce::FontOptions (tf (2)).withHeight (s * 1.25f)).withExtraKerningFactor (-0.01f); }
inline juce::Font num (float s) { if (cyber()) return jlook::mono (s * 1.08f); return juce::Font (juce::FontOptions (tf (3)).withHeight (s * 1.2f)); }
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

// ------------------------------------------------------------------------------------------------ CYBER: primitive
// sagoma con due angoli tagliati (in alto a sinistra e in basso a destra), gli altri appena arrotondati
inline juce::Path chamfer (juce::Rectangle<float> r, float cut, float rad = 2.5f)
{
    cut = juce::jmin (cut, r.getWidth() * 0.3f, r.getHeight() * 0.45f);
    juce::Path p; const float x0 = r.getX(), y0 = r.getY(), x1 = r.getRight(), y1 = r.getBottom();
    p.startNewSubPath (x0 + cut, y0); p.lineTo (x1 - rad, y0); p.quadraticTo (x1, y0, x1, y0 + rad);
    p.lineTo (x1, y1 - cut); p.lineTo (x1 - cut, y1); p.lineTo (x0 + rad, y1); p.quadraticTo (x0, y1, x0, y1 - rad);
    p.lineTo (x0, y0 + cut); p.closeSubPath(); return p;
}
// staffe d'angolo da mirino (HUD)
inline void brackets (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c, float len = 8.0f, float th = 1.2f)
{
    g.setColour (c);
    for (int k = 0; k < 4; ++k)
    {
        const float x = (k & 1) ? r.getRight() : r.getX(), y = (k & 2) ? r.getBottom() : r.getY(), sx = (k & 1) ? -1.0f : 1.0f, sy = (k & 2) ? -1.0f : 1.0f;
        g.fillRect (juce::Rectangle<float> (sx > 0 ? x : x - len, sy > 0 ? y : y - th, len, th));
        g.fillRect (juce::Rectangle<float> (sx > 0 ? x : x - th, sy > 0 ? y : y - len, th, len));
    }
}
// piastra di metallo: corpo, filo di luce in alto, codice a barre e tacca d'accento sull'angolo tagliato. Da disegnare nello sfondo in cache.
inline void plate (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour tagCol, float cut = 12.0f)
{
    auto p = chamfer (r, cut);
    g.setColour (K::lo.withAlpha (0.6f)); g.fillPath (p, juce::AffineTransform::translation (0, 3));
    juce::ColourGradient cg (K::panel0, 0, r.getY(), K::panel1, 0, r.getBottom(), false); g.setGradientFill (cg); g.fillPath (p);
    // trama a righe orizzontali finissime (metallo spazzolato)
    { g.saveState(); g.reduceClipRegion (p); g.setColour (K::hi.withAlpha (0.012f)); for (float y = r.getY() + 1; y < r.getBottom(); y += 2) g.fillRect (r.getX(), y, r.getWidth(), 1.0f); g.restoreState(); }
    g.setColour (K::hi.withAlpha (0.08f)); g.strokePath (p, juce::PathStrokeType (1.0f));
    g.setColour (K::hi.withAlpha (0.16f)); g.fillRect (r.getX() + cut + 2, r.getY() + 0.5f, juce::jmin (r.getWidth() * 0.4f, 160.0f), 1.0f);
    // tacca d'accento lungo il taglio
    { juce::Path t; t.startNewSubPath (r.getX() + 1.5f, r.getY() + cut + 5); t.lineTo (r.getX() + 1.5f, r.getY() + cut); t.lineTo (r.getX() + cut, r.getY() + 1.5f); t.lineTo (r.getX() + cut + 14, r.getY() + 1.5f);
      g.setColour (tagCol.withAlpha (0.9f)); g.strokePath (t, juce::PathStrokeType (1.6f)); }
    // codice a barre in basso a destra
    { juce::Random rr ((juce::int64) (r.getX() * 31 + r.getY())); float x = r.getRight() - cut - 46; const float y = r.getBottom() - 8;
      for (int i = 0; i < 14 && x < r.getRight() - cut - 4; ++i) { const float w = rr.nextBool() ? 1.0f : 2.0f; g.setColour (K::hi.withAlpha (0.13f)); g.fillRect (x, y, w, 4.0f); x += w + 1.5f; } }
}
// schermo a tubo catodico: vetro scuro, righe di scansione, vignetta e staffe. Da disegnare nello sfondo in cache.
inline void crt (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour edge, float cut = 8.0f)
{
    auto p = chamfer (r, cut, 2.0f);
    g.setColour (K::lo.withAlpha (0.8f)); g.fillPath (chamfer (r.expanded (1.5f), cut + 1, 2.5f));
    juce::ColourGradient cg (K::disp0, r.getCentreX(), r.getY() + r.getHeight() * 0.35f, K::disp1, r.getX(), r.getBottom(), true); g.setGradientFill (cg); g.fillPath (p);
    g.saveState(); g.reduceClipRegion (p);
    g.setColour (juce::Colours::black.withAlpha (0.22f)); for (float y = r.getY(); y < r.getBottom(); y += 3) g.fillRect (r.getX(), y, r.getWidth(), 1.0f);
    juce::ColourGradient sh (edge.withAlpha (0.05f), 0, r.getY(), edge.withAlpha (0.0f), 0, r.getY() + juce::jmin (40.0f, r.getHeight() * 0.3f), false); g.setGradientFill (sh); g.fillRect (r);
    g.restoreState();
    g.setColour (edge.withAlpha (0.16f)); g.strokePath (p, juce::PathStrokeType (1.0f));
    brackets (g, r.reduced (3), edge.withAlpha (0.55f), 7.0f, 1.0f);
}
// tasto rettangolare con un angolo tagliato e LED a sinistra quando e' acceso
inline void key (juce::Graphics& g, juce::Rectangle<float> r, bool on, juce::Colour c, bool hover = false)
{
    auto p = chamfer (r, juce::jmin (6.0f, r.getHeight() * 0.3f), 2.0f);
    if (on)
    {
        g.setColour (c.withAlpha (0.16f)); g.fillPath (p);
        g.setColour (c.withAlpha (0.65f)); g.strokePath (p, juce::PathStrokeType (1.0f));
        g.setColour (c); g.fillRect (r.getX() + 3, r.getCentreY() - juce::jmin (5.0f, r.getHeight() * 0.25f), 2.0f, juce::jmin (10.0f, r.getHeight() * 0.5f));
    }
    else
    {
        g.setColour (K::hi.withAlpha (hover ? 0.07f : 0.035f)); g.fillPath (p);
        g.setColour (K::hi.withAlpha (hover ? 0.2f : 0.1f)); g.strokePath (p, juce::PathStrokeType (1.0f));
    }
}
// anello a LED segmentati per le manopole
inline void ledRing (juce::Graphics& g, float cx, float cy, float R, float v, bool bipolar, juce::Colour c, float th = 2.2f, int n = 27)
{
    const float a0 = -2.356f, a1 = 2.356f, span = (a1 - a0) / n;
    const float vA = a0 + (a1 - a0) * v, from = bipolar ? 0.0f : a0;
    for (int i = 0; i < n; ++i)
    {
        const float s0 = a0 + i * span + span * 0.18f, s1 = a0 + (i + 1) * span - span * 0.18f, mid = 0.5f * (s0 + s1);
        const bool lit = (mid >= juce::jmin (from, vA) && mid <= juce::jmax (from, vA)) || (bipolar && std::abs (mid) < span * 0.5f);
        juce::Path a; a.addCentredArc (cx, cy, R, R, 0, s0, s1, true);
        g.setColour (lit ? c : K::hi.withAlpha (0.08f)); g.strokePath (a, juce::PathStrokeType (th));
    }
}

// fondo CYBER comune: inchiostro, orizzonte al neon appena acceso, pavimento a griglia in prospettiva, testata con logo
inline void cyberBackdrop (juce::Graphics& g, float W, float H, float horizon = 0.56f)
{
    juce::ColourGradient base (K::bg1, 0, 0, K::bg0, 0, H, false); g.setGradientFill (base); g.fillRect (0.0f, 0.0f, W, H);
    const float hy = H * horizon;
    { juce::ColourGradient hz (K::pink.withAlpha (0.0f), 0, hy - 120, K::pink.withAlpha (0.09f), 0, hy, false); hz.addColour (0.8, K::violet.withAlpha (0.05f)); g.setGradientFill (hz); g.fillRect (0.0f, hy - 120, W, 120.0f); }
    g.setColour (K::pink.withAlpha (0.18f)); g.fillRect (0.0f, hy, W, 1.0f);
    for (int i = 1; i < 14; ++i) { const float t = (float) i / 14, y = hy + (H - hy) * t * t; g.setColour (K::violet.withAlpha (0.03f + 0.05f * t)); g.fillRect (0.0f, y, W, 1.0f); }
    for (int i = -16; i <= 16; ++i) { g.setColour (K::violet.withAlpha (0.06f)); g.drawLine (W * 0.5f + i * 22.0f, hy, W * 0.5f + i * 180.0f, H, 1.0f); }
    juce::Random rnd (7); for (int i = 0; i < 70; ++i) { g.setColour (K::hi.withAlpha (0.05f + rnd.nextFloat() * 0.12f)); g.fillRect (rnd.nextFloat() * W, rnd.nextFloat() * (hy - 60), 1.0f, 1.0f); }
}
inline void cyberHeader (juce::Graphics& g, float W, float h, const juce::String& model, const juce::String& sub)
{
    g.setColour (K::lo.withAlpha (0.5f)); g.fillRect (0.0f, 0.0f, W, h);
    g.setColour (K::accent.withAlpha (0.35f)); g.fillRect (0.0f, h - 1, W, 1.0f);
    for (int i = 0; i < 6; ++i) { g.setColour (K::accent.withAlpha (0.7f - i * 0.1f)); g.fillRect (W - 26.0f - i * 7.0f, h - 4, 4.0f, 3.0f); }
    const float cy = h * 0.5f;
    { auto lr = juce::Rectangle<float> (22, cy - 13, 26, 26); g.setColour (K::accent); g.strokePath (chamfer (lr, 7.0f, 1.5f), juce::PathStrokeType (1.6f)); g.setColour (K::pink); g.fillRect (lr.reduced (8)); }
    g.setFont (jlook::logo (15.0f)); g.setColour (K::ink); g.drawText ("JACKPOT", juce::Rectangle<float> (60, cy - 14, 120, 20), juce::Justification::centredLeft);
    g.setColour (K::accent); g.drawText (model, juce::Rectangle<float> (172, cy - 14, 160, 20), juce::Justification::centredLeft);
    g.setFont (jlook::mono (8.5f)); g.setColour (K::faint); g.drawText (sub, juce::Rectangle<float> (60, cy + 7, 360, 12), juce::Justification::centredLeft);
}
// immagine in cache alla risoluzione vera dello schermo: si ridisegna solo se cambia misura, scala o chiave
struct Cache
{
    juce::Image img; float sc = 0; int k = -1; juce::Rectangle<int> sz;
    template <typename F> void draw (juce::Graphics& g, juce::Rectangle<float> area, int key, F&& paintFn)
    {
        const float s = juce::jlimit (1.0f, 4.0f, g.getInternalContext().getPhysicalPixelScaleFactor());
        const auto a = area.getSmallestIntegerContainer();
        if (img.isNull() || std::abs (s - sc) > 0.01f || key != k || a.withZeroOrigin() != sz)
        {
            sc = s; k = key; sz = a.withZeroOrigin();
            img = juce::Image (juce::Image::ARGB, juce::jmax (1, juce::roundToInt (a.getWidth() * s)), juce::jmax (1, juce::roundToInt (a.getHeight() * s)), true);
            juce::Graphics ig (img); ig.addTransform (juce::AffineTransform::translation ((float) -a.getX(), (float) -a.getY()).scaled (s)); paintFn (ig);
        }
        g.saveState(); g.addTransform (juce::AffineTransform::scale (1.0f / sc).translated ((float) a.getX(), (float) a.getY()));
        g.setImageResamplingQuality (juce::Graphics::lowResamplingQuality); g.drawImageAt (img, 0, 0); g.restoreState();
    }
};

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
        g.setColour (K::lo.withAlpha (light() ? 0.07f : 0.35f)); g.fillRoundedRectangle (all.expanded (3), rad + 3);
        g.setColour (K::hi.withAlpha (light() ? 0.08f : 0.05f)); g.drawRoundedRectangle (all.expanded (2.5f), rad + 2.5f, 1.0f);
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
    if (cyber()) { key (g, r, on, c, hover); g.setFont (cap (fs)); g.setColour (on ? K::ink : (hover ? K::ink : K::sub)); g.drawFittedText (t, r.toNearestInt().reduced (on ? 6 : 3, 0), juce::Justification::centred, 1, 0.75f); return; }
    const float rad = r.getHeight() * 0.5f;
    if (on)
    {
        g.setColour (c.withAlpha (0.16f)); g.fillRoundedRectangle (r.expanded (3), rad + 3);
        g.setColour (c.withAlpha (0.22f)); g.fillRoundedRectangle (r, rad);
        g.setColour (c.withAlpha (0.9f)); g.drawRoundedRectangle (r.reduced (0.5f), rad, 1.0f);
    }
    else
    {
        g.setColour (K::hi.withAlpha (hover ? 0.09f : 0.045f)); g.fillRoundedRectangle (r, rad);
        g.setColour (K::hi.withAlpha (hover ? 0.18f : 0.08f)); g.drawRoundedRectangle (r.reduced (0.5f), rad, 1.0f);
    }
    g.setFont (cap (fs)); g.setColour (on ? onText (c) : (hover ? K::ink : K::sub)); g.drawText (t, r, juce::Justification::centred);
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
        if (K::theme != 0) { paintFlat (g); return; }
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
                g.setColour (K::hi.withAlpha (i % 5 == 0 ? 0.22f : 0.1f));
                g.drawLine (cx + std::sin (a) * r0, cy - std::cos (a) * r0, cx + std::sin (a) * r1, cy - std::cos (a) * r1, 1.0f);
            }
        // binario
        { juce::Path t; t.addCentredArc (cx, cy, R, R, 0, a0, a1, true); g.setColour (K::hi.withAlpha (light() ? 0.1f : 0.07f)); g.strokePath (t, juce::PathStrokeType (tw, juce::PathStrokeType::curved, juce::PathStrokeType::rounded)); }
        // arco di valore con alone vero (in cache finche' il valore non cambia)
        juce::Path arc; const bool hasArc = std::abs (av - from) > 0.01f;
        if (hasArc) arc.addCentredArc (cx, cy, R, R, 0, juce::jmin (from, av), juce::jmax (from, av), true);
        const juce::Colour c1 = acc (this->c1), c2 = acc (this->c2);
        juce::ColourGradient cg (c1, cx - R, cy + R, c2, cx + R, cy - R, false);
        if (std::abs (bloomV - v) > 0.002f || bloomS != s || bloomT != K::theme)
        {
            bloomV = v; bloomS = s; bloomT = K::theme;
            bloom.make (getLocalBounds().toFloat().translated ((float) -pad, (float) -pad), big ? 3.0f : 2.0f, big ? 5 : 3, [&] (juce::Graphics& bg)
            {
                if (hasArc) { bg.setGradientFill (cg); bg.strokePath (arc, juce::PathStrokeType (tw * 3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded)); }
                const auto pc = c1.interpolatedWith (c2, v); const float pr = kr * 0.62f;
                bg.setColour (pc); bg.fillEllipse (cx + std::sin (av) * pr - 5, cy - std::cos (av) * pr - 5, 10, 10);
            });
        }
        bloom.draw (g, light() ? (hov ? 0.55f : 0.4f) : (hov ? 1.0f : 0.85f));
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
        if (light())
        {   // sul chiaro l'ombra del render ha i bordi: la taglio a cerchio e la sfumo
            juce::Graphics::ScopedSaveState ss (g); juce::Path cl; cl.addEllipse (cx - kr * 1.16f, cy - kr * 1.12f, kr * 2.32f, kr * 2.4f); g.reduceClipRegion (cl);
            g.drawImage (knobBody(), { cx - is * 0.5f, cy - is * 0.5f, is, is }, juce::RectanglePlacement::stretchToFit);
        }
        else g.drawImage (knobBody(), { cx - is * 0.5f, cy - is * 0.5f, is, is }, juce::RectanglePlacement::stretchToFit);
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
            g.setFont (big_ (40.0f)); g.setColour (juce::Colours::white); g.drawText (t, juce::Rectangle<float> (cx - kr, cy - 30, kr * 2, 46), juce::Justification::centred);   // sul cappello scuro: sempre chiaro
            g.setFont (cap (10.0f)); g.setColour (juce::Colours::white.withAlpha (0.6f)); g.drawText (label, juce::Rectangle<float> (cx - kr, cy + 16, kr * 2, 16), juce::Justification::centred);
            if (sub.isNotEmpty()) { g.setFont (f (9.5f)); g.setColour (K::faint); g.drawText (sub, juce::Rectangle<float> (0, (float) getHeight() - 2 * pad - 16, W, 14), juce::Justification::centred); }
        }
        else
        {
            g.setFont (cap (9.0f)); g.setColour (hov ? K::ink : K::sub); g.drawText (label, juce::Rectangle<float> (-20, s + 3, W + 40, 14), juce::Justification::centred);
            g.setFont (num (11.0f)); g.setColour (hov ? c1.interpolatedWith (c2, v).brighter (0.5f) : K::ink.withAlpha (0.9f)); g.drawText (t, juce::Rectangle<float> (-20, s + 18, W + 40, 16), juce::Justification::centred);
        }
    }
    // manopola sobria (temi del Master): disco opaco, anello sottile, arco di un colore solo, tacca bianca. Niente bagliori.
    void paintFlat (juce::Graphics& g)
    {
        g.addTransform (juce::AffineTransform::translation ((float) pad, (float) pad));
        const float W = (float) getWidth() - 2 * pad, s = S(), cx = W * 0.5f, cy = s * 0.5f;
        const float R = s * 0.5f - (big ? 16.0f : 4.0f), tw = big ? 3.0f : 2.2f, kr = R * (big ? 0.8f : 0.74f);
        const float a0 = -2.356f, a1 = 2.356f, v = shownV < 0 ? param->getValue() : shownV, av = a0 + (a1 - a0) * v, from = bipolar ? 0.0f : a0;
        const bool hov = isMouseOverOrDragging();
        if (big)
            for (int i = 0; i <= 40; ++i)
            {
                const float a = a0 + (a1 - a0) * i / 40.0f, r0 = R + 6, r1 = R + (i % 5 == 0 ? 11.0f : 9.0f);
                g.setColour (K::hi.withAlpha (i % 5 == 0 ? 0.16f : 0.07f));
                g.drawLine (cx + std::sin (a) * r0, cy - std::cos (a) * r0, cx + std::sin (a) * r1, cy - std::cos (a) * r1, 1.0f);
            }
        { juce::Path t; t.addCentredArc (cx, cy, R, R, 0, a0, a1, true); g.setColour (K::hi.withAlpha (light() ? 0.12f : 0.10f)); g.strokePath (t, juce::PathStrokeType (tw, juce::PathStrokeType::curved, juce::PathStrokeType::rounded)); }
        if (std::abs (av - from) > 0.01f)
        {
            juce::Path arc; arc.addCentredArc (cx, cy, R, R, 0, juce::jmin (from, av), juce::jmax (from, av), true);
            g.setColour (hov ? K::accent.brighter (0.15f) : K::accent); g.strokePath (arc, juce::PathStrokeType (tw, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        if (activity > 0.01f)
        {
            const float ro = R + 14; juce::Path sg; sg.addCentredArc (cx, cy, ro, ro, 0, a0, a0 + (a1 - a0) * juce::jlimit (0.0f, 1.0f, activity), true);
            g.setColour ((activity > 0.75f ? K::red : K::accent).withAlpha (0.8f)); g.strokePath (sg, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        // disco: ombra corta e morbida, superficie opaca appena sfumata
        g.setColour (juce::Colours::black.withAlpha (light() ? 0.16f : 0.30f)); g.fillEllipse (cx - kr, cy - kr + (big ? 4.0f : 2.0f), kr * 2, kr * 2);
        const juce::Colour top = light() ? juce::Colour (0xff2c2a33) : juce::Colour (0xff35323d), bot = light() ? juce::Colour (0xff17161b) : juce::Colour (0xff1c1a21);
        juce::ColourGradient bg (top, cx, cy - kr, bot, cx, cy + kr, false); g.setGradientFill (bg); g.fillEllipse (cx - kr, cy - kr, kr * 2, kr * 2);
        g.setColour (juce::Colours::white.withAlpha (hov ? 0.12f : 0.07f)); g.drawEllipse (cx - kr + 0.5f, cy - kr + 0.5f, kr * 2 - 1, kr * 2 - 1, 1.0f);
        {
            const float r0 = kr * (big ? 0.55f : 0.35f), r1 = kr * 0.86f, sn = std::sin (av), cs = std::cos (av);
            juce::Path pl; pl.startNewSubPath (cx + sn * r0, cy - cs * r0); pl.lineTo (cx + sn * r1, cy - cs * r1);
            g.setColour (juce::Colours::white.withAlpha (0.92f)); g.strokePath (pl, juce::PathStrokeType (big ? 2.4f : 1.7f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        const juce::String t = text ? text() : param->getCurrentValueAsText();
        if (big)
        {
            g.setFont (big_ (38.0f)); g.setColour (juce::Colours::white); g.drawText (t, juce::Rectangle<float> (cx - kr, cy - 28, kr * 2, 44), juce::Justification::centred);
            g.setFont (cap (9.5f)); g.setColour (juce::Colours::white.withAlpha (0.55f)); g.drawText (label, juce::Rectangle<float> (cx - kr, cy + 16, kr * 2, 16), juce::Justification::centred);
            if (sub.isNotEmpty()) { g.setFont (f (9.0f)); g.setColour (K::faint); g.drawText (sub, juce::Rectangle<float> (0, (float) getHeight() - 2 * pad - 16, W, 14), juce::Justification::centred); }
        }
        else
        {
            g.setFont (cap (8.5f)); g.setColour (hov ? K::ink : K::sub); g.drawText (label, juce::Rectangle<float> (-20, s + 3, W + 40, 14), juce::Justification::centred);
            g.setFont (num (10.5f)); g.setColour (K::ink.withAlpha (0.9f)); g.drawText (t, juce::Rectangle<float> (-20, s + 18, W + 40, 16), juce::Justification::centred);
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
    float start = 0, shownV = -1, shownAct = 0, bloomV = -1, bloomS = 0; int bloomT = -1; bool drag = false; Bloom bloom;
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
        const auto col = acc (this->col);
        g.setColour (on ? col.withAlpha (0.3f) : K::lo.withAlpha (light() ? 0.12f : 0.4f)); g.fillRoundedRectangle (sw, 9);
        g.setColour (on ? col.withAlpha (0.8f) : K::hi.withAlpha (0.12f)); g.drawRoundedRectangle (sw.reduced (0.5f), 9, 1.0f);
        const float x = on ? sw.getRight() - 16 : sw.getX() + 2;
        if (on) { g.setColour (col.withAlpha (0.25f)); g.fillEllipse (x - 5, sw.getY() - 3, 24, 24); }
        juce::ColourGradient kg (juce::Colours::white, x, sw.getY() + 2, juce::Colour (0xffb9bccb), x, sw.getBottom() - 2, false); g.setGradientFill (kg); g.fillEllipse (x, sw.getY() + 2, 14, 14);
        g.setFont (cap (9.0f)); g.setColour (on ? K::ink : K::sub); g.drawText (label, b.withTrimmedLeft (10), juce::Justification::centredLeft);
    }
    void mouseDown (const juce::MouseEvent&) override { param->setValueNotifyingHost (param->getValue() > 0.5f ? 0.0f : 1.0f); repaint(); }
    juce::RangedAudioParameter* param; juce::String label; juce::Colour col;
};
}
