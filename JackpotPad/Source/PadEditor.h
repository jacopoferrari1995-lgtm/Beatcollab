#pragma once
#include "JackpotLook.h"
#include "JackpotScale.h"
#include "PadProcessor.h"
#include "Library.h"
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

class Knob : public juce::Component
{
public:
    Knob (juce::String l, juce::Colour c) : label (std::move (l)), col (c) {}
    std::function<float()> get;
    std::function<void (float)> set;
    std::function<juce::String()> text;
    std::function<void()> reset;
    bool bipolar = false;
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent& e) override { if (! get) return; start = get(); dragging = true; if ((e.mods.isAltDown() || e.mods.isCommandDown()) && reset) { reset(); dragging = false; } }
    void mouseEnter (const juce::MouseEvent&) override { hover = true; setMouseCursor (juce::MouseCursor::UpDownResizeCursor); repaint(); }
    void mouseExit (const juce::MouseEvent&) override { hover = false; repaint(); }
    void mouseUp (const juce::MouseEvent&) override { dragging = false; repaint(); }
    void mouseDrag (const juce::MouseEvent& e) override { if (! dragging) return; if (set) set (juce::jlimit (0.0f, 1.0f, start - e.getDistanceFromDragStartY() / (e.mods.isShiftDown() ? 800.0f : 170.0f))); repaint(); }
    void mouseDoubleClick (const juce::MouseEvent&) override { if (reset) reset(); repaint(); }
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w) override { if (set) set (juce::jlimit (0.0f, 1.0f, get() + (w.deltaY > 0 ? 0.01f : -0.01f))); repaint(); }
    juce::String label; juce::Colour col; float start = 0; bool dragging = false, hover = false;
};

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

enum Page { PG_KIT, PG_BROWSE, PG_MIX, PG_SAMPLE, PG_VEL, PG_EQ, PG_FILTER, PG_COMP, PG_FX, PG_MASTER, kNumPages };

// schermo a pagine
class Screen : public juce::Component, public juce::FileDragAndDropTarget
{
public:
    explicit Screen (Editor& e) : ed (e) { setupSearch(); }
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override { drag = nullptr; }
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    bool isInterestedInFileDrag (const juce::StringArray&) override { return true; }
    void filesDropped (const juce::StringArray&, int, int) override;

    struct Hot
    {
        juce::Rectangle<float> r;
        std::function<void (const juce::MouseEvent&)> down;
        std::function<void (const juce::MouseEvent&)> drag;
        std::function<void (float)> wheel;
        std::function<void()> dbl;
    };
    std::vector<Hot> hots;
    std::function<void (const juce::MouseEvent&)> drag;
    Editor& ed; int kitScroll = 0, randMode = 0;
    // chop
    int chopMode = 0, chopDiv = 1; bool chopSnap = true; int dragMark = -1; float lastFx = 0;
    std::vector<float> marks, onsets; const SampleData* chopSmp = nullptr;
    void buildChop();
    float snapPos (float fx) const;
    std::vector<float> gridLines() const;
    juce::Rectangle<float> content() const { return getLocalBounds().toFloat().reduced (12).withTrimmedTop (56); }

private:
    void chip (juce::Graphics&, juce::Rectangle<float>, const juce::String& label, const juce::String& value, bool on, std::function<void (const juce::MouseEvent&)> click, juce::Colour c);
    void padHeader (juce::Graphics&, juce::Rectangle<float>);
    void pageKit (juce::Graphics&, juce::Rectangle<float>);
    void pageMix (juce::Graphics&, juce::Rectangle<float>);
    void pageSample (juce::Graphics&, juce::Rectangle<float>);
    void pageVel (juce::Graphics&, juce::Rectangle<float>);
    void pageEq (juce::Graphics&, juce::Rectangle<float>);
    void pageFilter (juce::Graphics&, juce::Rectangle<float>);
    void pageComp (juce::Graphics&, juce::Rectangle<float>);
    void pageFx (juce::Graphics&, juce::Rectangle<float>);
    void pageMaster (juce::Graphics&, juce::Rectangle<float>);
    void pageBrowse (juce::Graphics&, juce::Rectangle<float>);
public:
    int browseCat = 0, browseScroll = 0, browseGen = -1; bool browseDirty = true;
    std::vector<struct LibItem> browseItems;
    juce::TextEditor search;
    void setupSearch();
    void itemMenu (int i);
};

