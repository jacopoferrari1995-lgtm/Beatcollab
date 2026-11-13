#include "../Source/PhaseProcessor.h"
#include "../Source/PhaseEditor.h"
using namespace jphs;
static void setP (Processor& p, const char* id, float v) { auto* r = p.apvts.getParameter (id); r->setValueNotifyingHost (r->convertTo0to1 (v)); }
// cassa: colpo a 55 Hz che scende; basso: 808 a 50 Hz sfasato di 'ph' gradi
static void gen (std::vector<float>& k, std::vector<float>& b, double sr, int len, double phDeg, double bassF = 50)
{
    k.assign ((size_t) len, 0); b.assign ((size_t) len, 0);
    const int per = (int) (sr * 0.5);
    for (int i = 0; i < len; ++i)
    {
        const int t0 = i % per; const double t = t0 / sr;
        k[(size_t) i] = (float) (std::sin (2 * juce::MathConstants<double>::pi * (55 * t + 40 * (1 - std::exp (-t * 30)) / 30)) * std::exp (-t * 9) * 0.7);
        b[(size_t) i] = (float) (std::sin (2 * juce::MathConstants<double>::pi * bassF * t + juce::degreesToRadians (phDeg)) * std::exp (-t * 2) * 0.5);
    }
}
int main (int argc, char** argv)
{
    if (auto* t = std::getenv ("JTHEME")) skin::cur() = std::atoi (t);
    juce::ScopedJuceInitialiser_GUI gui;
    const double sr = 44100;
    float trueCorr = 0;
    auto run = [&] (int mode, double ph, float& corr, float& sumGainDb, float& theta, float& neutralErr, bool snapOut = false, const char* file = nullptr)
    {
        Processor p; p.enableAllBuses(); p.prepareToPlay (sr, 512); setP (p, "mode", (float) mode);
        std::vector<float> k, b; const int len = (int) (sr * 4); gen (k, b, sr, len, ph);
        juce::AudioBuffer<float> buf (4, 512); juce::MidiBuffer m; std::vector<float> out ((size_t) len, 0);
        for (int pos = 0; pos + 512 <= len; pos += 512)
        {
            for (int i = 0; i < 512; ++i) { buf.setSample (0, i, b[(size_t) (pos + i)]); buf.setSample (1, i, b[(size_t) (pos + i)]); buf.setSample (2, i, k[(size_t) (pos + i)]); buf.setSample (3, i, k[(size_t) (pos + i)]); }
            p.processBlock (buf, m);
            for (int i = 0; i < 512; ++i) out[(size_t) (pos + i)] = buf.getSample (0, i);
        }
        const int L = p.getLatencySamples();
        // energia della somma cassa + basso (allineato) nell'ultimo secondo
        double eS = 0, eD = 0, err = 0;
        for (int i = len - (int) sr; i < len - L; ++i) { const double s = k[(size_t) i] + out[(size_t) (i + L)], d = k[(size_t) i] + b[(size_t) i]; eS += s * s; eD += d * d; err = std::max (err, (double) std::abs (out[(size_t) (i + L)] - b[(size_t) i])); }
        if (mode == M_OFF && ph == 0) { int best = 0; double be = 1e9; for (int off = 0; off < 3000; ++off) { double e2 = 0; for (int i = len - 20000; i < len - 4000; ++i) e2 += std::abs (out[(size_t) (i + off)] - b[(size_t) i]); if (e2 < be) { be = e2; best = off; } } std::printf ("L %d miglior ritardo %d\n", L, best); }
        sumGainDb = (float) (10 * std::log10 (eS / eD));
        // correlazione vera nei primi 100 ms di ogni colpo (ultimi 2 secondi)
        { double kb = 0, kk = 0, bb = 0; const int per = (int) (sr * 0.5);
          for (int st = len - 2 * (int) sr; st + per < len - L; st += per) { const int s0 = st - st % per; for (int i = s0; i < s0 + (int) (sr * 0.1); ++i) { kb += k[(size_t) i] * out[(size_t) (i + L)]; kk += k[(size_t) i] * k[(size_t) i]; bb += out[(size_t) (i + L)] * out[(size_t) (i + L)]; } }
          trueCorr = (float) (kb / std::sqrt (kk * bb)); } corr = p.eng.caUi.load(); theta = p.eng.thetaUi.load(); neutralErr = (float) err;
        if (snapOut && file) { Editor ed (p); ed.setSize (Editor::W, Editor::H); auto img = ed.createComponentSnapshot (ed.getLocalBounds(), true, 1.0f); juce::File f (file); f.deleteFile(); juce::FileOutputStream os (f); juce::PNGImageFormat().writeImageToStream (img, os); }
    };
    if (argc > 2 && juce::String (argv[1]) == "snap") { float a, b2, c, d; run (M_ADAPT, juce::SystemStats::getEnvironmentVariable ("JPH", "150").getDoubleValue(), a, b2, c, d, true, argv[2]); return 0; }
    for (float ang : { 0.0f, 90.0f, 180.0f })
    {
        Processor p; p.prepareToPlay (sr, 512); setP (p, "mode", M_ROT); setP (p, "angle", ang); setP (p, "speed", 1);
        juce::AudioBuffer<float> buf (2, 512); juce::MidiBuffer m; double ph = 0; double dot = 0, e1 = 0, e2 = 0; std::vector<float> in, out;
        for (int k = 0; k < 200; ++k) { for (int i = 0; i < 512; ++i) { const float v = (float) std::sin (ph); ph += 2 * juce::MathConstants<double>::pi * 50 / sr; buf.setSample (0, i, v); buf.setSample (1, i, v); in.push_back (v); } p.processBlock (buf, m); for (int i = 0; i < 512; ++i) out.push_back (buf.getSample (0, i)); }
        const int L = p.getLatencySamples(); for (size_t i = 40000; i < 90000; ++i) { dot += in[i] * out[i + (size_t) L]; e1 += in[i] * in[i]; e2 += out[i + (size_t) L] * out[i + (size_t) L]; }
        std::printf ("ruota %.0f a 50 Hz: correlazione con l'originale %+.3f guadagno %+.2f dB\n", ang, dot / std::sqrt (e1 * e2), 10 * std::log10 (e2 / e1));
    }
    float corr, g, th, err;
    run (M_OFF, 0, corr, g, th, err); std::printf ("spento: errore max rispetto all'originale %.6f\n", err);
    for (double ph : { 0.0, 60.0, 120.0, 180.0, 240.0 })
    {
        float c0, g0, t0, e0; run (M_OFF, ph, c0, g0, t0, e0); const float tc0 = trueCorr;
        run (M_ADAPT, ph, corr, g, th, err); const float tcA = trueCorr;
        float ci, gi, ti, ei; run (M_INV, ph, ci, gi, ti, ei); const float tcI = trueCorr;
        std::printf ("basso sfasato %3.0f: correlazione nei colpi: spento %+.2f | adattiva %+.2f (angolo %+.0f, somma %+.2f dB) | inverti %+.2f   [meter %+.2f -> %+.2f]\n", ph, tc0, tcA, th, g, tcI, c0, corr);
    }
}
