#include "MasterEditor.h"

namespace jms
{
// font e superfici della modalita FACILE anche nell'AVANZATO: niente LCD, niente scatole
juce::Font uf (float size, bool bold) { return bold ? neo::cap (size * 0.86f) : neo::f (size * 0.92f); }
juce::Font mf (float size) { return neo::num (size * 0.95f); }
static void btn (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c, bool on, bool hover, const juce::String& t, juce::Font fo) { neo::chip (g, r, t, on, c, juce::jlimit (7.5f, 10.0f, fo.getHeight() / 1.18f * 0.95f), hover); }

void glowText (juce::Graphics& g, const juce::String& t, juce::Rectangle<float> r, juce::Colour c, juce::Font f, juce::Justification j)
{
    g.setFont (f);
    for (int k = 2; k >= 1; --k) { g.setColour (c.withAlpha (0.12f)); g.drawText (t, r.translated ((float) -k * 0.6f, 0), j); g.drawText (t, r.translated ((float) k * 0.6f, 0), j); }
    g.setColour (c); g.drawText (t, r, j);
}
void lcd (juce::Graphics& g, juce::Rectangle<float> r, float rad)
{   // vetro scuro: un velo, un filo di luce in alto, nessuna griglia
    juce::ColourGradient cg (veil (0.42f), r.getX(), r.getY(), veil (0.28f), r.getX(), r.getBottom(), false);
    g.setGradientFill (cg); g.fillRoundedRectangle (r, rad);
    g.setColour (glass (0.045f)); g.drawRoundedRectangle (r.reduced (0.5f), rad, 1.0f);
    juce::ColourGradient hl (glass (0.0f), r.getX(), r.getY(), glass (0.0f), r.getRight(), r.getY(), false);
    hl.addColour (0.5, glass (0.09f)); g.setGradientFill (hl); g.fillRect (r.getX() + rad, r.getY(), r.getWidth() - 2 * rad, 1.0f);
}
static void scanlines (juce::Graphics&, juce::Rectangle<float>) {}
static juce::String dbS (float v, int dec = 1) { return (v > 0.05f ? "+" : "") + juce::String (v, dec); }
static juce::String lufsS (float v) { return v < -60 ? juce::String (juce::String::fromUTF8 ("\xe2\x80\x93")) : juce::String (v, 1); }

// ======================================================================= controlli
void Knob::mouseDown (const juce::MouseEvent& e)
{
    if (onTouch) onTouch();
    if (e.mods.isAltDown() || e.mods.isCommandDown()) { param->setValueNotifyingHost (param->getDefaultValue()); return; }
    start = param->getValue(); param->beginChangeGesture(); drag = true;
}
void Knob::mouseDrag (const juce::MouseEvent& e)
{
    if (! drag) return;
    param->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, start - e.getDistanceFromDragStartY() / (e.mods.isShiftDown() ? 900.0f : 220.0f)));
}
void Knob::paint (juce::Graphics& g)
{   // manopola sobria come nel FACILE: disco opaco, anello sottile, arco di un colore solo, tacca bianca
    const float lh = big ? 32.0f : 27.0f;
    const float size = juce::jmin ((float) getWidth(), getHeight() - lh);
    {
        const float cx = getWidth() * 0.5f, cy = size * 0.5f, R = size * 0.5f - 4.0f, tw = big ? 2.8f : 2.0f, kr = R * (big ? 0.7f : 0.74f);
        const float a0 = -2.356f, a1 = 2.356f, v = param->getValue(), av = a0 + (a1 - a0) * v, from = bipolar ? 0.0f : a0;
        const auto ac = col.withMultipliedSaturation (neo::light() ? 0.9f : 0.62f).withMultipliedBrightness (neo::light() ? 0.7f : 0.95f);
        { juce::Path t; t.addCentredArc (cx, cy, R, R, 0, a0, a1, true); g.setColour (glass (0.09f)); g.strokePath (t, juce::PathStrokeType (tw, juce::PathStrokeType::curved, juce::PathStrokeType::rounded)); }
        if (std::abs (av - from) > 0.01f)
        {
            juce::Path arc; arc.addCentredArc (cx, cy, R, R, 0, juce::jmin (from, av), juce::jmax (from, av), true);
            g.setColour (hover || drag ? ac.brighter (0.15f) : ac); g.strokePath (arc, juce::PathStrokeType (tw, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        if (activity > 0.01f)
        {
            const float ro = R + 5; juce::Path sg; sg.addCentredArc (cx, cy, ro, ro, 0, a0, a0 + (a1 - a0) * juce::jlimit (0.0f, 1.0f, activity), true);
            g.setColour (C::magenta.withAlpha (0.8f)); g.strokePath (sg, juce::PathStrokeType (1.6f));
        }
        g.setColour (juce::Colours::black.withAlpha (neo::light() ? 0.16f : 0.3f)); g.fillEllipse (cx - kr, cy - kr + 2, kr * 2, kr * 2);
        const juce::Colour top = neo::light() ? juce::Colour (0xff2c2a33) : juce::Colour (0xff35323d), bot = neo::light() ? juce::Colour (0xff17161b) : juce::Colour (0xff1c1a21);
        juce::ColourGradient bgc (top, cx, cy - kr, bot, cx, cy + kr, false); g.setGradientFill (bgc); g.fillEllipse (cx - kr, cy - kr, kr * 2, kr * 2);
        g.setColour (juce::Colours::white.withAlpha (hover || drag ? 0.12f : 0.07f)); g.drawEllipse (cx - kr + 0.5f, cy - kr + 0.5f, kr * 2 - 1, kr * 2 - 1, 1.0f);
        const float r0 = kr * 0.35f, r1 = kr * 0.86f, sn = std::sin (av), cs = std::cos (av);
        juce::Path pl; pl.startNewSubPath (cx + sn * r0, cy - cs * r0); pl.lineTo (cx + sn * r1, cy - cs * r1);
        g.setColour (juce::Colours::white.withAlpha (0.92f)); g.strokePath (pl, juce::PathStrokeType (big ? 2.2f : 1.7f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    g.setColour (hover ? C::text : C::dim); g.setFont (neo::cap (big ? 9.5f : 8.0f)); g.drawText (label, juce::Rectangle<float> (-10, size + 1, (float) getWidth() + 20, 14), juce::Justification::centred);
    g.setFont (neo::num (big ? 12.5f : 10.5f)); g.setColour (C::text.withAlpha (0.92f));
    g.drawText (text ? text() : param->getCurrentValueAsText(), juce::Rectangle<float> (-10, size + 15, (float) getWidth() + 20, 14), juce::Justification::centred);
}
void Segs::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat(); const auto col = neo::acc (this->col);
    const float rr = b.getHeight() * 0.5f;
    g.setColour (veil (0.38f)); g.fillRoundedRectangle (b, rr);
    g.setColour (glass (0.05f)); g.drawRoundedRectangle (b.reduced (0.5f), rr, 1.0f);
    const int n = juce::jmax (1, items.size()), cur = juce::roundToInt (param->convertFrom0to1 (param->getValue()));
    const float w = (b.getWidth() - 6) / n;
    for (int i = 0; i < n; ++i)
    {
        auto r = juce::Rectangle<float> (3 + i * w, 3, w, b.getHeight() - 6).reduced (1, 0);
        if (i == cur)
        {
            g.setColour (col.withAlpha (0.25f)); g.fillRoundedRectangle (r.expanded (1, 2), r.getHeight() * 0.5f + 2);
            juce::ColourGradient pg (col.brighter (0.25f), r.getX(), r.getY(), col.darker (0.35f), r.getX(), r.getBottom(), false); g.setGradientFill (pg); g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
            g.setColour (juce::Colours::white); g.setFont (neo::cap (font * 0.8f)); g.drawText (items[i], r, juce::Justification::centred);
        }
        else { g.setColour (C::dim); g.setFont (neo::cap (font * 0.8f)); g.drawText (items[i], r, juce::Justification::centred); }
    }
}
void Segs::mouseDown (const juce::MouseEvent& e)
{
    const int n = juce::jmax (1, items.size()); const int k = juce::jlimit (0, n - 1, (int) ((e.position.x - 3) / ((getWidth() - 6) / (float) n)));
    param->setValueNotifyingHost (param->convertTo0to1 ((float) k)); repaint();
}
void Switch::paint (juce::Graphics& g)
{
    const bool on = param->getValue() > 0.5f; const float h = (float) juce::jmin (getHeight(), 20), w = h * 1.8f; const auto col = neo::acc (this->col);
    auto t = juce::Rectangle<float> (0, (getHeight() - h) / 2, w, h);
    g.setColour (on ? col.withAlpha (0.28f) : veil (0.45f)); g.fillRoundedRectangle (t, h / 2);
    g.setColour (on ? col.withAlpha (0.7f) : glass (0.08f)); g.drawRoundedRectangle (t.reduced (0.5f), h / 2, 1.0f);
    const float d = h - 6, x = on ? t.getRight() - d - 3 : 3.0f;
    juce::ColourGradient lg (juce::Colours::white, x, t.getY() + 3, juce::Colour (0xffb9bccb), x, t.getY() + 3 + d, false); g.setGradientFill (lg); g.fillEllipse (x, t.getY() + 3, d, d);
    auto lr = juce::Rectangle<float> (w + 8, 0, getWidth() - w - 8, (float) getHeight());
    g.setFont (neo::cap (8.5f)); g.setColour (on ? C::text : C::dim); g.drawText (label, lr, juce::Justification::centredLeft);
}

// ======================================================================= catena

static float modGr (Processor& p, int m)
{
    if (m == MAXIM) return p.grMax.load(); if (m == LIMIT) return p.grLim.load(); if (m == BASS) return std::abs (p.bassGain.load());
    if (m == DYNEQ) { float mx = 0; for (int b = 0; b < kEqBands; ++b) mx = juce::jmax (mx, std::abs (p.eqDyn[b].load())); return mx; }
    if (m == IMAGER) return (1.0f - p.corrAll.load()) * 4.0f;
    if (m == DEESS) return p.dsGr.load(); if (m == BUSCOMP) return p.busGr.load();
    if (m == EXCITER) return juce::jlimit (0.0f, 8.0f, p.exMeter.load() * 80.0f);
    if (m == VARIMU) return p.value ("vm_on") > 0.5f ? p.vmGr.load() : 0.0f;
    if (m == TAPE) return p.value ("tp_on") > 0.5f ? p.value ("tp_drive") * 8.0f : 0.0f;
    if (m == MULTI) { float mx = 0; for (int b = 0; b < kMbBands; ++b) mx = juce::jmax (mx, p.mbDown[b].load(), p.mbUp[b].load()); return mx; }
    return 0.0f;
}
void Chain::paintTile (juce::Graphics& g, int m, juce::Rectangle<float> t, bool ghost)
{
    const auto& mi = modInfo (m); const auto c = neo::acc (juce::Colour (mi.colour));
    const bool sel = ed.sel == m, ready = mi.ready, on = ready && ed.proc.value (onParam (m)) > 0.5f;
    // tessera neo: vetro scuro, luce del colore del modulo solo se scelto
    { juce::ColourGradient bgc (glass (sel ? 0.07f : 0.035f), t.getX(), t.getY(), glass (0.01f), t.getX(), t.getBottom(), false); g.setGradientFill (bgc); g.fillRoundedRectangle (t, 12); }
    if (sel || ghost)
    {
        juce::ColourGradient sg (c.withAlpha (0.22f), t.getCentreX(), t.getBottom(), c.withAlpha (0.0f), t.getCentreX(), t.getY(), false); g.setGradientFill (sg); g.fillRoundedRectangle (t, 12);
        g.setColour (c.withAlpha (0.75f)); g.drawRoundedRectangle (t.reduced (0.5f), 12, 1.2f);
    }
    else { g.setColour (glass (0.05f)); g.drawRoundedRectangle (t.reduced (0.5f), 12, 1.0f); }
    if (! ready) { g.setColour (veil (0.45f)); g.fillRoundedRectangle (t, 12); }
    auto pr = juce::Rectangle<float> (t.getX() + 8, t.getY() + 9, 14, 14);
    if (on) { g.setColour (c.withAlpha (0.35f)); g.fillEllipse (pr.expanded (4)); }
    juce::ColourGradient lg (on ? c.brighter (0.6f) : juce::Colour (0xff3a3e48), pr.getCentreX(), pr.getY(), on ? c : juce::Colour (0xff15171c), pr.getCentreX(), pr.getBottom(), false);
    g.setGradientFill (lg); g.fillEllipse (pr);
    auto tr = juce::Rectangle<float> (t.getX() + 28, t.getY() + 6, t.getWidth() - 34, 20);
    g.setFont (neo::cap (8.0f)); g.setColour (ready ? (on ? C::text : C::dim) : C::faint); g.drawFittedText (mi.name, tr.toNearestInt(), juce::Justification::centredLeft, 1, 0.8f);
    g.setColour (ready ? C::faint : C::faint.withAlpha (0.6f)); g.setFont (neo::f (8.5f));
    g.drawText (ready ? juce::String (mi.sub) : juce::String ("IN ARRIVO"), juce::Rectangle<float> (t.getX() + 10, t.getY() + 30, t.getWidth() - 20, 12), juce::Justification::centredLeft);
    // maniglia per trascinare
    g.setColour (glass (0.18f));
    for (int k = 0; k < 3; ++k) g.fillRect (t.getRight() - 14, t.getY() + 10 + k * 4.0f, 7.0f, 1.5f);
    if (ready && ! noBars)
    {
        auto bar = juce::Rectangle<float> (t.getX() + 10, t.getBottom() - 14, t.getWidth() - 20, 5);
        g.setColour (glass (0.06f)); g.fillRoundedRectangle (bar, 2);
        const float gr = juce::jlimit (0.0f, 1.0f, modGr (ed.proc, m) / 8.0f);
        g.setColour (c); g.fillRoundedRectangle (bar.withWidth (bar.getWidth() * gr), 2);
    }
}
void Chain::paint (juce::Graphics& g)
{
    const auto ord = ed.proc.getOrder();
    if (! dragging)
    {   // le tessere cambiano solo se cambia qualcosa (scelta, acceso/spento, ordine, mouse): le metto in cache e ridisegno solo le barrette
        const float sc = juce::jlimit (1.0f, 4.0f, g.getInternalContext().getPhysicalPixelScaleFactor());
        juce::String key = juce::String (ed.sel) + "/" + juce::String (hov) + "/" + juce::String (sc, 2) + "/" + juce::String (getWidth());
        for (int k = 0; k < kNumMods; ++k) { const int m = ord[(size_t) k]; key << "," << m << (ed.proc.value (onParam (m)) > 0.5f ? "+" : "-"); }
        if (key != cacheKey || cache.isNull())
        {
            cacheKey = key; cache = juce::Image (juce::Image::ARGB, juce::jmax (1, juce::roundToInt (getWidth() * sc)), juce::jmax (1, juce::roundToInt (getHeight() * sc)), true);
            juce::Graphics cg (cache); cg.addTransform (juce::AffineTransform::scale (sc)); noBars = true;
            for (int k = 0; k < kNumMods; ++k)
            {
                const int m = ord[(size_t) k]; auto t = tile (k); paintTile (cg, m, t, false);
                if (k < kNumMods - 1) { cg.setColour (C::dim.withAlpha (0.5f)); cg.drawLine (t.getRight() + 1, t.getCentreY(), t.getRight() + 7, t.getCentreY(), 1.0f); }
                if (modInfo (m).ready) { auto bar = juce::Rectangle<float> (t.getX() + 10, t.getBottom() - 14, t.getWidth() - 20, 5); cg.setColour (glass (0.06f)); cg.fillRoundedRectangle (bar, 2); }
            }
            noBars = false;
        }
        g.drawImageTransformed (cache, juce::AffineTransform::scale ((float) getWidth() / (float) cache.getWidth(), (float) getHeight() / (float) cache.getHeight()));
        for (int k = 0; k < kNumMods; ++k)
        {
            const int m = ord[(size_t) k]; if (! modInfo (m).ready) continue; auto t = tile (k);
            const float gr = juce::jlimit (0.0f, 1.0f, modGr (ed.proc, m) / 8.0f); if (gr <= 0.0f) continue;
            auto bar = juce::Rectangle<float> (t.getX() + 10, t.getBottom() - 14, t.getWidth() - 20, 5);
            g.setColour (neo::acc (juce::Colour (modInfo (m).colour))); g.fillRoundedRectangle (bar.withWidth (bar.getWidth() * gr), 2);
        }
        return;
    }
    for (int k = 0; k < kNumMods; ++k)
    {
        const int m = ord[(size_t) k]; auto t = tile (k);
        if (dragMod == m && dragging) { g.setColour (glass (0.05f)); g.drawRoundedRectangle (t.reduced (2), 10, 1.0f); }
        else paintTile (g, m, t, false);
        if (k < kNumMods - 1) { g.setColour (C::dim.withAlpha (0.5f)); g.drawLine (t.getRight() + 1, t.getCentreY(), t.getRight() + 7, t.getCentreY(), 1.0f); }
    }
    if (dragging && dragMod >= 0)
    {
        const int ins = insertAt (dragX);
        const float x = ins < kNumMods ? tile (ins).getX() - 4 : tile (kNumMods - 1).getRight() + 4;
        g.setColour (C::amber); g.fillRoundedRectangle (x - 1.5f, 2, 3, (float) getHeight() - 4, 1.5f);
        auto t = tile (0).withX (dragX - tile (0).getWidth() * 0.5f);
        paintTile (g, dragMod, t, true);
    }
}
int Chain::insertAt (float x) const
{
    for (int k = 0; k < kNumMods; ++k) if (x < tile (k).getCentreX()) return k;
    return kNumMods;
}
void Chain::mouseDown (const juce::MouseEvent& e)
{
    const auto ord = ed.proc.getOrder(); dragging = false; dragMod = -1;
    for (int k = 0; k < kNumMods; ++k)
    {
        if (! tile (k).contains (e.position)) continue;
        const int m = ord[(size_t) k];
        auto pr = juce::Rectangle<float> (tile (k).getX() + 8, tile (k).getY() + 9, 14, 14);
        if (modInfo (m).ready && pr.expanded (6).contains (e.position)) { auto* p = ed.P (onParam (m)); p->setValueNotifyingHost (p->getValue() > 0.5f ? 0.0f : 1.0f); }
        dragMod = m; ed.advView = 0; ed.select (m); ed.advTabsComp.repaint(); repaint(); return;
    }
}
void Chain::mouseDrag (const juce::MouseEvent& e)
{
    if (dragMod < 0) return;
    if (! dragging && e.getDistanceFromDragStart() > 8) dragging = true;
    if (dragging) { dragX = e.position.x; setMouseCursor (juce::MouseCursor::DraggingHandCursor); repaint(); }
}
void Chain::mouseUp (const juce::MouseEvent&)
{
    if (dragging && dragMod >= 0)
    {
        auto ord = ed.proc.getOrder(); int from = 0; for (int k = 0; k < kNumMods; ++k) if (ord[(size_t) k] == dragMod) from = k;
        int to = insertAt (dragX); if (to > from) --to;
        std::vector<int> v (ord.begin(), ord.end()); v.erase (v.begin() + from); v.insert (v.begin() + juce::jlimit (0, (int) v.size(), to), dragMod);
        for (int k = 0; k < kNumMods; ++k) ord[(size_t) k] = v[(size_t) k];
        ed.proc.setOrder (ord);
    }
    dragging = false; dragMod = -1; setMouseCursor (juce::MouseCursor::PointingHandCursor); repaint();
}
void Chain::mouseMove (const juce::MouseEvent& e)
{
    int h = -1; for (int m = 0; m < kNumMods; ++m) if (tile (m).contains (e.position)) h = m;
    if (h != hov) { hov = h; repaint(); }
    setMouseCursor (h >= 0 ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
}

// ======================================================================= storia
void History::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat(); lcd (g, b, 8);
    auto& P = ed.proc; const int n = Processor::kHist, w0 = P.hW.load();
    auto plot = b.reduced (40, 12).withTrimmedRight (-28);
    const float ceil = P.value ("lm_ceil"), tgt = target ((int) P.value ("target")).lufs;
    auto yDb = [&] (float db) { return plot.getBottom() - juce::jlimit (0.0f, 1.0f, (db + 36) / 36) * plot.getHeight(); };
    auto yGr = [&] (float gr) { return plot.getY() + juce::jlimit (0.0f, 1.0f, gr / 12.0f) * plot.getHeight() * 0.6f; };
    // griglia
    g.setFont (mf (8.5f));
    for (float db : { 0.0f, -6.0f, -12.0f, -18.0f, -24.0f, -30.0f })
    {
        const float y = yDb (db); g.setColour (C::phos.withAlpha (0.07f)); g.drawHorizontalLine ((int) y, plot.getX(), plot.getRight());
        g.setColour (C::dim.withAlpha (0.7f)); g.drawText (juce::String ((int) db), juce::Rectangle<float> (b.getX() + 4, y - 6, 30, 12), juce::Justification::centredRight);
    }
    // una colonna per pixel (il massimo delle colonne da 5 ms che ci cadono): grigio = ingresso, colore = uscita (pieno = RMS)
    const int npx = juce::jmax (8, (int) plot.getWidth()); const float dx = plot.getWidth() / (float) npx;
    std::vector<float> px ((size_t) npx);
    auto reduce = [&] (std::atomic<float>* arr, float init)
    {
        for (int x = 0; x < npx; ++x)
        {
            const int i0 = x * n / npx, i1 = juce::jmax (i0 + 1, (x + 1) * n / npx); float m = init;
            for (int i = i0; i < i1; ++i) m = juce::jmax (m, arr[(w0 + i) % n].load());
            px[(size_t) x] = m;
        }
    };
    auto cols = [&] (std::atomic<float>* arr, juce::Colour c)
    {
        reduce (arr, -200.0f); juce::RectangleList<float> rl;
        for (int x = 0; x < npx; ++x) { const float v = px[(size_t) x]; if (v > -36) { const float xx = plot.getX() + dx * x; rl.addWithoutMerging (juce::Rectangle<float>::leftTopRightBottom (xx, yDb (v), xx + dx + 0.35f, plot.getBottom())); } }
        g.setColour (c); g.fillRectList (rl);
    };
    cols (P.hIn, C::dim.withAlpha (0.28f));
    cols (P.hOut, (limiter ? C::red : C::magenta).withAlpha (0.3f));
    cols (P.hOutR, (limiter ? C::red : C::magenta).withAlpha (0.55f));
    // ceiling
    g.setColour (C::red.withAlpha (0.7f)); const float yc = yDb (ceil);
    for (float x = plot.getX(); x < plot.getRight(); x += 8) g.drawHorizontalLine ((int) yc, x, x + 4);
    g.drawText ("CEILING " + juce::String (ceil, 1), juce::Rectangle<float> (plot.getRight() - 120, yc - 14, 116, 12), juce::Justification::centredRight);
    // riduzione di guadagno dall'alto, a colonne
    {
        std::atomic<float>* gr = limiter ? P.hGrL : P.hGrM;
        reduce (gr, 0.0f); juce::RectangleList<float> rl;
        for (int x = 0; x < npx; ++x) { const float v = px[(size_t) x]; if (v > 0.05f) { const float xx = plot.getX() + dx * x; rl.addWithoutMerging (juce::Rectangle<float>::leftTopRightBottom (xx, plot.getY(), xx + dx + 0.35f, yGr (v))); } }
        const auto c = limiter ? C::amber : C::phos;
        g.setColour (c.withAlpha (0.75f)); g.fillRectList (rl);
    }
    // loudness short-term e target
    {
        juce::Path p; bool st = false;
        for (int i = 0; i < n; i += 8) { const float v = P.hS[(w0 + i) % n].load(); if (v < -60) { st = false; continue; } const float x = plot.getX() + plot.getWidth() * i / (n - 1); if (! st) { p.startNewSubPath (x, yDb (v)); st = true; } else p.lineTo (x, yDb (v)); }
        g.setColour (C::green.withAlpha (0.3f)); g.strokePath (p, juce::PathStrokeType (3.5f)); g.setColour (C::green); g.strokePath (p, juce::PathStrokeType (1.3f));
        const float yt = yDb (tgt); g.setColour (C::green.withAlpha (0.5f)); for (float x = plot.getX(); x < plot.getRight(); x += 6) g.drawHorizontalLine ((int) yt, x, x + 2);
    }
    g.setColour (C::dim); g.setFont (mf (8.5f));
    g.drawText (juce::String ("RIDUZIONE ") + (limiter ? "LIMITER" : "MAXIMIZER") + "   USCITA   INGRESSO   LUFS BREVE  e  TARGET", juce::Rectangle<float> (plot.getX(), b.getBottom() - 12, plot.getWidth(), 10), juce::Justification::centredLeft);
    scanlines (g, b);
}

// ======================================================================= pagine
MaxPage::MaxPage (Editor& e) : ed (e), hist (e, false)
{
    gain = std::make_unique<Knob> (ed.P ("mx_gain"), C::magenta); gain->label = "SPINTA"; gain->big = true; gain->text = [this] { return "+" + juce::String (ed.proc.value ("mx_gain"), 1) + " dB"; };
    clip = std::make_unique<Knob> (ed.P ("mx_clip"), C::amber); clip->label = "CLIP"; clip->text = [this] { return juce::String (juce::roundToInt (ed.proc.value ("mx_clip") * 100)) + "%"; };
    chr = std::make_unique<Knob> (ed.P ("mx_char"), C::phos); chr->label = "CARATTERE"; chr->text = [this] { const float v = ed.proc.value ("mx_char"); return v < 0.35f ? juce::String ("LENTO") : v > 0.65f ? juce::String ("VELOCE") : juce::String ("MEDIO"); };
    link = std::make_unique<Knob> (ed.P ("mx_link"), C::violet); link->label = "LINK STEREO"; link->text = [this] { return juce::String (juce::roundToInt (ed.proc.value ("mx_link") * 100)) + "%"; };
    juce::StringArray st; for (int s = 0; s < kNumStyles; ++s) st.add (styleInfo (s).name);
    style = std::make_unique<Segs> (ed.P ("mx_style"), C::magenta, st);
    on = std::make_unique<Switch> (ed.P ("mx_on"), C::magenta, "ATTIVO");
    delta = std::make_unique<Switch> (ed.P ("mx_delta"), C::amber, "DELTA");
    learn = std::make_unique<Switch> (ed.P ("mx_learn"), C::green, "PORTA AL TARGET IN AUTOMATICO");
    for (juce::Component* c : std::initializer_list<juce::Component*> { gain.get(), clip.get(), chr.get(), link.get(), style.get(), on.get(), delta.get(), learn.get(), &hist }) addAndMakeVisible (c);
    for (auto* k : { gain.get(), clip.get(), chr.get(), link.get() }) k->onTouch = [this] { auto* p = ed.P ("mx_on"); if (p->getValue() < 0.5f) p->setValueNotifyingHost (1.0f); };
}
void MaxPage::resized()
{
    on->setBounds (170, 12, 100, 22); delta->setBounds (290, 12, 240, 22);
    style->setBounds (16, 48, 560, 30);
    gain->setBounds (16, 132, 150, 150 + 32); clip->setBounds (186, 156, 100, 100 + 27); chr->setBounds (296, 156, 100, 100 + 27); link->setBounds (406, 156, 100, 100 + 27);
    learn->setBounds (540, 132, 300, 22);
    hist.setBounds (16, 324, 822, 290);
}
void MaxPage::paint (juce::Graphics& g)
{
    glowText (g, "MAXIMIZER", { 16, 10, 150, 26 }, C::magenta, uf (17.0f));
    const int s = (int) ed.proc.value ("mx_style");
    g.setColour (C::text.withAlpha (0.85f)); g.setFont (uf (11.5f, false));
    g.drawFittedText (styleInfo (s).what, juce::Rectangle<int> (16, 84, 820, 18), juce::Justification::topLeft, 1);
    // stato del target
    auto r = juce::Rectangle<float> (540, 160, 296, 110); lcd (g, r, 8);
    const auto& tg = target ((int) ed.proc.value ("target")); const float sNow = ed.proc.lufsS.load();
    g.setColour (C::dim); g.setFont (mf (9.0f)); g.drawText ("TARGET  " + juce::String (tg.name), r.reduced (12, 8).removeFromTop (14), juce::Justification::centredLeft);
    glowText (g, juce::String ((int) tg.lufs) + " LUFS", { r.getX() + 12, r.getY() + 24, 160, 30 }, C::green, neo::big (19.0f));
    juce::String st = sNow < -60 ? juce::String ("in attesa di segnale")
                      : std::abs (sNow - tg.lufs) < 0.6f ? juce::String ("sei sul target")
                      : sNow > tg.lufs ? "sei " + juce::String (sNow - tg.lufs, 1) + " dB sopra" : "mancano " + juce::String (tg.lufs - sNow, 1) + " dB";
    glowText (g, st, { r.getX() + 12, r.getY() + 58, r.getWidth() - 24, 18 }, std::abs (sNow - tg.lufs) < 0.6f ? C::green : C::amber, uf (12.0f));
    g.setColour (C::dim); g.setFont (uf (10.0f, false));
    g.drawFittedText ("Acceso: la SPINTA si regola da sola finche il volume breve arriva al target scelto a destra.", juce::Rectangle<int> ((int) r.getX() + 12, (int) r.getY() + 78, (int) r.getWidth() - 24, 28), juce::Justification::topLeft, 2);
    g.setColour (C::dim); g.setFont (uf (10.0f, false));
    g.drawFittedText ("SPINTA alza il volume dentro il limitatore. CLIP fa lavorare prima un clipper morbido (piu volume, transienti piu corti). CARATTERE: rilascio lento e pulito o veloce e presente.",
                      juce::Rectangle<int> (16, 104, 820, 18), juce::Justification::topLeft, 1);
}

LimPage::LimPage (Editor& e) : ed (e), hist (e, true)
{
    ceil = std::make_unique<Knob> (ed.P ("lm_ceil"), C::red); ceil->label = "CEILING"; ceil->big = true; ceil->text = [this] { return juce::String (ed.proc.value ("lm_ceil"), 1) + (ed.proc.value ("lm_tp") > 0.5f ? " dBTP" : " dBFS"); };
    gain = std::make_unique<Knob> (ed.P ("lm_gain"), C::red); gain->label = "GAIN"; gain->text = [this] { return "+" + juce::String (ed.proc.value ("lm_gain"), 1) + " dB"; };
    rel = std::make_unique<Knob> (ed.P ("lm_rel"), C::amber); rel->label = "RILASCIO"; rel->text = [this] { return ed.proc.value ("lm_auto") > 0.5f ? juce::String ("AUTO") : juce::String (juce::roundToInt (ed.proc.value ("lm_rel"))) + " ms"; };
    on = std::make_unique<Switch> (ed.P ("lm_on"), C::red, "ATTIVO");
    delta = std::make_unique<Switch> (ed.P ("lm_delta"), C::amber, "DELTA");
    autoR = std::make_unique<Switch> (ed.P ("lm_auto"), C::amber, "RILASCIO AUTOMATICO");
    tp = std::make_unique<Switch> (ed.P ("lm_tp"), C::red, "TRUE PEAK (8x)");
    for (juce::Component* c : std::initializer_list<juce::Component*> { gain.get(), ceil.get(), rel.get(), on.get(), delta.get(), autoR.get(), tp.get(), &hist }) addAndMakeVisible (c);
    for (auto* k : { gain.get(), ceil.get(), rel.get() }) k->onTouch = [this] { auto* p = ed.P ("lm_on"); if (p->getValue() < 0.5f) p->setValueNotifyingHost (1.0f); };
}
void LimPage::resized()
{
    on->setBounds (150, 12, 100, 22); delta->setBounds (270, 12, 240, 22);
    gain->setBounds (16, 60, 150, 150 + 32); ceil->setBounds (176, 60, 150, 150 + 32); rel->setBounds (336, 82, 90, 90 + 27);
    autoR->setBounds (336, 210, 200, 22); tp->setBounds (336, 238, 200, 22);
    hist.setBounds (16, 324, 822, 290);
}
void LimPage::paint (juce::Graphics& g)
{
    glowText (g, "LIMITER", { 16, 10, 130, 26 }, C::red, uf (17.0f));
    auto r = juce::Rectangle<float> (540, 60, 296, 200); lcd (g, r, 8);
    const float tp = ed.proc.tpMax.load(), ceil = ed.proc.value ("lm_ceil");
    g.setColour (C::dim); g.setFont (mf (9.0f)); g.drawText ("PICCO VERO MASSIMO IN USCITA", r.reduced (12, 8).removeFromTop (14), juce::Justification::centredLeft);
    glowText (g, tp < -60 ? juce::String ("-") : juce::String (tp, 2) + " dBTP", { r.getX() + 12, r.getY() + 26, 260, 34 }, tp > ceil + 0.05f ? C::red : C::green, neo::big (22.0f));
    g.setColour (C::dim); g.setFont (uf (10.5f, false));
    g.drawFittedText ("Ultimo stadio, sempre in fondo alla catena. Guarda i picchi tra un campione e l'altro (true peak) cosi la conversione in MP3/AAC "
                      "di Spotify e YouTube non distorce. -1 dBTP per lo streaming, -0.3 per club e CD.",
                      juce::Rectangle<int> ((int) r.getX() + 12, (int) r.getY() + 70, (int) r.getWidth() - 24, 120), juce::Justification::topLeft, 7);
    g.setColour (C::dim); g.setFont (uf (10.0f, false));
    g.drawFittedText ("Il limiter dovrebbe lavorare poco (0-2 dB): il volume lo fa il MAXIMIZER, il limiter tiene fermo il tetto.", juce::Rectangle<int> (16, 286, 820, 30), juce::Justification::topLeft, 2);
}

// ======================================================================= analizzatore
void Analyzer::run (const StereoRing& rg, float* out, int nPts, float f0, float f1, double sr, float smooth)
{
    const int order = 13, N = 1 << order;
    if (! fft) { fft = std::make_unique<juce::dsp::FFT> (order); a.resize ((size_t) (2 * N)); b.resize ((size_t) N); }
    rg.read (a.data(), b.data(), N);
    for (int i = 0; i < N; ++i) a[(size_t) i] = 0.5f * (a[(size_t) i] + b[(size_t) i]) * (0.5f - 0.5f * std::cos (6.2831853f * i / N));
    std::fill (a.begin() + N, a.end(), 0.0f);
    fft->performFrequencyOnlyForwardTransform (a.data(), true);
    const float lr = std::log (f1 / f0);
    for (int k = 0; k < nPts; ++k)
    {
        const float fa = f0 * std::exp (lr * k / nPts), fb = f0 * std::exp (lr * (k + 1) / nPts);
        const float xa = (float) (fa * N / sr), xb = (float) (fb * N / sr);
        float mx = 0;
        if (xb - xa < 1.0f) { const float x = 0.5f * (xa + xb); const int i0 = (int) x; const float t = x - i0; mx = a[(size_t) i0] * (1 - t) + a[(size_t) (i0 + 1)] * t; }
        else for (int j = (int) xa; j <= (int) xb && j < N / 2; ++j) mx = juce::jmax (mx, a[(size_t) j]);
        const float db = gToDb (mx * 4.0f / N) + 4.5f * std::log2 (juce::jmax (20.0f, 0.5f * (fa + fb)) / 1000.0f);
        const float v = juce::jmax (-110.0f, db);   // sale subito (a tempo coi colpi), scende morbido
        out[k] = v > out[k] ? out[k] * smooth * 0.25f + v * (1 - smooth * 0.25f) : out[k] * smooth + v * (1 - smooth);
    }
}
static void drawSpec (juce::Graphics& g, const float* sp, int n, juce::Rectangle<float> r, juce::Colour c, bool fill, float lo = -84, float hi = 0)
{
    juce::Path p;
    for (int k = 0; k < n; ++k) { const float x = r.getX() + r.getWidth() * (k + 0.5f) / n, y = juce::jlimit (r.getY(), r.getBottom(), r.getBottom() - (sp[k] - lo) / (hi - lo) * r.getHeight()); if (k == 0) p.startNewSubPath (x, y); else p.lineTo (x, y); }
    if (fill) { juce::Path f = p; f.lineTo (r.getRight(), r.getBottom()); f.lineTo (r.getX(), r.getBottom()); f.closeSubPath(); juce::ColourGradient cg (c.withAlpha (0.25f), 0, r.getY(), c.withAlpha (0.02f), 0, r.getBottom(), false); g.setGradientFill (cg); g.fillPath (f); }
    g.setColour (c.withAlpha (0.25f)); g.strokePath (p, juce::PathStrokeType (3.0f)); g.setColour (c); g.strokePath (p, juce::PathStrokeType (1.1f));
}
static juce::String hzT (float f) { return f >= 1000 ? juce::String (f / 1000.0f, f >= 10000 ? 1 : 2) + " kHz" : juce::String (juce::roundToInt (f)) + " Hz"; }

// ======================================================================= pagina base
ModPage::ModPage (Editor& e, int id, const char* onP, const char* deltaP) : ed (e), mod (id), onId (onP)
{
    const auto c = neo::acc (juce::Colour (modInfo (id).colour));
    on = std::make_unique<Switch> (ed.P (onP), c, "ATTIVO"); delta = std::make_unique<Switch> (ed.P (deltaP), C::amber, "DELTA");
    addAndMakeVisible (*on); addAndMakeVisible (*delta);
}
void ModPage::touch() { auto* p = ed.P (onId); if (p->getValue() < 0.5f) p->setValueNotifyingHost (1.0f); }
Knob* ModPage::knob (const char* pid, juce::Colour c, const juce::String& label, std::function<juce::String()> txt, bool bip)
{
    auto* k = knobs.add (new Knob (ed.P (pid), c)); k->label = label; k->text = std::move (txt); k->bipolar = bip; k->onTouch = [this] { touch(); };
    addAndMakeVisible (k); return k;
}
void ModPage::paintHeader (juce::Graphics& g, const juce::String& help)
{
    const auto c = neo::acc (juce::Colour (modInfo (mod).colour));
    glowText (g, modInfo (mod).name, { 16, 10, 200, 26 }, c, uf (17.0f));
    if (ed.proc.value (onId) < 0.5f) { g.setFont (neo::f (9.0f)); g.setColour (C::amber.withAlpha (0.8f)); g.drawText ("SPENTO: tocca un comando per accenderlo", juce::Rectangle<float> (480, 34, 356, 12), juce::Justification::centredRight); }
    g.setColour (C::dim); g.setFont (uf (10.0f, false));
    g.drawFittedText (help, juce::Rectangle<int> (16, getHeight() - 52, getWidth() - 32, 40), juce::Justification::topLeft, 3);
}

// ======================================================================= BASS CONTROL
BassPage::BassPage (Editor& e) : ModPage (e, BASS, "bc_on", "bc_delta"), view (e)
{
    auto& P = ed.proc; const auto a = C::amber;
    knob ("bc_freq", a, "FREQUENZA", [&P] { return juce::String (juce::roundToInt (P.value ("bc_freq"))) + " Hz"; });
    knob ("bc_punch", C::magenta, "WARM / PUNCH", [&P] { const float v = P.value ("bc_punch"); return std::abs (v) < 0.02f ? juce::String ("0") : v > 0 ? "PUNCH " + juce::String (juce::roundToInt (v * 100)) : "WARM " + juce::String (juce::roundToInt (-v * 100)); }, true);
    knob ("bc_tail", C::violet, "CODA", [&P] { const float v = P.value ("bc_tail"); return std::abs (v) < 0.02f ? juce::String ("0") : v < 0 ? "STRINGI " + juce::String (juce::roundToInt (-v * 100)) : "ALLUNGA " + juce::String (juce::roundToInt (v * 100)); }, true);
    knob ("bc_thr", C::red, "SOGLIA PICCHI", [&P] { const float v = P.value ("bc_thr"); return v > -0.05f ? juce::String ("OFF") : juce::String (v, 1) + " dB"; });
    knob ("bc_dirty", C::red, "PULITO / SPORCO", [&P] { const float v = P.value ("bc_dirty"); return v < 0.1f ? juce::String ("PULITO") : v > 0.9f ? juce::String ("SPORCO") : juce::String (juce::roundToInt (v * 100)) + "%"; });
    knob ("bc_harm", a, "ARMONICHE", [&P] { return juce::String (juce::roundToInt (P.value ("bc_harm") * 100)) + "%"; });
    knob ("bc_bal", a, "BILANCIO", [&P] { return dbS (P.value ("bc_bal")) + " dB"; }, true);
    knob ("bc_mono", C::phos, "MONO SOTTO", [&P] { const float v = P.value ("bc_mono"); return v < 20 ? juce::String ("OFF") : juce::String (juce::roundToInt (v)) + " Hz"; });
    addAndMakeVisible (view);
}
void BassPage::resized()
{
    on->setBounds (190, 12, 100, 22); delta->setBounds (306, 12, 230, 22);
    view.setBounds (16, 48, 822, 330);
    for (int i = 0; i < knobs.size(); ++i) knobs[i]->setBounds (16 + i * 103, 396, 98, 98 + 27);
}
void BassPage::paint (juce::Graphics& g)
{
    paintHeader (g, "Tutto lavora solo sotto FREQUENZA. PUNCH: piu attacco alla cassa. WARM: attacco morbido e piu sustain. CODA stringe o allunga l'808. "
                    "SOGLIA PICCHI tiene fermi i colpi di basso troppo forti (PULITO = morbido, SPORCO = veloce e saturo). BILANCIO alza o abbassa i bassi. DELTA fa sentire solo cosa cambia sotto FREQUENZA.");
}
void BassView::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat(); lcd (g, b, 8);
    auto& P = ed.proc; auto r = b.reduced (14, 10).withTrimmedBottom (16).withTrimmedRight (110);
    const float f0 = 20, f1 = 500; auto x = [&] (float f) { return r.getX() + (float) (std::log (f / f0) / std::log (f1 / f0)) * r.getWidth(); };
    anIn.run (P.ringIn, spIn, 200, f0, f1, P.sampleRate()); anOut.run (P.ringOut, spOut, 200, f0, f1, P.sampleRate());
    g.setFont (mf (8.5f));
    for (float f : { 30.0f, 40.0f, 60.0f, 80.0f, 100.0f, 150.0f, 200.0f, 300.0f, 400.0f })
    { g.setColour (C::phos.withAlpha (0.07f)); g.drawVerticalLine ((int) x (f), r.getY(), r.getBottom()); g.setColour (C::dim.withAlpha (0.7f)); g.drawText (juce::String ((int) f), juce::Rectangle<float> (x (f) - 16, r.getBottom() + 2, 32, 12), juce::Justification::centred); }
    // zona mono
    const float mono = P.value ("bc_mono");
    if (mono >= 20) { auto mr = r.withRight (x (mono)); g.setColour (C::phos.withAlpha (0.08f)); g.fillRect (mr); glowText (g, "MONO", mr.withHeight (18).translated (6, 4), C::phos, uf (10.0f)); }
    // taglio della banda bassa
    const float fx = x (P.value ("bc_freq"));
    g.setColour (C::amber.withAlpha (0.08f)); g.fillRect (r.withRight (fx));
    g.setColour (C::amber); g.drawVerticalLine ((int) fx, r.getY(), r.getBottom());
    glowText (g, "BANDA BASSA  " + juce::String (juce::roundToInt (P.value ("bc_freq"))) + " Hz", { fx + 6, r.getY() + 4, 180, 16 }, C::amber, uf (10.0f));
    drawSpec (g, spIn, 200, r, C::dim.withAlpha (0.6f), false, -70, 0);
    drawSpec (g, spOut, 200, r, C::amber, true, -70, 0);
    // guadagno della banda bassa nel tempo
    hist[hw] = P.value ("bc_on") > 0.5f ? P.bassGain.load() : 0.0f; hw = (hw + 1) % 160;
    g.setColour (C::dim); g.setFont (mf (9.0f));
    g.drawText ("ENERGIA SOTTO " + juce::String (juce::roundToInt (P.value ("bc_freq"))) + " Hz: " + juce::String (P.bassLow.load(), 1) + " dB del totale    PICCHI -" + juce::String (P.bassPeak.load(), 1) + " dB",
                juce::Rectangle<float> (r.getX(), r.getY() + 22, 420, 12), juce::Justification::centredLeft);
    auto hr = juce::Rectangle<float> (b.getRight() - 112, b.getY() + 10, 98, b.getHeight() - 36);
    g.setColour (veil (0.3f)); g.fillRoundedRectangle (hr, 4);
    g.setColour (C::dim.withAlpha (0.5f)); g.drawHorizontalLine ((int) hr.getCentreY(), hr.getX(), hr.getRight());
    juce::Path p; for (int i = 0; i < 160; ++i) { const float v = hist[(hw + i) % 160]; const float xx = hr.getX() + hr.getWidth() * i / 159.0f, yy = hr.getCentreY() - juce::jlimit (-1.0f, 1.0f, v / 9.0f) * hr.getHeight() * 0.48f; if (i == 0) p.startNewSubPath (xx, yy); else p.lineTo (xx, yy); }
    g.setColour (C::magenta); g.strokePath (p, juce::PathStrokeType (1.3f));
    g.setColour (C::dim); g.setFont (mf (8.5f)); g.drawText ("GUADAGNO BASSI", juce::Rectangle<float> (hr.getX(), hr.getBottom() + 2, hr.getWidth(), 12), juce::Justification::centred);
    glowText (g, dbS (P.bassGain.load()) + " dB", { hr.getX(), hr.getY() + 4, hr.getWidth(), 14 }, C::magenta, mf (10.0f), juce::Justification::centred);
    scanlines (g, b);
}

// ======================================================================= EQ (stile Pro-Q, motore condiviso con Mixology)
static juce::Colour bandCol (int b)
{
    static const juce::uint32 c[kEqBands] = { 0xffffb547, 0xffff8a3d, 0xffff3d9a, 0xff8b5cff, 0xff3ee8ff, 0xff4dffb4, 0xffffe066, 0xffff6b6b, 0xffc77dff, 0xff4d7dff, 0xff3dffd2, 0xffb8f04d };
    return juce::Colour (c[juce::jlimit (0, kEqBands - 1, b)]);
}
EqPage::EqPage (Editor& e) : ModPage (e, DYNEQ, "eq_on", "eq_delta"), view (e) { addAndMakeVisible (view); }
void EqPage::resized() { on->setBounds (190, 12, 110, 22); delta->setBounds (300, 12, 240, 22); view.setBounds (12, 44, getWidth() - 24, getHeight() - 104); }
void EqPage::paint (juce::Graphics& g)
{
    paintHeader (g, juce::String::fromUTF8 ("Doppio clic sul grafico = nuova banda (fino a 12)  \xc2\xb7  trascina un punto = frequenza e guadagno  \xc2\xb7  rotella = Q  \xc2\xb7  "
                    "passa sopra un punto: la sua scheda (tipo, canale, SOLO, manopoline)  \xc2\xb7  doppio clic su un punto = spegni la banda  \xc2\xb7  clic destro = menu. "
                    "Ogni banda e' gia' dinamica: gira SOGLIA e RANGE e lavora solo quando la sua zona supera la soglia. RANGE a zero = banda statica."));
}

EqView::EqView (Editor& e) : ed (e)
{
    win.resize (4096); buf.resize (8192); specPre.assign (2048, -100.0f); specPost.assign (2048, -100.0f);
    for (int i = 0; i < 4096; ++i) win[(size_t) i] = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * i / 4095.0f);
    addChildComponent (pop);
    // ridisegno agganciato al refresh dello schermo (come l'onda del FACILE): le bande dinamiche scorrono senza scatti
    vb = std::make_unique<juce::VBlankAttachment> (this, [this]
    {
        if (! isShowing()) return;
        const double now = juce::Time::getMillisecondCounterHiRes(); const float dt = (float) juce::jlimit (1.0, 50.0, now - lastFrame); lastFrame = now;
        const float k = 1.0f - std::exp (-dt / 45.0f);
        for (int b = 0; b < kEqBands; ++b) dynShown[b] += (ed.proc.eqDyn[b].load() - dynShown[b]) * k;
        repaint (plot().toNearestInt().expanded (14));
    });
}
float EqView::val (int b, const char* k) const { return ed.proc.value (eqId (b, k)); }
void EqView::setV (int b, const char* k, float v)
{
    if (auto* p = ed.P (eqId (b, k))) p->setValueNotifyingHost (p->convertTo0to1 (v));
    if (ed.proc.value ("eq_on") < 0.5f) if (auto* o = ed.P ("eq_on")) o->setValueNotifyingHost (1.0f);   // toccare l'EQ lo accende
}
float EqView::xOf (float f) const { auto p = plot(); return p.getX() + p.getWidth() * std::log (juce::jmax (10.0f, f) / 20.0f) / std::log (1000.0f); }
float EqView::fOf (float x) const { auto p = plot(); return 20.0f * std::pow (1000.0f, (x - p.getX()) / p.getWidth()); }
float EqView::yOf (float db) const { auto p = plot(); return p.getY() + p.getHeight() * (0.5f - db / 36.0f); }
float EqView::dbOf (float y) const { auto p = plot(); return (0.5f - (y - p.getY()) / p.getHeight()) * 36.0f; }
juce::Point<float> EqView::nodeOf (int b) const
{
    const int t = (int) val (b, "type");
    return { xOf (val (b, "f")), jmx::hasGain (t) ? yOf (val (b, "g")) : yOf (0) };
}
int EqView::hit (juce::Point<float> p) const
{
    int best = -1; float bd = 13;
    for (int b = 0; b < kEqBands; ++b) if (val (b, "on") > 0.5f) { const float d = p.getDistanceFrom (nodeOf (b)); if (d < bd) { bd = d; best = b; } }
    return best;
}
void EqView::tick()
{
    auto spec = [this] (const StereoRing& r, std::vector<float>& out)
    {
        std::vector<float> l (4096), rr (4096); r.read (l.data(), rr.data(), 4096);
        for (int i = 0; i < 4096; ++i) buf[(size_t) i] = 0.5f * (l[(size_t) i] + rr[(size_t) i]) * win[(size_t) i];
        std::fill (buf.begin() + 4096, buf.end(), 0.0f); fft.performFrequencyOnlyForwardTransform (buf.data());
        for (int i = 0; i < 2048; ++i) { const float db = juce::Decibels::gainToDecibels (buf[(size_t) i] / 1024.0f, -100.0f); out[(size_t) i] = db > out[(size_t) i] ? db : out[(size_t) i] * 0.85f + db * 0.15f; }
    };
    spec (ed.proc.ringIn, specPre); spec (ed.proc.ringOut, specPost);
    if (popHold > 0 && --popHold == 0 && ! pop.isMouseOver (true) && ! isMouseOver (false)) { popBand = sel; updatePop(); }
}
void EqView::updatePop()
{
    const int b = popBand >= 0 ? popBand : sel;
    if (b < 0 || val (b, "on") < 0.5f) { pop.setVisible (false); pop.band = -1; return; }
    pop.show (b);
    const auto n = nodeOf (b); const int pw = pop.getWidth(), ph = pop.getHeight();
    const int x = juce::jlimit (4, getWidth() - pw - 4, (int) n.x - pw / 2);
    int y = (int) n.y + 18; if (y + ph > getHeight() - 4) y = (int) n.y - 18 - ph;
    pop.setTopLeftPosition (x, juce::jlimit (4, getHeight() - ph - 4, y));
    pop.setVisible (true); pop.repaint();
}
void EqView::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    auto p = plot();
    const double sr = juce::jmax (8000.0, ed.proc.sampleRate());
    // vetro, griglia e scritte: fermi, in cache
    grid.draw (g, b, neo::K::theme, [&] (juce::Graphics& gg)
    {
        lcd (gg, b, 10);
        gg.setFont (neo::num (8.0f));
        for (float f : { 20.f, 30.f, 40.f, 50.f, 60.f, 80.f, 100.f, 200.f, 300.f, 400.f, 500.f, 600.f, 800.f, 1000.f, 2000.f, 3000.f, 4000.f, 5000.f, 6000.f, 8000.f, 10000.f, 20000.f })
        {
            const bool major = f == 20 || f == 50 || f == 100 || f == 200 || f == 500 || f == 1000 || f == 2000 || f == 5000 || f == 10000 || f == 20000;
            gg.setColour (glass (major ? 0.07f : 0.03f)); gg.drawVerticalLine ((int) xOf (f), p.getY(), p.getBottom());
            if (major) { gg.setColour (C::dim); gg.drawText (f >= 1000 ? juce::String ((int) (f / 1000)) + "k" : juce::String ((int) f), juce::Rectangle<float> (xOf (f) - 14, p.getBottom() + 2, 28, 12), juce::Justification::centred); }
        }
        for (int db = -12; db <= 12; db += 6)
        { gg.setColour (glass (db == 0 ? 0.13f : 0.04f)); gg.drawHorizontalLine ((int) yOf ((float) db), p.getX(), p.getRight()); gg.setColour (C::dim); gg.drawText ((db > 0 ? "+" : "") + juce::String (db), juce::Rectangle<float> (b.getX() + 4, yOf ((float) db) - 6, 26, 12), juce::Justification::centredRight); }
    });
    auto specPath = [&] (const std::vector<float>& sp, bool fill) -> juce::Path
    {
        juce::Path path; bool st = false;
        for (float x = p.getX(); x < p.getRight(); x += 2)
        {
            const float f = fOf (x); const int bin = juce::jlimit (1, 2047, (int) (f / sr * 4096));
            const float db = sp[(size_t) bin] + 4.5f * std::log2 (juce::jmax (0.05f, f / 1000.0f));
            const float y = juce::jlimit (p.getY(), p.getBottom(), p.getBottom() - (db + 90.0f) / 90.0f * p.getHeight());
            if (! st) { path.startNewSubPath (x, fill ? p.getBottom() : y); st = true; if (fill) path.lineTo (x, y); } else path.lineTo (x, y);
        }
        if (fill) { path.lineTo (p.getRight(), p.getBottom()); path.closeSubPath(); }
        return path;
    };
    g.setColour (glass (0.16f)); g.strokePath (specPath (specPre, false), juce::PathStrokeType (1.0f));
    { juce::ColourGradient sg (C::phos.withAlpha (0.22f), 0, p.getY(), C::phos.withAlpha (0.03f), 0, p.getBottom(), false); g.setGradientFill (sg); g.fillPath (specPath (specPost, true)); }
    const bool eqOn = ed.proc.value ("eq_on") > 0.5f;
    juce::Path tot; const int NX = (int) (p.getWidth() / 2) + 2; std::vector<double> sum ((size_t) NX, 0.0);
    for (int k = 0; k < kEqBands; ++k)
    {
        if (val (k, "on") < 0.5f) continue;
        jmx::BandSet B; B.on = true; B.type = (int) val (k, "type"); B.slope = (int) val (k, "slope"); B.ch = (int) val (k, "ch"); B.f = val (k, "f"); B.g = val (k, "g"); B.q = val (k, "q");
        B.dyn = val (k, "dyn") > 0.5f; B.range = val (k, "rng");
        jmx::BQ f[8]; const float gNow = B.g + (B.dyn ? dynShown[k] : 0.0f); const int n = jmx::designBand (B, sr, gNow, f);
        juce::Path path; int i = 0; bool st = false;
        for (float x = p.getX(); x < p.getRight() && i < NX; x += 2, ++i)
        {
            double d = 0; const float fr = fOf (x); for (int q = 0; q < n; ++q) d += f[q].magDb (fr, sr);
            if (B.ch <= 1) sum[(size_t) i] += d;
            const float y = yOf ((float) juce::jlimit (-24.0, 24.0, d));
            if (! st) { path.startNewSubPath (x, y); st = true; } else path.lineTo (x, y);
        }
        const auto c = bandCol (k); const bool focus = k == sel || k == hover || k == popBand;
        if (focus) { juce::Path fp = path; fp.lineTo (p.getRight(), yOf (0)); fp.lineTo (p.getX(), yOf (0)); fp.closeSubPath(); g.setColour (c.withAlpha (0.14f)); g.fillPath (fp); }
        g.setColour (c.withAlpha (focus ? 0.95f : 0.4f)); g.strokePath (path, juce::PathStrokeType (focus ? 1.6f : 1.0f));
    }
    { int i = 0; bool st = false; for (float x = p.getX(); x < p.getRight() && i < NX; x += 2, ++i) { const float y = yOf ((float) juce::jlimit (-24.0, 24.0, sum[(size_t) i])); if (! st) { tot.startNewSubPath (x, y); st = true; } else tot.lineTo (x, y); } }
    g.setColour (C::phos.withAlpha (eqOn ? 0.25f : 0.08f)); g.strokePath (tot, juce::PathStrokeType (5.0f));
    g.setColour (glass (eqOn ? 0.95f : 0.3f)); g.strokePath (tot, juce::PathStrokeType (1.8f));
    const int so = ed.proc.eqSolo.load();
    for (int k = 0; k < kEqBands; ++k)
    {
        if (val (k, "on") < 0.5f) continue;
        const auto n = nodeOf (k); const auto c = bandCol (k); const bool focus = k == sel || k == hover || k == popBand; const int t = (int) val (k, "type");
        if (val (k, "dyn") > 0.5f && jmx::hasGain (t)) { const float y1 = yOf (val (k, "g") + val (k, "rng")); g.setColour (c.withAlpha (0.55f)); g.drawLine (n.x, n.y, n.x, y1, 2.0f); g.fillEllipse (n.x - 3, y1 - 3, 6, 6); }
        g.setColour (c.withAlpha (focus ? 0.45f : 0.2f)); g.fillEllipse (n.x - 11, n.y - 11, 22, 22);
        g.setColour (c); g.drawEllipse (n.x - 9, n.y - 9, 18, 18, focus ? 2.2f : 1.3f);
        g.setFont (neo::num (9.0f)); g.setColour (juce::Colours::white); g.drawText (juce::String (k + 1), juce::Rectangle<float> (n.x - 9, n.y - 8, 18, 16), juce::Justification::centred);
        juce::String tags; const int ch = (int) val (k, "ch"); if (ch > 0) tags << juce::String ("MSLR")[ch - 1]; if (val (k, "dyn") > 0.5f) tags << " D"; if (so == k) tags << " SOLO";
        if (tags.isNotEmpty()) { g.setFont (neo::cap (7.5f)); g.setColour (c); g.drawText (tags.trim(), juce::Rectangle<float> (n.x + 11, n.y - 20, 60, 11), juce::Justification::centredLeft); }
    }
    if (! eqOn) { g.setFont (neo::f (12.0f)); g.setColour (C::amber); g.drawText ("EQ SPENTO: tocca un punto o fai doppio clic per accenderlo", p, juce::Justification::centredTop); }
}
void EqView::mouseMove (const juce::MouseEvent& e)
{
    const int h = hit (e.position);
    if (h != hover) { hover = h; repaint(); }
    if (h >= 0 && h != popBand) { popBand = h; updatePop(); }
    if (h >= 0) popHold = 90;
}
void EqView::mouseDown (const juce::MouseEvent& e)
{
    drag = hit (e.position);
    if (drag >= 0)
    {
        sel = drag; popBand = drag; updatePop();
        if (e.mods.isPopupMenu()) { bandMenu (drag); drag = -1; return; }
        ed.P (eqId (drag, "f"))->beginChangeGesture(); ed.P (eqId (drag, "g"))->beginChangeGesture();
    }
    else if (! e.mods.isPopupMenu()) { sel = -1; popBand = -1; updatePop(); }
}
void EqView::mouseDrag (const juce::MouseEvent& e)
{
    if (drag < 0) return;
    const int t = (int) val (drag, "type");
    setV (drag, "f", juce::jlimit (20.0f, 20000.0f, fOf (e.position.x)));
    if (jmx::hasGain (t)) setV (drag, "g", juce::jlimit (-18.0f, 18.0f, dbOf (e.position.y)));
    updatePop();
}
void EqView::mouseUp (const juce::MouseEvent&)
{
    if (drag >= 0) { ed.P (eqId (drag, "f"))->endChangeGesture(); ed.P (eqId (drag, "g"))->endChangeGesture(); }
    drag = -1;
}
void EqView::mouseDoubleClick (const juce::MouseEvent& e)
{
    const int h = hit (e.position);
    if (h >= 0) { setV (h, "on", 0); if (ed.proc.eqSolo.load() == h) ed.proc.eqSolo = -1; sel = popBand = -1; updatePop(); return; }
    for (int b = 0; b < kEqBands; ++b)
        if (val (b, "on") < 0.5f)
        {
            const float f = fOf (e.position.x); const int t = f < 35 ? jmx::BT_LCUT : f > 17000 ? jmx::BT_HCUT : jmx::BT_BELL;
            setV (b, "dyn", 0); setV (b, "type", (float) t); setV (b, "f", juce::jlimit (20.0f, 20000.0f, f)); setV (b, "q", t == jmx::BT_BELL ? 1.0f : 0.71f);
            setV (b, "g", t == jmx::BT_BELL ? juce::jlimit (-18.0f, 18.0f, dbOf (e.position.y)) : 0.0f); setV (b, "ch", 0);
            setV (b, "on", 1); sel = popBand = b; updatePop(); return;
        }
}
void EqView::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    const int h = hit (e.position); if (h < 0) return;
    auto* p = ed.P (eqId (h, "q")); p->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, p->getValue() + (w.deltaY > 0 ? 0.02f : -0.02f))); updatePop();
}
void EqView::bandMenu (int b)
{
    juce::PopupMenu m, tm, cm, sm; const int t = (int) val (b, "type"), ch = (int) val (b, "ch"), sl = (int) val (b, "slope");
    for (int i = 0; i < eqTypeNames().size(); ++i) tm.addItem (100 + i, eqTypeNames()[i], true, i == t);
    for (int i = 0; i < eqChNames().size(); ++i) cm.addItem (200 + i, eqChNames()[i], true, i == ch);
    static const char* sn[4] = { "12 dB/ott", "24 dB/ott", "48 dB/ott", "96 dB/ott" };
    for (int i = 0; i < 4; ++i) sm.addItem (300 + i, sn[i], true, i == sl);
    m.addSectionHeader ("BANDA " + juce::String (b + 1));
    m.addSubMenu ("Tipo", tm); m.addSubMenu ("Canale", cm); m.addSubMenu ("Pendenza (tagli)", sm, t == jmx::BT_LCUT || t == jmx::BT_HCUT);
    m.addItem (1, "Dinamica", jmx::hasGain (t), val (b, "dyn") > 0.5f);
    m.addItem (3, "Solo (ascolta la zona)", true, ed.proc.eqSolo.load() == b);
    m.addSeparator(); m.addItem (4, "Spegni la banda");
    juce::Component::SafePointer<EqView> sp (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [sp, b] (int r)
    {
        if (sp == nullptr || r <= 0) return;
        auto& v = *sp;
        if (r >= 300) v.setV (b, "slope", (float) (r - 300));
        else if (r >= 200) v.setV (b, "ch", (float) (r - 200));
        else if (r >= 100) v.setV (b, "type", (float) (r - 100));
        else if (r == 1) v.setV (b, "dyn", v.val (b, "dyn") > 0.5f ? 0.0f : 1.0f);
        else if (r == 3) v.ed.proc.eqSolo = v.ed.proc.eqSolo.load() == b ? -1 : b;
        else if (r == 4) { v.setV (b, "on", 0); v.sel = v.popBand = -1; }
        v.updatePop(); v.repaint();
    });
}

