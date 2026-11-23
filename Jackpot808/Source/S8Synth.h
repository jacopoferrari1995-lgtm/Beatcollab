#pragma once
// JACKPOT 808 SYNTH: motore delle voci. Sinusoide con TIMBRO (verso l'onda quadra), REESE (due denti di sega
// stonati, filtrati sulla nota), SUB un'ottava sotto, CLICK d'attacco, DIST asimmetrica; inviluppo di pitch,
// GLIDE (legato o sempre) con pila delle note, inviluppo d'ampiezza 808 (attacco, decadimento, sustain, rilascio),
// e un SAMPLE caricato dall'utente suonato accordato (radice trovata da sola). Tutto generato a 4x.
#include "S8DSP.h"

namespace s8
{
struct SampleData : juce::ReferenceCountedObject
{
    using Ptr = juce::ReferenceCountedObjectPtr<SampleData>;
    juce::AudioBuffer<float> buf; double fileSr = 44100; float root = 36; juce::String name; juce::MemoryBlock wav;
    static float findRoot (const juce::AudioBuffer<float>& b, double fsr)
    {   // YIN sul tratto dopo l'attacco: la nota dell'808
        Tuner t; t.prepare (fsr); std::vector<float> hz;
        const int skip = (int) (0.03 * fsr), n = juce::jmin (b.getNumSamples(), (int) (1.5 * fsr));
        for (int i = 0; i < n; ++i)
        {
            float x = 0; for (int c = 0; c < b.getNumChannels(); ++c) x += b.getSample (c, i);
            if (t.push (x) && i > skip && t.conf > 0.75f && t.hz > 22 && t.hz < 400) hz.push_back (t.hz);
        }
        if (hz.empty()) return 36;
        std::sort (hz.begin(), hz.end()); const float f = hz[hz.size() / 2];
        return (float) (69.0 + 12.0 * std::log2 (f / 440.0));
    }
};

struct SynthSet
{
    float tb = 0, reese = 0, sub = 0, click = 0, dist = 0, pAmt = 0, pTime = 60, glide = 0; int gMode = 1;
    float tune = 0, fine = 0, att = 1, dec = 2.5f, sus = 0, rel = 120, vel = 0.5f; bool poly = false; float bend = 2, sMix = 0, sStart = 0, lvl = 0;
};

class Synth
{
public:
    static constexpr int kVoices = 6, OS = 4;
    void prepare (double s, int block)
    {
        sr = s; osr = sr * OS; maxB = juce::jmax (block, 4096);
        os = std::make_unique<juce::dsp::Oversampling<float>> (2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false);
        os->initProcessing ((size_t) maxB);
        for (auto& v : V) v = Voice();
        nHeld = 0; bendSt = 0; modW = 0; lastPitch = -1; init = false;
        k20 = 1.0f - std::exp (-1.0f / (0.020f * (float) osr));
    }
    int latency() const { return os ? juce::roundToInt (os->getLatencyInSamples()) : 0; }
    void setSample (SampleData::Ptr p) { const juce::SpinLock::ScopedLockType sl (sLock); pending = p; hasPending = true; }
    SampleData::Ptr sampleForUi() const { return uiSample; }
    std::atomic<int> uiNote { -1 }; std::atomic<float> uiEnv { 0 }, uiPitch { 0 };
    std::atomic<int> uiVoices { 0 };
    // note suonate dall'editor (tastiera a schermo)
    std::atomic<int> uiOn { -1 }, uiOff { -1 };

