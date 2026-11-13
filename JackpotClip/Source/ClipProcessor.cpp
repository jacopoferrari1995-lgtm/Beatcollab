#include "ClipProcessor.h"
#include "ClipEditor.h"

namespace jclp
{
juce::AudioProcessorValueTreeState::ParameterLayout Processor::layout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    auto F = [&l] (const char* id, const char* n, NormalisableRange<float> r, float d) { l.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, n, r, d)); };
    auto B = [&l] (const char* id, const char* n, bool d) { l.add (std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, n, d)); };
    auto Cc = [&l] (const char* id, const char* n, StringArray s, int d) { l.add (std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, n, s, d)); };
    StringArray modes; for (int i = 0; i < kModes; ++i) modes.add (modeName (i));
    NormalisableRange<float> sp (40, 400); sp.setSkewForCentre (120);
    // di partenza: HARD a 0 dB con tetto a 0 dB = sotto lo zero non cambia niente
    F ("drive", "Drive", { 0, 24, 0.01f }, 0); F ("ceil", "Tetto", { -12, 0, 0.01f }, 0);
    Cc ("mode", "Modo", modes, HARD); Cc ("os", "Sovracampionamento", { "OFF", "2x", "4x", "8x" }, 2);
    F ("punch", "Punch", { -1, 1 }, 0); F ("mix", "Mix", { 0, 1 }, 1); F ("out", "Output", { -12, 12, 0.01f }, 0);
    B ("split", "Split bassi", false); F ("splitHz", "Frequenza split", sp, 120);
    B ("link", "Volume compensato", false); B ("delta", "Ascolta il tagliato", false);
    return l;
}
Processor::Processor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true).withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", layout()) {}
bool Processor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto o = l.getMainOutputChannelSet(), i = l.getMainInputChannelSet();
    return o == juce::AudioChannelSet::stereo() && (i == o || i == juce::AudioChannelSet::mono());
}
void Processor::prepareToPlay (double s, int)
{
    sr = s; eng.prepare (s, kChunk); lastOs = juce::roundToInt (value ("os")); setLatencySamples (eng.latency (lastOs));
}
Settings Processor::settings() const
{
    Settings S;
    S.driveDb = value ("drive"); S.ceilDb = value ("ceil"); S.mode = juce::roundToInt (value ("mode")); S.os = juce::roundToInt (value ("os"));
    S.punch = value ("punch"); S.mix = value ("mix"); S.outDb = value ("out"); S.split = value ("split") > 0.5f; S.splitHz = value ("splitHz");
    S.link = value ("link") > 0.5f; S.delta = value ("delta") > 0.5f;
    return S;
}
void Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals nd;
    const int n = buffer.getNumSamples(); if (n == 0) return;
    if (getTotalNumInputChannels() == 1) buffer.copyFrom (1, 0, buffer, 0, 0, n);
    const auto S = settings();
    if (S.os != lastOs) { lastOs = S.os; setLatencySamples (eng.latency (S.os)); }
    float* L = buffer.getWritePointer (0); float* R = buffer.getWritePointer (1);
    for (int off = 0; off < n; off += kChunk) eng.process (L + off, R + off, juce::jmin (kChunk, n - off), S, hist);
}
void Processor::loadPreset (int i)
{
    if (! juce::isPositiveAndBelow (i, (int) presets().size())) return;
    preset = i; jkit::applyPreset (*this, apvts, presets()[(size_t) i], { "out" });
}
void Processor::getStateInformation (juce::MemoryBlock& d)
{
    auto s = apvts.copyState(); s.setProperty ("preset", preset, nullptr);
    if (auto x = s.createXml()) copyXmlToBinary (*x, d);
}
void Processor::setStateInformation (const void* data, int size)
{
    if (auto x = getXmlFromBinary (data, size)) { auto t = juce::ValueTree::fromXml (*x); if (t.isValid()) { preset = t.getProperty ("preset", 0); apvts.replaceState (t); } }
}
juce::AudioProcessorEditor* Processor::createEditor() { return new Editor (*this); }

