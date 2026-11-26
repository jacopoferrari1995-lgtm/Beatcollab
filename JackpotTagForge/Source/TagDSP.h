#pragma once
// JACKPOT TAG FORGE (serie Cybernet): dalla frase registrata al tag del produttore.
// Il tag si "forgia" fuori dal tempo reale: ogni volta che cambia un comando la catena gira su tutta la frase
// (taglio, normalizza, nastro, pitch, robot, radio, drive, crush, stutter, swell al contrario, eco, riverbero, larghezza)
// e il risultato e' un buffer pronto che si suona con una nota MIDI o col tasto, si esporta o si trascina nella DAW.
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include <vector>

namespace jtag
{
struct Settings
{
    float start = 0, end = 1;        // taglio della frase (0..1)
    float pitch = 0, tape = 0;       // semitoni: pitch mantiene la durata, nastro la cambia
    float robot = 0, radio = 0, drive = 0, crush = 0;
    int stutter = 0; bool reverse = false; float swell = 0;
    float echo = 0, echoFb = 0.45f; int echoDiv = 2; // 0 1/8, 1 1/8., 2 1/4, 3 1/4., 4 1/2
    float verb = 0, size = 0.6f, width = 0;
    double bpm = 140;
    bool operator== (const Settings& o) const
    {
        return start == o.start && end == o.end && pitch == o.pitch && tape == o.tape && robot == o.robot && radio == o.radio && drive == o.drive && crush == o.crush
            && stutter == o.stutter && reverse == o.reverse && swell == o.swell && echo == o.echo && echoFb == o.echoFb && echoDiv == o.echoDiv
            && verb == o.verb && size == o.size && width == o.width && std::abs (bpm - o.bpm) < 0.01;
    }
    bool operator!= (const Settings& o) const { return ! (*this == o); }
};
inline const char* echoDivName (int d) { static const char* n[] = { "1/8", "1/8.", "1/4", "1/4.", "1/2" }; return n[juce::jlimit (0, 4, d)]; }
inline double echoBeats (int d) { static const double b[] = { 0.5, 0.75, 1.0, 1.5, 2.0 }; return b[juce::jlimit (0, 4, d)]; }

// biquad semplice (RBJ)
struct Bq
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    void set (double sr, double f, double q, int type)   // 0 passa basso, 1 passa alto
    {
        const double w = juce::MathConstants<double>::twoPi * juce::jlimit (10.0, sr * 0.45, f) / sr, c = std::cos (w), s = std::sin (w), al = s / (2 * q), a0 = 1 + al;
        if (type == 0) { b0 = (1 - c) / 2 / a0; b1 = (1 - c) / a0; b2 = b0; } else { b0 = (1 + c) / 2 / a0; b1 = -(1 + c) / a0; b2 = b0; }
        a1 = -2 * c / a0; a2 = (1 - al) / a0; z1 = z2 = 0;
    }
    float p (float x) { const double y = b0 * x + z1; z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y; return (float) y; }
};

inline float cubic (const float* d, int n, double pos)
{
    const int i = (int) std::floor (pos); const float t = (float) (pos - i);
    auto at = [d, n] (int k) { return k >= 0 && k < n ? d[k] : 0.0f; };
    const float y0 = at (i - 1), y1 = at (i), y2 = at (i + 1), y3 = at (i + 2);
    const float c1 = 0.5f * (y2 - y0), c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3, c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
    return ((c3 * t + c2) * t + c1) * t + y1;
}
inline void fade (juce::AudioBuffer<float>& b, int start, int len, bool in)
{
    len = juce::jmin (len, b.getNumSamples() - start); if (len <= 0) return;
    for (int c = 0; c < b.getNumChannels(); ++c) { auto* d = b.getWritePointer (c, start); for (int i = 0; i < len; ++i) { const float g = (float) i / len; d[i] *= in ? g : 1.0f - g; } }
}
inline float peak (const juce::AudioBuffer<float>& b) { float p = 0; for (int c = 0; c < b.getNumChannels(); ++c) p = juce::jmax (p, b.getMagnitude (c, 0, b.getNumSamples())); return p; }

