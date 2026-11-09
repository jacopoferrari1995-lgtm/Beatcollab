#pragma once
// Jackpot Studio Comp: motore dei compressori (emulazioni di comportamento, zero latenza)
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

namespace jc
{
enum Model { OPTO2A = 0, OPTO3A, FET76, VCABUS, VARIMU, RMS160, DISTRESS, kNumModels };

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
        { "OPTO 2A", "ispirato al Teletronix LA-2A", "PEAK RED.", false, false, false, { "COMPRESS", "LIMIT" }, "MODO", 0xffe9dfc6, 0xffffb000 },
        { "OPTO 3A", "ispirato all'Urei LA-3A", "PEAK RED.", false, false, false, { "COMPRESS", "LIMIT" }, "MODO", 0xffd8d2c4, 0xffff8a3d },
        { "FET 76", "ispirato all'Urei 1176", "INPUT", true, true, true, { "4", "8", "12", "20", "ALL" }, "RATIO", 0xff16181d, 0xff5ef2ff },
        { "VCA BUS", "ispirato al bus comp SSL G", "THRESH", false, true, true, { "2", "4", "10" }, "RATIO", 0xff1d2a24, 0xff7dff5a },
        { "VARI-MU", "ispirato a Fairchild 670 / Manley", "INPUT", true, false, false, { "TC1", "TC2", "TC3", "TC4", "TC5", "TC6" }, "TIME CONST.", 0xff2a2418, 0xffffd23f },
        { "RMS 160", "ispirato al dbx 160", "THRESH", false, false, false, { "2", "3", "4", "6", "10", "INF" }, "RATIO", 0xff121418, 0xffa66bff },
        { "DISTRESS", "ispirato all'Empirical Labs Distressor", "INPUT", true, true, true, { "1", "2", "3", "4", "6", "10", "20", "NUKE" }, "RATIO", 0xff10223a, 0xff2e8bff } };
    return I[juce::jlimit (0, kNumModels - 1, m)];
}

struct StageParams
{
    bool on = true; int model = 0; float drive = 0.3f; int ratio = 0; float att = 0.5f, rel = 0.5f, outDb = 0, color = 0.3f, mix = 1;
};

// valori mostrati a schermo
inline juce::String fmtDrive (int m, float d) { return info (m).fixedThreshold ? "+" + juce::String (d * 36.0f, 1) + " dB" : (m <= OPTO3A ? juce::String (juce::roundToInt (d * 100)) : juce::String (-d * 40.0f, 1) + " dB"); }
inline float attMs (int m, float a)
{
    switch (m)
    {
        case FET76: return 0.02f * std::pow (40.0f, a);                  // 20 us .. 800 us
        case VCABUS: { static const float t[] = { 0.1f, 0.3f, 1, 3, 10, 30 }; return t[juce::jlimit (0, 5, (int) (a * 5.99f))]; }
        case DISTRESS: return 0.05f * std::pow (600.0f, a);              // 0.05 .. 30 ms
        case OPTO2A: return 10.0f;
        case OPTO3A: return 1.5f;
        default: return 5.0f;
    }
}
inline float relMs (int m, float r)   // -1 = auto
{
    switch (m)
    {
        case FET76: return 50.0f * std::pow (22.0f, r);                  // 50 .. 1100 ms
        case VCABUS: { static const float t[] = { 100, 300, 600, 1200, -1 }; return t[juce::jlimit (0, 4, (int) (r * 4.99f))]; }
        case DISTRESS: return 50.0f * std::pow (70.0f, r);               // 50 .. 3500 ms
        default: return -1;
    }
}
inline juce::String fmtTime (float ms) { return ms < 0 ? "AUTO" : ms < 1 ? juce::String (juce::roundToInt (ms * 1000)) + " us" : ms < 10 ? juce::String (ms, 1) + " ms" : ms < 1000 ? juce::String (juce::roundToInt (ms)) + " ms" : juce::String (ms / 1000, 2) + " s"; }

