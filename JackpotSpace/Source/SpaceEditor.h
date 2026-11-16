#pragma once
#include "SpaceProcessor.h"
#include "Skin.h"
#include "JackpotScale.h"
#include "Neo.h"

namespace jsp
{
class Editor;

// display: il bagnato che scorre sopra il contorno del pulito, e lo schema della catena (serie e parallelo)
class Scope : public juce::Component
{
public:
    explicit Scope (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    Editor& ed;
};

class SlotCard : public juce::Component
{
public:
    SlotCard (Editor& e, int k);
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void refresh();
    void tick();
    void showMenu();
    Editor& ed; const int k;
    std::unique_ptr<neo::Knob> knob, k2, mixK;
    juce::Rectangle<float> nameR, powR, headR, gearR, routeR, meterR;
    int shownType = -1; bool dragging = false, dropHot = false; float lvlShown = 0;
};

class AdvPanel : public juce::Component
{
public:
    explicit AdvPanel (Editor& e) : ed (e) {}
    void open (int k);
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void tick();
    Editor& ed; int k = -1, shownType = -1;
    juce::OwnedArray<neo::Knob> knobs;
    juce::Rectangle<float> closeR, resetR;
};

class Editor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit Editor (Processor&);
    ~Editor() override { stopTimer(); }
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    static constexpr int W = 1200, H = 740;
    juce::SharedResourcePointer<jlook::Resources> keepLook; neo::KeepFonts keepFonts;
    void parentHierarchyChanged() override { jlook::preferFastRenderer (*this); }
    Processor& proc;
    jlook::Scaler scaler { *this, "space", W, H };
    jlook::SizeChip sizeChip { scaler };
    Scope scope { *this };
    AdvPanel advPanel { *this };
    juce::OwnedArray<SlotCard> cards;
    std::unique_ptr<neo::Knob> dryK, wetK, duckK, outK;
    juce::Rectangle<int> presetR, prevR, nextR, saveR, bypR, frzR, themeR;
    int dragFrom = -1; juce::Point<float> dragPos;
    void endDrag (juce::Point<float> editorPos);
    void presetMenu();
    void refreshAll() { for (auto* c : cards) c->refresh(); if (advPanel.isVisible()) advPanel.open (advPanel.k); repaint(); }
    std::unique_ptr<juce::FileChooser> chooser;
    juce::Image bg; float bgScale = 0; int bgTheme = -1;
    void paintOverChildren (juce::Graphics&) override { neo::setTheme (0); }
    void paintStatic (juce::Graphics&);
    static float cardW() { return (W - 48 - (kSlots - 1) * 10) / (float) kSlots; }
private:
    void timerCallback() override;
};
}
