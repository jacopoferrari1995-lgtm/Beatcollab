#include "PadProcessor.h"
#include "PadEditor.h"

namespace jp
{
const PInfo kPInfo[kNumPadParams] = {
    { "level",  "LEVEL",    0, 1, 0.8f, 0 },      { "pan",   "PAN",      -1, 1, 0, 9 },
    { "tune",   "TUNE",     -24, 24, 0, 2 },      { "fine",  "FINE",     -100, 100, 0, 3 },
    { "start",  "START",    0, 1, 0, 0 },         { "end",   "END",      0, 1, 1, 0 },
    { "attack", "ATTACK",   0, 200, 0, 4 },       { "decay", "DECAY",    10, 4000, 4000, 4 },
    { "release","RELEASE",  5, 2000, 60, 4 },     { "ftype", "FILTER",   0, 2, 0, 6 },
    { "cutoff", "CUTOFF",   40, 20000, 20000, 5 },{ "reso",  "RESO",     0, 1, 0.1f, 0 },
    { "vlevel", "VEL>LEVEL",0, 1, 0.75f, 0 },     { "vfilt", "VEL>FILT", 0, 1, 0, 0 },
    { "vstart", "VEL>START",0, 1, 0, 0 },         { "vpitch","VEL>PITCH",-1, 1, 0, 1 },
    { "choke",  "CHOKE",    0, 8, 0, 7 },         { "mode",  "MODE",     0, 1, 0, 6 },
    { "reverse","REVERSE",  0, 1, 0, 8 } };

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
        case 5: return v >= 19500 ? juce::String ("OPEN") : (v >= 1000 ? juce::String (v / 1000, 1) + "k" : juce::String (juce::roundToInt (v))) + (v >= 19500 ? "" : " Hz");
        case 6: if (i == FTYPE) { const char* t[] = { "LOW PASS", "HIGH PASS", "BAND PASS" }; return t[juce::jlimit (0, 2, juce::roundToInt (v))]; }
                return v > 0.5f ? "GATE" : "ONE SHOT";
        case 7: return v < 0.5f ? juce::String ("OFF") : juce::String (juce::roundToInt (v));
        case 8: return v > 0.5f ? "ON" : "OFF";
        case 9: return std::abs (v) < 0.01f ? juce::String ("C") : (v < 0 ? "L" : "R") + juce::String (juce::roundToInt (std::abs (v) * 100));
        default: return juce::String (v, 2);
    }
}

static const juce::StringArray kCurves { "LINEARE", "MORBIDA", "DURA", "S", "FISSA 127" };
static const juce::StringArray kGrid { "1/8", "1/8T", "1/16", "1/16T", "1/32" };
static const double kGridTicks[] = { 48, 32, 24, 16, 12 };
static const juce::StringArray kLen { "1 BAR", "2 BAR", "4 BAR", "8 BAR" };
static const int kLenBars[] = { 1, 2, 4, 8 };

juce::AudioProcessorValueTreeState::ParameterLayout Processor::layout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "out", 1 }, "Output", NormalisableRange<float> (-30, 6, 0.1f), 0));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "curve", 1 }, "Velocity Curve", kCurves, 0));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "grid", 1 }, "Quantize Grid", kGrid, 2));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "swing", 1 }, "Swing", NormalisableRange<float> (50, 75, 0.1f), 50));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "strength", 1 }, "Quantize Strength", NormalisableRange<float> (0, 100, 1), 100));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "len", 1 }, "Loop Length", kLen, 1));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "bpm", 1 }, "BPM interno", NormalisableRange<float> (50, 220, 0.1f), 120));
    l.add (std::make_unique<AudioParameterInt> (ParameterID { "root", 1 }, "Nota del pad 1", 0, 112, 36));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "click", 1 }, "Metronomo", false));
    return l;
}

