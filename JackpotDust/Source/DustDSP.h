#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include <atomic>
#include <vector>

namespace jds
{
constexpr double kPi = 3.141592653589793;
inline float dbToG (float d) { return std::pow (10.0f, d / 20.0f); }
inline float gToDb (float g) { return 20.0f * std::log10 (std::max (g, 1e-9f)); }

// ---------------------------------------------------------------- macchine di campionamento
enum SModel { S_CLEAN, S_SP12, S_MPC60, S_S950, S_CRUSH, kSModels };
struct SInfo { const char* name; const char* desc; float bits, rate; int aaOrder; float postLp; float mu; };
inline const SInfo& sInfo (int m)
{
    // aaOrder: 0 = niente filtro anti-alias (aliasing voluto), 2/4/6 = poli del filtro prima del campionamento
    static const SInfo I[kSModels] = {
        { "PULITO",  "Riduzione di bit e frequenza senza colore: filtri puliti prima e dopo.",                                16, 32000, 4, 0.45f, 0 },
        { "SP-12",   "12 bit a 26 kHz senza filtro anti-alias: il morso metallico e granuloso delle batterie anni 80-90.",     12, 26040, 0, 0.47f, 0 },
        { "MPC 60",  "12 bit a 40 kHz con conversione morbida: grosso, caldo, i transienti restano rotondi.",                12, 40000, 4, 0.42f, 6 },
        { "S950",    "12 bit con filtro ripido che segue la frequenza: scuro e pulito, perfetto per break campionati lenti.", 12, 32000, 6, 0.40f, 0 },
        { "CRUSH",   "Bit e frequenza liberi, niente filtri: dal lo-fi sporco al videogioco.",                                 8, 11025, 0, 0.0f, 0 } };
    return I[juce::jlimit (0, kSModels - 1, m)];
}
enum DModel { D_TAPE, D_TUBE, D_CONSOLE, D_PUSH, D_CLIP, kDModels };
inline const char* dName (int m) { static const char* n[] = { "NASTRO", "VALVOLA", "CONSOLE", "SPINTA", "CLIP" }; return n[juce::jlimit (0, kDModels - 1, m)]; }
inline const char* dDesc (int m)
{
    static const char* d[] = { "Compressione morbida e simmetrica: arrotonda i picchi, ingrossa il corpo.",
                               "Asimmetrica, armoniche pari: calda, gonfia la cassa e ammorbidisce i piatti.",
                               "Trasformatore da banco: un filo di peso in basso e saturazione presente sulle medie.",
                               "Clip deciso ma con ginocchio: transienti spezzati, rullante che spacca.",
                               "Clipper da trap e drill: taglia i picchi netto (a 8x, senza aliasing), batteria piu' forte e in faccia." };
    return d[juce::jlimit (0, kDModels - 1, m)];
}
enum CStyle { C_FET, C_BUS, C_PUMP, C_670, C_OTT, kCStyles };
inline const char* cName (int m) { static const char* n[] = { "FET", "BUS", "PUMP", "670", "OTT" }; return n[juce::jlimit (0, kCStyles - 1, m)]; }
inline const char* cDesc (int m)
{
    static const char* d[] = { "Veloce sui picchi come un 1176: morde i transienti, oltre meta' aggiunge lo sporco dei bottoni tutti premuti.",
                               "VCA da bus: lento e morbido, tiene insieme il kit. La colla.",
                               "Respira e pompa: le code salgono tra un colpo e l'altro (trip hop, boom bap lento).",
                               "Valvole a pendenza variabile stile Fairchild 670: piu' spingi piu' comprime, ingrossa e scalda. Il suono dei break grossi.",
                               "Multibanda su/giu' stile OTT: alza i dettagli piccoli e schiaccia i picchi su 3 bande. Hat lucidi, trap, batterie in faccia." };
    return d[juce::jlimit (0, kCStyles - 1, m)];
}

// ---------------------------------------------------------------- mattoni
struct Biquad
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    inline float run (float x) { const double y = b0 * x + z1; z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y; return (float) y; }
    void reset() { z1 = z2 = 0; }
    void lp (double f, double q, double sr) { const double w = 2 * kPi * std::min (f, sr * 0.49) / sr, c = std::cos (w), al = std::sin (w) / (2 * q), a0 = 1 + al; b0 = (1 - c) / 2 / a0; b1 = (1 - c) / a0; b2 = b0; a1 = -2 * c / a0; a2 = (1 - al) / a0; }
    void hp (double f, double q, double sr) { const double w = 2 * kPi * std::min (f, sr * 0.49) / sr, c = std::cos (w), al = std::sin (w) / (2 * q), a0 = 1 + al; b0 = (1 + c) / 2 / a0; b1 = -(1 + c) / a0; b2 = b0; a1 = -2 * c / a0; a2 = (1 - al) / a0; }
    void bell (double f, double q, double db, double sr) { const double A = std::pow (10.0, db / 40), w = 2 * kPi * std::min (f, sr * 0.49) / sr, c = std::cos (w), al = std::sin (w) / (2 * q), a0 = 1 + al / A; b0 = (1 + al * A) / a0; b1 = -2 * c / a0; b2 = (1 - al * A) / a0; a1 = b1; a2 = (1 - al / A) / a0; }
    void lowShelf (double f, double db, double sr) { const double A = std::pow (10.0, db / 40), w = 2 * kPi * f / sr, c = std::cos (w), s = std::sin (w), al = s / 2 * std::sqrt (2.0), sq = 2 * std::sqrt (A) * al, a0 = (A + 1) + (A - 1) * c + sq;
                                                     b0 = A * ((A + 1) - (A - 1) * c + sq) / a0; b1 = 2 * A * ((A - 1) - (A + 1) * c) / a0; b2 = A * ((A + 1) - (A - 1) * c - sq) / a0; a1 = -2 * ((A - 1) + (A + 1) * c) / a0; a2 = ((A + 1) + (A - 1) * c - sq) / a0; }
    void highShelf (double f, double db, double sr) { const double A = std::pow (10.0, db / 40), w = 2 * kPi * std::min (f, sr * 0.45) / sr, c = std::cos (w), s = std::sin (w), al = s / 2 * std::sqrt (2.0), sq = 2 * std::sqrt (A) * al, a0 = (A + 1) - (A - 1) * c + sq;
                                                      b0 = A * ((A + 1) + (A - 1) * c + sq) / a0; b1 = -2 * A * ((A - 1) + (A + 1) * c) / a0; b2 = A * ((A + 1) + (A - 1) * c - sq) / a0; a1 = 2 * ((A - 1) - (A + 1) * c) / a0; a2 = ((A + 1) - (A - 1) * c - sq) / a0; }
};
// filtro a stato variabile (TPT): passa basso e passa alto risonanti
struct Svf
{
    float ic1 = 0, ic2 = 0, g = 0, k = 1.4f, a1 = 0, a2 = 0, a3 = 0;
    void set (float f, float res, double sr) { g = (float) std::tan (kPi * std::min ((double) f, sr * 0.45) / sr); k = 1.4142f - 1.33f * res; a1 = 1 / (1 + g * (g + k)); a2 = g * a1; a3 = g * a2; }
    inline void tick (float x, float& lp, float& hp) { const float v3 = x - ic2, v1 = a1 * ic1 + a2 * v3, v2 = ic2 + a2 * ic1 + a3 * v3; ic1 = 2 * v1 - ic1; ic2 = 2 * v2 - ic2; lp = v2; hp = x - k * v1 - v2; }
    void reset() { ic1 = ic2 = 0; }
};
struct Rng { juce::uint32 s = 22222; inline float next() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return (float) s * 2.3283064e-10f * 2.0f - 1.0f; } inline float uni() { return 0.5f + 0.5f * next(); } };
struct Dl
{
    std::vector<float> d; int w = 0, mask = 0;
    void prepare (int n) { int m = 1; while (m < n + 4) m <<= 1; d.assign ((size_t) m, 0.0f); mask = m - 1; w = 0; }
    inline void push (float x) { d[(size_t) w] = x; w = (w + 1) & mask; }
    inline float read (float t) const { const float rp = (float) w - 1 - t; const int i = (int) std::floor (rp); const float f = rp - i; const float a = d[(size_t) (i & mask)], b = d[(size_t) ((i + 1) & mask)]; return a + (b - a) * f; }
};

