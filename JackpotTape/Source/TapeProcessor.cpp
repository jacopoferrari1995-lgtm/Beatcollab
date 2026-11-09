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
    { "gate",   "Gate",        "taglia il volume a tempo",                  0xfffacc15, '9' } };

const Step kDur[6]  = { { 0.25, "1/16" }, { 0.5, "1/8" }, { 1, "1/4" }, { 2, "1/2" }, { 4, "1 bar" }, { 8, "2 bar" } };
const Step kSize[6] = { { 0.125, "1/32" }, { 1.0 / 6, "1/16T" }, { 0.25, "1/16" }, { 1.0 / 3, "1/8T" }, { 0.5, "1/8" }, { 1, "1/4" } };
const Step kRate[5] = { { 0.5, "1/8" }, { 1.0 / 3, "1/8T" }, { 0.25, "1/16" }, { 1.0 / 6, "1/16T" }, { 0.125, "1/32" } };

const PSpec kParams[] = {
    { "stop_len", "Durata", STOP, 0, 5, 3, 1, 0 },        { "stop_curve", "Curva", STOP, -1, 1, -0.35f, 0, 1 },
    { "start_len", "Durata", START, 0, 5, 2, 1, 0 },      { "start_curve", "Curva", START, -1, 1, 0.3f, 0, 1 },
    { "spin_len", "Durata", SPIN, 0, 5, 2, 1, 0 },        { "spin_force", "Forza", SPIN, 1, 5, 3, 0, 5 },
    { "half_speed", "Velocita", HALF, 0.25f, 0.9f, 0.5f, 0, 2 }, { "half_glide", "Glide", HALF, 0, 400, 70, 0, 3 },
    { "rev_speed", "Velocita", REV, 0.5f, 1.5f, 1, 0, 2 },
    { "stut_size", "Misura", STUT, 0, 5, 2, 2, 0 },       { "stut_roll", "Roll", STUT, 0, 1, 0, 0, 0 },
    { "stut_pitch", "Pitch", STUT, -12, 12, 0, 0, 4 },    { "stut_gate", "Gate", STUT, 0.2f, 1, 0.9f, 0, 0 },
    { "loop_size", "Zona", LOOP, 0, 5, 4, 1, 0 },         { "loop_xf", "Morbido", LOOP, 0, 1, 0.3f, 0, 0 },
    { "glitch_dens", "Densita", GLITCH, 0, 1, 0.6f, 0, 0 }, { "glitch_chaos", "Caos", GLITCH, 0, 1, 0.5f, 0, 0 }, { "glitch_size", "Grana", GLITCH, 0, 5, 2, 2, 0 },
    { "gate_rate", "Rate", GATE, 0, 4, 2, 3, 0 },         { "gate_depth", "Prof.", GATE, 0, 1, 1, 0, 0 }, { "gate_duty", "Apertura", GATE, 0.1f, 0.9f, 0.5f, 0, 0 },
    { "stut_decay", "Decay", STUT, 0, 1, 0, 0, 0 },       { "stut_filt", "Filtro", STUT, -1, 1, 0, 0, 1 },
    { "stop_vol", "Volume", STOP, -18, 6, 0, 0, 6 },      { "start_vol", "Volume", START, -18, 6, 0, 0, 6 },   { "spin_vol", "Volume", SPIN, -18, 6, 0, 0, 6 },
    { "half_vol", "Volume", HALF, -18, 6, 0, 0, 6 },      { "rev_vol", "Volume", REV, -18, 6, 0, 0, 6 },       { "stut_vol", "Volume", STUT, -18, 6, 0, 0, 6 },
    { "loop_vol", "Volume", LOOP, -18, 6, 0, 0, 6 },      { "glitch_vol", "Volume", GLITCH, -18, 6, 0, 0, 6 }, { "gate_vol", "Volume", GATE, -18, 6, 0, 0, 6 } };
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
    f ("age", "Age", 0, 1, 0); f ("mix", "Mix", 0, 1, 1); f ("out", "Output", -24, 6, -0.9f);
    f ("wow", "Wow", 0, 1, 0.12f); f ("flut", "Flutter", 0, 1, 0.08f); f ("tone", "Tono", 0, 1, 0.7f);
    f ("drive", "Drive", 0, 1, 0.2f); f ("hiss", "Hiss", 0, 1, 0); f ("drop", "Dropout", 0, 1, 0);
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "mode", 1 }, "Modo pad", kModes, 0));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "quant", 1 }, "Quantizza", kQuant, 1));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "seqOn", 1 }, "Coreografia", false));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "bars", 1 }, "Lunghezza", kBars, 1));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "sync", 1 }, "BPM dall'host", true));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "genStyle", 1 }, "Stile coreografia", StringArray { "TRAP", "DRILL", "BOOM BAP", "LO-FI", "GLITCH", "BUILD-UP" }, 0));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "genAmt", 1 }, "Intensita coreografia", NormalisableRange<float> (0, 1), 0.5f));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "bpm", 1 }, "BPM manuale", NormalisableRange<float> (50, 220, 0.1f), 140));
    for (int i = 0; i < kNumFx; ++i) l.add (std::make_unique<AudioParameterBool> (ParameterID { "pad" + String (i + 1), 1 }, String ("Pad ") + kFx[i].name, false));
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
    const float spinLen = fxValue (SPIN, "len"), spinForce = fxValue (SPIN, "force"), halfSpeed = fxValue (HALF, "speed"), halfGlide = fxValue (HALF, "glide");
    const float revSpeed = fxValue (REV, "speed"), stutSize = fxValue (STUT, "size"), stutRoll = fxValue (STUT, "roll"), stutPitch = fxValue (STUT, "pitch"), stutGate = fxValue (STUT, "gate");
    const float loopXf = fxValue (LOOP, "xf"), gDens = fxValue (GLITCH, "dens"), gChaos = fxValue (GLITCH, "chaos"), gSize = fxValue (GLITCH, "size");
    const float gateRate = fxValue (GATE, "rate"), gateDepth = fxValue (GATE, "depth"), gateDuty = fxValue (GATE, "duty");
    const float stutDecay = fxValue (STUT, "decay"), stutFilt = fxValue (STUT, "filt");
    float fxVol[kNumFx]; for (int f = 0; f < kNumFx; ++f) fxVol[f] = juce::Decibels::decibelsToGain (fxValue (f, "vol"));
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
        const float base = 6000.0f * std::pow (20000.0f / 6000.0f, tone);
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
                    const double L = f == STOP ? stopLen + 1 : f == START ? startLen : f == SPIN ? spinLen + 1 : f == LOOP ? fxValue (LOOP, "size") : 2.0;
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
            auto& X = F[f]; const bool want = X.pad || X.seqOn || t < X.shot;
            if (want && ! X.on) { X.on = true; activate (f, bf, beat, t); changed = true; }
            else if (! want && X.on) { X.on = false; changed = true; }
            if (X.on) { mask |= 1 << f; if (f != GATE) anyMover = true; }
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
        { float vt = 1.0f; for (int f = 0; f < kNumFx; ++f) if (F[f].on) vt *= fxVol[f]; volSm += (vt - volSm) * 0.002f; }
        const float g = amp * dropG * (float) gateG * volSm;
        float oL = l * g * mix + inL * (1 - mix), oR = r * g * mix + inR * (1 - mix);
        // nastro: bump, saturazione (asimmetrica quando spinta), passa basso, uscita
        oL = bumpL.processSample (oL); oR = bumpR.processSample (oR);
        oL = (std::tanh (oL * dgSm + bias) - biasC) * postG; oR = (std::tanh (oR * dgSm + bias) - biasC) * postG;
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
        { "Battuta a meta", "Drill", 2, { { HALF, E8 (8, 15) }, { GATE, E8 (14, 15) } }, { { "half_speed", 0.5f }, { "half_glide", 90 }, { "gate_rate", 1.0f / 6 }, { "gate_depth", 0.7f }, { "gate_duty", 0.55f } }, 0 },
        { "Freno e ripartenza", "Drill", 2, { { STOP, E8 (6, 7) }, { START, E8 (8, 8) } }, { { "stop_len", 1 }, { "stop_curve", 0.25f }, { "start_len", 1 }, { "start_curve", 0.4f } }, 0 },
        { "Zona di un quarto", "Hook", 2, { { LOOP, E8 (12, 15) } }, { { "loop_size", 1 }, { "loop_xf", 0.3f } }, 0 },
        { "Zona di una battuta", "Breakdown", 4, { { LOOP, Q4 (8, 15) }, { GATE, Q4 (12, 15) } }, { { "loop_size", 4 }, { "loop_xf", 0.4f }, { "gate_rate", 0.25f }, { "gate_depth", 0.6f }, { "gate_duty", 0.6f } }, 0.25f },
        { "Glitch trap", "Glitch", 1, { { GLITCH, { 4, 5, 12, 13 } }, { STUT, { 7 } }, { REV, { 11 } }, { GATE, { 8, 9, 10 } } }, { { "glitch_dens", 0.7f }, { "glitch_chaos", 0.6f }, { "glitch_size", 0.25f }, { "stut_size", 0.25f }, { "stut_roll", 0.3f }, { "stut_pitch", -3 }, { "stut_gate", 0.7f } }, 0 },
        { "Freno da vinile", "Boom bap", 2, { { STOP, E8 (15, 15) }, { START, E8 (0, 0) } }, { { "stop_len", 0.5f }, { "stop_curve", 0.3f }, { "start_len", 0.25f }, { "start_curve", 0.5f } }, 0.55f },
        { "Respiro", "Lo-fi", 2, { { HALF, E8 (12, 13) } }, { { "half_speed", 0.78f }, { "half_glide", 260 } }, 0.65f },
        { "Nastro che muore", "Outro", 4, { { STOP, Q4 (8, 15) } }, { { "stop_len", 8 }, { "stop_curve", -0.6f } }, 0.8f },
        { "Frena giu", "Stutter", 2, { { STUT, E8 (10, 15) } }, { { "stut_size", 0.25f }, { "stut_roll", 0.2f }, { "stut_pitch", -12 }, { "stut_gate", 0.75f } }, 0 },
        { "Fine frase al contrario", "Reverse", 2, { { REV, E8 (14, 15) } }, {}, 0 },
        { "Otto battute: costruzione", "Arrangiamento", 8, { { GATE, Rg (48, 55, 2) }, { STUT, Rg (56, 61, 2) }, { STOP, Rg (62, 63, 2) }, { HALF, Rg (24, 31, 2) }, { LOOP, Rg (40, 47, 2) } },
          { { "stut_size", 0.25f }, { "stut_roll", 0.6f }, { "stut_pitch", 4 }, { "loop_size", 2 }, { "stop_len", 1 }, { "stop_curve", -0.4f } }, 0 },
        { "Roll con filtro che apre", "Trap", 2, { { STUT, E8 (12, 15) } }, { { "stut_size", 0.5f }, { "stut_roll", 0.9f }, { "stut_pitch", 7 }, { "stut_gate", 0.85f }, { "stut_filt", -0.7f }, { "stut_vol", -2 } }, 0 },
        { "Stutter che sfuma", "Stutter", 2, { { STUT, E8 (12, 15) } }, { { "stut_size", 0.25f }, { "stut_roll", 0.2f }, { "stut_decay", 0.7f }, { "stut_filt", 0.6f }, { "stut_gate", 0.8f } }, 0 },
        { "Stutter 1/32 secco", "Stutter", 1, { { STUT, { 14, 15 } } }, { { "stut_size", 0.125f }, { "stut_gate", 0.6f }, { "stut_pitch", 0 } }, 0 },
        { "Gate terzinato", "Drill", 2, { { GATE, E8 (4, 7) }, { GATE, E8 (12, 15) } }, { { "gate_rate", 1.0f / 6 }, { "gate_depth", 0.85f }, { "gate_duty", 0.45f } }, 0 },
        { "Slide 808", "Drill", 2, { { HALF, E8 (6, 7) }, { HALF, E8 (14, 15) } }, { { "half_speed", 0.7f }, { "half_glide", 150 } }, 0 },
        { "Cassetta stanca", "Lo-fi", 2, { { HALF, E8 (14, 15) } }, { { "half_speed", 0.82f }, { "half_glide", 320 }, { "half_vol", -1 } }, 0.5f },
        { "Freno lento", "Lo-fi", 4, { { STOP, Q4 (14, 15) }, { START, { 0, 1 } } }, { { "stop_len", 2 }, { "stop_curve", 0.4f }, { "start_len", 0.5f } }, 0.4f },
        { "Chop della frase", "Boom bap", 2, { { LOOP, E8 (6, 7) }, { REV, E8 (15, 15) } }, { { "loop_size", 0.5f }, { "loop_xf", 0.4f } }, 0.3f },
        { "Vinile al contrario", "Boom bap", 4, { { REV, Q4 (15, 15) } }, { { "rev_speed", 0.9f }, { "rev_vol", -2 } }, 0.45f },
        { "Phonk spin", "Phonk", 2, { { SPIN, E8 (15, 15) }, { GATE, E8 (12, 13) } }, { { "spin_len", 0.5f }, { "spin_force", 4.5f }, { "gate_rate", 0.125f }, { "gate_depth", 0.7f } }, 0.2f },
        { "Glitch caos", "Glitch", 2, { { GLITCH, { 4, 5, 6, 7, 20, 21, 22, 23 } }, { STUT, { 14, 15 } }, { REV, { 26, 27 } }, { STOP, { 30, 31 } } },
          { { "glitch_dens", 0.85f }, { "glitch_chaos", 0.85f }, { "glitch_size", 0.125f }, { "stut_size", 0.125f }, { "stop_len", 0.5f } }, 0 },
        { "Build-up 4 battute", "Arrangiamento", 4, { { GATE, Q4 (8, 11) }, { STUT, Q4 (12, 14) }, { STOP, Q4 (15, 15) } },
          { { "gate_rate", 0.5f }, { "gate_depth", 0.7f }, { "stut_size", 1 }, { "stut_roll", 0.9f }, { "stut_pitch", 7 }, { "stut_filt", -0.6f }, { "stop_len", 1 }, { "stop_curve", -0.6f } }, 0 },
        { "Nastro saturo", "Colore", 2, {}, { { "drive", 0.85f }, { "tone", 0.55f }, { "out", -2 } }, 0 } };
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
    setParam (apvts, "wow", 0.1f + a * 0.55f); setParam (apvts, "flut", 0.05f + a * 0.4f); setParam (apvts, "tone", juce::jmax (0.15f, 0.7f - a * 0.4f));
    setParam (apvts, "drive", 0.2f + a * 0.3f); setParam (apvts, "hiss", a * a * 0.35f); setParam (apvts, "drop", a > 0.5f ? (a - 0.5f) * 0.3f : 0.0f);
}

