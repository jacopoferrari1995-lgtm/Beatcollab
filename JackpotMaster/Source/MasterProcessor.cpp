#include "MasterProcessor.h"
#include "MasterEditor.h"

namespace jms
{
const char* onParam (int m)
{
    static const char* p[kNumMods] = { "bc_on", "eq_on", "ds_on", "bus_on", "mb_on", "ex_on", "tp_on", "im_on", "mx_on", "lm_on" };
    return p[juce::jlimit (0, kNumMods - 1, m)];
}
const char* deltaParam (int m)
{
    static const char* p[kNumMods] = { "bc_delta", "eq_delta", "ds_delta", "bus_delta", "mb_delta", "ex_delta", "tp_delta", "im_delta", "mx_delta", "lm_delta" };
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
        static const float df[kEqBands] = { 45, 150, 500, 2000, 6000, 12000 };
        static const int dt[kEqBands] = { Biquad::LSHELF, Biquad::BELL, Biquad::BELL, Biquad::BELL, Biquad::BELL, Biquad::HSHELF };
        for (int b = 0; b < kEqBands; ++b)
        {
            const String P = "EQ " + String (b + 1) + " ";
            l.add (std::make_unique<AudioParameterBool> (ParameterID { eqId (b, "on"), 1 }, P + "On", true));
            l.add (std::make_unique<AudioParameterChoice> (ParameterID { eqId (b, "type"), 1 }, P + "Type", StringArray { "CAMPANA", "SHELF BASSI", "SHELF ALTI", "TAGLIO BASSI", "TAGLIO ALTI" }, dt[b]));
            l.add (std::make_unique<AudioParameterChoice> (ParameterID { eqId (b, "ch"), 1 }, P + "Channel", StringArray { "STEREO", "MID", "SIDE" }, 0));
            l.add (std::make_unique<AudioParameterFloat> (ParameterID { eqId (b, "f"), 1 }, P + "Freq", NormalisableRange<float> (20, 20000, 0, 0.25f), df[b], AudioParameterFloatAttributes().withLabel ("Hz")));
            l.add (std::make_unique<AudioParameterFloat> (ParameterID { eqId (b, "g"), 1 }, P + "Gain", NormalisableRange<float> (-18, 18, 0.1f), 0, AudioParameterFloatAttributes().withLabel ("dB")));
            l.add (std::make_unique<AudioParameterFloat> (ParameterID { eqId (b, "q"), 1 }, P + "Q", NormalisableRange<float> (0.3f, 8, 0, 0.45f), 0.9f));
            l.add (std::make_unique<AudioParameterBool> (ParameterID { eqId (b, "dyn"), 1 }, P + "Dynamic", false));
            l.add (std::make_unique<AudioParameterFloat> (ParameterID { eqId (b, "thr"), 1 }, P + "Threshold", NormalisableRange<float> (-60, 0, 0.1f), -30, AudioParameterFloatAttributes().withLabel ("dB")));
            l.add (std::make_unique<AudioParameterFloat> (ParameterID { eqId (b, "rng"), 1 }, P + "Range", NormalisableRange<float> (-18, 18, 0.1f), -6, AudioParameterFloatAttributes().withLabel ("dB")));
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
    return l;
}

Processor::Processor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true).withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "JMS", layout())
{
    for (int k = 0; k < kNumMods; ++k) order[k] = k;
    for (int i = 0; i < kHist; ++i) { hIn[i] = -60; hOut[i] = -60; hGrM[i] = 0; hGrL[i] = 0; hS[i] = -60; }
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
    mx.prepare (s, mb); lim.prepare (s);
    bass.prepare (s); deq.prepare (s); img.prepare (s); dess.prepare (s); bus.prepare (s); mband.prepare (s); tape.prepare (s); exc.prepare (s);
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
    for (int i = 0; i < n; ++i)
    {
        L[i] *= trim; R[i] *= trim;
        pi[0] = juce::jmax (pi[0], std::abs (L[i])); pi[1] = juce::jmax (pi[1], std::abs (R[i]));
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
    for (int k = 0; k < kNumMods; ++k)
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
                    EqBand B[kEqBands];
                    for (int b = 0; b < kEqBands; ++b)
                    {
                        B[b].on = value (eqId (b, "on")) > 0.5f; B[b].type = (int) value (eqId (b, "type")); B[b].ch = (int) value (eqId (b, "ch"));
                        B[b].f = value (eqId (b, "f")); B[b].g = value (eqId (b, "g")); B[b].q = value (eqId (b, "q"));
                        B[b].dyn = value (eqId (b, "dyn")) > 0.5f; B[b].thr = value (eqId (b, "thr")); B[b].rng = value (eqId (b, "rng"));
                    }
                    if (d) copyIn();
                    deq.process (L, R, n, B);
                    if (d) subIn();
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
                    S.mode = (int) value ("im_smode"); S.delta = d;
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
    for (int b = 0; b < kEqBands; ++b) { eqDyn[b] = value ("eq_on") > 0.5f ? deq.dynNow[b] : 0.0f; eqLvl[b] = deq.levelDb[b]; }
    for (int b = 0; b < kImBands; ++b) imCorr[b] = img.corr (b);

    // ---- misure d'uscita (prima di confronto e ascolto normalizzato)
    float po[2] = { 0, 0 }; float tpb = 0;
    for (int i = 0; i < n; ++i)
    {
        loudOut.push (L[i], R[i]);
        po[0] = juce::jmax (po[0], std::abs (L[i])); po[1] = juce::jmax (po[1], std::abs (R[i]));
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

    // ---- ascolto in mono (controllo)
    if (value ("im_mono") > 0.5f) for (int i = 0; i < n; ++i) { const float m = 0.5f * (L[i] + R[i]); L[i] = R[i] = m; }
    // ---- dither (sempre l'ultimo)
    dith.process (L, R, n, (int) value ("dither"));

    // ---- misure
    lufsM = loudOut.M; lufsS = loudOut.S; lufsI = loudOut.I; lra = loudOut.lra; tpMax = tpHold; inS = loudIn.S;
    grMax = mx.gr; grLim = lim.gr;
    for (int c = 0; c < 2; ++c) { inPk[c] = juce::jmax (pi[c], inPk[c].load() * 0.9f); outPk[c] = juce::jmax (po[c], outPk[c].load() * 0.9f); }
    colIn = juce::jmax (colIn, juce::jmax (pi[0], pi[1])); colOut = juce::jmax (colOut, juce::jmax (po[0], po[1]));
    colM = juce::jmax (colM, mx.gr); colL = juce::jmax (colL, lim.gr); colN += n;
    if (colN >= (int) (sr * 0.025))
    {
        const int w = hW.load();
        hIn[w] = gToDb (colIn); hOut[w] = gToDb (colOut); hGrM[w] = colM; hGrL[w] = colL; hS[w] = loudOut.S;
        hW = (w + 1) % kHist; colIn = colOut = colM = colL = 0; colN = 0;
    }
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
        { "Master | Trap moderno", "Target -7 LUFS. Bassi mono sotto 120 Hz con punch, picchi dell'808 tenuti fermi, presenza dinamica a 3.5 kHz, aria sopra, colla leggera e maximizer PUNCH.",
          { { "bc_on", 1 }, { "bc_freq", 110 }, { "bc_mono", 120 }, { "bc_punch", 0.3f }, { "bc_thr", -14 }, { "bc_dirty", 0.2f }, { "bc_harm", 0.15f }, { "eq_on", 1 }, { "eq1_type", 1 }, { "eq1_f", 40 }, { "eq1_g", 1 }, { "eq1_q", 0.7f }, { "eq5_type", 0 }, { "eq5_f", 3500 }, { "eq5_g", 0 }, { "eq5_q", 1.5f }, { "eq5_dyn", 1 }, { "eq5_thr", -30 }, { "eq5_rng", -3 }, { "eq6_type", 2 }, { "eq6_f", 11000 }, { "eq6_g", 1.5f }, { "eq6_q", 0.7f }, { "ds_on", 1 }, { "ds_lo", 6000 }, { "ds_hi", 20000 }, { "ds_thr", -8 }, { "ds_amt", 0.35f }, { "ds_soft", 0.6f }, { "ds_ch", 0 }, { "bus_on", 1 }, { "bus_thr", -16 }, { "bus_ratio", 1 }, { "bus_att", 4 }, { "bus_rel", 4 }, { "bus_hpf", 90 }, { "bus_range", 4 }, { "bus_mix", 1 }, { "bus_auto", 1 }, { "bus_makeup", 0 }, { "im_on", 1 }, { "im_smode", 0 }, { "im_w1", 0.4f }, { "im_s1", 0 }, { "im_w2", 1 }, { "im_s2", 0 }, { "im_w3", 1.15f }, { "im_s3", 0 }, { "im_w4", 1.3f }, { "im_s4", 0.15f }, { "mx_on", 1 }, { "mx_gain", 10 }, { "mx_style", 1 }, { "mx_clip", 0.5f }, { "mx_char", 0.6f }, { "lm_on", 1 }, { "lm_ceil", -0.5f }, { "lm_tp", 1 }, { "target", 5 } } },
        { "Master | Trap muro (-6)", "Il massimo volume possibile: clipper deciso, maximizer AGGRESSIVO, 808 controllato sporco. Solo se il mix lo regge.",
          { { "bc_on", 1 }, { "bc_freq", 110 }, { "bc_mono", 130 }, { "bc_punch", 0.2f }, { "bc_thr", -10 }, { "bc_dirty", 0.5f }, { "bc_harm", 0.2f }, { "eq_on", 1 }, { "eq6_type", 2 }, { "eq6_f", 12000 }, { "eq6_g", 1 }, { "eq6_q", 0.7f }, { "bus_on", 1 }, { "bus_thr", -14 }, { "bus_ratio", 1 }, { "bus_att", 5 }, { "bus_rel", 4 }, { "bus_hpf", 100 }, { "bus_range", 3 }, { "bus_mix", 1 }, { "bus_auto", 1 }, { "bus_makeup", 0 }, { "im_on", 1 }, { "im_smode", 0 }, { "im_w1", 0.3f }, { "im_s1", 0 }, { "im_w2", 1 }, { "im_s2", 0 }, { "im_w3", 1.1f }, { "im_s3", 0 }, { "im_w4", 1.25f }, { "im_s4", 0 }, { "mx_on", 1 }, { "mx_gain", 13 }, { "mx_style", 2 }, { "mx_clip", 0.75f }, { "mx_char", 0.75f }, { "lm_on", 1 }, { "lm_ceil", -0.3f }, { "lm_tp", 1 }, { "target", 5 } } },
        { "Master | Drill", "Cassa e rullante in faccia, 808 stretto (CODA corta), compressione piu decisa e maximizer aggressivo.",
          { { "bc_on", 1 }, { "bc_freq", 120 }, { "bc_mono", 120 }, { "bc_punch", 0.5f }, { "bc_tail", -0.35f }, { "bc_thr", -12 }, { "bc_dirty", 0.4f }, { "eq_on", 1 }, { "eq2_type", 0 }, { "eq2_f", 250 }, { "eq2_g", -1.5f }, { "eq2_q", 1 }, { "eq5_type", 0 }, { "eq5_f", 4000 }, { "eq5_g", 0 }, { "eq5_q", 1.4f }, { "eq5_dyn", 1 }, { "eq5_thr", -28 }, { "eq5_rng", -2.5f }, { "bus_on", 1 }, { "bus_thr", -14 }, { "bus_ratio", 2 }, { "bus_att", 4 }, { "bus_rel", 2 }, { "bus_hpf", 100 }, { "bus_range", 5 }, { "bus_mix", 1 }, { "bus_auto", 1 }, { "bus_makeup", 0 }, { "mx_on", 1 }, { "mx_gain", 11 }, { "mx_style", 2 }, { "mx_clip", 0.6f }, { "mx_char", 0.7f }, { "lm_on", 1 }, { "lm_ceil", -0.3f }, { "lm_tp", 1 }, { "target", 4 } } },
        { "Master | Boom Bap caldo", "Bassi WARM con piu sustain, alti morbidi, colla lenta in parallelo e maximizer MORBIDO.",
          { { "bc_on", 1 }, { "bc_freq", 100 }, { "bc_mono", 100 }, { "bc_punch", -0.3f }, { "bc_tail", 0.2f }, { "eq_on", 1 }, { "eq1_type", 1 }, { "eq1_f", 80 }, { "eq1_g", 1.5f }, { "eq1_q", 0.7f }, { "eq6_type", 2 }, { "eq6_f", 10000 }, { "eq6_g", -1.5f }, { "eq6_q", 0.7f }, { "bus_on", 1 }, { "bus_thr", -14 }, { "bus_ratio", 1 }, { "bus_att", 5 }, { "bus_rel", 4 }, { "bus_hpf", 0 }, { "bus_range", 5 }, { "bus_mix", 0.8f }, { "bus_auto", 1 }, { "bus_makeup", 0 }, { "mx_on", 1 }, { "mx_gain", 7 }, { "mx_style", 3 }, { "mx_clip", 0.3f }, { "mx_char", 0.35f }, { "lm_on", 1 }, { "lm_ceil", -1 }, { "lm_tp", 1 }, { "target", 3 } } },
        { "Master | R&B morbido", "Dinamico e lucido: de-esser sulla voce al centro, aria a 12 kHz, alti un filo piu larghi e diffusi.",
          { { "bc_on", 1 }, { "bc_freq", 120 }, { "bc_mono", 120 }, { "bc_punch", -0.2f }, { "eq_on", 1 }, { "eq3_type", 0 }, { "eq3_f", 500 }, { "eq3_g", -1 }, { "eq3_q", 0.8f }, { "eq6_type", 2 }, { "eq6_f", 12000 }, { "eq6_g", 2 }, { "eq6_q", 0.7f }, { "ds_on", 1 }, { "ds_lo", 5500 }, { "ds_hi", 20000 }, { "ds_thr", -10 }, { "ds_amt", 0.3f }, { "ds_soft", 0.8f }, { "ds_ch", 1 }, { "bus_on", 1 }, { "bus_thr", -18 }, { "bus_ratio", 0 }, { "bus_att", 5 }, { "bus_rel", 4 }, { "bus_hpf", 80 }, { "bus_range", 3 }, { "bus_mix", 1 }, { "bus_auto", 1 }, { "bus_makeup", 0 }, { "im_on", 1 }, { "im_smode", 1 }, { "im_w1", 0.5f }, { "im_s1", 0 }, { "im_w2", 1 }, { "im_s2", 0 }, { "im_w3", 1.05f }, { "im_s3", 0 }, { "im_w4", 1.2f }, { "im_s4", 0.2f }, { "mx_on", 1 }, { "mx_gain", 5 }, { "mx_style", 0 }, { "mx_clip", 0.1f }, { "mx_char", 0.3f }, { "lm_on", 1 }, { "lm_ceil", -1 }, { "lm_tp", 1 }, { "target", 0 } } },
        { "Master | Pop brillante", "Brillante e presente: presenza dinamica, multibanda leggero sugli alti, maximizer PUNCH.",
          { { "bc_on", 1 }, { "bc_freq", 110 }, { "bc_mono", 110 }, { "bc_punch", 0.15f }, { "eq_on", 1 }, { "eq4_type", 0 }, { "eq4_f", 3000 }, { "eq4_g", 0 }, { "eq4_q", 1.2f }, { "eq4_dyn", 1 }, { "eq4_thr", -28 }, { "eq4_rng", -2 }, { "eq6_type", 2 }, { "eq6_f", 10000 }, { "eq6_g", 2 }, { "eq6_q", 0.7f }, { "bus_on", 1 }, { "bus_thr", -16 }, { "bus_ratio", 1 }, { "bus_att", 4 }, { "bus_rel", 4 }, { "bus_hpf", 80 }, { "bus_range", 4 }, { "bus_mix", 1 }, { "bus_auto", 1 }, { "bus_makeup", 0 }, { "mb_on", 1 }, { "mb_mix", 0.25f }, { "mb_speed", 0.5f }, { "mb1_thr", 0 }, { "mb1_ratio", 2 }, { "mb1_up", 0 }, { "mb1_gain", 0 }, { "mb2_thr", -20 }, { "mb2_ratio", 2 }, { "mb2_up", 0 }, { "mb2_gain", 0 }, { "mb3_thr", -24 }, { "mb3_ratio", 3 }, { "mb3_up", 0.3f }, { "mb3_gain", 0 }, { "im_on", 1 }, { "im_smode", 0 }, { "im_w1", 0.6f }, { "im_s1", 0 }, { "im_w2", 1 }, { "im_s2", 0 }, { "im_w3", 1.1f }, { "im_s3", 0 }, { "im_w4", 1.25f }, { "im_s4", 0 }, { "mx_on", 1 }, { "mx_gain", 7 }, { "mx_style", 1 }, { "mx_clip", 0.3f }, { "mx_char", 0.5f }, { "lm_on", 1 }, { "lm_ceil", -1 }, { "lm_tp", 1 }, { "target", 0 } } },
        { "Master | Lo-fi polveroso", "Taglio alto a 12 kHz, medi caldi, poco volume e tanta morbidezza.",
          { { "eq_on", 1 }, { "eq1_type", 3 }, { "eq1_f", 30 }, { "eq1_g", 0 }, { "eq1_q", 0.7f }, { "eq3_type", 0 }, { "eq3_f", 400 }, { "eq3_g", 1 }, { "eq3_q", 0.8f }, { "eq6_type", 4 }, { "eq6_f", 12000 }, { "eq6_g", 0 }, { "eq6_q", 0.9f }, { "bus_on", 1 }, { "bus_thr", -14 }, { "bus_ratio", 1 }, { "bus_att", 5 }, { "bus_rel", 3 }, { "bus_hpf", 0 }, { "bus_range", 5 }, { "bus_mix", 0.7f }, { "bus_auto", 1 }, { "bus_makeup", 0 }, { "im_on", 1 }, { "im_smode", 0 }, { "im_w1", 0.6f }, { "im_s1", 0 }, { "im_w2", 0.9f }, { "im_s2", 0 }, { "im_w3", 0.9f }, { "im_s3", 0 }, { "im_w4", 0.8f }, { "im_s4", 0 }, { "mx_on", 1 }, { "mx_gain", 4 }, { "mx_style", 3 }, { "mx_clip", 0.4f }, { "mx_char", 0.25f }, { "lm_on", 1 }, { "lm_ceil", -1 }, { "lm_tp", 1 }, { "target", 0 } } },
        { "Master | Afro / Reggaeton", "Kick con punch, medi bassi puliti, alti brillanti, colla veloce sul groove.",
          { { "bc_on", 1 }, { "bc_freq", 110 }, { "bc_mono", 110 }, { "bc_punch", 0.45f }, { "bc_tail", -0.1f }, { "eq_on", 1 }, { "eq2_type", 0 }, { "eq2_f", 200 }, { "eq2_g", -1 }, { "eq2_q", 1 }, { "eq6_type", 2 }, { "eq6_f", 9000 }, { "eq6_g", 1.5f }, { "eq6_q", 0.7f }, { "bus_on", 1 }, { "bus_thr", -15 }, { "bus_ratio", 1 }, { "bus_att", 4 }, { "bus_rel", 1 }, { "bus_hpf", 90 }, { "bus_range", 4 }, { "bus_mix", 1 }, { "bus_auto", 1 }, { "bus_makeup", 0 }, { "mx_on", 1 }, { "mx_gain", 9 }, { "mx_style", 1 }, { "mx_clip", 0.45f }, { "mx_char", 0.6f }, { "lm_on", 1 }, { "lm_ceil", -0.3f }, { "lm_tp", 1 }, { "target", 4 } } },
        { "Master | Rock / band live", "Fango a 250 Hz tenuto sotto controllo solo quando serve, colla in parallelo 4:1, maximizer PUNCH.",
          { { "eq_on", 1 }, { "eq2_type", 0 }, { "eq2_f", 250 }, { "eq2_g", 0 }, { "eq2_q", 1 }, { "eq2_dyn", 1 }, { "eq2_thr", -26 }, { "eq2_rng", -2 }, { "eq6_type", 2 }, { "eq6_f", 8000 }, { "eq6_g", 1 }, { "eq6_q", 0.7f }, { "bus_on", 1 }, { "bus_thr", -16 }, { "bus_ratio", 2 }, { "bus_att", 5 }, { "bus_rel", 2 }, { "bus_hpf", 60 }, { "bus_range", 6 }, { "bus_mix", 0.6f }, { "bus_auto", 1 }, { "bus_makeup", 0 }, { "im_on", 1 }, { "im_smode", 0 }, { "im_w1", 0.7f }, { "im_s1", 0 }, { "im_w2", 1 }, { "im_s2", 0 }, { "im_w3", 1.1f }, { "im_s3", 0 }, { "im_w4", 1.1f }, { "im_s4", 0 }, { "mx_on", 1 }, { "mx_gain", 7 }, { "mx_style", 1 }, { "mx_clip", 0.25f }, { "mx_char", 0.5f }, { "lm_on", 1 }, { "lm_ceil", -1 }, { "lm_tp", 1 }, { "target", 2 } } },
        { "Master | EDM / Club", "Bassi mono fino a 150 Hz, multibanda tipo OTT leggero, alti larghi, volume da club.",
          { { "bc_on", 1 }, { "bc_freq", 120 }, { "bc_mono", 150 }, { "bc_punch", 0.4f }, { "bc_thr", -10 }, { "bc_dirty", 0.3f }, { "mb_on", 1 }, { "mb_mix", 0.2f }, { "mb_speed", 0.65f }, { "mb1_thr", -24 }, { "mb1_ratio", 6 }, { "mb1_up", 0.5f }, { "mb1_gain", 0 }, { "mb2_thr", -24 }, { "mb2_ratio", 6 }, { "mb2_up", 0.5f }, { "mb2_gain", 0 }, { "mb3_thr", -26 }, { "mb3_ratio", 6 }, { "mb3_up", 0.5f }, { "mb3_gain", 0 }, { "im_on", 1 }, { "im_smode", 0 }, { "im_w1", 0.2f }, { "im_s1", 0 }, { "im_w2", 1 }, { "im_s2", 0 }, { "im_w3", 1.2f }, { "im_s3", 0.1f }, { "im_w4", 1.4f }, { "im_s4", 0.3f }, { "mx_on", 1 }, { "mx_gain", 11 }, { "mx_style", 2 }, { "mx_clip", 0.6f }, { "mx_char", 0.7f }, { "lm_on", 1 }, { "lm_ceil", -0.3f }, { "lm_tp", 1 }, { "target", 4 } } },
        { "Master | Acustico / jazz", "Quasi trasparente: colla 1.5:1 lenta e solo il volume necessario per Apple Music.",
          { { "eq_on", 1 }, { "eq6_type", 2 }, { "eq6_f", 12000 }, { "eq6_g", 1 }, { "eq6_q", 0.7f }, { "bus_on", 1 }, { "bus_thr", -20 }, { "bus_ratio", 0 }, { "bus_att", 5 }, { "bus_rel", 4 }, { "bus_hpf", 0 }, { "bus_range", 3 }, { "bus_mix", 1 }, { "bus_auto", 1 }, { "bus_makeup", 0 }, { "mx_on", 1 }, { "mx_gain", 3 }, { "mx_style", 0 }, { "mx_clip", 0 }, { "mx_char", 0.25f }, { "lm_on", 1 }, { "lm_ceil", -1 }, { "lm_tp", 1 }, { "target", 1 } } },
        { "Master | Streaming sicuro", "Punto di partenza pulito per Spotify e YouTube a -14: bassi mono, sibilanti domate, colla leggera.",
          { { "bc_on", 1 }, { "bc_freq", 110 }, { "bc_mono", 100 }, { "ds_on", 1 }, { "ds_lo", 6000 }, { "ds_hi", 20000 }, { "ds_thr", -9 }, { "ds_amt", 0.3f }, { "ds_soft", 0.6f }, { "ds_ch", 0 }, { "bus_on", 1 }, { "bus_thr", -18 }, { "bus_ratio", 1 }, { "bus_att", 4 }, { "bus_rel", 4 }, { "bus_hpf", 80 }, { "bus_range", 3 }, { "bus_mix", 1 }, { "bus_auto", 1 }, { "bus_makeup", 0 }, { "mx_on", 1 }, { "mx_gain", 5 }, { "mx_style", 0 }, { "mx_clip", 0.15f }, { "mx_char", 0.4f }, { "lm_on", 1 }, { "lm_ceil", -1 }, { "lm_tp", 1 }, { "target", 0 } } },
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
    for (auto* p : getParameters()) if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p)) if (rp->paramID != "trim" && rp->paramID != "dither") rp->setValueNotifyingHost (rp->getDefaultValue());
    for (auto& [id, v] : presets()[(size_t) i].v) if (auto* rp = apvts.getParameter (id)) rp->setValueNotifyingHost (rp->convertTo0to1 (v));
}

void Processor::getStateInformation (juce::MemoryBlock& d)
{
    auto s = apvts.copyState(); s.setProperty ("preset", preset, nullptr);
    juce::StringArray o; for (int k = 0; k < kNumMods; ++k) o.add (juce::String (order[k].load())); s.setProperty ("order", o.joinIntoString (","), nullptr);
    if (auto x = s.createXml()) copyXmlToBinary (*x, d);
}
void Processor::setStateInformation (const void* data, int size)
{
    if (auto x = getXmlFromBinary (data, size)) { auto t = juce::ValueTree::fromXml (*x); if (t.isValid()) { preset = t.getProperty ("preset", 0);
        const auto o = juce::StringArray::fromTokens (t.getProperty ("order", "").toString(), ",", "");
        if (o.size() == kNumMods) { std::array<int, kNumMods> a {}; bool ok = true; for (int k = 0; k < kNumMods; ++k) { a[(size_t) k] = o[k].getIntValue(); ok &= a[(size_t) k] >= 0 && a[(size_t) k] < kNumMods; } if (ok) setOrder (a); }
        apvts.replaceState (t); } }
}

juce::AudioProcessorEditor* Processor::createEditor() { return new Editor (*this); }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new jms::Processor(); }
