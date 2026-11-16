#include "DnaProcessor.h"
#include "DnaEditor.h"

namespace dna
{
juce::AudioProcessorValueTreeState::ParameterLayout Processor::layout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "chop", 1 }, "Modo di taglio", StringArray { "SMART", "TRANSIENTI", "GRIGLIA", "ARMONIA" }, 0));
    l.add (std::make_unique<AudioParameterInt> (ParameterID { "count", 1 }, "Slice", 2, 32, 16));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "sens", 1 }, "Sensibilita", NormalisableRange<float> (0, 1), 0.5f));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "pitch", 1 }, "Pitch", NormalisableRange<float> (-24, 24, 1), 0));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "att", 1 }, "Attacco", NormalisableRange<float> (0, 200, 0.1f, 0.5f), 1));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "rel", 1 }, "Rilascio", NormalisableRange<float> (5, 2000, 1, 0.4f), 30));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "play", 1 }, "Suona", StringArray { "ONE SHOT", "FINCHE' TIENI" }, 0));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "choke", 1 }, "Choke", true));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "gain", 1 }, "Volume", NormalisableRange<float> (-24, 12, 0.1f), 0));
    return l;
}
Processor::Processor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)), apvts (*this, nullptr, "PARAMS", layout())
{
    fm.registerBasicFormats();
    for (auto& p : playing) p = 0;
    for (auto id : { "chop", "count", "sens" }) apvts.addParameterListener (id, this);
}
Processor::~Processor() { cancelPendingUpdate(); for (auto id : { "chop", "count", "sens" }) apvts.removeParameterListener (id, this); while (busy.load()) juce::Thread::sleep (10); }
void Processor::prepareToPlay (double s, int) { sr = s; for (auto& v : voices) v.on = false; }

bool Processor::loadFile (const juce::File& f)
{
    std::unique_ptr<juce::AudioFormatReader> r (fm.createReaderFor (f)); if (r == nullptr) { status = "Formato non letto"; return false; }
    const int n = (int) std::min<juce::int64> (r->lengthInSamples, (juce::int64) (r->sampleRate * 90.0));
    juce::AudioBuffer<float> b ((int) juce::jmax (1u, juce::jmin (2u, r->numChannels)), n); r->read (&b, 0, n, 0, true, b.getNumChannels() > 1);
    path = f.getFullPathName();
    loadBuffer (std::move (b), r->sampleRate, f.getFileNameWithoutExtension());
    return true;
}
void Processor::loadBuffer (juce::AudioBuffer<float>&& b, double fileSr, const juce::String& name)
{
    raw = std::make_shared<juce::AudioBuffer<float>> (std::move (b)); rawSr = fileSr; variant = 0;
    auto k = std::make_shared<Kit>(); k->audio.makeCopyOf (*raw); k->sr = fileSr; k->starts = { 0 }; k->name = name;
    std::atomic_store (&kitPtr, std::shared_ptr<const Kit> (k));
    reanalyse = true; rechop();
}
void Processor::rechop()
{
    if (raw == nullptr) return;
    if (reanalyse)
    {
        if (busy.exchange (true)) { triggerAsyncUpdate(); return; }
        reanalyse = false; status = "Analisi...";
        auto src = raw; const double s = rawSr; const float sens = v ("sens");
        juce::Thread::launch ([this, src, s, sens]
        {
            auto an = std::make_shared<Analysis> (analyse (*src, s, sens));
            std::atomic_store (&anPtr, std::shared_ptr<const Analysis> (an));
            busy = false; status = {};
            triggerAsyncUpdate();
        });
        return;
    }
    auto an = analysis(); auto old = kit(); if (an == nullptr || old == nullptr) return;
    auto k = std::make_shared<Kit>(); k->audio.makeCopyOf (*raw); k->sr = rawSr; k->name = old->name;
    k->starts = chop (*an, juce::roundToInt (v ("chop")), juce::roundToInt (v ("count")), variant.load(), *raw);
    std::atomic_store (&kitPtr, std::shared_ptr<const Kit> (k));
}
void Processor::halveBpm() { auto a = analysis(); if (a == nullptr) return; auto c = std::make_shared<Analysis> (*a); c->bpm *= 0.5f; std::atomic_store (&anPtr, std::shared_ptr<const Analysis> (c)); rechop(); }
void Processor::doubleBpm() { auto a = analysis(); if (a == nullptr) return; auto c = std::make_shared<Analysis> (*a); c->bpm *= 2.0f; std::atomic_store (&anPtr, std::shared_ptr<const Analysis> (c)); rechop(); }

juce::File Processor::writeMidi()
{
    auto k = kit(); auto a = analysis(); if (k == nullptr || a == nullptr || k->starts.empty()) return {};
    const double bpm = a->bpm > 0 ? a->bpm : 120.0, ppq = 960.0;
    juce::MidiMessageSequence seq;
    seq.addEvent (juce::MidiMessage::tempoMetaEvent ((int) (60000000.0 / bpm)), 0);
    const int nS = (int) k->starts.size();
    for (int i = 0; i < nS && i < 32; ++i)
    {
        const double t0 = k->starts[(size_t) i] / k->sr, t1 = (i + 1 < nS ? k->starts[(size_t) i + 1] : k->audio.getNumSamples()) / k->sr;
        const double b0 = t0 * bpm / 60.0 * ppq, b1 = t1 * bpm / 60.0 * ppq;
        seq.addEvent (juce::MidiMessage::noteOn (1, 36 + i, (juce::uint8) 100), b0);
        seq.addEvent (juce::MidiMessage::noteOff (1, 36 + i), juce::jmax (b0 + 10, b1 - 5));
    }
    seq.updateMatchedPairs();
    juce::MidiFile mf; mf.setTicksPerQuarterNote ((int) ppq); mf.addTrack (seq);
    auto dir = juce::File::getSpecialLocation (juce::File::userMusicDirectory).getChildFile ("Jackpot").getChildFile ("Sample DNA"); dir.createDirectory();
    auto f = dir.getChildFile (k->name + " - " + modeName (juce::roundToInt (v ("chop"))) + " " + juce::String (nS) + " slice.mid");
    f.deleteFile(); juce::FileOutputStream os (f); if (os.openedOk()) mf.writeTo (os);
    return f;
}

