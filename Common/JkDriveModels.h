#pragma once
#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include <vector>

// Modelli di saturazione e distorsione di Jackpot Alter Drive.
// Lavorano sovracampionati (4x). Ogni modello e un piccolo circuito: filtri prima e dopo + non linearita.
namespace ja
{
// --------------------------------------------------------------------------- biquad (RBJ, forma trasposta II)
struct BQ
{
    // coefficienti e stato in double: a 8x i filtri molto bassi (10-30 Hz) in float fanno rumore e modulazione
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    inline float operator() (float xf) { const double x = xf, y = b0 * x + z1; z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y; return (float) y; }
    void reset() { z1 = z2 = 0; }
    void set (double B0, double B1, double B2, double A0, double A1, double A2) { b0 = B0 / A0; b1 = B1 / A0; b2 = B2 / A0; a1 = A1 / A0; a2 = A2 / A0; }
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

// antiderivata anti-aliasing (ADAA di primo ordine): al posto di f(x) si usa la media di f tra il campione prima e questo,
// (F(x) - F(x1)) / (x - x1). Toglie gran parte dell'aliasing dei clipper duri senza alzare il sovracampionamento.
inline float logcosh (float x) { const float a = std::abs (x); return a + std::log1p (std::exp (-2.0f * a)) - 0.6931472f; }
// integrale della curva CONSOLE u / (1 + u^4)^(1/4): tabella calcolata una volta (non ha una forma semplice), oltre 16 quasi una retta
inline float consoleF (float x)
{
    static const std::vector<double> T = []
    {
        std::vector<double> t (8193, 0.0); const double h = 16.0 / 8192.0; double acc = 0;
        auto f = [] (double u) { return u / std::pow (1.0 + u * u * u * u, 0.25); };
        for (int i = 1; i <= 8192; ++i) { const double a = (i - 1) * h, b = i * h; acc += h / 6.0 * (f (a) + 4.0 * f (0.5 * (a + b)) + f (b)); t[(size_t) i] = acc; }
        return t;
    }();
    const double a = std::abs ((double) x);
    if (a >= 16.0) return (float) (T[8192] + (a - 16.0));
    const double p = a / 16.0 * 8192.0; const int i = (int) p; const double fr = p - i;
    return (float) (T[(size_t) i] + (T[(size_t) juce::jmin (8192, i + 1)] - T[(size_t) i]) * fr);
}
struct Adaa
{
    float x1 = 0, F1 = 0; bool init = false;
    void reset() { x1 = 0; F1 = 0; init = false; }
    template <typename Fa, typename Ff> inline float run (float x, Fa&& F, Ff&& f)
    {
        const float Fx = F (x);
        if (! init) { init = true; x1 = x; F1 = Fx; return f (x); }
        const float dx = x - x1; float y = std::abs (dx) > 1.0e-4f ? (Fx - F1) / dx : f (0.5f * (x + x1));
        x1 = x; F1 = Fx; return y;
    }
};

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
        for (auto* a : { &ad1, &ad2, &ad3 }) a->reset();
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
        if (model == DISTORSIONE) { const double gain = 1.0 + 1500.0 * d * d * d; p1.lp (fs, juce::jlimit (2500.0, 25000.0, 1.0e6 / gain)); distG = (float) (0.25 * gain / (1.0 + 0.02 * gain)); }   // limite di banda dell'op-amp
        tsg = 1.0f + 118.0f * d * d;                                    // guadagno del pedale verde
        lofiFactor = osf * (1 + (int) (d * d * 14.0f)); lofiQ = std::pow (2.0f, 15.0f - 12.0f * d);
    }

