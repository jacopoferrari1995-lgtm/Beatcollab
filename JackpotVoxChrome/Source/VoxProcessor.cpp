#include "VoxProcessor.h"
#include "VoxEditor.h"

namespace jvox
{
const std::vector<Preset>& presets()
{
    using V = std::vector<std::pair<juce::String, float>>;
    static const std::vector<Preset> p = {
        { "INIT", "BASE", {}, "Tutto spento: la voce passa identica. Alza CROMO o i THROW per entrare nel sistema." },
        // voci cyborg
        { "CROMATO", "CYBORG", V { { "cromo", 0.35f }, { "drive", 0.15f }, { "width", 0.3f } }, "Un velo di metallo sulla voce: si sente il cromo ma resta tua." },
        { "CYBORG", "CYBORG", V { { "cromo", 0.55f }, { "shift", -0.12f }, { "bit", 0.15f }, { "drive", 0.25f }, { "width", 0.4f } }, "Meta' uomo meta' macchina: risonanze, un filo di bit e voce spostata in basso." },
        { "ANDROIDE", "CYBORG", V { { "robot", 0.35f }, { "cromo", 0.3f }, { "shift", 0.06f }, { "band", 0.2f } }, "Voce sintetica, dritta e un po' stretta." },
        { "DIRETTIVA", "CYBORG", V { { "robot", 0.6f }, { "band", 0.55f }, { "drive", 0.5f }, { "cromo", 0.2f }, { "out", -3.0f } }, "La voce del sistema dagli altoparlanti: robot, banda stretta e distorta." },
        { "CARNE E METALLO", "CYBORG", V { { "cromo", 0.75f }, { "drive", 0.35f }, { "width", 0.55f } }, "Cromo pesante e largo: la strofa cattiva." },
        { "DISERTORE", "CYBORG", V { { "shift", -0.3f }, { "bit", 0.35f }, { "drive", 0.3f }, { "cromo", 0.25f } }, "Voce spostata e corrotta: chi e' scappato dal sistema." },
        { "CODICE ROTTO", "CYBORG", V { { "bit", 0.7f }, { "drive", 0.4f }, { "band", 0.3f }, { "robot", 0.2f }, { "out", -4.5f } }, "Bit spezzati e saturazione: la voce che si rompe." },
        // throw
        { "THROW ECO", "THROW", V { { "eco", 0.6f }, { "fb", 0.45f }, { "div", 3 } }, "Voce pulita, eco a 1/4 solo sull'ultima parola di ogni frase." },
        { "THROW DRILL", "THROW", V { { "eco", 0.65f }, { "fb", 0.5f }, { "div", 2 }, { "cromo", 0.2f }, { "space", 0.25f } }, "Eco a 1/8 puntato a fine frase con un po' di spazio: il throw drill." },
        { "THROW SPAZIO", "THROW", V { { "space", 0.7f }, { "eco", 0.2f } }, "A fine frase la parola esplode in un riverbero scuro." },
        { "THROW GLITCH", "THROW", V { { "glitch", 0.7f }, { "eco", 0.25f }, { "div", 3 } }, "L'ultima sillaba si incanta e rallenta come una macchina che si spegne." },
        { "TUTTI I THROW", "THROW", V { { "eco", 0.5f }, { "space", 0.45f }, { "glitch", 0.45f }, { "fb", 0.5f }, { "div", 4 } }, "Eco, spazio e glitch insieme a ogni fine frase." },
        // completi
        { "PROPAGANDA", "SISTEMA", V { { "robot", 0.3f }, { "band", 0.45f }, { "drive", 0.35f }, { "eco", 0.55f }, { "div", 5 }, { "fb", 0.55f }, { "space", 0.5f } }, "Il comizio del regime: voce da altoparlante con eco lunga sui palazzi." },
        { "SORVEGLIANZA", "SISTEMA", V { { "cromo", 0.4f }, { "bit", 0.25f }, { "band", 0.3f }, { "glitch", 0.5f }, { "space", 0.3f } }, "Ti stanno registrando: voce digitale che a fine frase si inceppa." },
        { "COPRIFUOCO", "SISTEMA", V { { "cromo", 0.5f }, { "shift", -0.18f }, { "drive", 0.3f }, { "space", 0.6f }, { "eco", 0.35f }, { "div", 3 } }, "Scura, bassa e larga: per le intro e i ritornelli lenti." },
        { "RESISTENZA", "SISTEMA", V { { "cromo", 0.3f }, { "drive", 0.45f }, { "width", 0.5f }, { "eco", 0.45f }, { "div", 1 }, { "glitch", 0.3f } }, "Grezza e aggressiva, eco veloce e glitch: per la strofa d'attacco." },
        { "IA SUPREMA", "SISTEMA", V { { "robot", 0.5f }, { "cromo", 0.6f }, { "shift", 0.1f }, { "width", 0.7f }, { "space", 0.55f } }, "La macchina che parla dall'alto: robot largo in uno spazio enorme." },
        { "ULTIMA TRASMISSIONE", "SISTEMA", V { { "bit", 0.45f }, { "band", 0.6f }, { "drive", 0.3f }, { "glitch", 0.6f }, { "eco", 0.4f }, { "div", 4 }, { "space", 0.35f } }, "Radio militare che muore: banda stretta, bit e glitch a fine frase." },
        { "ACCIAIO", "SISTEMA", V { { "cromo", 0.85f }, { "shift", -0.06f }, { "drive", 0.2f }, { "width", 0.6f }, { "space", 0.3f } }, "Cromo a palla con un po' di spazio: voce di metallo liquido." },
    };
    return p;
}

juce::AudioProcessorValueTreeState::ParameterLayout Processor::layout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    auto F = [&l] (const char* id, const char* n, NormalisableRange<float> r, float d) { l.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, n, r, d)); };
    // tutto a zero: la voce passa identica
    F ("cromo", "Cromo", { 0, 1 }, 0); F ("robot", "Robot", { 0, 1 }, 0); F ("shift", "Sposta", { -1, 1 }, 0); F ("bit", "Bit", { 0, 1 }, 0);
    F ("drive", "Drive", { 0, 1 }, 0); F ("band", "Banda", { 0, 1 }, 0); F ("width", "Larghezza", { 0, 1 }, 0);
    F ("eco", "Throw eco", { 0, 1 }, 0); F ("space", "Throw spazio", { 0, 1 }, 0); F ("glitch", "Throw glitch", { 0, 1 }, 0);
    StringArray dv; for (int i = 0; i < 6; ++i) dv.add (divName (i));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "div", 1 }, "Tempo eco", dv, 3));
    F ("fb", "Ripetizioni", { 0, 0.9f }, 0.45f); F ("sens", "Soglia", { 0, 1 }, 0.5f); F ("out", "Uscita", { -18, 6, 0.01f }, 0);
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
    Settings S; S.cromo = value ("cromo"); S.robot = value ("robot"); S.shift = value ("shift"); S.bit = value ("bit"); S.drive = value ("drive"); S.band = value ("band");
    S.width = value ("width"); S.eco = value ("eco"); S.space = value ("space"); S.glitch = value ("glitch"); S.div = juce::roundToInt (value ("div"));
    S.fb = value ("fb"); S.sens = value ("sens"); S.outDb = value ("out"); S.bpm = hostBpm;
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
    juce::XmlElement x ("VOXPRESET"); x.setAttribute ("name", name);
    for (auto& id : allIds()) x.setAttribute (id, value (id));
    const bool ok = x.writeTo (presetFolder().getChildFile (juce::File::createLegalFileName (name) + ".voxchrome"));
    if (ok) userName = name;
    return ok;
}
bool Processor::loadUserPreset (const juce::File& f)
{
    auto x = juce::XmlDocument::parse (f); if (x == nullptr || ! x->hasTagName ("VOXPRESET")) return false;
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
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new jvox::Processor(); }
