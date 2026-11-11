#pragma once
// Linea grafica "analogica crema": pannelli verniciati, serigrafie, manopole in bachelite, deviatori a leva, spie a gemma.
#include <juce_gui_basics/juce_gui_basics.h>
#include "JackpotLook.h"

namespace alook
{
namespace A
{
    const juce::Colour cream { 0xffeee3c6 }, cream2 { 0xffdccdA6 }, ink { 0xff35261a }, inkSoft { 0xff6b5640 }, maroon { 0xff8e2620 }, red { 0xffc0392b };
    const juce::Colour rack { 0xff2b2a28 }, rack2 { 0xff1b1a19 }, chrome { 0xffc9c6bf }, phos { 0xff9cf2a8 }, crt { 0xff0d1a12 }, amberLamp { 0xffffb347 }, greenLamp { 0xff7dde5a };
}
inline juce::Font serif (float size, bool bold = true) { return jlook::ui (size, bold ? 2 : 1); }
inline juce::Font wide (float size) { return jlook::logo (size); }
inline juce::Font typed (float size) { return jlook::mono (size); }

// serigrafia: inchiostro con un filo di luce sotto (pannello verniciato)
inline void engrave (juce::Graphics& g, const juce::String& t, juce::Rectangle<float> r, juce::Font f, juce::Justification j = juce::Justification::centred, juce::Colour c = A::ink)
{
    g.setFont (f);
    g.setColour (juce::Colours::white.withAlpha (0.35f)); g.drawText (t, r.translated (0, 0.8f), j);
    g.setColour (c); g.drawText (t, r, j);
}
inline void screw (juce::Graphics& g, float x, float y, float r = 6, float rot = 0.6f)
{
    juce::ColourGradient sg (juce::Colour (0xffe9e6df), x - r * 0.5f, y - r * 0.6f, juce::Colour (0xff7d7a73), x + r * 0.6f, y + r * 0.7f, false);
    g.setColour (juce::Colours::black.withAlpha (0.35f)); g.fillEllipse (x - r + 0.8f, y - r + 1.5f, r * 2, r * 2);
    g.setGradientFill (sg); g.fillEllipse (x - r, y - r, r * 2, r * 2);
    g.setColour (juce::Colour (0xff4d4a44)); g.drawLine (x - std::cos (rot) * r * 0.75f, y - std::sin (rot) * r * 0.75f, x + std::cos (rot) * r * 0.75f, y + std::sin (rot) * r * 0.75f, 1.6f);
}
// pannello frontale crema con bordo, viti e un po' di patina
inline void faceplate (juce::Graphics& g, juce::Rectangle<float> r, bool screws = true, juce::Colour base = A::cream)
{
    g.setColour (juce::Colours::black.withAlpha (0.55f)); g.fillRoundedRectangle (r.translated (0, 3).expanded (1), 5);
    juce::ColourGradient pg (base.brighter (0.06f), r.getX(), r.getY(), base.darker (0.1f), r.getRight(), r.getBottom(), false); g.setGradientFill (pg); g.fillRoundedRectangle (r, 4);
    // patina: macchie leggere e grana
    juce::Random rnd ((juce::int64) (r.getX() * 31 + r.getY() * 7));
    for (int i = 0; i < 40; ++i) { const float x = r.getX() + rnd.nextFloat() * r.getWidth(), y = r.getY() + rnd.nextFloat() * r.getHeight(), s = 20 + rnd.nextFloat() * 80;
        juce::ColourGradient m (juce::Colour (0xff8a6a3a).withAlpha (0.035f), x, y, juce::Colours::transparentBlack, x + s, y, true); g.setGradientFill (m); g.fillEllipse (x - s, y - s, s * 2, s * 2); }
    jlook::grain (g, r.toNearestInt(), 0.05f);
    g.setColour (juce::Colours::white.withAlpha (0.5f)); g.drawHorizontalLine ((int) r.getY() + 1, r.getX() + 3, r.getRight() - 3);
    g.setColour (juce::Colours::black.withAlpha (0.35f)); g.drawRoundedRectangle (r, 4, 1.0f);
    if (screws) { screw (g, r.getX() + 12, r.getY() + 12); screw (g, r.getRight() - 12, r.getY() + 12, 6, 1.9f); screw (g, r.getX() + 12, r.getBottom() - 12, 6, 2.6f); screw (g, r.getRight() - 12, r.getBottom() - 12, 6, 0.2f); }
}
// spia a gemma (rossa, ambra, verde)
inline void jewel (juce::Graphics& g, float cx, float cy, float r, juce::Colour c, bool on)
{
    g.setColour (juce::Colour (0xff8f8a80)); g.fillEllipse (cx - r - 3, cy - r - 3, (r + 3) * 2, (r + 3) * 2);
    g.setColour (juce::Colour (0xff3a3833)); g.drawEllipse (cx - r - 3, cy - r - 3, (r + 3) * 2, (r + 3) * 2, 1.0f);
    if (on) { juce::ColourGradient hg (c.withAlpha (0.5f), cx, cy, c.withAlpha (0.0f), cx + r * 4, cy, true); g.setGradientFill (hg); g.fillEllipse (cx - r * 4, cy - r * 4, r * 8, r * 8); }
    juce::ColourGradient jg (on ? c.brighter (0.7f) : c.darker (0.6f).withMultipliedSaturation (0.6f), cx - r * 0.3f, cy - r * 0.4f, on ? c.darker (0.2f) : c.darker (0.9f), cx + r, cy + r, true);
    g.setGradientFill (jg); g.fillEllipse (cx - r, cy - r, r * 2, r * 2);
    // sfaccettature
    g.setColour (juce::Colours::white.withAlpha (on ? 0.55f : 0.2f)); g.fillEllipse (cx - r * 0.55f, cy - r * 0.6f, r * 0.5f, r * 0.35f);
}

// manopola in bachelite con gonna zigrinata e scala serigrafata 0-10 sul pannello
class Knob : public juce::Component
{
public:
    Knob (juce::RangedAudioParameter* p, juce::String l, juce::Colour cap = juce::Colour (0xff1d1915)) : param (p), label (std::move (l)), capCol (cap) {}
    void paint (juce::Graphics& g) override
    {
        const float lh = 16.0f, vh = 14.0f, S = juce::jmin ((float) getWidth(), getHeight() - lh - vh);
        const float cx = getWidth() * 0.5f, cy = lh + S * 0.5f, R = S * 0.5f;
        const float a0 = -2.36f, a1 = 2.36f, v = param->getValue();
        engrave (g, label, { 0, 0, (float) getWidth(), lh - 2 }, serif (big ? 11.0f : 9.5f));
        // scala
        const int n = ticks;
        for (int i = 0; i <= n; ++i)
        {
            const float a = a0 + (a1 - a0) * i / n, ro = R - 1, ri = R - (i % (n / 2 == 0 ? 1 : n / 2) == 0 ? 7.0f : 4.5f);
            g.setColour (A::ink.withAlpha (0.85f)); g.drawLine (cx + std::sin (a) * ri, cy - std::cos (a) * ri, cx + std::sin (a) * ro, cy - std::cos (a) * ro, 1.1f);
            if (numbers && (i % 2 == 0)) { g.setFont (typed (big ? 8.5f : 7.5f)); g.drawText (juce::String (i * 10 / n), juce::Rectangle<float> (cx + std::sin (a) * (R + 7) - 9, cy - std::cos (a) * (R + 7) - 5, 18, 10), juce::Justification::centred); }
        }
        const float rk = R - 9;   // raggio della manopola
        // ombra e gonna metallica zigrinata
        g.setColour (juce::Colours::black.withAlpha (0.45f)); g.fillEllipse (cx - rk + 1.5f, cy - rk + 3.5f, rk * 2, rk * 2);
        juce::ColourGradient sk (juce::Colour (0xffd9d5cc), cx - rk, cy - rk, juce::Colour (0xff6e6a62), cx + rk, cy + rk, false); g.setGradientFill (sk); g.fillEllipse (cx - rk, cy - rk, rk * 2, rk * 2);
        const float av = a0 + (a1 - a0) * v;
        g.setColour (juce::Colours::black.withAlpha (0.25f));
        for (int i = 0; i < 48; ++i) { const float a = av + i * juce::MathConstants<float>::twoPi / 48; g.drawLine (cx + std::sin (a) * rk * 0.84f, cy - std::cos (a) * rk * 0.84f, cx + std::sin (a) * rk, cy - std::cos (a) * rk, 0.7f); }
        // cappello in bachelite
        const float rc = rk * 0.74f;
        juce::ColourGradient cg (capCol.brighter (0.55f), cx - rc * 0.5f, cy - rc * 0.7f, capCol.darker (0.4f), cx + rc * 0.6f, cy + rc, true); g.setGradientFill (cg); g.fillEllipse (cx - rc, cy - rc, rc * 2, rc * 2);
        g.setColour (juce::Colours::white.withAlpha (0.18f)); g.fillEllipse (cx - rc * 0.6f, cy - rc * 0.75f, rc * 0.8f, rc * 0.45f);
        g.setColour (juce::Colours::black.withAlpha (0.5f)); g.drawEllipse (cx - rc, cy - rc, rc * 2, rc * 2, 0.8f);
        // indice bianco
        g.setColour (juce::Colour (0xfff4efe2)); g.drawLine (cx + std::sin (av) * rc * 0.25f, cy - std::cos (av) * rc * 0.25f, cx + std::sin (av) * rk * 0.97f, cy - std::cos (av) * rk * 0.97f, big ? 2.6f : 2.0f);
        // valore, come scritto a matita su un nastro
        const juce::String t = text ? text() : param->getCurrentValueAsText();
        g.setColour (drag || hover ? A::maroon : A::inkSoft); g.setFont (typed (big ? 10.0f : 8.8f));
        g.drawText (t, juce::Rectangle<float> (-10, (float) getHeight() - vh, getWidth() + 20.0f, vh), juce::Justification::centred);
        if (! lit) { g.setColour (A::cream.withAlpha (0.45f)); g.fillRect (getLocalBounds()); }
    }
    void mouseDown (const juce::MouseEvent& e) override
    {
        if (onTouch) onTouch();
        if (e.mods.isAltDown() || e.mods.isCommandDown()) { param->setValueNotifyingHost (param->getDefaultValue()); return; }
        start = param->getValue(); param->beginChangeGesture(); drag = true;
    }
    void mouseUp (const juce::MouseEvent&) override { if (drag) param->endChangeGesture(); drag = false; repaint(); }
    void mouseDrag (const juce::MouseEvent& e) override
    { if (drag) param->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, start - e.getDistanceFromDragStartY() / (e.mods.isShiftDown() ? 900.0f : 220.0f))); }
    void mouseDoubleClick (const juce::MouseEvent&) override { param->setValueNotifyingHost (param->getDefaultValue()); }
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w) override
    { if (onTouch) onTouch(); param->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, param->getValue() + (w.deltaY > 0 ? 0.01f : -0.01f))); }
    void mouseEnter (const juce::MouseEvent&) override { hover = true; setMouseCursor (juce::MouseCursor::UpDownResizeCursor); repaint(); }
    void mouseExit (const juce::MouseEvent&) override { hover = false; repaint(); }
    juce::RangedAudioParameter* param; juce::String label; juce::Colour capCol;
    std::function<juce::String()> text; std::function<void()> onTouch;
    bool big = false, numbers = true, lit = true; int ticks = 10; float shown = -1;
