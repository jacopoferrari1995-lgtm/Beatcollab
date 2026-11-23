#include "GtrProcessor.h"
#include "GtrEditor.h"

namespace gtr
{
juce::File presetFolder()
{
    auto d = juce::File::getSpecialLocation (juce::File::userMusicDirectory).getChildFile ("Jackpot").getChildFile ("Guitar Preset");
    d.createDirectory(); return d;
}
static const char* kGenIds[] = { "key", "scale", "genre", "mode", "pat", "vstyle", "tuning", "bars", "perBar", "color", "pos", "density", "dyn", "strum", "human", "swing", "ring", "emb", "tech", "borrow", "mood", "rest", "lyBase", "lyArp", "lyLead" };

// preset: gruppo | nome. Ogni preset e' una parte completa (giro, modo, suonata, suono); il giro parte dal grado indicato
const std::vector<GPreset>& presets()
{
    using V = std::vector<std::pair<const char*, float>>;
    static const std::vector<GPreset> P = {
        { "INIT | Giro base", "Arpeggio trap su La minore, i - VI - III - VII, nylon. Il punto di partenza neutro.", V {}, { 0, 5, 2, 6 } },
        { "COMPOSIZIONE | Trap a tre chitarre", "Accordi pizzicati a sinistra, arpeggio triste in alto a destra, melodia cantabile al centro. 8 battute.",
          V { { "key", 9 }, { "scale", MINORE }, { "genre", TRAP }, { "mood", MALINCONICO }, { "mode", COMPOSIZIONE }, { "lyBase", 7 }, { "lyArp", 2 }, { "lyLead", 1 }, { "bars", 3 }, { "vstyle", V_OPEN }, { "pos", 2 }, { "ring", 0.75f }, { "body", NYLON }, { "space", 0.3f } }, { 0, 5, 2, 6 } },
        { "COMPOSIZIONE | R&B notte", "Neo soul a sinistra, roll morbido a destra, run al centro. Fa maggiore ii - V - I.",
          V { { "key", 5 }, { "scale", MAGGIORE }, { "genre", RNB }, { "mood", CALDO }, { "mode", COMPOSIZIONE }, { "lyBase", 4 }, { "lyArp", 7 }, { "lyLead", 4 }, { "bars", 3 }, { "vstyle", V_JAZZ }, { "pos", 3 }, { "color", 3 }, { "swing", 0.2f }, { "body", PULITA }, { "space", 0.35f } }, { 1, 4, 0, 0 } },
        { "COMPOSIZIONE | Pop acustico", "Pennata pop a sinistra, fingerpicking in alto a destra, melodia al centro. Sol maggiore.",
          V { { "key", 7 }, { "scale", MAGGIORE }, { "genre", POP }, { "mood", APERTO }, { "mode", COMPOSIZIONE }, { "lyBase", 8 }, { "lyArp", 4 }, { "lyLead", 1 }, { "bars", 3 }, { "vstyle", V_OPEN }, { "pos", 2 }, { "body", ACUSTICA } }, { 0, 4, 5, 3 } },
        { "MELODIA | Cantabile", "Melodia da cantare su 8 battute: domanda e risposta, il motivo torna, note d'arrivo sugli accordi, respiri. Elettrica pulita.",
          V { { "key", 9 }, { "scale", MINORE }, { "genre", POP }, { "mood", MALINCONICO }, { "mode", LEAD }, { "pat", 0 }, { "bars", 3 }, { "pos", 5 }, { "tech", 0.35f }, { "density", 0.5f }, { "rest", 0.35f }, { "body", PULITA }, { "space", 0.3f }, { "width", 0.3f } }, { 0, 5, 2, 6 } },
        { "MELODIA | Aperta in maggiore", "Linea cantabile in Sol maggiore, arco che sale nella seconda frase. Nylon.",
          V { { "key", 7 }, { "scale", MAGGIORE }, { "genre", POP }, { "mood", APERTO }, { "mode", LEAD }, { "pat", 0 }, { "bars", 3 }, { "pos", 4 }, { "tech", 0.3f }, { "density", 0.45f }, { "body", NYLON } }, { 0, 4, 5, 3 } },
        { "MOOD | Ipnotico", "Due accordi, un arpeggio che gira sempre uguale con la nota alta ferma. Per loop trap e drill lenti.",
          V { { "key", 1 }, { "scale", MINORE }, { "genre", TRAP }, { "mood", IPNOTICO }, { "mode", ARPEGGIO }, { "pat", 1 }, { "bars", 3 }, { "vstyle", V_OPEN }, { "pos", 4 }, { "ring", 0.8f }, { "density", 0.55f }, { "body", NYLON }, { "rest", 0.0f }, { "space", 0.3f } }, { 0, 5 } },
        { "MOOD | Aperto", "Corde a vuoto, sus e add9, note comuni che restano in cima. Acustica.",
          V { { "key", 4 }, { "scale", MAGGIORE }, { "genre", FOLK }, { "mood", APERTO }, { "mode", ARPEGGIO }, { "pat", 3 }, { "bars", 3 }, { "vstyle", V_OPEN }, { "pos", 2 }, { "ring", 0.85f }, { "body", ACUSTICA }, { "width", 0.5f } }, { 0, 4, 5, 3 } },
        { "MOOD | Sognante", "Accordi di nona sfogliati, un accordo ogni due battute, molto spazio. Elettrica pulita con plate.",
          V { { "key", 5 }, { "scale", MAGGIORE }, { "genre", LOFI }, { "mood", SOGNANTE }, { "mode", ARPEGGIO }, { "pat", 6 }, { "bars", 3 }, { "perBar", 2 }, { "vstyle", V_JAZZ }, { "pos", 5 }, { "ring", 0.9f }, { "body", PULITA }, { "rest", 0.5f }, { "space", 0.45f } }, { 3, 2, 1, 0 } },
        { "MOOD | Teso", "Ritmica stretta e bassa, stoppate a fine frase. Re frigio.",
          V { { "key", 2 }, { "scale", FRIGIA }, { "genre", DRILL }, { "mood", TESO }, { "mode", RITMICA }, { "pat", 5 }, { "bars", 2 }, { "vstyle", V_POWER }, { "pos", 5 }, { "body", CRUNCH }, { "drive", 0.45f } }, { 0, 1, 0, 6 } },
        { "TRAP | Triste in La minore", "Arpeggio lento sulle corde a vuoto, le note si lasciano suonare. Il classico loop di chitarra trap.",
          V { { "key", 9 }, { "scale", MINORE }, { "genre", TRAP }, { "mode", ARPEGGIO }, { "pat", 1 }, { "vstyle", V_OPEN }, { "pos", 2 }, { "ring", 0.85f }, { "emb", 0.35f }, { "density", 0.55f }, { "body", NYLON }, { "space", 0.3f } }, { 0, 5, 2, 6 } },
        { "TRAP | Notte in salita", "Arpeggio in sedicesimi su e giu', Do minore con il VI e il VII: cupo e in movimento.",
          V { { "key", 0 }, { "scale", MINORE }, { "genre", TRAP }, { "mode", ARPEGGIO }, { "pat", 0 }, { "vstyle", V_BARRE }, { "pos", 3 }, { "ring", 0.6f }, { "density", 0.7f }, { "body", NYLON }, { "space", 0.35f } }, { 0, 5, 6, 4 } },
        { "TRAP | Lead scuro", "Melodia lenta con bend, pre-bend e vibrato in Mi minore, pentatonica. Da mettere sopra un loop di accordi.",
          V { { "key", 4 }, { "scale", MINORE }, { "genre", TRAP }, { "mode", LEAD }, { "pat", 1 }, { "pos", 7 }, { "tech", 0.7f }, { "density", 0.45f }, { "body", PULITA }, { "space", 0.45f }, { "width", 0.4f } }, { 0, 5, 2, 6 } },
        { "DRILL | Armonica minore", "Terzine scure in Do# armonica minore: il V maggiore e' gia' nella scala.",
          V { { "key", 1 }, { "scale", ARMONICA }, { "genre", DRILL }, { "mode", ARPEGGIO }, { "pat", 5 }, { "vstyle", V_BARRE }, { "pos", 4 }, { "ring", 0.6f }, { "density", 0.6f }, { "body", NYLON }, { "space", 0.3f } }, { 0, 5, 4, 0 } },
        { "DRILL | Lead in terzine", "Frasi in terzine con scivolate d'entrata, armonica minore. Il suono delle chitarre drill UK / NY.",
          V { { "key", 6 }, { "scale", ARMONICA }, { "genre", DRILL }, { "mode", LEAD }, { "pat", 2 }, { "pos", 5 }, { "tech", 0.6f }, { "density", 0.55f }, { "body", NYLON }, { "space", 0.3f } }, { 0, 5, 4, 0 } },
        { "EMO RAP | Ballata 6/8", "Arpeggio in 6/8 (terzine) su VI - VII - i, acustica. Il suono emo rap.",
          V { { "key", 4 }, { "scale", MINORE }, { "genre", EMO }, { "mode", ARPEGGIO }, { "pat", 4 }, { "vstyle", V_OPEN }, { "pos", 2 }, { "ring", 0.8f }, { "color", 3 }, { "body", ACUSTICA }, { "space", 0.3f } }, { 5, 6, 0, 0 } },
        { "EMO RAP | Pop punk", "Power chord in palm mute che si aprono a fine battuta, distorsione: pop punk / emo.",
          V { { "key", 4 }, { "scale", MINORE }, { "genre", EMO }, { "mode", RITMICA }, { "pat", 5 }, { "vstyle", V_POWER }, { "pos", 2 }, { "density", 0.6f }, { "body", CRUNCH }, { "drive", 0.6f }, { "width", 0.7f } }, { 0, 5, 2, 6 } },
        { "R&B | Neo soul", "Accordi di nona sfogliati, la nota alta arriva con hammer-on e scivolate. ii - V - I in Fa.",
          V { { "key", 5 }, { "scale", MAGGIORE }, { "genre", RNB }, { "mode", ACCORDI }, { "pat", 3 }, { "vstyle", V_JAZZ }, { "color", 2 }, { "pos", 5 }, { "emb", 0.7f }, { "density", 0.6f }, { "body", PULITA }, { "tone", 0.4f }, { "space", 0.35f } }, { 1, 4, 0, 0 } },
        { "R&B | Stab", "Colpi corti di settima sulle corde alte, sincopati. Mi bemolle maggiore, IV - iii - vi - I.",
          V { { "key", 3 }, { "scale", MAGGIORE }, { "genre", RNB }, { "mode", ACCORDI }, { "pat", 2 }, { "vstyle", V_TRIAD }, { "color", 1 }, { "pos", 6 }, { "density", 0.6f }, { "swing", 0.25f }, { "body", PULITA }, { "space", 0.25f } }, { 3, 2, 5, 0 } },
        { "R&B | Arpeggio morbido", "Accordo sfogliato e note sparse, none. Re minore dorico.",
          V { { "key", 2 }, { "scale", DORICA }, { "genre", RNB }, { "mode", ARPEGGIO }, { "pat", 6 }, { "vstyle", V_JAZZ }, { "color", 2 }, { "pos", 5 }, { "ring", 0.75f }, { "swing", 0.2f }, { "body", PULITA }, { "space", 0.35f } }, { 0, 3, 0, 3 } },
        { "R&B | Run", "Lead con corse in sedicesimi, legati (hammer-on e pull-off) che finiscono sulla nota dell'accordo.",
          V { { "key", 5 }, { "scale", MAGGIORE }, { "genre", RNB }, { "mode", LEAD }, { "pat", 3 }, { "pos", 8 }, { "tech", 0.65f }, { "density", 0.55f }, { "body", PULITA }, { "space", 0.3f } }, { 1, 4, 0, 0 } },
        { "POP | Acustica", "Pennata giu' - giu' su - su giu' su su accordi aperti, Sol maggiore I - V - vi - IV.",
          V { { "key", 7 }, { "scale", MAGGIORE }, { "genre", POP }, { "mode", RITMICA }, { "pat", 0 }, { "vstyle", V_OPEN }, { "pos", 2 }, { "color", 3 }, { "density", 0.55f }, { "body", ACUSTICA }, { "width", 0.5f } }, { 0, 4, 5, 3 } },
        { "POP | Ballata", "Pennate larghe sulla battuta, acustica: per le strofe.",
          V { { "key", 2 }, { "scale", MAGGIORE }, { "genre", POP }, { "mode", RITMICA }, { "pat", 1 }, { "vstyle", V_OPEN }, { "pos", 2 }, { "density", 0.45f }, { "strum", 0.5f }, { "body", ACUSTICA }, { "space", 0.3f } }, { 5, 3, 0, 4 } },
        { "REGGAETON | Dembow", "Pennate sul ritmo del dembow (3+3+2), nylon, La minore.",
          V { { "key", 9 }, { "scale", MINORE }, { "genre", REGGAETON }, { "mode", RITMICA }, { "pat", 3 }, { "vstyle", V_OPEN }, { "pos", 2 }, { "density", 0.55f }, { "body", NYLON }, { "space", 0.25f } }, { 0, 5, 2, 6 } },
        { "REGGAETON | Lead latino", "Melodia sincopata 3-3-2 con doppie note in terza, nylon.",
          V { { "key", 9 }, { "scale", MINORE }, { "genre", REGGAETON }, { "mode", LEAD }, { "pat", 5 }, { "pos", 5 }, { "tech", 0.5f }, { "body", NYLON }, { "space", 0.3f } }, { 0, 5, 2, 6 } },
        { "AFRO | Highlife", "Arpeggio intrecciato sulle corde alte, elettrica pulita. Do maggiore I - IV - vi - V.",
          V { { "key", 0 }, { "scale", MAGGIORE }, { "genre", AFRO }, { "mode", ARPEGGIO }, { "pat", 7 }, { "vstyle", V_TRIAD }, { "pos", 5 }, { "ring", 0.35f }, { "density", 0.65f }, { "swing", 0.15f }, { "body", PULITA }, { "space", 0.25f } }, { 0, 3, 5, 4 } },
        { "AFRO | Pennate", "Pennate leggere in sedicesimi sulle corde alte, afrobeats.",
          V { { "key", 7 }, { "scale", MAGGIORE }, { "genre", AFRO }, { "mode", RITMICA }, { "pat", 6 }, { "vstyle", V_TRIAD }, { "pos", 7 }, { "density", 0.55f }, { "swing", 0.2f }, { "body", PULITA } }, { 1, 4, 0, 0 } },
        { "BOOM BAP | Jazz", "Accordi di settima suonati su 1 e 3, voicing jazz, elettrica scura. ii - V - I - vi in Si bemolle.",
          V { { "key", 10 }, { "scale", MAGGIORE }, { "genre", BOOMBAP }, { "mode", ACCORDI }, { "pat", 1 }, { "vstyle", V_JAZZ }, { "color", 1 }, { "pos", 5 }, { "swing", 0.35f }, { "body", PULITA }, { "tone", 0.3f }, { "space", 0.3f } }, { 1, 4, 0, 5 } },
        { "LO-FI | Accordi", "Accordi di nona sfogliati lenti, swing, nylon scuro.",
          V { { "key", 5 }, { "scale", MAGGIORE }, { "genre", LOFI }, { "mode", ACCORDI }, { "pat", 4 }, { "vstyle", V_JAZZ }, { "color", 2 }, { "pos", 5 }, { "density", 0.7f }, { "swing", 0.4f }, { "body", NYLON }, { "tone", 0.35f }, { "space", 0.4f } }, { 1, 4, 0, 5 } },
        { "FUNK | Chicken scratch", "Sedicesimi stoppati con colpi secchi di triade, misolidio.",
          V { { "key", 4 }, { "scale", MISOLIDIA }, { "genre", FUNK }, { "mode", RITMICA }, { "pat", 4 }, { "vstyle", V_TRIAD }, { "color", 1 }, { "pos", 7 }, { "density", 0.6f }, { "strum", 0.15f }, { "body", PULITA }, { "tone", 0.65f } }, { 0, 3, 0, 3 } },
        { "FOLK | Travis", "Fingerpicking Travis: il pollice alterna i bassi, le dita in levare. Sol maggiore.",
          V { { "key", 7 }, { "scale", MAGGIORE }, { "genre", FOLK }, { "mode", ARPEGGIO }, { "pat", 2 }, { "vstyle", V_OPEN }, { "pos", 2 }, { "ring", 0.7f }, { "body", ACUSTICA }, { "space", 0.2f } }, { 0, 3, 0, 4 } },
        { "FOLK | Basso e pennata", "Basso sul tempo e pennata in levare (boom-chick), acustica.",
          V { { "key", 2 }, { "scale", MAGGIORE }, { "genre", FOLK }, { "mode", RITMICA }, { "pat", 8 }, { "vstyle", V_OPEN }, { "pos", 2 }, { "body", ACUSTICA } }, { 0, 4, 5, 3 } },
        { "ROCK | Palm mute", "Power chord in palm mute in ottavi, crunch.",
          V { { "key", 9 }, { "scale", MINORE }, { "genre", ROCK }, { "mode", RITMICA }, { "pat", 5 }, { "vstyle", V_POWER }, { "pos", 5 }, { "density", 0.5f }, { "body", CRUNCH }, { "drive", 0.7f }, { "width", 0.7f } }, { 0, 6, 3, 0 } },
        { "BLUES | Lead", "Lead pentatonico con bend, doppie note e vibrato, La minore, crunch leggero.",
          V { { "key", 9 }, { "scale", MINORE }, { "genre", ROCK }, { "mode", LEAD }, { "pat", 4 }, { "pos", 5 }, { "tech", 0.75f }, { "body", CRUNCH }, { "drive", 0.35f }, { "space", 0.3f } }, { 0, 3, 0, 4 } },
        { "LATIN | Romanza", "Arpeggio classico su nylon, i - iv - V - i con il V preso dall'armonica (interscambio).",
          V { { "key", 4 }, { "scale", MINORE }, { "genre", LATIN }, { "mode", ARPEGGIO }, { "pat", 8 }, { "vstyle", V_OPEN }, { "pos", 2 }, { "ring", 0.7f }, { "body", NYLON }, { "space", 0.3f } }, { 0, 3, 14, 0 } },
        { "REGGAE | Skank", "Il colpo in levare stoppato subito, elettrica pulita.",
          V { { "key", 7 }, { "scale", MAGGIORE }, { "genre", POP }, { "mode", RITMICA }, { "pat", 2 }, { "vstyle", V_TRIAD }, { "pos", 7 }, { "density", 0.45f }, { "body", PULITA }, { "space", 0.3f } }, { 0, 3, 4, 3 } },
        { "6/8 | Pennate in terzine", "Pennate in 12/8, acustica: ballate e trap lente.",
          V { { "key", 2 }, { "scale", MINORE }, { "genre", EMO }, { "mode", RITMICA }, { "pat", 7 }, { "vstyle", V_OPEN }, { "pos", 2 }, { "body", ACUSTICA }, { "space", 0.3f } }, { 0, 5, 2, 6 } },
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
    StringArray md; for (int i = 0; i < kModes; ++i) md.add (modeName (i));
    StringArray vs; for (int i = 0; i < kVStyles; ++i) vs.add (vstyleName (i));
    StringArray tu; for (int i = 0; i < kTunings; ++i) tu.add (tuningName (i));
    StringArray bd; for (int i = 0; i < kBodies; ++i) bd.add (bodyName (i));
    ch ("key", "Tonalita'", keys, 9); ch ("scale", "Scala", sc, MINORE); ch ("genre", "Genere", gn, TRAP); ch ("mode", "Modo", md, ARPEGGIO);
    ch ("pat", "Pattern", { "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12" }, 1);
    { StringArray b { "OFF" }; for (auto& n : patternNames (ACCORDI)) b.add ("ACCORDI " + n); for (auto& n : patternNames (RITMICA)) b.add ("RITMICA " + n); ch ("lyBase", "Base", b, 2); }
    { StringArray a { "OFF" }; for (auto& n : patternNames (ARPEGGIO)) a.add (n); ch ("lyArp", "Arpeggio", a, 2); }
    { StringArray l { "OFF" }; for (auto& n : patternNames (LEAD)) l.add (n); ch ("lyLead", "Lead", l, 1); }
    ch ("vstyle", "Voicing", vs, V_OPEN); ch ("tuning", "Accordatura", tu, STANDARD);
    ch ("bars", "Battute", { "1", "2", "4", "8" }, 2); ch ("perBar", "Accordi per battuta", { "1", "2", "1/2" }, 0);
    { StringArray mo; for (int i = 0; i < kMoods; ++i) mo.add (moodName (i)); ch ("mood", "Mood", mo, 0); }
    fl ("rest", "Pause", 0.3f);
    ch ("color", "Colore", { "AUTO", "TRIADI", "SETTIME", "NONE", "SUS / ADD9" }, 0);
    p.push_back (std::make_unique<AudioParameterInt> (ParameterID { "pos", 1 }, "Posizione", 0, 12, 2));
    fl ("density", "Densita'", 0.5f); fl ("dyn", "Dinamica", 0.6f); fl ("strum", "Pennata", 0.35f); fl ("human", "Umano", 0.4f); fl ("swing", "Swing", 0.0f);
    fl ("ring", "Risuona", 0.7f); fl ("emb", "Abbellimenti", 0.3f); fl ("tech", "Tecnica", 0.5f); fl ("borrow", "Interscambio", 0.0f);
    ch ("body", "Suono", bd, NYLON); fl ("tone", "Tono", 0.5f); fl ("bodyAmt", "Corpo", 0.6f); fl ("drive", "Drive", 0.5f); fl ("space", "Spazio", 0.25f); fl ("width", "Larghezza", 0.3f);
    fl ("vol", "Volume", 0.0f, -36.0f, 6.0f);
    bo ("listen", "Suono acceso", true); bo ("live", "Giro in play", true); bo ("midiOut", "MIDI out", true); bo ("chStr", "Canale per corda", false); bo ("play", "Prova", false);
    return { p.begin(), p.end() };
}

Processor::Processor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, &undo, "PARAMS", layout())
{
    for (auto* id : kGenIds) apvts.addParameterListener (id, this);
    apvts.state.setProperty ("prog", "0,5,2,6", nullptr); apvts.state.setProperty ("seed", 1, nullptr);
    apvts.state.setProperty ("seedB", 1, nullptr); apvts.state.setProperty ("seedA", 2, nullptr); apvts.state.setProperty ("seedL", 3, nullptr);
    apvts.state.addListener (this);
    inStr.fill (-1);
    regenerate();
}
Processor::~Processor() { cancelPendingUpdate(); apvts.state.removeListener (this); for (auto* id : kGenIds) apvts.removeParameterListener (id, this); }
void Processor::parameterChanged (const juce::String&, float) { triggerAsyncUpdate(); }

std::vector<int> Processor::prog() const
{
    std::vector<int> r; for (auto& t : juce::StringArray::fromTokens (apvts.state.getProperty ("prog", "0,5,2,6").toString(), ",", "")) if (t.trim().isNotEmpty()) r.push_back (juce::jlimit (0, 899, t.getIntValue()));
    if (r.empty()) r = { 0, 5, 2, 6 };
    if (r.size() > 8) r.resize (8);
    return r;
}
void Processor::setProg (const std::vector<int>& p)
{
    juce::StringArray s; for (int x : p) s.add (juce::String (x));
    apvts.state.setProperty ("prog", s.joinIntoString (","), &undo);
}
void Processor::generaGiro()
{
    undo.beginNewTransaction();
    const auto s = juce::Random::getSystemRandom().nextInt (0x7ffffff) | 1;
    const auto np = newProgression (vi ("genre"), vi ("mood"), vi ("scale"), barsOf (vi ("bars")), (juce::uint32) s);
    setProg (np.prog); setSeed ((juce::uint32) s); curPreset = -1;
    setV ("perBar", np.perBar == 2 ? 1.0f : np.perBar == 0 ? 2.0f : 0.0f);
    // anche il modo di suonare: un pattern adatto al genere (e in composizione i tre strati)
    juce::Random R ((juce::int64) s);
    const int g = vi ("genre"), mode = vi ("mode");
    struct GP { std::vector<int> acc, rit, arp, lead; };
    static const GP gp[kGenres] = {
        { { 0, 6, 4 }, { 9, 1 }, { 1, 0, 10, 9 }, { 1, 0 } },          // TRAP
        { { 6, 0 }, { 9, 5 }, { 5, 1, 10 }, { 2, 1 } },                // DRILL
        { { 0, 4 }, { 7, 0, 1 }, { 4, 1, 0 }, { 0, 1 } },              // EMO
        { { 3, 2, 4 }, { 10, 4 }, { 6, 11, 10 }, { 3, 0 } },           // R&B
        { { 0, 1, 5 }, { 0, 1 }, { 3, 0, 9 }, { 0 } },                 // POP
        { { 6, 2 }, { 3 }, { 7, 11 }, { 5, 0 } },                      // REGGAETON
        { { 2, 6 }, { 6, 10 }, { 7, 11 }, { 0, 5 } },                  // AFRO
        { { 1, 3 }, { 4, 10 }, { 6, 2 }, { 4, 3 } },                   // BOOM BAP
        { { 4, 3 }, { 10, 1 }, { 6, 10, 3 }, { 0, 3 } },               // LO-FI
        { { 1, 0 }, { 5, 0 }, { 0, 3 }, { 4, 0 } },                    // ROCK
        { { 2 }, { 4 }, { 11, 7 }, { 3, 4 } },                         // FUNK
        { { 0, 4 }, { 8, 0 }, { 2, 3 }, { 0 } },                       // FOLK
        { { 6, 0 }, { 3, 7 }, { 8, 9 }, { 5, 0 } } };                  // LATIN
    const auto& G = gp[juce::jlimit (0, kGenres - 1, g)];
    auto pick = [&R] (const std::vector<int>& v) { return v[(size_t) R.nextInt ((int) v.size())]; };
    if (mode == ACCORDI) setV ("pat", (float) pick (G.acc));
    else if (mode == RITMICA) setV ("pat", (float) pick (G.rit));
    else if (mode == ARPEGGIO) setV ("pat", (float) pick (G.arp));
    else if (mode == LEAD) setV ("pat", (float) pick (G.lead));
    else
    {
        const int na = patternNames (ACCORDI).size();
        setV ("lyBase", R.nextBool() ? (float) (1 + pick (G.acc)) : (float) (1 + na + pick (G.rit)));
        setV ("lyArp", (float) (1 + pick (G.arp))); setV ("lyLead", (float) (1 + pick (G.lead)));
        for (auto* k : { "seedB", "seedA", "seedL" }) apvts.state.setProperty (k, R.nextInt (0x7ffffff) | 1, &undo);
    }
}
void Processor::varia()
{
    // stesso giro e stessi pattern, nuova suonata (in composizione cambiano tutti e tre gli strati)
    undo.beginNewTransaction(); auto& R = juce::Random::getSystemRandom();
    setSeed ((juce::uint32) (R.nextInt (0x7ffffff) | 1));
    for (auto* k : { "seedB", "seedA", "seedL" }) apvts.state.setProperty (k, R.nextInt (0x7ffffff) | 1, &undo);
}

Settings Processor::settings() const
{
    Settings S;
    S.key = vi ("key"); S.scale = vi ("scale"); S.genre = vi ("genre"); S.mode = vi ("mode");
    S.pattern = juce::jlimit (0, patternNames (S.mode).size() - 1, vi ("pat"));
    S.vstyle = vi ("vstyle"); S.tuning = vi ("tuning"); S.bars = barsOf (vi ("bars")); { static const int pb[3] = { 1, 2, 0 }; S.perBar = pb[juce::jlimit (0, 2, vi ("perBar"))]; }
    S.mood = vi ("mood"); S.space = v ("rest");
    { const int b = vi ("lyBase"), na = patternNames (ACCORDI).size(); S.lyBase = b == 0 ? -1 : b <= na ? b - 1 : 100 + (b - 1 - na); }
    S.lyArp = vi ("lyArp") - 1; S.lyLead = vi ("lyLead") - 1;
    S.seedB = seedOf ("seedB"); S.seedA = seedOf ("seedA"); S.seedL = seedOf ("seedL");
    S.color = (float) (vi ("color") - 1); S.pos = vi ("pos");
    S.density = v ("density"); S.dyn = v ("dyn"); S.strum = v ("strum"); S.human = v ("human"); S.swing = v ("swing");
    S.ring = v ("ring"); S.emb = v ("emb"); S.tech = v ("tech"); S.borrow = v ("borrow");
    S.prog = prog(); S.seed = seed(); S.bpm = genBpm;
    return S;
}
juce::String Processor::settingsKey() const
{
    const auto S = settings(); juce::String k;
    auto f = [] (float x) { return juce::String (juce::roundToInt (x * 1000.0f)); };   // arrotondati: annulla / ripeti ridanno lo stesso valore
    k << S.key << "|" << S.scale << "|" << S.genre << "|" << S.mode << "|" << S.pattern << "|" << S.vstyle << "|" << S.tuning << "|" << S.bars << "|" << S.perBar << "|" << f (S.color) << "|" << S.pos
      << "|" << f (S.density) << "|" << f (S.dyn) << "|" << f (S.strum) << "|" << f (S.human) << "|" << f (S.swing) << "|" << f (S.ring) << "|" << f (S.emb) << "|" << f (S.tech) << "|" << f (S.borrow) << "|" << S.mood << "|" << f (S.space)
      << "|" << S.lyBase << "|" << S.lyArp << "|" << S.lyLead << "|" << (int) S.seed << "|" << (int) S.seedB << "|" << (int) S.seedA << "|" << (int) S.seedL << "|" << apvts.state.getProperty ("prog").toString();
    return k;
}
static juce::String serialize (const Part& P)
{
    juce::String s;
    for (auto& e : P.ev) s << "n" << e.beat << "," << e.len << "," << e.note << "," << e.vel << "," << e.str << "," << e.fret << "," << ((e.mute ? 1 : 0) | (e.pm ? 2 : 0) | (e.lead ? 4 : 0) | (e.art << 3)) << "," << e.layer << "," << e.slot << ";";
    for (auto& b : P.bend) s << "b" << b.beat << "," << b.semis << "," << b.str << ";";
    return s;
}
static void deserialize (const juce::String& s, Part& P)
{
    P.ev.clear(); P.bend.clear();
    for (auto& t : juce::StringArray::fromTokens (s, ";", ""))
    {
        if (t.isEmpty()) continue;
        const auto v = juce::StringArray::fromTokens (t.substring (1), ",", "");
        if (t[0] == 'n' && v.size() >= 9)
        {
            Ev e; e.beat = v[0].getDoubleValue(); e.len = v[1].getDoubleValue(); e.note = v[2].getIntValue(); e.vel = v[3].getFloatValue(); e.str = juce::jlimit (0, kStr - 1, v[4].getIntValue()); e.fret = v[5].getIntValue();
            const int f = v[6].getIntValue(); e.mute = f & 1; e.pm = f & 2; e.lead = f & 4; e.art = (f >> 3) & 3; e.layer = juce::jlimit (0, 2, v[7].getIntValue()); e.slot = v[8].getIntValue();
            if (e.len > 0 && e.beat >= 0) P.ev.push_back (e);
        }
        else if (t[0] == 'b' && v.size() >= 3) P.bend.push_back ({ v[0].getDoubleValue(), v[1].getFloatValue(), v[2].getIntValue() });
    }
    std::stable_sort (P.ev.begin(), P.ev.end(), [] (const Ev& a, const Ev& b) { return a.beat < b.beat; });
}
void Processor::commitEdit (const Part& p)
{
    undo.beginNewTransaction();
    apvts.state.setProperty ("editKey", settingsKey(), &undo);
    apvts.state.setProperty ("edit", serialize (p), &undo);
    auto np = std::make_shared<Part> (p); std::atomic_store (&partPtr, std::shared_ptr<const Part> (np));
}
void Processor::clearEdit() { undo.beginNewTransaction(); apvts.state.setProperty ("edit", "", &undo); }
void Processor::rerollLayer (int layer)
{
    static const char* k[3] = { "seedB", "seedA", "seedL" };
    undo.beginNewTransaction(); apvts.state.setProperty (k[juce::jlimit (0, 2, layer)], juce::Random::getSystemRandom().nextInt (0x7ffffff) | 1, &undo);
}
void Processor::regenerate()
{
    auto p = std::make_shared<Part> (generateAny (settings()));
    if (edited()) deserialize (apvts.state.getProperty ("edit").toString(), *p);
    std::atomic_store (&partPtr, std::shared_ptr<const Part> (p));
}
void Processor::prepareToPlay (double s, int block)
{
    sr = s; maxBlock = juce::jmax (block, 512);
    for (auto& y : syn) { y.body = vi ("body"); y.prepare (sr, 4096); } acts.reserve (4096);
    np = 0; lastEnd = -1; ownPpq = 0; inStr.fill (-1);
}

juce::String Processor::clipName() const
{
    const auto S = settings();
    return juce::String ("Guitar ") + noteName (S.key) + (isMinorScale (S.scale) ? "m " : " ") + genreName (S.genre) + " " + modeName (S.mode) + (S.mode == COMPOSIZIONE ? juce::String() : " " + patternNames (S.mode)[S.pattern]);
}
juce::File Processor::exportMidi (int layer)
{
    auto P = part(); const double T = 960; const bool comp = vi ("mode") == COMPOSIZIONE, cs = v ("chStr") > 0.5f && ! comp;
    juce::MidiFile mf; mf.setTicksPerQuarterNote ((int) T);
    juce::MidiMessageSequence meta;
    meta.addEvent (juce::MidiMessage::tempoMetaEvent ((int) (60000000.0 / juce::jmax (40.0, uiBpm.load()))), 0);
    meta.addEvent (juce::MidiMessage::timeSignatureMetaEvent (4, 4), 0); mf.addTrack (meta);
    static const char* ln[3] = { "Base", "Arpeggio", "Lead" };
    // in composizione ogni strato e' una traccia (trascinato in Arrangement diventa tre tracce); un solo strato se richiesto
    const int l0 = comp ? (layer >= 0 ? layer : 0) : 0, l1 = comp ? (layer >= 0 ? layer : 2) : 0;
    for (int ly = l0; ly <= l1; ++ly)
    {
        juce::MidiMessageSequence seq; seq.addEvent (juce::MidiMessage::textMetaEvent (3, comp ? juce::String ("Guitar ") + ln[ly] : clipName()), 0);
        int n = 0;
        for (auto& e : P->ev)
        {
            if (comp && e.layer != ly) continue;
            const int c = comp ? 1 + ly : cs ? 2 + e.str : 1, vel = juce::jlimit (1, 127, juce::roundToInt (e.vel * 127));
            seq.addEvent (juce::MidiMessage::noteOn (c, e.note, (juce::uint8) vel), e.beat * T);
            seq.addEvent (juce::MidiMessage::noteOff (c, e.note), juce::jmax (e.beat * T + 6, (e.beat + e.len) * T)); ++n;
        }
        if (! comp || ly == 2)
            for (auto& b : P->bend)
            {
                const int pw = juce::jlimit (0, 16383, 8192 + juce::roundToInt (b.semis / 2.0f * 8191.0f));
                seq.addEvent (juce::MidiMessage::pitchWheel (comp ? 3 : cs ? 2 + b.str : 1, pw), b.beat * T);
            }
        if (n == 0 && comp && layer < 0) continue;
        seq.addEvent (juce::MidiMessage::endOfTrack(), P->len() * T);
        seq.updateMatchedPairs(); seq.sort(); mf.addTrack (seq);
    }
    auto dir = juce::File::getSpecialLocation (juce::File::userMusicDirectory).getChildFile ("Jackpot").getChildFile ("Guitar"); dir.createDirectory();
    const juce::String suffix = comp && layer >= 0 ? juce::String (" ") + ln[layer] : juce::String();
    auto f = dir.getChildFile (juce::File::createLegalFileName (clipName() + suffix + " " + juce::String ((int) (seed() % 1000)) + ".mid"));
    f.deleteFile(); juce::FileOutputStream os (f); if (os.openedOk()) { mf.writeTo (os); os.flush(); }
    return f;
}

void Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals nd;
    buffer.clear(); const int N = buffer.getNumSamples(); if (N <= 0) return;
    // suono
    const bool comp = vi ("mode") == COMPOSIZIONE;
    for (int y = 0; y < 3; ++y)
    {
        auto& sy = syn[(size_t) y];
        sy.body = vi ("body"); sy.tone = v ("tone"); sy.bodyAmt = v ("bodyAmt"); sy.drive = v ("drive"); sy.space = v ("space");
        sy.width = comp ? v ("width") * 0.4f : v ("width");
        sy.pan = comp ? (y == 0 ? -0.55f : y == 1 ? 0.55f : 0.0f) : 0.0f;   // doppia traccia
        sy.vol = v ("listen") > 0.5f ? juce::Decibels::decibelsToGain (v ("vol"), -36.0f) * (comp ? 0.8f : 1.0f) : 0.0f;
    }
    const bool midiOn = v ("midiOut") > 0.5f, live = v ("live") > 0.5f, cs = v ("chStr") > 0.5f;
    const auto tun = tuning (vi ("tuning")); const int pos = vi ("pos");
    acts.clear(); out.clear();

