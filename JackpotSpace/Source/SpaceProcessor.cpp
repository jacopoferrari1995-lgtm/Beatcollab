#include "SpaceProcessor.h"
#include "SpaceEditor.h"

namespace jsp
{
const std::vector<SpPreset>& factoryPresets()
{
    using namespace map;
    static const std::vector<SpPreset> P = [] {
        auto S = [] (int t, float mix, bool par, std::vector<float> p = {}) { SpSlot x; x.type = t; x.mix = mix; x.par = par; x.p = std::move (p); return x; };
        auto D = [] (int t, float s) { return nDecay (t, s); };
        const float PITCH12 = nPick (3, kPitches), PITCH7 = nPick (2, kPitches), PITCH19 = nPick (4, kPitches);
        auto IR = [] (int i) { return nPick (i, kIRChoices); };
        auto PAT = [] (int i) { return nPick (i, kPats); };
        auto OFFS = [] (float ms) { return juce::jlimit (0.0f, 1.0f, 0.5f + ms / 40.0f); };
        std::vector<SpPreset> v;
        auto add = [&v] (const char* cat, const char* name, const char* note, std::vector<SpSlot> sl, float duck = 0, float dry = 1, float wet = 0) { SpPreset x; x.cat = cat; x.name = name; x.note = note; x.s = std::move (sl); x.duck = duck; x.dry = dry; x.wet = wet; v.push_back (x); };

        add ("INIT", "Init", "Tutto a zero: una lastra e un'eco a nastro pronte, con i MIX chiusi. Alza il MIX dello slot che vuoi sentire.",
             { S (PLATE, 0.0f, true), S (TAPE, 0.0f, true) });
        add ("INIT", "Plate + Tape", "Una lastra e un'eco a nastro in parallelo: la partenza per voci e synth.",
             { S (PLATE, 0.30f, true), S (TAPE, 0.25f, true) });
        // ------------------------------------------------------------------ VOCE
        add ("VOCE", "Plate Classica", "La lastra da studio sulla voce: pre-delay che lascia passare le consonanti, bassi tolti, alti morbidi.",
             { S (PLATE, 0.24f, true, { D (PLATE, 1.6f), nPre (35), 0.55f, nBr (7000), 0.30f, nLo (220), nHi (9000), 0.67f }) }, 0.20f);
        add ("VOCE", "Plate + Eco 1/4", "Eco a un quarto che finisce dentro la lastra: le ripetizioni si allargano e restano dietro.",
             { S (DIGITAL, 0.18f, false, { nDiv ("1/4"), 0.28f, 1, 0.12f, nHi (6000), nLo (300), 0.0f, 0.5f }),
               S (PLATE, 0.22f, false, { D (PLATE, 1.8f), nPre (25), 0.60f, nBr (6500), 0.35f, nLo (250), nHi (8500), 0.70f }) }, 0.30f);
        add ("VOCE", "Rap Asciutta", "Stanza cortissima e uno slap a 85 ms: la voce resta davanti, ma non sembra registrata nell'armadio.",
             { S (ROOM, 0.12f, true, { D (ROOM, 0.35f), nPre (5), 0.35f, 0.50f, nBr (6000), 0.10f, nLo (250), nHi (9000) }),
               S (DIGITAL, 0.10f, true, { nMs (85), 0.05f, 0, 0.05f, nHi (4500), nLo (350), 0.0f, OFFS (3) }) });
        add ("VOCE", "Trap Throw", "Eco a nastro puntata e sala dietro, col ducking forte: secca mentre canti, esplode nelle pause.",
             { S (TAPE, 0.28f, true, { nDiv ("1/4D"), 0.42f, 1, 0.25f, nTT (4000), 0.35f, 0.0f, nLo (300) }),
               S (HALL, 0.16f, true, { D (HALL, 2.6f), nPre (40), 0.60f, 0.50f, 0.45f, 0.40f, 0.75f, nHi (8000) }) }, 0.60f);
        add ("VOCE", "Drill Dark", "Ping-pong a un ottavo e una lastra scura e lunga: cupo, largo, la voce resta in mezzo.",
             { S (DIGITAL, 0.20f, true, { nDiv ("1/8"), 0.35f, 1, 0.15f, nHi (4000), nLo (350), 0.90f, 0.5f }),
               S (PLATE, 0.18f, true, { D (PLATE, 2.2f), nPre (20), 0.65f, nBr (3500), 0.40f, nLo (300), nHi (6000), 0.80f }) }, 0.45f);
        add ("VOCE", "Ballad Hall 224", "La sala Lexicon delle ballate: pre-delay lungo, chorus nelle code, un'eco leggera che apre.",
             { S (HALL, 0.26f, true, { D (HALL, 3.4f), nPre (70), 0.75f, 0.45f, 0.40f, 0.45f, 0.80f, nHi (7500) }),
               S (DIGITAL, 0.08f, true, { nDiv ("1/4"), 0.20f, 1, 0.20f, nHi (5000), nLo (300), 0.5f, 0.5f }) }, 0.35f);
        add ("VOCE", "Pop Brillante", "Lastra corta e luminosa con un ping-pong puntato basso: aria e larghezza senza impastare.",
             { S (PLATE, 0.22f, true, { D (PLATE, 1.4f), nPre (50), 0.50f, nBr (11000), 0.25f, nLo (300), nHi (14000), 0.80f }),
               S (DIGITAL, 0.12f, true, { nDiv ("1/8D"), 0.25f, 1, 0.10f, nHi (7000), nLo (400), 1.0f, 0.5f }) }, 0.30f);
        add ("VOCE", "R&B Lusso", "Un velo di shimmer all'ottava, sala morbida ed eco ping-pong: ricco ma pulito.",
             { S (SHIMMER, 0.10f, true, { D (SHIMMER, 5.0f), nPre (60), 0.30f, PITCH12, 0.75f, nHi (7000), 0.50f, nLo (400) }),
               S (HALL, 0.16f, true, { D (HALL, 2.8f), nPre (45), 0.65f, 0.45f, 0.40f, 0.40f, 0.80f, nHi (8000) }),
               S (DIGITAL, 0.10f, true, { nDiv ("1/4"), 0.30f, 1, 0.20f, nHi (6000), nLo (350), 0.8f, 0.5f }) }, 0.40f);
        add ("VOCE", "Doppia Larga", "Effetto doppia: due ritardi cortissimi diversi a sinistra e a destra, modulati. Allarga senza coda.",
             { S (DIGITAL, 0.22f, true, { nMs (25), 0.0f, 0, 0.35f, nHi (9000), nLo (250), 0.0f, OFFS (12) }),
               S (ROOM, 0.06f, true, { D (ROOM, 0.3f), nPre (0), 0.30f, 0.60f, nBr (7000), 0.10f, nLo (300), nHi (10000) }) });
        add ("VOCE", "Lo-fi Telefono", "Delay a secchielli sporco e una molla: voce da radio vecchia, perfetta per intro e bridge.",
             { S (BBD, 0.30f, false, { nDiv ("1/8"), 0.30f, 1, 0.40f, nTT (2500), 0.50f, 0.0f, nLo (500) }),
               S (SPRING, 0.20f, true, { D (SPRING, 1.2f), 0.60f, 0.50f, 0.50f, 0.45f, nLo (400), 0.50f, 0.0f }) });
        add ("VOCE", "Coro Etereo", "Stile Spaced Out: eco a un quarto dentro un riverbero enorme che si scansa quando canti.",
             { S (SPACED, 0.30f, true, { D (SPACED, 4.0f), 0.45f, nDiv ("1/4"), 0.55f, 0.45f, 0.35f, nHi (8000), 0.55f }) });
        add ("VOCE", "Neoverb Pulita", "Lastra e sala miscelate con l'unmask alto: tanta coda, ma la voce non si impasta mai.",
             { S (NEOVERB, 0.25f, true, { D (NEOVERB, 1.8f), nPre (30), 0.60f, 0.25f, 0.40f, 0.70f, nLo (250), nHi (10000) }) }, 0.15f);
        add ("VOCE", "Reverse Intro", "La voce che risale al contrario verso la frase, con la sala dietro. Per le entrate.",
             { S (REVERSE, 0.35f, true, { nDiv ("1/2"), 0.25f, 1, nHi (8000), nLo (300), 0.40f }),
               S (HALL, 0.20f, true, { D (HALL, 3.0f), nPre (20), 0.70f, 0.50f, 0.45f, 0.40f, 0.80f, nHi (8000) }) });
        add ("VOCE", "Slapback 50s", "Slap a nastro a 110 ms e un filo di molla: rockabilly, soul, voci vintage.",
             { S (TAPE, 0.22f, true, { nMs (110), 0.12f, 0, 0.20f, nTT (5000), 0.40f, 0.0f, nLo (200) }),
               S (SPRING, 0.10f, true, { D (SPRING, 1.0f), 0.40f, 0.0f, 0.30f, 0.50f, nLo (300), 0.30f, 0.0f }) });
        add ("VOCE", "Adlib Spazio", "Ripetizioni puntate che si spargono a destra e sinistra e una lastra lunga: per le ad-lib.",
             { S (MULTITAP, 0.25f, true, { nDiv ("1/8"), 0.20f, 1, PAT (1), 0.90f, nHi (7000), nLo (400), 0.55f }),
               S (PLATE, 0.20f, true, { D (PLATE, 2.4f), nPre (30), 0.65f, nBr (6000), 0.40f, nLo (300), nHi (9000), 0.90f }) }, 0.50f);
        add ("VOCE", "Dancehall Dub", "Eco a nastro puntata con tanto ritorno e la molla: il dub sulle voci.",
             { S (TAPE, 0.32f, true, { nDiv ("1/8D"), 0.55f, 1, 0.35f, nTT (3500), 0.50f, 0.60f, nLo (300) }),
               S (SPRING, 0.22f, true, { D (SPRING, 1.8f), 0.55f, 0.50f, 0.60f, 0.45f, nLo (300), 0.60f, 0.0f }) }, 0.30f);
        add ("VOCE", "Parlato Naturale", "Una stanza di ripresa vera, appena: per parlato, podcast e voci che devono sembrare nella stanza.",
             { S (CONV, 0.12f, true, { IR (0), 0.50f, nPre (0), 0.30f, nLo (200), nHi (12000), 0.67f, 0.0f }) });
        add ("VOCE", "Chamber Anni 60", "La camera d'eco in muratura degli studi storici con uno slap: Motown, Abbey Road.",
             { S (CONV, 0.20f, true, { IR (1), 0.50f, nPre (20), 0.30f, nLo (250), nHi (9000), 0.70f, 0.0f }),
               S (TAPE, 0.10f, true, { nMs (120), 0.10f, 0, 0.15f, nTT (5000), 0.30f, 0.0f, nLo (250) }) });
        add ("VOCE", "Shimmer Ritornello", "Lastra piu' un shimmer di quinta lontano: il ritornello che si apre.",
             { S (PLATE, 0.20f, true, { D (PLATE, 2.0f), nPre (40), 0.60f, nBr (8000), 0.35f, nLo (250), nHi (10000), 0.80f }),
               S (SHIMMER, 0.12f, true, { D (SHIMMER, 6.0f), nPre (80), 0.45f, PITCH7, 0.80f, nHi (6000), 0.50f, nLo (500) }) }, 0.35f);
        add ("VOCE", "Morph Strofa > Rit", "Parte da stanza: automatizza MORPH verso destra nel ritornello e diventa una sala.",
             { S (MORPH, 0.20f, true, { D (MORPH, 2.0f), nPre (30), 0.25f, 0.55f, 0.55f, 0.30f, nLo (250), nHi (10000) }) }, 0.25f);
        add ("VOCE", "Chamber Soul", "Camera d'eco calda e piena, pre-delay corto: soul, gospel, voci che devono stare dentro la band.",
             { S (CHAMBER, 0.22f, true, { D (CHAMBER, 1.6f), nPre (25), 0.50f, 0.60f, 0.45f, 0.20f, nLo (300), nHi (7500) }) }, 0.20f);
        add ("VOCE", "Studio Su Misura", "Prime riflessioni vicine e coda separata: la voce sembra in una stanza vera ma con la coda di una sala.",
             { S (STUDIO, 0.20f, true, { D (STUDIO, 1.8f), nPre (10), 0.35f, 0.45f, 0.80f, 0.55f, 0.70f, nHi (10000) }) }, 0.20f);
        add ("VOCE", "Ad-lib Granulari", "Le ad-lib si sbriciolano in grani all'ottava sparsi nello stereo: effetto moderno senza coprire la voce.",
             { S (GRANULAR, 0.25f, true, { nDiv ("1/8"), 0.30f, 1, 0.35f, 0.55f, nPick (4, kGPitch), 0.40f, 0.20f }),
               S (PLATE, 0.15f, true, { D (PLATE, 1.8f), nPre (20), 0.60f, nBr (7000), 0.30f, nLo (300), nHi (9000), 0.80f }) }, 0.50f);
        // ------------------------------------------------------------------ BATTERIA
        add ("BATTERIA", "Rullante Plate", "Lastra corta e brillante, bassi tagliati: il rullante prende coda senza sporcare la cassa.",
             { S (PLATE, 0.20f, true, { D (PLATE, 1.1f), nPre (0), 0.45f, nBr (9000), 0.20f, nLo (350), nHi (12000), 0.90f }) });
        add ("BATTERIA", "Batteria in Stanza", "Stanza vera e un filo di lastra: la batteria suona insieme, nello stesso posto.",
             { S (ROOM, 0.22f, true, { D (ROOM, 0.6f), nPre (0), 0.50f, 0.70f, nBr (7000), 0.15f, nLo (200), nHi (10000) }),
               S (PLATE, 0.08f, true, { D (PLATE, 1.2f), nPre (10), 0.50f, nBr (8000), 0.20f, nLo (400), nHi (11000), 0.80f }) });
        add ("BATTERIA", "Kit Chamber", "Camera d'eco corta: rende vive le batterie campionate secche.",
             { S (CONV, 0.18f, true, { IR (1), 0.30f, nPre (5), 0.20f, nLo (180), nHi (11000), 0.80f, 0.0f }) });
        add ("BATTERIA", "Hi-hat Terzine", "Ripetizioni a terzine sparse nello stereo, solo alti: hi-hat che rotolano.",
             { S (MULTITAP, 0.15f, true, { nDiv ("1/16"), 0.10f, 1, PAT (2), 0.90f, nHi (12000), nLo (500), 0.50f }) });
        add ("BATTERIA", "Clap Riverso", "Il clap che arriva al contrario un quarto prima, con la lastra.",
             { S (REVERSE, 0.30f, true, { nDiv ("1/4"), 0.10f, 1, nHi (10000), nLo (400), 0.30f }),
               S (PLATE, 0.15f, true, { D (PLATE, 1.5f), nPre (0), 0.50f, nBr (9000), 0.25f, nLo (400), nHi (12000), 0.80f }) });
        add ("BATTERIA", "Rullante Gated 80", "Il rullante gated delle console SSL: grande, schiacciato, tagliato secco dopo 300 ms.",
             { S (GATED, 0.35f, true, { 0.55f, 0.55f, 0.70f, 0.20f, 0.60f, nPre (0), nLo (250), nHi (9000) }) });
        add ("BATTERIA", "Tom Gated", "Gate piu' lungo e meno crunch per i tom e i fill.",
             { S (GATED, 0.30f, true, { 0.70f, 0.50f, 0.80f, 0.35f, 0.35f, nPre (5), nLo (120), nHi (8000) }) });
        add ("BATTERIA", "Clap Chamber", "Camera d'eco corta e brillante sui clap e gli snap.",
             { S (CHAMBER, 0.25f, true, { D (CHAMBER, 0.9f), nPre (0), 0.40f, 0.70f, 0.65f, 0.10f, nLo (400), nHi (11000) }) });
        // ------------------------------------------------------------------ STRUMENTI
        add ("STRUMENTI", "Chitarra Spring", "Molla piena e un'eco a nastro: la chitarra da ampli.",
             { S (SPRING, 0.30f, true, { D (SPRING, 2.0f), 0.50f, 1.0f, 0.50f, 0.50f, nLo (150), 0.70f, 0.0f }),
               S (TAPE, 0.15f, true, { nDiv ("1/8D"), 0.30f, 1, 0.30f, nTT (4000), 0.30f, 0.0f, nLo (200) }) });
        add ("STRUMENTI", "Pad Cielo", "Shimmer all'ottava lungo e lo spazio di Spaced: i pad diventano nuvole.",
             { S (SHIMMER, 0.35f, true, { D (SHIMMER, 6.0f), nPre (40), 0.60f, PITCH12, 0.90f, nHi (8000), 0.60f, nLo (250) }),
               S (SPACED, 0.20f, true, { D (SPACED, 5.0f), 0.40f, nDiv ("1/4D"), 0.60f, 0.60f, 0.30f, nHi (9000), 0.0f }) });
        add ("STRUMENTI", "Piano Hall", "Sala da concerto morbida e una stanza per il corpo.",
             { S (HALL, 0.22f, true, { D (HALL, 2.2f), nPre (20), 0.65f, 0.55f, 0.50f, 0.30f, 0.80f, nHi (9000) }),
               S (ROOM, 0.10f, true, { D (ROOM, 0.5f), nPre (0), 0.50f, 0.60f, nBr (6000), 0.10f, nLo (150), nHi (9000) }) });
        add ("STRUMENTI", "Synth BBD", "Delay analogico puntato che gira e si scurisce, con la lastra larga.",
             { S (BBD, 0.30f, true, { nDiv ("1/8D"), 0.45f, 1, 0.60f, nTT (3500), 0.30f, 0.70f, nLo (250) }),
               S (PLATE, 0.15f, true, { D (PLATE, 2.0f), nPre (20), 0.60f, nBr (6000), 0.50f, nLo (250), nHi (9000), 1.0f }) });
        add ("STRUMENTI", "Archi Cattedrale", "Cattedrale vera: per archi, cori e campioni orchestrali.",
             { S (CONV, 0.30f, true, { IR (3), 0.50f, nPre (30), 0.30f, nLo (150), nHi (10000), 0.80f, 0.0f }) });
        add ("STRUMENTI", "Dub Chamber", "Nastro che gira dentro la molla: il classico del dub.",
             { S (TAPE, 0.40f, false), S (SPRING, 0.35f, false), S (HALL, 0.12f, true) });
        add ("STRUMENTI", "Lo-fi Spring", "Molla e nastro sporco in fila, per chitarre e campioni.",
             { S (SPRING, 0.35f, false), S (TAPE, 0.25f, false) });
        // ------------------------------------------------------------------ CREATIVI
        add ("CREATIVI", "Spaced Out Galassia", "Tutto aperto: eco puntato, coda di otto secondi, sparkle e modulazione.",
             { S (SPACED, 0.50f, true, { D (SPACED, 8.0f), 0.60f, nDiv ("1/4D"), 0.70f, 0.70f, 0.60f, nHi (10000), 0.30f }) });
        add ("CREATIVI", "Tunnel", "Il sottopassaggio con l'eco che sfarfalla.",
             { S (CONV, 0.35f, true, { IR (5), 0.50f, nPre (10), 0.30f, nLo (150), nHi (9000), 0.80f, 0.0f }) });
        add ("CREATIVI", "Cisterna Riversa", "Una cisterna di cemento suonata al contrario: il suono che risucchia.",
             { S (CONV, 0.40f, true, { IR (7), 0.60f, nPre (0), 0.20f, nLo (200), nHi (9000), 1.0f, 1.0f }) });
        add ("CREATIVI", "Freeze Drone", "Sala infinita e shimmer: premi FREEZE e la coda resta sospesa.",
             { S (HALL, 0.40f, true, { D (HALL, 20.0f), nPre (0), 1.0f, 0.60f, 0.60f, 0.60f, 0.90f, nHi (9000) }),
               S (SHIMMER, 0.25f, true, { D (SHIMMER, 12.0f), nPre (0), 0.50f, PITCH19, 1.0f, nHi (7000), 0.70f, nLo (300) }) });
        add ("CREATIVI", "Ottava Riversa", "Reverse a mezza battuta dentro uno shimmer: transizioni e drop.",
             { S (REVERSE, 0.40f, false, { nDiv ("1/2"), 0.35f, 1, nHi (9000), nLo (300), 0.60f }),
               S (SHIMMER, 0.30f, false, { D (SHIMMER, 5.0f), nPre (0), 0.50f, PITCH12, 0.80f, nHi (8000), 0.60f, nLo (300) }) });
        add ("CREATIVI", "Nuvola Granulare", "Il suono si sbriciola in grani che salgono e scendono di ottava dentro una sala lunga.",
             { S (CLOUD, 0.45f, true, { D (CLOUD, 5.0f), 0.55f, 0.60f, nPick (5, kGPitch), 0.70f, 0.70f, 0.50f, nHi (9000) }) });
        add ("CREATIVI", "Glitch Grani", "Delay granulare a un sedicesimo, grani corti, meta' al contrario: glitch su loop e campioni.",
             { S (GRANULAR, 0.40f, true, { nDiv ("1/16"), 0.45f, 1, 0.15f, 0.80f, nPick (2, kGPitch), 0.60f, 0.50f }) });
        add ("CREATIVI", "Il Tuo Spazio (IR)", "Convoluzione con il tuo file: trascina un .wav sulla scheda o usa CARICA IR nei parametri.",
             { S (CONV, 0.30f, true, { 1.0f, 0.50f, nPre (0), 0.0f, nLo (100), nHi (16000), 0.67f, 0.0f }) });
        // ------------------------------------------------------------------ MANDATA (DRY a 0, da mettere su una traccia return)
        add ("MANDATA", "Mandata | Plate", "Solo lastra, DRY a 0: da mettere su una traccia di ritorno.",
             { S (PLATE, 1.0f, true, { D (PLATE, 1.8f), nPre (30), 0.60f, nBr (7000), 0.30f, nLo (250), nHi (10000), 0.80f }) }, 0.0f, 0.0f);
        add ("MANDATA", "Mandata | Hall 224", "Solo sala, DRY a 0.",
             { S (HALL, 1.0f, true, { D (HALL, 2.8f), nPre (40), 0.70f, 0.50f, 0.45f, 0.40f, 0.80f, nHi (8500) }) }, 0.0f, 0.0f);
        add ("MANDATA", "Mandata | Delay 1/4", "Solo delay a un quarto filtrato, DRY a 0.",
             { S (DIGITAL, 1.0f, true, { nDiv ("1/4"), 0.35f, 1, 0.15f, nHi (6000), nLo (300), 0.30f, 0.5f }) }, 0.0f, 0.0f);
        add ("MANDATA", "Mandata | Eco in Plate", "Eco a nastro che entra nella lastra, DRY a 0.",
             { S (TAPE, 0.60f, false, { nDiv ("1/4"), 0.35f, 1, 0.25f, nTT (4500), 0.30f, 0.0f, nLo (300) }),
               S (PLATE, 0.70f, false, { D (PLATE, 2.0f), nPre (20), 0.60f, nBr (6500), 0.35f, nLo (250), nHi (9000), 0.80f }) }, 0.0f, 0.0f);
        return v;
    }();
    return P;
}

juce::AudioProcessorValueTreeState::ParameterLayout Processor::layout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    StringArray tn; for (int t = 0; t < kNumTypes; ++t) tn.add (info (t).name);
    const auto& d = factoryPresets()[0];
    for (int k = 0; k < kSlots; ++k)
    {
        const String s = "Slot " + String (k + 1);
        SpSlot sl = k < (int) d.s.size() ? d.s[(size_t) k] : SpSlot(); sl.mix = 0.0f;   // all'apertura nessuno slot suona
        l.add (std::make_unique<AudioParameterChoice> (ParameterID { typeId (k), 1 }, s + " Motore", tn, sl.type));
        l.add (std::make_unique<AudioParameterBool> (ParameterID { onId (k), 1 }, s + " On", true));
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { mixId (k), 1 }, s + " Mix", NormalisableRange<float> (0, 1), sl.mix));
        l.add (std::make_unique<AudioParameterBool> (ParameterID { rtId (k), 1 }, s + " Parallelo", sl.par));
        const auto& I = info (sl.type);
        for (int i = 0; i < kP; ++i) l.add (std::make_unique<AudioParameterFloat> (ParameterID { pId (k, i), 1 }, s + " P" + String (i + 1), NormalisableRange<float> (0, 1), i < I.n ? I.def[i] : 0.5f));
    }
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "dry", 1 }, "Dry", NormalisableRange<float> (0, 1), 1.0f));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "wet", 1 }, "Wet", NormalisableRange<float> (-24, 6, 0.1f), 0.0f));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "duck", 1 }, "Ducking", NormalisableRange<float> (0, 1), 0.0f));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "out", 1 }, "Output", NormalisableRange<float> (-12, 12, 0.1f), 0.0f));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "freeze", 1 }, "Freeze", false));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "bypass", 1 }, "Bypass", false));
    return l;
}

