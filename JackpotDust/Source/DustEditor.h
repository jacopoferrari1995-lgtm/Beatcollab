#pragma once
#include "DustProcessor.h"
#include "Skin.h"
#include "JackpotScale.h"
#include "HwLook.h"
#include "PresetBrowser.h"

namespace jds
{
namespace C
{
    inline juce::Colour bg { 0xff0b0c10 }, text { 0xffdfe3ea }, dim { 0xff7d8494 }, faint { 0xff4f5562 };
    inline juce::Colour amber { 0xffffb000 }, rust { 0xffff7a1a }, cream { 0xfff3d9a4 }, olive { 0xffb5c94a }, phos { 0xff5ef2ff }, red { 0xffff3b5c }, rose { 0xffff6f91 }, green { 0xff7dff5a };
    inline juce::Colour lcd1 { 0xff1a1408 }, lcd2 { 0xff0c0904 };
    inline void sync()
    {
        if (! jlook::skinHooks().on) return;
        const auto& p = jlook::skinPal();
        bg = p.bg; text = p.text; dim = p.dim; faint = p.faint; amber = p.amber; rust = p.red.interpolatedWith (p.amber, 0.5f);
        cream = (p.light ? p.text : p.amber.interpolatedWith (p.hi, 0.6f)); olive = p.green; phos = p.cyan; red = p.red; rose = p.pink; green = p.green; lcd1 = p.lcd0; lcd2 = p.lcd1;
    }
}
juce::Font uf (float size, bool bold = true);
juce::Font mf (float size);
void glowText (juce::Graphics&, const juce::String&, juce::Rectangle<float>, juce::Colour, juce::Font, juce::Justification = juce::Justification::centredLeft);
void lcd (juce::Graphics&, juce::Rectangle<float>, float rad);

class Editor;
enum Mod { M_SAMP, M_SAT, M_FILT, M_COMP, M_TAPE, M_ROOM, M_DUST, kMods };

// manopola da frontale: si ridisegna solo quando cambia valore o stato
class Knob : public juce::Component
{
public:
    Knob (juce::RangedAudioParameter* p) : param (p) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent&) override { if (drag) param->endChangeGesture(); drag = false; repaint(); }
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseDoubleClick (const juce::MouseEvent&) override { param->setValueNotifyingHost (param->getDefaultValue()); }
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w) override;
    void mouseEnter (const juce::MouseEvent&) override { hover = true; setMouseCursor (juce::MouseCursor::UpDownResizeCursor); repaint(); }
    void mouseExit (const juce::MouseEvent&) override { hover = false; repaint(); }
    void refresh (bool on) { const auto t = text ? text() : param->getCurrentValueAsText(); const float v = param->getValue(); if (t != lastT || v != lastV || on != lit) { lastT = t; lastV = v; lit = on; repaint(); } }
    juce::RangedAudioParameter* param; hw::KnobSpec spec; std::function<juce::String()> text; std::function<void()> onTouch; bool lit = true;
private:
    float start = 0, lastV = -1; bool drag = false, hover = false; juce::String lastT;
};
class Segs : public juce::Component
{
public:
    Segs (juce::RangedAudioParameter* p, juce::Colour c, juce::StringArray it) : param (p), col (c), items (std::move (it)) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void refresh (bool on) { const int v = juce::roundToInt (param->convertFrom0to1 (param->getValue())); if (v != last || on != lit) { last = v; lit = on; repaint(); } }
    juce::RangedAudioParameter* param; juce::Colour col; juce::StringArray items; float font = 10.0f; bool lit = true; int last = -1; std::function<void (int)> onPick;
};
class Switch : public juce::Component
{
public:
    Switch (juce::RangedAudioParameter* p, juce::Colour c, juce::String l = {}) : param (p), col (c), label (std::move (l)) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { param->setValueNotifyingHost (param->getValue() > 0.5f ? 0.0f : 1.0f); if (onTouch) onTouch(); repaint(); }
    void refresh() { const bool v = param->getValue() > 0.5f; if (v != last) { last = v; repaint(); } }
    juce::RangedAudioParameter* param; juce::Colour col, ink { 0xffe8e4d8 }; juce::String label; bool last = false; std::function<void()> onTouch;
};
// display piccolo dentro ogni modulo
class ModView : public juce::Component
{
public:
    ModView (Editor& e, int m) : ed (e), mod (m) {}
    void paint (juce::Graphics&) override;
    juce::String key() const;   // cambia quando il disegno deve cambiare
    Editor& ed; int mod; float trace[90] {}; int tw = 0; float spin = 0; juce::String lastKey;
};
class Scope : public juce::Component
{
public:
    Scope (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    Editor& ed;
};

class Editor : public juce::AudioProcessorEditor, private juce::Timer
{
    skin::Keep skinKeep;
public:
    explicit Editor (Processor&);
    ~Editor() override { setLookAndFeel (nullptr); }
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void tickForTest() { for (int i = 0; i < 30; ++i) timerCallback(); }
    static constexpr int W = 1200, H = 816;
    static juce::Rectangle<float> unit (int u) { return { 18.0f + (u % 4) * 223.0f, 172.0f + (u / 4) * 322.0f, 213, 312 }; }
    static juce::Rectangle<float> outCard() { return { 910, 72, 272, 732 }; }
    static juce::Rectangle<float> genreCard() { return { 18, 72, 882, 92 }; }
    static juce::Rectangle<float> genreChip (int g) { return { 132.0f + g * 85.0f, 84, 80, 28 }; }
    static juce::Rectangle<float> targetChip (int t) { return { 132.0f + t * 110.0f, 124, 104, 28 }; }
    static const char* onId (int m) { static const char* ids[] = { "s_on", "d_on", "f_on", "c_on", "t_on", "r_on", "n_on" }; return ids[m]; }
    bool isOn (int m) const { return proc.value (onId (m)) > 0.5f; }
    jlook::LnF lnf;
    Processor& proc;
    jlook::Scaler scaler { *this, "dust", W, H };
    jlook::SizeChip sizeChip { scaler };
    juce::RangedAudioParameter* P (const juce::String& id) { return proc.apvts.getParameter (id); }
    jbrowse::Browser browser;
private:
    void timerCallback() override;
    juce::String staticKey();
    void paintStatic (juce::Graphics&);
    void setupBrowser();
    void askPresetName();
    std::vector<std::unique_ptr<Knob>> knobs; std::vector<std::pair<Knob*, int>> knobMod;
    std::vector<std::unique_ptr<Segs>> segs; std::vector<std::pair<Segs*, int>> segMod;
    std::vector<std::unique_ptr<Switch>> switches;
    std::vector<std::unique_ptr<ModView>> views;
    Scope scope { *this };
    Knob* kAmount = nullptr;
    jlook::StaticLayer bg;
    juce::Rectangle<int> presetRect, prevRect, nextRect, abRect, themeRect; bool abHover = false; int tick = 0; float mShown[4] { -1, -1, -1, -1 };
    juce::String lastKey;
};
}
