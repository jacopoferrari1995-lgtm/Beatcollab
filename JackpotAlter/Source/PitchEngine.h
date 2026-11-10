#pragma once
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <cmath>

// Motore di pitch per Jackpot Alter Drive:
// phase vocoder con aggancio di fase sui picchi (Laroche-Dolson) + inviluppo spettrale cepstrale
// per spostare intonazione e formanti in modo indipendente. Rilevatore YIN per QUANTIZZA e ROBOT.
namespace ja
{
static constexpr float kTwoPi = 6.283185307179586f;

inline float wrapPi (float x) { x = std::fmod (x + juce::MathConstants<float>::pi, kTwoPi); if (x < 0) x += kTwoPi; return x - juce::MathConstants<float>::pi; }

// --------------------------------------------------------------------------- un canale
class PhaseVocoder
{
public:
    void prepare (int order, int overlap)
    {
        N = 1 << order; H = N / overlap; nb = N / 2 + 1;
        fft = std::make_unique<juce::dsp::FFT> (order);
        win.assign ((size_t) N, 0); for (int i = 0; i < N; ++i) win[(size_t) i] = 0.5f - 0.5f * std::cos (kTwoPi * i / N);
        double acc = 0; for (int i = 0; i < N; ++i) acc += (double) win[(size_t) i] * win[(size_t) i]; olaGain = (float) ((double) H / acc);
        inBuf.assign ((size_t) N, 0); outAcc.assign ((size_t) (2 * N), 0);
        work.assign ((size_t) (2 * N), 0); cep.assign ((size_t) (2 * N), 0);
        mag.assign ((size_t) nb, 0); ph.assign ((size_t) nb, 0); prevPh.assign ((size_t) nb, 0); acc2.assign ((size_t) nb, 0); fq.assign ((size_t) nb, 0);
        env.assign ((size_t) nb, 1); cs.assign ((size_t) nb, 1); sn.assign ((size_t) nb, 0); isPeak.assign ((size_t) nb, 0); yRe.assign ((size_t) nb, 0); yIm.assign ((size_t) nb, 0); peaks.reserve ((size_t) nb);
        reset();
    }
    void reset()
    {
        std::fill (inBuf.begin(), inBuf.end(), 0.0f); std::fill (outAcc.begin(), outAcc.end(), 0.0f);
        std::fill (prevPh.begin(), prevPh.end(), 0.0f); std::fill (acc2.begin(), acc2.end(), 0.0f);
        inPos = 0; outPos = 0; fresh = true;
    }
    int latency() const { return N; }
    int hop() const { return H; }
    void markFresh() { fresh = true; std::fill (prevPh.begin(), prevPh.end(), 0.0f); }

    // un campione dentro, uno fuori (la cornice si calcola in frame())
    inline float push (float x)
    {
        inBuf[(size_t) inPos] = x; inPos = (inPos + 1) & (N - 1);
        const float y = outAcc[(size_t) outPos]; outAcc[(size_t) outPos] = 0; outPos = (outPos + 1) % (2 * N);
        return y;
    }

