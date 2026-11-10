#include "MasterEditor.h"

namespace jms
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
    g.setColour (C::phos.withAlpha (0.03f));
    for (float x = r.getX() + 10; x < r.getRight(); x += 12) g.drawVerticalLine ((int) x, r.getY() + 2, r.getBottom() - 2);
    for (float y = r.getY() + 10; y < r.getBottom(); y += 12) g.drawHorizontalLine ((int) y, r.getX() + 2, r.getRight() - 2);
}
static void scanlines (juce::Graphics& g, juce::Rectangle<float> r)
{
    g.setColour (juce::Colours::black.withAlpha (0.16f)); for (float y = r.getY(); y < r.getBottom(); y += 3) g.drawHorizontalLine ((int) y, r.getX(), r.getRight());
    jlook::glass (g, r, 6);
}
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
{
    const float lh = big ? 32.0f : 27.0f;
    const float size = juce::jmin ((float) getWidth(), getHeight() - lh);
    jlook::KnobLook kl; kl.big = big; kl.bipolar = bipolar; kl.hover = hover ? 1.0f : 0.0f; kl.active = drag ? 1.0f : 0.0f; kl.activity = activity;
    jlook::drawKnob (g, { (getWidth() - size) * 0.5f, 0, size, size }, param->getValue(), col, kl);
    g.setColour (C::dim); g.setFont (uf (big ? 11.5f : 9.5f)); g.drawText (label, juce::Rectangle<float> (0, size + 1, (float) getWidth(), 14), juce::Justification::centred);
    glowText (g, text ? text() : param->getCurrentValueAsText(), juce::Rectangle<float> (0, size + 15, (float) getWidth(), 14), drag ? col.brighter (0.4f) : col, mf (big ? 12.5f : 10.5f), juce::Justification::centred);
}
void Segs::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat(); lcd (g, b, 6);
    const int n = juce::jmax (1, items.size()), cur = juce::roundToInt (param->convertFrom0to1 (param->getValue()));
    const float w = (b.getWidth() - 6) / n;
    for (int i = 0; i < n; ++i)
    {
        auto r = juce::Rectangle<float> (3 + i * w, 3, w, b.getHeight() - 6).reduced (1, 0);
        if (i == cur) { g.setColour (col.withAlpha (0.18f)); g.fillRoundedRectangle (r, 4); g.setColour (col.withAlpha (0.85f)); g.drawRoundedRectangle (r, 4, 1.0f); glowText (g, items[i], r, col, uf (font), juce::Justification::centred); }
        else { g.setColour (C::dim); g.setFont (uf (font)); g.drawText (items[i], r, juce::Justification::centred); }
    }
    g.setColour (juce::Colours::black.withAlpha (0.15f)); for (float y = b.getY() + 2; y < b.getBottom(); y += 3) g.drawHorizontalLine ((int) y, b.getX() + 2, b.getRight() - 2);
}
void Segs::mouseDown (const juce::MouseEvent& e)
{
    const int n = juce::jmax (1, items.size()); const int k = juce::jlimit (0, n - 1, (int) ((e.position.x - 3) / ((getWidth() - 6) / (float) n)));
    param->setValueNotifyingHost (param->convertTo0to1 ((float) k)); repaint();
}
void Switch::paint (juce::Graphics& g)
{
    const bool on = param->getValue() > 0.5f; const float h = (float) juce::jmin (getHeight(), 20), w = h * 1.8f;
    auto t = juce::Rectangle<float> (0, (getHeight() - h) / 2, w, h);
    lcd (g, t, h / 2);
    const float d = h - 6, x = on ? t.getRight() - d - 3 : 3.0f;
    if (on) { g.setColour (col.withAlpha (0.35f)); g.fillEllipse (x - 3, t.getY(), d + 6, d + 6); }
    juce::ColourGradient lg (on ? col.brighter (0.6f) : juce::Colour (0xff4a4f5a), x, t.getY() + 3, on ? col : juce::Colour (0xff15171c), x, t.getY() + 3 + d, false); g.setGradientFill (lg); g.fillEllipse (x, t.getY() + 3, d, d);
    auto lr = juce::Rectangle<float> (w + 8, 0, getWidth() - w - 8, (float) getHeight());
    if (on) glowText (g, label, lr, col, uf (11.0f)); else { g.setColour (C::dim); g.setFont (uf (11.0f)); g.drawText (label, lr, juce::Justification::centredLeft); }
}

