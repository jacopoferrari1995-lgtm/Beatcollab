#include <set>
#include <map>
#include "PadProcessor.h"
#include "PadEditor.h"
#include <complex>
#include "KeyDetect.h"
#include "Library.h"
#include "Generator.h"

namespace jp
{
const PInfo kPInfo[kNumPadParams] = {
    { "gain",   "GAIN",      -24, 24, 0, 11 },      { "level", "VOLUME",    0, 1.41f, 1, 13 },     { "pan",   "PAN",      -1, 1, 0, 9 },
    { "tune",   "TUNE",      -24, 24, 0, 2 },       { "fine",  "FINE",      -100, 100, 0, 3 },
    { "start",  "START",     0, 1, 0, 0 },          { "end",   "END",       0, 1, 1, 0 },
    { "attack", "ATTACK",    0, 200, 0, 4 },        { "decay", "DECAY",     10, 4000, 4000, 4 },   { "release","RELEASE",  5, 2000, 60, 4 },
    { "mode",   "MODE",      0, 1, 0, 6 },          { "reverse","REVERSE",  0, 1, 0, 8 },          { "choke", "CHOKE",     0, 8, 0, 7 },
    { "loop",   "LOOP",      0, 1, 0, 8 },          { "warp",  "SYNC BPM",  0, 2, 0, 6 },          { "sbpm",  "BPM SAMPLE", 0, 240, 0, 10 },
    { "vlevel", "VEL>VOL",   0, 1, 0.6f, 0 },      { "vfilt", "VEL>FILT",  0, 1, 0, 0 },          { "vstart","VEL>START", 0, 1, 0, 0 },     { "vpitch","VEL>PITCH",-1, 1, 0, 1 },
    { "ftype",  "TIPO",      0, 2, 0, 6 },          { "cutoff","CUTOFF",    40, 20000, 20000, 5 }, { "reso",  "RESO",      0, 1, 0.1f, 0 },
    { "fenv",   "ENV AMT",   -1, 1, 0, 1 },         { "fdecay","ENV DECAY", 5, 2000, 200, 4 },
    { "eqon",   "EQ",        0, 1, 1, 8 },
    { "e1on", "B1", 0, 1, 0, 8 }, { "e1t", "TIPO 1", 0, 7, EQ_LC12, 6 },  { "e1f", "FREQ 1", 20, 20000, 40, 5 },     { "e1g", "GAIN 1", -18, 18, 0, 11 },   { "e1q", "Q 1", 0.3f, 8, 0.71f, 12 },
    { "e2on", "B2", 0, 1, 1, 8 }, { "e2t", "TIPO 2", 0, 7, EQ_LSHELF, 6 }, { "lowf", "FREQ 2", 20, 20000, 100, 5 },  { "lowg", "GAIN 2", -18, 18, 0, 11 },  { "e2q", "Q 2", 0.3f, 8, 0.71f, 12 },
    { "e3on", "B3", 0, 1, 1, 8 }, { "e3t", "TIPO 3", 0, 7, EQ_BELL, 6 },   { "midf", "FREQ 3", 20, 20000, 1000, 5 }, { "midg", "GAIN 3", -18, 18, 0, 11 },  { "midq", "Q 3", 0.3f, 8, 1, 12 },
    { "e4on", "B4", 0, 1, 1, 8 }, { "e4t", "TIPO 4", 0, 7, EQ_BELL, 6 },   { "e4f", "FREQ 4", 20, 20000, 3500, 5 },  { "e4g", "GAIN 4", -18, 18, 0, 11 },   { "e4q", "Q 4", 0.3f, 8, 1, 12 },
    { "e5on", "B5", 0, 1, 1, 8 }, { "e5t", "TIPO 5", 0, 7, EQ_HSHELF, 6 }, { "highf", "FREQ 5", 20, 20000, 8000, 5 }, { "highg", "GAIN 5", -18, 18, 0, 11 }, { "e5q", "Q 5", 0.3f, 8, 0.71f, 12 },
    { "e6on", "B6", 0, 1, 0, 8 }, { "e6t", "TIPO 6", 0, 7, EQ_HC12, 6 },  { "e6f", "FREQ 6", 20, 20000, 16000, 5 }, { "e6g", "GAIN 6", -18, 18, 0, 11 },   { "e6q", "Q 6", 0.3f, 8, 0.71f, 12 },
    { "cmode",  "COMP",      0, 3, 0, 6 },          { "camt",  "AMOUNT",    0, 1, 0.4f, 0 },       { "cmake", "MAKEUP",    0, 18, 0, 11 },   { "cmix",  "MIX",       0, 1, 1, 0 },
    { "drive",  "DRIVE",     0, 1, 0, 0 },          { "bits",  "BIT",       0, 1, 0, 0 },          { "rate",  "RATE",      0, 1, 0, 0 },
    { "srev",   "REVERB",    0, 1, 0, 0 },          { "sdly",  "DELAY",     0, 1, 0, 0 },
    { "mute",   "MUTE",      0, 1, 0, 8 },          { "solo",  "SOLO",      0, 1, 0, 8 },          { "slice", "SLICE",     0, 64, 0, 7 },
    { "skey",   "TONALITA",  -2, 35, -1, 7 },
    { "dtype",  "SATURAZIONE", 0, kDriveTypes - 1, 0, 6 },
    { "lgain",  "LAYER GAIN", -24, 24, 0, 11 },  { "ltune", "LAYER TUNE", -24, 24, 0, 2 },  { "lpan", "LAYER PAN", -1, 1, 0, 9 },
    { "lvel",   "LAYER VEL", 0, 127, 0, 7 },      { "lmode", "LAYER MODO", 0, 1, 0, 6 } };

const char* eqTypeName (int t) { static const char* n[] = { "TAGLIO BASSI 12", "TAGLIO BASSI 24", "SHELF BASSI", "CAMPANA", "NOTCH", "SHELF ALTI", "TAGLIO ALTI 12", "TAGLIO ALTI 24" }; return n[juce::jlimit (0, kEqTypes - 1, t)]; }
const char* driveTypeName (int t) { static const char* n[] = { "MORBIDA", "NASTRO", "VALVOLA", "CONSOLE", "OVERDRIVE", "DISTORSIONE", "FUZZ" }; return n[juce::jlimit (0, kDriveTypes - 1, t)]; }
const char* driveTypeDesc (int t)
{
    static const char* d[] = { "Saturazione pulita e simmetrica: arrotonda i picchi senza cambiare il colore.",
                               "Nastro: comprime dolce e scurisce un filo gli alti.",
                               "Valvola: asimmetrica, armoniche pari, gonfia il corpo.",
                               "Console: trasformatore, un po' di peso e medie presenti.",
                               "Overdrive da pedale: taglia i bassi prima, spinge le medie, suono crunchy.",
                               "Distorsione: clip deciso e scuro, ringhio da amplificatore.",
                               "Fuzz: squadra il suono, asimmetrico e sporco, sustain infinito." };
    return d[juce::jlimit (0, kDriveTypes - 1, t)];
}
float driveShape (int t, float x)
{
    switch (t)
    {
        case 1: return std::tanh (x + 0.05f * x * x) - 0.0f;
        case 2: { const float b = 0.2f; return std::tanh (x + b) - std::tanh (b) + 0.06f * x / (1 + std::abs (x)); }
        case 3: { const float y = x / std::sqrt (1 + 0.6f * x * x); return std::tanh (1.15f * y) / std::tanh (1.15f); }
        case 4: { const float a = std::abs (x); return std::copysign (a < 1.0f / 3 ? 2 * a : (a < 2.0f / 3 ? (3 - (2 - 3 * a) * (2 - 3 * a)) / 3 : 1.0f), x); }
        case 5: { const float a = std::abs (x); return std::copysign (a < 0.4f ? a * 1.6f : juce::jmin (1.0f, 0.64f + (a - 0.4f) * 0.35f), x); }
        case 6: { const float y = x + 0.35f * std::abs (x); return std::tanh (3.0f * y) * 0.9f; }
        default: return std::tanh (x);
    }
}

const int kAutoParams[kAutoN] = { GAIN, LEVEL, PAN, TUNE, FINE, START, DECAY, CUTOFF, RESO, DRIVE, SREV, SDLY };
int autoIndex (int i) { for (int k = 0; k < kAutoN; ++k) if (kAutoParams[k] == i) return k; return -1; }

int choiceCount (int i) { if (i >= B1ON && i <= B6Q && (i - B1ON) % 5 == 1) return kEqTypes; return i == FTYPE ? 3 : i == WARP ? 3 : i == CMODE ? 4 : i == CHOKE ? 9 : i == DTYPE ? kDriveTypes : 2; }

juce::String formatPadParam (int i, float v)
{
    const auto& p = kPInfo[i];
    switch (p.kind)
    {
        case 0: return juce::String (juce::roundToInt (v * 100)) + "%";
        case 1: return std::abs (v) < 0.005f ? juce::String ("0") : (v > 0 ? "+" : "") + juce::String (juce::roundToInt (v * 100));
        case 2: return (v > 0 ? "+" : "") + juce::String (juce::roundToInt (v)) + " st";
        case 3: return (v > 0 ? "+" : "") + juce::String (juce::roundToInt (v)) + " ct";
        case 4: return (i == DECAY && v >= 3990) ? juce::String ("HOLD") : (v < 10 ? juce::String (v, 1) : juce::String (juce::roundToInt (v))) + " ms";
        case 5: return (i == CUTOFF && v >= 19500) ? juce::String ("OPEN") : (v >= 1000 ? juce::String (v / 1000, 1) + "k" : juce::String (juce::roundToInt (v))) + " Hz";
        case 6:
        {
            const int k = juce::roundToInt (v);
            if (i == FTYPE) { const char* t[] = { "LOW PASS", "HIGH PASS", "BAND PASS" }; return t[juce::jlimit (0, 2, k)]; }
            if (i == WARP) { const char* t[] = { "OFF", "REPITCH", "STRETCH" }; return t[juce::jlimit (0, 2, k)]; }
            if (i == CMODE) { const char* t[] = { "OFF", "SOFT", "MEDIUM", "HARD" }; return t[juce::jlimit (0, 3, k)]; }
            if (i == DTYPE) return driveTypeName (k);
            if (i == LMODE) return k ? "VEL SWITCH" : "INSIEME";
            if (i >= B1ON && i <= B6Q) return eqTypeName (k);
            return k ? "GATE" : "ONE SHOT";
        }
        case 7: return v < 0.5f ? juce::String ("OFF") : juce::String (juce::roundToInt (v));
        case 8: return v > 0.5f ? "ON" : "OFF";
        case 9: return std::abs (v) < 0.01f ? juce::String ("C") : (v < 0 ? "L" : "R") + juce::String (juce::roundToInt (std::abs (v) * 100));
        case 10: return v < 40 ? juce::String ("--") : juce::String (v, 1);
        case 11: return std::abs (v) < 0.05f ? juce::String ("0 dB") : (v > 0 ? "+" : "") + juce::String (v, 1) + " dB";
        case 12: return juce::String (v, 2);
        case 13: return v < 0.001f ? juce::String ("-inf") : juce::String (juce::Decibels::gainToDecibels (v), 1) + " dB";
        default: return juce::String (v, 2);
    }
}

// ---------------- biquad ----------------
static void rbj (int type, double sr, double f, double gDb, double q, double& b0, double& b1, double& b2, double& a0, double& a1, double& a2)
{
    const double A = std::pow (10.0, gDb / 40.0), w = juce::MathConstants<double>::twoPi * juce::jlimit (10.0, sr * 0.45, f) / sr, cs = std::cos (w), sn = std::sin (w);
    if (type >= 3)
    {
        const double al = sn / (2 * q);
        if (type == 3) { b0 = (1 - cs) / 2; b1 = 1 - cs; b2 = b0; }
        else if (type == 4) { b0 = (1 + cs) / 2; b1 = -(1 + cs); b2 = b0; }
        else if (type == 5) { b0 = 1; b1 = -2 * cs; b2 = 1; }
        else { b0 = al; b1 = 0; b2 = -al; }
        a0 = 1 + al; a1 = -2 * cs; a2 = 1 - al;
    }
    else if (type == 1)
    {
        const double al = sn / (2 * q);
        b0 = 1 + al * A; b1 = -2 * cs; b2 = 1 - al * A; a0 = 1 + al / A; a1 = -2 * cs; a2 = 1 - al / A;
    }
    else
    {
        const double al = sn / 2 * std::sqrt (2.0), sA = 2 * std::sqrt (A) * al * 0.5;
        if (type == 0) { b0 = A * ((A + 1) - (A - 1) * cs + sA); b1 = 2 * A * ((A - 1) - (A + 1) * cs); b2 = A * ((A + 1) - (A - 1) * cs - sA);
                         a0 = (A + 1) + (A - 1) * cs + sA; a1 = -2 * ((A - 1) + (A + 1) * cs); a2 = (A + 1) + (A - 1) * cs - sA; }
        else           { b0 = A * ((A + 1) + (A - 1) * cs + sA); b1 = -2 * A * ((A - 1) + (A + 1) * cs); b2 = A * ((A + 1) + (A - 1) * cs - sA);
                         a0 = (A + 1) - (A - 1) * cs + sA; a1 = 2 * ((A - 1) - (A + 1) * cs); a2 = (A + 1) - (A - 1) * cs - sA; }
    }
}
void Biquad::set (int type, double sr, double f, double gDb, double q)
{
    double B0, B1, B2, A0, A1, A2; rbj (type, sr, f, gDb, q, B0, B1, B2, A0, A1, A2);
    b0 = (float) (B0 / A0); b1 = (float) (B1 / A0); b2 = (float) (B2 / A0); a1 = (float) (A1 / A0); a2 = (float) (A2 / A0);
}
float Biquad::magDb (int type, double sr, double f, double gDb, double q, double hz)
{
    double B0, B1, B2, A0, A1, A2; rbj (type, sr, f, gDb, q, B0, B1, B2, A0, A1, A2);
    const double w = juce::MathConstants<double>::twoPi * hz / sr;
    const std::complex<double> z = std::polar (1.0, -w), z2 = z * z;
    const auto num = B0 + B1 * z + B2 * z2, den = A0 + A1 * z + A2 * z2;
    return (float) (20.0 * std::log10 (std::abs (num / den) + 1e-12));
}

// ---------------- parametri globali ----------------
static const juce::StringArray kCurves { "LINEARE", "MORBIDA", "DURA", "S", "FISSA 127" };
static const juce::StringArray kSwGrid { "1/8", "1/16" };
static const juce::StringArray kDly { "1/32", "1/16T", "1/16", "1/16D", "1/8T", "1/8", "1/8D", "1/4T", "1/4", "1/4D", "1/2", "1/1", "LIBERO" };
static const double kDlyBeats[] = { 0.125, 1.0 / 6.0, 0.25, 0.375, 1.0 / 3.0, 0.5, 0.75, 2.0 / 3.0, 1.0, 1.5, 2.0, 4.0, 0 };
static const juce::StringArray kBus { "OFF", "GLUE", "PUNCH", "SMASH" };

juce::AudioProcessorValueTreeState::ParameterLayout Processor::layout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    auto F = [&l] (const char* id, const char* n, float a, float b, float d) { l.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, n, NormalisableRange<float> (a, b), d)); };
    auto C = [&l] (const char* id, const char* n, const StringArray& c, int d) { l.add (std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, n, c, d)); };
    F ("out", "Output", -30, 6, 0);
    C ("curve", "Velocity Curve", kCurves, 0);
    F ("swing", "Swing", 50, 75, 50);
    C ("swGrid", "Swing Grid", kSwGrid, 1);
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "swMidi", 1 }, "Swing sulle note MIDI", false));
    F ("human", "Humanize", 0, 1, 0);
    F ("bpm", "BPM interno", 50, 220, 120);
    l.add (std::make_unique<AudioParameterInt> (ParameterID { "root", 1 }, "Nota del pad 1", 0, 112, 36));
    {   StringArray k { "LIBERA" }; static const char* n[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        for (int i = 0; i < 12; ++i) k.add (String (n[i]) + " maggiore"); for (int i = 0; i < 12; ++i) k.add (String (n[i]) + " minore");
        C ("kkey", "Tonalita del kit", k, 0); }

    F ("revSize", "Reverb Decay", 0, 1, 0.5f); F ("revDamp", "Reverb Damp", 0, 1, 0.5f); F ("revRet", "Reverb Return", 0, 1, 0.7f);
    { NormalisableRange<float> r (20, 1000); r.setSkewForCentre (150); l.add (std::make_unique<AudioParameterFloat> (ParameterID { "revLo", 1 }, "Reverb taglio bassi", r, 120)); }
    { NormalisableRange<float> r (1000, 20000); r.setSkewForCentre (6000); l.add (std::make_unique<AudioParameterFloat> (ParameterID { "revHi", 1 }, "Reverb taglio alti", r, 9000)); }
    F ("revPre", "Reverb pre-delay ms", 0, 120, 10);
    C ("dlyDiv", "Delay Time", kDly, 5); F ("dlyFb", "Delay Feedback", 0, 0.95f, 0.35f); F ("dlyRet", "Delay Return", 0, 1, 0.6f);
    { NormalisableRange<float> r (10, 2000); r.setSkewForCentre (300); l.add (std::make_unique<AudioParameterFloat> (ParameterID { "dlyMs", 1 }, "Delay ms (libero)", r, 250)); }
    { NormalisableRange<float> r (20, 2000); r.setSkewForCentre (200); l.add (std::make_unique<AudioParameterFloat> (ParameterID { "dlyLo", 1 }, "Delay taglio bassi", r, 150)); }
    { NormalisableRange<float> r (500, 20000); r.setSkewForCentre (4000); l.add (std::make_unique<AudioParameterFloat> (ParameterID { "dlyHi", 1 }, "Delay taglio alti", r, 6000)); }
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "nrOn", 1 }, "Note Repeat", false));
    C ("nrRate", "Note Repeat Rate", StringArray { "1/4", "1/8", "1/8T", "1/16", "1/16T", "1/32", "1/32T" }, 3);
    C ("busMode", "Bus Comp", kBus, 0); F ("busAmt", "Bus Comp Amount", 0, 1, 0.4f);
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "clip", 1 }, "Clip", false));
    // master sul kit intero
    F ("revKit", "Reverb sul kit", 0, 1, 0); F ("dlyKit", "Delay sul kit", 0, 1, 0); F ("satAmt", "Saturazione kit", 0, 1, 0);
    // effetti da performance (manopole Q-LINK in modo FX, MPK CC 70-77)
    {   // 8 slot di effetti: tipo scelto da menu e una manopola ciascuno
        StringArray tn; for (int t = 0; t < pfx::kNumTypes; ++t) tn.add (pfx::info (t).name);
        for (int k = 0; k < 8; ++k)
        {
            const int dt = Processor::kFxDefault[k];
            l.add (std::make_unique<AudioParameterChoice> (ParameterID { "fx" + String (k + 1) + "type", 1 }, "FX " + String (k + 1) + " Tipo", tn, dt));
            l.add (std::make_unique<AudioParameterFloat> (ParameterID { "fx" + String (k + 1), 1 }, "FX " + String (k + 1), NormalisableRange<float> (0, 1), pfx::info (dt).neutral));
        }
    }
    // pad automatizzabili
    for (int p = 0; p < kPads; ++p)
        for (int k = 0; k < kAutoN; ++k)
        {
            const auto& I = kPInfo[kAutoParams[k]];
            const auto id = "p" + juce::String (p + 1).paddedLeft ('0', 2) + "_" + I.id;
            const auto lab = juce::String (I.label).toLowerCase();
            const auto nm = "Pad " + juce::String (p + 1).paddedLeft ('0', 2) + " " + lab.substring (0, 1).toUpperCase() + lab.substring (1);
            NormalisableRange<float> rg (I.min, I.max);
            if (kAutoParams[k] == CUTOFF) rg.setSkewForCentre (1000.0f);
            if (kAutoParams[k] == DECAY) rg.setSkewForCentre (400.0f);
            l.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, nm, rg, I.def));
        }
    return l;
}

