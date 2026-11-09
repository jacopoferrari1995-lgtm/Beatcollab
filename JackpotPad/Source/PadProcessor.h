#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <atomic>
#include <memory>
#include <vector>
#include "KitGen.h"
#include "Synth.h"

namespace jp
{
constexpr int kPads = 16, kVoices = 48, PPQ = 96, kBar = PPQ * 4;

struct SampleData { juce::AudioBuffer<float> buf; double sr = 44100; juce::String name, path; };

// parametri di ogni pad (catena: sample -> filtro -> EQ -> comp -> drive -> lo-fi -> fader/pan -> mandate)
enum PadParam { GAIN, LEVEL, PAN, TUNE, FINE, START, END, ATTACK, DECAY, RELEASE, MODE, REVERSE, CHOKE, LOOP, WARP, SBPM,
                VLEVEL, VFILTER, VSTART, VPITCH,
                FTYPE, CUTOFF, RESO, FENV, FDECAY,
                EQON, LOWG, LOWF, MIDG, MIDF, MIDQ, HIGHG, HIGHF,
                CMODE, CAMT, CMAKEUP, CMIX,
                DRIVE, BITS, RATE, SREV, SDLY, MUTE, SOLO, SLICE, kNumPadParams };
struct PInfo { const char* id; const char* label; float min, max, def; int kind; };
// kind: 0 %, 1 ±, 2 st, 3 ct, 4 ms, 5 Hz, 6 scelta, 7 int, 8 on/off, 9 pan, 10 bpm, 11 dB, 12 Q, 13 fader
extern const PInfo kPInfo[kNumPadParams];
juce::String formatPadParam (int i, float v);
int choiceCount (int i);

struct Pad
{
    std::atomic<float> v[kNumPadParams];
    std::atomic<int> colour { 0 };
    Pad() { reset(); }
    void reset() { for (int i = 0; i < kNumPadParams; ++i) v[i] = kPInfo[i].def; }
    float get (int i) const { return v[i].load(); }
};

struct PatEv { double tick; int pad; int vel; };
struct PatternDef { const char* genre; const char* name; std::vector<std::pair<int, const char*>> lanes; };
const std::vector<PatternDef>& patternBank();
juce::StringArray patternGenres();

// biquad RBJ
struct Biquad
{
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1[2] = {}, z2[2] = {};
    void set (int type, double sr, double f, double gDb, double q);   // 0 low shelf, 1 peak, 2 high shelf
    inline float run (float x, int c) { const float y = b0 * x + z1[c]; z1[c] = b1 * x - a1 * y + z2[c]; z2[c] = b2 * x - a2 * y; return y; }
    static float magDb (int type, double sr, double f, double gDb, double q, double atHz);
};

// kit di fabbrica
struct KitPad { DrumDesc d; std::vector<std::pair<int, float>> p; int colour; };
struct FactoryKit { const char* name; const char* style; std::vector<KitPad> pads; };
const std::vector<FactoryKit>& factoryKits();

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
    const juce::String getName() const override { return "Jackpot Sampler"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    double getTailLengthSeconds() const override { return 3.0; }
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
    void chopToPads (int src, const std::vector<float>& points);   // punti normalizzati, inizio e fine inclusi
    void clearPad (int p) { setSample (p, nullptr); }
    void detectBpm (int p);
    static float estimateBpm (const juce::String& name, double seconds, bool& confident);

    // kit
    juce::String kitName { "Boom Bap Classico" };
    juce::File kitFile;                       // vuoto = kit di fabbrica
    static juce::File kitFolder();
    static juce::File baseFolder();
    juce::Array<juce::File> userKits() const;
    void loadFactoryKit (int i);
    bool saveKit (const juce::File&, const juce::String& name);
    bool loadKit (const juce::File&);
    std::unique_ptr<juce::XmlElement> padsToXml() const;
    void padsFromXml (const juce::XmlElement&);

    // interfaccia -> audio
    void uiNoteOn (int p, int vel);
    void uiNoteOff (int p);
    // loop dalla banca e swing
    juce::SpinLock recLock;
    std::vector<PatEv> events;
    std::atomic<bool> intPlay { false }, loopOn { false };
    int patternIndex = 0;
    void requestPlay (bool start) { transportReq = start ? 1 : 2; }
    void loadPattern (int index);
    void randomPattern (const juce::String& genre);
    void varyPattern();
    double swingTick (double tick) const;      // posizione con swing applicato
    juce::File writeMidi (bool withSwing);
    std::atomic<int> patternBars { 2 };
    int loopTicks() const { return patternBars.load() * kBar; }

    // per la UI
    std::atomic<float> padGlow[kPads], padMeter[kPads], padGr[kPads];
    std::atomic<int> lastVel[kPads];
    std::atomic<juce::uint32> hits[kPads];
    std::atomic<double> uiTick { 0 }, uiBpm { 120 };
    std::atomic<bool> uiRunning { false }, uiHost { false };
    std::atomic<float> meterL { 0 }, meterR { 0 }, busGr { 0 };
    int rootNote() const { return (int) apvts.getRawParameterValue ("root")->load(); }
    float velCurve (int vel) const;
    juce::AudioFormatManager formats;

private:
    struct Grain { double start = 0, ph = 0; };
    struct Voice
    {
        bool active = false, rel = false, choking = false, loop = false; int pad = 0, age = 0, gate = -1, warp = 0;
        SamplePtr s; double pos = 0, inc = 1, a0 = 0, a1 = 0, tRate = 1, pRate = 1, G = 2048; int dir = 1;
        Grain gr[2];
        float gain = 1, env = 0, atkInc = 1, decCoef = 1, relCoef = 1, fEnv = 1, fEnvCoef = 1, cutBase = 20000, fenvAmt = 0, reso = 0.1f; bool atkDone = false, hold = false;
        float ic1[2] = {}, ic2[2] = {}, fg = 0, fk = 1; int ftype = 0, fCount = 0; bool filt = false;
    };
    struct PadChain
    {
        Biquad eq[3]; float cEnv = 0; float holdL = 0, holdR = 0; double holdPh = 0; int active = 0;
        float lastEq[7] = { 99, 99, 99, 99, 99, 99, 99 };
    };
    Voice voices[kVoices];
    PadChain chain[kPads];
    SamplePtr smp[kPads];
    mutable juce::SpinLock sLock;
    double sr = 44100, curBpm = 120;
    struct UiEv { int pad, vel; bool on; };
    juce::AbstractFifo fifo { 512 }; UiEv fifoBuf[512];
    std::atomic<int> transportReq { 0 };
    double intTick = 0; bool wasRunning = false;
    struct Pend { juce::int64 at; int pad, vel; bool on; };
    std::vector<Pend> pend; juce::int64 absSample = 0; juce::int64 padDelay[kPads] = {};
    int heldVel[kPads] = {}; double nrStart[kPads] = {}; double nrPhase[kPads] = {};
    struct Trig { int off, pad, vel; bool on; };
    std::vector<Trig> trigs;
    juce::AudioBuffer<float> padBuf;
    std::vector<float> revL, revR, dlyInL, dlyInR;
    juce::AudioBuffer<float> dlyBuf; int dlyW = 0; float dlyLpL = 0, dlyLpR = 0;
    juce::Reverb reverb;
    juce::Random rng;
    float busEnv = 0;
    void startVoice (int p, int vel);
    void releasePad (int p);
    void render (Voice& v, float* L, float* R, int i);

    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
};
}
