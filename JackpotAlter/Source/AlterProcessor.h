#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "JackpotLook.h"
#include "PitchEngine.h"
#include "DriveModels.h"

// Jackpot Alter Drive: pitch e formante (TRASPONI / QUANTIZZA / ROBOT) + drive con 13 modelli,
// a una banda o a tre bande con il drive dosato per banda. Niente eco: semplice come un AlterBoy.
namespace ja
{
struct Preset { const char* name; const char* note; std::vector<std::pair<const char*, float>> v; };
const std::vector<Preset>& presets();

enum PitchMode { TRASPONI, QUANTIZZA, ROBOT };

// anello senza lock per mandare l'audio all'interfaccia (analizzatore)
struct Ring
{
    static constexpr int kSize = 8192;
    float d[kSize] {}; std::atomic<int> w { 0 };
    void push (const float* x, int n) { int p = w.load(); for (int i = 0; i < n; ++i) { d[p] = x[i]; p = (p + 1) & (kSize - 1); } w = p; }
    void read (float* out, int n) const { int p = (w.load() - n) & (kSize - 1); for (int i = 0; i < n; ++i) { out[i] = d[p]; p = (p + 1) & (kSize - 1); } }
};

// crossover Linkwitz-Riley a 24 dB (due Butterworth in cascata), copiabile da un canale all'altro
struct LR4
{
    BQ l1, l2, h1, h2;
    void set (double fs, double f) { l1.lp (fs, f, 0.70710678); l2.b0 = l1.b0; l2.b1 = l1.b1; l2.b2 = l1.b2; l2.a1 = l1.a1; l2.a2 = l1.a2; h1.hp (fs, f, 0.70710678); h2.b0 = h1.b0; h2.b1 = h1.b1; h2.b2 = h1.b2; h2.a1 = h1.a1; h2.a2 = h1.a2; }
    void reset() { l1.reset(); l2.reset(); h1.reset(); h2.reset(); }
    inline void split (float x, float& lo, float& hi) { lo = l2 (l1 (x)); hi = h2 (h1 (x)); }
    inline float allpass (float x) { float a, b; split (x, a, b); return a + b; }
};

struct Settings
{
    int mode = TRASPONI, key = 9, scale = 2, model = TAPE;
    float pitch = 0, formant = 0, drive = 0, tone = 0, w[3] { 1, 1, 1 }, x1 = 200, x2 = 3000, mix = 1, outDb = 0;
    bool link = false, multi = true, autoGain = true;
    bool mono = false;   // sinistro e destro identici in questo blocco: si calcola un canale solo
};

// ---------------------------------------------------------------- stadio PITCH
class PitchStage
{
public:
    void prepare (double sr, int maxBlock);
    void process (float* L, float* R, int n, const Settings&);
    int latency() const { return N; }
    std::atomic<float> inHz { 0 }, outHz { 0 }, corrSemi { 0 }; std::atomic<bool> active { false };
private:
    void frame (const Settings&, bool monoOnly);
    double sr = 44100; int N = 2048, H = 256, hopPos = 0;
    PhaseVocoder pv[2]; PitchDetector det;
    std::vector<float> dly[2]; int dPos = 0;          // percorso pulito (stessa latenza) quando il pitch e' fermo
    float wet = 0, corr = 0, lastF0 = 0; int unvoiced = 0; bool wasMono = false;
};

// ---------------------------------------------------------------- stadio DRIVE: una banda o tre, un modello
class DriveStage
{
public:
    void prepare (double sr, int maxBlock);
    void process (float* L, float* R, int n, const Settings&);   // n <= maxBlock
    int latency() const { return osLat; }
    std::atomic<float> autoDb { 0 }, bandLev[3] {};
private:
    double sr = 44100; int osLat = 0, maxB = 512;
    std::unique_ptr<juce::dsp::Oversampling<float>> os;
    DriveChannel ch[4][2], postCh[2];                        // 0-2 bande, 3 banda unica; post: i filtri lineari, una volta sulla somma
    LR4 x1[2], x2[2], ap2[2];                                // tagli (sovracampionati) e allpass che riallinea i bassi
    float lastX1 = -1, lastX2 = -1;
    BQ toneLo[2], toneHi[2]; float toneSm = 0, toneSet = 99;
    float eIn = 1e-6f, eOut = 1e-6f, agSm = 1;
    float gPrev[4] { 1, 1, 1, 1 }, dPrev[4] {}, wPrev[4] {}, mbSm = 1;  // valori del blocco prima (rampe senza scalini)
    std::vector<float> bd[2], tdl, tdr; int bdPos = 0; float xw = 0;   // a riposo: solo un ritardo uguale (neutro al bit)
    int lastModel = -1;
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
    int getCurrentProgram() override { return juce::jmax (0, preset); }
    void setCurrentProgram (int i) override { loadPreset (i); }
    const juce::String getProgramName (int i) override { return juce::isPositiveAndBelow (i, (int) presets().size()) ? presets()[(size_t) i].name : ""; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    void loadPreset (int i);
    float value (const char* id) const { return apvts.getRawParameterValue (id)->load(); }
    Settings settings() const;   // solo puntatori in cache: si puo' chiamare dal thread audio

    juce::AudioProcessorValueTreeState apvts;
    jlook::ABCompare ab;
    int preset = 0;
    juce::String userName;
    juce::String currentName() { return userName.isNotEmpty() ? userName : getProgramName (juce::jmax (0, preset)); }
    static juce::File presetFolder();
    bool saveUserPreset (const juce::String&);
    bool loadUserPreset (const juce::File&);

    PitchStage pitch; DriveStage drive;
    Ring ringIn, ringOut;
    std::atomic<float> inPk[2] {}, outPk[2] {};
    double sampleRate() const { return sr; }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    struct Ptr { std::atomic<float>* mode, *pitch, *formant, *link, *key, *scale, *model, *drive, *tone, *multi, *w[3], *x1, *x2, *mix, *out, *autoG; } P {};
    double sr = 44100; int maxBlock = 512;
    std::vector<float> dry[2], dryL, dryR, tmpR, mono; int dryPos = 0, dryLen = 1, totalLat = 0;
    juce::SmoothedValue<float> mixSm { 1.0f }, outSm { 1.0f };
};
}
