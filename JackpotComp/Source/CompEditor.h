#pragma once
#include "CompProcessor.h"
#include "JackpotScale.h"
#include "Skin.h"

namespace jc
{
namespace C
{
    inline juce::Colour bg { 0xff0b0c10 }, text { 0xffdfe3ea }, dim { 0xff7d8494 }, faint { 0xff4f5562 };
    inline juce::Colour magenta { 0xffff2e88 }, phos { 0xff5ef2ff }, amber { 0xffffb000 }, green { 0xff7dff5a }, red { 0xffff3b5c };
    inline juce::Colour lcd1 { 0xff05161a }, lcd2 { 0xff020b0d };
    inline juce::Colour stage[kStages] = { juce::Colour (0xff5ef2ff), juce::Colour (0xffffb000), juce::Colour (0xffff2e88), juce::Colour (0xff7dff5a), juce::Colour (0xffa66bff), juce::Colour (0xffff7a3d) };
    // colori presi dal tema scelto
    inline void sync()
    {
        using namespace neo;
        bg = K::bg0; text = K::ink; dim = K::sub; faint = K::faint; magenta = K::pink; phos = K::cyan; amber = K::amber; green = K::mint; red = K::red; lcd1 = K::disp0; lcd2 = K::disp1;
        const juce::Colour s[kStages] = { K::cyan, K::amber, K::pink, K::mint, K::violet, K::red.interpolatedWith (K::amber, 0.4f) };
        for (int i = 0; i < kStages; ++i) stage[i] = s[i];
    }
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
    void mouseDoubleClick (const juce::MouseEvent&) override { if (enabled) param->setValueNotifyingHost (param->getDefaultValue()); }
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w) override { if (enabled) param->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, param->getValue() + (w.deltaY > 0 ? 0.01f : -0.01f))); }
    void mouseEnter (const juce::MouseEvent&) override { hover = true; setMouseCursor (juce::MouseCursor::UpDownResizeCursor); repaint(); }
    void mouseExit (const juce::MouseEvent&) override { hover = false; repaint(); }
    juce::RangedAudioParameter* param; juce::Colour col; juce::String label;
    std::function<juce::String()> text; std::function<void()> onTouch;
    bool enabled = true, big = false, bipolar = false, stepped = false; int steps = 0;
private:
    float start = 0; bool drag = false, hover = false;
};

class Segs : public juce::Component
{
public:
    Segs (juce::RangedAudioParameter* p, juce::Colour c) : param (p), col (c) {}
    std::function<juce::StringArray()> items; std::function<void()> onTouch; std::function<int (int)> map;   // map: indice pulsante -> valore
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    juce::RangedAudioParameter* param; juce::Colour col;
};

class Switch : public juce::Component
{
public:
    Switch (juce::RangedAudioParameter* p, juce::Colour c, juce::String l = {}) : param (p), col (c), label (std::move (l)) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { param->setValueNotifyingHost (param->getValue() > 0.5f ? 0.0f : 1.0f); if (onTouch) onTouch(); }
    juce::RangedAudioParameter* param; juce::Colour col; juce::String label; std::function<void()> onTouch;
};

// fila degli stadi: fino a 6 compressori, accendi, scegli, solo
class Rack : public juce::Component
{
public:
    Rack (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override { hov = -1; repaint(); }
    juce::Rectangle<float> tile (int s) const { const float w = (getWidth() - 5 * 8.0f) / kStages; return { s * (w + 8), 0, w, (float) getHeight() }; }
    juce::Rectangle<float> soloR (int s) const { auto t = tile (s); return { t.getRight() - 44, t.getBottom() - 30, 36, 22 }; }
    juce::Rectangle<float> powR (int s) const { auto t = tile (s); return { t.getX() + 8, t.getY() + 8, 18, 18 }; }
    Editor& ed; int hov = -1; float meter[kStages] {};
};

class StageEditor : public juce::Component
{
public:
    StageEditor (Editor&);
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override { xDrag = -1; }
    void bind (int s);
    void bindBand (int b);
    void layoutKnobs();
    bool isMb() const;
    // multibanda: posizione delle frequenze nel display
    float xOf (float hz) const { auto r = vuRect(); return r.getX() + r.getWidth() * std::log (juce::jlimit (20.0f, 20000.0f, hz) / 20.0f) / std::log (1000.0f); }
    float hzOf (float x) const { auto r = vuRect(); return 20.0f * std::pow (1000.0f, juce::jlimit (0.0f, 1.0f, (x - r.getX()) / r.getWidth())); }
    void mbBounds (float* edges, int& nb) const;
    Editor& ed; int s = 0; juce::String pfx = "a_"; int mbSel = 0, xDrag = -1;
    std::unique_ptr<Switch> power; std::unique_ptr<Segs> ratio;
    std::unique_ptr<Knob> drive, att, rel, out, color, mix, bThr, bRatio, bAtt, bRel, bGain;
    float needle = 0; int lastModel = -1;
    juce::Rectangle<float> vuRect() const { return { 16, 52, 330, 150 }; }
    juce::Rectangle<int> modelRect() const { return { getWidth() - 186, 10, 170, 28 }; }
    juce::Rectangle<int> soloRect() const { return { getWidth() - 270, 10, 76, 28 }; }
};

class Graph : public juce::Component
{
public:
    Graph (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    Editor& ed;
};

class GuidePanel : public juce::Component
{
public:
    GuidePanel (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    Editor& ed; int tab = 0; juce::StringArray tips; int warn = 0;
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
    static constexpr int W = 1240, H = 800;
    jlook::LnF lnf;
    Processor& proc;
    jlook::Scaler scaler { *this, "comp", W, H };
    jlook::SizeChip sizeChip { scaler };
    int focus = 0;
    juce::RangedAudioParameter* P (const juce::String& id) { return proc.apvts.getParameter (id); }
    float sv (int s, const char* k) const { return proc.value ((juce::String (stagePfx (s)) + k).toRawUTF8()); }
    void select (int s);
    void computeTips();
    Rack rack { *this };
    StageEditor stage { *this };
private:
    void timerCallback() override;
    Graph graph { *this };
    GuidePanel guide { *this };
    std::unique_ptr<Segs> route, scSrc, scTo;
    std::unique_ptr<Knob> scHpf, gMix, gOut;
    std::unique_ptr<Switch> listen, autoG;
    juce::Rectangle<int> presetRect, prevRect, nextRect, abRect, themeRect { 1112, 19, 100, 28 }; bool abHover = false;
    void recolor();
    int tick = 0;
};
}
