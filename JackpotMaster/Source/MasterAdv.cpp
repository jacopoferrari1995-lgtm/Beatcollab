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


// ---------------------------------------------------------------------------------------------------- miniature hardware
// pannelli ispirati agli strumenti classici da studio (colori, manopole, strumenti), senza marchi ne scritte di marca
static void screws (juce::Graphics& g, juce::Rectangle<float> r)
{
    if (r.getWidth() < 70) return;
    const float d = juce::jmax (2.8f, r.getHeight() * 0.075f);
    for (float x : { r.getX() + d * 1.7f, r.getRight() - d * 1.7f }) for (float y : { r.getY() + d * 1.7f, r.getBottom() - d * 1.7f })
    {   // vite a croce con riflesso
        g.setColour (juce::Colours::black.withAlpha (0.45f)); g.fillEllipse (x - d * 0.5f, y - d * 0.5f + 0.5f, d, d);
        juce::ColourGradient sg (juce::Colour (0xffe9eaee), x - d * 0.3f, y - d * 0.3f, juce::Colour (0xff6b6e75), x + d * 0.4f, y + d * 0.4f, false); g.setGradientFill (sg); g.fillEllipse (x - d * 0.5f, y - d * 0.5f, d, d);
        g.setColour (juce::Colours::black.withAlpha (0.55f)); g.drawLine (x - d * 0.32f, y, x + d * 0.32f, y, 0.6f); g.drawLine (x, y - d * 0.32f, x, y + d * 0.32f, 0.6f);
    }
}
static void plateBody (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour top, juce::Colour bot)
{
    const float rad = juce::jmin (5.0f, r.getHeight() * 0.12f);
    g.setColour (juce::Colours::black.withAlpha (0.45f)); g.fillRoundedRectangle (r.translated (0, 2.0f), rad);
    juce::ColourGradient cg (top, 0, r.getY(), bot, 0, r.getBottom(), false); g.setGradientFill (cg); g.fillRoundedRectangle (r, rad);
    // metallo spazzolato: righe finissime casuali
    { g.saveState(); g.reduceClipRegion (r.toNearestInt()); juce::Random rr ((juce::int64) top.getARGB());
      for (float y = r.getY() + 1; y < r.getBottom(); y += 1.0f) { g.setColour ((rr.nextBool() ? juce::Colours::white : juce::Colours::black).withAlpha (0.02f + rr.nextFloat() * 0.035f)); g.fillRect (r.getX(), y, r.getWidth(), 0.6f); }
      // riflesso morbido in diagonale (levigatura)
      juce::ColourGradient sh (juce::Colours::white.withAlpha (0.16f), r.getX(), r.getY(), juce::Colours::white.withAlpha (0.0f), r.getX() + r.getWidth() * 0.55f, r.getBottom(), false); g.setGradientFill (sh); g.fillRect (r);
      g.restoreState(); }
    // bisello: luce sopra, ombra sotto
    g.setColour (juce::Colours::white.withAlpha (0.35f)); g.drawLine (r.getX() + rad, r.getY() + 0.7f, r.getRight() - rad, r.getY() + 0.7f, 0.9f);
    g.setColour (juce::Colours::black.withAlpha (0.4f)); g.drawLine (r.getX() + rad, r.getBottom() - 0.7f, r.getRight() - rad, r.getBottom() - 0.7f, 0.9f);
    g.setColour (juce::Colours::black.withAlpha (0.35f)); g.drawRoundedRectangle (r.reduced (0.4f), rad, 0.8f);
    // orecchie da rack con le asole per le viti (solo sulle miniature grandi)
    if (r.getWidth() > 90)
    {
        const float ew = r.getHeight() * 0.11f;
        for (float x : { r.getX() + ew * 0.6f, r.getRight() - ew * 1.6f })
        { g.setColour (juce::Colours::black.withAlpha (0.25f)); g.fillRect (x + ew * 0.5f, r.getY() + 2, 0.8f, r.getHeight() - 4);
          for (float yy : { r.getY() + r.getHeight() * 0.22f, r.getBottom() - r.getHeight() * 0.22f }) { g.setColour (juce::Colours::black.withAlpha (0.6f)); g.fillRoundedRectangle (x, yy - ew * 0.35f, ew, ew * 0.7f, ew * 0.35f); } }
    }
}
static void led (juce::Graphics& g, float cx, float cy, float d, juce::Colour c, bool on = true)
{
    if (on) { g.setColour (c.withAlpha (0.35f)); g.fillEllipse (cx - d, cy - d, d * 2, d * 2); }
    juce::ColourGradient lg (on ? c.brighter (0.7f) : c.darker (0.8f), cx - d * 0.2f, cy - d * 0.3f, on ? c : c.darker (1.2f), cx + d * 0.4f, cy + d * 0.5f, false); g.setGradientFill (lg); g.fillEllipse (cx - d * 0.5f, cy - d * 0.5f, d, d);
}
static void miniKnob (juce::Graphics& g, float cx, float cy, float R, juce::Colour cap, float ang = 0.6f, bool chicken = false)
{
    // tacche della scala intorno
    if (R > 5) for (int i = 0; i <= 10; ++i) { const float a = -2.4f + 4.8f * i / 10.0f, r0 = R * 1.18f, r1 = R * 1.36f; g.setColour (juce::Colours::white.withAlpha (0.55f)); g.drawLine (cx + std::sin (a) * r0, cy - std::cos (a) * r0, cx + std::sin (a) * r1, cy - std::cos (a) * r1, juce::jmax (0.5f, R * 0.06f)); }
    g.setColour (juce::Colours::black.withAlpha (0.5f)); g.fillEllipse (cx - R, cy - R + R * 0.22f, R * 2, R * 2);
    if (chicken)
    {   // manopola a becco in bachelite nera lucida
        juce::Path p; p.addEllipse (cx - R * 0.78f, cy - R * 0.78f, R * 1.56f, R * 1.56f);
        juce::Path b; b.addTriangle (cx - R * 0.38f, cy, cx + R * 0.38f, cy, cx, cy - R * 1.28f); b.applyTransform (juce::AffineTransform::rotation (ang, cx, cy)); p.addPath (b);
        juce::ColourGradient kg (cap.brighter (0.5f), cx - R * 0.5f, cy - R * 0.7f, cap, cx + R * 0.4f, cy + R * 0.6f, false); g.setGradientFill (kg); g.fillPath (p);
        g.setColour (juce::Colours::white.withAlpha (0.25f)); g.fillEllipse (cx - R * 0.45f, cy - R * 0.55f, R * 0.5f, R * 0.3f);
        g.setColour (juce::Colours::white.withAlpha (0.85f)); g.drawLine (cx, cy, cx + std::sin (ang) * R * 1.12f, cy - std::cos (ang) * R * 1.12f, juce::jmax (0.8f, R * 0.12f)); return;
    }
    // gonna zigrinata scura, poi il cappuccio colorato con riflesso
    g.setColour (juce::Colour (0xff17181c)); g.fillEllipse (cx - R, cy - R, R * 2, R * 2);
    if (R > 5) for (int i = 0; i < 24; ++i) { const float a = i * 0.2618f; g.setColour (juce::Colours::white.withAlpha (0.10f)); g.drawLine (cx + std::sin (a) * R * 0.8f, cy - std::cos (a) * R * 0.8f, cx + std::sin (a) * R * 0.97f, cy - std::cos (a) * R * 0.97f, 0.6f); }
    const float cr = R * 0.72f;
    juce::ColourGradient kg (cap.brighter (0.45f), cx - cr * 0.5f, cy - cr * 0.6f, cap.darker (0.55f), cx + cr * 0.5f, cy + cr * 0.7f, false); g.setGradientFill (kg); g.fillEllipse (cx - cr, cy - cr, cr * 2, cr * 2);
    g.setColour (juce::Colours::white.withAlpha (0.3f)); g.fillEllipse (cx - cr * 0.55f, cy - cr * 0.7f, cr * 0.7f, cr * 0.4f);
    g.setColour (juce::Colours::white.withAlpha (0.9f)); g.drawLine (cx + std::sin (ang) * cr * 0.2f, cy - std::cos (ang) * cr * 0.2f, cx + std::sin (ang) * R * 0.95f, cy - std::cos (ang) * R * 0.95f, juce::jmax (0.8f, R * 0.14f));
}
static void vuWin (juce::Graphics& g, juce::Rectangle<float> v, float needle)
{
    const float rad = juce::jmin (3.0f, v.getHeight() * 0.12f);
    juce::ColourGradient f (juce::Colour (0xff4a3518), v.getCentreX(), v.getBottom(), juce::Colour (0xff1a120a), v.getCentreX(), v.getY(), false); g.setGradientFill (f); g.fillRoundedRectangle (v, rad);
    g.setColour (juce::Colour (0x66ffb547)); g.fillRoundedRectangle (v.withTrimmedTop (v.getHeight() * 0.5f), rad);
    const float cx = v.getCentreX(), cy = v.getBottom() + v.getHeight() * 0.25f, R = v.getHeight() * 1.0f;
    juce::Path sc; sc.addCentredArc (cx, cy, R * 0.82f, R * 0.82f, 0, -0.75f, 0.75f, true); g.setColour (juce::Colour (0xffe9dcbd).withAlpha (0.7f)); g.strokePath (sc, juce::PathStrokeType (juce::jmax (0.6f, v.getHeight() * 0.04f)));
    { juce::Path rd; rd.addCentredArc (cx, cy, R * 0.82f, R * 0.82f, 0, 0.35f, 0.75f, true); g.setColour (juce::Colour (0xffff6a4d)); g.strokePath (rd, juce::PathStrokeType (juce::jmax (0.8f, v.getHeight() * 0.06f))); }
    const float a = -0.75f + 1.5f * needle;
    g.saveState(); g.reduceClipRegion (v.toNearestInt());
    g.setColour (juce::Colours::black.withAlpha (0.35f)); g.drawLine (cx + 1.2f, cy, cx + 1.2f + std::sin (a) * R * 0.95f, cy - std::cos (a) * R * 0.95f, juce::jmax (0.7f, v.getHeight() * 0.04f));   // ombra dell'ago
    g.setColour (juce::Colour (0xfff3e7cc)); g.drawLine (cx, cy, cx + std::sin (a) * R * 0.95f, cy - std::cos (a) * R * 0.95f, juce::jmax (0.7f, v.getHeight() * 0.04f));
    { juce::Path gl; gl.addQuadrilateral (v.getX(), v.getY(), v.getX() + v.getWidth() * 0.55f, v.getY(), v.getX() + v.getWidth() * 0.35f, v.getBottom(), v.getX(), v.getBottom());   // riflesso del vetro
      g.setColour (juce::Colours::white.withAlpha (0.10f)); g.fillPath (gl); }
    g.restoreState();
    g.setColour (juce::Colour (0xff0d0d0f)); g.drawRoundedRectangle (v.expanded (0.8f), rad + 0.8f, 1.4f);   // cornice
    g.setColour (juce::Colours::white.withAlpha (0.25f)); g.drawLine (v.getX() + rad, v.getBottom() + 1.6f, v.getRight() - rad, v.getBottom() + 1.6f, 0.6f);
}
static void reel (juce::Graphics& g, float cx, float cy, float R, float rot)
{
    g.setColour (juce::Colour (0xffd9dbe0)); g.fillEllipse (cx - R, cy - R, R * 2, R * 2);
    g.setColour (juce::Colour (0xff3a2a1e)); g.fillEllipse (cx - R * 0.78f, cy - R * 0.78f, R * 1.56f, R * 1.56f);   // nastro avvolto
    g.setColour (juce::Colour (0xffd9dbe0)); g.fillEllipse (cx - R * 0.3f, cy - R * 0.3f, R * 0.6f, R * 0.6f);
    for (int k = 0; k < 3; ++k) { const float a = rot + k * 2.094f; juce::Path h; h.addEllipse (cx + std::sin (a) * R * 0.5f - R * 0.16f, cy - std::cos (a) * R * 0.5f - R * 0.16f, R * 0.32f, R * 0.32f); g.setColour (juce::Colour (0xff15110d)); g.fillPath (h); }
    g.setColour (juce::Colour (0xff15110d)); g.fillEllipse (cx - R * 0.08f, cy - R * 0.08f, R * 0.16f, R * 0.16f);
}
void hwIcon (juce::Graphics& g, int m, juce::Rectangle<float> r, bool on)
{
    g.saveState();
    const float H = r.getHeight(), W = r.getWidth(); auto in = r.reduced (W * 0.07f, H * 0.12f);
    switch (m)
    {
        case BASS:
        {   plateBody (g, r, juce::Colour (0xff2d2a29), juce::Colour (0xff181615));
            const float R = H * 0.38f, cx = r.getRight() - R - W * 0.08f, cy = r.getCentreY();
            g.setColour (juce::Colour (0xff0d0c0c)); g.fillEllipse (cx - R, cy - R, R * 2, R * 2);
            g.setColour (juce::Colour (0xff4a4644)); g.drawEllipse (cx - R * 0.92f, cy - R * 0.92f, R * 1.84f, R * 1.84f, R * 0.12f);
            juce::ColourGradient cg (juce::Colour (0xff3b3735), cx - R * 0.3f, cy - R * 0.3f, juce::Colour (0xff0f0e0e), cx + R * 0.5f, cy + R * 0.6f, true); g.setGradientFill (cg); g.fillEllipse (cx - R * 0.72f, cy - R * 0.72f, R * 1.44f, R * 1.44f);
            g.setColour (juce::Colour (0xff57514d)); g.fillEllipse (cx - R * 0.24f, cy - R * 0.24f, R * 0.48f, R * 0.48f);
            miniKnob (g, r.getX() + W * 0.17f, cy, H * 0.15f, juce::Colour (0xffffb000), -0.6f); miniKnob (g, r.getX() + W * 0.37f, cy, H * 0.15f, juce::Colour (0xffffb000), 0.8f);
            break; }
        case DYNEQ:
        {   plateBody (g, r, juce::Colour (0xff1a1e26), juce::Colour (0xff0c0e12));
            auto sc = in; g.setColour (juce::Colour (0xff06080b)); g.fillRoundedRectangle (sc, 3);
            juce::Path sp; for (int i = 0; i <= 40; ++i) { const float x = i / 40.0f; const float y = 0.75f - 0.25f * x + 0.08f * std::sin (x * 23.0f) * (1 - x); if (i == 0) sp.startNewSubPath (sc.getX(), sc.getBottom()); sp.lineTo (sc.getX() + x * sc.getWidth(), sc.getY() + y * sc.getHeight()); }
            sp.lineTo (sc.getRight(), sc.getBottom()); sp.closeSubPath(); g.setColour (juce::Colour (0x333ee8ff)); g.fillPath (sp);
            juce::Path cv; for (int i = 0; i <= 50; ++i) { const float x = i / 50.0f, y = 0.5f - 0.28f * std::exp (-std::pow ((x - 0.3f) * 8, 2.0f)) + 0.22f * std::exp (-std::pow ((x - 0.72f) * 11, 2.0f)); const float px = sc.getX() + x * sc.getWidth(), py = sc.getY() + y * sc.getHeight(); if (i == 0) cv.startNewSubPath (px, py); else cv.lineTo (px, py); }
            g.setColour (juce::Colours::white.withAlpha (0.9f)); g.strokePath (cv, juce::PathStrokeType (juce::jmax (1.0f, H * 0.03f)));
            const juce::Colour nc[3] = { juce::Colour (0xffffb547), juce::Colour (0xffff3d9a), juce::Colour (0xff3ee8ff) }; const float nx[3] = { 0.3f, 0.72f, 0.88f }, ny[3] = { 0.22f, 0.72f, 0.5f };
            for (int k = 0; k < 3; ++k) { const float d = juce::jmax (3.0f, H * 0.11f); g.setColour (nc[k]); g.drawEllipse (sc.getX() + nx[k] * sc.getWidth() - d * 0.5f, sc.getY() + ny[k] * sc.getHeight() - d * 0.5f, d, d, juce::jmax (0.8f, H * 0.025f)); }
            break; }
        case EQ432:
        {   plateBody (g, r, juce::Colour (0xffc9ccd2), juce::Colour (0xff8e9299)); screws (g, r);
            const juce::Colour caps[5] = { juce::Colour (0xffd23b3b), juce::Colour (0xffe8b23a), juce::Colour (0xff3fae5c), juce::Colour (0xff3b7bd2), juce::Colour (0xff8b5cd2) };
            for (int k = 0; k < 5; ++k) miniKnob (g, in.getX() + in.getWidth() * (k + 0.5f) / 5.0f, r.getCentreY(), juce::jmin (H * 0.17f, W * 0.07f), caps[k], -0.8f + k * 0.4f);
            break; }
        case PULTEC:
        {   plateBody (g, r, juce::Colour (0xff7a8fa3), juce::Colour (0xff4c5f71)); screws (g, r);
            led (g, r.getRight() - W * 0.07f, r.getBottom() - H * 0.2f, juce::jmax (2.0f, H * 0.07f), juce::Colour (0xffff3b2a));   // spia di accensione rossa
            for (int k = 0; k < 4; ++k) miniKnob (g, in.getX() + in.getWidth() * (k + 0.5f) / 4.0f, r.getCentreY() + H * 0.04f, juce::jmin (H * 0.22f, W * 0.09f), juce::Colour (0xff141414), -0.9f + k * 0.55f, true);
            g.setColour (juce::Colours::white.withAlpha (0.55f)); g.fillRect (in.getX(), r.getY() + H * 0.16f, in.getWidth(), juce::jmax (0.6f, H * 0.02f));
            break; }
        case DEESS:
        {   plateBody (g, r, juce::Colour (0xff3a3d44), juce::Colour (0xff22242a));
            auto ml = juce::Rectangle<float> (in.getX(), r.getCentreY() - H * 0.07f, in.getWidth() * 0.6f, H * 0.14f);
            for (int k = 0; k < 10; ++k) { const float t = k / 10.0f; g.setColour ((t > 0.75f ? juce::Colour (0xffff4d6a) : t > 0.55f ? juce::Colour (0xffffb547) : juce::Colour (0xff4dffb4)).withAlpha (k < 7 ? 1.0f : 0.3f)); g.fillRect (ml.getX() + k * ml.getWidth() / 10, ml.getY(), ml.getWidth() / 10 - 1, ml.getHeight()); }
            miniKnob (g, in.getRight() - in.getWidth() * 0.15f, r.getCentreY(), H * 0.2f, juce::Colour (0xff7dff5a), 0.4f);
            break; }
        case VARIMU:
        {   plateBody (g, r, juce::Colour (0xff8fb0cc), juce::Colour (0xff5d7e9c)); screws (g, r);
            const float vw = in.getWidth() * 0.4f, vh = in.getHeight() * 0.8f;
            vuWin (g, { in.getX(), r.getCentreY() - vh * 0.5f, vw, vh }, 0.35f); vuWin (g, { in.getRight() - vw, r.getCentreY() - vh * 0.5f, vw, vh }, 0.42f);
            led (g, r.getCentreX(), r.getCentreY(), juce::jmax (2.5f, H * 0.08f), juce::Colour (0xffff4a3a));
            break; }
        case BUSCOMP:
        {   plateBody (g, r, juce::Colour (0xff9a9da3), juce::Colour (0xff6e7177)); screws (g, r);
            led (g, r.getRight() - W * 0.08f, r.getY() + H * 0.2f, juce::jmax (2.0f, H * 0.06f), juce::Colour (0xff4dff6a));
            const float vh = in.getHeight() * 0.85f; vuWin (g, { in.getX(), r.getCentreY() - vh * 0.5f, in.getWidth() * 0.42f, vh }, 0.62f);
            const juce::Colour caps[4] = { juce::Colour (0xffd23b3b), juce::Colour (0xff2e5fb0), juce::Colour (0xff2e5fb0), juce::Colour (0xfff0f0f0) };
            for (int k = 0; k < 4; ++k) miniKnob (g, in.getX() + in.getWidth() * (0.55f + (k % 2) * 0.25f) + in.getWidth() * 0.08f, r.getCentreY() + (k < 2 ? -1.0f : 1.0f) * H * 0.2f, juce::jmin (H * 0.13f, W * 0.05f), caps[k], -0.5f + k * 0.4f);
            break; }
        case MULTI:
        {   plateBody (g, r, juce::Colour (0xff1c1c1f), juce::Colour (0xff0e0e10));
            for (int k = 0; k < 3; ++k) { auto br = juce::Rectangle<float> (in.getX() + in.getWidth() * (0.08f + k * 0.3f), in.getY(), in.getWidth() * 0.2f, in.getHeight());
              g.setColour (juce::Colours::white.withAlpha (0.06f)); g.fillRect (br); const float lv = 0.4f + 0.2f * k;
              juce::ColourGradient bg (juce::Colour (0xffff7a3d), 0, br.getBottom(), juce::Colour (0xffffd23d), 0, br.getY(), false); g.setGradientFill (bg); g.fillRect (br.withTrimmedTop (br.getHeight() * (1 - lv)));
              g.setColour (juce::Colour (0xff4dc3ff)); g.fillRect (br.getX(), br.getY() + br.getHeight() * 0.25f, br.getWidth(), juce::jmax (1.0f, H * 0.03f)); }
            break; }
        case EXCITER:
        {   plateBody (g, r, juce::Colour (0xff27497f), juce::Colour (0xff142a4d)); screws (g, r);
            led (g, r.getX() + W * 0.1f, r.getY() + H * 0.2f, juce::jmax (2.0f, H * 0.06f), juce::Colour (0xffffa23d));
            for (int k = 0; k < 3; ++k) miniKnob (g, in.getX() + in.getWidth() * (k + 0.5f) / 5.0f, r.getCentreY(), juce::jmin (H * 0.16f, W * 0.06f), juce::Colour (0xffd8dbe2), -0.4f + k * 0.5f);
            auto wv = juce::Rectangle<float> (in.getX() + in.getWidth() * 0.62f, in.getY(), in.getWidth() * 0.38f, in.getHeight());
            juce::Path p; for (int i = 0; i <= 30; ++i) { const float x = i / 30.0f, y = 0.5f - 0.38f * std::tanh (std::sin (x * 12.6f) * 2.5f) / std::tanh (2.5f); if (i == 0) p.startNewSubPath (wv.getX(), wv.getY() + y * wv.getHeight()); else p.lineTo (wv.getX() + x * wv.getWidth(), wv.getY() + y * wv.getHeight()); }
            g.setColour (juce::Colour (0xffffa23d)); g.strokePath (p, juce::PathStrokeType (juce::jmax (1.0f, H * 0.035f)));
            break; }
        case TAPE:
        {   plateBody (g, r, juce::Colour (0xff3b2f26), juce::Colour (0xff1e1712));
            const float R = juce::jmin (H * 0.36f, W * 0.18f);
            const float x1 = r.getX() + W * 0.27f, x2 = r.getRight() - W * 0.27f, cy = r.getCentreY() - H * 0.04f;
            g.setColour (juce::Colour (0xff6b4a30)); g.drawLine (x1, cy + R * 0.78f, x2, cy + R * 0.78f, juce::jmax (1.0f, H * 0.04f));
            reel (g, x1, cy, R, 0.3f); reel (g, x2, cy, R, 1.1f);
            break; }
        case IMAGER:
        {   plateBody (g, r, juce::Colour (0xff1a2226), juce::Colour (0xff0c1113));
            const float S = juce::jmin (in.getHeight(), in.getWidth() * 0.5f); auto sq = juce::Rectangle<float> (r.getCentreX() - S * 0.5f, r.getCentreY() - S * 0.5f, S, S);
            g.setColour (juce::Colour (0xff3dffd2).withAlpha (0.25f)); g.drawEllipse (sq, 0.8f); g.drawLine (sq.getX(), sq.getY(), sq.getRight(), sq.getBottom(), 0.6f); g.drawLine (sq.getRight(), sq.getY(), sq.getX(), sq.getBottom(), 0.6f);
            juce::Random rr (11); g.setColour (juce::Colour (0xff3dffd2).withAlpha (0.8f));
            for (int i = 0; i < 70; ++i) { const float a = rr.nextFloat() * 6.283f, d = std::pow (rr.nextFloat(), 1.5f) * S * 0.42f; g.fillRect (sq.getCentreX() + std::sin (a) * d * 0.45f, sq.getCentreY() - std::cos (a) * d, 1.0f, 1.0f); }
            break; }
        case MAXIM:
        {   plateBody (g, r, juce::Colour (0xff1b1a18), juce::Colour (0xff0b0a09));
            g.setColour (juce::Colour (0xffc9a24a)); g.fillRect (r.getX() + W * 0.05f, r.getY() + H * 0.1f, W * 0.9f, juce::jmax (0.7f, H * 0.025f));
            for (int k = 0; k < 2; ++k) { const float x = in.getX() + in.getWidth() * (0.18f + k * 0.22f); g.setColour (juce::Colours::black); g.fillRect (x - 1.0f, in.getY() + H * 0.05f, 2.0f, in.getHeight() * 0.9f);
              auto cap = juce::Rectangle<float> (x - W * 0.045f, in.getY() + in.getHeight() * (k ? 0.25f : 0.55f), W * 0.09f, H * 0.14f); g.setColour (juce::Colour (0xffd9d4c7)); g.fillRoundedRectangle (cap, 1.5f); g.setColour (juce::Colours::black); g.fillRect (cap.getX(), cap.getCentreY() - 0.4f, cap.getWidth(), 0.8f); }
            auto mt = juce::Rectangle<float> (in.getX() + in.getWidth() * 0.62f, in.getY(), in.getWidth() * 0.34f, in.getHeight());
            for (int k = 0; k < 2; ++k) { auto c = mt.withWidth (mt.getWidth() * 0.4f).translated (k * mt.getWidth() * 0.55f, 0); g.setColour (juce::Colours::white.withAlpha (0.06f)); g.fillRect (c); g.setColour (juce::Colour (0xffff3b5c)); g.fillRect (c.withHeight (c.getHeight() * (0.3f + 0.15f * k))); }
            break; }
        case LIMIT:
        {   plateBody (g, r, juce::Colour (0xff23181b), juce::Colour (0xff120c0e));
            auto ml = juce::Rectangle<float> (in.getX(), r.getCentreY() - H * 0.12f, in.getWidth(), H * 0.24f);
            for (int k = 0; k < 18; ++k) { const float t = k / 18.0f; g.setColour ((t > 0.83f ? juce::Colour (0xffff3b5c) : t > 0.65f ? juce::Colour (0xffffb547) : juce::Colour (0xff4dffb4)).withAlpha (t < 0.9f ? 0.95f : 0.25f)); g.fillRect (ml.getX() + k * ml.getWidth() / 18, ml.getY(), ml.getWidth() / 18 - 1, ml.getHeight()); }
            g.setColour (juce::Colours::white); g.fillRect (ml.getX() + ml.getWidth() * 0.9f, ml.getY() - H * 0.12f, juce::jmax (1.0f, W * 0.012f), ml.getHeight() + H * 0.24f);
            break; }
        default: break;
    }
    if (! on) { g.setColour (juce::Colours::black.withAlpha (0.55f)); g.fillRoundedRectangle (r, juce::jmin (6.0f, H * 0.14f)); }
    g.restoreState();
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
        hwIcon (g, m, { r.getX() + 28, r.getY() + 6, 48, r.getHeight() - 12 }, on);
        g.setFont (cap (7.6f)); g.setColour (on ? K::ink : K::faint); g.drawFittedText (modInfo (m).name, r.withTrimmedLeft (84).withTrimmedRight (22).withHeight (r.getHeight() * 0.55f).toNearestInt(), juce::Justification::bottomLeft, 1, 0.75f);
        const auto st = statOf (ed, m);
        g.setFont (num (8.0f)); g.setColour (on ? c : K::faint); g.drawFittedText (st.brief, r.withTrimmedLeft (84).withTrimmedRight (22).withTrimmedTop (r.getHeight() * 0.55f).toNearestInt(), juce::Justification::topLeft, 1, 0.8f);
        // togli
        if (h) { auto x = juce::Rectangle<float> (r.getRight() - 22, r.getCentreY() - 8, 16, 16); g.setColour (hovX ? K::red : K::sub); g.setFont (f (12.0f, 1)); g.drawText (juce::String::fromUTF8 ("\xc3\x97"), x, juce::Justification::centred); }
    }
    if (dragging && dragMod >= 0)
    {
        auto r = row (0).withY (dragY - grab); const auto c = acc (juce::Colour (modInfo (dragMod).colour));
        g.setColour (K::lo.withAlpha (0.35f)); g.fillRoundedRectangle (r.translated (0, 4), 9);
        g.setColour (K::panel0); g.fillRoundedRectangle (r, 9); g.setColour (c); g.drawRoundedRectangle (r.reduced (0.5f), 9, 1.2f);
        hwIcon (g, dragMod, { r.getX() + 28, r.getY() + 6, 48, r.getHeight() - 12 });
        g.setFont (cap (7.6f)); g.setColour (K::ink); g.drawText (modInfo (dragMod).name, r.withTrimmedLeft (84), juce::Justification::centredLeft);
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
        hwIcon (g, m, { r.getX() + 10, r.getY() + 10, r.getWidth() - 20, 58 }, ! in);
        g.setFont (cap (8.5f)); g.setColour (in ? K::faint : K::ink); g.drawFittedText (modInfo (m).name, juce::Rectangle<int> ((int) r.getX() + 12, (int) r.getY() + 76, (int) r.getWidth() - 24, 16), juce::Justification::centredLeft, 1, 0.8f);
        g.setFont (f (8.5f)); g.setColour (in ? K::faint : c); g.drawFittedText (modInfo (m).sub, juce::Rectangle<int> ((int) r.getX() + 12, (int) r.getY() + 92, (int) r.getWidth() - 24, 14), juce::Justification::centredLeft, 1, 0.8f);
        g.setFont (f (8.6f)); g.setColour (in ? K::faint : K::sub);
        g.drawFittedText (modText (m).what, juce::Rectangle<int> ((int) r.getX() + 12, (int) r.getY() + 110, (int) r.getWidth() - 22, 34), juce::Justification::topLeft, 3, 0.9f);
        g.setColour (in ? K::faint : K::ink.withAlpha (0.75f));
        g.drawFittedText (in ? juce::String ("gia in catena: clic per aprirlo") : juce::String ("quando: ") + modText (m).when, juce::Rectangle<int> ((int) r.getX() + 12, (int) r.getY() + 144, (int) r.getWidth() - 22, 30), juce::Justification::topLeft, 2, 0.9f);
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
    if (ed.modNote.isNotEmpty() && juce::Time::getMillisecondCounter() - ed.modNoteT < 9000) { tip = ed.modNote; }
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
static constexpr float kRowH = 70;
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
        g.setFont (f (9.8f)); g.setColour (K::sub); g.drawFittedText (presets()[(size_t) i].note, r.withTrimmedLeft (16).withTrimmedRight (12).withTrimmedTop (30).toNearestInt(), juce::Justification::topLeft, 2, 0.9f);
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
// ---------------------------------------------------------------------------------------------------- riepilogo della catena
juce::String keyLine (Processor& P, int m)
{
    auto v = [&P] (const char* id) { return P.value (id); };
    auto txt = [&P] (const char* id) { auto* p = P.apvts.getParameter (id); return p ? p->getCurrentValueAsText() : juce::String(); };
    auto pc = [] (float x) { return juce::String (juce::roundToInt (x * 100)) + "%"; };
    switch (m)
    {
        case BASS:    return "mono < " + juce::String ((int) v ("bc_mono")) + " Hz  ·  banda " + juce::String ((int) v ("bc_freq")) + " Hz";
        case DYNEQ:   { int on = 0, dy = 0; for (int b = 0; b < kEqBands; ++b) if (v (eqId (b, "on")) > 0.5f) { ++on; if (v (eqId (b, "dyn")) > 0.5f) ++dy; } return juce::String (on) + " bande  ·  " + juce::String (dy) + " dinamiche"; }
        case DEESS:   return juce::String (v ("ds_lo") / 1000.0f, 1) + " kHz  ·  soglia " + juce::String (v ("ds_thr"), 0);
        case BUSCOMP: return "soglia " + juce::String (v ("bus_thr"), 0) + "  ·  ratio " + txt ("bus_ratio") + "  ·  mix " + pc (v ("bus_mix"));
        case MULTI:   return "tagli " + juce::String ((int) v ("mb_x1")) + " / " + juce::String ((int) v ("mb_x2")) + " Hz  ·  mix " + pc (v ("mb_mix"));
        case EXCITER: return "mix " + pc (v ("ex_mix")) + "  ·  " + juce::String ((int) v ("ex_x1")) + " / " + juce::String ((int) v ("ex_x2")) + " Hz";
        case TAPE:    return txt ("tp_model") + "  ·  drive " + pc (v ("tp_drive"));
        case IMAGER:  return "larghezze " + pc (v ("im_w1")) + " " + pc (v ("im_w2")) + " " + pc (v ("im_w3")) + " " + pc (v ("im_w4"));
        case MAXIM:   return "gain +" + juce::String (v ("mx_gain"), 1) + " dB  ·  " + txt ("mx_style");
        case LIMIT:   return "ceiling " + juce::String (v ("lm_ceil"), 1) + " dBTP" + (v ("lm_tp") > 0.5f ? "  ·  true peak" : "");
        case EQ432:   return "colore " + pc (v ("e4_col")) + "  ·  range " + txt ("e4_range");
        case PULTEC:  return "bassi +" + juce::String (v ("pq_lb"), 1) + " / -" + juce::String (v ("pq_la"), 1) + "  ·  alti +" + juce::String (v ("pq_hb"), 1);
        case VARIMU:  return "soglia " + juce::String (v ("vm_thr"), 0) + "  ·  mix " + pc (v ("vm_mix"));
        default: return {};
    }
}
static float activityOf (Processor& P, int m)
{
    float a = 0;
    switch (m)
    {
        case MAXIM: a = P.grMax.load() / 6; break; case LIMIT: a = P.grLim.load() / 3; break; case BUSCOMP: a = P.busGr.load() / 4; break; case VARIMU: a = P.vmGr.load() / 4; break;
        case DEESS: a = P.dsGr.load() / 6; break; case BASS: a = P.bassPeak.load() / 4; break; case EXCITER: a = P.exMeter.load() * 8; break;
        case DYNEQ: for (int b = 0; b < kEqBands; ++b) a = juce::jmax (a, std::abs (P.eqDyn[b].load()) / 4); break;
        case MULTI: for (int b = 0; b < kMbBands; ++b) a = juce::jmax (a, P.mbDown[b].load() / 6); break;
        case IMAGER: a = 1.0f - juce::jlimit (0.0f, 1.0f, P.corrAll.load()); break;
        case TAPE: a = P.value ("tp_drive"); break; default: a = 0.3f; break;
    }
    return juce::jlimit (0.0f, 1.0f, a);
}
void RackRecap::paint (juce::Graphics& g)
{
    setTheme (ed.proc.uiTheme.load() == 2 ? 2 : 1);
    auto b = getLocalBounds().toFloat();
    { juce::ColourGradient dg (K::disp0, 0, 0, K::disp1, 0, b.getBottom(), false); g.setGradientFill (dg); g.fillRoundedRectangle (b, 22); g.setColour (K::hi.withAlpha (0.07f)); g.drawRoundedRectangle (b.reduced (0.5f), 22, 1.0f); }
    const auto v = ed.rack.visible();
    g.setFont (cap (9.5f)); g.setColour (K::ink); g.drawText ("CATENA", juce::Rectangle<float> (20, 14, 120, 24), juce::Justification::centredLeft);
    g.setFont (f (9.5f)); g.setColour (K::sub);
    int nOn = 0; for (int m : v) if (ed.proc.value (onParam (m)) > 0.5f) ++nOn;
    g.drawText (juce::String (v.size()) + " effetti, " + juce::String (nOn) + " accesi  ·  il suono scorre da sinistra a destra, riga per riga  ·  clic su un effetto per aprirlo", juce::Rectangle<float> (110, 14, b.getWidth() - 130, 24), juce::Justification::centredLeft);
    for (int i = 0; i <= (int) v.size(); ++i)
    {
        auto r = card (i);
        if (i == (int) v.size())
        {   // aggiungi
            g.setColour (K::hi.withAlpha (hov == 100 ? 0.08f : 0.03f)); g.fillRoundedRectangle (r, 14);
            juce::Path d; d.addRoundedRectangle (r.reduced (1), 14); juce::Path dd; const float dl[] = { 6, 5 }; juce::PathStrokeType (1.0f).createDashedStroke (dd, d, dl, 2); g.setColour (K::hi.withAlpha (0.18f)); g.fillPath (dd);
            g.setFont (big (24.0f)); g.setColour (K::sub); g.drawText ("+", r.withTrimmedBottom (30), juce::Justification::centred);
            g.setFont (cap (8.0f)); g.drawText ("AGGIUNGI EFFETTO", r.withTrimmedTop (r.getHeight() * 0.55f), juce::Justification::centred);
            break;
        }
        const int m = v[(size_t) i]; const bool on = ed.proc.value (onParam (m)) > 0.5f; const auto c = acc (juce::Colour (modInfo (m).colour));
        g.setColour (K::hi.withAlpha (hov == i ? 0.08f : 0.04f)); g.fillRoundedRectangle (r, 14);
        g.setColour (hov == i ? c.withAlpha (0.7f) : K::hi.withAlpha (0.06f)); g.drawRoundedRectangle (r.reduced (0.5f), 14, 1.0f);
        // freccia verso il prossimo
        if (i % 4 != 3 && i + 1 < (int) v.size()) { const float ax = r.getRight() + 3, ay = r.getY() + 38; juce::Path ar; ar.addTriangle (ax, ay - 4, ax, ay + 4, ax + 7, ay); g.setColour (K::hi.withAlpha (0.25f)); g.fillPath (ar); }
        g.setFont (num (8.0f)); g.setColour (K::faint); g.drawText (juce::String (i + 1), juce::Rectangle<float> (r.getRight() - 30, r.getY() + 8, 22, 12), juce::Justification::centredRight);
        hwIcon (g, m, { r.getX() + 10, r.getY() + 10, r.getWidth() - 44, 56 }, on);
        g.setFont (cap (8.0f)); g.setColour (on ? K::ink : K::faint); g.drawFittedText (modInfo (m).name, juce::Rectangle<int> ((int) r.getX() + 12, (int) r.getY() + 72, (int) r.getWidth() - 24, 14), juce::Justification::centredLeft, 1, 0.8f);
        const auto st = statOf (ed, m);
        g.setFont (num (13.0f)); g.setColour (on ? c : K::faint); g.drawFittedText (st.brief, juce::Rectangle<int> ((int) r.getX() + 12, (int) r.getY() + 88, (int) r.getWidth() - 24, 18), juce::Justification::centredLeft, 1, 0.75f);
        g.setFont (f (8.5f)); g.setColour (on ? K::sub : K::faint); g.drawFittedText (keyLine (ed.proc, m), juce::Rectangle<int> ((int) r.getX() + 12, (int) r.getY() + 108, (int) r.getWidth() - 24, 14), juce::Justification::centredLeft, 1, 0.7f);
        // attivita
        auto ab = juce::Rectangle<float> (r.getX() + 12, r.getBottom() - 12, r.getWidth() - 24, 3);
        g.setColour (K::hi.withAlpha (0.07f)); g.fillRoundedRectangle (ab, 1.5f);
        if (on) { g.setColour (c); g.fillRoundedRectangle (ab.withWidth (ab.getWidth() * activityOf (ed.proc, m)), 1.5f); }
    }
}
void RackRecap::mouseMove (const juce::MouseEvent& e)
{
    const auto v = ed.rack.visible(); int h = -1;
    for (int i = 0; i <= (int) v.size(); ++i) if (card (i).contains (e.position)) h = i == (int) v.size() ? 100 : i;
    if (h != hov) { hov = h; repaint(); }
    setMouseCursor (h >= 0 ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
}
void RackRecap::mouseDown (const juce::MouseEvent& e)
{
    const auto v = ed.rack.visible();
    for (int i = 0; i <= (int) v.size(); ++i) if (card (i).contains (e.position))
    {
        if (i == (int) v.size()) { ed.gallery.setVisible (true); ed.gallery.toFront (false); ed.rack.repaint(); }
        else ed.openEffect (v[(size_t) i]);
        return;
    }
}
}
