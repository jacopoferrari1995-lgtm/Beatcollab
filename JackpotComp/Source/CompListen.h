#pragma once
// ASCOLTO: il plugin ascolta qualche secondo di traccia, misura cosa c'e' davvero (picchi, dinamica, attacchi, bassi/alti)
// e propone un rack tarato su quei numeri. Poi, applicato, regola da solo le soglie finche' ogni stadio lavora quanto deve.
#include "CompDSP.h"

namespace jc
{
enum Src { SRC_AUTO = 0, SRC_VOCE, SRC_BATT, SRC_BASSO, SRC_BUS, kNumSrc };
inline const char* srcName (int s) { static const char* n[] = { "AUTO", "VOCE", "BATTERIA", "BASSO / 808", "BUS / MIX" }; return n[juce::jlimit (0, kNumSrc - 1, s)]; }

// ---------------------------------------------------------------------------------------------------- cattura (thread audio)
struct Capture
{
    static constexpr int kMax = 900, kNeed = 800;   // 8 secondi di segnale vero (i silenzi non contano)
    float pk[kMax] {}, ms[kMax] {}, lo[kMax] {}, hi[kMax] {};
    std::atomic<int> n { 0 }; std::atomic<bool> on { false };
    double sr = 44100; int fLen = 441, fPos = 0; float fPk = 0; double fMs = 0, fLo = 0, fHi = 0; float lp1 = 0, lp2 = 0;

