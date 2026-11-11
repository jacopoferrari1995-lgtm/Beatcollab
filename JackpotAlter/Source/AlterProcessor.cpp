#include <map>
#include "AlterProcessor.h"
#include "AlterEditor.h"

namespace ja
{
// ======================================================================= PITCH
void PitchStage::prepare (double s, int)
{
    sr = s;
    const int order = sr > 60000 ? 12 : 11; N = 1 << order;
    for (auto& p : pv) p.prepare (order, 8);
    H = pv[0].hop(); hopPos = 0;
    det.prepare (sr, N);
    for (auto& d : dly) d.assign ((size_t) N, 0.0f); dPos = 0;
    wet = 0; corr = 0; lastF0 = 0; dPhase = 0; dA = dB = dT = 0; unvoiced = 0;
}

void PitchStage::frame (const Settings& S)
{
    // deriva: WOW = nastro che ondeggia lento e irregolare, VIBRATO = sinusoide, CAOS = salti casuali con glissato
    const float dt = (float) H / (float) sr;
    dPhase += S.driftRate * dt;
    float drift = 0;
    if (S.drift > 0.0005f)
    {
        while (dPhase >= 1) { dPhase -= 1; dA = dB; dB = rng.nextFloat() * 2 - 1; }
        const float amt = S.drift * S.drift;
        if (S.driftType == VIBRATO) drift = std::sin (kTwoPi * dPhase) * amt * 1.5f;
        else if (S.driftType == WOW)
        {
            const float t = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::pi * dPhase);   // curva liscia tra due punti casuali
            drift = (dA + (dB - dA) * t) * amt * 2.0f + std::sin (kTwoPi * dPhase * 0.37f) * amt * 0.6f;
        }
        else
        {
            const float t = juce::jmin (1.0f, dPhase / 0.3f), e = t * t * (3 - 2 * t);           // salto con glissato al 30% del periodo
            drift = std::round ((dA + (dB - dA) * e) * 4.0f) / 4.0f * amt * 3.0f + (dB - dA) * 0.0f;
        }
    }
    driftSemi = drift;

    // correzione di intonazione per QUANTIZZA e ROBOT
    float target = 0;
    if (S.mode != TRASPONI)
    {
        const float f0 = det.detect();
        if (f0 > 55 && f0 < 1200 && det.conf > 0.75f)
        {
            unvoiced = 0; lastF0 = f0;
            const float m = midiOf (f0);
            float t = m;
            if (S.mode == QUANTIZZA) t = snapMidi (m, S.key, S.scale);
            else t = (float) (std::round ((m - S.key) / 12.0f) * 12.0f + S.key);     // ROBOT: sempre la stessa nota
            target = t - m;
            corr = S.mode == ROBOT ? target : corr + (target - corr) * 0.65f;
        }
        else if (++unvoiced > 6) corr *= 0.9f;
        target = corr;
        inHz = lastF0;
    }
    else { corr = 0; inHz = 0; }
    corrSemi = corr;
    const float semi = S.pitch + drift + corr;
    ratioNow = std::pow (2.0f, semi / 12.0f);
    fRatioNow = std::pow (2.0f, S.formant / 12.0f);
    keepNow = ! S.link;
    outHz = inHz.load() > 0 ? inHz.load() * ratioNow : 0.0f;
    const float lifter = (float) (sr * 0.0012);
    for (auto& p : pv) p.frame (ratioNow, fRatioNow, keepNow, lifter);
}

void PitchStage::process (float* L, float* R, int n, const Settings& S)
{
    const bool on = std::abs (S.pitch) > 0.004f || std::abs (S.formant) > 0.004f || S.drift > 0.0005f || S.mode != TRASPONI || (S.link && std::abs (S.pitch) > 0.004f);
    active = on;
    const float step = (float) (1.0 / (0.03 * sr));
    for (int i = 0; i < n; ++i)
    {
        const float xl = L[i], xr = R[i];
        det.push (0.5f * (xl + xr));
        const float yl = pv[0].push (xl), yr = pv[1].push (xr);
        const float cl = dly[0][(size_t) dPos], cr = dly[1][(size_t) dPos];
        dly[0][(size_t) dPos] = xl; dly[1][(size_t) dPos] = xr; dPos = (dPos + 1) % N;
        wet = on ? juce::jmin (1.0f, wet + step) : juce::jmax (0.0f, wet - step);
        const float w2 = wet * S.pmix;   // MIX del pitch: dry e voce spostata insieme (la "patina" di AlterBoy)
        L[i] = cl + (yl - cl) * w2; R[i] = cr + (yr - cr) * w2;
        if (++hopPos >= H)
        {
            hopPos = 0;
            if (on || wet > 0) frame (S);
            else { for (auto& p : pv) p.markFresh(); }   // fermo: niente calcolo, il percorso pulito basta
        }
    }
}

