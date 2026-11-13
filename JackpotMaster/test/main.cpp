#include <cstdlib>
#include "../Source/MasterProcessor.h"
#include "../Source/MasterEditor.h"
using namespace jms;

static void setP (Processor& p, const char* id, float v) { auto* rp = p.apvts.getParameter (id); rp->setValueNotifyingHost (rp->convertTo0to1 (v)); }
static void run (Processor& p, std::function<void (int, float&, float&)> gen, int len, std::vector<float>& oL, std::vector<float>& oR, int block = 512)
{
    juce::AudioBuffer<float> b (2, block); juce::MidiBuffer mb;
    for (int off = 0; off < len; off += block)
    {
        const int n = juce::jmin (block, len - off); b.setSize (2, n, false, false, true);
        for (int i = 0; i < n; ++i) { float l, r; gen (off + i, l, r); b.setSample (0, i, l); b.setSample (1, i, r); }
        p.processBlock (b, mb);
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
    if (argc > 1 && juce::String (argv[1]) == "easysnap")
    {
        auto p = blank(); p->loadPreset (1 + (std::getenv ("JPR") ? std::atoi (std::getenv ("JPR")) : 0));
        std::vector<float> a, b; run (*p, [&] (int k, float& l, float& r) { beat (k, l, r); l *= 0.6f; r *= 0.6f; }, (int) sr * 22, a, b);
        Editor ed (*p);
        if (auto* v = std::getenv ("JVIEW")) ed.easyPage.view = std::atoi (v);
        for (int i = 0; i < 30; ++i) ed.tickForTest();
        if (auto* gd = std::getenv ("JGUIDE")) { ed.easyPage.guide.topic = std::atoi (gd); ed.easyPage.guide.setVisible (true); ed.easyPage.guide.toFront (false); }
        if (std::getenv ("JPERF"))
        {
            const double t0 = juce::Time::getMillisecondCounterHiRes();
            for (int i = 0; i < 20; ++i) { ed.tickForTest(); auto im = ed.createComponentSnapshot (ed.getLocalBounds(), true, 2.0f); }
            std::printf ("frame completo @2x: %.1f ms\n", (juce::Time::getMillisecondCounterHiRes() - t0) / 20.0);
            const double t1 = juce::Time::getMillisecondCounterHiRes();
            for (int i = 0; i < 20; ++i) { ed.tickForTest(); auto im = ed.easyPage.stage.createComponentSnapshot (ed.easyPage.stage.getLocalBounds(), true, 2.0f); }
            std::printf ("solo onda @2x: %.1f ms\n", (juce::Time::getMillisecondCounterHiRes() - t1) / 20.0);
        }
        auto img = ed.createComponentSnapshot (ed.getLocalBounds(), true, 1.0f);
        juce::File f (argc > 2 ? argv[2] : "/tmp/master.png"); f.deleteFile(); juce::FileOutputStream os (f); juce::PNGImageFormat().writeImageToStream (img, os);
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "snap")
    {
        auto p = fresh(); setP (*p, "mode", 1); setP (*p, "mx_gain", 8); setP (*p, "mx_style", PUNCH); setP (*p, "mx_clip", 0.4f);
        setP (*p, "bc_on", 1); setP (*p, "bc_punch", 0.4f); setP (*p, "bc_mono", 110); setP (*p, "bc_harm", 0.3f); setP (*p, "bc_thr", -12);
        setP (*p, "eq_on", 1); setP (*p, eqId (2, "g"), -3); setP (*p, eqId (4, "dyn"), 1); setP (*p, eqId (4, "f"), 7000); setP (*p, eqId (4, "thr"), -40); setP (*p, eqId (5, "g"), 2.5f);
        setP (*p, "ds_on", 1); setP (*p, "ds_thr", -10); setP (*p, "bus_on", 1); setP (*p, "bus_thr", -18); setP (*p, "mb_on", 1); setP (*p, "mb3_up", 0.5f); setP (*p, "mb1_thr", -20);
        setP (*p, "im_on", 1); setP (*p, "im_w1", 0.3f); setP (*p, "im_w3", 1.4f); setP (*p, "im_w4", 1.6f); setP (*p, "im_s3", 0.3f); setP (*p, "im_s4", 0.4f);
        std::vector<float> a, b; run (*p, beat, (int) sr * 12, a, b);
        Editor ed (*p);
        if (auto* jsc = std::getenv ("JSCALE")) ed.scaler.set ((float) std::atof (jsc));
        if (auto* pg = std::getenv ("JPAGE")) ed.select (std::atoi (pg));
        if (auto* av = std::getenv ("JAVIEW")) { ed.advView = std::atoi (av); ed.applyAdvView(); }
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
        for (int i = 0; i < 30; ++i) ed.tickForTest();
        if (std::getenv ("JPERF"))
        {
            const double t0 = juce::Time::getMillisecondCounterHiRes();
            for (int i = 0; i < 20; ++i) { ed.tickForTest(); auto im = ed.createComponentSnapshot (ed.getLocalBounds(), true, 2.0f); }
            std::printf ("frame completo @2x: %.1f ms\n", (juce::Time::getMillisecondCounterHiRes() - t0) / 20.0);
            const double t1 = juce::Time::getMillisecondCounterHiRes();
            for (int i = 0; i < 20; ++i) { ed.tickForTest(); auto im = ed.easyPage.stage.createComponentSnapshot (ed.easyPage.stage.getLocalBounds(), true, 2.0f); }
            std::printf ("solo onda @2x: %.1f ms\n", (juce::Time::getMillisecondCounterHiRes() - t1) / 20.0);
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