// ======================================================================= catena
static const char* onId (int m)
{
    switch (m) { case MAXIM: return "mx_on"; case LIMIT: return "lm_on"; case BASS: return "bc_on"; case DYNEQ: return "eq_on"; case IMAGER: return "im_on";
                 case DEESS: return "ds_on"; case BUSCOMP: return "bus_on"; case MULTI: return "mb_on"; default: return nullptr; }
}
static float modGr (Processor& p, int m)
{
    if (m == MAXIM) return p.grMax.load(); if (m == LIMIT) return p.grLim.load(); if (m == BASS) return std::abs (p.bassGain.load());
    if (m == DYNEQ) { float mx = 0; for (int b = 0; b < kEqBands; ++b) mx = juce::jmax (mx, std::abs (p.eqDyn[b].load())); return mx; }
    if (m == IMAGER) return (1.0f - p.corrAll.load()) * 4.0f;
    if (m == DEESS) return p.dsGr.load(); if (m == BUSCOMP) return p.busGr.load();
    if (m == MULTI) { float mx = 0; for (int b = 0; b < kMbBands; ++b) mx = juce::jmax (mx, p.mbDown[b].load(), p.mbUp[b].load()); return mx; }
    return 0.0f;
}
void Chain::paint (juce::Graphics& g)
{
    for (int m = 0; m < kNumMods; ++m)
    {
        auto t = tile (m); const auto& mi = modInfo (m); const auto c = juce::Colour (mi.colour);
        const bool sel = ed.sel == m, ready = mi.ready, on = ready && onId (m) && ed.proc.value (onId (m)) > 0.5f;
        jlook::softShadow (g, t, 10, sel ? 16.0f : 8.0f, sel ? 0.65f : 0.4f, sel ? 6.0f : 3.0f);
        jlook::card (g, t, 10, c, on, sel, false);
        if (! ready) { g.setColour (juce::Colours::black.withAlpha (0.45f)); g.fillRoundedRectangle (t, 10); }
        if (sel) { g.setColour (c.withAlpha (0.9f)); g.drawRoundedRectangle (t.reduced (1), 9, 1.6f); }
        else if (hov == m && ready) { g.setColour (juce::Colours::white.withAlpha (0.2f)); g.drawRoundedRectangle (t.reduced (1), 9, 1.0f); }
        auto pr = powR (m);
        if (on) { g.setColour (c.withAlpha (0.35f)); g.fillEllipse (pr.expanded (4)); }
        juce::ColourGradient lg (on ? c.brighter (0.6f) : juce::Colour (0xff3a3e48), pr.getCentreX(), pr.getY(), on ? c : juce::Colour (0xff15171c), pr.getCentreX(), pr.getBottom(), false);
        g.setGradientFill (lg); g.fillEllipse (pr);
        auto tr = juce::Rectangle<float> (t.getX() + 28, t.getY() + 6, t.getWidth() - 34, 20);
        if (ready) glowText (g, mi.name, tr, on ? C::text : C::dim, uf (11.5f)); else { g.setColour (C::faint); g.setFont (uf (11.5f)); g.drawText (mi.name, tr, juce::Justification::centredLeft); }
        g.setColour (ready ? C::dim : C::faint); g.setFont (mf (8.5f));
        g.drawText (ready ? juce::String (mi.sub) : juce::String ("IN ARRIVO"), juce::Rectangle<float> (t.getX() + 10, t.getY() + 30, t.getWidth() - 20, 12), juce::Justification::centredLeft);
        // riduzione in tempo reale
        if (ready)
        {
            auto bar = juce::Rectangle<float> (t.getX() + 10, t.getBottom() - 14, t.getWidth() - 20, 5);
            g.setColour (juce::Colours::white.withAlpha (0.06f)); g.fillRoundedRectangle (bar, 2);
            const float gr = juce::jlimit (0.0f, 1.0f, modGr (ed.proc, m) / 8.0f);
            g.setColour (c); g.fillRoundedRectangle (bar.withWidth (bar.getWidth() * gr), 2);
        }
        if (m < kNumMods - 1) { g.setColour (C::dim.withAlpha (0.5f)); g.drawLine (t.getRight() + 1, t.getCentreY(), t.getRight() + 7, t.getCentreY(), 1.0f); }
    }
}
void Chain::mouseDown (const juce::MouseEvent& e)
{
    for (int m = 0; m < kNumMods; ++m)
    {
        if (! tile (m).contains (e.position)) continue;
        if (modInfo (m).ready && onId (m) && powR (m).expanded (6).contains (e.position)) { auto* p = ed.P (onId (m)); p->setValueNotifyingHost (p->getValue() > 0.5f ? 0.0f : 1.0f); }
        ed.select (m); repaint(); return;
    }
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
    auto area = [&] (std::atomic<float>* arr, juce::Colour c, float alpha)
    {
        juce::Path p; p.startNewSubPath (plot.getX(), plot.getBottom());
        for (int i = 0; i < n; ++i) p.lineTo (plot.getX() + plot.getWidth() * i / (n - 1), yDb (arr[(w0 + i) % n].load()));
        p.lineTo (plot.getRight(), plot.getBottom()); p.closeSubPath();
        juce::ColourGradient cg (c.withAlpha (alpha), 0, plot.getY(), c.withAlpha (alpha * 0.25f), 0, plot.getBottom(), false); g.setGradientFill (cg); g.fillPath (p);
    };
    area (P.hIn, C::dim, 0.35f);
    area (P.hOut, limiter ? C::red : C::magenta, 0.55f);
    // ceiling
    g.setColour (C::red.withAlpha (0.7f)); const float yc = yDb (ceil);
    for (float x = plot.getX(); x < plot.getRight(); x += 8) g.drawHorizontalLine ((int) yc, x, x + 4);
    g.drawText ("CEILING " + juce::String (ceil, 1), juce::Rectangle<float> (plot.getRight() - 120, yc - 14, 116, 12), juce::Justification::centredRight);
    // riduzione di guadagno dall'alto
    {
        std::atomic<float>* gr = limiter ? P.hGrL : P.hGrM;
        juce::Path p; p.startNewSubPath (plot.getX(), plot.getY());
        for (int i = 0; i < n; ++i) p.lineTo (plot.getX() + plot.getWidth() * i / (n - 1), yGr (gr[(w0 + i) % n].load()));
        p.lineTo (plot.getRight(), plot.getY()); p.closeSubPath();
        const auto c = limiter ? C::amber : C::phos;
        g.setColour (c.withAlpha (0.28f)); g.fillPath (p); g.setColour (c); g.strokePath (p, juce::PathStrokeType (1.2f));
    }
    // loudness short-term e target
    {
        juce::Path p; bool st = false;
        for (int i = 0; i < n; ++i) { const float v = P.hS[(w0 + i) % n].load(); if (v < -60) { st = false; continue; } const float x = plot.getX() + plot.getWidth() * i / (n - 1); if (! st) { p.startNewSubPath (x, yDb (v)); st = true; } else p.lineTo (x, yDb (v)); }
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
    delta = std::make_unique<Switch> (ed.P ("mx_delta"), C::amber, "DELTA: senti cosa toglie");
    learn = std::make_unique<Switch> (ed.P ("mx_learn"), C::green, "PORTA AL TARGET IN AUTOMATICO");
    for (juce::Component* c : std::initializer_list<juce::Component*> { gain.get(), clip.get(), chr.get(), link.get(), style.get(), on.get(), delta.get(), learn.get(), &hist }) addAndMakeVisible (c);
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
    glowText (g, juce::String ((int) tg.lufs) + " LUFS", { r.getX() + 12, r.getY() + 24, 160, 30 }, C::green, jlook::logo (19.0f));
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
    rel = std::make_unique<Knob> (ed.P ("lm_rel"), C::amber); rel->label = "RILASCIO"; rel->text = [this] { return ed.proc.value ("lm_auto") > 0.5f ? juce::String ("AUTO") : juce::String (juce::roundToInt (ed.proc.value ("lm_rel"))) + " ms"; };
    on = std::make_unique<Switch> (ed.P ("lm_on"), C::red, "ATTIVO");
    delta = std::make_unique<Switch> (ed.P ("lm_delta"), C::amber, "DELTA: senti cosa toglie");
    autoR = std::make_unique<Switch> (ed.P ("lm_auto"), C::amber, "RILASCIO AUTOMATICO");
    tp = std::make_unique<Switch> (ed.P ("lm_tp"), C::red, "TRUE PEAK (8x)");
    for (juce::Component* c : std::initializer_list<juce::Component*> { ceil.get(), rel.get(), on.get(), delta.get(), autoR.get(), tp.get(), &hist }) addAndMakeVisible (c);
}
void LimPage::resized()
{
    on->setBounds (150, 12, 100, 22); delta->setBounds (270, 12, 240, 22);
    ceil->setBounds (16, 60, 150, 150 + 32); rel->setBounds (186, 82, 100, 100 + 27);
    autoR->setBounds (306, 100, 220, 22); tp->setBounds (306, 132, 220, 22);
    hist.setBounds (16, 324, 822, 290);
}
void LimPage::paint (juce::Graphics& g)
{
    glowText (g, "LIMITER", { 16, 10, 130, 26 }, C::red, uf (17.0f));
    auto r = juce::Rectangle<float> (540, 60, 296, 200); lcd (g, r, 8);
    const float tp = ed.proc.tpMax.load(), ceil = ed.proc.value ("lm_ceil");
    g.setColour (C::dim); g.setFont (mf (9.0f)); g.drawText ("PICCO VERO MASSIMO IN USCITA", r.reduced (12, 8).removeFromTop (14), juce::Justification::centredLeft);
    glowText (g, tp < -60 ? juce::String ("-") : juce::String (tp, 2) + " dBTP", { r.getX() + 12, r.getY() + 26, 260, 34 }, tp > ceil + 0.05f ? C::red : C::green, jlook::logo (22.0f));
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
        out[k] = out[k] * smooth + juce::jmax (-110.0f, db) * (1 - smooth);
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
    const auto c = juce::Colour (modInfo (id).colour);
    on = std::make_unique<Switch> (ed.P (onP), c, "ATTIVO"); delta = std::make_unique<Switch> (ed.P (deltaP), C::amber, "DELTA: senti cosa cambia");
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
    const auto c = juce::Colour (modInfo (mod).colour);
    glowText (g, modInfo (mod).name, { 16, 10, 200, 26 }, c, uf (17.0f));
    if (ed.proc.value (onId) < 0.5f) glowText (g, "SPENTO: tocca un comando per accenderlo", { 520, 12, 320, 22 }, C::dim, uf (10.5f), juce::Justification::centredRight);
    g.setColour (C::dim); g.setFont (uf (10.0f, false));
    g.drawFittedText (help, juce::Rectangle<int> (16, getHeight() - 52, getWidth() - 32, 40), juce::Justification::topLeft, 3);
}

// ======================================================================= BASS CONTROL
BassPage::BassPage (Editor& e) : ModPage (e, BASS, "bc_on", "bc_delta"), view (e)
{
    auto& P = ed.proc; const auto a = C::amber;
    knob ("bc_freq", a, "BASSI SOTTO", [&P] { return juce::String (juce::roundToInt (P.value ("bc_freq"))) + " Hz"; });
    knob ("bc_mono", C::phos, "MONO SOTTO", [&P] { const float v = P.value ("bc_mono"); return v < 20 ? juce::String ("OFF") : juce::String (juce::roundToInt (v)) + " Hz"; });
    knob ("bc_punch", C::magenta, "PUNCH", [&P] { const float v = P.value ("bc_punch"); return std::abs (v) < 0.02f ? juce::String ("0") : (v > 0 ? "+" : "") + juce::String (juce::roundToInt (v * 100)); }, true);
    knob ("bc_tail", C::violet, "CODA", [&P] { const float v = P.value ("bc_tail"); return std::abs (v) < 0.02f ? juce::String ("0") : v < 0 ? "STRINGI " + juce::String (juce::roundToInt (-v * 100)) : "ALLUNGA " + juce::String (juce::roundToInt (v * 100)); }, true);
    knob ("bc_ctrl", C::green, "STABILIZZA", [&P] { return juce::String (juce::roundToInt (P.value ("bc_ctrl") * 100)) + "%"; });
    knob ("bc_harm", a, "ARMONICHE", [&P] { return juce::String (juce::roundToInt (P.value ("bc_harm") * 100)) + "%"; });
    knob ("bc_level", a, "LIVELLO BASSI", [&P] { return dbS (P.value ("bc_level")) + " dB"; }, true);
    addAndMakeVisible (view);
}
void BassPage::resized()
{
    on->setBounds (190, 12, 100, 22); delta->setBounds (306, 12, 230, 22);
    view.setBounds (16, 48, 822, 330);
    for (int i = 0; i < knobs.size(); ++i) knobs[i]->setBounds (16 + i * 117, 396, 108, 108 + 27);
}
void BassPage::paint (juce::Graphics& g)
{
    paintHeader (g, "BASSI SOTTO sceglie dove finisce la zona dei bassi. PUNCH da colpo alla cassa, CODA stringe o allunga l'808, STABILIZZA tiene fermo il livello dei bassi, "
                    "ARMONICHE fa sentire il sub sui telefoni, MONO SOTTO mette in mono la parte piu bassa (consigliato 80-150 Hz).");
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
    auto hr = juce::Rectangle<float> (b.getRight() - 112, b.getY() + 10, 98, b.getHeight() - 36);
    g.setColour (juce::Colours::black.withAlpha (0.3f)); g.fillRoundedRectangle (hr, 4);
    g.setColour (C::dim.withAlpha (0.5f)); g.drawHorizontalLine ((int) hr.getCentreY(), hr.getX(), hr.getRight());
    juce::Path p; for (int i = 0; i < 160; ++i) { const float v = hist[(hw + i) % 160]; const float xx = hr.getX() + hr.getWidth() * i / 159.0f, yy = hr.getCentreY() - juce::jlimit (-1.0f, 1.0f, v / 9.0f) * hr.getHeight() * 0.48f; if (i == 0) p.startNewSubPath (xx, yy); else p.lineTo (xx, yy); }
    g.setColour (C::magenta); g.strokePath (p, juce::PathStrokeType (1.3f));
    g.setColour (C::dim); g.setFont (mf (8.5f)); g.drawText ("GUADAGNO BASSI", juce::Rectangle<float> (hr.getX(), hr.getBottom() + 2, hr.getWidth(), 12), juce::Justification::centred);
    glowText (g, dbS (P.bassGain.load()) + " dB", { hr.getX(), hr.getY() + 4, hr.getWidth(), 14 }, C::magenta, mf (10.0f), juce::Justification::centred);
    scanlines (g, b);
}

// ======================================================================= EQ DINAMICO
static juce::Colour bandCol (int b) { static const juce::uint32 c[kEqBands] = { 0xffffb000, 0xffff7a3d, 0xffff2e88, 0xffa66bff, 0xff5ef2ff, 0xff7dff5a }; return juce::Colour (c[b]); }
EqPage::EqPage (Editor& e) : ModPage (e, DYNEQ, "eq_on", "eq_delta"), view (e)
{
    auto& P = ed.proc;
    for (int b = 0; b < kEqBands; ++b)
    {
        const auto c = bandCol (b);
        type[b] = std::make_unique<Segs> (ed.P (eqId (b, "type")), c, juce::StringArray { "CAMPANA", "SHELF BASSI", "SHELF ALTI", "TAGLIO BASSI", "TAGLIO ALTI" }); type[b]->font = 9.5f;
        ch[b] = std::make_unique<Segs> (ed.P (eqId (b, "ch")), c, juce::StringArray { "STEREO", "MID", "SIDE" }); ch[b]->font = 10.0f;
        bon[b] = std::make_unique<Switch> (ed.P (eqId (b, "on")), c, "BANDA ATTIVA");
        dyn[b] = std::make_unique<Switch> (ed.P (eqId (b, "dyn")), C::green, "DINAMICA");
        const juce::String fid = eqId (b, "f"), gid = eqId (b, "g"), qid = eqId (b, "q"), tid = eqId (b, "thr"), rid = eqId (b, "rng");
        fk[b] = knob (fid.toRawUTF8(), c, "FREQUENZA", [&P, fid] { return hzT (P.value (fid.toRawUTF8())); });
        gk[b] = knob (gid.toRawUTF8(), c, "GUADAGNO", [&P, gid] { return dbS (P.value (gid.toRawUTF8())) + " dB"; }, true);
        qk[b] = knob (qid.toRawUTF8(), c, "Q", [&P, qid] { return juce::String (P.value (qid.toRawUTF8()), 2); });
        tk[b] = knob (tid.toRawUTF8(), C::green, "SOGLIA", [&P, tid] { return juce::String (P.value (tid.toRawUTF8()), 1) + " dB"; });
        rk[b] = knob (rid.toRawUTF8(), C::green, "RANGE", [&P, rid] { return dbS (P.value (rid.toRawUTF8())) + " dB"; }, true);
        for (juce::Component* x : std::initializer_list<juce::Component*> { type[b].get(), ch[b].get(), bon[b].get(), dyn[b].get() }) addChildComponent (x);
    }
    addAndMakeVisible (view);
    selectBand (0);
}
void EqPage::selectBand (int b)
{
    band = b;
    for (int k = 0; k < kEqBands; ++k)
        for (juce::Component* x : std::initializer_list<juce::Component*> { type[k].get(), ch[k].get(), bon[k].get(), dyn[k].get(), fk[k], gk[k], qk[k], tk[k], rk[k] }) x->setVisible (k == b);
    repaint();
}
void EqPage::resized()
{
    on->setBounds (190, 12, 100, 22); delta->setBounds (306, 12, 230, 22);
    view.setBounds (16, 48, 822, 298);
    for (int b = 0; b < kEqBands; ++b)
    {
        type[b]->setBounds (16, 392, 430, 26); ch[b]->setBounds (456, 392, 220, 26); bon[b]->setBounds (690, 394, 150, 22);
        fk[b]->setBounds (16, 428, 92, 92 + 27); gk[b]->setBounds (116, 428, 92, 92 + 27); qk[b]->setBounds (216, 428, 92, 92 + 27);
        dyn[b]->setBounds (330, 430, 120, 22); tk[b]->setBounds (330, 458, 76, 76 + 27); rk[b]->setBounds (414, 458, 76, 76 + 27);
    }
}
void EqPage::mouseDown (const juce::MouseEvent& e) { for (int b = 0; b < kEqBands; ++b) if (chip (b).contains (e.position)) { selectBand (b); return; } }
void EqPage::paint (juce::Graphics& g)
{
    paintHeader (g, "Trascina i punti: sinistra/destra frequenza, su/giu guadagno, rotella = Q, doppio clic accende o spegne la banda. "
                    "DINAMICA: la banda interviene solo quando il segnale supera la SOGLIA, fino a RANGE (negativo = taglia, positivo = alza). MID/SIDE: lavora solo sul centro o sui lati.");
    auto& P = ed.proc;
    for (int b = 0; b < kEqBands; ++b)
    {
        const bool onB = P.value (eqId (b, "on")) > 0.5f, sel = b == band;
        const auto c = bandCol (b);
        jlook::button (g, chip (b), c, sel, false, "", uf (10.0f));
        g.setColour (onB ? c : C::faint); g.fillEllipse (chip (b).getX() + 10, chip (b).getCentreY() - 4, 8, 8);
        juce::String t = juce::String (b + 1) + "  " + hzT (P.value (eqId (b, "f")));
        const float gg = P.value (eqId (b, "g")); if (std::abs (gg) > 0.05f) t << "  " << dbS (gg);
        if (P.value (eqId (b, "dyn")) > 0.5f) t << "  D";
        glowText (g, t, chip (b).withTrimmedLeft (24), sel ? C::text : C::dim, uf (10.5f));
    }
    // misuratore della banda selezionata rispetto alla soglia
    auto m = juce::Rectangle<float> (520, 428, 318, 132); lcd (g, m, 8);
    const bool dy = P.value (eqId (band, "dyn")) > 0.5f;
    g.setColour (C::dim); g.setFont (mf (9.0f)); g.drawText ("BANDA " + juce::String (band + 1) + (dy ? "  livello e soglia" : "  statica"), m.reduced (10, 6).removeFromTop (12), juce::Justification::centredLeft);
    if (dy)
    {
        auto bar = juce::Rectangle<float> (m.getX() + 10, m.getY() + 30, m.getWidth() - 20, 14);
        const float lv = P.eqLvl[band].load(), th = P.value (eqId (band, "thr"));
        auto xx = [&] (float db) { return bar.getX() + juce::jlimit (0.0f, 1.0f, (db + 60) / 60) * bar.getWidth(); };
        g.setColour (juce::Colours::white.withAlpha (0.06f)); g.fillRoundedRectangle (bar, 3);
        g.setColour (lv > th ? C::green : C::phos.withAlpha (0.6f)); g.fillRoundedRectangle (bar.withRight (xx (lv)), 3);
        g.setColour (C::amber); g.fillRect (xx (th) - 1, bar.getY() - 4, 2.0f, bar.getHeight() + 8);
        glowText (g, "ADESSO " + dbS (P.eqDyn[band].load()) + " dB", { m.getX() + 10, m.getY() + 54, m.getWidth() - 20, 20 }, C::green, mf (13.0f));
        g.setColour (C::dim); g.setFont (uf (10.0f, false)); g.drawFittedText ("es. sibilanti: CAMPANA 7 kHz, RANGE -6. Fango: 250 Hz, RANGE -4. Cassa piu viva: 60 Hz, RANGE +3.", m.reduced (10, 0).withTrimmedTop (76).toNearestInt(), juce::Justification::topLeft, 2);
    }
    else { g.setColour (C::dim); g.setFont (uf (10.5f, false)); g.drawFittedText ("Accendi DINAMICA per farla reagire al segnale: utile su sibilanti, fango, risonanze, cassa e 808 che cambiano.", m.reduced (10, 0).withTrimmedTop (24).toNearestInt(), juce::Justification::topLeft, 4); }
}
juce::Point<float> EqView::node (int b) const
{
    auto& P = ed.proc; const int t = (int) P.value (eqId (b, "type"));
    const float g = (t == Biquad::LCUT || t == Biquad::HCUT) ? 0.0f : P.value (eqId (b, "g"));
    return { xOf (P.value (eqId (b, "f"))), yOf (g) };
}
void EqView::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat(); lcd (g, b, 8);
    auto& P = ed.proc; const double sr = P.sampleRate();
    an.run (P.ringOut, sp, 240, 20, 20000, sr);
    g.setFont (mf (8.5f));
    for (float f : { 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f })
    { g.setColour (C::phos.withAlpha (0.07f)); g.drawVerticalLine ((int) xOf (f), 4, b.getBottom() - 4); g.setColour (C::dim.withAlpha (0.7f)); g.drawText (f >= 1000 ? juce::String ((int) (f / 1000)) + "k" : juce::String ((int) f), juce::Rectangle<float> (xOf (f) - 16, b.getBottom() - 14, 32, 11), juce::Justification::centred); }
    for (float db : { -12.0f, -6.0f, 6.0f, 12.0f }) { g.setColour (C::phos.withAlpha (0.06f)); g.drawHorizontalLine ((int) yOf (db), 4, b.getRight() - 4); g.setColour (C::dim.withAlpha (0.6f)); g.drawText (dbS (db, 0), juce::Rectangle<float> (4, yOf (db) - 6, 28, 12), juce::Justification::centredLeft); }
    g.setColour (C::phos.withAlpha (0.15f)); g.drawHorizontalLine ((int) yOf (0), 4, b.getRight() - 4);
    drawSpec (g, sp, 240, b.reduced (2, 6), C::dim.withAlpha (0.35f), true, -100, 0);
    // curve: statica e reale (con la parte dinamica)
    Biquad f[kEqBands], fd[kEqBands]; bool act[kEqBands] {};
    for (int k = 0; k < kEqBands; ++k)
    {
        act[k] = P.value (eqId (k, "on")) > 0.5f; if (! act[k]) continue;
        const int t = (int) P.value (eqId (k, "type")); const bool cut = t == Biquad::LCUT || t == Biquad::HCUT;
        const float q = P.value (eqId (k, "q")), fr = P.value (eqId (k, "f")), gg = P.value (eqId (k, "g"));
        f[k].set (t, fr, gg, cut ? 0.707 * q : q, sr); fd[k].set (t, fr, gg + (P.value ("eq_on") > 0.5f && P.value (eqId (k, "dyn")) > 0.5f && ! cut ? P.eqDyn[k].load() : 0.0f), cut ? 0.707 * q : q, sr);
    }
    auto curve = [&] (Biquad* fl, juce::Colour c, float th, bool fill)
    {
        juce::Path p; const int n = (int) b.getWidth() / 2;
        for (int i = 0; i <= n; ++i) { const float x = b.getWidth() * i / n, fr = fOf (x); double db = 0; for (int k = 0; k < kEqBands; ++k) if (act[k]) db += fl[k].magDb (fr, sr); const float y = juce::jlimit (2.0f, b.getHeight() - 2, yOf ((float) db)); if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y); }
        if (fill) { juce::Path pf = p; pf.lineTo (b.getRight(), yOf (0)); pf.lineTo (0, yOf (0)); pf.closeSubPath(); g.setColour (c.withAlpha (0.10f)); g.fillPath (pf); }
        g.setColour (c.withAlpha (0.25f)); g.strokePath (p, juce::PathStrokeType (th * 3)); g.setColour (c); g.strokePath (p, juce::PathStrokeType (th));
    };
    const bool modOn = P.value ("eq_on") > 0.5f;
    curve (f, modOn ? C::phos.withAlpha (0.5f) : C::faint, 1.0f, false);
    curve (fd, modOn ? C::phos : C::faint, 1.8f, true);
    // nodi
    auto* page = dynamic_cast<EqPage*> (getParentComponent());
    for (int k = 0; k < kEqBands; ++k)
    {
        const auto pt = node (k); const auto c = bandCol (k); const bool sel = page && page->band == k;
        const bool onB = act[k];
        if (onB && P.value (eqId (k, "dyn")) > 0.5f && modOn)
        {   // freccia del guadagno dinamico
            const float yd = yOf (P.value (eqId (k, "g")) + P.eqDyn[k].load());
            g.setColour (C::green.withAlpha (0.8f)); g.drawLine (pt.x, pt.y, pt.x, yd, 2.0f); g.fillEllipse (pt.x - 3, yd - 3, 6, 6);
        }
        if (sel) { g.setColour (c.withAlpha (0.3f)); g.fillEllipse (pt.x - 13, pt.y - 13, 26, 26); }
        g.setColour (juce::Colours::black.withAlpha (0.7f)); g.fillEllipse (pt.x - 9, pt.y - 9, 18, 18);
        g.setColour (onB ? c : C::faint); g.drawEllipse (pt.x - 8, pt.y - 8, 16, 16, sel ? 2.0f : 1.4f);
        g.setFont (uf (10.0f)); g.drawText (juce::String (k + 1), juce::Rectangle<float> (pt.x - 8, pt.y - 8, 16, 16), juce::Justification::centred);
    }
    scanlines (g, b);
}
void EqView::mouseDown (const juce::MouseEvent& e)
{
    drag = -1; float best = 20;
    for (int k = 0; k < kEqBands; ++k) { const float d = node (k).getDistanceFrom (e.position); if (d < best) { best = d; drag = k; } }
    if (auto* page = dynamic_cast<EqPage*> (getParentComponent())) { if (drag >= 0) page->selectBand (drag); page->touch(); }
    if (drag >= 0) { ed.P (eqId (drag, "f"))->beginChangeGesture(); ed.P (eqId (drag, "g"))->beginChangeGesture(); }
}
void EqView::mouseDrag (const juce::MouseEvent& e)
{
    if (drag < 0) return;
    auto* pf = ed.P (eqId (drag, "f")); auto* pg = ed.P (eqId (drag, "g"));
    pf->setValueNotifyingHost (pf->convertTo0to1 (fOf (e.position.x)));
    const int t = (int) ed.proc.value (eqId (drag, "type"));
    if (t != Biquad::LCUT && t != Biquad::HCUT) pg->setValueNotifyingHost (pg->convertTo0to1 (juce::jlimit (-18.0f, 18.0f, dbOf (e.position.y))));
    repaint();
}
void EqView::mouseUp (const juce::MouseEvent&) { if (drag >= 0) { ed.P (eqId (drag, "f"))->endChangeGesture(); ed.P (eqId (drag, "g"))->endChangeGesture(); } drag = -1; }
void EqView::mouseDoubleClick (const juce::MouseEvent& e)
{
    for (int k = 0; k < kEqBands; ++k) if (node (k).getDistanceFrom (e.position) < 14) { auto* p = ed.P (eqId (k, "on")); p->setValueNotifyingHost (p->getValue() > 0.5f ? 0.0f : 1.0f); return; }
}
void EqView::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    auto* page = dynamic_cast<EqPage*> (getParentComponent()); const int k = page ? page->band : 0;
    juce::ignoreUnused (e);
    auto* q = ed.P (eqId (k, "q")); q->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, q->getValue() + (w.deltaY > 0 ? 0.03f : -0.03f)));
}

