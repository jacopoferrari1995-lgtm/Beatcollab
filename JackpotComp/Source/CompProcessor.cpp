#include "CompProcessor.h"
#include "CompEditor.h"

namespace jc
{
static juce::StringArray modelNames() { juce::StringArray a; for (int m = 0; m < kNumModels; ++m) a.add (info (m).name); return a; }

juce::AudioProcessorValueTreeState::ParameterLayout Processor::layout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    for (int st = 0; st < kStages; ++st)
    {
        const String p = String (stagePfx (st)).dropLastCharacters (1), P = "Stadio " + String (st + 1) + " ";
        l.add (std::make_unique<AudioParameterBool> (ParameterID { p + "_on", 1 }, P + "On", false));
        static const int defM[kStages] = { FET76, OPTO2A, VCABUS, RMS160, VARIMU, OTT };
        l.add (std::make_unique<AudioParameterChoice> (ParameterID { p + "_model", 1 }, P + "Model", modelNames(), defM[st]));
        l.add (std::make_unique<AudioParameterBool> (ParameterID { p + "_solo", 1 }, P + "Solo", false));
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { p + "_drive", 1 }, P + "Input/Thresh", NormalisableRange<float> (0, 1), 0.0f));
        l.add (std::make_unique<AudioParameterInt> (ParameterID { p + "_ratio", 1 }, P + "Ratio", 0, 7, 0));
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { p + "_att", 1 }, P + "Attack", NormalisableRange<float> (0, 1), 0.5f));
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { p + "_rel", 1 }, P + "Release", NormalisableRange<float> (0, 1), 0.35f));
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { p + "_out", 1 }, P + "Output", NormalisableRange<float> (-12, 24), 0));
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { p + "_color", 1 }, P + "Color", NormalisableRange<float> (0, 1), 0.0f));
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { p + "_mix", 1 }, P + "Mix", NormalisableRange<float> (0, 1), 1));
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { p + "_drive2", 1 }, P + "Input R/Vert", NormalisableRange<float> (0, 1), 0.0f));
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { p + "_att2", 1 }, P + "Threshold R/Vert", NormalisableRange<float> (0, 1), 0.0f));
        l.add (std::make_unique<AudioParameterInt> (ParameterID { p + "_ratio2", 1 }, P + "Time Const R/Vert", 0, 7, 0));
        l.add (std::make_unique<AudioParameterBool> (ParameterID { p + "_link", 1 }, P + "Link", true));
        l.add (std::make_unique<AudioParameterBool> (ParameterID { p + "_olink", 1 }, P + "Output Link", false));
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { p + "_thr", 1 }, P + "Threshold 670", NormalisableRange<float> (0, 1), 0.0f));
    }
    // multibanda
    auto F = [&l] (const String& id, const String& n, NormalisableRange<float> r, float d) { l.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, n, r, d)); };
    auto skew = [] (float a, float b, float c) { NormalisableRange<float> r (a, b); r.setSkewForCentre (c); return r; };
    F ("mb_x1", "MB taglio 1", skew (30, 1000, 150), 120); F ("mb_x2", "MB taglio 2", skew (150, 6000, 1000), 1000); F ("mb_x3", "MB taglio 3", skew (1000, 16000, 5000), 6000);
    for (int b = 1; b <= 4; ++b)
    {
        const String p = "mb_b" + String (b) + "_", P = "MB banda " + String (b) + " ";
        F (p + "thr", P + "soglia", { -60, 0 }, 0); F (p + "ratio", P + "ratio", skew (1, 20, 3), 2.5f);
        F (p + "att", P + "attacco", skew (0.1f, 200, 10), 10); F (p + "rel", P + "rilascio", skew (5, 2000, 150), 150); F (p + "gain", P + "guadagno", { -12, 12 }, 0);
        l.add (std::make_unique<AudioParameterBool> (ParameterID { p + "solo", 1 }, P + "solo", false));
        l.add (std::make_unique<AudioParameterBool> (ParameterID { p + "byp", 1 }, P + "bypass", false));
    }
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "route", 1 }, "Routing", StringArray { "SERIE", "SERIE", "PARALLELO" }, 0));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "scSrc", 1 }, "Sidechain", StringArray { "INTERNO", "ESTERNO" }, 0));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "scTo", 1 }, "Sidechain su", StringArray { "TUTTI", "STADIO 1", "STADIO 2", "STADIO 3", "STADIO 4", "STADIO 5", "STADIO 6" }, 0));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "scHpf", 1 }, "SC HPF", NormalisableRange<float> (20, 500, 0, 0.4f), 20));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "scListen", 1 }, "SC Listen", false));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "auto", 1 }, "Auto Gain", false));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "delta", 1 }, "Delta", false));
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
    for (int i = 0; i < kHist; ++i) { hIn[i] = -60; hOut[i] = -60; for (int s = 0; s < kStages; ++s) hGr[s][i] = 0; }
    // puntatori ai parametri presi una volta sola: nel processBlock niente stringhe ne' ricerche (allocavano memoria nel thread audio)
    auto R = [this] (const juce::String& id) { return apvts.getRawParameterValue (id); };
    for (int s = 0; s < kStages; ++s)
    {
        const juce::String p = juce::String (stagePfx (s)).dropLastCharacters (1);
        auto& q = pp[s];
        q.on = R (p + "_on"); q.model = R (p + "_model"); q.solo = R (p + "_solo"); q.drive = R (p + "_drive"); q.ratio = R (p + "_ratio");
        q.att = R (p + "_att"); q.rel = R (p + "_rel"); q.out = R (p + "_out"); q.color = R (p + "_color"); q.mix = R (p + "_mix");
        q.drive2 = R (p + "_drive2"); q.att2 = R (p + "_att2"); q.ratio2 = R (p + "_ratio2"); q.link = R (p + "_link"); q.olink = R (p + "_olink"); q.thr = R (p + "_thr");
    }
    for (int b = 0; b < 4; ++b)
    {
        const juce::String p = "mb_b" + juce::String (b + 1) + "_";
        mbp[b] = { R (p + "thr"), R (p + "ratio"), R (p + "att"), R (p + "rel"), R (p + "gain"), R (p + "solo"), R (p + "byp") };
    }
    gx[0] = R ("mb_x1"); gx[1] = R ("mb_x2"); gx[2] = R ("mb_x3");
    gScSrc = R ("scSrc"); gScTo = R ("scTo"); gScHpf = R ("scHpf"); gScListen = R ("scListen"); gRoute = R ("route"); gAuto = R ("auto"); gMix = R ("mix"); gOut = R ("out"); gDelta = R ("delta");
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
    cap.prepare (s);
    dry.setSize (2, maxBlock); det.setSize (2, maxBlock); aux.setSize (2, maxBlock); acc.setSize (2, maxBlock); dlt.setSize (2, maxBlock); tmp.setSize (2, maxBlock);
    for (auto& a : hp) for (auto& f : a) f.reset();
    lastHp = -1;
}

