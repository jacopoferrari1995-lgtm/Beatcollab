#pragma once
#include "MeterProcessor.h"
#include "JackpotScale.h"

namespace jm
{
namespace C
{
    const juce::Colour text { 0xffdfe3ea }, dim { 0xff7d8494 }, faint { 0xff4f5562 };
    const juce::Colour magenta { 0xffff2e88 }, phos { 0xff5ef2ff }, amber { 0xffffb000 }, green { 0xff7dff5a }, red { 0xffff3b5c }, violet { 0xffa66bff };
    const juce::Colour lcd1 { 0xff05161a }, lcd2 { 0xff020b0d };
    const juce::Colour band[kBands] = { juce::Colour (0xffff6a3d), juce::Colour (0xffffd23f), juce::Colour (0xff7dff5a), juce::Colour (0xff2de2e6), juce::Colour (0xffa66bff), juce::Colour (0xffff4fd8),
                                        juce::Colour (0xffff9f5a), juce::Colour (0xffd4ff4a), juce::Colour (0xff3dffb0), juce::Colour (0xff4aa8ff), juce::Colour (0xffd07bff), juce::Colour (0xffff6b9a) };
}
class Editor;

class Knob : public juce::Component
{
public:
    Knob (juce::Colour c) : col (c) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent&) override { if (drag && param) param->endChangeGesture(); drag = false; repaint(); }
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseDoubleClick (const juce::MouseEvent&) override { if (param) param->setValueNotifyingHost (param->getDefaultValue()); }
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w) override { if (param) param->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, param->getValue() + (w.deltaY > 0 ? 0.01f : -0.01f))); }
    void mouseEnter (const juce::MouseEvent&) override { hover = true; setMouseCursor (juce::MouseCursor::UpDownResizeCursor); repaint(); }
    void mouseExit (const juce::MouseEvent&) override { hover = false; repaint(); }
    juce::RangedAudioParameter* param = nullptr; juce::Colour col; juce::String label; std::function<juce::String()> text; bool bipolar = false;
private:
    float start = 0; bool drag = false, hover = false;
};

class SpecEq : public juce::Component
{
public:
    SpecEq (Editor& e);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void analyse();   // FFT dal buffer ad anello
    Editor& ed;
    static constexpr int kOrder = 13, kN = 1 << kOrder, kPts = 320;
    juce::dsp::FFT fft { kOrder }; juce::dsp::WindowingFunction<float> win { (size_t) kN, juce::dsp::WindowingFunction<float>::hann };
    std::vector<float> fbuf; float specA[kPts] {}, specB[kPts] {}, specC[kPts] {}, peak[kPts] {}; float bandE[kBal] {}; double bandRaw[kBal] {}; int bandN[kBal] {}; bool bandInit = false;
    int dragBand = -1, hoverBand = -1; float hoverF = 0; juce::Point<float> hoverPt { -1, -1 };
    struct Pin { float f, db; }; std::vector<Pin> pins;   // punti segnati sullo spettro fermo
    juce::Rectangle<float> freezeR, scR, dynScR, clearR;
    int snapPeak (float x) const;                        // indice del picco piu vicino al mouse
    static float tilt (float f) { return 4.5f * std::log2 (f / 1000.0f); }
    static float ptF (int i) { return 20.0f * std::pow (1000.0f, (i + 0.5f) / kPts); }
    juce::Rectangle<float> plot() const { return getLocalBounds().toFloat().reduced (14, 10).withTrimmedTop (30).withTrimmedBottom (16); }
    float xOf (float f) const; float fOf (float x) const; float yOf (float db) const; float dbOf (float y) const;
    float ySpec (float db) const;
    int nodeAt (juce::Point<float>) const;
};

class Loud : public juce::Component
{
public:
    Loud (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    Editor& ed; juce::Rectangle<float> resetR, targetR;
};

class Balance : public juce::Component
{
public:
    Balance (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    Editor& ed; juce::Rectangle<float> profR;
};

class Stereo : public juce::Component
{
public:
    Stereo (Editor& e);
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    Editor& ed; juce::Image persist;
    Knob width { C::violet }, bal { C::violet }, low { C::violet }, mid { C::violet }, high { C::violet }, mono { C::violet };
    juce::Rectangle<float> monoR, onR;
};

class BandPanel : public juce::Component
{
public:
    BandPanel (Editor& e);
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void bind();
    Editor& ed;
    Knob freq { C::phos }, gain { C::phos }, q { C::phos }, dyn { C::magenta }, thr { C::magenta };
    juce::Rectangle<float> soloR;
};

class Editor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit Editor (Processor&);
    ~Editor() override { setLookAndFeel (nullptr); }
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    static constexpr int W = 1240, H = 900;
    jlook::LnF lnf;
    Processor& proc;
    jlook::Scaler scaler { *this, "meter", W, H };
    jlook::SizeChip sizeChip { scaler };
    int selBand = 0;
    juce::RangedAudioParameter* P (const juce::String& id) { return proc.apvts.getParameter (id); }
    float bp (int b, const char* k) const { return proc.value (("b" + juce::String (b + 1) + "_" + k).toRawUTF8()); }
    juce::RangedAudioParameter* bpar (int b, const char* k) { return P ("b" + juce::String (b + 1) + "_" + k); }
    void setP (juce::RangedAudioParameter* p, float v) { p->setValueNotifyingHost (p->convertTo0to1 (v)); }
    SpecEq spec { *this };
    BandPanel band { *this };
private:
    void timerCallback() override;
    Loud loud { *this };
    Balance bal { *this };
    Stereo st { *this };
    juce::Rectangle<int> eqRect, specRect, abRect;
};

juce::Font uf (float size, bool bold = true);
juce::Font mf (float size);
void glowText (juce::Graphics&, const juce::String&, juce::Rectangle<float>, juce::Colour, juce::Font, juce::Justification = juce::Justification::centredLeft);
void lcd (juce::Graphics&, juce::Rectangle<float>, float rad);
}
