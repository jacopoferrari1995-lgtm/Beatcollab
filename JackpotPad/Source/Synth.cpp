#include "Synth.h"
#include "PadProcessor.h"

namespace jp
{
namespace
{
constexpr double SR = 44100.0;
constexpr double TWO_PI = 6.283185307179586;

struct Svf
{
    float g = 0, k = 1, ic1 = 0, ic2 = 0;
    void set (double fc, double q) { g = (float) std::tan (3.141592653589793 * juce::jlimit (10.0, SR * 0.45, fc) / SR); k = (float) (1.0 / q); }
    void run (float x, float& lp, float& bp, float& hp) { const float v3 = x - ic2, v1 = (ic1 + g * v3) / (1 + g * (g + k)), v2 = ic2 + g * v1; ic1 = 2 * v1 - ic1; ic2 = 2 * v2 - ic2; lp = v2; bp = v1; hp = x - k * v1 - v2; }
    float lp (float x) { float l, b, h; run (x, l, b, h); return l; }
    float bp (float x) { float l, b, h; run (x, l, b, h); return b; }
    float hp (float x) { float l, b, h; run (x, l, b, h); return h; }
};
inline float sq (double ph) { return std::fmod (ph, 1.0) < 0.5 ? 1.0f : -1.0f; }
inline float env (double t, double decay) { return (float) std::exp (-t * 4.6 / juce::jmax (0.005, decay)); }
inline double mtof (double m) { return 440.0 * std::pow (2.0, (m - 69.0) / 12.0); }
}

SamplePtr makeDrum (const DrumDesc& d)
{
    juce::Random rng (juce::String (d.name + juce::String (d.a) + juce::String (d.b)).hashCode());
    auto noise = [&rng] { return rng.nextFloat() * 2.0f - 1.0f; };
    double len = 0.5;
    switch (d.type)
    {
        case S_KICK: len = d.c * 1.6 + 0.05; break;     case S_808: len = d.b * 1.5 + 0.05; break;
        case S_SNARE: len = juce::jmax (d.b, d.c) * 1.6 + 0.05; break; case S_CLAP: len = d.b * 1.6 + 0.08 + d.d * 0.4; break;
        case S_HAT: len = d.a * 1.6 + 0.02; break;      case S_RIM: len = d.b * 2 + 0.03; break;
        case S_TOM: len = d.b * 1.6; break;              case S_SHAKER: len = d.a + 0.05; break;
        case S_CYMBAL: len = d.a * 1.4; break;           case S_BELL: len = d.c * 1.6; break;
        case S_SNAP: len = d.a * 2 + 0.03; break;        case S_STAB: len = d.c * 1.4 + 0.05; break;
        case S_CONGA: len = d.b * 1.6; break;            case S_CLAVE: len = d.b * 2 + 0.02; break;
        case S_NOISE: len = d.a; break;                  case S_PLUCK: len = d.b * 1.5; break;
        default: break;
    }
    const int n = (int) (juce::jlimit (0.05, 6.0, len) * SR);
    std::vector<float> y ((size_t) n, 0.0f);
    Svf f1, f2, f3; double ph = 0, ph2 = 0;
    switch (d.type)
    {
        case S_KICK:
        {
            f1.set (3000, 0.7);
            for (int i = 0; i < n; ++i)
            {
                const double t = i / SR, fr = d.b + (d.a - d.b) * std::exp (-t * 32);
                ph += fr / SR;
                float x = (float) std::sin (TWO_PI * ph) * env (t, d.c);
                if (t < 0.012) x += f1.hp (noise()) * d.d * (float) (1 - t / 0.012) * 0.6f + (float) std::sin (TWO_PI * 3200 * t) * d.d * 0.3f * (float) (1 - t / 0.012);
                y[(size_t) i] = std::tanh (x * (1 + d.e * 6)) / std::tanh (1 + d.e * 6);
            }
            break;
        }
        case S_808:
        {
            for (int i = 0; i < n; ++i)
            {
                const double t = i / SR, fr = d.a * (1 + d.d * 1.6 * std::exp (-t * 38));
                ph += fr / SR;
                const float a = juce::jmin (1.0f, (float) (t / 0.003)) * env (t, d.b);
                float x = (float) std::sin (TWO_PI * ph) * a;
                if (t < 0.01) x += (float) std::sin (TWO_PI * 1800 * t) * d.e * 0.35f * (float) (1 - t / 0.01);
                const float k = 1 + d.c * 9; y[(size_t) i] = std::tanh (x * k) / std::tanh (k);
            }
            break;
        }
        case S_SNARE:
        {
            f1.set (1500 + d.d * 4500, 0.7); f2.set (9000, 0.7);
            for (int i = 0; i < n; ++i)
            {
                const double t = i / SR;
                const float tone = (float) (std::sin (TWO_PI * d.a * t * (1 + 0.15 * std::exp (-t * 60))) + 0.5 * std::sin (TWO_PI * d.a * 1.62 * t)) * env (t, d.c) * d.e;
                const float nz = f2.lp (f1.hp (noise())) * env (t, d.b) * (1.0f - 0.3f * d.e);
                y[(size_t) i] = std::tanh ((tone * 0.7f + nz) * 1.4f);
            }
            break;
        }
        case S_CLAP:
        {
            f1.set (d.c, 1.4); f2.set (d.c * 0.8, 0.6);
            const double sp = d.a / 1000.0;
            for (int i = 0; i < n; ++i)
            {
                const double t = i / SR; float e = 0;
                for (int k = 0; k < 3; ++k) { const double tk = t - k * sp; if (tk >= 0 && tk < sp) e = juce::jmax (e, (float) std::exp (-tk * 180)); }
                if (t >= 3 * sp) e = juce::jmax (e, env (t - 3 * sp, d.b));
                const float nz = noise();
                float x = f1.bp (nz) * e * 2.2f;
                x += f2.lp (nz) * d.d * env (t, d.b * 3 + 0.2) * 0.35f;
                y[(size_t) i] = std::tanh (x * 1.5f);
            }
            break;
        }
        case S_HAT: case S_CYMBAL:
        {
            const bool cym = d.type == S_CYMBAL;
            static const double ratios[6] = { 205.3, 304.4, 369.6, 522.7, 540.0, 800.0 };
            const double scale = (cym ? 7.5 : d.b) / 7.0 * 1.6;
            const double decay = d.a;
            double phs[6] = {};
            f1.set ((cym ? 5000.0 : d.b * 900.0), 0.7); f2.set (cym ? 14000.0 : 16000.0, 0.7); f3.set (cym ? 3200 : 9000, 3.0);
            for (int i = 0; i < n; ++i)
            {
                const double t = i / SR; float m = 0;
                for (int k = 0; k < 6; ++k) { phs[k] += ratios[k] * scale / SR; m += sq (phs[k]); }
                const float metal = cym ? (1 - d.b * 0.5f) : d.c;
                float x = (m / 6.0f) * metal + noise() * (1.0f - metal * 0.6f);
                x = f2.lp (f1.hp (x));
                if (cym && d.c > 0) x += f3.bp (x) * d.c;
                const float a = juce::jmin (1.0f, (float) (t / 0.0008)) * env (t, decay);
                y[(size_t) i] = x * a * 1.3f;
            }
            break;
        }
        case S_RIM:
        {
            f1.set (d.a * 1.2, 4);
            for (int i = 0; i < n; ++i) { const double t = i / SR; y[(size_t) i] = ((float) std::sin (TWO_PI * d.a * t) * 0.8f + f1.bp (noise()) * 0.8f) * env (t, d.b); }
            break;
        }
        case S_TOM: case S_CONGA:
        {
            for (int i = 0; i < n; ++i)
            {
                const double t = i / SR, fr = d.a * (1 + (d.type == S_CONGA ? 0.25 : d.c) * std::exp (-t * (d.type == S_CONGA ? 60 : 14)));
                ph += fr / SR;
                y[(size_t) i] = ((float) std::sin (TWO_PI * ph) + noise() * 0.06f * env (t, 0.02)) * env (t, d.b);
            }
            break;
        }
        case S_SHAKER:
        {
            f1.set (d.b * 1000, 0.8);
            for (int i = 0; i < n; ++i) { const double t = i / SR; const float a = (float) juce::jmin (1.0, t / 0.02) * env (juce::jmax (0.0, t - 0.02), d.a * 0.6); y[(size_t) i] = f1.hp (noise()) * a; }
            break;
        }
        case S_BELL:
        {
            f1.set ((d.a + d.b) * 0.5, 1.2);
            for (int i = 0; i < n; ++i) { const double t = i / SR; ph += d.a / SR; ph2 += d.b / SR; const float x = (sq (ph) + sq (ph2)) * 0.5f; y[(size_t) i] = f1.bp (x) * 2.0f * (env (t, 0.02) * 0.6f + env (t, d.c) * 0.4f); }
            break;
        }
        case S_SNAP:
        {
            f1.set (d.b, 2.2);
            for (int i = 0; i < n; ++i) { const double t = i / SR; float e = (float) std::exp (-t * 300); if (t > 0.006) e = juce::jmax (e, (float) std::exp (-(t - 0.006) * 4.6 / d.a)); y[(size_t) i] = f1.bp (noise()) * e * 2.5f; }
            break;
        }
        case S_STAB:
        {
            static const int chords[6][5] = { { 0, 3, 7, 10, -1 }, { 0, 4, 7, 11, -1 }, { 0, 3, 7, 10, 14 }, { 0, 2, 7, 12, -1 }, { 0, 4, 7, 11, 14 }, { 0, 3, 7, 12, -1 } };
            const int ct = juce::jlimit (0, 5, (int) d.b); double phs[5][3] = {};
            f1.set (800 + d.d * 5000, 0.9);
            for (int i = 0; i < n; ++i)
            {
                const double t = i / SR; float x = 0; int cnt = 0;
                for (int k = 0; k < 5; ++k)
                {
                    if (chords[ct][k] < 0) continue; ++cnt;
                    for (int u = 0; u < 3; ++u) { const double det = (u - 1) * 0.004; phs[k][u] += mtof (d.a + chords[ct][k]) * (1 + det) / SR; x += (float) (2.0 * std::fmod (phs[k][u], 1.0) - 1.0); }
                }
                x /= (float) (cnt * 3);
                y[(size_t) i] = f1.lp (x) * juce::jmin (1.0f, (float) (t / 0.004)) * env (t, d.c) * 2.0f;
            }
            break;
        }
        case S_CLAVE:
        {
            for (int i = 0; i < n; ++i) { const double t = i / SR; y[(size_t) i] = (float) std::sin (TWO_PI * d.a * t) * env (t, d.b); }
            break;
        }
        case S_NOISE:
        {
            f1.set (d.b * 1000, 0.7);
            for (int i = 0; i < n; ++i) { float x = f1.lp (noise()) * 0.25f; if (rng.nextFloat() < d.c * 0.002f) x += noise() * 0.9f; y[(size_t) i] = x; }
            break;
        }
        case S_PLUCK:
        {
            const int p = juce::jmax (2, (int) (SR / mtof (d.a))); std::vector<float> dl ((size_t) p); for (auto& v : dl) v = noise();
            int w = 0; float last = 0; const float damp = 0.4f + 0.55f * d.c, fb = (float) std::exp (-1.0 / (d.b * SR / p * 1.2));
            for (int i = 0; i < n; ++i) { const float v = dl[(size_t) w]; const float nv = (v * damp + last * (1 - damp)) * fb; last = v; dl[(size_t) w] = nv; w = (w + 1) % p; y[(size_t) i] = v; }
            break;
        }
        default: break;
    }
    // livello: non sul picco ma su quanto suona forte (RMS dei primi 300 ms), con un bersaglio per tipo di suono.
    // Cosi hat, snap e rim non restano 20 dB sotto cassa e 808.
    float peak = 0; for (auto v : y) peak = juce::jmax (peak, std::abs (v));
    float k = peak > 0 ? 0.9f / peak : 1.0f;
    {
        const int m = juce::jmin (n, (int) (SR * 0.3)); double e = 0; for (int i = 0; i < m; ++i) e += (double) y[(size_t) i] * y[(size_t) i];
        const float rmsDb = juce::Decibels::gainToDecibels ((float) std::sqrt (e / juce::jmax (1, m)) * k, -100.0f);
        const bool open = d.name.toLowerCase().contains ("open");
        float target = -14.0f;
        switch (d.type)
        {
            case S_KICK: target = -10.0f; break;   case S_808: target = -10.0f; break;
            case S_SNARE: target = -12.5f; break;  case S_CLAP: case S_SNAP: target = -13.0f; break;
            case S_HAT: target = open ? -16.0f : -17.0f; break;
            case S_TOM: target = -12.5f; break;    case S_CYMBAL: target = -18.0f; break;
            case S_STAB: case S_PLUCK: target = -14.0f; break;
            case S_NOISE: target = -24.0f; break;
            default: target = -15.0f; break;      // rim, shaker, bell, conga, clave
        }
        const float extraDb = juce::jlimit (-12.0f, 9.0f, target - rmsDb);
        k *= juce::Decibels::decibelsToGain (extraDb);
    }
    // limitatore morbido: i transienti che superano il tetto vengono arrotondati, non tagliati
    const float ceil = 0.97f, th = 0.75f;
    auto soft = [=] (float v) { const float a = std::abs (v); if (a <= th) return v; const float o = th + (ceil - th) * std::tanh ((a - th) / (ceil - th)); return v < 0 ? -o : o; };
    auto s = std::make_shared<SampleData>();
    s->sr = SR; s->name = d.name; s->path = descToPath (d);
    s->buf.setSize (2, n);
    const int fade = juce::jmin (400, n / 4);
    for (int i = 0; i < n; ++i) { float v = soft (y[(size_t) i] * k); if (i >= n - fade) v *= (float) (n - 1 - i) / fade; s->buf.setSample (0, i, v); s->buf.setSample (1, i, v); }
    return s;
}

juce::String descToPath (const DrumDesc& d)
{
    return "gen:" + juce::String (d.type) + ":" + juce::String (d.a, 4) + ":" + juce::String (d.b, 4) + ":" + juce::String (d.c, 4) + ":" + juce::String (d.d, 4) + ":" + juce::String (d.e, 4) + ":" + d.name;
}
bool pathToDesc (const juce::String& p, DrumDesc& d)
{
    if (! p.startsWith ("gen:")) return false;
    auto t = juce::StringArray::fromTokens (p.substring (4), ":", "");
    if (t.size() < 7) return false;
    d.type = t[0].getIntValue(); d.a = t[1].getFloatValue(); d.b = t[2].getFloatValue(); d.c = t[3].getFloatValue(); d.d = t[4].getFloatValue(); d.e = t[5].getFloatValue();
    t.removeRange (0, 6); d.name = t.joinIntoString (":");
    return true;
}
}
