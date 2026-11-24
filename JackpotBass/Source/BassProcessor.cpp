#include "BassProcessor.h"
#include "BassEditor.h"

namespace bss
{
juce::File presetFolder()
{
    auto d = juce::File::getSpecialLocation (juce::File::userMusicDirectory).getChildFile ("Jackpot").getChildFile ("Bass Preset");
    d.createDirectory(); return d;
}
static const char* kGenIds[] = { "key", "scale", "genre", "mood", "mode", "fig", "rit", "rot", "bars", "perBar", "color", "octave", "density", "dyn", "human", "swing", "rest", "len", "glide", "approach", "ghost", "borrow", "vary" };

// preset: gruppo | nome. Lo stile imposta figura, ritmo, glide e lunghezza; il resto lo dice il preset
const std::vector<BPreset>& presets()
{
    using V = std::vector<std::pair<const char*, float>>;
    static const std::vector<BPreset> P = {
        { "INIT | 808 base", "808 lungo sul giro i - VI - III - VII in La minore.", V {}, { 0, 5, 2, 6 } },
        { "808 | Trap lungo", "Una nota per accordo che tiene, qualche colpo in levare. Il classico.", V { { "mode", B808 }, { "style", 0 }, { "genre", TRAP }, { "key", 9 }, { "decay", 0.75f }, { "drive", 0.35f } }, { 0, 5, 2, 6 } },
        { "808 | Trap roll", "Colpi e ottave con glide, roll in terzine a fine frase.", V { { "mode", B808 }, { "style", 1 }, { "genre", TRAP }, { "key", 1 }, { "density", 0.6f }, { "bars", 3 }, { "drive", 0.45f } }, { 0, 5, 6, 4 } },
        { "808 | Drill slide", "808 che scivola su ottave e quinte, ritmo drill.", V { { "mode", B808 }, { "style", 2 }, { "genre", DRILL }, { "key", 6 }, { "scale", ARMONICA }, { "glide", 0.75f }, { "drive", 0.5f }, { "punch", 0.6f } }, { 0, 5, 4, 0 } },
        { "808 | Ottave", "Fondamentale e ottava sul 3+3+2.", V { { "mode", B808 }, { "style", 3 }, { "genre", TRAP }, { "key", 4 } }, { 0, 3, 5, 6 } },
        { "808 | Emo", "Minime lunghe, fondamentale e quinta, malinconico.", V { { "mode", B808 }, { "style", 4 }, { "genre", EMO }, { "key", 4 }, { "mood", MALINCONICO }, { "decay", 0.85f } }, { 5, 6, 0, 0 } },
        { "808 | Salto di quinta", "Scivola sulla quinta e torna, poi sotto di ottava.", V { { "mode", B808 }, { "style", 5 }, { "genre", DRILL }, { "key", 2 }, { "glide", 0.6f } }, { 0, 5, 0, 6 } },
        { "808 | Reggaeton", "808 corto sul dembow.", V { { "mode", B808 }, { "style", 6 }, { "genre", REGGAETON }, { "key", 9 }, { "decay", 0.35f } }, { 0, 5, 2, 6 } },
        { "BASSO | Pop ottavi", "Ottavi sulla fondamentale con passaggi verso l'accordo dopo.", V { { "mode", BASSO }, { "style", 0 }, { "genre", POP }, { "key", 7 }, { "scale", MAGGIORE }, { "approach", 0.5f }, { "ghost", 0.1f } }, { 0, 4, 5, 3 } },
        { "BASSO | Rock", "Fondamentale e ottava in ottavi, spinto.", V { { "mode", BASSO }, { "style", 1 }, { "genre", ROCK }, { "key", 4 }, { "drive", 0.5f }, { "ghost", 0.05f } }, { 0, 6, 3, 0 } },
        { "BASSO | Folk", "Fondamentale e quinta sui tempi.", V { { "mode", BASSO }, { "style", 2 }, { "genre", FOLK }, { "key", 2 }, { "scale", MAGGIORE }, { "ghost", 0.0f } }, { 0, 3, 0, 4 } },
        { "BASSO | R&B", "Linea sincopata con terze e passaggi, qualche ghost.", V { { "mode", BASSO }, { "style", 3 }, { "genre", RNB }, { "key", 5 }, { "scale", MAGGIORE }, { "color", 2 }, { "approach", 0.6f }, { "ghost", 0.3f }, { "swing", 0.2f } }, { 1, 4, 0, 0 } },
        { "BASSO | Walking", "Semiminime che camminano sugli accordi e arrivano per grado.", V { { "mode", BASSO }, { "style", 4 }, { "genre", BOOMBAP }, { "key", 10 }, { "scale", MAGGIORE }, { "approach", 0.8f }, { "swing", 0.35f }, { "ghost", 0.1f } }, { 1, 4, 0, 5 } },
        { "BASSO | Funk", "Sedicesimi con ottave, settime e ghost note.", V { { "mode", BASSO }, { "style", 5 }, { "genre", FUNK }, { "key", 4 }, { "scale", MISOLIDIA }, { "ghost", 0.6f }, { "tone", 0.7f } }, { 0, 3, 0, 3 } },
        { "BASSO | Afro", "Basso sincopato sulle terze, afrobeats.", V { { "mode", BASSO }, { "style", 7 }, { "genre", AFRO }, { "key", 0 }, { "scale", MAGGIORE }, { "swing", 0.15f } }, { 0, 3, 5, 4 } },
        { "SYNTH | Synthwave", "Ottavi che pompano, filtro mezzo aperto.", V { { "mode", SYNTH }, { "style", 0 }, { "genre", POP }, { "key", 9 }, { "tone", 0.45f }, { "punch", 0.3f } }, { 0, 5, 2, 6 } },
        { "SYNTH | Acid", "Sedicesimi con accenti e slide, risonanza alta.", V { { "mode", SYNTH }, { "style", 1 }, { "genre", FUNK }, { "key", 9 }, { "tone", 0.35f }, { "punch", 0.85f }, { "decay", 0.3f }, { "drive", 0.5f } }, { 0, 0, 5, 6 } },
        { "SYNTH | Reese", "Note lunghe, scure, per drill e dark.", V { { "mode", SYNTH }, { "style", 2 }, { "genre", DRILL }, { "key", 1 }, { "tone", 0.3f }, { "punch", 0.2f }, { "decay", 0.9f } }, { 0, 5, 4, 0 } },
        { "SYNTH | House ottave", "Ottave in levare, corte.", V { { "mode", SYNTH }, { "style", 3 }, { "genre", AFRO }, { "key", 5 }, { "tone", 0.5f }, { "punch", 0.45f } }, { 0, 5, 3, 4 } },
        { "SYNTH | Disco", "Quinta e ottava in ottavi.", V { { "mode", SYNTH }, { "style", 4 }, { "genre", FUNK }, { "key", 2 }, { "scale", DORICA }, { "tone", 0.55f } }, { 0, 3, 0, 3 } },
        { "SYNTH | Trap synth", "Ottave con glide al posto dell'808.", V { { "mode", SYNTH }, { "style", 5 }, { "genre", TRAP }, { "key", 7 }, { "tone", 0.25f }, { "decay", 0.8f } }, { 0, 5, 2, 6 } },
    };
    return P;
}

juce::AudioProcessorValueTreeState::ParameterLayout Processor::layout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;
    auto ch = [&] (const char* id, const char* nm, StringArray items, int def) { p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, nm, items, def)); };
    auto fl = [&] (const char* id, const char* nm, float def, float lo = 0, float hi = 1) { p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, nm, NormalisableRange<float> (lo, hi), def)); };
    auto bo = [&] (const char* id, const char* nm, bool def) { p.push_back (std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, nm, def)); };
    StringArray keys; for (int i = 0; i < 12; ++i) keys.add (noteName (i));
    StringArray sc; for (int i = 0; i < kScales; ++i) sc.add (scaleName (i));
    StringArray gn; for (int i = 0; i < kGenres; ++i) gn.add (genreName (i));
    StringArray mo; for (int i = 0; i < kMoods; ++i) mo.add (moodName (i));
    StringArray md; for (int i = 0; i < kBModes; ++i) md.add (bmodeName (i));
    StringArray fg; for (int i = 0; i < kBFigs; ++i) fg.add (bfigOf (i).name);
    StringArray rt; for (int i = 0; i < kBRits; ++i) rt.add (britOf (i).name);
    ch ("key", "Tonalita'", keys, 9); ch ("scale", "Scala", sc, MINORE); ch ("genre", "Genere", gn, TRAP); ch ("mood", "Mood", mo, 0); ch ("mode", "Modo", md, B808);
    p.push_back (std::make_unique<AudioParameterInt> (ParameterID { "style", 1 }, "Stile", 0, 7, 0));
    ch ("fig", "Figura", fg, 0); ch ("rit", "Ritmo", rt, 0);
    p.push_back (std::make_unique<AudioParameterInt> (ParameterID { "rot", 1 }, "Partenza", 0, 7, 0));
    ch ("bars", "Battute", { "1", "2", "4", "8", "16" }, 2); ch ("perBar", "Accordi per battuta", { "1", "2", "1/2" }, 0);
    ch ("color", "Colore", { "AUTO", "TRIADI", "SETTIME", "NONE", "SUS / ADD9" }, 0);
    p.push_back (std::make_unique<AudioParameterInt> (ParameterID { "octave", 1 }, "Ottava", -1, 1, 0));
    fl ("density", "Densita'", 0.5f); fl ("dyn", "Dinamica", 0.6f); fl ("human", "Umano", 0.3f); fl ("swing", "Swing", 0.0f); fl ("rest", "Pause", 0.2f);
    fl ("len", "Lunghezza", 0.95f); fl ("glide", "Glide", 0.2f); fl ("approach", "Passaggi", 0.3f); fl ("ghost", "Ghost", 0.2f); fl ("borrow", "Interscambio", 0.0f); fl ("vary", "Variazione", 0.45f);
    fl ("tone", "Tono", 0.5f); fl ("drive", "Drive", 0.3f); fl ("decay", "Decay", 0.6f); fl ("punch", "Punch", 0.5f); fl ("vol", "Volume", 0.0f, -36.0f, 6.0f);
    bo ("listen", "Suono acceso", true); bo ("live", "Giro in play", true); bo ("midiOut", "MIDI out", true); bo ("play", "Prova", false);
    return { p.begin(), p.end() };
}

