#include <juce_audio_formats/juce_audio_formats.h>
#include "MixProcessor.h"
#include "MixEditor.h"

namespace jmx
{
const std::vector<Target>& targets()
{
    // LUFS short-term consigliati e picco massimo, sulla traccia singola dentro un mix (o sul master per gli ultimi)
    static const std::vector<Target> t = {
        { "VOCE LEAD", -18, -14, -6, "Voce principale a -18/-14 LUFS: lascia spazio al bus e al master." },
        { "VOCI DI SUPPORTO", -24, -19, -9, "Doppie, cori e ad-lib: 5-6 dB sotto la lead." },
        { "KICK", -16, -12, -4, "Cassa: picco a -4 lascia spazio all'808 sotto." },
        { "808 / BASSO", -16, -12, -4, "808 e basso: stessa zona della cassa, mai entrambi a picco insieme." },
        { "RULLANTE / CLAP", -18, -14, -5, "Rullante e clap poco sotto la cassa." },
        { "HI-HAT / PERC", -26, -20, -10, "Hat e percussioni: si sentono anche bassi." },
        { "DRUM BUS", -14, -10, -3, "Batteria intera sul bus." },
        { "CHITARRA", -22, -17, -8, "Chitarre: sotto la voce." },
        { "PIANO / KEYS", -22, -17, -8, "Piano, rhodes, organo." },
        { "SYNTH / PAD", -24, -18, -8, "Pad e synth di accompagnamento." },
        { "SAMPLE / LOOP", -20, -15, -6, "Il campione principale di un beat." },
        { "MIX BUS", -14, -10, -3, "Mix intero prima del master: niente sopra -3 dBFS." },
        { "MASTER STREAMING", -14, -10, -1, "Master per streaming: -14/-10 LUFS short-term, true peak sotto -1." },
        { "MASTER LOUD (TRAP/DRILL)", -9, -6, -1, "Master alto per trap e drill: -9/-6 LUFS, true peak sotto -1." } };
    return t;
}

juce::AudioProcessorValueTreeState::ParameterLayout Processor::layout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    auto F = [&l] (const String& id, const String& n, NormalisableRange<float> r, float d) { l.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, n, r, d)); };
    auto B = [&l] (const String& id, const String& n, bool d) { l.add (std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, n, d)); };
    auto C = [&l] (const String& id, const String& n, StringArray s, int d) { l.add (std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, n, s, d)); };
    auto I = [&l] (const String& id, const String& n, int a, int b, int d) { l.add (std::make_unique<AudioParameterInt> (ParameterID { id, 1 }, n, a, b, d)); };
    auto skew = [] (float a, float b, float c) { NormalisableRange<float> r (a, b); r.setSkewForCentre (c); return r; };
    F ("in", "Input", { -24, 24, 0.01f }, 0); F ("out", "Output", { -24, 24, 0.01f }, 0);
    B ("live", "Live (niente latenza spettrale)", false); B ("cmp", "Confronta a pari volume", false);
    StringArray tn; for (auto& t : targets()) tn.add (t.name); C ("target", "Target", tn, 0);
    I ("n", "Moduli nel rack", 0, kSlots, 0);
    B ("eq_on", "EQ on", true); I ("eq_pos", "Posizione EQ", 0, kSlots, 0);
    F ("eq_att", "EQ dinamica attacco", skew (0.5f, 100, 8), 8); F ("eq_rel", "EQ dinamica rilascio", skew (10, 1000, 120), 120);
    StringArray types; for (auto& m : mods()) types.add (m.name);
    for (int s = 0; s < kSlots; ++s)
    {
        const String n = "Slot " + String (s + 1);
        C (sid (s, "type"), n + " modulo", types, 0); B (sid (s, "byp"), n + " bypass", false); F (sid (s, "mix"), n + " mix", { 0, 1 }, 1);
        for (int i = 0; i < kP; ++i) F (sid (s, ("p" + String (i + 1)).toRawUTF8()), n + " comando " + String (i + 1), { 0, 1 }, 0);
    }
    const float defF[kBands] = { 80, 200, 400, 800, 1500, 2500, 4000, 6000, 9000, 12000, 30, 16000 };
    for (int b = 0; b < kBands; ++b)
    {
        const String n = "Banda " + String (b + 1);
        B (bid (b, "on"), n + " on", false); C (bid (b, "type"), n + " tipo", bandTypeNames(), 0);
        F (bid (b, "f"), n + " freq", skew (10, 22000, 1000), defF[b]); F (bid (b, "g"), n + " gain", { -30, 30, 0.01f }, 0); F (bid (b, "q"), n + " Q", skew (0.1f, 30, 1), 1);
        C (bid (b, "slope"), n + " pendenza", slopeNames(), 1); C (bid (b, "ch"), n + " canale", bandChNames(), 0);
        B (bid (b, "dyn"), n + " dinamica", false); F (bid (b, "thr"), n + " soglia", { -80, 0, 0.1f }, -30); F (bid (b, "range"), n + " range", { -30, 30, 0.01f }, -6);
        B (bid (b, "spec"), n + " spettrale", false); B (bid (b, "solo"), n + " solo", false);
    }
    return l;
}
Processor::Processor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true).withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                                       .withInput ("Sidechain", juce::AudioChannelSet::stereo(), false)),
      apvts (*this, nullptr, "PARAMS", layout())
{
    for (int s = 0; s < kSlots; ++s)
    {
        sp[s][0] = apvts.getRawParameterValue (sid (s, "type")); sp[s][1] = apvts.getRawParameterValue (sid (s, "byp")); sp[s][2] = apvts.getRawParameterValue (sid (s, "mix"));
        for (int i = 0; i < kP; ++i) sp[s][3 + i] = apvts.getRawParameterValue (sid (s, ("p" + juce::String (i + 1)).toRawUTF8()));
    }
    startTimerHz (20);
}
bool Processor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto o = l.getMainOutputChannelSet(), i = l.getMainInputChannelSet();
    if (o != juce::AudioChannelSet::stereo() || ! (i == o || i == juce::AudioChannelSet::mono())) return false;
    if (l.inputBuses.size() > 1) { const auto sc = l.getChannelSet (true, 1); if (! (sc.isDisabled() || sc == juce::AudioChannelSet::stereo() || sc == juce::AudioChannelSet::mono())) return false; }
    return true;
}
void Processor::rebuild (int s)
{
    const int t = slotType (s);
    std::shared_ptr<Engine> e;
    if (t > 0)
    {
        e = std::shared_ptr<Engine> (makeEngine (t).release());
        if (e) e->prepare (sr, maxBlock);
    }
    { const juce::SpinLock::ScopedLockType l (engLock[s]); std::swap (eng[s], e); }
    if (irPath[s].isNotEmpty() && t > 0 && mod (t).kind == K_CONV) setUserIr (s, juce::File (irPath[s]));
}
void Processor::prepareToPlay (double s, int mb)
{
    sr = s; maxBlock = juce::jmax (64, mb);
    for (int k = 0; k < kSlots; ++k) { dryD[k].prepare (32768); { const juce::SpinLock::ScopedLockType l (engLock[k]); eng[k].reset(); } rebuild (k); }
    cmpD.prepare (1 << 17);
    dryL.assign ((size_t) maxBlock, 0); dryR = dryL; scL = dryL; scR = dryL;
    eq.prepare (s, maxBlock);
    loudOut.prepare (s); loudIn.prepare (s); for (auto& t : tpc) t.reset();
    lat = -1;
}
void Processor::timerCallback()
{
    for (int s = 0; s < kSlots; ++s)
    {
        const int t = slotType (s); const int have = eng[s] ? eng[s]->type : 0;
        if (t != have) rebuild (s);
    }
}
EqSet Processor::eqSettings() const
{
    EqSet S; S.live = value ("live") > 0.5f; S.att = value ("eq_att"); S.rel = value ("eq_rel");
    for (int b = 0; b < kBands; ++b)
    {
        auto& B = S.b[b]; auto v = [this, b] (const char* w) { return valueS (bid (b, w)); };
        B.on = v ("on") > 0.5f; B.type = (int) v ("type"); B.f = v ("f"); B.g = v ("g"); B.q = v ("q"); B.slope = (int) v ("slope"); B.ch = (int) v ("ch");
        B.dyn = v ("dyn") > 0.5f; B.thr = v ("thr"); B.range = v ("range"); B.spec = v ("spec") > 0.5f; B.solo = v ("solo") > 0.5f;
    }
    return S;
}
void Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals nd;
    const int N = buffer.getNumSamples(); if (N == 0) return;
    auto main = getBusBuffer (buffer, true, 0);
    if (main.getNumChannels() == 1 || getTotalNumInputChannels() == 1) buffer.copyFrom (1, 0, buffer, 0, 0, N);
    float* L = buffer.getWritePointer (0); float* R = buffer.getWritePointer (1);
    // sidechain esterno (se collegato nel DAW)
    const float* sc[2] = { nullptr, nullptr };
    if (getBusCount (true) > 1 && getBus (true, 1)->isEnabled())
    {
        auto scb = getBusBuffer (buffer, true, 1);
        if (scb.getNumChannels() > 0) { sc[0] = scb.getReadPointer (0); sc[1] = scb.getReadPointer (scb.getNumChannels() > 1 ? 1 : 0); }
    }
    scConnected = sc[0] != nullptr;
    Ctx ctx;
    if (auto* ph = getPlayHead()) if (auto pos = ph->getPosition()) { if (auto b = pos->getBpm()) ctx.bpm = *b; if (auto q = pos->getPpqPosition()) ctx.ppq = *q; ctx.playing = pos->getIsPlaying(); }
    const float inG = juce::Decibels::decibelsToGain (value ("in"));
    if (std::abs (inG - 1.0f) > 1e-5f) for (int i = 0; i < N; ++i) { L[i] *= inG; R[i] *= inG; }
    for (int i = 0; i < N; ++i) loudIn.push (L[i], R[i]);
    inS = loudIn.S;
    const bool cmp = value ("cmp") > 0.5f;
    const EqSet E = eqSettings();
    const int n = chainLen(), eqPos = juce::jlimit (0, n, (int) value ("eq_pos")); const bool eqOn = value ("eq_on") > 0.5f;
    // latenza totale (per compensare il confronto e dirla al DAW)
    int total = 0; float vbuf[kSlots][kP];
    for (int s = 0; s < n; ++s) { for (int i = 0; i < kP; ++i) vbuf[s][i] = sp[s][3 + i]->load(); }
    for (int s = 0; s < n; ++s) { const juce::SpinLock::ScopedTryLockType tl (engLock[s]); if (tl.isLocked() && eng[s] && eng[s]->type == slotType (s)) total += eng[s]->latency (vbuf[s]); }
    if (eqOn) total += eq.latency (E);
    if (total != lat) { lat = total; setLatencySamples (total); }
    for (int off = 0; off < N; off += maxBlock)
    {
        const int m = juce::jmin (maxBlock, N - off); float* l = L + off; float* r = R + off;
        // copia per il confronto, ritardata come la catena
        for (int i = 0; i < m; ++i) { float a = l[i], b = r[i]; cmpD.run (a, b, total); dryL[(size_t) i] = a; dryR[(size_t) i] = b; }
        pre.push (l, r, m);
        if (sc[0]) { ctx.scL = sc[0] + off; ctx.scR = sc[1] + off; } else { ctx.scL = ctx.scR = nullptr; }
        for (int k = 0; k <= n; ++k)
        {
            if (k == eqPos && eqOn) eq.process (l, r, m, E);
            if (k == n) break;
            const int t = slotType (k); if (t == 0) continue;
            const juce::SpinLock::ScopedTryLockType tl (engLock[k]);
            if (! tl.isLocked() || ! eng[k] || eng[k]->type != t) continue;
            auto* e = eng[k].get(); const int Lk = e->latency (vbuf[k]);
            const bool byp = sp[k][1]->load() > 0.5f; const float mix = sp[k][2]->load(); const bool send = mod (t).send;
            if (byp) { for (int i = 0; i < m; ++i) dryD[k].run (l[i], r[i], Lk); e->gr = 0; continue; }
            const bool needDry = send || mix < 0.999f;
            if (needDry) for (int i = 0; i < m; ++i) { float a = l[i], b = r[i]; dryD[k].run (a, b, Lk); scL[(size_t) i] = a; scR[(size_t) i] = b; }
            e->process (l, r, m, vbuf[k], ctx);
            if (send) for (int i = 0; i < m; ++i) { l[i] = scL[(size_t) i] + l[i] * mix; r[i] = scR[(size_t) i] + r[i] * mix; }
            else if (needDry) for (int i = 0; i < m; ++i) { l[i] = scL[(size_t) i] + (l[i] - scL[(size_t) i]) * mix; r[i] = scR[(size_t) i] + (r[i] - scR[(size_t) i]) * mix; }
        }
        const float og = juce::Decibels::decibelsToGain (value ("out"));
        for (int i = 0; i < m; ++i) { l[i] *= og; r[i] *= og; }
        // confronto a pari volume: si sente il segnale originale portato allo stesso loudness
        if (cmp)
        {
            const float t = (loudIn.S > -70 && lufsS.load() > -70) ? juce::Decibels::decibelsToGain (juce::jlimit (-24.0f, 24.0f, lufsS.load() - loudIn.S)) : 1.0f;
            for (int i = 0; i < m; ++i) { cmpG += (t - cmpG) * 0.0005f; l[i] = dryL[(size_t) i] * cmpG; r[i] = dryR[(size_t) i] * cmpG; }
        }
        // meter in uscita
        if (resetMeter.exchange (false)) { loudOut.resetIntegrated(); tpHold = -100; }
        float tpk = 0, spk = 0;
        for (int i = 0; i < m; ++i)
        {
            if (! cmp) loudOut.push (l[i], r[i]);
            tpk = juce::jmax (tpk, tpc[0].push (l[i]), tpc[1].push (r[i])); spk = juce::jmax (spk, std::abs (l[i]), std::abs (r[i]));
        }
        const float tdb = juce::Decibels::gainToDecibels (tpk, -100.0f);
        tp = juce::jmax (tdb, tp.load() - 0.6f); tpHold = juce::jmax (tpHold.load(), tdb); pk = juce::jmax (juce::Decibels::gainToDecibels (spk, -100.0f), pk.load() - 0.6f);
        post.push (l, r, m);
        {   // imager: correlazione e larghezza
            double lr = 0, ll = 0, rr = 0, ms = 0, ss = 0;
            for (int i = 0; i < m; ++i) { lr += l[i] * r[i]; ll += l[i] * l[i]; rr += r[i] * r[i]; const float a = 0.5f * (l[i] + r[i]), b = 0.5f * (l[i] - r[i]); ms += a * a; ss += b * b; }
            const float c = ll * rr > 1e-12 ? (float) (lr / std::sqrt (ll * rr)) : 1.0f;
            corr = corr.load() * 0.9f + c * 0.1f; width = width.load() * 0.9f + (float) (ss / juce::jmax (1e-12, ms + ss)) * 0.1f;
        }
    }
    lufsM = loudOut.M; lufsS = loudOut.S; lufsI = loudOut.I;
}

