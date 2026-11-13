#pragma once
// Pezzi comuni per i plugin "semplici" della serie (Clip, Preamp EQ, Phase, Tube Box):
// manopole, selettori, interruttori, display LCD, barra preset con memoria A/B.
#include <juce_audio_processors/juce_audio_processors.h>
#include "JackpotLook.h"
#include "JackpotScale.h"
#include <complex>

namespace jkit
{
namespace C
{
    inline juce::Colour bg { 0xff0b0c10 }, text { 0xffdfe3ea }, dim { 0xff7d8494 }, faint { 0xff4f5562 };
    inline juce::Colour amber { 0xffffb000 }, rust { 0xffff7a1a }, cream { 0xfff3d9a4 }, phos { 0xff5ef2ff }, red { 0xffff3b5c }, green { 0xff4dff88 };
    inline juce::Colour magenta { 0xffff2e88 }, violet { 0xffa78bfa }, yellow { 0xffffe14d }, blue { 0xff60a5fa };
    // colori presi dal tema della pelle (se il plugin la usa)
    inline void sync()
    {
        if (! jlook::skinHooks().on) return;
        const auto& p = jlook::skinPal();
        bg = p.bg; text = p.text; dim = p.dim; faint = p.faint; amber = p.amber; rust = p.red.interpolatedWith (p.amber, 0.5f); cream = p.light ? p.text : p.amber.interpolatedWith (p.hi, 0.6f);
        phos = p.cyan; red = p.red; green = p.green; magenta = p.pink; violet = p.violet; yellow = p.amber.interpolatedWith (p.hi, p.light ? 0.0f : 0.3f); blue = p.blue;
    }
}
inline juce::Font uf (float size, bool bold = true) { return jlook::ui (size, bold ? 2 : 1); }
inline juce::Font mf (float size) { return jlook::mono (size); }
inline void glowText (juce::Graphics& g, const juce::String& t, juce::Rectangle<float> r, juce::Colour c, juce::Font f, juce::Justification j = juce::Justification::centredLeft)
{
    g.setFont (f);
    g.setColour (c.withAlpha (0.2f)); g.drawText (t, r.translated (-0.7f, 0.3f), j); g.drawText (t, r.translated (0.7f, 0.3f), j);
    g.setColour (c); g.drawText (t, r, j);
}
// schermo scuro con griglia leggera del colore del plugin
inline void lcd (juce::Graphics& g, juce::Rectangle<float> r, float rad, juce::Colour tint = C::phos)
{
    if (jlook::skinHooks().on && jlook::skinHooks().screen) { jlook::skinHooks().screen (g, r, rad, tint); return; }
    g.setColour (juce::Colours::black.withAlpha (0.65f)); g.fillRoundedRectangle (r.expanded (1.5f), rad + 1.5f);
    juce::ColourGradient cg (tint.interpolatedWith (juce::Colours::black, 0.9f), r.getCentreX(), r.getCentreY(), tint.interpolatedWith (juce::Colours::black, 0.96f), r.getX(), r.getY(), true);
    g.setGradientFill (cg); g.fillRoundedRectangle (r, rad);
    g.setColour (tint.withAlpha (0.035f));
    for (float x = r.getX() + 10; x < r.getRight(); x += 12) g.drawVerticalLine ((int) x, r.getY() + 2, r.getBottom() - 2);
    for (float y = r.getY() + 10; y < r.getBottom(); y += 12) g.drawHorizontalLine ((int) y, r.getX() + 2, r.getRight() - 2);
}
inline juce::String hzT (float f) { return f >= 1000 ? juce::String (f / 1000.0f, f >= 10000 ? 1 : 2) + " kHz" : juce::String (juce::roundToInt (f)) + " Hz"; }
inline juce::String dbT (float v, int dec = 1) { return (v > 0.05f ? "+" : "") + juce::String (v, dec) + " dB"; }
inline juce::String pctT (float v) { return juce::String (juce::roundToInt (v * 100)) + "%"; }

class Knob : public juce::Component
{
public:
    Knob (juce::RangedAudioParameter* p, juce::Colour c, juce::String l = {}) : param (p), col (c), label (std::move (l)) {}
    void paint (juce::Graphics& g) override
    {
        const float lh = big ? 32.0f : 27.0f, size = juce::jmin ((float) getWidth(), getHeight() - lh);
        const auto c = lit ? col : col.withSaturation (0.15f).withMultipliedBrightness (0.55f);
        jlook::KnobLook kl; kl.big = big; kl.bipolar = bipolar; kl.enabled = true; kl.hover = hover ? 1.0f : 0.0f; kl.active = drag ? 1.0f : 0.0f;
        jlook::drawKnob (g, { (getWidth() - size) * 0.5f, 0, size, size }, param->getValue(), c, kl);
        g.setColour (lit ? C::dim : C::faint); g.setFont (uf (big ? 11.5f : 9.0f)); g.drawText (label, juce::Rectangle<float> (-8, size + 1, getWidth() + 16.0f, 14), juce::Justification::centred);
        glowText (g, text ? text() : param->getCurrentValueAsText(), juce::Rectangle<float> (-8, size + 15, getWidth() + 16.0f, 14), drag ? c.brighter (0.4f) : c, mf (big ? 12.5f : 10.0f), juce::Justification::centred);
    }
    void mouseDown (const juce::MouseEvent& e) override
    {
        if (onTouch) onTouch();
        if (e.mods.isAltDown() || e.mods.isCommandDown()) { param->setValueNotifyingHost (param->getDefaultValue()); return; }
        start = param->getValue(); param->beginChangeGesture(); drag = true;
    }
    void mouseUp (const juce::MouseEvent&) override { if (drag) param->endChangeGesture(); drag = false; repaint(); }
    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (! drag) return;
        param->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, start - e.getDistanceFromDragStartY() / (e.mods.isShiftDown() ? 900.0f : 220.0f)));
    }
    void mouseDoubleClick (const juce::MouseEvent&) override { param->setValueNotifyingHost (param->getDefaultValue()); }
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w) override
    {
        if (onTouch) onTouch();
        param->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, param->getValue() + (w.deltaY > 0 ? 0.01f : -0.01f)));
    }
    void mouseEnter (const juce::MouseEvent&) override { hover = true; setMouseCursor (juce::MouseCursor::UpDownResizeCursor); repaint(); }
    void mouseExit (const juce::MouseEvent&) override { hover = false; repaint(); }
    juce::RangedAudioParameter* param; juce::Colour col; juce::String label;
    std::function<juce::String()> text; std::function<void()> onTouch;
    bool big = false, bipolar = false, lit = true; float shown = -1;