Processor::Processor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", layout())
{
    formats.registerBasicFormats();
    for (int i = 0; i < kPads; ++i) { padGlow[i] = 0; padMeter[i] = 0; padGr[i] = 0; lastVel[i] = 0; hits[i] = 0; pad[i].colour = i; }
    for (int i = 0; i < kScope; ++i) { scPk[i] = 0; scHit[i] = 0; scVel[i] = 0; }
    for (int i = 0; i < kPads; ++i) padPos[i] = -1.0f;
    for (int k = 0; k < kFx; ++k) ccPend[k] = -1.0f;
    if (! ccLoaded.exchange (true))
    {
        juce::PropertiesFile pf (jlook::settingsOptions());
        const auto t = juce::StringArray::fromTokens (pf.getValue ("sampler_ccmap"), ",", "");
        if (t.size() == kFx) for (int k = 0; k < kFx; ++k) ccMap[k] = t[k].getIntValue();
    }
    trigs.reserve (1024); events.reserve (1024); pend.reserve (512);
    for (int p = 0; p < kPads; ++p)
        for (int k = 0; k < kAutoN; ++k)
        {
            const auto id = "p" + juce::String (p + 1).paddedLeft ('0', 2) + "_" + kPInfo[kAutoParams[k]].id;
            hp[p][k] = apvts.getParameter (id); hpRaw[p][k] = apvts.getRawParameterValue (id); lastHost[p][k] = hpRaw[p][k]->load();
        }
    pendingRandom = true;   // kit casuale appena aperto (se non arriva un progetto salvato): lo fa il timer, cosi' il caricamento e' immediato
    startTimerHz (15);
}

// Ableton -> pad (nel processBlock) e pad -> Ableton (qui, sul thread dei messaggi)
// I movimenti che arrivano dal controller o dall'interfaccia sono "gesti" per Ableton (inizio ... fine):
// cosi' in registrazione l'automazione viene scritta, e quando smetti di girare la riprende a leggere.
void Processor::touch (juce::RangedAudioParameter* pp, float norm)
{
    const double now = juce::Time::getMillisecondCounterHiRes();
    auto it = std::find_if (gestures.begin(), gestures.end(), [pp] (const Gesture& g) { return g.p == pp; });
    if (it == gestures.end()) { pp->beginChangeGesture(); gestures.push_back ({ pp, now }); } else it->t = now;
    pp->setValueNotifyingHost (norm);
}
void Processor::timerCallback()
{
    if (pendingRandom && ++startTicks >= 3)
    {
        pendingRandom = false;
        randomKit (*this, 1); loadBankByRoles (*this, 0); ++kitVersion;
    }
    for (int k = 0; k < kFx; ++k)
    {
        const float v = ccPend[k].exchange (-1.0f);
        if (v >= 0) if (auto* pp = apvts.getParameter (kFxIds[k])) touch (pp, v);
    }
    for (int p = 0; p < kPads; ++p)
        for (int k = 0; k < kAutoN; ++k)
        {
            const float v = pad[p].get (kAutoParams[k]);
            if (std::abs (v - lastHost[p][k].load()) > 1e-5f) { lastHost[p][k] = v; touch (hp[p][k], hp[p][k]->convertTo0to1 (v)); lastHost[p][k] = hpRaw[p][k]->load(); }
        }
    if (ccDirty.exchange (false))
    {
        juce::String m; for (int k = 0; k < kFx; ++k) m << ccMap[k].load() << (k < kFx - 1 ? "," : "");
        juce::PropertiesFile pf (jlook::settingsOptions()); pf.setValue ("sampler_ccmap", m); pf.saveIfNeeded();
    }
    const double now = juce::Time::getMillisecondCounterHiRes();
    for (auto it = gestures.begin(); it != gestures.end();)
        if (now - it->t > 350) { it->p->endChangeGesture(); it = gestures.erase (it); } else ++it;
}
void Processor::setPadValue (int p, int i, float v)
{
    pad[p].v[i] = v;
    const int k = autoIndex (i);
    if (k >= 0) { lastHost[p][k] = v; hp[p][k]->setValueNotifyingHost (hp[p][k]->convertTo0to1 (v)); lastHost[p][k] = hpRaw[p][k]->load(); pad[p].v[i] = hpRaw[p][k]->load(); }
}

const char* const Processor::kFxIds[Processor::kFx] = { "fx1", "fx2", "fx3", "fx4", "fx5", "fx6", "fx7", "fx8" };
const int Processor::kFxDefault[Processor::kFx] = { pfx::FILTER, pfx::GAIN, pfx::REVERB, pfx::DELAY, pfx::SCRATCH, pfx::TAPESTOP, pfx::STUTTER, pfx::LOFI };

void Processor::prepareToPlay (double s, int block)
{
    sr = s;
    perf.prepare (sr);
    for (int k = 0; k < 8; ++k) { fxTypeRaw[k] = apvts.getRawParameterValue ("fx" + juce::String (k + 1) + "type"); fxAmtRaw[k] = apvts.getRawParameterValue ("fx" + juce::String (k + 1)); }
    kw1.set (2, sr, 1681, 4.0, 0.707); kw2.set (4, sr, 38, 0, 0.5); kw1.reset(); kw2.reset(); lufsAcc = 0; lufsN = 0; for (auto& b : lufsBins) b = 0;
    for (auto& v : voices) { v.active = false; v.s.reset(); }
    padBuf.setSize (kPads * 2, juce::jmax (block, 2048)); padBuf.clear();
    revL.assign ((size_t) juce::jmax (block, 2048), 0); revR = revL; dlyInL = revL; dlyInR = revL;
    dlyBuf.setSize (2, (int) (sr * 4.5)); dlyBuf.clear(); dlyW = 0;
    reverb.setSampleRate (sr); reverb.reset();
    preBuf.assign ((size_t) (2 * ((int) (sr * 0.15) + 8)), 0.0f); preW = 0; rvLp.reset();
    for (auto& c : chain) c = PadChain();
}

float Processor::velCurve (int vel) const
{
    const float x = juce::jlimit (0.0f, 1.0f, vel / 127.0f);
    switch ((int) apvts.getRawParameterValue ("curve")->load())
    {
        case 1: return std::pow (x, 0.55f);
        case 2: return std::pow (x, 1.9f);
        case 3: return x * x * (3 - 2 * x);
        case 4: return 1.0f;
        default: return x;
    }
}

