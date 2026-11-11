#pragma once
#include "JackpotLook.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

namespace jm
{
constexpr int kBands = 12;
constexpr int kBal = 6;   // bande del bilanciamento tonale
enum BandType { BELL = 0, LSHELF, HSHELF, LCUT, HCUT, kNumTypes };

struct Target { const char* name; float lufs; float tp; };
inline const Target& target (int i)
{
    static const Target T[] = { { "SPOTIFY -14", -14, -1 }, { "APPLE -16", -16, -1 }, { "YOUTUBE -14", -14, -1 }, { "CLUB -9", -9, -0.3f }, { "CD / LOUD -8", -8, -0.3f } };
    return T[juce::jlimit (0, 4, i)];
}
// bilanciamento tonale: scarti consigliati (dB) per banda rispetto a una curva bilanciata (rosa compensato 4.5 dB/ott)
struct Profile { const char* name; float off[6]; };
inline const Profile& profile (int i)
{
    static const Profile P[] = {
        { "TRAP / DRILL", { 5, 2.5f, -1, -1, -1.5f, -2 } },
        { "HIP HOP / BOOM BAP", { 3, 3, 0, -0.5f, -1, -2.5f } },
        { "LO-FI", { 2, 3, 1, -0.5f, -3, -5 } },
        { "POP", { 1.5f, 1, -0.5f, 0, 0, 0 } },
        { "ACUSTICO", { -2, 0.5f, 0.5f, 0.5f, 0, -0.5f } } };
    return P[juce::jlimit (0, 4, i)];
}
inline const char* bandName (int b) { static const char* n[] = { "SUB", "BASSI", "MEDIO-BASSI", "MEDI", "PRESENZA", "ARIA" }; return n[juce::jlimit (0, 5, b)]; }
inline float bandEdge (int k) { static const float e[] = { 20, 60, 250, 800, 2500, 6000, 20000 }; return e[juce::jlimit (0, 6, k)]; }

// filtro biquad semplice (RBJ)
struct Biq
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1[2] = {}, z2[2] = {};
    void set (int type, double f, double gDb, double q, double sr)
    {
        f = juce::jlimit (10.0, sr * 0.49, f); q = juce::jmax (0.1, q);
        const double A = std::pow (10.0, gDb / 40.0), w = juce::MathConstants<double>::twoPi * f / sr, cs = std::cos (w), sn = std::sin (w), al = sn / (2 * q);
        double B0, B1, B2, A0, A1, A2;
        switch (type)
        {
            case LSHELF: { const double s = 2 * std::sqrt (A) * al; B0 = A * ((A + 1) - (A - 1) * cs + s); B1 = 2 * A * ((A - 1) - (A + 1) * cs); B2 = A * ((A + 1) - (A - 1) * cs - s); A0 = (A + 1) + (A - 1) * cs + s; A1 = -2 * ((A - 1) + (A + 1) * cs); A2 = (A + 1) + (A - 1) * cs - s; break; }
            case HSHELF: { const double s = 2 * std::sqrt (A) * al; B0 = A * ((A + 1) + (A - 1) * cs + s); B1 = -2 * A * ((A - 1) + (A + 1) * cs); B2 = A * ((A + 1) + (A - 1) * cs - s); A0 = (A + 1) - (A - 1) * cs + s; A1 = 2 * ((A - 1) - (A + 1) * cs); A2 = (A + 1) - (A - 1) * cs - s; break; }
            case LCUT: B0 = (1 + cs) / 2; B1 = -(1 + cs); B2 = (1 + cs) / 2; A0 = 1 + al; A1 = -2 * cs; A2 = 1 - al; break;
            case HCUT: B0 = (1 - cs) / 2; B1 = 1 - cs; B2 = (1 - cs) / 2; A0 = 1 + al; A1 = -2 * cs; A2 = 1 - al; break;
            case 9: B0 = al; B1 = 0; B2 = -al; A0 = 1 + al; A1 = -2 * cs; A2 = 1 - al; break;   // passa banda (rilevatore)
            default: B0 = 1 + al * A; B1 = -2 * cs; B2 = 1 - al * A; A0 = 1 + al / A; A1 = -2 * cs; A2 = 1 - al / A; break;
        }
        b0 = B0 / A0; b1 = B1 / A0; b2 = B2 / A0; a1 = A1 / A0; a2 = A2 / A0;
    }
    float tick (float x, int c) { const double y = b0 * x + z1[c]; z1[c] = b1 * x - a1 * y + z2[c]; z2[c] = b2 * x - a2 * y; return (float) y; }
    void reset() { z1[0] = z1[1] = z2[0] = z2[1] = 0; }
    double magDb (double f, double sr) const
    {
        const std::complex<double> z = std::polar (1.0, -juce::MathConstants<double>::twoPi * f / sr);
        const auto h = (b0 + b1 * z + b2 * z * z) / (1.0 + a1 * z + a2 * z * z);
        return 20 * std::log10 (std::abs (h) + 1e-12);
    }
};