// ======================================================================= DRIVE multibanda
void DriveStage::prepare (double s, int maxBlock)
{
    sr = s; maxB = juce::jmax (64, juce::jmin (maxBlock, 1024));
    os = std::make_unique<juce::dsp::Oversampling<float>> (2, 2, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, true);
    os->initProcessing ((size_t) maxB);
    osLat = juce::roundToInt (os->getLatencyInSamples());
    for (auto& b : ch) for (auto& c : b) c.prepare (sr * 4, 4);
    juce::dsp::ProcessSpec sp { sr * 4, (juce::uint32) maxB * 4, 2 };
    for (int k = 0; k < 3; ++k)
    {
        xs[k].prepare (sp); xs[k].setType (juce::dsp::LinkwitzRileyFilterType::lowpass);
        ap[k].prepare (sp); ap[k].setType (juce::dsp::LinkwitzRileyFilterType::allpass);
        lastX[k] = -1;
    }
    lastNb = -1; for (auto& t : lastTone) t = 99; eIn = eOut = 1e-6f; agSm = 1; for (auto& l : lvSm) l = 1;
}

void DriveStage::process (float* L, float* R, int n, const Settings& S)
{
    const int nb = juce::jlimit (1, kBandsMax, S.bands);
    // tagli usati: con 2 bande x1, con 3 x1 e x2, con 4 tutti e tre (sempre in ordine crescente)
    float fx[3] = { S.xo[0], S.xo[1], S.xo[2] };
    for (int k = 1; k < 3; ++k) fx[k] = juce::jmax (fx[k], fx[k - 1] * 1.15f);
    if (nb != lastNb) { for (auto& x : xs) x.reset(); for (auto& a2 : ap) a2.reset(); lastNb = nb; }
    for (int k = 0; k < nb - 1; ++k) if (fx[k] != lastX[k]) { xs[k].setCutoffFrequency (juce::jlimit (20.0f, 20000.0f, fx[k])); ap[k].setCutoffFrequency (juce::jlimit (20.0f, 20000.0f, fx[k])); lastX[k] = fx[k]; }
    bool anySolo = false; for (int b = 0; b < nb; ++b) anySolo |= S.b[b].solo;
    for (int b = 0; b < nb; ++b)
    {
        for (auto& c : ch[b]) { c.setModel (S.b[b].model); c.setDrive (S.b[b].drive); }
        if (std::abs (S.b[b].tone - lastTone[b]) > 1e-3f)
        {   // tono (inclinazione attorno a 800 Hz) per banda, al rate sovracampionato
            lastTone[b] = S.b[b].tone;
            for (int c = 0; c < 2; ++c) { toneLo[b][c].shelf (sr * 4, 800, -6.0 * S.b[b].tone, false); toneHi[b][c].shelf (sr * 4, 800, 6.0 * S.b[b].tone, true); }
        }
    }
    float g[kBandsMax], lv[kBandsMax]; bool act[kBandsMax];
    for (int b = 0; b < nb; ++b)
    {
        g[b] = juce::Decibels::decibelsToGain (S.b[b].drive * modelInfo (S.b[b].model).maxDb);
        lv[b] = (anySolo && ! S.b[b].solo) ? 0.0f : juce::Decibels::decibelsToGain (S.b[b].lvlDb);
        act[b] = ! S.b[b].byp && S.b[b].drive > 0.0005f;
    }
    float bandPk[kBandsMax] {};
    for (int off = 0; off < n; off += maxB)
    {
        const int m = juce::jmin (maxB, n - off);
        float* l = L + off; float* r = R + off;
        double ei = 0; for (int i = 0; i < m; ++i) ei += l[i] * l[i] + r[i] * r[i];
        float* chans[2] = { l, r };
        juce::dsp::AudioBlock<float> blk (chans, 2, (size_t) m);
        auto up = os->processSamplesUp (blk);
        const int M = (int) up.getNumSamples();
        for (int c = 0; c < 2; ++c)
        {
            float* u = up.getChannelPointer ((size_t) c);
            for (int i = 0; i < M; ++i)
            {
                float band[kBandsMax]; float rest = u[i];
                for (int k = 0; k < nb - 1; ++k)
                {
                    float lo, hi; xs[k].processSample (c, rest, lo, hi);
                    for (int j = 0; j < k; ++j) band[j] = ap[k].processSample (c, band[j]);   // fase allineata: somma piatta
                    band[k] = lo; rest = hi;
                }
                band[nb - 1] = rest;
                float y = 0;
                for (int b = 0; b < nb; ++b)
                {
                    const float x = band[b]; float v = x;
                    if (act[b])
                    {
                        v = ch[b][c].process (x, g[b], S.b[b].drive);
                        if (std::abs (S.b[b].tone) > 1e-3f) v = toneHi[b][c] (toneLo[b][c] (v));
                        v = x + (v - x) * S.b[b].mix;
                    }
                    v *= lv[b];
                    bandPk[b] = juce::jmax (bandPk[b], std::abs (v));
                    y += v;
                }
                u[i] = y;
            }
        }
        os->processSamplesDown (blk);
        // guadagno automatico: stessa energia prima e dopo
        double eo = 0; for (int i = 0; i < m; ++i) eo += l[i] * l[i] + r[i] * r[i];
        const float k = std::exp (-(float) m / (0.15f * (float) sr));
        eIn = k * eIn + (1 - k) * (float) (ei / m); eOut = k * eOut + (1 - k) * (float) (eo / m);
        bool anyAct = false; for (int b = 0; b < nb; ++b) anyAct |= act[b];
        float target = 1;
        if (S.autoGain && anyAct && eIn > 1e-9f) target = juce::jlimit (0.03f, 4.0f, std::sqrt (eIn / juce::jmax (1e-12f, eOut)));
        float a0 = agSm; if (target < a0 * 0.7f) a0 = target;
        autoDb = juce::Decibels::gainToDecibels (target);
        const float step = (target - a0) / (float) m;
        for (int i = 0; i < m; ++i) { const float a = a0 + step * (float) (i + 1); l[i] *= a; r[i] *= a; }
        agSm = target;
    }
    for (int b = 0; b < kBandsMax; ++b) bandLev[b] = b < nb ? juce::jmax (bandPk[b], bandLev[b].load() * 0.85f) : 0.0f;
}