    // ratio: rapporto di intonazione; fRatio: spostamento delle formanti (1 = restano dove sono);
    // keepFormants: false = le formanti seguono il pitch (effetto chipmunk / mostro)
    void frame (float ratio, float fRatio, bool keepFormants, float lifterSamples)
    {
        // analisi
        for (int i = 0; i < N; ++i) work[(size_t) i] = inBuf[(size_t) ((inPos + i) & (N - 1))] * win[(size_t) i];
        std::fill (work.begin() + N, work.end(), 0.0f);
        fft->performRealOnlyForwardTransform (work.data(), true);
        float mx = 0;
        for (int k = 0; k < nb; ++k)
        {
            const float re = work[(size_t) (2 * k)], im = work[(size_t) (2 * k + 1)];
            mag[(size_t) k] = std::sqrt (re * re + im * im); ph[(size_t) k] = std::atan2 (im, re); mx = juce::jmax (mx, mag[(size_t) k]);
            const float inv = mag[(size_t) k] > 1e-20f ? 1.0f / mag[(size_t) k] : 0.0f; cs[(size_t) k] = re * inv; sn[(size_t) k] = im * inv;
            const float omega = kTwoPi * k * H / N;
            const float d = wrapPi (ph[(size_t) k] - prevPh[(size_t) k] - omega);
            fq[(size_t) k] = (omega + d) / H;      // frequenza vera in radianti per campione
            prevPh[(size_t) k] = ph[(size_t) k];
        }
        const bool formants = keepFormants ? (std::abs (ratio - 1) > 1e-4f || std::abs (fRatio - 1) > 1e-4f) : std::abs (fRatio - 1) > 1e-4f;
        if (formants) envelope (lifterSamples, mx);

        std::fill (yRe.begin(), yRe.end(), 0.0f); std::fill (yIm.begin(), yIm.end(), 0.0f);
        // picchi
        peaks.clear(); std::fill (isPeak.begin(), isPeak.end(), (char) 0);
        if (fresh) { for (int k = 0; k < nb; ++k) acc2[(size_t) k] = ph[(size_t) k] - H * ratio * fq[(size_t) k]; fresh = false; }
        const float floorMag = mx * 1.0e-5f;
        for (int k = 2; k < nb - 2; ++k)
        {
            const float m = mag[(size_t) k];
            if (m > floorMag && m > mag[(size_t) (k - 1)] && m >= mag[(size_t) (k + 1)] && m > mag[(size_t) (k - 2)] && m >= mag[(size_t) (k + 2)]) { peaks.push_back (k); isPeak[(size_t) k] = 1; }
        }
        // fattore di formante effettivo per l'inviluppo di destinazione
        const float eShift = keepFormants ? fRatio : fRatio * ratio;
        auto envAt = [&] (float pos) { if (pos <= 0) return env[0]; const int i0 = (int) pos; if (i0 >= nb - 1) return env[(size_t) (nb - 1)]; const float t = pos - i0; return env[(size_t) i0] * (1 - t) + env[(size_t) (i0 + 1)] * t; };
        for (size_t pi = 0; pi < peaks.size(); ++pi)
        {
            const int p = peaks[pi];
            const int lo = pi == 0 ? 0 : (peaks[pi - 1] + p + 1) / 2;
            const int hi = pi + 1 == peaks.size() ? nb - 1 : (p + peaks[pi + 1]) / 2;
            const int j0 = juce::roundToInt (p * ratio), shift = j0 - p;
            acc2[(size_t) p] += H * ratio * fq[(size_t) p];
            if (j0 <= 0 || j0 >= nb - 1) continue;
            const float rot = acc2[(size_t) p] - ph[(size_t) p];
            const float cr = std::cos (rot), sr = std::sin (rot);
            for (int k = lo; k <= hi; ++k)
            {
                const int j = k + shift; if (j < 0 || j >= nb) continue;
                float m = mag[(size_t) k];
                if (formants) m = m / env[(size_t) k] * envAt (j / eShift);
                // fase del bin = fase d'ingresso ruotata come il suo picco (aggancio di fase)
                const float c = cs[(size_t) k], s = sn[(size_t) k];
                yRe[(size_t) j] += m * (c * cr - s * sr); yIm[(size_t) j] += m * (s * cr + c * sr);
            }
        }
        // gli accumulatori dei bin non di picco seguono la loro frequenza (per quando un picco si sposta)
        for (int k = 0; k < nb; ++k) { if (! isPeak[(size_t) k]) acc2[(size_t) k] += H * ratio * fq[(size_t) k]; acc2[(size_t) k] = wrapPi (acc2[(size_t) k]); }

        // sintesi
        for (int k = 0; k < nb; ++k) { work[(size_t) (2 * k)] = yRe[(size_t) k]; work[(size_t) (2 * k + 1)] = yIm[(size_t) k]; }
        for (int k = nb; k < N; ++k) { work[(size_t) (2 * k)] = yRe[(size_t) (N - k)]; work[(size_t) (2 * k + 1)] = -yIm[(size_t) (N - k)]; }
        fft->performRealOnlyInverseTransform (work.data());
        for (int i = 0; i < N; ++i) outAcc[(size_t) ((outPos + i) % (2 * N))] += work[(size_t) i] * win[(size_t) i] * olaGain;
    }

private:
    // inviluppo spettrale col cepstrum (liftering delle quefrenze basse)
    void envelope (float lifter, float mx)
    {
        const float eps = mx * 1.0e-4f + 1.0e-9f;
        for (int k = 0; k < nb; ++k) { cep[(size_t) (2 * k)] = std::log (mag[(size_t) k] + eps); cep[(size_t) (2 * k + 1)] = 0; }
        for (int k = nb; k < N; ++k) { cep[(size_t) (2 * k)] = cep[(size_t) (2 * (N - k))]; cep[(size_t) (2 * k + 1)] = 0; }
        fft->performRealOnlyInverseTransform (cep.data());      // cepstrum reale in cep[0..N)
        const int L = juce::jlimit (4, N / 2 - 1, (int) lifter);
        for (int i = 0; i < N; ++i)
        {
            const int q = i <= N / 2 ? i : N - i;
            float w = q < L ? 1.0f : (q < L + 8 ? 0.5f + 0.5f * std::cos (juce::MathConstants<float>::pi * (q - L) / 8.0f) : 0.0f);
            cep[(size_t) i] *= w;
        }
        std::fill (cep.begin() + N, cep.end(), 0.0f);
        fft->performRealOnlyForwardTransform (cep.data(), true);
        for (int k = 0; k < nb; ++k) env[(size_t) k] = juce::jmax (1.0e-9f, std::exp (cep[(size_t) (2 * k)]));
    }

