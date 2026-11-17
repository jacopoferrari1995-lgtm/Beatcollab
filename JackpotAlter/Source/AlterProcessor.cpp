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
    wet = 0; corr = 0; lastF0 = 0; unvoiced = 0;
}

void PitchStage::frame (const Settings& S, bool monoOnly)
{
    // correzione di intonazione per QUANTIZZA e ROBOT
    if (S.mode != TRASPONI)
    {
        const float f0 = det.detect();
        if (f0 > 55 && f0 < 1200 && det.conf > 0.75f)
        {
            unvoiced = 0; lastF0 = f0;
            const float m = midiOf (f0);
            const float t = S.mode == QUANTIZZA ? snapMidi (m, S.key, S.scale) : (float) (std::round ((m - S.key) / 12.0f) * 12.0f + S.key);   // ROBOT: sempre la stessa nota
            const float target = t - m;
            corr = S.mode == ROBOT ? target : corr + (target - corr) * 0.65f;
        }
        else if (++unvoiced > 6) corr *= 0.9f;
        inHz = lastF0;
    }
    else { corr = 0; inHz = 0; }
    corrSemi = corr;
    const float ratio = std::pow (2.0f, (S.pitch + corr) / 12.0f), fRatio = std::pow (2.0f, S.formant / 12.0f);
    outHz = inHz.load() > 0 ? inHz.load() * ratio : 0.0f;
    const float lifter = (float) (sr * 0.0012);
    pv[0].frame (ratio, fRatio, ! S.link, lifter);
    if (! monoOnly) pv[1].frame (ratio, fRatio, ! S.link, lifter);
}

void PitchStage::process (float* L, float* R, int n, const Settings& S)
{
    const bool on = std::abs (S.pitch) > 0.004f || std::abs (S.formant) > 0.004f || S.mode != TRASPONI;
    active = on;
    const float step = (float) (1.0 / (0.03 * sr));
    if (wasMono && ! S.mono) pv[1].copyFrom (pv[0]);   // torna stereo: il destro riparte da dove sarebbe stato
    wasMono = S.mono;
    for (int i = 0; i < n; ++i)
    {
        const float xl = L[i], xr = R[i];
        det.push (0.5f * (xl + xr));
        const float yl = pv[0].push (xl), yr = S.mono ? yl : pv[1].push (xr);
        const float cl = dly[0][(size_t) dPos], cr = dly[1][(size_t) dPos];
        dly[0][(size_t) dPos] = xl; dly[1][(size_t) dPos] = xr; if (++dPos >= N) dPos = 0;
        wet = on ? juce::jmin (1.0f, wet + step) : juce::jmax (0.0f, wet - step);
        if (wet <= 0.0f) { L[i] = cl; R[i] = cr; }
        else { L[i] = cl + (yl - cl) * wet; R[i] = cr + (yr - cr) * wet; }
        if (++hopPos >= H)
        {
            hopPos = 0;
            if (on || wet > 0) frame (S, S.mono);
            else for (auto& p : pv) p.markFresh();   // fermo: niente calcolo, il percorso pulito basta
        }
    }
}

// ======================================================================= DRIVE
void DriveStage::prepare (double s, int maxBlock)
{
    sr = s; maxB = juce::jmax (16, maxBlock);
    os = std::make_unique<juce::dsp::Oversampling<float>> (2, 2, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, true);
    os->initProcessing ((size_t) maxB);
    osLat = juce::roundToInt (os->getLatencyInSamples());
    for (auto& b : ch) for (auto& c : b) c.prepare (sr * 4, 4);
    for (auto& c : postCh) c.prepare (sr * 4, 4);
    for (int c = 0; c < 2; ++c) { x1[c].reset(); x2[c].reset(); ap2[c].reset(); }
    lastX1 = lastX2 = -1; lastModel = -1; toneSet = 99; toneSm = 0;
    eIn = eOut = 1e-6f; agSm = 1; for (int b = 0; b < 4; ++b) { gPrev[b] = 1; dPrev[b] = 0; wPrev[b] = 0; } mbSm = 1;
    for (auto& d : bd) d.assign ((size_t) juce::jmax (1, osLat), 0.0f); bdPos = 0; xw = 0;
    tdl.assign ((size_t) maxB, 0.0f); tdr = tdl;
}

