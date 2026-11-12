#include "FxProcessor.h"
#include "FxEditor.h"

namespace jfx
{
using namespace pfx;
// -1 = valore neutro dell'effetto
const std::vector<FxPreset>& factoryPresets()
{
    static const std::vector<FxPreset> P = {
        { "Init  |  Beatmaker", "Il set di partenza: filtro, volume, spazio e i trucchi da performance.",
          { FILTER, GAIN, REVERB, DELAY, SCRATCH, TAPESTOP, STUTTER, LOFI }, { -1, -1, -1, -1, -1, -1, -1, -1 } },
        { "Transizioni", "Per entrare e uscire dai ritornelli: chiudi, riempi di spazio, taglia.",
          { FILTER, REVERB, DELAY, GATE, STUTTER, TAPESTOP, GLITCH, WIDE }, { -1, -1, -1, -1, -1, -1, -1, -1 } },
        { "Drill Drop", "Il drop prima della strofa: gate, stutter, tape stop e un filo di crush.",
          { FILTER, GATE, STUTTER, TAPESTOP, REVERB, CRUSH, GLITCH, GAIN }, { -1, -1, -1, -1, -1, -1, -1, -1 } },
        { "Lo-fi Room", "Nastro e disco gia accesi un poco: alzi per sporcare di piu.",
          { FILTER, TAPE, VINYL, LOFI, CRUSH, CHORUS, REVERB, GAIN }, { -1, 0.3f, 0.2f, -1, -1, -1, 0.15f, -1 } },
        { "Dub", "Delay e riverbero da spingere, phaser e autopan per farlo girare.",
          { FILTER, DELAY, REVERB, PHASER, TAPESTOP, AUTOPAN, WIDE, GAIN }, { -1, -1, -1, -1, -1, -1, -1, -1 } },
        { "Glitch Lab", "Tutto quello che rompe il tempo, in fila.",
          { STUTTER, GLITCH, GATE, CRUSH, SCRATCH, FILTER, FLANGER, DELAY }, { -1, -1, -1, -1, -1, -1, -1, -1 } },
        { "Wide & Air", "Allarga, addolcisce, apre: per intro e pad.",
          { WIDE, DIMENSION, CHORUS, REVERB, DELAY, FILTER, TREMOLO, GAIN }, { -1, -1, -1, -1, -1, -1, -1, -1 } },
        { "Vinyl DJ", "Come al giradischi: filtro, scratch, disco e frenata.",
          { FILTER, SCRATCH, VINYL, DELAY, REVERB, TAPESTOP, FLANGER, GAIN }, { -1, -1, 0.15f, -1, -1, -1, -1, -1 } } };
    return P;
}

juce::AudioProcessorValueTreeState::ParameterLayout Processor::layout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    StringArray tn; for (int t = 0; t < kNumTypes; ++t) tn.add (info (t).name);
    const auto& d = factoryPresets()[0];
    for (int k = 0; k < kSlots; ++k)
    {
        l.add (std::make_unique<AudioParameterChoice> (ParameterID { typeId (k), 1 }, "Slot " + String (k + 1) + " Effetto", tn, d.type[k]));
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { amtId (k), 1 }, "Slot " + String (k + 1), NormalisableRange<float> (0, 1), info (d.type[k]).neutral));
        l.add (std::make_unique<AudioParameterBool> (ParameterID { onId (k), 1 }, "Slot " + String (k + 1) + " On", true));
    }
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "mix", 1 }, "Mix", NormalisableRange<float> (0, 1), 1.0f));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "out", 1 }, "Output", NormalisableRange<float> (-12, 12, 0.1f), 0.0f));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "bypass", 1 }, "Bypass", false));
    return l;
}

Processor::Processor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true).withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", layout())
{
    for (int k = 0; k < kSlots; ++k)
    {
        ccMap[k] = 70 + k; ccPend[k] = -1.0f; ccSeen[k] = 0;
        typeRaw[k] = apvts.getRawParameterValue (typeId (k)); amtRaw[k] = apvts.getRawParameterValue (amtId (k)); onRaw[k] = apvts.getRawParameterValue (onId (k));
    }
    for (int i = 0; i < kHist; ++i) { hIn[i] = -100; hOut[i] = -100; }
    for (int c = 0; c < 2; ++c) { pkIn[c] = 0; pkOut[c] = 0; }
    presetName = factoryPresets()[0].name;
    startTimerHz (30);
}
bool Processor::isBusesLayoutSupported (const BusesLayout& l) const
{
    return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo() && l.getMainInputChannelSet() == juce::AudioChannelSet::stereo();
}
void Processor::prepareToPlay (double s, int block)
{
    sr = s; eng.prepare (s); dry.setSize (2, juce::jmax (block, 2048)); freeTicks = 0;
    setLatencySamples (0);
}

