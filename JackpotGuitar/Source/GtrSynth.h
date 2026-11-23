#pragma once
// Chitarra per ascoltare il giro: sei corde Karplus-Strong (una nota per corda, come sullo strumento), pennata filtrata
// sul punto di attacco, stoppate e palm mute, legati e bend sulla corda, cassa armonica o pickup, spazio e raddoppio.
#include "GtrEngine.h"
#include "JackpotKit.h"
#include "PerfVerb.h"

namespace gtr
{
enum Body { NYLON, ACUSTICA, PULITA, CRUNCH, kBodies };
inline const char* bodyName (int b) { static const char* n[kBodies] = { "NYLON", "ACUSTICA", "ELETTRICA", "CRUNCH" }; return n[juce::jlimit (0, kBodies - 1, b)]; }

struct Hit { int str = 0, note = 60; float vel = 0.7f; bool mute = false, pm = false; int art = 0; };

class KS
{
public:
    void prepare (double s) { sr = s; buf.assign (8192, 0.0f); mask = 8191; reset(); }
    void reset() { std::fill (buf.begin(), buf.end(), 0.0f); w = 0; prevY = 0; env = envT = 0; on = false; exN = 0; dc = dcY = 0; }
    bool on = false; float env = 0, envT = 0;
    // pizzicata: riempie l'eccitazione (rumore filtrato, pettine del punto di attacco)
    void pluck (float hz, float vel, int body, float tone, bool mute, bool pm, juce::Random& rnd, bool fresh = true)
    {
        const bool steel = body != NYLON;
        S = (steel ? 0.22f : 0.36f) + (1.0f - tone) * 0.22f; if (pm || mute) S = 0.49f;
        if (fresh) { std::fill (buf.begin(), buf.end(), 0.0f); prevY = 0; dc = dcY = 0; }
        setPitch (hz, true);
        const float P = (float) (sr / hz);
        float cut = (steel ? 2500.0f : 1400.0f) * std::pow (2.0f, 2.6f * vel * (0.4f + tone)) ; if (pm || mute) cut *= 0.35f;
        cut = juce::jmin (cut, (float) sr * 0.42f);
        const float a = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * cut / (float) sr);
        const int n = juce::jlimit (8, 4000, (int) P);
        const int comb = juce::jmax (1, (int) (P * (steel ? 0.13f : 0.2f)));
        exc.assign ((size_t) n, 0.0f); float lp = 0, lp2 = 0;
        for (int i = 0; i < n; ++i) { lp += a * (rnd.nextFloat() * 2 - 1 - lp); lp2 += a * (lp - lp2); exc[(size_t) i] = lp2; }
        for (int i = n - 1; i >= comb; --i) exc[(size_t) i] -= exc[(size_t) (i - comb)];
        float pk = 1e-6f; for (float x : exc) pk = juce::jmax (pk, std::abs (x));
        const float amp = (0.18f + 0.82f * vel * vel) / pk * (mute ? 0.5f : pm ? 0.8f : 1.0f);
        for (auto& x : exc) x *= amp;
        exN = n; exI = 0;
        // decadimento: corde basse piu' lunghe; nylon piu' breve; stoppata e palm mute quasi subito
        float t60 = (steel ? 5.5f : 3.6f) * std::pow (110.0f / juce::jmax (60.0f, hz), 0.45f);
        if (body == PULITA || body == CRUNCH) t60 *= 1.25f;
        if (pm) t60 = 0.28f; if (mute) t60 = 0.045f;
        t60base = t60; setT60 (t60);
        env = fresh ? 1.0f : env; envT = 1.0f; on = true; released = false;
    }
    // legato: la corda continua a vibrare, cambia il tasto (hammer-on con un piccolo colpo, scivolata morbida)
    void legato (float hz, int art, float vel, juce::Random& rnd)
    {
        glide = art == 2 ? (float) (1.0 - std::exp (-1.0 / (0.05 * sr))) : (float) (1.0 - std::exp (-1.0 / (0.0015 * sr)));
        setPitch (hz, false); released = false; envT = 1.0f; on = true; setT60 (t60base);
        if (art == 1)
        {   // il dito che batte: un impulso corto e scuro
            const int n = juce::jlimit (8, 400, (int) (sr / hz * 0.5)); exc.assign ((size_t) n, 0.0f); float lp = 0;
            for (int i = 0; i < n; ++i) { lp += 0.2f * (rnd.nextFloat() * 2 - 1 - lp); exc[(size_t) i] = lp * 0.25f * vel; }
            exN = n; exI = 0;
        }
    }
    void setPitch (float hz, bool jump) { hz0 = hz; dT = (float) (sr / (hz * std::pow (2.0f, bend / 12.0f))) - S; if (jump) { d = dT; glide = (float) (1.0 - std::exp (-1.0 / (0.0008 * sr))); } }
    void setBend (float semis) { bend = semis; dT = (float) (sr / (hz0 * std::pow (2.0f, bend / 12.0f))) - S; }
    void setT60 (float t) { const float f = hz0 * std::pow (2.0f, bend / 12.0f); gT = std::pow (10.0f, -3.0f / juce::jmax (0.01f, t * f)); }
    void release() { if (! released) { released = true; setT60 (0.09f); } }
    void fade() { envT = 0; }   // corda fermata o ripizzicata: sfuma in pochi millisecondi
    float hz0 = 110, bend = 0;
    inline float tick()
    {
        d += glide * (dT - d);
        g += 0.004f * (gT - g);
        env += (envT > env ? 0.05f : 0.0035f) * (envT - env);
        const float dd = juce::jlimit (2.0f, (float) mask - 4, d);
        const float rp = (float) w - dd; const int i0 = (int) std::floor (rp); const float fr = rp - (float) i0;
        const float y = buf[(size_t) (i0 & mask)] + fr * (buf[(size_t) ((i0 + 1) & mask)] - buf[(size_t) (i0 & mask)]);
        const float filt = (1.0f - S) * y + S * prevY; prevY = y;
        float in = 0; if (exI < exN) in = exc[(size_t) exI++];
        buf[(size_t) w] = filt * g + in; w = (w + 1) & mask;
        // blocco della continua (il rumore filtrato lascia un piccolo offset)
        const float o = y - dc + 0.995f * dcY; dc = y; dcY = o;
        if (env < 1e-4f && envT == 0) { on = false; }
        return o * env;
    }
    float level() const { return env; }
private:
    double sr = 48000; std::vector<float> buf, exc; int mask = 8191, w = 0, exN = 0, exI = 0;
    float d = 100, dT = 100, glide = 0.01f, g = 0.99f, gT = 0.99f, S = 0.3f, prevY = 0, t60base = 3, dc = 0, dcY = 0; bool released = false;
};