Processor::Processor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", layout())
{
    formats.registerBasicFormats();
    for (int i = 0; i < kPads; ++i) { padGlow[i] = 0; lastVel[i] = 0; hits[i] = 0; pad[i].colour = i; }
    trigs.reserve (1024);
    events.reserve (4096);
    loadDefaultKit();
}

void Processor::loadDefaultKit()
{
    // ordine MPC: riga in basso kick, snare, clap, hat chiuso
    const int order[kPads] = { 0, 1, 2, 3, 5, 6, 4, 9, 7, 8, 10, 11, 12, 13, 14, 15 };
    for (int p = 0; p < kPads; ++p) { pad[p].reset(); setSample (p, makeBuiltin (order[p])); }
    pad[3].v[CHOKE] = 1; pad[6].v[CHOKE] = 1;          // hat chiuso e aperto si strozzano
    pad[0].v[LEVEL] = 0.9f; pad[3].v[LEVEL] = 0.6f; pad[6].v[LEVEL] = 0.55f; pad[5].v[LEVEL] = 0.5f; pad[10].v[LEVEL] = 0.75f;
}

void Processor::prepareToPlay (double s, int)
{
    sr = s;
    for (auto& v : voices) { v.active = false; v.s.reset(); }
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
    hits[p] = hits[p].load() + 1; lastVel[p] = vel; lastPlayed = p;
    if (s == nullptr || s->buf.getNumSamples() < 4) return;
    auto& P = pad[p];
    const int choke = juce::roundToInt (P.get (CHOKE));
    for (auto& v : voices) if (v.active && (v.pad == p || (choke > 0 && juce::roundToInt (pad[v.pad].get (CHOKE)) == choke)) && v.age >= 0)
        if (v.pad != p || P.get (MODE) < 0.5f) v.choking = true;   // stesso pad: ritrigger morbido; choke group: taglio
    Voice* v = nullptr;
    for (auto& x : voices) if (! x.active) { v = &x; break; }
    if (v == nullptr) { int oldest = -1; for (auto& x : voices) if (x.age > oldest) { oldest = x.age; v = &x; } }
    const float vv = velCurve (vel);
    const int n = s->buf.getNumSamples();
    double a0 = P.get (START) * n, a1 = P.get (END) * n; if (a1 - a0 < 16) a1 = juce::jmin ((double) n, a0 + 16);
    a0 += (1.0 - vv) * P.get (VSTART) * 0.2 * (a1 - a0);   // velocity bassa: attacco più morbido
    const double semis = P.get (TUNE) + P.get (FINE) / 100.0 + (vv - 0.75) * P.get (VPITCH) * 4.0;
    v->active = true; v->rel = false; v->choking = false; v->pad = p; v->age = 0; v->s = s;
    v->dir = P.get (REVERSE) > 0.5f ? -1 : 1;
    v->a0 = a0; v->a1 = a1; v->pos = v->dir > 0 ? a0 : a1 - 2;
    v->inc = std::pow (2.0, semis / 12.0) * s->sr / sr;
    const float gainDb = -(1.0f - vv) * 30.0f * P.get (VLEVEL);
    const float g = P.get (LEVEL) * juce::Decibels::decibelsToGain (gainDb) * 1.1f;
    const float pan = P.get (PAN);
    v->gL = g * std::cos ((pan + 1) * juce::MathConstants<float>::pi * 0.25f) * 1.4142f;
    v->gR = g * std::sin ((pan + 1) * juce::MathConstants<float>::pi * 0.25f) * 1.4142f;
    const float atk = P.get (ATTACK);
    v->env = atk < 0.5f ? 1.0f : 0.0f; v->atkDone = atk < 0.5f; v->atkInc = atk < 0.5f ? 1.0f : (float) (1.0 / (atk * 0.001 * sr));
    const float dec = P.get (DECAY); v->hold = dec >= 3990;
    v->decCoef = (float) std::exp (-1.0 / (dec * 0.001 * sr / 4.6));
    v->relCoef = (float) std::exp (-1.0 / (P.get (RELEASE) * 0.001 * sr / 4.6));
    v->gate = P.get (MODE) > 0.5f ? 1 : -1;
    const float cut = juce::jlimit (30.0f, 20000.0f, P.get (CUTOFF) * std::pow (2.0f, (vv - 1.0f) * P.get (VFILTER) * 5.0f));
    v->ftype = juce::roundToInt (P.get (FTYPE));
    v->filt = ! (v->ftype == 0 && cut >= 19500.0f);
    v->fg = (float) std::tan (juce::MathConstants<double>::pi * juce::jmin ((double) cut, sr * 0.45) / sr);
    v->fk = 2.0f - 1.94f * P.get (RESO);
    for (int c = 0; c < 2; ++c) { v->ic1[c] = 0; v->ic2[c] = 0; }
}

