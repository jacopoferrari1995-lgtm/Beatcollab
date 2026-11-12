#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <atomic>
#include <memory>
#include <vector>
#include "KitGen.h"
#include "Synth.h"
#include "SampleScan.h"

namespace jp
{
constexpr int kPads = 16, kVoices = 48, PPQ = 96, kBar = PPQ * 4;

struct SampleData { juce::AudioBuffer<float> buf; double sr = 44100; juce::String name, path; };

// parametri di ogni pad (catena: sample -> filtro -> EQ -> comp -> drive -> lo-fi -> fader/pan -> mandate)
enum PadParam { GAIN, LEVEL, PAN, TUNE, FINE, START, END, ATTACK, DECAY, RELEASE, MODE, REVERSE, CHOKE, LOOP, WARP, SBPM,
                VLEVEL, VFILTER, VSTART, VPITCH,
                FTYPE, CUTOFF, RESO, FENV, FDECAY,
                EQON, B1ON, B1T, B1F, B1G, B1Q, B2ON, B2T, B2F, B2G, B2Q, B3ON, B3T, B3F, B3G, B3Q,
                B4ON, B4T, B4F, B4G, B4Q, B5ON, B5T, B5F, B5G, B5Q, B6ON, B6T, B6F, B6G, B6Q,
                CMODE, CAMT, CMAKEUP, CMIX,
                DRIVE, BITS, RATE, SREV, SDLY, MUTE, SOLO, SLICE, SKEY, DTYPE, LGAIN, LTUNE, LPAN, LVEL, LMODE, kNumPadParams,
                // nomi storici dell'EQ a 3 bande (kit e progetti vecchi): banda 2 = bassi, 3 = medi, 5 = alti
                LOWG = B2G, LOWF = B2F, MIDG = B3G, MIDF = B3F, MIDQ = B3Q, HIGHG = B5G, HIGHF = B5F };
// EQ a 6 bande per pad: parametro k (0 on, 1 tipo, 2 freq, 3 gain, 4 Q) della banda b
inline int bandP (int b, int k) { return B1ON + b * 5 + k; }
enum EqType { EQ_LC12, EQ_LC24, EQ_LSHELF, EQ_BELL, EQ_NOTCH, EQ_HSHELF, EQ_HC12, EQ_HC24, kEqTypes };
const char* eqTypeName (int t);
constexpr int kDriveTypes = 7;
const char* driveTypeName (int t);
const char* driveTypeDesc (int t);
float driveShape (int t, float x);
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
    void set (int type, double sr, double f, double gDb, double q);   // 0 low shelf, 1 peak, 2 high shelf, 3 passa basso, 4 passa alto, 5 notch, 6 passa banda
    void reset() { z1[0] = z1[1] = z2[0] = z2[1] = 0; }
    inline float run (float x, int c) { const float y = b0 * x + z1[c]; z1[c] = b1 * x - a1 * y + z2[c]; z2[c] = b2 * x - a2 * y; return y; }
    static float magDb (int type, double sr, double f, double gDb, double q, double atHz);
};

// kit di fabbrica
struct KitPad { DrumDesc d; std::vector<std::pair<int, float>> p; int colour; };
struct FactoryKit { const char* name; const char* style; std::vector<KitPad> pads; };
const std::vector<FactoryKit>& factoryKits();

// parametri dei pad visibili ad Ableton (automazione): 12 per pad
constexpr int kAutoN = 12;
extern const int kAutoParams[kAutoN];
int autoIndex (int padParam);   // -1 se non e automatizzabile

