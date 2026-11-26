#include "TagProcessor.h"
#include "TagEditor.h"

namespace jtag
{
const std::vector<Preset>& presets()
{
    static const std::vector<Preset> p = {
        { "INIT", "BASE", {}, "Tutto spento: la frase esce com'e'. Parti da qui per costruire il tuo tag." },
        { "PULITO LARGO", "BASE", { { "verb", 0.15f }, { "size", 0.5f }, { "width", 0.5f } }, "Il tag cosi' com'e', solo aperto e con un filo di stanza." },
        { "FIRMA CORTA", "BASE", { { "drive", 0.1f }, { "echo", 0.18f }, { "echoDiv", 2 }, { "echoFb", 0.25f } }, "Leggermente caldo con una sola eco a 1/4: la firma classica a inizio beat." },
        { "METRO DARK", "DARK", { { "tape", -5 }, { "drive", 0.15f }, { "echo", 0.3f }, { "echoDiv", 2 }, { "echoFb", 0.4f }, { "verb", 0.25f }, { "size", 0.7f } }, "Rallentato come un nastro, con eco e stanza da tunnel: tag scuro da trap." },
        { "GIGANTE", "DARK", { { "tape", -9 }, { "drive", 0.2f }, { "verb", 0.2f }, { "size", 0.6f } }, "Voce lenta e grossa, un mostro che firma il beat." },
        { "REVERSE GHOST", "DARK", { { "pitch", -3 }, { "swell", 0.8f }, { "verb", 0.35f }, { "size", 0.8f }, { "width", 0.5f } }, "Entra dal nulla con lo swell, un po' piu' bassa: fantasma." },
        { "SUSSURRO IN CHIESA", "DARK", { { "swell", 0.4f }, { "verb", 0.7f }, { "size", 0.95f }, { "width", 0.6f } }, "Bagnata in una stanza enorme: per i tag sussurrati." },
        { "DEMONE", "DARK", { { "pitch", -12 }, { "drive", 0.3f }, { "verb", 0.3f }, { "size", 0.8f }, { "width", 0.3f } }, "Un'ottava sotto e saturata: la voce del demone." },
        { "NEBBIA", "DARK", { { "tape", -2 }, { "radio", 0.25f }, { "swell", 0.6f }, { "echo", 0.35f }, { "echoDiv", 3 }, { "echoFb", 0.55f }, { "verb", 0.45f }, { "size", 0.85f }, { "width", 0.7f } }, "Ovattata e lontana, eco lunga: si perde nel fumo." },
        { "RADIO PIRATA", "SPORCO", { { "radio", 0.9f }, { "drive", 0.35f }, { "crush", 0.15f }, { "echo", 0.2f }, { "echoDiv", 1 }, { "echoFb", 0.35f } }, "Banda stretta e saturata, come una trasmissione pirata." },
        { "ROTTO", "SPORCO", { { "crush", 0.8f }, { "drive", 0.6f }, { "radio", 0.5f } }, "Bitcrusher e drive a palla: il tag distrutto." },
        { "DRILL UK", "SPORCO", { { "tape", -3 }, { "radio", 0.3f }, { "echo", 0.35f }, { "echoDiv", 1 }, { "echoFb", 0.45f }, { "swell", 0.3f }, { "verb", 0.15f } }, "Un po' rallentato e filtrato, eco a 1/8 puntato: il suono drill." },
        { "MEGAFONO", "SPORCO", { { "radio", 1.0f }, { "drive", 0.5f }, { "verb", 0.1f } }, "Tutto medio e distorto: annuncio dal megafono." },
        { "CASSETTA MORTA", "SPORCO", { { "tape", -1.5f }, { "radio", 0.4f }, { "crush", 0.3f }, { "drive", 0.25f } }, "Nastro stanco, un po' calante e consumato." },
        { "CYBORG", "CYBER", { { "pitch", -2 }, { "robot", 0.8f }, { "crush", 0.3f }, { "verb", 0.2f }, { "width", 0.4f } }, "Voce metallica con un filo di bit: mezzo uomo mezzo macchina." },
        { "ALIEN", "CYBER", { { "pitch", 5 }, { "robot", 0.5f }, { "swell", 0.5f }, { "verb", 0.3f }, { "width", 0.7f } }, "Piu' alta, robotica e larga: trasmissione da un altro pianeta." },
        { "GLITCH", "CYBER", { { "stutter", 2 }, { "crush", 0.5f }, { "robot", 0.3f }, { "echo", 0.15f }, { "echoDiv", 0 } }, "Si inceppa, crusha e riparte: errore di sistema." },
        { "NETRUNNER", "CYBER", { { "pitch", -1 }, { "robot", 0.45f }, { "radio", 0.35f }, { "echo", 0.3f }, { "echoDiv", 0 }, { "echoFb", 0.5f }, { "width", 0.6f } }, "Robot leggero filtrato con eco veloce a 1/8: hacker in rete." },
        { "ANDROIDE", "CYBER", { { "robot", 1.0f }, { "drive", 0.15f }, { "verb", 0.15f } }, "Robot pieno, voce sintetica e dritta." },
        { "OLOGRAMMA", "CYBER", { { "pitch", 7 }, { "robot", 0.25f }, { "swell", 0.7f }, { "verb", 0.5f }, { "size", 0.9f }, { "width", 0.9f } }, "Alta, eterea e larghissima: un ologramma che parla." },
        { "J-J-JACKPOT", "STUTTER", { { "stutter", 3 }, { "echo", 0.25f }, { "echoDiv", 2 }, { "verb", 0.2f } }, "La prima sillaba ripetuta: il classico tag balbettato." },
        { "MITRAGLIA", "STUTTER", { { "stutter", 6 }, { "drive", 0.3f }, { "echo", 0.15f }, { "echoDiv", 0 } }, "Raffica di ripetizioni veloci." },
        { "SCRATCH", "STUTTER", { { "stutter", 2 }, { "reverse", 1 }, { "tape", 2 }, { "drive", 0.2f } }, "Al contrario e balbettato, come uno scratch." },
        { "INDIETRO", "STUTTER", { { "reverse", 1 }, { "verb", 0.35f }, { "size", 0.7f }, { "width", 0.5f } }, "La frase al contrario nella stanza: entra risucchiata." },
        { "LANCIO", "ECO", { { "echo", 0.6f }, { "echoDiv", 3 }, { "echoFb", 0.6f }, { "verb", 0.3f }, { "width", 0.5f } }, "Eco lunga che rimbalza: per lanciare il beat." },
        { "TRAP STADIO", "ECO", { { "drive", 0.2f }, { "echo", 0.3f }, { "echoDiv", 2 }, { "verb", 0.3f }, { "size", 0.85f }, { "width", 0.8f } }, "Grande, largo e con eco: tag da palco." },
        { "VOCINA", "ECO", { { "pitch", 9 }, { "echo", 0.2f }, { "echoDiv", 0 }, { "width", 0.3f } }, "Voce alta e veloce con eco a 1/8: il tag simpatico." },
        { "CANYON", "ECO", { { "echo", 0.75f }, { "echoDiv", 4 }, { "echoFb", 0.7f }, { "verb", 0.2f }, { "width", 0.6f } }, "Eco a 1/2 che non finisce piu'." },
        { "PING PONG", "ECO", { { "echo", 0.45f }, { "echoDiv", 1 }, { "echoFb", 0.5f }, { "width", 1.0f } }, "Eco larga che salta tra destra e sinistra." },
        { "ACCHIAPPA", "ECO", { { "swell", 0.5f }, { "echo", 0.5f }, { "echoDiv", 2 }, { "echoFb", 0.65f }, { "verb", 0.4f }, { "size", 0.9f } }, "Entra piano e poi si allunga: per i tag in apertura." },
    };
    return p;
}

juce::AudioProcessorValueTreeState::ParameterLayout Processor::layout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    auto F = [&l] (const char* id, const char* n, NormalisableRange<float> r, float d) { l.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, n, r, d)); };
    // di partenza tutto neutro: l'ingresso passa com'e' e non c'e' nessun tag
    F ("start", "Inizio frase", { 0, 1, 0.0001f }, 0); F ("end", "Fine frase", { 0, 1, 0.0001f }, 1);
    F ("pitch", "Pitch", { -12, 12, 1 }, 0); F ("tape", "Nastro", { -12, 12, 0.1f }, 0);
    F ("robot", "Robot", { 0, 1 }, 0); F ("radio", "Radio", { 0, 1 }, 0); F ("drive", "Drive", { 0, 1 }, 0); F ("crush", "Crush", { 0, 1 }, 0);
    l.add (std::make_unique<AudioParameterInt> (ParameterID { "stutter", 1 }, "Stutter", 0, 8, 0));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "reverse", 1 }, "Al contrario", false));
    F ("swell", "Swell", { 0, 1 }, 0);
    F ("echo", "Eco", { 0, 1 }, 0); F ("echoFb", "Ripetizioni", { 0, 0.9f }, 0.45f);
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "echoDiv", 1 }, "Tempo eco", StringArray { "1/8", "1/8.", "1/4", "1/4.", "1/2" }, 2));
    F ("verb", "Riverbero", { 0, 1 }, 0); F ("size", "Stanza", { 0, 1 }, 0.6f); F ("width", "Larghezza", { 0, 1 }, 0);
    F ("tag", "Volume tag", { -24, 6, 0.01f }, 0); F ("dry", "Ingresso", { -60, 0, 0.01f }, 0);
    l.add (std::make_unique<AudioParameterInt> (ParameterID { "note", 1 }, "Nota", -1, 127, -1));
    return l;
}
Processor::Processor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true).withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      juce::Thread ("TagForge"), apvts (*this, &undo, "PARAMS", layout())
{
    recBuf.setSize (2, 48000 * 13); recBuf.clear();
    startThread (juce::Thread::Priority::low);
}
Processor::~Processor() { signalThreadShouldExit(); notify(); stopThread (3000); }
bool Processor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto o = l.getMainOutputChannelSet(), i = l.getMainInputChannelSet();
    return o == juce::AudioChannelSet::stereo() && (i == o || i == juce::AudioChannelSet::mono());
}
void Processor::prepareToPlay (double s, int)
{
    const bool changed = std::abs (s - sr) > 1;
    sr = s;
    if (recBuf.getNumSamples() < (int) (s * 12.5)) recBuf.setSize (2, (int) (s * 12.5) + 16);
    tagGain.reset (s, 0.02); dryGain.reset (s, 0.02);
    tagGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (value ("tag"))); dryGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (value ("dry"), -60.0f));
    v[0].pos = v[1].pos = -1;
    if (changed && hasSource()) requestRender();
}
Settings Processor::settings() const
{
    Settings S;
    S.start = value ("start"); S.end = value ("end"); S.pitch = value ("pitch"); S.tape = value ("tape");
    S.robot = value ("robot"); S.radio = value ("radio"); S.drive = value ("drive"); S.crush = value ("crush");
    S.stutter = juce::roundToInt (value ("stutter")); S.reverse = value ("reverse") > 0.5f; S.swell = value ("swell");
    S.echo = value ("echo"); S.echoFb = value ("echoFb"); S.echoDiv = juce::roundToInt (value ("echoDiv"));
    S.verb = value ("verb"); S.size = value ("size"); S.width = value ("width");
    S.bpm = std::round (hostBpm * 10.0) / 10.0;
    return S;
}

