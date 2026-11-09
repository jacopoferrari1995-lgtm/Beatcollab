#include "ButterDSP.h"
#include <complex>

namespace jb
{
using namespace juce;
static const StringArray kNoise { "WHITE", "PINK", "VINILE", "NASTRO", "METALLO", "SABBIA" };
static const StringArray kFocus { "FULL", "LOW", "MID", "HIGH" };
static const StringArray kComp { "PUNCH", "WARM", "GLUE", "GENTLE", "SMACK", "OTT" };
static const StringArray kSat { "CONSOLE", "4 TRACK", "TAPE", "TUBE", "DIGITAL", "FLUB" };
static const StringArray kSpace { "ROOM", "HALL", "PLATE", "SPRING" };
static const StringArray kStages { "1", "2", "3", "4" };

void Burro::addParams (AudioProcessorValueTreeState::ParameterLayout& l)
{
    auto F = [&l] (const char* id, const char* n, float a, float b, float d) { l.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, n, NormalisableRange<float> (a, b), d)); };
    auto B = [&l] (const char* id, const char* n, bool d) { l.add (std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, n, d)); };
    auto C = [&l] (const char* id, const char* n, const StringArray& c, int d) { l.add (std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, n, c, d)); };
    F ("u_mag", "Magnitude", 0, 1, 1);
    B ("u_bOn", "Boom On", true); F ("u_boom", "Boom", 0, 1, 0); F ("u_bTone", "Boom Tone", 30, 120, 55); B ("u_acid", "Boom Acid", false);
    F ("u_noise", "Noise", 0, 1, 0); C ("u_nType", "Noise Type", kNoise, 1);
    B ("u_shOn", "Shift On", true); F ("u_shAmt", "Shift Mix", 0, 1, 0); F ("u_shPitch", "Shift Pitch", -12, 12, 0); F ("u_shTgt", "Shift Target", -1, 1, 0); C ("u_shFoc", "Shift Focus", kFocus, 0);
    B ("u_cOn", "Comp On", true); C ("u_cType", "Comp Type", kComp, 0); F ("u_cAmt", "Comp Amount", -1, 1, 0); F ("u_cBands", "Comp Bands", 0, 1, 0);
    B ("u_sOn", "Sat On", true); C ("u_sType", "Sat Type", kSat, 0); F ("u_sAmt", "Sat Amount", 0, 1, 0); F ("u_sStyle", "Sat Style", 0, 1, 0.5f); F ("u_sTgt", "Sat Target", -1, 1, 0); C ("u_sFoc", "Sat Focus", kFocus, 0);
    B ("u_spOn", "Space On", true); C ("u_spType", "Space Type", kSpace, 0); F ("u_spAmt", "Space Amount", 0, 1, 0); F ("u_spDecay", "Space Decay", 0, 1, 0.5f);
    F ("u_spPre", "Space Predelay", 0, 120, 10); F ("u_spDuck", "Space Duck", 0, 1, 0); F ("u_spWidth", "Space Width", 0, 1, 0.8f);
    B ("u_mOn", "Clip On", true); F ("u_mAmt", "Clip Drive", 0, 1, 0); C ("u_mStages", "Clip Stages", kStages, 1); F ("u_mTone", "Clip Tone", -1, 1, 0); F ("u_mTgt", "Clip Target", -1, 1, 0); C ("u_mFoc", "Clip Focus", kFocus, 0);
    B ("u_eqOn", "EQ On", true);
    F ("u_e1g", "EQ Low Gain", -15, 15, 0); F ("u_e1f", "EQ Low Freq", 30, 400, 90);
    F ("u_e2g", "EQ LoMid Gain", -15, 15, 0); F ("u_e2f", "EQ LoMid Freq", 120, 2000, 400); F ("u_e2q", "EQ LoMid Q", 0.3f, 6, 1);
    F ("u_e3g", "EQ HiMid Gain", -15, 15, 0); F ("u_e3f", "EQ HiMid Freq", 800, 10000, 3000); F ("u_e3q", "EQ HiMid Q", 0.3f, 6, 1);
    F ("u_e4g", "EQ High Gain", -15, 15, 0); F ("u_e4f", "EQ High Freq", 3000, 18000, 9000);
    F ("u_tone", "Master Tone", -1, 1, 0); F ("u_air", "Master Air", 0, 1, 0); F ("u_att", "Master Attack", -1, 1, 0); F ("u_sus", "Master Sustain", -1, 1, 0); B ("u_clip", "Master Clip", false);
}

