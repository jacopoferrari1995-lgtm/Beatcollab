#include "ButterDSP.h"
#include <complex>

namespace jb
{
using namespace juce;
static const StringArray kNoise { "WHITE", "PINK", "METALLO", "SABBIA" };   // vinile e nastro sono il lavoro di Dust
static const StringArray kFocus { "FULL", "LOW", "MID", "HIGH" };
static const StringArray kComp { "PUNCH", "WARM", "GLUE", "GENTLE", "SMACK", "OTT" };
static const StringArray kSat { "CONSOLE", "4 TRACK", "TAPE", "TUBE", "LO-FI", "FLUB", "OVERDRIVE", "DISTORSORE", "FUZZ" };
static const StringArray kSpace { "ROOM", "HALL", "PLATE", "SPRING" };
static const StringArray kStages { "1", "2", "3", "4" };

void Burro::addParams (AudioProcessorValueTreeState::ParameterLayout& l)
{
    auto F = [&l] (const char* id, const char* n, float a, float b, float d) { l.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, n, NormalisableRange<float> (a, b), d)); };
    auto B = [&l] (const char* id, const char* n, bool d) { l.add (std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, n, d)); };
    auto C = [&l] (const char* id, const char* n, const StringArray& c, int d) { l.add (std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, n, c, d)); };
    F ("u_mag", "Magnitude", 0, 1, 1);
    B ("u_bOn", "Boom On", true); F ("u_boom", "Boom", 0, 1, 0); F ("u_bTone", "Boom Tone", 30, 120, 55); B ("u_acid", "Boom Acid", false);
    F ("u_bDecay", "Boom Decay", 0, 1, 0.35f); F ("u_bPunch", "Boom Punch", 0, 1, 0);
    F ("u_noise", "Noise", 0, 1, 0); C ("u_nType", "Noise Type", kNoise, 1);
    B ("u_shOn", "Shift On", true); F ("u_shAmt", "Shift Mix", 0, 1, 0); F ("u_shPitch", "Shift Pitch", -12, 12, 0); F ("u_shForm", "Shift Formant", -12, 12, 0); F ("u_shTgt", "Shift Target", -1, 1, 0); C ("u_shFoc", "Shift Focus", kFocus, 0);
    B ("u_cOn", "Comp On", true); C ("u_cType", "Comp Type", kComp, 0); F ("u_cAmt", "Comp Amount", -1, 1, 0); F ("u_cBands", "Comp Bands", 0, 1, 0);
    B ("u_sOn", "Sat On", true); C ("u_sType", "Sat Type", kSat, 0); F ("u_sAmt", "Sat Amount", 0, 1, 0); F ("u_sStyle", "Sat Style", 0, 1, 0.5f); F ("u_sTgt", "Sat Target", -1, 1, 0); C ("u_sFoc", "Sat Focus", kFocus, 0);
    B ("u_spOn", "Space On", true); C ("u_spType", "Space Type", kSpace, 0); F ("u_spAmt", "Space Amount", 0, 1, 0); F ("u_spDecay", "Space Decay", 0, 1, 0.5f);
    F ("u_spPre", "Space Predelay", 0, 120, 10); F ("u_spDuck", "Space Duck", 0, 1, 0); F ("u_spWidth", "Space Width", 0, 1, 0.8f);
    B ("u_mOn", "Clip On", true); F ("u_mAmt", "Clip Drive", 0, 1, 0); C ("u_mStages", "Clip Stages", kStages, 1); F ("u_mTone", "Clip Tone", -1, 1, 0); F ("u_mTgt", "Clip Target", -1, 1, 0); C ("u_mFoc", "Clip Focus", kFocus, 0);
    B ("u_eqOn", "EQ On", true);
    {   // 8 bande, tutte spente: si accendono con un doppio clic sul grafico
        static const float df[kQBands] = { 30, 100, 300, 1000, 3000, 8000, 12000, 18000 };
        static const int dt[kQBands] = { jmx::BT_LCUT, jmx::BT_LSHELF, jmx::BT_BELL, jmx::BT_BELL, jmx::BT_BELL, jmx::BT_BELL, jmx::BT_HSHELF, jmx::BT_HCUT };
        for (int b = 0; b < kQBands; ++b)
        {
            const String P = "EQ " + String (b + 1) + " ";
            l.add (std::make_unique<AudioParameterBool> (ParameterID { qId (b, "on"), 1 }, P + "On", false));
            l.add (std::make_unique<AudioParameterChoice> (ParameterID { qId (b, "type"), 1 }, P + "Type", qTypeNames(), dt[b]));
            l.add (std::make_unique<AudioParameterChoice> (ParameterID { qId (b, "ch"), 1 }, P + "Channel", qChNames(), 0));
            l.add (std::make_unique<AudioParameterChoice> (ParameterID { qId (b, "slope"), 1 }, P + "Slope", StringArray { "12 dB/ott", "24 dB/ott", "48 dB/ott", "96 dB/ott" }, 1));
            l.add (std::make_unique<AudioParameterFloat> (ParameterID { qId (b, "f"), 1 }, P + "Freq", NormalisableRange<float> (20, 20000, 0, 0.25f), df[b], AudioParameterFloatAttributes().withLabel ("Hz")));
            l.add (std::make_unique<AudioParameterFloat> (ParameterID { qId (b, "g"), 1 }, P + "Gain", NormalisableRange<float> (-18, 18, 0.1f), 0, AudioParameterFloatAttributes().withLabel ("dB")));
            l.add (std::make_unique<AudioParameterFloat> (ParameterID { qId (b, "q"), 1 }, P + "Q", NormalisableRange<float> (0.1f, 18, 0, 0.3f), 0.9f));
            l.add (std::make_unique<AudioParameterBool> (ParameterID { qId (b, "dyn"), 1 }, P + "Dynamic", false));
            l.add (std::make_unique<AudioParameterFloat> (ParameterID { qId (b, "thr"), 1 }, P + "Threshold", NormalisableRange<float> (-60, 0, 0.1f), -30, AudioParameterFloatAttributes().withLabel ("dB")));
            l.add (std::make_unique<AudioParameterFloat> (ParameterID { qId (b, "rng"), 1 }, P + "Range", NormalisableRange<float> (-18, 18, 0.1f), -6, AudioParameterFloatAttributes().withLabel ("dB")));
        }
    }
    B ("u_pOn", "Pultec On", true); F ("u_pLB", "Pultec LF Boost", 0, 10, 0); F ("u_pLA", "Pultec LF Atten", 0, 10, 0);
    C ("u_pLF", "Pultec LF Freq", StringArray { "20", "30", "60", "100" }, 2); F ("u_pHB", "Pultec HF Boost", 0, 10, 0); F ("u_pBW", "Pultec Bandwidth", 0, 10, 5);
    C ("u_pHF", "Pultec HF Freq", StringArray { "3k", "4k", "5k", "8k", "10k", "12k", "16k" }, 4); F ("u_pHA", "Pultec HF Atten", 0, 10, 0); C ("u_pAS", "Pultec Atten Sel", StringArray { "5k", "10k", "20k" }, 1);
    C ("u_limType", "Limite", StringArray { "CLIP", "MAXX" }, 0);
    B ("u_xOn", "Extra On", true); C ("u_xType", "Extra Type", StringArray { "DELAY", "BITCRUSH", "WOBBLE" }, 0);
    F ("u_xAmt", "Extra Amount", 0, 1, 0); F ("u_xP1", "Extra P1", 0, 1, 0.5f); F ("u_xP2", "Extra P2", 0, 1, 0.4f); F ("u_xTgt", "Extra Target", -1, 1, 0);
    auto FS = [&l] (const char* id, const char* n, float a, float b, float d, float skew) { l.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, n, NormalisableRange<float> (a, b, 0, skew), d)); };
    FS ("u_spLo", "Space Low Cut", 20, 1000, 20, 0.4f); FS ("u_spHi", "Space High Cut", 1000, 20000, 20000, 0.5f);
    FS ("u_xLo", "Extra Low Cut", 20, 1000, 20, 0.4f); FS ("u_xHi", "Extra High Cut", 1000, 20000, 20000, 0.5f);
    B ("u_iOn", "Imager On", true); F ("u_iWidth", "Imager Width", 0, 2, 1); FS ("u_iMono", "Imager Mono Bass", 20, 400, 20, 0.5f);
    F ("u_tone", "Master Tone", -1, 1, 0); F ("u_air", "Master Air", 0, 1, 0); F ("u_att", "Master Attack", -1, 1, 0); F ("u_sus", "Master Sustain", -1, 1, 0); B ("u_clip", "Master Clip", false);
}

const char* qId (int band, const char* key)
{
    static const char* keys[10] = { "on", "type", "ch", "slope", "f", "g", "q", "dyn", "thr", "rng" };
    static const std::vector<std::string> ids = []
    {
        std::vector<std::string> v; for (int b = 0; b < kQBands; ++b) for (auto* k : keys) v.push_back ("u_q" + std::to_string (b + 1) + "_" + k);
        return v;
    }();
    int ki = 0; for (int i = 0; i < 10; ++i) if (std::strcmp (keys[i], key) == 0) ki = i;
    return ids[(size_t) (jlimit (0, kQBands - 1, band) * 10 + ki)].c_str();
}
const StringArray& qTypeNames() { static const StringArray n { "CAMPANA", "SHELF BASSI", "SHELF ALTI", "TAGLIO BASSI", "TAGLIO ALTI", "NOTCH", "PASSA BANDA", "TILT" }; return n; }
const StringArray& qChNames() { static const StringArray n { "STEREO", "MID", "SIDE", "SINISTRO", "DESTRO" }; return n; }
void aeToBands (const AeSettings& S, const std::function<void (const char*, float)>& set)
{
    int b = 0;
    for (int k = 0; k < 4; ++k)
    {
        int t; float f, g, q; bool act; aeBandParams (S, k, t, f, g, q, act);
        if (! act) continue;
        const int nt = t == Bq::LSH ? jmx::BT_LSHELF : t == Bq::HSH ? jmx::BT_HSHELF : t == Bq::HP ? jmx::BT_LCUT : t == Bq::LP ? jmx::BT_HCUT : jmx::BT_BELL;
        set (qId (b, "on"), 1); set (qId (b, "type"), (float) nt); set (qId (b, "f"), jlimit (20.0f, 20000.0f, f)); set (qId (b, "g"), jlimit (-18.0f, 18.0f, g));
        set (qId (b, "q"), nt == jmx::BT_LCUT ? jlimit (0.3f, 3.0f, q / 0.7071f) : jlimit (0.1f, 18.0f, q)); set (qId (b, "slope"), nt == jmx::BT_LCUT ? 0.0f : 1.0f);
        set (qId (b, "dyn"), 0); set (qId (b, "ch"), 0);
        ++b;
    }
}

// tanh veloce (Pade 3/3, liscia: a +-3 vale 1 con derivata 0)
static inline float ftanh (float x) { x = juce::jlimit (-3.0f, 3.0f, x); const float x2 = x * x; return x * (27 + x2) / (27 + 9 * x2); }

