#include "../Source/VrProcessor.h"
#include "../Source/VrEditor.h"
using namespace vr;
int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gi;
    const double sr = 48000; const int B = 512;
    if (argc > 1 && juce::String (argv[1]) == "lab")
    {   // copia BEAT con la voce dal sidechain: tutto nello stesso processo, ripetibile
        auto mk = [&] (int blk, std::function<void (Processor&)> setup) { auto p = std::make_unique<Processor>(); { juce::AudioProcessor::BusesLayout l; l.inputBuses.add (juce::AudioChannelSet::stereo()); l.inputBuses.add (juce::AudioChannelSet::stereo()); l.outputBuses.add (juce::AudioChannelSet::stereo()); if (! p->setBusesLayout (l)) std::printf ("layout rifiutato\n"); } p->setRateAndBufferSizeDetails (sr, blk); p->prepareToPlay (sr, blk); p->setV ("mode", 1); p->setV ("src", 1); setup (*p); return p; };
        bool noiseOn = true;
        auto gen = [&] (int k, float& l, float& r, float& v)
        {
            const double t = k / sr; const bool sing = std::fmod (t, 2.0) < 1.0; juce::Random rr ((juce::int64) k * 7919 + 1);
            const float syl = (float) (0.5 + 0.5 * std::sin (2 * 3.14159 * 5 * t));
            v = (float) (0.3 * std::sin (2 * 3.14159 * 220 * t) + 0.15 * std::sin (2 * 3.14159 * 2600 * t)) * syl * (sing ? 1.0f : 0.0f);
            const float be = 0.25f * (float) std::sin (2 * 3.14159 * 55 * t) + 0.12f * (float) std::sin (2 * 3.14159 * 2900 * t) + (noiseOn ? 0.05f * (rr.nextFloat() * 2 - 1) : 0.0f);
            l = be; r = be * 0.9f + 0.03f * (float) std::sin (2 * 3.14159 * 700 * t);
        };
        auto render = [&] (Processor& p, int len, int blk, bool uneven, std::function<void (Processor&, int)> at = {})
        {
            std::vector<float> o; juce::MidiBuffer m; juce::Random rb (3);
            for (int off = 0; off < len; )
            {
                const int n = uneven ? juce::jmin (len - off, 1 + rb.nextInt (blk)) : juce::jmin (blk, len - off);
                if (at) at (p, off);
                juce::AudioBuffer<float> b (4, n);
                for (int i = 0; i < n; ++i) { float l, r, v; gen (off + i, l, r, v); b.setSample (0, i, l); b.setSample (1, i, r); b.setSample (2, i, v); b.setSample (3, i, v); }
                p.processBlock (b, m); for (int i = 0; i < n; ++i) { o.push_back (b.getSample (0, i)); o.push_back (b.getSample (1, i)); } off += n;
            }
            return o;
        };
        const int len = (int) sr * 6;
        { auto p = mk (B, [] (Processor&) {}); std::printf ("bus sidechain: %d canali, attivo %d, ingressi totali %d\n", p->getBus (true, 1)->getNumberOfChannels(), (int) p->getBus (true, 1)->isEnabled(), p->getTotalNumInputChannels()); }
        {   // neutro: AMOUNT 0 = ingresso ritardato al bit
            auto p = mk (B, [] (Processor&) {}); const int L = p->getLatencySamples(); auto o = render (*p, len, B, false);
            double e = 0; for (int i = L; i < len; ++i) { float l, r, v; gen (i - L, l, r, v); e = std::max (e, (double) std::abs (o[(size_t) i * 2] - l)); e = std::max (e, (double) std::abs (o[(size_t) i * 2 + 1] - r)); }
            std::printf ("neutro (AMOUNT 0): latenza %d, differenza massima %.2g\n", L, e);
        }
        for (int pr : { 1, 4, 11 })
        {
            auto a = mk (B, [pr] (Processor& p) { p.loadPreset (pr); }); auto b2 = mk (B, [pr] (Processor& p) { p.loadPreset (pr); });
            auto o1 = render (*a, len, B, false), o2 = render (*b2, len, B, true);
            double e = 0, d = 0; for (size_t i = 0; i < o1.size(); ++i) { e += (double) o1[i] * o1[i]; d += std::pow (o1[i] - o2[i], 2.0); }
            // quanto scende il beat a 2.9 kHz quando la voce canta
            std::printf ("preset %-24s blocchi irregolari %.1f dB", presets()[(size_t) pr].name, 10 * std::log10 (d / (e + 1e-30) + 1e-30));
            auto band = [&] (double t0, double t1) { double re = 0, im = 0; int n = 0; for (int i = (int) (t0 * sr); i < (int) (t1 * sr); ++i) { const double ph = 2 * 3.14159265 * 2900 * (i - a->getLatencySamples()) / sr; re += o1[(size_t) i * 2] * std::cos (ph); im += o1[(size_t) i * 2] * std::sin (ph); ++n; } return 20 * std::log10 (2 * std::sqrt (re * re + im * im) / n / 0.12); };
            std::printf ("   2.9 kHz: voce %.1f dB, pausa %.1f dB  (sidechain %d, taglio max %.1f)\n", band (4.3, 4.9), band (5.4, 5.95), (int) a->uiSide.load(), [&] { float m = 0; for (int b = 0; b < NB; ++b) m = std::max (m, a->uiCut[b].load()); return m; }());
        }
        {   // click: ogni comando salta 4 volte; derivata seconda nel punto del salto contro lo stesso punto senza salto
            const char* ids[] = { "amount", "pres", "body", "air", "vol", "stereo", "rel", "ant", "sens", "mix", "out", "delta", "byp" };
            int nc = 0; noiseOn = false;
            for (auto* id : ids)
            {
                auto run = [&] (int mode)
                {
                    auto p = mk (B, [] (Processor& q) { q.loadPreset (1); });
                    auto* prm = p->apvts.getParameter (id); const float v0 = prm->getValue(), v1 = v0 > 0.5f ? v0 - 0.5f : v0 + 0.5f;
                    if (mode == 1) prm->setValueNotifyingHost (v1);
                    return render (*p, len, B, false, [&] (Processor&, int off) { const int bk = off / B; if (mode == 2 && bk >= 300 && (bk - 300) % 60 == 0 && bk < 540) prm->setValueNotifyingHost (((bk - 300) / 60) % 2 ? v0 : v1); });
                };
                const auto a = run (0), c1 = run (1), b2 = run (2);
                double worst = 0;
                for (int j = 0; j < 4; ++j)
                {
                    const size_t at = (size_t) (300 + j * 60) * B;
                    auto hf = [&] (const std::vector<float>& x) { double s2 = 0; for (size_t i = at - 200; i < at + 2200; ++i) { const double d2 = x[(i + 2) * 2] - 2 * x[(i + 1) * 2] + x[i * 2]; s2 = std::max (s2, std::abs (d2)); } return s2; };
                    worst = std::max (worst, hf (b2) / juce::jmax (1e-9, hf (a), hf (c1)));
                }
                std::printf ("  %-8s salto / fermo %.2f%s\n", id, worst, worst > 2.0 ? "  CLICK?" : ""); if (worst > 2.0) ++nc;
            }
            std::printf ("click sospetti: %d\n", nc); noiseOn = true;
        }
        {   // CPU
            auto p = mk (B, [] (Processor& q) { q.loadPreset (1); }); const auto t0 = juce::Time::getMillisecondCounterHiRes(); render (*p, (int) sr * 10, B, false);
            std::printf ("CPU: %.1f%% di un core\n", (juce::Time::getMillisecondCounterHiRes() - t0) / 100.0);
        }
        return 0;
    }
    Processor voice, beat;
    voice.setV ("mode", 0); beat.setV ("mode", 1);
    voice.prepareToPlay (sr, B); beat.prepareToPlay (sr, B);
    if (argc > 1 && juce::String (argv[1]) == "preset") beat.loadPreset (1); else beat.setV ("amount", 8);
    juce::Random r (1); float lp1 = 0, lp2 = 0, bp = 0, pk = 0;
    juce::AudioBuffer<float> vb (2, B), bb (2, B); juce::MidiBuffer m;
    // misura: energia del beat 2-4 kHz in uscita quando la voce canta e quando tace
    double eOn = 0, eOff = 0, eIn = 0; int nOn = 0, nOff = 0; float z1 = 0, z2 = 0; bool nan = false; float bmax = 0;
    beat.learnStart();
    const int blocks = (int) ((std::getenv ("JMID") ? 12.5 : 12.0) * sr / B);
    for (int k = 0; k < blocks; ++k)
    {
        for (int i = 0; i < B; ++i)
        {
            const double t = (k * B + i) / sr; const bool sing = std::fmod (t, 2.0) < 1.0;
            const float syl = (float) (0.5 + 0.5 * std::sin (2 * 3.14159 * 5 * t));
            float n = r.nextFloat() * 2 - 1; lp1 += 0.25f * (n - lp1); lp2 += 0.04f * (lp1 - lp2); const float vo = (lp1 - lp2) * 0.6f * syl * (sing ? 1.0f : 0.0f);
            vb.setSample (0, i, vo); vb.setSample (1, i, vo);
            const float bn = r.nextFloat() * 2 - 1; bp += 0.3f * (bn - bp);
            const float be = bp * 0.3f + 0.3f * (float) std::sin (2 * 3.14159 * 55 * t);
            bb.setSample (0, i, be + (r.nextFloat() - 0.5f) * 0.05f); bb.setSample (1, i, be - (r.nextFloat() - 0.5f) * 0.05f);
        }
        voice.processBlock (vb, m); beat.processBlock (bb, m);
        for (int i = 0; i < B; ++i)
        {
            const double t = (k * B + i - 1920) / sr; const float x = bb.getSample (0, i);
            if (! std::isfinite (x)) nan = true; bmax = std::max (bmax, std::abs (x));
            z1 += 0.35f * (x - z1); z2 += 0.20f * (z1 - z2); const float band = z1 - z2;   // grosso modo 1.5-5 kHz
            if (t > 2.0) { const double ph = std::fmod (t, 2.0); if (ph > 0.3 && ph < 0.9) { eOn += band * band; ++nOn; } else if (ph > 1.5 && ph < 1.95) { eOff += band * band; ++nOff; } }
        }
    }
    beat.learnStop(); if (std::getenv ("JAPPLY")) beat.learnApply();
    std::printf ("latenza BEAT %d campioni (%.1f ms), VOCE %d\n", beat.getLatencySamples(), beat.getLatencySamples() / sr * 1000, voice.getLatencySamples());
    std::printf ("collegato: %d, beat in ascolto sul canale A: %d\n", (int) beat.uiLinked.load(), beatsListening (0));
    std::printf ("banda 1.5-5 kHz del beat: voce che canta %.1f dB, voce zitta %.1f dB  ->  spazio aperto %.1f dB\n", 10 * std::log10 (eOn / nOn), 10 * std::log10 (eOff / nOff), 10 * std::log10 (eOn / nOn) - 10 * std::log10 (eOff / nOff));
    std::printf ("picco uscita %.2f nan %d\n", bmax, (int) nan);
    for (auto& l : beat.proposal.lines) std::printf ("  > %s\n", l.toRawUTF8());
    if (argc > 2 && juce::String (argv[1]) == "snap")
    {
        Processor* P = std::getenv ("JVOICE") ? &voice : &beat;
        if (auto* t = std::getenv ("JTHEME")) skin::cur() = std::atoi (t);
        Editor ed (*P);
        if (std::getenv ("JTHEME")) { skin::cur() = std::atoi (std::getenv ("JTHEME")); neo::K::theme = -1; skin::apply(); }
        for (int b = 0; b < NB; ++b) { ed.space.smV[b] = P->uiVoice[b]; ed.space.smB[b] = P->uiBeat[b]; ed.space.smC[b] = P->uiCut[b]; }
        auto img = ed.createComponentSnapshot (ed.getLocalBounds(), true, 1.0f);
        { const auto t0 = juce::Time::getMillisecondCounterHiRes(); for (int i = 0; i < 30; ++i) { ed.space.createComponentSnapshot (ed.space.getLocalBounds(), true, 1.0f); ed.lanes.createComponentSnapshot (ed.lanes.getLocalBounds(), true, 1.0f); }
          std::printf ("fotogramma (spettro + corsie): %.2f ms\n", (juce::Time::getMillisecondCounterHiRes() - t0) / 30.0); }
        juce::File f (argv[2]); f.deleteFile(); juce::FileOutputStream os (f); juce::PNGImageFormat().writeImageToStream (img, os);
    }
    return 0;
}
