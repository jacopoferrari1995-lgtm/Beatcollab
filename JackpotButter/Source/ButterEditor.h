#pragma once
#include "ButterProcessor.h"

namespace jb
{
namespace BC
{
    const juce::Colour cream { 0xfff4ecd6 }, cream2 { 0xffe8dcbc }, ink { 0xff2a241d }, inkSoft { 0xff6f6454 }, line { 0xffcdbf9c };
    const juce::Colour mod[6] = { juce::Colour (0xffe8794a), juce::Colour (0xff8f7ae6), juce::Colour (0xffe2b93b), juce::Colour (0xffe05a5a), juce::Colour (0xff4fb3c9), juce::Colour (0xff7cbf5a) };
}
juce::Font bf (float size, bool bold = false);

class Editor;

// manopola legata a un parametro: grande (amount) o piccola
class PKnob : public juce::Component
{
public:
    PKnob (juce::RangedAudioParameter* p, juce::String l, juce::Colour c, bool big, std::function<juce::String (float)> f = {}) : param (p), label (std::move (l)), col (c), isBig (big), fmt (std::move (f)) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { start = param->getValue(); param->beginChangeGesture(); }
    void mouseUp (const juce::MouseEvent&) override { param->endChangeGesture(); }
    void mouseDrag (const juce::MouseEvent& e) override { param->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, start - e.getDistanceFromDragStartY() / (e.mods.isShiftDown() ? 900.0f : 200.0f))); }
    void mouseDoubleClick (const juce::MouseEvent&) override { param->setValueNotifyingHost (param->getDefaultValue()); }
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w) override { param->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, param->getValue() + (w.deltaY > 0 ? 0.01f : -0.01f))); }
    juce::RangedAudioParameter* param; juce::String label; juce::Colour col; bool isBig; std::function<juce::String (float)> fmt; float start = 0;
};

// pillola di scelta (clic: menu)
class Pill : public juce::Component
{
public:
    Pill (juce::AudioParameterChoice* p, juce::Colour c) : param (p), col (c) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    juce::AudioParameterChoice* param; juce::Colour col;
};

// segmenti FOCUS (FULL / LOW / MID / HIGH)
class Segs : public juce::Component
{
public:
    Segs (juce::AudioParameterChoice* p, juce::Colour c) : param (p), col (c) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent& e) override { const int n = param->choices.size(); const int i = juce::jlimit (0, n - 1, (int) (e.position.x / getWidth() * n)); param->setValueNotifyingHost (param->convertTo0to1 ((float) i)); }
    juce::AudioParameterChoice* param; juce::Colour col;
};

// slider TARGET: sustain <-> transienti
class Target : public juce::Component
{
public:
    Target (juce::RangedAudioParameter* p, juce::Colour c) : param (p), col (c) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent& e) override { param->beginChangeGesture(); mouseDrag (e); }
    void mouseUp (const juce::MouseEvent&) override { param->endChangeGesture(); }
    void mouseDrag (const juce::MouseEvent& e) override { param->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, (e.position.x - 8) / (getWidth() - 16))); }
    void mouseDoubleClick (const juce::MouseEvent&) override { param->setValueNotifyingHost (0.5f); }
    juce::RangedAudioParameter* param; juce::Colour col;
};

// interruttore
class Switch : public juce::Component
{
public:
    Switch (juce::RangedAudioParameter* p, juce::String l, juce::Colour c) : param (p), label (std::move (l)), col (c) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { param->setValueNotifyingHost (param->getValue() > 0.5f ? 0.0f : 1.0f); }
    juce::RangedAudioParameter* param; juce::String label; juce::Colour col;
};

// scheda di un modulo
class Module : public juce::Component
{
public:
    Module (Editor& e, int m);
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    Editor& ed; int mod; bool dragging = false; int dragX = 0;
    std::unique_ptr<PKnob> big; juce::OwnedArray<PKnob> smalls; std::unique_ptr<Pill> type; std::unique_ptr<Segs> focus; std::unique_ptr<Target> target;
    std::unique_ptr<Switch> power, extra; std::unique_ptr<Pill> extraPill;
    juce::Rectangle<int> lockRect() const { return { getWidth() - 30, 10, 18, 18 }; }
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
    void moduleDragged (Module& m, int x);
    void moduleDropped (Module& m);
    static constexpr int W = 1180, H = 640;
    Processor& proc;
    juce::OwnedArray<Module> modules;
    juce::OwnedArray<PKnob> master;
    std::unique_ptr<Switch> clip;
    juce::Rectangle<int> presetRect, prevRect, nextRect;
private:
    void timerCallback() override;
};
}
