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
    juce::MemoryBlock mb; p.getStateInformation (mb); Processor q; q.setStateInformation (mb.getData(), (int) mb.getSize());
    std::printf ("stato ricaricato: eventi %d, nome pad1 %s\n", (int) q.events.size(), q.getSample (0)->name.toRawUTF8());
}
