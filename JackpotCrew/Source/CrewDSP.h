#pragma once
// JACKPOT CYBERNET CREW: da una voce sola una crew che grida il ritornello (gang vocals).
// Ogni voce e' un personaggio: un raddoppio con il suo ritardo che respira (mai perfettamente a tempo),
// la sua intonazione (ottava, quinta o unisono stonato di qualche cent), il suo colore e il suo posto nello stereo.
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <cmath>

namespace jcrew
{
constexpr int kVoices = 6;
struct VoiceSet { bool on = false; float lvlDb = 0, pan = 0, pitch = 0, tone = 0; };
struct Settings
{
    std::array<VoiceSet, kVoices> v;
    float loose = 0.4f, drift = 0.4f, spread = 0.8f, shout = 0, room = 0, lowCut = 120, crewDb = 0, dryDb = 0;
};
// i personaggi: nome, ritardo di base (ms), cent di stonatura tipici e i valori di partenza della voce
struct Character { const char* name; const char* role; float baseMs, cents, pitch, pan, tone, lvl; };
inline const std::array<Character, kVoices>& characters()
{
    static const std::array<Character, kVoices> c = { {
        { "BIG MO", "il vocione: ottava sotto", 24, -6, -12, -0.35f, -0.35f, -3 },
        { "SPIKE", "il punk che urla: unisono sporco", 14, 11, 0, 0.6f, 0.45f, 0 },
        { "TRIDI", "la voce alta: ottava sopra", 31, 7, 12, 0.25f, 0.2f, -8 },
        { "ASTRO", "dal casco: unisono lontano", 41, -14, 0, -0.75f, 0.7f, -4 },
        { "BANDANA", "l'unisono di dietro", 27, 5, 0, -0.15f, -0.1f, -2 },
        { "GRILLZ", "il basso con l'oro: quinta sotto", 19, -9, -7, 0.85f, -0.25f, -4 },
    } };
    return c;
}

struct OnePole { float a = 0, z = 0; void lp (double sr, float hz) { a = (float) std::exp (-juce::MathConstants<double>::twoPi * hz / sr); } float p (float x) { z = x + a * (z - x); return z; } };
struct Biq
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    void hp (double sr, double f) { const double w = juce::MathConstants<double>::twoPi * juce::jlimit (10.0, sr * 0.45, f) / sr, c = std::cos (w), s = std::sin (w), al = s / (2 * 0.707), a0 = 1 + al; b0 = (1 + c) / 2 / a0; b1 = -(1 + c) / a0; b2 = b0; a1 = -2 * c / a0; a2 = (1 - al) / a0; }
    float p (float x) { const double y = b0 * x + z1; z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y; return (float) y; }
    void reset() { z1 = z2 = 0; }
};

