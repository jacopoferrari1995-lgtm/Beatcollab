#pragma once
#include "S8Processor.h"
#include "JackpotScale.h"
#include "CyberUI.h"

namespace s8
{
using namespace cyber;
class Editor;

struct Diag { int sev; juce::String title, text; };   // sev 2 = da fare, 1 = consigliato, 0 = ok

class TunerView : public juce::Component { public: explicit TunerView (Editor& e) : ed (e) {} void paint (juce::Graphics&) override; Editor& ed; float hzS = 0, centS = 0; };
class SpecView : public juce::Component
{
public:
    explicit SpecView (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    void compute();
    Editor& ed; static constexpr int ORD = 13, NF = 1 << ORD;
    juce::dsp::FFT fft { ORD }; std::vector<float> buf = std::vector<float> (NF * 2), win = std::vector<float> (NF), spm = std::vector<float> (NF / 2, -120.0f);
};
class DocView : public juce::Component { public: explicit DocView (Editor& e) : ed (e) {} void paint (juce::Graphics&) override; Editor& ed; };

class Editor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit Editor (Processor&);
    ~Editor() override { stopTimer(); }
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void parentHierarchyChanged() override { jlook::preferFastRenderer (*this); }
    static constexpr int W = 1180, H = 720;
    juce::SharedResourcePointer<jlook::Resources> keepLook; neo::KeepFonts keepFonts;
    Processor& proc;
    jlook::Scaler scaler { *this, "808studio", W, H };
    jlook::SizeChip sizeChip { scaler };
    TunerView tuner { *this }; SpecView spec { *this }; DocView doc { *this };
    juce::OwnedArray<CyberKnob> knobs;
    juce::Rectangle<float> modeR[4], invR, resetR, prevR, nextR, presetR;
    std::vector<Diag> diags; void diagnose();
    void presetMenu();
    juce::Image bg; float bgScale = 0; void paintStatic (juce::Graphics&);
    int frame = 0, glitchT = 0; float glitchOff = 0;
private:
    void timerCallback() override;
};
}
