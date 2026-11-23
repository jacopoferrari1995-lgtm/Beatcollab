#pragma once
#include "S8Processor.h"
#include "JackpotScale.h"
#include "CyberUI.h"

namespace s8
{
using namespace cyber;
class Editor;

struct Diag { int sev; juce::String title, text; };   // sev 2 = da fare, 1 = consigliato, 0 = ok

class TunerView : public juce::Component { public: explicit TunerView (Editor& e) : ed (e) {} void paint (juce::Graphics&) override; Editor& ed; float hzS = 0, centS = 0; };
class SpecView : public juce::Component
{
public:
    explicit SpecView (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    void compute();
    Editor& ed; static constexpr int ORD = 13, NF = 1 << ORD;
    juce::dsp::FFT fft { ORD }; std::vector<float> buf = std::vector<float> (NF * 2), win = std::vector<float> (NF), spm = std::vector<float> (NF / 2, -120.0f);
};
class DocView : public juce::Component { public: explicit DocView (Editor& e) : ed (e) {} void paint (juce::Graphics&) override; Editor& ed; };
// SYNTH: sample, inviluppi, tastiera
class SampleView : public juce::Component
{
public:
    explicit SampleView (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override; void mouseDown (const juce::MouseEvent&) override;
    Editor& ed; juce::Rectangle<float> loadR, clearR, rootDn, rootUp; juce::Path wave; const void* waveOf = nullptr; juce::String msg;
};
class EnvView : public juce::Component { public: explicit EnvView (Editor& e) : ed (e) {} void paint (juce::Graphics&) override; Editor& ed; };
class KeysView : public juce::Component
{
public:
    explicit KeysView (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override; void mouseDown (const juce::MouseEvent&) override; void mouseDrag (const juce::MouseEvent&) override; void mouseUp (const juce::MouseEvent&) override;
    int keyAt (juce::Point<float>) const; Editor& ed; int down = -1; static constexpr int kLo = 24, kN = 36;
};

class Editor : public juce::AudioProcessorEditor, public juce::FileDragAndDropTarget, private juce::Timer
{
public:
    explicit Editor (Processor&);
    ~Editor() override { stopTimer(); removeMouseListener (&undoMark); }
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    bool keyPressed (const juce::KeyPress&) override;
    bool isInterestedInFileDrag (const juce::StringArray&) override;
    void filesDropped (const juce::StringArray&, int, int) override;
    void fileDragEnter (const juce::StringArray&, int, int) override { dragOver = true; repaint(); }
    void fileDragExit (const juce::StringArray&) override { dragOver = false; repaint(); }
    void parentHierarchyChanged() override { jlook::preferFastRenderer (*this); }
    static constexpr int W = 1180, HE = 808, HS = 916;
    static constexpr int H = kSynth ? HS : HE;
    juce::SharedResourcePointer<jlook::Resources> keepLook; neo::KeepFonts keepFonts;
    Processor& proc;
    jlook::Scaler scaler { *this, kSynth ? "808synth" : "808studio", W, H };
    jlook::SizeChip sizeChip { scaler };
    TunerView tuner { *this }; SpecView spec { *this }; DocView doc { *this };
    SampleView smp { *this }; EnvView env { *this }; KeysView keys { *this };
    juce::OwnedArray<CyberKnob> knobs;
    juce::Rectangle<float> modeR[6], invR, resetR, prevR, nextR, presetR, undoR, redoR, gmodeR[3], polyR;
    std::vector<Diag> diags; void diagnose();
    void presetMenu(); void openSample(); void doUndo (bool redo);
    juce::Image bg; float bgScale = 0; void paintStatic (juce::Graphics&);
    int frame = 0, glitchT = 0; float glitchOff = 0; bool dragOver = false; int deckY = 0;
    std::unique_ptr<juce::FileChooser> chooser;
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
