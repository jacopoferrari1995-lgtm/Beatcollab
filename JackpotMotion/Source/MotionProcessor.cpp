#include "MotionProcessor.h"
#include "MotionEditor.h"

namespace jmo
{
juce::AudioProcessorValueTreeState::ParameterLayout Processor::layout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    StringArray fx; for (int i = 0; i < kNumFx; ++i) fx.add (fxInfo (i).name);
    const StringArray shapes { "SINE", "TRIANGOLO", "QUADRA", "CASUALE", "S&H" };
    auto F = [&l] (const char* id, const char* n, NormalisableRange<float> r, float d) { l.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, n, r, d)); };
    auto skew = [] (float a, float b, float c) { NormalisableRange<float> r (a, b); r.setSkewForCentre (c); return r; };
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "fx", 1 }, "Effetto", fx, CHORUS));
    F ("rate", "Rate", skew (0.02f, 20.0f, 1.0f), 0.6f);
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "sync", 1 }, "Sync", false));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "div", 1 }, "Divisione", divNames(), 6));
    F ("depth", "Depth", { 0, 1 }, 0.5f);
    F ("fb", "Feedback", { -0.95f, 0.95f }, 0.0f);
    F ("char", "Carattere", { 0, 1 }, 0.5f);
    F ("stereo", "Stereo", { 0, 180 }, 90.0f);
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "shape", 1 }, "Forma LFO", shapes, SINE));
    // uscita: il plugin parte neutro (MIX a zero)
    F ("mix", "Mix", { 0, 1 }, 0.0f);
    F ("tone", "Tono", skew (1000, 20000, 6000), 20000);
    F ("width", "Larghezza", { 0, 2 }, 1.0f);
    F ("bassmono", "Bassi mono", skew (20, 500, 120), 20);
    F ("out", "Output", { -12, 12 }, 0);
    // autopan
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "pan", 1 }, "Autopan", false));
    F ("panrate", "Pan rate", skew (0.02f, 20.0f, 1.0f), 1.0f);
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "pansync", 1 }, "Pan sync", false));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "pandiv", 1 }, "Pan divisione", divNames(), 6));
    F ("pandepth", "Pan depth", { 0, 1 }, 0.7f);
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "panshape", 1 }, "Pan forma", shapes, SINE));
    return l;
}

Processor::Processor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true).withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", layout())
{
}

bool Processor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto o = l.getMainOutputChannelSet(), i = l.getMainInputChannelSet();
    return o == juce::AudioChannelSet::stereo() && (i == o || i == juce::AudioChannelSet::mono());
}

void Processor::prepareToPlay (double s, int block)
{
    sr = s; eng.prepare (s, block); mono.assign ((size_t) juce::jmax (block, 4096), 0.0f);
    setLatencySamples (0);
}

Settings Processor::settings() const
{
    Settings S;
    S.fx = (int) value ("fx"); S.rateHz = value ("rate"); S.sync = value ("sync") > 0.5f; S.div = (int) value ("div");
    S.depth = value ("depth"); S.fb = value ("fb"); S.character = value ("char"); S.stereo = value ("stereo"); S.shape = (int) value ("shape");
    S.mix = value ("mix"); S.tone = value ("tone"); S.width = value ("width"); S.bassMono = value ("bassmono"); S.outDb = value ("out");
    S.panOn = value ("pan") > 0.5f; S.panHz = value ("panrate"); S.panSync = value ("pansync") > 0.5f; S.panDiv = (int) value ("pandiv");
    S.panDepth = value ("pandepth"); S.panShape = (int) value ("panshape");
    return S;
}

void Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals nd;
    const int n = buffer.getNumSamples(); if (n == 0) return;
    if (getTotalNumInputChannels() == 1) buffer.copyFrom (1, 0, buffer, 0, 0, n);
    float* L = buffer.getWritePointer (0); float* R = buffer.getWritePointer (1);
    double bpm = 120, ppq = -1;
    if (auto* ph = getPlayHead()) if (auto pos = ph->getPosition())
    {
        if (auto b = pos->getBpm()) bpm = *b;
        if (pos->getIsPlaying()) if (auto q = pos->getPpqPosition()) ppq = *q;
    }
    uiBpm = bpm;
    float pk[2] = { 0, 0 };
    for (int i = 0; i < n; ++i) { pk[0] = juce::jmax (pk[0], std::abs (L[i])); pk[1] = juce::jmax (pk[1], std::abs (R[i])); }
    inPk[0] = juce::jmax (pk[0], inPk[0].load() * 0.9f); inPk[1] = juce::jmax (pk[1], inPk[1].load() * 0.9f);
    if ((int) mono.size() < n) mono.resize ((size_t) n);
    for (int i = 0; i < n; ++i) mono[(size_t) i] = 0.5f * (L[i] + R[i]);
    ringIn.push (mono.data(), n);
    eng.process (L, R, n, settings(), ppq, bpm);
    pk[0] = pk[1] = 0;
    for (int i = 0; i < n; ++i)
    {
        pk[0] = juce::jmax (pk[0], std::abs (L[i])); pk[1] = juce::jmax (pk[1], std::abs (R[i]));
        cxy = cxy * 0.9995f + L[i] * R[i]; cxx = cxx * 0.9995f + L[i] * L[i]; cyy = cyy * 0.9995f + R[i] * R[i];
    }
    outPk[0] = juce::jmax (pk[0], outPk[0].load() * 0.9f); outPk[1] = juce::jmax (pk[1], outPk[1].load() * 0.9f);
    corr = cxx + cyy > 1e-7f ? cxy / std::sqrt (cxx * cyy + 1e-12f) : 1.0f;
    ringOut.push (L, n); ringOutR.push (R, n);
}

void Processor::loadPreset (int i)
{
    if (! juce::isPositiveAndBelow (i, (int) presets().size())) return;
    preset = i;
    for (auto* p : getParameters()) if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p)) if (rp->paramID != "out") rp->setValueNotifyingHost (rp->getDefaultValue());
    for (auto& [id, v] : presets()[(size_t) i].v) if (auto* rp = apvts.getParameter (id)) rp->setValueNotifyingHost (rp->convertTo0to1 (v));
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

