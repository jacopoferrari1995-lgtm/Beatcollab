#pragma once
#include "MasterProcessor.h"
#include "JackpotScale.h"

namespace jms
{
namespace C
{
    const juce::Colour bg { 0xff0b0c10 }, text { 0xffdfe3ea }, dim { 0xff7d8494 }, faint { 0xff4f5562 };
    const juce::Colour magenta { 0xffff2e88 }, phos { 0xff5ef2ff }, amber { 0xffffb000 }, green { 0xff7dff5a }, red { 0xffff3b5c }, violet { 0xffa66bff };
    const juce::Colour lcd1 { 0xff05161a }, lcd2 { 0xff020b0d };
}
juce::Font uf (float size, bool bold = true);
juce::Font mf (float size);
void glowText (juce::Graphics&, const juce::String&, juce::Rectangle<float>, juce::Colour, juce::Font, juce::Justification = juce::Justification::centredLeft);
void lcd (juce::Graphics&, juce::Rectangle<float>, float rad);

class Editor;

class Knob : public juce::Component
{
public:
    Knob (juce::RangedAudioParameter* p, juce::Colour c) : param (p), col (c) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent&) override { if (drag) param->endChangeGesture(); drag = false; repaint(); }
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseDoubleClick (const juce::MouseEvent&) override { param->setValueNotifyingHost (param->getDefaultValue()); }
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w) override { param->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, param->getValue() + (w.deltaY > 0 ? 0.01f : -0.01f))); }
    void mouseEnter (const juce::MouseEvent&) override { hover = true; setMouseCursor (juce::MouseCursor::UpDownResizeCursor); repaint(); }
    void mouseExit (const juce::MouseEvent&) override { hover = false; repaint(); }
    juce::RangedAudioParameter* param; juce::Colour col; juce::String label;
    std::function<juce::String()> text;
    bool big = false, bipolar = false; float activity = 0;
private:
    float start = 0; bool drag = false, hover = false;
};

class Segs : public juce::Component
{
public:
    Segs (juce::RangedAudioParameter* p, juce::Colour c, juce::StringArray it) : param (p), col (c), items (std::move (it)) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    juce::RangedAudioParameter* param; juce::Colour col; juce::StringArray items; float font = 11.0f; int rows = 1;
};

class Switch : public juce::Component
{
public:
    Switch (juce::RangedAudioParameter* p, juce::Colour c, juce::String l = {}) : param (p), col (c), label (std::move (l)) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { param->setValueNotifyingHost (param->getValue() > 0.5f ? 0.0f : 1.0f); }
    juce::RangedAudioParameter* param; juce::Colour col; juce::String label;
};

// catena dei moduli
class Chain : public juce::Component
{
public:
    Chain (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override { hov = -1; repaint(); }
    juce::Rectangle<float> tile (int m) const { const float w = (getWidth() - 8.0f * (kNumMods - 1)) / kNumMods; return { m * (w + 8), 0, w, (float) getHeight() }; }
    juce::Rectangle<float> powR (int m) const { auto t = tile (m); return { t.getX() + 8, t.getY() + 9, 14, 14 }; }
    Editor& ed; int hov = -1;
};

// storia: ingresso, uscita e riduzione di guadagno
class History : public juce::Component
{
public:
    History (Editor& e, bool lim) : ed (e), limiter (lim) {}
    void paint (juce::Graphics&) override;
    Editor& ed; bool limiter;
};

class MaxPage : public juce::Component
{
public:
    MaxPage (Editor&);
    void paint (juce::Graphics&) override;
    void resized() override;
    Editor& ed;
    std::unique_ptr<Knob> gain, clip, chr, link; std::unique_ptr<Segs> style; std::unique_ptr<Switch> on, delta, learn;
    History hist;
};

class LimPage : public juce::Component
{
public:
    LimPage (Editor&);
    void paint (juce::Graphics&) override;
    void resized() override;
    Editor& ed;
    std::unique_ptr<Knob> ceil, rel; std::unique_ptr<Switch> on, delta, autoR, tp;
    History hist;
};

class SoonPage : public juce::Component
{
public:
    SoonPage (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    Editor& ed; int mod = 0;
};

class LoudPanel : public juce::Component
{
public:
    LoudPanel (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    juce::Rectangle<float> tgtCell (int t) const { const float w = (getWidth() - 32 - 8) / 3.0f; return { 16 + (t % 3) * (w + 4), 268.0f + (t / 3) * 26.0f, w, 22 }; }
    juce::Rectangle<float> resetR() const { return { (float) getWidth() - 86, 10, 70, 22 }; }
    Editor& ed;
};

class Editor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit Editor (Processor&);
    ~Editor() override { setLookAndFeel (nullptr); }
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void tickForTest() { for (int i = 0; i < 5; ++i) timerCallback(); }
    void select (int m);
    static constexpr int W = 1280, H = 800;
    jlook::LnF lnf;
    Processor& proc;
    jlook::Scaler scaler { *this, "master", W, H };
    jlook::SizeChip sizeChip { scaler };
    juce::RangedAudioParameter* P (const juce::String& id) { return proc.apvts.getParameter (id); }
    int sel = MAXIM;
    Chain chain { *this };
    MaxPage maxPage { *this };
    LimPage limPage { *this };
    SoonPage soon { *this };
    LoudPanel loud { *this };
private:
    void timerCallback() override;
    std::unique_ptr<Segs> dither; std::unique_ptr<Switch> compare, norm; std::unique_ptr<Knob> trim;
    juce::Rectangle<int> presetRect, prevRect, nextRect, abRect; bool abHover = false; int tick = 0;
};
}
