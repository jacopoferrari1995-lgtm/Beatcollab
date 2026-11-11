#pragma once
#include "MotionProcessor.h"
#include "JackpotScale.h"

namespace jmo
{
namespace C
{
    const juce::Colour bg { 0xff0b0c10 }, text { 0xffdfe3ea }, dim { 0xff7d8494 }, faint { 0xff4f5562 };
    const juce::Colour magenta { 0xffff2e88 }, phos { 0xff5ef2ff }, amber { 0xffffb000 }, green { 0xff7dff5a }, red { 0xffff3b5c }, violet { 0xffa66bff };
    const juce::Colour lcd1 { 0xff05161a }, lcd2 { 0xff020b0d };
}
juce::Font uf (float size, bool bold = true);
juce::Font mf (float size);
void glowText (juce::Graphics&, const juce::String&, juce::Rectangle<float>, juce::Colour, juce::Font, juce::Justification = juce::Justification::centredLeft);
void lcd (juce::Graphics&, juce::Rectangle<float>, float rad);

class Editor;

class Knob : public juce::Component
{
public:
    Knob (juce::RangedAudioParameter* p, juce::Colour c) : param (p), col (c) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent&) override { if (drag) param->endChangeGesture(); drag = false; repaint(); }
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseDoubleClick (const juce::MouseEvent&) override { if (enabled) param->setValueNotifyingHost (param->getDefaultValue()); }
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w) override;
    void mouseEnter (const juce::MouseEvent&) override { hover = true; setMouseCursor (juce::MouseCursor::UpDownResizeCursor); repaint(); }
    void mouseExit (const juce::MouseEvent&) override { hover = false; repaint(); }
    juce::RangedAudioParameter* param; juce::Colour col; juce::String label;
    std::function<juce::String()> text; int steps = 0;   // steps > 0: parametro a scatti (divisioni)
    bool big = false, bipolar = false, enabled = true; float activity = 0, shown = -1;
private:
    float start = 0; bool drag = false, hover = false;
};

class Segs : public juce::Component
{
public:
    Segs (juce::RangedAudioParameter* p, juce::Colour c, juce::StringArray it) : param (p), col (c), items (std::move (it)) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    juce::RangedAudioParameter* param; juce::Colour col; juce::StringArray items; bool enabled = true; float font = 11.0f;
};

class Switch : public juce::Component
{
public:
    Switch (juce::RangedAudioParameter* p, juce::Colour c, juce::String l = {}) : param (p), col (c), label (std::move (l)) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { param->setValueNotifyingHost (param->getValue() > 0.5f ? 0.0f : 1.0f); }
    juce::RangedAudioParameter* param; juce::Colour col; juce::String label;
};

// griglia degli effetti: clic = modifica, LED = acceso/spento
class FxGrid : public juce::Component
{
public:
    FxGrid (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override { hov = -1; repaint(); }
    static constexpr int cols = 7;
    juce::Rectangle<float> cell (int m) const { const float gap = 6, w = (getWidth() - gap * (cols - 1)) / cols, h = (getHeight() - gap) / 2; return { (m % cols) * (w + gap), (m / cols) * (h + gap), w, h }; }
    juce::Rectangle<float> led (int m) const { auto c = cell (m); return { c.getX() + 6, c.getY() + 6, 26, c.getHeight() - 12 }; }
    Editor& ed; int hov = -1;
};

// display animato dell'effetto selezionato
class FxView : public juce::Component
{
public:
    FxView (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    Editor& ed; float trace[240] {}; int tw = 0;
};

// autopan: dove si trova il suono
class PanView : public juce::Component
{
public:
    PanView (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    Editor& ed; float trail[40] {}; int tw = 0;
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
    void tickForTest() { for (int i = 0; i < 12; ++i) timerCallback(); }
    static constexpr int W = 1200, H = 830;
    jlook::LnF lnf;
    Processor& proc;
    jlook::Scaler scaler { *this, "motion", W, H };
    jlook::SizeChip sizeChip { scaler };
    juce::RangedAudioParameter* P (const juce::String& id) { return proc.apvts.getParameter (id); }
    juce::RangedAudioParameter* FP (const char* p) { return proc.apvts.getParameter (pid (sel, p)); }
    float fv (const char* p) const { return proc.apvts.getRawParameterValue (pid (sel, p))->load(); }
    void select (int f);
    void layoutControls();
    int sel = CHORUS;
    FxGrid grid { *this };
    FxView view { *this };
    PanView panView { *this };
private:
    void timerCallback() override;
    std::unique_ptr<Knob> kRate, kDiv, kDepth, kChar, kFb, kFreq, kStereo, kMix, kTone, kWidth, kBass, kOut, kPanRate, kPanDiv, kPanDepth, kDry;
    std::unique_ptr<Knob> kVoice[4], kLevel[4];
    std::unique_ptr<Segs> sShape, sPanShape, sKey, sScale;
    std::unique_ptr<Switch> wSync, wPan, wPanSync, wOn;
    juce::Rectangle<int> presetRect, prevRect, nextRect, abRect; bool abHover = false; int tick = 0;
};
}
