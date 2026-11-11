#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_dsp/juce_dsp.h>
#include "JackpotKit.h"

namespace jrwd
{
// Registratore retroattivo: tiene sempre in memoria gli ultimi minuti di quello che passa sulla traccia.
// Niente si perde: selezioni un pezzo, lo ascolti e lo trascini in Ableton come WAV.
class Processor : public juce::AudioProcessor
{
public:
    Processor();
    void prepareToPlay (double, int) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& l) const override
    { const auto o = l.getMainOutputChannelSet(), i = l.getMainInputChannelSet(); return o == juce::AudioChannelSet::stereo() && (i == o || i == juce::AudioChannelSet::mono()); }
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Jackpot Rewind"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Rewind"; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;
    float value (const char* id) const { return apvts.getRawParameterValue (id)->load(); }

    // ---- memoria
    static constexpr double kColSec = 0.05;                  // una colonna della panoramica = 50 ms
    struct Col { std::atomic<float> pk { 0 }, rms { 0 }; std::atomic<double> ppq { -1 }, bpm { 0 }; std::atomic<int> num { 4 }; std::atomic<juce::int64> abs { -1 }; };
    juce::int64 capacity() const { return cap; }                // campioni per canale
    juce::int64 written() const { return wr.load(); }           // campioni scritti dall'inizio
    juce::int64 oldest() const { return juce::jmax ((juce::int64) 0, wr.load() - cap); }
    int colLen() const { return colN; }
    int numCols() const { return nCols; }
    const Col& col (juce::int64 absCol) const { return cols[(size_t) (absCol % nCols)]; }
    double sampleRate() const { return sr; }
    void readRange (juce::int64 a, juce::int64 b, juce::AudioBuffer<float>& out) const;   // [a,b) in campioni assoluti
    // posizione musicale di un campione (se l'host suonava): ppq, bpm, battuta
    bool musicalAt (juce::int64 s, double& ppq, double& bpm, int& num) const;
    juce::int64 snapToBar (juce::int64 s, int mode = 0) const;   // -1 giu, 0 piu vicina, +1 su

    // ---- segnalibri
    std::vector<juce::int64> marks; juce::CriticalSection marksLock;
    void addMark() { const juce::ScopedLock l (marksLock); marks.push_back (wr.load()); }

    // ---- ascolto della selezione
    void preview (juce::int64 a, juce::int64 b, bool loop) { pvA = a; pvB = b; pvLoop = loop; pvPos = a; pvOn = true; }
    void stopPreview() { pvOn = false; }
    std::atomic<bool> pvOn { false }; std::atomic<juce::int64> pvPos { 0 };

    // ---- blocco: smette di registrare per guardare con calma
    std::atomic<bool> hold { false };

    // ---- esportazione
    juce::File exportRange (juce::int64 a, juce::int64 b, bool normalize, bool reverse, juce::String& info);
    static juce::File folder();
    juce::String keyOf (const juce::AudioBuffer<float>&) const;

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<float> inPk { 0 };
private:
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    double sr = 44100; juce::int64 cap = 0; std::vector<juce::int16> ring; std::atomic<juce::int64> wr { 0 };
    std::vector<Col> cols; int nCols = 0, colN = 2205; juce::int64 colAt = 0; float cPk = 0; double cSq = 0;
    juce::int64 pvA = 0, pvB = 0; bool pvLoop = false;
};
}
