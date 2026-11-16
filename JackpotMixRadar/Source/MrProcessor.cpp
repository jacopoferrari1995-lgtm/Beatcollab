#include "MrProcessor.h"
#include "MrEditor.h"

namespace mr
{
juce::AudioProcessorValueTreeState::ParameterLayout Processor::layout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "mode", 1 }, "Modo", StringArray { "SENSORE", "RADAR" }, 0));
    StringArray roles; for (int r = 0; r < kRoles; ++r) roles.add (roleName (r));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "role", 1 }, "Ruolo", roles, OTHER));
    return l;
}
static juce::int64 newId() { static std::atomic<juce::int64> c { 1 }; return (juce::int64) juce::Time::currentTimeMillis() * 1000 + c.fetch_add (1); }
Processor::Processor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true).withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", layout()), myId (newId())
{
    for (int b = 0; b < NB; ++b) { mBands[b] = -120; sBands[b] = -120; }
    startTimerHz (20);
}
Processor::~Processor() { stopTimer(); if (slot >= 0) { hub().s[slot].stamp = 0; hub().s[slot].owner = 0; } }
void Processor::prepareToPlay (double s, int)
{
    sr = s; bank.prepare (sr); hop = (int) (sr * 0.021); hopN = 0;
    // K-weighting (BS.1770): shelf alto + passa-alto
    for (int c = 0; c < 2; ++c)
    {
        kw1[c].setCoefficients (juce::IIRCoefficients::makeHighShelf (sr, 1500.0, 0.707, juce::Decibels::decibelsToGain (4.0f)));
        kw2[c].setCoefficients (juce::IIRCoefficients::makeHighPass (sr, 38.0, 0.5)); kw1[c].reset(); kw2[c].reset();
    }
    kBlocks.assign (30, 0.0); kW = 0; kAcc = 0; kN = 0;
}

void Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals nd; midi.clear();
    const int N = buffer.getNumSamples(); const float* L = buffer.getReadPointer (0); const float* R = buffer.getReadPointer (1);
    const bool isRadar = radar();
    if (isRadar)
    {
        hub().radarStamp = nowMs();
        if (auto* ph = getPlayHead()) if (auto p = ph->getPosition())
        {
            playing = p->getIsPlaying(); if (auto q = p->getPpqPosition()) ppq = *q; if (auto b = p->getBpm()) bpm = *b;
            if (auto ts = p->getTimeSignature()) beatsPerBar = juce::jmax (1, ts->numerator * 4 / juce::jmax (1, ts->denominator));
        }
    }
    else
    {
        if (slot < 0 || hub().s[slot].owner.load() != myId) { slot = claim (myId); if (slot >= 0 && trackName.isNotEmpty()) hub().s[slot].setName (trackName); }
    }
    const float a = 1.0f - std::exp (-6.2832f * 120.0f / (float) sr);
    for (int i = 0; i < N; ++i)
    {
        const float m = 0.5f * (L[i] + R[i]), s = 0.5f * (L[i] - R[i]);
        bank.push (m);
        lpM1 += a * (m - lpM1); lpM2 += a * (lpM1 - lpM2); lpS1 += a * (s - lpS1); lpS2 += a * (lpS1 - lpS2);
        eMid += (double) lpM2 * lpM2; eSide += (double) lpS2 * lpS2;
        pk = juce::jmax (pk, std::abs (L[i]), std::abs (R[i])); rms += (double) m * m;
        if (isRadar)
        {
            for (int c = 0; c < 2; ++c) { const float x = kw2[c].processSingleSampleRaw (kw1[c].processSingleSampleRaw (c ? R[i] : L[i])); kAcc += (double) x * x; }
            cLR += (double) L[i] * R[i]; cLL += (double) L[i] * L[i]; cRR += (double) R[i] * R[i];
            if (++kN >= (int) (sr * 0.1))
            {
                kBlocks[(size_t) kW] = kAcc / kN; kW = (kW + 1) % 30; kAcc = 0; kN = 0;
                double m4 = 0, s30 = 0; for (int k = 0; k < 4; ++k) m4 += kBlocks[(size_t) ((kW - 1 - k + 30) % 30)]; for (auto x : kBlocks) s30 += x;
                lufsM = (float) (-0.691 + 10 * std::log10 (m4 / 4 + 1e-12)); lufsS = (float) (-0.691 + 10 * std::log10 (s30 / 30 + 1e-12));
                corr = (float) (cLR / std::sqrt (cLL * cRR + 1e-12)); cLR = cLL = cRR = 0;
            }
        }
        if (++hopN >= hop)
        {
            float bd[NB]; bank.take (bd);
            const float lvl = (float) (10 * std::log10 (rms / hopN + 1e-12)), pkDb = juce::Decibels::gainToDecibels (pk + 1e-9f);
            const float side = eMid > 1e-10 ? (float) (10 * std::log10 (eSide / eMid + 1e-12)) : -100.0f;
            crestS += 0.2f * ((pkDb - lvl) - crestS);
            if (isRadar)
            {
                double sub = 0, mid = 0; for (int b = 0; b < NB; ++b) { mBands[b] = mBands[b].load() * 0.6f + 0.4f * bd[b]; const double e = std::pow (10.0, bd[b] / 10); if (bandHz (b) < 100) sub += e; else if (bandHz (b) > 300 && bandHz (b) < 3000) mid += e; }
                if (lvl > -50) lowMid = lowMid.load() * 0.9f + 0.1f * (float) (10 * std::log10 ((sub + 1e-12) / (mid + 1e-12)));
                mPeak = juce::jmax (pkDb, mPeak.load() - 0.5f);
            }
            else if (slot >= 0)
            {
                auto& S = hub().s[slot];
                for (int b = 0; b < NB; ++b) { S.bands[b] = S.bands[b].load() * 0.5f + 0.5f * bd[b]; sBands[b] = S.bands[b].load(); }
                S.lvl = lvl; S.peak = juce::jmax (pkDb, S.peak.load() - 0.3f); S.sideLo = S.sideLo.load() * 0.9f + 0.1f * side; S.crest = crestS;
                S.role = juce::roundToInt (v ("role")); S.stamp = nowMs();
            }
            hopN = 0; rms = 0; pk = 0; eMid = eSide = 0;
        }
    }
}

