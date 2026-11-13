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
    if (jlook::skinHooks().on && jlook::skinHooks().screen) { jlook::skinHooks().screen (g, r, rad, C::phos); return; }
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


// ======================= Riquadro centrale =======================
static const char* kViewNames[5] = { "EQ", "REFERENCE", "IMAGER", "PEAK", "SPETTROGRAMMA" };
SpecEq::SpecEq (Editor& e) : ed (e)
{
    fbuf.assign ((size_t) kN * 2, 0.0f); for (int i = 0; i < kPts; ++i) { specA[i] = specB[i] = peak[i] = frameP[i] = -120; }
    addChildComponent (pop);
}
int SpecEq::view() const { return juce::jlimit (0, 4, (int) ed.proc.value ("view")); }
float SpecEq::xOf (float f) const { auto p = plot(); return p.getX() + std::log (juce::jmax (20.0f, f) / 20.0f) / std::log (1000.0f) * p.getWidth(); }
float SpecEq::fOf (float x) const { auto p = plot(); return 20.0f * std::pow (1000.0f, juce::jlimit (0.0f, 1.0f, (x - p.getX()) / p.getWidth())); }
float SpecEq::yOf (float db) const { auto p = plot(); return p.getCentreY() - db / 20.0f * p.getHeight() * 0.5f; }
float SpecEq::dbOf (float y) const { auto p = plot(); return (p.getCentreY() - y) / (p.getHeight() * 0.5f) * 20.0f; }
float SpecEq::ySpec (float db) const { auto p = plot(); return p.getBottom() - juce::jlimit (0.0f, 1.0f, (db + 84.0f) / 90.0f) * p.getHeight(); }
float SpecEq::ltasDb (int i) const { return ltasN > 0 ? (float) (10 * std::log10 (ltasPow[i] / ltasN + 1e-30)) + tilt (ptF (i)) : -120.0f; }

void SpecEq::analyse()
{
    auto& P = ed.proc; const int w = P.ringW.load(); const bool ms = P.value ("spec") < 0.5f; const double sr = P.getSr();
    const bool frozen = P.frozen.load(), sc = P.hasSc.load() && P.value ("scShow") > 0.5f;
    if (frozen) return;   // fermo: lo spettro resta l'istantanea
    for (int pass = 0; pass < (sc ? 3 : 2); ++pass)
    {
        for (int i = 0; i < kN; ++i)
        {
            const int k = (w - kN + i) & (Processor::kRing - 1);
            const float l = P.ringL[k], r = P.ringR[k];
            fbuf[(size_t) i] = pass == 2 ? P.ringS[k] : ms ? (pass == 0 ? 0.5f * (l + r) : 0.5f * (l - r)) : (pass == 0 ? l : r);
        }
        std::fill (fbuf.begin() + kN, fbuf.end(), 0.0f);
        win.multiplyWithWindowingTable (fbuf.data(), (size_t) kN);
        fft.performFrequencyOnlyForwardTransform (fbuf.data());
        float* dst = pass == 0 ? specA : pass == 1 ? specB : specC;
        const float norm = 4.0f / kN;
        for (int i = 0; i < kPts; ++i)
        {
            const float f0 = 20.0f * std::pow (1000.0f, i / (float) kPts), f1 = 20.0f * std::pow (1000.0f, (i + 1) / (float) kPts);
            int b0 = (int) (f0 / sr * kN), b1 = juce::jmax (b0 + 1, (int) (f1 / sr * kN));
            float m = 0; double pw = 0; int nb = 0;
            for (int b = b0; b < juce::jmin (b1, kN / 2); ++b) { m = juce::jmax (m, fbuf[(size_t) b]); pw += (double) fbuf[(size_t) b] * fbuf[(size_t) b]; ++nb; }
            const float fc = std::sqrt (f0 * f1);
            const float db = juce::Decibels::gainToDecibels (m * norm, -140.0f) + 4.5f * std::log2 (fc / 1000.0f);
            dst[i] = db > dst[i] ? db : dst[i] * 0.88f + db * 0.12f;
            if (pass == 0)
            {
                peak[i] = juce::jmax (db, peak[i] - 0.25f);
                frameP[i] = (float) (nb > 0 ? pw / nb * norm * norm : 0.0);   // potenza media nella banda (per lo spettro medio)
            }
        }
    }
    // spettro medio a lungo termine del mid (come il pezzo di riferimento): solo quando c'e musica.
    // Media "scorrevole" di circa 20 secondi, cosi segue il pezzo ma non salta.
    if (P.lufsS.load() > -50 && ms)
    {
        const int cap = 600;
        if (ltasN >= cap) for (int i = 0; i < kPts; ++i) ltasPow[i] *= (cap - 1.0) / cap; else ++ltasN;
        for (int i = 0; i < kPts; ++i) ltasPow[i] += frameP[i];
    }
    // colonna dello spettrogramma
    if (! sgram.isNull())
    {
        const int H = sgram.getHeight();
        for (int y = 0; y < H; ++y)
        {
            const int i = juce::jlimit (0, kPts - 1, (int) ((1.0f - (y + 0.5f) / H) * kPts));
            const float t = juce::jlimit (0.0f, 1.0f, (specA[i] + 80.0f) / 76.0f);
            juce::Colour c = t < 0.33f ? juce::Colour (0xff050a18).interpolatedWith (juce::Colour (0xff3a1c71), t / 0.33f)
                           : t < 0.66f ? juce::Colour (0xff3a1c71).interpolatedWith (juce::Colour (0xffd7266b), (t - 0.33f) / 0.33f)
                                       : juce::Colour (0xffd7266b).interpolatedWith (juce::Colour (0xffffe08a), (t - 0.66f) / 0.34f);
            sgram.setPixelAt (sgX, y, c);
        }
        sgX = (sgX + 1) % sgram.getWidth();
    }
}

// curva di riferimento allineata alla mia (stessa media fra 100 Hz e 8 kHz, cosi conta la forma e non il volume)
bool SpecEq::refCurve (float* out) const
{
    const int pr = (int) ed.proc.value ("profile");
    if (pr >= kGenres)
    {
        const auto R = ed.proc.reference(); if (! R.ok || (int) R.spec.size() != kPts) return false;
        for (int i = 0; i < kPts; ++i) out[i] = R.spec[(size_t) i];
    }
    else for (int i = 0; i < kPts; ++i) out[i] = profileAt (pr, ptF (i));
    double mr = 0, mm = 0; int n = 0;
    for (int i = 0; i < kPts; ++i) { const float f = ptF (i); if (f < 100 || f > 8000) continue; mr += out[i]; mm += hasLtas() ? ltasDb (i) : -30.0f; ++n; }
    const float off = (float) ((mm - mr) / juce::jmax (1, n));
    for (int i = 0; i < kPts; ++i) out[i] += off;
    return true;
}
void SpecEq::bandDeviation (float* dev) const
{
    float ref[kPts]; const bool ok = refCurve (ref);
    for (int k = 0; k < kBal; ++k)
    {
        double a = 0, b = 0; int n = 0;
        for (int i = 0; i < kPts; ++i) { const float f = ptF (i); if (f < bandEdge (k) || f >= bandEdge (k + 1)) continue; a += ltasDb (i); b += ok ? ref[i] : 0; ++n; }
        dev[k] = n > 0 && ok ? (float) ((a - b) / n) : 0.0f;
    }
}

int SpecEq::snapPeak (float x) const
{
    auto p = plot(); const int c = juce::jlimit (0, kPts - 1, (int) ((x - p.getX()) / p.getWidth() * kPts));
    int best = c; for (int i = juce::jmax (0, c - 6); i <= juce::jmin (kPts - 1, c + 6); ++i) if (specA[i] > specA[best]) best = i;
    return best;
}
static juce::String noteOf (float f) { static const char* n[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }; const int m = juce::roundToInt (69 + 12 * std::log2 (f / 440.0f)); return juce::String (n[((m % 12) + 12) % 12]) + juce::String (m / 12 - 1); }

