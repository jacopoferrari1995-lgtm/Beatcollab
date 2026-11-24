#pragma once
// JACKPOT TUBE BOX 2: poche manopole grandi e sei caratteri di valvola, ognuno con la sua personalita'.
// DRIVE spinge il carattere scelto (con volume compensato), CALORE aggiunge armoniche sui bassi (corpo, anche sul telefono),
// ARIA apre la parte alta con dolcezza, ENHANCER fa nascere armoniche nella zona della presenza (la voce passa davanti).
// Tutto a 4x con antiderivate (niente aliasing), latenza vera dichiarata. Tutto a zero = solo un ritardo (neutro al bit).
#include <juce_dsp/juce_dsp.h>
#include "JackpotKit.h"

namespace jtbx
{
// ----------------------------------------------------------------------- caratteri
enum Curve { TRIODO, PENTODO, TRAFO, DIODO };
struct CharK
{
    const char* name; const char* tube; const char* desc;
    int curve; double bias;              // forma della saturazione e asimmetria (armoniche pari)
    float preLo, loHz, preHi, hiHz;      // pre-enfasi: dove satura di piu' (tolta dopo, resta solo il colore)
    float driveDb, sag;                  // quanto spinge DRIVE al massimo, quanto la valvola "si siede" (compressione)
    int stages; float postLp;            // stadi in serie, taglio finale
    float enhHz; float h2, h3, h4, h5;   // ENHANCER: dove nasce e che armoniche
    float tilt;                          // colore finale (dB a 8 kHz rispetto a 300 Hz)
};
constexpr int kChars = 6;
inline const CharK& charOf (int i)
{
    static const CharK K[kChars] = {
        { "CALDO",  "12AX7", "triodo: seconda armonica, morbido, voce e strumenti piu' pieni",           TRIODO,  0.22, 1.0f, 180, -2.0f, 3000, 18, 0.35f, 1, 18000, 2600, 0.75f, 0.25f, 0.12f, 0.0f,  -0.5f },
        { "SPINTO", "EL84",  "pentodo: morde sui medi, fa uscire la voce rap e le chitarre",             PENTODO, 0.07, -2.0f, 160, 4.0f, 1800, 24, 0.20f, 1, 16000, 3400, 0.25f, 0.65f, 0.05f, 0.15f, 0.8f },
        { "CREMA",  "TRAFO", "trasformatore + valvola leggera: colla, bassi spessi, alti tondi",         TRAFO,   0.10, 4.0f, 140, -3.0f, 5000, 15, 0.50f, 1, 14000, 2000, 0.8f, 0.2f, 0.1f, 0.0f,   -1.2f },
        { "VETRO",  "ECC81", "pulito che brilla: armoniche in alto, ideale su voce e master",            TRIODO,  0.15, -3.0f, 200, 6.0f, 4500, 12, 0.10f, 1, 20000, 6000, 0.6f, 0.3f, 0.2f, 0.0f,   0.6f },
        { "FUOCO",  "6L6",   "spinto fino alla distorsione: 808, batterie, synth, voci aggressive",       DIODO,   0.35, 0.0f, 200, 2.0f, 1200, 34, 0.60f, 1, 12000, 3000, 0.3f, 0.6f, 0.1f, 0.3f,   0.0f },
        { "SPESSO", "2x12AX7", "due triodi in serie e tanta compressione: denso, grande, incollato",     TRIODO,  0.20, 2.0f, 160, -1.0f, 3000, 20, 0.70f, 2, 16000, 1500, 0.8f, 0.3f, 0.15f, 0.0f,  -0.3f } };
    return K[juce::jlimit (0, kChars - 1, i)];
}

struct Settings
{
    float drive = 0, warmth = 0, air = 0, enh = 0, mix = 1, outDb = 0;   // manopole 0..10, mix 0..1
    int character = 0; bool autoGain = true;
};

// ----------------------------------------------------------------------- antiderivata del primo ordine (in double)
template <typename F, typename Fi>
struct Adaa
{
    double x1 = 0, F1 = 0; bool init = false;
    void reset() { init = false; }
    inline float run (float xf, const F& f, const Fi& Fint)
    {
        const double x = xf, Fx = Fint (x);
        if (! init) { init = true; x1 = x; F1 = Fx; return (float) f (x); }
        const double d = x - x1; double y;
        if (std::abs (d) < 1e-6) y = f (0.5 * (x + x1)); else y = (Fx - F1) / d;
        x1 = x; F1 = Fx; return (float) y;
    }
};
inline double lcosh (double x) { const double a = std::abs (x); return a + std::log1p (std::exp (-2.0 * a)) - 0.6931471805599453; }
// curva unica con parametri: triodo (tanh a due pendenze, la griglia conduce prima), pentodo (ginocchio netto),
// trasformatore (morbidissimo), diodo (spalla dura e asimmetrica). b = polarizzazione (armoniche pari).
struct Shape
{
    int c = 0; double b = 0;
    static double sgm (double u, double k) { return u / std::pow (1.0 + std::pow (std::abs (u), k), 1.0 / k); }
    double raw (double u) const
    {
        switch (c)
        {
            case PENTODO: return sgm (u, 2.5);
            case TRAFO:   return std::tanh (u * 0.8) / 0.8;
            case DIODO:   return u > 0 ? sgm (u * 1.3, 4.0) / 1.3 : std::tanh (u * 0.7) / 0.7;
            default:      return u > 0 ? std::tanh (u * 1.25) / 1.25 : std::tanh (u * 0.85) / 0.85;
        }
    }
    double operator() (double x) const { return raw (x + b) - raw (b); }
};
// antiderivata numerica tabellata (Simpson) della curva: precisa e veloce per l'ADAA
struct ShapeTab
{   // curva e sua antiderivata in tabella (passo fine, interpolazione lineare: l'integrale del tratto e' esatto)
    Shape s; std::vector<double> F, V; double lo = -24, hi = 24, step = 0.002, inv = 500;
    void build (const Shape& sh)
    {
        s = sh; const int n = (int) ((hi - lo) / step) + 2; F.assign ((size_t) n, 0.0); V.assign ((size_t) n, 0.0); inv = 1.0 / step;
        for (int i = 0; i < n; ++i) V[(size_t) i] = s (lo + i * step);
        for (int i = 1; i < n; ++i) F[(size_t) i] = F[(size_t) i - 1] + step * 0.5 * (V[(size_t) i - 1] + V[(size_t) i]);
        const double z = Fi (0.0); for (auto& v : F) v -= z;
    }
    inline double val (double x) const
    {
        if (x <= lo) return V.front(); if (x >= hi - step) return V[V.size() - 2];
        const double p = (x - lo) * inv; const int i = (int) p; const double fr = p - i; return V[(size_t) i] + fr * (V[(size_t) i + 1] - V[(size_t) i]);
    }
    inline double Fi (double x) const
    {
        if (x <= lo) return F.front() + (x - lo) * V.front();
        if (x >= hi - step) return F[F.size() - 2] + (x - (hi - step)) * V[V.size() - 2];
        const double p = (x - lo) * inv; const int i = (int) p; const double fr = p - i, va = V[(size_t) i], vx = va + fr * (V[(size_t) i + 1] - va);
        return F[(size_t) i] + fr * step * 0.5 * (va + vx);
    }
};
struct TabF  { const ShapeTab* t; double operator() (double x) const { return t->val (x); } };
struct TabFi { const ShapeTab* t; double operator() (double x) const { return t->Fi (x); } };
struct TanhF  { double operator() (double x) const { return std::tanh (x); } };
struct TanhFi { double operator() (double x) const { return lcosh (x); } };

inline float softU (float u) { u = juce::jlimit (-1.5f, 1.5f, u); return u - u * u * u / 6.75f; }   // pendenza 1, arriva a 1 piatta
class Engine
{
public:
    static constexpr int kOs = 4;
    void prepare (double s, int maxBlock)
    {
        sr = s; sro = s * kOs; mb = juce::jmax (64, maxBlock);
        os = std::make_unique<juce::dsp::Oversampling<float>> (2, 2, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, true);
        os->initProcessing ((size_t) mb);
        // passa-basso a 16 poli a 21 kHz dentro il 4x (quello che nasce sopra non ripiega); il suo ritardo si conta
        static const double Q8[8] = { 0.5024, 0.5225, 0.5669, 0.6468, 0.7882, 1.0607, 1.7224, 5.1011 }; double sq = 0; for (double q : Q8) sq += 1.0 / q;
        const double fc = juce::jmin (20000.0, s * 0.42);
        for (int c = 0; c < 2; ++c) for (int j = 0; j < 8; ++j) { aaLp[c][j].lp (sro, fc, Q8[j]); aaLp[c][j].reset(); }
        lat = juce::jmax (1, (int) std::round (os->getLatencyInSamples() + sq / (juce::MathConstants<double>::twoPi * fc) * s));
        for (int c = 0; c < 2; ++c) { ring[c].assign ((size_t) lat, 0.0f); dryT[c].assign ((size_t) mb, 0.0f); wetT[c].assign ((size_t) mb, 0.0f); }
        for (int k = 0; k < kChars; ++k) { Shape sh; sh.c = charOf (k).curve; sh.b = charOf (k).bias; tabs[k].build (sh); }
        rw = 0; running = false; warm = 0; gate = 0; curChar = -1;
        resetState(); smInit = false; agS = 1; rD = rW = 0; mixS = 1; cf = 1;
    }
    int latency() const { return lat; }

