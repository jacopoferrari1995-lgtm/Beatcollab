#pragma once
// JACKPOT 808: la catena di Studio, condivisa da 808 STUDIO (effetto) e 808 SYNTH (strumento).
//  fase, transienti (PUNCH / SUSTAIN), SUB+ (sinusoide pulita agganciata alla nota), tono (SUB, KNOCK, TOP),
//  saturazione a 3 bande col sub pulito (6 caratteri, cambio sfumato), armoniche esatte, cassa in sidechain,
//  mono sui bassi, clipper, uscita. Tutti i comandi scivolano campione per campione.
#include "S8DSP.h"

namespace s8
{
struct Bq
{   // biquad RBJ in forma trasposta II (stato per due canali)
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1[2] {}, z2[2] {};
    inline float run (float x, int c) { const float y = b0 * x + z1[c]; z1[c] = b1 * x - a1 * y + z2[c]; z2[c] = b2 * x - a2 * y; return y; }
    void reset() { z1[0] = z1[1] = z2[0] = z2[1] = 0; }
    void set (int type, double sr, double f, double q, double db)
    {   // 0 lp, 1 hp, 2 peak, 3 lowshelf
        const double A = std::pow (10.0, db / 40.0), w = juce::MathConstants<double>::twoPi * juce::jlimit (5.0, sr * 0.45, f) / sr, cs = std::cos (w), sn = std::sin (w), al = sn / (2 * q);
        double B0 = 1, B1 = 0, B2 = 0, A0 = 1, A1 = 0, A2 = 0;
        switch (type)
        {
            case 0: B0 = (1 - cs) / 2; B1 = 1 - cs; B2 = B0; A0 = 1 + al; A1 = -2 * cs; A2 = 1 - al; break;
            case 1: B0 = (1 + cs) / 2; B1 = -(1 + cs); B2 = B0; A0 = 1 + al; A1 = -2 * cs; A2 = 1 - al; break;
            case 2: B0 = 1 + al * A; B1 = -2 * cs; B2 = 1 - al * A; A0 = 1 + al / A; A1 = -2 * cs; A2 = 1 - al / A; break;
            default:
            {
                const double sq = 2 * std::sqrt (A) * al;
                B0 = A * ((A + 1) - (A - 1) * cs + sq); B1 = 2 * A * ((A - 1) - (A + 1) * cs); B2 = A * ((A + 1) - (A - 1) * cs - sq);
                A0 = (A + 1) + (A - 1) * cs + sq; A1 = -2 * ((A - 1) + (A + 1) * cs); A2 = (A + 1) + (A - 1) * cs - sq;
            }
        }
        b0 = (float) (B0 / A0); b1 = (float) (B1 / A0); b2 = (float) (B2 / A0); a1 = (float) (A1 / A0); a2 = (float) (A2 / A0);
    }
};

// comandi della catena (valori reali), letti a ogni blocco
struct ChainSet
{
    bool inv = false; float punch = 0, sustain = 0, subPlus = 0, subDb = 0, knock = 0, knockHz = 160, topHz = 20000, drive = 0; int mode = 0;
    float harm = 0, duck = 0, rel = 120; int mono = 0; float ceil = 0, out = 0;
};

inline const char* modeName6 (int m) { static const char* n[6] = { "TAPE", "TUBE", "HARD", "FUZZ", "SPINZ", "RAGE" }; return n[juce::jlimit (0, 5, m)]; }
inline float sat6 (float x, int mode)
{
    switch (mode)
    {
        case 4: { const float y = x + 0.45f * std::abs (x); return std::tanh (y * 1.2f) * 0.8f - 0.05f * std::tanh (x * 3.0f); }   // SPINZ: grana asimmetrica, sporca
        case 5: { const float y = x * 1.7f, a = std::abs (y); return y / std::pow (1.0f + a * a * a * a * a * a, 1.0f / 6.0f) * 0.62f + 0.05f * std::tanh (x * x * 2.0f); }   // RAGE: dura e piena, senza spigoli vivi
        default: return sat (x, mode);
    }
}

class Chain
{
public:
    // ---- analisi per la grafica e la diagnosi (lette dall'editor)
    std::atomic<float> uiHz { 0 }, uiConf { 0 }, uiLevel { -120 };
    std::atomic<float> pcHist[12];
    std::atomic<float> centsAvg { 0 }, f0Avg { 0 }; std::atomic<int> pitchN { 0 };
    std::atomic<float> eSub { 0 }, eBody { 0 }, eHigh { 0 };
    std::atomic<float> sideRatio { -100 }, inPeak { -120 }, crest { 0 };
    std::atomic<int> clips { 0 }, kickHits { 0 };
    std::atomic<float> kickCorr { 0 }, overlapDb { -100 };
    std::atomic<bool> scLive { false };
    std::atomic<float> uiDuck { 1 }, uiGrClip { 0 }, uiSubPlus { 0 }, outPeak { -120 };
    static constexpr int kHist = 300, kRing = 8192;
    std::atomic<float> hLvl[kHist], hKick[kHist], hDuck[kHist]; std::atomic<int> hW { 0 };
    float ring[kRing] {}; std::atomic<int> ringW { 0 };
    std::atomic<bool> resetReq { false };