StageParams Processor::stageParams (int s) const
{
    const auto& p = pp[s];
    StageParams q;
    q.on = p.on->load() > 0.5f;
    q.model = juce::jlimit (0, kNumModels - 1, juce::roundToInt (p.model->load()));
    q.drive = p.drive->load();
    q.ratio = juce::jlimit (0, info (q.model).ratios.size() - 1, juce::roundToInt (p.ratio->load()));
    q.att = p.att->load(); q.rel = p.rel->load();
    q.outDb = p.out->load(); q.color = p.color->load(); q.mix = p.mix->load();
    q.drive2 = p.drive2->load(); q.att2 = p.att2->load(); q.thr670 = p.thr->load(); q.ratio2 = juce::jlimit (0, 5, juce::roundToInt (p.ratio2->load()));
    q.link = p.link->load() > 0.5f; q.outLink = false;
    q.mb = &mbNow;
    return q;
}

void Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals nd;
    auto main = getBusBuffer (buffer, true, 0);
    const int N = buffer.getNumSamples(), nch = juce::jmin (2, main.getNumChannels());
    if (N > dry.getNumSamples()) for (auto* b : { &dry, &det, &aux, &acc, &dlt, &tmp }) b->setSize (2, N, false, false, true);
    float* L = main.getWritePointer (0); float* R = nch > 1 ? main.getWritePointer (1) : main.getWritePointer (0);

    // sidechain esterno
    const float* eL = nullptr; const float* eR = nullptr;
    if (getBusCount (true) > 1 && getBus (true, 1)->isEnabled())
    {
        auto sc = getBusBuffer (buffer, true, 1);
        if (sc.getNumChannels() > 0) { eL = sc.getReadPointer (0); eR = sc.getReadPointer (juce::jmin (1, sc.getNumChannels() - 1)); }
    }
    extSc = eL != nullptr;
    const bool useExt = gScSrc->load() > 0.5f && eL != nullptr;
    const int scTo = juce::roundToInt (gScTo->load());

    // misura ingresso
    float pkI[2] = { 0, 0 }; double eIn = 0;
    for (int i = 0; i < N; ++i) { pkI[0] = juce::jmax (pkI[0], std::abs (L[i])); pkI[1] = juce::jmax (pkI[1], std::abs (R[i])); eIn += 0.5 * (L[i] * L[i] + R[i] * R[i]); }
    dry.copyFrom (0, 0, L, N); dry.copyFrom (1, 0, R, N);
    cap.push (L, R, N);

    // filtro passa alto del sidechain
    const float hpF = gScHpf->load();
    if (std::abs (hpF - lastHp) > 0.5f)
    {
        auto c = juce::dsp::IIR::Coefficients<float>::makeHighPass (sr, juce::jmax (20.0f, hpF), 0.707f);
        for (auto& a : hp) for (auto& f : a) f.coefficients = c;
        lastHp = hpF;
    }
    auto makeDet = [&] (int s, const float* sl, const float* sr2)
    {
        const bool ext = useExt && (scTo == 0 || scTo == s + 1);
        float* dl = det.getWritePointer (0); float* dr = det.getWritePointer (1);
        juce::FloatVectorOperations::copy (dl, ext ? eL : sl, N); juce::FloatVectorOperations::copy (dr, ext ? eR : sr2, N);
        if (hpF > 21) for (int i = 0; i < N; ++i) { dl[i] = hp[s][0].processSample (dl[i]); dr[i] = hp[s][1].processSample (dr[i]); }
    };

    for (int k = 0; k < 3; ++k) mbNow.xo[k] = gx[k]->load();
    for (int b = 0; b < 4; ++b)
    {
        const auto& q = mbp[b];
        mbNow.thr[b] = q.thr->load(); mbNow.ratio[b] = q.ratio->load(); mbNow.att[b] = q.att->load(); mbNow.rel[b] = q.rel->load(); mbNow.gain[b] = q.gain->load();
        mbNow.solo[b] = q.solo->load() > 0.5f; mbNow.byp[b] = q.byp->load() > 0.5f;
    }
    const bool parallel = juce::roundToInt (gRoute->load()) == 2, autoG = gAuto->load() > 0.5f, deltaAll = gDelta->load() > 0.5f;
    StageParams sp[kStages]; bool solo[kStages]; bool anyS = false; int nOn = 0;
    // DELTA = tutti gli stadi accesi in solo: si sente solo quello che la compressione toglie
    for (int s = 0; s < kStages; ++s) { sp[s] = stageParams (s); solo[s] = sp[s].on && (deltaAll || pp[s].solo->load() > 0.5f); anyS |= solo[s]; nOn += sp[s].on ? 1 : 0; }
    float* xL = aux.getWritePointer (0); float* xR = aux.getWritePointer (1);
    float* aL = acc.getWritePointer (0); float* aR = acc.getWritePointer (1);
    float* dlL = dlt.getWritePointer (0); float* dlR = dlt.getWritePointer (1);
    if (anyS) { juce::FloatVectorOperations::clear (dlL, N); juce::FloatVectorOperations::clear (dlR, N); }
    if (parallel && nOn > 0) { juce::FloatVectorOperations::clear (aL, N); juce::FloatVectorOperations::clear (aR, N); }
    float* tmpL = tmp.getWritePointer (0); float* tmpR = tmp.getWritePointer (1);
    for (int s = 0; s < kStages; ++s)
    {
        if (! sp[s].on) { st[s].reset(); continue; }
        float* pl = L; float* pr = R;
        if (parallel) { juce::FloatVectorOperations::copy (xL, dry.getReadPointer (0), N); juce::FloatVectorOperations::copy (xR, dry.getReadPointer (1), N); pl = xL; pr = xR; }
        makeDet (s, pl, pr);
        {   // livello che entra nel rilevatore di questo stadio: serve per mostrare la soglia
            const float a = juce::FloatVectorOperations::findMaximum (det.getReadPointer (0), N), b = -juce::FloatVectorOperations::findMinimum (det.getReadPointer (0), N);
            const float c = juce::FloatVectorOperations::findMaximum (det.getReadPointer (1), N), d = -juce::FloatVectorOperations::findMinimum (det.getReadPointer (1), N);
            const float pk = juce::jmax (a, b, juce::jmax (c, d));
            stIn[s] = juce::jmax (pk, stIn[s].load() * std::exp (-(float) N / (0.35f * (float) sr)));
        }
        const bool wantD = solo[s];
        st[s].process (pl, pr, det.getReadPointer (0), det.getReadPointer (1), N, sp[s], autoG, wantD ? tmpL : nullptr, wantD ? tmpR : nullptr);
        if (wantD) { juce::FloatVectorOperations::add (dlL, tmpL, N); juce::FloatVectorOperations::add (dlR, tmpR, N); }
        if (parallel) { juce::FloatVectorOperations::add (aL, xL, N); juce::FloatVectorOperations::add (aR, xR, N); }
    }
    if (parallel && nOn > 0) { const float k = 1.0f / (float) nOn; for (int i = 0; i < N; ++i) { L[i] = aL[i] * k; R[i] = aR[i] * k; } }

    if (gScListen->load() > 0.5f)
    {
        makeDet (0, dry.getReadPointer (0), dry.getReadPointer (1));
        juce::FloatVectorOperations::copy (L, det.getReadPointer (0), N); juce::FloatVectorOperations::copy (R, det.getReadPointer (1), N);
    }
    else if (anyS)
    {   // solo: si sente solo cio che gli stadi in solo stanno togliendo
        juce::FloatVectorOperations::copy (L, dlL, N); juce::FloatVectorOperations::copy (R, dlR, N);
    }
    else
    {
        const float mix = gMix->load(), og = juce::Decibels::decibelsToGain (gOut->load());
        for (int i = 0; i < N; ++i) { L[i] = (dry.getSample (0, i) * (1 - mix) + L[i] * mix) * og; R[i] = (dry.getSample (1, i) * (1 - mix) + R[i] * mix) * og; }
    }

    // misure
    float pkO[2] = { 0, 0 }; double eOut = 0;
    for (int i = 0; i < N; ++i) { pkO[0] = juce::jmax (pkO[0], std::abs (L[i])); pkO[1] = juce::jmax (pkO[1], std::abs (R[i])); eOut += 0.5 * (L[i] * L[i] + R[i] * R[i]); }
    for (int c = 0; c < 2; ++c) { inPk[c] = juce::jmax (pkI[c], inPk[c].load() * 0.92f); outPk[c] = juce::jmax (pkO[c], outPk[c].load() * 0.92f); }
    const float k = std::exp (-(float) N / (0.3f * (float) sr));
    rmsIn = k * rmsIn + (1 - k) * (float) (eIn / N); rmsOut = k * rmsOut + (1 - k) * (float) (eOut / N);
    inRms = rmsIn; outRms = rmsOut;
    for (int b = 0; b < 4; ++b) { float m = 0; for (int s = 0; s < kStages; ++s) if (sp[s].on && sp[s].model == MULTI) m = juce::jmax (m, st[s].bandGr[b].load()); mbGr[b] = juce::jmax (m, mbGr[b].load() * 0.85f); }
    for (int s = 0; s < kStages; ++s) { gr[s] = sp[s].on ? st[s].grNow() : 0.0f; avgGr[s] = sp[s].on ? st[s].grAvg : 0.0f; colGr[s] = juce::jmax (colGr[s], gr[s].load()); }
    colIn = juce::jmax (colIn, juce::jmax (pkI[0], pkI[1])); colOut = juce::jmax (colOut, juce::jmax (pkO[0], pkO[1]));
    colN += N;
    if (colN >= (int) (sr * 4.0 / kHist))
    {
        const int w = hW.load();
        hIn[w] = juce::Decibels::gainToDecibels (colIn, -60.0f); hOut[w] = juce::Decibels::gainToDecibels (colOut, -60.0f);
        for (int s = 0; s < kStages; ++s) { hGr[s][w] = colGr[s]; colGr[s] = 0; }
        hW = (w + 1) % kHist; colIn = colOut = 0; colN = 0;
    }
    for (int c = 2; c < buffer.getNumChannels(); ++c) buffer.clear (c, 0, N);
}

