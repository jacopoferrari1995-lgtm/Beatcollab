#include "MasterProcessor.h"
#include "MasterEditor.h"

namespace jms
{
juce::AudioProcessorValueTreeState::ParameterLayout Processor::layout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    StringArray st; for (int s = 0; s < kNumStyles; ++s) st.add (styleInfo (s).name);
    StringArray tg; for (int t = 0; t < kNumTargets; ++t) tg.add (String (target (t).name) + " " + String ((int) target (t).lufs));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "trim", 1 }, "Input Trim", NormalisableRange<float> (-12, 12, 0.1f), 0, AudioParameterFloatAttributes().withLabel ("dB")));
    // maximizer
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "mx_on", 1 }, "Maximizer On", true));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "mx_delta", 1 }, "Maximizer Delta", false));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "mx_gain", 1 }, "Maximizer Gain", NormalisableRange<float> (0, 20, 0.01f), 4, AudioParameterFloatAttributes().withLabel ("dB")));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "mx_style", 1 }, "Maximizer Style", st, 0));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "mx_clip", 1 }, "Maximizer Clip", NormalisableRange<float> (0, 1), 0.2f));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "mx_char", 1 }, "Maximizer Character", NormalisableRange<float> (0, 1), 0.5f));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "mx_link", 1 }, "Maximizer Stereo Link", NormalisableRange<float> (0, 1), 1));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "mx_learn", 1 }, "Maximizer Auto Target", false));
    // limiter
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "lm_on", 1 }, "Limiter On", true));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "lm_delta", 1 }, "Limiter Delta", false));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "lm_ceil", 1 }, "Ceiling", NormalisableRange<float> (-6, 0, 0.1f), -1, AudioParameterFloatAttributes().withLabel ("dBTP")));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "lm_rel", 1 }, "Limiter Release", NormalisableRange<float> (5, 1000, 0, 0.4f), 80, AudioParameterFloatAttributes().withLabel ("ms")));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "lm_auto", 1 }, "Limiter Auto Release", true));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "lm_tp", 1 }, "True Peak", true));
    // uscita e ascolto
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "target", 1 }, "Target", tg, 0));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "norm", 1 }, "Normalized Listen", false));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "compare", 1 }, "Compare Bypass", false));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "dither", 1 }, "Dither", StringArray { "OFF", "24 BIT", "16 BIT", "16 BIT SHAPED" }, 0));
    return l;
}

Processor::Processor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true).withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "JMS", layout())
{
    for (int i = 0; i < kHist; ++i) { hIn[i] = -60; hOut[i] = -60; hGrM[i] = 0; hGrL[i] = 0; hS[i] = -60; }
    startTimerHz (5);
}

bool Processor::isBusesLayoutSupported (const BusesLayout& l) const
{
    return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo() && l.getMainInputChannelSet() == l.getMainOutputChannelSet();
}

void Processor::prepareToPlay (double s, int maxBlock)
{
    sr = s;
    const int mb = juce::jmax (64, maxBlock);
    mx.prepare (s, mb); lim.prepare (s);
    for (auto& d : mxBy) d.prepare (mx.latency());
    for (auto& d : limBy) d.prepare (lim.latency());
    for (auto& d : mxIn) d.prepare (mx.latency());
    for (auto& d : deltaPad) d.prepare (lim.latency());
    const int total = mx.latency() + lim.latency();
    for (auto& d : dry) d.prepare (total);
    setLatencySamples (total);
    loudIn.prepare (s); loudOut.prepare (s); dith.reset(); for (auto& t : tpOut) t.reset();
    tmp.setSize (2, mb); keep.setSize (2, mb);
    matchSm = normSm = 1; tpHold = -100;
}

void Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals nd;
    const int n = buffer.getNumSamples(); if (n == 0) return;
    if (tmp.getNumSamples() < n) { tmp.setSize (2, n, false, false, true); keep.setSize (2, n, false, false, true); }
    float* L = buffer.getWritePointer (0); float* R = buffer.getWritePointer (1);
    if (resetReq.exchange (false)) { loudOut.resetIntegrated(); tpHold = -100; }

    const float trim = dbToG (value ("trim"));
    float pi[2] = { 0, 0 };
    for (int i = 0; i < n; ++i)
    {
        L[i] *= trim; R[i] *= trim;
        pi[0] = juce::jmax (pi[0], std::abs (L[i])); pi[1] = juce::jmax (pi[1], std::abs (R[i]));
        loudIn.push (L[i], R[i]);
    }
    // copia pulita (per il confronto a volume uguale)
    float* kL = keep.getWritePointer (0); float* kR = keep.getWritePointer (1);
    for (int i = 0; i < n; ++i) { kL[i] = dry[0] (L[i]); kR[i] = dry[1] (R[i]); }

    // ---- MAXIMIZER
    MaxSettings ms; ms.gainDb = value ("mx_gain"); ms.style = (int) value ("mx_style"); ms.clip = value ("mx_clip"); ms.character = value ("mx_char");
    ms.link = value ("mx_link"); ms.ceilDb = value ("lm_ceil");
    const bool mxOn = value ("mx_on") > 0.5f, mxDelta = mxOn && value ("mx_delta") > 0.5f;
    float* tL = tmp.getWritePointer (0); float* tR = tmp.getWritePointer (1);
    for (int i = 0; i < n; ++i) { tL[i] = L[i]; tR[i] = R[i]; }
    if (mxOn) mx.process (L, R, n, ms);
    else { for (int i = 0; i < n; ++i) { L[i] = mxBy[0] (L[i]); R[i] = mxBy[1] (R[i]); } mx.gr = 0; }
    if (mxDelta)
    {   // ascolti solo cosa toglie il maximizer (a parita di spinta)
        const float g = dbToG (ms.gainDb);
        for (int i = 0; i < n; ++i) { L[i] = L[i] - mxIn[0] (tL[i]) * g; R[i] = R[i] - mxIn[1] (tR[i]) * g; }
    }
    else for (int i = 0; i < n; ++i) { mxIn[0] (tL[i]); mxIn[1] (tR[i]); }

    // ---- LIMITER
    LimSettings ls; ls.ceilDb = ms.ceilDb; ls.relMs = value ("lm_rel"); ls.autoRel = value ("lm_auto") > 0.5f; ls.truePeak = value ("lm_tp") > 0.5f;
    const bool lmOn = value ("lm_on") > 0.5f, lmDelta = lmOn && value ("lm_delta") > 0.5f;
    if (mxDelta) { for (int i = 0; i < n; ++i) { L[i] = deltaPad[0] (L[i]); R[i] = deltaPad[1] (R[i]); } lim.gr = 0; }
    else
    {
        for (int i = 0; i < n; ++i) { tL[i] = L[i]; tR[i] = R[i]; }
        if (lmOn) lim.process (L, R, n, ls);
        else { for (int i = 0; i < n; ++i) { L[i] = limBy[0] (L[i]); R[i] = limBy[1] (R[i]); } lim.gr = 0; }
        if (lmDelta) for (int i = 0; i < n; ++i) { L[i] = L[i] - deltaPad[0] (tL[i]); R[i] = R[i] - deltaPad[1] (tR[i]); }
        else for (int i = 0; i < n; ++i) { deltaPad[0] (tL[i]); deltaPad[1] (tR[i]); }
    }

    // ---- misure d'uscita (prima di confronto e ascolto normalizzato)
    float po[2] = { 0, 0 }; float tpb = 0;
    for (int i = 0; i < n; ++i)
    {
        loudOut.push (L[i], R[i]);
        po[0] = juce::jmax (po[0], std::abs (L[i])); po[1] = juce::jmax (po[1], std::abs (R[i]));
        tpb = juce::jmax (tpb, juce::jmax (tpOut[0].push (L[i]), tpOut[1].push (R[i])));
    }
    tpHold = juce::jmax (tpHold, gToDb (tpb));

    // ---- confronto a volume uguale: il pulito alzato/abbassato alla loudness del master
    const bool compare = value ("compare") > 0.5f;
    float mdb = 0; if (loudIn.S > -60 && loudOut.S > -60) mdb = juce::jlimit (-24.0f, 24.0f, loudOut.S - loudIn.S);
    matchDb = mdb;
    if (compare)
    {
        const float tgt = dbToG (mdb), step = (tgt - matchSm) / (float) n;
        for (int i = 0; i < n; ++i) { matchSm += step; L[i] = kL[i] * matchSm; R[i] = kR[i] * matchSm; }
    }
    else matchSm = dbToG (mdb);

    // ---- ascolto normalizzato (come lo abbassera la piattaforma). Solo per ascoltare!
    const auto& tg = target ((int) value ("target"));
    const float lufsRef = loudOut.I > -60 ? loudOut.I : loudOut.S;
    const float ndb = (lufsRef > -60 && lufsRef > tg.lufs) ? tg.lufs - lufsRef : 0.0f;
    normDb = ndb;
    if (value ("norm") > 0.5f)
    {
        const float tgt = dbToG (ndb), step = (tgt - normSm) / (float) n;
        for (int i = 0; i < n; ++i) { normSm += step; L[i] *= normSm; R[i] *= normSm; }
    }
    else normSm = dbToG (ndb);

    // ---- dither (sempre l'ultimo)
    dith.process (L, R, n, (int) value ("dither"));

    // ---- misure
    lufsM = loudOut.M; lufsS = loudOut.S; lufsI = loudOut.I; lra = loudOut.lra; tpMax = tpHold; inS = loudIn.S;
    grMax = mx.gr; grLim = lim.gr;
    for (int c = 0; c < 2; ++c) { inPk[c] = juce::jmax (pi[c], inPk[c].load() * 0.9f); outPk[c] = juce::jmax (po[c], outPk[c].load() * 0.9f); }
    colIn = juce::jmax (colIn, juce::jmax (pi[0], pi[1])); colOut = juce::jmax (colOut, juce::jmax (po[0], po[1]));
    colM = juce::jmax (colM, mx.gr); colL = juce::jmax (colL, lim.gr); colN += n;
    if (colN >= (int) (sr * 0.025))
    {
        const int w = hW.load();
        hIn[w] = gToDb (colIn); hOut[w] = gToDb (colOut); hGrM[w] = colM; hGrL[w] = colL; hS[w] = loudOut.S;
        hW = (w + 1) % kHist; colIn = colOut = colM = colL = 0; colN = 0;
    }
}