// curve di saturazione (forma senza compensazione di guadagno)
inline float shapeFn (int m, float x)
{
    switch (m)
    {
        case D_TUBE:    { const float b = 0.18f; return std::tanh (x + b) - std::tanh (b) + 0.08f * x / (1 + std::abs (x)); }
        case D_CONSOLE: { const float y = x / std::sqrt (1 + 0.6f * x * x); return std::tanh (1.15f * y) / std::tanh (1.15f); }
        case D_CLIP:    return juce::jlimit (-1.0f, 1.0f, x * 0.98f + 0.02f * std::tanh (x));
        case D_PUSH:    { const float a = std::abs (x); const float y = a < 0.6f ? a : (a < 1.6f ? 0.6f + (a - 0.6f) * (1 - (a - 0.6f) * 0.5f) : 1.1f); return std::copysign (std::min (y, 1.1f), x) / 1.1f * 1.05f; }
        default:        return std::tanh (x);
    }
}

// ---------------------------------------------------------------- impostazioni
struct Settings
{
    float inDb = 0;
    bool sOn = false; int sModel = S_SP12; float sBits = 12, sRate = 26040, sMix = 1;
    bool dOn = false; int dModel = D_TAPE; float dDrive = 6, dTone = 0, dMix = 1;
    bool cOn = false; int cStyle = C_FET; float cAmt = 0.4f, cAtt = 10, cRel = 120, cPunch = 0, cMix = 1;
    bool fOn = false; float fHp = 20, fLp = 20000, fRes = 0, fBoom = 0, fCrack = 0, fWarm = 0, fAir = 0; bool f24 = false;
    bool tOn = false; float tAge = 0.4f, tWow = 0.2f, tFlut = 0.2f, tDrop = 0;
    bool nOn = false; float nVinyl = 0.4f, nPops = 0.3f, nHiss = 0.2f, nHum = 0; bool nFollow = true;
    bool rOn = false; float rSize = 0.4f, rDecay = 0.6f, rTone = 0.5f, rMix = 0.2f; bool rGate = false;
    float amount = 1, mix = 1, outDb = 0; bool mono = false, autoGain = false;
};

// ---------------------------------------------------------------- motore
// Ordine: sampler (1x, l'aliasing delle macchine e' voluto) -> [8x: saturazione, filtri, compressore, nastro] -> room -> polvere -> uscita.
// Le manopole che cambiano livello o colore sono smussate per campione (niente click), i filtri si aggiornano a piccoli passi.
struct Sm1 { float v = 0; bool init = false; inline float step (float t, float k) { if (! init) { v = t; init = true; } v += (t - v) * k; return v; } };

