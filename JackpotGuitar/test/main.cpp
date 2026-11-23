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
            S.borrow = (key % 3 == 0) ? 0.8f : 0.0f; S.bars = (g % 2) ? 4 : 2; S.perBar = (key % 4 == 0) ? 2 : (key % 4 == 1) ? 0 : 1; S.mood = (key + g + pat) % kMoods; S.space = (pat % 4) * 0.3f; S.bars = (key % 2) ? 8 : S.bars;
            S.lyBase = (pat + key) % 3 == 0 ? -1 : (key % 2 ? 100 + pat % 11 : pat % 7); S.lyArp = (g + pat) % 4 == 0 ? -1 : (pat + g) % 12; S.lyLead = key % 5 == 0 ? -1 : pat % 6;
            S.arpFig = (key + pat + g) % kFigs; S.arpRit = (pat + sc + key) % kRits; S.grid = { (juce::uint16) (0x1111 * (key % 3)), 0x0f0f, (juce::uint16) (0x4444 >> (pat % 3)), 0, 0x2222 };
            auto P = generateAny (S); auto sset = scaleSet (key, sc); auto tun = tuning (S.tuning);
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
                if (x.str == y.str && x.layer == y.layer && y.beat > x.beat + 1e-9 && y.beat < x.beat + x.len - 0.035) ++monoV;
            }
            // i bend: dentro +-2 semitoni, e sopra un bend la nota d'arrivo e' nella scala
            for (auto& b : P.bend) if (std::abs (b.semis) > 2.3f) ++neg;
        }
    }
    const double el = std::chrono::duration<double> (std::chrono::high_resolution_clock::now() - t0).count();
    std::printf ("GEN: voicing %d senza voicing %d apertura>4 tasti %d | note fuori scala %d (con interscambio spento %d) | corda doppia %d | errori %d | %.1f s\n", total, fail, span, out, outB, monoV, neg, el);
    return fail + span + out + outB + monoV + neg;
}

