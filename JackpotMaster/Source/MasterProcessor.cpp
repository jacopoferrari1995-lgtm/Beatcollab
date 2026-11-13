#include "MasterProcessor.h"
#include "MasterEditor.h"

namespace jms
{
const char* onParam (int m)
{
    static const char* p[kNumMods] = { "bc_on", "eq_on", "ds_on", "bus_on", "mb_on", "ex_on", "tp_on", "im_on", "mx_on", "lm_on", "e4_on", "pq_on", "vm_on" };
    return p[juce::jlimit (0, kNumMods - 1, m)];
}
const char* deltaParam (int m)
{
    static const char* p[kNumMods] = { "bc_delta", "eq_delta", "ds_delta", "bus_delta", "mb_delta", "ex_delta", "tp_delta", "im_delta", "mx_delta", "lm_delta", "e4_delta", "pq_delta", "vm_delta" };
    return p[juce::jlimit (0, kNumMods - 1, m)];
}
void Processor::setOrder (const std::array<int, kNumMods>& o) { for (int k = 0; k < kNumMods; ++k) order[k] = o[(size_t) k]; }
std::array<int, kNumMods> Processor::getOrder() const { std::array<int, kNumMods> o {}; for (int k = 0; k < kNumMods; ++k) o[(size_t) k] = order[k].load(); return o; }

juce::AudioProcessorValueTreeState::ParameterLayout Processor::layout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    StringArray st; for (int s = 0; s < kNumStyles; ++s) st.add (styleInfo (s).name);
    StringArray tg; for (int t = 0; t < kNumTargets; ++t) tg.add (String (target (t).name) + " " + String ((int) target (t).lufs));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "trim", 1 }, "Input Trim", NormalisableRange<float> (-12, 12, 0.1f), 0, AudioParameterFloatAttributes().withLabel ("dB")));
    // bass control
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "bc_on", 1 }, "Bass On", false));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "bc_delta", 1 }, "Bass Delta", false));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "bc_freq", 1 }, "Bass Freq", NormalisableRange<float> (40, 300, 0, 0.6f), 120, AudioParameterFloatAttributes().withLabel ("Hz")));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "bc_mono", 1 }, "Bass Mono Below", NormalisableRange<float> (0, 300, 0, 0.7f), 0, AudioParameterFloatAttributes().withLabel ("Hz")));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "bc_punch", 1 }, "Bass Punch-Warm", NormalisableRange<float> (-1, 1), 0));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "bc_tail", 1 }, "Bass Tail", NormalisableRange<float> (-1, 1), 0));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "bc_thr", 1 }, "Bass Peak Threshold", NormalisableRange<float> (-30, 0, 0.1f), 0, AudioParameterFloatAttributes().withLabel ("dB")));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "bc_dirty", 1 }, "Bass Clean-Dirty", NormalisableRange<float> (0, 1), 0));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "bc_harm", 1 }, "Bass Harmonics", NormalisableRange<float> (0, 1), 0));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "bc_bal", 1 }, "Bass Balance", NormalisableRange<float> (-6, 6, 0.1f), 0, AudioParameterFloatAttributes().withLabel ("dB")));
    // eq dinamico
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "eq_on", 1 }, "EQ On", false));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "eq_delta", 1 }, "EQ Delta", false));
    {
        // 12 bande libere stile Pro-Q: le prime 6 accese (piatte) come prima, le altre si creano con doppio clic
        static const float df[kEqBands] = { 45, 150, 500, 2000, 6000, 12000, 80, 300, 1000, 3000, 8000, 16000 };
        static const int dt[kEqBands] = { jmx::BT_LSHELF, jmx::BT_BELL, jmx::BT_BELL, jmx::BT_BELL, jmx::BT_BELL, jmx::BT_HSHELF, 0, 0, 0, 0, 0, 0 };
        for (int b = 0; b < kEqBands; ++b)
        {
            const String P = "EQ " + String (b + 1) + " ";
            l.add (std::make_unique<AudioParameterBool> (ParameterID { eqId (b, "on"), 1 }, P + "On", b < 6));
            l.add (std::make_unique<AudioParameterChoice> (ParameterID { eqId (b, "type"), 1 }, P + "Type", eqTypeNames(), dt[b]));
            l.add (std::make_unique<AudioParameterChoice> (ParameterID { eqId (b, "ch"), 1 }, P + "Channel", eqChNames(), 0));
            l.add (std::make_unique<AudioParameterChoice> (ParameterID { eqId (b, "slope"), 1 }, P + "Slope", StringArray { "12 dB/ott", "24 dB/ott", "48 dB/ott", "96 dB/ott" }, 1));
            l.add (std::make_unique<AudioParameterFloat> (ParameterID { eqId (b, "f"), 1 }, P + "Freq", NormalisableRange<float> (20, 20000, 0, 0.25f), df[b], AudioParameterFloatAttributes().withLabel ("Hz")));
            l.add (std::make_unique<AudioParameterFloat> (ParameterID { eqId (b, "g"), 1 }, P + "Gain", NormalisableRange<float> (-18, 18, 0.1f), 0, AudioParameterFloatAttributes().withLabel ("dB")));
            l.add (std::make_unique<AudioParameterFloat> (ParameterID { eqId (b, "q"), 1 }, P + "Q", NormalisableRange<float> (0.1f, 18, 0, 0.3f), 0.9f));
            l.add (std::make_unique<AudioParameterBool> (ParameterID { eqId (b, "dyn"), 1 }, P + "Dynamic", false));
            l.add (std::make_unique<AudioParameterFloat> (ParameterID { eqId (b, "thr"), 1 }, P + "Threshold", NormalisableRange<float> (-60, 0, 0.1f), -30, AudioParameterFloatAttributes().withLabel ("dB")));
            l.add (std::make_unique<AudioParameterFloat> (ParameterID { eqId (b, "rng"), 1 }, P + "Range", NormalisableRange<float> (-18, 18, 0.1f), -6, AudioParameterFloatAttributes().withLabel ("dB")));
        }
    }
    // PULTEC
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "pq_on", 1 }, "Pultec On", false));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "pq_delta", 1 }, "Pultec Delta", false));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "pq_lf", 1 }, "Pultec Low Freq", StringArray { "20 Hz", "30 Hz", "60 Hz", "100 Hz" }, 2));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "pq_lb", 1 }, "Pultec Low Boost", NormalisableRange<float> (0, 10, 0.05f), 0));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "pq_la", 1 }, "Pultec Low Atten", NormalisableRange<float> (0, 10, 0.05f), 0));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "pq_hf", 1 }, "Pultec High Freq", StringArray { "3 kHz", "4 kHz", "5 kHz", "8 kHz", "10 kHz", "12 kHz", "16 kHz" }, 4));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "pq_hb", 1 }, "Pultec High Boost", NormalisableRange<float> (0, 10, 0.05f), 0));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "pq_bw", 1 }, "Pultec Bandwidth", NormalisableRange<float> (0, 10, 0.05f), 5));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "pq_as", 1 }, "Pultec Atten Sel", StringArray { "5 kHz", "10 kHz", "20 kHz" }, 1));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "pq_ha", 1 }, "Pultec High Atten", NormalisableRange<float> (0, 10, 0.05f), 0));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "pq_tube", 1 }, "Pultec Tube", NormalisableRange<float> (0, 1), 0));
    // VARI-MU
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "vm_on", 1 }, "VariMu On", false));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "vm_delta", 1 }, "VariMu Delta", false));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "vm_thr", 1 }, "VariMu Threshold", NormalisableRange<float> (-40, 0, 0.1f), -18, AudioParameterFloatAttributes().withLabel ("dB")));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "vm_mode", 1 }, "VariMu Mode", StringArray { "COMPRESS", "LIMIT" }, 0));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "vm_att", 1 }, "VariMu Attack", StringArray { "1 ms", "5 ms", "15 ms", "30 ms", "70 ms" }, 2));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "vm_rel", 1 }, "VariMu Recovery", StringArray { "0.2 s", "0.4 s", "0.6 s", "0.8 s", "1.2 s", "2 s", "AUTO" }, 6));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "vm_hpf", 1 }, "VariMu SC HPF", StringArray { "OFF", "60 Hz", "100 Hz", "150 Hz" }, 2));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "vm_makeup", 1 }, "VariMu Makeup", NormalisableRange<float> (-6, 12, 0.1f), 0, AudioParameterFloatAttributes().withLabel ("dB")));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "vm_auto", 1 }, "VariMu Auto Makeup", true));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "vm_mix", 1 }, "VariMu Mix", NormalisableRange<float> (0, 1), 1));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "vm_tube", 1 }, "VariMu Tube", NormalisableRange<float> (0, 1), 0.3f));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "vm_ms", 1 }, "VariMu Mid Side", false));
    // EQ 432
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "e4_on", 1 }, "EQ432 On", false));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "e4_delta", 1 }, "EQ432 Delta", false));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "e4_range", 1 }, "EQ432 Range", StringArray { "6 dB", "9 dB", "12 dB" }, 1));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "e4_col", 1 }, "EQ432 Colour", NormalisableRange<float> (0, 1), 0));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "e4_sh1", 1 }, "EQ432 Low Shelf", true));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "e4_sh5", 1 }, "EQ432 High Shelf", true));
    {
        static const char* bn[k432Bands] = { "Low", "Low Mid", "Mid", "High Mid", "High" };
        static const float df[k432Bands] = { 60, 250, 1000, 4000, 12000 }, dq[k432Bands] = { 0.7f, 0.8f, 0.8f, 0.8f, 0.7f };
        for (int b = 0; b < k432Bands; ++b)
        {
            const String id = "e4_" + String (b + 1), nm = String ("EQ432 ") + bn[b] + " ";
            l.add (std::make_unique<AudioParameterFloat> (ParameterID { id + "f", 1 }, nm + "Freq", eq432Freq (b), df[b], AudioParameterFloatAttributes().withLabel ("Hz")));
            l.add (std::make_unique<AudioParameterFloat> (ParameterID { id + "g", 1 }, nm + "Gain", NormalisableRange<float> (-1, 1, 0.005f), 0));
            l.add (std::make_unique<AudioParameterFloat> (ParameterID { id + "q", 1 }, nm + "Bandwidth", NormalisableRange<float> (0.3f, 3.0f, 0, 0.5f), dq[b]));
        }
    }
    // de-esser
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "ds_on", 1 }, "DeEss On", false));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "ds_delta", 1 }, "DeEss Delta", false));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "ds_lo", 1 }, "DeEss From", NormalisableRange<float> (1500, 14000, 0, 0.5f), 5000, AudioParameterFloatAttributes().withLabel ("Hz")));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "ds_hi", 1 }, "DeEss To", NormalisableRange<float> (3000, 20000, 0, 0.5f), 20000, AudioParameterFloatAttributes().withLabel ("Hz")));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "ds_thr", 1 }, "DeEss Threshold", NormalisableRange<float> (-30, 6, 0.1f), 6, AudioParameterFloatAttributes().withLabel ("dB")));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "ds_amt", 1 }, "DeEss Amount", NormalisableRange<float> (0, 1), 0.4f));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "ds_soft", 1 }, "DeEss Softness", NormalisableRange<float> (0, 1), 0.6f));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "ds_ch", 1 }, "DeEss Channel", StringArray { "STEREO", "MID", "SIDE" }, 0));
    // bus comp
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "bus_on", 1 }, "Bus On", false));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "bus_delta", 1 }, "Bus Delta", false));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "bus_thr", 1 }, "Bus Threshold", NormalisableRange<float> (-30, 0, 0.1f), 0, AudioParameterFloatAttributes().withLabel ("dB")));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "bus_range", 1 }, "Bus Range", NormalisableRange<float> (1, 20, 0.1f), 10, AudioParameterFloatAttributes().withLabel ("dB")));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "bus_ratio", 1 }, "Bus Ratio", StringArray { "1.5", "2", "4", "10" }, 1));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "bus_att", 1 }, "Bus Attack", StringArray { "0.1", "0.3", "1", "3", "10", "30" }, 4));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "bus_rel", 1 }, "Bus Release", StringArray { "0.1", "0.3", "0.6", "1.2", "AUTO" }, 4));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "bus_makeup", 1 }, "Bus Makeup", NormalisableRange<float> (-6, 12, 0.1f), 0, AudioParameterFloatAttributes().withLabel ("dB")));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "bus_auto", 1 }, "Bus Auto Makeup", true));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "bus_mix", 1 }, "Bus Mix", NormalisableRange<float> (0, 1), 1));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "bus_hpf", 1 }, "Bus SC HPF", NormalisableRange<float> (0, 250, 0, 0.6f), 0, AudioParameterFloatAttributes().withLabel ("Hz")));
    // multibanda
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "mb_on", 1 }, "Multiband On", false));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "mb_delta", 1 }, "Multiband Delta", false));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "mb_x1", 1 }, "Multiband X1", NormalisableRange<float> (40, 500, 0, 0.5f), 120, AudioParameterFloatAttributes().withLabel ("Hz")));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "mb_x2", 1 }, "Multiband X2", NormalisableRange<float> (800, 10000, 0, 0.5f), 2500, AudioParameterFloatAttributes().withLabel ("Hz")));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "mb_speed", 1 }, "Multiband Speed", NormalisableRange<float> (0, 1), 0.5f));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "mb_mix", 1 }, "Multiband Mix", NormalisableRange<float> (0, 1), 1));
    for (int b = 0; b < kMbBands; ++b)
    {
        const String id = "mb" + String (b + 1) + "_", P = "Multiband " + String (b + 1) + " ";
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { id + "thr", 1 }, P + "Threshold", NormalisableRange<float> (-48, 0, 0.1f), 0, AudioParameterFloatAttributes().withLabel ("dB")));
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { id + "ratio", 1 }, P + "Ratio", NormalisableRange<float> (1, 20, 0, 0.4f), 3));
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { id + "up", 1 }, P + "Upward", NormalisableRange<float> (0, 1), 0));
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { id + "gain", 1 }, P + "Gain", NormalisableRange<float> (-12, 12, 0.1f), 0, AudioParameterFloatAttributes().withLabel ("dB")));
        l.add (std::make_unique<AudioParameterBool> (ParameterID { id + "solo", 1 }, P + "Solo", false));
    }
    // imager
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "im_on", 1 }, "Imager On", false));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "im_delta", 1 }, "Imager Delta", false));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "im_x1", 1 }, "Imager X1", NormalisableRange<float> (40, 500, 0, 0.5f), 120, AudioParameterFloatAttributes().withLabel ("Hz")));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "im_x2", 1 }, "Imager X2", NormalisableRange<float> (300, 4000, 0, 0.5f), 900, AudioParameterFloatAttributes().withLabel ("Hz")));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "im_x3", 1 }, "Imager X3", NormalisableRange<float> (2000, 16000, 0, 0.5f), 5000, AudioParameterFloatAttributes().withLabel ("Hz")));
    for (int b = 0; b < kImBands; ++b)
    {
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { "im_w" + String (b + 1), 1 }, "Imager Width " + String (b + 1), NormalisableRange<float> (0, 2), 1));
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { "im_s" + String (b + 1), 1 }, "Stereoize " + String (b + 1), NormalisableRange<float> (0, 1), 0));
        l.add (std::make_unique<AudioParameterBool> (ParameterID { "im_solo" + String (b + 1), 1 }, "Imager Solo " + String (b + 1), false));
    }
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "im_smode", 1 }, "Stereoize Mode", StringArray { "MODO I", "MODO II" }, 0));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "im_mono", 1 }, "Mono Check", false));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "im_safe", 1 }, "Imager Mono Safe", true));
    // tape
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "tp_on", 1 }, "Tape On", false));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "tp_delta", 1 }, "Tape Delta", false));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "tp_model", 1 }, "Tape Machine", StringArray { "30 IPS", "15 IPS", "CASSETTA" }, 1));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "tp_drive", 1 }, "Tape Drive", NormalisableRange<float> (0, 1), 0));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "tp_bias", 1 }, "Tape Bias", NormalisableRange<float> (-1, 1), 0));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "tp_noise", 1 }, "Tape Noise", NormalisableRange<float> (0, 1), 0));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "tp_mix", 1 }, "Tape Mix", NormalisableRange<float> (0, 1), 1));
    // exciter
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "ex_on", 1 }, "Exciter On", false));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "ex_delta", 1 }, "Exciter Delta", false));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "ex_x1", 1 }, "Exciter X1", NormalisableRange<float> (80, 1000, 0, 0.5f), 250, AudioParameterFloatAttributes().withLabel ("Hz")));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "ex_x2", 1 }, "Exciter X2", NormalisableRange<float> (1500, 12000, 0, 0.5f), 4000, AudioParameterFloatAttributes().withLabel ("Hz")));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "ex_mix", 1 }, "Exciter Mix", NormalisableRange<float> (0, 1), 1));
    {
        StringArray em; for (int m = 0; m < kExModes; ++m) em.add (exName (m));
        const int dm[kExBands] = { VALVOLA, NASTRO, BRILLANTE };
        for (int b = 0; b < kExBands; ++b)
        {
            const String id = "ex" + String (b + 1) + "_";
            l.add (std::make_unique<AudioParameterFloat> (ParameterID { id + "amt", 1 }, "Exciter " + String (b + 1) + " Amount", NormalisableRange<float> (0, 1), 0));
            l.add (std::make_unique<AudioParameterChoice> (ParameterID { id + "mode", 1 }, "Exciter " + String (b + 1) + " Mode", em, dm[b]));
            l.add (std::make_unique<AudioParameterBool> (ParameterID { id + "solo", 1 }, "Exciter " + String (b + 1) + " Solo", false));
        }
    }
    // maximizer
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "mx_on", 1 }, "Maximizer On", false));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "mx_delta", 1 }, "Maximizer Delta", false));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "mx_gain", 1 }, "Maximizer Gain", NormalisableRange<float> (0, 20, 0.01f), 0, AudioParameterFloatAttributes().withLabel ("dB")));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "mx_style", 1 }, "Maximizer Style", st, 0));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "mx_clip", 1 }, "Maximizer Clip", NormalisableRange<float> (0, 1), 0));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "mx_char", 1 }, "Maximizer Character", NormalisableRange<float> (0, 1), 0.5f));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "mx_link", 1 }, "Maximizer Stereo Link", NormalisableRange<float> (0, 1), 1));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "mx_learn", 1 }, "Maximizer Auto Target", false));
    // limiter
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "lm_on", 1 }, "Limiter On", false));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "lm_gain", 1 }, "Limiter Gain", NormalisableRange<float> (0, 12, 0.01f), 0, AudioParameterFloatAttributes().withLabel ("dB")));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "lm_delta", 1 }, "Limiter Delta", false));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "lm_ceil", 1 }, "Ceiling", NormalisableRange<float> (-6, 0, 0.1f), -1, AudioParameterFloatAttributes().withLabel ("dBTP")));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "lm_rel", 1 }, "Limiter Release", NormalisableRange<float> (5, 1000, 0, 0.4f), 80, AudioParameterFloatAttributes().withLabel ("ms")));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "lm_auto", 1 }, "Limiter Auto Release", true));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "lm_tp", 1 }, "True Peak", true));
    // uscita e ascolto
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "target", 1 }, "Target", tg, 0));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "norm", 1 }, "Normalized Listen", false));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "compare", 1 }, "Compare Bypass", false));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "dither", 1 }, "Dither", StringArray { "OFF", "24 BIT", "16 BIT", "16 BIT SHAPED" }, 0));
    // ---- modalita FACILE
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "mode", 1 }, "Modalita", StringArray { "FACILE", "AVANZATO" }, 0));
    auto db = [] (float v, int) { return (v > 0 ? "+" : "") + String (v, 1) + " dB"; };
    auto pc = [] (float v, int) { return String (juce::roundToInt (v * 100)) + "%"; };
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "e_loud", 1 }, "Loud", NormalisableRange<float> (0, 14, 0.01f), 0, AudioParameterFloatAttributes().withStringFromValueFunction (db)));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "e_pres", 1 }, "Presence (soft-hard)", NormalisableRange<float> (0, 1), 0.4f, AudioParameterFloatAttributes().withStringFromValueFunction (pc)));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "e_speed", 1 }, "Speed (smooth-fast)", NormalisableRange<float> (0, 1), 0.5f, AudioParameterFloatAttributes().withStringFromValueFunction (pc)));
    for (auto* id : { "e_low", "e_mid", "e_high" }) l.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, String (id).substring (2).toUpperCase(), NormalisableRange<float> (-9, 9, 0.1f), 0, AudioParameterFloatAttributes().withStringFromValueFunction (db)));
    for (auto* id : { "e_thick", "e_warm", "e_silk", "e_glue", "e_calm" }) l.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, String (id).substring (2).toUpperCase(), NormalisableRange<float> (0, 1), 0, AudioParameterFloatAttributes().withStringFromValueFunction (pc)));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "e_wide", 1 }, "Wide", NormalisableRange<float> (-1, 1), 0, AudioParameterFloatAttributes().withStringFromValueFunction (pc)));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "e_monob", 1 }, "Mono Bass", false));
    // carattere del LOUD (come gli algoritmi dei limiter moderni) e ascolto della differenza
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "e_style", 1 }, "Loud Style", StringArray { "MODERN", "AGGRESSIVE", "SMOOTH", "SAFE", "CLASSIC" }, 0));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "e_delta", 1 }, "Delta", false));
    return l;
}