class Strings
{
public:
    void prepare (double s, int maxBlock)
    {
        sr = s;
        for (auto& st : str) for (auto& u : st.u) u.prepare (sr);
        verb.prepare (sr); vl.assign ((size_t) maxBlock, 0); vr.assign ((size_t) maxBlock, 0); wl.assign ((size_t) maxBlock, 0); wr.assign ((size_t) maxBlock, 0);
        dbl.assign ((size_t) (0.05 * sr) + 8, 0.0f); dw = 0;
        reset(); bodyCur = body; bFade = 1.0f; fBody = -1; filtersFor (true);
    }
    void reset()
    {
        for (auto& st : str) { for (auto& u : st.u) u.reset(); st.cur = 0; st.note = -1; st.bendS = 0; st.bendT = 0; }
        for (auto& b : bq) for (auto& f : b) f.reset();
        verb.reset(); std::fill (dbl.begin(), dbl.end(), 0.0f); lfo = 0; drvS = 0; ctr = 0;
    }
    // parametri (dal processor, ogni blocco)
    int body = ACUSTICA; float tone = 0.5f, bodyAmt = 0.6f, space = 0.2f, width = 0.3f, vol = 0.8f, drive = 0.5f;
    void hit (const Hit& h)
    {
        auto& st = str[(size_t) juce::jlimit (0, kStr - 1, h.str)];
        const float hz = 440.0f * std::pow (2.0f, (h.note - 69) / 12.0f);
        auto& cu = st.u[(size_t) st.cur];
        if (h.art != 0 && cu.on && cu.level() > 0.05f && st.note >= 0) { cu.legato (hz, h.art, h.vel, rnd); st.note = h.note; return; }
        // nuova pizzicata: la vibrazione vecchia sfuma mentre parte la nuova (niente click)
        cu.fade(); st.cur ^= 1; auto& nu = st.u[(size_t) st.cur];
        nu.bend = st.bendS; nu.pluck (hz, h.vel, body, tone, h.mute, h.pm, rnd);
        st.note = h.note;
    }
    void off (int s, int note) { auto& st = str[(size_t) juce::jlimit (0, kStr - 1, s)]; if (st.note == note) { st.u[(size_t) st.cur].release(); st.note = -1; } }
    void choke (int s) { auto& st = str[(size_t) s]; for (auto& u : st.u) u.fade(); st.note = -1; }
    void bend (int s, float semis) { str[(size_t) juce::jlimit (0, kStr - 1, s)].bendT = semis; }
    void bendAll (float semis) { for (auto& st : str) st.bendT = semis; }
    void allOff() { for (int s = 0; s < kStr; ++s) choke (s); }
    // corda per una nota che arriva da fuori (tastiera, clip MIDI): la piu' vicina alla posizione, libera se possibile
    int pickString (int note, const std::array<int, kStr>& tun, int pos) const
    {
        int best = -1; float bc = 1e9f;
        for (int s = 0; s < kStr; ++s)
        {
            const int f = note - tun[(size_t) s]; if (f < 0 || f > 22) continue;
            float c = std::abs ((float) f - (float) pos - 1.5f) + (str[(size_t) s].note >= 0 ? 30.0f : 0.0f);
            if (c < bc) { bc = c; best = s; }
        }
        return best < 0 ? (note < tun[0] ? 0 : kStr - 1) : best;
    }
    bool stringBusy (int s) const { return str[(size_t) s].note >= 0; }