int SpecEq::nodeAt (juce::Point<float> pt) const
{
    if (view() != 0) return -1;
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
    auto& P = ed.proc;
    auto b = getLocalBounds().toFloat().reduced (1);
    jlook::card (g, b, 14, C::phos, true, false, false);
    const int v = view();
    // schede delle viste
    float x = 14;
    for (int i = 0; i < 5; ++i)
    {
        const float w = i == 4 ? 118.0f : i == 1 ? 96.0f : 70.0f;
        tabR[i] = { x, 7, w, 24 }; x += w + 4;
        jlook::button (g, tabR[i], C::phos, v == i, false, kViewNames[i], uf (10.5f));
    }
    freezeR = scR = dynScR = clearR = loadR = resetAvgR = {};
    for (auto& r : imTabR) r = {};
    const bool frozen = P.frozen.load(), hasSc = P.hasSc.load();
    if (v == 0)
    {
        freezeR = { b.getRight() - 336, 7, 84, 24 }; scR = { b.getRight() - 248, 7, 104, 24 }; dynScR = { b.getRight() - 140, 7, 124, 24 };
        jlook::button (g, freezeR, C::amber, frozen, false, frozen && ! P.freezeBtn.load() ? "FERMO" : "FREEZE", uf (10.0f));
        jlook::button (g, scR, C::amber, P.value ("scShow") > 0.5f, false, hasSc ? "CURVA SIDECHAIN" : "SC NON COLLEG.", uf (9.5f));
        jlook::button (g, dynScR, C::magenta, P.value ("dynSc") > 0.5f, false, "EQ DIN. DA SC", uf (9.5f));
    }
    else if (v == 1)
    {
        resetAvgR = { b.getRight() - 236, 7, 96, 24 }; loadR = { b.getRight() - 136, 7, 120, 24 };
        jlook::button (g, resetAvgR, C::magenta, false, false, "RIPARTI MEDIA", uf (9.5f));
        jlook::button (g, loadR, C::amber, (int) P.value ("profile") >= kGenres, false, "CARICA PEZZO...", uf (9.5f));
    }
    else if (v == 2)
    {
        static const char* n[3] = { "LISSAJOUS", "POLAR SAMPLE", "POLAR LEVEL" }; const int iv = (int) P.value ("imView");
        float xx = b.getRight() - 16;
        for (int i = 2; i >= 0; --i) { const float w = i == 0 ? 84.0f : 100.0f; xx -= w; imTabR[i] = { xx, 7, w, 24 }; xx -= 4; jlook::button (g, imTabR[i], C::violet, iv == i, false, n[i], uf (9.5f)); }
    }
    else if (v >= 3)
    {
        freezeR = { b.getRight() - 100, 7, 84, 24 };
        jlook::button (g, freezeR, C::amber, frozen, false, frozen && ! P.freezeBtn.load() ? "FERMO" : "FREEZE", uf (10.0f));
    }
    auto p = plot();
    lcd (g, p, 6);
    switch (v)
    {
        case 1: paintRef (g, p); break;
        case 2: paintImager (g, p); break;
        case 3: paintPeak (g, p); break;
        case 4: paintGram (g, p); break;
        default: paintEq (g, p); break;
    }
    if (frozen && v != 2) { g.setColour (C::amber.withAlpha (0.8f)); g.setFont (uf (10.0f)); g.drawText (P.freezeBtn.load() ? "CONGELATO" : "TRASPORTO FERMO", juce::Rectangle<float> (p.getRight() - 230, p.getY() + 20, 220, 14), juce::Justification::centredRight); }
    scan (g, p);
}

static void freqGrid (juce::Graphics& g, const SpecEq& s, juce::Rectangle<float> p)
{
    for (float f : { 30.f, 50.f, 100.f, 200.f, 500.f, 1000.f, 2000.f, 5000.f, 10000.f })
    {
        const float x = s.xOf (f); g.setColour (C::phos.withAlpha (f == 100 || f == 1000 || f == 10000 ? 0.12f : 0.05f)); g.drawVerticalLine ((int) x, p.getY(), p.getBottom());
        g.setColour (C::faint); g.setFont (mf (8.5f)); g.drawText (f >= 1000 ? juce::String ((int) f / 1000) + "k" : juce::String ((int) f), juce::Rectangle<float> (x - 15, p.getBottom() + 2, 30, 12), juce::Justification::centred);
    }
}

void SpecEq::paintEq (juce::Graphics& g, juce::Rectangle<float> p)
{
    auto& P = ed.proc; const double sr = P.getSr(); const bool frozen = P.frozen.load(), hasSc = P.hasSc.load();
    freqGrid (g, *this, p);
    for (int k = 1; k < kBal; ++k) { const float x = xOf (bandEdge (k)); g.setColour (C::amber.withAlpha (0.08f)); g.drawVerticalLine ((int) x, p.getY(), p.getBottom()); }
    for (int db : { -12, -6, 6, 12 }) { g.setColour (C::phos.withAlpha (0.05f)); g.drawHorizontalLine ((int) yOf ((float) db), p.getX(), p.getRight()); g.setColour (C::faint); g.setFont (mf (8.5f)); g.drawText ((db > 0 ? "+" : "") + juce::String (db), juce::Rectangle<float> (p.getRight() - 28, yOf ((float) db) - 11, 26, 10), juce::Justification::centredRight); }
    g.setColour (C::phos.withAlpha (0.12f)); g.drawHorizontalLine ((int) yOf (0), p.getX(), p.getRight());
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
    g.setColour (neo::K::hi.withAlpha (0.25f)); g.strokePath (path (peak, false), juce::PathStrokeType (0.8f));
    if (hasSc && P.value ("scShow") > 0.5f) { g.setColour (juce::Colour (0xffff8a3d).withAlpha (0.25f)); g.strokePath (path (specC, false), juce::PathStrokeType (3.5f)); g.setColour (juce::Colour (0xffff8a3d)); g.strokePath (path (specC, false), juce::PathStrokeType (1.3f)); }
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
        const bool cut = (int) ed.bp (k, "type") >= LCUT, foc = k == ed.selBand || k == popBand;
        g.setColour (C::band[k].withAlpha ((foc ? 0.18f : 0.08f) * (cut ? 0.4f : 1.0f))); g.fillPath (bpth);
    }
    g.setColour ((eqOn ? juce::Colours::white : C::dim).withAlpha (0.35f)); g.strokePath (tot, juce::PathStrokeType (4.0f));
    g.setColour (eqOn ? juce::Colours::white : C::dim); g.strokePath (tot, juce::PathStrokeType (1.6f));
    g.restoreState();
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
        const bool sel = k == ed.selBand || k == popBand, hov = k == hoverBand, solo = P.soloBand.load() == k;
        g.setColour (C::band[k].withAlpha (sel || hov ? 0.4f : 0.2f)); g.fillEllipse (x - 13, y - 13, 26, 26);
        g.setColour (juce::Colours::black.withAlpha (0.7f)); g.fillEllipse (x - 9, y - 9, 18, 18);
        g.setColour (C::band[k]); g.drawEllipse (x - 9, y - 9, 18, 18, sel ? 2.2f : 1.4f);
        g.setFont (mf (10.0f)); g.drawText (juce::String (k + 1), juce::Rectangle<float> (x - 9, y - 9, 18, 18), juce::Justification::centred);
        if (solo) { g.setColour (C::amber); g.setFont (uf (8.5f)); g.drawText ("SOLO", juce::Rectangle<float> (x + 10, y - 22, 40, 11), juce::Justification::centredLeft); }
    }
    for (auto& pn : pins)
    {
        const float x = xOf (pn.f), y = ySpec (pn.db + tilt (pn.f));
        g.setColour (C::amber.withAlpha (0.35f)); g.drawVerticalLine ((int) x, y, p.getBottom());
        g.setColour (C::amber); g.fillEllipse (x - 4, y - 4, 8, 8);
        jlook::valueBubble (g, { x - 62, y - 26, 124, 18 }, fmtF (pn.f) + "  " + noteOf (pn.f) + "  " + juce::String (pn.db, 1), C::amber);
    }
    if (! pins.empty()) { clearR = { p.getRight() - 80, p.getBottom() - 26, 72, 20 }; jlook::button (g, clearR, C::red, false, false, "TOGLI PIN", uf (9.0f)); }
    g.setFont (mf (8.5f)); g.setColour (C::dim);
    g.drawText (P.value ("spec") < 0.5f ? "ciano = mid   magenta = side   bianco = picchi   arancio = sidechain" : "ciano = sinistro   magenta = destro   bianco = picchi", juce::Rectangle<float> (p.getX() + 8, p.getBottom() - 16, p.getWidth() * 0.7f, 12), juce::Justification::centredLeft);
    if (hoverBand < 0 && popBand < 0 && hoverF > 0 && hoverPt.x > 0)
    {
        const int i = snapPeak (hoverPt.x); const float f = ptF (i), db = specA[i] - tilt (f), x = xOf (f), y = ySpec (specA[i]);
        g.setColour (C::text.withAlpha (0.5f)); g.drawEllipse (x - 6, y - 6, 12, 12, 1.4f);
        g.setColour (C::dim); g.setFont (mf (9.5f));
        g.drawText (fmtF (f) + "  " + noteOf (f) + "  " + juce::String (db, 1) + " dB   " + (frozen ? juce::String ("clic = segna il punto, doppio clic sul punto = banda EQ") : juce::String::fromUTF8 ("clic = nuova banda  \xc2\xb7  passa sopra una banda: SOLO, DINAMICA...")),
                    juce::Rectangle<float> (p.getX() + 8, p.getY() + 6, p.getWidth() - 16, 12), juce::Justification::centredLeft);
    }
}

