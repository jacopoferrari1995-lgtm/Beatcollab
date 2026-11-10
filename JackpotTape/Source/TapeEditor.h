#pragma once
#include "JackpotLook.h"
#include "JackpotScale.h"
#include "TapeProcessor.h"
#include <set>

namespace jt
{
namespace K
{
    const juce::Colour bg { 0xff0b0c10 }, win { 0xff121419 }, p1 { 0xff1d2027 }, p2 { 0xff16181e }, p3 { 0xff2a2d36 };
    const juce::Colour hair { 0x14ffffff }, hair2 { 0x24ffffff }, ink { 0xffdfe3ea }, dim { 0xff7d8494 }, faint { 0xff4f5562 };
    const juce::Colour brand1 { 0xffff2e88 }, brand2 { 0xff5ef2ff }, amber { 0xffffb000 }, lcd1 { 0xff05161a }, lcd2 { 0xff020b0d };
}
juce::Font tf (float size, int weight = 0, bool mono = false);   // weight 0 normale, 1 semi, 2 bold

class Editor;

// manopola moderna legata a un parametro
class Knob : public juce::Component
{
public:
    Knob (juce::RangedAudioParameter* p, juce::String label, juce::Colour c, std::function<juce::String (float)> fmt = {}) : param (p), lab (std::move (label)), col (c), fmtFn (std::move (fmt)) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void mouseEnter (const juce::MouseEvent&) override { hover = true; setMouseCursor (juce::MouseCursor::UpDownResizeCursor); repaint(); }
    void mouseExit (const juce::MouseEvent&) override { hover = false; repaint(); }
    bool hover = false, drag = false;
    std::function<void (float)> onUserChange;
    std::function<void()> onStart;   // inizio di un gesto: per annulla
    juce::RangedAudioParameter* param; juce::String lab; juce::Colour col; std::function<juce::String (float)> fmtFn;
private:
    float startV = 0; void setNorm (float v);
};

// interruttore a segmenti
class Seg : public juce::Component
{
public:
    Seg (juce::StringArray items, std::function<int()> get, std::function<void (int)> set) : it (std::move (items)), getter (std::move (get)), setter (std::move (set)) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    int preferredWidth() const;
    juce::StringArray it; std::function<int()> getter; std::function<void (int)> setter;
};

class Toggle : public juce::Component
{
public:
    Toggle (std::function<bool()> g, std::function<void()> t) : get (std::move (g)), tog (std::move (t)) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { tog(); repaint(); }
    std::function<bool()> get; std::function<void()> tog;
};

class TextBtn : public juce::Component
{
public:
    TextBtn (juce::String t, std::function<void()> c) : text (std::move (t)), click (std::move (c)) {}
    void paint (juce::Graphics&) override;
    void mouseUp (const juce::MouseEvent& e) override { if (contains (e.getPosition()) && click) click(); }
    void mouseEnter (const juce::MouseEvent&) override { hover = true; repaint(); }
    void mouseExit (const juce::MouseEvent&) override { hover = false; repaint(); }
    juce::String text; std::function<void()> click; bool hover = false, round = false, accent = false;
};

class FxList : public juce::Component
{
public:
    explicit FxList (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    Editor& ed;
};

class Display : public juce::Component
{
public:
    explicit Display (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    Editor& ed; float reelA = 0;
};

class PadBtn : public juce::Component
{
public:
    PadBtn (Editor& e, int f) : ed (e), fx (f) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    Editor& ed; int fx; bool down = false;
};

class FxEditor : public juce::Component
{
public:
    explicit FxEditor (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    void resized() override;
    void rebuild();
    juce::Rectangle<int> curveArea() const;
    float paramSig() const;
    Editor& ed; juce::OwnedArray<Knob> knobs; std::unique_ptr<Toggle> onTog; float lastSig = 0;
};

class Timeline : public juce::Component
{
public:
    explicit Timeline (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override { mode = NONE; }
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override { hLane = hStep = -1; repaint(); }
    void mouseDoubleClick (const juce::MouseEvent&) override;
    bool cellAt (juce::Point<float>, int& lane, int& step) const;
    int rawStep (float x) const;
    bool blockAt (int lane, int step, int& s, int& e) const;
    void setRange (int lane, int a, int b, bool v);
    void deleteSelected();
    juce::Rectangle<float> grid() const;
    int span() const;
    enum Mode { NONE, CREATE, ERASE, MOVE, RESL, RESR };
    Editor& ed; Mode mode = NONE; int dLane = -1, anchor = 0, dOff = 0, bS = 0, bE = 0, hLane = -1, hStep = -1;
    int selLane = -1, selS = 0, selE = 0;
    bool before[kMaxSteps] = {};
};

class Meter : public juce::Component
{
public:
    explicit Meter (Processor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    Processor& proc;
};

class Root : public juce::Component
{
public:
    explicit Root (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    Editor& ed;
};

class Editor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit Editor (Processor&);
    ~Editor() override;
    void resized() override;
    void paint (juce::Graphics& g) override { g.fillAll (K::bg); }
    bool keyPressed (const juce::KeyPress&) override;
    bool keyStateChanged (bool) override;

    jlook::LnF lnf;
    Processor& proc;
    jlook::Scaler scaler { *this, "tape", W, H };
    jlook::SizeChip sizeChip { scaler };
    int sel = STUT, res = 0;   // risoluzione di disegno: 0 1/16, 1 1/8, 2 1/4
    void select (int f);
    int steps() const;
    juce::Rectangle<int> rightPanel, sidePanel;
    static constexpr int W = 1240, H = 940;

    Root root { *this };
    FxList list { *this };
    Display disp { *this };
    FxEditor fxEd { *this };
    Timeline tl { *this };
    Meter meter { proc };
    juce::OwnedArray<PadBtn> pads;
    juce::OwnedArray<Knob> tapeKnobs;
    std::unique_ptr<Seg> modeSeg, qSeg, barsSeg, resSeg, genSeg;
    std::unique_ptr<Toggle> seqTog;
    std::unique_ptr<TextBtn> pPrev, pNext, pName, bpmPill, clearBtn, styleBtn, genBtn, abBtn, undoBtn, redoBtn;
    // annulla / rifai: stato dei parametri + coreografia
    struct Snap { juce::ValueTree st; juce::String seq; };
    std::vector<Snap> undoS, redoS;
    Snap snap() const { return { proc.apvts.copyState(), proc.seqString() }; }
    void pushUndo();
    void restore (const Snap&);
    void undo(); void redo();
    void refreshAll();

private:
    void timerCallback() override;
    std::set<int> keysDown;
};
}
