#include "MeterEditor.h"

namespace jm
{
juce::Font uf (float size, bool bold) { return jlook::ui (size, bold ? 2 : 1); }
juce::Font mf (float size) { return jlook::mono (size); }
void glowText (juce::Graphics& g, const juce::String& t, juce::Rectangle<float> r, juce::Colour c, juce::Font f, juce::Justification j)
{
    g.setFont (f);
    for (int k = 2; k >= 1; --k) { g.setColour (c.withAlpha (0.12f)); g.drawText (t, r.translated ((float) -k * 0.6f, 0), j); g.drawText (t, r.translated ((float) k * 0.6f, 0), j); }
    g.setColour (c); g.drawText (t, r, j);
}
void lcd (juce::Graphics& g, juce::Rectangle<float> r, float rad)
{
    g.setColour (juce::Colours::black.withAlpha (0.65f)); g.fillRoundedRectangle (r.expanded (1.5f), rad + 1.5f);
    juce::ColourGradient cg (C::lcd1, r.getCentreX(), r.getCentreY(), C::lcd2, r.getX(), r.getY(), true); g.setGradientFill (cg); g.fillRoundedRectangle (r, rad);
}
static void scan (juce::Graphics& g, juce::Rectangle<float> r)
{
    g.setColour (juce::Colours::black.withAlpha (0.16f)); for (float y = r.getY(); y < r.getBottom(); y += 3) g.drawHorizontalLine ((int) y, r.getX(), r.getRight());
    jlook::glass (g, r, 6);
}
static juce::String fmtF (float f) { return f >= 1000 ? juce::String (f / 1000, f >= 10000 ? 1 : 2) + " kHz" : juce::String (juce::roundToInt (f)) + " Hz"; }
static juce::String fmtLu (float v) { return v < -69 ? juce::String ("--") : juce::String (v, 1); }

// ======================= Knob =======================
void Knob::mouseDown (const juce::MouseEvent& e)
{
    if (! param) return;
    if (e.mods.isAltDown() || e.mods.isCommandDown()) { param->setValueNotifyingHost (param->getDefaultValue()); return; }
    start = param->getValue(); param->beginChangeGesture(); drag = true;
}
void Knob::mouseDrag (const juce::MouseEvent& e) { if (drag) param->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, start - e.getDistanceFromDragStartY() / (e.mods.isShiftDown() ? 900.0f : 200.0f))); }
void Knob::paint (juce::Graphics& g)
{
    if (! param) return;
    const float size = juce::jmin ((float) getWidth(), getHeight() - 27.0f);
    const float v = param->getValue();
    jlook::KnobLook kl; kl.bipolar = bipolar; kl.hover = hover ? 1.0f : 0.0f; kl.active = drag ? 1.0f : 0.0f;
    jlook::drawKnob (g, { (getWidth() - size) * 0.5f, 0, size, size }, v, col, kl);
    g.setColour (C::dim); g.setFont (uf (9.5f)); g.drawText (label, juce::Rectangle<float> (0, size + 1, (float) getWidth(), 13), juce::Justification::centred);
    glowText (g, text ? text() : param->getCurrentValueAsText(), juce::Rectangle<float> (0, size + 14, (float) getWidth(), 13), col, mf (10.5f), juce::Justification::centred);
}

// ======================= Spettro + EQ =======================
SpecEq::SpecEq (Editor& e) : ed (e) { fbuf.assign ((size_t) kN * 2, 0.0f); for (int i = 0; i < kPts; ++i) { specA[i] = specB[i] = peak[i] = -120; } }
float SpecEq::xOf (float f) const { auto p = plot(); return p.getX() + std::log (juce::jmax (20.0f, f) / 20.0f) / std::log (1000.0f) * p.getWidth(); }
float SpecEq::fOf (float x) const { auto p = plot(); return 20.0f * std::pow (1000.0f, juce::jlimit (0.0f, 1.0f, (x - p.getX()) / p.getWidth())); }
float SpecEq::yOf (float db) const { auto p = plot(); return p.getCentreY() - db / 20.0f * p.getHeight() * 0.5f; }
float SpecEq::dbOf (float y) const { auto p = plot(); return (p.getCentreY() - y) / (p.getHeight() * 0.5f) * 20.0f; }
float SpecEq::ySpec (float db) const { auto p = plot(); return p.getBottom() - juce::jlimit (0.0f, 1.0f, (db + 84.0f) / 90.0f) * p.getHeight(); }

void SpecEq::analyse()
{
    auto& P = ed.proc; const int w = P.ringW.load(); const bool ms = P.value ("spec") < 0.5f; const double sr = P.getSr();
    for (int pass = 0; pass < 2; ++pass)
    {
        for (int i = 0; i < kN; ++i)
        {
            const int k = (w - kN + i) & (Processor::kRing - 1);
            const float l = P.ringL[k], r = P.ringR[k];
            fbuf[(size_t) i] = ms ? (pass == 0 ? 0.5f * (l + r) : 0.5f * (l - r)) : (pass == 0 ? l : r);
        }
        std::fill (fbuf.begin() + kN, fbuf.end(), 0.0f);
        win.multiplyWithWindowingTable (fbuf.data(), (size_t) kN);
        fft.performFrequencyOnlyForwardTransform (fbuf.data());
        float* dst = pass == 0 ? specA : specB;
        const float norm = 4.0f / kN;
        for (int i = 0; i < kPts; ++i)
        {
            const float f0 = 20.0f * std::pow (1000.0f, i / (float) kPts), f1 = 20.0f * std::pow (1000.0f, (i + 1) / (float) kPts);
            int b0 = (int) (f0 / sr * kN), b1 = juce::jmax (b0 + 1, (int) (f1 / sr * kN));
            float m = 0; for (int b = b0; b < juce::jmin (b1, kN / 2); ++b) m = juce::jmax (m, fbuf[(size_t) b]);
            const float fc = std::sqrt (f0 * f1);
            const float db = juce::Decibels::gainToDecibels (m * norm, -140.0f) + 4.5f * std::log2 (fc / 1000.0f);
            dst[i] = db > dst[i] ? db : dst[i] * 0.88f + db * 0.12f;
            if (pass == 0) peak[i] = juce::jmax (db, peak[i] - 0.25f);
        }
        if (pass == 0)
        {   // energia per banda (per il bilanciamento), compensata come il rosa: per ottava
            double e[kBal] = {};
            for (int b = 1; b < kN / 2; ++b)
            {
                const double f = b * sr / kN; if (f < 20 || f > 20000) continue;
                int k = 0; while (k < kBal - 1 && f >= bandEdge (k + 1)) ++k;
                e[k] += (double) fbuf[(size_t) b] * fbuf[(size_t) b] * f;   // x f = pendenza rosa
            }
            for (int k = 0; k < kBal; ++k)
            {
                const double oct = std::log2 (bandEdge (k + 1) / bandEdge (k)), bins = (bandEdge (k + 1) - bandEdge (k)) / sr * kN;
                const float db = (float) (10 * std::log10 (e[k] / juce::jmax (1.0, bins) + 1e-20)); juce::ignoreUnused (oct);
                bandE[k] = bandInit ? bandE[k] * 0.96f + db * 0.04f : db;
            }
            bandInit = true;
        }
    }
}

