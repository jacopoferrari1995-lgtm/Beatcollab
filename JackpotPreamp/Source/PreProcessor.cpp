#include "PreProcessor.h"
#include "PreEditor.h"

namespace jpre
{
juce::AudioProcessorValueTreeState::ParameterLayout Processor::layout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    auto F = [&l] (const String& id, const String& n, NormalisableRange<float> r, float d) { l.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, n, r, d)); };
    auto B = [&l] (const char* id, const char* n, bool d) { l.add (std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, n, d)); };
    auto Cc = [&l] (const char* id, const char* n, StringArray s, int d) { l.add (std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, n, s, d)); };
    auto skew = [] (float a, float b, float c) { NormalisableRange<float> r (a, b); r.setSkewForCentre (c); return r; };
    // tutti gli stadi partono spenti: caricato non cambia il suono
    B ("s_on", "Sottrattivo on", false);
    F ("hp", "Passa alto", skew (20, 1000, 120), 20); Cc ("hpSl", "Pendenza passa alto", { "12", "24", "48" }, 1);
    F ("lp", "Passa basso", skew (1000, 20000, 8000), 20000); Cc ("lpSl", "Pendenza passa basso", { "12", "24", "48" }, 1);
    const float defF[4] = { 100, 400, 2500, 8000 };
    for (int k = 1; k <= 4; ++k)
    {
        F ("b" + String (k) + "f", "Taglio " + String (k) + " frequenza", skew (20, 20000, 1000), defF[k - 1]);
        F ("b" + String (k) + "g", "Taglio " + String (k) + " quantita", { -24, 0, 0.01f }, 0);
        F ("b" + String (k) + "q", "Taglio " + String (k) + " Q", skew (0.3f, 18, 3), 4);
    }
    B ("p_on", "Preamp on", false);
    StringArray pm; for (int i = 0; i < kPreModels; ++i) pm.add (preInfo (i).name);
    Cc ("p_model", "Preamp", pm, P_73);
    F ("p_gain", "Gain", { -10, 30, 0.01f }, 0); F ("p_out", "Uscita pre", { -24, 12, 0.01f }, 0);
    B ("p_phase", "Fase", false); B ("p_auto", "Gain compensato", true);
    B ("e_on", "EQ on", false);
    StringArray em; for (int i = 0; i < kEqModels; ++i) em.add (eqName (i));
    Cc ("e_model", "EQ", em, E_EQP1A);
    F ("pu_lb", "Pultec boost bassi", { 0, 10 }, 0); F ("pu_la", "Pultec attenua bassi", { 0, 10 }, 0); Cc ("pu_lf", "Pultec freq bassi", { "20", "30", "60", "100" }, 1);
    F ("pu_hb", "Pultec boost alti", { 0, 10 }, 0); F ("pu_bw", "Pultec banda", { 0, 10 }, 5); Cc ("pu_hf", "Pultec freq alti", { "3k", "4k", "5k", "8k", "10k", "12k", "16k" }, 3);
    F ("pu_ha", "Pultec attenua alti", { 0, 10 }, 0); Cc ("pu_as", "Pultec freq attenua", { "5k", "10k", "20k" }, 1);
    F ("tube", "Valvola", { 0, 1 }, 0.3f);
    Cc ("m_lf", "MEQ freq bassa", { "200", "300", "500", "700", "1k" }, 1); F ("m_lg", "MEQ picco basso", { 0, 10 }, 0);
    Cc ("m_dip", "MEQ freq dip", { "200", "300", "500", "700", "1k", "1.5k", "2k", "3k", "4k", "5k", "7k" }, 4); F ("m_dg", "MEQ dip", { 0, 10 }, 0);
    Cc ("m_hf", "MEQ freq alta", { "1.5k", "2k", "3k", "4k", "5k" }, 2); F ("m_hg", "MEQ picco alto", { 0, 10 }, 0);
    for (int k = 1; k <= 4; ++k) { F ("a_g" + String (k), "EQ banda " + String (k) + " guadagno", { -15, 15, 0.01f }, 0); F ("a_f" + String (k), "EQ banda " + String (k) + " frequenza", { 0, 1 }, 0.5f); }
    F ("a_q", "EQ Q", { 0, 1 }, 0.5f); F ("a_drive", "EQ colore", { 0, 1 }, 0.3f);
    F ("out", "Output", { -12, 12, 0.01f }, 0);
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
void Processor::prepareToPlay (double s, int) { sr = s; eng.prepare (s, kChunk); setLatencySamples (eng.latency()); }
Settings Processor::settings() const
{
    Settings S; auto b = [this] (const char* id) { return value (id) > 0.5f; };
    S.sOn = b ("s_on"); S.hpHz = value ("hp"); S.lpHz = value ("lp"); S.hpSlope = (int) value ("hpSl"); S.lpSlope = (int) value ("lpSl");
    for (int k = 0; k < 4; ++k)
    {
        const juce::String p = "b" + juce::String (k + 1);
        S.bF[k] = apvts.getRawParameterValue (p + "f")->load(); S.bG[k] = apvts.getRawParameterValue (p + "g")->load(); S.bQ[k] = apvts.getRawParameterValue (p + "q")->load();
    }
    S.pOn = b ("p_on"); S.pModel = (int) value ("p_model"); S.pGain = value ("p_gain"); S.pOut = value ("p_out"); S.pPhase = b ("p_phase"); S.pAuto = b ("p_auto");
    S.eOn = b ("e_on"); S.eModel = (int) value ("e_model");
    S.pu.lb = value ("pu_lb"); S.pu.la = value ("pu_la"); S.pu.lf = (int) value ("pu_lf"); S.pu.hb = value ("pu_hb"); S.pu.bw = value ("pu_bw"); S.pu.hf = (int) value ("pu_hf"); S.pu.ha = value ("pu_ha"); S.pu.as = (int) value ("pu_as");
    S.tube = value ("tube");
    S.mLf = (int) value ("m_lf"); S.mLg = value ("m_lg"); S.mDip = (int) value ("m_dip"); S.mDg = value ("m_dg"); S.mHf = (int) value ("m_hf"); S.mHg = value ("m_hg");
    for (int k = 0; k < 4; ++k) { S.ae.g[k] = apvts.getRawParameterValue ("a_g" + juce::String (k + 1))->load(); S.ae.f[k] = apvts.getRawParameterValue ("a_f" + juce::String (k + 1))->load(); }
    S.ae.q = value ("a_q"); S.ae.drive = value ("a_drive");
    S.outDb = value ("out");
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
    ring.push (L, R, n);
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

// ======================================================================= preset
using jkit::PV; using jkit::cat;
static PV pre (int m, float gain, float out = 0) { return { { "p_on", 1 }, { "p_model", (float) m }, { "p_gain", gain }, { "p_out", out } }; }
static PV hp (float f, int sl = 1) { return { { "s_on", 1 }, { "hp", f }, { "hpSl", (float) sl } }; }
static PV lp (float f, int sl = 1) { return { { "s_on", 1 }, { "lp", f }, { "lpSl", (float) sl } }; }
static PV cut (int k, float f, float g, float q) { const char* fs[] = { "b1f", "b2f", "b3f", "b4f" }; const char* gs[] = { "b1g", "b2g", "b3g", "b4g" }; const char* qs[] = { "b1q", "b2q", "b3q", "b4q" };
                                                   return { { "s_on", 1 }, { fs[k], f }, { gs[k], g }, { qs[k], q } }; }
static PV pultec (float lb, float la, int lf, float hb, float bw, int hf, float ha = 0, int as = 1, float tube = 0.3f)
{ return { { "e_on", 1 }, { "e_model", (float) E_EQP1A }, { "pu_lb", lb }, { "pu_la", la }, { "pu_lf", (float) lf }, { "pu_hb", hb }, { "pu_bw", bw }, { "pu_hf", (float) hf }, { "pu_ha", ha }, { "pu_as", (float) as }, { "tube", tube } }; }
static PV meq (int lf, float lg, int dip, float dg, int hf, float hg, float tube = 0.3f)
{ return { { "e_on", 1 }, { "e_model", (float) E_MEQ5 }, { "m_lf", (float) lf }, { "m_lg", lg }, { "m_dip", (float) dip }, { "m_dg", dg }, { "m_hf", (float) hf }, { "m_hg", hg }, { "tube", tube } }; }
static PV aeq (int model, std::initializer_list<float> g, std::initializer_list<float> f, float q = 0.5f, float drive = 0.3f)
{
    PV r { { "e_on", 1 }, { "e_model", (float) model }, { "a_q", q }, { "a_drive", drive } }; const char* gs[] = { "a_g1", "a_g2", "a_g3", "a_g4" }; const char* fs[] = { "a_f1", "a_f2", "a_f3", "a_f4" };
    int k = 0; for (float v : g) r.push_back ({ gs[k++], v }); k = 0; for (float v : f) r.push_back ({ fs[k++], v }); return r;
}
const std::vector<jkit::Preset>& presets()
{
    static const std::vector<jkit::Preset> p = {
        { "Init", "Tutto spento: accendi gli stadi che ti servono.", {} },
        { "Voce | Rap davanti", "Pulisci sotto i 90 e il fango a 300, 73 spinto, Pultec che apre in alto.", cat ({ hp (90), cut (1, 320, -3, 1.4f), pre (P_73, 12), pultec (0, 0, 1, 4, 4, 4, 0, 1, 0.35f) }) },
        { "Voce | Moderna ariosa", "737 a valvole pulito, MAAG con l'AIR a 20k.", cat ({ hp (80), cut (1, 250, -2.5f, 1.2f), pre (P_737, 8), aeq (E_MAAG, { 0, -1.5f, 1, 4 }, { 0.5f, 0.3f, 0, 0.75f }) }) },
        { "Voce | Calda anni 70", "V76 a valvole e MEQ-5: corpo e presenza morbida.", cat ({ hp (70), pre (P_V76, 14), meq (1, 2, 4, 2, 2, 3, 0.4f) }) },
        { "Voce | Togli la nasalita", "Tagli stretti a 1k e 3.5k, SSL pulito.", cat ({ hp (90), cut (1, 1000, -4, 5), cut (2, 3500, -3, 6), pre (P_SSL, 6) }) },
        { "Voce | De-mud", "Taglio largo nel medio basso e passa alto 24.", cat ({ hp (100), cut (0, 220, -4, 1.0f), cut (1, 450, -2, 2), pre (P_81, 6) }) },
        { "808 | Grosso dal pre", "TG2 che satura i bassi, Pultec bassi a 60.", cat ({ hp (25, 2), pre (P_TG2, 14), pultec (5, 3, 2, 0, 5, 3, 0, 1, 0.45f) }) },
        { "808 | Pulito che si sente", "SSL pulito, taglio a 250, 1084 a 100 Hz e 3k.", cat ({ hp (28, 2), cut (1, 250, -3, 1.5f), pre (P_SSL, 4), aeq (E_NEVE84, { 2, 2.5f, 0, 0 }, { 0.66f, 0.6f, 0, 0 }) }) },
        { "Basso | Trucco Pultec", "Boost e attenua a 60 Hz insieme: basso pieno senza fango.", cat ({ pre (P_610, 8), pultec (6, 4.5f, 2, 0, 5, 3, 0, 1, 0.4f) }) },
        { "Kick | Botta e click", "API che spinge, taglio a 400, Pultec a 60 e alti a 4k.", cat ({ hp (30), cut (1, 400, -5, 1.6f), pre (P_API, 12), pultec (4, 2, 2, 5, 3, 1, 0, 1, 0.3f) }) },
        { "Rullante | Corpo e schiocco", "1073 colorato, 73 EQ su 220 e 7.2k.", cat ({ hp (70), cut (1, 500, -2, 2), pre (P_73, 14), aeq (E_73, { 3, 2.5f, 3.5f, 0 }, { 0.66f, 1.0f, 0, 0.25f }, 0.5f, 0.45f) }) },
        { "Drum bus | Console", "SSL pre e SSL EQ: compatto e diretto.", cat ({ hp (25), pre (P_SSL, 10), aeq (E_SSL, { 1.5f, -2, 1.5f, 2 }, { 0.33f, 0.33f, 0.66f, 0.5f }, 0.4f, 0.35f) }) },
        { "Drum bus | Vintage", "TG2 e PE 1C: granuloso e morbido.", cat ({ hp (30), pre (P_TG2, 10), aeq (E_PE1C, { 4, -3, 3, -2 }, { 0.33f, 0.33f, 0.5f, 0.5f }, 0.5f, 0.4f) }) },
        { "Chitarra | API crunch", "API 312 spinto con taglio a 200 e presenza a 3k.", cat ({ hp (90), cut (0, 200, -3, 1.2f), pre (P_API, 18), aeq (E_SSL, { 0, 0, 2.5f, 1 }, { 0.5f, 0.5f, 0.5f, 0.5f }) }) },
        { "Piano | Rotondo", "81 e Lindell: medi larghi, aria leggera.", cat ({ hp (40), pre (P_81, 8), aeq (E_LIND81, { 1.5f, -1.5f, 1, 2 }, { 0.4f, 0.4f, 0.4f, 0.6f }) }) },
        { "Synth | Valvole", "610 e MEQ-5: piu grasso e meno digitale.", cat ({ pre (P_610, 12), meq (2, 2.5f, 7, 2, 3, 2, 0.5f) }) },
        { "Sample | Pulisci il loop", "Passa alto 48, tagli stretti sulle risonanze, niente colore.", cat ({ hp (60, 2), lp (16000, 1), cut (0, 180, -4, 3), cut (2, 2800, -3, 4) }) },
        { "Sample | Da vinile", "Banda stretta, V76 e MEQ: suono da disco.", cat ({ hp (110, 2), lp (9000, 1), pre (P_V76, 16), meq (1, 1, 6, 0, 2, 2, 0.6f) }) },
        { "Master | Pultec sul master", "Pultec leggero: bassi e aria, valvola bassa.", pultec (2, 1.5f, 1, 1.5f, 7, 5, 0, 2, 0.15f) },
        { "Master | Maag aria", "Solo l'AIR del MAAG a 20 kHz.", aeq (E_MAAG, { 0, 0, 0, 2.5f }, { 0, 0, 0, 0.75f }, 0.5f, 0.1f) },
        { "Sottrattivo | Rimbombo stanza", "Taglia la risonanza tipica di 120-250 Hz e il box a 500.", cat ({ hp (40), cut (0, 140, -5, 4), cut (1, 240, -4, 3), cut (2, 500, -3, 2.5f) }) },
        { "Sottrattivo | Asprezza", "Tagli stretti tra 2.5k e 6k, passa basso morbido.", cat ({ cut (1, 2600, -3, 5), cut (2, 4200, -3.5f, 6), cut (3, 6500, -2.5f, 5), lp (17000, 0) }) },
        { "Sottrattivo | Telefono", "Banda 400-3500: effetto radio.", cat ({ hp (400, 2), lp (3500, 2), pre (P_TG2, 16) }) } };
    return p;
}
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new jpre::Processor(); }
