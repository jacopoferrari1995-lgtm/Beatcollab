#include "../Source/PluginProcessor.h"
using namespace j60;
int main()
{
    juce::ScopedJuceInitialiser_GUI gui;
    Processor p; p.syncHost = false; p.prepareToPlay (44100, 512);
    auto set = [&] (const char* id, float v) { auto* pr = p.apvts.getParameter (id); pr->setValueNotifyingHost (pr->convertTo0to1 (v)); };
    j60::Burro::applyPreset (p.apvts, std::getenv ("BP") ? atoi (std::getenv ("BP")) : 1);
    p.pad[1].sendRev = 0.8f; p.pad[2].sendDly = 0.6f; p.pad[2].sendRev = 0.5f;
    // pad loop sincronizzato
    p.setSample (20, p.getSample (15)); p.pad[20].loop = 1; p.pad[20].sync = 1; p.pad[20].bpm = 120; p.pad[20].mode = 1;
    p.requestPlayStart();
    juce::AudioBuffer<float> out (2, 44100 * 8), blk (2, 512); juce::MidiBuffer m;
    float peak = 0; bool nan = false;
    for (int pos = 0; pos + 512 <= out.getNumSamples(); pos += 512)
    {
        if (pos == 512 * 172) p.toggleResample();
        if (pos == 512 * 86) p.uiPadDown (20, 110);
        if (pos == 512 * 258) p.uiPadUp (20);
        p.processBlock (blk, m);
        for (int c = 0; c < 2; ++c) for (int i = 0; i < 512; ++i) { const float x = blk.getSample (c, i); if (! std::isfinite (x)) nan = true; peak = std::max (peak, std::abs (x)); }
        out.copyFrom (0, pos, blk, 0, 0, 512); out.copyFrom (1, pos, blk, 1, 0, 512);
    }
    std::printf ("peak %.3f nan %d rsState %d rsLen %d\n", peak, (int) nan, p.rsState.load(), p.rsLen.load());
    auto r = p.takeResample(); std::printf ("resample %s %.2fs path %s\n", r ? "ok" : "none", r ? r->buf.getNumSamples() / 44100.0 : 0.0, r ? r->path.toRawUTF8() : "");
    bool c; for (auto t : { std::make_pair ("Loop Am 140bpm", 3.0), std::make_pair ("drill_142_BPM", 5.0), std::make_pair ("Vox 100", 0.8), std::make_pair ("break", 4.0), std::make_pair ("chords 90", 10.6667), std::make_pair ("melody", 6.857) })
    { float b = Processor::estimateBpm (t.first, t.second, c); std::printf ("%s %.2fs -> %.2f %s\n", t.first, t.second, b, c ? "sicuro" : ""); }
    juce::WavAudioFormat w; juce::File f ("/tmp/j60test.wav"); f.deleteFile();
    std::unique_ptr<juce::AudioFormatWriter> wr (w.createWriterFor (new juce::FileOutputStream (f), 44100, 2, 16, {}, 0)); wr->writeFromAudioSampleBuffer (out, 0, out.getNumSamples());
}