// ---------------------------------------------------------------------------- forgiatura (filo a parte)
void Processor::renderNow()
{
    const juce::ScopedLock rl (renderLock);
    dirty.store (false);
    const auto S = settings();
    juce::AudioBuffer<float> src; double rate;
    { const juce::ScopedLock l (srcLock); src.makeCopyOf (source); rate = sourceRate; }
    juce::AudioBuffer<float> out;
    if (src.getNumSamples() > 0)
    {
        out = forge (src, rate, S);
        if (std::abs (rate - sr) > 1 && out.getNumSamples() > 0)
        {   // la frase e' stata registrata a un'altra frequenza: si porta a quella della sessione
            juce::AudioBuffer<float> rs (2, juce::jmax (1, (int) (out.getNumSamples() * sr / rate)));
            for (int c = 0; c < 2; ++c) { const float* s = out.getReadPointer (c); float* d = rs.getWritePointer (c); for (int i = 0; i < rs.getNumSamples(); ++i) d[i] = cubic (s, out.getNumSamples(), i * rate / sr); }
            out = std::move (rs);
        }
    }
    tagSpare = std::move (out);
    { const juce::SpinLock::ScopedLockType l (tagLock); std::swap (tag, tagSpare); }
    lastSettings = S; everRendered = true;
    renderGen.fetch_add (1);
}
void Processor::run()
{
    while (! threadShouldExit())
    {
        wait (60);
        if (threadShouldExit()) break;
        // un cambio di comando (anche da automazione) fa ripartire la forgiatura
        if (! dirty.load() && hasSource() && (! everRendered || settings() != lastSettings)) dirty.store (true);
        if (dirty.load()) renderNow();
    }
}

