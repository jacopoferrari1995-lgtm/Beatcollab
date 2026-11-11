#include "TapeProcessor.h"
#include "TapeEditor.h"

namespace jt
{
const FxInfo kFx[kNumFx] = {
    { "stop",   "Tape Stop",   "il nastro frena fino a fermarsi",          0xffff6b6b, '1' },
    { "start",  "Tape Start",  "riparte da fermo",                          0xff4ade80, '2' },
    { "spin",   "Spinback",    "riavvolge come un DJ",                      0xfffbbf24, '3' },
    { "half",   "Half Speed",  "rallenta e scende di tono",                 0xff60a5fa, '4' },
    { "rev",    "Reverse",     "suona al contrario",                        0xfff472b6, '5' },
    { "stut",   "Stutter",     "ripete, accelera, cambia tono",             0xffa78bfa, '6' },
    { "loop",   "Ripeti zona", "il beat si ferma sulla zona e la ripete",   0xff2dd4bf, '7' },
    { "glitch", "Glitch",      "frammenti ripetuti, girati, saltati",       0xffa3e635, '8' },
    { "gate",   "Gate",        "taglia il volume a tempo",                  0xfffacc15, '9' },
    { "crush",  "Crush",       "saturazione, distorsione e bitcrusher per lo stacco", 0xffff8a3d, '0' },
    { "sweep",  "Sweep",       "filtro che si chiude (passa basso) o si apre (passa alto) sulla zona", 0xff38bdf8, 'Q' },
    { "echo",   "Eco coda",    "manda l'ultimo colpo in un eco a tempo che resta dopo la zona", 0xffe879f9, 'W' },
    { "throw",  "Lancio rev",  "lancia la zona in un riverbero che resta in coda", 0xff94a3b8, 'E' },
    { "riser",  "Riser",       "sale di tono (e si apre) per il build-up", 0xfffde047, 'R' },
    { "trem",   "Tremolo",     "il volume pulsa a tempo, anche tra destra e sinistra", 0xffffe066, 'T' },
    { "rota",   "Rotary",      "cassa rotante: il suono gira, lento o veloce", 0xffd9a066, 'Y' },
    { "afil",   "Autofiltro",  "filtro risonante che si apre e chiude a tempo (wah)", 0xffffb000, 'U' },
    { "ring",   "Ring mod",    "moltiplica per un tono: robot, campane, metallo", 0xffff5ef2, 'I' },
    { "fshf",   "Freq shift",  "sposta tutte le frequenze di qualche Hz: metallico e instabile", 0xff00d4a0, 'O' },
    { "dist",   "Distorsione", "drive da ampli: crunch, distorsione o fuzz sulla zona", 0xffff4d2e, 'P' } };

const Step kDur[6]  = { { 0.25, "1/16" }, { 0.5, "1/8" }, { 1, "1/4" }, { 2, "1/2" }, { 4, "1 bar" }, { 8, "2 bar" } };
const Step kSize[6] = { { 0.125, "1/32" }, { 1.0 / 6, "1/16T" }, { 0.25, "1/16" }, { 1.0 / 3, "1/8T" }, { 0.5, "1/8" }, { 1, "1/4" } };
const Step kRate[5] = { { 0.5, "1/8" }, { 1.0 / 3, "1/8T" }, { 0.25, "1/16" }, { 1.0 / 6, "1/16T" }, { 0.125, "1/32" } };

const PSpec kParams[] = {
    { "stop_len", "Durata", STOP, 0, 5, 3, 1, 0 },        { "stop_curve", "Curva", STOP, -1, 1, -0.35f, 0, 1 },
    { "start_len", "Durata", START, 0, 5, 2, 1, 0 },      { "start_curve", "Curva", START, -1, 1, 0.3f, 0, 1 },
    { "spin_len", "Durata", SPIN, 0, 5, 2, 1, 0 },        { "spin_force", "Forza", SPIN, 1, 5, 3, 0, 5 },
    { "half_div", "Rallenta", HALF, 1, 4, 2, 0, 7 }, { "half_glide", "Glide", HALF, 0, 400, 70, 0, 3 },
    { "rev_speed", "Velocita", REV, 0.5f, 1.5f, 1, 0, 2 },
    { "stut_size", "Misura", STUT, 0, 5, 2, 2, 0 },       { "stut_roll", "Roll", STUT, 0, 1, 0, 0, 0 },
    { "stut_pitch", "Pitch", STUT, -12, 12, 0, 0, 4 },    { "stut_gate", "Gate", STUT, 0.2f, 1, 0.9f, 0, 0 },
    { "loop_size", "Zona", LOOP, 0, 5, 4, 1, 0 },         { "loop_xf", "Morbido", LOOP, 0, 1, 0.3f, 0, 0 },
    { "glitch_dens", "Densita", GLITCH, 0, 1, 0.6f, 0, 0 }, { "glitch_chaos", "Caos", GLITCH, 0, 1, 0.5f, 0, 0 }, { "glitch_size", "Grana", GLITCH, 0, 5, 2, 2, 0 },
    { "gate_rate", "Rate", GATE, 0, 4, 2, 3, 0 },         { "gate_depth", "Prof.", GATE, 0, 1, 1, 0, 0 }, { "gate_duty", "Apertura", GATE, 0.1f, 0.9f, 0.5f, 0, 0 },
    { "stut_decay", "Decay", STUT, 0, 1, 0, 0, 0 },       { "stut_filt", "Filtro", STUT, -1, 1, 0, 0, 1 },
    { "stop_vol", "Volume", STOP, -18, 6, 0, 0, 6 },      { "start_vol", "Volume", START, -18, 6, 0, 0, 6 },   { "spin_vol", "Volume", SPIN, -18, 6, 0, 0, 6 },
    { "half_vol", "Volume", HALF, -18, 6, 0, 0, 6 },      { "rev_vol", "Volume", REV, -18, 6, 0, 0, 6 },       { "stut_vol", "Volume", STUT, -18, 6, 0, 0, 6 },
    { "loop_vol", "Volume", LOOP, -18, 6, 0, 0, 6 },      { "glitch_vol", "Volume", GLITCH, -18, 6, 0, 0, 6 }, { "gate_vol", "Volume", GATE, -18, 6, 0, 0, 6 },
    { "crush_drive", "Drive", CRUSH, 0, 1, 0.5f, 0, 0 },  { "crush_char", "Carattere", CRUSH, 0, 1, 0.3f, 0, 8 }, { "crush_bits", "Bit", CRUSH, 2, 16, 16, 0, 9 },
    { "crush_rate", "Campioni", CRUSH, 1, 32, 1, 0, 10 },  { "crush_tone", "Tono", CRUSH, 0, 1, 1, 0, 0 },          { "crush_vol", "Volume", CRUSH, -18, 6, 0, 0, 6 },
    { "stop_mix", "Mix", STOP, 0, 1, 1, 0, 0 },  { "start_mix", "Mix", START, 0, 1, 1, 0, 0 },  { "spin_mix", "Mix", SPIN, 0, 1, 1, 0, 0 },
    { "half_mix", "Mix", HALF, 0, 1, 1, 0, 0 },  { "rev_mix", "Mix", REV, 0, 1, 1, 0, 0 },      { "stut_mix", "Mix", STUT, 0, 1, 1, 0, 0 },
    { "loop_mix", "Mix", LOOP, 0, 1, 1, 0, 0 },  { "glitch_mix", "Mix", GLITCH, 0, 1, 1, 0, 0 }, { "gate_mix", "Mix", GATE, 0, 1, 1, 0, 0 },
    { "crush_mix", "Mix", CRUSH, 0, 1, 1, 0, 0 },
    { "sweep_len", "Durata", SWEEP, 0, 5, 4, 1, 0 },     { "sweep_dir", "Direzione", SWEEP, -1, 1, 1, 0, 11 }, { "sweep_reso", "Risonanza", SWEEP, 0, 1, 0.35f, 0, 0 },
    { "sweep_depth", "Quanto", SWEEP, 0, 1, 0.85f, 0, 0 }, { "sweep_vol", "Volume", SWEEP, -18, 6, 0, 0, 6 },  { "sweep_mix", "Mix", SWEEP, 0, 1, 1, 0, 0 },
    { "echo_time", "Tempo", ECHO, 0, 5, 4, 2, 0 },       { "echo_fb", "Ripetizioni", ECHO, 0, 0.95f, 0.55f, 0, 0 }, { "echo_cut", "Taglia dry", ECHO, 0, 1, 1, 0, 0 },
    { "echo_tone", "Tono", ECHO, 0, 1, 0.55f, 0, 0 },    { "echo_vol", "Volume", ECHO, -18, 6, 0, 0, 6 },    { "echo_mix", "Mix", ECHO, 0, 1, 0.8f, 0, 0 },
    { "throw_size", "Stanza", THROW, 0, 1, 0.8f, 0, 0 }, { "throw_tone", "Tono", THROW, 0, 1, 0.5f, 0, 0 }, { "throw_cut", "Taglia dry", THROW, 0, 1, 0.6f, 0, 0 },
    { "throw_pre", "Pre-delay", THROW, 0, 200, 30, 0, 3 }, { "throw_vol", "Volume", THROW, -18, 6, 0, 0, 6 }, { "throw_mix", "Mix", THROW, 0, 1, 0.8f, 0, 0 },
    { "riser_len", "Durata", RISER, 0, 5, 5, 1, 0 },     { "riser_pitch", "Sale di", RISER, 0, 24, 12, 0, 4 }, { "riser_curve", "Curva", RISER, -1, 1, 0.4f, 0, 1 },
    { "riser_open", "Apre", RISER, 0, 1, 0.5f, 0, 0 },   { "riser_noise", "Rumore", RISER, 0, 1, 0.2f, 0, 0 }, { "riser_vol", "Volume", RISER, -18, 6, 0, 0, 6 },
    { "riser_mix", "Mix", RISER, 0, 1, 1, 0, 0 },
    { "trem_rate", "Rate", TREM, 0, 4, 2, 3, 0 },        { "trem_depth", "Prof.", TREM, 0, 1, 0.8f, 0, 0 },   { "trem_stereo", "Stereo", TREM, 0, 1, 0, 0, 0 },
    { "trem_shape", "Spigolo", TREM, 0, 1, 0.3f, 0, 0 }, { "trem_vol", "Volume", TREM, -18, 6, 0, 0, 6 },     { "trem_mix", "Mix", TREM, 0, 1, 1, 0, 0 },
    { "rota_speed", "Velocita", ROTARY, 0, 1, 1, 0, 13 }, { "rota_depth", "Prof.", ROTARY, 0, 1, 0.7f, 0, 0 }, { "rota_drive", "Drive", ROTARY, 0, 1, 0.2f, 0, 0 },
    { "rota_vol", "Volume", ROTARY, -18, 6, 0, 0, 6 },   { "rota_mix", "Mix", ROTARY, 0, 1, 1, 0, 0 },
    { "afil_rate", "Rate", AUTOFILT, 0, 5, 2, 1, 0 },    { "afil_freq", "Centro", AUTOFILT, 0, 1, 0.45f, 0, 14 }, { "afil_depth", "Escursione", AUTOFILT, 0, 1, 0.7f, 0, 0 },
    { "afil_reso", "Risonanza", AUTOFILT, 0, 1, 0.6f, 0, 0 }, { "afil_vol", "Volume", AUTOFILT, -18, 6, 0, 0, 6 }, { "afil_mix", "Mix", AUTOFILT, 0, 1, 1, 0, 0 },
    { "ring_freq", "Tono", RINGMOD, 20, 2000, 220, 0, 12 }, { "ring_glide", "Scende", RINGMOD, 0, 1, 0, 0, 0 }, { "ring_vol", "Volume", RINGMOD, -18, 6, 0, 0, 6 },
    { "ring_mix", "Mix", RINGMOD, 0, 1, 0.7f, 0, 0 },
    { "fshf_hz", "Sposta", FSHIFT, -400, 400, 40, 0, 12 }, { "fshf_fb", "Feedback", FSHIFT, 0, 0.9f, 0.3f, 0, 0 }, { "fshf_vol", "Volume", FSHIFT, -18, 6, 0, 0, 6 },
    { "fshf_mix", "Mix", FSHIFT, 0, 1, 0.8f, 0, 0 },
    { "dist_drive", "Drive", DIST, 0, 1, 0.6f, 0, 0 },   { "dist_type", "Tipo", DIST, 0, 1, 0.5f, 0, 15 },     { "dist_tone", "Tono", DIST, 0, 1, 0.5f, 0, 0 },
    { "dist_vol", "Volume", DIST, -18, 6, 0, 0, 6 },     { "dist_mix", "Mix", DIST, 0, 1, 1, 0, 0 } };
const int kNumParams = (int) (sizeof (kParams) / sizeof (kParams[0]));

static const juce::StringArray kModes { "HOLD", "ONE-SHOT" }, kQuant { "OFF", "1/16", "1/4", "BAR" }, kBars { "1", "2", "4", "8" };
static const double kQBeats[] = { 0, 0.25, 1, 4 };
static const int kBarsN[] = { 1, 2, 4, 8 };

static juce::StringArray tableLabels (int t)
{
    juce::StringArray a;
    if (t == 1) for (auto& s : kDur) a.add (s.label);
    if (t == 2) for (auto& s : kSize) a.add (s.label);
    if (t == 3) for (auto& s : kRate) a.add (s.label);
    return a;
}
static const Step* table (int t) { return t == 1 ? kDur : t == 2 ? kSize : kRate; }

juce::AudioProcessorValueTreeState::ParameterLayout Processor::layout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    for (int i = 0; i < kNumParams; ++i)
    {
        const auto& p = kParams[i];
        const String name = String (kFx[p.fx].name) + " " + p.label;
        if (p.table) l.add (std::make_unique<AudioParameterChoice> (ParameterID { p.id, 1 }, name, tableLabels (p.table), (int) p.def));
        else l.add (std::make_unique<AudioParameterFloat> (ParameterID { p.id, 1 }, name, NormalisableRange<float> (p.min, p.max), p.def));
    }
    auto f = [&l] (const char* id, const char* n, float a, float b, float d) { l.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, n, NormalisableRange<float> (a, b), d)); };
    f ("age", "Age", 0, 1, 0); f ("mix", "Mix", 0, 1, 1); f ("out", "Output", -24, 6, 0);
    f ("wow", "Wow", 0, 1, 0); f ("flut", "Flutter", 0, 1, 0); f ("tone", "Tono", 0, 1, 1);
    f ("drive", "Drive", 0, 1, 0); f ("hiss", "Hiss", 0, 1, 0); f ("drop", "Dropout", 0, 1, 0);
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "mode", 1 }, "Modo pad", kModes, 0));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "quant", 1 }, "Quantizza", kQuant, 1));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "seqOn", 1 }, "Coreografia", false));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "bars", 1 }, "Lunghezza", kBars, 1));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "sync", 1 }, "BPM dall'host", true));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "genStyle", 1 }, "Stile coreografia", StringArray { "TRAP", "DRILL", "BOOM BAP", "LO-FI", "GLITCH", "BUILD-UP" }, 0));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "genAmt", 1 }, "Intensita coreografia", NormalisableRange<float> (0, 1), 0.5f));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "bpm", 1 }, "BPM manuale", NormalisableRange<float> (50, 220, 0.1f), 140));
    for (int i = 0; i < kNumFx; ++i) l.add (std::make_unique<AudioParameterBool> (ParameterID { "pad" + String (i + 1), 1 }, String ("Pad ") + kFx[i].name, false));
    for (int i = 0; i < kNumFx; ++i) l.add (std::make_unique<AudioParameterBool> (ParameterID { String (kFx[i].id) + "_on", 1 }, String (kFx[i].name) + " attivo", true));
    return l;
}