// finestrella della banda: tipo, canale, SOLO, DINAMICA e i valori trascinabili
enum PopId { PI_TYPE = 1, PI_CH, PI_SLOPE, PI_SOLO, PI_DYN, PI_DEL, PI_F = 20, PI_G, PI_Q, PI_THR, PI_RANGE };
void EqPop::show (int b) { band = b; setSize (296, jmx::hasGain ((int) view.val (b, "type")) ? 172 : 112); }
void EqPop::paint (juce::Graphics& g)
{   // scheda compatta: in alto tipo / canale / solo, sotto tre manopoline (frequenza, guadagno, Q), poi la dinamica sempre pronta
    if (band < 0) return;
    const int b = band; auto v = [this, b] (const char* id) { return view.val (b, id); };
    const auto c = bandCol (b); auto r = getLocalBounds().toFloat();
    g.setColour (veil (0.5f)); g.fillRoundedRectangle (r.translated (0, 4), 12);
    g.setColour ((neo::light() ? juce::Colour (0xfaf6f4f9) : juce::Colour (0xf40e1019))); g.fillRoundedRectangle (r, 12); g.setColour (c.withAlpha (0.7f)); g.drawRoundedRectangle (r.reduced (0.5f), 12, 1.2f);
    hots.clear();
    const int t = (int) v ("type"); const bool hg = jmx::hasGain (t);
    auto btn = [&] (juce::Rectangle<float> rr, const juce::String& s, bool on, juce::Colour col, int id) { neo::chip (g, rr, s, on, col, 7.5f); hots.push_back ({ rr, id }); };
    g.setFont (neo::num (13.0f)); g.setColour (c); g.drawText (juce::String (b + 1), juce::Rectangle<float> (8, 8, 18, 22), juce::Justification::centred);
    btn ({ 30, 8, 92, 22 }, eqTypeNames()[t] + juce::String::fromUTF8 (" \xe2\x96\xbe"), true, c, PI_TYPE);
    btn ({ 126, 8, 74, 22 }, eqChNames()[(int) v ("ch")] + juce::String::fromUTF8 (" \xe2\x96\xbe"), v ("ch") > 0.5f, c, PI_CH);
    btn ({ 204, 8, 54, 22 }, "SOLO", view.ed.proc.eqSolo.load() == b, C::amber, PI_SOLO);
    btn ({ r.getRight() - 30, 8, 22, 22 }, juce::String::fromUTF8 ("\xc3\x97"), false, C::red, PI_DEL);
    // manopolina: arco del valore, nome sopra, numero sotto
    auto dial = [&] (float cx, float cy, const juce::String& lab, const juce::String& txt, float norm, bool bip, bool en, juce::Colour col, int id)
    {
        const float R = 15, a0 = -2.36f, a1 = 2.36f; auto rr = juce::Rectangle<float> (cx - 34, cy - 30, 68, 62);
        g.setFont (neo::cap (6.5f)); g.setColour (C::dim); g.drawText (lab, juce::Rectangle<float> (cx - 34, cy - 30, 68, 10), juce::Justification::centred);
        juce::Path tr; tr.addCentredArc (cx, cy, R, R, 0, a0, a1, true);
        g.setColour (glass (0.08f)); g.strokePath (tr, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        if (en)
        {
            const float av = a0 + (a1 - a0) * juce::jlimit (0.0f, 1.0f, norm), as = bip ? 0.0f : a0;
            juce::Path va; va.addCentredArc (cx, cy, R, R, 0, juce::jmin (as, av), juce::jmax (as, av), true);
            g.setColour (col.withAlpha (0.3f)); g.strokePath (va, juce::PathStrokeType (6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setColour (col); g.strokePath (va, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setColour ((neo::light() ? juce::Colour (0xffe4e1ea) : juce::Colour (0xff1a1d28))); g.fillEllipse (cx - R + 5, cy - R + 5, (R - 5) * 2, (R - 5) * 2);
            g.setColour (C::text); g.drawLine (cx + std::sin (av) * (R - 9), cy - std::cos (av) * (R - 9), cx + std::sin (av) * (R - 4), cy - std::cos (av) * (R - 4), 1.6f);
            hots.push_back ({ rr, id });
        }
        g.setFont (neo::num (9.5f)); g.setColour (en ? C::text : C::faint); g.drawText (txt, juce::Rectangle<float> (cx - 34, cy + R + 2, 68, 12), juce::Justification::centred);
    };
    auto nv = [&] (const char* p) { return view.ed.P (eqId (b, p))->getValue(); };
    const float y1 = 66;
    dial (52, y1, "FREQUENZA", hzT (v ("f")), nv ("f"), false, true, c, PI_F);
    dial (148, y1, "GUADAGNO", hg ? dbS (v ("g")) + " dB" : juce::String ("--"), nv ("g"), true, hg, c, PI_G);
    dial (244, y1, "Q", juce::String (v ("q"), 2), nv ("q"), false, true, c, PI_Q);
    if (! hg)
    {
        if (t == jmx::BT_LCUT || t == jmx::BT_HCUT) { static const char* sn[4] = { "12 dB/OTT", "24 dB/OTT", "48 dB/OTT", "96 dB/OTT" }; btn ({ 98, 98, 100, 22 }, sn[juce::jlimit (0, 3, (int) v ("slope"))], false, c, PI_SLOPE); }
        return;
    }
    // dinamica: sempre li'. Basta girare SOGLIA o RANGE e la banda diventa dinamica (RANGE a zero = statica)
    const bool dy = v ("dyn") > 0.5f; const auto dc = C::green;
    g.setColour (glass (0.06f)); g.fillRect (12.0f, 100.0f, r.getWidth() - 24, 1.0f);
    dial (52, 136, "SOGLIA", juce::String (v ("thr"), 1) + " dB", nv ("thr"), false, true, dy ? dc : C::dim, PI_THR);
    dial (148, 136, "RANGE", dbS (v ("rng")) + " dB", nv ("rng"), true, true, dy ? dc : C::dim, PI_RANGE);
    // quanto si sta muovendo adesso
    auto mr = juce::Rectangle<float> (204, 108, 82, 46);
    g.setColour (veil (0.4f)); g.fillRoundedRectangle (mr, 6);
    g.setFont (neo::cap (6.5f)); g.setColour (dy ? dc : C::faint); g.drawText ("DINAMICA", mr.withHeight (12).translated (0, 3), juce::Justification::centred);
    const float mv = dy ? view.ed.proc.eqDyn[b].load() : 0.0f, rg = std::abs (v ("rng")) + 0.01f;
    auto bar = mr.reduced (8, 0).withHeight (5).withY (mr.getY() + 34);
    g.setColour (glass (0.08f)); g.fillRoundedRectangle (bar, 3);
    const float fr = juce::jlimit (0.0f, 1.0f, std::abs (mv) / rg);
    g.setColour (dc); g.fillRoundedRectangle (bar.withWidth (bar.getWidth() * fr), 3);
    g.setFont (neo::num (9.0f)); g.setColour (dy ? C::text : C::faint); g.drawText (dy ? dbS (mv) + " dB" : juce::String ("statica"), mr.withHeight (14).translated (0, 16), juce::Justification::centred);
}
int EqPop::hit (juce::Point<float> p) const { for (auto& h : hots) if (h.r.contains (p)) return h.id; return 0; }
static const char* popParam (int id) { switch (id) { case PI_F: return "f"; case PI_G: return "g"; case PI_Q: return "q"; case PI_THR: return "thr"; case PI_RANGE: return "rng"; default: return nullptr; } }
void EqPop::mouseDown (const juce::MouseEvent& e)
{
    if (band < 0) return;
    const int b = band; const int id = hit (e.position);
    auto menu = [&] (const juce::StringArray& items, const char* p)
    {
        juce::PopupMenu m; const int cur = (int) view.val (b, p); for (int i = 0; i < items.size(); ++i) m.addItem (i + 1, items[i], true, i == cur);
        juce::Component::SafePointer<EqView> sv (&view); const juce::String pid (p);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [sv, b, pid] (int x) { if (x > 0 && sv) { sv->setV (b, pid.toRawUTF8(), (float) (x - 1)); sv->updatePop(); } });
    };
    switch (id)
    {
        case PI_TYPE: menu (eqTypeNames(), "type"); break;
        case PI_CH: menu (eqChNames(), "ch"); break;
        case PI_SLOPE: menu ({ "12 dB/ott", "24 dB/ott", "48 dB/ott", "96 dB/ott" }, "slope"); break;
        case PI_SOLO: view.ed.proc.eqSolo = view.ed.proc.eqSolo.load() == b ? -1 : b; break;
        case PI_DYN: view.setV (b, "dyn", view.val (b, "dyn") > 0.5f ? 0.0f : 1.0f); break;
        case PI_DEL: view.setV (b, "on", 0); if (view.ed.proc.eqSolo.load() == b) view.ed.proc.eqSolo = -1; view.sel = view.popBand = -1; break;
        default:
            if (auto* p = popParam (id)) { dragK = id; dragStart = view.ed.P (eqId (b, p))->getValue(); view.ed.P (eqId (b, p))->beginChangeGesture(); }
    }
    view.updatePop(); repaint(); view.repaint();
}
void EqPop::mouseDrag (const juce::MouseEvent& e)
{
    if (dragK < 0 || band < 0) return;
    if (auto* p = popParam (dragK))
    {
        auto* pr = view.ed.P (eqId (band, p)); pr->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, dragStart - e.getDistanceFromDragStartY() / (e.mods.isShiftDown() ? 800.0f : 200.0f)));
        if (dragK == PI_THR || dragK == PI_RANGE) view.setV (band, "dyn", std::abs (view.val (band, "rng")) > 0.05f ? 1.0f : 0.0f);   // girare la dinamica la accende, RANGE a zero la spegne
        repaint(); view.repaint();
    }
}
void EqPop::mouseDoubleClick (const juce::MouseEvent& e)
{
    if (band < 0) return;
    if (auto* p = popParam (hit (e.position))) { auto* pr = view.ed.P (eqId (band, p)); pr->setValueNotifyingHost (pr->getDefaultValue()); repaint(); view.repaint(); }
}
void EqPop::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    if (band < 0) return;
    const int id = hit (e.position);
    if (auto* p = popParam (id))
    {
        auto* pr = view.ed.P (eqId (band, p)); pr->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, pr->getValue() + (w.deltaY > 0 ? 0.01f : -0.01f)));
        if (id == PI_THR || id == PI_RANGE) view.setV (band, "dyn", std::abs (view.val (band, "rng")) > 0.05f ? 1.0f : 0.0f);
        repaint(); view.repaint();
    }
}