// la frase vera dentro la registrazione: via il silenzio prima e dopo (con un po' di margine)
inline juce::Range<int> voiced (const juce::AudioBuffer<float>& b, double sr, float threshDb = -45.0f)
{
    const int n = b.getNumSamples(); if (n == 0) return {};
    const float pk = peak (b); if (pk < 1e-5f) return {};
    const float th = pk * juce::Decibels::decibelsToGain (threshDb + 6.0f);
    int a = -1, z = -1;
    for (int i = 0; i < n; ++i) { float m = 0; for (int c = 0; c < b.getNumChannels(); ++c) m = juce::jmax (m, std::abs (b.getSample (c, i))); if (m > th) { if (a < 0) a = i; z = i; } }
    if (a < 0) return {};
    return { juce::jmax (0, a - (int) (0.02 * sr)), juce::jmin (n, z + (int) (0.12 * sr)) };
}

// pitch che mantiene la durata: grani di ~43 ms sovrapposti a 4, letti alla velocita' del pitch
inline juce::AudioBuffer<float> pitchShift (const juce::AudioBuffer<float>& in, double sr, float semis)
{
    const int n = in.getNumSamples(), C = in.getNumChannels();
    juce::AudioBuffer<float> out (C, n); out.clear();
    const double r = std::pow (2.0, semis / 12.0);
    const int G = juce::jmax (256, (int) (sr * 0.043)), hop = G / 4;
    std::vector<float> w ((size_t) G); for (int i = 0; i < G; ++i) w[(size_t) i] = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * i / G);
    for (int c = 0; c < C; ++c)
    {
        const float* s = in.getReadPointer (c); float* o = out.getWritePointer (c);
        for (int g0 = -G; g0 < n; g0 += hop)
        {
            const double src0 = g0 + G * 0.5 - G * 0.5 * r;   // il centro del grano resta al suo posto nel tempo
            for (int i = 0; i < G; ++i) { const int k = g0 + i; if (k < 0 || k >= n) continue; o[k] += w[(size_t) i] * cubic (s, n, src0 + i * r); }
        }
        juce::FloatVectorOperations::multiply (o, 0.5f, n);
    }
    return out;
}
// nastro: cambia velocita' e altezza insieme
inline juce::AudioBuffer<float> varispeed (const juce::AudioBuffer<float>& in, float semis)
{
    const double r = std::pow (2.0, semis / 12.0); const int n = in.getNumSamples(), m = juce::jmax (1, (int) (n / r));
    juce::AudioBuffer<float> out (in.getNumChannels(), m);
    for (int c = 0; c < in.getNumChannels(); ++c) { const float* s = in.getReadPointer (c); float* o = out.getWritePointer (c); for (int i = 0; i < m; ++i) o[i] = cubic (s, n, i * r); }
    return out;
}
inline void reverb (juce::AudioBuffer<float>& b, double sr, float size, float wet, float dry)
{
    juce::Reverb rv; juce::Reverb::Parameters p; p.roomSize = juce::jlimit (0.0f, 1.0f, 0.4f + size * 0.58f); p.damping = 0.35f; p.wetLevel = wet; p.dryLevel = dry; p.width = 1.0f;
    rv.setParameters (p); rv.setSampleRate (sr); rv.reset();
    if (b.getNumChannels() > 1) rv.processStereo (b.getWritePointer (0), b.getWritePointer (1), b.getNumSamples()); else rv.processMono (b.getWritePointer (0), b.getNumSamples());
}
inline juce::AudioBuffer<float> withTail (const juce::AudioBuffer<float>& b, int extra)
{
    juce::AudioBuffer<float> o (b.getNumChannels(), b.getNumSamples() + extra); o.clear();
    for (int c = 0; c < b.getNumChannels(); ++c) o.copyFrom (c, 0, b, c, 0, b.getNumSamples());
    return o;
}