Processor::Processor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true).withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", layout())
{
    for (auto& r : seq) for (auto& s : r) s = false;
    for (auto& p : uiPad) p = false;
    for (int i = 0; i < kCols; ++i) { dispWet[i] = 0; dispDry[i] = 0; dispSpd[i] = 1; dispMask[i] = 0; }
    for (int i = 0; i < kNumParams; ++i) paramPtr[i] = apvts.getRawParameterValue (kParams[i].id);
    for (int i = 0; i < kNumFx; ++i) padPtr[i] = apvts.getRawParameterValue ("pad" + juce::String (i + 1));
    for (int i = 0; i < kNumFx; ++i) onPtr[i] = apvts.getRawParameterValue (juce::String (kFx[i].id) + "_on");
}

float Processor::fxValue (int fx, const char* p) const
{
    const size_t n = std::strlen (kFx[fx].id);
    for (int i = 0; i < kNumParams; ++i)
        if (kParams[i].fx == fx && std::strcmp (kParams[i].id + n + 1, p) == 0)
        {
            const float v = paramPtr[i]->load();
            return kParams[i].table ? (float) table (kParams[i].table)[juce::jlimit (0, kParams[i].table == 3 ? 4 : 5, (int) v)].v : v;
        }
    return 0.0f;
}

void Processor::prepareToPlay (double s, int)
{
    sr = s;
    HL = 1 << 21; while (HL < sr * 30.0) HL <<= 1; HM = HL - 1;
    hist.setSize (2, HL); hist.clear();
    T = 0; R = 0; live = true; xf.on = false;
    for (auto& f : F) f = FxState();
    juce::dsp::ProcessSpec spec { sr, 4096, 1 };
    bumpL.prepare (spec); bumpR.prepare (spec);
    eBuf.setSize (2, (int) (sr * 4.5) + 8); eBuf.clear(); eW = 0; eSend = eCut = 0; eTime = 0;
    verb.setSampleRate (sr); verb.reset(); vPre.setSize (2, (int) (sr * 0.25) + 8); vPre.clear(); vW = 0; vSend = vCut = 0;
    rBuf.setSize (2, 8192); rBuf.clear(); rW = 0; rPh = 0; rFade = 0;
    rotBuf.setSize (2, 4096); rotBuf.clear(); rotW = 0; for (auto& m : modFade) m = 0; rotSp = 0;
    {   // Hilbert IIR (due catene di allpass in quadratura)
        const float ca[4] = { 0.6923878f, 0.9360654322959f, 0.9882295226860f, 0.9987488452737f }, cb[4] = { 0.4021921162426f, 0.8561710882420f, 0.9722909545651f, 0.9952884791278f };
        for (int c = 0; c < 2; ++c) for (int k = 0; k < 4; ++k) { hA[c][k] = {}; hB[c][k] = {}; hA[c][k].c = ca[k] * ca[k]; hB[c][k].c = cb[k] * cb[k]; }
    }
    swFade = 0; for (auto& a : swIc) a[0] = a[1] = 0;
}

float Processor::rd (double pos, int ch) const
{
    pos = juce::jlimit ((double) (T - HL + 8), (double) (T - 2), pos);
    const juce::int64 i = (juce::int64) std::floor (pos); const float f = (float) (pos - (double) i);
    const float* d = hist.getReadPointer (ch);
    const float a = d[(i - 1) & HM], b = d[i & HM], c = d[(i + 1) & HM], e = d[(i + 2) & HM];
    return b + 0.5f * f * (c - a + f * (2.0f * a - 5.0f * b + 4.0f * c - e + f * (3.0f * (b - c) + e - a)));
}

static inline float shape (float x, float c) { x = juce::jlimit (0.0f, 1.0f, x); return c >= 0 ? 1.0f - std::pow (1.0f - x, 1.0f + 3.0f * c) : std::pow (x, 1.0f - 3.0f * c); }

void Processor::activate (int fx, double bf, double beatAtT, juce::int64 t)
{
    auto& X = F[fx]; X.t0 = t; X.k = 0; X.op = -1; for (auto& d : X.d) d = 0;
    const double rNow = live ? (double) (t - 3) : R;
    const double beatR = beatAtT - ((double) t - rNow) / bf;
    if (fx == START && ! live) { xf = { true, R, 1.0, 0, juce::jmax (32, (int) (0.008 * sr)), false }; R = (double) (t - 3); }
    if (fx == STUT)
    {
        const double g = juce::jmin (fxValue (STUT, "size"), 0.25f);
        const double anchorBeat = std::floor (beatR / g + 1e-6) * g;
        X.d[0] = (double) t - (beatAtT - anchorBeat) * bf; X.d[1] = (double) t; X.d[2] = fxValue (STUT, "size") * bf;
    }
    if (fx == LOOP)
    {
        const double z = fxValue (LOOP, "size");
        const double zb = std::floor (beatR / z + 1e-6) * z;
        X.d[0] = (double) t - (beatAtT - zb) * bf; X.d[1] = z * bf;
    }
    if (fx == GLITCH) { X.d[0] = (double) t; X.d[3] = rNow; }
    if (fx == STUT) { stLp[0] = stLp[1] = 0; }
}

void Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals nd;
    const int N = buffer.getNumSamples();
    if (buffer.getNumChannels() < 2) return;
    float* io[2] = { buffer.getWritePointer (0), buffer.getWritePointer (1) };

    double bpm = value ("bpm"); bool hostPlaying = false; double ppq = 0;
    const bool sync = value ("sync") > 0.5f;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (sync) if (auto b = pos->getBpm()) bpm = *b;
            hostPlaying = pos->getIsPlaying();
            if (auto p = pos->getPpqPosition()) ppq = *p;
        }
    bpm = juce::jlimit (30.0, 300.0, bpm);
    const double bf = 60.0 / bpm * sr;
    const double beat0 = hostPlaying ? ppq : freeBeat;
    const juce::int64 T0 = T;
    uiBpm = bpm; uiHost = sync && hostPlaying;

    const bool shotMode = value ("mode") > 0.5f;
    const double q = kQBeats[juce::jlimit (0, 3, (int) value ("quant"))];
    const bool seqOn = value ("seqOn") > 0.5f;
    const int steps = kBarsN[juce::jlimit (0, 3, (int) value ("bars"))] * 16;
    const double loopBeats = steps / 4.0;

    // parametri effetto
    const float stopLen = fxValue (STOP, "len"), stopCurve = fxValue (STOP, "curve"), startLen = fxValue (START, "len"), startCurve = fxValue (START, "curve");
    const float spinLen = fxValue (SPIN, "len"), spinForce = fxValue (SPIN, "force"), halfSpeed = 1.0f / juce::jmax (1.0f, fxValue (HALF, "div")), halfGlide = fxValue (HALF, "glide");
    const float revSpeed = fxValue (REV, "speed"), stutSize = fxValue (STUT, "size"), stutRoll = fxValue (STUT, "roll"), stutPitch = fxValue (STUT, "pitch"), stutGate = fxValue (STUT, "gate");
    const float loopXf = fxValue (LOOP, "xf"), gDens = fxValue (GLITCH, "dens"), gChaos = fxValue (GLITCH, "chaos"), gSize = fxValue (GLITCH, "size");
    const float gateRate = fxValue (GATE, "rate"), gateDepth = fxValue (GATE, "depth"), gateDuty = fxValue (GATE, "duty");
    const float stutDecay = fxValue (STUT, "decay"), stutFilt = fxValue (STUT, "filt");
    float fxVol[kNumFx], fxMix[kNumFx]; for (int f = 0; f < kNumFx; ++f) { fxVol[f] = juce::Decibels::decibelsToGain (fxValue (f, "vol")); fxMix[f] = fxValue (f, "mix"); }
    const float crDrive = fxValue (CRUSH, "drive"), crChar = fxValue (CRUSH, "char"), crBits = fxValue (CRUSH, "bits"), crTone = fxValue (CRUSH, "tone");
    const int crRate = juce::jmax (1, juce::roundToInt (fxValue (CRUSH, "rate")));
    const float crA = (float) std::exp (-juce::MathConstants<double>::twoPi * (900.0 * std::pow (20000.0 / 900.0, (double) crTone)) / sr);
    const float swLen = fxValue (SWEEP, "len"), swDir = fxValue (SWEEP, "dir"), swRes = fxValue (SWEEP, "reso"), swDepth = fxValue (SWEEP, "depth");
    const float eTimeT = (float) (fxValue (ECHO, "time") * bf), eFb = fxValue (ECHO, "fb"), eCutP = fxValue (ECHO, "cut"), eTone = fxValue (ECHO, "tone");
    const float vCutP = fxValue (THROW, "cut"), vPreN = fxValue (THROW, "pre") * 0.001f * (float) sr;
    const float trRate = fxValue (TREM, "rate"), trDepth = fxValue (TREM, "depth"), trStereo = fxValue (TREM, "stereo"), trShape = fxValue (TREM, "shape");
    const float rtSpeed = fxValue (ROTARY, "speed"), rtDepth = fxValue (ROTARY, "depth"), rtDrive = fxValue (ROTARY, "drive");
    const float afRate = fxValue (AUTOFILT, "rate"), afFreq = fxValue (AUTOFILT, "freq"), afDepth = fxValue (AUTOFILT, "depth"), afReso = fxValue (AUTOFILT, "reso");
    const float rmFreq = fxValue (RINGMOD, "freq"), rmGlide = fxValue (RINGMOD, "glide"), fsHz = fxValue (FSHIFT, "hz"), fsFbAmt = fxValue (FSHIFT, "fb");
    const float dsDrive = fxValue (DIST, "drive"), dsType = fxValue (DIST, "type"), dsTone = fxValue (DIST, "tone");
    const float rLen = fxValue (RISER, "len"), rPitch = fxValue (RISER, "pitch"), rCurve = fxValue (RISER, "curve"), rOpen = fxValue (RISER, "open"), rNoise = fxValue (RISER, "noise");
    {
        juce::Reverb::Parameters vp; vp.roomSize = 0.55f + fxValue (THROW, "size") * 0.43f; vp.damping = 0.85f - fxValue (THROW, "tone") * 0.75f;
        vp.wetLevel = 1.0f; vp.dryLevel = 0.0f; vp.width = 1.0f; vp.freezeMode = 0.0f; verb.setParameters (vp);
    }
    const float eLpA = (float) std::exp (-juce::MathConstants<double>::twoPi * (1200.0 * std::pow (12.0, (double) eTone)) / sr), eHpA = (float) std::exp (-juce::MathConstants<double>::twoPi * 120.0 / sr);
    const float vHpA = (float) std::exp (-juce::MathConstants<double>::twoPi * 180.0 / sr);
    if (eTime <= 0) eTime = eTimeT;
    const float wow = value ("wow"), flut = value ("flut"), tone = value ("tone"), drive = value ("drive"), hiss = value ("hiss"), drop = value ("drop"), mix = value ("mix");
    const float outG = juce::Decibels::decibelsToGain (value ("out"));
    const double dt = 1.0 / sr;

    // pad: interfaccia + automazione
    for (int f = 0; f < kNumFx; ++f)
    {
        const bool down = uiPad[f].load() || padPtr[f]->load() > 0.5f;
        auto& X = F[f];
        if (down && ! X.prevParam)
        {
            const double at = q > 0 ? std::ceil (beat0 / q - 1e-6) * q : beat0;
            X.pendAt = T0 + (juce::int64) std::llround ((at - beat0) * bf); X.pendOn = true;
        }
        if (! down && X.prevParam && ! shotMode) { X.pendAt = -1; X.pad = false; }
        X.prevParam = down;
    }

    // catena del nastro: coefficienti per blocco
    {
        const float s = speedN > 0 ? speedAcc / speedN : 1.0f; speedAcc = 0; speedN = 0;
        const float sc = juce::jlimit (0.02f, 1.2f, s);
        const float base = 700.0f * std::pow (20000.0f / 700.0f, tone);   // tono: da 700 Hz (scurissimo) a aperto
        const float cutT = juce::jlimit (150.0f, 20000.0f, base * std::pow (juce::jmin (1.0f, sc), 1.6f * (0.3f + tone)));
        cutoffSm += (cutT - cutoffSm) * 0.35f;
        // drive: curva progressiva, fino a ~+30 dB di spinta sul nastro
        const float dg = 1.0f + drive * drive * 30.0f + drive * 4.0f + drive * 6.0f * (1.0f - juce::jmin (1.0f, sc));
        dgSm += (dg - dgSm) * 0.3f; outSm += (outG - outSm) * 0.3f;
        auto c = juce::dsp::IIR::Coefficients<float>::makePeakFilter (sr, 85.0f, 0.9f, juce::Decibels::decibelsToGain (drive * 2.5f));
        *bumpL.coefficients = *c; *bumpR.coefficients = *c;
        uiSpeed = s;
    }
    const float lpG = (float) std::tan (juce::MathConstants<double>::pi * juce::jmin ((double) cutoffSm, sr * 0.45) / sr), lpK = 1.0f / 0.6f;
    // compensazione di loudness: un segnale a -7.5 dB esce allo stesso livello qualunque sia il drive
    const float postG = 0.42f / std::tanh (0.42f * dgSm), bias = juce::jmin (0.35f, (dgSm - 1.0f) * 0.012f), biasC = std::tanh (bias);

    float pkL = 0, pkR = 0; int mask = 0;
    for (int i = 0; i < N; ++i)
    {
        const float inL = io[0][i], inR = io[1][i];
        hist.setSample (0, (int) (T & HM), inL); hist.setSample (1, (int) (T & HM), inR);
        const juce::int64 t = T;
        const double beat = beat0 + i / bf;

        // pad in attesa della quantizzazione
        for (int f = 0; f < kNumFx; ++f)
        {
            auto& X = F[f];
            if (X.pendAt >= 0 && t >= X.pendAt)
            {
                X.pendAt = -1;
                if (! shotMode) X.pad = true;
                else
                {
                    const double L = f == STOP ? stopLen + 1 : f == START ? startLen : f == SPIN ? spinLen + 1 : f == LOOP ? fxValue (LOOP, "size") : f == SWEEP ? fxValue (SWEEP, "len") : f == RISER ? fxValue (RISER, "len") : f == ECHO ? 1.0 : f == THROW ? 1.0 : 2.0;
                    X.shot = t + (juce::int64) (L * bf);
                }
            }
        }
        // coreografia
        if (seqOn)
        {
            const double b = std::fmod (beat, loopBeats); const int st = juce::jlimit (0, steps - 1, (int) ((b < 0 ? b + loopBeats : b) / 0.25));
            if (st != lastStep) { lastStep = st; uiStep = st; for (int f = 0; f < kNumFx; ++f) F[f].seqOn = seq[f][st].load(); }
        }
        else if (lastStep != -1) { lastStep = -1; uiStep = -1; for (auto& X : F) X.seqOn = false; }

        bool changed = false; mask = 0; bool anyMover = false;
        for (int f = 0; f < kNumFx; ++f)
        {
            auto& X = F[f]; const bool want = (X.pad || X.seqOn || t < X.shot) && onPtr[f]->load() > 0.5f;
            if (want && ! X.on) { X.on = true; activate (f, bf, beat, t); changed = true; }
            else if (! want && X.on) { X.on = false; changed = true; }
            if (X.on) { mask |= 1 << f; if (! isOverlay (f)) anyMover = true; }
        }
        // velocità
        const float hf = F[HALF].on ? halfSpeed : 1.0f, gk = halfGlide > 0 ? (float) juce::jmin (1.0, dt * 1000.0 / halfGlide * 3.0) : 1.0f;
        halfF += (hf - halfF) * gk;
        double v = halfF;
        if (F[STOP].on) v *= 1.0 - shape ((float) ((t - F[STOP].t0) / (stopLen * bf)), stopCurve);
        if (F[START].on) v *= shape ((float) ((t - F[START].t0) / (startLen * bf)), -startCurve);
        if (F[REV].on) v *= -revSpeed;
        if (F[SPIN].on) { const double e = (t - F[SPIN].t0) / (spinLen * bf), a = juce::jmin (1.0, e * 10); v = e < 1 ? -spinForce * a * std::pow (1 - e, 1.6) : 0.0; }
        const double slow = 1.0 + 2.5 * (1.0 - juce::jmin (1.0, std::abs (v)));
        ph1 += dt * 0.55; ph2 += dt * (6.3 + std::sin (ph1 * 3) * 0.6);
        if (rng.nextFloat() < dt * 3) dtgt = rng.nextFloat() * 2 - 1;
        drift += (dtgt - drift) * dt * 2;
        const double mod = 1.0 + (wow * 0.010 * std::sin (juce::MathConstants<double>::twoPi * ph1) + wow * 0.004 * drift + flut * 0.0035 * std::sin (juce::MathConstants<double>::twoPi * ph2)) * slow;
        const double liveDelay = 3.0 + (wow * 0.004 * sr) * (1.0 + std::sin (juce::MathConstants<double>::twoPi * ph1)) + wow * 0.002 * sr * (1.0 + drift) + flut * 0.00025 * sr * (1.0 + std::sin (juce::MathConstants<double>::twoPi * ph2));
        const bool tapeMod = wow > 0.001f || flut > 0.001f;

        if (changed && ! anyMover && ! live) { xf = { true, R, juce::jlimit (-2.0, 2.0, v), 0, juce::jmax (32, (int) (0.014 * sr)), false }; live = true; }
        if (anyMover && live) { R = (double) t - (tapeMod ? liveDelay : 3.0); live = false; }

        float l, r, amp = 1.0f; double vv = v;
        if (live)
        {
            if (tapeMod) { l = rd ((double) t - liveDelay, 0); r = rd ((double) t - liveDelay, 1); } else { l = inL; r = inR; }
            vv = 1.0;
        }
        else
        {
            amp = (float) juce::jmin (1.0, std::pow (std::abs (v), 0.35) * 1.05);
            if (F[STUT].on)
            {
                auto& D = F[STUT].d;   // 0 anchor, 1 inizio ripetizione, 2 lunghezza
                while ((double) t - D[1] >= D[2]) { D[1] += D[2]; F[STUT].k++; D[2] = juce::jmax (bf / 8, stutSize * bf * std::pow (1 - stutRoll * 0.22, (double) F[STUT].k)); }
                const double w = (double) t - D[1], prog = juce::jmin (1.0, ((double) t - F[STUT].t0) / (2 * bf));
                const double rate = std::pow (2.0, stutPitch * prog / 12.0) * halfF, p = D[0] + w * rate;
                const double open = D[2] * stutGate, fa = 0.003 * sr, fr2 = 0.006 * sr;
                const double att = w < fa ? 0.5 - 0.5 * std::cos (juce::MathConstants<double>::pi * w / fa) : 1.0;
                const double rel = w < open - fr2 ? 1.0 : (w >= open ? 0.0 : 0.5 + 0.5 * std::cos (juce::MathConstants<double>::pi * (w - (open - fr2)) / fr2));
                const float dec = (float) std::pow (1.0 - stutDecay * 0.18, (double) F[STUT].k);
                const float env = (float) (att * rel) * dec;
                l = rd (p, 0) * env; r = rd (p, 1) * env; amp = 1; vv = rate * (env > 0.5f ? 1 : 0.15);
                if (std::abs (stutFilt) > 0.01f)
                {
                    // sweep di filtro lungo le ripetizioni: + chiude il passa basso, - apre il passa alto
                    const float pr = (float) juce::jmin (1.0, prog * 1.3) * std::abs (stutFilt);
                    const float fc = stutFilt > 0 ? 18000.0f * std::pow (0.015f, pr) : 30.0f * std::pow (80.0f, pr);
                    const float a = (float) std::exp (-juce::MathConstants<double>::twoPi * fc / sr);
                    stLp[0] = (1 - a) * l + a * stLp[0]; stLp[1] = (1 - a) * r + a * stLp[1];
                    if (stutFilt > 0) { l = stLp[0]; r = stLp[1]; } else { l -= stLp[0]; r -= stLp[1]; }
                }
            }
            else if (F[GLITCH].on)
            {
                auto& X = F[GLITCH]; auto& G = X.d;   // 0 prossimo, 1 inizio, 2 lunghezza, 3 sorgente
                if ((double) t >= G[0])
                {
                    const double sz = gSize * bf * (rng.nextFloat() < gChaos * 0.5f ? (rng.nextBool() ? 0.5 : 2.0) : 1.0);
                    G[1] = (double) t; G[2] = sz; G[0] = (double) t + sz;
                    if (rng.nextFloat() > gDens) { X.op = -1; G[3] = R; }
                    else
                    {
                        const int nOps = 3 + juce::roundToInt (gChaos * 4); const int op = rng.nextInt (nOps); // 0,1 ripeti 2 indietro 3 su 4 giu 5 muto 6 salta
                        if (! (op <= 1 && X.op >= 0)) G[3] = R;
                        if (op == 6) G[3] = R - sz * (1 + rng.nextInt (3));
                        X.op = op;
                    }
                }
                const double w = (double) t - G[1];
                const double rate = X.op == 3 ? 2.0 : X.op == 4 ? 0.5 : X.op == 2 ? -1.0 : 1.0;
                const double p = G[3] + w * rate * juce::jmax (0.25, std::abs (v));
                const float fd = (float) juce::jmin (1.0, juce::jmin (w / (0.002 * sr), (G[2] - w) / (0.003 * sr))), m = X.op == 5 ? 0.0f : 1.0f;
                l = rd (p, 0) * fd * m; r = rd (p, 1) * fd * m; amp = 1; vv = rate * m;
            }
            else { l = rd (R, 0); r = rd (R, 1); }
            R += v * mod;
            if (F[LOOP].on)
            {
                const double zs = F[LOOP].d[0], z = F[LOOP].d[1];
                if (R >= zs + z) { xf = { true, R, v, 0, juce::jmax (48, (int) ((0.002 + loopXf * 0.03) * sr)), false }; R -= z; }
                else if (R < zs) R += z;
            }
            R = juce::jlimit ((double) (t - HL + 16), (double) t - 2.0, R);
        }
        if (xf.on)
        {
            const float g = (float) xf.n / xf.N, a = std::sin (g * juce::MathConstants<float>::halfPi), b = std::cos (g * juce::MathConstants<float>::halfPi);
            l = l * a + rd (xf.from, 0) * b; r = r * a + rd (xf.from, 1) * b;
            xf.from += xf.spd; if (++xf.n >= xf.N) xf.on = false;
        }
        // gate
        if (F[GATE].on) { const double ph = std::fmod (((double) t - F[GATE].t0) / (gateRate * bf), 1.0); const float tg = ph < gateDuty ? 1.0f : 1.0f - gateDepth; gateG += (tg - gateG) * 0.025f; if (gateG < 0.5f) vv *= 0.2; }
        else gateG += (1 - gateG) * 0.025f;
        // dropout
        if (drop > 0) { if (dropT <= 0 && rng.nextFloat() < dt * drop * 1.2) dropT = (int) ((0.03 + rng.nextFloat() * 0.12) * sr); if (dropT > 0) { --dropT; dropG += (0.15f - dropG) * 0.01f; } else dropG += (1 - dropG) * 0.004f; }
        else dropG = 1;
        // volume per effetto (prodotto dei volumi degli effetti attivi, smussato)
        // mix per effetto: quanto del segnale trattato rispetto all'originale (media degli effetti di movimento attivi)
        { float vt = 1.0f, mt = 0.0f; int nm = 0; for (int f = 0; f < kNumFx; ++f) if (F[f].on && f != CRUSH && f < SWEEP) { vt *= fxVol[f]; if (f != GATE) { mt += fxMix[f]; ++nm; } }
          volSm += (vt - volSm) * 0.002f; mixSm += ((nm > 0 ? mt / nm : 1.0f) - mixSm) * 0.003f; }
        float wL = l * amp * volSm, wR = r * amp * volSm;
        if (mixSm < 0.999f) { wL = wL * mixSm + inL * (1 - mixSm); wR = wR * mixSm + inR * (1 - mixSm); }
        const float gg = 1.0f - fxMix[GATE] * (1.0f - (float) gateG);
        wL *= dropG * gg; wR *= dropG * gg;
        // crush dello stacco: entra e esce con una dissolvenza breve
        crFade += ((F[CRUSH].on ? 1.0f : 0.0f) - crFade) * 0.004f;
        if (crFade > 0.0005f)
        {
            if (crCnt <= 0) { crHold[0] = wL; crHold[1] = wR; crCnt = crRate; } --crCnt;
            float cv[2];
            for (int c = 0; c < 2; ++c)
            {
                float y = crushBits (crushShape (crHold[c], crDrive, crChar), crBits);
                crLp[c] = (1 - crA) * y + crA * crLp[c]; if (crTone < 0.99f) y = crLp[c];
                cv[c] = y * fxVol[CRUSH];
            }
            const float k = crFade * fxMix[CRUSH];
            wL += (cv[0] - wL) * k; wR += (cv[1] - wR) * k;
        }
        // ---- modulazioni sul nastro (entrano e escono con una dissolvenza breve, le code restano pulite)
        {
            const double twoPi = juce::MathConstants<double>::twoPi;
            auto fadeTo = [&] (int k, int fx) { modFade[k] += ((F[fx].on ? 1.0f : 0.0f) - modFade[k]) * 0.004f; return modFade[k] > 0.0005f; };
            auto blend = [&] (int fx, int k, float yl, float yr) { const float m = modFade[k] * fxMix[fx]; wL += (yl * fxVol[fx] - wL) * m; wR += (yr * fxVol[fx] - wR) * m; };
            if (fadeTo (0, TREM))
            {   // tremolo a tempo; STEREO sfasa i lati fino a rimbalzare
                const double ph = ((double) t - F[TREM].t0) / (trRate * bf);
                auto lfo = [&] (double p) { const float s = (float) std::cos (twoPi * p); const float k = 1 + trShape * 12; return 0.5f + 0.5f * std::tanh (s * k) / std::tanh (k); };
                const float gl = 1.0f - trDepth * (1.0f - lfo (ph)), gr = 1.0f - trDepth * (1.0f - lfo (ph + 0.5 * trStereo));
                blend (TREM, 0, wL * gl, wR * gr);
            }
            if (fadeTo (1, ROTARY))
            {   // tromba (acuti) e tamburo (bassi) che girano con inerzia; doppler sulla tromba
                const float tgtH = 0.8f + rtSpeed * 5.9f;
                rotSp += (tgtH - rotSp) * (float) (dt * (rtSpeed > 0.5f ? 1.2 : 0.7));
                rotH += rotSp * dt; rotD += rotSp * 0.86 * dt; rotH -= std::floor (rotH); rotD -= std::floor (rotD);
                const float m = 0.5f * (wL + wR), d = rtDrive > 0.01f ? std::tanh (m * (1 + rtDrive * 6)) / (1 + rtDrive * 2) : m;
                rotXo[0] += (d - rotXo[0]) * (float) (1.0 - std::exp (-twoPi * 800.0 / sr)); const float lo = rotXo[0], hi = d - lo;
                const int RL = rotBuf.getNumSamples(); float* rb = rotBuf.getWritePointer (0); rb[rotW] = hi;
                const float sH = (float) std::sin (twoPi * rotH), cH = (float) std::cos (twoPi * rotH), sD = (float) std::sin (twoPi * rotD);
                auto tap = [&] (float delMs) { double rp = rotW - delMs * 0.001 * sr; while (rp < 0) rp += RL; const int i0 = (int) rp; const float fr = (float) (rp - i0); return rb[i0] + (rb[(i0 + 1) % RL] - rb[i0]) * fr; };
                const float hL = tap (1.2f + 0.35f * rtDepth * (1 + sH)) * (1 - 0.35f * rtDepth * (0.5f + 0.5f * cH)), hR = tap (1.2f + 0.35f * rtDepth * (1 - sH)) * (1 - 0.35f * rtDepth * (0.5f - 0.5f * cH));
                const float dL = lo * (1 - 0.25f * rtDepth * (0.5f + 0.5f * sD)), dR = lo * (1 - 0.25f * rtDepth * (0.5f - 0.5f * sD));
                if (++rotW >= RL) rotW = 0;
                blend (ROTARY, 1, (hL + dL) * 0.7f, (hR + dR) * 0.7f);
            }
            else { rotSp = 0.8f; }
            if (fadeTo (2, AUTOFILT))
            {   // passa basso risonante che si muove a tempo
                const double ph = ((double) t - F[AUTOFILT].t0) / (afRate * bf);
                const float lf = 0.5f - 0.5f * (float) std::cos (twoPi * ph);
                const float fc = juce::jlimit (60.0f, (float) sr * 0.42f, (float) (150.0 * std::pow (40.0, (double) afFreq)) * std::pow (2.0f, (lf - 0.5f) * afDepth * 6.0f));
                const float gg = (float) std::tan (juce::MathConstants<double>::pi * fc / sr), kk = 2.0f - 1.9f * afReso;
                float y[2] = { wL, wR };
                for (int k = 0; k < 2; ++k) { auto* ic = afIc[k]; const float v3 = y[k] - ic[1], v1 = (ic[0] + gg * v3) / (1 + gg * (gg + kk)), v2 = ic[1] + gg * v1; ic[0] = 2 * v1 - ic[0]; ic[1] = 2 * v2 - ic[1]; y[k] = v2 + v1 * afReso * 0.6f; }
                blend (AUTOFILT, 2, y[0], y[1]);
            }
            else { afIc[0][0] = afIc[0][1] = afIc[1][0] = afIc[1][1] = 0; }
            if (fadeTo (3, RINGMOD))
            {   // SCENDE: il tono cala lungo la zona (effetto "robot che si spegne")
                const double pr = juce::jmin (1.0, ((double) t - F[RINGMOD].t0) / (4.0 * bf));
                const double f = rmFreq * std::pow (0.25, pr * rmGlide);
                ringPh += f / sr; ringPh -= std::floor (ringPh);
                const float c = (float) std::sin (twoPi * ringPh);
                blend (RINGMOD, 3, wL * c * 1.4f, wR * c * 1.4f);
            }
            if (fadeTo (4, FSHIFT))
            {   // single sideband: x*cos + H(x)*sin, con feedback
                fsPh += fsHz / sr; fsPh -= std::floor (fsPh);
                const float co = (float) std::cos (twoPi * fsPh), si = (float) std::sin (twoPi * fsPh);
                float y[2] = { wL, wR };
                for (int k = 0; k < 2; ++k)
                {
                    float a = y[k] + fsFb[k] * fsFbAmt, b = a; for (auto& h : hA[k]) a = h.run (a); for (auto& h : hB[k]) b = h.run (b);
                    const float re = a, im = hBd[k]; hBd[k] = b;
                    const float o = re * co - im * (k ? -si : si) * 1.0f;
                    fsFb[k] = std::tanh (o); y[k] = o;
                }
                blend (FSHIFT, 4, y[0], y[1]);
            }
            if (fadeTo (5, DIST))
            {   // ampli: enfasi sui medi prima, curva per tipo, tono dopo; uscita compensata
                const float g = 1.0f + dsDrive * dsDrive * 90.0f + dsDrive * 6.0f;
                const float aHp = (float) std::exp (-twoPi * 180.0 / sr), aLp = (float) std::exp (-twoPi * (1200.0 * std::pow (10.0, (double) dsTone)) / sr);
                float y[2] = { wL, wR };
                for (int k = 0; k < 2; ++k)
                {
                    dsHp[k] = (1 - aHp) * y[k] + aHp * dsHp[k]; float x = (y[k] - dsHp[k]) * g;
                    float o;
                    if (dsType < 0.34f) o = std::tanh (x + 0.15f) - std::tanh (0.15f);                                      // crunch asimmetrico
                    else if (dsType < 0.67f) o = juce::jlimit (-1.0f, 1.0f, x * 1.3f) * 0.8f + std::tanh (x) * 0.2f;          // distorsione dura
                    else { const float r = x > 0 ? x : -0.6f * x; o = std::tanh (r * 1.5f) * (x > 0 ? 1.0f : 0.7f); }         // fuzz rettificato
                    dsLp[k] = (1 - aLp) * o + aLp * dsLp[k];
                    y[k] = dsLp[k] * 0.18f;
                }
                blend (DIST, 5, y[0], y[1]);
            }
        }
        // riser: sale di tono lungo la zona (pitch shifter a due teste), si apre e aggiunge rumore
        rFade += ((F[RISER].on ? 1.0f : 0.0f) - rFade) * (F[RISER].on ? 0.01f : 0.0015f);
        {
            float* rb0 = rBuf.getWritePointer (0); float* rb1 = rBuf.getWritePointer (1); const int RL = rBuf.getNumSamples();
            rb0[rW] = wL; rb1[rW] = wR;
            if (rFade > 0.0005f)
            {
                const float pr = F[RISER].on ? (float) juce::jmin (1.0, ((double) t - F[RISER].t0) / (rLen * bf)) : 1.0f;
                rSt += ((F[RISER].on ? shape (pr, rCurve) * rPitch : rSt) - rSt) * 0.02f;
                const double ratio = std::pow (2.0, rSt / 12.0), win = 0.06 * sr;
                rPh += (1.0 - ratio) / win; rPh -= std::floor (rPh);
                float ps[2] = {};
                for (int k = 0; k < 2; ++k)
                {
                    const double ph = std::fmod (rPh + k * 0.5, 1.0), d = 4.0 + ph * win;
                    const float w = (float) std::sin (juce::MathConstants<double>::pi * ph); const float w2 = w * w;
                    double rp = rW - d; while (rp < 0) rp += RL; const int i0 = (int) rp; const float fr = (float) (rp - i0); const int i1 = (i0 + 1) % RL;
                    ps[0] += (rb0[i0] + (rb0[i1] - rb0[i0]) * fr) * w2; ps[1] += (rb1[i0] + (rb1[i1] - rb1[i0]) * fr) * w2;
                }
                // apertura: passa alto che sale + rumore che cresce
                const float hfc = 20.0f * std::pow (60.0f, pr * rOpen), ha = (float) std::exp (-juce::MathConstants<double>::twoPi * hfc / sr);
                rNoiseLp = rNoiseLp * 0.5f + (rng.nextFloat() * 2 - 1) * 0.5f;
                const float nz = rNoise * pr * pr * 0.12f * rNoiseLp;
                for (int k = 0; k < 2; ++k) { rHp[k] = (1 - ha) * ps[k] + ha * rHp[k]; ps[k] = (ps[k] - rHp[k]) * (1.0f + pr * 0.25f) + nz; }
                const float k2 = rFade * fxMix[RISER], g2 = fxVol[RISER];
                wL += (ps[0] * g2 - wL) * k2; wR += (ps[1] * g2 - wR) * k2;
            }
            else { rSt = 0; rHp[0] = rHp[1] = 0; }
            if (++rW >= RL) rW = 0;
        }
        // sweep: passa basso che si chiude (direzione +) o passa alto che si apre (-)
        swFade += ((F[SWEEP].on ? 1.0f : 0.0f) - swFade) * 0.003f;
        if (swFade > 0.0005f)
        {
            if (F[SWEEP].on) swProg = juce::jmin (1.0, ((double) t - F[SWEEP].t0) / (swLen * bf));
            const float pr = (float) swProg * swDepth;
            const bool lp = swDir >= 0;
            const float fcT = lp ? 20000.0f * std::pow (0.006f, pr * (0.4f + 0.6f * std::abs (swDir))) : 20.0f * std::pow (220.0f, pr * (0.4f + 0.6f * std::abs (swDir)));
            swCut += (fcT - swCut) * 0.01f;
            const float gg2 = (float) std::tan (juce::MathConstants<double>::pi * juce::jmin ((double) swCut, sr * 0.45) / sr), kk = 2.0f - 1.85f * swRes;
            float sv[2] = { wL, wR };
            for (int k = 0; k < 2; ++k)
            {
                auto* ic = swIc[k]; const float v3 = sv[k] - ic[1], v1 = (ic[0] + gg2 * v3) / (1 + gg2 * (gg2 + kk)), v2 = ic[1] + gg2 * v1;
                ic[0] = 2 * v1 - ic[0]; ic[1] = 2 * v2 - ic[1];
                const float y = lp ? v2 : sv[k] - kk * v1 - v2;
                sv[k] = y * fxVol[SWEEP];
            }
            const float k2 = swFade * fxMix[SWEEP];
            wL += (sv[0] - wL) * k2; wR += (sv[1] - wR) * k2;
        }
        else { swProg = 0; swCut = swDir >= 0 ? 20000.0f : 20.0f; swIc[0][0] = swIc[0][1] = swIc[1][0] = swIc[1][1] = 0; }
        // eco sull'ultimo colpo: manda in un delay a tempo; la coda resta dopo la zona
        {
            eSend += ((F[ECHO].on ? 1.0f : 0.0f) - eSend) * 0.004f;
            eCut += ((F[ECHO].on ? eCutP : 0.0f) - eCut) * 0.004f;
            eTime += (eTimeT - eTime) * 0.0005f;
            const int EL = eBuf.getNumSamples(); float* e0 = eBuf.getWritePointer (0); float* e1 = eBuf.getWritePointer (1);
            double rp = eW - juce::jlimit (16.0f, (float) EL - 8, eTime); while (rp < 0) rp += EL;
            const int i0 = (int) rp; const float fr = (float) (rp - i0); const int i1 = (i0 + 1) % EL;
            const float d0 = e0[i0] + (e0[i1] - e0[i0]) * fr, d1 = e1[i0] + (e1[i1] - e1[i0]) * fr;
            float fbv[2] = { d0, d1 };
            for (int k = 0; k < 2; ++k) { eLp[k] = (1 - eLpA) * fbv[k] + eLpA * eLp[k]; eHp[k] = (1 - eHpA) * eLp[k] + eHpA * eHp[k]; fbv[k] = std::tanh ((eLp[k] - eHp[k]) * 1.1f) / 1.1f; }
            e0[eW] = wL * eSend + fbv[0] * eFb; e1[eW] = wR * eSend + fbv[1] * eFb;
            if (++eW >= EL) eW = 0;
            const float ret = fxMix[ECHO] * fxVol[ECHO];
            wL = wL * (1.0f - eCut) + d0 * ret; wR = wR * (1.0f - eCut) + d1 * ret;
        }
        // lancio di riverbero
        {
            vSend += ((F[THROW].on ? 1.0f : 0.0f) - vSend) * 0.004f;
            vCut += ((F[THROW].on ? vCutP : 0.0f) - vCut) * 0.004f;
            const int VL = vPre.getNumSamples(); float* p0 = vPre.getWritePointer (0); float* p1 = vPre.getWritePointer (1);
            float sL = wL * vSend, sR = wR * vSend;
            vHp[0] = (1 - vHpA) * sL + vHpA * vHp[0]; vHp[1] = (1 - vHpA) * sR + vHpA * vHp[1];
            p0[vW] = sL - vHp[0]; p1[vW] = sR - vHp[1];
            int rp = vW - (int) vPreN; while (rp < 0) rp += VL;
            float vl = p0[rp], vr = p1[rp];
            if (++vW >= VL) vW = 0;
            if (vSend > 1e-5f || vIdle < (int) sr * 12) { verb.processStereo (&vl, &vr, 1); vIdle = vSend > 1e-5f ? 0 : vIdle + 1; } else { vl = vr = 0; }
            const float ret = fxMix[THROW] * fxVol[THROW] * 1.0f;
            wL = wL * (1.0f - vCut) + vl * ret; wR = wR * (1.0f - vCut) + vr * ret;
        }
        float oL = wL * mix + inL * (1 - mix), oR = wR * mix + inR * (1 - mix);
        // nastro: bump, saturazione (asimmetrica quando spinta), passa basso, uscita
        oL = bumpL.processSample (oL); oR = bumpR.processSample (oR);
        if (drive > 0.001f || dgSm > 1.001f) { oL = (std::tanh (oL * dgSm + bias) - biasC) * postG; oR = (std::tanh (oR * dgSm + bias) - biasC) * postG; }   // drive a zero: pulito
        { hissLp = hissLp * 0.6f + (rng.nextFloat() * 2 - 1) * 0.4f; const float hs = hiss * 0.0035f * hissLp; oL += hs; oR += hs * 0.9f; }
        auto svf = [lpG, lpK] (float x, float* ic) { const float v3 = x - ic[1], v1 = (ic[0] + lpG * v3) / (1 + lpG * (lpG + lpK)), v2 = ic[1] + lpG * v1; ic[0] = 2 * v1 - ic[0]; ic[1] = 2 * v2 - ic[1]; return v2; };
        if (cutoffSm < 19500.0f) { oL = svf (oL, lpIcL); oR = svf (oR, lpIcR); }
        oL *= outSm; oR *= outSm;
        io[0][i] = oL; io[1][i] = oR;
        pkL = juce::jmax (pkL, std::abs (oL)); pkR = juce::jmax (pkR, std::abs (oR));
        speedAcc += (float) std::abs (vv); ++speedN;
        // traccia per il display
        {
            double b = std::fmod (beat, loopBeats); if (b < 0) b += loopBeats;
            const int col = juce::jlimit (0, kCols - 1, (int) (b / loopBeats * kCols));
            if (col != lastCol) { lastCol = col; dispWet[col] = 0; dispDry[col] = 0; dispMask[col] = mask; dispSpd[col] = (float) vv; }
            dispWet[col] = juce::jmax (dispWet[col].load(), std::abs (oL)); dispDry[col] = juce::jmax (dispDry[col].load(), std::abs (inL));
            dispMask[col] = dispMask[col].load() | mask;
        }
        ++T;
    }
    freeBeat = beat0 + N / bf;
    activeMask = mask; uiBeat = freeBeat;
    meterL = juce::jmax (pkL, meterL.load() * 0.85f); meterR = juce::jmax (pkR, meterR.load() * 0.85f);
}