// ======================================================================= IMAGER
static juce::Colour imCol (int b) { static const juce::uint32 c[kImBands] = { 0xffffb000, 0xffff2e88, 0xff3dffd2, 0xff5ef2ff }; return juce::Colour (c[b]); }
ImagerPage::ImagerPage (Editor& e) : ModPage (e, IMAGER, "im_on", "im_delta"), view (e), gonio (e)
{
    auto& P = ed.proc;
    for (int b = 0; b < kImBands; ++b)
    {
        const juce::String w = "im_w" + juce::String (b + 1), st = "im_s" + juce::String (b + 1);
        knob (w.toRawUTF8(), imCol (b), "LARGHEZZA", [&P, w] { return juce::String (juce::roundToInt (P.value (w.toRawUTF8()) * 100)) + "%"; });
        knob (st.toRawUTF8(), C::violet, "STEREOIZE", [&P, st] { return juce::String (juce::roundToInt (P.value (st.toRawUTF8()) * 100)) + "%"; });
        solo[b] = std::make_unique<Switch> (ed.P ("im_solo" + juce::String (b + 1)), C::amber, "SOLO"); addAndMakeVisible (*solo[b]);
    }
    smode = std::make_unique<Segs> (ed.P ("im_smode"), C::violet, juce::StringArray { "MODO I (ritardo)", "MODO II (diffuso)" }); smode->font = 10.0f;
    mono = std::make_unique<Switch> (ed.P ("im_mono"), C::amber, "ASCOLTA IN MONO");
    safe = std::make_unique<Switch> (ed.P ("im_safe"), C::green, "SICUREZZA MONO");
    addAndMakeVisible (*smode); addAndMakeVisible (*mono); addAndMakeVisible (*safe); addAndMakeVisible (view); addAndMakeVisible (gonio);
}
void ImagerPage::resized()
{
    on->setBounds (130, 12, 100, 22); delta->setBounds (246, 12, 230, 22);
    view.setBounds (16, 48, 560, 300); gonio.setBounds (588, 48, 250, 300);
    for (int b = 0; b < kImBands; ++b)
    {
        const int x0 = 16 + b * 143;
        solo[b]->setBounds (x0 + 64, 360, 76, 20);
        knobs[b * 2]->setBounds (x0, 386, 68, 68 + 27); knobs[b * 2 + 1]->setBounds (x0 + 70, 386, 68, 68 + 27);
    }
    smode->setBounds (16, 500, 300, 26); mono->setBounds (330, 502, 170, 22); safe->setBounds (510, 502, 170, 22);
}
void ImagerPage::paint (juce::Graphics& g)
{
    paintHeader (g, "Trascina le barre per allargare o stringere ogni banda (100% = com'e) e le linee per spostare i tagli. Tieni i bassi stretti (0-60%). SOLO per sentire una banda, DELTA per sentire cosa cambia.");
    static const char* names[kImBands] = { "BASSI", "MEDIO-BASSI", "MEDI", "ALTI" };
    for (int b = 0; b < kImBands; ++b)
    {
        const float x0 = 16.0f + b * 143;
        g.setColour (glass (0.05f)); g.fillRoundedRectangle (x0 - 3, 356, 140, 136, 8);
        glowText (g, names[b], { x0 + 2, 360, 70, 20 }, imCol (b), uf (11.5f));
    }
    g.setColour (C::dim); g.setFont (mf (9.0f)); g.drawText ("STEREOIZE", juce::Rectangle<float> (16, 486, 120, 12), juce::Justification::centredLeft);
    auto r = juce::Rectangle<float> (588, 356, 250, 172); lcd (g, r, 8);
    auto t = r.reduced (10, 8);
    glowText (g, "CORRELAZIONE", t.removeFromTop (14), C::green, uf (11.0f));
    g.setColour (C::text.withAlpha (0.8f)); g.setFont (uf (10.0f, false));
    g.drawFittedText ("Quanto destra e sinistra sono uguali. +1 = mono, 0 = molto largo, sotto 0 = fasi opposte: in mono quella parte sparisce. Sul master stai sopra 0.3, i bassi vicino a +1.",
                      t.removeFromTop (62).toNearestInt(), juce::Justification::topLeft, 5);
    glowText (g, "STEREOIZE", t.removeFromTop (14), C::violet, uf (11.0f));
    g.setColour (C::text.withAlpha (0.8f));
    g.drawFittedText ("Crea larghezza anche da materiale mono aggiungendo ai lati una copia del centro: MODO I la ritarda di pochi ms (aperto e definito), MODO II la diffonde (piu morbido). In mono sparisce.",
                      t.toNearestInt(), juce::Justification::topLeft, 6);
}
float ImView::xb (int k) const { return xOf (ed.proc.value (k == 0 ? "im_x1" : k == 1 ? "im_x2" : "im_x3")); }
void ImView::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat(); lcd (g, b, 8);
    auto& P = ed.proc; const bool on = P.value ("im_on") > 0.5f;
    const float top = 30, bot = b.getHeight() - 40, mid = top + (bot - top) * 0.5f;
    an.run (P.ringOut, sp, 200, 20, 20000, P.sampleRate());
    drawSpec (g, sp, 200, juce::Rectangle<float> (0, top, b.getWidth(), bot - top), C::dim.withAlpha (0.35f), true, -100, 0);
    g.setFont (mf (8.5f));
    for (float f : { 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f })
    { g.setColour (C::phos.withAlpha (0.06f)); g.drawVerticalLine ((int) xOf (f), top, bot); g.setColour (C::dim.withAlpha (0.7f)); g.drawText (f >= 1000 ? juce::String ((int) (f / 1000)) + "k" : juce::String ((int) f), juce::Rectangle<float> (xOf (f) - 16, bot + 2, 32, 11), juce::Justification::centred); }
    g.setColour (C::dim.withAlpha (0.5f)); for (float x = 4; x < b.getRight(); x += 6) g.drawHorizontalLine ((int) mid, x, x + 2);
    g.drawText ("100%", juce::Rectangle<float> (4, mid - 14, 40, 12), juce::Justification::centredLeft);
    float xs[5] = { 0, xb (0), xb (1), xb (2), b.getWidth() };
    for (int k = 0; k < kImBands; ++k)
    {
        const float w = P.value (("im_w" + juce::String (k + 1)).toRawUTF8()), yw = mid - (w - 1) * (bot - top) * 0.5f;
        auto band = juce::Rectangle<float> (xs[k] + 3, juce::jmin (yw, mid), xs[k + 1] - xs[k] - 6, std::abs (yw - mid) + 2);
        const auto c = on ? imCol (k) : C::faint;
        juce::ColourGradient cg (c.withAlpha (0.45f), 0, band.getY(), c.withAlpha (0.12f), 0, band.getBottom(), false); g.setGradientFill (cg); g.fillRect (band);
        g.setColour (c); g.fillRect (xs[k] + 3, yw - 1.5f, xs[k + 1] - xs[k] - 6, 3.0f);
        glowText (g, juce::String (juce::roundToInt (w * 100)) + "%", { xs[k], yw + (w >= 1 ? -18.0f : 4.0f), xs[k + 1] - xs[k], 14 }, c, mf (10.5f), juce::Justification::centred);
        // correlazione della banda
        const float cr = P.imCorr[k].load();
        auto cb = juce::Rectangle<float> (xs[k] + 8, bot + 16, xs[k + 1] - xs[k] - 16, 8);
        g.setColour (glass (0.06f)); g.fillRoundedRectangle (cb, 3);
        const float cx = cb.getX() + (cr + 1) * 0.5f * cb.getWidth();
        g.setColour (cr < 0 ? C::red : cr < 0.3f ? C::amber : C::green); g.fillEllipse (cx - 4, cb.getY(), 8, 8);
        if (P.imLim[k].load() > 0.05f) { g.setFont (neo::cap (6.5f)); g.setColour (C::green); g.drawText ("SICUREZZA -" + juce::String (juce::roundToInt (P.imLim[k].load() * 100)) + "%", juce::Rectangle<float> (xs[k], bot + 26, xs[k + 1] - xs[k], 12), juce::Justification::centred); }
    }
    for (int k = 0; k < 3; ++k)
    {
        const float x = xs[k + 1]; g.setColour ((on ? C::text : C::faint).withAlpha (drag == k ? 1.0f : 0.6f)); g.fillRect (x - 1, top - 10, 2.0f, bot - top + 10);
        glowText (g, hzT (P.value (k == 0 ? "im_x1" : k == 1 ? "im_x2" : "im_x3")), { x - 40, 6, 80, 14 }, on ? C::text : C::dim, mf (9.5f), juce::Justification::centred);
    }
    scanlines (g, b);
}
void ImView::mouseDown (const juce::MouseEvent& e)
{
    drag = -1;
    for (int k = 0; k < 3; ++k) if (std::abs (e.position.x - xb (k)) < 8) drag = k;
    if (drag < 0) { float xs[5] = { 0, xb (0), xb (1), xb (2), (float) getWidth() }; for (int k = 0; k < kImBands; ++k) if (e.position.x >= xs[k] && e.position.x < xs[k + 1]) drag = 10 + k; }
    if (auto* page = dynamic_cast<ImagerPage*> (getParentComponent())) page->touch();
    mouseDrag (e);
}
void ImView::mouseDrag (const juce::MouseEvent& e)
{
    if (drag < 0) return;
    if (drag < 3)
    {
        auto* p = ed.P (drag == 0 ? "im_x1" : drag == 1 ? "im_x2" : "im_x3");
        float f = fOf (e.position.x);
        if (drag > 0) f = juce::jmax (f, ed.proc.value (drag == 1 ? "im_x1" : "im_x2") * 1.5f);
        if (drag < 2) f = juce::jmin (f, ed.proc.value (drag == 0 ? "im_x2" : "im_x3") / 1.5f);
        p->setValueNotifyingHost (p->convertTo0to1 (f));
    }
    else
    {
        const float top = 30, bot = getHeight() - 40.0f, mid = top + (bot - top) * 0.5f;
        const float w = juce::jlimit (0.0f, 2.0f, 1.0f + (mid - e.position.y) / ((bot - top) * 0.5f));
        auto* p = ed.P ("im_w" + juce::String (drag - 9)); p->setValueNotifyingHost (p->convertTo0to1 (w));
    }
    repaint();
}
void ImView::mouseUp (const juce::MouseEvent&) { drag = -1; }
void Gonio::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat(); lcd (g, b, 8);
    auto& P = ed.proc; auto sq = juce::Rectangle<float> (b.getX() + 10, b.getY() + 10, b.getWidth() - 20, b.getWidth() - 20);
    const float cx = sq.getCentreX(), cy = sq.getCentreY(), R = sq.getWidth() * 0.5f;
    g.setColour (C::phos.withAlpha (0.12f)); g.drawLine (cx, sq.getY(), cx, sq.getBottom()); g.drawLine (sq.getX(), cy, sq.getRight(), cy);
    g.drawLine (cx - R * 0.707f, cy - R * 0.707f, cx + R * 0.707f, cy + R * 0.707f); g.drawLine (cx + R * 0.707f, cy - R * 0.707f, cx - R * 0.707f, cy + R * 0.707f);
    g.setColour (C::dim); g.setFont (mf (8.5f)); g.drawText ("M", juce::Rectangle<float> (cx - 6, sq.getY(), 12, 10), juce::Justification::centred);
    g.drawText ("L", juce::Rectangle<float> (sq.getX() + 8, sq.getY() + 8, 12, 10), juce::Justification::centred); g.drawText ("R", juce::Rectangle<float> (sq.getRight() - 20, sq.getY() + 8, 12, 10), juce::Justification::centred);
    // nuvola con persistenza (si dissolve piano) e scala che cambia lentamente: niente sfarfallio
    const int TW = juce::roundToInt (sq.getWidth()), TH = juce::roundToInt (sq.getHeight());
    if (trail.isNull() || trail.getWidth() != TW) trail = juce::Image (juce::Image::ARGB, TW, TH, true);
    const double now = juce::Time::getMillisecondCounterHiRes();
    if (now - lastMs > 25)
    {
        lastMs = now;
        float l[2048], r[2048]; P.ringOut.read (l, r, 2048);
        float mx = 0.05f; for (int i = 0; i < 2048; ++i) mx = juce::jmax (mx, std::abs (l[i]), std::abs (r[i]));
        scale = mx > scale ? scale + (mx - scale) * 0.3f : scale + (mx - scale) * 0.02f;
        juce::Image::BitmapData bd (trail, juce::Image::BitmapData::readWrite);
        for (int y = 0; y < TH; ++y) for (int x = 0; x < TW; ++x) { auto c = bd.getPixelColour (x, y); if (c.getAlpha() > 0) bd.setPixelColour (x, y, c.withMultipliedAlpha (0.82f)); }
        const float sc = TW * 0.45f / juce::jmax (0.05f, scale);
        const auto gc = C::green.withAlpha (0.55f);
        for (int i = 0; i < 2048; i += 2)
        {
            const int px = juce::roundToInt (TW * 0.5f + (l[i] - r[i]) * 0.7071f * sc), py = juce::roundToInt (TH * 0.5f - (l[i] + r[i]) * 0.7071f * sc);
            if (px >= 0 && py >= 0 && px < TW && py < TH) bd.setPixelColour (px, py, gc);
        }
    }
    g.drawImageAt (trail, juce::roundToInt (sq.getX()), juce::roundToInt (sq.getY()));
    // correlazione
    auto cb = juce::Rectangle<float> (b.getX() + 14, sq.getBottom() + 22, b.getWidth() - 28, 10);
    g.setColour (C::dim); g.drawText ("-1", juce::Rectangle<float> (cb.getX(), cb.getY() - 14, 20, 12), juce::Justification::centredLeft);
    g.drawText ("+1", juce::Rectangle<float> (cb.getRight() - 20, cb.getY() - 14, 20, 12), juce::Justification::centredRight);
    g.drawText ("CORRELAZIONE", juce::Rectangle<float> (cb.getX(), cb.getY() - 14, cb.getWidth(), 12), juce::Justification::centred);
    g.setColour (glass (0.06f)); g.fillRoundedRectangle (cb, 3);
    crS += (juce::jlimit (-1.0f, 1.0f, P.corrAll.load()) - crS) * 0.08f;
    const float cr = crS, x = cb.getX() + (cr + 1) * 0.5f * cb.getWidth();
    g.setColour (cr < 0 ? C::red : cr < 0.3f ? C::amber : C::green); g.fillRoundedRectangle (juce::Rectangle<float> (cb.getCentreX(), cb.getY(), x - cb.getCentreX(), cb.getHeight()).withLeft (juce::jmin (x, cb.getCentreX())).withRight (juce::jmax (x, cb.getCentreX())), 3);
    glowText (g, juce::String (cr, 2), { cb.getX(), cb.getBottom() + 2, cb.getWidth(), 14 }, cr < 0 ? C::red : C::text, mf (10.5f), juce::Justification::centred);
    scanlines (g, b);
}

