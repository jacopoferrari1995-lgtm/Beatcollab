#include <juce_audio_formats/juce_audio_formats.h>
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
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "scShow", 1 }, "Mostra sidechain", true));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "dynSc", 1 }, "EQ dinamico da sidechain", false));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "target", 1 }, "Target", StringArray { "SPOTIFY -14", "APPLE -16", "YOUTUBE -14", "CLUB -9", "CD / LOUD -8" }, 0));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "profile", 1 }, "Profilo", [] { StringArray a; for (int i = 0; i < kGenres; ++i) a.add (profile (i).name); a.add ("TRACCIA CARICATA"); return a; }(), 0));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "view", 1 }, "Vista", StringArray { "EQ", "REFERENCE", "IMAGER", "PEAK", "SPETTROGRAMMA" }, 0));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "imView", 1 }, "Vista imager", StringArray { "LISSAJOUS", "POLAR SAMPLE", "POLAR LEVEL" }, 0));
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
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true).withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                                       .withInput ("Sidechain", juce::AudioChannelSet::stereo(), false)),
      apvts (*this, &undo, "JMH", layout())
{
    for (int i = 0; i < kHist; ++i) { histS[i] = -100; histM[i] = -100; }
    static const char* ks[7] = { "on", "type", "freq", "gain", "q", "dyn", "thr" };
    for (int b = 0; b < kBands; ++b) for (int k = 0; k < 7; ++k) bpp[b][k] = apvts.getRawParameterValue ("b" + juce::String (b + 1) + "_" + ks[k]);
    for (int i = 0; i < kPH; ++i) { phMax[i] = 0; phRms[i] = 0; }
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
    os.reset(); os.initProcessing ((size_t) juce::jmax (64, maxBlock)); tpBuf.setSize (2, juce::jmax (64, maxBlock));
    imX[0].set (HCUT, 150, 0, 0.707, sr); imX[1].set (HCUT, 150, 0, 0.707, sr); imX[2].set (LCUT, 2000, 0, 0.707, sr); imX[3].set (LCUT, 2000, 0, 0.707, sr);
    for (auto& x : imX) x.reset(); imMonoLast = -1; soloLast[0] = -1;
    xo[0].set (HCUT, 150, 0, 0.707, sr); xo[1].set (LCUT, 150, 0, 0.707, sr); xo[2].set (HCUT, 2000, 0, 0.707, sr); xo[3].set (LCUT, 2000, 0, 0.707, sr);
    for (auto& x : xo) x.reset();
    for (int b = 0; b < kBands; ++b)
    {
        eq[b].reset(); det[b].reset(); env[b] = 0;
        auto& S = bs[b]; S = {}; S.type = juce::roundToInt (bpp[b][1]->load()); S.f = bpp[b][2]->load(); S.g = bpp[b][3]->load(); S.q = bpp[b][4]->load();
        S.on = S.want = (value ("eqOn") > 0.5f && bpp[b][0]->load() > 0.5f) ? 1.0f : 0.0f; if (S.on > 0) eq[b].set (S.type, S.f, S.g, S.q, sr);
    }
    tick = 0; soloMix = 0; soloCur = -1; soloTick = imTick = 0; imRunning = false; monoS = value ("monoChk") > 0.5f ? 1.0f : 0.0f; ogS = -1;
    { const bool on = value ("imOn") > 0.5f; const float t[6] = { on ? value ("imWidth") : 1.0f, on ? value ("imLow") : 1.0f, on ? value ("imMid") : 1.0f, on ? value ("imHigh") : 1.0f, on ? value ("imMono") : 20.0f, on ? value ("imBal") : 0.0f }; for (int k = 0; k < 6; ++k) im[k] = t[k]; }
    resetLoudness();
}

static int binOf (double lufs) { return juce::jlimit (0, 759, (int) std::round ((lufs + 70.0) * 10.0)); }

void Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals nd;
    const int N = buffer.getNumSamples();
    auto mainB = getBusBuffer (buffer, true, 0);
    if (mainB.getNumChannels() < 2) return;
    float* L = mainB.getWritePointer (0); float* R = mainB.getWritePointer (1);
    // sidechain (es. la cassa): seconda curva nello spettro e, se scelto, rilevatore dell'EQ dinamico
    const float* sL = nullptr; const float* sR = nullptr;
    if (getBusCount (true) > 1 && getBus (true, 1)->isEnabled())
    {
        auto sc = getBusBuffer (buffer, true, 1);
        if (sc.getNumChannels() > 0) { sL = sc.getReadPointer (0); sR = sc.getReadPointer (juce::jmin (1, sc.getNumChannels() - 1)); }
    }
    hasSc = sL != nullptr;
    bool isPlay = true;
    if (auto* ph = getPlayHead()) if (auto pos = ph->getPosition()) isPlay = pos->getIsPlaying();
    playing = isPlay;
    const bool useScDyn = sL != nullptr && value ("dynSc") > 0.5f;
    if (resetReq.exchange (false)) resetLoudness();

    // ---------- EQ dinamico (IIR, latenza zero) ----------
    // ogni comando scivola (frequenza in scala logaritmica), i coefficienti si rifanno ogni 16 campioni su un contatore libero
    // (suono identico con qualsiasi buffer), accensione/spegnimento e cambio di tipo sfumati in 15 ms
    {
        const bool eqOn = value ("eqOn") > 0.5f;
        BP bp[kBands];
        for (int b = 0; b < kBands; ++b)
        {
            auto** q = bpp[b];   // puntatori presi una volta: niente stringhe nel thread audio
            bp[b] = { eqOn && q[0]->load() > 0.5f, juce::roundToInt (q[1]->load()), q[2]->load(), q[3]->load(), q[4]->load(), q[5]->load(), q[6]->load() };
        }
        const float at = std::exp (-1.0f / (0.005f * (float) sr)), rl = std::exp (-1.0f / (0.08f * (float) sr));
        const float fade = 1.0f / (0.015f * (float) sr);
        for (int i = 0; i < N; ++i)
        {
            if ((tick & 15) == 0)
            {
                for (int b = 0; b < kBands; ++b)
                {
                    auto& S = bs[b];
                    // tipo diverso: si spegne, si cambia, si riaccende
                    const bool wantOn = bp[b].on && bp[b].type == S.type;
                    if (S.on <= 0.0f && bp[b].type != S.type) { S.type = bp[b].type; eq[b].reset(); det[b].reset(); env[b] = 0; S.lastF = -1; }
                    S.want = wantOn ? 1.0f : 0.0f;
                    if (S.on <= 0.0f && S.want <= 0.0f) { S.f = bp[b].f; S.g = bp[b].g; S.q = bp[b].q; S.dg = 0; dynGain[b] = 0; continue; }
                    const float k = 1.0f - std::exp (-16.0f / (0.02f * (float) sr));   // 20 ms
                    S.f = std::exp (std::log (S.f) + (std::log (bp[b].f) - std::log (S.f)) * k); S.g += (bp[b].g - S.g) * k; S.q += (bp[b].q - S.q) * k;
                    float dg = 0;
                    if (std::abs (bp[b].dyn) > 0.05f && S.type <= HSHELF)
                    {
                        const float lev = juce::Decibels::gainToDecibels (env[b] * 1.414f, -100.0f);
                        dg = bp[b].dyn * juce::jlimit (0.0f, 1.0f, (lev - bp[b].thr) / 12.0f);
                        if (std::abs (S.f - S.detF) > 0.001f * S.f || std::abs (S.q - S.detQ) > 0.001f) { det[b].set (9, S.f, 0, juce::jmax (0.5f, S.q), sr); S.detF = S.f; S.detQ = S.q; }
                    }
                    S.dg += (dg - S.dg) * 0.3f; dynGain[b] = S.dg;
                    eq[b].set (S.type, S.f, S.g + S.dg, S.q, sr);
                }
            }
            ++tick;
            const float x0 = L[i], x1 = R[i];
            const float dIn = useScDyn ? 0.5f * (sL[i] + sR[i]) : 0.5f * (x0 + x1);
            float l = x0, r = x1;
            for (int b = 0; b < kBands; ++b)
            {
                auto& S = bs[b];
                if (S.on <= 0.0f && S.want <= 0.0f) continue;
                S.on = juce::jlimit (0.0f, 1.0f, S.on + (S.want > S.on ? fade : -fade));
                if (std::abs (bp[b].dyn) > 0.05f && S.type <= HSHELF) { const float xd = std::abs (det[b].tick (dIn, 0)); env[b] = xd > env[b] ? at * env[b] + (1 - at) * xd : rl * env[b] + (1 - rl) * xd; }
                const float yl = eq[b].tick (l, 0), yr = eq[b].tick (r, 1);
                if (S.on >= 1.0f) { l = yl; r = yr; } else { l += (yl - l) * S.on; r += (yr - r) * S.on; }
                if (S.on <= 0.0f && S.want <= 0.0f) { eq[b].reset(); dynGain[b] = 0; }
            }
            L[i] = l; R[i] = r;
        }
    }
    // ---------- solo di una banda (entra ed esce sfumato) ----------
    {
        const int sb = soloBand.load();
        if (sb != soloCur && soloMix <= 0.0f) { soloCur = sb; soloF.reset(); soloLast[0] = -1; }
        const float tgt = (sb == soloCur && soloCur >= 0 && soloCur < kBands) ? 1.0f : 0.0f;
        if (soloMix > 0.0f || tgt > 0.0f)
        {
            const int cb = juce::jlimit (0, kBands - 1, soloCur);
            const float f = bpp[cb][2]->load(), q = bpp[cb][4]->load(); const int t = juce::roundToInt (bpp[cb][1]->load());
            const float qq = t == BELL ? juce::jmax (0.5f, q) : 0.8f, fade = 1.0f / (0.015f * (float) sr);
            for (int i = 0; i < N; ++i)
            {
                if ((i + soloTick) % 16 == 0) { soloFs = soloLast[0] < 0 ? f : std::exp (std::log (soloFs) + (std::log (f) - std::log (soloFs)) * (1.0f - std::exp (-16.0f / (0.02f * (float) sr)))); soloF.set (9, soloFs, 0, qq, sr); soloLast[0] = 1; }
                soloMix = juce::jlimit (0.0f, 1.0f, soloMix + (tgt > soloMix ? fade : -fade));
                const float yl = soloF.tick (L[i], 0) * 1.4f, yr = soloF.tick (R[i], 1) * 1.4f;
                L[i] += (yl - L[i]) * soloMix; R[i] += (yr - R[i]) * soloMix;
            }
        }
        soloTick = (soloTick + N) % 16;
    }
    // ---------- imager multibanda (comandi che scivolano, acceso/spento sfumato) ----------
    {
        const bool on = value ("imOn") > 0.5f;
        const float t[6] = { on ? value ("imWidth") : 1.0f, on ? value ("imLow") : 1.0f, on ? value ("imMid") : 1.0f, on ? value ("imHigh") : 1.0f, on ? value ("imMono") : 20.0f, on ? value ("imBal") : 0.0f };
        bool still = true; for (int k = 0; k < 6; ++k) if (std::abs (im[k] - t[k]) > 1e-4f * (k == 4 ? 20.0f : 1.0f)) still = false;
        const bool neutral = std::abs (t[0] - 1) < 1e-4f && std::abs (t[1] - 1) < 1e-4f && std::abs (t[2] - 1) < 1e-4f && std::abs (t[3] - 1) < 1e-4f && t[4] <= 20.01f && std::abs (t[5]) < 1e-4f;
        if (still && neutral) { for (int k = 0; k < 6; ++k) im[k] = t[k]; imRunning = false; }
        else
        {
            if (! imRunning) { for (auto& x : imX) x.reset(); imMono.reset(); imRunning = true; }
            const float sk = 1.0f - std::exp (-1.0f / (0.02f * (float) sr));
            for (int i = 0; i < N; ++i)
            {
                for (int k = 0; k < 6; ++k) im[k] += (t[k] - im[k]) * sk;
                if (((imTick + i) & 15) == 0 && std::abs (im[4] - imMonoLast) > 0.01f) { imMono.set (HCUT, juce::jmax (20.0f, im[4]), 0, 0.707, sr); imMonoLast = im[4]; }
                const float m = 0.5f * (L[i] + R[i]), sd = 0.5f * (L[i] - R[i]);
                const float lo = imX[1].tick (imX[0].tick (sd, 0), 0), hi = imX[3].tick (imX[2].tick (sd, 0), 0), mid = sd - lo - hi;
                float s2 = (lo * im[1] + mid * im[2] + hi * im[3]) * im[0];
                // mono sui bassi: dosato da 20 a 30 Hz per entrare senza scatto
                const float mono = imMono.tick (s2, 0), mAmt = juce::jlimit (0.0f, 1.0f, (im[4] - 20.0f) / 10.0f);
                s2 -= mono * mAmt;
                const float gl = juce::jmin (1.0f, 1.0f - im[5]), gr = juce::jmin (1.0f, 1.0f + im[5]);
                L[i] = (m + s2) * gl; R[i] = (m - s2) * gr;
            }
        }
        imTick = (imTick + N) & 15;
    }
    {   // controllo mono e uscita sfumati
        const float mt = value ("monoChk") > 0.5f ? 1.0f : 0.0f, og = juce::Decibels::decibelsToGain (value ("out"));
        const float fade = 1.0f / (0.015f * (float) sr), sk = 1.0f - std::exp (-1.0f / (0.02f * (float) sr));
        if (monoS > 0.0f || mt > 0.0f)
            for (int i = 0; i < N; ++i) { monoS = juce::jlimit (0.0f, 1.0f, monoS + (mt > monoS ? fade : -fade)); const float m = 0.5f * (L[i] + R[i]); L[i] += (m - L[i]) * monoS; R[i] += (m - R[i]) * monoS; }
        if (ogS < 0) ogS = og;
        if (ogS != og || og != 1.0f)
        {
            for (int i = 0; i < N; ++i) { ogS += (og - ogS) * sk; L[i] *= ogS; R[i] *= ogS; }
            if (std::abs (ogS - og) < 1e-6f) ogS = og;
        }
    }

    // ---------- storia dei picchi (vista PEAK): un punto ogni 10 ms, solo in play ----------
    if (isPlay && ! freezeBtn.load())
    {
        const int chunk = juce::jmax (1, (int) (sr * 0.01));
        for (int i = 0; i < N; ++i)
        {
            phPk = juce::jmax (phPk, std::abs (L[i]), std::abs (R[i])); phE += 0.5 * (L[i] * L[i] + R[i] * R[i]);
            if (++phN >= chunk) { const int w = phW.load(); phMax[w] = phPk; phRms[w] = (float) std::sqrt (phE / phN); phW = (w + 1) % kPH; phPk = 0; phE = 0; phN = 0; }
        }
    }

    // ---------- anello per spettro e goniometro (si ferma con lo stop dell'host o con FREEZE) ----------
    const bool fz = freezeBtn.load() || ! isPlay;
    frozen = fz;
    if (! fz)
    {
        int w = ringW.load();
        for (int i = 0; i < N; ++i) { ringL[w] = L[i]; ringR[w] = R[i]; ringS[w] = sL ? 0.5f * (sL[i] + sR[i]) : 0.0f; w = (w + 1) & (kRing - 1); }
        ringW = w;
    }

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
        // a pezzi della misura preparata: con buffer piu' lunghi del previsto niente memoria nuova nel thread audio
        float pk = 0; const int cap = tpBuf.getNumSamples();
        for (int off = 0; off < N; off += cap)
        {
            const int n = juce::jmin (cap, N - off);
            tpBuf.copyFrom (0, 0, L + off, n); tpBuf.copyFrom (1, 0, R + off, n);
            juce::dsp::AudioBlock<float> blk (tpBuf.getArrayOfWritePointers(), 2, (size_t) n);
            auto up = os.processSamplesUp (blk);
            for (size_t c = 0; c < up.getNumChannels(); ++c) { auto* d = up.getChannelPointer (c); for (size_t i = 0; i < up.getNumSamples(); ++i) pk = juce::jmax (pk, std::abs (d[i])); }
        }
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

