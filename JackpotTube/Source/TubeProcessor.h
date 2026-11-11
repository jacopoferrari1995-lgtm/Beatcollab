#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "TubeDSP.h"

namespace jtbx
{
const std::vector<jkit::Preset>& presets();
class Processor : public juce::AudioProcessor
{
public:
    Processor();
    void prepareToPlay (double, int) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& l) const override
    { const auto o = l.getMainOutputChannelSet(), i = l.getMainInputChannelSet(); return o == juce::AudioChannelSet::stereo() && (i == o || i == juce::AudioChannelSet::mono()); }
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Jackpot Tube Box"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return (int) presets().size(); }
    int getCurrentProgram() override { return preset; }
    void setCurrentProgram (int i) override { loadPreset (i); }
    const juce::String getProgramName (int i) override { return juce::isPositiveAndBelow (i, (int) presets().size()) ? presets()[(size_t) i].name : ""; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;
    void loadPreset (int i);
    float value (const char* id) const { return apvts.getRawParameterValue (id)->load(); }
    float valueS (const juce::String& id) const { return apvts.getRawParameterValue (id)->load(); }
    Settings settings() const;
    juce::AudioProcessorValueTreeState apvts;
    jlook::ABCompare ab;
    int preset = 0;
    Engine eng;
private:
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    static constexpr int kChunk = 2048;
};
}