void SpecEq::paintRef (juce::Graphics& g, juce::Rectangle<float> p)
{
    auto& P = ed.proc; freqGrid (g, *this, p);
    const int pr = (int) P.value ("profile"); const auto R = P.reference();
    float ref[kPts]; const bool ok = refCurve (ref);
    // scala: +-15 dB intorno alla media
    double mm = 0; int n = 0; for (int i = 0; i < kPts; ++i) { const float f = ptF (i); if (f >= 100 && f <= 8000) { mm += hasLtas() ? ltasDb (i) : (ok ? ref[i] : 0); ++n; } }
    const float mid = (float) (mm / juce::jmax (1, n));
    auto yR = [&p, mid] (float db) { return p.getCentreY() - (db - mid) / 18.0f * p.getHeight() * 0.5f; };
    for (int d : { -12, -6, 0, 6, 12 }) { g.setColour (C::phos.withAlpha (d == 0 ? 0.12f : 0.05f)); g.drawHorizontalLine ((int) yR (mid + d), p.getX(), p.getRight()); g.setColour (C::faint); g.setFont (mf (8.5f)); g.drawText ((d > 0 ? "+" : "") + juce::String (d), juce::Rectangle<float> (p.getRight() - 28, yR (mid + d) - 11, 26, 10), juce::Justification::centredRight); }
    for (int k = 1; k < kBal; ++k) { const float x = xOf (bandEdge (k)); g.setColour (C::amber.withAlpha (0.1f)); g.drawVerticalLine ((int) x, p.getY(), p.getBottom()); }
    g.saveState(); g.reduceClipRegion (p.toNearestInt());
    auto curve = [&] (std::function<float (int)> f) { juce::Path pt; for (int i = 0; i < kPts; ++i) { const float x = p.getX() + p.getWidth() * (i + 0.5f) / kPts, y = yR (f (i)); if (i == 0) pt.startNewSubPath (x, y); else pt.lineTo (x, y); } return pt; };
    // spettro medio leggermente smussato (1/6 di ottava) per leggere la forma
    float mine[kPts]; for (int i = 0; i < kPts; ++i) { double s = 0; int c = 0; for (int j = juce::jmax (0, i - 4); j <= juce::jmin (kPts - 1, i + 4); ++j) { s += ltasDb (j); ++c; } mine[i] = (float) (s / c); }
    if (ok) { float tmp[kPts]; for (int i = 0; i < kPts; ++i) { double s = 0; int c = 0; for (int j = juce::jmax (0, i - 4); j <= juce::jmin (kPts - 1, i + 4); ++j) { s += ref[j]; ++c; } tmp[i] = (float) (s / c); } std::copy (tmp, tmp + kPts, ref); }
    if (ok && hasLtas())
    {   // zona fra le due curve: verde dove hai meno del riferimento, rosso dove hai di piu
        for (int i = 0; i < kPts - 1; ++i)
        {
            const float x0 = p.getX() + p.getWidth() * (i + 0.5f) / kPts, x1 = p.getX() + p.getWidth() * (i + 1.5f) / kPts;
            const float d = mine[i] - ref[i]; if (std::abs (d) < 0.5f) continue;
            g.setColour ((d > 0 ? C::red : C::phos).withAlpha (juce::jmin (0.35f, std::abs (d) * 0.05f)));
            g.fillRect (juce::Rectangle<float>::leftTopRightBottom (x0, juce::jmin (yR (mine[i]), yR (ref[i])), x1, juce::jmax (yR (mine[i]), yR (ref[i]))));
        }
    }
    if (ok) { g.setColour (C::amber.withAlpha (0.3f)); g.strokePath (curve ([&ref] (int i) { return ref[i]; }), juce::PathStrokeType (4.0f)); g.setColour (C::amber); g.strokePath (curve ([&ref] (int i) { return ref[i]; }), juce::PathStrokeType (1.6f)); }
    if (hasLtas()) { g.setColour (C::phos); g.strokePath (curve ([&mine] (int i) { return mine[i]; }), juce::PathStrokeType (1.8f)); }
    g.restoreState();
    // legenda e scarti per banda
    juce::String refName = pr >= kGenres ? (R.ok ? juce::String ("PEZZO: ") + R.name : P.refLoading.load() ? juce::String ("analisi del pezzo in corso...") : juce::String ("nessun pezzo caricato: premi CARICA PEZZO")) : juce::String ("GENERE: ") + profile (pr).name + "  (curva indicativa)";
    g.setFont (uf (10.0f)); g.setColour (C::amber); g.drawText (refName, juce::Rectangle<float> (p.getX() + 10, p.getY() + 6, p.getWidth() * 0.62f, 14), juce::Justification::centredLeft);
    g.setColour (C::phos); g.drawText (hasLtas() ? "ciano = il tuo mix (media ultimi ~20 s)" : "premi play: sto misurando il tuo mix...", juce::Rectangle<float> (p.getX() + 10, p.getY() + 22, 400, 14), juce::Justification::centredLeft);
    if (ok && hasLtas())
    {
        float dev[kBal]; bandDeviation (dev);
        for (int k = 0; k < kBal; ++k)
        {
            const float x0 = xOf (bandEdge (k)), x1 = xOf (bandEdge (k + 1)); const float d = dev[k];
            const auto c = std::abs (d) <= 1.5f ? C::green : std::abs (d) <= 3.5f ? C::amber : C::red;
            auto r = juce::Rectangle<float> (x0 + 3, p.getBottom() - 34, x1 - x0 - 6, 30);
            g.setColour (juce::Colours::black.withAlpha (0.55f)); g.fillRoundedRectangle (r, 4);
            g.setFont (uf (8.0f)); g.setColour (C::dim); g.drawText (bandName (k), r.withHeight (12).translated (0, 2), juce::Justification::centred);
            glowText (g, (d > 0 ? "+" : "") + juce::String (d, 1) + " dB", r.withTrimmedTop (13), c, mf (10.5f), juce::Justification::centred);
        }
    }
}