    void render (float* L, float* R, int N, juce::MidiBuffer& midi, const SynthSet& S)
    {
        {   // il sample nuovo entra quando il blocco e' libero (mai allocazioni qui)
            const juce::SpinLock::ScopedTryLockType tl (sLock);
            if (tl.isLocked() && hasPending) { std::swap (cur, pending); hasPending = false; uiSample = cur; for (auto& v : V) v.spos = -1; }
        }
        if (! init) { init = true; sm = S; }
        std::fill (L, L + N, 0.0f); std::fill (R, R + N, 0.0f);
        float* chans[2] = { L, R }; juce::dsp::AudioBlock<float> blk (chans, 2, (size_t) N);
        auto ob = os->processSamplesUp (blk);
        float* o[2] = { ob.getChannelPointer (0), ob.getChannelPointer (1) };
        if (const int n = uiOn.exchange (-1); n >= 0) noteOn (n, 0.8f, S);
        if (const int n = uiOff.exchange (-1); n >= 0) noteOff (n, S);
        auto it = midi.cbegin();
        int act = 0;
        for (int i = 0; i < N; ++i)
        {
            for (; it != midi.cend() && (*it).samplePosition <= i; ++it)
            {
                const auto m = (*it).getMessage();
                if (m.isNoteOn()) noteOn (m.getNoteNumber(), m.getFloatVelocity(), S);
                else if (m.isNoteOff()) noteOff (m.getNoteNumber(), S);
                else if (m.isPitchWheel()) bendSt = (float) ((m.getPitchWheelValue() - 8192) / 8192.0);
                else if (m.isController() && m.getControllerNumber() == 1) modW = m.getControllerValue() / 127.0f;
                else if (m.isAllNotesOff() || m.isAllSoundOff()) { for (auto& v : V) v.held = false; nHeld = 0; }
            }
            for (int k = 0; k < OS; ++k)
            {
                smooth (S);
                float l = 0, r = 0;
                for (auto& v : V) if (v.active) voice (v, l, r);
                o[0][i * OS + k] = l; o[1][i * OS + k] = r;
            }
        }
        for (auto& v : V) if (v.active) ++act;
        os->processSamplesDown (blk);
        uiVoices = act;
        float e = 0; int nt = -1; float pt = 0; for (auto& v : V) if (v.active && v.amp > e) { e = v.amp; nt = v.note; pt = v.pitch; }
        uiEnv = e; uiNote = nt; uiPitch = pt;
    }

private:
    struct Voice
    {
        bool active = false, held = false, rel = false; int note = 0; float vel = 1;
        double ph = 0, phS = 0, pr1 = 0, pr2 = 0; float pitch = 36, target = 36, t = 0, amp = 0, aLev = 0, relFrom = 0, clk = 0, lp1[2] {}, lp2[2] {}, hp = 0;
        double spos = -1; float relT = 0; juce::uint32 age = 0;
    };
    Voice V[kVoices]; juce::uint32 ageC = 0;
    int held[32] {}; int nHeld = 0; float lastPitch = -1;
    float bendSt = 0, modW = 0;

    void smooth (const SynthSet& S)
    {
        auto f = [this] (float& a, float b) { a += (b - a) * k20; };
        f (sm.tb, S.tb); f (sm.reese, S.reese); f (sm.sub, S.sub); f (sm.click, S.click); f (sm.dist, S.dist); f (sm.tune, S.tune); f (sm.fine, S.fine);
        f (sm.sus, S.sus); f (sm.sMix, S.sMix); f (sm.lvl, S.lvl); f (sm.vel, S.vel);
        bendS += (bendSt - bendS) * k20; modS += (modW - modS) * k20;
        sm.pAmt = S.pAmt; sm.pTime = S.pTime; sm.glide = S.glide; sm.gMode = S.gMode; sm.att = S.att; sm.dec = S.dec; sm.rel = S.rel; sm.poly = S.poly; sm.bend = S.bend; sm.sStart = S.sStart;
    }
    float bendS = 0, modS = 0;