// curva statica: riduzione di guadagno (dB positivi) per un livello in dB al rilevatore
inline float gainComputer (const StageParams& p, float levDb)
{
    float T = -18, R = 4, W = 6;
    switch (p.model)
    {
        case OPTO2A: T = -p.drive * 40.0f; R = p.ratio == 0 ? 3.0f : 12.0f; W = p.ratio == 0 ? 14.0f : 6.0f; break;
        case OPTO3A: T = -p.drive * 40.0f; R = p.ratio == 0 ? 3.0f : 10.0f; W = p.ratio == 0 ? 10.0f : 5.0f; break;
        case FET76: { static const float r[] = { 4, 8, 12, 20, 20 }; const int k = juce::jlimit (0, 4, p.ratio); R = r[k]; T = k == 0 ? -16.0f : -12.0f; W = k == 0 ? 6.0f : 2.0f; break; }
        case VCABUS: { static const float r[] = { 2, 4, 10 }; R = r[juce::jlimit (0, 2, p.ratio)]; T = -p.drive * 40.0f; W = R <= 2 ? 6.0f : 2.0f; break; }
        case VARIMU: { T = -12; W = 24; const float over = juce::jmax (0.0f, levDb - T); R = 1.5f + over / 5.0f; R = juce::jmin (R, 8.0f); break; }
        case RMS160: { static const float r[] = { 2, 3, 4, 6, 10, 60 }; R = r[juce::jlimit (0, 5, p.ratio)]; T = -p.drive * 40.0f; W = 0.5f; break; }
        case DISTRESS: { static const float r[] = { 1, 2, 3, 4, 6, 10, 20, 100 }; R = r[juce::jlimit (0, 7, p.ratio)]; T = -18; W = R <= 3 ? 10.0f : R < 20 ? 4.0f : 1.0f; break; }
        default: break;
    }
    const float over = levDb - T, s = 1.0f - 1.0f / R;
    if (2 * over < -W) return 0;
    if (2 * std::abs (over) <= W) return s * (over + W / 2) * (over + W / 2) / (2 * W);
    float gr = s * over;
    if (p.model == FET76 && p.ratio == 4) gr *= 1.15f;    // "all buttons": eccesso tipico
    return gr;
}
inline float inputGainDb (const StageParams& p) { return info (p.model).fixedThreshold ? p.drive * 36.0f : 0.0f; }

class Stage
{
public:
    void prepare (double s) { sr = s; reset(); }
    void reset() { gr = fast = slow = charge = rms = 0; grAvg = 0; eIn = eOut = 0; autoC = 1; hpS[0] = hpS[1] = 0; dc[0] = dc[1] = 0; }
    float grNow() const { return gr; }
    float grAvg = 0;

