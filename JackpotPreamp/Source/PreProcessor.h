#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PreDSP.h"

namespace jpre
{
const std::vector<jkit::Preset>& presets();
struct Ring
{
    static constexpr int kSize = 8192;
    float d[kSize] {}; std::atomic<int> w { 0 };
    void push (const float* l, const float* r, int n) { int p = w.load(); for (int i = 0; i < n; ++i) { d[p] = 0.5f * (l[i] + r[i]); p = (p + 1) & (kSize - 1); } w = p; }
    void read (float* out, int n) const { int p = (w.load() - n) & (kSize - 1); for (int i = 0; i < n; ++i) { out[i] = d[p]; p = (p + 1) & (kSize - 1); } }
};

class Processor : public juce::AudioProcessor
{
public:
    Processor();
    void prepareToPlay (double, int) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Jackpot Preamp EQ"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.05; }
    int getNumPrograms() override { return (int) presets().size(); }
    int getCurrentProgram() override { return preset; }
    void setCurrentProgram (int i) override { loadPreset (i); }
    const juce::String getProgramName (int i) override { return juce::isPositiveAndBelow (i, (int) presets().size()) ? presets()[(size_t) i].name : ""; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    void loadPreset (int i);
    float value (const char* id) const { return apvts.getRawParameterValue (id)->load(); }
    Settings settings() const;
    double sampleRate() const { return sr; }

    juce::AudioProcessorValueTreeState apvts;
    jlook::ABCompare ab;
    int preset = 0;
    Engine eng;
    Ring ring;
private:
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    double sr = 44100;
    static constexpr int kChunk = 2048;
};
}
