#pragma once
#include <juce_dsp/juce_dsp.h>
#include "JackpotKit.h"
#include "ButterAnalog.h"   // EQ analogici e Pultec, condivisi con il Butter Rack

namespace jpre
{
// ======================================================================= preamp
enum PreModel { P_73, P_81, P_V76, P_610, P_API, P_SSL, P_TG2, P_737, kPreModels };
struct PreInfo
{
    const char* name; const char* desc;
    float lfSat;      // il trasformatore d'ingresso satura le basse
    float bias;       // asimmetria = armoniche pari (seconda)
    float knee;       // durezza dello stadio = armoniche dispari
    bool tube;        // valvola: compressione morbida e ginocchio largo
    float headroom;   // dB prima che inizi a saturare
    float hpHz, lpHz; // banda del trasformatore
    float lowHz, lowDb, midHz, midDb, midQ, airHz, airDb;   // impronta in frequenza
};
inline const PreInfo& preInfo (int m)
{
    static const PreInfo I[kPreModels] = {
        { "73", "Neve 1073: classe A e trasformatori Marinair. Bassi spessi, medi in faccia, seconda e terza armonica. Il suono dei dischi.",
          1.0f, 0.16f, 1.0f, false, 18, 16, 24000, 70, 0.7f, 1600, 0.4f, 0.8f, 18000, -0.4f },
        { "81", "Neve 1081 (Lindell 81): stessa famiglia del 73 con piu margine. Rotondo ma piu pulito e aperto.",
          0.65f, 0.11f, 0.95f, false, 22, 14, 30000, 60, 0.4f, 2500, 0.2f, 0.7f, 20000, 0.0f },
        { "V76", "Telefunken V76 a valvole: tanta seconda armonica, medio-bassi pieni e alti morbidi. Caldo da broadcast tedesco.",
          0.7f, 0.42f, 0.7f, true, 14, 25, 17000, 220, 0.9f, 3500, -0.3f, 0.6f, 12000, -1.2f },
        { "610", "UA 610 a valvole: grasso nei bassi e nei medi bassi, si comprime dolcemente quando lo spingi. Voci e bassi.",
          0.85f, 0.34f, 0.75f, true, 15, 22, 19000, 140, 1.0f, 800, 0.3f, 0.7f, 14000, -0.6f },
        { "API 312", "API 312 con l'op-amp 2520: punch e medi davanti, terza armonica, basso stretto. Batterie e chitarre.",
          0.5f, 0.05f, 1.45f, false, 20, 28, 26000, 100, 0.3f, 2800, 0.9f, 0.9f, 15000, 0.2f },
        { "SSL E", "Pre della console SSL E: pulito e brillante, non colora finche non lo spingi forte, poi morde.",
          0.18f, 0.03f, 1.7f, false, 24, 18, 30000, 80, 0.0f, 4000, 0.3f, 0.8f, 12000, 0.6f },
        { "TG2", "EMI TG2 di Abbey Road: trasformatori pesanti, si rompe presto in una saturazione grossa e granulosa.",
          1.35f, 0.2f, 1.25f, false, 12, 20, 18000, 90, 0.6f, 1200, 0.7f, 0.7f, 16000, -0.6f },
        { "737", "Avalon 737 a valvole in classe A: pulito, arioso, alti setosi. Voci moderne.",
          0.3f, 0.24f, 0.8f, true, 22, 12, 32000, 120, 0.2f, 3000, 0.0f, 0.7f, 14000, 0.9f } };
    return I[juce::jlimit (0, kPreModels - 1, m)];
}

// ======================================================================= EQ di colore
enum EqModel { E_EQP1A, E_MEQ5, E_MAAG, E_SSL, E_NEVE84, E_LIND81, E_73, E_PE1C, kEqModels };
inline const char* eqName (int m) { static const char* n[] = { "EQP-1A", "MEQ-5", "MAAG EQ4", "SSL E", "NEVE 1084", "LINDELL 81", "73", "PE 1C" }; return n[juce::jlimit (0, kEqModels - 1, m)]; }
inline const char* eqDesc (int m)
{
    static const char* d[] = {
        "Pultec EQP-1A a valvole: boost e attenuazione dei bassi insieme (il trucco), alti dolci. Colore della valvola di uscita.",
        "Pultec MEQ-5 a valvole: tre bande sui medi. Picco basso, DIP per pulire, picco alto per la presenza.",
        "Maag EQ4: pulitissimo, AIR BAND fino a 40 kHz. Brillantezza senza durezza.",
        "SSL E: chirurgico, Q che si stringe con il guadagno, colore mordente.",
        "Neve 1084: classe A, bassi rotondi, medi larghi e musicali.",
        "Lindell 81 (1081): quattro bande, Neve ma piu pulito.",
        "Neve 1073: il piu colorato, trasformatore che spinge in faccia.",
        "Tube-Tech PE 1C stile Pultec: boost e cut sui bassi, alti morbidissimi." };
    return d[juce::jlimit (0, kEqModels - 1, m)];
}
inline int aeOf (int m) { return m - E_MAAG; }   // modelli condivisi con il Butter
inline const float* meqLpF() { static const float f[5] = { 200, 300, 500, 700, 1000 }; return f; }
inline const float* meqDipF() { static const float f[11] = { 200, 300, 500, 700, 1000, 1500, 2000, 3000, 4000, 5000, 7000 }; return f; }
inline const float* meqHpF() { static const float f[5] = { 1500, 2000, 3000, 4000, 5000 }; return f; }

struct Settings
{
    // sottrattivo
    bool sOn = false; float hpHz = 20, lpHz = 20000; int hpSlope = 1, lpSlope = 1; float bF[4] { 100, 400, 2500, 8000 }, bG[4] {}, bQ[4] { 4, 4, 4, 4 };
    // preamp
    bool pOn = false; int pModel = P_73; float pGain = 0, pOut = 0; bool pPhase = false, pAuto = true;
    // EQ di colore
    bool eOn = false; int eModel = E_EQP1A;
    jb::PultecSettings pu; float tube = 0.3f;
    int mLf = 1, mDip = 4, mHf = 2; float mLg = 0, mDg = 0, mHg = 0;
    jb::AeSettings ae;
    float outDb = 0;
};

// ----------------------------------------------------------------------- stadio preamp
struct Preamp
{
    jkit::Bq hp[2], lp[2], low[2], mid[2], air[2]; float lfLp[2] {}, dc[2] {}, env[2] {}, outLp[2] {};
    int lastModel = -1; double sr = 44100;
    void prepare (double s) { sr = s; lastModel = -1; for (int c = 0; c < 2; ++c) { hp[c].reset(); lp[c].reset(); low[c].reset(); mid[c].reset(); air[c].reset(); lfLp[c] = dc[c] = env[c] = outLp[c] = 0; } }
    void setModel (int m)
    {
        if (m == lastModel) return; lastModel = m; const auto& I = preInfo (m);
        for (int c = 0; c < 2; ++c)
        {
            hp[c].hp (sr, I.hpHz, 0.6); lp[c].lp (sr, juce::jmin ((double) I.lpHz, sr * 0.45), 0.6);
            low[c].lowShelf (sr, I.lowHz, I.lowDb, 0.7); mid[c].peak (sr, I.midHz, I.midQ, I.midDb); air[c].highShelf (sr, I.airHz, I.airDb, 0.7);
        }
    }
    // x gia sovracampionato e moltiplicato per il gain del pre
    inline float run (float x, int c, const PreInfo& I, float sre)
    {
        const float H = juce::Decibels::decibelsToGain (I.headroom) * 0.5f;   // livello a cui lo stadio inizia a piegare
        // trasformatore d'ingresso: le basse saturano prima (nucleo che si satura)
        const float aLf = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 110.0f / sre);
        lfLp[c] += (x - lfLp[c]) * aLf; const float lo = lfLp[c], Hl = H * 0.35f;
        float y = x + (Hl * std::tanh (lo / Hl) - lo) * juce::jmin (1.0f, I.lfSat * 0.8f);
        // valvola: comprime dolcemente seguendo l'inviluppo (sag dell'alimentazione)
        if (I.tube)
        {
            const float a = std::abs (y) / H; env[c] += (a - env[c]) * (a > env[c] ? 0.0015f : 0.00015f);
            y /= 1.0f + 0.35f * env[c] * env[c];
        }
        // stadio di guadagno: asimmetria (seconda armonica) e ginocchio (terza)
        const float u = y / H, k = I.knee, b = I.bias;
        const float nb = std::tanh (b);
        y = H * (std::tanh (k * u + b) - nb) / (k * (1.0f - nb * nb));
        // trasformatore d'uscita
        const float v = y / H; y -= 0.04f * I.lfSat * v * v * v * H;
        return y;
    }
    inline float tone (float y, int c) { y = hp[c].p (y); y = lp[c].p (y); y = low[c].p (y); y = mid[c].p (y); return air[c].p (y); }
    inline float dcBlock (float y, int c) { dc[c] += (y - dc[c]) * 0.0003f; return y - dc[c]; }
};

