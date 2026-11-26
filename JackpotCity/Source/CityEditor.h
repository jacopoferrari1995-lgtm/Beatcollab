#pragma once
// CITY EDGERUNNER: la citta' di notte. In alto lo skyline con le sei insegne al neon (le scene: clicchi l'insegna e ci entri),
// sotto la strada bagnata che le riflette, e il chiosco di vetro con il comando grande DENTRO LA CITTA' e le manopole.
#include "CityProcessor.h"
#include "JackpotScale.h"
#include "NeoData.h"
#include "CityArt.h"
#include "CyberBrowser.h"

namespace jcity
{
namespace ui
{
const juce::Colour SKY0 (0xff06040e), SKY1 (0xff1c0b2c), MAG (0xffff2fa0), CYAN (0xff36f0ff), ACID (0xffd4ff3a), AMBER (0xffffa526), INK (0xffe9ecf6), DIM (0xff6b6f86), GLASS (0xd00a0b16);
struct Faces
{
    juce::Typeface::Ptr neon, raj, mono;
    Faces()
    {
        neon = juce::Typeface::createSystemTypefaceFor (CityArt::MonotonRegular_ttf, CityArt::MonotonRegular_ttfSize);
        raj = juce::Typeface::createSystemTypefaceFor (CityArt::RajdhaniBold_ttf, CityArt::RajdhaniBold_ttfSize);
        mono = juce::Typeface::createSystemTypefaceFor (CityArt::ShareTechMonoRegular_ttf, CityArt::ShareTechMonoRegular_ttfSize);
    }
};
inline juce::Font neon (float s) { return juce::Font (juce::FontOptions (juce::SharedResourcePointer<Faces>()->neon).withHeight (s)); }
inline juce::Font raj (float s, float k = 0.06f) { return juce::Font (juce::FontOptions (juce::SharedResourcePointer<Faces>()->raj).withHeight (s)).withExtraKerningFactor (k); }
inline juce::Font mono (float s) { return juce::Font (juce::FontOptions (juce::SharedResourcePointer<Faces>()->mono).withHeight (s)); }
juce::Colour sceneColour (int k);
}

class Editor;
class Knob : public juce::Component, public juce::SettableTooltipClient
{
public:
    Knob (Editor&, const juce::String& id, juce::String label, juce::Colour c, std::function<juce::String (float)> fmt);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    Editor& ed; juce::RangedAudioParameter* prm; juce::String label; juce::Colour col; std::function<juce::String (float)> fmt;
    bool big = false, bipolar = false; float last = -1;
private:
    float start = 0; bool drag = false;
};
class Rain : public juce::Component
{
public:
    Rain() { setInterceptsMouseClicks (false, false); }
    void paint (juce::Graphics&) override;
    struct Drop { float x, y, len, sp, a; };
    std::vector<Drop> drops; float amount = 0.5f;
    void step();
};
class Root : public juce::Component
{
public:
    explicit Root (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override { hot = -1; repaint(); }
    Editor& ed; juce::Image bg, city; float bgScale = 0, cityScale = 0; int cityKey = -1; int hot = -1;
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
    jlook::Scaler scaler { *this, "city", W, H };
    jlook::SizeChip sizeChip { scaler };
    Root root { *this };
    Rain rain;
    std::vector<std::unique_ptr<Knob>> knobs;
    jcyber::Browser browser;
    juce::RangedAudioParameter* P (const juce::String& id) { return proc.apvts.getParameter (id); }
    enum Hot { H_PREV, H_NEXT, H_NAME, H_SAVE, H_UNDO, H_REDO, H_SCENE0, H_COUNT = H_SCENE0 + kScenes };
    juce::Rectangle<float> hotR[H_COUNT];
    static juce::Rectangle<float> signRect (int k);
    int scene() const { return juce::jlimit (0, kScenes - 1, juce::roundToInt (proc.value ("scene"))); }
    void click (int);
    void askSave();
    void drawStatic (juce::Graphics&);
    void drawCity (juce::Graphics&);   // cielo, palazzi, insegne e riflessi (in cache finche' non cambia scena)
    void drawHud (juce::Graphics&);
    int tick = 0, flick = 0; float inLvl = 0, outLvl = 0, gate = 1, lastOut = -1, lastGate = -1;
private:
    void timerCallback() override;
    int lastPreset = -2, lastScene = -1; juce::String lastUser;
};
}
