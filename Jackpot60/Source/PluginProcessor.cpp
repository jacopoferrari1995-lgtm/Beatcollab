#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace j60
{
static const juce::StringArray kTC { "OFF", "1/8", "1/8T", "1/16", "1/16T", "1/32" };
static const juce::StringArray kRPT { "1/4", "1/4T", "1/8", "1/8T", "1/16", "1/16T", "1/32", "1/32T" };
static const juce::StringArray kVIN { "CLEAN", "MPC60", "SP-1200", "S950", "CASSETTE" };
static const juce::StringArray kDLY { "1/16", "1/8T", "1/8", "1/8D", "1/4", "1/4D", "1/2" };
static const double kDlyBeats[] = { 0.25, 1.0 / 3.0, 0.5, 0.75, 1.0, 1.5, 2.0 };

int Processor::tcTicks (int c) { static const int t[] = { 0, 48, 32, 24, 16, 12 }; return t[juce::jlimit (0, 5, c)]; }
int Processor::repeatTicks (int c) { static const int t[] = { 96, 64, 48, 32, 24, 16, 12, 8 }; return t[juce::jlimit (0, 7, c)]; }

juce::AudioProcessorValueTreeState::ParameterLayout Processor::layout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "swing", 1 }, "Swing", NormalisableRange<float> (50.0f, 75.0f, 0.1f), 50.0f));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "tc", 1 }, "Timing Correct", kTC, 3));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "rpt", 1 }, "Note Repeat", kRPT, 4));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "vintage", 1 }, "Vintage", kVIN, 1));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "drive", 1 }, "Drive", 0.0f, 1.0f, 0.2f));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "tone", 1 }, "Tone", 0.0f, 1.0f, 0.8f));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "out", 1 }, "Output", NormalisableRange<float> (-24.0f, 6.0f, 0.1f), 0.0f));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "bpm", 1 }, "Internal BPM", NormalisableRange<float> (60.0f, 200.0f, 0.1f), 90.0f));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "click", 1 }, "Click", false));
    Burro::addParams (l);
    // delay e riverbero (mandate per pad)
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "dTime", 1 }, "Delay Time", kDLY, 3));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "dFb", 1 }, "Delay Feedback", 0.0f, 0.95f, 0.4f));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "dTone", 1 }, "Delay Tone", 0.0f, 1.0f, 0.5f));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "dRet", 1 }, "Delay Return", 0.0f, 1.0f, 0.7f));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "rSize", 1 }, "Reverb Size", 0.0f, 1.0f, 0.55f));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "rDamp", 1 }, "Reverb Damp", 0.0f, 1.0f, 0.5f));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "rRet", 1 }, "Reverb Return", 0.0f, 1.0f, 0.6f));
    return l;
}

Processor::Processor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", layout())
{
    formats.registerBasicFormats();
    for (auto& h : hitCount) h = 0;
    for (auto& h : heldVel) h = 0;
    for (auto& r : recAbs) r = -1.0e9;
    trigs.reserve (512);
    loadDefaultKit();
}

void Processor::loadDefaultKit()
{
    for (int i = 0; i < kBuiltinCount; ++i) setSample (i, makeBuiltin (i));
    pad[3].choke = 1; pad[4].choke = 1;      // hat chiuso e aperto si zittiscono a vicenda
    pad[3].level = 0.55f; pad[4].level = 0.5f; pad[6].level = 0.5f; pad[9].level = 0.45f;
    pad[10].level = 0.85f;
    // sequenza di partenza: 2 battute trap a mezzo tempo
    auto& s = seq[0]; s.bars = 2; s.ev.clear();
    auto add = [&s] (int step, int p, int v) { s.ev.push_back ({ step * kTick16, p, v }); };
    for (int b = 0; b < 2; ++b)
    {
        const int o = b * 16;
        add (o + 0, 0, 120); add (o + 7, 0, 100); add (o + 10, 0, 110);
        add (o + 0, 10, 110); add (o + 7, 10, 95); add (o + 10, 10, 105);
        add (o + 8, 2, 115); add (o + 8, 1, 90);
        for (int h = 0; h < 16; h += 2) add (o + h, 3, h % 4 == 0 ? 100 : 72);
        add (o + 14, 4, 80);
    }
    add (31, 3, 60); add (29, 3, 55);
}

void Processor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    for (auto& v : voices) v.active = false;
    dlyBuf.setSize (2, (int) (sr * 4.5)); dlyBuf.clear(); dlyW = 0;
    rsBuf.setSize (2, (int) (sr * 60.0)); rsState = 0;
    reverb.setSampleRate (sr); reverb.reset();
    burro.prepare (sr, samplesPerBlock);
}

double Processor::swingTick (int tick, float swing) const
{
    if (tick % (kTick16 * 2) == kTick16) return tick + (swing - 50.0f) / 100.0f * (kTick16 * 2);
    return tick;
}

