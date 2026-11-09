#pragma once
#include "PadProcessor.h"
#include <set>
#include <map>

namespace jp
{
namespace Col
{
    const juce::Colour bg { 0xff0a0b0e }, chassis1 { 0xff1c1f26 }, chassis2 { 0xff101217 }, inset { 0xff08090c };
    const juce::Colour magenta { 0xffff2e88 }, cyan { 0xff19e3ff }, amber { 0xffffb000 }, red { 0xffff3b4e }, green { 0xff3dff8b };
    const juce::Colour text { 0xffd9dde6 }, dim { 0xff7d8494 }, faint { 0xff4a505d };
    const juce::Colour lcd1 { 0xff041619 }, lcd2 { 0xff020b0d }, phos { 0xff5ef2ff };
}
juce::Colour padColour (int i);
juce::Font ft (float size, bool bold = false, bool mono = false);

class Editor;

// manopola 3D con anello di LED
class Knob : public juce::Component
{
public:
    Knob (juce::String l, juce::Colour c) : label (std::move (l)), col (c) {}
    std::function<float()> get;                 // normalizzato 0..1
    std::function<void (float)> set;
    std::function<juce::String()> text;
    std::function<void()> reset;
    bool bipolar = false;
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { start = get(); dragging = true; }
    void mouseUp (const juce::MouseEvent&) override { dragging = false; repaint(); }
    void mouseDrag (const juce::MouseEvent& e) override { set (juce::jlimit (0.0f, 1.0f, start - e.getDistanceFromDragStartY() / (e.mods.isShiftDown() ? 800.0f : 170.0f))); repaint(); }
    void mouseDoubleClick (const juce::MouseEvent&) override { if (reset) reset(); repaint(); }
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w) override { set (juce::jlimit (0.0f, 1.0f, get() + (w.deltaY > 0 ? 0.01f : -0.01f))); repaint(); }
    juce::String label; juce::Colour col; float start = 0; bool dragging = false;
};

// selettore LCD (clic avanza, clic destro torna, rotella)
class Selector : public juce::Component
{
public:
    Selector (juce::String l) : label (std::move (l)) {}
    std::function<int()> get; std::function<void (int)> set; std::function<int()> count; std::function<juce::String (int)> name;
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent& e) override { const int n = count(); set ((get() + (e.mods.isPopupMenu() || e.mods.isShiftDown() ? n - 1 : 1)) % n); repaint(); }
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w) override { const int n = count(); set ((get() + (w.deltaY > 0 ? 1 : n - 1)) % n); repaint(); }
    juce::String label; juce::Colour col { Col::cyan };
};

// tasto in gomma con LED
class Btn : public juce::Component
{
public:
    Btn (juce::String t, juce::Colour led) : label (std::move (t)), ledCol (led) {}
    std::function<bool()> lit; std::function<void()> click;
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { down = true; repaint(); }
    void mouseUp (const juce::MouseEvent& e) override { down = false; repaint(); if (contains (e.getPosition()) && click) click(); }
    juce::String label; juce::Colour ledCol; bool down = false;
};

class PadComp : public juce::Component, public juce::FileDragAndDropTarget
{
public:
    PadComp (Editor& e, int p) : ed (e), pad (p) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    bool isInterestedInFileDrag (const juce::StringArray&) override { return true; }
    void filesDropped (const juce::StringArray&, int, int) override;
    void fileDragEnter (const juce::StringArray&, int, int) override { dragOver = true; repaint(); }
    void fileDragExit (const juce::StringArray&) override { dragOver = false; repaint(); }
    juce::Rectangle<float> body() const { return getLocalBounds().toFloat().reduced (8); }
    Editor& ed; int pad; bool held = false, dragOver = false; float glow = 0, flash = 0; juce::uint32 lastHit = 0;
};

class Lcd : public juce::Component
{
public:
    explicit Lcd (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    juce::Rectangle<float> wave() const;
    Editor& ed; int marker = -1;
};

class PatternView : public juce::Component
{
public:
    explicit PatternView (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    Editor& ed;
};

class Keys : public juce::Component
{
public:
    explicit Keys (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    int noteAt (juce::Point<float>) const;
    Editor& ed; int mouseNote = -1;
};

class DragMidi : public juce::Component
{
public:
    explicit DragMidi (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    Editor& ed; bool dragging = false;
};

class Root : public juce::Component
{
public:
    explicit Root (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    Editor& ed;
};

class Editor : public juce::AudioProcessorEditor, public juce::DragAndDropContainer, private juce::Timer
{
public:
    explicit Editor (Processor&);
    ~Editor() override = default;
    void resized() override;
    void paint (juce::Graphics& g) override { g.fillAll (Col::bg); }
    bool keyPressed (const juce::KeyPress&) override;
    bool keyStateChanged (bool) override;
    void selectPad (int p);
    void playPad (int p, int vel);
    void stopPad (int p);
    int keyNote (int keyCode) const;        // nota dalla tastiera del computer, -1 se non mappata
    static constexpr int W = 1240, H = 840;

    Processor& proc;
    int sel = 0, octave = 1, kbVel = 100;   // ottava: C1 = nota 36
    std::set<int> heldNotes;
    std::set<int> keysDown;
    std::unique_ptr<juce::FileChooser> chooser;
    void loadDialog (int p);
    void padMenu (int p);

    Root root { *this };
    Lcd lcd { *this };
    PatternView pattern { *this };
    Keys keys { *this };
    DragMidi drag { *this };
    juce::OwnedArray<PadComp> pads;
    juce::OwnedArray<Knob> padKnobs, globalKnobs;
    juce::OwnedArray<Selector> selectors;
    juce::OwnedArray<Btn> btns;
    std::map<int, juce::Component*> padCtl;   // indice parametro -> controllo
    bool fullLevel = false;

private:
    void timerCallback() override;
    Knob* paramKnob (const char* id, const juce::String& label, juce::Colour c, std::function<juce::String (float)> fmt);
};
}