    void startVoice (Voice& v, int note, float vel, float fromPitch)
    {
        if (! v.active) { v.ph = 0; v.phS = 0; v.pr1 = 0.25; v.pr2 = 0.75; v.aLev = 0; }   // da ferma parte dallo zero della sinusoide; se suonava la fase continua: niente click
        v.active = true; v.held = true; v.rel = false; v.note = note; v.vel = vel; v.t = 0; v.clk = 1; v.age = ++ageC;
        v.target = (float) note; v.pitch = fromPitch >= 0 ? fromPitch : (float) note;
        v.spos = cur != nullptr ? sm.sStart * cur->buf.getNumSamples() : -1;
        v.relFrom = v.aLev;   // se la voce suonava, il nuovo attacco parte da dove era
    }
    void noteOn (int note, float vel, const SynthSet& S)
    {
        if (nHeld < 32) { for (int k = 0; k < nHeld; ++k) if (held[k] == note) { for (int j = k; j < nHeld - 1; ++j) held[j] = held[j + 1]; --nHeld; break; } held[nHeld++] = note; }
        if (! S.poly)
        {
            auto& v = V[0];
            for (int k = 1; k < kVoices; ++k) if (V[k].active && ! V[k].rel) { V[k].rel = true; V[k].relFrom = V[k].aLev; V[k].relT = 0; }
            const bool legato = v.active && v.held && ! v.rel;
            if (S.glide > 0.5f && S.gMode == 1 && legato) { v.target = (float) note; v.note = note; return; }   // LEGATO: scivola senza riattaccare
            const float from = (S.glide > 0.5f && S.gMode == 2 && lastPitch >= 0) ? (v.active ? v.pitch : lastPitch) : -1.0f;
            startVoice (v, note, vel, from); lastPitch = (float) note; return;
        }
        Voice* best = nullptr;
        for (auto& v : V) if (! v.active) { best = &v; break; }
        if (best == nullptr) { for (auto& v : V) if (v.rel && (best == nullptr || v.aLev < best->aLev)) best = &v; }
        if (best == nullptr) { for (auto& v : V) if (best == nullptr || v.age < best->age) best = &v; }
        startVoice (*best, note, vel, -1); lastPitch = (float) note;
    }
    void noteOff (int note, const SynthSet& S)
    {
        for (int k = 0; k < nHeld; ++k) if (held[k] == note) { for (int j = k; j < nHeld - 1; ++j) held[j] = held[j + 1]; --nHeld; break; }
        if (! S.poly)
        {
            auto& v = V[0];
            if (v.active && v.note == note)
            {
                if (nHeld > 0 && S.glide > 0.5f) { v.target = (float) held[nHeld - 1]; v.note = held[nHeld - 1]; return; }   // torna sulla nota ancora tenuta
                if (nHeld > 0) { startVoice (v, held[nHeld - 1], v.vel, -1); return; }
                v.held = false; v.rel = true; v.relFrom = v.aLev; v.relT = 0;
            }
            return;
        }
        for (auto& v : V) if (v.active && v.note == note && v.held) { v.held = false; v.rel = true; v.relFrom = v.aLev; v.relT = 0; }
    }

