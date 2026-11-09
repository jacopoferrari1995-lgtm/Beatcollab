#pragma once
#include "CompProcessor.h"

namespace jc
{
namespace C
{
    const juce::Colour bg { 0xff0b0c10 }, text { 0xffdfe3ea }, dim { 0xff7d8494 }, faint { 0xff4f5562 };
    const juce::Colour magenta { 0xffff2e88 }, phos { 0xff5ef2ff }, amber { 0xffffb000 }, green { 0xff7dff5a }, red { 0xffff3b5c };
    const juce::Colour lcd1 { 0xff05161a }, lcd2 { 0xff020b0d };
}
juce::Font uf (float size, bool bold = true);
juce::Font mf (float size);
void glowText (juce::Graphics&, const juce::String&, juce::Rectangle<float>, juce::Colour, juce::Font, juce::Justification = juce::Justification::centredLeft);
void lcd (juce::Graphics&, juce::Rectangle<float>, float rad);

class Editor;

class Knob : public juce::Component
{
public:
    Knob (juce::RangedAudioParameter* p, juce::Colour c) : param (p), col (c) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent&) override { if (drag) param->endChangeGesture(); drag = false; repaint(); }
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseDoubleClick (const juce::MouseEvent&) override { if (enabled) param->setValueNotifyingHost (param->getDefaultValue()); }
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w) override { if (enabled) param->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, param->getValue() + (w.deltaY > 0 ? 0.01f : -0.01f))); }
    void mouseEnter (const juce::MouseEvent&) override { hover = true; setMouseCursor (juce::MouseCursor::UpDownResizeCursor); repaint(); }
    void mouseExit (const juce::MouseEvent&) override { hover = false; repaint(); }
    juce::RangedAudioParameter* param; juce::Colour col; juce::String label;
    std::function<juce::String()> text; std::function<void()> onTouch;
    bool enabled = true, big = false, bipolar = false, stepped = false; int steps = 0;
private:
    float start = 0; bool drag = false, hover = false;
};

class Segs : public juce::Component
{
public:
    Segs (juce::RangedAudioParameter* p, juce::Colour c) : param (p), col (c) {}
    std::function<juce::StringArray()> items; std::function<void()> onTouch;
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    juce::RangedAudioParameter* param; juce::Colour col;
};

class Switch : public juce::Component
{
public:
    Switch (juce::RangedAudioParameter* p, juce::Colour c, juce::String l = {}) : param (p), col (c), label (std::move (l)) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { param->setValueNotifyingHost (param->getValue() > 0.5f ? 0.0f : 1.0f); if (onTouch) onTouch(); }
    juce::RangedAudioParameter* param; juce::Colour col; juce::String label; std::function<void()> onTouch;
};

class StageCard : public juce::Component
{
public:
    StageCard (Editor&, int s);
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void refresh();   // etichette che dipendono dal modello
    Editor& ed; int s; juce::String pfx;
    std::unique_ptr<Switch> power; std::unique_ptr<Segs> ratio;
    std::unique_ptr<Knob> drive, att, rel, out, color, mix;
    float needle = 0; int lastModel = -1;
    juce::Rectangle<float> vuRect() const { return { 16, 60, (float) getWidth() - 32, 100 }; }
    juce::Rectangle<int> modelRect() const { return { getWidth() - 168, 9, 154, 26 }; }
};

class Graph : public juce::Component
{
public:
    Graph (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    Editor& ed;
};

class GuidePanel : public juce::Component
{
public:
    GuidePanel (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    Editor& ed; int tab = 0; juce::StringArray tips; int warn = 0;
};

class Editor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit Editor (Processor&);
    ~Editor() override { setLookAndFeel (nullptr); }
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    static constexpr int W = 1240, H = 790;
    jlook::LnF lnf;
    Processor& proc;
    int focus = 0;
    juce::RangedAudioParameter* P (const juce::String& id) { return proc.apvts.getParameter (id); }
    void computeTips();
private:
    void timerCallback() override;
    StageCard a { *this, 0 }, b { *this, 1 };
    Graph graph { *this };
    GuidePanel guide { *this };
    std::unique_ptr<Segs> route, scSrc, scTo;
    std::unique_ptr<Knob> scHpf, gMix, gOut;
    std::unique_ptr<Switch> listen, autoG;
    juce::Rectangle<int> presetRect, prevRect, nextRect, abRect; bool abHover = false;
    float vIn = 0, vOut = 0; int tick = 0;
};
}
