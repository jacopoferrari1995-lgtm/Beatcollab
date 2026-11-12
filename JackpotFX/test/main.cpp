#include "../Source/FxProcessor.h"
#include "../Source/FxEditor.h"
using namespace jfx;
int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gi;
    const double sr = 48000;
    auto run = [&] (Processor& p, int blocks, std::vector<float>& out, std::function<void (int, juce::MidiBuffer&)> midi = nullptr)
    {
        juce::AudioBuffer<float> b (2, 512); juce::Random r (3);
        for (int k = 0; k < blocks; ++k)
        {
            for (int i = 0; i < 512; ++i) { const int n = k * 512 + i; const double t = n / sr; const float beat = (float) std::fmod (t * 2.0, 1.0);
                const float v = (float) (std::sin (2 * 3.14159 * 55 * t) * std::exp (-beat * 8) * 0.5 + (r.nextFloat() * 2 - 1) * std::exp (-std::fmod (t * 4.0, 1.0) * 30) * 0.2 + std::sin (2 * 3.14159 * 440 * t) * 0.05);
                b.setSample (0, i, v); b.setSample (1, i, v * 0.9f); }
            juce::MidiBuffer m; if (midi) midi (k, m);
            p.processBlock (b, m);
            for (int i = 0; i < 512; ++i) out.push_back (b.getSample (0, i));
        }
    };
    if (argc > 1 && juce::String (argv[1]) == "dsp")
    {
        Processor p0; p0.prepareToPlay (sr, 512); std::vector<float> ref; run (p0, 200, ref);
        // neutro: deve essere identico all'ingresso
        Processor pn; pn.prepareToPlay (sr, 512); std::vector<float> a; run (pn, 200, a);
        std::printf ("preset di partenza a riposo: uguale all'ingresso %s\n", a == ref ? "si" : "no");
        for (int i = 0; i < (int) factoryPresets().size(); ++i)
        {
            Processor p; p.prepareToPlay (sr, 512); p.loadFactory (i);
            for (int k = 0; k < kSlots; ++k) { const int t = p.slotType (k); if (! pfx::info (t).spring) p.P (amtId (k))->setValueNotifyingHost (pfx::info (t).bipolar ? 0.8f : 0.6f); }
            std::vector<float> o; run (p, 300, o); bool nan = false; float pk = 0; for (float x : o) { if (! std::isfinite (x)) nan = true; pk = std::max (pk, std::abs (x)); }
            std::printf ("%-22s tutti gli slot alzati: picco %.2f nan %d\n", factoryPresets()[(size_t) i].name, pk, (int) nan);
        }
        Processor pc; pc.prepareToPlay (sr, 512); std::vector<float> c; run (pc, 120, c, [] (int k, juce::MidiBuffer& m) { if (k == 20) m.addEvent (juce::MidiMessage::controllerEvent (1, 76, 127), 0); });
        std::printf ("CC 76 -> slot 7 (stutter): valore in arrivo %.2f\n", pc.ccPend[6].load());
        Processor pd; pd.prepareToPlay (sr, 512); pd.ctrlOn = false; std::vector<float> d; run (pd, 30, d, [] (int k, juce::MidiBuffer& m) { if (k == 2) m.addEvent (juce::MidiMessage::controllerEvent (1, 76, 127), 0); });
        std::printf ("MIDI OFF: CC ignorato %s\n", pd.ccPend[6].load() < 0 ? "si" : "no");
        pc.swapSlots (0, 4); std::printf ("scambio 1<->5: slot1 %s, slot5 %s\n", pfx::info (pc.slotType (0)).name, pfx::info (pc.slotType (4)).name);
        return 0;
    }
    if (argc > 2 && juce::String (argv[1]) == "snap")
    {
        Processor p; p.prepareToPlay (sr, 512);
        if (auto* pr = std::getenv ("JPR")) p.loadFactory (std::atoi (pr));
        p.P (amtId (2))->setValueNotifyingHost (0.55f); p.P (amtId (0))->setValueNotifyingHost (0.3f);
        std::vector<float> o; run (p, 400, o);
        Editor ed (p); for (int i = 0; i < 10; ++i) ed.repaint();
        auto img = ed.createComponentSnapshot (ed.getLocalBounds(), true, 1.0f);
        juce::File f (argv[2]); f.deleteFile(); juce::FileOutputStream os (f); juce::PNGImageFormat().writeImageToStream (img, os);
        return 0;
    }
    return 0;
}
