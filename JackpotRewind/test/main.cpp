#include "../Source/RewindProcessor.h"
#include "../Source/RewindEditor.h"
using namespace jrwd;
struct Head : juce::AudioPlayHead
{
    double ppq = 0, bpm = 92; bool play = true;
    juce::Optional<PositionInfo> getPosition() const override { PositionInfo p; p.setBpm (bpm); p.setPpqPosition (ppq); p.setIsPlaying (play); p.setTimeSignature (TimeSignature { 4, 4 }); return p; }
};
int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    const double sr = 44100;
    Processor p; Head h; p.setPlayHead (&h); p.prepareToPlay (sr, 512);
    juce::AudioBuffer<float> b (2, 512); juce::MidiBuffer m; double ph = 0; bool same = true;
    // 40 s: silenzio, 12 s di accordo di La minore, silenzio, 8 s di beat, silenzio
    const int total = (int) (sr * 40) / 512;
    for (int k = 0; k < total; ++k)
    {
        const double t0 = k * 512 / sr;
        for (int i = 0; i < 512; ++i)
        {
            const double t = t0 + i / sr; float v = 0;
            if (t > 4 && t < 16) v = (float) (0.2 * (std::sin (ph) + std::sin (ph * 1.189) + std::sin (ph * 1.498)));   // A C E
            if (t > 22 && t < 30) { const double bt = std::fmod (t * 92 / 60, 1.0); v = (float) (bt < 0.1 ? std::sin (t * 2 * 3.14159 * 55) * (1 - bt * 10) : 0); }
            ph += 2 * juce::MathConstants<double>::pi * 220 / sr;
            b.setSample (0, i, v); b.setSample (1, i, v);
        }
        juce::AudioBuffer<float> in; in.makeCopyOf (b);
        p.processBlock (b, m);
        for (int i = 0; i < 512; ++i) if (b.getSample (0, i) != in.getSample (0, i)) same = false;
        h.ppq += 512 / sr * h.bpm / 60.0;
    }
    std::printf ("uscita identica all'ingresso: %d   memoria %.0f s   scritti %.1f s\n", (int) same, p.capacity() / sr, p.written() / sr);
    Editor ed (p); ed.setSize (Editor::W, Editor::H);
    ed.resized();
    // prese trovate
    std::printf ("prese: %d\n", (int) ed.takes.size());
    for (auto& t : ed.takes) std::printf ("  %.2f s -> %.2f s  (%s)\n", t.a / sr, t.b / sr, ed.barsText (t.a, t.b).toRawUTF8());
    if (! ed.takes.empty())
    {
        ed.setSel (ed.takes[0].a, ed.takes[0].b);
        juce::AudioBuffer<float> sel; p.readRange (ed.selA, ed.selB, sel);
        std::printf ("selezione agganciata: %.3f s -> %.3f s  tonalita %s\n", ed.selA / sr, ed.selB / sr, p.keyOf (sel).toRawUTF8());
        auto f = ed.exportSel(); std::printf ("file: %s (%lld byte)\n", f.getFileName().toRawUTF8(), (long long) f.getSize());
        p.preview (ed.selA, ed.selB, false);
        for (int k = 0; k < 20; ++k) { b.clear(); p.processBlock (b, m); } std::printf ("ascolto: picco %.2f\n", b.getMagnitude (0, 512));
    }
    if (argc > 1)
    {
        auto img = ed.createComponentSnapshot (ed.getLocalBounds(), true, 1.0f);
        juce::File f (argv[1]); f.deleteFile(); juce::FileOutputStream os (f); juce::PNGImageFormat().writeImageToStream (img, os);
    }
}