int SpecEq::nodeAt (juce::Point<float> pt) const
{
    for (int b = kBands - 1; b >= 0; --b)
    {
        if (ed.bp (b, "on") < 0.5f) continue;
        const int t = (int) ed.bp (b, "type");
        const juce::Point<float> c (xOf (ed.bp (b, "freq")), t >= LCUT ? yOf (0) : yOf (ed.bp (b, "gain")));
        if (c.getDistanceFrom (pt) < 12) return b;
    }
    return -1;
}

void SpecEq::paint (juce::Graphics& g)
{
    auto& P = ed.proc; const double sr = P.getSr();
    auto b = getLocalBounds().toFloat().reduced (1);
    jlook::card (g, b, 14, C::phos, true, false, false);
    glowText (g, "SPETTRO  +  EQ DINAMICO", { 18, 8, 300, 22 }, C::text, uf (12.0f));
    g.setFont (mf (9.5f)); g.setColour (C::dim);
    g.drawText (P.value ("spec") < 0.5f ? "ciano = mid   magenta = side   pendenza 4.5 dB/ott" : "ciano = sinistro   magenta = destro   pendenza 4.5 dB/ott", juce::Rectangle<float> (b.getRight() - 470, 8, 456, 22), juce::Justification::centredRight);
    auto p = plot();
    lcd (g, p, 6);
    // griglia
    for (float f : { 30.f, 50.f, 100.f, 200.f, 500.f, 1000.f, 2000.f, 5000.f, 10000.f })
    {
        const float x = xOf (f); g.setColour (C::phos.withAlpha (f == 100 || f == 1000 || f == 10000 ? 0.12f : 0.05f)); g.drawVerticalLine ((int) x, p.getY(), p.getBottom());
        g.setColour (C::faint); g.setFont (mf (8.5f)); g.drawText (f >= 1000 ? juce::String ((int) f / 1000) + "k" : juce::String ((int) f), juce::Rectangle<float> (x - 15, p.getBottom() + 2, 30, 12), juce::Justification::centred);
    }
    for (int k = 1; k < kBal; ++k) { const float x = xOf (bandEdge (k)); g.setColour (C::amber.withAlpha (0.08f)); g.drawVerticalLine ((int) x, p.getY(), p.getBottom()); }
    for (int db : { -12, -6, 6, 12 }) { g.setColour (C::phos.withAlpha (0.05f)); g.drawHorizontalLine ((int) yOf ((float) db), p.getX(), p.getRight()); g.setColour (C::faint); g.setFont (mf (8.5f)); g.drawText ((db > 0 ? "+" : "") + juce::String (db), juce::Rectangle<float> (p.getRight() - 28, yOf ((float) db) - 11, 26, 10), juce::Justification::centredRight); }
    g.setColour (C::phos.withAlpha (0.12f)); g.drawHorizontalLine ((int) yOf (0), p.getX(), p.getRight());
    // spettro
    auto path = [&] (const float* s, bool closed)
    {
        juce::Path pt; if (closed) pt.startNewSubPath (p.getX(), p.getBottom());
        for (int i = 0; i < kPts; ++i) { const float x = p.getX() + p.getWidth() * (i + 0.5f) / kPts, y = ySpec (s[i]); if (! closed && i == 0) pt.startNewSubPath (x, y); else pt.lineTo (x, y); }
        if (closed) { pt.lineTo (p.getRight(), p.getBottom()); pt.closeSubPath(); }
        return pt;
    };
    g.saveState(); g.reduceClipRegion (p.toNearestInt());
    juce::ColourGradient sg (C::phos.withAlpha (0.35f), 0, p.getY(), C::phos.withAlpha (0.04f), 0, p.getBottom(), false); g.setGradientFill (sg); g.fillPath (path (specA, true));
    g.setColour (C::phos.withAlpha (0.9f)); g.strokePath (path (specA, false), juce::PathStrokeType (1.2f));
    g.setColour (C::magenta.withAlpha (0.75f)); g.strokePath (path (specB, false), juce::PathStrokeType (1.0f));
    g.setColour (juce::Colours::white.withAlpha (0.25f)); g.strokePath (path (peak, false), juce::PathStrokeType (0.8f));
    // curva EQ
    const bool eqOn = P.value ("eqOn") > 0.5f;
    Biq tmp[kBands]; bool on[kBands];
    for (int k = 0; k < kBands; ++k) { on[k] = ed.bp (k, "on") > 0.5f; if (on[k]) tmp[k].set ((int) ed.bp (k, "type"), ed.bp (k, "freq"), ed.bp (k, "gain") + P.dynGain[k].load(), ed.bp (k, "q"), sr); }
    juce::Path tot;
    for (int x = 0; x <= (int) p.getWidth(); x += 2)
    {
        const float f = fOf (p.getX() + x); double db = 0;
        for (int k = 0; k < kBands; ++k) if (on[k]) db += tmp[k].magDb (f, sr);
        const float y = yOf ((float) juce::jlimit (-24.0, 24.0, db));
        if (x == 0) tot.startNewSubPath (p.getX() + x, y); else tot.lineTo (p.getX() + x, y);
    }
    for (int k = 0; k < kBands; ++k)
    {
        if (! on[k]) continue;
        juce::Path bpth; bpth.startNewSubPath (p.getX(), yOf (0));
        for (int x = 0; x <= (int) p.getWidth(); x += 3) bpth.lineTo (p.getX() + x, yOf ((float) juce::jlimit (-24.0, 24.0, tmp[k].magDb (fOf (p.getX() + x), sr))));
        bpth.lineTo (p.getRight(), yOf (0)); bpth.closeSubPath();
        const bool cut = (int) ed.bp (k, "type") >= LCUT; g.setColour (C::band[k].withAlpha ((k == ed.selBand ? 0.18f : 0.08f) * (cut ? 0.4f : 1.0f))); g.fillPath (bpth);
    }
    g.setColour ((eqOn ? juce::Colours::white : C::dim).withAlpha (0.35f)); g.strokePath (tot, juce::PathStrokeType (4.0f));
    g.setColour (eqOn ? juce::Colours::white : C::dim); g.strokePath (tot, juce::PathStrokeType (1.6f));
    g.restoreState();
    // nodi
    for (int k = 0; k < kBands; ++k)
    {
        if (! on[k]) continue;
        const int t = (int) ed.bp (k, "type"); const float x = xOf (ed.bp (k, "freq")), y = t >= LCUT ? yOf (0) : yOf (ed.bp (k, "gain"));
        const float dg = P.dynGain[k].load();
        if (std::abs (ed.bp (k, "dyn")) > 0.05f && t <= HSHELF)
        {
            const float y2 = yOf (ed.bp (k, "gain") + ed.bp (k, "dyn"));
            g.setColour (C::band[k].withAlpha (0.35f)); g.drawLine (x, y, x, y2, 2.0f); g.fillEllipse (x - 3, y2 - 3, 6, 6);
            g.setColour (C::band[k]); g.fillEllipse (x - 4, yOf (ed.bp (k, "gain") + dg) - 4, 8, 8);
        }
        const bool sel = k == ed.selBand, hov = k == hoverBand;
        g.setColour (C::band[k].withAlpha (sel || hov ? 0.4f : 0.2f)); g.fillEllipse (x - 13, y - 13, 26, 26);
        g.setColour (juce::Colours::black.withAlpha (0.7f)); g.fillEllipse (x - 9, y - 9, 18, 18);
        g.setColour (C::band[k]); g.drawEllipse (x - 9, y - 9, 18, 18, sel ? 2.2f : 1.4f);
        g.setFont (mf (10.0f)); g.drawText (juce::String (k + 1), juce::Rectangle<float> (x - 9, y - 9, 18, 18), juce::Justification::centred);
        if (sel || hov)
        {
            const juce::String t2 = fmtF (ed.bp (k, "freq")) + (t >= LCUT ? juce::String() : "  " + juce::String (ed.bp (k, "gain"), 1) + " dB");
            jlook::valueBubble (g, { x - 60, y + 16, 120, 18 }, t2, C::band[k]);
        }
    }
    if (hoverBand < 0 && hoverF > 0) { g.setColour (C::dim); g.setFont (mf (9.0f)); g.drawText (fmtF (hoverF) + "   clic = nuova banda", juce::Rectangle<float> (p.getX() + 8, p.getY() + 6, 260, 12), juce::Justification::centredLeft); }
    scan (g, p);
}

