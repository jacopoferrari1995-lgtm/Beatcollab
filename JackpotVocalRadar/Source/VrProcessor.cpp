#include "VrProcessor.h"
#include "VrEditor.h"

namespace vr
{
// amount = taglio massimo dB; pres/body/air 0..1; vol dB; stereo 0..1; rel ms; ant ms
const std::vector<VrPreset>& presets()
{
    static const std::vector<VrPreset> P = {
        { "Init", "Punto di partenza: taglio medio sulla presenza, beat che resta largo.", 6, 0.7f, 0.4f, 0.3f, 1.0f, 0.7f, 220, 20 },
        { "Trap  |  Voce Rap", "Rap sopra 808 e melodie piene: si apre la presenza e il corpo, rilascio rapido per non pompare fra le parole.", 7, 0.85f, 0.55f, 0.35f, 1.5f, 0.75f, 140, 25 },
        { "Drill  |  Flow Fitto", "Flow serrato su drill: rilascio cortissimo, anticipo lungo per le consonanti, beat largo sui lati.", 7.5f, 0.9f, 0.45f, 0.4f, 1.0f, 0.85f, 110, 30 },
        { "Hip Hop  |  Boom Bap", "Sample caldi e pieni di medi: si lavora molto sul corpo, la presenza piu' gentile.", 6, 0.6f, 0.8f, 0.2f, 1.5f, 0.5f, 200, 20 },
        { "R&B  |  Voce Melodica", "Note lunghe: rilascio lento cosi' il beat non respira a ogni sillaba, tagli morbidi.", 5, 0.7f, 0.5f, 0.5f, 0.8f, 0.7f, 400, 15 },
        { "Pop  |  Voce Davanti", "La voce deve stare sopra a tutto: taglio piu' profondo e un filo di volume.", 9, 0.85f, 0.6f, 0.5f, 2.5f, 0.6f, 250, 20 },
        { "Ad-lib  |  Leggero", "Per le ad-lib e le doppie: tocco leggero, solo presenza.", 3.5f, 0.8f, 0.2f, 0.3f, 0.3f, 0.8f, 150, 15 },
        { "Parlato  |  Podcast", "Voce parlata sopra musica: il beat scende di volume e lascia il centro alla voce.", 8, 0.75f, 0.7f, 0.2f, 4.0f, 0.3f, 500, 25 },
        { "Aggressivo  |  Scream", "Voci urlate o distorte su beat pieni: tutto aperto, rilascio medio.", 10, 0.9f, 0.7f, 0.6f, 2.0f, 0.6f, 180, 30 },
        { "Solo Bus Strumentale", "Su un bus con tutto il beat: interviene su ogni banda, volume minimo.", 6, 0.75f, 0.55f, 0.4f, 0.8f, 0.7f, 220, 20 } };
    return P;
}

juce::AudioProcessorValueTreeState::ParameterLayout Processor::layout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    auto F = [&] (const char* id, const char* nm, float a, float b, float d, float step = 0.0f) { l.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, nm, NormalisableRange<float> (a, b, step), d)); };
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "mode", 1 }, "Modo", StringArray { "VOCE", "BEAT" }, 1));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "chan", 1 }, "Canale", StringArray { "A", "B", "C", "D" }, 0));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "src", 1 }, "Sorgente voce", StringArray { "LINK", "SIDECHAIN" }, 0));
    // neutro al caricamento: AMOUNT a 0, il resto ai valori dell'Init
    F ("amount", "Amount", 0, 12, 0, 0.1f);
    F ("pres", "Presenza", 0, 1, 0.7f); F ("body", "Corpo", 0, 1, 0.4f); F ("air", "Aria", 0, 1, 0.3f);
    F ("vol", "Volume", 0, 6, 0, 0.1f); F ("stereo", "Stereo", 0, 1, 0.7f);
    F ("rel", "Rilascio", 40, 1000, 220, 1); F ("ant", "Anticipo", 0, 40, 20, 1);
    F ("sens", "Sensibilita", -60, -20, -42, 1);
    F ("mix", "Mix", 0, 1, 1); F ("out", "Output", -12, 12, 0, 0.1f);
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "delta", 1 }, "Delta", false));
    return l;
}

