#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "ChoirDSP.h"

namespace jcho
{
const std::vector<jkit::Preset>& presets();

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
    const juce::String getName() const override { return "Jackpot Choir"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.2; }
    int getNumPrograms() override { return (int) presets().size(); }
    int getCurrentProgram() override { return juce::jmax (0, preset); }
    void setCurrentProgram (int i) override { loadPreset (i); }
    const juce::String getProgramName (int i) override { return juce::isPositiveAndBelow (i, (int) presets().size()) ? presets()[(size_t) i].name : ""; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    void loadPreset (int i);
    static juce::File presetFolder() { auto f = juce::File::getSpecialLocation (juce::File::userMusicDirectory).getChildFile ("Jackpot").getChildFile ("Choir Preset"); f.createDirectory(); return f; }
    bool saveUserPreset (const juce::String& name);
    bool loadUserPreset (const juce::File& f);
    juce::String currentName() { return userName.isNotEmpty() ? "I miei - " + userName : getProgramName (preset); }
    juce::String userName;
    float value (const char* id) const { return apvts.getRawParameterValue (id)->load(); }
    float value (const juce::String& id) const { return apvts.getRawParameterValue (id)->load(); }
    Settings settings() const;

    juce::UndoManager undo { 200000, 60 };
    juce::AudioProcessorValueTreeState apvts;
    jlook::ABCompare ab;
    int preset = 0;
    std::unique_ptr<Engine> eng { std::make_unique<Engine>() };
    std::array<std::atomic<int>, 4> uiHeld { -1, -1, -1, -1 };
private:
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    std::array<bool, 128> down {}; std::vector<int> order;
};
}
