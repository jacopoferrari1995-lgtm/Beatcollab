#pragma once
#include "PhaseProcessor.h"

namespace jphs
{
using namespace jkit;
class Editor;
class WaveView : public juce::Component { public: explicit WaveView (Editor& e) : ed (e) {} void paint (juce::Graphics&) override; Editor& ed; };
class StatusView : public juce::Component { public: explicit StatusView (Editor& e) : ed (e) {} void paint (juce::Graphics&) override; Editor& ed; float shown = 0, shownB = 0; };

class Editor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit Editor (Processor&);
    ~Editor() override { setLookAndFeel (nullptr); }
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    static constexpr int W = 1000, H = 660;
    jlook::LnF lnf;
    Processor& proc;
    jlook::Scaler scaler { *this, "phase", W, H };
    jlook::SizeChip sizeChip { scaler };
    juce::RangedAudioParameter* P (const juce::String& id) { return proc.apvts.getParameter (id); }
    const juce::Colour acc = C::green, acc2 = C::violet;
    static juce::Colour corrCol (float r) { return r > 0.2f ? C::green : r > -0.05f ? C::yellow : C::red; }
private:
    void timerCallback() override;
    std::vector<std::unique_ptr<Knob>> knobs; Knob *kAngle = nullptr, *kSpeed = nullptr, *kAmount = nullptr, *kFreq = nullptr;
    std::vector<std::unique_ptr<Pill>> modes;
    std::unique_ptr<Switch> listen;
    WaveView wave { *this }; StatusView status { *this };
    PresetBar bar; int tick = 0, lastSerial = -1;
};
}
