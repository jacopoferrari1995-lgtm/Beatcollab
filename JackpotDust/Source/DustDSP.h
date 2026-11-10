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
enum DModel { D_TAPE, D_TUBE, D_CONSOLE, D_PUSH, kDModels };
inline const char* dName (int m) { static const char* n[] = { "NASTRO", "VALVOLA", "CONSOLE", "SPINTA" }; return n[juce::jlimit (0, kDModels - 1, m)]; }
inline const char* dDesc (int m)
{
    static const char* d[] = { "Compressione morbida e simmetrica: arrotonda i picchi, ingrossa il corpo.",
                               "Asimmetrica, armoniche pari: calda, gonfia la cassa e ammorbidisce i piatti.",
                               "Trasformatore da banco: un filo di peso in basso e saturazione presente sulle medie.",
                               "Clip deciso ma con ginocchio: transienti spezzati, rullante che spacca." };
    return d[juce::jlimit (0, kDModels - 1, m)];
}
enum CStyle { C_FET, C_BUS, C_PUMP, kCStyles };
inline const char* cName (int m) { static const char* n[] = { "FET", "BUS", "PUMP" }; return n[juce::jlimit (0, kCStyles - 1, m)]; }

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
    bool fOn = false; float fHp = 20, fLp = 20000, fRes = 0, fBoom = 0, fCrack = 0; bool f24 = false;
    bool tOn = false; float tAge = 0.4f, tWow = 0.2f, tFlut = 0.2f, tDrop = 0;
    bool nOn = false; float nVinyl = 0.4f, nPops = 0.3f, nHiss = 0.2f, nHum = 0; bool nFollow = true;
    float amount = 1, mix = 1, outDb = 0; bool mono = false, autoGain = false;
};

// ---------------------------------------------------------------- motore
class Engine
{
public:
    void prepare (double s, int block)
    {
        sr = s; maxBlock = block;
        os = std::make_unique<juce::dsp::Oversampling<float>> (2, 2, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, true);
        os->initProcessing ((size_t) block); osLat = (int) std::round (os->getLatencyInSamples());
        dryDl[0].prepare (osLat + block + 8); dryDl[1].prepare (osLat + block + 8);
        for (auto& t : tapeDl) t.prepare ((int) (sr * 0.03));
        dry.setSize (2, block); tmp.setSize (2, block);
        for (auto& c : ch) c = Ch();
        holdPh = 1; lastDrive = -100; env = 0; inRms = outRms = 1e-6f; gr = 0; dropG = 1; dropT = 0; wowPh = flPh = fl2Ph = 0;
        humPh = 0; noiseEnv = 0; driveMk = 1; crackLp[0] = crackLp[1] = popZ[0] = popZ[1] = 0;
        updateFilters (Settings(), true);
    }
    int latency() const { return osLat; }