void Processor::uiNoteOn (int p, int vel)
{
    int s1, n1, s2, n2; fifo.prepareToWrite (1, s1, n1, s2, n2);
    if (n1 > 0) fifoBuf[s1] = { p, vel, true }; else if (n2 > 0) fifoBuf[s2] = { p, vel, true };
    fifo.finishedWrite (n1 + n2);
}
void Processor::uiNoteOff (int p)
{
    int s1, n1, s2, n2; fifo.prepareToWrite (1, s1, n1, s2, n2);
    if (n1 > 0) fifoBuf[s1] = { p, 0, false }; else if (n2 > 0) fifoBuf[s2] = { p, 0, false };
    fifo.finishedWrite (n1 + n2);
}

void Processor::startVoice (int p, int vel, float r0, float r1)
{
    auto s = getSample (p); auto s2 = r0 >= 0 ? SamplePtr() : getLayer (p);
    hits[p] = hits[p].load() + 1; lastVel[p] = vel; colHits |= 1 << p; colVel = juce::jmax (colVel, vel);
    const bool okA = s != nullptr && s->buf.getNumSamples() >= 8, okB = s2 != nullptr && s2->buf.getNumSamples() >= 8;
    if (! okA && ! okB) return;
    auto& P = pad[p];
    const int choke = juce::roundToInt (P.get (CHOKE));
    const bool loop = P.get (LOOP) > 0.5f;
    for (auto& v : voices)
        if (v.active && (v.pad == p || (choke > 0 && v.pad != p && juce::roundToInt (pad[v.pad].get (CHOKE)) == choke
                                         && v.age > (int) (sr * 0.03))))   // colpi insieme (entro 30 ms): suonano tutti e due, come a mano
            if (v.pad != p || P.get (MODE) < 0.5f || loop) v.choking = true;
    // layer: INSIEME = il secondo campione suona con il primo (sopra la soglia di velocity se impostata)
    //        VEL SWITCH = sotto la soglia suona il primo, sopra solo il layer
    const int thr = juce::roundToInt (P.get (LVEL)); const bool sw = P.get (LMODE) > 0.5f;
    bool playA = okA, playB = okB && vel >= thr;
    if (sw && okB && thr > 0) { playA = okA && vel < thr; playB = vel >= thr; }
    if (! playA && ! playB) playA = okA;
    const int cn = juce::jlimit (0, 5, chordN[p].load());
    if (playA && cn > 0 && r0 < 0)
    {   // pad accordo (progressioni MPC): lo stesso suono intonato su ogni nota dell'accordo
        const float gm = 1.0f / std::sqrt ((float) cn);
        for (int k = 0; k < cn; ++k) launch (p, vel, r0, r1, s, false, (float) chordIv[p][k].load(), gm);
    }
    else if (playA) launch (p, vel, r0, r1, s, false);
    if (playB) launch (p, vel, -1, -1, s2, true);
    chain[p].active = (int) (sr * 1.5);
}

void Processor::launch (int p, int vel, float r0, float r1, const SamplePtr& s, bool layer, float extraSemis, float gmul)
{
    auto& P = pad[p];
    const bool loop = P.get (LOOP) > 0.5f;
    Voice* v = nullptr;
    for (auto& x : voices) if (! x.active) { v = &x; break; }
    if (v == nullptr) { int oldest = -1; for (auto& x : voices) if (x.age > oldest) { oldest = x.age; v = &x; } }
    const float vv = velCurve (vel);
    const int n = s->buf.getNumSamples();
    double a0 = (layer ? 0.0 : (r0 >= 0 ? r0 : P.get (START))) * n, a1 = (layer ? 1.0 : (r1 >= 0 ? r1 : P.get (END))) * n; if (a1 - a0 < 32) a1 = juce::jmin ((double) n, a0 + 32);
    const double sa0 = a0;
    a0 += (1.0 - vv) * P.get (VSTART) * 0.2 * (a1 - a0);
    const double semis = P.get (TUNE) + P.get (FINE) / 100.0 + (vv - 0.75) * P.get (VPITCH) * 4.0 + (layer ? P.get (LTUNE) : 0.0f) + extraSemis;
    const double pitch = std::pow (2.0, semis / 12.0) * s->sr / sr;
    const int warp = layer ? 0 : juce::roundToInt (P.get (WARP)); const float sb = layer ? 0.0f : P.get (SBPM);
    const double tempo = (warp > 0 && sb > 40.0f) ? curBpm / sb : 1.0;
    v->active = true; v->rel = false; v->choking = false; v->pad = p; v->age = 0; v->s = s; v->loop = loop;
    v->layer = layer; v->panOff = layer ? P.get (LPAN) : 0.0f;
    v->dir = P.get (REVERSE) > 0.5f ? -1 : 1;
    v->a0 = loop ? sa0 : a0; v->a1 = a1;
    v->warp = (warp == 2 && sb > 40.0f) ? 2 : (warp == 1 && sb > 40.0f ? 1 : 0);
    v->sbpm = sb; v->pitch = pitch;
    v->velSemis = (float) ((vv - 0.75) * P.get (VPITCH) * 4.0); v->velDb = -(1.0f - vv) * 18.0f * P.get (VLEVEL); v->velCut = std::pow (2.0f, (vv - 1.0f) * P.get (VFILTER) * 5.0f);
    v->pos = v->dir > 0 ? a0 : a1 - 2;
    if (v->warp == 2)
    {
        v->tRate = tempo * s->sr / sr; v->pRate = pitch; v->inc = v->tRate;
        v->G = juce::jmax (256.0, 0.05 * sr);
        v->gr[0] = { v->pos - v->dir * v->G * 0.5 * v->pRate, v->G * 0.5 };
        v->gr[1] = { v->pos, 0.0 };
    }
    else v->inc = pitch * (v->warp == 1 ? tempo : 1.0);
    v->gain = gmul * 0.708f * juce::Decibels::decibelsToGain ((layer ? P.get (LGAIN) : P.get (GAIN)) - (1.0f - vv) * 18.0f * P.get (VLEVEL));   // -3 dB di margine
    const float atk = P.get (ATTACK);
    v->env = atk < 0.5f ? 1.0f : 0.0f; v->atkDone = atk < 0.5f; v->atkInc = atk < 0.5f ? 1.0f : (float) (1.0 / (atk * 0.001 * sr));
    const float dec = P.get (DECAY); v->hold = dec >= 3990 || loop;
    v->decCoef = (float) std::exp (-1.0 / (dec * 0.001 * sr / 4.6));
    v->relCoef = (float) std::exp (-1.0 / (P.get (RELEASE) * 0.001 * sr / 4.6));
    v->gate = (P.get (MODE) > 0.5f || loop) ? 1 : -1;
    if (r0 >= 0) { v->gate = -1; v->loop = false; v->hold = true; }   // ascolto di una fetta dall'editor
    v->cutBase = juce::jlimit (30.0f, 20000.0f, P.get (CUTOFF) * std::pow (2.0f, (vv - 1.0f) * P.get (VFILTER) * 5.0f));
    v->fenvAmt = P.get (FENV); v->fEnv = 1.0f; v->fEnvCoef = (float) std::exp (-1.0 / (P.get (FDECAY) * 0.001 * sr / 4.6));
    v->ftype = juce::roundToInt (P.get (FTYPE)); v->fCount = 0;
    v->filt = ! (v->ftype == 0 && v->cutBase >= 19500.0f && std::abs (v->fenvAmt) < 0.01f);
    v->fk = 2.0f - 1.94f * P.get (RESO);
    v->fg = (float) std::tan (juce::MathConstants<double>::pi * juce::jmin ((double) v->cutBase, sr * 0.45) / sr);
    for (int c = 0; c < 2; ++c) { v->ic1[c] = 0; v->ic2[c] = 0; }
}

void Processor::releasePad (int p) { for (auto& v : voices) if (v.active && v.pad == p && v.gate > 0) v.rel = true; }

void Processor::render (Voice& v, float* L, float* R, int i)
{
    const auto& b = v.s->buf; const int n = b.getNumSamples();
    const float* d0 = b.getReadPointer (0); const float* d1 = b.getReadPointer (juce::jmin (1, b.getNumChannels() - 1));
    auto rd = [&] (double pos, float& l, float& r)
    {
        if (v.loop) { const double span = v.a1 - v.a0; while (pos >= v.a1) pos -= span; while (pos < v.a0) pos += span; }
        if (pos < 0 || pos >= n - 1) { l = r = 0; return; }
        const int k = (int) pos; const float f = (float) (pos - k);
        const int km = juce::jmax (0, k - 1), k1 = juce::jmin (n - 1, k + 1), k2 = juce::jmin (n - 1, k + 2);
        auto cub = [f] (float y0, float y1, float y2, float y3) { return y1 + 0.5f * f * (y2 - y0 + f * (2 * y0 - 5 * y1 + 4 * y2 - y3 + f * (3 * (y1 - y2) + y3 - y0))); };
        l = cub (d0[km], d0[k], d0[k1], d0[k2]); r = cub (d1[km], d1[k], d1[k1], d1[k2]);
    };
    float out[2] = { 0, 0 };
    if (v.warp == 2)
    {
        for (auto& g : v.gr)
        {
            const float w = 0.5f - 0.5f * std::cos ((float) (juce::MathConstants<double>::twoPi * g.ph / v.G));
            float l, r; rd (g.start + v.dir * g.ph * v.pRate, l, r);
            out[0] += l * w; out[1] += r * w;
            g.ph += 1; if (g.ph >= v.G) { g.ph = 0; g.start = v.pos; }
        }
    }
    else rd (v.pos, out[0], out[1]);
    v.pos += v.inc * v.dir; ++v.age;
    // fine del campione o del loop
    if (v.loop) { const double span = v.a1 - v.a0; if (v.pos >= v.a1) v.pos -= span; if (v.pos < v.a0) v.pos += span; }
    else if (v.dir > 0 ? v.pos >= v.a1 - 2 : v.pos <= v.a0) { v.active = false; v.s.reset(); return; }
    // inviluppo
    if (v.choking) { v.env *= 0.996f; if (v.env < 0.0005f) { v.active = false; v.s.reset(); return; } }
    else if (v.rel) { v.env *= v.relCoef; if (v.env < 0.0002f) { v.active = false; v.s.reset(); return; } }
    else if (! v.atkDone) { v.env += v.atkInc; if (v.env >= 1) { v.env = 1; v.atkDone = true; } }
    else if (! v.hold) { v.env *= v.decCoef; if (v.env < 0.0002f) { v.active = false; v.s.reset(); return; } }
    const double dist = v.loop ? 1e9 : (v.dir > 0 ? v.a1 - v.pos : v.pos - v.a0);
    const float edge = dist < 64 ? (float) (dist / 64) : 1.0f;
    if (v.filt)
    {
        if (std::abs (v.fenvAmt) > 0.005f && (v.fCount++ & 15) == 0)
        {
            const float cut = juce::jlimit (30.0f, 20000.0f, v.cutBase * std::pow (2.0f, v.fenvAmt * 7.0f * v.fEnv));
            v.fg = (float) std::tan (juce::MathConstants<double>::pi * juce::jmin ((double) cut, sr * 0.45) / sr);
        }
        v.fEnv *= v.fEnvCoef;
        for (int c = 0; c < 2; ++c)
        {
            const float x = out[c], v3 = x - v.ic2[c], v1 = (v.ic1[c] + v.fg * v3) / (1 + v.fg * (v.fg + v.fk)), v2 = v.ic2[c] + v.fg * v1;
            v.ic1[c] = 2 * v1 - v.ic1[c]; v.ic2[c] = 2 * v2 - v.ic2[c];
            out[c] = v.ftype == 0 ? v2 : v.ftype == 1 ? x - v.fk * v1 - v2 : v1 * v.fk;
        }
    }
    const float g = v.gain * v.env * edge;
    if (v.panOff != 0.0f)
    {
        const float a = (v.panOff + 1) * juce::MathConstants<float>::pi * 0.25f;
        L[i] += out[0] * g * std::cos (a) * 1.41421f; R[i] += out[1] * g * std::sin (a) * 1.41421f;
    }
    else { L[i] += out[0] * g; R[i] += out[1] * g; }
}

double Processor::swingTick (double t) const
{
    const double g = apvts.getRawParameterValue ("swGrid")->load() > 0.5f ? 24.0 : 48.0;
    const double sw = apvts.getRawParameterValue ("swing")->load();
    const double n = std::round (t / g);
    if (((juce::int64) n % 2 != 0) && std::abs (t - n * g) < g * 0.3) return n * g + (sw - 50.0) / 50.0 * g + (t - n * g);
    return t;
}