// ======================================================================= DE-ESSER
DeEssPage::DeEssPage (Editor& e) : ModPage (e, DEESS, "ds_on", "ds_delta"), view (e)
{
    auto& P = ed.proc; const auto c = C::green;
    knob ("ds_lo", c, "DA", [&P] { return hzT (P.value ("ds_lo")); });
    knob ("ds_hi", c, "A", [&P] { const float v = P.value ("ds_hi"); return v >= 19000 ? juce::String ("FINO IN CIMA") : hzT (v); });
    knob ("ds_thr", c, "SOGLIA", [&P] { const float v = P.value ("ds_thr"); return v > 5.9f ? juce::String ("OFF") : juce::String (v, 1) + " dB"; });
    knob ("ds_amt", C::amber, "QUANTITA", [&P] { return juce::String (juce::roundToInt (P.value ("ds_amt") * 100)) + "%"; });
    knob ("ds_soft", C::phos, "MORBIDEZZA", [&P] { return juce::String (juce::roundToInt (P.value ("ds_soft") * 100)) + "%"; });
    ch = std::make_unique<Segs> (ed.P ("ds_ch"), c, juce::StringArray { "STEREO", "MID (voce)", "SIDE (piatti)" });
    addAndMakeVisible (*ch); addAndMakeVisible (view);
}
void DeEssPage::resized()
{
    on->setBounds (160, 12, 100, 22); delta->setBounds (276, 12, 230, 22);
    view.setBounds (16, 48, 822, 300);
    for (int i = 0; i < knobs.size(); ++i) knobs[i]->setBounds (16 + i * 104, 372, 96, 96 + 27);
    ch->setBounds (560, 410, 278, 28);
}
void DeEssPage::paint (juce::Graphics& g)
{
    paintHeader (g, "Scegli l'intervallo DA..A (o da DA fino in cima) trascinando i bordi nello spettro. Quando quell'intervallo sporge oltre la SOGLIA rispetto al resto del mix, viene abbassato "
                    "con dolcezza: QUANTITA dice quanto (max circa 10 dB), MORBIDEZZA rende l'intervento piu lento e graduale. DELTA: devi sentire solo S, Z e piatti duri.");
    g.setColour (C::dim); g.setFont (mf (9.0f)); g.drawText ("DOVE LAVORA", juce::Rectangle<float> (560, 394, 200, 12), juce::Justification::centredLeft);
}
void DsView::mouseDrag (const juce::MouseEvent& e)
{
    auto r = getLocalBounds().toFloat().reduced (14, 10).withTrimmedRight (110);
    const float f = 1000.0f * std::pow (20.0f, juce::jlimit (0.0f, 1.0f, (e.position.x - r.getX()) / r.getWidth()));
    const float lo = ed.proc.value ("ds_lo"), hi = ed.proc.value ("ds_hi");
    const bool moveHi = std::abs (std::log (f / hi)) < std::abs (std::log (f / lo));
    auto* p = ed.P (moveHi ? "ds_hi" : "ds_lo"); p->setValueNotifyingHost (p->convertTo0to1 (moveHi ? juce::jmax (f, lo * 1.3f) : juce::jmin (f, hi / 1.3f)));
    if (auto* pg = dynamic_cast<DeEssPage*> (getParentComponent())) pg->touch();
}
void DsView::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat(); lcd (g, b, 8);
    auto& P = ed.proc; auto r = b.reduced (14, 10).withTrimmedBottom (40).withTrimmedRight (110);
    an.run (P.ringOut, sp, 200, 1000, 20000, P.sampleRate());
    auto x = [&] (float f) { return r.getX() + (float) (std::log (f / 1000.0) / std::log (20.0)) * r.getWidth(); };
    g.setFont (mf (8.5f));
    for (float f : { 2000.0f, 3000.0f, 5000.0f, 7000.0f, 10000.0f, 15000.0f }) { g.setColour (C::phos.withAlpha (0.07f)); g.drawVerticalLine ((int) x (f), r.getY(), r.getBottom()); g.setColour (C::dim); g.drawText (juce::String ((int) (f / 1000)) + "k", juce::Rectangle<float> (x (f) - 14, r.getBottom() + 2, 28, 11), juce::Justification::centred); }
    const float lo = P.value ("ds_lo"), hi = P.value ("ds_hi"), fx = x (lo), fx2 = hi >= 19000 ? r.getRight() : x (hi);
    g.setColour (C::green.withAlpha (0.10f)); g.fillRect (juce::Rectangle<float> (fx, r.getY(), fx2 - fx, r.getHeight()));
    drawSpec (g, sp, 200, r, C::green, true, -110, 10);
    g.setColour (C::green); g.fillRect (fx - 1, r.getY(), 2.0f, r.getHeight()); if (hi < 19000) g.fillRect (fx2 - 1, r.getY(), 2.0f, r.getHeight());
    glowText (g, hzT (lo) + (hi >= 19000 ? juce::String::fromUTF8 (" \xe2\x86\x92 in su") : juce::String::fromUTF8 (" \xe2\x80\x93 ") + hzT (hi)), { fx + 6, r.getY() + 4, 200, 14 }, C::green, mf (10.0f));
    // quanto sporgono le sibilanti rispetto alla soglia
    auto bar = juce::Rectangle<float> (r.getX(), b.getBottom() - 28, r.getWidth(), 12);
    const float rel = P.dsRel.load(), thr = P.value ("ds_thr");
    auto bx = [&] (float db) { return bar.getX() + juce::jlimit (0.0f, 1.0f, (db + 40) / 46) * bar.getWidth(); };
    g.setColour (glass (0.06f)); g.fillRoundedRectangle (bar, 3);
    g.setColour (rel > thr ? C::red : C::green.withAlpha (0.7f)); g.fillRoundedRectangle (bar.withRight (bx (rel)), 3);
    g.setColour (C::amber); g.fillRect (bx (thr) - 1, bar.getY() - 4, 2.0f, bar.getHeight() + 8);
    g.setColour (C::dim); g.setFont (mf (8.5f)); g.drawText ("SIBILANTI RISPETTO AL MIX", juce::Rectangle<float> (bar.getX(), bar.getY() - 14, 200, 12), juce::Justification::centredLeft);
    // riduzione nel tempo
    hist[hw] = P.dsGr.load(); hw = (hw + 1) % 160;
    auto hr = juce::Rectangle<float> (b.getRight() - 112, b.getY() + 10, 98, b.getHeight() - 36);
    g.setColour (veil (0.3f)); g.fillRoundedRectangle (hr, 4);
    juce::Path p; p.startNewSubPath (hr.getX(), hr.getY());
    for (int i = 0; i < 160; ++i) p.lineTo (hr.getX() + hr.getWidth() * i / 159.0f, hr.getY() + juce::jlimit (0.0f, 1.0f, hist[(hw + i) % 160] / 12.0f) * hr.getHeight());
    p.lineTo (hr.getRight(), hr.getY()); p.closeSubPath(); g.setColour (C::red.withAlpha (0.35f)); g.fillPath (p); g.setColour (C::red); g.strokePath (p, juce::PathStrokeType (1.0f));
    glowText (g, "-" + juce::String (P.dsGr.load(), 1) + " dB", { hr.getX(), hr.getBottom() - 18, hr.getWidth(), 14 }, C::red, mf (10.0f), juce::Justification::centred);
    g.setColour (C::dim); g.setFont (mf (8.5f)); g.drawText ("RIDUZIONE", juce::Rectangle<float> (hr.getX(), hr.getBottom() + 2, hr.getWidth(), 12), juce::Justification::centred);
    scanlines (g, b);
}

