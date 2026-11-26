#include "../Source/TagProcessor.h"
#include "../Source/TagEditor.h"
#include <cstdio>
using namespace jtag;

// frase finta: tre sillabe di vocale con altezza che scende, pause tra una e l'altra, silenzio prima e dopo
static juce::AudioBuffer<float> phrase (double sr)
{
    juce::AudioBuffer<float> b (2, (int) (sr * 2.0)); b.clear(); double ph = 0;
    const double syl[][3] = { { 0.30, 0.18, 220 }, { 0.55, 0.16, 196 }, { 0.78, 0.40, 165 } };
    for (auto& s : syl)
        for (int i = 0; i < (int) (s[1] * sr); ++i)
        {
            const int k = (int) (s[0] * sr) + i; const double t = i / (s[1] * sr), env = std::sin (juce::MathConstants<double>::pi * t);
            ph += 2 * juce::MathConstants<double>::pi * s[2] / sr;
            const float v = (float) (0.4 * env * (std::sin (ph) + 0.5 * std::sin (2 * ph) + 0.3 * std::sin (3 * ph) + 0.15 * std::sin (5 * ph)));
            b.setSample (0, k, v); b.setSample (1, k, v * 0.97f);
        }
    return b;
}
static bool bad (const juce::AudioBuffer<float>& b) { for (int c = 0; c < b.getNumChannels(); ++c) for (int i = 0; i < b.getNumSamples(); ++i) if (! std::isfinite (b.getSample (c, i))) return true; return false; }

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    const juce::String mode = argc > 1 ? argv[1] : "test";
    const double sr = 48000; const int B = 512;
    if (mode == "snap")
    {
        Processor p; p.prepareToPlay (sr, B);
        if (! std::getenv ("JEMPTY")) { auto ph = phrase (sr); p.setSource (std::move (ph), sr, true); }
        if (auto* e = std::getenv ("JPRESET")) p.loadPreset (std::atoi (e));
        p.renderNow();
        Editor ed (p); ed.setSize (Editor::W, Editor::H);
        if (std::getenv ("JPLAY")) { p.play(); juce::AudioBuffer<float> b (2, B); juce::MidiBuffer m; for (int k = 0; k < 60; ++k) { b.clear(); p.processBlock (b, m); } ed.lvl = 0.6f; }
        for (int k = 0; k < 3; ++k) { ed.screen.gen = -1; ed.screen.refresh(); }
        if (std::getenv ("JBROWSE")) { ed.browser.open(); ed.browser.finishBoot(); }
        auto img = ed.createComponentSnapshot (ed.getLocalBounds(), true, std::getenv ("JX2") ? 2.0f : 1.0f);
        juce::File f (argc > 2 ? argv[2] : "/tmp/tag.png"); f.deleteFile(); juce::FileOutputStream os (f); juce::PNGImageFormat().writeImageToStream (img, os);
        std::printf ("snap %s\n", f.getFullPathName().toRawUTF8());
        return 0;
    }
    int fails = 0;
    {   // partenza neutra: l'ingresso esce identico, nessun tag
        Processor p; p.prepareToPlay (sr, B);
        juce::AudioBuffer<float> b (2, B); juce::MidiBuffer m; juce::Random r (1); double err = 0;
        for (int k = 0; k < 50; ++k)
        {
            for (int c = 0; c < 2; ++c) for (int i = 0; i < B; ++i) b.setSample (c, i, r.nextFloat() - 0.5f);
            juce::AudioBuffer<float> in; in.makeCopyOf (b);
            if (k == 10) m.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100), 10);
            p.processBlock (b, m); m.clear();
            for (int c = 0; c < 2; ++c) for (int i = 0; i < B; ++i) err = std::max (err, (double) std::abs (b.getSample (c, i) - in.getSample (c, i)));
        }
        std::printf ("neutro: differenza max %.2g %s\n", err, err < 1e-6 ? "OK" : "ERRORE"); if (err >= 1e-6) ++fails;
    }
    {   // ogni preset: tag valido, senza NaN, picco sotto 0 dB, durata sensata
        Processor p; p.prepareToPlay (sr, B);
        auto ph = phrase (sr); p.setSource (std::move (ph), sr, true);
        for (int i = 0; i < p.getNumPrograms(); ++i)
        {
            p.loadPreset (i);
            const auto t0 = juce::Time::getMillisecondCounterHiRes(); p.renderNow(); const double ms = juce::Time::getMillisecondCounterHiRes() - t0;
            std::vector<float> s, t; float ts = 0, ss = 0; p.waveSnapshot (s, t, 200, ts, ss);
            // suona il tag con una nota e misura
            juce::AudioBuffer<float> b (2, B); juce::MidiBuffer m; m.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 127), 0);
            float pk = 0; double e = 0; bool nan = false; int jumps = 0; float prev = 0;
            for (int k = 0; k < (int) (sr * (ts + 0.5) / B); ++k)
            {
                b.clear(); p.processBlock (b, m); m.clear(); if (bad (b)) nan = true;
                for (int q = 0; q < B; ++q) { const float v = b.getSample (0, q); pk = std::max (pk, std::abs (v)); e += v * v; if (std::abs (v - prev) > 0.6f) ++jumps; prev = v; }
            }
            const bool ok = ! nan && pk > 0.05f && pk <= 1.0f && ts > 0.2f && ts < 14.0f && (jumps < 3 || p.value ("crush") > 0.5f);
            std::printf ("%-20s frase %.2fs tag %.2fs picco %.2f salti %d forgiatura %.0f ms %s\n", p.getProgramName (i).toRawUTF8(), ss, ts, pk, jumps, ms, ok ? "OK" : "ERRORE");
            if (! ok) ++fails;
        }
    }
    {   // registrazione dalla traccia: la frase si ritaglia da sola
        Processor p; p.prepareToPlay (sr, B);
        auto ph = phrase (sr); juce::AudioBuffer<float> b (2, B); juce::MidiBuffer m;
        p.startRec();
        for (int k = 0; k * B < ph.getNumSamples(); ++k) { for (int c = 0; c < 2; ++c) b.copyFrom (c, 0, ph, c, k * B, juce::jmin (B, ph.getNumSamples() - k * B)); p.processBlock (b, m); }
        p.stopRec(); p.renderNow();
        std::vector<float> s, t; float ts = 0, ss = 0; p.waveSnapshot (s, t, 100, ts, ss);
        const bool ok = ss > 0.9f && ss < 1.3f;
        std::printf ("registrazione: frase ritagliata %.2f s %s\n", ss, ok ? "OK" : "ERRORE"); if (! ok) ++fails;
        // stato: la frase viaggia col progetto
        juce::MemoryBlock mb; p.getStateInformation (mb);
        Processor q; q.prepareToPlay (sr, B); q.setStateInformation (mb.getData(), (int) mb.getSize());
        std::vector<float> s2, t2; float ts2 = 0, ss2 = 0; q.waveSnapshot (s2, t2, 100, ts2, ss2);
        const bool ok2 = std::abs (ss2 - ss) < 0.01f && std::abs (ts2 - ts) < 0.01f;
        std::printf ("stato: frase %.2f s tag %.2f s (%d KB) %s\n", ss2, ts2, (int) (mb.getSize() / 1024), ok2 ? "OK" : "ERRORE"); if (! ok2) ++fails;
        // esportazione
        const auto f = q.exportTag(); const bool ok3 = f.existsAsFile() && f.getSize() > 1000;
        std::printf ("esporta: %s %s\n", f.getFileName().toRawUTF8(), ok3 ? "OK" : "ERRORE"); if (! ok3) ++fails; f.deleteFile();
    }
    {   // ripartenza mentre suona: nessun click
        Processor p; p.prepareToPlay (sr, B); auto ph = phrase (sr); p.setSource (std::move (ph), sr, true); p.loadPreset (1); p.renderNow();
        juce::AudioBuffer<float> b (2, B); juce::MidiBuffer m; float prev = 0, mx = 0;
        for (int k = 0; k < 200; ++k) { if (k % 23 == 0) m.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 120), 77); b.clear(); p.processBlock (b, m); m.clear();
            for (int q = 0; q < B; ++q) { mx = std::max (mx, std::abs (b.getSample (0, q) - prev)); prev = b.getSample (0, q); } }
        std::printf ("ripartenza: salto massimo tra campioni %.3f %s\n", mx, mx < 0.35f ? "OK" : "ERRORE"); if (mx >= 0.35f) ++fails;
    }
    std::printf (fails == 0 ? "TUTTO OK\n" : "ERRORI: %d\n", fails);
    return fails;
}
