#include "../Source/PadProcessor.h"
#include "../Source/PadEditor.h"
#include "../Source/Library.h"
using namespace jp;
int main (int argc, char** argv)
{
    if (argc > 2 && juce::String (argv[1]) == "snap")
    {
        juce::ScopedJuceInitialiser_GUI gui2;
        // loop sintetico: 4 battute a 92 BPM, nome senza BPM
        const double sr = 44100, bpm = 92; const int len = (int) (sr * 60.0 / bpm * 16);
        juce::AudioBuffer<float> b (2, len); b.clear(); juce::Random r (2);
        for (int beat = 0; beat < 16; ++beat)
        {
            const int st = (int) (beat * sr * 60.0 / bpm);
            for (int i = 0; i < (int) (sr * 0.3) && st + i < len; ++i)
            {
                const double t = i / sr; float v;
                if (beat % 2 == 0) v = (float) (std::sin (2 * 3.14159 * (50 + 100 * std::exp (-t * 30)) * t) * std::exp (-t * 7)) * 0.8f;
                else v = (r.nextFloat() * 2 - 1) * (float) std::exp (-t * 15) * 0.5f;
                b.addSample (0, st + i, v); b.addSample (1, st + i, v);
            }
            const int st2 = st + (int) (sr * 30.0 / bpm);
            for (int i = 0; i < (int) (sr * 0.05) && st2 + i < len; ++i) { const float v = (r.nextFloat() * 2 - 1) * (float) std::exp (-i / sr * 60) * 0.2f; b.addSample (0, st2 + i, v); b.addSample (1, st2 + i, v); }
        }
        juce::File wf ("/tmp/loop_test.wav"); wf.deleteFile();
        { juce::WavAudioFormat wav; std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (new juce::FileOutputStream (wf), sr, 2, 16, {}, 0)); w->writeFromAudioSampleBuffer (b, 0, len); }
        Processor p; p.prepareToPlay (48000, 480);
        p.loadFile (wf, 0);
        std::printf ("BPM rilevato %.2f  warp %d  mode %d\n", p.pad[0].get (SBPM), (int) p.pad[0].get (WARP), (int) p.pad[0].get (MODE));
        Editor ed (p);
        ed.openChop (0);
        auto img = ed.createComponentSnapshot (ed.getLocalBounds(), true, 1.0f);
        juce::File f (argv[2]); f.deleteFile(); juce::FileOutputStream os (f); juce::PNGImageFormat().writeImageToStream (img, os);
        return 0;
    }
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
        for (auto& [pd, str] : patternBank()[(size_t) i].lanes) if (std::strlen (str) != 32 && std::strlen (str) != 64) std::printf ("PATTERN ERRATO %s %s pad %d len %d\n", patternBank()[(size_t) i].genre, patternBank()[(size_t) i].name, pd, (int) std::strlen (str));
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
    // kit casuali
    for (int k = 0; k < 9; ++k)
    {
        randomKit (p, k % 3); float kp = 0; bool kn = false; int filled = 0;
        for (int pd = 0; pd < kPads; ++pd)
        {
            if (p.getSample (pd)) ++filled;
            juce::MidiBuffer m; m.addEvent (juce::MidiMessage::noteOn (1, 36 + pd, (juce::uint8) 110), 0);
            for (int b = 0; b < 20; ++b) { p.processBlock (blk, m); m.clear(); for (int i = 0; i < 512; ++i) { const float x = blk.getSample (0, i); if (! std::isfinite (x)) kn = true; kp = std::max (kp, std::abs (x)); } }
        }
        int hitCols = 0; for (int i = 0; i < Processor::kScope; ++i) if (p.scHit[i].load()) ++hitCols;
        std::printf ("random %-40s pad %d picco %.2f nan %d colonne-transienti %d\n", p.kitName.toRawUTF8(), filled, kp, (int) kn, hitCols);
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
    for (auto& c : libCategories()) { auto items = libItems (c); std::printf ("%s: %d suoni\n", c.toRawUTF8(), (int) items.size()); if (! items.empty()) loadLibItem (p, items.back(), 5); }
    for (int i = 0; i < (int) patternBank().size(); ++i) if (juce::String (patternBank()[(size_t) i].name).startsWith ("4 Bar")) { p.loadPattern (i); std::printf ("%s %s: %d battute, %d note\n", patternBank()[(size_t) i].genre, patternBank()[(size_t) i].name, p.patternBars.load(), (int) p.events.size()); }
    // note repeat: pad tenuto 1 battuta a 1/16 = 16 colpi
    {
        p.loopOn = false; p.apvts.getParameter ("nrOn")->setValueNotifyingHost (1); p.apvts.getParameter ("swing")->setValueNotifyingHost (0);
        p.requestPlay (false); { juce::MidiBuffer m; p.processBlock (blk, m); } p.requestPlay (true);
        const auto before = p.hits[3].load(); const double tps = 120.0 / 60 * PPQ / sr; const int nb = (int) (384 / tps / 512);
        for (int b = 0; b < nb; ++b) { juce::MidiBuffer m; if (b == 1) m.addEvent (juce::MidiMessage::noteOn (1, 39, (juce::uint8) 90), 0); if (b == nb - 1) m.addEvent (juce::MidiMessage::noteOff (1, 39), 0); p.processBlock (blk, m); }
        std::printf ("note repeat: %d colpi in ~1 battuta\n", (int) (p.hits[3].load() - before));
        p.apvts.getParameter ("nrOn")->setValueNotifyingHost (0);
    }
    juce::MemoryBlock mb; p.getStateInformation (mb); Processor q; q.setStateInformation (mb.getData(), (int) mb.getSize());
    std::printf ("stato ricaricato: eventi %d, nome pad1 %s\n", (int) q.events.size(), q.getSample (0) ? q.getSample (0)->name.toRawUTF8() : "-");
}