// ======================================================================= PROCESSOR
juce::AudioProcessorValueTreeState::ParameterLayout Processor::layout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    StringArray models; for (int m = 0; m < kNumModels; ++m) models.add (modelInfo (m).name);
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "mode", 1 }, "Mode", StringArray { "TRASPONI", "QUANTIZZA", "ROBOT" }, 0));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "pitch", 1 }, "Pitch", NormalisableRange<float> (-12, 12, 0.01f), 0, AudioParameterFloatAttributes().withLabel ("st")));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "formant", 1 }, "Formant", NormalisableRange<float> (-12, 12, 0.01f), 0, AudioParameterFloatAttributes().withLabel ("st")));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "link", 1 }, "Formant Link", false));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "key", 1 }, "Key", noteNames(), 9));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "scale", 1 }, "Scale", StringArray { "CROMATICA", "MAGGIORE", "MINORE", "ARMONICA", "PENTATONICA", "FRIGIA" }, 2));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "drift", 1 }, "Drift", NormalisableRange<float> (0, 1), 0));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "driftRate", 1 }, "Drift Rate", NormalisableRange<float> (0.05f, 12.0f, 0, 0.35f), 0.8f, AudioParameterFloatAttributes().withLabel ("Hz")));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "driftType", 1 }, "Drift Type", StringArray { "WOW", "VIBRATO", "CAOS" }, 0));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "pmix", 1 }, "Pitch Mix", NormalisableRange<float> (0, 1), 1));
    // multibanda: fino a 4 bande, ognuna con modello, drive, tono, mix, livello
    l.add (std::make_unique<AudioParameterInt> (ParameterID { "bands", 1 }, "Bande", 1, kBandsMax, 1));
    const float xd[3] = { 150, 1000, 5000 };
    for (int k = 0; k < 3; ++k) l.add (std::make_unique<AudioParameterFloat> (ParameterID { "x" + String (k + 1), 1 }, "Taglio " + String (k + 1), NormalisableRange<float> (30, 16000, 0, 0.3f), xd[k], AudioParameterFloatAttributes().withLabel ("Hz")));
    for (int b = 1; b <= kBandsMax; ++b)
    {
        const String p = "b" + String (b) + "_", n = "Banda " + String (b) + " ";
        l.add (std::make_unique<AudioParameterChoice> (ParameterID { p + "model", 1 }, n + "Model", models, 0));
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { p + "drive", 1 }, n + "Drive", NormalisableRange<float> (0, 1), 0));
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { p + "tone", 1 }, n + "Tone", NormalisableRange<float> (-1, 1), 0));
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { p + "mix", 1 }, n + "Mix", NormalisableRange<float> (0, 1), 1));
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { p + "lvl", 1 }, n + "Level", NormalisableRange<float> (-12, 12), 0));
        l.add (std::make_unique<AudioParameterBool> (ParameterID { p + "solo", 1 }, n + "Solo", false));
        l.add (std::make_unique<AudioParameterBool> (ParameterID { p + "byp", 1 }, n + "Bypass", false));
    }
    // echo
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "eOn", 1 }, "Echo", false));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "eSync", 1 }, "Echo Sync", true));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "eDiv", 1 }, "Echo Tempo", echoDivs(), 4));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "eMs", 1 }, "Echo ms", NormalisableRange<float> (10, 2000, 0, 0.4f), 300));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "eFb", 1 }, "Echo Feedback", NormalisableRange<float> (0, 0.95f), 0.35f));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "eMix", 1 }, "Echo Mix", NormalisableRange<float> (0, 1), 0.25f));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "eLo", 1 }, "Echo Low Cut", NormalisableRange<float> (20, 2000, 0, 0.4f), 150));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "eHi", 1 }, "Echo High Cut", NormalisableRange<float> (1000, 20000, 0, 0.4f), 6000));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "eSat", 1 }, "Echo Saturazione", NormalisableRange<float> (0, 1), 0.3f));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "ePing", 1 }, "Echo Ping-Pong", false));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "order", 1 }, "Order", StringArray { "PITCH > DRIVE", "DRIVE > PITCH" }, 0));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "auto", 1 }, "Auto Gain", true));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "mix", 1 }, "Mix", NormalisableRange<float> (0, 1), 1));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "out", 1 }, "Output", NormalisableRange<float> (-24, 12), 0, AudioParameterFloatAttributes().withLabel ("dB")));
    return l;
}

