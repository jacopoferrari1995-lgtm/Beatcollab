#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "BassGen.h"
#include "BassSynth.h"

namespace bss
{
struct BPreset { const char* name; const char* note; std::vector<std::pair<const char*, float>> p; std::vector<int> prog; };
const std::vector<BPreset>& presets();
juce::File presetFolder();
inline int barsOf (int idx) { static const int b[4] = { 1, 2, 4, 8 }; return b[juce::jlimit (0, 3, idx)]; }

class Processor : public juce::AudioProcessor, private juce::AsyncUpdater, private juce::AudioProcessorValueTreeState::Listener, private juce::ValueTree::Listener
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
    const juce::String getName() const override { return "Jackpot Bass"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    double getTailLengthSeconds() const override { return 2.0; }
    int getNumPrograms() override { return (int) presets().size(); }
    int getCurrentProgram() override { return juce::jmax (0, curPreset); }
    void setCurrentProgram (int i) override { loadPreset (i); }
    const juce::String getProgramName (int i) override { return presets()[(size_t) juce::jlimit (0, (int) presets().size() - 1, i)].name; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::UndoManager undo { 400000, 80 };
    juce::AudioProcessorValueTreeState apvts;
    float v (const char* id) const { return apvts.getRawParameterValue (id)->load(); }
    int vi (const char* id) const { return juce::roundToInt (v (id)); }
    void setV (const char* id, float real) { if (auto* p = apvts.getParameter (id)) { p->beginChangeGesture(); p->setValueNotifyingHost (p->convertTo0to1 (real)); p->endChangeGesture(); } }

    std::vector<int> prog() const;
    void setProg (const std::vector<int>& p);
    juce::uint32 seed() const { return (juce::uint32) (int) apvts.state.getProperty ("seed", 1); }
    void setSeed (juce::uint32 s) { apvts.state.setProperty ("seed", (int) (s & 0x7fffffff), &undo); }
    void generaGiro(); void varia();
    void applyStyle (int style);
    void setFigSeq (const juce::String& f) { apvts.state.setProperty ("figSeq", f, &undo); setV ("fig", (float) kBLibera); }
    std::array<juce::uint16, 5> grid() const; void setGrid (const std::array<juce::uint16, 5>& g);
    BSettings settings() const;
    juce::String settingsKey() const;
    void regenerate();
    std::shared_ptr<const Part> part() const { return std::atomic_load (&partPtr); }
    void commitEdit (const Part&); void clearEdit();
    bool edited() const { return apvts.state.getProperty ("edit").toString().isNotEmpty() && apvts.state.getProperty ("editKey").toString() == settingsKey(); }
    juce::File exportMidi(); juce::String clipName() const;

    void loadPreset (int i);
    bool savePresetFile (const juce::File&); bool loadPresetFile (const juce::File&);
    juce::String presetName { "Init" }; int curPreset = 0;
    std::atomic<double> uiBeat { -1 }, uiBpm { 120 }; std::atomic<bool> uiPlaying { false }; std::atomic<int> uiSlot { -1 };
    BassVoice syn; std::atomic<bool> allOffReq { false };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    void handleAsyncUpdate() override { regenerate(); }
    void parameterChanged (const juce::String&, float) override { triggerAsyncUpdate(); }
    void valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier&) override { triggerAsyncUpdate(); }
    std::shared_ptr<const Part> partPtr;
    double sr = 48000, ownPpq = 0, lastEnd = -1;
    struct Pend { double end; int note; }; std::array<Pend, 64> pend {}; int np = 0; int held = 0;
    struct Act { int at, type, note; float vel; bool legato, mute; };
    std::vector<Act> acts; juce::MidiBuffer out;
};
}
