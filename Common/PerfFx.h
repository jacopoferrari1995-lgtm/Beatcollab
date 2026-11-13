#pragma once
// Effetti da performance: 8 slot, in ogni slot un effetto a scelta, una manopola sola (0..1).
// Lo stesso codice serve al Sampler e servira' al plugin Jackpot FX: e' il "motore" degli effetti.
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <cmath>
#include "FxSamplesData.h"
#include "PerfVerb.h"

namespace pfx
{
enum Type { OFF, FILTER, GAIN, REVERB, DELAY, SCRATCH, TAPESTOP, STUTTER, GLITCH, GATE, CRUSH, LOFI, TAPE, VINYL,
            FLANGER, PHASER, CHORUS, DIMENSION, TREMOLO, AUTOPAN, WIDE,
            // aggiunti dopo (in fondo, cosi' i progetti salvati restano giusti)
            GATEMIX, STUTMIX, BEATROLL, ROLLMIX, TREMMIX, SCRATCHHAND, BEATREPEAT, RISER, DOWNLIFTER, REVERSE, HALFSPEED, PITCH, PUMP, FREEZE,
            RISERSYN, SNAREROLL, SUBDROP, kNumTypes };
// effetti che "muovono il tempo" (riletture del passato): suonano per primi, cosi' gli altri effetti si sommano sopra (freeze + reverb, ecc.)
inline bool isTimeFx (int t)
{
    return t == FREEZE || t == SCRATCH || t == SCRATCHHAND || t == TAPESTOP || t == STUTTER || t == BEATROLL || t == BEATREPEAT || t == GLITCH || t == REVERSE || t == HALFSPEED;
}
// campioni di transizione (veri, renderizzati e inclusi nel plugin), decodificati una volta sola
struct FxSample { juce::AudioBuffer<float> buf; double sr = 44100; };
inline const FxSample& fxSample (int which)
{
    struct Bank
    {
        FxSample s[5];
        Bank()
        {
            const void* d[5] = { FxSamplesData::riser_noise_ogg, FxSamplesData::riser_synth_ogg, FxSamplesData::riser_snareroll_ogg, FxSamplesData::down_noise_ogg, FxSamplesData::down_subdrop_ogg };
            const int sz[5] = { FxSamplesData::riser_noise_oggSize, FxSamplesData::riser_synth_oggSize, FxSamplesData::riser_snareroll_oggSize, FxSamplesData::down_noise_oggSize, FxSamplesData::down_subdrop_oggSize };
            juce::OggVorbisAudioFormat ogg;
            for (int i = 0; i < 5; ++i)
                if (std::unique_ptr<juce::AudioFormatReader> r (ogg.createReaderFor (new juce::MemoryInputStream (d[i], (size_t) sz[i], false), true)); r != nullptr)
                {
                    s[i].sr = r->sampleRate; s[i].buf.setSize (2, (int) r->lengthInSamples); r->read (&s[i].buf, 0, (int) r->lengthInSamples, 0, true, true);
                }
        }
    };
    static Bank bank; return bank.s[juce::jlimit (0, 4, which)];
}
// famiglie con la seconda manopola MIX: velocita' su una manopola, quantita' su un'altra
enum Family { F_GATE, F_STUT, F_ROLL, F_TREM, kFamilies };
inline int familyOfMix (int t) { return t == GATEMIX ? F_GATE : t == STUTMIX ? F_STUT : t == ROLLMIX ? F_ROLL : t == TREMMIX ? F_TREM : -1; }
// velocita' a zone: 1/4, 1/8, 1/8T, 1/16, 1/16T, 1/32, 1/64 (in battiti)
inline int rateZone (float v) { return v <= 0.01f ? -1 : juce::jlimit (0, 6, (int) ((v - 0.01f) / 0.99f * 7.0f)); }
inline double rateBeats (int z) { static const double r[7] = { 1.0, 0.5, 1.0 / 3.0, 0.25, 1.0 / 6.0, 0.125, 0.0625 }; return r[juce::jlimit (0, 6, z)]; }
inline const char* rateName (int z) { static const char* n[7] = { "1/4", "1/8", "1/8T", "1/16", "1/16T", "1/32", "1/64" }; return z < 0 ? "OFF" : n[juce::jlimit (0, 6, z)]; }

struct Info { const char* name; float neutral; bool spring; bool bipolar; const char* group; const char* tip; };
inline const Info& info (int t)
{
    static const Info I[kNumTypes] = {
        { "OFF",        0.0f, false, false, "",          "Slot vuoto." },
        { "FILTRO",     0.5f, false, true,  "FILTRI",    "Filtro da DJ: a sinistra passa basso, a destra passa alto, al centro spento." },
        { "GAIN",       0.5f, false, true,  "FILTRI",    "Volume da -12 a +12 dB, al centro 0." },
        { "REVERB",     0.0f, false, false, "SPAZIO",    "Riverbero grande sopra al suono." },
        { "DELAY",      0.0f, false, false, "SPAZIO",    "Delay ping pong a 1/8 puntato, a tempo con la DAW." },
        { "SCRATCH",    0.0f, true,  false, "TEMPO",     "Scratch automatico a tempo: piu giri, piu fitto (baby, chirp, transformer, anche in terzine). Molli e torna il disco." },
        { "TAPE STOP",  0.0f, true,  false, "TEMPO",     "Il nastro rallenta fino a fermarsi. Torna a zero e riparte." },
        { "STUTTER",    0.0f, true,  false, "TEMPO",     "Ripete l'ultimo pezzetto. La manopola e' la velocita: da 1/4 a 1/64, terzine comprese. Quanto si sente: STUTTER MIX." },
        { "GLITCH",     0.0f, true,  false, "TEMPO",     "Salti, ripetizioni e pezzi al contrario a tempo. Piu giri, piu succede." },
        { "GATE",       0.0f, true,  false, "TEMPO",     "Taglia il suono a tempo. La manopola e' la velocita: da 1/4 a 1/64, terzine comprese. Quanto taglia: GATE MIX." },
        { "CRUSH",      0.0f, false, false, "SPORCO",    "Meno bit: da granuloso a distrutto." },
        { "LO-FI",      0.0f, false, false, "SPORCO",    "Meno bit, meno campioni, meno alti: campionatore vecchio." },
        { "TAPE",       0.0f, false, false, "SPORCO",    "Nastro: saturazione morbida, wow e flutter." },
        { "VINYL",      0.0f, false, false, "SPORCO",    "Disco: fruscio, crepitio e alti arrotondati." },
        { "FLANGER",    0.0f, false, false, "MODULAZIONE", "Flanger: il jet che sale e scende." },
        { "PHASER",     0.0f, false, false, "MODULAZIONE", "Phaser: il suono che gira." },
        { "CHORUS",     0.0f, false, false, "MODULAZIONE", "Chorus: piu voci leggermente stonate, piu largo." },
        { "DIMENSION",  0.0f, false, false, "MODULAZIONE", "Dimension: allarga e addolcisce senza far ondeggiare." },
        { "TREMOLO",    0.0f, true,  false, "MODULAZIONE", "Volume che pulsa a tempo. La manopola e' la velocita (1/4 - 1/64). Quanto pulsa: TREMOLO MIX." },
        { "AUTOPAN",    0.0f, false, false, "MODULAZIONE", "Il suono va da sinistra a destra a tempo (1/4)." },
        { "WIDE",       0.5f, false, true,  "SPAZIO",    "A sinistra verso il mono, a destra piu largo." },
        { "GATE MIX",   1.0f, false, false, "TEMPO",     "Quanto taglia il GATE (seconda manopola). Senza questa voce il gate taglia tutto." },
        { "STUTTER MIX",1.0f, false, false, "TEMPO",     "Quanto si sente lo STUTTER sopra il pezzo (seconda manopola)." },
        { "BEAT ROLL",  0.0f, true,  false, "TEMPO",     "Roll a tempo che parte dalla griglia: piu giri, piu veloce, per salire verso il drop. Quanto si sente: BEAT ROLL MIX." },
        { "BEAT ROLL MIX",1.0f, false, false, "TEMPO",   "Quanto si sente il BEAT ROLL (seconda manopola)." },
        { "TREMOLO MIX",1.0f, false, false, "MODULAZIONE", "Quanto pulsa il TREMOLO (seconda manopola)." },
        { "SCRATCH MANO",0.5f, true, true,  "TEMPO",     "La manopola e' la mano sul vinile: muovila avanti e indietro. Ferma, il disco riparte." },
        { "BEAT REPEAT",0.0f, true,  false, "TEMPO",     "Come in Ableton: aspetta il battito, prende quel pezzo e lo ripete abbassandolo. La manopola sceglie la lunghezza (1/4, 1/8, 1/16)." },
        { "RISER",      0.0f, false, false, "TRANSIZIONI", "Rumore che sale per 2 battute: parte quando giri, la manopola e' il volume. Torna a zero per rilanciarlo." },
        { "DOWNLIFTER", 0.0f, false, false, "TRANSIZIONI", "Rumore che scende, per gli arrivi e le uscite. Parte quando giri, la manopola e' il volume." },
        { "REVERSE",    0.0f, true,  false, "TEMPO",     "L'ultima battuta suonata al contrario. La manopola e' il mix: piu giri, piu si sente il reverse." },
        { "HALF SPEED", 0.0f, true,  false, "TONO",      "Tutto a meta velocita, un'ottava sotto: il classico rallenty." },
        { "PITCH",      0.5f, false, true,  "TONO",      "Intonazione da -12 a +12 semitoni, al centro 0." },
        { "PUMP",       0.0f, false, false, "MODULAZIONE", "Sidechain finto: il volume pompa sui quarti, come con la cassa." },
        { "FREEZE",     0.0f, true,  false, "TEMPO",     "Ferma il suono in una nota sospesa, finche' tieni. Gli altri effetti si sommano sopra." },
        { "RISER SYNTH",0.0f, false, false, "TRANSIZIONI", "Synth che sale di due ottave con il filtro che apre, 2 battute. La manopola e' il volume." },
        { "SNARE ROLL", 0.0f, false, false, "TRANSIZIONI", "Rullata che accelera da ottavi a trentaduesimi, 2 battute. La manopola e' il volume." },
        { "SUB DROP",   0.0f, false, false, "TRANSIZIONI", "Sub che cade, per l'arrivo del drop. La manopola e' il volume." } };
    return I[juce::jlimit (0, kNumTypes - 1, t)];
}

// parametri avanzati (solo con il mouse, in Jackpot FX): fino a 6 per effetto, 0..1. I default riproducono il suono base.
constexpr int kAdv = 10;
struct AdvSpec { int n; const char* name[kAdv]; float def[kAdv]; };
inline const AdvSpec& adv (int t)
{
    static const AdvSpec none { 0, {}, {} };
    static const AdvSpec rv { 9, { "TIPO", "DECADIMENTO", "PRE-DELAY", "TAGLIA BASSI", "TAGLIA ALTI", "COLORE CODA", "RATE", "DEPTH", "WIDTH" }, { 0.08f, 0.5731f, 0.1f, 0, 1, 0.5f, 0.4515f, 0.35f, 0.6667f } };
    static const AdvSpec dl { 10, { "TEMPO", "FEEDBACK", "TAGLIA BASSI", "TAGLIA ALTI", "AGE", "DRIVE", "WOW", "FLUTTER", "SPREAD", "PING PONG" }, { 0.1875f, 0.5f, 0, 0.58f, 0, 0, 0, 0, 0, 1 } };
    static const AdvSpec wd { 1, { "FREQUENZA" }, { 0 } };
    static const AdvSpec fi { 3, { "RISONANZA", "PENDENZA", "DRIVE" }, { 0.3333f, 0, 0 } };
    static const AdvSpec ga { 3, { "FORMA", "APERTURA", "SWING" }, { 0.1236f, 0.5f, 0 } };
    static const AdvSpec st { 3, { "CALO", "PITCH", "SFUMA" }, { 0, 0.5f, 0.05f } };
    static const AdvSpec br { 1, { "CALO" }, { 0.36f } };
    static const AdvSpec tr { 2, { "FORMA", "STEREO" }, { 0, 0 } };
    static const AdvSpec cr { 3, { "CAMPIONI", "TONO", "VOLUME" }, { 0, 1, 1 } };
    static const AdvSpec lf { 3, { "RUMORE", "TONO", "VOLUME" }, { 0, 0.5f, 1 } };
    static const AdvSpec tp { 3, { "SATURA", "WOW", "VOLUME" }, { 0.5f, 0.5f, 1 } };
    static const AdvSpec vn { 3, { "FRUSCIO", "CREPITIO", "VOLUME" }, { 0.5f, 0.5f, 1 } };
    static const AdvSpec fl { 2, { "VELOCITA", "FEEDBACK" }, { 0.4002f, 0.6f } };
    static const AdvSpec ph { 2, { "VELOCITA", "FEEDBACK" }, { 0.4778f, 0.5f } };
    static const AdvSpec ch { 2, { "VELOCITA", "PROFONDITA" }, { 0.6157f, 0.5f } };
    static const AdvSpec dm { 2, { "VELOCITA", "PROFONDITA" }, { 0.4215f, 0.5f } };
    static const AdvSpec ts { 1, { "FRENATA" }, { 0.4376f } };
    static const AdvSpec rs { 1, { "DURATA" }, { 0.375f } };
    static const AdvSpec re { 1, { "DURATA" }, { 0.625f } };
    static const AdvSpec pu { 2, { "VELOCITA", "RILASCIO" }, { 0.5f, 0.477f } };
    static const AdvSpec fr { 1, { "GRANO" }, { 0.4628f } };
    switch (t)
    {
        case REVERB: return rv; case DELAY: return dl; case WIDE: return wd; case FILTER: return fi; case GATE: return ga;
        case STUTTER: case BEATROLL: return st; case BEATREPEAT: return br; case TREMOLO: return tr;
        case CRUSH: return cr; case LOFI: return lf; case TAPE: return tp; case VINYL: return vn;
        case FLANGER: return fl; case PHASER: return ph; case CHORUS: return ch; case DIMENSION: return dm;
        case TAPESTOP: return ts; case RISER: case RISERSYN: case SNAREROLL: return rs; case REVERSE: return re;
        case PUMP: return pu; case FREEZE: return fr;
        default: return none;
    }
}
inline int zoneOf (float v, int n) { return juce::jlimit (0, n - 1, (int) (v * (float) n)); }
inline double delayBeats (float v) { static const double b[8] = { 1.0, 0.75, 0.5, 1.0 / 3.0, 0.375, 0.25, 1.0 / 6.0, 0.125 }; return b[zoneOf (v, 8)]; }
inline double lfoHz (float v) { return 0.02 * std::pow (400.0, (double) v); }
inline float verbDecay (float v) { return (float) (0.2 * std::pow (100.0, (double) v)); }        // 0.2 - 20 s
inline double lfoRateVerb (float v) { return 0.05 * std::pow (100.0, (double) v); }              // 0.05 - 5 Hz
inline juce::String advText (int t, int i, float v)
{
    auto pct = [v] { return juce::String (juce::roundToInt (v * 100)) + "%"; };
    auto hz = [] (double f) { return f >= 1000 ? juce::String (f / 1000.0, 1) + " kHz" : juce::String (juce::roundToInt (f)) + " Hz"; };
    switch (t)
    {
        case REVERB:
            if (i == 0) return Verb::typeName (zoneOf (v, Verb::kTypes));
            if (i == 1) { const float d = verbDecay (v); return juce::String (d, d < 10 ? 2 : 1) + " s"; }
            if (i == 2) return juce::String (juce::roundToInt (v * 250)) + " ms";
            if (i == 3) return v < 0.01f ? juce::String ("OFF") : hz (20.0 * std::pow (40.0, (double) v));
            if (i == 4) return v > 0.99f ? juce::String ("OFF") : hz (20000.0 * std::pow (0.1, 1.0 - v));
            if (i == 5) return v < 0.35f ? "SCURO " + pct() : v > 0.65f ? "BRILLANTE " + pct() : pct();
            if (i == 6) return juce::String (lfoRateVerb (v), 2) + " Hz";
            if (i == 8) return juce::String (juce::roundToInt (v * 150)) + "%";
            return pct();
        case WIDE: return v < 0.01f ? juce::String ("TUTTO") : "da " + hz (20.0 * std::pow (50.0, (double) v));
        case DELAY:
        {
            static const char* n[8] = { "1/4", "1/8 .", "1/8", "1/8T", "1/16 .", "1/16", "1/16T", "1/32" };
            if (i == 0) return n[zoneOf (v, 8)];
            if (i == 2) return v < 0.01f ? juce::String ("OFF") : hz (20.0 * std::pow (40.0, (double) v));
            if (i == 3) return hz (1000.0 * std::pow (16.0, (double) v));
            if (i == 8) return "+" + juce::String (juce::roundToInt (v * 20)) + " ms";
            return pct();
        }
        case FILTER: if (i == 1) return v < 0.5f ? juce::String ("12 dB") : juce::String ("24 dB"); return pct();
        case STUTTER: case BEATROLL: if (i == 1) { const float st = (v - 0.5f) * 4; return std::abs (st) < 0.05f ? juce::String ("0") : (st > 0 ? "+" : "") + juce::String (st, 1) + " st"; } return pct();
        case FLANGER: case PHASER: case CHORUS: case DIMENSION: if (i == 0) return juce::String (lfoHz (v), 2) + " Hz"; return pct();
        case TAPESTOP: return juce::String (juce::roundToInt (10.0 * std::pow (60.0, (double) v) * 4)) + " ms";
        case RISER: case RISERSYN: case SNAREROLL: { static const char* n[4] = { "1 BATT", "2 BATT", "4 BATT", "8 BATT" }; return n[zoneOf (v, 4)]; }
        case REVERSE: { static const char* n[4] = { "1/4 BATT", "1/2 BATT", "1 BATT", "2 BATT" }; return n[zoneOf (v, 4)]; }
        case PUMP: if (i == 0) { static const char* n[3] = { "1/2", "1/4", "1/8" }; return n[zoneOf (v, 3)]; } return pct();
        case FREEZE: return juce::String (juce::roundToInt (50.0 * std::pow (20.0, (double) v))) + " ms";
        default: return pct();
    }
}

// ritardo modulabile, piccolo (fino a ~60 ms)
struct ModDelay
{
    std::vector<float> b[2]; int w = 0, n = 1;
    void prepare (double sr) { n = (int) (sr * 0.06) + 4; for (auto& x : b) x.assign ((size_t) n, 0.0f); w = 0; }
    void push (float l, float r) { b[0][(size_t) w] = l; b[1][(size_t) w] = r; w = (w + 1) % n; }
    float read (int c, float d) const   // d in campioni dietro all'ultimo scritto
    {
        float p = (float) w - 1 - d; while (p < 0) p += (float) n;
        const int a = juce::jmin (n - 1, (int) p); const float f = p - (float) a; const int a1 = (a + 1) % n;   // p puo' arrotondare a n esatto
        return b[c][(size_t) a] + (b[c][(size_t) a1] - b[c][(size_t) a]) * f;
    }
};

class Slot
{
public:
    void prepare (double s)
    {
        sr = s;
        hist.setSize (2, (int) (sr * 5)); hist.clear(); hw = 0;
        fxSample (0);   // decodifica i campioni di transizione adesso, non mentre suona
        dly.setSize (2, (int) (sr * 2)); dly.clear(); dw = 0;
        md.prepare (sr);
        verb.prepare (sr);
        reset();
    }
    void reset()
    {
        stOn = stuOn = scOn = false; stX = stuX = scX = 0; stSpeed = 1; scV = -1; glX = 0;
        for (int c = 0; c < 2; ++c) { ic1[c] = ic2[c] = ic3[c] = ic4[c] = rvLc[c] = rvHc[c] = 0; lfHold[c] = lfLp[c] = 0; tLp[c] = 0; vLp[c] = 0; for (auto& a : ap[c]) a = 0; dlLp[c] = 0; phFb[c] = 0; }
        dlHp[0] = dlHp[1] = 0; wdLp = vnLp = vnHp = crEnv = crLp = 0; wowPh = fltPh = 0; verb.reset();
        djMode = 0; fxG = 1; amtS = -1; phLfo = 0; free = 0;
        stuWait = brOn = rvOn = hsOn = frOn = plOn = plDone = false; plG = 0; brX = rvX = hsX = frX = 0; faderS = 1; psPh = 0; rsI1 = rsI2 = rsF = 0; lastPh = brLastPh = 1;
    }
    void setType (int t) { if (t != type) { type = t; reset(); } }
    int type = OFF;
    float a[kAdv] { 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f };   // parametri avanzati (li mette l'Engine)
    // amt: valore della manopola (0..1). bpm, ppq e playing: per gli effetti a tempo
    void process (float* L, float* R, int N, float amt, double bpm, double ppq, bool playing, const float* fam = nullptr)
    {
        auto mixOf = [fam] (int f) { return fam ? fam[f] : 1.0f; };
        // storia per gli effetti che rileggono il passato (sempre scritta, cosi' entrano senza buchi)
        if (type == OFF) { pushHist (L, R, N); return; }
        if (amtS < 0) amtS = amt;
        const double spb = 60.0 / juce::jmax (40.0, bpm) * sr;   // campioni per battito
        const double beat0 = playing ? ppq : free / spb; free += N;
        switch (type)
        {
            case FILTER:    filter (L, R, N, amt); pushHist (L, R, N); break;
            case GAIN:      { const float tg = juce::Decibels::decibelsToGain ((amt - 0.5f) * 24.0f); for (int i = 0; i < N; ++i) { fxG += (tg - fxG) * 0.002f; L[i] *= fxG; R[i] *= fxG; } pushHist (L, R, N); break; }
            case REVERB:    reverb (L, R, N, amt); pushHist (L, R, N); break;
            case DELAY:     delay (L, R, N, amt, spb); pushHist (L, R, N); break;
            case SCRATCH:   scratchAuto (L, R, N, amt, spb, beat0); break;
            case SCRATCHHAND: scratch (L, R, N, amt); break;
            case TAPESTOP:  tapeStop (L, R, N, amt); break;
            case STUTTER:   stutter (L, R, N, amt, spb, mixOf (F_STUT), beat0, false); break;
            case BEATROLL:  stutter (L, R, N, amt, spb, mixOf (F_ROLL), beat0, true); break;
            case GLITCH:    glitch (L, R, N, amt, spb, beat0); break;
            case GATE:      gate (L, R, N, amt, spb, beat0, mixOf (F_GATE)); pushHist (L, R, N); break;
            case CRUSH:     crush (L, R, N, amt); pushHist (L, R, N); break;
            case LOFI:      lofi (L, R, N, amt); pushHist (L, R, N); break;
            case TAPE:      tape (L, R, N, amt); pushHist (L, R, N); break;
            case VINYL:     vinyl (L, R, N, amt); pushHist (L, R, N); break;
            case FLANGER:   flanger (L, R, N, amt); pushHist (L, R, N); break;
            case PHASER:    phaser (L, R, N, amt); pushHist (L, R, N); break;
            case CHORUS:    chorus (L, R, N, amt, false); pushHist (L, R, N); break;
            case DIMENSION: chorus (L, R, N, amt, true); pushHist (L, R, N); break;
            case TREMOLO:   trem (L, R, N, amt, spb, beat0, mixOf (F_TREM)); pushHist (L, R, N); break;
            case AUTOPAN:   tremolo (L, R, N, amt, spb, beat0, true); pushHist (L, R, N); break;
            case WIDE:      wide (L, R, N, amt); pushHist (L, R, N); break;
            case BEATREPEAT: beatRepeat (L, R, N, amt, spb, beat0); break;
            case RISER:     player (L, R, N, amt, 0, spb, true); pushHist (L, R, N); break;
            case RISERSYN:  player (L, R, N, amt, 1, spb, true); pushHist (L, R, N); break;
            case SNAREROLL: player (L, R, N, amt, 2, spb, true); pushHist (L, R, N); break;
            case DOWNLIFTER: player (L, R, N, amt, 3, spb, false); pushHist (L, R, N); break;
            case SUBDROP:   player (L, R, N, amt, 4, spb, false); pushHist (L, R, N); break;
            case REVERSE:   reverse (L, R, N, amt, spb); break;
            case HALFSPEED: halfSpeed (L, R, N, amt); break;
            case PITCH:     pitch (L, R, N, amt); pushHist (L, R, N); break;
            case PUMP:      pump (L, R, N, amt, spb, beat0); pushHist (L, R, N); break;
            case FREEZE:    freeze (L, R, N, amt); break;
            default:        pushHist (L, R, N); break;
        }
    }
private:
    double sr = 48000, free = 0;
    juce::AudioBuffer<float> hist, dly; int hw = 0, dw = 0;
    ModDelay md; juce::Random rnd { 7 };
    static constexpr int kChunk = 512; float wl[kChunk] {}, wr[kChunk] {}, phFb[2] {};
    // stati
    bool stOn = false, stuOn = false, scOn = false; float stX = 0, stuX = 0, scX = 0, stSpeed = 1; double stR = 0, stuStart = 0, stuPos = 0, scAnchor = 0; int stuLen = 0, scIdle = 0; float scV = -1, scGrab = 0;
    float ic1[2] {}, ic2[2] {}, djMode = 0, djFc = 1000, fxG = 1, amtS = -1; float lfHold[2] {}, lfLp[2] {}; int lfCnt = 0;
    float tLp[2] {}, vLp[2] {}, ap[2][6] {}, dlLp[2] {}; double phLfo = 0;
    int glStep = -1, glMode = 0; double glStart = 0; float glX = 0;
    bool stuWait = false; double lastPh = 1; int stuFill = 0; float faderS = 1;
    bool brOn = false; double brStart = 0, brLastPh = 1; int brPos = 0, brN = 0, brLen = 1; float brX = 0;
    bool rvOn = false; double rvStart = 0; int rvPos = 0; float rvX = 0;
    bool plOn = false, plDone = false; double plPos = 0; float plG = 0;
    bool hsOn = false; double hsR = 0; float hsX = 0;
    bool frOn = false; double frStart = 0; int frPh[3] {}, frOff[3] {}; float frX = 0;
    float psPh = 0, rsI1 = 0, rsI2 = 0, rsF = 0;
    float ic3[2] {}, ic4[2] {}, rvP[3] { -1, -1, -1 }, rvLc[2] {}, rvHc[2] {}, liftK = 1; int stuRep = 0;
    Verb verb; double wowPh = 0, fltPh = 0; float dlHp[2] {}, wdLp = 0, vnLp = 0, vnHp = 0, crEnv = 0, crLp = 0, crTone = 0.5f;

