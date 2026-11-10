#include <cstdlib>
#include <juce_dsp/juce_dsp.h>
#include "../Source/DustProcessor.h"
#include "../Source/DustEditor.h"
using namespace jds;

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
static float rms (const std::vector<float>& x, int a, int b) { double e = 0; for (int i = a; i < b; ++i) e += x[(size_t) i] * x[(size_t) i]; return (float) std::sqrt (e / juce::jmax (1, b - a)); }
static float bandDb (const std::vector<float>& x, int start, double sr, float f0, float f1)
{
    const int order = 14, N = 1 << order; juce::dsp::FFT f (order); std::vector<float> d ((size_t) (2 * N), 0);
    for (int i = 0; i < N; ++i) d[(size_t) i] = x[(size_t) (start + i)] * (0.5f - 0.5f * std::cos (6.2831853f * i / N));
    f.performFrequencyOnlyForwardTransform (d.data(), true);
    double e = 0; for (int k = (int) (f0 * N / sr); k <= (int) (f1 * N / sr); ++k) e += d[(size_t) k] * d[(size_t) k];
    return 10 * (float) std::log10 (e + 1e-20);
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    const double sr = 48000;
    auto fresh = [&] { auto p = std::make_unique<Processor>(); p->setPlayConfigDetails (2, 2, sr, 512); p->prepareToPlay (sr, 512); return p; };
    // batteria finta: cassa ogni 0.5 s, rullante in levare, hat in ottavi
    juce::Random rnd (5);
    auto drums = [&] (int i, float& l, float& r)
    {
        const double t = i / sr, b = std::fmod (t, 0.5), h = std::fmod (t, 0.125), s2 = std::fmod (t + 0.25, 0.5);
        float v = (float) (0.8 * std::sin (2 * 3.14159 * (50 + 120 * std::exp (-b * 30)) * b) * std::exp (-b * 8));
        if (std::fmod (t, 1.0) >= 0.5 && s2 < 0.2) v += (float) ((rnd.nextFloat() - 0.5) * 0.7 * std::exp (-s2 * 25) + 0.3 * std::sin (2 * 3.14159 * 190 * s2) * std::exp (-s2 * 30));
        v += (float) ((rnd.nextFloat() - 0.5) * 0.25 * std::exp (-h * 90));
        l = v; r = v * 0.95f;
    };
    if (argc > 2 && juce::String (argv[1]) == "snap")
    {
        auto p = fresh(); if (auto* pr = std::getenv ("JPRESET")) p->loadPreset (std::atoi (pr));
        std::vector<float> L, R; run (*p, drums, (int) sr * 2, L, R);
        Editor ed (*p); if (auto* jsc = std::getenv ("JSCALE")) ed.scaler.set ((float) std::atof (jsc));
        ed.tickForTest();
        auto img = ed.createComponentSnapshot (ed.getLocalBounds(), true, 1.0f);
        juce::File f (argv[2]); f.deleteFile(); juce::FileOutputStream os (f); juce::PNGImageFormat().writeImageToStream (img, os);
        return 0;
    }
    {   // appena caricato: neutro (a parte la latenza)
        auto p = fresh(); const int lat = p->getLatencySamples(); std::vector<float> L, R, in;
        run (*p, [&] (int i, float& l, float& r) { drums (i, l, r); in.push_back (l); }, 48000, L, R);
        double e = 0; for (size_t i = (size_t) lat; i < L.size(); ++i) e += std::pow (L[i] - in[i - (size_t) lat], 2.0);
        std::printf ("neutro: latenza %d, differenza %.1f dB\n", lat, 10 * std::log10 (e / L.size() + 1e-30));
    }
    {   // SP-12 senza anti-alias: un 15 kHz a 26 kHz ritorna a 11 kHz; S950 lo filtra
        for (int m : { S_SP12, S_S950 })
        {
            auto p = fresh(); setP (*p, "s_on", 1); setP (*p, "s_model", (float) m); setP (*p, "s_bits", 12); setP (*p, "s_rate", m == S_SP12 ? 26040.0f : 32000.0f);
            std::vector<float> L, R; run (*p, [&] (int i, float& l, float& r) { l = r = 0.5f * (float) std::sin (2 * 3.14159 * 15000 * i / sr); }, 48000, L, R);
            const float rate = m == S_SP12 ? 26040.0f : 32000.0f, alias = rate - 15000;
            std::printf ("%s: 15 kHz -> immagine a %.0f Hz: %.1f dB rispetto al tono\n", sInfo (m).name, alias, bandDb (L, 20000, sr, alias - 100, alias + 100) - bandDb (L, 20000, sr, 14900, 15100));
        }
    }
    {   // saturazione: volume compensato
        for (int m = 0; m < kDModels; ++m)
        {
            auto p = fresh(); setP (*p, "d_on", 1); setP (*p, "d_model", (float) m); setP (*p, "d_drive", 18);
            std::vector<float> L, R, in; run (*p, [&] (int i, float& l, float& r) { drums (i, l, r); in.push_back (l); }, 96000, L, R);
            std::printf ("saturazione %s +18 dB: volume %+.1f dB\n", dName (m), juce::Decibels::gainToDecibels (rms (L, 48000, 96000) / rms (in, 48000, 96000)));
        }
    }
    {   // compressore
        for (int st = 0; st < kCStyles; ++st)
        {
            auto p = fresh(); setP (*p, "c_on", 1); setP (*p, "c_style", (float) st); setP (*p, "c_amt", 0.6f);
            std::vector<float> L, R, in; run (*p, [&] (int i, float& l, float& r) { drums (i, l, r); in.push_back (l); }, 96000, L, R);
            float pi = 0, po = 0; for (int i = 48000; i < 96000; ++i) { pi = std::max (pi, std::abs (in[(size_t) i])); po = std::max (po, std::abs (L[(size_t) i])); }
            std::printf ("comp %s: picchi %+.1f dB, rms %+.1f dB (crest ridotto di %.1f dB)\n", cName (st), juce::Decibels::gainToDecibels (po / pi), juce::Decibels::gainToDecibels (rms (L, 48000, 96000) / rms (in, 48000, 96000)),
                         juce::Decibels::gainToDecibels ((pi / rms (in, 48000, 96000)) / (po / rms (L, 48000, 96000))));
        }
        auto p = fresh(); setP (*p, "c_on", 1); setP (*p, "c_amt", 0); setP (*p, "c_punch", 1);
        std::vector<float> L, R, in; run (*p, [&] (int i, float& l, float& r) { drums (i, l, r); in.push_back (l); }, 96000, L, R);
        float pi = 0, po = 0; for (int i = 48000; i < 96000; ++i) { pi = std::max (pi, std::abs (in[(size_t) i])); po = std::max (po, std::abs (L[(size_t) i])); }
        std::printf ("punch +100: picchi %+.1f dB, rms %+.1f dB\n", juce::Decibels::gainToDecibels (po / pi), juce::Decibels::gainToDecibels (rms (L, 48000, 96000) / rms (in, 48000, 96000)));
    }
    {   // polvere: niente rumore nel silenzio se segue la batteria
        auto p = fresh(); setP (*p, "n_on", 1); setP (*p, "n_vinyl", 0.6f); setP (*p, "n_hiss", 0.5f);
        std::vector<float> L, R; run (*p, [&] (int i, float& l, float& r) { if (i < 48000) drums (i, l, r); else l = r = 0; }, 48000 * 3, L, R);
        std::printf ("polvere: con batteria %.1f dBFS, nel silenzio dopo 2 s %.1f dBFS\n", juce::Decibels::gainToDecibels (rms (L, 20000, 46000)), juce::Decibels::gainToDecibels (rms (L, 130000, 144000) + 1e-9f));
    }
    {   // mix al 50% senza filtro a pettine: con tutti i moduli spenti il segnale torna uguale
        auto p = fresh(); setP (*p, "mix", 0.5f); setP (*p, "d_on", 1); setP (*p, "d_drive", 0.0f); const int lat = p->getLatencySamples();
        std::vector<float> L, R, in; run (*p, [&] (int i, float& l, float& r) { drums (i, l, r); in.push_back (l); }, 48000, L, R);
        double e = 0; for (size_t i = (size_t) lat + 1000; i < L.size(); ++i) e += std::pow (L[i] - in[i - (size_t) lat], 2.0);
        std::printf ("mix 50%% allineato: differenza %.1f dB\n", 10 * std::log10 (e / L.size() + 1e-30));
    }
    for (int i = 0; i < (int) presets().size(); ++i)
    {
        auto p = fresh(); p->loadPreset (i); std::vector<float> L, R, in;
        const auto t0 = juce::Time::getMillisecondCounterHiRes();
        run (*p, [&] (int k, float& l, float& r) { drums (k, l, r); in.push_back (l); }, (int) sr * 4, L, R);
        const double ms = juce::Time::getMillisecondCounterHiRes() - t0;
        bool nan = false; float pk = 0; for (size_t k = 0; k < L.size(); ++k) { if (! std::isfinite (L[k]) || ! std::isfinite (R[k])) nan = true; pk = std::max (pk, std::abs (L[k])); }
        std::printf ("%-34s picco %.2f  volume %+.1f dB  nan %d  CPU %.2f%%\n", presets()[(size_t) i].name, pk, juce::Decibels::gainToDecibels (rms (L, 48000, (int) L.size()) / rms (in, 48000, (int) in.size())), (int) nan, ms / 40.0);
    }
    return 0;
}
