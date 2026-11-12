#pragma once
#include "FxProcessor.h"
#include "JackpotScale.h"
#include "Neo.h"

namespace jfx
{
class Editor;

// display centrale: onda prima/dopo che scorre, effetti in azione
class Scope : public juce::Component
{
public:
    explicit Scope (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    Editor& ed;
};

// una scheda per slot: nome dell'effetto (menu), manopola 3D, acceso/spento, controller
class SlotCard : public juce::Component
{
public:
    SlotCard (Editor& e, int k);
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void refresh();                 // ricollega la manopola all'effetto scelto
    void tick();
    void showMenu();
    Editor& ed; const int k;
    std::unique_ptr<neo::Knob> knob;
    juce::Rectangle<float> nameR, powR, ccR, headR;
    int shownType = -1, lastSeen = 0; float flash = 0; bool dragging = false, dropHot = false;
};

class Editor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit Editor (Processor&);
    ~Editor() override { stopTimer(); }
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    static constexpr int W = 1200, H = 690;
    Processor& proc;
    jlook::Scaler scaler { *this, "fx", W, H };
    jlook::SizeChip sizeChip { scaler };
    Scope scope { *this };
    juce::OwnedArray<SlotCard> cards;
    std::unique_ptr<neo::Knob> mixK, outK;
    juce::Rectangle<int> presetR, prevR, nextR, saveR, resetR, bypR, menuR, ctrlR;
    int dragFrom = -1; juce::Point<float> dragPos;
    void endDrag (juce::Point<float> editorPos);
    void presetMenu();
    std::unique_ptr<juce::FileChooser> chooser;
    juce::Image bg; float bgScale = 0;
    void paintStatic (juce::Graphics&);
private:
    void timerCallback() override;
};
}