// clip duro a 0.9 con un ginocchio stretto: stesso suono, spigolo arrotondato (meno aliasing)
static inline float softClip (float x)
{
    const float t = 0.9f, k = 0.15f, a = std::abs (x);
    if (a <= t - k) return x;
    if (a >= t + k) return std::copysign (t, x);
    const float d = a - (t - k); return std::copysign (a - d * d / (4 * k), x);
}

float Burro::Ad::softA (float x) { return run (x, [] (double v) { return sF (v); }, [] (float v) { return softClip (v); }); }

const std::vector<BurroPreset>& burroPresets()
{
    // u_cType: 0 PUNCH, 1 WARM, 2 GLUE, 3 GENTLE, 4 SMACK, 5 OTT  -  u_sType: 0 CONSOLE, 1 4 TRACK, 2 TAPE, 3 TUBE, 4 DIGITAL, 5 FLUB, 6 OVERDRIVE, 7 DISTORSORE, 8 FUZZ
    // u_sFoc / u_mFoc / u_shFoc: 0 FULL, 1 LOW, 2 MID, 3 HIGH  -  u_spType: 0 ROOM, 1 HALL, 2 PLATE, 3 SPRING  -  u_xType: 0 DELAY, 1 BITCRUSH, 2 WOBBLE
    // u_aeModel: 0 MAAG, 1 SSL, 2 NEVE 1084, 3 LINDELL 81, 4 73, 5 PE 1C  -  u_pLF: 20/30/60/100  -  u_pHF: 3k/4k/5k/8k/10k/12k/16k  -  u_limType: 0 CLIP, 1 MAXX
    static const std::vector<BurroPreset> P = {
        { "Init", {} },
        // ================= 808
        { "808 | Sub pieno e mono", { { "u_cType", 0 }, { "u_cAmt", 0.35f }, { "u_sType", 2 }, { "u_sAmt", 0.3f }, { "u_sFoc", 1 }, { "u_iMono", 120 }, { "u_iWidth", 0.9f },
                                     { "u_pLB", 4 }, { "u_pLA", 2 }, { "u_pLF", 2 } } },
        { "808 | Si sente sul telefono", { { "u_sType", 3 }, { "u_sAmt", 0.45f }, { "u_sStyle", 0.6f }, { "u_sFoc", 2 }, { "u_cType", 2 }, { "u_cAmt", 0.3f }, { "u_iMono", 150 },
                                          { "u_aeModel", 4 }, { "u_ae2g", 3 }, { "u_ae2f", 0.6f }, { "u_aeDrive", 0.4f } } },
        { "808 | Distorto trap", { { "u_sType", 7 }, { "u_sAmt", 0.4f }, { "u_sStyle", 0.4f }, { "u_mAmt", 0.3f }, { "u_mStages", 0 }, { "u_cType", 0 }, { "u_cAmt", 0.3f },
                                  { "u_iMono", 150 }, { "u_clip", 1 }, { "u_limType", 1 } } },
        { "808 | Drill scivolato", { { "u_sType", 6 }, { "u_sAmt", 0.35f }, { "u_cType", 4 }, { "u_cAmt", 0.3f }, { "u_aeModel", 2 }, { "u_aeDrive", 0.4f }, { "u_ae1g", 2 }, { "u_ae1f", 0.33f }, { "u_iMono", 150 } } },
        { "808 | Boom rinforzato", { { "u_boom", 0.45f }, { "u_bTone", 45 }, { "u_bDecay", 0.6f }, { "u_cType", 2 }, { "u_cAmt", 0.3f }, { "u_iMono", 140 } } },
        { "808 | Punch sull'attacco", { { "u_bPunch", 0.45f }, { "u_att", 0.4f }, { "u_cType", 0 }, { "u_cAmt", 0.35f }, { "u_sType", 0 }, { "u_sAmt", 0.25f }, { "u_sTgt", 0.7f }, { "u_iMono", 140 } } },
        { "808 | Fuzz phonk", { { "u_sType", 8 }, { "u_sAmt", 0.4f }, { "u_sStyle", 0.5f }, { "u_mAmt", 0.35f }, { "u_mStages", 1 }, { "u_cType", 4 }, { "u_cAmt", 0.4f },
                               { "u_aeModel", 4 }, { "u_ae1g", 3 }, { "u_ae1f", 0.33f }, { "u_aeDrive", 0.6f }, { "u_iMono", 160 } } },
        { "808 | Pulito per la cassa sopra", { { "u_cType", 3 }, { "u_cAmt", 0.3f }, { "u_pLA", 3 }, { "u_pLF", 1 }, { "u_sus", -0.15f }, { "u_iMono", 160 } } },
        { "808 | CHICCA Acid che urla", { { "u_boom", 0.4f }, { "u_acid", 1 }, { "u_bTone", 55 }, { "u_sType", 3 }, { "u_sAmt", 0.3f }, { "u_cType", 0 }, { "u_cAmt", 0.25f }, { "u_iMono", 140 } } },
        { "808 | CHICCA Ottava sopra per i telefoni", { { "u_shAmt", 0.3f }, { "u_shPitch", 12 }, { "u_shFoc", 2 }, { "u_sType", 3 }, { "u_sAmt", 0.25f }, { "u_sFoc", 2 }, { "u_iMono", 150 } } },
        // ================= BASSO ANALOGICO
        { "Basso analog | Moog rotondo", { { "u_sType", 2 }, { "u_sAmt", 0.35f }, { "u_sStyle", 0.5f }, { "u_cType", 1 }, { "u_cAmt", 0.35f }, { "u_aeModel", 2 }, { "u_ae1g", 2 }, { "u_ae1f", 0.33f }, { "u_aeDrive", 0.5f }, { "u_iMono", 120 } } },
        { "Basso analog | Mini con morso", { { "u_sType", 3 }, { "u_sAmt", 0.45f }, { "u_cType", 0 }, { "u_cAmt", 0.35f }, { "u_pLB", 3 }, { "u_pLA", 2 }, { "u_pLF", 2 }, { "u_pHB", 2 }, { "u_pHF", 3 }, { "u_iMono", 120 } } },
        { "Basso analog | Reese largo", { { "u_sType", 6 }, { "u_sAmt", 0.3f }, { "u_cType", 2 }, { "u_cAmt", 0.35f }, { "u_iWidth", 1.4f }, { "u_iMono", 180 }, { "u_spType", 0 }, { "u_spAmt", 0.08f }, { "u_spLo", 400 } } },
        { "Basso analog | Funk compresso", { { "u_cType", 4 }, { "u_cAmt", 0.45f }, { "u_att", 0.25f }, { "u_sType", 0 }, { "u_sAmt", 0.2f }, { "u_aeModel", 1 }, { "u_ae2g", 2 }, { "u_ae2f", 0.5f }, { "u_iMono", 120 } } },
        { "Basso analog | Sub sinusoide caldo", { { "u_sType", 2 }, { "u_sAmt", 0.3f }, { "u_sFoc", 1 }, { "u_cType", 3 }, { "u_cAmt", 0.3f }, { "u_pLB", 4 }, { "u_pLA", 3 }, { "u_pLF", 1 }, { "u_iMono", 140 } } },
        { "Basso analog | CHICCA Formante che parla", { { "u_shAmt", 0.5f }, { "u_shForm", -4 }, { "u_shFoc", 2 }, { "u_sType", 3 }, { "u_sAmt", 0.3f }, { "u_cType", 1 }, { "u_cAmt", 0.3f }, { "u_iMono", 120 } } },
        // ================= BASSO DIGITALE
        { "Basso digitale | FM pulito e deciso", { { "u_cType", 0 }, { "u_cAmt", 0.3f }, { "u_aeModel", 0 }, { "u_ae4g", 2.5f }, { "u_ae4f", 0.75f }, { "u_iMono", 120 } } },
        { "Basso digitale | Wavetable aggressivo", { { "u_sType", 7 }, { "u_sAmt", 0.35f }, { "u_cType", 5 }, { "u_cAmt", 0.35f }, { "u_cBands", 1 }, { "u_mAmt", 0.25f }, { "u_iMono", 150 } } },
        { "Basso digitale | Growl", { { "u_sType", 8 }, { "u_sAmt", 0.35f }, { "u_sFoc", 2 }, { "u_cType", 5 }, { "u_cAmt", 0.45f }, { "u_cBands", 0.8f }, { "u_iWidth", 1.2f }, { "u_iMono", 160 } } },
        { "Basso digitale | Bitcrush ma il sub resta", { { "u_xType", 1 }, { "u_xAmt", 0.35f }, { "u_xP1", 0.5f }, { "u_xP2", 0.3f }, { "u_xLo", 200 }, { "u_cType", 2 }, { "u_cAmt", 0.3f }, { "u_iMono", 150 } } },
        { "Basso digitale | Pluck con delay", { { "u_xType", 0 }, { "u_xAmt", 0.2f }, { "u_xP1", 0.33f }, { "u_xP2", 0.3f }, { "u_xLo", 300 }, { "u_cType", 0 }, { "u_cAmt", 0.3f }, { "u_iMono", 120 } } },
        { "Basso digitale | CHICCA Wobble cassetta", { { "u_xType", 2 }, { "u_xAmt", 0.4f }, { "u_xP1", 0.2f }, { "u_xP2", 0.3f }, { "u_xLo", 150 }, { "u_sType", 2 }, { "u_sAmt", 0.3f }, { "u_iMono", 130 } } },
        // ================= VOCE
        { "Voce | Rap davanti", { { "u_cType", 0 }, { "u_cAmt", 0.45f }, { "u_sType", 0 }, { "u_sAmt", 0.25f }, { "u_air", 0.25f }, { "u_spType", 2 }, { "u_spAmt", 0.12f }, { "u_spDecay", 0.3f }, { "u_spLo", 400 } } },
        { "Voce | Trap melodica", { { "u_cType", 5 }, { "u_cAmt", 0.3f }, { "u_sType", 2 }, { "u_sAmt", 0.2f }, { "u_air", 0.3f }, { "u_xType", 0 }, { "u_xAmt", 0.15f }, { "u_xP1", 0.17f }, { "u_xP2", 0.35f }, { "u_xLo", 400 },
                                   { "u_spType", 2 }, { "u_spAmt", 0.12f }, { "u_spLo", 400 } } },
        { "Voce | Drill scura", { { "u_cType", 4 }, { "u_cAmt", 0.4f }, { "u_sType", 3 }, { "u_sAmt", 0.3f }, { "u_sFoc", 2 }, { "u_tone", -0.2f }, { "u_spType", 0 }, { "u_spAmt", 0.08f } } },
        { "Voce | Ritornello largo", { { "u_cType", 2 }, { "u_cAmt", 0.35f }, { "u_xType", 0 }, { "u_xAmt", 0.2f }, { "u_xP1", 0.67f }, { "u_xP2", 0.35f }, { "u_xLo", 400 }, { "u_xHi", 5000 },
                                      { "u_spType", 1 }, { "u_spAmt", 0.12f }, { "u_iWidth", 1.2f } } },
        { "Voce | Intima asciutta", { { "u_cType", 3 }, { "u_cAmt", 0.4f }, { "u_sType", 2 }, { "u_sAmt", 0.15f }, { "u_tone", 0.1f }, { "u_spType", 0 }, { "u_spAmt", 0.08f }, { "u_spDecay", 0.2f } } },
        { "Voce | Pultec aria e corpo", { { "u_pLB", 2 }, { "u_pLA", 1 }, { "u_pLF", 3 }, { "u_pHB", 4 }, { "u_pHF", 5 }, { "u_pBW", 6 }, { "u_cType", 3 }, { "u_cAmt", 0.35f } } },
        { "Voce | Adlib filtrati", { { "u_sType", 1 }, { "u_sAmt", 0.4f }, { "u_xType", 0 }, { "u_xAmt", 0.3f }, { "u_xP1", 0.33f }, { "u_xP2", 0.4f }, { "u_iWidth", 1.4f }, { "u_aeModel", 4 }, { "u_ae4f", 1.0f } } },
        { "Voce | Plate anni 80", { { "u_spType", 2 }, { "u_spAmt", 0.3f }, { "u_spDecay", 0.55f }, { "u_spPre", 40 }, { "u_spLo", 500 }, { "u_spHi", 10000 }, { "u_cType", 2 }, { "u_cAmt", 0.3f }, { "u_air", 0.2f } } },
        { "Voce | Slap delay", { { "u_xType", 0 }, { "u_xAmt", 0.22f }, { "u_xP1", 0.0f }, { "u_xP2", 0.15f }, { "u_cType", 3 }, { "u_cAmt", 0.4f }, { "u_spType", 2 }, { "u_spAmt", 0.15f }, { "u_air", 0.25f } } },
        { "Voce | CHICCA Coro all'ottava sulle code", { { "u_shAmt", 0.35f }, { "u_shPitch", 12 }, { "u_shTgt", -0.6f }, { "u_spType", 1 }, { "u_spAmt", 0.15f }, { "u_iWidth", 1.4f }, { "u_cType", 2 }, { "u_cAmt", 0.3f } } },
        { "Voce | CHICCA Mostro all'ottava sotto", { { "u_shAmt", 0.55f }, { "u_shPitch", -12 }, { "u_shForm", 3 }, { "u_sType", 3 }, { "u_sAmt", 0.25f }, { "u_cType", 4 }, { "u_cAmt", 0.35f } } },
        { "Voce | CHICCA Telefono dal futuro", { { "u_sType", 4 }, { "u_sAmt", 0.3f }, { "u_sStyle", 0.6f }, { "u_iWidth", 0.0f }, { "u_aeModel", 4 }, { "u_ae4f", 1.0f }, { "u_pHA", 10.0f }, { "u_pAS", 0 },
                                                  { "u_xType", 0 }, { "u_xAmt", 0.2f }, { "u_xP1", 0.5f }, { "u_xP2", 0.45f }, { "u_xLo", 600 } } },
        { "Voce | CHICCA Molla dub", { { "u_spType", 3 }, { "u_spAmt", 0.3f }, { "u_spDecay", 0.5f }, { "u_spPre", 20 }, { "u_xType", 0 }, { "u_xAmt", 0.3f }, { "u_xP1", 0.5f }, { "u_xP2", 0.55f }, { "u_xLo", 300 }, { "u_xHi", 4000 } } },
        // ================= MASTER
        { "Master | Bus pronto (MAXX)", { { "u_cType", 2 }, { "u_cAmt", 0.3f }, { "u_sType", 0 }, { "u_sAmt", 0.2f }, { "u_pLB", 3 }, { "u_pLA", 2 }, { "u_pHB", 2.5f }, { "u_pHF", 4 }, { "u_aeModel", 1 }, { "u_aeDrive", 0.3f },
                                         { "u_iMono", 120 }, { "u_clip", 1 }, { "u_limType", 1 } } },
        { "Master | Trap moderna", { { "u_boom", 0.25f }, { "u_bTone", 45 }, { "u_bPunch", 0.3f }, { "u_cType", 0 }, { "u_cAmt", 0.35f }, { "u_sType", 2 }, { "u_sAmt", 0.2f }, { "u_aeModel", 0 }, { "u_ae4g", 3 }, { "u_ae4f", 0.75f },
                                     { "u_iMono", 130 }, { "u_clip", 1 }, { "u_limType", 1 } } },
        { "Master | Boom bap pieno", { { "u_cType", 1 }, { "u_cAmt", 0.4f }, { "u_sType", 1 }, { "u_sAmt", 0.35f }, { "u_aeModel", 4 }, { "u_ae1g", 3 }, { "u_ae1f", 0.33f }, { "u_aeDrive", 0.5f }, { "u_clip", 1 }, { "u_limType", 1 } } },
        { "Master | Pultec e colla", { { "u_pLB", 3 }, { "u_pLA", 2 }, { "u_pLF", 2 }, { "u_pHB", 2 }, { "u_pHF", 4 }, { "u_pBW", 6 }, { "u_cType", 2 }, { "u_cAmt", 0.25f }, { "u_clip", 1 }, { "u_limType", 1 } } },
        { "Master | Neve calda", { { "u_aeModel", 2 }, { "u_ae1g", 2 }, { "u_ae1f", 0.33f }, { "u_ae3g", 1.5f }, { "u_ae3f", 0.5f }, { "u_aeDrive", 0.5f }, { "u_cType", 2 }, { "u_cAmt", 0.2f } } },
        { "Master | Loud e pulito", { { "u_cType", 2 }, { "u_cAmt", 0.3f }, { "u_mAmt", 0.2f }, { "u_mStages", 0 }, { "u_clip", 1 }, { "u_limType", 1 }, { "u_iMono", 120 } } },
        { "Master | Drill scuro", { { "u_tone", -0.2f }, { "u_cType", 4 }, { "u_cAmt", 0.25f }, { "u_sType", 3 }, { "u_sAmt", 0.15f }, { "u_iMono", 140 }, { "u_clip", 1 }, { "u_limType", 1 } } },
        { "Master | CHICCA Largo sopra, mono sotto", { { "u_iWidth", 1.25f }, { "u_iMono", 140 }, { "u_air", 0.2f }, { "u_cType", 3 }, { "u_cAmt", 0.2f }, { "u_clip", 1 }, { "u_limType", 1 } } },
        { "Master | CHICCA Solo i transienti in avanti", { { "u_att", 0.35f }, { "u_sus", -0.1f }, { "u_sType", 0 }, { "u_sAmt", 0.2f }, { "u_sTgt", 0.8f }, { "u_cType", 2 }, { "u_cAmt", 0.2f }, { "u_clip", 1 }, { "u_limType", 1 } } },
        // ================= BATTERIA
        { "Batteria | Trap knock", { { "u_boom", 0.35f }, { "u_bTone", 50 }, { "u_cType", 0 }, { "u_cAmt", 0.45f }, { "u_sType", 0 }, { "u_sAmt", 0.3f }, { "u_sTgt", 0.5f }, { "u_mAmt", 0.25f }, { "u_mStages", 1 }, { "u_att", 0.3f } } },
        { "Batteria | Drill crunch", { { "u_cType", 4 }, { "u_cAmt", 0.5f }, { "u_sType", 4 }, { "u_sAmt", 0.35f }, { "u_sStyle", 0.4f }, { "u_sTgt", 0.6f }, { "u_mAmt", 0.4f }, { "u_mStages", 2 }, { "u_att", 0.4f } } },
        { "Batteria | Glue leggera", { { "u_cType", 2 }, { "u_cAmt", 0.35f }, { "u_sType", 2 }, { "u_sAmt", 0.15f }, { "u_air", 0.15f } } },
        { "Batteria | OTT sparkle", { { "u_cType", 5 }, { "u_cAmt", 0.6f }, { "u_cBands", 1 }, { "u_air", 0.3f } } },
        { "Batteria | Parallel smack", { { "u_cType", 4 }, { "u_cAmt", 0.85f }, { "u_mAmt", 0.2f }, { "u_mag", 0.45f } } },
        { "Batteria | Punch moderno", { { "u_cType", 0 }, { "u_cAmt", 0.45f }, { "u_sType", 3 }, { "u_sAmt", 0.3f }, { "u_att", 0.35f }, { "u_sus", -0.1f }, { "u_boom", 0.15f }, { "u_bTone", 50 }, { "u_air", 0.2f } } },
        { "Batteria | Pultec completa", { { "u_pLB", 4 }, { "u_pLA", 2.5f }, { "u_pLF", 2 }, { "u_pHB", 3 }, { "u_pHF", 3 }, { "u_pBW", 6 }, { "u_cType", 2 }, { "u_cAmt", 0.35f }, { "u_sType", 2 }, { "u_sAmt", 0.2f } } },
        { "Batteria | Cassa che sfonda", { { "u_boom", 0.45f }, { "u_bTone", 48 }, { "u_bDecay", 0.3f }, { "u_bPunch", 0.5f }, { "u_cType", 0 }, { "u_cAmt", 0.35f }, { "u_clip", 1 }, { "u_limType", 1 } } },
        { "Batteria | Rullante crack", { { "u_cType", 4 }, { "u_cAmt", 0.45f }, { "u_sType", 0 }, { "u_sAmt", 0.35f }, { "u_sTgt", 0.7f }, { "u_air", 0.35f } } },
        { "Batteria | Rullante grosso (formante giu)", { { "u_shAmt", 0.6f }, { "u_shForm", -5 }, { "u_shTgt", 0.3f }, { "u_shFoc", 2 }, { "u_cType", 0 }, { "u_cAmt", 0.3f } } },
        { "Batteria | Hat corti e secchi", { { "u_att", 0.4f }, { "u_sus", -0.6f }, { "u_tone", 0.2f } } },
        { "Batteria | CHICCA Riverbero solo sulle code", { { "u_spType", 2 }, { "u_spAmt", 0.35f }, { "u_spDecay", 0.6f }, { "u_spDuck", 0.8f }, { "u_spLo", 300 }, { "u_cType", 2 }, { "u_cAmt", 0.3f } } },
        // ================= EQ E STRUMENTI
        { "EQ | Maag aria sul bus", { { "u_aeModel", 0 }, { "u_ae4g", 4 }, { "u_ae4f", 0.75f }, { "u_ae1g", 2 }, { "u_ae1f", 0.0f } } },
        { "EQ | 73 in faccia", { { "u_aeModel", 4 }, { "u_ae1g", 4 }, { "u_ae1f", 0.33f }, { "u_ae2g", 3 }, { "u_ae2f", 0.6f }, { "u_ae3g", 3 }, { "u_ae4f", 0.25f }, { "u_aeDrive", 0.75f } } },
        { "EQ | Lindell 81 scolpito", { { "u_aeModel", 3 }, { "u_ae1g", 2.5f }, { "u_ae1f", 0.2f }, { "u_ae2g", -4 }, { "u_ae2f", 0.6f }, { "u_ae3g", 2.5f }, { "u_ae3f", 0.6f }, { "u_ae4g", 2 }, { "u_ae4f", 0.6f } } },
        { "Chitarra | Elettrica crunch", { { "u_sType", 3 }, { "u_sAmt", 0.45f }, { "u_sStyle", 0.6f }, { "u_cType", 1 }, { "u_cAmt", 0.35f }, { "u_spType", 0 }, { "u_spAmt", 0.15f } } },
        { "Chitarra | Clean con delay", { { "u_xType", 0 }, { "u_xAmt", 0.25f }, { "u_xP1", 0.67f }, { "u_xP2", 0.35f }, { "u_xLo", 300 }, { "u_xHi", 5000 }, { "u_cType", 3 }, { "u_cAmt", 0.3f }, { "u_spType", 1 }, { "u_spAmt", 0.12f } } } };
    return P;
}

