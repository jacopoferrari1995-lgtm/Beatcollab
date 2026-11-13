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
    if (auto* t = std::getenv ("JTHEME")) skin::cur() = std::atoi (t);
    juce::ScopedJuceInitialiser_GUI gui;
    const double sr = 48000;
    auto fresh = [&] { auto p = std::make_unique<Processor>(); p->setPlayConfigDetails (2, 2, sr, 512); p->prepareToPlay (sr, 512); return p; };
    juce::Random rnd (3);
    auto music = [&] (int i, float& l, float& r) { const double t = i / sr; l = (float) (0.3 * std::sin (6.2831853 * 220 * t) + 0.2 * std::sin (6.2831853 * 330 * t) + 0.05 * (rnd.nextFloat() - 0.5f)); r = l * 0.9f + 0.05f * (float) std::sin (6.2831853 * 554 * t); };
    if (argc > 1 && juce::String (argv[1]) == "voci")
    {   // nota "vocale" (fondamentale 196 Hz con armoniche): le copie non devono pulsare (pettine nei cambi di presa)
        auto voice = [&] (int i, float& l, float& r) { const double t = i / sr; double s = 0; for (int h = 1; h < 14; ++h) s += std::sin (6.2831853 * 196 * h * t + h * 0.7) / (h * 1.3); l = r = (float) (0.15 * s); };
        for (int f : { DOUBLER, TRIVOX, ENSEMBLE, OMNIVOX })
        {
            auto p = fresh(); const juce::String id (fxInfo (f).id);
            setP (*p, (id + "_on").toRawUTF8(), 1); setP (*p, (id + "_mix").toRawUTF8(), argc > 2 ? 0.5f : 1.0f); setP (*p, (id + "_dry").toRawUTF8(), argc > 2 ? 0.0f : -30.0f);
            std::vector<float> L, R; run (*p, voice, (int) sr * 6, L, R);
            float mn = 9, mx = 0; for (int k = (int) sr; k + 960 < (int) L.size(); k += 960) { const float v = rms (L, k, k + 960) + rms (R, k, k + 960); mn = juce::jmin (mn, v); mx = juce::jmax (mx, v); }
            std::vector<float> L2, R2; auto p2 = fresh(); setP (*p2, (id + "_on").toRawUTF8(), 1); setP (*p2, (id + "_mix").toRawUTF8(), 0.5f); run (*p2, voice, (int) sr * 2, L2, R2);
            std::printf ("%-9s pulsazione copie %.1f dB  livello copie %.3f  | mix 50%% voce+copie %.3f (dry solo 0.%03d)\n", fxInfo (f).name, juce::Decibels::gainToDecibels (mx / juce::jmax (1e-6f, mn)), mx * 0.5f, rms (L2, (int) sr, (int) sr * 2), 0);
        }
        return 0;
    }
    if (argc > 2 && juce::String (argv[1]) == "snap")
    {
        auto p = fresh(); if (auto* pr = std::getenv ("JPRESET")) p->loadPreset (std::atoi (pr));
        if (auto* js = std::getenv ("JSEL")) p->uiSel = std::atoi (js);
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
    // ogni effetto acceso a mix pieno: niente NaN, livello sensato, tempo di CPU
    for (int fx = 0; fx < kNumFx; ++fx)
    {
        auto p = fresh(); setP (*p, pid (fx, "on").toRawUTF8(), 1); setP (*p, pid (fx, "mix").toRawUTF8(), 1.0f); setP (*p, pid (fx, "depth").toRawUTF8(), 0.8f); setP (*p, pid (fx, "fb").toRawUTF8(), 0.5f); setP (*p, "pan", 1);
        std::vector<float> L, R; const auto t0 = juce::Time::getMillisecondCounterHiRes();
        run (*p, music, (int) sr * 10, L, R);
        const double ms = juce::Time::getMillisecondCounterHiRes() - t0;
        bool nan = false; float pk = 0; for (size_t i = 0; i < L.size(); ++i) { if (! std::isfinite (L[i]) || ! std::isfinite (R[i])) nan = true; pk = std::max (pk, std::max (std::abs (L[i]), std::abs (R[i]))); }
        std::printf ("%-10s picco %.2f  rms %.3f/%.3f  nan %d  CPU %.2f%%\n", fxInfo (fx).name, pk, rms (L, 48000, (int) L.size()), rms (R, 48000, (int) R.size()), (int) nan, ms / 100.0);
    }
    {   // tutti accesi insieme
        auto p = fresh(); for (int fx = 0; fx < kNumFx; ++fx) setP (*p, pid (fx, "on").toRawUTF8(), 1);
        std::vector<float> L, R; const auto t0 = juce::Time::getMillisecondCounterHiRes(); run (*p, music, (int) sr * 10, L, R);
        bool nan = false; float pk = 0; for (size_t i = 0; i < L.size(); ++i) { if (! std::isfinite (L[i])) nan = true; pk = std::max (pk, std::abs (L[i])); }
        std::printf ("TUTTI      picco %.2f nan %d CPU %.2f%%\n", pk, (int) nan, (juce::Time::getMillisecondCounterHiRes() - t0) / 100.0);
    }
    {   // trivox
        auto p = fresh(); setP (*p, "triv_on", 1); setP (*p, "triv_mix", 1.0f); setP (*p, "triv_depth", 0.5f); setP (*p, "triv_char", 0.0f);
        std::vector<float> L, R; run (*p, [&] (int i, float& l, float& r) { l = r = 0.5f * (float) std::sin (6.2831853 * 440 * i / sr); }, (int) sr * 3, L, R);
        std::printf ("trivox 15 cent: sinistra %.2f Hz  destra %.2f Hz\n", peakHz (L, 48000, sr), peakHz (R, 48000, sr));
    }
    {   // omnivox: La in La minore, voce 1 = terza sopra (Do, 523.25), solo voce
        auto p = fresh(); setP (*p, "omni_on", 1); setP (*p, "omni_key", 9); setP (*p, "omni_scale", 1); setP (*p, "omni_v1", 2); setP (*p, "omni_v2", 0); setP (*p, "omni_v4", 0); setP (*p, "omni_l1", 0); setP (*p, "omni_mix", 1.0f);
        std::vector<float> L, R, D; run (*p, [&] (int i, float& l, float& r) { double t = i / sr; l = r = 0.3f * (float) (std::sin (6.2831853 * 440 * t) + 0.5 * std::sin (6.2831853 * 880 * t)); D.push_back (l); }, (int) sr * 3, L, R);
        std::vector<float> V; for (size_t i = 0; i < L.size(); ++i) V.push_back (L[i] - D[i]);   // solo le voci (a sinistra)
        std::printf ("omnivox: nota %.3f  voce sinistra %.2f / %.2f Hz (atteso 523.25)\n", p->eng.units[OMNIVOX].noteNow.load(), peakHz (V, 72000, sr), peakHz (V, 40000, sr));
    }
    {   // freq shift 100 Hz su un 440
        auto p = fresh(); setP (*p, "fshf_on", 1); setP (*p, "fshf_mix", 1.0f); setP (*p, "fshf_freq", 100);
        std::vector<float> L, R; run (*p, [&] (int i, float& l, float& r) { l = r = 0.5f * (float) std::sin (6.2831853 * 440 * i / sr); }, (int) sr * 2, L, R);
        std::printf ("freq shift: %.2f Hz (atteso 540)\n", peakHz (L, 48000, sr));
    }
    {   // indipendenza: cambiare depth del chorus non tocca il phaser
        auto p = fresh(); const float before = p->value ("phas_depth"); setP (*p, "chor_depth", 0.1f);
        std::printf ("indipendenza: phaser depth %.2f -> %.2f\n", before, p->value ("phas_depth"));
    }
    {   // tremolo
        auto p = fresh(); setP (*p, "trem_on", 1); setP (*p, "trem_mix", 1.0f); setP (*p, "trem_depth", 1.0f); setP (*p, "trem_rate", 4.0f);
        std::vector<float> L, R; run (*p, [&] (int i, float& l, float& r) { l = r = 0.5f * (float) std::sin (6.2831853 * 1000 * i / sr); }, (int) sr * 2, L, R);
        float mn = 9, mx = 0; for (int b = 0; b < 80; ++b) { const float v = rms (L, 24000 + b * 240, 24000 + b * 240 + 240); mn = std::min (mn, v); mx = std::max (mx, v); }
        std::printf ("tremolo: rms min %.3f max %.3f\n", mn, mx);
    }
    {   // bassi mono
        auto p = fresh(); setP (*p, "chor_on", 1); setP (*p, "chor_mix", 1.0f); setP (*p, "bassmono", 150);
        std::vector<float> L, R; run (*p, [&] (int i, float& l, float& r) { l = 0.5f * (float) std::sin (6.2831853 * 60 * i / sr); r = -l; }, (int) sr * 2, L, R);
        std::vector<float> S; for (size_t i = 0; i < L.size(); ++i) S.push_back (0.5f * (L[i] - R[i]));
        std::printf ("bassi mono: lato a 60 Hz %.1f dB\n", juce::Decibels::gainToDecibels (rms (S, 48000, (int) S.size()) / 0.3536f));
    }
    for (int i = 0; i < (int) presets().size(); ++i)
    {
        auto p = fresh(); p->loadPreset (i); std::vector<float> L, R; run (*p, music, (int) sr * 3, L, R);
        bool nan = false; float pk = 0; for (size_t k = 0; k < L.size(); ++k) { if (! std::isfinite (L[k])) nan = true; pk = std::max (pk, std::abs (L[k])); }
        std::printf ("preset %-32s picco %.2f nan %d\n", presets()[(size_t) i].name, pk, (int) nan);
    }
    return 0;
}