void DriveStage::process (float* l, float* r, int m, const Settings& S)
{
    if (S.model != lastModel) { for (auto& b : ch) for (auto& c : b) c.setModel (S.model); for (auto& c : postCh) c.setModel (S.model); lastModel = S.model; }
    const float f1 = juce::jlimit (30.0f, 2000.0f, S.x1), f2 = juce::jlimit (f1 * 1.5f, 16000.0f, S.x2);
    if (f1 != lastX1) { for (auto& f : x1) f.set (sr * 4, f1); lastX1 = f1; }
    if (f2 != lastX2) { for (int c = 0; c < 2; ++c) { x2[c].set (sr * 4, f2); ap2[c].set (sr * 4, f2); } lastX2 = f2; }
    const float mbT = S.multi ? 1.0f : 0.0f;
    float dN[4]; for (int b = 0; b < 3; ++b) dN[b] = juce::jlimit (0.0f, 1.0f, S.drive * S.w[b]); dN[3] = S.drive;
    const bool need = S.drive > 0.0005f;
    // percorso a riposo: ingresso ritardato quanto il sovracampionamento
    const int BL = (int) bd[0].size();
    for (int i = 0; i < m; ++i)
    {
        tdl[(size_t) i] = bd[0][(size_t) bdPos]; tdr[(size_t) i] = bd[1][(size_t) bdPos];
        bd[0][(size_t) bdPos] = l[i]; bd[1][(size_t) bdPos] = r[i]; if (++bdPos >= BL) bdPos = 0;
    }
    if (! need && xw <= 0.0f)
    {
        for (int i = 0; i < m; ++i) { l[i] = tdl[(size_t) i]; r[i] = tdr[(size_t) i]; }
        agSm = 1; autoDb = 0; for (int b = 0; b < 4; ++b) { dPrev[b] = 0; gPrev[b] = 1; wPrev[b] = 0; } mbSm = mbT;
        for (auto& v : bandLev) v = v.load() * 0.85f;
        return;
    }
    auto resetBands = [this] { for (int c = 0; c < 2; ++c) { x1[c].reset(); x2[c].reset(); ap2[c].reset(); } for (int b = 0; b < 3; ++b) for (auto& c : ch[b]) c.reset(); };
    if (xw <= 0.0f) { os->reset(); resetBands(); for (auto& c : ch[3]) c.reset(); for (auto& c : postCh) c.reset(); for (auto& t : toneLo) t.reset(); for (auto& t : toneHi) t.reset(); }
    double ei = 0; for (int i = 0; i < m; ++i) ei += l[i] * l[i] + r[i] * r[i];
    // tono (inclinazione attorno a 800 Hz), a 1x dopo il drive
    toneSm += (S.tone - toneSm) * 0.3f; if (std::abs (S.tone - toneSm) < 1e-3f) toneSm = S.tone;
    if (std::abs (toneSm - toneSet) > 1e-4f) { toneSet = toneSm; for (int c = 0; c < 2; ++c) { toneLo[c].shelf (sr, 800, -6.0 * toneSm, false); toneHi[c].shelf (sr, 800, 6.0 * toneSm, true); } }
    // rampe per il blocco: guadagno d'ingresso, quanto drive, quanto effetto (sotto il 5% il modello entra piano)
    float gN[4], wN[4]; for (int b = 0; b < 4; ++b) { gN[b] = juce::Decibels::decibelsToGain (dN[b] * modelInfo (S.model).maxDb); wN[b] = juce::jlimit (0.0f, 1.0f, dN[b] / 0.05f); }
    const float mbStep = (float) m / (float) (0.03 * sr);
    const float mbN = mbT > mbSm ? juce::jmin (mbT, mbSm + mbStep) : juce::jmax (mbT, mbSm - mbStep);
    if (mbSm <= 0.0f && mbN > 0.0f) resetBands();
    if (mbSm >= 1.0f && mbN < 1.0f) for (auto& c : ch[3]) c.reset();
    for (int b = 0; b < 4; ++b) for (auto& c : ch[b]) c.setDrive (dN[b]);
    float* chans[2] = { l, r };
    juce::dsp::AudioBlock<float> blk (chans, 2, (size_t) m);
    auto up = os->processSamplesUp (blk);
    const int M = (int) up.getNumSamples(); const float invM = 1.0f / (float) M;
    float pk[3] {};
    const int nc = S.mono ? 1 : 2;   // mono: un canale solo, poi si copia
    for (int c = 0; c < nc; ++c)
    {
        float* u = up.getChannelPointer ((size_t) c);
        auto& po = postCh[c];
        for (int i = 0; i < M; ++i)
        {
            const float t = (float) (i + 1) * invM, x = u[i], mb = mbSm + (mbN - mbSm) * t;
            // ogni banda: parte non lineare pesata (va nei filtri lineari comuni) + parte pulita
            float wetSum = 0, drySum = 0;
            auto band = [&] (int b, float in, float k)
            {
                const float w = wPrev[b] + (wN[b] - wPrev[b]) * t;
                if (w <= 0.0f) { drySum += in * k; return in; }
                const float d = dPrev[b] + (dN[b] - dPrev[b]) * t, g = gPrev[b] + (gN[b] - gPrev[b]) * t;
                const float y = ch[b][c].core (in, g, d);
                wetSum += y * w * k; drySum += in * (1.0f - w) * k; return y;
            };
            if (mb > 0.0f)
            {
                float lo, rest, mid, hi; x1[c].split (x, lo, rest); x2[c].split (rest, mid, hi); lo = ap2[c].allpass (lo);
                const float yl = band (0, lo, mb), ym = band (1, mid, mb), yh = band (2, hi, mb);
                pk[0] = juce::jmax (pk[0], std::abs (yl)); pk[1] = juce::jmax (pk[1], std::abs (ym)); pk[2] = juce::jmax (pk[2], std::abs (yh));
            }
            if (mb < 1.0f) band (3, x, 1.0f - mb);
            u[i] = po.post (wetSum) + drySum;
        }
    }
    if (S.mono)
    {   // il destro e' uguale: si copia l'uscita e lo stato, cosi' se torna stereo riparte senza salti
        std::copy (up.getChannelPointer (0), up.getChannelPointer (0) + M, up.getChannelPointer (1));
        for (auto& b : ch) b[1] = b[0]; postCh[1] = postCh[0]; x1[1] = x1[0]; x2[1] = x2[0]; ap2[1] = ap2[0];
    }
    for (int b = 0; b < 4; ++b) { gPrev[b] = gN[b]; dPrev[b] = dN[b]; wPrev[b] = wN[b]; } mbSm = mbN;
    os->processSamplesDown (blk);
    for (int c = 0; c < 2; ++c) { float* x = chans[c]; for (int i = 0; i < m; ++i) x[i] = toneHi[c] (toneLo[c] (x[i])); }
    // guadagno automatico: stessa energia prima e dopo
    double eo = 0; for (int i = 0; i < m; ++i) eo += l[i] * l[i] + r[i] * r[i];
    const float k = std::exp (-(float) m / (0.15f * (float) sr));
    eIn = k * eIn + (1 - k) * (float) (ei / m); eOut = k * eOut + (1 - k) * (float) (eo / m);
    float target = 1;
    if (S.autoGain && eIn > 1e-9f) target = juce::jlimit (0.06f, 2.0f, std::sqrt (eIn / juce::jmax (1e-12f, eOut)));
    const float a0 = agSm; autoDb = juce::Decibels::gainToDecibels (target);
    for (int i = 0; i < m; ++i) { const float a = a0 + (target - a0) * (float) (i + 1) / (float) m; l[i] *= a; r[i] *= a; }
    agSm = target;
    // dissolvenza fra percorso a riposo e percorso con il drive
    const float xs1 = 1.0f / (float) (0.02 * sr);
    for (int i = 0; i < m; ++i)
    {
        xw = need ? juce::jmin (1.0f, xw + xs1) : juce::jmax (0.0f, xw - xs1);
        if (xw < 1.0f) { l[i] = tdl[(size_t) i] + (l[i] - tdl[(size_t) i]) * xw; r[i] = tdr[(size_t) i] + (r[i] - tdr[(size_t) i]) * xw; }
    }
    for (int b = 0; b < 3; ++b) bandLev[b] = juce::jmax (pk[b] * mbSm, bandLev[b].load() * 0.85f);
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
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "model", 2 }, "Drive Model", models, 0));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "drive", 2 }, "Drive", NormalisableRange<float> (0, 1), 0));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "tone", 2 }, "Tone", NormalisableRange<float> (-1, 1), 0));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "multi", 2 }, "Multibanda", true));
    const char* wn[3] = { "wLo", "wMid", "wHi" }; const char* wl[3] = { "Drive Bassi", "Drive Medi", "Drive Alti" };
    for (int b = 0; b < 3; ++b) l.add (std::make_unique<AudioParameterFloat> (ParameterID { wn[b], 2 }, wl[b], NormalisableRange<float> (0, 2), 1));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "x1", 1 }, "Taglio Bassi", NormalisableRange<float> (30, 1000, 0, 0.4f), 200, AudioParameterFloatAttributes().withLabel ("Hz")));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "x2", 1 }, "Taglio Alti", NormalisableRange<float> (1000, 12000, 0, 0.4f), 3000, AudioParameterFloatAttributes().withLabel ("Hz")));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "auto", 1 }, "Auto Gain", true));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "mix", 1 }, "Mix", NormalisableRange<float> (0, 1), 1));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "out", 1 }, "Output", NormalisableRange<float> (-24, 12), 0, AudioParameterFloatAttributes().withLabel ("dB")));
    return l;
}

