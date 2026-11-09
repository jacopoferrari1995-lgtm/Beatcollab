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
    preset = i;
    juce::StringArray keep; for (int m = 0; m < 7; ++m) if (locked[m]) keep.addArray (moduleParams (m));
    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            if (rp->getParameterID().startsWith ("u_") && ! keep.contains (rp->getParameterID()) && rp->getParameterID() != "u_in" && rp->getParameterID() != "u_out")
                rp->setValueNotifyingHost (rp->getDefaultValue());
    for (auto& [id, v] : all[(size_t) i].p) if (! keep.contains (id)) if (auto* rp = apvts.getParameter (id)) rp->setValueNotifyingHost (rp->convertTo0to1 (v));
}

void Processor::processBlock (juce::AudioBuffer<float>& b, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals nd;
    const int N = b.getNumSamples(); if (b.getNumChannels() < 2) return;
    float* L = b.getWritePointer (0); float* R = b.getWritePointer (1);
    const float gi = juce::Decibels::decibelsToGain (apvts.getRawParameterValue ("u_in")->load()), go = juce::Decibels::decibelsToGain (apvts.getRawParameterValue ("u_out")->load());
    float p0 = 0, p1 = 0;
    for (int i = 0; i < N; ++i) { L[i] *= gi; R[i] *= gi; p0 = juce::jmax (p0, std::abs (L[i])); p1 = juce::jmax (p1, std::abs (R[i])); }
    inL = juce::jmax (p0, inL.load() * 0.85f); inR = juce::jmax (p1, inR.load() * 0.85f);
    if (auto* ph = getPlayHead()) if (auto pos = ph->getPosition()) if (auto bp = pos->getBpm()) dsp.bpm = *bp;
    dsp.process (L, R, N, apvts);
    p0 = p1 = 0;
    for (int i = 0; i < N; ++i) { L[i] *= go; R[i] *= go; p0 = juce::jmax (p0, std::abs (L[i])); p1 = juce::jmax (p1, std::abs (R[i])); }
    outL = juce::jmax (p0, outL.load() * 0.85f); outR = juce::jmax (p1, outR.load() * 0.85f);
}

void Processor::getStateInformation (juce::MemoryBlock& d)
{
    auto st = apvts.copyState();
    juce::String ord, lk; for (auto& o : dsp.order) ord << o.load() << ","; for (auto& l : locked) lk << (l.load() ? "1" : "0");
    st.setProperty ("order", ord, nullptr); st.setProperty ("locks", lk, nullptr); st.setProperty ("preset", preset, nullptr);
    if (auto x = st.createXml()) copyXmlToBinary (*x, d);
}
void Processor::setStateInformation (const void* data, int size)
{
    auto x = getXmlFromBinary (data, size); if (x == nullptr) return;
    auto st = juce::ValueTree::fromXml (*x); if (! st.isValid()) return;
    apvts.replaceState (st);
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
            case 0: set ("u_bOn", 1); set ("u_boom", on ? rng (0.15f, 0.6f) : 0); set ("u_bTone", rng (40, 75)); set ("u_acid", r.nextFloat() < 0.2f ? 1.0f : 0.0f); set ("u_noise", r.nextFloat() < 0.3f ? rng (0, 0.25f) : 0); set ("u_nType", (float) r.nextInt (5)); break;
            case 1: set ("u_shOn", 1); set ("u_shAmt", on ? rng (0.2f, 0.7f) : 0); { const float iv[] = { -12, -7, -5, 0, 5, 7, 12 }; set ("u_shPitch", iv[r.nextInt (7)]); }; set ("u_shTgt", rng (0.2f, 1)); set ("u_shFoc", (float) r.nextInt (4)); break;
            case 2: set ("u_cOn", 1); set ("u_cType", (float) r.nextInt (6)); set ("u_cAmt", on ? rng (0.2f, 0.65f) : 0); set ("u_cBands", rng (0, 0.8f)); break;
            case 3: set ("u_sOn", 1); set ("u_sType", (float) r.nextInt (6)); set ("u_sAmt", on ? rng (0.15f, 0.6f) : 0); set ("u_sStyle", rng (0.2f, 0.8f)); set ("u_sTgt", rng (-0.5f, 0.6f)); set ("u_sFoc", (float) r.nextInt (4)); break;
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
