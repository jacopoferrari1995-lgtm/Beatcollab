#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "JackpotLook.h"
#include "MasterColor.h"
#include "MasterAnalysis.h"
#include "MasterEasy.h"
#include "JkProEq.h"

namespace jms
{
// moduli della catena di master (in ordine). Fase 1: MAXIMIZER e LIMITER.
enum ModId { BASS, DYNEQ, DEESS, BUSCOMP, MULTI, EXCITER, TAPE, IMAGER, MAXIM, LIMIT, EQ432, PULTEC, VARIMU, kNumMods };   // EQ432 aggiunto in fondo (i progetti salvati restano giusti)
struct ModInfo { const char* name; const char* sub; bool ready; juce::uint32 colour; };
inline const ModInfo& modInfo (int m)
{
    static const ModInfo I[kNumMods] = {
        { "BASS CONTROL", "sub e punch", true, 0xffffb000 }, { "EQ DINAMICO", "bande che reagiscono", true, 0xff5ef2ff }, { "DE-ESSER", "sibilanti e piatti", true, 0xff7dff5a },
        { "BUS COMP", "colla da console", true, 0xffa66bff }, { "MULTIBANDA", "dinamica + OTT", true, 0xffff7a3d },
        { "EXCITER", "armoniche per banda", true, 0xffff5ea8 }, { "TAPE", "nastro analogico", true, 0xffffd23d },
        { "IMAGER", "stereo per banda", true, 0xff3dffd2 }, { "MAXIMIZER", "volume e carattere", true, 0xffff2e88 }, { "LIMITER", "true peak finale", true, 0xffff3b5c },
        { "EQ 432", "eq classico da mastering", true, 0xffffd27a }, { "PULTEC", "eq passivo a valvole", true, 0xffe8c38a }, { "VARI-MU", "compressore a valvole", true, 0xffc98bff } };
    return I[juce::jlimit (0, kNumMods - 1, m)];
}

struct Target { const char* name; float lufs; float tp; };
inline const Target& target (int i)
{
    static const Target T[] = { { "SPOTIFY", -14, -1 }, { "APPLE MUSIC", -16, -1 }, { "YOUTUBE", -14, -1 }, { "SOUNDCLOUD", -10, -1 }, { "CLUB", -8, -0.3f }, { "TRAP FORTE", -7, -0.3f } };
    return T[juce::jlimit (0, 5, i)];
}
constexpr int kNumTargets = 6;

// anello stereo senza lock per l'interfaccia
struct StereoRing
{
    static constexpr int kSize = 8192;
    float l[kSize] {}, r[kSize] {}; std::atomic<int> w { 0 };
    void push (const float* a, const float* b, int n) { int p = w.load(); for (int i = 0; i < n; ++i) { l[p] = a[i]; r[p] = b[i]; p = (p + 1) & (kSize - 1); } w = p; }
    void read (float* a, float* b, int n) const { int p = (w.load() - n) & (kSize - 1); for (int i = 0; i < n; ++i) { a[i] = l[p]; if (b) b[i] = r[p]; p = (p + 1) & (kSize - 1); } }
};

inline const juce::StringArray& eqTypeNames() { static const juce::StringArray n { "CAMPANA", "SHELF BASSI", "SHELF ALTI", "TAGLIO BASSI", "TAGLIO ALTI", "NOTCH", "PASSA BANDA", "TILT" }; return n; }
inline const juce::StringArray& eqChNames() { static const juce::StringArray n { "STEREO", "MID", "SIDE", "SINISTRO", "DESTRO" }; return n; }
inline const char* eqId (int b, const char* k) { static thread_local char buf[24]; std::snprintf (buf, sizeof (buf), "eq%d_%s", b + 1, k); return buf; }

const char* onParam (int m);
const char* deltaParam (int m);
struct VarDelay { std::vector<float> b { 0.0f }; int w = 0; };

struct Preset { const char* name; const char* note; std::vector<std::pair<const char*, float>> v; };
const std::vector<Preset>& presets();
// preset del singolo effetto: per il master o per il mix bus. Caricano solo quell'effetto
struct ModPreset { int mod; bool master; const char* name; const char* note; std::vector<std::pair<const char*, float>> v; };
const std::vector<ModPreset>& modPresets();

class Processor : public juce::AudioProcessor, private juce::Timer
{
public:
    Processor();
    ~Processor() override { stopTimer(); for (int i = 0; i < 500 && refLoading.load(); ++i) juce::Thread::sleep (10); }
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
    void loadModPreset (int i);
    float value (const char* id) const { return apvts.getRawParameterValue (id)->load(); }
    double sampleRate() const { return sr; }

    juce::AudioProcessorValueTreeState apvts;
    jlook::ABCompare ab;
    int preset = 0;

