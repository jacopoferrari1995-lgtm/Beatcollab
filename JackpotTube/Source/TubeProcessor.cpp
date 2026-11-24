#include "TubeProcessor.h"
#include "TubeEditor.h"

namespace jtbx
{
juce::AudioProcessorValueTreeState::ParameterLayout Processor::layout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    auto F = [&l] (const String& id, const String& n, NormalisableRange<float> r, float d) { l.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 2 }, n, r, d)); };
    // tutto a zero al caricamento = suono intatto (solo la latenza compensata)
    F ("drive", "Drive", { 0, 10, 0.01f }, 0);
    StringArray ch; for (int i = 0; i < kChars; ++i) ch.add (charOf (i).name);
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "char", 2 }, "Carattere", ch, 0));
    F ("warmth", "Calore", { 0, 10, 0.01f }, 0);
    F ("air", "Aria", { 0, 10, 0.01f }, 0);
    F ("enh", "Enhancer", { 0, 10, 0.01f }, 0);
    F ("mix", "Mix", { 0, 1 }, 1);
    F ("out", "Output", { -12, 12, 0.01f }, 0);
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "auto", 2 }, "Volume automatico", true));
    return l;
}
Processor::Processor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true).withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, &undo, "PARAMS", layout()) {}
void Processor::prepareToPlay (double s, int) { eng.prepare (s, kChunk); setLatencySamples (eng.latency()); }
Settings Processor::settings() const
{
    Settings S;
    S.drive = value ("drive"); S.character = juce::roundToInt (value ("char")); S.warmth = value ("warmth"); S.air = value ("air"); S.enh = value ("enh");
    S.mix = value ("mix"); S.outDb = value ("out"); S.autoGain = value ("auto") > 0.5f;
    return S;
}
void Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals nd;
    const int n = buffer.getNumSamples(); if (n == 0) return;
    if (getTotalNumInputChannels() == 1) buffer.copyFrom (1, 0, buffer, 0, 0, n);
    const auto S = settings();
    float* L = buffer.getWritePointer (0); float* R = buffer.getWritePointer (1);
    for (int off = 0; off < n; off += kChunk) eng.process (L + off, R + off, juce::jmin (kChunk, n - off), S);
}
void Processor::loadPreset (int i)
{
    if (! juce::isPositiveAndBelow (i, (int) presets().size())) return;
    preset = i; userName = {}; undo.beginNewTransaction(); jkit::applyPreset (*this, apvts, presets()[(size_t) i], { "out", "auto" });
}
bool Processor::saveUser (const juce::File& f)
{
    auto x = apvts.copyState().createXml(); if (! x) return false;
    const bool ok = x->writeTo (f); if (ok) userName = f.getFileNameWithoutExtension(); return ok;
}
bool Processor::loadUser (const juce::File& f)
{
    auto x = juce::XmlDocument::parse (f); if (! x) return false;
    auto t = juce::ValueTree::fromXml (*x); if (! t.isValid()) return false;
    for (auto* pp : getParameters()) if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (pp))
    {
        for (int i = 0; i < t.getNumChildren(); ++i) { auto c = t.getChild (i); if (c.getProperty ("id").toString() == rp->paramID) { rp->setValueNotifyingHost (rp->convertTo0to1 ((float) (double) c.getProperty ("value"))); break; } }
    }
    userName = f.getFileNameWithoutExtension(); return true;
}
void Processor::getStateInformation (juce::MemoryBlock& d)
{
    auto s = apvts.copyState(); s.setProperty ("preset", preset, nullptr); s.setProperty ("user", userName, nullptr);
    if (auto x = s.createXml()) copyXmlToBinary (*x, d);
}
void Processor::setStateInformation (const void* data, int size)
{
    if (auto x = getXmlFromBinary (data, size)) { auto t = juce::ValueTree::fromXml (*x); if (t.isValid()) { preset = t.getProperty ("preset", 0); userName = t.getProperty ("user", "").toString(); apvts.replaceState (t); } }
}
juce::AudioProcessorEditor* Processor::createEditor() { return new Editor (*this); }