    void process (float* L, float* R, int n, const Settings& S)
    {
        const float A = S.amount;
        const float inG = dbToG (S.inDb);
        for (int i = 0; i < n; ++i) { L[i] *= inG; R[i] *= inG; }
        // copia asciutta per MIX finale (ritardata della latenza dell'oversampling)
        for (int i = 0; i < n; ++i) { dryDl[0].push (L[i]); dryDl[1].push (R[i]); }
        float* io[2] = { L, R };
        double ein = 0; for (int i = 0; i < n; ++i) ein += 0.5 * (L[i] * L[i] + R[i] * R[i]);

        // 1) campionatore d'epoca
        if (S.sOn && S.sMix * std::min (1.0f, A) > 0.001f)
        {
            const auto& M = sInfo (S.sModel);
            const float rate = std::min (S.sRate, (float) sr), mixS = std::min (1.0f, S.sMix * A);
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
                float x[2] = { L[i], R[i] };
                for (int c = 0; c < 2; ++c)
                {
                    float v = x[c];
                    for (int k = 0; k < M.aaOrder / 2; ++k) v = ch[c].aa[k].run (v);
                    ch[c].pre = v;
                }
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
                for (int c = 0; c < 2; ++c)
                {
                    float v = ch[c].held;
                    if (M.postLp > 0) v = ch[c].post[1].run (ch[c].post[0].run (v));
                    io[c][i] = x[c] + (v - x[c]) * mixS;
                }
            }
        }
        // 2) saturazione (4x)
        if (S.dOn && S.dDrive * A > 0.05f)
        {
            const float drive = S.dDrive * A;
            if (drive != lastDrive || S.dModel != lastDModel)
            {   // compenso il volume: una sinusoide a -6 dBFS (livello tipico dei colpi) esce allo stesso livello
                lastDrive = drive; lastDModel = S.dModel; const float g = dbToG (drive); double e = 0;
                for (int k = 0; k < 64; ++k) { const float y = shapeFn (S.dModel, g * 0.5f * std::sin (2 * (float) kPi * k / 64)); e += y * y; }
                driveMk = 0.5f / (float) std::sqrt (e / 64 + 1e-12) * 0.7071f;
            }
            for (int c = 0; c < 2; ++c) { ch[c].tiltLo.lowShelf (250, -S.dTone * 4.0, sr); ch[c].tiltHi.lowShelf (250, S.dTone * 4.0, sr); }
            juce::dsp::AudioBlock<float> blk (io, 2, (size_t) n);
            for (int c = 0; c < 2; ++c) for (int i = 0; i < n; ++i) { tmp.setSample (c, i, io[c][i]); io[c][i] = ch[c].tiltLo.run (io[c][i]); }
            auto up = os->processSamplesUp (blk);
            const float g = dbToG (drive);
            for (size_t c = 0; c < up.getNumChannels(); ++c) { auto* d = up.getChannelPointer (c); for (size_t i = 0; i < up.getNumSamples(); ++i) d[i] = shapeFn (S.dModel, d[i] * g) * driveMk; }
            os->processSamplesDown (blk);
            // il mix della saturazione usa il segnale ritardato della stessa latenza
            for (int c = 0; c < 2; ++c) for (int i = 0; i < n; ++i)
            {
                ch[c].satDl.push (tmp.getSample (c, i));
                const float d = ch[c].satDl.read ((float) osLat), w = ch[c].tiltHi.run (io[c][i]);
                io[c][i] = d + (w - d) * S.dMix;
            }
        }
        else if (osLat > 0)
        {   // latenza costante anche a modulo spento
            for (int c = 0; c < 2; ++c) for (int i = 0; i < n; ++i) { ch[c].satDl.push (io[c][i]); io[c][i] = ch[c].satDl.read ((float) osLat); }
        }
        // 3) filtri ed EQ da batteria
        if (S.fOn)
        {
            updateFilters (S, false);
            for (int c = 0; c < 2; ++c)
            {
                auto& C = ch[c];
                for (int i = 0; i < n; ++i)
                {
                    float v = io[c][i], lp, hp;
                    if (S.fHp > 21) { C.hp1.tick (v, lp, hp); v = hp; if (S.f24) { C.hp2.tick (v, lp, hp); v = hp; } }
                    if (S.fLp < 19900) { C.lp1.tick (v, lp, hp); v = lp; if (S.f24) { C.lp2.tick (v, lp, hp); v = lp; } }
                    if (std::abs (S.fBoom) > 0.05f) v = C.boom.run (v);
                    if (std::abs (S.fCrack) > 0.05f) v = C.crack.run (v);
                    io[c][i] = v;
                }
            }
        }
        // 4) compressore e punch
        if (S.cOn)
        {
            const float amt = std::min (1.0f, S.cAmt * A);
            const int st = S.cStyle;
            const float thr = st == C_FET ? -amt * 30 : st == C_BUS ? -amt * 24 : -amt * 36;
            const float ratio = st == C_FET ? 4 + amt * 16 : st == C_BUS ? 2 + 2 * amt : 10;
            const float knee = st == C_BUS ? 10.0f : 6.0f;
            const float att = (st == C_PUMP ? std::max (S.cAtt, 15.0f) : S.cAtt) * 0.001f, rel = (st == C_PUMP ? S.cRel * 2 : S.cRel) * 0.001f;
            const float aA = std::exp (-1.0f / (att * (float) sr)), aR = std::exp (-1.0f / (rel * (float) sr)), aRms = std::exp (-1.0f / (0.01f * (float) sr));
            // guadagno di compensazione: il 70% della riduzione che avrebbe un segnale a -10 dBFS
            float makeup = 0; { const float ov = -10.0f - thr; if (ov > knee / 2) makeup = ov * (1 - 1 / ratio); else if (ov > -knee / 2) { const float z = ov + knee / 2; makeup = (1 - 1 / ratio) * z * z / (2 * knee); } makeup *= 0.7f; }
            const float pA1 = std::exp (-1.0f / (0.0008f * (float) sr)), pR1 = std::exp (-1.0f / (0.04f * (float) sr)), pA2 = std::exp (-1.0f / (0.02f * (float) sr)), pR2 = std::exp (-1.0f / (0.15f * (float) sr));
            float maxGr = 0;
            for (int i = 0; i < n; ++i)
            {
                const float l = io[0][i], r = io[1][i], a = std::max (std::abs (l), std::abs (r));
                // punch: differenza tra inviluppo veloce e lento
                float pg = 1;
                if (std::abs (S.cPunch) > 0.01f)
                {
                    envF = a > envF ? pA1 * envF + (1 - pA1) * a : pR1 * envF + (1 - pR1) * a;
                    envS = a > envS ? pA2 * envS + (1 - pA2) * a : pR2 * envS + (1 - pR2) * a;
                    const float t = juce::jlimit (0.0f, 1.0f, (envF - envS) / (envS + 1e-4f));
                    pg = dbToG (S.cPunch * (S.cPunch > 0 ? 8.0f : 10.0f) * t);
                }
                float det;
                if (st == C_FET) det = gToDb (a);
                else { rms = aRms * rms + (1 - aRms) * 0.5f * (l * l + r * r); det = 10.0f * std::log10 (rms + 1e-12f); }
                const float over = det - thr;
                float want = 0;
                if (over > knee / 2) want = over * (1 - 1 / ratio);
                else if (over > -knee / 2) { const float z = over + knee / 2; want = (1 - 1 / ratio) * z * z / (2 * knee); }
                env = want > env ? aA * env + (1 - aA) * want : aR * env + (1 - aR) * want;
                maxGr = std::max (maxGr, env);
                const float gg = dbToG (makeup - env) * pg;
                float yl = l * gg, yr = r * gg;
                if (st == C_FET && amt > 0.5f) { const float k = 1 + (amt - 0.5f) * 1.6f; yl = std::tanh (yl * k) / k; yr = std::tanh (yr * k) / k; }   // tutti i bottoni: un po' di sporco
                io[0][i] = l + (yl - l) * S.cMix; io[1][i] = r + (yr - r) * S.cMix;
            }
            gr = maxGr;
        }
        else gr = 0;
        // 5) nastro: wow, flutter, eta, dropout
        if (S.tOn)
        {
            const float age = std::min (1.5f, S.tAge * A), wow = S.tWow * A, flut = S.tFlut * A, drop = S.tDrop * A;
            for (int c = 0; c < 2; ++c) { ch[c].bump.bell (85, 0.8, age * 2.5, sr); ch[c].tLp1.lp (20000 * (1 - 0.72f * std::min (1.0f, age)), 0.6, sr); }
            const float wA = (float) (0.0087 * wow / (2 * kPi * 0.55) * sr), fA = (float) (0.0025 * flut / (2 * kPi * 6.3) * sr), f2A = (float) (0.001 * flut / (2 * kPi * 11.0) * sr);
            const float sat = 1 + age * 1.5f;
            for (int i = 0; i < n; ++i)
            {
                wowPh += 0.55 / sr; flPh += 6.3 / sr; fl2Ph += 11.0 / sr;
                if (wowPh > 1) { wowPh -= 1; wowTarget = rng.next() * 0.4f; }
                wowDrift += 0.0002f * (wowTarget - wowDrift);
                const float d = 2 + wA * (1 + (float) std::sin (2 * kPi * wowPh) + wowDrift) + fA * (1 + (float) std::sin (2 * kPi * flPh)) + f2A * (1 + (float) std::sin (2 * kPi * fl2Ph));
                // dropout: brevi cali di livello casuali
                if (drop > 0.001f)
                {
                    if (dropT <= 0 && rng.uni() < drop * 1.5f / (float) sr) dropT = (int) (sr * (0.02 + 0.1 * rng.uni()));
                    const float tgt = dropT > 0 ? 0.35f + 0.3f * rng.uni() * 0.2f : 1.0f; if (dropT > 0) --dropT;
                    dropG += (tgt - dropG) * 0.004f;
                }
                else dropG = 1;
                for (int c = 0; c < 2; ++c)
                {
                    auto& C = ch[c];
                    tapeDl[c].push (io[c][i]);
                    float v = (wow + flut > 0.001f) ? tapeDl[c].read (d + (c ? 0.3f : 0.0f)) : io[c][i];
                    if (age > 0.001f) { v = std::tanh (v * sat) / sat * (1 + age * 0.15f); v = C.tLp1.run (C.bump.run (v)); }
                    io[c][i] = v * dropG;
                }
            }
        }
        // 6) polvere: vinile, crackle, pop, hiss, ronzio
        if (S.nOn && A > 0.001f)
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
                    // fruscio di superficie (rosa scuro) + crackle (impulsi filtrati) + pop rari
                    const float w = rng.next();
                    C.pink = 0.985f * C.pink + 0.015f * w; float s = C.pink * 4.0f * surf;
                    float ck = 0;
                    if (rng.uni() < crackRate / (float) sr) { const float u = rng.uni(); ck = (rng.next() > 0 ? 1.0f : -1.0f) * u * u * u * cLevel; }
                    crackLp[c] = 0.55f * crackLp[c] + 0.45f * ck; const float ckF = ck - crackLp[c] * 0.6f;
                    float pp = 0;
                    if (rng.uni() < popRate / (float) sr * 0.5f) popZ[c] = (rng.next() > 0 ? 1.0f : -1.0f) * pLevel * (0.5f + 0.5f * rng.uni());
                    popZ[c] *= 0.996f; C.popLp = 0.8f * C.popLp + 0.2f * popZ[c]; pp = C.popLp;
                    C.hissHp = 0.6f * C.hissHp + 0.4f * w; const float hs = (w - C.hissHp) * hiss;
                    io[c][i] += (s + ckF + pp + hs) * gate + hm * gate;
                }
            }
        }
        // 7) uscita: mix parallelo, mono, guadagno automatico, volume
        const float og = dbToG (S.outDb);
        double eout = 0;
        for (int i = 0; i < n; ++i)
        {
            float l = io[0][i], r = io[1][i];
            if (S.mix < 0.999f) { const float t = (float) (osLat + n - 1 - i), dl = dryDl[0].read (t), dr = dryDl[1].read (t); l = dl + (l - dl) * S.mix; r = dr + (r - dr) * S.mix; }
            if (S.mono) { const float m = 0.5f * (l + r); l = r = m; }
            eout += 0.5 * (l * l + r * r);
            io[0][i] = l; io[1][i] = r;
        }
        const float k = std::exp (-(float) n / (float) (sr * 1.5));
        inRms = k * inRms + (1 - k) * (float) (ein / n); outRms = k * outRms + (1 - k) * (float) (eout / n);
        float ag = 1;
        if (S.autoGain && inRms > 1e-7f) ag = juce::jlimit (0.25f, 4.0f, std::sqrt (inRms / std::max (outRms, 1e-9f)));
        agS += (ag - agS) * 0.05f;
        // protezione: sopra -1 dBFS un clip morbido (sotto e trasparente)
        auto safe = [] (float x) { const float a = std::abs (x); return a <= 0.9f ? x : std::copysign (0.9f + 0.1f * std::tanh ((a - 0.9f) / 0.1f), x); };
        for (int i = 0; i < n; ++i) { io[0][i] = safe (io[0][i] * og * (S.autoGain ? agS : 1.0f)); io[1][i] = safe (io[1][i] * og * (S.autoGain ? agS : 1.0f)); }
    }

    std::atomic<float> gr { 0 };
    float agS = 1;
