#include <cstdlib>
#include "../Source/PadProcessor.h"
#include "../Source/PadEditor.h"
#include "../Source/Library.h"
#include "../Source/KeyDetect.h"
#include "../Source/Generator.h"
#include <map>
#include <set>
using namespace jp;
int main (int argc, char** argv)
{
    if (argc > 1 && juce::String (argv[1]) == "extend")
    {
        Processor p; p.prepareToPlay (44100, 512); p.loadFactoryKit (1);
        generate (p, 0, 5, 0);
        { std::vector<PatEv> e2; for (auto& e : p.events) if (e.tick < 2 * kBar) e2.push_back (e); p.events = e2; p.patternBars = 2; }
        extendPattern (p, 8, 1234);
        for (int pd = 0; pd < 16; ++pd)
        {
            juce::String row; bool any = false;
            for (int b = 0; b < 8; ++b) { for (int s = 0; s < 16; ++s) { char c = '.'; for (auto& e : p.events) if (e.pad == pd && (int) (e.tick / (kBar / 16.0)) == b * 16 + s) c = e.vel > 90 ? 'X' : 'x'; row << c; if (c != '.') any = true; } row << "|"; }
            if (any) std::printf ("%2d %s\n", pd + 1, row.toRawUTF8());
        }
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "ctor")
    {
        { Processor p; for (int m = 0; m < 3; ++m) { const double t0 = juce::Time::getMillisecondCounterHiRes(); randomKit (p, m); std::printf ("randomKit modo %d: %.0f ms\n", m, juce::Time::getMillisecondCounterHiRes() - t0); }
          const double t1 = juce::Time::getMillisecondCounterHiRes(); p.loadFactoryKit (1); std::printf ("kit fabbrica: %.0f ms\n", juce::Time::getMillisecondCounterHiRes() - t1); }
        for (int k = 0; k < 3; ++k) { const double t0 = juce::Time::getMillisecondCounterHiRes(); Processor p; std::printf ("costruttore %d: %.0f ms  kit %s\n", k, juce::Time::getMillisecondCounterHiRes() - t0, p.kitName.toRawUTF8()); }
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "prog")
    {
        Processor p; p.prepareToPlay (44100, 512); p.loadFactoryKit (1);
        std::vector<int> ids; stylesInCat (GC_PROG, ids);
        for (int st : ids) for (juce::uint32 sd : { 3u, 4u, 5u }) std::printf ("%s\n", generate (p, st, sd, -1).toRawUTF8());
        stylesInCat (GC_PROGMPC, ids);
        for (int k = 0; k < 4; ++k) std::printf ("%s\n", generate (p, ids[(size_t) k * 5], 9, -1).toRawUTF8());
        // suono di un pad accordo
        juce::AudioBuffer<float> b (2, 512); juce::MidiBuffer m; double e = 0;
        m.addEvent (juce::MidiMessage::noteOn (1, (int) p.rootNote() + 13, (juce::uint8) 100), 0);
        for (int k = 0; k < 40; ++k) { b.clear(); p.processBlock (b, m); m.clear(); e += b.getRMSLevel (0, 0, 512); }
        std::printf ("pad accordo (%d note): rms medio %.4f\n", p.chordN[13].load(), e / 40);
        // 4 -> 2 -> 8 battute
        generate (p, 0, 11, -1); std::printf ("battute %d\n", p.patternBars.load());
        extendPattern (p, 4, 1); extendPattern (p, 2, 1); extendPattern (p, 8, 2);
        int perBar[8] {}; for (auto& ev : p.events) perBar[juce::jlimit (0, 7, (int) (ev.tick / kBar))]++;
        std::printf ("dopo 4->2->8: note per battuta"); for (int k = 0; k < 8; ++k) std::printf (" %d", perBar[k]); std::printf ("\n");
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "perf")
    {
        Processor p; p.prepareToPlay (44100, 512); p.loadFactoryKit (1);
        Editor ed (p); ed.setSize (Editor::W, Editor::H);
        juce::Image im (juce::Image::ARGB, Editor::W * 2, Editor::H * 2, true);
        auto t0 = juce::Time::getMillisecondCounterHiRes();
        for (int k = 0; k < 60; ++k) { juce::Graphics g (im); g.addTransform (juce::AffineTransform::scale (2.0f)); g.reduceClipRegion (1056, 686, 158, 22); ed.paintEntireComponent (g, false); }
        auto t1 = juce::Time::getMillisecondCounterHiRes();
        for (int k = 0; k < 10; ++k) { juce::Graphics g (im); g.addTransform (juce::AffineTransform::scale (2.0f)); ed.paintEntireComponent (g, false); }
        auto t2 = juce::Time::getMillisecondCounterHiRes();
        std::printf ("ridisegno del meter: %.2f ms, editor intero: %.1f ms\n", (t1 - t0) / 60, (t2 - t1) / 10);
        std::function<void (juce::Component&, int)> walk = [&] (juce::Component& c, int depth)
        {
            if ((depth > 0 && ! c.isVisible()) || depth > 6) return;
            juce::Image ci (juce::Image::ARGB, juce::jmax (1, c.getWidth() * 2), juce::jmax (1, c.getHeight() * 2), true);
            auto a = juce::Time::getMillisecondCounterHiRes();
            for (int k = 0; k < 5; ++k) { juce::Graphics g (ci); g.addTransform (juce::AffineTransform::scale (2.0f)); c.paint (g); }
            const double ms = (juce::Time::getMillisecondCounterHiRes() - a) / 5;
            if (ms > 0.3) std::printf ("%*s%s %s %dx%d: %.1f ms\n", depth * 2, "", typeid (c).name(), c.getName().toRawUTF8(), c.getWidth(), c.getHeight(), ms);
            for (auto* ch : c.getChildren()) walk (*ch, depth + 1);
        };
        walk (ed, 0);
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "allgen")
    {
        Processor p; p.prepareToPlay (44100, 512); p.loadFactoryKit (1);
        std::vector<int> ids; stylesInCat (GC_DRUM, ids);
        for (int st : ids) { int ev = 0; for (juce::uint32 sd : { 1u, 7u, 99u }) { generate (p, st, sd, 0); ev += (int) p.events.size(); }
            std::printf ("%-18s eventi medi %d\n", genStyles()[(size_t) st].name.toRawUTF8(), ev / 3); }
        auto f = p.writeMidi (false); juce::MidiFile mf; juce::FileInputStream is (f); mf.readFrom (is);
        std::printf ("midi: %d eventi\n", mf.getTrack (0)->getNumEvents());
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "rnd")
    {
        Processor p; p.prepareToPlay (44100, 512);
        int drumsHigh = 0, longHigh = 0, tot = 0;
        auto isDrum = [] (const juce::String& c) { return c == "KICK" || c == "SNARE" || c == "CLAP / SNAP" || c == "HAT" || c == "OPEN HAT" || c == "PERC" || c == "TOM"; };
        for (int k = 0; k < 40; ++k)
        {
            randomKit (p, 0);
            for (int q = 8; q < 16; ++q) if (auto s = p.getSample (q)) { ++tot; if (isDrum (p.padCat[q])) ++drumsHigh; if (s->buf.getNumSamples() / s->sr >= 1.5) ++longHigh; }
        }
        std::printf ("kit casuali: pad 9-16 riempiti %d, colpi di batteria %d, lunghi %d\n", tot, drumsHigh, longHigh);
        int same = 0, ok = 0;
        for (int k = 0; k < 40; ++k) { const int q = k % 8; const auto c = p.padCat[q]; randomPad (p, q, 0); if (p.padCat[q] == c) ++same; ++ok; }
        std::printf ("random pad 1-8: stessa categoria %d / %d\n", same, ok);
        int lg = 0, dr = 0; for (int k = 0; k < 40; ++k) { const int q = 8 + k % 8; randomPad (p, q, 0); auto s = p.getSample (q); if (s && s->buf.getNumSamples() / s->sr >= 1.5) ++lg; if (isDrum (p.padCat[q])) ++dr; }
        std::printf ("random pad 9-16: lunghi %d / 40, batteria %d\n", lg, dr);
        auto* pr = p.apvts.getParameter ("kkey"); pr->setValueNotifyingHost (pr->convertTo0to1 (1 + 12 + 9));   // La minore
        p.applyKeyLockAll(); int tonal = 0, inKey = 0;
        for (int q = 0; q < 16; ++q) if (p.getSample (q) && p.isTonalPad (q) && p.pad[q].get (SKEY) >= 0)
        { ++tonal; const int k = juce::roundToInt (p.pad[q].get (SKEY)), t = juce::roundToInt (p.pad[q].get (TUNE)); const int pc = (((k % 12) + t) % 12 + 12) % 12;
          const bool good = k >= 24 ? pc == 9 : (k >= 12 ? pc == 9 : pc == 0); if (good) ++inKey;
          std::printf ("  pad %2d %-10s %-4s tune %+d -> %s\n", q + 1, p.padCat[q].toRawUTF8(), keyName (k).toRawUTF8(), t, good ? "ok" : "NO"); }
        std::printf ("tonalita fissa Am: %d / %d parti tonali in tonalita\n", inKey, tonal);
        randomKit (p, 0); std::printf ("kit dopo blocco: %s\n", p.kitName.toRawUTF8());
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "scr")
    {   // SCRATCH da CC74 (avanti e indietro), poi mano ferma: deve tornare al suono normale. Rigenera una riga.
        juce::ScopedJuceInitialiser_GUI gi;
        auto go = [] (bool scratch, std::vector<float>& out)
        {
            Processor p; p.prepareToPlay (48000, 512); p.loadFactoryKit (1);
            juce::AudioBuffer<float> blk (2, 512);
            for (int b = 0; b < 400; ++b)
            {
                juce::MidiBuffer m; if (b % 12 == 0) m.addEvent (juce::MidiMessage::noteOn (1, p.rootNote() + (b / 12) % 4, (juce::uint8) 110), 0);
                if (scratch && b >= 100 && b < 160) { const int v = 64 + (int) (40 * std::sin ((b - 100) * 0.4)); m.addEvent (juce::MidiMessage::controllerEvent (1, 74, v), 0); }
                p.processBlock (blk, m); for (int i = 0; i < 512; ++i) out.push_back (blk.getSample (0, i));
            }
        };
        std::vector<float> a, b; go (false, a); go (true, b);
        auto diff = [&] (int b0, int b1) { double d = 0, e = 0; for (int i = b0 * 512; i < b1 * 512; ++i) { d += (a[i] - b[i]) * (a[i] - b[i]); e += a[i] * a[i]; } return 10 * std::log10 (d / (e + 1e-12) + 1e-20); };
        bool nan = false; for (float x : b) if (! std::isfinite (x)) nan = true;
        std::printf ("scratch: durante %.1f dB di differenza, dopo (mano ferma) %.1f dB, nan %d\n", diff (100, 160), diff (250, 400), (int) nan);
        Processor p; p.prepareToPlay (48000, 512); p.loadFactoryKit (1); jp::generate (p, 0, 5, 0);
        int before = 0, other = 0; for (auto& e : p.events) { if (e.pad == 2) ++before; else ++other; }
        const auto msg = jp::regenRow (p, 0, 99, 2);
        int after = 0, other2 = 0; for (auto& e : p.events) { if (e.pad == 2) ++after; else ++other2; }
        std::printf ("rigenera riga 3: colpi %d -> %d, altre righe %d -> %d  (%s)\n", before, after, other, other2, msg.toRawUTF8());
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "sat")
    {   // SATURA: quanto cambia davvero il suono (differenza, crest factor, armoniche su un sinusoide)
        juce::ScopedJuceInitialiser_GUI gi;
        auto run = [] (float sat, std::vector<float>& out)
        {
            Processor p; p.prepareToPlay (48000, 512); p.loadFactoryKit (1);
            auto* q = p.apvts.getParameter ("satAmt"); q->setValueNotifyingHost (sat);
            juce::AudioBuffer<float> blk (2, 512);
            for (int b = 0; b < 400; ++b) { juce::MidiBuffer m; if (b % 12 == 0) m.addEvent (juce::MidiMessage::noteOn (1, p.rootNote() + (b / 12) % 4, (juce::uint8) 110), 0); p.processBlock (blk, m); for (int i = 0; i < 512; ++i) out.push_back (blk.getSample (0, i)); }
        };
        std::vector<float> a0; run (0, a0);
        for (float sv : { 0.25f, 0.5f, 0.75f, 1.0f })
        {
            std::vector<float> a; run (sv, a);
            double e0 = 0, e = 0, d = 0; float p0 = 0, p1 = 0;
            for (size_t i = 0; i < a.size(); ++i) { e0 += a0[i] * a0[i]; e += a[i] * a[i]; d += (a[i] - a0[i]) * (a[i] - a0[i]); p0 = std::max (p0, std::abs (a0[i])); p1 = std::max (p1, std::abs (a[i])); }
            const double r0 = std::sqrt (e0 / a.size()), r1 = std::sqrt (e / a.size());
            std::printf ("SATURA %.2f: livello %+.1f dB  differenza %.1f dB  crest %.1f -> %.1f dB\n", sv, 20 * std::log10 (r1 / r0), 10 * std::log10 (d / e0), 20 * std::log10 (p0 / r0), 20 * std::log10 (p1 / r1));
        }
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "fx")
    {   // effetti da performance: neutri a zero, nessun NaN, il CC 70-77 arriva ai parametri
        juce::ScopedJuceInitialiser_GUI gi;
        auto run = [] (std::function<void (Processor&)> set, bool cc) -> std::pair<float, bool>
        {
            Processor p; p.prepareToPlay (48000, 512); p.loadFactoryKit (1); set (p);
            juce::AudioBuffer<float> blk (2, 512); float e = 0; bool nan = false;
            for (int b = 0; b < 400; ++b)
            {
                juce::MidiBuffer m; if (b % 12 == 0) m.addEvent (juce::MidiMessage::noteOn (1, p.rootNote() + (b / 12) % 4, (juce::uint8) 110), 0);
                if (cc && b == 50) m.addEvent (juce::MidiMessage::controllerEvent (1, 75, 127), 0);
                p.processBlock (blk, m);
                for (int i = 0; i < 512; ++i) { const float x = blk.getSample (0, i); if (! std::isfinite (x)) nan = true; e += x * x; }
            }
            return { std::sqrt (e / (400 * 512)), nan };
        };
        auto setp = [] (Processor& p, const char* id, float v) { auto* q = p.apvts.getParameter (id); q->setValueNotifyingHost (q->convertTo0to1 (v)); };
        const auto ref = run ([] (Processor&) {}, false);
        std::printf ("neutro rms %.4f nan %d\n", ref.first, (int) ref.second);
        for (int t = 1; t < pfx::kNumTypes; ++t)
        {
            const float v = pfx::info (t).bipolar ? 0.85f : 0.7f;
            const auto r = run ([&] (Processor& p) { setp (p, "fx1type", (float) t); setp (p, "fx1", v); }, false);
            std::printf ("%-10s %.2f  rms %.4f (%+.1f dB)  nan %d\n", pfx::info (t).name, v, r.first, 20 * std::log10 (r.first / ref.first + 1e-9f), (int) r.second);
        }
        {
            Processor p; p.prepareToPlay (48000, 512); p.learnSlot = 5;
            juce::AudioBuffer<float> blk (2, 512); juce::MidiBuffer m; m.addEvent (juce::MidiMessage::controllerEvent (1, 21, 100), 0); p.processBlock (blk, m);
            std::printf ("MIDI learn: CC21 -> slot 5: %d, valore in arrivo %.2f\n", p.ccMap[5].load(), p.ccPend[5].load());
        }
        const auto rc = run ([] (Processor&) {}, true);
        std::printf ("CC 75 (tape stop) dal controller: rms %.4f (%+.1f dB)\n", rc.first, 20 * std::log10 (rc.first / ref.first + 1e-9f));
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "kick")
    {   // kick su C1 mentre un campione lungo suona su C2: il kick deve sempre suonare
        juce::ScopedJuceInitialiser_GUI gi;
        Processor p; p.prepareToPlay (48000, 512); p.loadFactoryKit (1);
        const double sr = 48000; auto s = std::make_shared<SampleData>(); s->sr = sr; s->buf.setSize (2, (int) (sr * 8)); s->name = "lungo";
        for (int i = 0; i < s->buf.getNumSamples(); ++i) { const float v = 0.4f * (float) std::sin (i * 0.02); s->buf.setSample (0, i, v); s->buf.setSample (1, i, v); }
        const int root = p.rootNote();
        struct Case { const char* n; float mode, loop, choke12, choke0; };
        Case cs[] = { { "one shot", 0, 0, 0, 0 }, { "gate", 1, 0, 0, 0 }, { "loop", 0, 1, 0, 0 }, { "choke 7 sul lungo", 0, 0, 7, 0 }, { "stesso choke", 0, 0, 1, 1 } };
        for (auto& c : cs)
            for (int offMs : { 0, 3, 15, 40, 200 })
            {
                p.loadFactoryKit (1); p.setSample (12, s); p.pad[12].v[MODE] = c.mode; p.pad[12].v[LOOP] = c.loop; p.pad[12].v[CHOKE] = c.choke12; p.pad[12].v[DECAY] = 4000; p.pad[0].v[CHOKE] = c.choke0;
                juce::AudioBuffer<float> blk (2, 512); juce::MidiBuffer m;
                for (int b = 0; b < 20; ++b) { m.clear(); p.processBlock (blk, m); }
                m.clear(); m.addEvent (juce::MidiMessage::noteOn (1, root + 12, (juce::uint8) 100), 10);
                const int ko = 10 + (int) (offMs * 48);
                int kb = ko / 512; const int ks = ko % 512;
                float kpk = 0;
                for (int b = 0; b < 40; ++b)
                {
                    if (b == kb) m.addEvent (juce::MidiMessage::noteOn (1, root, (juce::uint8) 110), ks);
                    if (b == kb + 3) m.addEvent (juce::MidiMessage::noteOff (1, root), 0);
                    p.padGlow[0] = 0; p.processBlock (blk, m); m.clear();
                    if (b >= kb) kpk = std::max (kpk, p.padGlow[0].load());
                }
                std::printf ("%-18s kick dopo %3d ms: picco kick %.3f %s\n", c.n, offMs, kpk, kpk > 0.05f ? "ok" : "<<< MUTO");
            }
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "t2")
    {
        juce::ScopedJuceInitialiser_GUI gi;
        const char* kn[][2] = { { "Loop Dark 140 Bm", "Bm" }, { "piano_F#min_90bpm", "F#m" }, { "Vox C Minor", "Cm" }, { "Melody 85 G#", "G#" }, { "keys_Ebmaj", "D#" },
            { "Stab Am7", "Am" }, { "808 C1", "C" }, { "Kick Fat", "--" }, { "Bass Loop 120", "--" }, { "Guitar Loop 90 A", "A" }, { "drill_loop_142_bm", "Bm" }, { "am_i_dreaming", "--" },
            { "Snare 2", "--" }, { "Pad Bb", "A#" }, { "Lead_D_minor_128", "Dm" }, { "Cmaj9 chord", "C" } };
        for (auto& k : kn) { const auto got = keyName (keyFromName (k[0])); std::printf ("%-24s %-5s %s\n", k[0], got.toRawUTF8(), got == k[1] ? "ok" : "<<< atteso " ); if (got != k[1]) std::printf ("   atteso %s\n", k[1]); }
        const char* bn[][2] = { { "Loop 140 Bm", "140" }, { "drum_loop_92", "92" }, { "Guitar Loop 120 BPM", "120" }, { "bpm85_keys", "85" }, { "Loop_01_90", "90" }, { "808 Mafia 140", "140" } };
        for (auto& b : bn) { bool c = false; const float v = Processor::estimateBpm (b[0], 7.3, c); std::printf ("bpm %-24s %.1f conf %d %s\n", b[0], v, (int) c, juce::String (v, 0) == b[1] ? "ok" : "<<<"); }
        std::printf ("shift Bm->Dm %d  Bm->D %d  C->Am %d  nota C->Dm %d  Am->Em %d\n", keyShift (23, 14), keyShift (23, 2), keyShift (0, 21), keyShift (24, 14), keyShift (21, 16));
        // tonalita dall'ascolto: accordi di La minore
        {
            const double sr = 44100; juce::AudioBuffer<float> b (2, (int) (sr * 6)); b.clear();
            const double ch[][3] = { { 57, 60, 64 }, { 53, 57, 60 }, { 48, 52, 55 }, { 55, 59, 62 } };
            for (int c = 0; c < 4; ++c) for (int i = 0; i < (int) (sr * 1.5); ++i) { float v = 0; for (double m : ch[c]) v += (float) std::sin (2 * 3.14159 * 440 * std::pow (2, (m - 69) / 12) * i / sr) * 0.2f; v += (float) std::sin (2 * 3.14159 * 440 * std::pow (2, (ch[c][0] - 12 - 69) / 12) * i / sr) * 0.2f; b.setSample (0, c * (int) (sr * 1.5) + i, v); b.setSample (1, c * (int) (sr * 1.5) + i, v); }
            float conf = 0; const int k = keyFromAudio (b, sr, conf); std::printf ("ascolto Am-F-C-G: %s (conf %.2f)\n", keyName (k).toRawUTF8(), conf);
            juce::AudioBuffer<float> n (2, (int) (sr * 0.8)); for (int i = 0; i < n.getNumSamples(); ++i) { const float v = (float) (std::sin (2 * 3.14159 * 110 * i / sr) * std::exp (-i / sr * 3)); n.setSample (0, i, v); n.setSample (1, i, v); }
            const int k2 = keyFromAudio (n, sr, conf); std::printf ("ascolto nota La: %s\n", keyName (k2).toRawUTF8());
            juce::AudioFormatManager fm; fm.registerBasicFormats(); std::unique_ptr<juce::AudioFormatReader> rd (fm.createReaderFor (juce::File ("/tmp/loop_test.wav")));
            if (rd) { juce::AudioBuffer<float> lb (2, (int) rd->lengthInSamples); rd->read (&lb, 0, lb.getNumSamples(), 0, true, true); const int k3 = keyFromAudio (lb, rd->sampleRate, conf); std::printf ("ascolto loop di batteria: %s (conf %.2f)\n", keyName (k3).toRawUTF8(), conf); }
            for (int d : { 0, 2, 5, 9 }) { const auto smp = makeDrum (factoryKits()[1].pads[(size_t) d].d); const int kd = keyFromAudio (smp->buf, smp->sr, conf); std::printf ("ascolto %s: %s (%.2f)\n", smp->name.toRawUTF8(), keyName (kd).toRawUTF8(), conf); }
        }
        Processor p; p.prepareToPlay (48000, 512);
        // random kit
        int drumsLow = 0, tot = 0, weird = 0;
        for (int t = 0; t < 30; ++t)
        {
            randomKit (p, 0); if (p.kitName.contains ("Sperim")) ++weird;
            for (int q = 0; q < 8; ++q) if (auto s = p.getSample (q)) { DrumDesc d; if (pathToDesc (s->path, d)) { const auto c = categoryOf (d); if (c != "MELODICI" && c != "TEXTURE") ++drumsLow; } ++tot; }
            if (t < 3) { std::printf ("%s:", p.kitName.toRawUTF8()); for (int q = 0; q < 16; ++q) { auto s = p.getSample (q); std::printf (" %s%s", s ? s->name.toRawUTF8() : "-", juce::roundToInt (p.pad[q].get (TUNE)) ? juce::String ("(" + juce::String (juce::roundToInt (p.pad[q].get (TUNE))) + ")").toRawUTF8() : ""); } std::printf ("\n"); }
        }
        std::printf ("random: pad 1-8 batteria %d/%d, sperimentali %d/30\n", drumsLow, tot, weird);
        // annulla / ripeti
        p.loadFactoryKit (1); p.commitIfChanged(); p.pad[0].v[GAIN] = 5; p.commitIfChanged(); p.pad[0].v[GAIN] = 9; p.clearPad (3);
        const bool u1 = p.undo(); const float g1 = p.pad[0].get (GAIN); const bool has3 = p.getSample (3) != nullptr;
        p.undo(); const float g2 = p.pad[0].get (GAIN); p.redo(); const float g3 = p.pad[0].get (GAIN);
        std::printf ("undo: %d gain %.0f pad4 %d -> %.0f, redo %.0f\n", (int) u1, g1, (int) has3, g2, g3);
        // stretch che segue il tempo: loop a 92 BPM di 4 battute, DAW a 120 -> deve durare 8 s
        {
            const double sr = 48000; const int len = (int) (sr * 60.0 / 92 * 16); auto s = std::make_shared<SampleData>(); s->sr = sr; s->buf.setSize (2, len); s->buf.clear();
            for (int i = 0; i < len; ++i) { const float v = 0.3f * (float) std::sin (i * 0.05); s->buf.setSample (0, i, v); s->buf.setSample (1, i, v); }
            s->name = "loop 92"; p.loadFactoryKit (0); p.setSample (0, s); p.pad[0].v[SBPM] = 92; p.pad[0].v[WARP] = 2; p.pad[0].v[DECAY] = 4000;
            p.apvts.getParameter ("bpm")->setValueNotifyingHost (p.apvts.getParameter ("bpm")->convertTo0to1 (120));
            juce::AudioBuffer<float> blk (2, 512); juce::MidiBuffer m; m.addEvent (juce::MidiMessage::noteOn (1, p.rootNote(), (juce::uint8) 100), 0);
            int last = 0; for (int b = 0; b < 1200; ++b) { p.processBlock (blk, m); m.clear(); float pk = 0; for (int i = 0; i < 512; ++i) pk = std::max (pk, std::abs (blk.getSample (0, i))); if (pk > 1e-4f) last = b; }
            std::printf ("stretch 92->120: suona %.2f s (atteso 8.00)\n", (last + 1) * 512 / 48000.0);
        }
        {   // automazione: Ableton -> pad e pad -> Ableton
            auto* h = p.apvts.getParameter ("p01_tune"); h->setValueNotifyingHost (h->convertTo0to1 (7.0f));
            juce::AudioBuffer<float> blk (2, 512); juce::MidiBuffer m; p.processBlock (blk, m);
            const float a = p.pad[0].get (TUNE);
            p.setPadValue (0, CUTOFF, 800.0f); const float hc = p.apvts.getParameter ("p01_cutoff")->convertFrom0to1 (p.apvts.getParameter ("p01_cutoff")->getValue());
            p.apvts.getParameter ("satAmt")->setValueNotifyingHost (1.0f); p.apvts.getParameter ("revKit")->setValueNotifyingHost (0.5f); p.apvts.getParameter ("dlyKit")->setValueNotifyingHost (0.5f);
            p.loadFactoryKit (1); bool nan = false; float pk = 0;
            for (int b = 0; b < 200; ++b) { juce::MidiBuffer mm; if (b % 20 == 0) mm.addEvent (juce::MidiMessage::noteOn (1, p.rootNote() + (b / 20) % 4, (juce::uint8) 120), 0); p.processBlock (blk, mm); for (int i = 0; i < 512; ++i) { const float x = blk.getSample (0, i); if (! std::isfinite (x)) nan = true; pk = std::max (pk, std::abs (x)); } }
            std::printf ("automazione: tune da Ableton %.1f (7), cutoff verso Ableton %.0f (800); master kit: picco %.2f nan %d; parametri %d\n", a, hc, pk, (int) nan, p.getParameters().size());
        }
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "scan")
    {
        juce::ScopedJuceInitialiser_GUI gui3;
        juce::File dir ("/tmp/jscan"); dir.deleteRecursively(); dir.createDirectory();
        juce::AudioFormatManager fm; fm.registerBasicFormats(); juce::WavAudioFormat wav;
        auto write = [&] (const juce::File& f, const juce::AudioBuffer<float>& b, double sr)
        { f.getParentDirectory().createDirectory(); std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (new juce::FileOutputStream (f), sr, (unsigned) b.getNumChannels(), 24, {}, 0)); w->writeFromAudioSampleBuffer (b, 0, b.getNumSamples()); };
        // 1) suoni di fabbrica con nomi anonimi: il riconoscimento dall'ascolto deve indovinare la categoria
        int ok = 0, tot = 0; std::map<juce::String, std::pair<int,int>> per;
        juce::StringArray seen;
        for (auto& k : factoryKits()) for (auto& pd : k.pads)
        {
            const auto key = descToPath (pd.d); if (seen.contains (key)) continue; seen.add (key);
            auto smp = makeDrum (pd.d); if (! smp) continue;
            auto f = dir.getChildFile ("anon").getChildFile ("x" + juce::String (tot) + ".wav"); write (f, smp->buf, smp->sr);
            float dur = 0; const auto got = SampleIndex::classify (f, fm, dur); const auto want = categoryOf (pd.d);
            const bool good = got == want || (want == "OPEN HAT" && got == "HAT") || (want == "HAT" && got == "OPEN HAT") || (want == "TEXTURE" && got == "FX");
            ++tot; if (good) ++ok; per[want].second++; if (good) per[want].first++;
            if (! good)
            {
                std::unique_ptr<juce::AudioFormatReader> rd (fm.createReaderFor (f)); const auto F = SampleIndex::analyse (*rd);
                std::printf ("  %-18s %-10s->%-10s d%.2f a%.3f t30 %.2f L%.2f M%.2f H%.2f z%.0f ton%.2f f0 %.0f mic%d on%d\n", pd.d.name.toRawUTF8(), want.toRawUTF8(), got.toRawUTF8(), dur,
                             F.attack, F.t30, F.low, F.mid, F.high, F.zcr, F.tonal, F.f0, F.micro, F.onsets);
            }
        }
        for (auto& [c, v] : per) std::printf ("%-12s %d/%d\n", c.toRawUTF8(), v.first, v.second);
        std::printf ("ascolto: %d/%d giusti\n", ok, tot);
        // 2) nomi tipici dei pack
        const char* names[][2] = { { "Kicks/KSHMR_Kick_03.wav", "KICK" }, { "Drums/One Shots/Snares/TL_snr_12.wav", "SNARE" }, { "OpenHat02.wav", "OPEN HAT" }, { "hh_closed_tight.wav", "HAT" },
            { "Loops/Drum Loop 90bpm Am.wav", "LOOP" }, { "808 Mafia Kit/808_C_long.wav", "808" }, { "FX/Riser_Big_01.wav", "FX" }, { "Vocals/vox_chop_hey.wav", "VOCI" }, { "Melodic One Shots/Piano_C3.wav", "MELODICI" },
            { "Perc/Shaker_04.wav", "PERC" }, { "Crash 1.wav", "CYMBAL" }, { "Clap_Trap_05.wav", "CLAP / SNAP" }, { "Bass Drum 2.wav", "KICK" }, { "Vinyl Crackle.wav", "TEXTURE" }, { "Guitar Loop 120 BPM.wav", "LOOP" },
            { "Rim 3.wav", "PERC" }, { "Tom Low.wav", "TOM" }, { "Snare Roll.wav", "SNARE" }, { "superkick.wav", "KICK" } };
        int nok = 0, ntot = 0;
        for (auto& n : names)
        {
            // contenuto plausibile: loop lunghi, one shot corti
            const bool lng = juce::String (n[1]) == "LOOP"; const double sr = 44100; juce::AudioBuffer<float> b (1, (int) (sr * (lng ? 4.0 : 0.6))); b.clear(); juce::Random r (3);
            for (int beat = 0; beat < (lng ? 8 : 1); ++beat) for (int i = 0; i < (int) (sr * 0.25); ++i) b.addSample (0, beat * (int) (sr * 0.5) + i, (r.nextFloat() * 2 - 1) * (float) std::exp (-i / sr * 20) * 0.5f);
            auto f = dir.getChildFile (n[0]); write (f, b, sr);
            float dur = 0; const auto got = SampleIndex::classify (f, fm, dur); ++ntot; if (got == n[1]) ++nok; else std::printf ("  NOME %-40s atteso %-12s ottenuto %s\n", n[0], n[1], got.toRawUTF8());
        }
        std::printf ("nomi: %d/%d giusti\n", nok, ntot);
        // 3) scansione completa in sottofondo
        juce::SharedResourcePointer<SampleIndex> idx; idx->clearAll(); idx->addRoot (dir);
        while (idx->scanning()) juce::Thread::sleep (20);
        std::printf ("indice: %d file, KICK %d LOOP %d\n", idx->size(), idx->count ("KICK"), idx->count ("LOOP"));
        if (argc > 2)
        {
            Processor p; p.prepareToPlay (48000, 480);
            Editor ed (p);
            if (auto* jsc = std::getenv ("JSCALE")) ed.scaler.set ((float) std::atof (jsc)); ed.setPage (PG_BROWSE); ed.screen.browseCat = 0; ed.screen.browseDirty = true;
            auto img = ed.createComponentSnapshot (ed.getLocalBounds(), true, 1.0f);
            juce::File f (argv[2]); f.deleteFile(); juce::FileOutputStream os (f); juce::PNGImageFormat().writeImageToStream (img, os);
        }
        idx->clearAll();
        return 0;
    }
    if (argc > 2 && juce::String (argv[1]) == "snap")
    {
        juce::ScopedJuceInitialiser_GUI gui2;
        // loop sintetico: 4 battute a 92 BPM, nome senza BPM
        const double sr = 44100, bpm = 92; const int len = (int) (sr * 60.0 / bpm * 16);
        juce::AudioBuffer<float> b (2, len); b.clear(); juce::Random r (2);
        for (int beat = 0; beat < 16; ++beat)
        {
            const int st = (int) (beat * sr * 60.0 / bpm);
            for (int i = 0; i < (int) (sr * 0.3) && st + i < len; ++i)
            {
                const double t = i / sr; float v;
                if (beat % 2 == 0) v = (float) (std::sin (2 * 3.14159 * (50 + 100 * std::exp (-t * 30)) * t) * std::exp (-t * 7)) * 0.8f;
                else v = (r.nextFloat() * 2 - 1) * (float) std::exp (-t * 15) * 0.5f;
                b.addSample (0, st + i, v); b.addSample (1, st + i, v);
            }
            const int st2 = st + (int) (sr * 30.0 / bpm);
            for (int i = 0; i < (int) (sr * 0.05) && st2 + i < len; ++i) { const float v = (r.nextFloat() * 2 - 1) * (float) std::exp (-i / sr * 60) * 0.2f; b.addSample (0, st2 + i, v); b.addSample (1, st2 + i, v); }
        }
        juce::File wf ("/tmp/loop_test.wav"); wf.deleteFile();
        { juce::WavAudioFormat wav; std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (new juce::FileOutputStream (wf), sr, 2, 16, {}, 0)); w->writeFromAudioSampleBuffer (b, 0, len); }
        Processor p; p.prepareToPlay (48000, 480);
        p.loadFile (wf, 0);
        if (std::getenv ("JLAYER")) { p.loadLayerFile (wf, 0); p.pad[0].v[LGAIN] = -4; p.pad[0].v[LTUNE] = 12; }
        std::printf ("BPM rilevato %.2f  warp %d  mode %d\n", p.pad[0].get (SBPM), (int) p.pad[0].get (WARP), (int) p.pad[0].get (MODE));
        if (auto* jt = std::getenv ("JTHEME")) p.uiTheme = std::atoi (jt);
        Editor ed (p);
        if (auto* jsc = std::getenv ("JSCALE")) ed.scaler.set ((float) std::atof (jsc));
        if (auto* pg = std::getenv ("JPAGE")) { ed.selectPad (0); ed.setPage (std::atoi (pg)); ed.screen.kitSmall = std::getenv ("JSMALL") != nullptr; if (std::getenv ("JFX")) { p.qFx = true; ed.bindQlinks(); } for (int i = 0; i < 3; ++i) ed.screen.repaint(); }
        else { ed.openChop (0); ed.chop.selSlice = 2; ed.chop.hovPad = 5; ed.chop.assigned[3] = 1; ed.chop.pGlow[6] = 1.0f; ed.chop.pGlow[3] = 0.5f; }
        if (std::getenv ("JPERF"))
        {   // costo dei pezzi che si ridisegnano mentre suona (a 2x come uno schermo retina)
            juce::Image im (juce::Image::ARGB, ed.getWidth() * 2, ed.getHeight() * 2, true);
            auto timeIt = [&] (juce::Component& c, const char* nm, int n = 20)
            {
                const auto r = ed.getLocalArea (&c, c.getLocalBounds());
                const double t0 = juce::Time::getMillisecondCounterHiRes();
                for (int i = 0; i < n; ++i) { juce::Graphics g (im); g.addTransform (juce::AffineTransform::scale (2.0f)); g.reduceClipRegion (r); ed.paintEntireComponent (g, true); }
                std::printf ("  %-14s %6.2f ms  (%dx%d)\n", nm, (juce::Time::getMillisecondCounterHiRes() - t0) / n, r.getWidth(), r.getHeight());
            };
            p.uiRunning = true;
            for (auto* c : ed.getChildren()) for (auto* cc : c->getChildren()) juce::ignoreUnused (cc);
            timeIt (ed, "TUTTO", 5);
            timeIt (ed.screen, "schermo", std::getenv ("JN") ? 100000 : 20); timeIt (ed.pattern, "sequencer"); timeIt (*ed.pads[0], "un pad");
            { const auto r = ed.getLocalArea (ed.pads[0], ed.pads[0]->getLocalBounds()).getUnion (ed.getLocalArea (ed.pads[15], ed.pads[15]->getLocalBounds()));
              const double t0 = juce::Time::getMillisecondCounterHiRes();
              for (int i = 0; i < 20; ++i) for (auto* pc : ed.pads) { juce::Graphics g (im); g.addTransform (juce::AffineTransform::scale (2.0f)); g.reduceClipRegion (ed.getLocalArea (pc, pc->getLocalBounds())); ed.paintEntireComponent (g, true); }
              std::printf ("  16 pad accesi   %6.2f ms\n", (juce::Time::getMillisecondCounterHiRes() - t0) / 20); juce::ignoreUnused (r); }
            return 0;
        }
        if (std::getenv ("JKITS")) ed.kitBrowser.open();
        if (std::getenv ("JLIVE")) { ed.toggleLive(); for (int i = 0; i < 3; ++i) ed.screen.repaint(); }
        auto img = ed.createComponentSnapshot (ed.getLocalBounds(), true, 1.0f);
        juce::File f (argv[2]); f.deleteFile(); juce::FileOutputStream os (f); juce::PNGImageFormat().writeImageToStream (img, os);
        return 0;
    }
    juce::ScopedJuceInitialiser_GUI gui;
    if (argc > 1 && juce::String (argv[1]) == "gen")
    {
        Processor p; p.prepareToPlay (48000, 480);
        int bad = 0, total = 0;
        for (int kit = 0; kit < 6; ++kit)
        {
            if (kit < 4) p.loadFactoryKit (kit * 3); else randomKit (p, 0);
            juce::String roles; for (int k = 0; k < kPads; ++k) roles << (k + 1) << ":" << roleName (padRole (p, k)) << " ";
            std::printf ("\nKIT %s\n  %s\n", p.kitName.toRawUTF8(), roles.toRawUTF8());
            for (int st = 0; st < (int) genStyles().size(); ++st)
            {
                for (int seed = 1; seed <= 3; ++seed)
                {
                    if (kit < 4) p.loadFactoryKit (kit * 3);
                    const auto msg = generate (p, st, (juce::uint32) (seed * 977 + st), -1); ++total;
                    // doppioni: due pad dello stesso ruolo batteria nello stesso istante
                    std::map<long long, std::set<int>> at; int dbl = 0, n = 0;
                    for (auto& e : p.events) { const int ro = padRole (p, e.pad); if (ro == R_KICK || ro == R_SNARE || ro == R_CLAP || ro == R_HAT) { auto& v = at[(long long) std::llround (e.tick)]; if (v.count (ro)) ++dbl; v.insert (ro); } ++n; }
                    std::set<int> used; for (auto& e : p.events) used.insert (e.pad);
                    if (dbl) ++bad;
                    if (seed == 1) std::printf ("  %-22s ev %3d pad %2d dbl %d  %s\n", genStyles()[(size_t) st].name.toRawUTF8(), n, (int) used.size(), dbl, msg.toRawUTF8());
                }
            }
        }
        std::printf ("\npattern con doppioni: %d / %d\n", bad, total);
        // ascii del primo stile boom bap sul kit 0
        for (int sx : { 0, 4, 8 }) { p.loadFactoryKit (3); generate (p, sx, 5, -1); std::printf ("\n== %s\n", genStyles()[(size_t) sx].name.toRawUTF8());
        for (int k = 0; k < kPads; ++k)
        {
            juce::String row; for (int st = 0; st < p.patternBars.load() * 16; ++st) { char c = '.'; for (auto& e : p.events) if (e.pad == k && e.tick >= st * 24.0 && e.tick < st * 24.0 + 24) c = e.vel > 100 ? 'X' : 'x'; row << c << (st % 16 == 15 ? "|" : ""); }
            if (row.containsAnyOf ("Xx")) std::printf ("%2d %-9s %s\n", k + 1, roleName (padRole (p, k)), row.toRawUTF8());
        } }
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "outlvl")
    {   // picco in uscita di ogni pad dei kit di fabbrica, colpo a velocity 110
        Processor p; p.setPlayConfigDetails (0, 2, 44100, 512); p.prepareToPlay (44100, 512);
        for (int kit : { 1, 3, 5 })
        {
            p.loadFactoryKit (kit); std::printf ("KIT %s\n", p.kitName.toRawUTF8());
            for (int k = 0; k < kPads; ++k)
            {
                auto s = p.getSample (k); if (! s) continue;
                const float spk = s->buf.getMagnitude (0, s->buf.getNumSamples());
                juce::AudioBuffer<float> b (2, 512); juce::MidiBuffer m; float pk = 0;
                p.uiNoteOn (k, 110);
                for (int q = 0; q < 60; ++q) { b.clear(); p.processBlock (b, m); pk = juce::jmax (pk, b.getMagnitude (0, 512)); }
                p.uiNoteOff (k); for (int q = 0; q < 200; ++q) { b.clear(); p.processBlock (b, m); }
                std::printf ("  %2d %-22s campione %6.1f dBFS  uscita %6.1f dBFS\n", k + 1, s->name.toRawUTF8(), juce::Decibels::gainToDecibels (spk), juce::Decibels::gainToDecibels (pk));
            }
        }
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "levels")
    {
        std::vector<std::pair<float, juce::String>> all;
        for (auto& cat : libCategories())
            for (auto& it : libItems (cat))
            {
                if (it.user) continue;
                auto s = makeDrum (it.d); if (s == nullptr) continue;
                const int n = s->buf.getNumSamples(); const float pk = s->buf.getMagnitude (0, n);
                const int m = juce::jmin (n, (int) (s->sr * 0.3)); double e = 0; for (int i = 0; i < m; ++i) e += s->buf.getSample (0, i) * s->buf.getSample (0, i);
                const float rms = (float) juce::Decibels::gainToDecibels (std::sqrt (e / juce::jmax (1, m)));
                all.push_back ({ juce::Decibels::gainToDecibels (pk), cat + " / " + it.name + "  rms300ms " + juce::String (rms, 1) });
            }
        for (auto& a : all) a.first = a.second.fromFirstOccurrenceOf ("rms300ms ", false, false).getFloatValue(); std::sort (all.begin(), all.end(), [] (auto& a, auto& b) { return a.first < b.first; });
        std::printf ("suoni: %d\n", (int) all.size());
        for (size_t i = 0; i < all.size(); ++i) if (i < 25 || i + 5 > all.size()) std::printf ("%7.1f dB  %s\n", all[i].first, all[i].second.toRawUTF8());
        int under12 = 0, under20 = 0; for (auto& a : all) { if (a.first < -12) ++under12; if (a.first < -20) ++under20; }
        std::printf ("sotto -12 dB: %d   sotto -20 dB: %d\n", under12, under20);
        return 0;
    }
    Processor p; const double sr = 44100; p.prepareToPlay (sr, 512);
    juce::AudioBuffer<float> blk (2, 512); float peak = 0; bool nan = false;
    // velocity: picco del kick a diverse velocity
    for (int vel : { 127, 100, 64, 30 })
    {
        juce::MidiBuffer m; m.addEvent (juce::MidiMessage::noteOn (1, 36, (juce::uint8) vel), 0);
        float pk = 0; for (int b = 0; b < 20; ++b) { p.processBlock (blk, m); m.clear(); for (int i = 0; i < 512; ++i) pk = std::max (pk, std::abs (blk.getSample (0, i))); }
        std::printf ("kick vel %3d -> picco %.3f (%.1f dB)\n", vel, pk, juce::Decibels::gainToDecibels (pk));
    }
    // swing sulle note MIDI: nota sul secondo sedicesimo, swing 66, trasporto interno
    p.apvts.getParameter ("swing")->setValueNotifyingHost (p.apvts.getParameter ("swing")->convertTo0to1 (66));
    p.requestPlay (true);
    { juce::MidiBuffer m; p.processBlock (blk, m); }
    {
        const double tps = 120.0 / 60 * PPQ / sr; double t = 512 * tps; int onsetBlock = -1, onsetPos = -1;
        // nota a tick 24 (secondo sedicesimo): attesa a 24 + 0.32*24 = 31.7 tick
        const int b0 = (int) (24.0 / tps) / 512, off = (int) (24.0 / tps) % 512;
        for (int b = 1; b < 24; ++b)
        {
            juce::MidiBuffer m; if (b == b0) m.addEvent (juce::MidiMessage::noteOn (1, 37, (juce::uint8) 100), off);
            const auto before = p.hits[1].load(); p.processBlock (blk, m);
            if (p.hits[1].load() != before && onsetBlock < 0) { onsetBlock = b; for (int i = 0; i < 512; ++i) if (std::abs (blk.getSample (0, i)) > 1e-4f) { onsetPos = i; break; } }
            t += 512 * tps;
        }
        const double got = (onsetBlock * 512 + onsetPos) * tps;
        std::printf ("swing MIDI: nota a 24 tick suonata a %.1f tick (atteso ~31.7)\n", got);
    }
    // banca loop
    for (int i = 0; i < (int) patternBank().size(); ++i)
        for (auto& [pd, str] : patternBank()[(size_t) i].lanes) if (std::strlen (str) != 32 && std::strlen (str) != 64) std::printf ("PATTERN ERRATO %s %s pad %d len %d\n", patternBank()[(size_t) i].genre, patternBank()[(size_t) i].name, pd, (int) std::strlen (str));
    p.loadPattern (4); p.loopOn = true; p.varyPattern();
    std::printf ("pattern caricato: %d note\n", (int) p.events.size());
    // ogni kit, ogni pad, con catena attiva
    for (int k = 0; k < (int) factoryKits().size(); ++k)
    {
        p.loadFactoryKit (k); float kp = 0; bool kn = false;
        for (int pd = 0; pd < kPads; ++pd)
        {
            juce::MidiBuffer m; m.addEvent (juce::MidiMessage::noteOn (1, 36 + pd, (juce::uint8) 110), 0);
            for (int b = 0; b < 30; ++b) { p.processBlock (blk, m); m.clear(); for (int i = 0; i < 512; ++i) { const float x = blk.getSample (0, i); if (! std::isfinite (x)) kn = true; kp = std::max (kp, std::abs (x)); } }
        }
        std::printf ("kit %-22s picco %.2f nan %d\n", p.kitName.toRawUTF8(), kp, (int) kn);
    }
    // kit casuali
    for (int k = 0; k < 9; ++k)
    {
        randomKit (p, k % 3); float kp = 0; bool kn = false; int filled = 0;
        for (int pd = 0; pd < kPads; ++pd)
        {
            if (p.getSample (pd)) ++filled;
            juce::MidiBuffer m; m.addEvent (juce::MidiMessage::noteOn (1, 36 + pd, (juce::uint8) 110), 0);
            for (int b = 0; b < 20; ++b) { p.processBlock (blk, m); m.clear(); for (int i = 0; i < 512; ++i) { const float x = blk.getSample (0, i); if (! std::isfinite (x)) kn = true; kp = std::max (kp, std::abs (x)); } }
        }
        int hitCols = 0; for (int i = 0; i < Processor::kScope; ++i) if (p.scHit[i].load()) ++hitCols;
        std::printf ("random %-40s pad %d picco %.2f nan %d colonne-transienti %d\n", p.kitName.toRawUTF8(), filled, kp, (int) kn, hitCols);
    }
    // stretch: loop a 90 bpm suonato a 120 (interno)
    {
        p.loadFactoryKit (0); p.pad[0].v[SBPM] = 90; p.pad[0].v[WARP] = 2; p.pad[0].v[LOOP] = 1; p.pad[0].v[MODE] = 1;
        p.pad[0].v[CMODE] = 3; p.pad[0].v[LOWG] = 6; p.pad[0].v[DRIVE] = 0.5f; p.pad[0].v[BITS] = 0.5f; p.pad[0].v[RATE] = 0.5f; p.pad[0].v[SREV] = 0.5f; p.pad[0].v[SDLY] = 0.5f;
        p.apvts.getParameter ("busMode")->setValueNotifyingHost (1.0f); p.apvts.getParameter ("clip")->setValueNotifyingHost (1.0f);
        juce::MidiBuffer m; m.addEvent (juce::MidiMessage::noteOn (1, 36, (juce::uint8) 120), 0); float sp = 0; bool sn = false;
        for (int b = 0; b < 200; ++b) { p.processBlock (blk, m); m.clear(); if (b == 150) m.addEvent (juce::MidiMessage::noteOff (1, 36), 0); for (int i = 0; i < 512; ++i) { const float x = blk.getSample (0, i); if (! std::isfinite (x)) sn = true; sp = std::max (sp, std::abs (x)); } }
        std::printf ("stretch+catena: picco %.2f nan %d\n", sp, (int) sn);
    }
    bool c; std::printf ("bpm: %.1f %.1f\n", Processor::estimateBpm ("loop 92bpm", 5.2, c), Processor::estimateBpm ("break", 4.0, c));
    for (auto& c : libCategories()) { auto items = libItems (c); std::printf ("%s: %d suoni\n", c.toRawUTF8(), (int) items.size()); if (! items.empty()) loadLibItem (p, items.back(), 5); }
    for (int i = 0; i < (int) patternBank().size(); ++i) if (juce::String (patternBank()[(size_t) i].name).startsWith ("4 Bar")) { p.loadPattern (i); std::printf ("%s %s: %d battute, %d note\n", patternBank()[(size_t) i].genre, patternBank()[(size_t) i].name, p.patternBars.load(), (int) p.events.size()); }
    // note repeat: pad tenuto 1 battuta a 1/16 = 16 colpi
    {
        p.loopOn = false; p.apvts.getParameter ("nrOn")->setValueNotifyingHost (1); p.apvts.getParameter ("swing")->setValueNotifyingHost (0);
        p.requestPlay (false); { juce::MidiBuffer m; p.processBlock (blk, m); } p.requestPlay (true);
        const auto before = p.hits[3].load(); const double tps = 120.0 / 60 * PPQ / sr; const int nb = (int) (384 / tps / 512);
        for (int b = 0; b < nb; ++b) { juce::MidiBuffer m; if (b == 1) m.addEvent (juce::MidiMessage::noteOn (1, 39, (juce::uint8) 90), 0); if (b == nb - 1) m.addEvent (juce::MidiMessage::noteOff (1, 39), 0); p.processBlock (blk, m); }
        std::printf ("note repeat: %d colpi in ~1 battuta\n", (int) (p.hits[3].load() - before));
        p.apvts.getParameter ("nrOn")->setValueNotifyingHost (0);
    }
    // layer: kick + secondo campione sullo stesso pad
    {
        p.loopOn = false; p.requestPlay (false); p.loadFactoryKit (0);
        { auto c0 = libItems (libCategories()[0]); auto c1 = libItems (libCategories()[1]); loadLibItem (p, c0.front(), 0); loadLibItem (p, c1.front(), 1); p.pad[0].v[DECAY] = 4000; }
        std::printf ("pad1 %s\n", p.getSample (0) ? p.getSample (0)->name.toRawUTF8() : "-");
        for (int k = 0; k < kPads; ++k) p.pad[k].v[MUTE] = 0;
        auto hit = [&] (int vel) { juce::MidiBuffer m; m.addEvent (juce::MidiMessage::noteOn (1, 36, (juce::uint8) vel), 0); float pk = 0; bool nn = false;
                                   for (int b = 0; b < 30; ++b) { p.processBlock (blk, m); m.clear(); for (int i = 0; i < 512; ++i) { const float x = blk.getSample (0, i); if (! std::isfinite (x)) nn = true; pk = std::max (pk, std::abs (x)); } }
                                   for (int b = 0; b < 200; ++b) { juce::MidiBuffer e; p.processBlock (blk, e); } return nn ? -1.0f : pk; };
        const float a = hit (110);
        p.setLayer (0, p.getSample (1)); p.pad[0].v[LGAIN] = 0; p.pad[0].v[LPAN] = 0.6f;
        const float b = hit (110);
        p.pad[0].v[LVEL] = 100; p.pad[0].v[LMODE] = 1; const float sl = hit (60), sh = hit (120);
        p.pad[0].v[LMODE] = 0; p.pad[0].v[LVEL] = 0; p.pad[0].v[LPAN] = 0;
        std::printf ("layer: solo %.2f con layer %.2f | switch piano %.2f forte %.2f\n", a, b, sl, sh);
    }
    juce::MemoryBlock mb; p.getStateInformation (mb); Processor q; q.setStateInformation (mb.getData(), (int) mb.getSize());
    std::printf ("stato ricaricato: eventi %d, nome pad1 %s, layer pad1 %s\n", (int) q.events.size(), q.getSample (0) ? q.getSample (0)->name.toRawUTF8() : "-", q.getLayer (0) ? q.getLayer (0)->name.toRawUTF8() : "-");
}
