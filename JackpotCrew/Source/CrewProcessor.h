#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "CrewDSP.h"

namespace jcrew
{
struct Preset { juce::String name, cat; std::vector<std::pair<juce::String, float>> v; juce::String note; };
const std::vector<Preset>& presets();

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
    const juce::String getName() const override { return "Jackpot Cybernet Crew"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 1.5; }
    int getNumPrograms() override { return (int) presets().size(); }
    int getCurrentProgram() override { return juce::jmax (0, preset); }
    void setCurrentProgram (int i) override { loadPreset (i); }
    const juce::String getProgramName (int i) override { return juce::isPositiveAndBelow (i, (int) presets().size()) ? presets()[(size_t) i].name : ""; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    void loadPreset (int i);
    static juce::File presetFolder() { auto f = juce::File::getSpecialLocation (juce::File::userMusicDirectory).getChildFile ("Jackpot").getChildFile ("Crew Preset"); f.createDirectory(); return f; }
    bool saveUserPreset (const juce::String& name);
    bool loadUserPreset (const juce::File& f);
    juce::String currentName() const { return userName.isNotEmpty() ? userName : (juce::isPositiveAndBelow (preset, (int) presets().size()) ? presets()[(size_t) preset].name : juce::String ("INIT")); }
    juce::String userName;
    float value (const juce::String& id) const { return apvts.getRawParameterValue (id)->load(); }
    void setP (const juce::String& id, float v) { if (auto* p = apvts.getParameter (id)) p->setValueNotifyingHost (p->convertTo0to1 (v)); }
    Settings settings() const;
    static juce::String vid (int k, const char* what) { return "v" + juce::String (k + 1) + "_" + what; }
    static juce::StringArray allIds();

    juce::UndoManager undo { 200000, 60 };
    juce::AudioProcessorValueTreeState apvts;
    int preset = 0, uiTheme = 0;   // 0 ologrammi, 1 quaderno sporco
    Engine eng;
    std::array<std::atomic<float>, kVoices> uiMeter {};
    std::atomic<float> uiIn { 0 }; double hostBpm = 120;
private:
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
};
}
