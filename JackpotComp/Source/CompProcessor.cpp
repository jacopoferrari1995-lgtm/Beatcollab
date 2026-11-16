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
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { p + "_out", 1 }, P + "Output", NormalisableRange<float> (-40, 24), 0));
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { p + "_color", 1 }, P + "Color", NormalisableRange<float> (0, 1), 0.0f));
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { p + "_mix", 1 }, P + "Mix", NormalisableRange<float> (0, 1), 1));
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { p + "_drive2", 1 }, P + "Input R/Vert", NormalisableRange<float> (0, 1), 0.0f));
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { p + "_att2", 1 }, P + "Threshold R/Vert", NormalisableRange<float> (0, 1), 0.0f));
        l.add (std::make_unique<AudioParameterInt> (ParameterID { p + "_ratio2", 1 }, P + "Time Const R/Vert", 0, 7, 0));
        l.add (std::make_unique<AudioParameterBool> (ParameterID { p + "_link", 1 }, P + "Link", true));
        l.add (std::make_unique<AudioParameterBool> (ParameterID { p + "_olink", 1 }, P + "Output Link", false));
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { p + "_thr", 1 }, P + "Threshold (670/Vari-Mu)", NormalisableRange<float> (0, 1), 0.0f));
        { NormalisableRange<float> r (1, 60); r.setSkewForCentre (4); l.add (std::make_unique<AudioParameterFloat> (ParameterID { p + "_cratio", 1 }, P + "Ratio (160/SSL)", r, 4.0f)); }
        l.add (std::make_unique<AudioParameterInt> (ParameterID { p + "_mode", 1 }, P + "Mode (SSL att / Dist)", 0, 3, 0));
        l.add (std::make_unique<AudioParameterBool> (ParameterID { p + "_hp", 1 }, P + "HP detector", false));
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
        q.drive2 = R (p + "_drive2"); q.att2 = R (p + "_att2"); q.ratio2 = R (p + "_ratio2"); q.link = R (p + "_link"); q.olink = R (p + "_olink"); q.thr = R (p + "_thr"); q.cratio = R (p + "_cratio"); q.mode = R (p + "_mode"); q.hp = R (p + "_hp");
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
    // tutto il rack gira a 4x: la saturazione (VALVOLA, GRINTA, DIST...) non crea piu' aliasing e il rilevatore vede i picchi veri
    osMax = juce::jmax (64, maxBlock);
    os = std::make_unique<juce::dsp::Oversampling<float>> (2, kOsStages, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, true);
    osSc = std::make_unique<juce::dsp::Oversampling<float>> (2, kOsStages, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, true);
    os->initProcessing ((size_t) osMax); osSc->initProcessing ((size_t) osMax);
    latency = juce::roundToInt (os->getLatencyInSamples());
    setLatencySamples (latency);
    const int n4 = osMax * kOs;
    for (auto& x : st) x.prepare (s * kOs);
    cap.prepare (s);
    for (auto* b : { &dry, &det, &aux, &acc, &dlt, &tmp }) b->setSize (2, n4);
    inB.setSize (2, osMax); scB.setSize (2, osMax); dryD.setSize (2, osMax);
    dly.setSize (2, latency + osMax + 1); dly.clear(); dlyW = 0;
    for (auto& a : hp) for (auto& f : a) f.reset();
    lastHp = -1;
    sMix.reset (s, 0.02); sOut.reset (s, 0.02); sMix.setCurrentAndTargetValue (gMix->load()); sOut.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (gOut->load()));
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
    q.drive2 = p.drive2->load(); q.att2 = p.att2->load(); q.ratio2 = juce::jlimit (0, 5, juce::roundToInt (p.ratio2->load()));
    q.link = p.link->load() > 0.5f; q.outLink = false; q.thr670 = p.thr->load();
    q.cratio = p.cratio->load(); q.mode = juce::roundToInt (p.mode->load()); q.hp = p.hp->load() > 0.5f;
    q.mb = &mbNow;
    return q;
}

// ritardo pari alla latenza dell'oversampling: il segnale pulito (MIX generale, bypass) resta allineato al trattato
void Processor::delayDry (const float* L, const float* R, float* oL, float* oR, int N)
{
    const int D = dly.getNumSamples(); float* d0 = dly.getWritePointer (0); float* d1 = dly.getWritePointer (1);
    for (int i = 0; i < N; ++i)
    {
        d0[dlyW] = L[i]; d1[dlyW] = R[i];
        int r = dlyW - latency; if (r < 0) r += D;
        oL[i] = d0[r]; oR[i] = d1[r];
        if (++dlyW >= D) dlyW = 0;
    }
}
void Processor::processBlockBypassed (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    auto main = getBusBuffer (buffer, true, 0);
    const int N = buffer.getNumSamples(), nch = juce::jmin (2, main.getNumChannels());
    if (nch == 0 || dly.getNumSamples() == 0) return;
    float* L = main.getWritePointer (0); float* R = nch > 1 ? main.getWritePointer (1) : nullptr;
    for (int o = 0; o < N; o += osMax)
    {
        const int n = juce::jmin (osMax, N - o);
        float* tl = inB.getWritePointer (0); float* tr = inB.getWritePointer (1);
        delayDry (L + o, R ? R + o : L + o, tl, tr, n);
        juce::FloatVectorOperations::copy (L + o, tl, n); if (R) juce::FloatVectorOperations::copy (R + o, tr, n);
    }
    for (int c = 2; c < buffer.getNumChannels(); ++c) buffer.clear (c, 0, N);
}

void Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals nd;
    auto main = getBusBuffer (buffer, true, 0);
    const int N = buffer.getNumSamples(), nch = juce::jmin (2, main.getNumChannels());
    if (nch == 0 || os == nullptr) return;
    float* L = main.getWritePointer (0); float* R = nch > 1 ? main.getWritePointer (1) : nullptr;
    const float* eL = nullptr; const float* eR = nullptr;
    if (getBusCount (true) > 1 && getBus (true, 1)->isEnabled())
    {
        auto sc = getBusBuffer (buffer, true, 1);
        if (sc.getNumChannels() > 0) { eL = sc.getReadPointer (0); eR = sc.getReadPointer (juce::jmin (1, sc.getNumChannels() - 1)); }
    }
    extSc = eL != nullptr;
    // a pezzi grandi quanto quello preparato: funziona con qualunque dimensione di buffer
    for (int o = 0; o < N; o += osMax)
    {
        const int n = juce::jmin (osMax, N - o);
        processChunk (L + o, R ? R + o : nullptr, eL ? eL + o : nullptr, eR ? eR + o : nullptr, n);
    }
    for (int c = 2; c < buffer.getNumChannels(); ++c) buffer.clear (c, 0, N);
}

