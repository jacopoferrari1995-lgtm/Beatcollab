#pragma once
#include "JackpotLook.h"
#include "CompDSP.h"
#include "CompGuide.h"
#include "CompListen.h"

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
    void processBlockBypassed (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Jackpot Studio Comp"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0; }
    int getNumPrograms() override { return (int) presets().size(); }
    int getCurrentProgram() override { return juce::jmax (0, preset); }
    void setCurrentProgram (int i) override { loadPreset (i); }
    const juce::String getProgramName (int i) override { return juce::isPositiveAndBelow (i, (int) presets().size()) ? presets()[(size_t) i].name : ""; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    void loadPreset (int i);
    juce::String userName;   // preset dell'utente caricato (vuoto = preset di fabbrica)
    juce::String currentName() { return userName.isNotEmpty() ? userName : getProgramName (preset); }
    static juce::File presetFolder();
    bool saveUserPreset (const juce::String& name);
    bool loadUserPreset (const juce::File&);
    void convertLegacy (int fromVersion);
    void convertStage (int s, int fromVersion);
    static constexpr int kStateVersion = 3;
    StageParams stageParams (int s) const;
    float value (const char* id) const { return apvts.getRawParameterValue (id)->load(); }

    juce::AudioProcessorValueTreeState apvts;
    jlook::ABCompare ab;
    int preset = 0;
    bool hasExtSidechain() const { return extSc.load(); }

    // misure per l'interfaccia
    static constexpr int kHist = 400;
    std::atomic<float> hIn[kHist], hOut[kHist], hGr[kStages][kHist]; std::atomic<int> hW { 0 };
    std::atomic<float> stIn[kStages] {}, mbGr[4] {}, gr[kStages] {}, avgGr[kStages] {}, inPk[2] {}, outPk[2] {}, inRms { 0 }, outRms { 0 };
    // ASCOLTO: cattura, proposta, applicazione e taratura automatica delle soglie
    Capture cap; Plan plan; int listenSrc = SRC_AUTO;
    int calib = -1, calibOk = 0, calibSteps = 0; float calibT[kStages] {}; std::vector<float> calibBuf; juce::String calibMsg;
    void startListen() { plan = {}; calib = -1; calibMsg = {}; cap.start(); }
    void finishListen() { plan = analyse (cap, listenSrc); }
    void applyPlan();
    void calibTick();   // dal timer dell'editor, 30 volte al secondo
    bool anySolo() const { for (int s = 0; s < kStages; ++s) if (pp[s].solo->load() > 0.5f) return true; return false; }
    bool deltaOn() const { return gDelta->load() > 0.5f; }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    struct SP { std::atomic<float> *on, *model, *solo, *drive, *ratio, *att, *rel, *out, *color, *mix, *drive2, *att2, *ratio2, *link, *olink, *thr, *cratio, *mode, *hp; } pp[kStages] {};
    struct MP { std::atomic<float> *thr, *ratio, *att, *rel, *gain, *solo, *byp; } mbp[4] {};
    std::atomic<float> *gx[3] {}, *gScSrc = nullptr, *gScTo = nullptr, *gScHpf = nullptr, *gScListen = nullptr, *gRoute = nullptr, *gAuto = nullptr, *gMix = nullptr, *gOut = nullptr, *gDelta = nullptr;
    Stage st[kStages]; double sr = 44100; MbParams mbNow;
    juce::AudioBuffer<float> dry, det, aux, acc, dlt, tmp, inB, scB, dryD, dly; int dlyW = 0, latency = 0, osMax = 512;
    static constexpr int kOsStages = 3, kOs = 8;
    std::unique_ptr<juce::dsp::Oversampling<float>> os, osSc;
    juce::SmoothedValue<float> sMix, sOut;
    void processChunk (float* L, float* R, const float* eL, const float* eR, int N);
    void delayDry (const float* L, const float* R, float* oL, float* oR, int N);
    juce::dsp::IIR::Filter<float> hp[kStages][2];  // per stadio, per canale
    float lastHp = -1; std::atomic<bool> extSc { false };
    float colIn = 0, colOut = 0, colGr[kStages] {}; int colN = 0; float rmsIn = 0, rmsOut = 0;
};
}