void Processor::releasePad (int p) { for (auto& v : voices) if (v.active && v.pad == p && v.gate > 0) v.rel = true; }

void Processor::render (Voice& v, float& L, float& R)
{
    if (v.dir > 0 ? v.pos >= v.a1 - 2 : v.pos <= v.a0) { v.active = false; v.s.reset(); return; }
    const auto& b = v.s->buf; const int n = b.getNumSamples();
    const int i = (int) v.pos; const float f = (float) (v.pos - i);
    float out[2];
    for (int c = 0; c < 2; ++c)
    {
        const float* d = b.getReadPointer (juce::jmin (c, b.getNumChannels() - 1));
        const float y0 = d[juce::jmax (0, i - 1)], y1 = d[i], y2 = d[juce::jmin (n - 1, i + 1)], y3 = d[juce::jmin (n - 1, i + 2)];
        out[c] = y1 + 0.5f * f * (y2 - y0 + f * (2 * y0 - 5 * y1 + 4 * y2 - y3 + f * (3 * (y1 - y2) + y3 - y0)));
    }
    v.pos += v.inc * v.dir; ++v.age;
    // inviluppo
    if (v.choking) { v.env *= 0.995f; if (v.env < 0.0005f) { v.active = false; v.s.reset(); return; } }
    else if (v.rel) { v.env *= v.relCoef; if (v.env < 0.0002f) { v.active = false; v.s.reset(); return; } }
    else if (! v.atkDone) { v.env += v.atkInc; if (v.env >= 1) { v.env = 1; v.atkDone = true; } }
    else if (! v.hold) { v.env *= v.decCoef; if (v.env < 0.0002f) { v.active = false; v.s.reset(); return; } }
    // micro fade sui bordi della regione
    const double dist = v.dir > 0 ? v.a1 - v.pos : v.pos - v.a0;
    const float edge = dist < 48 ? (float) (dist / 48) : 1.0f;
    for (int c = 0; c < 2; ++c)
    {
        float x = out[c];
        if (v.filt)
        {
            const float v3 = x - v.ic2[c], v1 = (v.ic1[c] + v.fg * v3) / (1 + v.fg * (v.fg + v.fk)), v2 = v.ic2[c] + v.fg * v1;
            v.ic1[c] = 2 * v1 - v.ic1[c]; v.ic2[c] = 2 * v2 - v.ic2[c];
            x = v.ftype == 0 ? v2 : v.ftype == 1 ? x - v.fk * v1 - v2 : v1 * v.fk;
        }
        out[c] = x * v.env * edge;
    }
    L += out[0] * v.gL; R += out[1] * v.gR;
    const float lev = std::abs (out[0] * v.gL) + std::abs (out[1] * v.gR);
    if (lev > padGlow[v.pad].load (std::memory_order_relaxed)) padGlow[v.pad].store (lev, std::memory_order_relaxed);
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
    if (! l.isLocked()) return;
    if (events.size() < 8000) events.push_back ({ std::fmod (tick, (double) loopTicks()), p, vel, absTick });
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
    buffer.clear();
    trigs.clear();

    // trasporto: host se suona, altrimenti interno
    double bpm = apvts.getRawParameterValue ("bpm")->load(); bool hostPlay = false; double ppq = 0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = *b;
            hostPlay = pos->getIsPlaying();
            if (auto q = pos->getPpqPosition()) ppq = *q;
        }
    const int req = transportReq.exchange (0);
    if (req == 1) { intPlay = true; intTick = 0; }
    if (req == 2) { intPlay = false; recording = false; }
    const bool running = hostPlay || intPlay.load();
    const double tps = bpm / 60.0 * PPQ / sr;
    double t0 = hostPlay ? ppq * PPQ : intTick;
    const double t1 = t0 + N * tps;
    uiHost = hostPlay; uiRunning = running; uiBpm = bpm;
    if (running && ! wasRunning) for (auto& e : events) e.recAbs = -1e9;
    wasRunning = running;

    const int root = rootNote();
    const bool rec = recording.load() && running;
    auto noteOn = [&] (int off, int p, int vel)
    {
        trigs.push_back ({ off, p, vel, true });
        if (rec) { const double t = t0 + off * tps; record (p, vel, t, t); }
    };
    // eventi dall'interfaccia
    {
        int s1, n1, s2, n2; fifo.prepareToRead (fifo.getNumReady(), s1, n1, s2, n2);
        for (int k = 0; k < n1 + n2; ++k)
        {
            const auto& e = fifoBuf[k < n1 ? s1 + k : s2 + k - n1];
            if (e.on) noteOn (0, e.pad, e.vel); else trigs.push_back ({ 0, e.pad, 0, false });
        }
        fifo.finishedRead (n1 + n2);
    }
    // MIDI
    for (const auto m : midi)
    {
        const auto msg = m.getMessage(); const int p = msg.getNoteNumber() - root;
        if (! msg.isNoteOnOrOff() || p < 0 || p >= kPads) continue;
        if (msg.isNoteOn()) noteOn (m.samplePosition, p, msg.getVelocity());
        else trigs.push_back ({ m.samplePosition, p, 0, false });
    }
    // riproduzione del pattern
    if (running)
    {
        const double L = loopTicks();
        const juce::SpinLock::ScopedTryLockType l (recLock);
        if (l.isLocked())
            for (auto& e : events)
            {
                const double q = quantize (e.tick);
                const double base = std::floor (t0 / L) * L;
                for (double b = base; b <= t1; b += L)
                {
                    const double a = b + q;
                    if (a < t0 || a >= t1) continue;
                    if (e.recAbs > -1e8 && a >= e.recAbs - 1 && a - e.recAbs < L * 0.5) { e.recAbs = -1e9; continue; }   // appena suonato dal vivo
                    trigs.push_back ({ juce::jlimit (0, N - 1, (int) ((a - t0) / tps)), e.pad, e.vel, true });
                }
            }
        // metronomo
        if (apvts.getRawParameterValue ("click")->load() > 0.5f || recording.load())
            for (double b = std::ceil (t0 / PPQ) * PPQ; b < t1; b += PPQ)
                trigs.push_back ({ juce::jlimit (0, N - 1, (int) ((b - t0) / tps)), std::fmod (b, (double) kBar) < 1 ? -2 : -1, 0, true });
    }
    std::stable_sort (trigs.begin(), trigs.end(), [] (const Trig& a, const Trig& b) { return a.off < b.off; });
    if (! hostPlay && intPlay.load()) intTick += N * tps;
    uiTick = running ? t1 : 0.0;

    float* L = buffer.getWritePointer (0); float* R = buffer.getWritePointer (1);
    const float outG = juce::Decibels::decibelsToGain (apvts.getRawParameterValue ("out")->load());
    size_t ti = 0; float pk[2] = { 0, 0 };
    for (int i = 0; i < N; ++i)
    {
        while (ti < trigs.size() && trigs[ti].off <= i)
        {
            const auto& t = trigs[ti++];
            if (t.pad < 0) { clickEnv = 0.35f; clickPh = 0; clickF = t.pad == -2 ? 2000.0f : 1400.0f; }
            else if (t.on) startVoice (t.pad, t.vel); else releasePad (t.pad);
        }
        float l = 0, r = 0;
        for (auto& v : voices) if (v.active) render (v, l, r);
        if (clickEnv > 0.0005f) { const float c = std::sin (clickPh) * clickEnv; clickPh += (float) (juce::MathConstants<double>::twoPi * clickF / sr); clickEnv *= 0.997f; l += c; r += c; }
        l *= outG; r *= outG;
        L[i] = l; R[i] = r; pk[0] = juce::jmax (pk[0], std::abs (l)); pk[1] = juce::jmax (pk[1], std::abs (r));
    }
    meterL = juce::jmax (pk[0], meterL.load() * 0.9f); meterR = juce::jmax (pk[1], meterR.load() * 0.9f);
}

