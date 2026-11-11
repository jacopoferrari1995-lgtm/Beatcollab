#pragma once
#include "JackpotLook.h"
#include "CompDSP.h"
#include "CompGuide.h"

namespace jc
{
struct Preset { const char* name; const char* note; int route; std::vector<std::pair<const char*, float>> v; };
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
    const juce::String getName() const override { return "Jackpot Studio Comp"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0; }
    int getNumPrograms() override { return (int) presets().size(); }
    int getCurrentProgram() override { return preset; }
    void setCurrentProgram (int i) override { loadPreset (i); }
    const juce::String getProgramName (int i) override { return juce::isPositiveAndBelow (i, (int) presets().size()) ? presets()[(size_t) i].name : ""; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    void loadPreset (int i);
    StageParams stageParams (int s) const;
    float value (const char* id) const { return apvts.getRawParameterValue (id)->load(); }

    juce::AudioProcessorValueTreeState apvts;
    jlook::ABCompare ab;
    int preset = 0;
    bool hasExtSidechain() const { return extSc.load(); }

    // misure per l'interfaccia
    static constexpr int kHist = 400;
    std::atomic<float> hIn[kHist], hOut[kHist], hGr[kStages][kHist]; std::atomic<int> hW { 0 };
    std::atomic<float> mbGr[4] {}, gr[kStages] {}, avgGr[kStages] {}, inPk[2] {}, outPk[2] {}, inRms { 0 }, outRms { 0 };
    bool anySolo() const { for (int s = 0; s < kStages; ++s) if (value ((juce::String (stagePfx (s)) + "solo").toRawUTF8()) > 0.5f) return true; return false; }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    Stage st[kStages]; double sr = 44100; MbParams mbNow;
    juce::AudioBuffer<float> dry, det, aux, acc, dlt;
    juce::dsp::IIR::Filter<float> hp[kStages][2];  // per stadio, per canale
    float lastHp = -1; std::atomic<bool> extSc { false };
    float colIn = 0, colOut = 0, colGr[kStages] {}; int colN = 0; float rmsIn = 0, rmsOut = 0;
};
}
