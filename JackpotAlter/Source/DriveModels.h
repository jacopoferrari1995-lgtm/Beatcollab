#pragma once
#include <juce_dsp/juce_dsp.h>
#include <cmath>

// Modelli di saturazione e distorsione di Jackpot Alter Drive.
// Lavorano sovracampionati (4x). Ogni modello e un piccolo circuito: filtri prima e dopo + non linearita.
namespace ja
{
// --------------------------------------------------------------------------- biquad (RBJ, forma trasposta II)
struct BQ
{
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    inline float operator() (float x) { const float y = b0 * x + z1; z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y; return y; }
    void reset() { z1 = z2 = 0; }
    void set (double B0, double B1, double B2, double A0, double A1, double A2) { b0 = (float) (B0 / A0); b1 = (float) (B1 / A0); b2 = (float) (B2 / A0); a1 = (float) (A1 / A0); a2 = (float) (A2 / A0); }
    void lp (double fs, double f, double q = 0.7071) { const double w = 2 * juce::MathConstants<double>::pi * juce::jmin (f, fs * 0.45) / fs, c = std::cos (w), al = std::sin (w) / (2 * q); set ((1 - c) / 2, 1 - c, (1 - c) / 2, 1 + al, -2 * c, 1 - al); }
    void hp (double fs, double f, double q = 0.7071) { const double w = 2 * juce::MathConstants<double>::pi * juce::jmin (f, fs * 0.45) / fs, c = std::cos (w), al = std::sin (w) / (2 * q); set ((1 + c) / 2, -(1 + c), (1 + c) / 2, 1 + al, -2 * c, 1 - al); }
    void peak (double fs, double f, double q, double db)
    {
        const double A = std::pow (10.0, db / 40), w = 2 * juce::MathConstants<double>::pi * juce::jmin (f, fs * 0.45) / fs, c = std::cos (w), al = std::sin (w) / (2 * q);
        set (1 + al * A, -2 * c, 1 - al * A, 1 + al / A, -2 * c, 1 - al / A);
    }
    void shelf (double fs, double f, double db, bool high)
    {
        const double A = std::pow (10.0, db / 40), w = 2 * juce::MathConstants<double>::pi * juce::jmin (f, fs * 0.45) / fs, c = std::cos (w), s = std::sin (w), al = s / 2 * std::sqrt (2.0), sa = 2 * std::sqrt (A) * al;
        if (high) set (A * ((A + 1) + (A - 1) * c + sa), -2 * A * ((A - 1) + (A + 1) * c), A * ((A + 1) + (A - 1) * c - sa), (A + 1) - (A - 1) * c + sa, 2 * ((A - 1) - (A + 1) * c), (A + 1) - (A - 1) * c - sa);
        else      set (A * ((A + 1) - (A - 1) * c + sa), 2 * A * ((A - 1) - (A + 1) * c), A * ((A + 1) - (A - 1) * c - sa), (A + 1) + (A - 1) * c + sa, -2 * ((A - 1) + (A + 1) * c), (A + 1) + (A - 1) * c - sa);
    }
};
struct OnePole { float a = 0, z = 0; void lp (double fs, double f) { a = (float) std::exp (-2 * juce::MathConstants<double>::pi * f / fs); } inline float operator() (float x) { z = x + a * (z - x); return z; } void reset() { z = 0; } };

enum Model { TAPE, VALVOLA, TRASFORMATORE, CONSOLE, CLIPPER, OVERDRIVE, DISTORSIONE, FUZZ, OTTAVA, AMP, FOLD, LOFI, RADIO, kNumModels };

struct ModelInfo { const char* name; const char* sub; const char* what; const char* use; float maxDb; juce::uint32 colour; };
inline const ModelInfo& modelInfo (int m)
{
    static const ModelInfo I[kNumModels] = {
        { "TAPE", "nastro 15 ips", "Registratore a nastro: pre-enfasi sugli acuti, saturazione morbida del nastro con un filo di 2a armonica, head bump sui bassi e alti arrotondati.", "Voce, bus, 808 che deve stare nel mix senza graffiare.", 30, 0xffffb000 },
        { "VALVOLA", "triodo 12AX7", "Stadio a triodo in classe A: curva asimmetrica (armoniche pari), la polarizzazione si sposta coi picchi come una valvola vera.", "Voce calda, basso, synth. Il classico 'calore'.", 30, 0xffff7a3d },
        { "TRASFORMATORE", "ferro e nucleo", "Trasformatore d'uscita da preamp britannico: il ferro satura soprattutto sui bassi, gli acuti restano puliti.", "Basso e 808 piu grossi, voce piena, kick.", 24, 0xffa66bff },
        { "CONSOLE", "canale VCA", "Canale di console a VCA/op-amp: pulito finche non arriva al limite, poi armoniche dispari e ginocchio duro.", "Batteria, bus, dare densita senza colore.", 30, 0xff5ef2ff },
        { "CLIPPER", "taglio duro", "Clipper digitale sovracampionato: taglia i picchi netto. Molto volume, poca colorazione finche non esageri.", "808 e kick trap, alzare il volume dei bassi.", 24, 0xffff3b5c },
        { "OVERDRIVE", "pedale verde", "Overdrive stile pedale verde: spinge le medie (taglio sotto 720 Hz prima del clipping) e satura morbido coi diodi in controreazione.", "Chitarra crunch, voce sporca, basso che si sente sui telefoni.", 18, 0xff7dff5a },
        { "DISTORSIONE", "op-amp + diodi", "Distorsione da pedale con op-amp ad alto guadagno e diodi a massa: aggressiva, compressa, scura quando spingi.", "Chitarra rock, basso distorto, voce estrema.", 18, 0xffff2e88 },
        { "FUZZ", "transistor al germanio", "Fuzz a due transistor: clipping asimmetrico, si sfalda quando il segnale cala, tanto sustain.", "Chitarra e basso fuzz, synth sporchi.", 24, 0xffffd23d },
        { "OTTAVA", "fuzz + raddrizzatore", "Fuzz con raddrizzatore a onda intera: aggiunge l'ottava sopra, suono metallico e nasale.", "Assoli, basso synth, effetti.", 24, 0xffb4ff3d },
        { "AMP", "testata + cassa 4x12", "Testata britannica in crunch (tre stadi a valvola, tone stack) con simulazione di cassa 4x12.", "Chitarra senza amp, voce lo-fi da megafono rock.", 30, 0xffff9a3d },
        { "FOLD", "wavefolder", "Ripiegatore d'onda da synth modulare: invece di tagliare ripiega l'onda, armoniche brillanti e metalliche.", "808 e bassi synth che cantano, effetti.", 22, 0xff3dffd2 },
        { "LO-FI", "bit e campionamento", "Riduce bit e frequenza di campionamento come un campionatore vecchio: polvere, aliasing, grana.", "Voce lo-fi, batteria vintage, texture.", 24, 0xff9aa4b8 },
        { "RADIO", "radiolina", "Banda stretta da altoparlante piccolo (400 Hz - 3 kHz) con saturazione: radio, telefono, megafono.", "Voce in intro e bridge, effetti, adlib.", 30, 0xff8ad1ff } };
    return I[juce::jlimit (0, kNumModels - 1, m)];
}

// --------------------------------------------------------------------------- un canale di distorsione (al rate sovracampionato)
class DriveChannel
{
public:
    void prepare (double osRate, int osFactor) { fs = osRate; osf = osFactor; model = -1; reset(); }
    void reset()
    {
        for (auto* b : { &f1, &f2, &f3, &f4, &f5, &f6, &f7, &f8 }) b->reset();
        for (auto* o : { &p1, &p2, &p3 }) o->reset();
        dc1 = dc2 = 0; env = 0; flux = 0; fluxS = 0; hold = 0; holdN = 0; mag = 0;
    }
    void setModel (int m)
    {
        if (m == model) return;
        model = m; reset();
        switch (m)
        {
            case TAPE:      f1.shelf (fs, 3000, 4, true); f2.shelf (fs, 3000, -4.5, true); f3.peak (fs, 75, 0.9, 2.5); f4.lp (fs, 17000); f5.hp (fs, 25); break;
            case VALVOLA:   f1.lp (fs, 14000); f2.hp (fs, 25); break;
            case TRASFORMATORE: f1.lp (fs, 20000); f2.hp (fs, 18); break;
            case CONSOLE:   f1.hp (fs, 12); f2.lp (fs, 22000); break;
            case CLIPPER:   f1.hp (fs, 8); break;
            case OVERDRIVE: f1.hp (fs, 720, 0.6); f2.hp (fs, 35); f3.lp (fs, 3400, 0.6); f4.lp (fs, 7500); break;
            case DISTORSIONE: f1.hp (fs, 60); f3.lp (fs, 2900, 0.6); f4.lp (fs, 9000); break;
            case FUZZ:      f1.hp (fs, 110); f2.lp (fs, 6500); f3.hp (fs, 30); break;
            case OTTAVA:    f1.hp (fs, 140); f2.lp (fs, 5500); f3.hp (fs, 40); break;
            case AMP:       f1.hp (fs, 80); f2.shelf (fs, 1500, 4, true); f3.peak (fs, 600, 0.7, -5); f4.shelf (fs, 3000, 3, true);
                            f5.hp (fs, 90, 0.8); f6.peak (fs, 120, 1.0, 4); f7.peak (fs, 2200, 1.2, 3); f8.lp (fs, 4800, 0.8); p2.lp (fs, 6500); break;
            case FOLD:      f1.hp (fs, 10); f2.lp (fs, 16000); break;
            case LOFI:      f1.lp (fs, 9000); f2.hp (fs, 20); break;
            case RADIO:     f1.hp (fs, 420, 0.9); f2.hp (fs, 420, 0.9); f3.lp (fs, 3000, 0.9); f4.lp (fs, 3000, 0.9); f5.peak (fs, 1500, 1.4, 4); break;
            default: break;
        }
        // costanti di inviluppo (polarizzazione / sag)
        envA = (float) std::exp (-1.0 / (0.004 * fs)); envR = (float) std::exp (-1.0 / (0.09 * fs));
        dcA = (float) std::exp (-2 * juce::MathConstants<double>::pi * 12.0 / fs);
        kint = (float) (2 * juce::MathConstants<double>::pi * 90.0 / fs); leak = 1.0f - kint * 0.15f;
        lastD = -1;
    }
    // parametri che dipendono dal drive (una volta per blocco)
    void setDrive (float d)
    {
        if (std::abs (d - lastD) < 1e-4f) return;
        lastD = d;
        if (model == DISTORSIONE) { const double gain = 1.0 + 1500.0 * d * d * d; p1.lp (fs, juce::jlimit (2500.0, 25000.0, 1.0e6 / gain)); }   // limite di banda dell'op-amp
    }

