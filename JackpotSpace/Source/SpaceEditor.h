#pragma once
#include "SpaceProcessor.h"
#include "Skin.h"
#include "JackpotScale.h"
#include "HwLook.h"
#include "PresetBrowser.h"

namespace jsp
{
namespace C
{
    inline juce::Colour bg { 0xff0b0c10 }, text { 0xffdfe3ea }, dim { 0xff7d8494 }, faint { 0xff4f5562 };
    inline juce::Colour amber { 0xffffb000 }, pink { 0xffff4fa3 }, violet { 0xffa66bff }, cyan { 0xff5ef2ff }, mint { 0xff5effc1 }, red { 0xffff3b5c }, blue { 0xff5c8dff }, green { 0xff7dff5a };
    inline juce::Colour lcd1 { 0xff0f1420 }, lcd2 { 0xff070a10 };
    inline void sync()
    {
        if (! jlook::skinHooks().on) return;
        const auto& p = jlook::skinPal();
        bg = p.bg; text = p.text; dim = p.dim; faint = p.faint; amber = p.amber; pink = p.pink; cyan = p.cyan; red = p.red; green = p.green; lcd1 = p.lcd0; lcd2 = p.lcd1;
    }
}
juce::Colour typeCol (int t);

class Editor;

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
    juce::RangedAudioParameter* param; hw::KnobSpec spec; std::function<juce::String()> text; float defV = -1; bool lit = true;
private:
    float start = 0, lastV = -1; bool drag = false, hover = false; juce::String lastT;
};

// onda SCAN del Master: entrata scura dietro, solo riverberi e delay colorati sopra, corsie degli slot sotto
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
    std::unique_ptr<Knob> knob, k2, mixK;
    juce::Rectangle<float> nameR, powR, headR, gearR, serR, parR, meterR, soloR;
    int shownType = -1; bool dragging = false, dropHot = false; float lvlShown = 0;
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
    Editor& ed; int k = -1, shownType = -1;
    std::vector<std::unique_ptr<Knob>> knobs;
    juce::Rectangle<float> closeR, resetR, irR;
    std::unique_ptr<juce::FileChooser> chooser;
};

class Editor : public juce::AudioProcessorEditor, public juce::FileDragAndDropTarget, private juce::Timer
{
    skin::Keep skinKeep;
public:
    explicit Editor (Processor&);
    ~Editor() override { stopTimer(); removeMouseListener (&undoMark); setLookAndFeel (nullptr); }
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    bool isInterestedInFileDrag (const juce::StringArray& f) override { for (auto& x : f) if (juce::File (x).hasFileExtension ("wav;aif;aiff;flac")) return true; return false; }
    void filesDropped (const juce::StringArray& f, int x, int y) override;
    static constexpr int W = 1200, H = 800;
    static juce::Rectangle<float> scopeCard() { return { 18, 72, 882, 300 }; }
    static juce::Rectangle<float> outCard() { return { 910, 72, 272, 300 }; }
    static juce::Rectangle<float> card (int k) { const float w = (1164.0f - (kSlots - 1) * 8.0f) / kSlots; return { 18 + k * (w + 8), 382, w, 384 }; }
    jlook::LnF lnf;
    Processor& proc;
    jlook::Scaler scaler { *this, "space", W, H };
    jlook::SizeChip sizeChip { scaler };
    Scope scope { *this };
    AdvPanel advPanel { *this };
    juce::OwnedArray<SlotCard> cards;
    std::unique_ptr<Knob> dryK, wetK, duckK, outK;
    jbrowse::Browser browser;
    juce::Rectangle<int> presetR, prevR, nextR, frzR, bypR, themeR, undoR, redoR;
    bool keyPressed (const juce::KeyPress&) override;
    void doUndo (bool redo);
    struct UndoMark : juce::MouseListener
    {
        Processor* p = nullptr; double lastWheel = 0;
        void mouseDown (const juce::MouseEvent&) override { if (p) p->undo.beginNewTransaction(); }
        void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override { const double t = juce::Time::getMillisecondCounterHiRes(); if (p && t - lastWheel > 600) p->undo.beginNewTransaction(); lastWheel = t; }
    } undoMark;
    int hoverBtn = -1, dragFrom = -1, undoTick = 0; juce::Point<float> dragPos;
    void endDrag (juce::Point<float> editorPos);
    void refreshAll() { for (auto* c : cards) c->refresh(); if (advPanel.isVisible()) advPanel.open (advPanel.k); repaint(); }
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
