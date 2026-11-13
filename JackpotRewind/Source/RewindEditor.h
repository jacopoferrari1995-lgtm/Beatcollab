#pragma once
#include "RewindProcessor.h"
#include "Skin.h"

namespace jrwd
{
using namespace jkit;
class Editor;
struct Take { juce::int64 a, b; float energy; bool starred; };

class Overview : public juce::Component
{
public:
    explicit Overview (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    juce::int64 sampleAt (float x) const;
    float xOf (juce::int64 s) const;
    Editor& ed; juce::int64 dragFrom = -1, viewEnd = 0, viewLen = 1;
};
class Detail : public juce::Component
{
public:
    explicit Detail (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override { edge = 0; }
    void mouseMove (const juce::MouseEvent&) override;
    juce::int64 v0 = 0, v1 = 1;
    Editor& ed; int edge = 0;
    juce::AudioBuffer<float> cache; juce::int64 cA = -1, cB = -1;
};
class DragZone : public juce::Component
{
public:
    explicit DragZone (Editor& e) : ed (e) { setMouseCursor (juce::MouseCursor::DraggingHandCursor); }
    void paint (juce::Graphics&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseEnter (const juce::MouseEvent&) override { hover = true; repaint(); }
    void mouseExit (const juce::MouseEvent&) override { hover = false; repaint(); }
    Editor& ed; bool hover = false, dragging = false;
};

class Editor : public juce::AudioProcessorEditor, private juce::Timer
{
    juce::Rectangle<int> themeRect;
public:
    explicit Editor (Processor&);
    ~Editor() override { setLookAndFeel (nullptr); }
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    static constexpr int W = 1120, H = 700;
    jlook::LnF lnf;
    Processor& proc;
    jlook::Scaler scaler { *this, "rewind", W, H };
    jlook::SizeChip sizeChip { scaler };
    juce::RangedAudioParameter* P (const juce::String& id) { return proc.apvts.getParameter (id); }
    const juce::Colour acc = C::phos, acc2 = C::magenta;
    // selezione in campioni assoluti
    juce::int64 selA = -1, selB = -1;
    bool hasSel() const { return selB > selA && selA >= proc.oldest(); }
    void setSel (juce::int64 a, juce::int64 b, bool snap = true);
    std::vector<Take> takes;
    juce::File exportSel();
    juce::String lastFile, selKey; juce::int64 keyA = -1, keyB = -1;
    juce::String timeText (double sec) const { const int s = (int) std::round (sec); return juce::String (s / 60) + ":" + juce::String (s % 60).paddedLeft ('0', 2); }
    juce::String barsText (juce::int64 a, juce::int64 b) const;
private:
    void timerCallback() override;
    void findTakes();
    Overview over { *this }; Detail detail { *this }; DragZone dz { *this };
    std::vector<std::unique_ptr<Pill>> pills; std::vector<std::unique_ptr<Switch>> switches;
    std::unique_ptr<Knob> kGain;
    std::vector<juce::Rectangle<float>> takeChips;
    int tick = 0;
};
}
