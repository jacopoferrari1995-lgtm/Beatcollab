#pragma once
#include "GtrProcessor.h"
#include "JackpotScale.h"
#include "Skin.h"
#include "JackpotKit.h"
#include "PresetBrowser.h"

namespace gtr
{
using namespace jkit;
inline juce::String U (const char* s) { return juce::String::fromUTF8 (s); }

class Editor : public juce::AudioProcessorEditor, public juce::DragAndDropContainer, private juce::Timer
{
    skin::Keep skinKeep;
public:
    explicit Editor (Processor&);
    ~Editor() override { stopTimer(); removeMouseListener (&undoMark); }
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void mouseExit (const juce::MouseEvent&) override { hoverSlot = -1; hoverId = -1; }
    bool keyPressed (const juce::KeyPress&) override;
    void parentHierarchyChanged() override { jlook::preferFastRenderer (*this); }
    static constexpr int W = 1180, H = 944;
    juce::SharedResourcePointer<jlook::Resources> keepLook; neo::KeepFonts keepFonts;
    Processor& proc;
    jlook::Scaler scaler { *this, "guitar", W, H };
    jlook::SizeChip sizeChip { scaler };
    juce::OwnedArray<jkit::Knob> knobs;
    jbrowse::Browser browser;
    // zone cliccabili
    juce::Rectangle<float> keyR[12], scaleR[kScales], genreR, moodR, barsR[4], perBarR[3], slotR[8], addR, remR;
    juce::Rectangle<float> fretR, vstyleR[kVStyles], tuningR, octR[5], rndR;
    juce::Rectangle<float> modeR[kModes], patR[12], rollR, dragR, genR, varR, playR, liveR, midiR, csR, layR[3], diceR[3], layDragR[3], editLayR[3], snapR, resetEditR, styleR, figR, figDiceR, ritR, ritDiceR, gridBtnR, rotR, writeR;
    juce::Rectangle<float> bodyR[kBodies], listenR;
    juce::Rectangle<float> presetR, prevR, nextR, undoR, redoR, themeR;
    int hoverSlot = -1, selSlot = 0, hoverId = -1, bgTheme = -1; bool dragging = false; juce::String info;
    juce::Image bg; float bgScale = 0; void paintStatic (juce::Graphics&);
    void setupBrowser(); void askPresetName(); void doUndo (bool redo); void slotMenu (int i, juce::Rectangle<float> at = {}); void layerMenu (int ly); void styleMenu(); void figMenu(); void ritMenu(); void askFigure(); void randomFig();
    bool gridEdit = false; int gridPaint = -1; juce::Rectangle<float> gridCell (int row, int k) const;
    void drawGrid (juce::Graphics&); void genreMenu(); void moodMenu(); void repaintAll();
    void drawFretboard (juce::Graphics&, const Part&, int slot);
    void drawRoll (juce::Graphics&, const Part&);
    // piano roll modificabile
    Part edPart; bool editing = false; int editMode = 0; std::vector<int> sel; std::vector<Ev> orig; int hitIdx = -1;
    struct Sig { double beat; int note, layer; }; std::vector<Sig> selSig;
    juce::Point<float> downPos; juce::Rectangle<float> marquee; int rollLo = 40, rollHi = 76; int snapIdx = 0, editLayer = 2;
    std::vector<std::pair<juce::Rectangle<float>, int>> chordLabels;
    double grid() const { static const double g[4] = { 0.25, 0.5, 1.0 / 3.0, 0.0 }; return g[snapIdx]; }
    double snapB (double b) const { const double g = grid(); return g > 0 ? std::round (b / g) * g : b; }
    float bx (double b, double L) const { return rollR.getX() + (float) (b / L) * rollR.getWidth(); }
    double xb (float x, double L) const { return (x - rollR.getX()) / rollR.getWidth() * L; }
    std::vector<int> rows; bool folded = false;   // righe visibili (dal basso): tutte, o solo le note della scala se l'estensione e' ampia
    float rowH() const { return rollR.getHeight() / (float) juce::jmax<size_t> (1, rows.size()); }
    float ny (int n) const
    {
        if (rows.empty()) return rollR.getBottom();
        const auto it = std::lower_bound (rows.begin(), rows.end(), n); int i = (int) (it - rows.begin());
        if (it == rows.end() || *it != n) i = juce::jlimit (0, (int) rows.size() - 1, i);   // fuori dalle righe: la riga vicina
        return rollR.getBottom() - (i + 1) * rowH();
    }
    int yn (float y) const { if (rows.empty()) return 60; const int i = juce::jlimit (0, (int) rows.size() - 1, (int) std::floor ((rollR.getBottom() - y) / rowH())); return rows[(size_t) i]; }
    juce::Rectangle<float> noteRect (const Ev& e, double L) const { const float x = bx (e.beat, L); return { x, ny (e.note) + 0.75f, juce::jmax (3.0f, bx (e.beat + e.len, L) - x), juce::jmax (2.0f, rowH() - 1.5f) }; }
    int hitNote (const Part& P, juce::Point<float> p) const { for (int i = (int) P.ev.size() - 1; i >= 0; --i) if (noteRect (P.ev[(size_t) i], P.len()).expanded (1, 1).contains (p)) return i; return -1; }
    void resolveSel (const Part& P);
    void storeSig (const Part& P);
    void commitEdit();
    void fixString (Ev& e) const;
    int scaleShift (int note, int steps) const;
    int stepsBetween (int a, int b) const;
    void deleteSelected();
    struct UndoMark : juce::MouseListener
    {
        Processor* p = nullptr; double lastWheel = 0;
        void mouseDown (const juce::MouseEvent&) override { if (p) p->undo.beginNewTransaction(); }
        void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override { const double t = juce::Time::getMillisecondCounterHiRes(); if (p && t - lastWheel > 600) p->undo.beginNewTransaction(); lastWheel = t; }
    } undoMark;
private:
    void timerCallback() override;
};
}