// ======================================================================= BUS COMP
BusPage::BusPage (Editor& e) : ModPage (e, BUSCOMP, "bus_on", "bus_delta"), view (e)
{
    auto& P = ed.proc; const auto c = C::violet;
    auto* t = knob ("bus_thr", c, "SOGLIA", [&P] { return juce::String (P.value ("bus_thr"), 1) + " dB"; }); t->big = true;
    knob ("bus_makeup", c, "MAKEUP", [&P] { return dbS (P.value ("bus_makeup")) + " dB"; });
    knob ("bus_mix", C::phos, "MIX", [&P] { return juce::String (juce::roundToInt (P.value ("bus_mix") * 100)) + "%"; });
    knob ("bus_range", C::red, "RANGE", [&P] { return juce::String (P.value ("bus_range"), 1) + " dB"; });
    knob ("bus_hpf", C::green, "SC HPF", [&P] { const float v = P.value ("bus_hpf"); return v < 21 ? juce::String ("OFF") : juce::String (juce::roundToInt (v)) + " Hz"; });
    ratio = std::make_unique<Segs> (ed.P ("bus_ratio"), c, juce::StringArray { "1.5", "2", "4", "10" });
    att = std::make_unique<Segs> (ed.P ("bus_att"), c, juce::StringArray { ".1", ".3", "1", "3", "10", "30" });
    rel = std::make_unique<Segs> (ed.P ("bus_rel"), c, juce::StringArray { ".1", ".3", ".6", "1.2", "AUTO" });
    autoM = std::make_unique<Switch> (ed.P ("bus_auto"), C::phos, "MAKEUP AUTOMATICO");
    for (juce::Component* x : std::initializer_list<juce::Component*> { ratio.get(), att.get(), rel.get(), autoM.get(), &view }) addAndMakeVisible (x);
}
void BusPage::resized()
{
    on->setBounds (130, 12, 100, 22); delta->setBounds (246, 12, 230, 22);
    view.setBounds (16, 48, 520, 300);
    ratio->setBounds (556, 76, 282, 30); att->setBounds (556, 146, 282, 30); rel->setBounds (556, 216, 282, 30); autoM->setBounds (556, 276, 240, 22);
    knobs[0]->setBounds (16, 364, 140, 140 + 32);
    for (int i = 1; i < knobs.size(); ++i) knobs[i]->setBounds (176 + (i - 1) * 108, 384, 100, 100 + 27);
}
void BusPage::paint (juce::Graphics& g)
{
    paintHeader (g, "Compressore da bus in stile console, morbido (rivelatore RMS). Parte da SOGLIA 0 = non tocca nulla: scendi finche la lancetta lavora 1-3 dB. RATIO 2, ATTACCO 10-30 ms, RILASCIO AUTO. RANGE limita la riduzione massima. "
                    "SC HPF a 60-120 Hz evita che la cassa e l'808 facciano pompare tutto. MIX sotto 100% = compressione parallela.");
    g.setColour (C::dim); g.setFont (mf (9.0f));
    g.drawText ("RATIO", juce::Rectangle<float> (556, 60, 200, 12), juce::Justification::centredLeft);
    g.drawText ("ATTACCO (ms)", juce::Rectangle<float> (556, 130, 200, 12), juce::Justification::centredLeft);
    g.drawText ("RILASCIO (s)", juce::Rectangle<float> (556, 200, 200, 12), juce::Justification::centredLeft);
}
void BusView::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat(); lcd (g, b, 8);
    auto& P = ed.proc; const float gr = P.value ("bus_on") > 0.5f ? P.busGr.load() : 0.0f;
    needle += (gr - needle) * (gr > needle ? 0.5f : 0.15f);
    // strumento ad ago
    auto vu = juce::Rectangle<float> (b.getX() + 20, b.getY() + 16, b.getWidth() - 40, b.getHeight() * 0.62f);
    juce::ColourGradient face (juce::Colour (0xfff3e2b8), vu.getCentreX(), vu.getY(), juce::Colour (0xffc9b483), vu.getCentreX(), vu.getBottom(), false);
    g.setGradientFill (face); g.fillRoundedRectangle (vu, 8);
    const float cx = vu.getCentreX(), cy = vu.getBottom() + vu.getHeight() * 0.35f, R = vu.getHeight() * 1.0f;
    auto ang = [] (float db) { return -0.85f + juce::jlimit (0.0f, 1.0f, db / 20.0f) * 1.7f; };   // 0..20 dB di riduzione, da destra a sinistra
    g.setFont (mf (10.0f));
    for (float db : { 0.0f, 2.0f, 4.0f, 6.0f, 10.0f, 14.0f, 20.0f })
    {
        const float a = -ang (db);
        const juce::Point<float> p0 (cx + std::sin (a) * R * 0.86f, cy - std::cos (a) * R * 0.86f), p1 (cx + std::sin (a) * R * 0.96f, cy - std::cos (a) * R * 0.96f);
        g.setColour (db >= 10 ? juce::Colour (0xffb3261e) : juce::Colour (0xff2a2a2a)); g.drawLine ({ p0, p1 }, 1.6f);
        g.drawText (juce::String ((int) db), juce::Rectangle<float> (p1.x - 14, p1.y - 16, 28, 12), juce::Justification::centred);
    }
    g.setColour (juce::Colour (0xff2a2a2a)); g.setFont (uf (11.0f)); g.drawText ("GAIN REDUCTION  dB", vu.withTrimmedTop (vu.getHeight() * 0.55f).withHeight (16), juce::Justification::centred);
    {
        juce::Graphics::ScopedSaveState ss (g); g.reduceClipRegion (vu.toNearestInt());
        const float a = -ang (needle);
        g.setColour (veil (0.25f)); g.drawLine (cx + 3, cy, cx + 3 + std::sin (a) * R * 0.95f, cy - std::cos (a) * R * 0.95f, 2.0f);
        g.setColour (juce::Colour (0xff111111)); g.drawLine (cx, cy, cx + std::sin (a) * R * 0.95f, cy - std::cos (a) * R * 0.95f, 1.8f);
    }
    
    // storia
    hist[hw] = gr; hw = (hw + 1) % 200;
    auto hr = juce::Rectangle<float> (b.getX() + 20, vu.getBottom() + 14, b.getWidth() - 40, b.getBottom() - vu.getBottom() - 30);
    g.setColour (veil (0.3f)); g.fillRoundedRectangle (hr, 4);
    juce::Path p; p.startNewSubPath (hr.getX(), hr.getY());
    for (int i = 0; i < 200; ++i) p.lineTo (hr.getX() + hr.getWidth() * i / 199.0f, hr.getY() + juce::jlimit (0.0f, 1.0f, hist[(hw + i) % 200] / 8.0f) * hr.getHeight());
    p.lineTo (hr.getRight(), hr.getY()); p.closeSubPath(); g.setColour (C::violet.withAlpha (0.35f)); g.fillPath (p); g.setColour (C::violet); g.strokePath (p, juce::PathStrokeType (1.1f));
    glowText (g, "-" + juce::String (gr, 1) + " dB", { hr.getRight() - 90, hr.getY() + 2, 86, 14 }, C::violet, mf (10.5f), juce::Justification::centredRight);
    scanlines (g, b);
}

// ======================================================================= MULTIBANDA
static juce::Colour mbCol (int b) { static const juce::uint32 c[kMbBands] = { 0xffffb000, 0xffff7a3d, 0xff5ef2ff }; return juce::Colour (c[b]); }
static const char* mbName (int b) { static const char* n[] = { "BASSI", "MEDI", "ALTI" }; return n[b]; }
MultiPage::MultiPage (Editor& e) : ModPage (e, MULTI, "mb_on", "mb_delta"), view (e)
{
    auto& P = ed.proc;
    for (int b = 0; b < kMbBands; ++b)
    {
        const juce::String id = "mb" + juce::String (b + 1) + "_"; const auto c = mbCol (b);
        knob ((id + "thr").toRawUTF8(), c, "SOGLIA", [&P, id] { return juce::String (P.value ((id + "thr").toRawUTF8()), 1); });
        knob ((id + "ratio").toRawUTF8(), c, "RAPPORTO", [&P, id] { return juce::String (P.value ((id + "ratio").toRawUTF8()), 1) + ":1"; });
        knob ((id + "up").toRawUTF8(), C::green, "SOLLEVA", [&P, id] { return juce::String (juce::roundToInt (P.value ((id + "up").toRawUTF8()) * 100)) + "%"; });
        knob ((id + "gain").toRawUTF8(), c, "GUADAGNO", [&P, id] { return dbS (P.value ((id + "gain").toRawUTF8())); }, true);
        solo[b] = std::make_unique<Switch> (ed.P (id + "solo"), C::amber, "SOLO"); addAndMakeVisible (*solo[b]);
    }
    knob ("mb_speed", C::phos, "VELOCITA", [&P] { const float v = P.value ("mb_speed"); return v < 0.35f ? juce::String ("LENTA") : v > 0.65f ? juce::String ("VELOCE") : juce::String ("MEDIA"); });
    knob ("mb_mix", C::phos, "MIX", [&P] { return juce::String (juce::roundToInt (P.value ("mb_mix") * 100)) + "%"; });
    ottBtn = std::make_unique<ActBtn> ("CARICA OTT", C::amber, [this] { ott(); });
    resetBtn = std::make_unique<ActBtn> ("AZZERA", C::phos, [this]
    {
        for (int b = 0; b < kMbBands; ++b) for (auto k : { "thr", "ratio", "up", "gain" }) { auto* p = ed.P ("mb" + juce::String (b + 1) + "_" + k); p->setValueNotifyingHost (p->getDefaultValue()); }
        auto* m = ed.P ("mb_mix"); m->setValueNotifyingHost (1.0f);
    });
    addAndMakeVisible (*ottBtn); addAndMakeVisible (*resetBtn); addAndMakeVisible (view);
}
void MultiPage::ott()
{
    // OTT: giu forte sopra soglia e su forte sotto, su tre bande, poi mix in parallelo
    auto set = [this] (const juce::String& id, float v) { auto* p = ed.P (id); p->setValueNotifyingHost (p->convertTo0to1 (v)); };
    const float thr[kMbBands] = { -22, -24, -26 }, gain[kMbBands] = { 3, 2, 4 };
    for (int b = 0; b < kMbBands; ++b)
    {
        const juce::String id = "mb" + juce::String (b + 1) + "_";
        set (id + "thr", thr[b]); set (id + "ratio", 12); set (id + "up", 0.85f); set (id + "gain", gain[b]);
    }
    set ("mb_x1", 88); set ("mb_x2", 2500); set ("mb_speed", 0.7f); set ("mb_mix", 0.35f); set ("mb_on", 1);
}
void MultiPage::resized()
{
    on->setBounds (160, 12, 100, 22); delta->setBounds (276, 12, 230, 22);
    view.setBounds (16, 48, 822, 236);
    for (int b = 0; b < kMbBands; ++b)
    {
        const int x0 = 16 + b * 276;
        solo[b]->setBounds (x0 + 170, 296, 90, 22);
        for (int k = 0; k < 4; ++k) knobs[b * 4 + k]->setBounds (x0 + k * 66, 326, 62, 62 + 27);
    }
    knobs[12]->setBounds (16, 450, 76, 76 + 27); knobs[13]->setBounds (100, 450, 76, 76 + 27);
    ottBtn->setBounds (200, 470, 150, 32); resetBtn->setBounds (360, 470, 100, 32);
}
void MultiPage::paint (juce::Graphics& g)
{
    paintHeader (g, "Tre bande, ognuna abbassa sopra la SOGLIA (RAPPORTO) e alza i dettagli deboli sotto (SOLLEVA). CARICA OTT imposta il suono stile OTT: "
                    "aggressivo e stirato, da dosare col MIX (20-40%). SOLO per sentire una banda. Trascina i tagli nello schema.");
    for (int b = 0; b < kMbBands; ++b)
    {
        const int x0 = 16 + b * 276;
        glowText (g, mbName (b), { (float) x0, 296, 160, 22 }, mbCol (b), uf (13.0f));
        g.setColour (glass (0.05f)); g.fillRoundedRectangle ((float) x0 - 4, 290, 268, 136, 8);
    }
    g.setColour (C::dim); g.setFont (uf (10.0f, false));
    g.drawFittedText ("OTT al 100% di MIX e estremo: per il master parti da 20-30%.", juce::Rectangle<int> (470, 470, 360, 34), juce::Justification::centredLeft, 2);
}
void MbView::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat(); lcd (g, b, 8);
    auto& P = ed.proc; const bool on = P.value ("mb_on") > 0.5f;
    const float xs[4] = { 0, xOf (P.value ("mb_x1")), xOf (juce::jmax (P.value ("mb_x2"), P.value ("mb_x1") * 2)), b.getWidth() };
    const float top = 34, bot = b.getHeight() - 18;
    auto yDb = [&] (float db) { return bot - juce::jlimit (0.0f, 1.0f, (db + 60) / 60) * (bot - top); };
    g.setFont (mf (8.5f));
    for (float db : { -48.0f, -36.0f, -24.0f, -12.0f, 0.0f }) { g.setColour (C::phos.withAlpha (0.06f)); g.drawHorizontalLine ((int) yDb (db), 4, b.getRight() - 4); g.setColour (C::dim.withAlpha (0.6f)); g.drawText (juce::String ((int) db), juce::Rectangle<float> (4, yDb (db) - 6, 26, 12), juce::Justification::centredLeft); }
    for (int k = 0; k < kMbBands; ++k)
    {
        const juce::String id = "mb" + juce::String (k + 1) + "_"; const auto c = on ? mbCol (k) : C::faint;
        auto col = juce::Rectangle<float> (xs[k] + 34, top, xs[k + 1] - xs[k] - 44, bot - top);
        const float cx = col.getCentreX();
        // livello della banda
        const float lv = P.mbLevel[k].load();
        auto lb = juce::Rectangle<float> (cx - 26, yDb (lv), 18, bot - yDb (lv));
        juce::ColourGradient cg (c.withAlpha (0.8f), 0, lb.getY(), c.withAlpha (0.2f), 0, bot, false); g.setGradientFill (cg); g.fillRect (lb);
        // soglie
        const float thr = P.value ((id + "thr").toRawUTF8()), up = P.value ((id + "up").toRawUTF8());
        g.setColour (C::red); g.fillRect (col.getX(), yDb (thr) - 1, col.getWidth(), 2.0f);
        g.setColour (C::red.withAlpha (0.8f)); g.drawText ("GIU", juce::Rectangle<float> (col.getRight() - 40, yDb (thr) - 14, 40, 12), juce::Justification::centredRight);
        if (up > 1e-3f)
        {
            g.setColour (C::green.withAlpha (0.8f)); for (float x = col.getX(); x < col.getRight(); x += 6) g.drawHorizontalLine ((int) yDb (thr - 18), x, x + 3);
            g.drawText ("SU", juce::Rectangle<float> (col.getRight() - 40, yDb (thr - 18) + 2, 40, 12), juce::Justification::centredRight);
        }
        // guadagno applicato (giu rosso, su verde)
        const float dn = P.mbDown[k].load(), uu = P.mbUp[k].load();
        auto gb = juce::Rectangle<float> (cx + 8, top, 18, bot - top); const float mid = gb.getCentreY(), sc = (bot - top) * 0.5f / 18.0f;
        g.setColour (glass (0.05f)); g.fillRect (gb);
        g.setColour (C::dim.withAlpha (0.6f)); g.drawHorizontalLine ((int) mid, gb.getX() - 2, gb.getRight() + 2);
        if (dn > 0.05f) { g.setColour (C::red); g.fillRect (gb.getX(), mid, gb.getWidth(), juce::jmin (dn, 18.0f) * sc); }
        if (uu > 0.05f) { g.setColour (C::green); g.fillRect (gb.getX(), mid - juce::jmin (uu, 18.0f) * sc, gb.getWidth(), juce::jmin (uu, 18.0f) * sc); }
        glowText (g, mbName (k), { xs[k] + 8, 6, xs[k + 1] - xs[k] - 16, 16 }, c, uf (11.0f), juce::Justification::centred);
        g.setColour (C::dim); g.drawText ((uu > 0.05f ? "+" + juce::String (uu, 1) : juce::String()) + (dn > 0.05f ? "  -" + juce::String (dn, 1) : juce::String()), juce::Rectangle<float> (xs[k] + 8, 20, xs[k + 1] - xs[k] - 16, 12), juce::Justification::centred);
    }
    for (int k = 1; k < 3; ++k)
    {
        g.setColour ((on ? C::text : C::faint).withAlpha (drag == k ? 1.0f : 0.55f)); g.fillRect (xs[k] - 1, 4.0f, 2.0f, b.getHeight() - 8);
        glowText (g, hzT (P.value (k == 1 ? "mb_x1" : "mb_x2")), { xs[k] - 40, b.getHeight() - 16, 80, 12 }, C::dim, mf (9.0f), juce::Justification::centred);
    }
    scanlines (g, b);
}
void MbView::mouseDown (const juce::MouseEvent& e)
{
    drag = -1;
    for (int k = 1; k < 3; ++k) if (std::abs (e.position.x - xOf (ed.proc.value (k == 1 ? "mb_x1" : "mb_x2"))) < 10) drag = k;
    if (drag > 0) if (auto* pg = dynamic_cast<MultiPage*> (getParentComponent())) pg->touch();
}
void MbView::mouseDrag (const juce::MouseEvent& e)
{
    if (drag < 1) return;
    float f = fOf (e.position.x);
    if (drag == 1) f = juce::jmin (f, ed.proc.value ("mb_x2") / 2); else f = juce::jmax (f, ed.proc.value ("mb_x1") * 2);
    auto* p = ed.P (drag == 1 ? "mb_x1" : "mb_x2"); p->setValueNotifyingHost (p->convertTo0to1 (f)); repaint();
}

// ======================================================================= TAPE
TapePage::TapePage (Editor& e) : ModPage (e, TAPE, "tp_on", "tp_delta"), view (e)
{
    auto& P = ed.proc; const auto c = neo::acc (juce::Colour (modInfo (TAPE).colour));
    auto* d = knob ("tp_drive", c, "DRIVE", [&P] { return juce::String (juce::roundToInt (P.value ("tp_drive") * 100)) + "%"; }); d->big = true;
    knob ("tp_bias", C::amber, "BIAS", [&P] { const float v = P.value ("tp_bias"); return std::abs (v) < 0.03f ? juce::String ("TARATO") : v < 0 ? "BASSO " + juce::String (juce::roundToInt (-v * 100)) : "ALTO " + juce::String (juce::roundToInt (v * 100)); }, true);
    knob ("tp_noise", C::dim, "FRUSCIO", [&P] { return juce::String (juce::roundToInt (P.value ("tp_noise") * 100)) + "%"; });
    knob ("tp_mix", C::phos, "MIX", [&P] { return juce::String (juce::roundToInt (P.value ("tp_mix") * 100)) + "%"; });
    model = std::make_unique<Segs> (ed.P ("tp_model"), c, juce::StringArray { "30 IPS", "15 IPS", "CASSETTA" });
    addAndMakeVisible (*model); addAndMakeVisible (view);
}
void TapePage::resized()
{
    on->setBounds (110, 12, 100, 22); delta->setBounds (226, 12, 230, 22);
    model->setBounds (16, 50, 420, 30);
    view.setBounds (16, 112, 822, 240);
    knobs[0]->setBounds (16, 368, 140, 140 + 32);
    for (int i = 1; i < knobs.size(); ++i) knobs[i]->setBounds (176 + (i - 1) * 112, 388, 100, 100 + 27);
}
void TapePage::paint (juce::Graphics& g)
{
    paintHeader (g, "Nastro da master: DRIVE porta il segnale nella saturazione del nastro (colla, calore, transienti arrotondati). BIAS basso = piu acuti e piu armoniche, "
                    "alto = piu scuro e pulito. MIX sotto 100% = nastro in parallelo. Per il master bastano DRIVE 20-40% e 15 IPS.");
    g.setColour (C::text.withAlpha (0.85f)); g.setFont (uf (11.0f, false));
    g.drawFittedText (tapeInfo ((int) ed.proc.value ("tp_model")).what, juce::Rectangle<int> (16, 86, 822, 20), juce::Justification::centredLeft, 1);
}
void TapeView::update()
{
    auto& P = ed.proc; const int m = (int) P.value ("tp_model"); const float d = P.value ("tp_drive"), b = P.value ("tp_bias");
    if (m == lm && std::abs (d - ld) < 1e-3f && std::abs (b - lb) < 1e-3f) return;
    lm = m; ld = d; lb = b;
    TapeMachine tm; const double sr = 48000; tm.prepare (sr);
    const int order = 14, N = 1 << order, cyc = 32; std::vector<float> l ((size_t) (N + 8192)), r ((size_t) (N + 8192));
    for (int i = 0; i < (int) l.size(); ++i) l[(size_t) i] = r[(size_t) i] = 0.25f * (float) std::sin (2 * juce::MathConstants<double>::pi * cyc * (i % N) / N + 0.0);
    TapeSettings s; s.model = m; s.drive = d; s.bias = b; tm.process (l.data(), r.data(), (int) l.size(), s);
    juce::dsp::FFT f (order); std::vector<float> a ((size_t) (2 * N), 0); for (int i = 0; i < N; ++i) a[(size_t) i] = l[(size_t) (8192 + i)];
    f.performFrequencyOnlyForwardTransform (a.data(), true);
    const float h1 = juce::jmax (1e-9f, a[(size_t) cyc]);
    for (int h = 0; h < 8; ++h) harm[h] = gToDb (a[(size_t) ((h + 1) * cyc)] / h1);
}
void TapeView::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat(); lcd (g, b, 8);
    auto& P = ed.proc; const bool on = P.value ("tp_on") > 0.5f; const int m = (int) P.value ("tp_model");
    update();
    const auto c = neo::acc (juce::Colour (modInfo (TAPE).colour));
    // bobine
    angle += on ? (m == T30 ? 0.16f : m == T15 ? 0.09f : 0.05f) : 0.0f;
    auto reel = [&] (float cx, float cy, float R, float fill)
    {
        g.setColour (juce::Colour (0xff1b1d22)); g.fillEllipse (cx - R, cy - R, 2 * R, 2 * R);
        g.setColour (juce::Colour (0xff3a2a18)); g.fillEllipse (cx - R * fill, cy - R * fill, 2 * R * fill, 2 * R * fill);
        g.setColour (juce::Colour (0xff9aa2b1)); g.drawEllipse (cx - R, cy - R, 2 * R, 2 * R, 2.0f);
        for (int k = 0; k < 3; ++k)
        {
            const float a = angle + k * 2.094f; juce::Path p;
            p.addPieSegment (cx - R * 0.8f, cy - R * 0.8f, R * 1.6f, R * 1.6f, a, a + 0.6f, 0.35f);
            g.setColour (juce::Colour (0xff0b0c10)); g.fillPath (p);
        }
        g.setColour (juce::Colour (0xffc9ced8)); g.fillEllipse (cx - R * 0.18f, cy - R * 0.18f, R * 0.36f, R * 0.36f);
    };
    const float R = m == CASSETTA ? 38.0f : 70.0f, cy = b.getY() + 100;
    reel (b.getX() + 110, cy, R, 0.75f); reel (b.getX() + 330, cy, R, 0.55f);
    g.setColour (juce::Colour (0xff3a2a18)); g.drawLine (b.getX() + 110, cy + R * 0.75f, b.getX() + 330, cy + R * 0.55f, 2.0f);
    glowText (g, tapeInfo (m).name, { b.getX() + 150, b.getBottom() - 30, 140, 20 }, on ? c : C::faint, uf (13.0f), juce::Justification::centred);
    // armoniche
    auto hb = juce::Rectangle<float> (b.getX() + 470, b.getY() + 34, 330, b.getHeight() - 70);
    g.setColour (C::dim); g.setFont (mf (9.0f)); g.drawText ("ARMONICHE DEL NASTRO (sinusoide a -12 dBFS)", juce::Rectangle<float> (hb.getX(), b.getY() + 12, 330, 12), juce::Justification::centredLeft);
    const float bw = hb.getWidth() / 8;
    for (int h = 0; h < 8; ++h)
    {
        const float nn = juce::jlimit (0.0f, 1.0f, (harm[h] + 90) / 90);
        auto r = juce::Rectangle<float> (hb.getX() + h * bw + 4, hb.getY(), bw - 8, hb.getHeight());
        g.setColour (glass (0.04f)); g.fillRoundedRectangle (r, 3);
        auto f = r.withTop (r.getBottom() - r.getHeight() * nn); const auto hc = h == 0 ? C::text : (h % 2 == 1 ? C::amber : c);
        g.setColour (hc.withAlpha (0.25f)); g.fillRoundedRectangle (f.expanded (1.5f), 3); g.setColour (hc); g.fillRoundedRectangle (f, 2);
        g.setColour (C::dim); g.drawText (juce::String (h + 1), juce::Rectangle<float> (r.getX(), r.getBottom() + 2, r.getWidth(), 12), juce::Justification::centred);
    }
    scanlines (g, b);
}

