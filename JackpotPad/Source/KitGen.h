#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <memory>
namespace jp { struct SampleData; using SamplePtr = std::shared_ptr<SampleData>; }

namespace jp
{
// Kit interno generato al volo: 16 suoni pensati per trap e boom bap (tonalità di riferimento: La minore)
SamplePtr makeBuiltin (int index);
juce::String builtinName (int index);
constexpr int kBuiltinCount = 16;
}