void Processor::uiPadDown (int p, int vel)
{
    int s1, n1, s2, n2; fifo.prepareToWrite (1, s1, n1, s2, n2);
    if (n1 > 0) fifoBuf[s1] = { p, vel, true }; else if (n2 > 0) fifoBuf[s2] = { p, vel, true };
    fifo.finishedWrite (n1 + n2);
}
void Processor::uiPadUp (int p)
{
    int s1, n1, s2, n2; fifo.prepareToWrite (1, s1, n1, s2, n2);
    if (n1 > 0) fifoBuf[s1] = { p, 0, false }; else if (n2 > 0) fifoBuf[s2] = { p, 0, false };
    fifo.finishedWrite (n1 + n2);
}
void Processor::requestStop() { transportReq = 1; }
void Processor::requestPlayStart() { transportReq = 2; }
void Processor::requestPlay() { transportReq = 3; }

void Processor::recordHit (int p, int vel, double tick)
{
    const int tc = tcTicks ((int) *apvts.getRawParameterValue ("tc"));
    const juce::SpinLock::ScopedLockType l (seqLock);
    auto& s = seq[curSeq.load()];
    const int L = s.bars * kTicksPerBar;
    int t = (int) std::llround (tc > 0 ? std::round (tick / tc) * tc : tick);
    t = ((t % L) + L) % L;
    recAbs[p] = (double) std::llround (tc > 0 ? std::round (tick / tc) * tc : tick);
    const int tol = tc > 0 ? tc / 2 : 4;
    for (auto& e : s.ev)
        if (e.pad == p && std::abs (e.tick - t) < tol) { e.vel = std::max (e.vel, vel); return; }
    s.ev.push_back ({ t, p, vel });
}

