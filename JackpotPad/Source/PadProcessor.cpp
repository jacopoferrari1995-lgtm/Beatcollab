#include "PadProcessor.h"
#include "PadEditor.h"
#include <complex>

namespace jp
{
const PInfo kPInfo[kNumPadParams] = {
    { "gain",   "GAIN",      -24, 24, 0, 11 },      { "level", "VOLUME",    0, 1.41f, 1, 13 },     { "pan",   "PAN",      -1, 1, 0, 9 },
    { "tune",   "TUNE",      -24, 24, 0, 2 },       { "fine",  "FINE",      -100, 100, 0, 3 },
    { "start",  "START",     0, 1, 0, 0 },          { "end",   "END",       0, 1, 1, 0 },
    { "attack", "ATTACK",    0, 200, 0, 4 },        { "decay", "DECAY",     10, 4000, 4000, 4 },   { "release","RELEASE",  5, 2000, 60, 4 },
    { "mode",   "MODE",      0, 1, 0, 6 },          { "reverse","REVERSE",  0, 1, 0, 8 },          { "choke", "CHOKE",     0, 8, 0, 7 },
    { "loop",   "LOOP",      0, 1, 0, 8 },          { "warp",  "SYNC BPM",  0, 2, 0, 6 },          { "sbpm",  "BPM SAMPLE", 0, 240, 0, 10 },
    { "vlevel", "VEL>VOL",   0, 1, 0.75f, 0 },      { "vfilt", "VEL>FILT",  0, 1, 0, 0 },          { "vstart","VEL>START", 0, 1, 0, 0 },     { "vpitch","VEL>PITCH",-1, 1, 0, 1 },
    { "ftype",  "TIPO",      0, 2, 0, 6 },          { "cutoff","CUTOFF",    40, 20000, 20000, 5 }, { "reso",  "RESO",      0, 1, 0.1f, 0 },
    { "fenv",   "ENV AMT",   -1, 1, 0, 1 },         { "fdecay","ENV DECAY", 5, 2000, 200, 4 },
    { "eqon",   "EQ",        0, 1, 1, 8 },
    { "lowg",   "LOW",       -18, 18, 0, 11 },      { "lowf",  "LOW FREQ",  30, 500, 100, 5 },
    { "midg",   "MID",       -18, 18, 0, 11 },      { "midf",  "MID FREQ",  150, 8000, 1000, 5 },  { "midq",  "MID Q",     0.3f, 6, 1, 12 },
    { "highg",  "HIGH",      -18, 18, 0, 11 },      { "highf", "HIGH FREQ", 2000, 16000, 8000, 5 },
    { "cmode",  "COMP",      0, 3, 0, 6 },          { "camt",  "AMOUNT",    0, 1, 0.4f, 0 },       { "cmake", "MAKEUP",    0, 18, 0, 11 },   { "cmix",  "MIX",       0, 1, 1, 0 },
    { "drive",  "DRIVE",     0, 1, 0, 0 },          { "bits",  "BIT",       0, 1, 0, 0 },          { "rate",  "RATE",      0, 1, 0, 0 },
    { "srev",   "REVERB",    0, 1, 0, 0 },          { "sdly",  "DELAY",     0, 1, 0, 0 },
    { "mute",   "MUTE",      0, 1, 0, 8 },          { "solo",  "SOLO",      0, 1, 0, 8 } };

int choiceCount (int i) { return i == FTYPE ? 3 : i == WARP ? 3 : i == CMODE ? 4 : i == CHOKE ? 9 : 2; }

juce::String formatPadParam (int i, float v)
{
    const auto& p = kPInfo[i];
    switch (p.kind)
    {
        case 0: return juce::String (juce::roundToInt (v * 100)) + "%";
        case 1: return std::abs (v) < 0.005f ? juce::String ("0") : (v > 0 ? "+" : "") + juce::String (juce::roundToInt (v * 100));
        case 2: return (v > 0 ? "+" : "") + juce::String (juce::roundToInt (v)) + " st";
        case 3: return (v > 0 ? "+" : "") + juce::String (juce::roundToInt (v)) + " ct";
        case 4: return (i == DECAY && v >= 3990) ? juce::String ("HOLD") : (v < 10 ? juce::String (v, 1) : juce::String (juce::roundToInt (v))) + " ms";
        case 5: return (i == CUTOFF && v >= 19500) ? juce::String ("OPEN") : (v >= 1000 ? juce::String (v / 1000, 1) + "k" : juce::String (juce::roundToInt (v))) + " Hz";
        case 6:
        {
            const int k = juce::roundToInt (v);
            if (i == FTYPE) { const char* t[] = { "LOW PASS", "HIGH PASS", "BAND PASS" }; return t[juce::jlimit (0, 2, k)]; }
            if (i == WARP) { const char* t[] = { "OFF", "REPITCH", "STRETCH" }; return t[juce::jlimit (0, 2, k)]; }
            if (i == CMODE) { const char* t[] = { "OFF", "SOFT", "MEDIUM", "HARD" }; return t[juce::jlimit (0, 3, k)]; }
            return k ? "GATE" : "ONE SHOT";
        }
        case 7: return v < 0.5f ? juce::String ("OFF") : juce::String (juce::roundToInt (v));
        case 8: return v > 0.5f ? "ON" : "OFF";
        case 9: return std::abs (v) < 0.01f ? juce::String ("C") : (v < 0 ? "L" : "R") + juce::String (juce::roundToInt (std::abs (v) * 100));
        case 10: return v < 40 ? juce::String ("--") : juce::String (v, 1);
        case 11: return std::abs (v) < 0.05f ? juce::String ("0 dB") : (v > 0 ? "+" : "") + juce::String (v, 1) + " dB";
        case 12: return juce::String (v, 2);
        case 13: return v < 0.001f ? juce::String ("-inf") : juce::String (juce::Decibels::gainToDecibels (v), 1) + " dB";
        default: return juce::String (v, 2);
    }
}

// ---------------- biquad ----------------
static void rbj (int type, double sr, double f, double gDb, double q, double& b0, double& b1, double& b2, double& a0, double& a1, double& a2)
{
    const double A = std::pow (10.0, gDb / 40.0), w = juce::MathConstants<double>::twoPi * juce::jlimit (10.0, sr * 0.45, f) / sr, cs = std::cos (w), sn = std::sin (w);
    if (type == 1)
    {
        const double al = sn / (2 * q);
        b0 = 1 + al * A; b1 = -2 * cs; b2 = 1 - al * A; a0 = 1 + al / A; a1 = -2 * cs; a2 = 1 - al / A;
    }
    else
    {
        const double al = sn / 2 * std::sqrt (2.0), sA = 2 * std::sqrt (A) * al * 0.5;
        if (type == 0) { b0 = A * ((A + 1) - (A - 1) * cs + sA); b1 = 2 * A * ((A - 1) - (A + 1) * cs); b2 = A * ((A + 1) - (A - 1) * cs - sA);
                         a0 = (A + 1) + (A - 1) * cs + sA; a1 = -2 * ((A - 1) + (A + 1) * cs); a2 = (A + 1) + (A - 1) * cs - sA; }
        else           { b0 = A * ((A + 1) + (A - 1) * cs + sA); b1 = -2 * A * ((A - 1) + (A + 1) * cs); b2 = A * ((A + 1) + (A - 1) * cs - sA);
                         a0 = (A + 1) - (A - 1) * cs + sA; a1 = 2 * ((A - 1) - (A + 1) * cs); a2 = (A + 1) - (A - 1) * cs - sA; }
    }
}
void Biquad::set (int type, double sr, double f, double gDb, double q)
{
    double B0, B1, B2, A0, A1, A2; rbj (type, sr, f, gDb, q, B0, B1, B2, A0, A1, A2);
    b0 = (float) (B0 / A0); b1 = (float) (B1 / A0); b2 = (float) (B2 / A0); a1 = (float) (A1 / A0); a2 = (float) (A2 / A0);
}
float Biquad::magDb (int type, double sr, double f, double gDb, double q, double hz)
{
    double B0, B1, B2, A0, A1, A2; rbj (type, sr, f, gDb, q, B0, B1, B2, A0, A1, A2);
    const double w = juce::MathConstants<double>::twoPi * hz / sr;
    const std::complex<double> z = std::polar (1.0, -w), z2 = z * z;
    const auto num = B0 + B1 * z + B2 * z2, den = A0 + A1 * z + A2 * z2;
    return (float) (20.0 * std::log10 (std::abs (num / den) + 1e-12));
}

// ---------------- parametri globali ----------------
static const juce::StringArray kCurves { "LINEARE", "MORBIDA", "DURA", "S", "FISSA 127" };
static const juce::StringArray kGrid { "1/8", "1/8T", "1/16", "1/16T", "1/32" };
static const double kGridTicks[] = { 48, 32, 24, 16, 12 };
static const juce::StringArray kLen { "1 BAR", "2 BAR", "4 BAR", "8 BAR" };
static const int kLenBars[] = { 1, 2, 4, 8 };
static const juce::StringArray kDly { "1/16", "1/8T", "1/8", "1/8D", "1/4", "1/4D", "1/2" };
static const double kDlyBeats[] = { 0.25, 1.0 / 3.0, 0.5, 0.75, 1.0, 1.5, 2.0 };
static const juce::StringArray kBus { "OFF", "GLUE", "PUNCH", "SMASH" };

juce::AudioProcessorValueTreeState::ParameterLayout Processor::layout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    auto F = [&l] (const char* id, const char* n, float a, float b, float d) { l.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, n, NormalisableRange<float> (a, b), d)); };
    auto C = [&l] (const char* id, const char* n, const StringArray& c, int d) { l.add (std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, n, c, d)); };
    F ("out", "Output", -30, 6, 0);
    C ("curve", "Velocity Curve", kCurves, 0);
    C ("grid", "Quantize Grid", kGrid, 2);
    F ("swing", "Swing", 50, 75, 50);
    F ("strength", "Quantize Strength", 0, 100, 100);
    C ("len", "Loop Length", kLen, 1);
    F ("bpm", "BPM interno", 50, 220, 120);
    l.add (std::make_unique<AudioParameterInt> (ParameterID { "root", 1 }, "Nota del pad 1", 0, 112, 36));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "click", 1 }, "Metronomo", false));
    F ("revSize", "Reverb Size", 0, 1, 0.5f); F ("revDamp", "Reverb Damp", 0, 1, 0.5f); F ("revRet", "Reverb Return", 0, 1, 0.7f);
    C ("dlyTime", "Delay Time", kDly, 3); F ("dlyFb", "Delay Feedback", 0, 0.95f, 0.35f); F ("dlyTone", "Delay Tone", 0, 1, 0.5f); F ("dlyRet", "Delay Return", 0, 1, 0.6f);
    C ("busMode", "Bus Comp", kBus, 0); F ("busAmt", "Bus Comp Amount", 0, 1, 0.4f);
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "clip", 1 }, "Clip", false));
    return l;
}

