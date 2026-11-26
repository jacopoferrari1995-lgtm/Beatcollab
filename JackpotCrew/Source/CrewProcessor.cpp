#include "CrewProcessor.h"
#include "CrewEditor.h"
#include <map>

namespace jcrew
{
const std::vector<Preset>& presets()
{
    // nei preset si accendono i personaggi: il resto (altezza, formante, ritardo, posto, colore) parte dal loro carattere
    auto on = [] (std::initializer_list<int> who) { std::vector<std::pair<juce::String, float>> v; for (int k : who) v.push_back ({ Processor::vid (k, "on"), 1.0f }); return v; };
    auto plus = [] (std::vector<std::pair<juce::String, float>> a, std::initializer_list<std::pair<juce::String, float>> b) { for (auto& x : b) a.push_back (x); return a; };
    static const std::vector<Preset> p = {
        { "INIT", "BASE", {} },
        // ritornelli
        { "CREW IN STUDIO", "HOOK", plus (on ({ 0, 1, 4, 5 }), { { "loose", 0.35f }, { "drift", 0.45f }, { "spread", 0.8f }, { "verb", 0.12f } }) },
        { "TUTTI DENTRO", "HOOK", plus (on ({ 0, 1, 2, 3, 4, 5 }), { { "loose", 0.45f }, { "drift", 0.5f }, { "spread", 0.9f }, { "shout", 0.25f }, { "verb", 0.18f } }) },
        { "HOOK TRAP", "HOOK", plus (on ({ 0, 2, 4 }), { { "loose", 0.2f }, { "drift", 0.3f }, { "spread", 0.85f }, { "shout", 0.3f }, { "echo", 0.18f }, { "echoDiv", 3 }, { "verb", 0.15f }, { "lowcut", 160 } }) },
        { "HOOK DRILL", "HOOK", plus (on ({ 0, 5, 4 }), { { "loose", 0.3f }, { "drift", 0.35f }, { "shout", 0.4f }, { "drive", 0.25f }, { "echo", 0.22f }, { "echoDiv", 2 }, { "lowcut", 150 }, { "highcut", 9000 } }) },
        { "STADIO", "HOOK", plus (on ({ 0, 1, 2, 3, 4, 5 }), { { "loose", 0.7f }, { "drift", 0.6f }, { "spread", 1.0f }, { "shout", 0.45f }, { "verb", 0.5f }, { "size", 0.85f }, { "lowcut", 180 } }) },
        // rinforzi (il raddoppio che non si sente ma ingrossa)
        { "DOPPIA STRETTA", "RINFORZI", plus (on ({ 4 }), { { "loose", 0.12f }, { "drift", 0.3f }, { "spread", 0.6f }, { "v5_delay", 12 } }) },
        { "DOPPIA LARGA", "RINFORZI", plus (on ({ 1, 4 }), { { "loose", 0.2f }, { "drift", 0.35f }, { "spread", 1.0f }, { "v2_pan", 0.85f }, { "v5_pan", -0.85f }, { "v2_formant", 0.05f } }) },
        { "SPESSORE", "RINFORZI", plus (on ({ 0, 4 }), { { "v1_lvl", -12 }, { "v5_lvl", -6 }, { "loose", 0.15f }, { "drift", 0.25f }, { "lowcut", 90 } }) },
        { "OTTAVE", "RINFORZI", plus (on ({ 0, 2 }), { { "loose", 0.1f }, { "drift", 0.2f }, { "spread", 0.5f } }) },
        { "TRIPLA", "RINFORZI", plus (on ({ 1, 3, 4 }), { { "loose", 0.25f }, { "drift", 0.4f }, { "spread", 0.95f }, { "v4_delay", 22 }, { "v4_tone", 0.2f } }) },
        // gridati
        { "PUNK HOOK", "GRIDATI", plus (on ({ 1, 4, 5 }), { { "loose", 0.55f }, { "drift", 0.7f }, { "shout", 0.7f }, { "drive", 0.4f }, { "verb", 0.1f }, { "lowcut", 200 } }) },
        { "RISSA", "GRIDATI", plus (on ({ 0, 1, 3, 4, 5 }), { { "loose", 0.8f }, { "drift", 0.8f }, { "shout", 0.8f }, { "drive", 0.55f }, { "spread", 1.0f }, { "verb", 0.2f }, { "lowcut", 220 } }) },
        { "MEGAFONO", "GRIDATI", plus (on ({ 1, 4 }), { { "shout", 0.6f }, { "drive", 0.7f }, { "lowcut", 450 }, { "highcut", 4500 }, { "loose", 0.3f } }) },
        // dark
        { "ROCCIOSO", "DARK", plus (on ({ 0, 5 }), { { "loose", 0.25f }, { "drift", 0.3f }, { "spread", 0.6f }, { "shout", 0.2f }, { "drive", 0.15f } }) },
        { "MOSTRI", "DARK", plus (on ({ 0, 5, 3 }), { { "v4_pitch", -12 }, { "v4_formant", -0.7f }, { "v1_formant", -0.8f }, { "loose", 0.45f }, { "drift", 0.6f }, { "shout", 0.35f }, { "verb", 0.25f } }) },
        { "SOTTOTERRA", "DARK", plus (on ({ 0, 4, 5 }), { { "highcut", 6000 }, { "verb", 0.4f }, { "size", 0.7f }, { "echo", 0.2f }, { "echoDiv", 4 }, { "loose", 0.35f } }) },
        // cyber
        { "SPAZIALE", "CYBER", plus (on ({ 2, 3 }), { { "loose", 0.3f }, { "drift", 0.5f }, { "spread", 1.0f }, { "verb", 0.5f }, { "size", 0.9f }, { "echo", 0.25f }, { "echoDiv", 4 } }) },
        { "ANDROIDI", "CYBER", plus (on ({ 0, 2, 3, 5 }), { { "v1_formant", 0.6f }, { "v3_formant", -0.6f }, { "v6_formant", 0.5f }, { "drift", 0.05f }, { "loose", 0.05f }, { "drive", 0.3f }, { "echo", 0.15f }, { "echoDiv", 0 } }) },
        { "CORO OLOGRAMMA", "CYBER", plus (on ({ 2, 3, 4 }), { { "v3_formant", 0.4f }, { "spread", 1.0f }, { "verb", 0.55f }, { "size", 0.95f }, { "highcut", 11000 } }) },
        // spazio e ad-lib
        { "AD-LIB DIETRO", "AD-LIB", plus (on ({ 3, 4 }), { { "loose", 0.45f }, { "drift", 0.4f }, { "spread", 1.0f }, { "verb", 0.3f }, { "crew", -6 } }) },
        { "ECO LANCIATA", "AD-LIB", plus (on ({ 1, 3 }), { { "echo", 0.5f }, { "echoDiv", 4 }, { "echoFb", 0.55f }, { "verb", 0.25f }, { "spread", 1.0f } }) },
        { "CHIESA", "AD-LIB", plus (on ({ 0, 2, 4 }), { { "verb", 0.55f }, { "size", 1.0f }, { "loose", 0.3f }, { "drift", 0.35f }, { "lowcut", 140 } }) },
    };
    static const std::vector<Preset> withNotes = [] (std::vector<Preset> v)
    {
        static const std::map<juce::String, juce::String> n = {
            { "INIT", "Nessuno acceso: la tua voce passa com'e'. Clicca i personaggi per farli entrare." },
            { "CREW IN STUDIO", "Quattro amici in cabina: ottava sotto, punk, unisono dietro e quinta. Il ritornello classico." },
            { "TUTTI DENTRO", "Tutti e sei, un po' gridati e con una stanza piccola: ritornello pieno." },
            { "HOOK TRAP", "Ottava sotto + ottava sopra + unisono, delay a 1/4: il ritornello trap moderno." },
            { "HOOK DRILL", "Scuro e aggressivo: drive, gridato, delay a 1/8 puntato, acuti tagliati." },
            { "STADIO", "Tutta la crew fuori tempo e stonata in un grande spazio: lo stadio che canta." },
            { "DOPPIA STRETTA", "Un raddoppio solo, vicinissimo: la voce si ingrossa senza sentirne due." },
            { "DOPPIA LARGA", "Due raddoppi aperti a destra e sinistra: il classico doppio stereo." },
            { "SPESSORE", "Ottava sotto bassissima e unisono sotto: spessore sotto la voce, quasi invisibile." },
            { "OTTAVE", "Ottava sotto e ottava sopra, precise: voce gigante e cyber." },
            { "TRIPLA", "Tre unisoni sparsi con ritardi diversi: tripla da rinforzo." },
            { "PUNK HOOK", "Gridato, sporco e fuori tempo: il ritornello punk-rap." },
            { "RISSA", "Cinque persone che urlano a caso: caos voluto per i drop." },
            { "MEGAFONO", "La crew strizzata in banda stretta e saturata, come da un megafono." },
            { "ROCCIOSO", "Ottava e quinta sotto: un coro di bassi duri sotto la voce." },
            { "MOSTRI", "Formanti giu' e ottave sotto: voci da mostro per intro e skit dark." },
            { "SOTTOTERRA", "Crew lontana e scura con riverbero e delay lungo: da cantina." },
            { "SPAZIALE", "Ottava sopra e casco lontano con stanza enorme e delay a 1/4 puntato." },
            { "ANDROIDI", "Tutti a tempo e intonati, formanti spostate e drive: crew di robot." },
            { "CORO OLOGRAMMA", "Voci alte e larghe in una stanza infinita: coro etereo dietro la strofa." },
            { "AD-LIB DIETRO", "Due voci lontane e larghe, piu' basse: per ripetere gli ad-lib." },
            { "ECO LANCIATA", "Delay lungo con tante ripetizioni: le parole della crew rimbalzano." },
            { "CHIESA", "Ottave e unisono in una chiesa grande: momento solenne." },
        };
        for (auto& p : v) if (auto it = n.find (p.name); it != n.end()) p.note = it->second;
        return v;
    } (p);
    return withNotes;
}
juce::StringArray Processor::allIds()
{
    juce::StringArray s; for (int k = 0; k < kVoices; ++k) for (auto* w : { "on", "lvl", "pan", "pitch", "formant", "delay", "tone" }) s.add (vid (k, w));
    for (auto* g : { "loose", "drift", "spread", "shout", "drive", "verb", "size", "echo", "echoDiv", "echoFb", "lowcut", "highcut", "crew" }) s.add (g);
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
        F (vid (k, "pitch"), n + " altezza", { -12, 12, 1 }, c.pitch);
        F (vid (k, "formant"), n + " formante", { -1, 1, 0.001f }, c.formant);
        F (vid (k, "delay"), n + " ritardo", { 0, 120, 0.1f }, c.delayMs);
        F (vid (k, "tone"), n + " colore", { -1, 1, 0.001f }, c.tone);
    }
    F ("loose", "Fuori tempo", { 0, 1 }, 0.35f); F ("drift", "Stonati", { 0, 1 }, 0.4f); F ("spread", "Larghezza", { 0, 1 }, 0.8f);
    F ("shout", "Gridato", { 0, 1 }, 0); F ("drive", "Drive", { 0, 1 }, 0);
    F ("verb", "Riverbero", { 0, 1 }, 0); F ("size", "Stanza", { 0, 1 }, 0.5f);
    F ("echo", "Delay", { 0, 1 }, 0); F ("echoFb", "Ripetizioni", { 0, 0.85f }, 0.35f);
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "echoDiv", 1 }, "Tempo delay", StringArray { "1/16", "1/8", "1/8.", "1/4", "1/4.", "1/2" }, 3));
    NormalisableRange<float> lc (20, 600); lc.setSkewForCentre (140); F ("lowcut", "Taglio bassi crew", lc, 120);
    NormalisableRange<float> hc (2000, 20000); hc.setSkewForCentre (8000); F ("highcut", "Taglio alti crew", hc, 16000);
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
void Processor::prepareToPlay (double s, int mb) { eng.prepare (s, juce::jmax (mb, 32)); setLatencySamples (eng.latency()); }
Settings Processor::settings() const
{
    Settings S;
    for (int k = 0; k < kVoices; ++k) S.v[(size_t) k] = { value (vid (k, "on")) > 0.5f, value (vid (k, "lvl")), value (vid (k, "pan")), value (vid (k, "pitch")), value (vid (k, "formant")), value (vid (k, "delay")), value (vid (k, "tone")) };
    S.loose = value ("loose"); S.drift = value ("drift"); S.spread = value ("spread"); S.shout = value ("shout"); S.drive = value ("drive");
    S.verb = value ("verb"); S.size = value ("size"); S.echo = value ("echo"); S.echoFb = value ("echoFb"); S.echoDiv = juce::roundToInt (value ("echoDiv"));
    S.lowCut = value ("lowcut"); S.highCut = value ("highcut"); S.crewDb = value ("crew"); S.dryDb = value ("dry"); S.bpm = hostBpm;
    return S;
}
void Processor::processBlock (juce::AudioBuffer<float>& b, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals nd;
    const int n = b.getNumSamples(); if (n == 0) return;
    if (getTotalNumInputChannels() == 1) b.copyFrom (1, 0, b, 0, 0, n);
    uiIn.store (juce::jmax (b.getMagnitude (0, 0, n), uiIn.load() * 0.8f));
    if (auto* ph = getPlayHead()) if (auto pos = ph->getPosition()) if (auto bpm = pos->getBpm()) if (*bpm > 20 && *bpm < 400) hostBpm = *bpm;
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
