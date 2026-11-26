#pragma once
// CITY EDGERUNNER: mette la voce (o qualsiasi cosa) dentro la citta'.
// Sei scene; ogni scena e' un insieme di numeri (filtri, saturazione, bit, cadute di segnale, rumori di fondo, eco, stanza).
// Cambiando scena i numeri scivolano verso quelli nuovi, quindi nessun click e nessun buco.
// Un solo comando principale, DENTRO LA CITTA': a zero l'audio passa identico.
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <cmath>
#include <vector>

namespace jcity
{
constexpr int kScenes = 6;
enum F { HP, LP, PKF, PKG, DRV, HOLD, HOLDMIX, BITS, WOB, WOBHZ, DROP, DROPLEN, STUT, CRACK, HISS, RAIN, RUMB, ECHOMS, ECHOFB, ECHO, VSIZE, VDAMP, VERB, TREM, TREMHZ, MONO, GAIN, NF };
struct Scene { const char* name; const char* sign; const char* line; std::array<float, NF> f; };
inline const std::array<Scene, kScenes>& scenes()
{
    //                                       HP    LP     PKF   PKG  DRV  HOLD   HMIX BITS WOB  WOBHZ DROP DLEN  STUT CRACK HISS RAIN RUMB ECHOMS FB   ECHO VSIZE VDAMP VERB TREM THZ MONO GAIN
    static const std::array<Scene, kScenes> s = { {
        { "RADIO PIRATA", "RADIO", "una frequenza rubata, banda stretta, fruscio e sfumate",
          { 380, 3600, 1500, 0.7f, 0.55f, 44100, 0, 0, 0.5f, 0.25f, 0.35f, 0.22f, 0, 1.0f, 0.7f, 0, 0, 0, 0, 0, 0.3f, 0.6f, 0.06f, 0.45f, 0.35f, 1, 1.7f } },
        { "MEGAFONO", "MEGAFONO", "dall'altoparlante in strada, che rimbalza sui palazzi",
          { 750, 4300, 2100, 1.0f, 0.85f, 44100, 0, 0, 0, 0.5f, 0, 0.1f, 0, 0.12f, 0.08f, 0, 0.1f, 150, 0.32f, 0.45f, 0.75f, 0.5f, 0.3f, 0, 1, 1, 4.2f } },
        { "CHIAMATA DISTURBATA", "CHIAMATA", "telefono criptato che taglia e si rompe",
          { 320, 3300, 1100, 0.35f, 0.25f, 8000, 1, 7, 0, 0.5f, 1.3f, 0.09f, 0, 0, 0.22f, 0, 0, 0, 0, 0, 0.2f, 0.5f, 0, 0, 1, 1, 1.6f } },
        { "OLOGRAMMA", "OLOGRAMMA", "proiezione che perde segnale, trema e si incanta",
          { 160, 9500, 3200, 0.5f, 0.2f, 14000, 1, 10, 2.8f, 5.5f, 2.6f, 0.06f, 1, 0.35f, 0.12f, 0, 0, 65, 0.4f, 0.28f, 0.8f, 0.3f, 0.32f, 0.22f, 9, 0, 1.15f } },
        { "CLUB SOTTO LA PIOGGIA", "CLUB", "fuori dal club, il muro si mangia gli acuti e piove",
          { 50, 1300, 110, 0.6f, 0.15f, 44100, 0, 0, 0, 0.5f, 0, 0.1f, 0, 0, 0, 1.0f, 0.2f, 0, 0, 0, 0.55f, 0.75f, 0.32f, 0, 1, 0, 1.15f } },
        { "METRO", "METRO", "galleria della metro: ferro, rimbombo, treni che passano",
          { 120, 5600, 850, 0.45f, 0.1f, 44100, 0, 0, 0, 0.5f, 0, 0.1f, 0, 0.05f, 0.05f, 0, 1.0f, 95, 0.5f, 0.3f, 0.95f, 0.3f, 0.5f, 0, 1, 0, 1.15f } },
    } };
    return s;
}
struct Settings { int scene = 0; float city = 0, distance = 0.3f, disturb = 0.5f, ambience = 0.5f, width = 0.5f, tone = 0, outDb = 0; };

struct Svf   // filtro a variabili di stato (TPT): resta stabile anche se lo muovi di continuo
{
    float ic1 = 0, ic2 = 0, g = 0, k = 1, a1 = 0, a2 = 0, a3 = 0;
    void set (float fc, float sr, float q) { g = std::tan (juce::MathConstants<float>::pi * juce::jlimit (10.0f, sr * 0.45f, fc) / sr); k = 1.0f / q; a1 = 1.0f / (1.0f + g * (g + k)); a2 = g * a1; a3 = g * a2; }
    void tick (float x, float& lp, float& bp, float& hp)
    {
        const float v3 = x - ic2, v1 = a1 * ic1 + a2 * v3, v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2 * v1 - ic1; ic2 = 2 * v2 - ic2; lp = v2; bp = v1; hp = x - k * v1 - v2;
    }
    void reset() { ic1 = ic2 = 0; }
};

class Engine
{
public:
    void prepare (double s, int maxBlock)
    {
        sr = (float) s; mb = juce::jmax (32, maxBlock);
        for (auto* v : { &wL, &wR, &rvL, &rvR, &vg }) v->assign ((size_t) mb, 0.0f);
        for (auto& b : wob) b.assign (kWob, 0.0f);
        for (auto& b : echo) b.assign (kEcho, 0.0f);
        rev.setSampleRate (s); rev.reset();
        cur = scenes()[0].f; first = true; citySm = 0; wpos = epos = 0; rng.setSeedRandomly();
        for (auto& c : ch) c = {};
    }
    // monitor per l'interfaccia
    float gateView() const { return gateUi; }
    float outView() const { return outUi; }