// ======================= preset =======================
const std::vector<Preset>& presets()
{
    static const std::vector<Preset> P = {
        { "Init", "Tutto spento: accendi uno stadio dalla fila in alto.", 0, {} },
        // ---------------- VOCE
        { "Voce | Rap davanti (76 > 2A)", "La catena classica: il FET prende solo i picchi (3-5 dB, attacco medio per non mangiare le consonanti), l'OPTO livella le frasi (3-4 dB). Voce sempre davanti senza sentirsi compressa.", 1,
          { { "a_model", FET76 }, { "a_drive", 0.3f }, { "a_ratio", 0 }, { "a_att", 0.55f }, { "a_rel", 0.3f }, { "a_color", 0.35f },
            { "b_on", 1 }, { "b_model", OPTO2A }, { "b_drive", 0.35f }, { "b_ratio", 0 }, { "b_color", 0.3f }, { "auto", 1 } } },
        { "Voce | Rap con controllo S", "Come 76 > 2A, piu un multibanda solo sulle alte (oltre 6 kHz, attacco 2 ms): le S e le T non escono dopo la compressione.", 0,
          { { "a_model", FET76 }, { "a_drive", 0.3f }, { "a_att", 0.55f }, { "a_rel", 0.3f }, { "a_color", 0.3f },
            { "b_on", 1 }, { "b_model", OPTO2A }, { "b_drive", 0.3f }, { "b_color", 0.25f },
            { "c_on", 1 }, { "c_model", MULTI }, { "c_ratio", 0 }, { "mb_x2", 6000 }, { "mb_b1_thr", 0 }, { "mb_b2_thr", -30 }, { "mb_b2_ratio", 4 }, { "mb_b2_att", 2 }, { "mb_b2_rel", 60 }, { "auto", 1 } } },
        { "Voce | Morbida 2A", "Solo ottico, livellamento trasparente per voci cantate e ritornelli. Il sidechain del 2A sente di piu gli acuti: un filo di de-essing gratis.", 0,
          { { "a_model", OPTO2A }, { "a_drive", 0.45f }, { "a_ratio", 0 }, { "a_color", 0.4f }, { "auto", 1 } } },
        { "Voce | Cantata 670 calda", "VARI 670 in TC2 con 2-4 dB: la voce si arrotonda, prende corpo sotto e brillantezza sopra. Il suono dei dischi soul.", 0,
          { { "a_model", VC670 }, { "a_drive", 0.3f }, { "a_att", 0.3f }, { "a_ratio", 1 }, { "a_color", 0.45f }, { "auto", 1 } } },
        { "Voce | Lead 76 > 670", "Il FET ferma i picchi, il 670 incolla e scalda. Per la voce principale che deve stare sopra un beat pieno.", 0,
          { { "a_model", FET76 }, { "a_drive", 0.28f }, { "a_att", 0.5f }, { "a_rel", 0.35f }, { "a_color", 0.3f },
            { "b_on", 1 }, { "b_model", VC670 }, { "b_drive", 0.25f }, { "b_att", 0.25f }, { "b_ratio", 1 }, { "b_color", 0.4f }, { "auto", 1 } } },
        { "Voce | Aggressiva Distress", "Distress con distorsione di 3a armonica: voce che graffia e sta sopra il beat. Bene su trap e drill.", 0,
          { { "a_model", DISTRESS }, { "a_drive", 0.38f }, { "a_ratio", 3 }, { "a_att", 0.3f }, { "a_rel", 0.35f }, { "a_color", 0.7f }, { "auto", 1 } } },
        { "Voce | Adlib schiacciati", "FET in ALL: adlib piatti e distorti, da mettere sotto la voce principale.", 0,
          { { "a_model", FET76 }, { "a_drive", 0.6f }, { "a_ratio", 4 }, { "a_att", 0.7f }, { "a_rel", 0.2f }, { "a_color", 0.5f }, { "auto", 1 } } },
        { "Voce | Cori larghi", "670 in LAT/VERT: comprime il centro e lascia i lati piu aperti. I cori restano larghi senza scavalcare la voce.", 0,
          { { "a_model", VC670 }, { "a_drive", 0.25f }, { "a_att", 0.3f }, { "a_rel", 1.0f }, { "a_ratio", 2 }, { "a_color", 0.3f }, { "auto", 1 } } },
        { "Voce | Fango e S (multibanda)", "Multibanda a 4 bande: 200-500 Hz controllato quando la voce si gonfia, 6 kHz+ per le S. Medie e basse quasi libere.", 0,
          { { "a_model", MULTI }, { "a_ratio", 2 }, { "mb_x1", 200 }, { "mb_x2", 600 }, { "mb_x3", 6000 },
            { "mb_b1_thr", -30 }, { "mb_b1_ratio", 2 }, { "mb_b2_thr", -26 }, { "mb_b2_ratio", 3 }, { "mb_b2_att", 15 }, { "mb_b2_rel", 120 },
            { "mb_b3_thr", -12 }, { "mb_b3_ratio", 1.5f }, { "mb_b4_thr", -30 }, { "mb_b4_ratio", 5 }, { "mb_b4_att", 1.5f }, { "mb_b4_rel", 50 }, { "auto", 1 } } },
        // ---------------- BATTERIA
        { "Batteria | Drum bus colla", "VCA da bus: attacco 30 ms per i transienti, rilascio AUTO. 2-4 dB di riduzione.", 0,
          { { "a_model", VCABUS }, { "a_drive", 0.35f }, { "a_ratio", 1 }, { "a_att", 1.0f }, { "a_rel", 1.0f }, { "a_color", 0.2f }, { "auto", 1 } } },
        { "Batteria | Bus 670 spesso", "670 TC2 sul bus batteria: picchi arrotondati, coda che si alza, bassi che si gonfiano col trasformatore. Boom bap e soul.", 0,
          { { "a_model", VC670 }, { "a_drive", 0.45f }, { "a_att", 0.4f }, { "a_ratio", 1 }, { "a_color", 0.6f }, { "auto", 1 } } },
        { "Batteria | Parallelo New York", "Stadio A schiacciato, MIX al 35%: il dry tiene i transienti, il compresso da corpo e coda.", 0,
          { { "a_model", DISTRESS }, { "a_drive", 0.6f }, { "a_ratio", 6 }, { "a_att", 0.25f }, { "a_rel", 0.3f }, { "a_color", 0.55f }, { "a_mix", 0.35f }, { "auto", 1 } } },
        { "Batteria | Room ALL buttons", "Il trucco del 1176 sulla room: tutti i pulsanti, rilascio veloce che pompa, mix in parallelo.", 0,
          { { "a_model", FET76 }, { "a_drive", 0.7f }, { "a_ratio", 4 }, { "a_att", 0.9f }, { "a_rel", 0.15f }, { "a_color", 0.6f }, { "a_mix", 0.4f }, { "auto", 1 } } },
        { "Batteria | Cassa punch", "RMS 160 con soglia per 4-6 dB: la cassa resta compatta senza perdere l'attacco (il 160 lascia passare i primi ms).", 0,
          { { "a_model", RMS160 }, { "a_drive", 0.4f }, { "a_ratio", 2 }, { "a_color", 0.2f }, { "auto", 1 } } },
        { "Batteria | Rullante crack", "FET con attacco lento per far passare il colpo, poi OPTO 3A per tenerlo fermo.", 1,
          { { "a_model", FET76 }, { "a_drive", 0.3f }, { "a_ratio", 1 }, { "a_att", 0.2f }, { "a_rel", 0.25f },
            { "b_on", 1 }, { "b_model", OPTO3A }, { "b_drive", 0.3f }, { "auto", 1 } } },
        { "Batteria | Lo-fi Vari-Mu", "Valvola a pendenza variabile spinta con colore: batteria spessa e sporca.", 0,
          { { "a_model", VARIMU }, { "a_drive", 0.4f }, { "a_ratio", 1 }, { "a_color", 0.7f }, { "auto", 1 } } },
        { "Batteria | Trap bus pulito", "VCA ratio 4, attacco 10 ms, rilascio 0.1 s a tempo + multibanda leggero sulle alte: hat lucidi e 808 fermo.", 0,
          { { "a_model", VCABUS }, { "a_drive", 0.3f }, { "a_ratio", 1 }, { "a_att", 0.8f }, { "a_rel", 0.0f }, { "a_color", 0.1f },
            { "b_on", 1 }, { "b_model", MULTI }, { "b_ratio", 0 }, { "mb_x2", 5000 }, { "mb_b1_thr", 0 }, { "mb_b2_thr", -24 }, { "mb_b2_ratio", 2.5f }, { "mb_b2_att", 3 }, { "mb_b2_rel", 80 }, { "auto", 1 } } },
        { "Batteria | Boom bap a 3 bande", "Multibanda a 3 bande: basse tenute (cassa piena e costante), medie che respirano, alte addomesticate. WARM al 40%.", 0,
          { { "a_model", MULTI }, { "a_ratio", 1 }, { "a_color", 0.4f }, { "mb_x1", 150 }, { "mb_x3", 4500 },
            { "mb_b1_thr", -22 }, { "mb_b1_ratio", 4 }, { "mb_b1_att", 25 }, { "mb_b1_rel", 180 }, { "mb_b2_thr", -20 }, { "mb_b2_ratio", 2 }, { "mb_b2_att", 15 }, { "mb_b2_rel", 120 },
            { "mb_b3_thr", -24 }, { "mb_b3_ratio", 3 }, { "mb_b3_att", 4 }, { "mb_b3_rel", 90 }, { "auto", 1 } } },
        // ---------------- BASSO
        { "Basso | 808 costante", "RMS 160 per livellare l'808 nota per nota. HPF sidechain basso per non pompare.", 0,
          { { "a_model", RMS160 }, { "a_drive", 0.45f }, { "a_ratio", 2 }, { "a_color", 0.25f }, { "auto", 1 } } },
        { "Basso | 808 distorto", "Distress con 2a armonica: l'808 si sente anche sulle casse piccole.", 0,
          { { "a_model", DISTRESS }, { "a_drive", 0.4f }, { "a_ratio", 3 }, { "a_att", 0.4f }, { "a_rel", 0.3f }, { "a_color", 0.4f }, { "auto", 1 } } },
        { "Basso | 808 a 2 bande", "Multibanda a 2 bande a 120 Hz: il sub viene tenuto fermo (6:1), la parte alta che fa sentire la nota resta libera.", 0,
          { { "a_model", MULTI }, { "a_ratio", 0 }, { "mb_x2", 120 }, { "mb_b1_thr", -20 }, { "mb_b1_ratio", 6 }, { "mb_b1_att", 20 }, { "mb_b1_rel", 250 }, { "mb_b2_thr", -10 }, { "mb_b2_ratio", 1.5f }, { "mb_b2_gain", 1.5f }, { "auto", 1 } } },
        { "Basso | Elettrico 2A + 670", "Opto per il livello, 670 per il calore del trasformatore sui bassi. Il basso suonato che si incolla al beat.", 1,
          { { "a_model", OPTO2A }, { "a_drive", 0.4f }, { "a_color", 0.3f }, { "b_on", 1 }, { "b_model", VC670 }, { "b_drive", 0.3f }, { "b_att", 0.3f }, { "b_ratio", 2 }, { "b_color", 0.55f }, { "auto", 1 } } },
        // ---------------- SIDECHAIN
        { "Sidechain | Ducking cassa", "Metti la cassa nel sidechain (ESTERNO). Il VCA abbassa basso o pad su ogni colpo: rilascio a tempo.", 0,
          { { "a_model", VCABUS }, { "a_drive", 0.6f }, { "a_ratio", 2 }, { "a_att", 0.0f }, { "a_rel", 0.25f }, { "scSrc", 1 }, { "a_color", 0.0f } } },
        { "Sidechain | Pumping EDM", "Ducking profondo e rilascio lungo: il classico respiro a tempo. Sidechain ESTERNO dalla cassa.", 0,
          { { "a_model", DISTRESS }, { "a_drive", 0.7f }, { "a_ratio", 6 }, { "a_att", 0.0f }, { "a_rel", 0.45f }, { "scSrc", 1 }, { "a_color", 0.0f } } },
        { "Sidechain | De-boom (HPF)", "Sidechain interno con HPF a 150 Hz: il compressore non reagisce ai bassi e non pompa.", 0,
          { { "a_model", VCABUS }, { "a_drive", 0.4f }, { "a_ratio", 1 }, { "a_att", 0.8f }, { "a_rel", 1.0f }, { "scHpf", 150 } } },
        { "Sidechain | 808 sotto la cassa", "Sidechain ESTERNO dalla cassa su un multibanda a 2 bande: si abbassa solo il sub dell'808 quando suona la cassa, la nota resta.", 0,
          { { "a_model", MULTI }, { "a_ratio", 0 }, { "scSrc", 1 }, { "mb_x2", 100 }, { "mb_b1_thr", -30 }, { "mb_b1_ratio", 8 }, { "mb_b1_att", 1 }, { "mb_b1_rel", 120 }, { "mb_b2_thr", 0 } } },
        // ---------------- BUS E MASTER
        { "Bus | Mix bus glue", "VCA ratio 2, attacco 30 ms, AUTO: 1-2 dB per incollare il mix.", 0,
          { { "a_model", VCABUS }, { "a_drive", 0.25f }, { "a_ratio", 0 }, { "a_att", 1.0f }, { "a_rel", 1.0f }, { "a_color", 0.15f }, { "auto", 1 } } },
        { "Bus | Strumentale 670", "TC3 sul bus del beat: tutto si fonde, i bassi si fanno tondi e le alte brillano un po'.", 0,
          { { "a_model", VC670 }, { "a_drive", 0.2f }, { "a_att", 0.25f }, { "a_ratio", 2 }, { "a_color", 0.35f }, { "auto", 1 } } },
        { "Master | Vari-Mu caldo", "TC5 automatico, 1-2 dB: calore e coesione sul master prima del limiter.", 0,
          { { "a_model", VARIMU }, { "a_drive", 0.08f }, { "a_ratio", 4 }, { "a_color", 0.35f }, { "auto", 1 } } },
        { "Master | 670 LAT/VERT", "Il 670 come nei mastering degli anni 60: LAT/VERT, TC5, 1 dB. Il centro (voce, cassa, basso) sta fermo, i lati respirano.", 0,
          { { "a_model", VC670 }, { "a_drive", 0.1f }, { "a_att", 0.12f }, { "a_rel", 1.0f }, { "a_ratio", 4 }, { "a_color", 0.25f }, { "auto", 1 } } },
        { "Master | Multibanda alpha", "4 bande gentili (1-2 dB ciascuna), WARM al 50%: tiene in riga il mix senza schiacciarlo, alla maniera dei compressori da mastering.", 0,
          { { "a_model", MULTI }, { "a_ratio", 2 }, { "a_color", 0.5f }, { "mb_x1", 120 }, { "mb_x2", 900 }, { "mb_x3", 7000 },
            { "mb_b1_thr", -18 }, { "mb_b1_ratio", 2 }, { "mb_b1_att", 30 }, { "mb_b1_rel", 300 }, { "mb_b2_thr", -18 }, { "mb_b2_ratio", 1.6f }, { "mb_b2_att", 20 }, { "mb_b2_rel", 200 },
            { "mb_b3_thr", -20 }, { "mb_b3_ratio", 1.6f }, { "mb_b3_att", 10 }, { "mb_b3_rel", 150 }, { "mb_b4_thr", -24 }, { "mb_b4_ratio", 2 }, { "mb_b4_att", 5 }, { "mb_b4_rel", 100 }, { "auto", 1 } } },
        { "Master | Serie gentile", "VCA leggero + OPTO leggero: due stadi da 1 dB suonano meglio di uno da 2.", 1,
          { { "a_model", VCABUS }, { "a_drive", 0.2f }, { "a_ratio", 0 }, { "a_att", 1.0f }, { "a_rel", 1.0f }, { "a_color", 0.1f },
            { "b_on", 1 }, { "b_model", OPTO2A }, { "b_drive", 0.25f }, { "b_color", 0.25f }, { "auto", 1 } } },
        { "Master | Glue + multibanda", "VCA bus per la colla (1-2 dB) e multibanda a 3 bande per tenere fermi i bassi e le alte.", 1,
          { { "a_model", VCABUS }, { "a_drive", 0.22f }, { "a_att", 1.0f }, { "a_rel", 1.0f }, { "a_color", 0.1f },
            { "b_on", 1 }, { "b_model", MULTI }, { "b_ratio", 1 }, { "b_color", 0.3f }, { "mb_x1", 140 }, { "mb_x3", 6000 },
            { "mb_b1_thr", -18 }, { "mb_b1_ratio", 2.5f }, { "mb_b1_att", 30 }, { "mb_b1_rel", 250 }, { "mb_b2_thr", -10 }, { "mb_b2_ratio", 1.3f }, { "mb_b3_thr", -22 }, { "mb_b3_ratio", 2 }, { "mb_b3_att", 5 }, { "mb_b3_rel", 100 }, { "auto", 1 } } },
        // ---------------- OTT E CATENE
        { "OTT | Hat e Synth", "OTT a profondita media con MIX al 50%: hat e synth brillanti e densi senza perdere del tutto la dinamica.", 0,
          { { "a_model", OTT }, { "a_drive", 0.35f }, { "a_ratio", 0 }, { "a_mix", 0.5f }, { "a_color", 0.1f }, { "auto", 1 } } },
        { "OTT | 808 in Faccia", "FET per tenere il picco, poi OTT leggero: l'808 si sente anche su telefono.", 0,
          { { "a_model", FET76 }, { "a_drive", 0.25f }, { "a_ratio", 0 }, { "a_att", 0.6f }, { "a_rel", 0.3f },
            { "b_on", 1 }, { "b_model", OTT }, { "b_drive", 0.25f }, { "b_mix", 0.4f }, { "auto", 1 } } },
        { "Catena | Voce a 4 stadi", "Quattro stadi leggeri in serie: FET prende i picchi, OPTO livella, VCA incolla, OTT al 20% per la presenza. Ognuno lavora 1-3 dB.", 0,
          { { "a_model", FET76 }, { "a_drive", 0.22f }, { "a_ratio", 0 }, { "a_att", 0.55f }, { "a_rel", 0.3f },
            { "b_on", 1 }, { "b_model", OPTO2A }, { "b_drive", 0.3f }, { "c_on", 1 }, { "c_model", VCABUS }, { "c_drive", 0.25f }, { "c_ratio", 0 }, { "c_att", 0.8f }, { "c_rel", 1.0f },
            { "d_on", 1 }, { "d_model", OTT }, { "d_drive", 0.2f }, { "d_mix", 0.4f }, { "auto", 1 } } },
        { "Catena | Batteria a 4 stadi", "RMS 160 sulla cassa, FET sul transiente, VARI-MU per lo spessore, DISTRESS in parallelo al 25%.", 0,
          { { "a_model", RMS160 }, { "a_drive", 0.3f }, { "a_ratio", 2 }, { "b_on", 1 }, { "b_model", FET76 }, { "b_drive", 0.25f }, { "b_att", 0.2f }, { "b_rel", 0.25f },
            { "c_on", 1 }, { "c_model", VARIMU }, { "c_drive", 0.25f }, { "c_ratio", 1 }, { "d_on", 1 }, { "d_model", DISTRESS }, { "d_drive", 0.6f }, { "d_ratio", 6 }, { "d_mix", 0.25f }, { "auto", 1 } } },
        { "Catena | Voce da disco (76 > 2A > 670)", "Il FET ferma, l'opto livella, il 670 da il colore finale con 1 dB. Tre stadi leggeri: la voce piu rifinita possibile.", 0,
          { { "a_model", FET76 }, { "a_drive", 0.25f }, { "a_att", 0.55f }, { "a_rel", 0.3f }, { "a_color", 0.25f },
            { "b_on", 1 }, { "b_model", OPTO2A }, { "b_drive", 0.3f }, { "b_color", 0.25f },
            { "c_on", 1 }, { "c_model", VC670 }, { "c_drive", 0.15f }, { "c_att", 0.15f }, { "c_ratio", 1 }, { "c_color", 0.4f }, { "auto", 1 } } },
        { "Parallelo | Smash + Opto", "PARALLELO: A distrugge (NUKE), B livella morbido. Dosa i due MIX.", 2,
          { { "a_model", DISTRESS }, { "a_drive", 0.7f }, { "a_ratio", 7 }, { "a_att", 0.1f }, { "a_rel", 0.3f }, { "a_color", 0.6f }, { "a_mix", 0.6f },
            { "b_on", 1 }, { "b_model", OPTO2A }, { "b_drive", 0.4f }, { "auto", 1 } } },
        { "Parallelo | 670 + FET ALL", "PARALLELO: il 670 tiene il corpo, il FET in ALL aggiunge esplosione. Per batterie e room.", 2,
          { { "a_model", VC670 }, { "a_drive", 0.35f }, { "a_att", 0.35f }, { "a_ratio", 1 }, { "a_color", 0.5f },
            { "b_on", 1 }, { "b_model", FET76 }, { "b_drive", 0.65f }, { "b_ratio", 4 }, { "b_att", 0.8f }, { "b_rel", 0.2f }, { "b_mix", 0.5f }, { "auto", 1 } } } };
    return P;
}

