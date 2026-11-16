#pragma once
#include "FluxEngine.h"

namespace flux
{
struct Mem { bool used = false; juce::uint32 seed = 1; Macros m; std::vector<Rule> rules; std::array<bool, NL> mute {}; };
struct Section { Macros m; std::array<bool, NL> mute {}; };

class Processor : public juce::AudioProcessor, private juce::AsyncUpdater, private juce::AudioProcessorValueTreeState::Listener
{
public:
    Processor();
    ~Processor() override;
    void prepareToPlay (double, int) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& l) const override { return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo(); }
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Jackpot Flux"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    double getTailLengthSeconds() const override { return 0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;
    float v (const char* id) const { return apvts.getRawParameterValue (id)->load(); }
    void setV (const char* id, float real) { if (auto* p = apvts.getParameter (id)) p->setValueNotifyingHost (p->convertTo0to1 (real)); }
    Macros macros() const { return { v ("density"), v ("energy"), v ("movement"), v ("space") }; }

    std::shared_ptr<const Snap> snap() const { return std::atomic_load (&snapPtr); }
    void edit (const std::function<void (Snap&)>& fn) { auto s = std::make_shared<Snap> (*snap()); fn (*s); std::atomic_store (&snapPtr, std::shared_ptr<const Snap> (s)); }
    void loadStarter (int s, int key, bool minor);
    bool importLane (int lane, const juce::File& f);
    juce::File exportMidi (int bars = 8);
    void variate() { seed = hsh (seed.load(), (juce::uint32) juce::Time::getMillisecondCounter()) | 1u; }
    void storeMem (int i); void recallMem (int i);
    void selectSection (int s);              // dall'interfaccia: subito
    std::atomic<juce::uint32> seed { 1 }; std::atomic<bool> abOriginal { false };
    std::array<Mem, 8> mem; std::array<Section, NSEC> sections; int starter = TRAP;
    std::atomic<double> uiPpq { 0 }; std::atomic<bool> uiPlaying { false }; std::atomic<double> uiBpm { 120 };
    std::atomic<int> uiHits[NL];

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    void handleAsyncUpdate() override;
    void parameterChanged (const juce::String& id, float) override;
    std::shared_ptr<const Snap> snapPtr;
    double sr = 48000, ownPpq = 0, lastEnd = -1; int lastSection = -1; bool loadingSec = false;
    BarEvents cache[2]; int cacheBar[2] { -9999, -9999 }; juce::uint32 cacheKey[2] {};
    struct Pending { double end; int ch, note; }; Pending pend[512]; int np = 0;
    std::atomic<bool> allOff { false };
};
}
