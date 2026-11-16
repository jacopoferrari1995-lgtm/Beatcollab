#include "../Source/DnaProcessor.h"
#include "../Source/DnaEditor.h"
using namespace dna;
int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gi;
    const double sr = 44100, bpm = 90; const double spb = 60 / bpm; const int n = (int) (16 * spb * sr);
    juce::AudioBuffer<float> b (2, n); b.clear();
    // 4 battute: La minore, Fa, Do, Sol (accordi); cassa su 1 e 3, rullante su 2 e 4, hi-hat sugli ottavi
    const int chords[4][3] = { { 57, 60, 64 }, { 53, 57, 60 }, { 48, 52, 55 }, { 55, 59, 62 } };
    juce::Random r (3);
    for (int i = 0; i < n; ++i)
    {
        const double t = i / sr, beat = t / spb; const int bar = (int) (beat / 4) % 4; const double inBeat = std::fmod (beat, 1.0) * spb, in8 = std::fmod (beat * 2, 1.0) * spb / 2;
        float x = 0;
        for (int k = 0; k < 3; ++k) x += 0.08f * (float) std::sin (2 * 3.14159 * 440 * std::pow (2.0, (chords[bar][k] - 69) / 12.0) * t);
        const int bi = (int) beat % 4;
        if (bi == 0 || bi == 2) x += 0.7f * (float) (std::exp (-inBeat * 18) * std::sin (2 * 3.14159 * (50 + 80 * std::exp (-inBeat * 30)) * inBeat));
        if (bi == 1 || bi == 3) x += 0.4f * (float) std::exp (-inBeat * 25) * (r.nextFloat() * 2 - 1);
        x += 0.08f * (float) std::exp (-in8 * 80) * (r.nextFloat() * 2 - 1);
        b.setSample (0, i, x); b.setSample (1, i, x);
    }
    Processor p; p.prepareToPlay (48000, 512);
    p.loadBuffer (std::move (b), sr, "Test Loop");
    for (int i = 0; i < 500 && (p.busy.load() || p.analysis() == nullptr); ++i) juce::Thread::sleep (20);
    p.rechop();
    auto a = p.analysis();
    std::printf ("durata %.2f s, BPM %.2f (grezzo %.2f, loop %d da %d battiti), tonalita' %s %s conf %.2f\n", a->dur, a->bpm, a->bpmRaw, (int) a->loopFit, a->beats, pcName (a->key), a->minor ? "minore" : "maggiore", a->keyConf);
    for (size_t i = 0; i < a->onsets.size() && i < 12; ++i) std::printf ("  colpo %.2f beat forza %.2f\n", a->onsetSmp[i] / sr / spb, a->onsetStr[i]);
    std::printf ("colpi %d, cambi d'armonia %d:", (int) a->onsets.size(), (int) a->changes.size()); for (int c : a->changes) std::printf (" %.2fs", a->fSec (c)); std::printf ("\n");
    for (int m = 0; m < kModes; ++m)
    {
        p.setV ("chop", (float) m); p.rechop(); auto k = p.kit();
        std::printf ("%-11s %2d slice:", modeName (m), (int) k->starts.size()); for (int s : k->starts) std::printf (" %.2f", s / sr / spb); std::printf ("  (in battiti)\n");
    }
    p.setV ("chop", 0); p.rechop();
    auto mf = p.writeMidi(); std::printf ("MIDI: %s (%d byte)\n", mf.getFileName().toRawUTF8(), (int) mf.getSize());
    // suona slice 0..3 via MIDI
    juce::AudioBuffer<float> out (2, 512); juce::MidiBuffer mb; mb.addEvent (juce::MidiMessage::noteOn (1, 37, (juce::uint8) 100), 0); float pk = 0;
    for (int k = 0; k < 40; ++k) { p.processBlock (out, mb); mb.clear(); pk = std::max (pk, out.getMagnitude (0, 512)); }
    std::printf ("slice 2 suonata: picco %.2f\n", pk);
    if (argc > 1)
    {
        Editor ed (p); p.preview = 5; p.processBlock (out, mb);
        auto img = ed.createComponentSnapshot (ed.getLocalBounds(), true, 1.0f);
        juce::File f (argv[1]); f.deleteFile(); juce::FileOutputStream os (f); juce::PNGImageFormat().writeImageToStream (img, os);
    }
    return 0;
}
