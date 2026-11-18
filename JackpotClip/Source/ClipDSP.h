#pragma once
#include <juce_dsp/juce_dsp.h>
#include "JackpotKit.h"
#include <array>

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
        case TIGHT: { if (a < 0.85f) return x; if (a > 1.15f) return s; const float d = a - 0.85f; return s * (a - d * d / 0.6f); }
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
        sr = s; maxN = juce::jmax (64, maxBlock);
        for (int k = 0; k < 4; ++k)
        {
            if (k == 0) { ovs[k].reset(); intLat[0] = kLa - 1; continue; }
            ovs[k] = std::make_unique<juce::dsp::Oversampling<float>> (2, (size_t) k, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, true);
            ovs[k]->initProcessing ((size_t) maxN);
            // ritardo vero del sovracampionatore (non intero): misurato con un impulso e portato a un numero intero con un ritardo
            // frazionario a fase lineare. Cosi' latenza dichiarata = vera, e DELTA/MIX si sommano al pulito senza residui
            const double lt = measureLatency (k);
            intLat[k] = (int) std::ceil (lt) + kFdHalf + kLa - 1;
            const double d = intLat[k] - (kLa - 1) - lt;   // ritardo del filtro frazionario, tra kFdHalf e kFdHalf+1
            double sum = 0;
            for (int t = 0; t < kFd; ++t)
            {
                const double x = t - d, w = 0.42 - 0.5 * std::cos (2 * juce::MathConstants<double>::pi * (t + 0.5 - (d - kFdHalf)) / kFd) + 0.08 * std::cos (4 * juce::MathConstants<double>::pi * (t + 0.5 - (d - kFdHalf)) / kFd);
                const double sn = std::abs (x) < 1e-9 ? 1.0 : std::sin (juce::MathConstants<double>::pi * x) / (juce::MathConstants<double>::pi * x);
                fd[k][t] = (float) (sn * w); sum += sn * w;
            }
            for (int t = 0; t < kFd; ++t) fd[k][t] = (float) (fd[k][t] / sum);
            ovs[k]->reset();
        }
        for (auto* b : { &dry, &lo, &hi, &full, &spl }) b->setSize (2, maxN);
        for (auto* v : { &gArr, &dArr, &cArr, &oArr, &mxArr, &mdArr, &spArr, &dlArr, &gmArr }) v->assign ((size_t) maxN, 0.0f);
        for (auto& x : xo) x.reset(); for (auto& x : xoD) x.reset(); for (auto& b : crPre) b.reset(); for (auto& b : crPost) b.reset(); for (auto& b : crPreH) b.reset(); for (auto& b : crPostH) b.reset();
        envF[0] = envF[1] = envS[0] = envS[1] = 0; dcX[0] = dcX[1] = dcY[0] = dcY[1] = 0;
        for (auto& r : fdr) for (auto& x : r) x = 0; fdw = 0;
        for (auto& r : la) r.fill (0.0f); greq.fill (1.0f); hold.fill (1.0f); holdSum = (double) kLa; law = 0; lg = 1.0f;
        for (auto& d : dline) d.assign (1 << 14, 0.0f); dw = 0;
        colN = juce::jmax (64, (int) (sr * 0.012)); relK = (float) (1.0 - std::exp (-1.0 / (0.06 * sr))); holdN = (int) (0.3 * sr);   // piu' lungo del rilascio della sicura colC = 0; colIn = colOut = 0;
        first = true; T = 0; clipFrom = clipUntil = -1; gm = 0;
    }
    int latency (int os) const { return intLat[juce::jlimit (0, 3, os)]; }
    void process (float* L, float* R, int n, const Settings& S, History& H)
    {
        const int osK = juce::jlimit (0, 3, S.os);
        if (osK != lastOs) { lastOs = osK; for (auto& r : fdr) for (auto& x : r) x = 0; }
        const int lat = intLat[osK];
        float* ch[2] = { L, R };
        // valori che scivolano campione per campione (20 ms): niente click su drive, tetto, punch, mix, uscita, link
        const float kS = (float) (1.0 - std::exp (-1.0 / (0.02 * sr))), step = 1.0f / (float) (0.02 * sr);
        const float outT = S.outDb - (S.link ? S.driveDb : 0.0f);
        if (first) { dS = S.driveDb; cS = S.ceilDb; oS = outT; mxS = S.mix; pS = S.punch; hzS = S.splitHz; modeA = modeB = S.mode; modeX = 1; spX = S.split ? 1.0f : 0.0f; dlX = S.delta ? 1.0f : 0.0f; first = false; for (auto& x : xoD) x.set (sr, hzS); }
        if (S.mode != modeB && modeX >= 1.0f) { modeA = modeB; modeB = S.mode; modeX = 0; }   // cambio modo: dissolvenza fra le due curve
        if (std::abs (S.splitHz - hzS) > 0.05f) { hzS += (S.splitHz - hzS) * juce::jmin (1.0f, kS * (float) n); if (std::abs (S.splitHz - hzS) < 0.5f) hzS = S.splitHz; for (auto& x : xoD) x.set (sr, hzS); }
        const float aF = std::exp (-1.0f / (0.0015f * (float) sr)), aS = std::exp (-1.0f / (0.045f * (float) sr));
        for (int c = 0; c < 2; ++c) dry.copyFrom (c, 0, ch[c], n);
        for (int i = 0; i < n; ++i)
        {
            auto sm = [kS] (float& v, float t, float eps) { v += (t - v) * kS; if (std::abs (t - v) < eps) v = t; };
            sm (dS, S.driveDb, 1e-4f); sm (cS, S.ceilDb, 1e-4f); sm (oS, outT, 1e-4f); sm (mxS, S.mix, 1e-6f); sm (pS, S.punch, 1e-6f);
            modeX = juce::jmin (1.0f, modeX + step);
            spX = S.split ? juce::jmin (1.0f, spX + step) : juce::jmax (0.0f, spX - step);
            dlX = S.delta ? juce::jmin (1.0f, dlX + step) : juce::jmax (0.0f, dlX - step);
            // punch: rilevatore di transienti prima del clipper
            const float a = juce::jmax (std::abs (L[i]), std::abs (R[i]));
            envF[0] = a > envF[0] ? a : envF[0] * aF + a * (1 - aF);
            envS[0] = envS[0] * aS + a * (1 - aS);
            const float t = juce::jlimit (0.0f, 1.0f, (envF[0] / (envS[0] + 1e-5f) - 1.0f) * 0.6f);
            const float tr = pS != 0.0f ? std::pow (10.0f, (pS * t * 6.0f) / 20.0f) : 1.0f;   // fino a +-6 dB sui transienti
            const float drive = juce::Decibels::decibelsToGain (dS), ceil = juce::Decibels::decibelsToGain (cS);
            gArr[(size_t) i] = drive * tr / ceil; dArr[(size_t) i] = drive; cArr[(size_t) i] = ceil; oArr[(size_t) i] = juce::Decibels::decibelsToGain (oS);
            mxArr[(size_t) i] = mxS; mdArr[(size_t) i] = modeX; spArr[(size_t) i] = spX; dlArr[(size_t) i] = dlX;
            for (int c = 0; c < 2; ++c) { const float x = ch[c][i] * gArr[(size_t) i]; full.setSample (c, i, x); ch[c][i] = x; }
            // porta automatica (neutro al bit quando il clipper non tocca niente): se un picco supera il ginocchio della curva,
            // il percorso sovracampionato entra prima che quel campione esca (c'e' la latenza come anticipo)
            const bool canPass = ! forceWet && pS == 0.0f && spX == 0.0f && dlX == 0.0f && identityKnee (modeA) > 0 && identityKnee (modeB) > 0;
            const float knee = juce::jmin (identityKnee (modeA), identityKnee (modeB)) * 0.98f;
            const juce::int64 Ti = T + i;
            if (! canPass) { clipFrom = juce::jmin (clipFrom < 0 ? Ti : clipFrom, Ti); clipUntil = juce::jmax (clipUntil, Ti + lat + holdN); }
            else if (juce::jmax (std::abs (ch[0][i]), std::abs (ch[1][i])) >= knee)
            {
                if (clipUntil < Ti) clipFrom = Ti;   // si apre subito: copre anche il pre-ringing dei filtri
                clipUntil = juce::jmax (clipUntil, Ti + lat + holdN);
            }
            const bool want = clipFrom >= 0 && Ti >= clipFrom && Ti <= clipUntil;
            gm = want ? juce::jmin (1.0f, gm + 1.0f / 32.0f) : juce::jmax (0.0f, gm - 1.0f / 1024.0f);
            gmArr[(size_t) i] = gm;
        }
        const bool CR = modeA == CRISP || modeB == CRISP;
        if (CR) for (int c = 0; c < 2; ++c) { crPre[c].highShelf (sr * (1 << osK), 3500, -6.0, 0.7); crPost[c].highShelf (sr * (1 << osK), 3500, 6.0, 0.7); }
        // SPLIT dentro il sovracampionamento: crossover alla frequenza alta, bassi in TIGHT, alti nel modo scelto,
        // poi la somma passa ancora per un ginocchio TIGHT: le due bande non possono sommarsi sopra il tetto
        const int f = 1 << osK;
        const bool anySplit = spArr[0] > 0.0f || spArr[(size_t) n - 1] > 0.0f || S.split;
        if (anySplit) for (auto& x : xo) x.set (sr * f, hzS); else for (auto& x : xo) x.reset();
        if (CR) for (int c = 0; c < 2; ++c) { crPreH[c].highShelf (sr * f, 3500, -6.0, 0.7); crPostH[c].highShelf (sr * f, 3500, 6.0, 0.7); }
        auto shapeRun = [&] (float* d, int N, int c)
        {
            for (int i = 0; i < N; ++i)
            {
                const int idx = i / f; const float w = mdArr[(size_t) idx], sw = spArr[(size_t) idx], x = d[i];
                float y = w >= 1.0f ? sample (modeB, x, c, false) : sample (modeA, x, c, false) * (1 - w) + sample (modeB, x, c, false) * w;
                if (sw > 0.0f)
                {
                    float lo, hi; xo[c].split (x, lo, hi);
                    const float h = w >= 1.0f ? sample (modeB, hi, c, true) : sample (modeA, hi, c, true) * (1 - w) + sample (modeB, hi, c, true) * w;
                    const float ys = shapeMode (TIGHT, shapeMode (TIGHT, lo) + h);
                    y += (ys - y) * sw;
                }
                d[i] = y;
            }
        };
        if (osK > 0)
        {
            juce::dsp::AudioBlock<float> blk (ch, 2, (size_t) n);
            auto up = ovs[osK]->processSamplesUp (blk);
            for (int c = 0; c < 2; ++c) shapeRun (up.getChannelPointer ((size_t) c), (int) up.getNumSamples(), c);
            ovs[osK]->processSamplesDown (blk);
        }
        else for (int c = 0; c < 2; ++c) shapeRun (ch[c], n, c);
        // continua (asimmetrico): via la componente DC
        const float dA = 1.0f - (float) (juce::MathConstants<double>::twoPi * 5.0 / sr);
        if (modeA == ASYM || modeB == ASYM) for (int c = 0; c < 2; ++c) for (int i = 0; i < n; ++i) { const float x = ch[c][i]; const float y = x - dcX[c] + dA * dcY[c]; dcX[c] = x; dcY[c] = y; ch[c][i] = y; }
        else { dcX[0] = dcX[1] = dcY[0] = dcY[1] = 0; }
        float pkI = 0, pkO = 0;
        const int DM = (1 << 14) - 1;
        for (int i = 0; i < n; ++i)
        {
            // ritardo frazionario sul percorso tagliato: la latenza diventa un numero intero esatto
            float w[2] = { ch[0][i], ch[1][i] };
            if (osK > 0)
            {
                for (int c = 0; c < 2; ++c) fdr[c][(size_t) fdw] = w[c];
                for (int c = 0; c < 2; ++c) { float acc = 0; int r = fdw; for (int t = 0; t < kFd; ++t) { acc += fd[osK][t] * fdr[c][(size_t) r]; r = (r - 1) & (kFd - 1); } w[c] = acc; }
                fdw = (fdw + 1) & (kFd - 1);
            }
            float dd[2];
            for (int c = 0; c < 2; ++c) { dline[c][(size_t) dw] = dry.getSample (c, i); dd[c] = dline[c][(size_t) ((dw - lat) & DM)]; }
            dw = (dw + 1) & DM;
            // con SPLIT il percorso tagliato passa per il crossover (somma = passa-tutto): stesso giro di fase sul pulito, niente buchi col MIX
            if (spArr[(size_t) i] > 0.0f) for (int c = 0; c < 2; ++c) { float a, b; xoD[c].split (dd[c], a, b); dd[c] += (a + b - dd[c]) * spArr[(size_t) i]; }
            else { xoD[0].reset(); xoD[1].reset(); }
            const float drive = dArr[(size_t) i], ceil = cArr[(size_t) i], g = gmArr[(size_t) i], mix = mxArr[(size_t) i], og = oArr[(size_t) i], dX = dlArr[(size_t) i];
            // sicura sotto 0 dBFS: il filtro di uscita del sovracampionamento puo' superare il tetto (Gibbs).
            // Non e' una curva a 1x (farebbe aliasing): e' un limitatore con anticipo, guadagno liscio, stereo collegato
            float lw[2];
            {
                const float y0 = w[0] * ceil, y1 = w[1] * ceil, pk = juce::jmax (std::abs (y0), std::abs (y1));
                const float gr = pk > kSafe ? kSafe / pk : 1.0f;
                la[0][(size_t) law] = y0; la[1][(size_t) law] = y1; greq[(size_t) law] = gr;
                float h = 1.0f; for (float v : greq) h = juce::jmin (h, v);
                holdSum += h - hold[(size_t) law]; hold[(size_t) law] = h;
                const float av = juce::jmin (1.0f, (float) (holdSum / kLa));
                lg = av < lg ? av : lg + (av - lg) * relK;
                const int rd = (law + 1) & (kLa - 1);   // il campione piu' vecchio: L-1 campioni fa
                lw[0] = la[0][(size_t) rd] * lg; lw[1] = la[1][(size_t) rd] * lg;
                law = rd;
            }
            for (int c = 0; c < 2; ++c)
            {
                float y = lw[c];
                const float pass = dd[c] * drive;          // quello che uscirebbe se il clipper non toccasse niente
                if (g < 1.0f) y = pass + (y - pass) * g;
                if (dX > 0.0f) y += ((pass - y) - y) * dX;   // DELTA: quello che il clipper toglie
                ch[c][i] = (mix >= 1.0f ? y : y * mix + dd[c] * (1 - mix)) * og;
            }
            const float a = juce::jmax (std::abs (dd[0]), std::abs (dd[1])) * drive, b = juce::jmax (std::abs (L[i]), std::abs (R[i])) / og;
            pkI = juce::jmax (pkI, a); pkO = juce::jmax (pkO, b);
            colIn = juce::jmax (colIn, a); colOut = juce::jmax (colOut, b);
            if (++colC >= colN) { const int wi = H.w.load(); H.in[wi] = colIn; H.out[wi] = colOut; H.w = (wi + 1) % History::kCols; colC = 0; colIn = colOut = 0; }
        }
        T += n;
        const float og = juce::Decibels::decibelsToGain (oS);
        inPk = juce::jmax (pkI, inPk.load() * 0.9f); outPk = juce::jmax (pkO * og, outPk.load() * 0.9f);
        const float red = pkI > 1e-5f && pkO > 1e-5f ? juce::jmax (0.0f, juce::Decibels::gainToDecibels (pkI / pkO)) : 0.0f;
        clipDb = juce::jmax (red, clipDb.load() * 0.92f);
    }
    std::atomic<float> inPk { 0 }, outPk { 0 }, clipDb { 0 };
    static inline bool forceWet = false;   // solo per i test
