#include "KitGen.h"

namespace j60
{
namespace
{
constexpr double SR = 44100.0;
const double TWO_PI = juce::MathConstants<double>::twoPi;

struct Gen
{
    std::vector<float> d;
    juce::Random rnd { 4242 };
    explicit Gen (double seconds) : d ((size_t) (seconds * SR), 0.0f) {}
    float noise() { return rnd.nextFloat() * 2.0f - 1.0f; }
    size_t n() const { return d.size(); }
};

float softclip (float x, float k) { return (float) (std::tanh (x * k) / std::tanh (k)); }

// filtri semplici
struct OnePoleLP { float a = 0, z = 0; void set (double fc) { a = (float) std::exp (-TWO_PI * fc / SR); } float p (float x) { z = (1 - a) * x + a * z; return z; } };
struct OnePoleHP { OnePoleLP lp; void set (double fc) { lp.set (fc); } float p (float x) { return x - lp.p (x); } };
struct BP { float ic1 = 0, ic2 = 0, g = 0, k = 1;
    void set (double fc, double q) { g = (float) std::tan (juce::MathConstants<double>::pi * fc / SR); k = (float) (1.0 / q); }
    float p (float x) { float v3 = x - ic2; float v1 = (ic1 + g * v3) / (1 + g * (g + k)); float v2 = ic2 + g * v1; ic1 = 2 * v1 - ic1; ic2 = 2 * v2 - ic2; return v1; } };

SamplePtr finish (Gen& g, const juce::String& name, float gain = 0.9f)
{
    float peak = 0; for (auto v : g.d) peak = std::max (peak, std::abs (v));
    auto s = std::make_shared<SampleData>();
    s->sr = SR; s->name = name;
    s->buf.setSize (2, (int) g.n());
    const float k = peak > 0 ? gain / peak : 1.0f;
    for (int i = 0; i < (int) g.n(); ++i) { s->buf.setSample (0, i, g.d[(size_t) i] * k); s->buf.setSample (1, i, g.d[(size_t) i] * k); }
    // micro fade finale
    const int f = std::min (256, (int) g.n());
    for (int i = 0; i < f; ++i) { const float w = (float) i / (float) f; const int j = (int) g.n() - 1 - i; s->buf.setSample (0, j, s->buf.getSample (0, j) * w); s->buf.setSample (1, j, s->buf.getSample (1, j) * w); }
    return s;
}

SamplePtr kick()
{
    Gen g (0.55); double ph = 0;
    for (size_t i = 0; i < g.n(); ++i) { const double t = (double) i / SR; const double f = 46 + 110 * std::exp (-t * 28);
        ph += TWO_PI * f / SR; const double env = std::exp (-t * 6.5); float click = t < 0.004 ? g.noise() * (float) (1 - t / 0.004) * 0.4f : 0.0f;
        g.d[i] = softclip ((float) (std::sin (ph) * env) + click, 1.8f); }
    return finish (g, "Kick 60");
}
SamplePtr snare()
{
    Gen g (0.32); BP bp; bp.set (2200, 0.8); OnePoleHP hp; hp.set (500); double ph = 0;
    for (size_t i = 0; i < g.n(); ++i) { const double t = (double) i / SR; ph += TWO_PI * (185 + 40 * std::exp (-t * 40)) / SR;
        const float body = (float) (std::sin (ph) * std::exp (-t * 22)) * 0.6f; const float nz = hp.p (bp.p (g.noise()) * 2.2f) * (float) std::exp (-t * 13);
        g.d[i] = softclip (body + nz, 1.5f); }
    return finish (g, "Snare Dusty");
}
SamplePtr clap()
{
    Gen g (0.38); BP bp; bp.set (1400, 1.2);
    for (size_t i = 0; i < g.n(); ++i) { const double t = (double) i / SR; double env = 0;
        for (int k = 0; k < 3; ++k) { const double tk = t - k * 0.011; if (tk >= 0 && tk < 0.012) env = std::max (env, std::exp (-tk * 260)); }
        if (t >= 0.033) env = std::max (env, std::exp (-(t - 0.033) * 17));
        g.d[i] = bp.p (g.noise()) * (float) env * 2.5f; }
    return finish (g, "Clap Room");
}
SamplePtr hat (bool open)
{
    Gen g (open ? 0.6 : 0.09); OnePoleHP hp; hp.set (7000); double ph[6] = {}; const double fr[6] = { 205.3, 304.4, 369.6, 522.7, 540.0, 800.0 };
    for (size_t i = 0; i < g.n(); ++i) { const double t = (double) i / SR; float m = 0;
        for (int k = 0; k < 6; ++k) { ph[k] += fr[k] * 4.0 / SR; m += (std::fmod (ph[k], 1.0) < 0.5 ? 1.0f : -1.0f); }
        const float x = hp.p (m * 0.18f + g.noise() * 0.5f); g.d[i] = x * (float) std::exp (-t * (open ? 6.0 : 55.0)); }
    return finish (g, open ? "Open Hat" : "Closed Hat", open ? 0.7f : 0.75f);
}
SamplePtr rim()
{
    Gen g (0.08); BP bp; bp.set (1700, 4); double ph = 0;
    for (size_t i = 0; i < g.n(); ++i) { const double t = (double) i / SR; ph += TWO_PI * 1650 / SR;
        g.d[i] = (float) (std::sin (ph) * std::exp (-t * 70)) * 0.7f + bp.p (g.noise()) * (float) std::exp (-t * 120); }
    return finish (g, "Rimshot");
}
SamplePtr shaker()
{
    Gen g (0.14); OnePoleHP hp; hp.set (5000);
    for (size_t i = 0; i < g.n(); ++i) { const double t = (double) i / SR; const double env = std::sin (juce::jmin (1.0, t / 0.03) * 1.5708) * std::exp (-t * 30);
        g.d[i] = hp.p (g.noise()) * (float) env; }
    return finish (g, "Shaker", 0.6f);
}
SamplePtr tom (double f0, const char* name)
{
    Gen g (0.5); double ph = 0;
    for (size_t i = 0; i < g.n(); ++i) { const double t = (double) i / SR; ph += TWO_PI * (f0 + f0 * 0.6 * std::exp (-t * 18)) / SR;
        g.d[i] = (float) (std::sin (ph) * std::exp (-t * 7)) + g.noise() * 0.05f * (float) std::exp (-t * 40); }
    return finish (g, name);
}
SamplePtr crash()
{
    Gen g (1.6); OnePoleHP hp; hp.set (4000); double ph[5] = {}; const double fr[5] = { 3150, 4410, 5330, 6020, 7400 };
    for (size_t i = 0; i < g.n(); ++i) { const double t = (double) i / SR; float m = 0;
        for (int k = 0; k < 5; ++k) { ph[k] += fr[k] / SR; m += (float) std::sin (TWO_PI * ph[k]); }
        g.d[i] = hp.p (g.noise() * 0.8f + m * 0.1f) * (float) std::exp (-t * 2.2); }
    return finish (g, "Crash Lo-Fi", 0.6f);
}
SamplePtr bass808()
{
    // La1 (55 Hz): accordalo con TUNE per suonare le altre note della scala
    Gen g (1.6); double ph = 0;
    for (size_t i = 0; i < g.n(); ++i) { const double t = (double) i / SR; ph += TWO_PI * (55.0 + 25 * std::exp (-t * 35)) / SR;
        const double env = (t < 0.004 ? t / 0.004 : 1.0) * std::exp (-t * 1.6);
        g.d[i] = softclip ((float) (std::sin (ph) * env), 2.4f) * (float) env; }
    return finish (g, "808 A1");
}
SamplePtr snap()
{
    Gen g (0.15); BP bp; bp.set (2600, 2);
    for (size_t i = 0; i < g.n(); ++i) { const double t = (double) i / SR; g.d[i] = bp.p (g.noise()) * (float) std::exp (-t * 45) * 3.0f; }
    return finish (g, "Finger Snap");
}
SamplePtr cowbell()
{
    Gen g (0.4); BP bp; bp.set (800, 3); double a = 0, b = 0;
    for (size_t i = 0; i < g.n(); ++i) { const double t = (double) i / SR; a += 540.0 / SR; b += 800.0 / SR;
        const float sq = (std::fmod (a, 1.0) < 0.5 ? 1.0f : -1.0f) + (std::fmod (b, 1.0) < 0.5 ? 1.0f : -1.0f);
        g.d[i] = bp.p (sq) * (float) std::exp (-t * 9) * 1.6f; }
    return finish (g, "Cowbell 808", 0.7f);
}
SamplePtr woodblock()
{
    Gen g (0.12); double ph = 0;
    for (size_t i = 0; i < g.n(); ++i) { const double t = (double) i / SR; ph += TWO_PI * 980 / SR; g.d[i] = (float) (std::sin (ph) * std::exp (-t * 55)); }
    return finish (g, "Woodblock");
}
SamplePtr voxHey()
{
    Gen g (0.32); BP f1, f2; f1.set (700, 5); f2.set (1220, 6); double ph = 0;
    for (size_t i = 0; i < g.n(); ++i) { const double t = (double) i / SR; ph += TWO_PI * (180 - 50 * t) / SR;
        const float src = (float) (std::fmod (ph / TWO_PI, 1.0) * 2 - 1) * 0.6f + g.noise() * 0.15f;
        const double env = juce::jmin (1.0, t / 0.015) * std::exp (-t * 6);
        g.d[i] = (f1.p (src) * 1.2f + f2.p (src) * 0.8f) * (float) env * 3.0f; }
    return finish (g, "Vox Hey", 0.7f);
}
SamplePtr stab()
{
    // Am7: La Do Mi Sol (tutte note di La minore naturale)
    Gen g (1.2); const double notes[4] = { 220.0, 261.63, 329.63, 392.0 }; double ph[8] = {}; OnePoleLP lp;
    for (size_t i = 0; i < g.n(); ++i) { const double t = (double) i / SR; float s = 0;
        for (int k = 0; k < 4; ++k) for (int d = 0; d < 2; ++d) { ph[k * 2 + d] += notes[k] * (d ? 1.004 : 0.997) / SR; s += (float) (std::fmod (ph[k * 2 + d], 1.0) * 2 - 1); }
        lp.set (500 + 3500 * std::exp (-t * 7)); g.d[i] = lp.p (s * 0.15f) * (float) (juce::jmin (1.0, t / 0.005) * std::exp (-t * 2.5)); }
    return finish (g, "Stab Am7", 0.75f);
}
}

juce::String builtinName (int i)
{
    static const char* names[] = { "Kick 60", "Snare Dusty", "Clap Room", "Closed Hat", "Open Hat", "Rimshot", "Shaker", "Tom Low",
                                   "Tom High", "Crash Lo-Fi", "808 A1", "Finger Snap", "Cowbell 808", "Woodblock", "Vox Hey", "Stab Am7" };
    return (i >= 0 && i < kBuiltinCount) ? names[i] : "";
}

SamplePtr makeBuiltin (int i)
{
    SamplePtr s;
    switch (i)
    {
        case 0: s = kick(); break;      case 1: s = snare(); break;     case 2: s = clap(); break;      case 3: s = hat (false); break;
        case 4: s = hat (true); break;  case 5: s = rim(); break;       case 6: s = shaker(); break;    case 7: s = tom (95, "Tom Low"); break;
        case 8: s = tom (160, "Tom High"); break; case 9: s = crash(); break; case 10: s = bass808(); break; case 11: s = snap(); break;
        case 12: s = cowbell(); break;  case 13: s = woodblock(); break; case 14: s = voxHey(); break;  case 15: s = stab(); break;
        default: return nullptr;
    }
    s->path = "builtin:" + juce::String (i);
    return s;
}
}
