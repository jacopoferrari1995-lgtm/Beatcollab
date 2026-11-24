#include "ButterProcessor.h"
#include "ButterEditor.h"

namespace jb
{
juce::AudioProcessorValueTreeState::ParameterLayout Processor::layout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    Burro::addParams (l);
    l.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "u_in", 1 }, "Input", juce::NormalisableRange<float> (-18, 18), 0));
    l.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "u_out", 1 }, "Output", juce::NormalisableRange<float> (-24, 12), 0));
    return l;
}
Processor::Processor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true).withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", layout())
{
    for (auto& l : locked) l = false;
    dsp.bind (apvts); pIn = apvts.getRawParameterValue ("u_in"); pOut = apvts.getRawParameterValue ("u_out");
}

juce::StringArray Processor::moduleParams (int m)
{
    switch (m)
    {
        case 0: return { "u_bOn", "u_boom", "u_bTone", "u_acid", "u_noise", "u_nType" };
        case 1: return { "u_shOn", "u_shAmt", "u_shPitch", "u_shTgt", "u_shFoc" };
        case 2: return { "u_cOn", "u_cType", "u_cAmt", "u_cBands" };
        case 3: return { "u_sOn", "u_sType", "u_sAmt", "u_sStyle", "u_sTgt", "u_sFoc" };
        case 4: return { "u_spOn", "u_spType", "u_spAmt", "u_spDecay", "u_spPre", "u_spDuck", "u_spWidth" };
        case 6: return { "u_xOn", "u_xType", "u_xAmt", "u_xP1", "u_xP2", "u_xTgt" };
        default: return { "u_mOn", "u_mAmt", "u_mStages", "u_mTone", "u_mTgt", "u_mFoc" };
    }
}

void Processor::loadPreset (int i)
{
    const auto& all = burroPresets(); if (i < 0 || i >= (int) all.size()) return;
    preset = i; userName = {};
    juce::StringArray keep; for (int m = 0; m < 7; ++m) if (locked[m]) keep.addArray (moduleParams (m));
    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            if (rp->getParameterID().startsWith ("u_") && ! keep.contains (rp->getParameterID()) && rp->getParameterID() != "u_in" && rp->getParameterID() != "u_out")
                rp->setValueNotifyingHost (rp->getDefaultValue());
    AeSettings ae; bool hasAe = false;
    for (auto& [id, v] : all[(size_t) i].p)
    {
        const juce::String sid (id);
        if (sid.startsWith ("u_ae"))
        {   // preset scritti per il vecchio EQ analogico: diventano bande dell'EQ nuovo
            hasAe = true;
            if (sid == "u_aeModel") ae.model = (int) v; else if (sid == "u_aeQ") ae.q = v; else if (sid == "u_aeDrive") ae.drive = v;
            else { const int k = sid.substring (4, 5).getIntValue() - 1; if (k >= 0 && k < 4) { if (sid.endsWith ("g")) ae.g[k] = v; else ae.f[k] = v; } }
            continue;
        }
        if (! keep.contains (id)) if (auto* rp = apvts.getParameter (id)) rp->setValueNotifyingHost (rp->convertTo0to1 (v));
    }
    if (hasAe) aeToBands (ae, [this] (const char* id, float v) { if (auto* rp = apvts.getParameter (id)) rp->setValueNotifyingHost (rp->convertTo0to1 (v)); });
}

