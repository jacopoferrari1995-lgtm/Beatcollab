#include "../Source/FluxProcessor.h"
#include "../Source/FluxEditor.h"
#include <map>
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
    {   // ogni note-on ha il suo note-off dopo, mai una nota tagliata da un note-off vecchio; MIDI identico con buffer irregolari
        auto check = [&] (int blk, bool uneven, juce::uint32& h)
        {
            Processor q; q.setV ("density", 0.95f); q.setV ("energy", 0.9f); q.setV ("movement", 0.9f); PH ph; q.setPlayHead (&ph); q.prepareToPlay (48000, 4096);
            juce::Random rb (5); std::map<int, int> open; int bad = 0; juce::int64 t = 0; h = 0;
            while (t < (juce::int64) (48000 * 30))
            {
                const int n = uneven ? 1 + rb.nextInt (blk) : blk; ph.ppq = t / 48000.0 * 140 / 60;
                juce::AudioBuffer<float> b (2, n); juce::MidiBuffer m; q.processBlock (b, m);
                for (auto ev : m) { auto msg = ev.getMessage(); const int k = msg.getChannel() * 128 + msg.getNoteNumber();
                    if (msg.isNoteOn()) { if (open[k] > 0) ++bad; open[k]++; h = hsh (h, (juce::uint32) k, (juce::uint32) (t + ev.samplePosition)); }
                    else if (msg.isNoteOff() && open[k] > 0) open[k]--; }
                t += n;
            }
            q.setPlayHead (nullptr); return bad;
        };
        juce::uint32 ha, hb; const int b1 = check (512, false, ha), b2 = check (512, true, hb);
        std::printf ("note riaccese senza chiudere: %d / %d, MIDI con buffer irregolari: %s\n", b1, b2, ha == hb ? "identico" : "DIVERSO");
    }
    if (argc > 1)
    {
        PH ph; ph.ppq = 5.3; p.setPlayHead (&ph); juce::AudioBuffer<float> b (2, 512); juce::MidiBuffer m; p.processBlock (b, m);
        if (auto* t = std::getenv ("JTHEME")) skin::cur() = std::atoi (t);
        Editor ed (p); if (auto* t = std::getenv ("JTHEME")) { skin::cur() = std::atoi (t); neo::K::theme = -1; skin::apply(); } auto img = ed.createComponentSnapshot (ed.getLocalBounds(), true, 1.0f);
        juce::File fo (argv[1]); fo.deleteFile(); juce::FileOutputStream os (fo); juce::PNGImageFormat().writeImageToStream (img, os);
        p.setPlayHead (nullptr);
    }
    return 0;
}