bool Processor::loadFile (const juce::File& f, int p)
{
    std::unique_ptr<juce::AudioFormatReader> rd (formats.createReaderFor (f));
    if (rd == nullptr || rd->lengthInSamples < 4) return false;
    const int len = (int) juce::jmin<juce::int64> (rd->lengthInSamples, (juce::int64) (rd->sampleRate * 30));
    auto s = std::make_shared<SampleData>();
    s->buf.setSize (2, len); rd->read (&s->buf, 0, len, 0, true, true);
    if (rd->numChannels == 1) s->buf.copyFrom (1, 0, s->buf, 0, 0, len);
    s->sr = rd->sampleRate; s->name = f.getFileNameWithoutExtension(); s->path = f.getFullPathName();
    pad[p].v[START] = 0; pad[p].v[END] = 1;
    setSample (p, s);
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
    auto st = apvts.copyState(); juce::XmlElement x ("JPAD");
    x.addChildElement (st.createXml().release());
    auto* pads = x.createNewChildElement ("PADS");
    for (int p = 0; p < kPads; ++p)
    {
        auto* e = pads->createNewChildElement ("P");
        for (int i = 0; i < kNumPadParams; ++i) e->setAttribute (kPInfo[i].id, pad[p].get (i));
        e->setAttribute ("colour", pad[p].colour.load());
        if (auto s = getSample (p)) e->setAttribute ("path", s->path);
    }
    juce::String ev; { const juce::SpinLock::ScopedLockType l (recLock); for (auto& e : events) ev << juce::String (e.tick, 2) << "," << e.pad << "," << e.vel << ";"; }
    x.createNewChildElement ("PATTERN")->addTextElement (ev);
    copyXmlToBinary (x, dest);
}