void Processor::startVoice (int p, int vel, int gateSamples)
{
    hitCount[p].fetch_add (1);
    SamplePtr s = getSample (p);
    if (s == nullptr || s->buf.getNumSamples() < 16) return;
    auto& P = pad[p];
    const int ch = P.choke.load();
    if (ch > 0)
        for (auto& v : voices)
            if (v.active && v.pad != p && pad[v.pad].choke.load() == ch) v.choking = true;
    const bool isLoop = P.loop.load() != 0;
    if (isLoop) for (auto& x : voices) if (x.active && x.pad == p) x.choking = true;   // loop: una voce per pad
    Voice* v = nullptr;
    for (auto& x : voices) if (! x.active) { v = &x; break; }
    if (v == nullptr) { v = &voices[0]; for (auto& x : voices) if (x.age < v->age) v = &x; }
    if (fullLevel) vel = 127;
    const int len = s->buf.getNumSamples();
    v->smp = s; v->pad = p; v->active = true; v->releasing = false; v->choking = false; v->age = ++voiceAge;
    v->startF = juce::jlimit (0.0, (double) len - 2, (double) P.start.load() * len);
    v->endF = juce::jlimit (v->startF + 32.0, (double) len, (double) P.end.load() * len);
    v->dir = P.reverse.load() ? -1 : 1;
    v->pos = v->dir > 0 ? v->startF : v->endF - 1.0;
    v->inc = std::pow (2.0, (P.tune.load() + P.fine.load() / 100.0) / 12.0) * s->sr / sr;
    if (P.sync.load() && P.bpm.load() > 20.0f) v->inc *= curBpm / P.bpm.load();
    v->loop = isLoop; v->sDly = P.sendDly.load(); v->sRev = P.sendRev.load();
    const float vs = P.velSens.load();
    const float g = P.level.load() * (1.0f - vs + vs * std::pow (vel / 127.0f, 1.5f));
    const float pan = P.pan.load(), a = (pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f;
    v->gL = g * std::cos (a) * 1.4142f; v->gR = g * std::sin (a) * 1.4142f;
    const float atk = (float) (P.attack.load() / 1000.0 * sr);
    v->attackInc = atk > 1 ? 1.0f / atk : 1.0f; v->env = atk > 1 ? 0.0f : 1.0f; v->attackDone = atk > 1 ? 0 : 1;
    const float dec = P.decay.load();
    v->sustain = dec >= 4990.0f;
    v->decayCoef = (float) std::exp (-1.0 / (dec / 1000.0 * sr / 5.0));
    v->relCoef = (float) std::exp (-1.0 / (P.release.load() / 1000.0 * sr / 5.0));
    v->note = (P.mode.load() == 1) ? p : -1;
    v->gate = (P.mode.load() == 1 && gateSamples > 0 && ! isLoop) ? gateSamples : -1;
    const float fc = P.cutoff.load();
    v->filt = fc < 19000.0f;
    v->fg = (float) std::tan (juce::MathConstants<double>::pi * juce::jmin ((double) fc, sr * 0.45) / sr);
    v->fk = 2.0f - 1.94f * P.reso.load();
    v->ic1L = v->ic2L = v->ic1R = v->ic2R = 0;
}

void Processor::releasePad (int p)
{
    for (auto& v : voices) if (v.active && v.pad == p && v.note >= 0) v.releasing = true;
}

static inline float cubic (const float* d, int n, double pos)
{
    const int i = (int) pos; const float f = (float) (pos - i);
    auto at = [d, n] (int k) { return d[juce::jlimit (0, n - 1, k)]; };
    const float a = at (i - 1), b = at (i), c = at (i + 1), e = at (i + 2);
    return b + 0.5f * f * (c - a + f * (2.0f * a - 5.0f * b + 4.0f * c - e + f * (3.0f * (b - c) + e - a)));
}

void Processor::renderVoice (Voice& v, float& L, float& R)
{
    if (v.loop)
    {
        const double span = v.endF - 1.0 - v.startF;
        if (v.dir > 0 && v.pos >= v.endF - 1.0) v.pos -= span;
        if (v.dir < 0 && v.pos <= v.startF) v.pos += span;
    }
    if ((v.dir > 0 && v.pos >= v.endF - 1.0) || (v.dir < 0 && v.pos <= v.startF)) { v.active = false; v.smp.reset(); return; }
    const auto& b = v.smp->buf; const int n = b.getNumSamples();
    float l = cubic (b.getReadPointer (0), n, v.pos), r = cubic (b.getReadPointer (b.getNumChannels() > 1 ? 1 : 0), n, v.pos);
    if (v.choking) { v.env *= 0.992f; if (v.env < 1e-4f) { v.active = false; v.smp.reset(); return; } }
    else if (v.releasing) { v.env *= v.relCoef; if (v.env < 1e-4f) { v.active = false; v.smp.reset(); return; } }
    else if (! v.attackDone) { v.env += v.attackInc; if (v.env >= 1.0f) { v.env = 1.0f; v.attackDone = 1; } }
    else if (! v.sustain) { v.env *= v.decayCoef; if (v.env < 1e-4f) { v.active = false; v.smp.reset(); return; } }
    if (v.gate > 0 && --v.gate == 0) v.releasing = true;
    const double dist = v.dir > 0 ? v.endF - v.pos : v.pos - v.startF;
    const double dist0 = v.dir > 0 ? v.pos - v.startF : v.endF - v.pos;
    float edge = dist < 64.0 ? (float) (dist / 64.0) : 1.0f;
    if (v.loop && dist0 < 64.0) edge = juce::jmin (edge, (float) (dist0 / 64.0));
    if (v.filt)
    {
        auto svf = [&v] (float x, float& ic1, float& ic2) { const float v3 = x - ic2; const float v1 = (ic1 + v.fg * v3) / (1 + v.fg * (v.fg + v.fk)); const float v2 = ic2 + v.fg * v1; ic1 = 2 * v1 - ic1; ic2 = 2 * v2 - ic2; return v2; };
        l = svf (l, v.ic1L, v.ic2L); r = svf (r, v.ic1R, v.ic2R);
    }
    const float e = v.env * edge;
    L += l * v.gL * e; R += r * v.gR * e;
    if (v.sDly > 0) { sendDL += l * v.gL * e * v.sDly; sendDR += r * v.gR * e * v.sDly; }
    if (v.sRev > 0) { sendRL += l * v.gL * e * v.sRev; sendRR += r * v.gR * e * v.sRev; }
    v.pos += v.inc * v.dir;
}

void Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals nd;
    const int N = buffer.getNumSamples();
    buffer.clear();
    const float swing = *apvts.getRawParameterValue ("swing");
    const int rptC = (int) *apvts.getRawParameterValue ("rpt");
    const int vin = (int) *apvts.getRawParameterValue ("vintage");
    const float drive = *apvts.getRawParameterValue ("drive"), tone = *apvts.getRawParameterValue ("tone");
    const float outG = juce::Decibels::decibelsToGain ((float) *apvts.getRawParameterValue ("out"));
    const bool click = *apvts.getRawParameterValue ("click") > 0.5f;
    double bpm = *apvts.getRawParameterValue ("bpm");

    bool hostPlaying = false; double hostPpq = 0.0;
    if (syncHost)
        if (auto* ph = getPlayHead())
            if (auto pos = ph->getPosition())
            {
                if (auto b = pos->getBpm()) bpm = *b;
                hostPlaying = pos->getIsPlaying();
                if (auto p = pos->getPpqPosition()) hostPpq = *p;
            }
    const int req = transportReq.exchange (0);
    if (req == 1) { intPlaying = false; for (auto& v : voices) if (v.active) v.releasing = true; }
    if (req == 2) { intTick = 0.0; intPlaying = true; }
    if (req == 3) intPlaying = true;

    bool running = false; double t0 = 0.0;
    if (syncHost && hostPlaying) { running = true; t0 = hostPpq * PPQ; }
    else if (intPlaying) { running = true; t0 = intTick; }
    const double tps = bpm / 60.0 * PPQ / sr, t1 = t0 + N * tps;
    curBpm = bpm;
    int rsStartAt = 0;
    if (rsState.load() == 1 && running)
    {
        int bars = 2; { const juce::SpinLock::ScopedTryLockType lk (seqLock); if (lk.isLocked()) bars = seq[curSeq.load()].bars; }
        const double nb = std::ceil (t0 / kTicksPerBar - 1e-9) * kTicksPerBar;
        if (nb < t1)
        {
            rsStartAt = juce::jlimit (0, N - 1, (int) ((nb - t0) / tps));
            rsTarget = juce::jmin (rsBuf.getNumSamples(), (int) (bars * kTicksPerBar / tps));
            rsLen = 0; rsBpm = bpm; rsState = 2;
        }
    }
    trigs.clear();
    std::vector<std::pair<int,int>> toRecord; // pad, vel (al tick t0 + offset)
    double recTick[64]; int recN = 0;

    // pad dall'interfaccia
    {
        int s1, n1, s2, n2; fifo.prepareToRead (fifo.getNumReady(), s1, n1, s2, n2);
        auto handle = [&] (const UiEv& e)
        {
            if (e.on) { heldVel[e.pad] = e.vel; trigs.push_back ({ 0, e.pad, e.vel, true }); if (recording && running && recN < 64) { toRecord.push_back ({ e.pad, e.vel }); recTick[recN++] = t0; } }
            else { heldVel[e.pad] = 0; trigs.push_back ({ 0, e.pad, 0, false }); }
        };
        for (int i = 0; i < n1; ++i) handle (fifoBuf[s1 + i]);
        for (int i = 0; i < n2; ++i) handle (fifoBuf[s2 + i]);
        fifo.finishedRead (n1 + n2);
    }
    // MIDI dall'host: C1 (36) = A01 … 64 note
    for (const auto meta : midi)
    {
        const auto m = meta.getMessage(); const int note = m.getNoteNumber() - kBaseNote;
        if (note < 0 || note >= kPads) continue;
        if (m.isNoteOn())
        {
            const int vel = m.getVelocity();
            heldVel[note] = vel; trigs.push_back ({ meta.samplePosition, note, vel, true });
            if (recording && running && recN < 64) { toRecord.push_back ({ note, vel }); recTick[recN++] = t0 + meta.samplePosition * tps; }
        }
        else if (m.isNoteOff()) { heldVel[note] = 0; trigs.push_back ({ meta.samplePosition, note, 0, false }); }
    }
    midi.clear();

    const int gate16 = (int) (kTick16 / tps);
    if (running)
    {
        int L = kTicksPerBar * 2;
        {
            const juce::SpinLock::ScopedTryLockType lk (seqLock);
            if (lk.isLocked())
            {
                const auto& s = seq[curSeq.load()];
                L = s.bars * kTicksPerBar;
                for (const auto& e : s.ev)
                {
                    const double et = swingTick (e.tick, swing);
                    for (double k = std::ceil ((t0 - et) / L); et + k * L < t1; k += 1.0)
                    {
                        if (std::abs (et + k * L - recAbs[e.pad]) < 1.5) continue;   // appena registrato: non ripeterlo
                        const int off = juce::jlimit (0, N - 1, (int) ((et + k * L - t0) / tps));
                        trigs.push_back ({ off, e.pad, e.vel, true, gate16 });
                    }
                }
            }
        }
        if (noteRepeat)
        {
            const int R = repeatTicks (rptC);
            for (int p = 0; p < kPads; ++p)
            {
                const int hv = heldVel[p].load(); if (hv <= 0) continue;
                for (double m = std::ceil (t0 / R) * R; m < t1; m += R)
                {
                    const double mt = (R == kTick16) ? swingTick ((int) std::fmod (m, (double) L), swing) + std::floor (m / L) * L : m;
                    if (mt < t0 || mt >= t1) continue;
                    const int off = juce::jlimit (0, N - 1, (int) ((mt - t0) / tps));
                    trigs.push_back ({ off, p, hv, true, gate16 });
                    if (recording && recN < 64) { toRecord.push_back ({ p, hv }); recTick[recN++] = mt; }
                }
            }
        }
        if (click && recording)
            for (double q = std::ceil (t0 / PPQ) * PPQ; q < t1; q += PPQ)
                trigs.push_back ({ juce::jlimit (0, N - 1, (int) ((q - t0) / tps)), -1, std::fmod (q, (double) kTicksPerBar) < 1 ? 127 : 80, true });
        if (! (syncHost && hostPlaying)) intTick = t1;
        uiTick = std::fmod (t1, (double) L);
    }
    else if (noteRepeat)
    {
        const int R = repeatTicks (rptC); const double r0 = repeatClock, r1 = r0 + N * tps;
        for (int p = 0; p < kPads; ++p)
        {
            const int hv = heldVel[p].load(); if (hv <= 0) continue;
            for (double m = std::ceil (r0 / R) * R; m < r1; m += R)
                if (m > r0) trigs.push_back ({ juce::jlimit (0, N - 1, (int) ((m - r0) / tps)), p, hv, true, gate16 });
        }
        repeatClock = r1;
    }
    for (int i = 0; i < recN; ++i) recordHit (toRecord[(size_t) i].first, toRecord[(size_t) i].second, recTick[i]);
    uiRunning = running; uiBpm = bpm;

    std::stable_sort (trigs.begin(), trigs.end(), [] (const Trig& a, const Trig& b) { return a.offset < b.offset; });

    // modi vintage: frequenza, bit, filtro di carattere, saturazione
    static const double vRate[] = { 0, 40000, 26040, 32000, 0 };
    static const int vBits[] = { 0, 12, 12, 12, 0 };
    static const double vLp[] = { 0, 16000, 13000, 9500, 10500 };
    static const float vDrv[] = { 0.0f, 0.15f, 0.25f, 0.1f, 0.45f };
    const float charA = vLp[vin] > 0 ? (float) std::exp (-juce::MathConstants<double>::twoPi * vLp[vin] / sr) : 0.0f;
    const float toneA = (float) std::exp (-juce::MathConstants<double>::twoPi * (2000.0 * std::pow (10.0, (double) tone)) / sr);
    const float d = drive * 3.0f + vDrv[vin] * 2.0f, dn = 1.0f + 0.3f * d;
    const float q = vBits[vin] > 0 ? (float) (1 << (vBits[vin] - 1)) : 0.0f;

    // drum bus, delay, riverbero
    auto P = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };
    const float dFb = P ("dFb"), dRet = P ("dRet"), rRet = P ("rRet");
    const int dlyLen = dlyBuf.getNumSamples();
    const double dlyT = juce::jlimit (16.0, (double) dlyLen - 4.0, kDlyBeats[juce::jlimit (0, 6, (int) P ("dTime"))] * 60.0 / bpm * sr);
    const float dlyA = (float) std::exp (-juce::MathConstants<double>::twoPi * (800.0 * std::pow (10.0, P ("dTone") * 1.3)) / sr);
    { juce::Reverb::Parameters rp; rp.roomSize = 0.3f + 0.68f * P ("rSize"); rp.damping = P ("rDamp"); rp.wetLevel = 1.0f; rp.dryLevel = 0.0f; rp.width = 1.0f; reverb.setParameters (rp); }
    if (revL.size() < (size_t) N) { revL.resize ((size_t) N); revR.resize ((size_t) N); }

    float* oL = buffer.getWritePointer (0); float* oR = buffer.getWritePointer (1);
    size_t ti = 0;
    for (int i = 0; i < N; ++i)
    {
        while (ti < trigs.size() && trigs[ti].offset <= i)
        {
            const auto& t = trigs[ti++];
            if (t.pad < 0) { clickEnv = t.vel / 127.0f; clickPh = 0; }
            else if (t.on) startVoice (t.pad, t.vel, t.gate);
            else releasePad (t.pad);
        }
        float l = 0, r = 0; sendDL = sendDR = sendRL = sendRR = 0;
        for (auto& v : voices) if (v.active) renderVoice (v, l, r);
        // delay ping-pong a tempo
        {
            double rp = dlyW - dlyT; if (rp < 0) rp += dlyLen;
            const int i0 = (int) rp; const float fr = (float) (rp - i0); const int i1 = (i0 + 1) % dlyLen;
            const float yL = dlyBuf.getSample (0, i0) * (1 - fr) + dlyBuf.getSample (0, i1) * fr, yR = dlyBuf.getSample (1, i0) * (1 - fr) + dlyBuf.getSample (1, i1) * fr;
            dlyLpL = (1 - dlyA) * yR + dlyA * dlyLpL; dlyLpR = (1 - dlyA) * yL + dlyA * dlyLpR;
            dlyBuf.setSample (0, dlyW, sendDL + std::tanh (dlyLpL * dFb)); dlyBuf.setSample (1, dlyW, sendDR + std::tanh (dlyLpR * dFb));
            if (++dlyW >= dlyLen) dlyW = 0;
            l += yL * dRet; r += yR * dRet;
        }
        revL[(size_t) i] = sendRL; revR[(size_t) i] = sendRR;
        if (clickEnv > 0.001f) { const float c = std::sin (clickPh) * clickEnv * 0.25f; clickPh += (float) (juce::MathConstants<double>::twoPi * 1700.0 / sr); clickEnv *= 0.9965f; l += c; r += c; }
        oL[i] = l; oR[i] = r;
    }
    reverb.processStereo (revL.data(), revR.data(), N);

    for (int i = 0; i < N; ++i)
    {
        float l = oL[i] + revL[(size_t) i] * rRet * 0.7f, r = oR[i] + revR[(size_t) i] * rRet * 0.7f;
        // --- carattere vintage ---
        if (vRate[vin] > 0) { vPhase += vRate[vin] / sr; if (vPhase >= 1.0) { vPhase -= 1.0; vHoldL = l; vHoldR = r; } l = vHoldL; r = vHoldR; }
        if (q > 0) { l = std::round (l * q) / q; r = std::round (r * q) / q; }
        if (charA > 0) { charZL = (1 - charA) * l + charA * charZL; charZR = (1 - charA) * r + charA * charZR; l = charZL; r = charZR; }
        oL[i] = l; oR[i] = r;
    }
    burro.process (oL, oR, N, apvts);
    for (int i = 0; i < N; ++i)
    {
        float l = oL[i], r = oR[i];
        if (d > 0.01f) { l = std::tanh (l * (1 + d)) / dn; r = std::tanh (r * (1 + d)) / dn; }
        toneZL = (1 - toneA) * l + toneA * toneZL; toneZR = (1 - toneA) * r + toneA * toneZR;
        l = toneZL; r = toneZR;
        if (vin == 4) { const float h = (rnd.nextFloat() * 2 - 1) * 0.0007f; l += h; r += h; }
        oL[i] = l * outG; oR[i] = r * outG;
        if (rsState.load (std::memory_order_relaxed) == 2 && i >= rsStartAt)
        {
            const int k = rsLen.load (std::memory_order_relaxed);
            if (k < rsTarget) { rsBuf.setSample (0, k, oL[i]); rsBuf.setSample (1, k, oR[i]); rsLen.store (k + 1, std::memory_order_relaxed); }
            else rsState = 3;
        }
    }
}

