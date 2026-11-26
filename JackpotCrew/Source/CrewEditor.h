#pragma once
// CREW, due temi:
//  OLOGRAMMI: una stanza buia, al centro tu (solido, cyber), intorno e dietro la crew proiettata come ologrammi,
//             ognuno sul suo proiettore con il suo pannello di manopole.
//  QUADERNO:  il quaderno di schizzi, ma sporco e notturno: carta scura, gesso, neon, graffi.
#include "CrewProcessor.h"
#include "JackpotScale.h"
#include "NeoData.h"
#include "CrewArt.h"

namespace jcrew
{
namespace sk
{
// quaderno sporco: carta scura, tratti a gesso, neon
const juce::Colour PAPER (0xff121116), SHEET (0xff211f27), INK (0xffe9e4d8), PENCIL (0xff5f5a69), RED (0xffff2f6d), YEL (0xffd8ff3a), BLUE (0xff2fd6e6),
                   BOARD (0xff2a2320), STEEL (0xff8d929c), DARKF (0xff0c0b0f);
// ologrammi
const juce::Colour ROOM0 (0xff07080e), ROOM1 (0xff0e1020), CYAN (0xff38f2ff), MAG (0xffff2fa0), ACID (0xffd8ff3a), GLASS (0xcc0a0d18), METAL (0xff1b1d26);
struct Faces
{
    juce::Typeface::Ptr bungee, bebas, mono;
    Faces()
    {
        bungee = juce::Typeface::createSystemTypefaceFor (CrewArt::BungeeRegular_ttf, CrewArt::BungeeRegular_ttfSize);
        bebas = juce::Typeface::createSystemTypefaceFor (CrewArt::BebasNeueRegular_ttf, CrewArt::BebasNeueRegular_ttfSize);
        mono = juce::Typeface::createSystemTypefaceFor (NeoData::SpaceMonoBold_ttf, NeoData::SpaceMonoBold_ttfSize);
    }
};
inline juce::Font bun (float s) { return juce::Font (juce::FontOptions (juce::SharedResourcePointer<Faces>()->bungee).withHeight (s)); }
inline juce::Font beb (float s, float k = 0.04f) { return juce::Font (juce::FontOptions (juce::SharedResourcePointer<Faces>()->bebas).withHeight (s)).withExtraKerningFactor (k); }
inline juce::Font mon (float s) { return juce::Font (juce::FontOptions (juce::SharedResourcePointer<Faces>()->mono).withHeight (s)); }
juce::Colour accent (int k);
juce::Colour holoTint (int k);
}

class Editor;
class Knob : public juce::Component, public juce::SettableTooltipClient
{
public:
    Knob (Editor&, const juce::String& id, juce::String label, juce::Colour fill, std::function<juce::String (float)> fmt, int seed);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    Editor& ed; juce::RangedAudioParameter* prm; juce::String label; juce::Colour fill; std::function<juce::String (float)> fmt; int seed;
    int steps = 0; bool bipolar = false; float last = -1; int voice = -1;
private:
    float start = 0; bool drag = false;
};
// un personaggio: carta (quaderno) o proiezione (ologrammi); la bocca / la luce si muovono dal vivo
class Card : public juce::Component, public juce::SettableTooltipClient
{
public:
    Card (Editor& e, int k) : ed (e), idx (k) { setMouseCursor (juce::MouseCursor::PointingHandCursor); }
    void paint (juce::Graphics&) override;
    void paintSketch (juce::Graphics&, bool on);
    void paintHolo (juce::Graphics&, bool on);
    void mouseDown (const juce::MouseEvent&) override;
    void mouseEnter (const juce::MouseEvent&) override { hover = true; repaint(); }
    void mouseExit (const juce::MouseEvent&) override { hover = false; repaint(); }
    Editor& ed; const int idx; float mouth = 0, lastMouth = -1; bool hover = false, lastOn = false;
    juce::Image cache; float cacheScale = 0; int cacheKey = -1;
    juce::Image holo; float holoScale = 0;   // il personaggio gia' trasformato in luce
    float glitch = 0; int glitchSeed = 0;
};
class Root : public juce::Component
{
public:
    explicit Root (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override { hot = -1; repaint(); }
    Editor& ed; juce::Image bg; float bgScale = 0; int bgTheme = -1; int hot = -1;
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
    juce::SharedResourcePointer<sk::Faces> faces;
    jlook::Scaler scaler { *this, "crew", W, H };
    jlook::SizeChip sizeChip { scaler };
    Root root { *this };
    std::vector<std::unique_ptr<Card>> cards;
    std::vector<std::unique_ptr<Knob>> knobs;   // 4 per personaggio, poi 8 della crew
    juce::RangedAudioParameter* P (const juce::String& id) { return proc.apvts.getParameter (id); }
    enum Hot { H_PREV, H_NEXT, H_NAME, H_SAVE, H_UNDO, H_REDO, H_THEME, H_COUNT };
    juce::Rectangle<float> hotR[H_COUNT];
    bool holoTheme() const { return proc.uiTheme == 0; }
    void layout();
    void click (int);
    void presetMenu();
    void askSave();
    void drawSketchStatic (juce::Graphics&);
    void drawHoloStatic (juce::Graphics&);
    void drawMain (juce::Graphics&, float lvl);   // tu, al centro della stanza
    static void drawCharacter (juce::Graphics&, int k, juce::Rectangle<float> box, bool on);
    static void drawMouth (juce::Graphics&, int k, juce::Rectangle<float> box, bool on, float open, int tick);
    static juce::Rectangle<float> holoSlot (int k);
    int tick = 0; float inLvl = 0, lastIn = -1; float signFlicker = 1;
private:
    void timerCallback() override;
    int lastPreset = -2; juce::String lastUser;
};
}
