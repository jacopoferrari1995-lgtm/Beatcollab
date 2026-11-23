#include "S8Processor.h"
#include "S8Editor.h"

namespace s8
{
const std::vector<Preset>& presets()
{
    static const std::vector<Preset> E = {
        { "Init", "Tutto a zero: l'808 passa intatta. Guarda la diagnosi a destra e parti da li'.", {} },
        { "Trap | 808 Pulita", "Sub pulito, un filo di nastro sul corpo, armoniche per i telefoni, ducking leggero sulla cassa.", { { "punch", 0.3f }, { "drive", 0.15f }, { "harm", 0.25f }, { "duck", 0.3f }, { "rel", 90 }, { "mono", 3 }, { "ceil", -1 } } },
        { "Trap | Debole da rinforzare", "Per gli 808 sottili: SUB+ aggiunge una sinusoide pulita agganciata alla nota, KNOCK da' il colpo.", { { "subPlus", 0.6f }, { "punch", 0.25f }, { "knock", 3 }, { "knockHz", 180 }, { "harm", 0.3f }, { "mono", 3 }, { "ceil", -1 } } },
        { "Drill | Grintosa", "Attacco in avanti, saturazione dura sul corpo, ducking piu' deciso per le casse veloci.", { { "punch", 0.45f }, { "drive", 0.45f }, { "mode", 2 }, { "harm", 0.35f }, { "duck", 0.4f }, { "rel", 70 }, { "mono", 3 }, { "ceil", -0.5f } } },
        { "Drill | UK scivolata", "Per le 808 che scivolano: corpo duro, alti tagliati sopra i 6 kHz, sub in mono.", { { "punch", 0.3f }, { "drive", 0.35f }, { "mode", 2 }, { "topHz", 6000 }, { "harm", 0.3f }, { "duck", 0.35f }, { "rel", 80 }, { "mono", 3 } } },
        { "Spinz | Distorta", "La grana sporca e asimmetrica delle 808 distorte, con il sub che resta sotto pulito.", { { "drive", 0.7f }, { "mode", 4 }, { "harm", 0.2f }, { "punch", 0.2f }, { "mono", 4 }, { "ceil", -1 } } },
        { "Rage | Distorta", "808 che riempie tutto: carattere RAGE sul corpo e sugli alti, sub tenuto pulito sotto.", { { "punch", 0.2f }, { "sustain", 0.1f }, { "drive", 0.8f }, { "mode", 5 }, { "harm", 0.2f }, { "duck", 0.25f }, { "rel", 90 }, { "mono", 4 }, { "ceil", -1 } } },
        { "Su Telefono", "Per chi ascolta da telefono e cuffiette: tante armoniche, un po' di valvola, knock in alto.", { { "punch", 0.15f }, { "drive", 0.3f }, { "mode", 1 }, { "harm", 0.7f }, { "knock", 2.5f }, { "knockHz", 220 }, { "duck", 0.2f }, { "rel", 100 }, { "mono", 3 } } },
        { "Boom Bap | Sub Caldo", "Basso caldo e tondo: valvola leggera, alti morbidi, coda un po' piu' lunga.", { { "punch", 0.15f }, { "sustain", 0.2f }, { "drive", 0.25f }, { "mode", 1 }, { "harm", 0.2f }, { "topHz", 5000 }, { "duck", 0.15f }, { "rel", 150 }, { "mono", 2 } } },
        { "Lunga | Melodica", "808 che canta la melodia: coda allungata, armoniche per seguire le note.", { { "punch", 0.1f }, { "sustain", 0.5f }, { "drive", 0.2f }, { "harm", 0.35f }, { "duck", 0.25f }, { "rel", 150 }, { "mono", 3 } } },
        { "Corta | Bouncy", "808 corta e saltellante: coda accorciata, attacco in avanti.", { { "punch", 0.5f }, { "sustain", -0.5f }, { "drive", 0.2f }, { "harm", 0.2f }, { "duck", 0.2f }, { "rel", 60 }, { "mono", 3 } } },
        { "Club | Sub pieno", "Per gli impianti grossi: SUB in piu', sub in mono fino a 120 Hz, clipper che tiene i picchi.", { { "subDb", 3 }, { "subPlus", 0.3f }, { "punch", 0.2f }, { "drive", 0.15f }, { "mono", 3 }, { "ceil", -1.5f }, { "duck", 0.3f }, { "rel", 90 } } },
        { "Lo-fi | Scura", "808 smussata: alti chiusi, nastro, poco attacco.", { { "topHz", 1800 }, { "drive", 0.25f }, { "punch", -0.0f }, { "sustain", 0.15f }, { "mono", 2 } } },
        { "Cassa e 808 | Spazio", "Per quando cassa e 808 si pestano: ducking forte e veloce, controlla la fase nella diagnosi.", { { "duck", 0.6f }, { "rel", 70 }, { "punch", 0.2f }, { "mono", 3 } } } };
    static const std::vector<Preset> S = {
        { "Init", "Sinusoide pura, catena a zero: l'808 di partenza.", {} },
        { "TR-808 | Classica", "La 808 della drum machine: colpo di pitch rapido, click leggero, coda media.", { { "s_pamt", 7 }, { "s_ptime", 30 }, { "s_click", 0.15f }, { "s_dec", 1.6f }, { "s_tb", 0.08f }, { "harm", 0.2f } } },
        { "Trap | Lunga pulita", "Coda lunga e pulita, scivola fra le note tenute, armoniche per i telefoni.", { { "s_pamt", 3 }, { "s_ptime", 40 }, { "s_dec", 4.5f }, { "s_tb", 0.12f }, { "s_glide", 80 }, { "harm", 0.3f }, { "mono", 3 }, { "punch", 0.2f } } },
        { "Trap | Scivolata", "Tieni una nota e suonane un'altra: scivola senza riattaccare.", { { "s_glide", 140 }, { "s_gmode", 1 }, { "s_dec", 5 }, { "s_tb", 0.15f }, { "s_dist", 0.1f }, { "harm", 0.3f }, { "mono", 3 } } },
        { "Drill | UK scivolata", "Scivolate veloci, corpo duro e un po' di click.", { { "s_glide", 90 }, { "s_pamt", 5 }, { "s_ptime", 25 }, { "s_tb", 0.25f }, { "s_dist", 0.2f }, { "s_click", 0.1f }, { "s_dec", 3 }, { "drive", 0.3f }, { "mode", 2 }, { "duck", 0.3f }, { "mono", 3 } } },
        { "Drill | NY grezza", "Distorta e dura, scivola sempre da una nota all'altra.", { { "s_dist", 0.45f }, { "s_tb", 0.3f }, { "s_glide", 60 }, { "s_gmode", 2 }, { "drive", 0.5f }, { "mode", 2 }, { "s_dec", 2.5f }, { "mono", 3 } } },
        { "Spinz | Distorta", "Grana sporca e asimmetrica, colpo di pitch alto all'attacco.", { { "s_dist", 0.7f }, { "s_tb", 0.4f }, { "s_pamt", 12 }, { "s_ptime", 18 }, { "s_click", 0.3f }, { "mode", 4 }, { "drive", 0.5f }, { "ceil", -1 }, { "mono", 4 } } },
        { "Rage | Satura", "Tutto pieno: distorsione, un velo di reese e carattere RAGE.", { { "s_dist", 0.85f }, { "s_reese", 0.2f }, { "s_tb", 0.5f }, { "mode", 5 }, { "drive", 0.7f }, { "mono", 4 }, { "ceil", -1 }, { "s_dec", 3 } } },
        { "Reese | 808 Zay", "Due denti di sega stonati sopra la sinusoide: 808 che si muove.", { { "s_reese", 0.7f }, { "s_tb", 0.3f }, { "s_dist", 0.25f }, { "s_dec", 3.5f }, { "s_glide", 100 }, { "mono", 3 } } },
        { "Boom Bap | Tonda", "Basso tondo e caldo, coda corta, alti morbidi.", { { "s_dec", 1.2f }, { "s_tb", 0.05f }, { "s_pamt", 2 }, { "mode", 1 }, { "drive", 0.2f }, { "topHz", 6000 }, { "mono", 2 } } },
        { "Boom Bap | Kick 808", "La 808 usata come cassa: colpo di pitch enorme, click, corta.", { { "s_pamt", 24 }, { "s_ptime", 22 }, { "s_click", 0.5f }, { "s_dec", 0.6f }, { "punch", 0.4f } } },
        { "Corta | Bouncy", "Corta e saltellante per i giri veloci.", { { "s_dec", 0.45f }, { "s_pamt", 9 }, { "s_ptime", 20 }, { "s_click", 0.25f }, { "punch", 0.5f }, { "mono", 3 } } },
        { "Jersey | Saltata", "Cortissima e acuta all'attacco, per i giri jersey club.", { { "s_dec", 0.3f }, { "s_pamt", 14 }, { "s_ptime", 15 }, { "s_click", 0.35f }, { "punch", 0.3f } } },
        { "Sub | Solo sotto", "Solo il sub: sinusoide pura, niente sopra i 400 Hz.", { { "s_dec", 3 }, { "topHz", 400 }, { "mono", 3 } } },
        { "Telefono | Si sente ovunque", "Armoniche esatte e un po' di valvola: la nota si sente anche dove il sub non arriva.", { { "harm", 0.8f }, { "s_tb", 0.2f }, { "mode", 1 }, { "drive", 0.3f }, { "s_dist", 0.15f }, { "s_dec", 2.5f }, { "mono", 3 } } },
        { "Detroit | Sporca", "Distorsione FUZZ, attacco acuto, nessuno scivolamento.", { { "s_dist", 0.55f }, { "s_tb", 0.35f }, { "s_click", 0.2f }, { "s_pamt", 10 }, { "s_ptime", 35 }, { "drive", 0.4f }, { "mode", 3 }, { "s_dec", 1.8f } } },
        { "Lo-fi | Morbida", "Smussata e calda, alti chiusi.", { { "s_tb", 0.02f }, { "topHz", 1800 }, { "s_dec", 1.8f }, { "s_pamt", 1 }, { "drive", 0.25f } } },
        { "Amapiano | Log drum", "Il log drum: timbro legnoso, pitch che scende piano, corto.", { { "s_tb", 0.55f }, { "s_pamt", 5 }, { "s_ptime", 60 }, { "s_dec", 0.7f }, { "s_click", 0.2f }, { "topHz", 3000 }, { "drive", 0.2f }, { "mode", 1 } } },
        { "Tenuta | Basso melodico", "Resta su finche' tieni il tasto: per le linee di basso suonate.", { { "s_sus", 0.8f }, { "s_rel", 300 }, { "s_tb", 0.1f }, { "s_dec", 1.5f }, { "s_glide", 60 }, { "harm", 0.25f } } },
        { "Poly | Accordi bassi", "Polifonica: piu' note insieme per gli accordi nel basso.", { { "s_poly", 1 }, { "s_dec", 3 }, { "s_tb", 0.2f }, { "s_reese", 0.2f }, { "s_glide", 0 } } } };
    return kSynth ? S : E;
}

juce::AudioProcessorValueTreeState::ParameterLayout Processor::layout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    auto F = [&] (const char* id, const char* nm, float a, float b, float d, float st = 0, float skew = 1) { NormalisableRange<float> r (a, b, st); if (skew != 1) r.setSkewForCentre (skew); l.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, nm, r, d)); };
    auto C = [&] (const char* id, const char* nm, StringArray o, int d) { l.add (std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, nm, o, d)); };
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "inv", 1 }, "Inverti fase", false));
    F ("punch", "Punch", 0, 1, 0); F ("sustain", "Sustain", -1, 1, 0);
    F ("subPlus", "Sub+", 0, 1, 0); F ("subDb", "Sub", -9, 9, 0, 0.1f); F ("knock", "Knock", 0, 10, 0, 0.1f); F ("knockHz", "Knock Freq", 60, 400, 160, 1, 160);
    F ("topHz", "Top", 400, 20000, 20000, 1, 3000);
    F ("drive", "Drive", 0, 1, 0); C ("mode", "Carattere", { "TAPE", "TUBE", "HARD", "FUZZ", "SPINZ", "RAGE" }, 0);
    F ("harm", "Armoniche", 0, 1, 0);
    F ("duck", "Ducking cassa", 0, 1, 0); F ("rel", "Rilascio ducking", 20, 400, 120, 1);
    C ("mono", "Mono sub", { "OFF", "60 Hz", "90 Hz", "120 Hz", "150 Hz" }, 0);
    F ("ceil", "Ceiling", -12, 0, 0, 0.1f); F ("out", "Output", -12, 12, 0, 0.1f);
    if (kSynth)
    {
        F ("s_tb", "Timbro", 0, 1, 0); F ("s_reese", "Reese", 0, 1, 0); F ("s_sub", "Sub ottava", 0, 1, 0); F ("s_click", "Click", 0, 1, 0); F ("s_dist", "Distorsione", 0, 1, 0);
        F ("s_pamt", "Pitch", 0, 24, 0, 0.1f); F ("s_ptime", "Pitch tempo", 5, 300, 60, 1, 60);
        F ("s_glide", "Glide", 0, 1000, 0, 1, 150); C ("s_gmode", "Glide modo", { "OFF", "LEGATO", "SEMPRE" }, 1);
        F ("s_tune", "Tune", -12, 12, 0, 1); F ("s_fine", "Fine", -100, 100, 0, 1);
        F ("s_att", "Attacco", 0, 50, 1, 0.1f, 8); F ("s_dec", "Decadimento", 0.05f, 8, 2.5f, 0.01f, 1.5f); F ("s_sus", "Sustain", 0, 1, 0); F ("s_rel", "Rilascio", 10, 2000, 150, 1, 300);
        F ("s_vel", "Velocity", 0, 1, 0.5f); l.add (std::make_unique<AudioParameterBool> (ParameterID { "s_poly", 1 }, "Poly", false)); F ("s_bend", "Pitch bend", 0, 12, 2, 1);
        F ("s_smix", "Sample", 0, 1, 0); F ("s_sstart", "Inizio sample", 0, 0.5f, 0, 0.001f); F ("s_lvl", "Livello", -24, 6, 0, 0.1f);
    }
    return l;
}