// ---------------------------------------------------------------------------- frase
void Processor::setSource (juce::AudioBuffer<float>&& b, double rate, bool trim)
{
    juce::AudioBuffer<float> s;
    if (trim)
    {
        const auto r = voiced (b, rate);
        if (r.getLength() > 0) { s.setSize (2, r.getLength()); for (int c = 0; c < 2; ++c) s.copyFrom (c, 0, b, juce::jmin (c, b.getNumChannels() - 1), r.getStart(), r.getLength()); }
    }
    else { s.setSize (2, b.getNumSamples()); for (int c = 0; c < 2; ++c) s.copyFrom (c, 0, b, juce::jmin (c, b.getNumChannels() - 1), 0, b.getNumSamples()); }
    { const juce::ScopedLock l (srcLock); source = std::move (s); sourceRate = rate; srcLen.store (source.getNumSamples()); }
    if (trim) { setP ("start", 0); setP ("end", 1); }
    requestRender();
}
void Processor::startRec() { recPos.store (0); recOn.store (true); }
void Processor::stopRec()
{
    if (! recOn.exchange (false)) return;
    const int n = recPos.load(); if (n < (int) (sr * 0.05)) return;
    juce::AudioBuffer<float> b (2, n); for (int c = 0; c < 2; ++c) b.copyFrom (c, 0, recBuf, c, 0, n);
    setSource (std::move (b), sr, true);
}
bool Processor::loadFile (const juce::File& f)
{
    juce::AudioFormatManager fm; fm.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> r (fm.createReaderFor (f));
    if (r == nullptr) return false;
    const int n = (int) std::min<juce::int64> (r->lengthInSamples, (juce::int64) (r->sampleRate * 12));
    if (n < 64) return false;
    juce::AudioBuffer<float> b (2, n); r->read (&b, 0, n, 0, true, true);
    setSource (std::move (b), r->sampleRate, true);
    return true;
}
void Processor::waveSnapshot (std::vector<float>& s, std::vector<float>& t, int points, float& tagSec, float& srcSec)
{
    auto env = [points] (const juce::AudioBuffer<float>& b, std::vector<float>& out)
    {
        out.assign ((size_t) points, 0.0f); const int n = b.getNumSamples(); if (n == 0) return;
        for (int p = 0; p < points; ++p)
        {
            const int a = (int) ((juce::int64) p * n / points), z = juce::jmax (a + 1, (int) ((juce::int64) (p + 1) * n / points));
            float m = 0; for (int c = 0; c < b.getNumChannels(); ++c) m = juce::jmax (m, b.getMagnitude (c, a, juce::jmin (z, n) - a)); out[(size_t) p] = m;
        }
    };
    { const juce::ScopedLock l (srcLock); env (source, s); srcSec = (float) (source.getNumSamples() / sourceRate); }
    { const juce::SpinLock::ScopedLockType l (tagLock); env (tag, t); tagSec = (float) (tag.getNumSamples() / sr); }
}
juce::File Processor::exportTag()
{
    juce::AudioBuffer<float> b; { const juce::SpinLock::ScopedLockType l (tagLock); b.makeCopyOf (tag); }
    if (b.getNumSamples() == 0) return {};
    b.applyGain (juce::Decibels::decibelsToGain (value ("tag")));
    auto name = currentName().replaceCharacters ("/\\:*?\"<>|", "_________");
    auto f = userFolder().getChildFile ("Tag " + name + " " + juce::Time::getCurrentTime().formatted ("%d-%m %H.%M.%S") + ".wav");
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::OutputStream> os (f.createOutputStream());
    if (os == nullptr) return {};
    std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (os.get(), sr, 2, 24, {}, 0));
    if (w == nullptr) return {};
    os.release(); w->writeFromAudioSampleBuffer (b, 0, b.getNumSamples()); w.reset();
    lastExport = f; return f;
}

