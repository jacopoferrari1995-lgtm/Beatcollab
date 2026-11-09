#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <memory>

namespace jp
{
struct SampleData;
using SamplePtr = std::shared_ptr<SampleData>;

// Suono sintetizzato da parametri: il percorso "gen:..." contiene tutto per rigenerarlo
enum SynthType { S_KICK, S_808, S_SNARE, S_CLAP, S_HAT, S_RIM, S_TOM, S_SHAKER, S_CYMBAL, S_BELL, S_SNAP, S_STAB, S_CONGA, S_CLAVE, S_NOISE, S_PLUCK };
struct DrumDesc { int type; float a, b, c, d, e; juce::String name; };

SamplePtr makeDrum (const DrumDesc&);
juce::String descToPath (const DrumDesc&);
bool pathToDesc (const juce::String&, DrumDesc&);
}
