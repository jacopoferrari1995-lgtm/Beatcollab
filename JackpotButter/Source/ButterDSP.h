#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "ButterAnalog.h"

// BURRO BUS: catena di effetti per batteria ispirata alla logica dei bus multi-modulo
// Ordine: BOOM/NOISE -> SHIFT -> COMPRESS -> SATURATE -> SPACE -> CLIP -> MASTER, con MAGNITUDE dry/wet globale.
namespace jb
{
// storia per i display (scritta dall'audio, letta dall'interfaccia)
struct Scope
{
    static constexpr int N = 160;
    std::atomic<float> v[N]; std::atomic<int> w { 0 };
    Scope() { for (auto& x : v) x = 0; }
    void push (float x) { const int i = w.load (std::memory_order_relaxed); v[i] = x; w.store ((i + 1) % N, std::memory_order_relaxed); }
    float at (int k) const { return v[(w.load() + k) % N].load(); }   // k = 0 piu vecchio
};
struct Biq
{
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1[2] = {}, z2[2] = {};
    void set (int type, double sr, double f, double gDb, double q);
    inline float run (float x, int c) { const float y = b0 * x + z1[c]; z1[c] = b1 * x - a1 * y + z2[c]; z2[c] = b2 * x - a2 * y; return y; }
    static float magDb (int type, double sr, double f, double gDb, double q, double hz);
};

// riverbero a rete di ritardi (FDN 8 linee) con diffusione in ingresso e modulazione lenta
class Fdn
{
public:
    void prepare (double sr);
    void reset();
    void setup (int type, float decay);
    void process (const float* inL, const float* inR, float* outL, float* outR, int N);
private:
    static constexpr int NL = 8, NA = 4;
    double sr = 44100;
    std::vector<float> line[NL], ap[NA]; int lw[NL] = {}, aw[NA] = {}, len[NL] = {}, alen[NA] = {};
    float g[NL] = {}, lp[NL] = {}, damp = 0.3f, diff = 0.7f, lfo = 0, lfoInc = 0, modDepth = 0;
    int curType = -1; float curDecay = -1;
};

struct BurroPreset { const char* name; std::vector<std::pair<const char*, float>> p; };
const std::vector<BurroPreset>& burroPresets();

class Burro
{
public:
    static void addParams (juce::AudioProcessorValueTreeState::ParameterLayout& l);
    void prepare (double sampleRate, int maxBlock);
    void process (float* L, float* R, int N, juce::AudioProcessorValueTreeState& s);
    static void applyPreset (juce::AudioProcessorValueTreeState& s, int index);

    std::atomic<float> grMeter { 0 };   // riduzione di guadagno del compressore, dB
    Scope scIn, scOut, scGr, scTr;      // inviluppo in/out, riduzione, peso dei transienti
    // analizzatore: ultimi campioni mono prima e dopo
    static constexpr int AN = 4096;
    float anIn[AN] = {}, anOut[AN] = {}, anL[AN] = {}, anR[AN] = {}; std::atomic<int> anW { 0 };
    std::atomic<float> corr { 1 }; double cLL = 0, cRR = 0, cLR = 0;
private:
    double sr = 44100;
    std::vector<float> dryL, dryR, trW;
    // rilevatore transienti condiviso
    float envF = 0, envS = 0;
    // boom / impact
    float bIc1[2] = {}, bIc2[2] = {}, acidEnv = 0, thPh = 0, thEnv = 0, thPitch = 0, bLow[2] = {}; bool thArmed = true;
    // noise
    float pinkB[2][3] = {}, nLp[2] = {}, nBp1[2] = {}, nBp2[2] = {}; int crackle = 0; float crackleV = 0;
    juce::Random rng;
    // shift
    juce::AudioBuffer<float> shBuf; int shW = 0; double shPh = 0; Formant form;
    // focus
    float fLo[3][2] = {}, fHi[3][2] = {};
    // compressore
    float cEnv[4] = {}; float xLo[2] = {}, xHi[2] = {};
    // saturazione
    float satLp[2] = {}, holdV[2] = {}; double holdPh = 0;
    // spazio
    Fdn verb; Biq spF[2], xF[2], fF[4]; float spLast[2] = { -1, -1 }, xLast[2] = { -1, -1 }, fLast[4] = { -1, -1, -1, -1 };
    float imLp[2] = {}; Biq imB; float imLast = -1;
    juce::AudioBuffer<float> preBuf; int preW = 0; std::vector<float> wetL, wetR; float apZ[2][4] = {};
    // clip
    float mLp[2] = {};
    // master
    float tLp[2] = {}, aLp[2] = {}, mEnvF = 0, mEnvS = 0;
    int lastSpType = -1;
    Bq ae[4]; bool aeAct[4] {}; AeColour aeCol; float lastAe[12] = { 99 };
    Bq pu[4]; PultecSettings puLast; bool puInit = false; AeColour puCol; Maxx maxx;
    // modulo EXTRA
    juce::AudioBuffer<float> xBuf; int xW = 0; float xLp[2] = {}, xHold[2] = {}; double xPh = 0, xLfo = 0, xDrift = 0, xDt = 0; juce::Random xr; int lastXType = -1;
    float accIn = 0, accOut = 0, accGr = 0, accTr = 0; int accN = 0;
public:
    std::atomic<int> order[5] { 1, 2, 3, 4, 6 };   // moduli centrali riordinabili (1 shift, 2 comp, 3 sat, 4 space, 6 extra)
    std::atomic<double> bpm { 120.0 };
};
}
