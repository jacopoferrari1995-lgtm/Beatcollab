#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>
#include <vector>
#include <atomic>

namespace jmo
{
constexpr float kTwoPi = 6.283185307179586f;

// ordine = ordine della catena (si applicano uno dopo l'altro, da sinistra a destra, riga per riga)
enum Fx { OMNIVOX, DOUBLER, TRIVOX, ENSEMBLE, CHORUS, DIMENSION, VIBRATO, FLANGER, PHASER, FSHIFT, RINGMOD, AUTOFILT, ROTARY, TREMOLO, kNumFx };
enum Shape { SINE, TRI, SQUARE, RANDOM, SH, kNumShapes };
// controlli usati da ogni effetto
enum Ctl { C_RATE = 1, C_DEPTH = 2, C_CHAR = 4, C_FB = 8, C_FREQ = 16, C_STEREO = 32, C_SHAPE = 64 };

struct FxInfo
{
    const char* id; const char* name; const char* sub; int ctl;
    const char* rateName; const char* depthName; const char* charName; const char* fbName; const char* freqName;
    const char* what; const char* use; juce::uint32 colour;
    float dRate, dDepth, dChar, dFb, dFreq, dMix;
};
inline const FxInfo& fxInfo (int i)
{
    static const FxInfo I[kNumFx] = {
        { "omni", "OMNIVOX", "armonizzatore in scala", C_CHAR, "", "", "APERTURA", "", "",
          "Aggiunge fino a 4 voci in tonalita: riconosce la nota che entra e crea terze, quinte, ottave ecc. dentro la scala scelta. Ogni voce ha il suo volume. Funziona su suoni monofonici (voce, lead, basso).", "voci, cori, lead, linee di synth", 0xffff2e88, 0.5f, 0.5f, 0.6f, 0, 1000, 0.7f },
        { "dubl", "DOUBLER", "seconda take finta", C_RATE | C_DEPTH | C_CHAR, "DERIVA", "VARIAZIONE", "DISTANZA", "", "",
          "Due copie a destra e sinistra con ritardi diversi e una deriva lenta e casuale di tempo e intonazione: sembra una seconda registrazione, non un effetto.", "voci rap e cantate, chitarre ritmiche", 0xffff7a1a, 0.7f, 0.4f, 0.3f, 0, 1000, 0.45f },
        { "triv", "TRIVOX", "tre voci a pochi centesimi", C_DEPTH | C_CHAR, "", "CENTESIMI", "RITARDO", "", "",
          "Centro piu due voci intonate fisse: una pochi centesimi sotto a sinistra, una sopra a destra. Niente movimento: allarga e ispessisce in modo stabile.", "ritornelli, voci lead, synth lead", 0xffffd23f, 0.5f, 0.25f, 0.35f, 0, 1000, 0.4f },
        { "ensm", "ENSEMBLE", "coro di 6 voci", C_RATE | C_DEPTH | C_CHAR, "RATE", "DEPTH", "SPAZIO", "", "",
          "Sei copie ognuna con il suo ritardo e la sua oscillazione, aperte su tutto lo stereo: da una traccia a un ensemble.", "pad, archi, cori", 0xffc77dff, 0.4f, 0.6f, 0.6f, 0, 1000, 0.5f },
        { "chor", "CHORUS", "copie che oscillano", C_RATE | C_DEPTH | C_CHAR | C_STEREO | C_SHAPE, "RATE", "DEPTH", "VOCI", "", "",
          "Copie ritardate di 12-25 ms che oscillano in modo regolare: il classico giro del chorus, piu largo e lucido.", "chitarre clean, pad, keys", 0xff3dff8b, 0.8f, 0.5f, 0.3f, 0, 1000, 0.5f },
        { "dimn", "DIMENSION", "allargatore a BBD", C_RATE | C_DEPTH | C_CHAR | C_FREQ, "RATE", "DEPTH", "MODO", "", "TONO BBD",
          "Due linee a BBD in controfase sommate in croce: allarga senza far sentire il giro. MODO da 1 (sottile) a 4 (largo), TONO BBD = quanto e scura la linea.", "mix bus, synth, voci, acustiche", 0xff5ef2ff, 0.45f, 0.5f, 0.35f, 0, 7000, 1.0f },
        { "vibr", "VIBRATO", "intonazione che ondeggia", C_RATE | C_DEPTH | C_STEREO | C_SHAPE, "FREQUENZA", "PROFONDITA", "", "", "",
          "Solo il segnale mosso, senza l'originale: l'intonazione oscilla. FREQUENZA = quante oscillazioni al secondo. Con MIX al 50% diventa un chorus.", "voci, synth, effetti lo-fi", 0xff7dff5a, 5.0f, 0.3f, 0, 0, 1000, 1.0f },
        { "flan", "FLANGER", "jet a ritardo cortissimo", C_RATE | C_DEPTH | C_FB | C_FREQ | C_STEREO | C_SHAPE, "RATE", "ESCURSIONE", "", "FEEDBACK", "FREQUENZA",
          "Ritardo di pochi millisecondi che si muove: FREQUENZA sceglie dove sta il pettine, ESCURSIONE quanto si sposta. FEEDBACK negativo = suono piu cavo.", "batterie, synth, transizioni", 0xff19e3ff, 0.2f, 0.6f, 0, 0.5f, 900, 1.0f },
        { "phas", "PHASER", "tacche che scorrono", C_RATE | C_DEPTH | C_CHAR | C_FB | C_FREQ | C_STEREO | C_SHAPE, "RATE", "ESCURSIONE", "STADI", "RISONANZA", "FREQUENZA",
          "Allpass in serie che scavano tacche nello spettro e le fanno scorrere. FREQUENZA = centro del giro, STADI da 2 a 12, RISONANZA rende il suono cantato.", "chitarre, rhodes, hi-hat, loop", 0xffa66bff, 0.4f, 0.6f, 0.25f, 0.3f, 900, 1.0f },
        { "fshf", "FREQ SHIFT", "spostamento in Hz (non armonico)", C_RATE | C_DEPTH | C_CHAR | C_FB | C_FREQ, "RATE", "MODULAZIONE", "VERSO", "FEEDBACK", "SPOSTAMENTO",
          "Sposta tutte le frequenze dello stesso numero di Hz: le armoniche non restano armoniche, il suono diventa metallico e strano. Pochi Hz = phaser lento e profondo; con FEEDBACK = scala infinita.", "transizioni, fx, pad, voci strane", 0xff00d4a0, 0.3f, 0, 0.0f, 0, 12, 1.0f },
        { "ring", "RING MOD", "moltiplica per un tono", C_RATE | C_DEPTH | C_CHAR | C_FREQ | C_STEREO | C_SHAPE, "RATE", "MODULAZIONE", "FORMA", "", "FREQUENZA",
          "Moltiplica il suono per un oscillatore: frequenze basse = tremolo veloce, alte = campanelle, robot e metallo. MODULAZIONE fa muovere l'oscillatore con l'LFO.", "voci robot, synth, percussioni, fx", 0xffff5ef2, 0.5f, 0, 0, 0, 220, 0.6f },
        { "afil", "AUTOFILTRO", "wah / filtro che si muove", C_RATE | C_DEPTH | C_CHAR | C_FB | C_FREQ | C_STEREO | C_SHAPE, "RATE", "ESCURSIONE", "TIPO", "RISONANZA", "FREQUENZA",
          "Un filtro risonante il cui taglio si apre e si chiude a tempo: wah, filtri che respirano, sweep. TIPO = passa basso, passa banda o passa alto.", "chitarre funk, synth, loop, pad", 0xffffb000, 0.5f, 0.5f, 0.2f, 0.5f, 900, 1.0f },
        { "rota", "ROTARY", "cassa rotante (leslie)", C_DEPTH | C_CHAR | C_FREQ, "", "PROFONDITA", "VELOCITA", "", "CROSSOVER",
          "La cassa con tromba e tamburo che girano: doppler, volume e stereo si muovono insieme. VELOCITA da lento (chorale) a veloce (tremolo), con l'inerzia vera.", "organo, rhodes, chitarre, voci", 0xffd9a066, 0, 0.7f, 0.0f, 0, 800, 1.0f },
        { "trem", "TREMOLO", "volume che pulsa", C_RATE | C_DEPTH | C_CHAR | C_STEREO | C_SHAPE, "RATE", "PROFONDITA", "SPIGOLO", "", "",
          "Il volume sale e scende a tempo. SPIGOLO va da onda morbida a taglio netto; STEREO sfasa i due lati (180 = rimbalza tra destra e sinistra).", "chitarre, piano elettrico, pad", 0xffffe066, 4.0f, 0.6f, 0.0f, 0, 1000, 1.0f } };
    return I[juce::jlimit (0, kNumFx - 1, i)];
}

// divisioni a tempo: battiti per ciclo
inline const juce::StringArray& divNames() { static const juce::StringArray d { "4 BAR", "2 BAR", "1 BAR", "1/2", "1/2T", "1/4D", "1/4", "1/4T", "1/8D", "1/8", "1/8T", "1/16", "1/16T", "1/32" }; return d; }
inline double divBeats (int i) { static const double b[] = { 16, 8, 4, 2, 4.0 / 3.0, 1.5, 1, 2.0 / 3.0, 0.75, 0.5, 1.0 / 3.0, 0.25, 1.0 / 6.0, 0.125 }; return b[juce::jlimit (0, 13, i)]; }

// scale dell'armonizzatore
inline const juce::StringArray& scaleNames() { static const juce::StringArray s { "MAGG", "MIN", "DORICA", "MIXO", "ARM.MIN", "PENTA+", "PENTA-", "BLUES" }; return s; }
inline const std::vector<int>& scaleSteps (int i)
{
    static const std::vector<int> S[8] = { { 0, 2, 4, 5, 7, 9, 11 }, { 0, 2, 3, 5, 7, 8, 10 }, { 0, 2, 3, 5, 7, 9, 10 }, { 0, 2, 4, 5, 7, 9, 10 },
                                           { 0, 2, 3, 5, 7, 8, 11 }, { 0, 2, 4, 7, 9 }, { 0, 3, 5, 7, 10 }, { 0, 3, 5, 6, 7, 10 } };
    return S[juce::jlimit (0, 7, i)];
}
inline const juce::StringArray& noteNames() { static const juce::StringArray n { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }; return n; }
// nota midi intera piu vicina che appartiene alla scala
inline int snapToScale (float midi, int key, int scale)
{
    const auto& st = scaleSteps (scale); int best = juce::roundToInt (midi); float bd = 99;
    for (int c = (int) std::floor (midi) - 3; c <= (int) std::ceil (midi) + 3; ++c)
    {
        const int pc = ((c - key) % 12 + 12) % 12;
        if (std::find (st.begin(), st.end(), pc) != st.end() && std::abs (c - midi) < bd) { bd = std::abs (c - midi); best = c; }
    }
    return best;
}
// sposta una nota della scala di "deg" gradi
inline int stepInScale (int note, int deg, int key, int scale)
{
    const auto& st = scaleSteps (scale); const int n = (int) st.size();
    const int rel = note - key, oct = (int) std::floor (rel / 12.0), pc = rel - oct * 12;
    int idx = 0; for (int k = 0; k < n; ++k) if (st[(size_t) k] == pc) idx = k;
    const int t = idx + deg, o2 = (int) std::floor ((double) t / n), i2 = t - o2 * n;
    return key + (oct + o2) * 12 + st[(size_t) i2];
}
inline juce::String intervalName (int deg, int scale)
{
    if (deg == 0) return "OFF";
    const int n = (int) scaleSteps (scale).size();
    if (n == 7) { if (std::abs (deg) == 7) return deg > 0 ? "+8va" : "-8va"; return (deg > 0 ? "+" : "-") + juce::String (std::abs (deg) + 1) + "a"; }
    return (deg > 0 ? "+" : "-") + juce::String (std::abs (deg)) + " gr";
}

// ---------------------------------------------------------------- LFO
struct Lfo
{
    double ph = 0; float a = 0, b = 0; juce::Random rng { 7 };
    float at (double p, int shape, float shapeAmt = 0.0f)
    {
        p -= std::floor (p);
        switch (shape)
        {
            case TRI: return (float) (p < 0.5 ? 4 * p - 1 : 3 - 4 * p);
            case SQUARE: { const float s = std::sin (kTwoPi * (float) p); return std::tanh (s * 8.0f) / std::tanh (8.0f); }
            case RANDOM: { const float t = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::pi * (float) p); return a + (b - a) * t; }
            case SH: return b;
            default: { const float s = std::sin (kTwoPi * (float) p); return shapeAmt > 0 ? std::tanh (s * (1 + shapeAmt * 10)) / std::tanh (1 + shapeAmt * 10) : s; }
        }
    }
    void advance (double inc) { ph += inc; if (ph >= 1.0) { ph -= std::floor (ph); a = b; b = rng.nextFloat() * 2 - 1; } }
    void setPhase (double p) { if (p < ph - 0.5) { a = b; b = rng.nextFloat() * 2 - 1; } ph = p - std::floor (p); }
};

