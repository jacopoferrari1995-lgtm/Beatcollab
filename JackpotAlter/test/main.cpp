#include <cstdlib>
#include "../Source/AlterProcessor.h"
#include "../Source/AlterEditor.h"
using namespace ja;

static void setP (Processor& p, const char* id, float v) { auto* rp = p.apvts.getParameter (id); rp->setValueNotifyingHost (rp->convertTo0to1 (v)); }

static std::vector<float> run (Processor& p, std::function<float (int)> gen, int len, int block = 512, std::vector<float>* right = nullptr)
{
    juce::AudioBuffer<float> b (2, block); juce::MidiBuffer mb; std::vector<float> out;
    for (int off = 0; off < len; off += block)
    {
        const int n = juce::jmin (block, len - off); b.setSize (2, n, false, false, true);
        for (int i = 0; i < n; ++i) { const float x = gen (off + i); b.setSample (0, i, x); b.setSample (1, i, x); }
        p.processBlock (b, mb);
        for (int i = 0; i < n; ++i) { out.push_back (b.getSample (0, i)); if (right) right->push_back (b.getSample (1, i)); }
    }
    return out;
}

// frequenza del picco piu forte (FFT 32768 + interpolazione)
static float peakHz (const std::vector<float>& x, int start, double sr, float minHz = 30)
{
    const int order = 15, N = 1 << order; juce::dsp::FFT f (order); std::vector<float> d ((size_t) (2 * N), 0);
    for (int i = 0; i < N && start + i < (int) x.size(); ++i) d[(size_t) i] = x[(size_t) (start + i)] * (0.5f - 0.5f * std::cos (6.2831853f * i / N));
    f.performFrequencyOnlyForwardTransform (d.data(), true);
    int best = (int) (minHz * N / sr); for (int k = best; k < N / 2 - 1; ++k) if (d[(size_t) k] > d[(size_t) best]) best = k;
    const float a = std::log (d[(size_t) best - 1] + 1e-9f), b = std::log (d[(size_t) best] + 1e-9f), c = std::log (d[(size_t) best + 1] + 1e-9f);
    const float off = 0.5f * (a - c) / (a - 2 * b + c);
    return (float) ((best + off) * sr / N);
}
static float rms (const std::vector<float>& x, int a, int b) { double e = 0; for (int i = a; i < b; ++i) e += x[(size_t) i] * x[(size_t) i]; return (float) std::sqrt (e / juce::jmax (1, b - a)); }

