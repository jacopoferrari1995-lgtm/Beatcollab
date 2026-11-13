#pragma once
#include <juce_dsp/juce_dsp.h>
#include "JackpotKit.h"

namespace jclp
{
enum Mode { SOFT, BUS, HARD, LOUD, CRISP, ASYM, TIGHT, FOLD, SINE, kModes };
inline const char* modeName (int m)
{
    static const char* n[] = { "SOFT", "BUS", "HARD", "LOUD", "CRISP", "ASIMM.", "TIGHT", "FOLDBACK", "SINE" };
    return n[juce::jlimit (0, kModes - 1, m)];
}
inline const char* modeDesc (int m)
{
    static const char* d[] = {
        "Morbido: arrotonda solo la parte alta, armoniche dispari basse. Il piu musicale per voci e synth.",
        "Taglio gentile da mix bus: ginocchio largo intorno al tetto, quasi invisibile. Per incollare il mix.",
        "Duro: taglia netto al tetto, niente sopra. Trasparente finche non lavora, poi armoniche alte.",
        "Loudness e drum: appiattisce i picchi e alza il corpo. Tante armoniche alte, suono pieno e forte.",
        "Crisp: i bassi vengono tagliati prima degli alti, cosi transienti e schiocchi restano nitidi.",
        "Asimmetrico: le due meta dell'onda tagliano diverso. Armoniche pari (seconda), caldo come una valvola.",
        "Tight: ginocchio corto e pulito, tiene la fondamentale. Per basso, 808 e master.",
        "Foldback: oltre il tetto l'onda si ripiega su se stessa. Suoni aggressivi e sintetici.",
        "Sine: curva a seno che si ripiega. Il piu ricco di armoniche di tutti, da sound design." };
    return d[juce::jlimit (0, kModes - 1, m)];
}

// curva di ogni modo; x normalizzato al tetto (1 = tetto)
inline float softK (float a, float k) { return a <= k ? a : k + (1.0f - k) * std::tanh ((a - k) / (1.0f - k)); }
inline float shapeMode (int m, float x)
{
    const float a = std::abs (x), s = x < 0 ? -1.0f : 1.0f;
    switch (m)
    {
        case SOFT:  return s * softK (a, 0.5f);
        case BUS:   { const float w = 0.35f; if (a < 1 - w) return x; if (a > 1 + w) return s; const float d = a - (1 - w); return s * (a - d * d / (4 * w)); }
        case HARD:  return juce::jlimit (-1.0f, 1.0f, x);
        case LOUD:  return s * juce::jmin (1.0f, a / (1.0f + 0.25f * a) * 1.25f);   // corpo +2 dB, cima piatta
        case CRISP: return s * softK (a, 0.9f);
        case ASYM:  return x >= 0 ? softK (a, 0.6f) : -0.82f * softK (a / 0.82f, 0.45f);
        case TIGHT: { if (a < 0.85f) return x; if (a > 1.15f) return s; const float d = a - 0.85f; return s * (a - d * d / 1.2f); }
        case FOLD:  { float y = a; int g = 0; while (y > 1.0f && g++ < 8) y = 2.0f - y; if (y < -1.0f) y = -1.0f; return s * y; }
        case SINE:  return std::sin (juce::jlimit (-12.0f, 12.0f, x));
        default:    return x;
    }
}

struct Settings
{
    float driveDb = 0, ceilDb = 0, punch = 0, mix = 1, outDb = 0, splitHz = 120;
    int mode = HARD, os = 2; bool split = false, link = false, delta = false;
};

// cronologia per il display: inviluppo di ingresso e uscita per colonna
struct History
{
    static constexpr int kCols = 360;
    std::atomic<float> in[kCols], out[kCols]; std::atomic<int> w { 0 };
    History() { for (int i = 0; i < kCols; ++i) { in[i] = 0; out[i] = 0; } }
};

class Engine
{
public:
    void prepare (double s, int maxBlock)
    {
        sr = s; maxN = maxBlock;
        for (int k = 0; k < 4; ++k)
        {
            if (k == 0) { ovs[k].reset(); continue; }
            ovs[k] = std::make_unique<juce::dsp::Oversampling<float>> (2, (size_t) k, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, true);
            ovs[k]->initProcessing ((size_t) maxBlock);
            ovsLo[k] = std::make_unique<juce::dsp::Oversampling<float>> (2, (size_t) k, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, true);
            ovsLo[k]->initProcessing ((size_t) maxBlock);
        }
        dry.setSize (2, maxBlock); lo.setSize (2, maxBlock); hi.setSize (2, maxBlock);
        for (auto& x : xo) x.reset();
        for (auto& b : crPre) b.reset();
        for (auto& b : crPost) b.reset();
        envF[0] = envF[1] = envS[0] = envS[1] = 0; dcX[0] = dcX[1] = dcY[0] = dcY[1] = 0;
        colN = juce::jmax (64, (int) (sr * 0.012)); colC = 0; colIn = colOut = 0;
    }
    int latency (int os) const { return os > 0 && ovs[os] ? (int) std::round (ovs[os]->getLatencyInSamples()) : 0; }
    void process (float* L, float* R, int n, const Settings& S, History& H)
    {
        const int osK = juce::jlimit (0, 3, S.os);
        const float drive = juce::Decibels::decibelsToGain (S.driveDb), ceil = juce::Decibels::decibelsToGain (S.ceilDb);
        const float outG = juce::Decibels::decibelsToGain (S.outDb - (S.link ? S.driveDb : 0.0f));
        float* ch[2] = { L, R };
        // dry (ritardato come il percorso sovracampionato per mix e delta)
        for (int c = 0; c < 2; ++c) dry.copyFrom (c, 0, ch[c], n);
        // punch: rilevatore di transienti prima del clipper
        const float aF = std::exp (-1.0f / (0.0015f * (float) sr)), aS = std::exp (-1.0f / (0.045f * (float) sr));
        float tr[2048]; const int nn = juce::jmin (n, 2048);
        for (int i = 0; i < nn; ++i)
        {
            const float a = juce::jmax (std::abs (L[i]), std::abs (R[i]));
            envF[0] = a > envF[0] ? a : envF[0] * aF + a * (1 - aF);
            envS[0] = envS[0] * aS + a * (1 - aS);
            const float t = juce::jlimit (0.0f, 1.0f, (envF[0] / (envS[0] + 1e-5f) - 1.0f) * 0.6f);
            tr[i] = std::pow (10.0f, (S.punch * t * 6.0f) / 20.0f);   // fino a +-6 dB sui transienti
        }
        for (int c = 0; c < 2; ++c) for (int i = 0; i < nn; ++i) ch[c][i] *= drive * tr[i] / ceil;
        // banda bassa e alta separate (SPLIT): ognuna con il suo clipper
        if (S.split)
        {
            for (auto& x : xo) x.set (sr, S.splitHz);
            for (int c = 0; c < 2; ++c) for (int i = 0; i < n; ++i) { float a, b; xo[c].split (ch[c][i], a, b); lo.setSample (c, i, a); hi.setSample (c, i, b); }
        }
        if (S.mode == CRISP) for (int c = 0; c < 2; ++c) { crPre[c].highShelf (sr * (1 << osK), 3500, -6.0, 0.7); crPost[c].highShelf (sr * (1 << osK), 3500, 6.0, 0.7); }
        auto clipBuf = [&] (float* const* data, int numCh, int len, int m, std::unique_ptr<juce::dsp::Oversampling<float>>* bank)
        {
            juce::dsp::AudioBlock<float> blk (data, (size_t) numCh, (size_t) len);
            if (osK > 0)
            {
                auto up = bank[osK]->processSamplesUp (blk);
                for (int c = 0; c < numCh; ++c)
                {
                    float* d = up.getChannelPointer ((size_t) c); const int N = (int) up.getNumSamples();
                    for (int i = 0; i < N; ++i) d[i] = sample (m, d[i], c);
                }
                bank[osK]->processSamplesDown (blk);
            }
            else for (int c = 0; c < numCh; ++c) for (int i = 0; i < len; ++i) data[c][i] = sample (m, data[c][i], c);
        };
        if (S.split)
        {
            // in modo SPLIT i bassi vanno sempre in TIGHT (pulito sulla fondamentale), gli alti nel modo scelto
            float* lp[2] = { lo.getWritePointer (0), lo.getWritePointer (1) }; float* hp2[2] = { hi.getWritePointer (0), hi.getWritePointer (1) };
            clipBuf (hp2, 2, n, S.mode, ovs);
            clipBuf (lp, 2, n, TIGHT, ovsLo);
            for (int c = 0; c < 2; ++c) for (int i = 0; i < n; ++i) ch[c][i] = lp[c][i] + hp2[c][i];
        }
        else clipBuf (ch, 2, n, S.mode, ovs);
        // continua (asimmetrico): via la componente DC
        const float dA = 1.0f - (float) (juce::MathConstants<double>::twoPi * 5.0 / sr);
        if (S.mode == ASYM) for (int c = 0; c < 2; ++c) for (int i = 0; i < n; ++i) { const float x = ch[c][i]; const float y = x - dcX[c] + dA * dcY[c]; dcX[c] = x; dcY[c] = y; ch[c][i] = y; }
        // latenza del dry
        const int lat = latency (osK);
        for (int c = 0; c < 2; ++c)
        {
            auto& dl = dline[c]; if ((int) dl.size() < 1 << 14) dl.assign (1 << 14, 0.0f);
            for (int i = 0; i < n; ++i) { dl[(size_t) dw[c]] = dry.getSample (c, i); dry.setSample (c, i, dl[(size_t) ((dw[c] - lat) & ((1 << 14) - 1))]); dw[c] = (dw[c] + 1) & ((1 << 14) - 1); }
        }
        float pkI = 0, pkO = 0;
        for (int i = 0; i < n; ++i)
        {
            for (int c = 0; c < 2; ++c)
            {
                // tetto sicuro e uscita
                float y = juce::jlimit (-1.0f, 1.0f, ch[c][i]) * ceil;
                const float d = dry.getSample (c, i);
                if (S.delta) y = d * drive - y;            // quello che il clipper toglie
                y = (y * S.mix + d * (1 - S.mix) * (S.link ? 1.0f : 1.0f)) * outG;
                ch[c][i] = y;
            }
            const float a = juce::jmax (std::abs (dry.getSample (0, i)), std::abs (dry.getSample (1, i))) * drive, b = juce::jmax (std::abs (L[i]), std::abs (R[i])) / outG;
            pkI = juce::jmax (pkI, a); pkO = juce::jmax (pkO, b);
            colIn = juce::jmax (colIn, a); colOut = juce::jmax (colOut, b);
            if (++colC >= colN) { const int w = H.w.load(); H.in[w] = colIn; H.out[w] = colOut; H.w = (w + 1) % History::kCols; colC = 0; colIn = colOut = 0; }
        }
        inPk = juce::jmax (pkI, inPk.load() * 0.9f); outPk = juce::jmax (pkO * outG, outPk.load() * 0.9f);
        const float red = pkI > 1e-5f && pkO > 1e-5f ? juce::jmax (0.0f, juce::Decibels::gainToDecibels (pkI / pkO)) : 0.0f;
        clipDb = juce::jmax (red, clipDb.load() * 0.92f);
    }
    std::atomic<float> inPk { 0 }, outPk { 0 }, clipDb { 0 };
private:
    inline float sample (int m, float x, int c)
    {
        if (m == CRISP) { x = crPre[c].p (x); return crPost[c].p (shapeMode (CRISP, x)); }
        return shapeMode (m, x);
    }
    double sr = 44100; int maxN = 512;
    std::unique_ptr<juce::dsp::Oversampling<float>> ovs[4], ovsLo[4];
    juce::AudioBuffer<float> dry, lo, hi;
    jkit::LR4 xo[2]; jkit::Bq crPre[2], crPost[2];
    float envF[2] {}, envS[2] {}, dcX[2] {}, dcY[2] {};
    std::vector<float> dline[2]; int dw[2] {};
    int colN = 512, colC = 0; float colIn = 0, colOut = 0;
};
}
