#include "MixModules.h"

namespace jmx
{
// ======================================================================= costruzione del catalogo
static PSpec S (const char* l, float lo, float hi, float def, int fmt, juce::StringArray it = {}, int cap = -1)
{ PSpec s; s.label = l; s.lo = lo; s.hi = hi; s.def = def; s.fmt = fmt; s.items = std::move (it); s.cap = cap; return s; }
static PSpec H() { return PSpec(); }   // comando nascosto
static Look look (juce::uint32 plate, juce::uint32 ink, juce::uint32 acc, int knob, int meter, bool dark) { Look l; l.plate = plate; l.ink = ink; l.accent = acc; l.knob = knob; l.meter = meter; l.dark = dark; return l; }
static std::vector<PSpec> fromPre (int jt) { auto v = jpre::modSpec (jt); for (auto& s : v) s.cap = -1; return v; }
static juce::String msText (float ms) { return ms < 1 ? juce::String (ms, 2) + " ms" : ms < 1000 ? juce::String (ms, ms < 10 ? 1 : 0) + " ms" : juce::String (ms / 1000.0f, ms < 10000 ? 2 : 1) + " s"; }
static juce::String hzText (float f) { return f >= 1000 ? juce::String (f / 1000.0f, f >= 10000 ? 1 : 2) + " kHz" : juce::String (juce::roundToInt (f)) + " Hz"; }

const std::vector<ModDef>& mods()
{
    static const std::vector<ModDef> all = []
    {
        std::vector<ModDef> v;
        auto add = [&v] (ModDef d) { v.push_back (std::move (d)); return (int) v.size() - 1; };
        { ModDef e; e.name = "VUOTO"; e.desc = "Scegli un modulo."; add (e); }
        // ---------------------------------------------------------------- preamp
        {
            struct P { const char* insp; Look lk; };
            const P pl[jpre::kPreModels] = {
                { "Neve 1073", look (0xff6f7c86, 0xfff2efe6, 0xffd8452f, 0, 2, true) },          // grigio blu Neve, manopola rossa
                { "Neve 1081 / Lindell", look (0xff5d6a74, 0xfff2efe6, 0xffd8452f, 1, 2, true) },
                { "Telefunken V76", look (0xff7d8a7a, 0xfff2efe6, 0xffe0b04a, 2, 2, true) },        // verde grigio da broadcast
                { "UA 610", look (0xff1d1c1b, 0xffe8e2d2, 0xffd9a441, 2, 2, true) },
                { "API 312", look (0xff3c4c5c, 0xfff0eee6, 0xff4fa3ff, 1, 2, true) },
                { "SSL 4000 E", look (0xff2c2f33, 0xffe9e9e9, 0xffe24b3b, 1, 2, true) },
                { "EMI TG2", look (0xff9aa0a4, 0xff1c1c1c, 0xffd8452f, 2, 2, false) },
                { "Avalon 737", look (0xff141414, 0xffd9b45c, 0xff4fa3ff, 2, 2, true) },             // nero e oro con lampade blu
                { "Neve 1272", look (0xff4f6274, 0xfff2efe6, 0xffd8452f, 0, 2, true) },
                { "Helios 69", look (0xff3a3a3a, 0xfff0e9d6, 0xffe08a2f, 2, 2, true) },
                { "Trident A-Range", look (0xff2b3a2f, 0xfff0e9d6, 0xffd8452f, 1, 2, true) } };
            for (int m = 0; m < jpre::kPreModels; ++m)
            {
                ModDef d; d.name = juce::String (jpre::preInfo (m).name) + " PRE"; d.inspired = pl[m].insp; d.desc = jpre::preInfo (m).desc;
                d.cat = C_PRE; d.kind = K_PRE; d.sub = 1 + m; d.look = pl[m].lk; d.spec = fromPre (1 + m); d.spec.resize (kP);
                add (d);
            }
        }
        // ---------------------------------------------------------------- EQ di colore
        {
            const char* insp[] = { "Pultec EQP-1A", "Pultec MEQ-5", "Maag EQ4", "SSL 4000 E", "Neve 1084", "Lindell 7X / 1081", "Neve 1073", "Tube-Tech PE 1C" };
            const Look lk[] = { look (0xff3e4a57, 0xfff0ebe0, 0xffd9a441, 2, 2, true), look (0xff3e4a57, 0xfff0ebe0, 0xffd9a441, 2, 2, true), look (0xff1a1a1a, 0xffffffff, 0xffff9f1a, 2, 0, true),
                                look (0xff2c2f33, 0xffe9e9e9, 0xffe24b3b, 1, 0, true), look (0xff6f7c86, 0xfff2efe6, 0xffd8452f, 0, 0, true), look (0xff5d6a74, 0xfff2efe6, 0xffd8452f, 1, 0, true),
                                look (0xff6f7c86, 0xfff2efe6, 0xffd8452f, 0, 0, true), look (0xffb7b3a6, 0xff1c1c1c, 0xffd8452f, 2, 2, false) };
            for (int e = 0; e < jpre::kEqModels; ++e)
            {
                const int jt = 1 + jpre::kPreModels + e;
                ModDef d; d.name = jpre::eqName (e); d.inspired = insp[e]; d.desc = jpre::eqDesc (e);
                d.cat = C_EQ; d.kind = K_CEQ; d.sub = jt; d.look = lk[e]; d.spec = fromPre (jt); d.spec.resize (kP);
                add (d);
            }
        }
        // ---------------------------------------------------------------- de-esser
        {
            ModDef d; d.name = "DS 902"; d.inspired = "dbx 902 / Lindell 902"; d.cat = C_DEESS; d.kind = K_DS; d.sub = 0;
            d.desc = "De-esser analogico classico: sopra la frequenza scelta abbassa solo le sibilanti (SPLIT) o tutta la voce per un istante (WIDE). Veloce e trasparente.";
            d.look = look (0xff202226, 0xffe6e6e6, 0xffe7b416, 2, 3, true);
            d.spec = { S ("FREQ", 2500, 12000, 6000, F_HZ), S ("THRESH", -50, 0, -24, F_DB), S ("RANGE", 0, 20, 8, F_DB), S ("MODO", 0, 1, 0, F_ITEMS, { "SPLIT", "WIDE" }), S ("ASCOLTA", 0, 1, 0, F_BOOL) };
            d.spec.resize (kP); add (d);
            ModDef p; p.name = "PRO DS"; p.inspired = "FabFilter Pro-DS"; p.cat = C_DEESS; p.kind = K_DS; p.sub = 1;
            p.desc = "De-esser intelligente: guarda quanto le sibilanti sporgono sul resto della voce (non il volume assoluto), con lookahead: lavora uguale su strofe piano e ritornelli forti.";
            p.look = look (0xff1c2a33, 0xffd8f0ff, 0xff38c6ff, 2, 3, true);
            p.spec = { S ("SOGLIA", -30, 30, 0, F_NUM), S ("RANGE", 0, 20, 8, F_DB), S ("DA", 2500, 12000, 5500, F_HZ), S ("A", 8000, 20000, 16000, F_HZ),
                       S ("MODO", 0, 1, 0, F_ITEMS, { "SPLIT", "WIDE" }), S ("LOOKAHEAD", 0, 1, 1, F_BOOL), S ("ASCOLTA", 0, 1, 0, F_BOOL) };
            p.spec.resize (kP); add (p);
        }
        // ---------------------------------------------------------------- saturatori
        {
            ModDef d; d.name = "DECAP"; d.inspired = "Soundtoys Decapitator"; d.cat = C_SAT; d.kind = K_DECAP;
            d.desc = "Saturatore analogico a cinque stili: A nastro, E trasformatore, N console, T valvola, P taglio duro. TONE inclina scuro/brillante, PUNISH spinge di 20 dB.";
            d.look = look (0xff1d1f22, 0xffe9e9e9, 0xffff3b2f, 2, 2, true);
            d.spec = { S ("DRIVE", 0, 1, 0.3f, F_PCT), S ("STILE", 0, 4, 0, F_ITEMS, { "A", "E", "N", "T", "P" }), S ("TONE", -1, 1, 0, F_BIP), S ("LOW CUT", 10, 600, 10, F_HZ),
                       S ("HIGH CUT", 2000, 22000, 22000, F_HZ), S ("PUNISH", 0, 1, 0, F_BOOL), S ("OUTPUT", -24, 12, 0, F_DB), S ("AUTO", 0, 1, 1, F_BOOL) };
            d.spec.resize (kP); add (d);
            ModDef t; t.name = "TUBE BOX"; t.inspired = "Black Box HG-2"; t.cat = C_SAT; t.kind = K_TUBE;
            t.desc = "Pentodo e triodo in serie come l'HG-2: PENTODE spinge, TRIODE scalda, SAT in parallelo con la sua frequenza, AIR apre gli alti. Il nostro Tube Box dentro il rack.";
            t.look = look (0xff16181b, 0xffd9d9d9, 0xffff9f1a, 2, 2, true);
            t.spec = { S ("INPUT", -12, 12, 0, F_DB), S ("PENTODE", 0, 10, 5, F_NUM), S ("TRIODE", 0, 10, 5, F_NUM), S ("DENSITY", 0, 10, 0, F_NUM), S ("SAT", 0, 10, 0, F_NUM),
                       S ("SAT FREQ", 0, 2, 1, F_ITEMS, { "LOW", "FLAT", "HIGH" }), S ("AIR", 0, 10, 0, F_NUM), S ("CALIB", 0, 2, 1, F_ITEMS, { "DARK", "NORMAL", "BRIGHT" }), S ("OUTPUT", -12, 12, 0, F_DB) };
            t.spec.resize (kP); add (t);
            ModDef m; m.name = "MB SAT"; m.inspired = "saturatore multibanda"; m.cat = C_SAT; m.kind = K_MBSAT;
            m.desc = "Saturazione su tre bande, ognuna col suo carattere (valvola, nastro, triodo, caldo, retro, brillante): bassi pieni, medi densi e alti lucidi senza impastare.";
            m.look = look (0xff2a1d33, 0xffeedcff, 0xffc77dff, 1, 0, true);
            const juce::StringArray ex { "VALVOLA", "NASTRO", "TRIODO", "CALDO", "RETRO", "BRILLANTE" };
            m.spec = { S ("BASSI", 0, 1, 0, F_PCT), S ("MEDI", 0, 1, 0, F_PCT), S ("ALTI", 0, 1, 0, F_PCT), S ("TIPO BASSI", 0, 5, 0, F_ITEMS, ex), S ("TIPO MEDI", 0, 5, 1, F_ITEMS, ex),
                       S ("TIPO ALTI", 0, 5, 5, F_ITEMS, ex), S ("TAGLIO 1", 80, 1000, 250, F_HZ), S ("TAGLIO 2", 1500, 12000, 4000, F_HZ), S ("OUTPUT", -12, 12, 0, F_DB) };
            m.spec.resize (kP); add (m);
        }
        // ---------------------------------------------------------------- compressori (quelli del nostro Studio Comp)
        {
            const char* desc[] = {
                "Ottico a valvole: lento e musicale, la riduzione segue la voce come una mano sul fader. PEAK RED. quanto comprime.",
                "Ottico a stato solido: piu veloce e mordente del 2A, tiene in riga voci e bassi senza sentirsi.",
                "FET velocissimo: attacco in microsecondi, aggressivo e presente. ALL = tutti i ratio premuti, il suono che schiaccia.",
                "Bus compressor a VCA: la colla del mix e del drum bus. Attacco 30 ms + auto release = punch.",
                "Valvole a pendenza variabile: comprime di piu quanto piu spingi, densita e calore da mastering.",
                "RMS a VCA: preciso e punchy su kick, rullante e basso. Ginocchio duro, molto controllo.",
                "Il compressore che si comporta come vuoi: da invisibile a distruttivo (NUKE). Distorsione e colore regolabili.",
                "Compressione verso l'alto e verso il basso su tre bande: tutto davanti, dettagli in faccia. Usalo col MIX.",
                "Varimu stereo a valvole del 670: time constant fissi, densita e colla d'altri tempi." };
            const int models[] = { jc::OPTO2A, jc::OPTO3A, jc::FET76, jc::VCABUS, jc::VARIMU, jc::RMS160, jc::DISTRESS, jc::OTT, jc::VC670 };
            const char* names[] = { "OPTO 2A", "OPTO 3A", "FET 76", "BUS VCA", "VARI-MU", "RMS 160", "DISTRESS", "OTT", "VARI 670" };
            for (int k = 0; k < 9; ++k)
            {
                const int m = models[k]; const auto& I = jc::info (m);
                ModDef d; d.name = names[k]; d.inspired = juce::String (I.inspired).replace ("ispirato al ", "").replace ("ispirato all'", "").replace ("ispirato a ", ""); d.desc = desc[k];
                d.cat = C_COMP; d.kind = K_COMP; d.sub = m;
                const juce::Colour f (I.face); const bool dark = f.getPerceivedBrightness() < 0.5f;
                d.look = look (I.face, dark ? 0xffeeeeee : 0xff1e1b17, I.accent, dark ? 1 : 2, 1, dark);
                d.spec = { S (I.driveLabel, 0, 1, 0.3f, F_CUSTOM), S (I.ratioLabel, 0, (float) (I.ratios.size() - 1), 0, F_ITEMS, I.ratios),
                           I.attKnob ? S ("ATTACK", 0, 1, 0.5f, F_CUSTOM) : H(), I.relKnob ? S ("RELEASE", 0, 1, 0.5f, F_CUSTOM) : H(),
                           S ("COLORE", 0, 1, 0.3f, F_PCT), S ("OUTPUT", -12, 12, 0, F_DB), S ("AUTO GAIN", 0, 1, 1, F_BOOL), S ("SIDECHAIN", 0, 1, 0, F_ITEMS, { "INTERNO", "ESTERNO" }) };
                d.spec.resize (kP);
                d.custom = [m] (int i, float v) -> juce::String
                {
                    if (i == 0) return jc::fmtDrive (m, v);
                    if (i == 2) return jc::fmtTime (jc::attMs (m, v));
                    if (i == 3) return jc::fmtTime (jc::relMs (m, v));
                    return {};
                };
                add (d);
            }
        }
        // ---------------------------------------------------------------- clipper e limiter
        {
            juce::StringArray modes; for (int m = 0; m < jclp::kModes; ++m) modes.add (jclp::modeName (m));
            ModDef d; d.name = "CLASSIC CLIP"; d.inspired = "IK Classic Clipper"; d.cat = C_CLIP; d.kind = K_CLIP;
            d.desc = "Clipper a nove curve, sovracampionato: toglie i picchi di voci e batterie senza che si senta (SOFT, BUS) o li rende aggressivi (HARD, LOUD). PUNCH salva il transiente.";
            d.look = look (0xff111214, 0xffffffff, 0xffff2d2d, 2, 3, true);
            d.spec = { S ("MODO", 0, (float) (jclp::kModes - 1), 0, F_ITEMS, modes), S ("DRIVE", 0, 24, 0, F_DB), S ("CEILING", -18, 0, 0, F_DB), S ("PUNCH", 0, 1, 0, F_PCT),
                       S ("OVERSAMPLE", 0, 3, 2, F_ITEMS, { "1x", "2x", "4x", "8x" }), S ("OUTPUT", -12, 12, 0, F_DB), S ("LINK", 0, 1, 1, F_BOOL) };
            d.spec.resize (kP); add (d);
            ModDef l; l.name = "LIMITER"; l.inspired = "limiter da mastering"; l.cat = C_CLIP; l.kind = K_LIM;
            l.desc = "Limiter con lookahead e quattro stili (pulito, punch, aggressivo, morbido): alza il volume senza superare il CEILING.";
            l.look = look (0xff20232a, 0xffe5ecf4, 0xff2e8bff, 2, 3, true);
            l.spec = { S ("STILE", 0, 3, 0, F_ITEMS, { "PULITO", "PUNCH", "AGGRESSIVO", "MORBIDO" }), S ("GAIN", 0, 24, 0, F_DB), S ("CEILING", -6, 0, -1, F_DB), S ("CLIP", 0, 1, 0.2f, F_PCT), S ("CARATTERE", 0, 1, 0.5f, F_PCT) };
            l.spec.resize (kP); add (l);
        }
        // ---------------------------------------------------------------- riverberi
        {
            auto rev = [&] (const char* name, const char* insp, const char* desc, int kind, Look lk, float pre, float decMs, float size, float damp, float mod, float er, float shim)
            {
                ModDef d; d.name = name; d.inspired = insp; d.desc = desc; d.cat = C_REV; d.kind = K_REV; d.sub = kind; d.look = lk; d.defMix = 0.25f; d.send = true;
                d.spec = { S ("PRE-DELAY", 0, 250, pre, F_CUSTOM), S ("DECAY", 200, 20000, decMs, F_MS), S ("SIZE", 0, 1, size, F_PCT), S ("DAMP", 1000, 20000, damp, F_HZ),
                           S ("BASSI x", 0.5f, 2, 1, F_CUSTOM), S ("DIFFUSIONE", 0, 1, 0.7f, F_PCT), S ("MOD", 0, 1, mod, F_PCT), (kind == 2 || kind == 3) ? S ("EARLY", 0, 1, er, F_PCT) : H(),
                           S ("LOW CUT", 20, 1000, 150, F_HZ), S ("HIGH CUT", 1000, 20000, 12000, F_HZ), S ("WIDTH", 0, 2, 1, F_PCT), S ("DUCK", 0, 1, 0, F_PCT), kind == 4 ? S ("SHIMMER", 0, 1, shim, F_PCT) : H() };
                d.spec.resize (kP);
                d.custom = [] (int i, float v) -> juce::String { if (i == 0) return juce::String (juce::roundToInt (v * 250)) + " ms"; if (i == 4) return "x" + juce::String (0.5f + v * 1.5f, 2); return {}; };
                add (d);
            };
            rev ("PLATE 140", "EMT 140", "Piastra d'acciaio: denso subito, brillante, nessuna stanza. La voce degli anni 60-70 e il rullante classico.", 0, look (0xff2b2e33, 0xffe8e8e8, 0xffffb02e, 1, 0, true), 0, 1800, 0.45f, 9000, 0.2f, 0, 0);
            rev ("LUSH PLATE", "plate modulata da studio", "Plate morbida e modulata: coda che respira e si apre, ideale per voci R&B e pop.", 0, look (0xff3a2b44, 0xfff1e6ff, 0xffe08bff, 2, 0, true), 20, 2600, 0.55f, 8000, 0.6f, 0, 0);
            rev ("HALL 480", "Lexicon 480L", "Sala da concerto digitale classica: coda larga, bassi lunghi, modulata. Il riverbero dei dischi anni 80-90.", 1, look (0xffcfc8b8, 0xff1d1d1d, 0xffd8452f, 2, 0, false), 25, 2800, 0.7f, 7000, 0.4f, 0, 0);
            rev ("ROOM", "stanza da studio", "Stanza vera: riflessioni primarie e coda corta. Mette batterie e voci nello stesso spazio senza allontanarle.", 2, look (0xff4b3a2a, 0xfff0e2c8, 0xffe8b04a, 1, 0, true), 4, 700, 0.4f, 9000, 0.15f, 0.5f, 0);
            rev ("CHAMBER", "camera d'eco", "Camera di cemento: caldo, scuro e profondo, la coda si gonfia piano. Voci soul e piano.", 3, look (0xff59544d, 0xfff0ebe0, 0xffd9a441, 2, 0, true), 12, 1600, 0.55f, 5500, 0.25f, 0.35f, 0);
            rev ("BLACKHOLE", "Eventide Blackhole", "Spazio enorme e infinito con SHIMMER (ottava sopra nel ritorno): pad, transizioni, voci eteree.", 4, look (0xff0d0f1a, 0xffc7d2ff, 0xff7a5cff, 2, 0, true), 30, 9000, 0.8f, 6000, 0.6f, 0, 0.3f);
            {
                ModDef d; d.name = "SPRING"; d.inspired = "molla da ampli / AKG BX"; d.cat = C_REV; d.kind = K_SPRING; d.defMix = 0.25f; d.send = true;
                d.desc = "Molla vera: il 'boing' metallico e gocciolante del dub, delle chitarre surf e del boom bap sporco. TENSION cambia la molla.";
                d.look = look (0xff1e3a2e, 0xffd9f0e2, 0xff3dff9a, 2, 0, true);
                d.spec = { S ("DECAY", 500, 6000, 2000, F_MS), S ("TENSION", 0, 1, 0.5f, F_PCT), S ("TONE", 0, 1, 0.5f, F_PCT), S ("DRIP", 0, 1, 0.4f, F_PCT),
                           S ("LOW CUT", 40, 1000, 180, F_HZ), S ("HIGH CUT", 1500, 12000, 6000, F_HZ), S ("WIDTH", 0, 2, 0.7f, F_PCT) };
                d.spec.resize (kP); add (d);
            }
            {
                ModDef d; d.name = "CONVOLUTION"; d.inspired = "riverbero a convoluzione"; d.cat = C_REV; d.kind = K_CONV; d.defMix = 0.25f; d.send = true;
                d.desc = "Risposte all'impulso di stanze, plate e sale (generate da noi) o il TUO FILE WAV: lo spazio piu realistico.";
                d.look = look (0xff2a2f3a, 0xffe0e8ff, 0xff8fb3ff, 1, 0, true);
                d.spec = { S ("SPAZIO", 0, 9, 0, F_ITEMS, irNames()), S ("DURATA x", 0.5f, 2, 1, F_CUSTOM), S ("LOW CUT", 20, 1000, 120, F_HZ), S ("HIGH CUT", 1000, 20000, 14000, F_HZ), S ("WIDTH", 0, 2, 1, F_PCT) };
                d.spec.resize (kP);
                d.custom = [] (int i, float v) -> juce::String { if (i == 1) return "x" + juce::String (0.5f + v * 1.5f, 2); return {}; };
                add (d);
            }
        }
        // ---------------------------------------------------------------- delay
        {
            auto dly = [&] (const char* name, const char* insp, const char* desc, int kind, Look lk, bool sync, int div, float ms, float fb, float hi, float drive, float mod)
            {
                ModDef d; d.name = name; d.inspired = insp; d.desc = desc; d.cat = C_DLY; d.kind = K_DLY; d.sub = kind; d.look = lk; d.defMix = 0.25f; d.send = true;
                d.spec = { S ("SYNC", 0, 1, sync ? 1.0f : 0.0f, F_BOOL), S ("TEMPO", 0, 12, (float) div, F_ITEMS, divNames()), S ("TIME", 1, 2000, ms, F_MS), S ("FEEDBACK", 0, 1.05f, fb, F_PCT),
                           S ("LOW CUT", 20, 2000, 150, F_HZ), S ("HIGH CUT", 800, 20000, hi, F_HZ), S ("DRIVE", 0, 1, drive, F_PCT), S ("WOW", 0, 1, mod, F_PCT),
                           S ("WIDTH", 0, 2, 1, F_PCT), S ("DUCK", 0, 1, 0, F_PCT), kind == 3 ? H() : S ("SPREAD", 0, 1, 0, F_PCT) };
                d.spec.resize (kP); add (d);
            };
            dly ("TAPE ECHO", "Roland RE-201 Space Echo", "Eco a nastro: ripetizioni che si scuriscono e saturano, wow e flutter, il nastro che scivola quando cambi tempo.", 0, look (0xff2f3a2c, 0xffe9efe0, 0xffff6b3d, 2, 0, true), true, 7, 375, 0.4f, 5000, 0.35f, 0.35f);
            dly ("BBD ECHO", "Electro-Harmonix Memory Man", "Delay analogico a BBD: scuro, morbido, leggermente modulato. Piu lungo il tempo, piu scuro il suono.", 1, look (0xffb9c0c7, 0xff15181b, 0xffe03a3a, 2, 0, false), true, 5, 300, 0.45f, 4500, 0.3f, 0.4f);
            dly ("DIGITAL 2290", "TC 2290", "Delay digitale pulito e preciso: ripetizioni identiche, per tempi esatti e voci moderne.", 2, look (0xff1b1e24, 0xffe0f2ff, 0xff2ecbff, 2, 0, true), true, 5, 375, 0.3f, 16000, 0.0f, 0.0f);
            dly ("PING PONG", "delay stereo incrociato", "Le ripetizioni rimbalzano tra sinistra e destra: allarga voci e synth lasciando il centro libero.", 3, look (0xff242833, 0xffe6e9ff, 0xff8f7bff, 1, 0, true), true, 5, 375, 0.45f, 9000, 0.1f, 0.1f);
            dly ("DUB DELAY", "eco dub giamaicano", "Feedback alto, ripetizioni che saturano e si stringono di banda: spingi FEEDBACK oltre il 100% per l'autoscillazione.", 4, look (0xff3a2a1a, 0xfff7e6c8, 0xffffc23d, 0, 0, true), true, 6, 450, 0.7f, 3000, 0.5f, 0.3f);
            dly ("SLAPBACK", "slap da nastro anni 50", "Una sola ripetizione corta (60-140 ms) da nastro: spessore e anni 50 su voci e chitarre.", 5, look (0xffc9a77a, 0xff241a10, 0xffb5402b, 2, 0, false), false, 2, 95, 0.0f, 7000, 0.2f, 0.1f);
        }
        // ---------------------------------------------------------------- distorsioni (motori del nostro Alter Drive)
        {
            struct D { int m; const char* name; const char* insp; Look lk; };
            const D ds[] = {
                { ja::OVERDRIVE, "OVERDRIVE 808", "Ibanez Tube Screamer", look (0xff2f8a4a, 0xfff2f2e8, 0xffff3b2f, 2, 0, true) },
                { ja::DISTORSIONE, "RAT", "ProCo RAT", look (0xff1a1a1a, 0xffffffff, 0xffff2d2d, 2, 0, true) },
                { ja::FUZZ, "FUZZ FACE", "Dallas Arbiter Fuzz Face", look (0xffc23030, 0xfff7f0e0, 0xffffffff, 2, 0, true) },
                { ja::OTTAVA, "OCTAVIA", "Tycobrahe Octavia", look (0xff6b5aa8, 0xfff2eeff, 0xffffd23f, 2, 0, true) },
                { ja::AMP, "AMP CRUNCH", "testata britannica + 4x12", look (0xff2a2420, 0xffd9b45c, 0xffd9a441, 2, 0, true) },
                { ja::FOLD, "WAVEFOLDER", "folder da modulare", look (0xff0f2a2a, 0xffc9fff4, 0xff3dffd2, 1, 0, true) },
                { ja::LOFI, "LO-FI", "campionatore 12 bit", look (0xff8a8f99, 0xff121212, 0xffff6b3d, 2, 0, false) },
                { ja::RADIO, "RADIO", "radiolina / megafono", look (0xffd9c9a3, 0xff2b2015, 0xffb5402b, 2, 0, false) },
                { ja::CLIPPER, "HARD CLIP", "clipper a diodi", look (0xff202020, 0xffeeeeee, 0xffff9f1a, 2, 0, true) } };
            for (auto& x : ds)
            {
                const auto& I = ja::modelInfo (x.m);
                ModDef d; d.name = x.name; d.inspired = x.insp; d.desc = I.what; d.cat = C_DIST; d.kind = K_DIST; d.sub = x.m; d.look = x.lk;
                d.spec = { S ("DRIVE", 0, 1, 0.4f, F_PCT), S ("TONE", -1, 1, 0, F_BIP), S ("LOW CUT", 10, 800, 10, F_HZ), S ("HIGH CUT", 1500, 22000, 22000, F_HZ), S ("OUTPUT", -24, 12, 0, F_DB), S ("AUTO", 0, 1, 1, F_BOOL) };
                d.spec.resize (kP); add (d);
            }
        }
        // ---------------------------------------------------------------- modulazione (motori del nostro Motion)
        {
            struct M { int fx; const char* name; const char* insp; Look lk; float mix; };
            const M ms[] = {
                { jmo::DOUBLER, "DOUBLER", "doppia take (ADT)", look (0xff2b2f3a, 0xffffe6d2, 0xffff7a1a, 2, 0, true), 0.45f },
                { jmo::TRIVOX, "TRIVOX", "micro pitch a tre voci", look (0xff2b2a1c, 0xfffff4c2, 0xffffd23f, 2, 0, true), 0.4f },
                { jmo::ENSEMBLE, "ENSEMBLE", "ensemble a sei voci", look (0xff2a2236, 0xfff1e6ff, 0xffc77dff, 2, 0, true), 0.5f },
                { jmo::CHORUS, "CHORUS CE-1", "Boss CE-1", look (0xff4a7aa8, 0xffffffff, 0xff3dff8b, 2, 0, true), 0.5f },
                { jmo::DIMENSION, "DIMENSION D", "Roland SDD-320", look (0xff1a1d22, 0xffd9f6ff, 0xff5ef2ff, 1, 0, true), 1.0f },
                { jmo::FLANGER, "FLANGER", "flanger a nastro / BBD", look (0xff2d3640, 0xffd9f9ff, 0xff19e3ff, 2, 0, true), 0.5f },
                { jmo::PHASER, "PHASER 90", "MXR Phase 90", look (0xffe8742c, 0xff1e1209, 0xff1e1209, 2, 0, false), 0.5f },
                { jmo::VIBRATO, "VIBRATO", "vibrato a BBD", look (0xff27352a, 0xffe2ffd9, 0xff7dff5a, 2, 0, true), 1.0f } };
            for (auto& x : ms)
            {
                const auto& I = jmo::fxInfo (x.fx);
                ModDef d; d.name = x.name; d.inspired = x.insp; d.desc = I.what; d.cat = C_MOD; d.kind = K_MOD; d.sub = x.fx; d.look = x.lk; d.defMix = x.mix;
                const bool voice = jmo::isVoiceFx (x.fx);
                d.spec = { (I.ctl & jmo::C_RATE) ? S (I.rateName, 0.05f, 10, I.dRate, F_HZ) : H(), (I.ctl & jmo::C_DEPTH) ? S (I.depthName, 0, 1, I.dDepth, F_PCT) : H(),
                           (I.ctl & jmo::C_CHAR) ? S (I.charName, 0, 1, I.dChar, F_PCT) : H(), (I.ctl & jmo::C_FB) ? S (I.fbName, -0.95f, 0.95f, I.dFb, F_PCT) : H(),
                           (I.ctl & jmo::C_FREQ) ? S (I.freqName, 100, 10000, I.dFreq, F_HZ) : H(), (I.ctl & jmo::C_STEREO) ? S ("STEREO", 0, 180, 90, F_NUM) : H(),
                           voice ? S ("VOCE PRINCIPALE", -30, 6, 0, F_DB) : H() };
                d.spec.resize (kP); add (d);
            }
        }
        // ---------------------------------------------------------------- wider, nastro, dinamica, utility
        {
            ModDef w; w.name = "WIDER"; w.inspired = "allargatore M/S"; w.cat = C_WIDE; w.kind = K_WIDE;
            w.desc = "Allarga fino al 200% solo sopra la frequenza scelta: i bassi restano al centro e in mono, gli alti si aprono. HAAS aggiunge larghezza anche ai suoni mono.";
            w.look = look (0xff15243a, 0xffd2e8ff, 0xff6ec1ff, 1, 0, true);
            w.spec = { S ("WIDTH", 0, 2, 1, F_PCT), S ("DA", 20, 20000, 200, F_HZ), S ("MONO SOTTO", 0, 1, 0, F_BOOL), S ("HAAS", 0, 1, 0, F_PCT), S ("OUTPUT", -12, 12, 0, F_DB) };
            w.spec.resize (kP); add (w);
            ModDef t; t.name = "TAPE MACHINE"; t.inspired = "Studer A800 / Ampex ATR"; t.cat = C_TAPE; t.kind = K_TAPE;
            t.desc = "Registratore a nastro: head bump sui bassi, alti arrotondati, saturazione musicale. 30 IPS trasparente, 15 IPS caldo, cassetta lo-fi.";
            t.look = look (0xffc4bfb2, 0xff1c1a17, 0xffd8452f, 2, 2, false);
            t.spec = { S ("MACCHINA", 0, 2, 1, F_ITEMS, { "30 IPS", "15 IPS", "CASSETTA" }), S ("DRIVE", 0, 1, 0.3f, F_PCT), S ("BIAS", -1, 1, 0, F_BIP), S ("FRUSCIO", 0, 1, 0, F_PCT) };
            t.spec.resize (kP); add (t);
            ModDef g; g.name = "GATE"; g.inspired = "Drawmer DS201"; g.cat = C_DYN; g.kind = K_GATE;
            g.desc = "Gate con isteresi e hold: chiude rumori, rientri e code tra un colpo e l'altro. RANGE quanto chiude (poco = expander). Filtri sul rilevatore per aprirlo solo col colpo giusto.";
            g.look = look (0xff1c1c1c, 0xffe8e8e8, 0xff3dff6e, 2, 3, true);
            g.spec = { S ("THRESH", -80, 0, -40, F_DB), S ("RANGE", 0, 80, 40, F_DB), S ("ATTACK", 0.05f, 50, 1, F_MS), S ("HOLD", 1, 500, 20, F_MS), S ("RELEASE", 5, 2000, 120, F_MS),
                       S ("FILTRO BASSI", 20, 2000, 20, F_HZ), S ("FILTRO ALTI", 500, 20000, 20000, F_HZ), S ("ASCOLTA", 0, 1, 0, F_BOOL), S ("SIDECHAIN", 0, 1, 0, F_ITEMS, { "INTERNO", "ESTERNO" }) };
            g.spec.resize (kP); add (g);
            ModDef tr; tr.name = "TRANSIENT"; tr.inspired = "SPL Transient Designer"; tr.cat = C_DYN; tr.kind = K_TRANS;
            tr.desc = "Dosa l'attacco e la coda indipendentemente dal volume: cassa che schiocca, rullante corto, 808 piu lungo.";
            tr.look = look (0xff2e3436, 0xffeaf2f2, 0xffffd23f, 1, 3, true);
            tr.spec = { S ("ATTACK", -100, 100, 0, F_BIP), S ("SUSTAIN", -100, 100, 0, F_BIP), S ("VELOCITA", 0, 1, 0.5f, F_PCT), S ("OUTPUT", -12, 12, 0, F_DB), S ("CLIP", 0, 1, 0, F_BOOL) };
            tr.spec.resize (kP); add (tr);
            ModDef u; u.name = "UTILITY"; u.inspired = "utility"; u.cat = C_UTIL; u.kind = K_UTIL;
            u.desc = "Volume, pan, fase per lato, mono, scambio dei canali e filtri: la cosa da mettere per prima per sistemare il gain staging.";
            u.look = look (0xff3a3d42, 0xffeeeeee, 0xffaab4c0, 1, 2, true);
            u.spec = { S ("GAIN", -24, 24, 0, F_DB), S ("PAN", -1, 1, 0, F_BIP), S ("FASE L", 0, 1, 0, F_BOOL), S ("FASE R", 0, 1, 0, F_BOOL), S ("MONO", 0, 1, 0, F_BOOL),
                       S ("SCAMBIA", 0, 1, 0, F_BOOL), S ("LOW CUT", 10, 1000, 10, F_HZ), S ("HIGH CUT", 1000, 22000, 22000, F_HZ) };
            u.spec.resize (kP); add (u);
        }
        return v;
    }();
    return all;
}
int findMod (const juce::String& name) { for (int i = 0; i < (int) mods().size(); ++i) if (mods()[(size_t) i].name == name) return i; jassertfalse; return 0; }

juce::String paramText (int type, int i, float v)
{
    const auto& d = mod (type); const auto& s = d.spec[(size_t) i]; const float r = specReal (s, v);
    switch (s.fmt)
    {
        case F_DB: return (r > 0.05f ? "+" : "") + juce::String (r, 1) + " dB";
        case F_PCT: return juce::String (juce::roundToInt (r * 100)) + "%";
        case F_BOOL: return r > 0.5f ? "ON" : "OFF";
        case F_ITEMS: { const auto it = s.items[(int) r]; return it; }
        case F_HZ: { if (s.label != nullptr && juce::String (s.label).contains ("CUT") && ((s.lo <= 10.5f && r <= 10.6f) || r >= 21900)) return "OFF"; return d.kind == K_MOD && s.hi <= 10.5f ? juce::String (r, r < 1 ? 2 : 1) + " Hz" : hzText (r); }
        case F_MS: return msText (r);
        case F_BIP: return (r > 0.005f ? "+" : "") + juce::String (juce::roundToInt (r * (std::abs (s.hi) > 1.5f ? 1.0f : 100.0f)));
        case F_CUSTOM: return d.custom ? d.custom (i, v) : juce::String (r, 2);
        case F_AEF: return jpre::specText (d.sub, i, v);
        default: return juce::String (r, std::abs (r) < 10 ? 1 : 0);
    }
}

// ======================================================================= motori
namespace
{
// rimette il volume a pari: confronta l'energia prima e dopo (lento, non pompa)
struct AutoLevel
{
    double pin = 1e-6, pout = 1e-6; float g = 1; float a = 0.9995f;
    void prepare (double sr) { a = (float) std::exp (-1.0 / (0.4 * sr)); pin = pout = 1e-6; g = 1; }
    void measureIn (const float* L, const float* R, int n) { for (int i = 0; i < n; ++i) pin = pin * a + (1 - a) * (double) (L[i] * L[i] + R[i] * R[i]); }
    void apply (float* L, float* R, int n, bool on)
    {
        for (int i = 0; i < n; ++i)
        {
            pout = pout * a + (1 - a) * (double) (L[i] * L[i] + R[i] * R[i]);
            const float t = on ? (float) juce::jlimit (0.06, 4.0, std::sqrt (pin / juce::jmax (1e-9, pout))) : 1.0f;
            g += (t - g) * 0.0005f; L[i] *= g; R[i] *= g;
        }
    }
};
inline void peakOut (Engine& e, const float* L, const float* R, int n) { float p = 0; for (int i = 0; i < n; ++i) p = juce::jmax (p, std::abs (L[i]), std::abs (R[i])); e.out = juce::jmax (p, e.out.load() * 0.9f); }

class SlotEng : public Engine   // preamp ed EQ di colore: lo slot del vecchio Preamp EQ
{
public:
    void prepare (double s, int mb) override { sl.prepare (s, mb); }
    int latency (const float*) const override { return sl.latency (mod (type).sub); }
    void process (float* L, float* R, int n, const float* v, const Ctx&) override
    {
        jpre::SlotSet S; S.type = mod (type).sub; for (int i = 0; i < jpre::kSlotParams; ++i) S.v[i] = v[i];
        sl.process (L, R, n, S); peakOut (*this, L, R, n);
    }
    jpre::Slot sl;
};
class DsEng : public Engine
{
public:
    void prepare (double s, int) override { ds.prepare (s); }
    DsSettings set (const float* v) const
    {
        DsSettings S;
        if (mod (type).sub == 0) { S.freq = rv (v, 0); S.thr = rv (v, 1); S.range = rv (v, 2); S.wide = rv (v, 3) > 0.5f; S.listen = rv (v, 4) > 0.5f; S.relative = false; S.hiLim = 20000; }
        else { S.thr = rv (v, 0); S.range = rv (v, 1); S.freq = rv (v, 2); S.hiLim = rv (v, 3); S.wide = rv (v, 4) > 0.5f; S.look = rv (v, 5) > 0.5f; S.listen = rv (v, 6) > 0.5f; S.relative = true; }
        return S;
    }
    int latency (const float* v) const override { return ds.latency (set (v)); }
    void process (float* L, float* R, int n, const float* v, const Ctx&) override { ds.process (L, R, n, set (v)); gr = ds.grUi; peakOut (*this, L, R, n); }
    DeEsser ds;
};
// saturazioni e distorsioni con i modelli dell'Alter Drive, sovracampionate 4x
class DriveEng : public Engine
{
public:
    void prepare (double s, int mb) override
    {
        sr = s; os = std::make_unique<juce::dsp::Oversampling<float>> (2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false); os->initProcessing ((size_t) mb);
        for (auto& c : ch) c.prepare (sr * 4, 4); al.prepare (sr); for (auto& f : flt) f.reset(); last = -99;
    }
    int latency (const float*) const override { return os ? juce::roundToInt (os->getLatencyInSamples()) : 0; }
    void process (float* L, float* R, int n, const float* v, const Ctx&) override
    {
        const bool decap = mod (type).kind == K_DECAP;
        int model = mod (type).sub; float drive = rv (v, 0), tone = rv (v, decap ? 2 : 1), lo = rv (v, decap ? 3 : 2), hi = rv (v, decap ? 4 : 3), outDb = rv (v, decap ? 6 : 4); bool autoL = rv (v, decap ? 7 : 5) > 0.5f;
        float boost = 0;
        if (decap) { static const int st[] = { ja::TAPE, ja::TRASFORMATORE, ja::CONSOLE, ja::VALVOLA, ja::CLIPPER }; model = st[juce::jlimit (0, 4, (int) rv (v, 1))]; if (rv (v, 5) > 0.5f) boost = 20; }
        for (auto& c : ch) { c.setModel (model); c.setDrive (drive); }
        const float key = lo * 3 + hi * 7 + tone * 1000;
        if (key != last)
        {
            last = key;
            for (int c = 0; c < 2; ++c) { flt[c * 4].hp (sr, juce::jmax (10.0f, lo), 0.7071); flt[c * 4 + 1].lp (sr, juce::jmin ((double) hi, sr * 0.45), 0.7071); flt[c * 4 + 2].lowShelf (sr, 700, -tone * 6, 0.6); flt[c * 4 + 3].highShelf (sr, 1500, tone * 6, 0.6); }
        }
        al.measureIn (L, R, n);
        float* chp[2] = { L, R };
        for (int c = 0; c < 2; ++c) if (lo > 10.5f) for (int i = 0; i < n; ++i) chp[c][i] = flt[c * 4].p (chp[c][i]);
        juce::dsp::AudioBlock<float> blk (chp, 2, (size_t) n);
        auto up = os->processSamplesUp (blk);
        const float g = juce::Decibels::decibelsToGain (drive * ja::modelInfo (model).maxDb + boost);
        for (int c = 0; c < 2; ++c) { float* d = up.getChannelPointer ((size_t) c); for (int i = 0; i < (int) up.getNumSamples(); ++i) d[i] = ch[c].process (d[i], g, drive); }
        os->processSamplesDown (blk);
        for (int c = 0; c < 2; ++c) for (int i = 0; i < n; ++i) { float y = chp[c][i]; if (hi < 21900) y = flt[c * 4 + 1].p (y); if (std::abs (tone) > 0.01f) y = flt[c * 4 + 3].p (flt[c * 4 + 2].p (y)); chp[c][i] = y; }
        al.apply (L, R, n, autoL);
        const float og = juce::Decibels::decibelsToGain (outDb); for (int i = 0; i < n; ++i) { L[i] *= og; R[i] *= og; }
        peakOut (*this, L, R, n);
    }
    double sr = 44100; std::unique_ptr<juce::dsp::Oversampling<float>> os; ja::DriveChannel ch[2]; AutoLevel al; jkit::Bq flt[8]; float last = -99;
};
class TubeEng : public Engine
{
public:
    void prepare (double s, int mb) override { e.prepare (s, mb); }
    int latency (const float*) const override { return e.latency(); }
    void process (float* L, float* R, int n, const float* v, const Ctx&) override
    {
        jtbx::Settings S; S.hgOn = true; S.inDb = rv (v, 0); S.pentode = rv (v, 1); S.triode = rv (v, 2); S.density = rv (v, 3); S.sat = rv (v, 4); S.satIn = S.sat > 0.01f; S.satFreq = (int) rv (v, 5);
        S.air = rv (v, 6); S.airIn = S.air > 0.01f; S.calib = (int) rv (v, 7); S.outDb = rv (v, 8); S.mix = 1;
        e.process (L, R, n, S); peakOut (*this, L, R, n);
    }
    jtbx::Engine e;
};
class MbSatEng : public Engine
{
public:
    void prepare (double s, int) override { e.prepare (s); }
    void process (float* L, float* R, int n, const float* v, const Ctx&) override
    {
        jms::ExSettings S; S.x1 = rv (v, 6); S.x2 = juce::jmax (rv (v, 7), S.x1 * 2); S.mix = 1;
        for (int b = 0; b < 3; ++b) { S.amt[b] = rv (v, b); S.mode[b] = (int) rv (v, 3 + b); }
        for (int off = 0; off < n; off += 512) e.process (L + off, R + off, juce::jmin (512, n - off), S);
        const float og = juce::Decibels::decibelsToGain (rv (v, 8)); for (int i = 0; i < n; ++i) { L[i] *= og; R[i] *= og; }
        peakOut (*this, L, R, n);
    }
    jms::Exciter e;
};
class CompEng : public Engine
{
public:
    void prepare (double s, int mb) override { st.prepare (s); dL.assign ((size_t) mb, 0); dR = dL; }
    void process (float* L, float* R, int n, const float* v, const Ctx& c) override
    {
        jc::StageParams p; p.on = true; p.model = mod (type).sub; p.drive = v[0]; p.ratio = (int) rv (v, 1); p.att = v[2]; p.rel = v[3]; p.color = rv (v, 4); p.outDb = rv (v, 5); p.mix = 1;
        const bool ext = rv (v, 7) > 0.5f && c.scL != nullptr;
        for (int i = 0; i < n; ++i) { dL[(size_t) i] = ext ? c.scL[i] : L[i]; dR[(size_t) i] = ext ? c.scR[i] : R[i]; }
        st.process (L, R, dL.data(), dR.data(), n, p, rv (v, 6) > 0.5f);
        gr = -std::abs (st.grNow()); peakOut (*this, L, R, n);
    }
    jc::Stage st; std::vector<float> dL, dR;
};
class ClipEng : public Engine
{
public:
    void prepare (double s, int mb) override { e.prepare (s, mb); }
    int latency (const float* v) const override { return e.latency ((int) rv (v, 4)); }
    void process (float* L, float* R, int n, const float* v, const Ctx&) override
    {
        jclp::Settings S; S.mode = (int) rv (v, 0); S.driveDb = rv (v, 1); S.ceilDb = rv (v, 2); S.punch = rv (v, 3); S.os = (int) rv (v, 4); S.outDb = rv (v, 5); S.link = rv (v, 6) > 0.5f; S.mix = 1;
        float pin = 0; for (int i = 0; i < n; ++i) pin = juce::jmax (pin, std::abs (L[i]), std::abs (R[i]));
        e.process (L, R, n, S, h);
        const float ceil = juce::Decibels::decibelsToGain (S.ceilDb), pd = pin * juce::Decibels::decibelsToGain (S.driveDb);
        gr = pd > ceil ? -g2db (pd / ceil) : 0.0f; peakOut (*this, L, R, n);
    }
    jclp::Engine e; jclp::History h;
};
class LimEng : public Engine
{
public:
    void prepare (double s, int mb) override { e.prepare (s, mb); }
    int latency (const float*) const override { return e.latency(); }
    void process (float* L, float* R, int n, const float* v, const Ctx&) override
    {
        jms::MaxSettings S; S.style = (int) rv (v, 0); S.gainDb = rv (v, 1); S.ceilDb = rv (v, 2); S.clip = rv (v, 3); S.character = rv (v, 4);
        float pin = 0; for (int i = 0; i < n; ++i) pin = juce::jmax (pin, std::abs (L[i]), std::abs (R[i]));
        e.process (L, R, n, S);
        const float pd = pin * juce::Decibels::decibelsToGain (S.gainDb), ceil = juce::Decibels::decibelsToGain (S.ceilDb);
        const float t = pd > ceil ? -g2db (pd / ceil) : 0.0f; gr = juce::jmin (t, gr.load() * 0.85f); peakOut (*this, L, R, n);
    }
    jms::Maximizer e;
};
class RevEng : public Engine
{
public:
    void prepare (double s, int) override { r.prepare (s); }
    void process (float* L, float* R, int n, const float* v, const Ctx&) override
    {
        RevSettings S; S.kind = mod (type).sub; S.preMs = v[0] * 250; S.decay = rv (v, 1) * 0.001f; S.size = rv (v, 2); S.damp = rv (v, 3); S.lowMul = 0.5f + v[4] * 1.5f; S.diff = rv (v, 5); S.mod = rv (v, 6);
        S.er = mod (type).spec[7].label ? rv (v, 7) : 0.0f; S.loCut = rv (v, 8); S.hiCut = rv (v, 9); S.width = rv (v, 10); S.duck = rv (v, 11); S.shimmer = mod (type).spec[12].label ? rv (v, 12) : 0.0f;
        r.process (L, R, n, S); peakOut (*this, L, R, n);
    }
    Reverb r;
};
class SpringEng : public Engine
{
public:
    void prepare (double s, int) override { sp.prepare (s); }
    void process (float* L, float* R, int n, const float* v, const Ctx&) override
    {
        SpringSettings S; S.decay = rv (v, 0) * 0.001f; S.tension = rv (v, 1); S.tone = rv (v, 2); S.drip = rv (v, 3); S.loCut = rv (v, 4); S.hiCut = rv (v, 5); S.width = rv (v, 6);
        sp.process (L, R, n, S); peakOut (*this, L, R, n);
    }
    Spring sp;
};
class ConvEng : public Engine
{
public:
    void prepare (double s, int mb) override { sr = s; conv.prepare ({ s, (juce::uint32) mb, 2 }); conv.reset(); lastIr = -1; lastDec = -1; for (auto& f : flt) f.reset(); lastF = -1; }
    int latency (const float*) const override { return conv.getLatency(); }
    void process (float* L, float* R, int n, const float* v, const Ctx&) override
    {
        const int ir = (int) rv (v, 0); const float dec = 0.5f + v[1] * 1.5f;
        if (ir != lastIr || std::abs (dec - lastDec) > 0.02f)
        {
            lastIr = ir; lastDec = dec;
            if (ir < 9) conv.loadImpulseResponse (makeIr (ir, sr, dec), sr, juce::dsp::Convolution::Stereo::yes, juce::dsp::Convolution::Trim::no, juce::dsp::Convolution::Normalise::no);
            else if (userIr.getNumSamples() > 0) { auto b = userIr; conv.loadImpulseResponse (std::move (b), userSr, juce::dsp::Convolution::Stereo::yes, juce::dsp::Convolution::Trim::yes, juce::dsp::Convolution::Normalise::yes); }
        }
        float* ch[2] = { L, R }; juce::dsp::AudioBlock<float> blk (ch, 2, (size_t) n);
        conv.process (juce::dsp::ProcessContextReplacing<float> (blk));
        const float lo = rv (v, 2), hi = rv (v, 3), wd = rv (v, 4);
        if (lo * 3 + hi != lastF) { lastF = lo * 3 + hi; for (int c = 0; c < 2; ++c) { flt[c * 2].hp (sr, lo, 0.7071); flt[c * 2 + 1].lp (sr, juce::jmin ((double) hi, sr * 0.45), 0.7071); } }
        for (int i = 0; i < n; ++i)
        {
            float l = flt[1].p (flt[0].p (L[i])), r = flt[3].p (flt[2].p (R[i]));
            const float m = 0.5f * (l + r), sd = 0.5f * (l - r) * wd; L[i] = m + sd; R[i] = m - sd;
        }
        peakOut (*this, L, R, n);
    }
    void setUserIr (juce::AudioBuffer<float> b, double s) override { userIr = std::move (b); userSr = s; lastIr = -1; }
    juce::dsp::Convolution conv; double sr = 44100, userSr = 44100; juce::AudioBuffer<float> userIr; int lastIr = -1; float lastDec = -1, lastF = -1; jkit::Bq flt[4];
};
class DlyEng : public Engine
{
public:
    void prepare (double s, int) override { d.prepare (s); }
    void process (float* L, float* R, int n, const float* v, const Ctx& c) override
    {
        DlySettings S; S.kind = mod (type).sub; S.sync = rv (v, 0) > 0.5f; S.beats = divBeats ((int) rv (v, 1)); S.ms = rv (v, 2); S.fb = rv (v, 3); S.loCut = rv (v, 4); S.hiCut = rv (v, 5);
        S.drive = rv (v, 6); S.mod = rv (v, 7); S.width = rv (v, 8); S.duck = rv (v, 9); S.spread = mod (type).spec[10].label ? rv (v, 10) : 0.0f;
        d.process (L, R, n, S, c.bpm); peakOut (*this, L, R, n);
    }
    Delay d;
};
class ModEng : public Engine
{
public:
    void prepare (double s, int) override { u.type = mod (type).sub; u.prepare (s); }
    void process (float* L, float* R, int n, const float* v, const Ctx& c) override
    {
        const auto& sp = mod (type).spec; const auto& I = jmo::fxInfo (mod (type).sub);
        jmo::FxSet S; S.on = true; S.sync = false; S.shape = jmo::SINE; S.mix = 1;
        S.rateHz = sp[0].label ? rv (v, 0) : I.dRate; S.depth = sp[1].label ? rv (v, 1) : I.dDepth; S.character = sp[2].label ? rv (v, 2) : I.dChar; S.fb = sp[3].label ? rv (v, 3) : I.dFb;
        S.freq = sp[4].label ? rv (v, 4) : I.dFreq; S.stereo = sp[5].label ? rv (v, 5) : 90.0f; S.dryDb = sp[6].label ? rv (v, 6) : 0.0f;
        u.process (L, R, n, S, c.ppq, c.bpm); peakOut (*this, L, R, n);
    }
    jmo::Unit u;
};
class WideEng : public Engine
{
public:
    void prepare (double s, int) override { w.prepare (s); }
    void process (float* L, float* R, int n, const float* v, const Ctx&) override
    { WideSettings S; S.width = rv (v, 0); S.fromHz = rv (v, 1); S.monoBass = rv (v, 2) > 0.5f; S.haas = rv (v, 3); S.outDb = rv (v, 4); w.process (L, R, n, S); peakOut (*this, L, R, n); }
    Wider w;
};
class TapeEng : public Engine
{
public:
    void prepare (double s, int) override { t.prepare (s); }
    void process (float* L, float* R, int n, const float* v, const Ctx&) override
    { jms::TapeSettings S; S.model = (int) rv (v, 0); S.drive = rv (v, 1); S.bias = rv (v, 2); S.noise = rv (v, 3); S.mix = 1; t.process (L, R, n, S); peakOut (*this, L, R, n); }
    jms::TapeMachine t;
};
class GateEng : public Engine
{
public:
    void prepare (double s, int) override { g.prepare (s); }
    void process (float* L, float* R, int n, const float* v, const Ctx& c) override
    {
        GateSettings S; S.thr = rv (v, 0); S.range = rv (v, 1); S.attMs = rv (v, 2); S.holdMs = rv (v, 3); S.relMs = rv (v, 4); S.hpHz = rv (v, 5); S.lpHz = rv (v, 6); S.listen = rv (v, 7) > 0.5f;
        const bool ext = rv (v, 8) > 0.5f && c.scL != nullptr;
        g.process (L, R, n, ext ? c.scL : L, ext ? c.scR : R, S); gr = g.grUi; peakOut (*this, L, R, n);
    }
    Gate g;
};
class TransEng : public Engine
{
public:
    void prepare (double s, int) override { t.prepare (s); }
    void process (float* L, float* R, int n, const float* v, const Ctx&) override
    { TrSettings S; S.attack = rv (v, 0); S.sustain = rv (v, 1); S.speed = rv (v, 2); S.outDb = rv (v, 3); S.clip = rv (v, 4) > 0.5f; t.process (L, R, n, S); gr = t.actUi; peakOut (*this, L, R, n); }
    Transient t;
};
class UtilEng : public Engine
{
public:
    void prepare (double s, int) override { u.prepare (s); }
    void process (float* L, float* R, int n, const float* v, const Ctx&) override
    {
        UtilSettings S; S.gainDb = rv (v, 0); S.pan = rv (v, 1); S.phL = rv (v, 2) > 0.5f; S.phR = rv (v, 3) > 0.5f; S.mono = rv (v, 4) > 0.5f; S.swap = rv (v, 5) > 0.5f; S.loHz = rv (v, 6); S.hiHz = rv (v, 7);
        u.process (L, R, n, S); peakOut (*this, L, R, n);
    }
    Utility u;
};
}

std::unique_ptr<Engine> makeEngine (int t)
{
    std::unique_ptr<Engine> e;
    switch (mod (t).kind)
    {
        case K_PRE: case K_CEQ: e = std::make_unique<SlotEng>(); break;
        case K_DS: e = std::make_unique<DsEng>(); break;
        case K_DECAP: case K_DIST: e = std::make_unique<DriveEng>(); break;
        case K_TUBE: e = std::make_unique<TubeEng>(); break;
        case K_MBSAT: e = std::make_unique<MbSatEng>(); break;
        case K_COMP: e = std::make_unique<CompEng>(); break;
        case K_CLIP: e = std::make_unique<ClipEng>(); break;
        case K_LIM: e = std::make_unique<LimEng>(); break;
        case K_REV: e = std::make_unique<RevEng>(); break;
        case K_SPRING: e = std::make_unique<SpringEng>(); break;
        case K_CONV: e = std::make_unique<ConvEng>(); break;
        case K_DLY: e = std::make_unique<DlyEng>(); break;
        case K_MOD: e = std::make_unique<ModEng>(); break;
        case K_WIDE: e = std::make_unique<WideEng>(); break;
        case K_TAPE: e = std::make_unique<TapeEng>(); break;
        case K_GATE: e = std::make_unique<GateEng>(); break;
        case K_TRANS: e = std::make_unique<TransEng>(); break;
        case K_UTIL: e = std::make_unique<UtilEng>(); break;
        default: return nullptr;
    }
    e->type = t; return e;
}
}
