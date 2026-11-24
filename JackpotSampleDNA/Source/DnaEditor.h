#pragma once
#include "DnaProcessor.h"
#include "Skin.h"
#include "JackpotScale.h"
#include "Neo.h"

namespace dna
{
class Editor;
class WaveView : public juce::Component
{
public:
    explicit WaveView (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    Editor& ed;
};
class PadView : public juce::Component
{
public:
    explicit PadView (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    Editor& ed; int hit = -1;
};

class Editor : public juce::AudioProcessorEditor, public juce::FileDragAndDropTarget, public juce::DragAndDropContainer, private juce::Timer
{
    skin::Keep skinKeep;
public:
    explicit Editor (Processor&);
    ~Editor() override { stopTimer(); }
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    bool isInterestedInFileDrag (const juce::StringArray& f) override { for (auto& x : f) if (juce::File (x).hasFileExtension ("wav;aif;aiff;flac;mp3;m4a;ogg")) return true; return false; }
    void filesDropped (const juce::StringArray& f, int, int) override { for (auto& x : f) if (proc.loadFile (juce::File (x))) break; repaint(); }
    void parentHierarchyChanged() override { jlook::preferFastRenderer (*this); }
    static constexpr int W = 1100, H = 720;
    juce::SharedResourcePointer<jlook::Resources> keepLook; neo::KeepFonts keepFonts;
    Processor& proc;
    jlook::Scaler scaler { *this, "sampledna", W, H };
    jlook::SizeChip sizeChip { scaler };
    WaveView wave { *this }; PadView pads { *this };
    juce::OwnedArray<neo::Knob> knobs;
    juce::Rectangle<float> themeR, loadR, halfR, dblR, modeR[kModes], varR, playR[2], chokeR, midiR;
    std::unique_ptr<juce::FileChooser> chooser; bool dragging = false;
    static juce::Colour pcCol (int pc, float bright = 1.0f) { return juce::Colour::fromHSV ((float) (((pc % 12) + 12) % 12) / 12.0f, 0.62f, 0.55f + 0.4f * bright, 1.0f); }
    juce::Image bg; float bgScale = 0; int bgTheme = -1; void paintStatic (juce::Graphics&);
private:
    void timerCallback() override;
};
}