void SpecEq::paintImager (juce::Graphics& g, juce::Rectangle<float> p)
{
    auto& P = ed.proc; const int iv = (int) P.value ("imView");
    const float S = juce::jmin (p.getWidth() * 0.62f, p.getHeight() - 10);
    auto area = juce::Rectangle<float> (p.getX() + 12, p.getY() + 5, iv == 0 ? S : S * 2.0f * 0.98f, S);
    if (iv != 0) { const float R = juce::jmin (p.getWidth() * 0.33f, p.getHeight() - 40); area = juce::Rectangle<float> (p.getX() + 12, p.getY() + (p.getHeight() - R - 6) * 0.5f + 10, R * 2 + 8, R + 6); }
    const int iw = (int) area.getWidth(), ih = (int) area.getHeight();
    if (persist.isNull() || persist.getWidth() != iw || persist.getHeight() != ih) persist = juce::Image (juce::Image::ARGB, juce::jmax (1, iw), juce::jmax (1, ih), true);
    const int w = P.ringW.load(), n = 4096;
    double e = 0; for (int i = 0; i < n; ++i) { const int k = (w - n + i) & (Processor::kRing - 1); e += P.ringL[k] * P.ringL[k] + P.ringR[k] * P.ringR[k]; }
    float pkI = 1e-4f; for (int i = 0; i < n; ++i) { const int k = (w - n + i) & (Processor::kRing - 1); pkI = juce::jmax (pkI, std::abs (P.ringL[k]), std::abs (P.ringR[k])); }
    juce::ignoreUnused (e);
    const float auto_ = 0.45f / juce::jmax (0.02f, pkI);   // scala automatica: il picco arriva vicino al bordo
    {
        juce::Graphics pg (persist); persist.multiplyAllAlphas (iv == 2 ? 0.55f : 0.8f);
        if (iv == 0)
        {   // goniometro classico ruotato di 45 gradi: verticale = mono, orizzontale = side
            const float c0 = iw * 0.5f, sc = iw * auto_; pg.setColour (C::green.withAlpha (0.5f));
            for (int i = 0; i < n; i += 2) { const int k = (w - n + i) & (Processor::kRing - 1); const float l = P.ringL[k], r = P.ringR[k]; const float x = c0 + (r - l) * 0.707f * sc, y = c0 - (l + r) * 0.707f * sc; if (x > 0 && y > 0 && x < iw && y < ih) pg.fillRect (x, y, 1.4f, 1.4f); }
        }
        else
        {
            const float cx = iw * 0.5f, cy = ih - 3.0f, R = iw * 0.5f - 4; const float sc = R * auto_ * 1.8f;
            if (iv == 1)
            {   // polar sample: ogni campione e un punto, angolo = posizione nello stereo, distanza = ampiezza
                pg.setColour (C::phos.withAlpha (0.5f));
                for (int i = 0; i < n; i += 2)
                {
                    const int k = (w - n + i) & (Processor::kRing - 1); float M = (P.ringL[k] + P.ringR[k]) * 0.7071f, Sd = (P.ringL[k] - P.ringR[k]) * 0.7071f;
                    if (M < 0) { M = -M; Sd = -Sd; }
                    const float a = std::atan2 (Sd, M), rr = juce::jmin (R, std::sqrt (M * M + Sd * Sd) * sc);
                    if (std::abs (Sd) > M) pg.setColour (C::red.withAlpha (0.6f)); else pg.setColour (C::phos.withAlpha (0.5f));
                    pg.fillRect (cx - std::sin (a) * rr, cy - std::cos (a) * rr, 1.5f, 1.5f);
                }
            }
            else
            {   // polar level: per ogni direzione il livello massimo, come raggi
                constexpr int NA = 64; float lv[NA] {};
                for (int i = 0; i < n; ++i)
                {
                    const int k = (w - n + i) & (Processor::kRing - 1); float M = (P.ringL[k] + P.ringR[k]) * 0.7071f, Sd = (P.ringL[k] - P.ringR[k]) * 0.7071f;
                    if (M < 0) { M = -M; Sd = -Sd; }
                    const float a = std::atan2 (Sd, M); const int bi = juce::jlimit (0, NA - 1, (int) ((a / juce::MathConstants<float>::pi + 0.5f) * NA));
                    lv[bi] = juce::jmax (lv[bi], std::sqrt (M * M + Sd * Sd));
                }
                for (int bi = 0; bi < NA; ++bi)
                {
                    if (lv[bi] < 1e-4f) continue;
                    const float a = ((bi + 0.5f) / NA - 0.5f) * juce::MathConstants<float>::pi, rr = juce::jmin (R, lv[bi] * sc);
                    pg.setColour ((std::abs (a) > juce::MathConstants<float>::pi * 0.25f ? C::red : C::violet).withAlpha (0.85f));
                    pg.drawLine (cx, cy, cx - std::sin (a) * rr, cy - std::cos (a) * rr, 2.5f);
                }
            }
        }
    }
    // cornice e guide
    g.setColour (C::phos.withAlpha (0.12f));
    if (iv == 0)
    {
        g.drawLine (area.getCentreX(), area.getY() + 6, area.getCentreX(), area.getBottom() - 6); g.drawLine (area.getX() + 6, area.getCentreY(), area.getRight() - 6, area.getCentreY());
        g.drawLine (area.getX() + 20, area.getY() + 20, area.getRight() - 20, area.getBottom() - 20); g.drawLine (area.getRight() - 20, area.getY() + 20, area.getX() + 20, area.getBottom() - 20);
        g.setColour (C::dim); g.setFont (mf (8.5f));
        g.drawText ("M", juce::Rectangle<float> (area.getCentreX() - 8, area.getY() + 3, 16, 10), juce::Justification::centred);
        g.drawText ("L", juce::Rectangle<float> (area.getX() + 18, area.getY() + 10, 12, 10), juce::Justification::centred); g.drawText ("R", juce::Rectangle<float> (area.getRight() - 30, area.getY() + 10, 12, 10), juce::Justification::centred);
        g.drawText ("+S", juce::Rectangle<float> (area.getX() + 2, area.getCentreY() - 12, 20, 10), juce::Justification::centred); g.drawText ("-S", juce::Rectangle<float> (area.getRight() - 22, area.getCentreY() - 12, 20, 10), juce::Justification::centred);
    }
    else
    {
        const float cx = area.getCentreX(), cy = area.getBottom() - 3, R = area.getWidth() * 0.5f - 4;
        for (float rr : { R * 0.33f, R * 0.66f, R }) { juce::Path arc; arc.addCentredArc (cx, cy, rr, rr, 0, -juce::MathConstants<float>::halfPi, juce::MathConstants<float>::halfPi, true); g.strokePath (arc, juce::PathStrokeType (0.8f)); }
        for (float a : { -1.5708f, -0.785f, 0.0f, 0.785f, 1.5708f }) { g.setColour ((std::abs (a) > 1 ? C::red : C::phos).withAlpha (0.14f)); g.drawLine (cx, cy, cx + std::sin (a) * R, cy - std::cos (a) * R, 0.8f); }
        g.setColour (C::dim); g.setFont (mf (8.5f));
        g.drawText ("L", juce::Rectangle<float> (cx - R * 0.72f - 14, cy - R * 0.72f - 12, 12, 12), juce::Justification::centred);
        g.drawText ("R", juce::Rectangle<float> (cx + R * 0.72f + 2, cy - R * 0.72f - 12, 12, 12), juce::Justification::centred);
        g.drawText ("MONO", juce::Rectangle<float> (cx - 20, cy - R - 13, 40, 11), juce::Justification::centred);
        g.drawText ("FUORI FASE", juce::Rectangle<float> (area.getX(), cy - 14, 70, 11), juce::Justification::centredLeft); g.drawText ("FUORI FASE", juce::Rectangle<float> (area.getRight() - 70, cy - 14, 70, 11), juce::Justification::centredRight);
    }
    g.drawImageAt (persist, (int) area.getX(), (int) area.getY());
    // a destra: numeri
    auto info = juce::Rectangle<float> (area.getRight() + 18, p.getY() + 10, p.getRight() - area.getRight() - 30, p.getHeight() - 20);
    auto row = [&] (const juce::String& l, const juce::String& v, juce::Colour c) { g.setFont (uf (9.5f)); g.setColour (C::dim); g.drawText (l, info.removeFromTop (14), juce::Justification::centredLeft); glowText (g, v, info.removeFromTop (22), c, mf (15.0f)); info.removeFromTop (6); };
    const float co = P.corr.load();
    row ("CORRELAZIONE", juce::String (co, 2), co < 0 ? C::red : co < 0.3f ? C::amber : C::green);
    row ("LARGHEZZA S/M", juce::String (juce::roundToInt (P.width.load() * 100)) + "%", C::violet);
    const float bal = P.balance.load();
    row ("BILANCIO L/R", std::abs (bal) < 0.25f ? juce::String ("centro") : juce::String (std::abs (bal), 1) + " dB " + (bal > 0 ? "R" : "L"), C::phos);
    const char* bn[3] = { "BASSI", "MEDI", "ALTI" };
    for (int k = 0; k < 3; ++k) row (juce::String (bn[k]) + "  corr / largh.", juce::String (P.corrB[k].load(), 2) + "  " + juce::String (juce::roundToInt (P.widthB[k].load() * 100)) + "%", P.corrB[k].load() < (k == 0 ? 0.6f : k == 1 ? 0.2f : -0.15f) ? C::amber : C::text);
    static const char* tip[3] = { "Lissajous: verticale = mono, la forma larga = suono largo, orizzontale = fuori fase.",
                                  "Polar sample: ogni punto e un campione. Dentro il ventaglio centrale e compatibile mono; i punti rossi oltre L/R sono fuori fase.",
                                  "Polar level: un raggio per direzione con il livello massimo. Raggi rossi = energia fuori fase." };
    g.setFont (uf (9.0f, false)); g.setColour (C::dim); g.drawFittedText (tip[iv], info.toNearestInt(), juce::Justification::topLeft, 4);
}

void SpecEq::paintPeak (juce::Graphics& g, juce::Rectangle<float> p)
{
    auto& P = ed.proc; const auto& T = target ((int) P.value ("target"));
    // scala in dBFS a destra; onda specchiata (picco chiaro, RMS pieno); linee del picco consigliato e di 0 dB
    auto yD = [&p] (float lin, bool up) { const float h = p.getHeight() * 0.5f - 22; return p.getCentreY() + (up ? -1.0f : 1.0f) * juce::jlimit (0.0f, 1.05f, lin) * h; };   // ampiezza lineare, come l'onda nella DAW
    for (int d : { -18, -12, -6, -3, 0 })
    {
        const float lin = juce::Decibels::decibelsToGain ((float) d);
        g.setColour (C::phos.withAlpha (d == 0 ? 0.2f : 0.05f)); g.drawHorizontalLine ((int) yD (lin, true), p.getX(), p.getRight() - 30); g.drawHorizontalLine ((int) yD (lin, false), p.getX(), p.getRight() - 30);
        g.setColour (C::faint); g.setFont (mf (8.0f)); g.drawText (juce::String (d), juce::Rectangle<float> (p.getRight() - 28, yD (lin, true) - 5, 26, 10), juce::Justification::centredRight);
    }
    const float tpLin = juce::Decibels::decibelsToGain (T.tp);
    g.setColour (C::red.withAlpha (0.6f)); g.drawHorizontalLine ((int) yD (tpLin, true), p.getX(), p.getRight() - 30); g.drawHorizontalLine ((int) yD (tpLin, false), p.getX(), p.getRight() - 30);
    g.setFont (uf (8.5f)); g.drawText ("PICCO MAX CONSIGLIATO " + juce::String (T.tp, 1) + " dB (" + T.name + ")", juce::Rectangle<float> (p.getX() + 8, yD (tpLin, true) + 2, 300, 11), juce::Justification::centredLeft);
    const int NH = Processor::kPH, w0 = P.phW.load(); const float W = p.getWidth() - 34; int overs = 0; float mx = 0;
    for (int x = 0; x < (int) W; ++x)
    {
        const int a = (int) ((float) x / W * NH), b = juce::jmax (a + 1, (int) ((float) (x + 1) / W * NH));
        float pk = 0, rm = 0; for (int k = a; k < b; ++k) { const int i = (w0 + k) % NH; pk = juce::jmax (pk, P.phMax[i].load()); rm = juce::jmax (rm, P.phRms[i].load()); }
        mx = juce::jmax (mx, pk);
        if (pk < 1e-4f) continue;
        const bool over = pk > tpLin; overs += over ? 1 : 0;
        g.setColour ((over ? C::red : C::phos).withAlpha (0.4f)); g.drawVerticalLine ((int) p.getX() + x, yD (pk, true), yD (pk, false));
        g.setColour (C::phos.withAlpha (0.9f)); g.drawVerticalLine ((int) p.getX() + x, yD (rm, true), yD (rm, false));
    }
    g.setFont (uf (10.0f)); g.setColour (C::text);
    g.drawText (juce::String::fromUTF8 ("ultimi 20 s  \xc2\xb7  onda = picchi  \xc2\xb7  dentro = RMS  \xc2\xb7  rosso = sopra il picco consigliato"), juce::Rectangle<float> (p.getX() + 10, p.getBottom() - 34, p.getWidth() - 60, 14), juce::Justification::centredLeft);
    const float pkDb = juce::Decibels::gainToDecibels (mx, -100.0f), lu = P.lufsS.load();
    juce::String sum = "picco nella finestra " + (mx > 0 ? juce::String (pkDb, 1) + " dB" : juce::String ("--")) + "   true peak max " + (P.tpMax.load() > -90 ? juce::String (P.tpMax.load(), 1) + " dBTP" : juce::String ("--"))
                     + "   picco - LUFS (PSR) " + (mx > 0 && lu > -69 ? juce::String (pkDb - lu, 1) + " dB" : juce::String ("--"));
    g.setColour (overs > 0 ? C::red : C::green); g.drawText (sum, juce::Rectangle<float> (p.getX() + 10, p.getBottom() - 18, p.getWidth() - 60, 14), juce::Justification::centredLeft);
}