Processor::Processor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", layout())
{
    formats.registerBasicFormats();
    for (int i = 0; i < kPads; ++i) { padGlow[i] = 0; padMeter[i] = 0; padGr[i] = 0; lastVel[i] = 0; hits[i] = 0; pad[i].colour = i; }
    trigs.reserve (1024); events.reserve (4096);
    loadFactoryKit (0);
}

void Processor::prepareToPlay (double s, int block)
{
    sr = s;
    for (auto& v : voices) { v.active = false; v.s.reset(); }
    padBuf.setSize (kPads * 2, juce::jmax (block, 2048)); padBuf.clear();
    revL.assign ((size_t) juce::jmax (block, 2048), 0); revR = revL; dlyInL = revL; dlyInR = revL;
    dlyBuf.setSize (2, (int) (sr * 4.5)); dlyBuf.clear(); dlyW = 0;
    reverb.setSampleRate (sr); reverb.reset();
    for (auto& c : chain) c = PadChain();
}

float Processor::velCurve (int vel) const
{
    const float x = juce::jlimit (0.0f, 1.0f, vel / 127.0f);
    switch ((int) apvts.getRawParameterValue ("curve")->load())
    {
        case 1: return std::pow (x, 0.55f);
        case 2: return std::pow (x, 1.9f);
        case 3: return x * x * (3 - 2 * x);
        case 4: return 1.0f;
        default: return x;
    }
}

