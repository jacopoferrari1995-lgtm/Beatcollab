#pragma once
// PROVA D'ASCOLTO del Master: ascolta il pezzo (30 s, 60 s o tutto, finche' premi STOP), lo misura come un ingegnere di mastering
// (LUFS integrati con il gating della norma, range di loudness, picchi, micro-dinamica, equilibrio tonale sulle parti piene,
// stereo per zone, sibilanti, risonanze) e propone SOLO i moduli che servono, ognuno con la sua priorita' e il perche'.
#include "MasterAnalysis.h"
#include "MasterModules.h"

namespace jms
{
struct EarCapture
{
    static constexpr int kMax = 30000;                 // 5 minuti in fotogrammi da 10 ms
    std::vector<float> pk = std::vector<float> (kMax), kms = std::vector<float> (kMax), hiPk = std::vector<float> (kMax), hiMs = std::vector<float> (kMax);
    std::atomic<int> n { 0 }, need { 3000 }; std::atomic<bool> on { false };
    double sMid = 0, sSide = 0, sLoM = 0, sLoS = 0, sHiM = 0, sHiS = 0, sLR = 0, sLL = 0, sRR = 0;
    ThirdAnalyzer anaAll, anaLoud; std::vector<float> ring = std::vector<float> (4096, 0.0f); int fill = 0; double ringE = 0, longE = 0;
    double sr = 44100; int fLen = 441, fPos = 0; float fPk = 0, fHiPk = 0; double fMs = 0, fHi = 0; float lpM = 0, lpS = 0, hpM = 0, hpS = 0;
    Biquad kA[2], kB[2], hb;   // pesatura K (BS.1770) per canale, banda delle sibilanti

