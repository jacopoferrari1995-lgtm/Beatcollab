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
// curve di riferimento per genere: spettro medio a lungo termine di mix tipici del genere, gia compensato di 4.5 dB/ottava
// (una musica "bilanciata" e piatta), 0 dB a 1 kHz. Sono indicative: per un confronto vero carica un pezzo di riferimento.
constexpr int kRefPts = 22;
inline const float* refFreqs() { static const float f[kRefPts] = { 20, 30, 40, 50, 60, 80, 100, 150, 200, 300, 500, 800, 1000, 1500, 2000, 3000, 5000, 8000, 10000, 12000, 16000, 20000 }; return f; }
struct Profile { const char* name; float db[kRefPts]; };
constexpr int kGenres = 10;
inline const Profile& profile (int i)
{
    static const Profile P[kGenres] = {
        { "TRAP",              { -6, 2, 6, 7.5f, 8, 7, 5.5f, 2.5f, 1, 0, -0.5f, 0, 0, 0, 0, 0.5f, 0.5f, 0, -1, -2, -5, -12 } },
        { "DRILL",             { -5, 3, 6.5f, 7.5f, 7.5f, 6, 4.5f, 2, 1, 0, -0.5f, 0, 0, 0, 0.5f, 1, 1, 0.5f, -0.5f, -1.5f, -4.5f, -11 } },
        { "BOOM BAP",          { -10, -3, 1.5f, 3.5f, 4.5f, 5, 4.5f, 3, 2, 1, 0, 0, 0, -0.5f, -0.5f, -1, -2, -3, -4, -5, -8, -14 } },
        { "LO-FI",             { -12, -5, 0, 2.5f, 4, 5, 5, 4, 3, 1.5f, 0.5f, 0, 0, -1, -1.5f, -3, -5, -7, -8.5f, -10, -14, -20 } },
        { "R&B",               { -9, -2, 2.5f, 4.5f, 5.5f, 5.5f, 4.5f, 2.5f, 1.5f, 0.5f, 0, 0, 0, 0, 0, 0, 0.5f, 0, -1, -2, -5, -12 } },
        { "POP",               { -12, -5, -0.5f, 2, 3, 3.5f, 3, 2, 1.5f, 0.5f, 0, 0, 0, 0, 0.5f, 1, 1, 0.5f, 0, -1, -3.5f, -10 } },
        { "AFRO / REGGAETON",  { -9, -1, 3.5f, 5.5f, 6, 5.5f, 4.5f, 2.5f, 1.5f, 0.5f, 0, 0, 0, 0, 0, 0.5f, 0.5f, 0, -1, -2, -5, -12 } },
        { "ELETTRONICA",       { -8, 0, 4, 5.5f, 6, 5.5f, 4, 2, 1, 0, -0.5f, 0, 0, 0, 0.5f, 1, 1.5f, 1, 0.5f, -0.5f, -3, -9 } },
        { "ROCK",              { -15, -8, -3, 0, 1.5f, 2.5f, 3, 2.5f, 2, 1, 0.5f, 0, 0, 0, 0.5f, 0.5f, 0, -1, -2, -3, -6, -13 } },
        { "ACUSTICO",          { -18, -10, -5, -2.5f, -1, 0.5f, 1.5f, 2, 2, 1.5f, 0.5f, 0, 0, 0, 0, -0.5f, -1.5f, -3, -4, -5, -8, -15 } } };
    return P[juce::jlimit (0, kGenres - 1, i)];
}
inline float profileAt (int g, float f)
{
    const float* F = refFreqs(); const auto& P = profile (g);
    if (f <= F[0]) return P.db[0]; if (f >= F[kRefPts - 1]) return P.db[kRefPts - 1];
    int k = 0; while (f > F[k + 1]) ++k;
    const float t = std::log (f / F[k]) / std::log (F[k + 1] / F[k]); return P.db[k] + (P.db[k + 1] - P.db[k]) * t;
}
// punti dello spettro: 320 bande logaritmiche 20 Hz - 20 kHz
constexpr int kPts = 320;
inline float ptFreq (int i) { return 20.0f * std::pow (1000.0f, (i + 0.5f) / kPts); }
// pezzo di riferimento caricato
struct RefTrack { juce::String name, path; std::vector<float> spec; float lufs = -100, peak = -100, rms = -100; bool ok = false; juce::String error; };
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
    // storia dei picchi per la vista PEAK: un punto ogni 10 ms (20 s)
    static constexpr int kPH = 2000;
    std::atomic<float> phMax[kPH] {}, phRms[kPH] {}; std::atomic<int> phW { 0 };
    // riferimento
    void loadReference (const juce::File&);
    void clearReference();
    RefTrack reference() const { const juce::SpinLock::ScopedLockType l (refLock); return ref; }
    std::atomic<int> refVersion { 0 }; std::atomic<bool> refLoading { false };

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
    float phPk = 0; double phE = 0; int phN = 0;
    mutable juce::SpinLock refLock; RefTrack ref;
    std::atomic<float>* bpp[kBands][7] {}; std::atomic<float>* gp[16] {};
};
}
