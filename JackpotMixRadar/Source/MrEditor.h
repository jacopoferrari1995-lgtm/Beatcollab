#pragma once
#include "MrProcessor.h"
#include "Skin.h"
#include "JackpotScale.h"
#include "Neo.h"

namespace mr
{
class Editor : public juce::AudioProcessorEditor, private juce::Timer
{
    skin::Keep skinKeep;
public:
    explicit Editor (Processor&);
    ~Editor() override { stopTimer(); }
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void parentHierarchyChanged() override { jlook::preferFastRenderer (*this); }
    static constexpr int W = 1180, H = 760;
    juce::SharedResourcePointer<jlook::Resources> keepLook; neo::KeepFonts keepFonts;
    Processor& proc;
    jlook::Scaler scaler { *this, "mixradar", W, H };
    jlook::SizeChip sizeChip { scaler };
    juce::Rectangle<float> modeR[2], roleR[kRoles], clearR, themeR; bool hoverTheme = false;
    void mouseMove (const juce::MouseEvent&) override;
    float smB[MAXS][NB] {}; float smM[NB] {};
    static juce::Colour roleCol (int r);
    juce::Image bg; float bgScale = 0; int bgMode = -1, bgTheme = -1; void paintStatic (juce::Graphics&);
    void paintRadar (juce::Graphics&); void paintSensor (juce::Graphics&);
private:
    void timerCallback() override;
};
}
