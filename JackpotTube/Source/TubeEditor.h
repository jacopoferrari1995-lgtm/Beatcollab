#pragma once
#include "TubeProcessor.h"
#include "Skin.h"
#include "JackpotScale.h"
#include "HwLook.h"
#include "PresetBrowser.h"

namespace jtbx
{
class Editor;

// manopola da frontale (come Dust, FX e Space): si ridisegna solo quando cambia
class Knob : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit Knob (juce::RangedAudioParameter* p) : param (p) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override { param->setValueNotifyingHost (defV >= 0 ? defV : param->getDefaultValue()); }
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void mouseEnter (const juce::MouseEvent&) override { hover = true; setMouseCursor (juce::MouseCursor::UpDownResizeCursor); repaint(); }
    void mouseExit (const juce::MouseEvent&) override { hover = false; repaint(); }
    void refresh (bool on) { const auto t = text ? text() : param->getCurrentValueAsText(); const float v = param->getValue(); if (t != lastT || v != lastV || on != lit) { lastT = t; lastV = v; lit = on; repaint(); } }
    juce::RangedAudioParameter* param; hw::KnobSpec spec; std::function<juce::String()> text; float defV = -1; bool lit = true;
private:
    float start = 0, lastV = -1; bool drag = false, hover = false; juce::String lastT;
};

// le due valvole a vista e il VU: si accendono e scaldano con quanto lavorano
class TubeView : public juce::Component
{
public:
    explicit TubeView (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    Editor& ed; float glow = -1, needle = -0.75f, flick = 0;
};
// dove nascono le armoniche: la zona del FUOCO e le sue armoniche che salgono (2a, 3a, 4a, 5a)
class HarmView : public juce::Component
{
public:
    explicit HarmView (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent& e) override { mouseDrag (e); }
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    Editor& ed; bool dragging = false;
};

class Editor : public juce::AudioProcessorEditor, private juce::Timer
{
    skin::Keep skinKeep;
public:
    explicit Editor (Processor&);
    ~Editor() override { stopTimer(); setLookAndFeel (nullptr); }
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    static constexpr int W = 1180, H = 800;
    static juce::Rectangle<float> hgCard() { return { 18, 72, 1144, 400 }; }
    static juce::Rectangle<float> enCard() { return { 18, 482, 1144, 290 }; }
    jlook::LnF lnf;
    Processor& proc;
    jlook::Scaler scaler { *this, "tubebox", W, H };
    jlook::SizeChip sizeChip { scaler };
    juce::RangedAudioParameter* P (const juce::String& id) { return proc.apvts.getParameter (id); }
    bool on (const char* id) const { return proc.value (id) > 0.5f; }
    TubeView tubes { *this };
    HarmView harm { *this };
    jbrowse::Browser browser;
    void tickForTest() { for (int i = 0; i < 30; ++i) timerCallback(); }
    void refreshAll() { repaint(); }
private:
    void timerCallback() override;
    juce::String staticKey();
    void paintStatic (juce::Graphics&);
    void setupBrowser();
    void askPresetName();
    std::unique_ptr<Knob> mk (const char* id, const char* label, int style, juce::Colour col, std::function<juce::String()> txt, const char* tip, bool big = false, bool bip = false);
    struct Btn { juce::Rectangle<float> r; juce::String id; int value; juce::String label; juce::Colour col; };   // value -1 = interruttore
    std::vector<Btn> btns;
    std::vector<std::unique_ptr<Knob>> hgKnobs, enKnobs;
    juce::Rectangle<int> presetR, prevR, nextR, abR, themeR;
    int hoverBtn = -1, hoverTop = -1;
    jlook::StaticLayer bg;
};
}