void Processor::loadPattern (int index)
{
    const auto& bank = patternBank(); index = juce::jlimit (0, (int) bank.size() - 1, index);
    std::vector<PatEv> ev; int len = 32;
    for (auto& [pd, str] : bank[(size_t) index].lanes) len = juce::jmax (len, (int) std::strlen (str));
    len = len > 32 ? 64 : 32;
    // i pattern sono scritti con la vecchia numerazione per genere: li porto sulla disposizione attuale del kit
    // (1 cassa, 2 rullante, 3 hat, 4 open, 5 clap/rim, 6-7 tom/perc, 8 piatto, 9 808, 10-16 suoni)
    static const std::map<juce::String, std::map<int, int>> remap {
        { "Boom Bap",   { {1,1},{2,2},{3,5},{4,3},{5,4},{6,2},{7,1},{8,6} } },
        { "Trap",       { {1,1},{2,5},{3,2},{4,3},{5,4},{6,3},{9,9},{10,9},{11,9},{12,9} } },
        { "Drill",      { {1,1},{2,2},{3,5},{4,3},{6,6},{8,2},{9,9},{10,9},{11,9},{12,9} } },
        { "Lo-fi",      { {1,1},{2,2},{3,5},{4,3},{6,5},{7,6},{9,7},{10,7} } },
        { "Old School", { {1,1},{2,2},{3,5},{4,3},{5,4},{6,6},{8,7},{9,6},{10,7},{11,7},{12,6},{13,7} } },
        { "Phonk",      { {1,1},{2,2},{3,5},{4,3},{5,4},{6,9},{7,9},{9,7},{10,7},{11,7},{12,7},{13,7},{14,7},{15,7},{16,7} } },
        { "R&B",        { {1,1},{2,5},{3,2},{4,3},{6,6},{7,7},{8,6},{9,9},{10,9},{11,10},{12,11},{13,12},{14,13} } } };
    const auto gm = remap.find (bank[(size_t) index].genre);
    std::set<std::pair<int, int>> seen;
    for (auto [pd0, str] : bank[(size_t) index].lanes)
    {
        int pd = pd0;
        if (gm != remap.end()) { auto f = gm->second.find (pd0); pd = f != gm->second.end() ? f->second : pd0; }
        const juce::String s (str);
        for (int i = 0; i < juce::jmin (len, s.length()); ++i)
        {
            const auto c = s[i]; const int vel = c == 'X' ? 127 : c == 'x' ? 100 : c == 'o' ? 72 : c == '-' ? 48 : 0;
            if (vel > 0 && pd >= 1 && pd <= kPads && seen.insert ({ i, pd }).second) ev.push_back ({ i * 24.0, pd - 1, vel });
        }
    }
    const juce::SpinLock::ScopedLockType l (recLock); events = std::move (ev); patternIndex = index; patternBars = len / 16;
}
void Processor::randomPattern (const juce::String& genre)
{
    juce::Array<int> idx; const auto& bank = patternBank();
    for (int i = 0; i < (int) bank.size(); ++i) if (genre.isEmpty() || genre == bank[(size_t) i].genre) idx.add (i);
    if (idx.isEmpty()) return;
    juce::Random r; int pick = idx[r.nextInt (idx.size())];
    if (idx.size() > 1) while (pick == patternIndex) pick = idx[r.nextInt (idx.size())];
    loadPattern (pick); varyPattern();
}
void Processor::varyPattern()
{   // variazione per ruoli: tocca solo cassa e hat del kit attuale, mai pad a caso
    int kick = -1, hat = -1;
    for (int p = 0; p < kPads; ++p) { const auto ro = jp::padRole (*this, p); if (ro == jp::R_KICK && kick < 0) kick = p; if (ro == jp::R_HAT && hat < 0) hat = p; }
    juce::Random r; const juce::SpinLock::ScopedLockType l (recLock);
    std::vector<PatEv> out;
    for (auto e : events)
    {
        if (e.pad == kick && std::fmod (e.tick, (double) kBar) > 1 && r.nextFloat() < 0.12f) continue;   // toglie qualche cassa (mai sull'uno)
        if (e.pad == hat && r.nextFloat() < 0.12f) out.push_back ({ e.tick + 12, hat, juce::jmax (40, e.vel - 40) });
        e.vel = juce::jlimit (30, 127, e.vel + r.nextInt ({ -12, 8 }));
        out.push_back (e);
    }
    const int bars = patternBars.load();
    if (kick >= 0) for (int k = 0; k < 2; ++k) if (r.nextFloat() < 0.5f)
    {   // cassa in piu su un ottavo libero
        const double t = r.nextInt (bars * 8) * 48.0 + (r.nextBool() ? 24 : 0);
        bool busy = false; for (auto& e : out) if (std::abs (e.tick - t) < 12 && e.pad == kick) busy = true;
        if (! busy) out.push_back ({ t, kick, 88 });
    }
    if (hat >= 0 && r.nextFloat() < 0.5f) { const int st = bars * 16 - 8 + r.nextInt (6); for (int q = 0; q < 4; ++q) out.push_back ({ (st + q * 0.5) * 24.0, hat, 70 + q * 10 }); }
    std::sort (out.begin(), out.end(), [] (const PatEv& a, const PatEv& b) { return a.tick < b.tick; });
    events = std::move (out);
}

void Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals nd;
    const int N = buffer.getNumSamples();
    buffer.clear(); trigs.clear();
    if (padBuf.getNumSamples() < N) { padBuf.setSize (kPads * 2, N, false, false, true); revL.resize ((size_t) N); revR.resize ((size_t) N); dlyInL.resize ((size_t) N); dlyInR.resize ((size_t) N); }
    auto par = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };

    double bpm = par ("bpm"); bool hostPlay = false; double ppq = 0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = *b;
            hostPlay = pos->getIsPlaying();
            if (auto q = pos->getPpqPosition()) ppq = *q;
        }
    curBpm = bpm;
    for (int p = 0; p < kPads; ++p)
        for (int k = 0; k < kAutoN; ++k)
        {
            const float hv = hpRaw[p][k]->load();
            if (hv != lastHost[p][k].load()) { lastHost[p][k] = hv; pad[p].v[kAutoParams[k]] = hv; }
        }
    const int req = transportReq.exchange (0);
    if (req == 1) { intPlay = true; intTick = 0; }
    if (req == 2) intPlay = false;
    const bool running = hostPlay || intPlay.load();
    const double tps = bpm / 60.0 * PPQ / sr;
    const double t0 = hostPlay ? ppq * PPQ : intTick, t1 = t0 + N * tps;
    uiHost = hostPlay; uiRunning = running; uiBpm = bpm;
    // stop del trasporto: si fermano i campioni lunghi (loop, hold, gate)
    if (! running && wasRunning) for (auto& v : voices) if (v.active && (v.loop || v.hold || v.gate > 0)) v.rel = true;
    wasRunning = running;
    if (stopReq.exchange (false)) { for (auto& v : voices) if (v.active) v.choking = true; prevS = nullptr; }
    if (audReq.load() != audSeen) { audSeen = audReq.load(); const int ap = audP.load(); if (ap >= 0 && ap < kPads) { for (auto& v : voices) if (v.active && v.pad == ap) v.choking = true; startVoice (ap, 110, audS0.load(), audS1.load()); } }

    const int root = rootNote();
    const bool swMidi = par ("swMidi") > 0.5f && par ("swing") > 50.05f && running;
    juce::MidiBuffer out;
    // interfaccia: suona subito e manda le note in uscita (VST3: registrabili in Ableton)
    {
        int s1, n1, s2, n2; fifo.prepareToRead (fifo.getNumReady(), s1, n1, s2, n2);
        for (int k = 0; k < n1 + n2; ++k)
        {
            const auto& e = fifoBuf[k < n1 ? s1 + k : s2 + k - n1];
            trigs.push_back ({ 0, e.pad, e.vel, e.on });
            heldVel[e.pad] = e.on ? juce::jmax (1, e.vel) : 0; if (e.on) { nrStart[e.pad] = t0; nrPhase[e.pad] = 0; }
            out.addEvent (e.on ? juce::MidiMessage::noteOn (1, root + e.pad, (juce::uint8) juce::jlimit (1, 127, e.vel)) : juce::MidiMessage::noteOff (1, root + e.pad), 0);
        }
        fifo.finishedRead (n1 + n2);
    }
    // MIDI da Ableton, con swing in tempo reale sulle note in levare
    for (const auto m : midi)
    {
        const auto msg = m.getMessage();
        if (msg.isAllNotesOff() || msg.isAllSoundOff()) { for (auto& v : voices) if (v.active) v.choking = true; continue; }
        if (msg.isController())
        {
            if (! ctrlOn.load() && learnSlot.load() < 0) continue;   // MPK: le 8 manopole mandano CC 70-77 -> effetti da performance
            const int cc = msg.getControllerNumber();
            if (const int ls = learnSlot.load(); ls >= 0 && ls < kFx) { for (int j = 0; j < kFx; ++j) if (ccMap[j].load() == cc) ccMap[j] = -1; ccMap[ls] = cc; learnSlot = -1; ccDirty = true; }
            int k = -1; for (int j = 0; j < kFx; ++j) if (ccMap[j].load() == cc) k = j;
            if (k >= 0 && k < kFx)
            {
                float v = msg.getControllerValue() / 127.0f;
                { const int ty = (int) apvts.getRawParameterValue ("fx" + juce::String (k + 1) + "type")->load(); if (pfx::info (ty).bipolar && ty != pfx::SCRATCHHAND && std::abs (msg.getControllerValue() - 64) <= 8) v = 0.5f; }   // centro largo, facile da ritrovare   // filtro: zona morta al centro
                ccPend[k] = v;
            }
            continue;
        }
        if (! msg.isNoteOnOrOff()) continue;
        const int p = msg.getNoteNumber() - root; if (p < 0 || p >= kPads) continue;
        juce::int64 delay = 0;
        heldVel[p] = msg.isNoteOn() ? juce::jmax (1, (int) msg.getVelocity()) : 0;
        if (msg.isNoteOn()) { nrStart[p] = t0 + m.samplePosition * tps; nrPhase[p] = -m.samplePosition; }
        if (msg.isNoteOn())
        {
            if (swMidi) { const double t = t0 + m.samplePosition * tps, st = swingTick (t); if (st > t) delay = (juce::int64) ((st - t) / tps); }
            padDelay[p] = delay;
        }
        else delay = padDelay[p];
        if (delay <= 0) trigs.push_back ({ m.samplePosition, p, msg.isNoteOn() ? (int) msg.getVelocity() : 0, msg.isNoteOn() });
        else if (pend.size() < 500) pend.push_back ({ absSample + m.samplePosition + delay, p, msg.isNoteOn() ? (int) msg.getVelocity() : 0, msg.isNoteOn() });
    }
    midi.swapWith (out);
    // note con swing in attesa
    for (size_t k = 0; k < pend.size();)
    {
        if (pend[k].at < absSample + N) { trigs.push_back ({ (int) juce::jmax ((juce::int64) 0, pend[k].at - absSample), pend[k].pad, pend[k].vel, pend[k].on }); pend.erase (pend.begin() + (long) k); }
        else ++k;
    }
    absSample += N;
    // note repeat: ripete i pad tenuti a tempo, con lo swing
    if (par ("nrOn") > 0.5f)
    {
        static const double rates[] = { 96, 48, 32, 24, 16, 12, 8 };
        const int ri = juce::jlimit (0, 6, (int) par ("nrRate")); const double rt = rates[ri];
        const bool straight = ri == 1 || ri == 3;
        for (int p = 0; p < kPads; ++p)
        {
            if (heldVel[p] <= 0) continue;
            if (running)
            {
                for (double g = std::floor (t0 / rt) * rt - rt; g < t1; g += rt)
                {
                    if (g <= nrStart[p] + 0.5) continue;
                    const double a = straight ? swingTick (g) : g;
                    if (a >= t0 && a < t1) trigs.push_back ({ juce::jlimit (0, N - 1, (int) ((a - t0) / tps)), p, heldVel[p], true });
                }
            }
            else
            {
                const double step = rt / tps;
                double ph = nrPhase[p];
                for (double k = std::ceil ((ph + 1) / step) * step; k < ph + N; k += step) if (k > 0) trigs.push_back ({ juce::jlimit (0, N - 1, (int) (k - ph)), p, heldVel[p], true });
                nrPhase[p] = ph + N;
            }
        }
    }
    // loop della banca, sincronizzato
    if (running && loopOn.load())
    {
        const double L = loopTicks(); const float hum = par ("human");
        const juce::SpinLock::ScopedTryLockType l (recLock);
        if (l.isLocked())
            for (auto& e : events)
            {
                if (seqMuted (e.pad)) continue;
                const double q = swingTick (e.tick), base = std::floor (t0 / L) * L;
                for (double b = base; b <= t1; b += L)
                {
                    const double a = b + q; if (a < t0 || a >= t1) continue;
                    int vel = e.vel; if (hum > 0) vel = juce::jlimit (1, 127, vel + juce::roundToInt ((rng.nextFloat() - 0.5f) * 40 * hum));
                    trigs.push_back ({ juce::jlimit (0, N - 1, (int) ((a - t0) / tps)), e.pad, vel, true });
                }
            }
    }
    std::stable_sort (trigs.begin(), trigs.end(), [] (const Trig& a, const Trig& b) { return a.off < b.off; });
    if (! hostPlay && intPlay.load()) intTick += N * tps;
    uiTick = running ? t1 : 0.0;

    // i campioni sincronizzati seguono il tempo della DAW anche se cambia mentre suonano
    // e intonazione, gain e filtro seguono i parametri (automazioni di Ableton) mentre suonano
    for (auto& v : voices)
    {
        if (! v.active || ! v.s) continue;
        const auto& P = pad[v.pad];
        v.pitch = std::pow (2.0, (P.get (TUNE) + P.get (FINE) / 100.0 + v.velSemis) / 12.0) * v.s->sr / sr;
        const double tempo = (v.warp > 0 && v.sbpm > 40) ? curBpm / v.sbpm : 1.0;
        if (v.warp == 2) { v.tRate = tempo * v.s->sr / sr; v.inc = v.tRate; v.pRate = v.pitch; }
        else v.inc = v.pitch * (v.warp == 1 ? tempo : 1.0);
        v.gain = 0.708f * juce::Decibels::decibelsToGain (P.get (GAIN) + v.velDb);
        v.cutBase = juce::jlimit (30.0f, 20000.0f, P.get (CUTOFF) * v.velCut); v.reso = P.get (RESO); v.fk = 2.0f - 1.94f * P.get (RESO);
        const bool wantF = ! (v.ftype == 0 && v.cutBase >= 19500.0f && std::abs (v.fenvAmt) < 0.01f);
        if (wantF && ! v.filt) for (int c = 0; c < 2; ++c) { v.ic1[c] = 0; v.ic2[c] = 0; }
        v.filt = wantF;
        if (std::abs (v.fenvAmt) <= 0.005f) v.fg = (float) std::tan (juce::MathConstants<double>::pi * juce::jmin ((double) v.cutBase, sr * 0.45) / sr);
    }
    // 1) voci nei bus dei pad
    for (int p = 0; p < kPads; ++p) if (chain[p].active > 0) { padBuf.clear (p * 2, 0, N); padBuf.clear (p * 2 + 1, 0, N); }
    size_t ti = 0;
    float* cl = buffer.getWritePointer (0); float* cr = buffer.getWritePointer (1);
    for (int i = 0; i < N; ++i)
    {
        while (ti < trigs.size() && trigs[ti].off <= i)
        {
            const auto& t = trigs[ti++];
            if (t.on)
            {
                if (chain[t.pad].active <= 0) { padBuf.clear (t.pad * 2, 0, N); padBuf.clear (t.pad * 2 + 1, 0, N); }
                startVoice (t.pad, t.vel);
            }
            else releasePad (t.pad);
        }
        for (auto& v : voices) if (v.active) render (v, padBuf.getWritePointer (v.pad * 2), padBuf.getWritePointer (v.pad * 2 + 1), i);
    }
    // dove sta suonando ogni pad (la voce piu recente, non i layer)
    {
        int best[kPads]; float pos[kPads];
        for (int p = 0; p < kPads; ++p) { best[p] = 1 << 30; pos[p] = -1.0f; }
        for (auto& v : voices)
            if (v.active && ! v.layer && v.s && v.age < best[v.pad])
            { const int n = v.s->buf.getNumSamples(); best[v.pad] = v.age; pos[v.pad] = n > 0 ? (float) (v.pos / n) : -1.0f; }
        for (int p = 0; p < kPads; ++p) padPos[p] = pos[p];
    }
    // 2) catena di ogni pad, fader, mandate
    std::fill (revL.begin(), revL.begin() + N, 0.0f); std::fill (revR.begin(), revR.begin() + N, 0.0f);
    std::fill (dlyInL.begin(), dlyInL.begin() + N, 0.0f); std::fill (dlyInR.begin(), dlyInR.begin() + N, 0.0f);
    bool anySolo = false; for (int p = 0; p < kPads; ++p) if (pad[p].get (SOLO) > 0.5f) anySolo = true;
    for (int p = 0; p < kPads; ++p)
    {
        auto& C = chain[p]; auto& P = pad[p];
        if (C.active <= 0) { padMeter[p] = padMeter[p].load() * 0.8f; padGr[p] = 0; continue; }
        C.active -= N;
        float* L = padBuf.getWritePointer (p * 2); float* R = padBuf.getWritePointer (p * 2 + 1);
        float pk = 0; for (int i = 0; i < N; ++i) pk = juce::jmax (pk, std::abs (L[i]) + std::abs (R[i]));
        if (pk > padGlow[p].load()) padGlow[p] = pk;
        if (pk > 1e-5f) C.active = juce::jmax (C.active, (int) (sr * 0.5));
        // EQ a 6 bande (i tagli 24 dB usano due stadi)
        if (P.get (EQON) > 0.5f)
        {
            for (int bd = 0; bd < 6; ++bd)
            {
                if (P.get (bandP (bd, 0)) < 0.5f) continue;
                const int t = juce::roundToInt (P.get (bandP (bd, 1)));
                const float f = P.get (bandP (bd, 2)), gdb = P.get (bandP (bd, 3)), q = P.get (bandP (bd, 4));
                const bool isCut = t == EQ_LC12 || t == EQ_LC24 || t == EQ_HC12 || t == EQ_HC24;
                if (! isCut && t != EQ_NOTCH && std::abs (gdb) < 0.05f) continue;
                auto& lb = C.lastBand[bd];
                if (lb[0] != (float) t || lb[1] != f || lb[2] != gdb || lb[3] != q)
                {
                    lb[0] = (float) t; lb[1] = f; lb[2] = gdb; lb[3] = q;
                    const int rt = t == EQ_LSHELF ? 0 : t == EQ_BELL ? 1 : t == EQ_HSHELF ? 2 : t == EQ_NOTCH ? 5 : (t == EQ_LC12 || t == EQ_LC24) ? 4 : 3;
                    C.eq[bd][0].set (rt, sr, f, gdb, isCut ? q : (t == EQ_NOTCH ? juce::jmax (q, 0.5f) * 2 : q));
                    C.eq[bd][1].set (rt, sr, f, gdb, 0.54);
                }
                const int st = (t == EQ_LC24 || t == EQ_HC24) ? 2 : 1;
                for (int k = 0; k < st; ++k) for (int i = 0; i < N; ++i) { L[i] = C.eq[bd][k].run (L[i], 0); R[i] = C.eq[bd][k].run (R[i], 1); }
            }
        }
        // SOLO di una banda: si sente solo la zona che la banda tocca
        if (eqSoloPad.load() == p && eqSoloBand.load() >= 0)
        {
            const int bd = eqSoloBand.load(), t = juce::roundToInt (P.get (bandP (bd, 1))); const float f = P.get (bandP (bd, 2));
            const int ft = (t == EQ_LSHELF || t == EQ_HC12 || t == EQ_HC24) ? 3 : (t == EQ_HSHELF || t == EQ_LC12 || t == EQ_LC24) ? 4 : 6;
            const float q = ft == 6 ? juce::jmax (0.8f, P.get (bandP (bd, 4))) : 0.707f;
            if (C.solKey[0] != (float) ft || C.solKey[1] != f || C.solKey[2] != q) { C.solKey[0] = (float) ft; C.solKey[1] = f; C.solKey[2] = q; for (auto& bq : C.sol) bq.set (ft, sr, f, 0, q); }
            for (int i = 0; i < N; ++i) { L[i] = C.sol[1].run (C.sol[0].run (L[i], 0), 0); R[i] = C.sol[1].run (C.sol[0].run (R[i], 1), 1); }
        }
        // compressore
        const int cm = juce::roundToInt (P.get (CMODE));
        if (cm > 0)
        {
            struct CT { float ratio, att, rel, knee; };
            static const CT T[] = { { 1, 1, 1, 0 }, { 2.0f, 15, 160, 10 }, { 4.0f, 5, 100, 6 }, { 10.0f, 0.4f, 60, 1 } };
            const auto& t = T[cm];
            const float thr = -P.get (CAMT) * 36.0f, mk = juce::Decibels::decibelsToGain (P.get (CMAKEUP) + P.get (CAMT) * 36.0f * (1 - 1 / t.ratio) * 0.25f), mix = P.get (CMIX);
            const float aA = (float) std::exp (-1.0 / (t.att * 0.001 * sr)), aR = (float) std::exp (-1.0 / (t.rel * 0.001 * sr));
            float maxGr = 0;
            for (int i = 0; i < N; ++i)
            {
                const float x = juce::jmax (std::abs (L[i]), std::abs (R[i]));
                C.cEnv = x > C.cEnv ? aA * C.cEnv + (1 - aA) * x : aR * C.cEnv + (1 - aR) * x;
                const float lev = juce::Decibels::gainToDecibels (C.cEnv + 1e-7f), over = lev - thr;
                float gr = 0;
                if (over > t.knee * 0.5f) gr = over * (1 - 1 / t.ratio);
                else if (over > -t.knee * 0.5f) { const float o = over + t.knee * 0.5f; gr = (1 - 1 / t.ratio) * o * o / (2 * t.knee); }
                maxGr = juce::jmax (maxGr, gr);
                const float g = juce::Decibels::decibelsToGain (-gr) * mk;
                L[i] = L[i] * (1 - mix) + L[i] * g * mix; R[i] = R[i] * (1 - mix) + R[i] * g * mix;
            }
            padGr[p] = maxGr;
        }
        else padGr[p] = 0;
        // drive e lo-fi
        const float dr = P.get (DRIVE), bits = P.get (BITS), rate = P.get (RATE);
        if (dr > 0.001f)
        {
            const int dt = juce::roundToInt (P.get (DTYPE)); const float k = 1 + dr * dr * 30;
            if (dr != C.lastDrv || dt != C.lastDType)
            {   // stesso volume di una sinusoide a -6 dBFS prima e dopo
                C.lastDrv = dr; C.lastDType = dt; double e = 0;
                for (int j = 0; j < 64; ++j) { const float y = driveShape (dt, k * 0.5f * std::sin (6.2831853f * j / 64)); e += y * y; }
                C.drvNorm = 0.3536f / (float) std::sqrt (e / 64 + 1e-12);
            }
            // overdrive, distorsione e fuzz: bassi tagliati prima, alti ammorbiditi dopo (come un pedale)
            const float hpA = dt == 4 ? (float) std::exp (-6.2831853 * 350 / sr) : dt >= 5 ? (float) std::exp (-6.2831853 * 120 / sr) : 1.0f;
            const float lpA = dt == 1 ? (float) std::exp (-6.2831853 * 11000 / sr) : dt == 4 ? (float) std::exp (-6.2831853 * 6000 / sr) : dt == 5 ? (float) std::exp (-6.2831853 * 4500 / sr) : dt == 6 ? (float) std::exp (-6.2831853 * 3800 / sr) : 0.0f;
            float* io[2] = { L, R };
            for (int c = 0; c < 2; ++c)
                for (int i = 0; i < N; ++i)
                {
                    float x = io[c][i];
                    if (hpA < 1.0f) { const float y = hpA * (C.dHp[c] + x - C.dHpX[c]); C.dHpX[c] = x; C.dHp[c] = y; x = y; }
                    float y = driveShape (dt, x * k) * C.drvNorm;
                    if (lpA > 0) { C.dLp[c] = y + lpA * (C.dLp[c] - y); y = C.dLp[c]; }
                    io[c][i] = y;
                }
        }
        if (bits > 0.001f || rate > 0.001f)
        {
            const float q = std::pow (2.0f, 15.0f - bits * 11.0f); const double step = 1.0 - rate * 0.88;
            for (int i = 0; i < N; ++i)
            {
                C.holdPh += step; if (C.holdPh >= 1.0) { C.holdPh -= 1.0; C.holdL = L[i]; C.holdR = R[i]; }
                L[i] = std::round (C.holdL * q) / q; R[i] = std::round (C.holdR * q) / q;
            }
        }
        // fader, pan, mute/solo
        const bool audible = P.get (MUTE) < 0.5f && (! anySolo || P.get (SOLO) > 0.5f) && (eqSoloPad.load() < 0 || eqSoloBand.load() < 0 || eqSoloPad.load() == p);
        const float lv = audible ? P.get (LEVEL) : 0.0f, pan = P.get (PAN);
        const float gl = lv * std::cos ((pan + 1) * juce::MathConstants<float>::pi * 0.25f) * 1.41421f, gr2 = lv * std::sin ((pan + 1) * juce::MathConstants<float>::pi * 0.25f) * 1.41421f;
        const float sRev = P.get (SREV), sDly = P.get (SDLY);
        float mpk = 0;
        for (int i = 0; i < N; ++i)
        {
            const float l = L[i] * gl, r = R[i] * gr2;
            cl[i] += l; cr[i] += r; mpk = juce::jmax (mpk, juce::jmax (std::abs (l), std::abs (r)));
            if (sRev > 0) { revL[(size_t) i] += l * sRev; revR[(size_t) i] += r * sRev; }
            if (sDly > 0) { dlyInL[(size_t) i] += l * sDly; dlyInR[(size_t) i] += r * sDly; }
        }
        padMeter[p] = juce::jmax (mpk, padMeter[p].load() * 0.8f);
    }
    // 3) master: delay, riverbero, bus comp, saturazione, clip, uscita
    {
        {   // mandate del kit intero (oltre a quelle dei singoli pad)
            const float rk = par ("revKit"), dk = par ("dlyKit");
            if (rk > 0.0f || dk > 0.0f)
                for (int i = 0; i < N; ++i) { revL[(size_t) i] += cl[i] * rk; revR[(size_t) i] += cr[i] * rk; dlyInL[(size_t) i] += cl[i] * dk; dlyInR[(size_t) i] += cr[i] * dk; }
        }
        const int len = dlyBuf.getNumSamples();
        const int dv = juce::jlimit (0, kDly.size() - 1, (int) par ("dlyDiv"));
        const double dT = juce::jlimit (16.0, (double) len - 4, dv == kDly.size() - 1 ? par ("dlyMs") * 0.001 * sr : kDlyBeats[dv] * 60.0 / bpm * sr);
        const float dHpA = (float) std::exp (-juce::MathConstants<double>::twoPi * par ("dlyLo") / sr), dLpA = (float) std::exp (-juce::MathConstants<double>::twoPi * par ("dlyHi") / sr);
        const float fb = par ("dlyFb"), ret = par ("dlyRet");
        for (int i = 0; i < N; ++i)
        {
            double rp = dlyW - dT; if (rp < 0) rp += len;
            const int i0 = (int) rp; const float fr = (float) (rp - i0); const int i1 = (i0 + 1) % len;
            const float yL = dlyBuf.getSample (0, i0) * (1 - fr) + dlyBuf.getSample (0, i1) * fr, yR = dlyBuf.getSample (1, i0) * (1 - fr) + dlyBuf.getSample (1, i1) * fr;
            // ripetizioni a ping pong, filtrate: taglio bassi e taglio alti nel giro di feedback
            const float in2[2] = { yR, yL }; float o[2];
            for (int c = 0; c < 2; ++c)
            {
                const float h = dHpA * (dlHp[c] + in2[c] - dlHpX[c]); dlHpX[c] = in2[c]; dlHp[c] = h;
                dlLp[c] = h + dLpA * (dlLp[c] - h); o[c] = dlLp[c];
            }
            dlyLpL = o[0]; dlyLpR = o[1];
            dlyBuf.setSample (0, dlyW, dlyInL[(size_t) i] + std::tanh (dlyLpL * fb)); dlyBuf.setSample (1, dlyW, dlyInR[(size_t) i] + std::tanh (dlyLpR * fb));
            if (++dlyW >= len) dlyW = 0;
            cl[i] += yL * ret; cr[i] += yR * ret;
        }
        // riverbero: pre-delay, taglio bassi e alti sull'ingresso, coda in secondi
        {
            const int pre = juce::jlimit (0, (int) preBuf.size() / 2 - 2, (int) (par ("revPre") * 0.001 * sr)); const int pn = (int) preBuf.size() / 2;
            const float hA = (float) std::exp (-juce::MathConstants<double>::twoPi * par ("revLo") / sr);
            rvLp.set (3, sr, par ("revHi"), 0, 0.707);
            for (int i = 0; i < N; ++i)
            {
                float io2[2] = { revL[(size_t) i], revR[(size_t) i] };
                for (int c = 0; c < 2; ++c)
                {
                    const float h = hA * (rvHpZ[c] + io2[c] - rvHpX[c]); rvHpX[c] = io2[c]; rvHpZ[c] = h;
                    preBuf[(size_t) (c * pn + preW)] = rvLp.run (h, c);
                    io2[c] = preBuf[(size_t) (c * pn + (preW - pre + pn) % pn)];
                }
                if (++preW >= pn) preW = 0;
                revL[(size_t) i] = io2[0]; revR[(size_t) i] = io2[1];
            }
        }
        juce::Reverb::Parameters rp; rp.roomSize = 0.3f + 0.68f * par ("revSize"); rp.damping = par ("revDamp"); rp.wetLevel = 1; rp.dryLevel = 0; rp.width = 1;
        reverb.setParameters (rp); reverb.processStereo (revL.data(), revR.data(), N);
        const float rr = par ("revRet") * 0.6f;
        for (int i = 0; i < N; ++i) { cl[i] += revL[(size_t) i] * rr; cr[i] += revR[(size_t) i] * rr; }
        const int bm = (int) par ("busMode");
        float maxGr = 0;
        if (bm > 0)
        {
            struct BT { float ratio, att, rel; };
            static const BT T[] = { { 1, 1, 1 }, { 2.0f, 10, 200 }, { 4.0f, 20, 90 }, { 12.0f, 0.5f, 60 } };
            const auto& t = T[bm]; const float thr = -par ("busAmt") * 30.0f, mk = juce::Decibels::decibelsToGain (par ("busAmt") * 30.0f * (1 - 1 / t.ratio) * 0.4f);
            const float aA = (float) std::exp (-1.0 / (t.att * 0.001 * sr)), aR = (float) std::exp (-1.0 / (t.rel * 0.001 * sr));
            for (int i = 0; i < N; ++i)
            {
                const float x = juce::jmax (std::abs (cl[i]), std::abs (cr[i]));
                busEnv = x > busEnv ? aA * busEnv + (1 - aA) * x : aR * busEnv + (1 - aR) * x;
                const float lev = juce::Decibels::gainToDecibels (busEnv + 1e-7f), gr = lev > thr ? (lev - thr) * (1 - 1 / t.ratio) : 0.0f;
                maxGr = juce::jmax (maxGr, gr); const float g = juce::Decibels::decibelsToGain (-gr) * mk; cl[i] *= g; cr[i] *= g;
            }
        }
        busGr = maxGr;
        // saturazione del kit: morbida, a guadagno unitario sui livelli bassi
        if (const float sat = par ("satAmt"); sat > 0.001f)
        {
            // spinta fino a +12 dB nella tanh, guadagno sui bassi livelli da 0 a +3 dB, un filo di pari (polarizzazione)
            const float k = 1.0f + sat * 3.0f, mk = (1.0f + 0.41f * sat) / k, bias = sat * 0.06f, off = std::tanh (bias * k);
            for (int i = 0; i < N; ++i)
            {
                cl[i] = (std::tanh ((cl[i] + bias) * k) - off) * mk; cr[i] = (std::tanh ((cr[i] + bias) * k) - off) * mk;
                // blocco della continua introdotta dalla polarizzazione
                satZ += 0.0005f * (0.5f * (cl[i] + cr[i]) - satZ); cl[i] -= satZ; cr[i] -= satZ;
            }
        }
        {   // effetti da performance (8 slot)
            int ty[8]; float am[8];
            for (int k = 0; k < 8; ++k)
            {
                ty[k] = (int) fxTypeRaw[k]->load();
                const float pnd = ccPend[k].load(); am[k] = pnd >= 0 ? pnd : fxAmtRaw[k]->load();
            }
            perf.process (cl, cr, N, ty, am, bpm, hostPlay ? ppq : t0 / PPQ, running);
        }
        // ascolto di prova dalla banca
        if (prevReq.load() != prevSeen)
        {
            prevSeen = prevReq.load(); const juce::SpinLock::ScopedTryLockType l (sLock);
            if (l.isLocked()) { prevS = std::move (prevNext); prevNext = nullptr; prevPos = 0; } else prevSeen = -1;
        }
        if (prevS)
        {
            const auto& pb = prevS->buf; const int pn = pb.getNumSamples(); const double inc = prevS->sr / sr, maxLen = juce::jmin ((double) pn, prevS->sr * 12.0);
            const float* p0 = pb.getReadPointer (0); const float* p1 = pb.getReadPointer (juce::jmin (1, pb.getNumChannels() - 1));
            for (int i = 0; i < N; ++i)
            {
                if (prevPos >= maxLen - 2) { prevS = nullptr; break; }
                const int k = (int) prevPos; const float f = (float) (prevPos - k);
                const float fade = (float) juce::jmin (1.0, (maxLen - 2 - prevPos) / (prevS->sr * 0.05)) * 0.5f;
                cl[i] += (p0[k] + (p0[k + 1] - p0[k]) * f) * fade; cr[i] += (p1[k] + (p1[k + 1] - p1[k]) * f) * fade;
                prevPos += inc;
            }
        }
        const bool clip = par ("clip") > 0.5f; const float og = juce::Decibels::decibelsToGain (par ("out"));
        float pk[2] = { 0, 0 };
        for (int i = 0; i < N; ++i)
        {
            float l = cl[i] * og, r = cr[i] * og;
            if (clip) { auto sc = [] (float x) { const float a = std::abs (x); return a > 0.85f ? std::copysign (0.85f + 0.15f * std::tanh ((a - 0.85f) / 0.15f), x) : x; }; l = sc (l); r = sc (r); }
            cl[i] = l; cr[i] = r; pk[0] = juce::jmax (pk[0], std::abs (l)); pk[1] = juce::jmax (pk[1], std::abs (r));
            { const float kl = kw2.run (kw1.run (l, 0), 0), kr = kw2.run (kw1.run (r, 1), 1); lufsAcc += (double) kl * kl + (double) kr * kr; }
            if (++lufsN >= (int) (sr * 0.1))
            {   // short-term: media degli ultimi 3 s in blocchi da 100 ms
                lufsBins[lufsBi] = (float) (lufsAcc / lufsN); lufsBi = (lufsBi + 1) % 30; lufsAcc = 0; lufsN = 0;
                double m = 0; for (float b : lufsBins) m += b; m /= 30.0;
                lufsS = m > 1e-10 ? (float) (-0.691 + 10.0 * std::log10 (m)) : -100.0f;
            }
            colPk = juce::jmax (colPk, juce::jmax (std::abs (l), std::abs (r)));
            if (++colN >= (int) (sr * 3.0 / kScope))
            {
                const int w = scW.load(); scPk[w] = colPk; scHit[w] = colHits; scVel[w] = colVel; scW = (w + 1) % kScope;
                colPk = 0; colN = 0; colHits = 0; colVel = 0;
            }
        }
        meterL = juce::jmax (pk[0], meterL.load() * 0.85f); meterR = juce::jmax (pk[1], meterR.load() * 0.85f);
        peakHold = juce::jmax (peakHold.load(), juce::jmax (pk[0], pk[1]));
    }
}

