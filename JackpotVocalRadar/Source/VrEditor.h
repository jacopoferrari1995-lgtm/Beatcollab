#pragma once
#include "VrProcessor.h"
#include "JackpotScale.h"
#include "Neo.h"

namespace vr
{
namespace cp   // palette cyberpunk
{
const juce::Colour bg0 (0xff05040c), bg1 (0xff140626), mag (0xffff2e88), cya (0xff21f3ff), yel (0xfff4ff5e), pur (0xff8a3bff), red (0xffff3355), ink (0xffeef0ff), dim (0xff7d7aa6), panel (0xe60a0816);
}

class Editor;

class CyberKnob : public juce::Slider
{
public:
    CyberKnob (juce::RangedAudioParameter* p, juce::String label, juce::Colour c, bool big = false);
    void paint (juce::Graphics&) override;
    std::function<juce::String (float)> fmt;
    juce::String label; juce::Colour col; bool big; bool dimmed = false;
    std::unique_ptr<juce::SliderParameterAttachment> att;
};

class Radar : public juce::Component
{
public:
    explicit Radar (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    Editor& ed; float sweep = 0; float smV[NB] {}, smB[NB] {}, smC[NB] {};
};

class Timeline : public juce::Component
{
public:
    explicit Timeline (Editor& e) : ed (e) {}
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
    static constexpr int W = 1180, H = 720;
    juce::SharedResourcePointer<jlook::Resources> keepLook; neo::KeepFonts keepFonts;
    Processor& proc;
    jlook::Scaler scaler { *this, "vocalradar", W, H };
    jlook::SizeChip sizeChip { scaler };
    Radar radar { *this }; Timeline timeline { *this };
    juce::OwnedArray<CyberKnob> knobs; CyberKnob* amountK = nullptr; CyberKnob* sensK = nullptr;
    juce::Rectangle<float> modeR[2], chanR[4], srcR[2], deltaR, clickR, applyR, discardR, prevR, nextR, presetR;
    void presetMenu();
    juce::Image bg; float bgScale = 0;
    void paintStatic (juce::Graphics&);
    int glitchT = 0; float glitchOff = 0; int frame = 0;
private:
    void timerCallback() override;
};

// font
inline juce::Font fTitle (float s) { return juce::Font (juce::FontOptions (neo::tf (4)).withHeight (s)); }
inline juce::Font fLab (float s) { return juce::Font (juce::FontOptions (neo::tf (9)).withHeight (s)).withExtraKerningFactor (0.10f); }
inline juce::Font fMono (float s) { return juce::Font (juce::FontOptions (neo::tf (7)).withHeight (s)); }
inline juce::Font fWide (float s) { return juce::Font (juce::FontOptions (neo::tf (8)).withHeight (s)).withExtraKerningFactor (0.12f); }
}