private:
    float start = 0; bool drag = false, hover = false;
};

// selettore rotativo a scatti con le posizioni scritte intorno
class Rotary : public juce::Component
{
public:
    Rotary (juce::RangedAudioParameter* p, juce::String l, juce::StringArray it, float spanDeg = 270) : param (p), label (std::move (l)), items (std::move (it)), span (juce::degreesToRadians (spanDeg)) {}
    float angleOf (int k) const { const int n = juce::jmax (2, items.size()); return -span * 0.5f + span * k / (n - 1); }
    juce::Point<float> centre() const { return { getWidth() * 0.5f, 16.0f + (getHeight() - 16.0f) * 0.5f }; }
    float radius() const { return juce::jmin (getWidth() * 0.5f, (getHeight() - 16.0f) * 0.5f) * knobFrac; }
    void paint (juce::Graphics& g) override
    {
        engrave (g, label, { 0, 0, (float) getWidth(), 14 }, serif (10.0f));
        const auto c = centre(); const float rk = radius(); const int cur = juce::roundToInt (param->convertFrom0to1 (param->getValue()));
        for (int k = 0; k < items.size(); ++k)
        {
            const float a = angleOf (k), rl = rk + labelGap;
            g.setColour (A::ink); g.drawLine (c.x + std::sin (a) * (rk + 2), c.y - std::cos (a) * (rk + 2), c.x + std::sin (a) * (rk + 7), c.y - std::cos (a) * (rk + 7), 1.2f);
            auto r = juce::Rectangle<float> (c.x + std::sin (a) * rl - 34, c.y - std::cos (a) * rl - 7, 68, 14);
            engrave (g, items[k], r, serif (fontSize), juce::Justification::centred, k == cur ? A::maroon : A::ink);
        }
        // piastra di fondo cromata
        juce::ColourGradient cg (juce::Colour (0xffe6e2d9), c.x - rk * 0.6f, c.y - rk * 0.6f, juce::Colour (0xff77736b), c.x + rk * 0.7f, c.y + rk, false); g.setGradientFill (cg); g.fillEllipse (c.x - rk * 0.75f, c.y - rk * 0.75f, rk * 1.5f, rk * 1.5f);
        // testa a "becco": la manopola a forma di freccia dei selettori d'epoca
        const float a = angleOf (cur);
        // testa a pollo: corpo tondo e becco lungo che si assottiglia
        juce::Path ptr; ptr.startNewSubPath (-rk * 0.12f, -rk * 1.18f); ptr.lineTo (rk * 0.12f, -rk * 1.18f); ptr.lineTo (rk * 0.5f, rk * 0.1f); ptr.lineTo (-rk * 0.5f, rk * 0.1f); ptr.closeSubPath();
        ptr.addEllipse (-rk * 0.58f, -rk * 0.58f, rk * 1.16f, rk * 1.16f); ptr.startNewSubPath (-rk * 0.2f, rk * 0.5f); ptr.lineTo (rk * 0.2f, rk * 0.5f); ptr.lineTo (rk * 0.12f, rk * 0.85f); ptr.lineTo (-rk * 0.12f, rk * 0.85f); ptr.closeSubPath();
        ptr.applyTransform (juce::AffineTransform::rotation (a).translated (c.x, c.y));
        g.setColour (juce::Colours::black.withAlpha (0.4f)); g.fillPath (ptr, juce::AffineTransform::translation (1.5f, 2.5f));
        juce::ColourGradient pg (juce::Colour (0xff4a3b31), c.x - rk, c.y - rk, juce::Colour (0xff0d0a08), c.x + rk, c.y + rk, false); g.setGradientFill (pg); g.fillPath (ptr);
        g.setColour (juce::Colours::white.withAlpha (0.12f)); g.strokePath (ptr, juce::PathStrokeType (0.8f));
        g.setColour (juce::Colour (0xfff4efe2)); g.drawLine (c.x + std::sin (a) * rk * 0.35f, c.y - std::cos (a) * rk * 0.35f, c.x + std::sin (a) * rk * 1.1f, c.y - std::cos (a) * rk * 1.1f, 1.8f);
        if (! lit) { g.setColour (A::cream.withAlpha (0.45f)); g.fillRect (getLocalBounds()); }
    }
    void mouseDown (const juce::MouseEvent& e) override
    {
        if (onTouch) onTouch();
        // clic su una scritta: va li; altrimenti trascina
        const auto c = centre(); const float rl = radius() + labelGap;
        for (int k = 0; k < items.size(); ++k) { const float a = angleOf (k); if (e.position.getDistanceFrom ({ c.x + std::sin (a) * rl, c.y - std::cos (a) * rl }) < 16) { set (k); return; } }
        startIdx = juce::roundToInt (param->convertFrom0to1 (param->getValue()));
    }
    void mouseDrag (const juce::MouseEvent& e) override { set (juce::jlimit (0, items.size() - 1, startIdx + (int) (-e.getDistanceFromDragStartY() / 18))); }
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w) override { set (juce::jlimit (0, items.size() - 1, juce::roundToInt (param->convertFrom0to1 (param->getValue())) + (w.deltaY > 0 ? 1 : -1))); }
    void set (int k) { param->setValueNotifyingHost (param->convertTo0to1 ((float) k)); if (onPick) onPick (k); repaint(); }
    juce::RangedAudioParameter* param; juce::String label; juce::StringArray items; float span; float fontSize = 9.0f, knobFrac = 0.42f, labelGap = 18; bool lit = true;
    std::function<void()> onTouch; std::function<void (int)> onPick;
