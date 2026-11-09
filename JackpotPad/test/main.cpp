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
    // registrazione: nota suonata in ritardo di 7 tick sul secondo sedicesimo, swing 66
    p.apvts.getParameter ("swing")->setValueNotifyingHost (p.apvts.getParameter ("swing")->convertTo0to1 (66));
    p.requestPlay (true); p.recording = true;
    const double tps = 120.0 / 60 * PPQ / sr; int hitsPlayed = 0; std::vector<int> onsets;
    for (int b = 0; b < 400; ++b)
    {
        juce::MidiBuffer m;
        const double t0 = b * 512 * tps, t1 = t0 + 512 * tps;
        for (double target : { 0.0 + 3, 24.0 + 7, 48.0 - 4 }) if (target >= t0 && target < t1 && b < 50) m.addEvent (juce::MidiMessage::noteOn (1, 37, (juce::uint8) 90), (int) ((target - t0) / tps));
        p.processBlock (blk, m);
        for (int c = 0; c < 2; ++c) for (int i = 0; i < 512; ++i) { const float x = blk.getSample (c, i); if (! std::isfinite (x)) nan = true; peak = std::max (peak, std::abs (x)); }
        if (b == 60) p.recording = false;
    }
    std::printf ("eventi registrati %d, quantizzati:", (int) p.events.size());
    for (auto& e : p.events) std::printf (" %.1f->%.1f", e.tick, p.quantize (e.tick));
    std::printf ("\npeak %.3f nan %d\n", peak, (int) nan);
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