void Burro::applyPreset (AudioProcessorValueTreeState& s, int index)
{
    const auto& all = burroPresets(); if (index < 0 || index >= (int) all.size()) return;
    for (auto* p : s.processor.getParameters())
        if (auto* rp = dynamic_cast<RangedAudioParameter*> (p)) if (rp->getParameterID().startsWith ("u_")) rp->setValueNotifyingHost (rp->getDefaultValue());
    AeSettings ae; bool hasAe = false;
    for (auto& [id, v] : all[(size_t) index].p)
    {
        const String sid (id);
        if (sid.startsWith ("u_ae"))
        {   // i preset scritti per il vecchio EQ analogico
            hasAe = true;
            if (sid == "u_aeModel") ae.model = (int) v; else if (sid == "u_aeQ") ae.q = v; else if (sid == "u_aeDrive") ae.drive = v;
            else { const int k = sid.substring (4, 5).getIntValue() - 1; if (k >= 0 && k < 4) { if (sid.endsWith ("g")) ae.g[k] = v; else ae.f[k] = v; } }
            continue;
        }
        if (auto* rp = s.getParameter (id)) rp->setValueNotifyingHost (rp->convertTo0to1 (v));
    }
    if (hasAe) aeToBands (ae, [&s] (const char* id, float v) { if (auto* rp = s.getParameter (id)) rp->setValueNotifyingHost (rp->convertTo0to1 (v)); });
}