Processor::Processor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true).withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "JAD", layout())
{}

bool Processor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto out = l.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono()) && l.getMainInputChannelSet() == out;
}

Settings Processor::settings() const
{
    Settings S;
    S.mode = (int) value ("mode"); S.pitch = value ("pitch"); S.formant = value ("formant"); S.link = value ("link") > 0.5f;
    S.key = (int) value ("key"); S.scale = (int) value ("scale");
    S.drift = value ("drift"); S.driftRate = value ("driftRate"); S.driftType = (int) value ("driftType");
    S.pmix = value ("pmix"); S.bands = (int) value ("bands");
    for (int k = 0; k < 3; ++k) S.xo[k] = value (("x" + juce::String (k + 1)).toRawUTF8());
    for (int b = 0; b < kBandsMax; ++b)
    {
        const juce::String p = "b" + juce::String (b + 1) + "_"; auto& B = S.b[b];
        B.model = (int) value ((p + "model").toRawUTF8()); B.drive = value ((p + "drive").toRawUTF8()); B.tone = value ((p + "tone").toRawUTF8());
        B.mix = value ((p + "mix").toRawUTF8()); B.lvlDb = value ((p + "lvl").toRawUTF8()); B.solo = value ((p + "solo").toRawUTF8()) > 0.5f; B.byp = value ((p + "byp").toRawUTF8()) > 0.5f;
    }
    S.eOn = value ("eOn") > 0.5f; S.eSync = value ("eSync") > 0.5f; S.eDiv = (int) value ("eDiv"); S.eMs = value ("eMs"); S.eFb = value ("eFb");
    S.eMix = value ("eMix"); S.eLo = value ("eLo"); S.eHi = value ("eHi"); S.eSat = value ("eSat"); S.ePing = value ("ePing") > 0.5f;
    S.order = (int) value ("order"); S.autoGain = value ("auto") > 0.5f; S.mix = value ("mix"); S.outDb = value ("out");
    return S;
}