class Engine
{
public:
    static constexpr int kOsSt = 3, kOs = 8;
    void prepare (double s, int block)
    {
        sr = s; maxBlock = block; sr4 = s * kOs;
        os = std::make_unique<juce::dsp::Oversampling<float>> (2, kOsSt, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, true);
        os->initProcessing ((size_t) block); osLat = (int) std::round (os->getLatencyInSamples());
        dryDl[0].prepare (osLat + block + 8); dryDl[1].prepare (osLat + block + 8); bypDl[0].prepare (osLat + 8); bypDl[1].prepare (osLat + 8); upTail = 0;
        for (auto& t : tapeDl) t.prepare ((int) (sr4 * 0.03));
        satDry.setSize (2, block * kOs);
        for (auto& c : ch) c = Ch();
        holdPh = 1; env = 0; inRms = outRms = 1e-6f; gr = 0; dropG = 1; dropT = 0; wowPh = flPh = fl2Ph = 0;
        humPh = 0; noiseEnv = 0; crackLp[0] = crackLp[1] = popZ[0] = popZ[1] = 0; rms = envF = envS = 0; muCharge = 0;
        for (auto& e : ottEnv) e = 0;
        for (auto& v : smv) v = Sm1();
        lastRate = -1; lastModel = -1; for (auto& f : fk) f = -1;
        // room
        for (int c = 0; c < 2; ++c) erDl[c].prepare ((int) (sr * 0.12));
        for (int k = 0; k < 4; ++k) { fdn[k].prepare ((int) (sr * 0.2)); fdnLp[k] = 0; }
        roomEnv = 0; roomHold = 0; roomGate = 0; wetHp[0] = wetHp[1] = 0;
        ottSr = -1;
    }
    int latency() const { return osLat; }

    void process (float* L, float* R, int n, const Settings& S)
    {
        const float A = S.amount;
        const float kS = 1.0f - std::exp (-1.0f / (0.015f * (float) sr)), kS4 = 1.0f - std::exp (-1.0f / (0.015f * (float) sr4));
        for (int i = 0; i < n; ++i) { const float g = dbToG (smv[0].step (S.inDb, kS)); L[i] *= g; R[i] *= g; }
        // copia asciutta per il MIX generale (ritardata della latenza dell'oversampling)
        for (int i = 0; i < n; ++i) { dryDl[0].push (L[i]); dryDl[1].push (R[i]); }
        float* io[2] = { L, R };
        double ein = 0; for (int i = 0; i < n; ++i) ein += 0.5 * (L[i] * L[i] + R[i] * R[i]);

        // 1) campionatore d'epoca (a 1x: e' la macchina vera, aliasing compreso)
        const float sMixT = S.sOn ? std::min (1.0f, S.sMix * A) : 0.0f;
        if (S.sOn || smv[1].v > 0.0005f)
        {
            const auto& M = sInfo (S.sModel);
            const float rate = std::min (S.sRate, (float) sr);
            if (rate != lastRate || S.sModel != lastModel)
            {
                lastRate = rate; lastModel = S.sModel;
                for (int c = 0; c < 2; ++c)
                {
                    for (int k = 0; k < 3; ++k) { ch[c].aa[k].lp (rate * 0.45, k == 0 ? 0.54 : k == 1 ? 1.31 : 0.71, sr); }
                    if (M.postLp > 0) { ch[c].post[0].lp (rate * M.postLp, 0.71, sr); ch[c].post[1].lp (rate * M.postLp * 1.1, 0.6, sr); }
                }
            }
            const float steps = std::pow (2.0f, std::round (S.sBits) - 1.0f), inc = rate / (float) sr;
            for (int i = 0; i < n; ++i)
            {
                const float mixS = smv[1].step (sMixT, kS);
                float x[2] = { L[i], R[i] };
                for (int c = 0; c < 2; ++c) { float v = x[c]; for (int k = 0; k < M.aaOrder / 2; ++k) v = ch[c].aa[k].run (v); ch[c].pre = v; }
                holdPh += inc;
                if (holdPh >= 1.0f)
                {
                    holdPh -= 1.0f;
                    for (int c = 0; c < 2; ++c)
                    {
                        float v = juce::jlimit (-1.0f, 1.0f, ch[c].pre);
                        if (M.mu > 0) v = std::copysign (std::log1p (M.mu * std::abs (v)) / std::log1p (M.mu), v);
                        v = std::round (v * steps) / steps;
                        if (M.mu > 0) v = std::copysign ((std::pow (1 + M.mu, std::abs (v)) - 1) / M.mu, v);
                        ch[c].held = v;
                    }
                }
                for (int c = 0; c < 2; ++c) { float v = ch[c].held; if (M.postLp > 0) v = ch[c].post[1].run (ch[c].post[0].run (v)); io[c][i] = x[c] + (v - x[c]) * mixS; }
            }
        }
        else smv[1].step (0, 1);

        // 2-5) a 8x: niente aliasing da saturazione, clip, compressore e nastro
        // l'oversampling gira sempre (latenza costante, nessun salto quando accendi un modulo); i moduli spenti non costano nulla
        {
            juce::dsp::AudioBlock<float> blk (io, 2, (size_t) n);
            auto up = os->processSamplesUp (blk);
            const int N = (int) up.getNumSamples();
            float* u[2] = { up.getChannelPointer (0), up.getChannelPointer (1) };
            processUp (u, N, S, A, kS4);
            os->processSamplesDown (blk);
        }

        // 6) room: piccola stanza per batterie grandi (prime riflessioni + coda corta), con gate opzionale
        processRoom (io, n, S, kS);
        // 7) polvere: vinile, crackle, pop, hiss, ronzio
        if (S.nOn && A > 0.001f) processDust (io, n, S, A);
        // 8) uscita: mix parallelo, mono, guadagno automatico, volume
        double eout = 0;
        for (int i = 0; i < n; ++i)
        {
            float l = io[0][i], r = io[1][i];
            const float mx = smv[2].step (S.mix, kS);
            if (mx < 0.9995f) { const float t = (float) (osLat + n - 1 - i), dl = dryDl[0].read (t), dr = dryDl[1].read (t); l = dl + (l - dl) * mx; r = dr + (r - dr) * mx; }
            const float mo = smv[3].step (S.mono ? 1.0f : 0.0f, kS);
            if (mo > 0.0005f) { const float m = 0.5f * (l + r); l += (m - l) * mo; r += (m - r) * mo; }
            eout += 0.5 * (l * l + r * r);
            io[0][i] = l; io[1][i] = r;
        }
        const float k = std::exp (-(float) n / (float) (sr * 1.5));
        inRms = k * inRms + (1 - k) * (float) (ein / n); outRms = k * outRms + (1 - k) * (float) (eout / n);
        float ag = 1;
        if (S.autoGain && inRms > 1e-7f) ag = juce::jlimit (0.25f, 4.0f, std::sqrt (inRms / std::max (outRms, 1e-9f)));
        // protezione: sopra -1 dBFS un clip morbido (sotto e trasparente)
        auto safe = [] (float x) { const float a = std::abs (x); return a <= 0.9f ? x : std::copysign (0.9f + 0.1f * std::tanh ((a - 0.9f) / 0.1f), x); };
        for (int i = 0; i < n; ++i)
        {
            agS += ((S.autoGain ? ag : 1.0f) - agS) * 0.0005f;
            const float og = dbToG (smv[4].step (S.outDb, kS)) * agS;
            io[0][i] = safe (io[0][i] * og); io[1][i] = safe (io[1][i] * og);
        }
    }