    static inline float blep (double t, double dt)
    {
        if (t < dt) { const double x = t / dt; return (float) (x + x - x * x - 1.0); }
        if (t > 1.0 - dt) { const double x = (t - 1.0) / dt; return (float) (x * x + x + x + 1.0); }
        return 0.0f;
    }
    void voice (Voice& v, float& l, float& r)
    {
        const float dt = 1.0f / (float) osr;
        // glide: tempo costante fra le note (ms per tutto il salto), curva morbida
        if (std::abs (v.target - v.pitch) > 1e-4f)
        {
            const float g = sm.glide > 0.5f ? 1.0f - std::exp (-dt / (sm.glide * 0.001f * 0.35f)) : 1.0f;
            v.pitch += (v.target - v.pitch) * g;
        }
        const float pEnv = sm.pAmt * std::exp (-v.t / (sm.pTime * 0.001f));
        const float semis = v.pitch + pEnv + sm.tune + sm.fine * 0.01f + bendS * sm.bend;
        const double f = 440.0 * std::pow (2.0, (semis - 69.0) / 12.0), inc = f / osr;
        // inviluppo d'ampiezza
        float a;
        const float A = juce::jmax (0.0005f, sm.att * 0.001f);
        if (v.rel)
        {
            v.relT += dt; a = v.relFrom * std::exp (-v.relT * 6.9f / juce::jmax (0.01f, sm.rel * 0.001f));
            if (a < 1e-4f) { v.active = false; v.aLev = 0; v.amp = 0; return; }
        }
        else
        {
            const float decay = std::exp (-juce::jmax (0.0f, v.t - A) * 6.9f / juce::jmax (0.03f, sm.dec));
            const float body = sm.sus + (1.0f - sm.sus) * decay;
            a = v.t < A ? v.relFrom + (1.0f - v.relFrom) * (v.t / A) : body;
            if (sm.sus < 0.001f && decay < 1e-4f) { v.active = false; v.aLev = 0; v.amp = 0; return; }
        }
        v.aLev = a; v.t += dt;
        const float velG = 1.0f - sm.vel + sm.vel * v.vel;
        v.amp = a * velG;
        // oscillatori
        const float s = (float) std::sin (juce::MathConstants<double>::twoPi * v.ph);
        float y = s;
        if (sm.tb > 0.001f) { const float k = 1.0f + sm.tb * 7.0f; y = std::tanh (s * k) / std::tanh (k); }
        if (sm.sub > 0.001f) y += sm.sub * 0.8f * (float) std::sin (juce::MathConstants<double>::twoPi * v.phS);
        if (sm.reese > 0.001f)
        {
            const double i1 = inc * 1.0069, i2 = inc / 1.0069;
            float s1 = (float) (2.0 * v.pr1 - 1.0) - blep (v.pr1, i1), s2 = (float) (2.0 * v.pr2 - 1.0) - blep (v.pr2, i2);
            float z = 0.5f * (s1 + s2);
            const float fc = juce::jmin (6000.0f, (float) f * (3.0f + sm.tb * 9.0f)), g = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * fc / (float) osr);
            v.lp1[0] += (z - v.lp1[0]) * g; v.lp2[0] += (v.lp1[0] - v.lp2[0]) * g;
            y = y * (1.0f - 0.6f * sm.reese) + v.lp2[0] * sm.reese * 1.1f;
            v.pr1 += i1; if (v.pr1 >= 1) v.pr1 -= 1; v.pr2 += i2; if (v.pr2 >= 1) v.pr2 -= 1;
        }
        // sample accordato
        float smpL = 0, smpR = 0; bool hasS = false;
        if (cur != nullptr && v.spos >= 0)
        {
            const auto& b = cur->buf; const int n = b.getNumSamples();
            if (v.spos < n - 3)
            {
                const int i0 = (int) v.spos; const float fr = (float) (v.spos - i0);
                auto herm = [&] (int c) { const float* d = b.getReadPointer (juce::jmin (c, b.getNumChannels() - 1)); const float xm = d[juce::jmax (0, i0 - 1)], x0 = d[i0], x1 = d[i0 + 1], x2 = d[i0 + 2];
                    const float c1 = 0.5f * (x1 - xm), c2 = xm - 2.5f * x0 + 2.0f * x1 - 0.5f * x2, c3 = 0.5f * (x2 - xm) + 1.5f * (x0 - x1); return ((c3 * fr + c2) * fr + c1) * fr + x0; };
                smpL = herm (0); smpR = herm (1); hasS = true;
                v.spos += std::pow (2.0, (semis - cur->root) / 12.0) * cur->fileSr / osr;
            }
            else v.spos = -1;
        }
        // click d'attacco: rumore filtrato e un colpo acuto, 4 ms
        if (v.clk > 1e-4f && sm.click > 0.001f)
        {
            const float nz = rng.nextFloat() * 2.0f - 1.0f; v.hp += (nz - v.hp) * 0.35f;
            y += ((nz - v.hp) * 0.6f + 0.6f * (float) std::sin (juce::MathConstants<double>::twoPi * v.t * 1800.0)) * v.clk * sm.click * 1.4f;
        }
        v.clk *= std::exp (-dt / 0.004f);
        // distorsione asimmetrica (rotella = piu' distorsione)
        const float d = juce::jlimit (0.0f, 1.0f, sm.dist + modS * 0.6f);
        auto dist = [d] (float x) { if (d < 0.001f) return x; const float g = 1.0f + d * d * 24.0f, bias = 0.25f * d; return (std::tanh (x * g + bias) - std::tanh (bias)) / (1.0f + d * 1.2f); };
        const float mixS = hasS ? sm.sMix : 0.0f;
        float outL = dist (y * (1.0f - mixS) + smpL * mixS), outR = dist (y * (1.0f - mixS) + smpR * mixS);
        const float g = v.amp * 0.7f * juce::Decibels::decibelsToGain (sm.lvl);
        l += outL * g; r += outR * g;
        v.ph += inc; if (v.ph >= 1) v.ph -= 1; v.phS += inc * 0.5; if (v.phS >= 1) v.phS -= 1;
    }

    double sr = 48000, osr = 192000; int maxB = 4096; bool init = false; float k20 = 0.001f;
    SynthSet sm; juce::Random rng { 808 };
    std::unique_ptr<juce::dsp::Oversampling<float>> os;
    juce::SpinLock sLock; SampleData::Ptr cur, pending, uiSample; bool hasPending = false;
};
}
