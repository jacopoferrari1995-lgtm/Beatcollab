#pragma once
// Dimensione dell'interfaccia (25%..150%) per tutti i plugin Jackpot, ricordata per ogni plugin.
#include <juce_audio_processors/juce_audio_processors.h>
#include "JackpotLook.h"

namespace jlook
{
inline juce::PropertiesFile::Options settingsOptions()
{
    juce::PropertiesFile::Options o;
    o.applicationName = "Jackpot"; o.filenameSuffix = ".settings"; o.folderName = "Jackpot"; o.osxLibrarySubFolder = "Application Support";
    return o;
}
inline float loadScale (const juce::String& id) { juce::PropertiesFile pf (settingsOptions()); return juce::jlimit (0.25f, 1.5f, (float) pf.getDoubleValue ("scale_" + id, 1.0)); }
inline void saveScale (const juce::String& id, float s) { juce::PropertiesFile pf (settingsOptions()); pf.setValue ("scale_" + id, (double) s); pf.saveIfNeeded(); }

// trasforma i figli dell'editor (non la maniglia di ridimensionamento di JUCE)
inline void scaleChildren (juce::Component& parent, float s)
{
    for (auto* c : parent.getChildren())
        if (dynamic_cast<juce::ResizableCornerComponent*> (c) == nullptr && dynamic_cast<juce::ResizableBorderComponent*> (c) == nullptr)
            c->setTransform (juce::AffineTransform::scale (s));
}

struct Scaler
{
    Scaler (juce::AudioProcessorEditor& e, juce::String i, int w, int h) : ed (e), id (std::move (i)), W (w), H (h) {}
    ~Scaler() { saveScale (id, s); }
    void setup()
    {
        ed.setResizable (true, true);
        ed.setResizeLimits (W / 4, H / 4, W * 3 / 2, H * 3 / 2);
        if (auto* c = ed.getConstrainer()) c->setFixedAspectRatio ((double) W / H);
        s = loadScale (id);
        ed.setSize (juce::roundToInt (W * s), juce::roundToInt (H * s));
    }
    float update() { s = juce::jlimit (0.2f, 2.0f, ed.getWidth() / (float) W); return s; }
    void set (float ns) { s = ns; saveScale (id, ns); ed.setSize (juce::roundToInt (W * ns), juce::roundToInt (H * ns)); }
    juce::Rectangle<int> map (juce::Rectangle<int> r) const { return (r.toFloat() * s).getSmallestIntegerContainer(); }
    juce::AudioProcessorEditor& ed; juce::String id; int W, H; float s = 1.0f;
};

// pulsantino "dimensione" con menu 25..150%
class SizeChip : public juce::Component
{
public:
    explicit SizeChip (Scaler& sc) : scaler (sc) { setMouseCursor (juce::MouseCursor::PointingHandCursor); }
    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat().reduced (0.5f);
        g.setColour (juce::Colours::black.withAlpha (hover ? 0.7f : 0.5f)); g.fillRoundedRectangle (b, 4);
        g.setColour (juce::Colour (0xff5ef2ff).withAlpha (hover ? 0.8f : 0.35f)); g.drawRoundedRectangle (b, 4, 1.0f);
        // icona: due angoli
        auto ic = b.removeFromLeft (b.getHeight()).reduced (4);
        juce::Path p; p.startNewSubPath (ic.getX(), ic.getY() + 3); p.lineTo (ic.getX(), ic.getY()); p.lineTo (ic.getX() + 3, ic.getY());
        p.startNewSubPath (ic.getRight() - 3, ic.getBottom()); p.lineTo (ic.getRight(), ic.getBottom()); p.lineTo (ic.getRight(), ic.getBottom() - 3);
        g.setColour (juce::Colour (0xff5ef2ff)); g.strokePath (p, juce::PathStrokeType (1.2f));
        g.setColour (juce::Colour (0xffdfe3ea).withAlpha (0.85f)); g.setFont (mono (9.5f));
        g.drawText (juce::String (juce::roundToInt (scaler.s * 100)) + "%", b, juce::Justification::centred);
    }
    void mouseEnter (const juce::MouseEvent&) override { hover = true; repaint(); }
    void mouseExit (const juce::MouseEvent&) override { hover = false; repaint(); }
    void mouseDown (const juce::MouseEvent&) override
    {
        juce::PopupMenu m; m.addSectionHeader ("DIMENSIONE INTERFACCIA");
        const int cur = juce::roundToInt (scaler.s * 100);
        for (int pct : { 25, 50, 75, 100, 125, 150 }) m.addItem (pct, juce::String (pct) + "%", true, std::abs (cur - pct) < 3);
        juce::Component::SafePointer<SizeChip> safe (this);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [safe] (int r)
        {
            if (r <= 0 || safe == nullptr) return;
            auto* sc = &safe->scaler;
            juce::MessageManager::callAsync ([sc, r, safe] { if (safe != nullptr) sc->set (r / 100.0f); });
        });
    }
private:
    Scaler& scaler; bool hover = false;
};
}