void SpecEq::paintGram (juce::Graphics& g, juce::Rectangle<float> p)
{
    const int W = juce::jmax (8, (int) p.getWidth() - 34), H = juce::jmax (8, (int) p.getHeight() - 4);
    if (sgram.isNull() || sgram.getWidth() != W || sgram.getHeight() != H) { sgram = juce::Image (juce::Image::RGB, W, H, true); sgX = 0; }
    // scorre da destra a sinistra: la colonna piu nuova e sempre a destra
    const int x0 = (int) p.getX() + 2, y0 = (int) p.getY() + 2;
    g.drawImage (sgram, x0, y0, W - sgX, H, sgX, 0, W - sgX, H);
    if (sgX > 0) g.drawImage (sgram, x0 + W - sgX, y0, sgX, H, 0, 0, sgX, H);
    g.setFont (mf (8.0f));
    for (float f : { 50.f, 100.f, 200.f, 500.f, 1000.f, 2000.f, 5000.f, 10000.f })
    {
        const float y = y0 + H * (1.0f - std::log (f / 20.0f) / std::log (1000.0f));
        g.setColour (neo::K::hi.withAlpha (0.12f)); g.drawHorizontalLine ((int) y, (float) x0, (float) (x0 + W));
        g.setColour (C::dim); g.drawText (f >= 1000 ? juce::String ((int) f / 1000) + "k" : juce::String ((int) f), juce::Rectangle<float> ((float) x0 + W + 2, y - 5, 30, 10), juce::Justification::centredLeft);
    }
    g.setFont (uf (9.5f)); g.setColour (C::text.withAlpha (0.8f));
    g.drawText ("SPETTROGRAMMA: tempo da sinistra a destra, colore = energia. Utile per vedere risonanze fisse e code.", juce::Rectangle<float> ((float) x0 + 8, (float) y0 + 4, (float) W - 16, 14), juce::Justification::centredLeft);
}

// ----------------------------------------------------------------------- mouse
void SpecEq::mouseMove (const juce::MouseEvent& e)
{
    const int h = nodeAt (e.position); hoverF = plot().contains (e.position) ? fOf (e.position.x) : 0; hoverPt = e.position;
    hoverBand = h;
    if (h >= 0) { if (h != popBand) { popBand = h; updatePop(); } popHold = 60; }
    setMouseCursor (h >= 0 ? juce::MouseCursor::DraggingHandCursor : juce::MouseCursor::CrosshairCursor);
}
void SpecEq::mouseExit (const juce::MouseEvent&) { hoverPt = { -1, -1 }; hoverBand = -1; }
void SpecEq::updatePop()
{
    const int b = popBand;
    if (view() != 0 || b < 0 || ed.bp (b, "on") < 0.5f) { pop.setVisible (false); pop.band = -1; return; }
    pop.band = b; pop.setSize (std::abs (ed.bp (b, "dyn")) > 0.05f ? 470 : 380, 64);
    const int t = (int) ed.bp (b, "type");
    const juce::Point<float> n (xOf (ed.bp (b, "freq")), t >= LCUT ? yOf (0) : yOf (ed.bp (b, "gain")));
    int x = juce::jlimit (6, getWidth() - pop.getWidth() - 6, (int) n.x - pop.getWidth() / 2);
    int y = (int) n.y + 20; if (y + pop.getHeight() > getHeight() - 8) y = (int) n.y - 20 - pop.getHeight();
    pop.setTopLeftPosition (x, juce::jlimit (34, getHeight() - pop.getHeight() - 6, y));
    pop.setVisible (true); pop.repaint();
}
void SpecEq::bandMenu (int n)
{
    juce::PopupMenu m; auto* tp = dynamic_cast<juce::AudioParameterChoice*> (ed.bpar (n, "type"));
    m.addSectionHeader ("BANDA " + juce::String (n + 1));
    const bool solo = ed.proc.soloBand.load() == n;
    m.addItem (200, solo ? "Togli SOLO" : "SOLO (ascolta solo questa zona)");
    m.addItem (201, std::abs (ed.bp (n, "dyn")) > 0.05f ? "Dinamica: OFF" : "Dinamica: ON (taglia quando supera la soglia)");
    m.addSeparator();
    for (int i = 0; i < tp->choices.size(); ++i) m.addItem (i + 1, tp->choices[i], true, i == tp->getIndex());
    m.addSeparator(); m.addItem (100, "Rimuovi banda");
    juce::Component::SafePointer<SpecEq> sp (this);
    m.showMenuAsync (juce::PopupMenu::Options(), [sp, n, tp] (int r)
    {
        if (! sp) return; auto& ed = sp->ed;
        if (r == 100) { ed.setP (ed.bpar (n, "on"), 0); if (ed.proc.soloBand.load() == n) ed.proc.soloBand = -1; }
        else if (r == 200) ed.proc.soloBand = ed.proc.soloBand.load() == n ? -1 : n;
        else if (r == 201) ed.setP (ed.bpar (n, "dyn"), std::abs (ed.bp (n, "dyn")) > 0.05f ? 0.0f : -4.0f);
        else if (r > 0) tp->setValueNotifyingHost (tp->convertTo0to1 ((float) (r - 1)));
        sp->updatePop();
    });
}
void SpecEq::mouseDown (const juce::MouseEvent& e)
{
    auto& P = ed.proc;
    for (int i = 0; i < 5; ++i) if (tabR[i].contains (e.position)) { ed.setP (ed.P ("view"), (float) i); popBand = -1; updatePop(); repaint(); return; }
    for (int i = 0; i < 3; ++i) if (imTabR[i].contains (e.position)) { ed.setP (ed.P ("imView"), (float) i); persist = {}; return; }
    if (freezeR.contains (e.position)) { P.freezeBtn = ! P.freezeBtn.load(); repaint(); return; }
    if (scR.contains (e.position)) { ed.setP (ed.P ("scShow"), P.value ("scShow") > 0.5f ? 0.0f : 1.0f); return; }
    if (dynScR.contains (e.position)) { ed.setP (ed.P ("dynSc"), P.value ("dynSc") > 0.5f ? 0.0f : 1.0f); return; }
    if (resetAvgR.contains (e.position)) { resetAverage(); return; }
    if (loadR.contains (e.position)) { ed.refp.chooseFile(); return; }
    if (view() != 0) return;
    if (! pins.empty() && clearR.contains (e.position)) { pins.clear(); return; }
    for (size_t k = 0; k < pins.size(); ++k)
        if (juce::Point<float> (xOf (pins[k].f), ySpec (pins[k].db + tilt (pins[k].f))).getDistanceFrom (e.position) < 10)
        {
            if (e.mods.isPopupMenu()) pins.erase (pins.begin() + (long) k);
            return;
        }
    const int n = nodeAt (e.position);
    if (n >= 0)
    {
        ed.selBand = n; popBand = n; updatePop();
        if (e.mods.isPopupMenu()) { bandMenu (n); return; }
        dragBand = n; ed.bpar (n, "freq")->beginChangeGesture(); ed.bpar (n, "gain")->beginChangeGesture();
        return;
    }
    if (! plot().contains (e.position) || e.mods.isPopupMenu()) return;
    popBand = -1; updatePop();
    if (P.frozen.load())
    {   // spettro fermo: il clic segna il picco
        const int i = snapPeak (e.position.x); const float f = ptF (i);
        if (pins.size() >= 8) pins.erase (pins.begin());
        pins.push_back ({ f, specA[i] - tilt (f) });
        return;
    }
    for (int k = 0; k < kBands; ++k)
        if (ed.bp (k, "on") < 0.5f)
        {
            const float f = fOf (e.position.x);
            ed.setP (ed.bpar (k, "on"), 1); ed.setP (ed.bpar (k, "type"), f < 40 ? LCUT : f > 16000 ? HCUT : BELL);
            ed.setP (ed.bpar (k, "freq"), f); ed.setP (ed.bpar (k, "gain"), juce::jlimit (-18.0f, 18.0f, dbOf (e.position.y)));
            ed.selBand = k; popBand = k; dragBand = k; updatePop();
            ed.bpar (k, "freq")->beginChangeGesture(); ed.bpar (k, "gain")->beginChangeGesture();
            return;
        }
}
void SpecEq::mouseDrag (const juce::MouseEvent& e)
{
    if (dragBand < 0) return;
    ed.setP (ed.bpar (dragBand, "freq"), juce::jlimit (20.0f, 20000.0f, fOf (e.position.x)));
    if ((int) ed.bp (dragBand, "type") < LCUT) ed.setP (ed.bpar (dragBand, "gain"), juce::jlimit (-18.0f, 18.0f, dbOf (e.position.y)));
    updatePop();
}
void SpecEq::mouseUp (const juce::MouseEvent&)
{
    if (dragBand >= 0) { ed.bpar (dragBand, "freq")->endChangeGesture(); ed.bpar (dragBand, "gain")->endChangeGesture(); }
    dragBand = -1;
}
void SpecEq::mouseDoubleClick (const juce::MouseEvent& e)
{
    if (view() != 0) return;
    for (auto& pn : pins)
        if (juce::Point<float> (xOf (pn.f), ySpec (pn.db + tilt (pn.f))).getDistanceFrom (e.position) < 10)
        {   // doppio clic su un pin: banda a campana stretta che lo taglia di 3 dB
            for (int k = 0; k < kBands; ++k) if (ed.bp (k, "on") < 0.5f)
            {
                ed.setP (ed.bpar (k, "on"), 1); ed.setP (ed.bpar (k, "type"), BELL); ed.setP (ed.bpar (k, "freq"), pn.f); ed.setP (ed.bpar (k, "gain"), -3); ed.setP (ed.bpar (k, "q"), 4);
                ed.selBand = k; popBand = k; updatePop(); return;
            }
        }
    const int n = nodeAt (e.position); if (n >= 0) { ed.setP (ed.bpar (n, "on"), 0); if (ed.proc.soloBand.load() == n) ed.proc.soloBand = -1; popBand = -1; updatePop(); }
}
void SpecEq::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    const int n = nodeAt (e.position); if (n < 0) return;
    auto* q = ed.bpar (n, "q"); q->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, q->getValue() + (w.deltaY > 0 ? 0.03f : -0.03f))); updatePop();
}

