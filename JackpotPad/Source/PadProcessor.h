#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <atomic>
#include <memory>
#include <vector>
#include "KitGen.h"

namespace jp
{
constexpr int kPads = 16, kVoices = 48, PPQ = 96, kBar = PPQ * 4;

struct SampleData { juce::AudioBuffer<float> buf; double sr = 44100; juce::String name, path; };
using SamplePtr = std::shared_ptr<SampleData>;

// parametri di ogni pad
enum PadParam { LEVEL, PAN, TUNE, FINE, START, END, ATTACK, DECAY, RELEASE, FTYPE, CUTOFF, RESO,
                VLEVEL, VFILTER, VSTART, VPITCH, CHOKE, MODE, REVERSE, kNumPadParams };
struct PInfo { const char* id; const char* label; float min, max, def; int kind; };  // kind: 0 %, 1 ±%, 2 st, 3 ct, 4 ms, 5 Hz, 6 scelta, 7 int, 8 bool, 9 pan
extern const PInfo kPInfo[kNumPadParams];
juce::String formatPadParam (int i, float v);

struct Pad
{
    std::atomic<float> v[kNumPadParams];
    std::atomic<int> colour { 0 };
    Pad() { reset(); }
    void reset() { for (int i = 0; i < kNumPadParams; ++i) v[i] = kPInfo[i].def; }
    float get (int i) const { return v[i].load(); }
};

struct RecEv { double tick; int pad; int vel; double recAbs; };

class Processor : public juce::AudioProcessor
{
public:
    Processor();
    void prepareToPlay (double, int) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& l) const override { return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo(); }
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Jackpot Pad"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;
    Pad pad[kPads];
    SamplePtr getSample (int p) const { const juce::SpinLock::ScopedLockType l (sLock); return smp[p]; }
    void setSample (int p, SamplePtr s) { const juce::SpinLock::ScopedLockType l (sLock); smp[p] = std::move (s); }
    bool loadFile (const juce::File&, int p);
    void clearPad (int p) { setSample (p, nullptr); }
    void loadDefaultKit();

    // interfaccia -> audio
    void uiNoteOn (int p, int vel);
    void uiNoteOff (int p);
    // registratore
    juce::SpinLock recLock;
    std::vector<RecEv> events;
    std::atomic<bool> recording { false }, intPlay { false }, erase { false };
    void requestPlay (bool start) { transportReq = start ? 1 : 2; }
    void clearPattern() { const juce::SpinLock::ScopedLockType l (recLock); events.clear(); }
    void erasePad (int p);
    double quantize (double tick) const;       // posizione quantizzata (non distruttiva)
    juce::File writeMidi();
    int loopTicks() const;

    // per la UI
    std::atomic<float> padGlow[kPads];
    std::atomic<int> lastVel[kPads];
    std::atomic<juce::uint32> hits[kPads];
    std::atomic<double> uiTick { 0 }, uiBpm { 120 };
    std::atomic<bool> uiRunning { false }, uiHost { false };
    std::atomic<float> meterL { 0 }, meterR { 0 };
    std::atomic<int> lastPlayed { 0 };
    int rootNote() const { return (int) apvts.getRawParameterValue ("root")->load(); }
    float velCurve (int vel) const;
    juce::AudioFormatManager formats;

private:
    struct Voice
    {
        bool active = false, rel = false, choking = false; int pad = 0, age = 0, gate = -1;
        SamplePtr s; double pos = 0, inc = 1, a0 = 0, a1 = 0; int dir = 1;
        float gL = 1, gR = 1, env = 0, atkInc = 1, decCoef = 1, relCoef = 1; bool atkDone = false, hold = false;
        float ic1[2] = {}, ic2[2] = {}, fg = 0, fk = 1; int ftype = 0; bool filt = false;
    };
    Voice voices[kVoices];
    SamplePtr smp[kPads];
    mutable juce::SpinLock sLock;
    double sr = 44100;
    struct UiEv { int pad, vel; bool on; };
    juce::AbstractFifo fifo { 512 }; UiEv fifoBuf[512];
    std::atomic<int> transportReq { 0 };
    double intTick = 0; bool wasRunning = false;
    float clickPh = 0, clickEnv = 0, clickF = 1500;
    struct Trig { int off, pad, vel; bool on; };
    std::vector<Trig> trigs;
    void startVoice (int p, int vel);
    void releasePad (int p);
    void render (Voice& v, float& L, float& R);
    void record (int pad, int vel, double tick, double absTick);
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
};
}
