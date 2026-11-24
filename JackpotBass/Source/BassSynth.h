#pragma once
// Il suono per ascoltare la linea: una voce sola (il basso e' monofonico), tre motori.
// 808: seno con colpo di pitch, decay lungo, drive e glide. BASSO: corda Karplus-Strong scura (dita), stoppate.
// SYNTH: dente di sega + sub, filtro passa-basso risonante con inviluppo e accento (acid), glide.
#include "GtrSynth.h"
#include "BassGen.h"

namespace bss
{
class Mono
{
public:
    void prepare (double s) { sr = s; ks[0].prepare (sr); ks[1].prepare (sr); reset(); }
    void reset() { ctr = 0; ph = sub = 0; env = 0; on = false; penv = 0; curHz = tgtHz = 55; ic1 = ic2 = 0; fenv = 0; ks[0].reset(); ks[1].reset(); dcx = dcy = 0; lp1 = lp2 = 0; clickN = 0; note = -1; }
    int mode = B808; float tone = 0.5f, drive = 0.3f, decay = 0.6f, punch = 0.5f;
    void noteOn (int n, float vel, bool legato, bool mute)
    {
        const float hz = 440.0f * std::pow (2.0f, (n - 69) / 12.0f);
        const bool slide = legato && on && env > 0.05f;
        tgtHz = hz; if (! slide) curHz = hz;
        glideK = slide ? (float) (1.0 - std::exp (-1.0 / (0.065 * sr))) : 1.0f;
        velS = vel; held = true; note = n;
        if (mode == BASSO)
        {
            if (slide && ks[kc].on) { ks[kc].legato (hz, 2, vel, rnd); return; }
            ks[kc].fade(); kc ^= 1; ks[kc].pluck (hz, vel, gtr::PULITA, 0.25f + tone * 0.5f, mute, false, rnd);
            on = true; return;
        }
        if (! slide)
        {
            penv = mode == B808 ? punch * 14.0f : 0.0f;          // 808: la nota parte piu' alta e scende subito (il colpo)
            fenv = 1.0f; accent = vel > 0.82f ? 1.0f : 0.0f;
            clickN = mode == B808 ? (int) (0.003 * sr) : 0;
        }
        on = true; rel = false; mutedHit = mute;
    }
    void noteOff (int n) { if (n != note) return; held = false; rel = true; if (mode == BASSO) ks[kc].release(); }
    void allOff() { held = false; rel = true; note = -1; ks[0].fade(); ks[1].fade(); }
    void render (float* out, int n)
    {
        const float tone0 = tone, drive0 = drive, decay0 = decay, punch0 = punch;
        float tone = toneS, drive = driveS, decay = decayS, punch = punchS;
        float kRel = std::exp (-1.0f / (0.045f * (float) sr)), kAtt = 1.0f - std::exp (-1.0f / (0.0015f * (float) sr));
        float kP = std::exp (-1.0f / (0.028f * (float) sr));
        // i parametri arrivano a scatti dal processor: qui si muovono morbidi, ricalcolati ogni 16 campioni contati dall'inizio (uguale con ogni buffer)
        auto update = [&]
        {
            const float sk = 1.0f - std::exp (-16.0f / (0.02f * (float) sr));
            toneS += sk * (tone0 - toneS); driveS += sk * (drive0 - driveS); decayS += sk * (decay0 - decayS); punchS += sk * (punch0 - punchS);
            tone = toneS; drive = driveS; decay = decayS; punch = punchS;
            const float dec = mode == B808 ? 0.25f + decay * 4.0f : 0.05f + decay * 1.2f;
            kDec = std::exp (-1.0f / (dec * (float) sr)); kF = std::exp (-1.0f / ((0.04f + decay * 0.6f) * (float) sr));
        };
        for (int i = 0; i < n; ++i)
        {
            if ((++ctr & 15u) == 0) update();
            float y = 0;
            curHz += glideK * (tgtHz - curHz);
            if (mode == BASSO)
            {
                for (auto& k : ks) if (k.on) y += k.tick();
                // basso: scuro (dita), un po' di saturazione da DI
                const float c = 1.0f - std::exp (-6.2831853f * (500.0f + 3500.0f * tone) / (float) sr);
                lp1 += c * (y - lp1); lp2 += c * (lp1 - lp2); y = lp2 * 1.6f;
                y = std::tanh (y * (1.0f + drive * 3.0f)) / (1.0f + drive * 0.8f);
            }
            else if (on || env > 1e-5f)
            {
                // inviluppo d'ampiezza: attacco corto, poi decade (808) o resta (synth) finche' la nota e' tenuta
                if (rel) env *= kRel;
                else { if (envUp) { env += kAtt * (1.02f - env); if (env >= 1.0f) { env = 1.0f; envUp = false; } } else if (mode == B808) env *= kDec; }
                if (! rel && ! held && mode != B808) rel = true;
                const float hz = curHz * std::pow (2.0f, penv / 12.0f); penv *= kP;
                if (mode == B808)
                {
                    ph += hz / (float) sr; if (ph >= 1.0f) ph -= 1.0f;
                    float x = std::sin (6.2831853f * ph);
                    const float k = 1.0f + drive * 7.0f; x = std::tanh (x * k) / std::tanh (k);
                    if (clickN > 0) { x += (rnd.nextFloat() * 2 - 1) * 0.25f * punch * (float) clickN / (float) (0.003 * sr); --clickN; }
                    // TONO: toglie le armoniche del drive
                    const float cT = 1.0f - std::exp (-6.2831853f * (300.0f + 6000.0f * tone * tone) / (float) sr);
                    c808 += 0.002f * (cT - c808);   // il taglio si muove campione per campione
                    lp1 += c808 * (x - lp1); y = (lp1 + (x - lp1) * 0.15f) * env * (mutedHit ? 0.4f : 1.0f) * 0.72f;
                }
                else
                {
                    // dente di sega (polyBLEP) + sub un'ottava sotto
                    const float dt = hz / (float) sr;
                    ph += dt; if (ph >= 1.0f) ph -= 1.0f;
                    float saw = 2.0f * ph - 1.0f - blep (ph, dt);
                    sub += dt * 0.5f; if (sub >= 1.0f) sub -= 1.0f;
                    float sq = sub < 0.5f ? 1.0f : -1.0f; sq += blep (sub, dt * 0.5f); sq -= blep (std::fmod (sub + 0.5f, 1.0f), dt * 0.5f);
                    const float x = saw * 0.6f + sq * 0.45f;
                    // filtro risonante (TPT): taglio da TONO, inviluppo e accento (PUNCH = risonanza e colpo dell'inviluppo)
                    fenv *= kF;
                    const float cut = juce::jlimit (30.0f, (float) sr * 0.42f, 70.0f * std::pow (2.0f, tone * 6.5f) * std::pow (2.0f, fenv * (2.0f + punch * 3.0f + accent * 1.5f)));
                    const float g = std::tan (3.14159265f * cut / (float) sr), Q = 0.7f + 7.0f * (punch * 0.7f + accent * 0.15f), k = 1.0f / Q;
                    const float a1 = 1.0f / (1.0f + g * (g + k)), a2 = g * a1, a3 = g * a2;
                    const float v3 = x - ic2, v1 = a1 * ic1 + a2 * v3, v2 = ic2 + a2 * ic1 + a3 * v3;
                    ic1 = 2 * v1 - ic1; ic2 = 2 * v2 - ic2;
                    float f = v2; f = std::tanh (f * (1.0f + drive * 3.0f)) / (1.0f + drive);
                    y = f * env * 0.9f;
                }
                if (env < 1e-5f && rel) { on = false; env = 0; }
            }
            // blocco della continua
            const float o = y - dcx + 0.9995f * dcy; dcx = y; dcy = o;
            out[i] += o;
        }
    }
    void retrig() { envUp = true; }
    bool on = false;
private:
    static float blep (float t, float dt) { if (t < dt) { t /= dt; return t + t - t * t - 1.0f; } if (t > 1.0f - dt) { t = (t - 1.0f) / dt; return t * t + t + t + 1.0f; } return 0.0f; }
    float toneS = 0.5f, driveS = 0.3f, decayS = 0.6f, punchS = 0.5f, kDec = 0.9999f, kF = 0.999f, c808 = 0.1f; juce::uint32 ctr = 0;
    double sr = 48000; float ph = 0, sub = 0, env = 0, penv = 0, curHz = 55, tgtHz = 55, glideK = 1, velS = 0.7f, ic1 = 0, ic2 = 0, fenv = 0, accent = 0, dcx = 0, dcy = 0, lp1 = 0, lp2 = 0;
    bool held = false, rel = false, envUp = false, mutedHit = false; int clickN = 0, note = -1, kc = 0;
    gtr::KS ks[2]; juce::Random rnd { 7 };
    friend class BassVoice;
};

// voce completa: monofonica, con volume, tetto morbido e mono -> stereo
class BassVoice
{
public:
    void prepare (double s) { sr = s; m.prepare (s); buf.assign (8192, 0.0f); }
    void reset() { m.reset(); }
    float vol = 0.8f;
    Mono m;
    void hit (int note, float vel, bool legato, bool mute) { m.noteOn (note, vel, legato, mute); if (! (legato && m.on && m.env > 0.05f)) m.envUp = true; }
    void render (float* L, float* R, int n)
    {
        while (n > 0)
        {
            const int k = juce::jmin (n, (int) buf.size());
            std::fill (buf.begin(), buf.begin() + k, 0.0f);
            m.render (buf.data(), k);
            for (int i = 0; i < k; ++i)
            {
                volS += 0.001f * (vol - volS);
                float x = buf[(size_t) i] * volS * 0.8f;
                if (! std::isfinite (x)) { x = 0; m.reset(); }
                const float t = 0.89f, ax = std::abs (x); if (ax > t) x = std::copysign (t + (1.0f - t) * std::tanh ((ax - t) / (1.0f - t)), x);
                L[i] += x; R[i] += x;
            }
            L += k; R += k; n -= k;
        }
    }
private:
    double sr = 48000; std::vector<float> buf; float volS = 0.8f;
};
}