// ======================================================================= rack
static juce::StringArray slotIds() { juce::StringArray a { "type", "byp", "mix" }; for (int i = 0; i < kP; ++i) a.add ("p" + juce::String (i + 1)); return a; }
void Processor::moveRaw (int from, int to)
{
    for (auto& w : slotIds()) { auto* a = apvts.getParameter (sid (from, w.toRawUTF8())); auto* b = apvts.getParameter (sid (to, w.toRawUTF8())); b->setValueNotifyingHost (a->getValue()); }
    { const juce::SpinLock::ScopedLockType la (engLock[juce::jmin (from, to)]); const juce::SpinLock::ScopedLockType lb (engLock[juce::jmax (from, to)]); eng[to] = eng[from]; }
    irPath[to] = irPath[from];
}
void Processor::setModule (int s, int type)
{
    const auto& d = mod (type);
    setP (sid (s, "type"), (float) type); setP (sid (s, "byp"), 0); setP (sid (s, "mix"), d.defMix);
    for (int i = 0; i < kP; ++i) { const auto& sp0 = d.spec[(size_t) i]; setP (sid (s, ("p" + juce::String (i + 1)).toRawUTF8()), hidden (sp0) ? 0.0f : specNorm (sp0, sp0.def)); }
    irPath[s] = {};
    rebuild (s);
}
void Processor::insertModule (int pos, int type)
{
    const int n = chainLen(); if (n >= kSlots) return;
    pos = juce::jlimit (0, n, pos);
    for (int s = n; s > pos; --s) moveRaw (s - 1, s);
    { const juce::SpinLock::ScopedLockType l (engLock[pos]); eng[pos].reset(); }
    setModule (pos, type);
    setP ("n", (float) (n + 1));
    const int ep = (int) value ("eq_pos"); if (ep > pos) setP ("eq_pos", (float) (ep + 1));
}
void Processor::removeSlot (int s)
{
    const int n = chainLen(); if (s < 0 || s >= n) return;
    for (int k = s; k < n - 1; ++k) moveRaw (k + 1, k);
    { const juce::SpinLock::ScopedLockType l (engLock[n - 1]); eng[n - 1].reset(); }
    setP (sid (n - 1, "type"), 0); irPath[n - 1] = {};
    setP ("n", (float) (n - 1));
    const int ep = (int) value ("eq_pos"); if (ep > s) setP ("eq_pos", (float) (ep - 1));
}
void Processor::moveSlot (int from, int to)
{
    const int n = chainLen(); if (from == to || from < 0 || to < 0 || from >= n || to >= n) return;
    // salva il modulo, fai scorrere gli altri, rimettilo al posto nuovo
    std::vector<float> keep; for (auto& w : slotIds()) keep.push_back (apvts.getParameter (sid (from, w.toRawUTF8()))->getValue());
    std::shared_ptr<Engine> e; { const juce::SpinLock::ScopedLockType l (engLock[from]); e = eng[from]; } const auto ip = irPath[from];
    if (from < to) for (int k = from; k < to; ++k) moveRaw (k + 1, k); else for (int k = from; k > to; --k) moveRaw (k - 1, k);
    int i = 0; for (auto& w : slotIds()) apvts.getParameter (sid (to, w.toRawUTF8()))->setValueNotifyingHost (keep[(size_t) i++]);
    { const juce::SpinLock::ScopedLockType l (engLock[to]); eng[to] = e; } irPath[to] = ip;
}
void Processor::copySlot (int s)
{
    clip.type = slotType (s); int i = 0; for (auto& w : slotIds()) clip.v[juce::jmin (kP, i++)] = apvts.getParameter (sid (s, w.toRawUTF8()))->getValue();
    clip.v[0] = valueS (sid (s, "mix"));
    for (int k = 0; k < kP; ++k) clip.v[1 + k] = valueS (sid (s, ("p" + juce::String (k + 1)).toRawUTF8()));
}
bool Processor::pasteSlot (int s)
{
    if (clip.type <= 0) return false;
    if (s >= chainLen()) { insertModule (chainLen(), clip.type); s = chainLen() - 1; } else setModule (s, clip.type);
    setP (sid (s, "mix"), clip.v[0]); for (int k = 0; k < kP; ++k) setP (sid (s, ("p" + juce::String (k + 1)).toRawUTF8()), clip.v[1 + k]);
    return true;
}
void Processor::setUserIr (int s, const juce::File& f)
{
    juce::AudioFormatManager fm; fm.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> rd (fm.createReaderFor (f)); if (! rd) return;
    const int len = (int) std::min<juce::int64> (rd->lengthInSamples, (juce::int64) (rd->sampleRate * 12));
    juce::AudioBuffer<float> b (2, len); rd->read (&b, 0, len, 0, true, true);
    irPath[s] = f.getFullPathName();
    const juce::SpinLock::ScopedLockType l (engLock[s]);
    if (eng[s]) eng[s]->setUserIr (std::move (b), rd->sampleRate);
}
void Processor::loadPreset (int i)
{
    if (! juce::isPositiveAndBelow (i, (int) factoryPresets().size())) return;
    const auto& P = factoryPresets()[(size_t) i]; preset = i; presetName = P.cat + " | " + P.name;
    for (auto* pp : getParameters()) if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (pp)) if (rp->paramID != "in" && rp->paramID != "out" && rp->paramID != "live") rp->setValueNotifyingHost (rp->getDefaultValue());
    for (auto& s : irPath) s = {};
    setP ("target", (float) P.target);
    for (auto& [id, v] : P.v) setP (id, v);
    for (int s = 0; s < kSlots; ++s) rebuild (s);
    resetMeter = true;
}
juce::File Processor::userPresetDir() { auto d = juce::File::getSpecialLocation (juce::File::userMusicDirectory).getChildFile ("Jackpot Mixology").getChildFile ("Preset"); d.createDirectory(); return d; }
void Processor::saveUserPreset (const juce::String& name)
{
    juce::MemoryBlock mb; getStateInformation (mb);
    if (auto x = getXmlFromBinary (mb.getData(), (int) mb.getSize())) { x->setAttribute ("presetName", name); x->writeTo (userPresetDir().getChildFile (juce::File::createLegalFileName (name) + ".xml")); }
    presetName = "MIO | " + name;
}
void Processor::loadUserPreset (const juce::File& f)
{
    if (auto x = juce::XmlDocument::parse (f)) { juce::MemoryBlock mb; copyXmlToBinary (*x, mb); setStateInformation (mb.getData(), (int) mb.getSize()); presetName = "MIO | " + f.getFileNameWithoutExtension(); }
}
void Processor::getStateInformation (juce::MemoryBlock& d)
{
    auto s = apvts.copyState(); s.setProperty ("preset", preset, nullptr); s.setProperty ("presetName", presetName, nullptr);
    for (int k = 0; k < kSlots; ++k) s.setProperty ("ir" + juce::String (k), irPath[k], nullptr);
    if (auto x = s.createXml()) copyXmlToBinary (*x, d);
}
void Processor::setStateInformation (const void* data, int size)
{
    if (auto x = getXmlFromBinary (data, size))
    {
        auto t = juce::ValueTree::fromXml (*x); if (! t.isValid()) return;
        preset = t.getProperty ("preset", 0); presetName = t.getProperty ("presetName", "Init").toString();
        for (int k = 0; k < kSlots; ++k) irPath[k] = t.getProperty ("ir" + juce::String (k), "").toString();
        apvts.replaceState (t);
        for (int s = 0; s < kSlots; ++s) rebuild (s);
    }
}
juce::AudioProcessorEditor* Processor::createEditor() { return new Editor (*this); }
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new jmx::Processor(); }
