#include "TubeProcessor.h"
#include "TubeEditor.h"

namespace jtbx
{
juce::AudioProcessorValueTreeState::ParameterLayout Processor::layout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    auto F = [&l] (const String& id, const String& n, NormalisableRange<float> r, float d) { l.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, n, r, d)); };
    auto B = [&l] (const char* id, const char* n, bool d) { l.add (std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, n, d)); };
    auto Cc = [&l] (const char* id, const char* n, StringArray s, int d) { l.add (std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, n, s, d)); };
    // valvole: si accende con POWER (spento al caricamento = suono invariato)
    B ("hg_on", "Power valvole", false);
    F ("in", "Input", { -12, 12, 0.01f }, 0);
    F ("pentode", "Pentode", { 0, 10, 0.01f }, 5); F ("triode", "Triode", { 0, 10, 0.01f }, 5); F ("density", "Density", { 0, 10, 0.01f }, 0);
    B ("sat_in", "Saturation in", false); F ("sat", "Saturation", { 0, 10, 0.01f }, 3); Cc ("satFreq", "Sat freq", { "LOW", "FLAT", "HIGH" }, 1); B ("alt", "Alt tube", false);
    B ("air_in", "Air in", false); F ("air", "Air", { 0, 10, 0.01f }, 4); Cc ("calib", "Calibration", { "DARK", "NORMAL", "BRIGHT" }, 1);
    F ("mix", "Mix", { 0, 1 }, 1); B ("ms", "Mid/Side", false);
    // enhancer multibanda
    B ("mb_on", "Enhancer on", false);
    const float xo[4] = { 120, 600, 2500, 8000 }; const float lo[4] = { 40, 200, 800, 3000 }, hi[4] = { 300, 2000, 7000, 16000 };
    for (int k = 0; k < 4; ++k) { NormalisableRange<float> r (lo[k], hi[k]); r.setSkewForCentre (xo[k]); F ("x" + String (k + 1), "Taglio " + String (k + 1), r, xo[k]); }
    for (int k = 1; k <= 5; ++k) { F ("h" + String (k), "Armoniche " + String (k), { 0, 1 }, 0); F ("l" + String (k), "Livello " + String (k), { -12, 12, 0.01f }, 0); F ("w" + String (k), "Larghezza " + String (k), { 0, 2 }, 1); }
    Cc ("char", "Carattere", { "TUBO", "NASTRO", "TRANSISTOR" }, 0); F ("mb_mix", "Enhancer mix", { 0, 1 }, 1);
    F ("out", "Output", { -12, 12, 0.01f }, 0);
    return l;
}
Processor::Processor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true).withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", layout()) {}
void Processor::prepareToPlay (double s, int) { eng.prepare (s, kChunk); setLatencySamples (eng.latency()); }
Settings Processor::settings() const
{
    Settings S; auto b = [this] (const char* id) { return value (id) > 0.5f; };
    S.hgOn = b ("hg_on"); S.inDb = value ("in"); S.pentode = value ("pentode"); S.triode = value ("triode"); S.density = value ("density");
    S.satIn = b ("sat_in"); S.sat = value ("sat"); S.satFreq = (int) value ("satFreq"); S.alt = b ("alt");
    S.airIn = b ("air_in"); S.air = value ("air"); S.calib = (int) value ("calib"); S.mix = value ("mix"); S.ms = b ("ms");
    S.mbOn = b ("mb_on"); for (int k = 0; k < 4; ++k) S.xo[k] = valueS ("x" + juce::String (k + 1));
    for (int k = 0; k < 5; ++k) { S.harm[k] = valueS ("h" + juce::String (k + 1)); S.lvl[k] = valueS ("l" + juce::String (k + 1)); S.width[k] = valueS ("w" + juce::String (k + 1)); }
    S.character = (int) value ("char"); S.mbMix = value ("mb_mix"); S.outDb = value ("out");
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

using jkit::PV; using jkit::cat;
// orologio -> valore 0..10 (ore 7 = 0, ore 12 = 5, ore 17 = 10)
static float clk (float h) { return juce::jlimit (0.0f, 10.0f, (h - 7.0f) * 1.0f); }
static PV hg (float pent, float tri, float sat, int freq, bool alt, bool air, int calib = 1, float dens = 0, float mix = 1)
{
    return { { "hg_on", 1 }, { "pentode", pent }, { "triode", tri }, { "sat_in", sat > 0 ? 1.0f : 0.0f }, { "sat", sat }, { "satFreq", (float) freq }, { "alt", alt ? 1.0f : 0.0f },
             { "air_in", air ? 1.0f : 0.0f }, { "air", 4 }, { "calib", (float) calib }, { "density", dens }, { "mix", mix } };
}
static PV mb (std::initializer_list<float> h, std::initializer_list<float> l, std::initializer_list<float> w, int ch = 0, float mix = 1)
{
    PV r { { "mb_on", 1 }, { "char", (float) ch }, { "mb_mix", mix } }; int k = 1;
    for (float v : h) r.push_back ({ k == 1 ? "h1" : k == 2 ? "h2" : k == 3 ? "h3" : k == 4 ? "h4" : "h5", v }), ++k; k = 1;
    for (float v : l) r.push_back ({ k == 1 ? "l1" : k == 2 ? "l2" : k == 3 ? "l3" : k == 4 ? "l4" : "l5", v }), ++k; k = 1;
    for (float v : w) r.push_back ({ k == 1 ? "w1" : k == 2 ? "w2" : k == 3 ? "w3" : k == 4 ? "w4" : "w5", v }), ++k;
    return r;
}
const std::vector<jkit::Preset>& presets()
{
    // impostazioni dal manuale e dagli esempi d'uso dell'HG-2 (posizioni a orologio convertite)
    static const std::vector<jkit::Preset> p = {
        { "Init", "Spento: accendi POWER per le valvole o l'enhancer.", {} },
        { "HG-2 | Neutro", "Pentode a ore 12, triode a ore 10-11, niente parallelo: solo il colore delle valvole.", cat ({ hg (clk (12), clk (10.5f), 0, 1, false, false), PV { { "out", 4.3f } } }) },
        { "Mix | Bus caldo", "Sat LOW a ore 9, pentode ore 13, air: il classico sul mix.", cat ({ hg (clk (13), clk (12), clk (9), 0, true, true), PV { { "out", -2.2f } } }) },
        { "Mix | Mastering brillante", "Sat HIGH a ore 11, pentode e triode a mezzogiorno, air.", cat ({ hg (clk (12), clk (12), clk (11), 2, true, true, 1, 0, 0.8f), PV { { "out", 0.9f } } }) },
        { "Mix | Denso", "Density alta: le valvole lavorano senza alzare il volume.", cat ({ hg (clk (12), clk (11), clk (10), 1, false, true, 1, 5), PV { { "out", 2.2f } } }) },
        { "Batteria | Lenta e grossa", "Sat LOW a ore 13, pentode ore 10, alt tube.", cat ({ hg (clk (10), clk (12), clk (13), 0, true, false), PV { { "out", 2.3f } } }) },
        { "Batteria | Pugno", "Pentode ore 14, sat FLAT ore 11, triode basso.", cat ({ hg (clk (14), clk (10), clk (11), 1, false, false, 1, 2), PV { { "out", -0.7f } } }) },
        { "Basso | Spesso", "Sat FLAT, pentode ore 13, alt tube, air: il basso si sente ovunque.", cat ({ hg (clk (13), clk (13), clk (11), 1, true, true), PV { { "out", -5.4f } } }) },
        { "Basso | Ringhio", "Sat HIGH a ore 15, pentode al massimo.", cat ({ hg (clk (17), clk (14), clk (15), 2, true, true), PV { { "out", -12.0f } } }) },
        { "Voce | Calda", "Sat LOW a ore 17, pentode ore 13, air.", cat ({ hg (clk (13), clk (12), clk (17), 0, true, true), PV { { "out", -6.7f } } }) },
        { "Voce | Presenza", "Sat FLAT ore 14, triode ore 10, niente air.", cat ({ hg (clk (14), clk (10), clk (14), 1, true, false), PV { { "out", -2.1f } } }) },
        { "Chitarra | Crunch", "Sat LOW ore 10, pentode ore 13, triode ore 14.", cat ({ hg (clk (13), clk (14), clk (10), 0, true, true), PV { { "out", -7.1f } } }) },
        { "Parallelo | Meta valvole", "Pentode e triode spinti al 50% di mix.", cat ({ hg (clk (16), clk (12), clk (13), 1, true, true, 1, 3, 0.5f), PV { { "out", -4.5f } } }) },
        { "Enhancer | Mix che brilla", "Armoniche leggere su medi alti e aria, un po' di larghezza in alto.", mb ({ 0, 0.1f, 0.25f, 0.35f, 0.3f }, { 0, 0, 0.5f, 1, 1.5f }, { 1, 1, 1.1f, 1.25f, 1.35f }) },
        { "Enhancer | Basso che si sente", "Armoniche pari sul sub e sui bassi: il basso esce anche dal telefono.", cat ({ mb ({ 0.6f, 0.4f, 0, 0, 0 }, { 0, 1, 0, 0, 0 }, { 0.6f, 0.8f, 1, 1, 1 }), PV { { "out", -1.0f } } }) },
        { "Enhancer | Voce davanti", "Armoniche sui medi e presenza, medi bassi stretti.", cat ({ mb ({ 0, 0, 0.35f, 0.4f, 0.15f }, { -1, -1, 1, 1.5f, 1 }, { 0.8f, 0.8f, 1, 1.1f, 1.2f }), PV { { "out", 1.0f } } }) },
        { "Enhancer | Batteria viva", "Nastro su bassi e medi, transistor sugli alti.", mb ({ 0.35f, 0.2f, 0.25f, 0.4f, 0.3f }, { 1, 0, 0, 1, 1 }, { 1, 1, 1, 1.2f, 1.3f }, 1) },
        { "Enhancer | Largo", "Solo larghezza: bassi mono, alti larghi.", mb ({ 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0 }, { 0, 0.7f, 1.2f, 1.5f, 1.6f }) },
        { "Catena | Valvole + Enhancer", "HG-2 sul bus e harmonic enhancer leggero dopo.", cat ({ cat ({ hg (clk (13), clk (12), clk (10), 0, true, true), mb ({ 0.2f, 0.1f, 0.15f, 0.25f, 0.2f }, { 0, 0, 0, 0.5f, 1 }, { 0.8f, 1, 1, 1.15f, 1.25f }) }), PV { { "out", -2.7f } } }) } };
    return p;
}
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new jtbx::Processor(); }
