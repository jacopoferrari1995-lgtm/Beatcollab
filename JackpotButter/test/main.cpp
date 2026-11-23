#include <cstdlib>
#include "TestLab.h"
#include "../Source/ButterProcessor.h"
#include "../Source/ButterEditor.h"
using namespace jb;
int main (int argc, char** argv)
{
    if (argc > 1 && juce::String (argv[1]) == "torture")
    {   // caccia all'esplosione: preset a caso, comandi che saltano, blocchi irregolari, segnali da live (colpi, sinusoidi, silenzio, picchi)
        const double sr = std::getenv ("JSR") ? std::atof (std::getenv ("JSR")) : 48000; const int runs = std::getenv ("JRUNS") ? std::atoi (std::getenv ("JRUNS")) : 40;
        const int seed0 = std::getenv ("JSEED") ? std::atoi (std::getenv ("JSEED")) : 1;
        int bad = 0;
        for (int run = 0; run < runs; ++run)
        {
            juce::Random r (seed0 * 1000 + run);
            auto p = std::make_unique<Processor>(); p->setPlayConfigDetails (2, 2, sr, 1024); p->prepareToPlay (sr, 1024);
            const int np = p->getNumPrograms(); const int pr = (run * 7 + seed0 * 13) % np; p->loadPreset (pr);
            auto& params = p->getParameters();
            juce::AudioBuffer<float> buf (2, 1024); juce::MidiBuffer mb;
            const int total = (int) (sr * (std::getenv ("JLEN") ? std::atof (std::getenv ("JLEN")) : 40.0)); int pos = 0; float worst = 0; juce::String last; double ph = 0; float env = 0;
            int nextMove = 0, kind = 0, silStart = 0; juce::StringArray hist;
            while (pos < total)
            {
                const int n = 1 + r.nextInt (1024);
                if (pos >= nextMove)
                {   // ogni ~0.3 s: 1-4 comandi a valori a caso (anche estremi), a volte un preset nuovo
                    nextMove = pos + (int) (sr * (0.05 + r.nextFloat() * 0.5));
                    if (r.nextFloat() < 0.03f) { const int q = r.nextInt (np); p->loadPreset (q); last = "preset " + juce::String (q); }
                    for (int k = 0, m = 1 + r.nextInt (4); k < m; ++k)
                    {
                        auto* q = params[r.nextInt (params.size())]; const float v = r.nextFloat() < 0.3f ? (r.nextBool() ? 0.0f : 1.0f) : r.nextFloat();
                        q->setValueNotifyingHost (v); last = q->getName (40) + "=" + juce::String (v, 2); hist.add (juce::String ((pos) / sr, 2) + "s " + last); if (hist.size() > 25) hist.remove (0);
                    }
                    const int nk = r.nextInt (5); if (nk != kind || nk != 2) silStart = pos; kind = nk;
                    if (kind == 2) nextMove = pos + (int) (sr * 4);   // silenzio lungo, comandi fermi: deve spegnersi
                }
                for (int i = 0; i < n; ++i)
                {
                    float x = 0; const double t = (pos + i) / sr;
                    switch (kind)
                    {
                        case 0: x = (r.nextFloat() * 2 - 1) * (std::fmod (t, 0.25) < 0.02 ? 0.9f : 0.05f); break;   // colpi
                        case 1: ph += 55.0 / sr; x = 0.8f * (float) std::sin (juce::MathConstants<double>::twoPi * ph); break; // 808
                        case 2: x = 0; break;                                                                          // silenzio
                        case 3: x = (r.nextFloat() * 2 - 1) * 0.99f; break;                                            // rumore pieno
                        default: ph += 3000.0 / sr; x = 0.5f * (float) std::sin (juce::MathConstants<double>::twoPi * ph); break;
                    }
                    buf.setSample (0, i, x); buf.setSample (1, i, kind == 3 ? -x : x);
                }
                juce::AudioBuffer<float> view (buf.getArrayOfWritePointers(), 2, n);
                p->processBlock (view, mb);
                for (int c = 0; c < 2; ++c) for (int i = 0; i < n; ++i)
                {
                    const float y = view.getSample (c, i);
                    const bool selfOsc = kind == 2 && (pos + i - silStart) > (int) (sr * 3.5) && std::abs (y) > 0.3f;
                    if (! std::isfinite (y) || std::abs (y) > 2.0f || selfOsc)
                    {
                        std::printf ("ESPLOSO%s run %d preset %d a %.2f s: %s (ultimo: %s)\n", selfOsc ? " (si autoalimenta nel silenzio)" : "", run, pr, (pos + i) / sr, std::isfinite (y) ? juce::String (y).toRawUTF8() : "NaN/inf", last.toRawUTF8());
                        ++bad; pos = total; c = 2; if (std::getenv ("JV")) { std::printf ("%s\n", hist.joinIntoString ("\n").toRawUTF8()); for (auto* q : params) if (std::abs (q->getValue() - q->getDefaultValue()) > 1e-3f) std::printf ("  %s = %s\n", q->getName (40).toRawUTF8(), q->getCurrentValueAsText().toRawUTF8()); } break;
                    }
                    worst = std::max (worst, std::abs (y));
                }
                pos += n;
            }
            if (std::getenv ("JV") || p->panics.load() > 0) std::printf ("run %d preset %d picco %.2f protezioni %d\n", run, pr, worst, p->panics.load());
        }
        std::printf ("esplosioni: %d su %d\n", bad, runs);
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "loud")
    {   // rumore rosa a picco -1 e 20 dB sotto: guadagno RMS uscita/ingresso
        const double sr = 48000; const int pr = std::getenv ("JPRESET") ? std::atoi (std::getenv ("JPRESET")) : -1;
        for (float att : { 1.0f, 0.1f })
        {
            juce::Random r (3); std::vector<float> in ((size_t) sr * 6); float b0 = 0, b1 = 0, b2 = 0, pk = 0;
            for (auto& v : in) { const float w = r.nextFloat() * 2 - 1; b0 = 0.99765f * b0 + w * 0.099f; b1 = 0.963f * b1 + w * 0.2965f; b2 = 0.57f * b2 + w * 1.0527f; v = b0 + b1 + b2 + w * 0.1848f; pk = std::max (pk, std::abs (v)); }
            for (auto& v : in) v *= 0.89f / pk * att;
            auto p = std::make_unique<Processor>(); p->setPlayConfigDetails (2, 2, sr, 512); p->prepareToPlay (sr, 512); if (pr >= 0) p->loadPreset (pr);
            auto o = testlab::render (*p, (int) in.size(), [&] (int k) { return in[(size_t) k]; }, 512, false);
            double ei = 0, eo = 0; for (size_t k = (size_t) sr * 2; k < in.size(); ++k) { ei += (double) in[k] * in[k]; eo += (double) o[k] * o[k]; }
            std::printf ("ingresso %s: %+.1f dB\n", att > 0.5f ? "pieno " : "-20 dB", 10 * std::log10 (eo / ei));
        }
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "lab")
    {   // come THE_LAB: stato iniziale (o JPRESET), JSET=id=valore,...
        const int pr = std::getenv ("JPRESET") ? std::atoi (std::getenv ("JPRESET")) : -1;
        testlab::run ([&] (double rate, int blk) -> std::unique_ptr<juce::AudioProcessor>
        {
            auto p = std::make_unique<Processor>(); p->setPlayConfigDetails (2, 2, rate, blk); p->prepareToPlay (rate, blk);
            if (pr >= 0) p->loadPreset (pr);
            if (auto* js = std::getenv ("JSET")) for (auto kv : juce::StringArray::fromTokens (js, ",", "")) { auto* r = p->apvts.getParameter (kv.upToFirstOccurrenceOf ("=", false, false)); r->setValueNotifyingHost (r->convertTo0to1 (kv.fromFirstOccurrenceOf ("=", false, false).getFloatValue())); }
            return p;
        }, std::getenv ("JNOCLICK") == nullptr);
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "eqcurve")
    {
        jmx::BandSet B; B.on = true; B.type = jmx::BT_HSHELF; B.f = 12000; B.g = 1.5f; B.q = 0.7f; jmx::BQ f[8]; const int n = jmx::designBand (B, 48000, B.g, f);
        for (float fr = 3000; fr < 20000; fr *= 1.01f) { double d = 0; for (int q = 0; q < n; ++q) d += f[q].magDb (fr, 48000); if (d > 1.6 || d < -0.1) std::printf ("%.0f Hz: %.2f dB\n", fr, d); }
        std::printf ("b0 %f b1 %f b2 %f a1 %f a2 %f\n", f[0].b0, f[0].b1, f[0].b2, f[0].a1, f[0].a2);
        jmx::BandSet A; A.on = true; A.type = jmx::BT_LSHELF; A.f = 60; A.g = 2; A.q = 1.1f; jmx::BQ fa[8]; const int na = jmx::designBand (A, 48000, A.g, fa);
        double prev = 0; for (int x = 0; x < 1200; x += 2) { const float fr = 20.0f * std::pow (1000.0f, x / 1200.0f); double d = 0; for (int q = 0; q < na; ++q) d += fa[q].magDb (fr, 48000); for (int q = 0; q < n; ++q) d += f[q].magDb (fr, 48000); if (x > 0 && std::abs (d - prev) > 0.3) std::printf ("salto a %.0f Hz: %.2f -> %.2f\n", fr, prev, d); prev = d; }
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "alias")
    {   // tono acuto attraverso il rack (JSET=id=v,...): spur non armonico peggiore (aliasing)
        juce::ScopedJuceInitialiser_GUI gx; const double sr = 48000; const int N = 65536;
        for (float amp : { 0.25f, 0.5f }) for (int bin : { 8603, 13653 })
        {
            Processor q; q.prepareToPlay (sr, 512);
            if (auto* js = std::getenv ("JSET")) for (auto kv : juce::StringArray::fromTokens (js, ",", "")) { auto* r = q.apvts.getParameter (kv.upToFirstOccurrenceOf ("=", false, false)); r->setValueNotifyingHost (r->convertTo0to1 (kv.fromFirstOccurrenceOf ("=", false, false).getFloatValue())); }
            const double ff = bin * sr / N; std::vector<float> o; juce::AudioBuffer<float> b (2, 512); juce::MidiBuffer m;
            for (int off = 0; off < N * 2; off += 512) { for (int i = 0; i < 512; ++i) { const float x = amp * (float) std::sin (2 * 3.14159265358979 * ff * (off + i) / sr); b.setSample (0, i, x); b.setSample (1, i, x); } q.processBlock (b, m); for (int i = 0; i < 512; ++i) o.push_back (b.getSample (0, i)); }
            juce::dsp::FFT fft (16); std::vector<float> a ((size_t) 2 * N, 0); for (int i = 0; i < N; ++i) { const double w = 6.283185307179586 * i / N; a[(size_t) i] = o[(size_t) (N + i)] * (float) (0.35875 - 0.48829 * std::cos (w) + 0.14128 * std::cos (2 * w) - 0.01168 * std::cos (3 * w)); }
            fft.performFrequencyOnlyForwardTransform (a.data()); float ref0 = 0; for (int k = bin - 5; k <= bin + 5; ++k) ref0 = std::max (ref0, a[(size_t) k]);
            float worst = 0; double wf = 0; double hs = 0;
            for (int k = 40; k < N / 2; ++k) { bool h = false; for (int mm = 1; mm < 8; ++mm) if (std::abs (k - bin * mm) <= 14) { h = true; if (mm > 1) hs += (double) a[(size_t) k] * a[(size_t) k]; } if (std::abs (k - bin) < bin / 20) h = true; if (! h && a[(size_t) k] > worst) { worst = a[(size_t) k]; wf = k * sr / N; } }
            std::printf ("amp %.2f  %5.0f Hz: spur %6.1f dB a %5.0f Hz   armoniche %6.1f dB\n", amp, ff, juce::Decibels::gainToDecibels (worst / ref0, -200.0f), wf, juce::Decibels::gainToDecibels ((float) std::sqrt (hs) / ref0, -200.0f));
        }
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "knobs")
    {   // come THE_LAB: tono a 1 kHz, ogni manopola continua salta del 30%; energia sopra 5 kHz della differenza contro il picco del tono
        juce::ScopedJuceInitialiser_GUI gx; const double sr = 48000;
        Processor probe; const int np = probe.getParameters().size();
        for (int pi = 0; pi < np; ++pi)
        {
            auto* pp = dynamic_cast<juce::AudioParameterFloat*> (probe.getParameters()[pi]); if (pp == nullptr) continue;
            auto render = [&] (int mode)   // 0 = fermo su v0, 1 = fermo su v1, 2 = salti
            {
                Processor q; q.prepareToPlay (sr, 512); if (auto* pr = std::getenv ("JPR")) q.loadPreset (std::atoi (pr));
                auto* r = q.getParameters()[pi]; const float v0 = r->getValue(), v1 = v0 > 0.5f ? v0 - 0.3f : v0 + 0.3f;
                if (mode == 1) r->setValueNotifyingHost (v1);
                std::vector<float> o; juce::AudioBuffer<float> b (2, 512); juce::MidiBuffer m; long n = 0;
                for (int blk = 0; blk < 200; ++blk)
                {
                    if (mode == 2 && blk >= 100 && (blk - 100) % 20 == 0 && blk < 180) r->setValueNotifyingHost (((blk - 100) / 20) % 2 ? v0 : v1);
                    for (int i = 0; i < 512; ++i, ++n) { const float x = 0.5f * (float) std::sin (2 * 3.14159265358979 * 999.76 * n / sr); b.setSample (0, i, x); b.setSample (1, i, x); }
                    q.processBlock (b, m); if (blk >= 96) for (int i = 0; i < 512; ++i) o.push_back (b.getSample (0, i));
                }
                return o;
            };
            const auto a = render (0), b1 = render (1), bb = render (2);
            // per ogni salto: finestra di 2048 campioni dal salto; energia NON armonica fra 5 e 20 kHz (le armoniche del tono si muovono di proposito)
            // nella versione col salto, meno quella ferma sul valore di arrivo
            double e = 0, pk = 0, ch = 0; for (size_t i = 0; i < a.size(); ++i) { pk = std::max (pk, (double) std::abs (a[i])); ch = std::max (ch, (double) std::abs (bb[i] - a[i])); }
            juce::dsp::FFT fft (11);
            auto nonHarm = [&] (const std::vector<float>& x, size_t at)
            {
                std::vector<float> w (4096, 0.0f); for (int i = 0; i < 2048; ++i) w[(size_t) i] = x[at + (size_t) i] * (0.5f - 0.5f * std::cos (6.2831853f * i / 2047));
                fft.performFrequencyOnlyForwardTransform (w.data()); double s = 0;
                for (int k = 1; k < 1024; ++k) { const double fq = k * sr / 2048; if (fq < 5000 || fq > 20000) continue; const double hn = fq / 999.76; if (std::abs (hn - std::round (hn)) * 999.76 < 120) continue; s += (double) w[(size_t) k] * w[(size_t) k]; }
                return s;
            };
            for (int j = 0; j < 4; ++j)
            {
                const size_t at = (size_t) (4 + j * 20) * 512 - 1024;   // il salto al centro della finestra
                const auto& ref = j % 2 == 0 ? b1 : a;
                e = std::max (e, nonHarm (bb, at) - nonHarm (ref, at));
            }
            e = std::sqrt (std::max (0.0, e)) / 1024.0 * 2.0;   // ampiezza circa, contro il picco del tono
            if (ch < 1e-6) continue;
            const double db = 20 * std::log10 (e / (pk + 1e-12) + 1e-12);
            if (db > -45 || std::getenv ("JALL")) std::printf ("%-24s %6.1f dB%s\n", pp->getName (40).toRawUTF8(), db, db > -35 ? "  CLICK" : "");
        }
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "lat")
    {   // ritardo vero (correlazione col rumore) contro quello dichiarato; JPR = preset, JSET come sopra; JBUF = blocchi irregolari
        juce::ScopedJuceInitialiser_GUI gx; const double sr = 48000; const int N = 48000 * 3;
        auto render = [&] (bool uneven, std::vector<float>& in, std::vector<float>& out, int& rep)
        {
            Processor q; q.prepareToPlay (sr, 512);
            if (auto* pr = std::getenv ("JPR")) q.loadPreset (std::atoi (pr));
            if (auto* js = std::getenv ("JSET")) for (auto kv : juce::StringArray::fromTokens (js, ",", "")) { auto* r = q.apvts.getParameter (kv.upToFirstOccurrenceOf ("=", false, false)); r->setValueNotifyingHost (r->convertTo0to1 (kv.fromFirstOccurrenceOf ("=", false, false).getFloatValue())); }
            q.prepareToPlay (sr, 512); rep = q.getLatencySamples();
            juce::Random rnd (7); in.resize ((size_t) N); out.resize ((size_t) N);
            for (int i = 0; i < N; ++i) in[(size_t) i] = 0.05f * (rnd.nextFloat() * 2 - 1);
            juce::MidiBuffer m; juce::Random rb (3);
            for (int off = 0; off < N; )
            {
                const int n = uneven ? juce::jmin (N - off, 1 + rb.nextInt (512)) : juce::jmin (512, N - off);
                juce::AudioBuffer<float> b (2, n); for (int i = 0; i < n; ++i) { b.setSample (0, i, in[(size_t) (off + i)]); b.setSample (1, i, in[(size_t) (off + i)]); }
                q.processBlock (b, m); for (int i = 0; i < n; ++i) out[(size_t) (off + i)] = b.getSample (0, i);
                off += n;
            }
        };
        std::vector<float> in, out, in2, out2; int rep = 0; render (false, in, out, rep);
        double best = -1e30; int bl = 0; std::vector<double> cc (600);
        for (int l = 0; l < 600; ++l) { double c = 0; for (int i = 24000; i < N - 600; ++i) c += (double) in[(size_t) i] * out[(size_t) (i + l)]; cc[(size_t) l] = c; if (c > best) { best = c; bl = l; } }
        double fr = bl; if (bl > 0 && bl < 599) { const double a = cc[(size_t) bl - 1], b = cc[(size_t) bl], c = cc[(size_t) bl + 1]; fr = bl + 0.5 * (a - c) / (a - 2 * b + c); }
        render (true, in2, out2, rep);
        double e = 0, d = 0; for (int i = 24000; i < N; ++i) { e += (double) out[(size_t) i] * out[(size_t) i]; const double x = out[(size_t) i] - out2[(size_t) i]; d += x * x; }
        std::printf ("latenza dichiarata %d, misurata %.2f  |  blocchi irregolari vs 512: %.1f dB\n", rep, fr, 10 * std::log10 (d / (e + 1e-30) + 1e-30));
        return 0;
    }
    if (argc > 2 && juce::String (argv[1]) == "snap")
    {
        if (auto* t = std::getenv ("JTHEME")) skin::cur() = std::atoi (t);
        juce::ScopedJuceInitialiser_GUI gui3;
        jb::Processor p; p.prepareToPlay (48000, 480);
        const char* pn = std::getenv ("JPRESET") ? std::getenv ("JPRESET") : "Punch moderno";
        for (int i = 0; i < p.getNumPrograms(); ++i) if (p.getProgramName (i).contains (pn)) p.setCurrentProgram (i);
        jb::Editor ed (p);
        if (auto* jsc = std::getenv ("JSCALE")) ed.scaler.set ((float) std::atof (jsc));
        juce::AudioBuffer<float> b (2, 480); juce::MidiBuffer m; juce::Random r (5);
        for (int k = 0; k < 100 * 6; ++k)
        {
            for (int i = 0; i < 480; ++i)
            {
                const long n = (long) k * 480 + i; const double t = n / 48000.0, bt = std::fmod (t * 2.2, 1.0), st = std::fmod (t * 2.2 + 0.5, 1.0);
                const float kick = (float) (std::sin (2 * 3.14159 * (50 + 120 * std::exp (-bt * 30)) * bt) * std::exp (-bt * 6)) * 0.6f;
                const float sn = (r.nextFloat() * 2 - 1) * (float) std::exp (-st * 18) * 0.35f;
                const float hat = (r.nextFloat() * 2 - 1) * (float) std::exp (-std::fmod (t * 8.8, 1.0) * 40) * 0.12f;
                b.setSample (0, i, kick + sn + hat); b.setSample (1, i, kick + sn * 0.8f + hat * 1.2f);
            }
            p.processBlock (b, m);
            if (k % 3 == 0) ed.eq->updateSpectrum();
        }
        if (std::getenv ("JPERF"))
        {   // un fotogramma tipico: i 7 display dei moduli + analizzatore EQ, come fa il timer
            juce::Image im (juce::Image::RGB, ed.getWidth() * 2, ed.getHeight() * 2, true);
            auto t0 = juce::Time::getMillisecondCounterHiRes();
            for (int i = 0; i < 10; ++i) { juce::Graphics g (im); g.addTransform (juce::AffineTransform::scale (2.0f)); ed.paintEntireComponent (g, true); }
            auto t1 = juce::Time::getMillisecondCounterHiRes();
            for (int i = 0; i < 10; ++i)
            {
                for (auto* m : ed.modules) { juce::Graphics g (im); g.addTransform (juce::AffineTransform::scale (2.0f)); g.reduceClipRegion (m->getBounds().withHeight (125)); ed.paintEntireComponent (g, true); }
                juce::Graphics g (im); g.addTransform (juce::AffineTransform::scale (2.0f)); g.reduceClipRegion (ed.eq->getBounds().withHeight (130)); ed.paintEntireComponent (g, true);
            }
            auto t2 = juce::Time::getMillisecondCounterHiRes();
            std::printf ("ridisegno completo (retina) %.1f ms   fotogramma animato %.1f ms\n", (t1 - t0) / 10, (t2 - t1) / 10);
        }
        if (std::getenv ("JBROWSE")) ed.browser.open();
        if (std::getenv ("JEQBIG")) { ed.eq->setBig (true); for (int k = 0; k < 30; ++k) ed.eq->updateSpectrum(); }
        if (std::getenv ("JEQDUMP"))
        {
            auto& V = ed.eq->view;
            for (int k = 0; k < kQBands; ++k) if (V.val (k, "on") > 0.5f) std::printf ("banda %d tipo %.0f f %.1f g %.2f q %.2f ch %.0f slope %.0f dyn %.0f\n", k, V.val (k, "type"), V.val (k, "f"), V.val (k, "g"), V.val (k, "q"), V.val (k, "ch"), V.val (k, "slope"), V.val (k, "dyn"));
        }
        auto img = ed.createComponentSnapshot (ed.getLocalBounds(), true, 1.0f);
        juce::File f (argv[2]); f.deleteFile(); juce::FileOutputStream os (f); juce::PNGImageFormat().writeImageToStream (img, os);
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "verb")
    {
        juce::ScopedJuceInitialiser_GUI gui2;
        for (int t = 0; t < 4; ++t) for (float d : { 0.0f, 0.5f, 1.0f })
        {
            jb::Processor p; p.prepareToPlay (48000, 512);
            auto set = [&p] (const char* id, float v) { auto* r = p.apvts.getParameter (id); r->setValueNotifyingHost (r->convertTo0to1 (v)); };
            set ("u_spType", (float) t); set ("u_spAmt", 1); set ("u_spDecay", d); set ("u_spPre", 0);
            juce::AudioBuffer<float> b (2, 512); juce::MidiBuffer m; double e0 = 0; int t60 = -1; bool nan = false; double first = -1;
            for (int k = 0; k < 94 * 8; ++k)
            {
                b.clear(); if (k == 0) { b.setSample (0, 0, 1); b.setSample (1, 0, 1); }
                p.processBlock (b, m);
                double e = 0; for (int i = 0; i < 512; ++i) { const float x = b.getSample (0, i), y = b.getSample (1, i); if (! std::isfinite (x)) nan = true; e += x * x + y * y; }
                if (e > first) first = e;
                if (k > 10 && t60 < 0 && e < first * 1e-6) t60 = k;
            }
            std::printf ("tipo %d decay %.1f: RT60 ~ %.2f s nan %d\n", t, d, t60 < 0 ? -1.0 : t60 * 512 / 48000.0, (int) nan);
        }
        return 0;
    }
    juce::ScopedJuceInitialiser_GUI gui;
    {   // appena caricato: neutro
        Processor q; q.prepareToPlay (44100, 512); juce::AudioBuffer<float> b (2, 512); juce::MidiBuffer mm; double e = 0, ee = 0; juce::Random r (2);
        const int L = q.getLatencySamples(); std::vector<float> hist;
        for (int k = 0; k < 100; ++k) { for (int i = 0; i < 512; ++i) { static double ph0 = 0; ph0 += 0.1424; const float x = 0.4f * (float) (std::sin (ph0) + 0.5 * std::sin (ph0 * 3.7)) + 0.0f * r.nextFloat(); hist.push_back (x); b.setSample (0, i, x); b.setSample (1, i, x * 0.7f); } q.processBlock (b, mm);
            if (k > 4) for (int i = 0; i < 512; ++i) { const float ref = hist[(size_t) (k * 512 + i - L)]; e += std::pow (b.getSample (0, i) - ref, 2.0) + std::pow (b.getSample (1, i) - ref * 0.7f, 2.0); ee += ref * ref; } }
        std::printf ("neutro: differenza %.1f dB (latenza %d)\n", 10 * std::log10 (e / ee + 1e-30), L);
        if (std::getenv ("JLAG")) { Processor q2; q2.prepareToPlay (44100, 512); std::vector<float> o2, in2; juce::Random r2 (3);
            for (int k = 0; k < 20; ++k) { for (int i = 0; i < 512; ++i) { const float x = k == 2 && i == 0 ? 1.0f : 0.0f; in2.push_back (x); b.setSample (0, i, x); b.setSample (1, i, x); } q2.processBlock (b, mm); for (int i = 0; i < 512; ++i) o2.push_back (b.getSample (0, i)); }
            int bi = 0; for (int i = 0; i < (int) o2.size(); ++i) if (std::abs (o2[(size_t) i]) > std::abs (o2[(size_t) bi])) bi = i;
            std::printf ("impulso: picco a %d (atteso %d) valore %.3f; somma %.3f\n", bi - 1024, L, o2[(size_t) bi], [&]{ float t = 0; for (auto v : o2) t += v; return t; }()); }
    }
    {   // aliasing: sinusoide 7 kHz a 44.1k con SAT fuzz e CLIP a fondo: energia fuori dalle armoniche
        Processor q; q.prepareToPlay (44100, 512); auto set = [&q] (const char* id, float v) { auto* r = q.apvts.getParameter (id); r->setValueNotifyingHost (r->convertTo0to1 (v)); };
        set ("u_sAmt", std::getenv ("JA") ? (float) std::atof (std::getenv ("JA")) : 0.8f); set ("u_sType", std::getenv ("JT") ? (float) std::atoi (std::getenv ("JT")) : 8); set ("u_mAmt", std::getenv ("JM") ? (float) std::atof (std::getenv ("JM")) : 0.8f);
        const int NN = 16384; std::vector<float> out; juce::AudioBuffer<float> b (2, 512); juce::MidiBuffer mm; double ph = 0; const double f0 = std::getenv ("JF") ? std::atof (std::getenv ("JF")) : 7000;
        for (int k = 0; k < 60; ++k) { for (int i = 0; i < 512; ++i) { const float x = 0.5f * (float) std::sin (ph); ph += 2 * juce::MathConstants<double>::pi * f0 / 44100; b.setSample (0, i, x); b.setSample (1, i, x); } q.processBlock (b, mm); if (k >= 20) for (int i = 0; i < 512; ++i) out.push_back (b.getSample (0, i)); }
        double eh = 0, ea = 0;
        for (int bin = 1; bin < 2000; ++bin)
        {   const double f = bin * 11.025; double re = 0, im = 0;
            for (int n = 0; n < NN; ++n) { const double w = 0.5 - 0.5 * std::cos (2 * juce::MathConstants<double>::pi * n / NN); re += out[(size_t) n] * w * std::cos (2 * juce::MathConstants<double>::pi * f * n / 44100); im += out[(size_t) n] * w * std::sin (2 * juce::MathConstants<double>::pi * f * n / 44100); }
            const double pw = re * re + im * im; double fr = std::fmod (f, f0); fr = std::min (fr, f0 - fr);
            if (fr < 60) eh += pw; else ea += pw; }
        std::printf ("aliasing sat fuzz + clip: %.1f dB sotto le armoniche\n", 10 * std::log10 (eh / (ea + 1e-30)));
    }
    {   // cpu: preset pesante, 10 s a 48k
        Processor q; q.prepareToPlay (48000, 512); for (int i = 0; i < q.getNumPrograms(); ++i) if (q.getProgramName (i).contains (std::getenv ("JCPU") ? std::getenv ("JCPU") : "Fuzz phonk")) { q.loadPreset (i); break; }
        if (auto* js = std::getenv ("JSET")) for (auto kv : juce::StringArray::fromTokens (js, ",", "")) { auto* r = q.apvts.getParameter (kv.upToFirstOccurrenceOf ("=", false, false)); r->setValueNotifyingHost (r->convertTo0to1 (kv.fromFirstOccurrenceOf ("=", false, false).getFloatValue())); }
        juce::AudioBuffer<float> b (2, 512); juce::MidiBuffer mm; juce::Random r (1);
        const auto t0 = juce::Time::getMillisecondCounterHiRes();
        for (int k = 0; k < 940; ++k) { for (int i = 0; i < 512; ++i) { const float x = (r.nextFloat() - 0.5f) * 0.5f; b.setSample (0, i, x); b.setSample (1, i, x); } q.processBlock (b, mm); }
        std::printf ("cpu Fuzz phonk: %.1f%% di un core\n", (juce::Time::getMillisecondCounterHiRes() - t0) / 100.0);
    }
    {   // formante: sinusoide + rumore, pitch 0 e formante +7: niente NaN, energia simile
        Processor q; q.prepareToPlay (44100, 512); auto set = [&q] (const char* id, float v) { auto* r = q.apvts.getParameter (id); r->setValueNotifyingHost (r->convertTo0to1 (v)); };
        set ("u_shAmt", 1); set ("u_shForm", 7); juce::AudioBuffer<float> b (2, 512); juce::MidiBuffer mm; double ei = 0, eo = 0; bool nan = false; juce::Random r (4); double ph = 0;
        for (int k = 0; k < 300; ++k) { for (int i = 0; i < 512; ++i) { const float x = 0.3f * (float) std::sin (ph) + 0.05f * (r.nextFloat() - 0.5f); ph += 0.0314; b.setSample (0, i, x); b.setSample (1, i, x); if (k > 50) ei += x * x; } q.processBlock (b, mm); for (int i = 0; i < 512; ++i) { if (! std::isfinite (b.getSample (0, i))) nan = true; if (k > 50) eo += b.getSample (0, i) * b.getSample (0, i); } }
        std::printf ("formante +7: uscita/ingresso %+.1f dB nan %d\n", 10 * std::log10 (eo / ei), (int) nan);
    }
    Processor p; const double sr = 44100; p.prepareToPlay (sr, 512);
    juce::AudioBuffer<float> blk (2, 512); juce::MidiBuffer m;
    for (int pr = 0; pr < p.getNumPrograms(); ++pr)
    {
        p.loadPreset (pr); if (pr % 3 == 0) { p.dsp.order[0] = 4; p.dsp.order[3] = 1; } else { p.dsp.order[0] = 1; p.dsp.order[3] = 4; }
        float pk = 0, inpk = 0; bool nan = false; double ph = 0;
        for (int b = 0; b < 200; ++b)
        {
            for (int i = 0; i < 512; ++i) { const int n = b * 512 + i; const double t = n / sr, beat = std::fmod (t * 2, 1.0);
                const float kick = beat < 0.25 ? (float) (std::sin (2 * 3.14159 * 55 * t) * std::exp (-beat * 20)) * 0.8f : 0.0f;
                const float hat = (std::fmod (t * 8, 1.0) < 0.05) ? (float) ((rand() / (float) RAND_MAX) * 2 - 1) * 0.2f : 0.0f;
                blk.setSample (0, i, kick + hat); blk.setSample (1, i, kick + hat); inpk = std::max (inpk, std::abs (kick + hat)); ph += 1; }
            p.processBlock (blk, m);
            for (int c = 0; c < 2; ++c) for (int i = 0; i < 512; ++i) { const float x = blk.getSample (c, i); if (! std::isfinite (x)) nan = true; pk = std::max (pk, std::abs (x)); }
        }
        std::printf ("%-44s picco %.2f (in %.2f) nan %d\n", p.getProgramName (pr).toRawUTF8(), pk, inpk, (int) nan);
    }
    for (int rr = 0; rr < 30; ++rr)
    {
        p.randomize(); float pk = 0; bool nan = false;
        for (int b = 0; b < 120; ++b)
        {
            for (int i = 0; i < 512; ++i) { const int n = b * 512 + i; const double t = n / sr, beat = std::fmod (t * 2, 1.0);
                const float x = beat < 0.25 ? (float) (std::sin (2 * 3.14159 * 55 * t) * std::exp (-beat * 20)) * 0.8f : 0.0f; blk.setSample (0, i, x); blk.setSample (1, i, x); }
            p.processBlock (blk, m);
            for (int c = 0; c < 2; ++c) for (int i = 0; i < 512; ++i) { const float x = blk.getSample (c, i); if (! std::isfinite (x)) nan = true; pk = std::max (pk, std::abs (x)); }
        }
        std::printf ("random %2d: extra %d amt %.2f  picco %.2f nan %d\n", rr, (int) p.apvts.getRawParameterValue ("u_xType")->load(), p.apvts.getRawParameterValue ("u_xAmt")->load(), pk, (int) nan);
    }
    std::printf ("latenza: %d campioni\n", p.getLatencySamples());
    p.locked[2] = true; p.loadPreset (3); const float c1 = p.apvts.getRawParameterValue ("u_cAmt")->load(); p.loadPreset (6);
    std::printf ("lucchetto compress: %.2f -> %.2f (deve restare uguale)\n", c1, p.apvts.getRawParameterValue ("u_cAmt")->load());
}
