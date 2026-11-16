#include "SpaceProcessor.h"
#include "SpaceEditor.h"

namespace jsp
{
const std::vector<SpPreset>& factoryPresets()
{
    static const std::vector<SpPreset> P = {
        { "Init  |  Plate + Tape", "Una lastra e un'eco a nastro in parallelo: la partenza per voci e synth.",
          { PLATE, TAPE, OFF, OFF, OFF, OFF }, { 0.30f, 0.25f, 0.3f, 0.3f, 0.3f, 0.3f }, { true, true, true, true, true, true }, { true, true, true, true, true, true } },
        { "Voce Pop", "Eco a tempo che finisce nella lastra: le ripetizioni si allargano e restano dietro.",
          { DIGITAL, PLATE, ROOM, OFF, OFF, OFF }, { 0.22f, 0.28f, 0.15f, 0.3f, 0.3f, 0.3f }, { false, false, true, true, true, true }, { true, true, true, true, true, true } },
        { "Dub Chamber", "Nastro che gira dentro la molla: il classico del dub.",
          { TAPE, SPRING, HALL, OFF, OFF, OFF }, { 0.40f, 0.35f, 0.12f, 0.3f, 0.3f, 0.3f }, { false, false, true, true, true, true }, { true, true, true, true, true, true } },
        { "Hall 224", "Solo la sala, lunga, con il chorus nelle code.",
          { HALL, OFF, OFF, OFF, OFF, OFF }, { 0.35f, 0.3f, 0.3f, 0.3f, 0.3f, 0.3f }, { true, true, true, true, true, true }, { true, true, true, true, true, true } },
        { "Batteria in stanza", "Stanza corta e una lastra brillante solo sul rullante (abbassa il mix se serve).",
          { ROOM, PLATE, OFF, OFF, OFF, OFF }, { 0.25f, 0.12f, 0.3f, 0.3f, 0.3f, 0.3f }, { true, true, true, true, true, true }, { true, true, true, true, true, true } },
        { "Lo-fi Spring", "Molla e nastro sporco in fila, per chitarre e campioni.",
          { SPRING, TAPE, OFF, OFF, OFF, OFF }, { 0.35f, 0.25f, 0.3f, 0.3f, 0.3f, 0.3f }, { false, false, true, true, true, true }, { true, true, true, true, true, true } } };
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
        const String s = "Slot " + String (k + 1);
        l.add (std::make_unique<AudioParameterChoice> (ParameterID { typeId (k), 1 }, s + " Motore", tn, d.type[k]));
        l.add (std::make_unique<AudioParameterBool> (ParameterID { onId (k), 1 }, s + " On", true));
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { mixId (k), 1 }, s + " Mix", NormalisableRange<float> (0, 1), d.mix[k]));
        l.add (std::make_unique<AudioParameterBool> (ParameterID { rtId (k), 1 }, s + " Parallelo", d.par[k]));
        const auto& I = info (d.type[k]);
        for (int i = 0; i < kP; ++i) l.add (std::make_unique<AudioParameterFloat> (ParameterID { pId (k, i), 1 }, s + " " + String (i < I.n ? I.pn[i] : "P" + String (i + 1)), NormalisableRange<float> (0, 1), i < I.n ? I.def[i] : 0.5f));
    }
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "dry", 1 }, "Dry", NormalisableRange<float> (0, 1), 1.0f));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "wet", 1 }, "Wet", NormalisableRange<float> (-24, 6, 0.1f), 0.0f));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "duck", 1 }, "Ducking", NormalisableRange<float> (0, 1), 0.0f));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "out", 1 }, "Output", NormalisableRange<float> (-12, 12, 0.1f), 0.0f));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "freeze", 1 }, "Freeze", false));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "bypass", 1 }, "Bypass", false));
    return l;
}

