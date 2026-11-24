#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "ButterAnalog.h"
#include "../../Common/JkDriveModels.h"
#include "../../Common/JkProEq.h"
#include <juce_dsp/juce_dsp.h>
#include <unordered_map>
#include <string_view>

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
// EQ: 8 bande libere, lo stesso motore dell'EQ del Master (campana, shelf, tagli, notch, tilt; stereo/mid/side/L/R; dinamica)
constexpr int kQBands = 8;
const char* qId (int band, const char* key);          // "u_q3_f" ecc. (puntatori stabili)
const juce::StringArray& qTypeNames();
const juce::StringArray& qChNames();
// vecchio EQ analogico (modelli) -> bande dell'EQ nuovo. set (id, valore)
void aeToBands (const AeSettings& S, const std::function<void (const char*, float)>& set);
const std::vector<BurroPreset>& burroPresets();

struct Ramp { float a = 0, b = 0, n = 1; inline float at (int i) const { return a + (b - a) * (float) (i + 1) / n; } inline float atF (float t) const { return a + (b - a) * t; } };
struct Hist
{
    std::vector<float> b; int w = 0;
    void prepare (int n) { b.assign ((size_t) juce::jmax (8, n), 0.0f); w = 0; }
    inline void push (float x) { b[(size_t) w] = x; if (++w >= (int) b.size()) w = 0; }
    inline float read (int lag, int i, int N) const { int k = w - N + i - lag; const int n = (int) b.size(); k %= n; if (k < 0) k += n; return b[(size_t) k]; }
};

// porta di un modulo a 8x: quando e' spento passa solo un ritardo pari alla latenza (neutro al bit, niente CPU);
// quando si accende riparte pulito e entra con una dissolvenza
struct OsGate
{
    std::vector<float> ring[2], tmp[2]; int w = 0, L = 1, warm = 0; float m = 0; bool running = false;
    void prepare (int lat, int maxBlock) { L = juce::jmax (1, lat); for (int c = 0; c < 2; ++c) { ring[c].assign ((size_t) L, 0.0f); tmp[c].assign ((size_t) maxBlock, 0.0f); } w = 0; m = 0; warm = 0; running = false; }
    void mix (float* const* ch, int N, bool act)
    {
        const float step = 1.0f / 1024.0f;
        for (int i = 0; i < N; ++i)
        {
            if (warm > 0) --warm; else m = act ? juce::jmin (1.0f, m + step) : juce::jmax (0.0f, m - step);
            for (int c = 0; c < 2; ++c)
            {
                const float in = running ? tmp[c][(size_t) i] : ch[c][i], d = ring[c][(size_t) w]; ring[c][(size_t) w] = in;
                ch[c][i] = running ? d + (ch[c][i] - d) * m : d;
            }
            if (++w >= L) w = 0;
        }
        if (! act && m <= 0.0f && warm == 0) running = false;
    }
};

class Burro
{
public:
    static constexpr int kOs = 8;
    int latency() const { return osLat; }
    static void addParams (juce::AudioProcessorValueTreeState::ParameterLayout& l);
    void prepare (double sampleRate, int maxBlock);
    void bind (juce::AudioProcessorValueTreeState&);   // tabella dei parametri (una volta, fuori dal thread audio)
    void process (float* L, float* R, int N, juce::AudioProcessorValueTreeState& s);
    static void applyPreset (juce::AudioProcessorValueTreeState& s, int index);

    std::atomic<float> grMeter { 0 };   // riduzione di guadagno del compressore, dB
    Scope scIn, scOut, scGr, scTr;      // inviluppo in/out, riduzione, peso dei transienti
    // analizzatore: ultimi campioni mono prima e dopo
    static constexpr int AN = 4096;
    float anIn[AN] = {}, anOut[AN] = {}, anL[AN] = {}, anR[AN] = {}; std::atomic<int> anW { 0 };
    std::atomic<float> corr { 1 }; double cLL = 0, cRR = 0, cLR = 0;
private:
    std::unique_ptr<juce::dsp::Oversampling<float>> osSat, osClip, osLim; OsGate gSat, gClip, gLim; int latSat = 0, latClip = 0, latLim = 0, osLat = 0;
    Hist dryDl[2], trDl; float rampLast[20] {}; float cMk = 0; float fLo4[2][2] {}, fHi4[2][2] {};
    std::vector<std::string> pkeys; std::unordered_map<std::string_view, std::atomic<float>*> pcache;
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
    float satLp[2] = {}, holdV[2] = {}; double holdPh = 0; float satLvl[2] = {}, satAg = 1;
    jmx::ProEq peq;
public:
    std::atomic<int> eqSolo { -1 }; float eqDyn (int b) const { return peq.dynUi[b].load(); }
private:
    ja::DriveChannel drv[2]; ja::BQ satTone[2], satAA[2][4], satLp8[2][8], clipLp8[2][8];
    // curve con antiderivata (ADAA): tanh e clip morbido
    struct Ad
    {
        double x1 = 0; bool init = false;
        static double lc (double v) { const double a = std::abs (v); return a + std::log1p (std::exp (-2.0 * a)) - 0.6931471805599453; }
        static double sF (double x) { const double t = 0.9, k = 0.15, a = std::abs (x); if (a <= t - k) return 0.5 * x * x; if (a >= t + k) { const double e = t + k, Fe = 0.5 * (t - k) * (t - k) + (2 * k) * (t - k) + 0.5 * 4 * k * k - (8 * k * k * k) / (12 * k); return Fe + t * (a - e); } const double d = a - (t - k); return 0.5 * (t - k) * (t - k) + (t - k) * d + 0.5 * d * d - d * d * d / (12 * k); }
        template <typename F, typename G> float run (float x, F&& Fa, G&& f) { if (! init) { init = true; x1 = x; return f (x); } const double dx = x - x1; const float y = std::abs (dx) > 1e-5 ? (float) ((Fa (x) - Fa (x1)) / dx) : f ((float) (0.5 * (x + x1))); x1 = x; return y; }
        float tanhA (float x) { return run (x, [] (double v) { return lc (v); }, [] (float v) { return std::tanh (v); }); }
        float softA (float x);
        static double lF (double x) { const double a = std::abs (x); return a <= 0.8 ? 0.5 * x * x : 0.32 + 0.8 * (a - 0.8) + 0.04 * lc ((a - 0.8) / 0.2); }
        float limA (float x) { return run (x, [] (double v) { return lF (v); }, [] (float v) { const float a = std::abs (v); return a <= 0.8f ? v : std::copysign (0.8f + 0.2f * std::tanh ((a - 0.8f) / 0.2f), v); }); }
    };
    Ad clipAd[2][4], compAd[2], limAd[2]; ja::BQ warmLp[2][2]; bool satAAok = false;
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
    Bq pu[4]; PultecSettings puLast, puSm; bool puInit = false, puAct = false; float puMix = 0, puMixS = 0, puDrive = 0.15f; juce::uint32 puTick = 0; AeColour puCol; Maxx maxx;
    // modulo EXTRA
    juce::AudioBuffer<float> xBuf; int xW = 0; float xLp[2] = {}, xHold[2] = {}; double xPh = 0, xLfo = 0, xDrift = 0, xDt = 0; juce::Random xr; int lastXType = -1;
    float accIn = 0, accOut = 0, accGr = 0, accTr = 0; int accN = 0;
public:
    std::atomic<int> order[5] { 1, 2, 3, 4, 6 };   // moduli centrali riordinabili (1 shift, 2 comp, 3 sat, 4 space, 6 extra)
    std::atomic<double> bpm { 120.0 };
};
}
