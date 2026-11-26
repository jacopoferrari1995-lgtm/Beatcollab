#pragma once
// PHANTOM: una strada magenta anni '90, palazzi blu petrolio, cavi, neon. In mezzo tu, col cappuccio e gli occhiali ciano;
// intorno i tuoi quattro fantasmi, trasparenti e glitchati, che si accendono quando li alzi e tremano quando suonano.
// Sotto, quattro vecchi televisori (uno per fantasma) e l'impianto con i comandi comuni.
#include "PhantomProcessor.h"
#include "JackpotScale.h"
#include "NeoData.h"
#include "PhantomArt.h"
#include "CyberBrowser.h"

namespace jphn
{
namespace ui
{
const juce::Colour MAG (0xffff2e97), PINK (0xffff79c9), TEAL (0xff22f3dc), DEEP (0xff12071f), NAVY (0xff080b1c), INK (0xffeef0ff), DIM (0xff7a6f93), VIOLET (0xff7b4dff);
struct Faces
{
    juce::Typeface::Ptr krona, vt, raj;
    Faces()
    {
        krona = juce::Typeface::createSystemTypefaceFor (PhantomArt::KronaOne_ttf, PhantomArt::KronaOne_ttfSize);
        vt = juce::Typeface::createSystemTypefaceFor (PhantomArt::VT323Regular_ttf, PhantomArt::VT323Regular_ttfSize);
        raj = juce::Typeface::createSystemTypefaceFor (PhantomArt::RajdhaniBold_ttf, PhantomArt::RajdhaniBold_ttfSize);
    }
};
inline juce::Font krona (float s, float k = 0.04f) { return juce::Font (juce::FontOptions (juce::SharedResourcePointer<Faces>()->krona).withHeight (s)).withExtraKerningFactor (k); }
inline juce::Font vt (float s) { return juce::Font (juce::FontOptions (juce::SharedResourcePointer<Faces>()->vt).withHeight (s)); }
inline juce::Font raj (float s, float k = 0.08f) { return juce::Font (juce::FontOptions (juce::SharedResourcePointer<Faces>()->raj).withHeight (s)).withExtraKerningFactor (k); }
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
    int steps = 0; bool bipolar = false; float last = -1; juce::Colour col { 0xffff2e97 };
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
    jlook::Scaler scaler { *this, "phantom", W, H };
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
    static juce::Path figure (juce::Rectangle<float> box, bool flip = false);
    static juce::Rectangle<float> ghostBox (int k);
    static juce::Rectangle<float> tvRect (int k);
    int tick = 0; std::array<float, kGhosts> glow {}; std::array<std::vector<float>, kGhosts> scope; float inLvl = 0;
private:
    void timerCallback() override;
    int lastPreset = -2; juce::String lastUser;
};
}