void Processor::processChunk (float* L, float* R, const float* eL0, const float* eR0, int N)
{
    float* iL = inB.getWritePointer (0); float* iR = inB.getWritePointer (1);
    juce::FloatVectorOperations::copy (iL, L, N); juce::FloatVectorOperations::copy (iR, R ? R : L, N);
    // misura ingresso e ASCOLTO alla frequenza normale
    float pkI[2] = { 0, 0 }; double eIn = 0;
    for (int i = 0; i < N; ++i) { pkI[0] = juce::jmax (pkI[0], std::abs (iL[i])); pkI[1] = juce::jmax (pkI[1], std::abs (iR[i])); eIn += 0.5 * (iL[i] * iL[i] + iR[i] * iR[i]); }
    cap.push (iL, iR, N);
    // pulito ritardato per il MIX generale
    float* dryOutL = dryD.getWritePointer (0); float* dryOutR = dryD.getWritePointer (1);
    delayDry (iL, iR, dryOutL, dryOutR, N);

    // su a 4x
    juce::dsp::AudioBlock<float> inBlk (inB.getArrayOfWritePointers(), 2, (size_t) N);
    auto up = os->processSamplesUp (inBlk);
    const int N4 = (int) up.getNumSamples();
    float* uL = up.getChannelPointer (0); float* uR = up.getChannelPointer (1);
    const float* eL = nullptr; const float* eR = nullptr;
    if (eL0 != nullptr)
    {
        juce::FloatVectorOperations::copy (scB.getWritePointer (0), eL0, N); juce::FloatVectorOperations::copy (scB.getWritePointer (1), eR0, N);
        juce::dsp::AudioBlock<float> sb (scB.getArrayOfWritePointers(), 2, (size_t) N);
        auto su = osSc->processSamplesUp (sb); eL = su.getChannelPointer (0); eR = su.getChannelPointer (1);
    }
    const bool useExt = gScSrc->load() > 0.5f && eL != nullptr;
    const int scTo = juce::roundToInt (gScTo->load());
    float* dry4L = dry.getWritePointer (0); float* dry4R = dry.getWritePointer (1);
    juce::FloatVectorOperations::copy (dry4L, uL, N4); juce::FloatVectorOperations::copy (dry4R, uR, N4);

    const double sr4 = sr * kOs;
    const float hpF = gScHpf->load();
    if (std::abs (hpF - lastHp) > 0.5f)
    {
        auto c = juce::dsp::IIR::Coefficients<float>::makeHighPass (sr4, juce::jmax (20.0f, hpF), 0.707f);
        for (auto& a : hp) for (auto& f : a) f.coefficients = c;
        lastHp = hpF;
    }
    auto makeDet = [&] (int s, const float* sl, const float* sr2)
    {
        const bool ext = useExt && (scTo == 0 || scTo == s + 1);
        float* dl = det.getWritePointer (0); float* dr = det.getWritePointer (1);
        juce::FloatVectorOperations::copy (dl, ext ? eL : sl, N4); juce::FloatVectorOperations::copy (dr, ext ? eR : sr2, N4);
        if (hpF > 21) for (int i = 0; i < N4; ++i) { dl[i] = hp[s][0].processSample (dl[i]); dr[i] = hp[s][1].processSample (dr[i]); }
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
    float* tmpL = tmp.getWritePointer (0); float* tmpR = tmp.getWritePointer (1);
    if (anyS) { juce::FloatVectorOperations::clear (dlL, N4); juce::FloatVectorOperations::clear (dlR, N4); }
    if (parallel && nOn > 0) { juce::FloatVectorOperations::clear (aL, N4); juce::FloatVectorOperations::clear (aR, N4); }
    for (int s = 0; s < kStages; ++s)
    {
        if (! sp[s].on) { st[s].reset(); stIn[s] = 0.0f; continue; }
        float* pl = uL; float* pr = uR;
        if (parallel) { juce::FloatVectorOperations::copy (xL, dry4L, N4); juce::FloatVectorOperations::copy (xR, dry4R, N4); pl = xL; pr = xR; }
        makeDet (s, pl, pr);
        {   // livello che entra nel rilevatore di questo stadio: serve per mostrare la soglia
            const auto ra = juce::FloatVectorOperations::findMinAndMax (det.getReadPointer (0), N4), rb = juce::FloatVectorOperations::findMinAndMax (det.getReadPointer (1), N4);
            const float pk = juce::jmax (ra.getEnd(), -ra.getStart(), juce::jmax (rb.getEnd(), -rb.getStart()));
            stIn[s] = juce::jmax (pk, stIn[s].load() * std::exp (-(float) N / (0.35f * (float) sr)));
        }
        const bool wantD = solo[s];
        st[s].process (pl, pr, det.getReadPointer (0), det.getReadPointer (1), N4, sp[s], autoG, wantD ? tmpL : nullptr, wantD ? tmpR : nullptr);
        if (wantD) { juce::FloatVectorOperations::add (dlL, tmpL, N4); juce::FloatVectorOperations::add (dlR, tmpR, N4); }
        if (parallel) { juce::FloatVectorOperations::add (aL, xL, N4); juce::FloatVectorOperations::add (aR, xR, N4); }
    }
    if (parallel && nOn > 0) { const float k = 1.0f / (float) nOn; for (int i = 0; i < N4; ++i) { uL[i] = aL[i] * k; uR[i] = aR[i] * k; } }
    const bool listenSc = gScListen->load() > 0.5f;
    if (listenSc)
    {
        makeDet (0, dry4L, dry4R);
        juce::FloatVectorOperations::copy (uL, det.getReadPointer (0), N4); juce::FloatVectorOperations::copy (uR, det.getReadPointer (1), N4);
    }
    else if (anyS)
    {   // solo: si sente solo cio che gli stadi in solo stanno togliendo
        juce::FloatVectorOperations::copy (uL, dlL, N4); juce::FloatVectorOperations::copy (uR, dlR, N4);
    }
    // giu' a 1x, direttamente nel buffer di uscita
    float* outs[2] = { L, R ? R : tmpL };
    juce::dsp::AudioBlock<float> outBlk (outs, 2, (size_t) N);
    os->processSamplesDown (outBlk);
    float* oR = R ? R : tmpL;
    if (! listenSc && ! anyS)
    {
        sMix.setTargetValue (gMix->load()); sOut.setTargetValue (juce::Decibels::decibelsToGain (gOut->load()));
        for (int i = 0; i < N; ++i) { const float m = sMix.getNextValue(), og = sOut.getNextValue(); L[i] = (dryOutL[i] * (1 - m) + L[i] * m) * og; oR[i] = (dryOutR[i] * (1 - m) + oR[i] * m) * og; }
    }

    // misure
    float pkO[2] = { 0, 0 }; double eOut = 0;
    for (int i = 0; i < N; ++i) { pkO[0] = juce::jmax (pkO[0], std::abs (L[i])); pkO[1] = juce::jmax (pkO[1], std::abs (oR[i])); eOut += 0.5 * (L[i] * L[i] + oR[i] * oR[i]); }
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
}

// ======================= preset =======================
const std::vector<Preset>& presets()
{
    // valori della versione 3 (come l'hardware): FET/DISTRESS drive = INPUT/48 dB, OUT riporta il volume; VARI-MU drive = INPUT/24, _thr = -30 dB;
    // 670 drive = INPUT/18, _thr = -28 dB; soglie dei modelli a THRESHOLD = -40 dB x drive. FET att/rel: 0 = 1 (lento) .. 1 = 7 (veloce)
    // Tarati per tracce singole con picchi intorno a -6 dBFS, bus batteria -3, master -1.
    static const std::vector<Preset> P = {
        { "Init", "Tutto spento: accendi uno stadio dalla fila in alto. Ogni stadio parte neutro.", 0, {} },
        // ================= VOCE
        { "Voce | Rap lead (76 > 2A)", "La catena classica: il FET prende solo i picchi (3-5 dB, attacco 3 per lasciar passare le consonanti, rilascio 6), l'OPTO livella le frasi (2-3 dB). Voce davanti senza sentirsi compressa.", 0,
          { { "a_model", FET76 }, { "a_drive", 0.25f }, { "a_out", -8 }, { "a_ratio", 0 }, { "a_att", 0.33f }, { "a_rel", 0.83f }, { "a_color", 0.25f },
            { "b_on", 1 }, { "b_model", OPTO2A }, { "b_drive", 0.45f }, { "b_color", 0.25f }, { "auto", 1 } } },
        { "Voce | Rap lead con controllo S", "76 > 2A e in fondo un multibanda solo sopra 6 kHz (attacco 2 ms): le S e le T non escono dopo la compressione.", 0,
          { { "a_model", FET76 }, { "a_drive", 0.25f }, { "a_out", -8 }, { "a_att", 0.33f }, { "a_rel", 0.83f }, { "a_color", 0.2f },
            { "b_on", 1 }, { "b_model", OPTO2A }, { "b_drive", 0.42f }, { "b_color", 0.2f },
            { "c_on", 1 }, { "c_model", MULTI }, { "c_ratio", 0 }, { "mb_x2", 6000 }, { "mb_b1_thr", 0 }, { "mb_b2_thr", -32 }, { "mb_b2_ratio", 4 }, { "mb_b2_att", 2 }, { "mb_b2_rel", 60 }, { "auto", 1 } } },
        { "Voce | Melodica morbida (2A)", "Solo ottico: livellamento trasparente per voci cantate e ritornelli. Il 2A sente di piu' gli acuti: un filo di de-essing gratis.", 0,
          { { "a_model", OPTO2A }, { "a_drive", 0.5f }, { "a_color", 0.3f }, { "auto", 1 } } },
        { "Voce | Melodica calda (670)", "VARI 670 in TC2 con 2-4 dB: la voce si arrotonda, prende corpo sotto e brillantezza sopra. Il suono dei dischi soul.", 0,
          { { "a_model", VC670 }, { "a_drive", 0.22f }, { "a_thr", 0.55f }, { "a_ratio", 1 }, { "a_color", 0.45f }, { "auto", 1 } } },
        { "Voce | Lead su beat pieno (76 > 670)", "Il FET ferma i picchi, il 670 incolla e scalda. Per la voce principale che deve stare sopra un beat denso.", 0,
          { { "a_model", FET76 }, { "a_drive", 0.21f }, { "a_out", -7 }, { "a_att", 0.33f }, { "a_rel", 0.67f }, { "a_color", 0.3f },
            { "b_on", 1 }, { "b_model", VC670 }, { "b_drive", 0.2f }, { "b_thr", 0.5f }, { "b_ratio", 1 }, { "b_color", 0.4f }, { "auto", 1 } } },
        { "Voce | Trap aggressiva (Distress)", "Distress 4:1 con DIST 3 e HP sul rilevatore: graffia e sta sopra l'808 senza pompare sui bassi.", 0,
          { { "a_model", DISTRESS }, { "a_drive", 0.375f }, { "a_out", -12 }, { "a_ratio", 3 }, { "a_att", 0.35f }, { "a_rel", 0.35f }, { "a_mode", 2 }, { "a_hp", 1 }, { "auto", 1 } } },
        { "Voce | Drill dura (76 ALL > SSL)", "FET in ALL al 70% di MIX per l'aggressivita', poi il canale SSL con FAST ATT che tiene tutto in riga.", 0,
          { { "a_model", FET76 }, { "a_drive", 0.33f }, { "a_out", -10 }, { "a_ratio", 4 }, { "a_att", 0.5f }, { "a_rel", 0.83f }, { "a_color", 0.4f }, { "a_mix", 0.7f },
            { "b_on", 1 }, { "b_model", SSLCH }, { "b_drive", 0.35f }, { "b_cratio", 3 }, { "b_mode", 1 }, { "b_rel", 0.3f }, { "auto", 1 } } },
        { "Voce | Adlib schiacciati", "FET in ALL spinto: adlib piatti e distorti, da mettere sotto la voce principale.", 0,
          { { "a_model", FET76 }, { "a_drive", 0.5f }, { "a_out", -16 }, { "a_ratio", 4 }, { "a_att", 0.67f }, { "a_rel", 1.0f }, { "a_color", 0.5f }, { "auto", 1 } } },
        { "Voce | Adlib lontani (76 > OTT)", "FET per fermarli, OTT a meta' per farli brillare e respirare dietro la voce.", 0,
          { { "a_model", FET76 }, { "a_drive", 0.29f }, { "a_out", -10 }, { "a_ratio", 1 }, { "a_att", 0.5f }, { "a_rel", 0.83f },
            { "b_on", 1 }, { "b_model", OTT }, { "b_drive", 0.4f }, { "b_mix", 0.5f }, { "auto", 1 } } },
        { "Voce | Doppie e cori (670 LAT/VERT)", "670 in LAT/VERT: comprime il centro e lascia i lati aperti. I cori restano larghi senza scavalcare la voce.", 0,
          { { "a_model", VC670 }, { "a_drive", 0.2f }, { "a_thr", 0.5f }, { "a_rel", 1.0f }, { "a_ratio", 2 }, { "a_color", 0.3f }, { "auto", 1 } } },
        { "Voce | Back vocal incollati (VCA)", "VCA 4:1, attacco 3 ms, rilascio AUTO: le voci di sotto stanno ferme e si incollano tra loro.", 0,
          { { "a_model", VCABUS }, { "a_drive", 0.5f }, { "a_ratio", 1 }, { "a_att", 0.6f }, { "a_rel", 1.0f }, { "a_color", 0.15f }, { "auto", 1 } } },
        { "Voce | Ritornello largo (Vari-Mu)", "Vari-Mu in COMPRESS, attacco medio, RECOVERY 0.6 s: la voce si allarga e si scalda senza perdere vita.", 0,
          { { "a_model", VARIMU }, { "a_drive", 0.25f }, { "a_thr", 0.4f }, { "a_ratio", 0 }, { "a_att", 0.3f }, { "a_rel", 0.5f }, { "a_color", 0.4f }, { "auto", 1 } } },
        { "Voce | Fango e S (multibanda)", "Multibanda a 4 bande: 200-600 Hz controllato quando la voce si gonfia, sopra 6 kHz per le S. Il resto quasi libero.", 0,
          { { "a_model", MULTI }, { "a_ratio", 2 }, { "mb_x1", 200 }, { "mb_x2", 600 }, { "mb_x3", 6000 },
            { "mb_b1_thr", -30 }, { "mb_b1_ratio", 2 }, { "mb_b2_thr", -26 }, { "mb_b2_ratio", 3 }, { "mb_b2_att", 15 }, { "mb_b2_rel", 120 },
            { "mb_b3_thr", -12 }, { "mb_b3_ratio", 1.5f }, { "mb_b4_thr", -30 }, { "mb_b4_ratio", 5 }, { "mb_b4_att", 1.5f }, { "mb_b4_rel", 50 }, { "auto", 1 } } },
        { "Voce | Parlato e podcast (3A > 160)", "LA-3A per il livello, 160 a 2.5:1 per tenere tutto alla stessa distanza dal microfono.", 0,
          { { "a_model", OPTO3A }, { "a_drive", 0.45f }, { "a_color", 0.15f }, { "b_on", 1 }, { "b_model", RMS160 }, { "b_drive", 0.55f }, { "b_cratio", 2.5f }, { "auto", 1 } } },
        { "Voce | Da disco (76 > 2A > 670)", "Il FET ferma, l'opto livella, il 670 da' il colore finale con 1-2 dB. Tre stadi leggeri: la voce piu' rifinita possibile.", 0,
          { { "a_model", FET76 }, { "a_drive", 0.21f }, { "a_out", -7 }, { "a_att", 0.33f }, { "a_rel", 0.83f }, { "a_color", 0.25f },
            { "b_on", 1 }, { "b_model", OPTO2A }, { "b_drive", 0.4f }, { "b_color", 0.25f },
            { "c_on", 1 }, { "c_model", VC670 }, { "c_drive", 0.15f }, { "c_thr", 0.35f }, { "c_ratio", 1 }, { "c_color", 0.4f }, { "auto", 1 } } },
        { "Voce | Catena a 4 stadi", "FET sui picchi, OPTO livella, VCA incolla, OTT al 40% di MIX per la presenza. Ognuno lavora 1-3 dB.", 0,
          { { "a_model", FET76 }, { "a_drive", 0.19f }, { "a_out", -6 }, { "a_ratio", 0 }, { "a_att", 0.33f }, { "a_rel", 0.83f },
            { "b_on", 1 }, { "b_model", OPTO2A }, { "b_drive", 0.4f }, { "c_on", 1 }, { "c_model", VCABUS }, { "c_drive", 0.4f }, { "c_ratio", 0 }, { "c_att", 0.8f }, { "c_rel", 1.0f },
            { "d_on", 1 }, { "d_model", OTT }, { "d_drive", 0.2f }, { "d_mix", 0.4f }, { "auto", 1 } } },
        // ================= BATTERIA, STRUMENTO PER STRUMENTO
        { "Cassa | Punch (160)", "dbx 160 a 4:1: la cassa resta compatta e il 160 lascia passare i primi ms dell'attacco.", 0,
          { { "a_model", RMS160 }, { "a_drive", 0.35f }, { "a_cratio", 4 }, { "a_color", 0.2f }, { "auto", 1 } } },
        { "Cassa | Attacco (SSL)", "Canale SSL con attacco automatico: piu' la cassa picchia piu' l'attacco si accorcia. Rilascio corto per riaprire prima del colpo dopo.", 0,
          { { "a_model", SSLCH }, { "a_drive", 0.4f }, { "a_cratio", 4 }, { "a_mode", 0 }, { "a_rel", 0.2f }, { "auto", 1 } } },
        { "Cassa | Boom bap spessa (670)", "670 in TC1 con il trasformatore spinto: la cassa si gonfia sotto e perde l'asprezza.", 0,
          { { "a_model", VC670 }, { "a_drive", 0.4f }, { "a_thr", 0.5f }, { "a_ratio", 0 }, { "a_color", 0.55f }, { "auto", 1 } } },
        { "Rullante | Crack (76 lento > 3A)", "FET con attacco 1 (il piu' lento) per far passare il colpo, poi il 3A per tenerlo fermo.", 0,
          { { "a_model", FET76 }, { "a_drive", 0.25f }, { "a_out", -8 }, { "a_ratio", 0 }, { "a_att", 0.0f }, { "a_rel", 0.83f },
            { "b_on", 1 }, { "b_model", OPTO3A }, { "b_drive", 0.45f }, { "auto", 1 } } },
        { "Rullante | Grasso (Distress DIST 2)", "Distress 4:1 con DIST 2: il rullante prende corpo e un po' di sporco da nastro.", 0,
          { { "a_model", DISTRESS }, { "a_drive", 0.33f }, { "a_out", -11 }, { "a_ratio", 3 }, { "a_att", 0.55f }, { "a_rel", 0.4f }, { "a_mode", 1 }, { "auto", 1 } } },
        { "Rullante | Trap secco (SSL)", "SSL 6:1 con FAST ATT e rilascio corto: rullante corto, secco, sempre uguale.", 0,
          { { "a_model", SSLCH }, { "a_drive", 0.45f }, { "a_cratio", 6 }, { "a_mode", 1 }, { "a_rel", 0.25f }, { "auto", 1 } } },
        { "Hat | Dinamica ferma (VCA)", "VCA 4:1 velocissimo: gli hat smettono di saltare tra un colpo e l'altro.", 0,
          { { "a_model", VCABUS }, { "a_drive", 0.4f }, { "a_ratio", 1 }, { "a_att", 0.2f }, { "a_rel", 0.0f }, { "auto", 1 } } },
        { "Hat | Lucidi (OTT)", "OTT a meta' MIX: hat brillanti e densi senza perdere del tutto la dinamica.", 0,
          { { "a_model", OTT }, { "a_drive", 0.35f }, { "a_mix", 0.5f }, { "a_color", 0.1f }, { "auto", 1 } } },
        { "Percussioni | Avanti (76)", "FET 8:1, attacco 4, rilascio 6: percussioni che bucano il mix.", 0,
          { { "a_model", FET76 }, { "a_drive", 0.29f }, { "a_out", -10 }, { "a_ratio", 1 }, { "a_att", 0.5f }, { "a_rel", 0.83f }, { "a_color", 0.2f }, { "auto", 1 } } },
        { "Room | Esplosa (76 ALL)", "Il trucco del 1176 sulla room: tutti i pulsanti, rilascio 7 che pompa, al 40% di MIX.", 0,
          { { "a_model", FET76 }, { "a_drive", 0.58f }, { "a_out", -18 }, { "a_ratio", 4 }, { "a_att", 0.5f }, { "a_rel", 1.0f }, { "a_color", 0.6f }, { "a_mix", 0.4f }, { "auto", 1 } } },
        { "Overhead | Controllo (VCA)", "VCA 2:1, attacco 10 ms, AUTO: piatti in riga senza schiacciare i transienti.", 0,
          { { "a_model", VCABUS }, { "a_drive", 0.35f }, { "a_ratio", 0 }, { "a_att", 0.8f }, { "a_rel", 1.0f }, { "auto", 1 } } },
        // ================= 808 E BASSO
        { "808 | Costante (160)", "160 a 3:1 per livellare l'808 nota per nota.", 0,
          { { "a_model", RMS160 }, { "a_drive", 0.45f }, { "a_cratio", 3 }, { "a_color", 0.25f }, { "auto", 1 } } },
        { "808 | Distorto (Distress DIST 2)", "Distress con DIST 2: l'808 si sente anche sulle casse piccole.", 0,
          { { "a_model", DISTRESS }, { "a_drive", 0.3f }, { "a_out", -10 }, { "a_ratio", 3 }, { "a_att", 0.6f }, { "a_rel", 0.45f }, { "a_mode", 1 }, { "auto", 1 } } },
        { "808 | Sub fermo (2 bande)", "Multibanda a 2 bande a 120 Hz: il sub tenuto fermo (6:1), la parte che fa sentire la nota libera.", 0,
          { { "a_model", MULTI }, { "a_ratio", 0 }, { "mb_x2", 120 }, { "mb_b1_thr", -20 }, { "mb_b1_ratio", 6 }, { "mb_b1_att", 20 }, { "mb_b1_rel", 250 }, { "mb_b2_thr", -10 }, { "mb_b2_ratio", 1.5f }, { "mb_b2_gain", 1.5f }, { "auto", 1 } } },
        { "808 | In faccia (76 > OTT)", "FET per tenere il picco, poi OTT leggero: l'808 si sente anche su telefono.", 0,
          { { "a_model", FET76 }, { "a_drive", 0.25f }, { "a_out", -8 }, { "a_ratio", 0 }, { "a_att", 0.17f }, { "a_rel", 0.83f },
            { "b_on", 1 }, { "b_model", OTT }, { "b_drive", 0.25f }, { "b_mix", 0.4f }, { "auto", 1 } } },
        { "Basso | Elettrico (2A > 670)", "Opto per il livello, 670 per il calore del trasformatore sui bassi. Il basso suonato che si incolla al beat.", 0,
          { { "a_model", OPTO2A }, { "a_drive", 0.5f }, { "a_color", 0.3f }, { "b_on", 1 }, { "b_model", VC670 }, { "b_drive", 0.22f }, { "b_thr", 0.5f }, { "b_ratio", 2 }, { "b_color", 0.55f }, { "auto", 1 } } },
        // ================= BUS BATTERIA
        { "Batteria | Drum bus colla (VCA)", "VCA 4:1, attacco 30 ms per i transienti, rilascio AUTO: 2-4 dB di colla.", 0,
          { { "a_model", VCABUS }, { "a_drive", 0.3f }, { "a_ratio", 1 }, { "a_att", 1.0f }, { "a_rel", 1.0f }, { "a_color", 0.2f }, { "auto", 1 } } },
        { "Batteria | Drum bus spesso (670)", "670 TC2: picchi arrotondati, coda che si alza, bassi gonfiati dal trasformatore. Boom bap e soul.", 0,
          { { "a_model", VC670 }, { "a_drive", 0.3f }, { "a_thr", 0.55f }, { "a_ratio", 1 }, { "a_color", 0.55f }, { "auto", 1 } } },
        { "Batteria | Parallelo New York (Distress)", "Distress schiacciato con DIST 3 e MIX al 35%: il pulito tiene i transienti, il compresso da' corpo e coda.", 0,
          { { "a_model", DISTRESS }, { "a_drive", 0.625f }, { "a_out", -20 }, { "a_ratio", 4 }, { "a_att", 0.3f }, { "a_rel", 0.3f }, { "a_mode", 2 }, { "a_mix", 0.35f }, { "auto", 1 } } },
        { "Batteria | Boom bap a 3 bande", "Multibanda a 3 bande: basse tenute, medie che respirano, alte addomesticate. WARM al 40%.", 0,
          { { "a_model", MULTI }, { "a_ratio", 1 }, { "a_color", 0.4f }, { "mb_x1", 150 }, { "mb_x3", 4500 },
            { "mb_b1_thr", -22 }, { "mb_b1_ratio", 4 }, { "mb_b1_att", 25 }, { "mb_b1_rel", 180 }, { "mb_b2_thr", -20 }, { "mb_b2_ratio", 2 }, { "mb_b2_att", 15 }, { "mb_b2_rel", 120 },
            { "mb_b3_thr", -24 }, { "mb_b3_ratio", 3 }, { "mb_b3_att", 4 }, { "mb_b3_rel", 90 }, { "auto", 1 } } },
        { "Batteria | Trap bus pulito (VCA > MB)", "VCA 4:1 attacco 10 ms rilascio 0.1 s, poi multibanda leggero sopra 5 kHz: hat lucidi e 808 fermo.", 0,
          { { "a_model", VCABUS }, { "a_drive", 0.3f }, { "a_ratio", 1 }, { "a_att", 0.8f }, { "a_rel", 0.0f }, { "a_color", 0.1f },
            { "b_on", 1 }, { "b_model", MULTI }, { "b_ratio", 0 }, { "mb_x2", 5000 }, { "mb_b1_thr", 0 }, { "mb_b2_thr", -24 }, { "mb_b2_ratio", 2.5f }, { "mb_b2_att", 3 }, { "mb_b2_rel", 80 }, { "auto", 1 } } },
        { "Batteria | Lo-fi Vari-Mu", "Vari-Mu in LIMIT spinto con tanta valvola: batteria spessa e sporca.", 0,
          { { "a_model", VARIMU }, { "a_drive", 0.58f }, { "a_thr", 0.3f }, { "a_ratio", 1 }, { "a_att", 0.1f }, { "a_rel", 0.25f }, { "a_color", 0.7f }, { "a_out", -8 }, { "auto", 1 } } },
        { "Batteria | Catena a 4 stadi", "160 sulla cassa, FET sul transiente, Vari-Mu per lo spessore, Distress schiacciato al 25%.", 0,
          { { "a_model", RMS160 }, { "a_drive", 0.3f }, { "a_cratio", 3 }, { "b_on", 1 }, { "b_model", FET76 }, { "b_drive", 0.21f }, { "b_out", -7 }, { "b_att", 0.17f }, { "b_rel", 0.83f },
            { "c_on", 1 }, { "c_model", VARIMU }, { "c_drive", 0.25f }, { "c_thr", 0.3f }, { "c_att", 0.3f }, { "c_rel", 0.5f }, { "d_on", 1 }, { "d_model", DISTRESS }, { "d_drive", 0.6f }, { "d_out", -20 }, { "d_ratio", 4 }, { "d_mode", 2 }, { "d_mix", 0.25f }, { "auto", 1 } } },
        // ================= SIDECHAIN
        { "Sidechain | Ducking cassa (VCA)", "Metti la cassa nel sidechain (ESTERNO): il VCA abbassa basso o pad su ogni colpo, rilascio a tempo.", 0,
          { { "a_model", VCABUS }, { "a_drive", 0.6f }, { "a_ratio", 2 }, { "a_att", 0.0f }, { "a_rel", 0.25f }, { "scSrc", 1 } } },
        { "Sidechain | Pumping (SSL)", "Ducking profondo con il canale SSL a 10:1 e rilascio a tempo: il classico respiro. Sidechain ESTERNO dalla cassa.", 0,
          { { "a_model", SSLCH }, { "a_drive", 0.75f }, { "a_cratio", 10 }, { "a_mode", 1 }, { "a_rel", 0.2f }, { "scSrc", 1 } } },
        { "Sidechain | De-boom (HPF)", "Sidechain interno con HPF a 150 Hz: il compressore non reagisce ai bassi e non pompa.", 0,
          { { "a_model", VCABUS }, { "a_drive", 0.35f }, { "a_ratio", 1 }, { "a_att", 0.8f }, { "a_rel", 1.0f }, { "scHpf", 150 }, { "auto", 1 } } },
        { "Sidechain | 808 sotto la cassa", "Sidechain ESTERNO dalla cassa su un multibanda a 2 bande: si abbassa solo il sub dell'808 quando suona la cassa.", 0,
          { { "a_model", MULTI }, { "a_ratio", 0 }, { "scSrc", 1 }, { "mb_x2", 100 }, { "mb_b1_thr", -30 }, { "mb_b1_ratio", 8 }, { "mb_b1_att", 1 }, { "mb_b1_rel", 120 }, { "mb_b2_thr", 0 } } },
        // ================= BUS E MASTER
        { "Bus | Mix bus glue (VCA)", "VCA 2:1, attacco 30 ms, AUTO: 1-2 dB per incollare il mix.", 0,
          { { "a_model", VCABUS }, { "a_drive", 0.2f }, { "a_ratio", 0 }, { "a_att", 1.0f }, { "a_rel", 1.0f }, { "a_color", 0.15f }, { "auto", 1 } } },
        { "Bus | Strumentale (670)", "TC3 sul bus del beat: tutto si fonde, i bassi si fanno tondi e le alte brillano un po'.", 0,
          { { "a_model", VC670 }, { "a_drive", 0.15f }, { "a_thr", 0.4f }, { "a_ratio", 2 }, { "a_color", 0.35f }, { "auto", 1 } } },
        { "Master | Vari-Mu caldo", "Vari-Mu in COMPRESS, RECOVERY 4 s, 1-2 dB: calore e coesione prima del limiter.", 0,
          { { "a_model", VARIMU }, { "a_drive", 0.125f }, { "a_thr", 0.25f }, { "a_ratio", 0 }, { "a_att", 0.5f }, { "a_rel", 0.75f }, { "a_color", 0.35f }, { "auto", 1 } } },
        { "Master | 670 LAT/VERT", "Il 670 come nei mastering anni 60: LAT/VERT, TC5, 1 dB. Il centro sta fermo, i lati respirano.", 0,
          { { "a_model", VC670 }, { "a_drive", 0.1f }, { "a_thr", 0.3f }, { "a_rel", 1.0f }, { "a_ratio", 4 }, { "a_color", 0.25f }, { "auto", 1 } } },
        { "Master | Multibanda alpha", "4 bande gentili (1-2 dB ciascuna), WARM al 50%: tiene in riga il mix senza schiacciarlo.", 0,
          { { "a_model", MULTI }, { "a_ratio", 2 }, { "a_color", 0.5f }, { "mb_x1", 120 }, { "mb_x2", 900 }, { "mb_x3", 7000 },
            { "mb_b1_thr", -18 }, { "mb_b1_ratio", 2 }, { "mb_b1_att", 30 }, { "mb_b1_rel", 300 }, { "mb_b2_thr", -18 }, { "mb_b2_ratio", 1.6f }, { "mb_b2_att", 20 }, { "mb_b2_rel", 200 },
            { "mb_b3_thr", -20 }, { "mb_b3_ratio", 1.6f }, { "mb_b3_att", 10 }, { "mb_b3_rel", 150 }, { "mb_b4_thr", -24 }, { "mb_b4_ratio", 2 }, { "mb_b4_att", 5 }, { "mb_b4_rel", 100 }, { "auto", 1 } } },
        { "Master | Serie gentile (VCA > 2A)", "VCA leggero + OPTO leggero: due stadi da 1 dB suonano meglio di uno da 2.", 0,
          { { "a_model", VCABUS }, { "a_drive", 0.2f }, { "a_ratio", 0 }, { "a_att", 1.0f }, { "a_rel", 1.0f }, { "a_color", 0.1f },
            { "b_on", 1 }, { "b_model", OPTO2A }, { "b_drive", 0.3f }, { "b_color", 0.25f }, { "auto", 1 } } },
        { "Master | Glue + multibanda", "VCA per la colla (1-2 dB) e multibanda a 3 bande per tenere fermi bassi e alte.", 0,
          { { "a_model", VCABUS }, { "a_drive", 0.22f }, { "a_att", 1.0f }, { "a_rel", 1.0f }, { "a_color", 0.1f },
            { "b_on", 1 }, { "b_model", MULTI }, { "b_ratio", 1 }, { "b_color", 0.3f }, { "mb_x1", 140 }, { "mb_x3", 6000 },
            { "mb_b1_thr", -18 }, { "mb_b1_ratio", 2.5f }, { "mb_b1_att", 30 }, { "mb_b1_rel", 250 }, { "mb_b2_thr", -10 }, { "mb_b2_ratio", 1.3f }, { "mb_b3_thr", -22 }, { "mb_b3_ratio", 2 }, { "mb_b3_att", 5 }, { "mb_b3_rel", 100 }, { "auto", 1 } } },
        { "Master | Catena completa (VCA > Mu > MB)", "Colla del VCA, calore del Vari-Mu, multibanda gentile: ogni stadio 1 dB, il mix pronto per il limiter.", 0,
          { { "a_model", VCABUS }, { "a_drive", 0.18f }, { "a_ratio", 0 }, { "a_att", 1.0f }, { "a_rel", 1.0f },
            { "b_on", 1 }, { "b_model", VARIMU }, { "b_drive", 0.08f }, { "b_thr", 0.2f }, { "b_att", 0.5f }, { "b_rel", 0.75f }, { "b_color", 0.3f },
            { "c_on", 1 }, { "c_model", MULTI }, { "c_ratio", 1 }, { "mb_x1", 130 }, { "mb_x3", 7000 },
            { "mb_b1_thr", -16 }, { "mb_b1_ratio", 2 }, { "mb_b1_att", 30 }, { "mb_b1_rel", 250 }, { "mb_b2_thr", -10 }, { "mb_b2_ratio", 1.3f }, { "mb_b3_thr", -20 }, { "mb_b3_ratio", 1.8f }, { "mb_b3_att", 6 }, { "mb_b3_rel", 120 }, { "auto", 1 } } },
        // ================= OTT E PARALLELO
        { "OTT | Hat e synth", "OTT a profondita' media con MIX al 50%: hat e synth brillanti e densi.", 0,
          { { "a_model", OTT }, { "a_drive", 0.35f }, { "a_ratio", 0 }, { "a_mix", 0.5f }, { "a_color", 0.1f }, { "auto", 1 } } },
        { "OTT | Pad e atmosfere", "OTT leggero al 35%: i dettagli bassi dei pad salgono, il pad si allarga senza alzarsi.", 0,
          { { "a_model", OTT }, { "a_drive", 0.25f }, { "a_ratio", 0 }, { "a_mix", 0.35f }, { "auto", 1 } } },
        { "Parallelo | Smash + Opto", "PARALLELO: ogni stadio lavora sul segnale originale e poi si sommano. A distrugge (NUKE), B livella morbido.", 2,
          { { "a_model", DISTRESS }, { "a_drive", 0.6f }, { "a_out", -20 }, { "a_ratio", 7 }, { "a_att", 0.1f }, { "a_rel", 0.3f }, { "a_mode", 2 }, { "a_mix", 0.6f },
            { "b_on", 1 }, { "b_model", OPTO2A }, { "b_drive", 0.5f }, { "auto", 1 } } },
        { "Parallelo | 670 + FET ALL", "PARALLELO: il 670 tiene il corpo, il FET in ALL aggiunge esplosione. Per batterie e room.", 2,
          { { "a_model", VC670 }, { "a_drive", 0.3f }, { "a_thr", 0.55f }, { "a_ratio", 1 }, { "a_color", 0.5f },
            { "b_on", 1 }, { "b_model", FET76 }, { "b_drive", 0.54f }, { "b_out", -18 }, { "b_ratio", 4 }, { "b_att", 0.67f }, { "b_rel", 1.0f }, { "b_mix", 0.5f }, { "auto", 1 } } } };
    return P;
}

void Processor::loadPreset (int i)
{
    if (! juce::isPositiveAndBelow (i, (int) presets().size())) return;
    preset = i; userName = {};
    for (auto* p : getParameters()) if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p)) rp->setValueNotifyingHost (rp->getDefaultValue());
    const auto& P = presets()[(size_t) i];
    if (auto* rp = apvts.getParameter ("route")) rp->setValueNotifyingHost (rp->convertTo0to1 ((float) P.route));
    if (i > 0) if (auto* rp = apvts.getParameter ("a_on")) rp->setValueNotifyingHost (1.0f);   // i preset partono dallo stadio A acceso
    for (auto& [id, v] : P.v) if (auto* rp = apvts.getParameter (id)) rp->setValueNotifyingHost (rp->convertTo0to1 (v));
    // multibanda senza SOGLIE +/- indicato: al centro
    for (int s = 0; s < kStages; ++s)
    {
        const juce::String p = juce::String (stagePfx (s)).dropLastCharacters (1);
        if (juce::roundToInt (pp[s].model->load()) != MULTI) continue;
        bool given = false; for (auto& [id, v] : P.v) if (juce::String (id) == p + "_drive") given = true;
        if (! given) if (auto* rp = apvts.getParameter (p + "_drive")) rp->setValueNotifyingHost (0.3f);
    }
}
// ---- conversione dei valori salvati dalle versioni precedenti ----
// v1 (fino alla 1.1.96): soglia al centro del ginocchio, INPUT dei modelli FET/VARI-MU/DISTRESS dentro soglie fisse
// v2 (1.1.97): INPUT compensato in uscita. v3: INPUT vero come l'hardware, FET 1-7 verso destra = veloce, 160 e SSL a ratio continuo
void Processor::convertStage (int s, int from)
{
    if (from >= kStateVersion) return;
    const juce::String p = juce::String (stagePfx (s)).dropLastCharacters (1);
    auto set = [this, &p] (const char* k, float v) { if (auto* rp = apvts.getParameter (p + k)) rp->setValueNotifyingHost (rp->convertTo0to1 (v)); };
    const auto& q = pp[s];
    const int m = juce::roundToInt (q.model->load()), r = juce::roundToInt (q.ratio->load());
    const float d = q.drive->load(), out = q.out->load(), att = q.att->load(), rel = q.rel->load(), col = q.color->load();
    const bool ol = q.olink->load() > 0.5f;
    static const float rms160[] = { 2, 3, 4, 6, 10, 60 }, ssl[] = { 1.5f, 2, 3, 4, 6, 10, 20, 100 };
    static const float tcToRec[] = { 0.25f, 0.5f, 0.75f, 0.75f, 1.0f, 1.0f };
    if (m == RMS160) { set ("_cratio", rms160[juce::jlimit (0, 5, r)]); set ("_ratio", 0); }
    if (m == SSLCH) { set ("_cratio", juce::jmin (60.0f, ssl[juce::jlimit (0, 7, r)])); set ("_mode", att >= 0.5f ? 1.0f : 0.0f); set ("_ratio", 0); }
    if (from == 1)
    {
        if (m == OPTO2A || m == OPTO3A || m == VCABUS || m == RMS160 || m == SSLCH) set ("_drive", convOldDrive (m, r, d));
        if (m == FET76 || m == DISTRESS)
        {
            const float Tf = m == FET76 ? (r == 0 ? -16.0f : -12.0f) : -18.0f;
            const float G = juce::jlimit (0.0f, 48.0f, kneeDb (m, r) * 0.5f - Tf + 36.0f * d);
            set ("_drive", G / 48.0f); set ("_out", out + 36.0f * d - (ol ? 27.0f * d : 0.0f) - G);
        }
        if (m == VARIMU)
        {
            const float Go = 36.0f * d, Gn = juce::jmin (24.0f, Go);
            set ("_drive", Gn / 24.0f); set ("_thr", juce::jlimit (0.0f, 1.0f, (24.0f + Go - Gn) / 30.0f)); set ("_out", out + Go - (ol ? 27.0f * d : 0.0f) - Gn);
            set ("_rel", tcToRec[juce::jlimit (0, 5, r)]); set ("_att", 0.2f); set ("_ratio", 0);
        }
        if (m == VC670)
        {
            set ("_thr", convOld670Thr (att)); set ("_att2", convOld670Thr (q.att2->load())); set ("_att", 0.5f);
            if (ol) set ("_out", out - 13.5f * d);
        }
    }
    else if (from == 2)
    {
        if (m == FET76 || m == DISTRESS) set ("_out", out - 48.0f * d);
        if (m == VARIMU)
        {
            const float Gn = juce::jmin (24.0f, 48.0f * d);
            set ("_drive", Gn / 24.0f); set ("_thr", juce::jlimit (0.0f, 1.0f, (48.0f * d - Gn) / 30.0f)); set ("_out", out - Gn);
            set ("_rel", tcToRec[juce::jlimit (0, 5, r)]); set ("_att", 0.2f); set ("_ratio", 0);
        }
        if (m == VC670) set ("_out", out - 18.0f * d);
    }
    if (m == FET76) { set ("_att", 1.0f - att); set ("_rel", 1.0f - rel); }
    if (m == DISTRESS) set ("_mode", col < 0.05f ? 0.0f : col < 0.5f ? 1.0f : 2.0f);
    set ("_olink", 0);
}
void Processor::convertLegacy (int from) { for (int s = 0; s < kStages; ++s) convertStage (s, from); }

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
        set (p + "_drive", q.drive); set (p + "_color", q.color); set (p + "_mix", q.mix); set (p + "_out", 0); set (p + "_olink", 0);
        convertStage (k, 1);   // la proposta e' scritta nei valori della prima versione
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
    auto s = apvts.copyState(); s.setProperty ("preset", preset, nullptr); s.setProperty ("thrv", kStateVersion, nullptr); s.setProperty ("uname", userName, nullptr);
    if (auto x = s.createXml()) copyXmlToBinary (*x, d);
}
void Processor::setStateInformation (const void* data, int size)
{
    if (auto x = getXmlFromBinary (data, size)) { auto t = juce::ValueTree::fromXml (*x); if (t.isValid()) { preset = t.getProperty ("preset", 0); userName = t.getProperty ("uname", "").toString(); const int v = (int) t.getProperty ("thrv", 1); apvts.replaceState (t); if (v < kStateVersion) convertLegacy (v); } }
}