Processor::Processor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, &undo, "PARAMS", layout())
{
    for (auto* id : kGenIds) apvts.addParameterListener (id, this);
    apvts.state.setProperty ("prog", "0,5,2,6", nullptr); apvts.state.setProperty ("seed", 1, nullptr); apvts.state.setProperty ("figSeq", "R 8 5 R", nullptr);
    apvts.state.addListener (this);
    regenerate();
}
Processor::~Processor() { cancelPendingUpdate(); apvts.state.removeListener (this); for (auto* id : kGenIds) apvts.removeParameterListener (id, this); }

std::vector<int> Processor::prog() const
{
    std::vector<int> r; for (auto& t : juce::StringArray::fromTokens (apvts.state.getProperty ("prog", "0,5,2,6").toString(), ",", "")) if (t.trim().isNotEmpty()) r.push_back (juce::jlimit (0, 899, t.getIntValue()));
    if (r.empty()) r = { 0, 5, 2, 6 };
    if (r.size() > 8) r.resize (8);
    return r;
}
void Processor::setProg (const std::vector<int>& p) { juce::StringArray s; for (int x : p) s.add (juce::String (x)); apvts.state.setProperty ("prog", s.joinIntoString (","), &undo); }
std::array<juce::uint16, 5> Processor::grid() const
{
    std::array<juce::uint16, 5> g { 0x0010, 0x0400, 0, 0x8081, 0 };
    const auto t = juce::StringArray::fromTokens (apvts.state.getProperty ("grid").toString(), ",", "");
    if (t.size() == 5) for (int i = 0; i < 5; ++i) g[(size_t) i] = (juce::uint16) t[i].getHexValue32();
    return g;
}
void Processor::setGrid (const std::array<juce::uint16, 5>& g) { juce::StringArray t; for (auto x : g) t.add (juce::String::toHexString ((int) x)); apvts.state.setProperty ("grid", t.joinIntoString (","), &undo); }