// ---------------------------------------------------------------------------- suono
void Processor::trigger (float vel)
{
    if (v[cur].pos >= 0) { const int o = 1 - cur; v[o] = v[cur]; v[o].fadeStep = 1.0f / (float) (0.01 * sr); }
    v[cur].pos = 0; v[cur].gain = vel; v[cur].fade = 1; v[cur].fadeStep = 0;
}
void Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals nd;
    const int n = buffer.getNumSamples(); if (n == 0) return;
    if (getTotalNumInputChannels() == 1) buffer.copyFrom (1, 0, buffer, 0, 0, n);
    if (auto* ph = getPlayHead()) if (auto pos = ph->getPosition()) if (auto b = pos->getBpm()) if (*b > 20 && *b < 400) hostBpm = *b;
    // registrazione della frase (prima del volume d'ingresso)
    if (recOn.load())
    {
        const int p = recPos.load(), room = recBuf.getNumSamples() - p, k = juce::jmin (room, n);
        for (int c = 0; c < 2; ++c) recBuf.copyFrom (c, p, buffer, c, 0, k);
        recPos.store (p + k);   // pieno: la frase la chiude l'interfaccia
    }
    dryGain.setTargetValue (value ("dry") <= -59.9f ? 0.0f : juce::Decibels::decibelsToGain (value ("dry")));
    tagGain.setTargetValue (juce::Decibels::decibelsToGain (value ("tag")));
    float* L = buffer.getWritePointer (0); float* R = buffer.getWritePointer (1);
    for (int i = 0; i < n; ++i) { const float g = dryGain.getNextValue(); L[i] *= g; R[i] *= g; }
    // note che fanno partire il tag
    const int want = juce::roundToInt (value ("note"));
    int evPos[64]; float evVel[64]; int ne = 0;
    if (uiTrig.exchange (false)) { evPos[ne] = 0; evVel[ne++] = 1.0f; }
    for (const auto m : midi)
    {
        const auto msg = m.getMessage();
        if (msg.isNoteOn() && (want < 0 || msg.getNoteNumber() == want) && ne < 64) { evPos[ne] = juce::jlimit (0, n - 1, m.samplePosition); evVel[ne++] = 0.35f + 0.65f * msg.getFloatVelocity(); }
    }
    const juce::SpinLock::ScopedTryLockType lock (tagLock);
    if (! lock.isLocked()) return;   // il tag sta cambiando proprio ora: un blocco senza (rarissimo)
    const int T = tag.getNumSamples();
    float pk = 0; int ev = 0;
    const float* tl = T > 0 ? tag.getReadPointer (0) : nullptr; const float* tr = T > 0 ? tag.getReadPointer (1) : nullptr;
    for (int i = 0; i < n; ++i)
    {
        while (ev < ne && evPos[ev] <= i) { if (T > 0) trigger (evVel[ev]); ++ev; }
        const float g = tagGain.getNextValue();
        float ol = 0, orr = 0;
        for (auto& vv : v)
        {
            if (vv.pos < 0) continue;
            if (vv.pos >= T) { vv.pos = -1; continue; }
            const float a = vv.gain * vv.fade * g; ol += tl[vv.pos] * a; orr += tr[vv.pos] * a; ++vv.pos;
            if (vv.fadeStep > 0) { vv.fade -= vv.fadeStep; if (vv.fade <= 0) vv.pos = -1; }
        }
        L[i] += ol; R[i] += orr; pk = juce::jmax (pk, std::abs (ol), std::abs (orr));
    }
    uiLevel.store (juce::jmax (pk, uiLevel.load() * 0.85f));
    uiPlayPos.store (v[cur].pos >= 0 && T > 0 ? (float) v[cur].pos / (float) T : -1.0f);
}