void Processor::setStateInformation (const void* data, int size)
{
    auto x = getXmlFromBinary (data, size); if (x == nullptr || ! x->hasTagName ("JPAD")) return;
    if (auto* ps = x->getChildByName (apvts.state.getType())) apvts.replaceState (juce::ValueTree::fromXml (*ps));
    if (auto* pads = x->getChildByName ("PADS"))
    {
        int p = 0;
        for (auto* e : pads->getChildIterator())
        {
            if (p >= kPads) break;
            for (int i = 0; i < kNumPadParams; ++i) pad[p].v[i] = (float) e->getDoubleAttribute (kPInfo[i].id, kPInfo[i].def);
            pad[p].colour = e->getIntAttribute ("colour", p);
            const auto path = e->getStringAttribute ("path");
            if (path.startsWith ("builtin:")) setSample (p, makeBuiltin (path.fromFirstOccurrenceOf (":", false, false).getIntValue()));
            else if (path.isNotEmpty() && juce::File (path).existsAsFile())
            {
                float keep[kNumPadParams]; for (int i = 0; i < kNumPadParams; ++i) keep[i] = pad[p].get (i);
                loadFile (juce::File (path), p);
                for (int i = 0; i < kNumPadParams; ++i) pad[p].v[i] = keep[i];
            }
            else setSample (p, nullptr);
            ++p;
        }
    }
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