// ------------------------------------------------------------------------------------------------ il radar: chi crea il problema e dove
void Processor::addIssue (int type, const juce::String& key, const juce::String& who, const juce::String& what, const juce::String& fix, float hz, float sev, int bar)
{
    const juce::ScopedLock l (issueLock);
    auto& is = issues[juce::String (type) + "|" + key];
    is.type = type; is.who = who; is.what = what; is.fix = fix; is.hz = hz; is.sev = juce::jmax (is.sev, sev); ++is.count; is.last = nowMs();
    if (is.bars.empty() || is.bars.back() != bar) { if (is.bars.size() < 400) is.bars.push_back (bar); }
    barFlags[bar] |= 1 << type;
}
void Processor::evaluate()
{
    if (! radar() || ! playing.load()) return;
    const int bpb = beatsPerBar.load(); const double q = ppq.load(); const int bar = (int) std::floor (q / bpb) + 1, beatKey = (int) std::floor (q);
    lastBar = bar;
    auto& H = hub();
    struct T { int i; int role; juce::String name; float bands[NB]; float lvl, peak, side, crest; };
    std::vector<T> tr;
    for (int i = 0; i < MAXS; ++i)
    {
        auto& S = H.s[i]; if (! S.alive()) continue;
        T t; t.i = i; t.role = S.role.load(); t.name = S.getName(); if (t.name.isEmpty()) t.name = roleName (t.role);
        for (int b = 0; b < NB; ++b) t.bands[b] = S.bands[b].load(); t.lvl = S.lvl.load(); t.peak = S.peak.load(); t.side = S.sideLo.load(); t.crest = S.crest.load();
        tr.push_back (t);
    }
    auto once = [&] (int type) { if (lastBeatKey[type] == beatKey + 1) return false; lastBeatKey[type] = beatKey + 1; return true; };
    // 1) cassa e 808 / basso nella stessa banda bassa nello stesso momento
    for (auto& k : tr) if (k.role == KICK && k.lvl > -40)
        for (auto& bs : tr) if (bs.role == BASS && bs.lvl > -40)
        {
            int bestB = -1; float bestV = -200;
            for (int b = 0; b < NB && bandHz (b) < 150; ++b) { const float lo = juce::jmin (k.bands[b], bs.bands[b]); if (k.bands[b] > -36 && bs.bands[b] > -36 && std::abs (k.bands[b] - bs.bands[b]) < 8 && lo > bestV) { bestV = lo; bestB = b; } }
            if (bestB >= 0 && once (KICK808))
                addIssue (KICK808, k.name + "+" + bs.name, k.name + " e " + bs.name, "si coprono a " + juce::String (juce::roundToInt (bandHz (bestB))) + " Hz",
                          juce::String::fromUTF8 ("Ducking dell'808 sulla cassa (808 Studio, DUCKING 30-50%) oppure cassa e 808 su frequenze diverse."), bandHz (bestB), 2, bar);
        }
    // 2) voce coperta nella zona delle parole
    for (auto& vo : tr) if (vo.role == VOICE && vo.lvl > -40)
    {
        int hits = 0; float worst = 0; int culprit = -1; float cE = -200;
        for (int b = 0; b < NB; ++b)
        {
            const float hz = bandHz (b); if (hz < 1000 || hz > 5500) continue;
            double others = 0; for (auto& o : tr) if (o.role != VOICE) others += std::pow (10.0, o.bands[b] / 10);
            const float oDb = (float) (10 * std::log10 (others + 1e-12));
            if (oDb > vo.bands[b] - 3) { ++hits; worst = juce::jmax (worst, oDb - vo.bands[b] + 3); for (int j = 0; j < (int) tr.size(); ++j) if (tr[(size_t) j].role != VOICE && tr[(size_t) j].bands[b] > cE) { cE = tr[(size_t) j].bands[b]; culprit = j; } }
        }
        if (hits >= 2 && culprit >= 0 && once (VOICEMASK))
            addIssue (VOICEMASK, vo.name + "+" + tr[(size_t) culprit].name, tr[(size_t) culprit].name + juce::String::fromUTF8 (" copre ") + vo.name, juce::String::fromUTF8 ("nella zona delle parole (1-5 kHz)"),
                      juce::String::fromUTF8 ("Vocal Radar sul beat, oppure abbassa ") + tr[(size_t) culprit].name + " di 2-3 dB tra 2 e 4 kHz.", 2500, juce::jmin (3.0f, 1 + worst / 6), bar);
    }
    // 3) troppi bassi sul master
    if (lowMid.load() > 8 && once (LOWEND))
    {
        juce::String who = "il mix"; float best = -200;
        for (auto& t : tr) { double e = 0; for (int b = 0; b < NB && bandHz (b) < 100; ++b) e += std::pow (10.0, t.bands[b] / 10); const float d = (float) (10 * std::log10 (e + 1e-12)); if (d > best) { best = d; who = t.name; } }
        addIssue (LOWEND, who, who, juce::String::fromUTF8 ("sotto i 100 Hz e' ") + juce::String (lowMid.load(), 1) + " dB sopra i medi", juce::String::fromUTF8 ("Abbassa i sub di ") + who + " di 2-3 dB o taglia sotto i 30 Hz.", 60, 1, bar);
    }
    // 4) bassi larghi in stereo
    for (auto& t : tr) if (t.lvl > -40 && t.side > -12 && (t.role == BASS || t.role == KICK || t.role == PAD || t.role == OTHER || t.role == MELODY))
        if (once (WIDELOW)) addIssue (WIDELOW, t.name, t.name, juce::String::fromUTF8 ("ha i bassi (sotto 120 Hz) larghi in stereo"), juce::String::fromUTF8 ("Mono sotto 120 Hz (808 Studio: MONO SUB) su ") + t.name + ".", 80, 1, bar);
    // 5) picchi
    for (auto& t : tr) if (t.peak > -1.0f && t.crest > 18 && once (PEAKS))
        addIssue (PEAKS, t.name, t.name, juce::String::fromUTF8 ("picchi a ") + juce::String (t.peak, 1) + " dB con transienti molto piu' alti del corpo", juce::String::fromUTF8 ("Clipper o compressore veloce su ") + t.name + juce::String::fromUTF8 (", oppure abbassa il volume del clip."), 0, 1, bar);
    if (mPeak.load() > -0.3f && once (PEAKS)) addIssue (PEAKS, "master", "Master", juce::String::fromUTF8 ("arriva a 0 dB: rischio clipping"), juce::String::fromUTF8 ("Abbassa il master di qualche dB o metti un limiter alla fine."), 0, 2, bar);
    // 6) loudness
    if (lufsS.load() > -6.5f && once (LOUD)) addIssue (LOUD, "alto", "Master", "LUFS breve a " + juce::String (lufsS.load(), 1) + juce::String::fromUTF8 (": molto schiacciato"), juce::String::fromUTF8 ("Per Spotify -14, per un beat da vendere -9/-8: oltre -7 perdi i colpi."), 0, 1, bar);
}
void Processor::timerCallback() { evaluate(); }

void Processor::getStateInformation (juce::MemoryBlock& d) { if (auto x = apvts.copyState().createXml()) copyXmlToBinary (*x, d); }
void Processor::setStateInformation (const void* data, int size) { if (auto x = getXmlFromBinary (data, size)) { auto t = juce::ValueTree::fromXml (*x); if (t.isValid()) apvts.replaceState (t); } }
juce::AudioProcessorEditor* Processor::createEditor() { return new Editor (*this); }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new mr::Processor(); }
