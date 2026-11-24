#pragma once
// Il suono interno di FLUX: senza instradare niente in Ableton si sente subito il beat.
// Cassa, rullante, hi-hat, perc sintetizzati; 808 monofonico con glide; melodia a campanello morbido (seni, niente aliasing),
// contro-melodia a pad, chop a campana FM. Tutto deterministico (stesso MIDI = stesso suono con ogni buffer).
#include "FluxEngine.h"

namespace flux
{
class Kit
{
public:
    void prepare (double s) { sr = s; for (auto& v : vs) v = {}; m8 = {}; nz = 0x12345u; }
    float vol = 1.0f;
    void trigger (int lane, int note, int vel, double lenSamples)
    {
        const float a = vel / 127.0f;
        if (lane == BASS)
        {   // 808: se la nota prima suona ancora, scivola
            const bool slide = m8.on && m8.left > 0;
            m8.tgt = hz (note); if (! slide) { m8.f = m8.tgt; m8.ph = 0; m8.t = 0; }
            m8.amp = a; m8.on = true; m8.left = lenSamples; m8.rel = 1.0f; m8.slide = slide;
            return;
        }
        Voice* v = nullptr; float qd = 1e9f;
        for (auto& x : vs) { if (! x.on) { v = &x; break; } if (x.env < qd) { qd = x.env; v = &x; } }   // libera, o la piu' quieta
        *v = {}; v->on = true; v->lane = lane; v->note = note; v->a = a; v->left = lenSamples; v->f = hz (note); v->env = 1;
        if (lane == HATS || (lane == PERC && note == 46)) { v->open = note == 46 || lenSamples > 0.2 * sr; v->hp1.hp (sr, 7000, 0.7); v->hp2.hp (sr, 9000, 0.7); }
        if (lane == SNARE || lane == PERC) { v->bp.bp (sr, lane == SNARE ? 2200 : 1250, lane == SNARE ? 0.8 : 1.4); }
    }
    void render (float* L, float* R, int n)
    {
        for (int i = 0; i < n; ++i)
        {
            float l = 0, r = 0;
            for (auto& v : vs) if (v.on) { float pl, pr; tick (v, pl, pr); l += pl; r += pr; }
            const float b = tick808(); l += b; r += b;
            L[i] += std::tanh (l * vol * 1.8f) * 0.95f; R[i] += std::tanh (r * vol * 1.8f) * 0.95f;
        }
    }
private:
    struct BP { float a0 = 0, a1 = 0, a2 = 0, b1 = 0, b2 = 0, z1 = 0, z2 = 0;
        void bp (double sr, double f, double q) { const double w = juce::MathConstants<double>::twoPi * f / sr, c = std::cos (w), al = std::sin (w) / (2 * q), a0n = 1 + al; a0 = (float) (al / a0n); a1 = 0; a2 = (float) (-al / a0n); b1 = (float) (-2 * c / a0n); b2 = (float) ((1 - al) / a0n); }
        void hp (double sr, double f, double q) { const double w = juce::MathConstants<double>::twoPi * f / sr, c = std::cos (w), al = std::sin (w) / (2 * q), a0n = 1 + al; a0 = (float) ((1 + c) / 2 / a0n); a1 = (float) (-(1 + c) / a0n); a2 = a0; b1 = (float) (-2 * c / a0n); b2 = (float) ((1 - al) / a0n); }
        float p (float x) { const float y = a0 * x + z1; z1 = a1 * x - b1 * y + z2; z2 = a2 * x - b2 * y; return y; } };
    struct Voice { bool on = false, open = false; int lane = 0, note = 0; float a = 0, f = 440, env = 0, ph = 0, ph2 = 0, t = 0; double left = 0; BP bp, hp1, hp2; };
    struct M8 { bool on = false, slide = false; float f = 55, tgt = 55, ph = 0, t = 0, amp = 0, rel = 1; double left = 0; } m8;
    std::array<Voice, 24> vs; double sr = 48000; juce::uint32 nz = 0x12345u;
    static float hz (int n) { return 440.0f * std::pow (2.0f, (n - 69) / 12.0f); }
    float noise() { nz ^= nz << 13; nz ^= nz >> 17; nz ^= nz << 5; return (float) (nz >> 8) * (2.0f / 16777216.0f) - 1.0f; }
    float tick808()
    {
        if (! m8.on) return 0;
        const float dt = 1.0f / (float) sr; m8.t += dt;
        m8.f += (m8.tgt - m8.f) * (m8.slide ? 0.0009f : 1.0f);
        const float punch = m8.slide ? 0.0f : 9.0f * std::exp (-m8.t / 0.025f);
        m8.ph += m8.f * std::pow (2.0f, punch / 12.0f) * dt; if (m8.ph >= 1) m8.ph -= 1;
        float y = std::sin (juce::MathConstants<float>::twoPi * m8.ph);
        y = std::tanh (y * 2.2f) * 0.8f;
        const float env = std::exp (-m8.t / 1.6f) * std::min (1.0f, m8.t * 600.0f);
        if (m8.left > 0) m8.left -= 1; else { m8.rel *= 0.9985f; if (m8.rel < 1e-4f) m8.on = false; }
        return y * env * m8.rel * m8.amp * 0.55f;
    }
    void tick (Voice& v, float& outL, float& outR)
    {
        const float dt = 1.0f / (float) sr; v.t += dt; float y = 0, pan = 0;
        const bool held = v.left > 0; if (held) v.left -= 1;
        switch (v.lane)
        {
            case KICK:
            {
                const float f = 48.0f + 110.0f * std::exp (-v.t / 0.03f); v.ph += f * dt; if (v.ph >= 1) v.ph -= 1;
                v.env = std::exp (-v.t / 0.32f); y = std::tanh (std::sin (juce::MathConstants<float>::twoPi * v.ph) * 1.8f) * v.env;
                if (v.t < 0.002f) y += noise() * 0.4f * (1 - v.t / 0.002f);
                y *= 0.85f; break;
            }
            case SNARE:
            {
                v.ph += 185.0f * dt; if (v.ph >= 1) v.ph -= 1;
                const float tone = std::sin (juce::MathConstants<float>::twoPi * v.ph) * std::exp (-v.t / 0.07f) * 0.5f;
                v.env = std::exp (-v.t / 0.13f); y = (tone + v.bp.p (noise()) * 2.2f * v.env) * 0.55f; pan = -0.05f; break;
            }
            case HATS:
            {
                v.env = std::exp (-v.t / (v.open ? 0.28f : 0.04f)); y = v.hp2.p (v.hp1.p (noise())) * v.env * 0.35f; pan = 0.25f; break;
            }
            case PERC:
            {
                if (v.note == 46) { v.env = std::exp (-v.t / 0.3f); y = v.hp2.p (v.hp1.p (noise())) * v.env * 0.3f; }
                else if (v.note == 75 || v.note == 37) { v.ph += 1750.0f * dt; if (v.ph >= 1) v.ph -= 1; v.env = std::exp (-v.t / 0.025f); y = std::sin (juce::MathConstants<float>::twoPi * v.ph) * v.env * 0.4f; }
                else
                {   // clap: tre colpi ravvicinati di rumore filtrato
                    const float t = v.t; const float bursts = (t < 0.011f ? std::exp (-std::fmod (t, 0.0045f) / 0.002f) : 0.0f) + std::exp (-juce::jmax (0.0f, t - 0.011f) / 0.12f) * (t >= 0.011f ? 1.0f : 0.0f);
                    v.env = bursts; y = v.bp.p (noise()) * bursts * 1.2f;
                }
                pan = -0.3f; break;
            }
            case MEL:
            {   // campanello morbido: seno + armonica, decade
                v.ph += v.f * dt; if (v.ph >= 1) v.ph -= 1; v.ph2 += v.f * 2.0f * dt; if (v.ph2 >= 1) v.ph2 -= 1;
                const float atk = std::min (1.0f, v.t * 400.0f);
                v.env = atk * std::exp (-v.t / 0.7f);
                if (! held) { v.a *= 0.9992f; }
                y = (std::sin (juce::MathConstants<float>::twoPi * v.ph) + 0.25f * std::sin (juce::MathConstants<float>::twoPi * v.ph2) * std::exp (-v.t / 0.15f)) * v.env * 0.28f;
                pan = -0.2f; break;
            }
            case COUNTER:
            {   // pad: due seni stonati appena, attacco lento
                v.ph += v.f * 1.003f * dt; if (v.ph >= 1) v.ph -= 1; v.ph2 += v.f * 0.997f * dt; if (v.ph2 >= 1) v.ph2 -= 1;
                const float atk = std::min (1.0f, v.t / 0.08f);
                if (! held) v.a *= 0.9996f;
                v.env = atk * v.a;
                const float s1 = std::sin (juce::MathConstants<float>::twoPi * v.ph), s2 = std::sin (juce::MathConstants<float>::twoPi * v.ph2);
                outL += s1 * v.env * 0.14f; outR += s2 * v.env * 0.14f; if (v.env < 1e-4f && ! held) v.on = false; return;
            }
            default:
            {   // chop: campana FM
                v.ph += v.f * dt; if (v.ph >= 1) v.ph -= 1; v.ph2 += v.f * 3.5f * dt; if (v.ph2 >= 1) v.ph2 -= 1;
                const float idx = 2.5f * std::exp (-v.t / 0.12f);
                v.env = std::exp (-v.t / 0.5f) * std::min (1.0f, v.t * 800.0f);
                y = std::sin (juce::MathConstants<float>::twoPi * v.ph + idx * std::sin (juce::MathConstants<float>::twoPi * v.ph2)) * v.env * 0.22f; pan = 0.35f; break;
            }
        }
        y *= v.a;
        if (v.lane != KICK && v.lane != SNARE && v.lane != HATS && v.lane != PERC && v.lane != MEL && v.lane != COUNTER) { if (! held) v.a *= 0.9993f; }
        if (v.lane == MEL && ! held && v.a < 1e-3f) v.on = false;
        if ((v.lane <= PERC || v.lane == CHOP) && v.env < 1e-4f && v.t > 0.01f) v.on = false;
        if (v.lane == CHOP && v.a < 1e-4f) v.on = false;
        outL += y * (1.0f - pan); outR += y * (1.0f + pan);
    }
};
}