using jkit::PV;
static PV P (int mode, float drive, float ceil = -0.3f, int os = 2) { return { { "mode", (float) mode }, { "drive", drive }, { "ceil", ceil }, { "os", (float) os } }; }
const std::vector<jkit::Preset>& presets()
{
    static const std::vector<jkit::Preset> p = {
        { "Init", "HARD a 0 dB: finche non spingi il drive non cambia niente.", {} },
        { "Master | Trasparente +2", "Tight in 8x: due dB di volume senza sentirlo.", P (TIGHT, 2, -0.3f, 3) },
        { "Master | Loud +4", "Bus gentile poi tetto: piu forte, ancora pulito.", jkit::cat ({ P (BUS, 4, -0.3f, 3), PV { { "punch", 0.15f } } }) },
        { "Master | Loud aggressivo", "Loud con split: i bassi restano interi, gli alti spingono.", jkit::cat ({ P (LOUD, 6, -0.3f, 3), PV { { "split", 1 }, { "splitHz", 110 } } }) },
        { "Master | Club", "Hard con split a 90 Hz e punch: cassa davanti.", jkit::cat ({ P (HARD, 5, -0.2f, 3), PV { { "split", 1 }, { "splitHz", 90 }, { "punch", 0.3f } } }) },
        { "Drum | Bus che bussa", "Hard e punch: la batteria esce dal mix.", jkit::cat ({ P (HARD, 6, -1), PV { { "punch", 0.4f } } }) },
        { "Drum | Kick e rullante", "Loud in 4x: corpo e schiocco, picchi piatti.", P (LOUD, 7, -1) },
        { "Drum | Crisp sugli hat", "Crisp: taglia il basso prima, gli alti restano nitidi.", P (CRISP, 6, -1) },
        { "Drum | Rullante crack", "Hard +9 con punch negativo: schiaccia lo schiocco e lo allunga.", jkit::cat ({ P (HARD, 9, -2), PV { { "punch", -0.35f } } }) },
        { "Drum | Parallelo", "Clip forte a meta: pugno senza perdere i transienti veri.", jkit::cat ({ P (LOUD, 12, -1), PV { { "mix", 0.45f } } }) },
        { "808 | Tight pulito", "Tight: il sub resta intero, la cima si arrotonda.", P (TIGHT, 5, -1) },
        { "808 | Che si sente sul telefono", "Asimmetrico: seconda armonica, l'808 si sente anche senza sub.", P (ASYM, 9, -1.5f) },
        { "808 | Distorto", "Soft spinto: ringhia ma resta tondo.", P (SOFT, 14, -2) },
        { "Basso | Pieno", "Tight leggero con split: tiene la fondamentale.", jkit::cat ({ P (TIGHT, 4, -1), PV { { "split", 1 }, { "splitHz", 80 } } }) },
        { "Voce | Morbido", "Soft leggero: prende i picchi della voce senza sentirlo.", P (SOFT, 3, -1) },
        { "Voce | Valvola", "Asimmetrico al 60%: calore pari sulla voce.", jkit::cat ({ P (ASYM, 6, -1), PV { { "mix", 0.6f } } }) },
        { "Synth | Fold", "Foldback: ripiega l'onda, armoniche metalliche.", jkit::cat ({ P (FOLD, 10, -2), PV { { "mix", 0.7f } } }) },
        { "Synth | Sine ricco", "Sine: il massimo delle armoniche, da sound design.", jkit::cat ({ P (SINE, 8, -2), PV { { "mix", 0.8f } } }) },
        { "Creativo | Distruzione", "Foldback a +20: rumore controllato.", P (FOLD, 20, -3, 3) },
        { "Creativo | Ascolta cosa toglie", "DELTA acceso: senti solo la parte tagliata.", jkit::cat ({ P (HARD, 8, -1), PV { { "delta", 1 } } }) },
        { "Mix bus | Colla", "Bus +3 con volume compensato: incolla senza alzare.", jkit::cat ({ P (BUS, 3, -0.5f), PV { { "link", 1 } } }) } };
    return p;
}
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new jclp::Processor(); }
