#pragma once
#include "JackpotLook.h"
#include <juce_audio_utils/juce_audio_utils.h>
#include "ButterDSP.h"

namespace jb
{
class Processor : public juce::AudioProcessor, private juce::AsyncUpdater
{
public:
    Processor();
    void prepareToPlay (double sr, int block) override
    {
        curSr = sr; maxBlock = juce::jmax (64, block); dsp.prepare (sr, maxBlock); setLatencySamples (dsp.latency()); gIn.reset (sr, 0.02); gOut.reset (sr, 0.02); runaway = 0; muteLeft = 0; fadeIn = 1; wild = 0;
        bbLen = (int) (sr * 8); for (auto& b : bb) b.assign ((size_t) bbLen, 0.0f); bbW = 0; bbFreeze = false;
    }
    // scatola nera: gli ultimi 8 secondi d'ingresso; se il suono impazzisce si salvano (con le impostazioni) in Musica / Jackpot / Butter Debug
    std::vector<float> bb[2]; int bbLen = 1, bbW = 0; std::atomic<bool> bbFreeze { false }; int wild = 0, bbCool = 0, bbSaved = 0; float lastPk = 0; std::atomic<int> bbReason { 0 };
    void saveBlackBox();
    // protezione da live: un valore rotto (NaN) o un'autoalimentazione si spengono subito e il plugin riparte pulito
    std::atomic<int> panics { 0 };
    void safety (float* l, float* r, int n, float inPk);
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
    int getCurrentProgram() override { return juce::jmax (0, preset); }
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
    int preset = 0, maxBlock = 512;
    void handleAsyncUpdate() override
    {
        const bool onlyRecord = bbReason.load() == 3;
        if (bbFreeze.load()) saveBlackBox();
        if (! onlyRecord) { suspendProcessing (true); dsp.prepare (curSr, maxBlock); suspendProcessing (false); }
    }
    double curSr = 44100; int runaway = 0, muteLeft = 0; float fadeIn = 1;
    juce::SmoothedValue<float> gIn { 1.0f }, gOut { 1.0f };
    std::atomic<float>* pIn = nullptr; std::atomic<float>* pOut = nullptr;
    juce::String userName;
    juce::String currentName() { return userName.isNotEmpty() ? userName : getProgramName (preset); }
    static juce::File presetFolder();
    bool saveUserPreset (const juce::String&);
    bool loadUserPreset (const juce::File&);
    std::atomic<float> inL { 0 }, inR { 0 }, outL { 0 }, outR { 0 };
private:
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
};
}
