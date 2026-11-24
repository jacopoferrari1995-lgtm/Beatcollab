#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "DnaAnalysis.h"

namespace dna
{
struct Kit { juce::AudioBuffer<float> audio; double sr = 44100; std::vector<int> starts; juce::String name; };

class Processor : public juce::AudioProcessor, private juce::AsyncUpdater, private juce::AudioProcessorValueTreeState::Listener, private juce::Timer
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
    const juce::String getName() const override { return "Jackpot Sample DNA"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }
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

    bool loadFile (const juce::File&);               // legge e analizza (in background)
    void loadBuffer (juce::AudioBuffer<float>&& b, double sr, const juce::String& name);
    void rechop();                                   // rifa i tagli con modo / numero / variante
    void nextVariant() { ++variant; rechop(); }
    void halveBpm(); void doubleBpm();
    juce::File writeMidi();                          // il sample ricostruito come MIDI (una nota per slice)
    std::shared_ptr<const Kit> kit() const { return std::atomic_load (&kitPtr); }
    std::shared_ptr<const Analysis> analysis() const { return std::atomic_load (&anPtr); }
    std::atomic<bool> busy { false }; juce::String path, status;
    std::atomic<int> preview { -1 }, variant { 0 };
    std::atomic<int> playing[32];                    // slice che suonano (per la grafica)
    std::atomic<float> playPos { -1 };               // posizione dell'ultima voce (in campioni del file)

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    void handleAsyncUpdate() override { rechop(); }
    void parameterChanged (const juce::String& id, float) override { if (id == "chop" || id == "count") triggerAsyncUpdate(); if (id == "sens") reanalyse = true, triggerAsyncUpdate(); }
    std::shared_ptr<const Kit> kitPtr; std::shared_ptr<const Analysis> anPtr;
    // i kit sostituiti restano qui finche' l'audio non li usa piu': cosi' la memoria non si libera mai nel thread audio
    std::vector<std::shared_ptr<const Kit>> retired; juce::CriticalSection retiredLock;
    void publish (std::shared_ptr<const Kit> k) { auto old = std::atomic_exchange (&kitPtr, std::move (k)); if (old != nullptr) { const juce::ScopedLock l (retiredLock); retired.push_back (std::move (old)); } }
    void timerCallback() override { const juce::ScopedLock l (retiredLock); retired.erase (std::remove_if (retired.begin(), retired.end(), [] (const std::shared_ptr<const Kit>& k) { return k.use_count() == 1; }), retired.end()); }
    float gainS = 1; std::shared_ptr<const Kit> audioKit;
    std::shared_ptr<juce::AudioBuffer<float>> raw; double rawSr = 44100; bool reanalyse = false;
    struct Voice { bool on = false; int slice = 0, note = -1; double pos = 0, end = 0, rate = 1; float env = 0, vel = 1; int stage = 0; };
    Voice voices[16]; double sr = 48000;
    void startVoice (int slice, int note, float vel);
    juce::AudioFormatManager fm;
};
}
