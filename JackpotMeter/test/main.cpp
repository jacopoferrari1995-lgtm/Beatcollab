#include <juce_audio_formats/juce_audio_formats.h>
#include <cstdlib>
#include "../Source/MeterProcessor.h"
#include "../Source/MeterEditor.h"
#include <cstdio>
using namespace jm;
int main (int argc, char** argv)
{
    if (auto* t = std::getenv ("JTHEME")) skin::cur() = std::atoi (t);
    juce::ScopedJuceInitialiser_GUI gui;
    if (argc > 2 && juce::String (argv[1]) == "snap")
    {
        Processor p; p.prepareToPlay (48000, 480);
        auto set = [&p] (const char* id, float v) { auto* r = p.apvts.getParameter (id); r->setValueNotifyingHost (r->convertTo0to1 (v)); };
        set ("b1_on", 1); set ("b3_on", 1); set ("b3_freq", 350); set ("b3_gain", -3); set ("b3_dyn", -6); set ("b3_q", 1.5f); set ("b5_on", 1); set ("b5_type", BELL); set ("b5_freq", 5000); set ("b5_gain", 2.5f);
        juce::AudioBuffer<float> b (2, 480); juce::MidiBuffer m; juce::Random r (3); float pk[2] = {}; double ph = 0;
        if (std::getenv ("JNOEQ")) set ("eqOn", 0);
        if (auto* v = std::getenv ("JVIEW")) set ("view", (float) std::atoi (v));
        if (auto* v = std::getenv ("JIMV")) set ("imView", (float) std::atoi (v));
        if (auto* v = std::getenv ("JPROF")) set ("profile", (float) std::atoi (v));
        // stesso segnale salvato su disco come pezzo di riferimento: lo scarto deve venire quasi zero
        juce::File refF ("/tmp/jm_ref.wav");
        if (std::getenv ("JREF"))
        {
            juce::Random r2 (3); float q[2] = {}; double ph2 = 0; juce::AudioBuffer<float> rb (2, 48000 * 12);
            for (int n = 0; n < rb.getNumSamples(); ++n)
            {
                const double t = n / 48000.0, bt = std::fmod (t * 2.33, 1.0);
                const float w = r2.nextFloat() * 2 - 1; q[0] = 0.98f * q[0] + 0.02f * w; q[1] = 0.7f * q[1] + 0.3f * (r2.nextFloat() * 2 - 1);
                const float kick = (float) (std::sin (2 * 3.14159 * (50 + 120 * std::exp (-bt * 30)) * bt) * std::exp (-bt * 5)) * 0.5f;
                ph2 += 2 * 3.14159 * 220 / 48000; const float pad = 0.08f * (float) std::sin (ph2) + 0.05f * (float) std::sin (ph2 * 1.5);
                rb.setSample (0, n, kick + pad + q[0] * 0.6f + q[1] * 0.05f); rb.setSample (1, n, kick + pad * 0.8f + q[0] * 0.5f - q[1] * 0.05f);
            }
            refF.deleteFile(); { juce::WavAudioFormat wf; std::unique_ptr<juce::AudioFormatWriter> wr (wf.createWriterFor (new juce::FileOutputStream (refF), 48000, 2, 24, {}, 0)); wr->writeFromAudioSampleBuffer (rb, 0, rb.getNumSamples()); }
            p.loadReference (refF); while (p.refLoading.load()) juce::Thread::sleep (50);
            set ("profile", (float) kGenres);
        }
        Editor ed (p);
        if (auto* jsc = std::getenv ("JSCALE")) ed.scaler.set ((float) std::atof (jsc));
        for (int k = 0; k < 100 * 12; ++k)
        {
            for (int i = 0; i < 480; ++i)
            {
                const long n = (long) k * 480 + i; const double t = n / 48000.0, bt = std::fmod (t * 2.33, 1.0);
                const float w = r.nextFloat() * 2 - 1; pk[0] = 0.98f * pk[0] + 0.02f * w; pk[1] = 0.7f * pk[1] + 0.3f * (r.nextFloat() * 2 - 1);
                const float kick = (float) (std::sin (2 * 3.14159 * (50 + 120 * std::exp (-bt * 30)) * bt) * std::exp (-bt * 5)) * 0.5f;
                ph += 2 * 3.14159 * 220 / 48000; const float pad = 0.08f * (float) std::sin (ph) + 0.05f * (float) std::sin (ph * 1.5);
                const float L = kick + pad + pk[0] * 0.6f + pk[1] * 0.05f, R = kick + pad * 0.8f + pk[0] * 0.5f - pk[1] * 0.05f;
                b.setSample (0, i, L); b.setSample (1, i, R);
            }
            p.processBlock (b, m);
            if (k % 3 == 0) { ed.spec.analyse(); }
        }
        for (int k = 0; k < 20; ++k) ed.spec.analyse();
        if (auto* v = std::getenv ("JPOP")) { ed.spec.popBand = std::atoi (v); ed.spec.updatePop(); }
        { float dev[kBal]; ed.spec.bandDeviation (dev); std::printf ("scarti per banda:"); for (float d : dev) std::printf (" %+.1f", d); auto R = p.reference(); std::printf ("   rif ok %d lufs %.1f picco %.1f | io lufsI %.1f\n", (int) R.ok, R.lufs, R.peak, p.lufsI.load());
          if (R.ok) for (int i : { 10, 40, 80, 120, 160, 200, 240, 280, 310 }) std::printf ("  %6.0f Hz  live %.1f  file %.1f\n", ptFreq (i), ed.spec.ltasDb (i), R.spec[(size_t) i]); std::printf ("ltasN %d\n", ed.spec.ltasN); }
        auto img = ed.createComponentSnapshot (ed.getLocalBounds(), true, 1.0f);
        { const auto t0 = juce::Time::getMillisecondCounterHiRes(); for (int i = 0; i < 20; ++i) ed.spec.analyse();
          const auto t1 = juce::Time::getMillisecondCounterHiRes(); for (int i = 0; i < 20; ++i) ed.spec.createComponentSnapshot (ed.spec.getLocalBounds(), true, 1.0f);
          std::printf ("analisi %.2f ms, disegno spettro %.2f ms\n", (t1 - t0) / 20.0, (juce::Time::getMillisecondCounterHiRes() - t1) / 20.0); }
        juce::File f (argv[2]); f.deleteFile(); juce::FileOutputStream os (f); juce::PNGImageFormat().writeImageToStream (img, os);
        return 0;
    }
    const double sr = 48000;
    if (argc > 1 && juce::String (argv[1]) == "lab")
    {
        auto gen = [&] (long n, float& l, float& r) { const double t = n / sr; l = (float) (0.3 * std::sin (2 * 3.14159265 * 110 * t) + 0.1 * std::sin (2 * 3.14159265 * 1234 * t)); r = (float) (0.25 * std::sin (2 * 3.14159265 * 110 * t + 0.4) + 0.1 * std::sin (2 * 3.14159265 * 2345 * t)); };
        auto setup = [] (Processor& p, bool busy)
        {
            auto set = [&p] (const char* id, float v) { auto* r = p.apvts.getParameter (id); r->setValueNotifyingHost (r->convertTo0to1 (v)); };
            if (busy) { set ("b1_on", 1); set ("b3_on", 1); set ("b3_freq", 400); set ("b3_gain", -3); set ("b3_dyn", -6); set ("b5_on", 1); set ("b5_type", BELL); set ("b5_freq", 3000); set ("b5_gain", 3); set ("b6_on", 1);
                        set ("imOn", 1); set ("imWidth", 1.3f); set ("imMono", 120); set ("imHigh", 1.4f); }
        };
        auto render = [&] (Processor& p, int len, int blk, bool uneven, std::function<void (int)> at = {})
        {
            std::vector<float> o; juce::MidiBuffer m; juce::Random rb (3);
            for (int off = 0; off < len; )
            {
                const int n = uneven ? juce::jmin (len - off, 1 + rb.nextInt (blk)) : juce::jmin (blk, len - off);
                if (at) at (off);
                juce::AudioBuffer<float> b (2, n); for (int i = 0; i < n; ++i) { float l, r; gen (off + i, l, r); b.setSample (0, i, l); b.setSample (1, i, r); }
                p.processBlock (b, m); for (int i = 0; i < n; ++i) { o.push_back (b.getSample (0, i)); o.push_back (b.getSample (1, i)); } off += n;
            }
            return o;
        };
        const int len = (int) sr * 4;
        {
            Processor p; setup (p, false); p.prepareToPlay (sr, 512); auto o = render (p, len, 512, false);
            double e = 0; for (int i = 0; i < len; ++i) { float l, r; gen (i, l, r); e = std::max (e, (double) std::abs (o[(size_t) i * 2] - l) + std::abs (o[(size_t) i * 2 + 1] - r)); }
            std::printf ("neutro all'avvio: differenza massima %.2g, latenza %d\n", e, p.getLatencySamples());
        }
        {
            Processor a, b2; setup (a, true); setup (b2, true); a.prepareToPlay (sr, 512); b2.prepareToPlay (sr, 512);
            auto o1 = render (a, len, 512, false), o2 = render (b2, len, 512, true);
            double e = 0, d = 0; for (size_t i = 0; i < o1.size(); ++i) { e += (double) o1[i] * o1[i]; d += std::pow (o1[i] - o2[i], 2.0); }
            std::printf ("EQ + imager, blocchi irregolari: %.1f dB\n", 10 * std::log10 (d / (e + 1e-30) + 1e-30));
            Processor big; setup (big, true); big.prepareToPlay (sr, 256); auto o3 = render (big, len, 4096, false);   // buffer piu' lunghi del previsto
            bool nan = false; for (float x : o3) if (! std::isfinite (x)) nan = true; std::printf ("buffer 4096 con 256 preparati: nan %d, true peak %.2f\n", (int) nan, big.tpMax.load());
        }
        {
            const char* ids[] = { "b3_on", "b3_type", "b3_freq", "b3_gain", "b3_q", "b3_dyn", "b5_gain", "eqOn", "imOn", "imWidth", "imLow", "imHigh", "imMono", "imBal", "monoChk", "out" };
            int nc = 0;
            for (auto* id : ids)
            {
                auto run = [&] (int mode)
                {
                    Processor p; setup (p, true); p.prepareToPlay (sr, 512);
                    auto* prm = p.apvts.getParameter (id); const float v0 = prm->getValue(), v1 = v0 > 0.5f ? v0 - 0.4f : v0 + 0.4f;
                    if (mode == 1) prm->setValueNotifyingHost (v1);
                    return render (p, len, 512, false, [&] (int off) { const int bk = off / 512; if (mode == 2 && bk >= 100 && (bk - 100) % 50 == 0 && bk < 300) prm->setValueNotifyingHost (((bk - 100) / 50) % 2 ? v0 : v1); });
                };
                const auto a = run (0), c1 = run (1), b2 = run (2);
                double worst = 0;
                for (int j = 0; j < 4; ++j)
                {
                    const size_t at = (size_t) (100 + j * 50) * 512;
                    auto hf = [&] (const std::vector<float>& x) { double s2 = 0; for (size_t i = at - 100; i < at + 1500; ++i) for (int c = 0; c < 2; ++c) s2 = std::max (s2, (double) std::abs (x[(i + 2) * 2 + c] - 2 * x[(i + 1) * 2 + c] + x[i * 2 + c])); return s2; };
                    worst = std::max (worst, hf (b2) / juce::jmax (1e-9, hf (a), hf (c1)));
                }
                std::printf ("  %-8s salto / fermo %.2f%s\n", id, worst, worst > 1.5 ? "  CLICK?" : ""); if (worst > 1.5) ++nc;
            }
            std::printf ("click sospetti: %d\n", nc);
        }
        {   // solo di banda acceso e spento
            Processor p; setup (p, true); p.prepareToPlay (sr, 512); std::vector<float> o;
            o = render (p, len, 512, false, [&] (int off) { const int bk = off / 512; if (bk == 150) p.soloBand = 2; if (bk == 250) p.soloBand = -1; });
            double w = 0; for (int bk : { 150, 250 }) for (size_t i = (size_t) bk * 512 - 50; i < (size_t) bk * 512 + 1000; ++i) w = std::max (w, (double) std::abs (o[(i + 2) * 2] - 2 * o[(i + 1) * 2] + o[i * 2]));
            double ref = 0; for (size_t i = 60000; i < 70000; ++i) ref = std::max (ref, (double) std::abs (o[(i + 2) * 2] - 2 * o[(i + 1) * 2] + o[i * 2]));
            std::printf ("SOLO dentro/fuori: derivata seconda %.4f (fermo %.4f)\n", w, ref);
        }
        {
            Processor p; setup (p, true); p.prepareToPlay (sr, 512); const auto t0 = juce::Time::getMillisecondCounterHiRes(); render (p, (int) sr * 10, 512, false);
            std::printf ("CPU: %.1f%% di un core (6 bande, imager, misure)\n", (juce::Time::getMillisecondCounterHiRes() - t0) / 100.0);
        }
        return 0;
    }
    // 1) sinusoide 1 kHz a -20 dBFS stereo: deve dare circa -20 + 3.01 - 0.691 + K(1k)= ~ -17.0 LUFS? (stereo: -20 dBFS sin = -23 dB RMS, x2 canali +3 = -20 LUFS + 0.7 di K a 1 kHz)
    {
        Processor p; p.prepareToPlay (sr, 480);
        juce::AudioBuffer<float> b (2, 480); juce::MidiBuffer m; double ph = 0; const float a = juce::Decibels::decibelsToGain (-20.0f);
        for (int k = 0; k < 100 * 20; ++k)   // 20 s
        {
            for (int i = 0; i < 480; ++i) { const float v = a * (float) std::sin (ph); ph += 2 * 3.14159265 * 997 / sr; b.setSample (0, i, v); b.setSample (1, i, v); }
            p.processBlock (b, m);
        }
        std::printf ("sinusoide 997 Hz -20 dBFS stereo: M %.2f S %.2f I %.2f LUFS (atteso ~ -20.0)  TP %.2f  corr %.2f largh %.2f LRA %.2f\n", p.lufsM.load(), p.lufsS.load(), p.lufsI.load(), p.tpMax.load(), p.corr.load(), p.width.load(), p.lra.load());
    }
    // 2) true peak: sinusoide a fs/4 con fase 45 gradi: picco campionato -3 dB, true peak 0
    {
        Processor p; p.prepareToPlay (sr, 480);
        juce::AudioBuffer<float> b (2, 480); juce::MidiBuffer m; long n = 0;
        for (int k = 0; k < 200; ++k)
        {
            for (int i = 0; i < 480; ++i, ++n) { const float v = 0.9f * (float) std::sin (3.14159265 / 2 * n + 3.14159265 / 4); b.setSample (0, i, v); b.setSample (1, i, -v); }
            p.processBlock (b, m);
        }
        std::printf ("fs/4 a 45 gradi, ampiezza 0.9: picco campioni %.2f dB, true peak %.2f dB (atteso ~ -0.9)  corr %.2f\n", p.pkMax.load(), p.tpMax.load(), p.corr.load());
    }
    // 3) EQ dinamico: banda bell a 1k, dyn -12, soglia -30: il segnale forte deve essere attenuato
    {
        Processor p; p.prepareToPlay (sr, 480);
        auto set = [&p] (const char* id, float v) { auto* r = p.apvts.getParameter (id); r->setValueNotifyingHost (r->convertTo0to1 (v)); };
        set ("b3_on", 1); set ("b3_type", BELL); set ("b3_freq", 1000); set ("b3_gain", 0); set ("b3_q", 1); set ("b3_dyn", -12); set ("b3_thr", -30);
        juce::AudioBuffer<float> b (2, 480); juce::MidiBuffer m; double ph = 0; float pk = 0; bool nan = false;
        for (int k = 0; k < 400; ++k)
        {
            for (int i = 0; i < 480; ++i) { const float v = 0.5f * (float) std::sin (ph); ph += 2 * 3.14159265 * 1000 / sr; b.setSample (0, i, v); b.setSample (1, i, v); }
            p.processBlock (b, m);
            if (k > 300) for (int i = 0; i < 480; ++i) { if (! std::isfinite (b.getSample (0, i))) nan = true; pk = std::max (pk, std::abs (b.getSample (0, i))); }
        }
        std::printf ("EQ dinamico: in 0.50 -> out %.3f (%.1f dB), dynGain %.1f dB, nan %d\n", pk, juce::Decibels::gainToDecibels (pk / 0.5f), p.dynGain[2].load(), (int) nan);
    }
    std::printf ("latenza %d\n", Processor().getLatencySamples());
    return 0;
}
