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
        for (auto& c : ed.cards) c->mouth = std::getenv ("JSING") ? 0.8f : 0.0f;
        auto img = ed.createComponentSnapshot (ed.getLocalBounds(), true, std::getenv ("JX2") ? 2.0f : 1.0f);
        juce::File f (argc > 2 ? argv[2] : "/tmp/crew.png"); f.deleteFile(); juce::FileOutputStream os (f); juce::PNGImageFormat().writeImageToStream (img, os);
        std::printf ("snap %s\n", f.getFullPathName().toRawUTF8());
        return 0;
    }
    int fails = 0;
    {   // partenza neutra
        Processor p; p.prepareToPlay (sr, B); juce::AudioBuffer<float> b (2, B); juce::MidiBuffer m; double ph = 0, err = 0;
        for (int k = 0; k < 60; ++k) { voice (b, sr, ph, k * B); juce::AudioBuffer<float> in; in.makeCopyOf (b); p.processBlock (b, m);
            for (int c = 0; c < 2; ++c) for (int i = 0; i < B; ++i) err = std::max (err, (double) std::abs (b.getSample (c, i) - in.getSample (c, i))); }
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
    std::printf (fails == 0 ? "TUTTO OK\n" : "ERRORI: %d\n", fails);
    return fails;
}