// ---------------- campioni e BPM ----------------
float Processor::estimateBpm (const juce::String& name, double sec, bool& confident)
{
    confident = false;
    auto toks = juce::StringArray::fromTokens (name.toLowerCase().replaceCharacters ("_-.()[],+", "         "), " ", "");
    toks.removeEmptyStrings();
    // 1) numero marcato: "140bpm", "140 bpm", "bpm140", "bpm 140", "140_BPM"
    for (int i = 0; i < toks.size(); ++i)
    {
        auto t = toks[i]; juce::String num;
        if (t.endsWith ("bpm") && t.length() > 3) num = t.dropLastCharacters (3);
        else if (t.startsWith ("bpm") && t.length() > 3) num = t.substring (3);
        else if (i + 1 < toks.size() && toks[i + 1] == "bpm") num = t;
        else if (t == "bpm" && i + 1 < toks.size()) num = toks[i + 1];
        if (num.isNotEmpty() && num.containsOnly ("0123456789.")) { const float v = num.getFloatValue(); if (v >= 50 && v <= 220) { confident = true; return v; } }
    }
    // 2) numero nudo plausibile (60-200): se e uno solo ci si fida; se sono di piu vince chi torna con la durata
    juce::Array<int> cand;
    for (int i = 0; i < toks.size(); ++i)
    {
        const auto& t = toks[i];
        if (! t.containsOnly ("0123456789") || t.isEmpty() || t.length() > 3) continue;
        const int v = t.getIntValue(); if (v < 60 || v > 200 || v == 808 || v == 909) continue;
        if (i > 0 && (toks[i - 1] == "vol" || toks[i - 1] == "take" || toks[i - 1] == "v" || toks[i - 1] == "no")) continue;
        cand.add (v);
    }
    if (sec >= 1.2 && cand.size() > 0)
    {
        int best = cand.getLast(); double bestErr = 1e9;
        for (int v : cand)
        {
            const double beats = sec * v / 60.0, m = juce::jmax (1.0, std::round (beats));
            const double err = std::abs (beats - m) / m;
            if (err < bestErr) { bestErr = err; best = v; }
        }
        if (cand.size() == 1 || bestErr < 0.05) { confident = true; return (float) best; }
    }
    if (sec < 1.2) return 0.0f;
    float best = 0; double bestScore = 1e9;
    for (double beats : { 4.0, 8.0, 16.0, 32.0, 64.0, 2.0 })
    {
        const double b = beats * 60.0 / sec; if (b < 70 || b > 180) continue;
        const double score = std::abs (std::log (b / 115.0)) + std::abs (b - std::round (b)) * 0.5;
        if (score < bestScore) { bestScore = score; best = (float) b; }
    }
    if (best > 0 && std::abs (best - std::round (best)) < 0.06f) { confident = true; best = std::round (best); }
    return best;
}

