#pragma once
// JACKPOT FX: 8 slot di effetti da performance, uno per manopola del controller.
// Gli effetti sono quelli del Sampler (Common/PerfFx.h): stesso codice, stesso suono.
#include <juce_audio_processors/juce_audio_processors.h>
#include "PerfFx.h"
#include <array>
#include <map>

namespace jfx
{
constexpr int kSlots = 8;
inline juce::String typeId (int k) { return "s" + juce::String (k + 1) + "type"; }
inline juce::String amtId (int k)  { return "s" + juce::String (k + 1); }
inline juce::String onId (int k)   { return "s" + juce::String (k + 1) + "on"; }

struct FxPreset { const char* name; const char* note; int type[kSlots]; float amt[kSlots]; };
const std::vector<FxPreset>& factoryPresets();

class Processor : public juce::AudioProcessor, private juce::Timer
{
public:
    Processor();
    ~Processor() override { stopTimer(); }
    void prepareToPlay (double, int) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Jackpot FX"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return presetName; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;
    juce::RangedAudioParameter* P (const juce::String& id) { return apvts.getParameter (id); }
    float value (const juce::String& id) const { return apvts.getRawParameterValue (id)->load(); }
    int slotType (int k) const { return juce::roundToInt (value (typeId (k))); }

    // azioni dall'interfaccia (thread dei messaggi)
    void setSlot (int k, int type, float amt);
    void swapSlots (int a, int b);
    void resetAmounts();                  // tutte le manopole al neutro, effetti scelti invariati
    void loadFactory (int i);
    bool savePreset (const juce::File&);
    bool loadPreset (const juce::File&);
    static juce::File presetFolder();
    juce::String presetName { "Init" };

    // controller
    std::atomic<int> ccMap[kSlots];
    std::atomic<int> learnSlot { -1 };
    std::atomic<bool> ctrlOn { true };    // spento: i CC in arrivo (anche da clip vecchie) si ignorano
    std::atomic<float> ccPend[kSlots];
    std::atomic<int> ccSeen[kSlots];      // per far lampeggiare la manopola quando arriva il CC

    // misure per l'interfaccia
    static constexpr int kHist = 400;     // ~4 s a colonne da 10 ms
    std::atomic<float> hIn[kHist], hOut[kHist]; std::atomic<int> hW { 0 };
    std::atomic<float> pkIn[2], pkOut[2];
    std::atomic<double> uiBpm { 120 }; std::atomic<bool> uiPlay { false };

private:
    void timerCallback() override;
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    pfx::Engine eng;
    juce::AudioBuffer<float> dry;
    std::atomic<float>* typeRaw[kSlots] {}; std::atomic<float>* amtRaw[kSlots] {}; std::atomic<float>* onRaw[kSlots] {};
    double sr = 48000, freeTicks = 0; float mixS = 1, outS = 1;
    float colIn = 0, colOut = 0; int colN = 0;
};
}
