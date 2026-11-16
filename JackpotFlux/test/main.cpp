#include "../Source/FluxProcessor.h"
#include "../Source/FluxEditor.h"
using namespace flux;
struct PH : juce::AudioPlayHead
{
    double ppq = 0, bpm = 140; bool play = true;
    juce::Optional<PositionInfo> getPosition() const override { PositionInfo p; p.setIsPlaying (play); p.setPpqPosition (ppq); p.setBpm (bpm); return p; }
};
struct Count { int on[17] {}, off[17] {}; int total() const { int t = 0; for (int c = 1; c <= 16; ++c) t += on[c]; return t; } };
static Count run (Processor& p, int bars, juce::uint32* hash = nullptr)
{
    PH ph; p.setPlayHead (&ph); p.prepareToPlay (48000, 512);
    Count c; juce::AudioBuffer<float> b (2, 512); juce::uint32 h = 0;
    const int blocks = (int) (bars * 4 * 60.0 / 140 * 48000 / 512);
    for (int k = 0; k < blocks; ++k)
    {
        ph.ppq = k * 512 / 48000.0 * 140 / 60; juce::MidiBuffer m; p.processBlock (b, m);
        for (auto ev : m) { auto msg = ev.getMessage(); if (msg.isNoteOn()) { c.on[msg.getChannel()]++; h = hsh (h, (juce::uint32) msg.getNoteNumber(), (juce::uint32) (k * 512 + ev.samplePosition), (juce::uint32) msg.getVelocity()); } else if (msg.isNoteOff()) c.off[msg.getChannel()]++; }
    }
    ph.play = false; { juce::MidiBuffer m; p.processBlock (b, m); for (auto ev : m) if (ev.getMessage().isNoteOff()) c.off[ev.getMessage().getChannel()]++; }
    p.setPlayHead (nullptr); if (hash) *hash = h; return c;
}
static void show (const char* t, const Count& c) { std::printf ("%-28s", t); for (int l = 0; l < NL; ++l) std::printf (" %s=%d", laneName (l), c.on[l + 1]); std::printf ("  tot %d\n", c.total()); }
int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gi;
    Processor p;
    juce::uint32 h1, h2;
    auto c = run (p, 8, &h1); show ("TRAP, macro di partenza", c);
    int on = 0, off = 0; for (int i = 1; i <= 16; ++i) { on += c.on[i]; off += c.off[i]; } std::printf ("note on %d, note off %d (devono pareggiare o off >= on)\n", on, off);
    run (p, 8, &h2); std::printf ("stesso seme due volte: %s\n", h1 == h2 ? "identico" : "DIVERSO");
    p.setV ("density", 0.1f); show ("DENSITY 10%", run (p, 8));
    p.setV ("density", 0.9f); show ("DENSITY 90%", run (p, 8));
    p.setV ("density", 0.5f); p.setV ("space", 0.9f); show ("SPACE 90%", run (p, 8));
    p.setV ("space", 0.0f); p.setV ("movement", 0.9f); p.setV ("energy", 0.9f); show ("MOVEMENT + ENERGY 90%", run (p, 8));
    p.variate(); juce::uint32 h3; run (p, 8, &h3); std::printf ("dopo VARIATE: %s\n", h3 != h2 ? "cambia" : "uguale");
    p.loadStarter (DRILL, 0, true); p.setV ("movement", 0.3f); p.setV ("energy", 0.5f); show ("DRILL in C minore", run (p, 8));
    // le note pitchate devono stare nella scala di Do minore
    { auto S = p.snap(); int out = 0; static const bool inS[12] = { 1,0,1,1,0,1,0,1,1,0,1,0 }; for (int l : { BASS, MEL, COUNTER }) for (auto& n : S->lane[(size_t) l]) if (! inS[n.note % 12]) ++out; std::printf ("note fuori scala nel pattern: %d\n", out); }
    p.loadStarter (BOOMBAP, 9, true); show ("BOOM BAP", run (p, 8));
    auto f = p.exportMidi (8); std::printf ("MIDI: %s (%d byte)\n", f.getFileName().toRawUTF8(), (int) f.getSize());
    std::printf ("import corsia MELODIA dal file esportato: %s\n", p.importLane (MEL, f) ? "ok" : "no");
    p.loadStarter (TRAP, 9, true);
    if (argc > 1)
    {
        PH ph; ph.ppq = 5.3; p.setPlayHead (&ph); juce::AudioBuffer<float> b (2, 512); juce::MidiBuffer m; p.processBlock (b, m);
        Editor ed (p); auto img = ed.createComponentSnapshot (ed.getLocalBounds(), true, 1.0f);
        juce::File fo (argv[1]); fo.deleteFile(); juce::FileOutputStream os (fo); juce::PNGImageFormat().writeImageToStream (img, os);
        p.setPlayHead (nullptr);
    }
    return 0;
}