Processor::Processor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true).withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "JAD", layout())
{
    auto g = [this] (const char* id) { return apvts.getRawParameterValue (id); };
    P.mode = g ("mode"); P.pitch = g ("pitch"); P.formant = g ("formant"); P.link = g ("link"); P.key = g ("key"); P.scale = g ("scale");
    P.model = g ("model"); P.drive = g ("drive"); P.tone = g ("tone"); P.multi = g ("multi"); P.w[0] = g ("wLo"); P.w[1] = g ("wMid"); P.w[2] = g ("wHi");
    P.x1 = g ("x1"); P.x2 = g ("x2"); P.mix = g ("mix"); P.out = g ("out"); P.autoG = g ("auto");
}

bool Processor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto out = l.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono()) && l.getMainInputChannelSet() == out;
}

Settings Processor::settings() const
{
    Settings S;
    S.mode = juce::roundToInt (P.mode->load()); S.pitch = P.pitch->load(); S.formant = P.formant->load(); S.link = P.link->load() > 0.5f;
    S.key = juce::roundToInt (P.key->load()); S.scale = juce::roundToInt (P.scale->load());
    S.model = juce::roundToInt (P.model->load()); S.drive = P.drive->load(); S.tone = P.tone->load(); S.multi = P.multi->load() > 0.5f;
    for (int b = 0; b < 3; ++b) S.w[b] = P.w[b]->load();
    S.x1 = P.x1->load(); S.x2 = P.x2->load(); S.mix = P.mix->load(); S.outDb = P.out->load(); S.autoGain = P.autoG->load() > 0.5f;
    return S;
}

