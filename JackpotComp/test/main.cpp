#include "../Source/CompProcessor.h"
#include <cstdio>
using namespace jc;
int main()
{
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
            maxGr = std::max (maxGr, p.grA.load());
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
        std::printf ("preset %-34s picco %.2f GR A %.1f B %.1f  out-in %+.1f dB nan %d\n", presets()[(size_t) i].name, pk, p.avgGrA.load(), p.avgGrB.load(), juce::Decibels::gainToDecibels (std::sqrt (p.outRms.load() / p.inRms.load())), (int) nan);
    }
    std::printf ("latenza %d\n", Processor().getLatencySamples());
    return 0;
}
