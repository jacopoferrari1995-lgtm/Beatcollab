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
    auto fresh = [&] { auto p = std::make_unique<Processor>(); p->setPlayConfigDetails (2, 2, sr, 512); p->prepareToPlay (sr, 512); return p; };
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
    if (argc > 1 && juce::String (argv[1]) == "snap")
    {
        auto p = fresh(); setP (*p, "mx_gain", 8); setP (*p, "mx_style", PUNCH); setP (*p, "mx_clip", 0.4f);
        std::vector<float> a, b; run (*p, beat, (int) sr * 12, a, b);
        Editor ed (*p);
        if (auto* jsc = std::getenv ("JSCALE")) ed.scaler.set ((float) std::atof (jsc));
        ed.tickForTest();
        auto img = ed.createComponentSnapshot (ed.getLocalBounds(), true, 1.0f);
        juce::File f (argc > 2 ? argv[2] : "/tmp/master.png"); f.deleteFile(); juce::FileOutputStream os (f); juce::PNGImageFormat().writeImageToStream (img, os);
        return 0;
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
