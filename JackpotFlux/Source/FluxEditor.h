#pragma once
#include "FluxProcessor.h"
#include "Skin.h"
#include "JackpotScale.h"
#include "Neo.h"

namespace flux
{
class Editor : public juce::AudioProcessorEditor, public juce::FileDragAndDropTarget, public juce::DragAndDropContainer, private juce::Timer
{
    skin::Keep skinKeep;
public:
    explicit Editor (Processor&);
    ~Editor() override { stopTimer(); removeMouseListener (&undoMark); }
    bool keyPressed (const juce::KeyPress& k) override
    {
        if (k.getModifiers().isCommandDown() && (k.getKeyCode() == 'Z' || k.getKeyCode() == 'z')) { proc.undo.beginNewTransaction(); if (k.getModifiers().isShiftDown()) proc.undo.redo(); else proc.undo.undo(); repaint(); return true; }
        return false;
    }
    struct UndoMark : juce::MouseListener
    {
        Processor* p = nullptr; double lastWheel = 0;
        void mouseDown (const juce::MouseEvent&) override { if (p) p->undo.beginNewTransaction(); }
        void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override { const double t = juce::Time::getMillisecondCounterHiRes(); if (p && t - lastWheel > 600) p->undo.beginNewTransaction(); lastWheel = t; }
    } undoMark;
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    bool isInterestedInFileDrag (const juce::StringArray& f) override { for (auto& x : f) if (juce::File (x).hasFileExtension ("mid;midi")) return true; return false; }
    void filesDropped (const juce::StringArray& f, int x, int y) override;
    void parentHierarchyChanged() override { jlook::preferFastRenderer (*this); }
    static constexpr int W = 1180, H = 760;
    juce::SharedResourcePointer<jlook::Resources> keepLook; neo::KeepFonts keepFonts;
    Processor& proc;
    jlook::Scaler scaler { *this, "flux", W, H };
    jlook::SizeChip sizeChip { scaler };
    juce::OwnedArray<neo::Knob> knobs;
    juce::Rectangle<float> themeR, starterR[kStarters], keyR, playR, varR, abR, midiR, laneR[NL], muteR[NL], varLR[NL], secR[NSEC], memR[8], addR, drumR;
    struct RuleUI { juce::Rectangle<float> on, from, to, type, del; }; std::vector<RuleUI> ruleUI;
    bool dragging = false; int flash = 0; juce::String info;
    static juce::Colour laneCol (int l);
    juce::Image bg; float bgScale = 0; int bgTheme = -1; void paintStatic (juce::Graphics&);
private:
    void timerCallback() override;
};
}