void Processor::prepareToPlay (double s, int mb)
{
    sr = s; maxBlock = juce::jmax (16, mb);
    pitch.prepare (s, maxBlock); drive.prepare (s, maxBlock);
    totalLat = pitch.latency() + drive.latency();
    setLatencySamples (totalLat);
    dryLen = totalLat;
    for (auto& d : dry) d.assign ((size_t) dryLen, 0.0f); dryPos = 0;
    dryL.assign ((size_t) maxBlock, 0.0f); dryR = dryL; tmpR = dryL; mono = dryL;
    mixSm.reset (s, 0.03); mixSm.setCurrentAndTargetValue (P.mix->load());
    outSm.reset (s, 0.03); outSm.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (P.out->load()));
}

void Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals nd;
    const int n = buffer.getNumSamples(), nch = juce::jmin (2, buffer.getNumChannels());
    if (nch == 0 || n == 0) return;
    auto S = settings();
    mixSm.setTargetValue (S.mix); outSm.setTargetValue (juce::Decibels::decibelsToGain (S.outDb));
    float pkI[2] {}, pkO[2] {};
    // a pezzi grandi al massimo quanto preparato: nessun buffer da ingrandire mentre suona
    for (int off = 0; off < n; off += maxBlock)
    {
        const int m = juce::jmin (maxBlock, n - off);
        float* L = buffer.getWritePointer (0) + off; float* R = nch > 1 ? buffer.getWritePointer (1) + off : tmpR.data();
        if (nch < 2) juce::FloatVectorOperations::copy (R, L, m);
        S.mono = true; for (int i = 0; i < m; ++i) if (L[i] != R[i]) { S.mono = false; break; }
        for (int i = 0; i < m; ++i) { pkI[0] = juce::jmax (pkI[0], std::abs (L[i])); pkI[1] = juce::jmax (pkI[1], std::abs (R[i])); mono[(size_t) i] = 0.5f * (L[i] + R[i]); }
        ringIn.push (mono.data(), m);
        // percorso pulito ritardato quanto la catena (per MIX)
        for (int i = 0; i < m; ++i)
        {
            dryL[(size_t) i] = dry[0][(size_t) dryPos]; dryR[(size_t) i] = dry[1][(size_t) dryPos];
            dry[0][(size_t) dryPos] = L[i]; dry[1][(size_t) dryPos] = R[i];
            if (++dryPos >= dryLen) dryPos = 0;
        }
        pitch.process (L, R, m, S);
        drive.process (L, R, m, S);
        for (int i = 0; i < m; ++i)
        {
            const float mx = mixSm.getNextValue(), og = outSm.getNextValue();
            L[i] = (dryL[(size_t) i] + (L[i] - dryL[(size_t) i]) * mx) * og; R[i] = (dryR[(size_t) i] + (R[i] - dryR[(size_t) i]) * mx) * og;
            pkO[0] = juce::jmax (pkO[0], std::abs (L[i])); pkO[1] = juce::jmax (pkO[1], std::abs (R[i])); mono[(size_t) i] = 0.5f * (L[i] + R[i]);
        }
        ringOut.push (mono.data(), m);
    }
    for (int c = 0; c < 2; ++c) { inPk[c] = juce::jmax (pkI[c], inPk[c].load() * 0.88f); outPk[c] = juce::jmax (pkO[c], outPk[c].load() * 0.88f); }
}