Processor::Processor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true).withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, &undo, "PARAMS", layout())
{
    for (int i = 0; i < kHist; ++i) { hIn[i] = 0; hOut[i] = 0; hPpq[i] = -1; hMask[i] = 0; }
    for (int c = 0; c < 2; ++c) pkOut[c] = 0;
    for (int k = 0; k < kSlots; ++k) slotLvl[k] = 0;
    presetName = factoryPresets()[0].name;
    startTimerHz (10);
}
bool Processor::readIR (int k)
{
    if (userIR[k].isEmpty()) return false;
    if (irLoaded[k] == userIR[k] && irData[k].getNumSamples() > 0) return true;
    juce::File f (userIR[k]); if (! f.existsAsFile()) return false;
    juce::AudioFormatManager fm; fm.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> r (fm.createReaderFor (f)); if (r == nullptr) return false;
    const int n = (int) std::min<juce::int64> (r->lengthInSamples, (juce::int64) (r->sampleRate * 12.0));
    irData[k].setSize (2, n); irData[k].clear(); r->read (&irData[k], 0, n, 0, true, true);
    if (r->numChannels == 1) irData[k].copyFrom (1, 0, irData[k], 0, 0, n);
    irRate[k] = r->sampleRate; irLoaded[k] = userIR[k];
    return true;
}
bool Processor::setUserIR (int k, const juce::File& f)
{
    const auto old = userIR[k]; userIR[k] = f.getFullPathName();
    if (! readIR (k)) { userIR[k] = old; return false; }
    if (slotType (k) != CONV) setSlot (k, CONV);
    setP (k, 0, 1.0f);   // SPAZIO = TUO FILE
    updateIRs(); return true;
}
void Processor::updateIRs()
{
    if (! prepared) return;
    for (int k = 0; k < kSlots; ++k)
    {
        if (slotType (k) != CONV) continue;
        float p[kP]; for (int i = 0; i < kP; ++i) p[i] = value (pId (k, i));
        const bool user = map::pick (p[0], kIRChoices) == kIRs;
        if (user && ! readIR (k)) continue;   // nessun file: resta l'ultimo ambiente caricato
        const int key = slots[k].irWanted (p, user ? userIR[k].hashCode() : 0);
        if (key == slots[k].irKey) continue;
        if (user) slots[k].loadUserIR (key, irData[k], irRate[k], p); else slots[k].loadIR (key, p);
    }
}
bool Processor::isBusesLayoutSupported (const BusesLayout& l) const
{
    return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo() && l.getMainInputChannelSet() == juce::AudioChannelSet::stereo();
}
void Processor::prepareToPlay (double s, int block)
{
    sr = s; const int n = juce::jmax (block, 4096);
    for (auto& sl : slots) sl.prepare (s, n);
    dry.setSize (2, n); chain.setSize (2, n); par.setSize (2, n); wet.setSize (2, n);
    for (int k = 0; k < kSlots; ++k) { mixS[k] = value (mixId (k)); runT[k] = OFF; fadeG[k] = 0; idle[k] = true; for (int i = 0; i < kP; ++i) ps[k][i] = value (pId (k, i)); }
    setLatencySamples (0);
    prepared = true; updateIRs();
}

void Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals nd; midi.clear();
    const int N = buffer.getNumSamples(); if (N == 0) return;
    for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); ++c) buffer.clear (c, 0, N);
    double bpm = 120, ppq = 0; bool play = false;
    if (auto* ph = getPlayHead()) if (auto pos = ph->getPosition())
    { if (auto b = pos->getBpm()) bpm = *b; if (auto q = pos->getPpqPosition()) ppq = *q; play = pos->getIsPlaying(); }
    uiBpm = bpm; uiPlay = play;
    float* L = buffer.getWritePointer (0); float* R = buffer.getWritePointer (juce::jmin (1, buffer.getNumChannels() - 1));
    // a pezzi grandi quanto i buffer preparati: niente memoria nuova mentre suona
    const int maxN = dry.getNumSamples(); const double qps = bpm / 60.0 / sr;
    for (int off = 0; off < N; off += maxN) { const int m = juce::jmin (maxN, N - off); processChunk (L + off, R + off, m, bpm, ppq + off * qps, play); }
}
void Processor::processChunk (float* L, float* R, int N, double bpm, double ppq, bool play)
{
    for (int c = 0; c < 2; ++c) { dry.copyFrom (c, 0, c ? R : L, N); chain.copyFrom (c, 0, c ? R : L, N); par.clear (c, 0, N); }
    const bool byp = value ("bypass") > 0.5f, frz = value ("freeze") > 0.5f;
    float* cL = chain.getWritePointer (0); float* cR = chain.getWritePointer (1);
    float* pL = par.getWritePointer (0); float* pR = par.getWritePointer (1);
    const float* dL = dry.getReadPointer (0); const float* dR = dry.getReadPointer (1);
    float* wL = wet.getWritePointer (0); float* wR = wet.getWritePointer (1);
    const float fadeStep = 1.0f / (0.015f * (float) sr);   // 15 ms per uscire o entrare quando cambi motore
    int mask = 0;
    for (int k = 0; k < kSlots; ++k)
    {
        const int want = value (onId (k)) > 0.5f && ! byp ? slotType (k) : (int) OFF;
        const int solo = soloSlot.load();
        const float mt = want == OFF || (solo >= 0 && solo != k) ? 0.0f : solo == k ? juce::jmax (0.5f, value (mixId (k))) : value (mixId (k));
        // slot fermo: mix chiuso e coda finita -> non calcola (e quando riparte, riparte pulito)
        if (mt < 1e-4f && mixS[k] < 1e-4f && (fadeG[k] <= 0.0f || runT[k] == OFF))
        {
            if (! idle[k]) { idle[k] = true; needClear[k] = true; }
            mixS[k] = 0; slotLvl[k] = slotLvl[k].load() * 0.8f;
            if (want != runT[k]) { runT[k] = want; fadeG[k] = 0; }
            continue;
        }
        if (needClear[k]) { slots[k].clearAll(); needClear[k] = false; for (int i = 0; i < kP; ++i) ps[k][i] = value (pId (k, i)); }
        idle[k] = false;
        if (runT[k] == OFF && want != OFF) { runT[k] = want; fadeG[k] = 0; for (int i = 0; i < kP; ++i) ps[k][i] = value (pId (k, i)); }
        const bool leaving = want != runT[k];   // motore cambiato: si sfuma quello vecchio, poi entra il nuovo
        if (runT[k] == OFF) { mixS[k] = 0; continue; }
        // valori: i continui scivolano (30 ms), quelli a scatti (molle, pattern, pitch, on/off) entrano dopo un attimo di silenzio dello slot
        const auto& I = info (runT[k]);
        float pt[kP]; for (int i = 0; i < kP; ++i) pt[i] = value (pId (k, i));
        bool stepChanged = false;
        if (! leaving) for (int i = 0; i < I.n; ++i) if (isStep (I.kind[i]) && std::abs (pt[i] - ps[k][i]) > 1e-6f) stepChanged = true;
        const bool dip = leaving || stepChanged;
        const bool isPar = value (rtId (k)) > 0.5f;
        const float* inL = isPar ? dL : cL; const float* inR = isPar ? dR : cR;
        float lv = 0;
        for (int o = 0, m = 0; o < N; o += m)
        {   // pezzi da 32 allineati a un contatore libero: lo scivolamento non dipende dal buffer dell'host
            m = juce::jmin (kSub - (int) ((subPh + (unsigned) o) & (unsigned) (kSub - 1)), N - o);
            const float a = 1.0f - std::exp (-(float) m / (0.03f * (float) sr)), aSlow = 1.0f - std::exp (-(float) m / (0.12f * (float) sr)), aSize = 1.0f - std::exp (-(float) m / (0.5f * (float) sr));
            if (! leaving) for (int i = 0; i < kP; ++i)
            {
                if (i < I.n && isStep (I.kind[i])) { if (! dip || fadeG[k] <= 0.0f) ps[k][i] = pt[i]; }
                else { const Kind kd = i < I.n ? I.kind[i] : KP; ps[k][i] += (pt[i] - ps[k][i]) * (kd == KSIZE ? aSize : kd == KPRE || kd == KERS || kd == KTIME ? aSlow : a); }   // SIZE ancora piu' lenta: cambia le lunghezze delle linee   // tempi e grandezze: piu' lenti, niente graffi
            }
            // in parallelo lo slot sente il suono pulito, in serie quello che esce dagli slot prima
            slots[k].process (inL + o, inR + o, wL + o, wR + o, m, runT[k], ps[k], bpm, frz);
            for (int i = o; i < o + m; ++i)
            {
                mixS[k] += (mt - mixS[k]) * 0.003f;
                fadeG[k] = dip ? juce::jmax (0.0f, fadeG[k] - fadeStep) : juce::jmin (1.0f, fadeG[k] + fadeStep);
                const float g = mixS[k] * fadeG[k] * fadeG[k];
                const float aa = wL[i] * g, bb = wR[i] * g;
                if (isPar) { pL[i] += aa; pR[i] += bb; } else { cL[i] += aa; cR[i] += bb; }
                lv = juce::jmax (lv, std::abs (aa) + std::abs (bb));
            }
        }
        if (leaving && fadeG[k] <= 0.0f) { runT[k] = want; fadeG[k] = 0; for (int i = 0; i < kP; ++i) ps[k][i] = value (pId (k, i)); if (want != OFF) slots[k].clearAll(); }
        slotLvl[k] = juce::jmax (lv * 0.5f, slotLvl[k].load() * 0.85f);
        if (lv > 1e-4f) mask |= 1 << k;
    }
    subPh += (unsigned) N;
    // uscita: pulito + (catena - pulito) + parallelo; il ducking abbassa il bagnato quando suona l'ingresso
    const float dt = byp ? 1.0f : soloSlot.load() >= 0 ? 0.0f : value ("dry"), wt = byp ? 0.0f : juce::Decibels::decibelsToGain (value ("wet")), og = byp ? 1.0f : juce::Decibels::decibelsToGain (value ("out"));
    const float duck = value ("duck"), att = 1.0f - std::exp (-1.0f / (0.005f * (float) sr)), rel = 1.0f - std::exp (-1.0f / (0.25f * (float) sr));
    float po[2] = { 0, 0 };
    const int colLen = juce::jmax (1, (int) (sr * kColSec));
    for (int i = 0; i < N; ++i)
    {
        dryS += (dt - dryS) * 0.002f; wetS += (wt - wetS) * 0.002f; outS += (og - outS) * 0.002f;
        const float lvl = 0.5f * (std::abs (dL[i]) + std::abs (dR[i]));
        env += (lvl > env ? att : rel) * (lvl - env);
        const float red = juce::jlimit (0.0f, 1.0f, juce::Decibels::gainToDecibels (env + 1e-6f, -80.0f) + 40.0f) / 40.0f;   // 0 sotto -40 dB, 1 a 0 dB
        const float dg = 1.0f - duck * juce::jlimit (0.0f, 1.0f, red * 1.6f) * 0.9f; duckS += (dg - duckS) * 0.01f;
        const float wl = (cL[i] - dL[i] + pL[i]) * wetS * duckS, wr = (cR[i] - dR[i] + pR[i]) * wetS * duckS;
        L[i] = (dL[i] * dryS + wl) * outS; R[i] = (dR[i] * dryS + wr) * outS;
        if (! std::isfinite (L[i]) || ! std::isfinite (R[i])) { L[i] = R[i] = 0; }
        po[0] = juce::jmax (po[0], std::abs (L[i])); po[1] = juce::jmax (po[1], std::abs (R[i]));
        colIn = juce::jmax (colIn, 0.5f * (std::abs (dL[i]) + std::abs (dR[i]))); colOut = juce::jmax (colOut, 0.5f * (std::abs (wl) + std::abs (wr))); colMask |= mask;
        if (++colN >= colLen)
        {   // una colonna dell'onda ogni 5 ms: entrata, solo bagnato, posizione nel pezzo, slot che suonano
            const long t = hTot.load(); const int w = (int) (t % kHist);
            hIn[w] = colIn; hOut[w] = colOut; hPpq[w] = play ? (float) (ppq + (double) i * bpm / 60.0 / sr) : -1.0f; hMask[w] = colMask;
            hTot = t + 1; colIn = colOut = 0; colN = 0; colMask = 0;
        }
    }
    for (int c = 0; c < 2; ++c) pkOut[c] = juce::jmax (po[c], pkOut[c].load() * 0.9f);
}

