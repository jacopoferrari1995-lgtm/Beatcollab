#pragma once
#include "Shared.h"
#include "KitGen.h"
#include "Burro.h"

namespace j60
{
struct Voice
{
    bool active = false, releasing = false, choking = false;
    int pad = -1, note = -1; uint64_t age = 0;
    SamplePtr smp;
    double pos = 0, inc = 1, startF = 0, endF = 0; int dir = 1;
    float gL = 1, gR = 1, env = 0, attackInc = 1, decayCoef = 1, relCoef = 0.999f;
    int attackDone = 0; bool sustain = true; int gate = -1; bool loop = false;
    float sDly = 0, sRev = 0;
    // filtro SVF
    float ic1L = 0, ic2L = 0, ic1R = 0, ic2R = 0, fg = 1, fk = 1; bool filt = false;
};

class Processor : public juce::AudioProcessor
{
public:
    Processor();
    ~Processor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& l) const override { return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo(); }
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Jackpot 60"; }
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

    // ---- dati condivisi con l'interfaccia ----
    juce::AudioProcessorValueTreeState apvts;
    PadParams pad[kPads];
    juce::SpinLock padLock;                         // protegge sample[]
    SamplePtr sample[kPads];
    SamplePtr getSample (int p) { const juce::SpinLock::ScopedLockType l (padLock); return sample[p]; }
    void setSample (int p, SamplePtr s) { const juce::SpinLock::ScopedLockType l (padLock); sample[p] = std::move (s); }

    juce::SpinLock seqLock;                         // protegge seq[]
    Sequence seq[kSeqs];
    std::atomic<int> curSeq { 0 };

    std::atomic<bool> intPlaying { false }, recording { false }, noteRepeat { false }, fullLevel { false }, erase { false }, syncHost { true };
    std::atomic<double> uiTick { 0.0 }, uiBpm { 120.0 };
    std::atomic<bool> uiRunning { false };
    std::atomic<uint32_t> hitCount[kPads];
    std::atomic<int> heldVel[kPads];

    // eventi dai pad dell'interfaccia (thread audio li legge)
    void uiPadDown (int p, int vel);
    void uiPadUp (int p);
    void requestStop();
    void requestPlayStart();
    void requestPlay();

    // utilità per l'interfaccia
    bool loadFileToPad (const juce::File& f, int p);
    void chopPad (int src, int slices, bool transient);
    void clearPad (int p);
    juce::File writeMidiFile();
    static int tcTicks (int choice);
    static int repeatTicks (int choice);
    juce::AudioFormatManager formats;
    static float estimateBpm (const juce::String& name, double seconds, bool& confident);

    // resample: 0 fermo, 1 in attesa della battuta, 2 registra, 3 pronto
    std::atomic<int> rsState { 0 };
    std::atomic<int> rsLen { 0 };
    juce::AudioBuffer<float> rsBuf;
    double rsBpm = 120.0;
    float sendDL = 0, sendDR = 0, sendRL = 0, sendRR = 0;
    std::vector<float> revL, revR;
    void toggleResample();
    Burro burro;
    SamplePtr takeResample();
    double getSampleRateNow() const { return sr; }

private:
    struct UiEv { int pad, vel; bool on; };
    juce::AbstractFifo fifo { 256 };
    UiEv fifoBuf[256];
    std::atomic<int> transportReq { 0 }; // 1 stop, 2 play start, 3 play

    Voice voices[kVoices];
    uint64_t voiceAge = 0;
    double sr = 44100.0, intTick = 0.0, lastTick = -1.0;
    double repeatClock = 0.0;
    bool wasRunning = false;
    // vintage
    double vPhase = 1.0; float vHoldL = 0, vHoldR = 0, toneZL = 0, toneZR = 0, charZL = 0, charZR = 0;
    float clickPh = 0, clickEnv = 0;
    juce::Random rnd;
    double curBpm = 120.0;
    int rsTarget = 0, rsWaitOffset = -1;
    // effetti
    juce::AudioBuffer<float> dlyBuf; int dlyW = 0; float dlyLpL = 0, dlyLpR = 0;
    juce::Reverb reverb;

    struct Trig { int offset; int pad; int vel; bool on; int gate = 0; };
    std::vector<Trig> trigs;

    void startVoice (int p, int vel, int gateSamples);
    double recAbs[kPads];
    void releasePad (int p);
    void renderVoice (Voice& v, float& L, float& R);
    double swingTick (int tick, float swing) const;
    void recordHit (int p, int vel, double tick);
    void loadDefaultKit();
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
};
}
