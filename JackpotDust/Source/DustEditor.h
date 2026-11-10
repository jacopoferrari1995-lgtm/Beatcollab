#pragma once
#include "DustProcessor.h"
#include "JackpotScale.h"

namespace jds
{
namespace C
{
    const juce::Colour bg { 0xff0b0c10 }, text { 0xffdfe3ea }, dim { 0xff7d8494 }, faint { 0xff4f5562 };
    const juce::Colour amber { 0xffffb000 }, rust { 0xffff7a1a }, cream { 0xfff3d9a4 }, olive { 0xffb5c94a }, phos { 0xff5ef2ff }, red { 0xffff3b5c }, rose { 0xffff6f91 }, green { 0xff7dff5a };
    const juce::Colour lcd1 { 0xff1a1408 }, lcd2 { 0xff0c0904 };
}
juce::Font uf (float size, bool bold = true);
juce::Font mf (float size);
void glowText (juce::Graphics&, const juce::String&, juce::Rectangle<float>, juce::Colour, juce::Font, juce::Justification = juce::Justification::centredLeft);
void lcd (juce::Graphics&, juce::Rectangle<float>, float rad);

class Editor;
enum Mod { M_SAMP, M_SAT, M_COMP, M_FILT, M_TAPE, M_DUST, kMods };

class Knob : public juce::Component
{
public:
    Knob (juce::RangedAudioParameter* p, juce::Colour c) : param (p), col (c) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent&) override { if (drag) param->endChangeGesture(); drag = false; repaint(); }
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseDoubleClick (const juce::MouseEvent&) override { param->setValueNotifyingHost (param->getDefaultValue()); }
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w) override;
    void mouseEnter (const juce::MouseEvent&) override { hover = true; setMouseCursor (juce::MouseCursor::UpDownResizeCursor); repaint(); }
    void mouseExit (const juce::MouseEvent&) override { hover = false; repaint(); }
    juce::RangedAudioParameter* param; juce::Colour col; juce::String label;
    std::function<juce::String()> text; std::function<void()> onTouch;
    bool big = false, bipolar = false, lit = true; float shown = -1;
private:
    float start = 0; bool drag = false, hover = false;
};
class Segs : public juce::Component
{
public:
    Segs (juce::RangedAudioParameter* p, juce::Colour c, juce::StringArray it) : param (p), col (c), items (std::move (it)) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    juce::RangedAudioParameter* param; juce::Colour col; juce::StringArray items; float font = 10.0f; bool lit = true; std::function<void (int)> onPick;
};
class Switch : public juce::Component
{
public:
    Switch (juce::RangedAudioParameter* p, juce::Colour c, juce::String l = {}) : param (p), col (c), label (std::move (l)) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { param->setValueNotifyingHost (param->getValue() > 0.5f ? 0.0f : 1.0f); }
    juce::RangedAudioParameter* param; juce::Colour col; juce::String label;
};
// display piccolo dentro ogni modulo
class ModView : public juce::Component
{
public:
    ModView (Editor& e, int m) : ed (e), mod (m) {}
    void paint (juce::Graphics&) override;
    Editor& ed; int mod; float trace[120] {}; int tw = 0; float spin = 0;
};
class Scope : public juce::Component
{
public:
    Scope (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    Editor& ed;
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
    void tickForTest() { for (int i = 0; i < 30; ++i) timerCallback(); }
    static constexpr int W = 1200, H = 780;
    static juce::Rectangle<float> card (int m) { return { 18.0f + (m % 3) * 310.0f, 76.0f + (m / 3) * 342.0f, 300, 332 }; }
    static const char* onId (int m) { static const char* ids[] = { "s_on", "d_on", "c_on", "f_on", "t_on", "n_on" }; return ids[m]; }
    bool isOn (int m) const { return proc.value (onId (m)) > 0.5f; }
    jlook::LnF lnf;
    Processor& proc;
    jlook::Scaler scaler { *this, "dust", W, H };
    jlook::SizeChip sizeChip { scaler };
    juce::RangedAudioParameter* P (const juce::String& id) { return proc.apvts.getParameter (id); }
private:
    void timerCallback() override;
    std::vector<std::unique_ptr<Knob>> knobs; std::vector<std::pair<Knob*, int>> knobMod;
    std::vector<std::unique_ptr<Segs>> segs; std::vector<std::pair<Segs*, int>> segMod;
    std::vector<std::unique_ptr<Switch>> switches;
    std::vector<std::unique_ptr<ModView>> views;
    Scope scope { *this };
    Knob* kAmount = nullptr;
    juce::Rectangle<int> presetRect, prevRect, nextRect, abRect; bool abHover = false; int tick = 0;
};
}