void Burro::bind (AudioProcessorValueTreeState& s)
{
    pkeys.clear(); pcache.clear();
    for (auto* p : s.processor.getParameters()) if (auto* rp = dynamic_cast<RangedAudioParameter*> (p)) pkeys.push_back (rp->paramID.toStdString());
    for (auto& k : pkeys) pcache.emplace (std::string_view (k), s.getRawParameterValue (k));
}
void Burro::prepare (double sampleRate, int maxBlock)
{
    sr = sampleRate;
    const int n = jmax (maxBlock, 4096);
    dryL.assign ((size_t) n, 0); dryR.assign ((size_t) n, 0); trW.assign ((size_t) n, 0); wetL.assign ((size_t) n, 0); wetR.assign ((size_t) n, 0);
    shBuf.setSize (2, 16384); shBuf.clear(); shW = 0;
    preBuf.setSize (2, (int) (sr * 0.25) + 8); preBuf.clear(); preW = 0;
    verb.prepare (sr); verb.reset(); lastSpType = -1;
    xBuf.setSize (2, (int) (sr * 2.2)); xBuf.clear(); xW = 0;
    form.prepare (sr); for (auto& x : lastAe) x = 99; puInit = false; peq.prepare (sr, jmax (64, maxBlock));
    const int mb = jmax (64, maxBlock);
    osSat = std::make_unique<juce::dsp::Oversampling<float>> (2, 3, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, true);
    osClip = std::make_unique<juce::dsp::Oversampling<float>> (2, 3, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, true);
    for (auto& d : drv) d.prepare (sampleRate * kOs, kOs); satLvl[0] = satLvl[1] = 0; satAg = 1; satAAok = false; for (auto& c : satAA) for (auto& f : c) f.reset(); for (auto& c : satLp8) for (auto& f : c) f.reset(); for (auto& c : clipLp8) for (auto& f : c) f.reset(); for (auto& c : clipAd) for (auto& a : c) a = {}; compAd[0] = compAd[1] = {}; limAd[0] = limAd[1] = {}; for (auto& w : warmLp) { w[0].lp (sampleRate, 3000, 0.5412); w[1].lp (sampleRate, 3000, 1.3066); w[0].reset(); w[1].reset(); }
    { static const double Q16[8] = { 0.5024, 0.5225, 0.5669, 0.6468, 0.7882, 1.0607, 1.7224, 5.1011 }; for (int c = 0; c < 2; ++c) for (int j = 0; j < 8; ++j) { clipLp8[c][j].lp (sampleRate * kOs, jmin (21500.0, sampleRate * 0.45), Q16[j]); satLp8[c][j].lp (sampleRate * kOs, jmin (21500.0, sampleRate * 0.45), Q16[j]); } }
    osSat->initProcessing ((size_t) mb); osClip->initProcessing ((size_t) mb);
    osLim = std::make_unique<juce::dsp::Oversampling<float>> (2, 2, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, true);
    osLim->initProcessing ((size_t) mb);
    // il passa-basso a 16 poli dentro l'8x ritarda anche lui (somma di 1/Q diviso la pulsazione di taglio): va contato, altrimenti la DAW sbaglia di 4 campioni
    const double lpGd = [&] { static const double Q16[8] = { 0.5024, 0.5225, 0.5669, 0.6468, 0.7882, 1.0607, 1.7224, 5.1011 }; double s = 0; for (double q : Q16) s += 1.0 / q;
                              return s / (MathConstants<double>::twoPi * jmin (21500.0, sampleRate * 0.45)) * sampleRate; }();
    latSat = (int) std::round (osSat->getLatencyInSamples() + lpGd); latClip = (int) std::round (osClip->getLatencyInSamples() + lpGd); latLim = (int) std::round (osLim->getLatencyInSamples()); osLat = latSat + latClip + latLim;
    gSat.prepare (latSat, mb); gClip.prepare (latClip, mb); gLim.prepare (latLim, mb);
    dryDl[0].prepare (mb + osLat + 8); dryDl[1].prepare (mb + osLat + 8); trDl.prepare (mb + osLat + 8);
    for (auto& r : rampLast) r = -1e9f;
    for (auto& a : fLo4) for (auto& x : a) x = 0; for (auto& a : fHi4) for (auto& x : a) x = 0;
}

static inline float onePole (float x, float& z, float a) { z = (1 - a) * x + a * z; return z; }
static inline float coef (double hz, double sr) { return (float) std::exp (-MathConstants<double>::twoPi * hz / sr); }
static inline float tgtW (float t, float tr) { return t >= 0 ? (1 - t) + t * tr : (1 + t) + (-t) * (1 - tr); }

