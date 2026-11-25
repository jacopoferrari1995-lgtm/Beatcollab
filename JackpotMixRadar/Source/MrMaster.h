#pragma once
// ANALISI MASTER: funziona anche senza sensori, solo con Mix Radar sul Master.
// Spettro medio (ultimi secondi o tutto il brano) diviso in 7 fasce e confrontato con un riferimento per genere,
// risonanze strette, stereo per fascia, dinamica (PLR, LRA), loudness rispetto al genere, battute dove una fascia salta.
// Da qui escono consigli precisi: dove (Hz), quanto (dB), come (Q, tipo di filtro) e con cosa.
#include <juce_dsp/juce_dsp.h>
#include <map>
#include <array>

namespace mr
{
constexpr int NF = 7;
inline const char* fasciaName (int i) { static const char* n[NF] = { "SUB", "BASSI", "FANGO", "MEDI", "PRESENZA", "ALTI", "ARIA" }; return n[juce::jlimit (0, NF - 1, i)]; }
inline float fasciaEdge (int i) { static const float e[NF + 1] = { 20, 60, 150, 400, 1500, 5000, 10000, 20000 }; return e[juce::jlimit (0, NF, i)]; }
inline juce::String fasciaRange (int i)
{
    auto hz = [] (float f) { return f >= 1000 ? juce::String (f / 1000, f >= 10000 ? 0 : 1).replace (".0", "") + "k" : juce::String ((int) f); };
    return hz (fasciaEdge (i)) + "-" + hz (fasciaEdge (i + 1)) + " Hz";
}
enum Ref { R_TRAP, R_DRILL, R_BOOMBAP, R_RNB, R_POP, R_LOFI, R_ROCK, R_ELETTRO, kRefs };
inline const char* refName (int r) { static const char* n[kRefs] = { "TRAP", "DRILL", "BOOM BAP", "R&B", "POP", "LO-FI", "ROCK / LIVE", "ELETTRONICA" }; return n[juce::jlimit (0, kRefs - 1, r)]; }
// energia per ottava di ogni fascia rispetto ai medi (dB): medie indicative di mix commerciali del genere
inline const float* refCurve (int r)
{
    static const float c[kRefs][NF] = {
        { 6.0f, 5.0f, 1.0f, 0, -2.0f, -4.5f, -8.0f },   // trap
        { 7.0f, 4.0f, 0.5f, 0, -1.5f, -4.0f, -7.5f },   // drill
        { 2.5f, 5.0f, 2.0f, 0, -2.5f, -6.0f, -10.0f },  // boom bap
        { 4.0f, 4.0f, 1.0f, 0, -1.5f, -4.0f, -7.5f },   // r&b
        { 2.5f, 3.5f, 1.0f, 0, -1.0f, -3.5f, -7.0f },   // pop
        { 2.0f, 4.5f, 2.5f, 0, -3.5f, -8.0f, -13.0f },  // lo-fi
        { 0.0f, 3.0f, 1.5f, 0, 0.0f, -3.0f, -7.0f },    // rock
        { 5.0f, 4.5f, 0.5f, 0, -1.5f, -3.5f, -6.5f } }; // elettronica
    return c[juce::jlimit (0, kRefs - 1, r)];
}
inline float refTol (int i) { static const float t[NF] = { 3.5f, 2.5f, 2.5f, 2.0f, 2.5f, 3.0f, 4.0f }; return t[juce::jlimit (0, NF - 1, i)]; }
inline float refLufs (int r) { static const float l[kRefs] = { -8, -8, -9, -10, -9, -12, -9, -8 }; return l[juce::jlimit (0, kRefs - 1, r)]; }

struct Advice { float sev = 0; int fascia = -1; juce::String title, text, fix; };
struct Report
{
    bool ready = false; float seconds = 0;
    float dev[NF] {}, side[NF] {}, energy[NF] {};          // scarto dal riferimento (dB), side in %, energia per ottava (dB, gia' allineata)
    std::vector<float> dispHz, disp, ref;                   // curva per il grafico
    std::vector<std::pair<float, float>> res;               // risonanze: Hz, dB sopra i vicini
    float lufsI = -70, lra = 0, plr = 0, peak = -120, corr = 1;
    std::vector<Advice> advice; juce::String summary;
};

class Scan
{
public:
    static constexpr int ORD = 13, NFFT = 1 << ORD, HOP = NFFT / 2, RING = 1 << 17;
    // ---- thread audio: copia M/S nell'anello (niente calcoli)
    void push (const float* L, const float* R, int n)
    {
        juce::int64 w = wr.load (std::memory_order_relaxed);
        for (int i = 0; i < n; ++i, ++w) { ringL[(size_t) (w & (RING - 1))] = L[i]; ringR[(size_t) (w & (RING - 1))] = R[i]; }
        wr.store (w, std::memory_order_release);
    }
    void setRate (double s) { sr = s; }
    void requestReset() { resetReq = true; }
    // ---- thread messaggi: FFT, medie, battute
    void pump (double ppq, int bpb, bool playing, double bpm = 120)
    {
        if (resetReq.exchange (false)) clear();
        const juce::int64 w = wr.load (std::memory_order_acquire);
        if (w - rd > RING - NFFT) rd = w - NFFT;
        const double fps = sr / HOP, aRec = std::exp (-1.0 / (6.0 * fps));
        while (w - rd >= NFFT)
        {
            double eM = 0;
            for (int i = 0; i < NFFT; ++i)
            {
                const float l = ringL[(size_t) ((rd + i) & (RING - 1))], r = ringR[(size_t) ((rd + i) & (RING - 1))];
                const float m = 0.5f * (l + r) * win[(size_t) i], s = 0.5f * (l - r) * win[(size_t) i];
                bm[(size_t) i] = m; bs[(size_t) i] = s; bm[(size_t) (i + NFFT)] = 0; bs[(size_t) (i + NFFT)] = 0; eM += (double) m * m;
            }
            rd += HOP;
            if (eM / NFFT < 1e-7) continue;   // silenzio: non conta
            fft.performFrequencyOnlyForwardTransform (bm.data()); fft.performFrequencyOnlyForwardTransform (bs.data());
            double fe[NF] {};
            for (int k = 1; k < NFFT / 2; ++k)
            {
                const float pm = bm[(size_t) k] * bm[(size_t) k], ps = bs[(size_t) k] * bs[(size_t) k];
                recM[(size_t) k] = (float) (recM[(size_t) k] * aRec + pm * (1 - aRec)); recS[(size_t) k] = (float) (recS[(size_t) k] * aRec + ps * (1 - aRec));
                songM[(size_t) k] += pm; songS[(size_t) k] += ps;
                const int f = fasciaOfBin (k); if (f >= 0) fe[f] += pm + ps;
            }
            ++songN; recW = recW * aRec + (1 - aRec);
            if (playing)
            {
                // battuta del pezzo di audio appena analizzato (non di adesso): si torna indietro del ritardo dell'anello
                const double ago = (double) (w - (rd - HOP + NFFT / 2)) / sr, pq = ppq - ago * bpm / 60.0;
                const int bar = (int) std::floor (pq / juce::jmax (1, bpb)) + 1;
                auto& B = bars[bar]; for (int f = 0; f < NF; ++f) B.e[f] += fe[f]; ++B.n;
            }
        }
    }
    void clear()
    {
        std::fill (recM.begin(), recM.end(), 0.0f); std::fill (recS.begin(), recS.end(), 0.0f);
        std::fill (songM.begin(), songM.end(), 0.0); std::fill (songS.begin(), songS.end(), 0.0);
        songN = 0; recW = 0; bars.clear(); lufsHist.clear(); peakSong = -120;
    }
    Scan()
    {
        win.resize (NFFT); for (int i = 0; i < NFFT; ++i) win[(size_t) i] = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * i / NFFT);
        bm.assign (NFFT * 2, 0); bs.assign (NFFT * 2, 0); recM.assign (NFFT / 2, 0); recS.assign (NFFT / 2, 0); songM.assign (NFFT / 2, 0); songS.assign (NFFT / 2, 0);
        ringL.assign (RING, 0); ringR.assign (RING, 0);
    }
    // da processor (thread messaggi)
    void addLoudness (float lufsShort, float peakDb) { if (lufsShort > -60 && lufsHist.size() < 7200) lufsHist.push_back (lufsShort); peakSong = juce::jmax (peakSong, peakDb); }
    double seconds() const { return songN * HOP / sr; }