// ======================= preset =======================
struct Preset
{
    const char* name; const char* cat; int bars;
    std::vector<std::pair<int, std::vector<int>>> seq;
    std::vector<std::pair<const char*, float>> p;
    float age;
};
static std::vector<int> Rg (int a, int b, int n) { std::vector<int> o; for (int i = a; i <= b; ++i) for (int k = 0; k < n; ++k) o.push_back (i * n + k); return o; }
static std::vector<int> E8 (int a, int b) { return Rg (a, b, 2); }
static std::vector<int> Q4 (int a, int b) { return Rg (a, b, 4); }
static const std::vector<Preset>& presets()
{
    static const std::vector<Preset> P = {
        { "Init", "", 2, {}, {}, 0 },
        { "Stop prima del drop", "Trap", 2, { { STOP, E8 (14, 15) } }, { { "stop_len", 1 }, { "stop_curve", -0.5f } }, 0.1f },
        { "Spinback sulla 8", "Trap", 2, { { SPIN, E8 (15, 15) } }, { { "spin_len", 0.5f }, { "spin_force", 3.5f } }, 0 },
        { "Roll in salita", "Trap", 2, { { STUT, E8 (12, 15) } }, { { "stut_size", 0.5f }, { "stut_roll", 0.85f }, { "stut_pitch", 5 }, { "stut_gate", 0.85f } }, 0 },
        { "Battuta a meta", "Drill", 2, { { HALF, E8 (8, 15) }, { GATE, E8 (14, 15) } }, { { "half_div", 2.00f }, { "half_glide", 90 }, { "gate_rate", 1.0f / 6 }, { "gate_depth", 0.7f }, { "gate_duty", 0.55f } }, 0 },
        { "Freno e ripartenza", "Drill", 2, { { STOP, E8 (6, 7) }, { START, E8 (8, 8) } }, { { "stop_len", 1 }, { "stop_curve", 0.25f }, { "start_len", 1 }, { "start_curve", 0.4f } }, 0 },
        { "Zona di un quarto", "Hook", 2, { { LOOP, E8 (12, 15) } }, { { "loop_size", 1 }, { "loop_xf", 0.3f } }, 0 },
        { "Zona di una battuta", "Breakdown", 4, { { LOOP, Q4 (8, 15) }, { GATE, Q4 (12, 15) } }, { { "loop_size", 4 }, { "loop_xf", 0.4f }, { "gate_rate", 0.25f }, { "gate_depth", 0.6f }, { "gate_duty", 0.6f } }, 0.25f },
        { "Glitch trap", "Glitch", 1, { { GLITCH, { 4, 5, 12, 13 } }, { STUT, { 7 } }, { REV, { 11 } }, { GATE, { 8, 9, 10 } } }, { { "glitch_dens", 0.7f }, { "glitch_chaos", 0.6f }, { "glitch_size", 0.25f }, { "stut_size", 0.25f }, { "stut_roll", 0.3f }, { "stut_pitch", -3 }, { "stut_gate", 0.7f } }, 0 },
        { "Freno da vinile", "Boom bap", 2, { { STOP, E8 (15, 15) }, { START, E8 (0, 0) } }, { { "stop_len", 0.5f }, { "stop_curve", 0.3f }, { "start_len", 0.25f }, { "start_curve", 0.5f } }, 0.55f },
        { "Respiro", "Lo-fi", 2, { { HALF, E8 (12, 13) } }, { { "half_div", 1.28f }, { "half_glide", 260 } }, 0.65f },
        { "Nastro che muore", "Outro", 4, { { STOP, Q4 (8, 15) } }, { { "stop_len", 8 }, { "stop_curve", -0.6f } }, 0.8f },
        { "Frena giu", "Stutter", 2, { { STUT, E8 (10, 15) } }, { { "stut_size", 0.25f }, { "stut_roll", 0.2f }, { "stut_pitch", -12 }, { "stut_gate", 0.75f } }, 0 },
        { "Fine frase al contrario", "Reverse", 2, { { REV, E8 (14, 15) } }, {}, 0 },
        { "Otto battute: costruzione", "Arrangiamento", 8, { { GATE, Rg (48, 55, 2) }, { STUT, Rg (56, 61, 2) }, { STOP, Rg (62, 63, 2) }, { HALF, Rg (24, 31, 2) }, { LOOP, Rg (40, 47, 2) } },
          { { "stut_size", 0.25f }, { "stut_roll", 0.6f }, { "stut_pitch", 4 }, { "loop_size", 2 }, { "stop_len", 1 }, { "stop_curve", -0.4f } }, 0 },
        { "Roll con filtro che apre", "Trap", 2, { { STUT, E8 (12, 15) } }, { { "stut_size", 0.5f }, { "stut_roll", 0.9f }, { "stut_pitch", 7 }, { "stut_gate", 0.85f }, { "stut_filt", -0.7f }, { "stut_vol", -2 } }, 0 },
        { "Stutter che sfuma", "Stutter", 2, { { STUT, E8 (12, 15) } }, { { "stut_size", 0.25f }, { "stut_roll", 0.2f }, { "stut_decay", 0.7f }, { "stut_filt", 0.6f }, { "stut_gate", 0.8f } }, 0 },
        { "Stutter 1/32 secco", "Stutter", 1, { { STUT, { 14, 15 } } }, { { "stut_size", 0.125f }, { "stut_gate", 0.6f }, { "stut_pitch", 0 } }, 0 },
        { "Gate terzinato", "Drill", 2, { { GATE, E8 (4, 7) }, { GATE, E8 (12, 15) } }, { { "gate_rate", 1.0f / 6 }, { "gate_depth", 0.85f }, { "gate_duty", 0.45f } }, 0 },
        { "Slide 808", "Drill", 2, { { HALF, E8 (6, 7) }, { HALF, E8 (14, 15) } }, { { "half_div", 1.43f }, { "half_glide", 150 } }, 0 },
        { "Cassetta stanca", "Lo-fi", 2, { { HALF, E8 (14, 15) } }, { { "half_div", 1.22f }, { "half_glide", 320 }, { "half_vol", -1 } }, 0.5f },
        { "Freno lento", "Lo-fi", 4, { { STOP, Q4 (14, 15) }, { START, { 0, 1 } } }, { { "stop_len", 2 }, { "stop_curve", 0.4f }, { "start_len", 0.5f } }, 0.4f },
        { "Chop della frase", "Boom bap", 2, { { LOOP, E8 (6, 7) }, { REV, E8 (15, 15) } }, { { "loop_size", 0.5f }, { "loop_xf", 0.4f } }, 0.3f },
        { "Vinile al contrario", "Boom bap", 4, { { REV, Q4 (15, 15) } }, { { "rev_speed", 0.9f }, { "rev_vol", -2 } }, 0.45f },
        { "Phonk spin", "Phonk", 2, { { SPIN, E8 (15, 15) }, { GATE, E8 (12, 13) } }, { { "spin_len", 0.5f }, { "spin_force", 4.5f }, { "gate_rate", 0.125f }, { "gate_depth", 0.7f } }, 0.2f },
        { "Glitch caos", "Glitch", 2, { { GLITCH, { 4, 5, 6, 7, 20, 21, 22, 23 } }, { STUT, { 14, 15 } }, { REV, { 26, 27 } }, { STOP, { 30, 31 } } },
          { { "glitch_dens", 0.85f }, { "glitch_chaos", 0.85f }, { "glitch_size", 0.125f }, { "stut_size", 0.125f }, { "stop_len", 0.5f } }, 0 },
        { "Build-up 4 battute", "Arrangiamento", 4, { { GATE, Q4 (8, 11) }, { STUT, Q4 (12, 14) }, { STOP, Q4 (15, 15) } },
          { { "gate_rate", 0.5f }, { "gate_depth", 0.7f }, { "stut_size", 1 }, { "stut_roll", 0.9f }, { "stut_pitch", 7 }, { "stut_filt", -0.6f }, { "stop_len", 1 }, { "stop_curve", -0.6f } }, 0 },
        { "Nastro saturo", "Colore", 2, {}, { { "drive", 0.85f }, { "tone", 0.78f }, { "out", -2 } }, 0 },
        { "Stacco distorto", "Crush", 2, { { CRUSH, E8 (14, 15) } }, { { "crush_drive", 0.7f }, { "crush_char", 0.45f }, { "crush_bits", 16 }, { "crush_vol", -2 } }, 0 },
        { "Bitcrush sul fill", "Crush", 2, { { CRUSH, E8 (12, 15) }, { STUT, E8 (14, 15) } }, { { "crush_drive", 0.35f }, { "crush_bits", 6 }, { "crush_rate", 6 }, { "stut_size", 0.25f }, { "stut_gate", 0.8f } }, 0 },
        { "Filtro che chiude", "Sweep", 4, { { SWEEP, Q4 (12, 15) } }, { { "sweep_len", 4 }, { "sweep_dir", 1 }, { "sweep_reso", 0.4f }, { "sweep_depth", 0.85f } }, 0 },
        { "Passa alto che sale", "Sweep", 4, { { SWEEP, Q4 (8, 15) } }, { { "sweep_len", 8 }, { "sweep_dir", -1 }, { "sweep_reso", 0.5f }, { "sweep_depth", 0.8f } }, 0 },
        { "Filtro a mezza battuta", "Sweep", 2, { { SWEEP, E8 (12, 15) } }, { { "sweep_len", 2 }, { "sweep_dir", 0.6f }, { "sweep_reso", 0.25f }, { "sweep_depth", 0.6f } }, 0 },
        { "Eco sull'ultimo colpo", "Eco", 2, { { ECHO, E8 (15, 15) } }, { { "echo_time", 0.5f }, { "echo_fb", 0.6f }, { "echo_cut", 1 }, { "echo_tone", 0.5f } }, 0 },
        { "Eco terzinato", "Eco", 2, { { ECHO, E8 (14, 15) } }, { { "echo_time", 1.0f / 3 }, { "echo_fb", 0.55f }, { "echo_cut", 1 }, { "echo_tone", 0.4f } }, 0 },
        { "Eco dub lungo", "Eco", 4, { { ECHO, Q4 (15, 15) } }, { { "echo_time", 1 }, { "echo_fb", 0.78f }, { "echo_cut", 1 }, { "echo_tone", 0.3f }, { "echo_mix", 0.9f } }, 0.2f },
        { "Lancio di riverbero", "Riverbero", 2, { { THROW, E8 (15, 15) } }, { { "throw_size", 0.8f }, { "throw_tone", 0.5f }, { "throw_cut", 0.7f }, { "throw_pre", 30 } }, 0 },
        { "Rullante in cattedrale", "Riverbero", 2, { { THROW, { 28 } } }, { { "throw_size", 1 }, { "throw_tone", 0.65f }, { "throw_cut", 0.0f }, { "throw_pre", 15 }, { "throw_mix", 0.9f } }, 0 },
        { "Riverbero che chiude", "Riverbero", 4, { { THROW, Q4 (15, 15) }, { SWEEP, Q4 (14, 15) } }, { { "throw_size", 0.9f }, { "throw_cut", 1 }, { "sweep_len", 2 }, { "sweep_dir", 1 } }, 0 },
        { "Riser 2 battute", "Build-up", 2, { { RISER, E8 (0, 15) } }, { { "riser_len", 8 }, { "riser_pitch", 12 }, { "riser_curve", 0.5f }, { "riser_open", 0.6f }, { "riser_noise", 0.3f } }, 0 },
        { "Riser veloce", "Build-up", 1, { { RISER, { 8, 9, 10, 11, 12, 13, 14, 15 } } }, { { "riser_len", 2 }, { "riser_pitch", 7 }, { "riser_curve", 0.2f }, { "riser_open", 0.4f } }, 0 },
        { "Build-up completo", "Build-up", 4, { { RISER, Q4 (8, 15) }, { SWEEP, Q4 (8, 15) }, { STUT, Q4 (14, 15) }, { ECHO, Q4 (15, 15) } },
          { { "riser_len", 8 }, { "riser_pitch", 12 }, { "riser_noise", 0.35f }, { "sweep_len", 8 }, { "sweep_dir", -1 }, { "sweep_depth", 0.7f }, { "stut_size", 0.25f }, { "stut_roll", 0.6f }, { "echo_time", 0.5f } }, 0 },
        { "Fuzz che sporca", "Crush", 1, { { CRUSH, { 8, 9, 10, 11, 12, 13, 14, 15 } } }, { { "crush_drive", 0.85f }, { "crush_char", 0.9f }, { "crush_tone", 0.6f }, { "crush_mix", 0.7f } }, 0 },
        { "Tremolo sul giro", "Modulazioni", 1, { { TREM, { 8, 9, 10, 11, 12, 13, 14, 15 } } }, { { "trem_rate", 2 }, { "trem_depth", 0.85f }, { "trem_stereo", 1 }, { "trem_shape", 0.6f } }, 0 },
        { "Leslie sul break", "Modulazioni", 2, { { ROTARY, { 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31 } } }, { { "rota_speed", 1 }, { "rota_depth", 0.8f }, { "rota_drive", 0.3f } }, 0.1f },
        { "Wah a tempo", "Modulazioni", 1, { { AUTOFILT, { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 } } }, { { "afil_rate", 2 }, { "afil_freq", 0.4f }, { "afil_depth", 0.8f }, { "afil_reso", 0.7f } }, 0 },
        { "Robot che si spegne", "Modulazioni", 1, { { RINGMOD, { 12, 13, 14, 15 } } }, { { "ring_freq", 400 }, { "ring_glide", 1 }, { "ring_mix", 0.8f } }, 0 },
        { "Shift metallico", "Modulazioni", 1, { { FSHIFT, { 14, 15 } } }, { { "fshf_hz", 90 }, { "fshf_fb", 0.6f }, { "fshf_mix", 0.8f } }, 0 },
        { "Distorsione sullo stacco", "Distorsione", 1, { { DIST, { 12, 13, 14, 15 } } }, { { "dist_drive", 0.75f }, { "dist_type", 0.5f }, { "dist_tone", 0.55f } }, 0 },
        { "Fuzz da ampli", "Distorsione", 2, { { DIST, { 24, 25, 26, 27, 28, 29, 30, 31 } }, { STUT, { 30, 31 } } }, { { "dist_drive", 0.9f }, { "dist_type", 0.9f }, { "dist_tone", 0.4f }, { "stut_size", 2 } }, 0 } };
    return P;
}
int Processor::getNumPrograms() { return (int) presets().size(); }
void Processor::setCurrentProgram (int i) { if (i >= 0 && i < getNumPrograms()) loadPreset (i); }
const juce::String Processor::getProgramName (int i)
{
    const auto& p = presets()[(size_t) juce::jlimit (0, getNumPrograms() - 1, i)];
    return juce::String (p.cat).isEmpty() ? juce::String (p.name) : juce::String (p.cat) + " - " + p.name;
}