    // ingresso MIDI: la chitarra suona anche le note che arrivano (tastiera o il clip trascinato)
    for (const auto m : midi)
    {
        const auto& msg = m.getMessage(); const int at = juce::jlimit (0, N - 1, m.samplePosition), c = msg.getChannel();
        if (msg.isNoteOn())
        {
            const int s = ! comp && c >= 2 && c <= 7 ? c - 2 : syn[0].pickString (msg.getNoteNumber(), tun, pos);
            inStr[(size_t) msg.getNoteNumber()] = s; Hit h; h.str = s; h.note = msg.getNoteNumber(); h.vel = msg.getFloatVelocity();
            acts.push_back ({ at, 0, h, comp && c <= 3 ? c - 1 : 0, 0 });
        }
        else if (msg.isNoteOff()) { const int s = inStr[(size_t) msg.getNoteNumber()]; if (s >= 0) { Hit h; h.str = s; h.note = msg.getNoteNumber(); acts.push_back ({ at, 1, h, comp && c <= 3 ? c - 1 : 0, 0 }); inStr[(size_t) msg.getNoteNumber()] = -1; } }
        else if (msg.isPitchWheel()) { const float b = (msg.getPitchWheelValue() - 8192) / 8192.0f * 2.0f; Hit h; h.str = comp ? -1 : c >= 2 && c <= 7 ? c - 2 : -1; acts.push_back ({ at, 2, h, comp && c <= 3 ? c - 1 : 0, b }); }
        else if (msg.isAllNotesOff() || msg.isAllSoundOff()) { Hit h; h.str = -1; acts.push_back ({ at, 3, h, 0, 0 }); }
    }
    midi.clear();

