#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "GtrGen.h"
#include "GtrSynth.h"

namespace gtr
{
struct GPreset { const char* name; const char* note; std::vector<std::pair<const char*, float>> p; std::vector<int> prog; };
const std::vector<GPreset>& presets();
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
    const juce::String getName() const override { return "Jackpot Guitar"; }
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

    // giro e seme: stanno nello stato (con annulla / ripeti)
    std::vector<int> prog() const;
    void setProg (const std::vector<int>& p);
    juce::uint32 seed() const { return (juce::uint32) (int) apvts.state.getProperty ("seed", 1); }
    void setSeed (juce::uint32 s) { apvts.state.setProperty ("seed", (int) (s & 0x7fffffff), &undo); }
    void generaGiro();      // nuovo giro dal genere + nuovo seme
    void varia();           // stesso giro, nuova suonata
    Settings settings() const;
    void regenerate();      // subito (thread dei messaggi o test)
    std::shared_ptr<const Part> part() const { return std::atomic_load (&partPtr); }
    juce::File exportMidi();
    juce::String clipName() const;

    void loadPreset (int i);
    bool savePresetFile (const juce::File&); bool loadPresetFile (const juce::File&);
    juce::String presetName { "Init" }; int curPreset = 0;

    std::atomic<double> uiBeat { -1 }, uiBpm { 120 }; std::atomic<bool> uiPlaying { false };
    std::atomic<int> uiSlot { -1 };
    Strings syn;
    std::atomic<bool> allOffReq { false };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    void handleAsyncUpdate() override { regenerate(); }
    void parameterChanged (const juce::String& id, float) override;
    void valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier&) override { triggerAsyncUpdate(); }
    std::shared_ptr<const Part> partPtr;
    double sr = 48000, ownPpq = 0, lastEnd = -1, genBpm = 120; int maxBlock = 512;
    struct Pend { double end; int ch, note, str; };
    std::array<Pend, 256> pend {}; int np = 0;
    std::array<int, 128> inStr {};   // corda di ogni nota che arriva da fuori
    struct Act { int at; int type; Hit h; int ch; float bend; };   // 0 nota, 1 note-off, 2 bend
    std::vector<Act> acts;
    juce::MidiBuffer out;
};
}