void Processor::applyStyle (int style)
{
    const auto& L = bstyles (vi ("mode")); const auto& s = L[(size_t) juce::jlimit (0, (int) L.size() - 1, style)];
    setV ("style", (float) style); setV ("fig", (float) s.fig); setV ("rit", (float) s.rit); setV ("glide", s.glide); setV ("len", s.len); setV ("rot", 0);
    if (vi ("mode") == BASSO) setV ("ghost", s.rit == 10 ? 0.6f : s.rit == 12 ? 0.3f : 0.1f);
}
void Processor::generaGiro()
{
    undo.beginNewTransaction();
    const auto s = juce::Random::getSystemRandom().nextInt (0x7ffffff) | 1;
    const auto np = newProgression (vi ("genre"), vi ("mood"), vi ("scale"), barsOf (vi ("bars")), (juce::uint32) s);
    setProg (np.prog); setSeed ((juce::uint32) s); curPreset = -1;
    setV ("perBar", np.perBar == 2 ? 1.0f : np.perBar == 0 ? 2.0f : 0.0f);
    // stile adatto al genere per il modo scelto; a volte una combinazione nuova di figura e ritmo
    static const std::vector<int> G[kGenres][kBModes] = {
        { { 0, 1, 5 }, { 0 }, { 5, 2 } }, { { 2, 5 }, { 0 }, { 5, 2 } }, { { 4, 0 }, { 0, 2 }, { 2 } }, { { 4, 0 }, { 3, 4 }, { 2, 3 } },
        { { 0, 3 }, { 0, 2 }, { 0, 4 } }, { { 6 }, { 6 }, { 6 } }, { { 7 }, { 7 }, { 3 } }, { { 0 }, { 4, 3 }, { 2 } }, { { 4 }, { 3, 4 }, { 2 } },
        { { 3 }, { 1, 0 }, { 0 } }, { { 3 }, { 5 }, { 4, 1 } }, { { 0 }, { 2 }, { 0 } }, { { 6 }, { 6, 2 }, { 4 } } };
    juce::Random R ((juce::int64) s);
    const auto& L = G[juce::jlimit (0, kGenres - 1, vi ("genre"))][juce::jlimit (0, kBModes - 1, vi ("mode"))];
    applyStyle (L[(size_t) R.nextInt ((int) L.size())]);
    if (R.nextFloat() < 0.35f) { setV ("fig", (float) R.nextInt (kBLibera)); setV ("rit", (float) R.nextInt (kBRits)); }
}
void Processor::aCaso()
{
    auto& R = juce::Random::getSystemRandom(); undo.beginNewTransaction();
    auto rr = [&R] (float a, float b) { return a + (b - a) * R.nextFloat(); };
    const int g = R.nextInt (kGenres); setV ("genre", (float) g); setV ("mood", (float) R.nextInt (kMoods));
    setV ("density", rr (0.3f, 0.75f)); setV ("dyn", rr (0.4f, 0.8f)); setV ("human", rr (0.2f, 0.5f));
    const bool swingy = g == RNB || g == LOFI || g == BOOMBAP || g == AFRO || g == FUNK;
    setV ("swing", swingy ? rr (0.1f, 0.35f) : 0.0f); setV ("rest", rr (0.05f, 0.4f)); setV ("approach", rr (0.1f, 0.6f)); setV ("vary", rr (0.25f, 0.6f));
    setV ("borrow", R.nextFloat() < 0.2f ? rr (0.2f, 0.5f) : 0.0f); setV ("color", 0); setV ("rot", (float) R.nextInt (3));
    generaGiro();
    // dopo lo stile (che imposta glide, lunghezza e ghost) un po' di carattere a caso
    setV ("glide", juce::jlimit (0.0f, 1.0f, v ("glide") + rr (-0.15f, 0.25f))); setV ("ghost", vi ("mode") == BASSO ? rr (0.05f, 0.5f) : 0.0f);
    presetName = "A CASO";
}
void Processor::varia() { undo.beginNewTransaction(); setSeed ((juce::uint32) (juce::Random::getSystemRandom().nextInt (0x7ffffff) | 1)); }

