#pragma once
// Jackpot Studio Comp: motore dei compressori (emulazioni di comportamento, zero latenza)
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

namespace jc
{
enum Model { OPTO2A = 0, OPTO3A, FET76, VCABUS, VARIMU, RMS160, DISTRESS, OTT, VC670, MULTI, SSLCH, kNumModels };
constexpr int kStages = 6;
inline const char* stagePfx (int s) { static const char* p[] = { "a_", "b_", "c_", "d_", "e_", "f_" }; return p[s]; }

struct ModelInfo
{
    const char* name;      // nome a schermo
    const char* inspired;  // apparecchio a cui si ispira
    const char* driveLabel;
    bool fixedThreshold;   // true: la manopola spinge dentro una soglia fissa (input), false: e una soglia
    bool attKnob, relKnob; // false: tempi decisi dal modello
    juce::StringArray ratios; // etichette dei pulsanti di ratio / time constant
    const char* ratioLabel;
    juce::uint32 face;     // colore del frontale (VU)
    juce::uint32 accent;
};

inline const ModelInfo& info (int m)
{
    static const ModelInfo I[kNumModels] = {
        { "OPTO 2A", "ispirato al Teletronix LA-2A", "PEAK REDUCTION", false, false, false, { "COMPRESS", "LIMIT" }, "MODO", 0xffe9dfc6, 0xffffb000 },
        { "OPTO 3A", "ispirato all'Urei LA-3A", "PEAK REDUCTION", false, false, false, { "COMPRESS", "LIMIT" }, "MODO", 0xffd8d2c4, 0xffff8a3d },
        { "FET 76", "ispirato all'Urei 1176", "INPUT", true, true, true, { "4", "8", "12", "20", "ALL" }, "RATIO", 0xffeee6d2, 0xff5ef2ff },
        { "VCA BUS", "ispirato al bus comp SSL G", "THRESHOLD", false, true, true, { "2", "4", "10" }, "RATIO", 0xff1d2a24, 0xff7dff5a },
        { "VARI-MU", "ispirato al Manley Variable Mu", "INPUT", true, true, true, { "COMPRESS", "LIMIT" }, "MODO", 0xff2a2418, 0xffffd23f },
        { "RMS 160", "ispirato al dbx 160", "THRESHOLD", false, false, false, { "-" }, "", 0xff121418, 0xffa66bff },
        { "DISTRESS", "ispirato all'Empirical Labs Distressor", "INPUT", true, true, true, { "1", "2", "3", "4", "6", "10", "20", "NUKE" }, "RATIO", 0xff10223a, 0xff2e8bff },
        { "OTT", "ispirato all'OTT multibanda (Xfer)", "DEPTH", false, true, true, { "UP + DOWN", "DOWN", "UP" }, "MODO", 0xff1a1024, 0xffff4fd8 },
        { "VARI 670", "ispirato al Fairchild 670 (come il VC-670 di IK)", "INPUT", true, true, true, { "TC1", "TC2", "TC3", "TC4", "TC5", "TC6" }, "TIME CONSTANT", 0xff2b2620, 0xffffc861 },
        { "MULTIBANDA", "multibanda a 4 bande (stile Pro-MB / alpha)", "THRESH", false, true, true, { "2 BANDE", "3 BANDE", "4 BANDE" }, "BANDE", 0xff101820, 0xff3dd6ff },
        { "SSL 4000E", "ispirato al compressore di canale SSL 4000 E", "THRESHOLD", false, true, true, { "AUTO", "FAST ATT" }, "ATTACCO", 0xff1c1f24, 0xff4dd2ff } };
    return I[juce::jlimit (0, kNumModels - 1, m)];
}

// multibanda: impostazioni condivise
struct MbParams
{
    float xo[3] { 120, 1000, 6000 };
    float thr[4] { -20, -20, -20, -20 }, ratio[4] { 2.5f, 2.5f, 2.5f, 2.5f }, att[4] { 10, 10, 10, 10 }, rel[4] { 150, 150, 150, 150 }, gain[4] {};
    bool solo[4] {}, byp[4] {};
};
struct StageParams
{
    bool on = true; int model = 0; float drive = 0; int ratio = 0; float att = 0.5f, rel = 0.5f, outDb = 0, color = 0, mix = 1;
    float drive2 = 0, att2 = 0, thr670 = 0; int ratio2 = 0; bool link = true, outLink = false;   // 670: canale destro (o VERT) e collegamento
    float cratio = 4; int mode = 0; bool hp = false;   // ratio continuo (160, SSL); modo (SSL attacco, DISTRESS distorsione); HP del rilevatore (DISTRESS)
    const MbParams* mb = nullptr;
};
inline int mbBands (const StageParams& p) { return 2 + juce::jlimit (0, 2, p.ratio); }
// manopola SOGLIE +/-: 0 dB al valore di default (0.3), da -12 a +12 dB
inline float mbOffset (float d) { return d >= 0.3f ? (d - 0.3f) / 0.7f * 12.0f : (d - 0.3f) / 0.3f * 12.0f; }

// ---------------- soglie e ingresso ----------------
// Come l'hardware:
// - modelli a soglia (OPTO, VCA, RMS, SSL): THRESHOLD/PEAK REDUCTION da 0 a -40 dB, il volume non cambia
// - FET e DISTRESS: soglia fissa; l'INPUT alza davvero il segnale (fino a +48 dB) e lo spinge dentro: piu' compressione,
//   piu' volume e piu' carattere. Si riporta il volume con OUTPUT
// - VARI-MU (Manley) e 670: INPUT (alza il volume e spinge le valvole) e THRESHOLD separati
// A manopole a zero ogni modello e' neutro: soglia a 0 dBFS e nessun guadagno.
inline float t670Thr (float a) { return -a * 28.0f; }
inline float tMuThr (float a) { return -a * 30.0f; }
inline float inputRangeDb (int m) { return m == VC670 ? 18.0f : m == VARIMU ? 24.0f : 48.0f; }
inline bool isInputModel (int m) { return m == FET76 || m == DISTRESS; }                 // solo INPUT, soglia fissa
inline bool hasInput (int m) { return m == FET76 || m == DISTRESS || m == VARIMU || m == VC670; }
inline float inputDb (int m, float d) { return hasInput (m) ? d * inputRangeDb (m) : 0.0f; }
inline float knobThrDb (int m, float d, float t)   // dove inizia la compressione, riferita al segnale gia' alzato dall'INPUT
{
    if (m == VC670) return t670Thr (t);
    if (m == VARIMU) return tMuThr (t);
    if (isInputModel (m)) return 0.0f;
    return -d * 40.0f;
}
// soglia riferita al segnale che entra nel plugin (quella mostrata a schermo)
inline float stageThrDb (int m, float d, float t = 0) { return knobThrDb (m, d, t) - inputDb (m, d); }
// ginocchio: larghezza in dB della zona in cui la compressione entra piano piano
inline float kneeDb (int m, int ratio)
{
    switch (m)
    {
        case OPTO2A: return ratio == 0 ? 14.0f : 6.0f; case OPTO3A: return ratio == 0 ? 10.0f : 5.0f; case FET76: return ratio == 0 ? 6.0f : 2.0f;
        case VCABUS: return ratio == 0 ? 4.0f : ratio == 1 ? 2.0f : 1.0f; case VARIMU: return ratio == 0 ? 18.0f : 10.0f; case RMS160: return 0.5f; case VC670: return 12.0f;
        case SSLCH: return 4.0f; case DISTRESS: return ratio <= 2 ? 10.0f : ratio < 6 ? 4.0f : 1.0f; default: return 0.0f;
    }
}
// conversione dalla prima versione per i modelli a soglia: stessa curva, ma ora la manopola indica dove la compressione INIZIA
inline float convOldDrive (int m, int ratio, float d) { return juce::jlimit (0.0f, 1.0f, (kneeDb (m, ratio) * 0.5f + 40.0f * d) / 40.0f); }
inline float convOld670Thr (float a) { return juce::jlimit (0.0f, 1.0f, (10.0f + 24.0f * a) / 28.0f); }
// rapporto continuo del 160 e del canale SSL: da 1:1 a infinito (oltre 40 = limiter)
inline juce::String fmtRatio (float r) { return r >= 40.0f ? juce::String::fromUTF8 ("\xe2\x88\x9e : 1") : juce::String (r, r < 10 ? 1 : 0) + " : 1"; }

// valori mostrati a schermo
inline juce::String fmtDb (float v, int dec = 1) { return (v > 0.04f ? "+" : "") + juce::String (v, dec) + " dB"; }
inline juce::String fmtDrive (int m, float d)
{
    if (m == MULTI) return fmtDb (mbOffset (d));
    if (m == OTT) return juce::String (juce::roundToInt (d * 100)) + "%";
    if (hasInput (m)) return d < 0.002f ? juce::String ("0 dB") : "+" + juce::String (d * inputRangeDb (m), 1) + " dB";
    if (m <= OPTO3A) return juce::String (juce::roundToInt (d * 100));
    return d < 0.002f ? juce::String ("0 dB") : juce::String (-d * 40.0f, 1) + " dB";
}
// FET: manopole numerate 1..7 come sull'1176, verso destra = piu' veloce. DISTRESS: 0..10, verso destra = piu' lento
inline float fetAttMs (float v) { return 0.8f * std::pow (0.02f / 0.8f, v); }       // 800 us -> 20 us
inline float fetRelMs (float v) { return 1100.0f * std::pow (50.0f / 1100.0f, v); }  // 1.1 s -> 50 ms
inline float attMs (int m, float a)
{
    switch (m)
    {
        case FET76: return fetAttMs (a);
        case VCABUS: { static const float t[] = { 0.1f, 0.3f, 1, 3, 10, 30 }; return t[juce::jlimit (0, 5, (int) (a * 5.99f))]; }
        case DISTRESS: return 0.05f * std::pow (600.0f, a);              // 0.05 .. 30 ms
        case OTT: return 22.4f * std::pow (4.0f, (a - 0.5f) * 2.0f);
        case VARIMU: return 25.0f + 45.0f * a;                           // Manley: 25 .. 70 ms
        case SSLCH: return -1.0f;
        case OPTO2A: return 10.0f;
        case OPTO3A: return 1.5f;
        default: return 5.0f;
    }
}
inline float muRecMs (float r) { static const float t[] = { 200, 400, 600, 4000, 8000 }; return t[juce::jlimit (0, 4, juce::roundToInt (r * 4.0f))]; }
inline float relMs (int m, float r)   // -1 = auto
{
    switch (m)
    {
        case FET76: return fetRelMs (r);
        case VCABUS: { static const float t[] = { 100, 300, 600, 1200, -1 }; return t[juce::jlimit (0, 4, (int) (r * 4.99f))]; }
        case DISTRESS: return 50.0f * std::pow (70.0f, r);               // 50 .. 3500 ms
        case SSLCH: return 100.0f * std::pow (40.0f, r);                 // 0.1 .. 4 s
        case OTT: return 282.0f * std::pow (4.0f, (r - 0.5f) * 2.0f);
        case VARIMU: return muRecMs (r);                                  // Manley RECOVERY: 0.2 0.4 0.6 4 8 s
        default: return -1;
    }
}
inline juce::String fmtTime (float ms) { return ms < 0 ? "AUTO" : ms < 1 ? juce::String (juce::roundToInt (ms * 1000)) + " us" : ms < 10 ? juce::String (ms, 1) + " ms" : ms < 1000 ? juce::String (juce::roundToInt (ms)) + " ms" : juce::String (ms / 1000, 2) + " s"; }

// curva statica: riduzione di guadagno (dB positivi) per un livello in dB al rilevatore (gia' alzato dall'INPUT)
inline float gainComputer (const StageParams& p, float levDb)
{
    float T = knobThrDb (p.model, p.drive, p.thr670), R = 4, W = kneeDb (p.model, p.ratio);
    switch (p.model)
    {
        case OPTO2A: R = p.ratio == 0 ? 3.0f : 12.0f; break;
        case OPTO3A: R = p.ratio == 0 ? 3.0f : 10.0f; break;
        case FET76: { static const float r[] = { 4, 8, 12, 20, 20 }; R = r[juce::jlimit (0, 4, p.ratio)]; break; }
        case VCABUS: { static const float r[] = { 2, 4, 10 }; R = r[juce::jlimit (0, 2, p.ratio)]; break; }
        case VARIMU:
        {   // valvola a pendenza variabile: il rapporto cresce con la riduzione. COMPRESS ~1.5:1 -> 4:1, LIMIT ~4:1 -> 20:1
            const float over = juce::jmax (0.0f, levDb - T);
            R = p.ratio == 0 ? juce::jmin (4.0f, 1.5f + over / 8.0f) : juce::jmin (20.0f, 4.0f + over / 1.5f);
            break;
        }
        case RMS160: R = juce::jlimit (1.0f, 60.0f, p.cratio); break;
        case SSLCH: R = juce::jlimit (1.0f, 100.0f, p.cratio); break;
        case VC670:
        {   // valvola remote-cutoff: il rapporto cresce con quanto si va oltre la soglia (circa 1.3:1 -> 20:1), ginocchio larghissimo
            const float K = 6.0f;
            const float o = levDb - T - K, x = o > 30 ? o : K * std::log1p (std::exp (o / K));
            const float k = 0.18f;
            const float g = x - (1.0f / k) * std::log ((1.3f + k * x) / 1.3f);
            return juce::jmax (0.0f, g - 0.6f);
        }
        case MULTI: return 0;
        case DISTRESS: { static const float r[] = { 1, 2, 3, 4, 6, 10, 20, 100 }; R = r[juce::jlimit (0, 7, p.ratio)]; break; }
        case OTT:
        {   // curva statica indicativa della banda media: giu sopra -20, su sotto -40
            float g = 0; const bool dn = p.ratio != 2, up = p.ratio != 1;
            if (dn && levDb > -20) g += (levDb + 20) * (1 - 1.0f / 8.0f);
            if (up && levDb < -40 && levDb > -80) g -= juce::jmin ((-40 - levDb) * (1 - 1.0f / 3.0f), 24.0f);
            return g * p.drive;
        }
        default: break;
    }
    if (R <= 1.001f) return 0;
    T += W * 0.5f;   // la soglia a schermo e' dove il ginocchio comincia
    const float over = levDb - T, s = 1.0f - 1.0f / R;
    if (2 * over < -W) return 0;
    if (2 * std::abs (over) <= W) return s * (over + W / 2) * (over + W / 2) / (2 * W);
    float gr = s * over;
    if (p.model == FET76 && p.ratio == 4) gr *= 1.15f;    // "all buttons": eccesso tipico
    return gr;
}
// guadagno vero dell'INPUT (alza il volume come sull'hardware)
inline float inputGainDb (const StageParams& p) { return inputDb (p.model, p.drive); }
class Stage
{
public:
    void prepare (double s) { sr = s; reset(); }
    void reset() { gr = fast = slow = charge = rms = 0; grAvg = 0; eIn = eOut = 0; autoC = 1; hpS[0] = hpS[1] = 0; dHp[0] = dHp[1] = 0; dc[0] = dc[1] = 0; for (int c = 0; c < 2; ++c) { g6[c] = f6[c] = s6[c] = c6[c] = lfLp[c] = hfLp[c] = bLp[c] = 0; } for (auto& e : mEnv) e = 0; sm = {}; smInit = false; }
    float grNow() const { return gr; }
    float grAvg = 0;

