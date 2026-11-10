#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>
#include <vector>
#include <atomic>

namespace jmo
{
constexpr float kTwoPi = 6.283185307179586f;

enum Fx { TREMOLO, PHASER, FLANGER, CHORUS, DIMENSION, OMNIVOX, DOUBLER, TRIVOX, VIBRATO, kNumFx };
enum Shape { SINE, TRI, SQUARE, RANDOM, SH, kNumShapes };

struct FxInfo { const char* name; const char* sub; const char* charName; const char* fbName; const char* what; const char* use; juce::uint32 colour; };
inline const FxInfo& fxInfo (int i)
{
    static const FxInfo I[kNumFx] = {
        { "TREMOLO",   "volume che pulsa",            "FORMA",     "",          "Il volume sale e scende a tempo. FORMA va da onda morbida a taglio netto; STEREO sfasa i due lati.", "chitarre, piano elettrico, pad, synth", 0xffffb000 },
        { "PHASER",    "allpass che scorrono",        "STADI",     "RISONANZA", "Tacche nello spettro che si muovono: da 2 a 12 stadi. RISONANZA rende il giro piu aspro e cantato.", "chitarre, rhodes, hi-hat, loop di batteria", 0xffa66bff },
        { "FLANGER",   "jet a ritardo cortissimo",    "MANUALE",   "FEEDBACK",  "Un ritardo di pochi millisecondi che si muove: effetto aereo, a pettine. FEEDBACK negativo = piu cavo.", "batterie, synth, chitarre, transizioni", 0xff19e3ff },
        { "CHORUS",    "voci leggermente stonate",    "VOCI",      "",          "Copie ritardate e mosse che si sommano all'originale: suono piu largo e pieno. VOCI da 1 a 4 per lato.", "chitarre clean, pad, voci di fondo, keys", 0xff3dff8b },
        { "DIMENSION", "allargatore a BBD",           "MODO",      "",          "Ispirato ai Dimension a 4 tasti: allarga senza sentire il giro. MODO da 1 (sottile) a 4 (largo).", "mix bus, synth, voci, chitarre acustiche", 0xff5ef2ff },
        { "OMNIVOX",   "ensemble a 6 voci",           "SPAZIO",    "",          "Sei voci con ritardi e derive indipendenti, aperte su tutto lo stereo: un coro intero da una traccia.", "cori, pad, voci, archi", 0xffff2e88 },
        { "DOUBLER",   "raddoppio di voce",           "DISTANZA",  "",          "Due riprese finte a destra e sinistra, con piccole differenze di tempo e intonazione come una seconda take.", "voci rap e cantate, chitarre ritmiche", 0xffff7a1a },
        { "TRIVOX",    "tre voci con micro pitch",    "RITARDO",   "",          "Centro piu due voci spostate di pochi centesimi in su e in giu: il trucco classico per voci larghe e spesse.", "ritornelli, voci lead, synth lead", 0xffffd23f },
        { "VIBRATO",   "intonazione che ondeggia",    "FORMA",     "",          "Solo il segnale mosso, senza l'originale: l'intonazione oscilla. Con MIX al 50% diventa un chorus.", "voci, synth, chitarre, effetti lo-fi", 0xff7dff5a } };
    return I[juce::jlimit (0, kNumFx - 1, i)];
}

// divisioni a tempo: battiti per ciclo
inline const juce::StringArray& divNames() { static const juce::StringArray d { "4 BAR", "2 BAR", "1 BAR", "1/2", "1/2T", "1/4D", "1/4", "1/4T", "1/8D", "1/8", "1/8T", "1/16", "1/16T", "1/32" }; return d; }
inline double divBeats (int i) { static const double b[] = { 16, 8, 4, 2, 4.0 / 3.0, 1.5, 1, 2.0 / 3.0, 0.75, 0.5, 1.0 / 3.0, 0.25, 1.0 / 6.0, 0.125 }; return b[juce::jlimit (0, 13, i)]; }

// ---------------------------------------------------------------- LFO
struct Lfo
{
    double ph = 0; float a = 0, b = 0; juce::Random rng { 7 };
    // valore in [-1, 1] alla fase p (0..1)
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
    // avanza; se il ciclo ricomincia sceglie nuovi punti casuali
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
    // t = ritardo in campioni (>= 1)
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

// spostamento di intonazione costante (micro shift): due prese che scorrono, dissolvenza triangolare
struct MicroShift
{
    Delay dl; double ph = 0; float win = 1024;
    void prepare (double sr) { win = (float) (sr * 0.035); dl.prepare ((int) (win * 2 + sr * 0.06)); ph = 0; }
    inline float process (float x, float ratio, float baseDelay)
    {
        dl.push (x);
        ph += (1.0 - ratio) / win; ph -= std::floor (ph);
        const float p1 = (float) ph, p2 = p1 + 0.5f - std::floor (p1 + 0.5f);
        const float g1 = 1.0f - std::abs (2.0f * p1 - 1.0f), g2 = 1.0f - std::abs (2.0f * p2 - 1.0f);
        return dl.read (baseDelay + p1 * win) * g1 + dl.read (baseDelay + p2 * win) * g2;
    }
};

// ---------------------------------------------------------------- impostazioni
struct Settings
{
    int fx = CHORUS, shape = SINE, panShape = SINE;
    float rateHz = 0.5f, depth = 0.5f, fb = 0, character = 0.5f, stereo = 90, mix = 0, tone = 20000, width = 1, bassMono = 20, outDb = 0;
    bool panOn = false; float panHz = 1, panDepth = 0.7f;
    bool sync = false, panSync = false; int div = 6, panDiv = 6;
};

// ---------------------------------------------------------------- motore
class Engine
{
public:
    void prepare (double s, int)
    {
        sr = s;
        for (auto& c : dl) c.prepare ((int) (sr * 0.12));
        for (auto& m : ms) m.prepare (sr);
        for (auto& c : ch) { for (auto& a : c.ap) a = 0; c.fbS = 0; c.flFb = 0; }
        for (auto& t : wetLp) t.reset();
        lfo = Lfo(); panLfo = Lfo(); for (auto& v : vl) v = Lfo(); for (int i = 0; i < 12; ++i) { vl[i].rng.setSeed (100 + i * 37); vl[i].ph = i / 12.0; }
        for (auto& s : sideHp[0].z) s[0] = s[1] = 0;
        lastFx = -1;
    }
    // ppq < 0: trasporto fermo, l'LFO gira libero
    void process (float* L, float* R, int n, const Settings& S, double ppq, double bpm)
    {
        if (S.fx != lastFx) { for (auto& c : dl) c.clear(); for (auto& c : ch) { for (auto& a : c.ap) a = 0; c.fbS = 0; c.flFb = 0; } lastFx = S.fx; }
        const double rate = S.sync ? bpm / 60.0 / divBeats (S.div) : S.rateHz;
        const double pRate = S.panSync ? bpm / 60.0 / divBeats (S.panDiv) : S.panHz;
        if (ppq >= 0 && S.sync) lfo.setPhase (ppq / divBeats (S.div));
        if (ppq >= 0 && S.panSync) panLfo.setPhase (ppq / divBeats (S.panDiv));
        const double inc = rate / sr, pInc = pRate / sr;
        const float stOff = S.stereo / 360.0f;
        for (auto& t : wetLp) t.setLp (S.tone, sr);
        sideHp[0].set (S.bassMono, sr);
        const float mix = S.mix, dryG = std::cos (mix * juce::MathConstants<float>::halfPi), wetG = std::sin (mix * juce::MathConstants<float>::halfPi);
        const float outG = juce::Decibels::decibelsToGain (S.outDb);
        const int stages = 2 + 2 * juce::jlimit (0, 5, (int) (S.character * 5.99f));   // 2..12
        for (int i = 0; i < n; ++i)
        {
            const float xl = L[i], xr = R[i];
            float wl = xl, wr = xr;
            const float lA = lfo.at (lfo.ph, S.shape, S.fx == TREMOLO ? S.character : 0), lB = lfo.at (lfo.ph + stOff, S.shape, S.fx == TREMOLO ? S.character : 0);
            switch (S.fx)
            {
                case TREMOLO:
                {
                    const float gA = 1.0f - S.depth * (0.5f - 0.5f * lA), gB = 1.0f - S.depth * (0.5f - 0.5f * lB);
                    wl = xl * gA; wr = xr * gB; break;
                }
                case PHASER:
                {
                    // escursione su 3 ottave e mezzo intorno a 900 Hz
                    for (int c = 0; c < 2; ++c)
                    {
                        auto& C = ch[c]; const float m = c ? lB : lA;
                        if ((i & 7) == 0)
                        {
                            const float f = 900.0f * std::pow (2.0f, m * (0.4f + 3.1f * S.depth) * 0.5f + 0.2f);
                            const float t = std::tan (juce::MathConstants<float>::pi * juce::jmin (f, (float) sr * 0.45f) / (float) sr);
                            C.apA = (t - 1) / (t + 1);
                        }
                        float y = (c ? xr : xl) + C.fbS * S.fb;
                        for (int k = 0; k < stages; ++k) { const float v = C.apA * y + C.ap[k]; C.ap[k] = y - C.apA * v; y = v; }
                        C.fbS = y;
                        (c ? wr : wl) = 0.5f * ((c ? xr : xl) + y) * 1.4f;
                    }
                    break;
                }
                case FLANGER:
                {
                    const float base = 0.0001f + S.character * 0.006f, sweep = 0.004f * S.depth;
                    for (int c = 0; c < 2; ++c)
                    {
                        auto& C = ch[c]; const float m = 0.5f + 0.5f * (c ? lB : lA);
                        const float t = juce::jmax (1.5f, (float) ((base + sweep * m) * sr));
                        const float x = c ? xr : xl;
                        dl[c].push (x + C.flFb * S.fb);
                        const float y = dl[c].read (t); C.flFb = std::tanh (y);
                        (c ? wr : wl) = 0.5f * (x + y) * 1.3f;
                    }
                    break;
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
                    // quattro modi: profondita e velocita fisse come sui Dimension, linea BBD scura, somma incrociata
                    static const float dep[4] = { 0.0006f, 0.0011f, 0.0016f, 0.0024f }, rt[4] = { 0.25f, 0.4f, 0.5f, 0.65f };
                    const int md = juce::jlimit (0, 3, (int) (S.character * 3.99f));
                    dimPh += rt[md] / sr; dimPh -= std::floor (dimPh);
                    const float tri = (float) (dimPh < 0.5 ? 4 * dimPh - 1 : 3 - 4 * dimPh);
                    const float mono = 0.5f * (xl + xr);
                    dl[0].push (mono);
                    const float d1 = dl[0].read ((float) ((0.0065f + dep[md] * tri * (0.5f + S.depth)) * sr));
                    const float d2 = dl[0].read ((float) ((0.0065f - dep[md] * tri * (0.5f + S.depth)) * sr));
                    const float a = wetLp[0].lp (d1), b = wetLp[1].lp (d2);
                    wl = (xl + 0.7f * a - 0.25f * b) * 0.8f; wr = (xr + 0.7f * b - 0.25f * a) * 0.8f;   // gia comprende il dry: MIX regola quanto
                    break;
                }
                case OMNIVOX:
                {
                    // 6 voci, ciascuna con il suo ritardo, la sua deriva lenta e la sua posizione nello stereo
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
                case DOUBLER:
                {
                    // due riprese finte: ritardi diversi, deriva di intonazione lenta e casuale (in centesimi)
                    const float mono = 0.5f * (xl + xr);
                    dl[0].push (mono);
                    const float base = 0.010f + 0.045f * S.character;
                    vl[0].advance (inc * 0.9); vl[1].advance (inc * 1.13);
                    // escursione di pitch in centesimi -> ampiezza della modulazione del ritardo
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
                    // centro leggermente ritardato + sinistra in giu e destra in su di pochi centesimi
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
                default:
                {
                    const float sw = 0.0055f * S.depth;
                    for (int c = 0; c < 2; ++c)
                    {
                        dl[c].push (c ? xr : xl);
                        const float m = 0.5f + 0.5f * (c ? lB : lA);
                        (c ? wr : wl) = dl[c].read ((float) ((0.0006f + sw * m) * sr) + 2.0f);
                    }
                    break;
                }
            }
            lfo.advance (inc);
            // dry/wet: tremolo, phaser, flanger e dimension sono gia "effetto completo" -> crossfade lineare; gli altri a potenza costante
            float ol, orr;
            if (S.fx == TREMOLO || S.fx == PHASER || S.fx == FLANGER || S.fx == DIMENSION) { ol = xl + (wl - xl) * mix; orr = xr + (wr - xr) * mix; }
            else { ol = xl * dryG + wl * wetG; orr = xr * dryG + wr * wetG; if (S.fx == VIBRATO) { ol = xl * (1 - mix) + wl * mix; orr = xr * (1 - mix) + wr * mix; } }
            // larghezza e bassi mono (sul segnale finale)
            if (S.width != 1.0f || S.bassMono > 21.0f)
            {
                const float mid = 0.5f * (ol + orr); float side = 0.5f * (ol - orr) * S.width;
                if (S.bassMono > 21.0f) side = sideHp[0].run (side);
                ol = mid + side; orr = mid - side;
            }
            // autopan a potenza costante
            if (S.panOn)
            {
                const float pp = panLfo.at (panLfo.ph, S.panShape) * S.panDepth;
                const float gl = std::cos ((pp + 1) * juce::MathConstants<float>::pi * 0.25f) * 1.41421356f, gr = std::sin ((pp + 1) * juce::MathConstants<float>::pi * 0.25f) * 1.41421356f;
                ol *= gl; orr *= gr;
                panNow = pp;
            }
            panLfo.advance (pInc);
            L[i] = ol * outG; R[i] = orr * outG;
        }
        lfoNow = lfo.at (lfo.ph, S.shape); lfoPh = (float) lfo.ph; panPh = (float) panLfo.ph;
        if (! S.panOn) panNow = 0;
        for (int v = 0; v < 6; ++v) voicePh[v] = (float) vl[v].at (vl[v].ph, RANDOM);
    }
    // per l'interfaccia
    std::atomic<float> lfoNow { 0 }, lfoPh { 0 }, panNow { 0 }, panPh { 0 }, voicePh[6] {};
private:
    struct Ch { float ap[12] {}; float apA = 0, fbS = 0, flFb = 0; };
    double sr = 44100, dimPh = 0; int lastFx = -1;
    Delay dl[2]; MicroShift ms[2]; Ch ch[2]; OnePole wetLp[2]; Hp1 sideHp[1];
    Lfo lfo, panLfo, vl[12];
};
}