Processor::Processor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true).withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "JMS", layout())
{
    { for (int k = 0; k < kNumMods; ++k) order[k] = defaultOrder()[(size_t) k]; }
    rackFromOn();
    for (int i = 0; i < kHist; ++i) { hIn[i] = -60; hOut[i] = -60; hGrM[i] = 0; hGrL[i] = 0; hS[i] = -60; hRaw[i] = -100; hRawR[i] = -100; hOutR[i] = -100; hPpq[i] = -1; }
    startTimerHz (5);
}

bool Processor::isBusesLayoutSupported (const BusesLayout& l) const
{
    return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo() && l.getMainInputChannelSet() == l.getMainOutputChannelSet();
}

void Processor::prepareToPlay (double s, int maxBlock)
{
    sr = s;
    const int mb = juce::jmax (64, maxBlock);
    mRawA.assign ((size_t) mb * 2, 0.0f); mRawQ.assign ((size_t) mb * 2, 0.0f); mInA.assign ((size_t) mb * 2, 0.0f);
    mx.prepare (s, mb); lim.prepare (s); easyEng.prepare (s);
    bass.prepare (s); deq.prepare (s); peq.prepare (s, mb); e432.prepare (s); pult.prepare (s); vmu.prepare (s); img.prepare (s); dess.prepare (s); bus.prepare (s); mband.prepare (s); tape.prepare (s); exc.prepare (s);
    for (auto& d : deltaVar) { d.b.assign ((size_t) (mx.latency() + lim.latency() + 1), 0.0f); d.w = 0; }
    for (auto& d : mxBy) d.prepare (mx.latency());
    for (auto& d : limBy) d.prepare (lim.latency());
    for (auto& d : mxIn) d.prepare (mx.latency());
    for (auto& d : deltaPad) d.prepare (lim.latency());
    const int total = mx.latency() + lim.latency();
    for (auto& d : dry) d.prepare (total);
    setLatencySamples (total);
    loudIn.prepare (s); loudOut.prepare (s); dith.reset(); for (auto& t : tpOut) t.reset();
    tmp.setSize (2, mb); keep.setSize (2, mb);
    matchSm = normSm = 1; tpHold = -100;
}

void Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals nd;
    const int n = buffer.getNumSamples(); if (n == 0) return;
    if (tmp.getNumSamples() < n) { tmp.setSize (2, n, false, false, true); keep.setSize (2, n, false, false, true); }
    float* L = buffer.getWritePointer (0); float* R = buffer.getWritePointer (1);
    if (resetReq.exchange (false)) { loudOut.resetIntegrated(); tpHold = -100; }

    const float trim = dbToG (value ("trim"));
    float pi[2] = { 0, 0 };
    if ((int) mRawA.size() < n) { mRawA.resize ((size_t) n); mRawQ.resize ((size_t) n); mInA.resize ((size_t) n); }
    {   // tempo del pezzo all'inizio del blocco: le colonne dell'onda portano la loro posizione in battiti (griglia a tempo)
        hostPlay = false;
        if (auto* ph = getPlayHead())
            if (auto pos = ph->getPosition())
            {
                if (auto b = pos->getBpm()) hostBpm = (float) *b;
                if (auto q = pos->getPpqPosition()) { blockPpq = *q; hostPlay = pos->getIsPlaying(); }
                if (auto ts = pos->getTimeSignature()) hostNum = juce::jmax (1, ts->numerator);
            }
    }
    for (int i = 0; i < n; ++i)
    {
        mRawA[(size_t) i] = juce::jmax (std::abs (L[i]), std::abs (R[i])); mRawQ[(size_t) i] = 0.5f * (L[i] * L[i] + R[i] * R[i]);   // il wav cosi' com'e' (senza master)
        L[i] *= trim; R[i] *= trim;
        pi[0] = juce::jmax (pi[0], std::abs (L[i])); pi[1] = juce::jmax (pi[1], std::abs (R[i])); mInA[(size_t) i] = juce::jmax (std::abs (L[i]), std::abs (R[i]));
        loudIn.push (L[i], R[i]);
    }
    // copia pulita (per il confronto a volume uguale)
    float* kL = keep.getWritePointer (0); float* kR = keep.getWritePointer (1);
    for (int i = 0; i < n; ++i) { kL[i] = dry[0] (L[i]); kR[i] = dry[1] (R[i]); }

    ringIn.push (L, R, n);
    // ---- catena nell'ordine scelto (i moduli si trascinano nella fila in alto)
    float* tL = tmp.getWritePointer (0); float* tR = tmp.getWritePointer (1);
    int deltaFrom = -1, latSoFar = 0;
    auto copyIn = [&] { for (int i = 0; i < n; ++i) { tL[i] = L[i]; tR[i] = R[i]; } };
    auto subIn = [&] { for (int i = 0; i < n; ++i) { L[i] -= tL[i]; R[i] -= tR[i]; } };
    mx.gr = 0; lim.gr = 0;
    const bool isEasy = easy();
    if (isEasy)
    {   // ---- modalita FACILE: motore suo, poi LOUD (maximizer) e true peak
        EasySettings E; E.low = value ("e_low"); E.mid = value ("e_mid"); E.high = value ("e_high"); E.calm = value ("e_calm"); E.thick = value ("e_thick");
        E.warm = value ("e_warm"); E.silk = value ("e_silk"); E.glue = value ("e_glue"); E.wide = value ("e_wide"); E.monoBass = value ("e_monob") > 0.5f;
        { const int le = eqListen.load(); E.listenDelta = le >= 3 && le <= 5; E.listenBand = E.listenDelta ? le - 3 : -1; }
        easyEng.process (L, R, n, E);
        calmNow = easyEng.calmNow; glueGr = easyEng.glueGr;
        const float pres = value ("e_pres"), spd = value ("e_speed");
        const int lst = eqListen.load();   // ascolto di una banda dell'EQ: 0..2 solo, 3..5 delta
        if (E.listenDelta)
        {   // si sente solo cosa fa quella banda: niente LOUD, solo i ritardi per restare allineati
            for (int i = 0; i < n; ++i) { L[i] = limBy[0] (mxBy[0] (L[i])); R[i] = limBy[1] (mxBy[1] (R[i])); }
        }
        else
        {
            // LOUD: limiter "adattivo" in 5 caratteri. PRESENZA = quanto lavora il clipper sui transienti (adattivo: piu limiti, piu clippa),
            // VELOCITA = rilascio (il motore ha gia due tempi e sceglie da solo in base al materiale)
            const int st = (int) value ("e_style");
            MaxSettings ms; ms.gainDb = value ("e_loud"); ms.ceilDb = value ("lm_ceil"); ms.link = 1;
            const float adapt = juce::jlimit (0.3f, 1.0f, 0.3f + grSm / 5.0f);
            switch (st)
            {
                case 1:  ms.style = AGGRESSIVO; ms.clip = (0.35f + 0.6f * pres) * adapt; ms.character = 0.55f + 0.45f * spd; break;   // AGGRESSIVE
                case 2:  ms.style = MORBIDO;    ms.clip = 0.15f * pres * adapt; ms.character = 0.35f * spd; break;                     // SMOOTH
                case 3:  ms.style = PULITO;     ms.clip = 0.0f; ms.character = 0.2f + 0.4f * spd; break;                               // SAFE
                case 4:  ms.style = PULITO;     ms.clip = 0.0f; ms.character = 0.35f + 0.5f * spd; break;                              // CLASSIC (alla L2)
                default: ms.style = PUNCH;      ms.clip = (0.1f + 0.7f * pres) * adapt; ms.character = spd; break;                     // MODERN
            }
            if (ms.gainDb > 0.01f) mx.process (L, R, n, ms);
            else for (int i = 0; i < n; ++i) { L[i] = mxBy[0] (L[i]); R[i] = mxBy[1] (R[i]); }   // LOUD a zero: solo il ritardo, suono intatto
            LimSettings ls; ls.ceilDb = value ("lm_ceil"); ls.relMs = 80; ls.autoRel = true; ls.truePeak = true;
            lim.process (L, R, n, ls);
            // SOLO di una banda: si sente solo la zona di frequenze di BASSI / MEDI / ALTI, a valle di tutto
            if (lst >= 0 && lst < 3)
            {
                if (lst != soloKey)
                {
                    soloKey = lst;
                    for (int c = 0; c < 2; ++c)
                    {
                        if (lst == 0) { bandF[c][0].set (Biquad::HCUT, 250, 0, 0.707, sr); bandF[c][1].set (Biquad::HCUT, 250, 0, 0.707, sr); }
                        else if (lst == 1) { bandF[c][0].set (Biquad::LCUT, 300, 0, 0.707, sr); bandF[c][1].set (Biquad::HCUT, 4500, 0, 0.707, sr); }
                        else { bandF[c][0].set (Biquad::LCUT, 5000, 0, 0.707, sr); bandF[c][1].set (Biquad::LCUT, 5000, 0, 0.707, sr); }
                    }
                }
                for (int i = 0; i < n; ++i) { L[i] = bandF[0][1] (bandF[0][0] (L[i])); R[i] = bandF[1][1] (bandF[1][0] (R[i])); }
            }
            else soloKey = -1;
        }
    }
    for (int k = 0; k < (isEasy ? 0 : kNumMods); ++k)
    {
        const int id = order[k].load();
        const bool on = value (onParam (id)) > 0.5f, d = on && deltaFrom < 0 && value (deltaParam (id)) > 0.5f;
        if (deltaFrom >= 0) { if (id == MAXIM) latSoFar += 0; continue; }
        switch (id)
        {
            case BASS:
                if (! on) { bass.gainDb = 0; break; }
                { BassSettings b; b.freq = value ("bc_freq"); b.monoBelow = value ("bc_mono"); b.punch = value ("bc_punch"); b.tail = value ("bc_tail");
                  b.harm = value ("bc_harm"); b.balance = value ("bc_bal"); b.thr = value ("bc_thr"); b.dirty = value ("bc_dirty"); b.delta = d;
                  bass.process (L, R, n, b); }
                break;
            case DYNEQ:
                if (! on) break;
                {
                    jmx::EqSet S; S.live = true;   // live: niente modo spettrale (nessuna latenza in piu' nel master)
                    const int sb = eqSolo.load();
                    for (int b = 0; b < kEqBands; ++b)
                    {
                        auto& B = S.b[b];
                        B.on = value (eqId (b, "on")) > 0.5f; B.type = (int) value (eqId (b, "type")); B.ch = (int) value (eqId (b, "ch")); B.slope = (int) value (eqId (b, "slope"));
                        B.f = value (eqId (b, "f")); B.g = value (eqId (b, "g")); B.q = value (eqId (b, "q"));
                        B.dyn = value (eqId (b, "dyn")) > 0.5f; B.thr = value (eqId (b, "thr")); B.range = value (eqId (b, "rng"));
                        B.solo = ! d && b == sb && B.on;
                    }
                    if (d) copyIn();
                    peq.process (L, R, n, S);
                    if (d) subIn();
                    if (! d && sb >= 0 && sb < kEqBands && S.b[sb].on) deltaFrom = id;   // in SOLO il resto della catena si salta, come per il delta
                }
                break;
            case DEESS:
                if (! on) break;
                { DeEssSettings e; e.lo = value ("ds_lo"); e.hi = juce::jmax (value ("ds_hi"), e.lo * 1.3f); e.thr = value ("ds_thr"); e.amount = value ("ds_amt");
                  e.soft = value ("ds_soft"); e.ch = (int) value ("ds_ch"); e.delta = d; dess.process (L, R, n, e); }
                break;
            case BUSCOMP:
                if (! on) break;
                { BusSettings b; b.thr = value ("bus_thr"); b.ratio = (int) value ("bus_ratio"); b.att = (int) value ("bus_att"); b.rel = (int) value ("bus_rel");
                  b.makeup = value ("bus_makeup"); b.autoMakeup = value ("bus_auto") > 0.5f; b.mix = value ("bus_mix"); b.scHpf = value ("bus_hpf"); b.range = value ("bus_range");
                  if (d) copyIn(); bus.process (L, R, n, b); if (d) subIn(); }
                break;
            case MULTI:
                if (! on) break;
                {
                    MbSettings m; m.x1 = value ("mb_x1"); m.x2 = juce::jmax (value ("mb_x2"), m.x1 * 2); m.speed = value ("mb_speed"); m.mix = value ("mb_mix"); m.delta = d;
                    for (int b = 0; b < kMbBands; ++b)
                    {
                        const juce::String mid = "mb" + juce::String (b + 1) + "_";
                        m.b[b].thr = value ((mid + "thr").toRawUTF8()); m.b[b].ratio = value ((mid + "ratio").toRawUTF8()); m.b[b].up = value ((mid + "up").toRawUTF8());
                        m.b[b].gain = value ((mid + "gain").toRawUTF8()); m.b[b].solo = value ((mid + "solo").toRawUTF8()) > 0.5f;
                    }
                    mband.process (L, R, n, m);
                }
                break;
            case TAPE:
                if (! on) break;
                { TapeSettings t; t.model = (int) value ("tp_model"); t.drive = value ("tp_drive"); t.bias = value ("tp_bias"); t.noise = value ("tp_noise"); t.mix = value ("tp_mix"); t.delta = d;
                  tape.process (L, R, n, t); }
                break;
            case PULTEC:
                if (! on) break;
                {
                    PultecSettings q; q.lf = (int) value ("pq_lf"); q.hf = (int) value ("pq_hf"); q.as = (int) value ("pq_as");
                    q.lb = value ("pq_lb"); q.la = value ("pq_la"); q.hb = value ("pq_hb"); q.bw = value ("pq_bw"); q.ha = value ("pq_ha"); q.tube = value ("pq_tube");
                    if (d) copyIn();
                    pult.process (L, R, n, q);
                    if (d) subIn();
                }
                break;
            case VARIMU:
                if (! on) break;
                {
                    VariMuSettings v; v.thr = value ("vm_thr"); v.makeup = value ("vm_makeup"); v.mix = value ("vm_mix"); v.tube = value ("vm_tube");
                    v.mode = (int) value ("vm_mode"); v.att = (int) value ("vm_att"); v.rel = (int) value ("vm_rel"); v.hpf = (int) value ("vm_hpf");
                    v.autoMakeup = value ("vm_auto") > 0.5f; v.ms = value ("vm_ms") > 0.5f; v.delta = d;
                    vmu.process (L, R, n, v); vmGr = vmu.gr;
                }
                break;
            case EQ432:
                if (! on) break;
                {
                    Eq432Settings e; e.range = (int) value ("e4_range"); e.colour = value ("e4_col"); e.shelf[0] = value ("e4_sh1") > 0.5f; e.shelf[1] = value ("e4_sh5") > 0.5f;
                    for (int b = 0; b < k432Bands; ++b)
                    {
                        const juce::String id = "e4_" + juce::String (b + 1);
                        e.f[b] = value ((id + "f").toRawUTF8()); e.g[b] = value ((id + "g").toRawUTF8()); e.q[b] = value ((id + "q").toRawUTF8());
                    }
                    if (d) copyIn();
                    e432.process (L, R, n, e);
                    if (d) subIn();
                }
                break;
            case EXCITER:
                if (! on) break;
                {
                    ExSettings e; e.x1 = value ("ex_x1"); e.x2 = juce::jmax (value ("ex_x2"), e.x1 * 2); e.mix = value ("ex_mix"); e.delta = d;
                    for (int b = 0; b < kExBands; ++b)
                    {
                        const juce::String id = "ex" + juce::String (b + 1) + "_";
                        e.amt[b] = value ((id + "amt").toRawUTF8()); e.mode[b] = (int) value ((id + "mode").toRawUTF8()); e.solo[b] = value ((id + "solo").toRawUTF8()) > 0.5f;
                    }
                    exc.process (L, R, n, e);
                }
                break;
            case IMAGER:
                if (! on) break;
                {
                    ImagerSettings S; S.x[0] = value ("im_x1"); S.x[1] = juce::jmax (value ("im_x2"), S.x[0] * 1.5f); S.x[2] = juce::jmax (value ("im_x3"), S.x[1] * 1.5f);
                    for (int b = 0; b < kImBands; ++b)
                    {
                        S.w[b] = value (("im_w" + juce::String (b + 1)).toRawUTF8()); S.stz[b] = value (("im_s" + juce::String (b + 1)).toRawUTF8());
                        S.solo[b] = value (("im_solo" + juce::String (b + 1)).toRawUTF8()) > 0.5f;
                    }
                    S.mode = (int) value ("im_smode"); S.delta = d; S.safe = value ("im_safe") > 0.5f;
                    img.process (L, R, n, S);
                }
                break;
            case MAXIM:
            {
                MaxSettings ms; ms.gainDb = value ("mx_gain"); ms.style = (int) value ("mx_style"); ms.clip = value ("mx_clip"); ms.character = value ("mx_char");
                ms.link = value ("mx_link"); ms.ceilDb = value ("lm_ceil");
                copyIn();
                if (on) mx.process (L, R, n, ms);
                else for (int i = 0; i < n; ++i) { L[i] = mxBy[0] (L[i]); R[i] = mxBy[1] (R[i]); }
                if (d) { const float g = dbToG (ms.gainDb); for (int i = 0; i < n; ++i) { L[i] -= mxIn[0] (tL[i]) * g; R[i] -= mxIn[1] (tR[i]) * g; } }
                else for (int i = 0; i < n; ++i) { mxIn[0] (tL[i]); mxIn[1] (tR[i]); }
                latSoFar += mx.latency();
                break;
            }
            case LIMIT:
            {
                LimSettings ls; ls.ceilDb = value ("lm_ceil"); ls.relMs = value ("lm_rel"); ls.autoRel = value ("lm_auto") > 0.5f; ls.truePeak = value ("lm_tp") > 0.5f;
                if (on) { const float g = dbToG (value ("lm_gain")); if (g != 1.0f) for (int i = 0; i < n; ++i) { L[i] *= g; R[i] *= g; } }
                copyIn();
                if (on) lim.process (L, R, n, ls);
                else for (int i = 0; i < n; ++i) { L[i] = limBy[0] (L[i]); R[i] = limBy[1] (R[i]); }
                if (d) for (int i = 0; i < n; ++i) { L[i] -= deltaPad[0] (tL[i]); R[i] -= deltaPad[1] (tR[i]); }
                else for (int i = 0; i < n; ++i) { deltaPad[0] (tL[i]); deltaPad[1] (tR[i]); }
                latSoFar += lim.latency();
                break;
            }
            default: break;
        }
        if (d) deltaFrom = id;
    }
    // delta: il resto della catena e saltato, si ritarda quanto mancava per restare allineati
    const int remain = mx.latency() + lim.latency() - latSoFar;
    for (int c = 0; c < 2; ++c)
    {
        float* x = c ? R : L; auto& vd = deltaVar[c];
        for (int i = 0; i < n; ++i) { vd.b[(size_t) vd.w] = x[i]; if (deltaFrom >= 0) x[i] = vd.b[(size_t) ((vd.w - remain + (int) vd.b.size()) % (int) vd.b.size())]; vd.w = (vd.w + 1) % (int) vd.b.size(); }
    }
    exMeter = value ("ex_on") > 0.5f ? exc.meter : 0.0f;
    bassGain = bass.gainDb; bassLow = bass.lowShare; bassPeak = value ("bc_on") > 0.5f ? bass.peakGr : 0.0f;
    dsGr = value ("ds_on") > 0.5f ? dess.gr : 0.0f; dsRel = dess.relNow; busGr = value ("bus_on") > 0.5f ? bus.gr : 0.0f;
    for (int b = 0; b < kMbBands; ++b) { mbLevel[b] = mband.level[b]; mbDown[b] = value ("mb_on") > 0.5f ? mband.down[b] : 0.0f; mbUp[b] = value ("mb_on") > 0.5f ? mband.up[b] : 0.0f; }
    for (int b = 0; b < kEqBands; ++b) { eqDyn[b] = value ("eq_on") > 0.5f ? peq.dynUi[b].load() : 0.0f; eqLvl[b] = 0; }
    for (int b = 0; b < kImBands; ++b) { imCorr[b] = img.corr (b); imLim[b] = value ("im_on") > 0.5f ? img.limited (b) : 0.0f; }

    // ---- misure d'uscita (prima di confronto e ascolto normalizzato)
    float po[2] = { 0, 0 }; float tpb = 0;
    for (int i = 0; i < n; ++i)
    {
        loudOut.push (L[i], R[i]);
        po[0] = juce::jmax (po[0], std::abs (L[i])); po[1] = juce::jmax (po[1], std::abs (R[i]));
        sqL += L[i] * L[i]; sqR += R[i] * R[i];
        tpb = juce::jmax (tpb, juce::jmax (tpOut[0].push (L[i]), tpOut[1].push (R[i])));
    }
    tpHold = juce::jmax (tpHold, gToDb (tpb));
    ringOut.push (L, R, n);
    { double lr = 0, ll = 0, rr = 0; for (int i = 0; i < n; ++i) { lr += L[i] * R[i]; ll += L[i] * L[i]; rr += R[i] * R[i]; }
      const double k = std::exp (-(double) n / (0.3 * sr)); cLR = k * cLR + (1 - k) * lr; cLL = k * cLL + (1 - k) * ll; cRR = k * cRR + (1 - k) * rr;
      corrAll = (float) (cLR / std::sqrt (juce::jmax (1e-12, cLL * cRR))); }

    // ---- confronto a volume uguale: il pulito alzato/abbassato alla loudness del master
    const bool compare = value ("compare") > 0.5f;
    float mdb = 0; if (loudIn.S > -60 && loudOut.S > -60) mdb = juce::jlimit (-24.0f, 24.0f, loudOut.S - loudIn.S);
    matchDb = mdb;
    if (compare)
    {
        const float tgt = dbToG (mdb), step = (tgt - matchSm) / (float) n;
        for (int i = 0; i < n; ++i) { matchSm += step; L[i] = kL[i] * matchSm; R[i] = kR[i] * matchSm; }
    }
    else matchSm = dbToG (mdb);
    // ---- DELTA: si sente solo la differenza fra il master e l'originale (portato allo stesso volume)
    if (isEasy && value ("e_delta") > 0.5f && ! compare)
        for (int i = 0; i < n; ++i) { L[i] -= kL[i] * matchSm; R[i] -= kR[i] * matchSm; }

    // ---- ascolto normalizzato (come lo abbassera la piattaforma). Solo per ascoltare!
    const auto& tg = target ((int) value ("target"));
    const float lufsRef = loudOut.I > -60 ? loudOut.I : loudOut.S;
    const float ndb = (lufsRef > -60 && lufsRef > tg.lufs) ? tg.lufs - lufsRef : 0.0f;
    normDb = ndb;
    if (value ("norm") > 0.5f)
    {
        const float tgt = dbToG (ndb), step = (tgt - normSm) / (float) n;
        for (int i = 0; i < n; ++i) { normSm += step; L[i] *= normSm; R[i] *= normSm; }
    }
    else normSm = dbToG (ndb);

    // ---- ascolto della traccia di riferimento (a volume uguale)
    if (refListen.load())
    {
        std::shared_ptr<RefTrack> rt; { const juce::SpinLock::ScopedTryLockType tl (refLock); if (tl.isLocked()) rt = ref; }
        if (rt && rt->buf.getNumSamples() > 0)
        {
            const float me = loudOut.I > -60 ? loudOut.I : loudOut.S;
            const float gdb = (refMatch.load() && me > -60 && rt->lufs > -60) ? juce::jlimit (-24.0f, 24.0f, me - rt->lufs) : 0.0f;
            refGainDb = gdb; const float g = dbToG (gdb);
            const int len = rt->buf.getNumSamples(); int p = (int) refPos.load() % len;
            for (int i = 0; i < n; ++i) { L[i] = rt->buf.getSample (0, p) * g; R[i] = rt->buf.getSample (1, p) * g; if (++p >= len) p = 0; }
            refPos = (double) p;
        }
    }
    // ---- ascolto in mono (controllo)
    if (value ("im_mono") > 0.5f) for (int i = 0; i < n; ++i) { const float m = 0.5f * (L[i] + R[i]); L[i] = R[i] = m; }
    // ---- dither (sempre l'ultimo)
    dith.process (L, R, n, (int) value ("dither"));

    // ---- misure
    lufsM = loudOut.M; lufsS = loudOut.S; lufsI = loudOut.I; lra = loudOut.lra; tpMax = tpHold; inS = loudIn.S;
    grMax = mx.gr; grLim = lim.gr;
    {   // ingresso medio (short-term, solo con musica) per il consiglio sull'INPUT; indice SAFE / OTTIMALE / CRUSH
        if (loudIn.S > -45) { inAcc += loudIn.S * n; inAccN += n; if (inAccN > (int) (sr * 0.5)) { const float a = (float) (inAcc / inAccN); inAvg = inAvg.load() < -90 ? a : inAvg.load() * 0.85f + a * 0.15f; inAcc = 0; inAccN = 0; } }
        const float k = (float) std::exp (-(double) n / (2.5 * sr)); grSm = k * grSm + (1 - k) * (mx.gr + lim.gr);
        grAvg = grSm;
        const auto& T = target ((int) value ("target"));
        const float ideal = juce::jlimit (1.5f, 5.0f, 2.0f + (-14.0f - T.lufs) * 0.5f);   // riduzione "giusta" per quel target
        const float plr = (tpHold > -60 && loudOut.S > -60) ? tpHold - loudOut.S : 14.0f;
        // tre cose insieme: quanto sei sopra il target, quanto lavora il limiter, quanta dinamica resta (PLR)
        const float sLoud = (loudOut.S - T.lufs) / 10.0f;
        const float sGr = juce::jlimit (-0.25f, 0.3f, (grSm - ideal) / (2.0f * ideal));
        const float sPlr = juce::jlimit (-0.2f, 0.3f, (9.0f - plr) * 0.05f);
        const float sc = 0.5f + sLoud + sGr + sPlr;
        crush = loudOut.S > -50 ? juce::jlimit (0.0f, 1.0f, sc) : 0.0f;
    }
    for (int c = 0; c < 2; ++c) { inPk[c] = juce::jmax (pi[c], inPk[c].load() * 0.9f); outPk[c] = juce::jmax (po[c], outPk[c].load() * 0.9f); }
    {   // livello medio per la colonna (300 ms)
        const float a = (float) std::exp (-(double) n / (0.3 * sr));
        outRms[0] = a * outRms[0].load() + (1 - a) * (float) (sqL / juce::jmax (1, n)); outRms[1] = a * outRms[1].load() + (1 - a) * (float) (sqR / juce::jmax (1, n)); sqL = sqR = 0;
    }
    // colonne dell'onda da 25 ms esatti, chiuse anche a meta' blocco: il tempo sullo schermo e' quello vero, qualunque sia il buffer della DAW
    colM = juce::jmax (colM, mx.gr); colL = juce::jmax (colL, lim.gr);
    const int colLen = juce::jmax (1, (int) std::lround (sr * kColSec));
    lastBlock = n;
    const double bps = hostBpm.load() / 60.0;
    for (int i = 0; i < n; ++i)
    {
        colRaw = juce::jmax (colRaw, mRawA[(size_t) i]); sqIn += mRawQ[(size_t) i]; colIn = juce::jmax (colIn, mInA[(size_t) i]);
        const float a = juce::jmax (std::abs (L[i]), std::abs (R[i])); colOut = juce::jmax (colOut, a); sqOut += 0.5 * (L[i] * L[i] + R[i] * R[i]);
        if (++colN >= colLen)
        {
            const int w = hW.load();
            hIn[w] = gToDb (colIn); hOut[w] = gToDb (colOut); hGrM[w] = colM; hGrL[w] = colL; hS[w] = loudOut.S;
            hRaw[w] = gToDb (colRaw); hRawR[w] = gToDb ((float) std::sqrt (sqIn / colN)); hOutR[w] = gToDb ((float) std::sqrt (sqOut / colN));
            hPpq[w] = hostPlay.load() ? (float) (blockPpq + (i + 1) / sr * bps) : -1.0f;
            hW = (w + 1) % kHist; hTot = hTot.load() + 1; colIn = colOut = colRaw = 0; sqIn = sqOut = 0; colN = 0; colM = mx.gr; colL = lim.gr;
        }
    }
}

void Processor::loadReference (const juce::File& f)
{
    if (refLoading.exchange (true)) return;
    const double rate = sr;
    juce::Thread::launch ([this, f, rate]
    {
        auto t = loadRefTrack (f, rate);
        if (t) { const juce::SpinLock::ScopedLockType sl (refLock); ref = t; refPos = 0; }
        refLoading = false;
    });
}

void Processor::autoInput()
{
    const float a = inAvg.load(); if (a < -60) return;
    auto* p = apvts.getParameter ("trim");
    p->setValueNotifyingHost (p->convertTo0to1 (juce::jlimit (-12.0f, 12.0f, value ("trim") + (kInOpt - a))));
    inAvg = kInOpt;
}

// porta il maximizer al target (sul thread dei messaggi)
void Processor::timerCallback()
{
    if (value ("mx_learn") < 0.5f || value ("mx_on") < 0.5f) return;
    const float s = lufsS.load(); if (s < -40) return;
    const float diff = target ((int) value ("target")).lufs - s;
    auto* p = apvts.getParameter ("mx_gain");
    const float cur = value ("mx_gain"), step = juce::jlimit (-0.4f, 0.4f, diff * 0.2f);
    if (std::abs (diff) > 0.15f) p->setValueNotifyingHost (p->convertTo0to1 (juce::jlimit (0.0f, 20.0f, cur + step)));
}