static void setParam (juce::AudioProcessorValueTreeState& a, const char* id, float v)
{
    auto* pr = a.getParameter (id); if (pr == nullptr) return;
    for (int i = 0; i < kNumParams; ++i)
        if (juce::String (id) == kParams[i].id && kParams[i].table)
        {
            const Step* t = table (kParams[i].table); const int n = kParams[i].table == 3 ? 5 : 6; int best = 0;
            for (int k = 0; k < n; ++k) if (std::abs (t[k].v - v) < std::abs (t[best].v - v)) best = k;
            v = (float) best; break;
        }
    pr->setValueNotifyingHost (pr->convertTo0to1 (v));
}

void Processor::applyAge (float a)
{
    // invecchiamento misurato: piu wow e tono scuro, poco fruscio e pochi dropout
    setParam (apvts, "wow", 0.1f + a * 0.55f); setParam (apvts, "flut", 0.05f + a * 0.4f); setParam (apvts, "tone", juce::jmax (0.45f, 0.88f - a * 0.35f));
    setParam (apvts, "drive", 0.2f + a * 0.3f); setParam (apvts, "hiss", a * a * 0.35f); setParam (apvts, "drop", a > 0.5f ? (a - 0.5f) * 0.3f : 0.0f);
}

void Processor::loadPreset (int i)
{
    presetIndex = juce::jlimit (0, getNumPrograms() - 1, i);
    const auto& P = presets()[(size_t) presetIndex];
    for (int k = 0; k < kNumParams; ++k) { auto* pr = apvts.getParameter (kParams[k].id); pr->setValueNotifyingHost (pr->getDefaultValue()); }
    for (auto id : { "age", "mix", "out", "wow", "flut", "tone", "drive", "hiss", "drop" }) { auto* pr = apvts.getParameter (id); pr->setValueNotifyingHost (pr->getDefaultValue()); }
    for (int f = 0; f < kNumFx; ++f) { auto* pr = apvts.getParameter (juce::String (kFx[f].id) + "_on"); pr->setValueNotifyingHost (1.0f); }
    for (auto& r : seq) for (auto& s : r) s = false;
    for (auto& [fx, st] : P.seq) for (int s : st) if (s < kMaxSteps) seq[fx][s] = true;
    for (auto& [id, v] : P.p) setParam (apvts, id, v);
    const int bi = P.bars == 1 ? 0 : P.bars == 2 ? 1 : P.bars == 4 ? 2 : 3;
    setParam (apvts, "bars", (float) bi);
    if (P.age > 0) { setParam (apvts, "age", P.age); applyAge (P.age); }
    setParam (apvts, "seqOn", P.seq.empty() ? 0.0f : 1.0f);
}


