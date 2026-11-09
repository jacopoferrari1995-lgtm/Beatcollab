#include "PadProcessor.h"

// Kit di fabbrica: suoni sintetizzati + catena per pad pensata per lo stile.
// Ordine MPC: pad 1-4 riga in basso. Parti melodiche dentro la tonalità indicata nel nome.
namespace jp
{
namespace
{
using PV = std::vector<std::pair<int, float>>;
KitPad K (int type, float a, float b, float c, float d, float e, const char* name, int col, PV p = {}) { return { { type, a, b, c, d, e, name }, std::move (p), col }; }
enum { MAG = 0, ORA = 1, AMB = 2, YEL = 3, LIM = 4, MIN = 5, CYA = 6, BLU = 7, VIO = 8, FUC = 9, PNK = 10, MAN = 11, AQU = 12, ACI = 13, RED = 14, LAV = 15 };
}

const std::vector<FactoryKit>& factoryKits()
{
    static const std::vector<FactoryKit> kits = {
    { "Boom Bap Classico", "Hip Hop 85-95", {
        K (S_KICK, 120, 52, 0.45f, 0.5f, 0.35f, "Kick Dusty", MAG, { { LOWG, 3 }, { LOWF, 70 }, { CMODE, 2 }, { CAMT, 0.35f }, { BITS, 0.15f } }),
        K (S_SNARE, 195, 0.22f, 0.12f, 0.6f, 0.6f, "Snare Crack", ORA, { { CMODE, 3 }, { CAMT, 0.3f }, { MIDG, 2 }, { MIDF, 2200 }, { SREV, 0.12f } }),
        K (S_RIM, 1650, 0.05f, 0, 0, 0, "Rim Shot", LIM, { { LEVEL, 0.7f } }),
        K (S_HAT, 0.06f, 7.5f, 0.5f, 0, 0, "Hat Chiuso", CYA, { { LEVEL, 0.62f }, { CHOKE, 1 }, { BITS, 0.2f } }),
        K (S_HAT, 0.35f, 7.5f, 0.5f, 0, 0, "Hat Aperto", AQU, { { LEVEL, 0.55f }, { CHOKE, 1 }, { BITS, 0.2f } }),
        K (S_SNARE, 220, 0.12f, 0.06f, 0.8f, 0.3f, "Snare Ghost", ORA, { { LEVEL, 0.5f }, { VLEVEL, 0.9f } }),
        K (S_KICK, 100, 48, 0.35f, 0.2f, 0.1f, "Kick Soft", MAG, { { LEVEL, 0.8f } }),
        K (S_SHAKER, 0.12f, 6, 0, 0, 0, "Shaker", MIN, { { LEVEL, 0.5f } }),
        K (S_TOM, 95, 0.4f, 0.4f, 0, 0, "Tom Basso", BLU),
        K (S_TOM, 160, 0.3f, 0.4f, 0, 0, "Tom Alto", BLU),
        K (S_CYMBAL, 1.8f, 0.4f, 0, 0, 0, "Crash", YEL, { { LEVEL, 0.5f }, { BITS, 0.25f } }),
        K (S_CYMBAL, 1.2f, 0.2f, 0.6f, 0, 0, "Ride", YEL, { { LEVEL, 0.42f } }),
        K (S_BELL, 560, 845, 0.2f, 0, 0, "Cowbell", LIM, { { LEVEL, 0.55f } }),
        K (S_SNAP, 0.08f, 2200, 0, 0, 0, "Snap", PNK, { { SREV, 0.2f } }),
        K (S_STAB, 57, 0, 0.6f, 0.4f, 0, "Stab Am7", FUC, { { LEVEL, 0.7f }, { SREV, 0.25f }, { BITS, 0.2f }, { RATE, 0.25f } }),
        K (S_STAB, 50, 2, 0.7f, 0.35f, 0, "Stab Dm9", FUC, { { LEVEL, 0.7f }, { SREV, 0.25f }, { BITS, 0.2f }, { RATE, 0.25f } }) } },

    { "Dusty Lo-fi", "Lo-fi 70-85", {
        K (S_KICK, 95, 46, 0.4f, 0.15f, 0.2f, "Kick Cotone", MAG, { { CUTOFF, 5000 }, { BITS, 0.35f }, { RATE, 0.3f }, { CMODE, 1 }, { CAMT, 0.4f } }),
        K (S_SNARE, 180, 0.3f, 0.08f, 0.3f, 0.3f, "Snare Spazzola", ORA, { { CUTOFF, 7000 }, { BITS, 0.3f }, { SREV, 0.25f } }),
        K (S_RIM, 1450, 0.06f, 0, 0, 0, "Rim Legno", LIM, { { CUTOFF, 6000 }, { LEVEL, 0.65f } }),
        K (S_HAT, 0.05f, 6, 0.3f, 0, 0, "Hat Polveroso", CYA, { { CUTOFF, 9000 }, { BITS, 0.35f }, { CHOKE, 1 }, { LEVEL, 0.55f } }),
        K (S_HAT, 0.3f, 6, 0.3f, 0, 0, "Hat Aperto", AQU, { { CUTOFF, 8000 }, { BITS, 0.35f }, { CHOKE, 1 }, { LEVEL, 0.45f } }),
        K (S_SNAP, 0.1f, 1800, 0, 0, 0, "Snap Morbido", PNK, { { SREV, 0.35f } }),
        K (S_SHAKER, 0.16f, 5, 0, 0, 0, "Shaker", MIN, { { LEVEL, 0.45f }, { CUTOFF, 8000 } }),
        K (S_NOISE, 4.0f, 4, 0.6f, 0, 0, "Vinile", LAV, { { LOOP, 1 }, { LEVEL, 0.5f } }),
        K (S_CONGA, 210, 0.3f, 0, 0, 0, "Conga Bassa", LIM, { { CUTOFF, 5000 } }),
        K (S_CONGA, 300, 0.25f, 0, 0, 0, "Conga Alta", LIM, { { CUTOFF, 5000 } }),
        K (S_808, 55, 0.9f, 0.1f, 0.15f, 0.2f, "Basso A1", VIO, { { CUTOFF, 1800 } }),
        K (S_CYMBAL, 2.0f, 0.6f, 0.3f, 0, 0, "Ride Scuro", YEL, { { CUTOFF, 6000 }, { LEVEL, 0.4f } }),
        K (S_STAB, 53, 1, 1.0f, 0.25f, 0, "Keys Fmaj7", FUC, { { BITS, 0.3f }, { RATE, 0.35f }, { SREV, 0.35f } }),
        K (S_STAB, 48, 4, 1.0f, 0.3f, 0, "Keys Cmaj9", FUC, { { BITS, 0.3f }, { RATE, 0.35f }, { SREV, 0.35f } }),
        K (S_PLUCK, 69, 1.2f, 0.4f, 0, 0, "Pluck A4", PNK, { { SDLY, 0.3f }, { BITS, 0.2f } }),
        K (S_PLUCK, 76, 1.0f, 0.4f, 0, 0, "Pluck E5", PNK, { { SDLY, 0.3f }, { BITS, 0.2f } }) } },

    { "Trap Moderno", "Trap 130-150 (Fa minore)", {
        K (S_KICK, 170, 50, 0.35f, 0.7f, 0.5f, "Kick Punch", MAG, { { CMODE, 3 }, { CAMT, 0.25f }, { LOWG, 2 } }),
        K (S_CLAP, 9, 0.22f, 1300, 0.3f, 0, "Clap", AMB, { { SREV, 0.18f }, { CMODE, 2 }, { CAMT, 0.3f } }),
        K (S_SNARE, 210, 0.18f, 0.08f, 0.9f, 0.4f, "Snare", ORA, { { HIGHG, 3 } }),
        K (S_HAT, 0.04f, 9, 0.6f, 0, 0, "Hat", CYA, { { CHOKE, 1 }, { LEVEL, 0.6f }, { HIGHG, 2 } }),
        K (S_HAT, 0.45f, 9, 0.6f, 0, 0, "Open Hat", AQU, { { CHOKE, 1 }, { LEVEL, 0.5f } }),
        K (S_HAT, 0.02f, 9.5f, 0.6f, 0, 0, "Hat Roll", CYA, { { CHOKE, 1 }, { LEVEL, 0.5f }, { VLEVEL, 0.9f } }),
        K (S_RIM, 2000, 0.04f, 0, 0, 0, "Rim", LIM, { { LEVEL, 0.6f } }),
        K (S_SNAP, 0.07f, 2400, 0, 0, 0, "Snap", PNK, { { SREV, 0.15f } }),
        K (S_808, 43.65f, 1.2f, 0.3f, 0.3f, 0.4f, "808 F1", VIO, { { CMODE, 1 }, { CAMT, 0.3f } }),
        K (S_808, 51.91f, 1.2f, 0.3f, 0.3f, 0.4f, "808 G#1", VIO, { { CMODE, 1 }, { CAMT, 0.3f } }),
        K (S_808, 65.41f, 1.0f, 0.3f, 0.3f, 0.4f, "808 C2", VIO, { { CMODE, 1 }, { CAMT, 0.3f } }),
        K (S_808, 77.78f, 1.0f, 0.3f, 0.3f, 0.4f, "808 D#2", VIO, { { CMODE, 1 }, { CAMT, 0.3f } }),
        K (S_CYMBAL, 2.2f, 0.5f, 0, 0, 0, "Crash", YEL, { { LEVEL, 0.45f } }),
        K (S_CYMBAL, 2.0f, 0.5f, 0, 0, 0, "Crash Reverse", YEL, { { REVERSE, 1 }, { LEVEL, 0.45f } }),
        K (S_PLUCK, 65, 0.9f, 0.8f, 0, 0, "Bell F4", FUC, { { SREV, 0.3f }, { SDLY, 0.25f } }),
        K (S_STAB, 53, 2, 0.8f, 0.5f, 0, "Stab Fm9", FUC, { { SREV, 0.3f } }) } },

    { "Dark Trap 808", "Trap scuro (Do minore)", {
        K (S_KICK, 160, 45, 0.3f, 0.8f, 0.7f, "Kick Distorto", MAG, { { DRIVE, 0.3f }, { CMODE, 3 }, { CAMT, 0.3f } }),
        K (S_CLAP, 12, 0.35f, 1000, 0.6f, 0, "Clap Scuro", AMB, { { SREV, 0.35f }, { CUTOFF, 9000 } }),
        K (S_SNARE, 180, 0.25f, 0.1f, 0.7f, 0.5f, "Snare Pesante", ORA, { { DRIVE, 0.25f }, { SREV, 0.2f } }),
        K (S_HAT, 0.035f, 7, 0.7f, 0, 0, "Hat Scuro", CYA, { { CHOKE, 1 }, { LEVEL, 0.55f }, { CUTOFF, 10000 } }),
        K (S_HAT, 0.5f, 7, 0.7f, 0, 0, "Open Hat Scuro", AQU, { { CHOKE, 1 }, { LEVEL, 0.45f } }),
        K (S_RIM, 1200, 0.05f, 0, 0, 0, "Rim Scuro", LIM, { { SDLY, 0.3f } }),
        K (S_SNAP, 0.1f, 1600, 0, 0, 0, "Snap Riverbero", PNK, { { SREV, 0.5f } }),
        K (S_CYMBAL, 2.4f, 0.7f, 0, 0, 0, "Riser Reverse", YEL, { { REVERSE, 1 }, { LEVEL, 0.45f }, { SREV, 0.4f } }),
        K (S_808, 32.70f, 1.6f, 0.7f, 0.4f, 0.5f, "808 C1", VIO, { { DRIVE, 0.25f } }),
        K (S_808, 38.89f, 1.6f, 0.7f, 0.4f, 0.5f, "808 D#1", VIO, { { DRIVE, 0.25f } }),
        K (S_808, 43.65f, 1.5f, 0.7f, 0.4f, 0.5f, "808 F1", VIO, { { DRIVE, 0.25f } }),
        K (S_808, 49.00f, 1.5f, 0.7f, 0.4f, 0.5f, "808 G1", VIO, { { DRIVE, 0.25f } }),
        K (S_808, 51.91f, 1.4f, 0.7f, 0.4f, 0.5f, "808 G#1", VIO, { { DRIVE, 0.25f } }),
        K (S_808, 58.27f, 1.4f, 0.7f, 0.4f, 0.5f, "808 A#1", VIO, { { DRIVE, 0.25f } }),
        K (S_STAB, 48, 0, 1.2f, 0.3f, 0, "Pad Cm7", FUC, { { SREV, 0.5f }, { ATTACK, 60 } }),
        K (S_PLUCK, 67, 1.0f, 0.6f, 0, 0, "Pluck G4", PNK, { { SDLY, 0.35f }, { SREV, 0.3f } }) } },

    { "UK Drill", "Drill 140-145 (Sol minore)", {
        K (S_KICK, 150, 55, 0.25f, 0.6f, 0.4f, "Kick Drill", MAG, { { CMODE, 3 }, { CAMT, 0.3f } }),
        K (S_SNARE, 260, 0.16f, 0.06f, 1.0f, 0.5f, "Snare Drill", ORA, { { HIGHG, 3 }, { CMODE, 3 }, { CAMT, 0.25f } }),
        K (S_CLAP, 8, 0.15f, 1600, 0.2f, 0, "Clap Stretto", AMB),
        K (S_HAT, 0.03f, 10, 0.7f, 0, 0, "Hat Drill", CYA, { { CHOKE, 1 }, { LEVEL, 0.55f } }),
        K (S_HAT, 0.3f, 10, 0.7f, 0, 0, "Open Hat", AQU, { { CHOKE, 1 }, { LEVEL, 0.45f } }),
        K (S_RIM, 2200, 0.035f, 0, 0, 0, "Rim Drill", LIM),
        K (S_CONGA, 380, 0.15f, 0, 0, 0, "Perc", LIM, { { LEVEL, 0.6f } }),
        K (S_SNARE, 300, 0.08f, 0.04f, 1.0f, 0.4f, "Snare Roll", ORA, { { LEVEL, 0.6f } }),
        K (S_808, 49.00f, 0.6f, 0.5f, 0.6f, 0.5f, "808 G1", VIO),
        K (S_808, 58.27f, 0.6f, 0.5f, 0.6f, 0.5f, "808 A#1", VIO),
        K (S_808, 73.42f, 0.55f, 0.5f, 0.6f, 0.5f, "808 D2", VIO),
        K (S_808, 43.65f, 0.65f, 0.5f, 0.6f, 0.5f, "808 F1", VIO),
        K (S_CYMBAL, 1.8f, 0.5f, 0, 0, 0, "Crash", YEL, { { LEVEL, 0.45f } }),
        K (S_BELL, 700, 1050, 0.25f, 0, 0, "Bell", LIM, { { SDLY, 0.25f } }),
        K (S_STAB, 55, 0, 0.9f, 0.4f, 0, "Stab Gm7", FUC, { { SREV, 0.3f } }),
        K (S_PLUCK, 70, 0.8f, 0.7f, 0, 0, "Pluck A#4", PNK, { { SDLY, 0.3f } }) } },

    { "Old School 808", "Electro / Miami bass", {
        K (S_808, 50, 0.9f, 0.1f, 0.35f, 0.7f, "808 Kick", MAG),
        K (S_SNARE, 240, 0.15f, 0.1f, 0.4f, 0.7f, "808 Snare", ORA),
        K (S_CLAP, 11, 0.18f, 1100, 0.1f, 0, "808 Clap", AMB),
        K (S_HAT, 0.05f, 7, 0.9f, 0, 0, "808 Hat", CYA, { { CHOKE, 1 }, { LEVEL, 0.6f } }),
        K (S_HAT, 0.4f, 7, 0.9f, 0, 0, "808 Open Hat", AQU, { { CHOKE, 1 }, { LEVEL, 0.5f } }),
        K (S_CLAVE, 2500, 0.04f, 0, 0, 0, "Clave", LIM),
        K (S_SHAKER, 0.08f, 7, 0, 0, 0, "Maracas", MIN, { { LEVEL, 0.5f } }),
        K (S_BELL, 540, 800, 0.25f, 0, 0, "Cowbell", LIM),
        K (S_TOM, 90, 0.5f, 0.15f, 0, 0, "Tom Basso", BLU),
        K (S_TOM, 140, 0.4f, 0.15f, 0, 0, "Tom Medio", BLU),
        K (S_TOM, 200, 0.35f, 0.15f, 0, 0, "Tom Alto", BLU),
        K (S_CONGA, 220, 0.3f, 0, 0, 0, "Conga Bassa", LIM),
        K (S_CONGA, 330, 0.25f, 0, 0, 0, "Conga Alta", LIM),
        K (S_RIM, 1700, 0.04f, 0, 0, 0, "Rim", LIM),
        K (S_CYMBAL, 1.6f, 0.5f, 0, 0, 0, "Cymbal", YEL, { { LEVEL, 0.45f } }),
        K (S_808, 41.2f, 2.0f, 0.15f, 0.1f, 0.3f, "Bass E1", VIO) } },

    { "Memphis Phonk", "Phonk (cowbell melodico)", {
        K (S_KICK, 140, 48, 0.4f, 0.6f, 0.6f, "Kick Phonk", MAG, { { DRIVE, 0.35f }, { BITS, 0.25f } }),
        K (S_SNARE, 200, 0.2f, 0.1f, 0.6f, 0.5f, "Snare Lo-fi", ORA, { { BITS, 0.35f }, { RATE, 0.3f }, { DRIVE, 0.2f } }),
        K (S_CLAP, 10, 0.2f, 1200, 0.2f, 0, "Clap Sporco", AMB, { { BITS, 0.3f } }),
        K (S_HAT, 0.045f, 8, 0.6f, 0, 0, "Hat", CYA, { { CHOKE, 1 }, { BITS, 0.3f }, { LEVEL, 0.55f } }),
        K (S_HAT, 0.4f, 8, 0.6f, 0, 0, "Open Hat", AQU, { { CHOKE, 1 }, { BITS, 0.3f }, { LEVEL, 0.45f } }),
        K (S_808, 46.25f, 1.2f, 0.8f, 0.3f, 0.5f, "808 F#1", VIO, { { DRIVE, 0.3f } }),
        K (S_808, 55.0f, 1.1f, 0.8f, 0.3f, 0.5f, "808 A1", VIO, { { DRIVE, 0.3f } }),
        K (S_RIM, 1600, 0.05f, 0, 0, 0, "Rim", LIM, { { BITS, 0.3f } }),
        K (S_BELL, 540, 800, 0.3f, 0, 0, "Cowbell 0", LIM, { { DRIVE, 0.2f } }),
        K (S_BELL, 540, 800, 0.3f, 0, 0, "Cowbell +2", LIM, { { TUNE, 2 }, { DRIVE, 0.2f } }),
        K (S_BELL, 540, 800, 0.3f, 0, 0, "Cowbell +3", LIM, { { TUNE, 3 }, { DRIVE, 0.2f } }),
        K (S_BELL, 540, 800, 0.3f, 0, 0, "Cowbell +5", LIM, { { TUNE, 5 }, { DRIVE, 0.2f } }),
        K (S_BELL, 540, 800, 0.3f, 0, 0, "Cowbell +7", LIM, { { TUNE, 7 }, { DRIVE, 0.2f } }),
        K (S_BELL, 540, 800, 0.3f, 0, 0, "Cowbell +8", LIM, { { TUNE, 8 }, { DRIVE, 0.2f } }),
        K (S_BELL, 540, 800, 0.3f, 0, 0, "Cowbell +10", LIM, { { TUNE, 10 }, { DRIVE, 0.2f } }),
        K (S_BELL, 540, 800, 0.3f, 0, 0, "Cowbell +12", LIM, { { TUNE, 12 }, { DRIVE, 0.2f } }) } },

    { "R&B Soft", "R&B 65-80 (La minore)", {
        K (S_KICK, 90, 45, 0.5f, 0.1f, 0.05f, "Kick Morbido", MAG, { { LOWG, 2 }, { CMODE, 1 }, { CAMT, 0.3f } }),
        K (S_SNAP, 0.12f, 2000, 0, 0, 0, "Finger Snap", PNK, { { SREV, 0.35f } }),
        K (S_SNARE, 170, 0.25f, 0.1f, 0.2f, 0.5f, "Snare Soft", ORA, { { CUTOFF, 8000 }, { SREV, 0.25f } }),
        K (S_HAT, 0.05f, 6.5f, 0.3f, 0, 0, "Hat Soft", CYA, { { CHOKE, 1 }, { LEVEL, 0.5f } }),
        K (S_HAT, 0.3f, 6.5f, 0.3f, 0, 0, "Open Hat", AQU, { { CHOKE, 1 }, { LEVEL, 0.4f } }),
        K (S_RIM, 1500, 0.06f, 0, 0, 0, "Rim", LIM, { { SREV, 0.2f } }),
        K (S_SHAKER, 0.14f, 6, 0, 0, 0, "Shaker", MIN, { { LEVEL, 0.45f } }),
        K (S_CONGA, 260, 0.3f, 0, 0, 0, "Perc", LIM, { { LEVEL, 0.6f } }),
        K (S_808, 55.0f, 1.4f, 0.1f, 0.1f, 0.2f, "Sub A1", VIO, { { CUTOFF, 1500 } }),
        K (S_808, 43.65f, 1.4f, 0.1f, 0.1f, 0.2f, "Sub F1", VIO, { { CUTOFF, 1500 } }),
        K (S_STAB, 57, 2, 1.2f, 0.25f, 0, "Keys Am9", FUC, { { SREV, 0.35f }, { ATTACK, 8 } }),
        K (S_STAB, 53, 4, 1.2f, 0.25f, 0, "Keys Fmaj9", FUC, { { SREV, 0.35f }, { ATTACK, 8 } }),
        K (S_STAB, 50, 2, 1.2f, 0.25f, 0, "Keys Dm9", FUC, { { SREV, 0.35f }, { ATTACK, 8 } }),
        K (S_STAB, 55, 3, 1.2f, 0.25f, 0, "Keys Gsus", FUC, { { SREV, 0.35f }, { ATTACK, 8 } }),
        K (S_PLUCK, 72, 1.0f, 0.5f, 0, 0, "Pluck C5", PNK, { { SDLY, 0.3f }, { SREV, 0.3f } }),
        K (S_PLUCK, 76, 1.0f, 0.5f, 0, 0, "Pluck E5", PNK, { { SDLY, 0.3f }, { SREV, 0.3f } }) } } };
    return kits;
}
}