// ======================================================================= preset
const std::vector<Preset>& presets()
{
    static const std::vector<Preset> P = {
        { "Init", "Tutto spento: ogni modulo parte neutro. Accendi quello che ti serve.", {} },
        { "Facile | Trap", "Bassi pieni e alti lucidi, colla media, limiter duro e veloce. Target -7.",
          { { "e_monob", 1.0f }, { "e_low", 1.5f }, { "e_high", 1.5f }, { "e_thick", 0.25f }, { "e_warm", 0.15f }, { "e_silk", 0.35f }, { "e_glue", 0.35f }, { "e_calm", 0.4f }, { "e_wide", 0.25f }, { "e_loud", 8.2f }, { "e_pres", 0.7f }, { "e_speed", 0.65f }, { "target", 5.0f }, { "lm_ceil", -0.5f } } },
        { "Facile | Drill", "Piu pressione e controllo, rilascio veloce per cassa e rullante in faccia.",
          { { "e_monob", 1.0f }, { "e_low", 1.0f }, { "e_mid", 0.5f }, { "e_high", 1.0f }, { "e_thick", 0.2f }, { "e_warm", 0.1f }, { "e_silk", 0.3f }, { "e_glue", 0.45f }, { "e_calm", 0.45f }, { "e_wide", 0.15f }, { "e_loud", 8.0f }, { "e_pres", 0.8f }, { "e_speed", 0.75f }, { "target", 5.0f }, { "lm_ceil", -0.3f } } },
        { "Facile | Boom Bap", "Caldo e spesso: valvola e nastro, alti morbidi, limiter morbido e lento.",
          { { "e_low", 1.5f }, { "e_high", -0.5f }, { "e_thick", 0.45f }, { "e_warm", 0.45f }, { "e_silk", 0.1f }, { "e_glue", 0.4f }, { "e_calm", 0.3f }, { "e_wide", 0.1f }, { "e_loud", 4.5f }, { "e_pres", 0.25f }, { "e_speed", 0.35f }, { "target", 3.0f }, { "lm_ceil", -1.0f } } },
        { "Facile | R&B", "Lucido e dinamico: silk sugli alti, calm sulle durezze della voce, stereo aperto.",
          { { "e_monob", 1.0f }, { "e_low", 1.0f }, { "e_mid", -0.5f }, { "e_high", 1.5f }, { "e_thick", 0.2f }, { "e_warm", 0.2f }, { "e_silk", 0.45f }, { "e_glue", 0.3f }, { "e_calm", 0.5f }, { "e_wide", 0.3f }, { "e_loud", 5.0f }, { "e_pres", 0.3f }, { "e_speed", 0.4f }, { "target", 3.0f }, { "lm_ceil", -1.0f } } },
        { "Facile | Pop", "Equilibrato e brillante, volume da streaming forte.",
          { { "e_monob", 1.0f }, { "e_low", 0.5f }, { "e_mid", 0.5f }, { "e_high", 1.5f }, { "e_thick", 0.15f }, { "e_warm", 0.1f }, { "e_silk", 0.4f }, { "e_glue", 0.35f }, { "e_calm", 0.4f }, { "e_wide", 0.25f }, { "e_loud", 5.0f }, { "e_pres", 0.5f }, { "e_speed", 0.5f }, { "target", 3.0f }, { "lm_ceil", -1.0f } } },
        { "Facile | Lo-fi", "Scuro e polveroso: tanto nastro, alti tagliati, stereo un filo piu stretto.",
          { { "e_low", 1.0f }, { "e_high", -2.0f }, { "e_thick", 0.4f }, { "e_warm", 0.7f }, { "e_glue", 0.3f }, { "e_calm", 0.2f }, { "e_wide", -0.1f }, { "e_loud", 4.0f }, { "e_pres", 0.2f }, { "e_speed", 0.3f }, { "target", 3.0f }, { "lm_ceil", -1.0f } } },
        { "Facile | Club", "Sub e cassa davanti, limiter deciso. Target -8.",
          { { "e_monob", 1.0f }, { "e_low", 2.0f }, { "e_high", 1.0f }, { "e_thick", 0.3f }, { "e_warm", 0.1f }, { "e_silk", 0.3f }, { "e_glue", 0.4f }, { "e_calm", 0.35f }, { "e_wide", 0.2f }, { "e_loud", 7.0f }, { "e_pres", 0.6f }, { "e_speed", 0.6f }, { "target", 4.0f }, { "lm_ceil", -0.3f } } },
        { "Facile | Streaming pulito", "Trasparente, -14 LUFS: tocca poco e lascia respirare il mix.",
          { { "e_high", 0.5f }, { "e_silk", 0.2f }, { "e_glue", 0.25f }, { "e_calm", 0.3f }, { "e_loud", 1.4f }, { "e_pres", 0.4f }, { "e_speed", 0.5f }, { "target", 0.0f }, { "lm_ceil", -1.0f } } },
        { "Genere | Trap", "Target -7 LUFS. Bassi mono sotto 120 Hz con punch, picchi dell'808 tenuti fermi, presenza dinamica a 3.5 kHz, aria sopra, colla leggera e maximizer PUNCH.",
          { { "bc_on", 1 }, { "bc_freq", 110 }, { "bc_mono", 120 }, { "bc_punch", 0.3f }, { "bc_thr", -14 }, { "bc_dirty", 0.2f }, { "bc_harm", 0.15f }, { "eq_on", 1 }, { "eq1_type", 1 }, { "eq1_f", 40 }, { "eq1_g", 1 }, { "eq1_q", 0.7f }, { "eq5_type", 0 }, { "eq5_f", 3500 }, { "eq5_g", 0 }, { "eq5_q", 1.5f }, { "eq5_dyn", 1 }, { "eq5_thr", -30 }, { "eq5_rng", -3 }, { "eq6_type", 2 }, { "eq6_f", 11000 }, { "eq6_g", 1.5f }, { "eq6_q", 0.7f }, { "ds_on", 1 }, { "ds_lo", 6000 }, { "ds_hi", 20000 }, { "ds_thr", -8 }, { "ds_amt", 0.35f }, { "ds_soft", 0.6f }, { "ds_ch", 0 }, { "bus_on", 1 }, { "bus_thr", -16 }, { "bus_ratio", 1 }, { "bus_att", 4 }, { "bus_rel", 4 }, { "bus_hpf", 90 }, { "bus_range", 4 }, { "bus_mix", 1 }, { "bus_auto", 1 }, { "bus_makeup", 0 }, { "im_on", 1 }, { "im_smode", 0 }, { "im_w1", 0.4f }, { "im_s1", 0 }, { "im_w2", 1 }, { "im_s2", 0 }, { "im_w3", 1.15f }, { "im_s3", 0 }, { "im_w4", 1.15f }, { "im_s4", 0.05f }, { "mx_on", 1 }, { "mx_gain", 10 }, { "mx_style", 1 }, { "mx_clip", 0.5f }, { "mx_char", 0.6f }, { "lm_on", 1 }, { "lm_ceil", -0.5f }, { "lm_tp", 1 }, { "target", 5 } } },
        { "Genere | Trap muro (-6)", "Il massimo volume possibile: clipper deciso, maximizer AGGRESSIVO, 808 controllato sporco. Solo se il mix lo regge.",
          { { "bc_on", 1 }, { "bc_freq", 110 }, { "bc_mono", 130 }, { "bc_punch", 0.2f }, { "bc_thr", -10 }, { "bc_dirty", 0.5f }, { "bc_harm", 0.2f }, { "eq_on", 1 }, { "eq6_type", 2 }, { "eq6_f", 12000 }, { "eq6_g", 1 }, { "eq6_q", 0.7f }, { "bus_on", 1 }, { "bus_thr", -14 }, { "bus_ratio", 1 }, { "bus_att", 5 }, { "bus_rel", 4 }, { "bus_hpf", 100 }, { "bus_range", 3 }, { "bus_mix", 1 }, { "bus_auto", 1 }, { "bus_makeup", 0 }, { "im_on", 1 }, { "im_smode", 0 }, { "im_w1", 0.3f }, { "im_s1", 0 }, { "im_w2", 1 }, { "im_s2", 0 }, { "im_w3", 1.1f }, { "im_s3", 0 }, { "im_w4", 1.12f }, { "im_s4", 0 }, { "mx_on", 1 }, { "mx_gain", 13 }, { "mx_style", 2 }, { "mx_clip", 0.75f }, { "mx_char", 0.75f }, { "lm_on", 1 }, { "lm_ceil", -0.3f }, { "lm_tp", 1 }, { "target", 5 } } },
        { "Genere | Drill", "Cassa e rullante in faccia, 808 stretto (CODA corta), compressione piu decisa e maximizer aggressivo.",
          { { "bc_on", 1 }, { "bc_freq", 120 }, { "bc_mono", 120 }, { "bc_punch", 0.5f }, { "bc_tail", -0.35f }, { "bc_thr", -12 }, { "bc_dirty", 0.4f }, { "eq_on", 1 }, { "eq2_type", 0 }, { "eq2_f", 250 }, { "eq2_g", -1.5f }, { "eq2_q", 1 }, { "eq5_type", 0 }, { "eq5_f", 4000 }, { "eq5_g", 0 }, { "eq5_q", 1.4f }, { "eq5_dyn", 1 }, { "eq5_thr", -28 }, { "eq5_rng", -2.5f }, { "bus_on", 1 }, { "bus_thr", -14 }, { "bus_ratio", 2 }, { "bus_att", 4 }, { "bus_rel", 2 }, { "bus_hpf", 100 }, { "bus_range", 5 }, { "bus_mix", 1 }, { "bus_auto", 1 }, { "bus_makeup", 0 }, { "mx_on", 1 }, { "mx_gain", 11 }, { "mx_style", 2 }, { "mx_clip", 0.6f }, { "mx_char", 0.7f }, { "lm_on", 1 }, { "lm_ceil", -0.3f }, { "lm_tp", 1 }, { "target", 4 } } },
        { "Genere | Boom Bap", "Bassi WARM con piu sustain, alti morbidi, colla lenta in parallelo e maximizer MORBIDO.",
          { { "bc_on", 1 }, { "bc_freq", 100 }, { "bc_mono", 100 }, { "bc_punch", -0.3f }, { "bc_tail", 0.2f }, { "eq_on", 1 }, { "eq1_type", 1 }, { "eq1_f", 80 }, { "eq1_g", 1.5f }, { "eq1_q", 0.7f }, { "eq6_type", 2 }, { "eq6_f", 10000 }, { "eq6_g", -1.5f }, { "eq6_q", 0.7f }, { "bus_on", 1 }, { "bus_thr", -14 }, { "bus_ratio", 1 }, { "bus_att", 5 }, { "bus_rel", 4 }, { "bus_hpf", 0 }, { "bus_range", 5 }, { "bus_mix", 0.8f }, { "bus_auto", 1 }, { "bus_makeup", 0 }, { "mx_on", 1 }, { "mx_gain", 7 }, { "mx_style", 3 }, { "mx_clip", 0.3f }, { "mx_char", 0.35f }, { "lm_on", 1 }, { "lm_ceil", -1 }, { "lm_tp", 1 }, { "target", 3 } } },
        { "Genere | R&B", "Dinamico e lucido: de-esser sulla voce al centro, aria a 12 kHz, alti un filo piu larghi e diffusi.",
          { { "bc_on", 1 }, { "bc_freq", 120 }, { "bc_mono", 120 }, { "bc_punch", -0.2f }, { "eq_on", 1 }, { "eq3_type", 0 }, { "eq3_f", 500 }, { "eq3_g", -1 }, { "eq3_q", 0.8f }, { "eq6_type", 2 }, { "eq6_f", 12000 }, { "eq6_g", 2 }, { "eq6_q", 0.7f }, { "ds_on", 1 }, { "ds_lo", 5500 }, { "ds_hi", 20000 }, { "ds_thr", -10 }, { "ds_amt", 0.3f }, { "ds_soft", 0.8f }, { "ds_ch", 1 }, { "bus_on", 1 }, { "bus_thr", -18 }, { "bus_ratio", 0 }, { "bus_att", 5 }, { "bus_rel", 4 }, { "bus_hpf", 80 }, { "bus_range", 3 }, { "bus_mix", 1 }, { "bus_auto", 1 }, { "bus_makeup", 0 }, { "im_on", 1 }, { "im_smode", 1 }, { "im_w1", 0.5f }, { "im_s1", 0 }, { "im_w2", 1 }, { "im_s2", 0 }, { "im_w3", 1.05f }, { "im_s3", 0 }, { "im_w4", 1.1f }, { "im_s4", 0.08f }, { "mx_on", 1 }, { "mx_gain", 5 }, { "mx_style", 0 }, { "mx_clip", 0.1f }, { "mx_char", 0.3f }, { "lm_on", 1 }, { "lm_ceil", -1 }, { "lm_tp", 1 }, { "target", 0 } } },
        { "Genere | Pop", "Brillante e presente: presenza dinamica, multibanda leggero sugli alti, maximizer PUNCH.",
          { { "bc_on", 1 }, { "bc_freq", 110 }, { "bc_mono", 110 }, { "bc_punch", 0.15f }, { "eq_on", 1 }, { "eq4_type", 0 }, { "eq4_f", 3000 }, { "eq4_g", 0 }, { "eq4_q", 1.2f }, { "eq4_dyn", 1 }, { "eq4_thr", -28 }, { "eq4_rng", -2 }, { "eq6_type", 2 }, { "eq6_f", 10000 }, { "eq6_g", 2 }, { "eq6_q", 0.7f }, { "bus_on", 1 }, { "bus_thr", -16 }, { "bus_ratio", 1 }, { "bus_att", 4 }, { "bus_rel", 4 }, { "bus_hpf", 80 }, { "bus_range", 4 }, { "bus_mix", 1 }, { "bus_auto", 1 }, { "bus_makeup", 0 }, { "mb_on", 1 }, { "mb_mix", 0.25f }, { "mb_speed", 0.5f }, { "mb1_thr", 0 }, { "mb1_ratio", 2 }, { "mb1_up", 0 }, { "mb1_gain", 0 }, { "mb2_thr", -20 }, { "mb2_ratio", 2 }, { "mb2_up", 0 }, { "mb2_gain", 0 }, { "mb3_thr", -24 }, { "mb3_ratio", 3 }, { "mb3_up", 0.3f }, { "mb3_gain", 0 }, { "im_on", 1 }, { "im_smode", 0 }, { "im_w1", 0.6f }, { "im_s1", 0 }, { "im_w2", 1 }, { "im_s2", 0 }, { "im_w3", 1.1f }, { "im_s3", 0 }, { "im_w4", 1.12f }, { "im_s4", 0 }, { "mx_on", 1 }, { "mx_gain", 7 }, { "mx_style", 1 }, { "mx_clip", 0.3f }, { "mx_char", 0.5f }, { "lm_on", 1 }, { "lm_ceil", -1 }, { "lm_tp", 1 }, { "target", 0 } } },
        { "Genere | Lo-fi", "Taglio alto a 12 kHz, medi caldi, poco volume e tanta morbidezza.",
          { { "eq_on", 1 }, { "eq1_type", 3 }, { "eq1_f", 30 }, { "eq1_g", 0 }, { "eq1_q", 0.7f }, { "eq3_type", 0 }, { "eq3_f", 400 }, { "eq3_g", 1 }, { "eq3_q", 0.8f }, { "eq6_type", 4 }, { "eq6_f", 12000 }, { "eq6_g", 0 }, { "eq6_q", 0.9f }, { "bus_on", 1 }, { "bus_thr", -14 }, { "bus_ratio", 1 }, { "bus_att", 5 }, { "bus_rel", 3 }, { "bus_hpf", 0 }, { "bus_range", 5 }, { "bus_mix", 0.7f }, { "bus_auto", 1 }, { "bus_makeup", 0 }, { "im_on", 1 }, { "im_smode", 0 }, { "im_w1", 0.6f }, { "im_s1", 0 }, { "im_w2", 0.9f }, { "im_s2", 0 }, { "im_w3", 0.9f }, { "im_s3", 0 }, { "im_w4", 0.8f }, { "im_s4", 0 }, { "mx_on", 1 }, { "mx_gain", 4 }, { "mx_style", 3 }, { "mx_clip", 0.4f }, { "mx_char", 0.25f }, { "lm_on", 1 }, { "lm_ceil", -1 }, { "lm_tp", 1 }, { "target", 0 } } },
        { "Genere | Afrobeats", "Kick con punch, medi bassi puliti, alti brillanti, colla veloce sul groove.",
          { { "bc_on", 1 }, { "bc_freq", 110 }, { "bc_mono", 110 }, { "bc_punch", 0.45f }, { "bc_tail", -0.1f }, { "eq_on", 1 }, { "eq2_type", 0 }, { "eq2_f", 200 }, { "eq2_g", -1 }, { "eq2_q", 1 }, { "eq6_type", 2 }, { "eq6_f", 9000 }, { "eq6_g", 1.5f }, { "eq6_q", 0.7f }, { "bus_on", 1 }, { "bus_thr", -15 }, { "bus_ratio", 1 }, { "bus_att", 4 }, { "bus_rel", 1 }, { "bus_hpf", 90 }, { "bus_range", 4 }, { "bus_mix", 1 }, { "bus_auto", 1 }, { "bus_makeup", 0 }, { "mx_on", 1 }, { "mx_gain", 9 }, { "mx_style", 1 }, { "mx_clip", 0.45f }, { "mx_char", 0.6f }, { "lm_on", 1 }, { "lm_ceil", -0.3f }, { "lm_tp", 1 }, { "target", 4 } } },
        { "Genere | Rock", "Fango a 250 Hz tenuto sotto controllo solo quando serve, colla in parallelo 4:1, maximizer PUNCH.",
          { { "eq_on", 1 }, { "eq2_type", 0 }, { "eq2_f", 250 }, { "eq2_g", 0 }, { "eq2_q", 1 }, { "eq2_dyn", 1 }, { "eq2_thr", -26 }, { "eq2_rng", -2 }, { "eq6_type", 2 }, { "eq6_f", 8000 }, { "eq6_g", 1 }, { "eq6_q", 0.7f }, { "bus_on", 1 }, { "bus_thr", -16 }, { "bus_ratio", 2 }, { "bus_att", 5 }, { "bus_rel", 2 }, { "bus_hpf", 60 }, { "bus_range", 6 }, { "bus_mix", 0.6f }, { "bus_auto", 1 }, { "bus_makeup", 0 }, { "im_on", 1 }, { "im_smode", 0 }, { "im_w1", 0.7f }, { "im_s1", 0 }, { "im_w2", 1 }, { "im_s2", 0 }, { "im_w3", 1.1f }, { "im_s3", 0 }, { "im_w4", 1.1f }, { "im_s4", 0 }, { "mx_on", 1 }, { "mx_gain", 7 }, { "mx_style", 1 }, { "mx_clip", 0.25f }, { "mx_char", 0.5f }, { "lm_on", 1 }, { "lm_ceil", -1 }, { "lm_tp", 1 }, { "target", 2 } } },
        { "Genere | EDM", "Bassi mono fino a 150 Hz, multibanda tipo OTT leggero, alti larghi, volume da club.",
          { { "bc_on", 1 }, { "bc_freq", 120 }, { "bc_mono", 150 }, { "bc_punch", 0.4f }, { "bc_thr", -10 }, { "bc_dirty", 0.3f }, { "mb_on", 1 }, { "mb_mix", 0.2f }, { "mb_speed", 0.65f }, { "mb1_thr", -24 }, { "mb1_ratio", 6 }, { "mb1_up", 0.5f }, { "mb1_gain", 0 }, { "mb2_thr", -24 }, { "mb2_ratio", 6 }, { "mb2_up", 0.5f }, { "mb2_gain", 0 }, { "mb3_thr", -26 }, { "mb3_ratio", 6 }, { "mb3_up", 0.5f }, { "mb3_gain", 0 }, { "im_on", 1 }, { "im_smode", 0 }, { "im_w1", 0.2f }, { "im_s1", 0 }, { "im_w2", 1 }, { "im_s2", 0 }, { "im_w3", 1.2f }, { "im_s3", 0.1f }, { "im_w4", 1.4f }, { "im_s4", 0.3f }, { "mx_on", 1 }, { "mx_gain", 11 }, { "mx_style", 2 }, { "mx_clip", 0.6f }, { "mx_char", 0.7f }, { "lm_on", 1 }, { "lm_ceil", -0.3f }, { "lm_tp", 1 }, { "target", 4 } } },
        { "Genere | Jazz / acustico", "Quasi trasparente: colla 1.5:1 lenta e solo il volume necessario per Apple Music.",
          { { "eq_on", 1 }, { "eq6_type", 2 }, { "eq6_f", 12000 }, { "eq6_g", 1 }, { "eq6_q", 0.7f }, { "bus_on", 1 }, { "bus_thr", -20 }, { "bus_ratio", 0 }, { "bus_att", 5 }, { "bus_rel", 4 }, { "bus_hpf", 0 }, { "bus_range", 3 }, { "bus_mix", 1 }, { "bus_auto", 1 }, { "bus_makeup", 0 }, { "mx_on", 1 }, { "mx_gain", 3 }, { "mx_style", 0 }, { "mx_clip", 0 }, { "mx_char", 0.25f }, { "lm_on", 1 }, { "lm_ceil", -1 }, { "lm_tp", 1 }, { "target", 1 } } },
        { "Specifico | Streaming sicuro -14", "Punto di partenza pulito per Spotify e YouTube a -14: bassi mono, sibilanti domate, colla leggera.",
          { { "bc_on", 1 }, { "bc_freq", 110 }, { "bc_mono", 100 }, { "ds_on", 1 }, { "ds_lo", 6000 }, { "ds_hi", 20000 }, { "ds_thr", -9 }, { "ds_amt", 0.3f }, { "ds_soft", 0.6f }, { "ds_ch", 0 }, { "bus_on", 1 }, { "bus_thr", -18 }, { "bus_ratio", 1 }, { "bus_att", 4 }, { "bus_rel", 4 }, { "bus_hpf", 80 }, { "bus_range", 3 }, { "bus_mix", 1 }, { "bus_auto", 1 }, { "bus_makeup", 0 }, { "mx_on", 1 }, { "mx_gain", 5 }, { "mx_style", 0 }, { "mx_clip", 0.15f }, { "mx_char", 0.4f }, { "lm_on", 1 }, { "lm_ceil", -1 }, { "lm_tp", 1 }, { "target", 0 } } },
        { "Genere | Reggaeton", "Dembow in faccia: cassa e rullante con punch, sub mono e fermo, alti brillanti, voce chiara. Volume da club (-8).",
          { { "bc_on", 1 }, { "bc_freq", 110 }, { "bc_mono", 120 }, { "bc_punch", 0.5f }, { "bc_thr", -12 }, { "bc_dirty", 0.25f }, { "eq_on", 1 }, { "eq2_type", 0 }, { "eq2_f", 300 }, { "eq2_g", -1 }, { "eq2_q", 1 }, { "eq5_type", 0 }, { "eq5_f", 3000 }, { "eq5_g", 1 }, { "eq5_q", 1 }, { "eq6_type", 2 }, { "eq6_f", 10000 }, { "eq6_g", 2 }, { "eq6_q", 0.7f },
            { "ds_on", 1 }, { "ds_lo", 6000 }, { "ds_thr", -8 }, { "ds_amt", 0.35f }, { "bus_on", 1 }, { "bus_thr", -15 }, { "bus_ratio", 1 }, { "bus_att", 4 }, { "bus_rel", 1 }, { "bus_hpf", 90 }, { "bus_range", 4 },
            { "mx_on", 1 }, { "mx_gain", 10 }, { "mx_style", 1 }, { "mx_clip", 0.5f }, { "mx_char", 0.6f }, { "lm_on", 1 }, { "lm_ceil", -0.3f }, { "lm_tp", 1 }, { "target", 4 } } },
        { "Genere | Indie", "Dinamico e naturale: nastro leggero per il colore, aria morbida, poca compressione. Meglio un po' piu piano che schiacciato (-14).",
          { { "pq_on", 1 }, { "pq_lf", 1 }, { "pq_lb", 2 }, { "pq_la", 1 }, { "pq_hf", 5 }, { "pq_hb", 2 }, { "pq_bw", 6 }, { "pq_as", 2 }, { "pq_ha", 0 }, { "pq_tube", 0.3f },
            { "tp_on", 1 }, { "tp_model", 1 }, { "tp_drive", 0.25f }, { "tp_mix", 0.7f }, { "bus_on", 1 }, { "bus_thr", -18 }, { "bus_ratio", 0 }, { "bus_att", 5 }, { "bus_rel", 4 }, { "bus_range", 3 },
            { "mx_on", 1 }, { "mx_gain", 5 }, { "mx_style", 0 }, { "mx_clip", 0.15f }, { "lm_on", 1 }, { "lm_ceil", -1 }, { "lm_tp", 1 }, { "target", 0 } } },
        { "Genere | Rap aggressivo", "Voce e batteria che spingono: presenza dinamica 2-4 kHz, compressione 4:1 veloce, clipper deciso. Forte (-7).",
          { { "bc_on", 1 }, { "bc_freq", 120 }, { "bc_mono", 120 }, { "bc_punch", 0.5f }, { "bc_thr", -12 }, { "bc_dirty", 0.5f }, { "bc_harm", 0.25f }, { "eq_on", 1 }, { "eq2_type", 0 }, { "eq2_f", 250 }, { "eq2_g", -1.5f }, { "eq2_q", 1 },
            { "eq4_type", 0 }, { "eq4_f", 3000 }, { "eq4_g", 1.5f }, { "eq4_q", 1 }, { "eq4_dyn", 1 }, { "eq4_thr", -24 }, { "eq4_rng", -3 }, { "ds_on", 1 }, { "ds_lo", 5500 }, { "ds_thr", -8 }, { "ds_amt", 0.45f }, { "ds_ch", 1 },
            { "bus_on", 1 }, { "bus_thr", -16 }, { "bus_ratio", 2 }, { "bus_att", 3 }, { "bus_rel", 0 }, { "bus_hpf", 100 }, { "bus_range", 6 }, { "bus_mix", 0.8f },
            { "mx_on", 1 }, { "mx_gain", 11 }, { "mx_style", 2 }, { "mx_clip", 0.65f }, { "mx_char", 0.7f }, { "lm_on", 1 }, { "lm_ceil", -0.3f }, { "lm_tp", 1 }, { "target", 5 } } },
        { "Genere | Soul", "Caldo e vellutato: valvole sui bassi, vari-mu lento che incolla, nastro 15 IPS, maximizer morbido. Dinamico per Apple Music (-16).",
          { { "pq_on", 1 }, { "pq_lf", 2 }, { "pq_lb", 3 }, { "pq_la", 2 }, { "pq_hf", 4 }, { "pq_hb", 1.5f }, { "pq_bw", 7 }, { "pq_as", 1 }, { "pq_ha", 1 }, { "pq_tube", 0.4f },
            { "vm_on", 1 }, { "vm_thr", -22 }, { "vm_att", 3 }, { "vm_rel", 6 }, { "vm_mix", 0.8f }, { "vm_tube", 0.4f }, { "tp_on", 1 }, { "tp_model", 1 }, { "tp_drive", 0.3f }, { "tp_mix", 0.8f },
            { "mx_on", 1 }, { "mx_gain", 4 }, { "mx_style", 3 }, { "mx_clip", 0.1f }, { "lm_on", 1 }, { "lm_ceil", -1 }, { "lm_tp", 1 }, { "target", 1 } } },
        { "Genere | House / Techno", "Cassa dritta e sub mono fino a 150 Hz, multibanda leggero per la densita, alti larghi. Volume da club (-8).",
          { { "bc_on", 1 }, { "bc_freq", 130 }, { "bc_mono", 150 }, { "bc_punch", 0.35f }, { "bc_thr", -10 }, { "bc_dirty", 0.2f }, { "mb_on", 1 }, { "mb_mix", 0.25f }, { "mb_speed", 0.6f }, { "mb1_thr", -22 }, { "mb1_ratio", 3 }, { "mb2_thr", -24 }, { "mb2_ratio", 3 }, { "mb3_thr", -26 }, { "mb3_ratio", 3 }, { "mb3_up", 0.3f },
            { "im_on", 1 }, { "im_w1", 0.0f }, { "im_w2", 1 }, { "im_w3", 1.2f }, { "im_w4", 1.3f }, { "bus_on", 1 }, { "bus_thr", -14 }, { "bus_ratio", 1 }, { "bus_att", 4 }, { "bus_rel", 4 }, { "bus_hpf", 120 }, { "bus_range", 4 },
            { "mx_on", 1 }, { "mx_gain", 9.5f }, { "mx_style", 1 }, { "mx_clip", 0.45f }, { "lm_on", 1 }, { "lm_ceil", -0.3f }, { "lm_tp", 1 }, { "target", 4 } } },
        { "Specifico | 808 pesante senza distorsione", "Quando l'808 riempie tutto: picchi del sub tenuti fermi, coda stretta, mono sotto 130 Hz e il volume lo fa il maximizer, non il limiter.",
          { { "bc_on", 1 }, { "bc_freq", 130 }, { "bc_mono", 130 }, { "bc_tail", -0.25f }, { "bc_thr", -14 }, { "bc_dirty", 0.0f }, { "bc_harm", 0.3f }, { "mx_on", 1 }, { "mx_gain", 8 }, { "mx_style", 0 }, { "mx_clip", 0.2f }, { "lm_on", 1 }, { "lm_ceil", -1 }, { "lm_tp", 1 }, { "target", 0 } } },
        { "Specifico | Voce davanti", "La voce si perde nel beat: presenza dinamica solo al centro (MID), sibilanti domate, colla leggera.",
          { { "eq_on", 1 }, { "eq4_type", 0 }, { "eq4_f", 3000 }, { "eq4_g", 1.5f }, { "eq4_q", 1 }, { "eq4_ch", 1 }, { "eq4_dyn", 1 }, { "eq4_thr", -26 }, { "eq4_rng", -2 }, { "eq2_type", 0 }, { "eq2_f", 300 }, { "eq2_g", -1 }, { "eq2_q", 1 }, { "eq2_ch", 2 },
            { "ds_on", 1 }, { "ds_lo", 5500 }, { "ds_thr", -9 }, { "ds_amt", 0.4f }, { "ds_ch", 1 }, { "bus_on", 1 }, { "bus_thr", -18 }, { "bus_ratio", 1 }, { "bus_range", 3 }, { "mx_on", 1 }, { "mx_gain", 7 }, { "mx_style", 0 }, { "lm_on", 1 }, { "lm_ceil", -1 }, { "lm_tp", 1 } } },
        { "Specifico | Forte senza pompare", "Molto volume ma la cassa resta viva: il carico e' diviso tra clip, maximizer PULITO e limiter finale, ognuno lavora poco (-9).",
          { { "bc_on", 1 }, { "bc_freq", 110 }, { "bc_mono", 110 }, { "bc_thr", -12 }, { "bus_on", 1 }, { "bus_thr", -16 }, { "bus_ratio", 0 }, { "bus_att", 5 }, { "bus_rel", 4 }, { "bus_range", 3 },
            { "mx_on", 1 }, { "mx_gain", 10 }, { "mx_style", 0 }, { "mx_clip", 0.55f }, { "mx_char", 0.4f }, { "lm_on", 1 }, { "lm_ceil", -0.5f }, { "lm_tp", 1 }, { "target", 4 } } },
        { "Specifico | Dinamico per Apple Music", "Per chi vuole respiro: -16 LUFS, niente clip, colla 1.5:1 e limiter che tocca appena.",
          { { "bus_on", 1 }, { "bus_thr", -20 }, { "bus_ratio", 0 }, { "bus_att", 5 }, { "bus_rel", 4 }, { "bus_range", 3 }, { "mx_on", 1 }, { "mx_gain", 3 }, { "mx_style", 0 }, { "mx_clip", 0 }, { "lm_on", 1 }, { "lm_ceil", -1 }, { "lm_tp", 1 }, { "target", 1 } } },
        { "Specifico | Caldo analogico", "Suono da console e nastro: Pultec sui bassi, vari-mu, nastro 15 IPS ed exciter a valvola sui bassi-medi.",
          { { "pq_on", 1 }, { "pq_lf", 2 }, { "pq_lb", 3 }, { "pq_la", 2 }, { "pq_tube", 0.5f }, { "vm_on", 1 }, { "vm_thr", -20 }, { "vm_mix", 0.7f }, { "vm_tube", 0.5f }, { "tp_on", 1 }, { "tp_model", 1 }, { "tp_drive", 0.35f },
            { "ex_on", 1 }, { "ex1_amt", 0.25f }, { "ex1_mode", 0 }, { "ex_mix", 0.6f }, { "mx_on", 1 }, { "mx_gain", 6 }, { "mx_style", 3 }, { "lm_on", 1 }, { "lm_ceil", -1 }, { "lm_tp", 1 } } },
        { "Specifico | Stereo largo ma sicuro in mono", "Bassi stretti, alti aperti e diffusi; la SICUREZZA MONO controlla che in mono non sparisca niente.",
          { { "bc_on", 1 }, { "bc_freq", 120 }, { "bc_mono", 120 }, { "im_on", 1 }, { "im_safe", 1 }, { "im_smode", 1 }, { "im_w1", 0.0f }, { "im_w2", 1 }, { "im_w3", 1.25f }, { "im_s3", 0.2f }, { "im_w4", 1.4f }, { "im_s4", 0.25f },
            { "mx_on", 1 }, { "mx_gain", 7 }, { "mx_style", 0 }, { "lm_on", 1 }, { "lm_ceil", -1 }, { "lm_tp", 1 } } },
        { "Genere | Trap melodica", "Voce cantata e 808 lungo: bassi pieni ma morbidi, alti setosi, colla leggera. Forte ma non schiacciato (-9).",
          { { "bc_on", 1 }, { "bc_freq", 110 }, { "bc_mono", 120 }, { "bc_punch", 0.1f }, { "bc_thr", -14 }, { "bc_harm", 0.2f }, { "eq_on", 1 }, { "eq6_type", 2 }, { "eq6_f", 12000 }, { "eq6_g", 1.5f }, { "eq6_q", 0.7f }, { "ds_on", 1 }, { "ds_lo", 6000 }, { "ds_thr", -9 }, { "ds_amt", 0.35f },
            { "bus_on", 1 }, { "bus_thr", -16 }, { "bus_ratio", 1 }, { "bus_att", 4 }, { "bus_rel", 4 }, { "bus_hpf", 90 }, { "bus_range", 4 }, { "mx_on", 1 }, { "mx_gain", 9 }, { "mx_style", 1 }, { "mx_clip", 0.35f }, { "lm_on", 1 }, { "lm_ceil", -0.5f }, { "lm_tp", 1 }, { "target", 4 } } },
        { "Genere | Drill UK", "Slide dell'808 e rullante secco: sub mono stretto, coda corta, clipper deciso (-7).",
          { { "bc_on", 1 }, { "bc_freq", 120 }, { "bc_mono", 130 }, { "bc_punch", 0.4f }, { "bc_tail", -0.3f }, { "bc_thr", -12 }, { "bc_dirty", 0.45f }, { "eq_on", 1 }, { "eq2_type", 0 }, { "eq2_f", 300 }, { "eq2_g", -1 }, { "eq2_q", 1 },
            { "bus_on", 1 }, { "bus_thr", -15 }, { "bus_ratio", 2 }, { "bus_att", 4 }, { "bus_rel", 0 }, { "bus_hpf", 100 }, { "bus_range", 5 }, { "mx_on", 1 }, { "mx_gain", 11 }, { "mx_style", 2 }, { "mx_clip", 0.6f }, { "lm_on", 1 }, { "lm_ceil", -0.3f }, { "lm_tp", 1 }, { "target", 5 } } },
        { "Genere | Boom Bap sporco", "Campioni impolverati e batteria che schiaffeggia: nastro a cassetta leggero, vari-mu, alti tagliati (-10).",
          { { "tp_on", 1 }, { "tp_model", 2 }, { "tp_drive", 0.3f }, { "tp_mix", 0.6f }, { "vm_on", 1 }, { "vm_thr", -22 }, { "vm_att", 2 }, { "vm_rel", 2 }, { "vm_mix", 0.8f }, { "eq_on", 1 }, { "eq6_type", 4 }, { "eq6_f", 13000 }, { "eq6_q", 0.7f },
            { "bc_on", 1 }, { "bc_freq", 100 }, { "bc_punch", 0.3f }, { "mx_on", 1 }, { "mx_gain", 7 }, { "mx_style", 3 }, { "mx_clip", 0.3f }, { "lm_on", 1 }, { "lm_ceil", -0.5f }, { "lm_tp", 1 }, { "target", 3 } } },
        { "Genere | Pop urbano", "Voce in primo piano e batteria moderna: presenza dinamica, aria, OTT leggero, volume da playlist (-10).",
          { { "bc_on", 1 }, { "bc_freq", 110 }, { "bc_mono", 110 }, { "bc_punch", 0.25f }, { "eq_on", 1 }, { "eq4_type", 0 }, { "eq4_f", 3000 }, { "eq4_g", 1 }, { "eq4_q", 1 }, { "eq4_ch", 1 }, { "eq6_type", 2 }, { "eq6_f", 11000 }, { "eq6_g", 1.5f }, { "eq6_q", 0.7f },
            { "mb_on", 1 }, { "mb_mix", 0.15f }, { "mb_speed", 0.7f }, { "mb1_thr", -22 }, { "mb1_ratio", 12 }, { "mb1_up", 0.85f }, { "mb2_thr", -24 }, { "mb2_ratio", 12 }, { "mb2_up", 0.85f }, { "mb3_thr", -26 }, { "mb3_ratio", 12 }, { "mb3_up", 0.85f },
            { "mx_on", 1 }, { "mx_gain", 8.5f }, { "mx_style", 1 }, { "mx_clip", 0.3f }, { "lm_on", 1 }, { "lm_ceil", -1 }, { "lm_tp", 1 }, { "target", 3 } } },
        { "Genere | Latin pop", "Brillante e festoso: kick in faccia, alti aperti e larghi, voce chiara (-9).",
          { { "bc_on", 1 }, { "bc_freq", 110 }, { "bc_mono", 120 }, { "bc_punch", 0.4f }, { "eq_on", 1 }, { "eq6_type", 2 }, { "eq6_f", 9000 }, { "eq6_g", 2 }, { "eq6_q", 0.7f }, { "im_on", 1 }, { "im_w1", 0.0f }, { "im_w3", 1.15f }, { "im_w4", 1.3f },
            { "bus_on", 1 }, { "bus_thr", -15 }, { "bus_ratio", 1 }, { "bus_att", 4 }, { "bus_rel", 1 }, { "bus_range", 4 }, { "mx_on", 1 }, { "mx_gain", 9 }, { "mx_style", 1 }, { "mx_clip", 0.4f }, { "lm_on", 1 }, { "lm_ceil", -0.5f }, { "lm_tp", 1 }, { "target", 4 } } },
        { "Genere | Lo-fi chill", "Morbido e intimo: nastro 15 IPS, alti chiusi a 11 kHz, poco volume (-14).",
          { { "tp_on", 1 }, { "tp_model", 1 }, { "tp_drive", 0.35f }, { "tp_noise", 0.15f }, { "eq_on", 1 }, { "eq6_type", 4 }, { "eq6_f", 11000 }, { "eq6_q", 0.7f }, { "bc_on", 1 }, { "bc_punch", -0.3f },
            { "mx_on", 1 }, { "mx_gain", 4 }, { "mx_style", 3 }, { "lm_on", 1 }, { "lm_ceil", -1 }, { "lm_tp", 1 }, { "target", 0 } } },
        { "Specifico | Demo veloce", "Per far sentire un provino subito: catena corta e sicura, volume da SoundCloud (-10).",
          { { "bc_on", 1 }, { "bc_mono", 110 }, { "bc_thr", -12 }, { "bus_on", 1 }, { "bus_thr", -16 }, { "bus_ratio", 1 }, { "bus_range", 4 }, { "mx_on", 1 }, { "mx_gain", 8 }, { "mx_style", 1 }, { "mx_clip", 0.35f }, { "lm_on", 1 }, { "lm_ceil", -1 }, { "lm_tp", 1 }, { "target", 3 } } },
        { "Specifico | Beat senza voce", "Per strumentali e beat da vendere: lascia spazio sui medi per la voce che arrivera, sub fermo, forte ma pulito.",
          { { "bc_on", 1 }, { "bc_freq", 120 }, { "bc_mono", 120 }, { "bc_thr", -12 }, { "eq_on", 1 }, { "eq4_type", 0 }, { "eq4_f", 2500 }, { "eq4_g", -1 }, { "eq4_q", 0.8f }, { "eq4_ch", 1 },
            { "mx_on", 1 }, { "mx_gain", 9 }, { "mx_style", 0 }, { "mx_clip", 0.4f }, { "lm_on", 1 }, { "lm_ceil", -0.5f }, { "lm_tp", 1 }, { "target", 4 } } },
        { "Specifico | Campione vecchio", "Per riportare in vita un campione da vinile: rimbombo e fruscio sotto controllo, un po' di aria e corpo.",
          { { "eq_on", 1 }, { "eq1_type", 3 }, { "eq1_f", 35 }, { "eq2_type", 0 }, { "eq2_f", 200 }, { "eq2_dyn", 1 }, { "eq2_thr", -26 }, { "eq2_rng", -3 }, { "eq6_type", 2 }, { "eq6_f", 10000 }, { "eq6_g", 1.5f }, { "eq6_q", 0.7f },
            { "ds_on", 1 }, { "ds_lo", 7000 }, { "ds_thr", -10 }, { "ds_amt", 0.35f }, { "pq_on", 1 }, { "pq_lf", 2 }, { "pq_lb", 2 }, { "pq_la", 1 }, { "mx_on", 1 }, { "mx_gain", 5 }, { "mx_style", 3 }, { "lm_on", 1 }, { "lm_ceil", -1 }, { "lm_tp", 1 } } },
        { "Mix Bus | Colla console", "Il classico: 2:1, attacco 10 ms, rilascio AUTO, HPF del sidechain a 80 Hz. 1-3 dB sulla lancetta.",
          { { "bus_on", 1 }, { "bus_thr", -16 }, { "bus_ratio", 1 }, { "bus_att", 4 }, { "bus_rel", 4 }, { "bus_hpf", 80 }, { "bus_range", 4 }, { "bus_mix", 1 }, { "bus_auto", 1 }, { "bus_makeup", 0 } } },
        { "Mix Bus | Glue trap", "4:1 con attacco lento e rilascio veloce: il groove respira con la cassa, HPF a 100 Hz per non pompare sull'808.",
          { { "bus_on", 1 }, { "bus_thr", -14 }, { "bus_ratio", 2 }, { "bus_att", 5 }, { "bus_rel", 0 }, { "bus_hpf", 100 }, { "bus_range", 5 }, { "bus_mix", 1 }, { "bus_auto", 1 }, { "bus_makeup", 0 }, { "bc_on", 1 }, { "bc_freq", 110 }, { "bc_punch", 0.2f } } },
        { "Mix Bus | Parallelo New York", "Compressione forte 10:1 mescolata al 30%: corpo e densita senza perdere i transienti.",
          { { "bus_on", 1 }, { "bus_thr", -24 }, { "bus_ratio", 3 }, { "bus_att", 2 }, { "bus_rel", 0 }, { "bus_hpf", 0 }, { "bus_range", 15 }, { "bus_mix", 0.3f }, { "bus_auto", 0 }, { "bus_makeup", 6 } } },
        { "Mix Bus | Calore analogico", "Bassi un filo pieni, alti arrotondati, colla 1.5:1 lenta e una saturazione morbida dal maximizer.",
          { { "eq_on", 1 }, { "eq1_type", 1 }, { "eq1_f", 100 }, { "eq1_g", 1 }, { "eq1_q", 0.7f }, { "eq6_type", 2 }, { "eq6_f", 12000 }, { "eq6_g", -1 }, { "eq6_q", 0.7f }, { "bus_on", 1 }, { "bus_thr", -16 }, { "bus_ratio", 0 }, { "bus_att", 5 }, { "bus_rel", 4 }, { "bus_hpf", 0 }, { "bus_range", 4 }, { "bus_mix", 1 }, { "bus_auto", 1 }, { "bus_makeup", 0 }, { "mx_on", 1 }, { "mx_gain", 2 }, { "mx_style", 3 }, { "mx_clip", 0.2f }, { "mx_char", 0.3f } } },
        { "Mix Bus | Pulizia fango", "EQ dinamico che toglie i 250 Hz solo quando si accumulano, taglio sotto 25 Hz, sibilanti leggere.",
          { { "eq_on", 1 }, { "eq1_type", 3 }, { "eq1_f", 25 }, { "eq1_g", 0 }, { "eq1_q", 0.7f }, { "eq2_type", 0 }, { "eq2_f", 250 }, { "eq2_g", 0 }, { "eq2_q", 1.2f }, { "eq2_dyn", 1 }, { "eq2_thr", -28 }, { "eq2_rng", -3 }, { "ds_on", 1 }, { "ds_lo", 6500 }, { "ds_hi", 20000 }, { "ds_thr", -8 }, { "ds_amt", 0.25f }, { "ds_soft", 0.6f }, { "ds_ch", 0 } } },
        { "Mix Bus | Larghezza controllata", "Bassi stretti, medio-alti piu larghi, un filo di stereoize sugli alti. Controlla in mono.",
          { { "bc_on", 1 }, { "bc_freq", 120 }, { "bc_mono", 120 }, { "im_on", 1 }, { "im_smode", 0 }, { "im_w1", 0.2f }, { "im_s1", 0 }, { "im_w2", 0.9f }, { "im_s2", 0 }, { "im_w3", 1.15f }, { "im_s3", 0.15f }, { "im_w4", 1.3f }, { "im_s4", 0.25f } } },
        { "Mix Bus | OTT leggero", "Multibanda giu+su in stile OTT al 20%: tutto piu denso e dettagliato.",
          { { "mb_on", 1 }, { "mb_mix", 0.2f }, { "mb_speed", 0.7f }, { "mb1_thr", -22 }, { "mb1_ratio", 12 }, { "mb1_up", 0.85f }, { "mb1_gain", 3 }, { "mb2_thr", -24 }, { "mb2_ratio", 12 }, { "mb2_up", 0.85f }, { "mb2_gain", 2 }, { "mb3_thr", -26 }, { "mb3_ratio", 12 }, { "mb3_up", 0.85f }, { "mb3_gain", 4 }, { "mb_x1", 88 }, { "mb_x2", 2500 } } },
        { "Mix Bus | Voce davanti", "Presenza sulla sola voce al centro, de-esser sul MID e colla leggera.",
          { { "eq_on", 1 }, { "eq4_type", 0 }, { "eq4_f", 3000 }, { "eq4_g", 1.5f }, { "eq4_q", 1 }, { "eq4_ch", 1 }, { "ds_on", 1 }, { "ds_lo", 5000 }, { "ds_hi", 20000 }, { "ds_thr", -9 }, { "ds_amt", 0.4f }, { "ds_soft", 0.6f }, { "ds_ch", 1 }, { "bus_on", 1 }, { "bus_thr", -18 }, { "bus_ratio", 1 }, { "bus_att", 4 }, { "bus_rel", 4 }, { "bus_hpf", 80 }, { "bus_range", 3 }, { "bus_mix", 1 }, { "bus_auto", 1 }, { "bus_makeup", 0 } } },
        { "Batteria | Punch e colpo", "Bassi con PUNCH, compressione 4:1 lenta in parallelo, alti che si aprono e un tocco di clip.",
          { { "bc_on", 1 }, { "bc_freq", 120 }, { "bc_punch", 0.6f }, { "bus_on", 1 }, { "bus_thr", -18 }, { "bus_ratio", 2 }, { "bus_att", 5 }, { "bus_rel", 0 }, { "bus_hpf", 60 }, { "bus_range", 8 }, { "bus_mix", 0.7f }, { "bus_auto", 1 }, { "bus_makeup", 0 }, { "mb_on", 1 }, { "mb_mix", 1 }, { "mb_speed", 0.5f }, { "mb1_thr", 0 }, { "mb1_ratio", 1 }, { "mb1_up", 0 }, { "mb1_gain", 0 }, { "mb2_thr", 0 }, { "mb2_ratio", 1 }, { "mb2_up", 0 }, { "mb2_gain", 0 }, { "mb3_thr", -20 }, { "mb3_ratio", 2 }, { "mb3_up", 0.3f }, { "mb3_gain", 0 }, { "mx_on", 1 }, { "mx_gain", 4 }, { "mx_style", 1 }, { "mx_clip", 0.5f }, { "mx_char", 0.6f } } },
        { "Batteria | Room schiacciata", "Compressione 10:1 velocissima mescolata al 35%: la stanza esplode.",
          { { "bus_on", 1 }, { "bus_thr", -26 }, { "bus_ratio", 3 }, { "bus_att", 0 }, { "bus_rel", 0 }, { "bus_hpf", 0 }, { "bus_range", 20 }, { "bus_mix", 0.35f }, { "bus_auto", 0 }, { "bus_makeup", 6 } } },
        { "Batteria | Rullante crack", "Corpo a 200 Hz, schiocco a 5 kHz, compressione 4:1 con attacco lento.",
          { { "eq_on", 1 }, { "eq2_type", 0 }, { "eq2_f", 200 }, { "eq2_g", 1.5f }, { "eq2_q", 1 }, { "eq5_type", 0 }, { "eq5_f", 5000 }, { "eq5_g", 2 }, { "eq5_q", 1.2f }, { "bus_on", 1 }, { "bus_thr", -16 }, { "bus_ratio", 2 }, { "bus_att", 5 }, { "bus_rel", 1 }, { "bus_hpf", 80 }, { "bus_range", 6 }, { "bus_mix", 1 }, { "bus_auto", 1 }, { "bus_makeup", 0 } } },
        { "Batteria | 808 e cassa ferme", "Picchi dei bassi tenuti fermi, coda dell'808 stretta, bassi mono, compressore che non pompa.",
          { { "bc_on", 1 }, { "bc_freq", 130 }, { "bc_mono", 150 }, { "bc_tail", -0.3f }, { "bc_thr", -12 }, { "bc_dirty", 0.3f }, { "bus_on", 1 }, { "bus_thr", -16 }, { "bus_ratio", 1 }, { "bus_att", 4 }, { "bus_rel", 4 }, { "bus_hpf", 120 }, { "bus_range", 4 }, { "bus_mix", 1 }, { "bus_auto", 1 }, { "bus_makeup", 0 } } },
        { "Batteria | Lo-fi boom bap", "Alti tagliati a 9 kHz, compressione 4:1 in parallelo, saturazione morbida.",
          { { "eq_on", 1 }, { "eq6_type", 4 }, { "eq6_f", 9000 }, { "eq6_g", 0 }, { "eq6_q", 0.8f }, { "bus_on", 1 }, { "bus_thr", -16 }, { "bus_ratio", 2 }, { "bus_att", 5 }, { "bus_rel", 2 }, { "bus_hpf", 0 }, { "bus_range", 6 }, { "bus_mix", 0.6f }, { "bus_auto", 1 }, { "bus_makeup", 0 }, { "bc_on", 1 }, { "bc_freq", 110 }, { "bc_punch", -0.2f }, { "bc_tail", 0.2f }, { "mx_on", 1 }, { "mx_gain", 3 }, { "mx_style", 3 }, { "mx_clip", 0.5f }, { "mx_char", 0.3f } } },
        { "Chitarra | Elettrica rock", "Taglio sotto 90 Hz, presenza a 2.5 kHz, fango dinamico a 250 Hz, compressione 4:1 e larghezza.",
          { { "eq_on", 1 }, { "eq1_type", 3 }, { "eq1_f", 90 }, { "eq1_g", 0 }, { "eq1_q", 0.7f }, { "eq2_type", 0 }, { "eq2_f", 250 }, { "eq2_g", 0 }, { "eq2_q", 1.1f }, { "eq2_dyn", 1 }, { "eq2_thr", -26 }, { "eq2_rng", -2 }, { "eq4_type", 0 }, { "eq4_f", 2500 }, { "eq4_g", 1.5f }, { "eq4_q", 1 }, { "bus_on", 1 }, { "bus_thr", -16 }, { "bus_ratio", 2 }, { "bus_att", 4 }, { "bus_rel", 1 }, { "bus_hpf", 0 }, { "bus_range", 6 }, { "bus_mix", 1 }, { "bus_auto", 1 }, { "bus_makeup", 0 }, { "im_on", 1 }, { "im_smode", 0 }, { "im_w1", 1 }, { "im_s1", 0 }, { "im_w2", 1 }, { "im_s2", 0 }, { "im_w3", 1.2f }, { "im_s3", 0 }, { "im_w4", 1.3f }, { "im_s4", 0 } } },
        { "Chitarra | Acustica ariosa", "Rimbombo tenuto sotto controllo, aria a 10 kHz, fruscii delle dita domati dal de-esser a intervallo.",
          { { "eq_on", 1 }, { "eq1_type", 3 }, { "eq1_f", 70 }, { "eq1_g", 0 }, { "eq1_q", 0.7f }, { "eq2_type", 0 }, { "eq2_f", 200 }, { "eq2_g", 0 }, { "eq2_q", 1 }, { "eq2_dyn", 1 }, { "eq2_thr", -30 }, { "eq2_rng", -2.5f }, { "eq6_type", 2 }, { "eq6_f", 10000 }, { "eq6_g", 2.5f }, { "eq6_q", 0.7f }, { "ds_on", 1 }, { "ds_lo", 4000 }, { "ds_hi", 9000 }, { "ds_thr", -10 }, { "ds_amt", 0.3f }, { "ds_soft", 0.7f }, { "ds_ch", 0 }, { "bus_on", 1 }, { "bus_thr", -18 }, { "bus_ratio", 0 }, { "bus_att", 5 }, { "bus_rel", 4 }, { "bus_hpf", 0 }, { "bus_range", 3 }, { "bus_mix", 1 }, { "bus_auto", 1 }, { "bus_makeup", 0 } } },
        { "Chitarra | Clean jazz morbida", "Alti gentili, corpo a 300 Hz, compressione lenta e un filo di saturazione.",
          { { "eq_on", 1 }, { "eq3_type", 0 }, { "eq3_f", 300 }, { "eq3_g", 1 }, { "eq3_q", 0.9f }, { "eq6_type", 2 }, { "eq6_f", 8000 }, { "eq6_g", -1.5f }, { "eq6_q", 0.7f }, { "bus_on", 1 }, { "bus_thr", -18 }, { "bus_ratio", 1 }, { "bus_att", 5 }, { "bus_rel", 3 }, { "bus_hpf", 0 }, { "bus_range", 4 }, { "bus_mix", 1 }, { "bus_auto", 1 }, { "bus_makeup", 0 }, { "mx_on", 1 }, { "mx_gain", 2 }, { "mx_style", 3 }, { "mx_clip", 0.2f }, { "mx_char", 0.3f } } },
        { "Chitarra | Lead davanti", "Medi compressi e sollevati, presenza a 2 kHz, immagine un po' piu stretta e centrata.",
          { { "mb_on", 1 }, { "mb_mix", 1 }, { "mb_speed", 0.5f }, { "mb1_thr", 0 }, { "mb1_ratio", 1 }, { "mb1_up", 0 }, { "mb1_gain", 0 }, { "mb2_thr", -20 }, { "mb2_ratio", 3 }, { "mb2_up", 0.3f }, { "mb2_gain", 0 }, { "mb3_thr", 0 }, { "mb3_ratio", 1 }, { "mb3_up", 0 }, { "mb3_gain", 0 }, { "eq_on", 1 }, { "eq4_type", 0 }, { "eq4_f", 2000 }, { "eq4_g", 2 }, { "eq4_q", 1.2f }, { "im_on", 1 }, { "im_smode", 0 }, { "im_w1", 1 }, { "im_s1", 0 }, { "im_w2", 0.85f }, { "im_s2", 0 }, { "im_w3", 0.85f }, { "im_s3", 0 }, { "im_w4", 0.85f }, { "im_s4", 0 } } },
        { "Archi | Ensemble largo", "Durezze a 3 kHz domate in dinamica, aria sopra, alti larghi e diffusi, colla lenta.",
          { { "eq_on", 1 }, { "eq1_type", 3 }, { "eq1_f", 60 }, { "eq1_g", 0 }, { "eq1_q", 0.7f }, { "eq4_type", 0 }, { "eq4_f", 3000 }, { "eq4_g", 0 }, { "eq4_q", 1.2f }, { "eq4_dyn", 1 }, { "eq4_thr", -28 }, { "eq4_rng", -2 }, { "eq6_type", 2 }, { "eq6_f", 12000 }, { "eq6_g", 1.5f }, { "eq6_q", 0.7f }, { "im_on", 1 }, { "im_smode", 1 }, { "im_w1", 1 }, { "im_s1", 0 }, { "im_w2", 1 }, { "im_s2", 0 }, { "im_w3", 1.3f }, { "im_s3", 0.2f }, { "im_w4", 1.4f }, { "im_s4", 0.2f }, { "bus_on", 1 }, { "bus_thr", -20 }, { "bus_ratio", 0 }, { "bus_att", 5 }, { "bus_rel", 4 }, { "bus_hpf", 0 }, { "bus_range", 3 }, { "bus_mix", 1 }, { "bus_auto", 1 }, { "bus_makeup", 0 } } },
        { "Archi | Pizzicato definito", "Attacco a 1.5 kHz, compressione rapida 4:1, dettagli alti sollevati.",
          { { "eq_on", 1 }, { "eq3_type", 0 }, { "eq3_f", 1500 }, { "eq3_g", 1.5f }, { "eq3_q", 1 }, { "bus_on", 1 }, { "bus_thr", -18 }, { "bus_ratio", 2 }, { "bus_att", 3 }, { "bus_rel", 1 }, { "bus_hpf", 0 }, { "bus_range", 6 }, { "bus_mix", 1 }, { "bus_auto", 1 }, { "bus_makeup", 0 }, { "mb_on", 1 }, { "mb_mix", 1 }, { "mb_speed", 0.5f }, { "mb1_thr", 0 }, { "mb1_ratio", 1 }, { "mb1_up", 0 }, { "mb1_gain", 0 }, { "mb2_thr", 0 }, { "mb2_ratio", 1 }, { "mb2_up", 0 }, { "mb2_gain", 0 }, { "mb3_thr", -24 }, { "mb3_ratio", 2 }, { "mb3_up", 0.4f }, { "mb3_gain", 0 } } },
        { "Archi | Cinematico caldo", "Corpo sotto 150 Hz, alti scuri, larghezza diffusa e saturazione morbida.",
          { { "eq_on", 1 }, { "eq1_type", 1 }, { "eq1_f", 150 }, { "eq1_g", 1.5f }, { "eq1_q", 0.7f }, { "eq6_type", 2 }, { "eq6_f", 9000 }, { "eq6_g", -1 }, { "eq6_q", 0.7f }, { "im_on", 1 }, { "im_smode", 1 }, { "im_w1", 1 }, { "im_s1", 0 }, { "im_w2", 1 }, { "im_s2", 0 }, { "im_w3", 1.1f }, { "im_s3", 0 }, { "im_w4", 1.2f }, { "im_s4", 0.15f }, { "mx_on", 1 }, { "mx_gain", 3 }, { "mx_style", 3 }, { "mx_clip", 0.2f }, { "mx_char", 0.3f } } },
        { "Voce | Bus voci rap", "Sibilanti sulla voce al centro, fango dinamico, compressione 4:1 veloce e un po' di punch.",
          { { "ds_on", 1 }, { "ds_lo", 5500 }, { "ds_hi", 20000 }, { "ds_thr", -8 }, { "ds_amt", 0.5f }, { "ds_soft", 0.6f }, { "ds_ch", 1 }, { "eq_on", 1 }, { "eq2_type", 0 }, { "eq2_f", 300 }, { "eq2_g", 0 }, { "eq2_q", 1 }, { "eq2_dyn", 1 }, { "eq2_thr", -28 }, { "eq2_rng", -2 }, { "bus_on", 1 }, { "bus_thr", -18 }, { "bus_ratio", 2 }, { "bus_att", 3 }, { "bus_rel", 1 }, { "bus_hpf", 100 }, { "bus_range", 6 }, { "bus_mix", 1 }, { "bus_auto", 1 }, { "bus_makeup", 0 }, { "mx_on", 1 }, { "mx_gain", 3 }, { "mx_style", 1 }, { "mx_clip", 0.3f }, { "mx_char", 0.6f } } },
        { "Voce | Bus cori larghi", "Cori aperti e diffusi, sibilanti morbide, colla lenta.",
          { { "im_on", 1 }, { "im_smode", 1 }, { "im_w1", 1 }, { "im_s1", 0 }, { "im_w2", 1 }, { "im_s2", 0 }, { "im_w3", 1.3f }, { "im_s3", 0.3f }, { "im_w4", 1.5f }, { "im_s4", 0.3f }, { "ds_on", 1 }, { "ds_lo", 6000 }, { "ds_hi", 20000 }, { "ds_thr", -9 }, { "ds_amt", 0.35f }, { "ds_soft", 0.8f }, { "ds_ch", 0 }, { "bus_on", 1 }, { "bus_thr", -18 }, { "bus_ratio", 0 }, { "bus_att", 5 }, { "bus_rel", 4 }, { "bus_hpf", 80 }, { "bus_range", 3 }, { "bus_mix", 1 }, { "bus_auto", 1 }, { "bus_makeup", 0 } } } };
    return P;
}