Processor::Processor()
    : AudioProcessor (kSynth ? BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true).withInput ("Cassa (sidechain)", juce::AudioChannelSet::stereo(), false)
                             : BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true).withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                                                .withInput ("Cassa (sidechain)", juce::AudioChannelSet::stereo(), false)),
      apvts (*this, &undo, "PARAMS", layout())
{
    fmt.registerBasicFormats();
}
bool Processor::isBusesLayoutSupported (const BusesLayout& l) const
{
    if (l.getMainOutputChannelSet() != juce::AudioChannelSet::stereo()) return false;
    const int scBus = kSynth ? 0 : 1;
    if (! kSynth && l.getMainInputChannelSet() != juce::AudioChannelSet::stereo()) return false;
    if (l.inputBuses.size() > scBus) { const auto sc = l.inputBuses[scBus]; if (! sc.isDisabled() && sc != juce::AudioChannelSet::stereo() && sc != juce::AudioChannelSet::mono()) return false; }
    return true;
}
void Processor::prepareToPlay (double s, int block)
{
    srNow = s; ch.prepare (s, block);
    if (kSynth) syn.prepare (s, block);
    tmp.setSize (2, juce::jmax (block, 4096), false, true, false);
    runaway = 0; muteLeft = 0; fadeIn = 1;
    setLatencySamples (ch.latency() + (kSynth ? syn.latency() : 0));
}
ChainSet Processor::chainSet() const
{
    ChainSet S;
    S.inv = v ("inv") > 0.5f; S.punch = v ("punch"); S.sustain = v ("sustain"); S.subPlus = kSynth ? 0.0f : v ("subPlus"); S.subDb = v ("subDb"); S.knock = v ("knock"); S.knockHz = v ("knockHz");
    S.topHz = v ("topHz"); S.drive = v ("drive"); S.mode = juce::roundToInt (v ("mode")); S.harm = v ("harm"); S.duck = v ("duck"); S.rel = v ("rel");
    S.mono = juce::roundToInt (v ("mono")); S.ceil = v ("ceil"); S.out = v ("out");
    return S;
}
SynthSet Processor::synthSet() const
{
    SynthSet S;
    S.tb = v ("s_tb"); S.reese = v ("s_reese"); S.sub = v ("s_sub"); S.click = v ("s_click"); S.dist = v ("s_dist"); S.pAmt = v ("s_pamt"); S.pTime = v ("s_ptime");
    S.glide = v ("s_glide"); S.gMode = juce::roundToInt (v ("s_gmode")); if (S.gMode == 0) S.glide = 0;
    S.tune = v ("s_tune"); S.fine = v ("s_fine"); S.att = v ("s_att"); S.dec = v ("s_dec"); S.sus = v ("s_sus"); S.rel = v ("s_rel"); S.vel = v ("s_vel");
    S.poly = v ("s_poly") > 0.5f; S.bend = v ("s_bend"); S.sMix = v ("s_smix"); S.sStart = v ("s_sstart"); S.lvl = v ("s_lvl");
    return S;
}

void Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals nd;
    const int N = buffer.getNumSamples(); if (N == 0 || buffer.getNumChannels() < 2) return;
    float* L = buffer.getWritePointer (0); float* R = buffer.getWritePointer (1);
    const float* kL = nullptr; const float* kR = nullptr;
    auto sc = getBusBuffer (buffer, true, kSynth ? 0 : 1);
    if (sc.getNumChannels() > 0 && getBus (true, kSynth ? 0 : 1) != nullptr && getBus (true, kSynth ? 0 : 1)->isEnabled())
    {
        if (tmp.getNumSamples() < N) tmp.setSize (2, N, false, false, true);
        for (int c = 0; c < 2; ++c) tmp.copyFrom (c, 0, sc, juce::jmin (c, sc.getNumChannels() - 1), 0, N);   // copia: nel SYNTH il sidechain divide i canali con l'uscita
        kL = tmp.getReadPointer (0); kR = tmp.getReadPointer (1);
    }
    if (kSynth) syn.render (L, R, N, midi, synthSet());
    else for (int i = 0; i < N; ++i) { if (! std::isfinite (L[i])) L[i] = 0; if (! std::isfinite (R[i])) R[i] = 0; }
    ch.process (L, R, N, kL, kR, chainSet());
    for (int c = 2; c < buffer.getNumChannels(); ++c) buffer.clear (c, 0, N);
    // protezione da live: NaN o un suono enorme che si autoalimenta -> muto e ripartenza pulita; tetto a +6 dBFS
    bool broken = false; float pkO = 0;
    for (int i = 0; i < N; ++i) { if (! std::isfinite (L[i]) || ! std::isfinite (R[i])) { broken = true; break; } pkO = juce::jmax (pkO, std::abs (L[i]), std::abs (R[i])); }
    const float inPk = juce::Decibels::decibelsToGain (ch.inPeak.load());
    if (! broken && pkO > 1.4f && inPk < 0.05f && (! kSynth || syn.uiVoices.load() == 0)) { runaway += N; if (runaway > (int) (srNow * 0.5)) broken = true; } else runaway = juce::jmax (0, runaway - N);
    if (broken) { ++panics; runaway = 0; muteLeft = (int) (srNow * 0.15); fadeIn = 0; triggerAsyncUpdate(); }
    if (muteLeft > 0) { const int m = juce::jmin (N, muteLeft); std::fill (L, L + m, 0.0f); std::fill (R, R + m, 0.0f); muteLeft -= m; }
    constexpr float k = 1.4125f, c = 1.9953f; const float fs = 1.0f / (float) (0.05 * srNow);
    for (int i = 0; i < N; ++i)
    {
        if (fadeIn < 1.0f) { fadeIn = juce::jmin (1.0f, fadeIn + fs); L[i] *= fadeIn; R[i] *= fadeIn; }
        for (float* x : { L + i, R + i }) { const float a = std::abs (*x); if (a > k) *x = std::copysign (k + (c - k) * std::tanh ((a - k) / (c - k)), *x); }
    }
    midi.clear();
}