    std::atomic<float> gr { 0 };
    float agS = 1;
private:
    // ------------------------------------------------ parte a 8x
    void processUp (float** io, int n, const Settings& S, float A, float kS)
    {
        // saturazione / clip
        const float driveT = S.dOn ? S.dDrive * A : 0.0f, dMixT = S.dOn ? S.dMix : 0.0f;
        if (S.dOn || smv[5].v > 0.0005f)
        {
            if (S.dModel != lastDModel || std::abs (driveT - mkFor) > 0.25f) { lastDModel = S.dModel; mkFor = driveT; driveMk = makeup (S.dModel, driveT); }
            for (int c = 0; c < 2; ++c) { ch[c].tiltLo.lowShelf (250, -S.dTone * 4.0, sr4); ch[c].tiltHi.lowShelf (250, S.dTone * 4.0, sr4); }
            for (int i = 0; i < n; ++i)
            {
                const float dr = smv[6].step (driveT, kS), mx = smv[5].step (dMixT, kS), g = dbToG (dr);
                const float mk = driveMk;
                for (int c = 0; c < 2; ++c)
                {
                    const float x = io[c][i];
                    const float y = ch[c].tiltHi.run (shapeFn (S.dModel, ch[c].tiltLo.run (x) * g) * mk);
                    io[c][i] = x + (y - x) * mx;
                }
            }
        }
        else { smv[5].step (0, 1); smv[6].step (driveT, 1); }
        // filtri ed EQ da batteria (coefficenti aggiornati a passi di 32 campioni con valori smussati)
        if (S.fOn)
        {
            for (int o = 0; o < n; o += 32)
            {
                const int m = std::min (32, n - o);
                Settings F = S; const float kb = 1.0f - std::exp (-(float) m / (0.02f * (float) sr4));
                F.fHp = fs[0].step (S.fHp, kb); F.fLp = fs[1].step (S.fLp, kb); F.fBoom = fs[2].step (S.fBoom, kb); F.fCrack = fs[3].step (S.fCrack, kb);
                F.fWarm = fs[4].step (S.fWarm, kb); F.fAir = fs[5].step (S.fAir, kb); F.fRes = fs[6].step (S.fRes, kb);
                updateFilters (F);
                for (int c = 0; c < 2; ++c)
                {
                    auto& C = ch[c];
                    for (int i = o; i < o + m; ++i)
                    {
                        float v = io[c][i], lp, hp;
                        if (F.fHp > 21) { C.hp1.tick (v, lp, hp); v = hp; if (S.f24) { C.hp2.tick (v, lp, hp); v = hp; } }
                        if (F.fLp < 19900) { C.lp1.tick (v, lp, hp); v = lp; if (S.f24) { C.lp2.tick (v, lp, hp); v = lp; } }
                        if (std::abs (F.fBoom) > 0.02f) v = C.boom.run (v);
                        if (std::abs (F.fCrack) > 0.02f) v = C.crack.run (v);
                        if (F.fWarm > 0.001f)
                        {   // EQ caldo: bassi col trucco del Pultec (boost a 85 + dip a 320), alti arrotondati, trasformatore e valvola
                            v = C.wLo.run (v); v = C.wDip.run (v); v = C.wHi.run (v);
                            const float w = F.fWarm;
                            C.wLp += (v - C.wLp) * wLpA; const float lo = C.wLp, kd = 1.0f + w * 2.5f;
                            v += std::tanh (lo * kd) / kd - lo;
                            const float b = 0.18f * w, d = 1.0f + w * 1.2f, nn = d * (1 - std::tanh (b) * std::tanh (b));
                            v = (std::tanh (d * v + b) - std::tanh (b)) / nn;
                            C.wDc += (v - C.wDc) * 0.00005f; v -= C.wDc;
                        }
                        if (F.fAir > 0.001f) v = C.air.run (v);
                        io[c][i] = v;
                    }
                }
            }
        }
        // compressore e punch
        const float cMixT = S.cOn ? S.cMix : 0.0f;
        if (S.cOn || smv[7].v > 0.0005f)
        {
            if (S.cStyle == C_OTT) processOtt (io, n, S, A, kS);
            else processComp (io, n, S, A, kS);
        }
        else { gr = 0; smv[7].step (cMixT, 1); }
        // nastro: wow, flutter, eta, dropout
        if (S.tOn) processTape (io, n, S, A);
    }

