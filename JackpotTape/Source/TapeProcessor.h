#pragma once
#include "JackpotLook.h"
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <cstring>

namespace jt
{
enum Fx { STOP, START, SPIN, HALF, REV, STUT, LOOP, GLITCH, GATE, CRUSH, SWEEP, ECHO, THROW, RISER, TREM, ROTARY, AUTOFILT, RINGMOD, FSHIFT, DIST, kNumFx };
// effetti "sopra" il nastro: non spostano la testina
inline bool isOverlay (int f) { return f == GATE || f == CRUSH || f >= SWEEP; }
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

// distorsione dello stacco: da saturazione morbida (char 0) a distorsione dura (0.5) a fuzz ripiegato (1); uscita compensata
inline float crushShape (float x, float drive, float ch)
{
    const float g = 1.0f + drive * drive * 40.0f + drive * 3.0f;
    const float soft = std::tanh (x * g), hard = juce::jlimit (-1.0f, 1.0f, x * g * 1.2f), fold = std::sin (juce::jlimit (-6.0f, 6.0f, x * g) * 1.5707963f);
    const float y = ch < 0.5f ? soft + (hard - soft) * ch * 2.0f : hard + (fold - hard) * (ch - 0.5f) * 2.0f;
    return y * (0.3f / std::tanh (0.3f * g));
}
inline float crushBits (float y, float bits) { if (bits >= 15.9f) return y; const float q = std::pow (2.0f, bits - 1.0f); return std::round (y * q) / q; }

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
    double getTailLengthSeconds() const override { return 6.0; }
    int getNumPrograms() override;
    int getCurrentProgram() override { return presetIndex; }
    void setCurrentProgram (int) override;
    const juce::String getProgramName (int) override;
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;
    jlook::ABCompare ab;

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<bool> seq[kNumFx][kMaxSteps];
    std::atomic<bool> uiPad[kNumFx];           // pad tenuti dall'interfaccia o dalla tastiera
    int presetIndex = 0;
    void loadPreset (int i);
    void applyAge (float a);
    void generate();   // coreografia generata a regole
    float value (const char* id) const { return apvts.getRawParameterValue (id)->load(); }
    float fxValue (int fx, const char* p) const;
    bool fxEnabled (int fx) const { return onPtr[fx]->load() > 0.5f; }
    juce::String seqString() const;
    void setSeqString (const juce::String&);

    // per il display
    std::atomic<float> dispWet[kCols], dispDry[kCols], dispSpd[kCols];
    std::atomic<int> dispMask[kCols];
    std::atomic<int> activeMask { 0 }, uiStep { -1 };
    std::atomic<double> uiBpm { 140.0 }, uiBeat { 0.0 };
    std::atomic<bool> uiHost { false };
    std::atomic<float> meterL { 0 }, meterR { 0 }, uiSpeed { 1 };

private:
    std::atomic<float>* paramPtr[160] = {};
    std::atomic<float>* padPtr[kNumFx] = {};
    std::atomic<float>* onPtr[kNumFx] = {};
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
    float speedAcc = 0; int speedN = 0; float volSm = 1, hissLp = 0, stLp[2] = {};
    float mixSm = 1, crFade = 0, crHold[2] = {}, crLp[2] = {}; int crCnt = 0;
    // sweep, eco, lancio di riverbero, riser
    float swFade = 0, swIc[2][2] = {}, swCut = 1000; double swProg = 0;
    juce::AudioBuffer<float> eBuf; int eW = 0; float eSend = 0, eCut = 0, eLp[2] = {}, eHp[2] = {}, eTime = 0;
    juce::Reverb verb; juce::AudioBuffer<float> vPre; int vW = 0; float vSend = 0, vCut = 0, vHp[2] = {}; int vIdle = 1 << 30;
    juce::AudioBuffer<float> rBuf; int rW = 0; double rPh = 0; float rFade = 0, rHp[2] = {}, rNoiseLp = 0, rSt = 0;
    // modulazioni sul nastro: tremolo, rotary, autofiltro, ring mod, frequency shift, distorsione
    float modFade[6] = {}; double rotH = 0, rotD = 0, ringPh = 0, fsPh = 0; float rotSp = 0, afIc[2][2] = {}, rotXo[2] = {}, fsFb[2] = {}, dsHp[2] = {}, dsLp[2] = {}, dsPre[2] = {};
    juce::AudioBuffer<float> rotBuf; int rotW = 0;
    struct HAp { float c = 0, x1 = 0, x2 = 0, y1 = 0, y2 = 0; float run (float x) { const float y = c * (x + y2) - x2; x2 = x1; x1 = x; y2 = y1; y1 = y; return y; } };
    HAp hA[2][4], hB[2][4]; float hBd[2] = {};

    float rd (double pos, int ch) const;
    void activate (int fx, double bf, double beatAtT, juce::int64 t);
    double beatOf (juce::int64 t, juce::int64 t0, double beat0, double bf) const { return beat0 + (double) (t - t0) / bf; }
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
};
}