void Processor::getStateInformation (juce::MemoryBlock& d)
{
    auto st = apvts.copyState(); st.setProperty ("refPath", reference().path, nullptr);
    if (auto x = st.createXml()) copyXmlToBinary (*x, d);
}
void Processor::setStateInformation (const void* data, int size)
{
    if (auto x = getXmlFromBinary (data, size))
    {
        auto t = juce::ValueTree::fromXml (*x); if (! t.isValid()) return;
        const juce::String rp = t.getProperty ("refPath").toString(); t.removeProperty ("refPath", nullptr);
        apvts.replaceState (t);
        if (rp.isNotEmpty() && rp != reference().path) loadReference (juce::File (rp));
    }
}
void Processor::clearReference() { { const juce::SpinLock::ScopedLockType l (refLock); ref = {}; } ++refVersion; }

// analisi del pezzo di riferimento: spettro medio (stesse 320 bande e stessa compensazione dello spettro live),
// loudness integrata BS.1770 e picco. Gira su un thread a parte: non tocca l'audio.
void Processor::loadReference (const juce::File& f)
{
    if (refLoading.exchange (true)) return;
    juce::Thread::launch ([this, f]
    {
        RefTrack r; r.path = f.getFullPathName(); r.name = f.getFileNameWithoutExtension();
        juce::AudioFormatManager fm; fm.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> rd (fm.createReaderFor (f));
        if (rd == nullptr) r.error = "file non leggibile";
        else
        {
            const double fsr = rd->sampleRate; const int len = (int) std::min<juce::int64> (rd->lengthInSamples, (juce::int64) (fsr * 600));
            juce::AudioBuffer<float> b (2, len); rd->read (&b, 0, len, 0, true, true);
            if (rd->numChannels == 1) b.copyFrom (1, 0, b, 0, 0, len);
            const float* L = b.getReadPointer (0); const float* R = b.getReadPointer (1);
            // spettro medio
            constexpr int ord = 13, NF = 1 << ord; juce::dsp::FFT fft (ord); juce::dsp::WindowingFunction<float> win ((size_t) NF, juce::dsp::WindowingFunction<float>::hann);
            std::vector<float> buf ((size_t) NF * 2); std::vector<double> acc ((size_t) kPts, 0.0); int frames = 0;
            for (int pos = 0; pos + NF <= len; pos += NF / 2)
            {
                double e = 0; for (int i = 0; i < NF; ++i) { const float m = 0.5f * (L[pos + i] + R[pos + i]); buf[(size_t) i] = m; e += m * m; }
                if (e / NF < 1e-7) continue;   // salta i silenzi
                std::fill (buf.begin() + NF, buf.end(), 0.0f); win.multiplyWithWindowingTable (buf.data(), (size_t) NF); fft.performFrequencyOnlyForwardTransform (buf.data());
                for (int i = 0; i < kPts; ++i)
                {
                    const float f0 = 20.0f * std::pow (1000.0f, i / (float) kPts), f1 = 20.0f * std::pow (1000.0f, (i + 1) / (float) kPts);
                    const int b0 = (int) (f0 / fsr * NF), b1 = juce::jmax (b0 + 1, (int) (f1 / fsr * NF));
                    double pw = 0; int nb = 0; for (int k = b0; k < juce::jmin (b1, NF / 2); ++k) { pw += (double) buf[(size_t) k] * buf[(size_t) k]; ++nb; }
                    acc[(size_t) i] += nb > 0 ? pw / nb : 0.0;
                }
                ++frames;
            }
            if (frames == 0) r.error = "il file e vuoto o in silenzio";
            else
            {
                r.spec.resize ((size_t) kPts);
                const float norm = 4.0f / NF;
                for (int i = 0; i < kPts; ++i) r.spec[(size_t) i] = (float) (10 * std::log10 (acc[(size_t) i] / frames + 1e-30)) + juce::Decibels::gainToDecibels (norm) + 4.5f * std::log2 (ptFreq (i) / 1000.0f);
                // loudness integrata: blocchi da 400 ms al 75%, gate -70 e -10 LU
                double kbF[2][3], kaF[2][3];
                {
                    double f0 = 1681.974450955533, G = 3.999843853973347, Q = 0.7071752369554196, K = std::tan (juce::MathConstants<double>::pi * f0 / fsr);
                    const double Vh = std::pow (10.0, G / 20.0), Vb = std::pow (Vh, 0.4996667741545416), a0 = 1.0 + K / Q + K * K;
                    kbF[0][0] = (Vh + Vb * K / Q + K * K) / a0; kbF[0][1] = 2.0 * (K * K - Vh) / a0; kbF[0][2] = (Vh - Vb * K / Q + K * K) / a0; kaF[0][1] = 2.0 * (K * K - 1.0) / a0; kaF[0][2] = (1.0 - K / Q + K * K) / a0;
                    f0 = 38.13547087602444; Q = 0.5003270373238773; K = std::tan (juce::MathConstants<double>::pi * f0 / fsr);
                    kbF[1][0] = 1; kbF[1][1] = -2; kbF[1][2] = 1; kaF[1][1] = 2.0 * (K * K - 1.0) / (1.0 + K / Q + K * K); kaF[1][2] = (1.0 - K / Q + K * K) / (1.0 + K / Q + K * K);
                }
                const int hop = (int) (fsr * 0.1); std::vector<double> e100; double z[2][2][2] {}, acc100 = 0; int n100 = 0; float pk = 0; double eAll = 0;
                for (int i = 0; i < len; ++i)
                {
                    double e = 0;
                    for (int c = 0; c < 2; ++c)
                    {
                        double x = c ? R[i] : L[i]; pk = juce::jmax (pk, (float) std::abs (x)); eAll += x * x;
                        for (int s2 = 0; s2 < 2; ++s2) { const double y = kbF[s2][0] * x + z[s2][c][0]; z[s2][c][0] = kbF[s2][1] * x - kaF[s2][1] * y + z[s2][c][1]; z[s2][c][1] = kbF[s2][2] * x - kaF[s2][2] * y; x = y; }
                        e += x * x;
                    }
                    acc100 += e; if (++n100 >= hop) { e100.push_back (acc100 / n100); acc100 = 0; n100 = 0; }
                }
                std::vector<double> blocks; for (size_t k = 3; k < e100.size(); ++k) blocks.push_back ((e100[k] + e100[k - 1] + e100[k - 2] + e100[k - 3]) * 0.25);
                double s1 = 0; int n1 = 0; for (double v : blocks) if (-0.691 + 10 * std::log10 (v + 1e-20) > -70) { s1 += v; ++n1; }
                if (n1 > 0)
                {
                    const double gate = -0.691 + 10 * std::log10 (s1 / n1) - 10; double s2 = 0; int n2 = 0;
                    for (double v : blocks) if (-0.691 + 10 * std::log10 (v + 1e-20) > gate) { s2 += v; ++n2; }
                    if (n2 > 0) r.lufs = (float) (-0.691 + 10 * std::log10 (s2 / n2));
                }
                r.peak = juce::Decibels::gainToDecibels (pk, -100.0f); r.rms = (float) (10 * std::log10 (eAll / (2.0 * len) + 1e-20));
                r.ok = true;
            }
        }
        { const juce::SpinLock::ScopedLockType l (refLock); ref = std::move (r); }
        ++refVersion; refLoading = false;
    });
}
juce::AudioProcessorEditor* Processor::createEditor() { return new Editor (*this); }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new jm::Processor(); }
