#include <cstdlib>
#include <juce_dsp/juce_dsp.h>
#include "../Source/MotionProcessor.h"
#include "../Source/MotionEditor.h"
using namespace jmo;

static void setP (Processor& p, const char* id, float v) { auto* rp = p.apvts.getParameter (id); rp->setValueNotifyingHost (rp->convertTo0to1 (v)); }
static void run (Processor& p, std::function<void (int, float&, float&)> gen, int len, std::vector<float>& L, std::vector<float>& R, int block = 512)
{
    juce::AudioBuffer<float> b (2, block); juce::MidiBuffer mb;
    for (int off = 0; off < len; off += block)
    {
        const int n = juce::jmin (block, len - off); b.setSize (2, n, false, false, true);
        for (int i = 0; i < n; ++i) { float l, r; gen (off + i, l, r); b.setSample (0, i, l); b.setSample (1, i, r); }
        p.processBlock (b, mb);
        for (int i = 0; i < n; ++i) { L.push_back (b.getSample (0, i)); R.push_back (b.getSample (1, i)); }
    }
}
static float peakHz (const std::vector<float>& x, int start, double sr)
{
    const int order = 15, N = 1 << order; juce::dsp::FFT f (order); std::vector<float> d ((size_t) (2 * N), 0);
    for (int i = 0; i < N && start + i < (int) x.size(); ++i) d[(size_t) i] = x[(size_t) (start + i)] * (0.5f - 0.5f * std::cos (6.2831853f * i / N));
    f.performFrequencyOnlyForwardTransform (d.data(), true);
    int best = 10; for (int k = best; k < N / 2 - 1; ++k) if (d[(size_t) k] > d[(size_t) best]) best = k;
    const float a = std::log (d[(size_t) best - 1] + 1e-9f), b = std::log (d[(size_t) best] + 1e-9f), c = std::log (d[(size_t) best + 1] + 1e-9f);
    return (float) ((best + 0.5f * (a - c) / (a - 2 * b + c)) * sr / N);
}
static float rms (const std::vector<float>& x, int a, int b) { double e = 0; for (int i = a; i < b; ++i) e += x[(size_t) i] * x[(size_t) i]; return (float) std::sqrt (e / juce::jmax (1, b - a)); }

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    const double sr = 48000;
    auto fresh = [&] { auto p = std::make_unique<Processor>(); p->setPlayConfigDetails (2, 2, sr, 512); p->prepareToPlay (sr, 512); return p; };
    juce::Random rnd (3);
    auto music = [&] (int i, float& l, float& r) { const double t = i / sr; l = (float) (0.3 * std::sin (6.2831853 * 220 * t) + 0.2 * std::sin (6.2831853 * 330 * t) + 0.05 * (rnd.nextFloat() - 0.5f)); r = l * 0.9f + 0.05f * (float) std::sin (6.2831853 * 554 * t); };
    if (argc > 2 && juce::String (argv[1]) == "snap")
    {
        auto p = fresh(); if (auto* pr = std::getenv ("JPRESET")) p->loadPreset (std::atoi (pr));
        std::vector<float> L, R; run (*p, music, (int) sr * 2, L, R);
        Editor ed (*p); if (auto* jsc = std::getenv ("JSCALE")) ed.scaler.set ((float) std::atof (jsc));
        ed.tickForTest();
        auto img = ed.createComponentSnapshot (ed.getLocalBounds(), true, 1.0f);
        juce::File f (argv[2]); f.deleteFile(); juce::FileOutputStream os (f); juce::PNGImageFormat().writeImageToStream (img, os);
        return 0;
    }
    {   // appena caricato: neutro
        auto p = fresh(); std::vector<float> L, R, inL; run (*p, [&] (int i, float& l, float& r) { music (i, l, r); inL.push_back (l); }, 48000, L, R);
        double e = 0; for (size_t i = 0; i < L.size(); ++i) e += std::pow (L[i] - inL[i], 2.0);
        std::printf ("neutro: differenza %.1f dB\n", 10 * std::log10 (e / L.size() + 1e-30));
    }
    // ogni effetto a mix pieno: niente NaN, livello sensato, tempo di CPU
    for (int fx = 0; fx < kNumFx; ++fx)
    {
        auto p = fresh(); setP (*p, "fx", (float) fx); setP (*p, "mix", 1.0f); setP (*p, "depth", 0.8f); setP (*p, "fb", 0.5f); setP (*p, "pan", 1);
        std::vector<float> L, R; const auto t0 = juce::Time::getMillisecondCounterHiRes();
        run (*p, music, (int) sr * 10, L, R);
        const double ms = juce::Time::getMillisecondCounterHiRes() - t0;
        bool nan = false; float pk = 0; for (size_t i = 0; i < L.size(); ++i) { if (! std::isfinite (L[i]) || ! std::isfinite (R[i])) nan = true; pk = std::max (pk, std::max (std::abs (L[i]), std::abs (R[i]))); }
        std::printf ("%-10s picco %.2f  rms %.3f/%.3f  nan %d  CPU %.2f%%\n", fxInfo (fx).name, pk, rms (L, 48000, (int) L.size()), rms (R, 48000, (int) R.size()), (int) nan, ms / 100.0);
    }
    {   // trivox: la voce sinistra scende, la destra sale (sinusoide pura, solo effetto)
        auto p = fresh(); setP (*p, "fx", TRIVOX); setP (*p, "mix", 1.0f); setP (*p, "depth", 0.5f); setP (*p, "char", 0.0f);
        std::vector<float> L, R; run (*p, [&] (int i, float& l, float& r) { l = r = 0.5f * (float) std::sin (6.2831853 * 440 * i / sr); }, (int) sr * 3, L, R);
        std::printf ("trivox 15 cent: sinistra %.2f Hz  destra %.2f Hz  (attesi ~436.2 e ~443.8, con il centro dentro)\n", peakHz (L, 48000, sr), peakHz (R, 48000, sr));
    }
    {   // tremolo a tempo: profondita
        auto p = fresh(); setP (*p, "fx", TREMOLO); setP (*p, "mix", 1.0f); setP (*p, "depth", 1.0f); setP (*p, "rate", 4.0f);
        std::vector<float> L, R; run (*p, [&] (int i, float& l, float& r) { l = r = 0.5f * (float) std::sin (6.2831853 * 1000 * i / sr); }, (int) sr * 2, L, R);
        float mn = 9, mx = 0; for (int b = 0; b < 80; ++b) { const float v = rms (L, 24000 + b * 240, 24000 + b * 240 + 240); mn = std::min (mn, v); mx = std::max (mx, v); }
        std::printf ("tremolo: rms min %.3f max %.3f\n", mn, mx);
    }
    {   // bassi mono: lato sotto 120 Hz sparisce
        auto p = fresh(); setP (*p, "fx", CHORUS); setP (*p, "mix", 1.0f); setP (*p, "bassmono", 150);
        std::vector<float> L, R; run (*p, [&] (int i, float& l, float& r) { l = 0.5f * (float) std::sin (6.2831853 * 60 * i / sr); r = -l; }, (int) sr * 2, L, R);
        std::vector<float> S; for (size_t i = 0; i < L.size(); ++i) S.push_back (0.5f * (L[i] - R[i]));
        std::printf ("bassi mono: lato a 60 Hz %.1f dB (era 0)\n", juce::Decibels::gainToDecibels (rms (S, 48000, (int) S.size()) / 0.3536f));
    }
    for (int i = 0; i < (int) presets().size(); ++i)
    {
        auto p = fresh(); p->loadPreset (i); std::vector<float> L, R; run (*p, music, (int) sr * 3, L, R);
        bool nan = false; float pk = 0; for (size_t k = 0; k < L.size(); ++k) { if (! std::isfinite (L[k])) nan = true; pk = std::max (pk, std::abs (L[k])); }
        std::printf ("preset %-32s picco %.2f nan %d\n", presets()[(size_t) i].name, pk, (int) nan);
    }
    return 0;
}
