#include "../Source/MixProcessor.h"
#include "../Source/MixEditor.h"
#include "TestLab.h"
using namespace jmx;
static void feed (Processor& p, int blocks, float amp, double freq, double& ph, std::vector<float>* out = nullptr, bool* nan = nullptr)
{
    juce::AudioBuffer<float> b (4, 512); juce::MidiBuffer m; b.clear();
    for (int k = 0; k < blocks; ++k)
    {
        for (int i = 0; i < 512; ++i) { const float v = freq > 0 ? (float) std::sin (ph) * amp : 0.0f; ph += 2 * juce::MathConstants<double>::pi * freq / 44100.0; b.setSample (0, i, v); b.setSample (1, i, v * 0.9f); b.setSample (2, i, 0); b.setSample (3, i, 0); }
        juce::AudioBuffer<float> io (b.getArrayOfWritePointers(), 2, 512);
        p.processBlock (io, m);
        for (int i = 0; i < 512; ++i) { const float x = b.getSample (0, i); if (nan && ! std::isfinite (x)) *nan = true; if (out) out->push_back (x); }
    }
}
static float rmsDb (const std::vector<float>& v, size_t from) { double e = 0; for (size_t i = from; i < v.size(); ++i) e += v[i] * v[i]; return (float) juce::Decibels::gainToDecibels (std::sqrt (e / juce::jmax<size_t> (1, v.size() - from)), -200.0); }
static void pump (Processor& p) { p.forceRebuild(); }
int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    if (argc > 1 && juce::String (argv[1]) == "listen")
    {   // SOLO e DELTA contro il calcolo a mano; ANNULLA/RIPETI su un comando e sul cambio preset
        const double sr = 48000; const int pr = std::getenv ("JPRESET") ? std::atoi (std::getenv ("JPRESET")) : 1;
        juce::Random r (9); std::vector<float> in ((size_t) sr * 3); float b0 = 0;
        for (auto& v : in) { const float w = r.nextFloat() * 2 - 1; b0 = 0.98f * b0 + w * 0.2f; v = (b0 + w * 0.1f) * 0.5f; }
        auto mk = [&] (std::function<void (Processor&)> f) { auto p = std::make_unique<Processor>(); p->setPlayConfigDetails (2, 2, sr, 512); p->prepareToPlay (sr, 512); p->loadPreset (pr); f (*p); p->forceRebuild(); return p; };
        auto rnd = [&] (Processor& p) { return testlab::render (p, (int) in.size(), [&] (int k) { return in[(size_t) k]; }, 512, true); };
        auto diff = [&] (const std::vector<float>& a, const std::vector<float>& b) { double e = 0, d = 0; for (size_t i = (size_t) sr; i < a.size(); ++i) { e += (double) a[i] * a[i]; d += std::pow ((double) a[i] - b[i], 2.0); } return 10 * std::log10 (d / (e + 1e-30) + 1e-30); };
        auto n = mk ([] (Processor&) {})->chainLen();
        for (int k = 0; k < n; ++k)
        {
            auto pS = mk ([k] (Processor& p) { p.soloSlot = k; });
            auto pR = mk ([k, n] (Processor& p) { for (int s = 0; s < n; ++s) if (s != k) p.setP (Processor::sid (s, "byp"), 1); p.setP ("eq_on", 0); });
            const auto oS = rnd (*pS), oR = rnd (*pR);
            auto pD = mk ([k, n] (Processor& p) { p.deltaSlot = k; for (int s = 0; s < n; ++s) if (s != k) p.setP (Processor::sid (s, "byp"), 1); p.setP ("eq_on", 0); });
            const auto oD = rnd (*pD);
            // atteso: (uscita col solo modulo k) - (ingresso ritardato della stessa latenza), con EQ e altri moduli come dopo k... qui confronto con solo: delta + ingresso ritardato = uscita solo
            const int L = pR->getLatencySamples(); std::vector<float> rec (oR.size());
            for (size_t i = 0; i < oR.size(); ++i) rec[i] = oD[i] + (i >= (size_t) L ? in[i - (size_t) L] : 0.0f);
            std::printf ("slot %d %-14s  SOLO vs a mano %.1f dB   DELTA+dry vs SOLO %.1f dB\n", k + 1, mod (pS->slotType (k)).name.toRawUTF8(), diff (oR, oS), diff (oS, rec));
        }
        {   // la storia si scrive sul timer dell'APVTS: passi in sequenza nel ciclo dei messaggi
            auto p = mk ([] (Processor&) {}); auto* q = p->apvts.getParameter (Processor::sid (0, "p1")); auto* P = p.get();
            std::vector<std::function<void()>> steps; float v0 = q->getValue(), v1 = 0, v2 = 0, v3 = 0; int n0 = 0, n1 = 0, n2 = 0;
            steps.push_back ([&] { P->undo.beginNewTransaction(); q->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, v0 + 0.3f)); });
            steps.push_back ([&] { v1 = q->getValue(); P->undo.beginNewTransaction(); P->undo.undo(); });
            steps.push_back ([&] { v2 = q->getValue(); P->undo.redo(); });
            steps.push_back ([&] { v3 = q->getValue(); P->undo.beginNewTransaction(); n0 = P->slotType (0) * 100 + P->chainLen(); P->loadPreset (pr + 3); });
            steps.push_back ([&] { n1 = P->slotType (0) * 100 + P->chainLen(); P->undo.beginNewTransaction(); P->undo.undo(); });
            steps.push_back ([&] { n2 = P->slotType (0) * 100 + P->chainLen(); juce::MessageManager::getInstance()->stopDispatchLoop(); });
            std::function<void (size_t)> go = [&] (size_t i) { steps[i](); if (i + 1 < steps.size()) juce::Timer::callAfterDelay (300, [&go, i] { go (i + 1); }); };
            juce::Timer::callAfterDelay (100, [&go] { go (0); });
            juce::MessageManager::getInstance()->runDispatchLoop();
            std::printf ("annulla comando: %.3f -> %.3f -> annulla %.3f -> ripeti %.3f\n", v0, v1, v2, v3);
            std::printf ("annulla preset: moduli %d -> %d -> annulla %d\n", n0, n1, n2);
        }
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "glide")
    {   // dopo il caricamento: livello ogni 0.5 s su rumore rosa costante (JSLOT isola uno slot)
        const double sr = 48000; const int pr = std::getenv ("JPRESET") ? std::atoi (std::getenv ("JPRESET")) : 1;
        juce::Random r (9); std::vector<float> in ((size_t) sr * 12); float b0 = 0, b1 = 0, b2 = 0;
        for (auto& v : in) { const float w = r.nextFloat() * 2 - 1; b0 = 0.99765f * b0 + w * 0.099f; b1 = 0.963f * b1 + w * 0.2965f; b2 = 0.57f * b2 + w * 1.0527f; v = (b0 + b1 + b2 + w * 0.1848f) * 0.12f; }
        auto p = std::make_unique<Processor>(); p->setPlayConfigDetails (2, 2, sr, 512); p->prepareToPlay (sr, 512); p->loadPreset (pr);
        if (auto* o = std::getenv ("JSLOT")) { const int keep = std::atoi (o); for (int s2 = 0; s2 < 8; ++s2) if (s2 + 1 != keep) p->setP ("s" + juce::String (s2 + 1) + "_byp", 1.0f); }
        if (std::getenv ("JNOEQ")) p->setP ("eq_on", 0.0f);
        auto o1 = testlab::render (*p, (int) in.size(), [&] (int k) { return in[(size_t) k]; }, 512, false);
        for (int q = 0; q < 24; ++q) { double e = 0; for (size_t i = (size_t) (q * sr / 2); i < (size_t) ((q + 1) * sr / 2); ++i) e += (double) o1[i] * o1[i]; std::printf ("%.1f ", 10 * std::log10 (e / (sr / 2) + 1e-30)); }
        std::printf ("\n"); return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "blk")
    {   // blocchi irregolari 1..512 contro 512 su rumore rosa forte, preset scelto; JSLOT=n spegne tutti gli slot tranne n
        const double sr = 48000; const int pr = std::getenv ("JPRESET") ? std::atoi (std::getenv ("JPRESET")) : 1;
        juce::Random r (9); std::vector<float> in ((size_t) sr * 4); float b0 = 0, b1 = 0, b2 = 0;
        for (auto& v : in) { const float w = r.nextFloat() * 2 - 1; b0 = 0.99765f * b0 + w * 0.099f; b1 = 0.963f * b1 + w * 0.2965f; b2 = 0.57f * b2 + w * 1.0527f; v = (b0 + b1 + b2 + w * 0.1848f) * 0.12f; }
        auto mk = [&] { auto p = std::make_unique<Processor>(); p->setPlayConfigDetails (2, 2, sr, 512); p->prepareToPlay (sr, 512); p->loadPreset (pr);
            if (auto* o = std::getenv ("JSLOT")) { const int keep = std::atoi (o); for (int s2 = 0; s2 < 8; ++s2) if (s2 + 1 != keep) p->setP ("s" + juce::String (s2 + 1) + "_byp", 1.0f); }
            if (std::getenv ("JNOEQ")) p->setP ("eq_on", 0.0f);
            return p; };
        auto a = mk(); auto b = mk();
        auto o1 = testlab::render (*a, (int) in.size(), [&] (int k) { return in[(size_t) k]; }, 512, false);
        auto o2 = testlab::render (*b, (int) in.size(), [&] (int k) { return in[(size_t) k]; }, 512, true);
        double e = 0, d = 0; for (size_t i = (size_t) sr; i < in.size(); ++i) { e += (double) o1[i] * o1[i]; d += std::pow ((double) o1[i] - o2[i], 2.0); }
        std::printf ("irregolari 1..512 vs 512: %.1f dB\n", 10 * std::log10 (d / (e + 1e-30) + 1e-30));
        for (int sec = 0; sec < 4; ++sec) { double e2 = 0, d2 = 0; for (size_t i = (size_t) (sec * sr); i < (size_t) ((sec + 1) * sr); ++i) { e2 += (double) o1[i] * o1[i]; d2 += std::pow ((double) o1[i] - o2[i], 2.0); } std::printf ("  secondo %d: %.1f dB\n", sec, 10 * std::log10 (d2 / (e2 + 1e-30) + 1e-30)); }
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "findp")
    {   // trova il preset dai valori normalizzati del report
        const auto& F = factoryPresets();
        for (int pr = 0; pr < (int) F.size(); ++pr)
        {
            Processor p; p.prepareToPlay (48000, 512); p.loadPreset (pr);
            auto v = [&] (const char* id) { auto* r = p.apvts.getParameter (id); return r ? r->getValue() : -1.0f; };
            if (std::abs (v ("s1_p7") - 0.4771213f) < 1e-4f && std::abs (v ("s6_p2") - 0.3890756f) < 1e-4f)
            {
                std::printf ("%d %s | %s\n", pr, F[(size_t) pr].cat.toRawUTF8(), F[(size_t) pr].name.toRawUTF8());
                for (int s = 0; s < 8; ++s) std::printf ("  slot %d: %s\n", s + 1, mod (p.slotType (s)).name.toRawUTF8());
            }
        }
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "presets2")
    {   // ogni preset su un segnale musicale (rumore rosa a impulsi, -18 dBFS RMS): volume, picco, latenza, CPU
        const double sr = 44100; const auto& F = factoryPresets();
        juce::Random r (5); std::vector<float> in ((size_t) sr * 6); float b0 = 0, b1 = 0, b2 = 0;
        for (size_t i = 0; i < in.size(); ++i) { const float w = r.nextFloat() * 2 - 1; b0 = 0.99765f * b0 + w * 0.099f; b1 = 0.963f * b1 + w * 0.2965f; b2 = 0.57f * b2 + w * 1.0527f; const float pink = (b0 + b1 + b2 + w * 0.1848f) * 0.05f;
            const double t = i / sr; const float env = 0.35f + 0.65f * (float) std::pow (std::max (0.0, std::sin (juce::MathConstants<double>::pi * t * 3.1)), 2.0); in[i] = pink * env; }
        { double e = 0; for (float v : in) e += v * v; const float g = juce::Decibels::decibelsToGain (-18.0f) / (float) std::sqrt (e / in.size()); for (float& v : in) v *= g; }
        for (int pr = 0; pr < (int) F.size(); ++pr)
        {
            Processor p; p.prepareToPlay (sr, 512); p.loadPreset (pr); pump (p);
            juce::AudioBuffer<float> b (4, 512); juce::MidiBuffer m; double ei = 0, eo = 0; float pk = 0; bool nan = false;
            const auto t0 = juce::Time::getMillisecondCounterHiRes();
            for (size_t o = 0; o + 512 <= in.size(); o += 512)
            {
                b.clear(); for (int i = 0; i < 512; ++i) { b.setSample (0, i, in[o + i]); b.setSample (1, i, in[o + i]); }
                juce::AudioBuffer<float> io (b.getArrayOfWritePointers(), 2, 512); p.processBlock (io, m);
                if (o > sr * 2) for (int i = 0; i < 512; ++i) { const float y = b.getSample (0, i); if (! std::isfinite (y)) nan = true; eo += y * y; ei += in[o + i] * in[o + i]; pk = std::max (pk, std::abs (y)); }
            }
            const double cpu = (juce::Time::getMillisecondCounterHiRes() - t0) / 60.0;
            const float lv = (float) (10 * std::log10 (eo / ei)); const int lat = p.getLatencySamples();
            const bool master = F[(size_t) pr].cat.contains ("MASTER");
            const bool bad = nan || (! master && std::abs (lv) > 3.0f) || lat > 1000 || pk > 1.0f;
            std::printf ("%s %-9s %-34s %+5.1f dB  picco %5.2f  lat %4d  CPU %4.1f%%\n", bad ? "!!" : "  ", F[(size_t) pr].cat.toRawUTF8(), F[(size_t) pr].name.toRawUTF8(), lv, pk, lat, cpu);
        }
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "rt")
    {   // riverberi: tempo di decadimento misurato (T30 x 2, curva di Schroeder) contro il DECAY dichiarato, e livello della coda
        const double sr = 44100;
        for (int t = 1; t < (int) mods().size(); ++t)
        {
            if (mod (t).cat != C_REV && ! (std::getenv ("JDLY") && mod (t).cat == C_DLY)) continue;
            Processor p; p.prepareToPlay (sr, 512); p.insertModule (0, t); p.setP ("s1_mix", 1.0f);
            if (auto* js = std::getenv ("JSET")) for (auto kv : juce::StringArray::fromTokens (js, ",", "")) p.setP (kv.upToFirstOccurrenceOf ("=", false, false), kv.fromFirstOccurrenceOf ("=", false, false).getFloatValue());
            pump (p);
            juce::AudioBuffer<float> b (2, 512); juce::MidiBuffer m; std::vector<float> o;
            for (int k = 0; k < (int) (sr * 14 / 512); ++k) { b.clear(); if (k == 2) { b.setSample (0, 0, 0.5f); b.setSample (1, 0, 0.5f); } p.processBlock (b, m); for (int i = 0; i < 512; ++i) o.push_back (0.5f * (b.getSample (0, i) + b.getSample (1, i))); }
            { juce::String lv; for (int sec = 0; sec < 13; ++sec) { double e = 0; for (int i = (int) (sec * sr); i < (int) ((sec + 1) * sr) && i < (int) o.size(); ++i) e += (double) o[i] * o[i]; lv << juce::String (10 * std::log10 (e / sr + 1e-30), 0) << " "; } std::printf ("  livello per secondo: %s\n", lv.toRawUTF8()); }
            std::vector<double> sch (o.size()); double acc = 0; for (size_t i = o.size(); i-- > 0;) { acc += (double) o[i] * o[i]; sch[i] = acc; }
            const double e0 = sch[0]; double t5 = -1, t35 = -1;
            for (size_t i = 0; i < sch.size(); ++i) { const double db = 10 * std::log10 (sch[i] / e0 + 1e-30); if (t5 < 0 && db < -5) t5 = i / sr; if (t35 < 0 && db < -35) { t35 = i / sr; break; } }
            // decadimento a bande: 250 Hz, 1 kHz, 4 kHz tramite energia in finestre
            const float dec = juce::String (mod (t).spec[0].label) == "PRE-DELAY" ? p.getP ("s1_p2") : p.getP ("s1_p1");
            std::printf ("%-12s DECAY dichiarato %.2f s  misurato RT60 %.2f s  energia coda %.1f dB\n", mod (t).name.toRawUTF8(), dec / 1000.0, t35 > 0 && t5 > 0 ? (t35 - t5) * 2 : -1.0, 10 * std::log10 (e0 / 0.25 + 1e-30));
        }
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "audit")
    {   // ogni modulo a valori di default: livello, risposta, distorsione, aliasing, latenza
        const double sr = 44100; juce::dsp::FFT fft (14);
        auto render = [&] (int t, double f, float amp, int blocks, std::vector<float>& o, int& lat) {
            Processor p; p.prepareToPlay (sr, 512); p.insertModule (0, t);
            if (auto* js = std::getenv ("JSET")) for (auto kv : juce::StringArray::fromTokens (js, ",", "")) p.setP (kv.upToFirstOccurrenceOf ("=", false, false), kv.fromFirstOccurrenceOf ("=", false, false).getFloatValue());
            pump (p); double ph = 0; feed (p, blocks, amp, f, ph, &o); lat = p.getLatencySamples(); };
        auto harm = [&] (const std::vector<float>& o, double f, int k) { const int N = 16384; const size_t st = o.size() - N; double re = 0, im = 0; for (int i = 0; i < N; ++i) { const double w = 0.5 - 0.5 * std::cos (6.283185307 * i / (N - 1)); re += o[st + i] * w * std::cos (6.283185307 * f * k * i / sr); im += o[st + i] * w * std::sin (6.283185307 * f * k * i / sr); } return std::sqrt (re * re + im * im) / (N * 0.25); };
        for (int t = 1; t < (int) mods().size(); ++t)
        {
            if (auto* o = std::getenv ("JMOD")) if (! mod (t).name.containsIgnoreCase (o)) continue;
            int lat = 0; juce::String line;
            // risposta a -24 dBFS (lineare): rispetto all'ingresso
            const double fr[] = { 30, 100, 300, 1000, 3000, 6000, 10000, 15000 };
            for (double f : fr) { std::vector<float> o; render (t, f, 0.063f, 70, o, lat); line << juce::String (juce::Decibels::gainToDecibels (harm (o, f, 1) / 0.063, -99.0), 1) << " "; }
            // distorsione a -12 dBFS, 100 Hz e 1 kHz
            juce::String th;
            for (double f : { 100.0, 1000.0 }) { std::vector<float> o; render (t, f, 0.25f, 70, o, lat); const double h1 = harm (o, f, 1); double e = 0; for (int k = 2; k <= 7; ++k) { const double h = harm (o, f, k); e += h * h; } th << juce::String (100 * std::sqrt (e) / (h1 + 1e-12), 2) << "% "; }
            // aliasing a -6 dBFS 6.3 kHz: picco non armonico
            double worst = 0, wf = 0;
            { std::vector<float> o; render (t, 6301, 0.5f, 70, o, lat); const int N = 16384; std::vector<float> w (2 * N, 0.0f); for (int i = 0; i < N; ++i) w[(size_t) i] = o[o.size() - N + i] * (float) (0.5 - 0.5 * std::cos (6.283185307 * i / (N - 1))); fft.performFrequencyOnlyForwardTransform (w.data());
              float ref = 0; for (int k = 1; k < N / 2; ++k) { const double fq = k * sr / N; if (std::abs (fq - 6301) < 30) ref = std::max (ref, w[(size_t) k]); }
              for (int k = 20; k < N / 2; ++k) { const double fq = k * sr / N, hn = fq / 6301; if (std::abs (hn - std::round (hn)) * 6301 < 40) continue; if (w[(size_t) k] > worst) { worst = w[(size_t) k]; wf = fq; } } worst /= (ref + 1e-12); }
            std::printf ("%-14s risp[30..15k] %s | THD100/1k %s| alias %.0f dB@%.0f | lat %d\n", mod (t).name.toRawUTF8(), line.toRawUTF8(), th.toRawUTF8(), 20 * std::log10 (worst + 1e-9), wf, lat);
            std::fflush (stdout);
        }
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "mods")
    {   // elenco moduli
        for (int t = 1; t < (int) mods().size(); ++t) std::printf ("%d %s | %s\n", t, mod (t).name.toRawUTF8(), catName (mod (t).cat));
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "lab")
    {   // un modulo da solo (JMOD) o un preset (JPRESET), come THE_LAB
        const int mt = juce::SystemStats::getEnvironmentVariable ("JMOD", "0").getIntValue(), pr = juce::SystemStats::getEnvironmentVariable ("JPRESET", "0").getIntValue();
        { Processor q; std::printf ("%s\n", mt > 0 ? (juce::String ("modulo: ") + mod (mt).name + " (" + mod (mt).inspired + ")").toRawUTF8() : (juce::String ("preset: ") + q.getProgramName (pr)).toRawUTF8()); }
        testlab::run ([&] (double rate, int blk) -> std::unique_ptr<juce::AudioProcessor>
        {
            auto p = std::make_unique<Processor>(); p->setPlayConfigDetails (2, 2, rate, blk); p->prepareToPlay (rate, blk);
            if (mt > 0) { p->insertModule (0, mt); } else p->loadPreset (pr);
            if (auto* js = std::getenv ("JSET")) for (auto kv : juce::StringArray::fromTokens (js, ",", "")) { auto* r = p->apvts.getParameter (kv.upToFirstOccurrenceOf ("=", false, false)); r->setValueNotifyingHost (r->convertTo0to1 (kv.fromFirstOccurrenceOf ("=", false, false).getFloatValue())); }
            p->forceRebuild();
            return p;
        }, std::getenv ("JNOCLICK") == nullptr, [mt] (juce::AudioProcessorParameter* q)
        {
            auto* r = dynamic_cast<juce::RangedAudioParameter*> (q); if (r == nullptr) return true;
            const auto id = r->paramID;
            if (auto* only = std::getenv ("JONLY")) return ! id.contains (only);
            if (id == "n" || id == "eq_pos" || id == "target" || id == "live" || id.endsWith ("_type")) return true;
            if (mt > 0) return ! (id.startsWith ("s1_") || id == "in" || id == "out" || id == "cmp");
            return id.startsWith ("e") && id.contains ("_");   // nei preset: niente bande EQ (si provano a parte)
        });
        return 0;
    }
    if (argc > 2 && juce::String (argv[1]) == "snap")
    {
        Processor p; p.prepareToPlay (44100, 512);
        p.loadPreset (juce::SystemStats::getEnvironmentVariable ("JPRESET", "0").getIntValue()); pump (p);
        double ph = 0; juce::Random r;
        if (auto* th = std::getenv ("JTHEME")) skin::cur() = std::atoi (th);
        Editor ed (p); ed.setSize (Editor::W, Editor::H);
        if (auto* e = std::getenv ("JBAND")) { ed.graph.sel = atoi (e); ed.graph.updatePop(); }
        for (int i = 0; i < 40; ++i) { feed (p, 4, 0.3f, 220 + i * 13, ph); ed.timerCallback(); }
        if (auto* s = std::getenv ("JSOLO")) p.soloSlot = std::atoi (s);
        if (auto* s = std::getenv ("JDELTA")) p.deltaSlot = std::atoi (s);
        if (std::getenv ("JBROWSE")) ed.browser.open();
        if (std::getenv ("JEQ")) { ed.openEq (true); for (int i = 0; i < 20; ++i) { feed (p, 4, 0.3f, 300, ph); ed.timerCallback(); } }
        if (std::getenv ("JMODS")) ed.openModuleChooser (std::atoi (std::getenv ("JMODS")));
        if (std::getenv ("JMODP")) ed.openModulePresets (std::atoi (std::getenv ("JMODP")));
        if (std::getenv ("JSCROLL")) { ed.rackView.scrollTo ((float) std::atoi (std::getenv ("JSCROLL")), false); }
        auto img = ed.createComponentSnapshot (ed.getLocalBounds(), true, 1.0f);
        if (std::getenv ("JPERF"))
        {
            auto tm = [] (juce::Component& c, int n) { const auto t0 = juce::Time::getMillisecondCounterHiRes(); for (int i = 0; i < n; ++i) c.createComponentSnapshot (c.getLocalBounds(), true, 2.0f); return (juce::Time::getMillisecondCounterHiRes() - t0) / n; };
            std::printf ("intero %.2f ms, rack %.2f ms, EQ %.2f ms, loudness %.2f ms, imager %.2f ms\n", tm (ed, 5), tm (ed.rack, 10), tm (ed.graph, 20), tm (ed.loud, 20), tm (ed.imager, 20));
            if (ed.rack.slots.size() > 0) { auto& s0 = *ed.rack.slots[1]; double kn = 0; for (auto& c : s0.ctl) kn += tm (*c, 20); std::printf ("modulo 2 da solo %.2f ms, sue manopole %.2f ms\n", tm (s0, 20), kn); }
            {   // scorrimento del rack: ridisegno della sola zona visibile del rack dopo uno spostamento
                juce::Image im (juce::Image::RGB, Editor::W * 2, Editor::H * 2, true); const auto vb = ed.rackView.getBoundsInParent();
                const auto t0 = juce::Time::getMillisecondCounterHiRes();
                for (int i = 0; i < 20; ++i) { ed.rackView.setViewPosition ((i % 2) * 40, 0); juce::Graphics g (im); g.addTransform (juce::AffineTransform::scale (2.0f)); g.reduceClipRegion (vb); ed.paintEntireComponent (g, true); }
                if (std::getenv ("JHIDE")) { for (auto* sv : ed.rack.slots) sv->setVisible (false); const auto t9 = juce::Time::getMillisecondCounterHiRes(); for (int i = 0; i < 20; ++i) { ed.rackView.setViewPosition ((i % 2) * 40, 0); juce::Graphics g (im); g.addTransform (juce::AffineTransform::scale (2.0f)); g.reduceClipRegion (vb); ed.paintEntireComponent (g, true); } std::printf ("senza moduli %.2f ms\n", (juce::Time::getMillisecondCounterHiRes() - t9) / 20); for (auto* sv : ed.rack.slots) sv->setVisible (true); }
                std::printf ("cache %d  ", ed.rack.slots.size() ? (int) (ed.rack.slots[0]->getCachedComponentImage() != nullptr) : -1); std::printf ("scorrimento rack %.2f ms a fotogramma\n", (juce::Time::getMillisecondCounterHiRes() - t0) / 20);
            }
            const auto t0 = juce::Time::getMillisecondCounterHiRes(); for (int i = 0; i < 30; ++i) ed.timerCallback(); std::printf ("30 timer %.2f ms\n", juce::Time::getMillisecondCounterHiRes() - t0);
        }
        juce::File f (argv[2]); f.deleteFile(); juce::FileOutputStream os (f); juce::PNGImageFormat().writeImageToStream (img, os);
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "wet")
    {   // livello dell'effetto (send) rispetto al dry, con MIX al 100% e al default
        for (int t = 1; t < (int) mods().size(); ++t)
        {
            if (! mod (t).send) continue;
            for (float mx : { 1.0f, mod (t).defMix })
            {
                Processor p; p.prepareToPlay (44100, 512); p.insertModule (0, t); p.setP ("s1_mix", mx); pump (p);
                juce::Random r (3); juce::AudioBuffer<float> b (2, 512); juce::MidiBuffer m; double ei = 0, ew = 0;
                for (int k = 0; k < 400; ++k)
                {
                    float in[512]; for (int i = 0; i < 512; ++i) { in[i] = (r.nextFloat() - 0.5f) * 0.2f; b.setSample (0, i, in[i]); b.setSample (1, i, in[i]); }
                    p.processBlock (b, m);
                    if (k > 150) for (int i = 0; i < 512; ++i) { const float w = b.getSample (0, i) - in[i]; ei += in[i] * in[i]; ew += w * w; }
                }
                std::printf ("%-14s mix %3.0f%%: effetto %+6.1f dB rispetto al dry\n", mod (t).name.toRawUTF8(), mx * 100, juce::Decibels::gainToDecibels (std::sqrt (ew / ei)));
            }
        }
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "sat")
    {   // saturatori: livello d'uscita e armoniche al variare del DRIVE (sinusoide 110 Hz e 1 kHz a -18 dBFS)
        for (int t = 1; t < (int) mods().size(); ++t)
        {
            if (mod (t).cat != C_SAT && ! (std::getenv ("JALL") && mod (t).cat == C_DIST)) continue;
            int di = -1; for (int i = 0; i < kP; ++i) { juce::String l (mod (t).spec[(size_t) i].label); if (di < 0 && (l == "DRIVE" || l == "BASSI")) di = i; }
            for (double f : { 110.0, 1000.0 })
            {
                juce::String line;
                for (float d : { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f })
                {
                    Processor p; p.prepareToPlay (44100, 512); p.insertModule (0, t); p.setP ("eq_on", 0);
                    if (mod (t).kind == K_MBSAT) { p.setP ("s1_p1", d); p.setP ("s1_p2", d); p.setP ("s1_p3", d); } else if (di >= 0) p.setP ("s1_p" + juce::String (di + 1), d);
                    pump (p); double ph = 0; std::vector<float> o; feed (p, 80, 0.177f, f, ph, &o);
                    // fondamentale con i minimi quadrati, il resto sono armoniche e rumore
                    const size_t a = 20000, n = o.size() - a; double sc = 0, cc = 0, ss = 0, cs = 0, sx = 0, cx = 0;
                    for (size_t i = 0; i < n; ++i) { const double w = 2 * juce::MathConstants<double>::pi * f * (double) (a + i) / 44100.0, sn = std::sin (w), cn = std::cos (w); ss += sn * sn; cc += cn * cn; cs += sn * cn; sx += sn * o[a + i]; cx += cn * o[a + i]; }
                    const double det = ss * cc - cs * cs, A = (sx * cc - cx * cs) / det, B = (cx * ss - sx * cs) / det; double er = 0, et = 0;
                    for (size_t i = 0; i < n; ++i) { const double w = 2 * juce::MathConstants<double>::pi * f * (double) (a + i) / 44100.0; const double r = o[a + i] - A * std::sin (w) - B * std::cos (w); er += r * r; et += (double) o[a + i] * o[a + i]; }
                    line << juce::String (juce::roundToInt (d * 100)) << "%: " << juce::String (rmsDb (o, a) + 18.0f, 1) << "dB thd " << juce::String (100.0 * std::sqrt (er / juce::jmax (1e-12, et)), 1) << "%  ";
                }
                std::printf ("%-12s %5.0f Hz  %s\n", mod (t).name.toRawUTF8(), f, line.toRawUTF8());
            }
        }
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "ds")
    {   // de-esser: quanto toglie sulle sibilanti (rumore sopra 6 kHz) e quanto lascia sotto (sinusoide 300 Hz)
        for (const char* nm : { "DS 902", "PRO DS" })
            for (float rg : { 10.0f, 20.0f, 30.0f })
            {
                const int t = findMod (nm); Processor p; p.prepareToPlay (44100, 512); p.insertModule (0, t); p.setP ("eq_on", 0);
                const auto& d = mod (t); for (int i = 0; i < kP; ++i) { juce::String l (d.spec[(size_t) i].label); if (l == "RANGE") p.setP ("s1_p" + juce::String (i + 1), specNorm (d.spec[(size_t) i], rg)); if (l == "THRESH" || l == "SOGLIA") p.setP ("s1_p" + juce::String (i + 1), specNorm (d.spec[(size_t) i], d.spec[(size_t) i].lo)); }
                pump (p); juce::Random r (5); jkit::Bq h1, h2; h1.hp (44100, 6500); h2.hp (44100, 6500);
                juce::AudioBuffer<float> b (2, 512); juce::MidiBuffer mm; double eHi = 0, eHiIn = 0, eLo = 0, eLoIn = 0; double ph = 0; jkit::Bq a1, a2, z1, z2; a1.hp (44100, 6000); a2.hp (44100, 6000); z1.lp (44100, 1000); z2.lp (44100, 1000);
                for (int k = 0; k < 300; ++k)
                {
                    float in[512]; for (int i = 0; i < 512; ++i) { const float sib = h2.p (h1.p (r.nextFloat() - 0.5f)) * 0.5f; const float lo = (float) std::sin (ph) * 0.2f; ph += 2 * juce::MathConstants<double>::pi * 300 / 44100.0; in[i] = lo + sib; b.setSample (0, i, in[i]); b.setSample (1, i, in[i]); }
                    p.processBlock (b, mm);
                    if (k > 100) for (int i = 0; i < 512; ++i) { const float o = b.getSample (0, i); const float oh = a2.p (a1.p (o)), ol = z2.p (z1.p (o)); eHi += oh * oh; eLo += ol * ol; }
                }
                r = juce::Random (5); h1.reset(); h2.reset(); ph = 0; a1.reset(); a2.reset(); z1.reset(); z2.reset();
                for (int k = 0; k < 300; ++k) for (int i = 0; i < 512; ++i) { const float sib = h2.p (h1.p (r.nextFloat() - 0.5f)) * 0.5f; const float lo = (float) std::sin (ph) * 0.2f; ph += 2 * juce::MathConstants<double>::pi * 300 / 44100.0; const float x = lo + sib; const float oh = a2.p (a1.p (x)), ol = z2.p (z1.p (x)); if (k > 100) { eHiIn += oh * oh; eLoIn += ol * ol; } }
                std::printf ("%-7s RANGE %2.0f: sibilanti %+6.1f dB, sotto %+5.2f dB\n", nm, rg, 10 * std::log10 (eHi / eHiIn), 10 * std::log10 (eLo / eLoIn));
            }
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "eqmax")
    {   // ogni EQ analogico: tutte le manopole di gain al massimo, poi al minimo
        for (int t = 1; t < (int) mods().size(); ++t)
        {
            if (mod (t).cat != C_EQ) continue;
            for (int dir : { 1, -1 })
            {
                juce::String line;
                for (double f : { 60.0, 200.0, 1000.0, 3000.0, 10000.0 })
                {
                    Processor p; p.prepareToPlay (44100, 512); p.insertModule (0, t);
                    for (int i = 0; i < kP; ++i) { const auto& sp = mod (t).spec[(size_t) i]; if (hidden (sp)) continue; juce::String l (sp.label);
                        if (sp.fmt == F_DB || l == "BOOST" || l == "PEAK" || l == "ATTEN" || l == "DIP") { const bool cut = l == "ATTEN" || l == "DIP"; const float v = sp.fmt == F_DB ? (dir > 0 ? sp.hi : sp.lo) : ((dir > 0) != cut ? sp.hi : sp.lo); p.setP ("s1_p" + juce::String (i + 1), v); } }
                    pump (p); double ph = 0; std::vector<float> o; feed (p, 60, 0.05f, f, ph, &o);
                    line << juce::String (f, 0) << "Hz " << juce::String (rmsDb (o, 15000) - juce::Decibels::gainToDecibels (0.05f / std::sqrt (2.0f)), 1) << "  ";
                }
                std::printf ("%-12s %s %s\n", mod (t).name.toRawUTF8(), dir > 0 ? "MAX" : "MIN", line.toRawUTF8());
            }
        }
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "pultec")
    {
        const int t = findMod ("EQP-1A");
        for (int fsel = 0; fsel < 4; ++fsel)
        {
            juce::String line;
            for (double f : { 20.0, 30.0, 60.0, 100.0, 200.0, 500.0 })
            {
                Processor p; p.prepareToPlay (44100, 512); p.insertModule (0, t); p.setP ("s1_p1", (float) fsel); p.setP ("s1_p2", 10); pump (p);
                double ph = 0; std::vector<float> o; feed (p, 80, 0.05f, f, ph, &o);
                line << juce::String (f, 0) << ":" << juce::String (rmsDb (o, 20000) - juce::Decibels::gainToDecibels (0.05f / std::sqrt (2.0f)), 1) << " ";
            }
            std::printf ("LOW FREQ %d BOOST 10: %s\n", fsel, line.toRawUTF8());
        }
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "specs")
    {
        for (int t = 1; t < (int) mods().size(); ++t)
        {
            juce::String l; int n = 0; for (int i = 0; i < kP; ++i) if (! hidden (mod (t).spec[(size_t) i])) { l << i << ":" << mod (t).spec[(size_t) i].label << "(" << mod (t).spec[(size_t) i].fmt << ") "; ++n; }
            std::printf ("%-14s n=%d %s\n", mod (t).name.toRawUTF8(), n, l.toRawUTF8());
        }
        return 0;
    }
    // neutro
    { Processor p; p.prepareToPlay (44100, 512); pump (p); double ph = 0; std::vector<float> o; feed (p, 100, 0.3f, 1000, ph, &o);
      std::printf ("neutro (vuoto): %+.3f dB (atteso %+.3f) lat %d\n", rmsDb (o, 20000), juce::Decibels::gainToDecibels (0.3f / std::sqrt (2.0f)), p.getLatencySamples()); }
    // ogni modulo da solo
    for (int t = 1; t < (int) mods().size(); ++t)
    {
        Processor p; p.prepareToPlay (44100, 512); p.insertModule (0, t); pump (p);
        double ph = 0; std::vector<float> o; bool nan = false; feed (p, 120, 0.25f, 440, ph, &o, &nan);
        std::vector<float> s; feed (p, 200, 0, 0, ph, &s, &nan);
        std::printf ("%-16s %+7.2f dB  coda %+7.1f dB  lat %5d %s\n", mod (t).name.toRawUTF8(), rmsDb (o, 30000) - juce::Decibels::gainToDecibels (0.25f / std::sqrt (2.0f)), rmsDb (s, 80000), p.getLatencySamples(), nan ? "NAN!" : "");
    }
    // preset
    int bad = 0;
    for (int i = 0; i < (int) factoryPresets().size(); ++i)
    {
        Processor p; p.prepareToPlay (44100, 512); p.loadPreset (i); pump (p);
        double ph = 0; std::vector<float> o; bool nan = false; feed (p, 100, 0.2f, 330, ph, &o, &nan);
        float pk = 0; for (size_t k = 20000; k < o.size(); ++k) pk = juce::jmax (pk, std::abs (o[k]));
        const float d = rmsDb (o, 20000) - juce::Decibels::gainToDecibels (0.2f / std::sqrt (2.0f));
        const bool warn = nan || std::abs (d) > 9 || pk > 1.2f;
        if (warn) ++bad;
        std::printf ("%s%-10s %-26s n=%d %+6.2f dB pk %.2f lat %d\n", warn ? "!! " : "   ", factoryPresets()[(size_t) i].cat.toRawUTF8(), factoryPresets()[(size_t) i].name.toRawUTF8(), p.chainLen(), d, pk, p.getLatencySamples());
    }
    std::printf ("preset sospetti: %d / %d\n", bad, (int) factoryPresets().size());
    return 0;
}