private:
    float start = 0; bool drag = false, hover = false;
};

// selettore a segmenti (parametro a scelte)
class Segs : public juce::Component
{
public:
    Segs (juce::RangedAudioParameter* p, juce::Colour c, juce::StringArray it) : param (p), col (c), items (std::move (it)) {}
    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat(); lcd (g, b, 6, col);
        const int n = juce::jmax (1, items.size()), cur = juce::roundToInt (param->convertFrom0to1 (param->getValue()));
        const int rows = juce::jmax (1, this->rows), perRow = (n + rows - 1) / rows;
        const float w = (b.getWidth() - 6) / perRow, h = (b.getHeight() - 6) / rows; const auto c = lit ? col : col.withSaturation (0.15f).withMultipliedBrightness (0.55f);
        for (int i = 0; i < n; ++i)
        {
            auto r = juce::Rectangle<float> (3 + (i % perRow) * w, 3 + (i / perRow) * h, w, h).reduced (1, 1);
            if (i == cur) { g.setColour (c.withAlpha (0.2f)); g.fillRoundedRectangle (r, 4); g.setColour (c.withAlpha (0.85f)); g.drawRoundedRectangle (r, 4, 1.0f); glowText (g, items[i], r, c, uf (font), juce::Justification::centred); }
            else { g.setColour (C::dim); g.setFont (uf (font)); g.drawText (items[i], r, juce::Justification::centred); }
        }
    }
    void mouseDown (const juce::MouseEvent& e) override
    {
        const int n = juce::jmax (1, items.size()), rows = juce::jmax (1, this->rows), perRow = (n + rows - 1) / rows;
        const int cx = juce::jlimit (0, perRow - 1, (int) ((e.position.x - 3) / ((getWidth() - 6) / (float) perRow)));
        const int cy = juce::jlimit (0, rows - 1, (int) ((e.position.y - 3) / ((getHeight() - 6) / (float) rows)));
        const int k = juce::jmin (n - 1, cy * perRow + cx);
        param->setValueNotifyingHost (param->convertTo0to1 ((float) k)); if (onPick) onPick (k); repaint();
    }
    juce::RangedAudioParameter* param; juce::Colour col; juce::StringArray items; float font = 10.0f; int rows = 1; bool lit = true; std::function<void (int)> onPick;
};