void Processor::toggleResample()
{
    const int st = rsState.load();
    if (st == 0)
    {
        if (! uiRunning.load()) requestPlayStart();
        rsState = 1;
    }
    else if (st == 1) rsState = 0;
    else if (st == 2) rsState = 3;
}

SamplePtr Processor::takeResample()
{
    if (rsState.load() != 3) return nullptr;
    const int n = rsLen.load();
    SamplePtr s;
    if (n > 64)
    {
        s = std::make_shared<SampleData>();
        s->buf.setSize (2, n); s->buf.copyFrom (0, 0, rsBuf, 0, 0, n); s->buf.copyFrom (1, 0, rsBuf, 1, 0, n);
        s->sr = sr; s->name = "Resample " + juce::String (juce::roundToInt (rsBpm)) + " BPM";
        const auto f = juce::File::getSpecialLocation (juce::File::userMusicDirectory).getChildFile ("Jackpot 60").getChildFile ("Resample " + juce::Time::getCurrentTime().formatted ("%Y%m%d %H%M%S") + ".wav");
        f.getParentDirectory().createDirectory();
        juce::WavAudioFormat wav;
        if (auto os = std::unique_ptr<juce::OutputStream> (f.createOutputStream()))
            if (auto w = std::unique_ptr<juce::AudioFormatWriter> (wav.createWriterFor (os.get(), sr, 2, 24, {}, 0)))
            { os.release(); w->writeFromAudioSampleBuffer (s->buf, 0, n); s->path = f.getFullPathName(); }
    }
    rsState = 0;
    return s;
}