void Burro::process (float* L, float* R, int N, AudioProcessorValueTreeState& s)
{
    // parametri: cercati per testo in una tabella fatta una volta sola (prima ogni lettura allocava memoria nel thread audio)
    auto v = [this, &s] (const char* id) -> float
    {
        const auto it = pcache.find (std::string_view (id));
        if (it != pcache.end()) return it->second->load();
        return s.getRawParameterValue (id)->load();   // non succede: la tabella contiene tutti i parametri
    };
    // valori che cambiano il suono di colpo: rampa lineare dal blocco precedente a questo (niente click quando li muovi)
    // ogni manopola scivola verso il valore nuovo con costante di 20 ms (esponenziale: uguale con qualunque grandezza di buffer)
    const float rk = std::exp (-(float) N / (0.02f * (float) sr));
    // un modulo resta acceso finche' la sua quantita' non e' scivolata davvero a zero; da spento riparte da zero
    auto live = [this, &v] (int slot, const char* id) { const bool l = std::abs (v (id)) > 0.001f || (rampLast[slot] > -1e8f && std::abs (rampLast[slot]) > 0.001f); if (! l) rampLast[slot] = 0; return l; };
    auto ramp = [this, &v, N, rk] (int slot, const char* id) { Ramp r; const float tg = v (id); r.a = rampLast[slot] < -1e8f ? tg : rampLast[slot]; float e = tg + (r.a - tg) * rk; if (std::abs (e - tg) < 1e-5f) e = tg; r.b = e; r.n = (float) N; rampLast[slot] = e; return r; };
    const auto magR = ramp (0, "u_mag");
    // rilevatore dei transienti
    const float fA = (float) std::exp (-1.0 / (0.0005 * sr)), fR = (float) std::exp (-1.0 / (0.03 * sr)), sA = (float) std::exp (-1.0 / (0.025 * sr)), sR = (float) std::exp (-1.0 / (0.15 * sr));
    for (int i = 0; i < N; ++i)
    {
        dryL[(size_t) i] = L[i]; dryR[(size_t) i] = R[i]; dryDl[0].push (L[i]); dryDl[1].push (R[i]);
        const float x = 0.5f * (std::abs (L[i]) + std::abs (R[i]));
        envF = x > envF ? fA * envF + (1 - fA) * x : fR * envF + (1 - fR) * x;
        envS = x > envS ? sA * envS + (1 - sA) * x : sR * envS + (1 - sR) * x;
        trW[(size_t) i] = jlimit (0.0f, 1.0f, (envF / (envS + 1e-5f) - 1.0f) * 0.6f);
        trDl.push (trW[(size_t) i]);
    }
    int lag = 0;   // quanto il segnale e' gia' in ritardo rispetto al rilevatore (oversampling dei moduli prima)
    auto trAt = [this, &lag, N] (int i) { return lag == 0 ? trW[(size_t) i] : trDl.read (lag, i, N); };
    float* ch[2] = { L, R };
    const float aLo = coef (200, sr), aHi = coef (3000, sr);
    auto focusSplit = [&] (int k, int c, float x, int foc, float& band) -> float   // ritorna il resto
    {
        const float lo = onePole (x, fLo[k][c], aLo), lph = onePole (x, fHi[k][c], aHi);
        if (foc == 0) { band = x; return 0.0f; }
        band = foc == 1 ? lo : foc == 3 ? x - lph : lph - lo; return x - band;
    };

    for (int step = 0; step < 7; ++step)
    {
        const int mod = step == 0 ? 0 : step == 6 ? 5 : order[step - 1].load();
        switch (mod)
        {
        case 0:
        {
    // ---- 1. BOOM / NOISE ----
    if (v ("u_bOn") > 0.5f)
    {
        const auto boomR = ramp (2, "u_boom"); float boom = boomR.b; const float noise = ramp (16, "u_noise").b; const bool acid = v ("u_acid") > 0.5f; const int nt = (int) v ("u_nType");
        const float bf = v ("u_bTone"), bDec = v ("u_bDecay"), punch = v ("u_bPunch");
        const float hpA = coef (7000, sr);
        const float thDec = (float) std::exp (-1.0 / ((0.04 + bDec * 0.45) * sr)), thPd = (float) std::exp (-1.0 / (0.014 * sr)), lowA = coef (140, sr);
        for (int i = 0; i < N; ++i)
        {
            const float tr = trW[(size_t) i]; boom = boomR.at (i);
            acidEnv = jmax (tr, acidEnv * 0.9995f);
            // IMPACT: su ogni colpo parte un sub accordato su FREQ che scende di pitch (thump) + spinta dei transienti sotto 140 Hz
            if (boom > 0 || punch > 0)
            {
                if (thArmed && tr > 0.5f) { thArmed = false; thPh = 0; thEnv = 1; thPitch = 1; }
                if (tr < 0.15f) thArmed = true;
                thEnv *= thDec; thPitch *= thPd;
                thPh += (float) (bf * (1.0 + 1.8 * thPitch) / sr); thPh -= std::floor (thPh);
                const float th = std::sin (MathConstants<float>::twoPi * thPh) * thEnv * boom * 0.55f;
                for (int c = 0; c < 2; ++c)
                {
                    const float lo = onePole (ch[c][i], bLow[c], lowA);
                    ch[c][i] += th + lo * punch * tr * 1.1f;
                }
            }
            if (boom > 0)
            {
                const float f = acid ? bf * (1.0f + acidEnv * 2.5f) : bf, q = acid ? 9.0f : 1.6f;
                const float g = (float) std::tan (MathConstants<double>::pi * f / sr), k = 1.0f / q;
                for (int c = 0; c < 2; ++c)
                {
                    const float x = ch[c][i], v3 = x - bIc2[c], v1 = (bIc1[c] + g * v3) / (1 + g * (g + k)), v2 = bIc2[c] + g * v1;
                    bIc1[c] = 2 * v1 - bIc1[c]; bIc2[c] = 2 * v2 - bIc2[c];
                    const float y = acid ? std::tanh (v1 * k * 3.0f) : v1 * k;
                    ch[c][i] += y * boom * 1.6f;
                }
            }
            if (noise > 0)
            {
                const float env = jmin (1.0f, envF * 2.0f) * noise * 0.5f;
                for (int c = 0; c < 2; ++c)
                {
                    const float w = rng.nextFloat() * 2 - 1; float n = w;
                    switch (nt)
                    {
                        case 1: { auto& b = pinkB[c]; b[0] = 0.99765f * b[0] + w * 0.099046f; b[1] = 0.963f * b[1] + w * 0.2965164f; b[2] = 0.57f * b[2] + w * 1.0526913f; n = (b[0] + b[1] + b[2] + w * 0.1848f) * 0.25f; break; }
                        case 2: { const float a = onePole (w, nBp1[c], coef (5200, sr)), b2 = onePole (a, nBp2[c], coef (6800, sr)); n = (a - b2) * 6.0f; break; }
                        case 3: n = (w - onePole (w, nLp[c], hpA)) * 1.4f; break;
                        default: break;
                    }
                    ch[c][i] += n * env;
                }
            }
        }
    }

        break;
        }
        case 1:
        {
    // ---- 2. SHIFT ----
    const bool shLive = v ("u_shOn") > 0.5f && live (1, "u_shAmt");
    if (! shLive)
    {   // spento: il buffer si tiene aggiornato, cosi' quando si alza la manopola non suona audio vecchio
        const int BL = shBuf.getNumSamples();
        for (int i = 0; i < N; ++i) { shBuf.setSample (0, shW, ch[0][i]); shBuf.setSample (1, shW, ch[1][i]); if (++shW >= BL) shW = 0; }
    }
    if (shLive)
    {
        const auto amtR = ramp (1, "u_shAmt"); const float tgt = ramp (17, "u_shTgt").b; const int foc = (int) v ("u_shFoc");
        const float fst = v ("u_shForm"); const bool useF = std::abs (fst) > 0.05f;
        form.ratio = (float) std::pow (2.0, (v ("u_shPitch") + fst) / 12.0);
        const double ratio = std::pow (2.0, v ("u_shPitch") / 12.0), W = 0.045 * sr; const int BL = shBuf.getNumSamples();
        for (int i = 0; i < N; ++i)
        {
            shPh += 1.0 - ratio; while (shPh < 0) shPh += W; while (shPh >= W) shPh -= W;
            const double d1 = shPh, d2 = std::fmod (shPh + W * 0.5, W);
            const float g1 = 0.5f - 0.5f * std::cos ((float) (MathConstants<double>::twoPi * d1 / W)), g2 = 1 - g1;
            const float w = amtR.at (i) * tgtW (tgt, trAt (i));
            float bnd[2], rst[2], ys[2];
            for (int c = 0; c < 2; ++c)
            {
                rst[c] = focusSplit (0, c, ch[c][i], foc, bnd[c]);
                shBuf.setSample (c, shW, bnd[c]);
                auto rd = [&] (double d) { double p = shW - d - 2; while (p < 0) p += BL; const int a = (int) p; const float fr = (float) (p - a); return shBuf.getSample (c, a) * (1 - fr) + shBuf.getSample (c, (a + 1) % BL) * fr; };
                ys[c] = rd (d1) * g1 + rd (d2) * g2;
            }
            if (useF)
            {   // formante indipendente dal pitch
                form.push (0.5f * (bnd[0] + bnd[1]), 0.5f * (ys[0] + ys[1]));
                for (int c = 0; c < 2; ++c) ys[c] = form.process (ys[c], c);
            }
            for (int c = 0; c < 2; ++c) ch[c][i] = rst[c] + bnd[c] + (ys[c] - bnd[c]) * w;
            if (++shW >= BL) shW = 0;
        }
    }

        break;
        }
        case 2:
        {
    // ---- 3. COMPRESS ----
    grMeter = grMeter.load() * 0.9f;
    if (v ("u_cOn") > 0.5f && live (11, "u_cAmt"))
    {
        const auto amtR = ramp (11, "u_cAmt"); const int type = (int) v ("u_cType"); const float bands = v ("u_cBands");
        struct CT { float att, rel, ratio; };
        static const CT T[] = { { 25, 90, 4 }, { 8, 250, 3 }, { 10, 200, 2 }, { 30, 300, 1.6f }, { 0.3f, 60, 20 }, { 2, 120, 6 } };
        const auto& t = T[jlimit (0, 5, type)];
        float down = 0, up = 0, thrD = 0, makeup = 0; const float thrU = -22.0f;
        auto setAmt = [&] (float amt) { down = type == 5 ? std::abs (amt) : jmax (0.0f, amt); up = type == 5 ? std::abs (amt) * 0.45f : jmax (0.0f, -amt); thrD = -down * 36.0f; makeup = down * 36.0f * (1.0f - 1.0f / t.ratio) * 0.35f; };
        const float cA = (float) std::exp (-1.0 / (t.att * 0.001 * sr)), cR = (float) std::exp (-1.0 / (t.rel * 0.001 * sr));
        const float x1 = coef (150, sr), x2 = coef (2500, sr);
        auto gainDb = [&] (float lev)
        {
            float g = 0;
            if (lev > thrD) g -= (lev - thrD) * (1.0f - 1.0f / t.ratio);
            if (up > 0 && lev < thrU && lev > -70.0f) g += jmin ((thrU - lev) * 0.6f, up * 18.0f);
            return g + makeup;
        };
        auto det = [&] (float& e, float x) { e = x > e ? cA * e + (1 - cA) * x : cR * e + (1 - cR) * x; return Decibels::gainToDecibels (e + 1e-7f); };
        float maxGr = 0;
        for (int i = 0; i < N; ++i)
        {
            setAmt (amtR.at (i));
            const float l = ch[0][i], r = ch[1][i];
            const float gS = gainDb (det (cEnv[0], jmax (std::abs (l), std::abs (r))));
            maxGr = jmax (maxGr, -jmin (0.0f, gS - makeup));
            float sl = l * Decibels::decibelsToGain (gS), sr2 = r * Decibels::decibelsToGain (gS);
            if (bands > 0.001f)
            {
                float bl[3], br[3];
                const float ll = onePole (l, xLo[0], x1), lh = onePole (l, xHi[0], x2), rl = onePole (r, xLo[1], x1), rh = onePole (r, xHi[1], x2);
                bl[0] = ll; bl[2] = l - lh; bl[1] = lh - ll; br[0] = rl; br[2] = r - rh; br[1] = rh - rl;
                float ml = 0, mr = 0;
                for (int b = 0; b < 3; ++b) { const float g = Decibels::decibelsToGain (gainDb (det (cEnv[1 + b], jmax (std::abs (bl[b]), std::abs (br[b]))))); ml += bl[b] * g; mr += br[b] * g; }
                sl += (ml - sl) * bands; sr2 += (mr - sr2) * bands;
            }
            if (type == 1)
            {   // WARM: il colore solo sotto i 3 kHz (gira a 1x: le armoniche degli acuti tornerebbero giu' come sporco), con antiderivata
                const float lL = warmLp[0][1] (warmLp[0][0] (sl)), lR = warmLp[1][1] (warmLp[1][0] (sr2));
                sl = (compAd[0].tanhA (lL * 1.2f) / 1.2f + (sl - lL)) * 1.15f; sr2 = (compAd[1].tanhA (lR * 1.2f) / 1.2f + (sr2 - lR)) * 1.15f;
            }
            ch[0][i] = sl; ch[1][i] = sr2;
        }
        grMeter = jmax (grMeter.load(), maxGr);
    }

        break;
        }
        case 3:
        {
    // ---- 4. SATURATE (a 8x: niente aliasing anche con fuzz e distorsore) ----
    {
        const bool on = v ("u_sOn") > 0.5f;
        const auto aR = ramp (10, "u_sAmt"), stR = ramp (14, "u_sStyle"), tgR = ramp (15, "u_sTgt");
        juce::dsp::AudioBlock<float> blk (ch, 2, (size_t) N);
        const bool act = on && (aR.a > 0.001f || aR.b > 0.001f);
        if (act && ! gSat.running) { osSat->reset(); gSat.running = true; gSat.warm = 2 * latSat + 8; }
        if (gSat.running)
        {
        for (int c = 0; c < 2; ++c) std::copy (ch[c], ch[c] + N, gSat.tmp[c].begin());
        // mono (sinistro = destro, il caso dei bassi e degli 808): si calcola un canale solo e si copia, stato compreso
        const bool mono = std::memcmp (ch[0], ch[1], sizeof (float) * (size_t) N) == 0; const int nc = mono ? 1 : 2;
        auto up4 = osSat->processSamplesUp (blk); const int N4 = (int) up4.getNumSamples(); float* u[2] = { up4.getChannelPointer (0), up4.getChannelPointer (1) };
        if (act)
        {
            // i modelli di Alter Drive (antiderivata contro l'aliasing, gia' misurati) al posto delle vecchie curve
            const double s4 = sr * kOs;
            const int type = jlimit (0, 8, (int) v ("u_sType")); const float st = stR.b, tgt = tgR.b; const int foc = (int) v ("u_sFoc");
            static const int kMap[9] = { ja::CONSOLE, ja::TAPE, ja::TAPE, ja::VALVOLA, ja::LOFI, ja::FOLD, ja::OVERDRIVE, ja::DISTORSIONE, ja::FUZZ };
            const int model = kMap[type];
            for (auto& d : drv) d.setModel (model);
            const float aB = aR.b; for (auto& d : drv) d.setDrive (aB * aB);
            // STILE = tono del saturato: a sinistra scuro, al centro com'e', a destra brillante (il 4 TRACK e' sempre un po' chiuso)
            const float toneHz = type == 1 ? 7000.0f + 6000.0f * st : 4000.0f * std::pow (5.0f, st * 2.0f);
            for (int c = 0; c < 2; ++c) { satTone[c].lp (s4, jmin (40000.0f, toneHz)); if (! satAAok) { satAA[c][0].lp (s4, 5000, 0.5412); satAA[c][1].lp (s4, 5000, 1.3066); } }
            satAAok = true;
            const float aLo4 = coef (200, s4), aHi4 = coef (3000, s4);
            const float mx = (float) ja::modelInfo (model).maxDb * 0.8f;
            const float gA = Decibels::decibelsToGain (mx * aR.a * aR.a), gB = Decibels::decibelsToGain (mx * aR.b * aR.b);
            for (int k = 0; k < N4; ++k)
            {
                const int i = k / kOs; const float t = (k + 1) / (float) N4, a = aR.atF (t);
                const float ae = a * a, wet = jmin (1.0f, a * 3.0f), g = gA + (gB - gA) * t;
                const float w = tgtW (tgt, trAt (i));
                for (int c = 0; c < nc; ++c)
                {
                    const float xin = u[c][k];
                    float band = xin;
                    if (foc != 0) { const float lo = onePole (xin, fLo4[0][c], aLo4), lph = onePole (xin, fHi4[0][c], aHi4); band = foc == 1 ? lo : foc == 3 ? xin - lph : lph - lo; }
                    const float rest = xin - band;
                    // il fuzz ha gia' tanto guadagno suo: meno spinta, e gli acuti oltre i 5 kHz non entrano nel transistor (come da un pedale vero)
                    float y = model == ja::FUZZ ? drv[c].process (satAA[c][1] (satAA[c][0] (band)), g * 0.7f, ae * 0.3f) : drv[c].process (band, g, ae);
                    {   // STILE continuo: a sinistra sfuma verso lo scuro, a destra aggiunge brillantezza, al centro neutro (niente salti a meta')
                        const float lp = satTone[c] (y);
                        y = type == 1 ? lp : st >= 0.5f ? y + (y - lp) * (st - 0.5f) * 1.2f : y + (lp - y) * jmin (1.0f, (0.5f - st) * 4.0f);
                    }
                    u[c][k] = rest + band + (y - band) * w * wet;
                }
            }
            if (mono) { drv[1] = drv[0]; satTone[1] = satTone[0]; for (int j = 0; j < 4; ++j) satAA[1][j] = satAA[0][j]; fLo4[0][1] = fLo4[0][0]; fHi4[0][1] = fHi4[0][0]; }
        }
        // tutto quello che il saturatore crea sopra i 21 kHz si toglie qui, prima di tornare al rate normale: niente sporco ripiegato
        for (int c = 0; c < nc; ++c) { float* x = u[c]; auto* f = satLp8[c]; for (int k = 0; k < N4; ++k) { float y = x[k]; for (int j = 0; j < 8; ++j) y = f[j] (y); x[k] = y; } }
        if (mono) { std::copy (u[0], u[0] + N4, u[1]); for (int j = 0; j < 8; ++j) satLp8[1][j] = satLp8[0][j]; }
        osSat->processSamplesDown (blk);
        {   // volume pari: il saturato esce con la stessa energia che entra (media di 150 ms), campione per campione: uguale con ogni buffer
            const float kk = std::exp (-1.0f / (0.15f * (float) sr)), ka = 1.0f - std::exp (-1.0f / (0.05f * (float) sr));
            for (int i = 0; i < N; ++i)
            {
                const float xi = gSat.tmp[0][(size_t) i] * gSat.tmp[0][(size_t) i] + gSat.tmp[1][(size_t) i] * gSat.tmp[1][(size_t) i], xo = ch[0][i] * ch[0][i] + ch[1][i] * ch[1][i];
                satLvl[0] = kk * satLvl[0] + (1 - kk) * xi; satLvl[1] = kk * satLvl[1] + (1 - kk) * xo;
                const float tg = act && satLvl[0] > 1e-10f ? jlimit (0.1f, 2.0f, std::sqrt (satLvl[0] / jmax (1e-12f, satLvl[1]))) : 1.0f;
                satAg += (tg - satAg) * ka; ch[0][i] *= satAg; ch[1][i] *= satAg;
            }
        }
        }
        // fuori dall'8x quando e' spento: solo un ritardo uguale, quindi neutro al bit; dissolvenza quando si accende o si spegne
        gSat.mix (ch, N, act);
        lag += latSat;
    }

        break;
        }
        case 4:
        {
    // ---- 5. SPACE ----
    if (v ("u_spOn") > 0.5f && live (3, "u_spAmt"))
    {
        const auto amtR = ramp (3, "u_spAmt"); const int type = (int) v ("u_spType"); const float dec = v ("u_spDecay"), duck = v ("u_spDuck"), width = v ("u_spWidth");
        const int pre = jlimit (1, preBuf.getNumSamples() - 2, (int) (v ("u_spPre") * 0.001 * sr)); const int PL = preBuf.getNumSamples();
        verb.setup (type, dec);
        if (type != lastSpType) { verb.reset(); lastSpType = type; }
        const float lo = v ("u_spLo"), hi = v ("u_spHi");
        if (lo != spLast[0]) { spF[0].set (3, sr, lo, 0, 0.707); spLast[0] = lo; }
        if (hi != spLast[1]) { spF[1].set (4, sr, hi, 0, 0.707); spLast[1] = hi; }
        for (int i = 0; i < N; ++i)
        {
            preBuf.setSample (0, preW, ch[0][i]); preBuf.setSample (1, preW, ch[1][i]);
            int r = preW - pre; if (r < 0) r += PL;
            wetL[(size_t) i] = preBuf.getSample (0, r); wetR[(size_t) i] = preBuf.getSample (1, r);
            if (++preW >= PL) preW = 0;
        }
        verb.process (wetL.data(), wetR.data(), wetL.data(), wetR.data(), N);
        if (lo > 21 || hi < 19900)
            for (int i = 0; i < N; ++i)
            {
                float& wl = wetL[(size_t) i]; float& wr = wetR[(size_t) i];
                if (lo > 21) { wl = spF[0].run (wl, 0); wr = spF[0].run (wr, 1); }
                if (hi < 19900) { wl = spF[1].run (wl, 0); wr = spF[1].run (wr, 1); }
            }
        for (int i = 0; i < N; ++i)
        {
            float wl = wetL[(size_t) i], wr = wetR[(size_t) i];
            if (type == 3)   // molla: catena di allpass dispersivi
                for (int k = 0; k < 4; ++k) { const float a = 0.62f; float y = -a * wl + apZ[0][k]; apZ[0][k] = wl + a * y; wl = y; y = -a * wr + apZ[1][k]; apZ[1][k] = wr + a * y; wr = y; }
            const float m = 0.5f * (wl + wr), sd = 0.5f * (wl - wr) * width; wl = m + sd; wr = m - sd;
            const float am = amtR.at (i), g = 1.1f * std::pow (am, 1.6f) * (1.0f - duck * jlimit (0.0f, 0.95f, envF * 4.0f));
            ch[0][i] += wl * g; ch[1][i] += wr * g;
        }
    }

        break;
        }
        case 5:
        {
    // ---- 6. CLIP (a 8x) ----
    {
        const bool on = v ("u_mOn") > 0.5f;
        const auto aR = ramp (12, "u_mAmt"), toneR = ramp (13, "u_mTone");
        juce::dsp::AudioBlock<float> blk (ch, 2, (size_t) N);
        const bool act = on && (aR.a > 0.001f || aR.b > 0.001f);
        if (act && ! gClip.running) { osClip->reset(); gClip.running = true; gClip.warm = 2 * latClip + 8; }
        if (gClip.running)
        {
        for (int c = 0; c < 2; ++c) std::copy (ch[c], ch[c] + N, gClip.tmp[c].begin());
        const bool mono = std::memcmp (ch[0], ch[1], sizeof (float) * (size_t) N) == 0; const int nc = mono ? 1 : 2;
        auto up4 = osClip->processSamplesUp (blk); const int N4 = (int) up4.getNumSamples(); float* u[2] = { up4.getChannelPointer (0), up4.getChannelPointer (1) };
        if (act)
        {
            const int f = N4 / jmax (1, N); const double s4 = sr * f;
            const float tgt = v ("u_mTgt"); const int stages = (int) v ("u_mStages") + 1, foc = (int) v ("u_mFoc");
            const float tA = coef (2000, s4), aLo4 = coef (200, s4), aHi4 = coef (3000, s4);
            for (int k = 0; k < N4; ++k)
            {
                const int i = k / f; const float t = (k + 1) / (float) N4, a = aR.atF (t), tone = toneR.atF (t);
                const float ae = a * a, wet = jmin (1.0f, a * 3.0f), d = 1.0f + ae * 10.0f, comp = 1.0f / (1.0f + ae * 0.8f);
                const float w = tgtW (tgt, trAt (i));
                for (int c = 0; c < nc; ++c)
                {
                    const float xin = u[c][k];
                    float band = xin;
                    if (foc != 0) { const float lo = onePole (xin, fLo4[1][c], aLo4), lph = onePole (xin, fHi4[1][c], aHi4); band = foc == 1 ? lo : foc == 3 ? xin - lph : lph - lo; }
                    const float rest = xin - band;
                    float y = band * d;
                    for (int s2 = 0; s2 < stages; ++s2) y = (s2 % 2 == 0) ? clipAd[c][s2].tanhA (y * 1.1f) : clipAd[c][s2].softA (y * 1.25f);   // con antiderivata: niente sporco
                    const float lp = onePole (y, mLp[c], tA); y = lp * (1 - tone * 0.6f) + (y - lp) * (1 + tone * 0.6f);
                    y *= comp;
                    u[c][k] = rest + band + (y - band) * w * wet;
                }
            }
        }
        if (mono && act) { for (int j = 0; j < 4; ++j) clipAd[1][j] = clipAd[0][j]; fLo4[1][1] = fLo4[1][0]; fHi4[1][1] = fHi4[1][0]; mLp[1] = mLp[0]; }
        // sopra i 21 kHz via prima di tornare al rate normale (sempre, anche mentre sfuma: stessa fase)
        for (int c = 0; c < nc; ++c) { float* x = u[c]; auto* fl = clipLp8[c]; for (int k = 0; k < N4; ++k) { float y = x[k]; for (int j = 0; j < 8; ++j) y = fl[j] (y); x[k] = y; } }
        if (mono) { std::copy (u[0], u[0] + N4, u[1]); for (int j = 0; j < 8; ++j) clipLp8[1][j] = clipLp8[0][j]; }
        osClip->processSamplesDown (blk);
        }
        // fuori dall'8x quando e' spento: solo un ritardo uguale, quindi neutro al bit; dissolvenza quando si accende o si spegne
        gClip.mix (ch, N, act);
        lag += latClip;
    }

        break;
        }
        case 6:
        {
        // ---- 7. EXTRA: delay a tempo, bitcrush, wobble ----
        if (v ("u_xOn") > 0.5f && (live (9, "u_xAmt") || (int) v ("u_xType") == 0))
        {
            const auto amtR = ramp (9, "u_xAmt"); const int type = (int) v ("u_xType"); const float p1 = v ("u_xP1"), p2 = v ("u_xP2"), tgt = ramp (18, "u_xTgt").b;
            const int BL = xBuf.getNumSamples();
            const float xlo = v ("u_xLo"), xhi = v ("u_xHi");
            if (xlo != xLast[0]) { xF[0].set (3, sr, xlo, 0, 0.707); xLast[0] = xlo; }
            if (xhi != xLast[1]) { xF[1].set (4, sr, xhi, 0, 0.707); xLast[1] = xhi; }
            auto xfilt = [&] (float y, int c) { if (xlo > 21) y = xF[0].run (y, c); if (xhi < 19900) y = xF[1].run (y, c); return y; };
            if (type != lastXType) { xBuf.clear(); xLp[0] = xLp[1] = 0; lastXType = type; }
            if (type == 0)
            {
                static const double beats[] = { 0.25, 1.0 / 3.0, 0.5, 0.75, 1.0, 1.5, 2.0 };
                const double dt = juce::jlimit (32.0, BL - 4.0, beats[jlimit (0, 6, (int) (p1 * 6.99f))] * 60.0 / jmax (40.0, bpm.load()) * sr);
                const float fb = p2 * 0.9f, a = coef (juce::jmin (5500.0f, v ("u_xHi")), sr);
                for (int i = 0; i < N; ++i)
                {
                    const float w = tgtW (tgt, trAt (i));
                    double rp = xW - dt; if (rp < 0) rp += BL; const int i0 = (int) rp, i1 = (i0 + 1) % BL; const float fr = (float) (rp - i0);
                    const float yl = xBuf.getSample (0, i0) * (1 - fr) + xBuf.getSample (0, i1) * fr, yr = xBuf.getSample (1, i0) * (1 - fr) + xBuf.getSample (1, i1) * fr;
                    xLp[0] = (1 - a) * yr + a * xLp[0]; xLp[1] = (1 - a) * yl + a * xLp[1];
                    const float fl = xfilt (xLp[0], 0), fr2 = xfilt (xLp[1], 1);
                    // la quantita' entra nel delay e non all'uscita: le eco entrano ed escono morbide, la coda resta
                    const float am = amtR.at (i);
                    xBuf.setSample (0, xW, ch[0][i] * w * am + std::tanh (fl * fb)); xBuf.setSample (1, xW, ch[1][i] * w * am + std::tanh (fr2 * fb));
                    if (++xW >= BL) xW = 0;
                    ch[0][i] += yl; ch[1][i] += yr;
                }
            }
            else if (type == 1)
            {
                const float q = std::pow (2.0f, 2.0f + (1.0f - p1) * 13.0f); const double step = 1.0 - p2 * 0.95;
                for (int i = 0; i < N; ++i)
                {
                    const float w = tgtW (tgt, trAt (i)) * amtR.at (i);
                    xPh += step; if (xPh >= 1.0) { xPh -= 1.0; xHold[0] = ch[0][i]; xHold[1] = ch[1][i]; }
                    for (int c = 0; c < 2; ++c) { const float y = xfilt (std::round (xHold[c] * q) / q, c); ch[c][i] += (y - ch[c][i]) * w; }
                }
            }
            else
            {
                const double rate = 0.1 + p1 * p1 * 8.0, depth = (0.0005 + p2 * 0.0075) * sr;
                for (int i = 0; i < N; ++i)
                {
                    const float w = tgtW (tgt, trAt (i)) * amtR.at (i);
                    xLfo += rate / sr; if (xLfo > 1) xLfo -= 1;
                    if (xr.nextFloat() < 2.0f / (float) sr) xDt = xr.nextFloat() * 2 - 1; xDrift += (xDt - xDrift) * 2.0 / sr;
                    xBuf.setSample (0, xW, ch[0][i]); xBuf.setSample (1, xW, ch[1][i]);
                    for (int c = 0; c < 2; ++c)
                    {
                        const double d = 4 + depth * (1.0 + std::sin (juce::MathConstants<double>::twoPi * (xLfo + c * 0.08)) * 0.8 + xDrift * 0.2);
                        double rp = xW - d; if (rp < 0) rp += BL; const int i0 = (int) rp, i1 = (i0 + 1) % BL; const float fr = (float) (rp - i0);
                        const float y = xfilt (xBuf.getSample (c, i0) * (1 - fr) + xBuf.getSample (c, i1) * fr, c);
                        ch[c][i] += (y - ch[c][i]) * w;
                    }
                    if (++xW >= BL) xW = 0;
                }
            }
        }
        break;
        }
        default: break;
        }
    }
    // ---- MASTER ----
    {
        const auto toneR = ramp (4, "u_tone"), airR = ramp (5, "u_air"), attR = ramp (6, "u_att"), susR = ramp (7, "u_sus"), wR = ramp (8, "u_iWidth"); const bool clip = v ("u_clip") > 0.5f;
        const float tA = coef (700, sr), aA = coef (7500, sr);
        const bool eqOn = v ("u_eqOn") > 0.5f;
        // EQ (motore del Master): bande lette una volta per blocco
        jmx::EqSet eqS; eqS.live = true; bool eqAny = false;
        if (eqOn)
        {
            const int so = eqSolo.load();
            for (int b = 0; b < kQBands; ++b)
            {
                auto& B = eqS.b[b];
                B.on = v (qId (b, "on")) > 0.5f; if (! B.on) continue; eqAny = true;
                B.type = (int) v (qId (b, "type")); B.ch = (int) v (qId (b, "ch")); B.slope = (int) v (qId (b, "slope"));
                B.f = v (qId (b, "f")); B.g = v (qId (b, "g")); B.q = v (qId (b, "q"));
                B.dyn = v (qId (b, "dyn")) > 0.5f; B.thr = v (qId (b, "thr")); B.range = v (qId (b, "rng")); B.solo = b == so;
            }
        }
        // Pultec
        PultecSettings puS; puS.lb = v ("u_pLB"); puS.la = v ("u_pLA"); puS.lf = (int) v ("u_pLF"); puS.hb = v ("u_pHB"); puS.bw = v ("u_pBW"); puS.hf = (int) v ("u_pHF"); puS.ha = v ("u_pHA"); puS.as = (int) v ("u_pAS");
        // il Pultec scivola (20 ms) verso le manopole nuove, e si accende/spegne sfumando: niente click
        if (v ("u_pOn") < 0.5f) { puS.lb = puS.la = puS.hb = puS.ha = 0; }
        if (! puInit) { puSm = puS; puInit = true; puLast = {}; puLast.lb = -1; }
        puSm.lf = puS.lf; puSm.hf = puS.hf; puSm.as = puS.as;
        const bool iOn = v ("u_iOn") > 0.5f;
        const float iM = v ("u_iMono");
        if (iOn && iM > 21 && iM != imLast) { imB.set (4, sr, iM, 0, 0.707); imLast = iM; }
        const int limType = (int) v ("u_limType");
        for (int i = 0; i < N; ++i)
        {
            if ((i & 31) == 0)
            {
                const float a = 1.0f - std::exp (-32.0f / (0.02f * (float) sr));
                auto sm = [a] (float& x, float t) { x += (t - x) * a; if (std::abs (x - t) < 1e-4f) x = t; };
                sm (puSm.lb, puS.lb); sm (puSm.la, puS.la); sm (puSm.hb, puS.hb); sm (puSm.ha, puS.ha); sm (puSm.bw, puS.bw);
                puAct = pultecActive (puSm) || pultecActive (puS);
                if (puAct && std::memcmp (&puSm, &puLast, sizeof (puSm)) != 0) { pultecFilters (puSm, sr, pu); puLast = puSm; }
                puMix = jmin (1.0f, (puSm.lb + puSm.la + puSm.hb + puSm.ha) * 2.0f);
                puDrive = jmin (1.0f, (puSm.lb + puSm.hb) / 20.0f * 0.6f + 0.15f);
            }
            if (puAct) for (int c = 0; c < 2; ++c) { float y = ch[c][i]; for (auto& f : pu) y = f.run (y, c); const float col = puCol.run (y, c, AE_PE1C, puDrive, sr); ch[c][i] = y + (col - y) * puMix; }
            const float iW = wR.at (i);
            if (iOn && (std::abs (iW - 1.0f) > 0.005f || iM > 21))
            {
                const float m = 0.5f * (ch[0][i] + ch[1][i]); float sd = 0.5f * (ch[0][i] - ch[1][i]);
                if (iM > 21) sd -= imB.run (sd, 0);
                sd *= iW; ch[0][i] = m + sd; ch[1][i] = m - sd;
            }
        }
        for (int i = 0; i < N; ++i)
        {
            const float x = 0.5f * (std::abs (ch[0][i]) + std::abs (ch[1][i]));
            mEnvF = x > mEnvF ? fA * mEnvF + (1 - fA) * x : fR * mEnvF + (1 - fR) * x;
            mEnvS = x > mEnvS ? sA * mEnvS + (1 - sA) * x : sR * mEnvS + (1 - sR) * x;
            const float tr = jlimit (0.0f, 1.0f, (mEnvF / (mEnvS + 1e-5f) - 1.0f) * 0.6f);
            const float att = attR.at (i), sus = susR.at (i), tone = toneR.at (i), air = airR.at (i);
            const float g = jlimit (0.2f, 3.0f, (1.0f + att * tr * 1.5f) * (1.0f + sus * (1.0f - tr) * 0.7f));
            for (int c = 0; c < 2; ++c)
            {
                float y = ch[c][i];
                if (std::abs (tone) > 1e-4f) { const float lp = onePole (y, tLp[c], tA); y = lp * (1 - tone * 0.8f) + (y - lp) * (1 + tone * 0.85f); }
                if (air > 1e-4f) { const float lp = onePole (y, aLp[c], aA); y += (y - lp) * air * 3.4f; }
                ch[c][i] = y * g;
            }
        }
        if (eqAny) peq.process (ch[0], ch[1], N, eqS);
        else for (int b = 0; b < kQBands; ++b) peq.dynUi[b] = 0.0f;
        {   // LIMITE (CLIP o MAXX) a 4x: niente sporco ripiegato. Spento = solo un ritardo uguale (neutro al bit)
            juce::dsp::AudioBlock<float> blk (ch, 2, (size_t) N);
            if (clip && ! gLim.running) { osLim->reset(); gLim.running = true; gLim.warm = 2 * latLim + 8; limAd[0] = limAd[1] = {}; maxx = {}; }
            if (gLim.running)
            {
                for (int c = 0; c < 2; ++c) std::copy (ch[c], ch[c] + N, gLim.tmp[c].begin());
                auto up = osLim->processSamplesUp (blk); const int N4 = (int) up.getNumSamples(); float* u0 = up.getChannelPointer (0); float* u1 = up.getChannelPointer (1);
                if (clip)
                {
                    if (limType == 0) for (int k = 0; k < N4; ++k) { u0[k] = limAd[0].limA (u0[k]); u1[k] = limAd[1].limA (u1[k]); }
                    else for (int k = 0; k < N4; ++k) maxx.run (u0[k], u1[k], sr * 4, 0.966f);   // -0.3 dBFS
                }
                osLim->processSamplesDown (blk);
            }
            gLim.mix (ch, N, clip);
        }
        for (int i = 0; i < N; ++i)
        {
            {   // il pulito e' ritardato quanto l'oversampling di saturazione e clip
                const float mg = magR.at (i);
                const float dl = dryDl[0].read (osLat, i, N), dr = dryDl[1].read (osLat, i, N);
                ch[0][i] = dl + (ch[0][i] - dl) * mg; ch[1][i] = dr + (ch[1][i] - dr) * mg;
            }
            // dati per i display
            const float in = 0.5f * (std::abs (dryL[(size_t) i]) + std::abs (dryR[(size_t) i])), out = 0.5f * (std::abs (ch[0][i]) + std::abs (ch[1][i]));
            accIn = jmax (accIn, in); accOut = jmax (accOut, out); accTr = jmax (accTr, trW[(size_t) i]);
            const int aw = anW.load (std::memory_order_relaxed);
            anIn[aw] = 0.5f * (dryL[(size_t) i] + dryR[(size_t) i]); anOut[aw] = 0.5f * (ch[0][i] + ch[1][i]); anL[aw] = ch[0][i]; anR[aw] = ch[1][i]; anW.store ((aw + 1) % AN, std::memory_order_relaxed);
            { const double kk = 0.99995; cLL = kk * cLL + ch[0][i] * ch[0][i]; cRR = kk * cRR + ch[1][i] * ch[1][i]; cLR = kk * cLR + ch[0][i] * ch[1][i]; }
            if (++accN >= (int) (sr / 120))
            {
                scIn.push (accIn); scOut.push (accOut); scGr.push (grMeter.load()); scTr.push (accTr);
                corr = cLL * cRR > 1e-12 ? (float) (cLR / std::sqrt (cLL * cRR)) : 1.0f;
                accIn = accOut = accTr = 0; accN = 0;
            }
        }
    }
}
}