void Processor::prepareToPlay (double s, int maxBlock)
{
    sr = s;
    pitch.prepare (s, maxBlock); drive.prepare (s, maxBlock); echo.prepare (s);
    totalLat = pitch.latency() + drive.latency();
    setLatencySamples (totalLat);
    dryLen = totalLat + 1;
    for (auto& d : dry) d.assign ((size_t) dryLen, 0.0f); dryPos = 0;
    dryL.assign ((size_t) juce::jmax (maxBlock, 512), 0.0f); dryR = dryL; tmpR = dryL;
}

void Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals nd;
    const int n = buffer.getNumSamples(), nch = juce::jmin (2, buffer.getNumChannels());
    if (nch == 0 || n == 0) return;
    if ((int) dryL.size() < n) { dryL.resize ((size_t) n); dryR.resize ((size_t) n); tmpR.resize ((size_t) n); }
    float* L = buffer.getWritePointer (0); float* Rw = nch > 1 ? buffer.getWritePointer (1) : tmpR.data();
    if (nch < 2) juce::FloatVectorOperations::copy (Rw, L, n);
    const auto S = settings();
    if (auto* ph = getPlayHead()) if (auto pos = ph->getPosition()) if (auto b = pos->getBpm()) bpm = *b;

    float pkI[2] = { 0, 0 };
    for (int i = 0; i < n; ++i) { pkI[0] = juce::jmax (pkI[0], std::abs (L[i])); pkI[1] = juce::jmax (pkI[1], std::abs (Rw[i])); }
    for (int off = 0; off < n; off += 4096)
    {
        const int m = juce::jmin (4096, n - off);
        for (int i = 0; i < m; ++i) mono[i] = 0.5f * (L[off + i] + Rw[off + i]);
        ringIn.push (mono, m);
    }
    // percorso pulito ritardato quanto la catena (per MIX)
    for (int i = 0; i < n; ++i)
    {
        dryL[(size_t) i] = dry[0][(size_t) dryPos]; dryR[(size_t) i] = dry[1][(size_t) dryPos];
        dry[0][(size_t) dryPos] = L[i]; dry[1][(size_t) dryPos] = Rw[i];
        dryPos = (dryPos + 1) % dryLen;
    }
    if (S.order == 0) { pitch.process (L, Rw, n, S); drive.process (L, Rw, n, S); }
    else { drive.process (L, Rw, n, S); pitch.process (L, Rw, n, S); }

    bool solo = false; for (int b = 0; b < S.bands; ++b) solo |= S.b[b].solo;
    const float og = juce::Decibels::decibelsToGain (S.outDb), mix = solo ? 1.0f : S.mix;
    for (int i = 0; i < n; ++i) { L[i] = dryL[(size_t) i] * (1 - mix) + L[i] * mix; Rw[i] = dryR[(size_t) i] * (1 - mix) + Rw[i] * mix; }
    echo.process (L, Rw, n, S, bpm.load());
    float pkO[2] = { 0, 0 };
    for (int i = 0; i < n; ++i)
    {
        L[i] *= og; Rw[i] *= og;
        pkO[0] = juce::jmax (pkO[0], std::abs (L[i])); pkO[1] = juce::jmax (pkO[1], std::abs (Rw[i]));
    }
    for (int off = 0; off < n; off += 4096)
    {
        const int m = juce::jmin (4096, n - off);
        for (int i = 0; i < m; ++i) mono[i] = 0.5f * (L[off + i] + Rw[off + i]);
        ringOut.push (mono, m);
    }
    for (int c = 0; c < 2; ++c) { inPk[c] = juce::jmax (pkI[c], inPk[c].load() * 0.9f); outPk[c] = juce::jmax (pkO[c], outPk[c].load() * 0.9f); }
    for (int c = 2; c < buffer.getNumChannels(); ++c) buffer.clear (c, 0, n);
}

