#pragma once
#include "S8Chain.h"
#include "S8Synth.h"

#ifndef S8_SYNTH
 #define S8_SYNTH 0
#endif

namespace s8
{
constexpr bool kSynth = S8_SYNTH != 0;
struct Preset { const char* name; const char* note; std::vector<std::pair<const char*, float>> p; };
const std::vector<Preset>& presets();

class Processor : public juce::AudioProcessor, private juce::AsyncUpdater
{
public:
    Processor();
    void prepareToPlay (double, int) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return kSynth ? "Jackpot 808 Synth" : "Jackpot 808 Studio"; }
    bool acceptsMidi() const override { return kSynth; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return kSynth ? 8.0 : 0.0; }
    int getNumPrograms() override { return (int) presets().size(); }
    int getCurrentProgram() override { return curPreset; }
    void setCurrentProgram (int i) override { loadPreset (i); }
    const juce::String getProgramName (int i) override { return presets()[(size_t) juce::jlimit (0, (int) presets().size() - 1, i)].name; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::UndoManager undo { 400000, 60 };
    juce::AudioProcessorValueTreeState apvts;
    float v (const char* id) const { if (auto* p = apvts.getRawParameterValue (id)) return p->load(); return 0.0f; }
    void setV (const char* id, float real) { if (auto* p = apvts.getParameter (id)) p->setValueNotifyingHost (p->convertTo0to1 (real)); }
    void loadPreset (int i);
    juce::String presetName { "Init" }; int curPreset = 0;
    void resetStats() { ch.resetReq = true; }

    Chain ch; Synth syn;
    // sample dell'utente (solo SYNTH)
    bool loadSample (const juce::File& f, juce::String& err);
    bool loadSampleData (const void* d, size_t n, const juce::String& name, juce::String& err);
    void clearSample();
    void setSampleRoot (float r);
    SampleData::Ptr sampleRef;   // solo dal thread dei messaggi
    std::atomic<int> panics { 0 };
    double sampleRate() const { return ch.sampleRate(); }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    void handleAsyncUpdate() override { suspendProcessing (true); prepareToPlay (getSampleRate() > 0 ? getSampleRate() : 48000.0, juce::jmax (64, getBlockSize())); suspendProcessing (false); }
    ChainSet chainSet() const; SynthSet synthSet() const;
    juce::AudioFormatManager fmt;
    int runaway = 0, muteLeft = 0; float fadeIn = 1; double srNow = 48000;
    juce::AudioBuffer<float> tmp;
};
}
