#include "FluxProcessor.h"
#include "FluxEditor.h"

namespace flux
{
juce::AudioProcessorValueTreeState::ParameterLayout Processor::layout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    auto F = [&] (const char* id, const char* nm, float d) { l.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, nm, NormalisableRange<float> (0, 1), d)); };
    F ("density", "Density", 0.5f); F ("energy", "Energy", 0.5f); F ("movement", "Movement", 0.3f); F ("space", "Space", 0.0f);
    StringArray secs; for (int s = 0; s < NSEC; ++s) secs.add (secName (s));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "section", 1 }, "Sezione", secs, 1));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "drum1", 1 }, "Batteria sul canale 1", false));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "play", 1 }, "Anteprima", false));
    return l;
}
Processor::Processor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)), apvts (*this, nullptr, "PARAMS", layout())
{
    for (auto& h : uiHits) h = 0;
    snapPtr = std::make_shared<const Snap> (makeStarter (TRAP, 9, true));
    // sezioni di partenza: intro rada, strofa media, hook piena, bridge vuoto, outro che si spegne
    const Macros sm[NSEC] = { { 0.35f, 0.4f, 0.2f, 0.55f }, { 0.5f, 0.5f, 0.3f, 0.1f }, { 0.7f, 0.8f, 0.5f, 0.0f }, { 0.4f, 0.45f, 0.4f, 0.75f }, { 0.3f, 0.35f, 0.1f, 0.85f } };
    for (int s = 0; s < NSEC; ++s) sections[(size_t) s].m = sm[s];
    for (auto id : { "density", "energy", "movement", "space", "section" }) apvts.addParameterListener (id, this);
}
Processor::~Processor() { cancelPendingUpdate(); for (auto id : { "density", "energy", "movement", "space", "section" }) apvts.removeParameterListener (id, this); }
void Processor::prepareToPlay (double s, int) { sr = s; np = 0; cacheBar[0] = cacheBar[1] = -9999; lastEnd = -1; }

void Processor::parameterChanged (const juce::String& id, float)
{
    if (id == "section") { triggerAsyncUpdate(); return; }
    if (loadingSec) return;
    const int s = juce::roundToInt (v ("section")); sections[(size_t) s].m = macros();   // le macro modificano la sezione attiva
}
void Processor::handleAsyncUpdate()
{
    const int s = juce::roundToInt (v ("section")); if (s == lastSection) return; lastSection = s;
    loadingSec = true; const auto& m = sections[(size_t) s].m;
    setV ("density", m.density); setV ("energy", m.energy); setV ("movement", m.movement); setV ("space", m.space);
    loadingSec = false;
    edit ([this, s] (Snap& S) { S.mute = sections[(size_t) s].mute; });
}
void Processor::selectSection (int s) { setV ("section", (float) s); handleAsyncUpdate(); }
void Processor::loadStarter (int s, int key, bool minor)
{
    starter = s; auto S = std::make_shared<Snap> (makeStarter (s, key, minor)); std::atomic_store (&snapPtr, std::shared_ptr<const Snap> (S)); allOff = true;
}
void Processor::storeMem (int i) { auto S = snap(); auto& m = mem[(size_t) i]; m.used = true; m.seed = seed.load(); m.m = macros(); m.rules = S->rules; m.mute = S->mute; }
void Processor::recallMem (int i)
{
    const auto& m = mem[(size_t) i]; if (! m.used) return;
    seed = m.seed; setV ("density", m.m.density); setV ("energy", m.m.energy); setV ("movement", m.m.movement); setV ("space", m.m.space);
    edit ([&m] (Snap& S) { S.rules = m.rules; S.mute = m.mute; });
}
bool Processor::importLane (int lane, const juce::File& f)
{
    juce::FileInputStream in (f); if (! in.openedOk()) return false;
    juce::MidiFile mf; if (! mf.readFrom (in)) return false;
    const double tpq = mf.getTimeFormat() > 0 ? mf.getTimeFormat() : 960;
    std::vector<Note> notes;
    for (int t = 0; t < mf.getNumTracks(); ++t)
    {
        juce::MidiMessageSequence seq (*mf.getTrack (t)); seq.updateMatchedPairs();
        for (auto* e : seq)
            if (e->message.isNoteOn())
            {
                const float st = (float) (e->message.getTimeStamp() / tpq); if (st >= BARS * BPB) continue;
                const float en = e->noteOffObject != nullptr ? (float) (e->noteOffObject->message.getTimeStamp() / tpq) : st + 0.25f;
                notes.push_back ({ st, juce::jmax (0.05f, en - st), e->message.getNoteNumber(), e->message.getVelocity() });
            }
        if (! notes.empty()) break;
    }
    if (notes.empty()) return false;
    // pattern piu' corto di 4 battute: si ripete
    float endB = 0; for (auto& n : notes) endB = juce::jmax (endB, n.t + n.len); const int bars = juce::jlimit (1, BARS, (int) std::ceil (endB / BPB - 0.01f));
    std::vector<Note> full; for (int rep = 0; rep * bars < BARS; ++rep) for (auto n : notes) { n.t += (float) (rep * bars * BPB); if (n.t < BARS * BPB) full.push_back (n); }
    edit ([lane, &full] (Snap& S) { S.lane[(size_t) lane] = full; });
    return true;
}
juce::File Processor::exportMidi (int bars)
{
    auto S = snap(); const auto M = macros(); const double ppqT = 960;
    juce::MidiFile mf; mf.setTicksPerQuarterNote ((int) ppqT);
    juce::MidiMessageSequence tempo; tempo.addEvent (juce::MidiMessage::tempoMetaEvent ((int) (60000000.0 / juce::jmax (40.0, uiBpm.load()))), 0); mf.addTrack (tempo);
    for (int l = 0; l < NL; ++l)
    {
        juce::MidiMessageSequence seq; auto nm = juce::MidiMessage::textMetaEvent (3, laneName (l)); seq.addEvent (nm, 0);
        BarEvents be;
        for (int b = 0; b < bars; ++b)
        {
            renderBar (*S, M, seed.load(), b, be);
            for (int k = 0; k < be.n; ++k) if (be.ev[k].lane == l)
            {
                const double t = (b * BPB + be.ev[k].t) * ppqT;
                seq.addEvent (juce::MidiMessage::noteOn (1, be.ev[k].note, (juce::uint8) be.ev[k].vel), t);
                seq.addEvent (juce::MidiMessage::noteOff (1, be.ev[k].note), t + juce::jmax (10.0, be.ev[k].len * ppqT - 5));
            }
        }
        seq.updateMatchedPairs(); mf.addTrack (seq);
    }
    auto dir = juce::File::getSpecialLocation (juce::File::userMusicDirectory).getChildFile ("Jackpot").getChildFile ("Flux"); dir.createDirectory();
    auto f = dir.getChildFile ("Flux " + juce::String (starterName (starter)) + " seme " + juce::String ((int) (seed.load() % 10000)) + ".mid");
    f.deleteFile(); juce::FileOutputStream os (f); if (os.openedOk()) mf.writeTo (os); return f;
}

void Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    buffer.clear(); const int N = buffer.getNumSamples();
    juce::MidiBuffer in; in.swapWith (midi);   // l'ingresso MIDI per ora non serve: esce solo il beat di FLUX
    double p0 = 0, bpm = 120; bool playing = false;
    if (auto* ph = getPlayHead()) if (auto pos = ph->getPosition()) { playing = pos->getIsPlaying(); if (auto b = pos->getBpm()) bpm = *b; if (auto q = pos->getPpqPosition()) p0 = *q; }
    const bool preview = ! playing && v ("play") > 0.5f;
    if (preview) { p0 = ownPpq; ownPpq += N * bpm / 60.0 / sr; } else if (! playing) ownPpq = 0;
    uiBpm = bpm;
    const bool drum1 = v ("drum1") > 0.5f;
    auto chOf = [drum1] (int l) { return drum1 && isDrum (l) ? 1 : l + 1; };
    auto flushAll = [&] (int at) { for (int i = 0; i < np; ++i) midi.addEvent (juce::MidiMessage::noteOff (pend[i].ch, pend[i].note), at); np = 0; for (int c = 1; c <= 16; ++c) midi.addEvent (juce::MidiMessage::allNotesOff (c), at); };
    if (allOff.exchange (false)) { flushAll (0); cacheBar[0] = cacheBar[1] = -9999; }
    if (! playing && ! preview) { if (lastEnd >= 0) flushAll (0); lastEnd = -1; uiPlaying = false; return; }
    uiPlaying = true; uiPpq = p0;
    const double p1 = p0 + N * bpm / 60.0 / sr, spp = sr * 60.0 / bpm;   // campioni per battito
    if (lastEnd >= 0 && (p0 < lastEnd - 0.01 || p0 > lastEnd + 0.25)) flushAll (0);   // salto della testina
    lastEnd = p1;
    // note-off dovuti
    for (int i = 0; i < np;)
    {
        if (pend[i].end < p1) { midi.addEvent (juce::MidiMessage::noteOff (pend[i].ch, pend[i].note), juce::jlimit (0, N - 1, (int) ((pend[i].end - p0) * spp))); pend[i] = pend[--np]; }
        else ++i;
    }
    auto S = snap(); Macros M = macros(); juce::uint32 sd = seed.load();
    if (abOriginal.load()) { M = { 0.5f, 0.5f, 0.0f, 0.0f }; sd = 1; }
    const juce::uint32 key = hsh ((juce::uint32) (size_t) S.get(), sd, (juce::uint32) (M.density * 1000) ^ ((juce::uint32) (M.energy * 1000) << 10), (juce::uint32) (M.movement * 1000) ^ ((juce::uint32) (M.space * 1000) << 10));
    const int barA = (int) std::floor (p0 / BPB), barB = (int) std::floor ((p1 - 1e-9) / BPB);
    for (int bar = barA; bar <= barB; ++bar)
    {
        const int ci = bar & 1;
        if (cacheBar[ci] != bar || cacheKey[ci] != key) { renderBar (*S, M, sd, bar, cache[ci]); cacheBar[ci] = bar; cacheKey[ci] = key; }
        const auto& be = cache[ci];
        for (int k = 0; k < be.n; ++k)
        {
            const auto& e = be.ev[k]; const double t = bar * (double) BPB + e.t;
            if (t < p0 || t >= p1) continue;
            const int off = juce::jlimit (0, N - 1, (int) ((t - p0) * spp)), ch = chOf (e.lane);
            midi.addEvent (juce::MidiMessage::noteOn (ch, e.note, (juce::uint8) e.vel), off);
            if (np < 512) pend[np++] = { t + juce::jmax (0.02f, e.len), ch, e.note };
            uiHits[e.lane] = uiHits[e.lane].load() + 1;
        }
    }
}