// la catena intera: dalla frase registrata al tag pronto (stereo)
inline juce::AudioBuffer<float> forge (const juce::AudioBuffer<float>& src, double sr, const Settings& S)
{
    juce::AudioBuffer<float> b;
    const int N = src.getNumSamples(); if (N < 64 || sr < 8000) return b;
    const int a = juce::jlimit (0, N - 1, (int) (S.start * N)), z = juce::jlimit (a + 1, N, (int) (S.end * N));
    if (z - a < (int) (0.03 * sr)) return b;
    b.setSize (2, z - a);
    for (int c = 0; c < 2; ++c) b.copyFrom (c, 0, src, juce::jmin (c, src.getNumChannels() - 1), a, z - a);
    fade (b, 0, (int) (0.004 * sr), true); fade (b, b.getNumSamples() - (int) (0.006 * sr), (int) (0.006 * sr), false);
    {   // normalizza a -1 dBFS (al massimo +24 dB)
        const float pk = peak (b); if (pk > 1e-6f) b.applyGain (juce::jmin (juce::Decibels::decibelsToGain (24.0f), 0.89f / pk));
    }
    if (S.reverse) for (int c = 0; c < 2; ++c) { auto* d = b.getWritePointer (c); std::reverse (d, d + b.getNumSamples()); }
    if (std::abs (S.tape) > 0.01f) b = varispeed (b, S.tape);
    if (std::abs (S.pitch) > 0.01f) { const float before = peak (b); b = pitchShift (b, sr, S.pitch); const float after = peak (b); if (after > 1e-6f) b.applyGain (juce::jmin (2.5f, before / after)); }
    const int n = b.getNumSamples();
    if (S.robot > 0.001f)
    {   // cyborg: modulazione ad anello a 110 Hz e un pettine metallico corto
        const int D = (int) (sr * 0.0063); std::vector<float> line ((size_t) D + 1, 0.0f);
        for (int c = 0; c < 2; ++c)
        {
            auto* d = b.getWritePointer (c); int wp = 0; std::fill (line.begin(), line.end(), 0.0f);
            for (int i = 0; i < n; ++i)
            {
                const float ring = d[i] * std::sin (juce::MathConstants<float>::twoPi * 110.0f * i / (float) sr);
                const float x = d[i] * (1.0f - S.robot) + ring * S.robot * 1.6f;
                const float y = x + line[(size_t) wp] * 0.55f * S.robot; line[(size_t) wp] = y; wp = (wp + 1) % D;
                d[i] = y * (1.0f - 0.35f * S.robot);
            }
        }
    }
    if (S.radio > 0.001f)
    {   // radio / telefono: banda 400 Hz - 3 kHz, un po' saturata, e un filo di fruscio
        juce::Random rnd (7);
        for (int c = 0; c < 2; ++c)
        {
            Bq h1, h2, l1, l2; const double hp = 120 + 380 * S.radio, lp = 12000 - 9000 * S.radio;
            h1.set (sr, hp, 0.8, 1); h2.set (sr, hp, 0.8, 1); l1.set (sr, lp, 0.8, 0); l2.set (sr, lp, 0.8, 0);
            auto* d = b.getWritePointer (c);
            for (int i = 0; i < n; ++i) { const float y = std::tanh (l2.p (l1.p (h2.p (h1.p (d[i])))) * (1 + 2 * S.radio)) * 0.8f; d[i] = d[i] * (1 - S.radio) + (y + (rnd.nextFloat() - 0.5f) * 0.006f * S.radio) * S.radio; }
        }
    }
    if (S.drive > 0.001f)
    {
        const float g = 1.0f + S.drive * 18.0f, norm = 1.0f / std::tanh (g * 0.5f);
        for (int c = 0; c < 2; ++c) { auto* d = b.getWritePointer (c); for (int i = 0; i < n; ++i) d[i] = std::tanh (d[i] * g * 0.5f) * norm * 0.8f; }
    }
    if (S.crush > 0.001f)
    {   // bit e campionamento: da 16 bit a 4, tenuta fino a 1/12
        const float bits = 16.0f - 12.0f * S.crush, q = std::pow (2.0f, bits - 1); const int hold = 1 + (int) (11 * S.crush * S.crush);
        for (int c = 0; c < 2; ++c) { auto* d = b.getWritePointer (c); float h = 0; for (int i = 0; i < n; ++i) { if (i % hold == 0) h = std::round (d[i] * q) / q; d[i] = h; } }
    }
    if (S.stutter > 0)
    {   // J-J-J-Jackpot: il primo sedicesimo ripetuto prima della frase
        const int sl = juce::jmin (n, (int) (sr * 60.0 / juce::jmax (40.0, S.bpm) / 4.0));
        juce::AudioBuffer<float> o (2, sl * S.stutter + n); o.clear();
        juce::AudioBuffer<float> piece (2, sl); for (int c = 0; c < 2; ++c) piece.copyFrom (c, 0, b, c, 0, sl);
        fade (piece, 0, (int) (0.002 * sr), true); fade (piece, sl - (int) (0.004 * sr), (int) (0.004 * sr), false);
        for (int k = 0; k < S.stutter; ++k) { const float g = 0.75f + 0.25f * (float) (k + 1) / S.stutter; for (int c = 0; c < 2; ++c) o.addFrom (c, k * sl, piece, c, 0, sl, g); }
        for (int c = 0; c < 2; ++c) o.addFrom (c, sl * S.stutter, b, c, 0, n);
        b = o;
    }
    int lead = 0;
    if (S.swell > 0.001f)
    {   // riverbero al contrario che sale fino all'inizio del tag
        const int sw = (int) (sr * 1.3);
        juce::AudioBuffer<float> r = withTail (b, sw);
        reverb (r, sr, 0.85f, 1.0f, 0.0f);
        for (int c = 0; c < 2; ++c) { auto* d = r.getWritePointer (c); std::reverse (d, d + r.getNumSamples()); }
        // la coda girata finisce dove comincia la voce
        juce::AudioBuffer<float> o (2, sw + b.getNumSamples()); o.clear();
        const int from = r.getNumSamples() - sw - b.getNumSamples();   // la parte della coda dopo la fine originale, ora davanti
        juce::ignoreUnused (from);
        for (int c = 0; c < 2; ++c) o.addFrom (c, 0, r, c, 0, sw, S.swell * 1.4f);
        fade (o, 0, sw / 3, true);
        for (int c = 0; c < 2; ++c) o.addFrom (c, sw, b, c, 0, b.getNumSamples());
        b = o; lead = sw;
    }
    if (S.echo > 0.001f)
    {   // eco ping-pong a tempo: la coda continua finche' non si spegne
        const int D = juce::jmax (1, (int) (sr * 60.0 / juce::jmax (40.0, S.bpm) * echoBeats (S.echoDiv)));
        const float fb = juce::jlimit (0.0f, 0.92f, S.echoFb);
        int reps = 1; for (float g = fb; g > 0.003f && reps < 40; g *= fb) ++reps;
        const int len = b.getNumSamples();
        juce::AudioBuffer<float> o = withTail (b, juce::jmin ((int) (sr * 8), D * reps));
        std::vector<float> l ((size_t) D, 0.0f), r ((size_t) D, 0.0f); int wp = 0;
        Bq lpL, lpR; lpL.set (sr, 5500, 0.7, 0); lpR.set (sr, 5500, 0.7, 0);
        auto* L = o.getWritePointer (0); auto* R = o.getWritePointer (1);
        for (int i = 0; i < o.getNumSamples(); ++i)
        {
            const float inL = i < len ? b.getSample (0, i) : 0.0f, inR = i < len ? b.getSample (1, i) : 0.0f;
            const float dl = l[(size_t) wp], dr = r[(size_t) wp];
            l[(size_t) wp] = lpL.p (0.5f * (inL + inR) + dr * fb); r[(size_t) wp] = lpR.p (dl * fb);
            wp = (wp + 1) % D;
            L[i] += dl * S.echo * 1.2f; R[i] += dr * S.echo * 1.2f;
        }
        b = o;
    }
    if (S.verb > 0.001f)
    {
        juce::AudioBuffer<float> o = withTail (b, (int) (sr * (1.5 + 3.0 * S.size)));
        reverb (o, sr, S.size, S.verb * 0.9f, 1.0f);
        b = o;
    }
    if (S.width > 0.001f)
    {   // larghezza: il lato destro un po' in ritardo solo sulla parte "laterale"
        const int D = (int) (sr * 0.012 * S.width);
        juce::AudioBuffer<float> o = withTail (b, D);
        auto* L = o.getWritePointer (0); auto* R = o.getWritePointer (1);
        std::vector<float> side ((size_t) o.getNumSamples(), 0.0f);
        for (int i = 0; i < o.getNumSamples(); ++i) side[(size_t) i] = 0.5f * (L[i] - R[i]);
        for (int i = o.getNumSamples() - 1; i >= 0; --i)
        {
            const float m = 0.5f * (L[i] + R[i]), dm = i >= D ? 0.5f * (L[i - D] + R[i - D]) : 0.0f;
            const float s = side[(size_t) i] + (m - dm) * 0.45f * S.width;
            L[i] = m + s; R[i] = m - s;
        }
        b = o;
    }
    {   // coda: via il silenzio finale, dissolvenza breve, niente picchi oltre 0 dB
        int last = b.getNumSamples() - 1;
        const float th = juce::Decibels::decibelsToGain (-72.0f);
        while (last > lead + 32 && std::abs (b.getSample (0, last)) < th && std::abs (b.getSample (1, last)) < th) --last;
        b.setSize (2, last + 1, true, true, true);
        fade (b, b.getNumSamples() - juce::jmin (b.getNumSamples() / 4, (int) (0.01 * sr)), juce::jmin (b.getNumSamples() / 4, (int) (0.01 * sr)), false);
        const float pk = peak (b); if (pk > 0.97f) b.applyGain (0.97f / pk);
    }
    return b;
}
}
