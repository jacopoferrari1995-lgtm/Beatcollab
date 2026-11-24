#include <cstdlib>
#include "../Source/MasterProcessor.h"
#include "../Source/MasterEditor.h"
using namespace jms;

struct FakeHead : juce::AudioPlayHead
{
    double sr = 48000, bpm = 92; juce::int64 pos = 0;
    juce::Optional<PositionInfo> getPosition() const override
    { PositionInfo i; i.setBpm (bpm); i.setIsPlaying (true); i.setPpqPosition (pos / sr * bpm / 60.0); i.setTimeSignature (TimeSignature { 4, 4 }); return i; }
};
static FakeHead* gHead = nullptr;
static void setP (Processor& p, const char* id, float v) { auto* rp = p.apvts.getParameter (id); rp->setValueNotifyingHost (rp->convertTo0to1 (v)); }
static void run (Processor& p, std::function<void (int, float&, float&)> gen, int len, std::vector<float>& oL, std::vector<float>& oR, int block = 512)
{
    juce::AudioBuffer<float> b (2, block); juce::MidiBuffer mb;
    for (int off = 0; off < len; off += block)
    {
        const int n = juce::jmin (block, len - off); b.setSize (2, n, false, false, true);
        for (int i = 0; i < n; ++i) { float l, r; gen (off + i, l, r); b.setSample (0, i, l); b.setSample (1, i, r); }
        p.processBlock (b, mb); if (gHead) gHead->pos += n;
        for (int i = 0; i < n; ++i) { oL.push_back (b.getSample (0, i)); oR.push_back (b.getSample (1, i)); }
    }
}
// true peak misurato con sovracampionamento 8x indipendente
static float measTP (const std::vector<float>& x, int from)
{
    const int n = (int) x.size() - from; juce::dsp::Oversampling<float> os (1, 3, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true);
    os.initProcessing ((size_t) n); juce::AudioBuffer<float> b (1, n); for (int i = 0; i < n; ++i) b.setSample (0, i, x[(size_t) (from + i)]);
    juce::dsp::AudioBlock<float> blk (b); auto up = os.processSamplesUp (blk); float pk = 0;
    for (size_t i = 0; i < up.getNumSamples(); ++i) pk = juce::jmax (pk, std::abs (up.getSample (0, (int) i)));
    return gToDb (pk);
}
static float thd (const std::vector<float>& x, int from, double sr, float f0)
{
    const int order = 15, N = 1 << order; juce::dsp::FFT f (order); std::vector<float> d ((size_t) (2 * N), 0);
    for (int i = 0; i < N; ++i) d[(size_t) i] = x[(size_t) (from + i)] * (0.5f - 0.5f * std::cos (6.2831853f * i / N));
    f.performFrequencyOnlyForwardTransform (d.data(), true);
    auto at = [&] (float hz) { const int k = juce::roundToInt (hz * N / sr); float m = 0; for (int j = k - 3; j <= k + 3; ++j) m = juce::jmax (m, d[(size_t) j]); return m; };
    const float h1 = at (f0); double s = 0; for (int h = 2; h <= 10; ++h) { const float a = at (f0 * h) / h1; s += a * a; }
    return (float) std::sqrt (s) * 100;
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    const double sr = 48000;
    auto blank = [&] { auto p = std::make_unique<Processor>(); p->setPlayConfigDetails (2, 2, sr, 512); p->prepareToPlay (sr, 512); return p; };
    auto fresh = [&] { auto p = blank(); setP (*p, "mode", 1); setP (*p, "mx_on", 1); setP (*p, "lm_on", 1); setP (*p, "mx_gain", 4); return p; };
    juce::Random rnd (7);
    // materiale di prova "beat": cassa (sinusoide che scende), rullante (rumore), 808, hat, con picchi tra i campioni
    auto beat = [&] (int i, float& l, float& r)
    {
        const double t = i / sr, bt = std::fmod (t, 0.5), bar = std::fmod (t, 2.0);
        double k = bt < 0.25 ? std::sin (2 * 3.14159 * (50 + 120 * std::exp (-bt * 40)) * bt) * std::exp (-bt * 9) : 0;
        double s = (bar > 0.5 && bar < 0.7) || (bar > 1.5 && bar < 1.7) ? (rnd.nextFloat() * 2 - 1) * std::exp (-(std::fmod (bar, 1.0) - 0.5) * 25) : 0;
        double sub = 0.35 * std::sin (2 * 3.14159 * 49 * t);
        double hat = (std::fmod (t, 0.125) < 0.02 ? (rnd.nextFloat() * 2 - 1) * 0.15 : 0);
        double isp = 0.2 * std::sin (2 * 3.14159 * sr / 4 * t + 0.785);
        const double m = 0.42 * (k * 0.9 + s * 0.5 + sub + hat) + 0.1 * isp;
        l = (float) m; r = (float) (m * 0.95);
    };

    if (argc > 1 && juce::String (argv[1]) == "bassclick")
    {   // ogni comando del BASS salta 4 volte su un tono basso + uno medio: energia sopra 5 kHz nel salto contro fermo
        const char* ids[] = { "bc_freq", "bc_mono", "bc_punch", "bc_tail", "bc_harm", "bc_bal", "bc_thr", "bc_dirty" };
        for (auto* id : ids)
        {
            auto runIt = [&] (int mode)
            {
                auto p = blank(); setP (*p, "mode", 1); setP (*p, "engine", 1); setP (*p, "bc_on", 1); setP (*p, "bc_harm", 0.3f); setP (*p, "bc_thr", -6); setP (*p, "bc_punch", 0.3f);
                auto* prm = p->apvts.getParameter (id); const float v0 = prm->getValue(), v1 = v0 > 0.5f ? v0 - 0.3f : v0 + 0.3f; if (mode == 1) prm->setValueNotifyingHost (v1);
                std::vector<float> oL, oR; juce::AudioBuffer<float> b (2, 512); juce::MidiBuffer mb;
                for (int k = 0; k < 400; ++k)
                {
                    if (mode == 2 && k >= 200 && (k - 200) % 40 == 0 && k < 360) prm->setValueNotifyingHost (((k - 200) / 40) % 2 ? v0 : v1);
                    for (int i = 0; i < 512; ++i) { const double t = (k * 512 + i) / sr; const float v = (float) (0.4 * std::sin (2 * 3.14159265 * 55 * t) + 0.1 * std::sin (2 * 3.14159265 * 1000 * t)); b.setSample (0, i, v); b.setSample (1, i, v); }
                    p->processBlock (b, mb); for (int i = 0; i < 512; ++i) oL.push_back (b.getSample (0, i));
                }
                return oL;
            };
            const auto a = runIt (0), c1 = runIt (1), j = runIt (2);
            auto hf = [&] (const std::vector<float>& x, size_t at) { Biquad h1, h2; h1.set (Biquad::LCUT, 5000, 0, 0.707, sr); h2 = h1; double m = 0; for (size_t i = at - 2000; i < at + 4000; ++i) { const float y = h2 (h1 (x[i])); if (i > at - 1000) m = juce::jmax (m, (double) std::abs (y)); } return m; };
            double worst = -200;
            for (int q = 0; q < 4; ++q) { const size_t at = (size_t) (200 + q * 40) * 512; const double base = juce::jmax (hf (a, at), hf (c1, at)); worst = juce::jmax (worst, 20 * std::log10 ((hf (j, at) + 1e-12) / 0.5)); (void) base; }
            std::printf ("  %-9s picco sopra 5 kHz nel salto %.1f dB (rispetto al tono)\n", id, worst);
        }
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "xtalk")
    {   // solo il sinistro in ingresso: quanto esce a destra
        auto test = [&] (const char* name, std::function<void (Processor&)> setup)
        {
            auto p = blank(); setup (*p); juce::Random rr (3); std::vector<float> oL, oR; const int len = (int) sr * 6;
            run (*p, [&] (int, float& l, float& r) { l = 0.25f * (rr.nextFloat() * 2 - 1); r = 0; }, len, oL, oR);
            double eL = 0, eR = 0; for (int i = len / 2; i < len; ++i) { eL += (double) oL[(size_t) i] * oL[(size_t) i]; eR += (double) oR[(size_t) i] * oR[(size_t) i]; }
            std::printf ("%-36s destra %.1f dB rispetto alla sinistra\n", name, 10 * std::log10 (eR / (eL + 1e-30) + 1e-30));
        };
        test ("FACILE di partenza", [] (Processor&) {});
        test ("e_wide +0.5", [] (Processor& p) { setP (p, "e_wide", 0.5f); });
        test ("e_wide -0.5", [] (Processor& p) { setP (p, "e_wide", -0.5f); });
        test ("e_monob", [] (Processor& p) { setP (p, "e_monob", 1); });
        for (int st = 0; st < 5; ++st) test (("LOUD 7 stile " + juce::String (st)).toRawUTF8(), [st] (Processor& p) { setP (p, "e_loud", 7); setP (p, "e_style", (float) st); setP (p, "e_pres", 0.8f); });
        test ("FACILE LOUD 7", [] (Processor& p) { setP (p, "e_loud", 7); });
        test ("FACILE LOUD 7 + calma/colla", [] (Processor& p) { setP (p, "e_loud", 7); setP (p, "e_glue", 0.5f); setP (p, "e_calm", 0.5f); });
        for (auto* id : { "e_low", "e_mid", "e_high" }) test (id, [id] (Processor& p) { setP (p, id, 2); });
        for (auto* id : { "e_calm", "e_thick", "e_warm", "e_silk", "e_glue" }) test (id, [id] (Processor& p) { setP (p, id, 0.5f); });
        const char* mods[] = { "bc_on", "eq_on", "ds_on", "bus_on", "mb_on", "ex_on", "tp_on", "im_on", "mx_on", "lm_on", "e4_on", "pq_on", "vm_on", "ip_on", "sp_on", "st_on" };
        for (auto* id : mods) test (id, [id] (Processor& p) { setP (p, "mode", 1); setP (p, "engine", 1); setP (p, id, 1); setP (p, "mx_gain", 6); });
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "tapenoise")
    {   // solo il nastro acceso: silenzio, tono debole, e residuo rispetto al tono
        for (int model = 0; model < 3; ++model) for (float drv : { 0.0f, 0.5f, 1.0f })
        {
            auto p = blank(); setP (*p, "mode", 1); setP (*p, "engine", 1); setP (*p, "tp_on", 1); setP (*p, "tp_model", (float) model); setP (*p, "tp_drive", drv);
            if (auto* nz = std::getenv ("JNOISE")) setP (*p, "tp_noise", (float) std::atof (nz));
            std::vector<float> oL, oR; const int len = (int) sr * 4;
            run (*p, [&] (int i, float& l, float& r) { const double t = i / sr; l = r = i < len / 2 ? 0.0f : (float) (0.05 * std::sin (2 * 3.14159265 * 440 * t)); }, len, oL, oR);
            double eS = 0; for (int i = len / 4; i < len / 2 - 1000; ++i) eS += (double) oL[(size_t) i] * oL[(size_t) i];
            // residuo sul tono: togli la fondamentale (fit) e misura quel che resta
            const int a = len / 2 + (int) sr / 2, b = len; double re = 0, im = 0; for (int i = a; i < b; ++i) { const double w = 2 * 3.14159265 * 440 * i / sr; re += oL[(size_t) i] * std::cos (w); im += oL[(size_t) i] * std::sin (w); }
            re *= 2.0 / (b - a); im *= 2.0 / (b - a); double eR = 0, eT = 0; for (int i = a; i < b; ++i) { const double w = 2 * 3.14159265 * 440 * i / sr; const double f = re * std::cos (w) + im * std::sin (w); eR += std::pow (oL[(size_t) i] - f, 2.0); eT += f * f; }
            std::printf ("nastro %d drive %.1f: silenzio %.1f dBFS, residuo sul tono a -26 dBFS %.1f dB\n", model, drv, 10 * std::log10 (eS / (len / 4 - 1000) + 1e-30), 10 * std::log10 (eR / eT + 1e-30));
        }
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "harm")
    {   // THICK e SILK: armoniche prodotte e sporco fuori posto (aliasing) su sinusoidi
        juce::ScopedJuceInitialiser_GUI gh; const double fs = 44100; const int N = 1 << 15;
        auto spec = [&] (std::vector<float>& x, float f0, const char* nm)
        {
            juce::dsp::FFT fft (15); std::vector<float> a ((size_t) 2 * N, 0); for (int i = 0; i < N; ++i) a[(size_t) i] = x[(size_t) (x.size() - N + i)] * (0.5f - 0.5f * std::cos (6.2831853f * i / N));
            fft.performFrequencyOnlyForwardTransform (a.data());
            auto mag = [&] (double f) { int b = (int) std::round (f / fs * N); float m = 0; for (int k = b - 3; k <= b + 3; ++k) if (k > 0 && k < N / 2) m = std::max (m, a[(size_t) k]); return 20 * std::log10 (m + 1e-9f); };
            const float ref = mag (f0); std::printf ("%-6s %6.0f Hz: ", nm, f0);
            for (int h = 2; h <= 5; ++h) { const double f = f0 * h; if (f < fs / 2) std::printf ("H%d %+6.1f  ", h, mag (f) - ref); else std::printf ("H%d  oltre  ", h); }
            // sporco: il bin piu' alto che non e' armonico (aliasing)
            float worst = -300; for (int k = 20; k < N / 2; ++k) { const double f = k * fs / N; bool harm = false; for (int h = 1; h <= 40; ++h) if (std::abs (f - f0 * h) < 25) harm = true; if (! harm) worst = std::max (worst, 20 * std::log10 (a[(size_t) k] + 1e-9f) - ref); }
            std::printf (" sporco max %+6.1f dB\n", worst);
        };
        for (int which = 0; which < 2; ++which)
            for (float f0 : which == 0 ? std::vector<float> { 80.0f, 300.0f, 900.0f } : std::vector<float> { 5000.0f, 9000.0f, 14000.0f })
            {
                jms::EasyEngine e; e.prepare (fs); jms::EasySettings S; S.monoBass = false; if (which == 0) S.thick = 1; else S.silk = 1;
                std::vector<float> L ((size_t) N * 2), R ((size_t) N * 2);
                for (int i = 0; i < N * 2; ++i) L[(size_t) i] = R[(size_t) i] = 0.5f * (float) std::sin (2 * 3.14159265 * f0 * i / fs);
                for (int off = 0; off < N * 2; off += 512) e.process (L.data() + off, R.data() + off, 512, S);
                spec (L, f0, which == 0 ? "THICK" : "SILK");
            }
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "lim3")
    {   // i tre limiter a ogni sovracampionamento: picco vero in uscita, riduzione, latenza
        juce::ScopedJuceInitialiser_GUI gl; const double fs = 48000; juce::Random r (3);
        for (int t = 0; t < 3; ++t) for (int o = 0; o < 5; ++o)
        {
            jms::Limiter lm; lm.prepare (fs, 512); jms::LimSettings S; S.type = t; S.os = o; S.truePeak = o >= 2; S.ceilDb = -1; S.enhance = t == 1 ? 0.5f : 0;
            std::vector<float> L (512 * 280), R (512 * 280); double ph = 0;
            for (size_t i = 0; i < L.size(); ++i) { const double tt = i / fs, b = std::fmod (tt * 2, 1.0); ph += 2 * 3.14159 * (50 + 400 * std::exp (-b * 40)) / fs; L[i] = (float) (1.6 * std::sin (ph) * std::exp (-b * 5) + 0.3 * std::sin (2 * 3.14159 * 3000 * tt) + (r.nextFloat() - 0.5f) * 0.4f); R[i] = L[i] * 0.9f; }
            float grm = 0; for (size_t off = 0; off + 512 <= L.size(); off += 512) { lm.process (L.data() + off, R.data() + off, 512, S); grm = std::max (grm, lm.gr); }
            jms::TruePeak tp; float pk = 0; for (size_t i = 48000; i < L.size(); ++i) pk = std::max (pk, tp.push (L[i]));
            std::printf ("tipo %d os %2dx: true peak %+5.2f dB  GR max %4.1f  latenza %d\n", t, 1 << o, 20 * std::log10 (pk), grm, lm.latency());
        }
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "tp")
    {
        for (double f : { 1000.0, 5000.0, 12000.0, 17000.0, 20000.0 })
        {
            TruePeak tp; float mx = 0, sp = 0;
            for (int i = 0; i < 48000; ++i) { const float x = 0.5f * (float) std::sin (2 * 3.14159265358979 * f * i / 48000.0 + 0.785); mx = juce::jmax (mx, tp.push (x)); sp = juce::jmax (sp, std::abs (x)); }
            std::printf ("%6.0f Hz: stima %.4f  campione %.4f (vero 0.5)\n", f, mx, sp);
        }
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "lim")
    {
        Limiter lm; lm.prepare (sr); LimSettings ls; ls.ceilDb = -1;
        std::vector<float> a ((size_t) sr * 4), b ((size_t) sr * 4);
        for (int i = 0; i < (int) a.size(); ++i) { float l, r; beat (i, l, r); a[(size_t) i] = l * 4; b[(size_t) i] = r * 4; }
        // limitatore senza la sicurezza finale: guardiamo i picchi
        lm.process (a.data(), b.data(), (int) a.size(), ls);
        int hits = 0; for (auto v : a) if (std::abs (v) >= dbToG (-1.0f) * 0.9999f) ++hits;
        std::printf ("campioni al ceiling: %d, TP (JUCE 8x) %.2f\n", hits, measTP (a, 1000));
        // riferimento: sinc lungo (256 prese, finestra Kaiser) a 32x su tutto il segnale
        float best = 0; TruePeak t2; float mine = 0;
        for (size_t i = 0; i < a.size(); ++i) mine = juce::jmax (mine, t2.push (a[i]));
        for (int i = 200; i < (int) a.size() - 200; ++i)
        {
            if (std::abs (a[(size_t) i]) < 0.75f) continue;
            for (int ph = 1; ph < 32; ++ph)
            {
                const double pos = i + ph / 32.0; double s2 = 0;
                for (int k = -128; k < 128; ++k) { const int j = (int) std::floor (pos) + k; const double x = pos - j; const double sn = std::sin (3.14159265358979 * x) / (3.14159265358979 * x);
                    const double r = x / 129.0; const double w = 0.5 + 0.5 * std::cos (3.14159265358979 * r); s2 += a[(size_t) j] * sn * w; }
                best = juce::jmax (best, (float) std::abs (s2));
            }
        }
        std::printf ("TP di riferimento (sinc 256, 32x): %.2f   stima del limiter: %.2f\n", gToDb (best), gToDb (mine));
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "easy")
    {   // modalita FACILE: ogni preset, livello d'uscita, true peak, riduzione, indice crush; poi le manopole una alla volta
        for (int i = 0; i < (int) presets().size(); ++i)
        {
            if (! juce::String (presets()[(size_t) i].name).startsWith ("Facile")) continue;
            auto p = blank(); p->loadPreset (i); if (std::getenv ("JAUTO")) setP (*p, "trim", 7.6f);
            std::vector<float> a, b; run (*p, [&] (int k, float& l, float& r) { beat (k, l, r); l *= 0.6f; r *= 0.6f; }, (int) sr * 14, a, b);
            float nan = 0; for (float v : a) if (! std::isfinite (v)) nan = 1;
            std::printf ("%-28s mode %d  in %.1f  out S %.1f I %.1f LUFS  TP %.2f (8x %.2f)  GR %.1f  crush %.2f %s\n", presets()[(size_t) i].name, (int) p->value ("mode"), p->inS.load(), p->lufsS.load(), p->lufsI.load(), p->tpMax.load(), measTP (a, (int) sr * 4), p->grAvg.load(), p->crush.load(), nan > 0 ? "NAN!" : "");
        }
        for (auto* id : { "e_low", "e_mid", "e_high", "e_thick", "e_warm", "e_silk", "e_glue", "e_calm", "e_wide", "e_loud" })
        {
            auto p0 = blank(); auto p1 = blank(); setP (*p1, id, juce::String (id) == "e_loud" ? 6.0f : (juce::String (id).startsWith ("e_l") || juce::String (id) == "e_mid" || juce::String (id) == "e_high") ? 4.0f : 1.0f);
            std::vector<float> a0, b0, a1, b1;
            run (*p0, [&] (int k, float& l, float& r) { beat (k, l, r); l *= 0.5f; r *= 0.5f; }, (int) sr * 6, a0, b0);
            juce::Random rr (7); rnd = rr;
            run (*p1, [&] (int k, float& l, float& r) { beat (k, l, r); l *= 0.5f; r *= 0.5f; }, (int) sr * 6, a1, b1);
            double e0 = 0, e1 = 0, ed = 0; for (size_t k = (size_t) sr * 2; k < a0.size(); ++k) { e0 += a0[k] * a0[k]; e1 += a1[k] * a1[k]; ed += (a1[k] - a0[k]) * (a1[k] - a0[k]); }
            std::printf ("%-8s livello %+.2f dB   quanto cambia il suono %.1f dB\n", id, 10 * std::log10 (e1 / e0), 10 * std::log10 (ed / e0 + 1e-20));
        }
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "loud")
    {   // LOUD: i 5 caratteri, il ceiling rispettato (true peak misurato 8x), il delta, il solo/delta delle bande
        auto go = [&] (std::function<void (Processor&)> set, std::vector<float>& a, std::vector<float>& b) { auto p = blank(); set (*p); run (*p, [&] (int k, float& l, float& r) { beat (k, l, r); l *= 0.6f; r *= 0.6f; }, (int) sr * 10, a, b); return p; };
        const char* stn[] = { "MODERN", "AGGRESSIVE", "SMOOTH", "SAFE", "CLASSIC" };
        for (float ceil : { -0.3f, -1.0f, -3.0f })
            for (int st = 0; st < 5; ++st)
            {
                std::vector<float> a, b; auto p = go ([&] (Processor& q) { setP (q, "e_loud", 10); setP (q, "e_style", (float) st); setP (q, "lm_ceil", ceil); setP (q, "e_pres", 0.7f); }, a, b);
                std::printf ("ceil %5.1f %-10s  out S %.1f LUFS  TP 8x %.2f %s  GR %.1f\n", ceil, stn[st], p->lufsS.load(), measTP (a, (int) sr * 3), measTP (a, (int) sr * 3) <= ceil + 0.05f ? "ok" : "<<< SFORA", p->grAvg.load());
            }
        {
            std::vector<float> a, b; auto p = go ([&] (Processor& q) { setP (q, "e_loud", 8); setP (q, "e_delta", 1); }, a, b);
            double e = 0; for (size_t k = (size_t) sr * 4; k < a.size(); ++k) e += a[k] * a[k];
            std::vector<float> a0, b0; go ([&] (Processor& q) { setP (q, "e_loud", 8); }, a0, b0);
            double e0 = 0; for (size_t k = (size_t) sr * 4; k < a0.size(); ++k) e0 += a0[k] * a0[k];
            std::printf ("DELTA con LOUD 8: differenza %.1f dB rispetto al master\n", 10 * std::log10 (e / e0 + 1e-20));
            std::vector<float> a2, b2; go ([&] (Processor& q) { setP (q, "e_delta", 1); }, a2, b2);
            double e2 = 0; for (size_t k = (size_t) sr * 4; k < a2.size(); ++k) e2 += a2[k] * a2[k];
            std::printf ("DELTA a manopole a zero: %.1f dB (deve essere quasi silenzio)\n", 10 * std::log10 (e2 / e0 + 1e-20));
        }
        for (int lb = 0; lb < 6; ++lb)
        {
            std::vector<float> a, b; auto p = blank(); setP (*p, "e_low", 6); setP (*p, "e_mid", 0); setP (*p, "e_high", 6); p->eqListen = lb;
            run (*p, [&] (int k, float& l, float& r) { beat (k, l, r); l *= 0.6f; r *= 0.6f; }, (int) sr * 6, a, b);
            double e = 0; for (size_t k = (size_t) sr * 2; k < a.size(); ++k) e += a[k] * a[k];
            std::printf ("ascolto banda %d (%s %s): livello %.1f dB\n", lb % 3, lb < 3 ? "SOLO" : "DELTA", lb % 3 == 0 ? "bassi" : lb % 3 == 1 ? "medi (a zero)" : "alti", 10 * std::log10 (e / a.size() + 1e-20));
        }
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "xmode")
    {   // un preset non cambia modalita e l'altra vista viene tradotta
        juce::ScopedJuceInitialiser_GUI gui2; Processor p; p.prepareToPlay (48000, 512);
        int fi = -1, gi = -1; for (int i = 1; i < (int) presets().size(); ++i) { juce::String n (presets()[(size_t) i].name); if (fi < 0 && n.startsWith ("Facile")) fi = i; if (gi < 0 && n.startsWith ("Genere")) gi = i; }
        p.loadPreset (fi); std::printf ("FACILE preset %s -> mode %d  mx_on %.0f mx_gain %.1f bus_on %.0f eq_on %.0f ex_on %.0f tp %.0f im %.0f\n", presets()[(size_t) fi].name, (int) p.value ("mode"), p.value ("mx_on"), p.value ("mx_gain"), p.value ("bus_on"), p.value ("eq_on"), p.value ("ex_on"), p.value ("tp_on"), p.value ("im_on"));
        p.loadPreset (gi); std::printf ("AVANZ preset %s -> mode %d  e_loud %.1f e_glue %.2f e_low %.1f e_high %.1f e_calm %.2f e_wide %.2f\n", presets()[(size_t) gi].name, (int) p.value ("mode"), p.value ("e_loud"), p.value ("e_glue"), p.value ("e_low"), p.value ("e_high"), p.value ("e_calm"), p.value ("e_wide"));
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "emul")
    {   // risposta del Pultec contro le specifiche dell'EQP-1A
        auto resp = [] (PultecSettings S, std::initializer_list<float> fs)
        {
            Biquad f[4]; bool act[4]; pultecDesign (S, 48000, f, act); juce::String o;
            for (float fr : fs) { double d = 0; for (int b = 0; b < 4; ++b) if (act[b]) d += f[b].magDb (fr, 48000); o << juce::String ((int) fr) << "Hz " << juce::String (d, 1) << "  "; }
            return o;
        };
        PultecSettings a; a.lf = 2; a.lb = 10; std::printf ("LF boost 10 @60:     %s\n", resp (a, { 20, 40, 60, 100, 200, 500 }).toRawUTF8());
        PultecSettings b; b.lf = 2; b.la = 10; std::printf ("LF atten 10 @60:     %s\n", resp (b, { 20, 40, 60, 100, 200, 500 }).toRawUTF8());
        PultecSettings c; c.lf = 2; c.lb = 10; c.la = 10; std::printf ("trucco 10+10 @60:    %s\n", resp (c, { 20, 40, 60, 100, 200, 300, 500, 1000 }).toRawUTF8());
        PultecSettings d; d.hf = 4; d.hb = 10; d.bw = 5; std::printf ("HF boost 10 @10k:    %s\n", resp (d, { 2000, 5000, 10000, 15000, 20000 }).toRawUTF8());
        PultecSettings e; e.as = 2; e.ha = 10; std::printf ("HF atten 10 @20k:    %s\n", resp (e, { 2000, 5000, 10000, 15000, 20000 }).toRawUTF8());
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "presets")
    {   // ogni preset (anche quelli per effetto): niente NaN, picchi sotto il ceiling, loudness ragionevole
        int bad = 0;
        auto check = [&] (const juce::String& name, std::function<void (Processor&)> load)
        {
            auto p = fresh(); load (*p); std::vector<float> a, b; run (*p, beat, (int) sr * 8, a, b);
            float pk = 0; bool nan = false; for (size_t i = a.size() / 2; i < a.size(); ++i) { if (! std::isfinite (a[i]) || ! std::isfinite (b[i])) nan = true; pk = std::max (pk, std::max (std::abs (a[i]), std::abs (b[i]))); }
            const float pkDb = juce::Decibels::gainToDecibels (pk, -100.0f); const bool ko = nan || pkDb > 0.05f;
            if (ko) ++bad;
            std::printf ("%s %-46s picco %6.2f dB  LUFS-S %6.1f%s\n", ko ? "!!" : "  ", name.toRawUTF8(), pkDb, p->lufsS.load(), nan ? " NaN" : "");
        };
        for (int i = 0; i < (int) presets().size(); ++i) { const juce::String n (presets()[(size_t) i].name); if (n.startsWith ("Genere") || n.startsWith ("Specifico")) check (n, [i] (Processor& q) { q.loadPreset (i); }); }
        for (int i = 0; i < (int) modPresets().size(); ++i) check (juce::String ("  [") + modInfo (modPresets()[(size_t) i].mod).name + "] " + modPresets()[(size_t) i].name, [i] (Processor& q) { auto* m = q.apvts.getParameter ("mode"); m->setValueNotifyingHost (1.0f); q.loadModPreset (i); });
        std::printf ("preset con problemi: %d\n", bad);
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "deess")
    {   // beat + "s" (rumore 5-9 kHz, 80 ms ogni 0,75 s): quanto toglie al variare di soglia e quantita'
        Biquad bp; bp.set (Biquad::BPASS, 7000, 0, 1.5, sr);
        for (float sl : { 0.3f, 1.0f })
        for (float thr : { 6.0f, -6.0f, -12.0f, -20.0f, -30.0f })
            for (float amt : { 0.4f, 1.0f })
            {
                auto p = blank(); setP (*p, "mode", 1); setP (*p, "engine", 1); setP (*p, "ds_on", 1); setP (*p, "ds_thr", thr); setP (*p, "ds_amt", amt); p->rackFromOn();
                std::vector<float> a, b; float grMax = 0; juce::Random r3 (2);
                juce::AudioBuffer<float> buf (2, 512); juce::MidiBuffer mb; double eIn = 0, eOut = 0;
                for (int blk = 0; blk < 300; ++blk)
                {
                    std::vector<float> sib (512);
                    for (int i = 0; i < 512; ++i) { const int k = blk * 512 + i; float l, r; beat (k, l, r); const double ts = std::fmod (k / sr, 0.75); const float s2 = ts < 0.08 ? bp ((r3.nextFloat() * 2 - 1)) * sl : bp (0.0f); sib[(size_t) i] = s2; buf.setSample (0, i, l * 0.6f + s2); buf.setSample (1, i, r * 0.6f + s2); }
                    p->processBlock (buf, mb); grMax = std::max (grMax, p->dsGr.load());
                }
                std::printf ("s %.1f  soglia %+5.0f  quantita' %.1f : riduzione max %.1f dB  (livello relativo misurato %.1f dB)\n", sl, thr, amt, grMax, p->dsRel.load());
            }
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "undo")
    {
        auto p = blank(); p->undoTrack (false);
        setP (*p, "e_loud", 5); p->undoTrack (true); setP (*p, "e_low", 2); p->undoTrack (true); setP (*p, "e_low", 1); p->undoTrack (false);
        std::printf ("passi %d  loud %.1f low %.1f\n", (int) p->undoStack.size(), p->value ("e_loud"), p->value ("e_low"));
        p->undo(); std::printf ("annulla: loud %.1f low %.1f\n", p->value ("e_loud"), p->value ("e_low"));
        p->undo(); std::printf ("annulla: loud %.1f low %.1f\n", p->value ("e_loud"), p->value ("e_low"));
        p->redo(); std::printf ("rifai:   loud %.1f low %.1f\n", p->value ("e_loud"), p->value ("e_low"));
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "cpu")
    {
        for (int i = 0; i < (int) presets().size(); i += 3)
        {
            auto p = blank(); p->loadPreset (i); std::vector<float> a, b;
            const double t0 = juce::Time::getMillisecondCounterHiRes(); run (*p, beat, (int) sr * 10, a, b);
            std::printf ("%-44s %.1f%%\n", presets()[(size_t) i].name, (juce::Time::getMillisecondCounterHiRes() - t0) / 100.0);
        }
        auto p = blank(); setP (*p, "mode", 1); setP (*p, "engine", 1); setP (*p, "sp_on", 1); setP (*p, "sp_res", 0.5f); setP (*p, "sp_clar", 0.4f); std::vector<float> a, b;
        const double t0 = juce::Time::getMillisecondCounterHiRes(); run (*p, beat, (int) sr * 10, a, b);
        std::printf ("solo SPETTRALE %.1f%%\n", (juce::Time::getMillisecondCounterHiRes() - t0) / 100.0);
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "neutral")
    {
        for (int v = 0; v < 4; ++v) for (float amp : { 0.05f, 0.5f })
        {
            auto p = blank(); if (v == 1) setP (*p, "engine", 1); if (v == 2) { setP (*p, "mode", 1); setP (*p, "engine", 1); } if (v == 3) setP (*p, "e_loud", 0);
            std::vector<float> a, b, in; juce::Random r2 (3);
            run (*p, [&] (int k, float& l, float& r) { const float x = amp * (float) (std::sin (k * 0.0577) * 0.6 + (r2.nextFloat() - 0.5) * 0.4); in.push_back (x); l = x; r = x * 0.92f; }, 96000, a, b);
            const int L = p->getLatencySamples(); double e = 0, ee = 0; for (int i = 48000; i < 96000; ++i) { e += std::pow (a[(size_t) i] - in[(size_t) (i - L)], 2.0); ee += in[(size_t) i] * in[(size_t) i]; }
            std::printf ("caso %d ampiezza %.2f: differenza %.1f dB (lat %d) e_loud %.2f\n", v, amp, 10 * std::log10 (e / ee + 1e-30), L, p->value ("e_loud"));
        }
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "tapealias")
    {
        for (float amp : { 0.25f, 0.5f, 0.9f }) for (float drv : { 0.3f, 1.0f })
        {
            TapeMachine tm; tm.prepare (sr); TapeSettings ts; ts.model = 1; ts.drive = drv; ts.bias = 0.1f; ts.mix = 0.8f;
            const int N = 65536; std::vector<float> l ((size_t) N * 2), r ((size_t) N * 2); const double ff = 8603 * sr / 65536;
            for (int i = 0; i < N * 2; ++i) l[(size_t) i] = r[(size_t) i] = amp * (float) std::sin (2 * 3.14159265358979 * ff * i / sr);
            for (int o = 0; o < N * 2; o += 512) tm.process (l.data() + o, r.data() + o, 512, ts);
            juce::dsp::FFT fft (16); std::vector<float> a ((size_t) 2 * N, 0); for (int i = 0; i < N; ++i) a[(size_t) i] = l[(size_t) (N + i)] * (0.5f - 0.5f * std::cos (6.2831853f * i / N));
            fft.performFrequencyOnlyForwardTransform (a.data()); const int k0 = 8603; float ref0 = a[(size_t) k0]; float worst = 0; double wf = 0;
            for (int k = 40; k < N / 2; ++k) { bool h = false; for (int m = 1; m < 5; ++m) if (std::abs (k - k0 * m) <= 6) h = true; if (! h && a[(size_t) k] > worst) { worst = a[(size_t) k]; wf = k * sr / N; } }
            std::printf ("amp %.2f drive %.1f: spur %.1f dB a %.0f Hz\n", amp, drv, gToDb (worst / ref0), wf);
        }
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "dbg")
    {
        for (float ld : { 0.0f, 5.0f, 10.0f })
        {
            auto p = blank(); setP (*p, "e_loud", ld); setP (*p, "lm_ceil", -1.0f); std::vector<float> a, b;
            run (*p, [&] (int k, float& l, float& r) { beat (k, l, r); l *= 0.6f; r *= 0.6f; }, (int) sr * 10, a, b);
            float pk = 0; for (size_t i = a.size() / 2; i < a.size(); ++i) pk = std::max (pk, std::abs (a[i]));
            std::printf ("loud %.0f: in S %.1f out S %.1f  picco %.2f dB  TP %.2f  eng %d\n", ld, p->inS.load(), p->lufsS.load(), gToDb (pk), measTP (a, (int) sr * 3), (int) p->value ("engine"));
        }
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "lab")
    {   // come THE_LAB: preset Genere | Boom Bap in vista FACILE, salti del 30% su ogni manopola (click > 5 kHz), estremi e ritorno, aliasing a 6,3 kHz, crosstalk
        int pi = 0; for (int i = 0; i < (int) presets().size(); ++i) if (juce::String (presets()[(size_t) i].name) == (std::getenv ("JPRESET") ? std::getenv ("JPRESET") : "Genere | Boom Bap")) pi = i;
        auto mk = [&] { auto p = blank(); p->loadPreset (pi); setP (*p, "mode", 0);
            if (std::getenv ("JEASY")) { p->loadPreset (0); setP (*p, "engine", 0); setP (*p, "e_loud", 6); setP (*p, "e_glue", 0.4f); setP (*p, "e_thick", 0.3f); setP (*p, "e_warm", 0.3f); setP (*p, "e_silk", 0.3f); setP (*p, "e_low", 1); setP (*p, "e_high", 1); setP (*p, "e_calm", 0.3f); setP (*p, "e_wide", 0.2f); setP (*p, "e_monob", 1); } if (auto* off = std::getenv ("JOFF")) for (auto& t : juce::StringArray::fromTokens (off, ",", "")) setP (*p, t.toRawUTF8(), 0); p->syncSnap(); return p; };
        const int blk = 512;
        auto tone = [&] (int k, float& l, float& r) { l = r = 0.25f * (float) std::sin (2 * 3.14159265358979 * 999.76 * k / sr); };
        auto render = [&] (Processor& p, int len, std::function<void (int)> atBlock, std::function<void (int, float&, float&)> gen)
        {
            std::vector<float> o; juce::AudioBuffer<float> b (2, blk); juce::MidiBuffer mb;
            for (int off = 0, bi = 0; off < len; off += blk, ++bi)
            {
                atBlock (off); for (int i = 0; i < blk; ++i) { float l, r; gen (off + i, l, r); b.setSample (0, i, l); b.setSample (1, i, r); }
                p.processBlock (b, mb); if (bi % 19 == 0) p.tickForTest();
                for (int i = 0; i < blk; ++i) o.push_back (b.getSample (0, i));
            }
            return o;
        };
        const char* only = std::getenv ("JONLY");
        std::vector<juce::String> ps;
        { auto p = mk(); for (auto* q : p->getParameters()) if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (q)) if (! dynamic_cast<juce::AudioParameterChoice*> (rp) && ! dynamic_cast<juce::AudioParameterBool*> (rp)) ps.push_back (rp->paramID); }
        const int T = 512 * 282, J = 512 * 188;
        auto ref = [&] { auto p = mk(); return render (*p, T, [] (int) {}, tone); }();
        float tpk = 0; for (int i = J; i < T; ++i) tpk = std::max (tpk, std::abs (ref[(size_t) i]));
        int nClick = 0, nBroken = 0, nJ = 0;
        for (auto& id : ps)
        { if (only && ! id.contains (only)) continue;
            // salto: 4 volte del 30% (su e giu') a partire da 2 s
            auto p = mk(); auto* rp = p->apvts.getParameter (id); const float v0 = rp->getValue();
            auto out = render (*p, T, [&] (int off) { for (int j = 0; j < 4; ++j) if (off == J + j * 4096) rp->setValueNotifyingHost (j % 2 == 0 ? (v0 + 0.3f <= 1 ? v0 + 0.3f : v0 - 0.3f) : v0); }, tone);
            Biquad h1, h2; h1.set (Biquad::LCUT, 5000, 0, 0.707, sr); h2.set (Biquad::LCUT, 5000, 0, 0.707, sr);
            float pk = 0, chg = 0; for (int i = J - 2048; i < T; ++i) { const float d = out[(size_t) i] - ref[(size_t) i]; chg = std::max (chg, std::abs (d)); const float y = h2 (h1 (d)); if (i >= J) pk = std::max (pk, std::abs (y)); }
            float cdb = gToDb (pk / tpk + 1e-12f);
            {   // un salto solo, confrontato con lo stesso valore tenuto dall'inizio: resta solo il transitorio del salto
                const float v1 = v0 + 0.3f <= 1 ? v0 + 0.3f : v0 - 0.3f;
                auto pa = mk(); auto* ra = pa->apvts.getParameter (id); auto oa = render (*pa, J + 8192, [&] (int off) { if (off == J) ra->setValueNotifyingHost (v1); }, tone);
                auto pb = mk(); pb->apvts.getParameter (id)->setValueNotifyingHost (v1); auto ob = render (*pb, J + 8192, [] (int) {}, tone);
                Biquad g1, g2; g1.set (Biquad::LCUT, 5000, 0, 0.707, sr); g2.set (Biquad::LCUT, 5000, 0, 0.707, sr); float pk2 = 0;
                for (int i = J - 1024; i < J + 8192; ++i) { const float y = g2 (g1 (oa[(size_t) i] - ob[(size_t) i])); if (i >= J && i < J + 2400) pk2 = std::max (pk2, std::abs (y)); }
                cdb = gToDb (pk2 / tpk + 1e-12f);
            }
            if (gToDb (chg / tpk + 1e-12f) < -80) continue;   // non cambia niente: non giudicato
            ++nJ; const bool click = cdb > -35;
            // estremi: 0 per 0,5 s, 1 per 0,5 s, poi di nuovo il valore di partenza; rumore + tono
            auto p2 = mk(); auto* rq = p2->apvts.getParameter (id); juce::Random rn (5);
            auto gen = [&] (int k, float& l, float& r) { tone (k, l, r); const float nz = (rn.nextFloat() - 0.5f) * 0.1f; l += nz; r += nz; };
            juce::Random rn0 (5); auto gen0 = [&] (int k, float& l, float& r) { tone (k, l, r); const float nz = (rn0.nextFloat() - 0.5f) * 0.1f; l += nz; r += nz; };
            const int E = std::getenv ("JLONG") ? 512 * 1500 : 512 * 563;
            auto o2 = render (*p2, E, [&] (int off) { if (off == 512 * 94) rq->setValueNotifyingHost (0); if (off == 512 * 141) rq->setValueNotifyingHost (1); if (off == 512 * 188) rq->setValueNotifyingHost (v0); }, gen);
            auto p3 = mk(); auto o3 = render (*p3, E, [] (int) {}, gen0);
            bool nan = false; float mx2 = 0; double ed = 0, er = 0;
            for (int i = 0; i < E; ++i) { if (! std::isfinite (o2[(size_t) i])) nan = true; mx2 = std::max (mx2, std::abs (o2[(size_t) i])); if (i > E - (int) sr) { ed += std::pow (o2[(size_t) i] - o3[(size_t) i], 2.0); er += o3[(size_t) i] * o3[(size_t) i]; } }
            if (only) { for (int w = 4; w < E / 24000; ++w) { double a2 = 0, b2 = 0; for (int i = w * 24000; i < (w + 1) * 24000; ++i) { a2 += std::pow (o2[(size_t) i] - o3[(size_t) i], 2.0); b2 += o3[(size_t) i] * o3[(size_t) i]; } std::printf ("%.1fs %.1f  ", w * 0.5, 10 * std::log10 (a2 / b2 + 1e-30)); } std::printf ("\n"); }
            const float back = (float) (10 * std::log10 (ed / er + 1e-30)); const bool broken = nan || back > -60;
            if (click) ++nClick; if (broken) ++nBroken;
            if (click || broken || only) std::printf ("%-22s click %6.1f dB %s  estremi: max %+5.1f dBFS, dopo il ritorno %6.1f dB %s%s  motore %d\n", id.toRawUTF8(), cdb, click ? "CLICK" : "     ", gToDb (mx2), back, broken ? "ROTTO" : "", nan ? " NaN" : "", (int) p2->value ("engine"));
        }
        std::printf ("giudicati %d: click %d, rotti %d\n", nJ, nClick, nBroken);
        // aliasing con toni acuti e crosstalk
        for (float amp : { 0.5f, 0.9f }) for (double f0 : { 6300.0, 10000.0 })
        {
            const double fq = (double) (((int) std::round (f0 * 65536 / sr)) | 1); const double ff = fq * sr / 65536;
            auto p = mk(); auto o = render (*p, (int) sr * 4, [] (int) {}, [&] (int k, float& l, float& r) { l = r = amp * (float) std::sin (2 * 3.14159265358979 * ff * k / sr); });
            const int N = 65536; juce::dsp::FFT fft (16); std::vector<float> a ((size_t) 2 * N, 0);
            for (int i = 0; i < N; ++i) a[(size_t) i] = o[o.size() - N + (size_t) i] * (0.5f - 0.5f * std::cos (6.2831853f * i / N));
            fft.performFrequencyOnlyForwardTransform (a.data());
            const int k0 = (int) fq; float ref0 = 0; for (int k = k0 - 4; k <= k0 + 4; ++k) ref0 = std::max (ref0, a[(size_t) k]);
            float worst = 0; double wf = 0;
            for (int k = 40; k < N / 2; ++k) { bool harm = false; for (int h = 1; h < 10; ++h) if (std::abs (k - k0 * h) <= 6) harm = true; if (std::abs (k - k0) < k0 / 20) harm = true; if (! harm && a[(size_t) k] > worst) { worst = a[(size_t) k]; wf = k * sr / N; } }
            std::printf ("amp %.1f tono %.0f Hz: spur peggiore %.1f dB a %.0f Hz\n", amp, ff, gToDb (worst / ref0), wf);
        }
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "diag")
    {   // preset JPRESET (JOFF = moduli da spegnere): DC su tono 1 kHz a -6 dBFS, latenza vera (correlazione) contro dichiarata, risposta a 48 e 96 kHz
        int pi = 0; for (int i = 0; i < (int) presets().size(); ++i) if (juce::String (presets()[(size_t) i].name) == (std::getenv ("JPRESET") ? std::getenv ("JPRESET") : "Init")) pi = i;
        auto mk = [&] (double rate) { auto p = std::make_unique<Processor>(); p->setPlayConfigDetails (2, 2, rate, 512); p->prepareToPlay (rate, 512); p->loadPreset (pi); setP (*p, "mode", 0);
            if (auto* off = std::getenv ("JOFF")) for (auto& t : juce::StringArray::fromTokens (off, ",", "")) setP (*p, t.toRawUTF8(), 0);
            if (auto* js = std::getenv ("JSET")) for (auto kv : juce::StringArray::fromTokens (js, ",", "")) setP (*p, kv.upToFirstOccurrenceOf ("=", false, false).toRawUTF8(), kv.fromFirstOccurrenceOf ("=", false, false).getFloatValue());
            p->syncSnap(); p->prepareToPlay (rate, 512); return p; };
        {
            auto p = mk (sr); std::vector<float> oL, oR;
            run (*p, [&] (int k, float& l, float& r) { l = r = 0.5f * (float) std::sin (2 * 3.14159265358979 * 999.76 * k / sr); }, (int) sr * 5, oL, oR);
            double dc = 0; for (size_t i = oL.size() - 48000; i < oL.size(); ++i) dc += oL[i]; dc /= 48000;
            std::printf ("DC su tono -6 dBFS: %.1f dBFS\n", gToDb ((float) std::abs (dc) + 1e-12f));
        }
        {
            auto p = mk (sr); const int rep = p->getLatencySamples(); std::vector<float> in, oL, oR; juce::Random rr (9);
            const int N = (int) sr * 4; in.resize ((size_t) N); for (auto& x : in) x = 0.02f * (rr.nextFloat() * 2 - 1);
            run (*p, [&] (int k, float& l, float& r) { l = r = in[(size_t) k]; }, N, oL, oR);
            double best = -1e30; int bl = 0; std::vector<double> cc (1200);
            for (int l = 0; l < 1200; ++l) { double c = 0; for (int i = 48000; i < N - 1200; ++i) c += (double) in[(size_t) i] * oL[(size_t) (i + l)]; cc[(size_t) l] = c; if (c > best) { best = c; bl = l; } }
            double fr = bl; if (bl > 0 && bl < 1199) { const double a = cc[(size_t) bl - 1], b = cc[(size_t) bl], c = cc[(size_t) bl + 1]; fr = bl + 0.5 * (a - c) / (a - 2 * b + c); }
            std::printf ("latenza dichiarata %d, misurata %.2f\n", rep, fr);
        }
        for (double f : { 100.0, 1000.0, 4000.0, 7680.0, 10000.0 })
        {
            float g[2];
            for (int j = 0; j < 2; ++j)
            {
                const double rate = j ? 96000.0 : 48000.0; auto p = mk (rate); std::vector<float> oL, oR;
                run (*p, [&] (int k, float& l, float& r) { l = r = 0.05f * (float) std::sin (2 * 3.14159265358979 * f * k / rate); }, (int) rate * 4, oL, oR);
                double e = 0; for (size_t i = oL.size() - (size_t) rate; i < oL.size(); ++i) e += (double) oL[i] * oL[i];
                g[j] = gToDb ((float) std::sqrt (e / rate) / (0.05f / std::sqrt (2.0f)));
            }
            std::printf ("%6.0f Hz: 48k %+6.2f dB  96k %+6.2f dB  diff %+5.2f\n", f, g[0], g[1], g[1] - g[0]);
        }
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "ear")
    {   // prova d'ascolto: due mix diversi, proposta, applicazione, taratura
        for (int kind = 0; kind < 3; ++kind)
        {
            auto p = blank(); setP (*p, "mode", 1); setP (*p, "engine", 1); setP (*p, "ear_genre", kind == 0 ? 0 : kind == 1 ? 6 : 3); setP (*p, "target", kind == 0 ? 5 : 0);
            Biquad sibF; sibF.set (Biquad::BPASS, 7500, 0, 2.0, sr);
            std::vector<float> a, b; juce::Random rr (kind + 1); double ph = 0, ph2 = 0;
            auto gen = [&] (int k, float& l, float& r) { beat (k, l, r); l *= kind == 0 ? 0.35f : 0.2f; ph += 2 * 3.14159 * 330 / sr; ph2 += 2 * 3.14159 * 52 / sr;
                const float pad = (kind == 0 ? 0.04f : 0.12f) * (float) std::sin (ph), sub = (kind == 0 ? 0.18f : 0.04f) * (float) std::sin (ph2);
                r = l + pad * 0.6f + sub * (kind == 0 ? 0.6f : 1.0f); l = l + pad + sub;
                if (kind == 2) { const double ts = std::fmod (k / sr, 0.75); const float sb = ts < 0.07 ? sibF ((rr.nextFloat() * 2 - 1)) * 1.4f : sibF (0.0f); l += sb; r += sb; } };
            setP (*p, "ear_dur", 0); p->earStart(); int guard = 0; while (p->earCap.on.load() && guard++ < 200) run (*p, gen, (int) sr / 2, a, b);
            p->earFinish(); const auto& E = p->ear;
            std::printf ("\n==== mix %d  valid %d  ref %d  sib %.1f res %.1f@%.0f\n", kind, (int) E.valid, (int) E.useRef, E.sib, E.resMax, E.resHz);
            for (auto& h : E.heard) std::printf ("  . %s\n", h.toRawUTF8());
            for (auto& st : E.steps) std::printf ("  > [%s%s] %s\n      %s\n", prioName (st.prio), st.sel ? "" : " off", st.title.toRawUTF8(), st.why.toRawUTF8());
            for (auto& t : E.notes) std::printf ("  ! %s\n", t.toRawUTF8());
            p->earApply(); a.clear(); b.clear();
            for (int t = 0; t < 40 && p->earCalib >= 0; ++t) { run (*p, gen, (int) sr / 2, a, b); p->earTick(); }
            float pk = 0; bool nan = false; for (auto x : a) { if (! std::isfinite (x)) nan = true; pk = std::max (pk, std::abs (x)); }
            std::printf ("  = %s   (picco %.2f nan %d)\n", p->earMsg.toRawUTF8(), pk, (int) nan);
            if (kind == 0 && argc > 2)
            {
                juce::ScopedJuceInitialiser_GUI gg; Editor ed (*p); ed.tickForTest(); ed.openEffect (TOOL_EAR); ed.tickForTest();
                auto img = ed.createComponentSnapshot (ed.getLocalBounds(), true, 1.0f);
                juce::File f (argv[2]); f.deleteFile(); juce::FileOutputStream os (f); juce::PNGImageFormat().writeImageToStream (img, os);
            }
            p->earDiscard(); std::printf ("  scarta -> mx_on %.0f bc_on %.0f\n", p->value ("mx_on"), p->value ("bc_on"));
        }
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "newmods")
    {   // IMPACT, SPETTRALE, STABILIZER: suono (niente NaN, livello sensato) e pagina
        for (int m : { (int) IMPACT, (int) SPECTRAL, (int) STABIL })
        {
            auto p = blank(); setP (*p, "mode", 1); setP (*p, "engine", 1);
            const char* on = m == IMPACT ? "ip_on" : m == SPECTRAL ? "sp_on" : "st_on"; setP (*p, on, 1); p->inRack[m] = true;
            if (m == IMPACT) { setP (*p, "ip_b1", 0.8f); setP (*p, "ip_b3", -0.6f); }
            if (m == SPECTRAL) { setP (*p, "sp_res", 0.8f); setP (*p, "sp_clar", 0.5f); }
            if (m == STABIL) { setP (*p, "st_amt", 1); setP (*p, "st_speed", 1); }
            std::vector<float> a, b; double ph = 0;
            run (*p, [&] (int k, float& l, float& r) { beat (k, l, r); ph += 2 * 3.14159 * 2400 / sr; l = l * 0.5f + 0.15f * (float) std::sin (ph); r = l; }, (int) sr * 8, a, b);
            float pk = 0; bool nan = false; for (auto x : a) { if (! std::isfinite (x)) nan = true; pk = std::max (pk, std::abs (x)); }
            std::printf ("%-10s picco %.2f nan %d  ui:", modInfo (m).name, pk, (int) nan);
            if (m == IMPACT) for (int k = 0; k < 4; ++k) std::printf (" %.1f", p->ipUi[k].load());
            if (m == SPECTRAL) { float mn = 0; int bm = 0; for (int k = 0; k < Spectral::kB; ++k) if (p->spUi[k].load() < mn) { mn = p->spUi[k].load(); bm = k; } std::printf (" min %.1f dB alla banda %d", mn, bm); }
            if (m == STABIL) for (int k = 0; k < kRegions; ++k) std::printf (" %.1f/%.1f", p->stUi[k].load(), p->stDiff[k].load());
            std::printf ("\n");
            if (argc > 2)
            {
                juce::ScopedJuceInitialiser_GUI gg; Editor ed (*p); ed.tickForTest(); ed.openEffect (m); ed.tickForTest();
                auto img = ed.createComponentSnapshot (ed.getLocalBounds(), true, 1.0f);
                juce::File f (juce::String (argv[2]) + juce::String (m) + ".png"); f.deleteFile(); juce::FileOutputStream os (f); juce::PNGImageFormat().writeImageToStream (img, os);
            }
        }
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "easysnap")
    {
        static FakeHead fh2; if (std::getenv ("JBPM")) gHead = &fh2;
        auto p = blank(); if (gHead) p->setPlayHead (gHead); p->loadPreset (1 + (std::getenv ("JPR") ? std::atoi (std::getenv ("JPR")) : 0));
        if (auto* v = std::getenv ("JTHEME")) p->uiTheme = std::atoi (v);
        if (auto* v = std::getenv ("JSCAN")) p->uiScan = std::atoi (v);
        if (auto* v = std::getenv ("JWIN")) p->uiWin = std::atoi (v);
        if (auto* v = std::getenv ("JSPEC")) p->uiSpec = std::atoi (v);
        std::vector<float> a, b; run (*p, [&] (int k, float& l, float& r) { beat (k, l, r); l *= 0.6f; r *= 0.6f; if (std::getenv ("JSTEREO")) { const float pad = 0.2f * (float) std::sin (k * 0.031) ; l += pad + (rnd.nextFloat() - 0.5f) * 0.15f; r += -0.5f * pad + (rnd.nextFloat() - 0.5f) * 0.15f; } }, (int) sr * 22, a, b);
        Editor ed (*p);
        if (auto* v = std::getenv ("JVIEW")) ed.easyPage.view = std::atoi (v);
        for (int i = 0; i < 30; ++i) ed.tickForTest();
        if (auto* jsc = std::getenv ("JSCALE")) { ed.scaler.set ((float) std::atof (jsc)); ed.tickForTest(); }
        if (std::getenv ("JADV")) { setP (*p, "mode", 1); ed.tickForTest(); }
        if (std::getenv ("JBACK")) { setP (*p, "mode", 1); ed.tickForTest(); setP (*p, "mode", 0); ed.tickForTest(); }
        if (std::getenv ("JSTAGE"))
        {   { for (auto& x : gStageT) x = 0; }   // costo di un fotogramma dell'onda: il display si ridisegna insieme a quello che ha dietro
            auto& st = ed.easyPage.stage; const auto r = ed.getLocalArea (&st, st.getLocalBounds());
            juce::Image im (juce::Image::ARGB, ed.getWidth() * 2, ed.getHeight() * 2, true);
            const double t0 = juce::Time::getMillisecondCounterHiRes();
            for (int i = 0; i < (std::getenv ("JN") ? std::atoi (std::getenv ("JN")) : 40); ++i) { juce::Graphics g (im); g.addTransform (juce::AffineTransform::scale (2.0f)); g.reduceClipRegion (r); ed.paintEntireComponent (g, true); }
            { const double tf = juce::Time::getMillisecondCounterHiRes(); for (int k = 0; k < 5; ++k) std::printf ("  parte %d: %.2f ms\n", k, gStageT[k] / 40); std::printf ("  totale %.2f\n", (tf - t0) / 40); }
            { const double t1 = juce::Time::getMillisecondCounterHiRes();
              for (int i = 0; i < 40; ++i) { juce::Graphics g (im); g.addTransform (juce::AffineTransform::scale (2.0f)); st.paintEntireComponent (g, true); }
              std::printf ("  solo display %.2f ms\n", (juce::Time::getMillisecondCounterHiRes() - t1) / 40.0);
              for (int v = 0; v < 5; ++v) { ed.easyPage.view = v; const double t2 = juce::Time::getMillisecondCounterHiRes();
                for (int i = 0; i < 20; ++i) { juce::Graphics g (im); g.addTransform (juce::AffineTransform::scale (2.0f)); st.paintEntireComponent (g, true); }
                std::printf ("  vista %d: %.2f ms\n", v, (juce::Time::getMillisecondCounterHiRes() - t2) / 20.0); } ed.easyPage.view = 0; }
            for (auto* c : { (juce::Component*) ed.easyPage.trim, (juce::Component*) &ed.easyPage.input, (juce::Component*) ed.easyPage.loud })
            { const double t3 = juce::Time::getMillisecondCounterHiRes();
              for (int i = 0; i < 20; ++i) { juce::Graphics g (im); g.addTransform (juce::AffineTransform::scale (2.0f)); c->paintEntireComponent (g, true); }
              std::printf ("  %s %.2f ms vis %d\n", typeid (*c).name(), (juce::Time::getMillisecondCounterHiRes() - t3) / 20.0, (int) c->isShowing()); }
            { const double t4 = juce::Time::getMillisecondCounterHiRes();
              for (int i = 0; i < 20; ++i) { juce::Graphics g (im); g.addTransform (juce::AffineTransform::scale (2.0f)); g.reduceClipRegion (r); ed.paint (g); }
              std::printf ("  Editor::paint %.2f ms\n", (juce::Time::getMillisecondCounterHiRes() - t4) / 20.0);
              if (ed.easyPage.isVisible()) { const double t5 = juce::Time::getMillisecondCounterHiRes(); auto r2 = ed.easyPage.getLocalArea (&ed, r);
                for (int i = 0; i < 20; ++i) { juce::Graphics g (im); g.addTransform (juce::AffineTransform::scale (2.0f)); g.reduceClipRegion (r2); ed.easyPage.paint (g); }
                std::printf ("  EasyPage::paint %.2f ms\n", (juce::Time::getMillisecondCounterHiRes() - t5) / 20.0); } }
            std::function<void (juce::Component&, int)> walk = [&] (juce::Component& par, int depth)
            {
                for (auto* c : par.getChildren()) if (c->isVisible() && ed.getLocalArea (c, c->getLocalBounds()).intersects (r))
                { const double t6 = juce::Time::getMillisecondCounterHiRes();
                  for (int i = 0; i < 10; ++i) { juce::Graphics g (im); g.addTransform (juce::AffineTransform::scale (2.0f)); c->paintEntireComponent (g, true); }
                  std::printf ("  %*s%s %.2f ms\n", depth * 2, "", typeid (*c).name(), (juce::Time::getMillisecondCounterHiRes() - t6) / 10.0);
                  if (depth < 1) walk (*c, depth + 1); }
            };
            walk (ed, 0);
            std::printf ("scala editor %.3f\n", ed.scaler.s); std::printf ("fotogramma onda (%s, %dx%d): %.2f ms\n", p->easy() ? "FACILE" : "AVANZATO", r.getWidth(), r.getHeight(), (juce::Time::getMillisecondCounterHiRes() - t0) / 40.0);
            return 0;
        }
        if (auto* gd = std::getenv ("JGUIDE")) { ed.easyPage.guide.topic = std::atoi (gd); ed.easyPage.guide.setVisible (true); ed.easyPage.guide.toFront (false); }
        if (std::getenv ("JPERF"))
        {
            const double t0 = juce::Time::getMillisecondCounterHiRes();
            for (int i = 0; i < 20; ++i) { ed.tickForTest(); auto im = ed.createComponentSnapshot (ed.getLocalBounds(), true, 2.0f); }
            std::printf ("frame completo @2x: %.1f ms\n", (juce::Time::getMillisecondCounterHiRes() - t0) / 20.0);
            for (auto* c : ed.getChildren()) if (c->isVisible())
            {
                const double t2 = juce::Time::getMillisecondCounterHiRes();
                for (int i = 0; i < 10; ++i) { ed.tickForTest(); auto im = c->createComponentSnapshot (c->getLocalBounds(), true, 2.0f); }
                std::printf ("  %-28s %5.1f ms  (%dx%d)\n", typeid (*c).name(), (juce::Time::getMillisecondCounterHiRes() - t2) / 10.0, c->getWidth(), c->getHeight());
            }
            const double t1 = juce::Time::getMillisecondCounterHiRes();
            for (int i = 0; i < 20; ++i) { ed.tickForTest(); auto im = ed.easyPage.stage.createComponentSnapshot (ed.easyPage.stage.getLocalBounds(), true, 2.0f); }
            std::printf ("solo onda @2x: %.1f ms\n", (juce::Time::getMillisecondCounterHiRes() - t1) / 20.0);
            for (int v = 0; v < kEasyViews; ++v) { ed.easyPage.view = v; const double t3 = juce::Time::getMillisecondCounterHiRes();
              for (int i = 0; i < 20; ++i) { auto im = ed.easyPage.stage.createComponentSnapshot (ed.easyPage.stage.getLocalBounds(), true, 2.0f); }
              std::printf ("  vista %d @2x: %.1f ms\n", v, (juce::Time::getMillisecondCounterHiRes() - t3) / 20.0); }
        }
        auto img = ed.createComponentSnapshot (ed.getLocalBounds(), true, 1.0f);
        juce::File f (argc > 2 ? argv[2] : "/tmp/master.png"); f.deleteFile(); juce::FileOutputStream os (f); juce::PNGImageFormat().writeImageToStream (img, os);
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "ideal")
    {   // rumore con pendenza -4.5 dB/ottava (= "piatto" per il bilanciamento): tutte le regioni devono venire vicino a 0
        for (float slope : { -4.5f, -3.0f, -6.0f })
        {
            ThirdAnalyzer an; juce::dsp::FFT fft (12); std::vector<float> buf (8192); juce::Random rr (3);
            for (int fr = 0; fr < 200; ++fr)
            {
                std::vector<std::complex<float>> X (4096), x (4096);
                for (int k = 1; k < 2048; ++k) { const float f = k * 48000.0f / 4096, mag = std::pow (f / 1000.0f, slope / 20.0f / std::log10 (2.0f)); const float ph = rr.nextFloat() * 6.2832f; X[(size_t) k] = std::polar (mag, ph); X[(size_t) (4096 - k)] = std::conj (X[(size_t) k]); }
                fft.perform (X.data(), x.data(), true);
                std::vector<float> mono (4096); for (int i = 0; i < 4096; ++i) mono[(size_t) i] = x[(size_t) i].real();
                an.addFrame (mono.data(), 48000);
            }
            float th[kThirds], reg[kRegions]; an.result (th); ThirdAnalyzer::regions (th, reg);
            std::printf ("pendenza %+.1f dB/ott:", slope); for (int k = 0; k < kRegions; ++k) std::printf ("  %s %+.1f", regionName (k), reg[k]); std::printf ("\n");
        }
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "pv")
    {
        auto sine = [&] (float hz, float amp) { return [=] (int i, float& l, float& r) { l = r = amp * (float) std::sin (2 * 3.14159265 * hz * i / sr); }; };
        auto lvl = [] (const std::vector<float>& x, int from) { double s = 0; for (size_t i = (size_t) from; i < x.size(); ++i) s += x[i] * x[i]; return gToDb ((float) std::sqrt (s / (x.size() - from))); };
        auto base = [&] { rnd.setSeed (7); auto p = blank(); setP (*p, "mode", 1); return p; };
        // neutralita': pultec acceso a zero, vari-mu acceso con soglia alta
        { auto p = base(); setP (*p, "pq_on", 1); std::vector<float> a, b, c, d; run (*p, beat, (int) sr * 2, a, b);
          rnd.setSeed (7); auto q = base(); run (*q, beat, (int) sr * 2, c, d); float mx = 0; for (size_t i = 0; i < a.size(); ++i) mx = juce::jmax (mx, std::abs (a[i] - c[i]));
          std::printf ("pultec a zero: diff max %.2e\n", mx); }
        for (auto [id, v, hz] : std::vector<std::tuple<const char*, float, float>> { { "pq_lb", 10, 60 }, { "pq_lb", 10, 250 }, { "pq_la", 10, 60 }, { "pq_la", 10, 200 }, { "pq_hb", 10, 10000 }, { "pq_ha", 10, 15000 } })
        {
            auto p = base(); setP (*p, "pq_on", 1); setP (*p, id, v); std::vector<float> a, b; run (*p, sine (hz, 0.1f), (int) sr, a, b);
            std::printf ("%s=%g @%gHz: %+.1f dB\n", id, v, hz, lvl (a, (int) sr / 2) - gToDb (0.1f / std::sqrt (2.0f)));
        }
        { auto p = base(); setP (*p, "pq_on", 1); setP (*p, "pq_lb", 10); setP (*p, "pq_la", 10);
          for (float hz : { 30.f, 60.f, 120.f, 300.f, 1000.f }) { auto q = base(); setP (*q, "pq_on", 1); setP (*q, "pq_lb", 6); setP (*q, "pq_la", 4); std::vector<float> a, b; run (*q, sine (hz, 0.1f), (int) sr, a, b); std::printf ("trucco lb6 la4 @%gHz: %+.1f dB\n", hz, lvl (a, (int) sr / 2) - gToDb (0.1f / std::sqrt (2.0f))); } }
        { auto p = base(); setP (*p, "pq_on", 1); setP (*p, "pq_tube", 1); std::vector<float> a, b; run (*p, sine (1000, 0.5f), (int) sr * 2, a, b); std::printf ("pultec valvola 100%% THD %.2f%%  livello %+.2f dB\n", thd (a, (int) sr, sr, 1000), lvl (a, (int) sr) - gToDb (0.5f / std::sqrt (2.0f))); }
        { auto p = base(); setP (*p, "vm_on", 1); setP (*p, "vm_thr", 0); setP (*p, "vm_tube", 0); setP (*p, "vm_auto", 0); std::vector<float> a, b, c, d; run (*p, sine (1000, 0.05f), (int) sr * 2, a, b);
          std::printf ("varimu sotto soglia: %+.3f dB  gr %.2f\n", lvl (a, (int) sr) - gToDb (0.05f / std::sqrt (2.0f)), p->vmGr.load()); }
        for (float tb : { 0.3f, 1.0f }) { auto p = base(); setP (*p, "vm_on", 1); setP (*p, "vm_thr", 0); setP (*p, "vm_tube", tb); setP (*p, "vm_auto", 0); std::vector<float> a, b; run (*p, sine (1000, 0.5f), (int) sr * 2, a, b); std::printf ("varimu valvola %.0f%% THD %.2f%%\n", tb * 100, thd (a, (int) sr, sr, 1000)); }
        for (float thr : { -12.f, -18.f, -24.f, -30.f }) for (int mode : { 0, 1 })
        {
            auto p = base(); setP (*p, "vm_on", 1); setP (*p, "vm_thr", thr); setP (*p, "vm_mode", (float) mode); setP (*p, "vm_auto", 0);
            std::vector<float> a, b; run (*p, beat, (int) sr * 6, a, b); float mx = 0, mn = 99; double s = 0; int k = 0;
            std::vector<float> c, d; rnd.setSeed (7); auto q = base(); run (*q, beat, (int) sr * 6, c, d);
            std::printf ("varimu thr %g mode %d: in %.1f out %.1f dB  gr ora %.2f  TP %.1f\n", thr, mode, lvl (c, (int) sr * 3), lvl (a, (int) sr * 3), p->vmGr.load(), measTP (a, (int) sr * 3));
            (void) mx; (void) mn; (void) s; (void) k;
        }
        { auto p = base(); setP (*p, "vm_on", 1); setP (*p, "vm_thr", -24); setP (*p, "vm_auto", 1); std::vector<float> a, b, c, d; run (*p, beat, (int) sr * 8, a, b); rnd.setSeed (7); auto q = base(); run (*q, beat, (int) sr * 8, c, d);
          std::printf ("varimu auto makeup: in %.1f out %.1f\n", lvl (c, (int) sr * 4), lvl (a, (int) sr * 4)); }
        { auto p = base(); setP (*p, "vm_on", 1); setP (*p, "vm_thr", -24); setP (*p, "vm_delta", 1); std::vector<float> a, b; run (*p, beat, (int) sr * 4, a, b); std::printf ("varimu delta: %.1f dB\n", lvl (a, (int) sr * 2)); }
        { auto p = base(); setP (*p, "vm_on", 1); setP (*p, "vm_thr", -30); setP (*p, "vm_ms", 1); setP (*p, "vm_att", 0); std::vector<float> a, b; run (*p, beat, (int) sr * 4, a, b);
          bool ok = true; for (auto v : a) if (! std::isfinite (v) || std::abs (v) > 4) ok = false; std::printf ("varimu ms/att1: %s\n", ok ? "ok" : "PROBLEMA"); }
        for (int blk : { 37, 512, 4096 }) { auto p = base(); std::vector<float> a, b; run (*p, beat, (int) sr * 3, a, b, blk); std::printf ("blocco %d: colonne %d (attese %d)\n", blk, p->hTot.load(), (int) (3 / 0.025)); }
        // ordine: progetto vecchio senza i nuovi moduli
        { auto p = base(); auto st = p->apvts.copyState(); std::printf ("ordine default:"); for (int k = 0; k < kNumMods; ++k) std::printf (" %s", modInfo (p->order[k]).name); std::printf ("\n"); }
        { auto p = base(); juce::MemoryBlock mb; p->getStateInformation (mb); auto x = juce::AudioProcessor::getXmlFromBinary (mb.getData(), (int) mb.getSize());
          x->setAttribute ("order", "1,0,10,2,3,4,5,6,7,8,9"); juce::MemoryBlock m2; juce::AudioProcessor::copyXmlToBinary (*x, m2);
          auto q = base(); q->setStateInformation (m2.getData(), (int) m2.getSize()); std::printf ("ordine migrato:"); for (int k = 0; k < kNumMods; ++k) std::printf (" %s", modInfo (q->order[k]).name); std::printf ("\n"); }
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "snap")
    {

        static FakeHead fh; if (std::getenv ("JBPM")) { gHead = &fh; }
        auto p = fresh(); setP (*p, "mode", 1); setP (*p, "mx_gain", 8); if (gHead) p->setPlayHead (gHead); setP (*p, "mx_style", PUNCH); setP (*p, "mx_clip", 0.4f);
        setP (*p, "bc_on", 1); setP (*p, "bc_punch", 0.4f); setP (*p, "bc_mono", 110); setP (*p, "bc_harm", 0.3f); setP (*p, "bc_thr", -12);
        setP (*p, "eq_on", 1); setP (*p, eqId (2, "g"), -3); setP (*p, eqId (4, "dyn"), 1); setP (*p, eqId (4, "f"), 7000); setP (*p, eqId (4, "thr"), -40); setP (*p, eqId (5, "g"), 2.5f);
        setP (*p, "ds_on", 1); setP (*p, "ds_thr", -10); setP (*p, "bus_on", 1); setP (*p, "bus_thr", -18); setP (*p, "mb_on", 1); setP (*p, "mb3_up", 0.5f); setP (*p, "mb1_thr", -20);
        setP (*p, "im_on", 1); setP (*p, "im_w1", 0.3f); setP (*p, "im_w3", 1.4f); setP (*p, "im_w4", 1.6f); setP (*p, "im_s3", 0.3f); setP (*p, "im_s4", 0.4f);
        if (auto* v = std::getenv ("JTHEME")) p->uiTheme = std::atoi (v);
        if (std::getenv ("JMODS")) { setP (*p, "pq_on", 1); setP (*p, "pq_lb", 6); setP (*p, "pq_la", 4); setP (*p, "pq_hb", 3); setP (*p, "pq_ha", 1.5f); setP (*p, "pq_tube", 0.3f);
                                     setP (*p, "vm_on", 1); setP (*p, "vm_thr", -26); }
        if (std::getenv ("JNEW")) { setP (*p, "sp_on", 1); setP (*p, "sp_res", 0.6f); setP (*p, "sp_clar", 0.5f); setP (*p, "st_on", 1); setP (*p, "st_amt", 0.8f); setP (*p, "engine", 1); p->rackFromOn(); }
        std::vector<float> a, b; run (*p, beat, (int) sr * 12, a, b);
        Editor ed (*p);
        if (auto* jsc = std::getenv ("JSCALE")) ed.scaler.set ((float) std::atof (jsc));
        if (auto* pg = std::getenv ("JPAGE")) ed.select (std::atoi (pg));
        if (auto* av = std::getenv ("JAVIEW")) { ed.advView = std::atoi (av); ed.applyAdvView(); }
        if (std::getenv ("JGAL")) { ed.gallery.setVisible (true); ed.gallery.toFront (false); }
        if (std::getenv ("JBROWSE")) ed.browser.open();
        if (std::getenv ("JEQPOP")) { ed.eqPage.view.tick(); ed.eqPage.view.sel = 2; ed.eqPage.view.popBand = 2; auto* d = p->apvts.getParameter (eqId (2, "dyn")); d->setValueNotifyingHost (1); auto* gg = p->apvts.getParameter (eqId (2, "g")); gg->setValueNotifyingHost (gg->convertTo0to1 (-3)); ed.eqPage.view.updatePop(); }
        if (std::getenv ("JREF"))
        {
            juce::File wf ("/tmp/jref.wav"); wf.deleteFile();
            { juce::WavAudioFormat wav; std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (new juce::FileOutputStream (wf), sr, 2, 24, {}, 0));
              juce::AudioBuffer<float> bb (2, (int) sr * 8); for (int i = 0; i < bb.getNumSamples(); ++i) { float l, r; beat (i, l, r); bb.setSample (0, i, l * 1.6f); bb.setSample (1, i, r * 1.6f); }
              bb.applyGainRamp (0, bb.getNumSamples(), 1, 1); w->writeFromAudioSampleBuffer (bb, 0, bb.getNumSamples()); }
            p->loadReference (wf); juce::Thread::sleep (50); for (int i = 0; i < 400 && p->refLoading.load(); ++i) juce::Thread::sleep (10);
            p->refPos = sr * 3;
            auto rt = p->reference(); std::printf ("riferimento: %s  %.1f LUFS  %.1f dBTP\n", rt ? rt->name.toRawUTF8() : "nessuno", rt ? rt->lufs : 0.0f, rt ? rt->tp : 0.0f);
        }
        if (std::getenv ("JCLICK"))
        {
            for (int k = 0; k < 5; ++k)
            {
                auto c = ed.advTabs.r[k].getCentre() - juce::Point<float> (548, 156);
                juce::MouseEvent me (juce::Desktop::getInstance().getMainMouseSource(), c, juce::ModifierKeys(), 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, &ed.advTabsComp, &ed.advTabsComp, juce::Time::getCurrentTime(), c, juce::Time::getCurrentTime(), 1, false);
                ed.advTabsComp.mouseDown (me); for (int i = 0; i < 5; ++i) ed.tickForTest();
                std::printf ("tab %d -> advView %d stage %d pagina max %d\n", k, ed.advView, ed.advStage.isVisible(), ed.maxPage.isVisible());
            }
            ed.setVisible (true); std::printf ("scala %.2f\n", ed.scaler.s); for (int k = 0; k < 5; ++k) { auto pc = (ed.advTabs.r[k].getCentre() * ed.scaler.s).toInt(); auto* h = ed.getComponentAt (pc); std::printf ("  scheda %d: sotto il mouse %s\n", k, h ? typeid (*h).name() : "niente"); } auto* hit = ed.getComponentAt (juce::Point<int> (1, 1)); std::printf ("al punto delle schede c'e': %s\n", hit ? typeid (*hit).name() : "niente");
        }
        for (int i = 0; i < 30; ++i) ed.tickForTest();
        for (int i = 0; i < 10; ++i) { if (ed.spPage.fv) ed.spPage.fv->specFrame (30); if (ed.stPage.fv) ed.stPage.fv->specFrame (30); }
        if (std::getenv ("JPERF"))
        {
            const double t0 = juce::Time::getMillisecondCounterHiRes();
            for (int i = 0; i < 20; ++i) { ed.tickForTest(); auto im = ed.createComponentSnapshot (ed.getLocalBounds(), true, 2.0f); }
            std::printf ("frame completo @2x: %.1f ms\n", (juce::Time::getMillisecondCounterHiRes() - t0) / 20.0);
            for (auto* c : ed.getChildren()) if (c->isVisible())
            {
                const double t2 = juce::Time::getMillisecondCounterHiRes();
                for (int i = 0; i < 10; ++i) { ed.tickForTest(); auto im = c->createComponentSnapshot (c->getLocalBounds(), true, 2.0f); }
                std::printf ("  %-28s %5.1f ms  (%dx%d)\n", typeid (*c).name(), (juce::Time::getMillisecondCounterHiRes() - t2) / 10.0, c->getWidth(), c->getHeight());
            }
            const double t1 = juce::Time::getMillisecondCounterHiRes();
            for (int i = 0; i < 20; ++i) { ed.tickForTest(); auto im = ed.easyPage.stage.createComponentSnapshot (ed.easyPage.stage.getLocalBounds(), true, 2.0f); }
            std::printf ("solo onda @2x: %.1f ms\n", (juce::Time::getMillisecondCounterHiRes() - t1) / 20.0);
            for (int v = 0; v < kEasyViews; ++v) { ed.easyPage.view = v; const double t3 = juce::Time::getMillisecondCounterHiRes();
              for (int i = 0; i < 20; ++i) { auto im = ed.easyPage.stage.createComponentSnapshot (ed.easyPage.stage.getLocalBounds(), true, 2.0f); }
              std::printf ("  vista %d @2x: %.1f ms\n", v, (juce::Time::getMillisecondCounterHiRes() - t3) / 20.0); }
        }
        auto img = ed.createComponentSnapshot (ed.getLocalBounds(), true, 1.0f);
        juce::File f (argc > 2 ? argv[2] : "/tmp/master.png"); f.deleteFile(); juce::FileOutputStream os (f); juce::PNGImageFormat().writeImageToStream (img, os);
        return 0;
    }

    {   // plugin appena caricato: non deve toccare nulla (solo latenza)
        auto p = blank(); juce::Random r3 (9); std::vector<float> in; 
        std::vector<float> a, b; run (*p, [&] (int, float& l, float& r) { l = r3.nextFloat() - 0.5f; r = r3.nextFloat() - 0.5f; in.push_back (l); }, 48000, a, b);
        const int lat = p->getLatencySamples(); double e = 0; for (size_t i = (size_t) lat + 100; i < a.size(); ++i) e += std::pow (a[i] - in[i - (size_t) lat], 2.0);
        std::printf ("default neutro: differenza %.1f dB (deve essere molto bassa)\n", 10 * std::log10 (e / a.size() + 1e-30));
    }
    {   // delta del bass control: solo sotto la frequenza
        auto p = blank(); setP (*p, "mode", 1); setP (*p, "bc_on", 1); setP (*p, "bc_punch", 0.8f); setP (*p, "bc_bal", 3); setP (*p, "bc_delta", 1); setP (*p, "bc_freq", 120);
        std::vector<float> a, b; run (*p, [&] (int i, float& l, float& r) { const double t = i / sr; l = r = (float) (0.3 * std::sin (2 * 3.14159265 * 50 * t) * (std::fmod (t, 0.5) < 0.2 ? 1 : 0.2) + 0.3 * std::sin (2 * 3.14159265 * 2000 * t)); }, 96000, a, b);
        juce::dsp::FFT f (15); std::vector<float> d (65536, 0); for (int i = 0; i < 32768; ++i) d[(size_t) i] = a[(size_t) (48000 + i)] * (0.5f - 0.5f * std::cos (6.2831853f * i / 32768));
        f.performFrequencyOnlyForwardTransform (d.data(), true);
        auto at = [&] (float hz) { const int k = juce::roundToInt (hz * 32768 / sr); float m = 0; for (int j = k - 3; j <= k + 3; ++j) m = juce::jmax (m, d[(size_t) j]); return gToDb (m / 8192.0f); };
        std::printf ("delta bassi: 50 Hz %.1f dB, 2 kHz %.1f dB (deve sparire)\n", at (50), at (2000));
    }
    {   // exciter acceso ma a zero = piatto; tape: armoniche
        auto p = blank(); setP (*p, "mode", 1); setP (*p, "ex_on", 1);
        std::vector<float> a, b; run (*p, [] (int i, float& l, float& r) { l = r = i == 100 ? 0.5f : 0.0f; }, 65536, a, b);
        juce::dsp::FFT f (16); std::vector<float> d (131072, 0); for (int i = 0; i < 65536; ++i) d[(size_t) i] = a[(size_t) i];
        f.performFrequencyOnlyForwardTransform (d.data(), true);
        float mn = 100, mx2 = -100; for (int k = 3; k < 30000; ++k) { const float db = gToDb (d[(size_t) k] / 0.5f); mn = juce::jmin (mn, db); mx2 = juce::jmax (mx2, db); }
        std::printf ("exciter a zero: %.2f .. %.2f dB\n", mn, mx2);
        for (int m = 0; m < 3; ++m)
        {
            auto q = blank(); setP (*q, "mode", 1); setP (*q, "tp_on", 1); setP (*q, "tp_model", (float) m); setP (*q, "tp_drive", 0.5f);
            std::vector<float> x, y; run (*q, [&] (int i, float& l, float& r) { l = r = 0.4f * (float) std::sin (2 * 3.14159265 * 100 * i / sr); }, 96000, x, y);
            bool ok = true; for (auto v : x) ok &= std::isfinite (v);
            std::printf ("tape %d drive 50%%: THD %.2f%% %s\n", m, thd (x, 48000, sr, 100), ok ? "" : "NaN");
        }
        auto e = blank(); setP (*e, "mode", 1); setP (*e, "ex_on", 1); setP (*e, "ex2_amt", 0.5f); setP (*e, "ex2_mode", 0);
        std::vector<float> x, y; run (*e, [&] (int i, float& l, float& r) { l = r = 0.3f * (float) std::sin (2 * 3.14159265 * 1000 * i / sr); }, 96000, x, y);
        std::printf ("exciter medi valvola 50%%: THD %.2f%%\n", thd (x, 48000, sr, 1000));
    }
    // fase 2: moduli neutri = somma piatta (risposta all'impulso, spettro)
    {
        auto p = fresh(); setP (*p, "mode", 1); setP (*p, "mx_on", 0); setP (*p, "lm_on", 0); setP (*p, "bc_on", 1); setP (*p, "eq_on", 1); setP (*p, "im_on", 1);
        std::vector<float> a, b; run (*p, [] (int i, float& l, float& r) { l = i == 100 ? 1.0f : 0.0f; r = i == 100 ? 0.5f : 0.0f; }, 65536, a, b);
        juce::dsp::FFT f (16); std::vector<float> d (131072, 0); for (int i = 0; i < 65536; ++i) d[(size_t) i] = a[(size_t) i];
        f.performFrequencyOnlyForwardTransform (d.data(), true);
        float mn = 100, mx2 = -100; for (int k = 3; k < 30000; ++k) { const float db = gToDb (d[(size_t) k]); mn = juce::jmin (mn, db); mx2 = juce::jmax (mx2, db); }
        std::printf ("bass+eq+imager neutri: risposta %.2f .. %.2f dB (deve essere piatta)\n", mn, mx2);
    }
    {   // mono sotto 120 Hz: un 50 Hz in controfase sparisce, un 1 kHz in controfase resta
        auto p = fresh(); setP (*p, "mx_on", 0); setP (*p, "lm_on", 0); setP (*p, "bc_on", 1); setP (*p, "bc_mono", 120);
        std::vector<float> a, b; run (*p, [&] (int i, float& l, float& r) { const float x = 0.3f * (float) std::sin (2 * 3.14159265 * 50 * i / sr) + 0.3f * (float) std::sin (2 * 3.14159265 * 1000 * i / sr); l = x; r = -x; }, 96000, a, b);
        // il canale S residuo a 50 Hz e a 1 kHz
        std::vector<float> s2; for (size_t i = 48000; i < a.size(); ++i) s2.push_back (0.5f * (a[i] - b[i]));
        juce::dsp::FFT f (15); std::vector<float> d (65536, 0); for (int i = 0; i < 32768; ++i) d[(size_t) i] = s2[(size_t) i] * (0.5f - 0.5f * std::cos (6.2831853f * i / 32768));
        f.performFrequencyOnlyForwardTransform (d.data(), true);
        auto at = [&] (float hz) { const int k = juce::roundToInt (hz * 32768 / sr); float m = 0; for (int j = k - 2; j <= k + 2; ++j) m = juce::jmax (m, d[(size_t) j]); return gToDb (m / 8192.0f); };
        std::printf ("mono sotto 120 Hz: lato a 50 Hz %.1f dB, a 1 kHz %.1f dB (ingresso -10.5)\n", at (50), at (1000));
    }
    {   // EQ dinamico: banda a 1 kHz, soglia -30, range -9: un tono forte viene abbassato, uno debole no
        for (float amp : { 0.5f, 0.01f })
        {
            auto p = fresh(); setP (*p, "mode", 1); setP (*p, "mx_on", 0); setP (*p, "lm_on", 0); setP (*p, "eq_on", 1); setP (*p, eqId (2, "f"), 1000); setP (*p, eqId (2, "dyn"), 1); setP (*p, eqId (2, "thr"), -30); setP (*p, eqId (2, "rng"), -9); setP (*p, eqId (2, "q"), 1.5f);
            std::vector<float> a, b; run (*p, [&] (int i, float& l, float& r) { l = r = amp * (float) std::sin (2 * 3.14159265 * 1000 * i / sr); }, 48000, a, b);
            float pk = 0; for (size_t i = 30000; i < a.size(); ++i) pk = juce::jmax (pk, std::abs (a[i]));
            std::printf ("EQ dinamico, tono %.2f: uscita %.1f dB rispetto all'ingresso (dinamica %.1f dB)\n", amp, gToDb (pk / amp), p->eqDyn[2].load());
        }
    }
    {   // imager: larghezza 0 su tutte le bande = mono
        auto p = fresh(); setP (*p, "mx_on", 0); setP (*p, "lm_on", 0); setP (*p, "im_on", 1); for (auto id : { "im_w1", "im_w2", "im_w3", "im_w4" }) setP (*p, id, 0);
        juce::Random r2 (5); std::vector<float> a, b; run (*p, [&] (int, float& l, float& r) { l = r2.nextFloat() - 0.5f; r = r2.nextFloat() - 0.5f; }, 48000, a, b);
        double es = 0, em = 0; for (size_t i = 10000; i < a.size(); ++i) { es += (a[i] - b[i]) * (a[i] - b[i]); em += (a[i] + b[i]) * (a[i] + b[i]); }
        std::printf ("imager larghezza 0: lato/medio %.1f dB (deve essere molto basso)\n", 10 * std::log10 (es / em + 1e-20));
    }
    for (int safe = 0; safe < 2; ++safe)
    {   // alti gia' larghi (correlazione ~0.3) allargati al 200%: con la sicurezza non devono andare in controfase
        auto p = fresh(); setP (*p, "mx_on", 0); setP (*p, "lm_on", 0); setP (*p, "im_on", 1); setP (*p, "im_w4", 2.0f); setP (*p, "im_s4", 1.0f); setP (*p, "im_safe", (float) safe);
        juce::Random r2 (9); std::vector<float> a, b;
        run (*p, [&] (int, float& l, float& r) { const float c = r2.nextFloat() - 0.5f, d = r2.nextFloat() - 0.5f; l = c + 0.6f * d; r = c - 0.6f * d; }, 96000, a, b);
        std::printf ("imager alti 200%% + stereoize, sicurezza %s: correlazione alti %.2f (limitato %.0f%%)\n", safe ? "ON " : "OFF", p->imCorr[3].load(), p->imLim[3].load() * 100);
    }
    {   // multibanda neutro = piatto
        auto p = fresh(); setP (*p, "mx_on", 0); setP (*p, "lm_on", 0); setP (*p, "mb_on", 1); for (int b = 1; b <= 3; ++b) { setP (*p, ("mb" + juce::String (b) + "_thr").toRawUTF8(), 0); setP (*p, ("mb" + juce::String (b) + "_ratio").toRawUTF8(), 1); }
        std::vector<float> a, b; run (*p, [] (int i, float& l, float& r) { l = r = i == 100 ? 0.5f : 0.0f; }, 65536, a, b);
        juce::dsp::FFT f (16); std::vector<float> d (131072, 0); for (int i = 0; i < 65536; ++i) d[(size_t) i] = a[(size_t) i];
        f.performFrequencyOnlyForwardTransform (d.data(), true);
        float mn = 100, mx2 = -100; for (int k = 3; k < 30000; ++k) { const float db = gToDb (d[(size_t) k] / 0.5f); mn = juce::jmin (mn, db); mx2 = juce::jmax (mx2, db); }
        std::printf ("multibanda neutro: %.2f .. %.2f dB\n", mn, mx2);
    }
    for (int sib = 0; sib < 2; ++sib)
    {   // de-esser: 200 Hz + (8 kHz forte o no)
        auto p = fresh(); setP (*p, "mx_on", 0); setP (*p, "lm_on", 0); setP (*p, "ds_on", 1); setP (*p, "ds_lo", 6000); setP (*p, "ds_thr", -12); setP (*p, "ds_amt", 0.6f);
        const float hf = sib ? 0.25f : 0.02f;
        std::vector<float> a, b; run (*p, [&] (int i, float& l, float& r) { l = r = 0.3f * (float) std::sin (2 * 3.14159265 * 200 * i / sr) + hf * (float) std::sin (2 * 3.14159265 * 8000 * i / sr); }, 48000, a, b);
        std::printf ("de-esser, sibilante %s: riduzione %.1f dB (relativo %.1f)\n", sib ? "forte" : "debole", p->dsGr.load(), p->dsRel.load());
    }
    {   // bus comp: -6 dBFS, soglia -20, 4:1
        auto p = fresh(); setP (*p, "mx_on", 0); setP (*p, "lm_on", 0); setP (*p, "bus_on", 1); setP (*p, "bus_thr", -20); setP (*p, "bus_ratio", 2); setP (*p, "bus_auto", 0); setP (*p, "bus_range", 20);
        std::vector<float> a, b; run (*p, [&] (int i, float& l, float& r) { l = r = 0.5f * (float) std::sin (2 * 3.14159265 * 100 * i / sr); }, 48000, a, b);
        float pk = 0; for (size_t i = 30000; i < a.size(); ++i) pk = juce::jmax (pk, std::abs (a[i]));
        std::printf ("bus comp 4:1 soglia -20 su -6 dBFS: riduzione %.1f dB (attesa ~10.5), uscita %.1f dBFS\n", p->busGr.load(), gToDb (pk));
    }
    {   // OTT: un segnale debole viene alzato
        auto p = fresh(); setP (*p, "mx_on", 0); setP (*p, "lm_on", 0); setP (*p, "mb_on", 1);
        for (int b = 1; b <= 3; ++b) { const juce::String id = "mb" + juce::String (b) + "_"; setP (*p, (id + "thr").toRawUTF8(), -24); setP (*p, (id + "ratio").toRawUTF8(), 12); setP (*p, (id + "up").toRawUTF8(), 0.85f); }
        std::vector<float> a, b; run (*p, [&] (int i, float& l, float& r) { l = r = 0.002f * (float) std::sin (2 * 3.14159265 * 1000 * i / sr); }, 48000, a, b);
        float pk = 0; for (size_t i = 30000; i < a.size(); ++i) pk = juce::jmax (pk, std::abs (a[i]));
        std::printf ("OTT su tono a -54 dBFS: uscita %.1f dBFS (su %.1f dB)\n", gToDb (pk), p->mbUp[1].load());
    }
    // 1) latenza
    {
        auto p = fresh(); setP (*p, "mx_gain", 0);
        std::vector<float> a, b; run (*p, [] (int i, float& l, float& r) { l = r = i == 1000 ? 0.1f : 0.0f; }, 8000, a, b);
        int pk = 0; for (int i = 0; i < (int) a.size(); ++i) if (std::abs (a[(size_t) i]) > std::abs (a[(size_t) pk])) pk = i;
        std::printf ("latenza dichiarata %d, misurata %d, ampiezza %.4f (0.1)\n", p->getLatencySamples(), pk - 1000, a[(size_t) pk]);
    }
    // 2) LUFS: 997 Hz a -20 dBFS solo a sinistra = -23 LUFS
    {
        auto p = fresh(); setP (*p, "mx_on", 0); setP (*p, "lm_on", 0);
        std::vector<float> a, b; run (*p, [&] (int i, float& l, float& r) { l = 0.1f * (float) std::sin (2 * 3.14159265 * 997 * i / sr); r = 0; }, (int) sr * 6, a, b);
        std::printf ("LUFS sinusoide -20 dBFS sx: I %.2f  S %.2f (atteso -23.0)\n", p->lufsI.load(), p->lufsS.load());
    }
    // 3) ceiling e true peak sotto spinta forte, per ogni stile
    for (int st = 0; st < kNumStyles; ++st)
        for (float gdb : { 6.0f, 12.0f, 18.0f })
        {
            auto p = fresh(); setP (*p, "mx_gain", gdb); setP (*p, "mx_style", (float) st); setP (*p, "mx_clip", 0.4f); setP (*p, "lm_ceil", -1.0f);
            std::vector<float> a, b; run (*p, beat, (int) sr * 8, a, b);
            float sp = 0; for (size_t i = 24000; i < a.size(); ++i) sp = juce::jmax (sp, juce::jmax (std::abs (a[i]), std::abs (b[i])));
            std::printf ("%-11s +%2.0f dB: picco %.2f dBFS  true peak %.2f dBTP  LUFS-I %.1f  GR max %.1f  lim %.1f\n", styleInfo (st).name, gdb, gToDb (sp),
                         juce::jmax (measTP (a, 24000), measTP (b, 24000)), p->lufsI.load(), p->grMax.load(), p->grLim.load());
        }
    // 4) distorsione sui bassi: 60 Hz a -6 dBFS spinto di 8 dB
    for (int st = 0; st < kNumStyles; ++st)
    {
        auto p = fresh(); setP (*p, "mx_gain", 8); setP (*p, "mx_style", (float) st); setP (*p, "mx_clip", 0.0f);
        std::vector<float> a, b; run (*p, [&] (int i, float& l, float& r) { l = r = 0.5f * (float) std::sin (2 * 3.14159265 * 60 * i / sr); }, (int) sr * 3, a, b);
        std::printf ("THD 60 Hz +8 dB  %-11s %.2f%%\n", styleInfo (st).name, thd (a, 48000, sr, 60));
    }
    // 5) confronto a volume uguale
    {
        auto p = fresh(); setP (*p, "mx_gain", 9); setP (*p, "compare", 1);
        std::vector<float> a, b; run (*p, beat, (int) sr * 8, a, b);
        std::printf ("confronto: match %.1f dB, ingresso S %.1f, uscita S %.1f\n", p->matchDb.load(), p->inS.load(), p->lufsS.load());
    }
    // 6) dither 16 bit shaped: silenzio -> rumore
    {
        auto p = fresh(); setP (*p, "dither", 3); setP (*p, "mx_on", 0); setP (*p, "lm_on", 0);
        std::vector<float> a, b; run (*p, [] (int, float& l, float& r) { l = r = 0; }, 48000, a, b);
        double e = 0; for (size_t i = 10000; i < a.size(); ++i) e += a[i] * a[i];
        std::printf ("dither 16 shaped: rumore %.1f dBFS rms\n", gToDb ((float) std::sqrt (e / (a.size() - 10000))));
    }
    // 7) CPU
    {
        auto p = fresh(); setP (*p, "mx_gain", 10); setP (*p, "mx_style", PUNCH); setP (*p, "mx_clip", 0.5f);
        std::vector<float> a, b; a.reserve ((size_t) sr * 20); b.reserve ((size_t) sr * 20);
        const auto t0 = juce::Time::getMillisecondCounterHiRes();
        run (*p, beat, (int) sr * 20, a, b);
        const auto ms = juce::Time::getMillisecondCounterHiRes() - t0;
        std::printf ("CPU: 20 s in %.0f ms (%.1f%% di un core, con il generatore)\n", ms, ms / 200.0);
    }
    for (int i = 0; i < (int) presets().size(); ++i)
    {
        auto p = fresh(); p->loadPreset (i); std::vector<float> a, b; run (*p, beat, (int) sr * 4, a, b);
        float sp = 0; bool ok = true; for (auto v : a) { if (! std::isfinite (v)) ok = false; sp = juce::jmax (sp, std::abs (v)); }
        std::printf ("preset %-24s picco %.2f dBFS  LUFS-S %.1f %s\n", presets()[(size_t) i].name, gToDb (sp), p->lufsS.load(), ok ? "" : "NaN!");
    }
    return 0;
}
