#pragma once
// VOX CHROME: un muro di cemento nero sotto la pioggia, stendardi del regime, telecamere.
// Al centro la testa cromata del cyborg: la fessura degli occhi si accende con la tua voce e lampeggia quando parte un throw.
// A sinistra l'UNITA' CROMO, a destra il PROTOCOLLO THROW, sotto la testa il monitor che mostra frasi e throw.
#include "VoxProcessor.h"
#include "JackpotScale.h"
#include "NeoData.h"
#include "VoxArt.h"
#include "CyberBrowser.h"

namespace jvox
{
namespace ui
{
const juce::Colour BLACK (0xff050506), CONC (0xff111114), CONC2 (0xff17171b), STEEL (0xff26272c), RED (0xffff1f2d), RED2 (0xff8a0c14), INK (0xffd9dbe0), DIM (0xff66686f), WARN (0xffe8c21a);
struct Faces
{
    juce::Typeface::Ptr heavy, raj, mono;
    Faces()
    {
        heavy = juce::Typeface::createSystemTypefaceFor (VoxArt::RubikMonoOneRegular_ttf, VoxArt::RubikMonoOneRegular_ttfSize);
        raj = juce::Typeface::createSystemTypefaceFor (VoxArt::RajdhaniBold_ttf, VoxArt::RajdhaniBold_ttfSize);
        mono = juce::Typeface::createSystemTypefaceFor (VoxArt::ShareTechMonoRegular_ttf, VoxArt::ShareTechMonoRegular_ttfSize);
    }
};
inline juce::Font heavy (float s, float k = 0.04f) { return juce::Font (juce::FontOptions (juce::SharedResourcePointer<Faces>()->heavy).withHeight (s)).withExtraKerningFactor (k); }
inline juce::Font raj (float s, float k = 0.08f) { return juce::Font (juce::FontOptions (juce::SharedResourcePointer<Faces>()->raj).withHeight (s)).withExtraKerningFactor (k); }
inline juce::Font mono (float s) { return juce::Font (juce::FontOptions (juce::SharedResourcePointer<Faces>()->mono).withHeight (s)); }
}

class Editor;
class Knob : public juce::Component, public juce::SettableTooltipClient
{
public:
    Knob (Editor&, const juce::String& id, juce::String label, std::function<juce::String (float)> fmt);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    Editor& ed; juce::RangedAudioParameter* prm; juce::String label; std::function<juce::String (float)> fmt;
    int steps = 0; bool bipolar = false; float last = -1;
private:
    float start = 0; bool drag = false;
};
class Root : public juce::Component
{
public:
    explicit Root (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override { hot = -1; repaint(); }
    Editor& ed; juce::Image bg; float bgScale = 0; int hot = -1;
};

class Editor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit Editor (Processor&);
    ~Editor() override { stopTimer(); }
    void paint (juce::Graphics&) override {}
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;
    static constexpr int W = 1180, H = 720;
    Processor& proc;
    juce::SharedResourcePointer<ui::Faces> faces;
    jlook::Scaler scaler { *this, "voxchrome", W, H };
    jlook::SizeChip sizeChip { scaler };
    Root root { *this };
    std::vector<std::unique_ptr<Knob>> knobs;
    jcyber::Browser browser;
    juce::RangedAudioParameter* P (const juce::String& id) { return proc.apvts.getParameter (id); }
    enum Hot { H_PREV, H_NEXT, H_NAME, H_SAVE, H_UNDO, H_REDO, H_COUNT };
    juce::Rectangle<float> hotR[H_COUNT];
    void click (int);
    void askSave();
    void drawStatic (juce::Graphics&);
    void drawLive (juce::Graphics&);
    static void drawHead (juce::Graphics&, juce::Rectangle<float>);
    int tick = 0; float eye = 0, flash = 0, lastEye = -1; int lastThrows = 0; float ticker = 0;
    std::vector<float> envHist; std::vector<bool> throwHist; float gateNow = 0;
private:
    void timerCallback() override;
    int lastPreset = -2; juce::String lastUser;
};
}
