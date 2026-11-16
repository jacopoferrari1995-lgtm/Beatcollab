#pragma once
// JACKPOT SPACE: rack di 6 slot fra riverberi e delay. Ogni slot sceglie il motore, l'ordine si cambia trascinando,
// e ogni slot puo' stare IN SERIE (prende quello che esce dallo slot prima) o IN PARALLELO (prende il suono pulito).
#include <juce_audio_processors/juce_audio_processors.h>
#include "SpaceDSP.h"

namespace jsp
{
constexpr int kSlots = 6;
inline juce::String typeId (int k) { return "s" + juce::String (k + 1) + "type"; }
inline juce::String onId (int k)   { return "s" + juce::String (k + 1) + "on"; }
inline juce::String mixId (int k)  { return "s" + juce::String (k + 1) + "mix"; }
inline juce::String rtId (int k)   { return "s" + juce::String (k + 1) + "par"; }
inline juce::String pId (int k, int i) { return "s" + juce::String (k + 1) + "p" + juce::String (i + 1); }

struct SpSlot { int type = OFF; float mix = 0.3f; bool par = true; std::vector<float> p {}; };   // p: valori 0..1, -1 = tipico del motore
struct SpPreset { juce::String cat, name, note; std::vector<SpSlot> s; float dry = 1, wet = 0, duck = 0; };
const std::vector<SpPreset>& factoryPresets();

class Processor : public juce::AudioProcessor, private juce::Timer
{
public:
    Processor();
    ~Processor() override { stopTimer(); }
    void updateIRs();                    // carica le risposte della convoluzione quando cambiano (thread dei messaggi)
    void prepareToPlay (double, int) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Jackpot Space"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 20.0; }
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

    void setSlot (int k, int type);          // cambia motore e mette i suoi valori tipici
    void setP (int k, int i, float v);
    void resetP (int k);
    void swapSlots (int a, int b);
    void loadFactory (int i);
    bool savePreset (const juce::File&);
    bool loadPreset (const juce::File&);
    static juce::File presetFolder();
    juce::String presetName { "Init" };
    std::atomic<int> uiTheme { 1 };

    static constexpr int kHist = 400;
    std::atomic<float> hIn[kHist], hOut[kHist]; std::atomic<int> hW { 0 };
    std::atomic<float> pkOut[2], slotLvl[kSlots];
    std::atomic<double> uiBpm { 120 };

private:
    void timerCallback() override { updateIRs(); }
    bool prepared = false;
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    Slot slots[kSlots];
    juce::AudioBuffer<float> dry, chain, par, wet;
    double sr = 48000; float dryS = 1, wetS = 1, outS = 1, env = 0, duckS = 1, mixS[kSlots] {};
    float colIn = 0, colOut = 0; int colN = 0;
};
}