// ======================================================================= EXCITER
static juce::Colour exCol (int b) { static const juce::uint32 c[kExBands] = { 0xffffb000, 0xffff5ea8, 0xff5ef2ff }; return juce::Colour (c[b]); }
ExciterPage::ExciterPage (Editor& e) : ModPage (e, EXCITER, "ex_on", "ex_delta"), view (e)
{
    auto& P = ed.proc;
    for (int b = 0; b < kExBands; ++b)
    {
        const juce::String id = "ex" + juce::String (b + 1) + "_";
        auto* k = knob ((id + "amt").toRawUTF8(), exCol (b), "QUANTITA", [&P, id] { return juce::String (juce::roundToInt (P.value ((id + "amt").toRawUTF8()) * 100)) + "%"; }); juce::ignoreUnused (k);
        mode[b] = std::make_unique<Segs> (ed.P (id + "mode"), exCol (b), juce::StringArray { "VALV", "NAST", "TRIO", "CALD", "RETR", "BRIL" }); mode[b]->font = 9.0f;
        solo[b] = std::make_unique<Switch> (ed.P (id + "solo"), C::amber, "SOLO");
        addAndMakeVisible (*mode[b]); addAndMakeVisible (*solo[b]);
    }
    knob ("ex_mix", C::phos, "MIX", [&P] { return juce::String (juce::roundToInt (P.value ("ex_mix") * 100)) + "%"; });
    addAndMakeVisible (view);
}
void ExciterPage::resized()
{
    on->setBounds (130, 12, 100, 22); delta->setBounds (246, 12, 230, 22);
    view.setBounds (16, 48, 822, 236);
    for (int b = 0; b < kExBands; ++b)
    {
        const int x0 = 16 + b * 250;
        solo[b]->setBounds (x0 + 160, 298, 80, 20);
        mode[b]->setBounds (x0, 326, 240, 26);
        knobs[b]->setBounds (x0 + 6, 362, 90, 90 + 27);
    }
    knobs[3]->setBounds (766, 362, 72, 72 + 27);
}
void ExciterPage::paint (juce::Graphics& g)
{
    paintHeader (g, "Aggiunge armoniche a ogni banda, in parallelo: il suono resta lui ma diventa piu pieno, presente o brillante. Scegli il carattere per banda e dosa la QUANTITA "
                    "(sul master 5-25%). Trascina i tagli e alza le barre nello schema. SOLO e DELTA per sentire cosa aggiunge.");
    static const char* names[kExBands] = { "BASSI", "MEDI", "ALTI" };
    for (int b = 0; b < kExBands; ++b)
    {
        const float x0 = 16.0f + b * 250;
        g.setColour (glass (0.05f)); g.fillRoundedRectangle (x0 - 4, 292, 248, 192, 8);
        glowText (g, names[b], { x0 + 2, 298, 150, 20 }, exCol (b), uf (12.5f));
        g.setColour (C::dim); g.setFont (uf (10.0f, false));
        g.drawFittedText (exWhat ((int) ed.proc.value (("ex" + juce::String (b + 1) + "_mode").toRawUTF8())), juce::Rectangle<int> ((int) x0 + 104, 372, 136, 80), juce::Justification::topLeft, 4);
    }
}
void ExView::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat(); lcd (g, b, 8);
    auto& P = ed.proc; const bool on = P.value ("ex_on") > 0.5f;
    an.run (P.ringOut, sp, 200, 20, 20000, P.sampleRate());
    const float top = 26, bot = b.getHeight() - 18;
    drawSpec (g, sp, 200, juce::Rectangle<float> (0, top, b.getWidth(), bot - top), C::dim.withAlpha (0.35f), true, -100, 0);
    const float xs[4] = { 0, xOf (P.value ("ex_x1")), xOf (juce::jmax (P.value ("ex_x2"), P.value ("ex_x1") * 2)), b.getWidth() };
    for (int k = 0; k < kExBands; ++k)
    {
        const float a = P.value (("ex" + juce::String (k + 1) + "_amt").toRawUTF8()); const auto c = on ? exCol (k) : C::faint;
        const float y = bot - a * (bot - top);
        auto r = juce::Rectangle<float> (xs[k] + 3, y, xs[k + 1] - xs[k] - 6, bot - y);
        juce::ColourGradient cg (c.withAlpha (0.5f), 0, r.getY(), c.withAlpha (0.08f), 0, bot, false); g.setGradientFill (cg); g.fillRect (r);
        g.setColour (c); g.fillRect (xs[k] + 3, y - 1.5f, xs[k + 1] - xs[k] - 6, 3.0f);
        glowText (g, juce::String (exName ((int) P.value (("ex" + juce::String (k + 1) + "_mode").toRawUTF8()))) + "  " + juce::String (juce::roundToInt (a * 100)) + "%",
                  { xs[k], juce::jmax (top, y - 18), xs[k + 1] - xs[k], 14 }, c, mf (10.0f), juce::Justification::centred);
    }
    for (int k = 1; k < 3; ++k)
    {
        g.setColour ((on ? C::text : C::faint).withAlpha (drag == k ? 1.0f : 0.55f)); g.fillRect (xs[k] - 1, 4.0f, 2.0f, b.getHeight() - 8);
        glowText (g, hzT (P.value (k == 1 ? "ex_x1" : "ex_x2")), { xs[k] - 40, 6, 80, 14 }, C::dim, mf (9.0f), juce::Justification::centred);
    }
    scanlines (g, b);
}
void ExView::mouseDown (const juce::MouseEvent& e)
{
    drag = -1;
    for (int k = 1; k < 3; ++k) if (std::abs (e.position.x - xOf (ed.proc.value (k == 1 ? "ex_x1" : "ex_x2"))) < 9) drag = k;
    if (drag < 0) { const float x1 = xOf (ed.proc.value ("ex_x1")), x2 = xOf (ed.proc.value ("ex_x2")); drag = 10 + (e.position.x < x1 ? 0 : e.position.x < x2 ? 1 : 2); }
    if (auto* pg = dynamic_cast<ExciterPage*> (getParentComponent())) pg->touch();
    mouseDrag (e);
}
void ExView::mouseDrag (const juce::MouseEvent& e)
{
    if (drag < 0) return;
    if (drag < 3)
    {
        float f = fOf (e.position.x);
        if (drag == 1) f = juce::jmin (f, ed.proc.value ("ex_x2") / 2); else f = juce::jmax (f, ed.proc.value ("ex_x1") * 2);
        auto* p = ed.P (drag == 1 ? "ex_x1" : "ex_x2"); p->setValueNotifyingHost (p->convertTo0to1 (f));
    }
    else
    {
        const float top = 26, bot = getHeight() - 18.0f;
        auto* p = ed.P ("ex" + juce::String (drag - 9) + "_amt"); p->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, (bot - e.position.y) / (bot - top)));
    }
    repaint();
}

// ======================================================================= EQ 432
Eq432Page::Eq432Page (Editor& e) : ModPage (e, EQ432, "e4_on", "e4_delta"), view (e)
{
    auto& P = ed.proc; const auto c = neo::acc (juce::Colour (modInfo (EQ432).colour));
    static const char* bn[k432Bands] = { "BASSI", "M-BASSI", "MEDI", "M-ALTI", "ALTI" };
    for (int b = 0; b < k432Bands; ++b)
    {
        const juce::String id = "e4_" + juce::String (b + 1);
        const juce::String fid = id + "f", gid = id + "g", qid = id + "q";
        knob (fid.toRawUTF8(), c, juce::String (bn[b]) + " FREQ", [&P, fid] { return hzT (P.value (fid.toRawUTF8())); });
        knob (gid.toRawUTF8(), c.interpolatedWith (C::magenta, 0.35f), "GUADAGNO", [&P, gid] { return dbS (P.value (gid.toRawUTF8()) * eq432Range ((int) P.value ("e4_range"))) + " dB"; }, true);
        knob (qid.toRawUTF8(), c.interpolatedWith (C::phos, 0.4f), "LARGHEZZA", [&P, qid] { return juce::String (P.value (qid.toRawUTF8()), 2); });
    }
    knob ("e4_col", C::amber, "COLORE", [&P] { return juce::String (juce::roundToInt (P.value ("e4_col") * 100)) + "%"; });
    range = std::make_unique<Segs> (ed.P ("e4_range"), c, juce::StringArray { "6 dB", "9 dB", "12 dB" }); range->font = 10.0f;
    sh1 = std::make_unique<Switch> (ed.P ("e4_sh1"), c, "BASSI SHELF"); sh5 = std::make_unique<Switch> (ed.P ("e4_sh5"), c, "ALTI SHELF");
    for (juce::Component* x : std::initializer_list<juce::Component*> { range.get(), sh1.get(), sh5.get(), &view }) addAndMakeVisible (x);
}
void Eq432Page::resized()
{
    on->setBounds (190, 12, 100, 22); delta->setBounds (306, 12, 230, 22);
    view.setBounds (16, 48, 822, 210);
    const float cw = 822.0f / k432Bands;
    for (int b = 0; b < k432Bands; ++b)
        for (int r = 0; r < 3; ++r)
        {
            const int k = b * 3 + r; const int S = r == 1 ? 66 : 48;
            const float cx = 16 + cw * (b + 0.5f), y = r == 0 ? 264.0f : r == 1 ? 340.0f : 432.0f;
            knobs[k]->setBounds (juce::Rectangle<float> (cx - (S + 26) * 0.5f, y, (float) S + 26, (float) S + 27).toNearestInt());
        }
    knobs[k432Bands * 3]->setBounds (590, 512, 70, 40 + 27);
    range->setBounds (16, 540, 220, 26); sh1->setBounds (250, 542, 140, 22); sh5->setBounds (400, 542, 140, 22);
}
void Eq432Page::paint (juce::Graphics& g)
{
    paintHeader (g, juce::String::fromUTF8 ("Cinque bande larghe che si sovrappongono, come i parametrici classici da mastering: 1-2 dB bastano. GAMMA sceglie quanto arriva ogni manopola (6, 9 o 12 dB). "
                    "LARGHEZZA: valori bassi = curva larga e dolce. Con poco guadagno la campana si allarga da sola. COLORE: l'impronta dello stadio analogico."));
    g.setFont (neo::cap (7.5f)); g.setColour (C::dim); g.drawText ("GAMMA", juce::Rectangle<float> (16, 526, 120, 12), juce::Justification::centredLeft);
}
void E4View::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat(); lcd (g, b, 10);
    auto& P = ed.proc; const double sr = juce::jmax (8000.0, P.sampleRate());
    auto p = b.reduced (30, 14).withTrimmedBottom (8);
    auto xOf = [&p] (double f) { return p.getX() + p.getWidth() * (float) (std::log (f / 20.0) / std::log (1000.0)); };
    const float rng = eq432Range ((int) P.value ("e4_range"));
    auto yOf = [&p, rng] (double db) { return p.getCentreY() - (float) (db / (rng + 2.0)) * p.getHeight() * 0.5f; };
    g.setFont (neo::num (8.0f));
    for (float f : { 50.f, 100.f, 200.f, 500.f, 1000.f, 2000.f, 5000.f, 10000.f })
    { g.setColour (glass (0.05f)); g.drawVerticalLine ((int) xOf (f), p.getY(), p.getBottom()); g.setColour (C::dim); g.drawText (f >= 1000 ? juce::String ((int) (f / 1000)) + "k" : juce::String ((int) f), juce::Rectangle<float> (xOf (f) - 14, p.getBottom() + 1, 28, 10), juce::Justification::centred); }
    for (float d : { -rng, -rng * 0.5f, 0.0f, rng * 0.5f, rng })
    { g.setColour (glass (d == 0 ? 0.12f : 0.04f)); g.drawHorizontalLine ((int) yOf (d), p.getX(), p.getRight()); g.setColour (C::dim); g.drawText (dbS (d, 0), juce::Rectangle<float> (b.getX() + 2, yOf (d) - 6, 26, 12), juce::Justification::centredRight); }
    Biquad f[k432Bands]; bool act[k432Bands] {};
    for (int k = 0; k < k432Bands; ++k)
    {
        const juce::String id = "e4_" + juce::String (k + 1);
        const float gdb = P.value ((id + "g").toRawUTF8()) * rng, q0 = P.value ((id + "q").toRawUTF8()), fr = P.value ((id + "f").toRawUTF8());
        const int type = (k == 0 && P.value ("e4_sh1") > 0.5f) ? Biquad::LSHELF : (k == k432Bands - 1 && P.value ("e4_sh5") > 0.5f) ? Biquad::HSHELF : Biquad::BELL;
        const float q = type == Biquad::BELL ? q0 * (0.6f + 0.4f * std::abs (gdb) / rng) : juce::jmin (1.0f, q0);
        f[k].set (type, fr, gdb, q, sr); act[k] = std::abs (gdb) > 0.01f;
    }
    const auto c = neo::acc (juce::Colour (modInfo (EQ432).colour));
    juce::Path path; const int N = (int) p.getWidth() / 2;
    for (int i = 0; i <= N; ++i)
    {
        const double fr = 20.0 * std::pow (1000.0, (double) i / N); double d = 0;
        for (int k = 0; k < k432Bands; ++k) if (act[k]) d += f[k].magDb (fr, sr);
        const float x = p.getX() + p.getWidth() * i / N, y = yOf (d);
        if (i == 0) path.startNewSubPath (x, y); else path.lineTo (x, y);
    }
    juce::Path fill = path; fill.lineTo (p.getRight(), yOf (0)); fill.lineTo (p.getX(), yOf (0)); fill.closeSubPath();
    g.setColour (c.withAlpha (0.12f)); g.fillPath (fill);
    g.setColour (c.withAlpha (0.3f)); g.strokePath (path, juce::PathStrokeType (5.0f));
    g.setColour (c); g.strokePath (path, juce::PathStrokeType (1.8f));
    g.setFont (neo::cap (8.0f)); g.setColour (c.withAlpha (0.8f)); g.drawText ("EQ 432  " + juce::String (rng, 0) + " dB", b.reduced (12, 6), juce::Justification::topLeft);
}

// ======================================================================= PULTEC
PultecPage::PultecPage (Editor& e) : ModPage (e, PULTEC, "pq_on", "pq_delta"), view (e)
{
    auto& P = ed.proc; const auto c = neo::acc (juce::Colour (modInfo (PULTEC).colour));
    auto ten = [&P] (const char* id) { return [&P, id] { return juce::String (P.value (id), 1); }; };
    knob ("pq_lb", c, "BOOST BASSI", ten ("pq_lb"));
    knob ("pq_la", c.interpolatedWith (C::magenta, 0.4f), "ATTEN BASSI", ten ("pq_la"));
    knob ("pq_hb", c, "BOOST ALTI", ten ("pq_hb"));
    knob ("pq_bw", c.interpolatedWith (C::phos, 0.4f), "LARGHEZZA", [&P] { const float v = P.value ("pq_bw"); return v < 3.3f ? juce::String ("STRETTA") : v < 6.6f ? juce::String ("MEDIA") : juce::String ("LARGA"); });
    knob ("pq_ha", c.interpolatedWith (C::magenta, 0.4f), "ATTEN ALTI", ten ("pq_ha"));
    knob ("pq_tube", C::amber, "VALVOLA", [&P] { return juce::String (juce::roundToInt (P.value ("pq_tube") * 100)) + "%"; });
    lf = std::make_unique<Segs> (ed.P ("pq_lf"), c, juce::StringArray { "20", "30", "60", "100" }); lf->font = 10.0f;
    hf = std::make_unique<Segs> (ed.P ("pq_hf"), c, juce::StringArray { "3k", "4k", "5k", "8k", "10k", "12k", "16k" }); hf->font = 10.0f;
    as = std::make_unique<Segs> (ed.P ("pq_as"), c, juce::StringArray { "5k", "10k", "20k" }); as->font = 10.0f;
    for (juce::Component* x : std::initializer_list<juce::Component*> { lf.get(), hf.get(), as.get(), &view }) addAndMakeVisible (x);
}
void PultecPage::resized()
{
    on->setBounds (190, 12, 100, 22); delta->setBounds (306, 12, 230, 22);
    view.setBounds (16, 48, 822, 230);
    // sezione bassi a sinistra, alti a destra
    const int ky = 330, S = 78;
    auto put = [this, ky, S] (int k, float cx) { knobs[k]->setBounds (juce::Rectangle<float> (cx - (S + 26) * 0.5f, (float) ky, (float) S + 26, (float) S + 27).toNearestInt()); };
    put (0, 90); put (1, 210);
    put (2, 420); put (3, 540); put (4, 700);
    knobs[5]->setBounds (762, 532, 76, 50 + 27);
    lf->setBounds (40, 456, 220, 26); hf->setBounds (330, 456, 300, 26); as->setBounds (650, 456, 100, 26);
}
void PultecPage::paint (juce::Graphics& g)
{
    paintHeader (g, juce::String::fromUTF8 ("Il trucco del Pultec: alza e abbassa i bassi INSIEME sulla stessa frequenza. Il boost spinge sotto, l'attenuazione toglie un po' pi\xc3\xb9 in alto: "
                    "esce un basso pieno ma pulito, con un piccolo avvallamento sui bassi-medi. Sugli alti: campana morbida (LARGHEZZA) e taglio a scaffale separato. Scala 0-10 come l'hardware."));
    auto c = neo::acc (juce::Colour (modInfo (PULTEC).colour));
    g.setColour (glass (0.04f)); g.fillRoundedRectangle (16, 290, 270, 230, 10); g.fillRoundedRectangle (300, 290, 538, 230, 10);
    g.setFont (neo::cap (9.0f)); g.setColour (c);
    g.drawText ("BASSE FREQUENZE", juce::Rectangle<float> (30, 296, 240, 18), juce::Justification::centredLeft);
    g.drawText ("ALTE FREQUENZE", juce::Rectangle<float> (314, 296, 300, 18), juce::Justification::centredLeft);
    g.setFont (neo::cap (7.5f)); g.setColour (C::dim);
    g.drawText ("FREQUENZA  Hz", juce::Rectangle<float> (40, 442, 220, 12), juce::Justification::centredLeft);
    g.drawText ("FREQUENZA BOOST", juce::Rectangle<float> (330, 442, 300, 12), juce::Justification::centredLeft);
    g.drawText ("ATTEN SEL", juce::Rectangle<float> (650, 442, 100, 12), juce::Justification::centredLeft);
}
void PqView::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat(); lcd (g, b, 10);
    auto& P = ed.proc; const double sr = juce::jmax (8000.0, P.sampleRate());
    auto p = b.reduced (30, 14).withTrimmedBottom (8);
    auto xOf = [&p] (double f) { return p.getX() + p.getWidth() * (float) (std::log (f / 20.0) / std::log (1000.0)); };
    const float rng = 18.0f;
    auto yOf = [&p, rng] (double db) { return p.getCentreY() - (float) (db / rng) * p.getHeight() * 0.5f; };
    g.setFont (neo::num (8.0f));
    for (float f : { 30.f, 60.f, 100.f, 200.f, 500.f, 1000.f, 2000.f, 5000.f, 10000.f })
    { g.setColour (glass (0.05f)); g.drawVerticalLine ((int) xOf (f), p.getY(), p.getBottom()); g.setColour (C::dim); g.drawText (f >= 1000 ? juce::String ((int) (f / 1000)) + "k" : juce::String ((int) f), juce::Rectangle<float> (xOf (f) - 14, p.getBottom() + 1, 28, 10), juce::Justification::centred); }
    for (float d : { -15.f, -10.f, -5.f, 0.f, 5.f, 10.f, 15.f })
    { g.setColour (glass (d == 0 ? 0.12f : 0.04f)); g.drawHorizontalLine ((int) yOf (d), p.getX(), p.getRight()); g.setColour (C::dim); g.drawText (dbS (d, 0), juce::Rectangle<float> (b.getX() + 2, yOf (d) - 6, 26, 12), juce::Justification::centredRight); }
    PultecSettings S; S.lf = (int) P.value ("pq_lf"); S.hf = (int) P.value ("pq_hf"); S.as = (int) P.value ("pq_as");
    S.lb = P.value ("pq_lb"); S.la = P.value ("pq_la"); S.hb = P.value ("pq_hb"); S.bw = P.value ("pq_bw"); S.ha = P.value ("pq_ha");
    Biquad f[4]; bool act[4] {}; pultecDesign (S, sr, f, act);
    const auto c = neo::acc (juce::Colour (modInfo (PULTEC).colour)); const bool on = P.value ("pq_on") > 0.5f;
    // contributi separati, tenui
    for (int k = 0; k < 4; ++k)
    {
        if (! act[k]) continue;
        juce::Path q; const int N = (int) p.getWidth() / 3;
        for (int i = 0; i <= N; ++i) { const double fr = 20.0 * std::pow (1000.0, (double) i / N); const float x = p.getX() + p.getWidth() * i / N, y = yOf (f[k].magDb (fr, sr)); if (i == 0) q.startNewSubPath (x, y); else q.lineTo (x, y); }
        g.setColour ((k == 1 || k == 3 ? C::magenta : c).withAlpha (0.35f)); g.strokePath (q, juce::PathStrokeType (1.0f));
    }
    juce::Path path; const int N = (int) p.getWidth() / 2;
    for (int i = 0; i <= N; ++i)
    {
        const double fr = 20.0 * std::pow (1000.0, (double) i / N); double d = 0;
        for (int k = 0; k < 4; ++k) if (act[k]) d += f[k].magDb (fr, sr);
        const float x = p.getX() + p.getWidth() * i / N, y = yOf (d);
        if (i == 0) path.startNewSubPath (x, y); else path.lineTo (x, y);
    }
    const auto cc = on ? c : C::faint;
    juce::Path fill = path; fill.lineTo (p.getRight(), yOf (0)); fill.lineTo (p.getX(), yOf (0)); fill.closeSubPath();
    g.setColour (cc.withAlpha (0.12f)); g.fillPath (fill);
    g.setColour (cc.withAlpha (0.3f)); g.strokePath (path, juce::PathStrokeType (5.0f));
    g.setColour (cc); g.strokePath (path, juce::PathStrokeType (1.8f));
    g.setFont (neo::cap (8.0f)); g.setColour (cc.withAlpha (0.8f)); g.drawText ("EQP-1A", b.reduced (12, 6), juce::Justification::topLeft);
    scanlines (g, b);
}

// ======================================================================= VARI-MU
VariMuPage::VariMuPage (Editor& e) : ModPage (e, VARIMU, "vm_on", "vm_delta"), view (e)
{
    auto& P = ed.proc; const auto c = neo::acc (juce::Colour (modInfo (VARIMU).colour));
    knob ("vm_thr", c, "SOGLIA", [&P] { return juce::String (P.value ("vm_thr"), 1) + " dB"; });
    knob ("vm_makeup", c.interpolatedWith (C::phos, 0.4f), "GUADAGNO", [&P] { return dbS (P.value ("vm_makeup")) + " dB"; }, true);
    knob ("vm_mix", c.interpolatedWith (C::text, 0.3f), "MIX", [&P] { return juce::String (juce::roundToInt (P.value ("vm_mix") * 100)) + "%"; });
    knob ("vm_tube", C::amber, "VALVOLA", [&P] { return juce::String (juce::roundToInt (P.value ("vm_tube") * 100)) + "%"; });
    mode = std::make_unique<Segs> (ed.P ("vm_mode"), c, juce::StringArray { "COMPRIMI", "LIMITA" }); mode->font = 10.0f;
    att = std::make_unique<Segs> (ed.P ("vm_att"), c, juce::StringArray { "1", "5", "15", "30", "70" }); att->font = 10.0f;
    rel = std::make_unique<Segs> (ed.P ("vm_rel"), c, juce::StringArray { "0.2", "0.4", "0.6", "0.8", "1.2", "2", "AUTO" }); rel->font = 10.0f;
    hpf = std::make_unique<Segs> (ed.P ("vm_hpf"), c, juce::StringArray { "OFF", "60", "100", "150" }); hpf->font = 10.0f;
    autoM = std::make_unique<Switch> (ed.P ("vm_auto"), c, "COMPENSA AUTO"); ms = std::make_unique<Switch> (ed.P ("vm_ms"), c, "MID / SIDE");
    for (juce::Component* x : std::initializer_list<juce::Component*> { mode.get(), att.get(), rel.get(), hpf.get(), autoM.get(), ms.get(), &view }) addAndMakeVisible (x);
}
void VariMuPage::resized()
{
    on->setBounds (190, 12, 100, 22); delta->setBounds (306, 12, 230, 22);
    view.setBounds (16, 48, 400, 470);
    const int S = 78;
    auto put = [this, S] (int k, float cx, float y) { knobs[k]->setBounds (juce::Rectangle<float> (cx - (S + 26) * 0.5f, y, (float) S + 26, (float) S + 27).toNearestInt()); };
    put (0, 500, 56); put (1, 630, 56); put (2, 760, 56); put (3, 500, 410);
    mode->setBounds (440, 230, 200, 26);
    att->setBounds (440, 290, 260, 26); rel->setBounds (440, 350, 390, 26);
    hpf->setBounds (580, 420, 200, 26);
    autoM->setBounds (580, 470, 150, 22); ms->setBounds (580, 498, 150, 22);
}
void VariMuPage::paint (juce::Graphics& g)
{
    paintHeader (g, juce::String::fromUTF8 ("Compressore a valvole: rapporto morbido che cresce col segnale, rilascio lento che 'incolla' il mix. 1-3 dB di riduzione sul master bastano. "
                    "COMPRIMI = dolce, LIMITA = pi\xc3\xb9 deciso. Filtro sul sidechain: i bassi non fanno pompare. AUTO sul rilascio segue il programma."));
    g.setFont (neo::cap (7.5f)); g.setColour (C::dim);
    g.drawText ("MODO", juce::Rectangle<float> (440, 216, 200, 12), juce::Justification::centredLeft);
    g.drawText ("ATTACCO  ms", juce::Rectangle<float> (440, 276, 260, 12), juce::Justification::centredLeft);
    g.drawText ("RILASCIO  s", juce::Rectangle<float> (440, 336, 260, 12), juce::Justification::centredLeft);
    g.drawText ("FILTRO SIDECHAIN  Hz", juce::Rectangle<float> (580, 406, 200, 12), juce::Justification::centredLeft);
}
void VmView::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat(); lcd (g, b, 8);
    auto& P = ed.proc; const float gr = P.value ("vm_on") > 0.5f ? P.vmGr.load() : 0.0f;
    needle += (gr - needle) * (gr > needle ? 0.35f : 0.1f);
    auto vu = juce::Rectangle<float> (b.getX() + 20, b.getY() + 16, b.getWidth() - 40, b.getHeight() * 0.5f);
    juce::ColourGradient face (juce::Colour (0xfff0e6cc), vu.getCentreX(), vu.getY(), juce::Colour (0xffcdb98f), vu.getCentreX(), vu.getBottom(), false);
    g.setGradientFill (face); g.fillRoundedRectangle (vu, 8);
    const float cx = vu.getCentreX(), cy = vu.getBottom() + vu.getHeight() * 0.35f, R = vu.getHeight() * 1.0f;
    auto ang = [] (float db) { return -0.85f + juce::jlimit (0.0f, 1.0f, db / 12.0f) * 1.7f; };
    g.setFont (mf (10.0f));
    for (float db : { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 6.0f, 8.0f, 12.0f })
    {
        const float a = -ang (db);
        const juce::Point<float> p0 (cx + std::sin (a) * R * 0.86f, cy - std::cos (a) * R * 0.86f), p1 (cx + std::sin (a) * R * 0.96f, cy - std::cos (a) * R * 0.96f);
        g.setColour (db >= 6 ? juce::Colour (0xffb3261e) : juce::Colour (0xff2a2a2a)); g.drawLine ({ p0, p1 }, 1.6f);
        g.drawText (juce::String ((int) db), juce::Rectangle<float> (p1.x - 14, p1.y - 16, 28, 12), juce::Justification::centred);
    }
    g.setColour (juce::Colour (0xff2a2a2a)); g.setFont (uf (11.0f)); g.drawText ("RIDUZIONE  dB", vu.withTrimmedTop (vu.getHeight() * 0.55f).withHeight (16), juce::Justification::centred);
    {
        juce::Graphics::ScopedSaveState ss (g); g.reduceClipRegion (vu.toNearestInt());
        const float a = -ang (needle);
        g.setColour (veil (0.25f)); g.drawLine (cx + 3, cy, cx + 3 + std::sin (a) * R * 0.95f, cy - std::cos (a) * R * 0.95f, 2.0f);
        g.setColour (juce::Colour (0xff111111)); g.drawLine (cx, cy, cx + std::sin (a) * R * 0.95f, cy - std::cos (a) * R * 0.95f, 1.8f);
    }
    glowText (g, "-" + juce::String (needle, 1) + " dB", { vu.getX(), vu.getBottom() + 8, vu.getWidth(), 24 }, neo::acc (juce::Colour (modInfo (VARIMU).colour)), mf (16.0f), juce::Justification::centred);
    hist[hw] = gr; hw = (hw + 1) % 240;
    auto hr = juce::Rectangle<float> (b.getX() + 20, vu.getBottom() + 40, b.getWidth() - 40, b.getBottom() - vu.getBottom() - 56);
    g.setColour (veil (0.3f)); g.fillRoundedRectangle (hr, 4);
    const auto c = neo::acc (juce::Colour (modInfo (VARIMU).colour));
    const float cw = hr.getWidth() / 240.0f;
    for (int i = 0; i < 240; ++i)
    {
        const float v = hist[(hw + i) % 240]; if (v <= 0.01f) continue;
        const float h = juce::jlimit (0.0f, 1.0f, v / 12.0f) * hr.getHeight();
        g.setColour (c.withAlpha (0.65f)); g.fillRect (hr.getX() + i * cw, hr.getY(), juce::jmax (1.0f, cw), h);
    }
    g.setFont (neo::cap (7.5f)); g.setColour (C::dim); g.drawText ("STORIA", hr.reduced (6, 4), juce::Justification::bottomLeft);
}

