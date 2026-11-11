#include "../Source/TubeProcessor.h"
#include "../Source/TubeEditor.h"
using namespace jtbx;
static void setP (Processor& p, const juce::String& id, float v) { auto* r = p.apvts.getParameter (id); r->setValueNotifyingHost (r->convertTo0to1 (v)); }
int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    if (argc > 2 && juce::String (argv[1]) == "snap")
    {
        Processor p; p.prepareToPlay (44100, 512); p.loadPreset (juce::SystemStats::getEnvironmentVariable ("JPRESET", "0").getIntValue());
        juce::AudioBuffer<float> b (2, 512); juce::MidiBuffer m; juce::Random r;
        for (int k = 0; k < 100; ++k) { for (int i = 0; i < 512; ++i) { const float v = (r.nextFloat() - 0.5f) * 0.4f; b.setSample (0, i, v); b.setSample (1, i, v * 0.8f); } p.processBlock (b, m); }
        Editor ed (p); ed.setSize (Editor::W, Editor::H);
        auto img = ed.createComponentSnapshot (ed.getLocalBounds(), true, 1.0f);
        juce::File f (argv[2]); f.deleteFile(); juce::FileOutputStream os (f); juce::PNGImageFormat().writeImageToStream (img, os);
        return 0;
    }
    const double sr = 44100;
    auto run = [&] (std::function<void (Processor&)> setup, float amp, double freq, float& rmsDb, float& thd, float& h2, float& h3)
    {
        Processor p; p.prepareToPlay (sr, 512); setup (p);
        juce::AudioBuffer<float> b (2, 512); juce::MidiBuffer m; double ph = 0, ei = 0, eo = 0; bool nan = false; std::vector<float> out;
        for (int k = 0; k < 200; ++k)
        {
            for (int i = 0; i < 512; ++i) { const float v = (float) std::sin (ph) * amp; ph += 2 * juce::MathConstants<double>::pi * freq / sr; b.setSample (0, i, v); b.setSample (1, i, v); if (k > 40) ei += v * v; }
            p.processBlock (b, m);
            for (int i = 0; i < 512; ++i) { const float x = b.getSample (0, i); if (! std::isfinite (x)) nan = true; if (k > 40) { eo += x * x; out.push_back (x); } }
        }
        rmsDb = nan ? -999.0f : (float) juce::Decibels::gainToDecibels (std::sqrt (eo / ei));
        auto harm = [&] (double f) { double re = 0, im = 0; for (size_t i = 0; i < out.size(); ++i) { const double a = 2 * juce::MathConstants<double>::pi * f * i / sr; re += out[i] * std::cos (a); im += out[i] * std::sin (a); } return 2 * std::sqrt (re * re + im * im) / out.size(); };
        const double f1 = harm (freq); h2 = (float) (100 * harm (freq * 2) / f1); h3 = (float) (100 * harm (freq * 3) / f1);
        const double tot = eo / out.size(); thd = (float) (100.0 * std::sqrt (std::max (0.0, tot - f1 * f1 / 2) / std::max (1e-12, f1 * f1 / 2)));
    };
    float r, t, h2, h3;
    run ([] (Processor&) {}, 0.3f, 1000, r, t, h2, h3); std::printf ("spento: %+.3f dB thd %.3f%%\n", r, t);
    run ([] (Processor& p) { setP (p, "mb_on", 1); }, 0.3f, 1000, r, t, h2, h3); std::printf ("enhancer acceso a zero: %+.3f dB thd %.3f%%\n", r, t);
    for (double f : { 60.0, 300.0, 1500.0, 5000.0, 12000.0 }) { run ([] (Processor& p) { setP (p, "mb_on", 1); }, 0.3f, f, r, t, h2, h3); std::printf ("  piatto a %5.0f Hz: %+.3f dB\n", f, r); }
    for (float pent : { 5.0f, 7.0f, 9.0f })
        for (float amp : { 0.1f, 0.5f })
        { run ([pent] (Processor& p) { setP (p, "hg_on", 1); setP (p, "pentode", pent); }, amp, 220, r, t, h2, h3); std::printf ("HG pentode %.0f ampiezza %.1f: %+.2f dB thd %.2f%% (2a %.2f%% 3a %.2f%%)\n", pent, amp, r, t, h2, h3); }
    for (float tri : { 7.0f, 9.0f }) { run ([tri] (Processor& p) { setP (p, "hg_on", 1); setP (p, "triode", tri); }, 0.3f, 220, r, t, h2, h3); std::printf ("HG triode %.0f: %+.2f dB thd %.2f%% (2a %.2f%% 3a %.2f%%)\n", tri, r, t, h2, h3); }
    for (int fr = 0; fr < 3; ++fr) for (bool alt : { false, true }) { run ([fr, alt] (Processor& p) { setP (p, "hg_on", 1); setP (p, "sat_in", 1); setP (p, "sat", 7); setP (p, "satFreq", (float) fr); setP (p, "alt", alt ? 1.0f : 0.0f); }, 0.3f, 220, r, t, h2, h3); std::printf ("SAT 7 freq %d alt %d: %+.2f dB thd %.2f%% (2a %.2f%% 3a %.2f%%)\n", fr, (int) alt, r, t, h2, h3); }
    run ([] (Processor& p) { setP (p, "hg_on", 1); setP (p, "density", 8); }, 0.3f, 220, r, t, h2, h3); std::printf ("DENSITY 8: %+.2f dB thd %.2f%%\n", r, t);
    for (int c = 0; c < 3; ++c) { run ([c] (Processor& p) { setP (p, "mb_on", 1); setP (p, "char", (float) c); for (int k = 1; k <= 5; ++k) setP (p, "h" + juce::String (k), 0.6f); }, 0.3f, 220, r, t, h2, h3); std::printf ("enhancer carattere %d armoniche 60%%: %+.2f dB thd %.2f%% (2a %.2f%% 3a %.2f%%)\n", c, r, t, h2, h3); }
    for (int i = 0; i < (int) presets().size(); ++i) { run ([i] (Processor& p) { p.loadPreset (i); }, 0.3f, 150, r, t, h2, h3); std::printf ("preset %-34s %+.2f dB thd %.2f%%\n", presets()[(size_t) i].name, r, t); }
    {
        Processor p; p.prepareToPlay (sr, 512); p.loadPreset ((int) presets().size() - 1);
        juce::AudioBuffer<float> b (2, 512); juce::MidiBuffer m; juce::Random rr; const auto t0 = juce::Time::getMillisecondCounterHiRes();
        for (int k = 0; k < 860; ++k) { for (int i = 0; i < 512; ++i) { b.setSample (0, i, rr.nextFloat() - 0.5f); b.setSample (1, i, rr.nextFloat() - 0.5f); } p.processBlock (b, m); }
        std::printf ("CPU 10 s di audio (valvole + enhancer): %.0f ms\n", juce::Time::getMillisecondCounterHiRes() - t0);
    }
}
