#include "../Source/TapeProcessor.h"
#include "../Source/TapeEditor.h"
#include <cstdlib>
#include "TestLab.h"
using namespace jt;
int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    if (argc > 1 && juce::String (argv[1]) == "hiss")
    {   // fruscio al massimo: silenzio, poi un tono a -20 dBFS (fruscio sotto la musica)
        for (float h : { 0.35f, 1.0f })
        {
            Processor p; p.setPlayConfigDetails (2, 2, 48000, 512); p.prepareToPlay (48000, 512); p.loadPreset (0);
            auto* r = p.apvts.getParameter ("hiss"); r->setValueNotifyingHost (r->convertTo0to1 (h));
            std::vector<float> o = testlab::render (p, 48000 * 6, [&] (int k) { return k < 48000 * 3 ? 0.0f : 0.1f * (float) std::sin (2 * 3.14159265 * 440 * k / 48000.0); });
            double eS = 0; for (int i = 48000; i < 48000 * 3 - 2000; ++i) eS += (double) o[(size_t) i] * o[(size_t) i];
            const int a = 48000 * 4, b = 48000 * 6; double re = 0, im = 0; for (int i = a; i < b; ++i) { const double w = 2 * 3.14159265 * 440 * i / 48000.0; re += o[(size_t) i] * std::cos (w); im += o[(size_t) i] * std::sin (w); }
            re *= 2.0 / (b - a); im *= 2.0 / (b - a); double eR = 0; for (int i = a; i < b; ++i) { const double w = 2 * 3.14159265 * 440 * i / 48000.0; eR += std::pow (o[(size_t) i] - re * std::cos (w) - im * std::sin (w), 2.0); }
            std::printf ("HISS %.0f%%: silenzio %.1f dBFS, sotto la musica %.1f dBFS\n", h * 100, 10 * std::log10 (eS / (48000 * 2 - 2000) + 1e-30), 10 * std::log10 (eR / (b - a) + 1e-30));
        }
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "blk")
    {   // primo campione diverso tra blocchi da 512 e blocchi irregolari
        const int pr = juce::SystemStats::getEnvironmentVariable ("JPRESET", "3").getIntValue(); const int mb = juce::SystemStats::getEnvironmentVariable ("JBLK", "64").getIntValue();
        auto mk = [&] (int blk) { auto p = std::make_unique<Processor>(); p->setPlayConfigDetails (2, 2, 48000, blk); p->prepareToPlay (48000, blk); p->loadPreset (pr);
            if (auto* js = std::getenv ("JSET")) for (auto kv : juce::StringArray::fromTokens (js, ",", "")) { auto* r = p->apvts.getParameter (kv.upToFirstOccurrenceOf ("=", false, false)); r->setValueNotifyingHost (r->convertTo0to1 (kv.fromFirstOccurrenceOf ("=", false, false).getFloatValue())); }
            return p; };
        juce::Random rr (9); std::vector<float> in (48000 * 3); for (auto& x : in) x = 0.02f * (rr.nextFloat() * 2 - 1);
        auto a = mk (512); auto b = mk (mb);
        auto o1 = testlab::render (*a, (int) in.size(), [&] (int k) { return in[(size_t) k]; });
        auto o2 = testlab::render (*b, (int) in.size(), [&] (int k) { return in[(size_t) k]; }, mb, true);
        for (size_t i = 0; i < o1.size(); ++i) if (std::abs (o1[i] - o2[i]) > 1e-7f) { std::printf ("primo diverso a %d (%.4f s): %.6f vs %.6f\n", (int) i, i / 48000.0, o1[i], o2[i]); break; }
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "pads")
    {   // ogni pad acceso e spento 4 volte su un tono: salto massimo della derivata seconda (scatto) contro il tono da solo
        for (int f = 0; f < kNumFx; ++f)
        {
            Processor p; p.setPlayConfigDetails (2, 2, 48000, 512); p.prepareToPlay (48000, 512); p.loadPreset (0);
            auto* pr = p.apvts.getParameter ("pad" + juce::String (f + 1));
            auto o = testlab::render (p, 512 * 200, [] (int k) { return 0.5f * (float) std::sin (2 * 3.14159265 * 220 * k / 48000.0); }, 512, false,
                                      [&] (int off) { const int b = off / 512; if (b >= 60 && (b - 60) % 30 == 0 && b < 180) pr->setValueNotifyingHost (((b - 60) / 30) % 2 ? 0.0f : 1.0f); });
            float mx = 0; int at = 0; for (size_t i = 2 + 512 * 50; i < o.size(); ++i) { const float d = std::abs (o[i] - 2 * o[i - 1] + o[i - 2]); if (d > mx) { mx = d; at = (int) i; } }
            std::printf ("%-14s salto %.4f a %.3f s %s\n", kFx[f].name, mx, at / 48000.0, mx > 0.02f ? "<-- !!" : "");
        }
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "lab")
    {
        const int pr = juce::SystemStats::getEnvironmentVariable ("JPRESET", "1").getIntValue();
        { Processor q; std::printf ("preset: %s\n", q.getProgramName (pr).toRawUTF8()); }
        testlab::run ([&] (double rate, int blk) -> std::unique_ptr<juce::AudioProcessor>
        {
            auto p = std::make_unique<Processor>(); p->setPlayConfigDetails (2, 2, rate, blk); p->prepareToPlay (rate, blk); p->loadPreset (pr);
            if (auto* js = std::getenv ("JSET")) for (auto kv : juce::StringArray::fromTokens (js, ",", "")) { auto* r = p->apvts.getParameter (kv.upToFirstOccurrenceOf ("=", false, false)); r->setValueNotifyingHost (r->convertTo0to1 (kv.fromFirstOccurrenceOf ("=", false, false).getFloatValue())); }
            return p;
        }, std::getenv ("JNOCLICK") == nullptr, [] (juce::AudioProcessorParameter* q)
        {
            auto* r = dynamic_cast<juce::RangedAudioParameter*> (q); if (r == nullptr) return true;
            if (auto* only = std::getenv ("JONLY")) return ! r->paramID.contains (only);
            return false;
        });
        return 0;
    }
    if (argc > 2 && juce::String (argv[1]) == "snap")
    {
        if (auto* t = std::getenv ("JTHEME")) skin::cur() = std::atoi (t);
        Processor p; p.prepareToPlay (44100, 512);
        const int pr = juce::SystemStats::getEnvironmentVariable ("JPRESET", "0").getIntValue(); p.loadPreset (pr);
        std::unique_ptr<juce::AudioProcessorEditor> ed (p.createEditor());
        if (auto* e = dynamic_cast<Editor*> (ed.get())) e->select (juce::SystemStats::getEnvironmentVariable ("JFX", "9").getIntValue());
        ed->setSize (Editor::W, Editor::H);
        if (std::getenv ("JBROWSE")) if (auto* e = dynamic_cast<Editor*> (ed.get())) e->browser.open();
        if (std::getenv ("JPERF"))
        {   // tempo di un ridisegno completo e dei soli elementi animati
            juce::Image im (juce::Image::RGB, Editor::W * 2, Editor::H * 2, true);
            auto t0 = juce::Time::getMillisecondCounterHiRes();
            for (int i = 0; i < 20; ++i) { juce::Graphics g (im); g.addTransform (juce::AffineTransform::scale (2.0f)); ed->paintEntireComponent (g, true); }
            auto t1 = juce::Time::getMillisecondCounterHiRes();
            auto* e = dynamic_cast<Editor*> (ed.get());
            for (int i = 0; i < 20; ++i) { juce::Graphics g (im); g.addTransform (juce::AffineTransform::scale (2.0f)); g.reduceClipRegion (e->disp.getBoundsInParent()); ed->paintEntireComponent (g, true); }
            auto t2 = juce::Time::getMillisecondCounterHiRes();
            std::printf ("ridisegno completo (retina) %.1f ms   solo display %.1f ms\n", (t1 - t0) / 20, (t2 - t1) / 20);
            for (auto* c : ed->getChildren()) for (auto* cc : c->getChildren())
            {
                const auto t3 = juce::Time::getMillisecondCounterHiRes();
                for (int i = 0; i < 10; ++i) { juce::Image im2 (juce::Image::ARGB, juce::jmax (1, cc->getWidth() * 2), juce::jmax (1, cc->getHeight() * 2), true); juce::Graphics g2 (im2); g2.addTransform (juce::AffineTransform::scale (2.0f)); cc->paintEntireComponent (g2, true); }
                const double ms = (juce::Time::getMillisecondCounterHiRes() - t3) / 10; if (ms > 0.5) std::printf ("  %s %dx%d: %.2f ms\n", typeid (*cc).name(), cc->getWidth(), cc->getHeight(), ms);
            }
        }
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, std::getenv ("JX2") ? 2.0f : 1.0f);
        juce::File f (argv[2]); f.deleteFile(); juce::FileOutputStream os (f); juce::PNGImageFormat().writeImageToStream (img, os);
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "level")
    {
        auto run = [] (std::function<void (Processor&)> setup)
        {
            Processor p; const double sr = 44100; p.prepareToPlay (sr, 512); setup (p);
            juce::AudioBuffer<float> b (2, 512); juce::MidiBuffer m; double ph = 0, ei = 0, eo = 0; float pk = 0;
            for (int k = 0; k < 300; ++k)
            {
                for (int i = 0; i < 512; ++i) { const float v = (float) (std::sin (ph) * 0.35 + std::sin (ph * 0.25) * 0.2); ph += 2 * 3.14159 * 220 / sr; b.setSample (0, i, v); b.setSample (1, i, v); if (k > 50) ei += v * v; }
                p.processBlock (b, m);
                if (k > 50) for (int i = 0; i < 512; ++i) { eo += b.getSample (0, i) * b.getSample (0, i); pk = std::max (pk, std::abs (b.getSample (0, i))); }
            }
            return std::make_pair ((float) juce::Decibels::gainToDecibels (std::sqrt (eo / ei)), pk);
        };
        for (float a : { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f }) { auto r = run ([a] (Processor& p) { p.applyAge (a); }); std::printf ("age %.2f  guadagno %+.1f dB  picco %.2f\n", a, r.first, r.second); }
        for (float d : { 0.0f, 0.2f, 0.5f, 0.8f, 1.0f }) { auto r = run ([d] (Processor& p) { auto* pr = p.apvts.getParameter ("drive"); pr->setValueNotifyingHost (d); }); std::printf ("drive %.2f guadagno %+.1f dB  picco %.2f\n", d, r.first, r.second); }
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "mod")
    {   // nuovi effetti sovrapposti: livello rispetto all'ingresso, niente nan
        for (int f = TREM; f < kNumFx; ++f)
        {
            Processor p; const double sr = 44100; p.prepareToPlay (sr, 512); p.uiPad[f] = true;
            juce::AudioBuffer<float> b (2, 512); juce::MidiBuffer m; double ph = 0, ei = 0, eo = 0, el = 0, er = 0; bool nan = false;
            for (int k = 0; k < 400; ++k)
            {
                for (int i = 0; i < 512; ++i) { float v = 0; for (int h = 1; h < 6; ++h) v += (float) std::sin (ph * h) / h; v *= 0.2f; ph += 2 * 3.14159 * 196 / sr; b.setSample (0, i, v); b.setSample (1, i, v); if (k > 100) ei += v * v; }
                p.processBlock (b, m);
                if (k > 100) for (int i = 0; i < 512; ++i) { const float l = b.getSample (0, i), r = b.getSample (1, i); if (! std::isfinite (l)) nan = true; eo += 0.5 * (l * l + r * r); el += l * l; er += r * r; }
            }
            std::printf ("%-12s livello %+.1f dB  L/R %+.1f dB  nan %d\n", kFx[f].name, juce::Decibels::gainToDecibels (std::sqrt (eo / ei)), juce::Decibels::gainToDecibels (std::sqrt (el / juce::jmax (1e-9, er))), (int) nan);
        }
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "gen")
    {
        Processor p; const double sr = 44100; p.prepareToPlay (sr, 512);
        juce::AudioBuffer<float> b (2, 512); juce::MidiBuffer m;
        for (int st = 0; st < 6; ++st) for (float amt : { 0.2f, 0.5f, 0.9f })
        {
            p.apvts.getParameter ("genStyle")->setValueNotifyingHost (p.apvts.getParameter ("genStyle")->convertTo0to1 ((float) st));
            p.apvts.getParameter ("genAmt")->setValueNotifyingHost (amt);
            p.generate();
            const int S = (int) p.value ("bars") == 1 ? 32 : 64; int overlaps = 0;
            std::printf ("stile %d amt %.1f  %d passi\n", st, amt, S);
            for (int s2 = 0; s2 < S; ++s2) { int mv = 0; for (int f = 0; f < kNumFx; ++f) if (f != GATE && p.seq[f][s2].load()) ++mv; if (mv > 1) ++overlaps; }
            for (int f = 0; f < kNumFx; ++f) { juce::String row; bool any = false; for (int s2 = 0; s2 < S; ++s2) { row << (p.seq[f][s2].load() ? "#" : (s2 % 4 == 0 ? "|" : ".")); any |= p.seq[f][s2].load(); } if (any) std::printf ("  %-12s %s\n", kFx[f].name, row.toRawUTF8()); }
            bool nan = false; float pk = 0; double ph = 0;
            for (int k = 0; k < 400; ++k) { for (int i = 0; i < 512; ++i) { const float v = (float) std::sin (ph) * 0.4f; ph += 0.03; b.setSample (0, i, v); b.setSample (1, i, v); } p.processBlock (b, m); for (int i = 0; i < 512; ++i) { if (! std::isfinite (b.getSample (0, i))) nan = true; pk = std::max (pk, std::abs (b.getSample (0, i))); } }
            std::printf ("  sovrapposizioni %d  nan %d  picco %.2f\n", overlaps, (int) nan, pk);
        }
        return 0;
    }
    const int FX = argc > 1 ? atoi (argv[1]) : -1;
    Processor p; const double sr = 44100; p.prepareToPlay (sr, 512);
    juce::AudioBuffer<float> blk (2, 512); juce::MidiBuffer m;
    const int total = (int) (sr * 12); float peak = 0; bool nan = false; double ph = 0;
    juce::AudioBuffer<float> out (2, total);
    if (FX < 0) { p.loadPreset (14); auto* sq = p.apvts.getParameter ("seqOn"); sq->setValueNotifyingHost (1); }
    for (int pos = 0; pos + 512 <= total; pos += 512)
    {
        for (int i = 0; i < 512; ++i) { const double t = (pos + i) / sr; const double beat = std::fmod (t * 140 / 60, 1.0);
            const float v = (float) (std::sin (ph) * 0.4 + (beat < 0.05 ? std::sin (t * 2 * 3.14159 * 60) * (1 - beat / 0.05) * 0.5 : 0)); ph += 2 * 3.14159 * 220 / sr;
            blk.setSample (0, i, v); blk.setSample (1, i, v); }
        const int b = pos / 512;
        if (FX < 0) { for (int f = 0; f < kNumFx; ++f) p.uiPad[f] = (b / 20) % 12 == f + 1 && (b % 20) < 14; }
        else p.uiPad[FX] = pos >= sr * 2 && pos < sr * 4;
        p.processBlock (blk, m);
        for (int c = 0; c < 2; ++c) for (int i = 0; i < 512; ++i) { const float x = blk.getSample (c, i); if (! std::isfinite (x)) nan = true; peak = std::max (peak, std::abs (x)); }
        out.copyFrom (0, pos, blk, 0, 0, 512); out.copyFrom (1, pos, blk, 1, 0, 512);
    }
    std::printf ("peak %.3f nan %d programs %d\n", peak, (int) nan, p.getNumPrograms());
    if (FX >= 0) { for (int k = 18; k < 46; ++k) { double e = 0; const int a = (int) (k * sr / 10); for (int i = 0; i < 4410; ++i) e += out.getSample (0, a + i) * out.getSample (0, a + i); std::printf ("%.2f ", std::sqrt (e / 4410)); } std::printf ("\n"); }
    juce::WavAudioFormat w; juce::File f ("/tmp/tapetest.wav"); f.deleteFile();
    std::unique_ptr<juce::AudioFormatWriter> wr (w.createWriterFor (new juce::FileOutputStream (f), sr, 2, 16, {}, 0)); wr->writeFromAudioSampleBuffer (out, 0, total);
}