bool Processor::loadSampleData (const void* d, size_t n, const juce::String& name, juce::String& err)
{
    std::unique_ptr<juce::AudioFormatReader> rd (fmt.createReaderFor (std::make_unique<juce::MemoryInputStream> (d, n, false)));
    if (rd == nullptr) { err = "Formato non letto (usa WAV, AIFF o FLAC)"; return false; }
    const int len = (int) std::min<juce::int64> (rd->lengthInSamples, (juce::int64) (rd->sampleRate * 12.0));
    if (len < 64) { err = "Sample troppo corto"; return false; }
    SampleData::Ptr p = new SampleData();
    p->buf.setSize (2, len); rd->read (&p->buf, 0, len, 0, true, true);
    if (rd->numChannels == 1) p->buf.copyFrom (1, 0, p->buf, 0, 0, len);
    const float mx = p->buf.getMagnitude (0, len); if (mx > 1e-6f) p->buf.applyGain (0.89f / mx);   // livello uguale per tutti i sample
    p->fileSr = rd->sampleRate; p->name = name; p->wav.append (d, n);
    p->root = SampleData::findRoot (p->buf, p->fileSr);
    syn.setSample (p); sampleRef = p;
    if (v ("s_smix") < 0.01f) setV ("s_smix", 1.0f);
    return true;
}
bool Processor::loadSample (const juce::File& f, juce::String& err)
{
    if (f.getSize() > 24 * 1024 * 1024) { err = "File troppo grande (massimo 24 MB)"; return false; }
    juce::MemoryBlock mb; if (! f.loadFileAsData (mb)) { err = "Non riesco a leggere il file"; return false; }
    return loadSampleData (mb.getData(), mb.getSize(), f.getFileNameWithoutExtension(), err);
}
void Processor::clearSample() { syn.setSample (nullptr); sampleRef = nullptr; }
void Processor::setSampleRoot (float r)
{
    if (sampleRef == nullptr) return;
    SampleData::Ptr p = new SampleData(); p->buf.makeCopyOf (sampleRef->buf); p->fileSr = sampleRef->fileSr; p->name = sampleRef->name; p->wav = sampleRef->wav; p->root = r;
    syn.setSample (p); sampleRef = p;
}