int main (int argc, char** argv)
{
    if (auto* t = std::getenv ("JTHEME")) skin::cur() = std::atoi (t);
    juce::ScopedJuceInitialiser_GUI gui;
    const double sr = 48000;
    auto fresh = [&] () { auto p = std::make_unique<Processor>(); p->setPlayConfigDetails (2, 2, sr, 512); p->prepareToPlay (sr, 512); return p; };

    if (argc > 2 && juce::String (argv[1]) == "jumpdbg")
    {   // un salto solo di un parametro (argv[2]) a 2 s: residuo sopra 5 kHz rispetto al valore tenuto, ogni 5 ms
        const int pr = juce::SystemStats::getEnvironmentVariable ("JPRESET", "34").getIntValue(); const double sr = 48000;
        auto go = [&] (int mode) -> std::vector<float> { Processor p; p.setPlayConfigDetails (2, 2, sr, 512); p.prepareToPlay (sr, 512); p.loadPreset (pr); auto* r = p.apvts.getParameter (argv[2]); const float v0 = r->getValue(), v1 = v0 > 0.5f ? v0 - 0.3f : v0 + 0.3f;
            if (mode == 1) r->setValueNotifyingHost (v1); int blk = 0;
            return run (p, [&] (int k) { if (mode == 2 && k == 512 * 188) r->setValueNotifyingHost (v1); (void) blk; return 0.5f * (float) std::sin (2 * juce::MathConstants<double>::pi * 999.76 * k / sr); }, 512 * 400); };
        auto a = go (1), b = go (std::getenv ("JSAME") ? 1 : 2);
        juce::dsp::FFT fft (11); auto c0 = go (0);
        auto nh = [&] (const std::vector<float>& x, size_t at, double& tot) { std::vector<float> w (4096, 0.0f); for (int i = 0; i < 2048; ++i) w[(size_t) i] = x[at + (size_t) i] * (0.5f - 0.5f * std::cos (6.2831853f * i / 2047));
            fft.performFrequencyOnlyForwardTransform (w.data()); double s2 = 0; tot = 0; for (int k = 1; k < 1024; ++k) { const double fq = k * sr / 2048; if (fq < 5000 || fq > 20000) continue; tot += (double) w[(size_t) k] * w[(size_t) k]; const double hn = fq / 999.76; if (std::abs (hn - std::round (hn)) * 999.76 < 120) continue; s2 += (double) w[(size_t) k] * w[(size_t) k]; } return s2; };
        for (int off : { -8192, -2048, -1024, 0, 1024, 4096, 16384 }) { double t1, t2, t3; const size_t at = (size_t) (512 * 188 + off);
            std::printf ("off %6d: nonarm salto %.1f  fermo-arrivo %.1f  fermo-partenza %.1f   (tot %.1f %.1f %.1f)\n", off, 10 * std::log10 (nh (b, at, t1) + 1e-20), 10 * std::log10 (nh (a, at, t2) + 1e-20), 10 * std::log10 (nh (c0, at, t3) + 1e-20), 10 * std::log10 (t1), 10 * std::log10 (t2), 10 * std::log10 (t3)); }
        std::printf ("\n"); return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "lab")
    {   // come THE_LAB: latenza vera, buffer, aliasing, click per manopola, 48/96 kHz, CPU. JPRESET = indice del preset
        const int pr = juce::SystemStats::getEnvironmentVariable ("JPRESET", "1").getIntValue();
        auto mk = [&] (double rate, int blk = 512) { auto p = std::make_unique<Processor>(); p->setPlayConfigDetails (2, 2, rate, blk); p->prepareToPlay (rate, blk); p->loadPreset (pr); if (auto* js = std::getenv ("JSET")) for (auto kv : juce::StringArray::fromTokens (js, ",", "")) setP (*p, kv.upToFirstOccurrenceOf ("=", false, false).toRawUTF8(), kv.fromFirstOccurrenceOf ("=", false, false).getFloatValue()); return p; };
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
        std::printf ("preset: %s\n", juce::String (presets()[(size_t) pr].name).toRawUTF8());
        {   // latenza
            auto p = mk (sr); juce::Random rr (9); std::vector<float> in (48000 * 3); for (auto& x : in) x = 0.02f * (rr.nextFloat() * 2 - 1);
            auto o = render (*p, (int) in.size(), [&] (int k) { return in[(size_t) k]; });
            double best = -1e30; int bl = 0; std::vector<double> cc (4000);
            for (int l = 0; l < 4000; ++l) { double c = 0; for (int i = 24000; i < (int) in.size() - 4000; ++i) c += (double) in[(size_t) i] * o[(size_t) (i + l)]; cc[(size_t) l] = c; if (c > best) { best = c; bl = l; } }
            double fr = bl; if (bl > 0 && bl < 3999) { const double a = cc[(size_t) bl - 1], b2 = cc[(size_t) bl], c = cc[(size_t) bl + 1]; fr = bl + 0.5 * (a - c) / (a - 2 * b2 + c); }
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
    if (argc > 1 && juce::String (argv[1]) == "snap")
    {
        auto p = fresh();
        if (auto* pn = std::getenv ("JPRESET")) { for (int i = 0; i < (int) presets().size(); ++i) if (juce::String (presets()[(size_t) i].name).contains (pn)) { p->loadPreset (i); break; } }
        else { setP (*p, "pitch", -5); setP (*p, "drive", 0.45f); setP (*p, "model", VALVOLA); }
        juce::Random r (1);
        run (*p, [&] (int i) { const double t = i / sr; return (float) (0.3 * std::sin (2 * 3.14159 * 180 * t) + 0.15 * std::sin (2 * 3.14159 * 360 * t) + 0.05 * (r.nextFloat() - 0.5f)); }, (int) sr);
        Editor ed (*p);
        if (auto* jsc = std::getenv ("JSCALE")) ed.scaler.set ((float) std::atof (jsc));
        ed.tickForTest();
        if (std::getenv ("JBROWSE")) ed.browser.open();
        auto img = ed.createComponentSnapshot (ed.getLocalBounds(), true, 1.0f);
        juce::File f (argc > 2 ? argv[2] : "/tmp/alter.png"); f.deleteFile(); juce::FileOutputStream os (f); juce::PNGImageFormat().writeImageToStream (img, os);
        return 0;
    }
    if (std::getenv ("JBENCH"))
    {
        for (int m = 0; m < kNumModels; ++m) for (int mb = 0; mb < 2; ++mb)
        {
            auto p = fresh(); setP (*p, "model", (float) m); setP (*p, "drive", 0.5f); setP (*p, "multi", (float) mb); juce::Random r (2);
            const auto t0 = juce::Time::getMillisecondCounterHiRes();
            run (*p, [&] (int) { return r.nextFloat() * 0.2f - 0.1f; }, (int) sr * 5);
            std::printf ("%-14s %s %.1f%%\n", modelInfo (m).name, mb ? "3B" : "1B", (juce::Time::getMillisecondCounterHiRes() - t0) / 50.0);
        }
        return 0;
    }
    // 1) latenza e neutralita'
    {
        auto p = fresh();
        auto y = run (*p, [] (int i) { return i == 1000 ? 0.5f : 0.0f; }, 20000);
        int pk = 0; for (int i = 0; i < (int) y.size(); ++i) if (std::abs (y[(size_t) i]) > std::abs (y[(size_t) pk])) pk = i;
        std::printf ("latenza dichiarata %d, misurata %d (picco %.3f)\n", p->getLatencySamples(), pk - 1000, y[(size_t) pk]);
        auto q = fresh(); const int L = q->getLatencySamples(); juce::Random r (5); std::vector<float> in;
        auto z = run (*q, [&] (int) { const float x = r.nextFloat() - 0.5f; in.push_back (x); return x; }, 48000);
        double e = 0; for (int i = L; i < 48000; ++i) e += std::pow (z[(size_t) i] - in[(size_t) (i - L)], 2.0);
        std::printf ("neutro: differenza %.1f dB\n", 10 * std::log10 (e / 48000 + 1e-30));
        setP (*q, "drive", 0.5f); setP (*q, "mix", 0.5f);
        auto w = run (*q, [] (int i) { return (float) (0.3 * std::sin (2 * 3.14159 * 9000.0 * i / 48000.0)); }, 24000);
        std::printf ("mix 50%% drive 50%% a 9 kHz: rms %.4f (circa 0.21 se non c'e' pettine)\n", rms (w, 12000, 24000));
    }
    // 2) pitch su sinusoide
    for (float st : { 7.0f, -12.0f, 3.0f, 12.0f })
    {
        auto p = fresh(); setP (*p, "pitch", st);
        auto y = run (*p, [] (int i) { return (float) (0.3 * std::sin (2 * 3.14159265 * 220.0 * i / 48000.0)); }, 96000);
        // purezza: energia vicino alla nota attesa contro il resto
        const double fe = 220.0 * std::pow (2.0, st / 12.0); double ein = 0, eout = 0;
        for (double f = 40; f < 8000; f += 5) { double re = 0, im = 0; for (int i = 40000; i < 56384; ++i) { const double w = 0.5 - 0.5 * std::cos (6.283185307 * (i - 40000) / 16384.0); re += y[(size_t) i] * w * std::cos (6.283185307 * f * i / sr); im += y[(size_t) i] * w * std::sin (6.283185307 * f * i / sr); } (std::abs (f - fe) < 15 ? ein : eout) += re * re + im * im; }
        std::printf ("pitch %+5.1f: %.2f Hz (atteso %.2f)  rms %.3f  purezza %.1f dB\n", st, peakHz (y, 40000, sr), fe, rms (y, 40000, 90000), 10 * std::log10 (ein / (eout + 1e-30)));
    }
    {
        auto p = fresh(); setP (*p, "formant", 5);
        auto y = run (*p, [] (int i) { double t = i / 48000.0, s = 0; for (int h = 1; h < 20; ++h) s += std::sin (2 * 3.14159265 * 150 * h * t) / h; return (float) (0.2 * s); }, 96000);
        std::printf ("formant +5: fondamentale %.2f Hz (atteso 150)\n", peakHz (y, 40000, sr, 100));
    }
    {
        auto p = fresh(); setP (*p, "mode", QUANTIZZA); setP (*p, "key", 9); setP (*p, "scale", 2);
        auto y = run (*p, [] (int i) { double t = i / 48000.0, s = 0; for (int h = 1; h < 8; ++h) s += std::sin (2 * 3.14159265 * 227 * h * t) / h; return (float) (0.2 * s); }, 96000);
        std::printf ("quantizza La min, ingresso 227 Hz: %.2f Hz (atteso 220)\n", peakHz (y, 40000, sr, 150));
        auto q = fresh(); setP (*q, "mode", ROBOT); setP (*q, "key", 0);
        auto z = run (*q, [] (int i) { double t = i / 48000.0, s = 0; for (int h = 1; h < 8; ++h) s += std::sin (2 * 3.14159265 * 300 * h * t) / h; return (float) (0.2 * s); }, 96000);
        std::printf ("robot Do, ingresso 300 Hz: %.2f Hz (atteso 261.63)\n", peakHz (z, 40000, sr, 150));
    }
    // 3) modelli: NaN, guadagno automatico, aliasing (sinusoide 5 kHz: energia fuori dalle armoniche)
    for (int m = 0; m < kNumModels; ++m)
    {
        for (int multi = 0; multi < 2; ++multi)
        {
            auto p = fresh(); setP (*p, "model", (float) m); setP (*p, "drive", 0.7f); setP (*p, "multi", (float) multi);
            const double F0 = std::getenv ("JF") ? std::atof (std::getenv ("JF")) : 5000.0; auto y = run (*p, [F0] (int i) { return (float) (0.4 * std::sin (2 * 3.14159265 * F0 * i / 48000.0)); }, 48000 + 16384);
            bool ok = true; for (auto v : y) if (! std::isfinite (v)) ok = false;
            const int NN = 16384; double eh = 0, ea = 0;
            for (int bin = 2; bin < 1600; ++bin)
            {
                const double f = bin * 15.0; double re = 0, im = 0;
                for (int n = 0; n < NN; n += 1) { const double w = 0.5 - 0.5 * std::cos (6.283185307 * n / NN), x = y[(size_t) (40000 + n)] * w; re += x * std::cos (6.283185307 * f * n / sr); im += x * std::sin (6.283185307 * f * n / sr); }
                double fr = std::fmod (f, F0); fr = std::min (fr, F0 - fr); (fr < 40 ? eh : ea) += re * re + im * im;
            }
            std::printf ("%-14s %s  rms %.3f  %s  auto %+5.1f dB  aliasing %5.1f dB\n", modelInfo (m).name, multi ? "3B" : "1B", rms (y, 24000, 48000), ok ? "ok" : "NaN!", p->drive.autoDb.load(), 10 * std::log10 (eh / (ea + 1e-30)));
        }
    }
    // 4) multibanda: 60 Hz con i bassi a 0% resta pulito
    {
        auto p = fresh(); setP (*p, "model", DISTORSIONE); setP (*p, "drive", 0.9f); setP (*p, "wLo", 0); setP (*p, "x1", 300); setP (*p, "auto", 0);
        auto y = run (*p, [] (int i) { return (float) (0.4 * std::sin (2 * 3.14159265 * 60.0 * i / 48000.0)); }, 48000);
        double h2 = 0; { double re = 0, im = 0; for (int i = 24000; i < 48000; ++i) { re += y[(size_t) i] * std::cos (6.283185307 * 180 * i / sr); im += y[(size_t) i] * std::sin (6.283185307 * 180 * i / sr); } h2 = std::sqrt (re * re + im * im) / 12000; }
        std::printf ("bassi 0%%: 60 Hz rms %.3f (ingresso 0.283), 3a armonica %.4f\n", rms (y, 24000, 48000), h2);
    }
    // 5) automazione: drive e pitch che si muovono, cerca salti
    {
        auto p = fresh(); setP (*p, "model", TAPE); float maxJ = 0; double ph = 0; float prev = 0;
        juce::AudioBuffer<float> b (2, 256); juce::MidiBuffer mb;
        for (int k = 0; k < 600; ++k)
        {
            setP (*p, "drive", (float) (0.5 + 0.5 * std::sin (k * 0.05))); setP (*p, "multi", (k / 100) % 2 ? 1.0f : 0.0f); setP (*p, "tone", (float) std::sin (k * 0.03)); setP (*p, "mix", (float) (0.5 + 0.5 * std::cos (k * 0.04)));
            for (int i = 0; i < 256; ++i) { const float x = 0.2f * (float) std::sin (ph); ph += 2 * 3.14159265 * 100 / sr; b.setSample (0, i, x); b.setSample (1, i, x); }
            p->processBlock (b, mb);
            for (int i = 0; i < 256; ++i) { const float v = b.getSample (0, i); if (k > 20) maxJ = juce::jmax (maxJ, std::abs (v - prev)); prev = v; }
        }
        std::printf ("automazione: salto massimo tra campioni %.4f (una sinusoide 100 Hz a 0.2 salta al massimo ~0.003 pulita)\n", maxJ);
    }
    // 5b) da mono a stereo (destro uguale e poi diverso): nessun salto quando si passa a calcolare due canali
    {
        auto p = fresh(); setP (*p, "pitch", -3); setP (*p, "model", AMP); setP (*p, "drive", 0.5f);
        juce::AudioBuffer<float> b (2, 512); juce::MidiBuffer mb; double ph = 0; float prev = 0, maxJ = 0, maxJs = 0;
        for (int k = 0; k < 200; ++k)
        {
            for (int i = 0; i < 512; ++i) { const float x = 0.2f * (float) std::sin (ph); ph += 2 * 3.14159265 * 150 / sr; b.setSample (0, i, x); b.setSample (1, i, k < 100 ? x : x * 0.98f); }
            p->processBlock (b, mb);
            for (int i = 0; i < 512; ++i) { const float v = b.getSample (1, i); if (k > 30) (k >= 100 && k < 102 ? maxJ : maxJs) = juce::jmax (k >= 100 && k < 102 ? maxJ : maxJs, std::abs (v - prev)); prev = v; }
        }
        std::printf ("mono -> stereo: salto nel passaggio %.4f, altrove %.4f\n", maxJ, maxJs);
        auto q = fresh(); setP (*q, "pitch", -5); setP (*q, "formant", 2); setP (*q, "model", TAPE); setP (*q, "drive", 0.5f); juce::Random r (2);
        juce::AudioBuffer<float> bb (2, 512); const auto t0 = juce::Time::getMillisecondCounterHiRes();
        for (int k = 0; k < 940; ++k) { for (int i = 0; i < 512; ++i) { bb.setSample (0, i, r.nextFloat() * 0.2f - 0.1f); bb.setSample (1, i, r.nextFloat() * 0.2f - 0.1f); } q->processBlock (bb, mb); }
        std::printf ("CPU stereo vero, pitch + formante + TAPE 3 bande: %.1f%%\n", (juce::Time::getMillisecondCounterHiRes() - t0) / 100.0);
    }
    // 6) CPU
    for (int c = 0; c < 3; ++c)
    {
        auto p = fresh(); if (c >= 1) { setP (*p, "pitch", -5); setP (*p, "formant", 2); } if (c == 2) { setP (*p, "model", AMP); setP (*p, "drive", 0.5f); }
        juce::Random r (2);
        const auto t0 = juce::Time::getMillisecondCounterHiRes();
        run (*p, [&] (int) { return r.nextFloat() * 0.2f - 0.1f; }, (int) sr * 10);
        const auto ms = juce::Time::getMillisecondCounterHiRes() - t0;
        std::printf ("CPU %s: %.1f%% di un core\n", c == 0 ? "a riposo" : c == 1 ? "pitch" : "pitch + drive 3 bande", ms / 100.0);
    }
    // 7) preset
    for (int i = 0; i < (int) presets().size(); ++i)
    {
        auto p = fresh(); p->loadPreset (i); juce::Random r (3);
        auto y = run (*p, [&] (int k) { return (float) (0.25 * std::sin (2 * 3.14159265 * 196.0 * k / 48000.0) + 0.02 * (r.nextFloat() - 0.5)); }, 48000);
        bool ok = true; float pk = 0; for (auto v : y) { if (! std::isfinite (v)) ok = false; pk = juce::jmax (pk, std::abs (v)); }
        std::printf ("%-46s ok=%d picco %.2f rms %.3f\n", presets()[(size_t) i].name, ok, pk, rms (y, 24000, 48000));
    }
    // 8) stato vecchio (4 bande): modello e drive della banda piu' spinta
    {
        auto p = fresh(); juce::ValueTree t ("JAD");
        auto add = [&t] (const char* id, float v) { juce::ValueTree c ("PARAM"); c.setProperty ("id", id, nullptr); c.setProperty ("value", v, nullptr); t.appendChild (c, nullptr); };
        add ("bands", 2); add ("b1_model", 0); add ("b1_drive", 0.1f); add ("b2_model", 6); add ("b2_drive", 0.5f); add ("pitch", -3);
        juce::MemoryBlock mbk; p->copyXmlToBinary (*t.createXml(), mbk); p->setStateInformation (mbk.getData(), (int) mbk.getSize());
        std::printf ("stato vecchio: modello %d drive %.2f multi %.0f pitch %.1f\n", juce::roundToInt (p->value ("model")), p->value ("drive"), p->value ("multi"), p->value ("pitch"));
    }
    return 0;
}
