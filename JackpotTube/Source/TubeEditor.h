#pragma once
#include "TubeProcessor.h"

namespace jtbx
{
using namespace jkit;
class Editor;
class VuView : public juce::Component { public: explicit VuView (Editor& e) : ed (e) {} void paint (juce::Graphics&) override; Editor& ed; float needle = 0; };
class BandView : public juce::Component
{
public:
    explicit BandView (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    float xOf (float f) const { return 10.0f + (getWidth() - 20.0f) * std::log (f / 20.0f) / std::log (1000.0f); }
    float fOf (float x) const { return 20.0f * std::pow (1000.0f, juce::jlimit (0.0f, 1.0f, (x - 10.0f) / (getWidth() - 20.0f))); }
    Editor& ed; int drag = -1, hover = -1;
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
    static constexpr int W = 1180, H = 800;
    const juce::Rectangle<float> hgCard { 18, 76, 1144, 352 }, mbCard { 18, 440, 1144, 344 };
    jlook::LnF lnf;
    Processor& proc;
    jlook::Scaler scaler { *this, "tubebox", W, H };
    jlook::SizeChip sizeChip { scaler };
    juce::RangedAudioParameter* P (const juce::String& id) { return proc.apvts.getParameter (id); }
    const juce::Colour gold { 0xffe8b04a }, acc2 = C::magenta;
private:
    void timerCallback() override;
    struct KS { std::unique_ptr<Knob> k; int sec; };
    std::vector<KS> knobs; std::vector<std::unique_ptr<Switch>> switches; std::vector<std::unique_ptr<Segs>> segs;
    VuView vu { *this }; BandView bands { *this };
    PresetBar bar; int tick = 0;
};
}