// ======================================================================= IMAGER
static juce::Colour imCol (int b) { static const juce::uint32 c[kImBands] = { 0xffffb000, 0xffff2e88, 0xff3dffd2, 0xff5ef2ff }; return juce::Colour (c[b]); }
ImagerPage::ImagerPage (Editor& e) : ModPage (e, IMAGER, "im_on", "im_delta"), view (e), gonio (e)
{
    auto& P = ed.proc;
    static const char* names[kImBands] = { "LARG. BASSI", "LARG. MEDIO-BASSI", "LARG. MEDI", "LARG. ALTI" };
    for (int b = 0; b < kImBands; ++b) { const juce::String id = "im_w" + juce::String (b + 1); knob (id.toRawUTF8(), imCol (b), names[b], [&P, id] { return juce::String (juce::roundToInt (P.value (id.toRawUTF8()) * 100)) + "%"; }); }
    knob ("im_stz", C::violet, "STEREOIZE", [&P] { return juce::String (juce::roundToInt (P.value ("im_stz") * 100)) + "%"; });
    mono = std::make_unique<Switch> (ed.P ("im_mono"), C::amber, "ASCOLTA IN MONO");
    addAndMakeVisible (*mono); addAndMakeVisible (view); addAndMakeVisible (gonio);
}
void ImagerPage::resized()
{
    on->setBounds (130, 12, 100, 22); delta->setBounds (246, 12, 230, 22);
    view.setBounds (16, 48, 560, 330); gonio.setBounds (588, 48, 250, 330);
    for (int i = 0; i < knobs.size(); ++i) knobs[i]->setBounds (16 + i * 112, 396, 104, 104 + 27);
    mono->setBounds (590, 430, 240, 22);
}
void ImagerPage::paint (juce::Graphics& g)
{
    paintHeader (g, "Trascina le barre per allargare o stringere ogni banda (100% = com'e), le linee verticali spostano i tagli. Tieni i bassi stretti (0-60%), allarga medi e alti con gusto. "
                    "STEREOIZE crea larghezza anche da materiale mono e sparisce in mono. Controlla sempre con ASCOLTA IN MONO.");
    g.setColour (C::dim); g.setFont (uf (10.0f, false));
    g.drawFittedText ("Correlazione: +1 mono, 0 molto largo, sotto 0 rischio di sparire in mono.", juce::Rectangle<int> (590, 460, 248, 40), juce::Justification::topLeft, 3);
}
float ImView::xb (int k) const { return xOf (ed.proc.value (k == 0 ? "im_x1" : k == 1 ? "im_x2" : "im_x3")); }
void ImView::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat(); lcd (g, b, 8);
    auto& P = ed.proc; const bool on = P.value ("im_on") > 0.5f;
    const float top = 30, bot = b.getHeight() - 40, mid = top + (bot - top) * 0.5f;
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
        g.setColour (juce::Colours::white.withAlpha (0.06f)); g.fillRoundedRectangle (cb, 3);
        const float cx = cb.getX() + (cr + 1) * 0.5f * cb.getWidth();
        g.setColour (cr < 0 ? C::red : cr < 0.3f ? C::amber : C::green); g.fillEllipse (cx - 4, cb.getY(), 8, 8);
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
    float l[2048], r[2048]; P.ringOut.read (l, r, 2048);
    float mx = 0.05f; for (int i = 0; i < 2048; ++i) mx = juce::jmax (mx, std::abs (l[i]), std::abs (r[i]));
    const float sc = R * 0.9f / mx;
    for (int i = 0; i < 2048; i += 2)
    {
        const float sx = (l[i] - r[i]) * 0.7071f, sy = (l[i] + r[i]) * 0.7071f;
        const float px = cx + sx * sc, py = cy - sy * sc;
        g.setColour (C::green.withAlpha (0.35f)); g.fillRect (px, py, 1.6f, 1.6f);
    }
    // correlazione
    auto cb = juce::Rectangle<float> (b.getX() + 14, sq.getBottom() + 22, b.getWidth() - 28, 10);
    g.setColour (C::dim); g.drawText ("-1", juce::Rectangle<float> (cb.getX(), cb.getY() - 14, 20, 12), juce::Justification::centredLeft);
    g.drawText ("+1", juce::Rectangle<float> (cb.getRight() - 20, cb.getY() - 14, 20, 12), juce::Justification::centredRight);
    g.drawText ("CORRELAZIONE", juce::Rectangle<float> (cb.getX(), cb.getY() - 14, cb.getWidth(), 12), juce::Justification::centred);
    g.setColour (juce::Colours::white.withAlpha (0.06f)); g.fillRoundedRectangle (cb, 3);
    const float cr = juce::jlimit (-1.0f, 1.0f, P.corrAll.load()), x = cb.getX() + (cr + 1) * 0.5f * cb.getWidth();
    g.setColour (cr < 0 ? C::red : cr < 0.3f ? C::amber : C::green); g.fillRoundedRectangle (juce::Rectangle<float> (cb.getCentreX(), cb.getY(), x - cb.getCentreX(), cb.getHeight()).withLeft (juce::jmin (x, cb.getCentreX())).withRight (juce::jmax (x, cb.getCentreX())), 3);
    glowText (g, juce::String (cr, 2), { cb.getX(), cb.getBottom() + 2, cb.getWidth(), 14 }, cr < 0 ? C::red : C::text, mf (10.5f), juce::Justification::centred);
    scanlines (g, b);
}