void Processor::uiNoteOn (int p, int vel)
{
    int s1, n1, s2, n2; fifo.prepareToWrite (1, s1, n1, s2, n2);
    if (n1 > 0) fifoBuf[s1] = { p, vel, true }; else if (n2 > 0) fifoBuf[s2] = { p, vel, true };
    fifo.finishedWrite (n1 + n2);
}
void Processor::uiNoteOff (int p)
{
    int s1, n1, s2, n2; fifo.prepareToWrite (1, s1, n1, s2, n2);
    if (n1 > 0) fifoBuf[s1] = { p, 0, false }; else if (n2 > 0) fifoBuf[s2] = { p, 0, false };
    fifo.finishedWrite (n1 + n2);
}

void Processor::startVoice (int p, int vel)
{
    auto s = getSample (p);
    hits[p] = hits[p].load() + 1; lastVel[p] = vel;
    if (s == nullptr || s->buf.getNumSamples() < 8) return;
    auto& P = pad[p];
    const int choke = juce::roundToInt (P.get (CHOKE));
    const bool loop = P.get (LOOP) > 0.5f;
    for (auto& v : voices)
        if (v.active && (v.pad == p || (choke > 0 && v.pad != p && juce::roundToInt (pad[v.pad].get (CHOKE)) == choke)))
            if (v.pad != p || P.get (MODE) < 0.5f || loop) v.choking = true;
    Voice* v = nullptr;
    for (auto& x : voices) if (! x.active) { v = &x; break; }
    if (v == nullptr) { int oldest = -1; for (auto& x : voices) if (x.age > oldest) { oldest = x.age; v = &x; } }
    const float vv = velCurve (vel);
    const int n = s->buf.getNumSamples();
    double a0 = P.get (START) * n, a1 = P.get (END) * n; if (a1 - a0 < 32) a1 = juce::jmin ((double) n, a0 + 32);
    const double sa0 = a0;
    a0 += (1.0 - vv) * P.get (VSTART) * 0.2 * (a1 - a0);
    const double semis = P.get (TUNE) + P.get (FINE) / 100.0 + (vv - 0.75) * P.get (VPITCH) * 4.0;
    const double pitch = std::pow (2.0, semis / 12.0) * s->sr / sr;
    const int warp = juce::roundToInt (P.get (WARP)); const float sb = P.get (SBPM);
    const double tempo = (warp > 0 && sb > 40.0f) ? curBpm / sb : 1.0;
    v->active = true; v->rel = false; v->choking = false; v->pad = p; v->age = 0; v->s = s; v->loop = loop;
    v->dir = P.get (REVERSE) > 0.5f ? -1 : 1;
    v->a0 = loop ? sa0 : a0; v->a1 = a1;
    v->warp = (warp == 2 && sb > 40.0f) ? 2 : (warp == 1 && sb > 40.0f ? 1 : 0);
    v->pos = v->dir > 0 ? a0 : a1 - 2;
    if (v->warp == 2)
    {
        v->tRate = tempo * s->sr / sr; v->pRate = pitch; v->inc = v->tRate;
        v->G = juce::jmax (256.0, 0.05 * sr);
        v->gr[0] = { v->pos - v->dir * v->G * 0.5 * v->pRate, v->G * 0.5 };
        v->gr[1] = { v->pos, 0.0 };
    }
    else v->inc = pitch * (v->warp == 1 ? tempo : 1.0);
    v->gain = 0.5f * juce::Decibels::decibelsToGain (P.get (GAIN) - (1.0f - vv) * 30.0f * P.get (VLEVEL));   // -6 dB di headroom
    const float atk = P.get (ATTACK);
    v->env = atk < 0.5f ? 1.0f : 0.0f; v->atkDone = atk < 0.5f; v->atkInc = atk < 0.5f ? 1.0f : (float) (1.0 / (atk * 0.001 * sr));
    const float dec = P.get (DECAY); v->hold = dec >= 3990 || loop;
    v->decCoef = (float) std::exp (-1.0 / (dec * 0.001 * sr / 4.6));
    v->relCoef = (float) std::exp (-1.0 / (P.get (RELEASE) * 0.001 * sr / 4.6));
    v->gate = (P.get (MODE) > 0.5f || loop) ? 1 : -1;
    v->cutBase = juce::jlimit (30.0f, 20000.0f, P.get (CUTOFF) * std::pow (2.0f, (vv - 1.0f) * P.get (VFILTER) * 5.0f));
    v->fenvAmt = P.get (FENV); v->fEnv = 1.0f; v->fEnvCoef = (float) std::exp (-1.0 / (P.get (FDECAY) * 0.001 * sr / 4.6));
    v->ftype = juce::roundToInt (P.get (FTYPE)); v->fCount = 0;
    v->filt = ! (v->ftype == 0 && v->cutBase >= 19500.0f && std::abs (v->fenvAmt) < 0.01f);
    v->fk = 2.0f - 1.94f * P.get (RESO);
    v->fg = (float) std::tan (juce::MathConstants<double>::pi * juce::jmin ((double) v->cutBase, sr * 0.45) / sr);
    for (int c = 0; c < 2; ++c) { v->ic1[c] = 0; v->ic2[c] = 0; }
    chain[p].active = (int) (sr * 1.5);
}

void Processor::releasePad (int p) { for (auto& v : voices) if (v.active && v.pad == p && v.gate > 0) v.rel = true; }

