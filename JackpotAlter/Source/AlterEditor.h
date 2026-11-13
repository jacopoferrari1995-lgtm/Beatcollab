#pragma once
#include "AlterProcessor.h"
#include "Skin.h"
#include "JackpotScale.h"

namespace ja
{
namespace C
{
    inline juce::Colour bg { 0xff0b0c10 }, text { 0xffdfe3ea }, dim { 0xff7d8494 }, faint { 0xff4f5562 };
    inline juce::Colour magenta { 0xffff2e88 }, phos { 0xff5ef2ff }, amber { 0xffffb000 }, green { 0xff7dff5a }, red { 0xffff3b5c }, violet { 0xffa66bff };
    inline juce::Colour lcd1 { 0xff05161a }, lcd2 { 0xff020b0d };
    // colori presi dal tema della pelle
    inline void sync()
    {
        if (! jlook::skinHooks().on) return;
        const auto& p = jlook::skinPal();
        bg = p.bg;
        text = p.text;
        dim = p.dim;
        faint = p.faint;
        magenta = p.pink;
        phos = p.cyan;
        amber = p.amber;
        green = p.green;
        red = p.red;
        violet = p.violet;
        lcd1 = p.lcd0;
        lcd2 = p.lcd1;
    }
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
    void mouseDoubleClick (const juce::MouseEvent&) override { param->setValueNotifyingHost (param->getDefaultValue()); }
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w) override;
    void mouseEnter (const juce::MouseEvent&) override { hover = true; setMouseCursor (juce::MouseCursor::UpDownResizeCursor); repaint(); }
    void mouseExit (const juce::MouseEvent&) override { hover = false; repaint(); }
    juce::RangedAudioParameter* param; juce::Colour col; juce::String label;
    std::function<juce::String()> text; std::function<float (float, bool)> snap;   // snap: valore reale, shift premuto
    bool big = false, bipolar = false, enabled = true; float activity = 0;
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

// griglia dei modelli di saturazione
class ModelGrid : public juce::Component
{
public:
    ModelGrid (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override { hov = -1; repaint(); }
    juce::Rectangle<float> cell (int m) const;
    Editor& ed; int hov = -1;
};

// display del pitch: nota in ingresso / uscita e traccia
class PitchView : public juce::Component
{
public:
    PitchView (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    Editor& ed; float trace[160] {}; int tw = 0;
};

// forma d'onda e armoniche del modello scelto
class DriveView : public juce::Component
{
public:
    DriveView (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    void update (int model, float drive, float tone);
    Editor& ed; std::vector<float> waveIn, waveOut; float harm[12] {}; float thd = 0;
    int lastM = -1; float lastD = -1, lastT = -9;
};

// spettro con la banda FOCUS trascinabile
class FocusView : public juce::Component
{
public:
    FocusView (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void analyse();
    float xOf (float hz) const { return (float) (std::log (hz / 20.0) / std::log (1000.0) * getWidth()); }
    float hzOf (float x) const { return 20.0f * std::pow (1000.0f, juce::jlimit (0.0f, 1.0f, x / (float) getWidth())); }
    void edges (float* e, int& nb) const;          // bordi delle bande in Hz
    int xParam (int k, int nb) const;              // quale parametro "x" usa il taglio k
    int bandAt (float x) const;
    Editor& ed; int drag = -1, hovH = -1; float spIn[256] {}, spOut[256] {};
    std::unique_ptr<juce::dsp::FFT> fft;
};

// traccia della deriva
class DriftView : public juce::Component
{
public:
    DriftView (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    Editor& ed; float trace[200] {}; int tw = 0;
};

class Editor : public juce::AudioProcessorEditor, private juce::Timer
{
    juce::Rectangle<int> themeRect;
public:
    explicit Editor (Processor&);
    ~Editor() override { setLookAndFeel (nullptr); }
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void tickForTest() { for (int i = 0; i < 12; ++i) timerCallback(); }
    static constexpr int W = 1200, H = 930;
    int band = 0;   // banda del multibanda in modifica
    juce::String bp (const char* k) const { return "b" + juce::String (band + 1) + "_" + k; }
    float bv (const char* k) const { return proc.value (bp (k).toRawUTF8()); }
    void selectBand (int b);
    jlook::LnF lnf;
    Processor& proc;
    jlook::Scaler scaler { *this, "alter", W, H };
    jlook::SizeChip sizeChip { scaler };
    juce::RangedAudioParameter* P (const juce::String& id) { return proc.apvts.getParameter (id); }
    ModelGrid grid { *this };
    PitchView pitchView { *this };
    DriveView driveView { *this };
    FocusView focusView { *this };
    DriftView driftView { *this };
private:
    void timerCallback() override;
    std::unique_ptr<Knob> kPitch, kFormant, kPMix, kDrift, kRate, kDrive, kTone, kBMix, kBLvl, kMix, kOut;
    std::unique_ptr<Knob> kEDiv, kEMs, kEFb, kEMix, kELo, kEHi, kESat, kVWide, kVPan;
    std::unique_ptr<Segs> sMode, sKey, sScale, sDriftT, sOrder;
    std::unique_ptr<Switch> wLink, wSolo, wByp, wAuto, wEOn, wESync, wEPing, wVEcho;
    juce::Rectangle<int> presetRect, prevRect, nextRect, abRect; bool abHover = false; int tick = 0;
};
}
