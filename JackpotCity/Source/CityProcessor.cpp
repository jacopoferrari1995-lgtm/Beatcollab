#include "CityProcessor.h"
#include "CityEditor.h"

namespace jcity
{
const std::vector<Preset>& presets()
{
    using V = std::vector<std::pair<juce::String, float>>;
    auto sc = [] (int s, float city, float dist, float dis, float amb, float wid, float tone = 0, float out = 0)
    { return V { { "scene", (float) s }, { "city", city }, { "distance", dist }, { "disturb", dis }, { "ambience", amb }, { "width", wid }, { "tone", tone }, { "out", out } }; };
    static const std::vector<Preset> p = {
        { "INIT", "BASE", {}, "Fuori dalla citta': la voce passa identica. Gira DENTRO LA CITTA' per entrarci." },
        { "SUL TETTO", "BASE", sc (4, 0.45f, 0.15f, 0.2f, 0.4f, 0.6f, 0.4f), "Un velo di pioggia e di stanza intorno alla voce: dentro la citta' ma ancora tu." },
        // radio pirata
        { "RADIO PIRATA", "RADIO", sc (0, 1, 0.2f, 0.5f, 0.3f, 0.2f), "La frequenza rubata: banda stretta, fruscio, qualche sfumata." },
        { "STAZIONE LONTANA", "RADIO", sc (0, 1, 0.65f, 0.8f, 0.5f, 0.3f, -0.2f), "La radio presa male dall'altra parte della citta': sfuma e frigge." },
        { "INTRO RADIO", "RADIO", sc (0, 0.75f, 0.1f, 0.35f, 0.2f, 0.3f, 0.2f), "Per le intro: radio pulita a meta', poi la togli quando entra il beat." },
        { "AUTORADIO", "RADIO", sc (0, 0.9f, 0.35f, 0.25f, 0.6f, 0.5f, -0.4f), "La radio in macchina nel traffico: piu' calda, con la strada intorno." },
        // megafono
        { "MEGAFONO", "MEGAFONO", sc (1, 1, 0.25f, 0.5f, 0.5f, 0.3f), "L'altoparlante in strada: duro, medio e con i palazzi che rispondono." },
        { "COMIZIO", "MEGAFONO", sc (1, 1, 0.5f, 0.35f, 0.85f, 0.6f), "Il discorso in piazza: tanto rimbalzo, lontano." },
        { "POLIZIA", "MEGAFONO", sc (1, 1, 0.15f, 0.9f, 0.35f, 0.25f, 0.3f), "Megafono a palla, distorto: per ad-lib e grida nel ritornello." },
        // chiamata
        { "CHIAMATA", "CHIAMATA", sc (2, 1, 0.0f, 0.4f, 0.2f, 0.0f), "Telefono criptato: stretto e un po' digitale, ogni tanto taglia." },
        { "LINEA ROTTA", "CHIAMATA", sc (2, 1, 0.1f, 0.95f, 0.2f, 0.0f), "La chiamata che si rompe: tagli continui e bit spezzati." },
        { "SEGRETERIA", "CHIAMATA", sc (2, 0.85f, 0.2f, 0.15f, 0.4f, 0.2f, -0.2f), "Il messaggio lasciato in segreteria, quasi pulito." },
        // ologramma
        { "OLOGRAMMA", "OLOGRAMMA", sc (3, 1, 0.2f, 0.5f, 0.5f, 0.7f), "La proiezione che trema e perde segnale ogni tanto." },
        { "SEGNALE PERSO", "OLOGRAMMA", sc (3, 1, 0.3f, 1.0f, 0.5f, 0.8f), "L'ologramma che si incanta: ripete pezzi e salta. Per i drop." },
        { "MESSAGGIO DAL FUTURO", "OLOGRAMMA", sc (3, 0.8f, 0.5f, 0.3f, 0.8f, 0.9f, 0.3f), "Ologramma lontano in una stanza enorme: per le intro e gli skit." },
        { "IA DI QUARTIERE", "OLOGRAMMA", sc (3, 0.65f, 0.05f, 0.25f, 0.25f, 0.5f, 0.2f), "Un filo di ologramma sulla voce, quasi pulita: la voce dell'IA." },
        // club
        { "FUORI DAL CLUB", "CLUB", sc (4, 1, 0.4f, 0.3f, 0.7f, 0.6f), "Sei fuori, piove, il pezzo arriva dal muro: solo bassi e pioggia." },
        { "IN FILA", "CLUB", sc (4, 1, 0.15f, 0.2f, 0.5f, 0.5f, 0.5f), "Davanti alla porta: si sente di piu', la pioggia ancora intorno." },
        { "DOPO LA SERATA", "CLUB", sc (4, 0.9f, 0.7f, 0.1f, 1.0f, 0.8f), "Lontano, a casa sotto la pioggia: per gli outro." },
        // metro
        { "METRO", "METRO", sc (5, 1, 0.3f, 0.3f, 0.6f, 0.6f), "La galleria: ferro, rimbombo e treni che passano." },
        { "BANCHINA VUOTA", "METRO", sc (5, 1, 0.6f, 0.15f, 0.9f, 0.8f, -0.2f), "Notte fonda in metro: voce lontana e una galleria infinita." },
        { "TRENO IN ARRIVO", "METRO", sc (5, 1, 0.2f, 0.4f, 1.0f, 0.7f), "Il treno che entra in stazione: rimbombo forte sotto la voce." },
        { "SOTTOPASSO", "METRO", sc (5, 0.7f, 0.1f, 0.2f, 0.35f, 0.5f, 0.3f), "Una galleria corta e vicina: rimbombo sulla voce, senza perderla." },
    };
    return p;
}

juce::AudioProcessorValueTreeState::ParameterLayout Processor::layout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    StringArray sn; for (auto& s : scenes()) sn.add (s.name);
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "scene", 1 }, "Scena", sn, 0));
    auto F = [&l] (const char* id, const char* n, NormalisableRange<float> r, float d) { l.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, n, r, d)); };
    F ("city", "Dentro la citta'", { 0, 1 }, 0);   // a zero l'audio passa identico
    F ("distance", "Distanza", { 0, 1 }, 0.3f); F ("disturb", "Disturbo", { 0, 1 }, 0.5f); F ("ambience", "Ambiente", { 0, 1 }, 0.5f);
    F ("width", "Larghezza", { 0, 1 }, 0.5f); F ("tone", "Tono", { -1, 1 }, 0); F ("out", "Uscita", { -18, 6, 0.01f }, 0);
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
    Settings S; S.scene = juce::roundToInt (value ("scene")); S.city = value ("city"); S.distance = value ("distance"); S.disturb = value ("disturb");
    S.ambience = value ("ambience"); S.width = value ("width"); S.tone = value ("tone"); S.outDb = value ("out");
    return S;
}
void Processor::processBlock (juce::AudioBuffer<float>& b, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals nd;
    const int n = b.getNumSamples(); if (n == 0) return;
    if (getTotalNumInputChannels() == 1) b.copyFrom (1, 0, b, 0, 0, n);
    uiIn.store (juce::jmax (b.getMagnitude (0, 0, n), uiIn.load() * 0.8f));
    eng.process (b.getWritePointer (0), b.getWritePointer (1), n, settings());
    uiOut.store (juce::jmax (b.getMagnitude (0, 0, n), uiOut.load() * 0.8f)); uiGate.store (eng.gateView());
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
    juce::XmlElement x ("CITYPRESET"); x.setAttribute ("name", name);
    for (auto& id : allIds()) x.setAttribute (id, value (id));
    const bool ok = x.writeTo (presetFolder().getChildFile (juce::File::createLegalFileName (name) + ".city"));
    if (ok) userName = name;
    return ok;
}
bool Processor::loadUserPreset (const juce::File& f)
{
    auto x = juce::XmlDocument::parse (f); if (x == nullptr || ! x->hasTagName ("CITYPRESET")) return false;
    undo.beginNewTransaction();
    for (auto& id : allIds()) if (x->hasAttribute (id)) setP (id, (float) x->getDoubleAttribute (id));
    userName = x->getStringAttribute ("name", f.getFileNameWithoutExtension());
    return true;
}
void Processor::getStateInformation (juce::MemoryBlock& d)
{
    auto st = apvts.copyState(); std::unique_ptr<juce::XmlElement> x (st.createXml());
    x->setAttribute ("preset", preset); x->setAttribute ("user", userName); copyXmlToBinary (*x, d);
}
void Processor::setStateInformation (const void* data, int size)
{
    std::unique_ptr<juce::XmlElement> x (getXmlFromBinary (data, size));
    if (x == nullptr || ! x->hasTagName (apvts.state.getType())) return;
    preset = x->getIntAttribute ("preset", 0); userName = x->getStringAttribute ("user");
    apvts.replaceState (juce::ValueTree::fromXml (*x));
}
juce::AudioProcessorEditor* Processor::createEditor() { return new Editor (*this); }
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new jcity::Processor(); }