// tempo dall'audio: autocorrelazione del flusso di energia, con preferenza per durate a battute intere
float Processor::estimateBpmAudio (const juce::AudioBuffer<float>& b, double sampleRate, double& strength)
{
    strength = 0;
    const int n = b.getNumSamples(); const double sec = n / sampleRate; if (sec < 1.5) return 0;
    const int hop = juce::jmax (64, (int) (sampleRate / 200.0));   // 200 fotogrammi al secondo
    const float* d0 = b.getReadPointer (0); const float* d1 = b.getReadPointer (juce::jmin (1, b.getNumChannels() - 1));
    std::vector<float> fl; float prev = 0;
    for (int i = 0; i + hop <= n; i += hop) { float e = 0; for (int k = 0; k < hop; ++k) { const float x = 0.5f * (d0[i + k] + d1[i + k]); e += x * x; } e = std::log (1e-6f + e); fl.push_back (juce::jmax (0.0f, e - prev)); prev = e; }
    const double fps = sampleRate / hop; const int F = (int) fl.size(); if (F < 64) return 0;
    double mean = 0; for (float f : fl) mean += f; mean /= F; for (auto& f : fl) f = (float) (f - mean);
    double e0 = 0; for (float f : fl) e0 += f * f; if (e0 <= 0) return 0;
    float best = 0; double bestS = -1;
    for (double bpm = 70; bpm <= 180; bpm += 0.5)
    {
        const double lag = fps * 60.0 / bpm; double acc = 0;
        for (int m = 1; m <= 4; ++m)
        {
            const double L = lag * m; const int l0 = (int) L; const float fr = (float) (L - l0); double c = 0;
            for (int i = 0; i + l0 + 1 < F; ++i) c += fl[(size_t) i] * (fl[(size_t) (i + l0)] * (1 - fr) + fl[(size_t) (i + l0 + 1)] * fr);
            acc += c / e0 / m;
        }
        // durata a battute intere: premio forte
        const double bars = sec * bpm / 240.0, near = std::abs (bars - std::round (bars * 2) / 2);
        acc *= (near < 0.03 ? 1.35 : 1.0);
        acc *= 1.0 - 0.15 * std::abs (std::log (bpm / 110.0));   // preferenza leggera per tempi medi
        if (acc > bestS) { bestS = acc; best = (float) bpm; }
    }
    strength = bestS;
    // se la durata a battute intere da un tempo vicino, aggancialo
    for (double beats : { 4.0, 8.0, 16.0, 32.0, 64.0 }) { const double bb = beats * 60.0 / sec; if (std::abs (bb - best) < 1.2) { best = (float) bb; break; } }
    return std::abs (best - std::round (best)) < 0.15f ? std::round (best) : best;
}

void Processor::detectBpm (int p)
{
    auto s = getSample (p); if (! s) return;
    const double sec = s->buf.getNumSamples() / s->sr;
    bool conf = false; float b = estimateBpm (s->name, sec, conf);
    if (! conf && sec >= 1.5) { double st = 0; const float ba = estimateBpmAudio (s->buf, s->sr, st); if (ba > 0 && st > 0.15) { b = ba; conf = true; } else if (b <= 0) b = ba; }
    pad[p].v[SBPM] = b;
    // i campioni lunghi (loop, frasi) vanno a tempo con la DAW e si fermano quando lasci il pad
    if (b > 0 && sec >= 1.8) { pad[p].v[WARP] = 2; pad[p].v[DECAY] = 4000; pad[p].v[MODE] = 1; }
    else if (sec >= 1.8) { pad[p].v[MODE] = 1; pad[p].v[DECAY] = 4000; }
}

void Processor::detectKey (int p, bool audio)
{
    auto s = getSample (p); if (! s) { pad[p].v[SKEY] = -1; return; }
    int k = keyFromName (s->name);
    if (k < 0 && s->path.isNotEmpty() && ! s->path.startsWith ("gen:") && ! s->path.startsWith ("builtin:"))
        k = keyFromName (juce::File (s->path).getParentDirectory().getFileName());   // cartella "Loops in Am"
    if (k < 0 && audio) { float conf = 0; k = keyFromAudio (s->buf, s->sr, conf); if (conf < (k >= 24 ? 0.26f : 0.3f)) k = -1; }
    pad[p].v[SKEY] = (float) (k < 0 ? -2 : k);
}

void Processor::transposeTo (int p, int key)
{
    auto& P = pad[p]; const int from = juce::roundToInt (P.get (SKEY));
    if (from < 0 || key < 0) return;
    P.v[TUNE] = (float) keyShift (from, key);
    // un loop a tempo cambia solo l'intonazione, non la velocita
    auto s = getSample (p);
    if (s && P.get (SBPM) > 40 && s->buf.getNumSamples() / s->sr >= 1.8 && juce::roundToInt (P.get (WARP)) != 2) P.v[WARP] = 2;
}

bool Processor::isTonalPad (int p) const
{
    const auto r = padRole (*this, p);
    return r == R_808 || r == R_MEL || r == R_LOOP || r == R_VOX || r == R_TEX || r == R_CHOP;
}
void Processor::applyKeyLock (int p)
{
    const int k = keyLock(); if (k < 0 || getSample (p) == nullptr || ! isTonalPad (p)) return;
    if (juce::roundToInt (pad[p].get (SKEY)) == -1) detectKey (p);
    if (pad[p].get (SKEY) >= 0) transposeTo (p, k);
}
void Processor::applyKeyLockAll() { for (int p = 0; p < kPads; ++p) applyKeyLock (p); }
int Processor::kitEffectiveKey (int except) const
{
    if (keyLock() >= 0) return keyLock();
    for (int p : { 9, 10, 11, 12, 13, 14, 15, 8, 0, 1, 2, 3, 4, 5, 6, 7 })
    {
        if (p == except || getSample (p) == nullptr || ! isTonalPad (p)) continue;
        const int k = juce::roundToInt (pad[p].get (SKEY)); if (k < 0 || k >= 24) continue;
        const int t = juce::roundToInt (pad[p].get (TUNE));
        return (k >= 12 ? 12 : 0) + (((k % 12) + t) % 12 + 12) % 12;
    }
    return -1;
}

Processor::Snap Processor::capture() const
{
    Snap n;
    for (int p = 0; p < kPads; ++p) { for (int i = 0; i < kNumPadParams; ++i) n.v[p][i] = pad[p].get (i); n.col[p] = pad[p].colour.load(); n.s[p] = getSample (p); n.s2[p] = getLayer (p); }
    n.kit = kitName; return n;
}
void Processor::restore (const Snap& n)
{
    for (int p = 0; p < kPads; ++p) { for (int i = 0; i < kNumPadParams; ++i) pad[p].v[i] = n.v[p][i]; pad[p].colour = n.col[p]; setSample (p, n.s[p]); setLayer (p, n.s2[p]); }
    kitName = n.kit;
}
bool Processor::same (const Snap& a, const Snap& b)
{
    if (a.kit != b.kit) return false;
    for (int p = 0; p < kPads; ++p)
    {
        if (a.col[p] != b.col[p] || a.s[p] != b.s[p] || a.s2[p] != b.s2[p]) return false;
        for (int i = 0; i < kNumPadParams; ++i) if (i != MUTE && i != SOLO && std::abs (a.v[p][i] - b.v[p][i]) > 1e-6f) return false;
    }
    return true;
}
void Processor::commitIfChanged()
{
    auto cur = capture();
    if (! hasCommitted) { committed = std::move (cur); hasCommitted = true; return; }
    if (same (cur, committed)) return;
    undoStack.push_back (std::move (committed)); if (undoStack.size() > 80) undoStack.erase (undoStack.begin());
    redoStack.clear(); committed = std::move (cur);
}
bool Processor::undo()
{
    commitIfChanged();
    if (undoStack.empty()) return false;
    redoStack.push_back (std::move (committed)); committed = std::move (undoStack.back()); undoStack.pop_back();
    restore (committed); return true;
}
bool Processor::redo()
{
    commitIfChanged();
    if (redoStack.empty()) return false;
    undoStack.push_back (std::move (committed)); committed = std::move (redoStack.back()); redoStack.pop_back();
    restore (committed); return true;
}

bool Processor::loadFile (const juce::File& f, int p, bool analyse)
{
    std::unique_ptr<juce::AudioFormatReader> rd (formats.createReaderFor (f));
    if (rd == nullptr || rd->lengthInSamples < 8) return false;
    const int len = (int) std::min<long long> ((long long) rd->lengthInSamples, (long long) (rd->sampleRate * 60));
    auto s = std::make_shared<SampleData>();
    s->buf.setSize (2, len); rd->read (&s->buf, 0, len, 0, true, true);
    if (rd->numChannels == 1) s->buf.copyFrom (1, 0, s->buf, 0, 0, len);
    s->sr = rd->sampleRate; s->name = f.getFileNameWithoutExtension(); s->path = f.getFullPathName();
    padCat[p] = {};
    pad[p].v[START] = 0; pad[p].v[END] = 1; pad[p].v[WARP] = 0; pad[p].v[SBPM] = 0; pad[p].v[LOOP] = 0;
    setSample (p, s);
    if (analyse) { detectBpm (p); detectKey (p); applyKeyLock (p); }
    return true;
}

// ---------------- kit ----------------
juce::File Processor::baseFolder()
{
    const auto music = juce::File::getSpecialLocation (juce::File::userMusicDirectory);
    auto nu = music.getChildFile ("Jackpot Sampler"), old = music.getChildFile ("Jackpot Pad");
    if (! nu.exists() && old.isDirectory()) old.moveFileTo (nu);
    nu.createDirectory(); return nu;
}
juce::File Processor::kitFolder()
{
    auto d = baseFolder().getChildFile ("Kits");
    d.createDirectory(); return d;
}
juce::Array<juce::File> Processor::userKits() const
{
    auto a = kitFolder().findChildFiles (juce::File::findFiles, false, "*.jpkit");
    std::sort (a.begin(), a.end(), [] (const juce::File& x, const juce::File& y) { return x.getFileName().compareIgnoreCase (y.getFileName()) < 0; });
    return a;
}
static SamplePtr sampleFromPath (Processor& P, const juce::String& path, int p)
{
    DrumDesc d;
    if (pathToDesc (path, d)) return makeDrum (d);
    if (path.startsWith ("builtin:")) return makeBuiltin (path.fromFirstOccurrenceOf (":", false, false).getIntValue());
    if (path.isNotEmpty() && juce::File (path).existsAsFile())
    {
        float keep[kNumPadParams]; for (int i = 0; i < kNumPadParams; ++i) keep[i] = P.pad[p].get (i);
        P.loadFile (juce::File (path), p, false);
        for (int i = 0; i < kNumPadParams; ++i) P.pad[p].v[i] = keep[i];
        return P.getSample (p);
    }
    return nullptr;
}
static SamplePtr readSampleFile (Processor& P, const juce::File& f)
{
    std::unique_ptr<juce::AudioFormatReader> rd (P.formats.createReaderFor (f));
    if (rd == nullptr || rd->lengthInSamples < 8) return nullptr;
    const int len = (int) std::min<long long> ((long long) rd->lengthInSamples, (long long) (rd->sampleRate * 60));
    auto s = std::make_shared<SampleData>();
    s->buf.setSize (2, len); rd->read (&s->buf, 0, len, 0, true, true);
    if (rd->numChannels == 1) s->buf.copyFrom (1, 0, s->buf, 0, 0, len);
    s->sr = rd->sampleRate; s->name = f.getFileNameWithoutExtension(); s->path = f.getFullPathName();
    return s;
}
static SamplePtr layerFromPath (Processor& P, const juce::String& path)
{
    if (path.isEmpty()) return nullptr;
    DrumDesc d;
    if (pathToDesc (path, d)) return makeDrum (d);
    if (path.startsWith ("builtin:")) return makeBuiltin (path.fromFirstOccurrenceOf (":", false, false).getIntValue());
    if (juce::File (path).existsAsFile()) return readSampleFile (P, juce::File (path));
    return nullptr;
}
bool Processor::loadLayerFile (const juce::File& f, int p)
{
    auto s = readSampleFile (*this, f); if (s == nullptr) return false;
    setLayer (p, s); return true;
}
std::unique_ptr<juce::XmlElement> Processor::padsToXml() const
{
    auto x = std::make_unique<juce::XmlElement> ("PADS");
    for (int p = 0; p < kPads; ++p)
    {
        auto* e = x->createNewChildElement ("P");
        for (int i = 0; i < kNumPadParams; ++i) e->setAttribute (kPInfo[i].id, pad[p].get (i));
        e->setAttribute ("colour", pad[p].colour.load());
        if (auto s = getSample (p)) e->setAttribute ("path", s->path);
        if (auto s = getLayer (p)) e->setAttribute ("path2", s->path);
        if (padCat[p].isNotEmpty()) e->setAttribute ("cat", padCat[p]);
    }
    return x;
}
void Processor::padsFromXml (const juce::XmlElement& pads)
{
    for (int p = 0; p < kPads; ++p) { setChord (p, {}); chordName[p] = {}; } progSrc = -1;
    int p = 0;
    for (auto* e : pads.getChildIterator())
    {
        if (p >= kPads) break;
        for (int i = 0; i < kNumPadParams; ++i) pad[p].v[i] = (float) e->getDoubleAttribute (kPInfo[i].id, kPInfo[i].def);
        pad[p].colour = e->getIntAttribute ("colour", p);
        const auto path = e->getStringAttribute ("path");
        if (path.isEmpty()) setSample (p, nullptr);
        else { auto s = sampleFromPath (*this, path, p); setSample (p, s); }
        setLayer (p, layerFromPath (*this, e->getStringAttribute ("path2")));
        padCat[p] = e->getStringAttribute ("cat");
        ++p;
    }
}
// disposizione fissa dei pad 1-8: cassa, rullante, hat, hat aperto, clap/rim, tom/perc, tom/perc, piatto; poi il resto
static std::vector<const KitPad*> arrangeKit (const std::vector<KitPad>& src)
{
    std::vector<const KitPad*> out (kPads, nullptr); std::vector<bool> used (src.size(), false);
    auto take = [&] (std::function<bool (const KitPad&)> ok) -> const KitPad*
    { for (size_t i = 0; i < src.size(); ++i) if (! used[i] && ok (src[i])) { used[i] = true; return &src[i]; } return nullptr; };
    auto cat = [] (const KitPad& k) { return categoryOf (k.d); };
    auto is = [&] (std::initializer_list<const char*> cs) { return [&cat, cs] (const KitPad& k) { const auto c = cat (k); for (auto* x : cs) if (c == x) return true; return false; }; };
    auto rim = [] (const KitPad& k) { return k.d.name.toLowerCase().contains ("rim"); };
    out[0] = take (is ({ "KICK" }));
    out[1] = take (is ({ "SNARE" }));
    out[2] = take (is ({ "HAT" }));
    out[3] = take (is ({ "OPEN HAT" })); if (! out[3]) out[3] = take (is ({ "HAT", "CYMBAL" }));
    out[4] = take (is ({ "CLAP / SNAP" })); if (! out[4]) out[4] = take (rim); if (! out[4]) out[4] = take (is ({ "SNARE" }));
    out[5] = take (is ({ "TOM", "PERC" }));
    out[6] = take (is ({ "TOM", "PERC" }));
    out[7] = take (is ({ "CYMBAL" }));
    // seconda meta: prima 808 e melodici, poi tutto il resto nell'ordine del kit
    int w = 8;
    for (auto c : { "808", "MELODICI" }) while (w < kPads) { auto* k = take (is ({ c })); if (! k) break; out[(size_t) w++] = k; }
    for (int p = 0; p < 8; ++p) if (! out[(size_t) p]) out[(size_t) p] = take ([] (const KitPad&) { return true; });
    while (w < kPads) { auto* k = take ([] (const KitPad&) { return true; }); if (! k) break; out[(size_t) w++] = k; }
    return out;
}