    int N = 2048, H = 256, nb = 1025, inPos = 0, outPos = 0; float olaGain = 1;
    std::unique_ptr<juce::dsp::FFT> fft;
    std::vector<float> win, inBuf, outAcc, work, cep, mag, ph, prevPh, acc2, fq, env, yRe, yIm, cs, sn;
    std::vector<int> peaks; std::vector<char> isPeak; bool fresh = true;
};

// --------------------------------------------------------------------------- rilevatore di intonazione (YIN decimato)
class PitchDetector
{
public:
    void prepare (double sampleRate, int frameLen)
    {
        sr = sampleRate; dec = juce::jmax (1, (int) std::round (sr / 11025.0)); dsr = sr / dec;
        len = juce::jmax (256, frameLen / dec);
        buf.assign ((size_t) len, 0); d.assign ((size_t) len, 0); pos = 0; accN = 0; accV = 0;
        lp1 = lp2 = 0; f0 = 0; conf = 0;
    }
    inline void push (float x)
    {
        // passa basso semplice prima della decimazione
        const float a = (float) std::exp (-kTwoPi * 2000.0 / sr);
        lp1 = a * lp1 + (1 - a) * x; lp2 = a * lp2 + (1 - a) * lp1;
        accV += lp2; if (++accN >= dec) { buf[(size_t) pos] = accV / dec; pos = (pos + 1) % len; accV = 0; accN = 0; }
    }
    // ritorna f0 in Hz (0 se non intonato)
    float detect()
    {
        const int tauMin = juce::jmax (2, (int) (dsr / 1000.0)), tauMax = juce::jmin (len / 2, (int) (dsr / 55.0));
        const int W = len - tauMax;
        std::vector<float>& x = scratch; x.resize ((size_t) len);
        double e = 0; for (int i = 0; i < len; ++i) { x[(size_t) i] = buf[(size_t) ((pos + i) % len)]; e += x[(size_t) i] * x[(size_t) i]; }
        if (e / len < 1.0e-7) { conf = 0; return f0 = 0; }
        d[0] = 1; double run = 0;
        for (int tau = 1; tau <= tauMax; ++tau)
        {
            double s = 0; for (int i = 0; i < W; ++i) { const float df = x[(size_t) i] - x[(size_t) (i + tau)]; s += df * df; }
            run += s; d[(size_t) tau] = (float) (run > 0 ? s * tau / run : 1);
        }
        int best = -1;
        for (int tau = tauMin; tau < tauMax; ++tau)
            if (d[(size_t) tau] < 0.15f) { while (tau + 1 < tauMax && d[(size_t) (tau + 1)] < d[(size_t) tau]) ++tau; best = tau; break; }
        if (best < 0) { int bm = tauMin; for (int tau = tauMin; tau < tauMax; ++tau) if (d[(size_t) tau] < d[(size_t) bm]) bm = tau; if (d[(size_t) bm] < 0.3f) best = bm; }
        if (best < 0) { conf = 0; return f0 = 0; }
        // interpolazione parabolica
        float t = (float) best;
        if (best > 1 && best < tauMax - 1) { const float a = d[(size_t) (best - 1)], b = d[(size_t) best], c = d[(size_t) (best + 1)]; const float den = a - 2 * b + c; if (std::abs (den) > 1e-9f) t += 0.5f * (a - c) / den; }
        conf = 1 - d[(size_t) best];
        return f0 = (float) (dsr / t);
    }
    float f0 = 0, conf = 0;
private:
    double sr = 44100, dsr = 11025; int dec = 4, len = 512, pos = 0, accN = 0; float accV = 0, lp1 = 0, lp2 = 0;
    std::vector<float> buf, d, scratch;
};

// --------------------------------------------------------------------------- note e scale
inline const juce::StringArray& noteNames() { static const juce::StringArray n { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }; return n; }
inline juce::String noteOf (float hz) { if (hz <= 0) return "-"; const int m = juce::roundToInt (69 + 12 * std::log2 (hz / 440.0f)); return noteNames()[((m % 12) + 12) % 12] + juce::String (m / 12 - 1); }
inline float midiOf (float hz) { return 69.0f + 12.0f * std::log2 (juce::jmax (1.0f, hz) / 440.0f); }
inline float hzOf (float midi) { return 440.0f * std::pow (2.0f, (midi - 69.0f) / 12.0f); }

// maschere di scala (bit 0 = tonica)
inline int scaleMask (int s)
{
    static const int m[] = { 0xFFF,                                              // cromatica
                             (1 << 0) | (1 << 2) | (1 << 4) | (1 << 5) | (1 << 7) | (1 << 9) | (1 << 11),   // maggiore
                             (1 << 0) | (1 << 2) | (1 << 3) | (1 << 5) | (1 << 7) | (1 << 8) | (1 << 10),   // minore naturale
                             (1 << 0) | (1 << 2) | (1 << 3) | (1 << 5) | (1 << 7) | (1 << 8) | (1 << 11),   // minore armonica
                             (1 << 0) | (1 << 3) | (1 << 5) | (1 << 7) | (1 << 10),                         // pentatonica minore
                             (1 << 0) | (1 << 1) | (1 << 3) | (1 << 5) | (1 << 7) | (1 << 8) | (1 << 10) }; // frigia
    return m[juce::jlimit (0, 5, s)];
}
inline float snapMidi (float midi, int key, int scale)
{
    const int mask = scaleMask (scale); float best = midi; float bd = 1e9f;
    const int c = juce::roundToInt (midi);
    for (int n = c - 6; n <= c + 6; ++n) if (mask & (1 << (((n - key) % 12 + 12) % 12))) { const float dd = std::abs (n - midi); if (dd < bd) { bd = dd; best = (float) n; } }
    return best;
}
}
