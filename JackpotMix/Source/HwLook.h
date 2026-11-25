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

// manopole vettoriali: nitide a qualsiasi zoom. Famiglie: bakelite, chicken head, Davies 1900 (1176), avorio,
// Marconi Neve (gonna grigia e cappuccio colorato), SSL (gonna nera e cappuccio colorato), alluminio tornito, pedale
inline void vknob (juce::Graphics& g, float cx, float cy, float rs, float av, int style, bool enabled = true)
{
    using namespace juce;
    auto P = [cx, cy] (float a, float r) { return Point<float> (cx + std::sin (a) * r, cy - std::cos (a) * r); };
    auto disc = [cx, cy] (float r, float dy) { return Rectangle<float> (cx - r, cy - r + dy, 2 * r, 2 * r); };
    const float depth = rs * 0.15f;
    // ombra morbida sul pannello (luce dall'alto a sinistra)
    for (int i = 6; i >= 1; --i) { const float e = i * rs * 0.065f; g.setColour (Colours::black.withAlpha (0.075f)); g.fillEllipse (cx - rs - e * 0.6f + depth * 0.6f, cy - rs + depth * 1.5f - e * 0.4f, 2 * rs + e * 1.2f, 2 * rs + e * 1.2f); }
    auto knurl = [&] (float r0, float r1, int n, float a1, float a2)
    { for (int i = 0; i < n; ++i) { const float a = av + i * MathConstants<float>::twoPi / n; g.setColour (Colours::white.withAlpha (a1)); g.drawLine (Line<float> (P (a, r0), P (a, r1)), 1.0f); g.setColour (Colours::black.withAlpha (a2)); g.drawLine (Line<float> (P (a + 0.6f / n, r0), P (a + 0.6f / n, r1)), 0.9f); } };
    auto gloss = [&] (float r, float amt)
    { ColourGradient hl (Colours::white.withAlpha (amt), cx - r * 0.25f, cy - r * 0.85f, Colours::white.withAlpha (0.0f), cx, cy - r * 0.05f, false); g.setGradientFill (hl); g.fillEllipse (cx - r * 0.78f, cy - r * 0.96f, r * 1.45f, r * 0.95f); };
    auto bakelite = [&] (Colour hi, Colour lo, float r, float dy)
    {
        { ColourGradient sd (hi.darker (0.6f), cx - r, cy, lo, cx + r, cy, false); g.setGradientFill (sd); g.fillEllipse (disc (r, dy)); }
        { ColourGradient sk (hi, cx - r * 0.45f, cy - r, lo, cx + r * 0.45f, cy + r, false); g.setGradientFill (sk); g.fillEllipse (disc (r, 0)); }
    };
    auto line = [&] (Colour c, float r0, float r1, float w) { g.setColour (Colours::black.withAlpha (0.55f)); g.drawLine (Line<float> (P (av, r0), P (av, r1)).withShortenedStart (0).withShortenedEnd (0), w + 1.6f); g.setColour (c); g.drawLine (Line<float> (P (av, r0), P (av, r1)), w); };
    auto capCol = [] (int st) -> Colour
    {
        switch (st)
        {
            case KS_NEVE_BLUE: return Colour (0xff3f6fb0); case KS_NEVE_RED: return Colour (0xffb8372c); case KS_NEVE_GREY: return Colour (0xff8c9096);
            case KS_SSL_BLUE: return Colour (0xff2f6fd6); case KS_SSL_GREEN: return Colour (0xff2f9a52); case KS_SSL_RED: return Colour (0xffd23a2e);
            case KS_SSL_WHITE: return Colour (0xffe9e6de); case KS_SSL_YELLOW: return Colour (0xffe2b52a); default: return Colour (0xff888888);
        }
    };
    switch (style)
    {
        case KS_SILVER:
        {   // alluminio tornito: fianco zigrinato, anelli concentrici, riflesso conico
            { ColourGradient sd (Colour (0xff9a9ea5), cx - rs, cy, Colour (0xff3d4045), cx + rs, cy, false); sd.addColour (0.35, Colour (0xffd8dbe0)); g.setGradientFill (sd); g.fillEllipse (disc (rs, depth)); }
            { ColourGradient sk (Colour (0xfff2f3f5), cx - rs * 0.6f, cy - rs, Colour (0xff70747a), cx + rs * 0.5f, cy + rs, false); g.setGradientFill (sk); g.fillEllipse (disc (rs, 0)); }
            knurl (rs - 0.3f, rs - 3.0f, 48, 0.0f, 0.2f);
            const float rc = rs * 0.7f;
            g.setColour (Colours::black.withAlpha (0.35f)); g.drawEllipse (disc (rc + 1.2f, 0), 1.2f);
            { ColourGradient cg (Colour (0xffe9ebee), cx, cy - rc, Colour (0xffa2a6ad), cx, cy + rc, false); g.setGradientFill (cg); g.fillEllipse (disc (rc, 0)); }
            for (float r2 = rc - 1.5f; r2 > 2; r2 -= 1.5f) { g.setColour ((((int) (r2 * 3)) & 1 ? Colours::white : Colours::black).withAlpha (0.06f)); g.drawEllipse (disc (r2, 0), 0.7f); }
            for (float base : { -0.9f, 2.24f }) { Path sp; sp.addPieSegment (disc (rc, 0), base - 0.35f, base + 0.35f, 0.0f); ColourGradient hg (Colours::white.withAlpha (0.55f), cx, cy, Colours::white.withAlpha (0.0f), P (base, rc).x, P (base, rc).y, false); g.setGradientFill (hg); g.fillPath (sp); }
            line (Colour (0xff15161a), rc * 0.3f, rs - 1.5f, 2.2f);
            break;
        }
        case KS_CHICKEN:
        {   // chicken head (Pultec, LA-2A): base tonda piccola e becco lungo
            bakelite (Colour (0xff3a3b40), Colour (0xff050506), rs * 0.62f, depth);
            Path beak; const float L = rs * 1.02f, Wd = rs * 0.36f;
            beak.startNewSubPath (0, -L); beak.quadraticTo (Wd, -L * 0.45f, Wd * 1.05f, 0); beak.quadraticTo (Wd * 0.9f, L * 0.55f, 0, L * 0.62f);
            beak.quadraticTo (-Wd * 0.9f, L * 0.55f, -Wd * 1.05f, 0); beak.quadraticTo (-Wd, -L * 0.45f, 0, -L); beak.closeSubPath();
            const auto tr = AffineTransform::rotation (av).translated (cx, cy);
            g.setColour (Colours::black.withAlpha (0.5f)); g.fillPath (beak, tr.translated (depth * 0.5f, depth * 1.2f));
            { ColourGradient bg (Colour (0xff3e3f44), cx - rs * 0.5f, cy - rs, Colour (0xff060607), cx + rs * 0.5f, cy + rs, false); g.setGradientFill (bg); g.fillPath (beak, tr); }
            g.setColour (Colours::white.withAlpha (0.12f)); g.strokePath (beak, PathStrokeType (0.9f), tr);
            gloss (rs * 0.55f, 0.25f);
            line (Colour (0xfff3eee2).withAlpha (enabled ? 1.0f : 0.4f), rs * 0.1f, rs * 0.95f, 1.8f);
            break;
        }
        case KS_DAVIES: case KS_FET:
        {   // Davies 1900: gonna nera larga con scanalature, corpo alto, inserto in alluminio
            bakelite (Colour (0xff2c2d31), Colour (0xff030304), rs, depth);
            knurl (rs - 0.5f, rs * 0.78f, 36, 0.09f, 0.3f);
            const float rc = rs * 0.66f;
            bakelite (Colour (0xff45464c), Colour (0xff08080a), rc, depth * 0.8f);
            knurl (rc - 0.5f, rc - 3.0f, 28, 0.12f, 0.25f);
            const float ri = rc * 0.55f;
            { ColourGradient ag (Colour (0xffe3e5e8), cx - ri, cy - ri, Colour (0xff7d8187), cx + ri, cy + ri, false); g.setGradientFill (ag); g.fillEllipse (disc (ri, 0)); }
            for (float base : { -0.9f, 2.24f }) { Path sp; sp.addPieSegment (disc (ri, 0), base - 0.4f, base + 0.4f, 0.0f); g.setColour (Colours::white.withAlpha (0.25f)); g.fillPath (sp); }
            gloss (rc, 0.18f);
            line (Colour (0xfff5f1e6), ri + 1.5f, rs - 1.5f, 2.0f);
            break;
        }
        case KS_IVORY:
        {   // bakelite color avorio, gonna con scala
            bakelite (Colour (0xfff1e9d6), Colour (0xff8e8473), rs, depth);
            knurl (rs - 0.5f, rs - 3.5f, 40, 0.15f, 0.12f);
            const float rc = rs * 0.68f;
            { ColourGradient cs (Colour (0xffcfc5ad), cx, cy - rc, Colour (0xff7a705e), cx, cy + rc, false); g.setGradientFill (cs); g.fillEllipse (disc (rc + 1.2f, 0.8f)); }
            { ColourGradient cg (Colour (0xfffbf6ea), cx - rc * 0.5f, cy - rc, Colour (0xffb9ae95), cx + rc * 0.5f, cy + rc, true); g.setGradientFill (cg); g.fillEllipse (disc (rc, 0)); }
            gloss (rc, 0.5f);
            line (Colour (0xff1d1a16), rc * 0.15f, rs - 1.5f, 2.2f);
            break;
        }
        case KS_NEVE_BLUE: case KS_NEVE_RED: case KS_NEVE_GREY:
        {   // Marconi (Neve 1073): gonna grigia scanalata a cono, cappuccio colorato piccolo
            { ColourGradient sd (Colour (0xff6e7177), cx - rs, cy, Colour (0xff25272b), cx + rs, cy, false); g.setGradientFill (sd); g.fillEllipse (disc (rs, depth)); }
            { ColourGradient sk (Colour (0xffb3b6bb), cx - rs * 0.5f, cy - rs, Colour (0xff3b3d42), cx + rs * 0.5f, cy + rs, false); g.setGradientFill (sk); g.fillEllipse (disc (rs, 0)); }
            for (int i = 0; i < 32; ++i) { const float a = av + i * MathConstants<float>::twoPi / 32; g.setColour (Colours::black.withAlpha (0.22f)); g.drawLine (Line<float> (P (a, rs - 0.5f), P (a, rs * 0.62f)), 1.0f); g.setColour (Colours::white.withAlpha (0.12f)); g.drawLine (Line<float> (P (a + 0.05f, rs - 0.5f), P (a + 0.05f, rs * 0.62f)), 0.8f); }
            const float rc = rs * 0.5f; const auto cc = enabled ? capCol (style) : Colour (0xff55585e);
            { ColourGradient cs (cc.darker (0.9f), cx, cy - rc, cc.darker (1.8f), cx, cy + rc, false); g.setGradientFill (cs); g.fillEllipse (disc (rc + 1.5f, depth * 0.6f)); }
            { ColourGradient cg (cc.brighter (0.45f), cx - rc * 0.5f, cy - rc, cc.darker (0.5f), cx + rc * 0.4f, cy + rc, true); g.setGradientFill (cg); g.fillEllipse (disc (rc, 0)); }
            gloss (rc, 0.55f);
            line (Colour (0xfff4f0e6), rs * 0.18f, rs - 1.0f, 1.9f);
            break;
        }
        case KS_PEDAL:
        {   // pedale: manopolina nera a cappello piatto con tacca bianca
            bakelite (Colour (0xff36373b), Colour (0xff040405), rs, depth);
            knurl (rs - 0.4f, rs - 4.0f, 30, 0.1f, 0.3f);
            const float rc = rs * 0.8f;
            { ColourGradient cg (Colour (0xff2c2d31), cx, cy - rc, Colour (0xff0d0d0f), cx, cy + rc, false); g.setGradientFill (cg); g.fillEllipse (disc (rc, 0)); }
            gloss (rc, 0.2f);
            line (Colours::white, rc * 0.25f, rs - 2.0f, 2.2f);
            break;
        }
        case KS_SSL_BLUE: case KS_SSL_GREEN: case KS_SSL_RED: case KS_SSL_WHITE: case KS_SSL_YELLOW:
        {   // SSL: corpo nero con alette, cappuccio colorato lucido e tacca bianca
            { ColourGradient sd (Colour (0xff2a2c31), cx - rs, cy, Colour (0xff0b0c0e), cx + rs, cy, false); g.setGradientFill (sd); g.fillEllipse (disc (rs, depth)); }
            { ColourGradient sk (Colour (0xff50545b), cx, cy - rs, Colour (0xff15161a), cx, cy + rs, false); g.setGradientFill (sk); g.fillEllipse (disc (rs, 0)); }
            knurl (rs - 0.5f, rs * 0.7f, 24, 0.08f, 0.25f);
            const float rc = rs * 0.62f; const auto cc = enabled ? capCol (style) : Colour (0xff55585e);
            { ColourGradient cs (cc.darker (0.8f), cx - rc, cy, cc.darker (1.6f), cx + rc, cy, false); g.setGradientFill (cs); g.fillEllipse (disc (rc, depth * 0.7f)); }
            { ColourGradient cg (cc.brighter (0.5f), cx - rc * 0.5f, cy - rc, cc.darker (0.45f), cx + rc * 0.4f, cy + rc, true); g.setGradientFill (cg); g.fillEllipse (disc (rc, 0)); }
            gloss (rc, 0.55f);
            line (style == KS_SSL_WHITE ? Colour (0xff1a1a1a) : Colours::white, rc * 0.2f, rs - 1.5f, 2.0f);
            break;
        }
        default:
        {   // bakelite nera grande zigrinata (anni 60) con punta crema
            bakelite (Colour (0xff3f4045), Colour (0xff060607), rs, depth);
            knurl (rs - 0.5f, rs - 4.0f, 40, 0.1f, 0.0f);
            const float rc = rs * 0.64f;
            { ColourGradient cs (Colour (0xff1b1c1f), cx, cy - rc, Colour (0xff000000), cx, cy + rc, false); g.setGradientFill (cs); g.fillEllipse (disc (rc + 1.5f, 1.0f)); }
            { ColourGradient cg (Colour (0xff35363b), cx - rc * 0.5f, cy - rc, Colour (0xff040405), cx + rc * 0.5f, cy + rc, true); g.setGradientFill (cg); g.fillEllipse (disc (rc, 0)); }
            gloss (rc, 0.28f);
            line (Colour (0xfff3eee2).withAlpha (enabled ? 1.0f : 0.4f), rc * 0.15f, rs - 1.5f, 2.4f);
            break;
        }
    }
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
// materia: vernice non uniforme (macchie larghe), grana fine, graffietti, sporco verso i bordi. Disegnata una volta (il frontale e' in cache)
inline void grit (juce::Graphics& g, juce::Rectangle<float> r, int seed, float amt, bool metal)
{
    const float sc = juce::jlimit (1.0f, 4.0f, g.getInternalContext().getPhysicalPixelScaleFactor());
    const int W = juce::jmax (1, juce::roundToInt (r.getWidth() * sc)), H = juce::jmax (1, juce::roundToInt (r.getHeight() * sc));
    juce::Image im (juce::Image::ARGB, W, H, true);
    {
        juce::Image::BitmapData bd (im, juce::Image::BitmapData::writeOnly);
        juce::Random rnd (seed * 7919 + 13);
        // rumore a valori su griglia larga (macchie della vernice) e media
        auto grid = [&] (int cell, std::vector<float>& v, int& gw, int& gh) { gw = W / cell + 2; gh = H / cell + 2; v.resize ((size_t) (gw * gh)); for (auto& x : v) x = rnd.nextFloat() * 2 - 1; };
        std::vector<float> g1, g2; int w1, h1, w2, h2; const int c1 = juce::jmax (8, (int) (46 * sc)), c2 = juce::jmax (3, (int) (9 * sc));
        grid (c1, g1, w1, h1); grid (c2, g2, w2, h2);
        auto smp = [] (const std::vector<float>& v, int gw, int cell, int x, int y)
        { const float fx = x / (float) cell, fy = y / (float) cell; const int ix = (int) fx, iy = (int) fy; const float tx = fx - ix, ty = fy - iy;
          const float sx = tx * tx * (3 - 2 * tx), sy = ty * ty * (3 - 2 * ty);
          auto at = [&] (int a, int b) { return v[(size_t) (b * gw + a)]; };
          return (at (ix, iy) * (1 - sx) + at (ix + 1, iy) * sx) * (1 - sy) + (at (ix, iy + 1) * (1 - sx) + at (ix + 1, iy + 1) * sx) * sy; };
        for (int y = 0; y < H; ++y)
        {
            const float ey = juce::jmin (y, H - 1 - y) / (float) H;
            for (int x = 0; x < W; ++x)
            {
                const float ex = juce::jmin (x, W - 1 - x) / (float) W, edge = juce::jmax (0.0f, 1.0f - juce::jmin (ex * 9.0f, ey * 16.0f));
                float n = smp (g1, w1, c1, x, y) * 0.85f + smp (g2, w2, c2, x, y) * 0.35f + (rnd.nextFloat() * 2 - 1) * (metal ? 0.22f : 0.38f);
                n -= edge * edge * 0.9f;   // sporco e usura verso i bordi
                const float a = juce::jlimit (0.0f, 1.0f, std::abs (n) * amt);
                const juce::uint8 c = n > 0 ? 255 : 0;
                bd.setPixelColour (x, y, juce::Colour (c, c, c, a));
            }
        }
    }
    g.drawImage (im, r, juce::RectanglePlacement::stretchToFit);
    // graffietti sottili
    juce::Random rs (seed * 31 + 7);
    for (int i = 0; i < 24; ++i)
    {
        const float x = r.getX() + rs.nextFloat() * r.getWidth(), y = r.getY() + rs.nextFloat() * r.getHeight(), L = 6 + rs.nextFloat() * 26, a = -0.6f + rs.nextFloat() * 1.2f;
        g.setColour (juce::Colours::white.withAlpha (0.05f + rs.nextFloat() * 0.06f)); g.drawLine (x, y, x + std::cos (a) * L, y + std::sin (a) * L, 0.6f);
    }
}
// orecchie del 500-series: linguetta metallica con asola e vite
inline void ear (juce::Graphics& g, juce::Rectangle<float> r, float rot)
{
    juce::ColourGradient eg (juce::Colour (0xff6c6a66), r.getX(), r.getY(), juce::Colour (0xff2a2927), r.getX(), r.getBottom(), false); g.setGradientFill (eg); g.fillRoundedRectangle (r, 2);
    g.setColour (juce::Colours::white.withAlpha (0.18f)); g.drawHorizontalLine ((int) r.getY(), r.getX() + 2, r.getRight() - 2);
    g.setColour (juce::Colours::black.withAlpha (0.55f)); g.drawRoundedRectangle (r, 2, 0.8f);
    auto slot = r.withSizeKeepingCentre (juce::jmin (r.getWidth() - 10, 34.0f), r.getHeight() - 5);
    g.setColour (juce::Colour (0xff0a0a09)); g.fillRoundedRectangle (slot, slot.getHeight() * 0.5f);
    g.setColour (juce::Colours::white.withAlpha (0.12f)); g.drawRoundedRectangle (slot.translated (0, 0.8f), slot.getHeight() * 0.5f, 0.6f);
    screw (g, slot.getCentreX() + std::sin (rot * 3.1f) * slot.getWidth() * 0.22f, slot.getCentreY(), slot.getHeight() * 0.5f - 0.3f, rot);
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
    if (face != FACE_WOOD) grit (g, r, seed, face == FACE_CREAM ? 0.34f : face == FACE_BRUSHED ? 0.24f : face == FACE_BLACK ? 0.30f : 0.34f, face == FACE_BRUSHED);
    {   // ombra interna: il pannello e' spesso e un po' incassato
        juce::ColourGradient vg (juce::Colours::transparentBlack, r.getCentreX(), r.getCentreY(), juce::Colours::black.withAlpha (0.22f), r.getX(), r.getY(), true); g.setGradientFill (vg); g.fillRect (r);
    }
    g.restoreState();
    // smusso: luce sul bordo alto e sinistro, ombra sotto
    g.setColour (juce::Colours::white.withAlpha (face == FACE_BLACK ? 0.12f : 0.3f)); g.drawLine (r.getX() + rad, r.getY() + 0.8f, r.getRight() - rad, r.getY() + 0.8f, 1.2f);
    g.drawLine (r.getX() + 0.8f, r.getY() + rad, r.getX() + 0.8f, r.getBottom() - rad, 1.0f);
    g.setColour (juce::Colours::black.withAlpha (0.35f)); g.drawLine (r.getRight() - 0.8f, r.getY() + rad, r.getRight() - 0.8f, r.getBottom() - rad, 1.4f); g.drawLine (r.getX() + rad, r.getBottom() - 0.8f, r.getRight() - rad, r.getBottom() - 0.8f, 1.6f);
    g.setColour (juce::Colours::black.withAlpha (0.7f)); g.drawRoundedRectangle (r, rad, 1.0f);
    if (face != FACE_PEDAL)
    {
        ear (g, juce::Rectangle<float> (r.getCentreX() - 26, r.getY() + 2, 52, 14), 0.3f + (seed % 7) * 0.4f);
        ear (g, juce::Rectangle<float> (r.getCentreX() - 26, r.getBottom() - 16, 52, 14), 1.1f + (seed % 5) * 0.5f);
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
        const float rk = (R - 7.0f) * frameMul;
        {   // arco del valore sulla scala: sottile, piu' acceso quando la tocchi
            juce::Path arc; const float va = bipolar ? 0.0f : a0, ve = a0 + (a1 - a0) * v;
            arc.addCentredArc (cx, cy, R + 2.5f, R + 2.5f, 0, juce::jmin (va, ve), juce::jmax (va, ve), true);
            const bool on = isMouseOverOrDragging();
            g.setColour (hot.withAlpha (on ? 0.35f : 0.18f)); g.strokePath (arc, juce::PathStrokeType (on ? 5.0f : 4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setColour (hot.withAlpha (on ? 1.0f : 0.75f)); g.strokePath (arc, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        vknob (g, cx, cy, rk, a0 + (a1 - a0) * v, style, isEnabled());
        // lettura in un piccolo display, come sui moduli di Butter e Studio Comp
        const juce::String t = text ? text() : param->getCurrentValueAsText();
        g.setFont (alook::typed (valueSize + 0.5f));
        const float tw = juce::jlimit (34.0f, (float) getWidth() + 10.0f, juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), t) + 14.0f);
        auto rr = juce::Rectangle<float> (cx - tw * 0.5f, (float) getHeight() - vh, tw, vh - 1);
        g.setColour (juce::Colour (0xf0060708)); g.fillRoundedRectangle (rr, 3); g.setColour (hot.withAlpha (isMouseOverOrDragging() ? 0.8f : 0.35f)); g.drawRoundedRectangle (rr.reduced (0.5f), 3, 1.0f);
        const auto vc = hot.getPerceivedBrightness() < 0.5f ? hot.interpolatedWith (juce::Colours::white, 0.5f) : hot;
        g.setColour (isMouseOverOrDragging() ? vc.brighter (0.3f) : vc); g.drawText (t, rr, juce::Justification::centred);
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
        float nv = start - e.getDistanceFromDragStartY() / (e.mods.isShiftDown() ? 1200.0f : 300.0f);
        if (stepsN > 1) nv = std::round (nv * (stepsN - 1)) / (stepsN - 1);
        param->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, nv)); repaint();
    }
    void mouseDoubleClick (const juce::MouseEvent&) override { param->setValueNotifyingHost (param->getDefaultValue()); }
    void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override
    {
        // scorrimento orizzontale (trackpad, shift+rotella): passa al rack, la manopola non si muove
        if (std::abs (w.deltaX) > std::abs (w.deltaY) * 1.2f || e.mods.isShiftDown()) { juce::Component::mouseWheelMove (e, w); return; }
        const float dy = w.isReversed ? -w.deltaY : w.deltaY;
        if (stepsN > 1) { wheelAcc += dy; if (std::abs (wheelAcc) < 0.12f) return; const float st = 1.0f / (stepsN - 1); param->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, param->getValue() + (wheelAcc > 0 ? st : -st))); wheelAcc = 0; }
        else param->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, param->getValue() + juce::jlimit (-0.04f, 0.04f, dy * 0.06f)));
        repaint();
    }
    float wheelAcc = 0;
    juce::RangedAudioParameter* param; juce::String label; int style;
    std::function<juce::String()> text;
    juce::Colour inkC { 0xff1e1b17 }, hot { 0xffd8452f };
    float labelH = 15, valueH = 16, labelSize = 9.5f, valueSize = 9.0f, frameMul = 1.0f;
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
        silk (g, upT, { 0, 13, w, 11 }, alook::serif (8.0f), inkC.withAlpha (isUp() ? 1.0f : 0.5f));
        silk (g, dnT, { 0, h - 11, w, 11 }, alook::serif (8.0f), inkC.withAlpha (isUp() ? 0.5f : 1.0f));
        // deviatore a levetta: dado esagonale cromato e levetta a bastoncino che punta su o giu'
        using namespace juce;
        const float cx = w * 0.5f, cy = 13 + 11 + (h - 13 - 22) * 0.5f, r = juce::jmin (w * 0.3f, (h - 46) * 0.28f);
        { Path hex; for (int i = 0; i < 6; ++i) { const float a = i * MathConstants<float>::pi / 3 + 0.52f; const auto pt = Point<float> (cx + std::cos (a) * r * 1.35f, cy + std::sin (a) * r * 1.35f); if (i == 0) hex.startNewSubPath (pt); else hex.lineTo (pt); } hex.closeSubPath();
          g.setColour (Colours::black.withAlpha (0.4f)); g.fillPath (hex, AffineTransform::translation (1.0f, 2.0f));
          ColourGradient hg (Colour (0xffeceef1), cx - r, cy - r, Colour (0xff6c7076), cx + r, cy + r, false); g.setGradientFill (hg); g.fillPath (hex);
          g.setColour (Colours::black.withAlpha (0.35f)); g.strokePath (hex, PathStrokeType (0.8f)); }
        { ColourGradient rg (Colour (0xff2a2b2f), cx, cy - r, Colour (0xff6b6e74), cx, cy + r, false); g.setGradientFill (rg); g.fillEllipse (cx - r * 0.7f, cy - r * 0.7f, r * 1.4f, r * 1.4f); }
        const float dir = isUp() ? -1.0f : 1.0f, L = r * 2.6f;
        const auto tip = Point<float> (cx, cy + dir * L);
        g.setColour (Colours::black.withAlpha (0.35f)); g.drawLine (Line<float> (Point<float> (cx + 1.5f, cy + 2), tip.translated (2.5f, 3)), r * 0.7f);
        { ColourGradient lg (Colour (0xfff6f7f9), cx - r * 0.4f, cy, Colour (0xff7f838a), cx + r * 0.4f, cy, false); g.setGradientFill (lg);
          Path lev; lev.startNewSubPath (cx - r * 0.22f, cy); lev.lineTo (cx - r * 0.34f, tip.y); lev.lineTo (cx + r * 0.34f, tip.y); lev.lineTo (cx + r * 0.22f, cy); lev.closeSubPath(); g.fillPath (lev); }
        { ColourGradient bg (Colour (0xffffffff), tip.x - r * 0.3f, tip.y - r * 0.3f, Colour (0xff8a8e95), tip.x + r * 0.4f, tip.y + r * 0.4f, true); g.setGradientFill (bg); g.fillEllipse (tip.x - r * 0.45f, tip.y - r * 0.45f, r * 0.9f, r * 0.9f); }
    }
    void mouseDown (const juce::MouseEvent&) override { param->setValueNotifyingHost (param->getValue() > 0.5f ? 0.0f : 1.0f); if (onChange) onChange(); repaint(); }
    juce::RangedAudioParameter* param; juce::String label, upT, dnT; bool invert; juce::Colour inkC { 0xff1e1b17 }; std::function<void()> onChange;
};

