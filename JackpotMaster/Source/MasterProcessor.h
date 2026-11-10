#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "JackpotLook.h"
#include "MasterDSP.h"

namespace jms
{
// moduli della catena di master (in ordine). Fase 1: MAXIMIZER e LIMITER.
enum ModId { BASS, DYNEQ, DEESS, BUSCOMP, MULTI, TAPE, IMAGER, MAXIM, LIMIT, kNumMods };
struct ModInfo { const char* name; const char* sub; bool ready; juce::uint32 colour; };
inline const ModInfo& modInfo (int m)
{
    static const ModInfo I[kNumMods] = {
        { "BASS CONTROL", "sub e punch", false, 0xffffb000 }, { "EQ DINAMICO", "bande che reagiscono", false, 0xff5ef2ff }, { "DE-ESSER", "sibilanti e piatti", false, 0xff7dff5a },
        { "BUS COMP", "colla da console", false, 0xffa66bff }, { "MULTIBANDA", "dinamica + OTT", false, 0xffff7a3d }, { "TAPE", "nastro analogico", false, 0xffffd23d },
        { "IMAGER", "stereo per banda", false, 0xff3dffd2 }, { "MAXIMIZER", "volume e carattere", true, 0xffff2e88 }, { "LIMITER", "true peak finale", true, 0xffff3b5c } };
    return I[juce::jlimit (0, kNumMods - 1, m)];
}

struct Target { const char* name; float lufs; float tp; };
inline const Target& target (int i)
{
    static const Target T[] = { { "SPOTIFY", -14, -1 }, { "APPLE MUSIC", -16, -1 }, { "YOUTUBE", -14, -1 }, { "SOUNDCLOUD", -10, -1 }, { "CLUB", -8, -0.3f }, { "TRAP FORTE", -7, -0.3f } };
    return T[juce::jlimit (0, 5, i)];
}
constexpr int kNumTargets = 6;

struct Preset { const char* name; const char* note; std::vector<std::pair<const char*, float>> v; };
const std::vector<Preset>& presets();

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
    const juce::String getName() const override { return "Jackpot Master"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.05; }
    int getNumPrograms() override { return (int) presets().size(); }
    int getCurrentProgram() override { return preset; }
    void setCurrentProgram (int i) override { loadPreset (i); }
    const juce::String getProgramName (int i) override { return juce::isPositiveAndBelow (i, (int) presets().size()) ? presets()[(size_t) i].name : ""; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    void loadPreset (int i);
    float value (const char* id) const { return apvts.getRawParameterValue (id)->load(); }
    double sampleRate() const { return sr; }

    juce::AudioProcessorValueTreeState apvts;
    jlook::ABCompare ab;
    int preset = 0;

    // misure per l'interfaccia
    std::atomic<float> lufsM { -100 }, lufsS { -100 }, lufsI { -100 }, lra { 0 }, tpMax { -100 }, inS { -100 }, matchDb { 0 }, normDb { 0 };
    std::atomic<float> grMax { 0 }, grLim { 0 }, inPk[2] {}, outPk[2] {};
    std::atomic<bool> resetReq { false };
    static constexpr int kHist = 480;
    std::atomic<float> hIn[kHist], hOut[kHist], hGrM[kHist], hGrL[kHist], hS[kHist]; std::atomic<int> hW { 0 };

private:
    void timerCallback() override;
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    double sr = 44100;
    Maximizer mx; Limiter lim; Dither dith; Loudness loudIn, loudOut; TruePeak tpOut[2];
    Delay mxBy[2], limBy[2], dry[2], mxIn[2], deltaPad[2];
    juce::AudioBuffer<float> tmp, keep;
    float matchSm = 1, normSm = 1, colIn = 0, colOut = 0, colM = 0, colL = 0; int colN = 0;
    float tpHold = -100;
};
}