// ---------------------------------------------------------------------------- preset e stato
void Processor::loadPreset (int i)
{
    if (! juce::isPositiveAndBelow (i, (int) presets().size())) return;
    undo.beginNewTransaction();
    // solo il carattere del tag: frase, taglio, volumi e nota restano
    for (auto* id : { "pitch", "tape", "robot", "radio", "drive", "crush", "stutter", "reverse", "swell", "echo", "echoFb", "echoDiv", "verb", "size", "width" })
        if (auto* p = apvts.getParameter (id)) p->setValueNotifyingHost (p->getDefaultValue());
    for (auto& kv : presets()[(size_t) i].v) setP (kv.first, kv.second);
    preset = i; userName.clear();
}
bool Processor::saveUserPreset (const juce::String& name)
{
    juce::XmlElement x ("TAGPRESET"); x.setAttribute ("name", name);
    for (auto* id : { "pitch", "tape", "robot", "radio", "drive", "crush", "stutter", "reverse", "swell", "echo", "echoFb", "echoDiv", "verb", "size", "width" }) x.setAttribute (id, value (id));
    const bool ok = x.writeTo (presetFolder().getChildFile (juce::File::createLegalFileName (name) + ".tagforge"));
    if (ok) userName = name;
    return ok;
}
bool Processor::loadUserPreset (const juce::File& f)
{
    auto x = juce::XmlDocument::parse (f); if (x == nullptr || ! x->hasTagName ("TAGPRESET")) return false;
    undo.beginNewTransaction();
    for (auto* id : { "pitch", "tape", "robot", "radio", "drive", "crush", "stutter", "reverse", "swell", "echo", "echoFb", "echoDiv", "verb", "size", "width" })
        if (x->hasAttribute (id)) setP (id, (float) x->getDoubleAttribute (id));
    userName = x->getStringAttribute ("name", f.getFileNameWithoutExtension());
    return true;
}
void Processor::getStateInformation (juce::MemoryBlock& d)
{
    auto st = apvts.copyState(); std::unique_ptr<juce::XmlElement> x (st.createXml());
    x->setAttribute ("preset", preset); x->setAttribute ("user", userName);
    juce::AudioBuffer<float> s; double rate;
    { const juce::ScopedLock l (srcLock); s.makeCopyOf (source); rate = sourceRate; }
    if (s.getNumSamples() > 0)
    {   // la frase viaggia col progetto (FLAC, senza perdita)
        juce::MemoryBlock mb; juce::FlacAudioFormat flac;
        if (std::unique_ptr<juce::AudioFormatWriter> w { flac.createWriterFor (new juce::MemoryOutputStream (mb, false), rate, 2, 24, {}, 0) }) { w->writeFromAudioSampleBuffer (s, 0, s.getNumSamples()); }
        auto* e = x->createNewChildElement ("SRC"); e->setAttribute ("rate", rate); e->setAttribute ("data", mb.toBase64Encoding());
    }
    copyXmlToBinary (*x, d);
}
void Processor::setStateInformation (const void* data, int size)
{
    std::unique_ptr<juce::XmlElement> x (getXmlFromBinary (data, size));
    if (x == nullptr || ! x->hasTagName (apvts.state.getType())) return;
    preset = x->getIntAttribute ("preset", 0); userName = x->getStringAttribute ("user");
    juce::AudioBuffer<float> b; double rate = 48000;
    if (auto* e = x->getChildByName ("SRC"))
    {
        rate = e->getDoubleAttribute ("rate", 48000);
        juce::MemoryBlock mb; mb.fromBase64Encoding (e->getStringAttribute ("data"));
        juce::FlacAudioFormat flac;
        if (std::unique_ptr<juce::AudioFormatReader> r { flac.createReaderFor (new juce::MemoryInputStream (mb, true), true) })
        { b.setSize (2, (int) r->lengthInSamples); r->read (&b, 0, (int) r->lengthInSamples, 0, true, true); }
        x->removeChildElement (e, true);
    }
    apvts.replaceState (juce::ValueTree::fromXml (*x));
    { const juce::ScopedLock l (srcLock); source = std::move (b); sourceRate = rate; srcLen.store (source.getNumSamples()); }
    renderNow();   // subito: anche un export offline deve trovare il tag pronto
}
juce::AudioProcessorEditor* Processor::createEditor() { return new Editor (*this); }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new jtag::Processor(); }
