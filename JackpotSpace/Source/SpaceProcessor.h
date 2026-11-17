#pragma once
// JACKPOT SPACE: rack di 6 slot fra riverberi e delay. Ogni slot sceglie il motore, l'ordine si cambia trascinando,
// e ogni slot puo' stare IN SERIE (prende quello che esce dallo slot prima) o IN PARALLELO (prende il suono pulito).
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
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
    // risposte all'impulso caricate dall'utente (una per slot)
    juce::String userIR[kSlots];
    bool setUserIR (int k, const juce::File& f);      // carica il file, mette SPAZIO = TUO FILE
    juce::String userIRName (int k) const { return userIR[k].isEmpty() ? juce::String() : juce::File (userIR[k]).getFileNameWithoutExtension(); }
    std::atomic<int> uiTheme { 1 };

    static constexpr int kHist = 4800;   // 24 s a colonne da 5 ms (onda SCAN come il Master)
    static constexpr double kColSec = 0.005;
    std::atomic<float> hIn[kHist], hOut[kHist], hPpq[kHist]; std::atomic<int> hMask[kHist]; std::atomic<long> hTot { 0 };
    std::atomic<bool> uiPlay { false };
    std::atomic<float> pkOut[2], slotLvl[kSlots];
    std::atomic<double> uiBpm { 120 };

private:
    void timerCallback() override { updateIRs(); }
    bool prepared = false;
    juce::AudioBuffer<float> irData[kSlots]; double irRate[kSlots] {}; juce::String irLoaded[kSlots];
    bool readIR (int k);
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    Slot slots[kSlots];
    juce::AudioBuffer<float> dry, chain, par, wet;
    double sr = 48000; float dryS = 1, wetS = 1, outS = 1, env = 0, duckS = 1, mixS[kSlots] {};
    float colIn = 0, colOut = 0; int colN = 0, colMask = 0;
    static constexpr int kSub = 32;
    static bool isStep (Kind k) { return k == KSPR || k == KPITCH || k == KIR || k == KPAT || k == KONOFF || k == KGP || k == KSYNC; }
    int runT[kSlots] {}; float fadeG[kSlots] {}, ps[kSlots][kP] {}; bool idle[kSlots] {}, needClear[kSlots] {};
    void processChunk (float* L, float* R, int N, double bpm, double ppq, bool play);
};
}
