#include "../Source/CrewProcessor.h"
#include "../Source/CrewEditor.h"
#include <cstdio>
using namespace jcrew;

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
static int onlyType = -1;
int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    const juce::String mode = argc > 1 ? argv[1] : "test";
    const double sr = 48000; const int B = 512;
    if (mode == "snap")
    {
        Processor p; p.prepareToPlay (sr, B);
        if (auto* e = std::getenv ("JPRESET")) p.loadPreset (std::atoi (e));
        if (auto* e = std::getenv ("JTHEME")) p.uiTheme = std::atoi (e);
        Editor ed (p); ed.setSize (Editor::W, Editor::H);
        juce::AudioBuffer<float> b (2, B); juce::MidiBuffer m; double ph = 0;
        for (int k = 0; k < 40; ++k) { voice (b, sr, ph, k * B); p.processBlock (b, m); }
        for (auto& c : ed.cards) c->mouth = std::getenv ("JSING") ? 0.8f : 0.0f;
        if (std::getenv ("JBROWSE")) { ed.browser.open(); ed.browser.finishBoot(); }
        auto img = ed.createComponentSnapshot (ed.getLocalBounds(), true, std::getenv ("JX2") ? 2.0f : 1.0f);
        juce::File f (argc > 2 ? argv[2] : "/tmp/crew.png"); f.deleteFile(); juce::FileOutputStream os (f); juce::PNGImageFormat().writeImageToStream (img, os);
        std::printf ("snap %s\n", f.getFullPathName().toRawUTF8());
        return 0;
    }
    int fails = 0;
    {   // partenza neutra
        Processor p; p.prepareToPlay (sr, B); juce::AudioBuffer<float> b (2, B); juce::MidiBuffer m; double ph = 0, err = 0;
        const int Dl = p.getLatencySamples(); std::vector<float> hist;
        for (int k = 0; k < 60; ++k) { voice (b, sr, ph, k * B); for (int i = 0; i < B; ++i) hist.push_back (b.getSample (0, i)); p.processBlock (b, m);
            for (int i = 0; i < B; ++i) { const int src = k * B + i - Dl; const float want = src >= 0 ? hist[(size_t) src] : 0.0f; err = std::max (err, (double) std::abs (b.getSample (0, i) - want)); } }
        std::printf ("neutro: differenza max %.2g %s\n", err, err < 1e-6 ? "OK" : "ERRORE"); if (err >= 1e-6) ++fails;
    }
    for (int i = 1; i < (int) presets().size(); ++i)
    {   // ogni preset: niente NaN, niente clip, la crew si sente, nessun salto
        Processor p; p.prepareToPlay (sr, B); p.loadPreset (i);
        juce::AudioBuffer<float> b (2, B); juce::MidiBuffer m; double ph = 0, ein = 0, eout = 0; float pk = 0, prev = 0, jump = 0; bool nan = false;
        const auto t0 = juce::Time::getMillisecondCounterHiRes();
        for (int k = 0; k < 400; ++k)
        {
            voice (b, sr, ph, k * B); for (int q = 0; q < B; ++q) ein += b.getSample (0, q) * b.getSample (0, q);
            p.processBlock (b, m);
            for (int q = 0; q < B; ++q) { const float v = b.getSample (0, q); if (! std::isfinite (v)) nan = true; pk = std::max (pk, std::abs (v)); eout += v * v; if (k > 20) jump = std::max (jump, std::abs (v - prev)); prev = v; }
        }
        const double cpu = (juce::Time::getMillisecondCounterHiRes() - t0) / (400.0 * B / sr * 1000.0) * 100.0;
        const double gain = 10 * std::log10 (eout / ein);
        const bool ok = ! nan && pk < 1.5f && gain > 0.3 && jump < 0.3f;
        std::printf ("%-16s crew %+5.1f dB picco %.2f salto %.3f CPU %.1f%% %s\n", p.getProgramName (i).toRawUTF8(), gain, pk, jump, cpu, ok ? "OK" : "ERRORE");
        if (! ok) ++fails;
    }
    {   // click: accendi/spegni personaggi, cambia altezza, sciolti, stanza e gridato di colpo, a varie frequenze e blocchi
        for (double rate : { 44100.0, 48000.0, 96000.0, 192000.0 })
        {
            auto run = [&] (bool moves, double& worst)
            {
                Processor p; p.prepareToPlay (rate, 2048); p.loadPreset (2);
                juce::AudioBuffer<float> b (2, 2048); juce::MidiBuffer m; double ph = 0; float x1 = 0, x2 = 0; worst = 0; juce::Random r (9); int pos = 0;
                for (int k = 0; k < 300; ++k)
                {
                    const int n = 1 + r.nextInt (2047); b.setSize (2, n, false, false, true);
                    if (moves && k % 7 == 3 && (onlyType < 0 || k % 9 == onlyType))
                    {
                        const int v = r.nextInt (kVoices);
                        switch (k % 9) { case 0: p.setP (Processor::vid (v, "on"), r.nextBool() ? 1.0f : 0.0f); break; case 1: p.setP (Processor::vid (v, "pitch"), (float) (r.nextInt (25) - 12)); break;
                                         case 2: p.setP ("loose", r.nextFloat()); break; case 3: p.setP ("verb", r.nextFloat()); break; case 4: p.setP ("shout", r.nextFloat()); break;
                                         case 5: p.setP (Processor::vid (v, "formant"), r.nextFloat() * 2 - 1); break; case 6: p.setP (Processor::vid (v, "delay"), r.nextFloat() * 120); break;
                                         case 7: p.setP ("echo", r.nextFloat()); break; default: p.setP ("drive", r.nextFloat()); }
                    }
                    voice (b, rate, ph, pos); pos += n; p.processBlock (b, m);
                    for (int q = 0; q < n; ++q) { const float x = b.getSample (0, q); if (k > 10) worst = std::max (worst, (double) std::abs (x - 2 * x1 + x2)); x2 = x1; x1 = x; if (! std::isfinite (x)) worst = 99; }
                }
            };
            double base = 0, mov = 0; run (false, base); run (true, mov);
            if (std::getenv ("JONLY")) { for (int t = 0; t < 9; ++t) { onlyType = t; double m2 = 0; run (true, m2); std::printf ("  tipo %d: %.3f\n", t, m2); } onlyType = -1; }
            const bool ok = mov < base * 2.5 + 0.02;
            std::printf ("click %6.0f Hz, blocchi casuali: curvatura max %.3f (fermo %.3f) %s\n", rate, mov, base, ok ? "OK" : "ERRORE"); if (! ok) ++fails;
        }
    }
    {   // latenza dichiarata e stato
        Processor p; p.prepareToPlay (sr, B); p.loadPreset (3); p.uiTheme = 1;
        juce::MemoryBlock mb; p.getStateInformation (mb); Processor q; q.setStateInformation (mb.getData(), (int) mb.getSize());
        const bool ok = p.getLatencySamples() == (int) std::ceil (0.045 * sr) && q.uiTheme == 1 && std::abs (q.value ("v1_on") - 1.0f) < 0.01f;
        std::printf ("latenza %d campioni, stato e tema ripristinati %s\n", p.getLatencySamples(), ok ? "OK" : "ERRORE"); if (! ok) ++fails;
    }
    std::printf (fails == 0 ? "TUTTO OK\n" : "ERRORI: %d\n", fails);
    return fails;
}
