#pragma once
#include "MixProcessor.h"
#include "AnalogLook.h"

namespace jmx
{
class Editor;

class MeterPanel : public juce::Component
{
public:
    explicit MeterPanel (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    Editor& ed;
    juce::Rectangle<float> targetR() const { return { 12, 34, (float) getWidth() - 24, 22 }; }
    juce::Rectangle<float> resetR() const { return { (float) getWidth() - 62, 8, 50, 18 }; }
};
class ImagerPanel : public juce::Component
{
public:
    explicit ImagerPanel (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    Editor& ed; float bl[1024] {}, br[1024] {};
};

// il rack: un tassello per modulo, l'EQ dove sta nella catena, + in fondo
class RackStrip : public juce::Component, public juce::DragAndDropTarget
{
public:
    explicit RackStrip (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override { dragFrom = -2; }
    bool isInterestedInDragSource (const SourceDetails&) override { return true; }
    void itemDragMove (const SourceDetails& d) override { dropAt = insertIndex (d.localPosition.toFloat()); repaint(); }
    void itemDragExit (const SourceDetails&) override { dropAt = -1; repaint(); }
    void itemDropped (const SourceDetails&) override;
    struct Item { int kind; int slot; juce::Rectangle<float> r; };   // kind 0 modulo, 1 EQ, 2 piu
    std::vector<Item> items() const;
    int insertIndex (juce::Point<float>) const;
    void openMenu (int slot, bool replace);
    Editor& ed; int dragFrom = -2, dropAt = -1; juce::Point<float> downAt;
};

// elenco dei moduli per categoria, da cliccare o trascinare nel rack
class BrowserList : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit BrowserList (Editor& e);
    void paint (juce::Graphics&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override { hover = -1; repaint(); }
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    struct Row { int cat; int type; };   // type -1 = intestazione
    std::vector<Row> rows; std::vector<bool> open;
    void rebuildRows();
    int rowAt (float y) const { return (int) (y / 20.0f); }
    Editor& ed; int hover = -1, pressed = -1; bool dragged = false;
};

// il modulo selezionato, in grande, con la faccia del suo hardware
class ModulePanel : public juce::Component
{
public:
    explicit ModulePanel (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    void resized() override { layout(); }
    void mouseDown (const juce::MouseEvent&) override;
    void rebuild();   // ricrea i comandi per lo slot e il modulo selezionati
    void layout();
    Editor& ed; int shownSlot = -2, shownType = -1;
    std::vector<std::unique_ptr<juce::Component>> ctl;
    std::unique_ptr<alook::Knob> mixK;
    juce::Rectangle<float> meterR, powerR { 18, 14, 26, 26 }, irR;
    float vu = -40;
};

class EqGraph : public juce::Component
{
public:
    explicit EqGraph (Editor& e);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override { drag = -1; }
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void tick();
    float xOf (float f) const; float fOf (float x) const; float yOf (float db) const; float dbOf (float y) const;
    int hit (juce::Point<float>) const;
    juce::Point<float> nodeOf (int b) const;
    void bandMenu (int b);
    Editor& ed; int drag = -1, hover = -1;
    juce::dsp::FFT fft { 12 }; std::vector<float> win, buf, specPre, specPost;
};
class BandPanel : public juce::Component
{
public:
    explicit BandPanel (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void rebuild();
    Editor& ed; int shown = -2;
    std::vector<std::unique_ptr<alook::Knob>> knobs;
    juce::Rectangle<float> typeR { 12, 36, 140, 24 }, chR { 158, 36, 140, 24 }, slopeR { 12, 66, 140, 24 }, onR { 158, 66, 140, 24 },
                           dynR { 12, 204, 90, 24 }, specR { 108, 204, 90, 24 }, soloR { 204, 204, 94, 24 }, delR { 204, 266, 94, 24 };
};

class GainChip : public juce::Component
{
public:
    GainChip (juce::RangedAudioParameter* p, juce::String l) : param (p), label (std::move (l)) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { start = param->getValue(); param->beginChangeGesture(); }
    void mouseDrag (const juce::MouseEvent& e) override { param->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, start - e.getDistanceFromDragStartY() / 300.0f)); repaint(); }
    void mouseUp (const juce::MouseEvent&) override { param->endChangeGesture(); }
    void mouseDoubleClick (const juce::MouseEvent&) override { param->setValueNotifyingHost (param->getDefaultValue()); repaint(); }
    juce::RangedAudioParameter* param; juce::String label; float start = 0;
};

class Editor : public juce::AudioProcessorEditor, public juce::DragAndDropContainer, private juce::Timer
{
public:
    explicit Editor (Processor&);
    ~Editor() override { setLookAndFeel (nullptr); }
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    static constexpr int W = 1500, H = 950;
    jlook::LnF lnf;
    Processor& proc;
    jlook::Scaler scaler { *this, "mixology", W, H };
    jlook::SizeChip sizeChip { scaler };
    juce::TooltipWindow tips { nullptr, 500 };
    juce::RangedAudioParameter* P (const juce::String& id) { return proc.apvts.getParameter (id); }
    int sel = -1;        // slot selezionato
    int band = -1;       // banda EQ selezionata
    void selectSlot (int s) { sel = s; panel.rebuild(); rack.repaint(); }
    void selectBand (int b) { band = b; bandPanel.rebuild(); graph.repaint(); }
    void insertAt (int pos, int type) { proc.insertModule (pos, type); selectSlot (juce::jlimit (0, proc.chainLen() - 1, pos)); }
    void presetMenu();
    static juce::PopupMenu moduleMenu (int baseId, int current);   // id = baseId + tipo
    MeterPanel meter { *this }; ImagerPanel imager { *this }; RackStrip rack { *this };
    juce::Viewport browserView; BrowserList browser { *this };
    ModulePanel panel { *this }; EqGraph graph { *this }; BandPanel bandPanel { *this };
    std::unique_ptr<GainChip> inChip, outChip;
    const juce::Rectangle<float> presetR { 366, 10, 420, 36 }, prevR { 328, 12, 32, 32 }, nextR { 792, 12, 32, 32 }, abA { 846, 14, 28, 28 }, abB { 882, 14, 28, 28 },
                                 liveR { 930, 12, 70, 32 }, cmpR { 1006, 12, 110, 32 }, scR { 1290, 12, 196, 32 };
    void timerCallback() override;
    int tick = 0, lastN = -1;
};
}