BSettings Processor::settings() const
{
    BSettings S;
    S.key = vi ("key"); S.scale = vi ("scale"); S.genre = vi ("genre"); S.mood = vi ("mood"); S.mode = vi ("mode");
    S.fig = vi ("fig"); S.rit = vi ("rit"); S.rot = vi ("rot"); S.figSeq = apvts.state.getProperty ("figSeq", "R 8 5 R").toString(); S.grid = grid();
    S.bars = barsOf (vi ("bars")); { static const int pb[3] = { 1, 2, 0 }; S.perBar = pb[juce::jlimit (0, 2, vi ("perBar"))]; }
    S.color = (float) (vi ("color") - 1); S.octave = vi ("octave");
    S.density = v ("density"); S.dyn = v ("dyn"); S.human = v ("human"); S.swing = v ("swing"); S.space = v ("rest"); S.len = v ("len");
    S.glide = v ("glide"); S.approach = v ("approach"); S.ghost = v ("ghost"); S.borrow = v ("borrow"); S.vary = v ("vary");
    S.prog = prog(); S.seed = seed();
    return S;
}
juce::String Processor::settingsKey() const
{
    const auto S = settings(); juce::String k; auto f = [] (float x) { return juce::String (juce::roundToInt (x * 1000.0f)); };
    k << S.key << "|" << S.scale << "|" << S.genre << "|" << S.mood << "|" << S.mode << "|" << S.fig << "|" << S.rit << "|" << S.rot << "|" << S.figSeq << "|" << apvts.state.getProperty ("grid").toString()
      << "|" << S.bars << "|" << S.perBar << "|" << f (S.color) << "|" << S.octave << "|" << f (S.density) << "|" << f (S.dyn) << "|" << f (S.human) << "|" << f (S.swing) << "|" << f (S.space) << "|" << f (S.len)
      << "|" << f (S.glide) << "|" << f (S.approach) << "|" << f (S.ghost) << "|" << f (S.borrow) << "|" << f (S.vary) << "|" << (int) S.seed << "|" << apvts.state.getProperty ("prog").toString();
    return k;
}
static juce::String serialize (const Part& P)
{
    juce::String s;
    for (auto& e : P.ev) s << "n" << e.beat << "," << e.len << "," << e.note << "," << e.vel << "," << e.str << "," << e.fret << "," << ((e.mute ? 1 : 0) | (e.art << 3)) << "," << e.slot << ";";
    return s;
}
static void deserialize (const juce::String& s, Part& P)
{
    P.ev.clear();
    for (auto& t : juce::StringArray::fromTokens (s, ";", ""))
    {
        if (t.isEmpty() || t[0] != 'n') continue;
        const auto v = juce::StringArray::fromTokens (t.substring (1), ",", ""); if (v.size() < 8) continue;
        Ev e; e.beat = v[0].getDoubleValue(); e.len = v[1].getDoubleValue(); e.note = v[2].getIntValue(); e.vel = v[3].getFloatValue(); e.str = v[4].getIntValue(); e.fret = v[5].getIntValue();
        const int fl = v[6].getIntValue(); e.mute = fl & 1; e.art = (fl >> 3) & 3; e.slot = v[7].getIntValue();
        if (e.len > 0 && e.beat >= 0) P.ev.push_back (e);
    }
    std::stable_sort (P.ev.begin(), P.ev.end(), [] (const Ev& a, const Ev& b) { return a.beat < b.beat; });
}
void Processor::commitEdit (const Part& p)
{
    undo.beginNewTransaction();
    apvts.state.setProperty ("editKey", settingsKey(), &undo); apvts.state.setProperty ("edit", serialize (p), &undo);
    auto np2 = std::make_shared<Part> (p); std::atomic_store (&partPtr, std::shared_ptr<const Part> (np2));
}
void Processor::clearEdit() { undo.beginNewTransaction(); apvts.state.setProperty ("edit", "", &undo); }
void Processor::regenerate()
{
    auto S = settings(); S.bpm = uiBpm.load();
    auto p = std::make_shared<Part> (generateBass (S));
    if (edited()) deserialize (apvts.state.getProperty ("edit").toString(), *p);
    std::atomic_store (&partPtr, std::shared_ptr<const Part> (p));
}
void Processor::prepareToPlay (double s, int) { sr = s; syn.prepare (sr); acts.reserve (2048); np = 0; lastEnd = -1; ownPpq = 0; held = 0; }