class Engine
{
public:
    void prepare (double s, int maxBlock)
    {
        sr = s;
        os = std::make_unique<juce::dsp::Oversampling<float>> (2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false);
        os->initProcessing ((size_t) maxBlock);
        pre.prepare (sr); pre.setModel (P_73);
        for (auto& b : sub) for (auto& f : b) f.reset();
        for (auto& b : hpF) for (auto& f : b) f.reset();
        for (auto& b : lpF) for (auto& f : b) f.reset();
        lastSub = {}; subInit = false; eqInit = false;
        for (auto& f : ae) { f.z1[0] = f.z1[1] = f.z2[0] = f.z2[1] = 0; }
        gSm = 1;
    }
    int latency() const { return os ? (int) std::round (os->getLatencyInSamples()) : 0; }

    // filtri del sottrattivo (anche per la curva)
    static void subFilters (const Settings& S, double sr, jkit::Bq (&b)[4], jkit::Bq (&h)[4], jkit::Bq (&l)[4])
    {
        for (int k = 0; k < 4; ++k) { if (S.bG[k] < -0.05f) b[k].peak (sr, S.bF[k], S.bQ[k], S.bG[k]); else b[k].flat(); }
        // pendenze: 0 = 12, 1 = 24, 2 = 48 dB/ott (Butterworth a cascata)
        static const double q2[1] = { 0.7071 }, q4[2] = { 0.5412, 1.3066 }, q8[4] = { 0.5098, 0.6013, 0.9000, 2.5629 };
        auto make = [&] (jkit::Bq (&f)[4], bool hi, double fc, int slope)
        {
            const double* q = slope == 0 ? q2 : slope == 1 ? q4 : q8; const int n = slope == 0 ? 1 : slope == 1 ? 2 : 4;
            for (int k = 0; k < 4; ++k) { if (k < n) { if (hi) f[k].hp (sr, fc, q[k]); else f[k].lp (sr, fc, q[k]); } else f[k].flat(); }
        };
        make (h, true, S.hpHz, S.hpSlope); make (l, false, S.lpHz, S.lpSlope);
    }
    // EQ di colore: filtri per il modello
    static void eqFilters (const Settings& S, double sr, jb::Bq* f, bool* act, int& n)
    {
        n = 4;
        if (S.eModel == E_EQP1A) { jb::pultecFilters (S.pu, sr, f); for (int k = 0; k < 4; ++k) act[k] = true; return; }
        if (S.eModel == E_MEQ5)
        {
            f[0].set (jb::Bq::PEAK, sr, meqLpF()[juce::jlimit (0, 4, S.mLf)], S.mLg / 10.0f * 10.0f, 0.75); act[0] = S.mLg > 0.01f;
            f[1].set (jb::Bq::PEAK, sr, meqDipF()[juce::jlimit (0, 10, S.mDip)], -S.mDg / 10.0f * 10.0f, 0.9); act[1] = S.mDg > 0.01f;
            f[2].set (jb::Bq::PEAK, sr, meqHpF()[juce::jlimit (0, 4, S.mHf)], S.mHg / 10.0f * 10.0f, 1.1); act[2] = S.mHg > 0.01f;
            f[3].set (jb::Bq::PEAK, sr, 1000, 0, 1); act[3] = false; n = 3; return;
        }
        auto a = S.ae; a.model = aeOf (S.eModel);
        for (int k = 0; k < 4; ++k) jb::aeBandFilter (a, k, sr, f[k], act[k]);
    }

