#include "../Source/ChoirProcessor.h"
#include "../Source/ChoirEditor.h"
#include <cstdio>
#include <chrono>
using namespace jcho;

// voce finta: impulsi glottali filtrati da tre formanti (A), vibrato, piccolo respiro fra le note
struct FakeVoice
{
    double sr = 48000, ph = 0; jkit::Bq f1, f2, f3; juce::Random r { 3 };
    FakeVoice() { f1.peak (sr, 700, 4, 18); f2.peak (sr, 1200, 5, 14); f3.peak (sr, 2600, 6, 10); }
    float tick (double midi, double t)
    {
        const double hz = 440.0 * std::pow (2.0, (midi - 69.0 + 0.25 * std::sin (2 * juce::MathConstants<double>::pi * 5.5 * t)) / 12.0);
        ph += hz / sr; float pulse = 0; if (ph >= 1) { ph -= 1; pulse = 1; }
        const float ex = pulse * 0.9f + (r.nextFloat() - 0.5f) * 0.01f;
        return 0.06f * f3.p (f2.p (f1.p (ex)));
    }
};
// altezza misurata su un tratto (autocorrelazione semplice), in nota MIDI
static double pitchOf (const std::vector<float>& x, size_t a, size_t n, double sr)
{   // autocorrelazione normalizzata: il primo picco vicino al massimo (niente errori d'ottava)
    const int l0 = (int) (sr / 1000), l1 = (int) (sr / 70); std::vector<double> c ((size_t) l1 + 1, 0.0); double mx = 0;
    for (int lag = l0; lag <= l1; ++lag) { double s = 0, e1 = 0, e2 = 0; for (size_t i = a; i < a + n; ++i) { s += x[i] * x[i + (size_t) lag]; e1 += x[i] * x[i]; e2 += x[i + (size_t) lag] * x[i + (size_t) lag]; } c[(size_t) lag] = s / std::sqrt (e1 * e2 + 1e-12); mx = std::max (mx, c[(size_t) lag]); }
    for (int lag = l0 + 1; lag < l1; ++lag) if (c[(size_t) lag] >= 0.9 * mx && c[(size_t) lag] >= c[(size_t) lag - 1] && c[(size_t) lag] >= c[(size_t) lag + 1])
    { const double y0 = c[(size_t) lag - 1], y1 = c[(size_t) lag], y2 = c[(size_t) lag + 1], den = y0 - 2 * y1 + y2; const double t = lag + (std::abs (den) > 1e-12 ? 0.5 * (y0 - y2) / den : 0.0); return 69.0 + 12.0 * std::log2 (sr / t / 440.0); }
    return 0;
}
static std::vector<float> render (Processor& p, const std::vector<double>& notes, double secPer, std::vector<int> midiNotes = {}, int blk = 512, bool uneven = false)
{
    const double sr = 48000; FakeVoice fv; const size_t N = (size_t) (notes.size() * secPer * sr);
    std::vector<float> out (N), in (N);
    for (size_t i = 0; i < N; ++i) { const double t = i / sr; in[i] = fv.tick (notes[(size_t) (t / secPer)], t); }
    juce::AudioBuffer<float> b (2, 4096); juce::MidiBuffer m; juce::Random rr (7); size_t pos = 0; bool sent = false;
    while (pos < N)
    {
        int n = uneven ? 1 + rr.nextInt (blk) : blk; n = (int) std::min<size_t> ((size_t) n, N - pos);
        b.setSize (2, n, false, false, true); m.clear();
        if (! sent) { for (int k : midiNotes) m.addEvent (juce::MidiMessage::noteOn (1, k, (juce::uint8) 100), 0); sent = true; }
        for (int i = 0; i < n; ++i) { b.setSample (0, i, in[pos + i]); b.setSample (1, i, in[pos + i]); }
        p.processBlock (b, m);
        for (int i = 0; i < n; ++i) out[pos + i] = b.getSample (0, i);
        pos += n;
    }
    return out;
}
static std::unique_ptr<Processor> mk() { auto p = std::make_unique<Processor>(); p->setPlayConfigDetails (2, 2, 48000, 512); p->prepareToPlay (48000, 512); return p; }

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gi;
    const juce::String mode = argc > 1 ? argv[1] : "all"; int bad = 0;
    if (mode == "neutral" || mode == "all")
    {   // Init: esce la voce identica, ritardata della latenza
        auto p = mk(); p->loadPreset (0); const int D = p->getLatencySamples();
        FakeVoice fv; const size_t N = 48000; std::vector<float> in (N); for (size_t i = 0; i < N; ++i) in[i] = fv.tick (60, i / 48000.0);
        juce::AudioBuffer<float> b (2, 512); juce::MidiBuffer m; std::vector<float> out;
        for (size_t pos = 0; pos < N; pos += 512) { for (int i = 0; i < 512; ++i) { b.setSample (0, i, in[pos + i]); b.setSample (1, i, in[pos + i]); } p->processBlock (b, m); for (int i = 0; i < 512; ++i) out.push_back (b.getSample (0, i)); }
        float d = 0; for (size_t i = (size_t) D; i < N; ++i) d = std::max (d, std::abs (out[i] - in[i - (size_t) D]));
        std::printf ("NEUTRO: latenza %d campioni (%.1f ms), differenza massima %.2e %s\n", D, D / 48.0, d, d == 0 ? "" : "<-- !!"); if (d != 0) ++bad;
    }
    if (mode == "harm" || mode == "all")
    {   // terza sopra in DO maggiore: LA3 -> DO4, DO4 -> MI4, MI4 -> SOL4, SOL3 -> SI3; poi quinta e ottava sotto
        struct Case { int iv; std::vector<double> want; };
        const std::vector<double> mel { 57, 60, 64, 55 };
        for (auto& c : std::vector<Case> { { I_3U, { 60, 64, 67, 59 } }, { I_5U, { 64, 67, 71, 62 } }, { I_8D, { 45, 48, 52, 43 } }, { I_3D, { 53, 57, 60, 52 } }, { I_UNI, { 57, 60, 64, 55 } } })
        {
            auto p = mk(); p->loadPreset (0); p->apvts.getParameter ("dry")->setValueNotifyingHost (0); p->apvts.getParameter ("human")->setValueNotifyingHost (0);
            p->apvts.getParameter ("v1_int")->setValueNotifyingHost (p->apvts.getParameter ("v1_int")->convertTo0to1 ((float) c.iv));
            auto o = render (*p, mel, 0.8);
            juce::String line; int ok = 0;
            for (size_t k = 0; k < mel.size(); ++k) { const double m = pitchOf (o, (size_t) ((k * 0.8 + 0.5) * 48000), 4800, 48000); line << juce::String (m, 2) << " (" << c.want[k] << ")  "; ok += std::abs (m - c.want[k]) < 0.45; }
            std::printf ("%-18s %s %s\n", intervalName (c.iv), line.toRawUTF8(), ok == (int) mel.size() ? "ok" : "<-- !!"); if (ok != (int) mel.size()) ++bad;
        }
    }
    if (mode == "midi" || mode == "all")
    {   // MIDI: tiene MI4 e SOL4, la voce canta LA3: le due voci devono essere su MI4 e SOL4
        auto p = mk(); p->loadPreset (0);
        auto set = [&] (const char* id, float v) { auto* q = p->apvts.getParameter (id); q->setValueNotifyingHost (q->convertTo0to1 (v)); };
        set ("mode", 1); set ("dry", -60); set ("human", 0); set ("v1_int", I_UNI);
        auto o = render (*p, { 57, 57 }, 0.8, { 64 });
        const double m = pitchOf (o, (size_t) (1.0 * 48000), 4800, 48000);
        std::printf ("MIDI: nota tenuta 64, voce 57 -> coro %.2f %s\n", m, std::abs (m - 64) < 0.45 ? "ok" : "<-- !!"); if (std::abs (m - 64) >= 0.45) ++bad;
    }
    if (mode == "blk" || mode == "all")
    {   // stesso risultato con blocchi diversi, niente NaN, CPU
        const int pr = 10;   // muro pieno
        auto a = mk(); a->loadPreset (pr); auto b = mk(); b->loadPreset (pr);
        const auto t0 = std::chrono::high_resolution_clock::now();
        auto oa = render (*a, { 57, 60, 64, 62, 59, 55 }, 0.5, {}, 512);
        const double el = std::chrono::duration<double> (std::chrono::high_resolution_clock::now() - t0).count();
        auto ob = render (*b, { 57, 60, 64, 62, 59, 55 }, 0.5, {}, 333, true);
        float d = 0, pk = 0; bool fin = true; for (size_t i = 0; i < oa.size(); ++i) { d = std::max (d, std::abs (oa[i] - ob[i])); pk = std::max (pk, std::abs (oa[i])); fin &= std::isfinite (oa[i]) && std::isfinite (ob[i]); }
        std::printf ("BLOCCHI: differenza %.2e  picco %.2f  finito %s  cpu %.2f%%\n", d, pk, fin ? "si" : "NO", el / 3.0 * 100.0);
        if (d > 1e-4f || ! fin || pk > 1.5f) ++bad;
        // salto di preset in corsa e parametri: niente scatti (derivata seconda)
        float mx = 0; for (size_t i = 2; i < oa.size(); ++i) mx = std::max (mx, std::abs (oa[i] - 2 * oa[i - 1] + oa[i - 2]));
        std::printf ("derivata seconda massima %.4f\n", mx);
    }
    if (mode == "presets" || mode == "all")
    {
        for (int i = 0; i < (int) presets().size(); ++i)
        {
            auto p = mk(); p->loadPreset (i); auto o = render (*p, { 57, 60, 64, 62 }, 0.5, { 60, 64, 67 });
            float pk = 0; double e = 0; bool fin = true; for (auto v : o) { pk = std::max (pk, std::abs (v)); e += v * v; fin &= std::isfinite (v); }
            const bool ok = fin && pk < 1.2f && pk > 0.01f;
            std::printf ("%-34s picco %5.1f dB  rms %6.1f dB %s\n", presets()[(size_t) i].name, juce::Decibels::gainToDecibels (pk), juce::Decibels::gainToDecibels ((float) std::sqrt (e / o.size())), ok ? "" : "<-- !!");
            if (! ok) ++bad;
        }
    }
    if (mode == "snap")
    {
        auto p = mk(); p->loadPreset (std::getenv ("JPRESET") ? std::atoi (std::getenv ("JPRESET")) : 9);
        render (*p, { 57, 60, 64, 62, 64, 67 }, 0.4, { 60, 64, 67 });
        if (auto* t = std::getenv ("JTHEME")) skin::cur() = std::atoi (t);
        Editor ed (*p); if (std::getenv ("JBROWSE")) ed.setVisible (true);
        ed.setSize (Editor::W, Editor::H);
        auto img = ed.createComponentSnapshot (ed.getLocalBounds(), true, 1.0f);
        juce::PNGImageFormat f; juce::FileOutputStream os (juce::File (argc > 2 ? argv[2] : "/tmp/choir.png")); os.setPosition (0); os.truncate(); f.writeImageToStream (img, os);
        return 0;
    }
    std::printf (bad ? "\nPROBLEMI: %d\n" : "\nTUTTO OK\n", bad);
    return bad;
}
