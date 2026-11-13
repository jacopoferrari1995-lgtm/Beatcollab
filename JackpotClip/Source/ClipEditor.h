#pragma once
#include "ClipProcessor.h"
#include "Skin.h"

namespace jclp
{
using namespace jkit;
class Editor;
class WaveView : public juce::Component { public: explicit WaveView (Editor& e) : ed (e) {} void paint (juce::Graphics&) override; Editor& ed; };
class CurveView : public juce::Component { public: explicit CurveView (Editor& e) : ed (e) {} void paint (juce::Graphics&) override; Editor& ed; };

class Editor : public juce::AudioProcessorEditor, private juce::Timer
{
    juce::Rectangle<int> themeRect;
public:
    explicit Editor (Processor&);
    ~Editor() override { setLookAndFeel (nullptr); }
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    static constexpr int W = 1060, H = 680;
    jlook::LnF lnf;
    Processor& proc;
    jlook::Scaler scaler { *this, "clip", W, H };
    jlook::SizeChip sizeChip { scaler };
    juce::RangedAudioParameter* P (const juce::String& id) { return proc.apvts.getParameter (id); }
    const juce::Colour acc = C::red, acc2 = C::amber;
private:
    void timerCallback() override;
    std::vector<std::unique_ptr<Knob>> knobs; Knob* kSplit = nullptr;
    std::unique_ptr<Segs> modeSeg, osSeg;
    std::vector<std::unique_ptr<Switch>> switches;
    WaveView wave { *this }; CurveView curve { *this };
    PresetBar bar; int tick = 0;
};
}
