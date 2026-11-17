#pragma once
#include "FxProcessor.h"
#include "Skin.h"
#include "JackpotScale.h"
#include "HwLook.h"
#include "PresetBrowser.h"

namespace jfx
{
namespace C
{
    inline juce::Colour bg { 0xff0b0c10 }, text { 0xffdfe3ea }, dim { 0xff7d8494 }, faint { 0xff4f5562 };
    inline juce::Colour amber { 0xffffb000 }, pink { 0xffff4fa3 }, violet { 0xffa66bff }, cyan { 0xff5ef2ff }, mint { 0xff5effc1 }, red { 0xffff3b5c }, blue { 0xff5c8dff }, green { 0xff7dff5a };
    inline juce::Colour lcd1 { 0xff15101a }, lcd2 { 0xff09070c };
    inline void sync()
    {
        if (! jlook::skinHooks().on) return;
        const auto& p = jlook::skinPal();
        bg = p.bg; text = p.text; dim = p.dim; faint = p.faint; amber = p.amber; pink = p.pink; cyan = p.cyan; red = p.red; green = p.green; lcd1 = p.lcd0; lcd2 = p.lcd1;
    }
}
juce::Font uf (float size, bool bold = true);
juce::Font mf (float size);
void glowText (juce::Graphics&, const juce::String&, juce::Rectangle<float>, juce::Colour, juce::Font, juce::Justification = juce::Justification::centredLeft);
void lcd (juce::Graphics&, juce::Rectangle<float>, float rad);
juce::Colour groupCol (int type);

class Editor;

// manopola da frontale (come Dust e Alter): si ridisegna solo quando cambia
class Knob : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit Knob (juce::RangedAudioParameter* p) : param (p) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override { param->setValueNotifyingHost (defV >= 0 ? defV : param->getDefaultValue()); }
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void mouseEnter (const juce::MouseEvent&) override { hover = true; setMouseCursor (juce::MouseCursor::UpDownResizeCursor); repaint(); }
    void mouseExit (const juce::MouseEvent&) override { hover = false; repaint(); }
    void refresh (bool on) { const auto t = text ? text() : param->getCurrentValueAsText(); const float v = param->getValue(); if (t != lastT || v != lastV || on != lit) { lastT = t; lastV = v; lit = on; repaint(); } }
    juce::RangedAudioParameter* param; hw::KnobSpec spec; std::function<juce::String()> text; std::function<void()> onRelease; float defV = -1; bool lit = true;
private:
    float start = 0, lastV = -1; bool drag = false, hover = false; juce::String lastT;
};

// onda SCAN (come nel Master): il cursore spazza da sinistra a destra, l'entrata scura dietro e l'uscita colorata sopra;
// sotto, una corsia per slot che si accende dove quell'effetto ha lavorato
class Scope : public juce::Component
{
public:
    explicit Scope (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    Editor& ed;
};

class SlotCard : public juce::Component
{
public:
    SlotCard (Editor& e, int k);
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void refresh();
    void tick();
    void showMenu();
    juce::String key() const;
    Editor& ed; const int k;
    std::unique_ptr<Knob> knob, mixK;
    juce::Rectangle<float> nameR, gearR, ccR, learnR, railR;
    int shownType = -1, lastSeen = 0; float flash = 0; bool dragging = false, dropHot = false;
    jlook::StaticLayer base;
};

class AdvPanel : public juce::Component
{
public:
    explicit AdvPanel (Editor& e) : ed (e) {}
    void open (int k);
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void tick();
    Editor& ed; int k = -1, shownType = -1, verbZone = -1;
    std::vector<std::unique_ptr<Knob>> knobs;
    juce::Rectangle<float> closeR, resetR;
};

class Editor : public juce::AudioProcessorEditor, private juce::Timer
{
    skin::Keep skinKeep;
public:
    explicit Editor (Processor&);
    ~Editor() override { stopTimer(); setLookAndFeel (nullptr); }
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    static constexpr int W = 1200, H = 812;
    static juce::Rectangle<float> scopeCard() { return { 18, 72, 882, 290 }; }
    static juce::Rectangle<float> outCard() { return { 910, 72, 272, 290 }; }
    static juce::Rectangle<float> card (int k) { const float w = (1164.0f - 7 * 6.0f) / kSlots; return { 18 + k * (w + 6), 372, w, 410 }; }
    jlook::LnF lnf;
    Processor& proc;
    jlook::Scaler scaler { *this, "fx", W, H };
    jlook::SizeChip sizeChip { scaler };
    Scope scope { *this };
    AdvPanel advPanel { *this };
    juce::OwnedArray<SlotCard> cards;
    std::unique_ptr<Knob> mixK, outK;
    jbrowse::Browser browser;
    juce::Rectangle<int> presetR, prevR, nextR, resetR, bypR, ctrlR, themeR;
    int hoverBtn = -1, dragFrom = -1; juce::Point<float> dragPos;
    void endDrag (juce::Point<float> editorPos);
    void refreshCards() { for (auto* c : cards) c->refresh(); }
    void tickForTest() { for (int i = 0; i < 30; ++i) timerCallback(); }
private:
    void timerCallback() override;
    juce::String staticKey();
    void paintStatic (juce::Graphics&);
    void setupBrowser();
    void askPresetName();
    jlook::StaticLayer bg;
};
}
