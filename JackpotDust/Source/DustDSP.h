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
        case D_CLIP:
        {   // clipper con un ginocchio corto (derivata continua): taglia come un clip duro ma con molte meno armoniche alte da ripiegare
            const float k = 0.25f, ax = std::abs (x);
            if (ax <= 1 - k) return x;
            if (ax >= 1 + k) return std::copysign (1.0f, x);
            const float d = ax - (1 - k); return std::copysign (ax - d * d / (4 * k), x);
        }
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
    float amount = 1, mix = 1, outDb = 0; bool mono = false, autoGain = false, cal = true;
};

// ---------------------------------------------------------------- motore
// Ordine: sampler (1x, l'aliasing delle macchine e' voluto) -> filtri/EQ -> [8x: saturazione, compressore, nastro] -> trasporto nastro -> room -> polvere -> uscita.
// Le manopole che cambiano livello o colore sono smussate per campione (niente click), i filtri si aggiornano a piccoli passi.
// cancello dell'oversampling: quando nessun modulo a 8x lavora, il segnale passa solo da un ritardo uguale (neutro al bit);
// quando uno si accende l'8x si scalda per la sua latenza e poi entra con una dissolvenza
struct OsGate
{
    std::vector<float> ring[2], tmp[2]; int w = 0, L = 1, warm = 0; float m = 0; bool running = false;
    void prepare (int lat, int maxBlock) { L = std::max (1, lat); for (int c = 0; c < 2; ++c) { ring[c].assign ((size_t) L, 0.0f); tmp[c].assign ((size_t) maxBlock, 0.0f); } w = 0; m = 0; warm = 0; running = false; }
    void mix (float* const* ch, int N, bool act)
    {
        const float step = 1.0f / 1024.0f;
        for (int i = 0; i < N; ++i)
        {
            if (warm > 0) --warm; else m = act ? std::min (1.0f, m + step) : std::max (0.0f, m - step);
            for (int c = 0; c < 2; ++c)
            {
                const float in = running ? tmp[c][(size_t) i] : ch[c][i], d = ring[c][(size_t) w]; ring[c][(size_t) w] = in;
                ch[c][i] = running ? d + (ch[c][i] - d) * m : d;
            }
            if (++w >= L) w = 0;
        }
        if (! act && m <= 0.0f && warm == 0) running = false;
    }
};
struct Sm1 { float v = 0; bool init = false; inline float step (float t, float k) { if (! init) { v = t; init = true; } v += (t - v) * k; return v; } };

class Engine
{
public:
    static constexpr int kOsSt = 3, kOs = 8;
    void prepare (double s, int block)
    {
        sr = s; maxBlock = block; sr4 = s * kOs;
        os = std::make_unique<juce::dsp::Oversampling<float>> (2, kOsSt, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, false, true);
        os->initProcessing ((size_t) block); osLat = (int) std::round (os->getLatencyInSamples()); gUp.prepare (osLat, block);
        tapeD0 = (int) std::ceil ((0.0087 * 1.5 / (2 * kPi * 0.55) * 1.2 + 0.0025 * 1.5 / (2 * kPi * 6.3) + 0.001 * 1.5 / (2 * kPi * 11.0)) * sr) + 4;
        dryDl[0].prepare (osLat + tapeD0 + block + 8); dryDl[1].prepare (osLat + tapeD0 + block + 8); bypDl[0].prepare (osLat + 8); bypDl[1].prepare (osLat + 8); upTail = 0;
        for (auto& t : tapeDl) t.prepare (2 * tapeD0 + 16);
        satDry.setSize (2, block * kOs);
        for (auto& c : ch) c = Ch();
        holdPh = 1; env = 0; inRms = outRms = 1e-6f; gr = 0; dropG = 1; dropT = 0; wowPh = flPh = fl2Ph = 0;
        humPh = 0; noiseEnv = 0; crackLp[0] = crackLp[1] = popZ[0] = popZ[1] = 0; rms = envF = envS = 0; muCharge = 0; grAvgC = 0; grNum = grDen = 0; tapeW = 0; filtW = 0; cCur = -1; sCur = -1;
        for (auto& e : ottEnv) e = 0;
        for (auto& v : smv) v = Sm1();
        lastRate = -1; lastModel = -1; for (auto& f : fk) f = -1;
        // room
        for (int c = 0; c < 2; ++c) erDl[c].prepare ((int) (sr * 0.12));
        for (int k = 0; k < 4; ++k) { fdn[k].prepare ((int) (sr * 0.2)); fdnLp[k] = 0; }
        roomEnv = 0; roomHold = 0; roomGate = 0; wetHp[0] = wetHp[1] = 0;
        ottSr = -1;
        gRing.assign ((size_t) (latency() + block + 16), 1.0f); gRw = 0; calRms = 0; calLt = -100; calDb = 0;
    }
    // il nastro ha un ritardo fisso (centro del wow e del flutter) che c'e' sempre, anche a nastro spento: la latenza dichiarata e' quella vera
    int latency() const { return osLat + tapeD0; }