// ---------------------------------------------------------------- linea di ritardo con interpolazione Hermite
struct Delay
{
    std::vector<float> d; int w = 0, mask = 0;
    void prepare (int maxSamples) { int n = 1; while (n < maxSamples + 4) n <<= 1; d.assign ((size_t) n, 0.0f); mask = n - 1; w = 0; }
    void clear() { std::fill (d.begin(), d.end(), 0.0f); }
    inline void push (float x) { d[(size_t) w] = x; w = (w + 1) & mask; }
    inline float read (float t) const
    {
        const float rp = (float) w - 1.0f - t; const int i = (int) std::floor (rp); const float f = rp - (float) i;
        const float y0 = d[(size_t) ((i - 1) & mask)], y1 = d[(size_t) (i & mask)], y2 = d[(size_t) ((i + 1) & mask)], y3 = d[(size_t) ((i + 2) & mask)];
        const float c1 = 0.5f * (y2 - y0), c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3, c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
        return ((c3 * f + c2) * f + c1) * f + y1;
    }
};

struct OnePole { float z = 0, a = 0; void setLp (float hz, double sr) { a = (float) std::exp (-kTwoPi * hz / sr); } inline float lp (float x) { z = x + a * (z - x); return z; } void reset() { z = 0; } };
// passa alto a 4 poli (due biquad Butterworth) per i bassi mono
struct Hp1
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z[2][2] {}; float last = -1;
    void set (float hz, double sr)
    {
        if (hz == last) return; last = hz;
        const double w = 2 * juce::MathConstants<double>::pi * hz / sr, cs = std::cos (w), al = std::sin (w) / (2 * 0.7071), a0 = 1 + al;
        b0 = (1 + cs) / 2 / a0; b1 = -(1 + cs) / a0; b2 = b0; a1 = -2 * cs / a0; a2 = (1 - al) / a0;
    }
    inline float run (float x)
    {
        double y = x;
        for (auto& s : z) { const double o = b0 * y + s[0]; s[0] = b1 * y - a1 * o + s[1]; s[1] = b2 * y - a2 * o; y = o; }
        return (float) y;
    }
};

