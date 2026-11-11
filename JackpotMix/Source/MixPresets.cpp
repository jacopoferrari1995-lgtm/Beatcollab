#include "MixProcessor.h"

namespace jmx
{
namespace
{
// costruttore di preset: moduli per nome e comandi per etichetta (valori reali; "#2" = seconda etichetta uguale)
struct PB
{
    MixPreset p; int n = 0, band = 0;
    PB (const char* cat, const char* name, const char* note, int target) { p.cat = cat; p.name = name; p.note = note; p.target = target; }
    PB& m (const char* modName, std::initializer_list<std::pair<const char*, float>> vals = {}, float mix = -1)
    {
        const int t = findMod (modName); const auto& d = mod (t); const int s = n++;
        auto set = [this] (const juce::String& id, float v) { p.v.push_back ({ id, v }); };
        set (Processor::sid (s, "type"), (float) t); set (Processor::sid (s, "mix"), mix >= 0 ? mix : d.defMix);
        float v[kP]; for (int i = 0; i < kP; ++i) v[i] = hidden (d.spec[(size_t) i]) ? 0.0f : specNorm (d.spec[(size_t) i], d.spec[(size_t) i].def);
        for (auto& [lab, x] : vals)
        {
            juce::String L (lab); int want = 1; if (L.contains ("#")) { want = L.fromLastOccurrenceOf ("#", false, false).getIntValue(); L = L.upToLastOccurrenceOf ("#", false, false); }
            int idx = -1, seen = 0;
            for (int i = 0; i < kP; ++i) if (! hidden (d.spec[(size_t) i]) && L == d.spec[(size_t) i].label && ++seen == want) { idx = i; break; }
            jassert (idx >= 0); if (idx < 0) continue;
            const auto& sp = d.spec[(size_t) idx];
            if (sp.fmt == F_AEF && x > 1.0f)
            {   // frequenza in Hz: scatto piu vicino del selettore
                const auto& B = jb::aeInfo (jpre::aeOf (jpre::eqOf (d.sub))).b[sp.aeBand]; int best = 0; float bd = 1e9f;
                for (int k = 0; k < (int) B.freqs.size(); ++k) { const float dd = std::abs (std::log (juce::jmax (1.0f, B.freqs[(size_t) k]) / x)); if (dd < bd) { bd = dd; best = k; } }
                v[idx] = B.freqs.size() > 1 ? best / (float) (B.freqs.size() - 1) : 0.0f;
            }
            else if (sp.fmt == F_CUSTOM) v[idx] = x;   // gia 0..1
            else v[idx] = specNorm (sp, x);
        }
        for (int i = 0; i < kP; ++i) set (Processor::sid (s, ("p" + juce::String (i + 1)).toRawUTF8()), v[i]);
        return *this;
    }
    // banda dell'EQ centrale
    PB& eq (int type, float f, float g, float q = 1, int ch = 0, int slope = 1)
    {
        const int b = band++; auto set = [this, b] (const char* w, float v) { p.v.push_back ({ Processor::bid (b, w), v }); };
        set ("on", 1); set ("type", (float) type); set ("f", f); set ("g", g); set ("q", q); set ("ch", (float) ch); set ("slope", (float) slope); return *this;
    }
    PB& dyn (float thr, float range) { const int b = band - 1; p.v.push_back ({ Processor::bid (b, "dyn"), 1 }); p.v.push_back ({ Processor::bid (b, "thr"), thr }); p.v.push_back ({ Processor::bid (b, "range"), range }); return *this; }
    PB& spec (float thr, float range) { const int b = band - 1; p.v.push_back ({ Processor::bid (b, "spec"), 1 }); p.v.push_back ({ Processor::bid (b, "thr"), thr }); p.v.push_back ({ Processor::bid (b, "range"), range }); return *this; }
    PB& eqAt (int pos) { p.v.push_back ({ "eq_pos", (float) pos }); return *this; }
    MixPreset done() { p.v.push_back ({ "n", (float) n }); return p; }
};
enum { T_VOX, T_BGV, T_KICK, T_808, T_SNR, T_HAT, T_DRUMS, T_GTR, T_KEYS, T_SYN, T_SMP, T_MIX, T_MST, T_LOUD };
}

const std::vector<MixPreset>& factoryPresets()
{
    static const std::vector<MixPreset> P = []
    {
        std::vector<MixPreset> v;
        v.push_back (PB ("INIT", "Rack vuoto", "Nessun modulo, EQ piatto: il suono passa intatto.", T_VOX).done());
        // ================================================================ VOCE (lead)
        v.push_back (PB ("VOCE", "Rap Trap | Davanti e lucida", "Pulizia, 1176 veloce, LA-2A che leviga, de-esser, aria. La voce trap moderna sopra l'808.", T_VOX)
            .m ("UTILITY", { { "LOW CUT", 90 } }).eq (BT_BELL, 300, -3, 1.2f).dyn (-30, -4).eq (BT_BELL, 3200, 2, 0.9f).eq (BT_HSHELF, 11000, 3, 0.7f)
            .m ("FET 76", { { "INPUT", 0.42f }, { "RATIO", 1 }, { "ATTACK", 0.3f }, { "RELEASE", 0.55f } })
            .m ("OPTO 2A", { { "PEAK RED.", 0.35f } })
            .m ("PRO DS", { { "RANGE", 7 }, { "DA", 6000 } })
            .m ("CLASSIC CLIP", { { "MODO", 1 }, { "DRIVE", 3 }, { "CEILING", -3 } })
            .m ("PLATE 140", { { "DECAY", 1200 }, { "PRE-DELAY", 0.12f } }, 0.08f).eqAt (1).done());
        v.push_back (PB ("VOCE", "Rap Boom Bap | Calda anni 90", "73 spinto, 1176, Pultec per la presenza, room corta: la voce dei dischi 90.", T_VOX)
            .m ("73 PRE", { { "GAIN", 14 }, { "LOW CUT", 2 } }).eq (BT_BELL, 450, -2.5f, 1.0f)
            .m ("FET 76", { { "INPUT", 0.5f }, { "RATIO", 0 }, { "ATTACK", 0.45f } })
            .m ("EQP-1A", { { "HIGH FREQ", 3 }, { "BOOST#2", 3 }, { "BANDWIDTH", 5 } })
            .m ("DS 902", { { "FREQ", 6500 }, { "THRESH", -26 } })
            .m ("ROOM", { { "DECAY", 600 } }, 0.1f).eqAt (1).done());
        v.push_back (PB ("VOCE", "Drill | Dura e aggressiva", "SSL pre, 1176 in ALL, clip duro, telefono leggero sui bassi: voce drill che taglia.", T_VOX)
            .m ("SSL E PRE", { { "GAIN", 10 }, { "LOW CUT", 3 } }).eq (BT_BELL, 250, -3, 1.2f).eq (BT_BELL, 2500, 3, 1.1f).dyn (-24, -3).eq (BT_HSHELF, 9000, 2, 0.7f)
            .m ("FET 76", { { "INPUT", 0.55f }, { "RATIO", 4 }, { "ATTACK", 0.25f }, { "RELEASE", 0.45f } })
            .m ("PRO DS", { { "RANGE", 8 } })
            .m ("CLASSIC CLIP", { { "MODO", 2 }, { "DRIVE", 4 }, { "CEILING", -2 } })
            .m ("SLAPBACK", { { "TIME", 110 } }, 0.08f).eqAt (1).done());
        v.push_back (PB ("VOCE", "R&B | Seta e aria", "Avalon pulito, LA-2A, MAAG con l'aria, Lush Plate e delay a 1/4: voce R&B morbida e aperta.", T_VOX)
            .m ("737 PRE", { { "GAIN", 8 }, { "LOW CUT", 2 } }).eq (BT_BELL, 220, -2, 1.0f).eq (BT_BELL, 4200, -2, 4).dyn (-34, -4)
            .m ("OPTO 2A", { { "PEAK RED.", 0.45f } })
            .m ("MAAG EQ4", { { "AIR", 3.5f }, { "FREQ#3", 20000 } })
            .m ("PRO DS", { { "RANGE", 6 } })
            .m ("LUSH PLATE", { { "DECAY", 2400 } }, 0.16f)
            .m ("DIGITAL 2290", { { "TEMPO", 8 }, { "FEEDBACK", 0.25f }, { "HIGH CUT", 6000 } }, 0.1f).eqAt (1).done());
        v.push_back (PB ("VOCE", "Pop | Brillante e compatta", "SSL, compressione in serie, de-esser, aria e plate corta: voce pop da radio.", T_VOX)
            .m ("SSL E PRE", { { "GAIN", 6 }, { "LOW CUT", 2 } }).eq (BT_LCUT, 100, 0, 1, 0, 2).eq (BT_BELL, 350, -2, 1.1f).eq (BT_BELL, 3000, 2, 1.0f).eq (BT_HSHELF, 12000, 3.5f, 0.7f)
            .m ("FET 76", { { "INPUT", 0.38f }, { "RATIO", 0 }, { "ATTACK", 0.4f } })
            .m ("OPTO 3A", { { "PEAK RED.", 0.35f } })
            .m ("PRO DS", { { "RANGE", 8 } })
            .m ("PLATE 140", { { "DECAY", 1600 } }, 0.12f).eqAt (1).done());
        v.push_back (PB ("VOCE", "Melodic Trap | Ipnotica", "Voce cantata trap: Trivox leggero, plate lunga e delay ping pong a 1/8 puntato.", T_VOX)
            .m ("737 PRE", { { "GAIN", 6 }, { "LOW CUT", 2 } }).eq (BT_BELL, 280, -3, 1).eq (BT_HSHELF, 10000, 3, 0.7f)
            .m ("OPTO 2A", { { "PEAK RED.", 0.45f } })
            .m ("PRO DS", { { "RANGE", 7 } })
            .m ("TRIVOX", {}, 0.25f)
            .m ("PING PONG", { { "TEMPO", 6 }, { "FEEDBACK", 0.35f }, { "HIGH CUT", 5000 }, { "DUCK", 0.6f } }, 0.18f)
            .m ("LUSH PLATE", { { "DECAY", 3200 } }, 0.15f).eqAt (1).done());
        v.push_back (PB ("VOCE", "Lo-fi | Dietro il vinile", "Banda stretta, nastro a cassetta, V76 e spring: voce lo-fi calda e lontana.", T_VOX)
            .m ("V76 PRE", { { "GAIN", 14 } }).eq (BT_LCUT, 180, 0, 1, 0, 2).eq (BT_HCUT, 7000, 0, 1, 0, 1).eq (BT_BELL, 1200, 2, 0.8f)
            .m ("OPTO 2A", { { "PEAK RED.", 0.5f } })
            .m ("TAPE MACHINE", { { "MACCHINA", 2 }, { "DRIVE", 0.5f }, { "FRUSCIO", 0.25f } })
            .m ("SPRING", { { "DECAY", 1800 } }, 0.15f).eqAt (1).done());
        v.push_back (PB ("VOCE", "Rock | Spinta e sporca", "Helios, Distressor, Decapitator sul medio: voce rock che graffia.", T_VOX)
            .m ("HELIOS PRE", { { "GAIN", 16 }, { "LOW CUT", 2 } }).eq (BT_BELL, 2200, 3, 0.9f).eq (BT_BELL, 400, -2, 1)
            .m ("DISTRESS", { { "INPUT", 0.45f }, { "RATIO", 3 } })
            .m ("DECAP", { { "DRIVE", 0.35f }, { "STILE", 1 } }, 0.5f)
            .m ("PRO DS", { { "RANGE", 6 } })
            .m ("SLAPBACK", { { "TIME", 120 } }, 0.12f)
            .m ("ROOM", { { "DECAY", 800 } }, 0.1f).eqAt (1).done());
        v.push_back (PB ("VOCE", "Rap Aggressivo | Urlo", "Per voci urlate e spinte: limiter morbido sulle punte, distorsione in parallelo, presenza.", T_VOX)
            .m ("API 312 PRE", { { "GAIN", 16 }, { "LOW CUT", 3 } }).eq (BT_BELL, 3500, 3, 1).dyn (-20, -5)
            .m ("FET 76", { { "INPUT", 0.6f }, { "RATIO", 4 } })
            .m ("RAT", { { "DRIVE", 0.5f }, { "TONE", -0.3f } }, 0.25f)
            .m ("PRO DS", { { "RANGE", 9 } })
            .m ("CLASSIC CLIP", { { "MODO", 3 }, { "DRIVE", 5 }, { "CEILING", -2 } }).eqAt (1).done());
        v.push_back (PB ("VOCE", "Intima | Sussurro vicino", "Voce piano e vicina: Neve 1081 pulito, compressione dolce, poco riverbero, sibilanti morbide.", T_VOX)
            .m ("81 PRE", { { "GAIN", 8 }, { "LOW CUT", 1 } }).eq (BT_BELL, 180, 1.5f, 0.9f).eq (BT_BELL, 5000, -2, 3).dyn (-36, -4)
            .m ("OPTO 2A", { { "PEAK RED.", 0.3f } })
            .m ("PRO DS", { { "RANGE", 5 }, { "DA", 5000 } })
            .m ("CHAMBER", { { "DECAY", 1200 } }, 0.08f).eqAt (1).done());
        v.push_back (PB ("VOCE", "Ritornello | Largo e grosso", "Hook: Doubler, wider sugli alti, plate e delay a 1/4. Il ritornello che si apre.", T_VOX)
            .m ("SSL E PRE", { { "GAIN", 6 }, { "LOW CUT", 2 } }).eq (BT_BELL, 300, -2, 1).eq (BT_HSHELF, 10000, 3, 0.7f)
            .m ("FET 76", { { "INPUT", 0.4f } })
            .m ("PRO DS", { { "RANGE", 7 } })
            .m ("DOUBLER", {}, 0.35f)
            .m ("WIDER", { { "WIDTH", 1.35f }, { "DA", 600 } })
            .m ("LUSH PLATE", { { "DECAY", 2400 } }, 0.16f)
            .m ("DIGITAL 2290", { { "TEMPO", 8 }, { "FEEDBACK", 0.3f }, { "DUCK", 0.5f } }, 0.12f).eqAt (1).done());
        v.push_back (PB ("VOCE", "Spoken | Podcast e parlato", "Parlato chiaro: gate leggero, LA-3A, de-esser, presenza, niente spazio.", T_VOX)
            .m ("UTILITY", { { "LOW CUT", 80 } })
            .m ("GATE", { { "THRESH", -50 }, { "RANGE", 10 }, { "RELEASE", 200 } })
            .eq (BT_BELL, 250, -2, 1).eq (BT_BELL, 3000, 2, 1).eq (BT_HCUT, 15000, 0, 1, 0, 1)
            .m ("OPTO 3A", { { "PEAK RED.", 0.45f } })
            .m ("PRO DS", { { "RANGE", 6 } })
            .m ("LIMITER", { { "GAIN", 3 }, { "CEILING", -2 } }).eqAt (2).done());
        v.push_back (PB ("VOCE", "Gospel Soul | Piena", "UA 610, Pultec, Chamber: voce soul calda con la stanza.", T_VOX)
            .m ("610 PRE", { { "GAIN", 12 }, { "LOW CUT", 1 } })
            .m ("EQP-1A", { { "LOW FREQ", 1 }, { "BOOST", 2 }, { "ATTEN", 1.5f }, { "HIGH FREQ", 3 }, { "BOOST#2", 3 } })
            .m ("OPTO 2A", { { "PEAK RED.", 0.45f } })
            .m ("DS 902", {})
            .m ("CHAMBER", { { "DECAY", 2000 } }, 0.15f).done());
        v.push_back (PB ("VOCE", "Femminile Pop | Ariosa", "Voce femminile alta: niente fango, sibilanti tenute, aria a 16k, plate brillante.", T_VOX)
            .m ("737 PRE", { { "GAIN", 6 }, { "LOW CUT", 3 } }).eq (BT_BELL, 400, -2.5f, 1).eq (BT_BELL, 2800, -2, 3).dyn (-30, -4).eq (BT_HSHELF, 14000, 4, 0.7f)
            .m ("OPTO 2A", { { "PEAK RED.", 0.4f } })
            .m ("PRO DS", { { "RANGE", 8 }, { "DA", 6500 } })
            .m ("PLATE 140", { { "DECAY", 2000 } }, 0.12f).eqAt (1).done());
        v.push_back (PB ("VOCE", "Maschile Profonda | Petto", "Voce bassa: Neve 1073 per il petto, taglio del rimbombo, LA-2A.", T_VOX)
            .m ("73 PRE", { { "GAIN", 12 }, { "LOW CUT", 1 } }).eq (BT_BELL, 150, -2.5f, 1.4f).dyn (-28, -4).eq (BT_BELL, 1800, 2, 1).eq (BT_HSHELF, 9000, 2, 0.7f)
            .m ("OPTO 2A", { { "PEAK RED.", 0.45f } })
            .m ("DS 902", { { "FREQ", 5500 } })
            .m ("ROOM", { { "DECAY", 700 } }, 0.08f).eqAt (1).done());
        v.push_back (PB ("VOCE", "Autotune | Pulita per il pitch", "Prima del pitch: niente riverbero, compressione regolare, sibilanti basse e pulizia stretta.", T_VOX)
            .m ("UTILITY", { { "LOW CUT", 100 } }).eq (BT_BELL, 320, -3, 1.2f).eq (BT_BELL, 4000, -3, 5).spec (-38, -5)
            .m ("OPTO 3A", { { "PEAK RED.", 0.4f } })
            .m ("PRO DS", { { "RANGE", 8 } }).eqAt (1).done());
        v.push_back (PB ("VOCE", "Telefono | Effetto radio", "Voce dentro una radiolina: banda stretta, saturazione e un filo di spring.", T_VOX)
            .m ("RADIO", { { "DRIVE", 0.5f } })
            .m ("FET 76", { { "INPUT", 0.5f }, { "RATIO", 2 } })
            .m ("SPRING", { { "DECAY", 900 } }, 0.1f).done());
        // ================================================================ VOCI DI SUPPORTO
        v.push_back (PB ("SUPPORTO", "Doppie | Dietro la lead", "Doppie della strofa: piu scure e strette, compresse forte, si sentono senza coprire.", T_BGV)
            .m ("UTILITY", { { "LOW CUT", 180 } }).eq (BT_HSHELF, 8000, -3, 0.7f).eq (BT_BELL, 3000, -2, 1)
            .m ("OPTO 3A", { { "PEAK RED.", 0.6f } })
            .m ("PRO DS", { { "RANGE", 10 } })
            .m ("WIDER", { { "WIDTH", 1.4f }, { "DA", 400 } }).eqAt (1).done());
        v.push_back (PB ("SUPPORTO", "Ad-lib | Lati e lontane", "Ad-lib: telefono leggero, lati, delay e plate. Fuori dal centro della lead.", T_BGV)
            .m ("UTILITY", { { "LOW CUT", 250 }, { "HIGH CUT", 9000 } })
            .m ("FET 76", { { "INPUT", 0.55f }, { "RATIO", 2 } })
            .m ("DECAP", { { "DRIVE", 0.3f }, { "STILE", 2 } }, 0.5f)
            .m ("PING PONG", { { "TEMPO", 5 }, { "FEEDBACK", 0.3f } }, 0.2f)
            .m ("PLATE 140", { { "DECAY", 1500 } }, 0.15f).done());
        v.push_back (PB ("SUPPORTO", "Cori | Muro largo", "Cori e armonie: ensemble, wider, chamber. Un muro dietro la lead.", T_BGV)
            .m ("UTILITY", { { "LOW CUT", 200 } }).eq (BT_BELL, 2500, -2, 1)
            .m ("OPTO 2A", { { "PEAK RED.", 0.55f } })
            .m ("PRO DS", { { "RANGE", 9 } })
            .m ("ENSEMBLE", {}, 0.3f)
            .m ("WIDER", { { "WIDTH", 1.5f }, { "DA", 300 } })
            .m ("CHAMBER", { { "DECAY", 2200 } }, 0.2f).eqAt (1).done());
        v.push_back (PB ("SUPPORTO", "Armonie | Omni pulite", "Armonie gia registrate: compatte, sibilanti molto basse, plate lunga.", T_BGV)
            .m ("UTILITY", { { "LOW CUT", 160 } })
            .m ("OPTO 3A", { { "PEAK RED.", 0.5f } })
            .m ("PRO DS", { { "RANGE", 12 } })
            .m ("LUSH PLATE", { { "DECAY", 3000 } }, 0.22f).done());
        // ================================================================ KICK
        v.push_back (PB ("KICK", "Trap | Corta e dura", "Cassa trap: niente sub (lo fa l'808), click a 4k, clip morbido.", T_KICK)
            .eq (BT_LCUT, 35, 0, 1, 0, 2).eq (BT_BELL, 60, 2, 1.2f).eq (BT_BELL, 350, -4, 1.3f).eq (BT_BELL, 4000, 3, 1.2f)
            .m ("TRANSIENT", { { "ATTACK", 30 }, { "SUSTAIN", -20 } })
            .m ("CLASSIC CLIP", { { "MODO", 1 }, { "DRIVE", 4 } }).done());
        v.push_back (PB ("KICK", "Boom Bap | Polverosa", "Cassa 90: API, Pultec a 60, nastro e un filo di room.", T_KICK)
            .m ("API 312 PRE", { { "GAIN", 12 } })
            .m ("EQP-1A", { { "LOW FREQ", 2 }, { "BOOST", 4 }, { "ATTEN", 2.5f }, { "HIGH FREQ", 1 }, { "BOOST#2", 3 } })
            .m ("RMS 160", { { "THRESH", 0.4f }, { "RATIO", 2 } })
            .m ("TAPE MACHINE", { { "DRIVE", 0.5f } }).done());
        v.push_back (PB ("KICK", "Drill | Sub e botta", "Cassa drill: botta a 70 Hz, transiente in avanti, saturazione A.", T_KICK)
            .eq (BT_BELL, 70, 3, 1.4f).eq (BT_BELL, 300, -5, 1.2f).eq (BT_BELL, 3000, 2, 1)
            .m ("TRANSIENT", { { "ATTACK", 45 } })
            .m ("DECAP", { { "DRIVE", 0.35f }, { "STILE", 0 } })
            .m ("CLASSIC CLIP", { { "MODO", 2 }, { "DRIVE", 3 } }).done());
        v.push_back (PB ("KICK", "Rock | Battente", "Cassa acustica: gate, taglio del cartone, beater a 3k, compressore FET.", T_KICK)
            .m ("GATE", { { "THRESH", -30 }, { "RANGE", 30 }, { "RELEASE", 150 }, { "FILTRO ALTI", 2000 } })
            .eq (BT_BELL, 400, -6, 1.5f).eq (BT_BELL, 3000, 4, 1.2f).eq (BT_BELL, 60, 3, 1)
            .m ("FET 76", { { "INPUT", 0.45f }, { "ATTACK", 0.6f } }).eqAt (1).done());
        v.push_back (PB ("KICK", "House | Rotonda", "Cassa dance: corpo a 100 Hz, coda controllata, clip trasparente.", T_KICK)
            .eq (BT_LCUT, 30, 0, 1, 0, 2).eq (BT_BELL, 100, 2, 1).eq (BT_BELL, 250, -3, 1.5f)
            .m ("TRANSIENT", { { "SUSTAIN", -15 } })
            .m ("CLASSIC CLIP", { { "MODO", 0 }, { "DRIVE", 3 } }).done());
        v.push_back (PB ("KICK", "Lo-fi | Morbida", "Cassa lo-fi: arrotondata, alti chiusi, cassetta.", T_KICK)
            .eq (BT_HCUT, 5000, 0, 1, 0, 1).eq (BT_BELL, 80, 2, 1)
            .m ("OPTO 2A", { { "PEAK RED.", 0.4f } })
            .m ("TAPE MACHINE", { { "MACCHINA", 2 }, { "DRIVE", 0.4f } }).done());
        // ================================================================ 808 / BASSO
        v.push_back (PB ("808", "Trap | Grosso che si sente", "808 trap: taglio del sub inutile, saturazione per farlo sentire nei telefoni, clip.", T_808)
            .eq (BT_LCUT, 28, 0, 1, 0, 2).eq (BT_BELL, 300, -3, 1.2f)
            .m ("DECAP", { { "DRIVE", 0.35f }, { "STILE", 1 }, { "TONE", 0.2f } })
            .m ("MB SAT", { { "BASSI", 0.2f }, { "MEDI", 0.4f }, { "TIPO MEDI", 1 } })
            .m ("CLASSIC CLIP", { { "MODO", 2 }, { "DRIVE", 4 }, { "CEILING", -2 } }).done());
        v.push_back (PB ("808", "Drill | Scivolo e grinta", "808 drill: piu medio, compressione e un filo di distorsione.", T_808)
            .eq (BT_LCUT, 30, 0, 1, 0, 2).eq (BT_BELL, 800, 2, 1)
            .m ("RMS 160", { { "THRESH", 0.35f }, { "RATIO", 2 } })
            .m ("OVERDRIVE 808", { { "DRIVE", 0.3f } }, 0.35f)
            .m ("CLASSIC CLIP", { { "MODO", 3 }, { "DRIVE", 3 } }).done());
        v.push_back (PB ("808", "Pulito | Solo sub", "808 pulito per R&B: solo il sub, nessuna saturazione, mono sotto.", T_808)
            .eq (BT_LCUT, 25, 0, 1, 0, 2).eq (BT_HCUT, 2000, 0, 1, 0, 1)
            .m ("OPTO 3A", { { "PEAK RED.", 0.3f } })
            .m ("WIDER", { { "WIDTH", 1 }, { "DA", 150 }, { "MONO SOTTO", 1 } }).done());
        v.push_back (PB ("BASSO", "Elettrico | Funk", "Basso elettrico: 610, Pultec, LA-3A e un filo di drive in parallelo.", T_808)
            .m ("610 PRE", { { "GAIN", 12 } })
            .m ("EQP-1A", { { "LOW FREQ", 2 }, { "BOOST", 4 }, { "ATTEN", 3 }, { "HIGH FREQ", 0 }, { "BOOST#2", 2 } })
            .m ("OPTO 3A", { { "PEAK RED.", 0.5f } })
            .m ("AMP CRUNCH", { { "DRIVE", 0.3f } }, 0.25f).done());
        v.push_back (PB ("BASSO", "Synth Bass | Avanti", "Basso synth: Tube Box per le armoniche, compressione, clip.", T_808)
            .m ("TUBE BOX", { { "PENTODE", 6 }, { "TRIODE", 5 }, { "SAT", 3 }, { "SAT FREQ", 0 } })
            .m ("BUS VCA", { { "THRESH", 0.4f }, { "RATIO", 1 } })
            .m ("CLASSIC CLIP", { { "MODO", 0 }, { "DRIVE", 2 } }).done());
        v.push_back (PB ("808", "Distorto | Rage", "808 distorto da rage: fuzz in parallelo, wavefolder, tanto clip.", T_808)
            .eq (BT_LCUT, 30, 0, 1, 0, 2)
            .m ("FUZZ FACE", { { "DRIVE", 0.5f } }, 0.4f)
            .m ("WAVEFOLDER", { { "DRIVE", 0.3f } }, 0.3f)
            .m ("CLASSIC CLIP", { { "MODO", 3 }, { "DRIVE", 6 }, { "CEILING", -2 } }).done());
        // ================================================================ RULLANTE / CLAP
        v.push_back (PB ("RULLANTE", "Trap | Clap secco", "Clap trap: corpo a 200, schiocco a 5k, clip.", T_SNR)
            .eq (BT_LCUT, 120, 0, 1, 0, 2).eq (BT_BELL, 220, 2, 1.2f).eq (BT_BELL, 5000, 3, 1).eq (BT_BELL, 900, -2, 2)
            .m ("TRANSIENT", { { "ATTACK", 25 }, { "SUSTAIN", -15 } })
            .m ("CLASSIC CLIP", { { "MODO", 2 }, { "DRIVE", 4 } }).done());
        v.push_back (PB ("RULLANTE", "Boom Bap | Crack", "Rullante 90: 1073, 1176, plate corta.", T_SNR)
            .m ("73 PRE", { { "GAIN", 14 } }).eq (BT_BELL, 900, -3, 3)
            .m ("FET 76", { { "INPUT", 0.5f }, { "ATTACK", 0.6f } })
            .m ("PLATE 140", { { "DECAY", 900 } }, 0.15f).eqAt (1).done());
        v.push_back (PB ("RULLANTE", "Rock | Grosso con stanza", "Rullante acustico: gate, ring tolto in dinamico, Distressor, room.", T_SNR)
            .m ("GATE", { { "THRESH", -28 }, { "RANGE", 20 } })
            .eq (BT_BELL, 220, 3, 1.2f).eq (BT_BELL, 800, -5, 6).dyn (-30, -6).eq (BT_HSHELF, 8000, 3, 0.7f)
            .m ("DISTRESS", { { "INPUT", 0.45f }, { "RATIO", 2 } })
            .m ("ROOM", { { "DECAY", 900 } }, 0.2f).eqAt (1).done());
        v.push_back (PB ("RULLANTE", "Drill | Snap stretto", "Rullante drill: stretto, alto, ritardo sincopato corto.", T_SNR)
            .eq (BT_LCUT, 180, 0, 1, 0, 2).eq (BT_BELL, 3500, 3, 1)
            .m ("TRANSIENT", { { "ATTACK", 35 }, { "SUSTAIN", -25 } })
            .m ("CLASSIC CLIP", { { "MODO", 2 }, { "DRIVE", 3 } })
            .m ("ROOM", { { "DECAY", 500 } }, 0.1f).done());
        v.push_back (PB ("RULLANTE", "Lo-fi | Spazzola", "Rullante lo-fi: scuro, nastro, spring.", T_SNR)
            .eq (BT_HCUT, 7000, 0, 1, 0, 1)
            .m ("TAPE MACHINE", { { "MACCHINA", 2 }, { "DRIVE", 0.5f } })
            .m ("SPRING", { { "DECAY", 1500 } }, 0.2f).done());
        // ================================================================ HI-HAT / PERC
        v.push_back (PB ("HI-HAT", "Trap | Lucidi e stretti", "Hat trap: niente bassi, tagliate le durezze in spettrale, aria e un filo di larghezza.", T_HAT)
            .eq (BT_LCUT, 400, 0, 1, 0, 2).eq (BT_BELL, 8000, -3, 3).spec (-40, -6).eq (BT_HSHELF, 12000, 2, 0.7f)
            .m ("WIDER", { { "WIDTH", 1.2f }, { "DA", 3000 } }).done());
        v.push_back (PB ("HI-HAT", "Boom Bap | Sporchi", "Hat 90: bit ridotti in parallelo, nastro, scuri.", T_HAT)
            .eq (BT_LCUT, 300, 0, 1, 0, 2).eq (BT_HCUT, 11000, 0, 1, 0, 1)
            .m ("LO-FI", { { "DRIVE", 0.3f } }, 0.35f)
            .m ("TAPE MACHINE", { { "DRIVE", 0.4f } }).done());
        v.push_back (PB ("PERC", "Percussioni | Spazio", "Conga, shaker, rim: transiente, wider e chamber.", T_HAT)
            .eq (BT_LCUT, 150, 0, 1, 0, 1)
            .m ("TRANSIENT", { { "ATTACK", 20 } })
            .m ("WIDER", { { "WIDTH", 1.4f }, { "DA", 500 } })
            .m ("CHAMBER", { { "DECAY", 1100 } }, 0.18f).done());
        v.push_back (PB ("HI-HAT", "Drill | Rimbalzo", "Hat drill: delay a 1/16 terzinato basso, transiente.", T_HAT)
            .eq (BT_LCUT, 350, 0, 1, 0, 2)
            .m ("TRANSIENT", { { "ATTACK", 25 }, { "SUSTAIN", -15 } })
            .m ("DIGITAL 2290", { { "TEMPO", 1 }, { "FEEDBACK", 0.2f } }, 0.12f).done());
        v.push_back (PB ("PERC", "Open Hat e Piatti | Morbidi", "Piatti e open hat: durezze tolte in spettrale, alti morbidi.", T_HAT)
            .eq (BT_LCUT, 350, 0, 1, 0, 1).eq (BT_BELL, 5000, -3, 2).spec (-42, -6)
            .m ("TAPE MACHINE", { { "MACCHINA", 0 }, { "DRIVE", 0.3f } }).done());
        // ================================================================ DRUM BUS
        v.push_back (PB ("DRUM BUS", "Trap | Incollato e forte", "Bus batteria trap: VCA a 30 ms, Tube Box, clip.", T_DRUMS)
            .m ("BUS VCA", { { "THRESH", 0.35f }, { "RATIO", 1 }, { "ATTACK", 0.8f }, { "RELEASE", 1 } })
            .m ("TUBE BOX", { { "PENTODE", 5.5f }, { "TRIODE", 4 } })
            .m ("CLASSIC CLIP", { { "MODO", 1 }, { "DRIVE", 3 } }).done());
        v.push_back (PB ("DRUM BUS", "Boom Bap | Schiacciato", "Bus 90: 1176 in ALL in parallelo, nastro, low pass morbido.", T_DRUMS)
            .m ("FET 76", { { "INPUT", 0.6f }, { "RATIO", 4 } }, 0.4f)
            .m ("TAPE MACHINE", { { "DRIVE", 0.5f } }).eq (BT_HCUT, 14000, 0, 0.7f, 0, 0).eqAt (2).done());
        v.push_back (PB ("DRUM BUS", "Rock | Stanza grossa", "Bus rock: Distressor, room, transienti.", T_DRUMS)
            .m ("TRANSIENT", { { "ATTACK", 15 } })
            .m ("DISTRESS", { { "INPUT", 0.4f }, { "RATIO", 2 } }, 0.6f)
            .m ("ROOM", { { "DECAY", 1000 } }, 0.15f).done());
        v.push_back (PB ("DRUM BUS", "Lo-fi | Vinile", "Bus lo-fi: banda stretta, cassetta, SSL comp lento.", T_DRUMS)
            .eq (BT_LCUT, 50, 0, 1, 0, 1).eq (BT_HCUT, 9000, 0, 1, 0, 1)
            .m ("BUS VCA", { { "THRESH", 0.3f }, { "RATIO", 0 } })
            .m ("TAPE MACHINE", { { "MACCHINA", 2 }, { "DRIVE", 0.5f }, { "FRUSCIO", 0.2f } }).done());
        v.push_back (PB ("DRUM BUS", "Pulito | Solo colla", "Bus moderno trasparente: VCA leggero e limiter morbido.", T_DRUMS)
            .m ("BUS VCA", { { "THRESH", 0.25f }, { "RATIO", 0 }, { "ATTACK", 0.8f } })
            .m ("LIMITER", { { "STILE", 1 }, { "GAIN", 2 }, { "CEILING", -2 } }).done());
        v.push_back (PB ("DRUM BUS", "OTT | In faccia", "OTT in parallelo al 30%: tutti i dettagli davanti.", T_DRUMS)
            .m ("OTT", { { "DEPTH", 0.5f } }, 0.3f)
            .m ("CLASSIC CLIP", { { "MODO", 1 }, { "DRIVE", 2 } }).done());
        // ================================================================ CHITARRA
        v.push_back (PB ("CHITARRA", "Acustica | Brillante", "Acustica: 81 pulito, taglio del rimbombo, LA-2A, aria, plate.", T_GTR)
            .m ("81 PRE", { { "GAIN", 6 }, { "LOW CUT", 2 } }).eq (BT_BELL, 200, -3, 1.2f).dyn (-30, -4).eq (BT_HSHELF, 10000, 3, 0.7f)
            .m ("OPTO 2A", { { "PEAK RED.", 0.35f } })
            .m ("PLATE 140", { { "DECAY", 1500 } }, 0.12f).eqAt (1).done());
        v.push_back (PB ("CHITARRA", "Elettrica | Crunch", "DI o pulita: overdrive, amp e room.", T_GTR)
            .m ("OVERDRIVE 808", { { "DRIVE", 0.3f } })
            .m ("AMP CRUNCH", { { "DRIVE", 0.5f } })
            .m ("ROOM", { { "DECAY", 700 } }, 0.12f).done());
        v.push_back (PB ("CHITARRA", "Clean | Chorus e spazio", "Chitarra clean anni 80: chorus, delay, hall.", T_GTR)
            .m ("OPTO 3A", { { "PEAK RED.", 0.35f } })
            .m ("CHORUS CE-1", {}, 0.4f)
            .m ("TAPE ECHO", { { "TEMPO", 7 }, { "FEEDBACK", 0.3f } }, 0.18f)
            .m ("HALL 480", { { "DECAY", 2500 } }, 0.15f).done());
        v.push_back (PB ("CHITARRA", "Sample | Vintage campionata", "Chitarra campionata per beat: banda stretta, nastro, spring.", T_GTR)
            .eq (BT_LCUT, 150, 0, 1, 0, 2).eq (BT_HCUT, 8000, 0, 1, 0, 1)
            .m ("TAPE MACHINE", { { "MACCHINA", 1 }, { "DRIVE", 0.4f } })
            .m ("SPRING", { { "DECAY", 2000 } }, 0.18f).done());
        v.push_back (PB ("CHITARRA", "Distorta | Muro", "Chitarra distorta: RAT, amp, taglio dei medi duri, wider.", T_GTR)
            .m ("RAT", { { "DRIVE", 0.5f } })
            .m ("AMP CRUNCH", { { "DRIVE", 0.4f } }).eq (BT_BELL, 2500, -3, 2).spec (-36, -5).eq (BT_LCUT, 90, 0, 1, 0, 1)
            .m ("WIDER", { { "WIDTH", 1.3f }, { "DA", 300 } }).eqAt (2).done());
        // ================================================================ PIANO / KEYS
        v.push_back (PB ("PIANO", "Piano | Caldo e vicino", "Piano: Trident, Pultec, chamber.", T_KEYS)
            .m ("TRIDENT PRE", { { "GAIN", 8 } })
            .m ("EQP-1A", { { "LOW FREQ", 3 }, { "BOOST", 2 }, { "HIGH FREQ", 4 }, { "BOOST#2", 2 } })
            .m ("OPTO 2A", { { "PEAK RED.", 0.3f } })
            .m ("CHAMBER", { { "DECAY", 1800 } }, 0.15f).done());
        v.push_back (PB ("PIANO", "Rhodes | Lo-fi", "Rhodes per beat: phaser lento, nastro, spring.", T_KEYS)
            .m ("PHASER 90", { { "RATE", 0.3f } }, 0.35f)
            .m ("TAPE MACHINE", { { "MACCHINA", 2 }, { "DRIVE", 0.4f } })
            .m ("SPRING", { { "DECAY", 2200 } }, 0.15f).done());
        v.push_back (PB ("PIANO", "Organo | Leslie e grinta", "Organo: tube box, chorus, room.", T_KEYS)
            .m ("TUBE BOX", { { "PENTODE", 6 }, { "TRIODE", 6 } })
            .m ("VIBRATO", {}, 0.4f)
            .m ("ROOM", { { "DECAY", 900 } }, 0.12f).done());
        v.push_back (PB ("PIANO", "Piano | Pop brillante", "Piano pop: niente fango, aria, plate.", T_KEYS)
            .eq (BT_BELL, 300, -3, 1).eq (BT_HSHELF, 9000, 3, 0.7f)
            .m ("BUS VCA", { { "THRESH", 0.25f } })
            .m ("PLATE 140", { { "DECAY", 1800 } }, 0.12f).done());
        v.push_back (PB ("PIANO", "Keys | Larghi", "Keys e pad di piano: dimension, wider, hall.", T_KEYS)
            .m ("DIMENSION D", {}, 1.0f)
            .m ("WIDER", { { "WIDTH", 1.4f }, { "DA", 400 } })
            .m ("HALL 480", { { "DECAY", 2800 } }, 0.15f).done());
        // ================================================================ SYNTH / PAD
        v.push_back (PB ("SYNTH", "Pad | Infinito", "Pad: Blackhole con shimmer, wider, alti morbidi.", T_SYN)
            .eq (BT_LCUT, 150, 0, 1, 0, 1).eq (BT_HCUT, 12000, 0, 1, 0, 1)
            .m ("WIDER", { { "WIDTH", 1.5f }, { "DA", 300 } })
            .m ("BLACKHOLE", { { "DECAY", 9000 }, { "SHIMMER", 0.3f } }, 0.3f).done());
        v.push_back (PB ("SYNTH", "Lead | Avanti", "Lead synth: saturazione, compressione, delay a 1/8 puntato.", T_SYN)
            .m ("DECAP", { { "DRIVE", 0.3f }, { "STILE", 3 } })
            .m ("FET 76", { { "INPUT", 0.4f } })
            .m ("TAPE ECHO", { { "TEMPO", 6 }, { "FEEDBACK", 0.35f } }, 0.2f).done());
        v.push_back (PB ("SYNTH", "Pluck | Rimbalzo", "Pluck: ping pong a 1/16, plate corta.", T_SYN)
            .m ("PING PONG", { { "TEMPO", 2 }, { "FEEDBACK", 0.4f } }, 0.2f)
            .m ("PLATE 140", { { "DECAY", 1200 } }, 0.15f).done());
        v.push_back (PB ("SYNTH", "Arp | Wide e dinamico", "Arpeggio: OTT al 30%, wider, delay digitale.", T_SYN)
            .m ("OTT", { { "DEPTH", 0.4f } }, 0.3f)
            .m ("WIDER", { { "WIDTH", 1.4f }, { "DA", 500 } })
            .m ("DIGITAL 2290", { { "TEMPO", 5 }, { "FEEDBACK", 0.3f } }, 0.15f).done());
        v.push_back (PB ("SYNTH", "Bass Synth | Analogico", "Basso synth analogico: Decap N, mono sotto, compressione.", T_SYN)
            .m ("DECAP", { { "DRIVE", 0.3f }, { "STILE", 2 } })
            .m ("WIDER", { { "WIDTH", 1 }, { "DA", 150 }, { "MONO SOTTO", 1 } })
            .m ("RMS 160", { { "THRESH", 0.3f } }).done());
        // ================================================================ SAMPLE / LOOP
        v.push_back (PB ("SAMPLE", "Soul | Disco vecchio", "Campione soul: banda da vinile, Pultec, nastro, compressione lenta.", T_SMP)
            .eq (BT_LCUT, 80, 0, 1, 0, 1).eq (BT_HCUT, 12000, 0, 1, 0, 1)
            .m ("EQP-1A", { { "LOW FREQ", 2 }, { "BOOST", 3 }, { "ATTEN", 2 } })
            .m ("TAPE MACHINE", { { "DRIVE", 0.4f } })
            .m ("OPTO 2A", { { "PEAK RED.", 0.35f } }).done());
        v.push_back (PB ("SAMPLE", "Pulisci | Risonanze via", "Loop con fischi e rimbombi: EQ spettrale sulle zone critiche, poi niente altro.", T_SMP)
            .eq (BT_BELL, 250, -4, 1.5f).spec (-36, -6).eq (BT_BELL, 3000, -4, 1.2f).spec (-40, -6).eq (BT_LCUT, 40, 0, 1, 0, 2).done());
        v.push_back (PB ("SAMPLE", "Lo-fi | Cassetta", "Loop lo-fi: cassetta, bit ridotti in parallelo, filtrato.", T_SMP)
            .eq (BT_LCUT, 120, 0, 1, 0, 2).eq (BT_HCUT, 7000, 0, 1, 0, 1)
            .m ("TAPE MACHINE", { { "MACCHINA", 2 }, { "DRIVE", 0.5f }, { "FRUSCIO", 0.3f } })
            .m ("LO-FI", { { "DRIVE", 0.3f } }, 0.3f).done());
        v.push_back (PB ("SAMPLE", "Spazio | Lato del beat", "Sample di accompagnamento: lati larghi, centro libero per la voce.", T_SMP)
            .eq (BT_BELL, 2500, -3, 1, 1).eq (BT_LCUT, 120, 0, 1, 2, 1)
            .m ("WIDER", { { "WIDTH", 1.5f }, { "DA", 250 } })
            .m ("HALL 480", { { "DECAY", 2000 } }, 0.1f).done());
        v.push_back (PB ("SAMPLE", "Filtro | Intro chiusa", "Intro: passa basso a 800 Hz, nastro, room. Togli il filtro quando entra il beat.", T_SMP)
            .eq (BT_HCUT, 800, 0, 1.2f, 0, 2)
            .m ("TAPE MACHINE", { { "DRIVE", 0.3f } })
            .m ("ROOM", { { "DECAY", 900 } }, 0.15f).done());
        v.push_back (PB ("SAMPLE", "Chop | Ducking sulla voce", "Sample con compressore in SIDECHAIN: si abbassa quando entra la voce (collega la voce al sidechain).", T_SMP)
            .m ("BUS VCA", { { "THRESH", 0.5f }, { "RATIO", 1 }, { "SIDECHAIN", 1 }, { "AUTO GAIN", 0 } })
            .m ("TAPE MACHINE", { { "DRIVE", 0.3f } }).done());
        // ================================================================ MIX BUS
        v.push_back (PB ("MIX BUS", "Hip Hop | Colla e calore", "Mix bus: VCA a 2:1, nastro 15 ips, Pultec leggero.", T_MIX)
            .m ("BUS VCA", { { "THRESH", 0.2f }, { "RATIO", 0 }, { "ATTACK", 0.8f }, { "RELEASE", 1 } })
            .m ("TAPE MACHINE", { { "DRIVE", 0.3f } })
            .m ("EQP-1A", { { "LOW FREQ", 1 }, { "BOOST", 2 }, { "ATTEN", 1.5f }, { "HIGH FREQ", 5 }, { "BOOST#2", 1.5f }, { "TUBE", 0.15f } }).done());
        v.push_back (PB ("MIX BUS", "Trap | Moderno pulito", "VCA, Tube Box leggero, sub in mono.", T_MIX)
            .m ("BUS VCA", { { "THRESH", 0.2f }, { "RATIO", 0 } })
            .m ("TUBE BOX", { { "PENTODE", 5 }, { "TRIODE", 4 } })
            .m ("WIDER", { { "WIDTH", 1.1f }, { "DA", 120 }, { "MONO SOTTO", 1 } }).done());
        v.push_back (PB ("MIX BUS", "Vintage | Varimu", "Vari-mu lento, nastro: colla d'altri tempi.", T_MIX)
            .m ("VARI-MU", { { "INPUT", 0.3f }, { "TIME CONST.", 1 } })
            .m ("TAPE MACHINE", { { "MACCHINA", 0 }, { "DRIVE", 0.35f } }).done());
        v.push_back (PB ("MIX BUS", "Lo-fi | Beat tape", "Bus lo-fi: banda stretta, cassetta, compressione.", T_MIX)
            .eq (BT_LCUT, 40, 0, 1, 0, 1).eq (BT_HCUT, 11000, 0, 1, 0, 1)
            .m ("BUS VCA", { { "THRESH", 0.3f }, { "RATIO", 1 } })
            .m ("TAPE MACHINE", { { "MACCHINA", 2 }, { "DRIVE", 0.45f }, { "FRUSCIO", 0.2f } }).done());
        v.push_back (PB ("MIX BUS", "Mid/Side | Aperto", "EQ M/S: centro pulito sui medi bassi, lati brillanti, bassi solo al centro.", T_MIX)
            .eq (BT_BELL, 300, -1.5f, 1, 1).eq (BT_HSHELF, 8000, 2, 0.7f, 2).eq (BT_LCUT, 120, 0, 1, 2, 1)
            .m ("BUS VCA", { { "THRESH", 0.2f } }).done());
        // ================================================================ MASTER
        v.push_back (PB ("MASTER", "Streaming | Pulito", "Master per streaming: EQ leggero, VCA, limiter pulito a -1 dBTP.", T_MST)
            .eq (BT_LCUT, 25, 0, 1, 0, 2).eq (BT_HSHELF, 12000, 1, 0.7f)
            .m ("BUS VCA", { { "THRESH", 0.2f }, { "RATIO", 0 } })
            .m ("LIMITER", { { "STILE", 0 }, { "GAIN", 6 }, { "CEILING", -1 } }).done());
        v.push_back (PB ("MASTER", "Trap Drill | Forte", "Master loud: clip prima del limiter, limiter aggressivo.", T_LOUD)
            .eq (BT_LCUT, 25, 0, 1, 0, 2)
            .m ("TUBE BOX", { { "PENTODE", 5 }, { "TRIODE", 4 } })
            .m ("CLASSIC CLIP", { { "MODO", 3 }, { "DRIVE", 4 }, { "CEILING", -1.5f } })
            .m ("LIMITER", { { "STILE", 2 }, { "GAIN", 8 }, { "CEILING", -1 } }).done());
        v.push_back (PB ("MASTER", "Boom Bap | Caldo", "Master caldo: nastro, Pultec, limiter morbido.", T_MST)
            .m ("TAPE MACHINE", { { "DRIVE", 0.35f } })
            .m ("EQP-1A", { { "LOW FREQ", 1 }, { "BOOST", 1.5f }, { "ATTEN", 1 }, { "HIGH FREQ", 5 }, { "BOOST#2", 1 }, { "TUBE", 0.1f } })
            .m ("LIMITER", { { "STILE", 3 }, { "GAIN", 6 }, { "CEILING", -1 } }).done());
        v.push_back (PB ("MASTER", "R&B | Morbido e largo", "Master R&B: EQ spettrale sui medi alti, wider sopra 2k, limiter pulito.", T_MST)
            .eq (BT_BELL, 3000, -2, 1).spec (-34, -3)
            .m ("WIDER", { { "WIDTH", 1.12f }, { "DA", 2000 } })
            .m ("LIMITER", { { "STILE", 0 }, { "GAIN", 5 }, { "CEILING", -1 } }).done());
        v.push_back (PB ("MASTER", "Lo-fi | Morbido", "Master lo-fi: cassetta leggera, limiter morbido, poco volume.", T_MST)
            .m ("TAPE MACHINE", { { "MACCHINA", 2 }, { "DRIVE", 0.3f } })
            .m ("LIMITER", { { "STILE", 3 }, { "GAIN", 4 }, { "CEILING", -1 } }).done());
        v.push_back (PB ("MASTER", "Pop | Brillante", "Master pop: aria, VCA, clip leggero e limiter punch.", T_MST)
            .eq (BT_HSHELF, 10000, 1.5f, 0.7f).eq (BT_BELL, 250, -1, 1)
            .m ("BUS VCA", { { "THRESH", 0.2f } })
            .m ("CLASSIC CLIP", { { "MODO", 1 }, { "DRIVE", 2 }, { "CEILING", -1.5f } })
            .m ("LIMITER", { { "STILE", 1 }, { "GAIN", 6 }, { "CEILING", -1 } }).done());
        // ================================================================ CREATIVI
        v.push_back (PB ("FX", "Dub | Delay che si avvita", "Voce o rullante dub: delay con feedback alto e spring. Alza FEEDBACK oltre 100% per l'autoscillazione.", T_VOX)
            .m ("DUB DELAY", { { "FEEDBACK", 0.8f } }, 0.35f)
            .m ("SPRING", {}, 0.25f).done());
        v.push_back (PB ("FX", "Transizione | Lavaggio", "Lunghissimo e ampio: Blackhole con shimmer al massimo.", T_SYN)
            .m ("BLACKHOLE", { { "DECAY", 15000 }, { "SHIMMER", 0.6f } }, 0.6f).done());
        v.push_back (PB ("FX", "Megafono | Bridge", "Voce o sample attraverso un megafono rotto.", T_VOX)
            .m ("RADIO", { { "DRIVE", 0.7f } })
            .m ("HARD CLIP", { { "DRIVE", 0.4f } }).done());
        v.push_back (PB ("FX", "Underwater | Sott'acqua", "Passa basso stretto, chorus lento, hall.", T_SMP)
            .eq (BT_HCUT, 600, 0, 1.4f, 0, 2)
            .m ("CHORUS CE-1", {}, 0.5f)
            .m ("HALL 480", { { "DECAY", 3000 } }, 0.3f).done());
        v.push_back (PB ("FX", "Octave Fuzz | Synth sporco", "Octavia + wavefolder: per synth e bassi che urlano.", T_SYN)
            .m ("OCTAVIA", { { "DRIVE", 0.5f } })
            .m ("WAVEFOLDER", { { "DRIVE", 0.3f } }, 0.5f).done());
        v.push_back (PB ("FX", "Flanger | Jet sul beat", "Flanger sul loop o sul bus prima del drop.", T_DRUMS)
            .m ("FLANGER", {}, 0.5f).done());
        return v;
    }();
    return P;
}
}