void SpecEq::mouseMove (const juce::MouseEvent& e)
{
    const int h = nodeAt (e.position); hoverF = plot().contains (e.position) ? fOf (e.position.x) : 0;
    if (h != hoverBand) hoverBand = h;
    setMouseCursor (h >= 0 ? juce::MouseCursor::DraggingHandCursor : juce::MouseCursor::CrosshairCursor);
}
void SpecEq::mouseDown (const juce::MouseEvent& e)
{
    const int n = nodeAt (e.position);
    if (n >= 0)
    {
        ed.selBand = n; ed.band.bind();
        if (e.mods.isPopupMenu())
        {
            juce::PopupMenu m; auto* tp = dynamic_cast<juce::AudioParameterChoice*> (ed.bpar (n, "type"));
            for (int i = 0; i < tp->choices.size(); ++i) m.addItem (i + 1, tp->choices[i], true, i == tp->getIndex());
            m.addSeparator(); m.addItem (100, "Rimuovi banda");
            m.showMenuAsync (juce::PopupMenu::Options(), [this, n, tp] (int r) { if (r == 100) ed.setP (ed.bpar (n, "on"), 0); else if (r > 0) tp->setValueNotifyingHost (tp->convertTo0to1 ((float) (r - 1))); });
            return;
        }
        dragBand = n; ed.bpar (n, "freq")->beginChangeGesture(); ed.bpar (n, "gain")->beginChangeGesture();
        return;
    }
    if (! plot().contains (e.position) || e.mods.isPopupMenu()) return;
    for (int k = 0; k < kBands; ++k)
        if (ed.bp (k, "on") < 0.5f)
        {
            const float f = fOf (e.position.x);
            ed.setP (ed.bpar (k, "on"), 1); ed.setP (ed.bpar (k, "type"), f < 40 ? LCUT : f > 16000 ? HCUT : BELL);
            ed.setP (ed.bpar (k, "freq"), f); ed.setP (ed.bpar (k, "gain"), juce::jlimit (-18.0f, 18.0f, dbOf (e.position.y)));
            ed.selBand = k; ed.band.bind(); dragBand = k;
            ed.bpar (k, "freq")->beginChangeGesture(); ed.bpar (k, "gain")->beginChangeGesture();
            return;
        }
}
void SpecEq::mouseDrag (const juce::MouseEvent& e)
{
    if (dragBand < 0) return;
    ed.setP (ed.bpar (dragBand, "freq"), juce::jlimit (20.0f, 20000.0f, fOf (e.position.x)));
    if ((int) ed.bp (dragBand, "type") < LCUT) ed.setP (ed.bpar (dragBand, "gain"), juce::jlimit (-18.0f, 18.0f, dbOf (e.position.y)));
}
void SpecEq::mouseUp (const juce::MouseEvent&)
{
    if (dragBand >= 0) { ed.bpar (dragBand, "freq")->endChangeGesture(); ed.bpar (dragBand, "gain")->endChangeGesture(); }
    dragBand = -1;
}
void SpecEq::mouseDoubleClick (const juce::MouseEvent& e) { const int n = nodeAt (e.position); if (n >= 0) ed.setP (ed.bpar (n, "on"), 0); }
void SpecEq::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    const int n = nodeAt (e.position); if (n < 0) return;
    auto* q = ed.bpar (n, "q"); q->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, q->getValue() + (w.deltaY > 0 ? 0.03f : -0.03f)));
}