Processor::Processor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true).withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", layout())
{
    for (int i = 0; i < kHist; ++i) { hIn[i] = -100; hOut[i] = -100; }
    for (int c = 0; c < 2; ++c) pkOut[c] = 0;
    for (int k = 0; k < kSlots; ++k) slotLvl[k] = 0;
    presetName = factoryPresets()[0].name;
}
bool Processor::isBusesLayoutSupported (const BusesLayout& l) const
{
    return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo() && l.getMainInputChannelSet() == juce::AudioChannelSet::stereo();
}
void Processor::prepareToPlay (double s, int block)
{
    sr = s; const int n = juce::jmax (block, 4096);
    for (auto& sl : slots) sl.prepare (s, n);
    dry.setSize (2, n); chain.setSize (2, n); par.setSize (2, n); wet.setSize (2, n);
    for (int k = 0; k < kSlots; ++k) mixS[k] = value (mixId (k));
    setLatencySamples (0);
}

void Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals nd; midi.clear();
    const int N = buffer.getNumSamples(); if (N == 0) return;
    for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); ++c) buffer.clear (c, 0, N);
    if (dry.getNumSamples() < N) { dry.setSize (2, N, false, false, true); chain.setSize (2, N, false, false, true); par.setSize (2, N, false, false, true); wet.setSize (2, N, false, false, true); }
    double bpm = 120;
    if (auto* ph = getPlayHead()) if (auto pos = ph->getPosition()) if (auto b = pos->getBpm()) bpm = *b;
    uiBpm = bpm;
    float* L = buffer.getWritePointer (0); float* R = buffer.getWritePointer (juce::jmin (1, buffer.getNumChannels() - 1));
    for (int c = 0; c < 2; ++c) { dry.copyFrom (c, 0, c ? R : L, N); chain.copyFrom (c, 0, c ? R : L, N); par.clear (c, 0, N); }
    const bool byp = value ("bypass") > 0.5f, frz = value ("freeze") > 0.5f;
    float* cL = chain.getWritePointer (0); float* cR = chain.getWritePointer (1);
    float* pL = par.getWritePointer (0); float* pR = par.getWritePointer (1);
    const float* dL = dry.getReadPointer (0); const float* dR = dry.getReadPointer (1);
    float* wL = wet.getWritePointer (0); float* wR = wet.getWritePointer (1);
    for (int k = 0; k < kSlots; ++k)
    {
        const int t = value (onId (k)) > 0.5f && ! byp ? slotType (k) : (int) OFF;
        if (t == OFF) { slotLvl[k] = slotLvl[k].load() * 0.8f; continue; }
        float p[kP]; for (int i = 0; i < kP; ++i) p[i] = value (pId (k, i));
        const bool isPar = value (rtId (k)) > 0.5f;
        // in parallelo lo slot sente il suono pulito, in serie quello che esce dagli slot prima
        slots[k].process (isPar ? dL : cL, isPar ? dR : cR, wL, wR, N, t, p, bpm, frz);
        const float mt = value (mixId (k)); float lv = 0;
        for (int i = 0; i < N; ++i)
        {
            mixS[k] += (mt - mixS[k]) * 0.003f;
            const float a = wL[i] * mixS[k], b = wR[i] * mixS[k];
            if (isPar) { pL[i] += a; pR[i] += b; } else { cL[i] += a; cR[i] += b; }
            lv = juce::jmax (lv, std::abs (a) + std::abs (b));
        }
        slotLvl[k] = juce::jmax (lv * 0.5f, slotLvl[k].load() * 0.85f);
    }
    // uscita: pulito + (catena - pulito) + parallelo; il ducking abbassa il bagnato quando suona l'ingresso
    const float dt = byp ? 1.0f : value ("dry"), wt = byp ? 0.0f : juce::Decibels::decibelsToGain (value ("wet")), og = byp ? 1.0f : juce::Decibels::decibelsToGain (value ("out"));
    const float duck = value ("duck"), att = 1.0f - std::exp (-1.0f / (0.005f * (float) sr)), rel = 1.0f - std::exp (-1.0f / (0.25f * (float) sr));
    float po[2] = { 0, 0 };
    for (int i = 0; i < N; ++i)
    {
        dryS += (dt - dryS) * 0.002f; wetS += (wt - wetS) * 0.002f; outS += (og - outS) * 0.002f;
        const float lvl = 0.5f * (std::abs (dL[i]) + std::abs (dR[i]));
        env += (lvl > env ? att : rel) * (lvl - env);
        const float red = juce::jlimit (0.0f, 1.0f, juce::Decibels::gainToDecibels (env + 1e-6f, -80.0f) + 40.0f) / 40.0f;   // 0 sotto -40 dB, 1 a 0 dB
        const float dg = 1.0f - duck * juce::jlimit (0.0f, 1.0f, red * 1.6f) * 0.9f; duckS += (dg - duckS) * 0.01f;
        const float wl = (cL[i] - dL[i] + pL[i]) * wetS * duckS, wr = (cR[i] - dR[i] + pR[i]) * wetS * duckS;
        L[i] = (dL[i] * dryS + wl) * outS; R[i] = (dR[i] * dryS + wr) * outS;
        if (! std::isfinite (L[i]) || ! std::isfinite (R[i])) { L[i] = R[i] = 0; }
        po[0] = juce::jmax (po[0], std::abs (L[i])); po[1] = juce::jmax (po[1], std::abs (R[i]));
        colIn = juce::jmax (colIn, std::abs (dL[i]) + std::abs (dR[i])); colOut = juce::jmax (colOut, std::abs (wl) + std::abs (wr));
        if (++colN >= (int) (sr * 0.01))
        {
            const int w = hW.load(); hIn[w] = juce::Decibels::gainToDecibels (colIn * 0.5f, -100.0f); hOut[w] = juce::Decibels::gainToDecibels (colOut * 0.5f, -100.0f);
            hW = (w + 1) % kHist; colIn = colOut = 0; colN = 0;
        }
    }
    for (int c = 0; c < 2; ++c) pkOut[c] = juce::jmax (po[c], pkOut[c].load() * 0.9f);
}