// spostamento di intonazione (due prese che scorrono, dissolvenza triangolare)
struct MicroShift
{
    Delay dl; double ph = 0; float win = 1024;
    void prepare (double sr, float winSec = 0.035f) { win = (float) (sr * winSec); dl.prepare ((int) (win * 2 + sr * 0.06)); ph = 0; }
    void clear() { dl.clear(); ph = 0; }
    // finestra sincronizzata al periodo della nota: le due prese restano in fase (niente battimenti)
    void syncTo (float periodSamples, double sr)
    {
        if (periodSamples < 8) return;
        const float w = 2.0f * std::ceil ((float) (0.014 * sr) / periodSamples) * periodSamples;
        if (std::abs (w - win) > win * 0.02f) win = juce::jmin (w, (float) (sr * 0.05));
    }
    inline float process (float x, float ratio, float baseDelay)
    {
        dl.push (x);
        ph += (1.0 - ratio) / win; ph -= std::floor (ph);
        const float p1 = (float) ph, p2 = p1 + 0.5f - std::floor (p1 + 0.5f);
        const float g1 = 1.0f - std::abs (2.0f * p1 - 1.0f), g2 = 1.0f - std::abs (2.0f * p2 - 1.0f);
        return dl.read (baseDelay + p1 * win) * g1 + dl.read (baseDelay + p2 * win) * g2;
    }
};

