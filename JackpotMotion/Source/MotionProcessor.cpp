#include "MotionProcessor.h"
#include "MotionEditor.h"

namespace jmo
{
juce::String pid (int fx, const char* p) { return juce::String (fxInfo (fx).id) + "_" + p; }

juce::AudioProcessorValueTreeState::ParameterLayout Processor::layout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    const StringArray shapes { "SINE", "TRIANGOLO", "QUADRA", "CASUALE", "S&H" };
    auto F = [&l] (const String& id, const String& n, NormalisableRange<float> r, float d) { l.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, n, r, d)); };
    auto skew = [] (float a, float b, float c) { NormalisableRange<float> r (a, b); r.setSkewForCentre (c); return r; };
    // ogni effetto ha i suoi parametri: cambiare una manopola tocca solo l'effetto selezionato
    for (int f = 0; f < kNumFx; ++f)
    {
        const auto& I = fxInfo (f); const String nm (I.name);
        l.add (std::make_unique<AudioParameterBool> (ParameterID { pid (f, "on"), 1 }, nm + " acceso", false));
        F (pid (f, "rate"), nm + " rate", skew (0.02f, 20.0f, 1.0f), I.dRate > 0 ? I.dRate : 0.5f);
        l.add (std::make_unique<AudioParameterBool> (ParameterID { pid (f, "sync"), 1 }, nm + " sync", false));
        l.add (std::make_unique<AudioParameterChoice> (ParameterID { pid (f, "div"), 1 }, nm + " divisione", divNames(), 6));
        F (pid (f, "depth"), nm + " depth", { 0, 1 }, I.dDepth);
        F (pid (f, "char"), nm + " " + String (I.charName[0] ? I.charName : "carattere").toLowerCase(), { 0, 1 }, I.dChar);
        F (pid (f, "fb"), nm + " feedback", { -0.95f, 0.95f }, I.dFb);
        F (pid (f, "freq"), nm + " frequenza", f == FSHIFT ? skew (0.1f, 2000.0f, 40.0f) : f == RINGMOD ? skew (1.0f, 4000.0f, 200.0f) : f == ROTARY ? skew (200.0f, 3000.0f, 800.0f) : skew (40.0f, 16000.0f, 900.0f), I.dFreq);
        F (pid (f, "stereo"), nm + " stereo", { 0, 180 }, f == TREMOLO ? 0.0f : 90.0f);
        l.add (std::make_unique<AudioParameterChoice> (ParameterID { pid (f, "shape"), 1 }, nm + " forma", shapes, SINE));
        F (pid (f, "mix"), nm + " mix", { 0, 1 }, I.dMix);
        if (isVoiceFx (f)) F (pid (f, "dry"), nm + " voce principale", { -30, 6 }, 0);
    }
    // armonizzatore
    StringArray keys; for (auto& n : noteNames()) keys.add (n);
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "omni_key", 1 }, "Omnivox tonalita", keys, 0));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "omni_scale", 1 }, "Omnivox scala", scaleNames(), 1));
    const int dDeg[4] = { 2, 4, 0, 0 }; const float dLvl[4] = { 0, -4, -6, -6 };
    for (int v = 0; v < 4; ++v)
    {
        l.add (std::make_unique<AudioParameterInt> (ParameterID { "omni_v" + String (v + 1), 1 }, "Omnivox voce " + String (v + 1), -7, 7, dDeg[v]));
        F ("omni_l" + String (v + 1), "Omnivox volume " + String (v + 1), { -30, 6 }, dLvl[v]);
    }
    F ("tone", "Tono", skew (1000, 20000, 6000), 20000);
    F ("width", "Larghezza", { 0, 2 }, 1.0f);
    F ("bassmono", "Bassi mono", skew (20, 500, 120), 20);
    F ("out", "Output", { -12, 12 }, 0);
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
    static const char* names[12] = { "on", "rate", "sync", "div", "depth", "char", "fb", "freq", "stereo", "shape", "mix", "dry" };
    for (int f = 0; f < kNumFx; ++f) for (int k = 0; k < 12; ++k) ptr[f][k] = apvts.getRawParameterValue (pid (f, names[k]));
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
    for (int f = 0; f < kNumFx; ++f)
    {
        auto& X = S.fx[f];
        auto g = [&] (int k) { return ptr[f][k]->load(); };
        X.on = g (0) > 0.5f; X.rateHz = g (1); X.sync = g (2) > 0.5f; X.div = (int) g (3); X.depth = g (4); X.character = g (5);
        X.fb = g (6); X.freq = g (7); X.stereo = g (8); X.shape = (int) g (9); X.mix = g (10); X.dryDb = ptr[f][11] != nullptr ? g (11) : 0.0f;
    }
    auto& O = S.fx[OMNIVOX]; O.key = (int) value ("omni_key"); O.scale = (int) value ("omni_scale");
    for (int v = 0; v < 4; ++v) { O.deg[v] = (int) value (("omni_v" + juce::String (v + 1)).toRawUTF8()); O.lvlDb[v] = value (("omni_l" + juce::String (v + 1)).toRawUTF8()); }
    S.tone = value ("tone"); S.width = value ("width"); S.bassMono = value ("bassmono"); S.outDb = value ("out");
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
    for (auto& [id, v] : presets()[(size_t) i].v)
    {
        if (auto* rp = apvts.getParameter (id)) rp->setValueNotifyingHost (rp->convertTo0to1 (v));
        else jassertfalse;
    }
}
void Processor::getStateInformation (juce::MemoryBlock& d)
{
    auto s = apvts.copyState(); s.setProperty ("preset", preset, nullptr); s.setProperty ("sel", uiSel, nullptr);
    if (auto x = s.createXml()) copyXmlToBinary (*x, d);
}
void Processor::setStateInformation (const void* data, int size)
{
    if (auto x = getXmlFromBinary (data, size)) { auto t = juce::ValueTree::fromXml (*x); if (t.isValid()) { preset = t.getProperty ("preset", 0); uiSel = juce::jlimit (0, kNumFx - 1, (int) t.getProperty ("sel", (int) CHORUS)); apvts.replaceState (t); } }
}
juce::AudioProcessorEditor* Processor::createEditor() { return new Editor (*this); }