class Switch : public juce::Component
{
public:
    Switch (juce::RangedAudioParameter* p, juce::Colour c, juce::String l = {}) : param (p), col (c), label (std::move (l)) {}
    void paint (juce::Graphics& g) override
    {
        const bool on = param->getValue() > 0.5f; const float h = (float) juce::jmin (getHeight(), 20), w = h * 1.8f;
        auto t = juce::Rectangle<float> (0, (getHeight() - h) / 2, w, h); lcd (g, t, h / 2, col);
        const float d = h - 6, x = on ? t.getRight() - d - 3 : 3.0f;
        if (on) { g.setColour (col.withAlpha (0.35f)); g.fillEllipse (x - 3, t.getY(), d + 6, d + 6); }
        juce::ColourGradient lg (on ? col.brighter (0.6f) : juce::Colour (0xff4a4f5a), x, t.getY() + 3, on ? col : juce::Colour (0xff15171c), x, t.getY() + 3 + d, false); g.setGradientFill (lg); g.fillEllipse (x, t.getY() + 3, d, d);
        auto lr = juce::Rectangle<float> (w + 8, 0, getWidth() - w - 8, (float) getHeight());
        if (on) glowText (g, label, lr, col, uf (10.5f)); else { g.setColour (C::dim); g.setFont (uf (10.5f)); g.drawText (label, lr, juce::Justification::centredLeft); }
    }
    void mouseDown (const juce::MouseEvent&) override { param->setValueNotifyingHost (param->getValue() > 0.5f ? 0.0f : 1.0f); if (onPick) onPick(); repaint(); }
    juce::RangedAudioParameter* param; juce::Colour col; juce::String label; std::function<void()> onPick;
};

// grande pulsante legato a un parametro booleano o a scelte (cicla)
class Pill : public juce::Component
{
public:
    Pill (juce::RangedAudioParameter* p, juce::Colour c, std::function<juce::String()> t) : param (p), col (c), text (std::move (t)) {}
    void paint (juce::Graphics& g) override
    {
        const bool on = lit ? lit() : param->getValue() > 0.5f;
        jlook::button (g, getLocalBounds().toFloat().reduced (1), col, on, hover, text(), uf (fontSize));
    }
    void mouseDown (const juce::MouseEvent&) override
    {
        if (click) { click(); repaint(); return; }
        auto* ch = dynamic_cast<juce::AudioParameterChoice*> (param);
        if (ch != nullptr) { const int n = ch->choices.size(); param->setValueNotifyingHost (param->convertTo0to1 ((float) ((ch->getIndex() + 1) % n))); }
        else param->setValueNotifyingHost (param->getValue() > 0.5f ? 0.0f : 1.0f);
        repaint();
    }
    void mouseEnter (const juce::MouseEvent&) override { hover = true; repaint(); }
    void mouseExit (const juce::MouseEvent&) override { hover = false; repaint(); }
    juce::RangedAudioParameter* param; juce::Colour col; std::function<juce::String()> text; std::function<bool()> lit; std::function<void()> click;
    float fontSize = 11.5f; bool hover = false;
};

// preset: nome "Gruppo | Nome", nota, valori
struct Preset { const char* name; const char* note; std::vector<std::pair<const char*, float>> v; };
using PV = std::vector<std::pair<const char*, float>>;
inline PV cat (std::initializer_list<PV> parts) { PV r; for (auto& p : parts) r.insert (r.end(), p.begin(), p.end()); return r; }

// riporta tutto ai valori di partenza (tranne quelli indicati) e applica il preset
inline void applyPreset (juce::AudioProcessor& proc, juce::AudioProcessorValueTreeState& apvts, const Preset& p, juce::StringArray keep = { "out", "in" })
{
    for (auto* pp : proc.getParameters()) if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (pp)) if (! keep.contains (rp->paramID)) rp->setValueNotifyingHost (rp->getDefaultValue());
    for (auto& [id, v] : p.v) if (auto* rp = apvts.getParameter (id)) rp->setValueNotifyingHost (rp->convertTo0to1 (v));
}