void Processor::safety (float* l, float* r, int n, float inPk)
{
    // 1) un valore rotto (NaN/inf) o un'uscita enorme sul silenzio per mezzo secondo: muto, e lo stato si ricostruisce fuori dall'audio
    bool broken = false; float pk = 0;
    for (int i = 0; i < n; ++i) { if (! std::isfinite (l[i]) || ! std::isfinite (r[i])) { broken = true; break; } pk = juce::jmax (pk, std::abs (l[i]), std::abs (r[i])); }
    if (! broken && pk > 1.4f && inPk < 0.05f) { runaway += n; if (runaway > (int) (curSr * 0.5)) broken = true; } else runaway = juce::jmax (0, runaway - n);
    if (broken) { ++panics; runaway = 0; muteLeft = (int) (curSr * 0.15); fadeIn = 0; if (bbReason.load() == 0) bbReason = 1; bbFreeze = true; triggerAsyncUpdate(); }
    if (muteLeft > 0) { const int m = juce::jmin (n, muteLeft); std::fill (l, l + m, 0.0f); std::fill (r, r + m, 0.0f); muteLeft -= m; if (m == n) return; }
    // 2) tetto: sotto +3 dBFS non tocca niente, poi curva morbida fino a +6 dBFS che non si supera mai
    constexpr float k = 1.4125f, c = 1.9953f;
    const float fs = 1.0f / (float) (0.05 * curSr);
    for (int i = 0; i < n; ++i)
    {
        if (fadeIn < 1.0f) { fadeIn = juce::jmin (1.0f, fadeIn + fs); l[i] *= fadeIn; r[i] *= fadeIn; }
        for (float* x : { l + i, r + i }) { const float a = std::abs (*x); if (a > k) *x = std::copysign (k + (c - k) * std::tanh ((a - k) / (c - k)), *x); }
    }
}
void Processor::processBlock (juce::AudioBuffer<float>& b, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals nd;
    const int N = b.getNumSamples(); if (b.getNumChannels() < 2) return;
    float* L = b.getWritePointer (0); float* R = b.getWritePointer (1);
    gIn.setTargetValue (juce::Decibels::decibelsToGain (pIn->load())); gOut.setTargetValue (juce::Decibels::decibelsToGain (pOut->load()));
    if (auto* ph = getPlayHead()) if (auto pos = ph->getPosition()) if (auto bp = pos->getBpm()) dsp.bpm = *bp;
    // a pezzi grandi al massimo quanto preparato: nessun buffer da ingrandire mentre suona
    for (int o = 0; o < N; o += maxBlock)
    {
        const int n = juce::jmin (maxBlock, N - o); float* l = L + o; float* r = R + o;
        float p0 = 0, p1 = 0;
        for (int i = 0; i < n; ++i) { if (! std::isfinite (l[i])) l[i] = 0; if (! std::isfinite (r[i])) r[i] = 0; }   // un NaN da un plugin prima non entra
        for (int i = 0; i < n; ++i) { const float g = gIn.getNextValue(); l[i] *= g; r[i] *= g; p0 = juce::jmax (p0, std::abs (l[i])); p1 = juce::jmax (p1, std::abs (r[i])); }
        inL = juce::jmax (p0, inL.load() * 0.85f); inR = juce::jmax (p1, inR.load() * 0.85f);
        double eI = 0; for (int i = 0; i < n; ++i) eI += (double) l[i] * l[i] + (double) r[i] * r[i];
        if (! bbFreeze.load()) for (int i = 0; i < n; ++i) { bb[0][(size_t) bbW] = l[i]; bb[1][(size_t) bbW] = r[i]; if (++bbW >= bbLen) bbW = 0; }
        dsp.process (l, r, n, apvts);
        {   // il suono che impazzisce mentre la musica e' normale: uscita forte e 20 dB sopra l'ingresso per piu' di 30 ms
            double eO = 0; float pk = 0; int zc = 0;
            for (int i = 0; i < n; ++i) { eO += (double) l[i] * l[i] + (double) r[i] * r[i]; pk = juce::jmax (pk, std::abs (l[i]), std::abs (r[i])); if (i > 0 && (l[i] >= 0) != (l[i - 1] >= 0)) ++zc; }
            const double rI = std::sqrt (eI / (2.0 * n)), rO = std::sqrt (eO / (2.0 * n));
            // rumore (tanti attraversamenti dello zero), forte, e 30 dB sopra quello che entra: non e' musica, e' il motore rotto
            if (pk > 0.9f && rO > rI * 30.0 + 0.1 && zc > n / 4) wild += n; else wild = juce::jmax (0, wild - n / 2);
            if (wild > (int) (curSr * 0.03)) { wild = 0; bbReason = 2; for (int i = 0; i < n; ++i) l[i] = r[i] = std::numeric_limits<float>::quiet_NaN(); }   // la protezione qui sotto spegne e ricostruisce
        }
        p0 = p1 = 0;
        const float inPk = juce::jmax (inL.load(), inR.load());
        for (int i = 0; i < n; ++i) { const float g = gOut.getNextValue(); l[i] *= g; r[i] *= g; }
        safety (l, r, n, inPk);
        for (int i = 0; i < n; ++i) { p0 = juce::jmax (p0, std::abs (l[i])); p1 = juce::jmax (p1, std::abs (r[i])); }
        outL = juce::jmax (p0, outL.load() * 0.85f); outR = juce::jmax (p1, outR.load() * 0.85f);
    }
}