    // core: la parte non lineare (per banda); post: i filtri lineari dopo (si possono fare una volta sola sulla somma delle bande)
    inline float process (float x, float g, float d) { return post (core (x, g, d)); }
    inline float core (float x, float g, float d)
    {
        switch (model)
        {
            case TAPE:      { const float u = f1 (x) * g * 0.8f; return ad1.run (u, [] (float q) { return logcosh (q + 0.08f) - 0.0798f * q; }, [] (float q) { return std::tanh (q + 0.08f) - 0.0798f; }); }   // un filo di asimmetria (bias del nastro), con antiderivata
            case VALVOLA:
            {
                const float u = f1 (x) * g;
                const float a = std::abs (u); env = a > env ? envA * env + (1 - envA) * a : envR * env + (1 - envR) * a;
                const float k = 0.35f, bias = -0.22f * juce::jmin (env, 3.0f) * d;   // la polarizzazione scivola coi picchi
                const float kb = k + bias, T = std::tanh (kb);
                return ad1.run (u, [kb, T] (float q) { return logcosh (q + kb) - T * q; }, [kb, T] (float q) { return std::tanh (q + kb) - T; });
            }
            case TRASFORMATORE:
            {
                // flusso = integrale della tensione: il nucleo satura prima sui bassi
                flux = leak * flux + kint * x * g;
                const float fsat = std::tanh (flux * 1.6f) / 1.6f;
                const float y = (fsat - leak * fluxS) / kint; fluxS = fsat;
                return std::tanh (y * 0.9f + 0.05f) / 0.9f - 0.0555f;          // stadio in classe A dopo il ferro
            }
            case CONSOLE:   { const float u = f1 (x) * g; return ad1.run (u, [] (float q) { return consoleF (q); }, [] (float q) { const float q2 = q * q; return q / std::sqrt (std::sqrt (1.0f + q2 * q2)); }); }
            case CLIPPER:
            {
                const float u = f1 (x) * g;
                const float a = std::abs (u), knee = 0.92f;
                const float y = a < knee ? a : (a < 2 - knee ? knee + (a - knee) - (a - knee) * (a - knee) / (4 * (1 - knee)) : 1.0f);
                return std::copysign (juce::jmin (y, 1.0f), u);
            }
            case OVERDRIVE:
            {
                const float v = f2 (x) * g * 0.35f;
                const float u = v + (tsg - 1) * f1 (v);                     // il guadagno vale solo sopra ~720 Hz
                const float y = ad1.run (u, [] (float q) { return std::sqrt (1.0f + q * q); }, [] (float q) { return q / std::sqrt (1.0f + q * q); });   // diodi in controreazione: morbido
                return y + 0.15f * v;
            }
            case DISTORSIONE:
            {
                const float v = p1 (f1 (x) * g * distG);
                // diodi a massa: taglio duro (con antiderivata, cosi' non fischia)
                return ad1.run (v, [] (float q) { const float a = std::abs (q); return a <= 1.0f ? 0.5f * q * q : a - 0.5f; }, [] (float q) { return juce::jlimit (-1.0f, 1.0f, q); });
            }
            case FUZZ:
            {
                const float v = f1 (x) * g * (1.0f + 20.0f * d);
                const float a = std::abs (v); env = a > env ? envA * env + (1 - envA) * a : envR * env + (1 - envR) * a;
                const float bias = 0.25f - 0.2f * juce::jmin (1.0f, env);   // si sfalda quando il segnale scende
                const float T = std::tanh (2.2f * bias);
                const float y = ad1.run (v, [bias, T] (float q) { const float w = q + bias; return (w >= 0 ? logcosh (2.2f * w) / 2.2f : 0.75f * logcosh (0.7f * w) / 0.7f) - T * q; },
                                            [bias, T] (float q) { const float w = q + bias; return (w >= 0 ? std::tanh (2.2f * w) : 0.75f * std::tanh (0.7f * w)) - T; });
                return 0.9f * ad2.run (y * 1.4f / 0.9f, [] (float q) { return logcosh (q); }, [] (float q) { return std::tanh (q); });   // anche l'ultimo stadio con antiderivata
            }
            case OTTAVA:
            {
                const float v = f1 (x) * g * (1.0f + 12.0f * d);
                // fuzz + raddrizzatore (ottava sopra): 1.5 |tanh 2v| + 0.35 tanh 2v, con antiderivata
                return ad1.run (v, [] (float q) { const float L = logcosh (2.0f * q) * 0.5f; return 1.5f * (q >= 0 ? L : -L) + 0.35f * L; },
                                   [] (float q) { const float t = std::tanh (2.0f * q); return 1.5f * std::abs (t) + 0.35f * t; });
            }
            case AMP:
            {
                float u = f2 (f1 (x)) * g * 0.6f;
                u = std::tanh (u + 0.3f) - 0.2913f;                         // V1
                u = f4 (f3 (u)) * (1.0f + 6.0f * d);
                u = ad1.run (u, [] (float q) { return logcosh (q * 1.4f + 0.25f) / 1.4f - 0.2449f * q; }, [] (float q) { return std::tanh (q * 1.4f + 0.25f) - 0.2449f; });   // V2
                const float a = std::abs (u); env = a > env ? envA * env + (1 - envA) * a : envR * env + (1 - envR) * a;
                const float k = 2.2f - 0.6f * juce::jmin (env, 1.0f);
                return ad2.run (u, [k] (float q) { return logcosh (k * q) / k; }, [k] (float q) { return std::tanh (k * q); });   // finale push-pull con sag
            }
            case FOLD:      { const float u = f1 (x) * g; return std::sin (u * 1.5707963f); }
            case LOFI:
            {
                // trattiene il campione (frequenza ridotta) e riduce i bit
                if (holdN <= 0) { hold = x * g; holdN = lofiFactor; } --holdN;
                return std::round (std::tanh (hold) * lofiQ) / lofiQ;
            }
            case RADIO:     { float u = f4 (f3 (f2 (f1 (x)))) * g * 1.4f; return std::tanh (f5 (u)); }
            default: return x;
        }
    }
    inline float post (float y)
    {
        switch (model)
        {
            case TAPE:          return f5 (f4 (f3 (f2 (y)))) * 1.25f;
            case VALVOLA:       return f2 (dcBlock (y)) * 1.3f;
            case TRASFORMATORE: return f2 (f1 (y));
            case CONSOLE:       return f2 (y);
            case OVERDRIVE:     return f4 (f3 (y)) * 1.4f;
            case DISTORSIONE:   return f4 (f3 (y)) * 1.6f;
            case FUZZ: case OTTAVA: return f3 (f2 (dcBlock (y)));
            case AMP:           return f8 (f7 (f6 (f5 (p2 (dcBlock (y)))))) * 2.2f;   // cassa 4x12
            case FOLD:          return f2 (y) * 0.9f;
            case LOFI:          return f2 (f1 (y));
            case RADIO:         return y * 0.9f;
            default:            return y;
        }
    }
private:
    inline float dcBlock (float x) { const float y = x - dc1 + dcA * dc2; dc1 = x; dc2 = y; return y; }
    double fs = 176400; int osf = 4; int model = -1;
    BQ f1, f2, f3, f4, f5, f6, f7, f8; OnePole p1, p2, p3; Adaa ad1, ad2, ad3;
    float kint = 0.003f, leak = 0.999f, lastD = -1, tsg = 1, distG = 0.25f, lofiQ = 32768; int lofiFactor = 4; float dc1 = 0, dc2 = 0, dcA = 0.999f, env = 0, envA = 0.99f, envR = 0.999f, flux = 0, fluxS = 0, hold = 0, mag = 0; int holdN = 0;
};
}
