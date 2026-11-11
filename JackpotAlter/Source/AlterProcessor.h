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

constexpr int kBandsMax = 4;
struct BandSet { int model = TAPE; float drive = 0, tone = 0, mix = 1, lvlDb = 0; bool solo = false, byp = false; };
struct Settings
{
    int mode = TRASPONI, key = 0, scale = 0, driftType = WOW, order = 0, bands = 1;
    float pitch = 0, formant = 0, drift = 0, driftRate = 1, pmix = 1, mix = 1, outDb = 0, xo[3] { 150, 1000, 5000 };
    bool link = false, autoGain = true;
    BandSet b[kBandsMax];
    // echo
    bool eOn = false, eSync = true, ePing = false; int eDiv = 4; float eMs = 300, eFb = 0.35f, eMix = 0.25f, eLo = 150, eHi = 6000, eSat = 0.3f;
};
inline const juce::StringArray& echoDivs() { static const juce::StringArray d { "1/32", "1/16", "1/16D", "1/8T", "1/8", "1/8D", "1/4T", "1/4", "1/4D", "1/2", "1 BAR" }; return d; }
inline double echoBeats (int i) { static const double b[] = { 0.125, 0.25, 0.375, 1.0 / 3.0, 0.5, 0.75, 2.0 / 3.0, 1.0, 1.5, 2.0, 4.0 }; return b[juce::jlimit (0, 10, i)]; }

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

// ---------------------------------------------------------------- stadio DRIVE multibanda (fino a 4 bande, ognuna col suo modello)
class DriveStage
{
public:
    void prepare (double sr, int maxBlock);
    void process (float* L, float* R, int n, const Settings&);
    int latency() const { return osLat; }
    std::atomic<float> gainRed { 0 }, autoDb { 0 }, bandLev[kBandsMax] {};
private:
    double sr = 44100; int osLat = 0, maxB = 512;
    std::unique_ptr<juce::dsp::Oversampling<float>> os;
    DriveChannel ch[kBandsMax][2];
    juce::dsp::LinkwitzRileyFilter<float> xs[3], ap[3];   // tagli e allpass di compensazione (sovracampionati)
    float lastX[3] { -1, -1, -1 }; int lastNb = -1;
    BQ toneLo[kBandsMax][2], toneHi[kBandsMax][2]; float lastTone[kBandsMax] { 99, 99, 99, 99 };
    float eIn = 1e-6f, eOut = 1e-6f, agSm = 1, lvSm[kBandsMax] { 1, 1, 1, 1 };
};

// ---------------------------------------------------------------- ECHO con saturazione nel ritorno
class EchoStage
{
public:
    void prepare (double s) { sr = s; for (auto& d : buf) d.assign ((size_t) (sr * 4.2) + 8, 0.0f); w = 0; for (auto& f : hp) f.reset(); for (auto& f : lp) f.reset(); dSm = -1; fade = 0; }
    void process (float* L, float* R, int n, const Settings& S, double bpm)
    {
        const float tgt = S.eOn ? 1.0f : 0.0f;
        if (! S.eOn && fade <= 0) return;
        const double dt = juce::jlimit (8.0, (double) buf[0].size() - 8, (S.eSync ? echoBeats (S.eDiv) * 60.0 / juce::jmax (40.0, bpm) : S.eMs * 0.001) * sr);
        if (dSm < 0) dSm = dt;
        if (std::abs (S.eLo - lastLo) > 0.5f || std::abs (S.eHi - lastHi) > 0.5f) { for (int c = 0; c < 2; ++c) { hp[c].hp (sr, S.eLo, 0.6); lp[c].lp (sr, S.eHi, 0.6); } lastLo = S.eLo; lastHi = S.eHi; }
        const int BL = (int) buf[0].size(); const float drv = 1.0f + S.eSat * 3.0f;
        for (int i = 0; i < n; ++i)
        {
            dSm += (dt - dSm) * 0.0005;   // cambi di tempo morbidi (come un nastro che rallenta)
            double rp = w - dSm; while (rp < 0) rp += BL; const int i0 = (int) rp, i1 = (i0 + 1) % BL; const float fr = (float) (rp - i0);
            float y[2]; for (int c = 0; c < 2; ++c) y[c] = buf[c][(size_t) i0] * (1 - fr) + buf[c][(size_t) i1] * fr;
            float fb[2]; for (int c = 0; c < 2; ++c) { float v = lp[c] (hp[c] (y[c])); if (S.eSat > 0.001f) v = std::tanh (v * drv) / drv; fb[c] = v * S.eFb; }
            const float inL = L[i], inR = R[i];
            if (S.ePing) { buf[0][(size_t) w] = 0.5f * (inL + inR) + fb[1]; buf[1][(size_t) w] = fb[0]; }
            else { buf[0][(size_t) w] = inL + fb[0]; buf[1][(size_t) w] = inR + fb[1]; }
            w = (w + 1) % BL;
            fade = juce::jlimit (0.0f, 1.0f, fade + (tgt > fade ? 1.0f : -1.0f) / (float) (0.02 * sr));
            const float m = S.eMix * fade;
            L[i] = inL + y[0] * m; R[i] = inR + y[1] * m;
        }
    }
private:
    double sr = 44100, dSm = -1; std::vector<float> buf[2]; int w = 0; BQ hp[2], lp[2]; float lastLo = -1, lastHi = -1, fade = 0;
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
    PitchStage pitch; DriveStage drive; EchoStage echo;
    std::atomic<double> bpm { 120 };
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
