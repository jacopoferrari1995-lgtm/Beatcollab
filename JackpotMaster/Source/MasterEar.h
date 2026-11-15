#pragma once
// PROVA D'ASCOLTO del Master: ascolta 10 secondi del mix (dopo l'INGRESSO, prima della catena), misura tutto quello che conta
// per un master e propone una catena tarata su quei numeri, sul genere e (se c'e') sulla traccia di riferimento.
#include "MasterAnalysis.h"

namespace jms
{
struct EarCapture
{
    static constexpr int kMax = 1100, kNeed = 1000;   // 10 secondi di musica (i silenzi non contano)
    float pk[kMax] {}, ms[kMax] {};
    std::atomic<int> n { 0 }; std::atomic<bool> on { false };
    double sMid = 0, sSide = 0, sLoM = 0, sLoS = 0, sLR = 0, sLL = 0, sRR = 0;
    ThirdAnalyzer ana; std::vector<float> ring = std::vector<float> (4096, 0.0f); int fill = 0;
    double sr = 44100; int fLen = 441, fPos = 0; float fPk = 0; double fMs = 0; float lpM = 0, lpS = 0;

    void prepare (double s) { sr = s; fLen = juce::jmax (64, (int) (s * 0.01)); }
    void start() { n = 0; sMid = sSide = sLoM = sLoS = sLR = sLL = sRR = 0; ana.reset(); fill = 0; fPos = 0; fPk = 0; fMs = 0; on = true; }
    void push (const float* L, const float* R, int N)
    {
        if (! on.load()) return;
        const float a = 1.0f - std::exp (-2.0f * juce::MathConstants<float>::pi * 150.0f / (float) sr);
        for (int i = 0; i < N; ++i)
        {
            const float l = L[i], r = R[i], m = 0.5f * (l + r), s = 0.5f * (l - r);
            lpM += a * (m - lpM); lpS += a * (s - lpS);
            fPk = juce::jmax (fPk, std::abs (l), std::abs (r)); fMs += m * m;
            if (++fPos >= fLen)
            {
                const float msF = (float) (fMs / fLen); const int k = n.load();
                if (msF > 1.0e-7f && k < kMax) { pk[k] = fPk; ms[k] = msF; n = k + 1; if (k + 1 >= kNeed) on = false; }
                fPos = 0; fPk = 0; fMs = 0;
            }
            if (m * m > 1.0e-9f)
            {
                sMid += m * m; sSide += s * s; sLoM += lpM * lpM; sLoS += lpS * lpS; sLR += l * r; sLL += l * l; sRR += r * r;
                ring[(size_t) fill++] = m;
                if (fill == 4096) { ana.addFrame (ring.data(), sr); std::copy (ring.begin() + 2048, ring.end(), ring.begin()); fill = 2048; }
            }
        }
    }
};

// una mossa della catena proposta: quali parametri, con che valori, e perche'
struct EarStep { int mod = 0; juce::String title, why; std::vector<std::pair<juce::String, float>> set; };
struct EarPlan
{
    bool valid = false, useRef = false; int genre = 0;
    float level = -60, peak = -60, plr = 0, dr = 0, corr = 1, loSide = -60, sideDb = -60, reg[kRegions] {}, dev[kRegions] {}, resMax = 0, resHz = 0, targetLufs = -14, ceil = -1;
    juce::StringArray heard; std::vector<EarStep> steps; juce::StringArray notes;
};

namespace ear
{
inline float db (double x) { return 10.0f * (float) std::log10 (juce::jmax (1.0e-12, x)); }
inline float pct (std::vector<float> v, float p) { if (v.empty()) return -100; std::sort (v.begin(), v.end()); return v[(size_t) juce::jlimit (0, (int) v.size() - 1, (int) (p * (float) (v.size() - 1)))]; }
inline juce::String n1 (float v) { return (v > 0.05f ? "+" : "") + juce::String (v, 1); }
inline juce::String a1 (float v) { return juce::String (v, 1); }
}

// targetLufs: obiettivo scelto nel plugin. ref: traccia di riferimento (puo' essere nulla)
inline EarPlan earAnalyse (const EarCapture& C, int genreIdx, float targetLufs, float ceilDb, const RefTrack* ref)
{
    using namespace ear;
    EarPlan P; const int N = C.n.load(); if (N < 200 || ! C.ana.hasData()) return P;
    P.genre = genreIdx; P.targetLufs = targetLufs; P.ceil = ceilDb; P.useRef = ref != nullptr && ref->lufs > -60;
    std::vector<float> rms; double sm = 0; float pkMax = 0;
    for (int i = 0; i < N; ++i) { rms.push_back (db (C.ms[i])); sm += C.ms[i]; pkMax = juce::jmax (pkMax, C.pk[i]); }
    P.level = db (sm / N) - 0.7f;                     // circa LUFS (mono-somma, senza pesatura K: stima prudente)
    P.peak = 20.0f * std::log10 (juce::jmax (1.0e-6f, pkMax)); P.plr = P.peak - P.level;
    { std::vector<float> w; for (int i = 0; i + 30 <= N; i += 15) { double s = 0; for (int j = 0; j < 30; ++j) s += C.ms[i + j]; w.push_back (db (s / 30)); } P.dr = pct (w, 0.9f) - pct (w, 0.15f); }
    P.corr = (float) (C.sLR / std::sqrt (juce::jmax (1.0e-12, C.sLL * C.sRR)));
    P.loSide = db (C.sLoS) - db (C.sLoM); P.sideDb = db (C.sSide) - db (C.sMid);
    float th[kThirds]; C.ana.result (th); ThirdAnalyzer::regions (th, P.reg);
    float want[kRegions];
    if (P.useRef) { float rr[kRegions]; ThirdAnalyzer::regions (ref->thirds, rr); for (int r = 0; r < kRegions; ++r) want[r] = rr[r]; }
    else for (int r = 0; r < kRegions; ++r) want[r] = genre (genreIdx).off[r];
    for (int r = 0; r < kRegions; ++r) P.dev[r] = P.reg[r] - want[r];
    // risonanze: il terzo che sporge di piu' sulle vicine (fra 100 Hz e 10 kHz)
    for (int k = 2; k < kThirds - 2; ++k) { const float f = thirdHz (k); if (f < 100 || f > 10000) continue; const float ex = th[k] - 0.5f * (th[k - 2] + th[k + 2]); if (ex > P.resMax) { P.resMax = ex; P.resHz = f; } }
    if (P.useRef) P.targetLufs = juce::jlimit (-20.0f, -5.0f, ref->lufs);
    const juce::String gname = genre (genreIdx).name, cmp = P.useRef ? juce::String ("il riferimento") : juce::String ("un ") + gname;

    // ---- cosa ho sentito
    P.heard.add ("Volume medio circa " + a1 (P.level) + " LUFS, picco " + a1 (P.peak) + " dB: " + juce::String (P.plr > 14 ? "molto spazio fra picchi e corpo, si puo' alzare parecchio." : P.plr > 9 ? "dinamica sana da mix." : "gia' piuttosto compresso, attento a non schiacciarlo ancora."));
    P.heard.add ("Fra le parti piano e forti " + a1 (P.dr) + " dB" + juce::String (P.dr > 8 ? ": il pezzo respira molto, un po' di colla aiuta." : P.dr > 4 ? ": giusto." : ": e' gia' molto costante."));
    { juce::String t = "Rispetto a " + cmp + ":"; int shown = 0;
      for (int r = 0; r < kRegions; ++r) if (std::abs (P.dev[r]) >= 1.2f) { t << " " << juce::String (regionName (r)).toLowerCase() << " " << n1 (P.dev[r]) << " dB,"; ++shown; }
      P.heard.add (shown ? t.dropLastCharacters (1) + "." : t + " l'equilibrio tonale e' gia' vicino."); }
    P.heard.add ("Stereo: correlazione " + juce::String (P.corr, 2) + ", bassi sotto 150 Hz " + juce::String (P.loSide > -12 ? "larghi (" + a1 (P.loSide) + " dB di lati): in mono perdono forza" : "centrati") + ".");
    if (P.resMax > 3.5f) P.heard.add ("Una zona sporge: " + juce::String (juce::roundToInt (P.resHz)) + " Hz, " + a1 (P.resMax) + " dB sopra le vicine.");

    auto step = [&] (int mod, juce::String title, juce::String why, std::vector<std::pair<juce::String, float>> set) { EarStep s; s.mod = mod; s.title = std::move (title); s.why = std::move (why); s.set = std::move (set); P.steps.push_back (std::move (s)); };
    const bool lowGenre = genreIdx == 0 || genreIdx == 1 || genreIdx == 4;   // trap, hip hop, edm: tanti bassi
    // 1. bassi mono
    if (P.loSide > -14 || lowGenre)
    {
        const float f = P.loSide > -8 ? 150.0f : 110.0f;
        step (0 /*BASS*/, "BASS CONTROL  mono sotto " + juce::String ((int) f) + " Hz",
              P.loSide > -14 ? "I bassi hanno " + a1 (P.loSide) + " dB di lati: in mono (telefoni, club) si annullano in parte. Sotto " + juce::String ((int) f) + " Hz li metto al centro."
                             : "Per " + gname + " sub e cassa devono stare fermi al centro: li blocco sotto " + juce::String ((int) f) + " Hz anche se ora sono gia' quasi mono.",
              { { "bc_on", 1 }, { "bc_mono", f }, { "bc_freq", f }, { "bc_thr", P.dev[0] > 2 ? -10.0f : 0.0f } });
    }
    // 2. risonanza
    if (P.resMax > 3.5f)
        step (14 /*SPECTRAL*/, "SPETTRALE  risonanze " + juce::String (juce::roundToInt (juce::jlimit (30.0f, 80.0f, P.resMax * 12))) + "%",
              juce::String (juce::roundToInt (P.resHz)) + " Hz sporge di " + a1 (P.resMax) + " dB: la abbasso solo quando suona forte, il resto non lo tocco.",
              { { "sp_on", 1 }, { "sp_res", juce::jlimit (0.3f, 0.8f, P.resMax * 0.12f) }, { "sp_sens", 0.5f }, { "sp_clar", P.dev[3] < -1.5f ? 0.3f : 0.0f }, { "sp_lo", juce::jmax (80.0f, P.resHz * 0.5f) }, { "sp_hi", juce::jmin (16000.0f, P.resHz * 2.0f) } });
    // 3. tono: EQ dinamico a bande larghe, mezza correzione (il resto lo decide l'orecchio). Se poi entra lo STABILIZER, l'EQ fa meno
    int farZones = 0; for (auto d : P.dev) if (std::abs (d) > 2.5f) ++farZones;
    const bool withStab = farZones >= 2 && ! P.useRef;
    {
        std::vector<std::pair<juce::String, float>> s; juce::String what, why; int nb = 0;
        static const float fr[kRegions] = { 45, 110, 320, 1200, 3500, 10000 };
        static const int ty[kRegions] = { 1, 0, 0, 0, 0, 2 };   // 0 campana, 1 shelf basso, 2 shelf alto (come jmx::BT_*: vedi sotto)
        for (int r = 0; r < kRegions; ++r)
        {
            if (std::abs (P.dev[r]) < 1.5f) continue;
            const float g = juce::jlimit (-3.0f, 3.0f, -P.dev[r] * (withStab ? 0.35f : 0.55f)); const bool harsh = r == 4 && P.dev[r] > 1.5f, mud = r == 2 && P.dev[r] > 1.5f;
            const int b = nb++; if (b >= 6) break;
            auto id = [b] (const char* k) { return juce::String ("eq") + juce::String (b + 1) + "_" + k; };
            s.push_back ({ id ("on"), 1 }); s.push_back ({ id ("type"), (float) (ty[r] == 1 ? 1 : ty[r] == 2 ? 2 : 0) }); s.push_back ({ id ("f"), fr[r] });
            s.push_back ({ id ("g"), harsh || mud ? 0.0f : g }); s.push_back ({ id ("q"), ty[r] ? 0.6f : 0.9f });
            s.push_back ({ id ("dyn"), harsh || mud ? 1.0f : 0.0f }); s.push_back ({ id ("thr"), -30 }); s.push_back ({ id ("rng"), harsh || mud ? g * 1.6f : -6.0f }); s.push_back ({ id ("ch"), 0 });
            what << juce::String (regionName (r)).toLowerCase() << " " << n1 (g) << (harsh || mud ? " (dinamico)" : "") << ", ";
            why << juce::String (regionName (r)).toLowerCase() << " e' " << n1 (P.dev[r]) << " dB rispetto a " << cmp << (harsh ? ": la presenza la tolgo solo quando punge" : mud ? ": il fango lo tolgo solo quando si accumula" : "") << "; ";
        }
        if (nb > 0) { s.push_back ({ "eq_on", 1 }); step (1 /*DYNEQ*/, "EQ DINAMICO  " + what.dropLastCharacters (2), juce::String (withStab ? "Correggo un terzo della distanza (il resto lo segue lo STABILIZER): " : "Correggo meta' della distanza, non tutta: ") + why.dropLastCharacters (2) + ".", s); }
    }
    // 4. stabilizer quando lo spettro e' lontano in piu' zone
    { const int far = farZones;
      if (withStab) step (15 /*STABIL*/, "STABILIZER  " + gname + " al 40%", juce::String (far) + " zone sono lontane piu' di 2.5 dB dalla curva " + gname + ": lo stabilizer segue la canzone e corregge quello che l'EQ fisso non vede.",
                                         { { "st_on", 1 }, { "st_amt", 0.4f }, { "st_speed", 0.35f }, { "st_genre", (float) genreIdx } }); }
    // 5. colla
    if (P.dr > 6.5f)
    {
        const bool tube = genreIdx == 1 || genreIdx == 2 || genreIdx == 6;
        if (tube) step (12 /*VARIMU*/, "VARI-MU  1-2 dB", "Dinamica di " + a1 (P.dr) + " dB: per " + gname + " una colla a valvole lenta scalda e tiene insieme senza togliere groove.",
                        { { "vm_on", 1 }, { "vm_thr", P.level + 2.0f }, { "vm_att", 3 }, { "vm_rel", 6 }, { "vm_mix", 0.8f }, { "vm_tube", 0.35f } });
        else step (3 /*BUSCOMP*/, "BUS COMP  2:1  1-2 dB", "Dinamica di " + a1 (P.dr) + " dB: 1-2 dB di bus comp con attacco lento incollano senza perdere il colpo della cassa.",
                   { { "bus_on", 1 }, { "bus_thr", P.level + 3.0f }, { "bus_ratio", 1 }, { "bus_att", 4 }, { "bus_rel", 4 }, { "bus_hpf", 90 }, { "bus_range", 4 }, { "bus_mix", 1 }, { "bus_auto", 1 } });
    }
    // 6. punch
    if (P.plr < 10 && lowGenre) step (13 /*IMPACT*/, "IMPACT  bassi +25%", "Picchi solo " + a1 (P.plr) + " dB sopra il corpo: il mix e' gia' schiacciato. Ridò attacco alla cassa prima del maximizer.",
                                      { { "ip_on", 1 }, { "ip_b1", 0.25f }, { "ip_b2", 0.1f }, { "ip_b3", 0.0f }, { "ip_b4", 0.0f }, { "ip_speed", 0.5f } });
    // 7. colore e aria
    if (genreIdx == 1 || genreIdx == 6) step (6 /*TAPE*/, "TAPE  15 ips  drive 25%", "Per " + gname + " il nastro arrotonda gli alti e aggiunge corpo sui bassi: il suono che ci si aspetta.",
                                               { { "tp_on", 1 }, { "tp_model", 1 }, { "tp_drive", 0.25f }, { "tp_mix", 0.8f } });
    if (P.dev[5] < -2.0f && genreIdx != 6) step (5 /*EXCITER*/, "EXCITER  aria 25%", "Aria " + n1 (P.dev[5]) + " dB sotto " + cmp + ": invece di alzare l'EQ aggiungo armoniche sopra i 6 kHz, piu' luce senza piu' durezza.",
                                                 { { "ex_on", 1 }, { "ex1_amt", 0 }, { "ex2_amt", 0 }, { "ex3_amt", 0.25f }, { "ex_mix", 1 } });
    // 8. stereo
    if (P.corr > 0.92f && genreIdx != 6) step (7 /*IMAGER*/, "IMAGER  alti 115%", "Correlazione " + juce::String (P.corr, 2) + ": il mix e' quasi mono. Apro un po' solo gli alti, i bassi restano al centro.",
                                               { { "im_on", 1 }, { "im_w1", 1 }, { "im_w2", 1 }, { "im_w3", 1.1f }, { "im_w4", 1.15f } });
    else if (P.corr < 0.3f) step (7, "IMAGER  stringo i lati", "Correlazione " + juce::String (P.corr, 2) + ": troppo largo, in mono si perdono pezzi. Stringo medi e bassi.",
                                  { { "im_on", 1 }, { "im_w1", 0.6f }, { "im_w2", 0.85f }, { "im_w3", 1 }, { "im_w4", 1 } });
    // 9. volume e tetto
    {
        const int style = genreIdx == 0 ? 2 : genreIdx == 4 || genreIdx == 1 ? 1 : genreIdx == 2 || genreIdx == 6 ? 3 : 0;   // PULITO 0, PUNCH 1, AGGRESSIVO 2, MORBIDO 3
        static const char* sn[] = { "PULITO", "PUNCH", "AGGRESSIVO", "MORBIDO" };
        const float need = juce::jlimit (0.0f, 14.0f, P.targetLufs - P.level);
        step (8 /*MAXIM*/, "MAXIMIZER  " + juce::String (sn[style]) + "  verso " + a1 (P.targetLufs) + " LUFS",
              "Da " + a1 (P.level) + " a " + a1 (P.targetLufs) + " LUFS servono circa " + a1 (need) + " dB: li taro mentre suona. Stile " + sn[style] + " per " + (P.useRef ? juce::String ("avvicinarmi al riferimento") : gname) + ".",
              { { "mx_on", 1 }, { "mx_gain", need * 0.7f }, { "mx_style", (float) style }, { "mx_clip", style == 2 ? 0.45f : style == 1 ? 0.25f : 0.1f }, { "mx_char", 0.5f }, { "mx_link", 1 } });
        const int lt = lowGenre ? 2 : genreIdx == 2 || genreIdx == 6 ? 1 : 0; static const char* ln[] = { "JACKPOT", "OXFORD", "IRC" };
        step (9 /*LIMIT*/, "LIMITER  " + juce::String (ln[lt]) + "  " + a1 (P.ceil) + " dBTP", lt == 2 ? "Con tanti bassi il limiter a bande evita che la cassa abbassi tutto il resto." : lt == 1 ? "Ginocchio morbido e un filo di densita': piu' musicale per " + gname + "." : "Trasparente: tiene solo il tetto.",
              { { "lm_on", 1 }, { "lm_type", (float) lt }, { "lm_os", 3 }, { "lm_tp", 1 }, { "lm_auto", 1 }, { "lm_gain", 0 }, { "lm_enh", lt == 1 ? 0.3f : 0.0f } });
    }
    if (P.peak > -0.5f) P.notes.add ("Il mix arriva a " + a1 (P.peak) + " dB: abbassa l'INGRESSO di 3-6 dB, i moduli lavorano meglio con margine.");
    if (P.plr < 8) P.notes.add ("Il mix e' gia' molto compresso (" + a1 (P.plr) + " dB fra picchi e corpo): se puoi, togli un po' di limiter sul bus del mix prima del master.");
    P.valid = true;
    return P;
}
}