class Processor : public juce::AudioProcessor
{
public:
    Processor();
    void prepareToPlay (double, int) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& l) const override
    {
        if (l.getMainOutputChannelSet() != juce::AudioChannelSet::stereo() || l.getMainInputChannelSet() != l.getMainOutputChannelSet()) return false;
        if (l.inputBuses.size() > 1) { const auto sc = l.getChannelSet (true, 1); if (! sc.isDisabled() && sc != juce::AudioChannelSet::stereo() && sc != juce::AudioChannelSet::mono()) return false; }
        return true;
    }
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Jackpot Metering Hub"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;
    float value (const char* id) const { return apvts.getRawParameterValue (id)->load(); }
    double getSr() const { return sr; }

    juce::AudioProcessorValueTreeState apvts;
    jlook::ABCompare ab;

    // anello per FFT e goniometro (post EQ = cio che esce)
    static constexpr int kRing = 16384;
    float ringL[kRing] {}, ringR[kRing] {}, ringS[kRing] {}; std::atomic<int> ringW { 0 };
    std::atomic<bool> frozen { false }, freezeBtn { false }, hasSc { false }, playing { false };   // spettro fermo quando l'host e in stop
    // EQ: dati per l'interfaccia
    std::atomic<float> dynGain[kBands] {};
    std::atomic<int> soloBand { -1 };   // ascolto di una sola banda dell'EQ
    // livelli in dBFS per canale
    std::atomic<float> pkL { -100 }, pkR { -100 }, rmsL { -100 }, rmsR { -100 }, pkHoldL { -100 }, pkHoldR { -100 };
    // loudness
    std::atomic<float> lufsM { -100 }, lufsS { -100 }, lufsI { -100 }, lra { 0 }, tpMax { -100 }, tpNow { -100 }, rmsDb { -100 }, pkMax { -100 };
    static constexpr int kHist = 600;   // storia short-term, un punto ogni 0.2 s (2 minuti)
    std::atomic<float> histS[kHist], histM[kHist]; std::atomic<int> histW { 0 };
    // stereo
    std::atomic<float> corr { 1 }, corrB[3] {}, widthB[3] {}, balance { 0 }, width { 0 };
    std::atomic<bool> resetReq { false };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    double sr = 44100;
    // EQ dinamico
    Biq eq[kBands], det[kBands]; float env[kBands] {}; int sub = 0;
    // K-weighting
    double kb[2][3], ka[2][3], kz[2][2][2] {};
    double blockE = 0; int blockN = 0, blockLen = 4410;
    std::vector<double> blocks; int blockIdx = 0;   // ultimi 30 blocchi da 100 ms
    std::vector<int> histI, histST; int nI = 0, nST = 0;   // istogrammi per integrato e LRA (0.1 dB, -70..+5)
    double sumI = 0;
    juce::dsp::Oversampling<float> os { 2, 2, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true };
    juce::AudioBuffer<float> tpBuf;
    // stereo
    Biq soloF; float soloLast[3] = { -1, -1, -1 };
    Biq imX[4]; Biq imMono; float imMonoLast = -1; double eL = 0, eR = 0;
    Biq xo[4]; double sLL = 0, sRR = 0, sLR = 0, bLL[3] {}, bRR[3] {}, bLR[3] {}, bM[3] {}, bS[3] {}, eM = 0, eS = 0;
    int histTick = 0, blocksSeen = 0; float stMax = -100;
    void resetLoudness();
};
}