// ======================================================================= BILANCIAMENTO TONALE
static void drawThirds (juce::Graphics& g, const float* v, juce::Rectangle<float> r, juce::Colour c, float range, bool fill)
{
    juce::Path p;
    for (int k = 0; k < kThirds; ++k)
    {
        const float x = r.getX() + r.getWidth() * k / (kThirds - 1), y = r.getCentreY() - juce::jlimit (-1.0f, 1.0f, v[k] / range) * r.getHeight() * 0.5f;
        if (k == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
    }
    if (fill) { juce::Path f = p; f.lineTo (r.getRight(), r.getBottom()); f.lineTo (r.getX(), r.getBottom()); f.closeSubPath(); juce::ColourGradient cg (c.withAlpha (0.2f), 0, r.getY(), c.withAlpha (0.02f), 0, r.getBottom(), false); g.setGradientFill (cg); g.fillPath (f); }
    g.setColour (c.withAlpha (0.25f)); g.strokePath (p, juce::PathStrokeType (4.0f)); g.setColour (c); g.strokePath (p, juce::PathStrokeType (1.6f));
}
BalancePage::BalancePage (Editor& e) : ed (e)
{
    fix = std::make_unique<ActBtn> ("CORREGGI CON L'EQ DINAMICO", C::green, [this]
    {
        float reg[kRegions], dev[kRegions]; ThirdAnalyzer::regions (ed.liveThirds, reg);
        const auto& G = genre (genreSel); float m = 0; for (float o : G.off) m += o; m /= kRegions;
        for (int r = 0; r < kRegions; ++r) dev[r] = reg[r] - (G.off[r] - m);
        applyEq (dev);
    });
    reset = std::make_unique<ActBtn> ("RIMETTI L'EQ A ZERO", C::phos, [this] { for (int b = 0; b < kEqBands; ++b) { auto* p = ed.P (eqId (b, "g")); p->setValueNotifyingHost (p->getDefaultValue()); } });
    addAndMakeVisible (*fix); addAndMakeVisible (*reset);
}
void BalancePage::applyEq (const float* dev)
{
    // una banda per regione: shelf per gli estremi, campane in mezzo. Correzione al 70%, massimo 4 dB.
    static const float f[kRegions] = { 45, 130, 450, 1400, 4000, 11000 };
    static const int t[kRegions] = { Biquad::LSHELF, Biquad::BELL, Biquad::BELL, Biquad::BELL, Biquad::BELL, Biquad::HSHELF };
    auto set = [this] (const char* id, float v) { auto* p = ed.P (id); p->setValueNotifyingHost (p->convertTo0to1 (v)); };
    for (int r = 0; r < kRegions; ++r)
    {
        set (eqId (r, "on"), 1); set (eqId (r, "type"), (float) t[r]); set (eqId (r, "ch"), 0); set (eqId (r, "f"), f[r]);
        set (eqId (r, "q"), t[r] == Biquad::BELL ? 0.7f : 0.7f); set (eqId (r, "dyn"), 0);
        set (eqId (r, "g"), juce::jlimit (-4.0f, 4.0f, -dev[r] * 0.7f));
    }
    set ("eq_on", 1);
}
void BalancePage::resized() { fix->setBounds (16, 556, 300, 34); reset->setBounds (326, 556, 200, 34); }
void BalancePage::mouseDown (const juce::MouseEvent& e) { for (int g = 0; g < kGenres; ++g) if (genreCell (g).contains (e.position)) { genreSel = g; repaint(); } }
void BalancePage::paint (juce::Graphics& g)
{
    glowText (g, "BILANCIAMENTO TONALE", { 16, 10, 400, 26 }, C::green, uf (17.0f));
    g.setColour (C::dim); g.setFont (uf (10.5f, false)); g.drawText ("Confronta lo spettro medio del tuo master con quello tipico del genere.", juce::Rectangle<float> (300, 14, 540, 20), juce::Justification::centredRight);
    for (int k = 0; k < kGenres; ++k) btn (g, genreCell (k), C::green, k == genreSel, false, genre (k).name, uf (10.0f));
    auto r = juce::Rectangle<float> (16, 120, 822, 300); lcd (g, r, 8);
    auto plot = r.reduced (40, 16);
    const auto& G = genre (genreSel);
    // fascia obiettivo +-2.5 dB
    juce::Path band; const int n = 80;
    for (int i = 0; i <= n; ++i) { const float f = 20.0f * std::pow (1000.0f, i / (float) n), x = plot.getX() + plot.getWidth() * i / n, y = plot.getCentreY() - juce::jlimit (-1.0f, 1.0f, (targetAt (G.off, f) + 2.5f) / 15.0f) * plot.getHeight() * 0.5f; if (i == 0) band.startNewSubPath (x, y); else band.lineTo (x, y); }
    for (int i = n; i >= 0; --i) { const float f = 20.0f * std::pow (1000.0f, i / (float) n), x = plot.getX() + plot.getWidth() * i / n, y = plot.getCentreY() - juce::jlimit (-1.0f, 1.0f, (targetAt (G.off, f) - 2.5f) / 15.0f) * plot.getHeight() * 0.5f; band.lineTo (x, y); }
    band.closeSubPath(); g.setColour (C::green.withAlpha (0.13f)); g.fillPath (band); g.setColour (C::green.withAlpha (0.4f)); g.strokePath (band, juce::PathStrokeType (1.0f));
    // regioni
    g.setFont (mf (8.5f));
    for (int k = 0; k <= kRegions; ++k)
    {
        const float x = plot.getX() + plot.getWidth() * (float) (std::log (regionEdge (k) / 20.0) / std::log (1000.0));
        g.setColour (C::phos.withAlpha (0.08f)); g.drawVerticalLine ((int) x, plot.getY(), plot.getBottom());
        if (k < kRegions) { const float x2 = plot.getX() + plot.getWidth() * (float) (std::log (regionEdge (k + 1) / 20.0) / std::log (1000.0)); g.setColour (C::dim); g.drawText (regionName (k), juce::Rectangle<float> (x, plot.getBottom() + 2, x2 - x, 12), juce::Justification::centred); }
    }
    const bool has = ed.live.hasData();
    if (has) drawThirds (g, ed.liveThirds, plot, C::amber, 15.0f, false);
    else glowText (g, "FAI SUONARE IL PEZZO", plot, C::dim, uf (14.0f), juce::Justification::centred);
    // scarti per regione
    if (has)
    {
        float reg[kRegions]; ThirdAnalyzer::regions (ed.liveThirds, reg); float m = 0; for (float o : G.off) m += o; m /= kRegions;
        const float cw = 822.0f / kRegions;
        for (int k = 0; k < kRegions; ++k)
        {
            const float d = reg[k] - (G.off[k] - m);
            auto c = juce::Rectangle<float> (16 + k * cw + 3, 432, cw - 6, 110); lcd (g, c, 6);
            g.setColour (C::dim); g.setFont (mf (9.0f)); g.drawText (regionName (k), c.reduced (8, 6).removeFromTop (12), juce::Justification::centredLeft);
            const auto col = std::abs (d) < 2.5f ? C::green : d > 0 ? C::amber : C::phos;
            glowText (g, (d > 0 ? "+" : "") + juce::String (d, 1) + " dB", { c.getX() + 8, c.getY() + 24, c.getWidth() - 16, 26 }, col, neo::big (15.0f));
            g.setColour (C::text.withAlpha (0.8f)); g.setFont (uf (10.0f, false));
            g.drawFittedText (std::abs (d) < 2.5f ? juce::String ("nella norma") : d > 0 ? juce::String ("troppo: abbassa ") + regionName (k) : juce::String ("poco: alza ") + regionName (k),
                              c.reduced (8, 0).withTrimmedTop (56).toNearestInt(), juce::Justification::topLeft, 3);
        }
    }
    g.setColour (C::dim); g.setFont (uf (10.0f, false));
    g.drawFittedText ("La linea ambra e il tuo master (media degli ultimi secondi), la fascia verde e dove stanno i master del genere. CORREGGI scrive nell'EQ dinamico sei bande che riportano il suono verso la fascia (al 70%, massimo 4 dB): poi ritocca a orecchio.",
                      juce::Rectangle<int> (540, 552, 300, 60), juce::Justification::topLeft, 4);
}

// ======================================================================= RIFERIMENTO
RefPage::RefPage (Editor& e) : ed (e)
{
    load = std::make_unique<ActBtn> ("CARICA RIFERIMENTO...", C::phos, [this]
    {
        chooser = std::make_unique<juce::FileChooser> ("Scegli un master di riferimento", juce::File::getSpecialLocation (juce::File::userMusicDirectory), "*.wav;*.aif;*.aiff;*.mp3;*.flac");
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [this] (const juce::FileChooser& fc) { if (fc.getResult().existsAsFile()) ed.proc.loadReference (fc.getResult()); });
    });
    listen = std::make_unique<ActBtn> ("ASCOLTA: MIO MASTER", C::amber, [this] { ed.proc.refListen = ! ed.proc.refListen.load(); });
    match = std::make_unique<ActBtn> ("A VOLUME UGUALE: SI", C::green, [this] { ed.proc.refMatch = ! ed.proc.refMatch.load(); });
    matchEq = std::make_unique<ActBtn> ("MATCH EQ: AVVICINA AL RIFERIMENTO", C::green, [this]
    {
        auto rt = ed.proc.reference(); if (! rt || ! ed.live.hasData()) return;
        float a[kRegions], b[kRegions], dev[kRegions]; ThirdAnalyzer::regions (ed.liveThirds, a); ThirdAnalyzer::regions (rt->thirds, b);
        for (int r = 0; r < kRegions; ++r) dev[r] = a[r] - b[r];
        ed.balPage.applyEq (dev);
    });
    for (auto* x : { load.get(), listen.get(), match.get(), matchEq.get() }) addAndMakeVisible (x);
}
void RefPage::resized() { load->setBounds (16, 52, 220, 34); listen->setBounds (246, 52, 220, 34); match->setBounds (476, 52, 180, 34); matchEq->setBounds (16, 556, 340, 34); }
void RefPage::mouseDown (const juce::MouseEvent& e)
{
    auto rt = ed.proc.reference(); auto w = waveR();
    if (rt && w.contains (e.position)) ed.proc.refPos = (double) ((e.position.x - w.getX()) / w.getWidth() * rt->buf.getNumSamples());
}
void RefPage::paint (juce::Graphics& g)
{
    auto& P = ed.proc; auto rt = P.reference();
    listen->text = P.refListen.load() ? "ASCOLTA: RIFERIMENTO" : "ASCOLTA: MIO MASTER"; listen->down = P.refListen.load();
    match->text = P.refMatch.load() ? "A VOLUME UGUALE: SI" : "A VOLUME UGUALE: NO";
    glowText (g, "TRACCIA DI RIFERIMENTO", { 16, 10, 400, 26 }, C::phos, uf (17.0f));
    auto w = waveR(); lcd (g, w, 8);
    if (P.refLoading.load()) glowText (g, "CARICO E ANALIZZO...", w, C::amber, uf (13.0f), juce::Justification::centred);
    else if (! rt) glowText (g, "CARICA UN MASTER CHE TI PIACE (WAV, AIFF, MP3, FLAC)", w, C::dim, uf (12.0f), juce::Justification::centred);
    else
    {
        const int n = (int) rt->overview.size();
        for (int i = 0; i < n; ++i) { const float x = w.getX() + w.getWidth() * i / n, h = rt->overview[(size_t) i] * (w.getHeight() - 10) * 0.5f; g.setColour (C::phos.withAlpha (0.6f)); g.drawVerticalLine ((int) x, w.getCentreY() - h, w.getCentreY() + h); }
        const float px = w.getX() + w.getWidth() * (float) (P.refPos.load() / juce::jmax (1, rt->buf.getNumSamples()));
        g.setColour (C::amber); g.fillRect (px - 1, w.getY(), 2.0f, w.getHeight());
        glowText (g, rt->name, { w.getX() + 10, w.getY() + 6, 400, 14 }, C::text, mf (10.0f));
    }
    // confronto numeri
    auto c = juce::Rectangle<float> (16, 202, 822, 70); lcd (g, c, 8);
    const float myI = P.lufsI.load() > -60 ? P.lufsI.load() : P.lufsS.load();
    auto col = [&] (int i, const juce::String& k, const juce::String& v, juce::Colour cc)
    { auto r = juce::Rectangle<float> (c.getX() + 12 + i * 200.0f, c.getY() + 8, 190, 54); g.setColour (C::dim); g.setFont (mf (9.0f)); g.drawText (k, r.removeFromTop (14), juce::Justification::centredLeft); glowText (g, v, r, cc, neo::big (15.0f)); };
    col (0, "MIO MASTER", myI > -60 ? juce::String (myI, 1) + " LUFS" : juce::String ("-"), C::amber);
    col (1, "RIFERIMENTO", rt ? juce::String (rt->lufs, 1) + " LUFS" : juce::String ("-"), C::phos);
    col (2, "DIFFERENZA", (rt && myI > -60) ? dbS (myI - rt->lufs) + " dB" : juce::String ("-"), C::text);
    col (3, "TP RIFERIMENTO", rt ? juce::String (rt->tp, 1) + " dBTP" : juce::String ("-"), C::text);
    // spettri a confronto
    auto r = juce::Rectangle<float> (16, 284, 822, 260); lcd (g, r, 8);
    auto plot = r.reduced (30, 14);
    for (int k = 0; k <= kRegions; ++k) { const float x = plot.getX() + plot.getWidth() * (float) (std::log (regionEdge (k) / 20.0) / std::log (1000.0)); g.setColour (C::phos.withAlpha (0.08f)); g.drawVerticalLine ((int) x, plot.getY(), plot.getBottom()); }
    if (rt) drawThirds (g, rt->thirds, plot, C::phos, 15.0f, true);
    if (ed.live.hasData()) drawThirds (g, ed.liveThirds, plot, C::amber, 15.0f, false);
    g.setColour (C::dim); g.setFont (mf (9.0f)); g.drawText ("AMBRA = mio master   AZZURRO = riferimento   (spettro medio, a volume normalizzato)", juce::Rectangle<float> (plot.getX(), r.getBottom() - 14, 600, 12), juce::Justification::centredLeft);
    g.setColour (C::dim); g.setFont (uf (10.0f, false));
    g.drawFittedText ("Clicca sulla forma d'onda per scegliere il punto. A VOLUME UGUALE porta il riferimento alla loudness del tuo master: confronti il suono, non il volume. MATCH EQ scrive nell'EQ dinamico sei bande che avvicinano il tuo bilanciamento al riferimento.",
                      juce::Rectangle<int> (370, 552, 470, 60), juce::Justification::topLeft, 4);
}

void SoonPage::paint (juce::Graphics& g)
{
    const auto& mi = modInfo (mod); const auto c = juce::Colour (mi.colour);
    glowText (g, mi.name, { 16, 10, 400, 26 }, c, uf (17.0f));
    auto r = getLocalBounds().toFloat().reduced (16).withTrimmedTop (40); lcd (g, r, 10);
    glowText (g, "IN ARRIVO", r.withHeight (r.getHeight() * 0.5f).withY (r.getY() + r.getHeight() * 0.2f), c, neo::big (30.0f), juce::Justification::centred);
    g.setColour (C::dim); g.setFont (uf (12.0f, false));
    g.drawFittedText (juce::String (mi.name) + ": " + mi.sub + ". Questo modulo arriva nei prossimi aggiornamenti e prendera il suo posto nella catena.",
                      r.reduced (60, 0).withTrimmedTop (r.getHeight() * 0.55f).toNearestInt(), juce::Justification::centredTop, 3);
    scanlines (g, r);
}

// ======================================================================= loudness
void LoudPanel::paint (juce::Graphics& g)
{
    auto& P = ed.proc; const auto& tg = target ((int) P.value ("target"));
    glowText (g, "LOUDNESS", { 16, 8, 200, 26 }, C::text, uf (13.0f));
    btn (g, resetR(), C::phos, false, false, "AZZERA", uf (10.5f));
    const float I = P.lufsI.load(), S = P.lufsS.load(), M = P.lufsM.load(), tp = P.tpMax.load(), lra = P.lra.load();
    auto big = juce::Rectangle<float> (16, 40, (float) getWidth() - 32, 92); lcd (g, big, 10);
    g.setColour (C::dim); g.setFont (mf (9.0f)); g.drawText ("INTEGRATO", big.reduced (14, 8).removeFromTop (12), juce::Justification::centredLeft);
    const float dv = I - tg.lufs; const bool okT = I > -60 && std::abs (dv) < 1.0f;
    glowText (g, lufsS (I), { big.getX() + 14, big.getY() + 20, 180, 50 }, I < -60 ? C::faint : okT ? C::green : (dv > 0 ? C::amber : C::phos), neo::big (34.0f));
    g.setColour (C::dim); g.setFont (mf (11.0f)); g.drawText ("LUFS", juce::Rectangle<float> (big.getX() + 150, big.getY() + 40, 60, 20), juce::Justification::centredLeft);
    juce::String msg;
    if (I < -60) msg = "fai suonare il pezzo dall'inizio";
    else if (dv > 0.5f) msg = juce::String (tg.name) + " lo abbassera di " + juce::String (dv, 1) + " dB";
    else if (dv < -0.5f) msg = juce::String (dv, 1) + " dB sotto " + juce::String (tg.name);
    else msg = "perfetto per " + juce::String (tg.name);
    glowText (g, msg, { big.getX() + 14, big.getBottom() - 24, big.getWidth() - 28, 18 }, okT ? C::green : C::amber, uf (11.5f));
    // valori
    struct V { const char* k; juce::String v; juce::Colour c; };
    const float plr = (tp > -60 && I > -60) ? tp - I : 0.0f;
    V vals[6] = { { "BREVE", lufsS (S), C::text }, { "MOMENT.", lufsS (M), C::text }, { "TRUE PEAK", tp < -60 ? juce::String ("-") : juce::String (tp, 1), tp > tg.tp + 0.05f ? C::red : C::text },
                  { "LRA", juce::String (lra, 1) + " LU", C::text }, { "PLR", juce::String (plr, 1) + " dB", plr < 7 && I > -60 ? C::amber : C::text }, { "PIU FORTE DI", dbS (P.matchDb.load()) + " dB", C::phos } };
    const float cw = (getWidth() - 32 - 8) / 3.0f;
    for (int i = 0; i < 6; ++i)
    {
        auto c = juce::Rectangle<float> (16 + (i % 3) * (cw + 4), 142.0f + (i / 3) * 58.0f, cw, 52); lcd (g, c, 6);
        g.setColour (C::dim); g.setFont (mf (8.5f)); g.drawText (vals[i].k, c.reduced (8, 5).removeFromTop (11), juce::Justification::centredLeft);
        glowText (g, vals[i].v, c.reduced (8, 0).withTrimmedTop (20), vals[i].c, mf (14.0f));
    }
    // target
    g.setColour (C::dim); g.setFont (mf (9.0f)); g.drawText ("TARGET", juce::Rectangle<float> (16, 254, 100, 12), juce::Justification::centredLeft);
    for (int t = 0; t < kNumTargets; ++t)
    {
        auto r = tgtCell (t); const bool on = (int) P.value ("target") == t;
        btn (g, r, C::green, on, false, juce::String (target (t).name) + " " + juce::String ((int) target (t).lufs), uf (9.5f));
    }
    // storia breve
    auto h = juce::Rectangle<float> (16, 330, (float) getWidth() - 32, 62); lcd (g, h, 6);
    const int n = Processor::kHist, w0 = P.hW.load();
    auto y = [&] (float v) { return h.getBottom() - 4 - juce::jlimit (0.0f, 1.0f, (v + 30) / 26) * (h.getHeight() - 8); };
    g.setColour (C::green.withAlpha (0.6f)); const float yt = y (tg.lufs); for (float x = h.getX() + 4; x < h.getRight() - 4; x += 6) g.drawHorizontalLine ((int) yt, x, x + 2);
    juce::Path p; bool st = false;
    for (int i = 0; i < n; i += 8) { const float v = P.hS[(w0 + i) % n].load(); if (v < -60) { st = false; continue; } const float x = h.getX() + 4 + (h.getWidth() - 8) * i / (n - 1); if (! st) { p.startNewSubPath (x, y (v)); st = true; } else p.lineTo (x, y (v)); }
    g.setColour (C::amber); g.strokePath (p, juce::PathStrokeType (1.4f));
    scanlines (g, h);
}
void LoudPanel::mouseDown (const juce::MouseEvent& e)
{
    if (resetR().contains (e.position)) { ed.proc.resetReq = true; return; }
    for (int t = 0; t < kNumTargets; ++t) if (tgtCell (t).contains (e.position)) { auto* p = ed.P ("target"); p->setValueNotifyingHost (p->convertTo0to1 ((float) t)); repaint(); return; }
}

