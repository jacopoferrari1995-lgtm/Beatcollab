#include <cstdlib>
#include "../Source/CompProcessor.h"
#include "../Source/CompEditor.h"
#include <cstdio>
using namespace jc;
int main (int argc, char** argv)
{
    if (argc > 2 && juce::String (argv[1]) == "snap")
    {
        juce::ScopedJuceInitialiser_GUI gui2;
        Processor p; p.prepareToPlay (48000, 480);
        for (int i = 0; i < p.getNumPrograms(); ++i) if (p.getProgramName (i).contains (std::getenv ("JPRESET") ? std::getenv ("JPRESET") : "Voce a 4")) p.loadPreset (i);
        if (auto* t = std::getenv ("JTHEME")) skin::cur() = std::atoi (t);
        if (std::getenv ("JUNLINK")) for (auto* id : { "a_link", "b_link" }) if (auto* r = p.apvts.getParameter (id)) r->setValueNotifyingHost (0.0f);
        Editor ed (p);
        if (auto* jsc = std::getenv ("JSCALE")) ed.scaler.set ((float) std::atof (jsc));
        juce::AudioBuffer<float> b (2, 480); juce::MidiBuffer m; juce::Random r (5); double ph = 0;
        for (int k = 0; k < 100 * 5; ++k)
        {
            for (int i = 0; i < 480; ++i)
            {
                const long n = (long) k * 480 + i; const double t = n / 48000.0, sy = std::fmod (t * 3.1, 1.0);
                ph += 2 * 3.14159 * (180 + 40 * std::sin (t * 2)) / 48000;
                const float v = (float) (std::sin (ph) + 0.4 * std::sin (ph * 2.01)) * (float) (0.15 + 0.5 * std::exp (-sy * 4)) + (r.nextFloat() - 0.5f) * 0.05f;
                b.setSample (0, i, v); b.setSample (1, i, v * 0.95f);
            }
            p.processBlock (b, m);
        }
        ed.select (0);
        for (int k = 0; k < 20; ++k) { ed.stage.needle += (p.gr[0].load() - ed.stage.needle) * 0.5f; }
        if (std::getenv ("JBENCH"))
        {
            auto bench = [] (juce::Component& c, const char* nm)
            {
                const auto t0 = juce::Time::getMillisecondCounterHiRes(); const int n = 20;
                for (int i = 0; i < n; ++i) { auto im = c.createComponentSnapshot (c.getLocalBounds(), true, 2.0f); (void) im; }
                std::printf ("%-18s %7.2f ms per disegno (Retina 2x)\n", nm, (juce::Time::getMillisecondCounterHiRes() - t0) / n);
            };
            bench (ed, "editor intero"); bench (ed, "editor (cache calda)");
            for (auto* c : ed.getChildren()) c->setVisible (false); bench (ed, "solo sfondo editor"); for (auto* c : ed.getChildren()) c->setVisible (true);
            ed.graph.setVisible (true); bench (ed.stage, "stadio"); bench (ed.rack, "rack"); bench (ed.graph, "grafico"); bench (ed.guide, "guida");
            if (auto* k = ed.stage.drive.get()) bench (*k, "manopola grande"); if (auto* k = ed.stage.att.get()) bench (*k, "manopola");
            // DSP: 6 stadi accesi
            for (int s2 = 0; s2 < kStages; ++s2) if (auto* q = p.apvts.getParameter (juce::String (stagePfx (s2)) + "on")) q->setValueNotifyingHost (1.0f);
            const auto t0 = juce::Time::getMillisecondCounterHiRes(); for (int k = 0; k < 1000; ++k) p.processBlock (b, m);
            std::printf ("DSP 6 stadi: %.1f%% di un core (blocchi da 480 a 48 kHz)\n", (juce::Time::getMillisecondCounterHiRes() - t0) / 1000.0 / 10.0 * 100.0);
            return 0;
        }
        auto img = ed.createComponentSnapshot (ed.getLocalBounds(), true, 1.0f);
        juce::File f (argv[2]); f.deleteFile(); juce::FileOutputStream os (f); juce::PNGImageFormat().writeImageToStream (img, os);
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "presets")
    {
        juce::ScopedJuceInitialiser_GUI gui2;
        Processor p0; const int np = p0.getNumPrograms();
        for (int i = 1; i < np; ++i)
        {
            Processor p; p.prepareToPlay (48000, 480); p.loadPreset (i);
            juce::AudioBuffer<float> b (2, 480); juce::MidiBuffer mi; juce::Random r (5); double ph = 0; float g[kStages] {}; double ein = 0, eout = 0;
            for (int k = 0; k < 500; ++k)
            {
                for (int i2 = 0; i2 < 480; ++i2)
                {
                    const long n = (long) k * 480 + i2; const double t = n / 48000.0, sy = std::fmod (t * 3.1, 1.0);
                    ph += 2 * 3.14159 * (180 + 40 * std::sin (t * 2)) / 48000;
                    const float v = 0.5f * ((float) (std::sin (ph) + 0.4 * std::sin (ph * 2.01)) * (float) (0.15 + 0.5 * std::exp (-sy * 4)) + (r.nextFloat() - 0.5f) * 0.05f);
                    b.setSample (0, i2, v); b.setSample (1, i2, v * 0.95f);
                }
                if (k > 200) for (int i2 = 0; i2 < 480; ++i2) ein += b.getSample (0, i2) * b.getSample (0, i2);
                p.processBlock (b, mi);
                if (k > 200) for (int i2 = 0; i2 < 480; ++i2) eout += b.getSample (0, i2) * b.getSample (0, i2);
                if (k > 200) for (int s = 0; s < kStages; ++s) g[s] += p.avgGr[s].load() / 299.0f;
            }
            juce::String l; for (int s = 0; s < kStages; ++s) if (p.stageParams (s).on) l << info (p.stageParams (s).model).name << " " << juce::String (g[s], 1) << " (soglia " << juce::String (stageThrDb (p.stageParams (s).model, p.stageParams (s).drive, p.stageParams (s).thr670), 0) << ")  ";
            std::printf ("%-40s %+5.1f dB | %s\n", p.getProgramName (i).toRawUTF8(), 10 * std::log10 (eout / ein), l.toRawUTF8());
        }
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "neutral")
    {   // ogni modello acceso con i valori di partenza: non deve comprimere ne' cambiare volume
        juce::ScopedJuceInitialiser_GUI gui2;
        for (int m = 0; m < kNumModels; ++m)
        {
            Processor p; p.prepareToPlay (48000, 480);
            p.apvts.getParameter ("a_on")->setValueNotifyingHost (1.0f);
            auto* pm = p.apvts.getParameter ("a_model"); pm->setValueNotifyingHost (pm->convertTo0to1 ((float) m));
            if (m == MULTI) { auto* d = p.apvts.getParameter ("a_drive"); d->setValueNotifyingHost (0.3f); }
            juce::AudioBuffer<float> b (2, 480); juce::MidiBuffer mi; double ph = 0, ein = 0, eout = 0; float mx = 0;
            for (int k = 0; k < 300; ++k)
            {
                for (int i = 0; i < 480; ++i) { ph += 2 * 3.14159 * 220 / 48000; const float v = 0.89f * (float) std::sin (ph) * (float) (0.3 + 0.7 * ((k / 25) % 2)); b.setSample (0, i, v); b.setSample (1, i, v); }
                if (k > 100) for (int i = 0; i < 480; ++i) ein += b.getSample (0, i) * b.getSample (0, i);
                p.processBlock (b, mi);
                if (k > 100) for (int i = 0; i < 480; ++i) eout += b.getSample (0, i) * b.getSample (0, i);
                if (k > 100) mx = juce::jmax (mx, p.gr[0].load());
            }
            std::printf ("%-11s GR max %5.2f dB   volume %+.2f dB\n", info (m).name, mx, 10 * std::log10 (eout / ein));
        }
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "listen")
    {   // ASCOLTO su quattro materiali sintetici: proposta, applicazione, taratura
        juce::ScopedJuceInitialiser_GUI gl;
        const double fs = 48000; const int B = 480;
        auto gen = [] (int kind, long n, juce::Random& r, double& ph) -> float
        {
            const double t = n / 48000.0;
            if (kind == 0)
            {   // voce: sillabe a ~4.5/s, frasi che salgono e scendono, consonanti secche
                const double sy = std::fmod (t * 4.5, 1.0), phrase = 0.35 + 0.65 * (0.5 + 0.5 * std::sin (t * 1.3)) * (std::fmod (t, 4.0) < 3.4 ? 1 : 0.05);
                ph += 2 * 3.14159 * (160 + 20 * std::sin (t * 3)) / 48000;
                const double vow = (std::sin (ph) + 0.6 * std::sin (2 * ph) + 0.4 * std::sin (3 * ph) + 0.25 * std::sin (5 * ph)) * std::min (1.0, sy * 12) * std::exp (-sy * 1.5);
                const double cons = sy < 0.025 ? (r.nextFloat() - 0.5) * 2.2 : 0;
                return (float) (phrase * (0.22 * vow + 0.35 * cons));
            }
            if (kind == 1)
            {   // batteria: cassa sui 1 e 3, rullante sui 2 e 4, charleston in ottavi (90 bpm)
                const double beat = t * 1.5, b4 = std::fmod (beat, 4.0), e8 = std::fmod (beat * 2, 1.0) / 2.0 / 1.5;
                const double kb = std::fmod (b4, 2.0) / 1.5, kick = std::sin (2 * 3.14159 * (50 + 120 * std::exp (-kb * 30)) * kb) * std::exp (-kb * 7);
                const double sb = std::fmod (b4 + 1.0, 2.0) / 1.5, snare = (r.nextFloat() - 0.5) * 2 * std::exp (-sb * 22) + std::sin (2 * 3.14159 * 190 * sb) * std::exp (-sb * 30);
                const double hat = (r.nextFloat() - 0.5) * std::exp (-e8 * 90);
                return (float) (0.75 * kick + 0.45 * snare + 0.12 * hat);
            }
            if (kind == 2)
            {   // 808: note lunghe a 50-60 Hz con volumi diversi
                const double nb = std::fmod (t * 2, 1.0); const int idx = (int) (t * 2); const double amp = 0.3 + 0.5 * ((idx * 7) % 5) / 4.0;
                ph += 2 * 3.14159 * (idx % 3 == 0 ? 49 : 58) / 48000;
                return (float) (amp * std::sin (ph) * std::exp (-nb * 1.2) * std::min (1.0, nb * 400));
            }
            // mix: tutto insieme, gia' un po' limitato
            double d = 0; { juce::Random r2 (n); double p2 = ph; (void) r2; (void) p2; }
            const double beat = t * 1.5, b4 = std::fmod (beat, 2.0) / 1.5; d += 0.5 * std::sin (2 * 3.14159 * 55 * b4) * std::exp (-b4 * 4);
            ph += 2 * 3.14159 * 220 / 48000; d += 0.25 * (std::sin (ph) + 0.5 * std::sin (1.5 * ph)) + (r.nextFloat() - 0.5) * 0.08;
            return (float) std::tanh (1.6 * d) * 0.6f;
        };
        for (int kind = 0; kind < 4; ++kind)
        {
            Processor p; p.prepareToPlay (fs, B);
            juce::AudioBuffer<float> b (2, B); juce::MidiBuffer m; juce::Random r (kind + 3); double ph = 0; long n = 0;
            auto run = [&] (int blocks) { for (int k = 0; k < blocks; ++k) { for (int i = 0; i < B; ++i) { const float v = gen (kind, n++, r, ph); b.setSample (0, i, v); b.setSample (1, i, v * 0.97f); } p.processBlock (b, m); } };
            p.startListen(); int guard = 0; while (p.cap.on.load() && guard++ < 3000) run (1);
            p.finishListen(); const auto& L = p.plan;
            std::printf ("\n==== materiale %d -> %s (gaps %.0f sharp %.1f)\n%s\n", kind, srcName (L.src), L.gaps, L.sharp, L.srcWhy.toRawUTF8());
            for (auto& h : L.heard) std::printf ("  . %s\n", h.toRawUTF8());
            for (auto& q : L.stages) std::printf ("  > %s\n      %s\n", q.what.toRawUTF8(), q.why.toRawUTF8());
            for (auto& t : L.notes) std::printf ("  ! %s\n", t.toRawUTF8());
            p.applyPlan();
            for (int s = 0; s < 1800 && p.calib >= 0; ++s) { run (4); p.calibTick(); }   // ~1 minuto massimo
            std::printf ("  = %s\n", p.calibMsg.toRawUTF8());
            for (size_t k = 0; k < L.stages.size(); ++k) std::printf ("    stadio %d drive %.2f  obiettivo %.1f\n", (int) k + 1, p.value ((juce::String (stagePfx ((int) k)) + "drive").toRawUTF8()), L.stages[k].target);
            if (kind == 0 && argc > 2)
            {
                Editor ed (p); ed.guide.tab = 3; ed.resized(); ed.guide.listening = false;
                auto img = ed.createComponentSnapshot (ed.getLocalBounds(), true, 1.0f);
                juce::File f (argv[2]); f.deleteFile(); juce::FileOutputStream os (f); juce::PNGImageFormat().writeImageToStream (img, os);
            }
        }
        return 0;
    }
    juce::ScopedJuceInitialiser_GUI gui;
    const double sr = 44100;
    {   // multibanda senza compressione: la somma delle bande deve restare piatta
        std::printf ("multibanda piatto:");
        for (float f : { 50.0f, 120.0f, 400.0f, 1000.0f, 3000.0f, 6000.0f, 12000.0f })
        {
            Processor p; p.prepareToPlay (sr, 512);
            auto set = [&p] (const char* id, float v) { auto* r = p.apvts.getParameter (id); r->setValueNotifyingHost (r->convertTo0to1 (v)); };
            set ("a_on", 1); set ("a_model", MULTI); set ("a_ratio", 2); set ("a_color", 0); for (int b = 1; b <= 4; ++b) set (("mb_b" + juce::String (b) + "_thr").toRawUTF8(), 0);
            juce::AudioBuffer<float> b (2, 512); juce::MidiBuffer mi; double ph = 0, ei = 0, eo = 0;
            for (int k = 0; k < 100; ++k) { for (int i = 0; i < 512; ++i) { const float v = 0.1f * (float) std::sin (ph); ph += 2 * 3.14159 * f / sr; b.setSample (0, i, v); b.setSample (1, i, v); if (k > 50) ei += v * v; } p.processBlock (b, mi); if (k > 50) for (int i = 0; i < 512; ++i) eo += b.getSample (0, i) * b.getSample (0, i); }
            std::printf (" %g Hz %+.2f dB;", f, 10 * std::log10 (eo / ei));
        }
        std::printf ("\n");
    }
    // per ogni modello: sinusoide a -12 dBFS con burst a -3 dBFS, misura GR e NaN
    for (int m = 0; m < kNumModels; ++m)
    {
        Processor p; p.prepareToPlay (sr, 512);
        auto set = [&p] (const char* id, float v) { auto* r = p.apvts.getParameter (id); r->setValueNotifyingHost (r->convertTo0to1 (v)); };
        set ("a_on", 1); set ("a_model", (float) m); set ("a_drive", 0.5f); set ("a_ratio", 1);
        juce::AudioBuffer<float> b (2, 512); juce::MidiBuffer mi; double ph = 0; float pk = 0, maxGr = 0; bool nan = false;
        for (int k = 0; k < 400; ++k)
        {
            const float amp = (k / 40) % 2 ? 0.7f : 0.25f;
            for (int i = 0; i < 512; ++i) { const float v = amp * (float) std::sin (ph); ph += 2 * 3.14159 * 110 / sr; b.setSample (0, i, v); b.setSample (1, i, v); }
            p.processBlock (b, mi);
            for (int i = 0; i < 512; ++i) { const float x = b.getSample (0, i); if (! std::isfinite (x)) nan = true; pk = std::max (pk, std::abs (x)); }
            maxGr = std::max (maxGr, p.gr[0].load());
        }
        std::printf ("%-9s GR max %5.1f dB  picco out %.2f  nan %d\n", info (m).name, maxGr, pk, (int) nan);
    }
    // tutti i preset
    for (int i = 0; i < (int) presets().size(); ++i)
    {
        Processor p; p.prepareToPlay (sr, 512); p.loadPreset (i);
        juce::AudioBuffer<float> b (2, 512); juce::MidiBuffer mi; double ph = 0; float pk = 0; bool nan = false;
        for (int k = 0; k < 200; ++k)
        {
            for (int i2 = 0; i2 < 512; ++i2) { const double t = (k * 512 + i2) / sr, bt = std::fmod (t * 2, 1.0); const float v = (float) (std::sin (2 * 3.14159 * 60 * t) * std::exp (-bt * 12) * 0.8 + std::sin (ph) * 0.1); ph += 0.05; b.setSample (0, i2, v); b.setSample (1, i2, v); }
            p.processBlock (b, mi);
            for (int i2 = 0; i2 < 512; ++i2) { const float x = b.getSample (0, i2); if (! std::isfinite (x)) nan = true; pk = std::max (pk, std::abs (x)); }
        }
        std::printf ("preset %-34s picco %.2f GR A %.1f B %.1f  out-in %+.1f dB nan %d\n", presets()[(size_t) i].name, pk, p.avgGr[0].load(), p.avgGr[1].load(), juce::Decibels::gainToDecibels (std::sqrt (p.outRms.load() / p.inRms.load())), (int) nan);
    }
    std::printf ("latenza %d\n", Processor().getLatencySamples());
    return 0;
}