void Processor::saveBlackBox()
{
    auto dir = juce::File::getSpecialLocation (juce::File::userMusicDirectory).getChildFile ("Jackpot").getChildFile ("Butter Debug"); dir.createDirectory();
    const auto stamp = juce::Time::getCurrentTime().formatted ("%Y%m%d-%H%M%S");
    juce::AudioBuffer<float> b (2, bbLen);
    for (int i = 0; i < bbLen; ++i) { const int k = (bbW + i) % bbLen; b.setSample (0, i, bb[0][(size_t) k]); b.setSample (1, i, bb[1][(size_t) k]); }
    {
        juce::WavAudioFormat wav; auto f = dir.getChildFile ("butter-" + stamp + "-ingresso.wav");
        if (auto* os = new juce::FileOutputStream (f); os->openedOk()) { std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (os, curSr, 2, 24, {}, 0)); if (w) w->writeFromAudioSampleBuffer (b, 0, bbLen); else delete os; } else delete os;
    }
    juce::MemoryBlock mb; getStateInformation (mb);
    if (auto x = getXmlFromBinary (mb.getData(), (int) mb.getSize()))
    {
        x->setAttribute ("motivo", bbReason.load() == 2 ? "uscita impazzita con musica normale" : "valore rotto o autoalimentazione nel silenzio");
        x->setAttribute ("sampleRate", curSr); x->setAttribute ("blocco", maxBlock); 
#ifdef JucePlugin_VersionString
        x->setAttribute ("versione", JucePlugin_VersionString);
#endif
        x->writeTo (dir.getChildFile ("butter-" + stamp + "-impostazioni.xml"));
    }
    bbReason = 0; bbFreeze = false;
}

juce::File Processor::presetFolder()
{
    auto f = juce::File::getSpecialLocation (juce::File::userMusicDirectory).getChildFile ("Jackpot").getChildFile ("Butter Preset");
    f.createDirectory(); return f;
}
bool Processor::saveUserPreset (const juce::String& name)
{
    juce::MemoryBlock mb; getStateInformation (mb);
    auto x = getXmlFromBinary (mb.getData(), (int) mb.getSize()); if (! x) return false;
    const auto f = presetFolder().getChildFile (juce::File::createLegalFileName (name) + ".jbutter");
    if (! x->writeTo (f)) return false;
    userName = f.getFileNameWithoutExtension(); preset = -1; return true;
}
bool Processor::loadUserPreset (const juce::File& f)
{
    auto x = juce::XmlDocument::parse (f); if (! x) return false;
    juce::MemoryBlock mb; copyXmlToBinary (*x, mb); setStateInformation (mb.getData(), (int) mb.getSize());
    userName = f.getFileNameWithoutExtension(); preset = -1; return true;
}

void Processor::getStateInformation (juce::MemoryBlock& d)
{
    auto st = apvts.copyState();
    juce::String ord, lk; for (auto& o : dsp.order) ord << o.load() << ","; for (auto& l : locked) lk << (l.load() ? "1" : "0");
    st.setProperty ("order", ord, nullptr); st.setProperty ("locks", lk, nullptr); st.setProperty ("preset", preset, nullptr); st.setProperty ("bv", 3, nullptr); st.setProperty ("uname", userName, nullptr);
    if (auto x = st.createXml()) copyXmlToBinary (*x, d);
}
void Processor::setStateInformation (const void* data, int size)
{
    auto x = getXmlFromBinary (data, size); if (x == nullptr) return;
    auto st = juce::ValueTree::fromXml (*x); if (! st.isValid()) return;
    const bool old = (int) st.getProperty ("bv", 1) < 2;
    // sessioni salvate col vecchio EQ analogico (modelli): le sue impostazioni diventano bande dell'EQ nuovo
    AeSettings ae; bool hasAe = false;
    if ((int) st.getProperty ("bv", 1) < 3)
    {
        auto pv = [&st] (const char* id, float d) { auto c = st.getChildWithProperty ("id", id); return c.isValid() ? (float) c.getProperty ("value", d) : d; };
        ae.model = (int) pv ("u_aeModel", 2); ae.q = pv ("u_aeQ", 0.5f); ae.drive = pv ("u_aeDrive", 0);
        for (int k = 0; k < 4; ++k) { ae.g[k] = pv (("u_ae" + juce::String (k + 1) + "g").toRawUTF8(), 0); ae.f[k] = pv (("u_ae" + juce::String (k + 1) + "f").toRawUTF8(), k == 3 ? 0.0f : 0.5f); }
        hasAe = st.getChildWithProperty ("id", "u_aeModel").isValid();
    }
    apvts.replaceState (st);
    if (hasAe) aeToBands (ae, [this] (const char* id, float v) { if (auto* rp = apvts.getParameter (id)) rp->setValueNotifyingHost (rp->convertTo0to1 (v)); });
    if (old)
        if (auto* nt = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter ("u_nType")))
        {   // vinile e nastro non ci sono piu' (stanno in Dust): diventano pink; metallo e sabbia scalano di due posti
            static const int map[6] = { 0, 1, 1, 1, 2, 3 };
            const int i = (int) st.getChildWithProperty ("id", "u_nType").getProperty ("value", 1.0f);
            nt->setValueNotifyingHost (nt->convertTo0to1 ((float) map[juce::jlimit (0, 5, i)]));
        }
    userName = st.getProperty ("uname", "").toString();
    auto o = juce::StringArray::fromTokens (st.getProperty ("order").toString(), ",", "");
    if (o.size() >= 4)
    {
        int ord[5] = { 1, 2, 3, 4, 6 }; bool used[7] = {}; int n = 0;
        for (int i = 0; i < juce::jmin (5, o.size()); ++i) { const int m = o[i].getIntValue(); if ((m >= 1 && m <= 4) || m == 6) if (! used[m]) { used[m] = true; ord[n++] = m; } }
        for (int m : { 1, 2, 3, 4, 6 }) if (! used[m] && n < 5) ord[n++] = m;
        for (int i = 0; i < 5; ++i) dsp.order[i] = ord[i];
    }
    const auto lk = st.getProperty ("locks").toString(); for (int i = 0; i < 7 && i < lk.length(); ++i) locked[i] = lk[i] == '1';
    preset = st.getProperty ("preset", 0);
}

