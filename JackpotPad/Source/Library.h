#pragma once
#include "PadProcessor.h"
#include "SampleScan.h"

namespace jp
{
// Banca suoni: suoni di fabbrica (sintetizzati) + cartelle utente in Musica/Jackpot Pad/Samples/<categoria>
struct LibItem { juce::String name, category; bool user = false; DrumDesc d; juce::File file; bool scanned = false; };

const juce::StringArray& libCategories();
juce::File libFolder (const juce::String& category);
std::vector<LibItem> libItems (const juce::String& category);          // fabbrica + utente
juce::String categoryOf (const DrumDesc&);
bool loadLibItem (Processor&, const LibItem&, int pad);
bool addFileToLibrary (const juce::File& src, const juce::String& category);
bool savePadToLibrary (Processor&, int pad, const juce::String& category);
// cambia categoria a un suono (file della libreria: lo sposta; campione scansionato: aggiorna l'indice)
bool moveLibItem (const LibItem&, const juce::String& category);
// ricerca in tutte le categorie
std::vector<LibItem> searchLibrary (const juce::String& query, size_t maxItems = 3000);
// kit casuale: 0 mix dei kit di fabbrica, 1 libreria (fabbrica + tuoi), 2 solo i tuoi campioni
void randomKit (Processor&, int mode);
extern const char* kRandModes[3];
}