// trasformata di Hilbert a IIR (due catene di allpass): uscite in quadratura per il frequency shifter
struct Hilbert
{
    struct AP { float c = 0, x1 = 0, x2 = 0, y1 = 0, y2 = 0; inline float run (float x) { const float y = c * (x + y2) - x2; x2 = x1; x1 = x; y2 = y1; y1 = y; return y; } };
    AP a[4], b[4]; float bd = 0;
    Hilbert()
    {
        const float ca[4] = { 0.6923878f, 0.9360654322959f, 0.9882295226860f, 0.9987488452737f }, cb[4] = { 0.4021921162426f, 0.8561710882420f, 0.9722909545651f, 0.9952884791278f };
        for (int k = 0; k < 4; ++k) { a[k].c = ca[k] * ca[k]; b[k].c = cb[k] * cb[k]; }
    }
    void reset() { for (auto& x : a) x.x1 = x.x2 = x.y1 = x.y2 = 0; for (auto& x : b) x.x1 = x.x2 = x.y1 = x.y2 = 0; bd = 0; }
    inline void run (float x, float& re, float& im) { float ya = x, yb = x; for (auto& f : a) ya = f.run (ya); for (auto& f : b) yb = f.run (yb); re = ya; im = bd; bd = yb; }
};

// riconoscimento dell'altezza (YIN) su segnale sottocampionato di 2
struct PitchDet
{
    std::vector<float> buf; int w = 0, cnt = 0, dec = 0; float acc = 0; double sr2 = 24000;
    float hz = 0; bool voiced = false;
    static constexpr int N = 2048, Wn = 384;
    std::vector<float> d;
    void prepare (double sr) { sr2 = sr / 2; buf.assign (N, 0.0f); d.assign ((size_t) (Wn + 1), 0.0f); w = cnt = dec = 0; hz = 0; voiced = false; }
    inline void push (float x)
    {
        acc += x; if (++dec < 2) return; dec = 0;
        buf[(size_t) w] = acc * 0.5f; acc = 0; w = (w + 1) & (N - 1);
        if (++cnt >= 128) { cnt = 0; analyse(); }
    }
    void analyse()
    {
        const int maxT = juce::jmin (Wn, (int) (sr2 / 65.0)), minT = juce::jmax (2, (int) (sr2 / 1100.0));
        const int start = (w - Wn - maxT - 1) & (N - 1);
        auto at = [this, start] (int i) { return buf[(size_t) ((start + i) & (N - 1))]; };
        double e = 0; for (int i = 0; i < Wn; ++i) e += at (i) * at (i);
        if (e / Wn < 1e-5) { voiced = false; return; }   // sotto -50 dB: silenzio
        double run = 0; d[0] = 1;
        int best = -1;
        for (int t = 1; t <= maxT; ++t)
        {
            double s = 0; for (int i = 0; i < Wn; ++i) { const float df = at (i) - at (i + t); s += df * df; }
            run += s; d[(size_t) t] = (float) (run > 0 ? s * t / run : 1);
            if (t >= minT && best < 0 && t > 2 && d[(size_t) (t - 1)] < 0.15f && d[(size_t) (t - 1)] <= d[(size_t) t]) best = t - 1;
        }
        if (best < 0) { voiced = false; return; }
        float tb = (float) best;
        if (best > 1 && best < maxT) { const float a = d[(size_t) best - 1], b = d[(size_t) best], c = d[(size_t) best + 1]; const float den = a - 2 * b + c; if (std::abs (den) > 1e-9f) tb += 0.5f * (a - c) / den; }
        hz = (float) (sr2 / tb); voiced = true;
    }
};

// ---------------------------------------------------------------- impostazioni di un effetto
struct FxSet
{
    bool on = false, sync = false; int div = 6, shape = SINE;
    float rateHz = 0.5f, depth = 0.5f, character = 0.5f, fb = 0, freq = 1000, stereo = 90, mix = 0.5f;
    int key = 0, scale = 0, deg[4] { 2, 4, 0, -7 }; float lvlDb[4] { 0, -4, 0, -6 };
};
struct Settings
{
    FxSet fx[kNumFx];
    float tone = 20000, width = 1, bassMono = 20, outDb = 0;
    bool panOn = false, panSync = false; float panHz = 1, panDepth = 0.7f; int panDiv = 6, panShape = SINE;
};

// ---------------------------------------------------------------- un effetto della catena
class Unit
{
public:
    int type = CHORUS;
    std::atomic<float> lfoNow { 0 }, lfoPh { 0 }, voicePh[6] {}, noteNow { -1 }, voiceNote[4] {}, angle { 0 };
    float fade = 0;