class Engine
{
public:
    void prepare (double s, int maxBlock)
    {
        sr = s; buf.assign ((size_t) kLen, 0.0f); wp = 0;
        for (int i = 0; i < kVoices; ++i)
        {
            auto& v = vs[(size_t) i]; v = {}; juce::Random r (i * 977 + 13);
            v.p = r.nextFloat(); v.wobble = 0; v.target = 0; v.next = 0; v.seed = r.nextInt();
            v.lvl.reset (s, 0.03); v.panL.reset (s, 0.03); v.panR.reset (s, 0.03);
            v.lvl.setCurrentAndTargetValue (0); v.panL.setCurrentAndTargetValue (0.707f); v.panR.setCurrentAndTargetValue (0.707f);
        }
        hpL.reset(); hpR.reset(); env = 0;
        juce::Reverb::Parameters p; p.roomSize = 0.32f; p.damping = 0.5f; p.width = 1; p.wetLevel = 1; p.dryLevel = 0; rv.setParameters (p); rv.setSampleRate (s); rv.reset();
        wetL.assign ((size_t) maxBlock, 0.0f); wetR.assign ((size_t) maxBlock, 0.0f); tmpL.assign ((size_t) maxBlock, 0.0f); tmpR.assign ((size_t) maxBlock, 0.0f); shoutS.reset (s, 0.05); roomS.reset (s, 0.05); shoutS.setCurrentAndTargetValue (0); roomS.setCurrentAndTargetValue (0); crewGain.reset (s, 0.03); dryGain.reset (s, 0.03);
        crewGain.setCurrentAndTargetValue (1); dryGain.setCurrentAndTargetValue (1);
    }
    void process (float* L, float* R, int n, const Settings& S)
    {
        if ((int) wetL.size() < n) { wetL.resize ((size_t) n); wetR.resize ((size_t) n); tmpL.resize ((size_t) n); tmpR.resize ((size_t) n); }
        std::fill (wetL.begin(), wetL.begin() + n, 0.0f); std::fill (wetR.begin(), wetR.begin() + n, 0.0f);
        bool any = false;
        for (int k = 0; k < kVoices; ++k)
        {
            auto& v = vs[(size_t) k]; const auto& vsS = S.v[(size_t) k]; const auto& ch = characters()[(size_t) k];
            const float pan = juce::jlimit (-1.0f, 1.0f, vsS.pan * S.spread), a = (pan + 1) * juce::MathConstants<float>::pi * 0.25f;
            v.lvl.setTargetValue (vsS.on ? juce::Decibels::decibelsToGain (vsS.lvlDb) : 0.0f); v.panL.setTargetValue (std::cos (a)); v.panR.setTargetValue (std::sin (a));
            const float cents = ch.cents * S.drift * 1.6f;
            v.ratio = std::pow (2.0f, (vsS.pitch + cents / 100.0f) / 12.0f);
            v.win = (float) (sr * 0.075);
            // colore: scuro (passa basso) o chiaro (passa alto + un filo di presenza)
            v.lp.lp (sr, vsS.tone < 0 ? 18000.0f * std::pow (0.07f, -vsS.tone) : 18000.0f);
            v.hpA = vsS.tone > 0 ? (float) std::exp (-juce::MathConstants<double>::twoPi * (40.0 + 900.0 * vsS.tone) / sr) : 0.0f;
            v.baseT = (float) (sr * ch.baseMs * 0.001 * (0.6 + 0.8 * S.loose));
            v.active = vsS.on || v.lvl.isSmoothing() || v.lvl.getCurrentValue() > 1e-4f;
            any = any || v.active;
        }
        for (auto& v : vs) if (v.baseD < 0) v.baseD = v.baseT;   // al primo blocco niente scivolata
        const float baseK = (float) (1.0 / (sr * 0.05));
        for (int i = 0; i < n; ++i)
        {
            const float x = 0.5f * (L[i] + R[i]);
            buf[(size_t) wp] = x;
            for (int k = 0; k < kVoices; ++k)
            {
                auto& v = vs[(size_t) k];
                if (! v.active) { v.lvl.skip (1); v.panL.skip (1); v.panR.skip (1); continue; }
                // tempo che respira: un nuovo obiettivo ogni ~120 ms, raggiunto con morbidezza
                if (--v.next <= 0) { juce::Random r (v.seed += 7919); v.target = (r.nextFloat() * 2 - 1) * S.loose * (float) sr * 0.018f; v.next = (int) (sr * (0.09 + 0.08 * r.nextFloat())); }
                v.wobble += (v.target - v.wobble) * (float) (1.0 / (sr * 0.09));
                v.baseD += (v.baseT - v.baseD) * baseK;
                const float d0 = juce::jmax (8.0f, v.baseD + v.wobble);
                // due testine che scorrono e si danno il cambio (pitch che cambia altezza, non durata)
                v.p += (1.0f - v.ratio) / v.win; if (v.p >= 1) v.p -= 1; if (v.p < 0) v.p += 1;
                const float q = v.p + 0.5f >= 1 ? v.p - 0.5f : v.p + 0.5f;
                auto fadeOf = [] (float t) { return juce::jlimit (0.0f, 1.0f, juce::jmin (t, 1.0f - t) / 0.12f); };
                float ga = fadeOf (v.p), gb = fadeOf (q); const float nrm = 1.0f / std::sqrt (ga * ga + gb * gb + 1e-9f); ga *= nrm; gb *= nrm;
                float y = ga * read (d0 + v.p * v.win) + gb * read (d0 + q * v.win);
                y = v.lp.p (y);
                if (v.hpA > 0) { const float h = y - v.hz; v.hz = y + v.hpA * (v.hz - y); y = h * 1.15f + y * 0.15f; }
                const float g = v.lvl.getNextValue(); const float yl = y * g * v.panL.getNextValue(), yr = y * g * v.panR.getNextValue();
                wetL[(size_t) i] += yl; wetR[(size_t) i] += yr;
                v.meter = juce::jmax (std::abs (y * g), v.meter * 0.9995f);
            }
            wp = (wp + 1) & (kLen - 1);
        }
        // bus della crew: taglio dei bassi, "gridato" (compressione + saturazione), stanza
        hpL.hp (sr, S.lowCut); hpR.hp (sr, S.lowCut);
        crewGain.setTargetValue (juce::Decibels::decibelsToGain (S.crewDb)); dryGain.setTargetValue (S.dryDb <= -59.9f ? 0.0f : juce::Decibels::decibelsToGain (S.dryDb));
        if (any)
        {
            shoutS.setTargetValue (S.shout); roomS.setTargetValue (S.room);
            for (int i = 0; i < n; ++i)
            {
                float l = hpL.p (wetL[(size_t) i]), r = hpR.p (wetR[(size_t) i]);
                const float sh = shoutS.getNextValue();
                if (sh > 0.0005f)
                {
                    const float drive = 1.0f + sh * 7.0f, makeup = 1.0f / std::tanh (drive * 0.5f) * 0.55f;
                    const float lv = juce::jmax (std::abs (l), std::abs (r)); env += (lv - env) * (lv > env ? 0.02f : 0.0004f);
                    const float comp = 1.0f / (1.0f + sh * 3.0f * env);
                    const float sl = std::tanh (l * comp * drive * 0.5f) * makeup, sr2 = std::tanh (r * comp * drive * 0.5f) * makeup;
                    l = l * (1 - sh) + sl * sh * 1.6f; r = r * (1 - sh) + sr2 * sh * 1.6f;
                }
                wetL[(size_t) i] = l; wetR[(size_t) i] = r;
            }
            if (S.room > 0.0005f || roomS.isSmoothing() || roomS.getCurrentValue() > 0.0005f)
            {
                std::copy (wetL.begin(), wetL.begin() + n, tmpL.begin()); std::copy (wetR.begin(), wetR.begin() + n, tmpR.begin());
                rv.processStereo (tmpL.data(), tmpR.data(), n);
                for (int i = 0; i < n; ++i) { const float rm = roomS.getNextValue() * 0.7f; wetL[(size_t) i] += tmpL[(size_t) i] * rm; wetR[(size_t) i] += tmpR[(size_t) i] * rm; }
            }
            else roomS.skip (n);
        }
        for (int i = 0; i < n; ++i)
        {
            const float dg = dryGain.getNextValue(), cg = crewGain.getNextValue();
            L[i] = L[i] * dg + wetL[(size_t) i] * cg; R[i] = R[i] * dg + wetR[(size_t) i] * cg;
        }
    }
    float meter (int k) const { return vs[(size_t) k].meter; }
private:
    static constexpr int kLen = 1 << 16;
    float read (float d) const
    {
        float pos = (float) wp - d; while (pos < 0) pos += kLen;
        const int i0 = (int) pos; const float t = pos - i0;
        const float a = buf[(size_t) (i0 & (kLen - 1))], b = buf[(size_t) ((i0 + 1) & (kLen - 1))];
        return a + (b - a) * t;
    }
    struct V { float p = 0, ratio = 1, win = 2000, baseD = -1, baseT = 1000, wobble = 0, target = 0, hpA = 0, hz = 0, meter = 0; int next = 0, seed = 1; bool active = false; OnePole lp;
               juce::LinearSmoothedValue<float> lvl, panL, panR; };
    double sr = 48000; std::vector<float> buf; int wp = 0;
    std::array<V, kVoices> vs;
    std::vector<float> wetL, wetR, tmpL, tmpR;
    Biq hpL, hpR; float env = 0; juce::Reverb rv;
    juce::LinearSmoothedValue<float> crewGain, dryGain, shoutS, roomS;
};
}