    void pushHist (const float* L, const float* R, int N)
    {
        const int len = hist.getNumSamples(); float* a = hist.getWritePointer (0); float* b = hist.getWritePointer (1);
        for (int i = 0; i < N; ++i) { a[hw] = L[i]; b[hw] = R[i]; if (++hw >= len) hw = 0; }
    }
    float rd (int c, double p) const
    {
        const int len = hist.getNumSamples(); while (p < 0) p += len; while (p >= len) p -= len;
        const int a = (int) p; const float f = (float) (p - a); const float* d = hist.getReadPointer (c);
        return d[a] + (d[(a + 1) % len] - d[a]) * f;
    }
    // ---------------------------------------------------------------- effetti
    void filter (float* L, float* R, int N, float amt)
    {
        const float x = (amt - 0.5f) * 2.0f;
        if (std::abs (x) <= 0.03f) { djMode = 0; for (int c = 0; c < 2; ++c) ic1[c] = ic2[c] = ic3[c] = ic4[c] = 0; return; }
        const float mode = x < 0 ? -1.0f : 1.0f, t = (std::abs (x) - 0.03f) / 0.97f;
        const float fc = x < 0 ? (float) (20000.0 * std::pow (0.008, (double) t)) : (float) (20.0 * std::pow (150.0, (double) t));
        if (mode != djMode) { djMode = mode; djFc = fc; for (int c = 0; c < 2; ++c) ic1[c] = ic2[c] = ic3[c] = ic4[c] = 0; }
        const float k = 2.0f - 1.8f * a[0], k2 = 1.4f; const bool s24 = a[1] >= 0.5f; const float drv = 1.0f + a[2] * 6.0f;
        for (int i = 0; i < N; ++i)
        {
            djFc += (fc - djFc) * 0.002f;
            const float g = (float) std::tan (juce::MathConstants<double>::pi * juce::jmin ((double) djFc, sr * 0.45) / sr);
            const float a1 = 1.0f / (1.0f + g * (g + k)), a2 = g * a1, a3 = g * a2;
            float* io[2] = { L + i, R + i };
            for (int c = 0; c < 2; ++c)
            {
                float v0 = *io[c]; if (a[2] > 0.01f) v0 = std::tanh (v0 * drv) / std::sqrt (drv);
                const float v3 = v0 - ic2[c], v1 = a1 * ic1[c] + a2 * v3, v2 = ic2[c] + a2 * ic1[c] + a3 * v3;
                ic1[c] = 2 * v1 - ic1[c]; ic2[c] = 2 * v2 - ic2[c];
                float y = mode < 0 ? v2 : v0 - k * v1 - v2;
                if (s24)
                {
                    const float b1 = 1.0f / (1.0f + g * (g + k2)), b2 = g * b1, b3 = g * b2;
                    const float u3 = y - ic4[c], u1 = b1 * ic3[c] + b2 * u3, u2 = ic4[c] + b2 * ic3[c] + b3 * u3;
                    ic3[c] = 2 * u1 - ic3[c]; ic4[c] = 2 * u2 - ic4[c];
                    y = mode < 0 ? u2 : y - k2 * u1 - u2;
                }
                *io[c] = y;
            }
        }
    }
    void reverb (float* L, float* R, int N, float amt)
    {
        if (amt < 0.005f && amtS < 0.005f) { amtS = amt; return; }
        Verb::Params vp;
        vp.type = zoneOf (a[0], Verb::kTypes); vp.decay = verbDecay (a[1]); vp.preMs = a[2] * 250.0f;
        vp.lowHz = a[3] < 0.01f ? 0.0f : (float) (20.0 * std::pow (40.0, (double) a[3]));
        vp.highHz = a[4] > 0.99f ? 0.0f : (float) (20000.0 * std::pow (0.1, 1.0 - a[4]));
        vp.color = a[5]; vp.rate = (float) lfoRateVerb (a[6]); vp.depth = a[7]; vp.width = a[8] * 1.5f;
        for (int off = 0; off < N; off += kChunk)
        {
            const int m = juce::jmin (kChunk, N - off);
            verb.process (L + off, R + off, wl, wr, m, vp);
            for (int i = 0; i < m; ++i) { amtS += (amt - amtS) * 0.002f; L[off + i] += wl[i] * amtS * 0.6f; R[off + i] += wr[i] * amtS * 0.6f; }
        }
    }
    void delay (float* L, float* R, int N, float amt, double spb)
    {   // delay a nastro: AGE consuma le ripetizioni (piu scure, piu strette, piu sporche), WOW / FLUTTER muovono il nastro, SPREAD apre lo stereo
        const int len = dly.getNumSamples(); const double dT = juce::jlimit (16.0, (double) len - 4 - 0.05 * sr, spb * delayBeats (this->a[0]));
        float* a = dly.getWritePointer (0); float* b = dly.getWritePointer (1);
        const float age = this->a[4], drv = this->a[5], wow = this->a[6], flt = this->a[7], spr = this->a[8], pp = this->a[9];
        const float fb = juce::jmin (0.95f, this->a[1]);
        const double lcHz = (this->a[2] < 0.01f ? 0.0 : 20.0 * std::pow (40.0, (double) this->a[2])) + 350.0 * age;
        const double hcHz = 1000.0 * std::pow (16.0, (double) this->a[3]) * (1.0 - 0.65 * age);
        const float lpA = (float) std::exp (-juce::MathConstants<double>::twoPi * hcHz / sr);
        const float hpA = lcHz > 1 ? (float) std::exp (-juce::MathConstants<double>::twoPi * lcHz / sr) : -1.0f;
        const float sd = 1.0f + drv * 6.0f + age * 1.5f;
        const double dR = juce::jlimit (16.0, (double) len - 4 - 0.05 * sr, dT + spr * 0.02 * sr);   // SPREAD: il destro arriva un po' dopo
        for (int i = 0; i < N; ++i)
        {
            amtS += (amt - amtS) * 0.002f;
            wowPh += 0.55 / sr; fltPh += 6.3 / sr; if (wowPh > 1) wowPh -= 1; if (fltPh > 1) fltPh -= 1;
            const double mod = (wow * 0.004 * std::sin (wowPh * juce::MathConstants<double>::twoPi) + flt * 0.0005 * (std::sin (fltPh * juce::MathConstants<double>::twoPi) + 0.4 * std::sin (fltPh * 2.7 * juce::MathConstants<double>::twoPi))) * sr;
            auto rdD = [&] (const float* buf, double dd) { double rp = dw - dd - mod; while (rp < 0) rp += len; const int i0 = (int) rp % len; const float fr = (float) (rp - std::floor (rp)); const int i1 = (i0 + 1) % len; return buf[i0] + (buf[i1] - buf[i0]) * fr; };
            const float yL = rdD (a, dT), yR = rdD (b, dR);
            float f2[2] = { yR, yL };   // incrociati: ping pong
            for (int c = 0; c < 2; ++c)
            {
                dlLp[c] = f2[c] + lpA * (dlLp[c] - f2[c]); float v = dlLp[c];
                if (hpA >= 0) { dlHp[c] = v + hpA * (dlHp[c] - v); v -= dlHp[c]; }
                if (drv > 0.01f || age > 0.01f) v = std::tanh (v * sd) / sd * (1.0f + 0.25f * drv);
                if (age > 0.01f) v += (rnd.nextFloat() * 2 - 1) * 0.0015f * age;
                f2[c] = v;
            }
            const float sendIn = amtS, m = 0.5f * (L[i] + R[i]);
            const float fa = fb * (pp * f2[0] + (1 - pp) * f2[1]), fbb = fb * (pp * f2[1] + (1 - pp) * f2[0]);
            a[dw] = (pp * m + (1 - pp) * L[i]) * sendIn + fa; b[dw] = (1 - pp) * R[i] * sendIn + fbb;
            if (++dw >= len) dw = 0;
            L[i] += yL * amtS; R[i] += yR * amtS;
        }
    }
    void scratch (float* L, float* R, int N, float amt)
    {
        const int len = hist.getNumSamples(); float* b0 = hist.getWritePointer (0); float* b1 = hist.getWritePointer (1);
        if (scV < 0) scV = amt;
        const float sm = (float) (1.0 - std::exp (-1.0 / (0.012 * sr))); const double range = 0.9 * sr;
        for (int i = 0; i < N; ++i)
        {
            b0[hw] = L[i]; b1[hw] = R[i];
            const float prev = scV; scV += (amt - scV) * sm;
            if (std::abs (scV - prev) > 2e-6f || std::abs (amt - scV) > 1e-4f)
            { if (! scOn) { scOn = true; scAnchor = hw - 0.12 * sr; if (scAnchor < 0) scAnchor += len; scGrab = scV; } scIdle = 0; }
            else if (scOn && ++scIdle > (int) (0.25 * sr)) scOn = false;
            if (scOn || scX > 0)
            {
                double p = scAnchor + (scV - scGrab) * range; double lag = hw - p; while (lag < 0) lag += len; while (lag >= len) lag -= len; if (lag < 2) p = hw - 2;
                const float tl = rd (0, p), tr = rd (1, p);
                scX = juce::jlimit (0.0f, 1.0f, scX + (scOn ? 1.0f : -1.0f) / (float) (0.02 * sr));
                L[i] += (tl - L[i]) * scX; R[i] += (tr - R[i]) * scX;
            }
            if (++hw >= len) hw = 0;
        }
    }
    void tapeStop (float* L, float* R, int N, float amt)
    {
        const int len = hist.getNumSamples(); float* b0 = hist.getWritePointer (0); float* b1 = hist.getWritePointer (1);
        const float target = 1.0f - juce::jlimit (0.0f, 1.0f, amt);
        if (amt > 0.01f && ! stOn) { stOn = true; stR = hw; stSpeed = 1; }
        const float sc = (float) (1.0 - std::exp (-1.0 / (0.01 * std::pow (60.0, (double) a[0]) * sr)));
        for (int i = 0; i < N; ++i)
        {
            b0[hw] = L[i]; b1[hw] = R[i];
            if (stOn || stX > 0)
            {
                stSpeed += ((stOn ? target : 1.0f) - stSpeed) * sc;
                stR += stSpeed; if (stR >= len) stR -= len;
                double lag = hw - stR; if (lag < 0) lag += len; if (lag > len - 4) stR = hw;
                const float g = juce::jmin (1.0f, stSpeed * 3.0f);
                const float tl = rd (0, stR) * g, tr = rd (1, stR) * g;
                stX = juce::jlimit (0.0f, 1.0f, stX + (stOn ? 1.0f : -1.0f) / (float) (0.03 * sr));
                L[i] += (tl - L[i]) * stX; R[i] += (tr - R[i]) * stX;
            }
            if (++hw >= len) hw = 0;
        }
        if (amt <= 0.01f) stOn = false;
    }
    void stutter (float* L, float* R, int N, float amt, double spb, float mix, double beat0, bool roll)
    {   // STUTTER: prende subito l'ultimo pezzo. BEAT ROLL: aspetta la griglia e ripete da li' (il roll classico)
        const int len = hist.getNumSamples(); float* b0 = hist.getWritePointer (0); float* b1 = hist.getWritePointer (1);
        const int z = rateZone (amt);
        const int want = juce::jlimit (32, len - 8, (int) (rateBeats (juce::jmax (0, z)) * spb));
        const float fade = (float) ((0.0005 + a[2] * 0.03) * sr), semi = (a[1] - 0.5f) * 4.0f, dec = 1.0f - a[0] * 0.6f;
        for (int i = 0; i < N; ++i)
        {
            b0[hw] = L[i]; b1[hw] = R[i];
            if (z >= 0 && ! stuOn && ! stuWait)
            {
                if (! roll) { stuOn = true; stuStart = hw - want; if (stuStart < 0) stuStart += len; stuPos = 0; stuRep = 0; }
                else stuWait = true;
            }
            if (stuWait)
            {   // parte esattamente sul prossimo passo della griglia, e ripete quello che arriva da li'
                const double ph = std::fmod ((beat0 + i / spb) / rateBeats (juce::jmax (0, z)), 1.0);
                if (ph < lastPh || z < 0) { stuWait = false; if (z >= 0) { stuOn = true; stuStart = hw; stuPos = 0; stuFill = 0; stuRep = 0; } }
                lastPh = ph;
            }
            if (stuOn) { stuLen = want; if (stuPos >= stuLen) stuPos = std::fmod (stuPos, (double) stuLen); }   // cambi velocita' mentre ripete: niente salti
            if (stuOn || stuX > 0)
            {
                // nel roll il primo giro e' il suono vivo che riempie il pezzo, poi si ripete
                const bool live = roll && stuFill < stuLen;
                float tl = L[i], tr = R[i];
                if (! live)
                {
                    const float w = juce::jlimit (0.0f, 1.0f, juce::jmin ((float) stuPos / fade, (float) (stuLen - stuPos) / fade)) * (stuRep > 0 && dec < 1 ? std::pow (dec, (float) stuRep) : 1.0f);
                    tl = rd (0, stuStart + stuPos) * w; tr = rd (1, stuStart + stuPos) * w;
                }
                stuX = juce::jlimit (0.0f, 1.0f, stuX + (stuOn ? 1.0f : -1.0f) / (float) (0.005 * sr));
                const float m = stuX * mix;
                L[i] += (tl - L[i]) * m; R[i] += (tr - R[i]) * m;
                const double spd = (live || std::abs (semi) < 0.05f) ? 1.0 : juce::jlimit (0.25, 4.0, std::pow (2.0, semi * stuRep / 12.0));
                stuPos += spd; if (live) ++stuFill; if (stuPos >= stuLen) { stuPos = 0; if (! live) ++stuRep; else if (stuFill >= stuLen) stuRep = 1; }
            }
            if (++hw >= len) hw = 0;
        }
        if (z < 0) { stuOn = false; stuWait = false; }
    }
    void scratchAuto (float* L, float* R, int N, float amt, double spb, double beat0)
    {   // scratch a tempo: 0 baby 1/8, 1 baby 1/8T, 2 chirp 1/16, 3 chirp 1/16T, 4 transformer 1/16, 5 transformer 1/32
        const int len = hist.getNumSamples(); float* b0 = hist.getWritePointer (0); float* b1 = hist.getWritePointer (1);
        const int z = amt <= 0.01f ? -1 : juce::jlimit (0, 5, (int) ((amt - 0.01f) / 0.99f * 6.0f));
        static const double stepB[6] = { 0.5, 1.0 / 3.0, 0.25, 1.0 / 6.0, 0.25, 0.125 };
        if (z >= 0 && ! scOn) { scOn = true; scAnchor = hw - 0.35 * sr; if (scAnchor < 0) scAnchor += len; }
        if (z < 0) scOn = false;
        for (int i = 0; i < N; ++i)
        {
            b0[hw] = L[i]; b1[hw] = R[i];
            if (scOn || scX > 0)
            {
                const double sb = stepB[juce::jmax (0, z)], ph = std::fmod ((beat0 + i / spb) / sb, 1.0);
                const double A = juce::jmin (0.18 * sr, sb * spb * 0.45);                 // ampiezza del gesto avanti/indietro
                const double tri = ph < 0.5 ? ph * 2.0 : 2.0 - ph * 2.0;
                const double ease = 0.5 - 0.5 * std::cos (tri * juce::MathConstants<double>::pi);   // la mano accelera e frena
                double p = scAnchor + ease * A;
                float fader = 1.0f;
                if (z == 2 || z == 3) fader = ph < 0.5 ? 1.0f : 0.0f;                          // chirp: si sente solo l'andata
                if (z >= 4) fader = std::fmod (ph * 4.0, 1.0) < 0.5 ? 1.0f : 0.0f;             // transformer: fader che apre e chiude
                faderS += (fader - faderS) * (float) (1.0 - std::exp (-1.0 / (0.0015 * sr)));
                const float tl = rd (0, p) * faderS, tr = rd (1, p) * faderS;
                scX = juce::jlimit (0.0f, 1.0f, scX + (scOn ? 1.0f : -1.0f) / (float) (0.01 * sr));
                L[i] += (tl - L[i]) * scX; R[i] += (tr - R[i]) * scX;
            }
            if (++hw >= len) hw = 0;
        }
    }
    void beatRepeat (float* L, float* R, int N, float amt, double spb, double beat0)
    {   // aspetta il battito, cattura il pezzo e lo ripete abbassando ogni volta
        const int len = hist.getNumSamples(); float* b0 = hist.getWritePointer (0); float* b1 = hist.getWritePointer (1);
        const int z = amt <= 0.01f ? -1 : juce::jlimit (0, 2, (int) ((amt - 0.01f) / 0.99f * 3.0f));
        const int want = juce::jlimit (32, len - 8, (int) ((z == 0 ? 1.0 : z == 1 ? 0.5 : 0.25) * spb));
        const float fade = (float) (0.002 * sr);
        for (int i = 0; i < N; ++i)
        {
            b0[hw] = L[i]; b1[hw] = R[i];
            const double ph = std::fmod (beat0 + i / spb, 1.0);
            if (z >= 0 && ! brOn && ph < brLastPh) { brOn = true; brStart = hw; brPos = 0; brN = 0; brLen = want; }
            brLastPh = ph;
            if (z < 0) brOn = false;
            if (brOn || brX > 0)
            {
                float tl = L[i], tr = R[i];
                if (brN > 0)
                {
                    const float w = juce::jlimit (0.0f, 1.0f, juce::jmin ((float) brPos / fade, (float) (brLen - brPos) / fade)) * std::pow (1.0f - a[0] * 0.5f, (float) (brN - 1));
                    tl = rd (0, brStart + brPos) * w; tr = rd (1, brStart + brPos) * w;
                }
                brX = juce::jlimit (0.0f, 1.0f, brX + (brOn ? 1.0f : -1.0f) / (float) (0.005 * sr));
                L[i] += (tl - L[i]) * brX; R[i] += (tr - R[i]) * brX;
                if (++brPos >= brLen) { brPos = 0; ++brN; }
            }
            if (++hw >= len) hw = 0;
        }
    }
    void reverse (float* L, float* R, int N, float amt, double spb)
    {   // l'ultima battuta al contrario, in loop finche' tieni; la manopola e' il mix
        const int len = hist.getNumSamples(); float* b0 = hist.getWritePointer (0); float* b1 = hist.getWritePointer (1);
        const int L1 = juce::jlimit (256, len - 64, (int) ((1 << zoneOf (a[0], 4)) * spb));   // 1/4, 1/2, 1 (default), 2 battute
        if (amt > 0.01f && ! rvOn) { rvOn = true; rvStart = hw; rvPos = 0; }
        if (amt <= 0.01f) rvOn = false;
        const float fade = (float) (0.006 * sr);
        for (int i = 0; i < N; ++i)
        {
            b0[hw] = L[i]; b1[hw] = R[i];
            const float tgt = rvOn ? amt : 0.0f;
            rvX += (tgt - rvX) * (float) (1.0 - std::exp (-1.0 / (0.01 * sr)));
            if (rvX > 1e-4f)
            {
                if (rvPos >= L1) rvPos %= L1;
                const float w = juce::jlimit (0.0f, 1.0f, juce::jmin ((float) rvPos / fade, (float) (L1 - rvPos) / fade));
                const double p = rvStart - 1 - rvPos;
                const float tl = rd (0, p) * w, tr = rd (1, p) * w;
                L[i] += (tl - L[i]) * rvX; R[i] += (tr - R[i]) * rvX;
                if (++rvPos >= L1) rvPos = 0;
            }
            if (++hw >= len) hw = 0;
        }
    }
    void player (float* L, float* R, int N, float amt, int which, double spb, bool sync)
    {   // campione di transizione: parte appena giri, la manopola e' il volume; finito, si riarma tornando a zero
        const auto& S = fxSample (which); const int n = S.buf.getNumSamples(); if (n < 2) return;
        if (amt > 0.01f && ! plOn && ! plDone) { plOn = true; plPos = 0; }
        if (amt <= 0.01f) { plDone = false; if (plOn && plG < 1e-3f) plOn = false; }
        const double inc = sync ? n / (4.0 * (1 << zoneOf (a[0], 4)) * spb) : S.sr / sr;   // i riser a tempo: 1, 2 (default), 4, 8 battute
        const float* a = S.buf.getReadPointer (0); const float* b = S.buf.getReadPointer (1);
        const float sm = (float) (1.0 - std::exp (-1.0 / (0.02 * sr)));
        static constexpr float kGain[5] = { 0.8f, 0.8f, 0.8f, 0.8f, 0.42f };
        const float vg = kGain[juce::jlimit (0, 4, which)];
        for (int i = 0; i < N; ++i)
        {
            plG += ((plOn && amt > 0.01f ? amt : 0.0f) - plG) * sm;
            if (! plOn) continue;
            const int k = (int) plPos; if (k >= n - 1) { plOn = false; plDone = true; break; }
            const float f = (float) (plPos - k);
            L[i] += (a[k] + (a[k + 1] - a[k]) * f) * plG * vg; R[i] += (b[k] + (b[k + 1] - b[k]) * f) * plG * vg;
            plPos += inc;
        }
        if (amt <= 0.01f && plG < 1e-3f) plOn = false;
    }
    void halfSpeed (float* L, float* R, int N, float amt)
    {
        const int len = hist.getNumSamples(); float* b0 = hist.getWritePointer (0); float* b1 = hist.getWritePointer (1);
        if (amt > 0.01f && ! hsOn) { hsOn = true; hsR = hw; }
        if (amt <= 0.01f) hsOn = false;
        for (int i = 0; i < N; ++i)
        {
            b0[hw] = L[i]; b1[hw] = R[i];
            if (hsOn || hsX > 0)
            {
                hsR += 0.5; double lag = hw - hsR; while (lag < 0) lag += len;
                if (lag > len - sr * 0.2) hsR = hw;   // buffer finito: si riaggancia
                const float tl = rd (0, hsR), tr = rd (1, hsR);
                hsX = juce::jlimit (0.0f, 1.0f, hsX + (hsOn ? 1.0f : -1.0f) / (float) (0.01 * sr));
                L[i] += (tl - L[i]) * hsX; R[i] += (tr - R[i]) * hsX;
            }
            if (++hw >= len) hw = 0;
        }
    }
    void freeze (float* L, float* R, int N, float amt)
    {   // tre grani che si sovrappongono sugli ultimi 200 ms: nota sospesa, senza clic
        const int len = hist.getNumSamples(); float* b0 = hist.getWritePointer (0); float* b1 = hist.getWritePointer (1);
        const int span = (int) (0.05 * std::pow (20.0, (double) a[0]) * sr), G = (int) (span * 0.6);
        if (amt > 0.01f && ! frOn) { frOn = true; frStart = hw - span; if (frStart < 0) frStart += len; for (int g = 0; g < 3; ++g) frPh[g] = g * G / 3; }
        if (amt <= 0.01f) frOn = false;
        for (int i = 0; i < N; ++i)
        {
            if (! frOn) { b0[hw] = L[i]; b1[hw] = R[i]; }
            if (frOn || frX > 0)
            {
                float tl = 0, tr = 0;
                for (int g = 0; g < 3; ++g)
                {
                    if (frPh[g] == 0) frOff[g] = rnd.nextInt (juce::jmax (1, span - G));
                    const float w = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) frPh[g] / (float) G);
                    const double p = frStart + frOff[g] + frPh[g];
                    tl += rd (0, p) * w * 0.67f; tr += rd (1, p) * w * 0.67f;
                    if (++frPh[g] >= G) frPh[g] = 0;
                }
                frX = juce::jlimit (0.0f, 1.0f, frX + (frOn ? 1.0f : -1.0f) / (float) (0.02 * sr));
                L[i] += (tl - L[i]) * frX; R[i] += (tr - R[i]) * frX;
            }
            if (frOn) { /* la storia resta ferma sul pezzo congelato */ }
            else if (++hw >= len) hw = 0;
        }
    }
    void pitch (float* L, float* R, int N, float amt)
    {   // pitch shifter a due teste che scorrono sul ritardo, incrociate
        const float semis = (amt - 0.5f) * 24.0f;
        if (std::abs (semis) < 0.05f) { for (int i = 0; i < N; ++i) md.push (L[i], R[i]); return; }
        const float ratio = std::pow (2.0f, semis / 12.0f), W = (float) (0.05 * sr);
        for (int i = 0; i < N; ++i)
        {
            md.push (L[i], R[i]);
            psPh += (1.0f - ratio); if (psPh >= W) psPh -= W; if (psPh < 0) psPh += W;
            const float d1 = psPh + 4.0f, d2 = std::fmod (psPh + W * 0.5f, W) + 4.0f;
            const float w1 = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * psPh / W), w2 = 1.0f - w1;
            L[i] = md.read (0, d1) * w1 + md.read (0, d2) * w2; R[i] = md.read (1, d1) * w1 + md.read (1, d2) * w2;
        }
    }
    void pump (float* L, float* R, int N, float amt, double spb, double beat0)
    {
        if (amt <= 0.01f) return;
        for (int i = 0; i < N; ++i)
        {
            static const double pb[3] = { 2.0, 1.0, 0.5 };
            const double ph = std::fmod ((beat0 + i / spb) / pb[zoneOf (a[0], 3)], 1.0);
            const float env = (float) std::exp (-ph * 2.0 * std::pow (10.0, (double) a[1])), att = (float) juce::jmin (1.0, ph / 0.01);
            const float g = 1.0f - amt * 0.9f * env * att;
            L[i] *= g; R[i] *= g;
        }
    }
    void riser (float* L, float* R, int N, float amt, bool down)
    {   // rumore filtrato che sale (o scende) con un tono che glissa
        if (amt <= 0.01f) { rsF = 0; return; }
        const float t = down ? 1.0f - amt : amt;
        const float fc = 200.0f * std::pow (60.0f, t), lvl = down ? (1.0f - amt) * 0.25f : amt * amt * 0.28f;
        const float g = (float) std::tan (juce::MathConstants<double>::pi * juce::jmin ((double) fc, sr * 0.45) / sr), k = 0.6f;
        const float a1 = 1.0f / (1.0f + g * (g + k)), a2 = g * a1, a3 = g * a2;
        const float tone = 150.0f * std::pow (12.0f, t);
        for (int i = 0; i < N; ++i)
        {
            const float n = rnd.nextFloat() * 2 - 1;
            const float v3 = n - rsI2, v1 = a1 * rsI1 + a2 * v3, v2 = rsI2 + a2 * rsI1 + a3 * v3; rsI1 = 2 * v1 - rsI1; rsI2 = 2 * v2 - rsI2;
            rsF += tone / (float) sr; if (rsF > 1) rsF -= 1;
            const float tn = std::sin (juce::MathConstants<float>::twoPi * rsF) * 0.3f;
            const float o = (v1 * 1.4f + tn) * lvl;
            L[i] += o; R[i] += o;
        }
    }
    void trem (float* L, float* R, int N, float amt, double spb, double beat0, float depth)
    {
        const int z = rateZone (amt); if (z < 0) return;
        const double rb = rateBeats (z);
        for (int i = 0; i < N; ++i)
        {
            const double ph = std::fmod ((beat0 + i / spb) / rb, 1.0);
            auto curve = [this] (double q) { const float c = (float) (0.5 - 0.5 * std::cos (q * juce::MathConstants<double>::twoPi)); const float sq = q < 0.5 ? 0.0f : 1.0f; return c + a[0] * (sq - c); };
            const float gl = 1.0f - depth * curve (ph), gr = 1.0f - depth * curve (std::fmod (ph + a[1] * 0.5, 1.0));
            L[i] *= gl; R[i] *= gr;
        }
    }
    void glitch (float* L, float* R, int N, float amt, double spb, double beat0)
    {   // a ogni sedicesimo, con probabilita' = manopola: ripeti 1/32, pezzo al contrario, o salto di ottava giu
        const int len = hist.getNumSamples(); float* b0 = hist.getWritePointer (0); float* b1 = hist.getWritePointer (1);
        const double step = spb * 0.25;
        for (int i = 0; i < N; ++i)
        {
            b0[hw] = L[i]; b1[hw] = R[i];
            const double beat = beat0 + i / spb; const int st = (int) std::floor (beat * 4.0);
            if (st != glStep)
            {
                glStep = st;
                glMode = (amt > 0.01f && rnd.nextFloat() < amt * 0.85f) ? 1 + rnd.nextInt (3) : 0;
                glStart = hw;
            }
            const double inStep = (beat * 4.0 - st) * step;
            float tl = L[i], tr = R[i];
            if (glMode == 1) { const double sl = step * 0.5; const double p = glStart - sl + std::fmod (inStep, sl); tl = rd (0, p); tr = rd (1, p); }
            else if (glMode == 2) { const double p = glStart - inStep; tl = rd (0, p); tr = rd (1, p); }
            else if (glMode == 3) { const double p = glStart - step + inStep * 0.5; tl = rd (0, p); tr = rd (1, p); }
            const float target = glMode ? 1.0f : 0.0f;
            glX += (target - glX) * (float) (1.0 - std::exp (-1.0 / (0.002 * sr)));
            L[i] += (tl - L[i]) * glX; R[i] += (tr - R[i]) * glX;
            if (++hw >= len) hw = 0;
        }
    }
    void gate (float* L, float* R, int N, float amt, double spb, double beat0, float depth)
    {
        const int z = rateZone (amt); if (z < 0) return;
        const double rb = rateBeats (z); const float duty = 0.1f + a[1] * 0.8f;
        const float edge = juce::jmin (0.005f + a[0] * 0.445f, duty * 0.9f, (1.0f - duty) * 0.9f); const double sw = a[2] * 0.33;
        for (int i = 0; i < N; ++i)
        {
            const double t = (beat0 + i / spb) / rb; double ph = std::fmod (t, 1.0);
            if (sw > 0.001 && ((long long) std::floor (t) & 1)) ph = ph < sw ? 0.9999 : (ph - sw) / (1.0 - sw);   // swing: i passi dispari arrivano dopo
            float g = ph < duty ? 1.0f : 0.0f;
            if (ph < edge) g = (float) (ph / edge); else if (ph >= duty && ph < duty + edge) g = 1.0f - (float) ((ph - duty) / edge);
            g = 1.0f - depth * (1.0f - g);
            L[i] *= g; R[i] *= g;
        }
    }
    void crush (float* L, float* R, int N, float amt)
    {
        if (amt <= 0.01f) return;
        const float bits = 16.0f - amt * 13.0f, q = std::pow (2.0f, bits - 1); const int hold = 1 + (int) (a[0] * 15.0f);
        const bool tn = a[1] < 0.99f; const float c = tn ? (float) std::exp (-juce::MathConstants<double>::twoPi * 20000.0 * std::pow (0.08, 1.0 - a[1]) / sr) : 0;
        for (int i = 0; i < N; ++i)
        {
            if (++lfCnt >= hold) { lfCnt = 0; lfHold[0] = std::round (L[i] * q) / q; lfHold[1] = std::round (R[i] * q) / q; }
            L[i] = lfHold[0]; R[i] = lfHold[1];
            if (tn) { lfLp[0] = L[i] + c * (lfLp[0] - L[i]); lfLp[1] = R[i] + c * (lfLp[1] - R[i]); L[i] = lfLp[0]; R[i] = lfLp[1]; }
        }
        liftK = a[2]; lift (L, R, N, amt); liftK = 1;
    }
    void lofi (float* L, float* R, int N, float amt)
    {
        if (amt <= 0.01f) return;
        const float bits = 16.0f - amt * 10.0f, q = std::pow (2.0f, bits - 1); const int hold = 1 + (int) (amt * 7.0f);
        const float c = (float) std::exp (-juce::MathConstants<double>::twoPi * (16000.0 * std::pow (0.22, (double) amt) * std::pow (2.0, (a[1] - 0.5) * 2.0)) / sr);
        const float nz = a[0] * 0.02f * amt;
        for (int i = 0; i < N; ++i)
        {
            if (++lfCnt >= hold) { lfCnt = 0; lfHold[0] = std::round (L[i] * q) / q; lfHold[1] = std::round (R[i] * q) / q; }
            lfLp[0] = lfHold[0] + c * (lfLp[0] - lfHold[0]); lfLp[1] = lfHold[1] + c * (lfLp[1] - lfHold[1]);
            const float n = nz > 0 ? (rnd.nextFloat() * 2 - 1) * nz : 0.0f;
            L[i] = lfLp[0] + n; R[i] = lfLp[1] + n;
        }
        liftK = a[2]; lift (L, R, N, amt); liftK = 1;
    }
    // gli effetti "sporchi" tolgono energia: si recupera volume (fino a +3 dB) e un filo di saturazione, crescendo con la manopola
    void lift (float* L, float* R, int N, float amt)
    {
        if (amt <= 0.01f) return;
        const float g = juce::Decibels::decibelsToGain (4.5f * amt * liftK), d = 1.0f + 0.8f * amt;
        for (int i = 0; i < N; ++i) { L[i] = std::tanh (L[i] * g * d) / d; R[i] = std::tanh (R[i] * g * d) / d; }
    }
    void tape (float* L, float* R, int N, float amt)
    {
        if (amt <= 0.01f) { for (int i = 0; i < N; ++i) md.push (L[i], R[i]); return; }
        const float drive = 1.0f + amt * 8.0f * a[0], mk = 1.0f / drive, wk = a[1] * 2.0f;
        const float cf = (float) std::exp (-juce::MathConstants<double>::twoPi * (18000.0 - 9000.0 * amt) / sr);
        for (int i = 0; i < N; ++i)
        {
            md.push (L[i], R[i]);
            phLfo += 1.0 / sr;
            const float wow = (float) (std::sin (phLfo * juce::MathConstants<double>::twoPi * 0.6) * 0.0018 + std::sin (phLfo * juce::MathConstants<double>::twoPi * 7.3) * 0.00025) * amt * wk;
            const float d = (float) (0.004 * sr) + wow * (float) sr;
            float o[2] = { md.read (0, d), md.read (1, d) };
            for (int c = 0; c < 2; ++c) { const float s = std::tanh (o[c] * drive) * mk; tLp[c] = s + cf * (tLp[c] - s); o[c] = tLp[c]; }
            L[i] += (o[0] - L[i]) * juce::jmin (1.0f, amt * 2.0f); R[i] += (o[1] - R[i]) * juce::jmin (1.0f, amt * 2.0f);
        }
        liftK = a[2]; lift (L, R, N, amt); liftK = 1;
    }
    void vinyl (float* L, float* R, int N, float amt)
    {
        if (amt <= 0.01f) return;
        const float cf = (float) std::exp (-juce::MathConstants<double>::twoPi * (16000.0 - 10000.0 * amt) / sr);
        // fruscio: rumore rosa-ish nella banda del solco; crepitio: piccoli impulsi che decadono in fretta, con polarita' e colore a caso
        const float hiss = 0.010f * a[0], crk = 0.0009f * a[1];
        const float hsA = (float) std::exp (-juce::MathConstants<double>::twoPi * 5000.0 / sr), crD = (float) std::exp (-1.0 / (0.0006 * sr));
        for (int i = 0; i < N; ++i)
        {
            const float wn = rnd.nextFloat() * 2 - 1; vnLp = wn + hsA * (vnLp - wn); vnHp = vnLp + 0.995f * (vnHp - vnLp);
            float n = (vnLp - vnHp) * 2.2f * hiss * amt;
            if (rnd.nextFloat() < crk * amt) { crEnv = (0.25f + 0.75f * rnd.nextFloat()) * (rnd.nextBool() ? 1.0f : -1.0f) * 0.45f * amt; crTone = 0.3f + 0.6f * rnd.nextFloat(); }
            if (std::abs (crEnv) > 1e-4f) { crLp += (crEnv * (rnd.nextFloat() * 0.6f + 0.7f) - crLp) * crTone; n += crLp; crEnv *= crD; } else crLp *= 0.9f;
            for (int c = 0; c < 2; ++c) { float* x = c ? R : L; vLp[c] = x[i] + cf * (vLp[c] - x[i]); x[i] = vLp[c] + n * (c ? 0.92f : 1.0f); }
        }
        liftK = a[2]; lift (L, R, N, amt); liftK = 1;
    }
    void flanger (float* L, float* R, int N, float amt)
    {
        for (int i = 0; i < N; ++i)
        {
            phLfo += lfoHz (a[0]) / sr; const float l = (float) (0.5 + 0.5 * std::sin (phLfo * juce::MathConstants<double>::twoPi));
            const float d0 = (float) ((0.0006 + 0.004 * l) * sr), d1 = (float) ((0.0006 + 0.004 * (1 - l)) * sr);
            const float y0 = md.read (0, d0), y1 = md.read (1, d1);
            md.push (L[i] + y0 * juce::jmin (0.95f, a[1]) * amt, R[i] + y1 * juce::jmin (0.95f, a[1]) * amt);
            L[i] = L[i] * (1 - 0.5f * amt) + y0 * amt * 0.7f; R[i] = R[i] * (1 - 0.5f * amt) + y1 * amt * 0.7f;
        }
    }
    void phaser (float* L, float* R, int N, float amt)
    {
        if (amt <= 0.01f) return;
        for (int i = 0; i < N; ++i)
        {
            phLfo += lfoHz (a[0]) / sr; const float l = (float) (0.5 + 0.5 * std::sin (phLfo * juce::MathConstants<double>::twoPi));
            const float fc = 200.0f * std::pow (20.0f, l); const float t = (float) std::tan (juce::MathConstants<double>::pi * fc / sr), co = (t - 1) / (t + 1);
            float* io[2] = { L + i, R + i };
            for (int c = 0; c < 2; ++c)
            {
                float y = *io[c] + phFb[c] * juce::jmin (0.95f, a[1]) * amt;
                for (int k = 0; k < 6; ++k) { const float o = co * y + ap[c][k]; ap[c][k] = y - co * o; y = o; }
                phFb[c] = y;
                *io[c] = *io[c] * (1 - 0.5f * amt) + y * 0.5f * amt * 1.4f;
            }
        }
    }
    void chorus (float* L, float* R, int N, float amt, bool dim)
    {
        for (int i = 0; i < N; ++i)
        {
            md.push (L[i], R[i]);
            phLfo += lfoHz (a[0]) / sr; const double ph = phLfo * juce::MathConstants<double>::twoPi;
            const float dep = (float) ((dim ? 0.0012 : 0.003) * sr * a[1] * 2.0f), base = (float) ((dim ? 0.009 : 0.016) * sr);
            const float yl = md.read (0, base + dep * (float) std::sin (ph)), yr = md.read (1, base + dep * (float) std::sin (ph + juce::MathConstants<double>::pi));
            if (dim) { const float m = 0.5f * (yl + yr); L[i] += (0.5f * (yl - yr) + m * 0.3f) * amt * 0.8f; R[i] += (0.5f * (yr - yl) + m * 0.3f) * amt * 0.8f; }
            else { L[i] = L[i] * (1 - 0.35f * amt) + yl * amt * 0.6f; R[i] = R[i] * (1 - 0.35f * amt) + yr * amt * 0.6f; }
        }
    }
    void tremolo (float* L, float* R, int N, float amt, double spb, double beat0, bool pan)
    {
        if (amt <= 0.01f) return;
        for (int i = 0; i < N; ++i)
        {
            const double beat = beat0 + i / spb;
            if (pan) { const float p = (float) std::sin (beat * juce::MathConstants<double>::pi) * amt; L[i] *= std::sqrt (0.5f * (1 - p)) * 1.414f; R[i] *= std::sqrt (0.5f * (1 + p)) * 1.414f; }
            else { const float g = 1.0f - amt * (float) (0.5 + 0.5 * std::cos (beat * 2.0 * juce::MathConstants<double>::twoPi)); L[i] *= g; R[i] *= g; }
        }
    }
    void wide (float* L, float* R, int N, float amt)
    {
        const float w = amt < 0.5f ? amt * 2.0f : 1.0f + (amt - 0.5f) * 2.0f;
        if (std::abs (w - 1.0f) < 0.01f) return;
        // FREQUENZA: sotto resta com'e' (i bassi non si aprono), sopra si allarga o si stringe
        const bool split = a[0] >= 0.01f;
        const float c = split ? (float) std::exp (-juce::MathConstants<double>::twoPi * 20.0 * std::pow (50.0, (double) a[0]) / sr) : 0.0f;
        for (int i = 0; i < N; ++i)
        {
            const float m = 0.5f * (L[i] + R[i]); float s = 0.5f * (L[i] - R[i]);
            if (split) { wdLp = s + c * (wdLp - s); s = wdLp + (s - wdLp) * w; } else s *= w;
            L[i] = m + s; R[i] = m - s;
        }
    }
};

