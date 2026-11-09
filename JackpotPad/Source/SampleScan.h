#pragma once
#include <juce_audio_formats/juce_audio_formats.h>
#include <map>

namespace jp
{
// Indice dei campioni dell'utente: scansiona cartelle (anche enormi), riconosce il tipo di ogni suono
// dal nome e dall'ascolto e lo mette nella categoria giusta. I file non vengono mai spostati ne copiati.
struct ScanEntry { juce::String path, cat; juce::int64 mtime = 0; float dur = 0; bool manual = false; };

struct SoundFeatures
{
    float dur = 0, attack = 0, t30 = 0, low = 0, mid = 0, high = 0, zcr = 0, tonal = 0, f0 = 0, f0b = 0;
    int micro = 0, onsets = 0;
};

class SampleIndex : private juce::Thread
{
public:
    SampleIndex();
    ~SampleIndex() override;

    juce::StringArray roots() const;
    void addRoot (const juce::File&);
    void removeRoot (const juce::String&);
    void clearAll();
    void startScan();                  // aggiorna: analizza solo i file nuovi o cambiati
    void cancel() { signalThreadShouldExit(); }
    bool scanning() const { return isThreadRunning(); }
    std::atomic<int> done { 0 }, total { 0 }, generation { 0 }, added { 0 };

    std::vector<ScanEntry> entries (const juce::String& cat) const;
    int count (const juce::String& cat) const;
    int size() const;
    void setCategory (const juce::String& path, const juce::String& cat);
    void remove (const juce::String& path);
    bool contains (const juce::String& path) const;

    // riconoscimento
    static juce::String classifyName (const juce::File&, bool& oneShot, int& loop);   // loop: 0 no, 1 forse (bpm nel nome), 2 si
    static SoundFeatures analyse (juce::AudioFormatReader&);
    static juce::String classifySound (const SoundFeatures&, bool oneShot, bool loop);
    static juce::String classify (const juce::File&, juce::AudioFormatManager&, float& dur);

private:
    void run() override;
    void load();
    void save();
    void recount();
    static juce::File file();
    mutable juce::CriticalSection lock;
    std::vector<ScanEntry> items; std::map<juce::String, size_t> byPath; std::map<juce::String, int> counts;
    juce::StringArray rootList;
};
}