juce::String Processor::clipName() const
{
    const auto S = settings();
    return juce::String ("Bass ") + noteName (S.key) + (isMinorScale (S.scale) ? "m " : " ") + bmodeName (S.mode) + " " + genreName (S.genre) + " " + (S.fig == kBLibera ? juce::String ("LIBERA") : juce::String (bfigOf (S.fig).name)) + " " + britOf (S.rit).name;
}
juce::File Processor::exportMidi()
{
    auto P = part(); const double T = 960;
    juce::MidiFile mf; mf.setTicksPerQuarterNote ((int) T);
    juce::MidiMessageSequence meta;
    meta.addEvent (juce::MidiMessage::tempoMetaEvent ((int) (60000000.0 / juce::jmax (40.0, uiBpm.load()))), 0);
    meta.addEvent (juce::MidiMessage::timeSignatureMetaEvent (4, 4), 0); mf.addTrack (meta);
    juce::MidiMessageSequence seq; seq.addEvent (juce::MidiMessage::textMetaEvent (3, clipName()), 0);
    for (auto& e : P->ev)
    {
        seq.addEvent (juce::MidiMessage::noteOn (1, e.note, (juce::uint8) juce::jlimit (1, 127, juce::roundToInt (e.vel * 127))), e.beat * T);
        seq.addEvent (juce::MidiMessage::noteOff (1, e.note), juce::jmax (e.beat * T + 6, (e.beat + e.len) * T));
    }
    seq.addEvent (juce::MidiMessage::endOfTrack(), P->len() * T);
    seq.updateMatchedPairs(); seq.sort(); mf.addTrack (seq);
    auto dir = juce::File::getSpecialLocation (juce::File::userMusicDirectory).getChildFile ("Jackpot").getChildFile ("Bass"); dir.createDirectory();
    auto f = dir.getChildFile (juce::File::createLegalFileName (clipName() + " " + juce::String ((int) (seed() % 1000)) + ".mid"));
    f.deleteFile(); juce::FileOutputStream os (f); if (os.openedOk()) { mf.writeTo (os); os.flush(); }
    return f;
}

void Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals nd;
    buffer.clear(); const int N = buffer.getNumSamples(); if (N <= 0) return;
    syn.m.mode = vi ("mode"); syn.m.tone = v ("tone"); syn.m.drive = v ("drive"); syn.m.decay = v ("decay"); syn.m.punch = v ("punch");
    syn.vol = v ("listen") > 0.5f ? juce::Decibels::decibelsToGain (v ("vol"), -36.0f) : 0.0f;
    const bool midiOn = v ("midiOut") > 0.5f, live = v ("live") > 0.5f;
    acts.clear(); out.clear();
    // ingresso MIDI: il basso suona anche le note che arrivano (legato se c'e' gia' una nota tenuta = glide)
    for (const auto m : midi)
    {
        const auto& msg = m.getMessage(); const int at = juce::jlimit (0, N - 1, m.samplePosition);
        if (msg.isNoteOn()) { acts.push_back ({ at, 0, msg.getNoteNumber(), msg.getFloatVelocity(), held > 0, false }); ++held; }
        else if (msg.isNoteOff()) { acts.push_back ({ at, 1, msg.getNoteNumber(), 0, false, false }); held = juce::jmax (0, held - 1); }
        else if (msg.isAllNotesOff() || msg.isAllSoundOff()) { acts.push_back ({ at, 3, 0, 0, false, false }); held = 0; }
    }
    midi.clear();
    double p0 = 0, bpm = 120; bool playing = false;
    if (auto* ph = getPlayHead()) if (auto ps = ph->getPosition()) { playing = ps->getIsPlaying(); if (auto b = ps->getBpm()) bpm = *b; if (auto q = ps->getPpqPosition()) p0 = *q; }
    const bool preview = ! playing && v ("play") > 0.5f;
    if (preview) { p0 = ownPpq; ownPpq += N * bpm / 60.0 / sr; } else if (! playing) ownPpq = 0;
    uiBpm = bpm;
    const double spp = sr * 60.0 / bpm, p1 = p0 + N / spp;
    const juce::int64 s0 = (juce::int64) std::llround (p0 * spp);
    auto smp = [&] (double t) { return (juce::int64) std::llround (t * spp); };
    auto inBlk = [&] (double t) { const auto a = smp (t); return a >= s0 && a < s0 + N; };
    auto atOf = [&] (double t) { return (int) juce::jlimit ((juce::int64) 0, (juce::int64) N - 1, smp (t) - s0); };
    auto flushAll = [&] (int at)
    {
        for (int i = 0; i < np; ++i) out.addEvent (juce::MidiMessage::noteOff (1, pend[(size_t) i].note), at);
        np = 0; out.addEvent (juce::MidiMessage::allNotesOff (1), at); acts.push_back ({ at, 3, 0, 0, false, false });
    };
    if (allOffReq.exchange (false)) flushAll (0);
    if (! ((playing || preview) && live)) { if (lastEnd >= 0) flushAll (0); lastEnd = -1; uiPlaying = false; uiBeat = -1; uiSlot = -1; }
    else
    {
        if (lastEnd >= 0 && (p0 < lastEnd - 0.01 || p0 > lastEnd + 0.25)) flushAll (0);
        lastEnd = p1;
        auto P = part(); const double L = P->len();
        uiPlaying = true; uiBeat = std::fmod (juce::jmax (0.0, p0), L);
        uiSlot = P->chordAt.empty() ? -1 : juce::jlimit (0, (int) P->chordAt.size() - 1, (int) std::floor (uiBeat.load() / (L / (double) P->chordAt.size())));
        const long k0 = (long) std::floor (p0 / L) - 1, k1 = (long) std::floor (p1 / L) + 1;
        for (int pass = 0; pass < 2; ++pass)
            for (long k = k0; k <= k1; ++k)
            {
                const double off = (double) k * L;
                for (auto& e : P->ev)
                {
                    if (pass == 0)
                    {
                        const double t = off + e.beat + e.len; if (! inBlk (t)) continue;
                        out.addEvent (juce::MidiMessage::noteOff (1, e.note), atOf (t)); acts.push_back ({ atOf (t), 1, e.note, 0, false, false });
                        for (int i = 0; i < np; ++i) if (pend[(size_t) i].note == e.note) { pend[(size_t) i] = pend[(size_t) --np]; break; }
                    }
                    else
                    {
                        const double t = off + e.beat; if (! inBlk (t)) continue;
                        out.addEvent (juce::MidiMessage::noteOn (1, e.note, (juce::uint8) juce::jlimit (1, 127, juce::roundToInt (e.vel * 127))), atOf (t));
                        acts.push_back ({ atOf (t), 0, e.note, e.vel, e.art == 2, e.mute });
                        if (np < (int) pend.size()) pend[(size_t) np++] = { t + e.len, e.note };
                    }
                }
            }
    }
    std::stable_sort (acts.begin(), acts.end(), [] (const Act& a, const Act& b) { static const int pr[4] = { 2, 0, 1, 1 }; return a.at != b.at ? a.at < b.at : pr[a.type] < pr[b.type]; });
    float* L = buffer.getWritePointer (0); float* R = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : L;
    int cur = 0;
    auto renderTo = [&] (int to) { if (to > cur) { syn.render (L + cur, R + cur, to - cur); cur = to; } };
    for (auto& a : acts)
    {
        renderTo (a.at);
        if (a.type == 0) syn.hit (a.note, a.vel, a.legato, a.mute);
        else if (a.type == 1) syn.m.noteOff (a.note);
        else syn.m.allOff();
    }
    renderTo (N);
    if (buffer.getNumChannels() > 1 && R == L) {}
    if (midiOn) midi.swapWith (out);
}

