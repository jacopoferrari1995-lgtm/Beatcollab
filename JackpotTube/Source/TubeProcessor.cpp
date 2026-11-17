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
    // enhancer: quattro comandi
    B ("mb_on", "Enhancer on", false);
    F ("e_amt", "Enhancer amount", { 0, 10, 0.01f }, 4);
    Cc ("char", "Carattere", { "TUBO", "NASTRO", "TRANSISTOR" }, 0);
    F ("e_focus", "Enhancer focus", { 0, 1 }, 0.5f);
    F ("e_width", "Enhancer width", { 0, 2 }, 1);
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
    S.satIn = b ("sat_in"); S.sat = value ("sat"); S.satFreq = juce::roundToInt (value ("satFreq")); S.alt = b ("alt");
    S.airIn = b ("air_in"); S.air = value ("air"); S.calib = juce::roundToInt (value ("calib")); S.mix = value ("mix"); S.ms = b ("ms");
    S.mbOn = b ("mb_on"); S.amount = value ("e_amt"); S.character = juce::roundToInt (value ("char")); S.focus = value ("e_focus"); S.width = value ("e_width"); S.outDb = value ("out");
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
    preset = i; userName = {}; jkit::applyPreset (*this, apvts, presets()[(size_t) i], { "out" });
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

