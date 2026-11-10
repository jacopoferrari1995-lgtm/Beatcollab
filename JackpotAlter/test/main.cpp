#include <cstdlib>
#include "../Source/AlterProcessor.h"
#include "../Source/AlterEditor.h"
using namespace ja;

static void setP (Processor& p, const char* id, float v) { auto* rp = p.apvts.getParameter (id); rp->setValueNotifyingHost (rp->convertTo0to1 (v)); }

static std::vector<float> run (Processor& p, std::function<float (int)> gen, int len, int block = 512, std::vector<float>* right = nullptr)
{
    juce::AudioBuffer<float> b (2, block); juce::MidiBuffer mb; std::vector<float> out;
    for (int off = 0; off < len; off += block)
    {
        const int n = juce::jmin (block, len - off); b.setSize (2, n, false, false, true);
        for (int i = 0; i < n; ++i) { const float x = gen (off + i); b.setSample (0, i, x); b.setSample (1, i, x); }
        p.processBlock (b, mb);
        for (int i = 0; i < n; ++i) { out.push_back (b.getSample (0, i)); if (right) right->push_back (b.getSample (1, i)); }
    }
    return out;
}

// frequenza del picco piu forte (FFT 32768 + interpolazione)
static float peakHz (const std::vector<float>& x, int start, double sr, float minHz = 30)
{
    const int order = 15, N = 1 << order; juce::dsp::FFT f (order); std::vector<float> d ((size_t) (2 * N), 0);
    for (int i = 0; i < N && start + i < (int) x.size(); ++i) d[(size_t) i] = x[(size_t) (start + i)] * (0.5f - 0.5f * std::cos (6.2831853f * i / N));
    f.performFrequencyOnlyForwardTransform (d.data(), true);
    int best = (int) (minHz * N / sr); for (int k = best; k < N / 2 - 1; ++k) if (d[(size_t) k] > d[(size_t) best]) best = k;
    const float a = std::log (d[(size_t) best - 1] + 1e-9f), b = std::log (d[(size_t) best] + 1e-9f), c = std::log (d[(size_t) best + 1] + 1e-9f);
    const float off = 0.5f * (a - c) / (a - 2 * b + c);
    return (float) ((best + off) * sr / N);
}
static float rms (const std::vector<float>& x, int a, int b) { double e = 0; for (int i = a; i < b; ++i) e += x[(size_t) i] * x[(size_t) i]; return (float) std::sqrt (e / juce::jmax (1, b - a)); }

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    const double sr = 48000;
    auto fresh = [&] () { auto p = std::make_unique<Processor>(); p->setPlayConfigDetails (2, 2, sr, 512); p->prepareToPlay (sr, 512); return p; };

    if (argc > 1 && juce::String (argv[1]) == "snap")
    {
        auto p = fresh(); setP (*p, "pitch", -5); setP (*p, "drive", 0.45f); setP (*p, "model", VALVOLA); setP (*p, "focus", 1); setP (*p, "fLo", 300); setP (*p, "fHi", 6000);
        juce::Random r (1);
        run (*p, [&] (int i) { const double t = i / sr; return (float) (0.3 * std::sin (2 * 3.14159 * 180 * t) + 0.15 * std::sin (2 * 3.14159 * 360 * t) + 0.05 * (r.nextFloat() - 0.5f)); }, (int) sr);
        Editor ed (*p);
        if (auto* jsc = std::getenv ("JSCALE")) ed.scaler.set ((float) std::atof (jsc));
        ed.tickForTest();
        auto img = ed.createComponentSnapshot (ed.getLocalBounds(), true, 1.0f);
        juce::File f (argc > 2 ? argv[2] : "/tmp/alter.png"); f.deleteFile(); juce::FileOutputStream os (f); juce::PNGImageFormat().writeImageToStream (img, os);
        return 0;
    }

    // 1) latenza: impulso con tutto fermo e drive pulito
    {
        auto p = fresh(); setP (*p, "model", CONSOLE); setP (*p, "drive", 0); setP (*p, "auto", 0);
        auto y = run (*p, [] (int i) { return i == 1000 ? 0.5f : 0.0f; }, 20000);
        int pk = 0; for (int i = 0; i < (int) y.size(); ++i) if (std::abs (y[(size_t) i]) > std::abs (y[(size_t) pk])) pk = i;
        std::printf ("latenza dichiarata %d, misurata %d (picco %.3f)\n", p->getLatencySamples(), pk - 1000, y[(size_t) pk]);
        // mix 50%: deve sommare senza filtro a pettine
        setP (*p, "mix", 0.5f);
        auto z = run (*p, [] (int i) { return (float) (0.3 * std::sin (2 * 3.14159 * 9000.0 * i / 48000.0)); }, 24000);
        std::printf ("mix 50%% a 9 kHz: rms %.4f (atteso ~0.212)\n", rms (z, 12000, 24000));
    }
    // 2) pitch su sinusoide
    for (float st : { 7.0f, -12.0f, 3.0f, 12.0f })
    {
        auto p = fresh(); setP (*p, "pitch", st); setP (*p, "drive", 0); setP (*p, "model", CONSOLE);
        auto y = run (*p, [] (int i) { return (float) (0.3 * std::sin (2 * 3.14159265 * 220.0 * i / 48000.0)); }, 96000);
        std::printf ("pitch %+5.1f: %.2f Hz (atteso %.2f)  rms %.3f\n", st, peakHz (y, 40000, sr), 220.0 * std::pow (2.0, st / 12.0), rms (y, 40000, 90000));
    }
    // 3) formante da sola: la nota non deve cambiare
    {
        auto p = fresh(); setP (*p, "formant", 5); setP (*p, "drive", 0);
        auto y = run (*p, [] (int i) { double t = i / 48000.0, s = 0; for (int h = 1; h < 20; ++h) s += std::sin (2 * 3.14159265 * 150 * h * t) / h; return (float) (0.2 * s); }, 96000);
        std::printf ("formant +5: fondamentale %.2f Hz (atteso 150)\n", peakHz (y, 40000, sr, 100));
    }
    // 4) quantizza e robot
    {
        auto p = fresh(); setP (*p, "mode", QUANTIZZA); setP (*p, "key", 9); setP (*p, "scale", 2); setP (*p, "drive", 0);
        auto y = run (*p, [] (int i) { double t = i / 48000.0, s = 0; for (int h = 1; h < 8; ++h) s += std::sin (2 * 3.14159265 * 227 * h * t) / h; return (float) (0.2 * s); }, 96000);
        std::printf ("quantizza La min, ingresso 227 Hz: %.2f Hz (atteso 220)  rilevato %.1f\n", peakHz (y, 40000, sr, 150), p->pitch.inHz.load());
        auto q = fresh(); setP (*q, "mode", ROBOT); setP (*q, "key", 0); setP (*q, "drive", 0);
        auto z = run (*q, [] (int i) { double t = i / 48000.0, s = 0; for (int h = 1; h < 8; ++h) s += std::sin (2 * 3.14159265 * 300 * h * t) / h; return (float) (0.2 * s); }, 96000);
        std::printf ("robot Do, ingresso 300 Hz: %.2f Hz (atteso 261.63)\n", peakHz (z, 40000, sr, 150));
    }
    // 5) tutti i modelli
    for (int m = 0; m < kNumModels; ++m)
    {
        auto p = fresh(); setP (*p, "model", (float) m); setP (*p, "drive", 0.6f);
        auto y = run (*p, [] (int i) { return (float) (0.4 * std::sin (2 * 3.14159265 * 110.0 * i / 48000.0)); }, 48000);
        bool ok = true; for (auto v : y) if (! std::isfinite (v)) ok = false;
        std::printf ("%-14s rms %.3f  %s  autogain %+.1f dB\n", modelInfo (m).name, rms (y, 24000, 48000), ok ? "ok" : "NaN!", p->drive.autoDb.load());
    }
    // 6) FOCUS: sinusoide a 60 Hz fuori banda (200 Hz - 20 kHz) resta pulita
    {
        auto p = fresh(); setP (*p, "model", DISTORSIONE); setP (*p, "drive", 0.9f); setP (*p, "focus", 1); setP (*p, "fLo", 400); setP (*p, "fHi", 20000);
        auto y = run (*p, [] (int i) { return (float) (0.4 * std::sin (2 * 3.14159265 * 60.0 * i / 48000.0)); }, 48000);
        std::printf ("focus 400Hz+: 60 Hz in uscita rms %.3f (ingresso 0.283), picco a %.1f Hz\n", rms (y, 24000, 48000), peakHz (y, 10000, sr, 20));
    }
    // 7) CPU
    {
        auto p = fresh(); setP (*p, "pitch", -5); setP (*p, "formant", 2); setP (*p, "model", AMP); setP (*p, "drive", 0.5f); setP (*p, "focus", 1);
        juce::Random r (2);
        const auto t0 = juce::Time::getMillisecondCounterHiRes();
        run (*p, [&] (int) { return r.nextFloat() * 0.2f - 0.1f; }, (int) sr * 10);
        const auto ms = juce::Time::getMillisecondCounterHiRes() - t0;
        std::printf ("CPU: 10 s stereo in %.0f ms (%.1f%% di un core)\n", ms, ms / 100.0);
    }
    // 8) preset
    for (int i = 0; i < (int) presets().size(); ++i)
    {
        auto p = fresh(); p->loadPreset (i); juce::Random r (3);
        auto y = run (*p, [&] (int k) { return (float) (0.25 * std::sin (2 * 3.14159265 * 196.0 * k / 48000.0) + 0.02 * (r.nextFloat() - 0.5)); }, 48000);
        bool ok = true; float pk = 0; for (auto v : y) { if (! std::isfinite (v)) ok = false; pk = juce::jmax (pk, std::abs (v)); }
        if (! ok || pk > 2.0f || rms (y, 24000, 48000) < 0.01f) std::printf ("PRESET %s: ok=%d picco %.2f rms %.3f\n", presets()[(size_t) i].name, ok, pk, rms (y, 24000, 48000));
    }
    std::printf ("%d preset provati\n", (int) presets().size());
    return 0;
}
