#include "PreProcessor.h"
#include "PreEditor.h"
#include <map>

namespace jpre
{
juce::String sid (int slot, const char* what) { return "s" + juce::String (slot + 1) + "_" + what; }
const char* sidc (int slot, const char* what)
{
    static std::map<std::string, std::unique_ptr<std::string>> store;
    const auto k = sid (slot, what).toStdString();
    auto& p = store[k]; if (! p) p = std::make_unique<std::string> (k);
    return p->c_str();
}

juce::AudioProcessorValueTreeState::ParameterLayout Processor::layout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    auto F = [&l] (const String& id, const String& n, NormalisableRange<float> r, float d) { l.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, n, r, d)); };
    auto B = [&l] (const String& id, const String& n, bool d) { l.add (std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, n, d)); };
    auto Cc = [&l] (const String& id, const String& n, StringArray s, int d) { l.add (std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, n, s, d)); };
    auto skew = [] (float a, float b, float c) { NormalisableRange<float> r (a, b); r.setSkewForCentre (c); return r; };
    // tutto parte spento e il rack vuoto: caricato non cambia il suono
    B ("c_on", "Centro on", false);
    Cc ("c_mode", "Centro modo", { "EQ DINAMICO", "ANTI-RISONANZE" }, 0);
    l.add (std::make_unique<AudioParameterInt> (ParameterID { "c_pos", 1 }, "Posizione del centro", 0, kSlots, 3));
    F ("hp", "Passa alto", skew (20, 1000, 120), 20); Cc ("hpSl", "Pendenza passa alto", { "12", "24", "48" }, 1);
    F ("lp", "Passa basso", skew (1000, 20000, 8000), 20000); Cc ("lpSl", "Pendenza passa basso", { "12", "24", "48" }, 1);
    const float defF[4] = { 100, 400, 2500, 8000 };
    for (int k = 1; k <= 4; ++k)
    {
        const String p = "b" + String (k);
        F (p + "f", "Taglio " + String (k) + " frequenza", skew (20, 20000, 1000), defF[k - 1]);
        F (p + "g", "Taglio " + String (k) + " quantita", { -24, 0, 0.01f }, 0);
        F (p + "q", "Taglio " + String (k) + " Q", skew (0.3f, 18, 3), 4);
        B (p + "dyn", "Taglio " + String (k) + " dinamico", false);
        F (p + "thr", "Taglio " + String (k) + " soglia", { -60, 0, 0.1f }, -24);
        Cc (p + "ms", "Taglio " + String (k) + " canale", { "ST", "MID", "SIDE" }, 0);
        B (p + "solo", "Taglio " + String (k) + " solo", false);
    }
    F ("c_att", "Dinamica attacco", skew (0.5f, 100, 10), 10); F ("c_rel", "Dinamica rilascio", skew (20, 800, 150), 150);
    F ("r_depth", "Anti-risonanze profondita", { 0, 1 }, 0.5f); F ("r_sharp", "Anti-risonanze precisione", { 0, 1 }, 0.5f);
    F ("r_lo", "Anti-risonanze da", skew (20, 2000, 200), 150); F ("r_hi", "Anti-risonanze a", skew (1000, 20000, 6000), 14000);
    F ("r_speed", "Anti-risonanze velocita", { 0, 1 }, 0.5f); Cc ("r_ms", "Anti-risonanze canale", { "ST", "MID", "SIDE" }, 0); B ("r_delta", "Anti-risonanze delta", false);
    StringArray types; for (int t = 0; t < kModTypes; ++t) types.add (modName (t));
    for (int s = 0; s < kSlots; ++s)
    {
        const String n = "Slot " + String (s + 1);
        Cc (sid (s, "type"), n + " modulo", types, 0);
        B (sid (s, "byp"), n + " bypass", false);
        for (int i = 0; i < kSlotParams; ++i) F (sid (s, ("p" + String (i + 1)).toRawUTF8()), n + " comando " + String (i + 1), { 0, 1 }, 0);
    }
    F ("out", "Output", { -12, 12, 0.01f }, 0);
    return l;
}
Processor::Processor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true).withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", layout())
{
    for (int s = 0; s < kSlots; ++s)
    {
        slotPtr[s][0] = apvts.getRawParameterValue (sid (s, "type")); slotPtr[s][1] = apvts.getRawParameterValue (sid (s, "byp"));
        for (int i = 0; i < kSlotParams; ++i) slotPtr[s][2 + i] = apvts.getRawParameterValue (sid (s, ("p" + juce::String (i + 1)).toRawUTF8()));
    }
}
bool Processor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto o = l.getMainOutputChannelSet(), i = l.getMainInputChannelSet();
    return o == juce::AudioChannelSet::stereo() && (i == o || i == juce::AudioChannelSet::mono());
}
void Processor::prepareToPlay (double s, int) { sr = s; eng.prepare (s, kChunk); lat = eng.latency (settings()); setLatencySamples (lat); }
Settings Processor::settings() const
{
    Settings S; auto b = [this] (const juce::String& id) { return valueS (id) > 0.5f; };
    S.cOn = b ("c_on"); S.cMode = (int) value ("c_mode"); S.cPos = (int) value ("c_pos");
    S.hpHz = value ("hp"); S.lpHz = value ("lp"); S.hpSlope = (int) value ("hpSl"); S.lpSlope = (int) value ("lpSl");
    for (int k = 0; k < 4; ++k)
    {
        const juce::String p = "b" + juce::String (k + 1); auto& B = S.b[k];
        B.f = valueS (p + "f"); B.g = valueS (p + "g"); B.q = valueS (p + "q"); B.dyn = b (p + "dyn"); B.thr = valueS (p + "thr"); B.ms = (int) valueS (p + "ms"); B.solo = b (p + "solo");
    }
    S.att = value ("c_att"); S.rel = value ("c_rel");
    S.rDepth = value ("r_depth"); S.rSharp = value ("r_sharp"); S.rLo = value ("r_lo"); S.rHi = value ("r_hi"); S.rSpeed = value ("r_speed"); S.rMs = (int) value ("r_ms"); S.rDelta = value ("r_delta") > 0.5f;
    for (int s = 0; s < kSlots; ++s)
    {
        auto& X = S.slot[s]; X.type = juce::jlimit (0, kModTypes - 1, (int) slotPtr[s][0]->load()); X.byp = slotPtr[s][1]->load() > 0.5f;
        for (int i = 0; i < kSlotParams; ++i) X.v[i] = slotPtr[s][2 + i]->load();
    }
    S.outDb = value ("out");
    return S;
}
void Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals nd;
    const int n = buffer.getNumSamples(); if (n == 0) return;
    if (getTotalNumInputChannels() == 1) buffer.copyFrom (1, 0, buffer, 0, 0, n);
    const auto S = settings();
    const int l = eng.latency (S); if (l != lat) { lat = l; setLatencySamples (l); }   // cambia solo cambiando modulo o modo del centro
    float* L = buffer.getWritePointer (0); float* R = buffer.getWritePointer (1);
    for (int off = 0; off < n; off += kChunk) eng.process (L + off, R + off, juce::jmin (kChunk, n - off), S);
    ring.push (L, R, n);
}
void Processor::setP (const juce::String& id, float real) { if (auto* p = apvts.getParameter (id)) p->setValueNotifyingHost (p->convertTo0to1 (real)); }
void Processor::setModule (int s, int type)
{
    setP (sid (s, "type"), (float) type); setP (sid (s, "byp"), 0);
    const auto sp = modSpec (type);
    for (int i = 0; i < kSlotParams; ++i) setP (sid (s, ("p" + juce::String (i + 1)).toRawUTF8()), hidden (sp[(size_t) i]) ? 0.0f : specNorm (sp[(size_t) i], sp[(size_t) i].def));
}
void Processor::swapSlots (int a, int b)
{
    if (a == b) return;
    juce::StringArray ids { "type", "byp" }; for (int i = 0; i < kSlotParams; ++i) ids.add ("p" + juce::String (i + 1));
    for (auto& w : ids)
    {
        auto* pa = apvts.getParameter (sid (a, w.toRawUTF8())); auto* pb = apvts.getParameter (sid (b, w.toRawUTF8()));
        const float va = pa->getValue(), vb = pb->getValue(); pa->setValueNotifyingHost (vb); pb->setValueNotifyingHost (va);
    }
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
static PV core (bool dyn = false) { return { { "c_on", 1 }, { "c_mode", 0 } }; }
static PV hp (float f, int sl = 1) { return { { "c_on", 1 }, { "hp", f }, { "hpSl", (float) sl } }; }
static PV lp (float f, int sl = 1) { return { { "c_on", 1 }, { "lp", f }, { "lpSl", (float) sl } }; }
static PV cut (int k, float f, float g, float q, bool dyn = false, float thr = -24, int ms = 0)
{
    static const char* ids[4][6] = { { "b1f", "b1g", "b1q", "b1dyn", "b1thr", "b1ms" }, { "b2f", "b2g", "b2q", "b2dyn", "b2thr", "b2ms" }, { "b3f", "b3g", "b3q", "b3dyn", "b3thr", "b3ms" }, { "b4f", "b4g", "b4q", "b4dyn", "b4thr", "b4ms" } };
    return { { "c_on", 1 }, { ids[k][0], f }, { ids[k][1], g }, { ids[k][2], q }, { ids[k][3], dyn ? 1.0f : 0.0f }, { ids[k][4], thr }, { ids[k][5], (float) ms } };
}
static PV res (float depth, float sharp, float lo, float hi, float speed = 0.5f, int ms = 0)
{ return { { "c_on", 1 }, { "c_mode", 1 }, { "r_depth", depth }, { "r_sharp", sharp }, { "r_lo", lo }, { "r_hi", hi }, { "r_speed", speed }, { "r_ms", (float) ms } }; }
// modulo nello slot: valori reali per indice (gli altri restano ai valori di partenza del modulo)
static PV mod (int s, int type, std::initializer_list<std::pair<int, float>> vals)
{
    PV r { { sidc (s, "type"), (float) type } };
    const auto sp = modSpec (type); std::vector<float> v ((size_t) kSlotParams);
    for (int i = 0; i < kSlotParams; ++i) v[(size_t) i] = hidden (sp[(size_t) i]) ? 0.0f : specNorm (sp[(size_t) i], sp[(size_t) i].def);
    for (auto& [i, x] : vals) v[(size_t) i] = specNorm (sp[(size_t) i], x);
    static const char* pn[] = { "p1", "p2", "p3", "p4", "p5", "p6", "p7", "p8", "p9", "p10" };
    for (int i = 0; i < kSlotParams; ++i) r.push_back ({ sidc (s, pn[i]), v[(size_t) i] });
    return r;
}
static int PRE (int m) { return 1 + m; }
static int EQ (int m) { return 1 + kPreModels + m; }
static PV pos (int p) { return { { "c_pos", (float) p } }; }
// scorciatoie per i moduli
static PV pre (int s, int m, float gain, float mix = 1, int lowcut = 0) { return mod (s, PRE (m), { { 0, gain }, { 3, mix }, { 2, (float) lowcut } }); }
static PV pultec (int s, int lf, float lb, float la, int hf, float hb, float bw, float ha = 0, int as = 1, float tube = 0.3f) { return mod (s, EQ (E_EQP1A), { { 0, (float) lf }, { 1, lb }, { 2, la }, { 3, (float) hf }, { 4, hb }, { 5, bw }, { 6, (float) as }, { 7, ha }, { 8, tube } }); }
static PV meq (int s, int lf, float lg, int dip, float dg, int hf, float hg, float tube = 0.3f) { return mod (s, EQ (E_MEQ5), { { 0, (float) lf }, { 1, lg }, { 2, (float) dip }, { 3, dg }, { 4, (float) hf }, { 5, hg }, { 6, tube } }); }
static PV aeq (int s, int model, std::initializer_list<float> g, std::initializer_list<float> f, float q = 0.5f, float drive = 0.3f)
{
    std::vector<std::pair<int, float>> v; int k = 0; for (float x : g) v.push_back ({ k++, x }); k = 4; for (float x : f) v.push_back ({ k++, x }); v.push_back ({ 8, q }); v.push_back ({ 9, drive });
    PV r { { sidc (s, "type"), (float) EQ (model) } }; const auto sp = modSpec (EQ (model));
    std::vector<float> vv ((size_t) kSlotParams); for (int i = 0; i < kSlotParams; ++i) vv[(size_t) i] = hidden (sp[(size_t) i]) ? 0.0f : specNorm (sp[(size_t) i], sp[(size_t) i].def);
    for (auto& [i, x] : v) if (! hidden (sp[(size_t) i])) vv[(size_t) i] = specNorm (sp[(size_t) i], x);
    static const char* pn[] = { "p1", "p2", "p3", "p4", "p5", "p6", "p7", "p8", "p9", "p10" };
    for (int i = 0; i < kSlotParams; ++i) r.push_back ({ sidc (s, pn[i]), vv[(size_t) i] });
    return r;
}
const std::vector<jkit::Preset>& presets()
{
    static const std::vector<jkit::Preset> p = {
        { "Init", "Rack vuoto e centro spento: scegli i moduli dal menu di ogni slot.", {} },
        { "Voce | Rap davanti", "73 spinto, centro che pulisce fango e asprezza solo quando servono, Pultec che apre.",
          cat ({ pre (0, P_73, 12, 1, 2), hp (90), cut (1, 320, -4, 1.4f, true, -30), cut (2, 3200, -5, 4, true, -32), pos (1), pultec (1, 1, 0, 0, 4, 4, 4, 0, 1, 0.35f) }) },
        { "Voce | Moderna ariosa", "737 pulito, anti-risonanze sui medi alti, MAAG con l'AIR a 20k.",
          cat ({ pre (0, P_737, 8, 1, 2), res (0.45f, 0.6f, 800, 9000, 0.6f, 1), pos (1), aeq (1, E_MAAG, { 0, -1.5f, 1, 4 }, { 0.5f, 0.3f, 0, 0.75f }) }) },
        { "Voce | Calda anni 70", "V76 a valvole, MEQ-5 e un taglio dinamico sul medio basso.",
          cat ({ pre (0, P_V76, 14, 1, 2), hp (70), cut (0, 280, -4, 1.2f, true, -28), pos (1), meq (1, 1, 2, 4, 2, 2, 3, 0.4f) }) },
        { "Voce | Nasale fuori", "Tagli dinamici stretti a 1k e 3.5k: lavorano solo sulle note che suonano nasali.",
          cat ({ pre (0, P_SSL, 6), hp (90), cut (1, 1000, -6, 5, true, -30), cut (2, 3500, -6, 6, true, -32), pos (1) }) },
        { "Voce | Sibilanti e durezze", "Anti-risonanze sulla zona 4-12k, veloce: un de-esser che lavora solo dove fischia.",
          cat ({ pre (0, P_81, 6), res (0.6f, 0.7f, 4000, 12000, 0.9f), pos (1) }) },
        { "Voce | Doppio pre", "Due pre in serie come si faceva in studio: 610 morbido e poi 1272 crudo, poco mix.",
          cat ({ pre (0, P_610, 10), pre (1, P_1272, 10, 0.45f), hp (80), pos (2) }) },
        { "808 | Grosso dal pre", "TG2 che satura i bassi, Pultec bassi a 60, il centro tiene il sub sotto controllo.",
          cat ({ hp (25, 2), pos (0), pre (0, P_TG2, 14), pultec (1, 2, 5, 3, 3, 0, 5, 0, 1, 0.45f) }) },
        { "808 | Pulito che si sente", "SSL pulito, taglio a 250 e 1084 a 100 Hz e 3k.",
          cat ({ hp (28, 2), cut (1, 250, -3, 1.5f), pos (0), pre (0, P_SSL, 4), aeq (1, E_NEVE84, { 2, 2.5f, 0 }, { 0.66f, 0.6f, 0, 0 }) }) },
        { "Basso | Trucco Pultec", "Boost e attenua a 60 Hz insieme: basso pieno senza fango.", cat ({ pre (0, P_610, 8), pultec (1, 2, 6, 4.5f, 3, 0, 5, 0, 1, 0.4f) }) },
        { "Kick | Botta e click", "API che spinge, taglio dinamico a 400, Pultec a 60 e alti a 4k.",
          cat ({ pre (0, P_API, 12), hp (30), cut (1, 400, -6, 1.6f, true, -26), pos (1), pultec (1, 2, 4, 2, 1, 5, 3, 0, 1, 0.3f) }) },
        { "Rullante | Corpo e schiocco", "1073 colorato, ring tolto con un taglio stretto, 73 EQ su 220 e 7.2k.",
          cat ({ pre (0, P_73, 14), hp (70), cut (2, 900, -6, 8, true, -30), pos (1), aeq (1, E_73, { 3, 2.5f, 3.5f }, { 0.66f, 1.0f, 0, 0.25f }, 0.5f, 0.45f) }) },
        { "Drum bus | Console", "SSL pre e SSL EQ: compatto e diretto.", cat ({ pre (0, P_SSL, 10), aeq (1, E_SSL, { 1.5f, -2, 1.5f, 2 }, { 0.33f, 0.33f, 0.66f, 0.5f }, 0.4f, 0.35f) }) },
        { "Drum bus | Vintage", "TG2, Helios e PE 1C: granuloso e morbido.", cat ({ pre (0, P_TG2, 10), pre (1, P_HELIOS, 6, 0.5f), aeq (2, E_PE1C, { 4, -3, 3, -2 }, { 0.33f, 0.33f, 0.5f, 0.5f }, 0.5f, 0.4f) }) },
        { "Chitarra | Helios crunch", "Helios spinto, anti-risonanze sui medi, presenza a 3k.",
          cat ({ pre (0, P_HELIOS, 18, 1, 2), res (0.5f, 0.5f, 300, 5000, 0.5f), pos (1), aeq (1, E_SSL, { 0, 0, 2.5f, 1 }, { 0.5f, 0.5f, 0.5f, 0.5f }) }) },
        { "Piano | Rotondo", "Trident e Lindell: medi larghi, aria leggera.", cat ({ pre (0, P_TRIDENT, 8), aeq (1, E_LIND81, { 1.5f, -1.5f, 1, 2 }, { 0.4f, 0.4f, 0.4f, 0.6f }) }) },
        { "Synth | Valvole", "610 e MEQ-5: piu grasso e meno digitale.", cat ({ pre (0, P_610, 12), meq (1, 2, 2.5f, 7, 2, 3, 2, 0.5f) }) },
        { "Sample | Pulisci il loop", "Passa alto 48, anti-risonanze su tutto: toglie i fischi del campione.",
          cat ({ hp (60, 2), lp (16000, 1), res (0.55f, 0.55f, 120, 14000, 0.5f), pos (0) }) },
        { "Sample | Da vinile", "Banda stretta, V76 e MEQ: suono da disco.", cat ({ hp (110, 2), lp (9000, 1), pos (0), pre (0, P_V76, 16), meq (1, 1, 1, 6, 0, 2, 2, 0.6f) }) },
        { "Master | Pultec e aria", "Pultec leggero e MAAG solo AIR: bassi e aria, valvola bassa.", cat ({ pultec (0, 1, 2, 1.5f, 5, 1.5f, 7, 0, 2, 0.15f), aeq (1, E_MAAG, { 0, 0, 0, 2.5f }, { 0, 0, 0, 0.75f }, 0.5f, 0.1f) }) },
        { "Master | Anti-durezza", "Anti-risonanze leggero e lento sui medi alti, solo sul mid.", cat ({ res (0.3f, 0.4f, 1500, 9000, 0.25f, 1), pos (0) }) },
        { "Mix | Side pulito", "Bassi via dai lati e un taglio dinamico sul side quando i medi si gonfiano.",
          cat ({ cut (0, 120, -12, 0.7f, false, -24, 2), cut (1, 450, -4, 1.0f, true, -34, 2), pos (0) }) },
        { "Sottrattivo | Rimbombo stanza", "Taglia la risonanza tipica di 120-250 Hz e il box a 500, in dinamico.",
          cat ({ hp (40), cut (0, 140, -6, 4, true, -30), cut (1, 240, -5, 3, true, -30), cut (2, 500, -3, 2.5f, true, -32) }) },
        { "Sottrattivo | Telefono", "Banda 400-3500 e TG2 tirato: effetto radio.", cat ({ hp (400, 2), lp (3500, 2), pos (0), pre (0, P_TG2, 16) }) } };
    return p;
}
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new jpre::Processor(); }