class Engine
{
public:
    static constexpr int kSlots = 8;
    Slot slot[kSlots];
    void prepare (double s) { sr = s; for (auto& x : slot) x.prepare (s); for (auto& m : mixS) m = -1; }
    // advs: kSlots * kAdv valori avanzati (nullptr = default dell'effetto). mixes: mix di ogni slot (nullptr = 100%)
    void process (float* L, float* R, int N, const int* types, const float* amts, double bpm, double ppq, bool playing,
                  const float* advs = nullptr, const float* mixes = nullptr)
    {
        // le voci "... MIX" non suonano: danno la quantita' all'effetto della loro famiglia (se non ci sono: 100%)
        float fam[kFamilies]; for (auto& f : fam) f = 1.0f;
        for (int k = 0; k < kSlots; ++k) if (const int f = familyOfMix (types[k]); f >= 0) fam[f] = amts[k];
        for (int k = 0; k < kSlots; ++k)
        {
            slot[k].setType (types[k]);
            const auto& sp = adv (types[k]);
            for (int i = 0; i < sp.n; ++i) slot[k].a[i] = advs ? juce::jlimit (0.0f, 1.0f, advs[k * kAdv + i]) : sp.def[i];
        }
        if (mixes == nullptr)
        {   // prima gli effetti che muovono il tempo (freeze, stutter, scratch...), poi tutto il resto: cosi' si sommano
            for (int pass = 0; pass < 2; ++pass)
                for (int k = 0; k < kSlots; ++k)
                    if (isTimeFx (types[k]) == (pass == 0)) slot[k].process (L, R, N, amts[k], bpm, ppq, playing, fam);
            return;
        }
        // con il mix per slot: a pezzi da 256, ogni slot si mescola al suo ingresso
        const double ppqPerSample = juce::jmax (40.0, bpm) / 60.0 / sr;
        for (int off = 0; off < N; off += kBlk)
        {
            const int m = juce::jmin (kBlk, N - off); float* l = L + off; float* r = R + off;
            const double q = ppq + (playing ? off * ppqPerSample : 0.0);
            for (int pass = 0; pass < 2; ++pass)
                for (int k = 0; k < kSlots; ++k)
                {
                    if (isTimeFx (types[k]) != (pass == 0)) continue;
                    const float tgt = juce::jlimit (0.0f, 1.0f, mixes[k]);
                    if (mixS[k] < 0) mixS[k] = tgt;
                    if (tgt >= 0.999f && mixS[k] >= 0.999f) { mixS[k] = 1; slot[k].process (l, r, m, amts[k], bpm, q, playing, fam); continue; }
                    for (int i = 0; i < m; ++i) { dl[i] = l[i]; dr[i] = r[i]; }
                    slot[k].process (l, r, m, amts[k], bpm, q, playing, fam);
                    const float m0 = mixS[k], st = (tgt - m0) / (float) m;
                    for (int i = 0; i < m; ++i) { const float g = m0 + st * (float) (i + 1); l[i] = dl[i] + (l[i] - dl[i]) * g; r[i] = dr[i] + (r[i] - dr[i]) * g; }
                    mixS[k] = tgt;
                }
        }
    }
private:
    static constexpr int kBlk = 256;
    double sr = 48000; float mixS[kSlots] {}; float dl[kBlk] {}, dr[kBlk] {};
};