float Processor::estimateBpm (const juce::String& name, double sec, bool& confident)
{
    confident = false;
    // 1) BPM scritto nel nome del file: "loop 140bpm", "Am_92_BPM", "drill-142"
    juce::String n = name.toLowerCase().replaceCharacters ("_-.()[]", "       ");
    auto toks = juce::StringArray::fromTokens (n, " ", "");
    for (int i = 0; i < toks.size(); ++i)
    {
        auto t = toks[i];
        const bool tagged = t.endsWith ("bpm") || (i + 1 < toks.size() && toks[i + 1] == "bpm");
        const auto num = t.retainCharacters ("0123456789.");
        if (num.isEmpty() || num.length() != t.length() - (t.endsWith ("bpm") ? 3 : 0)) continue;
        const float v = num.getFloatValue();
        if (v >= 55.0f && v <= 210.0f && (tagged || (int) v == v)) { if (tagged) { confident = true; return v; } }
    }
    for (auto& t : toks) { if (t.containsOnly ("0123456789")) { const int v = t.getIntValue(); if (v >= 60 && v <= 200) { const double beats = sec * v / 60.0, m = std::round (beats / 2.0) * 2.0; if (m >= 2.0 && std::abs (beats - m) / m < 0.04) { confident = true; return (float) v; } } } }
    // 2) dalla durata: battute intere a un tempo plausibile
    if (sec < 1.2) return 0.0f;
    float best = 0.0f; double bestScore = 1e9;
    for (double beats : { 4.0, 8.0, 16.0, 32.0, 64.0, 2.0 })
    {
        const double b = beats * 60.0 / sec;
        if (b < 70.0 || b > 180.0) continue;
        const double score = std::abs (std::log (b / 115.0)) + std::abs (b - std::round (b)) * 0.5;
        if (score < bestScore) { bestScore = score; best = (float) b; }
    }
    if (best > 0.0f && std::abs (best - std::round (best)) < 0.06f) { confident = true; best = std::round (best); }
    return best;
}