const std::vector<BurroPreset>& burroPresets()
{
    // u_cType: 0 PUNCH, 1 WARM, 2 GLUE, 3 GENTLE, 4 SMACK, 5 OTT
    static const std::vector<BurroPreset> P = {
        { "Init", {} },
        { "Bus | Trap Knock", { { "u_boom", 0.35f }, { "u_bTone", 50 }, { "u_cType", 0 }, { "u_cAmt", 0.45f }, { "u_sType", 0 }, { "u_sAmt", 0.3f }, { "u_sTgt", 0.5f }, { "u_mAmt", 0.25f }, { "u_mStages", 1 }, { "u_att", 0.3f }, { "u_air", 0.2f }, { "u_clip", 1 } } },
        { "Bus | Boom Bap Dusty", { { "u_noise", 0.15f }, { "u_nType", 2 }, { "u_cType", 1 }, { "u_cAmt", 0.4f }, { "u_sType", 1 }, { "u_sAmt", 0.45f }, { "u_sStyle", 0.6f }, { "u_spType", 0 }, { "u_spAmt", 0.18f }, { "u_spDecay", 0.3f }, { "u_tone", -0.3f } } },
        { "Bus | Drill Crunch", { { "u_cType", 4 }, { "u_cAmt", 0.5f }, { "u_sType", 4 }, { "u_sAmt", 0.35f }, { "u_sStyle", 0.4f }, { "u_sTgt", 0.6f }, { "u_mAmt", 0.4f }, { "u_mStages", 2 }, { "u_att", 0.4f } } },
        { "Bus | Lo-fi Room", { { "u_noise", 0.2f }, { "u_nType", 3 }, { "u_sType", 2 }, { "u_sAmt", 0.4f }, { "u_spType", 0 }, { "u_spAmt", 0.35f }, { "u_spDecay", 0.5f }, { "u_spDuck", 0.4f }, { "u_tone", -0.5f } } },
        { "Bus | Glue Leggera", { { "u_cType", 2 }, { "u_cAmt", 0.35f }, { "u_sType", 2 }, { "u_sAmt", 0.15f }, { "u_air", 0.15f } } },
        { "Bus | OTT Sparkle", { { "u_cType", 5 }, { "u_cAmt", 0.6f }, { "u_cBands", 1 }, { "u_air", 0.3f } } },
        { "Bus | Parallel Smack", { { "u_cType", 4 }, { "u_cAmt", 0.85f }, { "u_mAmt", 0.2f }, { "u_mag", 0.45f } } },
        { "Bus | Phonk Sporco", { { "u_cType", 4 }, { "u_cAmt", 0.5f }, { "u_sType", 5 }, { "u_sAmt", 0.35f }, { "u_mAmt", 0.45f }, { "u_mStages", 2 }, { "u_noise", 0.15f }, { "u_nType", 3 }, { "u_clip", 1 } } },
        { "Kick | 808 Boom", { { "u_boom", 0.6f }, { "u_bTone", 45 }, { "u_cType", 2 }, { "u_cAmt", 0.3f }, { "u_sType", 3 }, { "u_sAmt", 0.3f }, { "u_sFoc", 1 } } },
        { "Kick | Acid Kick", { { "u_boom", 0.5f }, { "u_bTone", 60 }, { "u_acid", 1 }, { "u_cType", 0 }, { "u_cAmt", 0.35f }, { "u_clip", 1 } } },
        { "Kick | Punch Corto", { { "u_cType", 0 }, { "u_cAmt", 0.5f }, { "u_att", 0.5f }, { "u_sus", -0.4f }, { "u_mAmt", 0.2f } } },
        { "Snare | Spring", { { "u_spType", 3 }, { "u_spAmt", 0.4f }, { "u_spDecay", 0.5f }, { "u_spPre", 20 }, { "u_spDuck", 0.3f }, { "u_cType", 0 }, { "u_cAmt", 0.3f } } },
        { "Snare | Crack", { { "u_cType", 4 }, { "u_cAmt", 0.45f }, { "u_sType", 0 }, { "u_sAmt", 0.35f }, { "u_sTgt", 0.7f }, { "u_air", 0.35f } } },
        { "Snare | Plate Lunga", { { "u_spType", 2 }, { "u_spAmt", 0.5f }, { "u_spDecay", 0.8f }, { "u_spPre", 30 }, { "u_spWidth", 1 } } },
        { "Hat | Air", { { "u_noise", 0.25f }, { "u_nType", 5 }, { "u_sType", 2 }, { "u_sAmt", 0.2f }, { "u_sFoc", 3 }, { "u_air", 0.6f }, { "u_sus", -0.3f } } },
        { "Hat | Corti e Secchi", { { "u_att", 0.4f }, { "u_sus", -0.6f }, { "u_tone", 0.2f } } },
        { "Loop | Pitch Giu Sporco", { { "u_shAmt", 0.6f }, { "u_shPitch", -5 }, { "u_shTgt", -0.4f }, { "u_sType", 5 }, { "u_sAmt", 0.3f } } },
        { "Loop | Vinile Caldo", { { "u_noise", 0.3f }, { "u_nType", 2 }, { "u_sType", 1 }, { "u_sAmt", 0.4f }, { "u_tone", -0.45f }, { "u_cType", 1 }, { "u_cAmt", 0.3f } } },
        { "Loop | Pompa", { { "u_cType", 2 }, { "u_cAmt", 0.75f }, { "u_sus", 0.3f } } },
        { "FX | Ottava Sopra", { { "u_shAmt", 0.5f }, { "u_shPitch", 12 }, { "u_shTgt", 0.6f }, { "u_spType", 1 }, { "u_spAmt", 0.25f } } },
        { "FX | Bitcrush Totale", { { "u_sType", 4 }, { "u_sAmt", 0.8f }, { "u_sStyle", 0.9f }, { "u_mAmt", 0.5f }, { "u_mStages", 3 } } } };
    return P;
}