    Chain() { for (auto& h : pcHist) h = 0; for (int i = 0; i < kHist; ++i) { hLvl[i] = 0; hKick[i] = 0; hDuck[i] = 1; } }

    int latency() const { return os ? juce::roundToInt (os->getLatencyInSamples()) : 0; }
    void prepare (double s, int block)
    {
        sr = s; maxB = juce::jmax (block, 4096);
        os = std::make_unique<juce::dsp::Oversampling<float>> (2, 3, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false);
        os->initProcessing ((size_t) maxB);
        const double osr = sr * osF; const juce::dsp::ProcessSpec so { osr, (juce::uint32) (maxB * osF), 2 }, sb { sr, (juce::uint32) maxB, 2 };
        auto lr = [] (juce::dsp::LinkwitzRileyFilter<float>& f, juce::dsp::LinkwitzRileyFilterType t, float hz, const juce::dsp::ProcessSpec& sp) { f.setType (t); f.setCutoffFrequency (hz); f.prepare (sp); f.reset(); };
        using LT = juce::dsp::LinkwitzRileyFilterType;
        lr (xLo, LT::lowpass, 90, so); lr (xLoH, LT::highpass, 90, so); lr (xHi, LT::lowpass, 400, so); lr (xHiH, LT::highpass, 400, so); lr (apL, LT::lowpass, 400, so); lr (apH, LT::highpass, 400, so);
        lr (hHp, LT::highpass, 85, so); lr (sideLp, LT::lowpass, 120, sb); lr (midLp, LT::lowpass, 120, sb);
        for (int k = 0; k < 4; ++k) { lr (monoLp[k], LT::lowpass, monoHz[k + 1], sb); }
        tuner.prepare (sr);
        auto lp150 = juce::dsp::IIR::Coefficients<float>::makeLowPass (sr, 150.0f, 0.707f); kLp.coefficients = lp150; bLp.coefficients = lp150; kLp.reset(); bLp.reset();
        sp1.set (0, sr, 110, 0.54, 0); sp2.set (0, sr, 110, 1.31, 0); sp1.reset(); sp2.reset();
        for (auto* q : { &shelf, &bell, &topF }) q->reset();
        init = false; envF = envS = 0; kEnv = kSlow = 0; duckG = 1; kWin = 0; hEnv = 0; zcLast = 0; zcCnt = 0; zcPer = 0; subPh = 0; subEnv = 0; subF = 0; subG = 0; prevS = 0;
        mCur = mOld = 0; mX = 1; monoCur = 0; monoW = 0; tick = 0;
        dcL = dcR = 0; dcK = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 5.0f / (float) sr);
        k20 = 1.0f - std::exp (-1.0f / (0.020f * (float) sr)); k50 = 1.0f - std::exp (-1.0f / (0.050f * (float) sr)); k10 = 1.0f - std::exp (-1.0f / (0.004f * (float) sr)); k20o = 1.0f - std::exp (-1.0f / (0.020f * (float) osr));
    }

