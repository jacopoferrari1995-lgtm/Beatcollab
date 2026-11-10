#include "DustProcessor.h"
#include "DustEditor.h"

namespace jds
{
juce::AudioProcessorValueTreeState::ParameterLayout Processor::layout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    auto F = [&l] (const char* id, const char* n, NormalisableRange<float> r, float d) { l.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, n, r, d)); };
    auto B = [&l] (const char* id, const char* n, bool d) { l.add (std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, n, d)); };
    auto Cc = [&l] (const char* id, const char* n, StringArray s, int d) { l.add (std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, n, s, d)); };
    auto skew = [] (float a, float b, float c) { NormalisableRange<float> r (a, b); r.setSkewForCentre (c); return r; };
    StringArray sm; for (int i = 0; i < kSModels; ++i) sm.add (sInfo (i).name);
    StringArray dm; for (int i = 0; i < kDModels; ++i) dm.add (dName (i));
    StringArray cm; for (int i = 0; i < kCStyles; ++i) cm.add (cName (i));
    F ("in", "Input", { -12, 12 }, 0);
    // tutti i moduli partono spenti: il plugin appena caricato non cambia il suono
    B ("s_on", "Sampler on", false); Cc ("s_model", "Macchina", sm, S_SP12); F ("s_bits", "Bit", { 4, 16, 1 }, 12); F ("s_rate", "Frequenza", skew (4000, 48000, 22050), 26040); F ("s_mix", "Sampler mix", { 0, 1 }, 1);
    B ("d_on", "Saturazione on", false); Cc ("d_model", "Saturazione tipo", dm, D_TAPE); F ("d_drive", "Drive", { 0, 24 }, 6); F ("d_tone", "Colore", { -1, 1 }, 0); F ("d_mix", "Saturazione mix", { 0, 1 }, 1);
    B ("c_on", "Comp on", false); Cc ("c_style", "Comp tipo", cm, C_FET); F ("c_amt", "Compressione", { 0, 1 }, 0.4f); F ("c_att", "Attacco", skew (0.1f, 50, 8), 10); F ("c_rel", "Rilascio", skew (20, 1000, 150), 120);
    F ("c_punch", "Punch", { -1, 1 }, 0); F ("c_mix", "Comp mix", { 0, 1 }, 1);
    B ("f_on", "Filtri on", false); F ("f_hp", "Passa alto", skew (20, 1000, 120), 20); F ("f_lp", "Passa basso", skew (500, 20000, 4000), 20000); F ("f_res", "Risonanza", { 0, 1 }, 0);
    B ("f_24", "Pendenza 24", false); F ("f_boom", "Boom", { -12, 12 }, 0); F ("f_crack", "Crack", { -12, 12 }, 0);
    B ("t_on", "Nastro on", false); F ("t_age", "Eta nastro", { 0, 1 }, 0.4f); F ("t_wow", "Wow", { 0, 1 }, 0.2f); F ("t_flut", "Flutter", { 0, 1 }, 0.2f); F ("t_drop", "Dropout", { 0, 1 }, 0);
    B ("n_on", "Polvere on", false); F ("n_vinyl", "Vinile", { 0, 1 }, 0.4f); F ("n_pops", "Pop", { 0, 1 }, 0.3f); F ("n_hiss", "Hiss", { 0, 1 }, 0.2f); F ("n_hum", "Ronzio", { 0, 1 }, 0); B ("n_follow", "Segue la batteria", true);
    F ("amount", "Intensita", { 0, 1.5f }, 1); F ("mix", "Mix", { 0, 1 }, 1); F ("out", "Output", { -12, 12 }, 0); B ("mono", "Mono", false); B ("auto", "Volume automatico", false);
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
void Processor::prepareToPlay (double s, int block)
{
    sr = s; maxBlock = juce::jmax (64, block); eng.prepare (s, maxBlock); mono.assign ((size_t) maxBlock, 0.0f);
    setLatencySamples (eng.latency());
}
Settings Processor::settings() const
{
    Settings S; auto b = [this] (const char* id) { return value (id) > 0.5f; };
    S.inDb = value ("in");
    S.sOn = b ("s_on"); S.sModel = (int) value ("s_model"); S.sBits = value ("s_bits"); S.sRate = value ("s_rate"); S.sMix = value ("s_mix");
    S.dOn = b ("d_on"); S.dModel = (int) value ("d_model"); S.dDrive = value ("d_drive"); S.dTone = value ("d_tone"); S.dMix = value ("d_mix");
    S.cOn = b ("c_on"); S.cStyle = (int) value ("c_style"); S.cAmt = value ("c_amt"); S.cAtt = value ("c_att"); S.cRel = value ("c_rel"); S.cPunch = value ("c_punch"); S.cMix = value ("c_mix");
    S.fOn = b ("f_on"); S.fHp = value ("f_hp"); S.fLp = value ("f_lp"); S.fRes = value ("f_res"); S.f24 = b ("f_24"); S.fBoom = value ("f_boom"); S.fCrack = value ("f_crack");
    S.tOn = b ("t_on"); S.tAge = value ("t_age"); S.tWow = value ("t_wow"); S.tFlut = value ("t_flut"); S.tDrop = value ("t_drop");
    S.nOn = b ("n_on"); S.nVinyl = value ("n_vinyl"); S.nPops = value ("n_pops"); S.nHiss = value ("n_hiss"); S.nHum = value ("n_hum"); S.nFollow = b ("n_follow");
    S.amount = value ("amount"); S.mix = value ("mix"); S.outDb = value ("out"); S.mono = b ("mono"); S.autoGain = b ("auto");
    return S;
}
void Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals nd;
    const int n = buffer.getNumSamples(); if (n == 0) return;
    if (getTotalNumInputChannels() == 1) buffer.copyFrom (1, 0, buffer, 0, 0, n);
    float* L = buffer.getWritePointer (0); float* R = buffer.getWritePointer (1);
    float pk[2] = { 0, 0 };
    for (int i = 0; i < n; ++i) { pk[0] = juce::jmax (pk[0], std::abs (L[i])); pk[1] = juce::jmax (pk[1], std::abs (R[i])); }
    inPk[0] = juce::jmax (pk[0], inPk[0].load() * 0.9f); inPk[1] = juce::jmax (pk[1], inPk[1].load() * 0.9f);
    const auto S = settings();
    for (int off = 0; off < n; off += maxBlock) eng.process (L + off, R + off, juce::jmin (maxBlock, n - off), S);   // l'oversampling vuole blocchi al massimo grandi come in prepare
    pk[0] = pk[1] = 0;
    for (int i = 0; i < n; ++i) { pk[0] = juce::jmax (pk[0], std::abs (L[i])); pk[1] = juce::jmax (pk[1], std::abs (R[i])); }
    outPk[0] = juce::jmax (pk[0], outPk[0].load() * 0.9f); outPk[1] = juce::jmax (pk[1], outPk[1].load() * 0.9f);
    if ((int) mono.size() < n) mono.resize ((size_t) n);
    for (int i = 0; i < n; ++i) mono[(size_t) i] = 0.5f * (L[i] + R[i]);
    ringOut.push (mono.data(), n);
}
void Processor::loadPreset (int i)
{
    if (! juce::isPositiveAndBelow (i, (int) presets().size())) return;
    preset = i;
    for (auto* p : getParameters()) if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p)) if (rp->paramID != "out" && rp->paramID != "in") rp->setValueNotifyingHost (rp->getDefaultValue());
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
// abbreviazioni per scrivere i preset in poco spazio
using PV = std::vector<std::pair<const char*, float>>;
static PV sam (int m, float bits, float rate, float mix = 1) { return { { "s_on", 1 }, { "s_model", (float) m }, { "s_bits", bits }, { "s_rate", rate }, { "s_mix", mix } }; }
static PV sat (int m, float drive, float tone = 0, float mix = 1) { return { { "d_on", 1 }, { "d_model", (float) m }, { "d_drive", drive }, { "d_tone", tone }, { "d_mix", mix } }; }
static PV cmp (int st, float amt, float att, float rel, float punch = 0, float mix = 1) { return { { "c_on", 1 }, { "c_style", (float) st }, { "c_amt", amt }, { "c_att", att }, { "c_rel", rel }, { "c_punch", punch }, { "c_mix", mix } }; }
static PV flt (float hp, float lp, float boom = 0, float crack = 0, float res = 0, bool s24 = false) { return { { "f_on", 1 }, { "f_hp", hp }, { "f_lp", lp }, { "f_boom", boom }, { "f_crack", crack }, { "f_res", res }, { "f_24", s24 ? 1.0f : 0.0f } }; }
static PV tap (float age, float wow, float flut, float drop = 0) { return { { "t_on", 1 }, { "t_age", age }, { "t_wow", wow }, { "t_flut", flut }, { "t_drop", drop } }; }
static PV dst (float vin, float pops, float hiss, float hum = 0, bool follow = true) { return { { "n_on", 1 }, { "n_vinyl", vin }, { "n_pops", pops }, { "n_hiss", hiss }, { "n_hum", hum }, { "n_follow", follow ? 1.0f : 0.0f } }; }
static PV cat (std::initializer_list<PV> parts) { PV r; for (auto& p : parts) r.insert (r.end(), p.begin(), p.end()); return r; }