void Processor::loadFactoryKit (int k)
{
    for (int p = 0; p < kPads; ++p) { setChord (p, {}); chordName[p] = {}; } progSrc = -1;
    const auto& K = factoryKits()[(size_t) juce::jlimit (0, (int) factoryKits().size() - 1, k)];
    const auto lay = arrangeKit (K.pads);
    for (int p = 0; p < kPads; ++p)
    {
        pad[p].reset();
        if (lay[(size_t) p] != nullptr)
        {
            const auto& kp = *lay[(size_t) p];
            setSample (p, makeDrum (kp.d)); setLayer (p, nullptr); pad[p].colour = kp.colour; padCat[p] = categoryOf (kp.d);
            for (auto& [i, v] : kp.p) pad[p].v[i] = v;
            // i volumi dei kit erano tarati quando i suoni uscivano tutti a picco pieno; ora sono gia bilanciati in loudness: non abbassarli due volte
            if (pad[p].get (LEVEL) < 1.0f) pad[p].v[LEVEL] = 0.55f + 0.45f * pad[p].get (LEVEL);
        }
        else { setSample (p, nullptr); padCat[p] = {}; }
    }
    if (K.pads.empty())   // kit vuoto: anche la sezione master torna ai valori di partenza
        for (auto id : { "revSize", "revDamp", "revRet", "revLo", "revHi", "revPre", "dlyDiv", "dlyMs", "dlyFb", "dlyLo", "dlyHi", "dlyRet", "busMode", "busAmt", "clip", "out", "revKit", "dlyKit", "satAmt" })
            if (auto* rp = apvts.getParameter (id)) rp->setValueNotifyingHost (rp->getDefaultValue());
    kitName = K.name; kitFile = juce::File();
}
bool Processor::saveKit (const juce::File& f, const juce::String& name)
{
    juce::XmlElement x ("JPKIT"); x.setAttribute ("name", name); x.setAttribute ("version", 2);
    x.addChildElement (padsToXml().release());
    auto* m = x.createNewChildElement ("MASTER");
    for (auto id : { "revSize", "revDamp", "revRet", "revLo", "revHi", "revPre", "dlyDiv", "dlyMs", "dlyFb", "dlyLo", "dlyHi", "dlyRet", "busMode", "busAmt", "clip", "revKit", "dlyKit", "satAmt" }) m->setAttribute (id, (double) apvts.getParameter (id)->getValue());
    if (! x.writeTo (f)) return false;
    kitName = name; kitFile = f; return true;
}
bool Processor::loadKit (const juce::File& f)
{
    auto x = juce::XmlDocument::parse (f); if (x == nullptr || ! x->hasTagName ("JPKIT")) return false;
    if (auto* pads = x->getChildByName ("PADS")) padsFromXml (*pads);
    if (auto* m = x->getChildByName ("MASTER"))
        for (auto id : { "revSize", "revDamp", "revRet", "revLo", "revHi", "revPre", "dlyDiv", "dlyMs", "dlyFb", "dlyLo", "dlyHi", "dlyRet", "busMode", "busAmt", "clip", "revKit", "dlyKit", "satAmt" })
            if (m->hasAttribute (id)) apvts.getParameter (id)->setValueNotifyingHost ((float) m->getDoubleAttribute (id));
    kitName = x->getStringAttribute ("name", f.getFileNameWithoutExtension()); kitFile = f;
    return true;
}

juce::File Processor::writeMidi (bool withSwing)
{
    juce::MidiMessageSequence seq; const int root = rootNote();
    { const juce::SpinLock::ScopedLockType l (recLock);
      // durata delle note: i pad che si fermano al rilascio (gate, loop, campioni lunghi) tengono la nota fino al colpo dopo
      // dello stesso pad (o alla fine del giro); gli altri un sedicesimo
      const double L = loopTicks();
      for (auto& e : events)
      {
          if (seqMuted (e.pad)) continue;
          const double q = withSwing ? swingTick (e.tick) : e.tick;
          double len = PPQ / 4.0;
          auto s = getSample (e.pad);
          const bool held = pad[e.pad].get (MODE) > 0.5f || pad[e.pad].get (LOOP) > 0.5f || (s && s->buf.getNumSamples() / s->sr > 0.6);
          if (held)
          {
              double next = L;
              for (auto& o : events) if (o.pad == e.pad && o.tick > e.tick + 0.5 && o.tick < next) next = o.tick;
              if (next >= L) for (auto& o : events) if (o.pad == e.pad && o.tick + L < next + L && o.tick + L > e.tick) next = juce::jmin (next, o.tick + L);
              len = juce::jmax (PPQ / 4.0, (withSwing ? swingTick (next) : next) - q - 2);
          }
          seq.addEvent (juce::MidiMessage::noteOn (1, root + e.pad, (juce::uint8) e.vel), q); seq.addEvent (juce::MidiMessage::noteOff (1, root + e.pad), q + len);
      } }
    seq.updateMatchedPairs(); seq.sort();
    juce::MidiFile mf; mf.setTicksPerQuarterNote (PPQ); mf.addTrack (seq);
    const auto& pb = patternBank()[(size_t) juce::jlimit (0, (int) patternBank().size() - 1, patternIndex)];
    auto f = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile (juce::File::createLegalFileName (juce::String ("JPad ") + pb.genre + " " + pb.name) + ".mid");
    f.deleteFile(); juce::FileOutputStream os (f); if (os.openedOk()) mf.writeTo (os);
    return f;
}

void Processor::getStateInformation (juce::MemoryBlock& dest)
{
    juce::XmlElement x ("JPAD"); x.setAttribute ("kit", kitName); x.setAttribute ("kitFile", kitFile.getFullPathName());
    x.addChildElement (apvts.copyState().createXml().release());
    x.addChildElement (padsToXml().release());
    juce::String ev; { const juce::SpinLock::ScopedLockType l (recLock); for (auto& e : events) ev << juce::String (e.tick, 2) << "," << e.pad << "," << e.vel << ";"; }
    x.createNewChildElement ("PATTERN")->addTextElement (ev);
    x.setAttribute ("pattern", patternIndex); x.setAttribute ("genStyle", genStyle); x.setAttribute ("bars", patternBars.load()); x.setAttribute ("loopOn", loopOn.load() ? 1 : 0); x.setAttribute ("qFx", qFx.load() ? 1 : 0); x.setAttribute ("ctrlOn", ctrlOn.load() ? 1 : 0);
    { juce::String m; for (int k = 0; k < kFx; ++k) m << ccMap[k].load() << (k < kFx - 1 ? "," : ""); x.setAttribute ("ccMap", m); }
    x.setAttribute ("uiTheme", uiTheme.load()); x.setAttribute ("seqMute", seqMute.load());
    { juce::StringArray ch; for (int p = 0; p < kPads; ++p) { juce::String c; for (int k = 0; k < chordN[p].load(); ++k) c << (k ? " " : "") << chordIv[p][k].load(); ch.add (c); } x.setAttribute ("chords", ch.joinIntoString (",")); }
    copyXmlToBinary (x, dest);
}
void Processor::setStateInformation (const void* data, int size)
{
    pendingRandom = false;
    auto x = getXmlFromBinary (data, size); if (x == nullptr || ! x->hasTagName ("JPAD")) return;
    uiTheme = x->getIntAttribute ("uiTheme", 3); seqMute = x->getIntAttribute ("seqMute", 0) & 3;
    if (auto* ps = x->getChildByName (apvts.state.getType())) apvts.replaceState (juce::ValueTree::fromXml (*ps));
    if (auto* pads = x->getChildByName ("PADS")) padsFromXml (*pads);
    for (int p = 0; p < kPads; ++p) for (int k = 0; k < kAutoN; ++k) lastHost[p][k] = hpRaw[p][k]->load();   // i pad salvati vincono; il timer riallinea Ableton
    kitName = x->getStringAttribute ("kit", kitName);
    patternIndex = x->getIntAttribute ("pattern", 0); qFx = x->getIntAttribute ("qFx", 0) != 0; ctrlOn = x->getIntAttribute ("ctrlOn", 1) != 0;
    // la mappatura del controller e' quella del Mac (unica per tutti i Sampler): quella salvata nel progetto non la sovrascrive
    { const auto ch = juce::StringArray::fromTokens (x->getStringAttribute ("chords"), ",", "");
      for (int p = 0; p < kPads; ++p) { std::vector<int> iv; if (p < ch.size()) for (auto& t : juce::StringArray::fromTokens (ch[p], " ", "")) if (t.isNotEmpty()) iv.push_back (t.getIntValue()); setChord (p, iv); } } loopOn = false; patternBars = juce::jlimit (1, 8, x->getIntAttribute ("bars", 2)); genStyle = x->getIntAttribute ("genStyle", 0);   // il loop interno parte solo a mano
    const auto kf = x->getStringAttribute ("kitFile"); kitFile = kf.isNotEmpty() ? juce::File (kf) : juce::File();
    if (auto* pt = x->getChildByName ("PATTERN"))
    {
        const juce::SpinLock::ScopedLockType l (recLock); events.clear();
        for (auto& t : juce::StringArray::fromTokens (pt->getAllSubText(), ";", ""))
        {
            auto f = juce::StringArray::fromTokens (t, ",", ""); if (f.size() < 3) continue;
            events.push_back ({ f[0].getDoubleValue(), f[1].getIntValue(), f[2].getIntValue() });
        }
    }
}

juce::AudioProcessorEditor* Processor::createEditor() { return new Editor (*this); }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new jp::Processor(); }

namespace jp
{
void Processor::assignSlice (int src, float s0, float s1, int dst, int sliceNo)
{
    auto s = getSample (src); if (s == nullptr || dst < 0 || dst >= kPads) return;
    const int col = pad[src].colour.load(); const float gain = pad[src].get (GAIN), sb = pad[src].get (SBPM), wp = pad[src].get (WARP), sk = pad[src].get (SKEY), tu = pad[src].get (TUNE);
    pad[dst].reset(); pad[dst].colour = col; pad[dst].v[GAIN] = gain; pad[dst].v[SBPM] = sb; pad[dst].v[WARP] = wp; pad[dst].v[SKEY] = sk; pad[dst].v[TUNE] = tu;
    pad[dst].v[START] = s0; pad[dst].v[END] = s1; pad[dst].v[SLICE] = (float) sliceNo; pad[dst].v[DECAY] = 4000;
    setSample (dst, s); setLayer (dst, nullptr);
}
void Processor::chopToPads (int src, const std::vector<float>& pts, int firstPad)
{
    auto s = getSample (src); if (s == nullptr || pts.size() < 2) return;
    const int start = firstPad < 0 ? src : firstPad;
    // copia i dati della sorgente prima di sovrascrivere (la sorgente puo essere tra le destinazioni)
    const int col = pad[src].colour.load(); const float gain = pad[src].get (GAIN), sb = pad[src].get (SBPM), wp = pad[src].get (WARP), sk = pad[src].get (SKEY), tu = pad[src].get (TUNE);
    for (size_t k = 0; k + 1 < pts.size(); ++k)
    {
        const int p = start + (int) k; if (p >= kPads) break;
        pad[p].reset(); pad[p].colour = col; pad[p].v[GAIN] = gain; pad[p].v[SBPM] = sb; pad[p].v[WARP] = wp; pad[p].v[SKEY] = sk; pad[p].v[TUNE] = tu;
        pad[p].v[START] = pts[k]; pad[p].v[END] = pts[k + 1]; pad[p].v[SLICE] = (float) (k + 1); pad[p].v[DECAY] = 4000;
        setSample (p, s);
    }
}
}