    void process (float* L, float* R, int n, const Settings& S)
    {
        float* ch[2] = { L, R };
        for (int i = 0; i < n; ++i) { inPk = juce::jmax (inPk.load() * 0.99995f, juce::jmax (std::abs (L[i]), std::abs (R[i]))); }
        // 1) sottrattivo (pulito, senza colore)
        if (S.sOn)
        {
            const auto key = std::make_tuple (S.hpHz, S.lpHz, S.hpSlope, S.lpSlope, S.bF[0], S.bF[1], S.bF[2], S.bF[3], S.bG[0], S.bG[1], S.bG[2], S.bG[3], S.bQ[0] + S.bQ[1] * 10 + S.bQ[2] * 100 + S.bQ[3] * 1000);
            if (! subInit || key != lastSub)
            {
                jkit::Bq b[4], h[4], l[4]; subFilters (S, sr, b, h, l);
                for (int c = 0; c < 2; ++c) for (int k = 0; k < 4; ++k) { copyCoef (sub[c][k], b[k]); copyCoef (hpF[c][k], h[k]); copyCoef (lpF[c][k], l[k]); }
                lastSub = key; subInit = true;
            }
            const bool hpOn = S.hpHz > 20.5f, lpOn = S.lpHz < 19900;
            for (int c = 0; c < 2; ++c) for (int i = 0; i < n; ++i)
            {
                float x = ch[c][i];
                if (hpOn) for (int k = 0; k < 4; ++k) x = hpF[c][k].p (x);
                if (lpOn) for (int k = 0; k < 4; ++k) x = lpF[c][k].p (x);
                for (int k = 0; k < 4; ++k) if (S.bG[k] < -0.05f) x = sub[c][k].p (x);
                ch[c][i] = x;
            }
        }
        // 2) preamp (sovracampionato 4x)
        if (S.pOn)
        {
            pre.setModel (S.pModel); const auto& I = preInfo (S.pModel);
            const float g = juce::Decibels::decibelsToGain (S.pGain);
            juce::dsp::AudioBlock<float> blk (ch, 2, (size_t) n);
            auto up = os->processSamplesUp (blk);
            const float sre = (float) sr * 4;
            for (int c = 0; c < 2; ++c)
            {
                float* d = up.getChannelPointer ((size_t) c); const int N = (int) up.getNumSamples();
                for (int i = 0; i < N; ++i) d[i] = pre.run (d[i] * g, c, I, sre);
            }
            os->processSamplesDown (blk);
            // compensazione: il guadagno del pre si sente come colore, non come volume (se AUTO)
            const float comp = S.pAuto ? 1.0f / g : 1.0f;
            const float og = juce::Decibels::decibelsToGain (S.pOut) * comp * (S.pPhase ? -1.0f : 1.0f);
            for (int c = 0; c < 2; ++c) for (int i = 0; i < n; ++i) ch[c][i] = pre.dcBlock (pre.tone (ch[c][i], c), c) * og;
        }
        else if (S.pPhase) for (int c = 0; c < 2; ++c) for (int i = 0; i < n; ++i) ch[c][i] = -ch[c][i];
        // 3) EQ di colore
        if (S.eOn)
        {
            const bool tubeEq = S.eModel == E_EQP1A || S.eModel == E_MEQ5;
            const float sig[] = { (float) S.eModel, S.pu.lb, S.pu.la, S.pu.hb, S.pu.bw, S.pu.ha, (float) S.pu.lf, (float) S.pu.hf, (float) S.pu.as, (float) S.mLf, (float) S.mDip, (float) S.mHf, S.mLg, S.mDg, S.mHg,
                                  S.ae.g[0], S.ae.g[1], S.ae.g[2], S.ae.g[3], S.ae.f[0], S.ae.f[1], S.ae.f[2], S.ae.f[3], S.ae.q };
            if (! eqInit || std::memcmp (sig, lastEq, sizeof (sig)) != 0)
            {
                int nb = 4; eqFilters (S, sr, coefs, eqAct, nb); eqN = nb;
                for (int k = 0; k < 4; ++k) { auto keep = ae[k]; ae[k] = coefs[k]; std::memcpy (ae[k].z1, keep.z1, sizeof (keep.z1)); std::memcpy (ae[k].z2, keep.z2, sizeof (keep.z2)); }
                std::memcpy (lastEq, sig, sizeof (sig)); eqInit = true;
            }
            const int colModel = tubeEq ? jb::AE_PE1C : aeOf (S.eModel);
            const float drive = tubeEq ? S.tube : S.ae.drive;
            for (int c = 0; c < 2; ++c) for (int i = 0; i < n; ++i)
            {
                float x = ch[c][i];
                for (int k = 0; k < eqN; ++k) if (eqAct[k]) x = ae[k].run (x, c);
                x = colour.run (x, c, colModel, drive, sr);
                ch[c][i] = x;
            }
        }
        const float og = juce::Decibels::decibelsToGain (S.outDb);
        for (int i = 0; i < n; ++i) { L[i] *= og; R[i] *= og; outPk = juce::jmax (outPk.load() * 0.99995f, juce::jmax (std::abs (L[i]), std::abs (R[i]))); }
    }
    std::atomic<float> inPk { 0 }, outPk { 0 };
private:
    static void copyCoef (jkit::Bq& d, const jkit::Bq& s) { d.b0 = s.b0; d.b1 = s.b1; d.b2 = s.b2; d.a1 = s.a1; d.a2 = s.a2; }
    double sr = 44100;
    std::unique_ptr<juce::dsp::Oversampling<float>> os;
    Preamp pre;
    jkit::Bq sub[2][4], hpF[2][4], lpF[2][4];
    std::tuple<float, float, int, int, float, float, float, float, float, float, float, float, float> lastSub {}; bool subInit = false;
    jb::Bq coefs[4], ae[4]; bool eqAct[4] {}; int eqN = 4; float lastEq[24] {}; bool eqInit = false;
    jb::AeColour colour; float gSm = 1;
public:
    Preamp& preamp() { return pre; }
};
}