class Processor : public juce::AudioProcessor, private juce::Timer
{
public:
    Processor();
    ~Processor() override { stopTimer(); }
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
    juce::RangedAudioParameter* hostParam (int p, int i) const { const int a = autoIndex (i); return a < 0 ? nullptr : hp[p][a]; }
    void setPadValue (int p, int i, float v);
    std::atomic<int> eqSoloPad { -1 }, eqSoloBand { -1 };   // ascolto di una sola banda dell'EQ di un pad   // dall'interfaccia: aggiorna anche il parametro di Ableton
    SamplePtr getSample (int p) const { const juce::SpinLock::ScopedLockType l (sLock); return smp[p]; }
    void setSample (int p, SamplePtr s) { const juce::SpinLock::ScopedLockType l (sLock); smp[p] = std::move (s); }
    SamplePtr getLayer (int p) const { const juce::SpinLock::ScopedLockType l (sLock); return smp2[p]; }
    void setLayer (int p, SamplePtr s) { const juce::SpinLock::ScopedLockType l (sLock); smp2[p] = std::move (s); }
    bool loadLayerFile (const juce::File&, int p);
    bool loadFile (const juce::File&, int p, bool analyse = true);
    void detectKey (int p, bool audio = true);    // tonalita del campione (nome, poi ascolto)
    void transposeTo (int p, int key);
    int keyLock() const { return (int) apvts.getRawParameterValue ("kkey")->load() - 1; }   // -1 libera, 0..23 tonalita fissa
    bool isTonalPad (int p) const;
    void applyKeyLock (int p);                    // se la tonalita e fissata porta il pad li
    void applyKeyLockAll();
    int kitEffectiveKey (int except = -1) const;  // tonalita in cui suonano le parti tonali ora            // porta il campione nella tonalita scelta (stretch se e un loop)
    // ascolto di prova dalla banca, senza caricarlo sul pad
    void preview (SamplePtr s) { { const juce::SpinLock::ScopedLockType l (sLock); prevNext = std::move (s); } prevReq = prevReq.load() + 1; }
    // annulla / ripeti: istantanee dei pad prese dall'editor quando cambia qualcosa
    struct Snap { float v[kPads][kNumPadParams]; int col[kPads]; SamplePtr s[kPads], s2[kPads]; juce::String kit; };
    Snap capture() const;
    void restore (const Snap&);
    static bool same (const Snap&, const Snap&);
    std::vector<Snap> undoStack, redoStack; Snap committed; bool hasCommitted = false;
    void commitIfChanged();
    bool undo();
    bool redo();
    void chopToPads (int src, const std::vector<float>& points, int firstPad = -1);   // punti normalizzati, inizio e fine inclusi
    void assignSlice (int src, float s0, float s1, int dst, int sliceNo);
    void audition (int p, float s0, float s1) { audP = p; audS0 = s0; audS1 = s1; audReq = audReq.load() + 1; }
    void stopAll() { stopReq = true; }
    static float estimateBpmAudio (const juce::AudioBuffer<float>& b, double sampleRate, double& strength);
    double hostBpm() const { return curBpm; }
    void clearPad (int p) { setSample (p, nullptr); setLayer (p, nullptr); padCat[p] = {}; }
    juce::String padCat[kPads];               // categoria della banca da cui arriva il suono (serve al generatore)
    int genStyle = 0; juce::uint32 genSeed = 1; juce::String genInfo;
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
    // vista transienti: colonne a scorrimento (~3 s) con picco d'uscita e pad colpiti
    static constexpr int kScope = 480;
    std::atomic<float> scPk[kScope]; std::atomic<int> scHit[kScope], scVel[kScope]; std::atomic<int> scW { 0 };
    int rootNote() const { return (int) apvts.getRawParameterValue ("root")->load(); }
    // monitor: dove sta suonando ogni pad (0..1, -1 fermo), loudness e picco dell'uscita
    std::atomic<float> padPos[kPads];
    std::atomic<float> lufsS { -100 }, peakHold { 0 };
    // effetti da performance sulle 8 manopole (Q-LINK FX e manopole dell'MPK: CC 70-77)
    static constexpr int kFx = 8;
    static const char* const kFxIds[kFx];
    std::atomic<float> ccPend[kFx];        // valore (0..1) arrivato dal controller, da passare al parametro
    std::atomic<bool> qFx { false };       // le 8 manopole sotto lo schermo fanno da effetti
    std::atomic<int> ccMap[kFx];           // quale CC muove ogni effetto (di fabbrica 70-77, l'MPK); si cambia con MIDI learn
    std::atomic<int> learnSlot { -1 };     // in attesa di un CC per questo effetto
    float velCurve (int vel) const;
    juce::AudioFormatManager formats;
    juce::SharedResourcePointer<SampleIndex> sampleIndex;   // indice SCAN condiviso (la scansione continua anche a editor chiuso)

private:
    juce::RangedAudioParameter* hp[kPads][kAutoN] {}; std::atomic<float>* hpRaw[kPads][kAutoN] {}; std::atomic<float> lastHost[kPads][kAutoN];
    void timerCallback() override;
    float colPk = 0; int colN = 0, colHits = 0, colVel = 0;
    struct Grain { double start = 0, ph = 0; };
    struct Voice
    {
        bool active = false, rel = false, choking = false, loop = false; int pad = 0, age = 0, gate = -1, warp = 0;
        SamplePtr s; double pos = 0, inc = 1, a0 = 0, a1 = 0, tRate = 1, pRate = 1, G = 2048, sbpm = 0, pitch = 1; int dir = 1;
        float velSemis = 0, velDb = 0, velCut = 1, panOff = 0; bool layer = false;
        Grain gr[2];
        float gain = 1, env = 0, atkInc = 1, decCoef = 1, relCoef = 1, fEnv = 1, fEnvCoef = 1, cutBase = 20000, fenvAmt = 0, reso = 0.1f; bool atkDone = false, hold = false;
        float ic1[2] = {}, ic2[2] = {}, fg = 0, fk = 1; int ftype = 0, fCount = 0; bool filt = false;
    };
    struct PadChain
    {
        Biquad eq[6][2], sol[2]; float cEnv = 0; float holdL = 0, holdR = 0; double holdPh = 0; int active = 0;
        float lastBand[6][5]; float solKey[3] { -1, -1, -1 };
        float lastDrv = -1, drvNorm = 1; int lastDType = -1; float dHp[2] {}, dHpX[2] {}, dLp[2] {};
        PadChain() { for (auto& b : lastBand) for (auto& v : b) v = -999; }
    };
    Voice voices[kVoices];
    PadChain chain[kPads];
    SamplePtr smp[kPads], smp2[kPads];
    mutable juce::SpinLock sLock;
    double sr = 44100, curBpm = 120;
    std::atomic<int> audP { -1 }, audReq { 0 }; std::atomic<float> audS0 { 0 }, audS1 { 1 }; int audSeen = 0; std::atomic<bool> stopReq { false };
    SamplePtr prevNext, prevS; std::atomic<int> prevReq { 0 }; int prevSeen = 0; double prevPos = 0;
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
    juce::Reverb reverb; float satZ = 0;
    std::vector<float> preBuf; int preW = 0; float rvHpZ[2] {}, rvHpX[2] {}; Biquad rvLp; float dlHp[2] {}, dlHpX[2] {}, dlLp[2] {};
    juce::Random rng;
    float busEnv = 0;
    // effetti da performance
    juce::AudioBuffer<float> stBuf, stuBuf; int stW = 0, stuW = 0; double stR = 0; float stSpeed = 1, stX = 0; bool stOn = false;
    bool stuOn = false; double stuStart = 0, stuPos = 0; int stuLen = 0; float stuX = 0;
    juce::AudioBuffer<float> scBuf; int skW = 0, scIdle = 0; double scAnchor = 0; float scV = -1, scGrab = 0, scX = 0; bool scOn = false;
    float djIc1[2] {}, djIc2[2] {}, djFc = 1000, djMode = 0, fxG = 1; float lfHold[2] {}, lfLp[2] {}; int lfCnt = 0;
    void performFx (float* L, float* R, int N);
    // loudness
    Biquad kw1, kw2; double lufsAcc = 0; int lufsN = 0; float lufsBins[30] {}; int lufsBi = 0;
    void startVoice (int p, int vel, float r0 = -1, float r1 = -1);
    void launch (int p, int vel, float r0, float r1, const SamplePtr& s, bool layer);
    void releasePad (int p);
    void render (Voice& v, float* L, float* R, int i);

    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
};
}