// fila di pulsanti a scatto con la spia (rapporti del 1176, modi dei compressori, selettori a pochi valori)
class Buttons : public juce::Component, public juce::SettableTooltipClient
{
public:
    Buttons (juce::RangedAudioParameter* p, juce::String l, juce::StringArray it) : param (p), label (std::move (l)), items (std::move (it)) {}
    int sel() const { return juce::jlimit (0, items.size() - 1, juce::roundToInt (param->getValue() * (items.size() - 1))); }
    void paint (juce::Graphics& g) override
    {
        const float w = (float) getWidth(), h = (float) getHeight(); const int n = items.size(), cur = sel();
        silk (g, label, { 0, 0, w, 13 }, alook::serif (9.0f), inkC);
        const float bw = (w - 4) / n, by = 14, bh = juce::jmin (20.0f, h - 15);
        for (int i = 0; i < n; ++i)
        {
            auto r = juce::Rectangle<float> (2 + i * bw, by, bw, bh).reduced (2, 0);
            const bool on = i == cur;
            g.setColour (juce::Colours::black.withAlpha (0.55f)); g.fillRoundedRectangle (r.translated (0, 1.5f).expanded (1), 2.5f);
            juce::ColourGradient cg (on ? juce::Colour (0xfff4f1ea) : juce::Colour (0xff3b3a37), r.getX(), r.getY(), on ? juce::Colour (0xffb9b4a8) : juce::Colour (0xff1d1c1a), r.getX(), r.getBottom(), false);
            g.setGradientFill (cg); g.fillRoundedRectangle (on ? r.translated (0, 1) : r, 2.5f);   // premuto: un filo piu' giu'
            g.setColour (juce::Colours::white.withAlpha (on ? 0.5f : 0.14f)); g.drawHorizontalLine ((int) r.getY() + (on ? 2 : 1), r.getX() + 2, r.getRight() - 2);
            if (on) { g.setColour (hot.withAlpha (0.9f)); g.fillRoundedRectangle (r.getCentreX() - 5, r.getY() + 3 + 1, 10, 2.5f, 1); }
            g.setFont (alook::typed (juce::jmin (9.0f, bw * 0.24f))); g.setColour (on ? juce::Colour (0xff1c1915) : juce::Colour (0xffcfc9bc));
            g.drawFittedText (items[i], (on ? r.translated (0, 1) : r).withTrimmedTop (5).toNearestInt(), juce::Justification::centred, 1, 0.6f);
        }
    }
    void mouseDown (const juce::MouseEvent& e) override
    {
        const int n = items.size(); const float bw = (getWidth() - 4.0f) / n; const int i = juce::jlimit (0, n - 1, (int) ((e.position.x - 2) / bw));
        param->beginChangeGesture(); param->setValueNotifyingHost (n > 1 ? (float) i / (n - 1) : 0.0f); param->endChangeGesture(); repaint();
    }
    juce::RangedAudioParameter* param; juce::String label; juce::StringArray items; juce::Colour inkC { 0xff1e1b17 }, hot { 0xffd8452f };
};