void Processor::timerCallback()
{   // i CC del controller diventano movimenti dei parametri (cosi' Ableton li puo' registrare come automazione)
    for (int k = 0; k < kSlots; ++k)
    {
        const float v = ccPend[k].exchange (-1.0f);
        if (v >= 0) if (auto* p = P (amtId (k))) p->setValueNotifyingHost (v);
    }
}

void Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals nd;
    const int N = buffer.getNumSamples(); if (N == 0) return;
    for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); ++c) buffer.clear (c, 0, N);
    // controller: CC -> slot (con MIDI learn)
    for (const auto m : midi)
    {
        const auto msg = m.getMessage(); if (! msg.isController()) continue;
        if (! ctrlOn.load() && learnSlot.load() < 0) continue;
        const int cc = msg.getControllerNumber();
        if (const int ls = learnSlot.load(); ls >= 0 && ls < kSlots) { for (int j = 0; j < kSlots; ++j) if (ccMap[j].load() == cc) ccMap[j] = -1; ccMap[ls] = cc; learnSlot = -1; }
        for (int k = 0; k < kSlots; ++k)
            if (ccMap[k].load() == cc)
            {
                float v = msg.getControllerValue() / 127.0f;
                const int ty = juce::roundToInt (typeRaw[k]->load());
                if (info (ty).bipolar && ty != SCRATCH && std::abs (msg.getControllerValue() - 64) <= 8) v = 0.5f;   // centro largo
                ccPend[k] = v; ccSeen[k] = ccSeen[k].load() + 1;
            }
    }
    midi.clear();
    // tempo della DAW
    double bpm = 120, ppq = 0; bool play = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = *b;
            play = pos->getIsPlaying();
            if (auto q = pos->getPpqPosition()) ppq = *q;
        }
    if (! play) { ppq = freeTicks; freeTicks += N / sr * bpm / 60.0; }
    uiBpm = bpm; uiPlay = play;

    float* L = buffer.getWritePointer (0); float* R = buffer.getWritePointer (juce::jmin (1, buffer.getNumChannels() - 1));
    if (dry.getNumSamples() < N) dry.setSize (2, N, false, false, true);
    float* dL = dry.getWritePointer (0); float* dR = dry.getWritePointer (1);
    float pi[2] = { 0, 0 };
    for (int i = 0; i < N; ++i) { dL[i] = L[i]; dR[i] = R[i]; pi[0] = juce::jmax (pi[0], std::abs (L[i])); pi[1] = juce::jmax (pi[1], std::abs (R[i])); }

    const bool byp = value ("bypass") > 0.5f;
    if (! byp)
    {
        int ty[kSlots]; float am[kSlots];
        for (int k = 0; k < kSlots; ++k)
        {
            ty[k] = onRaw[k]->load() > 0.5f ? juce::roundToInt (typeRaw[k]->load()) : (int) OFF;
            const float pnd = ccPend[k].load(); am[k] = pnd >= 0 ? pnd : amtRaw[k]->load();
        }
        eng.process (L, R, N, ty, am, bpm, ppq, play);
    }
    else eng.process (L, R, N, std::array<int, kSlots> {}.data(), std::array<float, kSlots> {}.data(), bpm, ppq, play);   // tiene viva la storia
    if (byp) for (int i = 0; i < N; ++i) { L[i] = dL[i]; R[i] = dR[i]; }
    // mix e uscita
    const float mix = byp ? 1.0f : value ("mix"), og = byp ? 1.0f : juce::Decibels::decibelsToGain (value ("out"));
    float po[2] = { 0, 0 };
    for (int i = 0; i < N; ++i)
    {
        mixS += (mix - mixS) * 0.002f; outS += (og - outS) * 0.002f;
        L[i] = (dL[i] + (L[i] - dL[i]) * mixS) * outS; R[i] = (dR[i] + (R[i] - dR[i]) * mixS) * outS;
        po[0] = juce::jmax (po[0], std::abs (L[i])); po[1] = juce::jmax (po[1], std::abs (R[i]));
        colIn = juce::jmax (colIn, std::abs (dL[i]) + std::abs (dR[i])); colOut = juce::jmax (colOut, std::abs (L[i]) + std::abs (R[i]));
        if (++colN >= (int) (sr * 0.01))
        {
            const int w = hW.load(); hIn[w] = juce::Decibels::gainToDecibels (colIn * 0.5f, -100.0f); hOut[w] = juce::Decibels::gainToDecibels (colOut * 0.5f, -100.0f);
            hW = (w + 1) % kHist; colIn = colOut = 0; colN = 0;
        }
    }
    for (int c = 0; c < 2; ++c) { pkIn[c] = juce::jmax (pi[c], pkIn[c].load() * 0.9f); pkOut[c] = juce::jmax (po[c], pkOut[c].load() * 0.9f); }
}

