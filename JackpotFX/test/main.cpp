#include <juce_dsp/juce_dsp.h>
#include "../Source/FxProcessor.h"
#include "../Source/FxEditor.h"
using namespace jfx;
int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gi;
    const double sr = 48000;
    auto run = [&] (Processor& p, int blocks, std::vector<float>& out, std::function<void (int, juce::MidiBuffer&)> midi = nullptr)
    {
        juce::AudioBuffer<float> b (2, 512); juce::Random r (3);
        for (int k = 0; k < blocks; ++k)
        {
            for (int i = 0; i < 512; ++i) { const int n = k * 512 + i; const double t = n / sr; const float beat = (float) std::fmod (t * 2.0, 1.0);
                const float v = (float) (std::sin (2 * 3.14159 * 55 * t) * std::exp (-beat * 8) * 0.5 + (r.nextFloat() * 2 - 1) * std::exp (-std::fmod (t * 4.0, 1.0) * 30) * 0.2 + std::sin (2 * 3.14159 * 440 * t) * 0.05);
                b.setSample (0, i, v); b.setSample (1, i, v * 0.9f); }
            juce::MidiBuffer m; if (midi) midi (k, m);
            p.processBlock (b, m);
            for (int i = 0; i < 512; ++i) out.push_back (b.getSample (0, i));
        }
    };
    if (argc > 1 && juce::String (argv[1]) == "lab")
    {   // per ogni effetto nello slot 1: aliasing con toni acuti, click quando la manopola salta del 30%, NaN agli estremi
        const char* only = std::getenv ("JONLY");
        auto spur = [&] (int t, float amt, int bin, float amp, float* harm)
        {
            Processor p; p.prepareToPlay (sr, 512); for (int k = 0; k < kSlots; ++k) p.setSlot (k, pfx::OFF, -1); p.setSlot (0, t, amt);
            if (auto* av = std::getenv ("JADVV")) p.setAdv (0, std::atoi (av), 1.0f);
            if (auto* az = std::getenv ("JADVZ")) p.setAdv (0, std::atoi (az), 0.0f);
            const int N = 65536; const double ff = bin * sr / N; std::vector<float> o; juce::AudioBuffer<float> b (2, 512); juce::MidiBuffer m;
            for (int off = 0; off < N * 2; off += 512) { for (int i = 0; i < 512; ++i) { const float x = amp * (float) std::sin (2 * 3.14159265358979 * ff * (off + i) / sr); b.setSample (0, i, x); b.setSample (1, i, x); } p.processBlock (b, m); for (int i = 0; i < 512; ++i) o.push_back (b.getSample (0, i)); }
            juce::dsp::FFT fft (16); std::vector<float> a ((size_t) 2 * N, 0);
            for (int i = 0; i < N; ++i) { const double w = 6.283185307179586 * i / N; a[(size_t) i] = o[(size_t) (N + i)] * (float) (0.35875 - 0.48829 * std::cos (w) + 0.14128 * std::cos (2 * w) - 0.01168 * std::cos (3 * w)); }
            fft.performFrequencyOnlyForwardTransform (a.data()); float ref0 = 1e-9f; for (int k = bin - 5; k <= bin + 5; ++k) ref0 = std::max (ref0, a[(size_t) k]);
            float worst = 0; double hs = 0;
            for (int k = 40; k < N / 2; ++k) { bool h = false; for (int mm = 1; mm < 8; ++mm) if (std::abs (k - bin * mm) <= 14) { h = true; if (mm > 1) hs += (double) a[(size_t) k] * a[(size_t) k]; } if (std::abs (k - bin) < bin / 20) h = true; if (! h && a[(size_t) k] > worst) worst = a[(size_t) k]; }
            if (harm) *harm = juce::Decibels::gainToDecibels ((float) std::sqrt (hs) / ref0, -200.0f);
            return juce::Decibels::gainToDecibels (worst / ref0, -200.0f);
        };
        auto tone1k = [&] (Processor& p, int blocks, std::vector<float>& o, std::function<void (int)> at)
        {
            juce::AudioBuffer<float> b (2, 512); juce::MidiBuffer m;
            for (int k = 0; k < blocks; ++k) { at (k); for (int i = 0; i < 512; ++i) { const float x = 0.25f * (float) std::sin (2 * 3.14159265358979 * 999.76 * (k * 512 + i) / sr); b.setSample (0, i, x); b.setSample (1, i, x * 0.9f); } p.processBlock (b, m); for (int i = 0; i < 512; ++i) o.push_back (b.getSample (0, i)); }
        };
        for (int t = 1; t < pfx::kNumTypes; ++t)
        {
            const auto& I = pfx::info (t); if (only && ! juce::String (I.name).containsIgnoreCase (only)) continue;
            if (pfx::familyOfMix (t) >= 0) continue;
            const float amt = I.bipolar ? 0.8f : 0.6f; float h1 = 0;
            const float s1 = spur (t, amt, 8603, 0.5f, &h1), s2 = spur (t, amt, 13653, 0.5f, nullptr);
            // click: dal valore neutro +30% (o -30%) a meta' del suono, contro lo stesso valore tenuto da sempre
            float cdb = -200;
            if (! I.spring && t != pfx::RISER && t != pfx::DOWNLIFTER && t != pfx::RISERSYN && t != pfx::SNAREROLL && t != pfx::SUBDROP)
            {
                const float v0 = juce::jlimit (0.0f, 1.0f, I.neutral + 0.35f), v1 = v0 + 0.3f <= 1 ? v0 + 0.3f : v0 - 0.3f; const int J = 100;
                Processor pa; pa.prepareToPlay (sr, 512); pa.setSlot (0, t, v0); for (int k = 1; k < kSlots; ++k) pa.setSlot (k, pfx::OFF, -1);
                Processor pb; pb.prepareToPlay (sr, 512); pb.setSlot (0, t, v1); for (int k = 1; k < kSlots; ++k) pb.setSlot (k, pfx::OFF, -1);
                std::vector<float> oa, ob; tone1k (pa, J + 20, oa, [&] (int k) { if (k == J) pa.P (amtId (0))->setValueNotifyingHost (v1); }); tone1k (pb, J + 20, ob, [] (int) {});
                // energia sopra i 5 kHz della differenza nei 50 ms dopo il salto (passa alto semplice a 2 poli)
                // passa alto ripido (8 poli) a 5 kHz: del tono a 1 kHz non resta niente, resta solo lo scatto
                const double w0 = 2 * 3.14159265 * 5000 / sr, c0 = std::cos (w0); static const double Qs[4] = { 0.5098, 0.6013, 0.9000, 2.5629 };
                double st[4][4] {}; float pk = 0;
                for (int i = (J - 4) * 512; i < (J + 5) * 512; ++i)
                {
                    double x = oa[(size_t) i] - ob[(size_t) i];
                    for (int q = 0; q < 4; ++q)
                    {
                        const double al = std::sin (w0) / (2 * Qs[q]), b0 = (1 + c0) / 2 / (1 + al), b1 = -(1 + c0) / (1 + al), a1 = -2 * c0 / (1 + al), a2 = (1 - al) / (1 + al);
                        const double y = b0 * x + b1 * st[q][0] + b0 * st[q][1] - a1 * st[q][2] - a2 * st[q][3]; st[q][1] = st[q][0]; st[q][0] = x; st[q][3] = st[q][2]; st[q][2] = y; x = y;
                    }
                    if (i >= J * 512) pk = std::max (pk, (float) std::abs (x));
                }
                cdb = juce::Decibels::gainToDecibels (pk / 0.25f, -200.0f);
            }
            std::printf ("%-13s amt %.1f  spur 6,3k %6.1f  10k %6.1f  armoniche %6.1f   click %6.1f %s%s\n", I.name, amt, s1, s2, h1, cdb, (s1 > -60 || s2 > -60) ? "ALIAS" : "", cdb > -35 ? " CLICK" : "");
        }
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "dsp")
    {
        Processor p0; p0.prepareToPlay (sr, 512); std::vector<float> ref; run (p0, 300, ref);
        // neutro: deve essere identico all'ingresso
        Processor pn; pn.prepareToPlay (sr, 512); std::vector<float> a; run (pn, 200, a);
        { bool same = true; for (size_t i = 0; i < a.size(); ++i) if (a[i] != ref[i]) same = false; std::printf ("preset di partenza a riposo: uguale all'ingresso %s\n", same ? "si" : "no"); }
        for (int i = 0; i < (int) factoryPresets().size(); ++i)
        {
            Processor p; p.prepareToPlay (sr, 512); p.loadFactory (i);
            for (int k = 0; k < kSlots; ++k) { const int t = p.slotType (k); if (! pfx::info (t).spring) p.P (amtId (k))->setValueNotifyingHost (pfx::info (t).bipolar ? 0.8f : 0.6f); }
            std::vector<float> o; run (p, 300, o); bool nan = false; float pk = 0; for (float x : o) { if (! std::isfinite (x)) nan = true; pk = std::max (pk, std::abs (x)); }
            std::printf ("%-22s tutti gli slot alzati: picco %.2f nan %d\n", factoryPresets()[(size_t) i].name, pk, (int) nan);
        }
        Processor pc; pc.prepareToPlay (sr, 512); std::vector<float> c; run (pc, 120, c, [] (int k, juce::MidiBuffer& m) { if (k == 20) m.addEvent (juce::MidiMessage::controllerEvent (1, 76, 127), 0); });
        std::printf ("CC 76 -> slot 7 (stutter): valore in arrivo %.2f\n", pc.ccPend[6].load());
        Processor pd; pd.prepareToPlay (sr, 512); pd.ctrlOn = false; std::vector<float> d; run (pd, 30, d, [] (int k, juce::MidiBuffer& m) { if (k == 2) m.addEvent (juce::MidiMessage::controllerEvent (1, 76, 127), 0); });
        std::printf ("MIDI OFF: CC ignorato %s\n", pd.ccPend[6].load() < 0 ? "si" : "no");
        for (int fam = 0; fam < 4; ++fam)
        {
            const int eff[4] = { pfx::GATE, pfx::STUTTER, pfx::BEATROLL, pfx::TREMOLO }, mx[4] = { pfx::GATEMIX, pfx::STUTMIX, pfx::ROLLMIX, pfx::TREMMIX };
            float res[3];
            for (int m = 0; m < 3; ++m)
            {
                Processor p; p.prepareToPlay (sr, 512); p.setSlot (0, eff[fam], 0.5f); p.setSlot (1, m == 0 ? pfx::OFF : mx[fam], m == 1 ? 0.3f : 0.0f);
                for (int k = 2; k < kSlots; ++k) p.setSlot (k, pfx::OFF, -1);
                std::vector<float> o; run (p, 200, o); double d = 0, e = 0; for (size_t i = 0; i < o.size(); ++i) { d += (o[i] - ref[i]) * (o[i] - ref[i]); e += ref[i] * ref[i]; }
                res[m] = (float) (10 * std::log10 (d / e + 1e-20));
            }
            std::printf ("%-10s differenza dal pulito: senza MIX %.1f dB, MIX 30%% %.1f dB, MIX 0%% %.1f dB\n", pfx::info (eff[fam]).name, res[0], res[1], res[2]);
        }
        for (int t = 21; t < pfx::kNumTypes; ++t)
        {
            Processor p; p.prepareToPlay (sr, 512); p.setSlot (0, t, pfx::info (t).bipolar ? 0.8f : 0.6f); for (int k = 1; k < kSlots; ++k) p.setSlot (k, pfx::OFF, -1);
            std::vector<float> o; run (p, 300, o); bool nan = false; double d = 0, e = 0; for (size_t i = 0; i < o.size() && i < ref.size(); ++i) { if (! std::isfinite (o[i])) nan = true; d += (o[i] - ref[i]) * (o[i] - ref[i]); e += ref[i] * ref[i]; }
            std::printf ("nuovo %-14s cambia il suono di %.1f dB  nan %d\n", pfx::info (t).name, 10 * std::log10 (d / e + 1e-20), (int) nan);
        }
        for (int t : { pfx::CRUSH, pfx::LOFI, pfx::TAPE, pfx::VINYL })
        {
            Processor p; p.prepareToPlay (sr, 512); p.setSlot (0, t, 0.8f); for (int k = 1; k < kSlots; ++k) p.setSlot (k, pfx::OFF, -1);
            std::vector<float> o; run (p, 200, o); double e = 0, e0 = 0; for (size_t i = 0; i < o.size(); ++i) { e += o[i] * o[i]; e0 += ref[i] * ref[i]; }
            std::printf ("%-6s a 80%%: volume %+.1f dB\n", pfx::info (t).name, 10 * std::log10 (e / e0));
        }
        // avanzati: con i valori base il suono deve restare identico a prima
        {
            int bad = 0, tested = 0;
            for (int t = 1; t < pfx::kNumTypes; ++t)
            {
                if (pfx::adv (t).n == 0) continue; ++tested;
                pfx::Engine e1, e2; e1.prepare (sr); e2.prepare (sr);
                int ty[8] = { t, 0, 0, 0, 0, 0, 0, 0 }; float am[8] = { pfx::info (t).bipolar ? 0.8f : 0.6f, 0, 0, 0, 0, 0, 0, 0 };
                float av[8 * pfx::kAdv]; for (int k = 0; k < 8; ++k) for (int i = 0; i < pfx::kAdv; ++i) av[k * pfx::kAdv + i] = i < pfx::adv (ty[k]).n ? pfx::adv (ty[k]).def[i] : 0.5f;
                float mx[8] = { 1, 1, 1, 1, 1, 1, 1, 1 };
                double maxd = 0;
                for (int b = 0; b < 200; ++b)
                {
                    float l1[512], r1[512], l2[512], r2[512];
                    for (int i = 0; i < 512; ++i) { l1[i] = l2[i] = ref[(size_t) (b * 512 + i)]; r1[i] = r2[i] = ref[(size_t) (b * 512 + i)] * 0.9f; }
                    const double ppq = b * 512 / sr * 2.0;
                    e1.process (l1, r1, 512, ty, am, 120, ppq, true); e2.process (l2, r2, 512, ty, am, 120, ppq, true, av, mx);
                    for (int i = 0; i < 512; ++i) maxd = std::max (maxd, (double) std::abs (l1[i] - l2[i]));
                }
                if (maxd > 1e-4) { ++bad; std::printf ("  avanzati base diversi: %s (%.5f)\n", pfx::info (t).name, maxd); }
            }
            std::printf ("avanzati ai valori base = suono di prima: %d effetti provati, %d diversi\n", tested, bad);
            // valori estremi: niente NaN, niente esplosioni
            int worst = 0; float wpk = 0;
            for (int t = 1; t < pfx::kNumTypes; ++t)
                for (int ex = 0; ex < 2; ++ex)
                {
                    if (pfx::adv (t).n == 0) continue;
                    Processor p; p.prepareToPlay (sr, 512); p.setSlot (0, t, pfx::info (t).bipolar ? 0.85f : 0.8f); for (int k = 1; k < kSlots; ++k) p.setSlot (k, pfx::OFF, -1);
                    for (int i = 0; i < pfx::adv (t).n; ++i) p.setAdv (0, i, (float) ex);
                    std::vector<float> o; run (p, 200, o); float pk = 0; bool nan = false; for (float x : o) { if (! std::isfinite (x)) nan = true; pk = std::max (pk, std::abs (x)); }
                    if (nan || pk > 4) std::printf ("  ESTREMO %s %s: picco %.2f nan %d\n", pfx::info (t).name, ex ? "tutto su" : "tutto giu", pk, (int) nan);
                    if (pk > wpk) { wpk = pk; worst = t; }
                }
            std::printf ("avanzati agli estremi: picco massimo %.2f (%s)\n", wpk, pfx::info (worst).name);
            // mix dello slot
            for (float m : { 0.0f, 0.5f, 1.0f })
            {
                Processor p; p.prepareToPlay (sr, 512); p.setSlot (0, pfx::REVERB, 0.8f); for (int k = 1; k < kSlots; ++k) p.setSlot (k, pfx::OFF, -1);
                p.P (mixId (0))->setValueNotifyingHost (m);
                std::vector<float> o; run (p, 200, o); double d = 0, e = 0; for (size_t i = 0; i < o.size(); ++i) { d += (o[i] - ref[i]) * (o[i] - ref[i]); e += ref[i] * ref[i]; }
                std::printf ("reverb 80%% con MIX slot %3.0f%%: differenza dal pulito %.1f dB\n", m * 100, 10 * std::log10 (d / e + 1e-20));
            }
        }
        // riverbero: tempo di decadimento misurato (T30 x 2) per ogni tipo
        for (int ty = 0; ty < pfx::Verb::kTypes; ++ty)
            for (float dec : { 1.0f, 3.0f })
            {
                pfx::Verb v; v.prepare (sr); pfx::Verb::Params vp; vp.type = ty; vp.decay = dec;
                const int NN = (int) (sr * 8); std::vector<float> il ((size_t) NN, 0.0f), ol ((size_t) NN), orr ((size_t) NN);
                il[100] = 1.0f;
                for (int o = 0; o < NN; o += 512) v.process (il.data() + o, il.data() + o, ol.data() + o, orr.data() + o, std::min (512, NN - o), vp);
                // curva di Schroeder
                std::vector<double> e ((size_t) NN); double acc = 0; bool nan = false;
                for (int i = NN - 1; i >= 0; --i) { if (! std::isfinite (ol[(size_t) i])) nan = true; acc += (double) ol[(size_t) i] * ol[(size_t) i] + (double) orr[(size_t) i] * orr[(size_t) i]; e[(size_t) i] = acc; }
                auto tAt = [&] (double db) { for (int i = 0; i < NN; ++i) if (10 * std::log10 (e[(size_t) i] / e[0] + 1e-30) < db) return i / sr; return NN / sr; };
                float pk = 0; for (float x : ol) pk = std::max (pk, std::abs (x));
                std::printf ("riverbero %-8s decay %.0f s: misurato RT60 %.2f s  picco %.2f  nan %d\n", pfx::Verb::typeName (ty), dec, (tAt (-35) - tAt (-5)) * 2.0, pk, (int) nan);
            }
        // ogni effetto: quantità 0 / 50 / 100%, NaN, picco, e quantità mossa mentre suona (clic)
        {
            double dj = 0; for (size_t i = 1; i < ref.size(); ++i) dj = std::max (dj, (double) std::abs (ref[i] - ref[i - 1]));
            int probs = 0;
            for (int t = 1; t < pfx::kNumTypes; ++t)
            {
                float res[3]; float pk = 0; bool nan = false;
                for (int m = 0; m < 3; ++m)
                {
                    Processor p; p.prepareToPlay (sr, 512); p.setSlot (0, t, m * 0.5f); for (int k = 1; k < kSlots; ++k) p.setSlot (k, pfx::OFF, -1);
                    std::vector<float> o; run (p, 200, o); double d = 0, e = 0;
                    for (size_t i = 0; i < o.size(); ++i) { if (! std::isfinite (o[i])) nan = true; pk = std::max (pk, std::abs (o[i])); d += (o[i] - ref[i]) * (o[i] - ref[i]); e += ref[i] * ref[i]; }
                    res[m] = (float) (10 * std::log10 (d / e + 1e-20));
                }
                Processor p; p.prepareToPlay (sr, 512); p.setSlot (0, t, 0.0f); for (int k = 1; k < kSlots; ++k) p.setSlot (k, pfx::OFF, -1);
                std::vector<float> o; run (p, 200, o, [&] (int k, juce::MidiBuffer&) { p.P (amtId (0))->setValueNotifyingHost (0.5f + 0.5f * std::sin (k * 0.15f)); });
                double mj = 0; for (size_t i = 1; i < o.size(); ++i) mj = std::max (mj, (double) std::abs (o[i] - o[i - 1]));
                const bool bad = nan || pk > 3 || res[2] < -25 || mj > dj * 6;
                if (bad) ++probs;
                std::printf ("%s %-14s 0%% %6.1f  50%% %6.1f  100%% %6.1f dB  picco %.2f  salto %.2fx%s\n", bad ? "!!" : "  ", pfx::info (t).name, res[0], res[1], res[2], pk, mj / dj, nan ? " NaN" : "");
            }
            std::printf ("effetti con problemi: %d\n", probs);
        }
        pc.swapSlots (0, 4); std::printf ("scambio 1<->5: slot1 %s, slot5 %s\n", pfx::info (pc.slotType (0)).name, pfx::info (pc.slotType (4)).name);
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "perf")
    {
        // CPU: 20 s di audio col caso peggiore, in % del tempo reale
        {
            Processor p; p.prepareToPlay (sr, 512); for (int k = 0; k < kSlots; ++k) { if (k < 8) p.P (amtId (k))->setValueNotifyingHost (0.7f); }
            std::vector<float> o; o.reserve (2 * 512 * 2000);
            const auto t0 = juce::Time::getMillisecondCounterHiRes(); run (p, 1875, o); const auto ms = juce::Time::getMillisecondCounterHiRes() - t0;
            std::printf ("CPU caso peggiore: %.1f%% di un core (20 s in %.0f ms)\n", ms / 200.0, ms);
        }
        Processor p; p.prepareToPlay (sr, 512); for (int k = 0; k < kSlots; ++k) { if (k < 8) p.P (amtId (k))->setValueNotifyingHost (0.7f); }
        std::vector<float> o; run (p, 300, o);
        Editor ed (p); ed.tickForTest();
        const int th[] = { 3, 4, 8, 9, 10, 1, 2 };
        for (int t : th)
        {
            skin::cur() = t; neo::K::theme = -1; skin::apply();
            ed.repaint(); for (auto* c : ed.getChildren()) c->repaint();
            auto t0 = juce::Time::getMillisecondCounterHiRes();
            auto img = ed.createComponentSnapshot (ed.getLocalBounds(), true, 1.0f);
            const double full = juce::Time::getMillisecondCounterHiRes() - t0;
            // regime: 30 fotogrammi come li disegna il timer (pannello statico in cache)
            t0 = juce::Time::getMillisecondCounterHiRes();
            for (int i = 0; i < 30; ++i) { ed.tickForTest(); ed.scope.createComponentSnapshot (ed.scope.getLocalBounds(), true, 1.0f); }
            const double fr = (juce::Time::getMillisecondCounterHiRes() - t0) / 30.0;
            std::printf ("tema %2d: primo disegno %.1f ms, fotogramma onda + timer %.2f ms\n", t, full, fr);
            juce::File f ("/tmp/" "fx" "_tema" + juce::String (t) + ".png"); f.deleteFile(); juce::FileOutputStream os (f); juce::PNGImageFormat().writeImageToStream (img, os);
        }
        return 0;
    }
    if (argc > 2 && juce::String (argv[1]) == "snap")
    {
        Processor p; p.prepareToPlay (sr, 512);
        if (auto* th = std::getenv ("JTHEME")) p.uiTheme = std::atoi (th);
        if (auto* pr = std::getenv ("JPR")) p.loadFactory (std::atoi (pr));
        p.P (amtId (2))->setValueNotifyingHost (0.55f); p.P (amtId (0))->setValueNotifyingHost (0.3f);
        std::vector<float> o; run (p, 400, o);
        Editor ed (p); ed.tickForTest();
        if (auto* ss = std::getenv ("JSLOT")) p.setSlot (2, std::atoi (ss), 0.5f);
        if (std::getenv ("JADV")) { p.setAdv (2, 0, 0.3f); ed.advPanel.open (std::atoi (std::getenv ("JADV"))); for (auto* c : ed.cards) c->repaint(); }
        auto img = ed.createComponentSnapshot (ed.getLocalBounds(), true, 1.0f);
        juce::File f (argv[2]); f.deleteFile(); juce::FileOutputStream os (f); juce::PNGImageFormat().writeImageToStream (img, os);
        return 0;
    }
    return 0;
}