// ======================================================================= DE-ESSER
DeEssPage::DeEssPage (Editor& e) : ModPage (e, DEESS, "ds_on", "ds_delta"), view (e)
{
    auto& P = ed.proc; const auto c = C::green;
    knob ("ds_freq", c, "FREQUENZA", [&P] { return hzT (P.value ("ds_freq")); });
    knob ("ds_thr", c, "SOGLIA", [&P] { return juce::String (P.value ("ds_thr"), 1) + " dB"; });
    knob ("ds_range", C::red, "RIDUZIONE MAX", [&P] { return juce::String (P.value ("ds_range"), 1) + " dB"; });
    ch = std::make_unique<Segs> (ed.P ("ds_ch"), c, juce::StringArray { "STEREO", "MID (voce)", "SIDE (piatti)" });
    addAndMakeVisible (*ch); addAndMakeVisible (view);
}
void DeEssPage::resized()
{
    on->setBounds (160, 12, 100, 22); delta->setBounds (276, 12, 230, 22);
    view.setBounds (16, 48, 822, 300);
    for (int i = 0; i < knobs.size(); ++i) knobs[i]->setBounds (16 + i * 120, 372, 110, 110 + 27);
    ch->setBounds (400, 400, 420, 28);
}
void DeEssPage::paint (juce::Graphics& g)
{
    paintHeader (g, "De-esser da master: confronta le sibilanti con il resto del mix e abbassa solo la parte sopra FREQUENZA quando sporgono oltre la SOGLIA. "
                    "Usa DELTA per sentire cosa toglie: devono essere solo S, Z e piatti duri. MID per la voce al centro, SIDE per piatti larghi.");
    g.setColour (C::dim); g.setFont (mf (9.0f)); g.drawText ("DOVE LAVORA", juce::Rectangle<float> (400, 384, 200, 12), juce::Justification::centredLeft);
}
void DsView::mouseDrag (const juce::MouseEvent& e)
{
    auto r = getLocalBounds().toFloat().reduced (14, 10).withTrimmedRight (110);
    const float f = 1000.0f * std::pow (20.0f, juce::jlimit (0.0f, 1.0f, (e.position.x - r.getX()) / r.getWidth()));
    auto* p = ed.P ("ds_freq"); p->setValueNotifyingHost (p->convertTo0to1 (f));
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
    const float fx = x (P.value ("ds_freq"));
    g.setColour (C::green.withAlpha (0.08f)); g.fillRect (r.withLeft (fx));
    drawSpec (g, sp, 200, r, C::green, true, -110, 10);
    g.setColour (C::green); g.fillRect (fx - 1, r.getY(), 2.0f, r.getHeight());
    glowText (g, hzT (P.value ("ds_freq")) + juce::String::fromUTF8 (" \xe2\x86\x92"), { fx + 6, r.getY() + 4, 120, 14 }, C::green, mf (10.0f));
    // quanto sporgono le sibilanti rispetto alla soglia
    auto bar = juce::Rectangle<float> (r.getX(), b.getBottom() - 28, r.getWidth(), 12);
    const float rel = P.dsRel.load(), thr = P.value ("ds_thr");
    auto bx = [&] (float db) { return bar.getX() + juce::jlimit (0.0f, 1.0f, (db + 40) / 46) * bar.getWidth(); };
    g.setColour (juce::Colours::white.withAlpha (0.06f)); g.fillRoundedRectangle (bar, 3);
    g.setColour (rel > thr ? C::red : C::green.withAlpha (0.7f)); g.fillRoundedRectangle (bar.withRight (bx (rel)), 3);
    g.setColour (C::amber); g.fillRect (bx (thr) - 1, bar.getY() - 4, 2.0f, bar.getHeight() + 8);
    g.setColour (C::dim); g.setFont (mf (8.5f)); g.drawText ("SIBILANTI RISPETTO AL MIX", juce::Rectangle<float> (bar.getX(), bar.getY() - 14, 200, 12), juce::Justification::centredLeft);
    // riduzione nel tempo
    hist[hw] = P.dsGr.load(); hw = (hw + 1) % 160;
    auto hr = juce::Rectangle<float> (b.getRight() - 112, b.getY() + 10, 98, b.getHeight() - 36);
    g.setColour (juce::Colours::black.withAlpha (0.3f)); g.fillRoundedRectangle (hr, 4);
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
    for (int i = 1; i < knobs.size(); ++i) knobs[i]->setBounds (176 + (i - 1) * 116, 384, 104, 104 + 27);
}
void BusPage::paint (juce::Graphics& g)
{
    paintHeader (g, "Compressore da bus in stile console: incolla il mix. Per il master: RATIO 2, ATTACCO 10 o 30 ms (lascia passare i colpi), RILASCIO AUTO, 1-3 dB di riduzione. "
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
        g.setColour (juce::Colours::black.withAlpha (0.25f)); g.drawLine (cx + 3, cy, cx + 3 + std::sin (a) * R * 0.95f, cy - std::cos (a) * R * 0.95f, 2.0f);
        g.setColour (juce::Colour (0xff111111)); g.drawLine (cx, cy, cx + std::sin (a) * R * 0.95f, cy - std::cos (a) * R * 0.95f, 1.8f);
    }
    jlook::glass (g, vu, 8);
    // storia
    hist[hw] = gr; hw = (hw + 1) % 200;
    auto hr = juce::Rectangle<float> (b.getX() + 20, vu.getBottom() + 14, b.getWidth() - 40, b.getBottom() - vu.getBottom() - 30);
    g.setColour (juce::Colours::black.withAlpha (0.3f)); g.fillRoundedRectangle (hr, 4);
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
        g.setColour (juce::Colours::white.withAlpha (0.05f)); g.fillRoundedRectangle ((float) x0 - 4, 290, 268, 136, 8);
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
        g.setColour (juce::Colours::white.withAlpha (0.05f)); g.fillRect (gb);
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

void SoonPage::paint (juce::Graphics& g)
{
    const auto& mi = modInfo (mod); const auto c = juce::Colour (mi.colour);
    glowText (g, mi.name, { 16, 10, 400, 26 }, c, uf (17.0f));
    auto r = getLocalBounds().toFloat().reduced (16).withTrimmedTop (40); lcd (g, r, 10);
    glowText (g, "IN ARRIVO", r.withHeight (r.getHeight() * 0.5f).withY (r.getY() + r.getHeight() * 0.2f), c, jlook::logo (30.0f), juce::Justification::centred);
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
    jlook::button (g, resetR(), C::phos, false, false, "AZZERA", uf (10.5f));
    const float I = P.lufsI.load(), S = P.lufsS.load(), M = P.lufsM.load(), tp = P.tpMax.load(), lra = P.lra.load();
    auto big = juce::Rectangle<float> (16, 40, (float) getWidth() - 32, 92); lcd (g, big, 10);
    g.setColour (C::dim); g.setFont (mf (9.0f)); g.drawText ("INTEGRATO", big.reduced (14, 8).removeFromTop (12), juce::Justification::centredLeft);
    const float dv = I - tg.lufs; const bool okT = I > -60 && std::abs (dv) < 1.0f;
    glowText (g, lufsS (I), { big.getX() + 14, big.getY() + 20, 180, 50 }, I < -60 ? C::faint : okT ? C::green : (dv > 0 ? C::amber : C::phos), jlook::logo (34.0f));
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
        jlook::button (g, r, C::green, on, false, juce::String (target (t).name) + " " + juce::String ((int) target (t).lufs), uf (9.5f));
    }
    // storia breve
    auto h = juce::Rectangle<float> (16, 330, (float) getWidth() - 32, 62); lcd (g, h, 6);
    const int n = Processor::kHist, w0 = P.hW.load();
    auto y = [&] (float v) { return h.getBottom() - 4 - juce::jlimit (0.0f, 1.0f, (v + 30) / 26) * (h.getHeight() - 8); };
    g.setColour (C::green.withAlpha (0.6f)); const float yt = y (tg.lufs); for (float x = h.getX() + 4; x < h.getRight() - 4; x += 6) g.drawHorizontalLine ((int) yt, x, x + 2);
    juce::Path p; bool st = false;
    for (int i = 0; i < n; ++i) { const float v = P.hS[(w0 + i) % n].load(); if (v < -60) { st = false; continue; } const float x = h.getX() + 4 + (h.getWidth() - 8) * i / (n - 1); if (! st) { p.startNewSubPath (x, y (v)); st = true; } else p.lineTo (x, y (v)); }
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
    compare = std::make_unique<Switch> (P ("compare"), C::phos, "CONFRONTA A VOLUME UGUALE");
    norm = std::make_unique<Switch> (P ("norm"), C::green, "ASCOLTA COME LA PIATTAFORMA");
    dither = std::make_unique<Segs> (P ("dither"), C::violet, juce::StringArray { "OFF", "24 BIT", "16 BIT", "16 SHAPED" }); dither->font = 10.0f;
    for (juce::Component* c : std::initializer_list<juce::Component*> { &chain, &maxPage, &limPage, &bassPage, &eqPage, &imPage, &dsPage, &busPage, &mbPage, &soon, &loud, trim.get(), compare.get(), norm.get(), dither.get() }) addAndMakeVisible (c);
    addAndMakeVisible (sizeChip);
    select (MAXIM);
    scaler.setup();
    startTimerHz (30);
}
void Editor::select (int m)
{
    sel = m;
    maxPage.setVisible (m == MAXIM); limPage.setVisible (m == LIMIT); bassPage.setVisible (m == BASS); eqPage.setVisible (m == DYNEQ); imPage.setVisible (m == IMAGER);
    dsPage.setVisible (m == DEESS); busPage.setVisible (m == BUSCOMP); mbPage.setVisible (m == MULTI);
    soon.setVisible (! modInfo (m).ready); soon.mod = m;
    chain.repaint(); repaint();
}
void Editor::resized()
{
    const float sc = scaler.update();
    presetRect = { 450, 15, 330, 36 }; prevRect = { 412, 15, 36, 36 }; nextRect = { 782, 15, 36, 36 }; abRect = { 830, 15, 110, 36 };
    chain.setBounds (18, 76, 1244, 66);
    for (auto* pg : std::initializer_list<juce::Component*> { &maxPage, &limPage, &bassPage, &eqPage, &imPage, &dsPage, &busPage, &mbPage, &soon }) pg->setBounds (18, 154, 854, 628);
    loud.setBounds (884, 154, 378, 406);
    trim->setBounds (898, 606, 84, 84 + 27);
    compare->setBounds (996, 604, 260, 22); norm->setBounds (996, 632, 260, 22);
    dither->setBounds (996, 700, 252, 26);
    sizeChip.setBounds (W - 76, H - 18, 58, 15);
    jlook::scaleChildren (*this, sc);
}

