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
        default: return { "u_mOn", "u_mAmt", "u_mStages", "u_mTone", "u_mTgt", "u_mFoc" };
    }
}

void Processor::loadPreset (int i)
{
    const auto& all = burroPresets(); if (i < 0 || i >= (int) all.size()) return;
    preset = i;
    juce::StringArray keep; for (int m = 0; m < 6; ++m) if (locked[m]) keep.addArray (moduleParams (m));
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
    if (o.size() >= 4) for (int i = 0; i < 4; ++i) dsp.order[i] = juce::jlimit (1, 4, o[i].getIntValue());
    const auto lk = st.getProperty ("locks").toString(); for (int i = 0; i < 6 && i < lk.length(); ++i) locked[i] = lk[i] == '1';
    preset = st.getProperty ("preset", 0);
}

juce::AudioProcessorEditor* Processor::createEditor() { return new Editor (*this); }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new jb::Processor(); }