// barra preset in alto: ‹ nome › e MEMORIA A/B
struct PresetBar
{
    juce::Rectangle<int> preset { 450, 15, 330, 36 }, prev { 412, 15, 36, 36 }, next { 782, 15, 36, 36 }, ab { 830, 15, 110, 36 };
    bool abHover = false;
    void paint (juce::Graphics& g, const juce::String& name, juce::Colour c, bool abB, const juce::String& abLabel) const
    {
        lcd (g, preset.toFloat(), 8, c); jlook::glass (g, preset.toFloat(), 8);
        glowText (g, name.toUpperCase(), preset.toFloat(), c, mf (12.5f), juce::Justification::centred);
        g.setColour (c); g.setFont (uf (20.0f));
        g.drawText (juce::String::fromUTF8 ("\xe2\x80\xb9"), prev, juce::Justification::centred); g.drawText (juce::String::fromUTF8 ("\xe2\x80\xba"), next, juce::Justification::centred);
        jlook::button (g, ab.toFloat(), c, abB, abHover, "MEMORIA " + abLabel, uf (11.5f));
    }
    // true se il clic e stato usato
    template <class Proc> bool click (juce::Component& owner, Proc& proc, const std::vector<Preset>& list, juce::Point<int> pos, juce::Rectangle<int> screenArea)
    {
        const int n = (int) list.size();
        if (ab.contains (pos)) { proc.ab.toggle (proc); owner.repaint(); return true; }
        if (prev.contains (pos)) { proc.loadPreset ((proc.preset + n - 1) % n); owner.repaint(); return true; }
        if (next.contains (pos)) { proc.loadPreset ((proc.preset + 1) % n); owner.repaint(); return true; }
        if (! preset.contains (pos)) return false;
        juce::PopupMenu m; std::vector<std::pair<juce::String, juce::PopupMenu>> groups;
        for (int i = 0; i < n; ++i)
        {
            const juce::String name = list[(size_t) i].name;
            if (! name.contains (" | ")) { m.addItem (i + 1, name, true, i == proc.preset); continue; }
            const auto gn = name.upToFirstOccurrenceOf (" | ", false, false), it = name.fromFirstOccurrenceOf (" | ", false, false);
            bool found = false; for (auto& gr : groups) if (gr.first == gn) { gr.second.addItem (i + 1, it, true, i == proc.preset); found = true; }
            if (! found) { juce::PopupMenu pm; pm.addItem (i + 1, it, true, i == proc.preset); groups.push_back ({ gn, pm }); }
        }
        for (auto& gr : groups) m.addSubMenu (gr.first, gr.second);
        juce::Component::SafePointer<juce::Component> safe (&owner);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&owner).withTargetScreenArea (screenArea), [safe, &proc] (int r) { if (r > 0 && safe != nullptr) { proc.loadPreset (r - 1); safe->repaint(); } });
        return true;
    }
};

// card con titolo e led
inline void titledCard (juce::Graphics& g, juce::Rectangle<float> c, const juce::String& title, const juce::String& sub, juce::Colour col, bool on = true)
{
    jlook::softShadow (g, c, 14); jlook::card (g, c, 14, on ? col : C::faint, true, false, false);
    auto led = juce::Rectangle<float> (c.getX() + 16, c.getY() + 13, 10, 10);
    if (on) { g.setColour (col.withAlpha (0.35f)); g.fillEllipse (led.expanded (4)); }
    g.setColour (on ? col : juce::Colour (0xff3a3d45)); g.fillEllipse (led);
    glowText (g, title, { c.getX() + 34, c.getY() + 7, c.getWidth() - 50, 22 }, on ? C::text : C::dim, uf (13.0f));
    if (sub.isNotEmpty()) { g.setColour (C::dim); g.setFont (mf (9.0f)); g.drawText (sub, juce::Rectangle<float> (c.getX() + 150, c.getY() + 9, c.getWidth() - 166, 18), juce::Justification::centredRight); }
}
// misuratore verticale a barre (dB)
inline void vMeter (juce::Graphics& g, juce::Rectangle<float> r, float lin, juce::Colour c, float floorDb = -48.0f)
{
    lcd (g, r, 3, c);
    const float db = juce::Decibels::gainToDecibels (lin, -100.0f), t = juce::jlimit (0.0f, 1.0f, (db - floorDb) / -floorDb);
    auto f = r.reduced (2); auto lv = f.removeFromBottom (f.getHeight() * t);
    juce::ColourGradient cg (c, 0, f.getBottom(), db > -1 ? C::red : c.brighter (0.5f), 0, r.getY(), false); g.setGradientFill (cg); g.fillRect (lv);
}