void Processor::render (Voice& v, float* L, float* R, int i)
{
    const auto& b = v.s->buf; const int n = b.getNumSamples();
    const float* d0 = b.getReadPointer (0); const float* d1 = b.getReadPointer (juce::jmin (1, b.getNumChannels() - 1));
    auto rd = [&] (double pos, float& l, float& r)
    {
        if (v.loop) { const double span = v.a1 - v.a0; while (pos >= v.a1) pos -= span; while (pos < v.a0) pos += span; }
        if (pos < 0 || pos >= n - 1) { l = r = 0; return; }
        const int k = (int) pos; const float f = (float) (pos - k);
        const int km = juce::jmax (0, k - 1), k1 = juce::jmin (n - 1, k + 1), k2 = juce::jmin (n - 1, k + 2);
        auto cub = [f] (float y0, float y1, float y2, float y3) { return y1 + 0.5f * f * (y2 - y0 + f * (2 * y0 - 5 * y1 + 4 * y2 - y3 + f * (3 * (y1 - y2) + y3 - y0))); };
        l = cub (d0[km], d0[k], d0[k1], d0[k2]); r = cub (d1[km], d1[k], d1[k1], d1[k2]);
    };
    float out[2] = { 0, 0 };
    if (v.warp == 2)
    {
        for (auto& g : v.gr)
        {
            const float w = 0.5f - 0.5f * std::cos ((float) (juce::MathConstants<double>::twoPi * g.ph / v.G));
            float l, r; rd (g.start + v.dir * g.ph * v.pRate, l, r);
            out[0] += l * w; out[1] += r * w;
            g.ph += 1; if (g.ph >= v.G) { g.ph = 0; g.start = v.pos; }
        }
    }
    else rd (v.pos, out[0], out[1]);
    v.pos += v.inc * v.dir; ++v.age;
    // fine del campione o del loop
    if (v.loop) { const double span = v.a1 - v.a0; if (v.pos >= v.a1) v.pos -= span; if (v.pos < v.a0) v.pos += span; }
    else if (v.dir > 0 ? v.pos >= v.a1 - 2 : v.pos <= v.a0) { v.active = false; v.s.reset(); return; }
    // inviluppo
    if (v.choking) { v.env *= 0.996f; if (v.env < 0.0005f) { v.active = false; v.s.reset(); return; } }
    else if (v.rel) { v.env *= v.relCoef; if (v.env < 0.0002f) { v.active = false; v.s.reset(); return; } }
    else if (! v.atkDone) { v.env += v.atkInc; if (v.env >= 1) { v.env = 1; v.atkDone = true; } }
    else if (! v.hold) { v.env *= v.decCoef; if (v.env < 0.0002f) { v.active = false; v.s.reset(); return; } }
    const double dist = v.loop ? 1e9 : (v.dir > 0 ? v.a1 - v.pos : v.pos - v.a0);
    const float edge = dist < 64 ? (float) (dist / 64) : 1.0f;
    if (v.filt)
    {
        if (std::abs (v.fenvAmt) > 0.005f && (v.fCount++ & 15) == 0)
        {
            const float cut = juce::jlimit (30.0f, 20000.0f, v.cutBase * std::pow (2.0f, v.fenvAmt * 7.0f * v.fEnv));
            v.fg = (float) std::tan (juce::MathConstants<double>::pi * juce::jmin ((double) cut, sr * 0.45) / sr);
        }
        v.fEnv *= v.fEnvCoef;
        for (int c = 0; c < 2; ++c)
        {
            const float x = out[c], v3 = x - v.ic2[c], v1 = (v.ic1[c] + v.fg * v3) / (1 + v.fg * (v.fg + v.fk)), v2 = v.ic2[c] + v.fg * v1;
            v.ic1[c] = 2 * v1 - v.ic1[c]; v.ic2[c] = 2 * v2 - v.ic2[c];
            out[c] = v.ftype == 0 ? v2 : v.ftype == 1 ? x - v.fk * v1 - v2 : v1 * v.fk;
        }
    }
    const float g = v.gain * v.env * edge;
    L[i] += out[0] * g; R[i] += out[1] * g;
}

int Processor::loopTicks() const { return kLenBars[juce::jlimit (0, 3, (int) apvts.getRawParameterValue ("len")->load())] * kBar; }

double Processor::quantize (double t) const
{
    const int gi = juce::jlimit (0, 4, (int) apvts.getRawParameterValue ("grid")->load());
    const double g = kGridTicks[gi], sw = apvts.getRawParameterValue ("swing")->load(), st = apvts.getRawParameterValue ("strength")->load() / 100.0;
    const double n = std::round (t / g); double q = n * g;
    const bool straight = gi == 0 || gi == 2 || gi == 4;
    if (straight && ((juce::int64) n % 2 != 0)) q += (sw - 50.0) / 50.0 * g;
    const double L = loopTicks(); double r = t + (q - t) * st;
    r = std::fmod (r, L); if (r < 0) r += L;
    return r;
}

void Processor::record (int p, int vel, double tick, double absTick)
{
    const juce::SpinLock::ScopedTryLockType l (recLock);
    if (l.isLocked() && events.size() < 8000) events.push_back ({ std::fmod (tick, (double) loopTicks()), p, vel, absTick });
}
void Processor::erasePad (int p)
{
    const juce::SpinLock::ScopedLockType l (recLock);
    events.erase (std::remove_if (events.begin(), events.end(), [p] (const RecEv& e) { return e.pad == p; }), events.end());
}

void Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals nd;
    const int N = buffer.getNumSamples();
    buffer.clear(); trigs.clear();
    if (padBuf.getNumSamples() < N) { padBuf.setSize (kPads * 2, N, false, false, true); revL.resize ((size_t) N); revR.resize ((size_t) N); dlyInL.resize ((size_t) N); dlyInR.resize ((size_t) N); }
    auto par = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };

    double bpm = par ("bpm"); bool hostPlay = false; double ppq = 0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = *b;
            hostPlay = pos->getIsPlaying();
            if (auto q = pos->getPpqPosition()) ppq = *q;
        }
    curBpm = bpm;
    const int req = transportReq.exchange (0);
    if (req == 1) { intPlay = true; intTick = 0; }
    if (req == 2) { intPlay = false; recording = false; }
    const bool running = hostPlay || intPlay.load();
    const double tps = bpm / 60.0 * PPQ / sr;
    const double t0 = hostPlay ? ppq * PPQ : intTick, t1 = t0 + N * tps;
    uiHost = hostPlay; uiRunning = running; uiBpm = bpm;
    if (running && ! wasRunning) for (auto& e : events) e.recAbs = -1e9;
    if (! running && wasRunning) for (auto& v : voices) if (v.active && v.loop) v.rel = true;   // stop: i loop si fermano
    wasRunning = running;

    const int root = rootNote();
    const bool rec = recording.load() && running;
    auto noteOn = [&] (int off, int p, int vel) { trigs.push_back ({ off, p, vel, true }); if (rec) { const double t = t0 + off * tps; record (p, vel, t, t); } };
    {
        int s1, n1, s2, n2; fifo.prepareToRead (fifo.getNumReady(), s1, n1, s2, n2);
        for (int k = 0; k < n1 + n2; ++k) { const auto& e = fifoBuf[k < n1 ? s1 + k : s2 + k - n1]; if (e.on) noteOn (0, e.pad, e.vel); else trigs.push_back ({ 0, e.pad, 0, false }); }
        fifo.finishedRead (n1 + n2);
    }
    for (const auto m : midi)
    {
        const auto msg = m.getMessage(); if (! msg.isNoteOnOrOff()) continue;
        const int p = msg.getNoteNumber() - root; if (p < 0 || p >= kPads) continue;
        if (msg.isNoteOn()) noteOn (m.samplePosition, p, msg.getVelocity()); else trigs.push_back ({ m.samplePosition, p, 0, false });
    }
    if (running)
    {
        const double L = loopTicks();
        const juce::SpinLock::ScopedTryLockType l (recLock);
        if (l.isLocked())
            for (auto& e : events)
            {
                const double q = quantize (e.tick), base = std::floor (t0 / L) * L;
                for (double b = base; b <= t1; b += L)
                {
                    const double a = b + q; if (a < t0 || a >= t1) continue;
                    if (e.recAbs > -1e8 && a >= e.recAbs - 1 && a - e.recAbs < L * 0.5) { e.recAbs = -1e9; continue; }
                    trigs.push_back ({ juce::jlimit (0, N - 1, (int) ((a - t0) / tps)), e.pad, e.vel, true });
                }
            }
        if (par ("click") > 0.5f || recording.load())
            for (double b = std::ceil (t0 / PPQ) * PPQ; b < t1; b += PPQ)
                trigs.push_back ({ juce::jlimit (0, N - 1, (int) ((b - t0) / tps)), std::fmod (b, (double) kBar) < 1 ? -2 : -1, 0, true });
    }
    std::stable_sort (trigs.begin(), trigs.end(), [] (const Trig& a, const Trig& b) { return a.off < b.off; });
    if (! hostPlay && intPlay.load()) intTick += N * tps;
    uiTick = running ? t1 : 0.0;

    // 1) voci nei bus dei pad
    for (int p = 0; p < kPads; ++p) if (chain[p].active > 0) { padBuf.clear (p * 2, 0, N); padBuf.clear (p * 2 + 1, 0, N); }
    size_t ti = 0;
    float* cl = buffer.getWritePointer (0); float* cr = buffer.getWritePointer (1);
    for (int i = 0; i < N; ++i)
    {
        while (ti < trigs.size() && trigs[ti].off <= i)
        {
            const auto& t = trigs[ti++];
            if (t.pad < 0) { clickEnv = 0.3f; clickPh = 0; clickF = t.pad == -2 ? 2000.0f : 1400.0f; }
            else if (t.on)
            {
                if (chain[t.pad].active <= 0) { padBuf.clear (t.pad * 2, 0, N); padBuf.clear (t.pad * 2 + 1, 0, N); }
                startVoice (t.pad, t.vel);
            }
            else releasePad (t.pad);
        }
        for (auto& v : voices) if (v.active) render (v, padBuf.getWritePointer (v.pad * 2), padBuf.getWritePointer (v.pad * 2 + 1), i);
        if (clickEnv > 0.0005f) { const float c = std::sin (clickPh) * clickEnv; clickPh += (float) (juce::MathConstants<double>::twoPi * clickF / sr); clickEnv *= 0.997f; cl[i] += c; cr[i] += c; }
    }
    // 2) catena di ogni pad, fader, mandate
    std::fill (revL.begin(), revL.begin() + N, 0.0f); std::fill (revR.begin(), revR.begin() + N, 0.0f);
    std::fill (dlyInL.begin(), dlyInL.begin() + N, 0.0f); std::fill (dlyInR.begin(), dlyInR.begin() + N, 0.0f);
    bool anySolo = false; for (int p = 0; p < kPads; ++p) if (pad[p].get (SOLO) > 0.5f) anySolo = true;
    for (int p = 0; p < kPads; ++p)
    {
        auto& C = chain[p]; auto& P = pad[p];
        if (C.active <= 0) { padMeter[p] = padMeter[p].load() * 0.8f; padGr[p] = 0; continue; }
        C.active -= N;
        float* L = padBuf.getWritePointer (p * 2); float* R = padBuf.getWritePointer (p * 2 + 1);
        float pk = 0; for (int i = 0; i < N; ++i) pk = juce::jmax (pk, std::abs (L[i]) + std::abs (R[i]));
        if (pk > padGlow[p].load()) padGlow[p] = pk;
        if (pk > 1e-5f) C.active = juce::jmax (C.active, (int) (sr * 0.5));
        // EQ
        if (P.get (EQON) > 0.5f)
        {
            const float pv[7] = { P.get (LOWG), P.get (LOWF), P.get (MIDG), P.get (MIDF), P.get (MIDQ), P.get (HIGHG), P.get (HIGHF) };
            if (std::memcmp (pv, C.lastEq, sizeof (pv)) != 0)
            {
                C.eq[0].set (0, sr, pv[1], pv[0], 0.707); C.eq[1].set (1, sr, pv[3], pv[2], pv[4]); C.eq[2].set (2, sr, pv[6], pv[5], 0.707);
                std::memcpy (C.lastEq, pv, sizeof (pv));
            }
            for (int k = 0; k < 3; ++k)
            {
                if (std::abs (C.lastEq[k == 0 ? 0 : k == 1 ? 2 : 5]) < 0.05f) continue;
                for (int i = 0; i < N; ++i) { L[i] = C.eq[k].run (L[i], 0); R[i] = C.eq[k].run (R[i], 1); }
            }
        }
        // compressore
        const int cm = juce::roundToInt (P.get (CMODE));
        if (cm > 0)
        {
            struct CT { float ratio, att, rel, knee; };
            static const CT T[] = { { 1, 1, 1, 0 }, { 2.0f, 15, 160, 10 }, { 4.0f, 5, 100, 6 }, { 10.0f, 0.4f, 60, 1 } };
            const auto& t = T[cm];
            const float thr = -P.get (CAMT) * 36.0f, mk = juce::Decibels::decibelsToGain (P.get (CMAKEUP) + P.get (CAMT) * 36.0f * (1 - 1 / t.ratio) * 0.25f), mix = P.get (CMIX);
            const float aA = (float) std::exp (-1.0 / (t.att * 0.001 * sr)), aR = (float) std::exp (-1.0 / (t.rel * 0.001 * sr));
            float maxGr = 0;
            for (int i = 0; i < N; ++i)
            {
                const float x = juce::jmax (std::abs (L[i]), std::abs (R[i]));
                C.cEnv = x > C.cEnv ? aA * C.cEnv + (1 - aA) * x : aR * C.cEnv + (1 - aR) * x;
                const float lev = juce::Decibels::gainToDecibels (C.cEnv + 1e-7f), over = lev - thr;
                float gr = 0;
                if (over > t.knee * 0.5f) gr = over * (1 - 1 / t.ratio);
                else if (over > -t.knee * 0.5f) { const float o = over + t.knee * 0.5f; gr = (1 - 1 / t.ratio) * o * o / (2 * t.knee); }
                maxGr = juce::jmax (maxGr, gr);
                const float g = juce::Decibels::decibelsToGain (-gr) * mk;
                L[i] = L[i] * (1 - mix) + L[i] * g * mix; R[i] = R[i] * (1 - mix) + R[i] * g * mix;
            }
            padGr[p] = maxGr;
        }
        else padGr[p] = 0;
        // drive e lo-fi
        const float dr = P.get (DRIVE), bits = P.get (BITS), rate = P.get (RATE);
        if (dr > 0.001f) { const float k = 1 + dr * 14; const float nrm = (1.0f + (k - 1.0f) * 0.15f) / k; for (int i = 0; i < N; ++i) { L[i] = std::tanh (L[i] * k) * nrm; R[i] = std::tanh (R[i] * k) * nrm; } }
        if (bits > 0.001f || rate > 0.001f)
        {
            const float q = std::pow (2.0f, 15.0f - bits * 11.0f); const double step = 1.0 - rate * 0.88;
            for (int i = 0; i < N; ++i)
            {
                C.holdPh += step; if (C.holdPh >= 1.0) { C.holdPh -= 1.0; C.holdL = L[i]; C.holdR = R[i]; }
                L[i] = std::round (C.holdL * q) / q; R[i] = std::round (C.holdR * q) / q;
            }
        }
        // fader, pan, mute/solo
        const bool audible = P.get (MUTE) < 0.5f && (! anySolo || P.get (SOLO) > 0.5f);
        const float lv = audible ? P.get (LEVEL) : 0.0f, pan = P.get (PAN);
        const float gl = lv * std::cos ((pan + 1) * juce::MathConstants<float>::pi * 0.25f) * 1.41421f, gr2 = lv * std::sin ((pan + 1) * juce::MathConstants<float>::pi * 0.25f) * 1.41421f;
        const float sRev = P.get (SREV), sDly = P.get (SDLY);
        float mpk = 0;
        for (int i = 0; i < N; ++i)
        {
            const float l = L[i] * gl, r = R[i] * gr2;
            cl[i] += l; cr[i] += r; mpk = juce::jmax (mpk, juce::jmax (std::abs (l), std::abs (r)));
            if (sRev > 0) { revL[(size_t) i] += l * sRev; revR[(size_t) i] += r * sRev; }
            if (sDly > 0) { dlyInL[(size_t) i] += l * sDly; dlyInR[(size_t) i] += r * sDly; }
        }
        padMeter[p] = juce::jmax (mpk, padMeter[p].load() * 0.8f);
    }
    // 3) master: delay, riverbero, bus comp, clip, uscita
    {
        const int len = dlyBuf.getNumSamples();
        const double dT = juce::jlimit (16.0, (double) len - 4, kDlyBeats[juce::jlimit (0, 6, (int) par ("dlyTime"))] * 60.0 / bpm * sr);
        const float fb = par ("dlyFb"), ret = par ("dlyRet"), a = (float) std::exp (-juce::MathConstants<double>::twoPi * (700.0 * std::pow (10.0, par ("dlyTone") * 1.4)) / sr);
        for (int i = 0; i < N; ++i)
        {
            double rp = dlyW - dT; if (rp < 0) rp += len;
            const int i0 = (int) rp; const float fr = (float) (rp - i0); const int i1 = (i0 + 1) % len;
            const float yL = dlyBuf.getSample (0, i0) * (1 - fr) + dlyBuf.getSample (0, i1) * fr, yR = dlyBuf.getSample (1, i0) * (1 - fr) + dlyBuf.getSample (1, i1) * fr;
            dlyLpL = (1 - a) * yR + a * dlyLpL; dlyLpR = (1 - a) * yL + a * dlyLpR;
            dlyBuf.setSample (0, dlyW, dlyInL[(size_t) i] + std::tanh (dlyLpL * fb)); dlyBuf.setSample (1, dlyW, dlyInR[(size_t) i] + std::tanh (dlyLpR * fb));
            if (++dlyW >= len) dlyW = 0;
            cl[i] += yL * ret; cr[i] += yR * ret;
        }
        juce::Reverb::Parameters rp; rp.roomSize = 0.3f + 0.68f * par ("revSize"); rp.damping = par ("revDamp"); rp.wetLevel = 1; rp.dryLevel = 0; rp.width = 1;
        reverb.setParameters (rp); reverb.processStereo (revL.data(), revR.data(), N);
        const float rr = par ("revRet") * 0.6f;
        for (int i = 0; i < N; ++i) { cl[i] += revL[(size_t) i] * rr; cr[i] += revR[(size_t) i] * rr; }
        const int bm = (int) par ("busMode");
        float maxGr = 0;
        if (bm > 0)
        {
            struct BT { float ratio, att, rel; };
            static const BT T[] = { { 1, 1, 1 }, { 2.0f, 10, 200 }, { 4.0f, 20, 90 }, { 12.0f, 0.5f, 60 } };
            const auto& t = T[bm]; const float thr = -par ("busAmt") * 30.0f, mk = juce::Decibels::decibelsToGain (par ("busAmt") * 30.0f * (1 - 1 / t.ratio) * 0.4f);
            const float aA = (float) std::exp (-1.0 / (t.att * 0.001 * sr)), aR = (float) std::exp (-1.0 / (t.rel * 0.001 * sr));
            for (int i = 0; i < N; ++i)
            {
                const float x = juce::jmax (std::abs (cl[i]), std::abs (cr[i]));
                busEnv = x > busEnv ? aA * busEnv + (1 - aA) * x : aR * busEnv + (1 - aR) * x;
                const float lev = juce::Decibels::gainToDecibels (busEnv + 1e-7f), gr = lev > thr ? (lev - thr) * (1 - 1 / t.ratio) : 0.0f;
                maxGr = juce::jmax (maxGr, gr); const float g = juce::Decibels::decibelsToGain (-gr) * mk; cl[i] *= g; cr[i] *= g;
            }
        }
        busGr = maxGr;
        const bool clip = par ("clip") > 0.5f; const float og = juce::Decibels::decibelsToGain (par ("out"));
        float pk[2] = { 0, 0 };
        for (int i = 0; i < N; ++i)
        {
            float l = cl[i] * og, r = cr[i] * og;
            if (clip) { auto sc = [] (float x) { const float a = std::abs (x); return a > 0.85f ? std::copysign (0.85f + 0.15f * std::tanh ((a - 0.85f) / 0.15f), x) : x; }; l = sc (l); r = sc (r); }
            cl[i] = l; cr[i] = r; pk[0] = juce::jmax (pk[0], std::abs (l)); pk[1] = juce::jmax (pk[1], std::abs (r));
        }
        meterL = juce::jmax (pk[0], meterL.load() * 0.85f); meterR = juce::jmax (pk[1], meterR.load() * 0.85f);
    }
}