class PatternView : public juce::Component
{
public:
    explicit PatternView (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    Editor& ed;
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

// editor del campione a tutto schermo: BPM, zoom, chop, assegnazione delle fette ai pad
class ChopView : public juce::Component
{
public:
    explicit ChopView (Editor& e) : ed (e) { setOpaque (true); }
    void open (int pad);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    bool keyPressed (const juce::KeyPress&) override;
    Editor& ed; int src = 0; SamplePtr smp; int colour = 0; float gain = 0;
    float r0 = 0, r1 = 1, v0 = 0, v1 = 1, bpm = 0; int warp = 0;
    int mode = 0, div = 2, sens = 1; bool snap = true; int firstPad = 0, selSlice = -1, drag = 0, hovSlice = -1;   // drag: 0 nulla, 1+ marker, -1 inizio, -2 fine, -3 panoramica
    std::vector<float> marks, onsets, pkMin, pkMax; float lastFx = 0, dragOff = 0;
    std::map<int, int> assigned;   // pad -> fetta
    struct Hot { juce::Rectangle<float> r; std::function<void (const juce::MouseEvent&)> fn; std::function<void (float)> wheel; };
    std::vector<Hot> hots;
    juce::Rectangle<float> wave() const { return { 36, 156, 1168, 270 }; }
    juce::Rectangle<float> overview() const { return { 36, 436, 1168, 26 }; }
    float xOf (float fx) const { auto w = wave(); return w.getX() + (fx - v0) / (v1 - v0) * w.getWidth(); }
    float fxOf (float x) const { auto w = wave(); return v0 + (x - w.getX()) / w.getWidth() * (v1 - v0); }
    std::vector<float> points() const { std::vector<float> p { r0 }; for (float m : marks) if (m > r0 && m < r1) p.push_back (m); p.push_back (r1); return p; }
    std::vector<float> grid() const;
    void rebuild();
    float snapTo (float fx) const;
    void close();
    void paintChrome (juce::Graphics&);
    juce::Image chrome; float chromeScale = 0; bool chromeDirty = true;
    juce::Path wavePath; float pathV0 = -1, pathV1 = -1;
};

class Editor : public juce::AudioProcessorEditor, public juce::DragAndDropContainer, private juce::Timer
{
public:
    explicit Editor (Processor&);
    ~Editor() override { setLookAndFeel (nullptr); }
    void resized() override;
    void paint (juce::Graphics& g) override { g.fillAll (Col::bg); }
    bool keyPressed (const juce::KeyPress&) override;
    bool keyStateChanged (bool) override;
    void selectPad (int p);
    void setPage (int pg);
    void playPad (int p, int vel) { proc.uiNoteOn (p, vel); }
    void stopPad (int p) { proc.uiNoteOff (p); }
    static constexpr int W = 1240, H = 736;

    jlook::LnF lnf;
    Processor& proc;
    jlook::Scaler scaler { *this, "pad", W, H };
    jlook::SizeChip sizeChip { scaler };
    int sel = 0, page = PG_KIT, octave = 1, kbVel = 100, genre = 0;
    bool fullLevel = false, kbOn = false, exportSwing = false;
    std::set<int> keysDown;
    std::unique_ptr<juce::FileChooser> chooser;
    void loadDialog (int p);
    void padMenu (int p);
    void saveKit (bool askName);
    void loadKitDialog();
    void kitChanged();
    void openBrowser (int pad);
    void addToLibraryDialog();
    void scanMenu();

    // valori normalizzati dei parametri dei pad (per manopole e grafici)
    static float toNorm (int i, float v);
    static float fromNorm (int i, float n);
    void setPadParam (int i, float v);

    Root root { *this };
    Screen screen { *this };
    ChopView chop { *this };
    void openChop (int pad) { chop.open (pad); }
    PatternView pattern { *this };
    DragMidi drag { *this };
    juce::OwnedArray<PadComp> pads;
    juce::OwnedArray<Knob> qlinks, globalKnobs;
    juce::OwnedArray<Selector> selectors;
    juce::OwnedArray<Btn> btns;

private:
    void timerCallback() override;
    void bindQlinks();
    Knob* paramKnob (const char* id, const juce::String& label, juce::Colour c, std::function<juce::String (float)> fmt);
};
}
