#pragma once
#include <juce_dsp/juce_dsp.h>
#include "JackpotKit.h"

namespace jphs
{
enum Mode { M_OFF, M_INV, M_ROT, M_ADAPT, kModes };

// coppia di filtri passa-tutto con 90 gradi di differenza (trasformata di Hilbert IIR, progetto ellittico alla hiir)
inline std::vector<double> hilbertCoefs (int n, double tbw)
{
    double k = std::tan ((1 - tbw * 2) * juce::MathConstants<double>::pi / 4); k *= k;
    const double kk = std::pow (1 - k * k, 0.25), e = 0.5 * (1 - kk) / (1 + kk), e2 = e * e, e4 = e2 * e2;
    const double q = e * (1 + e4 * (2 + e4 * (15 + 150 * e4)));
    const int order = n * 2 + 1; std::vector<double> out;
    for (int idx = 0; idx < n; ++idx)
    {
        const int c = idx + 1; double acc = 0; int i = 0, j = 1;
        for (;;) { const double t = std::pow (q, i * (i + 1)) * std::sin ((i * 2 + 1) * c * juce::MathConstants<double>::pi / order) * j; acc += t; j = -j; ++i; if (std::abs (t) < 1e-100 || i > 60) break; }
        const double num = acc * std::pow (q, 0.25);
        acc = 0; i = 1; j = -1;
        for (;;) { const double t = std::pow (q, i * i) * std::cos (i * 2 * c * juce::MathConstants<double>::pi / order) * j; acc += t; j = -j; ++i; if (std::abs (t) < 1e-100 || i > 60) break; }
        const double den = acc + 0.5, ww = num / den, w2 = ww * ww;
        const double x = std::sqrt ((1 - w2 * k) * (1 - w2 / k)) / (1 + w2);
        out.push_back ((1 - x) / (1 + x));
    }
    return out;
}
struct ApChain
{
    static constexpr int N = 6;
    double c[N] {}, x1[N] {}, x2[N] {}, y1[N] {}, y2[N] {}, d = 0; bool delay = false;
    void reset() { for (int i = 0; i < N; ++i) x1[i] = x2[i] = y1[i] = y2[i] = 0; d = 0; }
    inline double p (double x)
    {
        for (int i = 0; i < N; ++i) { const double y = c[i] * (x + y2[i]) - x2[i]; x2[i] = x1[i]; x1[i] = x; y2[i] = y1[i]; y1[i] = y; x = y; }
        if (delay) { const double o = d; d = x; return o; }
        return x;
    }
};
struct Hilbert
{
    ApChain a, b;   // a: in ritardo di 90 gradi rispetto a b
    void setup (double sr)
    {
        const auto cs = hilbertCoefs (12, juce::jlimit (1e-5, 0.01, 9.0 / sr));
        for (int i = 0; i < 6; ++i) { a.c[i] = cs[(size_t) (i * 2 + 1)]; b.c[i] = cs[(size_t) (i * 2)]; }
        a.delay = true; b.delay = false; a.reset(); b.reset();
    }
};

struct Settings { int mode = M_OFF; float freq = 160, angle = 0, speedMs = 40, amount = 1, outDb = 0; bool listen = false; };

// istantanea dell'ultimo colpo di cassa per il display
struct Snapshot
{
    static constexpr int kLen = 1024;   // campioni decimati (x4) = ~90 ms a 44.1k
    float kick[kLen] {}, before[kLen] {}, after[kLen] {}; std::atomic<int> serial { 0 }; float theta = 0, corrBefore = 0, corrAfter = 0;
};

class Engine
{
public:
    void prepare (double s)
    {
        sr = s; L = juce::jmax (64, (int) (0.03 * sr));   // 30 ms di anticipo
        for (int c = 0; c < 2; ++c) { hb[c].setup (sr); hh[c].setup (sr); xo[c].reset(); dl[c].assign ((size_t) (L + 8), 0.0f); }
        hk.setup (sr); xk.reset(); kd.assign ((size_t) (L + 8), 0.0f);
        prodA.assign ((size_t) L, 0.0); prodB.assign ((size_t) L, 0.0); ek.assign ((size_t) L, 0.0); dA.assign ((size_t) L, 0.0); dB.assign ((size_t) L, 0.0); w = 0; sA = sB = sK = sDA = sDB = 0; n0C = 1; n0S = 0; rel = 0;
        thC = 1; thS = 0; theta = 0; lastF = -1; kEnv = 0; kSlow = 0; armed = true; snapPos = -1;
        for (auto& r : rings) r.assign (Snapshot::kLen * 4 + 8, 0.0f);
        corrN = 0; cKB = cKK = cBB = cKA = cAA = 0;
    }
    int latency() const { return L; }