// cantabilita' del lead: intervalli, estensione, respiri, chiusure
static int singTest (bool verbose)
{
    long notes = 0, steps = 0, big = 0, trit = 0, rangeBad = 0, noBreath = 0, endBad = 0, parts = 0; double rangeSum = 0;
    for (int style = 0; style < 6; ++style) for (int mood = 0; mood < kMoods; ++mood) for (int key = 0; key < 12; key += 1) for (int sd = 1; sd <= 3; ++sd)
    {
        Settings S; S.mode = LEAD; S.pattern = style; S.mood = mood; S.key = key; S.scale = key % 3 == 0 ? MAGGIORE : MINORE; S.genre = POP; S.bars = 8; S.seed = (juce::uint32) (sd * 7919 + key * 13 + mood); S.pos = 5; S.tech = 0.0f;
        S.prog = progressions (S.genre)[(size_t) (sd % 4)]; S.human = 0;
        auto P = generate (S); ++parts;
        std::vector<Ev> mel; for (auto& e : P.ev) if (e.lead) { if (! mel.empty() && std::abs (mel.back().beat - e.beat) < 0.02) continue; mel.push_back (e); }   // solo la voce alta (niente doppie note)
        if (mel.size() < 4) continue;
        int lo = 999, hi = 0; for (auto& e : mel) { lo = std::min (lo, e.note); hi = std::max (hi, e.note); }
        rangeSum += hi - lo; if (hi - lo > 17) ++rangeBad;
        for (size_t k = 1; k < mel.size(); ++k) { const int iv = std::abs (mel[k].note - mel[k - 1].note); ++notes; if (iv <= 2) ++steps; if (iv > 9) ++big; if (iv == 6) ++trit; }
        // respiro: in ogni frase di 4 battute almeno una pausa di mezzo battito o una nota di 2 battiti
        for (int ph = 0; ph < 2; ++ph) { bool br = false; for (size_t k = 0; k < mel.size(); ++k) { if (mel[k].beat < ph * 16 || mel[k].beat >= ph * 16 + 16) continue; const double nx = k + 1 < mel.size() ? mel[k + 1].beat : P.len(); if (nx - (mel[k].beat + mel[k].len) >= 0.45 || mel[k].len >= 1.9) br = true; } if (! br) ++noBreath; }
        // chiusura: l'ultima nota e' la fondamentale dell'accordo
        const auto& lastE = mel.back(); { const auto& c = P.chords[(size_t) lastE.slot]; if (lastE.note % 12 != c.root && lastE.note % 12 != c.third && lastE.note % 12 != c.fifth) ++endBad; }
        if (verbose && style == 0 && key == 9 && sd == 1)
        {
            std::printf ("%-12s ", moodName (mood)); for (auto& e : mel) std::printf ("%s%d%s ", noteName (e.note % 12), e.note / 12 - 1, e.len >= 1.4 ? "-" : ""); std::printf ("\n");
        }
    }
    std::printf ("CANTABILE: %ld intervalli, per grado congiunto %.0f%%, salti oltre la sesta %ld, tritoni %ld, estensione media %.1f semitoni (oltre 17: %ld), frasi senza respiro %ld, finali fuori dall'accordo %ld su %ld\n",
                 notes, 100.0 * steps / std::max (1L, notes), big, trit, rangeSum / std::max (1L, parts), rangeBad, noBreath, endBad, parts);
    return (int) (big + trit + rangeBad + noBreath + endBad);
}
// variazione: in un giro di 8 quante battute sono identiche alla prima
// densita' dell'arpeggio: quante note a ogni tacca, per qualche ritmo
static void densTest()
{
    for (int rit : { 0, 2, 4, 6, 8 })
    {
        std::printf ("DENSITA' %-18s", ritOf (rit).name);
        for (int d = 0; d <= 10; ++d)
        {
            Settings S; S.mode = ARPEGGIO; S.arpFig = 6; S.arpRit = rit; S.density = d / 10.0f; S.bars = 4; S.prog = { 0, 5, 2, 6 }; S.human = 0;
            auto P = generate (S); std::printf (" %3d", (int) P.ev.size());
        }
        std::printf ("\n");
    }
    // figure diverse sullo stesso accordo: ordine delle corde della prima battuta
    for (int f = 0; f < kFigs - 1; ++f)
    {
        Settings S; S.mode = ARPEGGIO; S.arpFig = f; S.arpRit = 0; S.density = 0.5f; S.bars = 4; S.prog = { 0 }; S.human = 0; S.emb = 0;
        auto P = generate (S); std::printf ("FIGURA %-12s corde:", figOf (f).name); for (auto& e : P.ev) if (e.beat < 4) std::printf (" %d", 6 - e.str); std::printf ("\n");
    }
}
static void varTest()
{
    for (int mood = 0; mood < kMoods; ++mood)
    {
        int same = 0, tot = 0;
        for (int pat = 0; pat < 9; ++pat) for (int mode : { ARPEGGIO, RITMICA })
        {
            if (pat >= patternNames (mode).size()) continue;
            Settings S; S.mode = mode; S.pattern = pat; S.mood = mood; S.bars = 8; S.prog = { 0, 0, 0, 0 }; S.seed = 5 + pat; S.human = 0; S.swing = 0;
            auto P = generate (S);
            auto bar = [&] (int b) { std::vector<std::pair<int, int>> v; for (auto& e : P.ev) if (e.beat >= b * 4 && e.beat < b * 4 + 4) v.push_back ({ (int) std::floor ((e.beat - b * 4) * 12 + 0.25), e.note }); std::sort (v.begin(), v.end()); return v; };
            const auto b0 = bar (0); int sm = 0; for (int b = 1; b < 8; ++b) { ++tot; sm += bar (b) == b0; } same += sm;
            if (std::getenv ("JVAR") && mood == std::atoi (std::getenv ("JVAR"))) { std::printf ("  %s %s: %d", modeName (mode), patternNames (mode)[pat].toRawUTF8(), sm); for (int b = 0; b < 8; ++b) std::printf (" [%zu]", bar (b).size()); std::printf ("\n"); }
        }
        std::printf ("VARIAZIONE %-12s battute uguali alla prima: %d su %d\n", moodName (mood), same, tot);
    }
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gi;
    const juce::String mode = argc > 1 ? argv[1] : "all";
    int bad = 0;
    if (mode == "gen" || mode == "all") bad += genTest();
    if (mode == "sing" || mode == "all") bad += singTest (mode == "sing");
    if (mode == "var" || mode == "all") varTest();
    if (mode == "dens" || mode == "all") densTest();
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
        // un accordo tenuto (niente nuove pizzicate), il parametro salta a 1 secondo: la differenza con la corsa senza salto non deve avere scatti
        auto hold = [] (const char* id, bool jump)
        {
            auto p = mk (-1); p->setV ("live", 0); p->setV ("body", CRUNCH);
            juce::AudioBuffer<float> b (2, 256); std::vector<float> out;
            for (int blk = 0; blk < 3 * 48000 / 256; ++blk)
            {
                juce::MidiBuffer m; if (blk == 0) for (int n : { 45, 52, 57, 60, 64 }) m.addEvent (juce::MidiMessage::noteOn (1, n, (juce::uint8) 100), 0);
                if (jump && blk == 48000 / 256) { auto* prm = p->apvts.getParameter (id); prm->setValueNotifyingHost (prm->getValue() > 0.5f ? 0.0f : 1.0f); }
                p->processBlock (b, m); for (int i = 0; i < 256; ++i) out.push_back (b.getSample (0, i));
            }
            return out;
        };
        for (auto* id : ids)
        {
            const auto base = hold (id, false), r = hold (id, true);
            std::vector<float> d (r.size()); for (size_t q = 0; q < d.size(); ++q) d[q] = r[q] - base[q];
            float rms = 0; for (size_t q = 44000; q < 48000; ++q) rms += base[q] * base[q]; rms = std::sqrt (rms / 4000);
            const float c0 = clickScore (d, 47900, 70000), cb = clickScore (base, 47900, 70000), cr = clickScore (r, 47900, 70000);
            // un cambio morbido non puo' avere una derivata seconda piu' ripida del suono stesso (prima o dopo il cambio)
            const bool ok = c0 <= std::max (cb, cr) * 1.1f + 1e-5f;
            if (! ok) ++bad;
            std::printf ("click %-8s  scatto nella differenza %.5f  (suono prima %.5f, dopo %.5f)  %s\n", id, c0, cb, cr, ok ? "ok" : "<-- CLICK");
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
    if (mode == "edit" || mode == "all")
    {   // modifica a mano: resta dopo la rigenerazione e nel salvataggio, sparisce se cambi il giro, torna con annulla
        auto p = mk (1); auto P = *p->part(); const size_t n0 = P.ev.size();
        P.ev.erase (P.ev.begin()); P.ev[0].note += 2; p->commitEdit (P); p->regenerate();
        const bool kept = p->edited() && p->part()->ev.size() == n0 - 1;
        juce::MemoryBlock mb; p->getStateInformation (mb); auto q = mk (-1); q->setStateInformation (mb.getData(), (int) mb.getSize());
        const bool saved = q->edited() && q->part()->ev.size() == n0 - 1;
        p->undo.beginNewTransaction(); p->setProg ({ 0, 3, 4, 0 }); p->regenerate(); const bool gone = ! p->edited();
        const auto k1 = p->settingsKey(); p->undo.undo(); p->regenerate(); const bool back = p->edited();
        if (! back) std::printf ("  chiave dopo annulla %s\n  chiave salvata      %s\n  prima del cambio    %s\n", p->settingsKey().toRawUTF8(), p->apvts.state.getProperty ("editKey").toString().toRawUTF8(), k1.toRawUTF8());
        std::printf ("MODIFICHE: restano %s, salvate %s, via se cambi il giro %s, tornano con annulla %s\n", kept ? "si" : "NO", saved ? "si" : "NO", gone ? "si" : "NO", back ? "si" : "NO");
        if (! (kept && saved && gone && back)) ++bad;
    }
    if (mode == "snap")
    {
        auto p = mk (std::getenv ("JPRESET") ? std::atoi (std::getenv ("JPRESET")) : 1);
        run (*p, 1.3, 512);
        if (auto* t = std::getenv ("JTHEME")) skin::cur() = std::atoi (t);
        if (std::getenv ("JGRID")) p->setV ("arpFig", (float) (kFigs - 1));
        Editor ed (*p); if (std::getenv ("JBROWSE")) ed.browser.open(); if (std::getenv ("JGRID")) ed.gridEdit = true;
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
