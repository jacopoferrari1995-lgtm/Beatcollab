#include "../Source/TubeProcessor.h"
#include "../Source/TubeEditor.h"
using namespace jtbx;
static void setP (Processor& p, const juce::String& id, float v) { auto* r = p.apvts.getParameter (id); r->setValueNotifyingHost (r->convertTo0to1 (v)); }
int main (int argc, char** argv)
{
    if (auto* t = std::getenv ("JTHEME")) skin::cur() = std::atoi (t);
    juce::ScopedJuceInitialiser_GUI gui;
    if (argc > 1 && juce::String (argv[1]) == "lab")
    {   // come THE_LAB: latenza vera, buffer, aliasing, click per manopola, 48/96 kHz, CPU. JPRESET = indice del preset
        const int pr = juce::SystemStats::getEnvironmentVariable ("JPRESET", "7").getIntValue();
        auto mk = [&] (double rate, int blk = 512) { auto p = std::make_unique<Processor>(); p->setPlayConfigDetails (2, 2, rate, blk); p->prepareToPlay (rate, blk); p->loadPreset (pr);
            for (auto kv : juce::StringArray::fromTokens (juce::SystemStats::getEnvironmentVariable ("JSET", ""), ",", "")) if (kv.contains ("=")) setP (*p, kv.upToFirstOccurrenceOf ("=", false, false), kv.fromFirstOccurrenceOf ("=", false, false).getFloatValue());
            return p; };
        auto render = [&] (Processor& p, int len, std::function<float (int)> gen, int blk = 512, bool uneven = false, std::function<void (int)> at = {})
        {
            std::vector<float> o; juce::MidiBuffer m; juce::Random rb (3);
            for (int off = 0; off < len; )
            {
                const int n = uneven ? juce::jmin (len - off, 1 + rb.nextInt (blk)) : juce::jmin (blk, len - off);
                if (at) at (off);
                juce::AudioBuffer<float> b (2, n); for (int i = 0; i < n; ++i) { const float v = gen (off + i); b.setSample (0, i, v); b.setSample (1, i, v); }
                p.processBlock (b, m); for (int i = 0; i < n; ++i) o.push_back (b.getSample (0, i)); off += n;
            }
            return o;
        };
        const double sr = 48000;
        std::printf ("preset: %s\n", presets()[(size_t) pr].name);
        {   // latenza
            auto p = mk (sr); juce::Random rr (9); std::vector<float> in (48000 * 3); for (auto& x : in) x = 0.02f * (rr.nextFloat() * 2 - 1);
            auto o = render (*p, (int) in.size(), [&] (int k) { return in[(size_t) k]; });
            double best = -1e30; int bl = 0; std::vector<double> cc (400);
            for (int l = 0; l < 400; ++l) { double c = 0; for (int i = 24000; i < (int) in.size() - 400; ++i) c += (double) in[(size_t) i] * o[(size_t) (i + l)]; cc[(size_t) l] = c; if (c > best) { best = c; bl = l; } }
            double fr = bl; if (bl > 0 && bl < 399) { const double a = cc[(size_t) bl - 1], b2 = cc[(size_t) bl], c = cc[(size_t) bl + 1]; fr = bl + 0.5 * (a - c) / (a - 2 * b2 + c); }
            std::printf ("latenza dichiarata %d, misurata %.2f\n", p->getLatencySamples(), fr);
            // buffer irregolari
            auto p2 = mk (sr); auto o2 = render (*p2, (int) in.size(), [&] (int k) { return in[(size_t) k]; }, 512, true);
            double e = 0, d = 0; for (size_t i = 24000; i < in.size(); ++i) { e += (double) o[i] * o[i]; d += std::pow (o[i] - o2[i], 2.0); }
            std::printf ("blocchi irregolari vs 512: %.1f dB\n", 10 * std::log10 (d / (e + 1e-30) + 1e-30));
        }
        // aliasing
        for (float amp : { 0.5f }) for (double f0 : { 6300.0, 10000.0 })
        {
            const double ff = (double) (((int) std::round (f0 * 65536 / sr)) | 1) * sr / 65536;
            auto p = mk (sr); auto o = render (*p, (int) sr * 3, [&] (int k) { return amp * (float) std::sin (2 * juce::MathConstants<double>::pi * ff * k / sr); });
            const int N = 65536; juce::dsp::FFT fft (16); std::vector<float> a ((size_t) 2 * N, 0);
            for (int i = 0; i < N; ++i) a[(size_t) i] = o[o.size() - N + (size_t) i] * (0.5f - 0.5f * std::cos (6.2831853f * i / N));
            fft.performFrequencyOnlyForwardTransform (a.data());
            const int k0 = (int) std::round (ff * N / sr); float ref = 0; for (int k = k0 - 4; k <= k0 + 4; ++k) ref = std::max (ref, a[(size_t) k]);
            float worst = 0; double wf = 0;
            for (int k = 40; k < N / 2; ++k) { bool h = false; for (int hh = 1; hh < 10; ++hh) if (std::abs (k - k0 * hh) <= 6) h = true; if (std::abs (k - k0) < k0 / 20) h = true; if (! h && a[(size_t) k] > worst) { worst = a[(size_t) k]; wf = k * sr / N; } }
            std::printf ("aliasing tono %.0f Hz: %.1f dB a %.0f Hz\n", ff, juce::Decibels::gainToDecibels (worst / ref), wf);
        }
        {   // 48 vs 96
            for (double f : { 100.0, 1000.0, 5000.0, 10000.0 })
            {
                float gg[2];
                for (int j = 0; j < 2; ++j) { const double rate = j ? 96000.0 : 48000.0; auto p = mk (rate); auto o = render (*p, (int) rate * 2, [&] (int k) { return 0.05f * (float) std::sin (2 * juce::MathConstants<double>::pi * f * k / rate); });
                    double e = 0; for (size_t i = o.size() - (size_t) rate; i < o.size(); ++i) e += (double) o[i] * o[i]; gg[j] = juce::Decibels::gainToDecibels ((float) std::sqrt (e / rate) / (0.05f / std::sqrt (2.0f))); }
                std::printf ("%6.0f Hz: 48k %+.2f  96k %+.2f\n", f, gg[0], gg[1]);
            }
        }
        {   // click: ogni manopola continua salta del 30% 4 volte su un tono a 1 kHz; energia non armonica 5-20 kHz contro la versione ferma all'arrivo
            auto probe = mk (sr); const int np = probe->getParameters().size(); int nC = 0;
            for (int pi = 0; pi < np; ++pi)
            {
                auto* fp = dynamic_cast<juce::AudioParameterFloat*> (probe->getParameters()[pi]); if (fp == nullptr) continue;
                auto rend = [&] (int mode)
                {
                    auto p = mk (sr); auto* r = p->getParameters()[pi]; const float v0 = r->getValue(), v1 = v0 > 0.5f ? v0 - 0.3f : v0 + 0.3f;
                    if (mode == 1) r->setValueNotifyingHost (v1);
                    return render (*p, 512 * 200, [&] (int k) { return 0.5f * (float) std::sin (2 * juce::MathConstants<double>::pi * 999.76 * k / sr); }, 512, false,
                                   [&] (int off) { const int blk = off / 512; if (mode == 2 && blk >= 100 && (blk - 100) % 20 == 0 && blk < 180) r->setValueNotifyingHost (((blk - 100) / 20) % 2 ? v0 : v1); });
                };
                const auto a = rend (0), b1 = rend (1), bb = rend (2);
                double ch = 0; for (size_t i = 0; i < a.size(); ++i) ch = std::max (ch, (double) std::abs (bb[i] - a[i])); if (ch < 1e-6) continue;
                juce::dsp::FFT fft (11);
                auto nonHarm = [&] (const std::vector<float>& x, size_t at) { std::vector<float> w (4096, 0.0f); for (int i = 0; i < 2048; ++i) w[(size_t) i] = x[at + (size_t) i] * (0.5f - 0.5f * std::cos (6.2831853f * i / 2047));
                    fft.performFrequencyOnlyForwardTransform (w.data()); double s2 = 0; for (int k = 1; k < 1024; ++k) { const double fq = k * sr / 2048; if (fq < 5000 || fq > 20000) continue; const double hn = fq / 999.76; if (std::abs (hn - std::round (hn)) * 999.76 < 120) continue; s2 += (double) w[(size_t) k] * w[(size_t) k]; } return s2; };
                double e = 0; for (int j = 0; j < 4; ++j) { const size_t at = (size_t) (100 + j * 20) * 512 - 1024; e = std::max (e, nonHarm (bb, at) - nonHarm (j % 2 == 0 ? b1 : a, at)); }
                const double db = 20 * std::log10 (std::sqrt (std::max (0.0, e)) / 1024.0 * 2.0 / 0.5 + 1e-12);
                if (db > -45) { std::printf ("  %-20s %.1f dB%s\n", fp->getName (30).toRawUTF8(), db, db > -35 ? " CLICK" : ""); if (db > -35) ++nC; }
            }
            std::printf ("click: %d\n", nC);
        }
        {   // CPU
            auto p = mk (sr); juce::Random rr (4); const auto t0 = juce::Time::getMillisecondCounterHiRes();
            render (*p, (int) sr * 10, [&] (int) { return 0.2f * (rr.nextFloat() * 2 - 1); });
            std::printf ("CPU: %.1f%% di un core\n", (juce::Time::getMillisecondCounterHiRes() - t0) / 100.0);
        }
        return 0;
    }
    if (argc > 2 && juce::String (argv[1]) == "snap")
    {
        Processor p; p.prepareToPlay (44100, 512); p.loadPreset (juce::SystemStats::getEnvironmentVariable ("JPRESET", "0").getIntValue());
        juce::AudioBuffer<float> b (2, 512); juce::MidiBuffer m; juce::Random r;
        for (int k = 0; k < 100; ++k) { for (int i = 0; i < 512; ++i) { const float v = (r.nextFloat() - 0.5f) * 0.4f; b.setSample (0, i, v); b.setSample (1, i, v * 0.8f); } p.processBlock (b, m); }
        Editor ed (p); ed.setSize (Editor::W, Editor::H); ed.tickForTest();
        if (std::getenv ("JPERF"))
        {
            for (int t : { 3, 4, 8, 9, 10, 1, 2 })
            {
                skin::cur() = t; neo::K::theme = -1; skin::apply(); ed.repaint();
                auto t0 = juce::Time::getMillisecondCounterHiRes(); auto im = ed.createComponentSnapshot (ed.getLocalBounds(), true, 1.0f); const double full = juce::Time::getMillisecondCounterHiRes() - t0;
                t0 = juce::Time::getMillisecondCounterHiRes();
                for (int i = 0; i < 30; ++i) { ed.tickForTest(); ed.tubes.createComponentSnapshot (ed.tubes.getLocalBounds(), true, 1.0f); }
                std::printf ("tema %2d: primo disegno %.1f ms, fotogramma %.2f ms\n", t, full, (juce::Time::getMillisecondCounterHiRes() - t0) / 30.0);
                juce::File f ("/tmp/tb_tema" + juce::String (t) + ".png"); f.deleteFile(); juce::FileOutputStream os (f); juce::PNGImageFormat().writeImageToStream (im, os);
            }
        }
        auto img = ed.createComponentSnapshot (ed.getLocalBounds(), true, 1.0f);
        juce::File f (argv[2]); f.deleteFile(); juce::FileOutputStream os (f); juce::PNGImageFormat().writeImageToStream (img, os);
        return 0;
    }
    const double sr = 44100;
    auto run = [&] (std::function<void (Processor&)> setup, float amp, double freq, float& rmsDb, float& thd, float& h2, float& h3)
    {
        Processor p; p.prepareToPlay (sr, 512); setup (p);
        juce::AudioBuffer<float> b (2, 512); juce::MidiBuffer m; double ph = 0, ei = 0, eo = 0; bool nan = false; std::vector<float> out;
        for (int k = 0; k < 200; ++k)
        {
            for (int i = 0; i < 512; ++i) { const float v = (float) std::sin (ph) * amp; ph += 2 * juce::MathConstants<double>::pi * freq / sr; b.setSample (0, i, v); b.setSample (1, i, v); if (k > 40) ei += v * v; }
            p.processBlock (b, m);
            for (int i = 0; i < 512; ++i) { const float x = b.getSample (0, i); if (! std::isfinite (x)) nan = true; if (k > 40) { eo += x * x; out.push_back (x); } }
        }
        rmsDb = nan ? -999.0f : (float) juce::Decibels::gainToDecibels (std::sqrt (eo / ei));
        auto harm = [&] (double f) { double re = 0, im = 0; for (size_t i = 0; i < out.size(); ++i) { const double a = 2 * juce::MathConstants<double>::pi * f * i / sr; re += out[i] * std::cos (a); im += out[i] * std::sin (a); } return 2 * std::sqrt (re * re + im * im) / out.size(); };
        const double f1 = harm (freq); h2 = (float) (100 * harm (freq * 2) / f1); h3 = (float) (100 * harm (freq * 3) / f1);
        const double tot = eo / out.size(); thd = (float) (100.0 * std::sqrt (std::max (0.0, tot - f1 * f1 / 2) / std::max (1e-12, f1 * f1 / 2)));
    };
    float r, t, h2, h3;
    {   // neutro: all'avvio l'uscita e' l'ingresso ritardato, al bit
        Processor p; p.prepareToPlay (sr, 512); const int L = p.getLatencySamples(); juce::Random rr (5); std::vector<float> in, out;
        juce::AudioBuffer<float> b (2, 512); juce::MidiBuffer m;
        for (int k = 0; k < 50; ++k) { for (int i = 0; i < 512; ++i) { const float v = rr.nextFloat() - 0.5f; in.push_back (v); b.setSample (0, i, v); b.setSample (1, i, v); } p.processBlock (b, m); for (int i = 0; i < 512; ++i) out.push_back (b.getSample (0, i)); }
        double e = 0; for (size_t i = (size_t) L; i < out.size(); ++i) e = std::max (e, (double) std::abs (out[i] - in[i - (size_t) L]));
        std::printf ("neutro all'avvio: latenza %d, differenza massima %.2g\n", L, e);
        // acceso e poi rimesso a zero: torna al bit
        setP (p, "drive", 6); setP (p, "enh", 5);
        for (int k = 0; k < 40; ++k) { for (int i = 0; i < 512; ++i) { const float v = rr.nextFloat() - 0.5f; b.setSample (0, i, v); b.setSample (1, i, v); } p.processBlock (b, m); }
        setP (p, "drive", 0); setP (p, "enh", 0); in.clear(); out.clear();
        for (int k = 0; k < 80; ++k) { for (int i = 0; i < 512; ++i) { const float v = rr.nextFloat() - 0.5f; in.push_back (v); b.setSample (0, i, v); b.setSample (1, i, v); } p.processBlock (b, m); for (int i = 0; i < 512; ++i) out.push_back (b.getSample (0, i)); }
        e = 0; for (size_t i = 40000; i < out.size(); ++i) e = std::max (e, (double) std::abs (out[i] - in[i - (size_t) L]));
        std::printf ("rimesso a zero: differenza massima %.2g\n", e);
    }
    for (int c = 0; c < kChars; ++c)
    {
        std::printf ("%-7s", charOf (c).name);
        for (float d : { 2.0f, 5.0f, 8.0f, 10.0f }) for (float amp : { 0.1f, 0.5f })
        { run ([c, d] (Processor& p) { setP (p, "char", (float) c); setP (p, "drive", d); }, amp, 220, r, t, h2, h3); std::printf ("  d%.0f/%.1f %+.1fdB %.1f%% (%.1f/%.1f)", d, amp, r, t, h2, h3); }
        std::printf ("\n");
    }
    for (const char* id : { "warmth", "air", "enh" }) for (double f : { 80.0, 1000.0, 6000.0 })
    { run ([id] (Processor& p) { setP (p, id, 8); }, 0.3f, f, r, t, h2, h3); std::printf ("%-7s 8 a %5.0f Hz: %+.2f dB thd %.2f%% (2a %.2f 3a %.2f)\n", id, f, r, t, h2, h3); }
    for (int c = 0; c < kChars; ++c) { run ([c] (Processor& p) { setP (p, "char", (float) c); setP (p, "auto", 0); setP (p, "drive", 6); }, 0.3f, 220, r, t, h2, h3); std::printf ("%-7s drive 6 senza AUTO: %+.2f dB\n", charOf (c).name, r); }
    for (int i = 0; i < (int) presets().size(); ++i) { run ([i] (Processor& p) { p.loadPreset (i); }, 0.3f, 150, r, t, h2, h3); std::printf ("preset %-34s %+.2f dB thd %.2f%%\n", presets()[(size_t) i].name, r, t); }
    {   // cambio di carattere mentre suona: cerca salti
        Processor p; p.prepareToPlay (sr, 512); setP (p, "drive", 6);
        juce::AudioBuffer<float> b (2, 512); juce::MidiBuffer m; double ph = 0; std::vector<float> o;
        for (int k = 0; k < 400; ++k) { if (k >= 100 && k % 40 == 0) setP (p, "char", (float) ((k / 40) % kChars)); for (int i = 0; i < 512; ++i) { const float v = 0.4f * (float) std::sin (ph); ph += 2 * juce::MathConstants<double>::pi * 110 / sr; b.setSample (0, i, v); b.setSample (1, i, v); } p.processBlock (b, m); for (int i = 0; i < 512; ++i) o.push_back (b.getSample (0, i)); }
        auto d2 = [&] (size_t a, size_t b) { double w = 0; for (size_t i = a; i + 2 < b; ++i) w = std::max (w, (double) std::abs (o[i + 2] - 2 * o[i + 1] + o[i])); return w; };
        double worst = 0;
        for (int k = 120; k < 400; k += 40) { const size_t at = (size_t) k * 512; const double sw = d2 (at - 2000, at + 4000), st = juce::jmax (d2 (at - 20000, at - 6000), d2 (at + 8000, at + 20000)); worst = std::max (worst, sw / st); }
        std::printf ("cambio carattere: salto / fermo peggiore %.2f\n", worst);
        if (std::getenv ("JDBG")) for (int k = 120; k < 400; k += 40) { std::printf ("%d:", (k / 40) % kChars); for (int j = -8; j < 24; ++j) { const size_t at = (size_t) k * 512 + (size_t) (j * 512); std::printf (" %.4f", d2 (at, at + 512)); } std::printf ("\n"); }
    }
    {
        Processor p; p.prepareToPlay (sr, 512); p.loadPreset (7);
        juce::AudioBuffer<float> b (2, 512); juce::MidiBuffer m; juce::Random rr; const auto t0 = juce::Time::getMillisecondCounterHiRes();
        for (int k = 0; k < 860; ++k) { for (int i = 0; i < 512; ++i) { b.setSample (0, i, rr.nextFloat() - 0.5f); b.setSample (1, i, rr.nextFloat() - 0.5f); } p.processBlock (b, m); }
        std::printf ("CPU 10 s di audio (preset 7): %.0f ms\n", juce::Time::getMillisecondCounterHiRes() - t0);
    }
}