    void process (float* Lc, float* Rc, const float* sL, const float* sR, int n, const Settings& S, Snapshot& snap)
    {
        if (S.freq != lastF) { lastF = S.freq; for (int c = 0; c < 2; ++c) xo[c].set (sr, S.freq); xk.set (sr, S.freq); }
        const float outG = juce::Decibels::decibelsToGain (S.outDb);
        const double spd = 1.0 - std::exp (-1.0 / (juce::jmax (1.0f, S.speedMs) * 0.001 * sr));
        float* ch[2] = { Lc, Rc };
        const bool hasK = sL != nullptr;
        for (int i = 0; i < n; ++i)
        {
            // cassa (sidechain): banda bassa, poi ramo "a" del Hilbert per avere la stessa fase del basso
            const float kin = hasK ? 0.5f * (sL[i] + (sR ? sR[i] : sL[i])) : 0.0f;
            float klo, khi; xk.split (kin, klo, khi);
            const double kA = kin;   // la cassa resta com'e: e lei il riferimento
            // basso: banda bassa e alta, entrambe nei rami del Hilbert
            double bA[2], bB[2], hA[2], bLo = 0;
            for (int c = 0; c < 2; ++c)
            {
                float lo, hi; xo[c].split (ch[c][i], lo, hi);
                bA[c] = hb[c].a.p (lo); bB[c] = hb[c].b.p (lo); hA[c] = hh[c].a.p (hi); bLo += 0.5 * ch[c][i];
            }
            const double mA = 0.5 * (bA[0] + bA[1]), mB = 0.5 * (bB[0] + bB[1]);
            // somme mobili su L campioni (il futuro rispetto all'uscita ritardata)
            sA += kA * mA - prodA[(size_t) w]; prodA[(size_t) w] = kA * mA;
            sB += kA * mB - prodB[(size_t) w]; prodB[(size_t) w] = kA * mB;
            sK += kA * kA - ek[(size_t) w]; ek[(size_t) w] = kA * kA;
            // angolo "neutro": quello che rende il basso ruotato uguale all'originale
            sDA += bLo * mA - dA[(size_t) w]; dA[(size_t) w] = bLo * mA;
            sDB += bLo * mB - dB[(size_t) w]; dB[(size_t) w] = bLo * mB;
            { const double mg = std::sqrt (sDA * sDA + sDB * sDB); if (mg > 1e-9) { n0C = sDA / mg; n0S = sDB / mg; } }
            if (++w >= L) w = 0;
            // angolo voluto
            const double a0 = std::atan2 (n0S, n0C);
            double ang = a0;
            if (S.mode == M_ROT) ang = a0 + juce::degreesToRadians ((double) S.angle);
            if (S.mode == M_ADAPT)
            {
                const double mag = std::sqrt (sA * sA + sB * sB);
                // si muove solo quando la cassa suona: tra un colpo e l'altro tiene l'ultimo scarto
                if (hasK && sK > kGate * L && mag > 1e-12) { double d = std::atan2 (sB, sA) - a0; while (d > juce::MathConstants<double>::pi) d -= juce::MathConstants<double>::twoPi; while (d < -juce::MathConstants<double>::pi) d += juce::MathConstants<double>::twoPi; rel = d; }
                ang = a0 + rel * S.amount;
            }
            double tc = std::cos (ang), ts = std::sin (ang);
            if (S.mode == M_ADAPT || S.mode == M_ROT)
            {
                thC += (tc - thC) * spd; thS += (ts - thS) * spd;
                const double m = std::sqrt (thC * thC + thS * thS) + 1e-12; thC /= m; thS /= m;
            }
            else { thC = 1; thS = 0; }
            { double d = std::atan2 (thS, thC) - a0; while (d > juce::MathConstants<double>::pi) d -= juce::MathConstants<double>::twoPi; while (d < -juce::MathConstants<double>::pi) d += juce::MathConstants<double>::twoPi;
              theta = (float) juce::radiansToDegrees (d); if (S.mode == M_OFF) theta = 0; }
            // uscita: banda bassa ruotata + banda alta (stesso ramo a) oppure originale ritardato
            float kOut = 0;
            { kd[(size_t) kw] = kin; int rp = kw - L; while (rp < 0) rp += (int) kd.size(); kOut = kd[(size_t) rp]; }
            float rotM = 0, befM = 0;
            for (int c = 0; c < 2; ++c)
            {
                auto& d = dl[c]; d[(size_t) kw] = ch[c][i]; int rp = kw - L; while (rp < 0) rp += (int) d.size();
                const float dry = d[(size_t) rp];
                float y = dry;
                if (S.mode == M_OFF) y = dry;
                else if (S.mode == M_INV) y = -dry;
                // rami del Hilbert ritardati: l'angolo trovato guardando avanti si applica al campione giusto
                auto& ra = rA[c]; auto& rb = rB[c]; auto& rh = rH[c];
                if (ra.size() != d.size()) { ra.assign (d.size(), 0.0); rb.assign (d.size(), 0.0); rh.assign (d.size(), 0.0); }
                ra[(size_t) kw] = bA[c]; rb[(size_t) kw] = bB[c]; rh[(size_t) kw] = hA[c];
                if (S.mode == M_ROT || S.mode == M_ADAPT) y = (float) (ra[(size_t) rp] * thC + rb[(size_t) rp] * thS + rh[(size_t) rp]);
                ch[c][i] = (y + (S.listen ? kOut : 0.0f)) * outG;
                rotM += 0.5f * y;
                befM += 0.5f * dry;
            }
            if (++kw >= (int) kd.size()) kw = 0;
            // misura su quello che si sente davvero (segnali ritardati): cassa, basso prima, basso dopo
            {
                const float kLo = kOut, bBef = befM, bAft = rotM;
                const float a = std::abs (kOut);
                kEnv = juce::jmax (a, kEnv * 0.9995f); kSlow = kSlow * 0.99995f + a * 0.00005f;
                if (kEnv > 0.02f)
                {   // solo mentre suona la cassa
                    const float dec = 0.9997f;
                    cKB = cKB * dec + kLo * bBef; cKK = cKK * dec + kLo * kLo; cBB = cBB * dec + bBef * bBef; cKA = cKA * dec + kLo * bAft; cAA = cAA * dec + bAft * bAft;
                    if (cKK > 1e-7 && cBB > 1e-7 && cAA > 1e-7) { corrBefore = (float) (cKB / std::sqrt (cKK * cBB)); corrAfter = (float) (cKA / std::sqrt (cKK * cAA)); }
                }
                if (armed && hasK && a > 0.05f && a > kSlow * 4.0f) { armed = false; snapPos = 0; }
                if (! armed && kEnv < 0.01f) armed = true;
                if (snapPos >= 0)
                {
                    rings[0][(size_t) snapPos] = kLo; rings[1][(size_t) snapPos] = bBef; rings[2][(size_t) snapPos] = bAft;
                    if (++snapPos >= Snapshot::kLen * 4)
                    {
                        float pk = 1e-6f; for (int k = 0; k < Snapshot::kLen * 4; ++k) pk = juce::jmax (pk, std::abs (rings[0][(size_t) k]), std::abs (rings[1][(size_t) k]));
                        for (int k = 0; k < Snapshot::kLen; ++k) { snap.kick[k] = rings[0][(size_t) k * 4] / pk; snap.before[k] = rings[1][(size_t) k * 4] / pk; snap.after[k] = rings[2][(size_t) k * 4] / pk; }
                        snap.theta = theta; snap.corrBefore = corrBefore; snap.corrAfter = corrAfter; snap.serial = snap.serial.load() + 1; snapPos = -1;
                    }
                }
            }
        }
        thetaUi = theta; cbUi = corrBefore; caUi = corrAfter; scUi = hasK;
    }
    std::atomic<float> thetaUi { 0 }, cbUi { 0 }, caUi { 0 }; std::atomic<bool> scUi { false };
private:
    double sr = 44100; int L = 1323, w = 0, kw = 0;
    Hilbert hb[2], hh[2], hk; jkit::LR4 xo[2], xk; float lastF = -1;
    std::vector<float> dl[2], kd; std::vector<double> rA[2], rB[2], rH[2];
    std::vector<double> prodA, prodB, ek, dA, dB; double sDA = 0, sDB = 0, n0C = 1, n0S = 0, rel = 0; double sA = 0, sB = 0, sK = 0, tgtC = 1, tgtS = 0, thC = 1, thS = 0; float theta = 0;
    static constexpr double kGate = 1e-5;   // energia media minima della cassa per decidere
    float kEnv = 0, kSlow = 0; bool armed = true; int snapPos = -1; float snapAfterTheta = 0;
    std::vector<float> rings[3];
    int corrN = 0; double cKB = 0, cKK = 0, cBB = 0, cKA = 0, cAA = 0; float corrBefore = 0, corrAfter = 0;
};
}