const std::vector<Preset>& presets()
{
    static const std::vector<Preset> p = {
        { "Init", "Tutto spento: accendi i moduli che ti servono.", {} },
        // generi, sul bus della batteria
        { "Boom Bap | Bus classico", "SP-12 parallelo, nastro leggero e un velo di vinile: la batteria 90 pronta.", cat ({ sam (S_SP12, 12, 26040, 0.8f), sat (D_TAPE, 6), cmp (C_FET, 0.45f, 8, 120, 0.3f), flt (30, 14000, 2, 1.5f), tap (0.3f, 0.1f, 0.15f), dst (0.25f, 0.15f, 0.1f) }) },
        { "Boom Bap | Polveroso anni 90", "S950 scuro, valvola e vinile in evidenza.", cat ({ sam (S_S950, 12, 32000), sat (D_TUBE, 8), cmp (C_BUS, 0.5f, 12, 180), flt (40, 9000, 3), tap (0.5f, 0.2f, 0.2f), dst (0.45f, 0.3f, 0.2f, 0.1f) }) },
        { "Boom Bap | Batteria da vinile", "Come campionata da un disco: MPC 60, console, mono.", cat ({ sam (S_MPC60, 12, 40000), sat (D_CONSOLE, 7), cmp (C_FET, 0.55f, 6, 110, 0.4f), flt (45, 12000, 0, 2), dst (0.6f, 0.4f, 0.15f), PV { { "mono", 1 } } }) },
        { "Boom Bap | Knock pesante", "Cassa e rullante che bussano: spinta, FET veloce, boom.", cat ({ sam (S_SP12, 12, 26040, 0.6f), sat (D_PUSH, 7), cmp (C_FET, 0.6f, 4, 90, 0.5f), flt (32, 13000, 3.5f, 2.5f) }) },
        { "Lo-fi | Chill tape", "Nastro morbido e oscillante, alti chiusi, polvere costante.", cat ({ sam (S_CLEAN, 12, 22050, 0.6f), sat (D_TAPE, 5, -0.3f), cmp (C_BUS, 0.35f, 20, 250), flt (60, 6500, 0, 0, 0.1f), tap (0.7f, 0.45f, 0.35f, 0.1f), dst (0.5f, 0.2f, 0.35f) }) },
        { "Lo-fi | Cassetta stanca", "Cassetta consumata: crush leggero, wow, dropout e fruscio.", cat ({ sam (S_CRUSH, 10, 16000, 0.5f), sat (D_TAPE, 9), flt (90, 5000), tap (1.0f, 0.6f, 0.5f, 0.3f), dst (0.2f, 0.1f, 0.6f) }) },
        { "Lo-fi | Notturno in cameretta", "Soffice e lontano: niente transienti duri, tanto calore.", cat ({ sat (D_TUBE, 6, -0.4f), cmp (C_BUS, 0.45f, 25, 300, -0.4f), flt (70, 4800, 1.5f, -2), tap (0.6f, 0.3f, 0.25f), dst (0.45f, 0.2f, 0.3f) }) },
        { "Trip Hop | Pesante e scuro", "Batteria lenta e enorme: pompa, scura, mono.", cat ({ sam (S_S950, 12, 24000), sat (D_TUBE, 10, -0.5f), cmp (C_PUMP, 0.6f, 25, 300), flt (35, 7000, 4, 0, 0.15f), tap (0.6f, 0.15f, 0.15f), dst (0.4f, 0.2f, 0.25f), PV { { "mono", 1 } } }) },
        { "Trip Hop | Break rallentato", "Il break campionato e rallentato: SP-12, console, vinile.", cat ({ sam (S_SP12, 12, 20000), sat (D_CONSOLE, 8), cmp (C_FET, 0.6f, 6, 140, 0.3f), flt (30, 8000, 3), tap (0.5f, 0.25f, 0.2f), dst (0.5f, 0.3f, 0.15f) }) },
        { "Old School | 12 bit crudo", "Solo la macchina: SP-12 pieno e un FET deciso.", cat ({ sam (S_SP12, 12, 26040), sat (D_PUSH, 6), cmp (C_FET, 0.5f, 3, 100, 0.2f), flt (30, 20000) }) },
        { "Old School | MPC grosso", "MPC 60 caldo, valvola, corpo in basso.", cat ({ sam (S_MPC60, 12, 40000), sat (D_TUBE, 7), cmp (C_FET, 0.5f, 8, 120, 0.2f), flt (25, 20000, 2.5f, 1) }) },
        { "Jazz Hop | Spazzole morbide", "Kit acustico morbido: bus lento, nastro, vinile.", cat ({ sat (D_TAPE, 4), cmp (C_BUS, 0.3f, 20, 250), flt (50, 10000), tap (0.4f, 0.25f, 0.2f), dst (0.5f, 0.2f, 0.15f) }) },
        { "Rap | Moderna ma sporca", "Batteria attuale con un velo di 12 bit e crack.", cat ({ sam (S_SP12, 12, 26040, 0.35f), sat (D_PUSH, 5), cmp (C_FET, 0.5f, 6, 100, 0.45f), flt (28, 20000, 1, 2) }) },
        { "Phonk | Cassetta Memphis", "Crush, spinta forte e nastro rovinato.", cat ({ sam (S_CRUSH, 10, 18000, 0.7f), sat (D_PUSH, 10), cmp (C_FET, 0.65f, 3, 90, 0.2f), flt (30, 9000, 3), tap (0.7f, 0.3f, 0.25f, 0.15f), dst (0.3f, 0.1f, 0.5f) }) },
        // singoli pezzi
        { "Cassa | Knock boom bap", "Cassa corta e piena che buca il mix.", cat ({ sam (S_SP12, 12, 26040, 0.7f), sat (D_TUBE, 8), cmp (C_FET, 0.5f, 12, 90, 0.4f), flt (30, 9000, 4) }) },
        { "Cassa | Morbida lo-fi", "Cassa rotonda, senza click, con coda calda.", cat ({ sam (S_CLEAN, 12, 22050, 0.6f), sat (D_TAPE, 6, -0.3f), cmp (C_BUS, 0.4f, 20, 200), flt (25, 5000, 3), tap (0.5f, 0.1f, 0.1f) }) },
        { "Cassa | Thump trip hop", "Cassa enorme e scura che respira.", cat ({ sat (D_CONSOLE, 9), cmp (C_PUMP, 0.5f, 25, 250), flt (25, 4500, 5, 0, 0.2f), tap (0.4f, 0.05f, 0.05f) }) },
        { "Rullante | Crack 12 bit", "Rullante che spacca, granuloso.", cat ({ sam (S_SP12, 12, 26040), sat (D_PUSH, 7), cmp (C_FET, 0.55f, 2, 90, 0.3f), flt (120, 20000, 0, 3) }) },
        { "Rullante | Polveroso", "Rullante da disco vecchio, morbido in cima.", cat ({ sam (S_S950, 12, 28000), sat (D_TAPE, 7), cmp (C_BUS, 0.4f, 10, 150), flt (150, 9000, 0, 2), tap (0.5f, 0.1f, 0.15f), dst (0.4f, 0.15f, 0.1f) }) },
        { "Rullante | Rim schiacciato", "Rim e clap stretti, filtrati e cattivi.", cat ({ sam (S_CRUSH, 9, 16000, 0.6f), sat (D_PUSH, 9), cmp (C_FET, 0.7f, 0.5f, 70), flt (250, 8000, 0, 4, 0.2f) }) },
        { "Hi-hat | Croccanti", "Hat granulosi e presenti.", cat ({ sam (S_SP12, 12, 26040), sat (D_PUSH, 4), flt (400, 12000, 0, 0, 0.1f) }) },
        { "Hi-hat | Lo-fi soffici", "Hat lontani e morbidi, nastro.", cat ({ sam (S_CLEAN, 10, 18000, 0.6f), flt (300, 6500), tap (0.6f, 0.2f, 0.2f) }) },
        { "Hi-hat | Telefono", "Hat filtrati stretti, 8 bit.", cat ({ sam (S_CRUSH, 8, 11025, 0.7f), flt (900, 4000, 0, 0, 0.3f) }) },
        { "Perc | Bongo vintage", "Percussioni calde da vecchio disco.", cat ({ sam (S_MPC60, 12, 40000), sat (D_TUBE, 6), flt (80, 10000), tap (0.4f, 0.15f, 0.15f), dst (0.3f, 0.1f, 0.1f) }) },
        { "Loop | Break dusty", "Il break completo: S950, nastro, vinile, punch.", cat ({ sam (S_S950, 12, 26000), sat (D_TAPE, 8), cmp (C_FET, 0.55f, 6, 120, 0.3f), flt (40, 9500, 2, 1.5f), tap (0.4f, 0.15f, 0.15f), dst (0.5f, 0.3f, 0.15f) }) },
        { "Loop | Break da campionare", "SP-12 e console: pronto da choppare.", cat ({ sam (S_SP12, 12, 26040), sat (D_CONSOLE, 6), cmp (C_FET, 0.6f, 5, 100, 0.2f), flt (35, 12000), dst (0.3f, 0.15f, 0.1f) }) },
        { "Bus | Colla vintage", "Tiene insieme il kit senza sporcarlo troppo.", cat ({ sat (D_TAPE, 4), cmp (C_BUS, 0.45f, 15, 200, 0, 0.7f), flt (25, 20000), tap (0.25f, 0.05f, 0.1f) }) },
        { "Bus | Parallelo schiacciato", "Compressione New York: il kit schiacciato sotto all'originale.", cat ({ sat (D_PUSH, 9), cmp (C_FET, 0.9f, 1, 80, 0.2f, 0.4f), flt (20, 20000, 2) }) },
        { "Effetti | Radio AM", "Batteria da radio: stretta, sporca, ronzio.", cat ({ sam (S_CRUSH, 8, 8000, 0.8f), sat (D_PUSH, 10), flt (400, 3500, 0, 0, 0.3f, true), dst (0, 0, 0.5f, 0.3f), PV { { "mono", 1 }, { "auto", 1 } } }) },
        { "Effetti | Vinile 78 giri", "Gommalacca: banda stretta, tanto crackle.", cat ({ sam (S_CLEAN, 10, 16000), flt (250, 5000), tap (0.3f, 0.3f, 0.2f), dst (0.9f, 0.7f, 0.4f), PV { { "mono", 1 }, { "auto", 1 } } }) },
        { "Effetti | Nastro che muore", "Wow, flutter e dropout al massimo.", cat ({ tap (1.0f, 0.9f, 0.7f, 0.7f), dst (0, 0, 0.6f), PV { { "amount", 1.2f } } }) },
        { "Effetti | Solo vinile", "Aggiunge solo la polvere del disco, sempre accesa.", cat ({ dst (0.5f, 0.3f, 0.15f, 0, false) }) } };
    return p;
}
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new jds::Processor(); }