    void prepare (double s)
    {
        sr = s; fLen = juce::jmax (64, (int) (s * 0.01));
        for (int c = 0; c < 2; ++c) { kA[c].set (Biquad::HSHELF, 1681.97, 4.0, 0.7071, sr); kB[c].set (Biquad::LCUT, 38.13, 0, 0.5003, sr); kA[c].reset(); kB[c].reset(); }
        hb.set (Biquad::BPASS, 7000, 0, 1.0, sr); hb.reset();
    }
    void start (int seconds) { n = 0; need = seconds > 0 ? juce::jmin (kMax, seconds * 100) : kMax; sMid = sSide = sLoM = sLoS = sHiM = sHiS = sLR = sLL = sRR = 0; anaAll.reset(); anaLoud.reset(); fill = 0; ringE = longE = 0; fPos = 0; fPk = fHiPk = 0; fMs = fHi = 0; on = true; }
    void stop() { on = false; }
    void push (const float* L, const float* R, int N)
    {
        if (! on.load()) return;
        const float a = 1.0f - std::exp (-2.0f * juce::MathConstants<float>::pi * 150.0f / (float) sr), ah = 1.0f - std::exp (-2.0f * juce::MathConstants<float>::pi * 4000.0f / (float) sr);
        for (int i = 0; i < N; ++i)
        {
            const float l = L[i], r = R[i], m = 0.5f * (l + r), s = 0.5f * (l - r);
            lpM += a * (m - lpM); lpS += a * (s - lpS); hpM += ah * (m - hpM); hpS += ah * (s - hpS);
            const float kl = kB[0] (kA[0] (l)), kr = kB[1] (kA[1] (r)), h = hb (m);
            fPk = juce::jmax (fPk, std::abs (l), std::abs (r)); fMs += kl * kl + kr * kr; fHi += h * h; fHiPk = juce::jmax (fHiPk, std::abs (h));
            if (m * m > 1.0e-10f)
            {
                sMid += m * m; sSide += s * s; sLoM += lpM * lpM; sLoS += lpS * lpS; sHiM += (m - hpM) * (m - hpM); sHiS += (s - hpS) * (s - hpS); sLR += l * r; sLL += l * l; sRR += r * r;
                ring[(size_t) fill++] = m; ringE += m * m;
                if (fill == 4096)
                {   // spettro in terzi: tutto il pezzo, e a parte solo le parti piene (piu' forti della media a lungo termine)
                    anaAll.addFrame (ring.data(), sr);
                    const double e = ringE / 4096; longE = longE <= 0 ? e : longE * 0.97 + e * 0.03;
                    if (e > longE * 1.1) anaLoud.addFrame (ring.data(), sr);
                    std::copy (ring.begin() + 2048, ring.end(), ring.begin()); fill = 2048; ringE = 0; for (int k = 0; k < 2048; ++k) ringE += ring[(size_t) k] * ring[(size_t) k];
                }
            }
            if (++fPos >= fLen)
            {
                const float msF = (float) (fMs / fLen); const int k = n.load();
                if (msF > 1.0e-8f && k < kMax)
                {
                    pk[(size_t) k] = fPk; kms[(size_t) k] = msF; hiPk[(size_t) k] = fHiPk; hiMs[(size_t) k] = (float) (fHi / fLen); n = k + 1;
                    if (k + 1 >= need.load()) on = false;
                }
                fPos = 0; fPk = fHiPk = 0; fMs = fHi = 0;
            }
        }
    }
};

enum EarPrio { PRIO_NEED = 0, PRIO_ADVISE, PRIO_TASTE };
inline const char* prioName (int p) { static const char* n[] = { "NECESSARIO", "CONSIGLIATO", "GUSTO" }; return n[juce::jlimit (0, 2, p)]; }
struct EarStep { int mod = 0, prio = PRIO_ADVISE; bool sel = true; juce::String title, why; std::vector<std::pair<juce::String, float>> set; };
struct EarPlan
{
    bool valid = false, useRef = false; int genre = 0; float secs = 0;
    float lufs = -60, stMax = -60, lra = 0, peak = -60, plr = 0, crest = 0, corr = 1, loSide = -60, hiSide = -60, sib = 0, reg[kRegions] {}, dev[kRegions] {}, resMax = 0, resHz = 0, targetLufs = -14, ceil = -1;
    juce::StringArray heard; std::vector<EarStep> steps; juce::StringArray notes;
};

namespace ear
{
inline float db (double x) { return 10.0f * (float) std::log10 (juce::jmax (1.0e-12, x)); }
inline float pct (std::vector<float> v, float p) { if (v.empty()) return -100; std::sort (v.begin(), v.end()); return v[(size_t) juce::jlimit (0, (int) v.size() - 1, (int) (p * (float) (v.size() - 1)))]; }
inline juce::String n1 (float v) { return (v > 0.05f ? "+" : "") + juce::String (v, 1); }
inline juce::String a1 (float v) { return juce::String (v, 1); }
}

inline EarPlan earAnalyse (const EarCapture& C, int genreIdx, float targetLufs, float ceilDb, const RefTrack* ref)
{
    using namespace ear;
    EarPlan P; const int N = C.n.load(); if (N < 1500 || ! C.anaAll.hasData()) return P;   // almeno 15 secondi
    P.genre = genreIdx; P.targetLufs = targetLufs; P.ceil = ceilDb; P.useRef = ref != nullptr && ref->lufs > -60; P.secs = N * 0.01f;
    // ---- loudness come da norma: blocchi da 400 ms (passo 100 ms), soglia assoluta -70, relativa -10
    std::vector<float> blk; for (int i = 0; i + 40 <= N; i += 10) { double s = 0; for (int j = 0; j < 40; ++j) s += C.kms[(size_t) (i + j)]; blk.push_back (-0.691f + db (s / 40)); }
    { double s = 0; int c = 0; for (auto b : blk) if (b > -70) { s += std::pow (10.0, b / 10.0); ++c; }
      const float rel = c ? db (s / c) - 10.0f : -70; s = 0; c = 0; for (auto b : blk) if (b > rel && b > -70) { s += std::pow (10.0, b / 10.0); ++c; } P.lufs = c ? db (s / c) : -70; }
    // short-term (3 s) e range di loudness
    std::vector<float> st; for (int i = 0; i + 300 <= N; i += 100) { double s = 0; for (int j = 0; j < 300; ++j) s += C.kms[(size_t) (i + j)]; st.push_back (-0.691f + db (s / 300)); }
    { std::vector<float> g; for (auto v : st) if (v > P.lufs - 20) g.push_back (v); P.lra = g.size() > 3 ? pct (g, 0.95f) - pct (g, 0.10f) : 0; P.stMax = g.empty() ? P.lufs : pct (g, 1.0f); }
    float pkMax = 0; for (int i = 0; i < N; ++i) pkMax = juce::jmax (pkMax, C.pk[(size_t) i]);
    P.peak = 20.0f * std::log10 (juce::jmax (1.0e-6f, pkMax)); P.plr = P.peak - P.lufs;
    // micro-dinamica: picco su rms nei blocchi da 400 ms (mediana)
    { std::vector<float> cr; for (int i = 0; i + 40 <= N; i += 20) { double s = 0; float p = 0; for (int j = 0; j < 40; ++j) { s += C.kms[(size_t) (i + j)] * 0.5; p = juce::jmax (p, C.pk[(size_t) (i + j)]); } cr.push_back (20.0f * std::log10 (juce::jmax (1.0e-6f, p)) - db (s / 40)); } P.crest = pct (cr, 0.5f); }
    // sibilanti: quanto i picchi della zona 5-10 kHz sporgono dalla loro media
    { std::vector<float> h; for (int i = 0; i < N; ++i) if (C.hiMs[(size_t) i] > 1.0e-10f) h.push_back (20.0f * std::log10 (juce::jmax (1.0e-7f, C.hiPk[(size_t) i])) - db (C.hiMs[(size_t) i])); P.sib = h.empty() ? 0 : pct (h, 0.99f) - pct (h, 0.5f); }
    P.corr = (float) (C.sLR / std::sqrt (juce::jmax (1.0e-12, C.sLL * C.sRR)));
    P.loSide = db (C.sLoS) - db (C.sLoM); P.hiSide = db (C.sHiS) - db (C.sHiM);
    float th[kThirds]; (C.anaLoud.hasData() ? C.anaLoud : C.anaAll).result (th); ThirdAnalyzer::regions (th, P.reg);
    float want[kRegions];
    if (P.useRef) { ThirdAnalyzer::regions (ref->thirds, want); P.targetLufs = juce::jlimit (-20.0f, -5.0f, ref->lufs); }
    else for (int r = 0; r < kRegions; ++r) want[r] = genre (genreIdx).off[r];
    for (int r = 0; r < kRegions; ++r) P.dev[r] = P.reg[r] - want[r];
    for (int k = 2; k < kThirds - 2; ++k) { const float f = thirdHz (k); if (f < 80 || f > 10000) continue; const float ex = th[k] - 0.25f * (th[k - 1] + th[k + 1]) - 0.25f * (th[k - 2] + th[k + 2]); if (ex > P.resMax) { P.resMax = ex; P.resHz = f; } }
    const juce::String gname = genre (genreIdx).name, cmp = P.useRef ? juce::String ("al riferimento") : juce::String ("alla curva ") + gname;
    const bool lowGenre = genreIdx == 0 || genreIdx == 1 || genreIdx == 4, soft = genreIdx == 2 || genreIdx == 6 || genreIdx == 7;
    const float tolTone = P.useRef ? 1.5f : 2.0f;

    // ---- cosa sento
    P.heard.add ("Ho ascoltato " + juce::String (juce::roundToInt (P.secs)) + " s. Loudness integrata " + a1 (P.lufs) + " LUFS (parte piu' forte " + a1 (P.stMax) + "), picco " + a1 (P.peak) + " dBFS.");
    P.heard.add ("Range di loudness " + a1 (P.lra) + " LU" + juce::String (P.lra > 8 ? ": strofe e ritornelli molto diversi, il master li puo' avvicinare un po'." : P.lra > 4 ? ": normale per un pezzo con strofe e ritornelli." : ": il pezzo sta sempre allo stesso livello."));
    P.heard.add ("Micro-dinamica (picchi sul corpo) " + a1 (P.crest) + " dB, PLR " + a1 (P.plr) + " dB" + juce::String (P.plr > 15 ? ": tanto spazio, si puo' alzare con calma." : P.plr > 10 ? ": dinamica sana." : ": e' gia' compresso, ogni dB in piu' costa punch."));
    { juce::String t = "Rispetto " + cmp + " (sulle parti piene):"; int shown = 0;
      for (int r = 0; r < kRegions; ++r) if (std::abs (P.dev[r]) >= tolTone) { t << " " << juce::String (regionName (r)).toLowerCase() << " " << n1 (P.dev[r]) << ","; ++shown; }
      P.heard.add (shown ? t.dropLastCharacters (1) + " dB." : t + " equilibrio gia' vicino, niente da correggere."); }
    P.heard.add ("Stereo: correlazione " + juce::String (P.corr, 2) + "; bassi " + (P.loSide > -12 ? "larghi (" + a1 (P.loSide) + " dB di lati)" : juce::String ("centrati")) + "; alti " + (P.hiSide > -6 ? "molto larghi" : P.hiSide > -14 ? "aperti" : "stretti") + ".");
    if (P.sib > 16) P.heard.add ("Sibilanti/piatti: i picchi fra 5 e 10 kHz sporgono di " + a1 (P.sib) + " dB sulla loro media, pungono.");
    if (P.resMax > 5) P.heard.add ("Una zona sporge: intorno a " + juce::String (juce::roundToInt (P.resHz)) + " Hz, " + a1 (P.resMax) + " dB sopra le vicine.");

    auto step = [&] (int mod, int prio, juce::String title, juce::String why, std::vector<std::pair<juce::String, float>> set)
    { EarStep s; s.mod = mod; s.prio = prio; s.sel = prio != PRIO_TASTE; s.title = std::move (title); s.why = std::move (why); s.set = std::move (set); P.steps.push_back (std::move (s)); };

    // 1. bassi
    if (P.loSide > -12 || (lowGenre && P.loSide > -18))
    {
        const float f = P.loSide > -8 ? 150.0f : 120.0f;
        step (0, P.loSide > -12 ? PRIO_NEED : PRIO_ADVISE, "BASS CONTROL  mono sotto " + juce::String ((int) f) + " Hz",
              "I bassi hanno " + a1 (P.loSide) + " dB di lati: su telefoni e impianti mono una parte si cancella. Al centro sotto " + juce::String ((int) f) + " Hz restano pieni ovunque.",
              { { "bc_on", 1 }, { "bc_mono", f }, { "bc_freq", f }, { "bc_thr", 0 } });
    }
    // 2. risonanza
    if (P.resMax > 5)
        step (14, P.resMax > 8 ? PRIO_NEED : PRIO_ADVISE, "SPETTRALE  su " + juce::String (juce::roundToInt (P.resHz)) + " Hz",
              juce::String (juce::roundToInt (P.resHz)) + " Hz sta " + a1 (P.resMax) + " dB sopra le zone vicine: la abbasso solo nei momenti in cui sporge, non tutto il tempo.",
              { { "sp_on", 1 }, { "sp_res", juce::jlimit (0.3f, 0.8f, P.resMax * 0.07f) }, { "sp_sens", 0.55f }, { "sp_clar", 0 }, { "sp_lo", juce::jmax (60.0f, P.resHz * 0.6f) }, { "sp_hi", juce::jmin (16000.0f, P.resHz * 1.7f) } });
    // 3. tono
    int farZones = 0; for (auto d : P.dev) if (std::abs (d) > 3.0f) ++farZones;
    {
        std::vector<std::pair<juce::String, float>> s; juce::String what, why; int nb = 0; float worst = 0;
        static const float fr[kRegions] = { 45, 110, 320, 1200, 3500, 10000 };
        for (int r = 0; r < kRegions; ++r)
        {
            if (std::abs (P.dev[r]) < tolTone) continue;
            worst = juce::jmax (worst, std::abs (P.dev[r]));
            const float g = juce::jlimit (-3.0f, 3.0f, -P.dev[r] * 0.5f); const bool dyn = (r == 4 && P.dev[r] > 0) || (r == 2 && P.dev[r] > 0);
            const int b = nb++; auto id = [b] (const char* k) { return juce::String ("eq") + juce::String (b + 1) + "_" + k; };
            s.push_back ({ id ("on"), 1 }); s.push_back ({ id ("type"), (float) (r == 0 ? 1 : r == kRegions - 1 ? 2 : 0) }); s.push_back ({ id ("f"), fr[r] });
            s.push_back ({ id ("g"), dyn ? 0.0f : g }); s.push_back ({ id ("q"), r == 0 || r == kRegions - 1 ? 0.6f : 0.9f });
            s.push_back ({ id ("dyn"), dyn ? 1.0f : 0.0f }); s.push_back ({ id ("thr"), -30 }); s.push_back ({ id ("rng"), dyn ? g * 1.6f : -6.0f }); s.push_back ({ id ("ch"), 0 });
            what << juce::String (regionName (r)).toLowerCase() << " " << n1 (g) << ", ";
            why << juce::String (regionName (r)).toLowerCase() << " " << n1 (P.dev[r]) << (dyn ? (r == 4 ? " (dinamico: solo quando punge)" : " (dinamico: solo quando si accumula)") : "") << ", ";
        }
        if (nb > 0) { s.push_back ({ "eq_on", 1 }); step (1, worst > 4 ? PRIO_NEED : PRIO_ADVISE, "EQ DINAMICO  " + what.dropLastCharacters (2), "Rispetto " + cmp + ": " + why.dropLastCharacters (2) + " dB. Correggo meta' della distanza: il resto va giudicato a orecchio.", s); }
    }
    if (farZones >= 2 && ! P.useRef) step (15, PRIO_TASTE, "STABILIZER  " + gname + " al 30%", juce::String (farZones) + " zone sono lontane oltre 3 dB: lo stabilizer corregge piano mentre il pezzo cambia (strofe piu' vuote, ritornelli piu' pieni).",
                                         { { "st_on", 1 }, { "st_amt", 0.3f }, { "st_speed", 0.3f }, { "st_genre", (float) genreIdx } });
    // 4. sibilanti
    if (P.sib > 16) step (2, P.sib > 20 ? PRIO_NEED : PRIO_ADVISE, "DE-ESSER  5.5-12 kHz", "I picchi fra 5 e 10 kHz sporgono di " + a1 (P.sib) + " dB: dopo il maximizer pungerebbero ancora di piu'. Li prendo solo quando arrivano.",
                          { { "ds_on", 1 }, { "ds_lo", 5500 }, { "ds_hi", 12000 }, { "ds_thr", -12 }, { "ds_amt", juce::jlimit (0.25f, 0.6f, (P.sib - 14) * 0.05f) }, { "ds_soft", 0.7f }, { "ds_ch", 0 } });
    // 5. colla: solo se il pezzo salta davvero
    if (P.lra > 7 && genreIdx != 7)
    {
        if (soft || genreIdx == 1) step (12, PRIO_ADVISE, "VARI-MU  1-2 dB", "Range di " + a1 (P.lra) + " LU: una colla a valvole lenta avvicina strofe e ritornelli senza toccare il groove.",
                                         { { "vm_on", 1 }, { "vm_thr", P.stMax - 4.0f }, { "vm_att", 3 }, { "vm_rel", 6 }, { "vm_mix", 0.8f }, { "vm_tube", 0.35f } });
        else step (3, PRIO_ADVISE, "BUS COMP  2:1  1-2 dB", "Range di " + a1 (P.lra) + " LU: 1-2 dB di bus comp sulle parti forti, attacco lento per lasciare la botta della cassa.",
                   { { "bus_on", 1 }, { "bus_thr", P.stMax - 4.0f }, { "bus_ratio", 1 }, { "bus_att", 4 }, { "bus_rel", 4 }, { "bus_hpf", 90 }, { "bus_range", 4 }, { "bus_mix", 1 }, { "bus_auto", 1 } });
    }
    // 6. punch
    if (P.plr < 9 && lowGenre) step (13, PRIO_TASTE, "IMPACT  bassi +20%", "PLR di " + a1 (P.plr) + " dB: il mix arriva gia' schiacciato. Un po' di attacco sui bassi prima del maximizer rida' la cassa.",
                                     { { "ip_on", 1 }, { "ip_b1", 0.2f }, { "ip_b2", 0.05f }, { "ip_b3", 0 }, { "ip_b4", 0 }, { "ip_speed", 0.5f } });
    // 7. colore e aria
    if (genreIdx == 1 || genreIdx == 6) step (6, PRIO_TASTE, "TAPE  15 ips  drive 20%", "Per " + gname + " il nastro arrotonda gli alti e da' corpo: e' il colore del genere, non una correzione.",
                                               { { "tp_on", 1 }, { "tp_model", 1 }, { "tp_drive", 0.2f }, { "tp_mix", 0.8f } });
    if (P.dev[5] < -2.5f && genreIdx != 6) step (5, PRIO_ADVISE, "EXCITER  aria 20%", "Aria " + n1 (P.dev[5]) + " dB rispetto " + cmp + ": armoniche sopra i 6 kHz danno luce senza alzare la durezza come farebbe l'EQ.",
                                                 { { "ex_on", 1 }, { "ex1_amt", 0 }, { "ex2_amt", 0 }, { "ex3_amt", 0.2f }, { "ex_mix", 1 } });
    // 8. stereo
    if (P.corr < 0.25f) step (7, PRIO_NEED, "IMAGER  stringo bassi e medi", "Correlazione " + juce::String (P.corr, 2) + ": in mono si perdono pezzi. Stringo sotto i 2 kHz, gli alti restano aperti.",
                              { { "im_on", 1 }, { "im_w1", 0.5f }, { "im_w2", 0.8f }, { "im_w3", 1 }, { "im_w4", 1 } });
    else if (P.corr > 0.95f && P.hiSide < -14 && genreIdx != 6) step (7, PRIO_TASTE, "IMAGER  alti 115%", "Correlazione " + juce::String (P.corr, 2) + " e alti stretti: aprirli un poco da' aria, i bassi restano al centro.",
                                                                       { { "im_on", 1 }, { "im_w1", 1 }, { "im_w2", 1 }, { "im_w3", 1.1f }, { "im_w4", 1.15f } });
    // 9. volume e tetto (sempre: e' il master)
    {
        const int style = genreIdx == 0 ? 2 : genreIdx == 4 || genreIdx == 1 ? 1 : soft ? 3 : 0; static const char* sn[] = { "PULITO", "PUNCH", "AGGRESSIVO", "MORBIDO" };
        const float need = juce::jlimit (0.0f, 14.0f, P.targetLufs - P.lufs);
        if (need > 0.5f)
            step (8, PRIO_NEED, "MAXIMIZER  " + juce::String (sn[style]) + "  verso " + a1 (P.targetLufs) + " LUFS",
                  "Da " + a1 (P.lufs) + " a " + a1 (P.targetLufs) + " LUFS mancano " + a1 (need) + " dB: li taro mentre suona. " + (need > 9 && P.plr < 12 ? juce::String ("Sono tanti per un mix gia' compresso: aspettati di perdere punch, valuta un obiettivo piu' basso.") : juce::String ("Stile ") + sn[style] + " per " + gname + "."),
                  { { "mx_on", 1 }, { "mx_gain", need * 0.7f }, { "mx_style", (float) style }, { "mx_clip", style == 2 ? 0.4f : style == 1 ? 0.25f : 0.1f }, { "mx_char", 0.5f }, { "mx_link", 1 } });
        const int lt = lowGenre ? 2 : soft ? 1 : 0; static const char* ln[] = { "JACKPOT", "OXFORD", "IRC" };
        step (9, PRIO_NEED, "LIMITER  " + juce::String (ln[lt]) + "  " + a1 (P.ceil) + " dBTP  8x",
              lt == 2 ? "Tanti bassi: il limiter a bande evita che la cassa abbassi voce e piatti." : lt == 1 ? "Ginocchio morbido e un filo di densita', piu' musicale per " + gname + "." : "Trasparente: tiene solo il tetto, true peak a 8x.",
              { { "lm_on", 1 }, { "lm_type", (float) lt }, { "lm_os", 3 }, { "lm_tp", 1 }, { "lm_auto", 1 }, { "lm_gain", 0 }, { "lm_enh", lt == 1 ? 0.25f : 0.0f } });
    }
    if (P.peak > -0.5f) P.notes.add ("Il mix tocca " + a1 (P.peak) + " dBFS: abbassa l'INGRESSO di 3-6 dB, i moduli lavorano meglio con margine.");
    if (P.plr < 8) P.notes.add ("PLR " + a1 (P.plr) + " dB: c'e' gia' un limiter sul mix? Toglilo o alleggeriscilo prima del master.");
    if (P.corr < 0) P.notes.add ("Correlazione negativa: qualcosa e' in controfase. Controlla nel mix (spesso un doppione stereo o un synth allargato).");
    P.valid = true;
    return P;
}
}
