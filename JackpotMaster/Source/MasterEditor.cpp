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
static const char* onId (int m) { return m == MAXIM ? "mx_on" : m == LIMIT ? "lm_on" : nullptr; }
static float modGr (Processor& p, int m) { return m == MAXIM ? p.grMax.load() : m == LIMIT ? p.grLim.load() : 0.0f; }
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
    for (juce::Component* c : std::initializer_list<juce::Component*> { &chain, &maxPage, &limPage, &soon, &loud, trim.get(), compare.get(), norm.get(), dither.get() }) addAndMakeVisible (c);
    addAndMakeVisible (sizeChip);
    select (MAXIM);
    scaler.setup();
    startTimerHz (30);
}
void Editor::select (int m)
{
    sel = m;
    maxPage.setVisible (m == MAXIM); limPage.setVisible (m == LIMIT); soon.setVisible (! modInfo (m).ready); soon.mod = m;
    chain.repaint(); repaint();
}
void Editor::resized()
{
    const float sc = scaler.update();
    presetRect = { 450, 15, 330, 36 }; prevRect = { 412, 15, 36, 36 }; nextRect = { 782, 15, 36, 36 }; abRect = { 830, 15, 110, 36 };
    chain.setBounds (18, 76, 1244, 66);
    for (auto* pg : std::initializer_list<juce::Component*> { &maxPage, &limPage, &soon }) pg->setBounds (18, 154, 854, 628);
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
    trim->repaint(); compare->repaint(); norm->repaint(); dither->repaint();
    if (++tick % 10 == 0) repaint (scaler.map ({ W - 320, 8, 312, 50 }));
}
}
