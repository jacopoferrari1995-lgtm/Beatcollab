#include "../Source/RewindProcessor.h"
#include "../Source/RewindEditor.h"
using namespace jrwd;
struct Head : juce::AudioPlayHead
{
    double ppq = 0, bpm = 92; bool play = true;
    juce::Optional<PositionInfo> getPosition() const override { PositionInfo p; p.setBpm (bpm); p.setPpqPosition (ppq); p.setIsPlaying (play); p.setTimeSignature (TimeSignature { 4, 4 }); return p; }
};
int main (int argc, char** argv)
{
    if (auto* t = std::getenv ("JTHEME")) skin::cur() = std::atoi (t);
    juce::ScopedJuceInitialiser_GUI gui;
    const double sr = 44100;
    Processor p; Head h; p.setPlayHead (&h); p.prepareToPlay (sr, 512);
    juce::AudioBuffer<float> b (2, 512); juce::MidiBuffer m; double ph = 0; bool same = true;
    // 40 s: silenzio, 12 s di accordo di La minore, silenzio, 8 s di beat, silenzio
    const int total = (int) (sr * 40) / 512;
    for (int k = 0; k < total; ++k)
    {
        const double t0 = k * 512 / sr;
        for (int i = 0; i < 512; ++i)
        {
            const double t = t0 + i / sr; float v = 0;
            if (t > 4 && t < 16) v = (float) (0.2 * (std::sin (ph) + std::sin (ph * 1.189) + std::sin (ph * 1.498)));   // A C E
            if (t > 22 && t < 30) { const double bt = std::fmod (t * 92 / 60, 1.0); v = (float) (bt < 0.1 ? std::sin (t * 2 * 3.14159 * 55) * (1 - bt * 10) : 0); }
            ph += 2 * juce::MathConstants<double>::pi * 220 / sr;
            b.setSample (0, i, v); b.setSample (1, i, v);
        }
        juce::AudioBuffer<float> in; in.makeCopyOf (b);
        p.processBlock (b, m);
        for (int i = 0; i < 512; ++i) if (b.getSample (0, i) != in.getSample (0, i)) same = false;
        h.ppq += 512 / sr * h.bpm / 60.0;
    }
    std::printf ("uscita identica all'ingresso: %d   memoria %.0f s   scritti %.1f s\n", (int) same, p.capacity() / sr, p.written() / sr);
    Editor ed (p); ed.setSize (Editor::W, Editor::H);
    ed.resized();
    // prese trovate
    std::printf ("prese: %d\n", (int) ed.takes.size());
    for (auto& t : ed.takes) std::printf ("  %.2f s -> %.2f s  (%s)\n", t.a / sr, t.b / sr, ed.barsText (t.a, t.b).toRawUTF8());
    if (! ed.takes.empty())
    {
        ed.setSel (ed.takes[0].a, ed.takes[0].b);
        juce::AudioBuffer<float> sel; p.readRange (ed.selA, ed.selB, sel);
        std::printf ("selezione agganciata: %.3f s -> %.3f s  tonalita %s\n", ed.selA / sr, ed.selB / sr, p.keyOf (sel).toRawUTF8());
        auto f = ed.exportSel(); std::printf ("file: %s (%lld byte)\n", f.getFileName().toRawUTF8(), (long long) f.getSize());
        p.preview (ed.selA, ed.selB, false);
        float mx = 0; for (int k = 0; k < 300; ++k) { b.clear(); p.processBlock (b, m); mx = juce::jmax (mx, b.getMagnitude (0, 512)); } std::printf ("ascolto: picco %.2f\n", mx);
        p.stopPreview(); for (int k = 0; k < 4; ++k) { b.clear(); p.processBlock (b, m); }
    }
    {   // fedelta': segnale sopra 0 dBFS e segnale debole, letti dalla memoria
        Processor q; q.prepareToPlay (sr, 512); juce::AudioBuffer<float> bb (2, 512); juce::MidiBuffer mm; std::vector<float> in;
        for (int k = 0; k < 400; ++k) { for (int i = 0; i < 512; ++i) { const double t = (k * 512 + i) / sr; const float v = (float) ((k < 200 ? 1.8 : 0.001) * std::sin (2 * 3.14159 * 330 * t)); in.push_back (v); bb.setSample (0, i, v); bb.setSample (1, i, -v); } q.processBlock (bb, mm); }
        juce::AudioBuffer<float> rb; q.readRange (0, q.written(), rb);
        double eLoud = 0, eQuiet = 0, sLoud = 0, sQuiet = 0;
        for (int i = 0; i < rb.getNumSamples(); ++i) { const double d = rb.getSample (0, i) - in[(size_t) i]; if (i < 200 * 512) { eLoud += d * d; sLoud += in[(size_t) i] * in[(size_t) i]; } else { eQuiet += d * d; sQuiet += in[(size_t) i] * in[(size_t) i]; } }
        std::printf ("fedelta' memoria: +5 dBFS errore %.1f dB, -60 dBFS errore %.1f dB (rispetto al segnale), picco letto %.2f\n", 10 * std::log10 (eLoud / sLoud), 10 * std::log10 (eQuiet / sQuiet), rb.getMagnitude (0, 0, 200 * 512));
        // ascolto in loop di 1 s su un tono continuo: salti al giro e all'avvio/stop
        const juce::int64 A = q.written() - (juce::int64) (sr * 2.5) - 512 * 200 + 512 * 200; (void) A;
        Processor z; z.prepareToPlay (sr, 512); for (int k = 0; k < 300; ++k) { for (int i = 0; i < 512; ++i) { const double t = (k * 512 + i) / sr; const float v = (float) (0.5 * std::sin (2 * 3.14159 * 200 * t)); bb.setSample (0, i, v); bb.setSample (1, i, v); } z.processBlock (bb, mm); }
        z.preview ((juce::int64) (sr * 1.0) + 37, (juce::int64) (sr * 2.0) + 11, true); std::vector<float> o;
        for (int k = 0; k < 400; ++k) { if (k == 300) z.stopPreview(); bb.clear(); z.processBlock (bb, mm); for (int i = 0; i < 512; ++i) o.push_back (bb.getSample (0, i)); }
        double w = 0; size_t wi = 0; for (size_t i = 0; i + 2 < o.size(); ++i) { const double d = std::abs (o[i + 2] - 2 * o[i + 1] + o[i]); if (d > w) { w = d; wi = i; } }
        std::printf ("  (peggiore al campione %zu = %.3f s; giro ogni %.3f s)\n", wi, wi / sr, (sr - 26 - (int) (0.006 * sr)) / sr);
        const double ref = 0.5 * std::pow (2 * 3.14159 * 200 / sr, 2);
        std::printf ("ascolto in loop (avvio, giri, stop): derivata seconda massima %.4f, tono pulito %.4f\n", w, ref);
    }
    if (argc > 1)
    {
        auto img = ed.createComponentSnapshot (ed.getLocalBounds(), true, 1.0f);
        juce::File f (argv[1]); f.deleteFile(); juce::FileOutputStream os (f); juce::PNGImageFormat().writeImageToStream (img, os);
    }
}
