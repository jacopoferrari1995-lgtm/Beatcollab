// JACKPOT MIX RADAR: grafica scura come FX e Master.
// RADAR: tracce collegate, i loro spettri sovrapposti, le battute con i problemi, la lista "chi e dove" con cosa fare.
// SENSORE: il ruolo della traccia e lo stato del collegamento.
#include "MrEditor.h"

namespace mr
{
using namespace neo;
static juce::String U (const char* s) { return juce::String::fromUTF8 (s); }
static void panel (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour edge, bool display = false)
{
    if (skin::cur() != 3) { jlook::softShadow (g, r, 14); jlook::card (g, r, 14, edge, true, false, false); if (display) { g.setColour (K::lo.withAlpha (skin::light() ? 0.06f : 0.25f)); g.fillRoundedRectangle (r.reduced (6), 10); } return; }
    const float rad = 16;
    g.setColour (K::lo.withAlpha (0.35f)); g.fillRoundedRectangle (r.translated (0, 3), rad);
    juce::ColourGradient bgc (display ? K::disp0 : K::panel0, 0, r.getY(), display ? K::disp1 : K::panel1, 0, r.getBottom(), false); g.setGradientFill (bgc); g.fillRoundedRectangle (r, rad);
    juce::ColourGradient tint (edge.withAlpha (0.10f), 0, r.getY(), edge.withAlpha (0.0f), 0, r.getY() + juce::jmin (140.0f, r.getHeight()), false); g.setGradientFill (tint); g.fillRoundedRectangle (r, rad);
    g.setColour (K::hi.withAlpha (0.07f)); g.drawRoundedRectangle (r.reduced (0.5f), rad, 1.0f);
}
static void title (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& t, juce::Colour c)
{ g.setFont (cap (9.5f)); g.setColour (c); g.drawText (t, juce::Rectangle<float> (r.getX() + 18, r.getY() + 12, 400, 14), juce::Justification::centredLeft); }
juce::Colour Editor::roleCol (int r)
{
    switch (r) { case KICK: return K::amber; case BASS: return K::red; case VOICE: return K::pink; case MELODY: return K::cyan; case PAD: return K::violet; case HATS: return K::mint; case FXR: return K::blue; default: return K::sub; }
}
static juce::Colour typeCol (int t) { static const juce::Colour* c[kTypes] = { &K::amber, &K::pink, &K::red, &K::violet, &K::cyan, &K::mint }; return *c[juce::jlimit (0, kTypes - 1, t)]; }

Editor::Editor (Processor& p) : AudioProcessorEditor (p), proc (p)
{
    skin::init ("mixradar", { 0xffe58aa0, 0xffb9a6e0, 0xffe9a260, 0xffa8bea0 }, 0xff3ee8ff);
    for (auto& r : smB) for (auto& x : r) x = -120; for (auto& x : smM) x = -120;
    addAndMakeVisible (sizeChip);
    scaler.setup();
    startTimerHz (20);
}
void Editor::resized()
{
    const float s = scaler.update();
    modeR[0] = { 760, 18, 130, 34 }; modeR[1] = { 896, 18, 130, 34 };
    for (int r = 0; r < kRoles; ++r) roleR[r] = { 48.0f + (r % 4) * 268, 214.0f + (r / 4) * 64, 256, 54 };
    clearR = { 1036, 18, 120, 34 }; themeR = { 636, 21, 110, 28 };
    viewR[0] = { 340, 21, 136, 28 }; viewR[1] = { 482, 21, 140, 28 };
    for (int r = 0; r < kRefs; ++r) refR[r] = { 800.0f + (r % 4) * 88, 636.0f + (r / 4) * 28, 84, 24 };
    spanR[0] = { 800, 698, 174, 24 }; spanR[1] = { 978, 698, 174, 24 };
    for (int f = 0; f < NF; ++f) fasciaR[f] = { 40.0f + f * 106, 416, 100, 170 };
    chartR = { 70, 112, 696, 236 };
    sizeChip.setBounds (W - 76, H - 18, 58, 15);
    bg = {};
    jlook::scaleChildren (*this, s);
}
void Editor::paintStatic (juce::Graphics& g)
{
    if (skin::cur() != 3)
    {
        jlook::backdrop (g, { 0, 0, (float) W, (float) H }, K::cyan, K::amber);
        jlook::header (g, { 0, 0, (float) W, 66 }, "MIX", K::cyan, "RADAR", K::amber, K::pink);
    }
    else
    {
    juce::ColourGradient base (K::bg1, 0, 0, K::bg0, 0, (float) H, false); g.setGradientFill (base); g.fillAll();
    for (int i = 0; i < 3; ++i) { const float r = 6.0f + i * 6; g.setColour (K::cyan.withAlpha (0.8f - i * 0.2f)); g.drawEllipse (36 - r, 34 - r, r * 2, r * 2, 1.6f); }
    g.setColour (K::amber); g.fillEllipse (41, 25, 5, 5); g.setColour (K::pink); g.fillEllipse (27, 38, 5, 5);
    g.setFont (num (15.0f)); g.setColour (K::ink); g.drawText ("JACKPOT", juce::Rectangle<float> (62, 18, 100, 20), juce::Justification::centredLeft);
    g.setFont (big (15.0f)); g.setColour (K::sub); g.drawText ("MIX RADAR", juce::Rectangle<float> (139, 18, 160, 20), juce::Justification::centredLeft);
    g.setFont (cap (7.5f)); g.setColour (K::faint); g.drawText (U ("CHI CREA IL PROBLEMA, E IN QUALE BATTUTA"), juce::Rectangle<float> (62, 38, 300, 12), juce::Justification::centredLeft);
    }
    if (proc.radar() && view == 0)
    {
        panel (g, { 24, 76, 760, 300 }, K::cyan, true); title (g, { 24, 76, 760, 300 }, "EQUILIBRIO TONALE", K::cyan);
        panel (g, { 24, 388, 760, 208 }, K::amber); title (g, { 24, 388, 760, 208 }, "ENERGIA NELLE FASCE", K::amber);
        panel (g, { 796, 76, 360, 520 }, K::red); title (g, { 796, 76, 360, 520 }, "CONSIGLI", K::red);
        panel (g, { 24, 608, 1132, 136 }, K::violet); title (g, { 24, 608, 1132, 136 }, "MASTER", K::violet);
    }
    else if (proc.radar())
    {
        panel (g, { 24, 76, 300, 520 }, K::cyan); title (g, { 24, 76, 300, 520 }, "TRACCE COLLEGATE", K::cyan);
        panel (g, { 336, 76, 500, 280 }, K::cyan, true); title (g, { 336, 76, 500, 280 }, "SPETTRI SOVRAPPOSTI", K::cyan);
        panel (g, { 336, 368, 500, 228 }, K::amber, true); title (g, { 336, 368, 500, 228 }, "BATTUTE CON PROBLEMI", K::amber);
        panel (g, { 848, 76, 308, 520 }, K::red); title (g, { 848, 76, 308, 520 }, "PROBLEMI: CHI E DOVE", K::red);
        panel (g, { 24, 608, 1132, 136 }, K::violet); title (g, { 24, 608, 1132, 136 }, "MASTER", K::violet);
    }
    else
    {
        panel (g, { 24, 76, 1132, 104 }, K::cyan); title (g, { 24, 76, 1132, 104 }, "SENSORE", K::cyan);
        panel (g, { 24, 192, 1132, 150 }, K::amber); title (g, { 24, 192, 1132, 150 }, "CHE COSA C'E' SU QUESTA TRACCIA", K::amber);
        panel (g, { 24, 354, 1132, 390 }, K::violet, true); title (g, { 24, 354, 1132, 390 }, "QUELLO CHE MANDA AL RADAR", K::violet);
    }
}
void Editor::paint (juce::Graphics& g)
{
    g.addTransform (juce::AffineTransform::scale (scaler.s));
    const float sc = juce::jlimit (1.0f, 4.0f, g.getInternalContext().getPhysicalPixelScaleFactor());
    const int mode = proc.radar() ? 1 + view : 0;
    if (bg.isNull() || std::abs (bgScale - sc) > 0.01f || bgMode != mode || bgTheme != skin::cur())
    { bgTheme = skin::cur(); bgScale = sc; bgMode = mode; bg = juce::Image (juce::Image::RGB, juce::roundToInt (W * sc), juce::roundToInt (H * sc), false); juce::Graphics ig (bg); ig.addTransform (juce::AffineTransform::scale (sc)); paintStatic (ig); }
    { g.saveState(); g.addTransform (juce::AffineTransform::scale (1.0f / sc)); g.drawImageAt (bg, 0, 0); g.restoreState(); }
    skin::chipDraw (g, themeR, hoverTheme);
    chip (g, modeR[0], "SENSORE (TRACCIA)", mode == 0, K::cyan, 8.5f); chip (g, modeR[1], "RADAR (MASTER)", mode > 0, K::amber, 8.5f);
    if (mode > 0)
    {
        int n = 0; for (int i = 0; i < MAXS; ++i) n += hub().s[i].alive();
        chip (g, viewR[0], U ("ANALISI MASTER"), view == 0, K::cyan, 8.5f); chip (g, viewR[1], "TRACCE (" + juce::String (n) + ")", view == 1, K::amber, 8.5f);
    }
    if (mode == 1) paintMaster (g); else if (mode == 2) paintRadar (g); else paintSensor (g);
}
void Editor::paintSensor (juce::Graphics& g)
{
    const bool rOk = radarAlive();
    g.setFont (f (14.0f, 1)); g.setColour (K::ink);
    g.drawText (proc.trackName.isNotEmpty() ? proc.trackName : U ("(nome della traccia non ricevuto)"), juce::Rectangle<float> (44, 104, 600, 20), juce::Justification::centredLeft);
    g.setFont (f (10.5f)); g.setColour (rOk ? K::mint : K::red);
    g.drawText (rOk ? U ("collegato al RADAR: questa traccia compare nella lista") : U ("nessun RADAR: metti una copia di Mix Radar sul Master e scegli RADAR"), juce::Rectangle<float> (44, 128, 700, 16), juce::Justification::centredLeft);
    g.setFont (f (9.5f)); g.setColour (K::faint);
    g.drawText (U ("Il sensore non cambia il suono. Copie tutte dello stesso tipo (tutte AU o tutte VST3)."), juce::Rectangle<float> (44, 148, 700, 14), juce::Justification::centredLeft);
    const int role = juce::roundToInt (proc.v ("role"));
    for (int r = 0; r < kRoles; ++r)
    {
        const auto c = roleCol (r); const bool on = r == role; auto rr = roleR[r];
        g.setColour (on ? c.withAlpha (0.18f) : K::lo.withAlpha (0.25f)); g.fillRoundedRectangle (rr, 10);
        g.setColour (on ? c : K::hi.withAlpha (0.1f)); g.drawRoundedRectangle (rr.reduced (0.5f), 10, on ? 1.6f : 1.0f);
        g.setFont (f (13.0f, 1)); g.setColour (on ? K::ink : K::sub); g.drawText (roleName (r), rr, juce::Justification::centred);
    }
    auto sp = juce::Rectangle<float> (60, 400, 1076, 300);
    auto xOf = [&] (int b) { return sp.getX() + sp.getWidth() * b / (NB - 1); };
    auto yOf = [&] (float db) { return sp.getBottom() - sp.getHeight() * juce::jlimit (0.0f, 1.0f, (db + 84) / 78); };
    for (int b = 0; b < NB; b += 3) { g.setColour (K::hi.withAlpha (0.05f)); g.fillRect (xOf (b), sp.getY(), 1.0f, sp.getHeight()); g.setFont (num (8.5f)); g.setColour (K::faint); const float hz = bandHz (b); g.drawText (hz >= 1000 ? juce::String (hz / 1000, 1) + "k" : juce::String ((int) hz), juce::Rectangle<float> (xOf (b) - 20, sp.getBottom() + 4, 40, 11), juce::Justification::centred); }
    juce::Path p; for (int b = 0; b < NB; ++b) { const float y = yOf (smM[b]); if (b == 0) p.startNewSubPath (xOf (b), y); else p.lineTo (xOf (b), y); }
    juce::Path fl = p; fl.lineTo (sp.getRight(), sp.getBottom()); fl.lineTo (sp.getX(), sp.getBottom()); fl.closeSubPath();
    const auto c = roleCol (role); g.setColour (c.withAlpha (0.2f)); g.fillPath (fl); g.setColour (c); g.strokePath (p, juce::PathStrokeType (2.0f));
}
static juce::Colour fasciaCol (int f) { static const juce::Colour* c[NF] = { &K::red, &K::amber, &K::violet, &K::blue, &K::pink, &K::cyan, &K::mint }; return *c[juce::jlimit (0, NF - 1, f)]; }
void Editor::paintMaster (juce::Graphics& g)
{
    chip (g, clearR, "AZZERA", false, K::red, 8.5f);
    const auto& R = proc.rep; const int ref = juce::roundToInt (proc.v ("ref")), solo = proc.soloNow();
    const float* rc = refCurve (ref);
    // ---------------- equilibrio tonale: la curva del mix e la fascia del riferimento
    auto cr = chartR; const float lo = -18, hi = 18;
    auto xOf = [&] (float hz) { return cr.getX() + cr.getWidth() * std::log (hz / 25.0f) / std::log (18000.0f / 25.0f); };
    auto yOf = [&] (float db) { return cr.getBottom() - cr.getHeight() * juce::jlimit (0.0f, 1.0f, (db - lo) / (hi - lo)); };
    g.setFont (cap (7.5f)); g.setColour (K::cyan); g.drawText (U ("RIFERIMENTO: ") + refName (ref) + (proc.v ("span") > 0.5f ? U ("  -  TUTTO IL BRANO") : U ("  -  ULTIMI SECONDI")), juce::Rectangle<float> (300, 88, 470, 12), juce::Justification::centredRight);
    for (int f = 0; f < NF; ++f)
    {
        const float x0 = xOf (juce::jmax (25.0f, fasciaEdge (f))), x1 = xOf (juce::jmin (18000.0f, fasciaEdge (f + 1)));
        const bool on = solo == f + 1;
        g.setColour (on ? fasciaCol (f).withAlpha (0.16f) : (f % 2 ? K::hi.withAlpha (0.025f) : juce::Colours::transparentBlack)); g.fillRect (x0, cr.getY(), x1 - x0, cr.getHeight());
        g.setFont (cap (7.0f)); g.setColour (fasciaCol (f).withAlpha (on ? 1.0f : 0.75f)); g.drawText (fasciaName (f), juce::Rectangle<float> (x0, cr.getY() + 2, x1 - x0, 10), juce::Justification::centred);
    }
    for (float hz : { 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f })
    { g.setFont (num (8.0f)); g.setColour (K::faint); g.drawText (hz >= 1000 ? juce::String ((int) (hz / 1000)) + "k" : juce::String ((int) hz), juce::Rectangle<float> (xOf (hz) - 18, cr.getBottom() + 3, 36, 10), juce::Justification::centred); }
    for (int db = -12; db <= 12; db += 6) { g.setColour (K::hi.withAlpha (db == 0 ? 0.08f : 0.035f)); g.drawHorizontalLine ((int) yOf ((float) db), cr.getX(), cr.getRight()); g.setFont (num (7.5f)); g.setColour (K::faint); g.drawText ((db > 0 ? "+" : "") + juce::String (db), juce::Rectangle<float> (cr.getX() - 32, yOf ((float) db) - 5, 28, 10), juce::Justification::centredRight); }
    if (R.ready && ! R.dispHz.empty())
    {
        // fascia del riferimento (piu' o meno la tolleranza)
        juce::Path band, line; const size_t N = R.dispHz.size();
        auto tolAt = [&] (float hz) { for (int f = 0; f < NF; ++f) if (hz < fasciaEdge (f + 1)) return refTol (f); return refTol (NF - 1); };
        for (size_t i = 0; i < N; ++i) { const float x = xOf (R.dispHz[i]), y = yOf (R.ref[i] + tolAt (R.dispHz[i])); if (i == 0) band.startNewSubPath (x, y); else band.lineTo (x, y); }
        for (size_t i = N; i-- > 0;) band.lineTo (xOf (R.dispHz[i]), yOf (R.ref[i] - tolAt (R.dispHz[i])));
        band.closeSubPath(); g.setColour (K::mint.withAlpha (0.10f)); g.fillPath (band);
        for (size_t i = 0; i < N; ++i) { const float x = xOf (R.dispHz[i]), y = yOf (R.ref[i]); if (i == 0) line.startNewSubPath (x, y); else line.lineTo (x, y); }
        { juce::Path d; const float dl[] = { 4, 4 }; juce::PathStrokeType (1.0f).createDashedStroke (d, line, dl, 2); g.setColour (K::mint.withAlpha (0.6f)); g.fillPath (d); }
        // la curva del mix: verde dentro, rossa dove esce dalla fascia
        for (size_t i = 1; i < N; ++i)
        {
            const float d = R.disp[i] - R.ref[i], t = tolAt (R.dispHz[i]);
            const auto c = std::abs (d) <= t ? K::ink : (d > 0 ? K::red : K::blue);
            g.setColour (c.withAlpha (0.95f)); g.drawLine (xOf (R.dispHz[i - 1]), yOf (R.disp[i - 1]), xOf (R.dispHz[i]), yOf (R.disp[i]), 2.2f);
        }
        for (auto& rr : R.res) { const float x = xOf (rr.first); float y = cr.getY() + 14; for (size_t i = 0; i < N; ++i) if (R.dispHz[i] >= rr.first) { y = yOf (R.disp[i]); break; } g.setColour (K::amber); g.drawEllipse (x - 6, y - 6, 12, 12, 1.6f); g.setFont (num (8.0f)); g.drawText (juce::String (rr.first >= 1000 ? juce::String (rr.first / 1000, 1) + "k" : juce::String ((int) rr.first)), juce::Rectangle<float> (x - 20, y - 20, 40, 10), juce::Justification::centred); }
    }
    else
    {
        g.setFont (f (12.0f)); g.setColour (K::sub);
        g.drawText (proc.playing.load() || proc.scan.seconds() > 0 ? U ("Ascolto in corso: servono un paio di secondi di musica...") : U ("Fai partire il pezzo: Mix Radar sul Master ascolta tutto il mix, anche senza sensori."), cr, juce::Justification::centred);
    }
    g.setFont (f (8.5f)); g.setColour (K::faint);
    g.drawText (U ("linea tratteggiata e fascia verde: il riferimento  -  bianco: dentro  -  rosso: troppo  -  blu: poco  -  cerchi: risonanze  -  clic su una zona: la ascolti da sola"), juce::Rectangle<float> (40, 362, 740, 12), juce::Justification::centredLeft);
    // ---------------- fasce
    for (int fa = 0; fa < NF; ++fa)
    {
        const auto r = fasciaR[fa]; const auto c = fasciaCol (fa); const bool on = solo == fa + 1;
        g.setColour (on ? c.withAlpha (0.16f) : K::lo.withAlpha (0.25f)); g.fillRoundedRectangle (r, 8);
        g.setColour (on ? c : K::hi.withAlpha (0.08f)); g.drawRoundedRectangle (r.reduced (0.5f), 8, on ? 1.6f : 1.0f);
        g.setFont (f (11.5f, 1)); g.setColour (c); g.drawText (fasciaName (fa), juce::Rectangle<float> (r.getX(), r.getY() + 6, r.getWidth(), 16), juce::Justification::centred);
        g.setFont (num (8.0f)); g.setColour (K::faint); g.drawText (fasciaRange (fa), juce::Rectangle<float> (r.getX(), r.getY() + 22, r.getWidth(), 11), juce::Justification::centred);
        // barra dello scarto: centro = riferimento, zona verde = tolleranza
        auto m = juce::Rectangle<float> (r.getCentreX() - 9, r.getY() + 40, 18, 74); const float tol = refTol (fa), sc = m.getHeight() / 16;
        g.setColour (K::hi.withAlpha (0.05f)); g.fillRoundedRectangle (m, 4);
        g.setColour (K::mint.withAlpha (0.16f)); g.fillRect (m.getX(), m.getCentreY() - tol * sc, m.getWidth(), tol * 2 * sc);
        g.setColour (K::hi.withAlpha (0.3f)); g.fillRect (m.getX() - 3, m.getCentreY(), m.getWidth() + 6, 1.0f);
        if (R.ready)
        {
            const float d = juce::jlimit (-8.0f, 8.0f, R.dev[fa]); const bool ok = std::abs (R.dev[fa]) <= tol;
            const auto col = ok ? K::mint : (R.dev[fa] > 0 ? K::red : K::blue);
            const float y0 = m.getCentreY(), y1 = y0 - d * sc; g.setColour (col); g.fillRoundedRectangle (m.getX() + 3, juce::jmin (y0, y1), m.getWidth() - 6, juce::jmax (2.0f, std::abs (y1 - y0)), 2);
            g.setFont (num (12.0f)); g.drawText ((R.dev[fa] > 0 ? "+" : "") + juce::String (R.dev[fa], 1), juce::Rectangle<float> (r.getX(), r.getY() + 118, r.getWidth(), 16), juce::Justification::centred);
            g.setFont (cap (6.5f)); g.setColour (K::faint); g.drawText (ok ? U ("IN LINEA") : (R.dev[fa] > 0 ? U ("TROPPO") : U ("POCO")), juce::Rectangle<float> (r.getX(), r.getY() + 133, r.getWidth(), 10), juce::Justification::centred);
            // stereo della fascia
            const float sw = juce::jlimit (0.0f, 1.0f, R.side[fa] / 50.0f);
            g.setColour (K::hi.withAlpha (0.06f)); g.fillRoundedRectangle (r.getX() + 12, r.getY() + 148, r.getWidth() - 24, 5, 2.5f);
            g.setColour (c.withAlpha (0.8f)); g.fillRoundedRectangle (r.getCentreX() - (r.getWidth() - 24) * sw * 0.5f, r.getY() + 148, juce::jmax (2.0f, (r.getWidth() - 24) * sw), 5, 2.5f);
            g.setFont (num (7.0f)); g.setColour (K::faint); g.drawText ("stereo " + juce::String (juce::roundToInt (R.side[fa])) + "%", juce::Rectangle<float> (r.getX(), r.getY() + 155, r.getWidth(), 10), juce::Justification::centred);
        }
        if (on) { g.setFont (cap (6.5f)); g.setColour (c); g.drawText (U ("IN ASCOLTO"), juce::Rectangle<float> (r.getX(), r.getY() + 33, r.getWidth(), 9), juce::Justification::centred); }
    }
    // ---------------- consigli
    float yy = 104;
    g.setFont (f (10.5f, 1)); g.setColour (K::ink);
    const juce::String sum = R.ready ? R.summary : U ("Il radar ascolta il master: equilibrio per fasce, risonanze, stereo, dinamica e loudness rispetto al genere scelto.");
    g.drawFittedText (sum, juce::Rectangle<int> (814, (int) yy, 326, 30), juce::Justification::topLeft, 2); yy += 38;
    int shown = 0;
    for (auto& a : R.advice)
    {
        const auto c = a.sev > 2 ? K::red : a.sev > 1.3f ? K::amber : (a.fascia >= 0 ? fasciaCol (a.fascia) : K::cyan);
        const int h2 = a.text.length() > 58 ? (a.text.length() > 116 ? 36 : 25) : 13, h3 = a.fix.length() > 54 ? (a.fix.length() > 108 ? 36 : 25) : 13;
        const float h = 20.0f + h2 + h3 + 12;
        if (yy + h > 588) break;
        g.setColour (c.withAlpha (0.09f)); g.fillRoundedRectangle (810, yy, 334, h - 6, 6); g.setColour (c); g.fillRect (810.0f, yy, 3.0f, h - 6);
        g.setFont (f (10.5f, 1)); g.setColour (K::ink); g.drawText (a.title, juce::Rectangle<float> (820, yy + 3, 318, 15), juce::Justification::centredLeft);
        g.setFont (f (9.0f)); g.setColour (K::sub); g.drawFittedText (a.text, juce::Rectangle<int> (820, (int) yy + 19, 318, h2), juce::Justification::topLeft, 3);
        g.setFont (f (9.0f, 1)); g.setColour (c.interpolatedWith (K::ink, 0.35f)); g.drawFittedText (juce::String::fromUTF8 ("\xe2\x86\x92 ") + a.fix, juce::Rectangle<int> (820, (int) yy + 21 + h2, 318, h3), juce::Justification::topLeft, 3);
        yy += h; ++shown;
    }
    if (shown < (int) R.advice.size()) { g.setFont (f (9.0f)); g.setColour (K::faint); g.drawText ("+ " + juce::String ((int) R.advice.size() - shown) + U (" consigli minori"), juce::Rectangle<float> (814, 576, 320, 12), juce::Justification::centredLeft); }
    // ---------------- master: misure e riferimento
    auto meter = [&] (float x, const juce::String& lab, const juce::String& val, juce::Colour c, const juce::String& note)
    {
        g.setFont (cap (7.5f)); g.setColour (K::faint); g.drawText (lab, juce::Rectangle<float> (x, 638, 124, 12), juce::Justification::centredLeft);
        g.setFont (num (19.0f)); g.setColour (c); g.drawText (val, juce::Rectangle<float> (x, 652, 124, 26), juce::Justification::centredLeft);
        g.setFont (f (8.0f)); g.setColour (K::faint); g.drawFittedText (note, juce::Rectangle<int> ((int) x, 682, 120, 24), juce::Justification::topLeft, 2);
    };
    const float ls = proc.lufsS.load(), pk = proc.mPeak.load(), co = proc.corr.load(), tl = refLufs (ref);
    meter (48, "LUFS BREVE", ls < -60 ? "--" : juce::String (ls, 1), ls > tl + 2 ? K::red : K::ink, U ("adesso (3 s)"));
    meter (172, "LUFS BRANO", R.lufsI < -60 ? "--" : juce::String (R.lufsI, 1), R.lufsI > tl + 2 ? K::red : K::mint, U ("riferimento ") + juce::String (tl, 0) + U (", Spotify -14"));
    meter (296, "PLR", R.plr <= 0 ? "--" : juce::String (R.plr, 1), R.plr > 0 && R.plr < 7 ? K::red : K::ink, U ("picco - loudness: sotto 7 schiacciato"));
    meter (420, "LRA", R.lra <= 0 ? "--" : juce::String (R.lra, 1), K::ink, U ("differenza fra parti piano e forti"));
    meter (544, "PICCO", pk < -100 ? "--" : juce::String (pk, 1), pk > -1.0f ? K::red : K::ink, U ("tienilo sotto -1 dB"));
    meter (668, "CORRELAZIONE", juce::String (co, 2), co < 0.1f ? K::red : K::ink, U ("sotto 0: il mono perde pezzi"));
    g.setFont (cap (7.5f)); g.setColour (K::faint); g.drawText (U ("RIFERIMENTO DEL GENERE"), juce::Rectangle<float> (800, 620, 300, 12), juce::Justification::centredLeft);
    for (int r = 0; r < kRefs; ++r) chip (g, refR[r], refName (r), r == ref, K::violet, 7.5f);
    const int sp = juce::roundToInt (proc.v ("span"));
    chip (g, spanR[0], U ("ULTIMI SECONDI"), sp == 0, K::cyan, 7.5f); chip (g, spanR[1], U ("TUTTO IL BRANO"), sp == 1, K::cyan, 7.5f);
    if (solo > 0)
    {
        auto w = juce::Rectangle<float> (340, 52, 420, 20);
        g.setColour (fasciaCol (solo - 1).withAlpha (0.9f)); g.fillRoundedRectangle (w, 6);
        g.setFont (f (9.5f, 1)); g.setColour (juce::Colours::black); g.drawText (U ("STAI ASCOLTANDO SOLO ") + fasciaName (solo - 1) + " (" + fasciaRange (solo - 1) + U (")  -  clic sulla fascia per tornare"), w, juce::Justification::centred);
    }
}
void Editor::paintRadar (juce::Graphics& g)
{
    auto& H = hub();
    chip (g, clearR, "AZZERA", false, K::red, 8.5f);
    // ---------------- tracce
    float y = 108; int n = 0;
    for (int i = 0; i < MAXS; ++i)
    {
        auto& S = H.s[i]; if (! S.alive()) continue; ++n;
        if (y > 570) continue;
        const int role = S.role.load(); const auto c = roleCol (role); juce::String nm = S.getName(); if (nm.isEmpty()) nm = roleName (role);
        g.setColour (c); g.fillEllipse (42, y + 5, 8, 8);
        g.setFont (f (11.5f, 1)); g.setColour (K::ink); g.drawText (nm, juce::Rectangle<float> (58, y, 150, 18), juce::Justification::centredLeft);
        g.setFont (cap (7.0f)); g.setColour (c); g.drawText (roleName (role), juce::Rectangle<float> (58, y + 17, 150, 10), juce::Justification::centredLeft);
        const float t = juce::jlimit (0.0f, 1.0f, (S.lvl.load() + 60) / 60);
        g.setColour (K::hi.withAlpha (0.06f)); g.fillRoundedRectangle (210, y + 7, 96, 6, 3); g.setColour (c); g.fillRoundedRectangle (210, y + 7, 96 * t, 6, 3);
        y += 36;
    }
    if (n == 0)
    {
        g.setFont (f (11.0f)); g.setColour (K::sub);
        g.drawFittedText (U ("Nessuna traccia collegata.\n\nMetti Mix Radar sulle tracce importanti (cassa, 808, voce, melodie...), scegli SENSORE e il ruolo della traccia.\n\nQuesta copia sul Master resta su RADAR."), juce::Rectangle<int> (42, 110, 270, 200), juce::Justification::topLeft, 12);
    }
    // ---------------- spettri
    auto sp = juce::Rectangle<float> (356, 108, 460, 220);
    auto xOf = [&] (int b) { return sp.getX() + sp.getWidth() * b / (NB - 1); };
    auto yOf = [&] (float db) { return sp.getBottom() - sp.getHeight() * juce::jlimit (0.0f, 1.0f, (db + 84) / 78); };
    for (int b = 0; b < NB; b += 3) { g.setColour (K::hi.withAlpha (0.05f)); g.fillRect (xOf (b), sp.getY(), 1.0f, sp.getHeight()); g.setFont (num (8.0f)); g.setColour (K::faint); const float hz = bandHz (b); g.drawText (hz >= 1000 ? juce::String (hz / 1000, 1) + "k" : juce::String ((int) hz), juce::Rectangle<float> (xOf (b) - 18, sp.getBottom() + 3, 36, 10), juce::Justification::centred); }
    for (int i = 0; i < MAXS; ++i)
    {
        auto& S = H.s[i]; if (! S.alive()) continue;
        juce::Path p; for (int b = 0; b < NB; ++b) { const float yy = yOf (smB[i][b]); if (b == 0) p.startNewSubPath (xOf (b), yy); else p.lineTo (xOf (b), yy); }
        g.setColour (roleCol (S.role.load()).withAlpha (0.85f)); g.strokePath (p, juce::PathStrokeType (1.6f));
    }
    { juce::Path p; for (int b = 0; b < NB; ++b) { const float yy = yOf (smM[b]); if (b == 0) p.startNewSubPath (xOf (b), yy); else p.lineTo (xOf (b), yy); } g.setColour (K::ink.withAlpha (0.5f)); g.strokePath (p, juce::PathStrokeType (3.0f)); }
    g.setFont (cap (7.5f)); g.setColour (K::ink.withAlpha (0.6f)); g.drawText (U ("BIANCO = MASTER"), juce::Rectangle<float> (sp.getRight() - 120, sp.getY() - 14, 120, 10), juce::Justification::centredRight);
    // ---------------- battute
    std::map<int, int> flags; std::vector<Issue> list;
    { const juce::ScopedLock l (proc.issueLock); flags = proc.barFlags; for (auto& kv : proc.issues) list.push_back (kv.second); }
    auto tl = juce::Rectangle<float> (440, 404, 380, 160);
    const int lastBar = juce::jmax (32, proc.lastBar), first = lastBar - 31;
    const float cw = tl.getWidth() / 32, rh = tl.getHeight() / kTypes;
    for (int t = 0; t < kTypes; ++t)
    {
        g.setFont (cap (7.0f)); g.setColour (typeCol (t)); g.drawText (typeName (t), juce::Rectangle<float> (350, tl.getY() + t * rh, 88, rh), juce::Justification::centredLeft);
        for (int k = 0; k < 32; ++k)
        {
            const int bar = first + k; auto it = flags.find (bar); const bool on = it != flags.end() && (it->second & (1 << t));
            auto r = juce::Rectangle<float> (tl.getX() + k * cw, tl.getY() + t * rh, cw, rh).reduced (1);
            g.setColour (on ? typeCol (t).withAlpha (0.85f) : K::hi.withAlpha (0.04f)); g.fillRoundedRectangle (r, 2);
        }
    }
    g.setFont (num (8.0f)); g.setColour (K::faint);
    for (int k = 0; k < 32; k += 4) g.drawText (juce::String (first + k), juce::Rectangle<float> (tl.getX() + k * cw, tl.getBottom() + 2, 30, 10), juce::Justification::centredLeft);
    g.drawText ("battuta", juce::Rectangle<float> (350, tl.getBottom() + 2, 80, 10), juce::Justification::centredLeft);
    // ---------------- problemi
    std::sort (list.begin(), list.end(), [] (const Issue& a, const Issue& b) { return a.sev * std::log1p ((float) a.count) > b.sev * std::log1p ((float) b.count); });
    float yy = 108;
    if (list.empty()) { g.setFont (f (11.0f)); g.setColour (K::sub); g.drawFittedText (proc.playing.load() ? U ("Per ora niente da segnalare.") : U ("Fai partire il pezzo: il radar ascolta mentre suona."), juce::Rectangle<int> (866, 110, 274, 60), juce::Justification::topLeft, 3); }
    for (auto& is : list)
    {
        juce::StringArray bs; for (size_t k = 0; k < is.bars.size() && k < 8; ++k) bs.add (juce::String (is.bars[k])); if (is.bars.size() > 8) bs.add ("...");
        const auto c = typeCol (is.type);
        const juce::String l1 = is.who + " " + is.what, l2 = "battute " + bs.joinIntoString (", ") + "  (" + juce::String (is.count) + " volte)";
        const int h1 = l1.length() > 44 ? 30 : 16, h3 = is.fix.length() > 48 ? 26 : 13; const float h = 18.0f + h1 + 14 + h3 + 10;
        if (yy + h > 588) break;
        g.setColour (c.withAlpha (0.10f)); g.fillRoundedRectangle (862, yy, 280, h - 6, 6); g.setColour (c); g.fillRect (862.0f, yy, 3.0f, h - 6);
        g.setFont (cap (8.0f)); g.drawText (typeName (is.type), juce::Rectangle<float> (872, yy + 4, 260, 12), juce::Justification::centredLeft);
        g.setFont (f (10.5f, 1)); g.setColour (K::ink); g.drawFittedText (l1, juce::Rectangle<int> (872, (int) yy + 17, 262, h1), juce::Justification::topLeft, 2);
        g.setFont (num (8.5f)); g.setColour (K::sub); g.drawText (l2, juce::Rectangle<float> (872, yy + 18 + h1, 262, 12), juce::Justification::centredLeft);
        g.setFont (f (9.5f)); g.setColour (K::faint); g.drawFittedText (is.fix, juce::Rectangle<int> (872, (int) yy + 32 + h1, 262, h3), juce::Justification::topLeft, 2);
        yy += h;
    }
    // ---------------- master
    auto meter = [&] (float x, const juce::String& lab, const juce::String& val, juce::Colour c, const juce::String& note)
    {
        g.setFont (cap (7.5f)); g.setColour (K::faint); g.drawText (lab, juce::Rectangle<float> (x, 640, 200, 12), juce::Justification::centredLeft);
        g.setFont (num (20.0f)); g.setColour (c); g.drawText (val, juce::Rectangle<float> (x, 654, 200, 26), juce::Justification::centredLeft);
        g.setFont (f (9.0f)); g.setColour (K::faint); g.drawText (note, juce::Rectangle<float> (x, 684, 210, 12), juce::Justification::centredLeft);
    };
    const float ls = proc.lufsS.load(), lm = proc.lufsM.load(), pk = proc.mPeak.load(), co = proc.corr.load(), lmid = proc.lowMid.load();
    meter (48, "LUFS BREVE (3 s)", ls < -60 ? "--" : juce::String (ls, 1), ls > -7 ? K::red : K::mint, U ("Spotify -14, beat da vendere -9/-8"));
    meter (268, "LUFS ISTANTANEO", lm < -60 ? "--" : juce::String (lm, 1), K::ink, "");
    meter (488, "PICCO", pk < -100 ? "--" : juce::String (pk, 1) + " dB", pk > -0.3f ? K::red : K::ink, U ("sopra -1 dB rischi il clipping"));
    meter (708, "CORRELAZIONE", juce::String (co, 2), co < 0 ? K::red : K::ink, U ("sotto 0: problemi in mono"));
    meter (928, "BASSI / MEDI", juce::String (lmid, 1) + " dB", lmid > 8 ? K::red : K::ink, U ("sopra +8: troppi bassi"));
}
void Editor::mouseDown (const juce::MouseEvent& e)
{
    const auto p = e.position / scaler.s;
    if (themeR.contains (p)) { skin::cycle(); bg = {}; repaint(); return; }
    if (modeR[0].contains (p)) { proc.setV ("mode", 0); bg = {}; repaint(); return; }
    if (modeR[1].contains (p)) { proc.setV ("mode", 1); bg = {}; repaint(); return; }
    if (proc.radar())
    {
        if (clearR.contains (p)) { proc.clearIssues(); repaint(); return; }
        for (int i = 0; i < 2; ++i) if (viewR[i].contains (p)) { view = i; bg = {}; repaint(); return; }
        if (view == 0)
        {
            for (int r = 0; r < kRefs; ++r) if (refR[r].contains (p)) { proc.setV ("ref", (float) r); repaint(); return; }
            for (int i = 0; i < 2; ++i) if (spanR[i].contains (p)) { proc.setV ("span", (float) i); repaint(); return; }
            // ascolta una fascia: clic sulla colonna o sulla sua zona del grafico, di nuovo per tornare al mix
            auto solo = [this] (int f) { proc.setV ("solo", proc.soloNow() == f + 1 ? 0.0f : (float) (f + 1)); repaint(); };
            for (int f = 0; f < NF; ++f) if (fasciaR[f].contains (p)) { solo (f); return; }
            if (chartR.contains (p)) { const float hz = 25.0f * std::pow (18000.0f / 25.0f, (p.x - chartR.getX()) / chartR.getWidth()); for (int f = 0; f < NF; ++f) if (hz >= fasciaEdge (f) && hz < fasciaEdge (f + 1)) { solo (f); return; } }
        }
        return;
    }
    for (int r = 0; r < kRoles; ++r) if (roleR[r].contains (p)) { proc.setV ("role", (float) r); repaint(); return; }
}
void Editor::mouseMove (const juce::MouseEvent& e)
{
    const auto p = e.position / scaler.s; const bool h = themeR.contains (p);
    if (h != hoverTheme) { hoverTheme = h; repaint(); }
    bool hand = h || modeR[0].contains (p) || modeR[1].contains (p) || (proc.radar() && (clearR.contains (p) || viewR[0].contains (p) || viewR[1].contains (p)));
    if (proc.radar() && view == 0) { for (auto& r : refR) hand |= r.contains (p); for (auto& r : spanR) hand |= r.contains (p); for (auto& r : fasciaR) hand |= r.contains (p); hand |= chartR.contains (p); }
    if (! proc.radar()) for (auto& r : roleR) hand |= r.contains (p);
    setMouseCursor (hand ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
}
void Editor::timerCallback()
{
    if (bgTheme != skin::cur() && bgTheme >= 0) { skin::apply(); bg = {}; }
    auto& H = hub();
    for (int i = 0; i < MAXS; ++i) for (int b = 0; b < NB; ++b) smB[i][b] += 0.4f * (H.s[i].bands[b].load() - smB[i][b]);
    for (int b = 0; b < NB; ++b) smM[b] += 0.4f * ((proc.radar() ? proc.mBands[b].load() : proc.sBands[b].load()) - smM[b]);
    repaint();
}
}
