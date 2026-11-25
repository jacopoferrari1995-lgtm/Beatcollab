#pragma once
#include "MixProcessor.h"
#include "HwLook.h"
#include "JackpotScale.h"
#include "PresetBrowser.h"
#include "Skin.h"

namespace jmx
{
class Editor;

// aspetto del frontale di ogni modulo, ricavato dall'hardware a cui si ispira
struct Design
{
    int face = hw::FACE_PAINT; juce::Colour plate { 0xff5b6b78 }, ink { 0xfff0ece2 }, accent { 0xffd8452f };
    int knob = hw::KS_DAVIES, meter = 0; bool pedal = false, buttons = false;
    juce::StringArray hero;   // comandi principali, disegnati grandi in prima fila (come sull'apparecchio)
    juce::String sub;         // funzione dell'apparecchio, serigrafata sotto il nome
    std::function<int (int, const PSpec&)> pick;   // stile della manopola per comando
    int styleFor (int i, const PSpec& s) const { return pick ? pick (i, s) : knob; }
};
Design designOf (int type);

// --------------------------------------------------------------------- un modulo nel rack (formato 500-series)
class SlotView : public juce::Component, public juce::SettableTooltipClient
{
public:
    SlotView (Editor&, int slot);
    static constexpr int kUnit = 112, kRail = 40;
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
    double paramSig = -1;   // le manopole si ridisegnano solo se un valore del modulo cambia
    juce::Rectangle<float> meterR, nameR, menuR, irR, footR, inR, soloR, deltaR, presetR, bypR;
    float vu = -20, ledV = 0; int heroN = 0;
private:
    void paintStatic (juce::Graphics&);
    juce::Image bg; juce::String bgKey;   // frontale disegnato una volta sola (prima si ridisegnava tutto a ogni lancetta)
    juce::Point<float> downAt; bool dragging = false;
};

// tessera stretta dell'EQ centrale dentro il rack (si sposta come un modulo)
class EqTile : public juce::Component
{
public:
    explicit EqTile (Editor& e) : ed (e) {}
    juce::Rectangle<float> openR, onR, curveR;
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
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
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override { setMouseCursor (juce::MouseCursor::NormalCursor); }
    int panFrom = 0;
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
    float k = 1.0f;                        // i moduli sono disegnati a kDesignH e ingranditi per riempire il rack
    static constexpr int kDesignH = 548;
};

// il rack scorre con la rotella (anche quella verticale), col trackpad, trascinando lo sfondo; i salti sono animati
class RackView : public juce::Viewport
{
public:
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w) override
    {
        const float d = std::abs (w.deltaX) > std::abs (w.deltaY) ? w.deltaX : w.deltaY;
        scrollTo ((float) (target >= 0 ? target : getViewPositionX()) - (w.isReversed ? -d : d) * (w.isSmooth ? 700.0f : 1100.0f), ! w.isSmooth);
    }
    void scrollTo (float x, bool anim = true)
    {
        const float mx = (float) juce::jmax (0, getViewedComponent() ? getViewedComponent()->getWidth() - getViewWidth() : 0);
        target = juce::jlimit (0.0f, mx, x);
        if (! anim) { setViewPosition (juce::roundToInt (target), 0); target = -1; }
    }
    void step()
    {
        if (target < 0) return;
        const float x = (float) getViewPositionX(), nx = x + (target - x) * 0.35f;
        if (std::abs (target - nx) < 1.0f) { setViewPosition (juce::roundToInt (target), 0); target = -1; } else setViewPosition (juce::roundToInt (nx), 0);
    }
    float target = -1;
};