    // misure per l'interfaccia
    std::atomic<float> lufsM { -100 }, lufsS { -100 }, lufsI { -100 }, lra { 0 }, tpMax { -100 }, inS { -100 }, matchDb { 0 }, normDb { 0 };
    std::atomic<float> grMax { 0 }, grLim { 0 }, inPk[2] {}, outPk[2] {}, outRms[2] {};   // outRms: media quadratica ~300 ms (colonna del livello)
    std::atomic<float> bassGain { 0 }, bassLow { 0 }, eqDyn[kEqBands] {}, eqLvl[kEqBands] {}, imCorr[kImBands] {}, imLim[kImBands] {}, corrAll { 1 }, vmGr { 0 };
    std::atomic<int> eqSolo { -1 };
    std::atomic<int> eqListen { -1 };  // FACILE: BASSI/MEDI/ALTI in ascolto. 0..2 = solo della zona, 3..5 = delta della banda
    Biquad bandF[2][2]; int soloKey = -1;   // banda dell'EQ in ascolto da sola (solo interfaccia)
    Biquad solF[2][2]; float solKey[4] { -1, -1, -1, -1 };
    std::atomic<int> order[kNumMods];
    // AVANZATO: effetti inseriti nella catena (un effetto in catena puo essere spento senza toglierlo)
    std::atomic<bool> inRack[kNumMods];
    void rackFromOn() { for (int m = 0; m < kNumMods; ++m) inRack[m] = value (onParam (m)) > 0.5f; }
    void setOrder (const std::array<int, kNumMods>&);
    static std::array<int, kNumMods> defaultOrder() { return { BASS, DYNEQ, EQ432, PULTEC, DEESS, VARIMU, BUSCOMP, MULTI, EXCITER, TAPE, IMAGER, MAXIM, LIMIT }; }
    std::array<int, kNumMods> getOrder() const;
    std::atomic<float> bassPeak { 0 }, exMeter { 0 };
    // traccia di riferimento
    void loadReference (const juce::File&);
    std::shared_ptr<RefTrack> reference() const { const juce::SpinLock::ScopedLockType sl (refLock); return ref; }
    std::atomic<bool> refListen { false }, refMatch { true }, refLoading { false };
    std::atomic<double> refPos { 0 }; std::atomic<float> refGainDb { 0 };
    std::atomic<float> dsGr { 0 }, dsRel { -60 }, busGr { 0 }, mbLevel[kMbBands] {}, mbDown[kMbBands] {}, mbUp[kMbBands] {};
    StereoRing ringOut, ringIn;
    double eqRate() const { return sr; }
    std::atomic<bool> resetReq { false };
    // modalita FACILE
    bool easy() const { return value ("mode") < 0.5f; }
    void autoInput();                                   // porta l'ingresso nella zona ottimale
    static constexpr float kInOpt = -15.0f;             // LUFS short-term ideali in ingresso (prima del LOUD)
    std::atomic<float> inAvg { -100 }, crush { 0 }, grAvg { 0 }, calmNow { 0 }, glueGr { 0 };
    static constexpr int kHist = 4096;  // 20 s a 5 ms: onde fluide anche zoomate
    static constexpr double kColSec = 0.005;
    std::atomic<float> hIn[kHist], hOut[kHist], hGrM[kHist], hGrL[kHist], hS[kHist]; std::atomic<int> hW { 0 };
    std::atomic<float> hRaw[kHist], hRawR[kHist], hOutR[kHist]; std::atomic<int> hTot { 0 };
    std::atomic<float> hPpq[kHist]; std::atomic<float> hostBpm { 120 }; std::atomic<bool> hostPlay { false }; std::atomic<int> hostNum { 4 }; double blockPpq = 0; std::atomic<int> lastBlock { 512 };
    // stato delle viste (non automatizzabile, salvato col progetto): tema, scorrimento, finestra, spettro
    std::atomic<int> uiTheme { 1 }, uiScan { 0 }, uiWin { 1 }, uiSpec { 0 }, uiNorm { 0 }, uiRel { 1 }, uiPrev { 0 };
    std::vector<float> mRawA, mRawQ, mInA;   // il wav senza master (picco e RMS) e l'RMS in uscita

private:
    void timerCallback() override;
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    double sr = 44100;
    EasyEngine easyEng; double inAcc = 0; int inAccN = 0; float grSm = 0;
    BassControl bass; DynEq deq; jmx::ProEq peq; Eq432 e432; Pultec pult; VariMu vmu; Imager img; DeEsser dess; BusComp bus; Multiband mband; TapeMachine tape; Exciter exc; VarDelay deltaVar[2];
    Maximizer mx; Limiter lim; Dither dith; Loudness loudIn, loudOut; TruePeak tpOut[2];
    Delay mxBy[2], limBy[2], dry[2], mxIn[2], deltaPad[2];
    juce::AudioBuffer<float> tmp, keep;
    double sqL = 0, sqR = 0; float matchSm = 1, normSm = 1, colIn = 0, colOut = 0, colM = 0, colL = 0, colRaw = 0; int colN = 0; double sqIn = 0, sqOut = 0;
    std::shared_ptr<RefTrack> ref; mutable juce::SpinLock refLock;
    float tpHold = -100; double cLR = 0, cLL = 1e-9, cRR = 1e-9;
};
}
