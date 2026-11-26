#include "../Source/VoxProcessor.h"
#include "../Source/VoxEditor.h"
#include <cstdio>
#include <juce_audio_formats/juce_audio_formats.h>
using namespace jvox;

static void voice (juce::AudioBuffer<float>& b, double sr, double& ph, int off)
{   // voce finta: sillabe con vocali e pause
    for (int i = 0; i < b.getNumSamples(); ++i)
    {
        const double t = (off + i) / sr, syl = std::fmod (t, 0.5), env = syl < 0.35 ? std::sin (juce::MathConstants<double>::pi * syl / 0.35) : 0.0;
        ph += 2 * juce::MathConstants<double>::pi * (180 + 20 * std::sin (t * 3)) / sr;
        const float v = (float) (0.35 * env * (std::sin (ph) + 0.5 * std::sin (2 * ph) + 0.25 * std::sin (3 * ph)));
        b.setSample (0, i, v); b.setSample (1, i, v);
    }
}
static int onlyT = -1;
static void phrases (juce::AudioBuffer<float>& b, double sr, double& ph, int off)
{   // frasi da 1.6 s e pause da 0.9 s
    voice (b, sr, ph, off);
    for (int i = 0; i < b.getNumSamples(); ++i) { const double t = std::fmod ((off + i) / sr, 2.5); if (t > 1.6) { b.setSample (0, i, 0); b.setSample (1, i, 0); } }
}
int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    const juce::String mode = argc > 1 ? argv[1] : "test";
    const double sr = 48000; const int B = 512;
    if (mode == "snap")
    {
        Processor p; p.prepareToPlay (sr, B);
        if (auto* e = std::getenv ("JPRESET")) p.loadPreset (std::atoi (e));
        Editor ed (p); ed.setSize (Editor::W, Editor::H);
        juce::AudioBuffer<float> b (2, B); juce::MidiBuffer m; double ph = 0;
        for (int k = 0; k < 40; ++k) { voice (b, sr, ph, k * B); p.processBlock (b, m); }
        for (int k = 0; k < 160; ++k) { voice (b, sr, ph, k * B); p.processBlock (b, m); ed.envHist[(size_t) k] = p.eng.uiEnv.load(); ed.throwHist[(size_t) k] = k % 31 == 20; }
        ed.eye = 0.5f;
        if (std::getenv ("JBROWSE")) { ed.browser.open(); ed.browser.finishBoot(); }
        auto img = ed.createComponentSnapshot (ed.getLocalBounds(), true, std::getenv ("JX2") ? 2.0f : 1.0f);
        juce::File f (argc > 2 ? argv[2] : "/tmp/vox.png"); f.deleteFile(); juce::FileOutputStream os (f); juce::PNGImageFormat().writeImageToStream (img, os);
        std::printf ("snap %s\n", f.getFullPathName().toRawUTF8());
        return 0;
    }
    if (mode == "wav")
    {   // ascolto: ogni preset su 4 secondi di voce finta
        juce::WavAudioFormat wf;
        for (int i = 1; i < (int) presets().size(); ++i)
        {
            Processor p; p.prepareToPlay (sr, B); p.loadPreset (i);
            juce::File f ("/tmp/vox_" + juce::String (i) + ".wav"); f.deleteFile();
            std::unique_ptr<juce::AudioFormatWriter> w (wf.createWriterFor (new juce::FileOutputStream (f), sr, 2, 24, {}, 0));
            juce::AudioBuffer<float> b (2, B); juce::MidiBuffer m; double ph = 0;
            for (int k = 0; k < (int) (4 * sr / B); ++k) { voice (b, sr, ph, k * B); p.processBlock (b, m); w->writeFromAudioSampleBuffer (b, 0, B); }
        }
        return 0;
    }
    int fails = 0;
    {   // partenza neutra: identico, campione per campione
        Processor p; p.prepareToPlay (sr, B); juce::AudioBuffer<float> b (2, B); juce::MidiBuffer m; double ph = 0, err = 0;
        for (int k = 0; k < 60; ++k) { voice (b, sr, ph, k * B); juce::AudioBuffer<float> in; in.makeCopyOf (b); p.processBlock (b, m);
            for (int c = 0; c < 2; ++c) for (int i = 0; i < B; ++i) err = std::max (err, (double) std::abs (b.getSample (c, i) - in.getSample (c, i))); }
        std::printf ("neutro: differenza max %.2g, latenza %d %s\n", err, p.getLatencySamples(), err < 1e-9 && p.getLatencySamples() == 0 ? "OK" : "ERRORE"); if (err >= 1e-9) ++fails;
    }
    for (int i = 1; i < (int) presets().size(); ++i)
    {   // ogni preset: niente NaN, niente clip, volume simile all'ingresso
        Processor p; p.prepareToPlay (sr, B); p.loadPreset (i);
        juce::AudioBuffer<float> b (2, B); juce::MidiBuffer m; double ph = 0, ein = 0, eout = 0; float pk = 0; bool nan = false;
        const auto t0 = juce::Time::getMillisecondCounterHiRes();
        for (int k = 0; k < 400; ++k)
        {
            voice (b, sr, ph, k * B); for (int q = 0; q < B; ++q) ein += b.getSample (0, q) * b.getSample (0, q);
            p.processBlock (b, m);
            for (int c = 0; c < 2; ++c) for (int q = 0; q < B; ++q) { const float v = b.getSample (c, q); if (! std::isfinite (v)) nan = true; if (k > 10) { pk = std::max (pk, std::abs (v)); eout += 0.5 * v * v; } }
        }
        const double cpu = (juce::Time::getMillisecondCounterHiRes() - t0) / (400.0 * B / sr * 1000.0) * 100.0;
        const double gain = 10 * std::log10 (eout / ein);
        const bool ok = ! nan && pk < 1.4f && gain > -6 && gain < 6;
        std::printf ("%-22s livello %+5.1f dB picco %.2f CPU %.1f%% %s\n", p.getProgramName (i).toRawUTF8(), gain, pk, cpu, ok ? "OK" : "ERRORE");
        if (! ok) ++fails;
    }
    {   // click: muove tutto di colpo, a varie frequenze e blocchi casuali
        for (double rate : { 44100.0, 48000.0, 96000.0, 192000.0 })
        {
            auto run = [&] (bool moves, double& worst)
            {
                Processor p; p.prepareToPlay (rate, 2048); p.loadPreset (1);
                juce::AudioBuffer<float> b (2, 2048); juce::MidiBuffer m; double ph = 0; float x1 = 0, x2 = 0; worst = 0; juce::Random r (9); int pos = 0;
                for (int k = 0; k < 300; ++k)
                {
                    const int n = 1 + r.nextInt (2047); b.setSize (2, n, false, false, true);
                    if (moves && k % 7 == 3 && (onlyT < 0 || k % 8 == onlyT))
                        switch (k % 8) { case 0: p.setP ("cromo", r.nextFloat()); break; case 1: p.setP ("robot", r.nextFloat() * 0.5f); break; case 2: p.setP ("shift", r.nextFloat() * 2 - 1); break;
                                         case 3: p.setP ("width", r.nextFloat()); break; case 4: p.setP ("space", r.nextFloat()); break; case 5: p.setP ("band", r.nextFloat()); break;
                                         case 6: p.setP ("out", r.nextFloat() * 12 - 9); break; default: p.setP ("eco", r.nextFloat()); }
                    voice (b, rate, ph, pos); pos += n; p.processBlock (b, m);
                    for (int q = 0; q < n; ++q) { const float x = b.getSample (0, q); if (k > 10) worst = std::max (worst, (double) std::abs (x - 2 * x1 + x2)); x2 = x1; x1 = x; if (! std::isfinite (x)) worst = 99; }
                }
            };
            double base = 0, mov = 0; run (false, base); run (true, mov);
            if (std::getenv ("JONLY")) { for (int t = 0; t < 8; ++t) { onlyT = t; double m2 = 0; run (true, m2); std::printf ("  tipo %d: %.3f\n", t, m2); } onlyT = -1; }
            const bool ok = mov < base * 2.5 + 0.02;
            std::printf ("click %6.0f Hz, blocchi casuali: curvatura max %.3f (fermo %.3f) %s\n", rate, mov, base, ok ? "OK" : "ERRORE"); if (! ok) ++fails;
        }
    }
    {   // i throw partono a fine frase (e non durante)
        Processor p; p.prepareToPlay (sr, B); p.setP ("eco", 1); p.setP ("space", 1); p.setP ("glitch", 1);
        juce::AudioBuffer<float> b (2, B); juce::MidiBuffer m; double ph = 0; float pk = 0; bool nan = false;
        for (int k = 0; k < (int) (10 * sr / B); ++k) { phrases (b, sr, ph, k * B); p.processBlock (b, m); for (int q = 0; q < B; ++q) { const float v = b.getSample (0, q); nan |= ! std::isfinite (v); pk = std::max (pk, std::abs (v)); } }
        // poi silenzio: la coda deve suonare e poi spegnersi
        double tail = 0, end = 0;
        for (int k = 0; k < (int) (12 * sr / B); ++k) { b.clear(); p.processBlock (b, m); const double e = b.getRMSLevel (0, 0, B); if (k < 40) tail = std::max (tail, e); if (k > (int) (10 * sr / B)) end = std::max (end, e); }
        const int t = p.eng.uiThrows.load();
        const bool ok = ! nan && t == 4 && pk < 2.0f && tail > 0.005 && end < 1e-4;
        std::printf ("throw: %d in 10 s (4 frasi), picco %.2f, coda %.4f, dopo 10 s %.6f %s\n", t, pk, tail, end, ok ? "OK" : "ERRORE"); if (! ok) ++fails;
    }
    {   // stato
        Processor p; p.prepareToPlay (sr, B); p.loadPreset (5);
        juce::MemoryBlock mb; p.getStateInformation (mb); Processor q; q.setStateInformation (mb.getData(), (int) mb.getSize());
        const bool ok = q.preset == 5 && std::abs (q.value ("robot") - p.value ("robot")) < 0.01f && std::abs (q.value ("band") - p.value ("band")) < 0.01f;
        std::printf ("stato ripristinato %s\n", ok ? "OK" : "ERRORE"); if (! ok) ++fails;
    }
    std::printf (fails == 0 ? "TUTTO OK\n" : "ERRORI: %d\n", fails);
    return fails;
}
