#include "S8Processor.h"
#include "S8Editor.h"

namespace s8
{
// punch, sustain, drive, mode, harm, duck, rel, mono(0=off,1=60,2=90,3=120,4=150), ceiling
const std::vector<Preset>& presets()
{
    static const std::vector<Preset> P = {
        { "Init", "Tutto a zero: l'808 passa intatta. Guarda la diagnosi a destra e parti da li'.", 0, 0, 0, 0, 0, 0, 120, 0, 0 },
        { "Trap  |  808 Pulita", "Sub pulito, un filo di nastro sul corpo, armoniche per i telefoni, ducking leggero sulla cassa.", 0.3f, 0, 0.15f, 0, 0.25f, 0.3f, 90, 3, -1 },
        { "Drill  |  Grintosa", "Attacco in avanti, saturazione dura sul corpo, ducking piu' deciso per le casse veloci.", 0.45f, 0, 0.45f, 2, 0.35f, 0.4f, 70, 3, -0.5f },
        { "Rage  |  Distorta", "808 distorta che riempie tutto: fuzz sul corpo e sugli alti, sub tenuto pulito sotto.", 0.2f, 0.1f, 0.8f, 3, 0.2f, 0.25f, 90, 4, -1 },
        { "Su Telefono", "Per chi ascolta da telefono e cuffiette: tante armoniche, un po' di valvola.", 0.15f, 0, 0.3f, 1, 0.7f, 0.2f, 100, 3, -1 },
        { "Boom Bap  |  Sub Caldo", "Basso caldo e tondo: valvola leggera, coda un po' piu' lunga.", 0.15f, 0.2f, 0.25f, 1, 0.2f, 0.15f, 150, 2, -1 },
        { "Lunga  |  Melodica", "808 che canta la melodia: coda allungata, armoniche per seguire le note.", 0.1f, 0.5f, 0.2f, 0, 0.35f, 0.25f, 150, 3, -1 },
        { "Corta  |  Bouncy", "808 corta e saltellante: coda accorciata, attacco in avanti.", 0.5f, -0.5f, 0.2f, 0, 0.2f, 0.2f, 60, 3, -1 } };
    return P;
}

juce::AudioProcessorValueTreeState::ParameterLayout Processor::layout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    auto F = [&] (const char* id, const char* nm, float a, float b, float d, float st = 0) { l.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, nm, NormalisableRange<float> (a, b, st), d)); };
    F ("punch", "Punch", 0, 1, 0); F ("sustain", "Sustain", -1, 1, 0);
    F ("drive", "Drive", 0, 1, 0);
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "mode", 1 }, "Carattere", StringArray { "TAPE", "TUBE", "HARD", "FUZZ" }, 0));
    F ("harm", "Armoniche", 0, 1, 0);
    F ("duck", "Ducking cassa", 0, 1, 0); F ("rel", "Rilascio ducking", 20, 400, 120, 1);
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "mono", 1 }, "Mono sub", StringArray { "OFF", "60 Hz", "90 Hz", "120 Hz", "150 Hz" }, 0));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "inv", 1 }, "Inverti fase", false));
    F ("ceil", "Ceiling", -12, 0, 0, 0.1f); F ("out", "Output", -12, 12, 0, 0.1f);
    return l;
}

Processor::Processor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true).withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                                       .withInput ("Cassa (sidechain)", juce::AudioChannelSet::stereo(), false)),
      apvts (*this, nullptr, "PARAMS", layout())
{
    for (auto& h : pcHist) h = 0;
    for (int i = 0; i < kHist; ++i) { hLvl[i] = 0; hKick[i] = 0; hDuck[i] = 1; }
}
bool Processor::isBusesLayoutSupported (const BusesLayout& l) const
{
    if (l.getMainOutputChannelSet() != juce::AudioChannelSet::stereo() || l.getMainInputChannelSet() != juce::AudioChannelSet::stereo()) return false;
    if (l.inputBuses.size() > 1) { const auto sc = l.inputBuses[1]; if (! sc.isDisabled() && sc != juce::AudioChannelSet::stereo() && sc != juce::AudioChannelSet::mono()) return false; }
    return true;
}
void Processor::prepareToPlay (double s, int block)
{
    sr = s;
    os = std::make_unique<juce::dsp::Oversampling<float>> (2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false);
    os->initProcessing ((size_t) juce::jmax (block, 4096));
    const double osr = sr * osF; const juce::dsp::ProcessSpec so { osr, (juce::uint32) (juce::jmax (block, 4096) * osF), 2 }, sb { sr, (juce::uint32) juce::jmax (block, 4096), 2 };
    auto lr = [] (juce::dsp::LinkwitzRileyFilter<float>& f, juce::dsp::LinkwitzRileyFilterType t, float hz, const juce::dsp::ProcessSpec& sp) { f.setType (t); f.setCutoffFrequency (hz); f.prepare (sp); f.reset(); };
    lr (xLo, juce::dsp::LinkwitzRileyFilterType::lowpass, 90, so); lr (xLoH, juce::dsp::LinkwitzRileyFilterType::highpass, 90, so);
    lr (xHi, juce::dsp::LinkwitzRileyFilterType::lowpass, 400, so); lr (xHiH, juce::dsp::LinkwitzRileyFilterType::highpass, 400, so);
    lr (hHp, juce::dsp::LinkwitzRileyFilterType::highpass, 85, so);
    lr (monoLp, juce::dsp::LinkwitzRileyFilterType::lowpass, 120, sb);
    lr (sideLp, juce::dsp::LinkwitzRileyFilterType::lowpass, 120, sb); lr (midLp, juce::dsp::LinkwitzRileyFilterType::lowpass, 120, sb);
    tuner.prepare (sr);
    auto lp150 = juce::dsp::IIR::Coefficients<float>::makeLowPass (sr, 150.0f, 0.707f); kLp.coefficients = lp150; bLp.coefficients = lp150; kLp.reset(); bLp.reset();
    envF = envS = 0; kEnv = kSlow = 0; duckG = 1; kWin = 0; hEnv = 0;
    setLatencySamples (juce::roundToInt (os->getLatencyInSamples()));
}

void Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals nd; midi.clear();
    const int N = buffer.getNumSamples(); if (N == 0) return;
    if (resetReq.exchange (false))
    {
        for (auto& a : sE) a = 0; sSideLo = sMidLo = 0; sN = 0; pk = 0; crestAvg = 0; for (auto& a : pcAcc) a = 0; centsAcc = logF0Acc = 0; pN = 0; corrAvg = 0; ovAvg = -100; hits = clipN = 0;
        for (auto& h : pcHist) h = 0; clips = 0; kickHits = 0; pitchN = 0;
    }
    float* L = buffer.getWritePointer (0); float* R = buffer.getWritePointer (1);
    auto sc = getBusBuffer (buffer, true, 1); const bool scOn = sc.getNumChannels() > 0;
    const float* kL = scOn ? sc.getReadPointer (0) : nullptr; const float* kR = scOn ? sc.getReadPointer (juce::jmin (1, sc.getNumChannels() - 1)) : nullptr;
    const bool inv = v ("inv") > 0.5f;
    const float punch = v ("punch"), sus = v ("sustain"), drive = v ("drive"), harm = v ("harm"), duck = v ("duck"), rel = v ("rel");
    const int mode = juce::roundToInt (v ("mode")), mono = juce::roundToInt (v ("mono"));
    const float ceil = juce::Decibels::decibelsToGain (v ("ceil")), og = juce::Decibels::decibelsToGain (v ("out"));
    const float fA = 1 - std::exp (-1.0f / (0.001f * (float) sr)), fR = 1 - std::exp (-1.0f / (0.030f * (float) sr));
    const float sA = 1 - std::exp (-1.0f / (0.030f * (float) sr)), sR = 1 - std::exp (-1.0f / (0.250f * (float) sr));
    const float dR = 1 - std::exp (-1.0f / (rel * 0.001f * (float) sr)), kA = 1 - std::exp (-1.0f / (0.0005f * (float) sr)), kRl = 1 - std::exp (-1.0f / (0.06f * (float) sr));
    const float kSl = 1 - std::exp (-1.0f / (0.2f * (float) sr)), pkDecay = std::exp (-1.0f / (3.0f * (float) sr));
    bool live = false;
    // ---------------- frequenza base: inversione, accordatore, transienti, cassa
    for (int i = 0; i < N; ++i)
    {
        if (inv) { L[i] = -L[i]; R[i] = -R[i]; }
        const float m = 0.5f * (L[i] + R[i]);
        pk = juce::jmax (pk, std::abs (L[i]), std::abs (R[i])); rmsAcc += m * m; ++rmsN;
        winPk = juce::jmax (winPk, std::abs (m));
        if (rmsN >= (int) (sr * 0.1)) { const float rr = std::sqrt (rmsAcc / rmsN); if (rr > 3e-3f) { crestAvg += (juce::Decibels::gainToDecibels (winPk / rr) - crestAvg) * 0.1f; crest = crestAvg; } rmsAcc = 0; rmsN = 0; winPk = 0; }
        if (tuner.push (m))
        {
            uiHz = tuner.hz; uiConf = tuner.conf; uiLevel = tuner.level;
            if (tuner.conf > 0.75f && tuner.hz > 25 && tuner.hz < 220)
            {
                const double midiN = 69.0 + 12.0 * std::log2 (tuner.hz / 440.0); const int nn = (int) std::round (midiN);
                pcAcc[((nn % 12) + 12) % 12] += 1.0; centsAcc += (midiN - nn) * 100.0; logF0Acc += std::log (tuner.hz); ++pN;
                for (int k = 0; k < 12; ++k) pcHist[k] = (float) pcAcc[k]; pitchN = pN; centsAvg = (float) (centsAcc / pN); f0Avg = (float) std::exp (logF0Acc / pN);
            }
        }
        // transienti: differenza fra inviluppo veloce e lento
        const float a = std::abs (m);
        envF += (a > envF ? fA : fR) * (a - envF); envS += (a > envS ? sA : sR) * (a - envS);
        float gdb = 0;
        if (envS > 1e-5f)
        {
            const float tr = juce::jlimit (0.0f, 1.0f, (juce::Decibels::gainToDecibels (envF + 1e-9f) - juce::Decibels::gainToDecibels (envS + 1e-9f)) / 9.0f);
            gdb = punch * 9.0f * tr + sus * (sus > 0 ? 6.0f : 9.0f) * (1.0f - tr) * juce::jlimit (0.0f, 1.0f, (juce::Decibels::gainToDecibels (envS) + 50.0f) / 20.0f);
        }
        // cassa in sidechain: ducking, fase, sovrapposizione
        float dg = 1;
        if (scOn)
        {
            const float km = 0.5f * (kL[i] + kR[i]); const float kl = kLp.processSample (km), bl = bLp.processSample (m);
            if (std::abs (km) > 1e-4f) live = true;
            const float ka = std::abs (km); kEnv += (ka > kEnv ? kA : kRl) * (ka - kEnv); kSlow += kSl * (kEnv - kSlow);
            kPeak = juce::jmax (kEnv, kPeak * pkDecay); bEnv += (std::abs (bl) > bEnv ? kA : kRl) * (std::abs (bl) - bEnv);
            if (kWin == 0 && kEnv > 0.05f * kPeak && kEnv > 2.5f * kSlow + 1e-4f && kEnv > 0.003f) { kWin = (int) (0.03 * sr); kb = kk = bb = 0; bEnvAtHit = bEnv; kMaxWin = 0; }
            if (kWin > 0)
            {
                kb += (double) kl * bl; kk += (double) kl * kl; bb += (double) bl * bl; kMaxWin = juce::jmax (kMaxWin, kEnv);
                if (--kWin == 0 && kk > 1e-9 && bb > 1e-9)
                {
                    const float c = (float) (kb / std::sqrt (kk * bb)); ++hits; corrAvg += (c - corrAvg) / (float) juce::jmin (hits, 20);
                    const float ov = juce::Decibels::gainToDecibels (bEnvAtHit + 1e-6f) - juce::Decibels::gainToDecibels (kMaxWin + 1e-6f);
                    ovAvg = hits == 1 ? ov : ovAvg + (ov - ovAvg) / (float) juce::jmin (hits, 20);
                    kickCorr = corrAvg; overlapDb = ovAvg; kickHits = hits;
                }
            }
            const float tgt = 1.0f - duck * 0.9f * juce::jlimit (0.0f, 1.0f, kEnv / (kPeak + 1e-6f) * 1.4f);
            duckG += (tgt < duckG ? kA * 4 : dR) * (tgt - duckG); dg = duckG;
            colK = juce::jmax (colK, kEnv / (kPeak + 1e-6f));
        }
        const float g = juce::Decibels::decibelsToGain (gdb) * dg;
        L[i] *= g; R[i] *= g;
    }
    if (live) liveHold = (int) (2.0 * sr); else liveHold = juce::jmax (0, liveHold - N);
    scLive = liveHold > 0; uiDuck = duckG;
    inPeak = juce::Decibels::gainToDecibels (pk + 1e-9f); pk *= 0.9f;

    // ---------------- sovracampionato: bande, saturazione, armoniche, clipper
    juce::dsp::AudioBlock<float> blk (buffer.getArrayOfWritePointers(), 2, (size_t) N);
    auto ob = os->processSamplesUp (blk);
    const int M = (int) ob.getNumSamples(); const double osr = sr * osF;
    const float pre = 1.0f + drive * drive * 14.0f, comp = std::pow (pre, -0.6f);
    const float hA = 1 - std::exp (-1.0f / (0.0005f * (float) osr)), hR = 1 - std::exp (-1.0f / (0.04f * (float) osr));
    float* o[2] = { ob.getChannelPointer (0), ob.getChannelPointer (1) };
    float gr = 0; int clipNow = 0;
    for (int i = 0; i < M; ++i)
    {
        float subM = 0;
        for (int c = 0; c < 2; ++c)
        {
            const float x = o[c][i];
            const float sub = xLo.processSample (c, x), rest = xLoH.processSample (c, x);
            const float body = xHi.processSample (c, rest), high = xHiH.processSample (c, rest);
            if ((i & 3) == 0) { sE[0] += (double) sub * sub; sE[1] += (double) body * body; sE[2] += (double) high * high; }
            const float bodyS = drive > 0.001f ? sat (body * pre, mode) * comp : body;
            const float highS = drive > 0.001f ? sat (high * pre * 1.2f, mode) * comp * 0.9f : high;
            o[c][i] = sub + bodyS + highS; subM += 0.5f * sub;
        }
        if ((i & 3) == 0) ++sN;
        // armoniche esatte del sub: T2 e T3 di Chebyshev sul segnale normalizzato all'inviluppo
        if (harm > 0.001f)
        {
            const float a = std::abs (subM); hEnv += (a > hEnv ? hA : hR) * (a - hEnv);
            const float xn = juce::jlimit (-1.0f, 1.0f, subM / (hEnv + 1e-5f));
            const float h = hEnv * (0.55f * (2 * xn * xn - 1) + 0.45f * (4 * xn * xn * xn - 3 * xn)) * harm * 1.6f;
            for (int c = 0; c < 2; ++c) o[c][i] += hHp.processSample (c, h);
        }
        // clipper morbido al ceiling
        for (int c = 0; c < 2; ++c)
        {
            const float x = o[c][i], ax = std::abs (x), knee = 0.8f * ceil;
            if (ax > knee)
            {
                const float y = knee + (ceil - knee) * std::tanh ((ax - knee) / (ceil - knee + 1e-6f));
                if (ax > ceil) ++clipNow;
                gr = juce::jmax (gr, juce::Decibels::gainToDecibels (ax) - juce::Decibels::gainToDecibels (y));
                o[c][i] = x < 0 ? -y : y;
            }
        }
    }
    os->processSamplesDown (blk);
    if (clipNow > 0) { clipN += 1; clips = clipN; }
    uiGrClip = gr;
    // ---------------- mono sui bassi, uscita, statistiche stereo, storia
    static const float monoHz[5] = { 0, 60, 90, 120, 150 };
    if (mono > 0) monoLp.setCutoffFrequency (monoHz[mono]);
    for (int i = 0; i < N; ++i)
    {
        float m = 0.5f * (L[i] + R[i]), s = 0.5f * (L[i] - R[i]);
        const float sl = sideLp.processSample (0, s), ml = midLp.processSample (0, m); sSideLo += (double) sl * sl; sMidLo += (double) ml * ml;
        if (mono > 0) s -= monoLp.processSample (0, s);
        L[i] = (m + s) * og; R[i] = (m - s) * og;
        const float mo = 0.5f * (L[i] + R[i]);
        const int w = ringW.load(); ring[w] = mo; ringW = (w + 1) & (kRing - 1);
        colL = juce::jmax (colL, std::abs (L[i]), std::abs (R[i])); colD = juce::jmin (colD, duckG);
        if (++colN >= (int) (sr * 0.02))
        {
            const int hw = hW.load(); hLvl[hw] = colL; hKick[hw] = colK; hDuck[hw] = colD; hW = (hw + 1) % kHist; colL = colK = 0; colD = 1; colN = 0;
        }
    }
    if (sN > 0) { eSub = (float) (10 * std::log10 (sE[0] / sN + 1e-14)); eBody = (float) (10 * std::log10 (sE[1] / sN + 1e-14)); eHigh = (float) (10 * std::log10 (sE[2] / sN + 1e-14)); }
    if (sMidLo > 1e-9) sideRatio = (float) (10 * std::log10 (sSideLo / sMidLo + 1e-14));
}

void Processor::loadPreset (int i)
{
    const auto& p = presets()[(size_t) juce::jlimit (0, (int) presets().size() - 1, i)];
    setV ("punch", p.punch); setV ("sustain", p.sustain); setV ("drive", p.drive); setV ("mode", (float) p.mode); setV ("harm", p.harm);
    setV ("duck", p.duck); setV ("rel", p.rel); setV ("mono", (float) p.mono); setV ("ceil", p.ceil);
    presetName = p.name;
}
void Processor::getStateInformation (juce::MemoryBlock& d)
{
    auto s = apvts.copyState(); s.setProperty ("preset", presetName, nullptr);
    if (auto x = s.createXml()) copyXmlToBinary (*x, d);
}
void Processor::setStateInformation (const void* data, int size)
{
    if (auto x = getXmlFromBinary (data, size)) { auto t = juce::ValueTree::fromXml (*x); if (! t.isValid()) return; presetName = t.getProperty ("preset", presetName).toString(); apvts.replaceState (t); }
}
juce::AudioProcessorEditor* Processor::createEditor() { return new Editor (*this); }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new s8::Processor(); }
