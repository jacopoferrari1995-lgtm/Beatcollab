#include "CompProcessor.h"
#include "CompEditor.h"

namespace jc
{
static juce::StringArray modelNames() { juce::StringArray a; for (int m = 0; m < kNumModels; ++m) a.add (info (m).name); return a; }

juce::AudioProcessorValueTreeState::ParameterLayout Processor::layout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    for (auto s : { "a", "b" })
    {
        const String p (s), P = p.toUpperCase() + " ";
        l.add (std::make_unique<AudioParameterBool> (ParameterID { p + "_on", 1 }, P + "On", p == "a"));
        l.add (std::make_unique<AudioParameterChoice> (ParameterID { p + "_model", 1 }, P + "Model", modelNames(), p == "a" ? FET76 : OPTO2A));
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { p + "_drive", 1 }, P + "Input/Thresh", NormalisableRange<float> (0, 1), 0.3f));
        l.add (std::make_unique<AudioParameterInt> (ParameterID { p + "_ratio", 1 }, P + "Ratio", 0, 7, 0));
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { p + "_att", 1 }, P + "Attack", NormalisableRange<float> (0, 1), 0.5f));
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { p + "_rel", 1 }, P + "Release", NormalisableRange<float> (0, 1), 0.35f));
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { p + "_out", 1 }, P + "Output", NormalisableRange<float> (-12, 24), 0));
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { p + "_color", 1 }, P + "Color", NormalisableRange<float> (0, 1), 0.3f));
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { p + "_mix", 1 }, P + "Mix", NormalisableRange<float> (0, 1), 1));
    }
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "route", 1 }, "Routing", StringArray { "SINGOLO", "SERIE", "PARALLELO" }, 0));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "scSrc", 1 }, "Sidechain", StringArray { "INTERNO", "ESTERNO" }, 0));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "scTo", 1 }, "Sidechain su", StringArray { "A + B", "SOLO A", "SOLO B" }, 0));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "scHpf", 1 }, "SC HPF", NormalisableRange<float> (20, 500, 0, 0.4f), 20));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "scListen", 1 }, "SC Listen", false));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "auto", 1 }, "Auto Gain", false));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "mix", 1 }, "Mix", NormalisableRange<float> (0, 1), 1));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "out", 1 }, "Output", NormalisableRange<float> (-12, 12), 0));
    return l;
}

Processor::Processor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                                       .withInput ("Sidechain", juce::AudioChannelSet::stereo(), false)),
      apvts (*this, nullptr, "JSC", layout())
{
    for (int i = 0; i < kHist; ++i) { hIn[i] = -60; hOut[i] = -60; hGrA[i] = 0; hGrB[i] = 0; }
}

bool Processor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto in = l.getMainInputChannelSet(), out = l.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::stereo() && out != juce::AudioChannelSet::mono()) return false;
    if (in != out) return false;
    if (l.inputBuses.size() > 1)
    {
        const auto sc = l.getChannelSet (true, 1);
        if (! sc.isDisabled() && sc != juce::AudioChannelSet::stereo() && sc != juce::AudioChannelSet::mono()) return false;
    }
    return true;
}

void Processor::prepareToPlay (double s, int maxBlock)
{
    sr = s;
    for (auto& x : st) x.prepare (s);
    dry.setSize (2, maxBlock); det.setSize (4, maxBlock); aux.setSize (2, maxBlock);
    for (auto& a : hp) for (auto& f : a) f.reset();
    lastHp = -1;
}

StageParams Processor::stageParams (int s) const
{
    const juce::String p (s == 0 ? "a" : "b");
    StageParams q;
    q.on = value ((p + "_on").toRawUTF8()) > 0.5f;
    q.model = (int) value ((p + "_model").toRawUTF8());
    q.drive = value ((p + "_drive").toRawUTF8());
    q.ratio = juce::jlimit (0, info (q.model).ratios.size() - 1, (int) value ((p + "_ratio").toRawUTF8()));
    q.att = value ((p + "_att").toRawUTF8()); q.rel = value ((p + "_rel").toRawUTF8());
    q.outDb = value ((p + "_out").toRawUTF8()); q.color = value ((p + "_color").toRawUTF8()); q.mix = value ((p + "_mix").toRawUTF8());
    return q;
}

void Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals nd;
    auto main = getBusBuffer (buffer, true, 0);
    const int N = buffer.getNumSamples(), nch = juce::jmin (2, main.getNumChannels());
    if (N > dry.getNumSamples()) { dry.setSize (2, N, false, false, true); det.setSize (4, N, false, false, true); aux.setSize (2, N, false, false, true); }
    float* L = main.getWritePointer (0); float* R = nch > 1 ? main.getWritePointer (1) : main.getWritePointer (0);

    // sidechain esterno
    const float* eL = nullptr; const float* eR = nullptr;
    if (getBusCount (true) > 1 && getBus (true, 1)->isEnabled())
    {
        auto sc = getBusBuffer (buffer, true, 1);
        if (sc.getNumChannels() > 0) { eL = sc.getReadPointer (0); eR = sc.getReadPointer (juce::jmin (1, sc.getNumChannels() - 1)); }
    }
    extSc = eL != nullptr;
    const bool useExt = value ("scSrc") > 0.5f && eL != nullptr;
    const int scTo = (int) value ("scTo");

    // misura ingresso
    float pkI[2] = { 0, 0 }; double eIn = 0;
    for (int i = 0; i < N; ++i) { pkI[0] = juce::jmax (pkI[0], std::abs (L[i])); pkI[1] = juce::jmax (pkI[1], std::abs (R[i])); eIn += 0.5 * (L[i] * L[i] + R[i] * R[i]); }
    dry.copyFrom (0, 0, L, N); dry.copyFrom (1, 0, R, N);

    // filtro passa alto del sidechain
    const float hpF = value ("scHpf");
    if (std::abs (hpF - lastHp) > 0.5f)
    {
        auto c = juce::dsp::IIR::Coefficients<float>::makeHighPass (sr, juce::jmax (20.0f, hpF), 0.707f);
        for (auto& a : hp) for (auto& f : a) f.coefficients = c;
        lastHp = hpF;
    }
    auto makeDet = [&] (int s, const float* sl, const float* sr2)
    {
        const bool ext = useExt && (scTo == 0 || scTo == s + 1);
        float* dl = det.getWritePointer (s * 2); float* dr = det.getWritePointer (s * 2 + 1);
        juce::FloatVectorOperations::copy (dl, ext ? eL : sl, N); juce::FloatVectorOperations::copy (dr, ext ? eR : sr2, N);
        if (hpF > 21) for (int i = 0; i < N; ++i) { dl[i] = hp[s][0].processSample (dl[i]); dr[i] = hp[s][1].processSample (dr[i]); }
    };

    const int route = (int) value ("route"); const bool autoG = value ("auto") > 0.5f;
    StageParams pa = stageParams (0), pb = stageParams (1);
    if (route == 0) pb.on = false;
    float* xL = aux.getWritePointer (0); float* xR = aux.getWritePointer (1);
    if (route == 2) { juce::FloatVectorOperations::copy (xL, L, N); juce::FloatVectorOperations::copy (xR, R, N); }

    if (pa.on) { makeDet (0, L, R); st[0].process (L, R, det.getReadPointer (0), det.getReadPointer (1), N, pa, autoG); } else st[0].reset();
    if (pb.on)
    {
        if (route == 2) { makeDet (1, xL, xR); st[1].process (xL, xR, det.getReadPointer (2), det.getReadPointer (3), N, pb, autoG); }
        else { makeDet (1, L, R); st[1].process (L, R, det.getReadPointer (2), det.getReadPointer (3), N, pb, autoG); }
    }
    else st[1].reset();
    if (route == 2)
    {
        if (pa.on && pb.on) for (int i = 0; i < N; ++i) { L[i] = 0.5f * (L[i] + xL[i]); R[i] = 0.5f * (R[i] + xR[i]); }
        else if (pb.on) { juce::FloatVectorOperations::copy (L, xL, N); juce::FloatVectorOperations::copy (R, xR, N); }
    }

    // ascolto del sidechain
    if (value ("scListen") > 0.5f)
    {
        if (! pa.on && ! pb.on) makeDet (0, dry.getReadPointer (0), dry.getReadPointer (1));
        const int s = pa.on || ! pb.on ? 0 : 1;
        juce::FloatVectorOperations::copy (L, det.getReadPointer (s * 2), N); juce::FloatVectorOperations::copy (R, det.getReadPointer (s * 2 + 1), N);
    }
    else
    {
        const float mix = value ("mix"), og = juce::Decibels::decibelsToGain (value ("out"));
        for (int i = 0; i < N; ++i) { L[i] = (dry.getSample (0, i) * (1 - mix) + L[i] * mix) * og; R[i] = (dry.getSample (1, i) * (1 - mix) + R[i] * mix) * og; }
    }
    if (nch == 1) {} // mono: gia scritto nel canale 0

    // misure
    float pkO[2] = { 0, 0 }; double eOut = 0;
    for (int i = 0; i < N; ++i) { pkO[0] = juce::jmax (pkO[0], std::abs (L[i])); pkO[1] = juce::jmax (pkO[1], std::abs (R[i])); eOut += 0.5 * (L[i] * L[i] + R[i] * R[i]); }
    for (int c = 0; c < 2; ++c) { inPk[c] = juce::jmax (pkI[c], inPk[c].load() * 0.92f); outPk[c] = juce::jmax (pkO[c], outPk[c].load() * 0.92f); }
    const float k = std::exp (-(float) N / (0.3f * (float) sr));
    rmsIn = k * rmsIn + (1 - k) * (float) (eIn / N); rmsOut = k * rmsOut + (1 - k) * (float) (eOut / N);
    inRms = rmsIn; outRms = rmsOut;
    grA = pa.on ? st[0].grNow() : 0.0f; grB = pb.on ? st[1].grNow() : 0.0f;
    avgGrA = pa.on ? st[0].grAvg : 0.0f; avgGrB = pb.on ? st[1].grAvg : 0.0f;
    colIn = juce::jmax (colIn, juce::jmax (pkI[0], pkI[1])); colOut = juce::jmax (colOut, juce::jmax (pkO[0], pkO[1]));
    colA = juce::jmax (colA, grA.load()); colB = juce::jmax (colB, grB.load()); colN += N;
    if (colN >= (int) (sr * 4.0 / kHist))
    {
        const int w = hW.load();
        hIn[w] = juce::Decibels::gainToDecibels (colIn, -60.0f); hOut[w] = juce::Decibels::gainToDecibels (colOut, -60.0f); hGrA[w] = colA; hGrB[w] = colB;
        hW = (w + 1) % kHist; colIn = colOut = colA = colB = 0; colN = 0;
    }
    for (int c = 2; c < buffer.getNumChannels(); ++c) buffer.clear (c, 0, N);
}

