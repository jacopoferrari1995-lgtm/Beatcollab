#include "../Source/MrProcessor.h"
#include "../Source/MrEditor.h"
using namespace mr;
struct PH : juce::AudioPlayHead
{
    double ppq = 0, bpm = 140;
    juce::Optional<PositionInfo> getPosition() const override { PositionInfo p; p.setIsPlaying (true); p.setPpqPosition (ppq); p.setBpm (bpm); p.setTimeSignature (TimeSignature { 4, 4 }); return p; }
};
int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gi;
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
        Editor ed (*P); for (int i = 0; i < 10; ++i) for (int s = 0; s < MAXS; ++s) for (int b = 0; b < NB; ++b) { ed.smB[s][b] = hub().s[s].bands[b].load(); }
        for (int b = 0; b < NB; ++b) ed.smM[b] = P->radar() ? P->mBands[b].load() : P->sBands[b].load();
        auto img = ed.createComponentSnapshot (ed.getLocalBounds(), true, 1.0f);
        juce::File f (argv[1]); f.deleteFile(); juce::FileOutputStream os (f); juce::PNGImageFormat().writeImageToStream (img, os);
    }
    rad.setPlayHead (nullptr);
    return 0;
}