// ======================= Loudness =======================
void Loud::mouseDown (const juce::MouseEvent& e)
{
    if (resetR.contains (e.position)) ed.proc.resetReq = true;
    else if (targetR.contains (e.position))
    {
        juce::PopupMenu m; auto* tp = dynamic_cast<juce::AudioParameterChoice*> (ed.P ("target"));
        for (int i = 0; i < tp->choices.size(); ++i) m.addItem (i + 1, tp->choices[i], true, i == tp->getIndex());
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [tp] (int r) { if (r > 0) tp->setValueNotifyingHost (tp->convertTo0to1 ((float) (r - 1))); });
    }
}
void Loud::paint (juce::Graphics& g)
{
    auto& P = ed.proc; auto b = getLocalBounds().toFloat().reduced (1);
    jlook::card (g, b, 14, C::amber, true, false, false);
    glowText (g, "LOUDNESS", { 18, 8, 160, 22 }, C::text, uf (12.0f));
    const auto& T = target ((int) P.value ("target"));
    targetR = { b.getRight() - 210, 8, 120, 24 }; resetR = { b.getRight() - 84, 8, 68, 24 };
    jlook::button (g, targetR, C::amber, true, false, juce::String (T.name) + juce::String::fromUTF8 (" \xe2\x96\xbe"), uf (11.0f));
    jlook::button (g, resetR, C::magenta, false, false, "RESET", uf (11.0f));
    auto d = juce::Rectangle<float> (16, 42, b.getWidth() - 30, 88); lcd (g, d, 8);
    const float I = P.lufsI.load(), delta = I - T.lufs;
    const auto ic = I < -69 ? C::dim : std::abs (delta) <= 1 ? C::green : delta > 1 ? C::amber : C::phos;
    glowText (g, fmtLu (I), d.withTrimmedBottom (34).withTrimmedLeft (14).withTrimmedTop (4), ic, mf (40.0f), juce::Justification::centredLeft);
    g.setColour (C::dim); g.setFont (uf (11.0f)); g.drawText ("LUFS INTEGRATI", juce::Rectangle<float> (d.getRight() - 150, d.getY() + 10, 136, 16), juce::Justification::centredRight);
    juce::String verdict = I < -69 ? juce::String ("in attesa di audio") : std::abs (delta) <= 1 ? juce::String ("sul target") : (delta > 0 ? "+" : "") + juce::String (delta, 1) + " LU " + (delta > 0 ? "sopra: la piattaforma abbassera" : "sotto: puoi spingere di piu");
    glowText (g, verdict, d.withTrimmedLeft (16).withTrimmedTop (60).withHeight (18), ic, uf (11.0f));
    struct V { const char* n; juce::String v; juce::Colour c; };
    const float tp = P.tpMax.load(), plr = (tp > -90 && I > -69) ? tp - I : -999, st = P.lufsS.load();
    const float rms = juce::jmax (P.rmsL.load(), P.rmsR.load()), pk = juce::jmax (P.pkHoldL.load(), P.pkHoldR.load());
    auto dbs = [] (float v, const char* u) { return v < -90 ? juce::String ("--") : juce::String (v, 1) + u; };
    V vals[] = { { "MOMENTARY", fmtLu (P.lufsM.load()), C::phos }, { "SHORT-TERM", fmtLu (st), C::phos }, { "LRA", juce::String (P.lra.load(), 1) + " LU", C::violet },
                 { "TRUE PEAK MAX", dbs (tp, " dBTP"), tp > T.tp ? C::red : C::green }, { "PLR", plr < -900 ? juce::String ("--") : juce::String (plr, 1) + " dB", C::amber }, { "PICCO MAX", dbs (P.pkMax.load(), " dBFS"), C::dim },
                 { "RMS", dbs (rms, " dBFS"), C::phos }, { "CREST", (rms > -90 && pk > -90) ? juce::String (pk - rms, 1) + " dB" : juce::String ("--"), C::amber }, { "PSR", (tp > -90 && st > -69) ? juce::String (tp - st, 1) + " dB" : juce::String ("--"), C::violet } };
    const float cw = (b.getWidth() - 30) / 3;
    for (int i = 0; i < 9; ++i)
    {
        auto r = juce::Rectangle<float> (16 + (i % 3) * cw, 138 + (i / 3) * 40.0f, cw - 6, 35);
        lcd (g, r, 5);
        g.setColour (C::dim); g.setFont (uf (8.5f)); g.drawText (vals[i].n, r.reduced (8, 2).withHeight (12), juce::Justification::centredLeft);
        glowText (g, vals[i].v, r.reduced (8, 2).withTrimmedTop (12), vals[i].c, mf (13.0f));
    }
    // livelli L/R in dBFS: barra = RMS, segmento = picco, tacca = tenuta
    {
        auto mr = juce::Rectangle<float> (16, 262, b.getWidth() - 30, 40); lcd (g, mr, 5);
        auto in = mr.reduced (24, 6).withTrimmedBottom (8);
        auto xD = [&in] (float db) { return in.getX() + juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f) * in.getWidth(); };
        for (int c = 0; c < 2; ++c)
        {
            auto r = juce::Rectangle<float> (in.getX(), in.getY() + c * 11.0f, in.getWidth(), 9);
            g.setColour (C::dim); g.setFont (mf (8.5f)); g.drawText (c ? "R" : "L", juce::Rectangle<float> (mr.getX() + 6, r.getY() - 1, 14, 11), juce::Justification::centredLeft);
            g.setColour (juce::Colours::white.withAlpha (0.05f)); g.fillRoundedRectangle (r, 2);
            const float rm = c ? P.rmsR.load() : P.rmsL.load(), pp = c ? P.pkR.load() : P.pkL.load(), ph = c ? P.pkHoldR.load() : P.pkHoldL.load();
            if (pp > -60) { g.setColour (C::phos.withAlpha (0.3f)); g.fillRoundedRectangle (r.withRight (xD (pp)), 2); }
            if (rm > -60) { juce::ColourGradient lg (C::phos, r.getX(), 0, rm > -6 ? C::red : rm > -12 ? C::amber : C::green, xD (rm), 0, false); g.setGradientFill (lg); g.fillRoundedRectangle (r.withRight (xD (rm)), 2); }
            if (ph > -60) { g.setColour (ph > -0.3f ? C::red : C::text); g.fillRect (xD (ph) - 1, r.getY() - 1, 2.0f, r.getHeight() + 2); }
        }
        for (int db : { -48, -36, -24, -18, -12, -6, -3, 0 }) { g.setColour (C::faint); g.setFont (mf (7.5f)); g.drawText (juce::String (db), juce::Rectangle<float> (xD ((float) db) - 10, in.getBottom() + 1, 20, 9), juce::Justification::centred); }
    }
    // storia
    auto h = juce::Rectangle<float> (16, 310, b.getWidth() - 30, b.getHeight() - 322); lcd (g, h, 6);
    auto yH = [&h] (float l) { return h.getBottom() - juce::jlimit (0.0f, 1.0f, (l + 36.0f) / 36.0f) * h.getHeight(); };
    g.setColour (C::amber.withAlpha (0.5f)); g.drawHorizontalLine ((int) yH (T.lufs), h.getX(), h.getRight());
    g.setColour (C::faint); g.setFont (mf (8.5f)); g.drawText (juce::String ((int) T.lufs), juce::Rectangle<float> (h.getX() + 3, yH (T.lufs) - 11, 30, 10), juce::Justification::centredLeft);
    const int NH = Processor::kHist, w0 = P.histW.load(); juce::Path pm, ps; bool sm = false, ss = false;
    for (int k = 0; k < NH; ++k)
    {
        const int i = (w0 + k) % NH; const float x = h.getX() + h.getWidth() * k / (NH - 1);
        const float m = P.histM[i].load(), st = P.histS[i].load();
        if (m > -69) { if (! sm) { pm.startNewSubPath (x, yH (m)); sm = true; } else pm.lineTo (x, yH (m)); } else sm = false;
        if (st > -69) { if (! ss) { ps.startNewSubPath (x, yH (st)); ss = true; } else ps.lineTo (x, yH (st)); } else ss = false;
    }
    g.setColour (C::phos.withAlpha (0.3f)); g.strokePath (pm, juce::PathStrokeType (1.0f));
    g.setColour (C::amber.withAlpha (0.3f)); g.strokePath (ps, juce::PathStrokeType (4.0f)); g.setColour (C::amber); g.strokePath (ps, juce::PathStrokeType (1.5f));
    g.setColour (C::dim); g.setFont (mf (8.5f)); g.drawText ("ultimi 2 min   ambra = short-term   ciano = momentary", h.reduced (6, 3), juce::Justification::bottomLeft);
    scan (g, h);
}

