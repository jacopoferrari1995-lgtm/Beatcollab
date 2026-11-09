#pragma once
#include "ButterProcessor.h"
#include <juce_dsp/juce_dsp.h>

namespace jb
{
namespace BC
{
    const juce::Colour bg { 0xff0b0c10 }, panel1 { 0xff1d2027 }, panel2 { 0xff121419 }, lcd1 { 0xff05161a }, lcd2 { 0xff020b0d };
    const juce::Colour text { 0xffdfe3ea }, dim { 0xff7d8494 }, faint { 0xff474d59 }, phos { 0xff5ef2ff }, amber { 0xffffb000 }, magenta { 0xffff2e88 };
    const juce::Colour mod[7] = { juce::Colour (0xffff6a3d), juce::Colour (0xffa66bff), juce::Colour (0xffffd23f), juce::Colour (0xffff3b5c), juce::Colour (0xff2de2e6), juce::Colour (0xff7dff5a), juce::Colour (0xffff4fd8) };
}
juce::Font bf (float size, bool bold = false, bool mono = false);

class Editor;

class PKnob : public juce::Component
{
public:
    PKnob (juce::RangedAudioParameter* p, juce::String l, juce::Colour c, bool big, std::function<juce::String (float)> f = {}) : param (p), label (std::move (l)), col (c), isBig (big), fmt (std::move (f)) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { start = param->getValue(); param->beginChangeGesture(); active = true; }
    void mouseUp (const juce::MouseEvent&) override { param->endChangeGesture(); active = false; repaint(); }
    void mouseDrag (const juce::MouseEvent& e) override { param->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, start - e.getDistanceFromDragStartY() / (e.mods.isShiftDown() ? 900.0f : 200.0f))); }
    void mouseDoubleClick (const juce::MouseEvent&) override { param->setValueNotifyingHost (param->getDefaultValue()); }
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w) override { param->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, param->getValue() + (w.deltaY > 0 ? 0.01f : -0.01f))); }
    juce::RangedAudioParameter* param; juce::String label; juce::Colour col; bool isBig; std::function<juce::String (float)> fmt; float start = 0; bool active = false; bool bipolar = false;
};

class Pill : public juce::Component
{
public:
    Pill (juce::AudioParameterChoice* p, juce::Colour c) : param (p), col (c) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    juce::AudioParameterChoice* param; juce::Colour col;
};

class Segs : public juce::Component
{
public:
    Segs (juce::AudioParameterChoice* p, juce::Colour c) : param (p), col (c) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent& e) override { const int n = param->choices.size(); param->setValueNotifyingHost (param->convertTo0to1 ((float) juce::jlimit (0, n - 1, (int) (e.position.x / getWidth() * n)))); }
    juce::AudioParameterChoice* param; juce::Colour col;
};

class Target : public juce::Component
{
public:
    Target (juce::RangedAudioParameter* p, juce::Colour c) : param (p), col (c) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent& e) override { param->beginChangeGesture(); mouseDrag (e); }
    void mouseUp (const juce::MouseEvent&) override { param->endChangeGesture(); }
    void mouseDrag (const juce::MouseEvent& e) override { param->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, (e.position.x - 10) / (getWidth() - 20))); }
    void mouseDoubleClick (const juce::MouseEvent&) override { param->setValueNotifyingHost (0.5f); }
    juce::RangedAudioParameter* param; juce::Colour col;
};

class Switch : public juce::Component
{
public:
    Switch (juce::RangedAudioParameter* p, juce::String l, juce::Colour c) : param (p), label (std::move (l)), col (c) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { param->setValueNotifyingHost (param->getValue() > 0.5f ? 0.0f : 1.0f); }
    juce::RangedAudioParameter* param; juce::String label; juce::Colour col;
};

class Module : public juce::Component
{
public:
    Module (Editor& e, int m);
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void drawDisplay (juce::Graphics&, juce::Rectangle<float>);
    Editor& ed; int mod; bool dragging = false; int dragX = 0;
    std::unique_ptr<PKnob> big; juce::OwnedArray<PKnob> smalls; std::unique_ptr<Pill> type, extraPill; std::unique_ptr<Segs> focus; std::unique_ptr<Target> target;
    std::unique_ptr<Switch> power, extra;
    juce::Rectangle<int> lockRect() const { return { getWidth() - 28, 9, 18, 18 }; }
    juce::Rectangle<float> displayRect() const { return { 8, 40, getWidth() - 16.0f, 80 }; }
};

// EQ grafico con analizzatore di spettro
class EqPanel : public juce::Component
{
public:
    explicit EqPanel (Editor& e);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void updateSpectrum();
    juce::Rectangle<float> plot() const { return getLocalBounds().toFloat().reduced (14).withTrimmedTop (26).withTrimmedBottom (4); }
    int nodeAt (juce::Point<float>) const;
    Editor& ed; int dragNode = -1;
    juce::dsp::FFT fft { 11 };
    std::vector<float> fftBuf, specIn, specOut;
    std::unique_ptr<Switch> on;
};

class Editor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit Editor (Processor&);
    ~Editor() override = default;
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void layoutModules();
    void moduleDropped (Module& m);
    static constexpr int W = 1386, H = 820, MX = 1130;
    Processor& proc;
    juce::OwnedArray<Module> modules;
    juce::OwnedArray<PKnob> master;
    std::unique_ptr<Switch> clip;
    std::unique_ptr<EqPanel> eq;
    juce::Rectangle<int> presetRect, prevRect, nextRect, randRect;
private:
    void timerCallback() override;
};
}