    // L/R in place; dL/dR = segnale del rilevatore (gia filtrato), stessa lunghezza
    void process (float* L, float* R, const float* dL, const float* dR, int N, const StageParams& p, bool autoGain)
    {
        const auto& I = info (p.model);
        const float inG = juce::Decibels::decibelsToGain (inputGainDb (p));
        const float a = std::exp (-1.0f / (0.001f * attMs (p.model, p.att) * (float) sr));
        const float rm = relMs (p.model, p.rel);
        const float rFixed = rm > 0 ? std::exp (-1.0f / (0.001f * rm * (float) sr)) : 0.0f;
        const float avgC = std::exp (-1.0f / (1.5f * (float) sr)), avgC2 = std::exp (-1.0f / (0.8f * (float) sr));
        const float rmsC = std::exp (-1.0f / (0.006f * (float) sr));
        const float col = p.color;
        for (int i = 0; i < N; ++i)
        {
            const float xl = L[i] * inG, xr = R[i] * inG;
            // rilevatore
            float lev;
            if (p.model == RMS160 || p.model == VCABUS)
            {
                const float e = 0.5f * (dL[i] * dL[i] + dR[i] * dR[i]) * inG * inG;
                rms = rmsC * rms + (1 - rmsC) * e;
                lev = p.model == RMS160 ? std::sqrt (rms) * 1.414f : juce::jmax (std::sqrt (rms) * 1.414f, juce::jmax (std::abs (dL[i]), std::abs (dR[i])) * inG * 0.8f);
            }
            else lev = juce::jmax (std::abs (dL[i]), std::abs (dR[i])) * inG;
            const float levDb = 20.0f * std::log10 (lev + 1.0e-9f);
            const float tgt = gainComputer (p, levDb);
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
                case RMS160:
                {
                    if (tgt > gr) { const float ms = juce::jlimit (3.0f, 15.0f, 150.0f / juce::jmax (10.0f, tgt - gr)); const float c = std::exp (-1.0f / (0.001f * ms * (float) sr)); gr = c * gr + (1 - c) * tgt; }
                    else gr = juce::jmax (tgt, gr - 120.0f / (float) sr);   // 120 dB/s
                    break;
                }
                case VARIMU:
                {
                    static const float ta[] = { 0.2f, 0.2f, 0.4f, 0.4f, 0.4f, 0.2f }, tr[] = { 300, 800, 2000, 5000, -1, -2 };
                    const int k = juce::jlimit (0, 5, p.ratio);
                    const float at = std::exp (-1.0f / (0.001f * ta[k] * 4.0f * (float) sr));   // la valvola smussa un po' l'attacco
                    if (tr[k] > 0) { const float rc = std::exp (-1.0f / (0.001f * tr[k] * (float) sr)); gr = tgt > gr ? at * gr + (1 - at) * tgt : rc * gr + (1 - rc) * tgt; }
                    else
                    {
                        const float f1 = tr[k] == -1 ? 2.0f : 0.3f, f2 = tr[k] == -1 ? 10.0f : 25.0f;
                        const float rf = std::exp (-1.0f / (f1 * (float) sr)), rs = std::exp (-1.0f / ((f1 + (f2 - f1) * charge) * (float) sr));
                        fast = tgt > fast ? at * fast + (1 - at) * tgt : rf * fast + (1 - rf) * tgt;
                        slow = tgt > slow ? at * slow + (1 - at) * tgt : rs * slow + (1 - rs) * tgt;
                        gr = 0.5f * (fast + slow);
                        charge += ((gr > 1 ? 1.0f : 0.0f) - charge) * (1.0f / (6.0f * (float) sr));
                    }
                    break;
                }
                default:
                {
                    if (tgt > gr) gr = a * gr + (1 - a) * tgt;
                    else if (rm > 0)
                    {
                        float rc = rFixed;
                        if (p.model == FET76) rc = std::exp (-1.0f / (0.001f * rm * (0.6f + 0.4f * juce::jmin (1.0f, gr / 10.0f)) * (float) sr));
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
            const float og = juce::Decibels::decibelsToGain (p.outDb);
            float yl = xl * g, yr = xr * g;
            // auto gain: allinea l'energia d'uscita a quella d'ingresso (media su ~1 s)
            eIn = avgC2 * eIn + (1 - avgC2) * 0.5f * (L[i] * L[i] + R[i] * R[i]);
            eOut = avgC2 * eOut + (1 - avgC2) * 0.5f * (yl * yl + yr * yr);
            if (autoGain && eIn > 1.0e-8f && eOut > 1.0e-10f) autoC += (juce::jlimit (0.06f, 16.0f, std::sqrt (eIn / eOut)) - autoC) * 0.0002f;
            else if (! autoGain) autoC = 1;
            yl *= og * autoC; yr *= og * autoC;
            // colore: lo stadio d'uscita satura in modo diverso per ogni modello (riferimento a 0 dBFS)
            sat (yl, 0, p.model, col); sat (yr, 1, p.model, col);
            L[i] = L[i] * (1 - p.mix) + yl * p.mix; R[i] = R[i] * (1 - p.mix) + yr * p.mix;
        }
        juce::ignoreUnused (I);
    }

private:
    double sr = 44100; float gr = 0, fast = 0, slow = 0, charge = 0, rms = 0, eIn = 0, eOut = 0, autoC = 1; float hpS[2]; float dc[2];
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
            case VCABUS: z = asym (y, 1.0f + col * 1.0f, 0.1f * col); break;
            case VARIMU: z = asym (y, 1.0f + col * 2.0f, 0.35f * col); break;
            case RMS160: z = odd (y, 1.0f + col * 1.5f); break;
            case DISTRESS: z = col < 0.5f ? asym (y, 1.0f + col * 3.0f, 0.5f * col) : odd (y, 1.0f + (col - 0.3f) * 4.0f); break;
            default: break;
        }
        // blocco della continua
        dc[c] += (z - dc[c]) * 0.0005f; y = z - (m == OPTO2A || m == VARIMU || m == VCABUS || (m == DISTRESS && col < 0.5f) ? dc[c] : 0.0f);
    }
};
}
