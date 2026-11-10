#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "JackpotLook.h"
#include "PitchEngine.h"
#include "DriveModels.h"

namespace ja
{
struct Preset { const char* name; const char* note; std::vector<std::pair<const char*, float>> v; };
const std::vector<Preset>& presets();

enum PitchMode { TRASPONI, QUANTIZZA, ROBOT };
enum DriftType { WOW, VIBRATO, CAOS };

// anello senza lock per mandare l'audio all'interfaccia (analizzatore)
struct Ring
{
    static constexpr int kSize = 8192;
    float d[kSize] {}; std::atomic<int> w { 0 };
    void push (const float* x, int n) { int p = w.load(); for (int i = 0; i < n; ++i) { d[p] = x[i]; p = (p + 1) & (kSize - 1); } w = p; }
    void read (float* out, int n) const { int p = (w.load() - n) & (kSize - 1); for (int i = 0; i < n; ++i) { out[i] = d[p]; p = (p + 1) & (kSize - 1); } }
};

struct Settings
{
    int mode = TRASPONI, key = 0, scale = 0, driftType = WOW, model = TAPE, order = 0;
    float pitch = 0, formant = 0, drift = 0, driftRate = 1, drive = 0, tone = 0, fLo = 20, fHi = 20000, mix = 1, outDb = 0;
    bool link = false, focus = false, listen = false, autoGain = true;
};

// ---------------------------------------------------------------- stadio PITCH
class PitchStage
{
public:
    void prepare (double sr, int maxBlock);
    void process (float* L, float* R, int n, const Settings&);
    int latency() const { return N; }
    std::atomic<float> inHz { 0 }, outHz { 0 }, driftSemi { 0 }, corrSemi { 0 }; std::atomic<bool> active { false };
private:
    void frame (const Settings&);
    double sr = 44100; int N = 2048, H = 256, hopPos = 0;
    PhaseVocoder pv[2]; PitchDetector det;
    std::vector<float> dly[2]; int dPos = 0;          // percorso pulito (stessa latenza) quando il pitch e fermo
    float wet = 0, corr = 0, lastF0 = 0, dPhase = 0, dA = 0, dB = 0, dT = 0; int unvoiced = 0;
    juce::Random rng;
    float ratioNow = 1, fRatioNow = 1; bool keepNow = true;
};

// ---------------------------------------------------------------- stadio DRIVE con banda FOCUS
class DriveStage
{
public:
    void prepare (double sr, int maxBlock);
    void process (float* L, float* R, int n, const Settings&);
    int latency() const { return osLat; }
    std::atomic<float> gainRed { 0 }, autoDb { 0 };
private:
    double sr = 44100; int osLat = 0, maxB = 512;
    std::unique_ptr<juce::dsp::Oversampling<float>> os;
    DriveChannel ch[2];
    juce::dsp::LinkwitzRileyFilter<float> xLo, xHi;
    juce::AudioBuffer<float> band, other;
    std::vector<float> odl[2]; int oPos = 0;
    BQ toneLo[2], toneHi[2]; float lastTone = 99;
    float eIn = 1e-6f, eOut = 1e-6f, agSm = 1;
};


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
    const juce::String getName() const override { return "Jackpot Alter Drive"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.1; }
    int getNumPrograms() override { return (int) presets().size(); }
    int getCurrentProgram() override { return preset; }
    void setCurrentProgram (int i) override { loadPreset (i); }
    const juce::String getProgramName (int i) override { return juce::isPositiveAndBelow (i, (int) presets().size()) ? presets()[(size_t) i].name : ""; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    void loadPreset (int i);
    float value (const char* id) const { return apvts.getRawParameterValue (id)->load(); }
    Settings settings() const;

    juce::AudioProcessorValueTreeState apvts;
    jlook::ABCompare ab;
    int preset = 0;
    PitchStage pitch; DriveStage drive;
    Ring ringIn, ringOut;
    std::atomic<float> inPk[2] {}, outPk[2] {};
    double sampleRate() const { return sr; }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    double sr = 44100;
    std::vector<float> dry[2], dryL, dryR, tmpR; int dryPos = 0, dryLen = 1, totalLat = 0;
    float mono[4096];
};
}