// ---------------- campioni e BPM ----------------
float Processor::estimateBpm (const juce::String& name, double sec, bool& confident)
{
    confident = false;
    auto toks = juce::StringArray::fromTokens (name.toLowerCase().replaceCharacters ("_-.()[]", "       "), " ", "");
    for (int i = 0; i < toks.size(); ++i)
    {
        const auto t = toks[i]; const bool tagged = t.endsWith ("bpm") || (i + 1 < toks.size() && toks[i + 1] == "bpm");
        const auto num = t.upToFirstOccurrenceOf ("bpm", false, true);
        if (tagged && num.containsOnly ("0123456789.") && num.isNotEmpty()) { const float v = num.getFloatValue(); if (v >= 55 && v <= 210) { confident = true; return v; } }
    }
    for (auto& t : toks)
        if (t.containsOnly ("0123456789") && t.isNotEmpty())
        {
            const int v = t.getIntValue();
            if (v >= 60 && v <= 200) { const double beats = sec * v / 60.0, m = std::round (beats / 2.0) * 2.0; if (m >= 2 && std::abs (beats - m) / m < 0.04) { confident = true; return (float) v; } }
        }
    if (sec < 1.2) return 0.0f;
    float best = 0; double bestScore = 1e9;
    for (double beats : { 4.0, 8.0, 16.0, 32.0, 64.0, 2.0 })
    {
        const double b = beats * 60.0 / sec; if (b < 70 || b > 180) continue;
        const double score = std::abs (std::log (b / 115.0)) + std::abs (b - std::round (b)) * 0.5;
        if (score < bestScore) { bestScore = score; best = (float) b; }
    }
    if (best > 0 && std::abs (best - std::round (best)) < 0.06f) { confident = true; best = std::round (best); }
    return best;
}