bool Processor::loadFileToPad (const juce::File& f, int p)
{
    std::unique_ptr<juce::AudioFormatReader> rd (formats.createReaderFor (f));
    if (rd == nullptr) return false;
    const int len = (int) juce::jmin ((juce::int64) (rd->sampleRate * 60.0), rd->lengthInSamples);
    if (len < 16) return false;
    auto s = std::make_shared<SampleData>();
    s->buf.setSize (2, len); s->sr = rd->sampleRate; s->name = f.getFileNameWithoutExtension(); s->path = f.getFullPathName();
    rd->read (&s->buf, 0, len, 0, true, true);
    if (rd->numChannels == 1) s->buf.copyFrom (1, 0, s->buf, 0, 0, len);
    bool conf = false;
    const float b = estimateBpm (s->name, len / rd->sampleRate, conf);
    pad[p].bpm = b; pad[p].sync = (conf && b > 0.0f) ? 1 : 0;
    setSample (p, s);
    return true;
}

void Processor::clearPad (int p) { setSample (p, nullptr); }

void Processor::chopPad (int src, int slices, bool transient)
{
    SamplePtr s = getSample (src); if (s == nullptr) return;
    const int len = s->buf.getNumSamples();
    const double a = pad[src].start.load() * len, b = pad[src].end.load() * len;
    std::vector<double> pts;
    if (! transient)
        for (int i = 0; i <= slices; ++i) pts.push_back (a + (b - a) * i / slices);
    else
    {
        const int hop = 512; std::vector<float> en;
        for (int i = (int) a; i + hop < (int) b; i += hop)
        { float e = 0; for (int k = 0; k < hop; ++k) { const float x = s->buf.getSample (0, i + k); e += x * x; } en.push_back (std::sqrt (e / hop)); }
        std::vector<float> flux (en.size(), 0.0f);
        for (size_t i = 1; i < en.size(); ++i) flux[i] = std::max (0.0f, en[i] - en[i - 1]);
        float mean = 0; for (auto f : flux) mean += f; mean /= std::max<size_t> (1, flux.size());
        float sd = 0; for (auto f : flux) sd += (f - mean) * (f - mean); sd = std::sqrt (sd / std::max<size_t> (1, flux.size()));
        const int minGap = (int) (0.07 * s->sr / hop);
        pts.push_back (a); int last = -minGap;
        for (size_t i = 1; i + 1 < flux.size() && pts.size() < (size_t) slices; ++i)
            if (flux[i] > mean + 1.2f * sd && flux[i] >= flux[i - 1] && flux[i] >= flux[i + 1] && (int) i - last >= minGap && i > 2)
            { pts.push_back (a + (double) i * hop); last = (int) i; }
        pts.push_back (b);
    }
    const int base = ((src / kPadsPerBank + 1) % kBanks) * kPadsPerBank;
    const int n = (int) pts.size() - 1;
    for (int i = 0; i < kPadsPerBank; ++i)
    {
        const int p = base + i;
        if (i < n)
        {
            setSample (p, s);
            auto& P = pad[p];
            P.level = pad[src].level.load(); P.pan = 0; P.tune = pad[src].tune.load(); P.fine = 0; P.attack = 0; P.decay = 5000; P.release = 60;
            P.cutoff = 20000; P.reso = 0.1f; P.velSens = 0.7f; P.reverse = 0; P.mode = 0; P.choke = 0;
            P.loop = 0; P.sync = pad[src].sync.load(); P.bpm = pad[src].bpm.load(); P.sendDly = pad[src].sendDly.load(); P.sendRev = pad[src].sendRev.load();
            P.start = (float) (pts[(size_t) i] / len); P.end = (float) (pts[(size_t) i + 1] / len);
        }
        else setSample (p, nullptr);
    }
}

