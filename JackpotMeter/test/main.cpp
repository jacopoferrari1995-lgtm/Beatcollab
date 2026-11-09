#include "../Source/MeterProcessor.h"
#include "../Source/MeterEditor.h"
#include <cstdio>
using namespace jm;
int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    if (argc > 2 && juce::String (argv[1]) == "snap")
    {
        Processor p; p.prepareToPlay (48000, 480);
        auto set = [&p] (const char* id, float v) { auto* r = p.apvts.getParameter (id); r->setValueNotifyingHost (r->convertTo0to1 (v)); };
        set ("b1_on", 1); set ("b3_on", 1); set ("b3_freq", 350); set ("b3_gain", -3); set ("b3_dyn", -6); set ("b3_q", 1.5f); set ("b5_on", 1); set ("b5_type", BELL); set ("b5_freq", 5000); set ("b5_gain", 2.5f);
        juce::AudioBuffer<float> b (2, 480); juce::MidiBuffer m; juce::Random r (3); float pk[2] = {}; double ph = 0;
        Editor ed (p);
        for (int k = 0; k < 100 * 12; ++k)
        {
            for (int i = 0; i < 480; ++i)
            {
                const long n = (long) k * 480 + i; const double t = n / 48000.0, bt = std::fmod (t * 2.33, 1.0);
                const float w = r.nextFloat() * 2 - 1; pk[0] = 0.98f * pk[0] + 0.02f * w; pk[1] = 0.7f * pk[1] + 0.3f * (r.nextFloat() * 2 - 1);
                const float kick = (float) (std::sin (2 * 3.14159 * (50 + 120 * std::exp (-bt * 30)) * bt) * std::exp (-bt * 5)) * 0.5f;
                ph += 2 * 3.14159 * 220 / 48000; const float pad = 0.08f * (float) std::sin (ph) + 0.05f * (float) std::sin (ph * 1.5);
                const float L = kick + pad + pk[0] * 0.6f + pk[1] * 0.05f, R = kick + pad * 0.8f + pk[0] * 0.5f - pk[1] * 0.05f;
                b.setSample (0, i, L); b.setSample (1, i, R);
            }
            p.processBlock (b, m);
            if (k % 3 == 0) { ed.spec.analyse(); }
        }
        for (int k = 0; k < 20; ++k) ed.spec.analyse();
        auto img = ed.createComponentSnapshot (ed.getLocalBounds(), true, 1.0f);
        juce::File f (argv[2]); f.deleteFile(); juce::FileOutputStream os (f); juce::PNGImageFormat().writeImageToStream (img, os);
        return 0;
    }
    const double sr = 48000;
    // 1) sinusoide 1 kHz a -20 dBFS stereo: deve dare circa -20 + 3.01 - 0.691 + K(1k)= ~ -17.0 LUFS? (stereo: -20 dBFS sin = -23 dB RMS, x2 canali +3 = -20 LUFS + 0.7 di K a 1 kHz)
    {
        Processor p; p.prepareToPlay (sr, 480);
        juce::AudioBuffer<float> b (2, 480); juce::MidiBuffer m; double ph = 0; const float a = juce::Decibels::decibelsToGain (-20.0f);
        for (int k = 0; k < 100 * 20; ++k)   // 20 s
        {
            for (int i = 0; i < 480; ++i) { const float v = a * (float) std::sin (ph); ph += 2 * 3.14159265 * 997 / sr; b.setSample (0, i, v); b.setSample (1, i, v); }
            p.processBlock (b, m);
        }
        std::printf ("sinusoide 997 Hz -20 dBFS stereo: M %.2f S %.2f I %.2f LUFS (atteso ~ -20.0)  TP %.2f  corr %.2f largh %.2f LRA %.2f\n", p.lufsM.load(), p.lufsS.load(), p.lufsI.load(), p.tpMax.load(), p.corr.load(), p.width.load(), p.lra.load());
    }
    // 2) true peak: sinusoide a fs/4 con fase 45 gradi: picco campionato -3 dB, true peak 0
    {
        Processor p; p.prepareToPlay (sr, 480);
        juce::AudioBuffer<float> b (2, 480); juce::MidiBuffer m; long n = 0;
        for (int k = 0; k < 200; ++k)
        {
            for (int i = 0; i < 480; ++i, ++n) { const float v = 0.9f * (float) std::sin (3.14159265 / 2 * n + 3.14159265 / 4); b.setSample (0, i, v); b.setSample (1, i, -v); }
            p.processBlock (b, m);
        }
        std::printf ("fs/4 a 45 gradi, ampiezza 0.9: picco campioni %.2f dB, true peak %.2f dB (atteso ~ -0.9)  corr %.2f\n", p.pkMax.load(), p.tpMax.load(), p.corr.load());
    }
    // 3) EQ dinamico: banda bell a 1k, dyn -12, soglia -30: il segnale forte deve essere attenuato
    {
        Processor p; p.prepareToPlay (sr, 480);
        auto set = [&p] (const char* id, float v) { auto* r = p.apvts.getParameter (id); r->setValueNotifyingHost (r->convertTo0to1 (v)); };
        set ("b3_on", 1); set ("b3_type", BELL); set ("b3_freq", 1000); set ("b3_gain", 0); set ("b3_q", 1); set ("b3_dyn", -12); set ("b3_thr", -30);
        juce::AudioBuffer<float> b (2, 480); juce::MidiBuffer m; double ph = 0; float pk = 0; bool nan = false;
        for (int k = 0; k < 400; ++k)
        {
            for (int i = 0; i < 480; ++i) { const float v = 0.5f * (float) std::sin (ph); ph += 2 * 3.14159265 * 1000 / sr; b.setSample (0, i, v); b.setSample (1, i, v); }
            p.processBlock (b, m);
            if (k > 300) for (int i = 0; i < 480; ++i) { if (! std::isfinite (b.getSample (0, i))) nan = true; pk = std::max (pk, std::abs (b.getSample (0, i))); }
        }
        std::printf ("EQ dinamico: in 0.50 -> out %.3f (%.1f dB), dynGain %.1f dB, nan %d\n", pk, juce::Decibels::gainToDecibels (pk / 0.5f), p.dynGain[2].load(), (int) nan);
    }
    std::printf ("latenza %d\n", Processor().getLatencySamples());
    return 0;
}