// ======================================================================= preset
const std::vector<Preset>& presets()
{
    static const std::vector<Preset> P = {
        { "Init", "Tutto a zero: TAPE leggero, pitch fermo.", {} },
        { "Voce | Demone", "Un'ottava sotto con formanti un po' giu: voce enorme e cupa. Satura col trasformatore per farla stare davanti.",
          { { "pitch", -12 }, { "formant", -3 }, { "model", TRASFORMATORE }, { "drive", 0.35f } } },
        { "Voce | Scura e sporca", "Due semitoni sotto, formanti ferme: la voce resta tua ma piu profonda. Valvola per il graffio.",
          { { "pitch", -2 }, { "formant", -1.5f }, { "model", VALVOLA }, { "drive", 0.4f } } },
        { "Voce | Chipmunk", "Pitch su con le formanti che seguono (LINK): il classico effetto voce accelerata.",
          { { "pitch", 7 }, { "link", 1 }, { "model", TAPE }, { "drive", 0.15f } } },
        { "Voce | Formante giovane", "Stessa nota, formanti +3: la voce sembra piu giovane e piccola.", { { "formant", 3 }, { "drive", 0.1f } } },
        { "Voce | Gigante", "Stessa nota, formanti -5: un gigante che canta la tua melodia.", { { "formant", -5 }, { "model", TRASFORMATORE }, { "drive", 0.3f } } },
        { "Voce | Hard tune La minore", "QUANTIZZA veloce sulla scala di La minore: intonazione a scatti stile trap. Cambia KEY e SCALA col tuo pezzo.",
          { { "mode", QUANTIZZA }, { "key", 9 }, { "scale", 2 }, { "model", CONSOLE }, { "drive", 0.2f } } },
        { "Voce | Robot", "ROBOT: ogni sillaba sulla stessa nota (KEY). Cambia KEY per scegliere la nota del robot.",
          { { "mode", ROBOT }, { "key", 9 }, { "model", CONSOLE }, { "drive", 0.25f } } },
        { "Voce | Adlib in alto", "Un'ottava sopra con formanti ferme + radiolina: adlib che stanno sopra la voce principale.",
          { { "pitch", 12 }, { "formant", 1 }, { "model", RADIO }, { "drive", 0.35f }, { "mix", 0.85f } } },
        { "Voce | Telefono", "Banda stretta e saturazione da altoparlante piccolo: intro, bridge, adlib.", { { "model", RADIO }, { "drive", 0.45f } } },
        { "Voce | Megafono rock", "Testata + cassa sulla voce: aggressiva e lo-fi.", { { "model", AMP }, { "drive", 0.45f }, { "tone", 0.2f } } },
        { "Voce | Nastro che ondeggia", "WOW leggero e saturazione da nastro: voce vintage, un po' stonata come una cassetta.",
          { { "model", TAPE }, { "drive", 0.35f }, { "drift", 0.3f }, { "driftRate", 0.6f }, { "driftType", WOW } } },
        { "Voce | Vibrato profondo", "Vibrato largo e lento: effetto lirico/psichedelico.", { { "drift", 0.55f }, { "driftRate", 5.0f }, { "driftType", VIBRATO }, { "drive", 0.1f } } },
        { "Voce | Saturata solo sulla presenza", "FOCUS 1.5-6 kHz: la saturazione tocca solo la zona della presenza, corpo e aria restano puliti.",
          { { "model", VALVOLA }, { "drive", 0.5f }, { "focus", 1 }, { "fLo", 1500 }, { "fHi", 6000 } } },
        { "Basso | 808 grosso", "Trasformatore: il ferro gonfia le basse, l'808 cresce senza diventare fangoso.", { { "model", TRASFORMATORE }, { "drive", 0.5f } } },
        { "Basso | 808 clipper trap", "Clipper duro: piu volume e un filo di grinta, come un clipper sul canale dell'808.", { { "model", CLIPPER }, { "drive", 0.45f } } },
        { "Basso | 808 che canta (fold)", "Wavefolder: armoniche alte che fanno sentire l'808 anche sui telefoni.", { { "model", FOLD }, { "drive", 0.35f }, { "mix", 0.7f } } },
        { "Basso | Distorto sopra 200 Hz", "FOCUS 200 Hz - 20 kHz: il sub resta pulito e mono, si distorce solo la parte che si sente.",
          { { "model", DISTORSIONE }, { "drive", 0.45f }, { "focus", 1 }, { "fLo", 200 }, { "fHi", 20000 } } },
        { "Basso | Valvola calda", "Triodo in classe A: armoniche pari morbide, il basso diventa piu rotondo.", { { "model", VALVOLA }, { "drive", 0.4f } } },
        { "Basso | Ottava sotto", "Pitch -12 su basso o synth: sub ottava per riempire.", { { "pitch", -12 }, { "model", TRASFORMATORE }, { "drive", 0.2f } } },
        { "Chitarra | Crunch pedale verde", "Overdrive con le medie in avanti: crunch classico.", { { "model", OVERDRIVE }, { "drive", 0.5f } } },
        { "Chitarra | Rock op-amp", "Distorsione con diodi a massa: rock compresso.", { { "model", DISTORSIONE }, { "drive", 0.6f }, { "tone", 0.15f } } },
        { "Chitarra | Fuzz anni 60", "Fuzz al germanio a manetta: muro di sustain.", { { "model", FUZZ }, { "drive", 0.75f } } },
        { "Chitarra | Ottava fuzz", "Fuzz con raddrizzatore: l'ottava sopra per gli assoli.", { { "model", OTTAVA }, { "drive", 0.6f } } },
        { "Chitarra | Amp britannico", "Testata in crunch con cassa 4x12: chitarra in diretta che sembra microfonata.", { { "model", AMP }, { "drive", 0.55f } } },
        { "Chitarra | Cassetta che ondeggia", "Nastro + WOW: chitarra lo-fi da beat.", { { "model", TAPE }, { "drive", 0.3f }, { "drift", 0.35f }, { "driftRate", 0.45f }, { "driftType", WOW } } },
        { "Batteria | Console densa", "Canale VCA spinto: batteria piu densa e presente.", { { "model", CONSOLE }, { "drive", 0.45f } } },
        { "Batteria | Campionatore lo-fi", "Bit e frequenza ridotti: batteria vintage da campionatore.", { { "model", LOFI }, { "drive", 0.45f }, { "mix", 0.8f } } },
        { "Batteria | Nastro sul bus", "Nastro morbido sul bus: arrotonda i transienti e incolla.", { { "model", TAPE }, { "drive", 0.4f } } },
        { "FX | Caos", "Pitch che salta a caso con glissati: transizioni e momenti strani.", { { "drift", 0.6f }, { "driftRate", 3.0f }, { "driftType", CAOS }, { "model", TAPE }, { "drive", 0.25f } } },
        { "FX | Radio rotta", "Radiolina + salti di intonazione: messaggi da un altro mondo.", { { "model", RADIO }, { "drive", 0.55f }, { "drift", 0.35f }, { "driftRate", 2.0f }, { "driftType", CAOS } } },
        // ---- patina col MIX del pitch (stile AlterBoy)
        { "Voce | Patina distorta (pitch 30%)", "Pitch -12 al 30% di mix sotto la voce originale + valvola: la voce resta tua con una patina grossa e sporca sotto.",
          { { "pitch", -12 }, { "pmix", 0.3f }, { "model", VALVOLA }, { "drive", 0.35f } } },
        { "Voce | Ottava sopra leggera", "Ottava sopra al 25%: brillantezza e spessore per ritornelli, senza effetto chipmunk evidente.", { { "pitch", 12 }, { "pmix", 0.25f }, { "model", TAPE }, { "drive", 0.2f } } },
        { "Voce | Doppia sporca", "Formanti -2 al 40% + nastro: sembra una seconda voce piu grande dietro.", { { "formant", -2 }, { "pmix", 0.4f }, { "model", TAPE }, { "drive", 0.3f } } },
        // ---- multibanda
        { "Multi | 808: sub pulito, sopra distorto", "2 bande a 120 Hz: sotto il sub resta pulito, sopra la distorsione fa sentire l'808 su ogni cassa.",
          { { "bands", 2 }, { "x1", 120 }, { "b2_model", DISTORSIONE }, { "b2_drive", 0.5f }, { "b2_mix", 0.8f } } },
        { "Multi | Voce: corpo valvola, presenza tape", "3 bande: corpo (sotto 300 Hz) con un filo di trasformatore, centro valvola, aria quasi pulita.",
          { { "bands", 3 }, { "x1", 300 }, { "x2", 5000 }, { "b1_model", TRASFORMATORE }, { "b1_drive", 0.2f }, { "b2_model", VALVOLA }, { "b2_drive", 0.45f }, { "b3_model", TAPE }, { "b3_drive", 0.15f } } },
        { "Multi | Batteria a 4 bande", "Sub trasformatore, medio-bassi console, medi clipper, alti nastro: densita ovunque senza fango.",
          { { "bands", 4 }, { "x1", 100 }, { "x2", 600 }, { "x3", 4000 }, { "b1_model", TRASFORMATORE }, { "b1_drive", 0.35f }, { "b2_model", CONSOLE }, { "b2_drive", 0.35f },
            { "b3_model", CLIPPER }, { "b3_drive", 0.3f }, { "b4_model", TAPE }, { "b4_drive", 0.3f } } },
        { "Multi | Basso: sotto pulito, fuzz sopra", "Fuzz solo sopra 250 Hz con mix al 60%: il basso resta fermo ma ringhia.", { { "bands", 2 }, { "x1", 250 }, { "b2_model", FUZZ }, { "b2_drive", 0.55f }, { "b2_mix", 0.6f } } },
        // ---- echo
        { "Echo | Slap rap", "Eco corto a 1/16 con poco ritorno e nastro: la voce si allarga senza riverbero.", { { "eOn", 1 }, { "eDiv", 1 }, { "eFb", 0.15f }, { "eMix", 0.2f }, { "eLo", 300 }, { "eHi", 5000 }, { "model", TAPE }, { "drive", 0.2f } } },
        { "Echo | Dub 1/4 puntato", "Ping-pong in 1/4 puntato, ritorno saturo e scuro: coda dub che si sporca a ogni ripetizione.",
          { { "eOn", 1 }, { "eDiv", 8 }, { "eFb", 0.6f }, { "eMix", 0.3f }, { "eLo", 200 }, { "eHi", 3500 }, { "eSat", 0.7f }, { "ePing", 1 } } },
        { "Echo | Adlib radio 1/8", "Radiolina + eco a 1/8 ping-pong: adlib che rimbalzano.", { { "model", RADIO }, { "drive", 0.4f }, { "eOn", 1 }, { "eDiv", 4 }, { "eFb", 0.35f }, { "eMix", 0.35f }, { "ePing", 1 } } },
        { "Echo | Nastro che si sfalda", "Eco libero 420 ms con wow sul pitch: la voce si perde nel nastro.", { { "eOn", 1 }, { "eSync", 0 }, { "eMs", 420 }, { "eFb", 0.55f }, { "eMix", 0.3f }, { "eSat", 0.6f }, { "eHi", 4000 },
          { "drift", 0.25f }, { "driftRate", 0.5f }, { "driftType", WOW }, { "model", TAPE }, { "drive", 0.3f } } } };
    return P;
}