private:
    int startIdx = 0;
};

// deviatore a leva (bat toggle) con le scritte sopra e sotto
class Toggle : public juce::Component
{
public:
    Toggle (juce::RangedAudioParameter* p, juce::String l, juce::String up = "ON", juce::String down = "OFF") : param (p), label (std::move (l)), upT (std::move (up)), dnT (std::move (down)) {}
    void paint (juce::Graphics& g) override
    {
        const bool on = param->getValue() > 0.5f; const float cx = 22, cy = getHeight() * 0.5f;
        engrave (g, upT, { 0, 0, 44, 12 }, serif (8.0f)); engrave (g, dnT, { 0, (float) getHeight() - 12, 44, 12 }, serif (8.0f));
        // dado cromato
        juce::ColourGradient ng (juce::Colour (0xffeeebe4), cx - 8, cy - 8, juce::Colour (0xff6e6a63), cx + 8, cy + 8, false); g.setGradientFill (ng); g.fillEllipse (cx - 9, cy - 9, 18, 18);
        g.setColour (juce::Colour (0xff2e2c28)); g.fillEllipse (cx - 4, cy - 4, 8, 8);
        // leva
        const float ty = on ? cy - 15 : cy + 15;
        g.setColour (juce::Colours::black.withAlpha (0.35f)); g.drawLine (cx + 1.5f, cy + 2, cx + 1.5f, ty + 2, 5.0f);
        juce::ColourGradient lg (juce::Colour (0xfff2efe8), cx - 3, 0, juce::Colour (0xff8b877f), cx + 3, 0, false); g.setGradientFill (lg);
        juce::Path lev; lev.startNewSubPath (cx, cy); lev.lineTo (cx, ty); g.strokePath (lev, juce::PathStrokeType (4.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.fillEllipse (cx - 4, ty - 4, 8, 8);
        engrave (g, label, { 46, 0, (float) getWidth() - 46, (float) getHeight() }, serif (9.5f), juce::Justification::centredLeft);
    }
    void mouseDown (const juce::MouseEvent&) override { param->setValueNotifyingHost (param->getValue() > 0.5f ? 0.0f : 1.0f); if (onPick) onPick(); repaint(); }
    juce::RangedAudioParameter* param; juce::String label, upT, dnT; std::function<void()> onPick;
};

// strumento ad ago in cornice crema
inline void vuMeter (juce::Graphics& g, juce::Rectangle<float> b, float db, const juce::String& title)
{
    g.setColour (juce::Colour (0xff1c1a17)); g.fillRoundedRectangle (b.expanded (4), 6);
    juce::ColourGradient cg (juce::Colour (0xfff7ecd0), b.getCentreX(), b.getY(), juce::Colour (0xffdcc79a), b.getCentreX(), b.getBottom(), false); g.setGradientFill (cg); g.fillRoundedRectangle (b, 3);
    juce::ColourGradient lamp (A::amberLamp.withAlpha (0.25f), b.getCentreX(), b.getBottom(), juce::Colours::transparentBlack, b.getCentreX(), b.getY(), false); g.setGradientFill (lamp); g.fillRoundedRectangle (b, 3);
    const float cx = b.getCentreX(), cy = b.getBottom() + b.getHeight() * 0.45f, R = b.getHeight() * 1.15f;
    auto ang = [] (float d) { return juce::jmap (juce::jlimit (-20.0f, 3.0f, d), -20.0f, 3.0f, -0.72f, 0.72f); };
    { juce::Path arc; arc.addCentredArc (cx, cy, R * 0.86f, R * 0.86f, 0, ang (-20), ang (0), true); g.setColour (A::ink); g.strokePath (arc, juce::PathStrokeType (1.0f)); }
    { juce::Path arc; arc.addCentredArc (cx, cy, R * 0.86f, R * 0.86f, 0, ang (0), ang (3), true); g.setColour (A::red); g.strokePath (arc, juce::PathStrokeType (2.5f)); }
    for (float d : { -20.f, -10.f, -7.f, -5.f, -3.f, 0.f, 3.f })
    {
        const float a = ang (d); g.setColour (d > 0 ? A::red : A::ink);
        g.drawLine (cx + std::sin (a) * R * 0.86f, cy - std::cos (a) * R * 0.86f, cx + std::sin (a) * R * 0.94f, cy - std::cos (a) * R * 0.94f, 1.2f);
        g.setFont (typed (7.0f)); g.drawText (d > 0 ? "+" + juce::String ((int) d) : juce::String ((int) d), juce::Rectangle<float> (cx + std::sin (a) * R * 0.76f - 10, cy - std::cos (a) * R * 0.76f - 5, 20, 10), juce::Justification::centred);
    }
    engrave (g, title, { b.getX(), b.getBottom() - 15, b.getWidth(), 12 }, serif (8.5f));
    g.saveState(); g.reduceClipRegion (b.toNearestInt());
    const float a = ang (db); g.setColour (juce::Colour (0xff15110c)); g.drawLine (cx, cy, cx + std::sin (a) * R * 0.98f, cy - std::cos (a) * R * 0.98f, 1.3f);
    g.restoreState();
    g.setColour (juce::Colours::white.withAlpha (0.2f)); g.drawRoundedRectangle (b, 3, 1.0f);
}
}