void Editor::paint (juce::Graphics& g)
{
    g.addTransform (juce::AffineTransform::scale (scaler.s));
    const float Wf = (float) W, Hf = (float) H;
    jlook::backdrop (g, { 0, 0, Wf, Hf }, C::magenta, C::red);
    jlook::header (g, { 0, 0, Wf, 64 }, "MASTER", C::red, "JM-1  CATENA DI MASTERING", C::magenta, C::red);
    lcd (g, presetRect.toFloat(), 8); jlook::glass (g, presetRect.toFloat(), 8);
    glowText (g, proc.getProgramName (proc.preset).toUpperCase(), presetRect.toFloat(), C::amber, mf (12.5f), juce::Justification::centred);
    g.setColour (C::phos); g.setFont (uf (20.0f));
    g.drawText (juce::String::fromUTF8 ("\xe2\x80\xb9"), prevRect, juce::Justification::centred); g.drawText (juce::String::fromUTF8 ("\xe2\x80\xba"), nextRect, juce::Justification::centred);
    jlook::button (g, abRect.toFloat(), C::phos, proc.ab.cur == 1, abHover, "CONFRONTO " + proc.ab.label(), uf (11.5f));
    if (proc.value ("compare") > 0.5f) jlook::button (g, { Wf - 300, 18, 280, 30 }, C::phos, true, false, "ASCOLTI L'ORIGINALE A VOLUME UGUALE", uf (11.0f));
    else if (proc.value ("norm") > 0.5f) jlook::button (g, { Wf - 300, 18, 280, 30 }, C::green, true, false, "ASCOLTO NORMALIZZATO: SPEGNI PRIMA DI ESPORTARE", uf (10.5f));
    else
    {
        g.setColour (C::dim); g.setFont (mf (9.0f));
        g.drawText ("latenza " + juce::String (proc.getLatencySamples() * 1000.0 / juce::jmax (1.0, proc.sampleRate()), 1) + " ms (compensata)", juce::Rectangle<float> (Wf - 250, 20, 230, 14), juce::Justification::centredRight);
        for (int i = 0; i < 9; ++i) { auto r = juce::Rectangle<float> (Wf - 120 + i * 11.0f, 38, 5, 14); g.setColour (juce::Colours::black.withAlpha (0.8f)); g.fillRoundedRectangle (r, 2.5f); }
    }
    const auto mc = juce::Colour (modInfo (sel).colour);
    auto card = [&] (juce::Rectangle<float> r, juce::Colour c) { jlook::softShadow (g, r, 14); jlook::card (g, r, 14, c, true, false, false); };
    card ({ 18, 154, 854, 628 }, mc);
    card ({ 884, 154, 378, 406 }, C::green);
    card ({ 884, 572, 378, 210 }, C::phos);
    glowText (g, "USCITA", { 900, 578, 200, 24 }, C::text, uf (13.0f));
    g.setColour (C::dim); g.setFont (mf (9.0f)); g.drawText ("DITHER (solo per l'export finale)", juce::Rectangle<float> (996, 684, 260, 12), juce::Justification::centredLeft);
    g.setColour (C::dim); g.setFont (uf (10.0f, false));
    g.drawFittedText ("Il confronto alza l'originale allo stesso volume: senti cosa cambia davvero, non solo il piu forte. 16 SHAPED solo se esporti a 16 bit.",
                      juce::Rectangle<int> (900, 734, 352, 40), juce::Justification::topLeft, 3);
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
    if (abRect.contains (e.getPosition())) { proc.ab.toggle (proc); repaint(); }
    else if (prevRect.contains (e.getPosition())) { proc.loadPreset ((proc.preset + n - 1) % n); repaint(); }
    else if (nextRect.contains (e.getPosition())) { proc.loadPreset ((proc.preset + 1) % n); repaint(); }
    else if (presetRect.contains (e.getPosition()))
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
    maxPage.gain->activity = juce::jlimit (0.0f, 1.0f, proc.grMax.load() / 6.0f);
    limPage.ceil->activity = juce::jlimit (0.0f, 1.0f, proc.grLim.load() / 3.0f);
    chain.repaint(); loud.repaint();
    if (maxPage.isVisible()) maxPage.repaint();
    if (limPage.isVisible()) limPage.repaint();
    for (auto* pg : std::initializer_list<juce::Component*> { &bassPage, &eqPage, &imPage, &dsPage, &busPage, &mbPage }) if (pg->isVisible()) pg->repaint();
    trim->repaint(); compare->repaint(); norm->repaint(); dither->repaint();
    if (++tick % 10 == 0) repaint (scaler.map ({ W - 320, 8, 312, 50 }));
}
}
