#pragma once
#include "PreProcessor.h"
#include "AnalogLook.h"

namespace jpre
{
class Editor;
// schermo del centro: spettro, curva totale, punti dei tagli, riduzione delle risonanze
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

// un modulo del rack
class SlotStrip : public juce::Component, public juce::SettableTooltipClient
{
public:
    SlotStrip (Editor& e, int s);
    void paint (juce::Graphics&) override;
    void resized() override { layout(); }
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void refresh();          // ricalcola comandi visibili, etichette e testi per il modulo caricato
    void layout();
    void openMenu();
    int type() const;
    Editor& ed; int slot; int shownType = -1; bool dragging = false, dropHover = false;
    std::unique_ptr<alook::Knob> knobs[kSlotParams];
    std::unique_ptr<alook::Toggle> toggles[kSlotParams];
    juce::Rectangle<float> nameR() const { return { 10, 30, getWidth() - 20.0f, 26 }; }
    juce::Rectangle<float> jewelR() const { return { 8, 6, 22, 22 }; }
};

class Editor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit Editor (Processor&);
    ~Editor() override { setLookAndFeel (nullptr); }
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    static constexpr int W = 1180, H = 900;
    const juce::Rectangle<float> headR { 16, 8, 1148, 92 }, coreR { 16, 106, 1148, 330 }, rackR { 16, 442, 1148, 450 };
    jlook::LnF lnf;
    Processor& proc;
    jlook::Scaler scaler { *this, "preamp", W, H };
    jlook::SizeChip sizeChip { scaler };
    juce::TooltipWindow tips { nullptr, 600 };
    juce::RangedAudioParameter* P (const juce::String& id) { return proc.apvts.getParameter (id); }
    bool coreOn() const { return proc.value ("c_on") > 0.5f; }
    int coreMode() const { return (int) proc.value ("c_mode"); }
    void turnOnCore() { if (! coreOn()) { P ("c_on")->setValueNotifyingHost (1.0f); repaint(); } }
    int band = 0;   // 0-3 tagli, 4 = passa alto / basso e dinamica
    void selectBand (int b) { band = juce::jlimit (0, 4, b); layoutCore(); repaint(); }
    void layoutCore();
    void layoutRack();
    juce::Rectangle<float> chipR() const;
    int slotAt (juce::Point<int> editorPos) const;   // per trascinare i moduli
    std::vector<std::unique_ptr<SlotStrip>> strips;
private:
    void timerCallback() override;
    juce::Rectangle<float> tabR (int k) const { return { 836.0f + k * 63.0f, 160, 58, 30 }; }
    juce::Rectangle<float> modeR (int k) const { return { 900.0f + k * 128.0f, 120, 122, 30 }; }
    juce::Rectangle<float> soloR() const { return { 1094, 346, 54, 30 }; }
    struct KS { std::unique_ptr<alook::Knob> k; int mode; int group; };   // mode 0 EQ / 1 risonanze; group = banda (-1 comune, 4 HP/LP)
    std::vector<KS> knobs;
    std::unique_ptr<alook::Rotary> hpSl, lpSl, msRot, rMsRot;
    std::unique_ptr<alook::Toggle> dynTog, deltaTog;
    std::vector<std::unique_ptr<alook::Toggle>> bandDyn;
    Graph graph { *this };
    juce::Rectangle<float> presetR { 430, 30, 330, 40 }, prevR { 392, 34, 32, 32 }, nextR { 766, 34, 32, 32 }, abA { 820, 34, 32, 32 }, abB { 860, 34, 32, 32 };
    int tick = 0, lastPos = -1, lastMode = -1; float vuIn = -40, vuOut = -40;
};
}
