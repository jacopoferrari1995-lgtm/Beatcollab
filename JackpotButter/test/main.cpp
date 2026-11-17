#include <cstdlib>
#include "../Source/ButterProcessor.h"
#include "../Source/ButterEditor.h"
using namespace jb;
int main (int argc, char** argv)
{
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