// ======================================================================= Editor
Editor::Editor (Processor& p) : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);
    trim = std::make_unique<Knob> (P ("trim"), C::phos); trim->label = "INGRESSO"; trim->bipolar = true; trim->text = [this] { return dbS (proc.value ("trim")) + " dB"; };
    compare = std::make_unique<Switch> (P ("compare"), C::phos, "CONFRONTA");
    norm = std::make_unique<Switch> (P ("norm"), C::green, "PIATTAFORMA");
    dither = std::make_unique<Segs> (P ("dither"), C::violet, juce::StringArray { "OFF", "24 BIT", "16 BIT", "16 SH" }); dither->font = 9.0f; dither->rows = 2;
    for (juce::Component* c : std::initializer_list<juce::Component*> { &chain, &maxPage, &limPage, &bassPage, &eqPage, &imPage, &dsPage, &busPage, &mbPage, &tpPage, &exPage, &e4Page, &pqPage, &vmPage, &balPage, &refPage, &soon, &loud, trim.get(), compare.get(), norm.get(), dither.get() }) addAndMakeVisible (c);
    addAndMakeVisible (sizeChip);
    addChildComponent (easyPage); addChildComponent (advStage); addAndMakeVisible (advTabsComp);
    for (juce::Component* c : std::initializer_list<juce::Component*> { &rack, &status }) addChildComponent (c);
    addChildComponent (gallery); addChildComponent (browser);
    chain.setVisible (false); loud.setVisible (false); trim->setVisible (false);
    for (int m = 0; m < kNumMods; ++m) if (proc.value (onParam (m)) > 0.5f) proc.inRack[m] = true;
    { const auto v = rack.visible(); sel = v.empty() ? MAXIM : v.back(); }
    select (sel);
    lastEasy = ! proc.easy(); applyMode();
    scaler.setup();
    startTimerHz (30);
}
void Editor::applyMode()
{
    const bool e = proc.easy(); lastEasy = e;
    easyPage.setVisible (e); advTabsComp.setVisible (! e); advStage.setVisible (false); gallery.setVisible (false);
    for (juce::Component* c : std::initializer_list<juce::Component*> { compare.get(), norm.get(), &rack, &status }) c->setVisible (! e);
    dither->setVisible (false);
    // il display e lo stesso componente della modalita FACILE: in AVANZATO lo si sposta qui, cosi onde, spettro e misure sono identiche
    auto& st = easyPage.stage; juce::Component* parts[] = { &st, &easyPage.input, easyPage.trim };
    if (e)
    {
        for (auto* c : parts) { c->setTransform ({}); easyPage.addAndMakeVisible (c); }
        easyPage.trim->toFront (false); easyPage.input.toFront (false); easyPage.resized();
        for (auto* pg : std::initializer_list<juce::Component*> { &maxPage, &limPage, &bassPage, &eqPage, &imPage, &dsPage, &busPage, &mbPage, &tpPage, &exPage, &e4Page, &pqPage, &vmPage, &balPage, &refPage, &soon }) pg->setVisible (false);
    }
    else
    {
        for (auto* c : parts) addChildComponent (c);
        if (advView < kEff) advView = juce::jlimit (0, kEff - 1, easyPage.view);
        layoutAdv(); applyAdvView();
    }
    browser.toFront (false);
    repaint();
}
void Editor::layoutAdv()
{
    const float s = scaler.s; const auto c = centreR;
    easyPage.stage.setBounds (c);
    easyPage.trim->setBounds (c.getX() - 12, c.getY() - 12, 120, 110);
    easyPage.input.setBounds (c.getX() + 92, c.getY() + 10, 250, 46);
    for (juce::Component* k : { (juce::Component*) &easyPage.stage, (juce::Component*) &easyPage.input, (juce::Component*) easyPage.trim }) k->setTransform (juce::AffineTransform::scale (s));
}
void Editor::openEffect (int m)
{
    advView = kEff; select (m); applyAdvView();
}
void Editor::insertModule (int m)
{
    if (m < 0 || m >= kNumMods) return;
    // posto naturale: dopo l'ultimo effetto in catena che nell'ordine di default lo precede
    auto o = proc.getOrder(); std::vector<int> v (o.begin(), o.end());
    v.erase (std::find (v.begin(), v.end(), m));
    const auto def = Processor::defaultOrder(); const int dp = (int) (std::find (def.begin(), def.end(), m) - def.begin());
    int pos = 0;
    for (int k = 0; k < (int) v.size(); ++k) { const int dk = (int) (std::find (def.begin(), def.end(), v[(size_t) k]) - def.begin()); if (proc.inRack[v[(size_t) k]].load() && dk < dp) pos = k + 1; }
    v.insert (v.begin() + pos, m);
    std::array<int, kNumMods> a {}; for (int k = 0; k < kNumMods; ++k) a[(size_t) k] = v[(size_t) k]; proc.setOrder (a);
    proc.inRack[m] = true; P (onParam (m))->setValueNotifyingHost (1.0f);
    gallery.setVisible (false); openEffect (m); rack.repaint();
}
void Editor::removeModule (int m)
{
    if (m < 0 || m >= kNumMods) return;
    P (onParam (m))->setValueNotifyingHost (0.0f); P (deltaParam (m))->setValueNotifyingHost (0.0f); proc.inRack[m] = false;
    if (sel == m) { const auto v = rack.visible(); if (! v.empty()) sel = v.front(); advView = juce::jlimit (0, kEff - 1, easyPage.view); applyAdvView(); }
    rack.repaint();
}
void Editor::select (int m)
{
    sel = m;
    if (m != DYNEQ) proc.eqSolo = -1;   // il solo dell'EQ vale solo mentre sei sulla pagina EQ
    maxPage.setVisible (m == MAXIM); limPage.setVisible (m == LIMIT); bassPage.setVisible (m == BASS); eqPage.setVisible (m == DYNEQ); imPage.setVisible (m == IMAGER);
    dsPage.setVisible (m == DEESS); busPage.setVisible (m == BUSCOMP); mbPage.setVisible (m == MULTI); tpPage.setVisible (m == TAPE); exPage.setVisible (m == EXCITER); e4Page.setVisible (m == EQ432); pqPage.setVisible (m == PULTEC); vmPage.setVisible (m == VARIMU); balPage.setVisible (m == TOOL_BAL); refPage.setVisible (m == TOOL_REF);
    soon.setVisible (m < kNumMods && ! modInfo (m).ready); soon.mod = m < kNumMods ? m : 0;
    if (proc.easy() || advView != kEff) for (auto* pg : std::initializer_list<juce::Component*> { &maxPage, &limPage, &bassPage, &eqPage, &imPage, &dsPage, &busPage, &mbPage, &tpPage, &exPage, &e4Page, &pqPage, &vmPage, &balPage, &refPage, &soon }) pg->setVisible (false);
    advStage.setVisible (false); advTabsComp.toFront (false); advTabsComp.repaint();
    rack.repaint(); status.repaint(); repaint();
}
// AVANZATO: al posto della pagina del modulo, le viste della modalita FACILE (onda, spettro, gain)
// stesse viste della modalita FACILE, piu la linguetta dell'effetto aperto
static juce::StringArray advViewNames (const Editor& e)
{
    juce::StringArray a { "ONDA", "GAIN", "SPETTRO", "IMMAGINE", "METERING", "IDEALE", "REFERENCE" };
    a.add (e.sel < kNumMods ? juce::String (modInfo (e.sel).name) : e.sel == TOOL_BAL ? juce::String ("BILANCIO") : juce::String ("RIFERIMENTO"));
    return a;
}
static juce::Point<float> kTabsOrigin { 260, 72 };
void Editor::AdvTabsComp::paint (juce::Graphics& g) { g.addTransform (juce::AffineTransform::translation (-kTabsOrigin.x, -kTabsOrigin.y)); ed.advTabs.paint (g, advViewNames (ed), ed.advView, C::violet, 7.5f); }
void Editor::AdvTabsComp::mouseDown (const juce::MouseEvent& e)
{
    const int t = ed.advTabs.hit (e.position + kTabsOrigin);
    if (t < 0) return;
    ed.advView = t; ed.applyAdvView();
    repaint();
}
void Editor::applyAdvView()
{
    const bool show = ! proc.easy() && advView < kEff;
    if (show) easyPage.view = advView;   // la vista scelta vale anche nel FACILE
    for (juce::Component* c : { (juce::Component*) &easyPage.stage, (juce::Component*) &easyPage.input, (juce::Component*) easyPage.trim }) c->setVisible (show);
    if (show) { easyPage.trim->toFront (false); easyPage.input.toFront (false); }
    if (! show) { select (sel); return; }
    if (show) for (auto* pg : std::initializer_list<juce::Component*> { &maxPage, &limPage, &bassPage, &eqPage, &imPage, &dsPage, &busPage, &mbPage, &tpPage, &exPage, &e4Page, &pqPage, &vmPage, &balPage, &refPage, &soon }) pg->setVisible (false);
    repaint();
}
void Editor::resized()
{
    const float sc = scaler.update();
    presetRect = { 474, 15, 300, 36 }; prevRect = { 438, 15, 34, 36 }; nextRect = { 776, 15, 34, 36 }; abRect = { 818, 15, 122, 36 };
    balRect = { 956, 15, 150, 36 }; refRect = { 1112, 15, 150, 36 };
    presetRect = { 486, 12, 380, 42 }; prevRect = { 446, 15, 36, 36 }; nextRect = { 870, 15, 36, 36 }; abRect = { 916, 15, 122, 36 };
    balRect = {}; refRect = {};
    for (auto* pg : std::initializer_list<juce::Component*> { &maxPage, &limPage, &bassPage, &eqPage, &imPage, &dsPage, &busPage, &mbPage, &tpPage, &exPage, &e4Page, &pqPage, &vmPage, &balPage, &refPage, &soon }) pg->setBounds (centreR);
    easyPage.setBounds (0, 64, 1280, 736); neoBg = {};
    rack.setBounds (16, 72, 232, 662);
    status.setBounds (16, 744, 1248, 34);
    gallery.setBounds (centreR.withTrimmedTop (-34));
    browser.setBounds (0, 0, W, H);
    for (int t = 0; t < kNumTargets; ++t) tgtR[t] = { 1136, 136 + t * 30, 118, 26 };
    advTabs.layout ({ 0, 74, 0, 26 }, advViewNames (*this), 7.5f); advTabs.ax = -1;
    { const float x0 = (float) centreR.getX() + 6 - advTabs.r[0].getX(); for (int i = 0; i < advTabs.n; ++i) advTabs.r[i].translate (x0, 0); kTabsOrigin = { (float) centreR.getX(), 70 }; }
    advTabsComp.setBounds (juce::Rectangle<float> (kTabsOrigin.x, kTabsOrigin.y, (float) centreR.getWidth(), 34).toNearestInt()); advTabsComp.toFront (false);
    modeSeg.layout ({ 282, 18, 0, 28 }, juce::StringArray { "FACILE", "AVANZATO" }, 9.0f); modeR[0] = modeSeg.r[0].toNearestInt(); modeR[1] = modeSeg.r[1].toNearestInt();
    compare->setBounds (1136, 360, 126, 22); norm->setBounds (1136, 388, 126, 22);
    dither->setBounds (0, 0, 0, 0);
    for (int d = 0; d < 4; ++d) dthR[d] = { 1136 + (d % 2) * 61, 460 + (d / 2) * 30, 57, 26 };
    sizeChip.setBounds (W - 76, H - 18, 58, 15);
    jlook::scaleChildren (*this, sc);
    if (! proc.easy()) layoutAdv();
}

void Editor::paint (juce::Graphics& g)
{
    neo::setTheme (proc.uiTheme.load() == 2 ? 2 : 1); C::sync();
    g.addTransform (juce::AffineTransform::scale (scaler.s));
    paintNeo (g);   // stesso sfondo e stessa testata in tutte e due le modalita
    if (proc.easy()) return;
    const auto mc = sel < kNumMods ? neo::acc (juce::Colour (modInfo (sel).colour)) : (sel == TOOL_BAL ? C::green : C::phos);
    auto card = [&] (juce::Rectangle<float> r, juce::Colour c)
    {   // zona senza scatola: velo scuro, un filo di luce del colore del modulo in basso
        juce::ColourGradient zg (veil (0.30f), r.getX(), r.getY(), veil (0.12f), r.getX(), r.getBottom(), false); g.setGradientFill (zg); g.fillRoundedRectangle (r, 16);
        g.setColour (glass (0.04f)); g.drawRoundedRectangle (r.reduced (0.5f), 16, 1.0f);
        juce::ColourGradient hl (c.withAlpha (0.0f), r.getX() + 30, 0, c.withAlpha (0.0f), r.getRight() - 30, 0, false); hl.addColour (0.5, c.withAlpha (0.55f));
        g.setGradientFill (hl); g.fillRect (r.getX() + 30, r.getBottom() - 1.0f, r.getWidth() - 60, 1.0f);
    };
    card (rack.getBounds().toFloat(), C::violet);
    if (advView == kEff) card (centreR.toFloat(), mc);
    // colonna d'uscita: obiettivo di loudness, confronto, dither
    auto out = juce::Rectangle<float> (1126, 72, 138, 662); card (out, C::phos);
    g.setFont (neo::cap (8.5f)); g.setColour (C::text); g.drawText ("USCITA", out.withHeight (30).withTrimmedLeft (10), juce::Justification::centredLeft);
    g.setFont (neo::cap (7.0f)); g.setColour (C::dim); g.drawText ("OBIETTIVO", juce::Rectangle<float> (1136, 116, 118, 14), juce::Justification::centredLeft);
    const int tg = (int) proc.value ("target");
    for (int t = 0; t < kNumTargets; ++t) neo::chip (g, tgtR[t].toFloat(), juce::String (target (t).name) + "  " + juce::String ((int) target (t).lufs), t == tg, C::green, 7.5f);
    g.setFont (neo::cap (7.0f)); g.setColour (C::dim); g.drawText ("ASCOLTO", juce::Rectangle<float> (1136, 340, 118, 14), juce::Justification::centredLeft);
    g.drawText ("DITHER (EXPORT)", juce::Rectangle<float> (1136, 442, 118, 14), juce::Justification::centredLeft);
    { static const char* dn[] = { "OFF", "24 BIT", "16 BIT", "16 SH" }; const int dv = (int) proc.value ("dither");
      for (int d = 0; d < 4; ++d) neo::chip (g, dthR[d].toFloat(), dn[d], d == dv, C::violet, 7.0f); }
    g.setFont (neo::f (9.0f)); g.setColour (C::dim);
    g.drawFittedText ("CONFRONTA: senti l'originale allo stesso volume. PIATTAFORMA: come lo abbassa lo streaming. Spegnili prima di esportare.", juce::Rectangle<int> (1136, 530, 118, 120), juce::Justification::topLeft, 8, 0.9f);
    g.setFont (neo::num (8.5f)); g.drawFittedText ("latenza " + juce::String (proc.getLatencySamples() * 1000.0 / juce::jmax (1.0, proc.sampleRate()), 1) + " ms", juce::Rectangle<int> (1136, 700, 118, 20), juce::Justification::centredLeft, 1, 0.8f);
}

// ---------------------------------------------------------------------------------------------------- FACILE: sfondo e testata neo
void Editor::paintNeo (juce::Graphics& g)
{
    using namespace neo;
    const float sc = juce::jlimit (1.0f, 4.0f, g.getInternalContext().getPhysicalPixelScaleFactor());
    if (neoBg.isNull() || sc > neoScale * 1.05f || neoTheme != K::theme)   // si ricostruisce solo se serve piu definizione o cambia il tema
    {
        neoScale = sc; neoTheme = K::theme; neoBg = juce::Image (juce::Image::RGB, juce::roundToInt (W * sc), juce::roundToInt (H * sc), false);
        juce::Graphics ig (neoBg); ig.addTransform (juce::AffineTransform::scale (sc));
        juce::ColourGradient base (K::bg1, 0, 0, K::bg0, 0, (float) H, false); ig.setGradientFill (base); ig.fillAll();
        // aurora: grandi campi di colore sfocati, nessun bordo
        juce::Image au (juce::Image::ARGB, W / 8, H / 8, true);
        {
            juce::Graphics ag (au); ag.addTransform (juce::AffineTransform::scale (1.0f / 8));
            auto blob = [&ag] (float x, float y, float rx, float ry, juce::Colour c, float a) { ag.setColour (c.withAlpha (a)); ag.fillEllipse (x - rx, y - ry, rx * 2, ry * 2); };
            const float la = light() ? 0.35f : 0.6f;
            blob (300, 190, 380, 170, K::violet, 0.28f * la); blob (980, 140, 300, 150, K::pink, 0.16f * la); blob (700, 300, 360, 90, K::blue, 0.18f * la);
            blob (640, 760, 520, 120, K::violet, 0.16f * la); blob (90, 700, 200, 120, K::cyan, 0.06f * la); blob (1220, 620, 200, 160, K::pink, 0.07f * la);
        }
        neo::blur (au, 10, 3);
        ig.setImageResamplingQuality (juce::Graphics::highResamplingQuality); ig.drawImage (au, juce::Rectangle<float> (0, 0, (float) W, (float) H));
        // vignetta
        juce::ColourGradient vg (juce::Colours::transparentBlack, W * 0.5f, H * 0.45f, K::lo.withAlpha (light() ? 0.12f : 0.45f), 0, 0, true); ig.setGradientFill (vg); ig.fillAll();
        // grana finissima
        juce::Random rnd (7);
        for (int i = 0; i < 14000; ++i) { ig.setColour (K::hi.withAlpha (rnd.nextFloat() * 0.03f)); ig.fillRect (rnd.nextFloat() * W, rnd.nextFloat() * H, 1.0f, 1.0f); }
        // logo
        { juce::Path ring; ring.addCentredArc (38, 32, 12, 12, 0, 0.5f, 6.2832f - 0.2f, true); juce::ColourGradient lg (K::pink, 26, 44, K::cyan, 50, 20, false);
          ig.setGradientFill (lg); ig.setOpacity (0.25f); ig.strokePath (ring, juce::PathStrokeType (7.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded)); ig.setOpacity (1.0f);
          ig.strokePath (ring, juce::PathStrokeType (2.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded)); ig.fillEllipse (34, 28, 8, 8); }
        ig.setFont (neo::num (15.0f)); ig.setColour (K::ink); ig.drawText ("JACKPOT", juce::Rectangle<float> (62, 15, 100, 20), juce::Justification::centredLeft);
        ig.setFont (neo::big (15.0f)); ig.setColour (K::sub); ig.drawText ("Master", juce::Rectangle<float> (139, 15, 100, 20), juce::Justification::centredLeft);
        ig.setFont (neo::cap (7.5f)); ig.setColour (K::faint); ig.drawText ("ONE KNOB MASTERING", juce::Rectangle<float> (62, 35, 200, 12), juce::Justification::centredLeft);
    }
    { g.saveState(); g.addTransform (juce::AffineTransform::scale (1.0f / neoScale)); g.drawImageAt (neoBg, 0, 0); g.restoreState(); }
    static const juce::StringArray modes { "FACILE", "AVANZATO" };
    modeSeg.paint (g, modes, proc.easy() ? 0 : 1, K::pink, 9.0f);
    // preset: solo testo, frecce discrete
    { auto nm = proc.getProgramName (proc.preset); juce::String grp;
      if (nm.contains (" | ")) { grp = nm.upToFirstOccurrenceOf (" | ", false, false); nm = nm.fromFirstOccurrenceOf (" | ", false, false); }
      auto pr = presetRect.toFloat();
      g.setFont (neo::cap (7.5f)); g.setColour (K::faint); g.drawText (grp.isEmpty() ? juce::String ("PRESET") : juce::String::fromUTF8 ("PRESET  \xc2\xb7  ") + grp.toUpperCase(), pr.withHeight (12).translated (0, 1), juce::Justification::centred);
      g.setFont (neo::f (15.0f, 1)); g.setColour (K::ink); g.drawText (nm, pr.withTrimmedTop (12), juce::Justification::centred);
      g.setFont (neo::f (18.0f)); g.setColour (K::sub);
      g.drawText (juce::String::fromUTF8 ("\xe2\x80\xb9"), prevRect, juce::Justification::centred); g.drawText (juce::String::fromUTF8 ("\xe2\x80\xba"), nextRect, juce::Justification::centred); }
    neo::chip (g, abRect.toFloat().reduced (8, 4), "MEMORIA " + proc.ab.label(), proc.ab.cur == 1, K::cyan, 9.0f, abHover);
    if (proc.easy()) { const float gr = proc.grAvg.load(); const bool on = proc.lufsS.load() > -60;
      auto r = juce::Rectangle<float> (1100, 22, 160, 20); const auto c = on ? (gr > 3 ? K::pink : K::mint) : K::faint;
      g.setColour (c.withAlpha (0.25f)); g.fillEllipse (r.getX() - 2, r.getCentreY() - 7, 14, 14); g.setColour (c); g.fillEllipse (r.getX() + 2, r.getCentreY() - 3, 6, 6);
      g.setFont (neo::cap (8.5f)); g.setColour (K::sub); g.drawText (on ? (gr > 0.05f ? "LIMITER  -" + juce::String (gr, 1) + " dB" : juce::String ("SEGNALE OK")) : juce::String ("IN ATTESA"), r.withTrimmedLeft (18), juce::Justification::centredLeft); }
}

void Editor::mouseMove (const juce::MouseEvent& e0)
{
    const auto e = e0.withNewPosition (e0.position / scaler.s);
    const bool h = abRect.contains (e.getPosition());
    if (h != abHover) { abHover = h; repaint (scaler.map (abRect.expanded (8))); }
    setMouseCursor (h || presetRect.contains (e.getPosition()) || prevRect.contains (e.getPosition()) || nextRect.contains (e.getPosition()) ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
}
void Editor::mouseDown (const juce::MouseEvent& e0)
{
    const auto e = e0.withNewPosition (e0.position / scaler.s);
    const int n = (int) presets().size();
    if (modeR[0].contains (e.getPosition()) || modeR[1].contains (e.getPosition())) { auto* p = P ("mode"); p->setValueNotifyingHost (p->convertTo0to1 (modeR[0].contains (e.getPosition()) ? 0.0f : 1.0f)); applyMode(); return; }
    if (! proc.easy()) for (int d = 0; d < 4; ++d) if (dthR[d].contains (e.getPosition())) { auto* p = P ("dither"); p->setValueNotifyingHost (p->convertTo0to1 ((float) d)); repaint(); return; }
    if (! proc.easy()) for (int t = 0; t < kNumTargets; ++t) if (tgtR[t].contains (e.getPosition())) { auto* p = P ("target"); p->setValueNotifyingHost (p->convertTo0to1 ((float) t)); repaint(); return; }
    if (abRect.contains (e.getPosition())) { proc.ab.toggle (proc); repaint(); }
    else if (proc.easy() && (balRect.contains (e.getPosition()) || refRect.contains (e.getPosition()))) return;
    else if (balRect.contains (e.getPosition())) { advView = 0; select (TOOL_BAL); }
    else if (refRect.contains (e.getPosition())) { advView = 0; select (TOOL_REF); }
    else if (prevRect.contains (e.getPosition())) { proc.loadPreset ((proc.preset + n - 1) % n); repaint(); }
    else if (nextRect.contains (e.getPosition())) { proc.loadPreset ((proc.preset + 1) % n); repaint(); }
    else if (presetRect.contains (e.getPosition())) browser.open();
    else if (false)
    {
        juce::PopupMenu m; std::vector<std::pair<juce::String, juce::PopupMenu>> groups;
        for (int i = 0; i < n; ++i)
        {
            const juce::String name = presets()[(size_t) i].name;
            if (! name.contains (" | ")) { m.addItem (i + 1, name, true, i == proc.preset); continue; }
            const auto gn = name.upToFirstOccurrenceOf (" | ", false, false), it = name.fromFirstOccurrenceOf (" | ", false, false);
            bool found = false; for (auto& gr : groups) if (gr.first == gn) { gr.second.addItem (i + 1, it, true, i == proc.preset); found = true; }
            if (! found) { juce::PopupMenu pm; pm.addItem (i + 1, it, true, i == proc.preset); groups.push_back ({ gn, pm }); }
        }
        for (auto& gr : groups) m.addSubMenu (gr.first, gr.second);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withTargetScreenArea (localAreaToGlobal (scaler.map (presetRect))), [this] (int r) { if (r > 0) { proc.loadPreset (r - 1); repaint(); } });
    }
}

void Editor::timerCallback()
{
    if (proc.easy() != lastEasy) applyMode();
    if (proc.easy())
    {
        easyPage.tick();
        if (tick % 3 == 0)
        {   // spettro medio dell'uscita (serve al confronto col riferimento)
            float l[4096], r[4096]; proc.ringOut.read (l, r, 4096);
            float e = 0; for (int i = 0; i < 4096; ++i) { l[i] = 0.5f * (l[i] + r[i]); e += l[i] * l[i]; }
            if (e > 4096 * 1e-7f) { live.addFrame (l, proc.sampleRate(), live.frames < 10 ? 0.3 : 0.03); live.result (liveThirds); }
        }
        if (modeSeg.step (0)) repaint (scaler.map ({ 270, 8, 200, 50 }));
        if (++tick % 10 == 0) repaint (scaler.map ({ 260, 8, 1020, 50 }));
        return;
    }
    if (modeSeg.step (1)) repaint (scaler.map ({ 270, 8, 200, 50 }));
    // il display condiviso col FACILE si aggiorna come la' (stesso tick)
    if (easyPage.stage.isVisible()) easyPage.tick();
    // un effetto acceso da automazione o preset entra in catena da solo
    { bool ch = false; for (int m = 0; m < kNumMods; ++m) if (! proc.inRack[m].load() && proc.value (onParam (m)) > 0.5f) { proc.inRack[m] = true; ch = true; } if (ch) rack.repaint(); }
    if (tick % 3 == 0) { rack.repaint(); status.repaint(); }
    maxPage.gain->activity = juce::jlimit (0.0f, 1.0f, proc.grMax.load() / 6.0f);
    limPage.ceil->activity = juce::jlimit (0.0f, 1.0f, proc.grLim.load() / 3.0f);
    if (maxPage.isVisible()) maxPage.repaint();
    if (limPage.isVisible()) limPage.repaint();
    if (eqPage.isVisible()) eqPage.view.tick();
    for (auto* pg : std::initializer_list<juce::Component*> { &bassPage, &imPage, &dsPage, &busPage, &mbPage, &tpPage, &exPage, &e4Page, &pqPage, &vmPage, &balPage, &refPage }) if (pg->isVisible()) pg->repaint();
    if (tick % 3 == 0)
    {   // spettro medio dell'uscita per bilanciamento e riferimento
        float l[4096], r[4096]; proc.ringOut.read (l, r, 4096);
        float e = 0; for (int i = 0; i < 4096; ++i) { l[i] = 0.5f * (l[i] + r[i]); e += l[i] * l[i]; }
        if (e > 4096 * 1e-7f) { live.addFrame (l, proc.sampleRate(), live.frames < 10 ? 0.3 : 0.03); live.result (liveThirds); }
    }
    if (tick % 6 == 0) { compare->repaint(); norm->repaint(); dither->repaint(); }
    if (advTabs.step (advView)) advTabsComp.repaint();
    if (++tick % 10 == 0) { repaint (scaler.map ({ 950, 8, 320, 50 })); repaint (scaler.map ({ 1126, 72, 138, 662 })); }
}
}