// ----------------------------------------------------------------------- finestrella banda
enum { PI_TYPE = 1, PI_SOLO, PI_DYNON, PI_DEL, PI_F = 20, PI_G, PI_Q, PI_DYN, PI_THR };
static const char* popKey (int id) { switch (id) { case PI_F: return "freq"; case PI_G: return "gain"; case PI_Q: return "q"; case PI_DYN: return "dyn"; case PI_THR: return "thr"; default: return nullptr; } }
void BandPop::paint (juce::Graphics& g)
{
    if (band < 0) return;
    auto& ed = sp.ed; const int b = band; const auto c = C::band[b];
    auto r = getLocalBounds().toFloat();
    g.setColour (juce::Colours::black.withAlpha (0.55f)); g.fillRoundedRectangle (r.translated (0, 3), 8);
    g.setColour (juce::Colour (0xf2101318)); g.fillRoundedRectangle (r, 8); g.setColour (c.withAlpha (0.85f)); g.drawRoundedRectangle (r.reduced (0.5f), 8, 1.3f);
    hots.clear();
    auto btn = [&] (juce::Rectangle<float> rr, const juce::String& t, bool on, juce::Colour col, int id) { jlook::button (g, rr, col, on, false, t, uf (9.5f)); hots.push_back ({ rr, id }); };
    static const char* tn[] = { "BELL", "LOW SHELF", "HIGH SHELF", "LOW CUT", "HIGH CUT" };
    const int t = (int) ed.bp (b, "type"); const bool dynOn = std::abs (ed.bp (b, "dyn")) > 0.05f, solo = ed.proc.soloBand.load() == b;
    glowText (g, juce::String (b + 1), { 8, 6, 18, 22 }, c, mf (12.0f), juce::Justification::centred);
    btn ({ 28, 6, 104, 22 }, juce::String (tn[t]) + juce::String::fromUTF8 (" \xe2\x96\xbe"), true, c, PI_TYPE);
    btn ({ 136, 6, 56, 22 }, "SOLO", solo, C::amber, PI_SOLO);
    btn ({ 196, 6, 78, 22 }, "DINAMICA", dynOn, C::magenta, PI_DYNON);
    btn ({ r.getRight() - 30, 6, 22, 22 }, juce::String::fromUTF8 ("\xc3\x97"), false, C::red, PI_DEL);
    float x = 8; const float y = 34;
    auto val = [&] (float w, const juce::String& lab, const juce::String& s, bool en, int id)
    {
        auto rr = juce::Rectangle<float> (x, y, w, 24); x += w + 4;
        lcd (g, rr, 4); g.setColour (en ? c.withAlpha (0.45f) : C::faint); g.drawRoundedRectangle (rr, 4, 1);
        g.setFont (uf (7.5f)); g.setColour (C::dim); g.drawText (lab, rr.withTrimmedLeft (6).withHeight (10).translated (0, 1), juce::Justification::centredLeft);
        g.setFont (mf (10.0f)); g.setColour (en ? C::text : C::faint); g.drawText (s, rr.withTrimmedTop (9).withTrimmedRight (6), juce::Justification::centredRight);
        if (en) hots.push_back ({ rr, id });
    };
    const float gv = ed.bp (b, "gain"), dv = ed.bp (b, "dyn");
    val (96, "FREQ", fmtF (ed.bp (b, "freq")), true, PI_F);
    val (84, "GAIN", t >= LCUT ? juce::String ("--") : (gv > 0 ? "+" : "") + juce::String (gv, 1) + " dB", t < LCUT, PI_G);
    val (64, "Q", juce::String (ed.bp (b, "q"), 2), true, PI_Q);
    if (dynOn && t <= HSHELF)
    {
        val (100, "DINAMICA", (dv > 0 ? "+" : "") + juce::String (dv, 1) + " dB", true, PI_DYN);
        val (96, "SOGLIA", juce::String (ed.bp (b, "thr"), 1) + " dB", true, PI_THR);
    }
    else { g.setFont (uf (8.5f, false)); g.setColour (C::dim); g.drawText (juce::String::fromUTF8 ("trascina i valori  \xc2\xb7  doppio clic = default"), juce::Rectangle<float> (x + 4, y, r.getRight() - x - 10, 24), juce::Justification::centredLeft); }
}
int BandPop::hit (juce::Point<float> p) const { for (auto& h : hots) if (h.r.contains (p)) return h.id; return 0; }
void BandPop::mouseDown (const juce::MouseEvent& e)
{
    if (band < 0) return;
    auto& ed = sp.ed; const int b = band; const int id = hit (e.position); sp.popHold = 90;
    if (id == PI_TYPE)
    {
        juce::PopupMenu m; auto* tp = dynamic_cast<juce::AudioParameterChoice*> (ed.bpar (b, "type"));
        for (int i = 0; i < tp->choices.size(); ++i) m.addItem (i + 1, tp->choices[i], true, i == tp->getIndex());
        juce::Component::SafePointer<SpecEq> s (&sp);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [s, tp] (int r) { if (r > 0 && s) { tp->setValueNotifyingHost (tp->convertTo0to1 ((float) (r - 1))); s->updatePop(); } });
    }
    else if (id == PI_SOLO) ed.proc.soloBand = ed.proc.soloBand.load() == b ? -1 : b;
    else if (id == PI_DYNON) ed.setP (ed.bpar (b, "dyn"), std::abs (ed.bp (b, "dyn")) > 0.05f ? 0.0f : -4.0f);
    else if (id == PI_DEL) { ed.setP (ed.bpar (b, "on"), 0); if (ed.proc.soloBand.load() == b) ed.proc.soloBand = -1; sp.popBand = -1; }
    else if (auto* k = popKey (id))
    {
        auto* prm = ed.bpar (b, k); dragK = id; dragStart = prm->getValue(); prm->beginChangeGesture();
        if (e.getNumberOfClicks() > 1) prm->setValueNotifyingHost (prm->getDefaultValue());
    }
    sp.updatePop(); sp.repaint();
}
void BandPop::mouseDrag (const juce::MouseEvent& e)
{
    if (band < 0 || dragK < 0) return;
    if (auto* k = popKey (dragK)) { auto* prm = sp.ed.bpar (band, k); prm->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, dragStart - e.getDistanceFromDragStartY() / (e.mods.isShiftDown() ? 800.0f : 200.0f))); }
    sp.popHold = 90; sp.updatePop(); sp.repaint();
}
void BandPop::mouseUp (const juce::MouseEvent&) { if (band >= 0 && dragK >= 0) if (auto* k = popKey (dragK)) sp.ed.bpar (band, k)->endChangeGesture(); dragK = -1; }
void BandPop::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    if (band < 0) return;
    if (auto* k = popKey (hit (e.position))) { auto* prm = sp.ed.bpar (band, k); prm->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, prm->getValue() + (w.deltaY > 0 ? 0.01f : -0.01f))); }
    sp.popHold = 90; sp.updatePop(); sp.repaint();
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
            g.setColour (neo::K::hi.withAlpha (0.05f)); g.fillRoundedRectangle (r, 2);
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
// ======================= Stereo =======================
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
    auto meterH = [&] (float y, const char* name, float v, float lo, float hi, bool corrScale, juce::String val, float warnAt = 0.3f, float redAt = 0.0f)
    {
        g.setColour (C::dim); g.setFont (uf (9.5f)); g.drawText (name, juce::Rectangle<float> (rx, y, rw, 13), juce::Justification::centredLeft);
        g.setFont (mf (10.0f)); g.drawText (val, juce::Rectangle<float> (rx, y, rw, 13), juce::Justification::centredRight);
        auto r = juce::Rectangle<float> (rx, y + 15, rw, 12); lcd (g, r, 4);
        const float t = juce::jlimit (0.0f, 1.0f, (v - lo) / (hi - lo)), c0 = corrScale ? r.getCentreX() : r.getX();
        const auto col = corrScale ? (v < redAt ? C::red : v < warnAt ? C::amber : C::green) : (v > 1.2f ? C::amber : C::violet);
        const float x = r.getX() + t * r.getWidth();
        g.setColour (col.withAlpha (0.85f)); g.fillRoundedRectangle (juce::jmin (c0, x), r.getY() + 3, std::abs (x - c0) + 2, r.getHeight() - 6, 2);
        if (corrScale) { g.setColour (C::dim.withAlpha (0.5f)); g.drawVerticalLine ((int) r.getCentreX(), r.getY(), r.getBottom()); }
    };
    meterH (40, "CORRELAZIONE", P.corr.load(), -1, 1, true, juce::String (P.corr.load(), 2));
    meterH (76, "LARGHEZZA S/M", P.width.load(), 0, 1.5f, false, juce::String (juce::roundToInt (P.width.load() * 100)) + "%");
    const float bal = P.balance.load();
    meterH (112, "BILANCIO L/R", juce::jlimit (-6.0f, 6.0f, bal), -6, 6, true, (std::abs (bal) < 0.25f ? juce::String ("centro") : juce::String (std::abs (bal), 1) + " dB " + (bal > 0 ? "R" : "L")));
    // soglie diverse per banda: i bassi devono stare quasi mono, gli alti larghi (vicino a 0) sono normali
    const char* bn[3] = { "BASSI < 150 Hz", "MEDI", "ALTI > 2 kHz  (vicino a 0 e' normale)" };
    const float wa[3] = { 0.6f, 0.2f, -0.15f }, ra[3] = { 0.0f, -0.05f, -0.35f };
    for (int k = 0; k < 3; ++k) meterH (152 + k * 34.0f, bn[k], P.corrB[k].load(), -1, 1, true, juce::String (P.corrB[k].load(), 2) + "  " + juce::String (juce::roundToInt (P.widthB[k].load() * 100)) + "%", wa[k], ra[k]);
    juce::String warnT; juce::Colour wc = C::green;
    if (P.lufsS.load() < -60) { warnT = "in attesa di audio"; wc = C::dim; }
    else if (P.corr.load() < 0) { warnT = "Fase invertita: in mono il mix si svuota!"; wc = C::red; }
    else if (P.corrB[0].load() < 0.6f) { warnT = "Bassi troppo larghi: mettili in mono sotto 120-150 Hz"; wc = C::amber; }
    else if (P.corrB[2].load() < -0.15f) { warnT = "Alti in controfase: widener o Haas troppo spinti (hats, cori), prova in mono"; wc = C::amber; }
    else if (P.width.load() > 1.0f) { warnT = "Molto largo: controlla la compatibilita mono"; wc = C::amber; }
    else warnT = "Compatibile mono";
    glowText (g, warnT, { rx, b.getBottom() - 30, rw, 16 }, wc, uf (10.5f));
}

