#include "ButterDSP.h"
#include <complex>

namespace jb
{
using namespace juce;
static const StringArray kNoise { "WHITE", "PINK", "VINILE", "NASTRO", "METALLO", "SABBIA" };
static const StringArray kFocus { "FULL", "LOW", "MID", "HIGH" };
static const StringArray kComp { "PUNCH", "WARM", "GLUE", "GENTLE", "SMACK", "OTT" };
static const StringArray kSat { "CONSOLE", "4 TRACK", "TAPE", "TUBE", "DIGITAL", "FLUB", "OVERDRIVE", "DISTORSORE", "FUZZ" };
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
    C ("u_aeModel", "EQ Modello", StringArray { "MAAG EQ4", "SSL E", "NEVE 1084", "LINDELL 81", "73", "PE 1C" }, 2);
    for (int k = 1; k <= 4; ++k)
    {
        const String id = "u_ae" + String (k);
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { id + "g", 1 }, "EQ banda " + String (k) + " gain", NormalisableRange<float> (-15, 15), 0));
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { id + "f", 1 }, "EQ banda " + String (k) + " freq", NormalisableRange<float> (0, 1), k == 4 ? 0.0f : 0.5f));
    }
    F ("u_aeQ", "EQ Q / Bandwidth", 0, 1, 0.5f); F ("u_aeDrive", "EQ Drive", 0, 1, 0.0f);
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

