#include "MeterProcessor.h"
#include "MeterEditor.h"

namespace jm
{
juce::AudioProcessorValueTreeState::ParameterLayout Processor::layout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    const float fr[kBands] = { 60, 200, 800, 2500, 6000, 12000, 100, 400, 1200, 3500, 8000, 16000 };
    const int ty[kBands] = { LCUT, LSHELF, BELL, BELL, BELL, HSHELF, BELL, BELL, BELL, BELL, BELL, HCUT };
    for (int b = 0; b < kBands; ++b)
    {
        const String p = "b" + String (b + 1) + "_", n = "Band " + String (b + 1) + " ";
        l.add (std::make_unique<AudioParameterBool> (ParameterID { p + "on", 1 }, n + "On", false));
        l.add (std::make_unique<AudioParameterChoice> (ParameterID { p + "type", 1 }, n + "Type", StringArray { "BELL", "LOW SHELF", "HIGH SHELF", "LOW CUT", "HIGH CUT" }, ty[b]));
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { p + "freq", 1 }, n + "Freq", NormalisableRange<float> (20, 20000, 0, 0.199f), fr[b]));
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { p + "gain", 1 }, n + "Gain", NormalisableRange<float> (-18, 18), 0));
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { p + "q", 1 }, n + "Q", NormalisableRange<float> (0.2f, 10, 0, 0.35f), 1));
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { p + "dyn", 1 }, n + "Dynamic", NormalisableRange<float> (-18, 18), 0));
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { p + "thr", 1 }, n + "Threshold", NormalisableRange<float> (-60, 0), -24));
    }
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "eqOn", 1 }, "EQ On", true));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "target", 1 }, "Target", StringArray { "SPOTIFY -14", "APPLE -16", "YOUTUBE -14", "CLUB -9", "CD / LOUD -8" }, 0));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "profile", 1 }, "Profilo", StringArray { "TRAP / DRILL", "HIP HOP / BOOM BAP", "LO-FI", "POP", "ACUSTICO" }, 0));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "spec", 1 }, "Spettro", StringArray { "MID + SIDE", "L + R" }, 0));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "out", 1 }, "Output", NormalisableRange<float> (-12, 12), 0));
    // imager
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "imOn", 1 }, "Imager On", false));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "imWidth", 1 }, "Imager Width", NormalisableRange<float> (0, 2), 1));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "imLow", 1 }, "Imager Low Width", NormalisableRange<float> (0, 2), 1));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "imMid", 1 }, "Imager Mid Width", NormalisableRange<float> (0, 2), 1));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "imHigh", 1 }, "Imager High Width", NormalisableRange<float> (0, 2), 1));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "imMono", 1 }, "Imager Mono Bass", NormalisableRange<float> (20, 300, 0, 0.5f), 20));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "imBal", 1 }, "Imager Balance", NormalisableRange<float> (-1, 1), 0));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "monoChk", 1 }, "Mono Check", false));
    return l;
}

Processor::Processor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true).withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "JMH", layout())
{
    for (int i = 0; i < kHist; ++i) { histS[i] = -100; histM[i] = -100; }
}

void Processor::resetLoudness()
{
    std::fill (histI.begin(), histI.end(), 0); std::fill (histST.begin(), histST.end(), 0); nI = nST = 0;
    lufsI = -100; lra = 0; tpMax = -100; pkMax = -100; blocksSeen = 0;
    for (int i = 0; i < kHist; ++i) { histS[i] = -100; histM[i] = -100; }
}

