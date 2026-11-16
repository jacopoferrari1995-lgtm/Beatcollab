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
    B ("f_24", "Pendenza 24", false); F ("f_boom", "Boom", { -12, 12 }, 0); F ("f_crack", "Crack", { -12, 12 }, 0); F ("f_warm", "Caldo", { 0, 1 }, 0); F ("f_air", "Aria", { 0, 1 }, 0);
    B ("t_on", "Nastro on", false); F ("t_age", "Eta nastro", { 0, 1 }, 0.4f); F ("t_wow", "Wow", { 0, 1 }, 0.2f); F ("t_flut", "Flutter", { 0, 1 }, 0.2f); F ("t_drop", "Dropout", { 0, 1 }, 0);
    B ("n_on", "Polvere on", false); F ("n_vinyl", "Vinile", { 0, 1 }, 0.4f); F ("n_pops", "Pop", { 0, 1 }, 0.3f); F ("n_hiss", "Hiss", { 0, 1 }, 0.2f); F ("n_hum", "Ronzio", { 0, 1 }, 0); B ("n_follow", "Segue la batteria", true);
    B ("r_on", "Room on", false); F ("r_size", "Room grandezza", { 0, 1 }, 0.4f); F ("r_decay", "Room coda", { 0, 1 }, 0.4f); F ("r_tone", "Room tono", { 0, 1 }, 0.5f); F ("r_mix", "Room mix", { 0, 1 }, 0.2f); B ("r_gate", "Room gate", false);
    F ("amount", "Carattere", { 0, 1.5f }, 1); F ("mix", "Mix", { 0, 1 }, 1); F ("out", "Output", { -12, 12 }, 0); B ("mono", "Mono", false); B ("auto", "Volume automatico", false);
    return l;
}