private:
    static constexpr int kFd = 32, kFdHalf = 15, kLa = 32;
    static constexpr float kSafe = 0.989f;   // -0.1 dBFS
    // fin dove la curva lascia passare il segnale intatto (0 = mai)
    static float identityKnee (int m) { switch (m) { case HARD: case FOLD: return 1.0f; case TIGHT: return 0.85f; case BUS: return 0.65f; case SOFT: return 0.5f; default: return 0.0f; } }
    double measureLatency (int k)
    {
        juce::dsp::Oversampling<float> o (2, (size_t) k, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, true);
        o.initProcessing (1024); o.reset();
        juce::AudioBuffer<float> b (2, 1024); b.clear(); b.setSample (0, 0, 1.0f); b.setSample (1, 0, 1.0f);
        juce::dsp::AudioBlock<float> blk (b); o.processSamplesUp (blk); o.processSamplesDown (blk);
        const float* d = b.getReadPointer (0); int pk = 0; for (int i = 1; i < 1024; ++i) if (std::abs (d[i]) > std::abs (d[pk])) pk = i;
        double num = 0, den = 0; for (int i = juce::jmax (0, pk - 24); i <= juce::jmin (1023, pk + 24); ++i) { num += i * (double) d[i]; den += d[i]; }   // baricentro del picco
        return den != 0 ? num / den : pk;
    }
    inline float sample (int m, float x, int c, bool hiBand)
    {
        if (m == CRISP) { auto& pre = hiBand ? crPreH[c] : crPre[c]; auto& post = hiBand ? crPostH[c] : crPost[c]; return post.p (shapeMode (CRISP, pre.p (x))); }
        return shapeMode (m, x);
    }
    double sr = 44100; int maxN = 512, lastOs = -1, intLat[4] {};
    std::unique_ptr<juce::dsp::Oversampling<float>> ovs[4];
    juce::AudioBuffer<float> dry, lo, hi, full, spl;
    std::vector<float> gArr, dArr, cArr, oArr, mxArr, mdArr, spArr, dlArr, gmArr;
    jkit::LR4 xo[2], xoD[2]; jkit::Bq crPre[2], crPost[2], crPreH[2], crPostH[2];
    float envF[2] {}, envS[2] {}, dcX[2] {}, dcY[2] {};
    float fd[4][kFd] {}; std::array<float, kFd> fdr[2] {}; int fdw = 0;
    std::vector<float> dline[2]; int dw = 0;
    std::array<float, kLa> la[2] {}, greq {}, hold {}; double holdSum = kLa; int law = 0; float lg = 1.0f, relK = 0.001f; int holdN = 13230;
    int colN = 512, colC = 0; float colIn = 0, colOut = 0;
    bool first = true; float dS = 0, cS = 0, oS = 0, mxS = 1, pS = 0, hzS = 120, modeX = 1, spX = 0, dlX = 0, gm = 0; int modeA = HARD, modeB = HARD;
    juce::int64 T = 0, clipFrom = -1, clipUntil = -1;
};
}
