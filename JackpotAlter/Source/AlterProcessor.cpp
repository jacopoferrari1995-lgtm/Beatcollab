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
        L[i] = cl + (yl - cl) * wet; R[i] = cr + (yr - cr) * wet;
        if (++hopPos >= H)
        {
            hopPos = 0;
            if (on || wet > 0) frame (S);
            else { for (auto& p : pv) p.markFresh(); }   // fermo: niente calcolo, il percorso pulito basta
        }
    }
}

// ======================================================================= DRIVE
void DriveStage::prepare (double s, int maxBlock)
{
    sr = s; maxB = juce::jmax (64, juce::jmin (maxBlock, 1024));
    os = std::make_unique<juce::dsp::Oversampling<float>> (2, 2, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, true);
    os->initProcessing ((size_t) maxB);
    osLat = juce::roundToInt (os->getLatencyInSamples());
    for (auto& c : ch) c.prepare (sr * 4, 4);
    juce::dsp::ProcessSpec sp { sr, (juce::uint32) maxB, 2 };
    xLo.prepare (sp); xHi.prepare (sp);
    xLo.setType (juce::dsp::LinkwitzRileyFilterType::lowpass); xHi.setType (juce::dsp::LinkwitzRileyFilterType::lowpass);
    band.setSize (2, maxB); other.setSize (2, maxB);
    for (auto& d : odl) d.assign ((size_t) juce::jmax (1, osLat), 0.0f); oPos = 0;
    lastTone = 99; eIn = eOut = 1e-6f; agSm = 1;
}

