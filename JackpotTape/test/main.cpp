#include "../Source/TapeProcessor.h"
using namespace jt;
int main (int argc, char** argv)
{
    const int FX = argc > 1 ? atoi (argv[1]) : -1;
    juce::ScopedJuceInitialiser_GUI gui;
    Processor p; const double sr = 44100; p.prepareToPlay (sr, 512);
    juce::AudioBuffer<float> blk (2, 512); juce::MidiBuffer m;
    const int total = (int) (sr * 12); float peak = 0; bool nan = false; double ph = 0;
    juce::AudioBuffer<float> out (2, total);
    if (FX < 0) { p.loadPreset (14); auto* sq = p.apvts.getParameter ("seqOn"); sq->setValueNotifyingHost (1); }
    for (int pos = 0; pos + 512 <= total; pos += 512)
    {
        for (int i = 0; i < 512; ++i) { const double t = (pos + i) / sr; const double beat = std::fmod (t * 140 / 60, 1.0);
            const float v = (float) (std::sin (ph) * 0.4 + (beat < 0.05 ? std::sin (t * 2 * 3.14159 * 60) * (1 - beat / 0.05) * 0.5 : 0)); ph += 2 * 3.14159 * 220 / sr;
            blk.setSample (0, i, v); blk.setSample (1, i, v); }
        const int b = pos / 512;
        if (FX < 0) { for (int f = 0; f < kNumFx; ++f) p.uiPad[f] = (b / 20) % 12 == f + 1 && (b % 20) < 14; }
        else p.uiPad[FX] = pos >= sr * 2 && pos < sr * 4;
        p.processBlock (blk, m);
        for (int c = 0; c < 2; ++c) for (int i = 0; i < 512; ++i) { const float x = blk.getSample (c, i); if (! std::isfinite (x)) nan = true; peak = std::max (peak, std::abs (x)); }
        out.copyFrom (0, pos, blk, 0, 0, 512); out.copyFrom (1, pos, blk, 1, 0, 512);
    }
    std::printf ("peak %.3f nan %d programs %d\n", peak, (int) nan, p.getNumPrograms());
    if (FX >= 0) { for (int k = 18; k < 46; ++k) { double e = 0; const int a = (int) (k * sr / 10); for (int i = 0; i < 4410; ++i) e += out.getSample (0, a + i) * out.getSample (0, a + i); std::printf ("%.2f ", std::sqrt (e / 4410)); } std::printf ("\n"); }
    juce::WavAudioFormat w; juce::File f ("/tmp/tapetest.wav"); f.deleteFile();
    std::unique_ptr<juce::AudioFormatWriter> wr (w.createWriterFor (new juce::FileOutputStream (f), sr, 2, 16, {}, 0)); wr->writeFromAudioSampleBuffer (out, 0, total);
}
