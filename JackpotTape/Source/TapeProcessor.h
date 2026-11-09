#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <cstring>

namespace jt
{
enum Fx { STOP, START, SPIN, HALF, REV, STUT, LOOP, GLITCH, GATE, kNumFx };
constexpr int kMaxSteps = 128;   // 8 battute in sedicesimi
constexpr int kCols = 512;       // risoluzione della traccia nel display

struct FxInfo { const char* id; const char* name; const char* desc; juce::uint32 colour; char key; };
extern const FxInfo kFx[kNumFx];

// tabelle a passi
struct Step { double v; const char* label; };
extern const Step kDur[6], kSize[6], kRate[5];

struct PSpec { const char* id; const char* label; int fx; float min, max, def; int table; int fmt; };
// table: 0 continuo, 1 durata, 2 misura, 3 rate del gate   fmt: 0 %, 1 ±, 2 x, 3 ms, 4 st, 5 forza
extern const PSpec kParams[];
extern const int kNumParams;

struct Preset;

class Processor : public juce::AudioProcessor
{
public:
    Processor();
    void prepareToPlay (double, int) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& l) const override
    { return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo() && l.getMainInputChannelSet() == juce::AudioChannelSet::stereo(); }
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Jackpot Tape"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.5; }
    int getNumPrograms() override;
    int getCurrentProgram() override { return presetIndex; }
    void setCurrentProgram (int) override;
    const juce::String getProgramName (int) override;
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<bool> seq[kNumFx][kMaxSteps];
    std::atomic<bool> uiPad[kNumFx];           // pad tenuti dall'interfaccia o dalla tastiera
    int presetIndex = 0;
    void loadPreset (int i);
    void applyAge (float a);
    float value (const char* id) const { return apvts.getRawParameterValue (id)->load(); }
    float fxValue (int fx, const char* p) const;

    // per il display
    std::atomic<float> dispWet[kCols], dispDry[kCols], dispSpd[kCols];
    std::atomic<int> dispMask[kCols];
    std::atomic<int> activeMask { 0 }, uiStep { -1 };
    std::atomic<double> uiBpm { 140.0 }, uiBeat { 0.0 };
    std::atomic<bool> uiHost { false };
    std::atomic<float> meterL { 0 }, meterR { 0 }, uiSpeed { 1 };

private:
    std::atomic<float>* paramPtr[64] = {};
    std::atomic<float>* padPtr[kNumFx] = {};
    juce::AudioBuffer<float> hist; int HL = 1 << 21, HM = (1 << 21) - 1;
    juce::int64 T = 0;               // campioni scritti
    double R = 0;                     // testina di lettura (assoluta)
    bool live = true;
    double sr = 44100;
    double freeBeat = 0;              // orologio interno quando l'host è fermo
    struct FxState { bool on = false, pad = false, seqOn = false, prevParam = false; juce::int64 shot = 0, t0 = 0, pendAt = -1; bool pendOn = false; double d[6] = {}; int k = 0; int op = 0; };
    FxState F[kNumFx];
    struct Xf { bool on = false; double from = 0, spd = 1; int n = 0, N = 1; bool fromLive = false; } xf;
    double halfF = 1, ph1 = 0, ph2 = 0, drift = 0, dtgt = 0, gateG = 1, dropG = 1; int dropT = 0;
    int lastStep = -1, lastCol = -1;
    juce::Random rng;
    // catena del nastro
    juce::dsp::IIR::Filter<float> bumpL, bumpR;
    float lpIcL[2] = {}, lpIcR[2] = {}, lpZL = 0, lpZR = 0, cutoffSm = 20000, dgSm = 1, outSm = 1;
    float speedAcc = 0; int speedN = 0;

    float rd (double pos, int ch) const;
    void activate (int fx, double bf, double beatAtT, juce::int64 t);
    double beatOf (juce::int64 t, juce::int64 t0, double beat0, double bf) const { return beat0 + (double) (t - t0) / bf; }
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
};
}
