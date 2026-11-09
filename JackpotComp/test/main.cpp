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
        for (int i = 0; i < p.getNumPrograms(); ++i) if (p.getProgramName (i).contains ("Voce a 4")) p.loadPreset (i);
        Editor ed (p);
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
        auto img = ed.createComponentSnapshot (ed.getLocalBounds(), true, 1.0f);
        juce::File f (argv[2]); f.deleteFile(); juce::FileOutputStream os (f); juce::PNGImageFormat().writeImageToStream (img, os);
        return 0;
    }
    juce::ScopedJuceInitialiser_GUI gui;
    const double sr = 44100;
    // per ogni modello: sinusoide a -12 dBFS con burst a -3 dBFS, misura GR e NaN
    for (int m = 0; m < kNumModels; ++m)
    {
        Processor p; p.prepareToPlay (sr, 512);
        auto set = [&p] (const char* id, float v) { auto* r = p.apvts.getParameter (id); r->setValueNotifyingHost (r->convertTo0to1 (v)); };
        set ("a_model", (float) m); set ("a_drive", 0.5f); set ("a_ratio", 1);
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
