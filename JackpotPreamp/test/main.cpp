#include "../Source/PreProcessor.h"
#include "../Source/PreEditor.h"
using namespace jpre;
static void setP (Processor& p, const char* id, float v) { auto* r = p.apvts.getParameter (id); r->setValueNotifyingHost (r->convertTo0to1 (v)); }
int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    if (argc > 2 && juce::String (argv[1]) == "snap")
    {
        Processor p; p.prepareToPlay (44100, 512);
        p.loadPreset (juce::SystemStats::getEnvironmentVariable ("JPRESET", "0").getIntValue());
        juce::AudioBuffer<float> b (2, 512); juce::MidiBuffer m; juce::Random r;
        for (int k = 0; k < 100; ++k) { for (int i = 0; i < 512; ++i) { const float v = (r.nextFloat() - 0.5f) * 0.3f; b.setSample (0, i, v); b.setSample (1, i, v); } p.processBlock (b, m); }
        { auto im = alook::Art::knob (0), jw = alook::Art::jewel (1, true); std::printf ("knob %d %dx%d  jewel %d %dx%d\n", (int) im.isValid(), im.getWidth(), im.getHeight(), (int) jw.isValid(), jw.getWidth(), jw.getHeight()); }
        Editor ed (p); ed.setSize (Editor::W, Editor::H);
        if (auto* e = std::getenv ("JBAND")) ed.selectBand (atoi (e));
        for (int i = 0; i < 20; ++i) { for (int k = 0; k < 4; ++k) { for (int j = 0; j < 512; ++j) { const float v = (r.nextFloat() - 0.5f) * 0.3f; b.setSample (0, j, v); b.setSample (1, j, v); } p.processBlock (b, m); } }
        auto img = ed.createComponentSnapshot (ed.getLocalBounds(), true, 1.0f);
        juce::File f (argv[2]); f.deleteFile(); juce::FileOutputStream os (f); juce::PNGImageFormat().writeImageToStream (img, os);
        return 0;
    }
    const double sr = 44100;
    auto run = [&] (std::function<void (Processor&)> setup, float amp, double freq, float& rmsDb, float& thd, float& pk)
    {
        Processor p; p.prepareToPlay (sr, 512); setup (p);
        juce::AudioBuffer<float> b (2, 512); juce::MidiBuffer m; double ph = 0, ei = 0, eo = 0; bool nan = false; std::vector<float> out; pk = 0;
        for (int k = 0; k < 200; ++k)
        {
            for (int i = 0; i < 512; ++i) { const float v = (float) std::sin (ph) * amp; ph += 2 * juce::MathConstants<double>::pi * freq / sr; b.setSample (0, i, v); b.setSample (1, i, v); if (k > 40) ei += v * v; }
            p.processBlock (b, m);
            for (int i = 0; i < 512; ++i) { const float x = b.getSample (0, i); if (! std::isfinite (x)) nan = true; if (k > 40) { eo += x * x; out.push_back (x); pk = std::max (pk, std::abs (x)); } }
        }
        rmsDb = nan ? -999.0f : (float) juce::Decibels::gainToDecibels (std::sqrt (eo / ei));
        double re = 0, im = 0; for (size_t i = 0; i < out.size(); ++i) { const double a = 2 * juce::MathConstants<double>::pi * freq * i / sr; re += out[i] * std::cos (a); im += out[i] * std::sin (a); }
        const double fund = 2 * (re * re + im * im) / out.size() / out.size(), tot = eo / out.size();
        thd = (float) (100.0 * std::sqrt (std::max (0.0, tot - fund) / std::max (1e-12, fund)));
    };
    float r, thd, pk;
    run ([] (Processor&) {}, 0.3f, 1000, r, thd, pk); std::printf ("neutro: %+.3f dB thd %.3f%%\n", r, thd);
    for (int m = 0; m < kPreModels; ++m)
        for (float g : { 0.0f, 12.0f, 24.0f })
        {
            float r1, t1, r2, t2, r3, t3;
            run ([m, g] (Processor& p) { setP (p, "p_on", 1); setP (p, "p_model", (float) m); setP (p, "p_gain", g); }, 0.25f, 100, r1, t1, pk);
            run ([m, g] (Processor& p) { setP (p, "p_on", 1); setP (p, "p_model", (float) m); setP (p, "p_gain", g); }, 0.25f, 1000, r2, t2, pk);
            run ([m, g] (Processor& p) { setP (p, "p_on", 1); setP (p, "p_model", (float) m); setP (p, "p_gain", g); }, 0.25f, 10000, r3, t3, pk);
            std::printf ("%-8s gain %2.0f: 100Hz %+.2f dB thd %.2f%% | 1k %+.2f thd %.2f%% | 10k %+.2f\n", preInfo (m).name, g, r1, t1, r2, t2, r3);
        }
    for (int i = 0; i < (int) presets().size(); ++i) { run ([i] (Processor& p) { p.loadPreset (i); }, 0.25f, 120, r, thd, pk); std::printf ("preset %-34s %+.2f dB thd %.2f%% picco %.2f\n", presets()[(size_t) i].name, r, thd, pk); }
    {
        Processor p; p.prepareToPlay (sr, 512); p.loadPreset (1);
        juce::AudioBuffer<float> b (2, 512); juce::MidiBuffer m; juce::Random rr;
        const auto t0 = juce::Time::getMillisecondCounterHiRes();
        for (int k = 0; k < 860; ++k) { for (int i = 0; i < 512; ++i) { b.setSample (0, i, rr.nextFloat() - 0.5f); b.setSample (1, i, rr.nextFloat() - 0.5f); } p.processBlock (b, m); }
        std::printf ("CPU 10 s di audio: %.0f ms\n", juce::Time::getMillisecondCounterHiRes() - t0);
    }
}