void Processor::loadPreset (int i)
{
    if (! juce::isPositiveAndBelow (i, (int) presets().size())) return;
    preset = i;
    for (auto* p : getParameters()) if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p)) rp->setValueNotifyingHost (rp->getDefaultValue());
    const auto& P = presets()[(size_t) i];
    if (auto* rp = apvts.getParameter ("route")) rp->setValueNotifyingHost (rp->convertTo0to1 ((float) P.route));
    if (i > 0) if (auto* rp = apvts.getParameter ("a_on")) rp->setValueNotifyingHost (1.0f);   // i preset partono dallo stadio A acceso
    if (i > 0)   // i preset sono scritti sui valori di partenza della versione precedente
        for (int s = 0; s < kStages; ++s)
        {
            const juce::String p = juce::String (stagePfx (s)).dropLastCharacters (1);
            for (auto [k, v] : { std::pair<const char*, float> { "_drive", 0.3f }, { "_color", 0.3f }, { "_drive2", 0.3f }, { "_att2", 0.5f } })
                if (auto* rp = apvts.getParameter (p + k)) rp->setValueNotifyingHost (rp->convertTo0to1 (v));
        }
    if (i > 0) for (int b = 1; b <= 4; ++b) if (auto* rp = apvts.getParameter ("mb_b" + juce::String (b) + "_thr")) rp->setValueNotifyingHost (rp->convertTo0to1 (-20.0f));
    for (auto& [id, v] : P.v) if (auto* rp = apvts.getParameter (id)) rp->setValueNotifyingHost (rp->convertTo0to1 (v));
    if (i > 0) convertLegacy();   // i preset sono scritti con le soglie della versione precedente
}
// valori della versione precedente -> nuove soglie (stesso punto di intervento, uscita gia' compensata)
void Processor::convertLegacy()
{
    auto set = [this] (const juce::String& id, float v) { if (auto* rp = apvts.getParameter (id)) rp->setValueNotifyingHost (rp->convertTo0to1 (v)); };
    for (int s = 0; s < kStages; ++s)
    {
        const juce::String p = juce::String (stagePfx (s)).dropLastCharacters (1);
        const auto& q = pp[s];
        const int m = juce::roundToInt (q.model->load()), r = juce::roundToInt (q.ratio->load());
        set (p + "_drive", convOldDrive (m, r, q.drive->load()));
        if (m == VC670) { set (p + "_thr", convOld670Thr (q.att->load())); set (p + "_att2", convOld670Thr (q.att2->load())); }
        set (p + "_olink", 0);
    }
}

