#pragma once
// banco di prova "come THE_LAB" per i test dei plugin: latenza vera, buffer irregolari, aliasing, 48/96 kHz, click per manopola, CPU
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <functional>
#include <cstdio>

namespace testlab
{
using Make = std::function<std::unique_ptr<juce::AudioProcessor> (double rate, int blk)>;

inline std::vector<float> render (juce::AudioProcessor& p, int len, std::function<float (int)> gen, int blk = 512, bool uneven = false, std::function<void (int)> at = {}, std::vector<float>* right = nullptr)
{
    std::vector<float> o; juce::MidiBuffer m; juce::Random rb (3);
    for (int off = 0; off < len; )
    {
        const int n = uneven ? juce::jmin (len - off, 1 + rb.nextInt (blk)) : juce::jmin (blk, len - off);
        if (at) at (off);
        juce::AudioBuffer<float> b (2, n); for (int i = 0; i < n; ++i) { const float v = gen (off + i); b.setSample (0, i, v); b.setSample (1, i, v); }
        p.processBlock (b, m); for (int i = 0; i < n; ++i) { o.push_back (b.getSample (0, i)); if (right) right->push_back (b.getSample (1, i)); }
        off += n;
    }
    return o;
}

inline void run (const Make& mk, bool clicks = true, std::function<bool (juce::AudioProcessorParameter*)> skip = {})
{
    const double sr = 48000;
    {   // latenza
        auto p = mk (sr, 512); juce::Random rr (9); std::vector<float> in (48000 * 3); for (auto& x : in) x = 0.02f * (rr.nextFloat() * 2 - 1);
        auto o = render (*p, (int) in.size(), [&] (int k) { return in[(size_t) k]; });
        double best = -1e30; int bl = 0; std::vector<double> cc (4000);
        for (int l = 0; l < 4000; ++l) { double c = 0; for (int i = 24000; i < (int) in.size() - 4000; ++i) c += (double) in[(size_t) i] * o[(size_t) (i + l)]; cc[(size_t) l] = c; if (c > best) { best = c; bl = l; } }
        double fr = bl; if (bl > 0 && bl < 3999) { const double a = cc[(size_t) bl - 1], b2 = cc[(size_t) bl], c = cc[(size_t) bl + 1]; fr = bl + 0.5 * (a - c) / (a - 2 * b2 + c); }
        std::printf ("latenza dichiarata %d, misurata %.2f\n", p->getLatencySamples(), fr);
        {   // uguale all'ingresso spostato della latenza dichiarata? (conta solo sul preset neutro)
            const int L = p->getLatencySamples(); double e = 0, d = 0;
            for (size_t i = 24000; i < in.size(); ++i) { e += (double) in[i - (size_t) L] * in[i - (size_t) L]; d += std::pow ((double) o[i] - in[i - (size_t) L], 2.0); }
            std::printf ("differenza dall'ingresso ritardato: %.1f dB\n", 10 * std::log10 (d / (e + 1e-30) + 1e-30));
        }
        for (int blk : { 64, 512, 2048 })
        {
            auto p2 = mk (sr, blk); auto o2 = render (*p2, (int) in.size(), [&] (int k) { return in[(size_t) k]; }, blk, true);
            double e = 0, d = 0; for (size_t i = 24000; i < in.size(); ++i) { e += (double) o[i] * o[i]; d += std::pow (o[i] - o2[i], 2.0); }
            std::printf ("blocchi irregolari fino a %d vs 512: %.1f dB\n", blk, 10 * std::log10 (d / (e + 1e-30) + 1e-30));
        }
        auto p3 = mk (sr, 512); auto o3 = render (*p3, (int) in.size(), [&] (int k) { return in[(size_t) k]; });
        double e = 0, d = 0; for (size_t i = 24000; i < in.size(); ++i) { e += (double) o[i] * o[i]; d += std::pow (o[i] - o3[i], 2.0); }
        std::printf ("due copie nuove: %.1f dB\n", 10 * std::log10 (d / (e + 1e-30) + 1e-30));
    }
    for (double f0 : { 6300.0, 10000.0 })
    {
        const double ff = (double) (((int) std::round (f0 * 65536 / sr)) | 1) * sr / 65536;
        auto p = mk (sr, 512); auto o = render (*p, (int) sr * 3, [&] (int k) { return 0.5f * (float) std::sin (2 * juce::MathConstants<double>::pi * ff * k / sr); });
        const int N = 65536; juce::dsp::FFT fft (16); std::vector<float> a ((size_t) 2 * N, 0);
        for (int i = 0; i < N; ++i) a[(size_t) i] = o[o.size() - N + (size_t) i] * (0.5f - 0.5f * std::cos (6.2831853f * i / N));
        fft.performFrequencyOnlyForwardTransform (a.data());
        const int k0 = (int) std::round (ff * N / sr); float ref = 0; for (int k = k0 - 4; k <= k0 + 4; ++k) ref = std::max (ref, a[(size_t) k]);
        float worst = 0; double wf = 0;
        for (int k = 40; k < N / 2; ++k) { bool h = false; for (int hh = 1; hh < 10; ++hh) if (std::abs (k - k0 * hh) <= 6) h = true; if (std::abs (k - k0) < k0 / 20) h = true; if (! h && a[(size_t) k] > worst) { worst = a[(size_t) k]; wf = k * sr / N; } }
        std::printf ("aliasing tono %.0f Hz: %.1f dB a %.0f Hz\n", ff, juce::Decibels::gainToDecibels (worst / juce::jmax (1e-9f, ref)), wf);
    }
    for (double f : { 100.0, 1000.0, 5000.0, 10000.0 })
    {
        float gg[2];
        for (int j = 0; j < 2; ++j) { const double rate = j ? 96000.0 : 48000.0; auto p = mk (rate, 512); auto o = render (*p, (int) rate * 2, [&] (int k) { return 0.05f * (float) std::sin (2 * juce::MathConstants<double>::pi * f * k / rate); });
            double e = 0; for (size_t i = o.size() - (size_t) rate; i < o.size(); ++i) e += (double) o[i] * o[i]; gg[j] = juce::Decibels::gainToDecibels ((float) std::sqrt (e / rate) / (0.05f / std::sqrt (2.0f))); }
        std::printf ("%6.0f Hz: 48k %+.2f  96k %+.2f\n", f, gg[0], gg[1]);
    }
    if (clicks)
    {   // ogni manopola continua salta del 30% 4 volte su un tono a 1 kHz; energia non armonica 5-20 kHz contro la versione ferma
        auto probe = mk (sr, 512); const int np = probe->getParameters().size(); int nC = 0;
        juce::dsp::FFT fft (11);
        auto nonHarm = [&] (const std::vector<float>& x, size_t at) { std::vector<float> w (4096, 0.0f); for (int i = 0; i < 2048; ++i) w[(size_t) i] = x[at + (size_t) i] * (0.5f - 0.5f * std::cos (6.2831853f * i / 2047));
            fft.performFrequencyOnlyForwardTransform (w.data()); double s2 = 0; for (int k = 1; k < 1024; ++k) { const double fq = k * sr / 2048; if (fq < 5000 || fq > 20000) continue; const double hn = fq / 999.76; if (std::abs (hn - std::round (hn)) * 999.76 < 120) continue; s2 += (double) w[(size_t) k] * w[(size_t) k]; } return s2; };
        for (int pi = 0; pi < np; ++pi)
        {
            auto* prm = probe->getParameters()[pi];
            if (skip && skip (prm)) continue;
            auto rend = [&] (int mode)
            {
                auto p = mk (sr, 512); auto* r = p->getParameters()[pi]; const float v0 = r->getValue();
                const int ns = r->getNumSteps();
                float v1 = v0 > 0.5f ? v0 - 0.3f : v0 + 0.3f;
                if (ns > 1 && ns < 100) { const float st = 1.0f / (float) (ns - 1); v1 = v0 + st <= 1.0001f ? v0 + st : v0 - st; }
                if (mode == 1) r->setValueNotifyingHost (v1);
                return render (*p, 512 * 200, [&] (int k) { return 0.5f * (float) std::sin (2 * juce::MathConstants<double>::pi * 999.76 * k / sr); }, 512, false,
                               [&] (int off) { const int blk = off / 512; if (mode == 2 && blk >= 100 && (blk - 100) % 20 == 0 && blk < 180) r->setValueNotifyingHost (((blk - 100) / 20) % 2 ? v0 : v1); });
            };
            const auto a = rend (0), b1 = rend (1), bb = rend (2);
            double ch = 0; for (size_t i = 0; i < a.size(); ++i) ch = std::max (ch, (double) std::abs (bb[i] - a[i])); if (ch < 1e-6) continue;
            double e = 0; for (int j = 0; j < 4; ++j) { const size_t at = (size_t) (100 + j * 20) * 512 - 1024; e = std::max (e, nonHarm (bb, at) - nonHarm (j % 2 == 0 ? b1 : a, at)); }
            const double db = 20 * std::log10 (std::sqrt (std::max (0.0, e)) / 1024.0 * 2.0 / 0.5 + 1e-12);
            if (db > -45) { std::printf ("  %-28s %.1f dB%s\n", prm->getName (30).toRawUTF8(), db, db > -35 ? " CLICK" : ""); if (db > -35) ++nC; }
        }
        std::printf ("click: %d\n", nC);
    }
    {   // CPU
        auto p = mk (sr, 512); juce::Random rr (4); const auto t0 = juce::Time::getMillisecondCounterHiRes();
        render (*p, (int) sr * 10, [&] (int) { return 0.2f * (rr.nextFloat() * 2 - 1); });
        std::printf ("CPU: %.1f%% di un core\n", (juce::Time::getMillisecondCounterHiRes() - t0) / 100.0);
    }
    {   // recall: stato salvato e ricaricato in una copia nuova
        auto p = mk (sr, 512); juce::MemoryBlock mb; p->getStateInformation (mb);
        auto q = mk (sr, 512); q->setStateInformation (mb.getData(), (int) mb.getSize());
        juce::MemoryBlock mb2; q->getStateInformation (mb2);
        std::printf ("recall: %s\n", mb == mb2 ? "identico" : "DIVERSO");
    }
}
}