void Processor::loadPreset (int i)
{
    if (! juce::isPositiveAndBelow (i, (int) presets().size())) return;
    preset = i;
    for (auto* p : getParameters()) if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p)) if (rp->paramID != "trim" && rp->paramID != "dither" && rp->paramID != "mode" && rp->paramID != "target" && rp->paramID != "lm_ceil") rp->setValueNotifyingHost (rp->getDefaultValue());
    if (i > 0) { auto* m = apvts.getParameter ("mode"); m->setValueNotifyingHost (m->convertTo0to1 (juce::String (presets()[(size_t) i].name).startsWith ("Facile") ? 0.0f : 1.0f)); }
    for (auto& [id, v] : presets()[(size_t) i].v) if (auto* rp = apvts.getParameter (id)) rp->setValueNotifyingHost (rp->convertTo0to1 (v));
    rackFromOn();   // la catena mostra gli effetti che il preset usa
}


// ---------------------------------------------------------------------------------------------------- preset per effetto
const std::vector<ModPreset>& modPresets()
{
    static const std::vector<ModPreset> P = {
        // BASS CONTROL
        { BASS, true,  "Sub mono e fermo", "Mono sotto 120 Hz e picchi del sub tenuti fermi: il punto di partenza per quasi tutto.", { { "bc_freq", 120 }, { "bc_mono", 120 }, { "bc_thr", -12 }, { "bc_dirty", 0.2f } } },
        { BASS, true,  "808 piu presente", "Armoniche sopra l'808: si sente anche su telefono e cuffiette.", { { "bc_freq", 110 }, { "bc_mono", 110 }, { "bc_harm", 0.4f }, { "bc_thr", -14 } } },
        { BASS, true,  "Cassa che picchia", "PUNCH sui bassi: piu attacco alla cassa, coda un filo piu corta.", { { "bc_freq", 120 }, { "bc_mono", 110 }, { "bc_punch", 0.5f }, { "bc_tail", -0.15f } } },
        { BASS, false, "Bassi caldi e lunghi", "WARM: attacco morbido e piu sustain, per boom bap e R&B.", { { "bc_freq", 110 }, { "bc_punch", -0.4f }, { "bc_tail", 0.25f } } },
        { BASS, false, "Solo mono sotto 100", "Tocca solo l'immagine: bassi al centro, il resto uguale.", { { "bc_mono", 100 } } },
        // EQ DINAMICO
        { DYNEQ, true,  "Aria e pulizia", "Taglio sotto 25 Hz, fango a 250 dinamico, aria a 12 kHz.", { { "eq1_type", 3 }, { "eq1_f", 25 }, { "eq2_type", 0 }, { "eq2_f", 250 }, { "eq2_dyn", 1 }, { "eq2_thr", -28 }, { "eq2_rng", -2 }, { "eq6_type", 2 }, { "eq6_f", 12000 }, { "eq6_g", 1.5f }, { "eq6_q", 0.7f }, { "eq3_on", 0 }, { "eq4_on", 0 }, { "eq5_on", 0 } } },
        { DYNEQ, true,  "Durezze sotto controllo", "2.5-4 kHz scendono solo quando pungono (voce urlata, synth, piatti).", { { "eq4_type", 0 }, { "eq4_f", 3200 }, { "eq4_q", 1.4f }, { "eq4_dyn", 1 }, { "eq4_thr", -30 }, { "eq4_rng", -3 }, { "eq1_on", 0 }, { "eq2_on", 0 }, { "eq3_on", 0 }, { "eq5_on", 0 }, { "eq6_on", 0 } } },
        { DYNEQ, true,  "Smile leggero", "+1 dB di bassi e alti con le shelf: piu pieno senza toccare la dinamica.", { { "eq1_type", 1 }, { "eq1_f", 60 }, { "eq1_g", 1 }, { "eq6_type", 2 }, { "eq6_f", 10000 }, { "eq6_g", 1 }, { "eq2_on", 0 }, { "eq3_on", 0 }, { "eq4_on", 0 }, { "eq5_on", 0 } } },
        { DYNEQ, false, "Fango e scatole", "250 e 500 Hz che si accumulano: scendono solo quando servono.", { { "eq2_type", 0 }, { "eq2_f", 250 }, { "eq2_dyn", 1 }, { "eq2_thr", -26 }, { "eq2_rng", -3 }, { "eq3_type", 0 }, { "eq3_f", 500 }, { "eq3_dyn", 1 }, { "eq3_thr", -28 }, { "eq3_rng", -2 }, { "eq1_on", 0 }, { "eq4_on", 0 }, { "eq5_on", 0 }, { "eq6_on", 0 } } },
        { DYNEQ, false, "Voce davanti (MID)", "Presenza solo al centro: la voce esce senza alzare il beat.", { { "eq4_type", 0 }, { "eq4_f", 3000 }, { "eq4_g", 1.5f }, { "eq4_ch", 1 }, { "eq1_on", 0 }, { "eq2_on", 0 }, { "eq3_on", 0 }, { "eq5_on", 0 }, { "eq6_on", 0 } } },
        // DE-ESSER
        { DEESS, true,  "Sibilanti leggere", "Tocca solo le S piu dure, non si sente lavorare.", { { "ds_lo", 6000 }, { "ds_thr", -8 }, { "ds_amt", 0.3f }, { "ds_soft", 0.7f } } },
        { DEESS, true,  "Piatti morbidi", "Hi-hat e piatti che pungono dopo il maximizer: sopra 8 kHz.", { { "ds_lo", 8000 }, { "ds_thr", -10 }, { "ds_amt", 0.4f }, { "ds_ch", 2 } } },
        { DEESS, false, "Voce rap", "Sulla voce al centro, piu deciso.", { { "ds_lo", 5000 }, { "ds_thr", -10 }, { "ds_amt", 0.5f }, { "ds_ch", 1 } } },
        // BUS COMP
        { BUSCOMP, true,  "Colla 2:1", "Il classico da master: 1-2 dB di lancetta, rilascio AUTO.", { { "bus_thr", -16 }, { "bus_ratio", 1 }, { "bus_att", 4 }, { "bus_rel", 4 }, { "bus_hpf", 80 }, { "bus_range", 4 } } },
        { BUSCOMP, true,  "Groove che respira", "Attacco lento e rilascio veloce: la cassa passa, il resto si muove con lei.", { { "bus_thr", -15 }, { "bus_ratio", 1 }, { "bus_att", 5 }, { "bus_rel", 0 }, { "bus_hpf", 100 }, { "bus_range", 4 } } },
        { BUSCOMP, false, "Glue trap 4:1", "Piu deciso, per il bus del beat prima della voce.", { { "bus_thr", -14 }, { "bus_ratio", 2 }, { "bus_att", 5 }, { "bus_rel", 0 }, { "bus_hpf", 100 }, { "bus_range", 6 } } },
        { BUSCOMP, false, "Parallelo New York", "10:1 schiacciato e mescolato al 30%: densita senza perdere i colpi.", { { "bus_thr", -24 }, { "bus_ratio", 3 }, { "bus_att", 2 }, { "bus_rel", 0 }, { "bus_range", 15 }, { "bus_mix", 0.3f }, { "bus_auto", 0 }, { "bus_makeup", 6 } } },
        // MULTIBANDA
        { MULTI, true,  "Controllo leggero", "Ognuna delle tre bande tocca 1-2 dB solo sui picchi.", { { "mb_mix", 1 }, { "mb1_thr", -20 }, { "mb1_ratio", 2 }, { "mb2_thr", -22 }, { "mb2_ratio", 2 }, { "mb3_thr", -24 }, { "mb3_ratio", 2 } } },
        { MULTI, true,  "OTT 20%", "Giu + su in stile OTT, poco: tutto piu denso e dettagliato.", { { "mb_mix", 0.2f }, { "mb_speed", 0.7f }, { "mb1_thr", -22 }, { "mb1_ratio", 12 }, { "mb1_up", 0.85f }, { "mb2_thr", -24 }, { "mb2_ratio", 12 }, { "mb2_up", 0.85f }, { "mb3_thr", -26 }, { "mb3_ratio", 12 }, { "mb3_up", 0.85f } } },
        { MULTI, false, "Bassi fermi", "Solo la banda bassa lavora: tiene insieme 808 e cassa.", { { "mb1_thr", -20 }, { "mb1_ratio", 4 }, { "mb2_thr", 0 }, { "mb3_thr", 0 } } },
        // EXCITER
        { EXCITER, true,  "Aria brillante", "Armoniche solo sugli alti: piu luce senza alzare l'EQ.", { { "ex3_amt", 0.3f }, { "ex3_mode", 2 }, { "ex_mix", 0.6f } } },
        { EXCITER, true,  "Calore valvolare", "Armoniche a valvola sui bassi-medi.", { { "ex1_amt", 0.3f }, { "ex1_mode", 0 }, { "ex2_amt", 0.15f }, { "ex2_mode", 1 }, { "ex_mix", 0.6f } } },
        { EXCITER, false, "Voce piu presente", "Medi con armoniche a nastro: la voce viene avanti.", { { "ex2_amt", 0.35f }, { "ex2_mode", 1 }, { "ex_mix", 0.7f } } },
        // TAPE
        { TAPE, true,  "15 IPS leggero", "Il colore del nastro senza saturare: bump sui bassi, alti morbidi.", { { "tp_model", 1 }, { "tp_drive", 0.2f }, { "tp_mix", 0.8f } } },
        { TAPE, true,  "30 IPS trasparente", "Coesione da macchina da master, quasi invisibile.", { { "tp_model", 0 }, { "tp_drive", 0.25f }, { "tp_mix", 1 } } },
        { TAPE, false, "Cassetta lo-fi", "Alti chiusi, saturazione evidente e un po' di fruscio.", { { "tp_model", 2 }, { "tp_drive", 0.5f }, { "tp_noise", 0.3f }, { "tp_mix", 1 } } },
        // IMAGER
        { IMAGER, true,  "Bassi stretti", "Mono sotto 120 Hz, il resto come prima.", { { "im_x1", 120 }, { "im_w1", 0.0f } } },
        { IMAGER, true,  "Alti aperti", "Medio-alti e alti piu larghi, con la sicurezza mono accesa.", { { "im_w1", 0.3f }, { "im_w3", 1.2f }, { "im_w4", 1.35f }, { "im_s4", 0.15f }, { "im_safe", 1 } } },
        { IMAGER, false, "Cori e pad larghi", "Diffusione ampia sugli alti (MODO II).", { { "im_smode", 1 }, { "im_w3", 1.3f }, { "im_s3", 0.3f }, { "im_w4", 1.5f }, { "im_s4", 0.3f } } },
        // MAXIMIZER
        { MAXIM, true,  "Streaming pulito (-14)", "Volume giusto per Spotify senza schiacciare: stile PULITO.", { { "mx_gain", 6 }, { "mx_style", 0 }, { "mx_clip", 0.15f } } },
        { MAXIM, true,  "Club (-8)", "Forte con la cassa che picchia: stile PUNCH e un po' di clip.", { { "mx_gain", 9 }, { "mx_style", 1 }, { "mx_clip", 0.45f }, { "mx_char", 0.6f } } },
        { MAXIM, true,  "Trap forte (-7)", "Il massimo con grinta: AGGRESSIVO, clipper deciso.", { { "mx_gain", 11 }, { "mx_style", 2 }, { "mx_clip", 0.6f }, { "mx_char", 0.7f } } },
        { MAXIM, true,  "Vintage morbido", "Arrotonda e satura un filo: boom bap, lo-fi, soul.", { { "mx_gain", 6 }, { "mx_style", 3 }, { "mx_clip", 0.1f } } },
        // LIMITER
        { LIMIT, true,  "Streaming -1 dBTP", "Ceiling sicuro per MP3/AAC di Spotify e YouTube.", { { "lm_ceil", -1 }, { "lm_tp", 1 }, { "lm_auto", 1 } } },
        { LIMIT, true,  "Club -0.3 dBTP", "Un filo di volume in piu per club, CD e SoundCloud.", { { "lm_ceil", -0.3f }, { "lm_tp", 1 }, { "lm_auto", 1 } } },
        { LIMIT, false, "Sicurezza bus", "Solo per non clippare il bus: ceiling -0.5, rilascio veloce.", { { "lm_ceil", -0.5f }, { "lm_tp", 0 }, { "lm_auto", 0 }, { "lm_rel", 40 } } },
        // EQ 432
        { EQ432, true,  "Lucido", "Mezzo dB di shelf sugli alti e una carezza ai bassi.", { { "e4_1g", 0.15f }, { "e4_5g", 0.2f } } },
        { EQ432, true,  "Caldo", "Medi bassi un filo su, alti appena giu.", { { "e4_2g", 0.15f }, { "e4_5g", -0.12f }, { "e4_col", 0.3f } } },
        { EQ432, false, "Meno scatola", "Medi bassi giu di poco, presenza su.", { { "e4_2g", -0.2f }, { "e4_4g", 0.15f } } },
        // PULTEC
        { PULTEC, true,  "Trucco dei bassi", "Boost e atten a 60 Hz insieme: pieni e puliti.", { { "pq_lf", 2 }, { "pq_lb", 4 }, { "pq_la", 3 } } },
        { PULTEC, true,  "Aria a 12 kHz", "Alti morbidi e larghi, senza durezza.", { { "pq_hf", 5 }, { "pq_hb", 3 }, { "pq_bw", 7 } } },
        { PULTEC, false, "Valvola calda", "Bassi pieni, alti appena giu e piu valvola.", { { "pq_lf", 1 }, { "pq_lb", 3 }, { "pq_la", 2 }, { "pq_as", 1 }, { "pq_ha", 1.5f }, { "pq_tube", 0.6f } } },
        // VARI-MU
        { VARIMU, true,  "Colla valvolare", "1-2 dB lenti e morbidi: incolla e scurisce appena.", { { "vm_thr", -20 }, { "vm_att", 3 }, { "vm_rel", 6 }, { "vm_mix", 1 } } },
        { VARIMU, true,  "Parallelo caldo", "Piu compressione mescolata al 50%: corpo senza perdere dinamica.", { { "vm_thr", -28 }, { "vm_att", 2 }, { "vm_rel", 6 }, { "vm_mix", 0.5f }, { "vm_tube", 0.5f } } },
        { VARIMU, false, "Bus pieno", "Per il bus di batteria o strumenti: piu decisa.", { { "vm_thr", -26 }, { "vm_att", 1 }, { "vm_rel", 2 }, { "vm_mix", 0.8f } } },
        // ---- aggiunte (quasi tutte per il master) ----
        { BASS, true,  "Bassi da club", "Mono fino a 140 Hz e picchi del sub tenuti stretti: regge impianti grandi.", { { "bc_freq", 130 }, { "bc_mono", 140 }, { "bc_thr", -10 }, { "bc_dirty", 0.3f } } },
        { BASS, true,  "Morbido e rotondo", "WARM leggero e un filo di armoniche: bassi caldi senza rimbombo.", { { "bc_freq", 110 }, { "bc_mono", 100 }, { "bc_punch", -0.3f }, { "bc_harm", 0.2f } } },
        { BASS, false, "Basso del beat", "Sul bus del beat: coda dell'808 stretta e picchi fermi prima della voce.", { { "bc_freq", 130 }, { "bc_thr", -14 }, { "bc_tail", -0.2f } } },
        { DYNEQ, true, "Togli il rimbombo", "120 Hz scende solo quando il basso si gonfia.", { { "eq2_type", 0 }, { "eq2_f", 120 }, { "eq2_q", 1.2f }, { "eq2_dyn", 1 }, { "eq2_thr", -22 }, { "eq2_rng", -3 }, { "eq1_on", 0 }, { "eq3_on", 0 }, { "eq4_on", 0 }, { "eq5_on", 0 }, { "eq6_on", 0 } } },
        { DYNEQ, true, "Medi meno nasali", "Intorno a 1 kHz, dinamico: il mix smette di suonare 'al telefono'.", { { "eq3_type", 0 }, { "eq3_f", 1000 }, { "eq3_q", 1.3f }, { "eq3_dyn", 1 }, { "eq3_thr", -26 }, { "eq3_rng", -2 }, { "eq1_on", 0 }, { "eq2_on", 0 }, { "eq4_on", 0 }, { "eq5_on", 0 }, { "eq6_on", 0 } } },
        { DYNEQ, true, "Master scuro piu aperto", "Shelf a 8 kHz e aria a 14 kHz: luce senza durezza.", { { "eq5_type", 2 }, { "eq5_f", 8000 }, { "eq5_g", 1.5f }, { "eq5_q", 0.7f }, { "eq6_type", 2 }, { "eq6_f", 14000 }, { "eq6_g", 1 }, { "eq1_on", 0 }, { "eq2_on", 0 }, { "eq3_on", 0 }, { "eq4_on", 0 } } },
        { DYNEQ, true, "Sibilanti dinamiche", "Banda stretta a 7 kHz che scende solo sulle S, alternativa al de-esser.", { { "eq5_type", 0 }, { "eq5_f", 7000 }, { "eq5_q", 3 }, { "eq5_dyn", 1 }, { "eq5_thr", -30 }, { "eq5_rng", -4 }, { "eq1_on", 0 }, { "eq2_on", 0 }, { "eq3_on", 0 }, { "eq4_on", 0 }, { "eq6_on", 0 } } },
        { DEESS, true, "Sibilanti forti", "Per voci molto brillanti: piu deciso, attento alla voce sdentata.", { { "ds_lo", 5500 }, { "ds_thr", -12 }, { "ds_amt", 0.6f }, { "ds_soft", 0.5f } } },
        { DEESS, true, "Solo ai lati", "Lavora solo sul SIDE: piatti larghi piu morbidi, la voce al centro resta intatta.", { { "ds_lo", 7000 }, { "ds_thr", -10 }, { "ds_amt", 0.45f }, { "ds_ch", 2 } } },
        { BUSCOMP, true, "Colla leggerissima", "1.5:1 lento: quasi non si sente, ma il mix sta insieme.", { { "bus_thr", -18 }, { "bus_ratio", 0 }, { "bus_att", 5 }, { "bus_rel", 4 }, { "bus_hpf", 60 }, { "bus_range", 3 } } },
        { BUSCOMP, true, "Punch parallelo 50%", "4:1 con attacco lento mescolato a meta: batteria piu viva.", { { "bus_thr", -20 }, { "bus_ratio", 2 }, { "bus_att", 5 }, { "bus_rel", 1 }, { "bus_range", 8 }, { "bus_mix", 0.5f } } },
        { MULTI, true, "Domare i bassi", "Solo la banda bassa, lenta: bassi stabili in tutto il pezzo.", { { "mb_speed", 0.3f }, { "mb1_thr", -18 }, { "mb1_ratio", 3 }, { "mb2_thr", 0 }, { "mb3_thr", 0 } } },
        { MULTI, true, "Alti sotto controllo", "La banda alta scende sui piatti e sulle S piu forti.", { { "mb_speed", 0.7f }, { "mb3_thr", -22 }, { "mb3_ratio", 3 }, { "mb1_thr", 0 }, { "mb2_thr", 0 } } },
        { MULTI, true, "OTT 35% energia", "Piu denso e aggressivo: per trap, EDM e rap.", { { "mb_mix", 0.35f }, { "mb_speed", 0.75f }, { "mb1_thr", -22 }, { "mb1_ratio", 12 }, { "mb1_up", 0.85f }, { "mb2_thr", -24 }, { "mb2_ratio", 12 }, { "mb2_up", 0.85f }, { "mb3_thr", -26 }, { "mb3_ratio", 12 }, { "mb3_up", 0.85f } } },
        { EXCITER, true, "Tutto piu vivo", "Un po' di armoniche su tutte e tre le bande.", { { "ex1_amt", 0.15f }, { "ex2_amt", 0.15f }, { "ex3_amt", 0.2f }, { "ex_mix", 0.6f } } },
        { EXCITER, true, "Bassi sul telefono", "Armoniche a valvola sui bassi: l'808 si sente anche senza sub.", { { "ex_x1", 200 }, { "ex1_amt", 0.4f }, { "ex1_mode", 0 }, { "ex_mix", 0.7f } } },
        { TAPE, true, "15 IPS caldo", "Piu drive: spessore e calore evidenti.", { { "tp_model", 1 }, { "tp_drive", 0.45f }, { "tp_bias", 0.1f }, { "tp_mix", 1 } } },
        { TAPE, true, "30 IPS saturato", "Bassi profondi e saturazione fine, alti aperti.", { { "tp_model", 0 }, { "tp_drive", 0.5f }, { "tp_mix", 1 } } },
        { IMAGER, true, "Centro piu solido", "Medi un filo piu stretti: voce e cassa piu al centro.", { { "im_w1", 0.0f }, { "im_w2", 0.85f } } },
        { IMAGER, true, "Larghezza moderata", "Solo gli alti un po' piu aperti, tutto il resto com'e'.", { { "im_w4", 1.2f }, { "im_safe", 1 } } },
        { MAXIM, true, "Pop -10", "Brillante e forte ma senza esagerare: PUNCH con poco clip.", { { "mx_gain", 8 }, { "mx_style", 1 }, { "mx_clip", 0.3f } } },
        { MAXIM, true, "Rap -8", "Cassa e voce davanti, clipper medio.", { { "mx_gain", 9.5f }, { "mx_style", 1 }, { "mx_clip", 0.5f }, { "mx_char", 0.65f } } },
        { MAXIM, true, "Morbido -12", "Dinamico e caldo, per R&B, soul e jazz.", { { "mx_gain", 5 }, { "mx_style", 3 }, { "mx_clip", 0.05f } } },
        { LIMIT, true, "Apple -1 lento", "Rilascio lungo: trasparente per musica dinamica.", { { "lm_ceil", -1 }, { "lm_tp", 1 }, { "lm_auto", 0 }, { "lm_rel", 150 } } },
        { LIMIT, true, "Trasparente", "Rilascio automatico e ceiling -0.8: tocca solo i picchi veri.", { { "lm_ceil", -0.8f }, { "lm_tp", 1 }, { "lm_auto", 1 } } },
        { EQ432, true, "Aria", "Solo la shelf alta, un filo: luce su tutto.", { { "e4_5g", 0.3f } } },
        { EQ432, true, "Corpo", "Bassi e medio-bassi su di poco: piu pieno.", { { "e4_1g", 0.2f }, { "e4_2g", 0.1f } } },
        { PULTEC, true, "Bassi enormi", "Boost forte e atten a 60 Hz: sub pieno, senza fango.", { { "pq_lf", 2 }, { "pq_lb", 6 }, { "pq_la", 4 } } },
        { PULTEC, true, "Brillante morbido", "Boost largo a 10 kHz e un filo di atten a 20 kHz.", { { "pq_hf", 4 }, { "pq_hb", 5 }, { "pq_bw", 8 }, { "pq_as", 2 }, { "pq_ha", 1 } } },
        { VARIMU, true, "Limita i picchi", "Modo LIMIT: tiene fermi i colpi piu forti con il suono delle valvole.", { { "vm_mode", 1 }, { "vm_thr", -14 }, { "vm_att", 1 }, { "vm_rel", 1 } } },
        { VARIMU, true, "Solo il centro (M/S)", "Comprime il centro: voce e cassa piu ferme, i lati respirano.", { { "vm_ms", 1 }, { "vm_thr", -20 }, { "vm_mix", 1 } } } };
    return P;
}
static bool ownsParam (int m, const juce::String& id)
{
    static const char* pre[kNumMods] = { "bc_", "eq", "ds_", "bus_", "mb", "ex", "tp_", "im_", "mx_", "lm_", "e4_", "pq_", "vm_" };
    return id.startsWith (pre[juce::jlimit (0, kNumMods - 1, m)]);
}
void Processor::loadModPreset (int i)
{
    if (! juce::isPositiveAndBelow (i, (int) modPresets().size())) return;
    const auto& mp = modPresets()[(size_t) i];
    // l'effetto riparte neutro, poi i valori del preset: il resto della catena non si tocca
    for (auto* p : getParameters()) if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
        if (ownsParam (mp.mod, rp->paramID) && rp->paramID != onParam (mp.mod) && rp->paramID != deltaParam (mp.mod)) rp->setValueNotifyingHost (rp->getDefaultValue());
    for (auto& [id, v] : mp.v) if (auto* rp = apvts.getParameter (id)) rp->setValueNotifyingHost (rp->convertTo0to1 (v));
    if (auto* on = apvts.getParameter (onParam (mp.mod))) on->setValueNotifyingHost (1.0f);
    inRack[mp.mod] = true;
}