// ======================= Bilanciamento =======================
void Balance::mouseDown (const juce::MouseEvent& e)
{
    if (! profR.contains (e.position)) return;
    juce::PopupMenu m; auto* tp = dynamic_cast<juce::AudioParameterChoice*> (ed.P ("profile"));
    for (int i = 0; i < tp->choices.size(); ++i) m.addItem (i + 1, tp->choices[i], true, i == tp->getIndex());
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [tp] (int r) { if (r > 0) tp->setValueNotifyingHost (tp->convertTo0to1 ((float) (r - 1))); });
}
void Balance::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (1);
    jlook::card (g, b, 14, C::green, true, false, false);
    glowText (g, "BILANCIAMENTO", { 18, 8, 160, 22 }, C::text, uf (12.0f));
    const auto& Pf = profile ((int) ed.proc.value ("profile"));
    profR = { b.getRight() - 150, 8, 134, 24 };
    jlook::button (g, profR, C::green, true, false, juce::String (Pf.name) + juce::String::fromUTF8 (" \xe2\x96\xbe"), uf (10.5f));
    auto area = juce::Rectangle<float> (16, 42, b.getWidth() - 30, b.getHeight() - 58); lcd (g, area, 6);
    auto plotA = area.reduced (8, 24);
    const auto& E = ed.spec.bandE; float mean = 0; for (int k = 0; k < kBal; ++k) mean += E[k]; mean /= kBal;
    const bool live = ed.proc.lufsS.load() > -60;
    auto yD = [&plotA] (float d) { return plotA.getCentreY() - juce::jlimit (-10.0f, 10.0f, d) / 10.0f * plotA.getHeight() * 0.5f; };
    g.setColour (C::green.withAlpha (0.08f)); g.drawHorizontalLine ((int) yD (0), plotA.getX(), plotA.getRight());
    const float cw = plotA.getWidth() / kBal;
    for (int k = 0; k < kBal; ++k)
    {
        const float dev = E[k] - mean - Pf.off[k];
        const float x = plotA.getX() + k * cw + cw * 0.22f, w = cw * 0.56f;
        g.setColour (C::green.withAlpha (0.10f)); g.fillRoundedRectangle (x - 3, yD (2), w + 6, yD (-2) - yD (2), 3);   // zona ok
        const auto c = ! live ? C::faint : std::abs (dev) <= 2 ? C::green : std::abs (dev) <= 4 ? C::amber : C::red;
        if (live)
        {
            const float y0 = yD (0), y1 = yD (dev);
            juce::ColourGradient bg2 (c.withAlpha (0.9f), 0, y1, c.withAlpha (0.25f), 0, y0, false); g.setGradientFill (bg2);
            g.fillRoundedRectangle (x, juce::jmin (y0, y1), w, juce::jmax (2.0f, std::abs (y1 - y0)), 3);
            glowText (g, (dev > 0 ? "+" : "") + juce::String (dev, 1), { x - 10, area.getY() + 4, w + 20, 14 }, c, mf (10.0f), juce::Justification::centred);
        }
        g.setColour (C::dim); g.setFont (uf (8.5f)); g.drawText (bandName (k), juce::Rectangle<float> (x - 14, area.getBottom() - 20, w + 28, 12), juce::Justification::centred);
    }
    g.setColour (C::faint); g.setFont (mf (8.0f)); g.drawText ("scarto in dB dal profilo (indicativo)", area.reduced (6, 2), juce::Justification::bottomLeft);
    scan (g, area);
}