    static float makeup (int model, float drive)
    {   // compenso il volume: una sinusoide a -6 dBFS (livello tipico dei colpi) esce allo stesso livello
        const float g = dbToG (drive); double e = 0;
        for (int k = 0; k < 64; ++k) { const float y = shapeFn (model, g * 0.5f * std::sin (2 * (float) kPi * k / 64)); e += y * y; }
        return 0.5f / (float) std::sqrt (e / 64 + 1e-12) * 0.7071f;
    }

    void processComp (float** io, int n, const Settings& S, float A, float kS)
    {
        const int st = S.cStyle;
        const float aRms = std::exp (-1.0f / (0.01f * (float) sr4));
        const float pA1 = std::exp (-1.0f / (0.0008f * (float) sr4)), pR1 = std::exp (-1.0f / (0.04f * (float) sr4)), pA2 = std::exp (-1.0f / (0.02f * (float) sr4)), pR2 = std::exp (-1.0f / (0.15f * (float) sr4));
        const float att = (st == C_PUMP ? std::max (S.cAtt, 15.0f) : st == C_670 ? std::max (S.cAtt * 0.5f, 0.4f) : S.cAtt) * 0.001f, rel = (st == C_PUMP ? S.cRel * 2 : S.cRel) * 0.001f;
        const float aA = std::exp (-1.0f / (att * (float) sr4)), aR = std::exp (-1.0f / (rel * (float) sr4));
        float maxGr = 0;
        for (int i = 0; i < n; ++i)
        {
            const float amt = smv[8].step (std::min (1.0f, S.cAmt * A), kS), mixC = smv[7].step (S.cOn ? S.cMix : 0.0f, kS), punch = smv[9].step (S.cPunch, kS);
            const float thr = st == C_FET ? -amt * 30 : st == C_BUS ? -amt * 24 : st == C_670 ? -amt * 30 : -amt * 36;
            const float knee = st == C_BUS ? 10.0f : st == C_670 ? 16.0f : 6.0f;
            float makeupDb = 0;
            {   // compensazione: il 70% della riduzione che avrebbe un segnale a -10 dBFS
                const float ratio0 = st == C_FET ? 4 + amt * 16 : st == C_BUS ? 2 + 2 * amt : st == C_670 ? 3.0f : 10;
                const float ov = -10.0f - thr;
                if (ov > knee / 2) makeupDb = ov * (1 - 1 / ratio0); else if (ov > -knee / 2) { const float z = ov + knee / 2; makeupDb = (1 - 1 / ratio0) * z * z / (2 * knee); }
                makeupDb *= 0.7f;
            }
            const float l = io[0][i], r = io[1][i], a = std::max (std::abs (l), std::abs (r));
            float pg = 1;
            if (std::abs (punch) > 0.005f)
            {   // punch: differenza tra inviluppo veloce e lento
                envF = a > envF ? pA1 * envF + (1 - pA1) * a : pR1 * envF + (1 - pR1) * a;
                envS = a > envS ? pA2 * envS + (1 - pA2) * a : pR2 * envS + (1 - pR2) * a;
                const float t = juce::jlimit (0.0f, 1.0f, (envF - envS) / (envS + 1e-4f));
                pg = dbToG (punch * (punch > 0 ? 8.0f : 10.0f) * t);
            }
            float det;
            if (st == C_FET || st == C_670) det = gToDb (a);
            else { rms = aRms * rms + (1 - aRms) * 0.5f * (l * l + r * r); det = 10.0f * std::log10 (rms + 1e-12f); }
            const float over = det - thr;
            // 670: il rapporto cresce con quanto si va oltre la soglia (valvola remote-cutoff)
            const float ratio = st == C_FET ? 4 + amt * 16 : st == C_BUS ? 2 + 2 * amt : st == C_670 ? juce::jmin (14.0f, 1.6f + juce::jmax (0.0f, over) * 0.35f) : 10;
            float want = 0;
            if (over > knee / 2) want = over * (1 - 1 / ratio);
            else if (over > -knee / 2) { const float z = over + knee / 2; want = (1 - 1 / ratio) * z * z / (2 * knee); }
            if (st == C_670)
            {   // rilascio a due tempi come TC5: veloce sui colpi singoli, lento se continua a lavorare
                const float rr = rel * (1.0f + 4.0f * muCharge);
                const float aR2 = std::exp (-1.0f / (rr * (float) sr4));
                env = want > env ? aA * env + (1 - aA) * want : aR2 * env + (1 - aR2) * want;
                muCharge += ((env > 2 ? 1.0f : 0.0f) - muCharge) * (1.0f / (2.0f * (float) sr4));
            }
            else env = want > env ? aA * env + (1 - aA) * want : aR * env + (1 - aR) * want;
            maxGr = std::max (maxGr, env);
            const float gg = dbToG (makeupDb - env) * pg;
            float yl = l * gg, yr = r * gg;
            if (st == C_FET && amt > 0.5f) { const float k = 1 + (amt - 0.5f) * 1.6f; yl = std::tanh (yl * k) / k; yr = std::tanh (yr * k) / k; }   // tutti i bottoni: un po' di sporco
            if (st == C_670)
            {   // valvola: seconda armonica che cresce con la riduzione, il corpo si gonfia
                const float b = 0.06f + 0.012f * std::min (env, 15.0f), d = 1.0f + 0.4f * amt, nn = d * (1 - std::tanh (b) * std::tanh (b));
                yl = (std::tanh (d * yl + b) - std::tanh (b)) / nn; yr = (std::tanh (d * yr + b) - std::tanh (b)) / nn;
                for (int c = 0; c < 2; ++c) { float& y = c ? yr : yl; ch[c].muDc += (y - ch[c].muDc) * 0.00003f; y -= ch[c].muDc; }
            }
            io[0][i] = l + (yl - l) * mixC; io[1][i] = r + (yr - r) * mixC;
        }
        gr = maxGr;
    }

