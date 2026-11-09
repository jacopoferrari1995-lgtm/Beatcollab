#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <atomic>
#include <memory>
#include <vector>

namespace j60
{
constexpr int kBanks = 4, kPadsPerBank = 16, kPads = kBanks * kPadsPerBank;
constexpr int kSeqs = 8, kMaxBars = 16, kVoices = 32;
constexpr int PPQ = 96, kTicksPerBar = PPQ * 4, kTick16 = PPQ / 4;
constexpr int kBaseNote = 36; // C1 = A01

struct SampleData
{
    juce::AudioBuffer<float> buf;   // stereo
    double sr = 44100.0;
    juce::String name, path;        // path vuoto = suono interno
};
using SamplePtr = std::shared_ptr<SampleData>;

struct PadParams
{
    std::atomic<float> level { 0.8f }, pan { 0.0f }, tune { 0.0f }, fine { 0.0f },
                       start { 0.0f }, end { 1.0f }, attack { 0.0f }, decay { 5000.0f },
                       release { 80.0f }, cutoff { 20000.0f }, reso { 0.1f }, velSens { 0.7f };
    std::atomic<int> reverse { 0 }, mode { 0 }, choke { 0 }, loop { 0 }, sync { 0 };
    std::atomic<float> bpm { 0.0f }, sendDly { 0.0f }, sendRev { 0.0f };
};

struct ParamInfo { const char* id; const char* label; float min, max, def; int kind; }; // kind 0 float,1 int,2 bool, 3 Hz, 4 ms, 5 st, 6 bpm
static const ParamInfo kPadParamInfo[] = {
    { "level",   "LEVEL",    0.0f, 1.0f, 0.8f, 0 },
    { "pan",     "PAN",     -1.0f, 1.0f, 0.0f, 0 },
    { "tune",    "TUNE",   -36.0f, 36.0f, 0.0f, 5 },
    { "fine",    "FINE",   -50.0f, 50.0f, 0.0f, 1 },
    { "start",   "START",    0.0f, 1.0f, 0.0f, 0 },
    { "end",     "END",      0.0f, 1.0f, 1.0f, 0 },
    { "attack",  "ATTACK",   0.0f, 500.0f, 0.0f, 4 },
    { "decay",   "DECAY",   10.0f, 5000.0f, 5000.0f, 4 },
    { "release", "RELEASE",  5.0f, 2000.0f, 80.0f, 4 },
    { "cutoff",  "FILTER",  80.0f, 20000.0f, 20000.0f, 3 },
    { "reso",    "RESO",     0.0f, 1.0f, 0.1f, 0 },
    { "velSens", "VEL SENS", 0.0f, 1.0f, 0.7f, 0 },
    { "mode",    "MODE",     0.0f, 1.0f, 0.0f, 2 },
    { "reverse", "REVERSE",  0.0f, 1.0f, 0.0f, 2 },
    { "choke",   "CHOKE",    0.0f, 8.0f, 0.0f, 1 },
    { "loop",    "LOOP",     0.0f, 1.0f, 0.0f, 2 },
    { "sync",    "BPM SYNC", 0.0f, 1.0f, 0.0f, 2 },
    { "bpm",     "BPM CAMP", 0.0f, 240.0f, 0.0f, 6 },
    { "sendDly", "DELAY",    0.0f, 1.0f, 0.0f, 0 },
    { "sendRev", "REVERB",   0.0f, 1.0f, 0.0f, 0 }
};
constexpr int kNumPadParams = (int) (sizeof (kPadParamInfo) / sizeof (kPadParamInfo[0]));

struct Event { int tick = 0; int pad = 0; int vel = 100; };
struct Sequence { int bars = 2; std::vector<Event> ev; };

inline juce::String padName (int p) { return juce::String::charToString ((juce::juce_wchar) ('A' + p / kPadsPerBank)) + juce::String (p % kPadsPerBank + 1).paddedLeft ('0', 2); }
}