    void prepare (double s)
    {
        sr = s;
        for (auto& c : dl) c.prepare ((int) (sr * 0.12));
        for (auto& m : ms) m.prepare (sr, type == OMNIVOX ? 0.03f : 0.035f);
        pd.prepare (sr); reset();
    }
    void reset()
    {
        for (auto& c : dl) c.clear(); for (auto& m : ms) m.clear();
        for (auto& c : ch) { for (auto& a : c.ap) a = 0; c.fbS = 0; c.flFb = 0; c.s1 = c.s2 = 0; c.car = 0; }
        for (auto& t : wetLp) t.reset(); for (auto& h : hil) h.reset();
        lfo = Lfo(); for (int i = 0; i < 12; ++i) { vl[i] = Lfo(); vl[i].rng.setSeed (100 + i * 37); vl[i].ph = i / 12.0; }
        dimPh = 0; shPh = 0; fsFb = 0; rotH = rotD = 0; rotSpH = rotSpD = -1; xLp = 0;
        for (auto& r : hRatio) r = 1; lastNote = -1;
    }

    void process (float* L, float* R, int n, const FxSet& S, double ppq, double bpm)
    {
        const float tgt = S.on ? 1.0f : 0.0f;
        if (fade <= 0.0f && ! S.on) { noteNow = -1; return; }
        if (fade <= 0.0f && S.on) reset();
        const float fStep = 1.0f / (float) (0.02 * sr);
        const double rate = S.sync ? bpm / 60.0 / divBeats (S.div) : S.rateHz;
        if (ppq >= 0 && S.sync) lfo.setPhase (ppq / divBeats (S.div));
        const double inc = rate / sr;
        const float stOff = S.stereo / 360.0f, mix = S.mix;
        const float dryG = std::cos (mix * juce::MathConstants<float>::halfPi), wetG = std::sin (mix * juce::MathConstants<float>::halfPi);
        const int stages = 2 + 2 * juce::jlimit (0, 5, (int) (S.character * 5.99f));
        if (type == DIMENSION) { wetLp[0].setLp (S.freq, sr); wetLp[1].setLp (S.freq, sr); }
        else { wetLp[0].setLp (15000.0f, sr); wetLp[1].setLp (15000.0f, sr); }
        // armonizzatore: rapporti delle voci dalla nota riconosciuta
        float hGain[4] {}, hPan[4] {};
        if (type == OMNIVOX)
        {
            static const float pans[4] = { -1.0f, 1.0f, -0.5f, 0.5f };
            for (int v = 0; v < 4; ++v) { hGain[v] = S.deg[v] == 0 ? 0.0f : juce::Decibels::decibelsToGain (S.lvlDb[v], -40.0f); hPan[v] = pans[v] * S.character; }
        }
        const float glide = (float) (1.0 - std::exp (-1.0 / (0.015 * sr)));
        for (int i = 0; i < n; ++i)
        {
            const float xl = L[i], xr = R[i];
            float wl = xl, wr = xr; bool full = false;   // full: l'effetto contiene gia il dry -> crossfade lineare
            const float lA = lfo.at (lfo.ph, S.shape, type == TREMOLO ? S.character : 0), lB = lfo.at (lfo.ph + stOff, S.shape, type == TREMOLO ? S.character : 0);
            switch (type)
            {
                case TREMOLO:
                {
                    wl = xl * (1.0f - S.depth * (0.5f - 0.5f * lA)); wr = xr * (1.0f - S.depth * (0.5f - 0.5f * lB)); full = true; break;
                }
                case PHASER:
                {
                    for (int c = 0; c < 2; ++c)
                    {
                        auto& C = ch[c]; const float m = c ? lB : lA;
                        if ((i & 7) == 0)
                        {
                            const float f = juce::jlimit (30.0f, (float) sr * 0.45f, S.freq * std::pow (2.0f, m * (0.4f + 3.1f * S.depth) * 0.5f));
                            const float t = std::tan (juce::MathConstants<float>::pi * f / (float) sr);
                            C.apA = (t - 1) / (t + 1);
                        }
                        float y = (c ? xr : xl) + C.fbS * S.fb;
                        for (int k = 0; k < stages; ++k) { const float v = C.apA * y + C.ap[k]; C.ap[k] = y - C.apA * v; y = v; }
                        C.fbS = y;
                        (c ? wr : wl) = 0.5f * ((c ? xr : xl) + y) * 1.4f;
                    }
                    full = true; break;
                }
                case FLANGER:
                {
                    // FREQUENZA = primo picco del pettine (ritardo = 1/f), ESCURSIONE fino a due ottave sotto
                    const float base = 1.0f / juce::jmax (50.0f, S.freq);
                    for (int c = 0; c < 2; ++c)
                    {
                        auto& C = ch[c]; const float m = 0.5f + 0.5f * (c ? lB : lA);
                        const float t = juce::jlimit (1.5f, (float) (0.025 * sr), (float) (base * (1.0f + S.depth * 3.0f * m) * sr));
                        const float x = c ? xr : xl;
                        dl[c].push (x + C.flFb * S.fb);
                        const float y = dl[c].read (t); C.flFb = std::tanh (y);
                        (c ? wr : wl) = 0.5f * (x + y) * 1.3f;
                    }
                    full = true; break;
                }
                case CHORUS:
                {
                    const int voices = 1 + juce::jlimit (0, 3, (int) (S.character * 3.99f));
                    const float base = 0.012f + 0.004f * voices, sw = 0.0045f * S.depth;
                    for (int c = 0; c < 2; ++c)
                    {
                        dl[c].push (c ? xr : xl);
                        float y = 0;
                        for (int v = 0; v < voices; ++v)
                        {
                            const float m = lfo.at (lfo.ph + (c ? stOff : 0) + v / (float) voices, S.shape);
                            y += dl[c].read ((float) ((base + v * 0.0027f + sw * (0.5f + 0.5f * m)) * sr));
                        }
                        (c ? wr : wl) = wetLp[c].lp (y / std::sqrt ((float) voices));
                    }
                    break;
                }
                case DIMENSION:
                {
                    static const float dep[4] = { 0.0006f, 0.0011f, 0.0016f, 0.0024f };
                    const int md = juce::jlimit (0, 3, (int) (S.character * 3.99f));
                    const float tri = lfo.at (lfo.ph, TRI);
                    const float mono = 0.5f * (xl + xr);
                    dl[0].push (mono);
                    const float d1 = dl[0].read ((float) ((0.0065f + dep[md] * tri * (0.5f + S.depth)) * sr));
                    const float d2 = dl[0].read ((float) ((0.0065f - dep[md] * tri * (0.5f + S.depth)) * sr));
                    const float a = wetLp[0].lp (d1), b = wetLp[1].lp (d2);
                    wl = (xl + 0.7f * a - 0.25f * b) * 0.8f; wr = (xr + 0.7f * b - 0.25f * a) * 0.8f; full = true;
                    break;
                }
                case ENSEMBLE:
                {
                    for (int c = 0; c < 2; ++c) dl[c].push (c ? xr : xl);
                    float yl = 0, yr = 0;
                    const float spread = 0.012f + 0.022f * S.character;
                    for (int v = 0; v < 6; ++v)
                    {
                        auto& V = vl[v]; V.advance (inc * (0.6 + 0.13 * v));
                        const float m = V.at (V.ph, RANDOM) * 0.6f + std::sin (kTwoPi * (float) (V.ph * 2 + v * 0.17)) * 0.4f;
                        const float t = (float) ((0.010f + spread * v / 5.0f + 0.004f * S.depth * m) * sr);
                        static const float pans[6] = { -1.0f, 1.0f, -0.55f, 0.55f, -0.15f, 0.15f }; const float pan = pans[v], gL = std::sqrt (0.5f * (1 - pan * 0.9f)), gR = std::sqrt (0.5f * (1 + pan * 0.9f));
                        const float s = 0.5f * (dl[0].read (t) + dl[1].read (t + 7));
                        yl += s * gL; yr += s * gR;
                    }
                    wl = wetLp[0].lp (yl * 0.5f); wr = wetLp[1].lp (yr * 0.5f);
                    break;
                }
                case OMNIVOX:
                {
                    const float mono = 0.5f * (xl + xr);
                    pd.push (mono);
                    if ((i & 63) == 0)
                    {
                        if (pd.voiced && pd.hz > 40)
                        {
                            const float midi = 69.0f + 12.0f * std::log2 (pd.hz / 440.0f);
                            for (auto& m : ms) m.syncTo ((float) (sr / pd.hz), sr);
                            const int root = snapToScale (midi, S.key, S.scale);
                            for (int v = 0; v < 4; ++v)
                            {
                                const int tn = stepInScale (root, S.deg[v], S.key, S.scale);
                                tRatio[v] = std::pow (2.0f, ((float) tn - midi) / 12.0f); voiceNote[v] = S.deg[v] == 0 ? -1.0f : (float) tn;
                            }
                            noteNow = midi; lastNote = midi;
                        }
                        else
                        {
                            if (lastNote < 0) for (int v = 0; v < 4; ++v) { const int tn = stepInScale (S.key + 60, S.deg[v], S.key, S.scale); tRatio[v] = std::pow (2.0f, (float) (tn - S.key - 60) / 12.0f); }
                            noteNow = -1;
                        }
                    }
                    float yl = 0, yr = 0;
                    for (int v = 0; v < 4; ++v)
                    {
                        hRatio[v] += (tRatio[v] - hRatio[v]) * glide;
                        if (hGain[v] <= 0) continue;
                        const float y = ms[v].process (mono, hRatio[v], 2.0f) * hGain[v];
                        yl += y * std::sqrt (0.5f * (1 - hPan[v])); yr += y * std::sqrt (0.5f * (1 + hPan[v]));
                    }
                    // il dry resta intero: MIX regola quanto si sentono le voci
                    wl = xl + yl * mix * 0.8f; wr = xr + yr * mix * 0.8f; full = true;
                    break;
                }
                case DOUBLER:
                {
                    const float mono = 0.5f * (xl + xr);
                    dl[0].push (mono);
                    const float base = 0.010f + 0.045f * S.character;
                    vl[0].advance (inc * 0.9); vl[1].advance (inc * 1.13);
                    const float cents = 3.0f + 20.0f * S.depth;
                    const float amp = (float) juce::jmin (0.008 * sr, (cents / 1731.0) * sr / (kTwoPi * juce::jmax (0.1, rate)));
                    const float dA = (float) (base * sr) + amp * (1.0f + vl[0].at (vl[0].ph, RANDOM));
                    const float dB = (float) (base * 1.37f * sr) + amp * (1.0f + vl[1].at (vl[1].ph, RANDOM));
                    const float a = dl[0].read (juce::jmax (2.0f, dA)), b = dl[0].read (juce::jmax (2.0f, dB));
                    const float gA = 1.0f + 0.08f * vl[0].at (vl[0].ph * 3.1, RANDOM), gB = 1.0f + 0.08f * vl[1].at (vl[1].ph * 2.7, RANDOM);
                    wl = wetLp[0].lp (a * gA); wr = wetLp[1].lp (b * gB);
                    break;
                }
                case TRIVOX:
                {
                    const float mono = 0.5f * (xl + xr);
                    const float cents = 3.0f + 27.0f * S.depth;
                    const float up = std::pow (2.0f, cents / 1200.0f), dn = 1.0f / up;
                    const float d = (float) (S.character * 0.03 * sr);
                    const float a = ms[0].process (mono, dn, d + 2), b = ms[1].process (mono, up, d * 1.3f + 2);
                    dl[0].push (mono); const float cen = dl[0].read (juce::jmax (2.0f, d * 0.5f + 2));
                    wl = wetLp[0].lp (a * 0.9f + cen * 0.35f); wr = wetLp[1].lp (b * 0.9f + cen * 0.35f);
                    break;
                }
                case VIBRATO:
                {
                    const float sw = 0.0055f * S.depth;
                    for (int c = 0; c < 2; ++c)
                    {
                        dl[c].push (c ? xr : xl);
                        const float m = 0.5f + 0.5f * (c ? lB : lA);
                        (c ? wr : wl) = dl[c].read ((float) ((0.0006f + sw * m) * sr) + 2.0f);
                    }
                    full = true;
                    break;
                }
                case RINGMOD:
                {
                    for (int c = 0; c < 2; ++c)
                    {
                        auto& C = ch[c]; const float m = c ? lB : lA;
                        const float f = juce::jlimit (1.0f, (float) sr * 0.4f, S.freq * std::pow (2.0f, S.depth * 2.0f * m));
                        C.car += f / (float) sr; C.car -= std::floor (C.car);
                        float s = std::sin (kTwoPi * (C.car + (c ? stOff * 0.0f : 0.0f)));
                        if (S.character > 0.01f) s = std::tanh (s * (1 + S.character * 12)) / std::tanh (1 + S.character * 12);
                        (c ? wr : wl) = (c ? xr : xl) * s;
                    }
                    full = true; break;
                }
                case FSHIFT:
                {
                    const float sh = S.freq * (1.0f + S.depth * lA);
                    shPh += sh / sr; shPh -= std::floor (shPh);
                    const float cs = std::cos (kTwoPi * (float) shPh), sn = std::sin (kTwoPi * (float) shPh);
                    const int dir = S.character < 0.34f ? 0 : S.character < 0.67f ? 1 : 2;   // su, giu, stereo
                    for (int c = 0; c < 2; ++c)
                    {
                        float re, im; hil[c].run ((c ? xr : xl) + ch[c].fbS * S.fb, re, im);
                        const bool down = dir == 1 || (dir == 2 && c == 1);
                        const float y = down ? re * cs - im * sn : re * cs + im * sn;
                        ch[c].fbS = std::tanh (y);
                        (c ? wr : wl) = y;
                    }
                    break;
                }
                case AUTOFILT:
                {
                    const int ft = S.character < 0.34f ? 0 : S.character < 0.67f ? 1 : 2;
                    const float k = 2.0f - 1.9f * juce::jlimit (0.0f, 1.0f, S.fb);
                    for (int c = 0; c < 2; ++c)
                    {
                        auto& C = ch[c]; const float m = c ? lB : lA;
                        if ((i & 3) == 0)
                        {
                            const float f = juce::jlimit (25.0f, (float) sr * 0.42f, S.freq * std::pow (2.0f, m * S.depth * 3.0f));
                            C.g = std::tan (juce::MathConstants<float>::pi * f / (float) sr);
                        }
                        const float x = c ? xr : xl, g = C.g;
                        const float hp = (x - (k + g) * C.s1 - C.s2) / (1 + g * (k + g));
                        const float bp = g * hp + C.s1; C.s1 = g * hp + bp;
                        const float lp = g * bp + C.s2; C.s2 = g * bp + lp;
                        float y = ft == 0 ? lp : ft == 1 ? bp * k : hp;
                        (c ? wr : wl) = std::tanh (y * 0.9f) / 0.9f;
                    }
                    full = true; break;
                }
                case ROTARY:
                {
                    // velocita lenta 0.8 Hz -> veloce 6.7 Hz; tromba accelera in fretta, tamburo lento
                    const float tH = 0.8f + 5.9f * S.character, tD = 0.65f + 5.2f * S.character;
                    if (rotSpH < 0) { rotSpH = tH; rotSpD = tD; }
                    rotSpH += (tH - rotSpH) * (float) (1.0 / (0.7 * sr)); rotSpD += (tD - rotSpD) * (float) (1.0 / (2.5 * sr));
                    rotH += rotSpH / sr; rotH -= std::floor (rotH); rotD += rotSpD / sr; rotD -= std::floor (rotD);
                    const float mono = 0.5f * (xl + xr);
                    const float a = (float) std::exp (-kTwoPi * juce::jlimit (100.0f, 4000.0f, S.freq) / sr);
                    xLp = mono + a * (xLp - mono);
                    const float lo = xLp, hi = mono - xLp;
                    const float sH = std::sin (kTwoPi * (float) rotH), cH = std::cos (kTwoPi * (float) rotH), sD = std::sin (kTwoPi * (float) rotD);
                    dl[0].push (hi);
                    const float hd = dl[0].read ((float) ((0.0018 + 0.0011 * S.depth * (1 + sH)) * sr));
                    const float hAm = 1.0f - 0.4f * S.depth * (0.5f - 0.5f * cH), dAm = 1.0f - 0.25f * S.depth * (0.5f - 0.5f * sD);
                    const float hL = hd * hAm * (1.0f + 0.7f * S.depth * sH), hR = hd * hAm * (1.0f - 0.7f * S.depth * sH);
                    const float dL = lo * dAm * (1.0f + 0.25f * S.depth * sD), dR = lo * dAm * (1.0f - 0.25f * S.depth * sD);
                    wl = (hL + dL) * 0.95f; wr = (hR + dR) * 0.95f; full = true;
                    break;
                }
                default: break;
            }
            lfo.advance (inc);
            float ol, orr;
            if (type == OMNIVOX) { ol = wl; orr = wr; }
            else if (full) { ol = xl + (wl - xl) * mix; orr = xr + (wr - xr) * mix; }
            else { ol = xl * dryG + wl * wetG; orr = xr * dryG + wr * wetG; }
            // accensione e spegnimento senza click
            fade = juce::jlimit (0.0f, 1.0f, fade + (tgt > fade ? fStep : -fStep));
            L[i] = xl + (ol - xl) * fade; R[i] = xr + (orr - xr) * fade;
        }
        lfoNow = lfo.at (lfo.ph, S.shape); lfoPh = (float) lfo.ph;
        for (int v = 0; v < 6; ++v) voicePh[v] = (float) vl[v].at (vl[v].ph, RANDOM);
        angle = (float) rotH;
        if (! S.on && fade <= 0) noteNow = -1;
    }
private:
    struct Ch { float ap[12] {}; float apA = 0, fbS = 0, flFb = 0, s1 = 0, s2 = 0, g = 0.1f; double car = 0; };
    double sr = 44100, dimPh = 0, shPh = 0, rotH = 0, rotD = 0; float fsFb = 0, rotSpH = -1, rotSpD = -1, xLp = 0;
    Delay dl[2]; MicroShift ms[4]; Ch ch[2]; OnePole wetLp[2]; Hilbert hil[2]; PitchDet pd;
    Lfo lfo, vl[12];
    float hRatio[4] { 1, 1, 1, 1 }, tRatio[4] { 1, 1, 1, 1 }, lastNote = -1;
};

