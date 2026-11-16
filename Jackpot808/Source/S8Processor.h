#pragma once
#include "S8DSP.h"

namespace s8
{
struct Preset { const char* name; const char* note; float punch, sustain, drive; int mode; float harm, duck, rel; int mono; float ceil; };
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
    const juce::String getName() const override { return "Jackpot 808 Studio"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0; }
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
    void loadPreset (int i);
    juce::String presetName { "Init" };
    void resetStats() { resetReq = true; }

    // ---- per la grafica e la diagnosi
    std::atomic<float> uiHz { 0 }, uiConf { 0 }, uiLevel { -120 };
    std::atomic<float> pcHist[12];            // quanto suona ogni nota
    std::atomic<float> centsAvg { 0 }, f0Avg { 0 }; std::atomic<int> pitchN { 0 };
    std::atomic<float> eSub { 0 }, eBody { 0 }, eHigh { 0 };   // energie medie per banda (ingresso)
    std::atomic<float> sideRatio { -100 }, inPeak { -120 }, crest { 0 };
    std::atomic<int> clips { 0 }, kickHits { 0 };
    std::atomic<float> kickCorr { 0 }, overlapDb { -100 };
    std::atomic<bool> scLive { false };
    std::atomic<float> uiDuck { 1 }, uiGrClip { 0 };
    static constexpr int kHist = 300, kRing = 8192;
    std::atomic<float> hLvl[kHist], hKick[kHist], hDuck[kHist]; std::atomic<int> hW { 0 };
    float ring[kRing] {}; std::atomic<int> ringW { 0 };            // uscita mono per lo spettro
    double sampleRate() const { return sr; }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    double sr = 48000; int osF = 4;
    std::unique_ptr<juce::dsp::Oversampling<float>> os;
    juce::dsp::LinkwitzRileyFilter<float> xLo, xLoH, xHi, xHiH, hHp, monoLp;
    Tuner tuner;
    // transienti
    float envF = 0, envS = 0;
    // cassa
    juce::dsp::IIR::Filter<float> kLp, bLp; float kEnv = 0, kSlow = 0, kPeak = 1e-3f, duckG = 1; int kWin = 0; double kb = 0, kk = 0, bb = 0; float bEnvAtHit = 0, kMaxWin = 0; float bEnv = 0;
    // armoniche
    float hEnv = 0;
    // statistiche
    std::atomic<bool> resetReq { false };
    double sE[3] {}, sSideLo = 0, sMidLo = 0; juce::int64 sN = 0; float pk = 0, winPk = 0, rmsAcc = 0; int rmsN = 0; float crestAvg = 0;
    double pcAcc[12] {}; double centsAcc = 0, logF0Acc = 0; int pN = 0; float corrAvg = 0; float ovAvg = -100; int hits = 0, clipN = 0;
    juce::dsp::LinkwitzRileyFilter<float> sideLp, midLp;
    float colL = 0, colK = 0, colD = 1; int colN = 0, liveHold = 0;
};
}