void Processor::loadPreset (int i)
{
    const auto& P = presets(); i = juce::jlimit (0, (int) P.size() - 1, i);
    undo.beginNewTransaction();
    for (auto* prm : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (prm))
        {   // tutto torna al neutro, il sample caricato resta e resta il suo mix
            if (rp->getParameterID() == "s_smix") continue;
            rp->setValueNotifyingHost (rp->getDefaultValue());
        }
    for (auto& kv : P[(size_t) i].p) setV (kv.first, kv.second);
    presetName = P[(size_t) i].name; curPreset = i;
}
void Processor::getStateInformation (juce::MemoryBlock& d)
{
    auto s = apvts.copyState(); s.setProperty ("preset", presetName, nullptr);
    if (kSynth && sampleRef != nullptr) { s.setProperty ("sample", sampleRef->wav.toBase64Encoding(), nullptr); s.setProperty ("sname", sampleRef->name, nullptr); s.setProperty ("sroot", sampleRef->root, nullptr); }
    if (auto x = s.createXml()) copyXmlToBinary (*x, d);
}
void Processor::setStateInformation (const void* data, int size)
{
    if (auto x = getXmlFromBinary (data, size))
    {
        auto t = juce::ValueTree::fromXml (*x); if (! t.isValid()) return;
        presetName = t.getProperty ("preset", presetName).toString();
        if (kSynth)
        {
            const auto b64 = t.getProperty ("sample").toString();
            if (b64.isNotEmpty())
            {
                juce::MemoryBlock mb; juce::String err;
                if (mb.fromBase64Encoding (b64) && loadSampleData (mb.getData(), mb.getSize(), t.getProperty ("sname").toString(), err))
                    if (t.hasProperty ("sroot")) setSampleRoot ((float) t.getProperty ("sroot"));
            }
            else clearSample();
            t.removeProperty ("sample", nullptr);
        }
        apvts.replaceState (t);
    }
}
juce::AudioProcessorEditor* Processor::createEditor() { return new Editor (*this); }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new s8::Processor(); }