// ======================= coreografia generata =======================
// Regole: un solo effetto "di movimento" per volta (il gate puo sovrapporsi), i fill vanno a fine frase,
// stop seguito da start sul primo ottavo, eventi a meta frase solo con intensita sufficiente.
void Processor::generate()
{
    juce::Random r;
    const int style = juce::jlimit (0, 5, (int) value ("genStyle")); const float amt = value ("genAmt");
    const int bars = style == 5 ? 4 : (amt > 0.65f ? 4 : 2), S = bars * 16;
    for (int k = 0; k < kNumParams; ++k) { auto* pr = apvts.getParameter (kParams[k].id); pr->setValueNotifyingHost (pr->getDefaultValue()); }
    for (auto& row : seq) for (auto& st : row) st = false;
    std::vector<bool> busy ((size_t) S, false);
    auto put = [&] (int fx, int a, int len)
    {
        if (a < 0 || a + len > S) return false;
        const bool over = isOverlay (fx);
        if (! over) for (int k = a; k < a + len; ++k) if (busy[(size_t) k]) return false;
        for (int k = a; k < a + len; ++k) { seq[fx][k] = true; if (! over) busy[(size_t) k] = true; }
        return true;
    };
    auto P = [this] (const char* id, float v) { setParam (apvts, id, v); };
    auto pick = [&r] (std::initializer_list<int> w) { int tot = 0; for (int x : w) tot += x; int k = r.nextInt (juce::jmax (1, tot)), i = 0; for (int x : w) { if (k < x) return i; k -= x; ++i; } return 0; };
    const int end = S;                      // fine frase
    const int fillLen = amt < 0.5f ? 4 : 8;   // passi da 1/16
    const bool mid = amt > 0.3f, mid2 = amt > 0.6f;

    switch (style)
    {
        case 0: // TRAP
        {
            const int f = pick ({ 5, 3, 2 });
            if (f == 0) { put (STUT, end - fillLen, fillLen); P ("stut_size", 0.5f); P ("stut_roll", 0.75f + r.nextFloat() * 0.2f); P ("stut_pitch", r.nextBool() ? 5.0f : 7.0f); P ("stut_gate", 0.85f); P ("stut_decay", 0.15f); P ("stut_filt", -0.35f); }
            else if (f == 1) { put (STOP, end - 4, 4); P ("stop_len", 1); P ("stop_curve", -0.5f); }
            else { put (SPIN, end - 2, 2); P ("spin_len", 0.5f); P ("spin_force", 3.5f); }
            if (mid) { put (GATE, 12, 4); P ("gate_rate", 1.0f / 6); P ("gate_depth", 0.75f); P ("gate_duty", 0.55f); }
            if (mid2) { put (REV, 30, 2); }
            if (bars == 4 && mid2) { put (HALF, 40, 8); P ("half_div", 2.00f); P ("half_glide", 80); }
            if (mid2 && r.nextBool()) { put (CRUSH, end - 4, 4); P ("crush_drive", 0.6f); P ("crush_char", 0.4f); }
            else if (mid && r.nextBool()) { put (ECHO, end - 2, 2); P ("echo_time", 0.5f); P ("echo_fb", 0.55f); }
            break;
        }
        case 1: // DRILL
        {
            const int f = pick ({ 4, 4, 2 });
            if (f == 0) { put (STUT, end - fillLen, fillLen); P ("stut_size", 0.25f); P ("stut_roll", 0.3f); P ("stut_pitch", -5); P ("stut_gate", 0.75f); P ("stut_decay", 0.3f); P ("stut_filt", 0.4f); }
            else if (f == 1) { put (STOP, end - 4, 4); put (START, 0, 2); P ("stop_len", 1); P ("stop_curve", 0.25f); P ("start_len", 0.5f); P ("start_curve", 0.4f); }
            else { put (REV, end - 4, 4); P ("rev_speed", 1.0f); }
            if (mid) { put (HALF, 8, 8); P ("half_div", 2.00f); P ("half_glide", 90); }
            if (mid2) { put (GATE, bars == 4 ? 44 : 24, 4); P ("gate_rate", 1.0f / 6); P ("gate_depth", 0.8f); P ("gate_duty", 0.5f); }
            break;
        }
        case 2: // BOOM BAP
        {
            const int f = pick ({ 5, 3, 2 });
            if (f == 0) { put (STOP, end - 2, 2); put (START, 0, 1); P ("stop_len", 0.5f); P ("stop_curve", 0.3f); P ("start_len", 0.25f); P ("start_curve", 0.5f); }
            else if (f == 1) { put (REV, end - 4, 4); }
            else { put (LOOP, end - 4, 4); P ("loop_size", 0.5f); P ("loop_xf", 0.3f); }
            if (mid) { put (LOOP, 12, 4); P ("loop_size", 1); P ("loop_xf", 0.35f); }
            if (mid2 && bars == 4) { put (STUT, 30, 2); P ("stut_size", 0.25f); P ("stut_gate", 0.8f); }
            break;
        }
        case 3: // LO-FI
        {
            put (HALF, end - 8, 8); P ("half_div", 1.0f / (0.75f + r.nextFloat() * 0.08f)); P ("half_glide", 260); P ("half_vol", -1);
            if (mid) { put (STOP, 12, 4); put (START, 16, 2); P ("stop_len", 1); P ("stop_curve", -0.3f); P ("start_len", 0.5f); P ("start_curve", 0.2f); }
            if (mid2 && bars == 4) { put (REV, 28, 4); P ("rev_speed", 0.8f); }
            break;
        }
        case 4: // GLITCH
        {
            P ("glitch_dens", 0.5f + amt * 0.4f); P ("glitch_chaos", 0.3f + amt * 0.5f); P ("glitch_size", 0.25f);
            P ("stut_size", 0.125f); P ("stut_roll", 0.4f); P ("stut_pitch", -3); P ("stut_gate", 0.7f);
            P ("gate_rate", 0.25f); P ("gate_depth", 0.9f); P ("gate_duty", 0.4f);
            P ("crush_drive", 0.4f); P ("crush_bits", 5); P ("crush_rate", 8);
            put (STOP, end - 2, 2); P ("stop_len", 0.5f); P ("stop_curve", -0.4f);
            put (GLITCH, 4, 4);
            const int nEv = 1 + juce::roundToInt (amt * 4) * bars / 2;
            for (int e = 0, tries = 0; e < nEv && tries < 60; ++tries)
            {
                const int beatPos = r.nextInt (S / 4) * 4 + (r.nextBool() ? 0 : 2);   // ottavi, mai sul battere della battuta
                if (beatPos % 16 == 0) continue;
                const int fx = pick ({ 5, 3, 2, 2, 2 }); const int fxs[] = { GLITCH, STUT, REV, GATE, CRUSH }; const int len = fxs[fx] == GLITCH ? 4 : 2;
                if (put (fxs[fx], beatPos, len)) ++e;
            }
            break;
        }
        default: // BUILD-UP su 4 battute
        {
            put (GATE, 32, 16); P ("gate_rate", 0.5f); P ("gate_depth", 0.55f + amt * 0.3f); P ("gate_duty", 0.6f);
            put (STUT, 48, 12); P ("stut_size", 1); P ("stut_roll", 0.9f); P ("stut_pitch", 7); P ("stut_gate", 0.85f); P ("stut_decay", 0.0f); P ("stut_filt", -0.6f);
            put (STOP, 60, 4); P ("stop_len", 1); P ("stop_curve", -0.6f);
            put (CRUSH, 56, 8); P ("crush_drive", 0.3f + amt * 0.4f); P ("crush_bits", amt > 0.6f ? 8.0f : 16.0f);
            put (RISER, 32, 28); P ("riser_len", 8); P ("riser_pitch", amt > 0.5f ? 12.0f : 7.0f); P ("riser_noise", 0.2f + amt * 0.3f);
            put (SWEEP, 32, 32); P ("sweep_len", 8); P ("sweep_dir", -1); P ("sweep_depth", 0.5f + amt * 0.35f);
            if (mid2) { put (HALF, 24, 8); P ("half_div", 2.00f); P ("half_glide", 120); }
            break;
        }
    }
    setParam (apvts, "bars", bars == 2 ? 1.0f : 2.0f);
    setParam (apvts, "seqOn", 1.0f);
}

