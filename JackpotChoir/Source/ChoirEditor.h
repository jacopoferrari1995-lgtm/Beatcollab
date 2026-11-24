#pragma once
#include "ChoirProcessor.h"
#include "Skin.h"
#include "PresetBrowser.h"

namespace jcho
{
using namespace jkit;
class Editor;
// la melodia che canti (bianca) e le voci del coro (colorate) che scorrono sulle righe della scala
class PitchView : public juce::Component { public: explicit PitchView (Editor& e) : ed (e) {} void paint (juce::Graphics&) override; Editor& ed; float lo = 48, hi = 76; };

class Editor : public juce::AudioProcessorEditor, private juce::Timer
{
    skin::Keep skinKeep;
    juce::Rectangle<int> themeRect;
public:
    explicit Editor (Processor&);
    ~Editor() override { removeMouseListener (&undoMark); setLookAndFeel (nullptr); }
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    bool keyPressed (const juce::KeyPress&) override;
    static constexpr int W = 1100, H = 740;
    jlook::LnF lnf;
    Processor& proc;
    jlook::Scaler scaler { *this, "choir", W, H };
    jlook::SizeChip sizeChip { scaler };
    juce::RangedAudioParameter* P (const juce::String& id) { return proc.apvts.getParameter (id); }
    static juce::Colour voiceCol (int k);
    const juce::Colour acc = C::violet, acc2 = C::phos;
    struct UndoMark : juce::MouseListener { Processor* p = nullptr; void mouseDown (const juce::MouseEvent&) override { if (p) p->undo.beginNewTransaction(); } } undoMark;
private:
    void timerCallback() override;
    void doUndo (bool redo);
    std::vector<std::unique_ptr<Knob>> knobs;
    std::vector<std::unique_ptr<Segs>> segs;
    PitchView pitch { *this };
    PresetBar bar; int tick = 0;
    juce::Rectangle<int> undoR { 296, 20, 52, 26 }, redoR { 352, 20, 52, 26 };
    jbrowse::Browser browser;
    void setupBrowser();
    void askPresetName();
};
}