void Burro::applyPreset (AudioProcessorValueTreeState& s, int index)
{
    const auto& all = burroPresets(); if (index < 0 || index >= (int) all.size()) return;
    for (auto* p : s.processor.getParameters())
        if (auto* rp = dynamic_cast<RangedAudioParameter*> (p)) if (rp->getParameterID().startsWith ("u_")) rp->setValueNotifyingHost (rp->getDefaultValue());
    for (auto& [id, v] : all[(size_t) index].p) if (auto* rp = s.getParameter (id)) rp->setValueNotifyingHost (rp->convertTo0to1 (v));
}

void Burro::prepare (double sampleRate, int maxBlock)
{
    sr = sampleRate;
    const int n = jmax (maxBlock, 4096);
    dryL.assign ((size_t) n, 0); dryR.assign ((size_t) n, 0); trW.assign ((size_t) n, 0); wetL.assign ((size_t) n, 0); wetR.assign ((size_t) n, 0);
    shBuf.setSize (2, 16384); shBuf.clear(); shW = 0;
    preBuf.setSize (2, (int) (sr * 0.25) + 8); preBuf.clear(); preW = 0;
    verb.setSampleRate (sr); verb.reset(); lastSpType = -1;
}

static inline float onePole (float x, float& z, float a) { z = (1 - a) * x + a * z; return z; }
static inline float coef (double hz, double sr) { return (float) std::exp (-MathConstants<double>::twoPi * hz / sr); }
static inline float tgtW (float t, float tr) { return t >= 0 ? (1 - t) + t * tr : (1 + t) + (-t) * (1 - tr); }