    void process (float* L, float* R, int N, const float* kL, const float* kR, const ChainSet& S)
    {
        if (resetReq.exchange (false))
        {
            for (auto& a : sE) a = 0; sSideLo = sMidLo = 0; sN = 0; pk = 0; crestAvg = 0; for (auto& a : pcAcc) a = 0; centsAcc = logF0Acc = 0; pN = 0; corrAvg = 0; ovAvg = -100; hits = clipN = 0;
            for (auto& h : pcHist) h = 0; clips = 0; kickHits = 0; pitchN = 0;
        }
        if (! init)
        {   // partenza: i comandi sono gia' dove stanno, nessuna salita
            init = true; sm = S; invG = S.inv ? -1.0f : 1.0f; mCur = mOld = S.mode; mX = 1; monoCur = S.mono; monoW = S.mono > 0 ? 1.0f : 0.0f;
            design (true);
        }
        const bool scOn = kL != nullptr;
        const float fA = 1 - std::exp (-1.0f / (0.001f * (float) sr)), fR = 1 - std::exp (-1.0f / (0.030f * (float) sr));
        const float sA = 1 - std::exp (-1.0f / (0.030f * (float) sr)), sR = 1 - std::exp (-1.0f / (0.250f * (float) sr));
        const float kA = 1 - std::exp (-1.0f / (0.0005f * (float) sr)), kRl = 1 - std::exp (-1.0f / (0.06f * (float) sr));
        const float kSl = 1 - std::exp (-1.0f / (0.2f * (float) sr)), pkDecay = std::exp (-1.0f / (3.0f * (float) sr));
        const float eA = 1 - std::exp (-1.0f / (0.002f * (float) sr)), eR = 1 - std::exp (-1.0f / (0.045f * (float) sr));
        bool live = false;
        for (int i = 0; i < N; ++i)
        {
            // comandi che scivolano (20 ms)
            sm.punch += (S.punch - sm.punch) * k20; sm.sustain += (S.sustain - sm.sustain) * k20; sm.subPlus += (S.subPlus - sm.subPlus) * k20;
            sm.duck += (S.duck - sm.duck) * k20; sm.rel += (S.rel - sm.rel) * k20; sm.out += (S.out - sm.out) * k50;
            if ((tick++ & 15u) == 0)
            {
                const float a = sm.subDb, b = sm.knock, c = sm.knockHz, d = sm.topHz;
                sm.subDb += (S.subDb - sm.subDb) * k20 * 16; sm.knock += (S.knock - sm.knock) * k20 * 16;
                sm.knockHz *= std::pow (S.knockHz / sm.knockHz, juce::jmin (1.0f, k20 * 16)); sm.topHz *= std::pow (S.topHz / sm.topHz, juce::jmin (1.0f, k20 * 16));
                if (std::abs (a - sm.subDb) > 1e-4f || std::abs (b - sm.knock) > 1e-4f || std::abs (c - sm.knockHz) > 1e-3f || std::abs (d - sm.topHz) > 1e-2f) design (false);
            }
            invG += ((S.inv ? -1.0f : 1.0f) - invG) * k10; L[i] *= invG; R[i] *= invG;   // passa per lo zero in 10 ms
            const float m = 0.5f * (L[i] + R[i]);
            pk = juce::jmax (pk, std::abs (L[i]), std::abs (R[i])); rmsAcc += m * m; ++rmsN; winPk = juce::jmax (winPk, std::abs (m));
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
            // ---- SUB+: sinusoide pulita agganciata alla fondamentale (passaggi per lo zero del sub) e al suo inviluppo
            {
                const float s = sp2.run (sp1.run (m, 0), 0), as = std::abs (s);
                subEnv += (as > subEnv ? eA : eR) * (as - subEnv);
                ++zcCnt;
                if (prevS < 0 && s >= 0)
                {
                    const float fr = prevS / (prevS - s), per = (float) zcCnt - 1.0f + fr - zcLast;   // periodo frazionario
                    zcLast = fr; zcCnt = 0;
                    if (per > sr / 230.0 && per < sr / 24.0 && subEnv > 1e-3f)
                    {
                        const float f = (float) sr / per; subF = subF < 20 ? f : subF + 0.5f * (f - subF);
                        float e = (float) (subPh - std::floor (subPh + 0.5)); subPh -= 0.35 * e;   // aggancio di fase
                    }
                }
                prevS = s;
                const float tgtG = subF > 20 ? 1.0f : 0.0f; subG += (tgtG - subG) * eR;
                subPh += subF / sr; if (subPh >= 1) subPh -= 1;
                const float add = (float) std::sin (juce::MathConstants<double>::twoPi * subPh) * subEnv * subG * sm.subPlus * 1.2f;
                L[i] += add; R[i] += add;
                if (sm.subPlus > 0.01f) uiSubPlus = subF;
            }
            // ---- tono
            L[i] = topF.run (bell.run (shelf.run (L[i], 0), 0), 0); R[i] = topF.run (bell.run (shelf.run (R[i], 1), 1), 1);
            // ---- transienti
            const float mm = 0.5f * (L[i] + R[i]), a = std::abs (mm);
            envF += (a > envF ? fA : fR) * (a - envF); envS += (a > envS ? sA : sR) * (a - envS);
            float gdb = 0;
            if (envS > 1e-5f)
            {
                const float tr = juce::jlimit (0.0f, 1.0f, (juce::Decibels::gainToDecibels (envF + 1e-9f) - juce::Decibels::gainToDecibels (envS + 1e-9f)) / 9.0f);
                gdb = sm.punch * 9.0f * tr + sm.sustain * (sm.sustain > 0 ? 6.0f : 9.0f) * (1.0f - tr) * juce::jlimit (0.0f, 1.0f, (juce::Decibels::gainToDecibels (envS) + 50.0f) / 20.0f);
            }
            // ---- cassa in sidechain
            float dg = 1;
            const float dR = 1 - std::exp (-1.0f / (sm.rel * 0.001f * (float) sr));
            if (scOn)
            {
                const float km = 0.5f * (kL[i] + kR[i]); const float kl = kLp.processSample (km), bl = bLp.processSample (mm);
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
                colK = juce::jmax (colK, kEnv / (kPeak + 1e-6f));
            }
            const float tgt = scOn ? 1.0f - sm.duck * 0.9f * juce::jlimit (0.0f, 1.0f, kEnv / (kPeak + 1e-6f) * 1.4f) : 1.0f;
            duckG += (tgt < duckG ? kA * 4 : dR) * (tgt - duckG); dg = duckG;
            const float g = juce::Decibels::decibelsToGain (gdb) * dg;
            L[i] *= g; R[i] *= g;
        }
        if (live) liveHold = (int) (2.0 * sr); else liveHold = juce::jmax (0, liveHold - N);
        scLive = liveHold > 0; uiDuck = duckG;
        inPeak = juce::Decibels::gainToDecibels (pk + 1e-9f); pk *= 0.9f;

        // ---- sovracampionato: bande, saturazione (cambio di carattere sfumato), armoniche, clipper
        float* chans[2] = { L, R }; juce::dsp::AudioBlock<float> blk (chans, 2, (size_t) N);
        auto ob = os->processSamplesUp (blk);
        const int M = (int) ob.getNumSamples();
        if (S.mode != mCur && mX >= 1.0f) { mOld = mCur; mCur = S.mode; mX = 0; }
        const float hA = 1 - std::exp (-1.0f / (0.0005f * (float) (sr * osF))), hR = 1 - std::exp (-1.0f / (0.04f * (float) (sr * osF)));
        const float xStep = 1.0f / (float) (0.03 * sr * osF);
        float* o[2] = { ob.getChannelPointer (0), ob.getChannelPointer (1) };
        float gr = 0; int clipNow = 0;
        for (int i = 0; i < M; ++i)
        {
            sm.drive += (S.drive - sm.drive) * k20o; sm.harm += (S.harm - sm.harm) * k20o; sm.ceil += (S.ceil - sm.ceil) * k20o;
            const float drive = sm.drive, pre = 1.0f + drive * drive * 14.0f, comp = std::pow (pre, -0.8f), wet = juce::jmin (1.0f, drive * 40.0f);
            const float ceil = juce::Decibels::decibelsToGain (sm.ceil);
            if (mX < 1.0f) mX = juce::jmin (1.0f, mX + xStep);
            const float wN = 0.5f - 0.5f * std::cos (mX * juce::MathConstants<float>::pi);
            float subM = 0;
            for (int c = 0; c < 2; ++c)
            {
                const float x = o[c][i];
                const float sub0 = xLo.processSample (c, x), rest = xLoH.processSample (c, x);
                const float sub = apL.processSample (c, sub0) + apH.processSample (c, sub0);   // stessa fase delle altre bande: somma piatta
                const float body = xHi.processSample (c, rest), high = xHiH.processSample (c, rest);
                if ((i & 3) == 0) { sE[0] += (double) sub * sub; sE[1] += (double) body * body; sE[2] += (double) high * high; }
                float bS = body, hS = high;
                if (wet > 0.0f)
                {
                    float b1 = sat6 (body * pre, mCur) * comp, h1 = sat6 (high * pre * 1.2f, mCur) * comp * 0.9f;
                    if (wN < 1.0f) { const float b0 = sat6 (body * pre, mOld) * comp, h0 = sat6 (high * pre * 1.2f, mOld) * comp * 0.9f; b1 = b0 + (b1 - b0) * wN; h1 = h0 + (h1 - h0) * wN; }
                    bS = body + (b1 - body) * wet; hS = high + (h1 - high) * wet;
                }
                o[c][i] = sub + bS + hS; subM += 0.5f * sub;
            }
            if ((i & 3) == 0) ++sN;
            {   // armoniche esatte del sub (T2 e T3 di Chebyshev sul segnale normalizzato all'inviluppo); filtro sempre caldo
                const float a = std::abs (subM); hEnv += (a > hEnv ? hA : hR) * (a - hEnv);
                const float xn = juce::jlimit (-1.0f, 1.0f, subM / (hEnv + 1e-5f));
                const float h = hEnv * (0.55f * (2 * xn * xn - 1) + 0.45f * (4 * xn * xn * xn - 3 * xn)) * sm.harm * 1.6f;
                for (int c = 0; c < 2; ++c) o[c][i] += hHp.processSample (c, h);
            }
            for (int c = 0; c < 2; ++c)
            {   // clipper morbido al ceiling
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
        // ---- mono sui bassi (cambio sfumato), uscita, statistiche
        if (S.mono != monoCur && monoW <= 0.0f) monoCur = S.mono;
        const float mT = (S.mono == monoCur && monoCur > 0) ? 1.0f : 0.0f, mStep = 1.0f / (float) (0.03 * sr);
        float opk = 0;
        for (int i = 0; i < N; ++i)
        {
            float m = 0.5f * (L[i] + R[i]), s = 0.5f * (L[i] - R[i]);
            const float sl = sideLp.processSample (0, s), ml = midLp.processSample (0, m); sSideLo += (double) sl * sl; sMidLo += (double) ml * ml;
            float lows[4]; for (int k = 0; k < 4; ++k) lows[k] = monoLp[k].processSample (0, s);   // filtri sempre caldi
            monoW = mT > monoW ? juce::jmin (1.0f, monoW + mStep) : juce::jmax (0.0f, monoW - mStep);
            if (monoCur > 0) s -= lows[monoCur - 1] * monoW;
            const float og = juce::Decibels::decibelsToGain (sm.out);
            float l = (m + s), r = (m - s);
            // la saturazione asimmetrica crea continua: filtro a 5 Hz, sempre acceso
            dcL += (l - dcL) * dcK; dcR += (r - dcR) * dcK; l -= dcL; r -= dcR;
            L[i] = l * og; R[i] = r * og; opk = juce::jmax (opk, std::abs (L[i]), std::abs (R[i]));
            const float mo = 0.5f * (L[i] + R[i]);
            const int w = ringW.load(); ring[w] = mo; ringW = (w + 1) & (kRing - 1);
            colL = juce::jmax (colL, std::abs (L[i]), std::abs (R[i])); colD = juce::jmin (colD, duckG);
            if (++colN >= (int) (sr * 0.02)) { const int hw = hW.load(); hLvl[hw] = colL; hKick[hw] = colK; hDuck[hw] = colD; hW = (hw + 1) % kHist; colL = colK = 0; colD = 1; colN = 0; }
        }
        outPeak = juce::jmax (juce::Decibels::gainToDecibels (opk + 1e-9f), outPeak.load() - 1.5f);
        if (sN > 0) { eSub = (float) (10 * std::log10 (sE[0] / sN + 1e-14)); eBody = (float) (10 * std::log10 (sE[1] / sN + 1e-14)); eHigh = (float) (10 * std::log10 (sE[2] / sN + 1e-14)); }
        if (sMidLo > 1e-9) sideRatio = (float) (10 * std::log10 (sSideLo / sMidLo + 1e-14));
    }
    double sampleRate() const { return sr; }

private:
    void design (bool)
    {
        shelf.set (3, sr, 70, 0.7, sm.subDb); bell.set (2, sr, sm.knockHz, 1.2, sm.knock); topF.set (0, sr, juce::jmin (sm.topHz, (float) (sr * 0.45)), 0.707, 0);
    }
    static constexpr float monoHz[5] = { 0, 60, 90, 120, 150 };
    double sr = 48000; int osF = 8, maxB = 4096; bool init = false; unsigned tick = 0;
    ChainSet sm; float k20 = 0.001f, k20o = 0.001f, k50 = 0.001f, k10 = 0.01f, invG = 1;
    std::unique_ptr<juce::dsp::Oversampling<float>> os;
    juce::dsp::LinkwitzRileyFilter<float> xLo, xLoH, xHi, xHiH, apL, apH, hHp, sideLp, midLp, monoLp[4];
    Tuner tuner; Bq sp1, sp2, shelf, bell, topF;
    float envF = 0, envS = 0;
    juce::dsp::IIR::Filter<float> kLp, bLp; float kEnv = 0, kSlow = 0, kPeak = 1e-3f, duckG = 1; int kWin = 0; double kb = 0, kk = 0, bb = 0; float bEnvAtHit = 0, kMaxWin = 0, bEnv = 0;
    float hEnv = 0;
    float prevS = 0, zcLast = 0, subEnv = 0, subF = 0, subG = 0; int zcCnt = 0; float zcPer = 0; double subPh = 0;
    int mCur = 0, mOld = 0, monoCur = 0; float mX = 1, monoW = 0, dcL = 0, dcR = 0, dcK = 0.0007f;
    double sE[3] {}, sSideLo = 0, sMidLo = 0; juce::int64 sN = 0; float pk = 0, winPk = 0, rmsAcc = 0; int rmsN = 0; float crestAvg = 0;
    double pcAcc[12] {}; double centsAcc = 0, logF0Acc = 0; int pN = 0; float corrAvg = 0; float ovAvg = -100; int hits = 0, clipN = 0;
    float colL = 0, colK = 0, colD = 1; int colN = 0, liveHold = 0;
};
}
