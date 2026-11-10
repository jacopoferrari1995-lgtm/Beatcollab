#pragma once
#include <juce_dsp/juce_dsp.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "MasterDSP.h"

// Analisi tonale (bilanciamento) e traccia di riferimento
namespace jms
{
constexpr int kThirds = 31;                     // terzi d'ottava 20 Hz .. 20 kHz
inline float thirdHz (int k) { return 20.0f * std::pow (2.0f, k / 3.0f); }
constexpr int kRegions = 6;
inline const char* regionName (int r) { static const char* n[] = { "SUB", "BASSI", "MEDIO-BASSI", "MEDI", "PRESENZA", "ARIA" }; return n[juce::jlimit (0, 5, r)]; }
inline float regionEdge (int k) { static const float e[] = { 20, 60, 250, 800, 2500, 6000, 20000 }; return e[juce::jlimit (0, 6, k)]; }
inline float regionCentre (int r) { return std::sqrt (regionEdge (r) * regionEdge (r + 1)); }

// curve obiettivo per genere: scarto (dB) di ogni regione rispetto a un mix "piatto" compensato 4.5 dB/ottava
struct Genre { const char* name; float off[kRegions]; };
constexpr int kGenres = 8;
inline const Genre& genre (int g)
{
    static const Genre G[kGenres] = {
        { "TRAP / DRILL", { 5, 2.5f, -1, -1, -1.5f, -2 } }, { "HIP HOP / BOOM BAP", { 3, 3, 0, -0.5f, -1, -2.5f } }, { "R&B", { 3, 2, 0, -0.5f, -0.5f, 0 } },
        { "POP", { 1.5f, 1, -0.5f, 0, 0, 0 } }, { "EDM / CLUB", { 4, 2, -1, -0.5f, 0, 0.5f } }, { "ROCK", { 0, 1, 0, 0, 0.5f, -0.5f } },
        { "LO-FI", { 2, 3, 1, -0.5f, -3, -5 } }, { "ACUSTICO", { -2, 0.5f, 0.5f, 0.5f, 0, -0.5f } } };
    return G[juce::jlimit (0, kGenres - 1, g)];
}

// spettro medio in terzi d'ottava (dB, compensato +4.5 dB/ott rispetto a 1 kHz). Accumula energia.
class ThirdAnalyzer
{
public:
    ThirdAnalyzer() : fft (12), win ((size_t) 4096) { for (int i = 0; i < 4096; ++i) win[(size_t) i] = 0.5f - 0.5f * std::cos (6.2831853f * i / 4096); buf.resize (8192); }
    void reset() { std::fill (std::begin (acc), std::end (acc), 0.0); frames = 0; }
    // aggiunge un blocco di 4096 campioni mono. k <= 0: media su tutto; k > 0: media mobile esponenziale
    void addFrame (const float* x, double sr, double k = 0)
    {
        for (int i = 0; i < 4096; ++i) buf[(size_t) i] = x[i] * win[(size_t) i];
        std::fill (buf.begin() + 4096, buf.end(), 0.0f);
        fft.performFrequencyOnlyForwardTransform (buf.data(), true);
        for (int b3 = 0; b3 < kThirds; ++b3)
        {
            const double fc = thirdHz (b3), f0 = fc / std::pow (2.0, 1.0 / 6.0), f1 = fc * std::pow (2.0, 1.0 / 6.0);
            const int a = juce::jmax (1, (int) std::floor (f0 * 4096 / sr)), b = juce::jmin (2047, juce::jmax (a, (int) std::ceil (f1 * 4096 / sr)));
            double e = 0; for (int j = a; j <= b; ++j) e += (double) buf[(size_t) j] * buf[(size_t) j];
            e /= (b - a + 1);
            acc[b3] = k <= 0 ? acc[b3] + e : acc[b3] * (1 - k) + e * k;
        }
        ++frames;
    }
    // dB per terzo, compensati, normalizzati sulla media
    void result (float* out) const
    {
        double m = 0; float v[kThirds];
        for (int k = 0; k < kThirds; ++k) { v[k] = 10.0f * std::log10 ((float) acc[k] + 1e-20f) + 4.5f * std::log2 (thirdHz (k) / 1000.0f); m += v[k]; }
        m /= kThirds; for (int k = 0; k < kThirds; ++k) out[k] = v[k] - (float) m;
    }
    bool hasData() const { return frames > 2; }
    static void regions (const float* thirds, float* reg)
    {
        double m = 0;
        for (int r = 0; r < kRegions; ++r)
        {
            double s = 0; int c = 0;
            for (int k = 0; k < kThirds; ++k) { const float f = thirdHz (k); if (f >= regionEdge (r) && f < regionEdge (r + 1)) { s += thirds[k]; ++c; } }
            reg[r] = c ? (float) (s / c) : 0.0f; m += reg[r];
        }
        m /= kRegions; for (int r = 0; r < kRegions; ++r) reg[r] -= (float) m;
    }
    int frames = 0;
private:
    juce::dsp::FFT fft; std::vector<float> win, buf; double acc[kThirds] {};
};

// traccia di riferimento gia decodificata e analizzata
struct RefTrack
{
    juce::AudioBuffer<float> buf; juce::String name; float lufs = -100, tp = -100; float thirds[kThirds] {}; std::vector<float> overview;
};
inline std::shared_ptr<RefTrack> loadRefTrack (const juce::File& f, double sr)
{
    juce::AudioFormatManager fm; fm.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> r (fm.createReaderFor (f));
    if (r == nullptr || r->lengthInSamples <= 0) return nullptr;
    const int n = (int) std::min<long long> ((long long) r->lengthInSamples, (long long) (r->sampleRate * 600));
    juce::AudioBuffer<float> raw (2, n); raw.clear();
    r->read (&raw, 0, n, 0, true, r->numChannels > 1);
    if (r->numChannels == 1) raw.copyFrom (1, 0, raw, 0, 0, n);
    auto t = std::make_shared<RefTrack>(); t->name = f.getFileNameWithoutExtension();
    // ricampiona alla frequenza della sessione
    const double ratio = r->sampleRate / sr; const int m = (int) (n / ratio);
    t->buf.setSize (2, m);
    for (int c = 0; c < 2; ++c) { juce::LagrangeInterpolator li; li.process (ratio, raw.getReadPointer (c), t->buf.getWritePointer (c), m); }
    // loudness integrata e true peak
    Loudness lu; lu.prepare (sr); TruePeak tpd[2]; float tp = 0;
    for (int i = 0; i < m; ++i) { lu.push (t->buf.getSample (0, i), t->buf.getSample (1, i)); tp = juce::jmax (tp, tpd[0].push (t->buf.getSample (0, i)), tpd[1].push (t->buf.getSample (1, i))); }
    lu.flush(); t->lufs = lu.I > -60 ? lu.I : lu.S; t->tp = gToDb (tp);
    // spettro medio
    ThirdAnalyzer an; std::vector<float> mono (4096);
    for (int s = 0; s + 4096 <= m; s += 2048) { for (int i = 0; i < 4096; ++i) mono[(size_t) i] = 0.5f * (t->buf.getSample (0, s + i) + t->buf.getSample (1, s + i)); an.addFrame (mono.data(), sr); }
    an.result (t->thirds);
    // panoramica della forma d'onda
    t->overview.assign (600, 0.0f);
    for (int i = 0; i < m; ++i) { const int k = (int) ((juce::int64) i * 600 / m); t->overview[(size_t) k] = juce::jmax (t->overview[(size_t) k], std::abs (t->buf.getSample (0, i))); }
    return t;
}
}
