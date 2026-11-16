#pragma once
#include "MrHub.h"
#include <map>

namespace mr
{
enum IssueType { KICK808 = 0, VOICEMASK, LOWEND, WIDELOW, PEAKS, LOUD, kTypes };
inline const char* typeName (int t) { static const char* n[kTypes] = { "CASSA x 808", "VOCE COPERTA", "TROPPI BASSI", "BASSI LARGHI", "PICCHI", "LOUDNESS" }; return n[juce::jlimit (0, kTypes - 1, t)]; }

struct Issue
{
    int type = 0; juce::String who, what, fix; int count = 0; std::vector<int> bars; float hz = 0; float sev = 0; juce::uint32 last = 0;
};

class Processor : public juce::AudioProcessor, private juce::Timer
{
public:
    Processor();
    ~Processor() override;
    void prepareToPlay (double, int) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& l) const override { return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo() && l.getMainInputChannelSet() == juce::AudioChannelSet::stereo(); }
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Jackpot Mix Radar"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;
    void updateTrackProperties (const TrackProperties& p) override { trackName = p.name; if (slot >= 0 && p.name.isNotEmpty()) hub().s[slot].setName (p.name); }

    juce::AudioProcessorValueTreeState apvts;
    float v (const char* id) const { return apvts.getRawParameterValue (id)->load(); }
    void setV (const char* id, float real) { if (auto* p = apvts.getParameter (id)) p->setValueNotifyingHost (p->convertTo0to1 (real)); }
    bool radar() const { return v ("mode") > 0.5f; }
    juce::String trackName; int slot = -1; const juce::int64 myId;

    // radar
    std::atomic<float> mBands[NB], lufsM { -70 }, lufsS { -70 }, mPeak { -120 }, corr { 1 }, lowMid { 0 };
    std::atomic<double> ppq { 0 }, bpm { 120 }; std::atomic<int> beatsPerBar { 4 }; std::atomic<bool> playing { false };
    juce::CriticalSection issueLock; std::map<juce::String, Issue> issues; std::map<int, int> barFlags; int lastBar = 0;
    void clearIssues() { const juce::ScopedLock l (issueLock); issues.clear(); barFlags.clear(); }
    void evaluate();                         // incrocia i sensori (chiamato dal timer)

    // sensore (anche per la grafica)
    std::atomic<float> sBands[NB];

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    void timerCallback() override;
    void addIssue (int type, const juce::String& key, const juce::String& who, const juce::String& what, const juce::String& fix, float hz, float sev, int bar);
    double sr = 48000; Bank bank; int hopN = 0, hop = 1024;
    float lpM1 = 0, lpM2 = 0, lpS1 = 0, lpS2 = 0; double eMid = 0, eSide = 0; float pk = 0; double rms = 0; float crestS = 0;
    // loudness (K-weighting) a blocchi di 100 ms
    juce::IIRFilter kw1[2], kw2[2]; double kAcc = 0; int kN = 0; std::vector<double> kBlocks; int kW = 0; double cLR = 0, cLL = 0, cRR = 0;
    int lastBeatKey[kTypes] {};
};
}