// ======================================================================= PRESET
const std::vector<Preset>& presets()
{
    // modelli: TAPE, VALVOLA, TRASFORMATORE, CONSOLE, CLIPPER, OVERDRIVE, DISTORSIONE, FUZZ, OTTAVA, AMP, FOLD, LOFI, RADIO
    // wLo / wMid / wHi: quanto drive arriva a bassi, medi e alti (100% = uguale, 0% = pulito)
    static const std::vector<Preset> P = {
        { "Init", "Tutto fermo: il suono passa com'e'.", {} },
        // ---------------- VOCE
        { "Voce | Scura e grossa", "Due semitoni sotto con le formanti ferme: la voce resta tua ma piu' profonda. Valvola solo sui medi per il graffio.",
          { { "pitch", -2 }, { "formant", -1.5f }, { "model", VALVOLA }, { "drive", 0.4f }, { "wLo", 0.4f }, { "wHi", 0.6f } } },
        { "Voce | Demone", "Un'ottava sotto, formanti giu': voce enorme e cupa. Il trasformatore la tiene davanti.",
          { { "pitch", -12 }, { "formant", -3 }, { "model", TRASFORMATORE }, { "drive", 0.35f } } },
        { "Voce | Gigante", "Stessa nota, formanti -5: un gigante che canta la tua melodia.", { { "formant", -5 }, { "model", TRASFORMATORE }, { "drive", 0.25f } } },
        { "Voce | Formante giovane", "Stessa nota, formanti +3: la voce sembra piu' giovane e piccola.", { { "formant", 3 }, { "model", TAPE }, { "drive", 0.1f } } },
        { "Voce | Chipmunk", "Pitch su con le formanti che seguono (LINK): la voce accelerata dei sample soul.", { { "pitch", 7 }, { "link", 1 }, { "model", TAPE }, { "drive", 0.15f } } },
        { "Voce | Hard tune La minore", "QUANTIZZA sulla scala di La minore: intonazione a scatti stile trap. Cambia KEY e SCALA col tuo pezzo.",
          { { "mode", QUANTIZZA }, { "key", 9 }, { "scale", 2 }, { "model", CONSOLE }, { "drive", 0.2f } } },
        { "Voce | Hard tune Fa# minore drill", "QUANTIZZA in Fa# minore armonica, un filo di distorsione solo sulla presenza.",
          { { "mode", QUANTIZZA }, { "key", 6 }, { "scale", 3 }, { "model", DISTORSIONE }, { "drive", 0.3f }, { "wLo", 0 }, { "wMid", 1 }, { "wHi", 0.4f }, { "x1", 300 }, { "x2", 5000 } } },
        { "Voce | Robot", "ROBOT: ogni sillaba sulla stessa nota (KEY). Cambia KEY per scegliere la nota.", { { "mode", ROBOT }, { "key", 9 }, { "model", CONSOLE }, { "drive", 0.25f } } },
        { "Voce | Saturata solo sulla presenza", "Valvola solo tra 1,5 e 6 kHz: la voce buca il mix, corpo e aria restano puliti.",
          { { "model", VALVOLA }, { "drive", 0.55f }, { "wLo", 0 }, { "wMid", 1 }, { "wHi", 0.2f }, { "x1", 1500 }, { "x2", 6000 } } },
        { "Voce | Calda col nastro", "Nastro sui bassi e sui medi, alti quasi puliti: voce morbida e piena.", { { "model", TAPE }, { "drive", 0.4f }, { "wHi", 0.3f }, { "x2", 5000 } } },
        { "Voce | Telefono", "Banda stretta e saturazione da altoparlante piccolo: intro, bridge, adlib.", { { "model", RADIO }, { "drive", 0.45f }, { "multi", 0 } } },
        { "Voce | Megafono rock", "Testata + cassa sulla voce: aggressiva e sporca.", { { "model", AMP }, { "drive", 0.45f }, { "tone", 0.2f }, { "multi", 0 } } },
        { "Voce | Patina sotto (mix 35%)", "Ottava sotto e valvola al 35% di MIX: la voce resta la tua con un'ombra grossa sotto.",
          { { "pitch", -12 }, { "model", VALVOLA }, { "drive", 0.35f }, { "mix", 0.35f } } },
        { "Voce | Ottava sopra leggera", "Ottava sopra al 25%: brillantezza per i ritornelli senza effetto chipmunk.", { { "pitch", 12 }, { "model", TAPE }, { "drive", 0.2f }, { "mix", 0.25f } } },
        { "Voce | CHICCA Adlib dall'alto", "Ottava sopra, formanti ferme e radiolina: adlib che stanno sopra la voce principale.",
          { { "pitch", 12 }, { "formant", 1 }, { "model", RADIO }, { "drive", 0.35f }, { "multi", 0 }, { "mix", 0.85f } } },
        { "Voce | CHICCA Doppia sporca", "Formanti -2 al 40% con nastro: sembra una seconda voce piu' grande dietro.", { { "formant", -2 }, { "model", TAPE }, { "drive", 0.3f }, { "mix", 0.4f } } },
        { "Voce | CHICCA Robot che sussurra", "ROBOT un'ottava sotto con fold sugli alti: voce sintetica, fredda, da intro.",
          { { "mode", ROBOT }, { "key", 2 }, { "pitch", -12 }, { "model", FOLD }, { "drive", 0.35f }, { "wLo", 0 }, { "wMid", 0.5f }, { "wHi", 1.5f }, { "mix", 0.7f } } },
        // ---------------- 808 E BASSI
        { "808 | Grosso col ferro", "Trasformatore: il ferro gonfia i bassi, l'808 cresce senza diventare fangoso.", { { "model", TRASFORMATORE }, { "drive", 0.5f }, { "multi", 0 } } },
        { "808 | Sub pulito, sopra distorto", "Sotto i 120 Hz il sub resta pulito, sopra la distorsione fa sentire l'808 su ogni cassa.",
          { { "model", DISTORSIONE }, { "drive", 0.5f }, { "wLo", 0 }, { "wMid", 1 }, { "wHi", 0.8f }, { "x1", 120 } } },
        { "808 | Clipper trap", "Clipper: piu' volume e un filo di grinta, come un clipper sul canale dell'808.", { { "model", CLIPPER }, { "drive", 0.45f }, { "multi", 0 } } },
        { "808 | Che canta (fold)", "Wavefolder solo sopra i 150 Hz: armoniche alte che fanno sentire l'808 anche sui telefoni.",
          { { "model", FOLD }, { "drive", 0.4f }, { "wLo", 0 }, { "wMid", 1 }, { "wHi", 1 }, { "x1", 150 }, { "mix", 0.8f } } },
        { "808 | Fuzz phonk", "Fuzz sui medi, sub quasi pulito: l'808 rotto del phonk che non perde il peso.",
          { { "model", FUZZ }, { "drive", 0.55f }, { "wLo", 0.2f }, { "wMid", 1 }, { "wHi", 0.6f }, { "x1", 140 } } },
        { "808 | CHICCA Ottava sopra nascosta", "Un'ottava sopra al 20% di MIX: l'808 si sente anche sulle casse piccole.", { { "pitch", 12 }, { "model", TAPE }, { "drive", 0.3f }, { "mix", 0.2f } } },
        { "Basso | Valvola calda", "Triodo in classe A: armoniche pari morbide, il basso diventa piu' rotondo.", { { "model", VALVOLA }, { "drive", 0.4f }, { "multi", 0 } } },
        { "Basso | Fuzz sopra 250 Hz", "Fuzz solo sopra i 250 Hz: il basso resta fermo ma ringhia.", { { "model", FUZZ }, { "drive", 0.55f }, { "wLo", 0 }, { "wMid", 1 }, { "wHi", 0.7f }, { "x1", 250 } } },
        { "Basso | Ottava sotto", "Pitch -12 su basso o synth: sub ottava per riempire.", { { "pitch", -12 }, { "model", TRASFORMATORE }, { "drive", 0.2f }, { "mix", 0.6f } } },
        // ---------------- SYNTH E STRUMENTI
        { "Synth | Lead che morde", "Overdrive sui medi e un po' sugli alti: il lead esce dal mix.", { { "model", OVERDRIVE }, { "drive", 0.5f }, { "wLo", 0.3f }, { "wHi", 0.7f } } },
        { "Synth | Pad su nastro", "Nastro morbido e tono scuro: pad caldi e vecchi.", { { "model", TAPE }, { "drive", 0.35f }, { "tone", -0.25f } } },
        { "Synth | Pluck ottava", "Fuzz ottava sui medi: pluck metallici per melodie trap.", { { "model", OTTAVA }, { "drive", 0.45f }, { "wLo", 0 }, { "mix", 0.6f } } },
        { "Chitarra | Crunch pedale verde", "Overdrive con le medie in avanti: crunch classico.", { { "model", OVERDRIVE }, { "drive", 0.5f }, { "multi", 0 } } },
        { "Chitarra | Amp britannico", "Testata in crunch con cassa 4x12: chitarra in diretta che sembra microfonata.", { { "model", AMP }, { "drive", 0.55f }, { "multi", 0 } } },
        { "Chitarra | Fuzz anni 60", "Fuzz al germanio a manetta: muro di sustain.", { { "model", FUZZ }, { "drive", 0.75f }, { "multi", 0 } } },
        { "Chitarra | Sample soul alzato", "Pitch +3 con formanti che seguono e nastro: il campione soul accelerato.", { { "pitch", 3 }, { "link", 1 }, { "model", TAPE }, { "drive", 0.3f } } },
        // ---------------- BATTERIA
        { "Batteria | Console densa", "Canale VCA spinto: batteria piu' densa e presente.", { { "model", CONSOLE }, { "drive", 0.45f } } },
        { "Batteria | Cassa sotto, crack sopra", "Trasformatore poco sui bassi, clipper sui medi: cassa piena e rullante che schiocca.",
          { { "model", CLIPPER }, { "drive", 0.45f }, { "wLo", 0.3f }, { "wMid", 1.2f }, { "wHi", 0.5f }, { "x1", 150 }, { "x2", 5000 } } },
        { "Batteria | Nastro sul bus", "Nastro morbido sul bus: arrotonda i transienti e incolla.", { { "model", TAPE }, { "drive", 0.4f }, { "multi", 0 } } },
        { "Batteria | CHICCA Rullante piu' grosso", "Formante -4 solo sul pitch, mix 50%: il rullante sembra suonato su un fusto piu' grande.",
          { { "formant", -4 }, { "model", CONSOLE }, { "drive", 0.25f }, { "mix", 0.5f } } },
        { "Batteria | CHICCA Hat che brillano", "Fold solo sugli alti: hat e piatti con armoniche nuove, il resto intatto.",
          { { "model", FOLD }, { "drive", 0.35f }, { "wLo", 0 }, { "wMid", 0 }, { "wHi", 1.2f }, { "x2", 6000 } } } };
    return P;
}