    double p0 = 0, bpm = 120; bool playing = false;
    if (auto* ph = getPlayHead()) if (auto ps = ph->getPosition()) { playing = ps->getIsPlaying(); if (auto b = ps->getBpm()) bpm = *b; if (auto q = ps->getPpqPosition()) p0 = *q; }
    const bool preview = ! playing && v ("play") > 0.5f;
    if (preview) { p0 = ownPpq; ownPpq += N * bpm / 60.0 / sr; } else if (! playing) ownPpq = 0;
    uiBpm = bpm;
    if (std::abs (bpm - genBpm) > 1.0) { genBpm = bpm; triggerAsyncUpdate(); }   // il vibrato e' in Hz: si rifa' con il nuovo tempo
    const double spp = sr * 60.0 / bpm, p1 = p0 + N / spp;
    // posizioni al campione calcolate dal tempo assoluto: lo stesso evento cade sempre sullo stesso campione, qualunque sia il blocco
    const juce::int64 s0 = (juce::int64) std::llround (p0 * spp);
    auto smp = [&] (double t) { return (juce::int64) std::llround (t * spp); };
    auto inBlk = [&] (double t) { const auto a = smp (t); return a >= s0 && a < s0 + N; };
    auto atOf = [&] (double t) { return (int) juce::jlimit ((juce::int64) 0, (juce::int64) N - 1, smp (t) - s0); };
    auto flushAll = [&] (int at)
    {
        for (int i = 0; i < np; ++i) { out.addEvent (juce::MidiMessage::noteOff (pend[(size_t) i].ch, pend[(size_t) i].note), at); }
        np = 0;
        for (int c = 1; c <= 7; ++c) { out.addEvent (juce::MidiMessage::pitchWheel (c, 8192), at); out.addEvent (juce::MidiMessage::allNotesOff (c), at); }
        Hit h; h.str = -1; acts.push_back ({ at, 3, h, 0, 0 });
    };
    if (allOffReq.exchange (false)) flushAll (0);
    const bool run = (playing || preview) && live;
    if (! run)
    {
        if (lastEnd >= 0) flushAll (0);
        lastEnd = -1; uiPlaying = false; uiBeat = -1; uiSlot = -1;
    }
    else
    {
        if (lastEnd >= 0 && (p0 < lastEnd - 0.01 || p0 > lastEnd + 0.25)) flushAll (0);   // salto della testina
        lastEnd = p1;
        auto P = part(); const double L = P->len();
        uiPlaying = true; uiBeat = std::fmod (juce::jmax (0.0, p0), L);
        uiSlot = P->chordAt.empty() ? -1 : juce::jlimit (0, (int) P->chordAt.size() - 1, (int) std::floor (uiBeat.load() / (L / (double) P->chordAt.size())));
        // tutto si ricava dal tempo assoluto, senza stato fra un blocco e l'altro: note-off prima, poi bend, poi note-on (stesso campione = ordine giusto)
        const long k0 = (long) std::floor (p0 / L) - 1, k1 = (long) std::floor (p1 / L) + 1;
        auto vel7 = [] (float v) { return (juce::uint8) juce::jlimit (1, 127, juce::roundToInt (v * 127)); };
        for (int pass = 0; pass < 3; ++pass)
            for (long k = k0; k <= k1; ++k)
            {
                const double off = (double) k * L;
                if (pass == 0)
                    for (auto& e : P->ev)
                    {
                        const double t = off + e.beat + e.len; if (! inBlk (t)) continue;
                        const int at = atOf (t), c = comp ? 1 + e.layer : cs ? 2 + e.str : 1;
                        out.addEvent (juce::MidiMessage::noteOff (c, e.note), at);
                        Hit h; h.str = e.str; h.note = e.note; acts.push_back ({ at, 1, h, e.layer, 0 });
                        for (int i = 0; i < np; ++i) if (pend[(size_t) i].ch == c && pend[(size_t) i].note == e.note) { pend[(size_t) i] = pend[(size_t) --np]; break; }
                    }
                else if (pass == 1)
                    for (auto& b : P->bend)
                    {
                        const double t = off + b.beat; if (! inBlk (t)) continue;
                        const int at = atOf (t);
                        out.addEvent (juce::MidiMessage::pitchWheel (comp ? 3 : cs ? 2 + b.str : 1, juce::jlimit (0, 16383, 8192 + juce::roundToInt (b.semis / 2.0f * 8191.0f))), at);
                        Hit h; h.str = cs ? b.str : -1; acts.push_back ({ at, 2, h, comp ? 2 : 0, b.semis });
                    }
                else
                    for (auto& e : P->ev)
                    {
                        const double t = off + e.beat; if (! inBlk (t)) continue;
                        const int at = atOf (t), c = comp ? 1 + e.layer : cs ? 2 + e.str : 1;
                        out.addEvent (juce::MidiMessage::noteOn (c, e.note, vel7 (e.vel)), at);
                        Hit h; h.str = e.str; h.note = e.note; h.vel = e.vel; h.mute = e.mute; h.pm = e.pm; h.art = e.art; acts.push_back ({ at, 0, h, e.layer, 0 });
                        if (np < (int) pend.size()) pend[(size_t) np++] = { t + e.len, c, e.note, e.str };   // solo per chiudere tutto se il trasporto salta
                    }
            }
    }
    // la chitarra interna: a pezzi fra un evento e l'altro (tempo preciso al campione)
    std::stable_sort (acts.begin(), acts.end(), [] (const Act& a, const Act& b) { static const int pr[4] = { 3, 0, 2, 1 }; return a.at != b.at ? a.at < b.at : pr[a.type] < pr[b.type]; });   // prima i rilasci, poi i bend, poi le nuove note
    float* L = buffer.getWritePointer (0); float* R = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : L;
    int cur = 0;
    auto renderTo = [&] (int to) { while (cur < to) { const int n = juce::jmin (to - cur, 4096); for (int y = 0; y < (comp ? 3 : 1); ++y) syn[(size_t) y].render (L + cur, R + cur, n); cur += n; } };
    for (auto& a : acts)
    {
        renderTo (a.at);
        auto& sy = syn[(size_t) juce::jlimit (0, comp ? 2 : 0, a.ch)];
        switch (a.type)
        {
            case 0: sy.hit (a.h); break;
            case 1: sy.off (a.h.str, a.h.note); break;
            case 2: if (a.h.str >= 0) sy.bend (a.h.str, a.bend); else sy.bendAll (a.bend); break;
            default: for (auto& y : syn) { y.allOff(); y.bendAll (0); } break;
        }
    }
    renderTo (N);
    if (buffer.getNumChannels() == 1) {}
    if (midiOn) midi.swapWith (out);
}

void Processor::loadPreset (int i)
{
    const auto& P = presets(); i = juce::jlimit (0, (int) P.size() - 1, i);
    undo.beginNewTransaction();
    static const juce::StringArray keep { "vol", "listen", "live", "midiOut", "chStr", "play", "tuning" };
    for (auto* prm : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (prm))
            if (! keep.contains (rp->getParameterID())) rp->setValueNotifyingHost (rp->getDefaultValue());
    for (auto& kv : P[(size_t) i].p) if (auto* rp = apvts.getParameter (kv.first)) rp->setValueNotifyingHost (rp->convertTo0to1 (kv.second));
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
    if (t.hasProperty ("prog")) apvts.state.setProperty ("prog", t.getProperty ("prog"), &undo);
    if (t.hasProperty ("seed")) apvts.state.setProperty ("seed", t.getProperty ("seed"), &undo);
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
    undo.clearUndoHistory();
    regenerate();
}
juce::AudioProcessorEditor* Processor::createEditor() { return new Editor (*this); }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new gtr::Processor(); }
