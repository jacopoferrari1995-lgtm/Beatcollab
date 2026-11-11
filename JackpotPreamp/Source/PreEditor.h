#pragma once
#include "PreProcessor.h"
#include "AnalogLook.h"

namespace jpre
{
class Editor;
class Graph : public juce::Component
{
public:
    explicit Graph (Editor& e);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override { drag = -1; }
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void tick();
    float xOf (float f) const; float fOf (float x) const; float yOf (float db) const; float dbOf (float y) const;
    int hit (juce::Point<float>) const;
    juce::Point<float> pointOf (int k) const;
    Editor& ed; int drag = -1, hover = -1;
    juce::dsp::FFT fft { 12 }; std::vector<float> win, buf, spec;
};

class Editor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit Editor (Processor&);
    ~Editor() override { setLookAndFeel (nullptr); }
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    static constexpr int W = 1180, H = 850;
    static juce::Rectangle<float> card (int k) { return k == 0 ? juce::Rectangle<float> (16, 404, 374, 430) : k == 1 ? juce::Rectangle<float> (398, 404, 374, 430) : juce::Rectangle<float> (780, 404, 384, 430); }
    jlook::LnF lnf;
    Processor& proc;
    jlook::Scaler scaler { *this, "preamp", W, H };
    jlook::SizeChip sizeChip { scaler };
    juce::RangedAudioParameter* P (const juce::String& id) { return proc.apvts.getParameter (id); }
    bool stageOn (int k) const { return proc.value (k == 0 ? "s_on" : k == 1 ? "p_on" : "e_on") > 0.5f; }
    void turnOn (int k) { if (! stageOn (k)) { P (k == 0 ? "s_on" : k == 1 ? "p_on" : "e_on")->setValueNotifyingHost (1.0f); repaint(); } }
    int band = 0;
    void selectBand (int b) { band = juce::jlimit (0, 3, b); layoutVisibility(); repaint(); }
private:
    void timerCallback() override;
    void layoutVisibility();
    juce::Rectangle<float> bandBtn (int k) const { const auto c = card (0); return { c.getX() + 34 + k * 82.0f, c.getY() + 236, 60, 34 }; }
    struct KS { std::unique_ptr<alook::Knob> k; int stage; int group; };
    std::vector<KS> knobs;
    std::unique_ptr<alook::Rotary> preRot, eqRot, hpSl, lpSl;
    std::vector<std::unique_ptr<alook::Toggle>> toggles;
    Graph graph { *this };
    juce::Rectangle<float> presetR { 430, 30, 330, 40 }, prevR { 392, 34, 32, 32 }, nextR { 766, 34, 32, 32 }, abA { 820, 34, 32, 32 }, abB { 860, 34, 32, 32 };
    int tick = 0, lastEqModel = -1; float vuIn = -40, vuOut = -40;
};
}