void Processor::setP (int k, int i, float v) { if (auto* p = P (pId (k, i))) p->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, v)); }
void Processor::resetP (int k) { const auto& I = info (slotType (k)); for (int i = 0; i < kP; ++i) setP (k, i, i < I.n ? I.def[i] : 0.5f); }
void Processor::setSlot (int k, int type)
{
    auto* t = P (typeId (k)); t->setValueNotifyingHost (t->convertTo0to1 ((float) type)); resetP (k);
}
void Processor::swapSlots (int a, int b)
{
    if (a == b) return;
    const int ta = slotType (a), tb = slotType (b);
    float xa[kP + 3], xb[kP + 3];
    for (int i = 0; i < kP; ++i) { xa[i] = value (pId (a, i)); xb[i] = value (pId (b, i)); }
    xa[kP] = value (mixId (a)); xb[kP] = value (mixId (b)); xa[kP + 1] = value (onId (a)); xb[kP + 1] = value (onId (b)); xa[kP + 2] = value (rtId (a)); xb[kP + 2] = value (rtId (b));
    setSlot (a, tb); setSlot (b, ta);
    for (int i = 0; i < kP; ++i) { setP (a, i, xb[i]); setP (b, i, xa[i]); }
    P (mixId (a))->setValueNotifyingHost (xb[kP]); P (mixId (b))->setValueNotifyingHost (xa[kP]);
    P (onId (a))->setValueNotifyingHost (xb[kP + 1]); P (onId (b))->setValueNotifyingHost (xa[kP + 1]);
    P (rtId (a))->setValueNotifyingHost (xb[kP + 2]); P (rtId (b))->setValueNotifyingHost (xa[kP + 2]);
}
void Processor::loadFactory (int i)
{
    const auto& p = factoryPresets()[(size_t) juce::jlimit (0, (int) factoryPresets().size() - 1, i)];
    for (int k = 0; k < kSlots; ++k)
    {
        setSlot (k, p.type[k]); P (mixId (k))->setValueNotifyingHost (p.mix[k]);
        P (rtId (k))->setValueNotifyingHost (p.par[k] ? 1.0f : 0.0f); P (onId (k))->setValueNotifyingHost (p.on[k] ? 1.0f : 0.0f);
    }
    presetName = p.name;
}
juce::File Processor::presetFolder()
{
    auto f = juce::File::getSpecialLocation (juce::File::userMusicDirectory).getChildFile ("Jackpot").getChildFile ("Space Preset");
    f.createDirectory(); return f;
}
bool Processor::savePreset (const juce::File& f)
{
    juce::XmlElement x ("JACKPOTSPACE");
    for (int k = 0; k < kSlots; ++k)
    {
        auto* s = x.createNewChildElement ("SLOT");
        s->setAttribute ("type", juce::String (info (slotType (k)).name)); s->setAttribute ("on", value (onId (k)) > 0.5f ? 1 : 0);
        s->setAttribute ("mix", (double) value (mixId (k))); s->setAttribute ("par", value (rtId (k)) > 0.5f ? 1 : 0);
        juce::StringArray av; for (int i = 0; i < kP; ++i) av.add (juce::String (value (pId (k, i)), 4)); s->setAttribute ("p", av.joinIntoString (","));
    }
    for (auto id : { "dry", "wet", "duck", "out" }) x.setAttribute (id, (double) value (id));
    const bool ok = x.writeTo (f); if (ok) presetName = f.getFileNameWithoutExtension(); return ok;
}
bool Processor::loadPreset (const juce::File& f)
{
    auto x = juce::XmlDocument::parse (f); if (! x || ! x->hasTagName ("JACKPOTSPACE")) return false;
    int k = 0;
    for (auto* s : x->getChildIterator())
    {
        if (k >= kSlots) break;
        int t = OFF; for (int i = 0; i < kNumTypes; ++i) if (s->getStringAttribute ("type") == info (i).name) t = i;
        setSlot (k, t); P (onId (k))->setValueNotifyingHost (s->getIntAttribute ("on", 1) ? 1.0f : 0.0f);
        P (mixId (k))->setValueNotifyingHost ((float) s->getDoubleAttribute ("mix", 0.3)); P (rtId (k))->setValueNotifyingHost (s->getIntAttribute ("par", 1) ? 1.0f : 0.0f);
        const auto av = juce::StringArray::fromTokens (s->getStringAttribute ("p"), ",", "");
        for (int i = 0; i < juce::jmin (kP, av.size()); ++i) setP (k, i, av[i].getFloatValue());
        ++k;
    }
    for (auto id : { "dry", "wet", "duck", "out" }) if (x->hasAttribute (id)) { auto* p = P (id); p->setValueNotifyingHost (p->convertTo0to1 ((float) x->getDoubleAttribute (id))); }
    presetName = f.getFileNameWithoutExtension(); return true;
}
void Processor::getStateInformation (juce::MemoryBlock& d)
{
    auto s = apvts.copyState(); s.setProperty ("preset", presetName, nullptr); s.setProperty ("theme", uiTheme.load(), nullptr);
    if (auto x = s.createXml()) copyXmlToBinary (*x, d);
}
void Processor::setStateInformation (const void* data, int size)
{
    if (auto x = getXmlFromBinary (data, size))
    {
        auto t = juce::ValueTree::fromXml (*x); if (! t.isValid()) return;
        presetName = t.getProperty ("preset", presetName).toString(); uiTheme = (int) t.getProperty ("theme", 1);
        apvts.replaceState (t);
    }
}
juce::AudioProcessorEditor* Processor::createEditor() { return new Editor (*this); }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new jsp::Processor(); }
