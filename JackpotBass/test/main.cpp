#include "../Source/BassProcessor.h"
#include "../Source/BassEditor.h"
#include <juce_dsp/juce_dsp.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <cstdio>
#include <chrono>
using namespace bss;

struct Head : juce::AudioPlayHead
{
    double sr = 48000, bpm = 140; juce::int64 pos = 0; bool playing = true;
    juce::Optional<PositionInfo> getPosition() const override { PositionInfo p; p.setIsPlaying (playing); p.setBpm (bpm); p.setPpqPosition (pos / sr * bpm / 60.0); p.setTimeInSamples (pos); return p; }
};
static std::unique_ptr<Processor> mk (int preset)
{
    auto p = std::make_unique<Processor>(); if (preset >= 0) p->loadPreset (preset); p->setPlayConfigDetails (0, 2, 48000, 512); p->prepareToPlay (48000, 512); return p;
}
struct Run { std::vector<float> L; std::vector<std::pair<juce::int64, juce::String>> midi; double cpu = 0; };
static Run run (Processor& p, double len, int blk, bool uneven = false)
{
    Head h; p.setPlayHead (&h); Run r; const juce::int64 N = (juce::int64) (len * 48000); r.L.resize ((size_t) N);
    juce::AudioBuffer<float> b (2, 4096); juce::MidiBuffer m; juce::Random rnd (5); double t = 0;
    while (h.pos < N)
    {
        int n = uneven ? 1 + rnd.nextInt (blk) : blk; n = (int) juce::jmin ((juce::int64) n, N - h.pos);
        b.setSize (2, n, false, false, true); m.clear();
        const auto t0 = std::chrono::high_resolution_clock::now(); p.processBlock (b, m); t += std::chrono::duration<double> (std::chrono::high_resolution_clock::now() - t0).count();
        for (int i = 0; i < n; ++i) r.L[(size_t) (h.pos + i)] = b.getSample (0, i);
        for (const auto mm : m) r.midi.push_back ({ h.pos + mm.samplePosition, mm.getMessage().getDescription() });
        h.pos += n;
    }
    r.cpu = t / len * 100.0; p.setPlayHead (nullptr); return r;
}
static float peakOf (const std::vector<float>& x) { float p = 0; for (float v : x) p = std::max (p, std::abs (v)); return p; }
static bool finite (const std::vector<float>& x) { for (float v : x) if (! std::isfinite (v)) return false; return true; }
static float d2 (const std::vector<float>& x, size_t a, size_t b) { float mx = 0; for (size_t i = std::max<size_t> (a, 2); i < std::min (b, x.size()); ++i) mx = std::max (mx, std::abs (x[i] - 2 * x[i - 1] + x[i - 2])); return mx; }

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gi;
    const juce::String mode = argc > 1 ? argv[1] : "all";
    int bad = 0;
    if (mode == "gen" || mode == "all")
    {   // scala, monofonia, glide, range, per tutte le combinazioni
        long n = 0, out = 0, over = 0, range = 0, glides = 0;
        for (int k = 0; k < 12; ++k) for (int sc = 0; sc < kScales; ++sc) for (int md = 0; md < kBModes; ++md) for (int f = 0; f < kBFigs; ++f) for (int r = 0; r < kBRits; r += 2)
        {
            BSettings S; S.key = k; S.scale = sc; S.mode = md; S.fig = f; S.rit = r; S.seed = (juce::uint32) (k * 131 + f * 7 + r + md); S.prog = progressions ((k + r) % kGenres)[0];
            S.approach = 0.7f; S.ghost = 0.5f; S.glide = 0.6f; S.bars = (k % 2) ? 8 : 4; S.density = (float) ((f + r) % 11) / 10.0f; S.borrow = sc == 0 && k % 3 == 0 ? 0.7f : 0.0f; S.octave = (k % 3) - 1;
            S.perBar = k % 4 == 0 ? 2 : k % 4 == 1 ? 0 : 1; if (auto* vv = std::getenv ("JVARY")) S.vary = (float) std::atof (vv); S.space = (r % 4) * 0.3f; S.mood = (k + f) % kMoods; S.figSeq = "R ~8 3 ^ L 7"; S.rot = f % 3;
            auto P = generateBass (S); const auto ss = scaleSet (k, sc);
            for (size_t i = 0; i < P.ev.size(); ++i)
            {
                const auto& e = P.ev[i]; ++n;
                bool okN = ss[(size_t) (e.note % 12)]; if (! okN && P.chords[(size_t) e.slot].borrowed) for (int t : P.chords[(size_t) e.slot].tones) okN |= t == e.note % 12;
                if (! okN) { ++out; if (out < 5) std::printf ("FUORI SCALA k%d %s fig %s nota %d\n", k, scaleName (sc), bfigOf (f).name, e.note); }
                if (e.note < 12 || e.note > 84) ++range;
                if (e.art == 2) ++glides;
                if (i + 1 < P.ev.size()) { const double ov = e.beat + e.len - P.ev[i + 1].beat; if (ov > (P.ev[i + 1].art == 2 ? 0.07 : 0.001)) { ++over; if (std::getenv ("JDBG") && over < 6) std::printf ("OV %.3f+%.3f art%d -> %.3f art%d\n", e.beat, e.len, e.art, P.ev[i + 1].beat, P.ev[i + 1].art); } }
                if (e.beat < 0 || e.beat + e.len > P.len() + 1e-6) { ++over; if (std::getenv ("JDBG") && over < 6) std::printf ("FUORI %.4f+%.4f L %.1f\n", e.beat, e.len, P.len()); }
            }
        }
        std::printf ("GEN: %ld note, fuori scala %ld, sovrapposizioni non volute %ld, fuori registro %ld, glide %ld\n", n, out, over, range, glides);
        bad += (int) (out + over + range);
        for (int r : { 0, 6, 8, 10 })
        {
            std::printf ("DENSITA' %-15s", britOf (r).name);
            for (int d = 0; d <= 10; ++d) { BSettings S; S.rit = r; S.density = d / 10.0f; S.prog = { 0, 5, 2, 6 }; S.ghost = 0; S.mode = BASSO; auto P = generateBass (S); std::printf (" %3d", (int) P.ev.size()); }
            std::printf ("\n");
        }
    }
    if (mode == "preview" || mode == "all")
    {   // PROVA a trasporto fermo: deve suonare (tutti e tre i motori), anche senza playhead
        for (int md = 0; md < kBModes; ++md)
            for (int withHead = 0; withHead < 2; ++withHead)
            {
                auto p = mk (-1); p->setV ("mode", (float) md); p->setV ("play", 1); p->regenerate();
                Head h; h.playing = false; if (withHead) p->setPlayHead (&h);
                juce::AudioBuffer<float> b (2, 512); juce::MidiBuffer m; float pk = 0; int non = 0;
                for (int k = 0; k < 400; ++k) { m.clear(); p->processBlock (b, m); for (int i = 0; i < 512; ++i) pk = std::max (pk, std::abs (b.getSample (0, i))); for (const auto mm : m) non += mm.getMessage().isNoteOn(); }
                p->setPlayHead (nullptr);
                std::printf ("PROVA %s %s: picco %.3f note MIDI %d %s\n", bmodeName (md), withHead ? "playhead fermo" : "senza playhead", pk, non, pk > 0.02f ? "" : "  <-- !!");
                if (pk <= 0.02f) ++bad;
            }
    }
    if (mode == "render" || mode == "all")
    {
        for (int i = 0; i < (int) presets().size(); ++i)
        {
            auto r = run (*mk (i), 8.0, 512), r2 = run (*mk (i), 8.0, 300, true);
            std::sort (r.midi.begin(), r.midi.end()); std::sort (r2.midi.begin(), r2.midi.end());
            int notes = 0; for (auto& m : r.midi) notes += m.second.startsWith ("Note on");
            const bool same = r.midi == r2.midi; float diff = 0; for (size_t k = 0; k < r.L.size(); ++k) diff = std::max (diff, std::abs (r.L[k] - r2.L[k]));
            const float pk = peakOf (r.L);
            const bool ok = finite (r.L) && pk < 1.0f && pk > 0.02f && same && notes > 0 && diff < 1e-3f;
            if (! ok) ++bad;
            std::printf ("%-26s note %3d  picco %5.1f dB  cpu %5.2f%%  MIDI uguale %s  audio diff %.1e %s\n", presets()[(size_t) i].name, notes, juce::Decibels::gainToDecibels (pk), r.cpu, same ? "SI" : "NO", diff, ok ? "" : "  <-- !!");
        }
    }
    if (mode == "clicks" || mode == "all")
    {   // nota tenuta per ogni motore, salto dei parametri del suono a 1 secondo
        for (int md = 0; md < kBModes; ++md)
            for (auto* id : { "tone", "drive", "decay", "punch", "vol", "listen" })
            {
                auto hold = [&] (bool jump)
                {
                    auto p = mk (-1); p->setV ("live", 0); p->setV ("mode", (float) md); p->setV ("decay", 1.0f);
                    juce::AudioBuffer<float> b (2, 256); std::vector<float> o;
                    for (int blk = 0; blk < 2 * 48000 / 256; ++blk)
                    {
                        juce::MidiBuffer m; if (blk == 0) m.addEvent (juce::MidiMessage::noteOn (1, 36, (juce::uint8) 100), 0);
                        if (jump && blk == 48000 / 256) { auto* prm = p->apvts.getParameter (id); prm->setValueNotifyingHost (prm->getValue() > 0.5f ? 0.0f : 1.0f); }
                        p->processBlock (b, m); for (int i = 0; i < 256; ++i) o.push_back (b.getSample (0, i));
                    }
                    return o;
                };
                const auto r = hold (true);
                // un click e' un picco della derivata seconda nel punto del salto piu' alto del suono stabile prima e dopo
                const float c0 = d2 (r, 47900, 52000), cb = std::max (d2 (r, 30000, 46000), d2 (r, 52000, 60000));
                const bool ok = c0 <= cb * 1.15f + 1e-5f; if (! ok) ++bad;
                if (! ok && std::getenv ("JDBG")) { for (size_t w = 44000; w < 64000; w += 1000) std::printf ("   %zu %.5f\n", w, d2 (r, w, w + 1000)); }
                std::printf ("click %-6s %-7s scatto %.5f  suono %.5f %s\n", bmodeName (md), id, c0, cb, ok ? "ok" : "<-- CLICK");
            }
        // glide e note ribattute nel 808: niente salti
        auto p = mk (3); auto r = run (*p, 6.0, 256);
        float rms = 0; for (float x : r.L) rms += x * x; rms = std::sqrt (rms / (float) r.L.size());
        std::printf ("808 drill slide: picco %.1f dB, rms %.1f dB, derivata seconda massima %.4f\n", juce::Decibels::gainToDecibels (peakOf (r.L)), juce::Decibels::gainToDecibels (rms), d2 (r.L, 0, r.L.size()));
    }
    if (mode == "alias" || mode == "all")
    {   // synth con drive e risonanza su una nota acuta: energia fuori dalle armoniche
        for (int md : { B808, SYNTH })
        {
            auto p = mk (-1); p->setV ("live", 0); p->setV ("mode", (float) md); p->setV ("drive", 1.0f); p->setV ("tone", 1.0f); p->setV ("punch", 0.0f);
            juce::AudioBuffer<float> b (2, 48000); juce::MidiBuffer m; m.addEvent (juce::MidiMessage::noteOn (1, md == B808 ? 48 : 60, (juce::uint8) 100), 0);
            p->processBlock (b, m);
            std::vector<float> d (65536, 0); for (int i = 0; i < 32768; ++i) d[(size_t) i] = b.getSample (0, i + 9600) * (0.5f - 0.5f * std::cos (6.2831853f * i / 32767.0f));
            juce::dsp::FFT fft (15); fft.performFrequencyOnlyForwardTransform (d.data());
            const double f0 = 440 * std::pow (2.0, ((md == B808 ? 48 : 60) - 69) / 12.0) * (md == SYNTH ? 0.5 : 1.0); double harm = 0, rest = 0;
            for (int k = 4; k < 16384; ++k) { const double f = k * 48000.0 / 32768, h = f / f0, e = d[(size_t) k] * d[(size_t) k]; if (std::abs (h - std::round (h)) < 0.03) harm += e; else rest += e; }
            std::printf ("alias %s: non armonico %.1f dB sotto\n", bmodeName (md), 10 * std::log10 (harm / (rest + 1e-12)));
        }
    }
    if (mode == "edit" || mode == "all")
    {
        auto p = mk (1); auto P = *p->part(); const size_t n0 = P.ev.size();
        P.ev.erase (P.ev.begin()); p->commitEdit (P); p->regenerate();
        const bool kept = p->edited() && p->part()->ev.size() == n0 - 1;
        juce::MemoryBlock mb; p->getStateInformation (mb); auto q = mk (-1); q->setStateInformation (mb.getData(), (int) mb.getSize());
        const bool saved = q->edited() && q->part()->ev.size() == n0 - 1;
        p->undo.beginNewTransaction(); p->setProg ({ 0, 3, 4, 0 }); p->regenerate(); const bool gone = ! p->edited();
        p->undo.undo(); p->regenerate(); const bool back = p->edited();
        std::printf ("MODIFICHE: restano %s, salvate %s, via se cambi il giro %s, tornano con annulla %s\n", kept ? "si" : "NO", saved ? "si" : "NO", gone ? "si" : "NO", back ? "si" : "NO");
        if (! (kept && saved && gone && back)) ++bad;
        auto f = p->exportMidi(); juce::FileInputStream in (f); juce::MidiFile mf; const bool rd = mf.readFrom (in);
        std::printf ("MIDI: %s letto %s\n", f.getFileName().toRawUTF8(), rd ? "si" : "NO"); if (! rd) ++bad;
    }
    if (mode == "snap")
    {
        auto p = mk (std::getenv ("JPRESET") ? std::atoi (std::getenv ("JPRESET")) : 2);
        run (*p, 1.3, 512);
        if (auto* t = std::getenv ("JTHEME")) skin::cur() = std::atoi (t);
        Editor ed (*p); if (std::getenv ("JBROWSE")) ed.browser.open(); if (std::getenv ("JGRID")) ed.gridEdit = true;
        auto img = ed.createComponentSnapshot (ed.getLocalBounds(), true, 1.0f);
        juce::File f (argc > 2 ? argv[2] : "/tmp/bass.png"); f.deleteFile(); juce::FileOutputStream os (f); juce::PNGImageFormat().writeImageToStream (img, os);
    }
    std::printf (bad ? "\nPROBLEMI: %d\n" : "\nTUTTO OK\n", bad);
    return bad ? 1 : 0;
}
