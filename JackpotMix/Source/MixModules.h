#pragma once
// Jackpot Mixology: catalogo dei moduli del rack e motori
#include "MixPreEq.h"
#include "MixFx.h"
#include "CompDSP.h"
#include "ClipDSP.h"
#include "TubeDSP.h"
#include "DriveModels.h"
#include "MotionDSP.h"
#include "MasterColor.h"

namespace jmx
{
using jpre::PSpec;
using namespace jpre;   // F_ e funzioni dei comandi
constexpr int kSlots = 10, kP = 16;

enum Cat { C_PRE, C_EQ, C_DEESS, C_SAT, C_COMP, C_CLIP, C_REV, C_DLY, C_DIST, C_MOD, C_WIDE, C_TAPE, C_DYN, C_UTIL, kCats };
inline const char* catName (int c)
{
    static const char* n[] = { "PREAMP", "EQ", "DE-ESSER", "SATURATORI", "COMPRESSORI", "CLIPPER E LIMITER", "RIVERBERI", "DELAY", "DISTORSIONI", "MODULAZIONE", "WIDER", "NASTRO", "GATE E TRANSIENTI", "UTILITY" };
    return n[juce::jlimit (0, kCats - 1, c)];
}
inline juce::Colour catColour (int c)
{
    static const juce::uint32 k[] = { 0xffd9a441, 0xff7fb8d9, 0xffc9d97f, 0xffe0703c, 0xff6fcf97, 0xffef5b5b, 0xff9b8cff, 0xff4fd1c5, 0xffff6b9d, 0xffb98cff, 0xff6ec1ff, 0xffc8a27a, 0xffa0d468, 0xffaab4c0 };
    return juce::Colour (k[juce::jlimit (0, kCats - 1, c)]);
}
enum Kind { K_NONE, K_PRE, K_CEQ, K_DS, K_DECAP, K_TUBE, K_MBSAT, K_COMP, K_CLIP, K_LIM, K_REV, K_SPRING, K_CONV, K_DLY, K_DIST, K_MOD, K_WIDE, K_TAPE, K_GATE, K_TRANS, K_UTIL };
// aspetto del frontale: ogni modulo riconoscibile come il suo hardware
struct Look
{
    juce::uint32 plate = 0xffd9d3c4, ink = 0xff2a2620, accent = 0xffd8452f;
    int knob = 2;        // 0 rossa, 1 grigia, 2 nera
    int meter = 0;       // 0 niente, 1 VU riduzione, 2 VU uscita, 3 LED riduzione
    bool dark = false;   // frontale scuro (testi chiari)
    int screws = 4;
};
struct ModDef
{
    juce::String name, inspired, desc;
    int cat = 0, kind = K_NONE, sub = 0;
    Look look; std::vector<PSpec> spec;
    float defMix = 1, wetGain = 1; bool send = false;   // send: il dry resta intero e MIX dosa solo l'effetto (riverberi, delay)
    std::function<juce::String (int, float)> custom;   // testo dei comandi F_CUSTOM (indice, valore 0..1)
};
const std::vector<ModDef>& mods();
inline const ModDef& mod (int t) { return mods()[(size_t) juce::jlimit (0, (int) mods().size() - 1, t)]; }
int findMod (const juce::String& name);
juce::String paramText (int type, int i, float v);

// ----------------------------------------------------------------------- motore di un modulo
struct Ctx { const float* scL = nullptr; const float* scR = nullptr; double bpm = 120, ppq = 0; bool playing = false; };
class Engine
{
public:
    virtual ~Engine() = default;
    virtual void prepare (double sr, int maxBlock) = 0;
    virtual void process (float* L, float* R, int n, const float* v, const Ctx& c) = 0;   // v: comandi 0..1
    virtual int latency (const float*) const { return 0; }
    virtual void setUserIr (juce::AudioBuffer<float>, double) {}
    std::atomic<float> gr { 0 }, out { 0 };   // riduzione (dB) e livello d'uscita per i meter
    int type = 0;
    float rv (const float* v, int i) const { return specReal (mod (type).spec[(size_t) i], v[i]); }
};
std::unique_ptr<Engine> makeEngine (int type);
}
