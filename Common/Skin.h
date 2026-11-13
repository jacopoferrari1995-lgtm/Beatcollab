#pragma once
// PELLE della serie: i plugin "vecchi" (jlook + jkit) si ridisegnano con i temi del Sampler.
// CYBER, ROSSO, RETRO, MACCHINA (tavolozza propria del plugin), SCURO, CHIARO.
// Il tema e' uno per plugin su questo Mac (tutte le istanze uguali), salvato nelle preferenze.
#include "Neo.h"

namespace skin
{
inline int& cur() { static int t = -1; return t; }
inline juce::String& plug() { static juce::String p; return p; }
inline juce::String& machineName() { static juce::String n = "MACCHINA"; return n; }
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
inline juce::String label (int t) { return t == 3 ? "CYBER" : t == 4 ? "ROSSO" : t == 8 ? "RETRO" : t == 9 ? machineName() : t == 2 ? "CHIARO" : "SCURO"; }
inline int next (int t) { return t == 3 ? 4 : t == 4 ? 8 : t == 8 ? 9 : t == 9 ? 1 : t == 1 ? 2 : 3; }

// ------------------------------------------------------------------------------------------------ disegno
using namespace neo;
inline void panel (juce::Graphics& g, juce::Rectangle<float> r, float rad, juce::Colour accent, bool active, bool lifted)
{
    juce::ignoreUnused (lifted);
    const auto tag = active ? accent : K::faint;
    if (cyber()) { plate (g, r, tag, juce::jlimit (8.0f, 14.0f, rad)); return; }
    rad = juce::jmin (rad, 12.0f);
    g.setColour (K::lo.withAlpha (light() ? 0.10f : 0.45f)); g.fillRoundedRectangle (r.translated (0, 3), rad);
    juce::ColourGradient cg (K::panel0, 0, r.getY(), K::panel1, 0, r.getBottom(), false); g.setGradientFill (cg); g.fillRoundedRectangle (r, rad);
    if (K::theme == 9 && K::customStyle != 2)
    {   // metallo spazzolato con le viti
        g.saveState(); g.reduceClipRegion (r.toNearestInt());
        juce::Random rr ((juce::int64) (r.getX() * 13 + r.getY()));
        for (float y = r.getY() + 1; y < r.getBottom(); y += 1.5f) { g.setColour (K::hi.withAlpha (0.008f + rr.nextFloat() * 0.018f)); g.fillRect (r.getX(), y, r.getWidth(), 0.7f); }
        g.restoreState();
        for (int k = 0; k < 4; ++k)
        {
            const float x = (k & 1) ? r.getRight() - 10 : r.getX() + 10, y = (k & 2) ? r.getBottom() - 10 : r.getY() + 10;
            if (r.getWidth() < 160 || r.getHeight() < 70) break;
            juce::ColourGradient sg (K::hi.withAlpha (0.35f), x - 2, y - 2, K::lo.withAlpha (0.6f), x + 2, y + 2, false); g.setGradientFill (sg); g.fillEllipse (x - 3, y - 3, 6, 6);
            g.setColour (K::lo.withAlpha (0.7f)); g.drawLine (x - 2, y + 1.5f, x + 2, y - 1.5f, 0.9f);
        }
    }
    g.setColour (K::hi.withAlpha (light() ? 0.10f : 0.07f)); g.drawRoundedRectangle (r.reduced (0.5f), rad, 1.0f);
    g.setColour (K::hi.withAlpha (light() ? 0.0f : 0.06f)); g.drawHorizontalLine ((int) r.getY() + 1, r.getX() + rad, r.getRight() - rad);
    if (active) { g.setColour (tag.withAlpha (0.9f)); g.fillRoundedRectangle (r.getX() + rad + 4, r.getY() - 1, juce::jmin (46.0f, r.getWidth() * 0.25f), 2.5f, 1.2f); }
}
inline void backdrop (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour, juce::Colour)
{
    const float W = r.getWidth(), H = r.getHeight();
    if (K::theme == 8) { memphisBackdrop (g, W, H); return; }
    if (K::theme == 3) { cyberBackdrop (g, W, H, 0.6f); return; }
    juce::ColourGradient base (K::bg1, 0, 0, K::bg0, 0, H, false); g.setGradientFill (base); g.fillRect (r);
    if (K::theme == 4)
    {   // rosso: trama a righe diagonali appena visibili
        g.setColour (juce::Colours::black.withAlpha (0.10f)); for (float x = -H; x < W; x += 9) g.drawLine (x, H, x + H, 0, 1.0f);
    }
    if (K::theme == 9 && K::customStyle == 1)
    {   // fianchi in legno
        for (int s = 0; s < 2; ++s)
        {
            auto w = juce::Rectangle<float> (s ? W - 14 : 0, 0, 14, H);
            juce::ColourGradient wg (juce::Colour (0xff5a341c), w.getX(), 0, juce::Colour (0xff3a2010), w.getRight(), 0, false); g.setGradientFill (wg); g.fillRect (w);
            juce::Random rr (s + 3); for (int i = 0; i < 40; ++i) { g.setColour (juce::Colour (0xff2a1608).withAlpha (0.35f)); const float x = w.getX() + rr.nextFloat() * 14; g.drawLine (x, 0, x + rr.nextFloat() * 3 - 1.5f, H, 0.7f); }
        }
    }
    if (K::theme == 9 && K::customStyle == 2)
    {   // tolex: puntinato fitto
        juce::Random rr (11); for (int i = 0; i < 9000; ++i) { g.setColour ((rr.nextBool() ? K::hi : K::lo).withAlpha (0.05f)); g.fillRect (rr.nextFloat() * W, rr.nextFloat() * H, 1.2f, 1.2f); }
    }
    juce::ColourGradient vg (juce::Colours::transparentBlack, r.getCentreX(), r.getCentreY(), juce::Colours::black.withAlpha (light() ? 0.10f : 0.35f), 0, 0, true);
    vg.addColour (0.6, juce::Colours::transparentBlack); g.setGradientFill (vg); g.fillRect (r);
}
inline void header (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& product, juce::Colour pc, const juce::String& model)
{
    juce::ignoreUnused (pc);
    const float W = r.getWidth(), h = r.getHeight(), cy = r.getCentreY();
    if (K::theme == 8)
    {   // fascia color crema con le righe arcobaleno sotto
        g.setColour (juce::Colour (0xffefe2cc)); g.fillRect (r);
        static const juce::uint32 st[4] = { 0xffe0663d, 0xffe8b448, 0xff5f9c93, 0xff6d58a6 };
        for (int i = 0; i < 4; ++i) { g.setColour (juce::Colour (st[i])); g.fillRect (0.0f, r.getBottom() - 10 + i * 2.5f, W, 2.5f); }
    }
    else if (cyber()) { g.setColour (K::lo.withAlpha (0.5f)); g.fillRect (r); g.setColour (K::accent.withAlpha (0.35f)); g.fillRect (0.0f, h - 1, W, 1.0f); }
    else
    {
        juce::ColourGradient hg (K::panel0, 0, 0, K::panel1, 0, h, false); g.setGradientFill (hg); g.fillRect (r);
        g.setColour (K::hi.withAlpha (0.08f)); g.fillRect (0.0f, h - 1, W, 1.0f);
        g.setColour (K::accent); g.fillRect (0.0f, h - 2, 120.0f, 2.0f);
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
    const float size = juce::jmin (area.getWidth(), area.getHeight());
    const float cx = area.getCentreX(), cy = area.getY() + size * 0.5f, R = size * 0.5f - 3.0f, kr = R * (k.big ? 0.72f : 0.7f);
    const auto c = k.enabled ? col : K::faint;
    const float live = juce::jmax (k.hover * 0.6f, k.active);
    const float av = -2.356f + 4.712f * juce::jlimit (0.0f, 1.0f, v);
    if (cyber() || K::theme == 4) ledRing (g, cx, cy, R, juce::jlimit (0.0f, 1.0f, v), k.bipolar, live > 0.01f ? c.brighter (0.2f) : c, k.big ? 3.2f : 2.4f, k.big ? 31 : 23);
    else
    {   // arco continuo sottile
        juce::Path tr; tr.addCentredArc (cx, cy, R - 1, R - 1, 0, -2.356f, 2.356f, true); g.setColour (K::hi.withAlpha (light() ? 0.14f : 0.09f)); g.strokePath (tr, juce::PathStrokeType (2.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        juce::Path va; va.addCentredArc (cx, cy, R - 1, R - 1, 0, k.bipolar ? 0.0f : -2.356f, av, true);
        g.setColour (c); g.strokePath (va, juce::PathStrokeType (2.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    g.setColour (K::lo.withAlpha (light() ? 0.18f : 0.45f)); g.fillEllipse (cx - kr, cy - kr + 2, kr * 2, kr * 2);
    if (K::theme == 9)
    {   // manopola d'epoca: gonna zigrinata e cappello lucido
        juce::ColourGradient sk (K::panel0.brighter (0.15f), cx, cy - kr, K::panel1.darker (0.5f), cx, cy + kr, false); g.setGradientFill (sk); g.fillEllipse (cx - kr, cy - kr, kr * 2, kr * 2);
        for (int i = 0; i < 36; ++i) { const float a = i * juce::MathConstants<float>::twoPi / 36; g.setColour (K::lo.withAlpha (0.35f)); g.drawLine (cx + std::sin (a) * kr * 0.86f, cy - std::cos (a) * kr * 0.86f, cx + std::sin (a) * kr, cy - std::cos (a) * kr, 0.8f); }
        const float ck = kr * 0.7f; juce::ColourGradient cp (K::hi.withAlpha (0.5f).interpolatedWith (K::panel0, 0.4f), cx - ck * 0.4f, cy - ck * 0.6f, K::panel1, cx + ck * 0.5f, cy + ck, true); g.setGradientFill (cp); g.fillEllipse (cx - ck, cy - ck, ck * 2, ck * 2);
    }
    else
    {
        const auto top = light() ? juce::Colour (0xfffbf9f6) : juce::Colour (0xff2a2e3a), bot = light() ? juce::Colour (0xffd9d4cd) : juce::Colour (0xff111319);
        juce::ColourGradient bgc (top, cx, cy - kr, bot, cx, cy + kr, false); g.setGradientFill (bgc); g.fillEllipse (cx - kr, cy - kr, kr * 2, kr * 2);
        g.setColour (K::hi.withAlpha (light() ? 0.12f : 0.08f)); g.drawEllipse (cx - kr + 0.5f, cy - kr + 0.5f, kr * 2 - 1, kr * 2 - 1, 1.0f);
    }
    const float r0 = kr * 0.3f, r1 = kr * 0.88f, sn = std::sin (av), cs = std::cos (av);
    juce::Path pl; pl.startNewSubPath (cx + sn * r0, cy - cs * r0); pl.lineTo (cx + sn * r1, cy - cs * r1);
    g.setColour (K::theme == 9 ? K::ink : c); g.strokePath (pl, juce::PathStrokeType (k.big ? 2.6f : 2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}
inline void glass (juce::Graphics& g, juce::Rectangle<float> r, float rad)
{
    if (cyber()) { brackets (g, r.reduced (3), K::accent.withAlpha (0.45f), 6.0f, 1.0f); return; }
    g.setColour (K::hi.withAlpha (light() ? 0.10f : 0.06f)); g.drawRoundedRectangle (r.reduced (0.5f), rad, 1.0f);
}
inline void bubble (juce::Graphics& g, juce::Rectangle<float> area, const juce::String& t, juce::Colour c)
{
    auto fo = num (10.5f); const float w = juce::GlyphArrangement::getStringWidth (fo, t) + 14;
    auto r = juce::Rectangle<float> (area.getCentreX() - w / 2, area.getY(), w, 18);
    g.setColour (K::panel1); g.fillRoundedRectangle (r, 4); g.setColour (c.withAlpha (0.8f)); g.drawRoundedRectangle (r, 4, 1.0f);
    g.setFont (fo); g.setColour (light() ? c.darker (0.4f) : c); g.drawText (t, r, juce::Justification::centred);
}
// schermo: vetro col colore del tema (sostituisce gli LCD verdi dei vecchi plugin)
inline void screen (juce::Graphics& g, juce::Rectangle<float> r, float rad, juce::Colour edge)
{
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
inline void chipDraw (juce::Graphics& g, juce::Rectangle<float> r, bool hover = false) { chip (g, r, label (cur()), false, K::accent, 8.5f, hover); }

// ------------------------------------------------------------------------------------------------ uso
inline void apply()
{
    if (cur() < 0) cur() = 3;
    setTheme (cur());
    auto& h = jlook::skinHooks(); h.on = true;
    h.card = panel; h.backdrop = backdrop; h.header = header; h.button = button; h.knob = knob; h.glass = glass; h.bubble = bubble;
}
// da chiamare nel costruttore dell'editor: id del plugin per le preferenze, nome e tavolozza del tema MACCHINA
inline void init (const juce::String& id, const juce::String& machine, std::initializer_list<juce::uint32> pal, std::initializer_list<juce::uint32> accents, bool lightPal = false, int style = 0)
{
    plug() = id; machineName() = machine;
    int i = 0; for (auto c : pal) if (i < 10) K::custom[i++] = c;
    i = 0; for (auto c : accents) if (i < 7) K::customAcc[i++] = c;
    K::customLight = lightPal; K::customStyle = style;
    if (cur() < 0) cur() = load (id, 3);
    K::theme = -1; apply();
}
inline void cycle() { cur() = next (cur()); save (plug(), cur()); K::theme = -1; apply(); }
}