const std::vector<BurroPreset>& burroPresets()
{
    // u_cType: 0 PUNCH, 1 WARM, 2 GLUE, 3 GENTLE, 4 SMACK, 5 OTT
    static const std::vector<BurroPreset> P = {
        { "Init", {} },
        { "Bus | Trap Knock", { { "u_boom", 0.35f }, { "u_bTone", 50 }, { "u_cType", 0 }, { "u_cAmt", 0.45f }, { "u_sType", 0 }, { "u_sAmt", 0.3f }, { "u_sTgt", 0.5f }, { "u_mAmt", 0.25f }, { "u_mStages", 1 }, { "u_att", 0.3f }, { "u_air", 0.2f }, { "u_clip", 1 } } },
        { "Bus | Boom Bap Dusty", { { "u_noise", 0.15f }, { "u_nType", 2 }, { "u_cType", 1 }, { "u_cAmt", 0.4f }, { "u_sType", 1 }, { "u_sAmt", 0.45f }, { "u_sStyle", 0.6f }, { "u_spType", 0 }, { "u_spAmt", 0.18f }, { "u_spDecay", 0.3f }, { "u_tone", -0.3f } } },
        { "Bus | Drill Crunch", { { "u_cType", 4 }, { "u_cAmt", 0.5f }, { "u_sType", 4 }, { "u_sAmt", 0.35f }, { "u_sStyle", 0.4f }, { "u_sTgt", 0.6f }, { "u_mAmt", 0.4f }, { "u_mStages", 2 }, { "u_att", 0.4f } } },
        { "Bus | Lo-fi Room", { { "u_noise", 0.2f }, { "u_nType", 3 }, { "u_sType", 2 }, { "u_sAmt", 0.4f }, { "u_spType", 0 }, { "u_spAmt", 0.35f }, { "u_spDecay", 0.5f }, { "u_spDuck", 0.4f }, { "u_tone", -0.5f } } },
        { "Bus | Glue Leggera", { { "u_cType", 2 }, { "u_cAmt", 0.35f }, { "u_sType", 2 }, { "u_sAmt", 0.15f }, { "u_air", 0.15f } } },
        { "Bus | OTT Sparkle", { { "u_cType", 5 }, { "u_cAmt", 0.6f }, { "u_cBands", 1 }, { "u_air", 0.3f } } },
        { "Bus | Parallel Smack", { { "u_cType", 4 }, { "u_cAmt", 0.85f }, { "u_mAmt", 0.2f }, { "u_mag", 0.45f } } },
        { "Bus | Phonk Sporco", { { "u_cType", 4 }, { "u_cAmt", 0.5f }, { "u_sType", 5 }, { "u_sAmt", 0.35f }, { "u_mAmt", 0.45f }, { "u_mStages", 2 }, { "u_noise", 0.15f }, { "u_nType", 3 }, { "u_clip", 1 } } },
        { "Kick | 808 Boom", { { "u_boom", 0.6f }, { "u_bTone", 45 }, { "u_cType", 2 }, { "u_cAmt", 0.3f }, { "u_sType", 3 }, { "u_sAmt", 0.3f }, { "u_sFoc", 1 } } },
        { "Kick | Acid Kick", { { "u_boom", 0.5f }, { "u_bTone", 60 }, { "u_acid", 1 }, { "u_cType", 0 }, { "u_cAmt", 0.35f }, { "u_clip", 1 } } },
        { "Kick | Punch Corto", { { "u_cType", 0 }, { "u_cAmt", 0.5f }, { "u_att", 0.5f }, { "u_sus", -0.4f }, { "u_mAmt", 0.2f } } },
        { "Snare | Spring", { { "u_spType", 3 }, { "u_spAmt", 0.4f }, { "u_spDecay", 0.5f }, { "u_spPre", 20 }, { "u_spDuck", 0.3f }, { "u_cType", 0 }, { "u_cAmt", 0.3f } } },
        { "Snare | Crack", { { "u_cType", 4 }, { "u_cAmt", 0.45f }, { "u_sType", 0 }, { "u_sAmt", 0.35f }, { "u_sTgt", 0.7f }, { "u_air", 0.35f } } },
        { "Snare | Plate Lunga", { { "u_spType", 2 }, { "u_spAmt", 0.5f }, { "u_spDecay", 0.8f }, { "u_spPre", 30 }, { "u_spWidth", 1 } } },
        { "Hat | Air", { { "u_noise", 0.25f }, { "u_nType", 5 }, { "u_sType", 2 }, { "u_sAmt", 0.2f }, { "u_sFoc", 3 }, { "u_air", 0.6f }, { "u_sus", -0.3f } } },
        { "Hat | Corti e Secchi", { { "u_att", 0.4f }, { "u_sus", -0.6f }, { "u_tone", 0.2f } } },
        { "Loop | Pitch Giu Sporco", { { "u_shAmt", 0.6f }, { "u_shPitch", -5 }, { "u_shTgt", -0.4f }, { "u_sType", 5 }, { "u_sAmt", 0.3f } } },
        { "Loop | Vinile Caldo", { { "u_noise", 0.3f }, { "u_nType", 2 }, { "u_sType", 1 }, { "u_sAmt", 0.4f }, { "u_tone", -0.45f }, { "u_cType", 1 }, { "u_cAmt", 0.3f } } },
        { "Loop | Pompa", { { "u_cType", 2 }, { "u_cAmt", 0.75f }, { "u_sus", 0.3f } } },
        { "FX | Ottava Sopra", { { "u_shAmt", 0.5f }, { "u_shPitch", 12 }, { "u_shTgt", 0.6f }, { "u_spType", 1 }, { "u_spAmt", 0.25f } } },
        { "FX | Bitcrush Totale", { { "u_sType", 4 }, { "u_sAmt", 0.8f }, { "u_sStyle", 0.9f }, { "u_mAmt", 0.5f }, { "u_mStages", 3 } } },
        // u_xType: 0 DELAY, 1 BITCRUSH, 2 WOBBLE  -  u_xP1 delay: 0 1/16, .17 1/8T, .33 1/8, .5 3/16, .67 1/4, .83 3/8, 1 1/2
        { "Extra | Delay 1/8 Snare", { { "u_xType", 0 }, { "u_xAmt", 0.3f }, { "u_xP1", 0.33f }, { "u_xP2", 0.4f }, { "u_xTgt", 0.6f }, { "u_cType", 0 }, { "u_cAmt", 0.3f } } },
        { "Extra | Dub Delay 3/16", { { "u_xType", 0 }, { "u_xAmt", 0.35f }, { "u_xP1", 0.5f }, { "u_xP2", 0.6f }, { "u_spType", 3 }, { "u_spAmt", 0.2f }, { "u_tone", -0.25f } } },
        { "Extra | Delay Lungo 1/4", { { "u_xType", 0 }, { "u_xAmt", 0.25f }, { "u_xP1", 0.67f }, { "u_xP2", 0.45f }, { "u_xTgt", -0.3f }, { "u_cType", 2 }, { "u_cAmt", 0.3f } } },
        { "Extra | Crush 8bit", { { "u_xType", 1 }, { "u_xAmt", 0.6f }, { "u_xP1", 0.55f }, { "u_xP2", 0.35f }, { "u_cType", 4 }, { "u_cAmt", 0.35f } } },
        { "Extra | SP Grit", { { "u_xType", 1 }, { "u_xAmt", 0.45f }, { "u_xP1", 0.3f }, { "u_xP2", 0.55f }, { "u_sType", 0 }, { "u_sAmt", 0.3f }, { "u_tone", -0.2f }, { "u_cType", 2 }, { "u_cAmt", 0.35f } } },
        { "Extra | Digital Rotto", { { "u_xType", 1 }, { "u_xAmt", 0.85f }, { "u_xP1", 0.8f }, { "u_xP2", 0.8f }, { "u_xTgt", 0.5f }, { "u_mAmt", 0.2f } } },
        { "Extra | Wobble Cassetta", { { "u_xType", 2 }, { "u_xAmt", 0.6f }, { "u_xP1", 0.2f }, { "u_xP2", 0.4f }, { "u_noise", 0.15f }, { "u_nType", 3 }, { "u_sType", 2 }, { "u_sAmt", 0.3f }, { "u_tone", -0.3f } } },
        { "Extra | Seasick", { { "u_xType", 2 }, { "u_xAmt", 0.85f }, { "u_xP1", 0.35f }, { "u_xP2", 0.8f }, { "u_spType", 1 }, { "u_spAmt", 0.2f } } },
        { "Extra | Chorus Largo", { { "u_xType", 2 }, { "u_xAmt", 0.45f }, { "u_xP1", 0.55f }, { "u_xP2", 0.25f }, { "u_spWidth", 1 }, { "u_air", 0.2f } } },
        { "Bus | Lo-fi Wobble", { { "u_xType", 2 }, { "u_xAmt", 0.5f }, { "u_xP1", 0.15f }, { "u_xP2", 0.35f }, { "u_noise", 0.2f }, { "u_nType", 2 }, { "u_cType", 1 }, { "u_cAmt", 0.4f }, { "u_sType", 1 }, { "u_sAmt", 0.35f }, { "u_tone", -0.4f } } },
        { "Bus | Memphis Crush", { { "u_xType", 1 }, { "u_xAmt", 0.4f }, { "u_xP1", 0.45f }, { "u_xP2", 0.5f }, { "u_cType", 4 }, { "u_cAmt", 0.55f }, { "u_mAmt", 0.35f }, { "u_mStages", 2 }, { "u_boom", 0.3f }, { "u_clip", 1 } } },
        { "Bus | Rage Wide", { { "u_cType", 5 }, { "u_cAmt", 0.45f }, { "u_sType", 4 }, { "u_sAmt", 0.3f }, { "u_xType", 2 }, { "u_xAmt", 0.3f }, { "u_xP1", 0.6f }, { "u_xP2", 0.2f }, { "u_boom", 0.4f }, { "u_bTone", 48 }, { "u_clip", 1 } } },
        { "Lo-fi | Radio AM", { { "u_sType", 1 }, { "u_sAmt", 0.35f }, { "u_noise", 0.12f }, { "u_nType", 3 }, { "u_cType", 1 }, { "u_cAmt", 0.3f }, { "u_iWidth", 0.4f }, { "u_aeModel", 4 }, { "u_ae4f", 1.00f }, { "u_pHA", 10.0f }, { "u_pAS", 0 } } },
        { "Lo-fi | Cassetta Scura", { { "u_xType", 2 }, { "u_xAmt", 0.45f }, { "u_xP1", 0.2f }, { "u_xP2", 0.35f }, { "u_sType", 2 }, { "u_sAmt", 0.4f }, { "u_noise", 0.15f }, { "u_nType", 3 }, { "u_tone", -0.3f }, { "u_aeModel", 4 }, { "u_ae4f", 0.25f }, { "u_pHA", 10.0f }, { "u_pAS", 0 } } },
        { "Lo-fi | Vinile Filtrato", { { "u_noise", 0.25f }, { "u_nType", 2 }, { "u_sType", 1 }, { "u_sAmt", 0.35f }, { "u_cType", 1 }, { "u_cAmt", 0.3f }, { "u_iMono", 150 }, { "u_aeModel", 4 }, { "u_ae4f", 0.50f }, { "u_pHA", 6.0f }, { "u_pAS", 0 } } },
        { "Lo-fi | Sotto l'Acqua", { { "u_spType", 1 }, { "u_spAmt", 0.3f }, { "u_spDecay", 0.6f }, { "u_spHi", 3000 }, { "u_iWidth", 1.3f }, { "u_pHA", 10.0f }, { "u_pAS", 0 } } },
        { "Lo-fi | Telefono", { { "u_sType", 4 }, { "u_sAmt", 0.3f }, { "u_sStyle", 0.6f }, { "u_iWidth", 0.0f }, { "u_aeModel", 4 }, { "u_ae4f", 1.00f }, { "u_pHA", 10.0f }, { "u_pAS", 0 } } },
        { "Drum Bus | Glue Pulito", { { "u_cType", 2 }, { "u_cAmt", 0.3f }, { "u_sType", 0 }, { "u_sAmt", 0.2f }, { "u_att", 0.15f }, { "u_iMono", 120 }, { "u_aeModel", 4 }, { "u_ae4f", 0.25f } } },
        { "Drum Bus | Punch Moderno", { { "u_cType", 0 }, { "u_cAmt", 0.45f }, { "u_sType", 3 }, { "u_sAmt", 0.3f }, { "u_att", 0.35f }, { "u_sus", -0.1f }, { "u_boom", 0.15f }, { "u_bTone", 50 }, { "u_air", 0.2f }, { "u_clip", 1 }, { "u_iMono", 150 } } },
        { "Drum Bus | Room Vintage", { { "u_cType", 1 }, { "u_cAmt", 0.4f }, { "u_sType", 2 }, { "u_sAmt", 0.35f }, { "u_spType", 0 }, { "u_spAmt", 0.25f }, { "u_spDecay", 0.35f }, { "u_spLo", 200 }, { "u_spHi", 8000 }, { "u_tone", -0.2f }, { "u_iWidth", 1.2f } } },
        { "Chitarra | Acustica Aria", { { "u_cType", 3 }, { "u_cAmt", 0.35f }, { "u_air", 0.3f }, { "u_spType", 2 }, { "u_spAmt", 0.2f }, { "u_spDecay", 0.4f }, { "u_spLo", 300 }, { "u_iWidth", 1.25f }, { "u_aeModel", 4 }, { "u_ae4f", 0.50f } } },
        { "Chitarra | Elettrica Crunch", { { "u_sType", 3 }, { "u_sAmt", 0.45f }, { "u_sStyle", 0.6f }, { "u_cType", 1 }, { "u_cAmt", 0.35f }, { "u_spType", 0 }, { "u_spAmt", 0.15f }, { "u_aeModel", 4 }, { "u_ae4f", 0.50f }, { "u_pHA", 6.0f }, { "u_pAS", 1 } } },
        { "Chitarra | Clean con Delay", { { "u_xType", 0 }, { "u_xAmt", 0.25f }, { "u_xP1", 0.67f }, { "u_xP2", 0.35f }, { "u_xLo", 300 }, { "u_xHi", 5000 }, { "u_cType", 3 }, { "u_cAmt", 0.3f }, { "u_spType", 1 }, { "u_spAmt", 0.2f }, { "u_spLo", 250 }, { "u_iWidth", 1.3f } } },
        { "Basso | 808 Pieno", { { "u_cType", 0 }, { "u_cAmt", 0.4f }, { "u_sType", 2 }, { "u_sAmt", 0.3f }, { "u_iMono", 200 }, { "u_iWidth", 0.8f }, { "u_pHA", 6.0f }, { "u_pAS", 1 } } },
        { "Basso | Elettrico Rotondo", { { "u_cType", 1 }, { "u_cAmt", 0.45f }, { "u_sType", 2 }, { "u_sAmt", 0.3f }, { "u_tone", -0.2f }, { "u_iMono", 150 }, { "u_aeModel", 4 }, { "u_ae4f", 0.25f } } },
        { "Basso | Synth Grinta", { { "u_sType", 3 }, { "u_sAmt", 0.55f }, { "u_mAmt", 0.3f }, { "u_cType", 0 }, { "u_cAmt", 0.35f }, { "u_iMono", 180 } } },
        { "Voce | Rap Davanti", { { "u_cType", 0 }, { "u_cAmt", 0.45f }, { "u_sType", 0 }, { "u_sAmt", 0.25f }, { "u_air", 0.25f }, { "u_spType", 2 }, { "u_spAmt", 0.12f }, { "u_spDecay", 0.3f }, { "u_spLo", 400 }, { "u_spHi", 8000 }, { "u_aeModel", 4 }, { "u_ae4f", 0.50f } } },
        { "Voce | Ritornello Largo", { { "u_cType", 2 }, { "u_cAmt", 0.35f }, { "u_xType", 0 }, { "u_xAmt", 0.2f }, { "u_xP1", 0.67f }, { "u_xP2", 0.35f }, { "u_xLo", 400 }, { "u_xHi", 5000 }, { "u_spType", 1 }, { "u_spAmt", 0.25f }, { "u_spLo", 350 }, { "u_spHi", 9000 }, { "u_air", 0.3f }, { "u_iWidth", 1.3f } } },
        { "Voce | Adlib Filtrati", { { "u_sType", 1 }, { "u_sAmt", 0.4f }, { "u_xType", 0 }, { "u_xAmt", 0.3f }, { "u_xP1", 0.33f }, { "u_xP2", 0.4f }, { "u_iWidth", 1.4f }, { "u_aeModel", 4 }, { "u_ae4f", 1.00f }, { "u_pHA", 10.0f }, { "u_pAS", 0 } } },
        { "Voce | Intima Asciutta", { { "u_cType", 3 }, { "u_cAmt", 0.4f }, { "u_sType", 2 }, { "u_sAmt", 0.15f }, { "u_tone", 0.1f }, { "u_spType", 0 }, { "u_spAmt", 0.08f }, { "u_spDecay", 0.2f }, { "u_aeModel", 4 }, { "u_ae4f", 0.50f } } },
        { "Voce | Plate Anni 80", { { "u_spType", 2 }, { "u_spAmt", 0.3f }, { "u_spDecay", 0.55f }, { "u_spPre", 40 }, { "u_spLo", 500 }, { "u_spHi", 10000 }, { "u_cType", 2 }, { "u_cAmt", 0.3f }, { "u_air", 0.2f } } },
        { "Voce | Slap Delay", { { "u_xType", 0 }, { "u_xAmt", 0.22f }, { "u_xP1", 0.0f }, { "u_xP2", 0.15f }, { "u_cType", 3 }, { "u_cAmt", 0.4f }, { "u_spType", 2 }, { "u_spAmt", 0.15f }, { "u_air", 0.25f } } },
        // ---- novita: impact, formanti, distorsori, Pultec, EQ analogici, MAXX
        { "Impact | Cassa che sfonda", { { "u_boom", 0.45f }, { "u_bTone", 48 }, { "u_bDecay", 0.3f }, { "u_bPunch", 0.5f }, { "u_cType", 0 }, { "u_cAmt", 0.35f }, { "u_clip", 1 }, { "u_limType", 1 } } },
        { "Impact | 808 Thump", { { "u_boom", 0.6f }, { "u_bTone", 42 }, { "u_bDecay", 0.65f }, { "u_bPunch", 0.25f }, { "u_sType", 3 }, { "u_sAmt", 0.25f }, { "u_sFoc", 1 }, { "u_iMono", 160 } } },
        { "Impact | Punch senza boom", { { "u_bPunch", 0.4f }, { "u_cType", 2 }, { "u_cAmt", 0.3f }, { "u_att", 0.2f } } },
        { "Impact | Boom bap pieno", { { "u_boom", 0.3f }, { "u_bTone", 55 }, { "u_bDecay", 0.25f }, { "u_bPunch", 0.35f }, { "u_cType", 1 }, { "u_cAmt", 0.4f }, { "u_sType", 1 }, { "u_sAmt", 0.35f },
                                       { "u_aeModel", 4 }, { "u_ae1g", 2.5f }, { "u_ae1f", 0.33f }, { "u_ae2g", -2 }, { "u_ae2f", 0.2f }, { "u_aeDrive", 0.5f } } },
        { "Shift | Snare grosso (formante giu)", { { "u_shAmt", 0.6f }, { "u_shPitch", 0 }, { "u_shForm", -5 }, { "u_shTgt", 0.3f }, { "u_shFoc", 2 }, { "u_cType", 0 }, { "u_cAmt", 0.3f } } },
        { "Shift | Pitch giu formante fermo", { { "u_shAmt", 0.8f }, { "u_shPitch", -5 }, { "u_shForm", 5 }, { "u_cType", 2 }, { "u_cAmt", 0.25f } } },
        { "Shift | Voci chipmunk", { { "u_shAmt", 1.0f }, { "u_shPitch", 0 }, { "u_shForm", 7 } } },
        { "Shift | Mostro", { { "u_shAmt", 1.0f }, { "u_shPitch", -12 }, { "u_shForm", -4 }, { "u_sType", 7 }, { "u_sAmt", 0.3f } } },
        { "Drive | Overdrive caldo", { { "u_sType", 6 }, { "u_sAmt", 0.45f }, { "u_sStyle", 0.4f }, { "u_cType", 1 }, { "u_cAmt", 0.3f } } },
        { "Drive | Distorsore sul rullante", { { "u_sType", 7 }, { "u_sAmt", 0.45f }, { "u_sStyle", 0.5f }, { "u_sTgt", 0.7f }, { "u_sFoc", 2 }, { "u_cType", 4 }, { "u_cAmt", 0.35f } } },
        { "Drive | Fuzz trap", { { "u_sType", 8 }, { "u_sAmt", 0.4f }, { "u_sStyle", 0.5f }, { "u_cType", 5 }, { "u_cAmt", 0.35f }, { "u_clip", 1 }, { "u_limType", 1 } } },
        { "Pultec | Trucco dei bassi (60 Hz)", { { "u_pLB", 5 }, { "u_pLA", 3 }, { "u_pLF", 2 }, { "u_cType", 2 }, { "u_cAmt", 0.25f } } },
        { "Pultec | Aria 10k", { { "u_pHB", 4 }, { "u_pHF", 4 }, { "u_pBW", 7 }, { "u_pHA", 1.5f }, { "u_pAS", 2 } } },
        { "Pultec | Batteria completa", { { "u_pLB", 4 }, { "u_pLA", 2.5f }, { "u_pLF", 2 }, { "u_pHB", 3 }, { "u_pHF", 3 }, { "u_pBW", 6 }, { "u_cType", 2 }, { "u_cAmt", 0.35f }, { "u_sType", 2 }, { "u_sAmt", 0.2f } } },
        { "EQ | Maag aria sul bus", { { "u_aeModel", 0 }, { "u_ae4g", 4 }, { "u_ae4f", 0.75f }, { "u_ae1g", 2 }, { "u_ae1f", 0.0f } } },
        { "EQ | SSL drum bus", { { "u_aeModel", 1 }, { "u_ae1g", 3 }, { "u_ae1f", 0.33f }, { "u_ae2g", -3 }, { "u_ae2f", 0.33f }, { "u_ae3g", 2 }, { "u_ae3f", 0.67f }, { "u_ae4g", 2.5f }, { "u_ae4f", 0.67f }, { "u_aeQ", 0.4f }, { "u_aeDrive", 0.35f }, { "u_cType", 2 }, { "u_cAmt", 0.3f } } },
        { "EQ | Neve calda", { { "u_aeModel", 2 }, { "u_ae1g", 3 }, { "u_ae1f", 0.33f }, { "u_ae2g", 2 }, { "u_ae2f", 0.4f }, { "u_ae3g", 2 }, { "u_ae3f", 0.5f }, { "u_aeDrive", 0.6f } } },
        { "EQ | 73 in faccia", { { "u_aeModel", 4 }, { "u_ae1g", 4 }, { "u_ae1f", 0.33f }, { "u_ae2g", 3 }, { "u_ae2f", 0.6f }, { "u_ae3g", 3 }, { "u_ae4f", 0.25f }, { "u_aeDrive", 0.75f } } },
        { "EQ | Lindell 81 scolpito", { { "u_aeModel", 3 }, { "u_ae1g", 2.5f }, { "u_ae1f", 0.2f }, { "u_ae2g", -4 }, { "u_ae2f", 0.6f }, { "u_ae3g", 2.5f }, { "u_ae3f", 0.6f }, { "u_ae4g", 2 }, { "u_ae4f", 0.6f }, { "u_aeQ", 0.6f }, { "u_aeDrive", 0.4f } } },
        { "EQ | PE 1C morbido", { { "u_aeModel", 5 }, { "u_ae1g", 5 }, { "u_ae1f", 0.67f }, { "u_ae2g", -3 }, { "u_ae3g", 4 }, { "u_ae3f", 0.5f }, { "u_aeQ", 0.3f }, { "u_ae4g", -2 }, { "u_ae4f", 1.0f }, { "u_aeDrive", 0.5f } } },
        { "Master | Bus pronto (MAXX)", { { "u_cType", 2 }, { "u_cAmt", 0.3f }, { "u_sType", 0 }, { "u_sAmt", 0.2f }, { "u_pLB", 3 }, { "u_pLA", 2 }, { "u_pHB", 2.5f }, { "u_pHF", 4 }, { "u_aeModel", 1 }, { "u_aeDrive", 0.3f }, { "u_clip", 1 }, { "u_limType", 1 }, { "u_in", 3 } } },
        { "Master | Trap moderna", { { "u_boom", 0.25f }, { "u_bTone", 45 }, { "u_bPunch", 0.3f }, { "u_cType", 0 }, { "u_cAmt", 0.35f }, { "u_sType", 2 }, { "u_sAmt", 0.2f }, { "u_aeModel", 0 }, { "u_ae4g", 3 }, { "u_ae4f", 0.5f }, { "u_iMono", 150 }, { "u_clip", 1 }, { "u_limType", 1 } } },
        { "Master | Boom bap da 45 giri", { { "u_noise", 0.12f }, { "u_nType", 2 }, { "u_cType", 1 }, { "u_cAmt", 0.4f }, { "u_sType", 1 }, { "u_sAmt", 0.4f }, { "u_aeModel", 4 }, { "u_ae1g", 3 }, { "u_ae1f", 0.33f }, { "u_aeDrive", 0.6f }, { "u_pHA", 4 }, { "u_pAS", 1 }, { "u_clip", 1 } } } };    return P;
}