// ---------------------------------------------------------------- motore: catena + uscita
class Engine
{
public:
    Engine() { for (int i = 0; i < kNumFx; ++i) units[i].type = i; }
    void prepare (double s, int)
    {
        sr = s; for (auto& u : units) { u.prepare (s); u.fade = 0; }
        panLfo = Lfo(); for (auto& s2 : sideHp[0].z) s2[0] = s2[1] = 0; toneLp[0].reset(); toneLp[1].reset();
        dryL.assign (4096, 0.0f); dryR.assign (4096, 0.0f);
    }
    void process (float* L, float* R, int n, const Settings& S, double ppq, double bpm)
    {
        for (int off = 0; off < n; off += 2048)
        {
            const int m = juce::jmin (2048, n - off);
            processChunk (L + off, R + off, m, S, ppq < 0 ? ppq : ppq + off / sr * bpm / 60.0, bpm);
        }
    }
    Unit units[kNumFx];
    std::atomic<float> panNow { 0 }, panPh { 0 };
private:
    void processChunk (float* L, float* R, int n, const Settings& S, double ppq, double bpm)
    {
        std::copy (L, L + n, dryL.begin()); std::copy (R, R + n, dryR.begin());
        for (int f = 0; f < kNumFx; ++f) units[f].process (L, R, n, S.fx[f], ppq, bpm);
        const double pRate = S.panSync ? bpm / 60.0 / divBeats (S.panDiv) : S.panHz;
        if (ppq >= 0 && S.panSync) panLfo.setPhase (ppq / divBeats (S.panDiv));
        const double pInc = pRate / sr;
        toneLp[0].setLp (S.tone, sr); toneLp[1].setLp (S.tone, sr);
        sideHp[0].set (S.bassMono, sr);
        const float outG = juce::Decibels::decibelsToGain (S.outDb);
        const bool toneOn = S.tone < 19900.0f;
        for (int i = 0; i < n; ++i)
        {
            float ol = L[i], orr = R[i];
            // tono: scurisce solo la parte aggiunta dagli effetti
            if (toneOn) { ol = dryL[(size_t) i] + toneLp[0].lp (ol - dryL[(size_t) i]); orr = dryR[(size_t) i] + toneLp[1].lp (orr - dryR[(size_t) i]); }
            if (S.width != 1.0f || S.bassMono > 21.0f)
            {
                const float mid = 0.5f * (ol + orr); float side = 0.5f * (ol - orr) * S.width;
                if (S.bassMono > 21.0f) side = sideHp[0].run (side);
                ol = mid + side; orr = mid - side;
            }
            if (S.panOn)
            {
                const float pp = panLfo.at (panLfo.ph, S.panShape) * S.panDepth;
                const float gl = std::cos ((pp + 1) * juce::MathConstants<float>::pi * 0.25f) * 1.41421356f, gr = std::sin ((pp + 1) * juce::MathConstants<float>::pi * 0.25f) * 1.41421356f;
                ol *= gl; orr *= gr; panNow = pp;
            }
            panLfo.advance (pInc);
            L[i] = ol * outG; R[i] = orr * outG;
        }
        panPh = (float) panLfo.ph;
        if (! S.panOn) panNow = 0;
    }
    double sr = 44100;
    Hp1 sideHp[1]; OnePole toneLp[2]; Lfo panLfo;
    std::vector<float> dryL, dryR;
};
}