void Processor::applyPlan()
{
    if (! plan.valid) return;
    auto set = [&] (const juce::String& id, float v) { if (auto* rp = apvts.getParameter (id)) rp->setValueNotifyingHost (rp->convertTo0to1 (v)); };
    set ("route", 0); set ("auto", 1); set ("mix", 1); set ("out", 0); set ("scSrc", 0); set ("scTo", 0); set ("scHpf", plan.scHpf); set ("scListen", 0);
    for (int k = 0; k < kStages; ++k)
    {
        const juce::String p = juce::String (stagePfx (k)).dropLastCharacters (1);
        if (k >= (int) plan.stages.size()) { set (p + "_on", 0); set (p + "_solo", 0); continue; }
        const auto& q = plan.stages[(size_t) k];
        set (p + "_on", 1); set (p + "_solo", 0); set (p + "_model", (float) q.model); set (p + "_ratio", (float) q.ratio); set (p + "_att", q.att); set (p + "_rel", q.rel);
        set (p + "_drive", convOldDrive (q.model, q.ratio, q.drive)); set (p + "_color", q.color); set (p + "_mix", q.mix); set (p + "_out", 0);
        calibT[k] = q.target;
    }
    calib = plan.stages.empty() ? -1 : 0; calibOk = 0; calibSteps = 0; calibBuf.clear();
    calibMsg = "Taratura: fai suonare la traccia, regolo le soglie da solo.";
}
void Processor::calibTick()
{
    if (calib < 0 || calib >= (int) plan.stages.size()) return;
    const float inDb = juce::Decibels::gainToDecibels (std::sqrt (inRms.load()), -100.0f);
    if (inDb < -50) { calibMsg = "Taratura in pausa: aspetto che la traccia suoni."; return; }
    calibBuf.push_back (gr[calib].load());
    if (calibBuf.size() < 24) return;                      // ~0.8 s di riduzione, poi correggo
    std::sort (calibBuf.begin(), calibBuf.end());
    const float g = calibBuf[(size_t) (calibBuf.size() * 3 / 4)];   // quanto lavora sui passaggi forti
    calibBuf.clear();
    const auto& q = plan.stages[(size_t) calib];
    const juce::String id = juce::String (stagePfx (calib)).dropLastCharacters (1) + "_drive";
    auto* rp = apvts.getParameter (id); const float d = value (id.toRawUTF8());
    const float err = calibT[calib] - g; ++calibSteps;
    const juce::String nm = juce::String (calib + 1) + " (" + info (q.model).name + ")";
    if (std::abs (err) < 0.7f) ++calibOk; else calibOk = 0;
    if (std::abs (err) >= 0.7f && rp != nullptr)
    {
        const float nd = juce::jlimit (0.0f, 1.0f, d + juce::jlimit (-0.08f, 0.08f, err * 0.03f));
        rp->setValueNotifyingHost (rp->convertTo0to1 (nd));
        if ((nd >= 1.0f && err > 0) || (nd <= 0.0f && err < 0))
        {   // a fine corsa: tengo il massimo possibile e lo dico
            calibOk = 3;
            plan.notes.add (nd <= 0.0f ? "Stadio " + nm + ": lavora troppo anche con " + juce::String (info (q.model).driveLabel) + " a zero (" + juce::String (g, 1) + " dB). Abbassa il volume prima del plugin di " + juce::String (juce::roundToInt (-err * 1.5f)) + "-" + juce::String (juce::roundToInt (-err * 2.5f)) + " dB, poi rifai APPLICA."
                                       : "Stadio " + nm + ": anche al massimo arriva a " + juce::String (g, 1) + " dB. Il segnale e' basso: alzalo prima del plugin.");
        }
    }
    calibMsg = "Taratura stadio " + nm + ": " + juce::String (g, 1) + " dB su " + juce::String (calibT[calib], 1) + " voluti.";
    if (calibOk >= 3 || calibSteps > 40)
    {
        calibMsg = "Stadio " + nm + " tarato a " + juce::String (g, 1) + " dB.";
        ++calib; calibOk = 0; calibSteps = 0;
        if (calib >= (int) plan.stages.size()) { calib = -1; calibMsg = "Rack tarato. AUTO GAIN acceso: il confronto col bypass e' allo stesso volume."; }
    }
}

void Processor::getStateInformation (juce::MemoryBlock& d)
{
    auto s = apvts.copyState(); s.setProperty ("preset", preset, nullptr); s.setProperty ("thrv", 2, nullptr);
    if (auto x = s.createXml()) copyXmlToBinary (*x, d);
}
void Processor::setStateInformation (const void* data, int size)
{
    if (auto x = getXmlFromBinary (data, size)) { auto t = juce::ValueTree::fromXml (*x); if (t.isValid()) { preset = t.getProperty ("preset", 0); const bool old = (int) t.getProperty ("thrv", 1) < 2; apvts.replaceState (t); if (old) convertLegacy(); } }
}

juce::AudioProcessorEditor* Processor::createEditor() { return new Editor (*this); }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new jc::Processor(); }