    // L/R in place; dL/dR = segnale del rilevatore (gia filtrato), stessa lunghezza
    void process (float* L, float* R, const float* dL, const float* dR, int N, const StageParams& p0, bool autoGain, float* deltaL = nullptr, float* deltaR = nullptr)
    {
        // manopole che cambiano il suono di colpo vengono "smussate" (circa 15 ms): niente click quando le muovi o le automatizzi
        StageParams p = p0;
        const float kS = 1.0f - std::exp (-1.0f / (0.015f * (float) sr));
        if (! smInit) { sm.inDb = inputGainDb (p0); sm.out = p0.outDb; sm.mix = p0.mix; sm.col = p0.color; sm.drive = p0.drive; sm.thr = p0.thr670; smInit = true; }
        if (p.model == OTT) { processOtt (L, R, N, p, autoGain, deltaL, deltaR, kS); return; }
        if (p.model == VC670) { process670 (L, R, dL, dR, N, p, autoGain, deltaL, deltaR, kS); return; }
        if (p.model == MULTI) { processMb (L, R, dL, dR, N, p, autoGain, deltaL, deltaR, kS); return; }
        const float a = std::exp (-1.0f / (0.001f * juce::jmax (0.01f, attMs (p.model, p.att)) * (float) sr));
        const float rm = relMs (p.model, p.rel);
        const float rFixed = rm > 0 ? std::exp (-1.0f / (0.001f * rm * (float) sr)) : 0.0f;
        const float avgC = std::exp (-1.0f / (1.5f * (float) sr)), avgC2 = std::exp (-1.0f / (0.8f * (float) sr));
        const float rmsC = std::exp (-1.0f / (0.006f * (float) sr));
        hfA = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 2500.0f / (float) sr);
        const float hpK = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 100.0f / (float) sr);   // HP del rilevatore DISTRESS
        const float inT = inputGainDb (p0);
        // DISTRESS: tipo di distorsione dai pulsanti (come l'originale non ha una manopola: cresce con il livello)
        int satM = p.model; float colT = p.color;
        if (p.model == DISTRESS) colT = p.mode == 0 ? 0.0f : 0.55f;
        for (int i = 0; i < N; ++i)
        {
            sm.inDb += (inT - sm.inDb) * kS; sm.out += (p0.outDb - sm.out) * kS; sm.mix += (p0.mix - sm.mix) * kS; sm.col += (colT - sm.col) * kS; sm.drive += (p0.drive - sm.drive) * kS; sm.thr += (p0.thr670 - sm.thr) * kS;
            p.drive = sm.drive; p.thr670 = sm.thr;
            const float inG = juce::Decibels::decibelsToGain (sm.inDb);
            const float xl = L[i] * inG, xr = R[i] * inG;
            // rilevatore
            float lev;
            if (p.model == RMS160 || p.model == VCABUS || p.model == SSLCH)
            {
                const float e = 0.5f * (dL[i] * dL[i] + dR[i] * dR[i]) * inG * inG;
                rms = rmsC * rms + (1 - rmsC) * e;
                lev = p.model == RMS160 ? std::sqrt (rms) * 1.414f : juce::jmax (std::sqrt (rms) * 1.414f, juce::jmax (std::abs (dL[i]), std::abs (dR[i])) * inG * 0.8f);
            }
            else if (p.model == OPTO2A || p.model == OPTO3A)
            {   // il sidechain dell'LA-2A/3A e piu sensibile agli acuti (enfasi del circuito T4): de-essa un poco da solo
                float m2 = 0;
                for (int c = 0; c < 2; ++c) { const float x = c ? dR[i] : dL[i]; hpS[c] += (x - hpS[c]) * hfA; const float hf = x - hpS[c]; m2 = juce::jmax (m2, std::abs (x + hf * (p.model == OPTO2A ? 0.8f : 0.5f))); }
                lev = m2 * inG;
            }
            else if (p.model == DISTRESS && p.hp)
            {   // HP: il rilevatore ignora i bassi (cassa e basso non fanno pompare)
                float m2 = 0;
                for (int c = 0; c < 2; ++c) { const float x = c ? dR[i] : dL[i]; dHp[c] += (x - dHp[c]) * hpK; m2 = juce::jmax (m2, std::abs (x - dHp[c])); }
                lev = m2 * inG;
            }
            else lev = juce::jmax (std::abs (dL[i]), std::abs (dR[i])) * inG;
            // OPTO e FET sono a retroazione come gli originali: il rilevatore sente l'uscita (gia' compressa).
            const bool fb = p.model == OPTO2A || p.model == OPTO3A || p.model == FET76;
            if (fb) lev *= juce::Decibels::decibelsToGain (-gr);
            const float levDb = 20.0f * std::log10 (lev + 1.0e-9f);
            float tgt = gainComputer (p, levDb);
            if (fb)
            {
                static const float fr[] = { 4, 8, 12, 20, 20 };
                const float Rn = p.model == FET76 ? fr[juce::jlimit (0, 4, p.ratio)] : (p.ratio == 0 ? 3.0f : (p.model == OPTO2A ? 12.0f : 10.0f));
                tgt = juce::jmin (40.0f, tgt * Rn);
            }
            // balistica
            switch (p.model)
            {
                case OPTO2A: case OPTO3A:
                {
                    const bool la2 = p.model == OPTO2A;
                    const float at = std::exp (-1.0f / (0.001f * (la2 ? 10.0f : 1.5f) * (float) sr));
                    const float rf = std::exp (-1.0f / (0.001f * (la2 ? 60.0f : 40.0f) * (float) sr));
                    const float rs = std::exp (-1.0f / ((la2 ? 0.5f + 3.5f * charge : 0.3f + 1.2f * charge) * (float) sr));
                    fast = tgt > fast ? at * fast + (1 - at) * tgt : rf * fast + (1 - rf) * tgt;
                    slow = tgt > slow ? at * slow + (1 - at) * tgt : rs * slow + (1 - rs) * tgt;
                    gr = 0.5f * (fast + slow);
                    charge += ((gr > 1 ? juce::jmin (1.0f, gr / 10.0f) : 0.0f) - charge) * (gr > 1 ? 1.0f / (4.0f * (float) sr) : 1.0f / (15.0f * (float) sr));
                    break;
                }
                case SSLCH:
                {   // canale SSL E: attacco che dipende dal segnale (30 ms per 20 dB -> 3 ms) o FAST ATT (3 ms per 20 dB), rilascio 0.1-4 s
                    if (tgt > gr)
                    {
                        const float ms = p.mode == 1 ? 3.0f : juce::jlimit (3.0f, 30.0f, 30.0f / (1.0f + (tgt - gr) / 2.5f));
                        const float c = std::exp (-1.0f / (0.001f * ms * (float) sr)); gr = c * gr + (1 - c) * tgt;
                    }
                    else gr = rFixed * gr + (1 - rFixed) * tgt;
                    break;
                }
                case RMS160:
                {
                    if (tgt > gr)
                    {   // attacco del 160: 15 ms per 10 dB, 5 ms per 20 dB, 3 ms per 30 dB
                        const float d = tgt - gr; const float ms = d <= 10 ? 15.0f : d <= 20 ? 15.0f - (d - 10) : juce::jmax (3.0f, 5.0f - (d - 20) * 0.2f);
                        const float c = std::exp (-1.0f / (0.001f * ms * (float) sr)); gr = c * gr + (1 - c) * tgt;
                    }
                    else gr = juce::jmax (tgt, gr - 120.0f / (float) sr);   // 120 dB/s
                    break;
                }
                case VARIMU:
                {   // Manley: ATTACK 25-70 ms, RECOVERY a scatti; la valvola rende il rilascio un po' piu' lento quando ha lavorato tanto
                    if (tgt > gr) gr = a * gr + (1 - a) * tgt;
                    else { const float rc = std::exp (-1.0f / (0.001f * rm * (1.0f + 0.3f * charge) * (float) sr)); gr = rc * gr + (1 - rc) * tgt; }
                    charge += ((gr > 3 ? 1.0f : 0.0f) - charge) * (1.0f / (3.0f * (float) sr));
                    break;
                }
                default:
                {
                    if (tgt > gr) gr = a * gr + (1 - a) * tgt;
                    else if (rm > 0)
                    {
                        float rc = rFixed;
                        if (p.model == FET76) rc = std::exp (-1.0f / (0.001f * rm * (0.6f + 0.4f * juce::jmin (1.0f, gr / 10.0f)) * (p.ratio == 4 ? 0.6f : 1.0f) * (float) sr));   // ALL: rilascio piu rapido, pompa
                        if (p.model == DISTRESS && p.ratio == 7) rc = std::exp (-1.0f / (0.001f * juce::jmin (rm, 80.0f) * (float) sr));
                        gr = rc * gr + (1 - rc) * tgt;
                    }
                    else
                    {   // auto release del bus: doppia costante
                        const float rf = std::exp (-1.0f / (0.1f * (float) sr)), rs = std::exp (-1.0f / ((0.15f + 1.2f * charge) * (float) sr));
                        fast = tgt > fast ? a * fast + (1 - a) * tgt : rf * fast + (1 - rf) * tgt;
                        slow = tgt > slow ? a * slow + (1 - a) * tgt : rs * slow + (1 - rs) * tgt;
                        gr = 0.5f * (fast + slow);
                        charge += ((gr > 1 ? 1.0f : 0.0f) - charge) * (1.0f / (3.0f * (float) sr));
                    }
                    break;
                }
            }
            if (p.model == DISTRESS && p.ratio == 0) gr = 0;   // 1:1 = solo colore
            const float g = juce::Decibels::decibelsToGain (-gr);
            grAvg = avgC * grAvg + (1 - avgC) * gr;
            const float og = juce::Decibels::decibelsToGain (sm.out);
            float yl = xl * g, yr = xr * g;
            if (deltaL) { deltaL[i] = xl * (1 - g); deltaR[i] = xr * (1 - g); }   // cio che lo stadio toglie
            eIn = avgC2 * eIn + (1 - avgC2) * 0.5f * (L[i] * L[i] + R[i] * R[i]);
            yl *= og; yr *= og;
            // colore: satura dove satura l'hardware (dopo il guadagno: piu' INPUT = piu' carattere)
            if (p.model == DISTRESS) { satDist (yl, 0, p.mode, sm.col); satDist (yr, 1, p.mode, sm.col); }
            else { sat (yl, 0, satM, sm.col); sat (yr, 1, satM, sm.col); }
            // auto gain: allinea l'energia d'uscita a quella d'ingresso (media su ~1 s)
            eOut = avgC2 * eOut + (1 - avgC2) * 0.5f * (yl * yl + yr * yr);
            if (autoGain && eIn > 1.0e-8f && eOut > 1.0e-10f) autoC += (juce::jlimit (0.02f, 16.0f, std::sqrt (eIn / eOut)) - autoC) * 0.0002f;
            else if (! autoGain) autoC = 1;
            yl *= autoC; yr *= autoC;
            L[i] = L[i] * (1 - sm.mix) + yl * sm.mix; R[i] = R[i] * (1 - sm.mix) + yr * sm.mix;
        }
    }