    void process (float* L, float* R, int n, const Settings& S)
    {
        const bool want = S.drive > 0.001f || S.warmth > 0.001f || S.air > 0.001f || S.enh > 0.001f;
        const bool act = want || anyS > 1e-4f;
        if (want && ! running) { running = true; warm = lat + 64; gate = 0; os->reset(); resetState(); }
        float* ch[2] = { L, R };
        for (int off = 0; off < n; off += mb)
        {
            const int m = juce::jmin (mb, n - off);
            float* io[2] = { L + off, R + off };
            if (running) { for (int c = 0; c < 2; ++c) std::copy (io[c], io[c] + m, dryT[c].begin()); runOs (io, m, S); }
            const float step = 1.0f / 1024.0f;
            const float rk = 1.0f - std::exp (-1.0f / (0.4f * (float) sr)), gk = 1.0f - std::exp (-1.0f / (0.25f * (float) sr)), mk = 1.0f - std::exp (-1.0f / (0.03f * (float) sr));
            for (int i = 0; i < m; ++i)
            {
                if (warm > 0) --warm; else if (running) gate = act ? juce::jmin (1.0f, gate + step) : juce::jmax (0.0f, gate - step);
                float d[2];
                for (int c = 0; c < 2; ++c)
                {
                    const float in = running ? dryT[c][(size_t) i] : io[c][i]; d[c] = ring[c][(size_t) rw]; ring[c][(size_t) rw] = in;
                }
                if (++rw >= lat) rw = 0;
                if (! running) { io[0][i] = d[0]; io[1][i] = d[1]; continue; }
                // AUTO: il trattato segue il volume del pulito (lento, niente pompaggio), poi MIX; tutto allineato col ritardo
                const float wl = io[0][i], wr = io[1][i];
                rD += rk * (0.5f * (d[0] * d[0] + d[1] * d[1]) - rD); rW += rk * (0.5f * (wl * wl + wr * wr) - rW);
                float tg = agS;
                if (rD > 1e-8f && rW > 1e-10f) tg = juce::jlimit (0.06f, 4.0f, std::sqrt (rD / rW));
                agS += gk * (tg - agS); mixS += mk * (S.mix - mixS);
                const float g = S.autoGain ? agS : 1.0f, mx = mixS * gate;
                io[0][i] = d[0] + (wl * g - d[0]) * mx; io[1][i] = d[1] + (wr * g - d[1]) * mx;
            }
            if (! want && gate <= 0.0f && warm == 0 && anyS <= 1e-4f) running = false;
        }
        const float og = juce::Decibels::decibelsToGain (S.outDb), ok = 1.0f - std::exp (-1.0f / (0.02f * (float) sr));
        if (outS < 0) outS = og;
        float pk = 0;
        for (int i = 0; i < n; ++i) { outS += (og - outS) * ok; if (outS != 1.0f) { ch[0][i] *= outS; ch[1][i] *= outS; } pk = juce::jmax (pk, std::abs (ch[0][i]), std::abs (ch[1][i])); }
        if (std::abs (outS - og) < 1e-6f) outS = og;
        outPk = juce::jmax (pk, outPk.load() * 0.9f);
        vuUi = vu; heat = heatS; agUi = agS;
        if (! running) { vu *= 0.9f; heatS *= 0.9f; }
    }
    std::atomic<float> vuUi { 0 }, outPk { 0 }, heat { 0 }, agUi { 1 };
private:
    struct Sm { float pre = 1, sag = 0, wet = 0, warm = 0, air = 0, enh = 0, mix = 1, tilt = 0; };
    void resetState()
    {
        for (int c = 0; c < 2; ++c)
        {
            for (auto& f : preF[c]) f.reset(); for (auto& f : postF[c]) f.reset(); for (auto& f : aaLp[c]) f.reset();
            for (auto& a : ad[c]) a.reset(); aW[c].reset();
            env[c] = 0; dcx[c] = dcy[c] = 0; wLp1[c] = wLp2[c] = 0; for (auto& f : airF[c]) f.reset(); for (auto& f : enF[c]) f.reset(); hEnv[c] = aEnv[c] = 0;
        }
        lastAir = -1; lastTilt = -100; anyS = 0;
    }
    void resetTube() { for (int c = 0; c < 2; ++c) { for (auto& f : preF[c]) f.reset(); for (auto& f : postF[c]) f.reset(); for (auto& a : ad[c]) a.reset(); for (auto& f : enF[c]) f.reset(); env[c] = dcx[c] = dcy[c] = hEnv[c] = 0; } lastTilt = -100; }
    void setChar (int k)
    {
        const auto& C = charOf (k); curChar = k; compFor = -1;
        for (int c = 0; c < 2; ++c)
        {
            preF[c][0].lowShelf (sro, C.loHz, C.preLo, 0.7); preF[c][1].highShelf (sro, C.hiHz, C.preHi, 0.7);
            postF[c][0].lowShelf (sro, C.loHz, -C.preLo, 0.7); postF[c][1].highShelf (sro, C.hiHz, -C.preHi, 0.7); postF[c][2].lp (sro, juce::jmin ((double) C.postLp, sro * 0.2), 0.6);
            enF[c][0].hp (sro, C.enhHz * 0.7, 0.6); enF[c][1].lp (sro, C.enhHz * 2.0, 0.6); enF[c][2].hp (sro, C.enhHz * 1.3, 0.7);
            airF[c][1].hp (sro, 9000, 0.6); airF[c][2].hp (sro, 11000, 0.7);
        }
    }
    void runOs (float* const* io, int m, const Settings& S)
    {
        const bool mono = std::memcmp (io[0], io[1], sizeof (float) * (size_t) m) == 0; const int nc = mono ? 1 : 2;
        juce::dsp::AudioBlock<float> blk (io, 2, (size_t) m);
        auto up = os->processSamplesUp (blk);
        const int N = (int) up.getNumSamples();
        float* u[2] = { up.getChannelPointer (0), up.getChannelPointer (1) };
        // cambio di carattere: le curve sono diverse, quindi si sfuma (il gate generale lo fa gia' all'accensione)
        const int ck = juce::jlimit (0, kChars - 1, S.character);
        wantChar = ck;
        if (curChar < 0 || (ck != curChar && anyS < 1e-4f)) { setChar (ck); cf = 1; resetTube(); }
        const auto* Kp = &charOf (curChar);
        // bersagli dei comandi
        Sm t;
        const float d = juce::jlimit (0.0f, 1.0f, S.drive / 10.0f);
        const auto& K0 = *Kp;
        t.pre = juce::Decibels::decibelsToGain (-4.0f + d * (K0.driveDb + 4.0f));     // anche a DRIVE basso la valvola colora un poco
        t.wet = juce::jlimit (0.0f, 1.0f, S.drive / 0.8f);                           // da 0 a 0.8 la valvola entra piano: a zero e' neutra
        t.sag = K0.sag * (0.3f + 0.7f * d);
        t.warm = S.warmth / 10.0f; t.air = S.air / 10.0f; t.enh = S.enh / 10.0f; t.mix = S.mix; t.tilt = K0.tilt * t.wet;
        if (! smInit) { sm = t; smInit = true; }
        const float k4 = 1.0f - std::exp (-1.0f / (0.03f * (float) sro));             // 30 ms
        const float eA = 1.0f - std::exp (-1.0f / (0.004f * (float) sro)), eR = 1.0f - std::exp (-1.0f / (0.15f * (float) sro));   // la valvola si siede e si rialza
        const float wA = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 220.0f / (float) sro);
        const float dcA = 1.0f - (float) (juce::MathConstants<double>::twoPi * 6.0 / sro);
        const float eqA = 1.0f - (float) std::exp (-1.0 / (0.002 * sro)), eqR = 1.0f - (float) std::exp (-1.0 / (0.06 * sro));
        TabF f { &tabs[curChar] }; TabFi fi { &tabs[curChar] };
        float vuAcc = vu, heatAcc = heatS;
        for (int i = 0; i < N; ++i)
        {
            sm.pre += (t.pre - sm.pre) * k4; sm.sag += (t.sag - sm.sag) * k4; sm.wet += (t.wet - sm.wet) * k4; sm.warm += (t.warm - sm.warm) * k4;
            sm.air += (t.air - sm.air) * k4; sm.enh += (t.enh - sm.enh) * k4; sm.mix += (t.mix - sm.mix) * k4; sm.tilt += (t.tilt - sm.tilt) * k4;
            anyS = juce::jmax (sm.wet, sm.warm, juce::jmax (sm.air, sm.enh));
            if (wantChar != curChar) { cf -= 1.0f / (0.02f * (float) sro); if (cf <= 0) { cf = 0; setChar (wantChar); Kp = &charOf (curChar); f.t = fi.t = &tabs[curChar]; } }
            else if (cf < 1) cf = juce::jmin (1.0f, cf + 1.0f / (0.03f * (float) sro));
            const auto& K = *Kp;
            if ((tick++ & 31) == 0)
            {
                // volume compensato: un -12 dBFS esce uguale a ogni DRIVE (il resto lo fa AUTO)
                if (std::abs (sm.pre - compFor) > 1e-4f * compFor || compFor < 0)
                {
                    compFor = sm.pre; double r = 0.125 * sm.pre; for (int st = 0; st < K.stages; ++st) r = tabs[curChar].val (r);
                    comp = (float) (0.125 / juce::jmax (1e-6, std::abs (r)));
                }   // filtri che scivolano, a piccoli passi
                if (std::abs (sm.air - lastAir) > 0.002f) { lastAir = sm.air; for (int c = 0; c < 2; ++c) airF[c][0].highShelf (sro, 11000, sm.air * 5.0f, 0.6); }
                if (std::abs (sm.tilt - lastTilt) > 0.01f) { lastTilt = sm.tilt; for (int c = 0; c < 2; ++c) { postF[c][3].highShelf (sro, 6000, sm.tilt, 0.5); postF[c][4].lowShelf (sro, 250, -sm.tilt * 0.5f, 0.5); } }
            }
            float x[2] = { u[0][i], u[1][i] }, y[2] = { x[0], x[1] };
            for (int c = 0; c < nc; ++c)
            {
                float v = x[c];
                // ---- valvola del carattere
                if (sm.wet * cf > 1e-4f)
                {
                    float w = preF[c][1].p (preF[c][0].p (v)) * sm.pre;
                    // la valvola si siede: con il segnale forte la polarizzazione si sposta e il guadagno cala (compressione morbida)
                    const float a = std::abs (w); env[c] += (a > env[c] ? eA : eR) * (a - env[c]);
                    const float sag = sm.sag * juce::jmin (2.0f, env[c]);
                    w /= (1.0f + 0.35f * sag);
                    for (int st = 0; st < K.stages; ++st) w = ad[c][st].run (w * (1.0f + 0.12f * sag), f, fi);
                    // il volume resta quello di prima per i segnali piccoli (la compensazione fine la fa AUTO)
                    w = w * comp * (1.0f + 0.25f * sag);
                    const float yy = w - dcx[c] + dcA * dcy[c]; dcx[c] = w; dcy[c] = yy; w = yy;
                    for (int j = 0; j < 5; ++j) w = postF[c][j].p (w);
                    v = v + (w - v) * sm.wet * cf * cf * (3 - 2 * cf);
                }
                // ---- CALORE: armoniche dai bassi (seconda e terza di cio' che sta sotto i 220 Hz), piu' un filo di corpo
                if (sm.warm > 1e-4f)
                {
                    wLp1[c] += wA * (v - wLp1[c]); wLp2[c] += wA * (wLp1[c] - wLp2[c]);
                    const float lo = wLp2[c], g = 1.5f + 5.0f * sm.warm;
                    const float sat = aW[c].run (lo * g + 0.25f, TanhF(), TanhFi()) / g - std::tanh (0.25f) / g;
                    v += (sat - lo) * (0.6f + 0.8f * sm.warm) * sm.warm + lo * 0.25f * sm.warm;
                }
                // ---- ENHANCER: armoniche nate dalla zona del carattere, solo sopra di essa (la voce viene avanti)
                if (sm.enh > 1e-4f)
                {
                    const float b = enF[c][1].p (enF[c][0].p (v));
                    // inviluppo morbido (niente spigoli che fanno aliasing) e forma limitata con dolcezza
                    hEnv[c] += (b * b > hEnv[c] ? eqA : eqR) * (b * b - hEnv[c]);
                    const float en = std::sqrt (hEnv[c]) * 1.41f + 1e-6f, xx = softU (b / en), x2 = xx * xx;
                    float hh = K.h2 * (2 * x2 - 1) + K.h3 * xx * (4 * x2 - 3) + K.h4 * (8 * x2 * x2 - 8 * x2 + 1) + K.h5 * xx * (16 * x2 * x2 - 20 * x2 + 5);
                    hh *= en * (en / (en + 0.003f));
                    v += enF[c][2].p (hh) * sm.enh * cf * 0.9f;
                }
                // ---- ARIA: mensola dolce sopra gli 11 kHz piu' un velo di armoniche in alto
                if (sm.air > 1e-4f)
                {
                    v = airF[c][0].p (v);
                    const float b = airF[c][1].p (v); aEnv[c] += (b * b > aEnv[c] ? eqA : eqR) * (b * b - aEnv[c]);
                    const float en = std::sqrt (aEnv[c]) * 1.41f + 1e-6f, xx = softU (b / en);
                    v += airF[c][2].p ((2 * xx * xx - 1) * en * (en / (en + 0.002f))) * sm.air * 0.5f;
                }
                for (auto& fl : aaLp[c]) v = fl.p (v);
                y[c] = v;
            }
            if (mono) y[1] = y[0];
            u[0][i] = y[0]; u[1][i] = y[1];
            heatAcc += (std::abs (y[0] - x[0]) + std::abs (y[1] - x[1]) - heatAcc) * 0.00002f;
            vuAcc += (std::abs (y[0]) + std::abs (y[1]) - vuAcc) * 0.00003f;
        }
        if (mono)
        {
            for (int j = 0; j < 2; ++j) preF[1][j] = preF[0][j]; for (int j = 0; j < 5; ++j) postF[1][j] = postF[0][j];
            for (int j = 0; j < 8; ++j) aaLp[1][j] = aaLp[0][j]; for (int j = 0; j < 3; ++j) { airF[1][j] = airF[0][j]; enF[1][j] = enF[0][j]; }
            ad[1][0] = ad[0][0]; ad[1][1] = ad[0][1]; aW[1] = aW[0];
            env[1] = env[0]; dcx[1] = dcx[0]; dcy[1] = dcy[0]; wLp1[1] = wLp1[0]; wLp2[1] = wLp2[0]; hEnv[1] = hEnv[0]; aEnv[1] = aEnv[0];
        }
        os->processSamplesDown (blk);
        vu = vuAcc * 0.5f; heatS = heatAcc;
    }
    double sr = 48000, sro = 192000; int mb = 512, lat = 1;
    std::unique_ptr<juce::dsp::Oversampling<float>> os;
    std::vector<float> ring[2], dryT[2], wetT[2]; int rw = 0, warm = 0; float gate = 0; bool running = false;
    jkit::Bq preF[2][2], postF[2][5], aaLp[2][8], airF[2][3], enF[2][3];
    ShapeTab tabs[kChars]; Adaa<TabF, TabFi> ad[2][2]; Adaa<TanhF, TanhFi> aW[2];
    float env[2] {}, dcx[2] {}, dcy[2] {}, wLp1[2] {}, wLp2[2] {}, hEnv[2] {}, aEnv[2] {};
    int curChar = -1, wantChar = 0; float cf = 1, mixS = 1, comp = 1, lastAir = -1, lastTilt = -100, anyS = 0, outS = -1, vu = 0, heatS = 0, agS = 1, rD = 0, rW = 0;
    Sm sm; bool smInit = false; float compFor = -1; juce::uint32 tick = 0;
};
}
