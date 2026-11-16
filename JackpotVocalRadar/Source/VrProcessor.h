#pragma once
#include "VrDSP.h"

namespace vr
{
struct VrPreset { const char* name; const char* note; float amount, pres, body, air, vol, stereo, rel, ant; };
const std::vector<VrPreset>& presets();

// risultato della prova d'ascolto (ONE CLICK)
struct Proposal { bool ready = false; juce::StringArray lines; float amount = 0, pres = 0, body = 0, air = 0, vol = 0, stereo = 0, rel = 0, ant = 0; };

class Processor : public juce::AudioProcessor
{
public:
    Processor();
    ~Processor() override;
    void prepareToPlay (double, int) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Jackpot Vocal Radar"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.05; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;
    float v (const char* id) const { return apvts.getRawParameterValue (id)->load(); }
    void setV (const char* id, float real) { if (auto* p = apvts.getParameter (id)) p->setValueNotifyingHost (p->convertTo0to1 (real)); }
    bool beatMode() const { return v ("mode") > 0.5f; }
    int channel() const { return juce::roundToInt (v ("chan")); }
    void loadPreset (int i);
    juce::String presetName { "Init" };

    // ONE CLICK: ascolta voce e beat insieme e propone l'intervento
    void learnStart(); void learnStop(); void learnApply(); void learnDiscard();
    std::atomic<bool> learning { false }; std::atomic<float> learnSec { 0 };
    Proposal proposal; bool hasBackup = false; float backup[8] {};

    // per la grafica
    std::atomic<float> uiVoice[NB], uiBeat[NB], uiCut[NB], uiAct { 0 }, uiVol { 0 }, uiVoiceLvl { -120 };
    static constexpr int kHist = 360;
    std::atomic<float> hAct[kHist], hCut[kHist]; std::atomic<int> hW { 0 };
    std::atomic<bool> uiLinked { false }, uiSide { false };
    std::atomic<int> uiTheme { 0 };
    const int slot;   // posto fra i ricevitori

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    void processVoice (const float* L, const float* R, int n, double pos0, bool playing);
    void processBeat (juce::AudioBuffer<float>& buf, int n, double pos0, bool playing);
    const Frame* findFrame (int c, double want, bool playing) const;
    double sr = 48000; int L = 1920;
    Bank vBank, bBank, scBank; Frame scFrame; bool scValid = false;
    Peak pm[NB], ps[NB];
    std::vector<float> dM, dS; int dW = 0, dMask = 0;
    std::vector<std::array<float, NB + 1>> gq; int gW = 0, gMask = 0;   // tagli per fotogramma, ritardati per l'anticipo
    float cutS[NB] {}, volS = 0, actEnv = 0, avgLvl = -30; int hopN = 0; double vPos = 0; juce::int64 vCounter = 0;
    float colA = 0, colC = 0; int colN = 0;
    // statistiche della prova d'ascolto
    double lsV[NB] {}, lsB[NB] {}, lsSideRatio = 0; int lsN = 0, lsHops = 0, lsOn = 0, lsOnsets = 0; float lsPrevAct = 0; double lsBeatMinusVoice = 0;
    void learnHop (const float* vb, const float* bb, float act, float sideRatio, float bAll, float vAll);
};
}
