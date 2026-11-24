#include "ChoirProcessor.h"
#include "ChoirEditor.h"

namespace jcho
{
juce::AudioProcessorValueTreeState::ParameterLayout Processor::layout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    auto F = [&l] (const juce::String& id, const juce::String& n, NormalisableRange<float> r, float d) { l.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, n, r, d)); };
    auto Cc = [&l] (const juce::String& id, const juce::String& n, StringArray s, int d) { l.add (std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, n, s, d)); };
    StringArray keys; for (int i = 0; i < 12; ++i) keys.add (noteName (i));
    StringArray sc; for (int i = 0; i < kScales; ++i) sc.add (scaleName (i));
    StringArray iv; for (int i = 0; i < kIntervals; ++i) iv.add (intervalName (i));
    Cc ("mode", "Modo", { "SCALA", "MIDI" }, 0); Cc ("key", "Tonalita'", keys, 0); Cc ("scale", "Scala", sc, MAGGIORE);
    // di partenza tutte le voci spente: esce la voce com'e' (solo ritardata della latenza)
    for (int v = 1; v <= 4; ++v)
    {
        const juce::String p = "v" + juce::String (v) + "_";
        Cc (p + "int", "Voce " + juce::String (v) + " intervallo", iv, I_OFF);
        F (p + "lvl", "Voce " + juce::String (v) + " livello", { -24, 6, 0.01f }, -3);
        F (p + "pan", "Voce " + juce::String (v) + " pan", { -1, 1, 0.001f }, 0);
        F (p + "form", "Voce " + juce::String (v) + " formante", { -1, 1, 0.001f }, 0);
    }
    F ("dry", "Voce principale", { -60, 6, 0.01f }, 0); F ("choir", "Coro", { -24, 12, 0.01f }, 0);
    F ("human", "Umano", { 0, 1 }, 0.5f); F ("width", "Larghezza", { 0, 1 }, 0.7f); F ("cons", "Consonanti", { 0, 1 }, 0.35f);
    NormalisableRange<float> lc (20, 600); lc.setSkewForCentre (120);
    F ("lowcut", "Taglio bassi coro", lc, 120); F ("tone", "Tono coro", { -1, 1 }, 0); F ("glide", "Glide", { 0, 1 }, 0.3f);
    F ("out", "Output", { -24, 12, 0.01f }, 0);
    return l;
}
Processor::Processor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true).withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, &undo, "PARAMS", layout()) { order.reserve (128); }
bool Processor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto o = l.getMainOutputChannelSet(), i = l.getMainInputChannelSet();
    return o == juce::AudioChannelSet::stereo() && (i == o || i == juce::AudioChannelSet::mono());
}
void Processor::prepareToPlay (double s, int mb) { eng->prepare (s, mb); setLatencySamples (eng->latency()); down.fill (false); order.clear(); }
Settings Processor::settings() const
{
    Settings S;
    S.mode = juce::roundToInt (value ("mode")); S.key = juce::roundToInt (value ("key")); S.scale = juce::roundToInt (value ("scale"));
    for (int v = 0; v < 4; ++v)
    {
        const juce::String p = "v" + juce::String (v + 1) + "_";
        S.v[(size_t) v] = { juce::roundToInt (value (p + "int")), value (p + "lvl"), value (p + "pan"), value (p + "form") };
    }
    S.dryDb = value ("dry"); S.choirDb = value ("choir"); S.human = value ("human"); S.width = value ("width"); S.cons = value ("cons");
    S.lowCut = value ("lowcut"); S.tone = value ("tone"); S.glide = value ("glide"); S.outDb = value ("out");
    return S;
}
void Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals nd;
    const int n = buffer.getNumSamples(); if (n == 0) return;
    if (getTotalNumInputChannels() == 1) buffer.copyFrom (1, 0, buffer, 0, 0, n);
    const auto S = settings();
    float* L = buffer.getWritePointer (0); float* R = buffer.getWritePointer (1);
    // modo MIDI: le note tenute (dal basso) dicono cosa cantano le voci; si aggiorna nel punto esatto della nota
    int pos = 0;
    auto flushHeld = [this]
    {   // fino a quattro note: le piu' recenti, poi in ordine dal basso
        std::array<int, 4> h { -1, -1, -1, -1 }; int c = 0;
        for (auto it = order.rbegin(); it != order.rend() && c < 4; ++it) if (down[(size_t) *it]) h[(size_t) c++] = *it;
        std::sort (h.begin(), h.begin() + c);
        eng->nHeld = c; for (int k = 0; k < 4; ++k) { eng->held[(size_t) k] = k < c ? h[(size_t) k] : -1; uiHeld[(size_t) k] = eng->held[(size_t) k]; }
    };
    for (const auto m : midi)
    {
        const auto& msg = m.getMessage(); const int at = juce::jlimit (0, n, m.samplePosition);
        if (at > pos) { eng->process (L + pos, R + pos, at - pos, S); pos = at; }
        if (msg.isNoteOn()) { down[(size_t) msg.getNoteNumber()] = true; order.erase (std::remove (order.begin(), order.end(), msg.getNoteNumber()), order.end()); order.push_back (msg.getNoteNumber()); }
        else if (msg.isNoteOff()) { down[(size_t) msg.getNoteNumber()] = false; order.erase (std::remove (order.begin(), order.end(), msg.getNoteNumber()), order.end()); }
        else if (msg.isAllNotesOff() || msg.isAllSoundOff()) { down.fill (false); order.clear(); }
        flushHeld();
    }
    if (pos < n) eng->process (L + pos, R + pos, n - pos, S);
}
void Processor::loadPreset (int i)
{
    if (! juce::isPositiveAndBelow (i, (int) presets().size())) return;
    undo.beginNewTransaction();
    preset = i; userName = {}; jkit::applyPreset (*this, apvts, presets()[(size_t) i], { "out", "key", "scale" });
}
void Processor::getStateInformation (juce::MemoryBlock& d)
{
    auto s = apvts.copyState(); s.setProperty ("preset", preset, nullptr); s.setProperty ("uname", userName, nullptr);
    if (auto x = s.createXml()) copyXmlToBinary (*x, d);
}
void Processor::setStateInformation (const void* data, int size)
{
    if (auto x = getXmlFromBinary (data, size)) { auto t = juce::ValueTree::fromXml (*x); if (t.isValid()) { preset = t.getProperty ("preset", 0); userName = t.getProperty ("uname", "").toString(); apvts.replaceState (t); undo.clearUndoHistory(); } }
}
bool Processor::saveUserPreset (const juce::String& name)
{
    auto x = apvts.copyState().createXml(); if (! x) return false;
    const auto f = presetFolder().getChildFile (juce::File::createLegalFileName (name) + ".jchoir");
    if (! x->writeTo (f)) return false;
    userName = f.getFileNameWithoutExtension(); preset = -1; return true;
}
bool Processor::loadUserPreset (const juce::File& f)
{
    auto x = juce::XmlDocument::parse (f); if (! x) return false;
    auto t = juce::ValueTree::fromXml (*x); if (! t.isValid() || ! t.hasType (apvts.state.getType())) return false;
    undo.beginNewTransaction(); apvts.replaceState (t); userName = f.getFileNameWithoutExtension(); preset = -1; return true;
}
juce::AudioProcessorEditor* Processor::createEditor() { return new Editor (*this); }

