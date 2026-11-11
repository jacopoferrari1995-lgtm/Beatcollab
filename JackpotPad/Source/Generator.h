#pragma once
#include "PadProcessor.h"

namespace jp
{
// Generatore di pattern per ruoli: guarda cosa c'e su ogni pad (cassa, rullante, hat, 808, melodico...)
// e scrive le parti giuste sui pad giusti. Un ruolo suona su un solo pad: niente doppioni che si sovrappongono.
enum Role { R_NONE, R_KICK, R_SNARE, R_CLAP, R_HAT, R_OPEN, R_PERC, R_TOM, R_CYM, R_808, R_MEL, R_LOOP, R_VOX, R_FX, R_TEX, R_CHOP, kRoles };
const char* roleName (int r);
Role roleOfCategory (const juce::String& cat);
Role padRole (const Processor&, int pad);

enum GenCat { GC_DRUM, GC_MPC, GC_PROG, kGenCats };
const char* genCatName (int c);
struct GenStyle { juce::String name; int cat; int idx; juce::String desc; };
const std::vector<GenStyle>& genStyles();
int stylesInCat (int cat, std::vector<int>& out);

// preset di uno stile: i loop scritti a mano (se ci sono) e le varianti numerate (seme fisso)
struct GenPreset { juce::String name; int bankIndex; juce::uint32 seed; };
std::vector<GenPreset> genPresets (int style);

// scrive il pattern (e per MPC e PROGRESSIONI prepara i pad che servono). Ritorna una riga da mostrare.
juce::String generate (Processor&, int style, juce::uint32 seed, int selectedPad);
// carica un loop della banca riportandolo sui ruoli del kit attuale
void loadBankByRoles (Processor&, int bankIndex);
}
