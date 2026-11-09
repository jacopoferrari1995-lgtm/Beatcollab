#pragma once
#include "JackpotLook.h"
#include <juce_audio_utils/juce_audio_utils.h>
#include "ButterDSP.h"

namespace jb
{
class Processor : public juce::AudioProcessor
{
public:
    Processor();
    void prepareToPlay (double sr, int block) override { dsp.prepare (sr, block); }
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& l) const override
    { return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo() && l.getMainInputChannelSet() == juce::AudioChannelSet::stereo(); }
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Jackpot Butter Rack"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }
    int getNumPrograms() override { return (int) burroPresets().size(); }
    int getCurrentProgram() override { return preset; }
    void setCurrentProgram (int i) override { loadPreset (i); }
    const juce::String getProgramName (int i) override { return burroPresets()[(size_t) juce::jlimit (0, getNumPrograms() - 1, i)].name; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;
    jlook::ABCompare ab;

    void loadPreset (int i);
    static juce::StringArray moduleParams (int m);
    juce::AudioProcessorValueTreeState apvts;
    Burro dsp;
    std::atomic<bool> locked[7];
    void randomize();
    int preset = 0;
    std::atomic<float> inL { 0 }, inR { 0 }, outL { 0 }, outR { 0 };
private:
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
};
}