private:
    // ---- OTT: tre bande (120 Hz / 2.5 kHz), compressione verso il basso sopra soglia e verso l'alto sotto soglia ----
    struct Bq { double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1[2] = {}, z2[2] = {}; float run (float x, int c) { const double y = b0 * x + z1[c]; z1[c] = b1 * x - a1 * y + z2[c]; z2[c] = b2 * x - a2 * y; return (float) y; }
        void set (bool hp, double f, double sr) { const double w = juce::MathConstants<double>::twoPi * f / sr, cs = std::cos (w), al = std::sin (w) / (2 * 0.7071); const double a0 = 1 + al; if (hp) { b0 = (1 + cs) / 2 / a0; b1 = -(1 + cs) / a0; b2 = b0; } else { b0 = (1 - cs) / 2 / a0; b1 = (1 - cs) / a0; b2 = b0; } a1 = -2 * cs / a0; a2 = (1 - al) / a0; } };
    Bq xo[4][2]; double xoSr = 0; float oEnv[3] = {}, oG[3] = {};
    void processOtt (float* L, float* R, int N, const StageParams& p, bool autoGain, float* deltaL, float* deltaR, float kS)
    {
        if (xoSr != sr) { for (int k = 0; k < 2; ++k) { xo[0][k].set (false, 120, sr); xo[1][k].set (false, 120, sr); xo[2][k].set (true, 2500, sr); xo[3][k].set (true, 2500, sr); } xoSr = sr; }
        static const float baseA[3] = { 47.8f, 22.4f, 13.5f }, baseR[3] = { 282, 282, 132 };
        const float tA = attMs (OTT, p.att) / 22.4f, tR = relMs (OTT, p.rel) / 282.0f;
        float ca[3], cr[3]; for (int b = 0; b < 3; ++b) { ca[b] = std::exp (-1.0f / (0.001f * baseA[b] * tA * (float) sr)); cr[b] = std::exp (-1.0f / (0.001f * baseR[b] * tR * (float) sr)); }
        const bool dn = p.ratio != 2, up = p.ratio != 1;
        const float avgC = std::exp (-1.0f / (1.5f * (float) sr)), avgC2 = std::exp (-1.0f / (0.8f * (float) sr));
        for (int i = 0; i < N; ++i)
        {
            sm.out += (p.outDb - sm.out) * kS; sm.mix += (p.mix - sm.mix) * kS; sm.col += (p.color - sm.col) * kS; sm.drive += (p.drive - sm.drive) * kS;
            const float depth = sm.drive, og = juce::Decibels::decibelsToGain (sm.out);
            float band[3][2];
            for (int c = 0; c < 2; ++c)
            {
                const float x = c ? R[i] : L[i];
                const float lo = xo[1][c].run (xo[0][c].run (x, c), c), hi = xo[3][c].run (xo[2][c].run (x, c), c);
                band[0][c] = lo; band[2][c] = hi; band[1][c] = x - lo - hi;
            }
            float yl = 0, yr = 0, maxDown = 0;
            for (int b = 0; b < 3; ++b)
            {
                const float lev = juce::jmax (std::abs (band[b][0]), std::abs (band[b][1]));
                oEnv[b] = lev > oEnv[b] ? ca[b] * oEnv[b] + (1 - ca[b]) * lev : cr[b] * oEnv[b] + (1 - cr[b]) * lev;
                const float db = 20.0f * std::log10 (oEnv[b] + 1.0e-9f);
                float gdb = 0;
                if (dn && db > -20) gdb -= (db + 20) * (1 - 1.0f / 8.0f);
                if (up && db < -40 && db > -80) gdb += juce::jmin ((-40 - db) * (1 - 1.0f / 3.0f), 24.0f);
                gdb *= depth; maxDown = juce::jmax (maxDown, -gdb);
                const float gg = juce::Decibels::decibelsToGain (gdb);
                oG[b] = gg;
                yl += band[b][0] * gg; yr += band[b][1] * gg;
            }
            gr = juce::jmax (0.0f, maxDown);
            grAvg = avgC * grAvg + (1 - avgC) * gr;
            sat (yl, 0, OTT, sm.col); sat (yr, 1, OTT, sm.col);
            if (deltaL) { deltaL[i] = L[i] - yl; deltaR[i] = R[i] - yr; }
            eIn = avgC2 * eIn + (1 - avgC2) * 0.5f * (L[i] * L[i] + R[i] * R[i]);
            eOut = avgC2 * eOut + (1 - avgC2) * 0.5f * (yl * yl + yr * yr);
            if (autoGain && eIn > 1.0e-8f && eOut > 1.0e-10f) autoC += (juce::jlimit (0.06f, 16.0f, std::sqrt (eIn / eOut)) - autoC) * 0.0002f;
            else if (! autoGain) autoC = 1;
            yl *= og * autoC; yr *= og * autoC;
            L[i] = L[i] * (1 - sm.mix) + yl * sm.mix; R[i] = R[i] * (1 - sm.mix) + yr * sm.mix;
        }
    }
    // ---- 670: valvole 6386 a pendenza variabile, trasformatori d'ingresso e d'uscita, modo L/R o LAT/VERT (M/S) ----
    float g6[2] {}, f6[2] {}, s6[2] {}, c6[2] {}, lfLp[2] {}, hfLp[2] {}, bLp[2] {};
    void process670 (float* L, float* R, const float* dL, const float* dR, int N, const StageParams& p, bool autoGain, float* deltaL, float* deltaR, float kS)
    {
        // due canali veri come l'originale: INPUT, THRESHOLD e TIME CONSTANT per lato. LINK acceso = il destro segue il sinistro
        // e (in L/R) i due rilevatori sono uniti, cosi' l'immagine stereo non si sposta
        StageParams pc[2] = { p, p };
        if (! p.link) { pc[1].drive = p.drive2; pc[1].thr670 = p.att2; pc[1].ratio = p.ratio2; }
        const float inT[2] = { inputGainDb (pc[0]), inputGainDb (pc[1]) };
        const bool ms = p.rel > 0.5f;
        static const float ta[] = { 0.2f, 0.2f, 0.4f, 0.4f, 0.4f, 0.2f }, tr[] = { 300, 800, 2000, 5000, -1, -2 };
        const int k = juce::jlimit (0, 5, p.ratio);
        const float at = std::exp (-1.0f / (0.001f * ta[k] * 1.5f * (float) sr));
        const float avgC = std::exp (-1.0f / (1.5f * (float) sr)), avgC2 = std::exp (-1.0f / (0.8f * (float) sr));
        const float aLf = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 110.0f / (float) sr), aHf = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 7000.0f / (float) sr);
        for (int i = 0; i < N; ++i)
        {
            sm.inDb += (inT[0] - sm.inDb) * kS; sm.in2 += (inT[1] - sm.in2) * kS; sm.out += (p.outDb - sm.out) * kS; sm.mix += (p.mix - sm.mix) * kS; sm.col += (p.color - sm.col) * kS;
            sm.thr += (p.thr670 - sm.thr) * kS; pc[0].thr670 = sm.thr; if (p.link) pc[1].thr670 = sm.thr;
            const float inGc[2] = { juce::Decibels::decibelsToGain (sm.inDb), juce::Decibels::decibelsToGain (p.link ? sm.inDb : sm.in2) };
            const float og = juce::Decibels::decibelsToGain (sm.out), col = sm.col;
            float a = L[i] * inGc[0], b = R[i] * inGc[1], da = dL[i] * inGc[0], db = dR[i] * inGc[1];
            if (ms) { const float m = 0.7071f * (a + b), s2 = 0.7071f * (a - b); a = m; b = s2; const float dm = 0.7071f * (da + db), dsd = 0.7071f * (da - db); da = dm; db = dsd; }
            float lev[2] = { std::abs (da), std::abs (db) };
            if (! ms && p.link) lev[0] = lev[1] = juce::jmax (lev[0], lev[1]);   // L/R collegati
            for (int c = 0; c < 2; ++c)
            {
                const float tgt = gainComputer (pc[c], 20.0f * std::log10 (lev[c] + 1.0e-9f));
                const int kc = juce::jlimit (0, 5, pc[c].ratio); const float atc = kc == k ? at : std::exp (-1.0f / (0.001f * ta[kc] * 1.5f * (float) sr));
                float& G = g6[c];
                if (tr[kc] > 0) { const float rc = std::exp (-1.0f / (0.001f * tr[kc] * (float) sr)); G = tgt > G ? atc * G + (1 - atc) * tgt : rc * G + (1 - rc) * tgt; }
                else
                {   // TC5 e TC6: rilascio automatico a due (tre) tempi
                    const float r1 = tr[kc] == -1 ? 2.0f : 0.3f, r2 = tr[kc] == -1 ? 10.0f : 25.0f;
                    const float rf = std::exp (-1.0f / (r1 * (float) sr)), rs = std::exp (-1.0f / ((r1 + (r2 - r1) * c6[c]) * (float) sr));
                    f6[c] = tgt > f6[c] ? atc * f6[c] + (1 - atc) * tgt : rf * f6[c] + (1 - rf) * tgt;
                    s6[c] = tgt > s6[c] ? atc * s6[c] + (1 - atc) * tgt : rs * s6[c] + (1 - rs) * tgt;
                    G = 0.5f * (f6[c] + s6[c]);
                    c6[c] += ((G > 1 ? 1.0f : 0.0f) - c6[c]) * (1.0f / (6.0f * (float) sr));
                }
            }
            float ya = a * juce::Decibels::decibelsToGain (-g6[0]), yb = b * juce::Decibels::decibelsToGain (-g6[1]);
            if (ms) { const float l2 = 0.7071f * (ya + yb), r2 = 0.7071f * (ya - yb); ya = l2; yb = r2; }
            gr = juce::jmax (g6[0], g6[1]); grAvg = avgC * grAvg + (1 - avgC) * gr;
            // catena valvolare: trasformatore d'ingresso (satura i bassi), valvola con bias che si sposta con la riduzione (2a armonica),
            // trasformatore d'uscita con un filo di brillantezza
            float y[2] = { ya, yb };
            for (int c = 0; c < 2; ++c)
            {
                float x = y[c];
                lfLp[c] += (x - lfLp[c]) * aLf; const float lo = lfLp[c], kd = 1.0f + col * 2.5f;
                x += (std::tanh (lo * kd * 1.6f) / (kd * 1.6f) - lo) * 0.9f * (0.25f + 0.75f * col);
                const float bias = 0.04f + 0.22f * col + 0.012f * juce::jmin (gr, 15.0f), d = 1.0f + col * 1.4f;
                const float n = d * (1 - std::tanh (bias) * std::tanh (bias));
                const float tube = 1.4f * (std::tanh (d * x / 1.4f + bias) - std::tanh (bias)) / n, wt = 0.3f + 0.7f * col;
                x += (tube - x) * wt;
                bLp[c] += (x - bLp[c]) * 0.0005f; x -= bLp[c];
                hfLp[c] += (x - hfLp[c]) * aHf; x += (x - hfLp[c]) * (0.06f + 0.14f * col);
                y[c] = x;
            }
            if (deltaL) { deltaL[i] = L[i] * inGc[0] - y[0]; deltaR[i] = R[i] * inGc[1] - y[1]; }
            float yl = y[0], yr = y[1];
            eIn = avgC2 * eIn + (1 - avgC2) * 0.5f * (L[i] * L[i] + R[i] * R[i]);
            eOut = avgC2 * eOut + (1 - avgC2) * 0.5f * (yl * yl + yr * yr);
            if (autoGain && eIn > 1.0e-8f && eOut > 1.0e-10f) autoC += (juce::jlimit (0.06f, 16.0f, std::sqrt (eIn / eOut)) - autoC) * 0.0002f;
            else if (! autoGain) autoC = 1;
            yl *= og * autoC; yr *= og * autoC;
            L[i] = L[i] * (1 - sm.mix) + yl * sm.mix; R[i] = R[i] * (1 - sm.mix) + yr * sm.mix;
        }
    }

    // ---- multibanda: crossover Linkwitz-Riley 24 dB/ott con compensazione di fase, compressore per banda ----
    struct Bq2 { double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1[2] = {}, z2[2] = {};
        inline float run (float x, int c) { const double y = b0 * x + z1[c]; z1[c] = b1 * x - a1 * y + z2[c]; z2[c] = b2 * x - a2 * y; return (float) y; }
        void set (int type, double f, double sr) { const double w = juce::MathConstants<double>::twoPi * juce::jlimit (10.0, sr * 0.45, f) / sr, cs = std::cos (w), al = std::sin (w) / (2 * 0.70710678), a0 = 1 + al;
            if (type == 0) { b0 = (1 - cs) / 2 / a0; b1 = (1 - cs) / a0; b2 = b0; } else if (type == 1) { b0 = (1 + cs) / 2 / a0; b1 = -(1 + cs) / a0; b2 = b0; } else { b0 = (1 - al) / a0; b1 = -2 * cs / a0; b2 = (1 + al) / a0; }
            a1 = -2 * cs / a0; a2 = (1 - al) / a0; } };
    struct Split { Bq2 lp[2], hp[2], ap[3]; float f = -1; void set (float fr, double sr) { if (fr == f) return; f = fr; for (auto& q : lp) q.set (0, fr, sr); for (auto& q : hp) q.set (1, fr, sr); for (auto& q : ap) q.set (2, fr, sr); }
        inline void run (float x, int c, float& lo, float& hi) { lo = lp[1].run (lp[0].run (x, c), c); hi = hp[1].run (hp[0].run (x, c), c); } };
    Split xs[3]; float mEnv[4] {}, mG[4] {};