void Processor::getStateInformation (juce::MemoryBlock& d)
{
    auto s = apvts.copyState(); s.setProperty ("preset", preset, nullptr);
    juce::StringArray o; for (int k = 0; k < kNumMods; ++k) o.add (juce::String (order[k].load())); s.setProperty ("order", o.joinIntoString (","), nullptr);
    { int mask = 0; for (int m = 0; m < kNumMods; ++m) if (inRack[m].load()) mask |= 1 << m; s.setProperty ("rack", mask, nullptr); }
    s.setProperty ("ui", juce::String (uiTheme.load()) + "," + juce::String (uiScan.load()) + "," + juce::String (uiWin.load()) + "," + juce::String (uiSpec.load()) + "," + juce::String (uiNorm.load()) + "," + juce::String (uiRel.load()) + "," + juce::String (uiPrev.load()), nullptr);
    if (auto x = s.createXml()) copyXmlToBinary (*x, d);
}
void Processor::setStateInformation (const void* data, int size)
{
    if (auto x = getXmlFromBinary (data, size)) { auto t = juce::ValueTree::fromXml (*x); if (t.isValid()) { preset = t.getProperty ("preset", 0);
        if (! t.getChildWithProperty ("id", "mode").isValid()) { juce::ValueTree pm ("PARAM"); pm.setProperty ("id", "mode", nullptr); pm.setProperty ("value", 1.0f, nullptr); t.appendChild (pm, nullptr); }
        { const auto u = juce::StringArray::fromTokens (t.getProperty ("ui", "").toString(), ",", "");
          std::atomic<int>* dst[7] = { &uiTheme, &uiScan, &uiWin, &uiSpec, &uiNorm, &uiRel, &uiPrev };
          for (int i = 0; i < juce::jmin (7, u.size()); ++i) dst[i]->store (u[i].getIntValue()); }
        const auto o = juce::StringArray::fromTokens (t.getProperty ("order", "").toString(), ",", "");
        if (o.size() > 0 && o.size() <= kNumMods)
        {   // ordine salvato: i moduli che quel progetto non conosceva vanno al loro posto di default, subito dopo il modulo che li precede
            std::vector<int> v; bool ok = true; bool seen[kNumMods] {};
            for (auto& t2 : o) { const int m = t2.getIntValue(); if (m < 0 || m >= kNumMods || seen[m]) { ok = false; break; } seen[m] = true; v.push_back (m); }
            if (ok)
            {
                const auto def = defaultOrder();
                for (int k = 0; k < kNumMods; ++k)
                {
                    const int m = def[(size_t) k]; if (seen[m]) continue;
                    auto pos = v.begin();
                    if (k > 0) { const int prev = def[(size_t) k - 1]; auto it = std::find (v.begin(), v.end(), prev); pos = it == v.end() ? v.end() : it + 1; }
                    v.insert (pos, m); seen[m] = true;
                }
                std::array<int, kNumMods> a {}; for (int k = 0; k < kNumMods; ++k) a[(size_t) k] = v[(size_t) k]; setOrder (a);
            }
        }
        apvts.replaceState (t);
        if (t.hasProperty ("rack")) { const int mask = t.getProperty ("rack"); for (int m = 0; m < kNumMods; ++m) inRack[m] = ((mask >> m) & 1) != 0; }
        else rackFromOn(); } }
}

juce::AudioProcessorEditor* Processor::createEditor() { return new Editor (*this); }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new jms::Processor(); }
