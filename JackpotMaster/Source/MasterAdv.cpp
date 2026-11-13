// AVANZATO a inserimento: catena verticale, galleria degli effetti, barra di stato con consigli, scheda dei preset
#include "MasterEditor.h"

namespace jms
{
using namespace neo;

// ---------------------------------------------------------------------------------------------------- testi degli effetti
struct ModText { const char* what; const char* when; };
static const ModText& modText (int m)
{
    static const ModText T[kNumMods] = {
        { "Tiene fermi sub e cassa: mono sotto una frequenza, punch e coda dei bassi.", "Quando 808 e cassa ballano o in mono perdono forza." },
        { "Sei bande stile Pro-Q: ogni banda puo intervenire solo quando serve.", "Fango, durezze, risonanze che vanno e vengono." },
        { "Abbassa solo le sibilanti e i piatti troppo forti.", "S e piatti che pungono dopo il maximizer." },
        { "Compressore da console: incolla il mix con 1-3 dB.", "Quando il mix suona slegato o troppo nervoso." },
        { "Dinamica su tre bande, anche verso l'alto (stile OTT).", "Per densita e dettaglio, o per domare una sola zona." },
        { "Aggiunge armoniche per banda: valvola, nastro, brillante.", "Quando il mix e piatto o gli alti sono spenti." },
        { "Nastro analogico: bump sui bassi, alti morbidi, saturazione.", "Per calore e coesione, soprattutto su boom bap e lo-fi." },
        { "Larghezza stereo per banda, con controllo della mono.", "Bassi da stringere, alti da aprire." },
        { "Volume: porta la traccia al loudness che vuoi, con carattere.", "Sempre, verso la fine della catena." },
        { "Ultimo controllo: nessun picco oltre il ceiling, in true peak.", "Sempre, come ultimo effetto." },
        { "EQ largo e musicale da mastering, cinque bande.", "Ritocchi ampi di colore: +1/-1 dB fanno tanto." },
        { "EQ passivo a valvole: il trucco boost + atten sui bassi.", "Bassi pieni ma puliti, aria morbida sugli alti." },
        { "Compressore a valvole lento e morbido.", "Colla piu calda e scura del BUS COMP." } };
    return T[juce::jlimit (0, kNumMods - 1, m)];
}

// cosa sta facendo l'effetto (breve per la catena, lungo per la barra) e un consiglio pratico
struct Stat { juce::String brief, doing, tip; };
static juce::String db1 (float v) { return juce::String (std::abs (v), 1) + " dB"; }
static Stat statOf (Editor& ed, int m)
{
    auto& P = ed.proc; Stat s;
    const bool on = P.value (onParam (m)) > 0.5f;
    if (! on) return { "spento", "e in catena ma spento: non tocca il suono.", "Accendilo dal quadratino a sinistra, oppure toglilo con la x se non ti serve." };
    const float tgt = target ((int) P.value ("target")).lufs, lu = P.lufsS.load();
    switch (m)
    {
        case BASS:
        {   const float pk = P.bassPeak.load();
            s.brief = pk > 0.1f ? "-" + db1 (pk) : "mono < " + juce::String ((int) P.value ("bc_mono")) + " Hz";
            s.doing = "bassi mono sotto " + juce::String ((int) P.value ("bc_mono")) + " Hz" + (pk > 0.1f ? ", tengo fermi i picchi del sub di " + db1 (pk) : juce::String (", i picchi del sub sono gia sotto controllo"));
            s.tip = "Sotto 100-120 Hz il mono non si sente come perdita ma salva i bassi in club e sul telefono."; break; }
        case DYNEQ:
        {   float mx = 0; int mb = -1; for (int b = 0; b < kEqBands; ++b) if (std::abs (P.eqDyn[b].load()) > mx) { mx = std::abs (P.eqDyn[b].load()); mb = b; }
            s.brief = mx > 0.1f ? "B" + juce::String (mb + 1) + " " + db1 (mx) : "statico";
            s.doing = mx > 0.1f ? "la banda " + juce::String (mb + 1) + " si muove di " + db1 (mx) + " seguendo il suono" : "le bande lavorano fisse (nessuna banda dinamica attiva in questo momento)";
            s.tip = "Sul master tagli stretti e piccoli: oltre 2-3 dB e meglio correggere nel mix."; break; }
        case DEESS:
        {   const float g = P.dsGr.load(); s.brief = g > 0.1f ? "-" + db1 (g) : "a riposo";
            s.doing = g > 0.1f ? "abbasso le sibilanti di " + db1 (g) : "nessuna sibilante sopra la soglia";
            s.tip = g > 6 ? "Taglia tanto: la voce rischia di diventare sdentata, alza la soglia." : "Ascolta con DELTA: devi sentire solo le S, non la voce."; break; }
        case BUSCOMP:
        {   const float g = P.busGr.load(); s.brief = g > 0.1f ? "-" + db1 (g) : "a riposo";
            s.doing = g > 0.1f ? "comprimo di " + db1 (g) : "non sto comprimendo";
            s.tip = g > 4 ? "Sul master 1-3 dB bastano: alza la soglia o usa il MIX." : g < 0.3f ? "Abbassa la soglia finche la lancetta tocca 1-2 dB sui colpi." : "Zona giusta: colla senza perdere i transienti."; break; }
        case MULTI:
        {   float d[kMbBands]; float mx = 0; for (int b = 0; b < kMbBands; ++b) { d[b] = P.mbDown[b].load(); mx = juce::jmax (mx, d[b]); }
            s.brief = mx > 0.1f ? "-" + db1 (mx) : "a riposo";
            s.doing = "bassi -" + db1 (d[0]) + ", medi -" + db1 (d[1]) + ", alti -" + db1 (d[2]);
            s.tip = "Se una banda lavora molto piu delle altre, prova prima con l'EQ: il multibanda cambia il bilanciamento."; break; }
        case EXCITER:
        {   const float e = P.exMeter.load(); s.brief = juce::String (juce::roundToInt (e * 100)) + "%";
            s.doing = "aggiungo armoniche, attivita " + juce::String (juce::roundToInt (e * 100)) + "%";
            s.tip = "Accendi e spegni a volume uguale (CONFRONTA): l'exciter inganna, sembra sempre meglio perche e piu forte."; break; }
        case TAPE:
            s.brief = "drive " + juce::String (juce::roundToInt (P.value ("tp_drive") * 100)) + "%";
            s.doing = "nastro con drive " + juce::String (juce::roundToInt (P.value ("tp_drive") * 100)) + "%, mix " + juce::String (juce::roundToInt (P.value ("tp_mix") * 100)) + "%";
            s.tip = "Il nastro arrotonda i transienti: se la cassa perde schiocco, abbassa il drive."; break;
        case IMAGER:
        {   const float c = P.corrAll.load(); s.brief = "corr " + juce::String (c, 2);
            s.doing = "correlazione stereo " + juce::String (c, 2) + (c < 0 ? " (in mono si perdono pezzi)" : c < 0.3f ? " (molto largo)" : " (sano)");
            s.tip = c < 0 ? "Correlazione negativa: stringi i bassi o riduci la larghezza degli alti." : "Allarga solo sopra i 2-3 kHz: i bassi larghi spariscono in mono."; break; }
        case MAXIM:
        {   const float g = P.grMax.load(); s.brief = g > 0.1f ? "-" + db1 (g) : "+" + juce::String (P.value ("mx_gain"), 1);
            s.doing = "alzo il volume di " + juce::String (P.value ("mx_gain"), 1) + " dB e taglio i picchi di " + db1 (g);
            s.tip = g > 6 ? "Oltre 6 dB senti il pompaggio: meno gain o stile MORBIDO." : (lu > -60 && lu < tgt - 2) ? "Sei a " + juce::String (lu, 1) + " LUFS, il target e " + juce::String ((int) tgt) + ": puoi alzare il gain." : "Il volume giusto e quello in cui la cassa resta in faccia: ascolta con CONFRONTA."; break; }
        case LIMIT:
        {   const float g = P.grLim.load(), tp = P.tpMax.load(); s.brief = g > 0.1f ? "-" + db1 (g) : "a riposo";
            s.doing = "limito di " + db1 (g) + ", true peak " + juce::String (tp, 1) + " dBTP";
            s.tip = g > 3 ? "Il limiter lavora troppo: il volume va fatto col maximizer, qui solo i picchi." : tp > P.value ("lm_ceil") + 0.05f ? "Il true peak supera il ceiling: accendi TRUE PEAK." : "Va bene: il limiter finale deve toccare appena."; break; }
        case EQ432:
            s.brief = "colore"; s.doing = "EQ largo da mastering sul tono generale";
            s.tip = "Mezzo dB su una shelf cambia piu di 3 dB su una campana stretta: muoviti piano."; break;
        case PULTEC:
            s.brief = "bassi +" + juce::String (P.value ("pq_lb"), 1); s.doing = "bassi: boost " + juce::String (P.value ("pq_lb"), 1) + ", atten " + juce::String (P.value ("pq_la"), 1) + "  ·  alti: boost " + juce::String (P.value ("pq_hb"), 1);
            s.tip = "Boost e atten insieme sulla stessa frequenza: bassi pieni senza fango."; break;
        case VARIMU:
        {   const float g = P.vmGr.load(); s.brief = g > 0.1f ? "-" + db1 (g) : "a riposo";
            s.doing = g > 0.1f ? "comprimo a valvole di " + db1 (g) : "non sto comprimendo";
            s.tip = g > 3 ? "Il vari-mu e lento: oltre 2-3 dB il mix si scurisce e respira troppo." : "1-2 dB di lancetta e il punto in cui si sente la colla."; break; }
        default: break;
    }
    return s;
}

// ---------------------------------------------------------------------------------------------------- catena
std::vector<int> Rack::visible() const
{
    std::vector<int> v; for (int k = 0; k < kNumMods; ++k) { const int m = ed.proc.order[k].load(); if (ed.proc.inRack[m].load()) v.push_back (m); } return v;
}
void Rack::paint (juce::Graphics& g)
{
    setTheme (ed.proc.uiTheme.load() == 2 ? 2 : 1);
    g.setFont (cap (8.5f)); g.setColour (K::ink); g.drawText ("CATENA", juce::Rectangle<float> (14, 10, 120, 24), juce::Justification::centredLeft);
    g.setFont (cap (7.0f)); g.setColour (K::faint); g.drawText (juce::String::fromUTF8 ("INGRESSO \xe2\x86\x93"), juce::Rectangle<float> (0, 10, (float) getWidth() - 14, 24), juce::Justification::centredRight);
    const auto v = visible();
    if (v.empty())
    {
        g.setFont (f (10.0f)); g.setColour (K::sub);
        g.drawFittedText ("Catena vuota.\nAggiungi il primo effetto con il tasto qui sotto, oppure scegli un preset.", juce::Rectangle<int> (16, 60, getWidth() - 32, 80), juce::Justification::topLeft, 4);
    }
    int slot = 0;
    for (int i = 0; i < (int) v.size(); ++i)
    {
        const int m = v[(size_t) i];
        if (dragging && m == dragMod) continue;
        // durante il trascinamento si apre il posto dove cadra
        if (dragging) { const int at = juce::jlimit (0, (int) v.size() - 1, (int) ((dragY - 44) / 46)); if (slot == at) ++slot; }
        auto r = row (slot++);
        const auto c = acc (juce::Colour (modInfo (m).colour)); const bool on = ed.proc.value (onParam (m)) > 0.5f, s = ed.sel == m && ed.advView == Editor::kEff, h = hov == m;
        g.setColour (s ? c.withAlpha (light() ? 0.14f : 0.16f) : K::hi.withAlpha (h ? 0.07f : 0.035f)); g.fillRoundedRectangle (r, 9);
        if (s) { g.setColour (c.withAlpha (0.7f)); g.drawRoundedRectangle (r.reduced (0.5f), 9, 1.0f); }
        // interruttore
        auto pw = juce::Rectangle<float> (r.getX() + 8, r.getCentreY() - 7, 14, 14);
        g.setColour (on ? c : K::hi.withAlpha (0.12f)); g.fillRoundedRectangle (pw, 4);
        if (on) { g.setColour (c.withAlpha (0.25f)); g.fillRoundedRectangle (pw.expanded (3), 6); }
        g.setFont (cap (8.0f)); g.setColour (on ? K::ink : K::faint); g.drawFittedText (modInfo (m).name, r.withTrimmedLeft (30).withTrimmedRight (24).withHeight (r.getHeight() * 0.55f).toNearestInt(), juce::Justification::bottomLeft, 1, 0.8f);
        const auto st = statOf (ed, m);
        g.setFont (num (8.0f)); g.setColour (on ? c : K::faint); g.drawFittedText (st.brief, r.withTrimmedLeft (30).withTrimmedRight (24).withTrimmedTop (r.getHeight() * 0.55f).toNearestInt(), juce::Justification::topLeft, 1, 0.8f);
        // togli
        if (h) { auto x = juce::Rectangle<float> (r.getRight() - 22, r.getCentreY() - 8, 16, 16); g.setColour (hovX ? K::red : K::sub); g.setFont (f (12.0f, 1)); g.drawText (juce::String::fromUTF8 ("\xc3\x97"), x, juce::Justification::centred); }
    }
    if (dragging && dragMod >= 0)
    {
        auto r = row (0).withY (dragY - grab); const auto c = acc (juce::Colour (modInfo (dragMod).colour));
        g.setColour (K::lo.withAlpha (0.35f)); g.fillRoundedRectangle (r.translated (0, 4), 9);
        g.setColour (K::panel0); g.fillRoundedRectangle (r, 9); g.setColour (c); g.drawRoundedRectangle (r.reduced (0.5f), 9, 1.2f);
        g.setFont (cap (8.0f)); g.setColour (K::ink); g.drawText (modInfo (dragMod).name, r.withTrimmedLeft (30), juce::Justification::centredLeft);
    }
    // aggiungi + strumenti
    neo::chip (g, addR(), "+  AGGIUNGI EFFETTO", ed.gallery.isVisible(), K::violet, 8.5f, hov == 100);
    neo::chip (g, toolR (0), "BILANCIO", ed.sel == TOOL_BAL && ed.advView == Editor::kEff, K::mint, 7.5f, hov == 101);
    neo::chip (g, toolR (1), ed.proc.refListen.load() ? "RIF. IN ASCOLTO" : "RIFERIMENTO", ed.sel == TOOL_REF && ed.advView == Editor::kEff, K::cyan, 7.5f, hov == 102);
    g.setFont (cap (7.0f)); g.setColour (K::faint); g.drawText (juce::String::fromUTF8 ("USCITA \xe2\x86\x93"), juce::Rectangle<float> (0, (float) getHeight() - 32, (float) getWidth() - 14, 22), juce::Justification::centredRight);
    g.setFont (f (8.5f)); g.drawText ("trascina per spostare", juce::Rectangle<float> (14, (float) getHeight() - 32, 160, 22), juce::Justification::centredLeft);
}
void Rack::mouseMove (const juce::MouseEvent& e)
{
    int h = -1; bool x = false;
    const auto v = visible();
    for (int i = 0; i < (int) v.size(); ++i) if (row (i).contains (e.position)) { h = v[(size_t) i]; x = e.position.x > row (i).getRight() - 26; }
    if (addR().contains (e.position)) h = 100; if (toolR (0).contains (e.position)) h = 101; if (toolR (1).contains (e.position)) h = 102;
    if (h != hov || x != hovX) { hov = h; hovX = x; repaint(); }
    setMouseCursor (h >= 0 ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
}
void Rack::mouseDown (const juce::MouseEvent& e)
{
    dragMod = -1; dragging = false;
    if (addR().contains (e.position)) { ed.gallery.setVisible (! ed.gallery.isVisible()); ed.gallery.toFront (false); repaint(); return; }
    if (toolR (0).contains (e.position)) { ed.gallery.setVisible (false); ed.openEffect (TOOL_BAL); return; }
    if (toolR (1).contains (e.position)) { ed.gallery.setVisible (false); ed.openEffect (TOOL_REF); return; }
    const auto v = visible();
    for (int i = 0; i < (int) v.size(); ++i)
    {
        auto r = row (i); if (! r.contains (e.position)) continue;
        const int m = v[(size_t) i];
        if (e.position.x > r.getRight() - 26) { ed.removeModule (m); return; }
        if (e.position.x < r.getX() + 28) { auto* p = ed.P (onParam (m)); p->setValueNotifyingHost (p->getValue() > 0.5f ? 0.0f : 1.0f); repaint(); return; }
        dragMod = m; grab = e.position.y - r.getY(); dragY = e.position.y;
        ed.gallery.setVisible (false); ed.openEffect (m); return;
    }
}
void Rack::mouseDrag (const juce::MouseEvent& e)
{
    if (dragMod < 0 || e.getDistanceFromDragStart() < 5) return;
    dragging = true; dragY = e.position.y; repaint();
}
void Rack::mouseUp (const juce::MouseEvent&)
{
    if (dragging && dragMod >= 0)
    {   // nuova posizione tra gli effetti in catena; quelli fuori catena restano dove sono
        auto v = visible(); v.erase (std::find (v.begin(), v.end(), dragMod));
        const int at = juce::jlimit (0, (int) v.size(), (int) ((dragY - 44) / 46));
        v.insert (v.begin() + at, dragMod);
        auto o = ed.proc.getOrder(); int k = 0;
        for (auto& x : o) if (ed.proc.inRack[x].load()) x = v[(size_t) k++];
        ed.proc.setOrder (o);
    }
    dragging = false; dragMod = -1; repaint();
}

// ---------------------------------------------------------------------------------------------------- galleria
// piccola anteprima disegnata: la forma tipica di quello che fa l'effetto
static void glyph (juce::Graphics& g, int m, juce::Rectangle<float> r, juce::Colour c)
{
    juce::Path p; const int N = 60;
    auto curve = [&] (std::function<float (float)> fn) { for (int i = 0; i <= N; ++i) { const float x = i / (float) N, y = juce::jlimit (0.0f, 1.0f, fn (x)); const float px = r.getX() + x * r.getWidth(), py = r.getBottom() - y * r.getHeight(); if (i == 0) p.startNewSubPath (px, py); else p.lineTo (px, py); } };
    switch (m)
    {
        case DYNEQ: case EQ432: curve ([m] (float x) { return 0.5f + (m == DYNEQ ? 0.3f * std::exp (-std::pow ((x - 0.3f) * 9, 2.0f)) - 0.25f * std::exp (-std::pow ((x - 0.7f) * 12, 2.0f)) : 0.12f * std::tanh ((x - 0.75f) * 8) - 0.1f * std::tanh ((0.2f - x) * 8)); }); break;
        case PULTEC: curve ([] (float x) { return 0.5f + 0.28f * std::exp (-std::pow (x * 4, 2.0f)) - 0.12f * std::exp (-std::pow ((x - 0.18f) * 9, 2.0f)) + 0.15f * std::exp (-std::pow ((x - 0.85f) * 6, 2.0f)); }); break;
        case BUSCOMP: case VARIMU: case LIMIT: case MAXIM:
            curve ([m] (float x) { const float t = m == LIMIT ? 0.8f : m == MAXIM ? 0.7f : 0.55f, k = m == LIMIT ? 0.02f : m == MAXIM ? 0.25f : m == VARIMU ? 0.35f : 0.5f;
                                   return x < t ? x : t + (x - t) * k; }); break;
        case MULTI: for (int b = 0; b < 3; ++b) { auto sr = r.withWidth (r.getWidth() / 3 - 4).translated (b * r.getWidth() / 3, 0); p.addRectangle (sr.withTrimmedTop (sr.getHeight() * (0.2f + 0.25f * b))); } break;
        case BASS: curve ([] (float x) { return x < 0.3f ? 0.75f : 0.75f - (x - 0.3f) * 0.6f; }); break;
        case DEESS: curve ([] (float x) { return 0.55f - 0.3f * std::exp (-std::pow ((x - 0.78f) * 10, 2.0f)); }); break;
        case EXCITER: case TAPE: curve ([m] (float x) { return 0.5f + 0.42f * std::tanh ((x - 0.5f) * (m == TAPE ? 3.0f : 6.0f)) / std::tanh (m == TAPE ? 1.5f : 3.0f); }); break;
        case IMAGER: p.addEllipse (r.getCentreX() - r.getHeight() * 0.7f, r.getY(), r.getHeight() * 1.4f, r.getHeight()); break;
        default: break;
    }
    g.setColour (c.withAlpha (0.25f)); g.strokePath (p, juce::PathStrokeType (4.0f)); g.setColour (c); g.strokePath (p, juce::PathStrokeType (1.6f));
}
void Gallery::paint (juce::Graphics& g)
{
    setTheme (ed.proc.uiTheme.load() == 2 ? 2 : 1);
    auto b = getLocalBounds().toFloat();
    g.setColour (K::panel1); g.fillRoundedRectangle (b, 16);
    g.setColour (K::hi.withAlpha (0.08f)); g.drawRoundedRectangle (b.reduced (0.5f), 16, 1.0f);
    g.setFont (cap (10.0f)); g.setColour (K::ink); g.drawText ("AGGIUNGI EFFETTO", juce::Rectangle<float> (22, 14, 300, 26), juce::Justification::centredLeft);
    g.setFont (f (9.5f)); g.setColour (K::sub); g.drawText ("entra nella catena al suo posto naturale, poi puoi spostarlo", juce::Rectangle<float> (220, 14, 420, 26), juce::Justification::centredLeft);
    neo::chip (g, closeR(), "CHIUDI", false, K::sub, 8.0f, hov == 99);
    static const int list[kNumMods] = { BASS, DYNEQ, EQ432, PULTEC, DEESS, VARIMU, BUSCOMP, MULTI, EXCITER, TAPE, IMAGER, MAXIM, LIMIT };
    for (int i = 0; i < kNumMods; ++i)
    {
        const int m = list[i]; auto r = card (i); const auto c = acc (juce::Colour (modInfo (m).colour)); const bool in = ed.proc.inRack[m].load();
        g.setColour (K::hi.withAlpha (hov == i && ! in ? 0.08f : 0.04f)); g.fillRoundedRectangle (r, 12);
        g.setColour (hov == i && ! in ? c.withAlpha (0.8f) : K::hi.withAlpha (0.07f)); g.drawRoundedRectangle (r.reduced (0.5f), 12, 1.0f);
        auto gr = juce::Rectangle<float> (r.getRight() - 70, r.getY() + 12, 56, 30);
        g.setColour (K::lo.withAlpha (0.25f)); g.fillRoundedRectangle (gr.expanded (4), 6); glyph (g, m, gr, in ? K::faint : c);
        g.setFont (cap (8.5f)); g.setColour (in ? K::faint : K::ink); g.drawText (modInfo (m).name, juce::Rectangle<float> (r.getX() + 12, r.getY() + 10, r.getWidth() - 90, 16), juce::Justification::centredLeft);
        g.setFont (f (8.5f)); g.setColour (in ? K::faint : c); g.drawText (modInfo (m).sub, juce::Rectangle<float> (r.getX() + 12, r.getY() + 27, r.getWidth() - 90, 14), juce::Justification::centredLeft);
        g.setFont (f (8.8f)); g.setColour (in ? K::faint : K::sub);
        g.drawFittedText (modText (m).what, juce::Rectangle<int> ((int) r.getX() + 12, (int) r.getY() + 50, (int) r.getWidth() - 22, 34), juce::Justification::topLeft, 2, 0.9f);
        g.setColour (in ? K::faint : K::ink.withAlpha (0.75f));
        g.drawFittedText (in ? juce::String ("gia in catena") : juce::String ("quando: ") + modText (m).when, juce::Rectangle<int> ((int) r.getX() + 12, (int) r.getY() + 88, (int) r.getWidth() - 22, 34), juce::Justification::topLeft, 2, 0.9f);
    }
}
void Gallery::mouseMove (const juce::MouseEvent& e)
{
    int h = closeR().contains (e.position) ? 99 : -1; for (int i = 0; i < kNumMods; ++i) if (card (i).contains (e.position)) h = i;
    if (h != hov) { hov = h; repaint(); }
    setMouseCursor (h >= 0 ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
}
void Gallery::mouseDown (const juce::MouseEvent& e)
{
    static const int list[kNumMods] = { BASS, DYNEQ, EQ432, PULTEC, DEESS, VARIMU, BUSCOMP, MULTI, EXCITER, TAPE, IMAGER, MAXIM, LIMIT };
    if (closeR().contains (e.position)) { setVisible (false); ed.rack.repaint(); return; }
    for (int i = 0; i < kNumMods; ++i) if (card (i).contains (e.position))
    {
        const int m = list[i];
        if (ed.proc.inRack[m].load()) { setVisible (false); ed.openEffect (m); } else ed.insertModule (m);
        return;
    }
}

// ---------------------------------------------------------------------------------------------------- barra di stato
void StatusBar::paint (juce::Graphics& g)
{
    setTheme (ed.proc.uiTheme.load() == 2 ? 2 : 1);
    auto b = getLocalBounds().toFloat();
    g.setColour (K::lo.withAlpha (light() ? 0.06f : 0.3f)); g.fillRoundedRectangle (b, 10);
    g.setColour (K::hi.withAlpha (0.05f)); g.drawRoundedRectangle (b.reduced (0.5f), 10, 1.0f);
    juce::String name, doing, tip; juce::Colour c = K::violet;
    if (ed.sel < kNumMods && ed.proc.inRack[ed.sel].load())
    {
        const auto st = statOf (ed, ed.sel); name = modInfo (ed.sel).name; doing = st.doing; tip = st.tip; c = acc (juce::Colour (modInfo (ed.sel).colour));
    }
    else
    {   // nessun effetto aperto: loudness rispetto al target
        const float lu = ed.proc.lufsS.load(), tgt = target ((int) ed.proc.value ("target")).lufs;
        name = "MASTER"; doing = lu > -60 ? "short-term " + juce::String (lu, 1) + " LUFS, obiettivo " + juce::String ((int) tgt) : juce::String ("in attesa del segnale");
        tip = lu < -60 ? juce::String ("Fai partire la traccia: le misure si accendono da sole.") : lu < tgt - 2 ? juce::String ("Sotto il target: alza il gain del MAXIMIZER.") : lu > tgt + 1.5f ? juce::String ("Sopra il target: lo streaming ti abbassera, guadagni solo distorsione.") : juce::String ("Sei sul target: ora conta solo come suona.");
    }
    g.setColour (c); g.fillEllipse (b.getX() + 12, b.getCentreY() - 4, 8, 8);
    g.setFont (cap (8.5f)); g.setColour (K::ink); g.drawText (name, juce::Rectangle<float> (b.getX() + 28, b.getY(), 150, b.getHeight()), juce::Justification::centredLeft);
    g.setFont (num (9.5f)); g.setColour (c); g.drawFittedText (doing, juce::Rectangle<int> ((int) b.getX() + 170, 0, 470, getHeight()), juce::Justification::centredLeft, 1, 0.8f);
    g.setColour (K::hi.withAlpha (0.1f)); g.fillRect (b.getX() + 650, b.getY() + 8, 1.0f, b.getHeight() - 16);
    g.setFont (cap (7.0f)); g.setColour (K::mint); g.drawText ("CONSIGLIO", juce::Rectangle<float> (b.getX() + 664, b.getY(), 70, b.getHeight()), juce::Justification::centredLeft);
    g.setFont (f (9.5f)); g.setColour (K::sub); g.drawFittedText (tip, juce::Rectangle<int> ((int) b.getX() + 736, 0, (int) b.getWidth() - 836, getHeight()), juce::Justification::centredLeft, 2, 0.85f);
}

// ---------------------------------------------------------------------------------------------------- scheda dei preset
juce::StringArray PresetBrowser::groups() const
{
    juce::StringArray g; g.add ("TUTTI");
    for (auto& p : presets()) { const juce::String n (p.name); const auto gn = n.contains (" | ") ? n.upToFirstOccurrenceOf (" | ", false, false) : juce::String ("Base"); if (! g.contains (gn)) g.add (gn); }
    return g;
}
std::vector<int> PresetBrowser::items (int gi) const
{
    const auto gs = groups(); std::vector<int> v;
    for (int i = 0; i < (int) presets().size(); ++i)
    {
        const juce::String n (presets()[(size_t) i].name); const auto gn = n.contains (" | ") ? n.upToFirstOccurrenceOf (" | ", false, false) : juce::String ("Base");
        if (gi == 0 || gs[gi] == gn) v.push_back (i);
    }
    return v;
}
void PresetBrowser::open()
{
    // si apre sul gruppo del preset attuale
    const juce::String n (presets()[(size_t) juce::jlimit (0, (int) presets().size() - 1, ed.proc.preset)].name);
    const auto gn = n.contains (" | ") ? n.upToFirstOccurrenceOf (" | ", false, false) : juce::String ("Base");
    group = juce::jmax (0, groups().indexOf (gn)); scroll = 0; hov = hovG = -1;
    setVisible (true); toFront (true);
}
static juce::Rectangle<float> grpCell (juce::Rectangle<float> bx, int i) { return { bx.getX() + 16, bx.getY() + 60 + i * 40.0f, 200, 34 }; }
static constexpr float kRowH = 58;
void PresetBrowser::paint (juce::Graphics& g)
{
    setTheme (ed.proc.uiTheme.load() == 2 ? 2 : 1);
    g.fillAll (K::lo.withAlpha (light() ? 0.25f : 0.6f));
    auto bx = box();
    g.setColour (K::lo.withAlpha (0.4f)); g.fillRoundedRectangle (bx.translated (0, 6), 20);
    juce::ColourGradient bg (K::panel0, 0, bx.getY(), K::panel1, 0, bx.getBottom(), false); g.setGradientFill (bg); g.fillRoundedRectangle (bx, 20);
    g.setColour (K::hi.withAlpha (0.08f)); g.drawRoundedRectangle (bx.reduced (0.5f), 20, 1.0f);
    g.setFont (big (20.0f)); g.setColour (K::ink); g.drawText ("Preset", juce::Rectangle<float> (bx.getX() + 22, bx.getY() + 14, 200, 32), juce::Justification::centredLeft);
    g.setFont (f (10.5f)); g.setColour (K::sub); g.drawText (juce::String::fromUTF8 ("clic per caricare  \xc2\xb7  clic fuori per chiudere"), juce::Rectangle<float> (bx.getX() + 130, bx.getY() + 14, 400, 32), juce::Justification::centredLeft);
    const auto gs = groups();
    for (int i = 0; i < gs.size(); ++i) neo::chip (g, grpCell (bx, i), gs[i].toUpperCase(), i == group, K::violet, 10.0f, hovG == i);
    // elenco
    auto list = juce::Rectangle<float> (bx.getX() + 236, bx.getY() + 60, bx.getWidth() - 256, bx.getHeight() - 76);
    g.saveState(); g.reduceClipRegion (list.toNearestInt());
    const auto it = items (group);
    for (int k = 0; k < (int) it.size(); ++k)
    {
        const int i = it[(size_t) k]; auto r = juce::Rectangle<float> (list.getX(), list.getY() + k * kRowH - scroll, list.getWidth(), kRowH - 6);
        if (r.getBottom() < list.getY() || r.getY() > list.getBottom()) continue;
        const bool cur = i == ed.proc.preset, h = hov == i;
        g.setColour (cur ? K::violet.withAlpha (0.18f) : K::hi.withAlpha (h ? 0.07f : 0.03f)); g.fillRoundedRectangle (r, 10);
        if (cur) { g.setColour (K::violet.withAlpha (0.7f)); g.drawRoundedRectangle (r.reduced (0.5f), 10, 1.0f); }
        juce::String n (presets()[(size_t) i].name); if (n.contains (" | ")) n = n.fromFirstOccurrenceOf (" | ", false, false);
        g.setFont (f (14.0f, 1)); g.setColour (K::ink); g.drawText (n, r.withTrimmedLeft (16).withHeight (26).translated (0, 4), juce::Justification::centredLeft);
        g.setFont (f (9.8f)); g.setColour (K::sub); g.drawFittedText (presets()[(size_t) i].note, r.withTrimmedLeft (16).withTrimmedRight (12).withTrimmedTop (28).toNearestInt(), juce::Justification::topLeft, 1, 0.85f);
    }
    g.restoreState();
    const float total = it.size() * kRowH;
    if (total > list.getHeight()) { const float h = list.getHeight() * list.getHeight() / total, y = list.getY() + (list.getHeight() - h) * scroll / (total - list.getHeight()); g.setColour (K::hi.withAlpha (0.15f)); g.fillRoundedRectangle (list.getRight() + 6, y, 4, h, 2); }
}
void PresetBrowser::mouseMove (const juce::MouseEvent& e)
{
    auto bx = box(); int h = -1, hg = -1;
    const auto gs = groups(); for (int i = 0; i < gs.size(); ++i) if (grpCell (bx, i).contains (e.position)) hg = i;
    auto list = juce::Rectangle<float> (bx.getX() + 236, bx.getY() + 60, bx.getWidth() - 256, bx.getHeight() - 76);
    if (list.contains (e.position)) { const auto it = items (group); const int k = (int) ((e.position.y - list.getY() + scroll) / kRowH); if (k >= 0 && k < (int) it.size()) h = it[(size_t) k]; }
    if (h != hov || hg != hovG) { hov = h; hovG = hg; repaint(); }
    setMouseCursor (h >= 0 || hg >= 0 ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
}
void PresetBrowser::mouseDown (const juce::MouseEvent& e)
{
    auto bx = box();
    if (! bx.contains (e.position)) { setVisible (false); return; }
    const auto gs = groups(); for (int i = 0; i < gs.size(); ++i) if (grpCell (bx, i).contains (e.position)) { group = i; scroll = 0; repaint(); return; }
    if (hov >= 0) { ed.proc.loadPreset (hov); setVisible (false); if (! ed.proc.easy()) { ed.rack.repaint(); const auto v = ed.rack.visible(); if (ed.advView == Editor::kEff && ! v.empty() && (ed.sel >= kNumMods || ! ed.proc.inRack[ed.sel].load())) ed.openEffect (v.back()); } ed.repaint(); }
}
void PresetBrowser::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w)
{
    auto bx = box(); const float lh = bx.getHeight() - 76, total = items (group).size() * kRowH;
    scroll = juce::jlimit (0.0f, juce::jmax (0.0f, total - lh), scroll - w.deltaY * 220.0f); repaint();
}
}
