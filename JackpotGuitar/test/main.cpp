#include "../Source/GtrProcessor.h"
#include "../Source/GtrEditor.h"
#include <cstdio>
#include <juce_dsp/juce_dsp.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <chrono>
using namespace gtr;

struct Head : juce::AudioPlayHead
{
    double sr = 48000, bpm = 120; juce::int64 pos = 0; bool playing = true;
    juce::Optional<PositionInfo> getPosition() const override
    {
        PositionInfo p; p.setIsPlaying (playing); p.setBpm (bpm); p.setPpqPosition (pos / sr * bpm / 60.0); p.setTimeInSamples (pos); return p;
    }
};
static std::unique_ptr<Processor> mk (int preset)
{
    auto p = std::make_unique<Processor>(); if (preset >= 0) p->loadPreset (preset); p->setPlayConfigDetails (0, 2, 48000, 512); p->prepareToPlay (48000, 512); return p;
}
struct Run { std::vector<float> L, R; std::vector<std::pair<juce::int64, juce::String>> midi; double cpu = 0; };
// suona per len secondi; ogni blocco puo' chiamare fn (per cambiare parametri a meta')
static Run run (Processor& p, double len, int blk, bool uneven = false, std::function<void (juce::int64)> fn = {}, double bpm = 120)
{
    Head h; h.bpm = bpm; p.setPlayHead (&h);
    Run r; const juce::int64 N = (juce::int64) (len * 48000); r.L.resize ((size_t) N); r.R.resize ((size_t) N);
    juce::AudioBuffer<float> b (2, 4096); juce::MidiBuffer m; juce::Random rnd (5);
    double t = 0;
    while (h.pos < N)
    {
        int n = uneven ? 1 + rnd.nextInt (blk) : blk; n = (int) juce::jmin ((juce::int64) n, N - h.pos);
        if (fn) fn (h.pos);
        b.setSize (2, n, false, false, true); m.clear();
        const auto t0 = std::chrono::high_resolution_clock::now();
        p.processBlock (b, m);
        t += std::chrono::duration<double> (std::chrono::high_resolution_clock::now() - t0).count();
        for (int i = 0; i < n; ++i) { r.L[(size_t) (h.pos + i)] = b.getSample (0, i); r.R[(size_t) (h.pos + i)] = b.getSample (1, i); }
        for (const auto mm : m) { const auto& msg = mm.getMessage(); r.midi.push_back ({ h.pos + mm.samplePosition, msg.getDescription() }); }
        h.pos += n;
    }
    r.cpu = t / len * 100.0; p.setPlayHead (nullptr);
    return r;
}
static float peakOf (const std::vector<float>& x) { float p = 0; for (float v : x) p = std::max (p, std::abs (v)); return p; }
static bool finite (const std::vector<float>& x) { for (float v : x) if (! std::isfinite (v)) return false; return true; }
// click: picco della derivata seconda rispetto al livello del segnale vicino
static float clickScore (const std::vector<float>& x, size_t a, size_t b)
{
    float mx = 0; for (size_t i = std::max<size_t> (a, 2); i < std::min (b, x.size()); ++i) mx = std::max (mx, std::abs (x[i] - 2 * x[i - 1] + x[i - 2]));
    return mx;
}