    // OTT: tre bande (120 Hz / 2.5 kHz), giu' sopra -20 dB, su sotto -40 dB, profondita' = QUANTITA
    void processOtt (float** io, int n, const Settings& S, float A, float kS)
    {
        if (ottSr != sr4) { for (int c = 0; c < 2; ++c) { ch[c].xo[0].lp (120, 0.7071, sr4); ch[c].xo[1].lp (120, 0.7071, sr4); ch[c].xo[2].hp (2500, 0.7071, sr4); ch[c].xo[3].hp (2500, 0.7071, sr4); } ottSr = sr4; }
        static const float baseA[3] = { 47.8f, 22.4f, 13.5f }, baseR[3] = { 282, 282, 132 };
        const float tA = S.cAtt / 10.0f, tR = S.cRel / 150.0f;
        float ca[3], cr[3]; for (int b = 0; b < 3; ++b) { ca[b] = std::exp (-1.0f / (0.001f * baseA[b] * tA * (float) sr4)); cr[b] = std::exp (-1.0f / (0.001f * baseR[b] * tR * (float) sr4)); }
        float maxDown = 0;
        for (int i = 0; i < n; ++i)
        {
            const float depth = smv[8].step (std::min (1.0f, S.cAmt * A), kS), mixC = smv[7].step (S.cOn ? S.cMix : 0.0f, kS);
            float band[3][2];
            for (int c = 0; c < 2; ++c)
            {
                const float x = io[c][i];
                const float lo = ch[c].xo[1].run (ch[c].xo[0].run (x)), hi = ch[c].xo[3].run (ch[c].xo[2].run (x));
                band[0][c] = lo; band[2][c] = hi; band[1][c] = x - lo - hi;
            }
            float yl = 0, yr = 0;
            for (int b = 0; b < 3; ++b)
            {
                const float lev = std::max (std::abs (band[b][0]), std::abs (band[b][1]));
                ottEnv[b] = lev > ottEnv[b] ? ca[b] * ottEnv[b] + (1 - ca[b]) * lev : cr[b] * ottEnv[b] + (1 - cr[b]) * lev;
                const float db = gToDb (ottEnv[b] + 1e-9f);
                float gdb = 0;
                if (db > -20) gdb -= (db + 20) * (1 - 1.0f / 8.0f);
                if (db < -40 && db > -80) gdb += std::min ((-40 - db) * (1 - 1.0f / 3.0f), 24.0f);
                gdb *= depth; maxDown = std::max (maxDown, -gdb);
                const float gg = dbToG (gdb + 3.0f * depth);   // un filo di guadagno come l'OUT dell'originale
                yl += band[b][0] * gg; yr += band[b][1] * gg;
            }
            io[0][i] += (yl - io[0][i]) * mixC; io[1][i] += (yr - io[1][i]) * mixC;
        }
        gr = maxDown;
    }

    void processTape (float** io, int n, const Settings& S, float A)
    {
        const float age = std::min (1.5f, S.tAge * A), wow = S.tWow * A, flut = S.tFlut * A, drop = S.tDrop * A;
        if (std::abs (age - lastAge) > 0.002f) { lastAge = age; for (int c = 0; c < 2; ++c) { ch[c].bump.bell (85, 0.8, age * 2.5, sr4); ch[c].tLp1.lp (20000 * (1 - 0.72f * std::min (1.0f, age)), 0.6, sr4); } }
        const float wA = (float) (0.0087 * wow / (2 * kPi * 0.55) * sr4), fA = (float) (0.0025 * flut / (2 * kPi * 6.3) * sr4), f2A = (float) (0.001 * flut / (2 * kPi * 11.0) * sr4);
        const float sat = 1 + age * 1.5f;
        for (int i = 0; i < n; ++i)
        {
            wowPh += 0.55 / sr4; flPh += 6.3 / sr4; fl2Ph += 11.0 / sr4;
            if (wowPh > 1) { wowPh -= 1; wowTarget = rng.next() * 0.4f; }
            wowDrift += 0.00003f * (wowTarget - wowDrift);
            wA_s += (wA - wA_s) * 0.0001f; fA_s += (fA - fA_s) * 0.0001f; f2A_s += (f2A - f2A_s) * 0.0001f;
            const float d = 2 * kOs + wA_s * (1 + (float) std::sin (2 * kPi * wowPh) + wowDrift) + fA_s * (1 + (float) std::sin (2 * kPi * flPh)) + f2A_s * (1 + (float) std::sin (2 * kPi * fl2Ph));
            if (drop > 0.001f)
            {
                if (dropT <= 0 && rng.uni() < drop * 1.5f / (float) sr4) dropT = (int) (sr4 * (0.02 + 0.1 * rng.uni()));
                const float tgt = dropT > 0 ? 0.35f + 0.06f * rng.uni() : 1.0f; if (dropT > 0) --dropT;
                dropG += (tgt - dropG) * 0.0006f;
            }
            else dropG += (1.0f - dropG) * 0.0006f;
            for (int c = 0; c < 2; ++c)
            {
                auto& C = ch[c];
                tapeDl[c].push (io[c][i]);
                float v = tapeDl[c].read (d + (c ? 0.3f * kOs : 0.0f));
                if (age > 0.001f) { v = std::tanh (v * sat) / sat * (1 + age * 0.15f); v = C.tLp1.run (C.bump.run (v)); }
                io[c][i] = v * dropG;
            }
        }
    }