juce::String Processor::seqString() const
{
    juce::String s;
    for (int f = 0; f < kNumFx; ++f) { for (int k = 0; k < kMaxSteps; ++k) s << (seq[f][k].load() ? '1' : '0'); s << ';'; }
    return s;
}
void Processor::setSeqString (const juce::String& str)
{
    auto rows = juce::StringArray::fromTokens (str, ";", "");
    for (int f = 0; f < kNumFx; ++f) for (int k = 0; k < kMaxSteps; ++k) seq[f][k] = f < rows.size() && k < rows[f].length() && rows[f][k] == '1';
}
void Processor::getStateInformation (juce::MemoryBlock& dest)
{
    auto st = apvts.copyState();
    st.setProperty ("seq", seqString(), nullptr); st.setProperty ("preset", presetIndex, nullptr);
    if (auto x = st.createXml()) copyXmlToBinary (*x, dest);
}
void Processor::setStateInformation (const void* data, int size)
{
    auto x = getXmlFromBinary (data, size); if (x == nullptr) return;
    auto st = juce::ValueTree::fromXml (*x); if (! st.isValid()) return;
    apvts.replaceState (st);
    setSeqString (st.getProperty ("seq").toString());
    presetIndex = st.getProperty ("preset", 0);
}

juce::AudioProcessorEditor* Processor::createEditor() { return new Editor (*this); }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new jt::Processor(); }