Processor::Processor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true).withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", layout())
{
    // puntatori presi una volta: nel thread audio niente ricerche per nome (allocavano memoria a ogni blocco)
    for (auto* pr : getParameters()) if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (pr)) raw[rp->paramID] = apvts.getRawParameterValue (rp->paramID);
    auto R = [this] (const char* id) { return raw.at (id); };
    pIn = R ("in"); pSOn = R ("s_on"); pSModel = R ("s_model"); pSBits = R ("s_bits"); pSRate = R ("s_rate"); pSMix = R ("s_mix");
    pDOn = R ("d_on"); pDModel = R ("d_model"); pDDrive = R ("d_drive"); pDTone = R ("d_tone"); pDMix = R ("d_mix");
    pCOn = R ("c_on"); pCStyle = R ("c_style"); pCAmt = R ("c_amt"); pCAtt = R ("c_att"); pCRel = R ("c_rel"); pCPunch = R ("c_punch"); pCMix = R ("c_mix");
    pFOn = R ("f_on"); pFHp = R ("f_hp"); pFLp = R ("f_lp"); pFRes = R ("f_res"); pF24 = R ("f_24"); pFBoom = R ("f_boom"); pFCrack = R ("f_crack"); pFWarm = R ("f_warm"); pFAir = R ("f_air");
    pTOn = R ("t_on"); pTAge = R ("t_age"); pTWow = R ("t_wow"); pTFlut = R ("t_flut"); pTDrop = R ("t_drop");
    pNOn = R ("n_on"); pNVinyl = R ("n_vinyl"); pNPops = R ("n_pops"); pNHiss = R ("n_hiss"); pNHum = R ("n_hum"); pNFollow = R ("n_follow");
    pROn = R ("r_on"); pRSize = R ("r_size"); pRDecay = R ("r_decay"); pRTone = R ("r_tone"); pRMix = R ("r_mix"); pRGate = R ("r_gate");
    pAmount = R ("amount"); pMix = R ("mix"); pOut = R ("out"); pMono = R ("mono"); pAuto = R ("auto");
}

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
    Settings S; auto b = [] (std::atomic<float>* p) { return p->load() > 0.5f; }; auto v = [] (std::atomic<float>* p) { return p->load(); };
    S.inDb = v (pIn);
    S.sOn = b (pSOn); S.sModel = juce::roundToInt (v (pSModel)); S.sBits = v (pSBits); S.sRate = v (pSRate); S.sMix = v (pSMix);
    S.dOn = b (pDOn); S.dModel = juce::roundToInt (v (pDModel)); S.dDrive = v (pDDrive); S.dTone = v (pDTone); S.dMix = v (pDMix);
    S.cOn = b (pCOn); S.cStyle = juce::roundToInt (v (pCStyle)); S.cAmt = v (pCAmt); S.cAtt = v (pCAtt); S.cRel = v (pCRel); S.cPunch = v (pCPunch); S.cMix = v (pCMix);
    S.fOn = b (pFOn); S.fHp = v (pFHp); S.fLp = v (pFLp); S.fRes = v (pFRes); S.f24 = b (pF24); S.fBoom = v (pFBoom); S.fCrack = v (pFCrack); S.fWarm = v (pFWarm); S.fAir = v (pFAir);
    S.tOn = b (pTOn); S.tAge = v (pTAge); S.tWow = v (pTWow); S.tFlut = v (pTFlut); S.tDrop = v (pTDrop);
    S.nOn = b (pNOn); S.nVinyl = v (pNVinyl); S.nPops = v (pNPops); S.nHiss = v (pNHiss); S.nHum = v (pNHum); S.nFollow = b (pNFollow);
    S.rOn = b (pROn); S.rSize = v (pRSize); S.rDecay = v (pRDecay); S.rTone = v (pRTone); S.rMix = v (pRMix); S.rGate = b (pRGate);
    S.amount = v (pAmount); S.mix = v (pMix); S.outDb = v (pOut); S.mono = b (pMono); S.autoGain = b (pAuto);
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
    for (int off = 0; off < n; off += (int) mono.size())
    {   // a pezzi: niente memoria allocata qui dentro
        const int m = juce::jmin ((int) mono.size(), n - off);
        for (int i = 0; i < m; ++i) mono[(size_t) i] = 0.5f * (L[off + i] + R[off + i]);
        ringOut.push (mono.data(), m);
    }
}
void Processor::loadPreset (int i)
{
    if (! juce::isPositiveAndBelow (i, (int) presets().size())) return;
    preset = i; userName = {};
    if (i >= 1 && i < 1 + kGenresN * kTargetsN) { genre = (i - 1) / kTargetsN; target = (i - 1) % kTargetsN; } else genre = -1;
    for (auto* p : getParameters()) if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p)) if (rp->paramID != "out" && rp->paramID != "in") rp->setValueNotifyingHost (rp->getDefaultValue());
    for (auto& [id, v] : presets()[(size_t) i].v) if (auto* rp = apvts.getParameter (id)) rp->setValueNotifyingHost (rp->convertTo0to1 (v));
}
void Processor::getStateInformation (juce::MemoryBlock& d)
{
    auto s = apvts.copyState(); s.setProperty ("preset", preset, nullptr); s.setProperty ("genre", genre, nullptr); s.setProperty ("target", target, nullptr); s.setProperty ("uname", userName, nullptr);
    if (auto x = s.createXml()) copyXmlToBinary (*x, d);
}
void Processor::setStateInformation (const void* data, int size)
{
    if (auto x = getXmlFromBinary (data, size)) { auto t = juce::ValueTree::fromXml (*x); if (t.isValid()) { preset = t.getProperty ("preset", 0); genre = t.getProperty ("genre", -1); target = t.getProperty ("target", 0); userName = t.getProperty ("uname", "").toString(); apvts.replaceState (t); } }
}
juce::File Processor::presetFolder()
{
    auto f = juce::File::getSpecialLocation (juce::File::userMusicDirectory).getChildFile ("Jackpot").getChildFile ("Dust Preset");
    f.createDirectory(); return f;
}
bool Processor::saveUserPreset (const juce::String& name)
{
    auto s = apvts.copyState(); auto x = s.createXml(); if (! x) return false;
    const auto f = presetFolder().getChildFile (juce::File::createLegalFileName (name) + ".jdust");
    if (! x->writeTo (f)) return false;
    userName = f.getFileNameWithoutExtension(); preset = -1; genre = -1; return true;
}
bool Processor::loadUserPreset (const juce::File& f)
{
    auto x = juce::XmlDocument::parse (f); if (! x) return false;
    auto t = juce::ValueTree::fromXml (*x); if (! t.isValid() || ! t.hasType (apvts.state.getType())) return false;
    apvts.replaceState (t); userName = f.getFileNameWithoutExtension(); preset = -1; genre = -1; return true;
}
juce::AudioProcessorEditor* Processor::createEditor() { return new Editor (*this); }

