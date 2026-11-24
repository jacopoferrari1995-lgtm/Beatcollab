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
class Editor : public juce::AudioProcessorEditor, private juce::Timer
{
    skin::Keep skinKeep;
public:
    explicit Editor (Processor&);
    ~Editor() override { stopTimer(); removeMouseListener (&undoMark); setLookAndFeel (nullptr); }
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    bool keyPressed (const juce::KeyPress&) override;
    static constexpr int W = 1100, H = 680;
    static juce::Rectangle<float> mainCard() { return { 18, 76, 1064, 340 }; }
    static juce::Rectangle<float> lowCard() { return { 18, 428, 1064, 222 }; }
    jlook::LnF lnf;
    Processor& proc;
    jlook::Scaler scaler { *this, "tubebox2", W, H };
    jlook::SizeChip sizeChip { scaler };
    juce::RangedAudioParameter* P (const juce::String& id) { return proc.apvts.getParameter (id); }
    bool on (const char* id) const { return proc.value (id) > 0.5f; }
    bool active() const { return proc.value ("drive") > 0.001f || proc.value ("warmth") > 0.001f || proc.value ("air") > 0.001f || proc.value ("enh") > 0.001f; }
    int character() const { return juce::jlimit (0, kChars - 1, juce::roundToInt (proc.value ("char"))); }
    TubeView tubes { *this };
    jbrowse::Browser browser;
    void tickForTest() { for (int i = 0; i < 30; ++i) timerCallback(); }
    void refreshAll() { repaint(); }
    struct UndoMark : juce::MouseListener
    {
        Processor* p = nullptr; double lastWheel = 0;
        void mouseDown (const juce::MouseEvent&) override { if (p) p->undo.beginNewTransaction(); }
        void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override { const double t = juce::Time::getMillisecondCounterHiRes(); if (p && t - lastWheel > 600) p->undo.beginNewTransaction(); lastWheel = t; }
    } undoMark;
private:
    void timerCallback() override;
    juce::String staticKey();
    void paintStatic (juce::Graphics&);
    void setupBrowser();
    void askPresetName();
    void doUndo (bool redo);
    std::unique_ptr<Knob> mk (const char* id, const char* label, int style, juce::Colour col, std::function<juce::String()> txt, const char* tip, bool big = false, bool bip = false);
    std::vector<std::unique_ptr<Knob>> knobs;   // DRIVE, CALORE, ARIA, ENHANCER, MIX, OUTPUT
    juce::Rectangle<float> charR[kChars], autoR;
    juce::Rectangle<int> presetR, prevR, nextR, abR, themeR, undoR, redoR;
    int hoverChar = -1, hoverTop = -1;
    jlook::StaticLayer bg;
};
}