void Processor::loadPreset (int i)
{
    presetIndex = juce::jlimit (0, getNumPrograms() - 1, i);
    const auto& P = presets()[(size_t) presetIndex];
    for (int k = 0; k < kNumParams; ++k) { auto* pr = apvts.getParameter (kParams[k].id); pr->setValueNotifyingHost (pr->getDefaultValue()); }
    for (auto id : { "age", "mix", "out", "wow", "flut", "tone", "drive", "hiss", "drop" }) { auto* pr = apvts.getParameter (id); pr->setValueNotifyingHost (pr->getDefaultValue()); }
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
        if (fx != GATE) for (int k = a; k < a + len; ++k) if (busy[(size_t) k]) return false;
        for (int k = a; k < a + len; ++k) { seq[fx][k] = true; if (fx != GATE) busy[(size_t) k] = true; }
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
            if (bars == 4 && mid2) { put (HALF, 40, 8); P ("half_speed", 0.5f); P ("half_glide", 80); }
            break;
        }
        case 1: // DRILL
        {
            const int f = pick ({ 4, 4, 2 });
            if (f == 0) { put (STUT, end - fillLen, fillLen); P ("stut_size", 0.25f); P ("stut_roll", 0.3f); P ("stut_pitch", -5); P ("stut_gate", 0.75f); P ("stut_decay", 0.3f); P ("stut_filt", 0.4f); }
            else if (f == 1) { put (STOP, end - 4, 4); put (START, 0, 2); P ("stop_len", 1); P ("stop_curve", 0.25f); P ("start_len", 0.5f); P ("start_curve", 0.4f); }
            else { put (REV, end - 4, 4); P ("rev_speed", 1.0f); }
            if (mid) { put (HALF, 8, 8); P ("half_speed", 0.5f); P ("half_glide", 90); }
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
            put (HALF, end - 8, 8); P ("half_speed", 0.75f + r.nextFloat() * 0.08f); P ("half_glide", 260); P ("half_vol", -1);
            if (mid) { put (STOP, 12, 4); put (START, 16, 2); P ("stop_len", 1); P ("stop_curve", -0.3f); P ("start_len", 0.5f); P ("start_curve", 0.2f); }
            if (mid2 && bars == 4) { put (REV, 28, 4); P ("rev_speed", 0.8f); }
            break;
        }
        case 4: // GLITCH
        {
            P ("glitch_dens", 0.5f + amt * 0.4f); P ("glitch_chaos", 0.3f + amt * 0.5f); P ("glitch_size", 0.25f);
            P ("stut_size", 0.125f); P ("stut_roll", 0.4f); P ("stut_pitch", -3); P ("stut_gate", 0.7f);
            P ("gate_rate", 0.25f); P ("gate_depth", 0.9f); P ("gate_duty", 0.4f);
            put (STOP, end - 2, 2); P ("stop_len", 0.5f); P ("stop_curve", -0.4f);
            put (GLITCH, 4, 4);
            const int nEv = 1 + juce::roundToInt (amt * 4) * bars / 2;
            for (int e = 0, tries = 0; e < nEv && tries < 60; ++tries)
            {
                const int beatPos = r.nextInt (S / 4) * 4 + (r.nextBool() ? 0 : 2);   // ottavi, mai sul battere della battuta
                if (beatPos % 16 == 0) continue;
                const int fx = pick ({ 5, 3, 2, 2 }); const int fxs[] = { GLITCH, STUT, REV, GATE }; const int len = fxs[fx] == GLITCH ? 4 : 2;
                if (put (fxs[fx], beatPos, len)) ++e;
            }
            break;
        }
        default: // BUILD-UP su 4 battute
        {
            put (GATE, 32, 16); P ("gate_rate", 0.5f); P ("gate_depth", 0.55f + amt * 0.3f); P ("gate_duty", 0.6f);
            put (STUT, 48, 12); P ("stut_size", 1); P ("stut_roll", 0.9f); P ("stut_pitch", 7); P ("stut_gate", 0.85f); P ("stut_decay", 0.0f); P ("stut_filt", -0.6f);
            put (STOP, 60, 4); P ("stop_len", 1); P ("stop_curve", -0.6f);
            if (mid2) { put (HALF, 24, 8); P ("half_speed", 0.5f); P ("half_glide", 120); }
            break;
        }
    }
    setParam (apvts, "bars", bars == 2 ? 1.0f : 2.0f);
    setParam (apvts, "seqOn", 1.0f);
}

void Processor::getStateInformation (juce::MemoryBlock& dest)
{
    auto st = apvts.copyState();
    juce::String s;
    for (int f = 0; f < kNumFx; ++f) { for (int k = 0; k < kMaxSteps; ++k) s << (seq[f][k].load() ? '1' : '0'); s << ';'; }
    st.setProperty ("seq", s, nullptr); st.setProperty ("preset", presetIndex, nullptr);
    if (auto x = st.createXml()) copyXmlToBinary (*x, dest);
}
void Processor::setStateInformation (const void* data, int size)
{
    auto x = getXmlFromBinary (data, size); if (x == nullptr) return;
    auto st = juce::ValueTree::fromXml (*x); if (! st.isValid()) return;
    apvts.replaceState (st);
    auto rows = juce::StringArray::fromTokens (st.getProperty ("seq").toString(), ";", "");
    for (int f = 0; f < kNumFx && f < rows.size(); ++f) for (int k = 0; k < kMaxSteps && k < rows[f].length(); ++k) seq[f][k] = rows[f][k] == '1';
    presetIndex = st.getProperty ("preset", 0);
}

juce::AudioProcessorEditor* Processor::createEditor() { return new Editor (*this); }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new jt::Processor(); }