juce::File Processor::presetFolder()
{
    auto f = juce::File::getSpecialLocation (juce::File::userMusicDirectory).getChildFile ("Jackpot").getChildFile ("Studio Comp Preset");
    f.createDirectory(); return f;
}
bool Processor::saveUserPreset (const juce::String& name)
{
    auto s = apvts.copyState(); s.setProperty ("thrv", kStateVersion, nullptr);
    auto x = s.createXml(); if (! x) return false;
    const auto f = presetFolder().getChildFile (juce::File::createLegalFileName (name) + ".jcomp");
    if (! x->writeTo (f)) return false;
    userName = f.getFileNameWithoutExtension(); preset = -1; return true;
}
bool Processor::loadUserPreset (const juce::File& f)
{
    auto x = juce::XmlDocument::parse (f); if (! x) return false;
    auto t = juce::ValueTree::fromXml (*x); if (! t.isValid() || ! t.hasType (apvts.state.getType())) return false;
    const int v = (int) t.getProperty ("thrv", 1);
    apvts.replaceState (t); if (v < kStateVersion) convertLegacy (v);
    userName = f.getFileNameWithoutExtension(); preset = -1; return true;
}

juce::AudioProcessorEditor* Processor::createEditor() { return new Editor (*this); }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new jc::Processor(); }