void Processor::setP (int k, int i, float v) { if (auto* p = P (pId (k, i))) p->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, v)); }
void Processor::resetP (int k) { const auto& I = info (slotType (k)); for (int i = 0; i < kP; ++i) setP (k, i, i < I.n ? I.def[i] : 0.5f); }
void Processor::setSlot (int k, int type)
{
    auto* t = P (typeId (k)); t->setValueNotifyingHost (t->convertTo0to1 ((float) type)); resetP (k);
}
void Processor::swapSlots (int a, int b)
{
    if (a == b) return;
    { const int s = soloSlot.load(); if (s == a) soloSlot = b; else if (s == b) soloSlot = a; }
    std::swap (userIR[a], userIR[b]); std::swap (irData[a], irData[b]); std::swap (irRate[a], irRate[b]); std::swap (irLoaded[a], irLoaded[b]);
    const int ta = slotType (a), tb = slotType (b);
    float xa[kP + 3], xb[kP + 3];
    for (int i = 0; i < kP; ++i) { xa[i] = value (pId (a, i)); xb[i] = value (pId (b, i)); }
    xa[kP] = value (mixId (a)); xb[kP] = value (mixId (b)); xa[kP + 1] = value (onId (a)); xb[kP + 1] = value (onId (b)); xa[kP + 2] = value (rtId (a)); xb[kP + 2] = value (rtId (b));
    setSlot (a, tb); setSlot (b, ta);
    for (int i = 0; i < kP; ++i) { setP (a, i, xb[i]); setP (b, i, xa[i]); }
    P (mixId (a))->setValueNotifyingHost (xb[kP]); P (mixId (b))->setValueNotifyingHost (xa[kP]);
    P (onId (a))->setValueNotifyingHost (xb[kP + 1]); P (onId (b))->setValueNotifyingHost (xa[kP + 1]);
    P (rtId (a))->setValueNotifyingHost (xb[kP + 2]); P (rtId (b))->setValueNotifyingHost (xa[kP + 2]);
}
void Processor::loadFactory (int i)
{
    soloSlot = -1; undo.beginNewTransaction();
    const auto& p = factoryPresets()[(size_t) juce::jlimit (0, (int) factoryPresets().size() - 1, i)];
    for (int k = 0; k < kSlots; ++k)
    {
        const SpSlot sl = k < (int) p.s.size() ? p.s[(size_t) k] : SpSlot();
        setSlot (k, sl.type); P (mixId (k))->setValueNotifyingHost (sl.mix);
        P (rtId (k))->setValueNotifyingHost (sl.par ? 1.0f : 0.0f); P (onId (k))->setValueNotifyingHost (1.0f);
        for (int j = 0; j < juce::jmin (kP, (int) sl.p.size()); ++j) if (sl.p[(size_t) j] >= 0) setP (k, j, sl.p[(size_t) j]);
    }
    auto setReal = [this] (const char* id, float v) { auto* q = P (id); q->setValueNotifyingHost (q->convertTo0to1 (v)); };
    setReal ("dry", p.dry); setReal ("wet", p.wet); setReal ("duck", p.duck); setReal ("out", 0.0f); P ("freeze")->setValueNotifyingHost (0.0f);
    presetName = p.name;
}
juce::File Processor::presetFolder()
{
    auto f = juce::File::getSpecialLocation (juce::File::userMusicDirectory).getChildFile ("Jackpot").getChildFile ("Space Preset");
    f.createDirectory(); return f;
}
bool Processor::savePreset (const juce::File& f)
{
    juce::XmlElement x ("JACKPOTSPACE");
    for (int k = 0; k < kSlots; ++k)
    {
        auto* s = x.createNewChildElement ("SLOT");
        s->setAttribute ("type", juce::String (info (slotType (k)).name)); s->setAttribute ("on", value (onId (k)) > 0.5f ? 1 : 0);
        s->setAttribute ("mix", (double) value (mixId (k))); s->setAttribute ("par", value (rtId (k)) > 0.5f ? 1 : 0);
        juce::StringArray av; for (int i = 0; i < kP; ++i) av.add (juce::String (value (pId (k, i)), 4)); s->setAttribute ("p", av.joinIntoString (","));
        if (userIR[k].isNotEmpty()) s->setAttribute ("ir", userIR[k]);
    }
    for (auto id : { "dry", "wet", "duck", "out" }) x.setAttribute (id, (double) value (id));
    const bool ok = x.writeTo (f); if (ok) presetName = f.getFileNameWithoutExtension(); return ok;
}
bool Processor::loadPreset (const juce::File& f)
{
    auto x = juce::XmlDocument::parse (f); if (! x || ! x->hasTagName ("JACKPOTSPACE")) return false;
    int k = 0;
    for (auto* s : x->getChildIterator())
    {
        if (k >= kSlots) break;
        int t = OFF; for (int i = 0; i < kNumTypes; ++i) if (s->getStringAttribute ("type") == info (i).name) t = i;
        setSlot (k, t); P (onId (k))->setValueNotifyingHost (s->getIntAttribute ("on", 1) ? 1.0f : 0.0f);
        P (mixId (k))->setValueNotifyingHost ((float) s->getDoubleAttribute ("mix", 0.3)); P (rtId (k))->setValueNotifyingHost (s->getIntAttribute ("par", 1) ? 1.0f : 0.0f);
        const auto av = juce::StringArray::fromTokens (s->getStringAttribute ("p"), ",", "");
        for (int i = 0; i < juce::jmin (kP, av.size()); ++i) setP (k, i, av[i].getFloatValue());
        userIR[k] = s->getStringAttribute ("ir");
        ++k;
    }
    for (auto id : { "dry", "wet", "duck", "out" }) if (x->hasAttribute (id)) { auto* p = P (id); p->setValueNotifyingHost (p->convertTo0to1 ((float) x->getDoubleAttribute (id))); }
    presetName = f.getFileNameWithoutExtension(); return true;
}
void Processor::getStateInformation (juce::MemoryBlock& d)
{
    auto s = apvts.copyState(); s.setProperty ("preset", presetName, nullptr); s.setProperty ("theme", uiTheme.load(), nullptr);
    for (int k = 0; k < kSlots; ++k) s.setProperty ("ir" + juce::String (k + 1), userIR[k], nullptr);
    if (auto x = s.createXml()) copyXmlToBinary (*x, d);
}
void Processor::setStateInformation (const void* data, int size)
{
    if (auto x = getXmlFromBinary (data, size))
    {
        auto t = juce::ValueTree::fromXml (*x); if (! t.isValid()) return;
        presetName = t.getProperty ("preset", presetName).toString(); uiTheme = (int) t.getProperty ("theme", 1);
        for (int k = 0; k < kSlots; ++k) userIR[k] = t.getProperty ("ir" + juce::String (k + 1), "").toString();
        apvts.replaceState (t); undo.clearUndoHistory(); soloSlot = -1;
    }
}
juce::AudioProcessorEditor* Processor::createEditor() { return new Editor (*this); }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new jsp::Processor(); }
