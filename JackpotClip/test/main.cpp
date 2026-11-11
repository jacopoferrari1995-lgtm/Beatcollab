#include "../Source/ClipProcessor.h"
#include "../Source/ClipEditor.h"
using namespace jclp;
static void setP (Processor& p, const char* id, float v) { auto* r = p.apvts.getParameter (id); r->setValueNotifyingHost (r->convertTo0to1 (v)); }
int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    if (argc > 2 && juce::String (argv[1]) == "snap")
    {
        Processor p; p.prepareToPlay (44100, 512);
        p.loadPreset (juce::SystemStats::getEnvironmentVariable ("JPRESET", "0").getIntValue());
        juce::AudioBuffer<float> b (2, 512); juce::MidiBuffer m; double ph = 0;
        for (int k = 0; k < 400; ++k) { for (int i = 0; i < 512; ++i) { const float v = (float) (std::sin (ph) * (0.5 + 0.5 * std::sin (ph * 0.0013))) ; ph += 0.06; b.setSample (0, i, v); b.setSample (1, i, v); } p.processBlock (b, m); }
        Editor ed (p); ed.setSize (Editor::W, Editor::H);
        auto img = ed.createComponentSnapshot (ed.getLocalBounds(), true, 1.0f);
        juce::File f (argv[2]); f.deleteFile(); juce::FileOutputStream os (f); juce::PNGImageFormat().writeImageToStream (img, os);
        return 0;
    }
    const double sr = 44100;
    auto run = [&] (std::function<void (Processor&)> setup, float amp, double freq, float& rmsDb, float& pk, float& thd)
    {
        Processor p; p.prepareToPlay (sr, 512); setup (p);
        juce::AudioBuffer<float> b (2, 512); juce::MidiBuffer m; double ph = 0, ei = 0, eo = 0; pk = 0; bool nan = false;
        std::vector<float> out;
        for (int k = 0; k < 200; ++k)
        {
            for (int i = 0; i < 512; ++i) { const float v = (float) std::sin (ph) * amp; ph += 2 * juce::MathConstants<double>::pi * freq / sr; b.setSample (0, i, v); b.setSample (1, i, v); if (k > 40) ei += v * v; }
            p.processBlock (b, m);
            for (int i = 0; i < 512; ++i) { const float x = b.getSample (0, i); if (! std::isfinite (x)) nan = true; if (k > 40) { eo += x * x; pk = std::max (pk, std::abs (x)); out.push_back (x); } }
        }
        rmsDb = (float) juce::Decibels::gainToDecibels (std::sqrt (eo / ei)); if (nan) rmsDb = -999;
        // thd: energia fuori dalla fondamentale (Goertzel)
        double re = 0, im = 0; for (size_t i = 0; i < out.size(); ++i) { const double a = 2 * juce::MathConstants<double>::pi * freq * i / sr; re += out[i] * std::cos (a); im += out[i] * std::sin (a); }
        const double fund = 2 * (re * re + im * im) / out.size() / out.size(), tot = eo / out.size() * 1.0;
        thd = (float) (100.0 * std::sqrt (std::max (0.0, tot - fund) / std::max (1e-12, fund)));
    };
    float r, pk, thd;
    run ([] (Processor&) {}, 0.5f, 220, r, pk, thd); std::printf ("neutro: %+.3f dB picco %.3f thd %.3f%%\n", r, pk, thd);
    for (int m = 0; m < kModes; ++m)
    {
        run ([m] (Processor& p) { setP (p, "mode", (float) m); setP (p, "drive", 6); setP (p, "ceil", -1); }, 0.7f, 100, r, pk, thd);
        std::printf ("%-9s drive 6: %+.2f dB picco %.3f (tetto %.3f) thd %.1f%%\n", modeName (m), r, pk, juce::Decibels::decibelsToGain (-1.0f), thd);
    }
    for (int i = 0; i < (int) presets().size(); ++i) { run ([i] (Processor& p) { p.loadPreset (i); }, 0.7f, 80, r, pk, thd); std::printf ("preset %-30s %+.2f dB picco %.3f thd %.1f%%\n", presets()[(size_t) i].name, r, pk, thd); }
    // velocita
    {
        Processor p; p.prepareToPlay (sr, 512); setP (p, "os", 3); setP (p, "split", 1); setP (p, "drive", 8);
        juce::AudioBuffer<float> b (2, 512); juce::MidiBuffer m; juce::Random rr;
        const auto t0 = juce::Time::getMillisecondCounterHiRes();
        for (int k = 0; k < 860; ++k) { for (int i = 0; i < 512; ++i) { b.setSample (0, i, rr.nextFloat() - 0.5f); b.setSample (1, i, rr.nextFloat() - 0.5f); } p.processBlock (b, m); }
        std::printf ("CPU 10 s di audio (8x + split): %.0f ms\n", juce::Time::getMillisecondCounterHiRes() - t0);
    }
}