// strumento a ago (VU) con cornice nera e luce calda
inline void vu (juce::Graphics& g, juce::Rectangle<float> b, float db, const juce::String& title, bool grMode)
{
    {   // cornice metallica spessa con ombra sul pannello
        g.setColour (juce::Colours::black.withAlpha (0.45f)); g.fillRoundedRectangle (b.expanded (7).translated (1.5f, 3), 7);
        juce::ColourGradient fg (juce::Colour (0xff8d8a84), b.getX(), b.getY() - 7, juce::Colour (0xff1d1c1a), b.getX(), b.getBottom() + 7, false); g.setGradientFill (fg); g.fillRoundedRectangle (b.expanded (7), 7);
        g.setColour (juce::Colours::white.withAlpha (0.25f)); g.drawRoundedRectangle (b.expanded (6.5f), 6.5f, 0.8f);
        g.setColour (juce::Colour (0xff0b0a09)); g.fillRoundedRectangle (b.expanded (3), 4);
    }
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
    // vetro: ombra interna in alto, riflesso obliquo, polvere
    { juce::ColourGradient ish (juce::Colours::black.withAlpha (0.35f), b.getX(), b.getY(), juce::Colours::transparentBlack, b.getX(), b.getY() + 10, false); g.setGradientFill (ish); g.fillRect (b.withHeight (10)); }
    { juce::Path ref; ref.startNewSubPath (b.getX(), b.getY()); ref.lineTo (b.getX() + b.getWidth() * 0.62f, b.getY()); ref.lineTo (b.getX() + b.getWidth() * 0.38f, b.getBottom()); ref.lineTo (b.getX(), b.getBottom()); ref.closeSubPath();
      juce::ColourGradient gl (juce::Colours::white.withAlpha (0.16f), b.getX(), b.getY(), juce::Colours::transparentWhite, b.getX() + b.getWidth() * 0.5f, b.getBottom(), false); g.setGradientFill (gl); g.fillPath (ref); }
    { juce::Random rd ((int) (b.getWidth() * 7)); for (int i = 0; i < 40; ++i) { g.setColour (juce::Colours::white.withAlpha (0.08f + rd.nextFloat() * 0.1f)); g.fillEllipse (b.getX() + rd.nextFloat() * b.getWidth(), b.getY() + rd.nextFloat() * b.getHeight(), 0.8f, 0.8f); } }
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