void Processor::loadPreset (int i)
{
    if (! juce::isPositiveAndBelow (i, (int) presets().size())) return;
    preset = i;
    for (auto* p : getParameters()) if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p)) if (rp->paramID != "out") rp->setValueNotifyingHost (rp->getDefaultValue());
    // i preset usano anche i nomi brevi: model/drive/tone = banda 1; focus + fLo/fHi = 3 bande con la saturazione solo in quella centrale
    std::map<juce::String, float> v; for (auto& [id, val] : presets()[(size_t) i].v) v[id] = val;
    auto set = [this] (const juce::String& id, float val) { if (auto* rp = apvts.getParameter (id)) rp->setValueNotifyingHost (rp->convertTo0to1 (val)); };
    if (v.count ("focus") && v["focus"] > 0.5f)
    {
        const float lo = v.count ("fLo") ? v["fLo"] : 150, hi = v.count ("fHi") ? v["fHi"] : 5000;
        const bool top = hi >= 19000;
        set ("bands", top ? 2.0f : 3.0f); set ("x1", lo); if (! top) set ("x2", hi);
        for (auto k : { "model", "drive", "tone" }) if (v.count (k)) set (juce::String ("b2_") + k, v[k]);
    }
    else for (auto k : { "model", "drive", "tone" }) if (v.count (k)) set (juce::String ("b1_") + k, v[k]);
    for (auto& [id, val] : v) if (! (id == "model" || id == "drive" || id == "tone" || id == "focus" || id == "fLo" || id == "fHi")) set (id, val);
}

void Processor::getStateInformation (juce::MemoryBlock& d)
{
    auto s = apvts.copyState(); s.setProperty ("preset", preset, nullptr);
    if (auto x = s.createXml()) copyXmlToBinary (*x, d);
}
void Processor::setStateInformation (const void* data, int size)
{
    if (auto x = getXmlFromBinary (data, size)) { auto t = juce::ValueTree::fromXml (*x); if (t.isValid()) { preset = t.getProperty ("preset", 0); apvts.replaceState (t); } }
}

juce::AudioProcessorEditor* Processor::createEditor() { return new Editor (*this); }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new ja::Processor(); }