void Processor::getStateInformation (juce::MemoryBlock& d)
{
    auto st = apvts.copyState(); auto S = snap();
    juce::ValueTree fl ("FLUX"); fl.setProperty ("seed", (int) seed.load(), nullptr); fl.setProperty ("key", S->key, nullptr); fl.setProperty ("minor", S->minor, nullptr); fl.setProperty ("starter", starter, nullptr);
    for (int l = 0; l < NL; ++l)
    {
        juce::ValueTree ln ("LANE"); ln.setProperty ("var", S->var[(size_t) l], nullptr); ln.setProperty ("mute", S->mute[(size_t) l], nullptr);
        juce::StringArray ns; for (auto& n : S->lane[(size_t) l]) ns.add (juce::String (n.t, 4) + ":" + juce::String (n.len, 4) + ":" + juce::String (n.note) + ":" + juce::String (n.vel));
        ln.setProperty ("notes", ns.joinIntoString (";"), nullptr); fl.appendChild (ln, nullptr);
    }
    for (auto& r : S->rules) { juce::ValueTree rt ("RULE"); rt.setProperty ("f", r.from, nullptr); rt.setProperty ("t", r.to, nullptr); rt.setProperty ("y", r.type, nullptr); rt.setProperty ("on", r.on, nullptr); fl.appendChild (rt, nullptr); }
    for (int s = 0; s < NSEC; ++s) { juce::ValueTree sc ("SEC"); const auto& m = sections[(size_t) s].m; sc.setProperty ("m", juce::String (m.density) + ";" + juce::String (m.energy) + ";" + juce::String (m.movement) + ";" + juce::String (m.space), nullptr); fl.appendChild (sc, nullptr); }
    st.appendChild (fl, nullptr);
    if (auto x = st.createXml()) copyXmlToBinary (*x, d);
}
void Processor::setStateInformation (const void* data, int size)
{
    auto x = getXmlFromBinary (data, size); if (! x) return;
    auto t = juce::ValueTree::fromXml (*x); if (! t.isValid()) return;
    auto fl = t.getChildWithName ("FLUX"); if (fl.isValid()) t.removeChild (fl, nullptr);
    apvts.replaceState (t);
    if (! fl.isValid()) return;
    seed = (juce::uint32) (int) fl.getProperty ("seed", 1); starter = fl.getProperty ("starter", 0);
    auto S = std::make_shared<Snap>(); S->key = fl.getProperty ("key", 9); S->minor = fl.getProperty ("minor", true);
    int l = 0, s = 0;
    for (auto c : fl)
    {
        if (c.hasType ("LANE") && l < NL)
        {
            S->var[(size_t) l] = c.getProperty ("var", true); S->mute[(size_t) l] = c.getProperty ("mute", false);
            for (auto& tok : juce::StringArray::fromTokens (c.getProperty ("notes").toString(), ";", "")) { auto p = juce::StringArray::fromTokens (tok, ":", ""); if (p.size() == 4) S->lane[(size_t) l].push_back ({ p[0].getFloatValue(), p[1].getFloatValue(), p[2].getIntValue(), p[3].getIntValue() }); }
            ++l;
        }
        else if (c.hasType ("RULE")) S->rules.push_back ({ (int) c.getProperty ("f"), (int) c.getProperty ("t"), (int) c.getProperty ("y"), (bool) c.getProperty ("on") });
        else if (c.hasType ("SEC") && s < NSEC) { auto p = juce::StringArray::fromTokens (c.getProperty ("m").toString(), ";", ""); if (p.size() == 4) sections[(size_t) s].m = { p[0].getFloatValue(), p[1].getFloatValue(), p[2].getFloatValue(), p[3].getFloatValue() }; ++s; }
    }
    std::atomic_store (&snapPtr, std::shared_ptr<const Snap> (S)); lastSection = juce::roundToInt (v ("section"));
}
juce::AudioProcessorEditor* Processor::createEditor() { return new Editor (*this); }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new flux::Processor(); }
