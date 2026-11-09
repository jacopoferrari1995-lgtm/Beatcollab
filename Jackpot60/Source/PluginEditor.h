#pragma once
#include "PluginProcessor.h"
#include <set>

namespace j60
{
// ---------- palette vintage calda ----------
namespace C
{
    const juce::Colour chassisHi   { 0xffefe7d5 }, chassisLo { 0xffd9cdb4 }, chassisLine { 0xffc4b799 };
    const juce::Colour darkPanel   { 0xff3b342c }, darkPanel2 { 0xff2c2620 };
    const juce::Colour ink         { 0xff3a2e22 }, inkSoft { 0xff7a6a55 };
    const juce::Colour orange      { 0xffd4602c }, red { 0xffc43c2b }, green { 0xff6f9a4a };
    const juce::Colour lcdBg       { 0xff241608 }, lcdBg2 { 0xff1a0f05 }, amber { 0xffffb347 }, amberDim { 0xff8a5a20 };
    const juce::Colour padLo       { 0xffa79c8a }, padHi { 0xffcfc5b2 };
}
juce::Font fnt (float size, bool bold = false, bool mono = false);

class VintageLAF : public juce::LookAndFeel_V4
{
public:
    VintageLAF();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider&) override;
    void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float pos, float minP, float maxP, juce::Slider::SliderStyle, juce::Slider&) override;
    juce::Label* createSliderTextBox (juce::Slider&) override;
};

// pulsante "hardware" con led
class HwButton : public juce::Component
{
public:
    HwButton (juce::String text, juce::Colour led = C::red) : label (std::move (text)), ledColour (led) {}
    std::function<void()> onClick, onPress, onRelease;
    std::function<bool()> isLit;
    bool big = false;
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { down = true; repaint(); if (onPress) onPress(); }
    void mouseUp (const juce::MouseEvent& e) override { down = false; repaint(); if (onRelease) onRelease(); if (contains (e.getPosition()) && onClick) onClick(); }
    juce::String label; juce::Colour ledColour; bool down = false;
};

// valore modificabile in stile display (trascina, rotella, clic per le scelte)
class ParamBox : public juce::Component
{
public:
    ParamBox (juce::RangedAudioParameter* p, juce::String caption) : param (p), cap (std::move (caption)) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    juce::RangedAudioParameter* param; juce::String cap; float startV = 0;
};

class Editor;

class PadComp : public juce::Component
{
public:
    PadComp (Editor& e, int idx) : ed (e), slot (idx) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    Editor& ed; int slot; float flash = 0; uint32_t lastHit = 0; bool held = false;
};

class LcdComp : public juce::Component
{
public:
    explicit LcdComp (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override { dragRow = -1; dragMarker = -1; }
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    juce::Rectangle<int> waveArea() const, listArea() const;
    int rowAt (juce::Point<int>) const;
    Editor& ed; int dragRow = -1, dragMarker = -1; float dragStart = 0; juce::Point<int> downPos;
};

class GridComp : public juce::Component
{
public:
    explicit GridComp (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    juce::Rectangle<float> cellArea() const;
    Editor& ed; int page = 0; int velEdit = -1; int velTick = 0, velPad = 0, velStart = 100; bool paintMode = false, paintVal = false;
};

class FxRack : public juce::Component
{
public:
    explicit FxRack (Processor&);
    void paint (juce::Graphics&) override;
    void resized() override;
    Processor& proc;
    struct Lab { juce::Rectangle<int> r; juce::String t; };
    juce::OwnedArray<juce::Slider> knobs;
    juce::OwnedArray<juce::AudioProcessorValueTreeState::SliderAttachment> atts;
    juce::OwnedArray<ParamBox> boxes;
    juce::OwnedArray<HwButton> toggles;
    juce::Array<Lab> labs;
    std::unique_ptr<HwButton> close, preset;
    int presetIdx = 0;
    juce::Rectangle<int> modRect (int m) const;
    juce::Slider* mag = nullptr;
};

class DragMidi : public juce::Component
{
public:
    explicit DragMidi (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    Editor& ed; bool dragging = false;
};

class Editor : public juce::AudioProcessorEditor, public juce::DragAndDropContainer, public juce::FileDragAndDropTarget, private juce::Timer
{
public:
    explicit Editor (Processor&);
    ~Editor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    bool isInterestedInFileDrag (const juce::StringArray&) override { return true; }
    void filesDropped (const juce::StringArray&, int x, int y) override;
    bool keyPressed (const juce::KeyPress&) override;
    bool keyStateChanged (bool isKeyDown) override;

    Processor& proc;
    int bank = 0, selPad = 0;
    void selectPad (int p);
    void refreshQLink();
    juce::String status;
    void setStatus (const juce::String& s) { status = s; statusTime = juce::Time::getMillisecondCounter(); }

private:
    void timerCallback() override;
    void loadDialog();
    VintageLAF laf;
    std::unique_ptr<LcdComp> lcd; std::unique_ptr<GridComp> grid; std::unique_ptr<DragMidi> dragMidi; std::unique_ptr<FxRack> fxRack;
    juce::OwnedArray<PadComp> pads; juce::OwnedArray<HwButton> buttons;
    HwButton *bRec = nullptr, *bStop = nullptr, *bPlay = nullptr, *bStart = nullptr, *bRepeat = nullptr, *bFull = nullptr, *bErase = nullptr, *bSync = nullptr, *bClick = nullptr;
    HwButton* bBank[4] = {};
    HwButton *bSeqDn = nullptr, *bSeqUp = nullptr, *bBarDn = nullptr, *bBarUp = nullptr, *bPgDn = nullptr, *bPgUp = nullptr, *bResample = nullptr, *bFx = nullptr;
    std::unique_ptr<ParamBox> pbBpm, pbTc, pbRpt, pbVin;
    juce::Slider swing, drive, tone, out;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> aSwing, aDrive, aTone, aOut;
    juce::Slider qTune, qDecay, qFilter, qLevel;
    std::unique_ptr<juce::FileChooser> chooser;
    std::set<int> keysDown;
    uint32_t statusTime = 0;
    friend class LcdComp; friend class GridComp;
};
}
