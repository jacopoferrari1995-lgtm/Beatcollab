#pragma once
// TAG FORGE: estetica dai poster "cyber techwear" (illustrazione piatta a inchiostro nero, giallo, magenta, viola;
// personaggio col cappuccio e la maschera antigas, schizzi, retino, adesivi e toppe).
#include "TagProcessor.h"
#include "JackpotScale.h"
#include "NeoData.h"
#include "TagArt.h"

namespace jtag
{
namespace ink
{
const juce::Colour Y (0xfff6dc1e), Y2 (0xffd9a90c), M (0xffe6177e), M2 (0xffa80d58), PU (0xff3d1d63), P2 (0xff7a4bb5), P3 (0xffbf9be6),
                   K (0xff141016), WH (0xfff5efe4), G (0xff3d3547), T (0xff27d8c9);
struct Faces
{
    juce::Typeface::Ptr bungee, archivo, mono;
    Faces()
    {
        bungee = juce::Typeface::createSystemTypefaceFor (TagArt::BungeeRegular_ttf, TagArt::BungeeRegular_ttfSize);
        archivo = juce::Typeface::createSystemTypefaceFor (NeoData::ArchivoBlackRegular_ttf, NeoData::ArchivoBlackRegular_ttfSize);
        mono = juce::Typeface::createSystemTypefaceFor (NeoData::SpaceMonoBold_ttf, NeoData::SpaceMonoBold_ttfSize);
    }
};
inline juce::Font bun (float s) { return juce::Font (juce::FontOptions (juce::SharedResourcePointer<Faces>()->bungee).withHeight (s)); }
inline juce::Font arc (float s, float k = 0.02f) { return juce::Font (juce::FontOptions (juce::SharedResourcePointer<Faces>()->archivo).withHeight (s)).withExtraKerningFactor (k); }
inline juce::Font mon (float s) { return juce::Font (juce::FontOptions (juce::SharedResourcePointer<Faces>()->mono).withHeight (s)); }
}

class Editor;
// manopola da fumetto: contorno a inchiostro, cappuccio colorato piatto, lancetta spessa, arco del valore
class Knob : public juce::Component, public juce::SettableTooltipClient
{
public:
    Knob (Editor& e, const char* id, juce::String label, juce::Colour cap, juce::Colour inkOn, std::function<juce::String (float)> fmt);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    juce::RangedAudioParameter* prm; juce::String label; juce::Colour cap, on; std::function<juce::String (float)> fmt;
    int steps = 0; bool bipolar = false; float last = -1;
    juce::Colour labelInk { 0xff141016 }, valueBox { 0xff141016 }, valueInk { 0xfff6dc1e };
private:
    float start = 0; bool drag = false;
};
// lo schermo: la frase registrata (sopra, con il taglio) e il tag forgiato (sotto, con la testina)
class Screen : public juce::Component, public juce::FileDragAndDropTarget
{
public:
    explicit Screen (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override { handle = -1; }
    void mouseMove (const juce::MouseEvent&) override;
    bool isInterestedInFileDrag (const juce::StringArray&) override;
    void filesDropped (const juce::StringArray&, int, int) override;
    void fileDragEnter (const juce::StringArray&, int, int) override { dropHover = true; repaint(); }
    void fileDragExit (const juce::StringArray&) override { dropHover = false; repaint(); }
    void refresh();
    Editor& ed; std::vector<float> src, tag; float tagSec = 0, srcSec = 0; int gen = -1, handle = -1, hoverH = -1; bool dropHover = false;
    juce::Rectangle<float> srcR() const, tagR() const;
};
// tutto il plugin in un componente a dimensione fissa, ingrandito dallo zoom
class Root : public juce::Component
{
public:
    explicit Root (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override { dragOut = false; }
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override { hot = -1; repaint(); }
    Editor& ed; juce::Image bg; float bgScale = 0; int hot = -1; bool dragOut = false;
};

class Editor : public juce::AudioProcessorEditor, public juce::DragAndDropContainer, public juce::FileDragAndDropTarget, private juce::Timer
{
public:
    explicit Editor (Processor&);
    ~Editor() override;
    void paint (juce::Graphics&) override {}
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;
    bool isInterestedInFileDrag (const juce::StringArray& f) override { return screen.isInterestedInFileDrag (f); }
    void filesDropped (const juce::StringArray& f, int x, int y) override { screen.filesDropped (f, x, y); }
    static constexpr int W = 1180, H = 720;
    Processor& proc;
    juce::SharedResourcePointer<ink::Faces> faces;
    jlook::Scaler scaler { *this, "tagforge", W, H };
    jlook::SizeChip sizeChip { scaler };
    Root root { *this };
    Screen screen { *this };
    std::vector<std::unique_ptr<Knob>> knobs;
    juce::RangedAudioParameter* P (const char* id) { return proc.apvts.getParameter (id); }
    // zone cliccabili (coordinate del progetto)
    enum Hot { H_PREV, H_NEXT, H_NAME, H_SAVE, H_REC, H_PLAY, H_LOAD, H_EXPORT, H_DRAG, H_NOTE, H_REV, H_UNDO, H_REDO, H_COUNT };
    juce::Rectangle<float> hotR[H_COUNT];
    void click (int h, const juce::MouseEvent&);
    void presetMenu();
    void askSave();
    void drawStatic (juce::Graphics&);
    void drawCharacter (juce::Graphics&);
    float lvl = 0, recPulse = 0; int tick = 0; juce::String flash; int flashT = 0;
    struct Puff { float x, y, vx, vy, r, a; }; std::vector<Puff> puffs;
    std::unique_ptr<juce::FileChooser> chooser;
private:
    void timerCallback() override;
    int lastPreset = -2; juce::String lastUser;
};
}
