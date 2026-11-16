#include "../Source/SpaceProcessor.h"
#include "../Source/SpaceEditor.h"
using namespace jsp;
int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gi;
    const double sr = 48000;
    auto run = [&] (Processor& p, int blocks, std::vector<float>& out, bool impulse = false)
    {
        juce::AudioBuffer<float> b (2, 512); juce::Random r (3);
        for (int k = 0; k < blocks; ++k)
        {
            for (int i = 0; i < 512; ++i) { const int n = k * 512 + i; const double t = n / sr;
                float v = impulse ? (n == 4096 ? 1.0f : 0.0f) : (float) ((r.nextFloat() * 2 - 1) * std::exp (-std::fmod (t * 2.0, 1.0) * 25) * 0.5 + std::sin (2 * 3.14159 * 220 * t) * 0.1 * (std::fmod (t, 2.0) < 1));
                b.setSample (0, i, v); b.setSample (1, i, v); }
            juce::MidiBuffer m; p.processBlock (b, m);
            for (int i = 0; i < 512; ++i) { out.push_back (b.getSample (0, i)); out.push_back (b.getSample (1, i)); }
        }
    };
    if (argc > 1 && juce::String (argv[1]) == "dsp")
    {
        // risposta all'impulso di ogni motore da solo, 100% bagnato: RT60 misurato, picco, correlazione L/R, NaN
        for (int t = 1; t < kNumTypes; ++t)
        {
            Processor p; p.prepareToPlay (sr, 512);
            for (int k = 0; k < kSlots; ++k) p.setSlot (k, k == 0 ? t : OFF);
            p.P (mixId (0))->setValueNotifyingHost (1.0f); p.P ("dry")->setValueNotifyingHost (0.0f);
            if (isDelay (t)) p.setP (0, 2, 0.0f);   // libero
            if (t == CONV) { p.updateIRs(); juce::Thread::sleep (600); }
            std::vector<float> o; run (p, 600, o, true);
            const size_t n = o.size() / 2; std::vector<double> e (n); double tot = 0, pk = 0, lr = 0, ll = 0, rr = 0; bool nan = false;
            for (size_t i = 4100; i < n; ++i) { const double l = o[2 * i], r = o[2 * i + 1]; if (! std::isfinite (l) || ! std::isfinite (r)) nan = true; e[i] = l * l + r * r; tot += e[i]; pk = std::max (pk, std::max (std::abs (l), std::abs (r))); lr += l * r; ll += l * l; rr += r * r; }
            // integrazione di Schroeder: tempo per -5 -> -35 dB, estrapolato a 60
            double acc = tot, t5 = -1, t35 = -1; for (size_t i = 4100; i < n; ++i) { const double db = 10 * std::log10 (acc / tot + 1e-30); if (t5 < 0 && db < -5) t5 = i / sr; if (t35 < 0 && db < -35) { t35 = i / sr; break; } acc -= e[i]; }
            float pdec[kP]; for (int i = 0; i < kP; ++i) pdec[i] = p.value (pId (0, i));
            std::printf ("%-10s decay impostato %-8s RT60 misurato %.2f s  energia %.1f dB  picco %.3f  corr L/R %.2f  nan %d\n", info (t).name, paramText (t, 0, pdec[0], pdec).toRawUTF8(),
                         t35 > 0 ? (t35 - t5) * 2.0 : -1.0, 10 * std::log10 (tot), pk, lr / std::sqrt (ll * rr + 1e-30), (int) nan);
        }
        // preset con musica: picco e NaN
        for (int i = 0; i < (int) factoryPresets().size(); ++i)
        {
            Processor p; p.prepareToPlay (sr, 512); p.loadFactory (i); p.updateIRs(); juce::Thread::sleep (300);
            std::vector<float> o; run (p, 800, o); bool nan = false; float pk = 0; for (float x : o) { if (! std::isfinite (x)) nan = true; pk = std::max (pk, std::abs (x)); }
            double e = 0; for (float x : o) e += (double) x * x; std::printf ("%-10s %-24s picco %.2f  rms %.1f dB  nan %d\n", factoryPresets()[(size_t) i].cat.toRawUTF8(), factoryPresets()[(size_t) i].name.toRawUTF8(), pk, 10 * std::log10 (e / o.size() + 1e-20), (int) nan);
        }
        // feedback al massimo + freeze: deve restare limitato
        {
            Processor p; p.prepareToPlay (sr, 512); p.setSlot (0, TAPE); p.setP (0, 1, 1.0f); p.setSlot (1, HALL); p.setSlot (2, SHIMMER); p.setP (2, 2, 1.0f); p.setSlot (3, SPACED); p.setP (3, 1, 1.0f); p.setSlot (4, MULTITAP); p.setP (4, 1, 1.0f); p.setSlot (5, REVERSE); p.setP (5, 1, 1.0f); p.P ("freeze")->setValueNotifyingHost (1.0f);
            std::vector<float> o; run (p, 2000, o); float pk = 0; bool nan = false; for (float x : o) { if (! std::isfinite (x)) nan = true; pk = std::max (pk, std::abs (x)); }
            std::printf ("feedback max + freeze, 21 s: picco %.2f nan %d\n", pk, (int) nan);
        }
        {
            Processor p; p.prepareToPlay (sr, 512); for (int k = 0; k < kSlots; ++k) p.setSlot (k, OFF);
            p.setSlot (0, SHIMMER); p.setP (0, 0, 1.0f); p.setP (0, 2, 1.0f); p.setP (0, 4, 1.0f); p.P (mixId (0))->setValueNotifyingHost (1.0f);
            std::vector<float> o; run (p, 3000, o); float pk = 0, pkEnd = 0; for (size_t i = 0; i < o.size(); ++i) { pk = std::max (pk, std::abs (o[i])); if (i > o.size() * 9 / 10) pkEnd = std::max (pkEnd, std::abs (o[i])); }
            std::printf ("shimmer al massimo 32 s: picco %.2f, picco nell'ultimo 10%% %.2f\n", pk, pkEnd);
        }
        { Processor p; p.prepareToPlay (sr, 512); p.loadFactory (4); p.swapSlots (0, 1); std::printf ("scambio 1<->2: %s %s\n", info (p.slotType (0)).name, info (p.slotType (1)).name); }
        return 0;
    }
    if (argc > 2 && juce::String (argv[1]) == "snap")
    {
        Processor p; p.prepareToPlay (sr, 512);
        if (auto* th = std::getenv ("JTHEME")) p.uiTheme = std::atoi (th);
        if (auto* pr = std::getenv ("JPR")) p.loadFactory (std::atoi (pr));
        std::vector<float> o; run (p, 400, o);
        Editor ed (p);
        if (std::getenv ("JADV")) ed.advPanel.open (std::atoi (std::getenv ("JADV")));
        auto img = ed.createComponentSnapshot (ed.getLocalBounds(), true, 1.0f);
        juce::File f (argv[2]); f.deleteFile(); juce::FileOutputStream os (f); juce::PNGImageFormat().writeImageToStream (img, os);
        return 0;
    }
    return 0;
}