// ---------------------------------------------------------------- ricette per genere e per parte del kit
// Ogni ricetta e' una catena completa pensata per quel suono; CARATTERE (amount) la dosa tutta insieme.
static const char* kGenres[kGenresN] = { "BOOM BAP", "LO-FI", "DUSTY SOUL", "TRAP", "DRILL", "PHONK", "JAZZ HOP", "TRIP HOP" };
static const char* kTargets[kTargetsN] = { "BUS", "CASSA", "RULLANTE", "HAT", "BREAK" };
const char* genreName (int g) { return kGenres[juce::jlimit (0, kGenresN - 1, g)]; }
const char* targetName (int t) { return kTargets[juce::jlimit (0, kTargetsN - 1, t)]; }
const char* genreIdea (int g)
{
    static const char* d[kGenresN] = {
        "Anni 90 New York: SP-12 a 12 bit, nastro, 670 che ingrossa, crack sul rullante e un velo di vinile. Batteria che bussa.",
        "Morbida e lontana: alti chiusi, nastro che ondeggia, polvere costante, compressione lenta. Niente transienti duri.",
        "Il break anni 70 campionato dal disco: S950, valvola, stanza vera, 670 e tanto vinile. Grosso e caldo.",
        "Moderna e pulita: niente rumori, clipper per il volume, OTT per hat lucidi, aria sopra e sub pieno sotto.",
        "Scura ma definita: clipper, FET con punch, crack e boom tenuti, niente sporco. Kick e snare che tagliano.",
        "Cassetta Memphis: crush, spinta forte, FET schiacciato, nastro rovinato e fruscio.",
        "Kit acustico morbido: nastro, bus lento, stanza piccola e calore. Spazzole e rullante jazz.",
        "Lenta e enorme: pompa, scura, valvola, stanza grande e mono. Il suono di Bristol." };
    return d[juce::jlimit (0, kGenresN - 1, g)];
}
namespace
{
struct Rc
{
    int sM = -1; float sB = 12, sR = 26040, sMix = 1;                     // sampler (-1 spento)
    int dM = -1; float dD = 6, dT = 0, dMix = 1;                          // saturazione
    bool f = false; float hp = 20, lp = 20000, boom = 0, crack = 0, res = 0, warm = 0, air = 0;
    int cS = -1; float cA = 0.4f, cAt = 10, cR = 120, cP = 0, cMix = 1;   // compressore
    bool t = false; float age = 0.3f, wow = 0.1f, flut = 0.1f, drop = 0;
    bool r = false; float rSize = 0.4f, rDec = 0.4f, rTone = 0.5f, rMix = 0.15f; bool rGate = false;
    bool n = false; float vin = 0.3f, pops = 0.15f, hiss = 0.1f, hum = 0;
    bool mono = false;
};
Rc baseRecipe (int g)
{
    Rc R;
    switch (g)
    {
        case 0: R.sM = S_SP12; R.sB = 12; R.sR = 26040; R.sMix = 0.7f; R.dM = D_TAPE; R.dD = 6; R.f = true; R.hp = 30; R.lp = 14000; R.boom = 2; R.crack = 1.5f; R.warm = 0.3f;
                R.cS = C_670; R.cA = 0.45f; R.cAt = 8; R.cR = 150; R.cP = 0.3f; R.t = true; R.age = 0.25f; R.wow = 0.08f; R.flut = 0.12f; R.n = true; R.vin = 0.2f; R.pops = 0.1f; R.hiss = 0.08f; break;
        case 1: R.sM = S_CLEAN; R.sB = 12; R.sR = 22050; R.sMix = 0.5f; R.dM = D_TAPE; R.dD = 5; R.dT = -0.3f; R.f = true; R.hp = 60; R.lp = 6500; R.warm = 0.4f;
                R.cS = C_BUS; R.cA = 0.35f; R.cAt = 20; R.cR = 250; R.t = true; R.age = 0.7f; R.wow = 0.45f; R.flut = 0.35f; R.drop = 0.05f; R.r = true; R.rSize = 0.3f; R.rDec = 0.3f; R.rTone = 0.3f; R.rMix = 0.12f;
                R.n = true; R.vin = 0.5f; R.pops = 0.2f; R.hiss = 0.3f; break;
        case 2: R.sM = S_S950; R.sB = 12; R.sR = 32000; R.dM = D_TUBE; R.dD = 7; R.f = true; R.hp = 40; R.lp = 11000; R.warm = 0.5f; R.crack = 1;
                R.cS = C_670; R.cA = 0.5f; R.cAt = 10; R.cR = 200; R.cP = 0.15f; R.t = true; R.age = 0.4f; R.wow = 0.15f; R.flut = 0.15f;
                R.r = true; R.rSize = 0.45f; R.rDec = 0.45f; R.rTone = 0.45f; R.rMix = 0.18f; R.n = true; R.vin = 0.45f; R.pops = 0.25f; R.hiss = 0.1f; break;
        case 3: R.dM = D_CLIP; R.dD = 4; R.f = true; R.hp = 25; R.boom = 1.5f; R.air = 0.4f; R.cS = C_OTT; R.cA = 0.3f; R.cAt = 10; R.cR = 150; R.cMix = 0.45f; break;
        case 4: R.dM = D_CLIP; R.dD = 5; R.f = true; R.hp = 25; R.boom = 1.5f; R.crack = 1.5f; R.air = 0.25f; R.cS = C_FET; R.cA = 0.4f; R.cAt = 6; R.cR = 80; R.cP = 0.4f; break;
        case 5: R.sM = S_CRUSH; R.sB = 10; R.sR = 18000; R.sMix = 0.6f; R.dM = D_PUSH; R.dD = 10; R.f = true; R.hp = 30; R.lp = 9000; R.boom = 3;
                R.cS = C_FET; R.cA = 0.65f; R.cAt = 3; R.cR = 90; R.cP = 0.2f; R.t = true; R.age = 0.6f; R.wow = 0.3f; R.flut = 0.25f; R.drop = 0.12f; R.n = true; R.vin = 0.1f; R.pops = 0.05f; R.hiss = 0.5f; break;
        case 6: R.dM = D_TAPE; R.dD = 4; R.f = true; R.hp = 50; R.lp = 10000; R.warm = 0.5f; R.cS = C_BUS; R.cA = 0.3f; R.cAt = 20; R.cR = 250;
                R.t = true; R.age = 0.4f; R.wow = 0.25f; R.flut = 0.2f; R.r = true; R.rSize = 0.5f; R.rDec = 0.5f; R.rTone = 0.4f; R.rMix = 0.15f; R.n = true; R.vin = 0.5f; R.pops = 0.2f; R.hiss = 0.15f; break;
        default: R.sM = S_S950; R.sB = 12; R.sR = 24000; R.dM = D_TUBE; R.dD = 10; R.dT = -0.5f; R.f = true; R.hp = 35; R.lp = 7000; R.boom = 4; R.res = 0.15f;
                R.cS = C_PUMP; R.cA = 0.6f; R.cAt = 25; R.cR = 300; R.t = true; R.age = 0.6f; R.wow = 0.15f; R.flut = 0.15f; R.r = true; R.rSize = 0.8f; R.rDec = 0.7f; R.rTone = 0.35f; R.rMix = 0.2f;
                R.n = true; R.vin = 0.4f; R.pops = 0.2f; R.hiss = 0.25f; R.mono = true; break;
    }
    return R;
}
Rc recipe (int g, int t)
{
    Rc R = baseRecipe (g);
    const bool modern = g == 3 || g == 4;
    switch (t)
    {
        case 1:   // cassa: niente polvere ne' stanza, piu' peso e punch, alti un po' piu' chiusi
            R.n = false; if (g != 7) R.r = false; R.f = true; R.boom += 2; R.hp = 25; R.lp = std::min (R.lp * 0.75f, 12000.0f); R.cP += 0.2f; R.sMix *= 0.8f;
            if (modern) { R.dD += 2; R.boom += 1; R.cS = C_FET; R.cA = 0.35f; R.cAt = 12; R.cR = 90; R.cMix = 1; R.air = 0; }
            break;
        case 2:   // rullante: niente sub, crack, un po' di stanza (gate sulle moderne)
            R.f = true; R.hp = modern ? 110 : 140; R.crack += 2; R.boom = 0; R.n = R.n && g != 5; R.vin *= 0.6f;
            R.r = true; R.rSize = modern ? 0.35f : std::max (R.rSize, 0.35f); R.rDec = modern ? 0.35f : std::max (R.rDec, 0.4f); R.rMix = std::max (R.rMix * 1.4f, 0.15f); R.rGate = modern;
            R.cAt = std::max (1.0f, R.cAt * 0.6f);
            if (modern) { R.cS = C_FET; R.cA = 0.4f; R.cP = 0.35f; R.cMix = 1; }
            break;
        case 3:   // hat: solo la cima, niente boom, compressione leggera (OTT sulle moderne)
            R.f = true; R.hp = modern ? 300 : 350; R.boom = 0; R.crack = 0; R.lp = std::min (20000.0f, R.lp * 1.25f); R.r = false; R.n = R.n && g != 5; R.vin *= 0.5f;
            R.age *= 0.6f; R.wow *= 0.6f;
            if (modern) { R.cS = C_OTT; R.cA = 0.35f; R.cMix = 0.5f; R.air += 0.2f; R.dD = 2; }
            else { R.cS = C_BUS; R.cA = 0.25f; R.cAt = 5; R.cR = 120; }
            break;
        case 4:   // break: il loop intero, piu' macchina, piu' disco
            if (R.sM >= 0) R.sMix = 1;
            if (! modern) { R.t = true; R.age += 0.1f; R.vin = std::min (1.0f, R.vin * 1.3f + 0.05f); R.pops = std::min (1.0f, R.pops * 1.3f); R.n = true; }
            else { R.cS = C_FET; R.cA = 0.45f; R.cP = 0.35f; R.cMix = 1; }
            break;
        default: break;   // bus
    }
    return R;
}
using PV = std::vector<std::pair<const char*, float>>;
PV toPV (const Rc& R)
{
    PV v;
    if (R.sM >= 0) v.insert (v.end(), { { "s_on", 1 }, { "s_model", (float) R.sM }, { "s_bits", R.sB }, { "s_rate", R.sR }, { "s_mix", R.sMix } });
    if (R.dM >= 0) v.insert (v.end(), { { "d_on", 1 }, { "d_model", (float) R.dM }, { "d_drive", R.dD }, { "d_tone", R.dT }, { "d_mix", R.dMix } });
    if (R.f) v.insert (v.end(), { { "f_on", 1 }, { "f_hp", R.hp }, { "f_lp", R.lp }, { "f_boom", R.boom }, { "f_crack", R.crack }, { "f_res", R.res }, { "f_warm", R.warm }, { "f_air", R.air } });
    if (R.cS >= 0) v.insert (v.end(), { { "c_on", 1 }, { "c_style", (float) R.cS }, { "c_amt", R.cA }, { "c_att", R.cAt }, { "c_rel", R.cR }, { "c_punch", juce::jlimit (-1.0f, 1.0f, R.cP) }, { "c_mix", R.cMix } });
    if (R.t) v.insert (v.end(), { { "t_on", 1 }, { "t_age", R.age }, { "t_wow", R.wow }, { "t_flut", R.flut }, { "t_drop", R.drop } });
    if (R.r) v.insert (v.end(), { { "r_on", 1 }, { "r_size", R.rSize }, { "r_decay", R.rDec }, { "r_tone", R.rTone }, { "r_mix", R.rMix }, { "r_gate", R.rGate ? 1.0f : 0.0f } });
    if (R.n) v.insert (v.end(), { { "n_on", 1 }, { "n_vinyl", R.vin }, { "n_pops", R.pops }, { "n_hiss", R.hiss }, { "n_hum", R.hum }, { "n_follow", 1 } });
    if (R.mono) v.push_back ({ "mono", 1 });
    return v;
}
juce::String targetNote (int g, int t)
{
    const bool modern = g == 3 || g == 4;
    switch (t)
    {
        case 1: return modern ? "Cassa: clipper piu' spinto e FET con punch, boom a 70 Hz. Senza rumori." : "Cassa: niente polvere ne' stanza, piu' boom e punch, alti un filo chiusi per farla bussare.";
        case 2: return modern ? "Rullante: via il sub, crack a 2.6 kHz, stanza corta col gate stile anni 80." : "Rullante: via il sub, crack, un po' di stanza che lo fa sembrare suonato in una sala vera.";
        case 3: return modern ? "Hat: solo la cima, OTT a meta' MIX e aria: lucidi senza essere duri." : "Hat: solo la cima, compressione leggera, meno polvere: presenti ma morbidi.";
        case 4: return modern ? "Break: tutto il loop con FET e punch, pulito." : "Break: il loop intero con la macchina al 100%, piu' nastro e piu' disco. Pronto da choppare.";
        default: return "Bus della batteria: la catena intera sul gruppo.";
    }
}
}
int recipeIndex (int g, int t) { return 1 + juce::jlimit (0, kGenresN - 1, g) * kTargetsN + juce::jlimit (0, kTargetsN - 1, t); }

