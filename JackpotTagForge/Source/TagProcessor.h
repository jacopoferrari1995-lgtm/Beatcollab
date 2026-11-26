#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "TagDSP.h"

namespace jtag
{
struct Preset { juce::String name, cat; std::vector<std::pair<const char*, float>> v; };
const std::vector<Preset>& presets();

class Processor : public juce::AudioProcessor, private juce::Thread, private juce::AsyncUpdater
{
public:
    Processor();
    ~Processor() override;
    void prepareToPlay (double, int) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Jackpot Tag Forge"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }
    int getNumPrograms() override { return (int) presets().size(); }
    int getCurrentProgram() override { return juce::jmax (0, preset); }
    void setCurrentProgram (int i) override { loadPreset (i); }
    const juce::String getProgramName (int i) override { return juce::isPositiveAndBelow (i, (int) presets().size()) ? presets()[(size_t) i].name : ""; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    void loadPreset (int i);
    static juce::File userFolder() { auto f = juce::File::getSpecialLocation (juce::File::userMusicDirectory).getChildFile ("Jackpot").getChildFile ("Tag Forge"); f.createDirectory(); return f; }
    static juce::File presetFolder() { auto f = userFolder().getChildFile ("Preset"); f.createDirectory(); return f; }
    bool saveUserPreset (const juce::String& name);
    bool loadUserPreset (const juce::File& f);
    juce::String currentName() const { return userName.isNotEmpty() ? userName : (juce::isPositiveAndBelow (preset, (int) presets().size()) ? presets()[(size_t) preset].name : juce::String ("INIT")); }
    juce::String userName;
    float value (const char* id) const { return apvts.getRawParameterValue (id)->load(); }
    void setP (const char* id, float v) { if (auto* p = apvts.getParameter (id)) p->setValueNotifyingHost (p->convertTo0to1 (v)); }
    Settings settings() const;

    // registrazione dalla traccia (fino a 12 s) e caricamento da file
    void startRec();
    void stopRec();
    bool recording() const { return recOn.load(); }
    float recSeconds() const { return (float) recPos.load() / (float) juce::jmax (1.0, sr); }
    bool loadFile (const juce::File&);
    void setSource (juce::AudioBuffer<float>&& b, double rate, bool trim);
    bool hasSource() const { return srcLen.load() > 0; }

    // il tag forgiato: suona, si esporta, si trascina
    void play() { uiTrig.store (true); }
    juce::File exportTag();
    juce::File lastExport;
    std::atomic<float> uiLevel { 0 }, uiPlayPos { -1 };
    std::atomic<int> renderGen { 0 };
    // per il disegno: copie leggere della frase e del tag
    void waveSnapshot (std::vector<float>& src, std::vector<float>& tag, int points, float& tagSeconds, float& srcSeconds);
    void requestRender() { dirty.store (true); notify(); }
    void renderNow();

    juce::UndoManager undo { 200000, 60 };
    juce::AudioProcessorValueTreeState apvts;
    int preset = 0;
private:
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    void run() override;
    void handleAsyncUpdate() override {}
    double sr = 48000, hostBpm = 140;
    // frase sorgente (messaggi) e tag pronto (scambiato sotto lucchetto leggero)
    juce::CriticalSection srcLock; juce::AudioBuffer<float> source; double sourceRate = 48000; std::atomic<int> srcLen { 0 };
    juce::SpinLock tagLock; juce::AudioBuffer<float> tag, tagSpare; Settings lastSettings; bool everRendered = false;
    std::atomic<bool> dirty { false }; juce::CriticalSection renderLock;
    // registrazione
    juce::AudioBuffer<float> recBuf; std::atomic<bool> recOn { false }; std::atomic<int> recPos { 0 };
    // riproduzione: voce principale e voce che sfuma (niente click se riparte mentre suona)
    struct Voice { int pos = -1; float gain = 1, fade = 1, fadeStep = 0; };
    Voice v[2]; int cur = 0;
    std::atomic<bool> uiTrig { false };
    void trigger (float vel);
    juce::LinearSmoothedValue<float> tagGain, dryGain;
};
}