void Processor::loadPreset (int i)
{
    const auto& P = presets(); i = juce::jlimit (0, (int) P.size() - 1, i);
    undo.beginNewTransaction();
    static const juce::StringArray keep { "vol", "listen", "live", "midiOut", "play" };
    for (auto* prm : getParameters()) if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (prm)) if (! keep.contains (rp->getParameterID())) rp->setValueNotifyingHost (rp->getDefaultValue());
    float style = 0;
    for (auto& kv : P[(size_t) i].p) { if (juce::String (kv.first) == "style") style = kv.second; if (auto* rp = apvts.getParameter (kv.first)) rp->setValueNotifyingHost (rp->convertTo0to1 (kv.second)); }
    applyStyle ((int) style);
    for (auto& kv : P[(size_t) i].p) { const juce::String k (kv.first); if (k == "glide" || k == "len" || k == "ghost") setV (kv.first, kv.second); }   // il preset vince sullo stile
    setProg (P[(size_t) i].prog); setSeed (1);
    curPreset = i; presetName = juce::String (P[(size_t) i].name).fromFirstOccurrenceOf (" | ", false, false);
    regenerate();
}
bool Processor::savePresetFile (const juce::File& f)
{
    auto s = apvts.copyState();
    if (auto x = s.createXml()) { const bool ok = x->writeTo (f); if (ok) { presetName = f.getFileNameWithoutExtension(); curPreset = -1; } return ok; }
    return false;
}
bool Processor::loadPresetFile (const juce::File& f)
{
    auto x = juce::XmlDocument::parse (f); if (x == nullptr) return false;
    auto t = juce::ValueTree::fromXml (*x); if (! t.isValid() || ! t.hasType (apvts.state.getType())) return false;
    undo.beginNewTransaction();
    for (auto* prm : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (prm))
        {
            if (rp->getParameterID() == "vol" || rp->getParameterID() == "play") continue;
            for (auto c : t) if (c.getProperty ("id").toString() == rp->getParameterID()) rp->setValueNotifyingHost (rp->convertTo0to1 ((float) c.getProperty ("value")));
        }
    for (auto* k : { "prog", "seed", "figSeq", "grid" }) if (t.hasProperty (k)) apvts.state.setProperty (k, t.getProperty (k), &undo);
    presetName = f.getFileNameWithoutExtension(); curPreset = -1; regenerate();
    return true;
}
void Processor::getStateInformation (juce::MemoryBlock& d)
{
    auto st = apvts.copyState(); st.setProperty ("presetName", presetName, nullptr); st.setProperty ("curPreset", curPreset, nullptr);
    if (auto x = st.createXml()) copyXmlToBinary (*x, d);
}
void Processor::setStateInformation (const void* data, int size)
{
    auto x = getXmlFromBinary (data, size); if (! x) return;
    auto t = juce::ValueTree::fromXml (*x); if (! t.isValid() || ! t.hasType (apvts.state.getType())) return;
    apvts.state.removeListener (this);
    apvts.replaceState (t);
    if (! apvts.state.hasProperty ("prog")) apvts.state.setProperty ("prog", "0,5,2,6", nullptr);
    if (! apvts.state.hasProperty ("seed")) apvts.state.setProperty ("seed", 1, nullptr);
    presetName = apvts.state.getProperty ("presetName", "Init").toString(); curPreset = apvts.state.getProperty ("curPreset", -1);
    apvts.state.addListener (this);
    undo.clearUndoHistory(); regenerate();
}
juce::AudioProcessorEditor* Processor::createEditor() { return new Editor (*this); }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new bss::Processor(); }