// ======================================================================= preset
const std::vector<Preset>& presets()
{
    static const std::vector<Preset> p = {
        { "Init", "Tutto spento: accendi uno o piu effetti.", {} },
        { "Voce | Doubler rap", "Raddoppio stretto per strofe rap: piu presenza senza perdere il centro.", { { "dubl_on", 1 }, { "dubl_char", 0.25f }, { "dubl_depth", 0.35f }, { "dubl_mix", 0.42f }, { "tone", 12000 }, { "bassmono", 150 } } },
        { "Voce | Doubler cantato largo", "Seconda take finta, larga, per ritornelli.", { { "dubl_on", 1 }, { "dubl_char", 0.5f }, { "dubl_depth", 0.55f }, { "dubl_rate", 0.6f }, { "dubl_mix", 0.5f }, { "width", 1.3f }, { "bassmono", 150 } } },
        { "Voce | Trivox ritornello", "Centro + due voci spostate di 9 centesimi: voce spessa e larga.", { { "triv_on", 1 }, { "triv_depth", 0.22f }, { "triv_char", 0.4f }, { "triv_mix", 0.45f }, { "bassmono", 180 } } },
        { "Voce | Ritornello pieno", "Trivox + doubler insieme: il ritornello che riempie lo stereo.", { { "triv_on", 1 }, { "triv_depth", 0.18f }, { "triv_mix", 0.35f }, { "dubl_on", 1 }, { "dubl_char", 0.4f }, { "dubl_mix", 0.35f }, { "bassmono", 180 } } },
        { "Voce | Armonia terza e quinta", "Omnivox in La minore: terza e quinta sopra, un filo larghe.", { { "omni_on", 1 }, { "omni_key", 9 }, { "omni_scale", 1 }, { "omni_v1", 2 }, { "omni_v2", 4 }, { "omni_l1", -3 }, { "omni_l2", -6 }, { "omni_mix", 0.7f } } },
        { "Voce | Ottave", "Ottava sotto e sopra: voce enorme da hook.", { { "omni_on", 1 }, { "omni_v1", 7 }, { "omni_v2", -7 }, { "omni_l1", -8 }, { "omni_l2", -4 }, { "omni_mix", 0.8f } } },
        { "Voce | Coro ensemble", "Da una voce a un coro: sei voci aperte su tutto lo stereo.", { { "ensm_on", 1 }, { "ensm_depth", 0.6f }, { "ensm_char", 0.6f }, { "ensm_mix", 0.55f }, { "tone", 9000 }, { "width", 1.4f }, { "bassmono", 200 } } },
        { "Voce | Robot", "Ring mod + vibrato: la voce del robot.", { { "ring_on", 1 }, { "ring_freq", 110 }, { "ring_mix", 0.7f }, { "vibr_on", 1 }, { "vibr_depth", 0.15f }, { "vibr_rate", 7 } } },
        { "Voce | Vibrato lo-fi", "Intonazione che ondeggia come un nastro stanco.", { { "vibr_on", 1 }, { "vibr_depth", 0.35f }, { "vibr_rate", 4.5f }, { "vibr_shape", RANDOM }, { "tone", 7000 } } },
        { "Chitarra | Chorus anni 80", "Chorus classico a due voci, largo e lucido.", { { "chor_on", 1 }, { "chor_char", 0.3f }, { "chor_depth", 0.55f }, { "chor_rate", 0.8f }, { "chor_stereo", 180 }, { "chor_mix", 0.5f } } },
        { "Chitarra | Phaser 4 stadi lento", "Il phaser da pedale: 4 stadi, giro lento.", { { "phas_on", 1 }, { "phas_char", 0.25f }, { "phas_depth", 0.7f }, { "phas_rate", 0.35f }, { "phas_fb", 0.2f } } },
        { "Chitarra | Tremolo surf", "Tremolo a tempo in 1/8, quasi squadrato.", { { "trem_on", 1 }, { "trem_sync", 1 }, { "trem_div", 9 }, { "trem_depth", 0.8f }, { "trem_char", 0.6f } } },
        { "Chitarra | Wah funk", "Autofiltro passa banda a tempo in 1/8: wah automatico.", { { "afil_on", 1 }, { "afil_sync", 1 }, { "afil_div", 9 }, { "afil_char", 0.5f }, { "afil_depth", 0.55f }, { "afil_freq", 800 }, { "afil_fb", 0.6f } } },
        { "Chitarra | Chorus + tremolo", "Chorus largo e tremolo che pulsa insieme.", { { "chor_on", 1 }, { "chor_mix", 0.45f }, { "trem_on", 1 }, { "trem_sync", 1 }, { "trem_div", 6 }, { "trem_depth", 0.5f } } },
        { "Keys | Rhodes phaser", "Rhodes con phaser cantato e un po' di risonanza.", { { "phas_on", 1 }, { "phas_char", 0.4f }, { "phas_depth", 0.6f }, { "phas_rate", 0.5f }, { "phas_fb", 0.45f } } },
        { "Keys | Rhodes autopan", "Autopan in 1/4 puntato come i vecchi Suitcase, con un filo di chorus.", { { "pan", 1 }, { "pansync", 1 }, { "pandiv", 5 }, { "pandepth", 0.8f }, { "chor_on", 1 }, { "chor_mix", 0.2f }, { "chor_depth", 0.3f } } },
        { "Keys | Organo leslie lento", "Rotary in chorale: l'organo che gira piano.", { { "rota_on", 1 }, { "rota_char", 0.0f }, { "rota_depth", 0.7f } } },
        { "Keys | Organo leslie veloce", "Rotary in tremolo: per i passaggi che salgono.", { { "rota_on", 1 }, { "rota_char", 1.0f }, { "rota_depth", 0.8f } } },
        { "Pad | Dimension largo", "Allarga il pad senza giri evidenti (modo 4).", { { "dimn_on", 1 }, { "dimn_char", 1.0f }, { "dimn_depth", 0.6f } } },
        { "Pad | Ensemble archi", "Ensemble a sei voci per archi e pad sintetici.", { { "ensm_on", 1 }, { "ensm_char", 0.8f }, { "ensm_depth", 0.75f }, { "ensm_rate", 0.25f }, { "ensm_mix", 0.6f }, { "width", 1.5f } } },
        { "Pad | Barber pole", "Frequency shifter lento con feedback: sale all'infinito.", { { "fshf_on", 1 }, { "fshf_freq", 0.6f }, { "fshf_fb", 0.6f }, { "fshf_mix", 0.6f } } },
        { "Batteria | Flanger jet", "Flanger classico sul loop: il jet in 1 battuta.", { { "flan_on", 1 }, { "flan_sync", 1 }, { "flan_div", 2 }, { "flan_depth", 0.8f }, { "flan_freq", 1500 }, { "flan_fb", 0.6f } } },
        { "Batteria | Hat phaser 1/16", "Phaser veloce a tempo sugli hi-hat.", { { "phas_on", 1 }, { "phas_sync", 1 }, { "phas_div", 3 }, { "phas_char", 0.6f }, { "phas_depth", 0.8f }, { "phas_fb", 0.3f }, { "phas_freq", 2500 }, { "bassmono", 200 } } },
        { "Batteria | Filtro che respira", "Passa basso che si apre in 2 battute.", { { "afil_on", 1 }, { "afil_sync", 1 }, { "afil_div", 1 }, { "afil_char", 0.0f }, { "afil_depth", 0.8f }, { "afil_freq", 1200 }, { "afil_fb", 0.3f }, { "afil_shape", TRI } } },
        { "Batteria | Autopan hat 1/8", "Hi-hat che rimbalzano a tempo.", { { "pan", 1 }, { "pansync", 1 }, { "pandiv", 9 }, { "pandepth", 0.6f }, { "panshape", TRI } } },
        { "Synth | Flanger negativo", "Flanger cavo con feedback negativo.", { { "flan_on", 1 }, { "flan_depth", 0.6f }, { "flan_rate", 0.15f }, { "flan_freq", 600 }, { "flan_fb", -0.7f } } },
        { "Synth | Chorus 4 voci", "Chorus denso per lead e pad, quattro voci per lato.", { { "chor_on", 1 }, { "chor_char", 1.0f }, { "chor_depth", 0.6f }, { "chor_rate", 0.45f }, { "chor_mix", 0.55f }, { "width", 1.2f } } },
        { "808 | Chorus bassi mono", "Chorus sull'808 lasciando i bassi al centro.", { { "chor_on", 1 }, { "chor_char", 0.3f }, { "chor_depth", 0.4f }, { "chor_rate", 0.6f }, { "chor_mix", 0.5f }, { "bassmono", 220 } } },
        { "Mix | Dimension sottile", "Un filo di larghezza sul bus (modo 1).", { { "dimn_on", 1 }, { "dimn_char", 0.0f }, { "dimn_depth", 0.4f }, { "dimn_mix", 0.6f }, { "bassmono", 150 } } },
        { "Effetti | Metallo", "Frequency shift + ring mod: suono metallico da transizione.", { { "fshf_on", 1 }, { "fshf_freq", 180 }, { "fshf_mix", 0.5f }, { "ring_on", 1 }, { "ring_freq", 700 }, { "ring_depth", 0.3f }, { "ring_mix", 0.4f } } },
        { "Effetti | Vibrato casuale", "Pitch che sbanda a caso, nastro rovinato.", { { "vibr_on", 1 }, { "vibr_depth", 0.6f }, { "vibr_rate", 2.5f }, { "vibr_shape", SH }, { "tone", 5000 } } },
        { "Effetti | Autopan S&H", "Il suono salta da un lato all'altro a ogni 1/8.", { { "pan", 1 }, { "pansync", 1 }, { "pandiv", 9 }, { "pandepth", 1.0f }, { "panshape", SH } } },
        { "Effetti | Phaser 12 stadi", "Phaser estremo da transizione.", { { "phas_on", 1 }, { "phas_char", 1.0f }, { "phas_depth", 1.0f }, { "phas_rate", 0.2f }, { "phas_fb", 0.7f } } } };
    return p;
}
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new jmo::Processor(); }
