#include "../Source/ClipProcessor.h"
#include "../Source/ClipEditor.h"
#include "TestLab.h"
using namespace jclp;
static void setP (Processor& p, const char* id, float v) { auto* r = p.apvts.getParameter (id); r->setValueNotifyingHost (r->convertTo0to1 (v)); }
int main (int argc, char** argv)
{
    if (argc > 1 && juce::String (argv[1]) == "osresp")
    {
        for (int k : { 2, 3 }) for (double fr : { 20000.0, 24000.0, 26000.0, 28000.0, 30000.0, 34000.0, 40000.0 })
        {
            juce::dsp::Oversampling<float> o (1, (size_t) k, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, true);
            o.initProcessing (4096); juce::AudioBuffer<float> b (1, 4096); b.clear(); juce::dsp::AudioBlock<float> blk (b);
            float pk = 0;
            for (int it = 0; it < 4; ++it) { auto up = o.processSamplesUp (blk); float* d = up.getChannelPointer (0); const int N = (int) up.getNumSamples(); const double fs = 48000.0 * (1 << k);
                for (int n = 0; n < N; ++n) d[n] = (float) std::sin (2 * juce::MathConstants<double>::pi * fr * (it * N + n) / fs);
                o.processSamplesDown (blk); if (it == 3) for (int n = 0; n < 4096; ++n) pk = juce::jmax (pk, std::abs (b.getSample (0, n))); }
            std::printf ("%dx %6.0f Hz -> %.1f dB\n", 1 << k, fr, juce::Decibels::gainToDecibels (pk));
        }
        return 0;
    }

    if (argc > 1 && juce::String (argv[1]) == "gate")
    {
        auto go = [] (bool fw, int mode) { jclp::Engine::forceWet = fw; Processor p; p.prepareToPlay (44100, 512); setP (p, "mode", (float) mode);
            std::vector<float> o; juce::AudioBuffer<float> b (2, 512); juce::MidiBuffer m; juce::Random r (3);
            for (int k = 0; k < 400; ++k) { for (int i = 0; i < 512; ++i) { const int t = k * 512 + i; float x = 0.3f * std::sin (t * 0.031f) + 0.05f * std::sin (t * 0.43f); (void) r; if (t % 22050 < 300) x *= 4.0f * std::exp (-(t % 22050) / 80.0f) + 1.0f; b.setSample (0, i, x); b.setSample (1, i, x * 0.9f); } p.processBlock (b, m); for (int i = 0; i < 512; ++i) o.push_back (b.getSample (0, i)); }
            return o; };
        for (int mode : { 2, 6, 7, 0 })
        {
            auto a = go (false, mode), b = go (true, mode); double e = 0, s2 = 0, mx = 0; for (size_t i = 0; i < a.size(); ++i) { e += (a[i] - b[i]) * (a[i] - b[i]); s2 += b[i] * b[i]; mx = juce::jmax (mx, (double) std::abs (a[i] - b[i])); }
            std::printf ("%-8s porta vs sempre acceso: %.1f dB (max %.1f dBFS)\n", jclp::modeName (mode), 10 * std::log10 (e / s2 + 1e-30), 20 * std::log10 (mx + 1e-12));
        }
        return 0;
    }

    if (auto* t = std::getenv ("JTHEME")) skin::cur() = std::atoi (t);
    juce::ScopedJuceInitialiser_GUI gui;
    if (argc > 1 && juce::String (argv[1]) == "lab")
    {
        const int pr = juce::SystemStats::getEnvironmentVariable ("JPRESET", "0").getIntValue();
        std::printf ("preset: %s\n", presets()[(size_t) pr].name);
        testlab::run ([&] (double rate, int blk) -> std::unique_ptr<juce::AudioProcessor>
        {
            auto p = std::make_unique<Processor>(); p->setPlayConfigDetails (2, 2, rate, blk); p->prepareToPlay (rate, blk); p->loadPreset (pr);
            if (auto* js = std::getenv ("JSET")) for (auto kv : juce::StringArray::fromTokens (js, ",", "")) setP (*p, kv.upToFirstOccurrenceOf ("=", false, false).toRawUTF8(), kv.fromFirstOccurrenceOf ("=", false, false).getFloatValue());
            return p;
        }, std::getenv ("JNOCLICK") == nullptr);
        return 0;
    }
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