// ---------------------------------------------------------------- slot e preset
void Processor::setSlot (int k, int type, float amt)
{
    auto* t = P (typeId (k)); auto* a = P (amtId (k));
    t->setValueNotifyingHost (t->convertTo0to1 ((float) type));
    a->setValueNotifyingHost (amt < 0 ? info (type).neutral : amt);
}
void Processor::swapSlots (int a, int b)
{
    if (a == b) return;
    const int ta = slotType (a), tb = slotType (b);
    const float va = P (amtId (a))->getValue(), vb = P (amtId (b))->getValue();
    const bool oa = value (onId (a)) > 0.5f, ob = value (onId (b)) > 0.5f;
    const int ca = ccMap[a].load(), cb = ccMap[b].load(); juce::ignoreUnused (ca, cb);   // il CC resta legato alla manopola fisica
    setSlot (a, tb, vb); setSlot (b, ta, va);
    P (onId (a))->setValueNotifyingHost (ob ? 1.0f : 0.0f); P (onId (b))->setValueNotifyingHost (oa ? 1.0f : 0.0f);
}
void Processor::resetAmounts() { for (int k = 0; k < kSlots; ++k) P (amtId (k))->setValueNotifyingHost (info (slotType (k)).neutral); }
void Processor::loadFactory (int i)
{
    const auto& p = factoryPresets()[(size_t) juce::jlimit (0, (int) factoryPresets().size() - 1, i)];
    for (int k = 0; k < kSlots; ++k) { setSlot (k, p.type[k], p.amt[k]); P (onId (k))->setValueNotifyingHost (1.0f); }
    presetName = p.name;
}
juce::File Processor::presetFolder()
{
    auto f = juce::File::getSpecialLocation (juce::File::userMusicDirectory).getChildFile ("Jackpot").getChildFile ("FX Preset");
    f.createDirectory(); return f;
}
bool Processor::savePreset (const juce::File& f)
{
    juce::XmlElement x ("JACKPOTFX");
    for (int k = 0; k < kSlots; ++k)
    {
        auto* s = x.createNewChildElement ("SLOT");
        s->setAttribute ("type", juce::String (info (slotType (k)).name)); s->setAttribute ("amt", (double) P (amtId (k))->getValue()); s->setAttribute ("on", value (onId (k)) > 0.5f ? 1 : 0);
    }
    x.setAttribute ("mix", (double) value ("mix")); x.setAttribute ("out", (double) value ("out"));
    const bool ok = x.writeTo (f);
    if (ok) presetName = f.getFileNameWithoutExtension();
    return ok;
}
bool Processor::loadPreset (const juce::File& f)
{
    auto x = juce::XmlDocument::parse (f); if (! x || ! x->hasTagName ("JACKPOTFX")) return false;
    int k = 0;
    for (auto* s : x->getChildIterator())
    {
        if (k >= kSlots) break;
        int t = OFF; for (int i = 0; i < kNumTypes; ++i) if (s->getStringAttribute ("type") == info (i).name) t = i;
        setSlot (k, t, (float) s->getDoubleAttribute ("amt", -1)); P (onId (k))->setValueNotifyingHost (s->getIntAttribute ("on", 1) ? 1.0f : 0.0f); ++k;
    }
    P ("mix")->setValueNotifyingHost ((float) x->getDoubleAttribute ("mix", 1));
    P ("out")->setValueNotifyingHost (P ("out")->convertTo0to1 ((float) x->getDoubleAttribute ("out", 0)));
    presetName = f.getFileNameWithoutExtension();
    return true;
}

void Processor::getStateInformation (juce::MemoryBlock& d)
{
    auto s = apvts.copyState(); s.setProperty ("preset", presetName, nullptr);
    juce::StringArray m; for (int k = 0; k < kSlots; ++k) m.add (juce::String (ccMap[k].load())); s.setProperty ("cc", m.joinIntoString (","), nullptr); s.setProperty ("ctrl", ctrlOn.load(), nullptr);
    if (auto x = s.createXml()) copyXmlToBinary (*x, d);
}
void Processor::setStateInformation (const void* data, int size)
{
    if (auto x = getXmlFromBinary (data, size))
    {
        auto t = juce::ValueTree::fromXml (*x); if (! t.isValid()) return;
        presetName = t.getProperty ("preset", presetName).toString();
        const auto m = juce::StringArray::fromTokens (t.getProperty ("cc", "").toString(), ",", "");
        if (m.size() == kSlots) for (int k = 0; k < kSlots; ++k) ccMap[k] = m[k].getIntValue();
        ctrlOn = (bool) t.getProperty ("ctrl", true);
        apvts.replaceState (t);
    }
}
juce::AudioProcessorEditor* Processor::createEditor() { return new Editor (*this); }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new jfx::Processor(); }
