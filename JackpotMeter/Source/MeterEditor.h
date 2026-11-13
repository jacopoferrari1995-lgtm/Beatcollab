#pragma once
#include "MeterProcessor.h"
#include "Skin.h"
#include "JackpotScale.h"

namespace jm
{
namespace C
{
    inline juce::Colour text { 0xffdfe3ea }, dim { 0xff7d8494 }, faint { 0xff4f5562 };
    inline juce::Colour magenta { 0xffff2e88 }, phos { 0xff5ef2ff }, amber { 0xffffb000 }, green { 0xff7dff5a }, red { 0xffff3b5c }, violet { 0xffa66bff };
    inline juce::Colour lcd1 { 0xff05161a }, lcd2 { 0xff020b0d };
    inline juce::Colour band[kBands] = { juce::Colour (0xffff6a3d), juce::Colour (0xffffd23f), juce::Colour (0xff7dff5a), juce::Colour (0xff2de2e6), juce::Colour (0xffa66bff), juce::Colour (0xffff4fd8),
                                        juce::Colour (0xffff9f5a), juce::Colour (0xffd4ff4a), juce::Colour (0xff3dffb0), juce::Colour (0xff4aa8ff), juce::Colour (0xffd07bff), juce::Colour (0xffff6b9a) };
    // colori presi dal tema della pelle
    inline void sync()
    {
        if (! jlook::skinHooks().on) return;
        const auto& p = jlook::skinPal();
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

class SpecEq;
// finestrella della banda sul grafico (come Pro-Q): tipo, SOLO, DINAMICA, valori trascinabili
class BandPop : public juce::Component
{
public:
    explicit BandPop (SpecEq& s) : sp (s) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    SpecEq& sp; int band = -1;
    struct Hot { juce::Rectangle<float> r; int id; }; std::vector<Hot> hots;
private:
    int dragK = -1; float dragStart = 0;
    int hit (juce::Point<float>) const;
};

// riquadro centrale: EQ, REFERENCE, IMAGER, PEAK, SPETTROGRAMMA
class SpecEq : public juce::Component
{
public:
    SpecEq (Editor& e);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void analyse();   // FFT dal buffer ad anello
    void updatePop();
    void bandMenu (int b);
    void resetAverage() { std::fill (std::begin (ltasPow), std::end (ltasPow), 0.0); ltasN = 0; }
    int view() const;
    // spettro medio a lungo termine (dB, compensato 4.5 dB/ott) e scarti per banda rispetto al riferimento scelto
    bool hasLtas() const { return ltasN > 8; }
    float ltasDb (int i) const;
    bool refCurve (float* out) const;            // curva del riferimento scelto (genere o traccia), allineata alla mia
    void bandDeviation (float* dev) const;       // 6 bande: io - riferimento (dB), media tolta
    Editor& ed;
    static constexpr int kOrder = 13, kN = 1 << kOrder;
    juce::dsp::FFT fft { kOrder }; juce::dsp::WindowingFunction<float> win { (size_t) kN, juce::dsp::WindowingFunction<float>::hann };
    std::vector<float> fbuf; float specA[kPts] {}, specB[kPts] {}, specC[kPts] {}, peak[kPts] {}, frameP[kPts] {};
    double ltasPow[kPts] {}; int ltasN = 0;
    int dragBand = -1, hoverBand = -1, popBand = -1, popHold = 0; float hoverF = 0; juce::Point<float> hoverPt { -1, -1 };
    struct Pin { float f, db; }; std::vector<Pin> pins;   // punti segnati sullo spettro fermo
    juce::Rectangle<float> tabR[5], freezeR, scR, dynScR, clearR, imTabR[3], loadR, resetAvgR;
    BandPop pop { *this };
    juce::Image sgram, persist; int sgX = 0;
    int snapPeak (float x) const;                        // indice del picco piu vicino al mouse
    static float tilt (float f) { return 4.5f * std::log2 (f / 1000.0f); }
    static float ptF (int i) { return ptFreq (i); }
    juce::Rectangle<float> plot() const { return getLocalBounds().toFloat().reduced (14, 10).withTrimmedTop (30).withTrimmedBottom (16); }
    float xOf (float f) const; float fOf (float x) const; float yOf (float db) const; float dbOf (float y) const;
    float ySpec (float db) const;
    int nodeAt (juce::Point<float>) const;
private:
    void paintEq (juce::Graphics&, juce::Rectangle<float>);
    void paintRef (juce::Graphics&, juce::Rectangle<float>);
    void paintImager (juce::Graphics&, juce::Rectangle<float>);
    void paintPeak (juce::Graphics&, juce::Rectangle<float>);
    void paintGram (juce::Graphics&, juce::Rectangle<float>);
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

// riferimento: genere o pezzo caricato, con il confronto di loudness e picco
class RefPanel : public juce::Component
{
public:
    explicit RefPanel (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void chooseFile();
    Editor& ed; juce::Rectangle<float> genreR, loadR, clearR, viewR;
    std::unique_ptr<juce::FileChooser> chooser;
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
    static constexpr int W = 1240, H = 900;
    jlook::LnF lnf;
    Processor& proc;
    jlook::Scaler scaler { *this, "meter", W, H };
    jlook::SizeChip sizeChip { scaler };
    int selBand = 0, tick = 0;
    juce::RangedAudioParameter* P (const juce::String& id) { return proc.apvts.getParameter (id); }
    float bp (int b, const char* k) const { return proc.value (("b" + juce::String (b + 1) + "_" + k).toRawUTF8()); }
    juce::RangedAudioParameter* bpar (int b, const char* k) { return P ("b" + juce::String (b + 1) + "_" + k); }
    void setP (juce::RangedAudioParameter* p, float v) { p->setValueNotifyingHost (p->convertTo0to1 (v)); }
    SpecEq spec { *this };
    RefPanel refp { *this };
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