juce::File Processor::writeMidiFile()
{
    juce::MidiMessageSequence ms;
    int bars = 2; std::vector<Event> ev;
    { const juce::SpinLock::ScopedLockType l (seqLock); bars = seq[curSeq.load()].bars; ev = seq[curSeq.load()].ev; }
    const float swing = *apvts.getRawParameterValue ("swing");
    for (const auto& e : ev)
    {
        const double t = swingTick (e.tick, swing);
        ms.addEvent (juce::MidiMessage::noteOn (1, kBaseNote + e.pad, (juce::uint8) e.vel), t);
        ms.addEvent (juce::MidiMessage::noteOff (1, kBaseNote + e.pad), t + kTick16 - 1);
    }
    ms.addEvent (juce::MidiMessage::endOfTrack(), (double) bars * kTicksPerBar);
    ms.updateMatchedPairs(); ms.sort();
    juce::MidiFile mf; mf.setTicksPerQuarterNote (PPQ); mf.addTrack (ms);
    auto f = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("Jackpot60 SEQ" + juce::String (curSeq.load() + 1).paddedLeft ('0', 2) + ".mid");
    f.deleteFile();
    juce::FileOutputStream os (f);
    if (os.openedOk()) mf.writeTo (os);
    return f;
}

void Processor::getStateInformation (juce::MemoryBlock& dest)
{
    juce::XmlElement root ("J60");
    if (auto x = apvts.copyState().createXml()) root.addChildElement (x.release());
    auto* pads = root.createNewChildElement ("PADS");
    for (int p = 0; p < kPads; ++p)
    {
        auto* e = pads->createNewChildElement ("PAD"); e->setAttribute ("i", p);
        auto& P = pad[p];
        e->setAttribute ("level", P.level.load()); e->setAttribute ("pan", P.pan.load()); e->setAttribute ("tune", P.tune.load()); e->setAttribute ("fine", P.fine.load());
        e->setAttribute ("start", P.start.load()); e->setAttribute ("end", P.end.load()); e->setAttribute ("attack", P.attack.load()); e->setAttribute ("decay", P.decay.load());
        e->setAttribute ("release", P.release.load()); e->setAttribute ("cutoff", P.cutoff.load()); e->setAttribute ("reso", P.reso.load()); e->setAttribute ("velSens", P.velSens.load());
        e->setAttribute ("reverse", P.reverse.load()); e->setAttribute ("mode", P.mode.load()); e->setAttribute ("choke", P.choke.load());
        e->setAttribute ("loop", P.loop.load()); e->setAttribute ("sync", P.sync.load()); e->setAttribute ("bpm", P.bpm.load());
        e->setAttribute ("sDly", P.sendDly.load()); e->setAttribute ("sRev", P.sendRev.load());
        if (auto s = getSample (p)) e->setAttribute ("path", s->path);
    }
    auto* seqs = root.createNewChildElement ("SEQS"); seqs->setAttribute ("cur", curSeq.load());
    {
        const juce::SpinLock::ScopedLockType l (seqLock);
        for (int i = 0; i < kSeqs; ++i)
        {
            auto* e = seqs->createNewChildElement ("SEQ"); e->setAttribute ("bars", seq[i].bars);
            juce::String txt; for (const auto& ev : seq[i].ev) txt << ev.tick << "," << ev.pad << "," << ev.vel << ";";
            e->setAttribute ("ev", txt);
        }
    }
    root.setAttribute ("sync", syncHost.load() ? 1 : 0);
    copyXmlToBinary (root, dest);
}