void Processor::detectBpm (int p)
{
    auto s = getSample (p); if (! s) return;
    bool conf = false; const float b = estimateBpm (s->name, s->buf.getNumSamples() / s->sr, conf);
    pad[p].v[SBPM] = b;
    if (conf && b > 0) { pad[p].v[WARP] = 2; pad[p].v[DECAY] = 4000; }
}

bool Processor::loadFile (const juce::File& f, int p)
{
    std::unique_ptr<juce::AudioFormatReader> rd (formats.createReaderFor (f));
    if (rd == nullptr || rd->lengthInSamples < 8) return false;
    const int len = (int) juce::jmin<juce::int64> (rd->lengthInSamples, (juce::int64) (rd->sampleRate * 60));
    auto s = std::make_shared<SampleData>();
    s->buf.setSize (2, len); rd->read (&s->buf, 0, len, 0, true, true);
    if (rd->numChannels == 1) s->buf.copyFrom (1, 0, s->buf, 0, 0, len);
    s->sr = rd->sampleRate; s->name = f.getFileNameWithoutExtension(); s->path = f.getFullPathName();
    pad[p].v[START] = 0; pad[p].v[END] = 1; pad[p].v[WARP] = 0; pad[p].v[SBPM] = 0; pad[p].v[LOOP] = 0;
    setSample (p, s);
    detectBpm (p);
    return true;
}