    void render (float* L, float* R, int n)
    {
        // cambio di chitarra: si abbassa in 5 ms, si cambia il corpo, si risale (niente salto nei filtri)
        const int bodySave = body; body = bodyCur;
        std::fill (vl.begin(), vl.begin() + n, 0.0f); std::fill (vr.begin(), vr.begin() + n, 0.0f);
        for (int s = 0; s < kStr; ++s)
        {
            auto& st = str[(size_t) s];
            if (! st.u[0].on && ! st.u[1].on) { st.bendS = st.bendT; continue; }
            const float pan = -0.14f + 0.056f * (float) s, gl = std::sqrt (0.5f - 0.5f * pan), gr = std::sqrt (0.5f + 0.5f * pan);
            for (int i = 0; i < n; ++i)
            {
                if (((ctr + i) & 15) == 0) { st.bendS += 0.25f * (st.bendT - st.bendS); for (auto& u : st.u) if (u.on) u.setBend (st.bendS); }
                float x = 0; for (auto& u : st.u) if (u.on) x += u.tick();
                vl[(size_t) i] += x * gl; vr[(size_t) i] += x * gr;
            }
        }
        const juce::uint32 c0 = ctr; ctr += (juce::uint32) n;
        // corpo / pickup (i filtri si aggiornano ogni 32 campioni contati dall'inizio: uguale con qualsiasi buffer)
        for (int i = 0; i < n; ++i)
        {
            if (((c0 + (juce::uint32) i) & 31u) == 0) filtersFor (false);
            float a = vl[(size_t) i], b = vr[(size_t) i];
            for (int k = 0; k < 4; ++k) { a = bq[0][k].p (a); b = bq[1][k].p (b); }
            if (body == CRUNCH)
            {   // distorsione morbida prima del cono: ingresso gia' scurito dal pickup, poi il cono taglia sopra 4.5k
                drvS += 0.001f * (drive - drvS);
                const float k = 1.5f + 9.0f * drvS, nrm = 1.0f / std::tanh (k * 0.5f);
                a = std::tanh (a * k) * nrm * 0.5f; b = std::tanh (b * k) * nrm * 0.5f;
                for (int q = 4; q < 6; ++q) { a = bq[0][q].p (a); b = bq[1][q].p (b); }
            }
            bFade += ((bodySave != bodyCur ? 0.0f : 1.0f) - bFade) * 0.01f;
            if (bodySave != bodyCur && bFade < 0.002f)
            {   // il cambio avviene al campione, non al blocco: stesso suono con qualsiasi buffer
                bodyCur = body = bodySave; for (auto& bb : bq) for (auto& f : bb) f.reset(); fBody = -1; filtersFor (true);
            }
            vl[(size_t) i] = a * bFade; vr[(size_t) i] = b * bFade;
        }
        // raddoppio (una seconda chitarra appena in ritardo dall'altra parte) e spazio
        for (int i = 0; i < n; ++i)
        {
            const float m = 0.5f * (vl[(size_t) i] + vr[(size_t) i]);
            dbl[(size_t) dw] = m;
            lfo += (float) (0.37 / sr); if (lfo > 1) lfo -= 1;
            const float dly = (float) (sr * (0.016 + 0.0012 * std::sin (juce::MathConstants<double>::twoPi * lfo)));
            float rp = (float) dw - dly; while (rp < 0) rp += (float) dbl.size();
            const int i0 = (int) rp; const float fr = rp - (float) i0; const int i1 = (i0 + 1) % (int) dbl.size();
            const float dv = dbl[(size_t) i0] + fr * (dbl[(size_t) i1] - dbl[(size_t) i0]);
            dw = (dw + 1) % (int) dbl.size();
            wS += 0.001f * (width - wS);
            vl[(size_t) i] = vl[(size_t) i] * (1.0f - 0.35f * wS) - dv * 0.25f * wS;
            vr[(size_t) i] = vr[(size_t) i] * (1.0f - 0.35f * wS) + dv * 0.75f * wS;
        }
        pfx::Verb::Params vp; vp.type = bodySave == NYLON || bodySave == ACUSTICA ? pfx::Verb::ROOM : pfx::Verb::PLATE; vp.decay = bodySave == NYLON || bodySave == ACUSTICA ? 1.1f : 1.6f; vp.preMs = 8; vp.color = 0.45f; vp.width = 1.0f; vp.lowHz = 150; vp.highHz = 7000;
        verb.process (vl.data(), vr.data(), wl.data(), wr.data(), n, vp);
        for (int i = 0; i < n; ++i)
        {
            spS += 0.001f * (space - spS); volS += 0.001f * (vol - volS);
            const float wet = spS * 0.55f, dry = 1.0f - spS * 0.25f;
            float a = (vl[(size_t) i] * dry + wl[(size_t) i] * wet) * volS * 0.72f, b = (vr[(size_t) i] * dry + wr[(size_t) i] * wet) * volS * 0.72f;
            if (! std::isfinite (a) || ! std::isfinite (b)) { a = b = 0; bad = true; }
            // tetto morbido: oltre -1 dBFS si arrotonda
            auto sc = [] (float x) { const float t = 0.89f; const float ax = std::abs (x); return ax <= t ? x : std::copysign (t + (1.0f - t) * std::tanh ((ax - t) / (1.0f - t)), x); };
            L[i] += sc (a); R[i] += sc (b);
        }
        if (bad) { reset(); bad = false; }
        body = bodySave;
    }
    bool active() const { for (auto& st : str) for (auto& u : st.u) if (u.on) return true; return false; }
private:
    struct Str { std::array<KS, 2> u; int cur = 0, note = -1; float bendS = 0, bendT = 0; };
    std::array<Str, kStr> str; juce::Random rnd { 1234 };
    jkit::Bq bq[2][6]; int fBody = -1; float fTone = -1, fAmt = -1;
    pfx::Verb verb; std::vector<float> vl, vr, wl, wr, dbl; int dw = 0; juce::uint32 ctr = 0; int bodyCur = 1; float bFade = 1.0f; float lfo = 0, wS = 0.3f, spS = 0.2f, volS = 0.8f, drvS = 0.5f; bool bad = false;
    double sr = 48000;
    void filtersFor (bool force)
    {
        if (! force && fBody == body && std::abs (fTone - tone) < 0.004f && std::abs (fAmt - bodyAmt) < 0.004f) return;
        // passi piccoli (al massimo 0.004 ogni 32 campioni): nessun click quando si gira TONO o CORPO
        fTone = fTone < 0 ? tone : fTone + juce::jlimit (-0.004f, 0.004f, tone - fTone); fAmt = fAmt < 0 ? bodyAmt : fAmt + juce::jlimit (-0.004f, 0.004f, bodyAmt - fAmt); fBody = body;
        const double A = fAmt, T = fTone;
        for (int c = 0; c < 2; ++c)
        {
            auto& f = bq[c];
            switch (body)
            {
                case NYLON:    f[0].peak (sr, 98, 1.6, 5.0 * A); f[1].peak (sr, 205, 1.4, 3.0 * A); f[2].peak (sr, 2200, 0.8, -2.5 * A); f[3].lp (sr, 5000 + 6000 * T, 0.6); break;
                case ACUSTICA: f[0].peak (sr, 102, 1.4, 4.0 * A); f[1].peak (sr, 230, 1.2, 2.5 * A); f[2].peak (sr, 3400, 0.9, 2.5 * A * T); f[3].lp (sr, 8000 + 8000 * T, 0.6); break;
                case PULITA:   f[0].hp (sr, 70, 0.7); f[1].peak (sr, 3000 + 1200 * T, 1.3, 3.0 + 2.0 * A); f[2].peak (sr, 400, 0.8, -1.5 * A); f[3].lp (sr, 5500 + 3500 * T, 0.7); break;
                default:       f[0].hp (sr, 90, 0.7); f[1].peak (sr, 2600 + 900 * T, 1.0, 2.0 + 2.0 * A); f[2].lp (sr, 6500, 0.7); f[3].peak (sr, 750, 0.9, 2.5 * A);
                               f[4].lp (sr, 3800 + 1500 * T, 0.8); f[5].lp (sr, 5200, 0.6); break;
            }
            if (body != CRUNCH) { f[4].flat(); f[5].flat(); }
        }
    }
};
}