private:
    void updateFilters (const Settings& S, bool force)
    {
        if (! force && S.fHp == fk[0] && S.fLp == fk[1] && S.fRes == fk[2] && S.fBoom == fk[3] && S.fCrack == fk[4]) return;
        fk[0] = S.fHp; fk[1] = S.fLp; fk[2] = S.fRes; fk[3] = S.fBoom; fk[4] = S.fCrack;
        for (auto& C : ch)
        {
            C.hp1.set (S.fHp, S.fRes * 0.6f, sr); C.hp2.set (S.fHp, 0.0f, sr);
            C.lp1.set (S.fLp, S.fRes, sr); C.lp2.set (S.fLp, 0.0f, sr);
            C.boom.bell (70, 0.9, S.fBoom, sr); C.crack.bell (2600, 1.0, S.fCrack, sr);
        }
    }
    struct Ch
    {
        Biquad aa[3], post[2], tiltLo, tiltHi, boom, crack, bump, tLp1; Svf hp1, hp2, lp1, lp2;
        float pre = 0, held = 0, pink = 0, popLp = 0, hissHp = 0; Dl satDl;
        Ch() { satDl.prepare (2048); }
    };
    double sr = 44100, wowPh = 0, flPh = 0, fl2Ph = 0, humPh = 0;
    int maxBlock = 512, osLat = 0, lastModel = -1, dropT = 0, lastDModel = -1;
    float holdPh = 1, lastRate = -1, lastDrive = -100, driveMk = 1, env = 0, rms = 0, envF = 0, envS = 0, inRms = 1e-6f, outRms = 1e-6f, dropG = 1, wowTarget = 0, wowDrift = 0, noiseEnv = 0;
    float fk[5] { -1, -1, -1, -1, -1 }, crackLp[2] {}, popZ[2] {};
    std::unique_ptr<juce::dsp::Oversampling<float>> os;
    Dl dryDl[2], tapeDl[2]; Ch ch[2]; Rng rng;
    juce::AudioBuffer<float> dry, tmp;
};
}
