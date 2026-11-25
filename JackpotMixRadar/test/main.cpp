#include "../Source/MrProcessor.h"
#include "../Source/MrEditor.h"
using namespace mr;
struct PH : juce::AudioPlayHead
{
    double ppq = 0, bpm = 140;
    juce::Optional<PositionInfo> getPosition() const override { PositionInfo p; p.setIsPlaying (true); p.setPpqPosition (ppq); p.setBpm (bpm); p.setTimeSignature (TimeSignature { 4, 4 }); return p; }
};
// mix finto per il solo master: rumore rosa "da mix" con difetti voluti
static void masterTest (int variant)
{
    const double sr = 48000; const int B = 512; Processor rad; rad.setV ("mode", 1); rad.setV ("ref", R_TRAP);
    PH ph; rad.setPlayHead (&ph); rad.prepareToPlay (sr, B);
    juce::Random r (4); double b0 = 0, b1 = 0, b2 = 0, b3 = 0, b4 = 0, b5 = 0, b6 = 0; juce::IIRFilter mud[2], res[2], sub[2], air[2];
    for (int c = 0; c < 2; ++c)
    {
        mud[c].setCoefficients (juce::IIRCoefficients::makePeakFilter (sr, 260, 1.0, juce::Decibels::decibelsToGain (variant == 1 ? 7.0f : 0.0f)));
        res[c].setCoefficients (juce::IIRCoefficients::makePeakFilter (sr, 3150, 12, juce::Decibels::decibelsToGain (variant == 1 ? 12.0f : 0.0f)));
        sub[c].setCoefficients (juce::IIRCoefficients::makeLowShelf (sr, 90, 0.7, juce::Decibels::decibelsToGain (variant == 0 ? 6.0f : 3.0f)));
        air[c].setCoefficients (juce::IIRCoefficients::makeHighShelf (sr, 6000, 0.7, juce::Decibels::decibelsToGain (variant == 1 ? -4.0f : -6.0f)));
    }
    juce::AudioBuffer<float> b (2, B); juce::MidiBuffer m; juce::Random rs (9); double lowS = 0;
    const int blocks = (int) (40 * sr / B);
    for (int k = 0; k < blocks; ++k)
    {
        for (int i = 0; i < B; ++i)
        {
            const double w = r.nextFloat() * 2 - 1;   // rosa (Kellet)
            b0 = 0.99886 * b0 + w * 0.0555179; b1 = 0.99332 * b1 + w * 0.0750759; b2 = 0.96900 * b2 + w * 0.1538520; b3 = 0.86650 * b3 + w * 0.3104856; b4 = 0.55000 * b4 + w * 0.5329522; b5 = -0.7616 * b5 - w * 0.0168980;
            const double pink = (b0 + b1 + b2 + b3 + b4 + b5 + b6 + w * 0.5362) * 0.05; b6 = w * 0.115926;
            const double s = (rs.nextFloat() * 2 - 1) * 0.02; lowS += 0.01 * (s - lowS);
            const double t = (k * B + i) / sr; const double env = 1.0; const double bump = t > 20 && t < 28 ? 4.0 : 0.0;   // battute con piu' bassi
            for (int c = 0; c < 2; ++c)
            {
                double x = pink + bump * b0 * 0.3 + (c ? -1 : 1) * (variant == 1 ? lowS * 6 : lowS * 0.5) + (c ? 0.3 : -0.3) * s * 3;
                float y = sub[c].processSingleSampleRaw ((float) x); y = mud[c].processSingleSampleRaw (y); y = res[c].processSingleSampleRaw (y); y = air[c].processSingleSampleRaw (y);
                b.setSample (c, i, y * 0.6f * (float) env);
            }
        }
        ph.ppq = (k * B) / sr * 140 / 60; rad.processBlock (b, m);
        if (k % 47 == 0) rad.timerCallbackForTest();
    }
    auto R = rad.scan.analyse (R_TRAP, true, rad.corr.load());
    std::printf ("\n=== MASTER variante %d (%.0f s)\n", variant, R.seconds);
    for (int f = 0; f < NF; ++f) std::printf ("%-9s scarto %+5.1f dB  side %4.1f%%\n", fasciaName (f), R.dev[f], R.side[f]);
    for (auto& x : R.res) std::printf ("risonanza %.0f Hz +%.1f dB\n", x.first, x.second);
    std::printf ("LUFS %.1f  PLR %.1f  LRA %.1f\n%s\n", R.lufsI, R.plr, R.lra, R.summary.toRawUTF8());
    for (auto& a : R.advice) std::printf (" [%.1f] %s\n      %s\n      -> %s\n", a.sev, a.title.toRawUTF8(), a.text.toRawUTF8(), a.fix.toRawUTF8());
    // fascia isolata: con SOLO BASSI la parte sopra 400 Hz quasi sparisce
    rad.setV ("solo", 2); juce::AudioBuffer<float> t (2, B); double eLo = 0, eHi = 0;
    for (int k = 0; k < 200; ++k) { for (int i = 0; i < B; ++i) { const float x = (float) std::sin (2 * 3.14159 * 1000 * (k * B + i) / sr) * 0.3f + (float) std::sin (2 * 3.14159 * 100 * (k * B + i) / sr) * 0.3f; t.setSample (0, i, x); t.setSample (1, i, x); } rad.processBlock (t, m); if (k > 50) for (int i = 0; i < B; ++i) { eHi += 0; eLo += t.getSample (0, i) * t.getSample (0, i); } }
    std::printf ("SOLO BASSI: rms %.3f (100 Hz da solo = 0.212)\n", std::sqrt (eLo / (149.0 * B)));
    if (auto* sn = std::getenv ("JSNAP"))
    {
        rad.rep = R; if (! std::getenv ("JSOLO")) rad.setV ("solo", 0);
        if (auto* t = std::getenv ("JTHEME")) skin::cur() = std::atoi (t);
        Editor ed (rad); ed.setSize (Editor::W, Editor::H); auto img = ed.createComponentSnapshot (ed.getLocalBounds(), true, 1.0f);
        juce::File fo (juce::String (sn) + juce::String (variant) + ".png"); fo.deleteFile(); juce::FileOutputStream os (fo); juce::PNGImageFormat().writeImageToStream (img, os);
    }
    rad.setPlayHead (nullptr);
}
int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gi;
    if (argc > 1 && juce::String (argv[1]) == "master") { masterTest (0); masterTest (1); return 0; }
    const double sr = 48000; const int B = 512;
    Processor kick, bass, voice, mel, rad;
    kick.setV ("role", KICK); bass.setV ("role", BASS); voice.setV ("role", VOICE); mel.setV ("role", MELODY); rad.setV ("mode", 1);
    kick.updateTrackProperties ({ "Kick", {} }); bass.updateTrackProperties ({ "808", {} }); voice.updateTrackProperties ({ "Lead Vox", {} }); mel.updateTrackProperties ({ "Pluck", {} });
    PH ph; rad.setPlayHead (&ph);
    for (auto* p : { &kick, &bass, &voice, &mel, &rad }) p->prepareToPlay (sr, B);
    juce::Random r (2); juce::AudioBuffer<float> bk (2, B), bb (2, B), bv (2, B), bm (2, B), master (2, B); juce::MidiBuffer m;
    float v1 = 0, v2 = 0; double ph808 = 0; int evalCount = 0;
    const int blocks = (int) (20 * sr / B);
    for (int k = 0; k < blocks; ++k)
    {
        for (int i = 0; i < B; ++i)
        {
            const double t = (k * B + i) / sr, beat = t * 140 / 60, ib = std::fmod (beat, 1.0) * 60 / 140;
            const float kk = (int) beat % 2 == 0 ? 0.8f * (float) (std::exp (-ib * 15) * std::sin (2 * 3.14159 * 55 * ib)) : 0.0f;
            ph808 += 2 * 3.14159 * 55 / sr; const float b8 = 0.5f * (float) std::sin (ph808) * (float) std::exp (-std::fmod (beat, 2.0) * 0.5);
            const bool sing = std::fmod (t, 4.0) < 3.0; float n = r.nextFloat() * 2 - 1; v1 += 0.4f * (n - v1); v2 += 0.08f * (v1 - v2); const float vo = sing ? (v1 - v2) * 0.25f : 0.0f;
            const float me = 0.25f * (float) (std::sin (2 * 3.14159 * 1760 * t) + std::sin (2 * 3.14159 * 2637 * t) + 0.5 * std::sin (2 * 3.14159 * 3520 * t));
            bk.setSample (0, i, kk); bk.setSample (1, i, kk); bb.setSample (0, i, b8 * 1.1f); bb.setSample (1, i, b8 * 0.6f);   // 808 larga in stereo
            bv.setSample (0, i, vo); bv.setSample (1, i, vo); bm.setSample (0, i, me); bm.setSample (1, i, me);
        }
        kick.processBlock (bk, m); bass.processBlock (bb, m); voice.processBlock (bv, m); mel.processBlock (bm, m);
        master.clear(); for (auto* x : { &bk, &bb, &bv, &bm }) for (int c = 0; c < 2; ++c) master.addFrom (c, 0, *x, c, 0, B);
        ph.ppq = (k * B) / sr * 140 / 60; rad.processBlock (master, m);
        if (k % 5 == 0) { rad.evaluate(); ++evalCount; }
    }
    std::printf ("sensori vivi:"); for (int i = 0; i < MAXS; ++i) if (hub().s[i].alive()) std::printf (" %s(%s)", hub().s[i].getName().toRawUTF8(), roleName (hub().s[i].role.load())); std::printf ("\n");
    std::printf ("master: LUFS-S %.1f picco %.1f corr %.2f bassi/medi %.1f\n", rad.lufsS.load(), rad.mPeak.load(), rad.corr.load(), rad.lowMid.load());
    for (auto& kv : rad.issues) { auto& is = kv.second; std::printf ("[%s] %s %s  (%d volte, battute %d..%d)  -> %s\n", typeName (is.type), is.who.toRawUTF8(), is.what.toRawUTF8(), is.count, is.bars.empty() ? 0 : is.bars.front(), is.bars.empty() ? 0 : is.bars.back(), is.fix.toRawUTF8()); }
    if (argc > 1)
    {
        Processor* P = std::getenv ("JSENS") ? &bass : &rad;
        if (auto* t = std::getenv ("JTHEME")) skin::cur() = std::atoi (t);
        Editor ed (*P); if (auto* t = std::getenv ("JTHEME")) { skin::cur() = std::atoi (t); neo::K::theme = -1; skin::apply(); } for (int i = 0; i < 10; ++i) for (int s = 0; s < MAXS; ++s) for (int b = 0; b < NB; ++b) { ed.smB[s][b] = hub().s[s].bands[b].load(); }
        for (int b = 0; b < NB; ++b) ed.smM[b] = P->radar() ? P->mBands[b].load() : P->sBands[b].load();
        auto img = ed.createComponentSnapshot (ed.getLocalBounds(), true, 1.0f);
        { const auto t0 = juce::Time::getMillisecondCounterHiRes(); for (int i = 0; i < 20; ++i) ed.createComponentSnapshot (ed.getLocalBounds(), true, 1.0f); std::printf ("fotogramma intero: %.2f ms\n", (juce::Time::getMillisecondCounterHiRes() - t0) / 20.0); }
        juce::File f (argv[1]); f.deleteFile(); juce::FileOutputStream os (f); juce::PNGImageFormat().writeImageToStream (img, os);
    }
    {   // il suono passa intatto, con qualsiasi buffer
        Processor q; q.prepareToPlay (sr, 4096); juce::Random rr (4); bool same = true; juce::MidiBuffer mm;
        for (int k = 0; k < 50; ++k) { const int n = 1 + rr.nextInt (4096); juce::AudioBuffer<float> b (2, n), c (2, n); for (int i = 0; i < n; ++i) { const float x = rr.nextFloat() - 0.5f; b.setSample (0, i, x); b.setSample (1, i, -x); } c.makeCopyOf (b); q.processBlock (b, mm); for (int i = 0; i < n; ++i) if (b.getSample (0, i) != c.getSample (0, i) || b.getSample (1, i) != c.getSample (1, i)) same = false; }
        std::printf ("sensore: uscita identica all'ingresso %d\n", (int) same);
    }
    rad.setPlayHead (nullptr);
    return 0;
}