    void prepare (double s) { sr = s; fLen = juce::jmax (64, (int) (s * 0.01)); fPos = 0; fPk = 0; fMs = fLo = fHi = 0; lp1 = lp2 = 0; }
    void start() { n = 0; on = true; }
    void push (const float* L, const float* R, int N)
    {
        if (! on.load()) return;
        const float a1 = 1.0f - std::exp (-2.0f * juce::MathConstants<float>::pi * 150.0f / (float) sr);
        const float a2 = 1.0f - std::exp (-2.0f * juce::MathConstants<float>::pi * 4000.0f / (float) sr);
        for (int i = 0; i < N; ++i)
        {
            const float m = 0.5f * (L[i] + R[i]);
            lp1 += a1 * (m - lp1); lp2 += a2 * (m - lp2); const float h = m - lp2;
            fPk = juce::jmax (fPk, std::abs (L[i]), std::abs (R[i])); fMs += m * m; fLo += lp1 * lp1; fHi += h * h;
            if (++fPos >= fLen)
            {
                const float msF = (float) (fMs / fLen);
                const int k = n.load();
                if (msF > 1.0e-6f && k < kMax) { pk[k] = fPk; ms[k] = msF; lo[k] = (float) (fLo / fLen); hi[k] = (float) (fHi / fLen); n = k + 1; if (k + 1 >= kNeed) on = false; }
                fPos = 0; fPk = 0; fMs = fLo = fHi = 0;
            }
        }
    }
};

// ---------------------------------------------------------------------------------------------------- proposta
struct PlanStage { int model = FET76, ratio = 0; float att = 0.5f, rel = 0.5f, target = 4, mix = 1, color = 0.3f, drive = 0.3f; juce::String what, why; };
struct Plan
{
    bool valid = false; int src = SRC_BUS; juce::String srcWhy;
    float level = -60, peak = -60, crest = 0, dr = 0, onsets = 0, sharp = 0, loPct = 0, hiPct = 0, gaps = 0;
    juce::StringArray heard;              // cosa ho sentito, con i numeri
    std::vector<PlanStage> stages;        // il rack proposto, in ordine
    float scHpf = 20; juce::StringArray notes;
};

namespace detail
{
inline float db (float x) { return 10.0f * std::log10 (juce::jmax (1.0e-12f, x)); }
inline float pct (std::vector<float> v, float p) { if (v.empty()) return -100; std::sort (v.begin(), v.end()); return v[(size_t) juce::jlimit (0, (int) v.size() - 1, (int) (p * (float) (v.size() - 1)))]; }
inline float fetAtt (float us) { return juce::jlimit (0.0f, 1.0f, std::log (us / 20.0f) / std::log (40.0f)); }
inline float fetRel (float msv) { return juce::jlimit (0.0f, 1.0f, std::log (juce::jlimit (50.0f, 1100.0f, msv) / 50.0f) / std::log (22.0f)); }
inline float disAtt (float msv) { return juce::jlimit (0.0f, 1.0f, std::log (msv / 0.05f) / std::log (600.0f)); }
inline float disRel (float msv) { return juce::jlimit (0.0f, 1.0f, std::log (juce::jlimit (50.0f, 3500.0f, msv) / 50.0f) / std::log (70.0f)); }
inline int vcaRelIdx (float msv) { static const float t[] = { 100, 300, 600, 1200 }; int b = 0; for (int k = 1; k < 4; ++k) if (std::abs (std::log (t[k] / msv)) < std::abs (std::log (t[b] / msv))) b = k; return b; }
inline juce::String n1 (float v) { return juce::String (v, 1); }
inline juce::String n0 (float v) { return juce::String (juce::roundToInt (v)); }
}

inline Plan analyse (const Capture& cap, int forced)
{
    using namespace detail;
    Plan P; const int N = cap.n.load(); if (N < 150) return P;
    std::vector<float> rms, pkv; double sMs = 0, sLo = 0, sHi = 0; float pMax = 0;
    for (int i = 0; i < N; ++i) { rms.push_back (db (cap.ms[i])); pkv.push_back (20.0f * std::log10 (juce::jmax (1.0e-6f, cap.pk[i]))); sMs += cap.ms[i]; sLo += cap.lo[i]; sHi += cap.hi[i]; pMax = juce::jmax (pMax, cap.pk[i]); }
    P.level = db ((float) (sMs / N)); P.peak = 20.0f * std::log10 (juce::jmax (1.0e-6f, pMax));
    const float rMed = pct (rms, 0.5f), rHi = pct (rms, 0.9f);
    P.crest = pct (pkv, 0.97f) - rMed;                                        // picchi tipici sopra il corpo
    { std::vector<float> w; for (int i = 0; i + 30 <= N; i += 15) { double s = 0; for (int j = 0; j < 30; ++j) s += cap.ms[i + j]; w.push_back (db ((float) (s / 30))); }
      P.dr = pct (w, 0.9f) - pct (w, 0.15f); }                                  // frasi piano / frasi forti (finestre di 300 ms)
    int on = 0; float jump = 0; int last = -100;
    for (int i = 3; i < N; ++i)
    {
        const float prev = db ((cap.ms[i - 1] + cap.ms[i - 2] + cap.ms[i - 3]) / 3.0f), d = rms[(size_t) i] - prev;
        if (d > 5.0f && rms[(size_t) i] > rMed - 4 && i - last > 6) { ++on; jump += d; last = i; }
    }
    const float secs = (float) N * 0.01f; P.onsets = (float) on / secs; P.sharp = on > 0 ? jump / (float) on : 0;
    P.loPct = 100.0f * (float) (sLo / juce::jmax (1.0e-12, sMs)); P.hiPct = 100.0f * (float) (sHi / juce::jmax (1.0e-12, sMs));
    { int g = 0; for (auto r : rms) if (r < rHi - 20) ++g; P.gaps = 100.0f * (float) g / (float) N; }

    // ---- che cos'e'
    int s = forced;
    if (s == SRC_AUTO)
    {
        if (P.onsets > 1.4f && P.crest > 14 && P.sharp > 7 && (P.gaps > 12 || (P.loPct > 35 && P.hiPct > 6))) { s = SRC_BATT; P.srcWhy = n1 (P.onsets) + " colpi al secondo, secchi (+" + n1 (P.sharp) + " dB in 10 ms) e picchi " + n1 (P.crest) + " dB sopra il corpo: lo tratto come batteria."; }
        else if (P.loPct > 55) { s = SRC_BASSO; P.srcWhy = "Oltre la meta' dell'energia (" + n0 (P.loPct) + "%) sta sotto i 150 Hz: lo tratto come basso / 808."; }
        else if (P.loPct < 48 && P.dr > 4.5f && P.onsets > 1.0f && P.onsets < 8) { s = SRC_VOCE; P.srcWhy = "Pochi bassi, frasi che salgono e scendono di " + n1 (P.dr) + " dB: lo tratto come voce."; }
        else { s = SRC_BUS; P.srcWhy = "Spettro pieno e dinamica gia' contenuta: lo tratto come bus o mix."; }
    }
    else P.srcWhy = juce::String ("Hai scelto ") + srcName (s) + ": il rack segue i numeri qui sotto.";
    P.src = s;

    // ---- cosa ho sentito
    P.heard.add ("Livello medio " + n1 (P.level) + " dBFS, picco " + n1 (P.peak) + " dBFS.");
    P.heard.add ("Picchi " + n1 (P.crest) + " dB sopra il corpo: " + juce::String (P.crest > 17 ? "molto spigoloso" : P.crest > 12 ? "dinamico, normale per una ripresa" : P.crest > 8 ? "gia' abbastanza controllato" : "gia' compresso o limitato") + ".");
    P.heard.add ("Fra parti piano e forti " + n1 (P.dr) + " dB" + juce::String (P.dr > 9 ? ": salta molto, va livellato." : P.dr > 5 ? ": si muove, un livellatore aiuta." : ": e' gia' costante."));
    P.heard.add (n1 (P.onsets) + " attacchi al secondo, " + juce::String (P.sharp > 11 ? "molto secchi" : P.sharp > 7.5f ? "decisi" : "morbidi") + ". Bassi " + n0 (P.loPct) + "%, alti " + n0 (P.hiPct) + "% dell'energia.");

    const float ioi = P.onsets > 0.3f ? 1000.0f / P.onsets : 600.0f;   // ms fra un attacco e l'altro
    auto add = [&] (PlanStage st) { P.stages.push_back (st); };

    if (s == SRC_VOCE)
    {
        P.scHpf = P.loPct > 12 ? 110.0f : 80.0f;
        if (P.crest > 12.5f)
        {
            PlanStage f; f.model = FET76; f.ratio = P.crest > 18 ? 1 : 0;
            const float us = P.sharp > 10 ? 80.0f : P.sharp > 7.5f ? 200.0f : 400.0f, rel = juce::jlimit (60.0f, 350.0f, ioi * 0.45f);
            f.att = fetAtt (us); f.rel = fetRel (rel); f.target = juce::jlimit (2.5f, 7.0f, 0.45f * (P.crest - 9.0f)); f.drive = 0.25f; f.color = 0.25f;
            f.what = "FET 76  ratio " + juce::String (f.ratio ? "8" : "4") + "  att " + n0 (us) + " us  ril " + n0 (rel) + " ms  ~" + n1 (f.target) + " dB";
            f.why = "Prende i " + n1 (P.crest) + " dB di picchi sulle consonanti prima che escano dal mix. Rilascio a " + n0 (rel) + " ms: torna su prima della sillaba dopo (ne arriva una ogni " + n0 (ioi) + " ms).";
            add (f);
        }
        if (P.dr > 5.0f || P.crest <= 12.5f)
        {
            PlanStage o; o.model = OPTO2A; o.ratio = 0; o.target = juce::jlimit (1.5f, 6.0f, P.dr > 5 ? 0.45f * P.dr : 2.0f); o.drive = 0.35f; o.color = 0.35f;
            o.what = "OPTO 2A  COMPRESS  ~" + n1 (o.target) + " dB";
            o.why = P.dr > 5 ? "Avvicina le frasi piano a quelle forti (" + n1 (P.dr) + " dB di differenza) con un rilascio lento che non si sente."
                             : "La voce e' gia' controllata: qui serve solo colla e un po' di calore valvolare, niente di piu'.";
            add (o);
        }
        if (P.hiPct > 14) P.notes.add ("Alti al " + n0 (P.hiPct) + "%: dopo la compressione le S escono di piu'. Metti un de-esser dopo questo plugin.");
        if (P.loPct > 12) P.notes.add ("Bassi al " + n0 (P.loPct) + "% per una voce: HPF del sidechain a " + n0 (P.scHpf) + " Hz, cosi' i colpi di fiato non fanno pompare. Taglia anche con un EQ.");
    }
    else if (s == SRC_BATT)
    {
        P.scHpf = P.loPct > 40 ? 120.0f : 90.0f;
        PlanStage v; v.model = VCABUS; v.ratio = P.crest > 17 ? 1 : 0;
        const int ai = P.sharp > 9 ? 5 : 4; v.att = (ai + 0.5f) / 6.0f;
        const float relW = ioi * 0.7f; const int ri = P.onsets > 1.5f ? vcaRelIdx (relW) : 4; v.rel = (ri + 0.5f) / 5.0f;
        static const float relT[] = { 100, 300, 600, 1200 };
        v.target = juce::jlimit (1.5f, 4.5f, 0.28f * (P.crest - 8.0f)); v.drive = 0.3f; v.color = 0.2f;
        v.what = "VCA BUS  ratio " + juce::String (v.ratio ? "4" : "2") + "  att " + juce::String (ai == 5 ? "30" : "10") + " ms  ril " + (ri < 4 ? n0 (relT[ri]) + " ms" : juce::String ("AUTO")) + "  ~" + n1 (v.target) + " dB";
        v.why = "Colla: attacco a " + juce::String (ai == 5 ? "30" : "10") + " ms lascia passare la botta (attacchi " + juce::String (P.sharp > 9 ? "molto secchi" : "decisi") + "), "
              + (ri < 4 ? "rilascio a " + n0 (relT[ri]) + " ms per respirare a tempo, un colpo ogni " + n0 (ioi) + " ms." : juce::String ("rilascio AUTO perche' il ritmo e' irregolare."));
        add (v);
        if (P.crest > 15)
        {
            PlanStage d; d.model = DISTRESS; d.ratio = 7; d.att = disAtt (1.0f); d.rel = disRel (juce::jlimit (60.0f, 300.0f, ioi * 0.4f)); d.target = 10; d.mix = juce::jlimit (0.2f, 0.4f, 0.015f * P.crest); d.color = 0.6f; d.drive = 0.4f;
            d.what = "DISTRESS  NUKE  att 1 ms  mix " + n0 (d.mix * 100) + "%  ~10 dB";
            d.why = "Schiacciata parallela: con " + n1 (P.crest) + " dB di picchi la batteria ha transienti ma poco corpo. Mischiata al " + n0 (d.mix * 100) + "% alza stanza e coda senza perdere la botta.";
            add (d);
        }
        if (P.loPct > 40) P.notes.add ("La cassa pesa il " + n0 (P.loPct) + "% dell'energia: HPF del sidechain a " + n0 (P.scHpf) + " Hz, se no la cassa fa pompare tutto il resto.");
    }
    else if (s == SRC_BASSO)
    {
        P.scHpf = 20;
        if (P.crest > 11)
        {
            PlanStage f; f.model = FET76; f.ratio = 0; const float rel = juce::jlimit (120.0f, 600.0f, ioi * 0.5f);
            f.att = fetAtt (800.0f); f.rel = fetRel (rel); f.target = juce::jlimit (2.0f, 5.0f, 0.35f * (P.crest - 8.0f)); f.drive = 0.25f; f.color = 0.3f;
            f.what = "FET 76  ratio 4  att 800 us  ril " + n0 (rel) + " ms  ~" + n1 (f.target) + " dB";
            f.why = "Attacco lento (il massimo del 76) per non mangiare il click dell'attacco, rilascio sopra i 100 ms: sotto, sulle frequenze basse il compressore segue la forma d'onda e distorce.";
            add (f);
        }
        PlanStage r; r.model = RMS160; r.ratio = P.dr > 6 ? 2 : 1; r.target = juce::jlimit (2.5f, 7.0f, 0.6f * P.dr + 1.5f); r.drive = 0.3f;
        r.what = "RMS 160  ratio " + juce::String (r.ratio == 2 ? "4" : "3") + "  ~" + n1 (r.target) + " dB";
        r.why = "Segue l'energia e non i picchi: tiene il basso alla stessa altezza nota dopo nota (" + n1 (P.dr) + " dB di differenza adesso), cosi' resta sotto la cassa senza sparire.";
        add (r);
        if (P.loPct > 80) P.notes.add ("Quasi tutto sotto i 150 Hz (" + n0 (P.loPct) + "%): e' un 808 puro. Se vuoi che si senta sui telefoni aggiungi saturazione dopo (Alter o Tube).");
    }
    else
    {
        P.scHpf = 120;
        if (P.dr > 4 || P.crest > 12)
        {
            PlanStage m; m.model = VARIMU; m.ratio = P.onsets > 2 ? 1 : 4; m.target = juce::jlimit (1.0f, 3.0f, 0.3f * P.dr); m.drive = 0.2f; m.color = 0.35f;
            m.what = "VARI-MU  " + juce::String (m.ratio == 1 ? "TC2" : "TC5") + "  ~" + n1 (m.target) + " dB";
            m.why = juce::String (m.ratio == 1 ? "TC2 e' veloce abbastanza per seguire il groove" : "TC5 ha un rilascio che si adatta da solo, ideale su materiale lento") + "; " + n1 (P.dr) + " dB di dinamica: ne prendo solo " + n1 (m.target) + " per incollare senza appiattire.";
            add (m);
        }
        else
        {
            PlanStage v; v.model = VCABUS; v.ratio = 0; v.att = 5.5f / 6.0f; v.rel = 4.5f / 5.0f; v.target = 1.5f; v.drive = 0.25f;
            v.what = "VCA BUS  ratio 2  att 30 ms  ril AUTO  ~1.5 dB";
            v.why = "Dinamica gia' stretta (" + n1 (P.dr) + " dB) e picchi contenuti: basta una passata leggera di colla, di piu' toglierebbe vita.";
            add (v);
        }
        P.notes.add ("Su bus e mix HPF del sidechain a 120 Hz: il compressore non reagisce alla cassa e non pompa.");
        if (P.crest < 8) P.notes.add ("Picchi solo " + n1 (P.crest) + " dB sopra il corpo: e' gia' molto compresso o limitato. Valuta di togliere compressione prima, invece di aggiungerne.");
    }
    if (P.peak > -1.0f) P.notes.add ("Il segnale arriva a " + n1 (P.peak) + " dBFS: abbassalo in ingresso di qualche dB, i modelli a INPUT fisso lavorano meglio con un po' di margine.");
    if (P.level < -30) P.notes.add ("Livello molto basso (" + n1 (P.level) + " dBFS): alzalo prima del plugin, i modelli a INPUT fisso faticano ad arrivare in zona.");
    P.valid = true;
    return P;
}
}
