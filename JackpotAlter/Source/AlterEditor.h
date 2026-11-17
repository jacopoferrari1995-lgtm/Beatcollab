#pragma once
#include "AlterProcessor.h"
#include "Skin.h"
#include "JackpotScale.h"
#include "HwLook.h"
#include "PresetBrowser.h"

namespace ja
{
namespace C
{
    inline juce::Colour bg { 0xff0b0c10 }, text { 0xffdfe3ea }, dim { 0xff7d8494 }, faint { 0xff4f5562 };
    inline juce::Colour magenta { 0xffff2e88 }, phos { 0xff5ef2ff }, amber { 0xffffb000 }, green { 0xff7dff5a }, red { 0xffff3b5c }, violet { 0xffa66bff };
    inline juce::Colour lcd1 { 0xff05161a }, lcd2 { 0xff020b0d };
    inline void sync()
    {
        if (! jlook::skinHooks().on) return;
        const auto& p = jlook::skinPal();
        bg = p.bg; text = p.text; dim = p.dim; faint = p.faint; magenta = p.pink; phos = p.cyan; amber = p.amber; green = p.green; red = p.red; violet = p.violet; lcd1 = p.lcd0; lcd2 = p.lcd1;
    }
}
juce::Font uf (float size, bool bold = true);
juce::Font mf (float size);
void glowText (juce::Graphics&, const juce::String&, juce::Rectangle<float>, juce::Colour, juce::Font, juce::Justification = juce::Justification::centredLeft);
void lcd (juce::Graphics&, juce::Rectangle<float>, float rad);

class Editor;

// manopola da frontale: si ridisegna solo quando cambia valore, testo o stato
class Knob : public juce::Component
{
public:
    explicit Knob (juce::RangedAudioParameter* p) : param (p) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent&) override { if (drag) param->endChangeGesture(); drag = false; repaint(); }
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseDoubleClick (const juce::MouseEvent&) override { param->setValueNotifyingHost (param->getDefaultValue()); }
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w) override;
    void mouseEnter (const juce::MouseEvent&) override { hover = true; setMouseCursor (juce::MouseCursor::UpDownResizeCursor); repaint(); }
    void mouseExit (const juce::MouseEvent&) override { hover = false; repaint(); }
    void refresh (bool on) { const auto t = text ? text() : param->getCurrentValueAsText(); const float v = param->getValue(); if (t != lastT || v != lastV || on != lit) { lastT = t; lastV = v; lit = on; repaint(); } }
    juce::RangedAudioParameter* param; hw::KnobSpec spec; std::function<juce::String()> text; std::function<float (float, bool)> snap; bool lit = true;
private:
    float start = 0, lastV = -1; bool drag = false, hover = false; juce::String lastT;
};
class Segs : public juce::Component
{
public:
    Segs (juce::RangedAudioParameter* p, juce::Colour c, juce::StringArray it) : param (p), col (c), items (std::move (it)) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void refresh (bool on) { const int v = juce::roundToInt (param->convertFrom0to1 (param->getValue())); if (v != last || on != lit) { last = v; lit = on; repaint(); } }
    juce::RangedAudioParameter* param; juce::Colour col; juce::StringArray items; float font = 10.0f; bool lit = true; int last = -1; int cols = 0;
};
class Switch : public juce::Component
{
public:
    Switch (juce::RangedAudioParameter* p, juce::Colour c, juce::String l = {}) : param (p), col (c), label (std::move (l)) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { param->setValueNotifyingHost (param->getValue() > 0.5f ? 0.0f : 1.0f); repaint(); }
    void refresh() { const bool v = param->getValue() > 0.5f; if (v != last) { last = v; repaint(); } }
    juce::RangedAudioParameter* param; juce::Colour col, ink { 0xffe8e4d8 }; juce::String label; bool last = false;
};
// accordatore: nota che entra, nota che esce, correzione
class Tuner : public juce::Component
{
public:
    explicit Tuner (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    juce::String key() const;
    Editor& ed; juce::String lastKey;
};
// forma d'onda di una sinusoide passata nel modello scelto
class Curve : public juce::Component
{
public:
    explicit Curve (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    juce::String key() const;
    Editor& ed; juce::String lastKey;
};
// spettro d'uscita con le tre bande: i tagli si trascinano
class MbView : public juce::Component
{
public:
    explicit MbView (Editor& e) : ed (e) { spec.fill (-100.0f); }
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void update();
    float f2x (float f) const; float x2f (float x) const;
    Editor& ed; int dragK = -1; std::array<float, 1024> spec; std::vector<float> buf = std::vector<float> (4096, 0.0f); juce::dsp::FFT fft { 11 };
};

class Editor : public juce::AudioProcessorEditor, private juce::Timer
{
    skin::Keep skinKeep;
public:
    explicit Editor (Processor&);
    ~Editor() override { setLookAndFeel (nullptr); }
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void tickForTest() { for (int i = 0; i < 30; ++i) timerCallback(); }
    static constexpr int W = 1180, H = 700;
    static juce::Rectangle<float> pitchUnit() { return { 18, 76, 400, 418 }; }
    static juce::Rectangle<float> driveUnit() { return { 428, 76, 400, 418 }; }
    static juce::Rectangle<float> mbUnit() { return { 18, 504, 810, 180 }; }
    static juce::Rectangle<float> outCard() { return { 838, 76, 324, 608 }; }
    static juce::Rectangle<float> modelCell (int m) { auto u = driveUnit(); const float w = 368.0f / 7; return { u.getX() + 16 + (m % 7) * w, u.getY() + 44 + (m / 7) * 28.0f, w - 4, 24 }; }
    bool pitchOn() const { return std::abs (proc.value ("pitch")) > 0.004f || std::abs (proc.value ("formant")) > 0.004f || juce::roundToInt (proc.value ("mode")) != TRASPONI; }
    bool driveOn() const { return proc.value ("drive") > 0.0005f; }
    jlook::LnF lnf;
    Processor& proc;
    jlook::Scaler scaler { *this, "alter", W, H };
    jlook::SizeChip sizeChip { scaler };
    juce::RangedAudioParameter* P (const juce::String& id) { return proc.apvts.getParameter (id); }
    jbrowse::Browser browser;
private:
    void timerCallback() override;
    juce::String staticKey();
    void paintStatic (juce::Graphics&);
    void setupBrowser();
    void askPresetName();
    std::vector<std::unique_ptr<Knob>> knobs; std::vector<std::pair<Knob*, int>> knobUnit;   // unita': 0 pitch, 1 drive, 2 multibanda, -1 uscita
    std::unique_ptr<Segs> mode, keySeg, scaleSeg; std::unique_ptr<Switch> link, multi, autoG;
    Tuner tuner { *this }; Curve curve { *this }; MbView mb { *this };
    jlook::StaticLayer bg;
    juce::Rectangle<int> presetRect, prevRect, nextRect, abRect, themeRect; bool abHover = false; int tick = 0; int hovModel = -1;
};
}