void Burro::applyPreset (AudioProcessorValueTreeState& s, int index)
{
    const auto& all = burroPresets(); if (index < 0 || index >= (int) all.size()) return;
    for (auto* p : s.processor.getParameters())
        if (auto* rp = dynamic_cast<RangedAudioParameter*> (p)) if (rp->getParameterID().startsWith ("u_")) rp->setValueNotifyingHost (rp->getDefaultValue());
    for (auto& [id, v] : all[(size_t) index].p) if (auto* rp = s.getParameter (id)) rp->setValueNotifyingHost (rp->convertTo0to1 (v));
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
    form.prepare (sr); for (auto& x : lastAe) x = 99; puInit = false;
}

static inline float onePole (float x, float& z, float a) { z = (1 - a) * x + a * z; return z; }
static inline float coef (double hz, double sr) { return (float) std::exp (-MathConstants<double>::twoPi * hz / sr); }
static inline float tgtW (float t, float tr) { return t >= 0 ? (1 - t) + t * tr : (1 + t) + (-t) * (1 - tr); }

void Burro::process (float* L, float* R, int N, AudioProcessorValueTreeState& s)
{
    if ((int) dryL.size() < N) { dryL.resize ((size_t) N); dryR.resize ((size_t) N); trW.resize ((size_t) N); wetL.resize ((size_t) N); wetR.resize ((size_t) N); }
    auto v = [&s] (const char* id) { return s.getRawParameterValue (id)->load(); };
    const float mag = v ("u_mag");
    // rilevatore dei transienti
    const float fA = (float) std::exp (-1.0 / (0.0005 * sr)), fR = (float) std::exp (-1.0 / (0.03 * sr)), sA = (float) std::exp (-1.0 / (0.025 * sr)), sR = (float) std::exp (-1.0 / (0.15 * sr));
    for (int i = 0; i < N; ++i)
    {
        dryL[(size_t) i] = L[i]; dryR[(size_t) i] = R[i];
        const float x = 0.5f * (std::abs (L[i]) + std::abs (R[i]));
        envF = x > envF ? fA * envF + (1 - fA) * x : fR * envF + (1 - fR) * x;
        envS = x > envS ? sA * envS + (1 - sA) * x : sR * envS + (1 - sR) * x;
        trW[(size_t) i] = jlimit (0.0f, 1.0f, (envF / (envS + 1e-5f) - 1.0f) * 0.6f);
    }
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
        const float boom = v ("u_boom"), noise = v ("u_noise"); const bool acid = v ("u_acid") > 0.5f; const int nt = (int) v ("u_nType");
        const float bf = v ("u_bTone"), bDec = v ("u_bDecay"), punch = v ("u_bPunch");
        const float nlpA = coef (nt == 3 ? 3000 : 9000, sr), hpA = coef (7000, sr);
        const float thDec = (float) std::exp (-1.0 / ((0.04 + bDec * 0.45) * sr)), thPd = (float) std::exp (-1.0 / (0.014 * sr)), lowA = coef (140, sr);
        for (int i = 0; i < N; ++i)
        {
            const float tr = trW[(size_t) i];
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
                if (nt == 2 && crackle <= 0 && rng.nextFloat() < 30.0f / (float) sr) { crackle = 6 + rng.nextInt (30); crackleV = (rng.nextFloat() * 2 - 1) * 0.6f; }
                for (int c = 0; c < 2; ++c)
                {
                    const float w = rng.nextFloat() * 2 - 1; float n = w;
                    switch (nt)
                    {
                        case 1: { auto& b = pinkB[c]; b[0] = 0.99765f * b[0] + w * 0.099046f; b[1] = 0.963f * b[1] + w * 0.2965164f; b[2] = 0.57f * b[2] + w * 1.0526913f; n = (b[0] + b[1] + b[2] + w * 0.1848f) * 0.25f; break; }
                        case 2: n = (crackle > 0 ? crackleV : 0.0f) + w * 0.04f; break;
                        case 3: n = onePole (w, nLp[c], nlpA) * 1.6f; break;
                        case 4: { const float a = onePole (w, nBp1[c], coef (5200, sr)), b2 = onePole (a, nBp2[c], coef (6800, sr)); n = (a - b2) * 6.0f; break; }
                        case 5: n = (w - onePole (w, nLp[c], hpA)) * 1.4f; break;
                        default: break;
                    }
                    ch[c][i] += n * env * (nt == 2 ? 1.0f : 1.0f);
                }
                if (crackle > 0) --crackle;
            }
        }
    }

        break;
        }
        case 1:
        {
    // ---- 2. SHIFT ----
    if (v ("u_shOn") > 0.5f && v ("u_shAmt") > 0.001f)
    {
        const float amt = v ("u_shAmt"), tgt = v ("u_shTgt"); const int foc = (int) v ("u_shFoc");
        const float fst = v ("u_shForm"); const bool useF = std::abs (fst) > 0.05f;
        form.ratio = (float) std::pow (2.0, (v ("u_shPitch") + fst) / 12.0);
        const double ratio = std::pow (2.0, v ("u_shPitch") / 12.0), W = 0.045 * sr; const int BL = shBuf.getNumSamples();
        for (int i = 0; i < N; ++i)
        {
            shPh += 1.0 - ratio; while (shPh < 0) shPh += W; while (shPh >= W) shPh -= W;
            const double d1 = shPh, d2 = std::fmod (shPh + W * 0.5, W);
            const float g1 = 0.5f - 0.5f * std::cos ((float) (MathConstants<double>::twoPi * d1 / W)), g2 = 1 - g1;
            const float w = amt * tgtW (tgt, trW[(size_t) i]);
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
    if (v ("u_cOn") > 0.5f && std::abs (v ("u_cAmt")) > 0.001f)
    {
        const int type = (int) v ("u_cType"); const float amt = v ("u_cAmt"), bands = v ("u_cBands");
        struct CT { float att, rel, ratio; };
        static const CT T[] = { { 25, 90, 4 }, { 8, 250, 3 }, { 10, 200, 2 }, { 30, 300, 1.6f }, { 0.3f, 60, 20 }, { 2, 120, 6 } };
        const auto& t = T[jlimit (0, 5, type)];
        const float down = type == 5 ? std::abs (amt) : jmax (0.0f, amt), up = type == 5 ? std::abs (amt) * 0.45f : jmax (0.0f, -amt);
        const float cA = (float) std::exp (-1.0 / (t.att * 0.001 * sr)), cR = (float) std::exp (-1.0 / (t.rel * 0.001 * sr));
        const float thrD = -down * 36.0f, thrU = -22.0f, makeup = down * 36.0f * (1.0f - 1.0f / t.ratio) * 0.35f;
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
            if (type == 1) { sl = std::tanh (sl * 1.2f) / 1.2f * 1.15f; sr2 = std::tanh (sr2 * 1.2f) / 1.2f * 1.15f; }
            ch[0][i] = sl; ch[1][i] = sr2;
        }
        grMeter = jmax (grMeter.load(), maxGr);
    }

        break;
        }
        case 3:
        {
    // ---- 4. SATURATE ----
    if (v ("u_sOn") > 0.5f && v ("u_sAmt") > 0.001f)
    {
        const int type = (int) v ("u_sType"); const float a = v ("u_sAmt"), st = v ("u_sStyle"), tgt = v ("u_sTgt"); const int foc = (int) v ("u_sFoc");
        // curva morbida: i primi punti percentuali sono delicati, la spinta vera arriva nella seconda meta
        const float ae = a * a, wet = jmin (1.0f, a * 3.0f);
        const float d = 1.0f + ae * 12.0f, comp = 1.0f / (1.0f + ae * 1.2f), lpA = coef (14000 - st * 9000, sr);
        const double holdRate = 1.0 - st * 0.85; const float q = std::pow (2.0f, 14.0f - st * 10.0f);
        for (int i = 0; i < N; ++i)
        {
            const float w = tgtW (tgt, trW[(size_t) i]);
            bool tick = false; if (type == 4) { holdPh += holdRate; if (holdPh >= 1.0) { holdPh -= 1.0; tick = true; } }
            for (int c = 0; c < 2; ++c)
            {
                float band; const float rest = focusSplit (1, c, ch[c][i], foc, band);
                const float x = band * d; float y;
                switch (type)
                {
                    case 1: y = onePole (std::tanh (x * 0.8f) + 0.08f * st * x * x / (1 + x * x), satLp[c], lpA); break;
                    case 2: y = std::atan (x * (1 + st)) / std::atan (1.0f + st) * 0.8f; y = onePole (y, satLp[c], coef (16000 - st * 6000, sr)); break;
                    case 3: y = x > 0 ? std::tanh (x) : std::tanh (x * (0.55f + 0.45f * (1 - st))) / (0.55f + 0.45f * (1 - st)) * 0.8f; y += 0.12f * st * y * y; break;
                    case 4: { if (tick) holdV[c] = jlimit (-1.0f, 1.0f, x); y = std::round (holdV[c] * q) / q; break; }
                    case 5: y = std::sin (x * (1.0f + st * 2.0f)) * 0.9f; break;
                    case 6: { const float xx = x * 1.4f; y = xx > 0 ? std::tanh (xx) : std::tanh (xx * 0.75f) / 0.75f * 0.9f; y = onePole (y, satLp[c], coef (9000 - st * 6000, sr)) * 0.95f; break; }   // overdrive: asimmetrico, medi in avanti
                    case 7: { const float xx = x * (2.0f + st * 4.0f); y = jlimit (-1.0f, 1.0f, xx); y = y - y * y * y / 3.0f; y = onePole (y * 1.3f, satLp[c], coef (7000 - st * 4000, sr)); break; }   // distorsore: duro
                    case 8: { const float xx = x * (3.0f + st * 6.0f); y = std::copysign (1.0f - std::exp (-std::abs (xx)), xx); y = y + 0.3f * st * (y * y - 0.3f); y = onePole (y, satLp[c], coef (6000 - st * 2500, sr)) * 0.85f; break; }   // fuzz
                    default: y = std::tanh (x + st * 0.3f * x * x / (1 + std::abs (x))); break;
                }
                y *= comp;
                ch[c][i] = rest + band + (y - band) * w * wet;
            }
        }
    }

        break;
        }
        case 4:
        {
    // ---- 5. SPACE ----
    if (v ("u_spOn") > 0.5f && v ("u_spAmt") > 0.001f)
    {
        const int type = (int) v ("u_spType"); const float amt = v ("u_spAmt"), dec = v ("u_spDecay"), duck = v ("u_spDuck"), width = v ("u_spWidth");
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
            const float g = 1.1f * std::pow (amt, 1.6f) * (1.0f - duck * jlimit (0.0f, 0.95f, envF * 4.0f));
            ch[0][i] += wl * g; ch[1][i] += wr * g;
        }
    }

        break;
        }
        case 5:
        {
    // ---- 6. CLIP (more) ----
    if (v ("u_mOn") > 0.5f && v ("u_mAmt") > 0.001f)
    {
        const float a = v ("u_mAmt"), tone = v ("u_mTone"), tgt = v ("u_mTgt"); const int stages = (int) v ("u_mStages") + 1, foc = (int) v ("u_mFoc");
        const float ae = a * a, wet = jmin (1.0f, a * 3.0f);
        const float d = 1.0f + ae * 10.0f, comp = 1.0f / (1.0f + ae * 0.8f), tA = coef (2000, sr);
        for (int i = 0; i < N; ++i)
        {
            const float w = tgtW (tgt, trW[(size_t) i]);
            for (int c = 0; c < 2; ++c)
            {
                float band; const float rest = focusSplit (2, c, ch[c][i], foc, band);
                float y = band * d;
                for (int s2 = 0; s2 < stages; ++s2) y = (s2 % 2 == 0) ? std::tanh (y * 1.1f) : jlimit (-0.9f, 0.9f, y * 1.25f);
                const float lp = onePole (y, mLp[c], tA); y = lp * (1 - tone * 0.6f) + (y - lp) * (1 + tone * 0.6f);
                y *= comp;
                ch[c][i] = rest + band + (y - band) * w * wet;
            }
        }
    }

        break;
        }
        case 6:
        {
        // ---- 7. EXTRA: delay a tempo, bitcrush, wobble ----
        if (v ("u_xOn") > 0.5f && v ("u_xAmt") > 0.001f)
        {
            const int type = (int) v ("u_xType"); const float amt = v ("u_xAmt"), p1 = v ("u_xP1"), p2 = v ("u_xP2"), tgt = v ("u_xTgt");
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
                    const float w = tgtW (tgt, trW[(size_t) i]);
                    double rp = xW - dt; if (rp < 0) rp += BL; const int i0 = (int) rp, i1 = (i0 + 1) % BL; const float fr = (float) (rp - i0);
                    const float yl = xBuf.getSample (0, i0) * (1 - fr) + xBuf.getSample (0, i1) * fr, yr = xBuf.getSample (1, i0) * (1 - fr) + xBuf.getSample (1, i1) * fr;
                    xLp[0] = (1 - a) * yr + a * xLp[0]; xLp[1] = (1 - a) * yl + a * xLp[1];
                    const float fl = xfilt (xLp[0], 0), fr2 = xfilt (xLp[1], 1);
                    xBuf.setSample (0, xW, ch[0][i] * w + std::tanh (fl * fb)); xBuf.setSample (1, xW, ch[1][i] * w + std::tanh (fr2 * fb));
                    if (++xW >= BL) xW = 0;
                    ch[0][i] += yl * amt; ch[1][i] += yr * amt;
                }
            }
            else if (type == 1)
            {
                const float q = std::pow (2.0f, 2.0f + (1.0f - p1) * 13.0f); const double step = 1.0 - p2 * 0.95;
                for (int i = 0; i < N; ++i)
                {
                    const float w = tgtW (tgt, trW[(size_t) i]) * amt;
                    xPh += step; if (xPh >= 1.0) { xPh -= 1.0; xHold[0] = ch[0][i]; xHold[1] = ch[1][i]; }
                    for (int c = 0; c < 2; ++c) { const float y = xfilt (std::round (xHold[c] * q) / q, c); ch[c][i] += (y - ch[c][i]) * w; }
                }
            }
            else
            {
                const double rate = 0.1 + p1 * p1 * 8.0, depth = (0.0005 + p2 * 0.0075) * sr;
                for (int i = 0; i < N; ++i)
                {
                    const float w = tgtW (tgt, trW[(size_t) i]) * amt;
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
        const float tone = v ("u_tone"), air = v ("u_air"), att = v ("u_att"), sus = v ("u_sus"); const bool clip = v ("u_clip") > 0.5f;
        const float tA = coef (700, sr), aA = coef (7500, sr);
        const bool eqOn = v ("u_eqOn") > 0.5f;
        // EQ analogico: ricalcolo solo quando cambia qualcosa
        AeSettings ae; ae.model = (int) v ("u_aeModel"); ae.q = v ("u_aeQ"); ae.drive = v ("u_aeDrive");
        for (int k = 0; k < 4; ++k) { ae.g[k] = v (k == 0 ? "u_ae1g" : k == 1 ? "u_ae2g" : k == 2 ? "u_ae3g" : "u_ae4g"); ae.f[k] = v (k == 0 ? "u_ae1f" : k == 1 ? "u_ae2f" : k == 2 ? "u_ae3f" : "u_ae4f"); }
        {
            const float pv[12] = { (float) ae.model, ae.g[0], ae.g[1], ae.g[2], ae.g[3], ae.f[0], ae.f[1], ae.f[2], ae.f[3], ae.q, 0, 0 };
            if (std::memcmp (pv, lastAe, sizeof (pv)) != 0) { for (int k = 0; k < 4; ++k) aeBandFilter (ae, k, sr, this->ae[k], aeAct[k]); std::memcpy (lastAe, pv, sizeof (pv)); }
        }
        bool aeAny = false; for (bool b2 : aeAct) aeAny |= b2;
        // Pultec
        PultecSettings puS; puS.lb = v ("u_pLB"); puS.la = v ("u_pLA"); puS.lf = (int) v ("u_pLF"); puS.hb = v ("u_pHB"); puS.bw = v ("u_pBW"); puS.hf = (int) v ("u_pHF"); puS.ha = v ("u_pHA"); puS.as = (int) v ("u_pAS");
        const bool puOn = v ("u_pOn") > 0.5f && pultecActive (puS);
        if (puOn && (! puInit || std::memcmp (&puS, &puLast, sizeof (puS)) != 0)) { pultecFilters (puS, sr, pu); puLast = puS; puInit = true; }
        const float puDrive = juce::jmin (1.0f, (puS.lb + puS.hb) / 20.0f * 0.6f + 0.15f);
        const bool iOn = v ("u_iOn") > 0.5f;
        const float iW = v ("u_iWidth"), iM = v ("u_iMono");
        if (iOn && iM > 21 && iM != imLast) { imB.set (4, sr, iM, 0, 0.707); imLast = iM; }
        const int limType = (int) v ("u_limType");
        for (int i = 0; i < N; ++i)
        {
            if (puOn) for (int c = 0; c < 2; ++c) { float y = ch[c][i]; for (auto& f : pu) y = f.run (y, c); ch[c][i] = puCol.run (y, c, AE_PE1C, puDrive, sr); }
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
            const float g = jlimit (0.2f, 3.0f, (1.0f + att * tr * 1.5f) * (1.0f + sus * (1.0f - tr) * 0.7f));
            for (int c = 0; c < 2; ++c)
            {
                float y = ch[c][i];
                if (tone != 0) { const float lp = onePole (y, tLp[c], tA); y = lp * (1 - tone * 0.8f) + (y - lp) * (1 + tone * 0.85f); }
                if (air > 0) { const float lp = onePole (y, aLp[c], aA); y += (y - lp) * air * 3.4f; }
                y *= g;
                if (eqOn && (aeAny || ae.drive > 0.001f)) { for (int k = 0; k < 4; ++k) if (aeAct[k]) y = this->ae[k].run (y, c); y = aeCol.run (y, c, ae.model, ae.drive, sr); }
                if (clip && limType == 0) { const float ay = std::abs (y); if (ay > 0.8f) y = std::copysign (0.8f + 0.2f * std::tanh ((ay - 0.8f) / 0.2f), y); }
                ch[c][i] = y;
            }
            if (clip && limType == 1) maxx.run (ch[0][i], ch[1][i], sr, 0.966f);   // -0.3 dBFS
            for (int c = 0; c < 2; ++c) { const float dry = c == 0 ? dryL[(size_t) i] : dryR[(size_t) i]; ch[c][i] = dry + (ch[c][i] - dry) * mag; }
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