namespace jb
{
static void rbj (int type, double sr, double f, double gDb, double q, double& b0, double& b1, double& b2, double& a0, double& a1, double& a2)
{
    const double A = std::pow (10.0, gDb / 40.0), w = juce::MathConstants<double>::twoPi * juce::jlimit (10.0, sr * 0.45, f) / sr, cs = std::cos (w), sn = std::sin (w);
    if (type == 1) { const double al = sn / (2 * q); b0 = 1 + al * A; b1 = -2 * cs; b2 = 1 - al * A; a0 = 1 + al / A; a1 = -2 * cs; a2 = 1 - al / A; return; }
    if (type == 3 || type == 4)
    {
        const double al = sn / (2 * q);
        if (type == 3) { b0 = (1 + cs) / 2; b1 = -(1 + cs); b2 = (1 + cs) / 2; } else { b0 = (1 - cs) / 2; b1 = 1 - cs; b2 = (1 - cs) / 2; }
        a0 = 1 + al; a1 = -2 * cs; a2 = 1 - al; return;
    }
    const double sA = std::sqrt (A) * sn / std::sqrt (2.0);
    if (type == 0) { b0 = A * ((A + 1) - (A - 1) * cs + sA); b1 = 2 * A * ((A - 1) - (A + 1) * cs); b2 = A * ((A + 1) - (A - 1) * cs - sA); a0 = (A + 1) + (A - 1) * cs + sA; a1 = -2 * ((A - 1) + (A + 1) * cs); a2 = (A + 1) + (A - 1) * cs - sA; }
    else { b0 = A * ((A + 1) + (A - 1) * cs + sA); b1 = -2 * A * ((A - 1) + (A + 1) * cs); b2 = A * ((A + 1) + (A - 1) * cs - sA); a0 = (A + 1) - (A - 1) * cs + sA; a1 = 2 * ((A - 1) - (A + 1) * cs); a2 = (A + 1) - (A - 1) * cs - sA; }
}
void Biq::set (int type, double sr, double f, double gDb, double q)
{
    double B0, B1, B2, A0, A1, A2; rbj (type, sr, f, gDb, q, B0, B1, B2, A0, A1, A2);
    b0 = (float) (B0 / A0); b1 = (float) (B1 / A0); b2 = (float) (B2 / A0); a1 = (float) (A1 / A0); a2 = (float) (A2 / A0);
}
float Biq::magDb (int type, double sr, double f, double gDb, double q, double hz)
{
    double B0, B1, B2, A0, A1, A2; rbj (type, sr, f, gDb, q, B0, B1, B2, A0, A1, A2);
    const std::complex<double> z = std::polar (1.0, -juce::MathConstants<double>::twoPi * hz / sr), z2 = z * z;
    return (float) (20.0 * std::log10 (std::abs ((B0 + B1 * z + B2 * z2) / (A0 + A1 * z + A2 * z2)) + 1e-12));
}
}