const std::vector<Preset>& presets()
{
    static const std::vector<Preset> p = []
    {
        std::vector<Preset> v;
        v.push_back ({ "Init", "Tutto spento: scegli un genere e una parte del kit, oppure accendi i moduli che ti servono.", {} });
        for (int g = 0; g < kGenresN; ++g)
            for (int t = 0; t < kTargetsN; ++t)
            {
                static std::vector<juce::String> keep; keep.reserve (128);   // i testi devono restare vivi
                keep.push_back (juce::String (genreName (g)) + " | " + targetName (t));
                keep.push_back (juce::String (genreIdea (g)) + " " + targetNote (g, t));
                v.push_back ({ keep[keep.size() - 2].toRawUTF8(), keep.back().toRawUTF8(), toPV (recipe (g, t)) });
            }
        auto add = [&v] (const char* n, const char* d, PV pv) { v.push_back ({ n, d, std::move (pv) }); };
        add ("EFFETTI | Radio AM", "Batteria da radio: stretta, sporca, ronzio.",
             { { "s_on", 1 }, { "s_model", S_CRUSH }, { "s_bits", 8 }, { "s_rate", 8000 }, { "s_mix", 0.8f }, { "d_on", 1 }, { "d_model", D_PUSH }, { "d_drive", 10 },
               { "f_on", 1 }, { "f_hp", 400 }, { "f_lp", 3500 }, { "f_res", 0.3f }, { "f_24", 1 }, { "n_on", 1 }, { "n_vinyl", 0 }, { "n_pops", 0 }, { "n_hiss", 0.5f }, { "n_hum", 0.3f }, { "mono", 1 }, { "auto", 1 } });
        add ("EFFETTI | Vinile 78 giri", "Gommalacca: banda stretta, tanto crackle.",
             { { "s_on", 1 }, { "s_model", S_CLEAN }, { "s_bits", 10 }, { "s_rate", 16000 }, { "f_on", 1 }, { "f_hp", 250 }, { "f_lp", 5000 }, { "t_on", 1 }, { "t_age", 0.3f }, { "t_wow", 0.3f }, { "t_flut", 0.2f },
               { "n_on", 1 }, { "n_vinyl", 0.9f }, { "n_pops", 0.7f }, { "n_hiss", 0.4f }, { "mono", 1 }, { "auto", 1 } });
        add ("EFFETTI | Nastro che muore", "Wow, flutter e dropout al massimo.",
             { { "t_on", 1 }, { "t_age", 1.0f }, { "t_wow", 0.9f }, { "t_flut", 0.7f }, { "t_drop", 0.7f }, { "n_on", 1 }, { "n_vinyl", 0 }, { "n_pops", 0 }, { "n_hiss", 0.6f }, { "amount", 1.2f } });
        add ("EFFETTI | Solo vinile", "Aggiunge solo la polvere del disco, sempre accesa.", { { "n_on", 1 }, { "n_vinyl", 0.5f }, { "n_pops", 0.3f }, { "n_hiss", 0.15f }, { "n_follow", 0 } });
        add ("EFFETTI | Stanza gated anni 80", "Solo la stanza col gate: il rullante enorme che si chiude di colpo.", { { "r_on", 1 }, { "r_size", 0.7f }, { "r_decay", 0.7f }, { "r_tone", 0.6f }, { "r_mix", 0.45f }, { "r_gate", 1 } });
        add ("EFFETTI | Filtro telefono", "Hat e loop filtrati stretti, 8 bit.", { { "s_on", 1 }, { "s_model", S_CRUSH }, { "s_bits", 8 }, { "s_rate", 11025 }, { "s_mix", 0.7f }, { "f_on", 1 }, { "f_hp", 900 }, { "f_lp", 4000 }, { "f_res", 0.3f } });
        return v;
    }();
    return p;
}
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new jds::Processor(); }
