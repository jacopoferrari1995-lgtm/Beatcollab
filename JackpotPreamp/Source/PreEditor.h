#pragma once
#include "PreProcessor.h"

namespace jpre
{
using namespace jkit;
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
    void mouseMove (const juce::MouseEvent&) override;
    static constexpr int W = 1180, H = 780;
    static juce::Rectangle<float> card (int k) { return k == 0 ? juce::Rectangle<float> (18, 406, 372, 356) : k == 1 ? juce::Rectangle<float> (400, 406, 372, 356) : juce::Rectangle<float> (782, 406, 380, 356); }
    jlook::LnF lnf;
    Processor& proc;
    jlook::Scaler scaler { *this, "preamp", W, H };
    jlook::SizeChip sizeChip { scaler };
    juce::RangedAudioParameter* P (const juce::String& id) { return proc.apvts.getParameter (id); }
    bool stageOn (int k) const { return proc.value (k == 0 ? "s_on" : k == 1 ? "p_on" : "e_on") > 0.5f; }
    void turnOn (int k) { if (! stageOn (k)) P (k == 0 ? "s_on" : k == 1 ? "p_on" : "e_on")->setValueNotifyingHost (1.0f); }
    int band = 0;
    void selectBand (int b) { band = juce::jlimit (0, 3, b); layoutVisibility(); repaint(); }
    const juce::Colour colSub = C::red, colPre = C::amber, colEq = C::phos;
private:
    void timerCallback() override;
    void layoutVisibility();
    struct KS { std::unique_ptr<Knob> k; int stage; int group; };   // group: per il sottrattivo la banda (-1 = sempre), per l'EQ il tipo (0 pultec, 1 meq, 2 analogici, -1 sempre)
    std::vector<KS> knobs;
    std::unique_ptr<Segs> preSeg, eqSeg, hpSl, lpSl;
    std::vector<std::unique_ptr<Switch>> switches;
    Graph graph { *this };
    PresetBar bar; int tick = 0, lastEqModel = -1;
    juce::Rectangle<float> bandTabs { 30, 600, 348, 26 };
};
}