    // stanza da batteria: 8 prime riflessioni per lato + 4 linee con smorzamento; GATE chiude la coda dopo il colpo
    void processRoom (float** io, int n, const Settings& S, float kS)
    {
        const float mixT = S.rOn ? S.rMix : 0.0f;
        if (! S.rOn && smv[10].v < 0.0005f) { smv[10].step (0, 1); return; }
        const float size = 0.35f + 1.15f * S.rSize;
        static const float er[8] = { 4.3f, 7.9f, 11.7f, 15.1f, 19.3f, 23.9f, 29.3f, 34.1f };
        static const float fl[4] = { 29.7f, 37.1f, 41.3f, 53.9f };
        const float rt = 0.15f + 2.35f * S.rDecay * S.rDecay;
        float fb[4], flT[4]; for (int k = 0; k < 4; ++k) { fb[k] = std::pow (10.0f, -3.0f * fl[k] * size * 0.001f / rt); flT[k] = fl[k] * size * 0.001f * (float) sr; }
        float erT[8], erG[8]; for (int k = 0; k < 8; ++k) { erT[k] = er[k] * size * 0.001f * (float) sr; erG[k] = 0.75f * std::pow (0.82f, (float) k); }
        const float damp = 1.0f - std::exp (-2 * (float) kPi * (1500.0f + 12000.0f * S.rTone) / (float) sr);
        const float hpA = 1.0f - std::exp (-2 * (float) kPi * 140.0f / (float) sr);
        const float gA = std::exp (-1.0f / (0.002f * (float) sr)), gR = std::exp (-1.0f / (0.03f * (float) sr)), holdN = (float) (sr * (0.06 + 0.16 * S.rSize));
        for (int i = 0; i < n; ++i)
        {
            const float mx = smv[10].step (mixT, kS);
            const float l = io[0][i], r = io[1][i];
            erDl[0].push (l); erDl[1].push (r);
            float e[2] = { 0, 0 };
            for (int k = 0; k < 8; ++k) { const float t = erT[k], g = erG[k]; e[k & 1] += erDl[k & 1].read (t) * g; e[(k + 1) & 1] += erDl[(k + 1) & 1].read (t * 1.07f) * g * 0.45f; }
            // coda: matrice di Householder 4x4
            float o[4]; for (int k = 0; k < 4; ++k) { o[k] = fdn[k].read (flT[k]); fdnLp[k] += (o[k] - fdnLp[k]) * damp; o[k] = fdnLp[k]; }
            const float sum = 0.5f * (o[0] + o[1] + o[2] + o[3]);
            const float inj[4] = { e[0] * 0.5f, e[1] * 0.5f, e[0] * 0.35f, e[1] * 0.35f };
            for (int k = 0; k < 4; ++k) fdn[k].push ((o[k] - sum) * fb[k] + inj[k]);
            float wl = e[0] * 0.6f + o[0] + o[2] * 0.7f, wr = e[1] * 0.6f + o[1] + o[3] * 0.7f;
            // niente fango: la stanza non porta i bassi sotto 140 Hz
            wetHp[0] += (wl - wetHp[0]) * hpA; wl -= wetHp[0]; wetHp[1] += (wr - wetHp[1]) * hpA; wr -= wetHp[1];
            if (S.rGate)
            {   // gate da anni 80: si apre sul colpo, tiene, poi chiude di netto
                const float a = std::max (std::abs (l), std::abs (r));
                roomEnv = a > roomEnv ? gA * roomEnv + (1 - gA) * a : 0.999f * roomEnv;
                if (roomEnv > 0.05f && a > roomEnv * 0.8f) roomHold = holdN;
                const float tgt = roomHold > 0 ? 1.0f : 0.0f; if (roomHold > 0) roomHold -= 1;
                roomGate = tgt > roomGate ? 0.6f * roomGate + 0.4f * tgt : gR * roomGate;
                wl *= roomGate; wr *= roomGate;
            }
            io[0][i] = l + wl * mx * 0.8f; io[1][i] = r + wr * mx * 0.8f;
        }
    }