// ======================= Bilanciamento =======================
static juce::PopupMenu refMenu (int cur)
{
    juce::PopupMenu m; m.addSectionHeader ("CONFRONTA CON...");
    for (int i = 0; i < kGenres; ++i) m.addItem (i + 1, juce::String ("Genere: ") + profile (i).name, true, i == cur);
    m.addSeparator(); m.addItem (kGenres + 1, "Il pezzo caricato", true, cur == kGenres);
    return m;
}
void Balance::mouseDown (const juce::MouseEvent& e)
{
    if (! profR.contains (e.position)) return;
    auto* tp = ed.P ("profile");
    refMenu ((int) ed.proc.value ("profile")).showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [tp] (int r) { if (r > 0) tp->setValueNotifyingHost (tp->convertTo0to1 ((float) (r - 1))); });
}
void Balance::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (1);
    jlook::card (g, b, 14, C::green, true, false, false);
    glowText (g, "BILANCIAMENTO", { 18, 8, 120, 22 }, C::text, uf (12.0f));
    const int pr = (int) ed.proc.value ("profile");
    profR = { b.getRight() - 112, 8, 96, 24 };
    jlook::button (g, profR, C::green, true, false, (pr >= kGenres ? juce::String ("PEZZO") : juce::String (profile (pr).name)) + juce::String::fromUTF8 (" \xe2\x96\xbe"), uf (9.5f));
    auto area = juce::Rectangle<float> (16, 42, b.getWidth() - 30, b.getHeight() - 58); lcd (g, area, 6);
    auto plotA = area.reduced (8, 24);
    float dev[kBal] {}; ed.spec.bandDeviation (dev);
    float ref[kPts]; const bool ok = ed.spec.refCurve (ref), live = ed.spec.hasLtas() && ok;
    auto yD = [&plotA] (float d) { return plotA.getCentreY() - juce::jlimit (-10.0f, 10.0f, d) / 10.0f * plotA.getHeight() * 0.5f; };
    g.setColour (C::green.withAlpha (0.08f)); g.drawHorizontalLine ((int) yD (0), plotA.getX(), plotA.getRight());
    const float cw = plotA.getWidth() / kBal;
    for (int k = 0; k < kBal; ++k)
    {
        const float d = dev[k];
        const float x = plotA.getX() + k * cw + cw * 0.22f, w = cw * 0.56f;
        g.setColour (C::green.withAlpha (0.10f)); g.fillRoundedRectangle (x - 3, yD (1.5f), w + 6, yD (-1.5f) - yD (1.5f), 3);   // zona ok
        const auto c = ! live ? C::faint : std::abs (d) <= 1.5f ? C::green : std::abs (d) <= 3.5f ? C::amber : C::red;
        if (live)
        {
            const float y0 = yD (0), y1 = yD (d);
            juce::ColourGradient bg2 (c.withAlpha (0.9f), 0, y1, c.withAlpha (0.25f), 0, y0, false); g.setGradientFill (bg2);
            g.fillRoundedRectangle (x, juce::jmin (y0, y1), w, juce::jmax (2.0f, std::abs (y1 - y0)), 3);
            glowText (g, (d > 0 ? "+" : "") + juce::String (d, 1), { x - 10, area.getY() + 4, w + 20, 14 }, c, mf (10.0f), juce::Justification::centred);
        }
        g.setColour (C::dim); g.setFont (uf (8.0f)); g.drawText (bandName (k), juce::Rectangle<float> (x - 16, area.getBottom() - 20, w + 32, 12), juce::Justification::centred);
    }
    g.setColour (C::faint); g.setFont (mf (8.0f));
    g.drawText (! ok ? "carica un pezzo di riferimento" : ! ed.spec.hasLtas() ? "premi play: misura in corso" : "+ = hai piu del riferimento", area.reduced (6, 2), juce::Justification::bottomLeft);
    scan (g, area);
}

