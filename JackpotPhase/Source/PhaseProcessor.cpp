#include "PhaseProcessor.h"
#include "PhaseEditor.h"

namespace jphs
{
juce::AudioProcessorValueTreeState::ParameterLayout Processor::layout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    auto F = [&l] (const char* id, const char* n, NormalisableRange<float> r, float d) { l.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, n, r, d)); };
    NormalisableRange<float> fr (40, 400); fr.setSkewForCentre (140);
    NormalisableRange<float> sp (1, 300); sp.setSkewForCentre (40);
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "mode", 1 }, "Modo", StringArray { "SPENTO", "INVERTI", "RUOTA", "ADATTIVA" }, M_OFF));
    F ("freq", "Frequenza", fr, 160); F ("angle", "Angolo", { 0, 360, 1 }, 0); F ("speed", "Velocita", sp, 40); F ("amount", "Quanto", { 0, 1 }, 1);
    F ("out", "Output", { -12, 12, 0.01f }, 0);
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "listen", 1 }, "Ascolta con la cassa", false));
    return l;
}
Processor::Processor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true).withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                                       .withInput ("Sidechain", juce::AudioChannelSet::stereo(), false)),
      apvts (*this, nullptr, "PARAMS", layout()) {}
void Processor::prepareToPlay (double s, int) { eng.prepare (s); setLatencySamples (eng.latency()); }
Settings Processor::settings() const
{
    Settings S; S.mode = (int) value ("mode"); S.freq = value ("freq"); S.angle = value ("angle"); S.speedMs = value ("speed"); S.amount = value ("amount");
    S.outDb = value ("out"); S.listen = value ("listen") > 0.5f; return S;
}
void Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals nd;
    const int n = buffer.getNumSamples(); if (n == 0) return;
    auto mainB = getBusBuffer (buffer, true, 0);
    if (mainB.getNumChannels() == 1 && buffer.getNumChannels() > 1) buffer.copyFrom (1, 0, buffer, 0, 0, n);
    const float* sL = nullptr; const float* sR = nullptr;
    if (getBusCount (true) > 1 && getBus (true, 1)->isEnabled())
    {
        auto sc = getBusBuffer (buffer, true, 1);
        if (sc.getNumChannels() > 0) { sL = sc.getReadPointer (0); sR = sc.getReadPointer (juce::jmin (1, sc.getNumChannels() - 1)); }
    }
    eng.process (buffer.getWritePointer (0), buffer.getWritePointer (1), sL, sR, n, settings(), snap);
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
const std::vector<jkit::Preset>& presets()
{
    static const std::vector<jkit::Preset> p = {
        { "Init", "Spento: il suono passa uguale (solo ritardato dei 30 ms di anticipo, compensati dal DAW).", {} },
        { "808 | Adattiva sulla cassa", "Allinea l'808 a ogni colpo di cassa sotto i 160 Hz.", { { "mode", M_ADAPT }, { "freq", 160 }, { "speed", 30 } } },
        { "808 | Adattiva lenta", "Si muove piano: corregge senza far ballare il sub.", { { "mode", M_ADAPT }, { "freq", 140 }, { "speed", 150 } } },
        { "808 | Solo il sub", "Lavora solo sotto gli 80 Hz.", { { "mode", M_ADAPT }, { "freq", 80 }, { "speed", 40 } } },
        { "Basso | Adattiva morbida", "Meta della correzione: piu naturale sul basso suonato.", { { "mode", M_ADAPT }, { "freq", 200 }, { "speed", 60 }, { "amount", 0.6f } } },
        { "Basso | Inverti", "Polarita invertita fissa: quando cassa e basso si cancellano sempre.", { { "mode", M_INV } } },
        { "Fisso | Ruota 90", "Rotazione fissa di 90 gradi sotto i 160 Hz.", { { "mode", M_ROT }, { "angle", 90 }, { "freq", 160 } } },
        { "Fisso | Ruota 270", "Rotazione fissa di 270 gradi (meno 90).", { { "mode", M_ROT }, { "angle", 270 }, { "freq", 160 } } },
        { "Controllo | Ascolta con la cassa", "Adattiva e somma la cassa in uscita per sentire l'incastro.", { { "mode", M_ADAPT }, { "listen", 1 } } } };
    return p;
}
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new jphs::Processor(); }