// testo da mostrare sotto la manopola
inline juce::String valueText (int t, float v)
{
    switch (t)
    {
        case FILTER: { const float x = (v - 0.5f) * 2; return std::abs (x) <= 0.03f ? juce::String ("OFF") : x < 0 ? "LP " + juce::String (juce::roundToInt (-x * 100)) + "%" : "HP " + juce::String (juce::roundToInt (x * 100)) + "%"; }
        case GAIN: { const float d = (v - 0.5f) * 24; return (d > 0 ? "+" : "") + juce::String (d, 1) + " dB"; }
        case WIDE: return juce::String (juce::roundToInt (v * 200)) + "%";
        case SCRATCHHAND: return juce::String ("MUOVI");
        case SCRATCH: { static const char* n[6] = { "BABY 1/8", "BABY 1/8T", "CHIRP 1/16", "CHIRP 1/16T", "TRANSF 1/16", "TRANSF 1/32" }; return v <= 0.01f ? juce::String ("OFF") : juce::String (n[juce::jlimit (0, 5, (int) ((v - 0.01f) / 0.99f * 6.0f))]); }
        case STUTTER: case GATE: case BEATROLL: case TREMOLO: return juce::String (rateName (rateZone (v)));
        case BEATREPEAT: return v <= 0.01f ? juce::String ("OFF") : juce::String (v < 0.34f ? "1/4" : v < 0.67f ? "1/8" : "1/16");
        case REVERSE: return v <= 0.01f ? juce::String ("OFF") : "MIX " + juce::String (juce::roundToInt (v * 100)) + "%";
        case PITCH: { const float st = (v - 0.5f) * 24; return (st > 0 ? "+" : "") + juce::String (st, 1) + " st"; }
        case GATEMIX: case STUTMIX: case ROLLMIX: case TREMMIX: return juce::String (juce::roundToInt (v * 100)) + "%";
        case OFF: return juce::String ("-");
        default: return v <= 0.01f ? juce::String ("OFF") : juce::String (juce::roundToInt (v * 100)) + "%";
    }
}
}
