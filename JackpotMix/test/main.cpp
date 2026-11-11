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
        if (auto* e = std::getenv ("JBAND")) { ed.graph.sel = atoi (e); ed.graph.updatePop(); }
        for (int i = 0; i < 40; ++i) { feed (p, 4, 0.3f, 220 + i * 13, ph); ed.timerCallback(); }
        auto img = ed.createComponentSnapshot (ed.getLocalBounds(), true, 1.0f);
        juce::File f (argv[2]); f.deleteFile(); juce::FileOutputStream os (f); juce::PNGImageFormat().writeImageToStream (img, os);
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "wet")
    {   // livello dell'effetto (send) rispetto al dry, con MIX al 100% e al default
        for (int t = 1; t < (int) mods().size(); ++t)
        {
            if (! mod (t).send) continue;
            for (float mx : { 1.0f, mod (t).defMix })
            {
                Processor p; p.prepareToPlay (44100, 512); p.insertModule (0, t); p.setP ("s1_mix", mx); pump (p);
                juce::Random r (3); juce::AudioBuffer<float> b (2, 512); juce::MidiBuffer m; double ei = 0, ew = 0;
                for (int k = 0; k < 400; ++k)
                {
                    float in[512]; for (int i = 0; i < 512; ++i) { in[i] = (r.nextFloat() - 0.5f) * 0.2f; b.setSample (0, i, in[i]); b.setSample (1, i, in[i]); }
                    p.processBlock (b, m);
                    if (k > 150) for (int i = 0; i < 512; ++i) { const float w = b.getSample (0, i) - in[i]; ei += in[i] * in[i]; ew += w * w; }
                }
                std::printf ("%-14s mix %3.0f%%: effetto %+6.1f dB rispetto al dry\n", mod (t).name.toRawUTF8(), mx * 100, juce::Decibels::gainToDecibels (std::sqrt (ew / ei)));
            }
        }
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "eqmax")
    {   // ogni EQ analogico: tutte le manopole di gain al massimo, poi al minimo
        for (int t = 1; t < (int) mods().size(); ++t)
        {
            if (mod (t).cat != C_EQ) continue;
            for (int dir : { 1, -1 })
            {
                juce::String line;
                for (double f : { 60.0, 200.0, 1000.0, 3000.0, 10000.0 })
                {
                    Processor p; p.prepareToPlay (44100, 512); p.insertModule (0, t);
                    for (int i = 0; i < kP; ++i) { const auto& sp = mod (t).spec[(size_t) i]; if (hidden (sp)) continue; juce::String l (sp.label);
                        if (sp.fmt == F_DB || l == "BOOST" || l == "PEAK" || l == "ATTEN" || l == "DIP") { const bool cut = l == "ATTEN" || l == "DIP"; const float v = sp.fmt == F_DB ? (dir > 0 ? sp.hi : sp.lo) : ((dir > 0) != cut ? sp.hi : sp.lo); p.setP ("s1_p" + juce::String (i + 1), v); } }
                    pump (p); double ph = 0; std::vector<float> o; feed (p, 60, 0.05f, f, ph, &o);
                    line << juce::String (f, 0) << "Hz " << juce::String (rmsDb (o, 15000) - juce::Decibels::gainToDecibels (0.05f / std::sqrt (2.0f)), 1) << "  ";
                }
                std::printf ("%-12s %s %s\n", mod (t).name.toRawUTF8(), dir > 0 ? "MAX" : "MIN", line.toRawUTF8());
            }
        }
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "pultec")
    {
        const int t = findMod ("EQP-1A");
        for (int fsel = 0; fsel < 4; ++fsel)
        {
            juce::String line;
            for (double f : { 20.0, 30.0, 60.0, 100.0, 200.0, 500.0 })
            {
                Processor p; p.prepareToPlay (44100, 512); p.insertModule (0, t); p.setP ("s1_p1", (float) fsel); p.setP ("s1_p2", 10); pump (p);
                double ph = 0; std::vector<float> o; feed (p, 80, 0.05f, f, ph, &o);
                line << juce::String (f, 0) << ":" << juce::String (rmsDb (o, 20000) - juce::Decibels::gainToDecibels (0.05f / std::sqrt (2.0f)), 1) << " ";
            }
            std::printf ("LOW FREQ %d BOOST 10: %s\n", fsel, line.toRawUTF8());
        }
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "specs")
    {
        for (int t = 1; t < (int) mods().size(); ++t)
        {
            juce::String l; int n = 0; for (int i = 0; i < kP; ++i) if (! hidden (mod (t).spec[(size_t) i])) { l << i << ":" << mod (t).spec[(size_t) i].label << "(" << mod (t).spec[(size_t) i].fmt << ") "; ++n; }
            std::printf ("%-14s n=%d %s\n", mod (t).name.toRawUTF8(), n, l.toRawUTF8());
        }
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