void Processor::loadPreset (int i)
{
    if (! juce::isPositiveAndBelow (i, (int) presets().size())) return;
    preset = i; userName = {};
    for (auto* p : getParameters()) if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p)) if (rp->paramID != "out") rp->setValueNotifyingHost (rp->getDefaultValue());
    for (auto& [id, val] : presets()[(size_t) i].v) if (auto* rp = apvts.getParameter (id)) rp->setValueNotifyingHost (rp->convertTo0to1 (val));
}

juce::File Processor::presetFolder()
{
    auto f = juce::File::getSpecialLocation (juce::File::userMusicDirectory).getChildFile ("Jackpot").getChildFile ("Alter Drive Preset");
    f.createDirectory(); return f;
}
bool Processor::saveUserPreset (const juce::String& name)
{
    auto s = apvts.copyState(); auto x = s.createXml(); if (! x) return false;
    const auto f = presetFolder().getChildFile (juce::File::createLegalFileName (name) + ".jalter");
    if (! x->writeTo (f)) return false;
    userName = f.getFileNameWithoutExtension(); preset = -1; return true;
}
bool Processor::loadUserPreset (const juce::File& f)
{
    auto x = juce::XmlDocument::parse (f); if (! x) return false;
    auto t = juce::ValueTree::fromXml (*x); if (! t.isValid()) return false;
    apvts.replaceState (t); userName = f.getFileNameWithoutExtension(); preset = -1; return true;
}

