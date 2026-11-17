#include "../Source/SpaceProcessor.h"
#include "../Source/SpaceEditor.h"
using namespace jsp;
int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gi;
    const double sr = 48000;
    auto run = [&] (Processor& p, int blocks, std::vector<float>& out, bool impulse = false)
    {
        juce::AudioBuffer<float> b (2, 512); juce::Random r (3);
        for (int k = 0; k < blocks; ++k)
        {
            for (int i = 0; i < 512; ++i) { const int n = k * 512 + i; const double t = n / sr;
                float v = impulse ? (n == 4096 ? 1.0f : 0.0f) : (float) ((r.nextFloat() * 2 - 1) * std::exp (-std::fmod (t * 2.0, 1.0) * 25) * 0.5 + std::sin (2 * 3.14159 * 220 * t) * 0.1 * (std::fmod (t, 2.0) < 1));
                b.setSample (0, i, v); b.setSample (1, i, v); }
            juce::MidiBuffer m; p.processBlock (b, m);
            for (int i = 0; i < 512; ++i) { out.push_back (b.getSample (0, i)); out.push_back (b.getSample (1, i)); }
        }
    };
    if (argc > 1 && juce::String (argv[1]) == "dsp")
    {
        // risposta all'impulso di ogni motore da solo, 100% bagnato: RT60 misurato, picco, correlazione L/R, NaN
        std::vector<std::pair<int, float>> cases; for (int t = 1; t < kNumTypes; ++t) cases.push_back ({ t, -1.0f });
        if (std::getenv ("JRT")) { cases.clear(); for (int t : { (int) PLATE, (int) HALL, (int) ROOM, (int) SPRING, (int) CHAMBER, (int) STUDIO, (int) MORPH, (int) NEOVERB, (int) SHIMMER, (int) SPACED, (int) CLOUD }) for (float v : { 0.15f, 0.45f, 0.75f }) cases.push_back ({ t, v }); }
        for (auto [t, dv] : cases)
        {
            Processor p; p.prepareToPlay (sr, 512);
            for (int k = 0; k < kSlots; ++k) p.setSlot (k, k == 0 ? t : OFF);
            p.P (mixId (0))->setValueNotifyingHost (1.0f); p.P ("dry")->setValueNotifyingHost (0.0f);
            if (isDelay (t)) p.setP (0, 2, 0.0f);   // libero
            if (dv >= 0) p.setP (0, 0, dv);
            if (t == CONV) { p.updateIRs(); juce::Thread::sleep (600); }
            std::vector<float> o; run (p, dv > 0.7f ? 2400 : 600, o, true);
            const size_t n = o.size() / 2; std::vector<double> e (n); double tot = 0, pk = 0, lr = 0, ll = 0, rr = 0; bool nan = false;
            for (size_t i = 4100; i < n; ++i) { const double l = o[2 * i], r = o[2 * i + 1]; if (! std::isfinite (l) || ! std::isfinite (r)) nan = true; e[i] = l * l + r * r; tot += e[i]; pk = std::max (pk, std::max (std::abs (l), std::abs (r))); lr += l * r; ll += l * l; rr += r * r; }
            // integrazione di Schroeder: tempo per -5 -> -35 dB, estrapolato a 60
            double acc = tot, t5 = -1, t35 = -1; for (size_t i = 4100; i < n; ++i) { const double db = 10 * std::log10 (acc / tot + 1e-30); if (t5 < 0 && db < -5) t5 = i / sr; if (t35 < 0 && db < -35) { t35 = i / sr; break; } acc -= e[i]; }
            // RT a 1 kHz (banda d'ottava, come si misura nelle sale vere)
            double rt1k = -1;
            {
                std::vector<double> eb (n, 0.0); double tb = 0; float z[2][4] {};
                const float a1 = 1.0f - std::exp (-6.2832f * 1400.0f / 48000.0f), a2 = 1.0f - std::exp (-6.2832f * 700.0f / 48000.0f);
                for (size_t i = 4100; i < n; ++i)
                {
                    double ee = 0;
                    for (int c = 0; c < 2; ++c) { float x = o[2 * i + c]; z[c][0] += a1 * (x - z[c][0]); z[c][1] += a1 * (z[c][0] - z[c][1]); x = z[c][1];
                        z[c][2] += a2 * (x - z[c][2]); x -= z[c][2]; z[c][3] += a2 * (x - z[c][3]); x -= z[c][3]; ee += (double) x * x; }
                    eb[i] = ee; tb += ee;
                }
                double acc = tb, b5 = -1, b35 = -1;
                for (size_t i = 4100; i < n; ++i) { const double db = 10 * std::log10 (acc / tb + 1e-30); if (b5 < 0 && db < -5) b5 = i / sr; if (b35 < 0 && db < -35) { b35 = i / sr; break; } acc -= eb[i]; }
                if (b35 > 0) rt1k = (b35 - b5) * 2.0;
            }
            float pdec[kP]; for (int i = 0; i < kP; ++i) pdec[i] = p.value (pId (0, i));
            std::printf ("%-10s decay impostato %-8s RT60 %.2f s  a 1 kHz %.2f s  energia %.1f dB  picco %.3f  corr L/R %.2f  nan %d\n", info (t).name, paramText (t, 0, pdec[0], pdec).toRawUTF8(),
                         t35 > 0 ? (t35 - t5) * 2.0 : -1.0, rt1k, 10 * std::log10 (tot), pk, lr / std::sqrt (ll * rr + 1e-30), (int) nan);
        }
        if (std::getenv ("JDLY"))
        {   // delay: dove cade la prima eco (rispetto al tempo scritto) e quanto scende da un'eco alla seguente (rispetto al FEEDBACK)
            for (int t : { (int) TAPE, (int) DIGITAL, (int) BBD, (int) MULTITAP, (int) REVERSE })
                for (float tv : { 0.3f, 0.6f }) for (float fb : { 0.3f, 0.6f })
                {
                    Processor p; p.prepareToPlay (sr, 512);
                    for (int k = 0; k < kSlots; ++k) p.setSlot (k, k == 0 ? t : OFF);
                    p.P (mixId (0))->setValueNotifyingHost (1.0f); p.P ("dry")->setValueNotifyingHost (0.0f);
                    p.setP (0, 2, 0.0f); p.setP (0, 0, tv); p.setP (0, 1, fb);
                    if (t == TAPE) { p.setP (0, 3, 0.0f); p.setP (0, 5, 0.0f); }
                    if (t == DIGITAL) { p.setP (0, 3, 0.0f); p.setP (0, 6, 0.0f); p.setP (0, 7, 0.5f); }
                    if (t == BBD) { p.setP (0, 3, 0.0f); p.setP (0, 5, 0.0f); }
                    for (int w = 0; w < 30; ++w) { std::vector<float> o0; run (p, 1, o0); }   // fa assestare i tempi
                    p.prepareToPlay (sr, 512); for (int k = 0; k < kSlots; ++k) if (k) p.setSlot (k, OFF);
                    std::vector<float> o; run (p, 900, o, true);
                    const size_t n = o.size() / 2; const float set = map::delayMs (tv);
                    // inviluppo a finestre di 2 ms, picchi locali sopra -60 dB
                    std::vector<std::pair<double, double>> pk;
                    const int W = 96;
                    for (size_t i = 4097; i + W < n; i += W)
                    {
                        double m = 0; size_t at = i; for (size_t j = i; j < i + W; ++j) { const double v = std::abs (o[2 * j]) + std::abs (o[2 * j + 1]); if (v > m) { m = v; at = j; } }
                        if (m > 1e-3 && (pk.empty() || (at - 4096) / sr * 1000.0 - pk.back().first > set * 0.5)) pk.push_back ({ (at - 4096) / sr * 1000.0, m });
                        else if (! pk.empty() && (at - 4096) / sr * 1000.0 - pk.back().first <= set * 0.5 && m > pk.back().second) pk.back() = { (at - 4096) / sr * 1000.0, m };
                        if (pk.size() > 5) break;
                    }
                    std::printf ("%-10s tempo %-8s fb %-5s | eco:", info (t).name, (juce::String (juce::roundToInt (set)) + " ms").toRawUTF8(), paramText (t, 1, fb, nullptr).toRawUTF8());
                    for (size_t j = 0; j < pk.size(); ++j) std::printf (" %.0fms/%.1fdB", pk[j].first, 20 * std::log10 (pk[j].second / pk[0].second));
                    std::printf ("\n");
                }
            return 0;
        }
        // preset con musica: picco e NaN
        for (int i = 0; i < (int) factoryPresets().size(); ++i)
        {
            Processor p; p.prepareToPlay (sr, 512); p.loadFactory (i); p.updateIRs(); juce::Thread::sleep (300);
            std::vector<float> o; run (p, 800, o); bool nan = false; float pk = 0; for (float x : o) { if (! std::isfinite (x)) nan = true; pk = std::max (pk, std::abs (x)); }
            double e = 0; for (float x : o) e += (double) x * x; std::printf ("%-10s %-24s picco %.2f  rms %.1f dB  nan %d\n", factoryPresets()[(size_t) i].cat.toRawUTF8(), factoryPresets()[(size_t) i].name.toRawUTF8(), pk, 10 * std::log10 (e / o.size() + 1e-20), (int) nan);
        }
        // feedback al massimo + freeze: deve restare limitato
        {
            Processor p; p.prepareToPlay (sr, 512); p.setSlot (0, TAPE); p.setP (0, 1, 1.0f); p.setSlot (1, HALL); p.setSlot (2, SHIMMER); p.setP (2, 2, 1.0f); p.setSlot (3, SPACED); p.setP (3, 1, 1.0f); p.setSlot (4, MULTITAP); p.setP (4, 1, 1.0f); p.setSlot (5, REVERSE); p.setP (5, 1, 1.0f); for (int k = 0; k < kSlots; ++k) p.P (mixId (k))->setValueNotifyingHost (0.5f); p.P ("freeze")->setValueNotifyingHost (1.0f);
            std::vector<float> o; run (p, 2000, o); float pk = 0; bool nan = false; for (float x : o) { if (! std::isfinite (x)) nan = true; pk = std::max (pk, std::abs (x)); }
            std::printf ("feedback max + freeze, 21 s: picco %.2f nan %d\n", pk, (int) nan);
        }
        {
            Processor p; p.prepareToPlay (sr, 512); for (int k = 0; k < kSlots; ++k) p.setSlot (k, OFF);
            p.setSlot (0, SHIMMER); p.setP (0, 0, 1.0f); p.setP (0, 2, 1.0f); p.setP (0, 4, 1.0f); p.P (mixId (0))->setValueNotifyingHost (1.0f);
            std::vector<float> o; run (p, 3000, o); float pk = 0, pkEnd = 0; for (size_t i = 0; i < o.size(); ++i) { pk = std::max (pk, std::abs (o[i])); if (i > o.size() * 9 / 10) pkEnd = std::max (pkEnd, std::abs (o[i])); }
            std::printf ("shimmer al massimo 32 s: picco %.2f, picco nell'ultimo 10%% %.2f\n", pk, pkEnd);
        }
        {   // IR dell'utente: scrive un wav e lo carica
            juce::File wf ("/tmp/test_ir.wav"); wf.deleteFile();
            { auto ir = makeIR (2, 1.0f, 0.0f, false, sr); juce::WavAudioFormat wav; std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (new juce::FileOutputStream (wf), sr, 2, 24, {}, 0)); w->writeFromAudioSampleBuffer (ir, 0, ir.getNumSamples()); }
            Processor p; p.prepareToPlay (sr, 512); for (int k = 0; k < kSlots; ++k) p.setSlot (k, OFF);
            const bool ok = p.setUserIR (2, wf); juce::Thread::sleep (600);
            p.P (mixId (2))->setValueNotifyingHost (1.0f); p.P ("dry")->setValueNotifyingHost (0.0f);
            std::vector<float> o; run (p, 300, o); double e = 0; for (size_t i = o.size() / 2; i < o.size(); ++i) e += (double) o[i] * o[i];
            std::printf ("IR utente caricata %s, slot 3 = %s / %s, rms %.1f dB\n", ok ? "si" : "no", info (p.slotType (2)).name, paramText (CONV, 0, p.value (pId (2, 0)), nullptr).toRawUTF8(), 10 * std::log10 (e / (o.size() / 2) + 1e-20));
            juce::MemoryBlock mb; p.getStateInformation (mb); Processor q; q.setStateInformation (mb.getData(), (int) mb.getSize()); std::printf ("IR salvata nella sessione: %s\n", q.userIRName (2).toRawUTF8());
        }
        { Processor p; p.prepareToPlay (sr, 512); p.loadFactory (4); p.swapSlots (0, 1); std::printf ("scambio 1<->2: %s %s\n", info (p.slotType (0)).name, info (p.slotType (1)).name); }
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "click")
    {   // per ogni motore e ogni manopola: salto a meta' mentre suona un seno a 220 Hz; energia sopra 5 kHz della differenza = clic
        auto render = [&] (int t, int pi, float from, float to, bool jump, bool typeSwap)
        {
            Processor p; p.prepareToPlay (sr, 512);
            for (int k = 0; k < kSlots; ++k) p.setSlot (k, k == 0 ? t : OFF);
            p.P (mixId (0))->setValueNotifyingHost (0.6f);
            if (pi >= 0) p.setP (0, pi, from);
            juce::AudioBuffer<float> b (2, 512); std::vector<float> o; long n = 0;
            for (int blk = 0; blk < 160; ++blk)
            {
                if (blk == 120 && jump) { if (typeSwap) p.setSlot (0, t == HALL ? PLATE : HALL); else if (pi >= 0) p.setP (0, pi, to); else p.P (mixId (0))->setValueNotifyingHost (0.0f); }
                for (int i = 0; i < 512; ++i, ++n) { const float v = 0.4f * (float) std::sin (2 * juce::MathConstants<double>::pi * 220.0 * n / sr); b.setSample (0, i, v); b.setSample (1, i, v); }
                juce::MidiBuffer m; p.processBlock (b, m); if (blk >= 118 && blk < 126) for (int i = 0; i < 512; ++i) o.push_back (b.getSample (0, i));
            }
            return o;
        };
        auto hfEnergy = [] (const std::vector<float>& a, const std::vector<float>& b)
        {
            juce::dsp::IIR::Filter<float> f[4]; for (auto& x : f) x.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass (48000.0, 5000.0, 0.707f);
            double e = 0; for (size_t i = 0; i < a.size(); ++i) { float d = a[i] - b[i]; for (auto& x : f) d = x.processSample (d); if (i > 64) e = std::max (e, (double) std::abs (d)); }
            return 20 * std::log10 (e + 1e-12);
        };
        for (int t = 1; t < kNumTypes; ++t)
        {
            if (t == CONV) continue;
            juce::String line; line << juce::String (info (t).name).paddedRight (' ', 11);
            float worst = -200; juce::String wn;
            for (int pi = 0; pi < info (t).n; ++pi)
            {
                const auto held = render (t, pi, 0.25f, 0.25f, false, false), jm = render (t, pi, 0.25f, 0.75f, true, false);
                const float hf = (float) hfEnergy (jm, held);
                if (hf > worst) { worst = hf; wn = info (t).pn[pi]; }
                if (hf > -60) line << " " << info (t).pn[pi] << " " << juce::String (hf, 0);
            }
            const auto held = render (t, -1, 0, 0, false, false), sw = render (t, -1, 0, 0, true, true);
            line << " | cambio motore " << juce::String (hfEnergy (sw, held), 0) << " | peggiore " << wn << " " << juce::String (worst, 0);
            std::printf ("%s\n", line.toRawUTF8());
        }
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "alias")
    {   // seno a 7 kHz forte dentro ogni motore: tutto quello che esce sotto 6 kHz e' aliasing (tranne i motori che traspongono)
        for (int t = 1; t < kNumTypes; ++t)
        {
            if (t == SHIMMER || t == CLOUD || t == GRANULAR || t == SPACED) continue;
            Processor p; p.prepareToPlay (sr, 512);
            for (int k = 0; k < kSlots; ++k) p.setSlot (k, k == 0 ? t : OFF);
            p.P (mixId (0))->setValueNotifyingHost (1.0f); p.P ("dry")->setValueNotifyingHost (0.0f);
            if (t == TAPE || t == BBD) p.setP (0, 5, std::getenv ("JDR") ? (float) std::atof (std::getenv ("JDR")) : 1.0f);
            if (t == GATED) p.setP (0, 4, 1.0f);
            if (t == SPRING) p.setP (0, 3, 1.0f);
            if (t == CONV) { p.updateIRs(); juce::Thread::sleep (600); }
            const double f0 = std::getenv ("JF") ? std::atof (std::getenv ("JF")) : 7000.0; const float amp = 0.9f;
            juce::AudioBuffer<float> b (2, 512); std::vector<float> o; long n = 0;
            for (int blk = 0; blk < 300; ++blk)
            {
                for (int i = 0; i < 512; ++i, ++n) { const float v = amp * (float) std::sin (2 * juce::MathConstants<double>::pi * f0 * n / sr); b.setSample (0, i, v); b.setSample (1, i, v); }
                juce::MidiBuffer m; p.processBlock (b, m); if (blk >= 300 - 32) for (int i = 0; i < 512; ++i) o.push_back (b.getSample (0, i));
            }
            const int order = 14, N = 1 << order; juce::dsp::FFT fft (order); std::vector<float> d (2 * N, 0.0f);
            for (int i = 0; i < N; ++i) d[(size_t) i] = o[(size_t) i] * (0.5f - 0.5f * std::cos (2 * juce::MathConstants<float>::pi * i / (N - 1)));
            fft.performFrequencyOnlyForwardTransform (d.data());
            float ref = 0, al = 0; for (int i = 1; i < N / 2; ++i) { const double f = i * sr / N; if (std::abs (f - f0) < 60) ref = std::max (ref, d[(size_t) i]); else if (f > 40 && f < f0 - 1000) al = std::max (al, d[(size_t) i]); }
            std::printf ("%-12s aliasing sotto %.0f kHz: %.1f dB\n", info (t).name, (f0 - 1000) / 1000, 20 * std::log10 (al / (ref + 1e-20) + 1e-12));
        }
        return 0;
    }
    if (argc > 2 && juce::String (argv[1]) == "snap")
    {
        Processor p; p.prepareToPlay (sr, 512);
        if (auto* th = std::getenv ("JTHEME")) p.uiTheme = std::atoi (th);
        if (auto* pr = std::getenv ("JPR")) p.loadFactory (std::atoi (pr));
        std::vector<float> o; run (p, 400, o);
        Editor ed (p); ed.tickForTest();
        if (std::getenv ("JADV")) ed.advPanel.open (std::atoi (std::getenv ("JADV")));
        auto img = ed.createComponentSnapshot (ed.getLocalBounds(), true, 1.0f);
        juce::File f (argv[2]); f.deleteFile(); juce::FileOutputStream os (f); juce::PNGImageFormat().writeImageToStream (img, os);
        return 0;
    }
    return 0;
}