    void process (float* L, float* R, int n, const Settings& S)
    {
        const float A = S.amount;
        const float kS = 1.0f - std::exp (-1.0f / (0.015f * (float) sr)), kS4 = 1.0f - std::exp (-1.0f / (0.015f * (float) sr4));
        for (int i = 0; i < n; ++i) { const float g = dbToG (smv[0].step (S.inDb, kS)); L[i] *= g; R[i] *= g; }
        // copia asciutta per il MIX generale (ritardata della latenza dell'oversampling)
        for (int i = 0; i < n; ++i) { dryDl[0].push (L[i]); dryDl[1].push (R[i]); }
        float* io[2] = { L, R };
        // LIVELLO AUTO: la catena lavora sempre allo stesso livello (parti forti della batteria a circa -14 dB RMS), qualsiasi sia
        // il volume del campione; all'uscita il guadagno si toglie. Cosi' lo stesso preset spinge uguale su una cassa piano e su un bus forte
        {
            const bool active = S.sOn || S.dOn || S.cOn || S.fOn || S.tOn || S.nOn || S.rOn;
            const float a300 = std::exp (-1.0f / (0.3f * (float) sr)), dec = 3.0f / (float) sr;
            const float up = 1.0f - std::exp (-1.0f / (2.0f * (float) sr)), down = 1.0f - std::exp (-1.0f / (0.05f * (float) sr));
            const int GL = (int) gRing.size();
            for (int i = 0; i < n; ++i)
            {
                calRms = a300 * calRms + (1 - a300) * 0.5f * (L[i] * L[i] + R[i] * R[i]);
                const float db = 10.0f * std::log10 (calRms + 1e-12f);
                if (db > -60) calLt = std::max (db, calLt - dec);
                const float tgt = (S.cal && active && calLt > -60) ? juce::jlimit (-12.0f, 18.0f, -14.0f - calLt) : 0.0f;
                calDb += (tgt - calDb) * (tgt > calDb ? up : down);
                if (std::abs (calDb) < 1e-4f && tgt == 0.0f) calDb = 0;
                const float g = calDb == 0.0f ? 1.0f : dbToG (calDb);
                L[i] *= g; R[i] *= g; gRing[(size_t) gRw] = g; if (++gRw >= GL) gRw = 0;
            }
        }
        double ein = 0; for (int i = 0; i < n; ++i) ein += 0.5 * (L[i] * L[i] + R[i] * R[i]);

        // 1) campionatore d'epoca (a 1x: e' la macchina vera, aliasing compreso)
        // cambio macchina: il campionatore esce, cambia, rientra (prima a scatto)
        if (sCur != S.sModel && smv[1].v <= 0.0005f) { sCur = S.sModel; lastModel = -1; for (auto& C : ch) { for (auto& b : C.aa) b.reset(); for (auto& b : C.post) b.reset(); } }
        const float sMixT = S.sOn && sCur == S.sModel ? std::min (1.0f, S.sMix * A) : 0.0f;
        if (S.sOn || smv[1].v > 0.0005f)
        {
            const auto& M = sInfo (sCur);
            const float rate = std::min (S.sRate, (float) sr);
            if (rate != lastRate || sCur != lastModel)
            {
                lastRate = rate; lastModel = sCur;
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

        // filtri ed EQ prima della saturazione e del compressore: boom e crack spingono dentro la macchina (piu' grosso),
        // e il clipper resta l'ultimo a toccare i picchi (prima l'EQ dopo il clip rialzava i picchi tagliati)
        processFilters (io, n, S);
        // 2-5) a 8x: niente aliasing da saturazione, clip, compressore e nastro
        // l'oversampling gira sempre (latenza costante, nessun salto quando accendi un modulo); i moduli spenti non costano nulla
        {   // solo se un modulo a 8x lavora; altrimenti un ritardo uguale (cosi' a moduli spenti il plugin non cambia il suono)
            const bool upAct = S.dOn || smv[5].v > 0.0005f || S.cOn || smv[7].v > 0.0005f || (S.tOn && S.tAge * A > 0.001f) || smv[11].v > 0.001f;
            if (upAct && ! gUp.running) { os->reset(); gUp.running = true; gUp.warm = 2 * osLat + 8; }
            if (gUp.running)
            {
                for (int c = 0; c < 2; ++c) std::copy (io[c], io[c] + n, gUp.tmp[c].begin());
                juce::dsp::AudioBlock<float> blk (io, 2, (size_t) n);
                auto up = os->processSamplesUp (blk);
                const int N = (int) up.getNumSamples();
                float* u[2] = { up.getChannelPointer (0), up.getChannelPointer (1) };
                processUp (u, N, S, A, kS4);
                os->processSamplesDown (blk);
            }
            gUp.mix (io, n, upAct);
        }
        // a 1x: il trasporto del nastro
        if (S.tOn || tapeW > 0.0f) processTape (io, n, S, A);   // acceso/spento con una dissolvenza di 20 ms (prima a scatto)
        else for (int c = 0; c < 2; ++c) for (int i = 0; i < n; ++i) { tapeDl[c].push (io[c][i]); io[c][i] = tapeDl[c].read ((float) tapeD0); }   // solo il ritardo fisso

        // 6) room: piccola stanza per batterie grandi (prime riflessioni + coda corta), con gate opzionale
        processRoom (io, n, S, kS);
        // 7) polvere: vinile, crackle, pop, hiss, ronzio
        if (S.nOn && A > 0.001f) processDust (io, n, S, A);
        // 8) uscita: mix parallelo, mono, guadagno automatico, volume
        double eout = 0;
        const int GL = (int) gRing.size(), lat = latency();
        for (int i = 0; i < n; ++i)
        {
            float l = io[0][i], r = io[1][i];
            {   // toglie il LIVELLO AUTO con lo stesso guadagno che aveva questo campione all'ingresso
                int k = gRw - n + i - lat; while (k < 0) k += GL; const float g = gRing[(size_t) k];
                if (g != 1.0f) { l /= g; r /= g; }
            }
            const float mx = smv[2].step (S.mix, kS);
            if (mx < 0.9995f) { const float t = (float) (osLat + tapeD0 + n - 1 - i), dl = dryDl[0].read (t), dr = dryDl[1].read (t); l = dl + (l - dl) * mx; r = dr + (r - dr) * mx; }
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
        const float driveT = S.dDrive * A, dMixT = S.dOn ? S.dMix : 0.0f;   // spegnendo esce solo il MIX: la spinta (e il suo makeup) restano fermi, niente salto di volume
        if (S.dOn || smv[5].v > 0.0005f)
        {
            if (S.dModel != lastDModel || std::abs (driveT - mkFor) > 0.25f) { lastDModel = S.dModel; mkFor = driveT; driveMk = makeup (S.dModel, driveT); }
            if (S.dTone != lastTone) { lastTone = S.dTone; for (int c = 0; c < 2; ++c) { ch[c].tiltLo.lowShelf (250, -S.dTone * 4.0, sr4); ch[c].tiltHi.lowShelf (250, S.dTone * 4.0, sr4); } }
            const bool tilt = std::abs (S.dTone) > 0.01f;
            for (int i = 0; i < n; ++i)
            {
                const float dr = smv[6].step (driveT, kS), mx = smv[5].step (dMixT, kS * 0.4f), g = dbToG (dr) * 1.0f;   // entrata della saturazione in ~40 ms
                for (int c = 0; c < 2; ++c)
                {
                    const float x = io[c][i];
                    float y = shapeFn (S.dModel, (tilt ? ch[c].tiltLo.run (x) : x) * g) * driveMk;
                    if (tilt) y = ch[c].tiltHi.run (y);
                    io[c][i] = x + (y - x) * mx;
                }
            }
        }
        else { smv[5].step (0, 1); smv[6].step (driveT, 1); }
        // compressore e punch
        if (S.cOn || smv[7].v > 0.0005f)
        {
            // cambio tipo: il compressore esce (mix a zero), cambia, e rientra
            if (cCur != S.cStyle && smv[7].v < 0.001f) { cCur = S.cStyle; env = rms = envF = envS = muCharge = 0; grNum = grDen = grAvgC = 0; compG = 1; for (auto& e : ottEnv) e = 0; for (auto& g : ottG) g = 1; }
            Settings C2 = S; C2.cStyle = cCur; if (cCur != S.cStyle) C2.cOn = false;
            if (cCur == C_OTT) processOtt (io, n, C2, A, kS);
            else processComp (io, n, C2, A, kS);
        }
        else { gr = 0; smv[7].step (0, 1); }
        // saturazione del nastro (il resto del nastro, che e' lineare, gira a 1x)
        const float ageT = S.tOn ? std::min (1.5f, S.tAge * A) : 0.0f;
        if (ageT > 0.001f || smv[11].v > 0.001f)
            for (int i = 0; i < n; ++i)
            {
                const float age = smv[11].step (ageT, kS), sat = 1 + age * 1.5f, k = (1 + age * 0.15f) / sat;
                io[0][i] = std::tanh (io[0][i] * sat) * k; io[1][i] = std::tanh (io[1][i] * sat) * k;
            }
    }

    static float makeup (int model, float drive)
    {   // compenso il volume: una sinusoide a -6 dBFS (livello tipico dei colpi) esce allo stesso livello
        const float g = dbToG (drive); double e = 0;
        for (int k = 0; k < 64; ++k) { const float y = shapeFn (model, g * 0.5f * std::sin (2 * (float) kPi * k / 64)); e += y * y; }
        return 0.5f / (float) std::sqrt (e / 64 + 1e-12) * 0.7071f;
    }

    // filtri ed EQ da batteria a 1x (sono lineari: a 8x costavano otto volte tanto senza suonare diversi)
    void processFilters (float** io, int n, const Settings& S)
    {
        if (! S.fOn && filtW <= 0.0f) return;
        const float fStep = 1.0f / (0.02f * (float) sr);
        for (int o = 0; o < n; o += 32)
        {
            const int m = std::min (32, n - o);
            Settings F = S; const float kb = 1.0f - std::exp (-(float) m / (0.02f * (float) sr));
            F.fHp = fs[0].step (S.fHp, kb); F.fLp = fs[1].step (S.fLp, kb); F.fBoom = fs[2].step (S.fBoom, kb); F.fCrack = fs[3].step (S.fCrack, kb);
            F.fWarm = fs[4].step (S.fWarm, kb); F.fAir = fs[5].step (S.fAir, kb); F.fRes = fs[6].step (S.fRes, kb);
            updateFilters (F);
            for (int c = 0; c < 2; ++c)
            {
                auto& C = ch[c];
                for (int i = o; i < o + m; ++i)
                {
                    float v = io[c][i], lp, hp;
                    // i filtri girano sempre (stato caldo): quando entrano non c'e' il gradino di un filtro che parte da zero;
                    // il secondo stadio (24 dB) entra sfumando
                    if (c == 0) { w24 = S.f24 ? std::min (1.0f, w24 + fStep) : std::max (0.0f, w24 - fStep); w24a[i - o] = w24; }
                    const float k24 = w24a[i - o];
                    { float h1, h2, l2; C.hp1.tick (v, lp, h1); C.hp2.tick (h1, l2, h2); if (F.fHp > 21) v = h1 + (h2 - h1) * k24; }
                    { float l1, l2, hh; C.lp1.tick (v, l1, hh); C.lp2.tick (l1, l2, hh); if (F.fLp < 19900) v = l1 + (l2 - l1) * k24; }
                    if (std::abs (F.fBoom) > 0.02f) v = C.boom.run (v);
                    if (std::abs (F.fCrack) > 0.02f) v = C.crack.run (v);
                    if (F.fWarm > 0.001f)
                    {   // EQ caldo: bassi col trucco del Pultec (boost a 85 + dip a 320), alti arrotondati, trasformatore sui bassi
                        v = C.wLo.run (v); v = C.wDip.run (v); v = C.wHi.run (v);
                        const float w = F.fWarm;
                        C.wLp += (v - C.wLp) * wLpA; const float lo = C.wLp, kd = 1.0f + w * 2.5f;
                        v += std::tanh (lo * kd) / kd - lo;
                        C.wDc += (v - C.wDc) * 0.00005f; v -= C.wDc;
                    }
                    if (F.fAir > 0.001f) v = C.air.run (v);
                    const float w = c == 0 ? (filtW = S.fOn ? std::min (1.0f, filtW + fStep) : std::max (0.0f, filtW - fStep)) : fW1[i - o];
                    if (c == 0) fW1[i - o] = w;
                    io[c][i] = w >= 1.0f ? v : io[c][i] + (v - io[c][i]) * w;   // filtri accesi/spenti sfumati
                }
            }
        }
    }

    // compressore: il segnale passa a 8x, ma il calcolo del guadagno (logaritmi ed esponenziali) si fa una volta ogni 8 campioni
    // e si interpola: l'inviluppo e' lento rispetto al campione, il suono non cambia e la CPU scende di molto
    void processComp (float** io, int n, const Settings& S, float A, float kS)
    {
        const int st = S.cStyle; const float sc = (float) sr;   // tempi alla frequenza di controllo (1x)
        const float aRms = std::exp (-1.0f / (0.01f * sc));
        const float pA1 = std::exp (-1.0f / (0.0008f * sc)), pR1 = std::exp (-1.0f / (0.04f * sc)), pA2 = std::exp (-1.0f / (0.02f * sc)), pR2 = std::exp (-1.0f / (0.15f * sc));
        const float att = (st == C_PUMP ? std::max (S.cAtt, 15.0f) : st == C_670 ? std::max (S.cAtt * 0.5f, 0.4f) : S.cAtt) * 0.001f, rel = (st == C_PUMP ? S.cRel * 2 : S.cRel) * 0.001f;
        const float aA = std::exp (-1.0f / (att * sc)), aR = std::exp (-1.0f / (rel * sc));
        const float kG = 1.0f - std::pow (1.0f - kS, (float) kOs);
        float maxGr = 0;
        for (int i0 = 0; i0 < n; i0 += kOs)
        {
            const int m = std::min (kOs, n - i0);
            const float amt = smv[8].step (std::min (1.0f, S.cAmt * A), kG), mixT = S.cOn ? S.cMix : 0.0f, punch = smv[9].step (S.cPunch, kG);
            const float thr = st == C_FET ? -amt * 30 : st == C_BUS ? -4 - amt * 24 : st == C_670 ? -amt * 30 : -4 - amt * 32;   // BUS e PUMP sentono l'RMS: soglia piu' bassa (prima il BUS quasi non lavorava)
            const float knee = st == C_BUS ? 10.0f : st == C_670 ? 16.0f : 6.0f;
            // makeup che segue la riduzione media (circa 1 s): il compressore ingrossa senza far scendere il volume
            {   // media della riduzione pesata dall'energia: conta quanto comprime dove c'e' suono, non nei silenzi tra i colpi
                const float k1 = 1.0f - std::exp (-(float) m / (1.0f * sc)); float e1 = 0;
                for (int i = i0; i < i0 + m; ++i) e1 += 0.5f * (io[0][i] * io[0][i] + io[1][i] * io[1][i]);
                grNum += (e1 * env - grNum) * k1; grDen += (e1 - grDen) * k1;
                grAvgC = grDen > 1e-12f ? grNum / grDen : 0.0f;
            }
            const float makeupDb = juce::jmin (18.0f, grAvgC * 0.9f);
            float a = 0, e2 = 0;
            for (int i = i0; i < i0 + m; ++i) { const float l = io[0][i], r = io[1][i]; a = std::max (a, std::max (std::abs (l), std::abs (r))); e2 += 0.5f * (l * l + r * r); }
            e2 /= (float) m;
            float pg = 1;
            if (std::abs (punch) > 0.005f)
            {
                envF = a > envF ? pA1 * envF + (1 - pA1) * a : pR1 * envF + (1 - pR1) * a;
                envS = a > envS ? pA2 * envS + (1 - pA2) * a : pR2 * envS + (1 - pR2) * a;
                const float t = juce::jlimit (0.0f, 1.0f, (envF - envS) / (envS + 1e-4f));
                pg = dbToG (punch * (punch > 0 ? 8.0f : 10.0f) * t);
            }
            float det;
            if (st == C_FET || st == C_670) det = gToDb (a);
            else { rms = aRms * rms + (1 - aRms) * e2; det = 10.0f * std::log10 (rms + 1e-12f); }
            const float over = det - thr;
            const float ratio = st == C_FET ? 4 + amt * 16 : st == C_BUS ? 2 + 2 * amt : st == C_670 ? juce::jmin (14.0f, 1.6f + juce::jmax (0.0f, over) * 0.35f) : 10;
            float want = 0;
            if (over > knee / 2) want = over * (1 - 1 / ratio);
            else if (over > -knee / 2) { const float z = over + knee / 2; want = (1 - 1 / ratio) * z * z / (2 * knee); }
            if (st == C_670)
            {   // rilascio a due tempi come TC5: veloce sui colpi singoli, lento se continua a lavorare
                const float aR2 = std::exp (-1.0f / (rel * (1.0f + 1.5f * muCharge) * sc));   // rilascio che si allunga quando lavora tanto (prima fino a 5 volte: la riduzione restava ferma e non ingrossava)
                env = want > env ? aA * env + (1 - aA) * want : aR2 * env + (1 - aR2) * want;
                muCharge += ((env > 2 ? 1.0f : 0.0f) - muCharge) * (1.0f / (2.0f * sc));
            }
            else env = want > env ? aA * env + (1 - aA) * want : aR * env + (1 - aR) * want;
            maxGr = std::max (maxGr, env);
            const float gNew = dbToG (makeupDb - env) * pg, g0 = compG, mix0 = smv[7].v, mixN = smv[7].step (mixT, kG);
            const bool fetSat = st == C_FET && amt > 0.5f; const float fk = 1 + (amt - 0.5f) * 1.6f;
            const float b = 0.06f + 0.012f * std::min (env, 15.0f), d = 1.0f + 0.4f * amt, tb = std::tanh (b), nn = d * (1 - tb * tb);
            for (int k = 0; k < m; ++k)
            {
                const float t = (k + 1) / (float) m, gg = g0 + (gNew - g0) * t, mixC = mix0 + (mixN - mix0) * t;
                const int i = i0 + k; const float l = io[0][i], r = io[1][i];
                float yl = l * gg, yr = r * gg;
                if (fetSat) { yl = std::tanh (yl * fk) / fk; yr = std::tanh (yr * fk) / fk; }   // tutti i bottoni: un po' di sporco
                if (st == C_670)
                {   // valvola: seconda armonica che cresce con la riduzione, il corpo si gonfia
                    yl = (std::tanh (d * yl + b) - tb) / nn; yr = (std::tanh (d * yr + b) - tb) / nn;
                    ch[0].muDc += (yl - ch[0].muDc) * 0.00003f; yl -= ch[0].muDc; ch[1].muDc += (yr - ch[1].muDc) * 0.00003f; yr -= ch[1].muDc;
                }
                io[0][i] = l + (yl - l) * mixC; io[1][i] = r + (yr - r) * mixC;
            }
            compG = gNew;
        }
        gr = maxGr;
    }

    // OTT: tre bande (120 Hz / 2.5 kHz) divise a 8x; i guadagni di banda si calcolano ogni 8 campioni e si interpolano
    void processOtt (float** io, int n, const Settings& S, float A, float kS)
    {
        if (ottSr != sr4) { for (int c = 0; c < 2; ++c) { ch[c].xo[0].lp (120, 0.7071, sr4); ch[c].xo[1].lp (120, 0.7071, sr4); ch[c].xo[2].hp (2500, 0.7071, sr4); ch[c].xo[3].hp (2500, 0.7071, sr4); } ottSr = sr4; }
        static const float baseA[3] = { 47.8f, 22.4f, 13.5f }, baseR[3] = { 282, 282, 132 };
        const float tA = S.cAtt / 10.0f, tR = S.cRel / 150.0f, sc = (float) sr;
        float ca[3], cr[3]; for (int b = 0; b < 3; ++b) { ca[b] = std::exp (-1.0f / (0.001f * baseA[b] * tA * sc)); cr[b] = std::exp (-1.0f / (0.001f * baseR[b] * tR * sc)); }
        const float kG = 1.0f - std::pow (1.0f - kS, (float) kOs);
        float maxDown = 0;
        for (int i0 = 0; i0 < n; i0 += kOs)
        {
            const int m = std::min (kOs, n - i0);
            const float depth = smv[8].step (std::min (1.0f, S.cAmt * A), kG), mix0 = smv[7].v, mixN = smv[7].step (S.cOn ? S.cMix : 0.0f, kG);
            float band[3][2][8], pk[3] {};
            for (int k = 0; k < m; ++k)
                for (int c = 0; c < 2; ++c)
                {
                    const float x = io[c][i0 + k];
                    const float lo = ch[c].xo[1].run (ch[c].xo[0].run (x)), hi = ch[c].xo[3].run (ch[c].xo[2].run (x));
                    band[0][c][k] = lo; band[2][c][k] = hi; band[1][c][k] = x - lo - hi;
                    for (int b = 0; b < 3; ++b) pk[b] = std::max (pk[b], std::abs (band[b][c][k]));
                }
            float gN[3];
            for (int b = 0; b < 3; ++b)
            {
                ottEnv[b] = pk[b] > ottEnv[b] ? ca[b] * ottEnv[b] + (1 - ca[b]) * pk[b] : cr[b] * ottEnv[b] + (1 - cr[b]) * pk[b];
                const float db = gToDb (ottEnv[b] + 1e-9f);
                float gdb = 0;
                if (db > -20) gdb -= (db + 20) * (1 - 1.0f / 8.0f);
                if (db < -40 && db > -80) gdb += std::min ((-40 - db) * (1 - 1.0f / 3.0f), 24.0f);
                gdb *= depth; maxDown = std::max (maxDown, -gdb);
                gN[b] = dbToG (gdb + 3.0f * depth);   // un filo di guadagno come l'OUT dell'originale
            }
            for (int k = 0; k < m; ++k)
            {
                const float t = (k + 1) / (float) m, mixC = mix0 + (mixN - mix0) * t; const int i = i0 + k;
                for (int c = 0; c < 2; ++c)
                {
                    float y = 0; for (int b = 0; b < 3; ++b) y += band[b][c][k] * (ottG[b] + (gN[b] - ottG[b]) * t);
                    io[c][i] += (y - io[c][i]) * mixC;
                }
            }
            for (int b = 0; b < 3; ++b) ottG[b] = gN[b];
        }
        gr = maxDown;
    }

    // nastro a 1x: wow, flutter, eta (bump e alti chiusi), dropout. La saturazione del nastro e' gia' passata a 8x
    void processTape (float** io, int n, const Settings& S, float A)
    {
        const float age = std::min (1.5f, S.tAge * A), wow = S.tWow * A, flut = S.tFlut * A, drop = S.tDrop * A, tStep = 1.0f / (0.02f * (float) sr);
        if (std::abs (age - lastAge) > 0.002f) { lastAge = age; for (int c = 0; c < 2; ++c) { ch[c].bump.bell (85, 0.8, age * 2.5, sr); ch[c].tLp1.lp (20000 * (1 - 0.72f * std::min (1.0f, age)), 0.6, sr); } }
        const float wA = (float) (0.0087 * wow / (2 * kPi * 0.55) * sr), fA = (float) (0.0025 * flut / (2 * kPi * 6.3) * sr), f2A = (float) (0.001 * flut / (2 * kPi * 11.0) * sr);
        const double sw = std::sin (2 * kPi * 0.55 / sr), cw = std::cos (2 * kPi * 0.55 / sr), sf = std::sin (2 * kPi * 6.3 / sr), cf = std::cos (2 * kPi * 6.3 / sr), s2 = std::sin (2 * kPi * 11.0 / sr), c2 = std::cos (2 * kPi * 11.0 / sr);
        for (int i = 0; i < n; ++i)
        {
            // oscillatori a rotazione (niente seno per campione)
            { const double x = oW[0] * cw - oW[1] * sw, y = oW[0] * sw + oW[1] * cw; oW[0] = x; oW[1] = y; }
            { const double x = oF[0] * cf - oF[1] * sf, y = oF[0] * sf + oF[1] * cf; oF[0] = x; oF[1] = y; }
            { const double x = oF2[0] * c2 - oF2[1] * s2, y = oF2[0] * s2 + oF2[1] * c2; oF2[0] = x; oF2[1] = y; }
            wowPh += 0.55 / sr; if (wowPh > 1) { wowPh -= 1; wowTarget = rng.next() * 0.4f; const double r = std::sqrt (oW[0] * oW[0] + oW[1] * oW[1]); oW[0] /= r; oW[1] /= r; oF[0] /= std::hypot (oF[0], oF[1]); oF[1] /= std::hypot (oF[0], oF[1]); }
            wowDrift += 0.0003f * (wowTarget - wowDrift);
            wA_s += (wA - wA_s) * 0.001f; fA_s += (fA - fA_s) * 0.001f; f2A_s += (f2A - f2A_s) * 0.001f;
            // modulazione attorno al centro fisso (media zero): il nastro oscilla ma non sposta il tempo del pezzo
            const float d = (float) tapeD0 + wA_s * ((float) oW[1] + (wowDrift - 0.2f)) + fA_s * (float) oF[1] + f2A_s * (float) oF2[1];
            if (drop > 0.001f)
            {
                if (dropT <= 0 && rng.uni() < drop * 1.5f / (float) sr) dropT = (int) (sr * (0.02 + 0.1 * rng.uni()));
                const float tgt = dropT > 0 ? 0.35f + 0.06f * rng.uni() : 1.0f; if (dropT > 0) --dropT;
                dropG += (tgt - dropG) * 0.005f;
            }
            else dropG += (1.0f - dropG) * 0.005f;
            tapeW = S.tOn ? juce::jmin (1.0f, tapeW + tStep) : juce::jmax (0.0f, tapeW - tStep);
            for (int c = 0; c < 2; ++c)
            {
                auto& C = ch[c];
                tapeDl[c].push (io[c][i]);
                float v = tapeDl[c].read (d + (c ? 0.3f : 0.0f) * juce::jmin (1.0f, wA_s + fA_s));   // il piccolo scarto fra i canali solo quando il nastro si muove
                if (age > 0.001f) v = C.tLp1.run (C.bump.run (v));
                v *= dropG;
                if (tapeW < 1.0f) { const float plain = tapeDl[c].read ((float) tapeD0); v = plain + (v - plain) * tapeW; }
                io[c][i] = v;
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
        wLpA = (float) (1.0 - std::exp (-2 * kPi * 100.0 / sr));
        for (auto& C : ch)
        {
            C.hp1.set (S.fHp, S.fRes * 0.6f, sr); C.hp2.set (S.fHp, 0.0f, sr);
            C.lp1.set (S.fLp, S.fRes, sr); C.lp2.set (S.fLp, 0.0f, sr);
            C.boom.bell (70, 0.9, S.fBoom, sr); C.crack.bell (2600, 1.0, S.fCrack, sr);
            C.wLo.lowShelf (85, S.fWarm * 4.5, sr); C.wDip.bell (320, 0.8, -S.fWarm * 2.5, sr); C.wHi.highShelf (9000, -S.fWarm * 3.0, sr);
            C.air.highShelf (11000, S.fAir * 7.0, sr);
        }
    }
    struct Ch
    {
        Biquad aa[3], post[2], tiltLo, tiltHi, boom, crack, bump, tLp1, wLo, wDip, wHi, air, xo[4]; Svf hp1, hp2, lp1, lp2; float wLp = 0, wDc = 0, muDc = 0;
        float pre = 0, held = 0, pink = 0, popLp = 0, hissHp = 0;
    };
    double sr = 44100, sr4 = 44100 * 8, wowPh = 0, flPh = 0, fl2Ph = 0, humPh = 0, ottSr = -1;
    int maxBlock = 512, osLat = 0, lastModel = -1, dropT = 0, lastDModel = -1, tapeD0 = 8;
    float holdPh = 1, lastRate = -1, driveMk = 1, mkFor = -100, env = 0, rms = 0, envF = 0, envS = 0, inRms = 1e-6f, outRms = 1e-6f, dropG = 1, wowTarget = 0, wowDrift = 0, noiseEnv = 0, muCharge = 0, lastAge = -1;
    float tapeW = 0, filtW = 0, fW1[32] {}, w24 = 0, w24a[32] {}; int cCur = -1, sCur = -1;
    float wA_s = 0, fA_s = 0, f2A_s = 0, compG = 1, grAvgC = 0, grNum = 0, grDen = 0, ottG[3] { 1, 1, 1 }, lastTone = -100;
    double oW[2] { 1, 0 }, oF[2] { 1, 0 }, oF2[2] { 1, 0 };
    float fk[7] { -1, -1, -1, -1, -1, -1, -1 }, crackLp[2] {}, popZ[2] {}, wLpA = 0.01f, ottEnv[3] {};
    Sm1 smv[12], fs[7];
    std::unique_ptr<juce::dsp::Oversampling<float>> os;
    OsGate gUp; Dl dryDl[2], tapeDl[2], erDl[2], fdn[4], bypDl[2]; int upTail = 0; float fdnLp[4] {}, wetHp[2] {}, roomEnv = 0, roomHold = 0, roomGate = 0;
    Ch ch[2]; Rng rng;
    std::vector<float> gRing; int gRw = 0; float calRms = 0, calLt = -100, calDb = 0;
    juce::AudioBuffer<float> satDry;
};
}