namespace jb
{
void Fdn::prepare (double s)
{
    sr = s;
    for (int k = 0; k < NL; ++k) line[k].assign ((size_t) (0.25 * sr) + 8, 0.0f);
    for (int k = 0; k < NA; ++k) ap[k].assign ((size_t) (0.03 * sr) + 4, 0.0f);
    curType = -1; curDecay = -1;
}
void Fdn::reset()
{
    for (auto& l : line) std::fill (l.begin(), l.end(), 0.0f);
    for (auto& a : ap) std::fill (a.begin(), a.end(), 0.0f);
    for (auto& v : lp) v = 0;
}
void Fdn::setup (int type, float decay)
{
    if (type == curType && std::abs (decay - curDecay) < 0.002f) return;
    curType = type; curDecay = decay;
    static const float base[NL] = { 29.7f, 37.1f, 41.1f, 43.7f, 47.9f, 53.3f, 59.3f, 67.1f };
    // dimensione, RT60 min/max (s), smorzamento degli alti, diffusione
    static const float sizeF[] = { 0.55f, 1.45f, 0.95f, 0.75f }, rtLo[] = { 0.25f, 1.1f, 0.7f, 0.6f }, rtHi[] = { 1.3f, 4.5f, 3.2f, 2.6f }, dmp[] = { 0.45f, 0.25f, 0.08f, 0.35f }, dif[] = { 0.62f, 0.7f, 0.78f, 0.72f };
    const int t = juce::jlimit (0, 3, type);
    const float rt = rtLo[t] * std::pow (rtHi[t] / rtLo[t], decay);
    for (int k = 0; k < NL; ++k)
    {
        len[k] = juce::jlimit (16, (int) line[k].size() - 4, (int) (base[k] * sizeF[t] * 0.001 * sr));
        g[k] = (float) std::pow (10.0, -3.0 * len[k] / (rt * sr));
    }
    static const float abase[NA] = { 4.77f, 3.59f, 12.73f, 9.31f };
    for (int k = 0; k < NA; ++k) alen[k] = juce::jlimit (8, (int) ap[k].size() - 2, (int) (abase[k] * (t == 2 ? 1.3f : 1.0f) * 0.001 * sr));
    damp = dmp[t]; diff = dif[t];
    lfoInc = (float) (0.6 / sr); modDepth = (float) (0.0006 * sr) * (t == 1 ? 1.0f : 0.6f);
}
void Fdn::process (const float* inL, const float* inR, float* outL, float* outR, int N)
{
    for (int i = 0; i < N; ++i)
    {
        // diffusione in ingresso (mono)
        float x = 0.5f * (inL[i] + inR[i]);
        for (int k = 0; k < NA; ++k)
        {
            auto& b = ap[k]; const int sz = (int) b.size(); int r = aw[k] - alen[k]; if (r < 0) r += sz;
            const float d = b[(size_t) r], y = -diff * x + d;
            b[(size_t) aw[k]] = x + diff * y; aw[k] = (aw[k] + 1) % sz; x = y;
        }
        // lettura delle linee (due modulate)
        lfo += lfoInc; if (lfo > 1) lfo -= 1;
        float o[NL];
        for (int k = 0; k < NL; ++k)
        {
            auto& b = line[k]; const int sz = (int) b.size();
            float dl = (float) len[k];
            if (k == 2) dl += modDepth * std::sin (juce::MathConstants<float>::twoPi * lfo);
            if (k == 5) dl += modDepth * std::cos (juce::MathConstants<float>::twoPi * lfo * 0.83f);
            float rp = (float) lw[k] - dl; while (rp < 0) rp += (float) sz;
            const int a = (int) rp; const float fr = rp - (float) a;
            float v = b[(size_t) a] * (1 - fr) + b[(size_t) ((a + 1) % sz)] * fr;
            lp[k] = v + damp * (lp[k] - v);   // smorzamento degli alti
            o[k] = lp[k] * g[k];
        }
        // matrice di Hadamard 8x8 (normalizzata)
        float h[NL];
        {
            float a0 = o[0] + o[1], a1 = o[0] - o[1], a2 = o[2] + o[3], a3 = o[2] - o[3], a4 = o[4] + o[5], a5 = o[4] - o[5], a6 = o[6] + o[7], a7 = o[6] - o[7];
            float b0 = a0 + a2, b1 = a1 + a3, b2 = a0 - a2, b3 = a1 - a3, b4 = a4 + a6, b5 = a5 + a7, b6 = a4 - a6, b7 = a5 - a7;
            const float n = 0.35355339f;
            h[0] = (b0 + b4) * n; h[1] = (b1 + b5) * n; h[2] = (b2 + b6) * n; h[3] = (b3 + b7) * n; h[4] = (b0 - b4) * n; h[5] = (b1 - b5) * n; h[6] = (b2 - b6) * n; h[7] = (b3 - b7) * n;
        }
        for (int k = 0; k < NL; ++k)
        {
            auto& b = line[k]; const int sz = (int) b.size();
            b[(size_t) lw[k]] = h[k] + x * (k % 2 ? 0.5f : -0.5f); lw[k] = (lw[k] + 1) % sz;
        }
        outL[i] = 0.6f * (o[0] - o[2] + o[4] - o[6] + o[1] * 0.5f);
        outR[i] = 0.6f * (o[1] - o[3] + o[5] - o[7] - o[0] * 0.5f);
    }
}
}
