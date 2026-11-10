#include "../Source/TapeProcessor.h"
#include "../Source/TapeEditor.h"
using namespace jt;
int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    if (argc > 2 && juce::String (argv[1]) == "snap")
    {
        Processor p; p.prepareToPlay (44100, 512);
        const int pr = juce::SystemStats::getEnvironmentVariable ("JPRESET", "0").getIntValue(); p.loadPreset (pr);
        std::unique_ptr<juce::AudioProcessorEditor> ed (p.createEditor());
        if (auto* e = dynamic_cast<Editor*> (ed.get())) e->select (juce::SystemStats::getEnvironmentVariable ("JFX", "9").getIntValue());
        ed->setSize (Editor::W, Editor::H);
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
        juce::File f (argv[2]); f.deleteFile(); juce::FileOutputStream os (f); juce::PNGImageFormat().writeImageToStream (img, os);
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "level")
    {
        auto run = [] (std::function<void (Processor&)> setup)
        {
            Processor p; const double sr = 44100; p.prepareToPlay (sr, 512); setup (p);
            juce::AudioBuffer<float> b (2, 512); juce::MidiBuffer m; double ph = 0, ei = 0, eo = 0; float pk = 0;
            for (int k = 0; k < 300; ++k)
            {
                for (int i = 0; i < 512; ++i) { const float v = (float) (std::sin (ph) * 0.35 + std::sin (ph * 0.25) * 0.2); ph += 2 * 3.14159 * 220 / sr; b.setSample (0, i, v); b.setSample (1, i, v); if (k > 50) ei += v * v; }
                p.processBlock (b, m);
                if (k > 50) for (int i = 0; i < 512; ++i) { eo += b.getSample (0, i) * b.getSample (0, i); pk = std::max (pk, std::abs (b.getSample (0, i))); }
            }
            return std::make_pair ((float) juce::Decibels::gainToDecibels (std::sqrt (eo / ei)), pk);
        };
        for (float a : { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f }) { auto r = run ([a] (Processor& p) { p.applyAge (a); }); std::printf ("age %.2f  guadagno %+.1f dB  picco %.2f\n", a, r.first, r.second); }
        for (float d : { 0.0f, 0.2f, 0.5f, 0.8f, 1.0f }) { auto r = run ([d] (Processor& p) { auto* pr = p.apvts.getParameter ("drive"); pr->setValueNotifyingHost (d); }); std::printf ("drive %.2f guadagno %+.1f dB  picco %.2f\n", d, r.first, r.second); }
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "gen")
    {
        Processor p; const double sr = 44100; p.prepareToPlay (sr, 512);
        juce::AudioBuffer<float> b (2, 512); juce::MidiBuffer m;
        for (int st = 0; st < 6; ++st) for (float amt : { 0.2f, 0.5f, 0.9f })
        {
            p.apvts.getParameter ("genStyle")->setValueNotifyingHost (p.apvts.getParameter ("genStyle")->convertTo0to1 ((float) st));
            p.apvts.getParameter ("genAmt")->setValueNotifyingHost (amt);
            p.generate();
            const int S = (int) p.value ("bars") == 1 ? 32 : 64; int overlaps = 0;
            std::printf ("stile %d amt %.1f  %d passi\n", st, amt, S);
            for (int s2 = 0; s2 < S; ++s2) { int mv = 0; for (int f = 0; f < kNumFx; ++f) if (f != GATE && p.seq[f][s2].load()) ++mv; if (mv > 1) ++overlaps; }
            for (int f = 0; f < kNumFx; ++f) { juce::String row; bool any = false; for (int s2 = 0; s2 < S; ++s2) { row << (p.seq[f][s2].load() ? "#" : (s2 % 4 == 0 ? "|" : ".")); any |= p.seq[f][s2].load(); } if (any) std::printf ("  %-12s %s\n", kFx[f].name, row.toRawUTF8()); }
            bool nan = false; float pk = 0; double ph = 0;
            for (int k = 0; k < 400; ++k) { for (int i = 0; i < 512; ++i) { const float v = (float) std::sin (ph) * 0.4f; ph += 0.03; b.setSample (0, i, v); b.setSample (1, i, v); } p.processBlock (b, m); for (int i = 0; i < 512; ++i) { if (! std::isfinite (b.getSample (0, i))) nan = true; pk = std::max (pk, std::abs (b.getSample (0, i))); } }
            std::printf ("  sovrapposizioni %d  nan %d  picco %.2f\n", overlaps, (int) nan, pk);
        }
        return 0;
    }
    const int FX = argc > 1 ? atoi (argv[1]) : -1;
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
