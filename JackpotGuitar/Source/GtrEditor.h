#pragma once
#include "GtrProcessor.h"
#include "JackpotScale.h"
#include "Skin.h"
#include "JackpotKit.h"
#include "PresetBrowser.h"

namespace gtr
{
using namespace jkit;
inline juce::String U (const char* s) { return juce::String::fromUTF8 (s); }

class Editor : public juce::AudioProcessorEditor, public juce::DragAndDropContainer, private juce::Timer
{
    skin::Keep skinKeep;
public:
    explicit Editor (Processor&);
    ~Editor() override { stopTimer(); removeMouseListener (&undoMark); }
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override { hoverSlot = -1; hoverId = -1; }
    bool keyPressed (const juce::KeyPress&) override;
    void parentHierarchyChanged() override { jlook::preferFastRenderer (*this); }
    static constexpr int W = 1180, H = 834;
    juce::SharedResourcePointer<jlook::Resources> keepLook; neo::KeepFonts keepFonts;
    Processor& proc;
    jlook::Scaler scaler { *this, "guitar", W, H };
    jlook::SizeChip sizeChip { scaler };
    juce::OwnedArray<jkit::Knob> knobs;
    jbrowse::Browser browser;
    // zone cliccabili
    juce::Rectangle<float> keyR[12], scaleR[kScales], genreR, moodR, barsR[4], perBarR[3], slotR[8], addR, remR;
    juce::Rectangle<float> fretR, vstyleR[kVStyles], tuningR;
    juce::Rectangle<float> modeR[kModes], patR[9], rollR, dragR, genR, varR, playR, liveR, midiR, csR;
    juce::Rectangle<float> bodyR[kBodies], listenR;
    juce::Rectangle<float> presetR, prevR, nextR, undoR, redoR, themeR;
    int hoverSlot = -1, selSlot = 0, hoverId = -1, bgTheme = -1; bool dragging = false; juce::String info;
    juce::Image bg; float bgScale = 0; void paintStatic (juce::Graphics&);
    void setupBrowser(); void askPresetName(); void doUndo (bool redo); void slotMenu (int i); void genreMenu(); void moodMenu(); void repaintAll();
    void drawFretboard (juce::Graphics&, const Part&, int slot);
    void drawRoll (juce::Graphics&, const Part&);
    struct UndoMark : juce::MouseListener
    {
        Processor* p = nullptr; double lastWheel = 0;
        void mouseDown (const juce::MouseEvent&) override { if (p) p->undo.beginNewTransaction(); }
        void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override { const double t = juce::Time::getMillisecondCounterHiRes(); if (p && t - lastWheel > 600) p->undo.beginNewTransaction(); lastWheel = t; }
    } undoMark;
private:
    void timerCallback() override;
};
}