    // x gia moltiplicato per il guadagno di ingresso g (drive)
    inline float process (float x, float g, float d)
    {
        switch (model)
        {
            case TAPE:
            {
                float u = f1 (x) * g * 0.8f;
                float y = std::tanh (u + 0.08f) - 0.0798f;                 // un filo di asimmetria (bias del nastro)
                y = f2 (y); y = f3 (y); y = f4 (y); return f5 (y) * 1.25f;
            }
            case VALVOLA:
            {
                float u = f1 (x) * g;
                const float a = std::abs (u); env = a > env ? envA * env + (1 - envA) * a : envR * env + (1 - envR) * a;
                const float k = 0.35f, bias = -0.22f * juce::jmin (env, 3.0f) * d;   // la polarizzazione scivola coi picchi
                float y = std::tanh (u + k + bias) - std::tanh (k + bias);
                return f2 (dcBlock (y)) * 1.3f;
            }
            case TRASFORMATORE:
            {
                // flusso = integrale della tensione: il nucleo satura prima sui bassi
                flux = leak * flux + kint * x * g;
                const float fsat = std::tanh (flux * 1.6f) / 1.6f;
                float y = (fsat - leak * fluxS) / kint; fluxS = fsat;
                y = std::tanh (y * 0.9f + 0.05f) / 0.9f - 0.0555f;          // stadio in classe A dopo il ferro
                return f2 (f1 (y));
            }
            case CONSOLE:
            {
                const float u = f1 (x) * g;
                const float y = u / std::pow (1.0f + std::pow (std::abs (u), 4.0f), 0.25f);
                return f2 (y);
            }
            case CLIPPER:
            {
                const float u = f1 (x) * g;
                const float a = std::abs (u), knee = 0.92f;
                float y = a < knee ? a : (a < 2 - knee ? knee + (a - knee) - (a - knee) * (a - knee) / (4 * (1 - knee)) : 1.0f);
                return std::copysign (juce::jmin (y, 1.0f), u);
            }
            case OVERDRIVE:
            {
                const float tsg = 1.0f + 118.0f * d * d;                    // guadagno del pedale
                const float v = f2 (x) * g * 0.35f;
                const float u = v + (tsg - 1) * f1 (v);                     // il guadagno vale solo sopra ~720 Hz
                float y = u / std::sqrt (1.0f + u * u);                     // diodi in controreazione: morbido
                y = f3 (y + 0.15f * v); return f4 (y) * 1.4f;
            }
            case DISTORSIONE:
            {
                const float gain = 1.0f + 1500.0f * d * d * d;
                const float v = p1 (f1 (x) * g * 0.25f * gain / (1.0f + 0.02f * gain));
                const float y = v / std::pow (1.0f + std::pow (std::abs (v), 6.0f), 1.0f / 6.0f);   // diodi a massa: duro
                return f4 (f3 (y)) * 1.6f;
            }
            case FUZZ:
            {
                const float v = f1 (x) * g * (1.0f + 20.0f * d);
                const float a = std::abs (v); env = a > env ? envA * env + (1 - envA) * a : envR * env + (1 - envR) * a;
                const float bias = 0.25f - 0.2f * juce::jmin (1.0f, env);   // si sfalda quando il segnale scende
                float y = v > -bias ? std::tanh (2.2f * (v + bias)) - std::tanh (2.2f * bias) : -std::tanh (0.7f * (-v - bias)) * 0.75f - std::tanh (2.2f * bias);
                y = juce::jlimit (-0.9f, 0.9f, y * 1.4f);
                return f3 (f2 (dcBlock (y)));
            }
            case OTTAVA:
            {
                const float v = f1 (x) * g * (1.0f + 12.0f * d);
                const float fz = std::tanh (2.0f * v);
                const float r = std::abs (fz) * 2.0f;                       // raddrizzatore: ottava sopra
                return f3 (f2 (dcBlock (0.75f * r + 0.35f * fz)));
            }
            case AMP:
            {
                float u = f2 (f1 (x)) * g * 0.6f;
                u = std::tanh (u + 0.3f) - 0.2913f;                         // V1
                u = f4 (f3 (u)) * (1.0f + 6.0f * d);
                u = std::tanh (u * 1.4f + 0.25f) - 0.2449f;                 // V2
                const float a = std::abs (u); env = a > env ? envA * env + (1 - envA) * a : envR * env + (1 - envR) * a;
                u = std::tanh (u * (2.2f - 0.6f * juce::jmin (env, 1.0f)));      // finale push-pull con sag
                u = p2 (dcBlock (u));
                return f8 (f7 (f6 (f5 (u)))) * 2.2f;                        // cassa 4x12
            }
            case FOLD:
            {
                const float u = f1 (x) * g;
                const float y = std::sin (u * 1.5707963f);
                return f2 (y) * 0.9f;
            }
            case LOFI:
            {
                // trattiene il campione (frequenza ridotta) e riduce i bit
                const int factor = osf * (1 + (int) (d * d * 14.0f));
                if (holdN <= 0) { hold = x * g; holdN = factor; } --holdN;
                const float bits = 16.0f - 12.0f * d, q = std::pow (2.0f, bits - 1);
                float y = std::round (std::tanh (hold) * q) / q;
                return f2 (f1 (y));
            }
            case RADIO:
            {
                float u = f4 (f3 (f2 (f1 (x)))) * g * 1.4f;
                u = f5 (u);
                return std::tanh (u) * 0.9f;
            }
            default: return x;
        }
    }
private:
    inline float dcBlock (float x) { const float y = x - dc1 + dcA * dc2; dc1 = x; dc2 = y; return y; }
    double fs = 176400; int osf = 4; int model = -1;
    BQ f1, f2, f3, f4, f5, f6, f7, f8; OnePole p1, p2, p3;
    float kint = 0.003f, leak = 0.999f, lastD = -1; float dc1 = 0, dc2 = 0, dcA = 0.999f, env = 0, envA = 0.99f, envR = 0.999f, flux = 0, fluxS = 0, hold = 0, mag = 0; int holdN = 0;
};
}