// ======================= Stereo =======================
Stereo::Stereo (Editor& e) : ed (e)
{
    auto pc = [] (float v) { return juce::String (juce::roundToInt (v * 100)) + "%"; };
    struct K { Knob* k; const char* id; const char* l; bool bip; };
    for (auto x : { K { &width, "imWidth", "WIDTH", true }, K { &bal, "imBal", "BILANCIO", true }, K { &low, "imLow", "LARG. BASSI", true }, K { &mid, "imMid", "LARG. MEDI", true }, K { &high, "imHigh", "LARG. ALTI", true }, K { &mono, "imMono", "MONO BASSI", false } })
    { x.k->param = ed.P (x.id); x.k->label = x.l; x.k->bipolar = x.bip; addAndMakeVisible (x.k); }
    for (auto* k : { &width, &low, &mid, &high }) { auto* kk = k; kk->text = [kk, pc] { return pc (kk->param->convertFrom0to1 (kk->param->getValue())); }; }
    bal.text = [this] { const float v = ed.proc.value ("imBal"); return std::abs (v) < 0.01f ? juce::String ("C") : juce::String (juce::roundToInt (std::abs (v) * 100)) + (v > 0 ? " R" : " L"); };
    mono.text = [this] { const float v = ed.proc.value ("imMono"); return v < 21 ? juce::String ("OFF") : "< " + juce::String (juce::roundToInt (v)) + " Hz"; };
}
void Stereo::resized()
{
    const int x0 = getWidth() - 166, w = 78;
    Knob* ks[] = { &width, &bal, &low, &mid, &high, &mono };
    for (int i = 0; i < 6; ++i) ks[i]->setBounds (x0 + (i % 2) * w, 40 + (i / 2) * 76, w, 74);
}
void Stereo::mouseDown (const juce::MouseEvent& e)
{
    if (monoR.contains (e.position)) ed.setP (ed.P ("monoChk"), ed.proc.value ("monoChk") > 0.5f ? 0.0f : 1.0f);
    else if (onR.contains (e.position)) ed.setP (ed.P ("imOn"), ed.proc.value ("imOn") > 0.5f ? 0.0f : 1.0f);
    repaint();
}
void Stereo::paint (juce::Graphics& g)
{
    auto& P = ed.proc; auto b = getLocalBounds().toFloat().reduced (1);
    jlook::card (g, b, 14, C::violet, true, false, false);
    glowText (g, "IMMAGINE STEREO", { 18, 8, 200, 22 }, C::text, uf (12.0f));
    monoR = { b.getRight() - 112, 8, 96, 24 }; onR = { b.getRight() - 224, 8, 104, 24 };
    jlook::button (g, monoR, C::amber, P.value ("monoChk") > 0.5f, false, "MONO CHECK", uf (10.5f));
    jlook::button (g, onR, C::violet, P.value ("imOn") > 0.5f, false, P.value ("imOn") > 0.5f ? "IMAGER ON" : "IMAGER OFF", uf (10.5f));
    const float gs = b.getHeight() - 54; auto gr = juce::Rectangle<float> (16, 42, gs, gs); lcd (g, gr, 6);
    // goniometro con persistenza
    if (persist.isNull() || persist.getWidth() != (int) gs) persist = juce::Image (juce::Image::ARGB, (int) gs, (int) gs, true);
    {
        juce::Graphics pg (persist);
        pg.setColour (juce::Colours::black.withAlpha (0.22f)); pg.fillAll(); // dissolvenza
        persist.multiplyAllAlphas (0.78f);
        const int w = P.ringW.load(), n = 2048; double e = 0;
        for (int i = 0; i < n; ++i) { const int k = (w - n + i) & (Processor::kRing - 1); e += P.ringL[k] * P.ringL[k] + P.ringR[k] * P.ringR[k]; }
        const float rms = (float) std::sqrt (e / (2 * n)), sc = gs * 0.28f / juce::jmax (0.02f, rms * 2.2f), c0 = gs * 0.5f;
        pg.setColour (C::green.withAlpha (0.55f));
        for (int i = 0; i < n; i += 2)
        {
            const int k = (w - n + i) & (Processor::kRing - 1); const float l = P.ringL[k], r = P.ringR[k];
            const float x = c0 + (r - l) * 0.707f * sc, y = c0 - (l + r) * 0.707f * sc;
            if (x > 0 && y > 0 && x < gs && y < gs) pg.fillRect (x, y, 1.4f, 1.4f);
        }
    }
    g.drawImageAt (persist, (int) gr.getX(), (int) gr.getY());
    g.setColour (C::phos.withAlpha (0.12f)); g.drawLine (gr.getCentreX(), gr.getY() + 6, gr.getCentreX(), gr.getBottom() - 6); g.drawLine (gr.getX() + 6, gr.getCentreY(), gr.getRight() - 6, gr.getCentreY());
    g.drawLine (gr.getX() + 20, gr.getY() + 20, gr.getRight() - 20, gr.getBottom() - 20); g.drawLine (gr.getRight() - 20, gr.getY() + 20, gr.getX() + 20, gr.getBottom() - 20);
    g.setColour (C::dim); g.setFont (mf (8.5f));
    g.drawText ("M", juce::Rectangle<float> (gr.getCentreX() - 8, gr.getY() + 3, 16, 10), juce::Justification::centred);
    g.drawText ("L", juce::Rectangle<float> (gr.getX() + 8, gr.getY() + 10, 12, 10), juce::Justification::centred); g.drawText ("R", juce::Rectangle<float> (gr.getRight() - 20, gr.getY() + 10, 12, 10), juce::Justification::centred);
    scan (g, gr);
    // correlazione e larghezza
    auto rx = gr.getRight() + 12, rw = b.getRight() - 180 - rx;
    auto meterH = [&] (float y, const char* name, float v, float lo, float hi, bool corrScale, juce::String val)
    {
        g.setColour (C::dim); g.setFont (uf (9.5f)); g.drawText (name, juce::Rectangle<float> (rx, y, rw, 13), juce::Justification::centredLeft);
        g.setFont (mf (10.0f)); g.drawText (val, juce::Rectangle<float> (rx, y, rw, 13), juce::Justification::centredRight);
        auto r = juce::Rectangle<float> (rx, y + 15, rw, 12); lcd (g, r, 4);
        const float t = juce::jlimit (0.0f, 1.0f, (v - lo) / (hi - lo)), c0 = corrScale ? r.getCentreX() : r.getX();
        const auto col = corrScale ? (v < 0 ? C::red : v < 0.3f ? C::amber : C::green) : (v > 1.2f ? C::amber : C::violet);
        const float x = r.getX() + t * r.getWidth();
        g.setColour (col.withAlpha (0.85f)); g.fillRoundedRectangle (juce::jmin (c0, x), r.getY() + 3, std::abs (x - c0) + 2, r.getHeight() - 6, 2);
        if (corrScale) { g.setColour (C::dim.withAlpha (0.5f)); g.drawVerticalLine ((int) r.getCentreX(), r.getY(), r.getBottom()); }
    };
    meterH (40, "CORRELAZIONE", P.corr.load(), -1, 1, true, juce::String (P.corr.load(), 2));
    meterH (76, "LARGHEZZA S/M", P.width.load(), 0, 1.5f, false, juce::String (juce::roundToInt (P.width.load() * 100)) + "%");
    const float bal = P.balance.load();
    meterH (112, "BILANCIO L/R", juce::jlimit (-6.0f, 6.0f, bal), -6, 6, true, (std::abs (bal) < 0.25f ? juce::String ("centro") : juce::String (std::abs (bal), 1) + " dB " + (bal > 0 ? "R" : "L")));
    const char* bn[3] = { "BASSI < 150 Hz", "MEDI", "ALTI > 2 kHz" };
    for (int k = 0; k < 3; ++k) meterH (152 + k * 34.0f, bn[k], P.corrB[k].load(), -1, 1, true, juce::String (P.corrB[k].load(), 2) + "  " + juce::String (juce::roundToInt (P.widthB[k].load() * 100)) + "%");
    juce::String warnT; juce::Colour wc = C::green;
    if (P.lufsS.load() < -60) { warnT = "in attesa di audio"; wc = C::dim; }
    else if (P.corr.load() < 0) { warnT = "Fase invertita: in mono il mix si svuota!"; wc = C::red; }
    else if (P.corrB[0].load() < 0.6f) { warnT = "Bassi troppo larghi: mettili in mono sotto 120-150 Hz"; wc = C::amber; }
    else if (P.width.load() > 1.0f) { warnT = "Molto largo: controlla la compatibilita mono"; wc = C::amber; }
    else warnT = "Compatibile mono";
    glowText (g, warnT, { rx, b.getBottom() - 30, rw, 16 }, wc, uf (10.5f));
}

