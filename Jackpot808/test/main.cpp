#include "../Source/S8Processor.h"
#include "../Source/S8Editor.h"
using namespace s8;
int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gi;
    const double sr = 48000; const int B = 512;
    Processor p;
    // abilita il sidechain
    auto lay = p.getBusesLayout(); lay.inputBuses.getReference (1) = juce::AudioChannelSet::stereo(); p.setBusesLayout (lay);
    p.prepareToPlay (sr, B);
    if (argc > 1 && juce::String (argv[1]) == "preset") p.loadPreset (2);
    juce::AudioBuffer<float> buf (4, B); juce::MidiBuffer m;
    const float notes[4] = { 49.0f * 1.0116f, 49.0f * 1.0116f, 58.27f * 1.0116f, 43.65f * 1.0116f };   // G1 G1 A#1 F1, +20 cent
    double ph = 0; float pk = 0; bool nan = false;
    for (int k = 0; k < (int) (10 * sr / B); ++k)
    {
        for (int i = 0; i < B; ++i)
        {
            const double t = (k * B + i) / sr; const double beat = std::fmod (t, 0.5); const int note = (int) (t / 1.0) % 4;
            ph += 2 * 3.14159265 * notes[note] / sr;
            const float env = (float) std::exp (-beat * 2.5), x = 0.5f * env * (float) std::sin (ph) + (beat < 0.004 ? 0.3f * (float) std::sin (ph * 6) : 0.0f);
            buf.setSample (0, i, x); buf.setSample (1, i, x * 0.98f + 0.02f * (float) std::sin (ph * 1.01));
            const float kick = beat < 0.08 ? -0.8f * (float) std::exp (-beat * 40) * (float) std::sin (2 * 3.14159 * 55 * beat) : 0.0f;   // cassa in controfase
            buf.setSample (2, i, kick); buf.setSample (3, i, kick);
        }
        p.processBlock (buf, m);
        for (int i = 0; i < B; ++i) { const float x = buf.getSample (0, i); if (! std::isfinite (x)) nan = true; pk = std::max (pk, std::abs (x)); }
    }
    std::printf ("latenza %d, picco %.2f nan %d\n", p.getLatencySamples(), pk, (int) nan);
    std::printf ("nota ora %.1f Hz conf %.2f, media %.1f Hz, scarto medio %.1f cent, misure %d\n", p.uiHz.load(), p.uiConf.load(), p.f0Avg.load(), p.centsAvg.load(), p.pitchN.load());
    std::printf ("note:"); for (int k = 0; k < 12; ++k) std::printf (" %s=%d", noteName (k), (int) p.pcHist[k].load()); std::printf ("\n");
    std::printf ("cassa: colpi %d corr %.2f sovrapposizione %.1f dB; sub %.1f corpo %.1f alti %.1f; side %.1f dB; crest %.1f\n", p.kickHits.load(), p.kickCorr.load(), p.overlapDb.load(), p.eSub.load(), p.eBody.load(), p.eHigh.load(), p.sideRatio.load(), p.crest.load());
    Editor ed (p); ed.diagnose(); for (auto& d : ed.diags) std::printf ("  [%d] %s: %s\n", d.sev, d.title.toRawUTF8(), d.text.toRawUTF8());
    if (argc > 2)
    {
        for (int i = 0; i < 12; ++i) ed.spec.compute();
        ed.tuner.hzS = p.uiHz.load() > 20 ? p.uiHz.load() : 49.5f; ed.tuner.centS = 20;
        auto img = ed.createComponentSnapshot (ed.getLocalBounds(), true, 1.0f);
        juce::File f (argv[2]); f.deleteFile(); juce::FileOutputStream os (f); juce::PNGImageFormat().writeImageToStream (img, os);
    }
    return 0;
}
