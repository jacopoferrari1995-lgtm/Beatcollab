#include "../Source/PadProcessor.h"
using namespace jp;
int main()
{
    juce::ScopedJuceInitialiser_GUI gui;
    Processor p; const double sr = 44100; p.prepareToPlay (sr, 512);
    juce::AudioBuffer<float> blk (2, 512); float peak = 0; bool nan = false;
    // velocity: picco del kick a diverse velocity
    for (int vel : { 127, 100, 64, 30 })
    {
        juce::MidiBuffer m; m.addEvent (juce::MidiMessage::noteOn (1, 36, (juce::uint8) vel), 0);
        float pk = 0; for (int b = 0; b < 20; ++b) { p.processBlock (blk, m); m.clear(); for (int i = 0; i < 512; ++i) pk = std::max (pk, std::abs (blk.getSample (0, i))); }
        std::printf ("kick vel %3d -> picco %.3f (%.1f dB)\n", vel, pk, juce::Decibels::gainToDecibels (pk));
    }
    // swing sulle note MIDI: nota sul secondo sedicesimo, swing 66, trasporto interno
    p.apvts.getParameter ("swing")->setValueNotifyingHost (p.apvts.getParameter ("swing")->convertTo0to1 (66));
    p.requestPlay (true);
    { juce::MidiBuffer m; p.processBlock (blk, m); }
    {
        const double tps = 120.0 / 60 * PPQ / sr; double t = 512 * tps; int onsetBlock = -1, onsetPos = -1;
        // nota a tick 24 (secondo sedicesimo): attesa a 24 + 0.32*24 = 31.7 tick
        const int b0 = (int) (24.0 / tps) / 512, off = (int) (24.0 / tps) % 512;
        for (int b = 1; b < 24; ++b)
        {
            juce::MidiBuffer m; if (b == b0) m.addEvent (juce::MidiMessage::noteOn (1, 37, (juce::uint8) 100), off);
            const auto before = p.hits[1].load(); p.processBlock (blk, m);
            if (p.hits[1].load() != before && onsetBlock < 0) { onsetBlock = b; for (int i = 0; i < 512; ++i) if (std::abs (blk.getSample (0, i)) > 1e-4f) { onsetPos = i; break; } }
            t += 512 * tps;
        }
        const double got = (onsetBlock * 512 + onsetPos) * tps;
        std::printf ("swing MIDI: nota a 24 tick suonata a %.1f tick (atteso ~31.7)\n", got);
    }
    // banca loop
    for (int i = 0; i < (int) patternBank().size(); ++i)
        for (auto& [pd, str] : patternBank()[(size_t) i].lanes) if (std::strlen (str) != 32) std::printf ("PATTERN ERRATO %s %s pad %d len %d\n", patternBank()[(size_t) i].genre, patternBank()[(size_t) i].name, pd, (int) std::strlen (str));
    p.loadPattern (4); p.loopOn = true; p.varyPattern();
    std::printf ("pattern caricato: %d note\n", (int) p.events.size());
    // ogni kit, ogni pad, con catena attiva
    for (int k = 0; k < (int) factoryKits().size(); ++k)
    {
        p.loadFactoryKit (k); float kp = 0; bool kn = false;
        for (int pd = 0; pd < kPads; ++pd)
        {
            juce::MidiBuffer m; m.addEvent (juce::MidiMessage::noteOn (1, 36 + pd, (juce::uint8) 110), 0);
            for (int b = 0; b < 30; ++b) { p.processBlock (blk, m); m.clear(); for (int i = 0; i < 512; ++i) { const float x = blk.getSample (0, i); if (! std::isfinite (x)) kn = true; kp = std::max (kp, std::abs (x)); } }
        }
        std::printf ("kit %-22s picco %.2f nan %d\n", p.kitName.toRawUTF8(), kp, (int) kn);
    }
    // stretch: loop a 90 bpm suonato a 120 (interno)
    {
        p.loadFactoryKit (0); p.pad[0].v[SBPM] = 90; p.pad[0].v[WARP] = 2; p.pad[0].v[LOOP] = 1; p.pad[0].v[MODE] = 1;
        p.pad[0].v[CMODE] = 3; p.pad[0].v[LOWG] = 6; p.pad[0].v[DRIVE] = 0.5f; p.pad[0].v[BITS] = 0.5f; p.pad[0].v[RATE] = 0.5f; p.pad[0].v[SREV] = 0.5f; p.pad[0].v[SDLY] = 0.5f;
        p.apvts.getParameter ("busMode")->setValueNotifyingHost (1.0f); p.apvts.getParameter ("clip")->setValueNotifyingHost (1.0f);
        juce::MidiBuffer m; m.addEvent (juce::MidiMessage::noteOn (1, 36, (juce::uint8) 120), 0); float sp = 0; bool sn = false;
        for (int b = 0; b < 200; ++b) { p.processBlock (blk, m); m.clear(); if (b == 150) m.addEvent (juce::MidiMessage::noteOff (1, 36), 0); for (int i = 0; i < 512; ++i) { const float x = blk.getSample (0, i); if (! std::isfinite (x)) sn = true; sp = std::max (sp, std::abs (x)); } }
        std::printf ("stretch+catena: picco %.2f nan %d\n", sp, (int) sn);
    }
    bool c; std::printf ("bpm: %.1f %.1f\n", Processor::estimateBpm ("loop 92bpm", 5.2, c), Processor::estimateBpm ("break", 4.0, c));
    juce::MemoryBlock mb; p.getStateInformation (mb); Processor q; q.setStateInformation (mb.getData(), (int) mb.getSize());
    std::printf ("stato ricaricato: eventi %d, nome pad1 %s\n", (int) q.events.size(), q.getSample (0) ? q.getSample (0)->name.toRawUTF8() : "-");
}