void Burro::process (float* L, float* R, int N, AudioProcessorValueTreeState& s)
{
    if ((int) dryL.size() < N) { dryL.resize ((size_t) N); dryR.resize ((size_t) N); trW.resize ((size_t) N); wetL.resize ((size_t) N); wetR.resize ((size_t) N); }
    auto v = [&s] (const char* id) { return s.getRawParameterValue (id)->load(); };
    const float mag = v ("u_mag");
    // rilevatore dei transienti
    const float fA = (float) std::exp (-1.0 / (0.0005 * sr)), fR = (float) std::exp (-1.0 / (0.03 * sr)), sA = (float) std::exp (-1.0 / (0.025 * sr)), sR = (float) std::exp (-1.0 / (0.15 * sr));
    for (int i = 0; i < N; ++i)
    {
        dryL[(size_t) i] = L[i]; dryR[(size_t) i] = R[i];
        const float x = 0.5f * (std::abs (L[i]) + std::abs (R[i]));
        envF = x > envF ? fA * envF + (1 - fA) * x : fR * envF + (1 - fR) * x;
        envS = x > envS ? sA * envS + (1 - sA) * x : sR * envS + (1 - sR) * x;
        trW[(size_t) i] = jlimit (0.0f, 1.0f, (envF / (envS + 1e-5f) - 1.0f) * 0.6f);
    }
    float* ch[2] = { L, R };
    const float aLo = coef (200, sr), aHi = coef (3000, sr);
    auto focusSplit = [&] (int k, int c, float x, int foc, float& band) -> float   // ritorna il resto
    {
        const float lo = onePole (x, fLo[k][c], aLo), lph = onePole (x, fHi[k][c], aHi);
        if (foc == 0) { band = x; return 0.0f; }
        band = foc == 1 ? lo : foc == 3 ? x - lph : lph - lo; return x - band;
    };

    for (int step = 0; step < 6; ++step)
    {
        const int mod = step == 0 ? 0 : step == 5 ? 5 : juce::jlimit (1, 4, order[step - 1].load());
        switch (mod)
        {
        case 0:
        {
    // ---- 1. BOOM / NOISE ----
    if (v ("u_bOn") > 0.5f)
    {
        const float boom = v ("u_boom"), noise = v ("u_noise"); const bool acid = v ("u_acid") > 0.5f; const int nt = (int) v ("u_nType");
        const float bf = v ("u_bTone");
        const float nlpA = coef (nt == 3 ? 3000 : 9000, sr), hpA = coef (7000, sr);
        for (int i = 0; i < N; ++i)
        {
            const float tr = trW[(size_t) i];
            acidEnv = jmax (tr, acidEnv * 0.9995f);
            if (boom > 0)
            {
                const float f = acid ? bf * (1.0f + acidEnv * 2.5f) : bf, q = acid ? 9.0f : 1.6f;
                const float g = (float) std::tan (MathConstants<double>::pi * f / sr), k = 1.0f / q;
                for (int c = 0; c < 2; ++c)
                {
                    const float x = ch[c][i], v3 = x - bIc2[c], v1 = (bIc1[c] + g * v3) / (1 + g * (g + k)), v2 = bIc2[c] + g * v1;
                    bIc1[c] = 2 * v1 - bIc1[c]; bIc2[c] = 2 * v2 - bIc2[c];
                    const float y = acid ? std::tanh (v1 * k * 3.0f) : v1 * k;
                    ch[c][i] += y * boom * 2.6f;
                }
            }
            if (noise > 0)
            {
                const float env = jmin (1.0f, envF * 2.0f) * noise * 0.5f;
                if (nt == 2 && crackle <= 0 && rng.nextFloat() < 30.0f / (float) sr) { crackle = 6 + rng.nextInt (30); crackleV = (rng.nextFloat() * 2 - 1) * 0.6f; }
                for (int c = 0; c < 2; ++c)
                {
                    const float w = rng.nextFloat() * 2 - 1; float n = w;
                    switch (nt)
                    {
                        case 1: { auto& b = pinkB[c]; b[0] = 0.99765f * b[0] + w * 0.099046f; b[1] = 0.963f * b[1] + w * 0.2965164f; b[2] = 0.57f * b[2] + w * 1.0526913f; n = (b[0] + b[1] + b[2] + w * 0.1848f) * 0.25f; break; }
                        case 2: n = (crackle > 0 ? crackleV : 0.0f) + w * 0.04f; break;
                        case 3: n = onePole (w, nLp[c], nlpA) * 1.6f; break;
                        case 4: { const float a = onePole (w, nBp1[c], coef (5200, sr)), b2 = onePole (a, nBp2[c], coef (6800, sr)); n = (a - b2) * 6.0f; break; }
                        case 5: n = (w - onePole (w, nLp[c], hpA)) * 1.4f; break;
                        default: break;
                    }
                    ch[c][i] += n * env * (nt == 2 ? 1.0f : 1.0f);
                }
                if (crackle > 0) --crackle;
            }
        }
    }

        break;
        }
        case 1:
        {
    // ---- 2. SHIFT ----
    if (v ("u_shOn") > 0.5f && v ("u_shAmt") > 0.001f)
    {
        const float amt = v ("u_shAmt"), tgt = v ("u_shTgt"); const int foc = (int) v ("u_shFoc");
        const double ratio = std::pow (2.0, v ("u_shPitch") / 12.0), W = 0.045 * sr; const int BL = shBuf.getNumSamples();
        for (int i = 0; i < N; ++i)
        {
            shPh += 1.0 - ratio; while (shPh < 0) shPh += W; while (shPh >= W) shPh -= W;
            const double d1 = shPh, d2 = std::fmod (shPh + W * 0.5, W);
            const float g1 = 0.5f - 0.5f * std::cos ((float) (MathConstants<double>::twoPi * d1 / W)), g2 = 1 - g1;
            const float w = amt * tgtW (tgt, trW[(size_t) i]);
            for (int c = 0; c < 2; ++c)
            {
                float band; const float rest = focusSplit (0, c, ch[c][i], foc, band);
                shBuf.setSample (c, shW, band);
                auto rd = [&] (double d) { double p = shW - d - 2; while (p < 0) p += BL; const int a = (int) p; const float fr = (float) (p - a); return shBuf.getSample (c, a) * (1 - fr) + shBuf.getSample (c, (a + 1) % BL) * fr; };
                const float y = rd (d1) * g1 + rd (d2) * g2;
                ch[c][i] = rest + band + (y - band) * w;
            }
            if (++shW >= BL) shW = 0;
        }
    }

        break;
        }
        case 2:
        {
    // ---- 3. COMPRESS ----
    grMeter = grMeter.load() * 0.9f;
    if (v ("u_cOn") > 0.5f && std::abs (v ("u_cAmt")) > 0.001f)
    {
        const int type = (int) v ("u_cType"); const float amt = v ("u_cAmt"), bands = v ("u_cBands");
        struct CT { float att, rel, ratio; };
        static const CT T[] = { { 25, 90, 4 }, { 8, 250, 3 }, { 10, 200, 2 }, { 30, 300, 1.6f }, { 0.3f, 60, 20 }, { 2, 120, 6 } };
        const auto& t = T[jlimit (0, 5, type)];
        const float down = type == 5 ? std::abs (amt) : jmax (0.0f, amt), up = type == 5 ? std::abs (amt) * 0.45f : jmax (0.0f, -amt);
        const float cA = (float) std::exp (-1.0 / (t.att * 0.001 * sr)), cR = (float) std::exp (-1.0 / (t.rel * 0.001 * sr));
        const float thrD = -down * 36.0f, thrU = -22.0f, makeup = down * 36.0f * (1.0f - 1.0f / t.ratio) * 0.35f;
        const float x1 = coef (150, sr), x2 = coef (2500, sr);
        auto gainDb = [&] (float lev)
        {
            float g = 0;
            if (lev > thrD) g -= (lev - thrD) * (1.0f - 1.0f / t.ratio);
            if (up > 0 && lev < thrU && lev > -70.0f) g += jmin ((thrU - lev) * 0.6f, up * 18.0f);
            return g + makeup;
        };
        auto det = [&] (float& e, float x) { e = x > e ? cA * e + (1 - cA) * x : cR * e + (1 - cR) * x; return Decibels::gainToDecibels (e + 1e-7f); };
        float maxGr = 0;
        for (int i = 0; i < N; ++i)
        {
            const float l = ch[0][i], r = ch[1][i];
            const float gS = gainDb (det (cEnv[0], jmax (std::abs (l), std::abs (r))));
            maxGr = jmax (maxGr, -jmin (0.0f, gS - makeup));
            float sl = l * Decibels::decibelsToGain (gS), sr2 = r * Decibels::decibelsToGain (gS);
            if (bands > 0.001f)
            {
                float bl[3], br[3];
                const float ll = onePole (l, xLo[0], x1), lh = onePole (l, xHi[0], x2), rl = onePole (r, xLo[1], x1), rh = onePole (r, xHi[1], x2);
                bl[0] = ll; bl[2] = l - lh; bl[1] = lh - ll; br[0] = rl; br[2] = r - rh; br[1] = rh - rl;
                float ml = 0, mr = 0;
                for (int b = 0; b < 3; ++b) { const float g = Decibels::decibelsToGain (gainDb (det (cEnv[1 + b], jmax (std::abs (bl[b]), std::abs (br[b]))))); ml += bl[b] * g; mr += br[b] * g; }
                sl += (ml - sl) * bands; sr2 += (mr - sr2) * bands;
            }
            if (type == 1) { sl = std::tanh (sl * 1.2f) / 1.2f * 1.15f; sr2 = std::tanh (sr2 * 1.2f) / 1.2f * 1.15f; }
            ch[0][i] = sl; ch[1][i] = sr2;
        }
        grMeter = jmax (grMeter.load(), maxGr);
    }

        break;
        }
        case 3:
        {
    // ---- 4. SATURATE ----
    if (v ("u_sOn") > 0.5f && v ("u_sAmt") > 0.001f)
    {
        const int type = (int) v ("u_sType"); const float a = v ("u_sAmt"), st = v ("u_sStyle"), tgt = v ("u_sTgt"); const int foc = (int) v ("u_sFoc");
        const float d = 1.0f + a * 12.0f, comp = 1.0f / (1.0f + a * 1.2f), lpA = coef (14000 - st * 9000, sr);
        const double holdRate = 1.0 - st * 0.85; const float q = std::pow (2.0f, 14.0f - st * 10.0f);
        for (int i = 0; i < N; ++i)
        {
            const float w = tgtW (tgt, trW[(size_t) i]);
            bool tick = false; if (type == 4) { holdPh += holdRate; if (holdPh >= 1.0) { holdPh -= 1.0; tick = true; } }
            for (int c = 0; c < 2; ++c)
            {
                float band; const float rest = focusSplit (1, c, ch[c][i], foc, band);
                const float x = band * d; float y;
                switch (type)
                {
                    case 1: y = onePole (std::tanh (x * 0.8f) + 0.08f * st * x * x / (1 + x * x), satLp[c], lpA); break;
                    case 2: y = std::atan (x * (1 + st)) / std::atan (1.0f + st) * 0.8f; y = onePole (y, satLp[c], coef (16000 - st * 6000, sr)); break;
                    case 3: y = x > 0 ? std::tanh (x) : std::tanh (x * (0.55f + 0.45f * (1 - st))) / (0.55f + 0.45f * (1 - st)) * 0.8f; y += 0.12f * st * y * y; break;
                    case 4: { if (tick) holdV[c] = jlimit (-1.0f, 1.0f, x); y = std::round (holdV[c] * q) / q; break; }
                    case 5: y = std::sin (x * (1.0f + st * 2.0f)) * 0.9f; break;
                    default: y = std::tanh (x + st * 0.3f * x * x / (1 + std::abs (x))); break;
                }
                y *= comp;
                ch[c][i] = rest + band + (y - band) * w;
            }
        }
    }

        break;
        }
        case 4:
        {
    // ---- 5. SPACE ----
    if (v ("u_spOn") > 0.5f && v ("u_spAmt") > 0.001f)
    {
        const int type = (int) v ("u_spType"); const float amt = v ("u_spAmt"), dec = v ("u_spDecay"), duck = v ("u_spDuck"), width = v ("u_spWidth");
        const int pre = jlimit (1, preBuf.getNumSamples() - 2, (int) (v ("u_spPre") * 0.001 * sr)); const int PL = preBuf.getNumSamples();
        Reverb::Parameters rp; rp.wetLevel = 1; rp.dryLevel = 0; rp.width = 1;
        static const float base[] = { 0.45f, 0.72f, 0.62f, 0.55f }, span[] = { 0.3f, 0.26f, 0.34f, 0.3f }, damp[] = { 0.45f, 0.3f, 0.05f, 0.6f };
        rp.roomSize = jmin (0.98f, base[type] + span[type] * dec); rp.damping = damp[type];
        verb.setParameters (rp);
        if (type != lastSpType) { verb.reset(); lastSpType = type; }
        for (int i = 0; i < N; ++i)
        {
            preBuf.setSample (0, preW, ch[0][i]); preBuf.setSample (1, preW, ch[1][i]);
            int r = preW - pre; if (r < 0) r += PL;
            wetL[(size_t) i] = preBuf.getSample (0, r); wetR[(size_t) i] = preBuf.getSample (1, r);
            if (++preW >= PL) preW = 0;
        }
        verb.processStereo (wetL.data(), wetR.data(), N);
        for (int i = 0; i < N; ++i)
        {
            float wl = wetL[(size_t) i], wr = wetR[(size_t) i];
            if (type == 3)   // molla: catena di allpass dispersivi
                for (int k = 0; k < 4; ++k) { const float a = 0.62f; float y = -a * wl + apZ[0][k]; apZ[0][k] = wl + a * y; wl = y; y = -a * wr + apZ[1][k]; apZ[1][k] = wr + a * y; wr = y; }
            const float m = 0.5f * (wl + wr), sd = 0.5f * (wl - wr) * width; wl = m + sd; wr = m - sd;
            const float g = amt * 1.3f * (1.0f - duck * jlimit (0.0f, 0.95f, envF * 4.0f));
            ch[0][i] += wl * g; ch[1][i] += wr * g;
        }
    }

        break;
        }
        case 5:
        {
    // ---- 6. CLIP (more) ----
    if (v ("u_mOn") > 0.5f && v ("u_mAmt") > 0.001f)
    {
        const float a = v ("u_mAmt"), tone = v ("u_mTone"), tgt = v ("u_mTgt"); const int stages = (int) v ("u_mStages") + 1, foc = (int) v ("u_mFoc");
        const float d = 1.0f + a * 10.0f, comp = 1.0f / (1.0f + a * 0.8f), tA = coef (2000, sr);
        for (int i = 0; i < N; ++i)
        {
            const float w = tgtW (tgt, trW[(size_t) i]);
            for (int c = 0; c < 2; ++c)
            {
                float band; const float rest = focusSplit (2, c, ch[c][i], foc, band);
                float y = band * d;
                for (int s2 = 0; s2 < stages; ++s2) y = (s2 % 2 == 0) ? std::tanh (y * 1.1f) : jlimit (-0.9f, 0.9f, y * 1.25f);
                const float lp = onePole (y, mLp[c], tA); y = lp * (1 - tone * 0.6f) + (y - lp) * (1 + tone * 0.6f);
                y *= comp;
                ch[c][i] = rest + band + (y - band) * w;
            }
        }
    }

        break;
        }
        default: break;
        }
    }
    // ---- MASTER ----
    {
        const float tone = v ("u_tone"), air = v ("u_air"), att = v ("u_att"), sus = v ("u_sus"); const bool clip = v ("u_clip") > 0.5f;
        const float tA = coef (900, sr), aA = coef (9000, sr);
        const bool eqOn = v ("u_eqOn") > 0.5f;
        {
            const float pv[11] = { v ("u_e1g"), v ("u_e1f"), v ("u_e2g"), v ("u_e2f"), v ("u_e2q"), v ("u_e3g"), v ("u_e3f"), v ("u_e3q"), v ("u_e4g"), v ("u_e4f"), 0 };
            if (std::memcmp (pv, lastEq, sizeof (pv)) != 0)
            {
                eq[0].set (0, sr, pv[1], pv[0], 0.707); eq[1].set (1, sr, pv[3], pv[2], pv[4]); eq[2].set (1, sr, pv[6], pv[5], pv[7]); eq[3].set (2, sr, pv[9], pv[8], 0.707);
                std::memcpy (lastEq, pv, sizeof (pv));
            }
        }
        for (int i = 0; i < N; ++i)
        {
            const float x = 0.5f * (std::abs (ch[0][i]) + std::abs (ch[1][i]));
            mEnvF = x > mEnvF ? fA * mEnvF + (1 - fA) * x : fR * mEnvF + (1 - fR) * x;
            mEnvS = x > mEnvS ? sA * mEnvS + (1 - sA) * x : sR * mEnvS + (1 - sR) * x;
            const float tr = jlimit (0.0f, 1.0f, (mEnvF / (mEnvS + 1e-5f) - 1.0f) * 0.6f);
            const float g = jlimit (0.2f, 3.0f, (1.0f + att * tr * 1.5f) * (1.0f + sus * (1.0f - tr) * 0.7f));
            for (int c = 0; c < 2; ++c)
            {
                float y = ch[c][i];
                if (tone != 0) { const float lp = onePole (y, tLp[c], tA); y = lp * (1 - tone * 0.5f) + (y - lp) * (1 + tone * 0.5f); }
                if (air > 0) { const float lp = onePole (y, aLp[c], aA); y += (y - lp) * air * 2.0f; }
                y *= g;
                if (eqOn) for (int k = 0; k < 4; ++k) if (std::abs (lastEq[k == 0 ? 0 : k == 1 ? 2 : k == 2 ? 5 : 8]) > 0.05f) y = eq[k].run (y, c);
                if (clip) { const float ay = std::abs (y); if (ay > 0.8f) y = std::copysign (0.8f + 0.2f * std::tanh ((ay - 0.8f) / 0.2f), y); }
                const float dry = c == 0 ? dryL[(size_t) i] : dryR[(size_t) i];
                ch[c][i] = dry + (y - dry) * mag;
            }
            // dati per i display
            const float in = 0.5f * (std::abs (dryL[(size_t) i]) + std::abs (dryR[(size_t) i])), out = 0.5f * (std::abs (ch[0][i]) + std::abs (ch[1][i]));
            accIn = jmax (accIn, in); accOut = jmax (accOut, out); accTr = jmax (accTr, trW[(size_t) i]);
            const int aw = anW.load (std::memory_order_relaxed);
            anIn[aw] = 0.5f * (dryL[(size_t) i] + dryR[(size_t) i]); anOut[aw] = 0.5f * (ch[0][i] + ch[1][i]); anW.store ((aw + 1) % AN, std::memory_order_relaxed);
            if (++accN >= (int) (sr / 120))
            {
                scIn.push (accIn); scOut.push (accOut); scGr.push (grMeter.load()); scTr.push (accTr);
                accIn = accOut = accTr = 0; accN = 0;
            }
        }
    }
}
}