void Processor::startVoice (int slice, int note, float vel)
{
    auto k = kit(); if (k == nullptr || slice < 0 || slice >= (int) k->starts.size()) return;
    if (v ("choke") > 0.5f) for (auto& vv : voices) if (vv.on) vv.stage = 2;
    Voice* best = nullptr; for (auto& vv : voices) if (! vv.on) { best = &vv; break; } if (best == nullptr) best = &voices[0];
    best->on = true; best->slice = slice; best->note = note; best->pos = k->starts[(size_t) slice];
    best->end = slice + 1 < (int) k->starts.size() ? k->starts[(size_t) slice + 1] : k->audio.getNumSamples();
    best->rate = k->sr / sr * std::pow (2.0, v ("pitch") / 12.0); best->env = 0; best->vel = vel; best->stage = 0;
}
void Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals nd; buffer.clear();
    auto k = kit(); const int N = buffer.getNumSamples();
    if (k == nullptr) return;
    const int pv = preview.exchange (-1); if (pv >= 0) startVoice (pv, -2, 0.85f);
    const float att = 1.0f / juce::jmax (1.0f, v ("att") * 0.001f * (float) sr), rel = std::exp (-1.0f / juce::jmax (1.0f, v ("rel") * 0.001f * (float) sr));
    const bool gate = v ("play") > 0.5f; const float g = juce::Decibels::decibelsToGain (v ("gain"));
    float* L = buffer.getWritePointer (0); float* R = buffer.getWritePointer (1);
    const float* aL = k->audio.getReadPointer (0); const float* aR = k->audio.getReadPointer (juce::jmin (1, k->audio.getNumChannels() - 1));
    const int len = k->audio.getNumSamples();
    auto it = midi.begin();
    for (int i = 0; i < N; ++i)
    {
        for (; it != midi.end() && (*it).samplePosition <= i; ++it)
        {
            const auto m = (*it).getMessage();
            if (m.isNoteOn()) startVoice (m.getNoteNumber() - 36, m.getNoteNumber(), m.getFloatVelocity());
            else if (m.isNoteOff() && gate) for (auto& vv : voices) if (vv.on && vv.note == m.getNoteNumber()) vv.stage = 2;
        }
        float l = 0, r = 0;
        for (auto& vv : voices)
        {
            if (! vv.on) continue;
            if (vv.stage == 0) { vv.env += att; if (vv.env >= 1) { vv.env = 1; vv.stage = 1; } }
            else if (vv.stage == 2) { vv.env *= rel; if (vv.env < 1e-4f) { vv.on = false; continue; } }
            const int p0 = (int) vv.pos; const float fr = (float) (vv.pos - p0);
            if (p0 + 1 >= len || vv.pos >= vv.end) { if (vv.stage != 2) vv.stage = 2; if (p0 + 1 >= len) { vv.on = false; continue; } }
            // fine della slice: piccola dissolvenza per non fare click
            const float tail = (float) juce::jlimit (0.0, 1.0, (vv.end - vv.pos) / (0.004 * k->sr));
            const float gg = vv.env * vv.vel * g * (vv.stage == 2 && vv.pos >= vv.end ? 0.0f : juce::jmin (1.0f, tail + (vv.pos >= vv.end ? 0.0f : 0.0f)));
            l += (aL[p0] + (aL[p0 + 1] - aL[p0]) * fr) * gg; r += (aR[p0] + (aR[p0 + 1] - aR[p0]) * fr) * gg;
            vv.pos += vv.rate;
        }
        L[i] = l; R[i] = r;
    }
    for (int s = 0; s < 32; ++s) playing[s] = 0;
    float pp = -1; for (auto& vv : voices) if (vv.on) { if (vv.slice < 32) playing[vv.slice] = 1; pp = (float) vv.pos; }
    playPos = pp;
}
void Processor::getStateInformation (juce::MemoryBlock& d)
{
    auto s = apvts.copyState(); s.setProperty ("path", path, nullptr); s.setProperty ("variant", variant.load(), nullptr);
    if (auto x = s.createXml()) copyXmlToBinary (*x, d);
}
void Processor::setStateInformation (const void* data, int size)
{
    if (auto x = getXmlFromBinary (data, size))
    {
        auto t = juce::ValueTree::fromXml (*x); if (! t.isValid()) return;
        apvts.replaceState (t);
        const juce::String p = t.getProperty ("path", "").toString(); const int var = (int) t.getProperty ("variant", 0);
        if (p.isNotEmpty() && juce::File (p).existsAsFile()) { loadFile (juce::File (p)); variant = var; }
    }
}
juce::AudioProcessorEditor* Processor::createEditor() { return new Editor (*this); }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new dna::Processor(); }