Processor::Processor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true).withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                                       .withInput ("Sidechain", juce::AudioChannelSet::stereo(), false)),
      apvts (*this, nullptr, "PARAMS", layout()), slot (juce::Random::getSystemRandom().nextInt (SLOTS))
{
    for (int b = 0; b < NB; ++b) { uiVoice[b] = -120; uiBeat[b] = -120; uiCut[b] = 0; }
    for (int i = 0; i < kHist; ++i) { hAct[i] = 0; hCut[i] = 0; }
}
Processor::~Processor() {}
bool Processor::isBusesLayoutSupported (const BusesLayout& l) const
{
    if (l.getMainOutputChannelSet() != juce::AudioChannelSet::stereo() || l.getMainInputChannelSet() != juce::AudioChannelSet::stereo()) return false;
    if (l.inputBuses.size() > 1) { const auto sc = l.inputBuses[1]; if (! sc.isDisabled() && sc != juce::AudioChannelSet::stereo() && sc != juce::AudioChannelSet::mono()) return false; }
    return true;
}
void Processor::prepareToPlay (double s, int)
{
    sr = s; L = (int) std::round (0.040 * sr);   // 40 ms di anticipo massimo: il beat esce in ritardo, Ableton lo riallinea
    vBank.prepare (sr); bBank.prepare (sr); scBank.prepare (sr);
    int n = 1; while (n < L + 8) n <<= 1; dM.assign ((size_t) n, 0.0f); dS = dM; dMask = n - 1; dW = 0;
    int g = 1; while (g < L / HOP + 8) g <<= 1; gq.assign ((size_t) g, {}); gMask = g - 1; gW = 0;
    for (auto& q : gq) q.fill (0.0f);
    for (int b = 0; b < NB; ++b) { pm[b] = {}; ps[b] = {}; cutS[b] = 0; }
    volS = 0; actEnv = 0; hopN = 0;
    setLatencySamples (beatMode() ? L : 0);
}

const Frame* Processor::findFrame (int c, double want, bool playing) const
{
    auto& ch = hub().ch[c]; const int w = ch.w.load(); if (w == 0) return nullptr;
    const Frame* best = &ch.f[(w - 1) & (RING - 1)];
    if (! playing || best->pos < 0) return best;
    for (int k = 1; k < 64 && k < w; ++k)   // il fotogramma della voce allo stesso punto del pezzo (se e' gia' arrivato)
    {
        if (best->pos <= want) break;
        best = &ch.f[(w - 1 - k) & (RING - 1)];
    }
    return best;
}

void Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals nd; midi.clear();
    const int N = buffer.getNumSamples(); if (N == 0) return;
    double pos0 = -1; bool playing = false;
    if (auto* ph = getPlayHead()) if (auto p = ph->getPosition()) { playing = p->getIsPlaying(); if (auto t = p->getTimeInSamples()) pos0 = (double) *t; }
    if (! playing || pos0 < 0) { playing = false; pos0 = -1; }
    const int want = beatMode() ? L : 0; if (getLatencySamples() != want) setLatencySamples (want);
    if (! beatMode()) { processVoice (buffer.getReadPointer (0), buffer.getReadPointer (1), N, pos0, playing); return; }
    processBeat (buffer, N, pos0, playing);
}

// ------------------------------------------------------------------------------------------------ VOCE: misura e trasmette, l'audio passa intatto
void Processor::processVoice (const float* Lp, const float* Rp, int n, double pos0, bool playing)
{
    auto& ch = hub().ch[channel()];
    for (int i = 0; i < n; ++i)
    {
        vBank.push (0.5f * (Lp[i] + Rp[i]));
        if (++hopN >= HOP)
        {
            hopN = 0; const int w = ch.w.load(); Frame& f = ch.f[w & (RING - 1)];
            f.lvl = vBank.take (f.band); f.pos = playing ? pos0 + i : -1; ch.w.store (w + 1);
            ch.stamp = nowMs(); ch.lvl = f.lvl; for (int b = 0; b < NB; ++b) ch.bands[b] = f.band[b];
            const float act = juce::jlimit (0.0f, 1.0f, (f.lvl - v ("sens")) / 6.0f);
            for (int b = 0; b < NB; ++b) uiVoice[b] = f.band[b];
            uiVoiceLvl = f.lvl; uiAct = act;
            colA = juce::jmax (colA, act); if (++colN >= 8) { const int hw = hW.load(); hAct[hw] = colA; hCut[hw] = 0; hW = (hw + 1) % kHist; colA = 0; colN = 0; }
        }
    }
    int listeners = beatsListening (channel()); uiLinked = listeners > 0;
}