// ======================= preset =======================
const std::vector<Preset>& presets()
{
    static const std::vector<Preset> P = {
        { "Init", "Punto di partenza: FET 76 sullo stadio A.", 0, {} },
        { "Voce | Rap davanti (76 > 2A)", "Il FET prende i picchi (3-5 dB), l'OPTO livella (3-4 dB). Voce sempre davanti senza sentirsi compressa.", 1,
          { { "a_model", FET76 }, { "a_drive", 0.3f }, { "a_ratio", 0 }, { "a_att", 0.55f }, { "a_rel", 0.3f }, { "a_color", 0.35f },
            { "b_on", 1 }, { "b_model", OPTO2A }, { "b_drive", 0.35f }, { "b_ratio", 0 }, { "b_color", 0.3f }, { "auto", 1 } } },
        { "Voce | Morbida 2A", "Solo ottico, livellamento trasparente per voci cantate e ritornelli.", 0,
          { { "a_model", OPTO2A }, { "a_drive", 0.45f }, { "a_ratio", 0 }, { "a_color", 0.4f }, { "auto", 1 } } },
        { "Voce | Aggressiva Distress", "Distress con distorsione di 3a armonica: voce che graffia e sta sopra il beat.", 0,
          { { "a_model", DISTRESS }, { "a_drive", 0.38f }, { "a_ratio", 3 }, { "a_att", 0.3f }, { "a_rel", 0.35f }, { "a_color", 0.7f }, { "auto", 1 } } },
        { "Voce | Adlib schiacciati", "FET in ALL: adlib piatti e distorti, da mettere sotto la voce principale.", 0,
          { { "a_model", FET76 }, { "a_drive", 0.6f }, { "a_ratio", 4 }, { "a_att", 0.7f }, { "a_rel", 0.2f }, { "a_color", 0.5f }, { "auto", 1 } } },
        { "Batteria | Drum bus colla", "VCA da bus: attacco 30 ms per i transienti, rilascio AUTO. 2-4 dB di riduzione.", 0,
          { { "a_model", VCABUS }, { "a_drive", 0.35f }, { "a_ratio", 1 }, { "a_att", 1.0f }, { "a_rel", 1.0f }, { "a_color", 0.2f }, { "auto", 1 } } },
        { "Batteria | Parallelo New York", "Stadio A schiacciato, MIX al 35%: il dry tiene i transienti, il compresso da corpo e coda.", 0,
          { { "a_model", DISTRESS }, { "a_drive", 0.6f }, { "a_ratio", 6 }, { "a_att", 0.25f }, { "a_rel", 0.3f }, { "a_color", 0.55f }, { "a_mix", 0.35f }, { "auto", 1 } } },
        { "Batteria | Room ALL buttons", "Il classico trucco del 1176 sulla room: tutti i pulsanti e mix in parallelo.", 0,
          { { "a_model", FET76 }, { "a_drive", 0.7f }, { "a_ratio", 4 }, { "a_att", 0.9f }, { "a_rel", 0.15f }, { "a_color", 0.6f }, { "a_mix", 0.4f }, { "auto", 1 } } },
        { "Batteria | Cassa punch", "RMS 160 con soglia per 4-6 dB: la cassa resta compatta senza perdere l'attacco.", 0,
          { { "a_model", RMS160 }, { "a_drive", 0.4f }, { "a_ratio", 2 }, { "a_color", 0.2f }, { "auto", 1 } } },
        { "Batteria | Rullante crack", "FET con attacco lento per far passare il colpo, poi OPTO 3A per tenerlo fermo.", 1,
          { { "a_model", FET76 }, { "a_drive", 0.3f }, { "a_ratio", 1 }, { "a_att", 0.2f }, { "a_rel", 0.25f },
            { "b_on", 1 }, { "b_model", OPTO3A }, { "b_drive", 0.3f }, { "auto", 1 } } },
        { "Batteria | Lo-fi Vari-Mu", "Valvola a pendenza variabile spinta con colore: batteria spessa e sporca.", 0,
          { { "a_model", VARIMU }, { "a_drive", 0.4f }, { "a_ratio", 1 }, { "a_color", 0.7f }, { "auto", 1 } } },
        { "Basso | 808 costante", "RMS 160 per livellare l'808, HPF sidechain basso per non pompare.", 0,
          { { "a_model", RMS160 }, { "a_drive", 0.45f }, { "a_ratio", 2 }, { "a_color", 0.25f }, { "auto", 1 } } },
        { "Basso | 808 distorto", "Distress con 2a armonica: l'808 si sente anche sulle casse piccole.", 0,
          { { "a_model", DISTRESS }, { "a_drive", 0.4f }, { "a_ratio", 3 }, { "a_att", 0.4f }, { "a_rel", 0.3f }, { "a_color", 0.4f }, { "auto", 1 } } },
        { "Sidechain | Ducking cassa", "Metti la cassa nel sidechain (ESTERNO). Il VCA abbassa basso o pad su ogni colpo: rilascio a tempo.", 0,
          { { "a_model", VCABUS }, { "a_drive", 0.6f }, { "a_ratio", 2 }, { "a_att", 0.0f }, { "a_rel", 0.25f }, { "scSrc", 1 }, { "a_color", 0.0f } } },
        { "Sidechain | Pumping EDM", "Ducking profondo e rilascio lungo: il classico 'respiro' a tempo. Sidechain ESTERNO dalla cassa.", 0,
          { { "a_model", DISTRESS }, { "a_drive", 0.7f }, { "a_ratio", 6 }, { "a_att", 0.0f }, { "a_rel", 0.45f }, { "scSrc", 1 }, { "a_color", 0.0f } } },
        { "Sidechain | De-boom (HPF)", "Sidechain interno con HPF a 150 Hz: il compressore non reagisce ai bassi e non pompa.", 0,
          { { "a_model", VCABUS }, { "a_drive", 0.4f }, { "a_ratio", 1 }, { "a_att", 0.8f }, { "a_rel", 1.0f }, { "scHpf", 150 } } },
        { "Bus | Mix bus glue", "VCA ratio 2, attacco 30 ms, AUTO: 1-2 dB per 'incollare' il mix.", 0,
          { { "a_model", VCABUS }, { "a_drive", 0.25f }, { "a_ratio", 0 }, { "a_att", 1.0f }, { "a_rel", 1.0f }, { "a_color", 0.15f }, { "auto", 1 } } },
        { "Master | Vari-Mu caldo", "TC5 automatico, 1-2 dB: calore e coesione sul master prima del limiter.", 0,
          { { "a_model", VARIMU }, { "a_drive", 0.08f }, { "a_ratio", 4 }, { "a_color", 0.35f }, { "auto", 1 } } },
        { "Master | Serie gentile", "VCA leggero + OPTO leggero: due stadi da 1 dB suonano meglio di uno da 2.", 1,
          { { "a_model", VCABUS }, { "a_drive", 0.2f }, { "a_ratio", 0 }, { "a_att", 1.0f }, { "a_rel", 1.0f }, { "a_color", 0.1f },
            { "b_on", 1 }, { "b_model", OPTO2A }, { "b_drive", 0.25f }, { "b_color", 0.25f }, { "auto", 1 } } },
        { "Parallelo | Smash + Opto", "PARALLELO: A distrugge (NUKE), B livella morbido. Dosa i due MIX.", 2,
          { { "a_model", DISTRESS }, { "a_drive", 0.7f }, { "a_ratio", 7 }, { "a_att", 0.1f }, { "a_rel", 0.3f }, { "a_color", 0.6f }, { "a_mix", 0.6f },
            { "b_on", 1 }, { "b_model", OPTO2A }, { "b_drive", 0.4f }, { "auto", 1 } } } };
    return P;
}

void Processor::loadPreset (int i)
{
    if (! juce::isPositiveAndBelow (i, (int) presets().size())) return;
    preset = i;
    for (auto* p : getParameters()) if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p)) rp->setValueNotifyingHost (rp->getDefaultValue());
    const auto& P = presets()[(size_t) i];
    if (auto* rp = apvts.getParameter ("route")) rp->setValueNotifyingHost (rp->convertTo0to1 ((float) P.route));
    for (auto& [id, v] : P.v) if (auto* rp = apvts.getParameter (id)) rp->setValueNotifyingHost (rp->convertTo0to1 (v));
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

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new jc::Processor(); }