namespace jb
{
static void rbj (int type, double sr, double f, double gDb, double q, double& b0, double& b1, double& b2, double& a0, double& a1, double& a2)
{
    const double A = std::pow (10.0, gDb / 40.0), w = juce::MathConstants<double>::twoPi * juce::jlimit (10.0, sr * 0.45, f) / sr, cs = std::cos (w), sn = std::sin (w);
    if (type == 1) { const double al = sn / (2 * q); b0 = 1 + al * A; b1 = -2 * cs; b2 = 1 - al * A; a0 = 1 + al / A; a1 = -2 * cs; a2 = 1 - al / A; return; }
    const double sA = std::sqrt (A) * sn / std::sqrt (2.0);
    if (type == 0) { b0 = A * ((A + 1) - (A - 1) * cs + sA); b1 = 2 * A * ((A - 1) - (A + 1) * cs); b2 = A * ((A + 1) - (A - 1) * cs - sA); a0 = (A + 1) + (A - 1) * cs + sA; a1 = -2 * ((A - 1) + (A + 1) * cs); a2 = (A + 1) + (A - 1) * cs - sA; }
    else { b0 = A * ((A + 1) + (A - 1) * cs + sA); b1 = -2 * A * ((A - 1) + (A + 1) * cs); b2 = A * ((A + 1) + (A - 1) * cs - sA); a0 = (A + 1) - (A - 1) * cs + sA; a1 = 2 * ((A - 1) - (A + 1) * cs); a2 = (A + 1) - (A - 1) * cs - sA; }
}
void Biq::set (int type, double sr, double f, double gDb, double q)
{
    double B0, B1, B2, A0, A1, A2; rbj (type, sr, f, gDb, q, B0, B1, B2, A0, A1, A2);
    b0 = (float) (B0 / A0); b1 = (float) (B1 / A0); b2 = (float) (B2 / A0); a1 = (float) (A1 / A0); a2 = (float) (A2 / A0);
}
float Biq::magDb (int type, double sr, double f, double gDb, double q, double hz)
{
    double B0, B1, B2, A0, A1, A2; rbj (type, sr, f, gDb, q, B0, B1, B2, A0, A1, A2);
    const std::complex<double> z = std::polar (1.0, -juce::MathConstants<double>::twoPi * hz / sr), z2 = z * z;
    return (float) (20.0 * std::log10 (std::abs ((B0 + B1 * z + B2 * z2) / (A0 + A1 * z + A2 * z2)) + 1e-12));
}
}