static int genTest()
{
    int total = 0, fail = 0, out = 0, monoV = 0, span = 0, outB = 0, neg = 0; const auto t0 = std::chrono::high_resolution_clock::now();
    for (int key = 0; key < 12; ++key) for (int sc = 0; sc < kScales; ++sc) for (int g = 0; g < kGenres; ++g) for (int mode = 0; mode < kModes; ++mode)
    {
        const int np = patternNames (mode).size();
        for (int pat = 0; pat < np; ++pat)
        {
            Settings S; S.key = key; S.scale = sc; S.genre = g; S.mode = mode; S.pattern = pat; S.vstyle = (key + pat) % kVStyles; S.tuning = (g + pat) % kTunings;
            S.seed = (juce::uint32) (key * 1000 + g * 31 + pat + 1); const auto& pl = progressions (g); S.prog = pl[(size_t) ((key + sc) % (int) pl.size())];
            S.pos = (key * 3 + pat) % 11; S.density = ((key + pat) % 5) / 4.0f; S.tech = 0.8f; S.emb = 0.6f; S.swing = (pat % 3) * 0.3f; S.human = 0.6f;
            S.borrow = (key % 3 == 0) ? 0.8f : 0.0f; S.bars = (g % 2) ? 4 : 2; S.perBar = (key % 4 == 0) ? 2 : 1;
            auto P = generate (S); auto sset = scaleSet (key, sc); auto tun = tuning (S.tuning);
            for (size_t i = 0; i < P.voic.size(); ++i)
            {
                ++total; const auto& v = P.voic[i]; if (v.score < -1e8 || v.count() < 2) ++fail;
                int mn = 99, mx = 0; for (int f : v.fret) if (f > 0) { mn = std::min (mn, f); mx = std::max (mx, f); } if (mx > 0 && mx - mn > 3) ++span;
            }
            for (auto& e : P.ev)
            {
                bool okN = sset[(size_t) (e.note % 12)];
                if (! okN) { const auto& c = P.chords[(size_t) e.slot]; for (int t : c.tones) okN |= t == e.note % 12; if (! c.borrowed) okN = false; }
                if (! okN) { ++out; if (out < 6) std::printf ("FUORI SCALA k%d %s %s %s pat%d nota %d borrow %.1f\n", key, scaleName (sc), genreName (g), modeName (mode), pat, e.note, S.borrow); }
                if (S.borrow == 0 && ! sset[(size_t) (e.note % 12)]) ++outB;
                if (e.beat < 0 || e.beat + e.len > P.len() + 1e-6 || e.len <= 0) ++neg;
                if (e.note != tun[(size_t) e.str] + e.fret) ++neg;
            }
            for (size_t a = 0; a < P.ev.size(); ++a) for (size_t b = a + 1; b < P.ev.size(); ++b)
            {
                const auto& x = P.ev[a]; const auto& y = P.ev[b];
                if (x.str == y.str && y.beat > x.beat + 1e-9 && y.beat < x.beat + x.len - 0.035) ++monoV;
            }
            // i bend: dentro +-2 semitoni, e sopra un bend la nota d'arrivo e' nella scala
            for (auto& b : P.bend) if (std::abs (b.semis) > 2.3f) ++neg;
        }
    }
    const double el = std::chrono::duration<double> (std::chrono::high_resolution_clock::now() - t0).count();
    std::printf ("GEN: voicing %d senza voicing %d apertura>4 tasti %d | note fuori scala %d (con interscambio spento %d) | corda doppia %d | errori %d | %.1f s\n", total, fail, span, out, outB, monoV, neg, el);
    return fail + span + out + outB + monoV + neg;
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gi;
    const juce::String mode = argc > 1 ? argv[1] : "all";
    int bad = 0;
    if (mode == "gen" || mode == "all") bad += genTest();
    if (mode == "render" || mode == "all")
    {
        for (int i = 0; i < (int) presets().size(); ++i)
        {
            auto p = mk (i); auto r = run (*p, 8.0, 512);
            auto r2 = run (*mk (i), 8.0, 300, true);
            std::sort (r.midi.begin(), r.midi.end()); std::sort (r2.midi.begin(), r2.midi.end());
            int notes = 0; for (auto& m : r.midi) notes += m.second.startsWith ("Note on");
            bool same = r.midi.size() == r2.midi.size(); if (same) for (size_t k = 0; k < r.midi.size(); ++k) same &= r.midi[k] == r2.midi[k];
            if (! same && std::getenv ("JDBG")) { for (size_t k = 0; k < std::min (r.midi.size(), r2.midi.size()); ++k) if (r.midi[k] != r2.midi[k]) { for (size_t q = k > 2 ? k - 2 : 0; q < k + 3 && q < r.midi.size(); ++q) std::printf ("  A %lld %s | B %lld %s\n", (long long) r.midi[q].first, r.midi[q].second.toRawUTF8(), (long long) r2.midi[q].first, r2.midi[q].second.toRawUTF8()); break; } std::printf ("  sizes %zu %zu\n", r.midi.size(), r2.midi.size()); }
            float diff = 0; for (size_t k = 0; k < r.L.size(); ++k) diff = std::max (diff, std::abs (r.L[k] - r2.L[k]));
            const float pk = std::max (peakOf (r.L), peakOf (r.R));
            const bool ok = finite (r.L) && finite (r.R) && pk < 1.0f && pk > 0.02f && same && notes > 0;
            if (! ok) ++bad;
            std::printf ("%-34s note %4d  picco %5.1f dB  cpu %5.2f%%  MIDI uguale a blocchi diversi %s  audio diff %.1e %s\n", presets()[(size_t) i].name, notes, juce::Decibels::gainToDecibels (pk), r.cpu, same ? "SI" : "NO", diff, ok ? "" : "  <-- !!");
        }
    }
    if (mode == "clicks" || mode == "all")
    {
        // salti dei parametri del suono a meta' di una nota tenuta
        const char* ids[] = { "tone", "bodyAmt", "drive", "space", "width", "vol", "body", "listen" };
        for (auto* id : ids)
        {
            auto p = mk (1);
            auto base = run (*p, 3.0, 256);
            auto p2 = mk (1);
            auto r = run (*p2, 3.0, 256, false, [&] (juce::int64 pos) { if (pos >= 48000 && pos < 48256) { auto* prm = p2->apvts.getParameter (id); prm->setValueNotifyingHost (prm->getValue() > 0.5f ? 0.0f : 1.0f); } });
            const float b0 = clickScore (base.L, 47000, 52000), c0 = clickScore (r.L, 47000, 52000), ref = clickScore (base.L, 0, base.L.size());
            const bool ok = c0 <= std::max (ref, b0) * 1.6f + 1e-4f;
            if (! ok) ++bad;
            std::printf ("click %-8s  salto %.4f  stesso punto senza salto %.4f  massimo normale %.4f  %s\n", id, c0, b0, ref, ok ? "ok" : "<-- CLICK");
        }
        {   // cambio di giro, di modo e di accordo mentre suona
            auto p = mk (1); auto r = run (*p, 6.0, 512, false, [&] (juce::int64 pos) { if (pos == 512 * 150) { p->setV ("mode", RITMICA); p->regenerate(); } if (pos == 512 * 300) { p->generaGiro(); p->regenerate(); } if (pos == 512 * 420) { p->setV ("key", 2); p->regenerate(); } });
            std::printf ("cambi in corsa: picco %.1f dB, finito %s\n", juce::Decibels::gainToDecibels (std::max (peakOf (r.L), peakOf (r.R))), finite (r.L) ? "si" : "NO");
        }
    }
    if (mode == "midi" || mode == "all")
    {
        auto p = mk (3); p->uiBpm = 140; auto f = p->exportMidi();
        juce::FileInputStream in (f); juce::MidiFile mf; const bool rd = mf.readFrom (in);
        int on = 0, pw = 0; double last = 0;
        if (rd) for (int t = 0; t < mf.getNumTracks(); ++t) for (auto* e : *mf.getTrack (t)) { on += e->message.isNoteOn(); pw += e->message.isPitchWheel(); last = std::max (last, e->message.getTimeStamp()); }
        std::printf ("MIDI: %s letto %s, note %d, pitch bend %d, fine a %.2f battiti\n", f.getFileName().toRawUTF8(), rd ? "si" : "NO", on, pw, last / 960.0);
        if (! rd || on == 0) ++bad;
    }
    if (mode == "cpu" || mode == "all")
    {
        for (int body = 0; body < kBodies; ++body)
        {
            auto p = mk (13); p->setV ("body", (float) body); p->setV ("density", 1.0f); p->regenerate();
            auto r = run (*p, 10.0, 128); std::printf ("cpu %-9s %5.2f%% (blocchi da 128, pop acustica piena)\n", bodyName (body), r.cpu);
        }
    }
    if (mode == "alias")
    {   // CRUNCH: nota acuta, energia non armonica
        auto p = mk (-1); p->setV ("body", CRUNCH); p->setV ("drive", 1.0f); p->setV ("live", 0); p->setV ("space", 0); p->setV ("width", 0);
        juce::AudioBuffer<float> b (2, 48000 * 2); juce::MidiBuffer m; m.addEvent (juce::MidiMessage::noteOn (1, 88, (juce::uint8) 120), 0);
        p->processBlock (b, m);
        std::vector<float> x ((size_t) 32768); for (size_t i = 0; i < x.size(); ++i) x[i] = b.getSample (0, (int) i + 4800) * (0.5f - 0.5f * std::cos (6.2831853f * i / 32767.0f));
        juce::dsp::FFT fft (15); std::vector<float> d (65536, 0); std::copy (x.begin(), x.end(), d.begin()); fft.performFrequencyOnlyForwardTransform (d.data());
        const double f0 = 440 * std::pow (2.0, (88 - 69) / 12.0); double harm = 0, rest = 0;
        for (int k = 4; k < 16384; ++k) { const double f = k * 48000.0 / 32768; const double h = f / f0; const double e = d[(size_t) k] * d[(size_t) k]; if (std::abs (h - std::round (h)) < 0.03) harm += e; else rest += e; }
        std::printf ("alias CRUNCH nota %d: non armonico %.1f dB sotto le armoniche\n", 88, 10 * std::log10 (harm / (rest + 1e-12)));
    }
    if (mode == "snap")
    {
        auto p = mk (std::getenv ("JPRESET") ? std::atoi (std::getenv ("JPRESET")) : 1);
        run (*p, 1.3, 512);
        if (auto* t = std::getenv ("JTHEME")) skin::cur() = std::atoi (t);
        Editor ed (*p); if (std::getenv ("JBROWSE")) ed.browser.open();
        auto img = ed.createComponentSnapshot (ed.getLocalBounds(), true, 1.0f);
        juce::File f (argc > 2 ? argv[2] : "/tmp/gtr.png"); f.deleteFile(); juce::FileOutputStream os (f); juce::PNGImageFormat().writeImageToStream (img, os);
        std::printf ("snap %s\n", f.getFullPathName().toRawUTF8());
    }
    if (mode == "wav")
    {   // ascolto: un preset su file
        const int pi = argc > 2 ? std::atoi (argv[2]) : 1; auto p = mk (pi); auto r = run (*p, 8.0, 512, false, {}, argc > 4 ? std::atof (argv[4]) : 120.0);
        juce::File f (argc > 3 ? argv[3] : "/tmp/gtr.wav"); f.deleteFile();
        juce::WavAudioFormat w; std::unique_ptr<juce::AudioFormatWriter> wr (w.createWriterFor (new juce::FileOutputStream (f), 48000, 2, 24, {}, 0));
        juce::AudioBuffer<float> b (2, (int) r.L.size()); for (size_t i = 0; i < r.L.size(); ++i) { b.setSample (0, (int) i, r.L[i]); b.setSample (1, (int) i, r.R[i]); }
        wr->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
    }
    std::printf (bad ? "\nPROBLEMI: %d\n" : "\nTUTTO OK\n", bad);
    return bad ? 1 : 0;
}