void Processor::getStateInformation (juce::MemoryBlock& d)
{
    auto s = apvts.copyState(); s.setProperty ("preset", preset, nullptr); s.setProperty ("uname", userName, nullptr); s.setProperty ("av", 2, nullptr);
    if (auto x = s.createXml()) copyXmlToBinary (*x, d);
}
void Processor::setStateInformation (const void* data, int size)
{
    auto x = getXmlFromBinary (data, size); if (! x) return;
    auto t = juce::ValueTree::fromXml (*x); if (! t.isValid()) return;
    const bool old = (int) t.getProperty ("av", 1) < 2;
    auto oldVal = [&t] (const char* id, float def) { auto c = t.getChildWithProperty ("id", id); return c.isValid() ? (float) c.getProperty ("value", def) : def; };
    // versione vecchia (4 bande, eco): si tiene il modello e il drive della banda piu' spinta
    int bm = 0; float bdv = -1, tone = 0; const int nb = juce::roundToInt (oldVal ("bands", 1));
    if (old) for (int b = 1; b <= juce::jlimit (1, 4, nb); ++b) { const auto p = "b" + juce::String (b) + "_"; const float d = oldVal ((p + "drive").toRawUTF8(), 0); if (d > bdv) { bdv = d; bm = juce::roundToInt (oldVal ((p + "model").toRawUTF8(), 0)); tone = oldVal ((p + "tone").toRawUTF8(), 0); } }
    preset = t.getProperty ("preset", 0); userName = t.getProperty ("uname", "").toString();
    apvts.replaceState (t);
    if (old)
    {
        auto set = [this] (const char* id, float v) { if (auto* rp = apvts.getParameter (id)) rp->setValueNotifyingHost (rp->convertTo0to1 (v)); };
        set ("model", (float) bm); set ("drive", juce::jmax (0.0f, bdv)); set ("tone", tone); set ("multi", nb > 1 ? 1.0f : 0.0f);
        for (auto* id : { "wLo", "wMid", "wHi" }) set (id, 1.0f);
        preset = -1;
    }
}

juce::AudioProcessorEditor* Processor::createEditor() { return new Editor (*this); }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new ja::Processor(); }