    void process (float* L, float* R, int n, const Settings& S)
    {
        for (int o = 0; o < n; o += mb) processChunk (L + o, R + o, juce::jmin (mb, n - o), S);
    }
private:
    static constexpr int kWob = 1 << 13, kEcho = 1 << 17, kSub = 16;
    struct Ch { Svf hp1, hp2, lp1, lp2, pk, eLp, rain; float hold = 0, dropEnv = 1, crk = 0, brown = 0, rainEnv = 0, wobRnd = 0, wobRndT = 0; };
    float sr = 44100; int mb = 512, wpos = 0, epos = 0;
    std::array<float, NF> cur {}; bool first = true;
    std::array<Ch, 2> ch {};
    std::array<std::vector<float>, 2> wob, echo;
    std::vector<float> wL, wR, rvL, rvR, vg;
    juce::Reverb rev; juce::Reverb::Parameters rp;
    juce::Random rng;
    float citySm = 0, distSm = 0.3f, disSm = 0.5f, ambSm = 0.5f, widthSm = 0.5f, toneSm = 0, outSm = 1;
    double holdPh = 0, wobPh = 0, tremPh = 0, trainPh = 0; float tremRnd = 0.5f;
    int dropLeft = 0, loopStart = 0, loopLen = 1, loopPos = 0;
    float gateUi = 1, outUi = 0;

    float noise() { return rng.nextFloat() * 2.0f - 1.0f; }