// ======================= Banda EQ =======================
BandPanel::BandPanel (Editor& e) : ed (e)
{
    freq.label = "FREQ"; gain.label = "GAIN"; q.label = "Q"; dyn.label = "DYNAMIC"; thr.label = "SOGLIA";
    gain.bipolar = dyn.bipolar = true;
    for (auto* k : { &freq, &gain, &q, &dyn, &thr }) addAndMakeVisible (k);
    bind();
}
void BandPanel::bind()
{
    const int b = ed.selBand;
    freq.param = ed.bpar (b, "freq"); gain.param = ed.bpar (b, "gain"); q.param = ed.bpar (b, "q"); dyn.param = ed.bpar (b, "dyn"); thr.param = ed.bpar (b, "thr");
    freq.text = [this, b] { return fmtF (ed.bp (b, "freq")); };
    gain.text = [this, b] { const float v = ed.bp (b, "gain"); return (v > 0 ? "+" : "") + juce::String (v, 1) + " dB"; };
    q.text = [this, b] { return juce::String (ed.bp (b, "q"), 2); };
    dyn.text = [this, b] { const float v = ed.bp (b, "dyn"); return std::abs (v) < 0.05f ? juce::String ("OFF") : (v > 0 ? "+" : "") + juce::String (v, 1) + " dB"; };
    thr.text = [this, b] { return juce::String (ed.bp (b, "thr"), 1) + " dB"; };
    for (auto* k : { &freq, &gain, &q, &dyn, &thr }) k->col = k == &dyn || k == &thr ? C::magenta : C::band[b];
    repaint();
}
void BandPanel::resized()
{
    const int y = 132, w = (getWidth() - 24) / 5;
    int i = 0; for (auto* k : { &freq, &gain, &q, &dyn, &thr }) { k->setBounds (12 + i * w + (w - 64) / 2, y, 64, 64 + 27); ++i; }
}
void BandPanel::mouseDown (const juce::MouseEvent& e)
{
    const float cw = (getWidth() - 32) / 6.0f;
    if (soloR.contains (e.position)) { ed.proc.soloBand = ed.proc.soloBand.load() == ed.selBand ? -1 : ed.selBand; repaint(); return; }
    if (e.position.y > 38 && e.position.y < 90)
    {
        const int b = juce::jlimit (0, kBands - 1, (int) ((e.position.x - 16) / cw) + (e.position.y > 64 ? 6 : 0));
        if (b == ed.selBand) ed.setP (ed.bpar (b, "on"), ed.bp (b, "on") > 0.5f ? 0.0f : 1.0f);
        if (ed.proc.soloBand.load() >= 0) ed.proc.soloBand = b;
        ed.selBand = b; bind(); ed.spec.repaint();
    }
    else if (e.position.y > 96 && e.position.y < 122)
    {
        const float tw = (getWidth() - 32) / (float) kNumTypes; const int t = juce::jlimit (0, kNumTypes - 1, (int) ((e.position.x - 16) / tw));
        ed.setP (ed.bpar (ed.selBand, "type"), (float) t); ed.setP (ed.bpar (ed.selBand, "on"), 1); repaint();
    }
}
void BandPanel::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (1); const int s = ed.selBand;
    jlook::card (g, b, 14, C::band[s], true, false, false);
    glowText (g, "BANDA EQ " + juce::String (s + 1), { 18, 8, 150, 22 }, C::text, uf (12.0f));
    soloR = { b.getRight() - 90, 7, 74, 24 };
    jlook::button (g, soloR, C::amber, ed.proc.soloBand.load() == s, false, "SOLO", uf (11.0f));
    const float cw = (b.getWidth() - 32) / 6;
    for (int k = 0; k < kBands; ++k)
    {
        auto r = juce::Rectangle<float> (16 + (k % 6) * cw, 40 + (k / 6) * 26.0f, cw - 4, 22); const bool on = ed.bp (k, "on") > 0.5f;
        jlook::button (g, r, C::band[k], k == s, false, juce::String (k + 1), uf (11.0f));
        g.setColour (on ? C::band[k] : C::faint); g.fillEllipse (r.getX() + 6, r.getCentreY() - 3, 6, 6);
    }
    const char* tn[] = { "BELL", "LOW SH", "HIGH SH", "LOW CUT", "HIGH CUT" };
    const float tw = (b.getWidth() - 32) / kNumTypes; const int ty = (int) ed.bp (s, "type");
    auto tr = juce::Rectangle<float> (16, 96, b.getWidth() - 32, 26); lcd (g, tr, 6);
    for (int t = 0; t < kNumTypes; ++t)
    {
        auto r = juce::Rectangle<float> (16 + t * tw, 97, tw, 24).reduced (2, 0);
        if (t == ty) { g.setColour (C::band[s].withAlpha (0.2f)); g.fillRoundedRectangle (r, 4); g.setColour (C::band[s]); g.drawRoundedRectangle (r, 4, 1.0f); glowText (g, tn[t], r, C::band[s], uf (10.0f), juce::Justification::centred); }
        else { g.setColour (C::dim); g.setFont (uf (10.0f)); g.drawText (tn[t], r, juce::Justification::centred); }
    }
    g.setColour (C::dim); g.setFont (uf (9.5f, false));
    g.drawFittedText ("DYNAMIC: quanto la banda si muove sopra la SOGLIA (negativo = de-esser). SOLO: ascolti solo quella zona. Clic sullo spettro = nuova banda, doppio clic sul nodo = toglila.",
                      juce::Rectangle<int> (16, (int) b.getHeight() - 46, (int) b.getWidth() - 32, 40), juce::Justification::topLeft, 3);
}

