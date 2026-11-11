#include "../Source/MixProcessor.h"
#include "../Source/MixEditor.h"
using namespace jmx;
static void feed (Processor& p, int blocks, float amp, double freq, double& ph, std::vector<float>* out = nullptr, bool* nan = nullptr)
{
    juce::AudioBuffer<float> b (4, 512); juce::MidiBuffer m; b.clear();
    for (int k = 0; k < blocks; ++k)
    {
        for (int i = 0; i < 512; ++i) { const float v = freq > 0 ? (float) std::sin (ph) * amp : 0.0f; ph += 2 * juce::MathConstants<double>::pi * freq / 44100.0; b.setSample (0, i, v); b.setSample (1, i, v * 0.9f); b.setSample (2, i, 0); b.setSample (3, i, 0); }
        juce::AudioBuffer<float> io (b.getArrayOfWritePointers(), 2, 512);
        p.processBlock (io, m);
        for (int i = 0; i < 512; ++i) { const float x = b.getSample (0, i); if (nan && ! std::isfinite (x)) *nan = true; if (out) out->push_back (x); }
    }
}
static float rmsDb (const std::vector<float>& v, size_t from) { double e = 0; for (size_t i = from; i < v.size(); ++i) e += v[i] * v[i]; return (float) juce::Decibels::gainToDecibels (std::sqrt (e / juce::jmax<size_t> (1, v.size() - from)), -200.0); }
static void pump (Processor& p) { p.forceRebuild(); }
int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    if (argc > 2 && juce::String (argv[1]) == "snap")
    {
        Processor p; p.prepareToPlay (44100, 512);
        p.loadPreset (juce::SystemStats::getEnvironmentVariable ("JPRESET", "0").getIntValue()); pump (p);
        double ph = 0; juce::Random r;
        Editor ed (p); ed.setSize (Editor::W, Editor::H);
        if (auto* e = std::getenv ("JSEL")) ed.selectSlot (atoi (e));
        if (auto* e = std::getenv ("JBAND")) ed.selectBand (atoi (e));
        for (int i = 0; i < 40; ++i) { feed (p, 4, 0.3f, 220 + i * 13, ph); ed.timerCallback(); }
        auto img = ed.createComponentSnapshot (ed.getLocalBounds(), true, 1.0f);
        juce::File f (argv[2]); f.deleteFile(); juce::FileOutputStream os (f); juce::PNGImageFormat().writeImageToStream (img, os);
        return 0;
    }
    // neutro
    { Processor p; p.prepareToPlay (44100, 512); pump (p); double ph = 0; std::vector<float> o; feed (p, 100, 0.3f, 1000, ph, &o);
      std::printf ("neutro (vuoto): %+.3f dB (atteso %+.3f) lat %d\n", rmsDb (o, 20000), juce::Decibels::gainToDecibels (0.3f / std::sqrt (2.0f)), p.getLatencySamples()); }
    // ogni modulo da solo
    for (int t = 1; t < (int) mods().size(); ++t)
    {
        Processor p; p.prepareToPlay (44100, 512); p.insertModule (0, t); pump (p);
        double ph = 0; std::vector<float> o; bool nan = false; feed (p, 120, 0.25f, 440, ph, &o, &nan);
        std::vector<float> s; feed (p, 200, 0, 0, ph, &s, &nan);
        std::printf ("%-16s %+7.2f dB  coda %+7.1f dB  lat %5d %s\n", mod (t).name.toRawUTF8(), rmsDb (o, 30000) - juce::Decibels::gainToDecibels (0.25f / std::sqrt (2.0f)), rmsDb (s, 80000), p.getLatencySamples(), nan ? "NAN!" : "");
    }
    // preset
    int bad = 0;
    for (int i = 0; i < (int) factoryPresets().size(); ++i)
    {
        Processor p; p.prepareToPlay (44100, 512); p.loadPreset (i); pump (p);
        double ph = 0; std::vector<float> o; bool nan = false; feed (p, 100, 0.2f, 330, ph, &o, &nan);
        float pk = 0; for (size_t k = 20000; k < o.size(); ++k) pk = juce::jmax (pk, std::abs (o[k]));
        const float d = rmsDb (o, 20000) - juce::Decibels::gainToDecibels (0.2f / std::sqrt (2.0f));
        const bool warn = nan || std::abs (d) > 9 || pk > 1.2f;
        if (warn) ++bad;
        std::printf ("%s%-10s %-26s n=%d %+6.2f dB pk %.2f lat %d\n", warn ? "!! " : "   ", factoryPresets()[(size_t) i].cat.toRawUTF8(), factoryPresets()[(size_t) i].name.toRawUTF8(), p.chainLen(), d, pk, p.getLatencySamples());
    }
    std::printf ("preset sospetti: %d / %d\n", bad, (int) factoryPresets().size());
    return 0;
}