using jkit::PV;
// carattere, drive, calore, aria, enhancer, mix
static PV T (int c, float d, float w, float a, float e, float mix = 1) { return { { "char", (float) c }, { "drive", d }, { "warmth", w }, { "air", a }, { "enh", e }, { "mix", mix } }; }
enum { CALDO, SPINTO, CREMA, VETRO, FUOCO, SPESSO };
const std::vector<jkit::Preset>& presets()
{
    static const std::vector<jkit::Preset> p = {
        { "Init", "Tutto a zero: suono intatto. Gira DRIVE e scegli il carattere.", {} },
        { "Voce | Rap davanti", "SPINTO: i medi mordono e la voce esce dal beat.", T (SPINTO, 4.5f, 1.5f, 3, 4) },
        { "Voce | Calda e piena", "CALDO con un filo di calore: voce vicina, morbida.", T (CALDO, 5, 3, 2, 2) },
        { "Voce | Aria R&B", "VETRO: pulita, lucida, aria in alto senza fischiare.", T (VETRO, 3.5f, 1, 5, 3) },
        { "Voce | Ad-lib sporche", "FUOCO al 60% di mix: grinta per doppie e ad-lib.", T (FUOCO, 6.5f, 2, 1, 3, 0.6f) },
        { "Voce | Ritornello largo", "SPESSO: denso e incollato, il ritornello si alza.", T (SPESSO, 4, 2, 4, 3) },
        { "808 | Si sente sul telefono", "CALDO + CALORE alto: l'808 tira fuori le armoniche e passa dalle casse piccole.", T (CALDO, 4, 7, 0, 2) },
        { "808 | Ringhio drill", "FUOCO spinto: l'808 distorce e morde.", T (FUOCO, 7.5f, 4, 0, 2) },
        { "Basso | Spesso", "CREMA: il basso si allarga e resta morbido in alto.", T (CREMA, 5.5f, 4, 0, 1) },
        { "Batteria | Pugno", "SPINTO sul bus: cassa e rullante prendono attacco.", T (SPINTO, 5, 2, 2, 2) },
        { "Batteria | Boom bap", "SPESSO e poca aria: suono da campionatore, incollato.", T (SPESSO, 6, 3, 0, 1) },
        { "Batteria | Rotta", "FUOCO a meta' mix: distorsione parallela sulla batteria.", T (FUOCO, 8.5f, 2, 1, 0, 0.45f) },
        { "Chitarra | Crunch", "SPINTO spinto: il crunch del pentodo.", T (SPINTO, 7, 1, 1, 2) },
        { "Chitarra | Campione soul", "CREMA: il campione si scalda come da vinile.", T (CREMA, 4.5f, 3, 1, 1) },
        { "Keys | Rhodes in avanti", "CALDO con enhancer: il Rhodes esce senza alzarlo.", T (CALDO, 4, 2, 2, 4) },
        { "Synth | Lead che taglia", "FUOCO con enhancer: il lead passa sopra tutto.", T (FUOCO, 5, 0, 3, 4) },
        { "Pad | Largo e caldo", "SPESSO leggero e aria: il pad si apre.", T (SPESSO, 3, 2, 4, 1) },
        { "Mix | Bus caldo", "CREMA leggera sul mix: colla e corpo.", T (CREMA, 3, 2, 2, 1) },
        { "Mix | Brillante", "VETRO sul mix: pulito, armoniche in alto.", T (VETRO, 3, 1, 3, 2) },
        { "Master | Ultimo tocco", "CALDO al 50% di mix: calore che non si sente ma si sente.", T (CALDO, 3, 1, 1.5f, 1, 0.5f) } };
    return p;
}
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new jtbx::Processor(); }