// ======================= Editor =======================
Editor::Editor (Processor& p) : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);
    for (auto* c : std::initializer_list<juce::Component*> { &spec, &loud, &bal, &st, &band }) addAndMakeVisible (c);
    addAndMakeVisible (sizeChip);
    scaler.setup();
    startTimerHz (30);
}
void Editor::resized()
{
    const float sc = scaler.update();
    specRect = { 470, 15, 220, 36 }; eqRect = { 700, 15, 120, 36 }; abRect = { 830, 15, 110, 36 };
    spec.setBounds (18, 80, 820, 410); loud.setBounds (850, 80, 372, 410);
    bal.setBounds (18, 502, 250, 284); st.setBounds (278, 502, 560, 284); band.setBounds (850, 502, 372, 284);
    sizeChip.setBounds (W - 76, H - 18, 58, 15);
    jlook::scaleChildren (*this, sc);
}
void Editor::paint (juce::Graphics& g)
{
    g.addTransform (juce::AffineTransform::scale (scaler.s));
    const float Wf = (float) W, Hf = (float) H;
    jlook::backdrop (g, { 0, 0, Wf, Hf }, C::magenta, C::phos);
    jlook::header (g, { 0, 0, Wf, 64 }, "METERING HUB", C::green, "JM-1  ANALISI PRE / POST MASTER", C::magenta, C::phos);
    for (auto* c : std::initializer_list<juce::Component*> { &spec, &loud, &bal, &st, &band }) jlook::softShadow (g, c->getBounds().toFloat(), 14);
    jlook::button (g, specRect.toFloat(), C::phos, true, false, proc.value ("spec") < 0.5f ? "SPETTRO: MID + SIDE" : "SPETTRO: L + R", uf (11.5f));
    jlook::button (g, eqRect.toFloat(), C::magenta, proc.value ("eqOn") > 0.5f, false, proc.value ("eqOn") > 0.5f ? "EQ ATTIVO" : "EQ BYPASS", uf (11.5f));
    jlook::button (g, abRect.toFloat(), C::phos, proc.ab.cur == 1, false, "CONFRONTO " + proc.ab.label(), uf (11.5f));
    g.setColour (C::dim); g.setFont (mf (9.0f)); g.drawText ("analisi: latenza zero", juce::Rectangle<float> (Wf - 250, 20, 230, 24), juce::Justification::centredRight);
}
void Editor::mouseDown (const juce::MouseEvent& e0)
{
    const auto e = e0.withNewPosition (e0.position / scaler.s);
    if (specRect.contains (e.getPosition())) setP (P ("spec"), proc.value ("spec") < 0.5f ? 1.0f : 0.0f);
    else if (eqRect.contains (e.getPosition())) setP (P ("eqOn"), proc.value ("eqOn") > 0.5f ? 0.0f : 1.0f);
    else if (abRect.contains (e.getPosition())) { proc.ab.toggle (proc); band.bind(); }
    repaint();
}
void Editor::timerCallback()
{
    spec.analyse();
    spec.repaint(); loud.repaint(); st.repaint(); bal.repaint();
    for (auto* k : { &band.freq, &band.gain, &band.q, &band.dyn, &band.thr, &st.width, &st.bal, &st.low, &st.mid, &st.high, &st.mono }) k->repaint();
}
}