// striscia sopra il rack: tutti i moduli in fila, clic per andarci, x per toglierlo, tasto destro per il menu, + per aggiungerne
class ChainNav : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit ChainNav (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override { hov = -1; hovX = false; repaint(); }
    void mouseDown (const juce::MouseEvent&) override;
    struct Chip { juce::Rectangle<float> r, x; int slot; };   // slot: -1 EQ, -2 aggiungi
    std::vector<Chip> layout() const;
    Editor& ed; int hov = -1; bool hovX = false;
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
    juce::Rectangle<float> plot() const { return getLocalBounds().toFloat().reduced (8).withTrimmedTop (30).withTrimmedBottom (46).withTrimmedLeft (26); }
    Editor& ed; BandPop pop { *this };
    int hover = -1, drag = -1, sel = -1, popBand = -1, popHold = 0; float dragF = 1000, dragG = 0; juce::Point<float> lastPos;
    float span = 12;   // scala verticale: +-6, +-12, +-24 dB
    int ana = 0;       // analizzatore: 0 prima e dopo, 1 solo dopo, 2 spento
    struct Hot { juce::Rectangle<float> r; int id; };
    std::vector<Hot> hots;   // pulsanti della testata e tessere delle bande in basso
    void addBand (float f);
private:
    juce::dsp::FFT fft { 12 }; std::vector<float> win, buf, specPre, specPost, dispPre, dispPost;
    juce::Image grid; juce::String gridKey;
    std::vector<float> bandDb[16]; juce::String bandKey[16];   // curve delle bande: si ricalcolano solo se la banda cambia
    void paintGrid (juce::Graphics&);
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
    skin::Keep keepSkin;   // primo membro: i caratteri dei temi restano caricati (niente sfarfallio)
public:
    explicit Editor (Processor&);
    ~Editor() override { removeMouseListener (&undoMark); setLookAndFeel (nullptr); }
    bool keyPressed (const juce::KeyPress&) override;
    void doUndo (bool redo);
    void listenChanged() { for (auto* s : rack.slots) s->repaint(); }
    struct UndoMark : juce::MouseListener
    {   // ogni clic (e ogni giro di rotella dopo una pausa) apre un passo nuovo della storia
        Processor* p = nullptr; double lastWheel = 0;
        void mouseDown (const juce::MouseEvent&) override { if (p) p->undo.beginNewTransaction(); }
        void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override
        { const double t = juce::Time::getMillisecondCounterHiRes(); if (p && t - lastWheel > 600) p->undo.beginNewTransaction(); lastWheel = t; }
    } undoMark;
    static constexpr int W = 1400, H = 860;
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void timerCallback() override;
    juce::RangedAudioParameter* P (const juce::String& id) { return proc.apvts.getParameter (id); }
    void insertAt (int pos, int type) { proc.insertModule (pos, type); rack.rebuild(); }
    void chainChanged() { rack.rebuild(); repaint(); }
    void presetMenu();
    jbrowse::Browser browser; void setupBrowser();
    void slotMenu (int s, juce::Component* target);
    jbrowse::Browser modBrowser;                   // scelta dei moduli e preset del singolo modulo
    void openModuleChooser (int s);                // s = -1: nuovo modulo in fondo
    void openModulePresets (int s);
    void scrollToSlot (int s);
    struct ModPre { int type = 0, id = -1; juce::String name; } modPre[kSlots];
    juce::Image bgImg; juce::String bgKey;
    static juce::PopupMenu moduleMenu (int baseId, int current);
    void pickModule (int s, juce::Component* target);   // s = -1: nuovo modulo in fondo
    Processor& proc;
    jlook::Scaler scaler { *this, "mixology", W, H };
    jlook::LnF lnf;
    juce::TooltipWindow tips { this, 600 };
    RackView rackView; RackContent rack { *this }; ChainNav nav { *this };
    float lastEqOn = -1, lastIo = -1e9f; double lastEqSig = -1;
    bool eqOpen = false; void openEq (bool o);
    int resizeHold = 0, lastW = 0;   // durante lo zoom si stirano le immagini gia' pronte invece di rifarle a ogni passo
    EqGraph graph { *this };
    LoudPanel loud { *this }; ImagerPanel imager { *this };
    std::unique_ptr<GainChip> inChip, outChip;
    jlook::SizeChip sizeChip { scaler };
    int tick = 0;
    const juce::Rectangle<float> undoR { 206, 13, 58, 30 }, redoR { 268, 13, 58, 30 }, themeR { 1330, 3, 58, 17 }, presetR { 372, 11, 380, 34 }, prevR { 336, 13, 30, 30 }, nextR { 758, 13, 30, 30 }, abA { 808, 15, 26, 26 }, abB { 840, 15, 26, 26 },
                                 liveR { 884, 12, 62, 32 }, cmpR { 952, 12, 104, 32 }, scR { 1218, 12, 104, 32 };
};
}