// ======================= Riferimento =======================
void RefPanel::chooseFile()
{
    chooser = std::make_unique<juce::FileChooser> ("Scegli il pezzo di riferimento", juce::File::getSpecialLocation (juce::File::userMusicDirectory), "*.wav;*.aif;*.aiff;*.mp3;*.m4a;*.flac");
    juce::Component::SafePointer<RefPanel> sp (this);
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [sp] (const juce::FileChooser& c)
    {
        if (! sp) return; const auto f = c.getResult(); if (! f.existsAsFile()) return;
        sp->ed.proc.loadReference (f);
        sp->ed.setP (sp->ed.P ("profile"), (float) kGenres);
        sp->ed.setP (sp->ed.P ("view"), 1.0f);
    });
}
void RefPanel::mouseDown (const juce::MouseEvent& e)
{
    if (loadR.contains (e.position)) { chooseFile(); return; }
    if (clearR.contains (e.position)) { ed.proc.clearReference(); if ((int) ed.proc.value ("profile") >= kGenres) ed.setP (ed.P ("profile"), 0); return; }
    if (viewR.contains (e.position)) { ed.setP (ed.P ("view"), 1.0f); return; }
    if (genreR.contains (e.position))
    {
        auto* tp = ed.P ("profile");
        refMenu ((int) ed.proc.value ("profile")).showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [tp] (int r) { if (r > 0) tp->setValueNotifyingHost (tp->convertTo0to1 ((float) (r - 1))); });
    }
}
void RefPanel::paint (juce::Graphics& g)
{
    auto& P = ed.proc; auto b = getLocalBounds().toFloat().reduced (1);
    jlook::card (g, b, 14, C::amber, true, false, false);
    glowText (g, "RIFERIMENTO", { 18, 8, 160, 22 }, C::text, uf (12.0f));
    viewR = { b.getRight() - 110, 8, 94, 24 }; jlook::button (g, viewR, C::phos, (int) P.value ("view") == 1, false, "VEDI CONFRONTO", uf (9.0f));
    const int pr = (int) P.value ("profile"); const auto R = P.reference();
    genreR = { 16, 42, b.getWidth() - 30, 26 };
    jlook::button (g, genreR, C::green, true, false, juce::String ("CONFRONTA CON: ") + (pr >= kGenres ? juce::String ("PEZZO CARICATO") : juce::String (profile (pr).name)) + juce::String::fromUTF8 ("  \xe2\x96\xbe"), uf (10.0f));
    loadR = { 16, 76, b.getWidth() * 0.62f - 16, 26 }; clearR = { b.getWidth() * 0.62f + 6, 76, b.getWidth() * 0.38f - 22, 26 };
    jlook::button (g, loadR, C::amber, false, false, P.refLoading.load() ? "ANALISI IN CORSO..." : "CARICA UN PEZZO...", uf (10.0f));
    jlook::button (g, clearR, C::red, false, false, "TOGLI PEZZO", uf (9.5f));
    auto d = juce::Rectangle<float> (16, 110, b.getWidth() - 30, b.getHeight() - 124); lcd (g, d, 6);
    auto in = d.reduced (10, 8);
    g.setFont (uf (10.0f)); g.setColour (R.ok ? C::amber : C::dim);
    g.drawFittedText (R.ok ? R.name : R.error.isNotEmpty() ? "Errore: " + R.error : juce::String ("Nessun pezzo caricato. Carica un master che ti piace (WAV, AIFF, MP3): confronto spettro, loudness e picco."),
                      in.removeFromTop (30).toNearestInt(), juce::Justification::topLeft, 2);
    if (! R.ok) return;
    in.removeFromTop (4);
    auto rowH = [&] (const char* l, float refV, float mineV, const char* u, bool lowerBetter)
    {
        auto r = in.removeFromTop (34);
        g.setFont (uf (8.5f)); g.setColour (C::dim); g.drawText (l, r.withHeight (12), juce::Justification::centredLeft);
        auto v = r.withTrimmedTop (12);
        glowText (g, "RIF " + (refV > -90 ? juce::String (refV, 1) : juce::String ("--")) + u, v.removeFromLeft (v.getWidth() * 0.36f), C::amber, mf (11.0f));
        glowText (g, "TU " + (mineV > -90 ? juce::String (mineV, 1) : juce::String ("--")) + u, v.removeFromLeft (v.getWidth() * 0.55f), C::phos, mf (11.0f));
        if (refV > -90 && mineV > -90)
        {
            const float dd = mineV - refV; const auto c = std::abs (dd) <= 1 ? C::green : C::amber;
            glowText (g, (dd > 0 ? "+" : "") + juce::String (dd, 1), v, c, mf (11.0f), juce::Justification::centredRight);
        }
        juce::ignoreUnused (lowerBetter);
    };
    rowH ("LOUDNESS INTEGRATA", R.lufs, P.lufsI.load(), " LU", false);
    rowH ("PICCO", R.peak, P.pkMax.load(), " dB", true);
    rowH ("RMS MEDIO", R.rms, juce::jmax (P.rmsL.load(), P.rmsR.load()), " dB", false);
    g.setFont (uf (8.5f, false)); g.setColour (C::dim);
    g.drawFittedText ("Per confrontare bene metti questo plugin sul master e alza il volume finche la loudness e uguale a quella del riferimento.", in.toNearestInt(), juce::Justification::topLeft, 3);
}

// ======================= Editor =======================
Editor::Editor (Processor& p) : AudioProcessorEditor (&p), proc (p)
{
    skin::init ("meter", { 0xffa8bea0, 0xffcbcbc6, 0xff9fd0bf, 0xffe3d3a8 }, 0xff4dff88); C::sync(); themeRect = { 952, 15, 100, 36 };
    setLookAndFeel (&lnf);
    for (auto* c : std::initializer_list<juce::Component*> { &spec, &loud, &bal, &st, &refp }) addAndMakeVisible (c);
    addAndMakeVisible (sizeChip);
    scaler.setup();
    startTimerHz (30);
}
void Editor::resized()
{
    const float sc = scaler.update();
    specRect = { 470, 15, 220, 36 }; eqRect = { 700, 15, 120, 36 }; abRect = { 830, 15, 110, 36 };
    spec.setBounds (18, 80, 820, 510); loud.setBounds (850, 80, 372, 510);
    bal.setBounds (18, 602, 250, 284); st.setBounds (278, 602, 560, 284); refp.setBounds (850, 602, 372, 284);
    sizeChip.setBounds (W - 76, 24, 58, 15);
    jlook::scaleChildren (*this, sc);
}
void Editor::paint (juce::Graphics& g)
{
    skin::apply(); C::sync();
    g.addTransform (juce::AffineTransform::scale (scaler.s));
    const float Wf = (float) W, Hf = (float) H;
    jlook::backdrop (g, { 0, 0, Wf, Hf }, C::magenta, C::phos);
    jlook::header (g, { 0, 0, Wf, 64 }, "METERING HUB", C::green, "JM-1  ANALISI PRE / POST MASTER", C::magenta, C::phos);
    for (auto* c : std::initializer_list<juce::Component*> { &spec, &loud, &bal, &st, &refp }) jlook::softShadow (g, c->getBounds().toFloat(), 14);
    jlook::button (g, specRect.toFloat(), C::phos, true, false, proc.value ("spec") < 0.5f ? "SPETTRO: MID + SIDE" : "SPETTRO: L + R", uf (11.5f));
    jlook::button (g, eqRect.toFloat(), C::magenta, proc.value ("eqOn") > 0.5f, false, proc.value ("eqOn") > 0.5f ? "EQ ATTIVO" : "EQ BYPASS", uf (11.5f));
    jlook::button (g, abRect.toFloat(), C::phos, proc.ab.cur == 1, false, "MEMORIA " + proc.ab.label(), uf (11.5f));
    skin::chipDraw (g, themeRect.toFloat());
}
void Editor::mouseDown (const juce::MouseEvent& e0)
{
    { const auto tp = e0.position / scaler.s; if (themeRect.toFloat().contains (tp)) { skin::cycle(); C::sync(); for (auto* ch : getChildren()) ch->repaint(); repaint(); return; } }
    const auto e = e0.withNewPosition (e0.position / scaler.s);
    if (specRect.contains (e.getPosition())) setP (P ("spec"), proc.value ("spec") < 0.5f ? 1.0f : 0.0f);
    else if (eqRect.contains (e.getPosition())) setP (P ("eqOn"), proc.value ("eqOn") > 0.5f ? 0.0f : 1.0f);
    else if (abRect.contains (e.getPosition())) proc.ab.toggle (proc);
    repaint();
}
void Editor::timerCallback()
{
    spec.analyse();
    // la finestrella della banda resta qualche istante dopo che esci dal punto
    if (spec.popHold > 0 && --spec.popHold == 0 && ! spec.pop.isMouseOver (true) && spec.dragBand < 0) { spec.popBand = -1; spec.updatePop(); }
    spec.repaint(); loud.repaint(); st.repaint();
    if (++tick % 5 == 0) { bal.repaint(); refp.repaint(); }
    for (auto* k : { &st.width, &st.bal, &st.low, &st.mid, &st.high, &st.mono }) k->repaint();
}
}
