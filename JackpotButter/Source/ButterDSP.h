#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>

// BURRO BUS: catena di effetti per batteria ispirata alla logica dei bus multi-modulo
// Ordine: BOOM/NOISE -> SHIFT -> COMPRESS -> SATURATE -> SPACE -> CLIP -> MASTER, con MAGNITUDE dry/wet globale.
namespace jb
{
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
private:
    double sr = 44100;
    std::vector<float> dryL, dryR, trW;
    // rilevatore transienti condiviso
    float envF = 0, envS = 0;
    // boom
    float bIc1[2] = {}, bIc2[2] = {}, acidEnv = 0;
    // noise
    float pinkB[2][3] = {}, nLp[2] = {}, nBp1[2] = {}, nBp2[2] = {}; int crackle = 0; float crackleV = 0;
    juce::Random rng;
    // shift
    juce::AudioBuffer<float> shBuf; int shW = 0; double shPh = 0;
    // focus
    float fLo[3][2] = {}, fHi[3][2] = {};
    // compressore
    float cEnv[4] = {}; float xLo[2] = {}, xHi[2] = {};
    // saturazione
    float satLp[2] = {}, holdV[2] = {}; double holdPh = 0;
    // spazio
    juce::Reverb verb; juce::AudioBuffer<float> preBuf; int preW = 0; std::vector<float> wetL, wetR; float apZ[2][4] = {};
    // clip
    float mLp[2] = {};
    // master
    float tLp[2] = {}, aLp[2] = {}, mEnvF = 0, mEnvS = 0;
    int lastSpType = -1;
public:
    std::atomic<int> order[4] { 1, 2, 3, 4 };   // moduli centrali riordinabili (1 shift, 2 comp, 3 sat, 4 space)
};
}