void DriveStage::process (float* L, float* R, int n, const Settings& S)
{
    for (int off = 0; off < n; off += maxB)
    {
        const int m = juce::jmin (maxB, n - off);
        float* l = L + off; float* r = R + off;
        float* bl = band.getWritePointer (0); float* br = band.getWritePointer (1);
        float* ol = other.getWritePointer (0); float* orr = other.getWritePointer (1);
        // banda FOCUS: si satura solo questa, il resto passa pulito (crossover Linkwitz-Riley, somma piatta)
        if (S.focus)
        {
            xLo.setCutoffFrequency (juce::jlimit (20.0f, 18000.0f, S.fLo)); xHi.setCutoffFrequency (juce::jlimit (S.fLo * 1.05f, 20000.0f, S.fHi));
            for (int i = 0; i < m; ++i)
            {
                float lo, rest, b, hi;
                xLo.processSample (0, l[i], lo, rest); xHi.processSample (0, rest, b, hi); bl[i] = b; ol[i] = lo + hi;
                xLo.processSample (1, r[i], lo, rest); xHi.processSample (1, rest, b, hi); br[i] = b; orr[i] = lo + hi;
            }
        }
        else { juce::FloatVectorOperations::copy (bl, l, m); juce::FloatVectorOperations::copy (br, r, m); juce::FloatVectorOperations::clear (ol, m); juce::FloatVectorOperations::clear (orr, m); }
        double ei = 0; for (int i = 0; i < m; ++i) ei += bl[i] * bl[i] + br[i] * br[i];

        // distorsione sovracampionata 4x
        for (auto& c : ch) { c.setModel (S.model); c.setDrive (S.drive); }
        const float g = juce::Decibels::decibelsToGain (S.drive * modelInfo (S.model).maxDb);
        juce::dsp::AudioBlock<float> blk (band.getArrayOfWritePointers(), 2, (size_t) m);
        auto up = os->processSamplesUp (blk);
        for (int c = 0; c < 2; ++c)
        {
            float* u = up.getChannelPointer ((size_t) c); auto& dc = ch[c];
            for (size_t i = 0; i < up.getNumSamples(); ++i) u[i] = dc.process (u[i], g, S.drive);
        }
        os->processSamplesDown (blk);

        // tono (inclinazione attorno a 800 Hz)
        if (std::abs (S.tone - lastTone) > 1e-3f)
        {
            lastTone = S.tone;
            for (int c = 0; c < 2; ++c) { toneLo[c].shelf (sr, 800, -6.0 * S.tone, false); toneHi[c].shelf (sr, 800, 6.0 * S.tone, true); }
        }
        if (std::abs (S.tone) > 1e-3f) for (int i = 0; i < m; ++i) { bl[i] = toneHi[0] (toneLo[0] (bl[i])); br[i] = toneHi[1] (toneLo[1] (br[i])); }

        // guadagno automatico: stessa energia prima e dopo la saturazione
        double eo = 0; for (int i = 0; i < m; ++i) eo += bl[i] * bl[i] + br[i] * br[i];
        const float k = std::exp (-(float) m / (0.15f * (float) sr));
        eIn = k * eIn + (1 - k) * (float) (ei / m); eOut = k * eOut + (1 - k) * (float) (eo / m);
        float target = 1;
        if (S.autoGain && eIn > 1e-9f) target = juce::jlimit (0.03f, 4.0f, std::sqrt (eIn / juce::jmax (1e-12f, eOut)));
        // se deve abbassare tanto lo fa subito (niente botte quando cambi modello), se alza lo fa piano
        float a0 = agSm; if (target < a0 * 0.7f) a0 = target;
        autoDb = juce::Decibels::gainToDecibels (target);
        const float step = (target - a0) / (float) m;
        for (int i = 0; i < m; ++i) { const float a = a0 + step * (float) (i + 1); bl[i] *= a; br[i] *= a; }
        agSm = target;

        // ricomposizione: banda saturata + resto ritardato della stessa latenza
        const int D = (int) odl[0].size();
        for (int i = 0; i < m; ++i)
        {
            float dl = ol[i], dr = orr[i];
            if (osLat > 0) { dl = odl[0][(size_t) oPos]; dr = odl[1][(size_t) oPos]; odl[0][(size_t) oPos] = ol[i]; odl[1][(size_t) oPos] = orr[i]; oPos = (oPos + 1) % D; }
            if (S.listen) { l[i] = bl[i]; r[i] = br[i]; }
            else { l[i] = bl[i] + dl; r[i] = br[i] + dr; }
        }
    }
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
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "model", 1 }, "Model", models, 0));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "drive", 1 }, "Drive", NormalisableRange<float> (0, 1), 0.25f));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "tone", 1 }, "Tone", NormalisableRange<float> (-1, 1), 0));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "focus", 1 }, "Focus", false));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "fLo", 1 }, "Focus Low", NormalisableRange<float> (20, 8000, 0, 0.3f), 150, AudioParameterFloatAttributes().withLabel ("Hz")));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "fHi", 1 }, "Focus High", NormalisableRange<float> (100, 20000, 0, 0.3f), 5000, AudioParameterFloatAttributes().withLabel ("Hz")));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "listen", 1 }, "Focus Listen", false));
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
    S.model = (int) value ("model"); S.drive = value ("drive"); S.tone = value ("tone");
    S.focus = value ("focus") > 0.5f; S.fLo = value ("fLo"); S.fHi = juce::jmax (value ("fHi"), S.fLo * 1.1f); S.listen = value ("listen") > 0.5f;
    S.order = (int) value ("order"); S.autoGain = value ("auto") > 0.5f; S.mix = value ("mix"); S.outDb = value ("out");
    return S;
}

void Processor::prepareToPlay (double s, int maxBlock)
{
    sr = s;
    pitch.prepare (s, maxBlock); drive.prepare (s, maxBlock);
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

    const float og = juce::Decibels::decibelsToGain (S.outDb), mix = S.listen ? 1.0f : S.mix;
    float pkO[2] = { 0, 0 };
    for (int i = 0; i < n; ++i)
    {
        L[i] = (dryL[(size_t) i] * (1 - mix) + L[i] * mix) * og; Rw[i] = (dryR[(size_t) i] * (1 - mix) + Rw[i] * mix) * og;
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
        { "FX | Radio rotta", "Radiolina + salti di intonazione: messaggi da un altro mondo.", { { "model", RADIO }, { "drive", 0.55f }, { "drift", 0.35f }, { "driftRate", 2.0f }, { "driftType", CAOS } } } };
    return P;
}

void Processor::loadPreset (int i)
{
    if (! juce::isPositiveAndBelow (i, (int) presets().size())) return;
    preset = i;
    for (auto* p : getParameters()) if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p)) if (rp->paramID != "out") rp->setValueNotifyingHost (rp->getDefaultValue());
    for (auto& [id, v] : presets()[(size_t) i].v) if (auto* rp = apvts.getParameter (id)) rp->setValueNotifyingHost (rp->convertTo0to1 (v));
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
