#pragma once
#include "VrProcessor.h"
#include "Skin.h"
#include "JackpotScale.h"
#include "Neo.h"
#include "PresetBrowser.h"

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
    skin::Keep skinKeep;
public:
    explicit Editor (Processor&);
    ~Editor() override { stopTimer(); removeMouseListener (&undoMark); }
    bool keyPressed (const juce::KeyPress&) override;
    void mouseMove (const juce::MouseEvent&) override;
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
    juce::Rectangle<float> undoR, redoR, themeR; int hoverTop = -1, bgTheme = -1;
    jbrowse::Browser browser;
    void setupBrowser(); void askPresetName(); void doUndo (bool redo);
    struct UndoMark : juce::MouseListener
    {
        Processor* p = nullptr; double lastWheel = 0;
        void mouseDown (const juce::MouseEvent&) override { if (p) p->undo.beginNewTransaction(); }
        void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override { const double t = juce::Time::getMillisecondCounterHiRes(); if (p && t - lastWheel > 600) p->undo.beginNewTransaction(); lastWheel = t; }
    } undoMark;
    juce::String liveSentence() const;
    juce::Image bg; float bgScale = 0; void paintStatic (juce::Graphics&);
private:
    void timerCallback() override;
};
}