    Report analyse (int ref, bool song, float corrNow) const
    {
        Report R; R.seconds = (float) seconds();
        if (songN < 40) return R;   // meno di 2 secondi di musica
        const double wN = song ? (double) songN : juce::jmax (1e-9, recW);
        auto pM = [&] (int k) { return song ? songM[(size_t) k] / wN : recM[(size_t) k] / wN; };
        auto pS = [&] (int k) { return song ? songS[(size_t) k] / wN : recS[(size_t) k] / wN; };
        const double binHz = sr / NFFT;
        // energia per ottava di ogni fascia
        double raw[NF] {}, sideE[NF] {}, midE[NF] {};
        for (int k = 1; k < NFFT / 2; ++k) { const int f = fasciaOfBin (k); if (f < 0) continue; raw[f] += pM (k) + pS (k); sideE[f] += pS (k); midE[f] += pM (k); }
        const float* rc = refCurve (ref); double devRaw[NF]; double off = 0; int no = 0;
        for (int f = 0; f < NF; ++f)
        {
            const double oct = std::log2 (fasciaEdge (f + 1) / fasciaEdge (f));
            const double db = 10 * std::log10 (raw[f] / oct + 1e-20);
            devRaw[f] = db - rc[f]; if (f >= 1 && f <= 5) { off += devRaw[f]; ++no; }
            R.side[f] = (float) (100.0 * sideE[f] / (sideE[f] + midE[f] + 1e-20));
        }
        off /= no;
        for (int f = 0; f < NF; ++f) { R.dev[f] = (float) (devRaw[f] - off); R.energy[f] = R.dev[f] + rc[f]; }
        // curva: 1/6 di ottava, allineata come le fasce
        auto bandDb = [&] (double lo, double hi) { double e = 0; const int k0 = juce::jmax (1, (int) (lo / binHz)), k1 = juce::jmin (NFFT / 2 - 1, (int) (hi / binHz)); for (int k = k0; k <= k1; ++k) e += pM (k) + pS (k); if (k1 < k0) e = pM (k0) + pS (k0); return 10 * std::log10 (e / std::log2 (hi / lo) + 1e-20); };
        for (double f = 25; f < 18000; f *= std::pow (2.0, 1.0 / 24))
        {
            R.dispHz.push_back ((float) f); R.disp.push_back ((float) (bandDb (f * std::pow (2.0, -1.0 / 12), f * std::pow (2.0, 1.0 / 12)) - off));
            R.ref.push_back (refAt (ref, (float) f));
        }
        // risonanze: picchi stretti (1/24 ott) sopra i vicini (1/2 ott)
        {
            std::vector<std::pair<float, float>> c;
            for (double f = 80; f < 9000; f *= std::pow (2.0, 1.0 / 48))
            {   // finestra stretta mai sotto i 3 bin (in basso la risoluzione non basta per 1/24 di ottava)
                const double hw = juce::jmax (f * 0.0289, 1.6 * binHz); if (hw > f * 0.12) continue;
                const double fine = bandDb (f - hw, f + hw), wide = bandDb (f * std::pow (2.0, -0.25), f * std::pow (2.0, 0.25));
                const double ex = fine - wide; if (ex > 4.5) c.push_back ({ (float) f, (float) ex });
            }
            std::sort (c.begin(), c.end(), [] (auto& a, auto& b) { return a.second > b.second; });
            for (auto& x : c) { bool near = false; for (auto& y : R.res) if (std::abs (std::log2 (x.first / y.first)) < 0.25) near = true; if (! near) R.res.push_back (x); if (R.res.size() >= 3) break; }
        }
        // dinamica e loudness (sui LUFS brevi del brano)
        if (! lufsHist.empty())
        {
            double e = 0; for (auto l : lufsHist) e += std::pow (10.0, l / 10); const double ung = 10 * std::log10 (e / lufsHist.size());
            double e2 = 0; int n2 = 0; for (auto l : lufsHist) if (l > ung - 10) { e2 += std::pow (10.0, l / 10); ++n2; }
            R.lufsI = (float) (n2 ? 10 * std::log10 (e2 / n2) : ung);
            std::vector<float> v; for (auto l : lufsHist) if (l > R.lufsI - 20) v.push_back (l); std::sort (v.begin(), v.end());
            if (v.size() > 5) R.lra = v[(size_t) (v.size() * 0.95)] - v[(size_t) (v.size() * 0.10)];
            R.peak = peakSong; R.plr = peakSong - R.lufsI;
        }
        R.corr = corrNow;
        R.ready = true;
        advise (R, ref);
        return R;
    }
    std::atomic<juce::int64> wr { 0 };
private:
    double sr = 48000; juce::dsp::FFT fft { ORD };
    std::vector<float> win, bm, bs, recM, recS, ringL, ringR; std::vector<double> songM, songS;
    juce::int64 rd = 0, songN = 0; double recW = 0; std::atomic<bool> resetReq { false };
    struct BarE { double e[NF] {}; int n = 0; }; std::map<int, BarE> bars;
    // quota della fascia sull'energia della battuta: conta il cambio di colore, non il volume (un ritornello piu' forte e' normale)
    static double share (const BarE& b, int f) { double t = 0; for (int i = 0; i < NF; ++i) t += b.e[i]; return b.e[f] / (t + 1e-30); }
    std::vector<float> lufsHist; float peakSong = -120;
    int fasciaOfBin (int k) const { const double f = k * sr / NFFT; if (f < fasciaEdge (0) || f >= fasciaEdge (NF)) return -1; for (int i = 0; i < NF; ++i) if (f < fasciaEdge (i + 1)) return i; return -1; }
    static float refAt (int ref, float f)
    {
        const float* rc = refCurve (ref); float x[NF]; for (int i = 0; i < NF; ++i) x[i] = std::log2 (std::sqrt (fasciaEdge (i) * fasciaEdge (i + 1)));
        const float lf = std::log2 (f); if (lf <= x[0]) return rc[0]; if (lf >= x[NF - 1]) return rc[NF - 1];
        for (int i = 0; i < NF - 1; ++i) if (lf < x[i + 1]) { const float t = (lf - x[i]) / (x[i + 1] - x[i]); const float s = t * t * (3 - 2 * t); return rc[i] + (rc[i + 1] - rc[i]) * s; }
        return rc[NF - 1];
    }
    // il punto della fascia che sporge di piu' rispetto al riferimento (per dire DOVE intervenire)
    float peakIn (const Report& R, int f, bool up) const
    {
        float best = std::sqrt (fasciaEdge (f) * fasciaEdge (f + 1)), bv = -1e9f;
        for (size_t i = 0; i < R.dispHz.size(); ++i) { const float hz = R.dispHz[i]; if (hz < fasciaEdge (f) || hz >= fasciaEdge (f + 1)) continue; const float d = (R.disp[i] - R.ref[i]) * (up ? 1 : -1); if (d > bv) { bv = d; best = hz; } }
        return best;
    }
    static juce::String hzT (float f) { return f >= 1000 ? juce::String (f / 1000, 1) + " kHz" : juce::String (juce::roundToInt (f / 5) * 5) + " Hz"; }
    static juce::String dbT (float d) { return juce::String (std::abs (d), 1) + " dB"; }
    void advise (Report& R, int ref) const
    {
        auto U = [] (const char* s) { return juce::String::fromUTF8 (s); };
        auto add = [&R] (float sev, int fa, const juce::String& t, const juce::String& x, const juce::String& fix) { R.advice.push_back ({ sev, fa, t, x, fix }); };
        const juce::String rn = refName (ref);
        // --- equilibrio tonale per fascia
        for (int f = 0; f < NF; ++f)
        {
            const float d = R.dev[f], tol = refTol (f); if (std::abs (d) < tol) continue;
            const bool up = d > 0; const float at = peakIn (R, f, up), amt = juce::jlimit (1.0f, 4.0f, (std::abs (d) - tol * 0.5f) * 0.6f);
            const juce::String where = fasciaRange (f);
            juce::String t, x, fix;
            switch (f)
            {
                case 0: t = up ? U ("Troppo sub") : U ("Poco sub");
                        x = up ? U ("Il sub e' ") + dbT (d) + U (" sopra il riferimento ") + rn + U (": su impianti grandi rimbomba e ruba headroom al limiter.") : U ("Il sub e' ") + dbT (d) + U (" sotto il riferimento ") + rn + U (": sull'impianto del club il pezzo suona leggero.");
                        fix = up ? U ("Passa-alto 24 dB/ott a 25-30 Hz sul master e shelf basso -") + dbT (amt) + U (" a 50 Hz; se viene dall'808 abbassalo o accorcialo.") : U ("Shelf basso +") + dbT (amt) + U (" a 50 Hz, oppure 808 piu' lungo o saturato (808 Studio) cosi' si sente anche sui telefoni."); break;
                case 1: t = up ? U ("Bassi gonfi") : U ("Bassi magri");
                        x = up ? U ("Tra 60 e 150 Hz c'e' ") + dbT (d) + U (" in piu' (picco a ") + hzT (at) + U ("): cassa e 808/basso si pestano e il mix perde punch.") : U ("Tra 60 e 150 Hz mancano ") + dbT (d) + U (": la cassa non spinge e il basso non si sente.");
                        fix = up ? U ("Campana -") + dbT (amt) + U (" a ") + hzT (at) + U (" con Q 1; se sono cassa e 808 insieme, ducking dell'808 sulla cassa.") : U ("Campana +") + dbT (amt) + U (" a ") + hzT (at) + U (" con Q 0.8 sulla cassa o sul basso, non sul master se puoi."); break;
                case 2: t = up ? U ("Fango") : U ("Poco corpo");
                        x = up ? U ("Tra 150 e 400 Hz c'e' ") + dbT (d) + U (" in piu' (picco a ") + hzT (at) + U ("): suono impastato, scatolato, la voce perde definizione.") : U ("Tra 150 e 400 Hz mancano ") + dbT (d) + U (": mix sottile, senza calore.");
                        fix = up ? U ("Taglio largo -") + dbT (amt) + U (" a ") + hzT (at) + U (" con Q 1.2 su pad, chitarre, piano e cori (meglio traccia per traccia che sul master).") : U ("+") + dbT (amt) + U (" a 250 Hz con Q 0.8 su voce o accordi, o una saturazione calda (Tube Box)."); break;
                case 3: t = up ? U ("Medi in avanti") : U ("Medi scavati");
                        x = up ? U ("Tra 400 Hz e 1.5 kHz c'e' ") + dbT (d) + U (" in piu' (picco a ") + hzT (at) + U ("): suono nasale, da radiolina.") : U ("Tra 400 Hz e 1.5 kHz mancano ") + dbT (d) + U (": il mix a smile perde voce e melodie su telefoni e cuffiette.");
                        fix = up ? U ("Campana -") + dbT (amt) + U (" a ") + hzT (at) + U (" con Q 1.5; cerca la traccia che suona nasale.") : U ("+") + dbT (amt) + U (" tra 800 Hz e 1 kHz con Q 0.7, oppure togli meno medi da voce e melodie."); break;
                case 4: t = up ? U ("Duro / aggressivo") : U ("Poco presente");
                        x = up ? U ("Tra 1.5 e 5 kHz c'e' ") + dbT (d) + U (" in piu' (picco a ") + hzT (at) + U ("): stanca l'orecchio ad alto volume.") : U ("Tra 1.5 e 5 kHz mancano ") + dbT (d) + U (": voce e rullante non escono.");
                        fix = up ? U ("-") + dbT (amt) + U (" a ") + hzT (at) + U (" con Q 2, meglio con un EQ dinamico (scende solo quando picca).") : U ("+") + dbT (amt) + U (" a 3 kHz con Q 1 sulla voce, o un exciter leggero (Tube Box, ENHANCER)."); break;
                case 5: t = up ? U ("Troppo brillante") : U ("Spento");
                        x = up ? U ("Tra 5 e 10 kHz c'e' ") + dbT (d) + U (" in piu' (picco a ") + hzT (at) + U ("): sibilanti e hi-hat pungono.") : U ("Tra 5 e 10 kHz mancano ") + dbT (d) + U (": hi-hat e consonanti spenti, mix chiuso.");
                        fix = up ? U ("De-esser sulla voce e -") + dbT (amt) + U (" a ") + hzT (at) + U (" su hi-hat/piatti; sul master shelf -1 dB a 8 kHz.") : U ("Shelf alto +") + dbT (amt) + U (" a 8 kHz (su voce e hi-hat prima che sul master)."); break;
                default: t = up ? U ("Troppa aria") : U ("Poca aria");
                        x = up ? U ("Sopra 10 kHz c'e' ") + dbT (d) + U (" in piu': fruscio, sibili, digitale.") : U ("Sopra 10 kHz mancano ") + dbT (d) + U (": il mix non respira in alto.");
                        fix = up ? U ("Passa-basso dolce a 17-18 kHz o shelf -") + dbT (amt) + U (" a 12 kHz.") : U ("Shelf +") + dbT (amt) + U (" a 12 kHz (EQ pulito, Q basso).");
            }
            add (std::abs (d) / tol, f, t + "  " + where, x, fix);
        }
        // --- risonanze
        for (auto& rr : R.res)
            add (rr.second / 4.0f, -1, U ("Risonanza a ") + hzT (rr.first), U ("Una frequenza stretta sporge di ") + dbT (rr.second) + U (" sui vicini: si sente come un fischio o un rimbombo che torna sempre."),
                 U ("Taglio stretto -") + dbT (juce::jmin (6.0f, rr.second * 0.6f)) + U (" a ") + hzT (rr.first) + U (" con Q 6-8; se viene da una traccia sola, taglialo li' (Mixology, EQ con SOLO)."));
        // --- stereo
        if (R.side[0] + R.side[1] > 2 * 12) add ((R.side[0] + R.side[1]) / 2 / 12, 1, U ("Bassi larghi in stereo"), U ("Sotto 150 Hz il ") + juce::String (juce::roundToInt ((R.side[0] + R.side[1]) / 2)) + U ("% e' stereo: in mono e in club i bassi cambiano e perdono forza."), U ("Mono sotto 120 Hz (Master: IMAGER, o 808 Studio MONO SUB sulla traccia larga)."));
        if (R.side[5] < 6 && R.side[6] < 8) add (1.0f, 5, U ("Mix stretto in alto"), U ("Sopra 5 kHz quasi tutto e' al centro (side ") + juce::String (juce::roundToInt (R.side[5])) + U ("%): il mix suona piccolo."), U ("Allarga hi-hat, pad e riverberi (Motion, Space) invece di allargare il master."));
        for (int f = 2; f < NF; ++f) if (R.side[f] > 50) { add (1.5f, f, U ("Piu' side che centro: ") + fasciaName (f), U ("In ") + fasciaRange (f) + U (" il side supera il centro: possibile fase invertita o wider esagerato, in mono sparisce."), U ("Controlla in mono; togli o riduci i wider su quella zona.")); break; }
        if (R.corr < 0.1f) add (2.0f, -1, U ("Problema di fase"), U ("Correlazione ") + juce::String (R.corr, 2) + U (": canali quasi opposti, in mono il mix perde pezzi."), U ("Cerca la traccia con la fase girata o un wider troppo aperto."));
        // --- dinamica e loudness
        if (R.lufsI > -60)
        {
            const float tl = refLufs (ref);
            if (R.lufsI > tl + 2) add ((R.lufsI - tl) / 2, -1, U ("Troppo forte per ") + rn, U ("Loudness del brano ") + juce::String (R.lufsI, 1) + U (" LUFS, il riferimento e' ") + juce::String (tl, 0) + U (": stai schiacciando i colpi."), U ("Togli 1-2 dB di guadagno al limiter; su Spotify verra' comunque abbassato a -14."));
            else if (R.lufsI < tl - 3) add (juce::jmin (1.2f, (tl - R.lufsI) / 3), -1, U ("Piano rispetto a ") + rn, U ("Loudness del brano ") + juce::String (R.lufsI, 1) + U (" LUFS, il riferimento e' ") + juce::String (tl, 0) + U ("."), U ("Se e' gia' il master finale: clipper leggero e limiter fino a ") + juce::String (tl, 0) + U (" LUFS (Jackpot Master)."));
            if (R.plr > 0 && R.plr < 7) add ((7 - R.plr) / 2 + 1, -1, U ("Schiacciato"), U ("PLR ") + juce::String (R.plr, 1) + U (" dB (picco meno loudness): colpi piatti, cassa e rullante senza impatto."), U ("Meno limiter, o un clipper prima del limiter per tenere i transienti."));
            else if (R.plr > 16) add (1.0f, -1, U ("Molto dinamico"), U ("PLR ") + juce::String (R.plr, 1) + U (" dB: i picchi sono molto sopra il corpo del suono."), U ("Clipper leggero sulla batteria o sul master per guadagnare volume senza perdere niente."));
            if (R.lra > 0 && R.lra < 3 && R.seconds > 60) add (1.0f, -1, U ("Tutto alla stessa intensita'"), U ("LRA ") + juce::String (R.lra, 1) + U (" LU: strofa e ritornello hanno lo stesso volume percepito."), U ("Lascia piu' differenza fra le parti: ritornello piu' pieno, strofe piu' leggere."));
            if (R.peak > -0.5f) add (1.5f, -1, U ("Picchi a 0 dB"), U ("Il master arriva a ") + juce::String (R.peak, 1) + U (" dB: sulle piattaforme la conversione puo' distorcere."), U ("Ceiling del limiter a -1 dB (true peak)."));
        }
        // --- battute dove una fascia salta rispetto al resto del brano
        if (bars.size() >= 8)
        {
            double mean[NF] {}; int nb = 0; for (auto& kv : bars) if (kv.second.n > 2) ++nb;
            if (nb > 4)
            {   // mediana di ogni fascia sulle battute: il punto di riferimento "normale" del brano
                for (int f = 0; f < NF; ++f) { std::vector<double> v; for (auto& kv : bars) if (kv.second.n > 2) v.push_back (share (kv.second, f)); std::nth_element (v.begin(), v.begin() + (long) v.size() / 2, v.end()); mean[f] = v[v.size() / 2]; }
                int bf = -1, bb0 = 0, bb1 = 0; float bd = 0;
                for (int f = 0; f < NF; ++f)
                {
                    int run0 = -1, last = -10; float runMax = 0;
                    auto close = [&] { if (run0 >= 0 && runMax > bd) { bd = runMax; bf = f; bb0 = run0; bb1 = last; } run0 = -1; runMax = 0; };
                    for (auto& kv : bars)
                    {
                        if (kv.second.n <= 2) continue;
                        const float d = (float) (10 * std::log10 ((share (kv.second, f) + 1e-20) / (mean[f] + 1e-20)));
                        if (d > 4.0f) { if (run0 < 0 || kv.first != last + 1) { close(); run0 = kv.first; } last = kv.first; runMax = juce::jmax (runMax, d); }
                        else close();
                    }
                    close();
                }
                if (bf >= 0) add (bd / 4.5f, bf, U ("Salto di ") + fasciaName (bf) + U (" in battuta ") + juce::String (bb0) + (bb1 > bb0 ? "-" + juce::String (bb1) : juce::String()),
                                  fasciaName (bf) + U (" (") + fasciaRange (bf) + U (") sale di ") + dbT (bd) + U (" rispetto al resto del brano (a parita' di volume: cambia il colore del mix)."),
                                  U ("Controlla cosa entra in quelle battute (un 808 piu' lungo, un pad, un riser) e livellalo o automatizza l'EQ."));
            }
        }
        std::sort (R.advice.begin(), R.advice.end(), [] (const Advice& a, const Advice& b) { return a.sev > b.sev; });
        if (R.advice.empty()) R.summary = U ("Equilibrato rispetto a ") + rn + U (": niente di importante da sistemare.");
        else
        {
            juce::StringArray a; for (size_t i = 0; i < R.advice.size() && i < 3; ++i) { auto t = R.advice[i].title.upToFirstOccurrenceOf ("  ", false, false); a.add (t.substring (0, 1).toLowerCase() + t.substring (1)); }
            R.summary = U ("Rispetto a ") + rn + ": " + a.joinIntoString (", ") + ".";
        }
    }
};
}