using jkit::PV;
// voce: intervallo, livello, pan, formante
static PV V (int k, int iv, float lvl = -3, float pan = 0, float form = 0)
{
    static const char* ids[4][4] = { { "v1_int", "v1_lvl", "v1_pan", "v1_form" }, { "v2_int", "v2_lvl", "v2_pan", "v2_form" }, { "v3_int", "v3_lvl", "v3_pan", "v3_form" }, { "v4_int", "v4_lvl", "v4_pan", "v4_form" } };
    return { { ids[k][0], (float) iv }, { ids[k][1], lvl }, { ids[k][2], pan }, { ids[k][3], form } };
}
const std::vector<jkit::Preset>& presets()
{
    using jkit::cat;
    static const std::vector<jkit::Preset> p = {
        { "Init", "Tutte le voci spente: esce la tua voce com'e'. Accendi una voce e scegli l'intervallo, oppure parti da un preset.", {} },
        { "DOPPIE | Doppia stretta", "Due doppie ai lati, poco ritardo: la voce si allarga come un raddoppio vero.", cat ({ V (0, I_UNI, -5, -0.7f), V (1, I_UNI, -5, 0.7f), PV { { "human", 0.45f }, { "width", 0.9f } } }) },
        { "DOPPIE | Doppia larga e lenta", "Doppie piu' lente e libere: il ritornello che si apre.", cat ({ V (0, I_UNI, -4, -1), V (1, I_UNI, -4, 1), PV { { "human", 0.85f }, { "width", 1.0f } } }) },
        { "DOPPIE | Doppia in ottava sotto", "Una doppia e un'ottava sotto piano: voce piu' grossa senza sentirla.", cat ({ V (0, I_UNI, -6, -0.5f), V (1, I_8D, -10, 0.5f, -0.15f), PV { { "human", 0.4f } } }) },
        { "ARMONIE | Terza sopra", "La terza della scala sopra la melodia: l'armonia piu' classica.", cat ({ V (0, I_3U, -4, 0.35f), PV { { "human", 0.35f } } }) },
        { "ARMONIE | Terza sotto", "Terza sotto: armonia piu' scura, da R&B.", cat ({ V (0, I_3D, -4, -0.35f), PV { { "human", 0.35f } } }) },
        { "ARMONIE | Terza e quinta", "Triade sopra la voce: terza a sinistra, quinta a destra.", cat ({ V (0, I_3U, -5, -0.55f), V (1, I_5U, -6, 0.55f), PV { { "human", 0.45f } } }) },
        { "ARMONIE | Sesta sopra", "Sesta sopra: dolce, anni 60, soul.", cat ({ V (0, I_6U, -5, 0.4f), PV { { "human", 0.4f } } }) },
        { "ARMONIE | Quinta potente", "Quinta sotto e ottava sotto: hook pieno e duro.", cat ({ V (0, I_5D, -6, -0.4f), V (1, I_8D, -9, 0.4f), PV { { "human", 0.3f } } }) },
        { "CORO | Gospel tre voci", "Terza sotto, terza e quinta sopra, larghi: coro gospel dietro la lead.", cat ({ V (0, I_3D, -6, -0.8f), V (1, I_3U, -6, 0.8f), V (2, I_5U, -8, -0.3f), PV { { "human", 0.65f }, { "width", 1.0f }, { "cons", 0.25f } } }) },
        { "CORO | Muro pieno", "Doppia, terza, quinta e ottava sotto: il coro che riempie il ritornello.", cat ({ V (0, I_UNI, -6, -0.9f), V (1, I_3U, -7, 0.9f), V (2, I_5U, -8, -0.45f), V (3, I_8D, -9, 0.45f), PV { { "human", 0.7f }, { "width", 1.0f }, { "cons", 0.2f } } }) },
        { "CORO | Angeli", "In alto e leggero: ottava e decima sopra, bassi tagliati e aria.", cat ({ V (0, I_8U, -9, -0.7f, 0.1f), V (1, I_10U, -11, 0.7f, 0.1f), V (2, I_5U, -10, 0), PV { { "human", 0.6f }, { "lowcut", 320 }, { "tone", 0.5f }, { "cons", 0.15f } } }) },
        { "CORO | Bassi profondi", "Ottava e quinta sotto con formante scura: un coro di bassi.", cat ({ V (0, I_8D, -5, -0.5f, -0.35f), V (1, I_5D, -7, 0.5f, -0.25f), PV { { "human", 0.5f }, { "lowcut", 60 }, { "tone", -0.35f } } }) },
        { "HOOK | Ritornello largo", "Due doppie ai lati e una terza al centro: il ritornello che si stacca dalla strofa.", cat ({ V (0, I_UNI, -5, -1), V (1, I_UNI, -5, 1), V (2, I_3U, -8, 0), PV { { "human", 0.55f }, { "width", 1.0f } } }) },
        { "AD-LIB | Quinta lontana", "Quinta sopra, piano e di lato: risposta all'ad-lib.", cat ({ V (0, I_5U, -8, 0.75f), PV { { "human", 0.5f }, { "lowcut", 220 } } }) },
        { "MIDI | Armonia dalla tastiera", "Le voci cantano le note che tieni sulla tastiera (MIDI verso il plugin), col vibrato del cantante.", cat ({ PV { { "mode", 1 } }, V (0, I_UNI, -4, -0.6f), V (1, I_UNI, -4, 0.6f), V (2, I_UNI, -5, -0.2f), V (3, I_UNI, -5, 0.2f), PV { { "human", 0.45f } } }) },
        { "MIDI | Coro da tastiera largo", "Accordi suonati sulla tastiera: coro largo e umano sopra la voce.", cat ({ PV { { "mode", 1 } }, V (0, I_UNI, -5, -1), V (1, I_UNI, -5, 1), V (2, I_UNI, -6, -0.5f), V (3, I_UNI, -6, 0.5f), PV { { "human", 0.75f }, { "width", 1.0f }, { "dry", -3 } } }) },
        { "EFFETTO | Ottave robot", "Ottava sopra e sotto senza umano: voce sintetica, da hook elettronico.", cat ({ V (0, I_8U, -6, -0.5f, 0.3f), V (1, I_8D, -6, 0.5f, -0.3f), PV { { "human", 0.0f }, { "glide", 0.0f } } }) },
        { "EFFETTO | Solo coro", "Voce principale spenta: senti solo le armonie (per stamparle su una traccia).", cat ({ V (0, I_3U, -3, -0.5f), V (1, I_5U, -4, 0.5f), PV { { "dry", -60 }, { "human", 0.5f } } }) },
    };
    return p;
}
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new jcho::Processor(); }
