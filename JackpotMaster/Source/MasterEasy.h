#pragma once
#include "MasterDyn.h"
#include "MasterColor.h"

// Motore della modalita FACILE: poche manopole, algoritmi che lavorano da soli.
// Ordine: EQ a bande fisse -> CALM (risonanze) -> THICK / WARM / SILK -> GLUE -> WIDE. Poi il processore fa LOUD (maximizer) e il true peak.
namespace jms
{
struct EasySettings { float low = 0, mid = 0, high = 0, calm = 0, thick = 0, warm = 0, silk = 0, glue = 0, wide = 0; bool monoBass = true; };

class EasyEngine
{
public:
    void prepare (double s)
    {
        sr = s;
        for (auto& c : eq) for (auto& f : c) f.reset();
        for (auto& b : calm) { b.env = b.slow = 0; b.g = 0; for (auto& f : b.det) f.reset(); for (auto& f : b.cut) f.reset(); b.det[0].set (Biquad::BPASS, b.f, 0, 1.4, sr); b.det[1].set (Biquad::BPASS, b.f, 0, 1.4, sr); }
        for (int c = 0; c < 2; ++c) { tLp[c].set (Biquad::HCUT, 1400, 0, 0.707, sr); tLp2[c].set (Biquad::HCUT, 1400, 0, 0.707, sr); tLp[c].reset(); tLp2[c].reset(); sHp[c].set (Biquad::LCUT, 4500, 0, 0.707, sr); sHp[c].reset(); }
        wLp.set (Biquad::HCUT, 120, 0, 0.707, sr); wLp2.set (Biquad::HCUT, 120, 0, 0.707, sr); wLp.reset(); wLp2.reset();
        tape.prepare (s); bus.prepare (s);
        last = {}; last.low = 99;
    }
    void process (float* L, float* R, int n, const EasySettings& S)
    {
        // ---- EQ: shelf morbido sui bassi (Baxandall), campana larga sui medi, shelf d'aria sugli alti
        if (S.low != last.low || S.mid != last.mid || S.high != last.high)
            for (int c = 0; c < 2; ++c) { eq[c][0].set (Biquad::LSHELF, 90, S.low, 0.55, sr); eq[c][1].set (Biquad::BELL, 1200, S.mid, 0.45, sr); eq[c][2].set (Biquad::HSHELF, 9000, S.high, 0.55, sr); }
        if (std::abs (S.low) > 0.01f || std::abs (S.mid) > 0.01f || std::abs (S.high) > 0.01f)
            for (int i = 0; i < n; ++i) { L[i] = eq[0][2] (eq[0][1] (eq[0][0] (L[i]))); R[i] = eq[1][2] (eq[1][1] (eq[1][0] (R[i]))); }
        // ---- CALM: tre zone critiche (fango, durezza, sibilanti). Ogni zona e confrontata con la sua media lunga:
        // quando "sporge" (risonanza che si accende, nota che rimbomba, s) viene abbassata, fino a 6 dB con CALM al massimo.
        if (S.calm > 0.001f)
        {
            const float at = (float) std::exp (-1.0 / (0.004 * sr)), rl = (float) std::exp (-1.0 / (0.09 * sr)), sl = (float) std::exp (-1.0 / (2.5 * sr));
            for (int i0 = 0; i0 < n; i0 += 32)
            {
                const int m = juce::jmin (32, n - i0);
                for (auto& b : calm)
                {
                    for (int i = i0; i < i0 + m; ++i)
                    {
                        const float x = 0.5f * (std::abs (b.det[0] (L[i])) + std::abs (b.det[1] (R[i])));
                        b.env = x > b.env ? at * b.env + (1 - at) * x : rl * b.env + (1 - rl) * x;
                        b.slow = sl * b.slow + (1 - sl) * b.env;
                    }
                    const float over = 20.0f * std::log10 ((b.env + 1e-9f) / (b.slow + 1e-9f));   // dB sopra la sua media
                    const float tgt = -juce::jlimit (0.0f, b.maxCut, (over - b.knee) * 0.7f) * S.calm;
                    b.g += (tgt - b.g) * 0.25f;
                    b.cut[0].set (Biquad::BELL, b.f, b.g, b.q, sr); b.cut[1].set (Biquad::BELL, b.f, b.g, b.q, sr);
                    for (int i = i0; i < i0 + m; ++i) { L[i] = b.cut[0] (L[i]); R[i] = b.cut[1] (R[i]); }
                }
            }
            calmNow = juce::jmin (calm[0].g, juce::jmin (calm[1].g, calm[2].g));
        }
        else calmNow = 0;
        // ---- THICK: saturazione a valvola (armoniche pari) solo sotto 1.4 kHz, aggiunta in parallelo: spessore e corpo
        if (S.thick > 0.001f)
        {   // la saturazione comprime: si riporta al livello di prima (inviluppi), cosi resta solo la "pasta" delle armoniche
            const float d = 1.5f + 4.0f * S.thick, amt = 0.5f + 0.7f * S.thick;
            const float dcK = (float) std::exp (-1.0 / (0.02 * sr)), eK = (float) std::exp (-1.0 / (0.05 * sr));
            for (int c = 0; c < 2; ++c)
            {
                float* x = c ? R : L;
                for (int i = 0; i < n; ++i)
                {
                    const float lo = tLp2[c] (tLp[c] (x[i]));
                    float sat = std::tanh (d * (lo + 0.2f * lo * lo)) / d;
                    dcAvg[c] = dcK * dcAvg[c] + (1 - dcK) * sat; sat -= dcAvg[c];
                    eLo[c] = eK * eLo[c] + (1 - eK) * std::abs (lo); eSat[c] = eK * eSat[c] + (1 - eK) * std::abs (sat);
                    const float gc = eSat[c] > 1e-6f ? juce::jlimit (0.5f, 4.0f, eLo[c] / eSat[c]) : 1.0f;
                    x[i] += (sat * gc - lo) * amt;
                }
            }
        }
        // ---- WARM: nastro da 15 pollici (head bump, compressione morbida, alti arrotondati)
        if (S.warm > 0.001f) { TapeSettings t; t.model = 1; t.drive = 0.65f * S.warm; t.bias = 0.1f; t.noise = 0; t.mix = juce::jmin (1.0f, 0.4f + S.warm); tape.process (L, R, n, t); }
        // ---- SILK: armoniche solo sopra 4.5 kHz, piu un filo di shelf: lucentezza senza durezza
        if (S.silk > 0.001f)
        {
            const float k = S.silk;
            for (int c = 0; c < 2; ++c)
            {
                float* x = c ? R : L;
                for (int i = 0; i < n; ++i) { const float h = sHp[c] (x[i]); const float s = std::tanh (3.0f * h) / 3.0f; x[i] += k * (0.35f * h + 1.6f * (s - h * 0.92f)); }
            }
        }
        // ---- GLUE: compressore di bus 2:1, attacco 10 ms, rilascio auto, filtro sui bassi nel sidechain
        if (S.glue > 0.001f)
        {
            BusSettings b; b.thr = -4.0f - 16.0f * S.glue; b.ratio = S.glue > 0.6f ? 2 : 1; b.att = 4; b.rel = 4; b.autoMakeup = true; b.makeup = 0; b.mix = 1; b.scHpf = 90; b.range = 6;
            bus.process (L, R, n, b); glueGr = bus.gr;
        }
        else glueGr = 0;
        // ---- WIDE: allarga (o stringe) il side sopra 120 Hz; sotto 120 Hz i bassi restano mono se MONO BASSI e acceso
        if (std::abs (S.wide) > 0.001f || S.monoBass)
        {
            const float w = S.wide >= 0 ? 1.0f + 0.8f * S.wide : 1.0f + S.wide;
            for (int i = 0; i < n; ++i)
            {
                const float m = 0.5f * (L[i] + R[i]), s = 0.5f * (L[i] - R[i]);
                const float sLo = wLp2 (wLp (s)), sHi = s - sLo;
                const float s2 = (S.monoBass ? 0.0f : sLo) + sHi * w;
                L[i] = m + s2; R[i] = m - s2;
            }
        }
        last = S;
    }
    float calmNow = 0, glueGr = 0;
private:
    struct CalmBand { float f, q, knee, maxCut; Biquad det[2], cut[2]; float env = 0, slow = 0, g = 0; };
    double sr = 48000;
    Biquad eq[2][3], tLp[2], tLp2[2], sHp[2], wLp, wLp2;
    CalmBand calm[3] { { 280, 1.2f, 3.0f, 4.0f }, { 3200, 1.6f, 2.5f, 6.0f }, { 7000, 2.0f, 2.0f, 6.0f } };
    TapeMachine tape; BusComp bus; EasySettings last; float dcAvg[2] {}, eLo[2] {}, eSat[2] {};
};
}