// porta il maximizer al target (sul thread dei messaggi)
void Processor::timerCallback()
{
    if (value ("mx_learn") < 0.5f || value ("mx_on") < 0.5f) return;
    const float s = lufsS.load(); if (s < -40) return;
    const float diff = target ((int) value ("target")).lufs - s;
    auto* p = apvts.getParameter ("mx_gain");
    const float cur = value ("mx_gain"), step = juce::jlimit (-0.4f, 0.4f, diff * 0.2f);
    if (std::abs (diff) > 0.15f) p->setValueNotifyingHost (p->convertTo0to1 (juce::jlimit (0.0f, 20.0f, cur + step)));
}

// ======================================================================= preset
const std::vector<Preset>& presets()
{
    static const std::vector<Preset> P = {
        { "Init", "Maximizer pulito +4 dB, limiter a -1 dBTP.", {} },
        { "Streaming | Pulito -14", "Per Spotify e YouTube: trasparente, niente schiacciamento inutile.", { { "mx_gain", 3 }, { "mx_style", PULITO }, { "mx_clip", 0.1f }, { "target", 0 } } },
        { "Streaming | Apple -16", "Ancora piu dinamico per Apple Music.", { { "mx_gain", 1.5f }, { "mx_style", PULITO }, { "mx_clip", 0.0f }, { "target", 1 } } },
        { "Trap | Forte e pulito", "Volume da trap senza perdere il colpo della cassa.", { { "mx_gain", 8 }, { "mx_style", PUNCH }, { "mx_clip", 0.45f }, { "mx_char", 0.55f }, { "lm_ceil", -0.5f }, { "target", 5 } } },
        { "Trap | Muro", "Clipper e rilascio rapido: massimo volume, grinta voluta.", { { "mx_gain", 11 }, { "mx_style", AGGRESSIVO }, { "mx_clip", 0.7f }, { "mx_char", 0.7f }, { "lm_ceil", -0.3f }, { "target", 5 } } },
        { "Drill | Punch", "Rullante e cassa in faccia, 808 fermo.", { { "mx_gain", 9 }, { "mx_style", PUNCH }, { "mx_clip", 0.55f }, { "mx_char", 0.6f }, { "lm_ceil", -0.5f }, { "target", 4 } } },
        { "Boom Bap | Morbido", "Saturazione leggera, rilascio lento: caldo e rotondo.", { { "mx_gain", 6 }, { "mx_style", MORBIDO }, { "mx_clip", 0.3f }, { "mx_char", 0.35f }, { "target", 3 } } },
        { "R&B | Arioso", "Volume medio, dinamica preservata, molto trasparente.", { { "mx_gain", 4 }, { "mx_style", PULITO }, { "mx_clip", 0.15f }, { "mx_char", 0.3f }, { "target", 0 } } },
        { "Lo-fi | Vintage", "Morbido e scuro, niente volume eccessivo.", { { "mx_gain", 4 }, { "mx_style", MORBIDO }, { "mx_clip", 0.4f }, { "mx_char", 0.25f }, { "target", 0 } } },
        { "Club | Impatto", "Per la cassa: forte, con transienti vivi e ceiling quasi a zero.", { { "mx_gain", 9 }, { "mx_style", PUNCH }, { "mx_clip", 0.5f }, { "lm_ceil", -0.3f }, { "target", 4 } } } };
    return P;
}

void Processor::loadPreset (int i)
{
    if (! juce::isPositiveAndBelow (i, (int) presets().size())) return;
    preset = i;
    for (auto* p : getParameters()) if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p)) if (rp->paramID != "trim" && rp->paramID != "dither") rp->setValueNotifyingHost (rp->getDefaultValue());
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
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new jms::Processor(); }