void Processor::setStateInformation (const void* data, int size)
{
    auto xml = getXmlFromBinary (data, size);
    if (xml == nullptr || ! xml->hasTagName ("J60")) return;
    if (auto* ps = xml->getChildByName ("PARAMS")) apvts.replaceState (juce::ValueTree::fromXml (*ps));
    syncHost = xml->getIntAttribute ("sync", 1) != 0;
    if (auto* pads = xml->getChildByName ("PADS"))
        for (auto* e : pads->getChildIterator())
        {
            const int p = e->getIntAttribute ("i", -1); if (p < 0 || p >= kPads) continue;
            auto& P = pad[p];
            P.level = (float) e->getDoubleAttribute ("level", 0.8); P.pan = (float) e->getDoubleAttribute ("pan"); P.tune = (float) e->getDoubleAttribute ("tune"); P.fine = (float) e->getDoubleAttribute ("fine");
            P.start = (float) e->getDoubleAttribute ("start", 0); P.end = (float) e->getDoubleAttribute ("end", 1); P.attack = (float) e->getDoubleAttribute ("attack"); P.decay = (float) e->getDoubleAttribute ("decay", 5000);
            P.release = (float) e->getDoubleAttribute ("release", 80); P.cutoff = (float) e->getDoubleAttribute ("cutoff", 20000); P.reso = (float) e->getDoubleAttribute ("reso", 0.1); P.velSens = (float) e->getDoubleAttribute ("velSens", 0.7);
            P.reverse = e->getIntAttribute ("reverse"); P.mode = e->getIntAttribute ("mode"); P.choke = e->getIntAttribute ("choke");
            const juce::String path = e->getStringAttribute ("path");
            if (path.startsWith ("builtin:")) setSample (p, makeBuiltin (path.fromFirstOccurrenceOf (":", false, false).getIntValue()));
            else if (path.isNotEmpty() && juce::File (path).existsAsFile()) loadFileToPad (juce::File (path), p);
            else setSample (p, nullptr);
            P.loop = e->getIntAttribute ("loop"); P.sync = e->getIntAttribute ("sync"); P.bpm = (float) e->getDoubleAttribute ("bpm");
            P.sendDly = (float) e->getDoubleAttribute ("sDly"); P.sendRev = (float) e->getDoubleAttribute ("sRev");
        }
    if (auto* seqs = xml->getChildByName ("SEQS"))
    {
        const juce::SpinLock::ScopedLockType l (seqLock);
        int i = 0;
        for (auto* e : seqs->getChildIterator())
        {
            if (i >= kSeqs) break;
            seq[i].bars = juce::jlimit (1, kMaxBars, e->getIntAttribute ("bars", 2)); seq[i].ev.clear();
            for (auto& tok : juce::StringArray::fromTokens (e->getStringAttribute ("ev"), ";", ""))
            { auto f = juce::StringArray::fromTokens (tok, ",", ""); if (f.size() == 3) seq[i].ev.push_back ({ f[0].getIntValue(), f[1].getIntValue(), f[2].getIntValue() }); }
            ++i;
        }
        curSeq = juce::jlimit (0, kSeqs - 1, seqs->getIntAttribute ("cur", 0));
    }
}

juce::AudioProcessorEditor* Processor::createEditor() { return new Editor (*this); }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new j60::Processor(); }
