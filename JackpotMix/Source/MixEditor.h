#pragma once
#include "MixProcessor.h"
#include "HwLook.h"
#include "JackpotScale.h"

namespace jmx
{
class Editor;

// aspetto del frontale di ogni modulo, ricavato dall'hardware a cui si ispira
struct Design
{
    int face = hw::FACE_PAINT; juce::Colour plate { 0xff5b6b78 }, ink { 0xfff0ece2 }, accent { 0xffd8452f };
    int knob = hw::KS_DAVIES, meter = 0; bool pedal = false;
    std::function<int (int, const PSpec&)> pick;   // stile della manopola per comando
    int styleFor (int i, const PSpec& s) const { return pick ? pick (i, s) : knob; }
};
Design designOf (int type);

// --------------------------------------------------------------------- un modulo nel rack (formato 500-series)
class SlotView : public juce::Component, public juce::SettableTooltipClient
{
public:
    SlotView (Editor&, int slot);
    static constexpr int kUnit = 136, kRail = 26;
    static int unitsFor (int type, int height);
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void tickMeters();
    Editor& ed; const int slot; int type = 0; Design D;
    std::vector<std::unique_ptr<juce::Component>> ctl;
    std::unique_ptr<hw::Knob> mixK;
    juce::Rectangle<float> meterR, nameR, menuR, irR, footR, inR;
    float vu = -20, ledV = 0;
private:
    juce::Point<float> downAt; bool dragging = false;
};

// tessera stretta dell'EQ centrale dentro il rack (si sposta come un modulo)
class EqTile : public juce::Component
{
public:
    explicit EqTile (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    Editor& ed;
};

// slot vuoto con il +
class AddTile : public juce::Component
{
public:
    explicit AddTile (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseEnter (const juce::MouseEvent&) override { repaint(); }
    void mouseExit (const juce::MouseEvent&) override { repaint(); }
    Editor& ed;
};

// il telaio (lunchbox) che contiene i moduli, scorre in orizzontale
class RackContent : public juce::Component, public juce::DragAndDropTarget
{
public:
    explicit RackContent (Editor& e) : ed (e) {}
    void rebuild();
    void layoutTiles (int height);
    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    bool isInterestedInDragSource (const SourceDetails&) override { return true; }
    void itemDragMove (const SourceDetails& d) override { dropAt = insertIndex (d.localPosition.toFloat()); repaint(); }
    void itemDragExit (const SourceDetails&) override { dropAt = -1; repaint(); }
    void itemDropped (const SourceDetails&) override;
    int insertIndex (juce::Point<float>) const;
    Editor& ed;
    juce::OwnedArray<SlotView> slots; std::unique_ptr<EqTile> eqTile; std::unique_ptr<AddTile> addTile;
    juce::String signature; int dropAt = -1;
};

// --------------------------------------------------------------------- misure a destra
class LoudPanel : public juce::Component
{
public:
    explicit LoudPanel (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    Editor& ed; juce::Rectangle<float> targetR, resetR;
    float holdS = -100;
};
class ImagerPanel : public juce::Component
{
public:
    explicit ImagerPanel (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    Editor& ed; float bl[2048] {}, br[2048] {};
};

// --------------------------------------------------------------------- EQ centrale stile Pro-Q
class EqGraph;
class BandPop : public juce::Component
{
public:
    explicit BandPop (EqGraph& g) : graph (g) {}
    void show (int band);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override { dragK = -1; }
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    EqGraph& graph; int band = -1;
    struct Hot { juce::Rectangle<float> r; int id; };
    std::vector<Hot> hots;
private:
    int dragK = -1; float dragStart = 0;
    int hit (juce::Point<float>) const;
};
class EqGraph : public juce::Component
{
public:
    explicit EqGraph (Editor&);
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void tick();
    void bandMenu (int b);
    void updatePop();
    float xOf (float f) const; float fOf (float x) const; float yOf (float db) const; float dbOf (float y) const;
    juce::Point<float> nodeOf (int b) const; int hit (juce::Point<float>) const;
    juce::Rectangle<float> plot() const { return getLocalBounds().toFloat().reduced (8).withTrimmedTop (22).withTrimmedBottom (16).withTrimmedLeft (26); }
    Editor& ed; BandPop pop { *this };
    int hover = -1, drag = -1, sel = -1, popBand = -1, popHold = 0;
private:
    juce::dsp::FFT fft { 12 }; std::vector<float> win, buf, specPre, specPost;
};

class GainChip : public juce::Component
{
public:
    GainChip (juce::RangedAudioParameter* p, juce::String l) : param (p), label (std::move (l)) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { start = param->getValue(); }
    void mouseDrag (const juce::MouseEvent& e) override { param->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, start - e.getDistanceFromDragStartY() / 300.0f)); repaint(); }
    void mouseDoubleClick (const juce::MouseEvent&) override { param->setValueNotifyingHost (param->getDefaultValue()); repaint(); }
    juce::RangedAudioParameter* param; juce::String label; float start = 0;
};

// --------------------------------------------------------------------- editor
class Editor : public juce::AudioProcessorEditor, public juce::DragAndDropContainer, private juce::Timer
{
public:
    explicit Editor (Processor&);
    ~Editor() override { setLookAndFeel (nullptr); }
    static constexpr int W = 1400, H = 860;
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void timerCallback() override;
    juce::RangedAudioParameter* P (const juce::String& id) { return proc.apvts.getParameter (id); }
    void insertAt (int pos, int type) { proc.insertModule (pos, type); rack.rebuild(); }
    void chainChanged() { rack.rebuild(); repaint(); }
    void presetMenu();
    void slotMenu (int s, juce::Component* target);
    static juce::PopupMenu moduleMenu (int baseId, int current);
    void pickModule (int s, juce::Component* target);   // s = -1: nuovo modulo in fondo
    Processor& proc;
    jlook::Scaler scaler { *this, "mixology", W, H };
    jlook::LnF lnf;
    juce::TooltipWindow tips { this, 600 };
    juce::Viewport rackView; RackContent rack { *this };
    EqGraph graph { *this };
    LoudPanel loud { *this }; ImagerPanel imager { *this };
    std::unique_ptr<GainChip> inChip, outChip;
    jlook::SizeChip sizeChip { scaler };
    int tick = 0;
    const juce::Rectangle<float> presetR { 372, 11, 380, 34 }, prevR { 336, 13, 30, 30 }, nextR { 758, 13, 30, 30 }, abA { 808, 15, 26, 26 }, abB { 840, 15, 26, 26 },
                                 liveR { 884, 12, 62, 32 }, cmpR { 952, 12, 104, 32 }, scR { 1218, 12, 104, 32 };
};
}