// ------------------------------------------------------------------------------------------------ BEAT: fa spazio
void Processor::processBeat (juce::AudioBuffer<float>& buffer, int n, double pos0, bool playing)
{
    const int c = channel(); hub().ch[c].beatStamp[slot] = nowMs();
    float* Lp = buffer.getWritePointer (0); float* Rp = buffer.getWritePointer (1);
    const bool useSc = v ("src") > 0.5f; auto sc = getBusBuffer (buffer, true, 1);
    const bool scOn = useSc && sc.getNumChannels() > 0;
    const bool linked = ! useSc && voiceAlive (c); uiLinked = linked; uiSide = scOn;
    const float amount = v ("amount"), pres = v ("pres"), body = v ("body"), air = v ("air"), volMax = v ("vol"), st = v ("stereo");
    const float relMs = v ("rel"), antMs = v ("ant"), thr = v ("sens"), mix = v ("mix"), og = juce::Decibels::decibelsToGain (v ("out"));
    const bool delta = v ("delta") > 0.5f;
    const float hopSec = (float) HOP / (float) sr;
    const float aAtt = 1.0f - std::exp (-hopSec / 0.006f), aRel = 1.0f - std::exp (-hopSec / (relMs * 0.001f));
    const float bandAtt = 1.0f - std::exp (-hopSec / 0.004f), bandRel = 1.0f - std::exp (-hopSec / (juce::jmax (0.03f, relMs * 0.0006f)));
    const int lagHops = juce::jlimit (0, gMask, (int) std::round ((L - antMs * 0.001f * (float) sr) / HOP));
    // peso di ogni banda: dove la voce si fa capire
    float wgt[NB];
    for (int b = 0; b < NB; ++b)
    {
        const float f = bandHz (b), lf = std::log2 (f);
        auto bump = [lf] (float lo, float hi) { const float c0 = 0.5f * (std::log2 (lo) + std::log2 (hi)), w = 0.5f * (std::log2 (hi) - std::log2 (lo)) + 0.6f; return juce::jlimit (0.0f, 1.0f, 1.0f - std::abs (lf - c0) / w); };
        wgt[b] = juce::jlimit (0.0f, 1.0f, 0.15f + 0.85f * juce::jmax (pres * bump (1800, 5500), body * bump (160, 480), air * bump (7000, 12000), 0.55f * juce::jmax (pres, body) * bump (500, 1800)));
    }
    const float* scL = scOn ? sc.getReadPointer (0) : nullptr; const float* scR = scOn ? sc.getReadPointer (juce::jmin (1, sc.getNumChannels() - 1)) : nullptr;
    for (int i = 0; i < n; ++i)
    {
        const float l = Lp[i], r = Rp[i], m = 0.5f * (l + r), s = 0.5f * (l - r);
        bBank.push (m); if (scOn) scBank.push (0.5f * (scL[i] + scR[i]));
        if (++hopN >= HOP)
        {
            hopN = 0;
            float bb[NB]; const float bAll = bBank.take (bb);
            const Frame* vf = nullptr;
            if (scOn) { scFrame.lvl = scBank.take (scFrame.band); vf = &scFrame; }
            else if (linked) vf = findFrame (c, playing ? pos0 + i : -1, playing);
            float vb[NB]; float vAll = -120;
            if (vf != nullptr) { for (int b = 0; b < NB; ++b) vb[b] = vf->band[b]; vAll = vf->lvl; } else for (auto& x : vb) x = -120;
            const float act = juce::jlimit (0.0f, 1.0f, (vAll - thr) / 6.0f);
            actEnv += (act > actEnv ? aAtt : aRel) * (act - actEnv);
            if (act > 0.5f) avgLvl += 0.004f * (vAll - avgLvl);
            const float dens = juce::jlimit (0.0f, 1.2f, (vAll - avgLvl + 6.0f) / 12.0f);   // le frasi piu' piene aprono di piu'
            float vMax = -120; for (float x : vb) vMax = juce::jmax (vMax, x);
            auto& g = gq[(size_t) (gW & gMask)]; float totCut = 0;
            for (int b = 0; b < NB; ++b)
            {
                const float mask = juce::jlimit (0.0f, 1.0f, (bb[b] - vb[b] + 6.0f) / 12.0f);       // il beat copre la voce in questa banda?
                const float vin = juce::jlimit (0.0f, 1.0f, (vb[b] - (vMax - 30.0f)) / 18.0f);      // la voce ha davvero qualcosa qui?
                const float tgt = amount * wgt[b] * mask * vin * actEnv * (0.6f + 0.4f * dens);
                cutS[b] += (tgt > cutS[b] ? bandAtt : bandRel) * (tgt - cutS[b]);
                g[(size_t) b] = cutS[b]; totCut = juce::jmax (totCut, cutS[b]);
                uiBeat[b] = bb[b]; uiVoice[b] = vb[b]; uiCut[b] = cutS[b];
            }
            g[NB] = volMax * actEnv * juce::jlimit (0.0f, 1.0f, (bAll - vAll + 9.0f) / 12.0f);
            uiAct = actEnv; uiVol = g[NB]; uiVoiceLvl = vAll;
            if (learning.load()) learnHop (vb, bb, act, 0, bAll, vAll);
            ++gW;
            // applica i tagli di (L - anticipo) fa: cosi' il beat (ritardato di L) si apre un attimo prima della voce
            const auto& ga = gq[(size_t) ((gW - 1 - lagHops) & gMask)];
            for (int b = 0; b < NB; ++b) { pm[b].set (bandHz (b), 1.8f, -ga[(size_t) b], sr); ps[b].set (bandHz (b), 1.8f, -ga[(size_t) b] * (1.0f - st), sr); }
            volS = ga[NB];
            colA = juce::jmax (colA, actEnv); colC = juce::jmax (colC, totCut);
            if (++colN >= 8) { const int hw = hW.load(); hAct[hw] = colA; hCut[hw] = colC; hW = (hw + 1) % kHist; colA = colC = 0; colN = 0; }
        }
        // ritardo di L e filtri
        dM[(size_t) dW] = m; dS[(size_t) dW] = s;
        const float md = dM[(size_t) ((dW - L) & dMask)], sd = dS[(size_t) ((dW - L) & dMask)]; dW = (dW + 1) & dMask;
        float mo = md, so = sd;
        for (int b = 0; b < NB; ++b) { mo = pm[b].run (mo); so = ps[b].run (so); }
        const float vg = juce::Decibels::decibelsToGain (-volS);
        mo *= vg; so *= vg;
        float ol = mo + so, orr = mo - so;
        const float dl = md + sd, dr = md - sd;
        if (delta) { ol = dl - ol; orr = dr - orr; }
        else { ol = dl + (ol - dl) * mix; orr = dr + (orr - dr) * mix; }
        Lp[i] = ol * og; Rp[i] = orr * og;
    }
    if (learning.load()) learnSec = learnSec.load() + (float) n / (float) sr;
}

