#pragma once
// CREW: un quaderno di schizzi. Ogni voce e' un personaggio disegnato a inchiostro con i colori piatti fuori registro;
// si accende cliccandolo e quando canta apre la bocca.
#include "CrewProcessor.h"
#include "JackpotScale.h"
#include "NeoData.h"
#include "CrewArt.h"

namespace jcrew
{
namespace sk
{
const juce::Colour PAPER (0xffefe7d6), SHEET (0xfffbf8f1), INK (0xff1d1a1c), PENCIL (0xff8d8579), RED (0xffd9443a), YEL (0xfff3dd6f), BLUE (0xff8fb4d9),
                   BOARD (0xff9a6b3f), STEEL (0xffb9bcc2);
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
    juce::RangedAudioParameter* prm; juce::String label; juce::Colour fill; std::function<juce::String (float)> fmt; int seed;
    int steps = 0; bool bipolar = false; float last = -1;
private:
    float start = 0; bool drag = false;
};
// la carta di un personaggio: disegno (in cache), nome, e la bocca che si muove dal vivo
class Card : public juce::Component, public juce::SettableTooltipClient
{
public:
    Card (Editor& e, int k) : ed (e), idx (k) { setMouseCursor (juce::MouseCursor::PointingHandCursor); }
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseEnter (const juce::MouseEvent&) override { hover = true; repaint(); }
    void mouseExit (const juce::MouseEvent&) override { hover = false; repaint(); }
    Editor& ed; const int idx; float mouth = 0, lastMouth = -1; bool hover = false, lastOn = false;
    juce::Image cache; float cacheScale = 0; int cacheOn = -1;
    juce::Rectangle<float> headR() const { return { 0, 0, (float) getWidth(), 176 }; }
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
    juce::SharedResourcePointer<sk::Faces> faces;
    jlook::Scaler scaler { *this, "crew", W, H };
    jlook::SizeChip sizeChip { scaler };
    Root root { *this };
    std::vector<std::unique_ptr<Card>> cards;
    std::vector<std::unique_ptr<Knob>> knobs;
    juce::RangedAudioParameter* P (const juce::String& id) { return proc.apvts.getParameter (id); }
    enum Hot { H_PREV, H_NEXT, H_NAME, H_SAVE, H_UNDO, H_REDO, H_COUNT };
    juce::Rectangle<float> hotR[H_COUNT];
    void click (int);
    void presetMenu();
    void askSave();
    void drawStatic (juce::Graphics&);
    static void drawCharacter (juce::Graphics&, int k, juce::Rectangle<float> box, bool on);
    static void drawMouth (juce::Graphics&, int k, juce::Rectangle<float> box, bool on, float open, int tick);
    int tick = 0;
private:
    void timerCallback() override;
    int lastPreset = -2; juce::String lastUser;
};
}