// ======================================================================= preset
const std::vector<Preset>& presets()
{
    static const std::vector<Preset> p = {
        { "Init", "Tutto neutro: scegli un effetto e alza MIX.", {} },
        { "Voce | Doubler rap", "Raddoppio stretto per strofe rap: piu presenza senza perdere il centro.", { { "fx", DOUBLER }, { "char", 0.25f }, { "depth", 0.35f }, { "rate", 0.8f }, { "mix", 0.42f }, { "tone", 12000 }, { "bassmono", 150 } } },
        { "Voce | Doubler cantato largo", "Seconda take finta, larga, per ritornelli.", { { "fx", DOUBLER }, { "char", 0.5f }, { "depth", 0.55f }, { "rate", 0.6f }, { "mix", 0.5f }, { "tone", 10000 }, { "width", 1.3f }, { "bassmono", 150 } } },
        { "Voce | Trivox ritornello", "Centro + due voci spostate di 9 centesimi: voce spessa e larga.", { { "fx", TRIVOX }, { "depth", 0.22f }, { "char", 0.4f }, { "mix", 0.45f }, { "tone", 12000 }, { "bassmono", 180 } } },
        { "Voce | Trivox lead sottile", "Allarga il lead senza farlo sembrare un effetto.", { { "fx", TRIVOX }, { "depth", 0.08f }, { "char", 0.25f }, { "mix", 0.3f }, { "bassmono", 200 } } },
        { "Voce | Omnivox coro", "Da una voce a un coro: sei voci aperte su tutto lo stereo.", { { "fx", OMNIVOX }, { "depth", 0.6f }, { "char", 0.6f }, { "rate", 0.4f }, { "mix", 0.55f }, { "tone", 9000 }, { "width", 1.4f }, { "bassmono", 200 } } },
        { "Voce | Vibrato lo-fi", "Intonazione che ondeggia come un nastro stanco.", { { "fx", VIBRATO }, { "depth", 0.35f }, { "rate", 4.5f }, { "shape", RANDOM }, { "mix", 1.0f }, { "tone", 7000 } } },
        { "Chitarra | Chorus anni 80", "Chorus classico a due voci, largo e lucido.", { { "fx", CHORUS }, { "char", 0.3f }, { "depth", 0.55f }, { "rate", 0.8f }, { "stereo", 180 }, { "mix", 0.5f } } },
        { "Chitarra | Phaser 4 stadi lento", "Il phaser da pedale: 4 stadi, giro lento.", { { "fx", PHASER }, { "char", 0.25f }, { "depth", 0.7f }, { "rate", 0.35f }, { "fb", 0.2f }, { "mix", 1.0f } } },
        { "Chitarra | Tremolo surf", "Tremolo a tempo in 1/8, quasi squadrato.", { { "fx", TREMOLO }, { "sync", 1 }, { "div", 9 }, { "depth", 0.8f }, { "char", 0.6f }, { "stereo", 0 }, { "mix", 1.0f } } },
        { "Chitarra | Tremolo stereo", "Volume che rimbalza tra destra e sinistra, in 1/4.", { { "fx", TREMOLO }, { "sync", 1 }, { "div", 6 }, { "depth", 0.9f }, { "stereo", 180 }, { "mix", 1.0f } } },
        { "Keys | Rhodes phaser", "Rhodes con phaser cantato e un po' di risonanza.", { { "fx", PHASER }, { "char", 0.4f }, { "depth", 0.6f }, { "rate", 0.5f }, { "fb", 0.45f }, { "mix", 1.0f } } },
        { "Keys | Rhodes autopan", "Autopan in 1/4 dotted come i vecchi Suitcase.", { { "pan", 1 }, { "pansync", 1 }, { "pandiv", 5 }, { "pandepth", 0.8f }, { "fx", CHORUS }, { "mix", 0.2f }, { "depth", 0.3f } } },
        { "Pad | Dimension largo", "Allarga il pad senza giri evidenti (modo 4).", { { "fx", DIMENSION }, { "char", 1.0f }, { "depth", 0.6f }, { "mix", 1.0f } } },
        { "Pad | Ensemble archi", "Ensemble a sei voci per archi e pad sintetici.", { { "fx", OMNIVOX }, { "char", 0.8f }, { "depth", 0.75f }, { "rate", 0.25f }, { "mix", 0.6f }, { "width", 1.5f } } },
        { "Batteria | Flanger jet", "Flanger classico sul loop: il jet in 1 battuta.", { { "fx", FLANGER }, { "sync", 1 }, { "div", 2 }, { "depth", 0.8f }, { "char", 0.15f }, { "fb", 0.6f }, { "mix", 1.0f } } },
        { "Batteria | Hat phaser 1/16", "Phaser veloce a tempo sugli hi-hat.", { { "fx", PHASER }, { "sync", 1 }, { "div", 3 }, { "char", 0.6f }, { "depth", 0.8f }, { "fb", 0.3f }, { "mix", 1.0f }, { "bassmono", 200 } } },
        { "Batteria | Autopan hat 1/8", "Hi-hat che rimbalzano a tempo.", { { "pan", 1 }, { "pansync", 1 }, { "pandiv", 9 }, { "pandepth", 0.6f }, { "panshape", TRI } } },
        { "Synth | Flanger negativo", "Flanger cavo con feedback negativo.", { { "fx", FLANGER }, { "depth", 0.6f }, { "rate", 0.15f }, { "char", 0.3f }, { "fb", -0.7f }, { "mix", 1.0f } } },
        { "Synth | Chorus 4 voci", "Chorus denso per lead e pad, quattro voci per lato.", { { "fx", CHORUS }, { "char", 1.0f }, { "depth", 0.6f }, { "rate", 0.45f }, { "mix", 0.55f }, { "width", 1.2f } } },
        { "808 | Chorus bassi mono", "Chorus sull'808 lasciando i bassi al centro.", { { "fx", CHORUS }, { "char", 0.3f }, { "depth", 0.4f }, { "rate", 0.6f }, { "mix", 0.5f }, { "bassmono", 220 } } },
        { "Mix | Dimension sottile", "Un filo di larghezza sul bus (modo 1).", { { "fx", DIMENSION }, { "char", 0.0f }, { "depth", 0.4f }, { "mix", 0.6f }, { "bassmono", 150 } } },
        { "Effetti | Vibrato casuale", "Pitch che sbanda a caso, nastro rovinato.", { { "fx", VIBRATO }, { "depth", 0.6f }, { "rate", 2.5f }, { "shape", SH }, { "mix", 1.0f }, { "tone", 5000 } } },
        { "Effetti | Autopan S&H", "Il suono salta da un lato all'altro a ogni 1/8.", { { "pan", 1 }, { "pansync", 1 }, { "pandiv", 9 }, { "pandepth", 1.0f }, { "panshape", SH } } },
        { "Effetti | Phaser 12 stadi", "Phaser estremo da transizione.", { { "fx", PHASER }, { "char", 1.0f }, { "depth", 1.0f }, { "rate", 0.2f }, { "fb", 0.7f }, { "mix", 1.0f } } } };
    return p;
}
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new jmo::Processor(); }
