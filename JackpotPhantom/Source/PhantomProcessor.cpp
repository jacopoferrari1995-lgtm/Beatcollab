#include "PhantomProcessor.h"
#include "PhantomEditor.h"

namespace jphn
{
const std::vector<Preset>& presets()
{
    using V = std::vector<std::pair<juce::String, float>>;
    static const std::vector<Preset> p = {
        { "INIT", "BASE", {}, "Nessun fantasma: la voce passa identica. Alza un fantasma per farlo uscire." },
        // uno alla volta
        { "SUSSURRO DIETRO", "UNO SOLO", V { { "g1", 0.6f }, { "c1", 0.6f }, { "delay", 30 }, { "space", 0.35f } }, "Un sussurro che ripete le tue parole da dietro la spalla." },
        { "L'OMBRA", "UNO SOLO", V { { "g2", 0.55f }, { "c2", 0.0f }, { "dark", 0.55f }, { "space", 0.3f } }, "Un'altra te un'ottava sotto, scura: sotto le strofe dark." },
        { "SPECCHIO ROTTO", "UNO SOLO", V { { "g3", 0.6f }, { "c3", 0.5f }, { "space", 0.45f }, { "drift", 0.5f } }, "Le parole tornano al contrario, a pezzi da 1/8, girando intorno." },
        { "AURA", "UNO SOLO", V { { "g4", 0.5f }, { "c4", 1.0f }, { "space", 0.5f } }, "Un'aura un'ottava sopra che si scioglie: per i ritornelli eterei." },
        // fantasmi per i pezzi
        { "STANZA INFESTATA", "DARK", V { { "g1", 0.5f }, { "g2", 0.4f }, { "g3", 0.35f }, { "delay", 60 }, { "drift", 0.5f }, { "dark", 0.5f }, { "space", 0.6f } }, "Sussurro, ombra e riflesso insieme in una stanza buia." },
        { "INTRO MALEDETTA", "DARK", V { { "g3", 0.7f }, { "c3", 0.75f }, { "g4", 0.4f }, { "c4", 0.0f }, { "dark", 0.4f }, { "space", 0.8f }, { "dry", -6 } }, "Al contrario a pezzi lunghi con l'alone alla quinta: per le intro." },
        { "VOCE DAL POZZO", "DARK", V { { "g2", 0.65f }, { "c2", 0.3f }, { "delay", 120 }, { "dark", 0.75f }, { "space", 0.7f } }, "L'ombra ritardata, scurissima, in fondo a un pozzo." },
        { "PARANOIA", "DARK", V { { "g1", 0.7f }, { "c1", 0.85f }, { "delay", 80 }, { "drift", 0.9f }, { "width", 1.0f }, { "space", 0.25f } }, "Sussurri che ti girano intorno nella testa." },
        // trap / rap
        { "HOOK FANTASMA", "TRAP", V { { "g1", 0.35f }, { "g4", 0.35f }, { "c4", 1.0f }, { "space", 0.4f }, { "width", 0.9f } }, "Sussurro largo e aura sopra: il ritornello che respira." },
        { "AD-LIB SPETTRALI", "TRAP", V { { "g3", 0.55f }, { "c3", 0.25f }, { "g1", 0.3f }, { "drift", 0.6f }, { "space", 0.4f } }, "Pezzi al contrario a 1/16 e sussurri: dietro gli ad-lib." },
        { "DOPPIA OMBRA", "TRAP", V { { "g2", 0.45f }, { "c2", 1.0f }, { "g1", 0.25f }, { "space", 0.2f } }, "Ombra alla quarta sotto e un filo di sussurro: strofa piu' cattiva." },
        { "DRILL NEBBIA", "TRAP", V { { "g2", 0.4f }, { "c2", 0.0f }, { "g3", 0.3f }, { "c3", 0.5f }, { "dark", 0.6f }, { "space", 0.5f }, { "delay", 40 } }, "Ombra all'ottava e riflesso a 1/8 nel buio: per la drill." },
        { "CORO DI MORTI", "TRAP", V { { "g1", 0.45f }, { "g2", 0.45f }, { "g3", 0.3f }, { "g4", 0.4f }, { "drift", 0.7f }, { "dark", 0.45f }, { "space", 0.75f }, { "width", 1.0f } }, "Tutti e quattro: un coro di fantasmi per i momenti grossi." },
        // atmosfere
        { "SEGNALE DAL VUOTO", "ATMOSFERA", V { { "g4", 0.6f }, { "c4", 0.0f }, { "g3", 0.3f }, { "c3", 1.0f }, { "space", 0.9f }, { "dry", -8 } }, "Quasi solo fantasmi: la voce arriva da lontano, per skit e outro." },
        { "RICORDO", "ATMOSFERA", V { { "g1", 0.4f }, { "c1", 0.3f }, { "g4", 0.3f }, { "c4", 1.0f }, { "delay", 200 }, { "space", 0.6f }, { "dark", 0.35f } }, "Le parole tornano tardi, morbide: come un ricordo." },
        { "NEON PIOGGIA", "ATMOSFERA", V { { "g1", 0.35f }, { "c1", 0.9f }, { "g4", 0.35f }, { "c4", 0.5f }, { "space", 0.55f }, { "width", 1.0f }, { "drift", 0.4f } }, "Sussurri chiari e aura larga: notte in citta'." },
        { "SOLO FANTASMI", "ATMOSFERA", V { { "g1", 0.5f }, { "g2", 0.4f }, { "g4", 0.4f }, { "space", 0.6f }, { "dry", -40 } }, "Togli la voce: restano solo i fantasmi. Per transizioni." },
    };
    return p;
}

juce::AudioProcessorValueTreeState::ParameterLayout Processor::layout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    auto F = [&l] (const char* id, const char* n, NormalisableRange<float> r, float d) { l.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, n, r, d)); };
    // fantasmi a zero: la voce passa identica
    F ("g1", "Sussurro", { 0, 1 }, 0); F ("g2", "Ombra", { 0, 1 }, 0); F ("g3", "Riflesso", { 0, 1 }, 0); F ("g4", "Alone", { 0, 1 }, 0);
    F ("c1", "Aria sussurro", { 0, 1 }, 0.5f); F ("c2", "Intervallo ombra", { 0, 1 }, 0.0f); F ("c3", "Pezzi riflesso", { 0, 1 }, 0.5f); F ("c4", "Intervallo alone", { 0, 1 }, 1.0f);
    F ("delay", "Ritardo", { 0, 400, 0.1f }, 0); F ("drift", "Deriva", { 0, 1 }, 0.3f); F ("dark", "Buio", { 0, 1 }, 0.3f); F ("space", "Spazio", { 0, 1 }, 0.4f);
    F ("width", "Larghezza", { 0, 1 }, 0.7f); F ("ghost", "Fantasmi", { -24, 6, 0.01f }, 0); F ("dry", "Tu", { -40, 6, 0.01f }, 0);
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
    for (int g = 0; g < kGhosts; ++g) { S.lvl[(size_t) g] = value ("g" + juce::String (g + 1)); S.chr[(size_t) g] = value ("c" + juce::String (g + 1)); }
    S.delayMs = value ("delay"); S.drift = value ("drift"); S.dark = value ("dark"); S.space = value ("space"); S.width = value ("width");
    S.ghostDb = value ("ghost"); S.dryDb = value ("dry") <= -39.9f ? -200.0f : value ("dry"); S.bpm = hostBpm;
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
    uiOut.store (juce::jmax (b.getMagnitude (0, 0, n), uiOut.load() * 0.8f));
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
    juce::XmlElement x ("PHANTOMPRESET"); x.setAttribute ("name", name);
    for (auto& id : allIds()) x.setAttribute (id, value (id));
    const bool ok = x.writeTo (presetFolder().getChildFile (juce::File::createLegalFileName (name) + ".phantom"));
    if (ok) userName = name;
    return ok;
}
bool Processor::loadUserPreset (const juce::File& f)
{
    auto x = juce::XmlDocument::parse (f); if (x == nullptr || ! x->hasTagName ("PHANTOMPRESET")) return false;
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
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new jphn::Processor(); }
