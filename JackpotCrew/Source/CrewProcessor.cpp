#include "CrewProcessor.h"
#include "CrewEditor.h"

namespace jcrew
{
const std::vector<Preset>& presets()
{
    // nei preset si accendono i personaggi: il resto della voce (pan, pitch, colore) parte dal suo carattere
    auto on = [] (std::initializer_list<int> who) { std::vector<std::pair<juce::String, float>> v; for (int k : who) v.push_back ({ Processor::vid (k, "on"), 1.0f }); return v; };
    auto plus = [] (std::vector<std::pair<juce::String, float>> a, std::initializer_list<std::pair<juce::String, float>> b) { for (auto& x : b) a.push_back (x); return a; };
    static const std::vector<Preset> p = {
        { "INIT", "BASE", {} },
        { "CREW IN STUDIO", "HOOK", plus (on ({ 0, 1, 4, 5 }), { { "loose", 0.4f }, { "drift", 0.45f }, { "spread", 0.8f } }) },
        { "TUTTI DENTRO", "HOOK", plus (on ({ 0, 1, 2, 3, 4, 5 }), { { "loose", 0.5f }, { "drift", 0.5f }, { "spread", 0.9f }, { "shout", 0.25f }, { "room", 0.15f } }) },
        { "STADIO", "HOOK", plus (on ({ 0, 1, 2, 3, 4, 5 }), { { "loose", 0.7f }, { "drift", 0.6f }, { "spread", 1.0f }, { "shout", 0.45f }, { "room", 0.45f }, { "lowcut", 180 } }) },
        { "DOPPIA STRETTA", "RADDOPPI", plus (on ({ 4 }), { { "loose", 0.15f }, { "drift", 0.3f }, { "spread", 0.6f } }) },
        { "DOPPIA LARGA", "RADDOPPI", plus (on ({ 1, 4 }), { { "loose", 0.25f }, { "drift", 0.35f }, { "spread", 1.0f }, { "v2_pan", 0.8f }, { "v5_pan", -0.8f } }) },
        { "ROCCIOSO", "DARK", plus (on ({ 0, 5 }), { { "loose", 0.3f }, { "drift", 0.3f }, { "spread", 0.6f }, { "shout", 0.2f } }) },
        { "MOSTRI", "DARK", plus (on ({ 0, 5, 3 }), { { "v4_pitch", -12 }, { "loose", 0.5f }, { "drift", 0.6f }, { "shout", 0.35f }, { "room", 0.2f } }) },
        { "PUNK HOOK", "GRIDATO", plus (on ({ 1, 4, 5 }), { { "loose", 0.55f }, { "drift", 0.7f }, { "shout", 0.7f }, { "room", 0.1f }, { "lowcut", 200 } }) },
        { "SPAZIALE", "CYBER", plus (on ({ 2, 3 }), { { "loose", 0.3f }, { "drift", 0.5f }, { "spread", 1.0f }, { "room", 0.5f } }) },
        { "OTTAVE", "CYBER", plus (on ({ 0, 2 }), { { "loose", 0.1f }, { "drift", 0.2f }, { "spread", 0.5f } }) },
        { "AD-LIB DIETRO", "AD-LIB", plus (on ({ 3, 4 }), { { "loose", 0.45f }, { "drift", 0.4f }, { "spread", 1.0f }, { "room", 0.3f }, { "crew", -6 } }) },
    };
    return p;
}
juce::StringArray Processor::allIds()
{
    juce::StringArray s; for (int k = 0; k < kVoices; ++k) for (auto* w : { "on", "lvl", "pan", "pitch", "tone" }) s.add (vid (k, w));
    for (auto* g : { "loose", "drift", "spread", "shout", "room", "lowcut", "crew" }) s.add (g);
    return s;
}
juce::AudioProcessorValueTreeState::ParameterLayout Processor::layout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    auto F = [&l] (const juce::String& id, const juce::String& n, NormalisableRange<float> r, float d) { l.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, n, r, d)); };
    for (int k = 0; k < kVoices; ++k)
    {
        const auto& c = characters()[(size_t) k]; const juce::String n = c.name;
        // di partenza nessun personaggio acceso: la voce passa com'e'
        l.add (std::make_unique<AudioParameterBool> (ParameterID { vid (k, "on"), 1 }, n + " acceso", false));
        F (vid (k, "lvl"), n + " volume", { -30, 6, 0.01f }, c.lvl);
        F (vid (k, "pan"), n + " pan", { -1, 1, 0.001f }, c.pan);
        F (vid (k, "pitch"), n + " pitch", { -12, 12, 1 }, c.pitch);
        F (vid (k, "tone"), n + " colore", { -1, 1, 0.001f }, c.tone);
    }
    F ("loose", "Sciolti", { 0, 1 }, 0.4f); F ("drift", "Stonati", { 0, 1 }, 0.4f); F ("spread", "Larghezza", { 0, 1 }, 0.8f);
    F ("shout", "Gridato", { 0, 1 }, 0); F ("room", "Stanza", { 0, 1 }, 0);
    NormalisableRange<float> lc (20, 600); lc.setSkewForCentre (140); F ("lowcut", "Taglio bassi crew", lc, 120);
    F ("crew", "Volume crew", { -30, 6, 0.01f }, 0); F ("dry", "Voce principale", { -60, 0, 0.01f }, 0);
    return l;
}
Processor::Processor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true).withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, &undo, "PARAMS", layout()) {}
bool Processor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto o = l.getMainOutputChannelSet(), i = l.getMainInputChannelSet();
    return o == juce::AudioChannelSet::stereo() && (i == o || i == juce::AudioChannelSet::mono());
}
void Processor::prepareToPlay (double s, int mb) { eng.prepare (s, juce::jmax (mb, 32)); }
Settings Processor::settings() const
{
    Settings S;
    for (int k = 0; k < kVoices; ++k) S.v[(size_t) k] = { value (vid (k, "on")) > 0.5f, value (vid (k, "lvl")), value (vid (k, "pan")), value (vid (k, "pitch")), value (vid (k, "tone")) };
    S.loose = value ("loose"); S.drift = value ("drift"); S.spread = value ("spread"); S.shout = value ("shout"); S.room = value ("room");
    S.lowCut = value ("lowcut"); S.crewDb = value ("crew"); S.dryDb = value ("dry");
    return S;
}
void Processor::processBlock (juce::AudioBuffer<float>& b, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals nd;
    const int n = b.getNumSamples(); if (n == 0) return;
    if (getTotalNumInputChannels() == 1) b.copyFrom (1, 0, b, 0, 0, n);
    uiIn.store (juce::jmax (b.getMagnitude (0, 0, n), uiIn.load() * 0.8f));
    eng.process (b.getWritePointer (0), b.getWritePointer (1), n, settings());
    for (int k = 0; k < kVoices; ++k) uiMeter[(size_t) k].store (eng.meter (k));
}
void Processor::loadPreset (int i)
{
    if (! juce::isPositiveAndBelow (i, (int) presets().size())) return;
    undo.beginNewTransaction();
    for (auto& id : allIds()) if (auto* p = apvts.getParameter (id)) p->setValueNotifyingHost (p->getDefaultValue());
    for (auto& kv : presets()[(size_t) i].v) setP (kv.first, kv.second);
    preset = i; userName.clear();
}
bool Processor::saveUserPreset (const juce::String& name)
{
    juce::XmlElement x ("CREWPRESET"); x.setAttribute ("name", name);
    for (auto& id : allIds()) x.setAttribute (id, value (id));
    const bool ok = x.writeTo (presetFolder().getChildFile (juce::File::createLegalFileName (name) + ".crew"));
    if (ok) userName = name;
    return ok;
}
bool Processor::loadUserPreset (const juce::File& f)
{
    auto x = juce::XmlDocument::parse (f); if (x == nullptr || ! x->hasTagName ("CREWPRESET")) return false;
    undo.beginNewTransaction();
    for (auto& id : allIds()) if (x->hasAttribute (id)) setP (id, (float) x->getDoubleAttribute (id));
    userName = x->getStringAttribute ("name", f.getFileNameWithoutExtension());
    return true;
}
void Processor::getStateInformation (juce::MemoryBlock& d)
{
    auto st = apvts.copyState(); std::unique_ptr<juce::XmlElement> x (st.createXml());
    x->setAttribute ("preset", preset); x->setAttribute ("user", userName); x->setAttribute ("theme", uiTheme); copyXmlToBinary (*x, d);
}
void Processor::setStateInformation (const void* data, int size)
{
    std::unique_ptr<juce::XmlElement> x (getXmlFromBinary (data, size));
    if (x == nullptr || ! x->hasTagName (apvts.state.getType())) return;
    preset = x->getIntAttribute ("preset", 0); userName = x->getStringAttribute ("user"); uiTheme = x->getIntAttribute ("theme", uiTheme);
    apvts.replaceState (juce::ValueTree::fromXml (*x));
}
juce::AudioProcessorEditor* Processor::createEditor() { return new Editor (*this); }
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new jcrew::Processor(); }