// ---------------- kit ----------------
juce::File Processor::kitFolder()
{
    auto d = juce::File::getSpecialLocation (juce::File::userMusicDirectory).getChildFile ("Jackpot Pad").getChildFile ("Kits");
    d.createDirectory(); return d;
}
juce::Array<juce::File> Processor::userKits() const
{
    auto a = kitFolder().findChildFiles (juce::File::findFiles, false, "*.jpkit");
    std::sort (a.begin(), a.end(), [] (const juce::File& x, const juce::File& y) { return x.getFileName().compareIgnoreCase (y.getFileName()) < 0; });
    return a;
}
static SamplePtr sampleFromPath (Processor& P, const juce::String& path, int p)
{
    DrumDesc d;
    if (pathToDesc (path, d)) return makeDrum (d);
    if (path.startsWith ("builtin:")) return makeBuiltin (path.fromFirstOccurrenceOf (":", false, false).getIntValue());
    if (path.isNotEmpty() && juce::File (path).existsAsFile())
    {
        float keep[kNumPadParams]; for (int i = 0; i < kNumPadParams; ++i) keep[i] = P.pad[p].get (i);
        P.loadFile (juce::File (path), p);
        for (int i = 0; i < kNumPadParams; ++i) P.pad[p].v[i] = keep[i];
        return P.getSample (p);
    }
    return nullptr;
}
std::unique_ptr<juce::XmlElement> Processor::padsToXml() const
{
    auto x = std::make_unique<juce::XmlElement> ("PADS");
    for (int p = 0; p < kPads; ++p)
    {
        auto* e = x->createNewChildElement ("P");
        for (int i = 0; i < kNumPadParams; ++i) e->setAttribute (kPInfo[i].id, pad[p].get (i));
        e->setAttribute ("colour", pad[p].colour.load());
        if (auto s = getSample (p)) e->setAttribute ("path", s->path);
    }
    return x;
}
void Processor::padsFromXml (const juce::XmlElement& pads)
{
    int p = 0;
    for (auto* e : pads.getChildIterator())
    {
        if (p >= kPads) break;
        for (int i = 0; i < kNumPadParams; ++i) pad[p].v[i] = (float) e->getDoubleAttribute (kPInfo[i].id, kPInfo[i].def);
        pad[p].colour = e->getIntAttribute ("colour", p);
        const auto path = e->getStringAttribute ("path");
        if (path.isEmpty()) setSample (p, nullptr);
        else { auto s = sampleFromPath (*this, path, p); setSample (p, s); }
        ++p;
    }
}
void Processor::loadFactoryKit (int k)
{
    const auto& K = factoryKits()[(size_t) juce::jlimit (0, (int) factoryKits().size() - 1, k)];
    for (int p = 0; p < kPads; ++p)
    {
        pad[p].reset();
        if (p < (int) K.pads.size())
        {
            const auto& kp = K.pads[(size_t) p];
            setSample (p, makeDrum (kp.d)); pad[p].colour = kp.colour;
            for (auto& [i, v] : kp.p) pad[p].v[i] = v;
        }
        else setSample (p, nullptr);
    }
    kitName = K.name; kitFile = juce::File();
}
bool Processor::saveKit (const juce::File& f, const juce::String& name)
{
    juce::XmlElement x ("JPKIT"); x.setAttribute ("name", name); x.setAttribute ("version", 2);
    x.addChildElement (padsToXml().release());
    auto* m = x.createNewChildElement ("MASTER");
    for (auto id : { "revSize", "revDamp", "revRet", "dlyTime", "dlyFb", "dlyTone", "dlyRet", "busMode", "busAmt", "clip" }) m->setAttribute (id, (double) apvts.getParameter (id)->getValue());
    if (! x.writeTo (f)) return false;
    kitName = name; kitFile = f; return true;
}
bool Processor::loadKit (const juce::File& f)
{
    auto x = juce::XmlDocument::parse (f); if (x == nullptr || ! x->hasTagName ("JPKIT")) return false;
    if (auto* pads = x->getChildByName ("PADS")) padsFromXml (*pads);
    if (auto* m = x->getChildByName ("MASTER"))
        for (auto id : { "revSize", "revDamp", "revRet", "dlyTime", "dlyFb", "dlyTone", "dlyRet", "busMode", "busAmt", "clip" })
            if (m->hasAttribute (id)) apvts.getParameter (id)->setValueNotifyingHost ((float) m->getDoubleAttribute (id));
    kitName = x->getStringAttribute ("name", f.getFileNameWithoutExtension()); kitFile = f;
    return true;
}

juce::File Processor::writeMidi()
{
    juce::MidiMessageSequence seq; const int root = rootNote();
    { const juce::SpinLock::ScopedLockType l (recLock);
      for (auto& e : events) { const double q = quantize (e.tick); seq.addEvent (juce::MidiMessage::noteOn (1, root + e.pad, (juce::uint8) e.vel), q); seq.addEvent (juce::MidiMessage::noteOff (1, root + e.pad), q + 12); } }
    seq.updateMatchedPairs(); seq.sort();
    juce::MidiFile mf; mf.setTicksPerQuarterNote (PPQ); mf.addTrack (seq);
    auto f = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("Jackpot Pad pattern.mid");
    f.deleteFile(); juce::FileOutputStream os (f); if (os.openedOk()) mf.writeTo (os);
    return f;
}

void Processor::getStateInformation (juce::MemoryBlock& dest)
{
    juce::XmlElement x ("JPAD"); x.setAttribute ("kit", kitName); x.setAttribute ("kitFile", kitFile.getFullPathName());
    x.addChildElement (apvts.copyState().createXml().release());
    x.addChildElement (padsToXml().release());
    juce::String ev; { const juce::SpinLock::ScopedLockType l (recLock); for (auto& e : events) ev << juce::String (e.tick, 2) << "," << e.pad << "," << e.vel << ";"; }
    x.createNewChildElement ("PATTERN")->addTextElement (ev);
    copyXmlToBinary (x, dest);
}
void Processor::setStateInformation (const void* data, int size)
{
    auto x = getXmlFromBinary (data, size); if (x == nullptr || ! x->hasTagName ("JPAD")) return;
    if (auto* ps = x->getChildByName (apvts.state.getType())) apvts.replaceState (juce::ValueTree::fromXml (*ps));
    if (auto* pads = x->getChildByName ("PADS")) padsFromXml (*pads);
    kitName = x->getStringAttribute ("kit", kitName);
    const auto kf = x->getStringAttribute ("kitFile"); kitFile = kf.isNotEmpty() ? juce::File (kf) : juce::File();
    if (auto* pt = x->getChildByName ("PATTERN"))
    {
        const juce::SpinLock::ScopedLockType l (recLock); events.clear();
        for (auto& t : juce::StringArray::fromTokens (pt->getAllSubText(), ";", ""))
        {
            auto f = juce::StringArray::fromTokens (t, ",", ""); if (f.size() < 3) continue;
            events.push_back ({ f[0].getDoubleValue(), f[1].getIntValue(), f[2].getIntValue(), -1e9 });
        }
    }
}

juce::AudioProcessorEditor* Processor::createEditor() { return new Editor (*this); }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new jp::Processor(); }