juce::AudioProcessorEditor* Processor::createEditor() { return new Editor (*this); }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new jb::Processor(); }

namespace jb
{
// Randomizzatore "musicale": accende 3-5 moduli con quantita sensate, rispetta i lucchetti
void Processor::randomize()
{
    userName = {};
    juce::Random r;
    auto set = [this] (const char* id, float v) { if (auto* rp = apvts.getParameter (id)) rp->setValueNotifyingHost (rp->convertTo0to1 (v)); };
    auto rng = [&r] (float a, float b) { return a + r.nextFloat() * (b - a); };
    int mods[] = { 0, 1, 2, 3, 4, 5, 6 }; int active = 0;
    for (int m : mods)
    {
        if (locked[m]) continue;
        const float pOn = m == 2 || m == 3 ? 0.85f : m == 0 ? 0.7f : m == 4 ? 0.6f : m == 6 ? 0.5f : 0.4f;
        const bool on = r.nextFloat() < pOn && active < 5;
        if (on) ++active;
        switch (m)
        {
            case 0: set ("u_bOn", 1); set ("u_boom", on ? rng (0.15f, 0.6f) : 0); set ("u_bTone", rng (40, 75)); set ("u_acid", r.nextFloat() < 0.2f ? 1.0f : 0.0f); set ("u_noise", r.nextFloat() < 0.2f ? rng (0, 0.2f) : 0); set ("u_nType", (float) r.nextInt (4)); break;
            case 1: set ("u_shOn", 1); set ("u_shAmt", on ? rng (0.2f, 0.7f) : 0); { const float iv[] = { -12, -7, -5, 0, 5, 7, 12 }; set ("u_shPitch", iv[r.nextInt (7)]); }; set ("u_shTgt", rng (0.2f, 1)); set ("u_shFoc", (float) r.nextInt (4)); break;
            case 2: set ("u_cOn", 1); set ("u_cType", (float) r.nextInt (6)); set ("u_cAmt", on ? rng (0.2f, 0.65f) : 0); set ("u_cBands", rng (0, 0.8f)); break;
            case 3: set ("u_sOn", 1); { const int ty[] = { 0, 1, 2, 3, 5, 6 }; set ("u_sType", (float) ty[r.nextInt (6)]); } set ("u_sAmt", on ? rng (0.15f, 0.6f) : 0); set ("u_sStyle", rng (0.2f, 0.8f)); set ("u_sTgt", rng (-0.5f, 0.6f)); set ("u_sFoc", (float) r.nextInt (4)); break;
            case 4: set ("u_spOn", 1); set ("u_spType", (float) r.nextInt (4)); set ("u_spAmt", on ? rng (0.08f, 0.35f) : 0); set ("u_spDecay", rng (0.2f, 0.7f)); set ("u_spPre", rng (0, 40)); set ("u_spDuck", rng (0.2f, 0.8f)); set ("u_spWidth", rng (0.4f, 1)); break;
            case 5: set ("u_mOn", 1); set ("u_mAmt", on ? rng (0.1f, 0.45f) : 0); set ("u_mStages", (float) r.nextInt (3)); set ("u_mTone", rng (-0.5f, 0.5f)); set ("u_mTgt", rng (-0.3f, 0.6f)); set ("u_mFoc", (float) r.nextInt (4)); break;
            case 6:
            {
                const int t = r.nextInt (3); set ("u_xOn", 1); set ("u_xType", (float) t);
                set ("u_xAmt", on ? (t == 0 ? rng (0.12f, 0.35f) : t == 1 ? rng (0.25f, 0.7f) : rng (0.3f, 0.8f)) : 0);
                set ("u_xP1", t == 0 ? (float) (r.nextInt (3) + 2) / 6.0f : rng (0.2f, 0.8f)); set ("u_xP2", t == 0 ? rng (0.2f, 0.55f) : rng (0.2f, 0.7f)); set ("u_xTgt", rng (-0.5f, 0.5f));
                break;
            }
            default: break;
        }
    }
    // bilanciamento loudness: piu moduli -> meno ingresso
    set ("u_out", -1.0f * (float) juce::jmax (0, active - 2));
}
}
