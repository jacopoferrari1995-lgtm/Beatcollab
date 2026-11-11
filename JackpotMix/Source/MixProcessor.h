#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "MixModules.h"
#include "MixEq.h"
#include "JackpotLook.h"
#include "MasterDSP.h"

namespace jmx
{
// target per strumento: dove dovrebbe stare il livello (gain staging in mix o master)
struct Target { const char* name; float lufsLo, lufsHi, peak; const char* note; };
const std::vector<Target>& targets();

struct MixPreset { juce::String cat, name, note; int target = 0; std::vector<std::pair<juce::String, float>> v; };
const std::vector<MixPreset>& factoryPresets();

struct Ring
{
    static constexpr int kSize = 8192;
    float l[kSize] {}, r[kSize] {}; std::atomic<int> w { 0 };
    void push (const float* L, const float* R, int n) { int p = w.load(); for (int i = 0; i < n; ++i) { l[p] = L[i]; r[p] = R[i]; p = (p + 1) & (kSize - 1); } w = p; }
    void read (float* outL, float* outR, int n) const { int p = (w.load() - n) & (kSize - 1); for (int i = 0; i < n; ++i) { outL[i] = l[p]; if (outR) outR[i] = r[p]; p = (p + 1) & (kSize - 1); } }
};
// ritardo a lunghezza variabile senza allocare nel thread audio
struct VDelay
{
    std::vector<float> b[2]; int w = 0, mask = 0;
    void prepare (int maxLen) { int n = 1; while (n < maxLen + 2) n <<= 1; for (auto& x : b) x.assign ((size_t) n, 0.0f); mask = n - 1; w = 0; }
    inline void run (float& l, float& r, int d)
    {
        b[0][(size_t) w] = l; b[1][(size_t) w] = r;
        const int p = (w - juce::jlimit (0, mask, d)) & mask; l = b[0][(size_t) p]; r = b[1][(size_t) p]; w = (w + 1) & mask;
    }
};

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
    const juce::String getName() const override { return "Jackpot Mixology"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }
    int getNumPrograms() override { return (int) factoryPresets().size(); }
    int getCurrentProgram() override { return preset; }
    void setCurrentProgram (int i) override { loadPreset (i); }
    const juce::String getProgramName (int i) override { return juce::isPositiveAndBelow (i, (int) factoryPresets().size()) ? factoryPresets()[(size_t) i].cat + " | " + factoryPresets()[(size_t) i].name : ""; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // rack
    int chainLen() const { return juce::jlimit (0, kSlots, (int) value ("n")); }
    int slotType (int s) const { return juce::jlimit (0, (int) mods().size() - 1, (int) valueS (sid (s, "type"))); }
    void insertModule (int pos, int type);   // aggiunge (pos = chainLen() in fondo)
    void setModule (int s, int type);        // sostituisce
    void removeSlot (int s);
    void moveSlot (int from, int to);
    void copySlot (int s); bool pasteSlot (int s); bool hasClip() const { return clip.type > 0; }
    void loadPreset (int i);
    void loadUserPreset (const juce::File& f);
    void saveUserPreset (const juce::String& name);
    static juce::File userPresetDir();
    void setUserIr (int slot, const juce::File& f);
    juce::String userIrName (int slot) const { return irPath[slot].isEmpty() ? juce::String() : juce::File (irPath[slot]).getFileNameWithoutExtension(); }

    static juce::String sid (int s, const char* what) { return "s" + juce::String (s + 1) + "_" + what; }
    static juce::String bid (int b, const char* what) { return "e" + juce::String (b + 1) + "_" + what; }
    float value (const char* id) const { return apvts.getRawParameterValue (id)->load(); }
    float valueS (const juce::String& id) const { return apvts.getRawParameterValue (id)->load(); }
    void setP (const juce::String& id, float real) { if (auto* p = apvts.getParameter (id)) p->setValueNotifyingHost (p->convertTo0to1 (real)); }
    EqSet eqSettings() const;
    double sampleRate() const { return sr; }
    Engine* engine (int s) { return eng[s].get(); }
    void forceRebuild() { timerCallback(); }

    juce::AudioProcessorValueTreeState apvts;
    jlook::ABCompare ab;
    int preset = 0; juce::String presetName { "Init" };
    ProEq eq;
    Ring pre, post;           // spettro prima/dopo e imager
    // meter (uscita)
    std::atomic<float> lufsM { -100 }, lufsS { -100 }, lufsI { -100 }, tp { -100 }, tpHold { -100 }, pk { -100 }, corr { 1 }, width { 0 }, inS { -100 };
    std::atomic<bool> resetMeter { false };
    std::atomic<bool> scConnected { false };
private:
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    void timerCallback() override;
    void rebuild (int s);
    void moveRaw (int from, int to);
    double sr = 44100; int maxBlock = 512, lat = -1;
    std::shared_ptr<Engine> eng[kSlots]; juce::SpinLock engLock[kSlots];
    VDelay dryD[kSlots], cmpD; std::vector<float> dryL, dryR, scL, scR;
    jms::Loudness loudOut, loudIn; jms::TruePeak tpc[2];
    float cmpG = 1;
    struct Clip { int type = 0; float v[kP + 1] {}; } clip;
    juce::String irPath[kSlots];
    std::atomic<float>* sp[kSlots][kP + 3] {};
};
}
