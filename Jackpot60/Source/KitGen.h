#pragma once
#include "Shared.h"

namespace j60
{
// Kit interno generato al volo: 16 suoni pensati per trap e boom bap (tonalità di riferimento: La minore)
SamplePtr makeBuiltin (int index);
juce::String builtinName (int index);
constexpr int kBuiltinCount = 16;
}