    void processDust (float** io, int n, const Settings& S, float A)
    {
        const float aE = std::exp (-1.0f / (0.004f * (float) sr)), rE = std::exp (-1.0f / (0.12f * (float) sr));
        const float surf = S.nVinyl > 0 ? dbToG (-62 + 34 * S.nVinyl) * A : 0, crackRate = S.nVinyl * S.nVinyl * 600.0f, popRate = S.nPops * 2.5f;
        const float hiss = S.nHiss > 0 ? dbToG (-72 + 36 * S.nHiss) * A : 0, hum = S.nHum > 0 ? dbToG (-70 + 36 * S.nHum) * A : 0;
        const float cLevel = dbToG (-34 + 20 * S.nVinyl) * A, pLevel = dbToG (-22 + 10 * S.nPops) * A;
        for (int i = 0; i < n; ++i)
        {
            const float a = 0.5f * (std::abs (io[0][i]) + std::abs (io[1][i]));
            noiseEnv = a > noiseEnv ? aE * noiseEnv + (1 - aE) * a : rE * noiseEnv + (1 - rE) * a;
            const float gate = S.nFollow ? juce::jlimit (0.0f, 1.0f, (gToDb (noiseEnv + 1e-9f) + 50) / 20) : 1.0f;
            humPh += 50.0 / sr; if (humPh > 1) humPh -= 1;
            const float hm = hum * (float) (std::sin (2 * kPi * humPh) + 0.5 * std::sin (4 * kPi * humPh) + 0.3 * std::sin (6 * kPi * humPh + 0.7));
            for (int c = 0; c < 2; ++c)
            {
                auto& C = ch[c];
                const float w = rng.next();
                C.pink = 0.985f * C.pink + 0.015f * w; const float s = C.pink * 4.0f * surf;
                float ck = 0;
                if (rng.uni() < crackRate / (float) sr) { const float u = rng.uni(); ck = (rng.next() > 0 ? 1.0f : -1.0f) * u * u * u * cLevel; }
                crackLp[c] = 0.55f * crackLp[c] + 0.45f * ck; const float ckF = ck - crackLp[c] * 0.6f;
                if (rng.uni() < popRate / (float) sr * 0.5f) popZ[c] = (rng.next() > 0 ? 1.0f : -1.0f) * pLevel * (0.5f + 0.5f * rng.uni());
                popZ[c] *= 0.996f; C.popLp = 0.8f * C.popLp + 0.2f * popZ[c];
                C.hissHp = 0.6f * C.hissHp + 0.4f * w; const float hs = (w - C.hissHp) * hiss;
                io[c][i] += (s + ckF + C.popLp + hs) * gate + hm * gate;
            }
        }
    }

    void updateFilters (const Settings& S)
    {
        if (S.fHp == fk[0] && S.fLp == fk[1] && S.fRes == fk[2] && S.fBoom == fk[3] && S.fCrack == fk[4] && S.fWarm == fk[5] && S.fAir == fk[6]) return;
        fk[0] = S.fHp; fk[1] = S.fLp; fk[2] = S.fRes; fk[3] = S.fBoom; fk[4] = S.fCrack; fk[5] = S.fWarm; fk[6] = S.fAir;
        wLpA = (float) (1.0 - std::exp (-2 * kPi * 100.0 / sr4));
        for (auto& C : ch)
        {
            C.hp1.set (S.fHp, S.fRes * 0.6f, sr4); C.hp2.set (S.fHp, 0.0f, sr4);
            C.lp1.set (S.fLp, S.fRes, sr4); C.lp2.set (S.fLp, 0.0f, sr4);
            C.boom.bell (70, 0.9, S.fBoom, sr4); C.crack.bell (2600, 1.0, S.fCrack, sr4);
            C.wLo.lowShelf (85, S.fWarm * 4.5, sr4); C.wDip.bell (320, 0.8, -S.fWarm * 2.5, sr4); C.wHi.highShelf (9000, -S.fWarm * 3.0, sr4);
            C.air.highShelf (11000, S.fAir * 7.0, sr4);
        }
    }
    struct Ch
    {
        Biquad aa[3], post[2], tiltLo, tiltHi, boom, crack, bump, tLp1, wLo, wDip, wHi, air, xo[4]; Svf hp1, hp2, lp1, lp2; float wLp = 0, wDc = 0, muDc = 0;
        float pre = 0, held = 0, pink = 0, popLp = 0, hissHp = 0;
    };
    double sr = 44100, sr4 = 44100 * 8, wowPh = 0, flPh = 0, fl2Ph = 0, humPh = 0, ottSr = -1;
    int maxBlock = 512, osLat = 0, lastModel = -1, dropT = 0, lastDModel = -1;
    float holdPh = 1, lastRate = -1, driveMk = 1, mkFor = -100, env = 0, rms = 0, envF = 0, envS = 0, inRms = 1e-6f, outRms = 1e-6f, dropG = 1, wowTarget = 0, wowDrift = 0, noiseEnv = 0, muCharge = 0, lastAge = -1;
    float wA_s = 0, fA_s = 0, f2A_s = 0;
    float fk[7] { -1, -1, -1, -1, -1, -1, -1 }, crackLp[2] {}, popZ[2] {}, wLpA = 0.01f, ottEnv[3] {};
    Sm1 smv[12], fs[7];
    std::unique_ptr<juce::dsp::Oversampling<float>> os;
    Dl dryDl[2], tapeDl[2], erDl[2], fdn[4], bypDl[2]; int upTail = 0; float fdnLp[4] {}, wetHp[2] {}, roomEnv = 0, roomHold = 0, roomGate = 0;
    Ch ch[2]; Rng rng;
    juce::AudioBuffer<float> satDry;
};
}
