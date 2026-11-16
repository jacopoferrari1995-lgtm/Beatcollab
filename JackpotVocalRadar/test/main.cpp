#include "../Source/VrProcessor.h"
#include "../Source/VrEditor.h"
using namespace vr;
int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gi;
    const double sr = 48000; const int B = 512;
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
        Editor ed (*P);
        
        for (int b = 0; b < NB; ++b) { ed.space.smV[b] = P->uiVoice[b]; ed.space.smB[b] = P->uiBeat[b]; ed.space.smC[b] = P->uiCut[b]; }
        auto img = ed.createComponentSnapshot (ed.getLocalBounds(), true, 1.0f);
        juce::File f (argv[2]); f.deleteFile(); juce::FileOutputStream os (f); juce::PNGImageFormat().writeImageToStream (img, os);
    }
    return 0;
}