// ======================= DSP comune =======================
struct Bq
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    inline float p (float x) { const double y = b0 * x + z1; z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y; return (float) y; }
    void reset() { z1 = z2 = 0; }
    void flat() { b0 = 1; b1 = b2 = a1 = a2 = 0; }
    void set (double B0, double B1, double B2, double A0, double A1, double A2) { b0 = B0 / A0; b1 = B1 / A0; b2 = B2 / A0; a1 = A1 / A0; a2 = A2 / A0; }
    void lp (double sr, double f, double q = 0.7071) { const double w = juce::MathConstants<double>::twoPi * juce::jlimit (5.0, sr * 0.49, f) / sr, c = std::cos (w), a = std::sin (w) / (2 * q); set ((1 - c) / 2, 1 - c, (1 - c) / 2, 1 + a, -2 * c, 1 - a); }
    void hp (double sr, double f, double q = 0.7071) { const double w = juce::MathConstants<double>::twoPi * juce::jlimit (5.0, sr * 0.49, f) / sr, c = std::cos (w), a = std::sin (w) / (2 * q); set ((1 + c) / 2, -(1 + c), (1 + c) / 2, 1 + a, -2 * c, 1 - a); }
    void ap (double sr, double f, double q = 0.7071) { const double w = juce::MathConstants<double>::twoPi * juce::jlimit (5.0, sr * 0.49, f) / sr, c = std::cos (w), a = std::sin (w) / (2 * q); set (1 - a, -2 * c, 1 + a, 1 + a, -2 * c, 1 - a); }
    void peak (double sr, double f, double q, double db)
    {
        const double A = std::pow (10.0, db / 40), w = juce::MathConstants<double>::twoPi * juce::jlimit (5.0, sr * 0.49, f) / sr, c = std::cos (w), a = std::sin (w) / (2 * q);
        set (1 + a * A, -2 * c, 1 - a * A, 1 + a / A, -2 * c, 1 - a / A);
    }
    void lowShelf (double sr, double f, double db, double s = 0.8)
    {
        const double A = std::pow (10.0, db / 40), w = juce::MathConstants<double>::twoPi * juce::jlimit (5.0, sr * 0.49, f) / sr, c = std::cos (w), sn = std::sin (w);
        const double al = sn / 2 * std::sqrt ((A + 1 / A) * (1 / s - 1) + 2), sq = 2 * std::sqrt (A) * al;
        set (A * ((A + 1) - (A - 1) * c + sq), 2 * A * ((A - 1) - (A + 1) * c), A * ((A + 1) - (A - 1) * c - sq), (A + 1) + (A - 1) * c + sq, -2 * ((A - 1) + (A + 1) * c), (A + 1) + (A - 1) * c - sq);
    }
    void highShelf (double sr, double f, double db, double s = 0.8)
    {
        const double A = std::pow (10.0, db / 40), w = juce::MathConstants<double>::twoPi * juce::jlimit (5.0, sr * 0.49, f) / sr, c = std::cos (w), sn = std::sin (w);
        const double al = sn / 2 * std::sqrt ((A + 1 / A) * (1 / s - 1) + 2), sq = 2 * std::sqrt (A) * al;
        set (A * ((A + 1) + (A - 1) * c + sq), -2 * A * ((A - 1) + (A + 1) * c), A * ((A + 1) + (A - 1) * c - sq), (A + 1) - (A - 1) * c + sq, 2 * ((A - 1) - (A + 1) * c), (A + 1) - (A - 1) * c - sq);
    }
    double magDb (double f, double sr) const
    {
        const double w = juce::MathConstants<double>::twoPi * f / sr; const std::complex<double> z1c = std::polar (1.0, -w), z2c = z1c * z1c;
        return 20 * std::log10 (std::abs ((b0 + b1 * z1c + b2 * z2c) / (1.0 + a1 * z1c + a2 * z2c)) + 1e-12);
    }
};
// crossover Linkwitz-Riley 24 dB/ott
struct LR4
{
    Bq l1, l2, h1, h2;
    void set (double sr, double f) { l1.lp (sr, f); l2.lp (sr, f); h1.hp (sr, f); h2.hp (sr, f); }
    inline void split (float x, float& lo, float& hi) { lo = l2.p (l1.p (x)); hi = h2.p (h1.p (x)); }
    void reset() { l1.reset(); l2.reset(); h1.reset(); h2.reset(); }
};
// allpass del secondo ordine che riproduce la fase di un LR4 (per allineare le bande)
struct LRAp
{
    Bq a;
    void set (double sr, double f) { a.ap (sr, f, 0.7071); }
    inline float p (float x) { return a.p (x); }
    void reset() { a.reset(); }
};
}