void Processor::prepareToPlay (double s, int maxBlock)
{
    sr = s;
    // K-weighting (ITU BS.1770) per qualsiasi frequenza di campionamento
    {
        double f0 = 1681.974450955533, G = 3.999843853973347, Q = 0.7071752369554196, K = std::tan (juce::MathConstants<double>::pi * f0 / sr);
        const double Vh = std::pow (10.0, G / 20.0), Vb = std::pow (Vh, 0.4996667741545416), a0 = 1.0 + K / Q + K * K;
        kb[0][0] = (Vh + Vb * K / Q + K * K) / a0; kb[0][1] = 2.0 * (K * K - Vh) / a0; kb[0][2] = (Vh - Vb * K / Q + K * K) / a0;
        ka[0][1] = 2.0 * (K * K - 1.0) / a0; ka[0][2] = (1.0 - K / Q + K * K) / a0;
        f0 = 38.13547087602444; Q = 0.5003270373238773; K = std::tan (juce::MathConstants<double>::pi * f0 / sr);
        kb[1][0] = 1; kb[1][1] = -2; kb[1][2] = 1;
        ka[1][1] = 2.0 * (K * K - 1.0) / (1.0 + K / Q + K * K); ka[1][2] = (1.0 - K / Q + K * K) / (1.0 + K / Q + K * K);
    }
    blockLen = juce::jmax (1, (int) (sr * 0.1)); blockE = 0; blockN = 0;
    blocks.assign (30, 0.0); blockIdx = 0;
    histI.assign (760, 0); histST.assign (760, 0); nI = nST = 0;
    os.reset(); os.initProcessing ((size_t) maxBlock); tpBuf.setSize (2, maxBlock);
    imX[0].set (HCUT, 150, 0, 0.707, sr); imX[1].set (HCUT, 150, 0, 0.707, sr); imX[2].set (LCUT, 2000, 0, 0.707, sr); imX[3].set (LCUT, 2000, 0, 0.707, sr);
    for (auto& x : imX) x.reset(); imMonoLast = -1; soloLast[0] = -1;
    xo[0].set (HCUT, 150, 0, 0.707, sr); xo[1].set (LCUT, 150, 0, 0.707, sr); xo[2].set (HCUT, 2000, 0, 0.707, sr); xo[3].set (LCUT, 2000, 0, 0.707, sr);
    for (auto& x : xo) x.reset();
    for (int b = 0; b < kBands; ++b) { eq[b].reset(); det[b].reset(); env[b] = 0; }
    resetLoudness();
}

static int binOf (double lufs) { return juce::jlimit (0, 759, (int) std::round ((lufs + 70.0) * 10.0)); }

void Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals nd;
    const int N = buffer.getNumSamples();
    if (buffer.getNumChannels() < 2) return;
    float* L = buffer.getWritePointer (0); float* R = buffer.getWritePointer (1);
    if (resetReq.exchange (false)) resetLoudness();

    // ---------- EQ dinamico (IIR, latenza zero) ----------
    if (value ("eqOn") > 0.5f)
    {
        struct BP { bool on; int type; float f, g, q, dyn, thr; } bp[kBands];
        bool any = false;
        for (int b = 0; b < kBands; ++b)
        {
            const juce::String p = "b" + juce::String (b + 1) + "_";
            bp[b] = { value ((p + "on").toRawUTF8()) > 0.5f, (int) value ((p + "type").toRawUTF8()), value ((p + "freq").toRawUTF8()), value ((p + "gain").toRawUTF8()),
                      value ((p + "q").toRawUTF8()), value ((p + "dyn").toRawUTF8()), value ((p + "thr").toRawUTF8()) };
            any |= bp[b].on;
            if (! bp[b].on) dynGain[b] = 0;
        }
        if (any)
        {
            const float at = std::exp (-1.0f / (0.005f * (float) sr)), rl = std::exp (-1.0f / (0.08f * (float) sr));
            for (int i0 = 0; i0 < N; i0 += 32)
            {
                const int n = juce::jmin (32, N - i0);
                for (int b = 0; b < kBands; ++b)
                {
                    if (! bp[b].on) continue;
                    float dg = 0;
                    if (std::abs (bp[b].dyn) > 0.05f && bp[b].type <= HSHELF)
                    {
                        det[b].set (9, bp[b].f, 0, juce::jmax (0.5f, bp[b].q), sr);
                        for (int i = i0; i < i0 + n; ++i) { const float x = std::abs (det[b].tick (0.5f * (L[i] + R[i]), 0)); env[b] = x > env[b] ? at * env[b] + (1 - at) * x : rl * env[b] + (1 - rl) * x; }
                        const float lev = juce::Decibels::gainToDecibels (env[b] * 1.414f, -100.0f);
                        dg = bp[b].dyn * juce::jlimit (0.0f, 1.0f, (lev - bp[b].thr) / 12.0f);
                    }
                    const float cur = dynGain[b].load() + (dg - dynGain[b].load()) * 0.3f; dynGain[b] = cur;
                    eq[b].set (bp[b].type, bp[b].f, bp[b].g + cur, bp[b].q, sr);
                }
                for (int i = i0; i < i0 + n; ++i)
                    for (int b = 0; b < kBands; ++b) if (bp[b].on) { L[i] = eq[b].tick (L[i], 0); R[i] = eq[b].tick (R[i], 1); }
            }
        }
    }
    // ---------- solo di una banda ----------
    {
        const int sb = soloBand.load();
        if (sb >= 0 && sb < kBands)
        {
            const juce::String p = "b" + juce::String (sb + 1) + "_";
            const float f = value ((p + "freq").toRawUTF8()), q = value ((p + "q").toRawUTF8()); const int t = (int) value ((p + "type").toRawUTF8());
            const float qq = t == BELL ? juce::jmax (0.5f, q) : 0.8f;
            if (f != soloLast[0] || qq != soloLast[1]) { soloF.set (9, f, 0, qq, sr); soloLast[0] = f; soloLast[1] = qq; }
            for (int i = 0; i < N; ++i) { L[i] = soloF.tick (L[i], 0) * 1.4f; R[i] = soloF.tick (R[i], 1) * 1.4f; }
        }
    }
    // ---------- imager multibanda ----------
    if (value ("imOn") > 0.5f)
    {
        const float wG = value ("imWidth"), wl = value ("imLow"), wm = value ("imMid"), wh = value ("imHigh"), mono = value ("imMono"), bal = value ("imBal");
        const bool active = std::abs (wG - 1) > 0.003f || std::abs (wl - 1) > 0.003f || std::abs (wm - 1) > 0.003f || std::abs (wh - 1) > 0.003f || mono > 21 || std::abs (bal) > 0.003f;
        if (active)
        {
            if (mono > 21 && mono != imMonoLast) { imMono.set (HCUT, mono, 0, 0.707, sr); imMonoLast = mono; }
            const float gl = juce::jmin (1.0f, 1.0f - bal), gr = juce::jmin (1.0f, 1.0f + bal);
            for (int i = 0; i < N; ++i)
            {
                const float m = 0.5f * (L[i] + R[i]), sd = 0.5f * (L[i] - R[i]);
                const float lo = imX[1].tick (imX[0].tick (sd, 0), 0), hi = imX[3].tick (imX[2].tick (sd, 0), 0), mid = sd - lo - hi;
                float s2 = (lo * wl + mid * wm + hi * wh) * wG;
                if (mono > 21) s2 -= imMono.tick (s2, 0);
                L[i] = (m + s2) * gl; R[i] = (m - s2) * gr;
            }
        }
    }
    if (value ("monoChk") > 0.5f) for (int i = 0; i < N; ++i) { const float m = 0.5f * (L[i] + R[i]); L[i] = R[i] = m; }
    const float og = juce::Decibels::decibelsToGain (value ("out"));
    if (std::abs (og - 1.0f) > 1.0e-4f) buffer.applyGain (0, 0, N, og);

    // ---------- anello per spettro e goniometro ----------
    int w = ringW.load();
    for (int i = 0; i < N; ++i) { ringL[w] = L[i]; ringR[w] = R[i]; w = (w + 1) & (kRing - 1); }
    ringW = w;

    // ---------- loudness ----------
    for (int i = 0; i < N; ++i)
    {
        double e = 0;
        for (int c = 0; c < 2; ++c)
        {
            double x = c ? R[i] : L[i];
            for (int s = 0; s < 2; ++s)
            {
                const double y = kb[s][0] * x + kz[s][c][0];
                kz[s][c][0] = kb[s][1] * x - ka[s][1] * y + kz[s][c][1];
                kz[s][c][1] = kb[s][2] * x - ka[s][2] * y;
                x = y;
            }
            e += x * x;
        }
        blockE += e;
        if (++blockN >= blockLen)
        {
            blocks[(size_t) blockIdx] = blockE / blockN; blockIdx = (blockIdx + 1) % 30; blockE = 0; blockN = 0;
            auto meanOf = [this] (int n) { double s = 0; for (int k = 1; k <= n; ++k) s += blocks[(size_t) ((blockIdx - k + 30) % 30)]; return s / n; };
            const double m = -0.691 + 10 * std::log10 (meanOf (4) + 1e-12), st = -0.691 + 10 * std::log10 (meanOf (30) + 1e-12);
            lufsM = (float) m; lufsS = (float) st;
            if (m > -70) { histI[(size_t) binOf (m)]++; ++nI; }
            if (++blocksSeen >= 30 && st > -70) { histST[(size_t) binOf (st)]++; ++nST; }
            // integrato: gate assoluto -70, relativo -10 LU
            if (nI > 0)
            {
                double s = 0; for (int k = 0; k < 760; ++k) if (histI[(size_t) k]) s += histI[(size_t) k] * std::pow (10.0, (k / 10.0 - 70.0 + 0.691) / 10.0);
                const double ung = -0.691 + 10 * std::log10 (s / nI), gate = ung - 10;
                double s2 = 0; int n2 = 0; for (int k = binOf (gate); k < 760; ++k) if (histI[(size_t) k]) { s2 += histI[(size_t) k] * std::pow (10.0, (k / 10.0 - 70.0 + 0.691) / 10.0); n2 += histI[(size_t) k]; }
                if (n2 > 0) lufsI = (float) (-0.691 + 10 * std::log10 (s2 / n2));
            }
            // LRA: short-term con gate relativo -20 LU, percentili 10-95
            if (nST > 10)
            {
                double s = 0; for (int k = 0; k < 760; ++k) if (histST[(size_t) k]) s += histST[(size_t) k] * std::pow (10.0, (k / 10.0 - 70.0 + 0.691) / 10.0);
                const int g0 = binOf (-0.691 + 10 * std::log10 (s / nST) - 20);
                int tot = 0; for (int k = g0; k < 760; ++k) tot += histST[(size_t) k];
                if (tot > 0)
                {
                    int acc = 0, lo = -1, hi = -1;
                    for (int k = g0; k < 760; ++k) { acc += histST[(size_t) k]; if (lo < 0 && acc >= tot * 0.10) lo = k; if (hi < 0 && acc >= tot * 0.95) { hi = k; break; } }
                    if (lo >= 0 && hi >= 0) lra = (hi - lo) / 10.0f;
                }
            }
            if (++histTick >= 2)
            {
                histTick = 0; const int hw = histW.load(); histS[hw] = (float) st; histM[hw] = (float) m; histW = (hw + 1) % kHist;
            }
        }
    }

    // ---------- true peak (sovracampionamento 4x solo sulla copia di analisi) ----------
    {
        if (tpBuf.getNumSamples() < N) tpBuf.setSize (2, N, false, false, true);
        tpBuf.copyFrom (0, 0, L, N); tpBuf.copyFrom (1, 0, R, N);
        juce::dsp::AudioBlock<float> blk (tpBuf.getArrayOfWritePointers(), 2, (size_t) N);
        auto up = os.processSamplesUp (blk);
        float pk = 0;
        for (size_t c = 0; c < up.getNumChannels(); ++c) { auto* d = up.getChannelPointer (c); for (size_t i = 0; i < up.getNumSamples(); ++i) pk = juce::jmax (pk, std::abs (d[i])); }
        float sp = 0; for (int i = 0; i < N; ++i) sp = juce::jmax (sp, juce::jmax (std::abs (L[i]), std::abs (R[i])));
        pk = juce::jmax (pk, sp);
        const float db = juce::Decibels::gainToDecibels (pk, -100.0f);
        tpNow = juce::jmax (db, tpNow.load() - 0.5f); tpMax = juce::jmax (tpMax.load(), db);
        pkMax = juce::jmax (pkMax.load(), juce::Decibels::gainToDecibels (sp, -100.0f));
    }

    // ---------- stereo ----------
    {
        const double k = std::exp (-(double) N / (0.3 * sr)), ki = 1 - k;
        double ll = 0, rr = 0, lr = 0, bl[3] = {}, br[3] = {}, blr[3] = {}, mm = 0, ss = 0;
        for (int i = 0; i < N; ++i)
        {
            const float l = L[i], r = R[i];
            ll += l * l; rr += r * r; lr += l * r; mm += 0.25 * (l + r) * (l + r); ss += 0.25 * (l - r) * (l - r);
            const float lo[2] = { xo[0].tick (l, 0), xo[0].tick (r, 1) };
            const float hm[2] = { xo[1].tick (l, 0), xo[1].tick (r, 1) };
            const float mi[2] = { xo[2].tick (hm[0], 0), xo[2].tick (hm[1], 1) };
            const float hi[2] = { xo[3].tick (l, 0), xo[3].tick (r, 1) };
            const float* bands[3] = { lo, mi, hi };
            for (int b = 0; b < 3; ++b) { bl[b] += bands[b][0] * bands[b][0]; br[b] += bands[b][1] * bands[b][1]; blr[b] += bands[b][0] * bands[b][1]; }
        }
        sLL = k * sLL + ki * ll; sRR = k * sRR + ki * rr; sLR = k * sLR + ki * lr; eM = k * eM + ki * mm; eS = k * eS + ki * ss;
        corr = sLL * sRR > 1e-12 ? (float) (sLR / std::sqrt (sLL * sRR)) : 1.0f;
        balance = (float) (10 * std::log10 ((sRR + 1e-12) / (sLL + 1e-12)));
        width = eM > 1e-12 ? (float) juce::jlimit (0.0, 2.0, std::sqrt (eS / eM)) : 0.0f;
        for (int b = 0; b < 3; ++b)
        {
            bLL[b] = k * bLL[b] + ki * bl[b]; bRR[b] = k * bRR[b] + ki * br[b]; bLR[b] = k * bLR[b] + ki * blr[b];
            corrB[b] = bLL[b] * bRR[b] > 1e-14 ? (float) (bLR[b] / std::sqrt (bLL[b] * bRR[b])) : 1.0f;
            const double m2 = 0.25 * (bLL[b] + bRR[b] + 2 * bLR[b]), s2 = 0.25 * (bLL[b] + bRR[b] - 2 * bLR[b]);
            widthB[b] = m2 > 1e-14 ? (float) juce::jlimit (0.0, 2.0, std::sqrt (juce::jmax (0.0, s2) / m2)) : 0.0f;
        }
        rmsDb = (float) (10 * std::log10 (0.5 * (sLL + sRR) / juce::jmax (1, N) + 1e-12));
        // livelli per canale in dBFS: picco istantaneo con tenuta, RMS su 300 ms
        float pl = 0, pr = 0; double el = 0, er = 0;
        for (int i = 0; i < N; ++i) { pl = juce::jmax (pl, std::abs (L[i])); pr = juce::jmax (pr, std::abs (R[i])); el += L[i] * L[i]; er += R[i] * R[i]; }
        eL = k * eL + ki * el / juce::jmax (1, N); eR = k * eR + ki * er / juce::jmax (1, N);
        const float dl = juce::Decibels::gainToDecibels (pl, -100.0f), dr = juce::Decibels::gainToDecibels (pr, -100.0f);
        pkL = juce::jmax (dl, pkL.load() - 0.6f); pkR = juce::jmax (dr, pkR.load() - 0.6f);
        pkHoldL = juce::jmax (dl, pkHoldL.load() - 0.02f); pkHoldR = juce::jmax (dr, pkHoldR.load() - 0.02f);
        rmsL = (float) (10 * std::log10 (eL + 1e-12)); rmsR = (float) (10 * std::log10 (eR + 1e-12));
    }
}

void Processor::getStateInformation (juce::MemoryBlock& d) { if (auto x = apvts.copyState().createXml()) copyXmlToBinary (*x, d); }
void Processor::setStateInformation (const void* data, int size)
{
    if (auto x = getXmlFromBinary (data, size)) { auto t = juce::ValueTree::fromXml (*x); if (t.isValid()) apvts.replaceState (t); }
}
juce::AudioProcessorEditor* Processor::createEditor() { return new Editor (*this); }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new jm::Processor(); }
