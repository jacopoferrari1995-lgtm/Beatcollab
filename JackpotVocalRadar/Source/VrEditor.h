#pragma once
#include "VrProcessor.h"
#include "Skin.h"
#include "JackpotScale.h"
#include "Neo.h"

namespace vr
{
class Editor;

// cosa sta facendo: spettro di voce e beat, e la curva di quanto il beat si abbassa
class SpaceView : public juce::Component
{
public:
    explicit SpaceView (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    Editor& ed; float smV[NB] {}, smB[NB] {}, smC[NB] {};
};
// voce e beat nel tempo, su due corsie
class LaneView : public juce::Component
{
public:
    explicit LaneView (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    Editor& ed;
};

class Editor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit Editor (Processor&);
    ~Editor() override { stopTimer(); }
    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void parentHierarchyChanged() override { jlook::preferFastRenderer (*this); }
    static constexpr int W = 1100, H = 770;
    juce::SharedResourcePointer<jlook::Resources> keepLook; neo::KeepFonts keepFonts;
    Processor& proc;
    jlook::Scaler scaler { *this, "vocalradar", W, H };
    jlook::SizeChip sizeChip { scaler };
    SpaceView space { *this }; LaneView lanes { *this };
    juce::OwnedArray<neo::Knob> knobs; juce::StringArray tips;
    juce::Rectangle<float> roleR[2], chanR[NCH], srcR[2], deltaR, bypR, clickR, applyR, discardR, prevR, nextR, presetR;
    int hoverKnob = -1, frame = 0;
    void presetMenu();
    juce::String liveSentence() const;
    juce::Image bg; float bgScale = 0; void paintStatic (juce::Graphics&);
private:
    void timerCallback() override;
};
}