// ------------------------------------------------------------------------------------------------ preset
void Processor::loadPreset (int i)
{
    const auto& p = presets()[(size_t) juce::jlimit (0, (int) presets().size() - 1, i)];
    setV ("amount", p.amount); setV ("pres", p.pres); setV ("body", p.body); setV ("air", p.air); setV ("vol", p.vol); setV ("stereo", p.stereo); setV ("rel", p.rel); setV ("ant", p.ant);
    presetName = p.name;
}

// ------------------------------------------------------------------------------------------------ ONE CLICK
void Processor::learnStart()
{
    for (int b = 0; b < NB; ++b) { lsV[b] = 0; lsB[b] = 0; }
    lsN = lsHops = lsOn = lsOnsets = 0; lsPrevAct = 0; lsBeatMinusVoice = 0; proposal = {}; learnSec = 0; learning = true;
}
void Processor::learnHop (const float* vb, const float* bb, float act, float, float bAll, float vAll)
{
    ++lsHops;
    if (act > 0.5f && lsPrevAct <= 0.5f) ++lsOnsets;
    lsPrevAct = act;
    if (act < 0.5f) return;
    ++lsOn; ++lsN;
    for (int b = 0; b < NB; ++b) { lsV[b] += vb[b]; lsB[b] += bb[b]; }
    lsBeatMinusVoice += bAll - vAll;
}
void Processor::learnStop()
{
    learning = false;
    Proposal p; p.ready = true;
    const float secs = lsHops * (float) HOP / (float) sr;
    if (lsN < 40 || secs < 3)
    {
        p.lines.add ("Ho sentito troppa poca voce (" + juce::String (lsOn * HOP / sr, 1) + " s su " + juce::String (secs, 1) + " s).");
        p.lines.add (hub().ch[channel()].w.load() == 0 && v ("src") < 0.5f ? "Nessuna copia VOCE sul canale " + juce::String (chName (channel())) + ": mettila sulla traccia della voce." : "Fai suonare voce e beat insieme per almeno 10 secondi.");
        p.ready = false; proposal = p; return;
    }
    float mv[NB], mb[NB], over[NB];
    for (int b = 0; b < NB; ++b) { mv[b] = (float) (lsV[b] / lsN); mb[b] = (float) (lsB[b] / lsN); over[b] = mb[b] - mv[b]; }
    auto region = [&] (float lo, float hi) { float s = 0; int k = 0; for (int b = 0; b < NB; ++b) if (bandHz (b) >= lo && bandHz (b) <= hi) { s += juce::jmax (0.0f, over[b] + 6.0f); ++k; } return k ? s / k : 0.0f; };
    const float rBody = region (150, 500), rPres = region (1800, 5500), rAir = region (7000, 12000), rMid = region (500, 1800);
    const float sev = (rBody + rPres + rMid) / 3.0f;
    const float rate = lsOnsets / juce::jmax (1.0f, secs), ratio = (float) lsOn / juce::jmax (1, lsHops);
    const float bmv = (float) (lsBeatMinusVoice / lsN);
    p.amount = juce::jlimit (2.0f, 11.0f, 3.0f + sev * 0.55f);
    const float mx = juce::jmax (0.1f, rBody, rPres, rAir);
    p.pres = juce::jlimit (0.2f, 1.0f, 0.3f + 0.7f * rPres / mx); p.body = juce::jlimit (0.1f, 1.0f, 0.2f + 0.8f * rBody / mx); p.air = juce::jlimit (0.0f, 1.0f, 0.1f + 0.7f * rAir / mx);
    const bool dense = rate > 2.2f || ratio > 0.7f;
    p.rel = dense ? 130.0f : rate > 1.2f ? 220.0f : 380.0f; p.ant = dense ? 28.0f : 18.0f;
    p.vol = juce::jlimit (0.0f, 3.5f, (bmv + 2.0f) * 0.4f);
    p.stereo = 0.7f;
    p.lines.add ("Ascoltati " + juce::String (secs, 1) + " s, voce presente il " + juce::String (juce::roundToInt (ratio * 100)) + "% del tempo, " + juce::String (rate, 1) + " attacchi al secondo.");
    p.lines.add (juce::String ("Voce ") + (dense ? "fitta (rap): rilascio corto e anticipo lungo per le consonanti." : rate > 1.2f ? "a frasi medie: rilascio medio." : "melodica, frasi lunghe: rilascio lento per non far respirare il beat."));
    auto zone = [] (float r) { return r > 8 ? juce::String ("forte") : r > 4 ? juce::String ("media") : juce::String ("leggera"); };
    p.lines.add ("Copertura del beat sulla voce: corpo " + zone (rBody) + ", centro " + zone (rMid) + ", presenza " + zone (rPres) + ", aria " + zone (rAir) + ".");
    p.lines.add ("Il beat sta " + juce::String (std::abs (bmv), 1) + " dB " + (bmv > 0 ? "sopra" : "sotto") + " la voce mentre canta" + (p.vol > 0.5f ? ": aggiungo " + juce::String (p.vol, 1) + " dB di volume." : ": niente volume, bastano le bande."));
    p.lines.add ("Proposta: AMOUNT " + juce::String (p.amount, 1) + " dB, PRESENZA " + juce::String (juce::roundToInt (p.pres * 100)) + "%, CORPO " + juce::String (juce::roundToInt (p.body * 100)) + "%, ARIA " + juce::String (juce::roundToInt (p.air * 100)) + "%, RILASCIO " + juce::String (juce::roundToInt (p.rel)) + " ms.");
    proposal = p;
}
void Processor::learnApply()
{
    if (! proposal.ready) return;
    const char* ids[8] = { "amount", "pres", "body", "air", "vol", "stereo", "rel", "ant" };
    for (int i = 0; i < 8; ++i) backup[i] = v (ids[i]); hasBackup = true;
    setV ("amount", proposal.amount); setV ("pres", proposal.pres); setV ("body", proposal.body); setV ("air", proposal.air);
    setV ("vol", proposal.vol); setV ("stereo", proposal.stereo); setV ("rel", proposal.rel); setV ("ant", proposal.ant);
    presetName = "ONE CLICK"; proposal.ready = false;
}
void Processor::learnDiscard()
{
    if (hasBackup) { const char* ids[8] = { "amount", "pres", "body", "air", "vol", "stereo", "rel", "ant" }; for (int i = 0; i < 8; ++i) setV (ids[i], backup[i]); hasBackup = false; }
    proposal = {};
}

void Processor::getStateInformation (juce::MemoryBlock& d)
{
    auto s = apvts.copyState(); s.setProperty ("preset", presetName, nullptr); s.setProperty ("theme", uiTheme.load(), nullptr);
    if (auto x = s.createXml()) copyXmlToBinary (*x, d);
}
void Processor::setStateInformation (const void* data, int size)
{
    if (auto x = getXmlFromBinary (data, size)) { auto t = juce::ValueTree::fromXml (*x); if (! t.isValid()) return; presetName = t.getProperty ("preset", presetName).toString(); uiTheme = (int) t.getProperty ("theme", 0); apvts.replaceState (t); }
}
juce::AudioProcessorEditor* Processor::createEditor() { return new Editor (*this); }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new vr::Processor(); }