using jkit::PV; using jkit::cat;
// orologio -> valore 0..10 (ore 7 = 0, ore 12 = 5, ore 17 = 10)
static float clk (float h) { return juce::jlimit (0.0f, 10.0f, (h - 7.0f) * 1.0f); }
static PV hg (float pent, float tri, float sat, int freq, bool alt, bool air, int calib = 1, float dens = 0, float mix = 1)
{
    return { { "hg_on", 1 }, { "pentode", pent }, { "triode", tri }, { "sat_in", sat > 0 ? 1.0f : 0.0f }, { "sat", sat }, { "satFreq", (float) freq }, { "alt", alt ? 1.0f : 0.0f },
             { "air_in", air ? 1.0f : 0.0f }, { "air", 4 }, { "calib", (float) calib }, { "density", dens }, { "mix", mix } };
}
// enhancer: quantita' 0..10, carattere 0 TUBO / 1 NASTRO / 2 TRANSISTOR, fuoco in Hz, larghezza 0..2
static float nFoc (float hz) { return juce::jlimit (0.0f, 1.0f, std::log (hz / 60.0f) / std::log (150.0f)); }
static PV en (float amt, int ch, float hz, float w = 1) { return { { "mb_on", 1 }, { "e_amt", amt }, { "char", (float) ch }, { "e_focus", nFoc (hz) }, { "e_width", w } }; }
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
        { "Basso | Ringhio", "Sat HIGH a ore 15, pentode quasi al massimo.", cat ({ hg (clk (16), clk (13), clk (14), 2, true, true), PV { { "out", -10.0f } } }) },
        { "Voce | Calda", "Sat LOW a ore 17, pentode ore 13, air.", cat ({ hg (clk (13), clk (12), clk (17), 0, true, true), PV { { "out", -6.7f } } }) },
        { "Voce | Presenza", "Sat FLAT ore 14, triode ore 10, niente air.", cat ({ hg (clk (14), clk (10), clk (14), 1, true, false), PV { { "out", -2.1f } } }) },
        { "Chitarra | Crunch", "Sat LOW ore 10, pentode ore 13, triode ore 14.", cat ({ hg (clk (13), clk (14), clk (10), 0, true, true), PV { { "out", -7.1f } } }) },
        { "Parallelo | Meta valvole", "Pentode e triode spinti al 50% di mix.", cat ({ hg (clk (16), clk (12), clk (13), 1, true, true, 1, 3, 0.5f), PV { { "out", -4.5f } } }) },
        { "Mix | Glue di valvola", "Pentode e triode a ore 12, density un filo: il mix si stringe e prende corpo.", cat ({ hg (clk (12), clk (12), 0, 1, false, false, 1, 2), PV { { "out", 1.5f } } }) },
        { "Master | Ultimo tocco", "Pochissimo: pentode ore 11, air, mix al 60%. Calore che non si sente ma si sente.", cat ({ hg (clk (11), clk (11), clk (8), 2, false, true, 1, 0, 0.6f), PV { { "out", 1.2f } } }) },
        { "808 | Si sente sul telefono", "Triode spinto e TUBO sulle armoniche dei bassi: l'808 esce dalle casse piccole.", cat ({ hg (clk (12), clk (14), clk (10), 0, true, false), en (5.5f, 0, 90, 0.6f), PV { { "out", -3.0f } } }) },
        { "808 | Ringhio drill", "Pentode al limite, ALT TUBE, TRANSISTOR basso: l'808 morde.", cat ({ hg (clk (15), clk (13), clk (13), 1, true, false, 1, 3), en (4, 2, 120, 0.5f), PV { { "out", -8.0f } } }) },
        { "Voce | Rap davanti", "Sat FLAT, triode basso, enhancer NASTRO sulla presenza: la voce esce dal beat.", cat ({ hg (clk (13), clk (10), clk (12), 1, false, true), en (4, 1, 2500, 1.1f), PV { { "out", -2.5f } } }) },
        { "Voce | Aria R&B", "Valvole leggere, TUBO sull'aria e largo: lucido senza fischiare.", cat ({ hg (clk (11), clk (11), clk (9), 2, false, true, 1, 0, 0.7f), en (3.5f, 0, 6000, 1.3f), PV { { "out", -0.5f } } }) },
        { "Voce | Telefono vintage", "Sat HIGH a ore 16, CALIBRATION dark, TRANSISTOR sui medi: voce da radio, per intro e ad-lib.", cat ({ hg (clk (14), clk (13), clk (16), 2, true, false, 0, 2), en (6, 2, 1500, 0.4f), PV { { "out", -9.0f } } }) },
        { "Batteria | Rullante che spacca", "TRANSISTOR sul corpo del rullante e pentode spinto.", cat ({ hg (clk (14), clk (11), clk (12), 1, false, false, 1, 2), en (5, 2, 700, 1.0f), PV { { "out", -3.5f } } }) },
        { "Batteria | Boom bap", "Sat LOW, alt tube, NASTRO sui bassi: cassa e rullante come da campionatore.", cat ({ hg (clk (13), clk (12), clk (13), 0, true, false, 0, 1), en (4, 1, 150, 0.8f), PV { { "out", -4.0f } } }) },
        { "Chitarra | Campione soul", "Valvole calde e TUBO sui medi: il campione si scalda come da vinile.", cat ({ hg (clk (13), clk (13), clk (11), 0, true, false, 0), en (4, 0, 900, 1.2f), PV { { "out", -5.0f } } }) },
        { "Keys | Rhodes in avanti", "Pentode ore 12, NASTRO sui medi-alti, un po' largo.", cat ({ hg (clk (12), clk (12), clk (10), 1, false, true), en (3.5f, 1, 1800, 1.3f), PV { { "out", -1.5f } } }) },
        { "Enhancer | Mix che brilla", "Solo enhancer, TUBO sulla presenza e largo in alto.", en (3.5f, 0, 4000, 1.35f) },
        { "Enhancer | Basso che si sente", "Solo enhancer: armoniche pari sui bassi, il basso esce anche dal telefono.", en (6, 0, 80, 0.6f) },
        { "Enhancer | Voce davanti", "Solo enhancer, NASTRO sui medi della voce.", en (4.5f, 1, 2000, 1.0f) },
        { "Enhancer | Grinta", "Solo enhancer, TRANSISTOR sui medi: piu' aggressivo, per synth e chitarre.", en (5, 2, 1200, 1.1f) },
        { "Catena | Valvole + Enhancer", "HG-2 sul bus e un filo di enhancer TUBO in alto.", cat ({ hg (clk (13), clk (12), clk (10), 0, true, true), en (2.5f, 0, 5000, 1.2f), PV { { "out", -2.7f } } }) } };
    return p;
}
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new jtbx::Processor(); }