    void processChunk (float* L, float* R, int n, const Settings& S)
    {
        const auto& tgt = scenes()[(size_t) juce::jlimit (0, kScenes - 1, S.scene)].f;
        if (first) { cur = tgt; first = false; distSm = S.distance; disSm = S.disturb; ambSm = S.ambience; widthSm = S.width; toneSm = S.tone; outSm = juce::Decibels::decibelsToGain (S.outDb); }
        const float cityT = juce::jlimit (0.0f, 1.0f, S.city);
        if (cityT <= 0.0f && citySm <= 1e-5f) { citySm = 0; gateUi = 1; outUi = 0; return; }   // fuori dalla citta': passa identico
        const float sub = 1.0f - std::exp (-(float) kSub / (0.12f * sr));   // le scene scivolano in ~120 ms
        const float ksm = 1.0f - std::exp (-1.0f / (0.03f * sr));
        const float outT = juce::Decibels::decibelsToGain (S.outDb);
        const float invSr = 1.0f / sr;
        // ---------------------------------------------------------------- 1: catena per campione, fino all'eco
        for (int i0 = 0; i0 < n; i0 += kSub)
        {
            const int i1 = juce::jmin (n, i0 + kSub);
            for (int f = 0; f < NF; ++f) cur[(size_t) f] += (tgt[(size_t) f] - cur[(size_t) f]) * sub;
            distSm += (S.distance - distSm) * sub; disSm += (S.disturb - disSm) * sub; ambSm += (S.ambience - ambSm) * sub; toneSm += (S.tone - toneSm) * sub; widthSm += (S.width - widthSm) * sub;
            const float dis = disSm, amb = ambSm, dist = distSm;
            const float tmul = std::pow (2.0f, toneSm * 1.2f);
            const float lpF = cur[LP] * tmul * std::pow (2.0f, -2.6f * dist), hpF = cur[HP] * tmul;
            for (auto& c : ch) { c.hp1.set (hpF, sr, 0.707f); c.hp2.set (hpF, sr, 0.707f); c.lp1.set (lpF, sr, 0.707f); c.lp2.set (lpF, sr, 0.707f); c.pk.set (cur[PKF], sr, 1.4f); c.eLp.set (2800.0f * std::pow (2.0f, -dist), sr, 0.707f); }
            const float drv = cur[DRV] * (0.35f + 0.65f * dis), gd = 1.0f + drv * 14.0f, gdn = 0.5f / std::tanh (0.5f * gd);
            const float holdMix = cur[HOLDMIX] * (0.3f + 0.7f * dis), holdInc = cur[HOLD] / sr;
            const float bits = 16.0f - (16.0f - juce::jmax (4.0f, cur[BITS] > 0 ? cur[BITS] : 16.0f)) * (0.3f + 0.7f * dis), q = std::pow (2.0f, bits - 1.0f);
            const float wobS = cur[WOB] * dis * 0.001f * sr;
            const float dropRate = cur[DROP] * dis * dis, trem = cur[TREM] * dis;
            const float crack = cur[CRACK] * dis, hiss = cur[HISS] * (0.4f + 0.6f * dis) * 0.012f;
            const float rainL = cur[RAIN] * amb, rumbL = cur[RUMB] * amb;
            const float echoD = juce::jlimit (1.0f, (float) kEcho - 4, cur[ECHOMS] * (1.0f + 0.4f * dist) * 0.001f * sr), echoFb = cur[ECHOFB] * (0.5f + 0.5f * amb), echoL = cur[ECHO] * (0.4f + 0.6f * amb) * (0.6f + 0.8f * dist);
            const float direct = (1.0f - 0.72f * dist) * cur[GAIN], verbG = cur[VERB] * (0.3f + 0.9f * amb) * (0.7f + 1.3f * dist);
            const float mono = cur[MONO], pkG = cur[PKG];
            for (int i = i0; i < i1; ++i)
            {
                // le cose che cambiano nel tempo, uguali per i due canali
                holdPh += holdInc; const bool grab = holdPh >= 1.0; if (grab) holdPh -= std::floor (holdPh);
                wobPh += (double) (cur[WOBHZ] * invSr); if (wobPh > 1) wobPh -= 1;
                tremPh += (double) (cur[TREMHZ] * invSr); if (tremPh > 1) tremPh -= 1;
                trainPh += (double) (0.035f * invSr); if (trainPh > 1) trainPh -= 1;
                tremRnd += ((rng.nextFloat()) - tremRnd) * (1.0f / (0.5f * sr));
                // caduta di segnale (e, nell'ologramma, il segnale che si incanta)
                if (dropLeft > 0) --dropLeft;
                else if (rng.nextFloat() < dropRate * invSr)
                {
                    dropLeft = (int) (cur[DROPLEN] * sr * (0.5f + rng.nextFloat()));
                    loopLen = juce::jmax (64, (int) (sr * (0.025f + 0.04f * rng.nextFloat()))); loopStart = (wpos - loopLen + kWob) & (kWob - 1); loopPos = 0;
                }
                const float gateT = dropLeft > 0 ? 0.0f : 1.0f;
                const float tr = 1.0f - trem * (0.5f + 0.5f * (float) std::sin (juce::MathConstants<double>::twoPi * tremPh)) * (0.6f + 0.8f * tremRnd);
                const float train = std::pow (juce::jmax (0.0f, (float) std::sin (juce::MathConstants<double>::twoPi * trainPh)), 3.0f);
                const float dl = wobS * (0.5f - 0.5f * (float) std::cos (juce::MathConstants<double>::twoPi * wobPh));
                const float inM = 0.5f * (L[i] + R[i]);
                float out[2];
                for (int c = 0; c < 2; ++c)
                {
                    auto& C = ch[(size_t) c];
                    const float xin = c == 0 ? L[i] : R[i];
                    float x = xin + (inM - xin) * mono;
                    // rumori che passano dalla radio / dal telefono
                    x += noise() * hiss;
                    if (rng.nextFloat() < 40.0f * crack * invSr) C.crk = noise() * 0.35f * crack;
                    x += C.crk; C.crk *= 0.92f;
                    float lp, bp, hp;
                    C.hp1.tick (x, lp, bp, hp); C.hp2.tick (hp, lp, bp, hp);
                    C.lp1.tick (hp, lp, bp, hp); C.lp2.tick (lp, lp, bp, hp);
                    float y = lp; { float l2, b2, h2; C.pk.tick (y, l2, b2, h2); y += pkG * b2; }
                    // saturazione
                    y = std::tanh (y * gd) * gdn;
                    // bit e campioni
                    if (grab || holdMix < 1e-4f) C.hold = y;
                    const float yh = y + (C.hold - y) * holdMix;
                    y = yh + (std::round (yh * q) / q - yh) * juce::jmin (1.0f, holdMix + (cur[BITS] > 0 ? dis : 0.0f));
                    // il tremolio dell'ologramma / della radio
                    auto& wb = wob[(size_t) c]; wb[(size_t) wpos] = y;
                    if (wobS > 0.01f)
                    {
                        const float rp = (float) wpos - dl; const int i0r = (int) std::floor (rp); const float fr = rp - (float) i0r;
                        const float a = wb[(size_t) (i0r & (kWob - 1))], b = wb[(size_t) ((i0r + 1) & (kWob - 1))];
                        y = a + (b - a) * fr;
                    }
                    // caduta di segnale
                    C.dropEnv += (gateT - C.dropEnv) * (gateT > C.dropEnv ? 0.004f : 0.02f) * (48000.0f / sr);
                    const float lpv = wb[(size_t) ((loopStart + loopPos) & (kWob - 1))];
                    y = y * C.dropEnv + (1.0f - C.dropEnv) * (cur[STUT] * lpv * 0.85f + noise() * 0.02f * crack);
                    y *= tr;
                    // la citta' intorno: pioggia e treni (non passano dalla radio)
                    float amb2 = 0;
                    if (rainL > 0.001f)
                    {
                        if (rng.nextFloat() < 55.0f * invSr) { C.rain.set (1800.0f + rng.nextFloat() * 4500.0f, sr, 9.0f); C.rainEnv = 0.25f + rng.nextFloat() * 0.5f; }
                        float rl, rb, rh; C.rain.tick (noise() * C.rainEnv, rl, rb, rh); C.rainEnv *= 0.993f;
                        amb2 += rainL * (rb * 0.06f + noise() * 0.006f);
                    }
                    if (rumbL > 0.001f) { C.brown += (noise() - C.brown) * (90.0f * invSr); amb2 += rumbL * C.brown * (0.35f + 1.4f * train); }
                    out[c] = y * direct + amb2;
                }
                if (dropLeft > 0) loopPos = (loopPos + 1) % loopLen;
                wpos = (wpos + 1) & (kWob - 1);
                // eco sui palazzi / in galleria (incrociata)
                for (int c = 0; c < 2; ++c)
                {
                    auto& eb = echo[(size_t) c];
                    const float d = echoD * (c == 0 ? 1.0f : 1.07f);
                    const float rp = (float) epos - d; const int i0r = (int) std::floor (rp); const float fr = rp - (float) i0r;
                    const float a = eb[(size_t) (i0r & (kEcho - 1))], b = eb[(size_t) ((i0r + 1) & (kEcho - 1))];
                    const float e = a + (b - a) * fr;
                    float el, ebp, eh; ch[(size_t) c].eLp.tick (e, el, ebp, eh);
                    const float other = c == 0 ? out[1] : out[0];
                    (c == 0 ? wL : wR)[(size_t) i] = out[c] + el * echoL;
                    eb[(size_t) epos] = juce::jlimit (-2.0f, 2.0f, (0.7f * out[c] + 0.3f * other) * (echoL > 0.001f ? 1.0f : 0.0f) + el * echoFb);
                }
                epos = (epos + 1) & (kEcho - 1);
                vg[(size_t) i] = verbG;
                gateUi = ch[0].dropEnv;
            }
            rp.roomSize = juce::jlimit (0.0f, 1.0f, cur[VSIZE] * (0.75f + 0.25f * amb)); rp.damping = cur[VDAMP]; rp.wetLevel = 1; rp.dryLevel = 0; rp.width = 1; rp.freezeMode = 0;
        }
        // ---------------------------------------------------------------- 2: stanza / strada
        rev.setParameters (rp);
        std::copy (wL.begin(), wL.begin() + n, rvL.begin()); std::copy (wR.begin(), wR.begin() + n, rvR.begin());
        rev.processStereo (rvL.data(), rvR.data(), n);
        // ---------------------------------------------------------------- 3: larghezza, uscita, dentro/fuori
        float pk = 0;
        for (int i = 0; i < n; ++i)
        {
            citySm += (cityT - citySm) * ksm; if (cityT <= 0 && citySm < 1e-5f) citySm = 0;
            outSm += (outT - outSm) * ksm;
            float a = wL[(size_t) i] + rvL[(size_t) i] * vg[(size_t) i] * 0.35f, b = wR[(size_t) i] + rvR[(size_t) i] * vg[(size_t) i] * 0.35f;
            const float m = 0.5f * (a + b), s = 0.5f * (a - b) * (widthSm * 2.0f);
            a = (m + s) * outSm; b = (m - s) * outSm;
            const float c = citySm;
            L[i] = L[i] * (1.0f - c) + a * c; R[i] = R[i] * (1.0f - c) + b * c;
            pk = juce::jmax (pk, std::abs (a), std::abs (b));
        }
        outUi = juce::jmax (pk, outUi * 0.8f);
    }
};
}