public:
    std::atomic<float> bandGr[4] {};
private:
    void processMb (float* L, float* R, const float* dL, const float* dR, int N, const StageParams& p, bool autoGain, float* deltaL, float* deltaR, float kS)
    {
        juce::ignoreUnused (dL, dR);
        static const MbParams def; const MbParams& M = p.mb ? *p.mb : def;
        const int nb = mbBands (p);
        // frequenze di taglio usate: 2 bande -> xo[1]; 3 -> xo[0], xo[2]; 4 -> tutte
        float fx[3]; int nx = nb - 1;
        if (nb == 2) fx[0] = M.xo[1]; else if (nb == 3) { fx[0] = M.xo[0]; fx[1] = M.xo[2]; } else { fx[0] = M.xo[0]; fx[1] = M.xo[1]; fx[2] = M.xo[2]; }
        for (int k = 1; k < nx; ++k) fx[k] = juce::jmax (fx[k], fx[k - 1] * 1.2f);
        for (int k = 0; k < nx; ++k) xs[k].set (fx[k], sr);
        const float off = mbOffset (p.drive);   // la manopola grande sposta tutte le soglie insieme
        const float tScale = std::pow (4.0f, (p.att - 0.5f) * 2.0f), rScale = std::pow (4.0f, (p.rel - 0.5f) * 2.0f);
        const float warm = p.color;   // "warm" stile alpha: attacco piu morbido e un filo di saturazione
        float ca[4], cr[4], th[4], sl[4], mk[4];
        bool anySolo = false; for (int b = 0; b < nb; ++b) anySolo |= M.solo[b];
        for (int b = 0; b < nb; ++b)
        {
            ca[b] = std::exp (-1.0f / (0.001f * M.att[b] * tScale * (1.0f + warm * 2.0f) * (float) sr));
            cr[b] = std::exp (-1.0f / (0.001f * M.rel[b] * rScale * (float) sr));
            th[b] = M.thr[b] + off; sl[b] = 1.0f - 1.0f / juce::jmax (1.0f, M.ratio[b]); mk[b] = juce::Decibels::decibelsToGain (M.gain[b]);
        }
        const float avgC = std::exp (-1.0f / (1.5f * (float) sr)), avgC2 = std::exp (-1.0f / (0.8f * (float) sr));
        float bandMax[4] {};
        for (int i = 0; i < N; ++i)
        {
            sm.out += (p.outDb - sm.out) * kS; sm.mix += (p.mix - sm.mix) * kS; sm.col += (warm - sm.col) * kS;
            const float og = juce::Decibels::decibelsToGain (sm.out);
            float band[4][2];
            for (int c = 0; c < 2; ++c)
            {
                const float x = c ? R[i] : L[i];
                float rest = x;
                for (int k = 0; k < nx; ++k)
                {
                    float lo, hi; xs[k].run (rest, c, lo, hi);
                    // le bande gia separate passano negli allpass dei tagli successivi: somma piatta
                    for (int j = 0; j < k; ++j) band[j][c] = xs[k].ap[j].run (band[j][c], c);
                    band[k][c] = lo; rest = hi;
                }
                band[nx][c] = rest;
            }
            float yl = 0, yr = 0, mx = 0;
            for (int b = 0; b < nb; ++b)
            {
                const float lev = juce::jmax (std::abs (band[b][0]), std::abs (band[b][1]));
                mEnv[b] = lev > mEnv[b] ? ca[b] * mEnv[b] + (1 - ca[b]) * lev : cr[b] * mEnv[b] + (1 - cr[b]) * lev;
                const float db = 20.0f * std::log10 (mEnv[b] + 1.0e-9f), o = db - th[b];
                const float g = o <= -3 ? 0.0f : o < 3 ? sl[b] * (o + 3) * (o + 3) / 12.0f : sl[b] * o;   // ginocchio di 6 dB
                mG[b] = g; mx = juce::jmax (mx, g); bandMax[b] = juce::jmax (bandMax[b], g);
                if (anySolo && ! M.solo[b]) continue;
                const float gg = M.byp[b] ? 1.0f : juce::Decibels::decibelsToGain (-g) * mk[b];
                yl += band[b][0] * gg; yr += band[b][1] * gg;
            }
            if (sm.col > 0.001f) sat (yl, 0, OPTO2A, sm.col * 0.5f), sat (yr, 1, OPTO2A, sm.col * 0.5f);
            gr = mx; grAvg = avgC * grAvg + (1 - avgC) * gr;
            if (deltaL) { deltaL[i] = L[i] - yl; deltaR[i] = R[i] - yr; }
            eIn = avgC2 * eIn + (1 - avgC2) * 0.5f * (L[i] * L[i] + R[i] * R[i]);
            eOut = avgC2 * eOut + (1 - avgC2) * 0.5f * (yl * yl + yr * yr);
            if (autoGain && eIn > 1.0e-8f && eOut > 1.0e-10f) autoC += (juce::jlimit (0.06f, 16.0f, std::sqrt (eIn / eOut)) - autoC) * 0.0002f;
            else if (! autoGain) autoC = 1;
            yl *= og * autoC; yr *= og * autoC;
            L[i] = L[i] * (1 - sm.mix) + yl * sm.mix; R[i] = R[i] * (1 - sm.mix) + yr * sm.mix;
        }
        for (int b = 0; b < 4; ++b) bandGr[b] = b < nb ? bandMax[b] : 0.0f;
    }
    float hfA = 0.3f;
    struct Sm { float inDb = 0, in2 = 0, out = 0, mix = 1, col = 0, drive = 0, thr = 0; } sm; bool smInit = false; float dHp[2] {};
    // DISTRESS: DIST 2 = seconda armonica (asimmetrica, tipo valvola/nastro), DIST 3 = terza (simmetrica), 2+3 entrambe
    void satDist (float& y, int c, int mode, float amt)
    {
        if (amt < 0.001f || mode == 0) return;
        constexpr float ref = 1.4f; const float x = y / ref;
        float z = x;
        if (mode == 1 || mode == 3) { const float b = 0.35f * amt, d = 1.0f + 1.6f * amt; z = (std::tanh (d * z + b) - std::tanh (b)) / (d * (1 - std::tanh (b) * std::tanh (b))); }
        if (mode == 2 || mode == 3) { const float d = 1.0f + 2.2f * amt; z = std::tanh (d * z) / d; }
        z *= ref;
        dc[c] += (z - dc[c]) * 0.0005f; y = z - (mode != 2 ? dc[c] : 0.0f);
    }
    double sr = 44100; float gr = 0, fast = 0, slow = 0, charge = 0, rms = 0, eIn = 0, eOut = 0, autoC = 1; float hpS[2] {}; float dc[2] {};
    void sat (float& y, int c, int m, float col)
    {
        if (col < 0.001f) return;
        // curve normalizzate: guadagno 1 sui segnali piccoli, saturazione che parte intorno a -6 dBFS
        constexpr float ref = 1.4f;
        auto asym = [] (float x, float d, float b) { x /= ref; const float n = d * (1 - std::tanh (b) * std::tanh (b)); return ref * (std::tanh (d * x + b) - std::tanh (b)) / n; };
        auto odd = [] (float x, float d) { x /= ref; return ref * std::tanh (d * x) / d; };
        float z = y;
        switch (m)
        {
            case OPTO2A: z = asym (y, 1.0f + col * 1.6f, 0.25f * col); break;
            case OPTO3A: z = odd (y, 1.0f + col * 1.8f); break;
            case FET76:  z = odd (y, 1.0f + col * (1.2f + gr * 0.06f)); break;
            case SSLCH:  z = odd (y, 1.0f + col * 0.9f); break;
            case VCABUS: z = asym (y, 1.0f + col * 1.0f, 0.1f * col); break;
            case VARIMU: z = asym (y, 1.0f + col * 2.0f, 0.35f * col); break;
            case RMS160: z = odd (y, 1.0f + col * 1.5f); break;
            case OTT: z = odd (y, 1.0f + col * 1.5f); break;
            case DISTRESS: z = col < 0.5f ? asym (y, 1.0f + col * 3.0f, 0.5f * col) : odd (y, 1.0f + (col - 0.3f) * 4.0f); break;
            default: break;
        }
        // blocco della continua
        dc[c] += (z - dc[c]) * 0.0005f; y = z - (m == OPTO2A || m == VARIMU || m == VCABUS || (m == DISTRESS && col < 0.5f) ? dc[c] : 0.0f);
    }
};
}
