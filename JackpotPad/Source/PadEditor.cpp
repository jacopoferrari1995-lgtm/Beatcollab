#include "Generator.h"
#include "PadEditor.h"
#include "KeyDetect.h"

namespace jp
{
juce::Colour padColour (int i)
{
    static const juce::uint32 c[16] = { 0xffff2e88, 0xffff5e3a, 0xffffb000, 0xfff5ff3a, 0xff7dff3a, 0xff19ffa0, 0xff19e3ff, 0xff3a8bff,
                                        0xff8b5cff, 0xffd23aff, 0xffff3ad2, 0xffff8a3a, 0xff3affea, 0xffb4ff3a, 0xffff3a5e, 0xff9fb4ff };
    return juce::Colour (c[(i % 16 + 16) % 16]);
}
juce::Font ft (float size, bool bold, bool mono)
{
    return mono ? jlook::mono (size) : jlook::ui (size, bold ? 2 : 1);
}
static juce::String noteName (int n)
{
    static const char* nm[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    return juce::String (nm[((n % 12) + 12) % 12]) + juce::String (n / 12 - 2);
}
static void glowText (juce::Graphics& g, const juce::String& t, juce::Rectangle<float> r, juce::Colour c, juce::Font f, juce::Justification j = juce::Justification::centredLeft)
{
    // alone leggero: 2 passate invece di 9 (il testo e la parte piu costosa da disegnare)
    g.setFont (f);
    g.setColour (c.withAlpha (0.22f)); g.drawText (t, r.translated (-0.7f, 0.4f), j); g.drawText (t, r.translated (0.7f, 0.4f), j);
    g.setColour (c); g.drawText (t, r, j);
}
static void insetPanel (juce::Graphics& g, juce::Rectangle<float> r, float rad)
{
    g.setColour (juce::Colours::black.withAlpha (0.6f)); g.fillRoundedRectangle (r.expanded (2), rad + 2);
    juce::ColourGradient cg (juce::Colour (0xff0c0d11), r.getX(), r.getY(), juce::Colour (0xff121419), r.getX(), r.getBottom(), false);
    g.setGradientFill (cg); g.fillRoundedRectangle (r, rad);
    g.setColour (juce::Colours::white.withAlpha (0.05f)); g.drawLine (r.getX() + rad, r.getBottom() + 1.5f, r.getRight() - rad, r.getBottom() + 1.5f, 1.0f);
    g.setColour (juce::Colours::black.withAlpha (0.7f)); g.drawLine (r.getX() + rad, r.getY() + 0.5f, r.getRight() - rad, r.getY() + 0.5f, 1.0f);
}
static void screw (juce::Graphics& g, float x, float y)
{
    juce::ColourGradient cg (juce::Colour (0xff5a606c), x - 4, y - 4, juce::Colour (0xff15171c), x + 4, y + 4, false);
    g.setGradientFill (cg); g.fillEllipse (x - 5, y - 5, 10, 10);
    g.setColour (juce::Colour (0xff0b0c0f)); g.drawLine (x - 3, y - 3, x + 3, y + 3, 1.5f); g.drawLine (x - 3, y + 3, x + 3, y - 3, 1.5f);
}

// ======================= Knob =======================
void Knob::paint (juce::Graphics& g)
{
    const float size = (float) juce::jmin (getWidth(), getHeight() - 26);
    const float v = get ? get() : 0.0f;
    jlook::KnobLook kl; kl.bipolar = bipolar; kl.big = size > 70; kl.hover = hover ? 1.0f : 0.0f; kl.active = dragging ? 1.0f : 0.0f;
    jlook::drawKnob (g, { (getWidth() - size) * 0.5f, 0, size, size }, v, col, kl);
    g.setFont (ft (9.0f, true)); g.setColour (Col::dim); g.drawText (label, juce::Rectangle<float> (0, size + 3, (float) getWidth(), 11), juce::Justification::centred);
    g.setFont (ft (10.0f, false, true)); g.setColour (dragging ? col.brighter (0.4f) : col.withAlpha (0.9f)); g.drawText (text(), juce::Rectangle<float> (0, size + 14, (float) getWidth(), 12), juce::Justification::centred);
}

// ======================= Selector =======================
void Selector::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    const auto lb = labelFn ? labelFn() : label;
    if (lb.isNotEmpty()) { g.setFont (ft (9.0f, true)); g.setColour (Col::dim); g.drawText (lb, b.removeFromTop (13), juce::Justification::centredLeft); }
    insetPanel (g, b, 4);
    juce::ColourGradient cg (Col::lcd1, 0, b.getY(), Col::lcd2, 0, b.getBottom(), false); g.setGradientFill (cg); g.fillRoundedRectangle (b.reduced (1), 4);
    glowText (g, name (get()), b.reduced (6, 0), col, ft (11.5f, true, true), juce::Justification::centred);
    g.setColour (col.withAlpha (0.4f)); g.setFont (ft (8.0f));
    if (menu) { g.drawText (juce::String::fromUTF8 ("\xe2\x96\xbc"), b.reduced (4, 0), juce::Justification::centredRight); return; }
    g.drawText (juce::String::fromUTF8 ("\xe2\x97\x80"), b.reduced (4, 0), juce::Justification::centredLeft);
    g.drawText (juce::String::fromUTF8 ("\xe2\x96\xb6"), b.reduced (4, 0), juce::Justification::centredRight);
}

// ======================= Btn =======================
void Btn::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (2);
    const bool on = lit ? lit() : false;
    g.setColour (juce::Colours::black.withAlpha (0.7f)); g.fillRoundedRectangle (b.translated (0, 2.5f), 6);
    auto f = b.translated (0, down ? 1.5f : 0);
    juce::ColourGradient cg (down ? juce::Colour (0xff20232a) : juce::Colour (0xff30343d), 0, f.getY(), juce::Colour (0xff15171c), 0, f.getBottom(), false);
    g.setGradientFill (cg); g.fillRoundedRectangle (f, 6);
    g.setColour (juce::Colours::white.withAlpha (0.08f)); g.drawLine (f.getX() + 6, f.getY() + 1, f.getRight() - 6, f.getY() + 1, 1.0f);
    g.setColour (juce::Colours::black.withAlpha (0.6f)); g.drawRoundedRectangle (f, 6, 1.0f);
    // striscia LED
    auto led = juce::Rectangle<float> (f.getX() + 8, f.getY() + 6, f.getWidth() - 16, 3);
    if (on) { g.setColour (ledCol.withAlpha (0.35f)); g.fillRoundedRectangle (led.expanded (3, 3), 3); g.setColour (ledCol.brighter (0.2f)); }
    else g.setColour (ledCol.withAlpha (0.12f));
    g.fillRoundedRectangle (led, 1.5f);
    g.setFont (ft (10.5f, true)); g.setColour (on ? Col::text : Col::dim); g.drawText (label, f.withTrimmedTop (8).toNearestInt(), juce::Justification::centred);
}

// ======================= Pad =======================
void PadComp::paint (juce::Graphics& g)
{
    auto& P = ed.proc; const auto c = padColour (P.pad[pad].colour.load());
    auto b = body(); const bool sel = ed.sel == pad;
    const float lum = juce::jlimit (0.0f, 1.0f, juce::jmax (glow, flash));
    if (held) b = b.reduced (2.0f).translated (0, 1.5f);
    // alone esterno (bloom)
    if (lum > 0.02f)
        for (int k = 4; k >= 1; --k) { g.setColour (c.withAlpha (0.07f * lum)); g.fillRoundedRectangle (b.expanded (k * 3.0f), 12 + k * 3.0f); }
    // incavo
    g.setColour (juce::Colours::black.withAlpha (0.85f)); g.fillRoundedRectangle (b.expanded (3).translated (0, 2), 13);
    // gomma traslucida
    juce::ColourGradient rub (juce::Colour (0xff2c2f37), b.getCentreX(), b.getY(), juce::Colour (0xff15171c), b.getCentreX(), b.getBottom(), false);
    g.setGradientFill (rub); g.fillRoundedRectangle (b, 11);
    // retroilluminazione
    if (lum > 0.01f)
    {
        juce::ColourGradient bl (c.withAlpha (0.95f * lum), b.getCentreX(), b.getCentreY(), c.withAlpha (0.18f * lum), b.getX(), b.getY(), true);
        g.setGradientFill (bl); g.fillRoundedRectangle (b.reduced (1), 11);
        juce::ColourGradient hot (juce::Colours::white.withAlpha (0.45f * lum * lum), b.getCentreX(), b.getCentreY(), juce::Colours::transparentWhite, b.getCentreX(), b.getY() + b.getHeight() * 0.1f, true);
        g.setGradientFill (hot); g.fillRoundedRectangle (b.reduced (1), 11);
    }
    // texture e bordo smussato
    if (dots.isEmpty()) { juce::Random rr (pad * 97 + 5); auto bb = body(); for (int i = 0; i < 140; ++i) dots.addEllipse (bb.getX() + rr.nextFloat() * bb.getWidth(), bb.getY() + rr.nextFloat() * bb.getHeight(), 1.3f, 1.3f); }
    g.setColour (juce::Colours::black.withAlpha (0.12f)); g.fillPath (dots);
    juce::ColourGradient bevel (juce::Colours::white.withAlpha (0.16f), 0, b.getY(), juce::Colours::transparentWhite, 0, b.getY() + 18, false);
    g.setGradientFill (bevel); g.fillRoundedRectangle (b.reduced (1.5f).withHeight (18), 10);
    g.setColour (juce::Colours::white.withAlpha (0.07f)); g.drawRoundedRectangle (b.reduced (0.5f), 11, 1.0f);
    // LED inferiore sempre acceso al minimo
    auto strip = juce::Rectangle<float> (b.getX() + 14, b.getBottom() - 5, b.getWidth() - 28, 2.5f);
    g.setColour (c.withAlpha (0.25f + 0.75f * lum)); g.fillRoundedRectangle (strip, 1.2f);
    if (sel) { g.setColour (c.withAlpha (0.9f)); g.drawRoundedRectangle (b.expanded (2.5f), 13, 1.6f); }
    if (dragOver) { g.setColour (Col::cyan); g.drawRoundedRectangle (b.reduced (3), 10, 2.0f); }
    // testi
    const int root = P.rootNote();
    auto tb = b.reduced (10, 8);
    g.setFont (ft (12.0f, true, true)); g.setColour (lum > 0.4f ? juce::Colours::white : Col::text.withAlpha (0.85f));
    g.drawText (juce::String (pad + 1).paddedLeft ('0', 2), tb, juce::Justification::topLeft);
    g.setFont (ft (9.5f, false, true)); g.setColour (c.withAlpha (0.85f)); g.drawText (noteName (root + pad), tb, juce::Justification::topRight);
    auto s = P.getSample (pad);
    g.setFont (ft (12.0f)); g.setColour (lum > 0.4f ? juce::Colours::white : Col::text.withAlpha (0.8f));
    const int sl = juce::roundToInt (P.pad[pad].get (SLICE));
    g.drawFittedText (s ? s->name + (sl > 0 ? " #" + juce::String (sl) : juce::String()) : juce::String ("vuoto"), tb.withTrimmedBottom (4).withTrimmedTop (14).toNearestInt(), juce::Justification::bottomLeft, 2, 0.75f);
    if (flash > 0.05f) { g.setFont (ft (9.0f, true, true)); g.setColour (juce::Colours::white.withAlpha (flash)); g.drawText (juce::String (P.lastVel[pad].load()), tb.withTrimmedBottom (4), juce::Justification::bottomRight); }
}
void PadComp::mouseDown (const juce::MouseEvent& e)
{
    ed.selectPad (pad);
    if (e.mods.isPopupMenu()) { ed.padMenu (pad); return; }
    if (ed.page != PG_SAMPLE) ed.setPage (PG_SAMPLE);
    auto b = body();
    // velocity: in alto forte, in basso piano; curva quadratica per avere più controllo sui colpi deboli
    const float y = juce::jlimit (0.0f, 1.0f, (e.position.y - b.getY()) / b.getHeight());
    int vel = juce::jlimit (1, 127, juce::roundToInt (127.0f * (1.0f - 0.92f * y * y) - 0.0f));
    if (e.mods.isShiftDown() || ed.fullLevel) vel = 127;
    held = true; ed.playPad (pad, vel); repaint();
}
void PadComp::mouseUp (const juce::MouseEvent&) { if (held) { held = false; ed.stopPad (pad); repaint(); } }
void PadComp::filesDropped (const juce::StringArray& files, int, int)
{
    dragOver = false; int n = 0;
    for (auto& f : files) { if (pad + n >= kPads) break; if (ed.proc.loadFile (juce::File (f), pad + n)) ++n; }
    ed.selectPad (pad);
}

// ======================= Schermo =======================
static const char* kPageNames[kNumPages] = { "KIT", "SUONI", "MIXER", "SAMPLE", "VELOCITY", "EQ + FILTRO", "COMP", "FX", "MASTER" };

static float freqToX (float f, juce::Rectangle<float> r) { return r.getX() + r.getWidth() * std::log (f / 20.0f) / std::log (1000.0f); }
static float xToFreq (float x, juce::Rectangle<float> r) { return 20.0f * std::pow (1000.0f, juce::jlimit (0.0f, 1.0f, (x - r.getX()) / r.getWidth())); }
static void freqGrid (juce::Graphics& g, juce::Rectangle<float> r, float dbRange)
{
    for (float f : { 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f })
    {
        const float x = freqToX (f, r); g.setColour (Col::phos.withAlpha (0.08f)); g.drawVerticalLine ((int) x, r.getY(), r.getBottom());
        g.setColour (Col::phos.withAlpha (0.35f)); g.setFont (ft (8.5f, false, true));
        g.drawText (f >= 1000 ? juce::String ((int) (f / 1000)) + "k" : juce::String ((int) f), juce::Rectangle<float> (x + 2, r.getBottom() - 11, 30, 10), juce::Justification::centredLeft);
    }
    for (float db = -dbRange; db <= dbRange; db += dbRange / 3)
    {
        const float y = r.getCentreY() - db / dbRange * r.getHeight() * 0.5f;
        g.setColour (Col::phos.withAlpha (std::abs (db) < 0.1f ? 0.25f : 0.07f)); g.drawHorizontalLine ((int) y, r.getX(), r.getRight());
    }
}

void Screen::chip (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& label, const juce::String& value, bool on, std::function<void (const juce::MouseEvent&)> click, juce::Colour c)
{
    g.setColour (on ? c.withAlpha (0.18f) : Col::phos.withAlpha (0.05f)); g.fillRoundedRectangle (r, 4);
    g.setColour (on ? c.withAlpha (0.9f) : Col::phos.withAlpha (0.25f)); g.drawRoundedRectangle (r, 4, 1.0f);
    if (label.isNotEmpty())
    {
        g.setFont (ft (8.5f, true, true)); g.setColour (Col::phos.withAlpha (0.55f)); g.drawText (label, r.reduced (6, 3).withHeight (10), juce::Justification::centredLeft);
        glowText (g, value, r.reduced (6, 2).withTrimmedTop (11), on ? c : Col::phos, ft (11.0f, true, true), juce::Justification::centredLeft);
    }
    else glowText (g, value, r, on ? c : Col::phos.withAlpha (0.8f), ft (10.5f, true, true), juce::Justification::centred);
    if (click) hots.push_back ({ r, std::move (click), nullptr, nullptr, nullptr });
}

void Screen::padHeader (juce::Graphics& g, juce::Rectangle<float> r)
{
    auto& P = ed.proc; const int p = ed.sel; auto s = P.getSample (p); const auto pc = padColour (P.pad[p].colour.load());
    g.setColour (pc); g.fillRoundedRectangle (r.getX(), r.getY() + 3, 4, r.getHeight() - 6, 2);
    glowText (g, "PAD " + juce::String (p + 1).paddedLeft ('0', 2) + "  " + noteName (P.rootNote() + p) + "  " + (s ? s->name.toUpperCase() : juce::String ("VUOTO")) + juce::String::fromUTF8 ("  \xe2\x96\xbe"),
              r.withTrimmedLeft (12).withWidth (330), Col::phos, ft (13.0f, true, true));
    hots.push_back ({ r.withTrimmedLeft (12).withWidth (330), [this] (const juce::MouseEvent&) { ed.openBrowser (ed.sel); }, nullptr, nullptr, nullptr });
    // gain e volume come barre grafiche
    auto bars = juce::Rectangle<float> (r.getRight() - 300, r.getY() + 2, 300, r.getHeight() - 4);
    auto drawBar = [&] (juce::Rectangle<float> b, const char* lab, float norm, const juce::String& val, bool bip, int param)
    {
        g.setFont (ft (8.5f, true, true)); g.setColour (Col::phos.withAlpha (0.6f)); g.drawText (lab, b.removeFromLeft (34), juce::Justification::centredLeft);
        auto t = b.removeFromLeft (80).withSizeKeepingCentre (80, 7);
        g.setColour (Col::phos.withAlpha (0.12f)); g.fillRoundedRectangle (t, 2);
        auto f = bip ? (norm >= 0.5f ? t.withLeft (t.getCentreX()).withWidth (t.getWidth() * (norm - 0.5f)) : t.withLeft (t.getX() + t.getWidth() * norm).withRight (t.getCentreX()))
                     : t.withWidth (t.getWidth() * norm);
        g.setColour (pc); g.fillRoundedRectangle (f, 2);
        if (bip) { g.setColour (Col::phos.withAlpha (0.5f)); g.drawVerticalLine ((int) t.getCentreX(), t.getY() - 2, t.getBottom() + 2); }
        glowText (g, val, b.withTrimmedLeft (4), Col::phos, ft (10.0f, true, true));
        hots.push_back ({ t.expanded (4, 8), [this, param, t] (const juce::MouseEvent& e) { ed.setPadParam (param, Editor::fromNorm (param, juce::jlimit (0.0f, 1.0f, (e.position.x - t.getX()) / t.getWidth()))); },
                          [this, param, t] (const juce::MouseEvent& e) { ed.setPadParam (param, Editor::fromNorm (param, juce::jlimit (0.0f, 1.0f, (e.position.x - t.getX()) / t.getWidth()))); }, nullptr,
                          [this, param] { ed.setPadParam (param, kPInfo[param].def); } });
    };
    drawBar (bars.removeFromLeft (150), "GAIN", Editor::toNorm (GAIN, P.pad[p].get (GAIN)), formatPadParam (GAIN, P.pad[p].get (GAIN)), true, GAIN);
    drawBar (bars, "VOL", Editor::toNorm (LEVEL, P.pad[p].get (LEVEL)), formatPadParam (LEVEL, P.pad[p].get (LEVEL)), false, LEVEL);
}

void Screen::paint (juce::Graphics& g)
{
    hots.clear();
    if (search.isVisible() != (ed.page == PG_BROWSE)) search.setVisible (ed.page == PG_BROWSE);
    auto b = getLocalBounds().toFloat();
    auto s = b.reduced (6);
    // fondo e vetro si disegnano una volta sola (immagini in cache alla risoluzione dello schermo)
    const float ps = juce::jmax (1.0f, g.getInternalContext().getPhysicalPixelScaleFactor());
    if (bgImg.isNull() || std::abs (ps - imgScale) > 0.01f || bgImg.getWidth() != juce::roundToInt (getWidth() * ps))
    {
        imgScale = ps; const int iw = juce::roundToInt (getWidth() * ps), ih = juce::roundToInt (getHeight() * ps);
        bgImg = juce::Image (juce::Image::ARGB, iw, ih, true); ovImg = juce::Image (juce::Image::ARGB, iw, ih, true);
        {
            juce::Graphics gb (bgImg); gb.addTransform (juce::AffineTransform::scale (ps));
            insetPanel (gb, b, 10);
            juce::ColourGradient bg (Col::lcd1, s.getCentreX(), s.getCentreY(), Col::lcd2, s.getX(), s.getY(), true);
            gb.setGradientFill (bg); gb.fillRoundedRectangle (s, 6);
            gb.setColour (Col::phos.withAlpha (0.03f));
            for (float x = s.getX(); x < s.getRight(); x += 12) gb.drawVerticalLine ((int) x, s.getY(), s.getBottom());
            for (float y = s.getY(); y < s.getBottom(); y += 12) gb.drawHorizontalLine ((int) y, s.getX(), s.getRight());
        }
        {
            juce::Graphics go (ovImg); go.addTransform (juce::AffineTransform::scale (ps));
            go.setColour (juce::Colours::black.withAlpha (0.2f)); for (float y = s.getY(); y < s.getBottom(); y += 3) go.drawHorizontalLine ((int) y, s.getX(), s.getRight());
            juce::ColourGradient gl (juce::Colours::white.withAlpha (0.05f), s.getX(), s.getY(), juce::Colours::transparentWhite, s.getX() + s.getWidth() * 0.35f, s.getY() + s.getHeight() * 0.6f, false);
            go.setGradientFill (gl); go.fillRoundedRectangle (s, 6);
        }
    }
    g.drawImage (bgImg, b);
    // schede
    const float tw = (s.getWidth() - 12) / kNumPages;
    for (int i = 0; i < kNumPages; ++i)
    {
        auto r = juce::Rectangle<float> (s.getX() + 6 + i * tw, s.getY() + 6, tw - 3, 22);
        const bool on = ed.page == i;
        g.setColour (on ? Col::phos : Col::phos.withAlpha (0.06f)); g.fillRoundedRectangle (r, 3);
        if (on) { g.setColour (Col::phos.withAlpha (0.25f)); g.fillRoundedRectangle (r.expanded (2), 4); g.setColour (Col::phos); g.fillRoundedRectangle (r, 3); }
        g.setFont (ft (10.5f, true, true)); g.setColour (on ? Col::lcd2 : Col::phos.withAlpha (0.75f)); g.drawText (kPageNames[i], r, juce::Justification::centred);
        hots.push_back ({ r, [this, i] (const juce::MouseEvent&) { ed.setPage (i); }, nullptr, nullptr, nullptr });
    }
    auto hdr = juce::Rectangle<float> (s.getX() + 10, s.getY() + 34, s.getWidth() - 20, 20);
    auto c = content();
    switch (ed.page)
    {
        case PG_KIT:
        {
            glowText (g, "KIT", hdr, Col::phos.withAlpha (0.6f), ft (11.0f, true, true));
            {   // nome intero del suono del pad selezionato
                auto sm = ed.proc.getSample (ed.sel);
                const juce::String nm = juce::String (ed.sel + 1).paddedLeft ('0', 2) + "  " + (sm ? sm->name : juce::String ("vuoto"));
                const auto f = ft (10.5f, true, true); g.setFont (f); g.setColour (Col::amber);
                auto area = hdr.withTrimmedLeft (44).withTrimmedRight (130);
                const float tw = juce::GlyphArrangement::getStringWidth (f, nm);
                if (tw <= area.getWidth()) g.drawText (nm, area, juce::Justification::centred);
                else
                {   // nome troppo lungo: scorre (come il display di una MPC), cosi si legge tutto
                    const float gap = 60, period = tw + gap;
                    const float off = std::fmod ((float) (juce::Time::getMillisecondCounter() % 1000000) * 0.04f, period);
                    g.saveState(); g.reduceClipRegion (area.toNearestInt());
                    for (float x = area.getX() - off; x < area.getRight(); x += period) g.drawText (nm, juce::Rectangle<float> (x, area.getY(), tw + 4, area.getHeight()), juce::Justification::centredLeft);
                    g.restoreState();
                }
            }
            juce::String rt; rt << juce::String (ed.proc.uiBpm.load(), 1) << " BPM  " << (ed.proc.uiHost.load() ? "HOST" : ed.proc.uiRunning.load() ? "INT" : "STOP");
            glowText (g, rt, hdr, Col::phos.withAlpha (0.8f), ft (10.5f, false, true), juce::Justification::centredRight);
            pageKit (g, c); break;
        }
        case PG_MASTER: glowText (g, juce::String::fromUTF8 ("MASTER  \xc2\xb7  REVERB  \xc2\xb7  DELAY  \xc2\xb7  SATURAZIONE  \xc2\xb7  BUS"), hdr, Col::phos, ft (12.0f, true, true)); pageMaster (g, c); break;
        case PG_BROWSE: padHeader (g, hdr); pageBrowse (g, c); break;
        case PG_MIX: glowText (g, juce::String::fromUTF8 ("MIXER  \xc2\xb7  GAIN  \xc2\xb7  PAN  \xc2\xb7  VOLUME  \xc2\xb7  MUTE / SOLO"), hdr, Col::phos, ft (12.0f, true, true)); pageMix (g, c); break;
        default:
            padHeader (g, hdr);
            if (ed.page == PG_SAMPLE) pageSample (g, c);
            else if (ed.page == PG_VEL) pageVel (g, c);
            else if (ed.page == PG_EQ) pageEq (g, c);
            else if (ed.page == PG_COMP) pageComp (g, c);
            else if (ed.page == PG_FX) pageFx (g, c);
    }
    g.drawImage (ovImg, b);
    // spiegazione del controllo sotto il mouse
    if (hint.isNotEmpty())
    {
        auto hr = s.reduced (10, 6).removeFromBottom (36);
        g.setColour (juce::Colours::black.withAlpha (0.88f)); g.fillRoundedRectangle (hr, 6); g.setColour (Col::amber.withAlpha (0.8f)); g.drawRoundedRectangle (hr, 6, 1.0f);
        g.setColour (Col::text); g.setFont (ft (10.0f, false, true)); g.drawFittedText (hint, hr.reduced (10, 3).toNearestInt(), juce::Justification::centredLeft, 2);
    }
}

// ---------- SUONI (banca per categoria) ----------
void Screen::setupSearch()
{
    addChildComponent (search);
    search.setFont (ft (11.5f, true, true)); search.setJustification (juce::Justification::centredLeft);
    search.setColour (juce::TextEditor::backgroundColourId, juce::Colours::black.withAlpha (0.35f));
    search.setColour (juce::TextEditor::outlineColourId, Col::phos.withAlpha (0.25f));
    search.setColour (juce::TextEditor::focusedOutlineColourId, Col::phos.withAlpha (0.7f));
    search.setColour (juce::TextEditor::textColourId, Col::text);
    search.setColour (juce::CaretComponent::caretColourId, Col::phos);
    search.setTextToShowWhenEmpty (juce::String::fromUTF8 ("cerca per nome o cartella\xe2\x80\xa6"), Col::phos.withAlpha (0.35f));
    search.setIndents (6, 2);
    search.onTextChange = [this] { browseScroll = 0; browseDirty = true; repaint(); };
    search.onEscapeKey = [this] { search.clear(); browseDirty = true; ed.grabKeyboardFocus(); repaint(); };
}

void Screen::itemMenu (int i)
{
    if (i < 0 || i >= (int) browseItems.size()) return;
    const auto it = browseItems[(size_t) i];
    juce::PopupMenu m, cats; m.addSectionHeader (it.name);
    const auto& cs = libCategories();
    for (int k = 0; k < cs.size(); ++k) cats.addItem (100 + k, cs[k], it.user && cs[k] != it.category, cs[k] == it.category);
    m.addSubMenu ("Sposta in categoria", cats, it.user);
    m.addItem (1, "Carica sul pad selezionato");
    m.addItem (4, "Carica come LAYER sul pad selezionato");
    m.addItem (2, "Mostra nel Finder", it.user);
    if (it.scanned) m.addItem (3, "Togli dall'indice (il file resta)");
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [this, it] (int r)
    {
        if (r >= 100) { moveLibItem (it, libCategories()[r - 100]); browseDirty = true; }
        else if (r == 1) { loadLibItem (ed.proc, it, ed.sel); ed.playPad (ed.sel, 100); ed.kitChanged(); }
        else if (r == 4) { loadLibLayer (ed.proc, it, ed.sel); ed.playPad (ed.sel, 100); ed.kitChanged(); }
        else if (r == 2) it.file.revealToUser();
        else if (r == 3) { ed.proc.sampleIndex->remove (it.file.getFullPathName()); browseDirty = true; }
        repaint();
    });
}

void Screen::pageBrowse (juce::Graphics& g, juce::Rectangle<float> c)
{
    auto& P = ed.proc; const auto& cats = libCategories(); auto& idx = *P.sampleIndex;
    const auto query = search.getText().trim();
    if (idx.generation.load() != browseGen) { browseGen = idx.generation.load(); browseDirty = true; }
    if (browseDirty)
    {   // la ricerca guarda solo nella categoria scelta, a meno di IN TUTTE
        if (query.isEmpty()) browseItems = libItems (cats[browseCat]);
        else { browseItems.clear(); for (auto& it : searchLibrary (query)) if (searchAll || it.category == cats[browseCat]) browseItems.push_back (it); }
        browseDirty = false;
    }
    auto bottom = c.removeFromBottom (30); c.removeFromBottom (6);
    auto left = c.removeFromLeft (150); c.removeFromLeft (10);
    const float ch = left.getHeight() / cats.size();
    for (int i = 0; i < cats.size(); ++i)
    {
        auto r = juce::Rectangle<float> (left.getX(), left.getY() + i * ch, left.getWidth(), ch - 1.5f);
        const bool on = i == browseCat && ! (searchAll && query.isNotEmpty());
        g.setColour (on ? Col::magenta.withAlpha (0.22f) : Col::phos.withAlpha (0.04f)); g.fillRoundedRectangle (r, 3);
        if (on) { g.setColour (Col::magenta); g.fillRoundedRectangle (r.getX(), r.getY() + 2, 3, r.getHeight() - 4, 1.5f); }
        glowText (g, cats[i], r.reduced (9, 0), on ? Col::magenta : Col::phos.withAlpha (0.85f), ft (11.5f, true, true));
        const int nScan = idx.count (cats[i]);
        if (nScan > 0) { g.setColour (Col::cyan.withAlpha (on ? 0.9f : 0.55f)); g.setFont (ft (9.0f, false, true)); g.drawText (juce::String (nScan), r.reduced (6, 0), juce::Justification::centredRight); }
        hots.push_back ({ r, [this, i] (const juce::MouseEvent&) { browseCat = i; browseScroll = 0; searchAll = false; browseDirty = true; }, nullptr, nullptr, nullptr });
    }
    // barra di ricerca e stato dello SCAN
    auto top = c.removeFromTop (24); c.removeFromTop (4);
    const auto sb = top.removeFromLeft (top.getWidth() * 0.48f).toNearestInt();
    if (search.getBounds() != sb) search.setBounds (sb);
    top.removeFromLeft (6);
    chip (g, top.removeFromLeft (84), "", searchAll ? "IN TUTTE" : "IN " + cats[browseCat].upToFirstOccurrenceOf (" ", false, false), searchAll, [this] (const juce::MouseEvent&) { searchAll = ! searchAll; browseDirty = true; }, Col::cyan);
    hots.back().tip = "Dove cerca la ricerca: solo nella categoria scelta a sinistra, oppure in tutte.";
    top.removeFromLeft (6);
    juce::String st;
    if (idx.scanning()) st << "SCAN  " << idx.done.load() << " / " << idx.total.load() << "  +" << idx.added.load();
    else if (query.isNotEmpty()) st << browseItems.size() << " RISULTATI";
    else st << browseItems.size() << " SUONI" << (idx.size() > 0 ? juce::String::fromUTF8 ("  \xc2\xb7  ") + juce::String (idx.size()) + " IN INDICE" : juce::String());
    glowText (g, st, top, idx.scanning() ? Col::amber : Col::phos.withAlpha (0.7f), ft (9.5f, true, true), juce::Justification::centredRight);
    if (idx.scanning() && idx.total.load() > 0)
    {
        auto pb = juce::Rectangle<float> (top.getX(), top.getBottom() - 2, top.getWidth(), 2);
        g.setColour (Col::amber.withAlpha (0.2f)); g.fillRect (pb);
        g.setColour (Col::amber); g.fillRect (pb.withWidth (pb.getWidth() * idx.done.load() / (float) idx.total.load()));
    }
    // elenco su due colonne
    auto cur = P.getSample (ed.sel); const juce::String curPath = cur ? cur->path : juce::String();
    const float rh = 25; const int rows = juce::jmax (1, (int) (c.getHeight() / rh)), cols = 2, perPage = rows * cols;
    browseScroll = juce::jlimit (0, juce::jmax (0, ((int) browseItems.size() - perPage + 1) / 2 * 2), browseScroll);
    const float cw = c.getWidth() / cols;
    auto wheel = [this] (float d) { browseScroll += d > 0 ? -2 : 2; repaint(); };
    hots.push_back ({ c, nullptr, nullptr, wheel, nullptr });
    for (int k = 0; k < perPage; ++k)
    {
        const int i = browseScroll + k; if (i >= (int) browseItems.size()) break;
        const auto& it = browseItems[(size_t) i];
        auto r = juce::Rectangle<float> (c.getX() + (k / rows) * cw, c.getY() + (k % rows) * rh, cw - 6, rh - 2);
        const auto key = it.user ? it.file.getFullPathName() : descToPath (it.d);
        const bool on = curPath.isNotEmpty() && key == curPath, pend = key == pendKey && ! on;
        g.setColour (on ? padColour (P.pad[ed.sel].colour.load()).withAlpha (0.25f) : pend ? Col::amber.withAlpha (0.16f) : Col::phos.withAlpha (0.03f)); g.fillRoundedRectangle (r, 3);
        if (pend) { g.setColour (Col::amber); g.drawRoundedRectangle (r, 3, 1.4f); }
        const auto col = on ? Col::text : it.scanned ? Col::cyan : (it.user ? Col::amber : Col::phos);
        auto tr = r.reduced (8, 0);
        if (pend) { g.setColour (Col::amber); g.setFont (ft (8.5f, true, true)); g.drawText ("CLIC = CARICA", tr.removeFromRight (84), juce::Justification::centredRight); }
        else if (query.isNotEmpty()) { g.setColour (Col::dim); g.setFont (ft (8.5f, false, true)); g.drawText (it.category, tr.removeFromRight (66), juce::Justification::centredRight); }
        glowText (g, (it.user && ! it.scanned ? juce::String::fromUTF8 ("\xe2\x98\x85 ") : juce::String()) + it.name, tr, pend ? Col::amber : col, ft (13.0f, on || pend, true));
        hots.push_back ({ r, [this, i, key] (const juce::MouseEvent& e)
                          {
                              if (i >= (int) browseItems.size()) return;
                              if (e.mods.isPopupMenu()) { itemMenu (i); return; }
                              // primo clic: si accende e si ascolta; secondo clic sullo stesso: si carica sul pad
                              if (pendKey != key) { pendKey = key; ed.proc.preview (libItemSample (ed.proc, browseItems[(size_t) i])); return; }
                              pendKey.clear(); ed.proc.preview (nullptr);
                              loadLibItem (ed.proc, browseItems[(size_t) i], ed.sel); ed.playPad (ed.sel, 100); ed.kitChanged();
                          }, nullptr, wheel, nullptr });
    }
    if ((int) browseItems.size() > perPage)
    {
        const float frac = (float) perPage / browseItems.size(), pos = (float) browseScroll / browseItems.size();
        g.setColour (Col::phos.withAlpha (0.1f)); g.fillRect (c.getRight() - 3, c.getY(), 3.0f, c.getHeight());
        g.setColour (Col::phos.withAlpha (0.6f)); g.fillRect (c.getRight() - 3, c.getY() + c.getHeight() * pos, 3.0f, juce::jmax (6.0f, c.getHeight() * frac));
    }
    if (browseItems.empty())
        glowText (g, query.isNotEmpty() ? juce::String ("NESSUN RISULTATO") : (cats[browseCat] == "LOOP" || cats[browseCat] == "FX" || cats[browseCat] == "VOCI") ? juce::String::fromUTF8 ("NESSUN SUONO  \xc2\xb7  usa SCAN per trovare i tuoi") : juce::String ("NESSUN SUONO"),
                  c, Col::phos.withAlpha (0.4f), ft (11.0f, true, true), juce::Justification::centred);
    // azioni
    const float bw = (bottom.getWidth() - 24) / 4;
    chip (g, bottom.removeFromLeft (bw), "", "AGGIUNGI I TUOI...", false, [this] (const juce::MouseEvent&) { ed.addToLibraryDialog(); }, Col::amber); bottom.removeFromLeft (8);
    chip (g, bottom.removeFromLeft (bw), "", "SALVA SUONO DEL PAD", false, [this] (const juce::MouseEvent&)
          { savePadToLibrary (ed.proc, ed.sel, libCategories()[browseCat]); browseDirty = true; }, Col::amber); bottom.removeFromLeft (8);
    chip (g, bottom.removeFromLeft (bw), "", idx.scanning() ? "SCAN IN CORSO..." : juce::String::fromUTF8 ("SCAN CARTELLE \xe2\x96\xbe"), true, [this] (const juce::MouseEvent&) { ed.scanMenu(); }, Col::cyan); bottom.removeFromLeft (8);
    chip (g, bottom, "", "APRI CARTELLA", false, [this] (const juce::MouseEvent&) { libFolder (libCategories()[browseCat]).revealToUser(); }, Col::phos);
}

// ---------- KIT ----------
void Screen::pageKit (juce::Graphics& g, juce::Rectangle<float> c)
{
    auto& P = ed.proc;
    if (kitSmall)
    {   // colonna stretta: si riapre con un clic, restano a portata i RANDOM
        auto left = c.removeFromLeft (112); c.removeFromLeft (10);
        auto tg = left.removeFromTop (30); left.removeFromTop (6);
        chip (g, tg, "", juce::String::fromUTF8 ("KIT  \xe2\x96\xb6"), false, [this] (const juce::MouseEvent&) { kitSmall = false; repaint(); }, Col::phos);
        hots.back().tip = "Mostra di nuovo l'elenco dei kit.";
        g.setFont (ft (10.5f, true, true)); g.setColour (Col::magenta);
        g.drawFittedText (P.kitName.toUpperCase(), left.removeFromTop (40).toNearestInt(), juce::Justification::centred, 3, 0.7f);
        left.removeFromTop (6);
        auto bigV = [&] (juce::Rectangle<float> r, const juce::String& t, const juce::String& sub, bool on, std::function<void (const juce::MouseEvent&)> fn)
        {
            g.setColour (on ? Col::amber.withAlpha (0.22f) : Col::phos.withAlpha (0.06f)); g.fillRoundedRectangle (r, 6);
            g.setColour (on ? Col::amber : Col::phos.withAlpha (0.3f)); g.drawRoundedRectangle (r.reduced (0.5f), 6, 1.4f);
            glowText (g, t, r.withTrimmedBottom (14), on ? Col::amber : Col::phos, ft (13.0f, true, true), juce::Justification::centred);
            g.setFont (ft (8.5f, false, true)); g.setColour (Col::phos.withAlpha (0.55f)); g.drawText (sub, r.withTrimmedTop (r.getHeight() - 20), juce::Justification::centred);
            hots.push_back ({ r, std::move (fn), nullptr, nullptr, nullptr });
        };
        const float bh = juce::jmin (78.0f, (left.getHeight() - 12) / 3);
        bigV (left.removeFromTop (bh), "RANDOM KIT", "tutti i pad", true, [this] (const juce::MouseEvent&) { randomKit (ed.proc, randMode); ed.kitChanged(); ed.playPad (0, 100); }); left.removeFromTop (6);
        bigV (left.removeFromTop (bh), "RANDOM PAD", "solo il pad " + juce::String (ed.sel + 1), true, [this] (const juce::MouseEvent&) { randomPad (ed.proc, ed.sel, randMode); ed.kitChanged(); ed.playPad (ed.sel, 100); }); left.removeFromTop (6);
        bigV (left.removeFromTop (bh), kRandModes[randMode], "sorgente", false, [this] (const juce::MouseEvent&) { randMode = (randMode + 1) % 3; repaint(); });
        { auto strip = c.removeFromBottom (58); c.removeFromBottom (6); pageSample (g, c); masterStrip (g, strip); }
        return;
    }
    auto left = c.removeFromLeft (240); c.removeFromLeft (10);
    // riduci la colonna
    {
        auto tg = left.removeFromTop (24); left.removeFromTop (4);
        chip (g, tg.removeFromRight (120), "", juce::String::fromUTF8 ("\xe2\x97\x80  RIDUCI"), false, [this] (const juce::MouseEvent&) { kitSmall = true; repaint(); }, Col::phos);
        hots.back().tip = "Stringe l'elenco dei kit: piu spazio alla forma d'onda e alle misure.";
    }
    // nome del kit e frecce
    auto nm = left.removeFromTop (44);
    const bool user = P.kitFile.existsAsFile();
    g.setFont (ft (8.5f, true, true)); g.setColour (Col::phos.withAlpha (0.5f)); g.drawText (user ? "KIT UTENTE" : "KIT DI FABBRICA", nm.removeFromTop (12), juce::Justification::centredLeft);
    auto arrowL = nm.removeFromLeft (22), arrowR = nm.removeFromRight (22);
    glowText (g, P.kitName.toUpperCase(), nm, Col::magenta, ft (15.0f, true, true), juce::Justification::centred);
    glowText (g, juce::String::fromUTF8 ("\xe2\x97\x80"), arrowL, Col::phos, ft (13.0f), juce::Justification::centred);
    glowText (g, juce::String::fromUTF8 ("\xe2\x96\xb6"), arrowR, Col::phos, ft (13.0f), juce::Justification::centred);
    // elenco: fabbrica + utente
    struct Item { juce::String name, style; int factory; juce::File f; };
    std::vector<Item> items;
    for (int i = 0; i < (int) factoryKits().size(); ++i) items.push_back ({ factoryKits()[(size_t) i].name, factoryKits()[(size_t) i].style, i, {} });
    for (auto& f : P.userKits()) items.push_back ({ f.getFileNameWithoutExtension(), "utente", -1, f });
    int cur = -1;
    for (int i = 0; i < (int) items.size(); ++i) if ((items[(size_t) i].factory >= 0 && ! user && items[(size_t) i].name == P.kitName) || (items[(size_t) i].factory < 0 && items[(size_t) i].f == P.kitFile)) cur = i;
    auto loadItem = [this, items] (int i)
    {
        if (i < 0 || i >= (int) items.size()) return;
        if (items[(size_t) i].factory >= 0) ed.proc.loadFactoryKit (items[(size_t) i].factory); else ed.proc.loadKit (items[(size_t) i].f);
        ed.kitChanged();
    };
    hots.push_back ({ arrowL, [loadItem, cur, n = (int) items.size()] (const juce::MouseEvent&) { loadItem ((cur - 1 + n) % n); }, nullptr, nullptr, nullptr });
    hots.push_back ({ arrowR, [loadItem, cur, n = (int) items.size()] (const juce::MouseEvent&) { loadItem ((cur + 1) % n); }, nullptr, nullptr, nullptr });
    auto btnRow = left.removeFromBottom (30); left.removeFromBottom (5);
    auto rndRow = left.removeFromBottom (44); left.removeFromBottom (6);
    auto keyRow = left.removeFromBottom (26); left.removeFromBottom (6);
    {
        // RANDOM grandi: si usano di continuo
        auto big = [&] (juce::Rectangle<float> r, const juce::String& t, const juce::String& sub, bool on, std::function<void (const juce::MouseEvent&)> fn)
        {
            g.setColour (on ? Col::amber.withAlpha (0.22f) : Col::phos.withAlpha (0.06f)); g.fillRoundedRectangle (r, 6);
            g.setColour (on ? Col::amber : Col::phos.withAlpha (0.3f)); g.drawRoundedRectangle (r.reduced (0.5f), 6, 1.4f);
            glowText (g, t, r.withTrimmedBottom (sub.isEmpty() ? 0 : 12), on ? Col::amber : Col::phos, ft (14.0f, true, true), juce::Justification::centred);
            if (sub.isNotEmpty()) { g.setFont (ft (8.5f, false, true)); g.setColour (Col::phos.withAlpha (0.55f)); g.drawText (sub, r.withTrimmedTop (r.getHeight() - 16), juce::Justification::centred); }
            hots.push_back ({ r, std::move (fn), nullptr, nullptr, nullptr });
        };
        const float rw = (rndRow.getWidth() - 12) / 3;
        big (rndRow.removeFromLeft (rw * 1.1f), "RANDOM KIT", "tutti i pad", true, [this] (const juce::MouseEvent&) { randomKit (ed.proc, randMode); ed.kitChanged(); ed.playPad (0, 100); });
        rndRow.removeFromLeft (6);
        big (rndRow.removeFromLeft (rw * 1.1f), "RANDOM PAD", "solo il pad " + juce::String (ed.sel + 1), true, [this] (const juce::MouseEvent&) { randomPad (ed.proc, ed.sel, randMode); ed.kitChanged(); ed.playPad (ed.sel, 100); });
        rndRow.removeFromLeft (6);
        big (rndRow, kRandModes[randMode], "sorgente", false, [this] (const juce::MouseEvent&) { randMode = (randMode + 1) % 3; repaint(); });
        const int kl = P.keyLock();
        chip (g, keyRow, "", juce::String::fromUTF8 ("TONALIT\xc3\x80 KIT: ") + (kl < 0 ? juce::String ("LIBERA") : keyName (kl) + (kl >= 12 ? " (minore)" : " (maggiore)")) + juce::String::fromUTF8 ("  \xe2\x96\xbe"), kl >= 0,
              [this] (const juce::MouseEvent&) { ed.keyLockMenu(); }, Col::cyan);
    }
    const float bw = (btnRow.getWidth() - 8) / 3;
    chip (g, btnRow.removeFromLeft (bw), "", "SALVA", true, [this] (const juce::MouseEvent&) { ed.saveKit (false); }, Col::magenta); btnRow.removeFromLeft (4);
    chip (g, btnRow.removeFromLeft (bw), "", "SALVA COME", false, [this] (const juce::MouseEvent&) { ed.saveKit (true); }, Col::magenta); btnRow.removeFromLeft (4);
    chip (g, btnRow, "", "APRI...", false, [this] (const juce::MouseEvent&) { ed.loadKitDialog(); }, Col::magenta);
    // lista
    const float lh = 22; const int vis = (int) (left.getHeight() / lh);
    kitScroll = juce::jlimit (0, juce::jmax (0, (int) items.size() - vis), kitScroll);
    g.setColour (Col::phos.withAlpha (0.05f)); g.fillRoundedRectangle (left, 4);
    for (int k = 0; k < vis && k + kitScroll < (int) items.size(); ++k)
    {
        const int i = k + kitScroll; auto r = juce::Rectangle<float> (left.getX(), left.getY() + k * lh, left.getWidth(), lh);
        if (i == cur) { g.setColour (Col::magenta.withAlpha (0.2f)); g.fillRect (r); }
        if (i > 0 && items[(size_t) i].factory < 0 && items[(size_t) i - 1].factory >= 0) { g.setColour (Col::phos.withAlpha (0.3f)); g.drawHorizontalLine ((int) r.getY(), r.getX(), r.getRight()); }
        g.setFont (ft (12.5f, i == cur, true)); g.setColour (i == cur ? Col::magenta : Col::phos.withAlpha (0.9f));
        g.drawText (items[(size_t) i].name, r.reduced (6, 0), juce::Justification::centredLeft);
        g.setFont (ft (8.5f, false, true)); g.setColour (Col::phos.withAlpha (0.4f));
        g.drawText (items[(size_t) i].style, r.reduced (6, 0), juce::Justification::centredRight);
        hots.push_back ({ r, [loadItem, i] (const juce::MouseEvent&) { loadItem (i); }, nullptr, [this] (float d) { kitScroll += d > 0 ? -1 : 1; repaint(); }, nullptr });
    }
    // ---- il campione del pad scelto (tutta la pagina SAMPLE) e sotto la striscia MASTER ----
    { auto strip = c.removeFromBottom (58); c.removeFromBottom (6); pageSample (g, c); masterStrip (g, strip); }
}

// ======================= misure e testina della pagina KIT =======================
void KitMonitor::tick()
{
    auto& P = ed.proc;
    const float l = P.lufsS.load(), ph = P.peakHold.load();
    const float m[2] = { P.meterL.load(), P.meterR.load() };
    bool ch = std::abs (l - lastL) > 0.05f || ph != lastP;
    for (int c = 0; c < 2; ++c) { const float v = juce::jmax (m[c], pkS[c] * 0.88f); if (std::abs (v - pkS[c]) > 1e-4f) ch = true; pkS[c] = v; }
    if (ch) { lastL = l; lastP = ph; repaint(); }
}
void KitMonitor::paint (juce::Graphics& g)
{
    auto& P = ed.proc; auto b = getLocalBounds().toFloat();
    const float l = P.lufsS.load();
    auto lr = b.removeFromLeft (74);
    glowText (g, l < -69 ? juce::String ("--.-") : juce::String (l, 1), lr.removeFromTop (26), l > -9 ? Col::magenta : l > -14 ? Col::amber : Col::phos, ft (19.0f, true, true));
    g.setFont (ft (7.5f, true, true)); g.setColour (Col::phos.withAlpha (0.55f)); g.drawText ("LUFS SHORT", lr, juce::Justification::topLeft);
    auto pr = b.removeFromRight (64);
    const float ph = juce::Decibels::gainToDecibels (P.peakHold.load(), -60.0f);
    glowText (g, ph <= -59 ? juce::String ("--.-") : juce::String (ph, 1), pr.removeFromTop (26), ph > -0.3f ? Col::red : Col::phos, ft (19.0f, true, true), juce::Justification::centredRight);
    g.setFont (ft (7.5f, true, true)); g.setColour (Col::phos.withAlpha (0.55f)); g.drawText ("PICCO MAX", pr, juce::Justification::topRight);
    b.reduce (8, 4);
    for (int c = 0; c < 2; ++c)
    {
        auto r = juce::Rectangle<float> (b.getX() + 10, b.getY() + 4 + c * 13, b.getWidth() - 10, 7);
        g.setFont (ft (7.5f, true, true)); g.setColour (Col::phos.withAlpha (0.5f)); g.drawText (c ? "R" : "L", juce::Rectangle<float> (b.getX(), r.getY() - 3, 10, 13), juce::Justification::centredLeft);
        g.setColour (juce::Colours::white.withAlpha (0.05f)); g.fillRoundedRectangle (r, 2);
        const float db = juce::Decibels::gainToDecibels (pkS[c], -48.0f), t = juce::jlimit (0.0f, 1.0f, (db + 48.0f) / 48.0f);
        juce::ColourGradient mg (Col::green, r.getX(), 0, Col::red, r.getRight(), 0, false); mg.addColour (0.8, Col::amber);
        g.setGradientFill (mg); g.fillRoundedRectangle (r.withWidth (r.getWidth() * t), 2);
    }
    g.setFont (ft (7.0f, false, true)); g.setColour (Col::phos.withAlpha (0.35f));
    for (int d : { -36, -24, -12, -6, 0 }) { const float x = b.getX() + 10 + (b.getWidth() - 10) * (d + 48) / 48.0f; g.drawText (juce::String (d), juce::Rectangle<float> (x - 12, b.getY() + 30, 24, 10), juce::Justification::centred); }
}
void KitMonitor::mouseDown (const juce::MouseEvent& e) { if (e.position.x > getWidth() - 70) { ed.proc.peakHold = 0.0f; repaint(); } }
void PlayheadView::tick()
{
    const float pos = ed.proc.padPos[ed.sel].load();
    if (std::abs (pos - last) > 0.0005f) { last = pos; repaint(); }
}
void PlayheadView::paint (juce::Graphics& g)
{
    if (last < 0) return;
    const float x = getWidth() * juce::jlimit (0.0f, 1.0f, last);
    g.setColour (juce::Colours::white.withAlpha (0.15f)); g.fillRect (x - 3, 0.0f, 6.0f, (float) getHeight());
    g.setColour (juce::Colours::white); g.fillRect (x - 0.75f, 0.0f, 1.5f, (float) getHeight());
}

// striscia MASTER in fondo alla pagina KIT: misure dal vivo e le mandate principali; il resto nella pagina MASTER
void Screen::masterStrip (juce::Graphics& g, juce::Rectangle<float> r)
{
    g.setColour (Col::magenta.withAlpha (0.06f)); g.fillRoundedRectangle (r, 4); g.setColour (Col::magenta.withAlpha (0.35f)); g.drawRoundedRectangle (r, 4, 1.0f);
    auto lab = r.removeFromLeft (22);
    g.saveState(); g.addTransform (juce::AffineTransform::rotation (-juce::MathConstants<float>::halfPi, lab.getCentreX(), lab.getCentreY()));
    g.setFont (ft (8.5f, true, true)); g.setColour (Col::magenta); g.drawText ("MASTER", juce::Rectangle<float> (lab.getCentreX() - 30, lab.getCentreY() - 6, 60, 12), juce::Justification::centred);
    g.restoreState();
    r.reduce (6, 6);
    auto met = r.removeFromLeft (juce::jmin (220.0f, r.getWidth() * 0.36f)); r.removeFromLeft (8);
    if (ed.kitMon.getBounds() != met.toNearestInt()) ed.kitMon.setBounds (met.toNearestInt());
    auto open = r.removeFromRight (70); r.removeFromRight (6);
    chip (g, open, "", juce::String::fromUTF8 ("MASTER \xe2\x96\xb6"), false, [this] (const juce::MouseEvent&) { ed.setPage (PG_MASTER); }, Col::magenta);
    hots.back().tip = "Pagina MASTER completa: riverbero, delay, compressore e uscita con tutti i dettagli.";
    struct G { const char* id; const char* lab; };
    static const G gs[] = { { "revKit", "REVERB" }, { "dlyKit", "DELAY" }, { "busAmt", "COMP" }, { "satAmt", "SATURA" }, { "out", "OUTPUT" } };
    const float cw = (r.getWidth() - 4 * 5) / 5;
    for (int k = 0; k < 5; ++k)
    {
        auto cr = r.removeFromLeft (cw); r.removeFromLeft (5);
        auto* pr = ed.proc.apvts.getParameter (gs[k].id); const juce::String id (gs[k].id);
        const float nv = pr->getValue(), v = pr->convertFrom0to1 (nv);
        juce::String txt = id == "out" ? (v > 0 ? "+" : "") + juce::String (v, 1) + " dB" : juce::String (juce::roundToInt (nv * 100)) + "%";
        if (id == "busAmt") { const int bm = (int) ed.proc.apvts.getRawParameterValue ("busMode")->load(); if (bm == 0) txt = "OFF"; }
        const bool on = id == "out" ? std::abs (v) > 0.05f : (id == "busAmt" ? (int) ed.proc.apvts.getRawParameterValue ("busMode")->load() > 0 : nv > 0.005f);
        chip (g, cr, gs[k].lab, txt, on, nullptr, Col::magenta);
        // barretta del valore
        g.setColour (Col::magenta.withAlpha (0.5f)); g.fillRect (cr.getX() + 4, cr.getBottom() - 4, (cr.getWidth() - 8) * nv, 2.0f);
        hots.push_back ({ cr, [this, pr, id] (const juce::MouseEvent& e)
                          {
                              dragStart = pr->getValue(); pr->beginChangeGesture();
                              if (id == "busAmt" && (int) ed.proc.apvts.getRawParameterValue ("busMode")->load() == 0) { auto* bm = ed.proc.apvts.getParameter ("busMode"); bm->setValueNotifyingHost (bm->convertTo0to1 (1.0f)); }
                              juce::ignoreUnused (e);
                          },
                          [this, pr] (const juce::MouseEvent& e) { pr->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, dragStart - e.getDistanceFromDragStartY() / (e.mods.isShiftDown() ? 800.0f : 160.0f))); },
                          [pr] (float d) { pr->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, pr->getValue() + (d > 0 ? 0.02f : -0.02f))); },
                          [pr] { pr->setValueNotifyingHost (pr->getDefaultValue()); } });
        hots.back().tip = id == "busAmt" ? "Compressore sul kit: trascina su/giu per la quantita (si accende da solo). Il tipo (GLUE, PUNCH, SMASH) si sceglie nella pagina MASTER."
                        : "Trascina su/giu, rotella, doppio clic = valore di partenza. Dettagli nella pagina MASTER.";
    }
}

// ---------- MIXER ----------
void Screen::pageMix (juce::Graphics& g, juce::Rectangle<float> c)
{
    auto& P = ed.proc; const float sw = c.getWidth() / kPads;
    for (int p = 0; p < kPads; ++p)
    {
        auto st = juce::Rectangle<float> (c.getX() + p * sw, c.getY(), sw, c.getHeight()).reduced (2, 0);
        const auto pc = padColour (P.pad[p].colour.load()); const bool sel = p == ed.sel;
        g.setColour (sel ? pc.withAlpha (0.12f) : Col::phos.withAlpha (0.03f)); g.fillRoundedRectangle (st, 3);
        if (sel) { g.setColour (pc.withAlpha (0.7f)); g.drawRoundedRectangle (st, 3, 1.0f); }
        auto r = st.reduced (2, 3);
        g.setFont (ft (9.0f, true, true)); g.setColour (pc); g.drawText (juce::String (p + 1), r.removeFromTop (11), juce::Justification::centred);
        // gain: arco
        auto ga = r.removeFromTop (30); const float gn = Editor::toNorm (GAIN, P.pad[p].get (GAIN));
        { const float cx = ga.getCentreX(), cy = ga.getCentreY() + 2, rad = 10; juce::Path tr; tr.addCentredArc (cx, cy, rad, rad, 0, -2.3f, 2.3f, true);
          g.setColour (Col::phos.withAlpha (0.15f)); g.strokePath (tr, juce::PathStrokeType (2.5f));
          juce::Path v; const float a = -2.3f + gn * 4.6f; v.addCentredArc (cx, cy, rad, rad, 0, juce::jmin (0.0f, a), juce::jmax (0.0f, a), true);
          g.setColour (pc); g.strokePath (v, juce::PathStrokeType (2.5f)); }
        hots.push_back ({ ga, [this, p] (const juce::MouseEvent&) { ed.selectPad (p); }, [this, p, start = P.pad[p].get (GAIN)] (const juce::MouseEvent& e) mutable
                          { ed.proc.pad[p].v[GAIN] = juce::jlimit (-24.0f, 24.0f, std::round ((start - e.getDistanceFromDragStartY() * 0.25f) * 2) / 2); },
                          [this, p] (float d) { ed.proc.pad[p].v[GAIN] = juce::jlimit (-24.0f, 24.0f, ed.proc.pad[p].get (GAIN) + (d > 0 ? 0.5f : -0.5f)); },
                          [this, p] { ed.proc.pad[p].v[GAIN] = 0; } });
        // pan
        auto pn = r.removeFromTop (12).reduced (2, 3); const float pv = P.pad[p].get (PAN);
        g.setColour (Col::phos.withAlpha (0.15f)); g.fillRect (pn);
        g.setColour (pc); g.fillRect (juce::Rectangle<float> (pn.getCentreX() + pv * pn.getWidth() * 0.5f - 1.5f, pn.getY() - 1, 3, pn.getHeight() + 2));
        hots.push_back ({ pn.expanded (0, 3), [this, p, pn] (const juce::MouseEvent& e) { ed.selectPad (p); ed.proc.pad[p].v[PAN] = juce::jlimit (-1.0f, 1.0f, (e.position.x - pn.getCentreX()) / (pn.getWidth() * 0.5f)); },
                          [this, p, pn] (const juce::MouseEvent& e) { ed.proc.pad[p].v[PAN] = juce::jlimit (-1.0f, 1.0f, (e.position.x - pn.getCentreX()) / (pn.getWidth() * 0.5f)); }, nullptr, [this, p] { ed.proc.pad[p].v[PAN] = 0; } });
        // mute / solo
        auto ms = r.removeFromBottom (14); auto m = ms.removeFromLeft (ms.getWidth() / 2).reduced (1), so = ms.reduced (1);
        const bool mu = P.pad[p].get (MUTE) > 0.5f, sl = P.pad[p].get (SOLO) > 0.5f;
        g.setColour (mu ? Col::red : Col::phos.withAlpha (0.12f)); g.fillRoundedRectangle (m, 2); g.setColour (sl ? Col::amber : Col::phos.withAlpha (0.12f)); g.fillRoundedRectangle (so, 2);
        g.setFont (ft (8.0f, true, true)); g.setColour (mu ? Col::lcd2 : Col::phos.withAlpha (0.7f)); g.drawText ("M", m, juce::Justification::centred);
        g.setColour (sl ? Col::lcd2 : Col::phos.withAlpha (0.7f)); g.drawText ("S", so, juce::Justification::centred);
        hots.push_back ({ m, [this, p] (const juce::MouseEvent&) { auto& v = ed.proc.pad[p].v[MUTE]; v = v.load() > 0.5f ? 0.0f : 1.0f; }, nullptr, nullptr, nullptr });
        hots.push_back ({ so, [this, p] (const juce::MouseEvent&) { auto& v = ed.proc.pad[p].v[SOLO]; v = v.load() > 0.5f ? 0.0f : 1.0f; }, nullptr, nullptr, nullptr });
        // valore in dB
        auto vt = r.removeFromBottom (12); g.setFont (ft (8.0f, false, true)); g.setColour (Col::phos.withAlpha (0.8f));
        const float lv = P.pad[p].get (LEVEL); g.drawText (lv < 0.001f ? juce::String ("-inf") : juce::String (juce::Decibels::gainToDecibels (lv), 1), vt, juce::Justification::centred);
        // fader + meter
        auto fd = r.reduced (0, 4); auto meter = fd.removeFromRight (5); auto track = fd.withSizeKeepingCentre (4, fd.getHeight());
        g.setColour (Col::phos.withAlpha (0.12f)); g.fillRoundedRectangle (track, 2);
        const float ln = Editor::toNorm (LEVEL, lv), y = fd.getBottom() - ln * fd.getHeight();
        g.setColour (pc.withAlpha (0.5f)); g.fillRoundedRectangle (track.withTop (y), 2);
        auto cap = juce::Rectangle<float> (fd.getCentreX() - 9, y - 5, 18, 10);
        g.setColour (juce::Colour (0xff1c2a2e)); g.fillRoundedRectangle (cap, 2); g.setColour (pc); g.drawRoundedRectangle (cap, 2, 1.2f); g.drawHorizontalLine ((int) cap.getCentreY(), cap.getX() + 3, cap.getRight() - 3);
        // meter post fader
        const float mdb = juce::Decibels::gainToDecibels (P.padMeter[p].load(), -48.0f), mn = juce::jlimit (0.0f, 1.0f, (mdb + 48.0f) / 51.0f);
        g.setColour (Col::phos.withAlpha (0.08f)); g.fillRect (meter);
        for (int k = 0; k < 20; ++k) { const float t = k / 20.0f; if (t > mn) break; g.setColour (t > 0.9f ? Col::red : t > 0.75f ? Col::amber : Col::green); g.fillRect (meter.getX(), meter.getBottom() - (k + 1) * meter.getHeight() / 20 + 1, meter.getWidth(), meter.getHeight() / 20 - 1.5f); }
        hots.push_back ({ fd, [this, p, fd] (const juce::MouseEvent& e) { ed.selectPad (p); juce::ignoreUnused (e, fd); },
                          [this, p, fd, start = ln] (const juce::MouseEvent& e) { ed.proc.pad[p].v[LEVEL] = Editor::fromNorm (LEVEL, juce::jlimit (0.0f, 1.0f, start - e.getDistanceFromDragStartY() / fd.getHeight())); },
                          [this, p] (float d) { ed.proc.pad[p].v[LEVEL] = Editor::fromNorm (LEVEL, juce::jlimit (0.0f, 1.0f, Editor::toNorm (LEVEL, ed.proc.pad[p].get (LEVEL)) + (d > 0 ? 0.02f : -0.02f))); },
                          [this, p] { ed.proc.pad[p].v[LEVEL] = 1.0f; } });
    }
}

// ---------- SAMPLE ----------
void Screen::pageSample (juce::Graphics& g, juce::Rectangle<float> c)
{
    auto& P = ed.proc; const int p = ed.sel; auto smp = P.getSample (p); const auto pc = padColour (P.pad[p].colour.load()); auto& pd = P.pad[p];
    auto chips = c.removeFromBottom (38); c.removeFromBottom (8);
    auto lay = chopMode == 0 ? c.removeFromBottom (30) : juce::Rectangle<float>(); if (chopMode == 0) c.removeFromBottom (4);
    auto w = c.reduced (6, 10);
    waveR = w;
    {   // testina sopra l'onda (componente a parte, solo lei si muove)
        const bool ph = chopMode == 0 && smp != nullptr;
        if (ed.playhead.isVisible() != ph) ed.playhead.setVisible (ph);
        if (ph && ed.playhead.getBounds() != w.toNearestInt()) ed.playhead.setBounds (w.toNearestInt());
    }
    if (chopMode == 0)
    {   // LAYER: secondo campione sullo stesso pad
        auto l2 = P.getLayer (p); const bool has = l2 != nullptr;
        const auto lc = Col::cyan;
        auto nameR = lay.removeFromLeft (lay.getWidth() * 0.34f); lay.removeFromLeft (5);
        g.setColour (lc.withAlpha (has ? 0.14f : 0.05f)); g.fillRoundedRectangle (nameR, 4); g.setColour (lc.withAlpha (has ? 0.7f : 0.3f)); g.drawRoundedRectangle (nameR, 4, 1.0f);
        if (has)
        {   // mini onda del layer
            auto wr = nameR.reduced (4, 5).withTrimmedRight (22); wr = wr.removeFromRight (wr.getWidth() * 0.38f); const int nn = l2->buf.getNumSamples(), W = juce::jmax (1, (int) wr.getWidth()); const float* d = l2->buf.getReadPointer (0);
            g.setColour (lc.withAlpha (0.45f));
            for (int x = 0; x < W; ++x) { float mx = 0; const int a = (int) ((double) x / W * nn), b = juce::jmin (nn, (int) ((double) (x + 1) / W * nn)); for (int i = a; i < b; i += 4) mx = juce::jmax (mx, std::abs (d[i])); const float h = juce::jmin (1.0f, mx) * wr.getHeight() * 0.5f; g.drawVerticalLine ((int) wr.getX() + x, wr.getCentreY() - h, wr.getCentreY() + h + 1); }
        }
        glowText (g, "LAYER", nameR.withWidth (56).withTrimmedLeft (8), lc, ft (10.0f, true, true));
        glowText (g, has ? l2->name.toUpperCase() : juce::String::fromUTF8 ("NESSUNO  \xc2\xb7  CLIC = CARICA"), nameR.withTrimmedLeft (58).withTrimmedRight (has ? nameR.getWidth() * 0.4f + 10 : 24), has ? Col::phos : Col::phos.withAlpha (0.45f), ft (9.0f, false, true));
        if (has)
        {
            auto x = nameR.removeFromRight (22).reduced (3, 6);
            glowText (g, "X", x, Col::red, ft (10.0f, true, true), juce::Justification::centred);
            hots.push_back ({ x.expanded (2), [this] (const juce::MouseEvent&) { ed.proc.setLayer (ed.sel, nullptr); ed.kitChanged(); }, nullptr, nullptr, nullptr });
            hots.back().tip = "Togli il layer da questo pad.";
        }
        hots.push_back ({ nameR, [this] (const juce::MouseEvent&)
                          {
                              const int pp = ed.sel;
                              ed.chooser = std::make_unique<juce::FileChooser> ("Layer per il pad " + juce::String (pp + 1), juce::File(), ed.proc.formats.getWildcardForAllFormats());
                              ed.chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                                                       [this, pp] (const juce::FileChooser& fc) { const auto f = fc.getResult(); if (f.existsAsFile() && ed.proc.loadLayerFile (f, pp)) { ed.playPad (pp, 100); ed.kitChanged(); } });
                          }, nullptr, nullptr, nullptr });
        hots.back().tip = "LAYER: un secondo campione che suona insieme al primo sullo stesso pad (es. kick + sub, clap + snare). Clic = carica un file; dal BROWSER: clic destro su un suono > Carica come LAYER.";
        const float uw = (lay.getWidth() - 20) / 5;
        auto knobChip = [&] (int id, const char* lab, float sens, float wheelStep)
        {
            auto r = lay.removeFromLeft (uw); lay.removeFromLeft (5);
            const float v = pd.get (id); const bool on = has && std::abs (v - kPInfo[id].def) > 1e-4f;
            chip (g, r, lab, formatPadParam (id, v), on, nullptr, lc);
            hots.push_back ({ r, [] (const juce::MouseEvent&) {}, [this, id, sens, start = v] (const juce::MouseEvent& ev)
                              {
                                  const auto& in = kPInfo[id]; const float s = (in.max - in.min) * sens * (ev.mods.isShiftDown() ? 0.1f : 1.0f);
                                  float nv = juce::jlimit (in.min, in.max, start - ev.getDistanceFromDragStartY() * s);
                                  if (in.kind == 2 || in.kind == 7) nv = std::round (nv);
                                  ed.proc.pad[ed.sel].v[id] = nv;
                              },
                              [this, id, wheelStep] (float d) { const auto& in = kPInfo[id]; auto& vv = ed.proc.pad[ed.sel].v[id]; vv = juce::jlimit (in.min, in.max, vv.load() + (d > 0 ? wheelStep : -wheelStep)); },
                              [this, id] { ed.proc.pad[ed.sel].v[id] = kPInfo[id].def; } });
        };
        knobChip (LGAIN, "LAYER GAIN", 0.004f, 0.5f);   hots.back().tip = "Volume del layer rispetto al campione principale. Doppio clic = 0 dB.";
        knobChip (LTUNE, "LAYER TUNE", 0.004f, 1.0f);   hots.back().tip = "Intonazione del layer in semitoni (si somma al TUNE del pad).";
        knobChip (LPAN,  "LAYER PAN", 0.005f, 0.05f);   hots.back().tip = "Posizione stereo del layer (il PAN del pad sposta tutto).";
        knobChip (LVEL,  "SOGLIA VEL", 0.004f, 1.0f);   hots.back().tip = "OFF = il layer suona sempre. Con un valore: in INSIEME il layer entra solo dai colpi piu forti; in VEL SWITCH sotto la soglia suona il principale, sopra solo il layer.";
        {
            auto r = lay; const bool sw = pd.get (LMODE) > 0.5f;
            chip (g, r, "MODO", formatPadParam (LMODE, pd.get (LMODE)), sw, [this] (const juce::MouseEvent&) { auto& vv = ed.proc.pad[ed.sel].v[LMODE]; vv = vv.load() > 0.5f ? 0.0f : 1.0f; }, lc);
            hots.back().tip = "INSIEME: i due campioni suonano insieme. VEL SWITCH: la SOGLIA VEL decide quale dei due suona.";
        }
    }
    g.setColour (Col::phos.withAlpha (0.12f)); g.drawHorizontalLine ((int) w.getCentreY(), w.getX(), w.getRight());
    if (smp)
    {
        const int n = smp->buf.getNumSamples(); const int Wd = (int) w.getWidth();
        const float s0 = pd.get (START), s1 = pd.get (END);
        // picchi in cache (si ricalcolano solo se cambia il campione)
        if (wfSmp != smp.get() || wfW != Wd)
        {
            wfSmp = smp.get(); wfW = Wd; wfPk.assign ((size_t) Wd, 0.0f);
            const float* d0 = smp->buf.getReadPointer (0); const float* d1 = smp->buf.getReadPointer (juce::jmin (1, smp->buf.getNumChannels() - 1));
            for (int x = 0; x < Wd; ++x)
            {
                const int a2 = (int) ((double) x / Wd * n), e = juce::jmin (n, (int) ((double) (x + 1) / Wd * n) + 1);
                float mx = 0; for (int i = a2; i < e; ++i) mx = juce::jmax (mx, std::abs (d0[i]), std::abs (d1[i]));
                wfPk[(size_t) x] = mx;
            }
        }
        // l'onda si alza e si abbassa con il GAIN, come la clip gain di Ableton
        const float gl = juce::Decibels::decibelsToGain (pd.get (GAIN)), half = w.getHeight() * 0.47f;
        for (int x = 0; x < Wd; ++x)
        {
            const float fx = (float) x / Wd, v = wfPk[(size_t) x] * gl, h = juce::jmin (1.0f, v) * half;
            g.setColour (fx >= s0 && fx <= s1 ? pc.withAlpha (0.85f) : Col::phos.withAlpha (0.15f));
            g.drawVerticalLine ((int) w.getX() + x, w.getCentreY() - h, w.getCentreY() + h + 1);
            if (v > 1.0f && fx >= s0 && fx <= s1) { g.setColour (Col::red); g.drawVerticalLine ((int) w.getX() + x, w.getCentreY() - half - 2, w.getCentreY() - half + 2); g.drawVerticalLine ((int) w.getX() + x, w.getCentreY() + half - 2, w.getCentreY() + half + 2); }
        }
        g.setColour (juce::Colours::black.withAlpha (0.45f)); g.fillRect (w.withWidth (w.getWidth() * s0)); g.fillRect (w.withTrimmedLeft (w.getWidth() * s1));
        // griglia in battute se sincronizzato
        if (pd.get (SBPM) > 40)
        {
            const double beats = n / smp->sr * pd.get (SBPM) / 60.0;
            for (int k = 1; k < beats; ++k) { const float x = w.getX() + w.getWidth() * (float) (k / beats); g.setColour (Col::amber.withAlpha (k % 4 == 0 ? 0.4f : 0.12f)); g.drawVerticalLine ((int) x, w.getY(), w.getBottom()); }
        }
        for (int k = 0; k < 2; ++k)
        {
            const float x = w.getX() + (k ? s1 : s0) * w.getWidth(); const bool act = sDrag == k + 1;
            g.setColour (act ? Col::amber : Col::phos); g.drawLine (x, w.getY() - 4, x, w.getBottom() + 4, act ? 2.2f : 1.5f);
            juce::Path t; t.addTriangle (x - 7, w.getY() - 7, x + 7, w.getY() - 7, x, w.getY() + 4); g.fillPath (t);
            glowText (g, k ? "END" : "START", juce::Rectangle<float> (k ? x - 44 : x + 4, w.getBottom() - 14, 40, 12), Col::phos, ft (9.5f, true, true), k ? juce::Justification::centredRight : juce::Justification::centredLeft);
        }
        // inviluppo
        const float atk = pd.get (ATTACK), dec = pd.get (DECAY), len = (float) (n / smp->sr * 1000.0) * (s1 - s0);
        juce::Path env; const float x0 = w.getX() + s0 * w.getWidth(), xw = (s1 - s0) * w.getWidth();
        env.startNewSubPath (x0, w.getBottom());
        const float ax = x0 + xw * juce::jmin (1.0f, atk / juce::jmax (1.0f, len)); env.lineTo (ax, w.getY() + 4);
        for (int i = 1; i <= 40; ++i) { const float t = i / 40.0f, ms = (len - atk) * t; const float lv = dec >= 3990 ? 1.0f : std::exp (-ms / (dec / 4.6f)); env.lineTo (ax + (x0 + xw - ax) * t, w.getBottom() - (w.getHeight() - 4) * lv); }
        g.setColour (Col::amber.withAlpha (0.18f)); g.strokePath (env, juce::PathStrokeType (5.0f)); g.setColour (Col::amber.withAlpha (0.85f)); g.strokePath (env, juce::PathStrokeType (1.4f));
        glowText (g, juce::String (n / smp->sr, 2) + " s  " + juce::String (smp->sr / 1000.0, 1) + " kHz", w.withHeight (12), Col::phos.withAlpha (0.6f), ft (9.5f, false, true), juce::Justification::centredRight);
        if (chopMode == 0)
        {
            glowText (g, "GAIN " + formatPadParam (GAIN, pd.get (GAIN)), juce::Rectangle<float> (w.getX() + 8, w.getY() + 2, 160, 14), sDrag == 3 ? Col::amber : Col::phos.withAlpha (0.55f), ft (10.0f, true, true));
            glowText (g, juce::String::fromUTF8 ("trascina su/giu sull'onda = volume  \xc2\xb7  trascina i marker = inizio/fine  \xc2\xb7  doppio clic = 0 dB"),
                      juce::Rectangle<float> (w.getX() + 8, w.getBottom() + 1, w.getWidth() - 16, 10), Col::phos.withAlpha (0.35f), ft (8.0f, false, true));
            auto down = [this, w] (const juce::MouseEvent& e)
            {
                auto& PP = ed.proc.pad[ed.sel]; const float xs = w.getX() + PP.get (START) * w.getWidth(), xe = w.getX() + PP.get (END) * w.getWidth();
                const float ds = std::abs (e.position.x - xs), de = std::abs (e.position.x - xe);
                if (e.mods.isRightButtonDown()) { sDrag = 2; PP.v[END] = juce::jmax (juce::jlimit (0.0f, 1.0f, (e.position.x - w.getX()) / w.getWidth()), PP.get (START) + 0.002f); return; }
                if (juce::jmin (ds, de) < 10 || e.position.y < w.getY() + 4) sDrag = ds <= de ? 1 : 2;
                else { sDrag = 3; sGain0 = PP.get (GAIN); }
            };
            auto dragFn = [this, w] (const juce::MouseEvent& e)
            {
                auto& PP = ed.proc.pad[ed.sel]; const float fx = juce::jlimit (0.0f, 1.0f, (e.position.x - w.getX()) / w.getWidth());
                if (sDrag == 1) PP.v[START] = juce::jmin (fx, PP.get (END) - 0.002f);
                else if (sDrag == 2) PP.v[END] = juce::jmax (fx, PP.get (START) + 0.002f);
                else if (sDrag == 3)
                {
                    const float dy = (float) -e.getDistanceFromDragStartY() * (e.mods.isShiftDown() ? 0.02f : 0.12f);
                    PP.v[GAIN] = juce::jlimit (-24.0f, 24.0f, std::round ((sGain0 + dy) * 10.0f) / 10.0f);
                }
            };
            hots.push_back ({ w.expanded (6, 10), down, dragFn, [this] (float d) { auto& v = ed.proc.pad[ed.sel].v[GAIN]; v = juce::jlimit (-24.0f, 24.0f, v.load() + (d > 0 ? 0.5f : -0.5f)); },
                              [this] { auto& PP = ed.proc.pad[ed.sel]; PP.v[GAIN] = 0.0f; } });
        }
        else
        {
            if (chopSmp != smp.get()) buildChop();
            // griglia o transienti di riferimento
            if (chopMode == 2) for (float o : onsets) { g.setColour (Col::amber.withAlpha (0.25f)); g.drawVerticalLine ((int) (w.getX() + o * w.getWidth()), w.getBottom() - 10, w.getBottom()); }
            else for (float l : gridLines()) { g.setColour (Col::magenta.withAlpha (0.15f)); g.drawVerticalLine ((int) (w.getX() + l * w.getWidth()), w.getY(), w.getBottom()); }
            std::vector<float> pts { s0 }; for (float m : marks) pts.push_back (m); pts.push_back (s1);
            for (size_t k = 0; k + 1 < pts.size(); ++k)
            {
                const float xa = w.getX() + pts[k] * w.getWidth(), xb = w.getX() + pts[k + 1] * w.getWidth();
                g.setColour ((k % 2 ? Col::magenta : Col::cyan).withAlpha (0.08f)); g.fillRect (juce::Rectangle<float> (xa, w.getY(), xb - xa, w.getHeight()));
                const int target = ed.sel + (int) k;
                glowText (g, juce::String ((int) k + 1) + (target < kPads ? " > P" + juce::String (target + 1) : juce::String (" X")), juce::Rectangle<float> (xa + 3, w.getY() + 2, juce::jmax (30.0f, xb - xa - 4), 12),
                          target < kPads ? Col::phos : Col::red, ft (8.5f, true, true));
            }
            for (size_t k = 0; k < marks.size(); ++k)
            {
                const float x = w.getX() + marks[k] * w.getWidth();
                g.setColour ((int) k == dragMark ? Col::amber : Col::magenta); g.drawLine (x, w.getY(), x, w.getBottom(), (int) k == dragMark ? 2.4f : 1.6f);
                juce::Path t; t.addTriangle (x - 6, w.getBottom() + 7, x + 6, w.getBottom() + 7, x, w.getBottom() - 3); g.fillPath (t);
            }
            auto toFx = [w] (const juce::MouseEvent& e) { return juce::jlimit (0.0f, 1.0f, (e.position.x - w.getX()) / w.getWidth()); };
            hots.push_back ({ w.expanded (6, 10),
                [this, w, toFx] (const juce::MouseEvent& e)
                {
                    dragMark = -1; const float fx = toFx (e); lastFx = fx; float best = 11.0f;
                    for (size_t k = 0; k < marks.size(); ++k) { const float dpx = std::abs (marks[k] - fx) * w.getWidth(); if (dpx < best) { best = dpx; dragMark = (int) k; } }
                    if (dragMark >= 0 && e.mods.isPopupMenu()) { marks.erase (marks.begin() + dragMark); dragMark = -1; }
                },
                [this, toFx] (const juce::MouseEvent& e)
                {
                    if (dragMark < 0 || dragMark >= (int) marks.size()) return;
                    auto& PP = ed.proc.pad[ed.sel]; const size_t k = (size_t) dragMark;
                    const float lo = k > 0 ? marks[k - 1] + 0.002f : PP.get (START) + 0.002f, hi = k + 1 < marks.size() ? marks[k + 1] - 0.002f : PP.get (END) - 0.002f;
                    const float fx = e.mods.isShiftDown() ? toFx (e) : snapPos (toFx (e));
                    if (hi > lo) marks[k] = juce::jlimit (lo, hi, fx);
                }, nullptr, nullptr });
            hots.back().dbl = [this]
            {
                auto& PP = ed.proc.pad[ed.sel]; const float fx = snapPos (lastFx);
                if (fx > PP.get (START) + 0.002f && fx < PP.get (END) - 0.002f && marks.size() < 15) { marks.push_back (fx); std::sort (marks.begin(), marks.end()); }
            };
        }
    }
    else glowText (g, "TRASCINA UN CAMPIONE QUI O SUL PAD", w, Col::phos.withAlpha (0.6f), ft (13.0f, true, true), juce::Justification::centred);
    // campi
    if (chopMode > 0 && smp)
    {
        const float cw2 = (chips.getWidth() - 20) / 5;
        static const char* modes[] = { "", "GRIGLIA BPM", "TRANSIENTI", "UGUALI" };
        static const char* divG[] = { "1/4", "1/8", "1/16" }; static const char* divE[] = { "4", "8", "16" };
        chip (g, chips.removeFromLeft (cw2), "CHOP", modes[chopMode], true, [this] (const juce::MouseEvent& e) { chopMode = chopMode % 3 + 1; juce::ignoreUnused (e); buildChop(); }, Col::magenta); chips.removeFromLeft (5);
        chip (g, chips.removeFromLeft (cw2), chopMode == 2 ? "SENSIBILITA" : "DIVISIONE", chopMode == 3 ? divE[chopDiv] : chopMode == 2 ? juce::String (divE[chopDiv]) + " MAX" : divG[chopDiv], true,
              [this] (const juce::MouseEvent&) { chopDiv = (chopDiv + 1) % 3; buildChop(); }, Col::magenta); chips.removeFromLeft (5);
        chip (g, chips.removeFromLeft (cw2), "SNAP", chopSnap ? "ON" : "OFF", chopSnap, [this] (const juce::MouseEvent&) { chopSnap = ! chopSnap; }, Col::amber); chips.removeFromLeft (5);
        chip (g, chips.removeFromLeft (cw2), "", juce::String::fromUTF8 ("SPARGI SUI PAD \xe2\x96\xb6"), true, [this] (const juce::MouseEvent&)
        {
            auto& PP = ed.proc.pad[ed.sel]; std::vector<float> pts { PP.get (START) }; for (float m : marks) pts.push_back (m); pts.push_back (PP.get (END));
            ed.proc.chopToPads (ed.sel, pts); chopMode = 0; ed.kitChanged();
        }, Col::green); chips.removeFromLeft (5);
        chip (g, chips, "", "ESCI", false, [this] (const juce::MouseEvent&) { chopMode = 0; }, Col::phos);
        glowText (g, juce::String::fromUTF8 ("TRASCINA I MARKER  \xc2\xb7  DOPPIO CLIC AGGIUNGE  \xc2\xb7  CLIC DESTRO TOGLIE"), juce::Rectangle<float> (c.getX() + 6, c.getY() - 2, 400, 12), Col::phos.withAlpha (0.45f), ft (8.5f, false, true));
        return;
    }
    const float u = (chips.getWidth() - 35) / 8.6f;
    auto cycle = [this] (int i) { return [this, i] (const juce::MouseEvent& e) { auto& PP = ed.proc.pad[ed.sel]; const int n = choiceCount (i);
                                                                               PP.v[i] = (float) ((juce::roundToInt (PP.get (i)) + (e.mods.isPopupMenu() ? n - 1 : 1)) % n); }; };
    chip (g, chips.removeFromLeft (u * 1.0f), "MODE", formatPadParam (MODE, pd.get (MODE)), pd.get (MODE) > 0.5f, cycle (MODE), Col::amber); chips.removeFromLeft (5);
    hots.back().tip = "ONE SHOT: il colpo suona tutto anche se lasci subito il tasto. GATE: suona solo finche tieni premuta la nota (in registrazione conta la lunghezza della nota MIDI).";
    chip (g, chips.removeFromLeft (u * 0.8f), "REVERSE", formatPadParam (REVERSE, pd.get (REVERSE)), pd.get (REVERSE) > 0.5f, cycle (REVERSE), Col::amber); chips.removeFromLeft (5);
    hots.back().tip = "Suona il campione al contrario.";
    chip (g, chips.removeFromLeft (u * 0.8f), "CHOKE", formatPadParam (CHOKE, pd.get (CHOKE)), pd.get (CHOKE) > 0.5f, cycle (CHOKE), Col::amber); chips.removeFromLeft (5);
    hots.back().tip = "CHOKE: i pad con lo stesso numero si zittiscono a vicenda. Esempio: hat chiuso e aperto su 1, quando suona il chiuso l'aperto si ferma, come su una batteria vera.";
    chip (g, chips.removeFromLeft (u * 0.8f), "LOOP", formatPadParam (LOOP, pd.get (LOOP)), pd.get (LOOP) > 0.5f, cycle (LOOP), Col::amber); chips.removeFromLeft (5);
    hots.back().tip = "LOOP: ripete tra START ed END finche la nota e tenuta. Funziona anche in registrazione: dura quanto la nota MIDI (usalo con MODE su GATE).";
    chip (g, chips.removeFromLeft (u * 1.0f), "SYNC BPM", formatPadParam (WARP, pd.get (WARP)), pd.get (WARP) > 0.5f, cycle (WARP), Col::magenta); chips.removeFromLeft (5);
    hots.back().tip = "SYNC BPM: REPITCH porta il campione al tempo di Ableton cambiando anche l'intonazione (come un giradischi); STRETCH lo porta a tempo mantenendo l'intonazione.";
    // BPM del campione: trascina, rotella, doppio clic = rileva; mezzo e doppio
    auto bpmR = chips.removeFromLeft (u * 1.6f); chips.removeFromLeft (5);
    juce::String bv = formatPadParam (SBPM, pd.get (SBPM));
    if (pd.get (SBPM) > 40 && smp && pd.get (WARP) > 0.5f) bv << juce::String::fromUTF8 (" \xe2\x86\x92 ") << juce::String (ed.proc.hostBpm(), 0);
    chip (g, bpmR.withTrimmedRight (44), "BPM CAMPIONE", bv, pd.get (WARP) > 0.5f, nullptr, Col::magenta);
    hots.push_back ({ bpmR.withTrimmedRight (44), [] (const juce::MouseEvent&) {}, [this, start = pd.get (SBPM)] (const juce::MouseEvent& e) mutable
                      { ed.proc.pad[ed.sel].v[SBPM] = juce::jlimit (0.0f, 240.0f, std::round ((juce::jmax (start, 60.0f) - e.getDistanceFromDragStartY() * (e.mods.isShiftDown() ? 0.05f : 0.5f)) * 10) / 10); },
                      [this] (float d) { auto& v = ed.proc.pad[ed.sel].v[SBPM]; v = juce::jlimit (0.0f, 240.0f, juce::jmax (v.load(), 59.0f) + (d > 0 ? 1.0f : -1.0f)); },
                      [this] { ed.proc.detectBpm (ed.sel); } });
    {
        auto half = bpmR.removeFromRight (42); auto top = half.removeFromTop (half.getHeight() / 2).reduced (1, 1), bot = half.reduced (1, 1);
        for (int k = 0; k < 2; ++k)
        {
            auto r = k ? bot : top; g.setColour (Col::magenta.withAlpha (0.12f)); g.fillRoundedRectangle (r, 3); g.setColour (Col::magenta.withAlpha (0.6f)); g.drawRoundedRectangle (r, 3, 1.0f);
            glowText (g, k ? juce::String::fromUTF8 ("\xc3\xb7" "2") : juce::String::fromUTF8 ("\xc3\x97" "2"), r, Col::magenta, ft (9.5f, true, true), juce::Justification::centred);
            hots.push_back ({ r, [this, k] (const juce::MouseEvent&) { auto& v = ed.proc.pad[ed.sel].v[SBPM]; if (v.load() > 40) v = juce::jlimit (0.0f, 240.0f, k ? v.load() * 0.5f : v.load() * 2.0f); }, nullptr, nullptr, nullptr });
        }
    }
    // tonalita
    {
        int k = juce::roundToInt (pd.get (SKEY));
        if (k == -1 && smp) { ed.proc.detectKey (p); k = juce::roundToInt (pd.get (SKEY)); }
        const int tu = juce::roundToInt (pd.get (TUNE)) % 12;
        juce::String kv = k >= 0 ? keyName (k) : juce::String ("--");
        if (k >= 0 && tu != 0) { const int to = (k < 12 ? 0 : k < 24 ? 12 : 24) + (((k % 12) + tu) % 12 + 12) % 12; kv << juce::String::fromUTF8 (" \xe2\x86\x92 ") << keyName (to); }
        chip (g, chips.removeFromLeft (u * 1.4f), juce::String::fromUTF8 ("TONALIT\xc3\x80 \xe2\x96\xbe"), kv, k >= 0 && tu != 0, [this] (const juce::MouseEvent&) { keyMenu(); }, Col::green); chips.removeFromLeft (5);
    }
    if (smp) chip (g, chips, "", juce::String::fromUTF8 ("CHOP \xe2\x96\xb6"), true, [this] (const juce::MouseEvent&) { ed.openChop (ed.sel); }, Col::green);
}

void Screen::keyMenu()
{
    auto& P = ed.proc; const int p = ed.sel; if (! P.getSample (p)) return;
    const int k = juce::roundToInt (P.pad[p].get (SKEY)), tune = juce::roundToInt (P.pad[p].get (TUNE));
    juce::PopupMenu m, to, is;
    m.addSectionHeader (k >= 0 ? (k >= 24 ? juce::String ("NOTA DEL CAMPIONE: ") : juce::String::fromUTF8 ("TONALIT\xc3\x80 DEL CAMPIONE: ")) + keyName (k)
                               : juce::String::fromUTF8 ("TONALIT\xc3\x80 SCONOSCIUTA: SCEGLILA SOTTO"));
    for (int t = 0; t < 24; ++t)
    {
        const int idx = (t % 2) * 12 + t / 2;   // C, Cm, C#, C#m ...
        const int sh = keyShift (k, idx);
        to.addItem (100 + idx, keyName (idx) + (k >= 0 ? "   (" + juce::String (sh > 0 ? "+" : "") + juce::String (sh) + " st)" : juce::String()), k >= 0, k >= 0 && sh == ((tune % 12 + 18) % 12 - 6));
    }
    m.addSubMenu (juce::String::fromUTF8 ("Porta in\xe2\x80\xa6"), to, k >= 0);
    m.addItem (1, "Torna all'originale", tune != 0);
    m.addSeparator();
    for (int t = 0; t < 24; ++t) { const int idx = (t % 2) * 12 + t / 2; is.addItem (200 + idx, keyName (idx), true, idx == k); }
    juce::PopupMenu notes; for (int t = 0; t < 12; ++t) notes.addItem (300 + t, "nota " + keyName (24 + t), true, k == 24 + t);
    is.addSubMenu ("One shot (nota singola)", notes);
    m.addSubMenu (juce::String::fromUTF8 ("Il campione \xc3\xa8 in\xe2\x80\xa6"), is);
    m.addItem (2, "Rileva di nuovo (nome e ascolto)");
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [this, p] (int r)
    {
        auto& PP = ed.proc;
        if (r >= 100 && r < 124) PP.transposeTo (p, r - 100);
        else if (r == 1) PP.pad[p].v[TUNE] = 0.0f;
        else if (r >= 200 && r < 224) PP.pad[p].v[SKEY] = (float) (r - 200);
        else if (r >= 300 && r < 312) PP.pad[p].v[SKEY] = (float) (24 + r - 300);
        else if (r == 2) PP.detectKey (p);
        repaint(); ed.root.repaint();
    });
}

std::vector<float> Screen::gridLines() const
{
    std::vector<float> out; auto s = ed.proc.getSample (ed.sel); if (! s) return out;
    auto& PP = ed.proc.pad[ed.sel]; const float s0 = PP.get (START), s1 = PP.get (END);
    const double dur = s->buf.getNumSamples() / s->sr; float bpm = PP.get (SBPM);
    if (bpm < 40) { bool c = false; bpm = Processor::estimateBpm (s->name, dur * (s1 - s0), c); }
    const double beat = bpm >= 40 ? 60.0 / bpm : dur * (s1 - s0) / 4.0;
    static const double div[] = { 1.0, 0.5, 0.25 };
    const double step = (chopMode == 3) ? (s1 - s0) / (chopDiv == 0 ? 4 : chopDiv == 1 ? 8 : 16) : beat * div[chopDiv] / dur;
    if (step <= 0) return out;
    for (double x = s0 + step; x < s1 - step * 0.25; x += step) out.push_back ((float) x);
    return out;
}

float Screen::snapPos (float fx) const
{
    if (! chopSnap) return fx;
    const auto& ref = chopMode == 2 ? onsets : gridLines();
    float best = fx, bd = 1e9f;
    for (float r : ref) if (std::abs (r - fx) < bd) { bd = std::abs (r - fx); best = r; }
    return bd < 0.05f ? best : fx;
}

void Screen::buildChop()
{
    auto s = ed.proc.getSample (ed.sel); chopSmp = s.get(); marks.clear(); onsets.clear(); dragMark = -1;
    if (! s) return;
    // transienti: flusso di energia
    const int n = s->buf.getNumSamples(), hop = 256; const float* d = s->buf.getReadPointer (0);
    std::vector<float> en; for (int i = 0; i + hop < n; i += hop) { float e = 0; for (int k = 0; k < hop; ++k) e += d[i + k] * d[i + k]; en.push_back (std::sqrt (e / hop)); }
    std::vector<float> fl (en.size(), 0); double mean = 0;
    for (size_t i = 1; i < en.size(); ++i) { fl[i] = juce::jmax (0.0f, en[i] - en[i - 1]); mean += fl[i]; }
    mean /= juce::jmax ((size_t) 1, en.size()); double var = 0; for (float f : fl) var += (f - mean) * (f - mean); const double sd = std::sqrt (var / juce::jmax ((size_t) 1, fl.size()));
    const int gap = (int) (0.06 * s->sr / hop); int last = -gap;
    for (size_t i = 1; i < fl.size(); ++i) if (fl[i] > mean + 1.4 * sd && (int) i - last >= gap) { onsets.push_back ((float) (i * hop) / n); last = (int) i; }
    auto& PP = ed.proc.pad[ed.sel]; const float s0 = PP.get (START), s1 = PP.get (END);
    const int maxSl = chopDiv == 0 ? 4 : chopDiv == 1 ? 8 : 16;
    if (chopMode == 2) { for (float o : onsets) if (o > s0 + 0.002f && o < s1 - 0.002f && (int) marks.size() < maxSl - 1) marks.push_back (o); }
    else { for (float l : gridLines()) if ((int) marks.size() < 15) marks.push_back (l); }
}

// ---------- VELOCITY ----------
void Screen::pageVel (juce::Graphics& g, juce::Rectangle<float> c)
{
    auto& P = ed.proc; auto& pd = P.pad[ed.sel]; const auto pc = padColour (pd.colour.load());
    auto side = c.removeFromRight (200); c.removeFromRight (10);
    auto gr = c.reduced (24, 10).withTrimmedBottom (8);
    for (int k = 0; k <= 4; ++k) { const float y = gr.getY() + gr.getHeight() * k / 4; g.setColour (Col::phos.withAlpha (0.08f)); g.drawHorizontalLine ((int) y, gr.getX(), gr.getRight());
        g.setFont (ft (8.5f, false, true)); g.setColour (Col::phos.withAlpha (0.4f)); g.drawText (juce::String (-k * 9) + "dB", juce::Rectangle<float> (gr.getX() - 30, y - 6, 28, 12), juce::Justification::centredRight); }
    for (int v : { 1, 32, 64, 96, 127 }) { const float x = gr.getX() + gr.getWidth() * v / 127.0f; g.setColour (Col::phos.withAlpha (0.08f)); g.drawVerticalLine ((int) x, gr.getY(), gr.getBottom());
        g.setFont (ft (8.5f, false, true)); g.setColour (Col::phos.withAlpha (0.4f)); g.drawText (juce::String (v), juce::Rectangle<float> (x - 15, gr.getBottom() + 1, 30, 10), juce::Justification::centred); }
    juce::Path vol, filt;
    for (int v = 1; v <= 127; ++v)
    {
        const float vv = ed.fullLevel ? 1.0f : P.velCurve (v), db = -(1 - vv) * 30 * pd.get (VLEVEL), x = gr.getX() + gr.getWidth() * v / 127.0f, y = gr.getY() + juce::jlimit (0.0f, 1.0f, -db / 36.0f) * gr.getHeight();
        const float fo = -(1 - vv) * 5 * pd.get (VFILTER), yf = gr.getY() + juce::jlimit (0.0f, 1.0f, -fo / 6.0f) * gr.getHeight();
        if (v == 1) { vol.startNewSubPath (x, y); filt.startNewSubPath (x, yf); } else { vol.lineTo (x, y); filt.lineTo (x, yf); }
    }
    g.setColour (pc.withAlpha (0.25f)); g.strokePath (vol, juce::PathStrokeType (6.0f)); g.setColour (pc); g.strokePath (vol, juce::PathStrokeType (2.0f));
    if (pd.get (VFILTER) > 0.01f) { g.setColour (Col::amber.withAlpha (0.8f)); g.strokePath (filt, juce::PathStrokeType (1.4f, juce::PathStrokeType::curved), juce::AffineTransform()); }
    const int lv = P.lastVel[ed.sel].load();
    if (lv > 0) { const float x = gr.getX() + gr.getWidth() * lv / 127.0f; g.setColour (Col::magenta); g.drawVerticalLine ((int) x, gr.getY(), gr.getBottom());
        glowText (g, "ULTIMO " + juce::String (lv), juce::Rectangle<float> (x + 4, gr.getY(), 80, 12), Col::magenta, ft (9.0f, true, true)); }
    glowText (g, "VOLUME", juce::Rectangle<float> (gr.getX() + 6, gr.getBottom() - 30, 80, 12), pc, ft (9.0f, true, true));
    if (pd.get (VFILTER) > 0.01f) glowText (g, "FILTRO", juce::Rectangle<float> (gr.getX() + 6, gr.getBottom() - 16, 80, 12), Col::amber, ft (9.0f, true, true));
    // curve
    static const char* curves[] = { "LINEARE", "MORBIDA", "DURA", "S", "FISSA 127" };
    auto* cp = dynamic_cast<juce::AudioParameterChoice*> (P.apvts.getParameter ("curve"));
    const int cur = cp->getIndex();
    g.setFont (ft (8.5f, true, true)); g.setColour (Col::phos.withAlpha (0.55f)); g.drawText ("CURVA (TUTTI I PAD)", side.removeFromTop (14), juce::Justification::centredLeft);
    for (int i = 0; i < 5; ++i)
    {
        chip (g, side.removeFromTop (24), "", curves[i], i == cur, [cp, i] (const juce::MouseEvent&) { cp->setValueNotifyingHost (cp->convertTo0to1 ((float) i)); }, Col::magenta);
        side.removeFromTop (4);
    }
    side.removeFromTop (4);
    chip (g, side.removeFromTop (28), "", ed.fullLevel ? "FULL LEVEL ON" : "FULL LEVEL OFF", ed.fullLevel, [this] (const juce::MouseEvent&) { ed.fullLevel = ! ed.fullLevel; ed.root.repaint(); }, Col::amber);
}

// ---------- EQ ----------
static double bandDb (const Pad& pd, int b, double f, double sr)
{
    if (pd.get (bandP (b, 0)) < 0.5f) return 0;
    const int t = juce::roundToInt (pd.get (bandP (b, 1))); const float fr = pd.get (bandP (b, 2)), gd = pd.get (bandP (b, 3)), q = pd.get (bandP (b, 4));
    const bool cut = t == EQ_LC12 || t == EQ_LC24 || t == EQ_HC12 || t == EQ_HC24;
    const int rt = t == EQ_LSHELF ? 0 : t == EQ_BELL ? 1 : t == EQ_HSHELF ? 2 : t == EQ_NOTCH ? 5 : (t == EQ_LC12 || t == EQ_LC24) ? 4 : 3;
    double d = Biquad::magDb (rt, sr, fr, gd, cut ? q : (t == EQ_NOTCH ? juce::jmax (q, 0.5f) * 2 : q), f);
    if (t == EQ_LC24 || t == EQ_HC24) d += Biquad::magDb (rt, sr, fr, gd, 0.54, f);
    return d;
}
static const juce::Colour kBandCol[6] = { juce::Colour (0xffff5c8a), juce::Colour (0xffffb000), juce::Colour (0xff7dff5a), juce::Colour (0xff19e3ff), juce::Colour (0xffa66bff), juce::Colour (0xffff7a1a) };
static const char* kTypeShort[kEqTypes] = { "LC 12", "LC 24", "LOW SH", "BELL", "NOTCH", "HI SH", "HC 12", "HC 24" };

// ---------- EQ + FILTRO: 6 bande con curve a scelta, solo per banda, filtro della voce con inviluppo ----------
void Screen::pageEq (juce::Graphics& g, juce::Rectangle<float> c)
{
    auto& P = ed.proc; auto& pd = P.pad[ed.sel]; const bool on = pd.get (EQON) > 0.5f; const int sb = eqBand;
    auto top = c.removeFromTop (26); c.removeFromTop (4);
    chip (g, top.removeFromLeft (70), "", on ? "EQ ON" : "EQ OFF", on, [this] (const juce::MouseEvent&) { auto& v = ed.proc.pad[ed.sel].v[EQON]; v = v.load() > 0.5f ? 0.0f : 1.0f; }, Col::green);
    hots.back().tip = "Accende o spegne tutto l'EQ del pad.";
    top.removeFromLeft (6);
    // le 6 bande: clic = scegli, S = ascolta solo quella banda
    for (int b = 0; b < 6; ++b)
    {
        auto r = top.removeFromLeft (78); top.removeFromLeft (4);
        const bool bo = pd.get (bandP (b, 0)) > 0.5f, sel = b == sb, so = P.eqSoloPad.load() == ed.sel && P.eqSoloBand.load() == b;
        auto sr2 = r.removeFromRight (20).reduced (2, 3);
        chip (g, r, "", juce::String (b + 1) + " " + kTypeShort[juce::roundToInt (pd.get (bandP (b, 1)))], sel, [this, b] (const juce::MouseEvent&) { eqBand = b; ed.bindQlinks(); }, bo ? kBandCol[b] : Col::phos);
        hots.back().tip = "Banda " + juce::String (b + 1) + ": clic per sceglierla, poi la regoli con le manopole Q-link o trascinando il punto.";
        g.setColour (so ? Col::amber : Col::phos.withAlpha (0.08f)); g.fillRoundedRectangle (sr2, 3);
        g.setColour (so ? juce::Colours::black : Col::phos.withAlpha (0.7f)); g.setFont (ft (9.0f, true, true)); g.drawText ("S", sr2, juce::Justification::centred);
        hots.push_back ({ sr2, [this, b] (const juce::MouseEvent&)
                          { auto& PP = ed.proc; const bool was = PP.eqSoloPad.load() == ed.sel && PP.eqSoloBand.load() == b;
                            PP.eqSoloBand = was ? -1 : b; PP.eqSoloPad = was ? -1 : ed.sel; if (! was) { PP.pad[ed.sel].v[EQON] = 1; PP.pad[ed.sel].v[bandP (b, 0)] = 1; } eqBand = b; ed.bindQlinks(); },
                          nullptr, nullptr, nullptr, "SOLO: senti solo la zona di frequenze di questa banda (gli altri pad tacciono). Clicca di nuovo per tornare." });
    }
    // tipo della banda scelta + filtro della voce
    auto bot = c.removeFromBottom (24); c.removeFromBottom (4);
    {
        const bool bo = pd.get (bandP (sb, 0)) > 0.5f;
        chip (g, bot.removeFromLeft (52), "", bo ? "ON" : "OFF", bo, [this, sb] (const juce::MouseEvent&) { auto& v = ed.proc.pad[ed.sel].v[bandP (sb, 0)]; v = v.load() > 0.5f ? 0.0f : 1.0f; ed.proc.pad[ed.sel].v[EQON] = 1; }, kBandCol[sb]);
        bot.removeFromLeft (4);
        static const char* tips[kEqTypes] = { "Taglia i bassi sotto la frequenza (12 dB/ottava). Q = risonanza.", "Taglia i bassi sotto la frequenza, ripido (24 dB/ottava).",
                                              "Alza o abbassa tutto sotto la frequenza.", "Campana: alza o abbassa intorno alla frequenza, Q = larghezza.", "Toglie una frequenza stretta (fischi, risonanze).",
                                              "Alza o abbassa tutto sopra la frequenza.", "Taglia gli alti sopra la frequenza (12 dB/ottava). Q = risonanza.", "Taglia gli alti sopra la frequenza, ripido (24 dB/ottava)." };
        const int ct = juce::roundToInt (pd.get (bandP (sb, 1)));
        for (int t = 0; t < kEqTypes; ++t)
        {
            chip (g, bot.removeFromLeft (54), "", kTypeShort[t], ct == t, [this, sb, t] (const juce::MouseEvent&) { auto& PP = ed.proc.pad[ed.sel]; PP.v[bandP (sb, 1)] = (float) t; PP.v[bandP (sb, 0)] = 1; PP.v[EQON] = 1; }, kBandCol[sb]);
            hots.back().tip = tips[t]; bot.removeFromLeft (3);
        }
        bot.removeFromLeft (8);
        const int ft0 = juce::roundToInt (pd.get (FTYPE)); const char* fts[] = { "LP", "HP", "BP" };
        g.setFont (ft (8.5f, true, true)); g.setColour (Col::amber.withAlpha (0.8f)); g.drawText ("FILTRO", bot.removeFromLeft (42), juce::Justification::centredLeft);
        for (int i = 0; i < 3; ++i)
        {
            chip (g, bot.removeFromLeft (30), "", fts[i], ft0 == i, [this, i] (const juce::MouseEvent&) { ed.proc.pad[ed.sel].v[FTYPE] = (float) i; }, Col::amber); bot.removeFromLeft (2);
            hots.back().tip = "Filtro della voce (punto F, ambra): passa basso, passa alto, passa banda. Ha l'inviluppo (ENV) e segue la velocity. Si automatizza in Ableton.";
        }
    }
    auto r = c.reduced (4, 2); const float range = 18;
    freqGrid (g, r, range);
    const double sr = 48000;
    // curva dell'EQ
    juce::Path curve, fill, fcurve;
    const float fc = pd.get (CUTOFF), k = 2.0f - 1.94f * pd.get (RESO); const int ft0 = juce::roundToInt (pd.get (FTYPE));
    const bool fOn = ! (ft0 == 0 && fc >= 19500);
    for (int x = 0; x <= (int) r.getWidth(); x += 2)
    {
        const float f = xToFreq (r.getX() + x, r);
        double db = 0; if (on) for (int b = 0; b < 6; ++b) db += bandDb (pd, b, f, sr);
        const float y = r.getCentreY() - juce::jlimit (-range, range, (float) db) / range * r.getHeight() * 0.5f;
        if (x == 0) { curve.startNewSubPath (r.getX() + x, y); fill.startNewSubPath (r.getX() + x, r.getCentreY()); fill.lineTo (r.getX() + x, y); }
        else { curve.lineTo (r.getX() + x, y); fill.lineTo (r.getX() + x, y); }
        if (fOn)
        {
            const float w = f / fc, w2 = w * w, den = std::sqrt ((1 - w2) * (1 - w2) + (k * w) * (k * w));
            const float mag = ft0 == 0 ? 1 / den : ft0 == 1 ? w2 / den : (k * w) / den;
            const float yf = r.getCentreY() - juce::jlimit (-range, range, 20 * std::log10 (mag + 1e-6f)) / range * r.getHeight() * 0.5f;
            if (x == 0) fcurve.startNewSubPath (r.getX() + x, yf); else fcurve.lineTo (r.getX() + x, yf);
        }
    }
    fill.lineTo (r.getRight(), r.getCentreY()); fill.closeSubPath();
    g.setColour (Col::green.withAlpha (on ? 0.14f : 0.04f)); g.fillPath (fill);
    g.setColour (Col::green.withAlpha (on ? 0.3f : 0.1f)); g.strokePath (curve, juce::PathStrokeType (5.0f)); g.setColour (on ? Col::green : Col::green.withAlpha (0.3f)); g.strokePath (curve, juce::PathStrokeType (1.8f));
    if (fOn) { juce::Path dash; float dl[] = { 5, 4 }; juce::PathStrokeType (1.4f).createDashedStroke (dash, fcurve, dl, 2); g.setColour (Col::amber.withAlpha (0.85f)); g.fillPath (dash); }
    // punti delle bande
    for (int b = 0; b < 6; ++b)
    {
        const int t = juce::roundToInt (pd.get (bandP (b, 1))); const bool cut = t == EQ_LC12 || t == EQ_LC24 || t == EQ_HC12 || t == EQ_HC24 || t == EQ_NOTCH;
        const bool bo = pd.get (bandP (b, 0)) > 0.5f;
        const float x = freqToX (pd.get (bandP (b, 2)), r), y = cut ? r.getCentreY() : r.getCentreY() - pd.get (bandP (b, 3)) / range * r.getHeight() * 0.5f;
        const auto bc = kBandCol[b];
        if (b == sb) { g.setColour (bc.withAlpha (0.25f)); g.fillEllipse (x - 13, y - 13, 26, 26); }
        g.setColour (Col::lcd2); g.fillEllipse (x - 8, y - 8, 16, 16); g.setColour (bo ? bc : bc.withAlpha (0.3f)); g.drawEllipse (x - 8, y - 8, 16, 16, 2.0f);
        g.setFont (ft (9.0f, true, true)); g.drawText (juce::String (b + 1), juce::Rectangle<float> (x - 8, y - 8, 16, 16), juce::Justification::centred);
        auto dragNode = [this, r, b, cut] (const juce::MouseEvent& e)
        {
            auto& PP = ed.proc.pad[ed.sel];
            PP.v[bandP (b, 2)] = juce::jlimit (20.0f, 20000.0f, xToFreq (e.position.x, r));
            if (! cut) PP.v[bandP (b, 3)] = std::round (juce::jlimit (-18.0f, 18.0f, (r.getCentreY() - e.position.y) / (r.getHeight() * 0.5f) * 18.0f) * 2) / 2;
            PP.v[bandP (b, 0)] = 1; PP.v[EQON] = 1;
        };
        hots.push_back ({ juce::Rectangle<float> (x - 12, y - 12, 24, 24), [this, b] (const juce::MouseEvent&) { if (eqBand != b) { eqBand = b; ed.bindQlinks(); } }, dragNode,
                          [this, b] (float d) { auto& q = ed.proc.pad[ed.sel].v[bandP (b, 4)]; q = juce::jlimit (0.3f, 8.0f, q.load() * (d > 0 ? 1.15f : 0.87f)); },
                          [this, b] { auto& PP = ed.proc.pad[ed.sel]; PP.v[bandP (b, 0)] = PP.get (bandP (b, 0)) > 0.5f ? 0.0f : 1.0f; },
                          "Banda " + juce::String (b + 1) + ": trascina (destra/sinistra = frequenza, su/giu = guadagno), rotella = Q, doppio clic = accendi/spegni." });
    }
    // punto del filtro della voce
    {
        const float nx = freqToX (juce::jmin (fc, 19999.0f), r), ny = r.getY() + 10 + (1 - pd.get (RESO)) * (r.getHeight() * 0.35f);
        g.setColour (Col::lcd2); g.fillEllipse (nx - 8, ny - 8, 16, 16); g.setColour (Col::amber.withAlpha (fOn ? 1.0f : 0.4f)); g.drawEllipse (nx - 8, ny - 8, 16, 16, 2.0f);
        g.setFont (ft (9.0f, true, true)); g.drawText ("F", juce::Rectangle<float> (nx - 8, ny - 8, 16, 16), juce::Justification::centred);
        auto dragF = [this, r] (const juce::MouseEvent& e)
        {
            auto& PP = ed.proc.pad[ed.sel];
            ed.proc.setPadValue (ed.sel, CUTOFF, juce::jlimit (40.0f, 20000.0f, xToFreq (e.position.x, r)));
            ed.proc.setPadValue (ed.sel, RESO, juce::jlimit (0.0f, 1.0f, 1 - (e.position.y - r.getY() - 10) / (r.getHeight() * 0.35f)));
            juce::ignoreUnused (PP);
        };
        hots.push_back ({ juce::Rectangle<float> (nx - 12, ny - 12, 24, 24), [] (const juce::MouseEvent&) {}, dragF, nullptr,
                          [this] { ed.proc.setPadValue (ed.sel, CUTOFF, 20000); ed.proc.setPadValue (ed.sel, RESO, 0.1f); },
                          "F = filtro della voce: destra/sinistra = cutoff, su/giu = risonanza, doppio clic = aperto. ENV e DECAY sulle Q-link fanno muovere il cutoff a ogni colpo." });
    }
    glowText (g, juce::String ("BANDA ") + juce::String (sb + 1) + "  " + eqTypeName (juce::roundToInt (pd.get (bandP (sb, 1)))) + "  " + formatPadParam (bandP (sb, 2), pd.get (bandP (sb, 2))),
              juce::Rectangle<float> (r.getX() + 6, r.getY() + 2, 360, 13), kBandCol[sb], ft (9.5f, true, true));
    if (P.eqSoloPad.load() == ed.sel && P.eqSoloBand.load() >= 0)
        glowText (g, "SOLO BANDA " + juce::String (P.eqSoloBand.load() + 1) + " (S per tornare)", juce::Rectangle<float> (r.getRight() - 250, r.getY() + 2, 244, 13), Col::amber, ft (9.5f, true, true), juce::Justification::centredRight);
}

// ---------- COMP ----------
void Screen::pageComp (juce::Graphics& g, juce::Rectangle<float> c)
{
    auto& P = ed.proc; auto& pd = P.pad[ed.sel]; const auto pc = padColour (pd.colour.load());
    auto top = c.removeFromTop (26); c.removeFromTop (6);
    const char* modes[] = { "OFF", "SOFT", "MEDIUM", "HARD" }; const int m = juce::roundToInt (pd.get (CMODE));
    for (int i = 0; i < 4; ++i) { chip (g, top.removeFromLeft (84), "", modes[i], m == i, [this, i] (const juce::MouseEvent&) { ed.proc.pad[ed.sel].v[CMODE] = (float) i; }, i == 0 ? Col::phos : pc); top.removeFromLeft (6); }
    static const float ratio[] = { 1, 2, 4, 10 }, knee[] = { 0, 10, 6, 1 };
    auto gr = c.removeFromLeft (c.getHeight()).reduced (14, 4); c.removeFromLeft (10);
    const float thr = -pd.get (CAMT) * 36, mk = pd.get (CMAKEUP) + pd.get (CAMT) * 36 * (1 - 1 / ratio[m]) * 0.25f;
    for (int k = 0; k <= 4; ++k) { g.setColour (Col::phos.withAlpha (0.08f)); g.drawHorizontalLine ((int) (gr.getY() + gr.getHeight() * k / 4), gr.getX(), gr.getRight()); g.drawVerticalLine ((int) (gr.getX() + gr.getWidth() * k / 4), gr.getY(), gr.getBottom()); }
    g.setColour (Col::phos.withAlpha (0.2f)); g.drawLine (gr.getX(), gr.getBottom(), gr.getRight(), gr.getY(), 1.0f);
    juce::Path tc;
    for (int i = 0; i <= 60; ++i)
    {
        const float in = -48 + i * 0.8f, over = in - thr; float out = in;
        if (m > 0) { if (over > knee[m] / 2) out = in - over * (1 - 1 / ratio[m]); else if (over > -knee[m] / 2) { const float o = over + knee[m] / 2; out = in - (1 - 1 / ratio[m]) * o * o / (2 * juce::jmax (0.1f, knee[m])); } out += mk; }
        const float x = gr.getX() + gr.getWidth() * (in + 48) / 48, y = gr.getBottom() - gr.getHeight() * juce::jlimit (0.0f, 1.0f, (out + 48) / 48);
        if (i == 0) tc.startNewSubPath (x, y); else tc.lineTo (x, y);
    }
    g.setColour (pc.withAlpha (0.3f)); g.strokePath (tc, juce::PathStrokeType (6.0f)); g.setColour (m > 0 ? pc : pc.withAlpha (0.3f)); g.strokePath (tc, juce::PathStrokeType (2.0f));
    if (m > 0) { const float tx = gr.getX() + gr.getWidth() * (thr + 48) / 48; g.setColour (Col::amber.withAlpha (0.6f)); g.drawVerticalLine ((int) tx, gr.getY(), gr.getBottom());
        glowText (g, "THR " + juce::String (thr, 1), juce::Rectangle<float> (tx + 3, gr.getBottom() - 14, 70, 12), Col::amber, ft (8.5f, true, true)); }
    hots.push_back ({ gr, [] (const juce::MouseEvent&) {}, [this, gr] (const juce::MouseEvent& e) { ed.proc.pad[ed.sel].v[CAMT] = juce::jlimit (0.0f, 1.0f, -((e.position.x - gr.getX()) / gr.getWidth() * 48 - 48) / 36); if (ed.proc.pad[ed.sel].get (CMODE) < 0.5f) ed.proc.pad[ed.sel].v[CMODE] = 2; }, nullptr, nullptr });
    // misuratore di riduzione
    auto info = c;
    glowText (g, "RIDUZIONE", info.removeFromTop (14), Col::phos.withAlpha (0.6f), ft (9.0f, true, true));
    auto bar = info.removeFromTop (22).reduced (0, 4); const float grv = juce::jlimit (0.0f, 24.0f, P.padGr[ed.sel].load());
    g.setColour (Col::phos.withAlpha (0.1f)); g.fillRoundedRectangle (bar, 3);
    g.setColour (Col::amber); g.fillRoundedRectangle (bar.withLeft (bar.getRight() - bar.getWidth() * grv / 24), 3);
    glowText (g, "-" + juce::String (grv, 1) + " dB", info.removeFromTop (18), Col::amber, ft (11.0f, true, true));
    info.removeFromTop (8);
    const char* desc[] = { "compressore spento", "SOFT: 2:1, attacco lento, ginocchio morbido. Colla senza schiacciare.",
                           "MEDIUM: 4:1, attacco medio. Controllo e punch sui colpi.", "HARD: 10:1, attacco rapidissimo. Schiaccia e porta su la coda." };
    g.setFont (ft (10.0f, false, true)); g.setColour (Col::phos.withAlpha (0.7f)); g.drawFittedText (desc[m], info.removeFromTop (60).toNearestInt(), juce::Justification::topLeft, 4);
    glowText (g, "MAKEUP " + formatPadParam (CMAKEUP, pd.get (CMAKEUP)) + "   MIX " + formatPadParam (CMIX, pd.get (CMIX)), info.removeFromTop (16), Col::phos, ft (10.0f, true, true));
}

// ---------- FX: saturazione e distorsione, lo-fi, mandate ----------
void Screen::padSlider (juce::Graphics& g, juce::Rectangle<float>& area, const juce::String& label, int param, juce::Colour col)
{
    auto& pd = ed.proc.pad[ed.sel]; auto rr = area.removeFromTop (34); area.removeFromTop (4);
    const float v = pd.get (param), n = ed.toNorm (param, v);
    g.setFont (ft (9.5f, true, true)); g.setColour (Col::phos.withAlpha (0.65f)); g.drawText (label, rr.withHeight (16), juce::Justification::centredLeft);
    juce::String val = formatPadParam (param, v);
    if (param == BITS) val = v < 0.005f ? juce::String ("OFF (16 bit)") : juce::String (juce::roundToInt (15 - v * 11)) + " bit";
    if (param == RATE) val = v < 0.005f ? juce::String ("OFF") : juce::String (48.0f * (1.0f - v * 0.88f), 1) + " kHz";
    glowText (g, val, rr.withHeight (16), col, ft (11.0f, true, true), juce::Justification::centredRight);
    auto bar = juce::Rectangle<float> (rr.getX(), rr.getY() + 21, rr.getWidth(), 7);
    g.setColour (Col::phos.withAlpha (0.1f)); g.fillRoundedRectangle (bar, 3); g.setColour (col.withAlpha (0.85f)); g.fillRoundedRectangle (bar.withWidth (juce::jmax (3.0f, bar.getWidth() * n)), 3);
    hots.push_back ({ rr, [this, param] (const juce::MouseEvent&) { sGain0 = ed.toNorm (param, ed.proc.pad[ed.sel].get (param)); },
                      [this, param] (const juce::MouseEvent& e) { const float d = (e.getDistanceFromDragStartX() - e.getDistanceFromDragStartY()) / (e.mods.isShiftDown() ? 900.0f : 180.0f);
                                                              ed.setPadParam (param, ed.fromNorm (param, juce::jlimit (0.0f, 1.0f, sGain0 + d))); },
                      [this, param] (float d) { ed.setPadParam (param, ed.fromNorm (param, juce::jlimit (0.0f, 1.0f, ed.toNorm (param, ed.proc.pad[ed.sel].get (param)) + (d > 0 ? 0.02f : -0.02f)))); },
                      [this, param] { ed.setPadParam (param, kPInfo[param].def); } });
}

void Screen::pageFx (juce::Graphics& g, juce::Rectangle<float> c)
{
    auto& P = ed.proc; auto& pd = P.pad[ed.sel];
    const float bw = (c.getWidth() - 20) / 3;
    auto box = [&] (juce::Rectangle<float> r, const char* nm, juce::Colour col, bool on)
    {
        g.setColour (Col::phos.withAlpha (0.04f)); g.fillRoundedRectangle (r, 6); g.setColour (on ? col : Col::phos.withAlpha (0.22f)); g.drawRoundedRectangle (r, 6, 1.2f);
        glowText (g, nm, r.reduced (10, 6).withHeight (16), on ? col : Col::phos.withAlpha (0.6f), ft (11.5f, true, true));
        return r.reduced (10, 0).withTrimmedTop (28);
    };
    // saturazione / distorsione
    auto b1 = c.removeFromLeft (bw); c.removeFromLeft (10);
    const int dt = juce::roundToInt (pd.get (DTYPE)); const bool dOn = pd.get (DRIVE) > 0.005f;
    auto in1 = box (b1, "SATURAZIONE", Col::amber, dOn);
    for (int row = 0; row < 2; ++row)
    {
        auto rr = in1.removeFromTop (22); in1.removeFromTop (3); const int n = row == 0 ? 4 : 3; const float cw = (rr.getWidth() - (n - 1) * 3) / n;
        for (int k = 0; k < n; ++k)
        {
            const int t = row * 4 + k;
            chip (g, rr.removeFromLeft (cw), "", driveTypeName (t), dt == t, [this, t] (const juce::MouseEvent&) { auto& PP = ed.proc.pad[ed.sel]; PP.v[DTYPE] = (float) t; if (PP.get (DRIVE) < 0.005f) ed.setPadParam (DRIVE, 0.35f); }, t >= 4 ? Col::red : Col::amber);
            hots.back().tip = driveTypeDesc (t); rr.removeFromLeft (3);
        }
    }
    in1.removeFromTop (4);
    padSlider (g, in1, "QUANTITA", DRIVE, dt >= 4 ? Col::red : Col::amber);
    g.setColour (Col::phos.withAlpha (0.6f)); g.setFont (ft (9.0f, false, true));
    g.drawFittedText (driveTypeDesc (dt), in1.removeFromTop (44).toNearestInt(), juce::Justification::topLeft, 4);
    // lo-fi
    auto b2 = c.removeFromLeft (bw); c.removeFromLeft (10);
    auto in2 = box (b2, "LO-FI", Col::cyan, pd.get (BITS) > 0.005f || pd.get (RATE) > 0.005f);
    padSlider (g, in2, "BIT (meno bit = piu sporco)", BITS, Col::cyan);
    padSlider (g, in2, "FREQUENZA (piu bassa = piu scuro)", RATE, Col::cyan);
    g.setColour (Col::phos.withAlpha (0.6f)); g.setFont (ft (9.0f, false, true));
    g.drawFittedText ("Riduce bit e frequenza di campionamento come i vecchi campionatori: grana, aliasing, suono da 12 bit. Per un lo-fi completo prova anche Jackpot Dust.",
                      in2.removeFromTop (70).toNearestInt(), juce::Justification::topLeft, 5);
    // mandate
    auto b3 = c;
    auto in3 = box (b3, "MANDATE", Col::magenta, pd.get (SREV) > 0.005f || pd.get (SDLY) > 0.005f);
    padSlider (g, in3, "AL RIVERBERO", SREV, Col::cyan);
    padSlider (g, in3, "AL DELAY", SDLY, Col::magenta);
    auto par = [&P] (const char* id) { return P.apvts.getRawParameterValue (id)->load(); };
    auto* dd = dynamic_cast<juce::AudioParameterChoice*> (P.apvts.getParameter ("dlyDiv"));
    const juce::String dly = dd->getIndex() == dd->choices.size() - 1 ? juce::String (juce::roundToInt (par ("dlyMs"))) + " ms" : dd->getCurrentChoiceName();
    g.setColour (Col::phos.withAlpha (0.6f)); g.setFont (ft (9.0f, false, true));
    g.drawFittedText ("Riverbero: coda " + juce::String (0.4f + par ("revSize") * par ("revSize") * 7.6f, 1) + " s, pre-delay " + juce::String (juce::roundToInt (par ("revPre"))) + " ms.\nDelay: " + dly
                      + ", feedback " + juce::String (juce::roundToInt (par ("dlyFb") * 100)) + "%.\nSono comuni a tutto il kit: si regolano in MASTER.",
                      in3.removeFromTop (54).toNearestInt(), juce::Justification::topLeft, 4);
    chip (g, in3.removeFromTop (24), "", juce::String::fromUTF8 ("REGOLA IN MASTER \xe2\x96\xb6"), true, [this] (const juce::MouseEvent&) { ed.setPage (PG_MASTER); }, Col::magenta);
}

// ---------- MASTER ----------
void Screen::pageMaster (juce::Graphics& g, juce::Rectangle<float> c)
{
    auto& P = ed.proc; auto par = [&P] (const char* id) { return P.apvts.getParameter (id); };
    auto val = [&P] (const char* id) { return P.apvts.getRawParameterValue (id)->load(); };
    const float bw = (c.getWidth() - 30) / 4;
    auto box = [&] (juce::Rectangle<float> r, const char* nm, juce::Colour col, bool on)
    {
        g.setColour (Col::phos.withAlpha (0.04f)); g.fillRoundedRectangle (r, 6); g.setColour (on ? col : Col::phos.withAlpha (0.25f)); g.drawRoundedRectangle (r, 6, 1.2f);
        glowText (g, nm, r.reduced (10, 6).withHeight (16), on ? col : Col::phos.withAlpha (0.6f), ft (12.0f, true, true));
    };
    auto pct = [] (float v) { return juce::String (juce::roundToInt (v * 100)) + "%"; };
    // cursore: trascina (su/giu o destra/sinistra), rotella, doppio clic = valore di serie
    auto slider = [&] (juce::Rectangle<float>& r, const juce::String& lab, const char* id, juce::Colour col, std::function<juce::String (float)> fmt)
    {
        auto* pr = par (id); auto rr = r.removeFromTop (34); r.removeFromTop (2);
        const float n = pr->getValue();
        g.setFont (ft (9.5f, true, true)); g.setColour (Col::phos.withAlpha (0.6f)); g.drawText (lab, rr.withHeight (16), juce::Justification::centredLeft);
        glowText (g, fmt (pr->convertFrom0to1 (n)), rr.withHeight (16), col, ft (11.0f, true, true), juce::Justification::centredRight);
        auto bar = juce::Rectangle<float> (rr.getX(), rr.getY() + 21, rr.getWidth(), 7);
        g.setColour (Col::phos.withAlpha (0.1f)); g.fillRoundedRectangle (bar, 3); g.setColour (col.withAlpha (0.85f)); g.fillRoundedRectangle (bar.withWidth (juce::jmax (3.0f, bar.getWidth() * n)), 3);
        hots.push_back ({ rr, [this, pr] (const juce::MouseEvent&) { sGain0 = pr->getValue(); pr->beginChangeGesture(); upFn = [pr] { pr->endChangeGesture(); }; },
                          [this, pr] (const juce::MouseEvent& e) { const float d = (e.getDistanceFromDragStartX() - e.getDistanceFromDragStartY()) / (e.mods.isShiftDown() ? 900.0f : 180.0f); pr->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, sGain0 + d)); },
                          [pr] (float d) { pr->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, pr->getValue() + (d > 0 ? 0.02f : -0.02f))); },
                          [pr] { pr->setValueNotifyingHost (pr->getDefaultValue()); } });
    };
    // riverbero
    auto r1 = c.removeFromLeft (bw); c.removeFromLeft (10);
    box (r1, "REVERB", Col::cyan, val ("revKit") > 0 || val ("revRet") > 0); auto in1 = r1.reduced (10, 0).withTrimmedTop (28);
    slider (in1, "SUL KIT", "revKit", Col::cyan, pct); slider (in1, "RITORNO SEND", "revRet", Col::cyan, pct);
    slider (in1, "CODA", "revSize", Col::cyan, [] (float v) { return juce::String (0.4f + v * v * 7.6f, 1) + " s"; });
    slider (in1, "PRE-DELAY", "revPre", Col::cyan, [] (float v) { return juce::String (juce::roundToInt (v)) + " ms"; });
    slider (in1, "TAGLIO BASSI", "revLo", Col::cyan, [] (float v) { return juce::String (juce::roundToInt (v)) + " Hz"; });
    slider (in1, "TAGLIO ALTI", "revHi", Col::cyan, [] (float v) { return juce::String (v / 1000.0f, 1) + " kHz"; });
    { juce::Path dec; auto gr = in1.reduced (0, 6);
      for (int i = 0; i <= 60; ++i) { const float t = i / 60.0f, y = std::exp (-t * (5.5f - val ("revSize") * 4.5f)); const juce::Point<float> pt (gr.getX() + gr.getWidth() * t, gr.getBottom() - gr.getHeight() * y); if (i == 0) dec.startNewSubPath (pt); else dec.lineTo (pt); }
      g.setColour (Col::cyan.withAlpha (0.8f)); g.strokePath (dec, juce::PathStrokeType (1.5f)); }
    // delay
    auto r2 = c.removeFromLeft (bw); c.removeFromLeft (10);
    box (r2, "DELAY", Col::magenta, val ("dlyKit") > 0 || val ("dlyRet") > 0); auto in2 = r2.reduced (10, 0).withTrimmedTop (28);
    auto* dt = dynamic_cast<juce::AudioParameterChoice*> (par ("dlyDiv"));
    const bool libero = dt->getIndex() == dt->choices.size() - 1;
    {
        auto tr = in2.removeFromTop (24);
        auto step = [dt] (int d) { return [dt, d] (const juce::MouseEvent&) { const int n = dt->choices.size(); dt->setValueNotifyingHost (dt->convertTo0to1 ((float) ((dt->getIndex() + d + n) % n))); }; };
        chip (g, tr.removeFromLeft (24), "", "<", false, step (-1), Col::magenta);
        chip (g, tr.removeFromRight (24), "", ">", false, step (1), Col::magenta);
        chip (g, tr.reduced (3, 0), "", "TEMPO " + dt->getCurrentChoiceName(), true, step (1), Col::magenta);
        hots.back().tip = "Tempo del delay: a tempo con Ableton (da 1/32 a 1/1, puntati D e terzine T) oppure LIBERO in millisecondi.";
    }
    in2.removeFromTop (4);
    if (libero) slider (in2, "TEMPO LIBERO", "dlyMs", Col::magenta, [] (float v) { return juce::String (juce::roundToInt (v)) + " ms"; });
    slider (in2, "SUL KIT", "dlyKit", Col::magenta, pct); slider (in2, "RITORNO SEND", "dlyRet", Col::magenta, pct);
    slider (in2, "FEEDBACK", "dlyFb", Col::magenta, pct);
    slider (in2, "TAGLIO BASSI", "dlyLo", Col::magenta, [] (float v) { return juce::String (juce::roundToInt (v)) + " Hz"; });
    slider (in2, "TAGLIO ALTI", "dlyHi", Col::magenta, [] (float v) { return juce::String (v / 1000.0f, 1) + " kHz"; });
    // saturazione e bus
    auto r3 = c.removeFromLeft (bw); c.removeFromLeft (10);
    box (r3, "COLORE", Col::amber, val ("busMode") > 0.5f || val ("satAmt") > 0 || val ("clip") > 0.5f); auto in3 = r3.reduced (10, 0).withTrimmedTop (28);
    slider (in3, "SATURAZIONE", "satAmt", Col::amber, pct);
    g.setFont (ft (9.5f, true, true)); g.setColour (Col::phos.withAlpha (0.6f)); g.drawText ("BUS COMP", in3.removeFromTop (14), juce::Justification::centredLeft);
    auto* bm = dynamic_cast<juce::AudioParameterChoice*> (par ("busMode"));
    for (int row2 = 0; row2 < 2; ++row2)
    {
        auto row = in3.removeFromTop (22); in3.removeFromTop (3); const float cw = (row.getWidth() - 4) / 2;
        for (int k = 0; k < 2; ++k) { const int i = row2 * 2 + k; chip (g, row.removeFromLeft (cw), "", bm->choices[i], bm->getIndex() == i, [bm, i] (const juce::MouseEvent&) { bm->setValueNotifyingHost (bm->convertTo0to1 ((float) i)); }, Col::amber); row.removeFromLeft (4); }
    }
    slider (in3, "QUANTITA BUS", "busAmt", Col::amber, pct);
    auto grb = in3.removeFromTop (10).reduced (0, 2); const float gr = juce::jlimit (0.0f, 18.0f, P.busGr.load());
    g.setColour (Col::phos.withAlpha (0.1f)); g.fillRoundedRectangle (grb, 2); g.setColour (Col::amber); g.fillRoundedRectangle (grb.withLeft (grb.getRight() - grb.getWidth() * gr / 18), 2);
    in3.removeFromTop (6);
    auto* cl = par ("clip");
    chip (g, in3.removeFromTop (24), "", val ("clip") > 0.5f ? "CLIP ON" : "CLIP OFF", val ("clip") > 0.5f, [cl] (const juce::MouseEvent&) { cl->setValueNotifyingHost (cl->getValue() > 0.5f ? 0.0f : 1.0f); }, Col::red);
    // uscita
    auto r4 = c; box (r4, "USCITA", Col::green, true); auto in4 = r4.reduced (10, 0).withTrimmedTop (28);
    slider (in4, "OUTPUT", "out", Col::green, [] (float v) { return juce::String (v, 1) + " dB"; });
    const float m[2] = { P.meterL.load(), P.meterR.load() };
    for (int ch = 0; ch < 2; ++ch)
    {
        auto mb = in4.removeFromTop (10).reduced (0, 1); in4.removeFromTop (3);
        const float db = juce::Decibels::gainToDecibels (m[ch], -48.0f), n = juce::jlimit (0.0f, 1.0f, (db + 48) / 51);
        for (int k = 0; k < 24; ++k) { const float t = k / 24.0f; g.setColour (t <= n ? (t > 0.9f ? Col::red : t > 0.75f ? Col::amber : Col::green) : Col::phos.withAlpha (0.08f)); g.fillRect (mb.getX() + k * mb.getWidth() / 24, mb.getY(), mb.getWidth() / 24 - 1.5f, mb.getHeight()); }
    }
    in4.removeFromTop (8);
    g.setColour (Col::phos.withAlpha (0.55f)); g.setFont (ft (9.0f, false, true));
    g.drawFittedText ("SUL KIT = effetto su tutto il kit.\nRITORNO SEND = volume di reverb e delay mandati dai singoli pad (pagina FX).\n\nTrascina i cursori, rotella per i passi, doppio clic = valore di serie.",
                      in4.toNearestInt(), juce::Justification::topLeft, 12);
}

void Screen::mouseMove (const juce::MouseEvent& e)
{
    juce::String h;
    for (auto it = hots.rbegin(); it != hots.rend(); ++it) if (it->r.contains (e.position)) { h = it->tip; break; }
    if (h != hint) { hint = h; repaint(); }
}
void Screen::mouseDown (const juce::MouseEvent& e)
{
    drag = nullptr;
    for (auto it = hots.rbegin(); it != hots.rend(); ++it)
        if (it->r.contains (e.position)) { if (it->down) it->down (e); drag = it->drag; repaint(); ed.repaint(); return; }
}
void Screen::mouseDrag (const juce::MouseEvent& e) { if (drag) { drag (e); repaint(); } }
void Screen::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    for (auto it = hots.rbegin(); it != hots.rend(); ++it) if (it->r.contains (e.position) && it->wheel) { it->wheel (w.deltaY); repaint(); return; }
}
void Screen::mouseDoubleClick (const juce::MouseEvent& e)
{
    for (auto it = hots.rbegin(); it != hots.rend(); ++it) if (it->r.contains (e.position) && it->dbl) { it->dbl(); repaint(); return; }
}
void Screen::filesDropped (const juce::StringArray& files, int, int)
{
    int n = 0; for (auto& f : files) { if (ed.sel + n >= kPads) break; if (ed.proc.loadFile (juce::File (f), ed.sel + n)) ++n; }
    if (ed.page == PG_BROWSE)
    {
        const auto cat = libCategories()[browseCat]; bool first = true;
        for (auto& f : files) if (addFileToLibrary (juce::File (f), cat) && first) { ed.proc.loadFile (juce::File (f), ed.sel); first = false; }
        browseDirty = true; ed.root.repaint(); return;
    }
    if (ed.page != PG_SAMPLE) ed.setPage (PG_SAMPLE);
    ed.root.repaint();
}
// ======================= Pattern =======================
bool PatternView::cellAt (juce::Point<float> pt, int& pad, int& step) const
{
    auto g = grid(); if (! g.contains (pt)) return false;
    const int row = (int) ((pt.y - g.getY()) / (g.getHeight() / 8)), col = (int) ((pt.x - g.getX()) / (g.getWidth() / 16));
    pad = bank * 8 + (7 - juce::jlimit (0, 7, row)); step = barPage * 16 + juce::jlimit (0, 15, col);
    return true;
}
void PatternView::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat(); auto& P = ed.proc;
    const int bars = P.patternBars.load();
    const bool running = P.uiRunning.load();
    const double playT = running ? std::fmod (P.uiTick.load(), (double) P.loopTicks()) : -1.0;
    if (follow && running && playT >= 0) barPage = (int) (playT / kBar);   // la pagina segue la battuta che suona
    barPage = juce::jlimit (0, bars - 1, barPage);
    heads.clear();
    // intestazione: banco di pad, battuta, lunghezza, pulisci
    auto hd = b.reduced (4, 0).withHeight (20).withY (1);
    glowText (g, "SEQUENCER", hd.removeFromLeft (80), Col::cyan, ft (10.0f, true));
    auto chipH = [&] (juce::Rectangle<float> r, const juce::String& t, bool on, juce::Colour c, std::function<void()> fn)
    {
        g.setColour (on ? c.withAlpha (0.22f) : Col::phos.withAlpha (0.05f)); g.fillRoundedRectangle (r, 3);
        g.setColour (on ? c : Col::phos.withAlpha (0.3f)); g.drawRoundedRectangle (r, 3, 1.0f);
        g.setColour (on ? c : Col::phos.withAlpha (0.7f)); g.setFont (ft (8.5f, true, true)); g.drawText (t, r, juce::Justification::centred);
        heads.push_back ({ r, std::move (fn) });
    };
    chipH (hd.removeFromLeft (44).reduced (1, 2), "1-8", bank == 0, Col::magenta, [this] { bank = 0; }); 
    chipH (hd.removeFromLeft (44).reduced (1, 2), "9-16", bank == 1, Col::magenta, [this] { bank = 1; });
    hd.removeFromLeft (8);
    for (int L : { 1, 2, 4, 8 }) chipH (hd.removeFromLeft (24).reduced (1, 2), juce::String (L), bars == L, Col::amber, [this, L] { extendPattern (ed.proc, L, juce::Random::getSystemRandom().nextInt()); barPage = juce::jmin (barPage, L - 1); });
    chipH (hd.removeFromRight (56).reduced (1, 2), "PULISCI", false, Col::red, [this] { const juce::SpinLock::ScopedLockType l (ed.proc.recLock); ed.proc.events.clear(); });
    chipH (hd.removeFromRight (50).reduced (1, 2), "SEGUI", follow, Col::green, [this] { follow = ! follow; });
    // panoramica di tutte le battute: colpi del banco visibile, pagina aperta e testina che scorre
    {
        auto mr = mini(); const float bw = mr.getWidth() / (float) bars;
        g.setColour (Col::phos.withAlpha (0.6f)); g.setFont (ft (7.5f, true, true));
        g.drawText (juce::String (barPage + 1) + "/" + juce::String (bars), juce::Rectangle<float> (6, mr.getY(), 46, mr.getHeight()), juce::Justification::centredLeft);
        for (int k = 0; k < bars; ++k)
        {
            auto sr = juce::Rectangle<float> (mr.getX() + k * bw, mr.getY(), bw, mr.getHeight()).reduced (1, 0);
            g.setColour (k == barPage ? Col::cyan.withAlpha (0.22f) : Col::phos.withAlpha (0.05f)); g.fillRoundedRectangle (sr, 2);
            g.setColour (k == barPage ? Col::cyan : Col::phos.withAlpha (0.2f)); g.drawRoundedRectangle (sr, 2, 1.0f);
            heads.push_back ({ sr, [this, k] { barPage = k; follow = false; } });
        }
        const juce::SpinLock::ScopedLockType l (P.recLock);
        for (auto& e : P.events)
        {
            if (e.pad / 8 != bank || e.tick >= bars * (double) kBar) continue;
            const float x = mr.getX() + (float) (e.tick / (bars * (double) kBar)) * mr.getWidth();
            const float y = mr.getY() + 1.5f + (7 - e.pad % 8) * (mr.getHeight() - 3) / 8.0f;
            g.setColour (padColour (P.pad[e.pad].colour.load()).withAlpha (0.85f)); g.fillRect (x, y, 1.6f, 1.6f);
        }
        if (playT >= 0) { const float x = mr.getX() + (float) (playT / (bars * (double) kBar)) * mr.getWidth(); g.setColour (Col::magenta); g.fillRect (x - 0.5f, mr.getY() - 1, 2.0f, mr.getHeight() + 2); }
    }
    // griglia
    auto gr = grid(); const float rh = gr.getHeight() / 8, cw = gr.getWidth() / 16;
    g.setColour (Col::lcd2); g.fillRoundedRectangle (b.withTrimmedTop (39), 5);
    for (int r = 0; r < 8; ++r)
    {
        const int pad = bank * 8 + (7 - r); auto smp = P.getSample (pad); const auto pc = padColour (P.pad[pad].colour.load());
        auto lr = juce::Rectangle<float> (6, gr.getY() + r * rh, 46, rh).reduced (0, 1);
        g.setColour (pad == ed.sel ? pc.withAlpha (0.3f) : Col::phos.withAlpha (0.04f)); g.fillRoundedRectangle (lr, 2);
        g.setColour (pc); g.setFont (ft (8.0f, true, true));
        g.drawText (juce::String (pad + 1) + " " + (smp ? smp->name.substring (0, 5) : juce::String ("-")), lr.reduced (3, 0), juce::Justification::centredLeft);
        for (int c = 0; c < 16; ++c)
        {
            auto cr = juce::Rectangle<float> (gr.getX() + c * cw, gr.getY() + r * rh, cw, rh).reduced (1.2f);
            g.setColour (Col::phos.withAlpha (c % 4 == 0 ? 0.09f : 0.045f)); g.fillRoundedRectangle (cr, 2);
        }
    }
    const double t0 = barPage * (double) kBar;
    { const juce::SpinLock::ScopedLockType l (P.recLock);
      for (auto& e : P.events)
      {
          if (e.pad / 8 != bank || e.tick < t0 || e.tick >= t0 + kBar) continue;
          const double st = (e.tick - t0) / 24.0; const int c = (int) std::floor (st), r = 7 - e.pad % 8;
          auto cr = juce::Rectangle<float> (gr.getX() + c * cw, gr.getY() + r * rh, cw, rh).reduced (1.2f);
          const auto pc = padColour (P.pad[e.pad].colour.load());
          if (std::abs (st - c) > 0.01) { g.setColour (pc.withAlpha (0.9f)); g.fillRect (cr.withTrimmedLeft (cr.getWidth() * 0.55f).reduced (0, 2)); continue; }   // colpo fuori griglia (roll, terzine)
          g.setColour (pc.withAlpha (0.25f + 0.75f * e.vel / 127.0f)); g.fillRoundedRectangle (cr, 2);
          g.setColour (juce::Colours::white.withAlpha (0.25f)); g.fillRect (cr.withHeight (cr.getHeight() * (1 - e.vel / 127.0f)).withY (cr.getY()));
      } }
    if (P.uiRunning.load())
    {
        const double t = std::fmod (P.uiTick.load(), (double) P.loopTicks());
        if (t >= t0 && t < t0 + kBar) { const float x = gr.getX() + gr.getWidth() * (float) ((t - t0) / kBar); g.setColour (Col::magenta.withAlpha (0.8f)); g.drawVerticalLine ((int) x, gr.getY(), gr.getBottom()); }
    }
}
void PatternView::paintOverChildren (juce::Graphics& g)
{
    if (info.isEmpty() || juce::Time::getMillisecondCounter() - infoT > 4500) return;
    auto r = grid().removeFromBottom (18).reduced (2, 1);
    g.setColour (juce::Colours::black.withAlpha (0.8f)); g.fillRoundedRectangle (r, 3);
    g.setColour (Col::amber); g.setFont (ft (8.5f, true, true)); g.drawFittedText (info, r.reduced (4, 0).toNearestInt(), juce::Justification::centredLeft, 1, 0.8f);
}
void PatternView::mouseDown (const juce::MouseEvent& e)
{
    for (auto& [r, fn] : heads) if (r.contains (e.position)) { fn(); repaint(); return; }
    if (labels().contains (e.position))
    {
        const int row = (int) ((e.position.y - grid().getY()) / (grid().getHeight() / 8)); const int pad = bank * 8 + 7 - juce::jlimit (0, 7, row);
        ed.selectPad (pad);
        if (e.mods.isPopupMenu())
        {   // tasto destro sul nome della riga: rigenera o svuota solo quel pad
            juce::PopupMenu m; m.addSectionHeader ("PAD " + juce::String (pad + 1));
            m.addItem (1, "Rigenera questa riga"); m.addItem (2, "Svuota questa riga");
            juce::Component::SafePointer<PatternView> sp (this);
            m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [sp, pad] (int r)
            {
                if (sp == nullptr || r <= 0) return;
                auto& P = sp->ed.proc;
                if (r == 1) { sp->info = jp::regenRow (P, P.genStyle, juce::Random::getSystemRandom().nextInt() | 1, pad); sp->infoT = juce::Time::getMillisecondCounter(); }
                else { const juce::SpinLock::ScopedLockType l (P.recLock); P.events.erase (std::remove_if (P.events.begin(), P.events.end(), [pad] (const PatEv& ev) { return ev.pad == pad; }), P.events.end()); }
                sp->repaint();
            });
            return;
        }
        ed.playPad (pad, 100); repaint(); return;
    }
    int pad, step; if (! cellAt (e.position, pad, step)) return;
    auto& P = ed.proc; const double tick = step * 24.0;
    const juce::SpinLock::ScopedLockType l (P.recLock);
    auto it = std::find_if (P.events.begin(), P.events.end(), [&] (const PatEv& ev) { return ev.pad == pad && ev.tick >= tick && ev.tick < tick + 24; });
    if (it != P.events.end()) { P.events.erase (it); paintMode = 0; }
    else { P.events.push_back ({ tick, pad, e.mods.isShiftDown() ? 127 : e.mods.isAltDown() ? 64 : 100 }); paintMode = 1; }
    lastCell = pad * 1000 + step; repaint();
}
void PatternView::mouseDrag (const juce::MouseEvent& e)
{
    int pad, step; if (paintMode < 0 || ! cellAt (e.position, pad, step) || pad * 1000 + step == lastCell) return;
    lastCell = pad * 1000 + step; auto& P = ed.proc; const double tick = step * 24.0;
    const juce::SpinLock::ScopedLockType l (P.recLock);
    auto it = std::find_if (P.events.begin(), P.events.end(), [&] (const PatEv& ev) { return ev.pad == pad && ev.tick >= tick && ev.tick < tick + 24; });
    if (paintMode == 1 && it == P.events.end()) P.events.push_back ({ tick, pad, 100 });
    if (paintMode == 0 && it != P.events.end()) P.events.erase (it);
    repaint();
}
void PatternView::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{   // rotella su un colpo = velocity
    int pad, step; if (! cellAt (e.position, pad, step)) return;
    auto& P = ed.proc; const double tick = step * 24.0; const juce::SpinLock::ScopedLockType l (P.recLock);
    for (auto& ev : P.events) if (ev.pad == pad && ev.tick >= tick && ev.tick < tick + 24) ev.vel = juce::jlimit (10, 127, ev.vel + (w.deltaY > 0 ? 8 : -8));
    repaint();
}

// ======================= Drag MIDI =======================
void DragMidi::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (2);
    g.setColour (juce::Colours::black.withAlpha (0.6f)); g.fillRoundedRectangle (b.translated (0, 2), 6);
    juce::ColourGradient cg (Col::magenta.withAlpha (0.9f), b.getX(), b.getY(), juce::Colour (0xff8b2cff), b.getRight(), b.getBottom(), false);
    g.setGradientFill (cg); g.fillRoundedRectangle (b, 6);
    g.setColour (juce::Colours::white.withAlpha (0.25f)); g.drawLine (b.getX() + 6, b.getY() + 1, b.getRight() - 6, b.getY() + 1, 1.0f);
    g.setColour (juce::Colours::white); g.setFont (ft (10.5f, true)); g.drawText (dragging ? "RILASCIA IN ABLETON" : "DRAG MIDI", b.toNearestInt(), juce::Justification::centred);
}
void DragMidi::mouseDrag (const juce::MouseEvent& e)
{
    if (dragging || e.getDistanceFromDragStart() < 5) return;
    dragging = true; repaint();
    const auto f = ed.proc.writeMidi (true);   // il MIDI esce come lo senti (con lo swing)
    juce::DragAndDropContainer::performExternalDragDropOfFiles ({ f.getFullPathName() }, false, this, [this] { dragging = false; repaint(); });
}

// ======================= Root =======================
void Root::paintStatic (juce::Graphics& g)
{

    const float W = (float) Editor::W, H = (float) Editor::H;
    jlook::backdrop (g, { 0, 0, W, H }, Col::magenta, Col::cyan);
    jlook::header (g, { 0, 0, W, 66 }, "SAMPLER", Col::magenta, "JP-16  DRUM SAMPLER", Col::magenta, Col::cyan);
    for (auto pt : { juce::Point<float> (12, 80), juce::Point<float> (W - 12, 80), juce::Point<float> (12, H - 12), juce::Point<float> (W - 12, H - 12) }) screw (g, pt.x, pt.y);
    jlook::softShadow (g, { 962, 78, 414, 416 }, 14); insetPanel (g, { 962, 78, 414, 416 }, 14);
    // fascia Q-link
    jlook::card (g, { 24, 536, 928, 94 }, 10, Col::cyan, true);
    g.saveState(); g.addTransform (juce::AffineTransform::rotation (-juce::MathConstants<float>::halfPi, 38, 583));
    g.setColour (Col::cyan); g.setFont (ft (9.5f, true)); g.drawText ("Q-LINK", juce::Rectangle<float> (38 - 40, 577, 80, 12), juce::Justification::centred);
    g.restoreState();
    // sequencer
    jlook::card (g, { 24, 640, 928, 182 }, 10, Col::magenta, true);

    g.setColour (Col::dim); g.setFont (ft (9.0f));
    g.drawText ("per suonare dalla tastiera del Mac: in Ableton premi M", juce::Rectangle<float> (962, 800, 300, 14), juce::Justification::centredLeft);
}
void Root::paint (juce::Graphics& g)
{
    // lo sfondo (gradienti, grana, ombre, pannelli) non cambia: si disegna una volta in un'immagine e poi si copia.
    // Prima veniva ridisegnato tutto a ogni aggiornamento e il plugin (e Ableton) andavano a scatti.
    const float W = (float) Editor::W, H = (float) Editor::H;
    const float sc = juce::jlimit (1.0f, 4.0f, g.getInternalContext().getPhysicalPixelScaleFactor());
    if (bg.isNull() || std::abs (bgScale - sc) > 0.01f)
    {
        bgScale = sc;
        bg = juce::Image (juce::Image::RGB, juce::roundToInt (W * sc), juce::roundToInt (H * sc), false);
        juce::Graphics ig (bg); ig.addTransform (juce::AffineTransform::scale (sc)); paintStatic (ig);
    }
    { g.saveState(); g.addTransform (juce::AffineTransform::scale (1.0f / sc)); g.setImageResamplingQuality (juce::Graphics::lowResamplingQuality); g.drawImageAt (bg, 0, 0); g.restoreState(); }   // copia pixel per pixel
    g.setFont (ft (10.0f, true, true)); g.setColour (Col::dim); g.drawText ("KIT", juce::Rectangle<float> (430, 12, 40, 40), juce::Justification::centredLeft);
    glowText (g, ed.proc.kitName.toUpperCase(), { 462, 12, 400, 40 }, Col::amber, ft (11.0f, true, true));
    // meter master
    const float m[2] = { ed.proc.meterL.load(), ed.proc.meterR.load() };
    for (int c = 0; c < 2; ++c)
    {
        auto mb = juce::Rectangle<float> (1210, 712.0f + c * 9, 150, 6);
        const float db = juce::Decibels::gainToDecibels (m[c], -48.0f), n = juce::jlimit (0.0f, 1.0f, (db + 48) / 51);
        for (int k = 0; k < 19; ++k) { const float t = k / 19.0f; g.setColour (t <= n ? (t > 0.9f ? Col::red : t > 0.75f ? Col::amber : Col::green) : juce::Colours::white.withAlpha (0.05f)); g.fillRect (mb.getX() + k * 6.0f, mb.getY(), 4.5f, mb.getHeight()); }
    }
}


// ======================= Editor del campione a tutto schermo =======================
void ChopView::open (int pad)
{
    src = pad; smp = ed.proc.getSample (pad);
    if (smp == nullptr) return;
    auto& P = ed.proc.pad[pad];
    r0 = P.get (START); r1 = P.get (END); v0 = 0; v1 = 1; bpm = P.get (SBPM); warp = juce::roundToInt (P.get (WARP));
    colour = P.colour.load(); gain = P.get (GAIN); firstPad = pad; selSlice = -1; assigned.clear();
    // cache dei picchi per lo zoom indietro
    const int n = smp->buf.getNumSamples(), blk = 128; pkMin.assign ((size_t) (n / blk + 1), 0); pkMax.assign ((size_t) (n / blk + 1), 0);
    const float* d0 = smp->buf.getReadPointer (0); const float* d1 = smp->buf.getReadPointer (juce::jmin (1, smp->buf.getNumChannels() - 1));
    for (int i = 0; i < n; ++i) { const float x = 0.5f * (d0[i] + d1[i]); auto& mn = pkMin[(size_t) (i / blk)]; auto& mx = pkMax[(size_t) (i / blk)]; mn = juce::jmin (mn, x); mx = juce::jmax (mx, x); }
    mode = bpm > 40 ? 0 : 1; rebuild();
    wavePath.clear(); chromeDirty = true;
    setVisible (true); toFront (true); setWantsKeyboardFocus (true); grabKeyboardFocus(); repaint();
}
void ChopView::close() { setVisible (false); ed.kitChanged(); ed.screen.repaint(); }

std::vector<float> ChopView::grid() const
{
    std::vector<float> out; if (! smp) return out;
    const double dur = smp->buf.getNumSamples() / smp->sr;
    if (mode == 2) { static const int nn[] = { 2, 4, 8, 12, 16 }; const int k = nn[juce::jlimit (0, 4, div)]; for (int i = 1; i < k; ++i) out.push_back (r0 + (r1 - r0) * i / k); return out; }
    if (bpm < 40) return out;
    static const double beats[] = { 4.0, 2.0, 1.0, 0.5, 0.25 };   // 1 battuta, 1/2, 1/4, 1/8, 1/16
    const double step = beats[juce::jlimit (0, 4, div)] * 60.0 / bpm / dur;
    if (step <= 0) return out;
    for (double x = r0 + step; x < r1 - step * 0.2; x += step) out.push_back ((float) x);
    return out;
}
float ChopView::snapTo (float fx) const
{
    if (! snap) return fx;
    const auto ref = mode == 1 ? onsets : grid();
    float best = fx, bd = 1e9f; for (float r : ref) if (std::abs (r - fx) < bd) { bd = std::abs (r - fx); best = r; }
    return bd * wave().getWidth() / (v1 - v0) < 14 ? best : fx;
}
void ChopView::rebuild()
{
    marks.clear(); onsets.clear(); if (! smp) return;
    const int n = smp->buf.getNumSamples(), hop = 256; const float* d = smp->buf.getReadPointer (0);
    std::vector<float> en; for (int i = 0; i + hop < n; i += hop) { float e = 0; for (int k = 0; k < hop; ++k) e += d[i + k] * d[i + k]; en.push_back (std::log (1e-6f + e / hop)); }
    std::vector<float> fl (en.size(), 0); double mean = 0;
    for (size_t i = 1; i < en.size(); ++i) { fl[i] = juce::jmax (0.0f, en[i] - en[i - 1]); mean += fl[i]; }
    mean /= juce::jmax ((size_t) 1, en.size()); double var = 0; for (float f : fl) var += (f - mean) * (f - mean); const double sd = std::sqrt (var / juce::jmax ((size_t) 1, fl.size()));
    static const float th[] = { 2.4f, 1.5f, 0.8f };
    const int gap = (int) (0.07 * smp->sr / hop); int last = -gap;
    for (size_t i = 1; i < fl.size(); ++i) if (fl[i] > mean + th[juce::jlimit (0, 2, sens)] * sd && (int) i - last >= gap) { onsets.push_back ((float) (i * hop) / n); last = (int) i; }
    if (mode == 1) { for (float o : onsets) if (o > r0 + 0.002f && o < r1 - 0.002f && marks.size() < 31) marks.push_back (o); }
    else if (mode == 0 || mode == 2) { for (float l : grid()) if (marks.size() < 31) marks.push_back (l); }
    selSlice = -1;
}

void ChopView::paintChrome (juce::Graphics& g)
{
    hots.clear();
    jlook::backdrop (g, getLocalBounds().toFloat(), Col::magenta, Col::cyan);
    g.fillAll (juce::Colours::black.withAlpha (0.55f));
    auto card = juce::Rectangle<float> (14, 14, (float) getWidth() - 28, (float) getHeight() - 28);
    jlook::softShadow (g, card, 16, 30, 0.8f, 10);
    jlook::card (g, card, 16, Col::phos, true, false, false);
    if (! smp) return;
    const auto pc = padColour (colour); const double dur = smp->buf.getNumSamples() / smp->sr;
    auto btn = [&] (juce::Rectangle<float> r, const juce::String& t, juce::Colour c, bool lit, std::function<void (const juce::MouseEvent&)> fn, std::function<void (float)> wh = nullptr)
    { jlook::button (g, r, c, lit, false, t, ft (11.0f, true)); hots.push_back ({ r, std::move (fn), std::move (wh) }); };
    // intestazione
    glowText (g, "EDITOR CAMPIONE", { 34, 24, 220, 28 }, Col::text, ft (17.0f, true));
    glowText (g, "PAD " + juce::String (src + 1) + "  " + smp->name.toUpperCase(), { 230, 24, 560, 28 }, pc, ft (13.0f, true, true));
    btn ({ card.getRight() - 130, 22, 108, 30 }, "CHIUDI  ESC", Col::phos, false, [this] (const juce::MouseEvent&) { close(); });
    btn ({ card.getRight() - 236, 22, 96, 30 }, "STOP", Col::red, false, [this] (const juce::MouseEvent&) { ed.proc.stopAll(); });
    btn ({ card.getRight() - 372, 22, 126, 30 }, "SUONA TUTTO", Col::green, false, [this] (const juce::MouseEvent&) { ed.proc.audition (src, r0, r1); });
    // riga tempo
    float x = 34; const float y1 = 64;
    auto info = [&] (float w, const juce::String& lab, const juce::String& val, juce::Colour c, std::function<void (const juce::MouseEvent&)> fn = nullptr, std::function<void (float)> wh = nullptr)
    {
        auto r = juce::Rectangle<float> (x, y1, w, 38); x += w + 6;
        g.setColour (juce::Colours::black.withAlpha (0.45f)); g.fillRoundedRectangle (r, 6); g.setColour (c.withAlpha (0.5f)); g.drawRoundedRectangle (r, 6, 1.0f);
        g.setColour (Col::dim); g.setFont (ft (8.5f, true, true)); g.drawText (lab, r.reduced (8, 3).withHeight (11), juce::Justification::centredLeft);
        glowText (g, val, r.reduced (8, 2).withTrimmedTop (12), c, ft (13.0f, true, true));
        if (fn || wh) hots.push_back ({ r, fn ? fn : [] (const juce::MouseEvent&) {}, wh });
    };
    const double host = ed.proc.hostBpm();
    info (150, "BPM CAMPIONE (rotella)", bpm > 40 ? juce::String (bpm, 1) : juce::String ("--"), Col::magenta, nullptr, [this] (float d) { bpm = juce::jlimit (0.0f, 240.0f, std::round ((juce::jmax (bpm, 59.0f) + (d > 0 ? 0.5f : -0.5f)) * 2) / 2); if (mode != 1) rebuild(); });
    btn ({ x, y1 + 4, 40, 30 }, juce::String::fromUTF8 ("\xc3\xb7" "2"), Col::magenta, false, [this] (const juce::MouseEvent&) { bpm = juce::jmax (40.0f, bpm / 2); if (mode != 1) rebuild(); }); x += 46;
    btn ({ x, y1 + 4, 40, 30 }, juce::String::fromUTF8 ("\xc3\x97" "2"), Col::magenta, false, [this] (const juce::MouseEvent&) { bpm = juce::jmin (240.0f, bpm * 2); if (mode != 1) rebuild(); }); x += 46;
    btn ({ x, y1 + 4, 74, 30 }, "RILEVA", Col::magenta, false, [this] (const juce::MouseEvent&)
    {
        bool c = false; float b = Processor::estimateBpm (smp->name, smp->buf.getNumSamples() / smp->sr * (r1 - r0), c);
        if (! c) { double st = 0; const float ba = Processor::estimateBpmAudio (smp->buf, smp->sr, st); if (ba > 0) b = ba; }
        if (b > 0) bpm = b; if (mode != 1) rebuild();
    }); x += 82;
    info (110, "BPM DAW", juce::String (host, 1), Col::cyan);
    static const char* wn[] = { "OFF", "REPITCH", "STRETCH" };
    info (130, "A TEMPO CON LA DAW", wn[juce::jlimit (0, 2, warp)], warp > 0 ? Col::green : Col::dim, [this] (const juce::MouseEvent&) { warp = (warp + 1) % 3; });
    const double bars = bpm > 40 ? dur * (r1 - r0) * bpm / 240.0 : 0;
    info (150, "DURATA", juce::String (dur * (r1 - r0), 2) + " s" + (bars > 0 ? "  " + juce::String (bars, 1) + " batt." : juce::String()), Col::phos);
    if (bpm > 40 && warp > 0) info (150, "VELOCITA", juce::String (host / bpm, 3) + juce::String::fromUTF8 ("\xc3\x97"), Col::amber);
    // riga chop
    x = 34; const float y2 = 112;
    static const char* mn[] = { "GRIGLIA BPM", "TRANSIENTI", "PARTI UGUALI", "A MANO" };
    for (int i = 0; i < 4; ++i) { btn ({ x, y2, 116, 32 }, mn[i], Col::magenta, mode == i, [this, i] (const juce::MouseEvent&) { mode = i; if (i < 3) rebuild(); else { marks.clear(); selSlice = -1; } }); x += 120; }
    x += 10;
    if (mode == 0) { static const char* dv[] = { "1 BAR", "1/2", "1/4", "1/8", "1/16" }; for (int i = 0; i < 5; ++i) { btn ({ x, y2, 58, 32 }, dv[i], Col::amber, div == i, [this, i] (const juce::MouseEvent&) { div = i; rebuild(); }); x += 62; } }
    if (mode == 2) { static const char* dv[] = { "2", "4", "8", "12", "16" }; for (int i = 0; i < 5; ++i) { btn ({ x, y2, 52, 32 }, dv[i], Col::amber, div == i, [this, i] (const juce::MouseEvent&) { div = i; rebuild(); }); x += 56; } }
    if (mode == 1) { static const char* sn[] = { "POCHI", "MEDI", "TANTI" }; for (int i = 0; i < 3; ++i) { btn ({ x, y2, 72, 32 }, sn[i], Col::amber, sens == i, [this, i] (const juce::MouseEvent&) { sens = i; rebuild(); }); x += 76; } }
    x += 10;
    btn ({ x, y2, 82, 32 }, snap ? "SNAP ON" : "SNAP OFF", Col::cyan, snap, [this] (const juce::MouseEvent&) { snap = ! snap; }); x += 88;
    btn ({ x, y2, 82, 32 }, "PULISCI", Col::red, false, [this] (const juce::MouseEvent&) { marks.clear(); selSlice = -1; });
    // forma d'onda con zoom
    auto w = wave();
    g.setColour (juce::Colours::black.withAlpha (0.65f)); g.fillRoundedRectangle (w.expanded (4), 8);
    juce::ColourGradient lg (Col::lcd1, w.getCentreX(), w.getCentreY(), Col::lcd2, w.getX(), w.getY(), true); g.setGradientFill (lg); g.fillRoundedRectangle (w, 6);
    g.setColour (Col::dim); g.setFont (ft (9.5f, false, true));
    g.drawText (juce::String::fromUTF8 ("rotella = zoom \xc2\xb7 doppio clic = nuovo taglio \xc2\xb7 tasto destro sul taglio = togli \xc2\xb7 trascina un taglio o una fetta intera per spostarli \xc2\xb7 MAIUSC = senza snap \xc2\xb7 cmd+Z annulla"),
                juce::Rectangle<float> (w.getX(), w.getY() - 14, w.getWidth(), 12), juce::Justification::centredLeft);
    // fette e pad (i pad si disegnano vivi in paint)
    const float yA = 476;
    glowText (g, "ASSEGNA AI PAD", { 34, yA, 200, 22 }, Col::text, ft (13.0f, true));
    g.setColour (Col::dim); g.setFont (ft (10.5f));
    g.drawText (juce::String::fromUTF8 ("trascina una fetta GI\xc3\x99 su un pad  \xc2\xb7  oppure clicca la fetta e poi il pad  \xc2\xb7  oppure SPARGI per riempire i pad in ordine"), juce::Rectangle<float> (200, yA, 800, 22), juce::Justification::centredLeft);
    const auto pts = points();
    const int nSl = (int) pts.size() - 1;
    // azioni sotto i pad
    auto ax = juce::Rectangle<float> (34, padRect (8).getBottom() + 12, card.getRight() - 54, 40);
    g.setColour (juce::Colours::black.withAlpha (0.35f)); g.fillRoundedRectangle (ax, 8);
    glowText (g, juce::String (nSl) + (nSl == 1 ? " FETTA" : " FETTE"), { ax.getX() + 12, ax.getY() + 10, 110, 20 }, Col::phos, ft (12.0f, true, true));
    g.setColour (Col::dim); g.setFont (ft (10.0f)); g.drawText ("dal pad", juce::Rectangle<float> (ax.getX() + 118, ax.getY() + 10, 50, 20), juce::Justification::centredLeft);
    btn ({ ax.getX() + 168, ax.getY() + 8, 26, 24 }, "-", Col::cyan, false, [this] (const juce::MouseEvent&) { firstPad = juce::jmax (0, firstPad - 1); });
    glowText (g, juce::String (firstPad + 1), { ax.getX() + 196, ax.getY() + 8, 30, 24 }, Col::amber, ft (13.0f, true, true), juce::Justification::centred);
    btn ({ ax.getX() + 228, ax.getY() + 8, 26, 24 }, "+", Col::cyan, false, [this] (const juce::MouseEvent&) { firstPad = juce::jmin (kPads - 1, firstPad + 1); });
    btn ({ ax.getX() + 270, ax.getY() + 4, 330, 32 }, juce::String::fromUTF8 ("SPARGI LE FETTE SUI PAD \xe2\x96\xb6"), Col::green, true, [this] (const juce::MouseEvent&)
    {
        const auto pts2 = points();
        for (size_t k = 0; k + 1 < pts2.size(); ++k)
        {
            const int p = firstPad + (int) k; if (p >= kPads) break;
            auto& D = ed.proc.pad[p]; D.reset(); D.colour = colour; D.v[GAIN] = gain; D.v[SBPM] = bpm; D.v[WARP] = (float) warp;
            D.v[START] = pts2[k]; D.v[END] = pts2[k + 1]; D.v[SLICE] = (float) (k + 1); D.v[DECAY] = 4000;
            D.v[SKEY] = ed.proc.pad[src].get (SKEY); D.v[TUNE] = ed.proc.pad[src].get (TUNE);
            ed.proc.setSample (p, smp); assigned[p] = (int) k;
        }
    });
    btn ({ ax.getX() + 612, ax.getY() + 4, 170, 32 }, "SALVA REGIONE", Col::cyan, false, [this] (const juce::MouseEvent&)
    {   // aggiorna solo il pad sorgente con regione, BPM e sync
        auto& D = ed.proc.pad[src]; D.v[START] = r0; D.v[END] = r1; D.v[SBPM] = bpm; D.v[WARP] = (float) warp;
    });
    btn ({ ax.getX() + 790, ax.getY() + 4, 170, 32 }, "ANNULLA ZOOM", Col::cyan, false, [this] (const juce::MouseEvent&) { v0 = 0; v1 = 1; });
    if (nSl > kPads - firstPad) { g.setColour (Col::red); g.setFont (ft (9.5f, true)); g.drawText ("fette oltre il pad 16 non verranno assegnate", juce::Rectangle<float> (ax.getX() + 970, ax.getY() + 4, 220, 32), juce::Justification::centredLeft); }
    // la sorgente salva sempre BPM e sync scelti qui
    { auto& D = ed.proc.pad[src]; if (ed.proc.getSample (src) == smp) { D.v[SBPM] = bpm; D.v[WARP] = (float) warp; } }
}

void ChopView::paint (juce::Graphics& g)
{
    const float ps = juce::jmax (1.0f, g.getInternalContext().getPhysicalPixelScaleFactor());
    if (chromeDirty || chrome.isNull() || std::abs (ps - chromeScale) > 0.01f)
    {
        chromeScale = ps; chromeDirty = false;
        chrome = juce::Image (juce::Image::RGB, juce::jmax (1, juce::roundToInt (getWidth() * ps)), juce::jmax (1, juce::roundToInt (getHeight() * ps)), false);
        juce::Graphics gi (chrome); gi.addTransform (juce::AffineTransform::scale (ps));
        gi.fillAll (Col::bg); paintChrome (gi);
    }
    g.drawImage (chrome, getLocalBounds().toFloat());
    if (! smp) return;
    const auto pc = padColour (colour); const double dur = smp->buf.getNumSamples() / smp->sr;
    auto w = wave();
    const int n = smp->buf.getNumSamples(); const float* d0 = smp->buf.getReadPointer (0); const float* d1 = smp->buf.getReadPointer (juce::jmin (1, smp->buf.getNumChannels() - 1));
    // griglia del tempo (battute e movimenti) sempre visibile se c'e il BPM
    if (bpm > 40)
    {
        const double beatFx = 60.0 / bpm / dur;
        if ((w.getWidth() * beatFx / (v1 - v0)) > 6)
            for (int k = 0; r0 + k * beatFx <= v1 + beatFx; ++k)
            {
                const float fx = (float) (r0 + k * beatFx); if (fx < v0) continue; const float xx = xOf (fx);
                const bool bar = k % 4 == 0;
                g.setColour ((bar ? Col::amber : Col::phos).withAlpha (bar ? 0.35f : 0.1f)); g.drawVerticalLine ((int) xx, w.getY(), w.getBottom());
                if (bar) { g.setFont (ft (9.0f, true, true)); g.setColour (Col::amber.withAlpha (0.8f)); g.drawText (juce::String (k / 4 + 1), juce::Rectangle<float> (xx + 3, w.getY() + 2, 30, 11), juce::Justification::centredLeft); }
            }
    }
    {   // onda (il percorso si ricalcola solo quando cambia lo zoom)
        const float cy = w.getCentreY(), hh = w.getHeight() * 0.45f;
        if (v0 != pathV0 || v1 != pathV1 || wavePath.isEmpty())
        {
        pathV0 = v0; pathV1 = v1;
        float mxAll = 0.01f; for (size_t i = 0; i < pkMax.size(); ++i) mxAll = juce::jmax (mxAll, pkMax[i], -pkMin[i]);
        const float sc = 1.0f / mxAll;
        juce::Path p; std::vector<float> top, bot;
        for (int px = 0; px < (int) w.getWidth(); ++px)
        {
            const double a = (v0 + (v1 - v0) * px / w.getWidth()) * n, b = (v0 + (v1 - v0) * (px + 1) / w.getWidth()) * n;
            float mn = 0, mx = 0;
            if (b - a > 256) { for (int k = (int) (a / 128); k <= (int) (b / 128) && k < (int) pkMax.size(); ++k) { mn = juce::jmin (mn, pkMin[(size_t) k]); mx = juce::jmax (mx, pkMax[(size_t) k]); } }
            else for (int k = (int) a; k <= (int) b && k < n; ++k) { if (k < 0) continue; const float v = 0.5f * (d0[k] + d1[k]); mn = juce::jmin (mn, v); mx = juce::jmax (mx, v); }
            top.push_back (cy - mx * sc * hh); bot.push_back (cy - mn * sc * hh);
        }
        p.startNewSubPath (w.getX(), top[0]); for (size_t i = 1; i < top.size(); ++i) p.lineTo (w.getX() + (float) i, top[i]);
        for (int i = (int) bot.size() - 1; i >= 0; --i) p.lineTo (w.getX() + (float) i, bot[(size_t) i]); p.closeSubPath();
        wavePath = p;
        }
        const auto& p = wavePath;
        juce::ColourGradient wg (pc.withAlpha (0.85f), 0, w.getY(), pc.withAlpha (0.35f), 0, cy, false); wg.addColour (1.0, pc.withAlpha (0.85f));
        g.setGradientFill (wg); g.fillPath (p); g.setColour (pc.brighter (0.4f).withAlpha (0.8f)); g.strokePath (p, juce::PathStrokeType (0.8f));
        g.setColour (Col::phos.withAlpha (0.15f)); g.drawHorizontalLine ((int) cy, w.getX(), w.getRight());
    }
    g.saveState(); g.reduceClipRegion (w.toNearestInt());
    // fuori dalla regione
    if (r0 > v0) { g.setColour (juce::Colours::black.withAlpha (0.55f)); g.fillRect (w.withRight (xOf (r0))); }
    if (r1 < v1) { g.setColour (juce::Colours::black.withAlpha (0.55f)); g.fillRect (w.withLeft (xOf (r1))); }
    // fette
    const auto pts = points();
    for (size_t k = 0; k + 1 < pts.size(); ++k)
    {
        const float xa = xOf (pts[k]), xb = xOf (pts[k + 1]);
        const bool sel = (int) k == selSlice, hov = (int) k == hovSlice;
        g.setColour ((k % 2 ? Col::magenta : Col::cyan).withAlpha (sel ? 0.22f : hov ? 0.12f : 0.06f)); g.fillRect (juce::Rectangle<float> (xa, w.getY(), xb - xa, w.getHeight()));
        if (sel) { g.setColour (Col::amber); g.drawRect (juce::Rectangle<float> (xa, w.getY(), xb - xa, w.getHeight()), 2.0f); }
        if (xb - xa > 18) glowText (g, juce::String ((int) k + 1), { xa + 4, w.getBottom() - 18, 30, 14 }, sel ? Col::amber : Col::phos, ft (11.0f, true, true));
    }
    if (mode == 1) for (float o : onsets) { g.setColour (Col::amber.withAlpha (0.35f)); g.drawVerticalLine ((int) xOf (o), w.getBottom() - 12, w.getBottom()); }
    for (size_t k = 0; k < marks.size(); ++k)
    {
        const float xx = xOf (marks[k]); const bool dr = drag == (int) k + 1 || (drag == 0 && hovMark == (int) k + 1);
        g.setColour ((dr ? Col::amber : Col::magenta).withAlpha (0.3f)); g.fillRect (xx - 2.5f, w.getY(), 5.0f, w.getHeight());
        g.setColour (dr ? Col::amber : Col::magenta); g.drawLine (xx, w.getY(), xx, w.getBottom(), 1.8f);
        juce::Path t; t.addTriangle (xx - 7, w.getY(), xx + 7, w.getY(), xx, w.getY() + 10); g.fillPath (t);
    }
    for (int k = 0; k < 2; ++k)
    {
        const float xx = xOf (k ? r1 : r0);
        g.setColour (Col::green); g.drawLine (xx, w.getY(), xx, w.getBottom(), 2.0f);
        auto hr = juce::Rectangle<float> (k ? xx - 46 : xx, w.getY() + 14, 46, 18); g.fillRoundedRectangle (hr, 3);
        g.setColour (juce::Colours::black); g.setFont (ft (9.5f, true, true)); g.drawText (k ? "FINE" : "INIZIO", hr, juce::Justification::centred);
    }
    // posizione mentre trascini
    if (drag != 0 && drag != -3 && ! toPad)
    {
        const float xx = juce::jlimit (w.getX() + 60, w.getRight() - 60, xOf (drag == -4 ? (selSlice >= 0 && (size_t) selSlice < pts.size() ? pts[(size_t) selSlice] : lastFx) : lastFx));
        auto tag = juce::Rectangle<float> (xx - 60, w.getY() + 36, 120, 18);
        g.setColour (juce::Colours::black.withAlpha (0.8f)); g.fillRoundedRectangle (tag, 4); g.setColour (Col::amber); g.drawRoundedRectangle (tag, 4, 1.0f);
        glowText (g, posText (drag == -4 && selSlice >= 0 && (size_t) selSlice < pts.size() ? pts[(size_t) selSlice] : lastFx), tag, Col::amber, ft (9.5f, true, true), juce::Justification::centred);
    }
    g.restoreState();
    // panoramica
    auto ov = overview();
    g.setColour (juce::Colours::black.withAlpha (0.5f)); g.fillRoundedRectangle (ov, 4);
    for (int px = 0; px < (int) ov.getWidth(); px += 2)
    {
        const int k0 = (int) (px / ov.getWidth() * pkMax.size()), k1 = (int) ((px + 2) / ov.getWidth() * pkMax.size());
        float mx = 0; for (int k = k0; k < k1 && k < (int) pkMax.size(); ++k) mx = juce::jmax (mx, pkMax[(size_t) k], -pkMin[(size_t) k]);
        g.setColour (pc.withAlpha (0.5f)); g.drawVerticalLine ((int) (ov.getX() + px), ov.getCentreY() - mx * ov.getHeight() * 0.5f, ov.getCentreY() + mx * ov.getHeight() * 0.5f + 1);
    }
    auto vr = juce::Rectangle<float> (ov.getX() + v0 * ov.getWidth(), ov.getY(), (v1 - v0) * ov.getWidth(), ov.getHeight());
    g.setColour (Col::phos.withAlpha (0.18f)); g.fillRoundedRectangle (vr, 3); g.setColour (Col::phos); g.drawRoundedRectangle (vr, 3, 1.2f);
    glowText (g, "ZOOM " + juce::String (1.0 / (v1 - v0), 1) + juce::String::fromUTF8 ("\xc3\x97"), { ov.getRight() - 100, ov.getY() - 1, 96, ov.getHeight() }, Col::phos, ft (9.5f, true, true), juce::Justification::centredRight);
    // pad: colore del suono, si accendono quando suonano, si evidenziano al passaggio o come destinazione
    int sliceOn = -1; for (auto& [pp, k] : assigned) if (k == selSlice) sliceOn = pp;
    for (int p = 0; p < kPads; ++p)
    {
        auto r = padRect (p); auto ps = ed.proc.getSample (p); const auto c2 = padColour (ed.proc.pad[p].colour.load());
        const bool fromHere = assigned.count (p) > 0, hov = p == hovPad, target = (toPad || selSlice >= 0) && hov;
        const float lum = juce::jlimit (0.0f, 1.0f, pGlow[p]);
        g.setColour (juce::Colours::black.withAlpha (0.6f)); g.fillRoundedRectangle (r.translated (0, 2), 7);
        juce::ColourGradient pg (fromHere ? c2.withAlpha (0.45f) : juce::Colour (0xff2a2d35), 0, r.getY(), fromHere ? c2.withAlpha (0.18f) : juce::Colour (0xff17191e), 0, r.getBottom(), false);
        g.setGradientFill (pg); g.fillRoundedRectangle (r, 7);
        if (lum > 0.01f) { juce::ColourGradient bl (c2.withAlpha (0.95f * lum), r.getCentreX(), r.getCentreY(), c2.withAlpha (0.15f * lum), r.getX(), r.getY(), true); g.setGradientFill (bl); g.fillRoundedRectangle (r, 7); }
        if (target) { g.setColour (Col::amber.withAlpha (0.25f)); g.fillRoundedRectangle (r, 7); }
        g.setColour (target ? Col::amber : p == sliceOn ? Col::amber.withAlpha (0.8f) : p == src ? Col::green : hov ? juce::Colours::white.withAlpha (0.5f) : juce::Colours::white.withAlpha (0.12f));
        g.drawRoundedRectangle (r, 7, target ? 2.5f : (p == src || p == sliceOn) ? 1.8f : 1.0f);
        g.setColour (lum > 0.4f ? juce::Colours::white : Col::text); g.setFont (ft (11.0f, true, true)); g.drawText (juce::String (p + 1).paddedLeft ('0', 2), r.reduced (8, 5), juce::Justification::topLeft);
        g.setColour (lum > 0.4f ? juce::Colours::white : Col::dim); g.setFont (ft (9.5f));
        const int sl = juce::roundToInt (ed.proc.pad[p].get (SLICE));
        g.drawText (ps ? ps->name + (sl > 0 ? " #" + juce::String (sl) : juce::String()) : juce::String ("vuoto"), r.reduced (8, 5), juce::Justification::bottomLeft);
        if (fromHere) glowText (g, "F" + juce::String (assigned[p] + 1), r.reduced (8, 3), Col::amber, ft (16.0f, true, true), juce::Justification::topRight);
        if (target && selSlice >= 0) glowText (g, juce::String::fromUTF8 ("METTI F") + juce::String (selSlice + 1) + " QUI", r, Col::amber, ft (11.0f, true, true), juce::Justification::centred);
    }
    // fetta trascinata verso i pad
    if (toPad && selSlice >= 0)
    {
        auto tag = juce::Rectangle<float> (dragPos.x - 34, dragPos.y - 14, 68, 28);
        g.setColour (padColour (colour).withAlpha (0.9f)); g.fillRoundedRectangle (tag, 6); g.setColour (juce::Colours::white); g.drawRoundedRectangle (tag, 6, 1.2f);
        g.setFont (ft (13.0f, true, true)); g.drawText ("F" + juce::String (selSlice + 1), tag, juce::Justification::centred);
    }
    else if (selSlice >= 0)
    {
        glowText (g, juce::String::fromUTF8 ("F") + juce::String (selSlice + 1) + (sliceOn >= 0 ? juce::String::fromUTF8 (" \xc3\xa8 sul pad ") + juce::String (sliceOn + 1) : juce::String::fromUTF8 (" scelta: clicca un pad o trascinala gi\xc3\xb9")),
                  { 760, 476, 440, 22 }, Col::amber, ft (11.5f, true, true), juce::Justification::centredRight);
    }
}

void ChopView::assignTo (int p)
{
    const auto pts2 = points();
    if (p < 0 || p >= kPads || selSlice < 0 || selSlice + 1 >= (int) pts2.size()) return;
    auto& D = ed.proc.pad[p]; const float sk = ed.proc.pad[src].get (SKEY), tu = ed.proc.pad[src].get (TUNE);
    D.reset(); D.colour = colour; D.v[GAIN] = gain; D.v[SBPM] = bpm; D.v[WARP] = (float) warp; D.v[SKEY] = sk; D.v[TUNE] = tu;
    D.v[START] = pts2[(size_t) selSlice]; D.v[END] = pts2[(size_t) selSlice + 1]; D.v[SLICE] = (float) (selSlice + 1); D.v[DECAY] = 4000;
    ed.proc.setSample (p, smp);
    for (auto it = assigned.begin(); it != assigned.end();) { if (it->first == p) it = assigned.erase (it); else ++it; }
    assigned[p] = selSlice;
    pGlow[p] = 1.0f;
    selSlice = juce::jmin (selSlice + 1, (int) pts2.size() - 2);   // passa alla fetta dopo
    chromeDirty = true;
}
void ChopView::tickPads()
{
    bool any = false;
    for (int p = 0; p < kPads; ++p)
    {
        const auto h = ed.proc.hits[p].load();
        if (h != pHits[p]) { pHits[p] = h; pGlow[p] = 1.0f; any = true; }
        else if (pGlow[p] > 0.01f) { pGlow[p] *= 0.82f; any = true; } else pGlow[p] = 0;
    }
    if (any) repaint (padRect (0).getUnion (padRect (15)).expanded (6).toNearestInt());
}

int ChopView::markAt (float x, float tol) const
{
    // marker piu vicino entro la tolleranza: 1.. marker, -1 inizio, -2 fine, 0 niente
    int best = 0; float bd = tol;
    for (size_t k = 0; k < marks.size(); ++k) { const float d = std::abs (xOf (marks[k]) - x); if (d < bd) { bd = d; best = (int) k + 1; } }
    const float ds = std::abs (xOf (r0) - x), de = std::abs (xOf (r1) - x);
    if (ds < bd) { bd = ds; best = -1; }
    if (de < bd) { best = -2; }
    return best;
}
juce::String ChopView::posText (float fx) const
{
    if (! smp) return {};
    const double dur = smp->buf.getNumSamples() / smp->sr, sec = fx * dur;
    juce::String t (sec, 3); t << " s";
    if (bpm > 40)
    {
        const double beats = (fx - r0) * dur * bpm / 60.0; if (beats < -0.001) return t;
        const int bar = (int) std::floor (beats / 4) + 1, bt = (int) std::floor (std::fmod (beats, 4.0)) + 1, sx = (int) std::floor (std::fmod (beats, 1.0) * 4) + 1;
        t = juce::String (bar) + "." + juce::String (bt) + "." + juce::String (sx) + "   " + t;
    }
    return t;
}
void ChopView::mouseMove (const juce::MouseEvent& e)
{
    int h = -1;
    if (wave().contains (e.position)) { const auto pts = points(); const float fx = fxOf (e.position.x); for (size_t k = 0; k + 1 < pts.size(); ++k) if (fx >= pts[k] && fx < pts[k + 1]) h = (int) k; }
    const int hm = wave().expanded (0, 8).contains (e.position) ? markAt (e.position.x, 10) : 0;
    const int hp2 = padAt (e.position);
    if (h != hovSlice || hm != hovMark || hp2 != hovPad) { hovSlice = h; hovMark = hm; hovPad = hp2; repaint(); }
    bool hand = hp2 >= 0; for (auto& h2 : hots) if (h2.r.contains (e.position)) hand = true;
    setMouseCursor (hand ? juce::MouseCursor::PointingHandCursor : hm != 0 ? juce::MouseCursor::LeftRightResizeCursor
                    : wave().contains (e.position) ? (e.mods.isCommandDown() ? juce::MouseCursor::DraggingHandCursor : juce::MouseCursor::PointingHandCursor) : juce::MouseCursor::NormalCursor);
}
void ChopView::mouseDown (const juce::MouseEvent& e)
{
    drag = 0; moved = false; toPad = false; chromeDirty = true;
    if (const int pp = padAt (e.position); pp >= 0) { if (selSlice >= 0) assignTo (pp); ed.playPad (pp, 110); repaint(); return; }
    for (auto it = hots.rbegin(); it != hots.rend(); ++it) if (it->r.contains (e.position) && it->fn) { it->fn (e); repaint(); return; }
    auto w = wave(), ov = overview();
    if (ov.expanded (0, 4).contains (e.position)) { drag = -3; dragOff = (e.position.x - ov.getX()) / ov.getWidth() - v0; mouseDrag (e); return; }
    if (! w.expanded (0, 8).contains (e.position)) return;
    const float fx = fxOf (e.position.x); lastFx = fx;
    const int m = markAt (e.position.x, 10);
    if (m > 0 && e.mods.isPopupMenu()) { pushUndo(); marks.erase (marks.begin() + (long) (m - 1)); selSlice = -1; hovMark = 0; repaint(); return; }
    if (m != 0) { pushUndo(); drag = m; return; }
    // clic su una fetta: la sceglie e la fa sentire; trascinandola si sposta tutta (i due bordi insieme)
    const auto pts = points();
    for (size_t k = 0; k + 1 < pts.size(); ++k)
        if (fx >= pts[k] && fx < pts[k + 1])
        {
            selSlice = (int) k; ed.proc.audition (src, pts[k], pts[k + 1]);
            drag = -4; dragStartFx = fx; dragMarks0 = marks; dragR00 = r0; dragR10 = r1;
        }
    repaint();
}
void ChopView::mouseDrag (const juce::MouseEvent& e)
{
    if (drag == 0) return;
    if (drag == -3) { auto ov = overview(); const float span = v1 - v0; v0 = juce::jlimit (0.0f, 1.0f - span, (e.position.x - ov.getX()) / ov.getWidth() - dragOff); v1 = v0 + span; wavePath.clear(); repaint(); return; }
    // scorre da solo se trascini oltre i bordi con lo zoom attivo
    auto w = wave(); const float span = v1 - v0;
    if (span < 0.999f && (e.position.x < w.getX() || e.position.x > w.getRight()))
    {
        const float d = (e.position.x < w.getX() ? -1.0f : 1.0f) * span * 0.02f;
        v0 = juce::jlimit (0.0f, 1.0f - span, v0 + d); v1 = v0 + span;
    }
    const float raw = juce::jlimit (0.0f, 1.0f, fxOf (e.position.x));
    const float fx = e.mods.isShiftDown() ? raw : juce::jlimit (0.0f, 1.0f, snapTo (raw));   // SHIFT = senza snap
    lastFx = fx;
    const float gap = 0.0005f;
    if (drag == -1) r0 = juce::jlimit (0.0f, (marks.empty() ? r1 : juce::jmin (r1, marks.front())) - gap, fx);
    else if (drag == -2) r1 = juce::jlimit ((marks.empty() ? r0 : juce::jmax (r0, marks.back())) + gap, 1.0f, fx);
    else if (drag > 0 && drag - 1 < (int) marks.size())
    {
        // un marker non scavalca i vicini: l'ordine delle fette resta quello che vedi
        const size_t k = (size_t) drag - 1;
        const float lo = (k > 0 ? marks[k - 1] : r0) + gap, hi = (k + 1 < marks.size() ? marks[k + 1] : r1) - gap;
        if (hi > lo) marks[k] = juce::jlimit (lo, hi, fx);
    }
    else if (drag == -4)
    {
        // trascinata in basso, fuori dall'onda: la fetta va su un pad
        if (e.position.y > w.getBottom() + 30 || toPad)
        {
            if (! toPad && moved) { marks = dragMarks0; r0 = dragR00; r1 = dragR10; }
            toPad = e.position.y > w.getBottom() + 10; dragPos = e.position; hovPad = padAt (e.position);
            repaint(); return;
        }
        if (! moved && std::abs (e.getDistanceFromDragStartX()) < 5) return;
        if (! moved) { undoE.push_back ({ dragMarks0, dragR00, dragR10 }); redoE.clear(); moved = true; }
        // sposta la fetta scelta: i suoi due bordi insieme, dentro i vicini
        marks = dragMarks0; r0 = dragR00; r1 = dragR10;
        const int k = selSlice; const int nm = (int) marks.size();
        float* L = k == 0 ? &r0 : &marks[(size_t) k - 1];
        float* R = k == nm ? &r1 : &marks[(size_t) k];
        const float lo = k == 0 ? 0.0f : (k - 1 == 0 ? r0 : marks[(size_t) k - 2]) + gap;
        const float hi = k == nm ? 1.0f : (k + 1 == nm + 0 ? r1 : marks[(size_t) k + 1]) - gap;
        const float width = *R - *L;
        float nl = *L + (raw - dragStartFx);
        if (! e.mods.isShiftDown()) nl = snapTo (nl);
        nl = juce::jlimit (lo, juce::jmax (lo, hi - width), nl);
        *L = nl; *R = nl + width;
    }
    repaint();
}
void ChopView::mouseUp (const juce::MouseEvent& e)
{
    if (toPad) { toPad = false; drag = 0; moved = false; const int pp = padAt (e.position); if (pp >= 0) { assignTo (pp); ed.playPad (pp, 110); } chromeDirty = true; repaint(); return; }
    if (drag == -4 && moved && selSlice >= 0) { const auto pts = points(); if ((size_t) selSlice + 1 < pts.size()) ed.proc.audition (src, pts[(size_t) selSlice], pts[(size_t) selSlice + 1]); }
    drag = 0; moved = false; chromeDirty = true; repaint();
}
void ChopView::mouseDoubleClick (const juce::MouseEvent& e)
{
    if (! wave().contains (e.position)) return;
    const float fx = e.mods.isShiftDown() ? fxOf (e.position.x) : snapTo (fxOf (e.position.x));
    if (fx > r0 + 0.0005f && fx < r1 - 0.0005f && marks.size() < 63) { pushUndo(); marks.push_back (fx); std::sort (marks.begin(), marks.end()); if (mode != 3) mode = 3; }
    chromeDirty = true; repaint();
}
void ChopView::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wd)
{
    for (auto& h : hots) if (h.r.contains (e.position) && h.wheel) { h.wheel (wd.deltaY); chromeDirty = true; repaint(); return; }
    if (! wave().contains (e.position) && ! overview().contains (e.position)) return;
    const float span = v1 - v0;
    if (e.mods.isShiftDown() || std::abs (wd.deltaX) > std::abs (wd.deltaY))
    {
        const float d = (std::abs (wd.deltaX) > std::abs (wd.deltaY) ? -wd.deltaX : -wd.deltaY) * span * 0.5f;
        v0 = juce::jlimit (0.0f, 1.0f - span, v0 + d); v1 = v0 + span;
    }
    else
    {
        const float c = fxOf (e.position.x), z = wd.deltaY > 0 ? 0.8f : 1.25f;
        const float ns = juce::jlimit (0.002f, 1.0f, span * z);
        v0 = juce::jlimit (0.0f, 1.0f - ns, c - (c - v0) * ns / span); v1 = v0 + ns;
    }
    repaint();
}
bool ChopView::keyPressed (const juce::KeyPress& k)
{
    if (k.getModifiers().isCommandDown() && (k.getKeyCode() == 'Z' || k.getKeyCode() == 'z'))
    {
        auto& from = k.getModifiers().isShiftDown() ? redoE : undoE; auto& to = k.getModifiers().isShiftDown() ? undoE : redoE;
        if (! from.empty()) { to.push_back ({ marks, r0, r1 }); marks = from.back().marks; r0 = from.back().r0; r1 = from.back().r1; from.pop_back(); selSlice = -1; chromeDirty = true; repaint(); }
        return true;
    }
    if (k == juce::KeyPress::escapeKey) { close(); return true; }
    if (k == juce::KeyPress::spaceKey) { ed.proc.audition (src, r0, r1); return true; }
    return false;
}

// ======================= Editor =======================
float Editor::toNorm (int i, float v)
{
    const auto& p = kPInfo[i];
    if (p.kind == 4 || p.kind == 5) { const float mn = juce::jmax (1.0f, p.min); return std::log (juce::jmax (mn, v) / mn) / std::log (p.max / mn); }
    if (p.kind == 13) return std::pow (juce::jlimit (0.0f, 1.0f, v / p.max), 0.5f);
    return juce::jlimit (0.0f, 1.0f, (v - p.min) / (p.max - p.min));
}
float Editor::fromNorm (int i, float n)
{
    const auto& p = kPInfo[i];
    if (p.kind == 4 || p.kind == 5) { const float mn = juce::jmax (1.0f, p.min); float v = mn * std::pow (p.max / mn, n); if (i == ATTACK && n < 0.02f) v = 0; return v; }
    if (p.kind == 13) return n * n * p.max;
    float v = p.min + n * (p.max - p.min);
    if (p.kind == 2 || p.kind == 3 || p.kind == 6 || p.kind == 7 || p.kind == 8) v = std::round (v);
    if (p.kind == 11) v = std::round (v * 2) / 2;
    return v;
}
void Editor::setPadParam (int i, float v)
{
    auto& P = proc.pad[sel];
    if (i == START) v = juce::jmin (v, P.get (END) - 0.002f);
    if (i == END) v = juce::jmax (v, P.get (START) + 0.002f);
    proc.setPadValue (sel, i, juce::jlimit (kPInfo[i].min, kPInfo[i].max, v));
}

Knob* Editor::paramKnob (const char* id, const juce::String& label, juce::Colour c, std::function<juce::String (float)> fmt)
{
    auto* pr = proc.apvts.getParameter (id);
    auto* k = globalKnobs.add (new Knob (label, c));
    k->get = [pr] { return pr->getValue(); };
    k->set = [pr] (float v) { pr->setValueNotifyingHost (v); };
    k->reset = [pr] { pr->setValueNotifyingHost (pr->getDefaultValue()); };
    k->text = [pr, fmt] { return fmt (pr->convertFrom0to1 (pr->getValue())); };
    root.addAndMakeVisible (k); return k;
}

Editor::Editor (Processor& p) : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (root);
    root.addAndMakeVisible (screen); root.addAndMakeVisible (pattern); root.addAndMakeVisible (drag);
    screen.addChildComponent (kitMon); kitMon.setVisible (true); screen.addChildComponent (playhead);
    for (int i = 0; i < kPads; ++i) root.addAndMakeVisible (pads.add (new PadComp (*this, i)));
    for (int i = 0; i < 8; ++i) root.addAndMakeVisible (qlinks.add (new Knob ("", Col::cyan)));

    auto choice = [this] (const char* id, const juce::String& label, juce::Colour c)
    {
        auto* pr = dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter (id));
        auto* s = selectors.add (new Selector (label)); s->col = c;
        s->get = [pr] { return pr->getIndex(); };
        s->set = [pr] (int v) { pr->setValueNotifyingHost (pr->convertTo0to1 ((float) v)); };
        s->count = [pr] { return pr->choices.size(); };
        s->name = [pr] (int v) { return pr->choices[v]; };
        s->setName (id); root.addAndMakeVisible (s); return s;
    };
    choice ("curve", "CURVA VELOCITY", Col::magenta);
    choice ("swGrid", "SWING SU", Col::magenta);
    choice ("nrRate", "RATE", Col::magenta);
    {
        auto* s = selectors.add (new Selector ("STILE")); s->setName ("genre"); s->col = Col::magenta;
        s->get = [this] { return proc.genStyle; };
        s->set = [this] (int v) { proc.genStyle = v; genPreset = 0; runGen (juce::Random::getSystemRandom().nextInt() | 1); };
        s->count = [] { return (int) jp::genStyles().size(); };
        s->name = [] (int v) { return jp::genStyles()[(size_t) v].name.toUpperCase(); };
        s->labelFn = [this] { return juce::String ("STILE  ") + jp::genCatName (jp::genStyles()[(size_t) juce::jlimit (0, (int) jp::genStyles().size() - 1, proc.genStyle)].cat); };
        s->menu = [this] (Selector& sl)
        {
            juce::PopupMenu m;
            for (int c = 0; c < jp::kGenCats; ++c)
            {
                juce::PopupMenu sub; std::vector<int> ids; jp::stylesInCat (c, ids);
                for (int i : ids) sub.addItem (i + 1, jp::genStyles()[(size_t) i].name + "   -   " + jp::genStyles()[(size_t) i].desc, true, i == proc.genStyle);
                m.addSubMenu (jp::genCatName (c), sub, true, nullptr, jp::genStyles()[(size_t) proc.genStyle].cat == c);
            }
            m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&sl), [this, &sl] (int r) { if (r > 0) { sl.set (r - 1); sl.repaint(); } });
        };
        root.addAndMakeVisible (s);
    }
    {
        auto* s = selectors.add (new Selector ("PRESET")); s->setName ("pattern"); s->col = Col::cyan;
        auto apply = [this] (int v)
        {
            const auto ps = jp::genPresets (proc.genStyle); if (ps.empty()) return;
            genPreset = juce::jlimit (0, (int) ps.size() - 1, v); const auto& pr = ps[(size_t) genPreset];
            if (pr.bankIndex >= 0) { jp::loadBankByRoles (proc, pr.bankIndex); proc.loopOn = true; pattern.info = pr.name; pattern.infoT = juce::Time::getMillisecondCounter(); pattern.repaint(); }
            else runGen (pr.seed);
        };
        s->get = [this] { return genPreset; };
        s->set = apply;
        s->count = [this] { return juce::jmax (1, (int) jp::genPresets (proc.genStyle).size()); };
        s->name = [this] (int v) { const auto ps = jp::genPresets (proc.genStyle); return v < (int) ps.size() ? ps[(size_t) v].name.toUpperCase() : juce::String ("-"); };
        s->menu = [this, apply] (Selector& sl)
        {
            juce::PopupMenu m; const auto ps = jp::genPresets (proc.genStyle);
            m.addSectionHeader (jp::genStyles()[(size_t) proc.genStyle].name);
            for (int i = 0; i < (int) ps.size(); ++i) m.addItem (i + 1, ps[(size_t) i].name, true, i == genPreset);
            m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&sl), [apply, &sl] (int r) { if (r > 0) { apply (r - 1); sl.repaint(); } });
        };
        root.addAndMakeVisible (s);
    }
    {
        auto* pr = proc.apvts.getParameter ("root");
        auto* s = selectors.add (new Selector ("NOTA PAD 1")); s->setName ("root");
        s->get = [pr] { return juce::roundToInt (pr->convertFrom0to1 (pr->getValue())); };
        s->set = [this, pr] (int v) { pr->setValueNotifyingHost (pr->convertTo0to1 ((float) v)); for (auto* pd : pads) pd->repaint(); };
        s->count = [] { return 113; };
        s->name = [] (int v) { return noteName (v); };
        root.addAndMakeVisible (s);
    }
    paramKnob ("swing", "SWING", Col::magenta, [] (float v) { return juce::String (v, 1) + "%"; });
    paramKnob ("human", "HUMANIZE", Col::cyan, [] (float v) { return juce::String (juce::roundToInt (v * 100)) + "%"; });
    paramKnob ("bpm", "BPM INT", Col::amber, [] (float v) { return juce::String (v, 1); });
    paramKnob ("out", "OUTPUT", Col::cyan, [] (float v) { return juce::String (v, 1) + " dB"; });

    auto btn = [this] (const juce::String& t, juce::Colour c, std::function<bool()> lit, std::function<void()> click)
    { auto* b = btns.add (new Btn (t, c)); b->lit = std::move (lit); b->click = std::move (click); root.addAndMakeVisible (b); return b; };
    btn ("LOOP", Col::magenta, [this] { return proc.loopOn.load(); }, [this] { proc.loopOn = ! proc.loopOn.load(); })->setName ("loop");
    btn ("GENERA", Col::amber, [] { return false; }, [this] { runGen (juce::Random::getSystemRandom().nextInt() | 1); })->setName ("random");
    btn ("RANDOM", Col::magenta, [] { return false; }, [this]
    {   // stile a caso (quasi sempre batteria) e un groove nuovo
        auto& r = juce::Random::getSystemRandom(); const int c = r.nextFloat() < 0.7f ? jp::GC_DRUM : (r.nextBool() ? jp::GC_MPC : jp::GC_PROG);
        std::vector<int> ids; jp::stylesInCat (c, ids); proc.genStyle = ids[(size_t) r.nextInt ((int) ids.size())]; genPreset = 0;
        runGen (r.nextInt() | 1); for (auto* sl : selectors) sl->repaint();
    })->setName ("rnd");
    btn ("VARIA", Col::amber, [] { return false; }, [this] { proc.varyPattern(); })->setName ("vary");
    btn ("PLAY", Col::green, [this] { return proc.uiRunning.load(); }, [this] { proc.requestPlay (! proc.intPlay.load()); })->setName ("play");
    btn ("SWING DAW", Col::magenta, [this] { return proc.apvts.getRawParameterValue ("swMidi")->load() > 0.5f; },
         [this] { auto* pr = proc.apvts.getParameter ("swMidi"); pr->setValueNotifyingHost (pr->getValue() > 0.5f ? 0.0f : 1.0f); })->setName ("swmidi");

    btn ("NOTE REPEAT", Col::magenta, [this] { return proc.apvts.getRawParameterValue ("nrOn")->load() > 0.5f; },
         [this] { auto* pr = proc.apvts.getParameter ("nrOn"); pr->setValueNotifyingHost (pr->getValue() > 0.5f ? 0.0f : 1.0f); })->setName ("nr");
    btn ("FULL LEVEL", Col::magenta, [this] { return fullLevel; }, [this] { fullLevel = ! fullLevel; root.repaint(); })->setName ("full");
    btn ("LOAD", Col::cyan, [] { return false; }, [this] { loadDialog (sel); })->setName ("load");

    btn ("STOP", Col::red, [] { return false; }, [this] { proc.stopAll(); })->setName ("stop");
    btn ("FX", Col::magenta, [this] { return proc.qFx.load(); }, [this] { proc.qFx = ! proc.qFx.load(); bindQlinks(); })->setName ("qfx");
    btn (juce::String::fromUTF8 ("\xe2\x86\xb6 ANNULLA"), Col::cyan, [this] { return ! proc.undoStack.empty(); }, [this] { doUndo (false); })->setName ("undo");
    btn (juce::String::fromUTF8 ("RIFAI \xe2\x86\xb7"), Col::cyan, [this] { return ! proc.redoStack.empty(); }, [this] { doUndo (true); })->setName ("redo");
    root.addChildComponent (chop);
    bindQlinks();
    setWantsKeyboardFocus (true);   // cmd+Z / cmd+shift+Z quando il plugin e in primo piano; gli altri tasti passano ad Ableton
    proc.commitIfChanged();
    root.addAndMakeVisible (sizeChip);
    scaler.setup();
    startTimerHz (30);
}

void Editor::runGen (juce::uint32 seed)
{
    proc.genSeed = seed;
    pattern.info = jp::generate (proc, proc.genStyle, seed, sel); pattern.infoT = juce::Time::getMillisecondCounter();
    proc.loopOn = true; pattern.repaint(); for (auto* pd : pads) pd->repaint();
}

static juce::Component* named (juce::Component& parent, const char* n) { for (auto* c : parent.getChildren()) if (c->getName() == n) return c; return nullptr; }

void Editor::resized()
{
    const float s = scaler.update();
    sizeChip.setBounds (W - 76, H - 18, 58, 15);
    root.setBounds (0, 0, W, H); root.setTransform (juce::AffineTransform::scale (s));
    screen.setBounds (24, 78, 928, 450);
    chop.setBounds (0, 0, W, H);
    named (root, "stop")->setBounds (W - 128, 18, 100, 30);
    named (root, "undo")->setBounds (1006, 18, 108, 30);
    named (root, "redo")->setBounds (1120, 18, 108, 30);
    for (int i = 0; i < 8; ++i) qlinks[i]->setBounds (66 + i * 104, 540, 88, 88);
    named (root, "qfx")->setBounds (898, 550, 46, 68);
    for (int i = 0; i < kPads; ++i) { const int r = 3 - i / 4, c = i % 4; pads[i]->setBounds (969 + c * 100, 86 + r * 100, 100, 100); }
    named (root, "curve")->setBounds (962, 512, 140, 40);
    named (root, "full")->setBounds (1106, 522, 86, 30);
    named (root, "load")->setBounds (1196, 522, 56, 30);
    named (root, "root")->setBounds (1256, 512, 120, 40);
    named (root, "nr")->setBounds (962, 572, 130, 34);
    named (root, "nrRate")->setBounds (1100, 566, 96, 44);
    globalKnobs[3]->setBounds (1210, 616, 92, 84);
    named (root, "genre")->setBounds (646, 648, 146, 40);
    named (root, "pattern")->setBounds (798, 648, 148, 40);
    const char* tb[] = { "loop", "random", "rnd", "vary", "play" };
    for (int i = 0; i < 5; ++i) named (root, tb[i])->setBounds (646 + i * 60, 692, 57, 30);
    named (root, "swGrid")->setBounds (820, 730, 60, 40);
    named (root, "swmidi")->setBounds (884, 734, 62, 32);
    pattern.setBounds (30, 642, 610, 178);
    globalKnobs[0]->setBounds (664, 726, 62, 70); globalKnobs[1]->setBounds (744, 726, 62, 70); globalKnobs[2]->setVisible (false);   // BPM interno: in Ableton comanda il tempo della DAW
    drag.setBounds (884, 772, 62, 40);
}

// le 8 manopole Q-link cambiano con la pagina, come sulle MPC
void Editor::bindQlinks()
{
    struct Q { int pad; const char* global; const char* label; };
    static const std::vector<std::vector<Q>> map = {
        { { GAIN }, { LEVEL }, { PAN }, { TUNE }, { DECAY }, { CUTOFF }, { SREV }, { SDLY } },
        { { GAIN }, { LEVEL }, { PAN }, { TUNE }, { DECAY }, { CUTOFF }, { SREV }, { SDLY } },
        { { GAIN }, { LEVEL }, { PAN }, { SREV }, { SDLY }, { DRIVE }, { -1 }, { -1 } },
        { { START }, { END }, { TUNE }, { FINE }, { ATTACK }, { DECAY }, { RELEASE }, { GAIN } },
        { { VLEVEL }, { VFILTER }, { VSTART }, { VPITCH }, { GAIN }, { LEVEL }, { -1 }, { -1 } },
        { { B3F }, { B3G }, { B3Q }, { B3T }, { CUTOFF }, { RESO }, { FENV }, { FDECAY } },   // EQ: la riga si riscrive sulla banda scelta
        { { CAMT }, { CMAKEUP }, { CMIX }, { GAIN }, { LEVEL }, { -1 }, { -1 }, { -1 } },
        { { DTYPE }, { DRIVE }, { BITS }, { RATE }, { SREV }, { SDLY }, { PAN }, { LEVEL } },
        { { -1, "revKit", "REV KIT" }, { -1, "revSize", "REV CODA" }, { -1, "dlyKit", "DLY KIT" }, { -1, "dlyFb", "DLY FB" },
          { -1, "satAmt", "SATURA" }, { -1, "busAmt", "BUS AMT" }, { -1, "revRet", "REV RET" }, { -1, "out", "OUTPUT" } } };
    static const std::vector<Q> fxMap = { { -1, "fxFilter", "FILTRO" }, { -1, "revKit", "REVERB" }, { -1, "dlyKit", "DELAY" }, { -1, "satAmt", "SATURA" },
                                          { -1, "fxScratch", "SCRATCH" }, { -1, "fxStop", "TAPE STOP" }, { -1, "fxStut", "STUTTER" }, { -1, "fxLofi", "LO-FI" } };
    const bool fx = proc.qFx.load();
    auto m = fx ? fxMap : map[(size_t) page];
    if (! fx && page == PG_EQ) for (int k = 0; k < 4; ++k) m[(size_t) k].pad = bandP (screen.eqBand, k == 0 ? 2 : k == 1 ? 3 : k == 2 ? 4 : 1);
    for (int k = 0; k < 8; ++k)
    {
        auto* q = qlinks[k]; const auto& e = m[(size_t) k];
        if (e.pad >= 0)
        {
            const int i = e.pad;
            q->label = kPInfo[i].label; q->bipolar = kPInfo[i].min < 0;
            if (i >= B1ON && i <= B6Q) { static const char* bl[] = { "", "TIPO", "FREQ", "GAIN", "Q" }; q->label = juce::String (bl[(i - B1ON) % 5]) + " " + juce::String ((i - B1ON) / 5 + 1); }
            if (i == CUTOFF) q->label = "FILTRO"; if (i == RESO) q->label = "RISONANZA"; if (i == FENV) q->label = "ENV"; if (i == FDECAY) q->label = "ENV DECAY";
            q->col = (i == VLEVEL || i == VFILTER || i == VSTART || i == VPITCH || i == SREV || i == SDLY) ? Col::magenta : (i == GAIN || i == LEVEL ? Col::amber : Col::cyan);
            q->get = [this, i] { return toNorm (i, proc.pad[sel].get (i)); };
            q->set = [this, i] (float n) { setPadParam (i, fromNorm (i, n)); screen.repaint(); };
            q->reset = [this, i] { setPadParam (i, kPInfo[i].def); screen.repaint(); };
            q->text = [this, i] { return formatPadParam (i, proc.pad[sel].get (i)); };
            q->gesture = [this, i] (bool b) { if (auto* h = proc.hostParam (sel, i)) { if (b) h->beginChangeGesture(); else h->endChangeGesture(); } };
            q->popup = nullptr;
            q->setVisible (true);
        }
        else if (e.global != nullptr)
        {
            auto* pr = proc.apvts.getParameter (e.global);
            const juce::String gid (e.global);
            q->label = e.label; q->bipolar = gid == "fxFilter" || gid == "fxGain" || gid == "fxScratch"; q->col = fx ? (k < 4 ? Col::magenta : Col::amber) : Col::cyan;
            if (fx)
                q->popup = [this, k, q]
                {   // MIDI learn: qualsiasi controller (MPK, KeyLab, fader o manopole)
                    juce::PopupMenu m; const int cc = proc.ccMap[k].load();
                    m.addSectionHeader (q->label + (cc >= 0 ? "  -  CC " + juce::String (cc) : juce::String ("  -  nessun CC")));
                    m.addItem (1, "MIDI LEARN: muovi una manopola o un fader del controller");
                    m.addItem (2, "Scollega");
                    m.addItem (3, "Tutte e 8 come di fabbrica (CC 70-77, Akai MPK)");
                    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (q), [this, k] (int r)
                    {
                        if (r == 1) proc.learnSlot = k;
                        else if (r == 2) proc.ccMap[k] = -1;
                        else if (r == 3) for (int j = 0; j < Processor::kFx; ++j) proc.ccMap[j] = 70 + j;
                    });
                };
            else q->popup = nullptr;
            q->get = [pr] { return pr->getValue(); };
            q->set = [this, pr] (float v) { pr->setValueNotifyingHost (v); screen.repaint(); };
            q->reset = [pr] { pr->setValueNotifyingHost (pr->getDefaultValue()); };
            q->gesture = [pr] (bool b) { if (b) pr->beginChangeGesture(); else pr->endChangeGesture(); };
            const juce::String id (e.global);
            q->text = [pr, id] {
                const float v = pr->convertFrom0to1 (pr->getValue());
                if (id == "out" || id == "fxGain") return (v > 0 ? "+" : "") + juce::String (v, 1) + " dB";
                if (id == "fxFilter") return std::abs (v) <= 0.02f ? juce::String ("OFF") : v < 0 ? "LP " + juce::String (juce::roundToInt (-v * 100)) + "%" : "HP " + juce::String (juce::roundToInt (v * 100)) + "%";
                if (id == "fxScratch") return juce::String ("MUOVI");
                if (id == "fxStut") return v <= 0.01f ? juce::String ("OFF") : v < 0.34f ? juce::String ("1/8") : v < 0.67f ? juce::String ("1/16") : juce::String ("1/32");
                if ((id == "fxStop" || id == "fxLofi") && v <= 0.01f) return juce::String ("OFF");
                return juce::String (juce::roundToInt (pr->getValue() * 100)) + "%"; };
            q->setVisible (true);
        }
        else { q->setVisible (false); q->gesture = nullptr; }
        q->shown = -1.0f; q->repaint();
    }
}

void Editor::setPage (int pg) { page = juce::jlimit (0, kNumPages - 1, pg); kitMon.setVisible (page == PG_KIT); if (page != PG_KIT && page != PG_SAMPLE) playhead.setVisible (false); if (page != PG_EQ) { proc.eqSoloPad = -1; proc.eqSoloBand = -1; } screen.hint.clear(); bindQlinks(); screen.repaint(); }
void Editor::selectPad (int p) { if (p != sel) { screen.pendKey.clear(); proc.eqSoloPad = -1; proc.eqSoloBand = -1; } sel = juce::jlimit (0, kPads - 1, p); root.repaint(); }
void Editor::doUndo (bool redo)
{
    if (chop.isVisible()) return;
    if (redo ? proc.redo() : proc.undo()) { screen.chopSmp = nullptr; screen.wfSmp = nullptr; kitChanged(); }
    root.repaint();
}
void Editor::keyLockMenu()
{
    juce::PopupMenu m, ma, mi; const int cur = proc.keyLock();
    for (int i = 0; i < 12; ++i) { ma.addItem (2 + i, keyName (i) + " maggiore", true, cur == i); mi.addItem (14 + i, keyName (12 + i).dropLastCharacters (1) + " minore", true, cur == 12 + i); }
    m.addItem (1, "LIBERA (non forzare la tonalita)", true, cur < 0);
    m.addSubMenu ("Maggiore", ma); m.addSubMenu ("Minore", mi);
    m.addSeparator();
    m.addItem (40, "Usa la tonalita del kit attuale", proc.kitEffectiveKey() >= 0);
    m.showMenuAsync (juce::PopupMenu::Options(), [this] (int r)
    {
        if (r <= 0) return;
        int k = r == 1 ? -1 : r == 40 ? proc.kitEffectiveKey() : r - 2;
        auto* pr = proc.apvts.getParameter ("kkey"); pr->setValueNotifyingHost (pr->convertTo0to1 ((float) (k + 1)));
        if (k >= 0) proc.applyKeyLockAll();
        kitChanged();
    });
}
void Editor::kitChanged() { for (auto* pd : pads) pd->repaint(); root.repaint(); screen.repaint(); }
void Editor::openBrowser (int p)
{
    selectPad (p);
    if (auto s = proc.getSample (p)) { DrumDesc d; if (pathToDesc (s->path, d)) screen.browseCat = juce::jmax (0, libCategories().indexOf (categoryOf (d))); }
    screen.browseScroll = 0; screen.browseDirty = true; setPage (PG_BROWSE);
}
void Editor::scanMenu()
{
    auto& idx = *proc.sampleIndex;
    auto pick = [this]
    {
        chooser = std::make_unique<juce::FileChooser> ("Scegli una cartella di campioni da scansionare", juce::File::getSpecialLocation (juce::File::userMusicDirectory));
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories, [this] (const juce::FileChooser& fc)
        {
            const auto d = fc.getResult(); if (! d.isDirectory()) return;
            proc.sampleIndex->addRoot (d); screen.browseDirty = true; screen.repaint();
        });
    };
    const auto roots = idx.roots();
    if (roots.isEmpty() && ! idx.scanning()) { pick(); return; }
    juce::PopupMenu m, rm; m.addSectionHeader ("SCAN DEI CAMPIONI");
    m.addItem (1, "Aggiungi una cartella da scansionare...");
    m.addItem (2, "Aggiorna (analizza solo i file nuovi)", ! roots.isEmpty() && ! idx.scanning());
    if (idx.scanning()) m.addItem (3, "Ferma la scansione");
    for (int i = 0; i < roots.size(); ++i) rm.addItem (100 + i, roots[i]);
    m.addSubMenu ("Togli una cartella dall'indice", rm, ! roots.isEmpty());
    m.addSeparator();
    m.addItem (4, "Svuota tutto l'indice", idx.size() > 0 || ! roots.isEmpty());
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&screen), [this, pick, roots] (int r)
    {
        auto& ix = *proc.sampleIndex;
        if (r == 1) pick();
        else if (r == 2) ix.startScan();
        else if (r == 3) ix.cancel();
        else if (r == 4) ix.clearAll();
        else if (r >= 100 && r - 100 < roots.size()) ix.removeRoot (roots[r - 100]);
        screen.browseDirty = true; screen.repaint();
    });
}
void Editor::addToLibraryDialog()
{
    chooser = std::make_unique<juce::FileChooser> ("Aggiungi suoni alla categoria " + libCategories()[screen.browseCat], juce::File(), proc.formats.getWildcardForAllFormats());
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::canSelectMultipleItems,
                          [this] (const juce::FileChooser& fc)
                          {
                              bool first = true;
                              for (auto& f : fc.getResults()) if (addFileToLibrary (f, libCategories()[screen.browseCat]) && first) { proc.loadFile (f, sel); first = false; }
                              screen.browseDirty = true; kitChanged();
                          });
}

void Editor::saveKit (bool askName)
{
    if (! askName && proc.kitFile.existsAsFile()) { proc.saveKit (proc.kitFile, proc.kitName); kitChanged(); return; }
    auto* w = new juce::AlertWindow ("Salva kit", "Nome del kit (finisce in Musica/Jackpot Sampler/Kits)", juce::MessageBoxIconType::NoIcon, this);
    w->addTextEditor ("name", proc.kitFile.existsAsFile() ? proc.kitName : proc.kitName + " (mio)");
    w->addButton ("Salva", 1, juce::KeyPress (juce::KeyPress::returnKey)); w->addButton ("Annulla", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    w->enterModalState (true, juce::ModalCallbackFunction::create ([this, w] (int r)
    {
        if (r != 1) return;
        const auto nm = w->getTextEditorContents ("name").trim(); if (nm.isEmpty()) return;
        const auto f = Processor::kitFolder().getChildFile (juce::File::createLegalFileName (nm) + ".jpkit");
        proc.saveKit (f, nm); kitChanged();
    }), true);
}
void Editor::loadKitDialog()
{
    chooser = std::make_unique<juce::FileChooser> ("Apri kit", Processor::kitFolder(), "*.jpkit");
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [this] (const juce::FileChooser& fc) { const auto f = fc.getResult(); if (f.existsAsFile() && proc.loadKit (f)) kitChanged(); });
}
void Editor::loadDialog (int p)
{
    chooser = std::make_unique<juce::FileChooser> ("Campione per il pad " + juce::String (p + 1), juce::File(), proc.formats.getWildcardForAllFormats());
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [this, p] (const juce::FileChooser& fc) { const auto f = fc.getResult(); if (f.existsAsFile()) { proc.loadFile (f, p); selectPad (p); setPage (PG_SAMPLE); } });
}
void Editor::padMenu (int p)
{
    juce::PopupMenu m, cm;
    m.addItem (7, "Scegli suono dalla banca...");
    m.addItem (8, p < 8 ? "Random: altro suono della stessa categoria" : "Random: altro campione (di solito lungo)");
    m.addItem (1, "Carica campione da file...");
    m.addItem (2, "Svuota pad");
    m.addItem (4, "Ripristina parametri del pad");
    m.addItem (6, "Rileva BPM del campione");
    static const char* names[] = { "Magenta", "Arancio", "Ambra", "Giallo", "Lime", "Menta", "Ciano", "Blu", "Viola", "Fucsia", "Rosa", "Mandarino", "Acqua", "Acido", "Rosso", "Lavanda" };
    for (int i = 0; i < 16; ++i) cm.addColouredItem (100 + i, names[i], padColour (i), true, proc.pad[p].colour.load() == i);
    m.addSubMenu ("Colore", cm);
    m.showMenuAsync (juce::PopupMenu::Options(), [this, p] (int r)
    {
        if (r == 1) loadDialog (p);
        else if (r == 2) proc.clearPad (p);
        else if (r == 4) { const int c = proc.pad[p].colour.load(); proc.pad[p].reset(); proc.pad[p].colour = c; }
        else if (r == 6) { proc.detectBpm (p); setPage (PG_SAMPLE); }
        else if (r == 7) openBrowser (p);
        else if (r == 8) { randomPad (proc, p, screen.randMode); kitChanged(); playPad (p, 100); }
        else if (r >= 100) proc.pad[p].colour = r - 100;
        root.repaint();
    });
}

void Editor::timerCallback()
{
    // istantanea per annulla/ripeti quando non si sta trascinando
    if (++undoTick % 8 == 0 && ! juce::ModifierKeys::currentModifiers.isAnyMouseButtonDown() && ! chop.isVisible()) proc.commitIfChanged();
    if (chop.isVisible()) { chop.tickPads(); return; }   // l'editor grande copre tutto: niente ridisegni sotto
    for (int i = 0; i < kPads; ++i)
    {
        auto* pc = pads[i]; const auto h = proc.hits[i].load();
        if (h != pc->lastHit) { pc->lastHit = h; pc->flash = 0.35f + 0.65f * proc.lastVel[i].load() / 127.0f; }
        const float lev = proc.padGlow[i].exchange (0.0f);
        const float target = juce::jlimit (0.0f, 1.0f, std::sqrt (lev) * 1.4f);
        pc->glow = juce::jmax (target, pc->glow * 0.86f); pc->flash *= 0.8f;
        if (pc->glow > 0.003f || pc->flash > 0.003f || pc->held) pc->repaint(); else if (pc->glow > 0) { pc->glow = 0; pc->repaint(); }
    }
    // ridisegna solo cio che cambia: lo schermo se cambiano i valori o se la pagina ha misure dal vivo
    {
        juce::uint64 h = (juce::uint64) page * 1315423911ull + (juce::uint64) sel * 2654435761ull;
        auto mix = [&h] (float v) { juce::uint32 u; std::memcpy (&u, &v, 4); h = (h ^ u) * 1099511628211ull; };
        for (int i = 0; i < kNumPadParams; ++i) mix (proc.pad[sel].get (i));
        mix ((float) (juce::pointer_sized_int) proc.getSample (sel).get()); mix ((float) proc.kitName.hashCode());
        for (auto id : { "revSize", "revDamp", "revRet", "dlyTime", "dlyFb", "dlyTone", "dlyRet", "busMode", "busAmt", "clip", "out", "revKit", "dlyKit", "satAmt", "curve" }) mix (proc.apvts.getRawParameterValue (id)->load());
        if (page != PG_KIT && kitMon.isVisible()) kitMon.setVisible (false);
        if (kitMon.isVisible()) kitMon.tick();
        if (playhead.isVisible()) playhead.tick();
        const bool live = page == PG_MIX || page == PG_COMP || page == PG_MASTER || (page == PG_BROWSE && proc.sampleIndex->scanning());
        if (h != screen.sig || (live && (undoTick % 3) == 0)) { screen.sig = h; screen.repaint(); }
    }
    if (proc.uiRunning.load() || (undoTick % 10) == 0) pattern.repaint();
    for (auto* b : btns) { const int l = b->lit && b->lit() ? 1 : 0; if (l != b->shown) { b->shown = l; b->repaint(); } }
    for (auto* k : qlinks) if (k->isVisible() && k->get) { const float v = k->get(); if (v != k->shown) { k->shown = v; k->repaint(); } }
    for (auto* k : globalKnobs) if (k->get) { const float v = k->get(); if (v != k->shown) { k->shown = v; k->repaint(); } }
    for (auto* sl : selectors) if (sl->get) { const int v = sl->get(); if (v != sl->shown) { sl->shown = v; sl->repaint(); } }
    root.repaint (1206, 708, 158, 22);
}

static int keyNoteOffset (juce::juce_wchar c)
{
    static const char* letters = "awsedftgyhujkolp;";
    for (int i = 0; letters[i]; ++i) if (letters[i] == c) return i;
    return -1;
}
bool Editor::keyPressed (const juce::KeyPress& k)
{
    if (k.getModifiers().isCommandDown() && (k.getKeyCode() == 'Z' || k.getKeyCode() == 'z')) { doUndo (k.getModifiers().isShiftDown()); return true; }
    if (k.getModifiers().isCommandDown() && (k.getKeyCode() == 'Y' || k.getKeyCode() == 'y')) { doUndo (true); return true; }
    if (! kbOn) return false;
    const auto c = juce::CharacterFunctions::toLowerCase ((juce::juce_wchar) k.getKeyCode());
    static const int vels[] = { 1, 20, 40, 60, 80, 100, 120, 127 };
    auto velStep = [this] (int d) { int i = 0; while (i < 7 && vels[i] < kbVel) ++i; kbVel = vels[juce::jlimit (0, 7, i + d)]; root.repaint(); };
    if (c == 'z') { octave = juce::jmax (-2, octave - 1); root.repaint(); return true; }
    if (c == 'x') { octave = juce::jmin (7, octave + 1); root.repaint(); return true; }
    if (c == 'c') { velStep (-1); return true; }
    if (c == 'v') { velStep (1); return true; }
    return keyNoteOffset (c) >= 0;
}
bool Editor::keyStateChanged (bool)
{
    if (! kbOn) return false;
    static const char* letters = "awsedftgyhujkolp;";
    bool used = false; const int root0 = proc.rootNote();
    for (int i = 0; letters[i]; ++i)
    {
        const int code = letters[i];
        const bool d = juce::KeyPress::isKeyCurrentlyDown (code) || juce::KeyPress::isKeyCurrentlyDown (juce::CharacterFunctions::toUpperCase ((juce::juce_wchar) code));
        const int p = 12 * (octave + 2) + i - root0;
        if (d && ! keysDown.count (i)) { keysDown.insert (i); used = true; if (p >= 0 && p < kPads) { playPad (p, fullLevel ? 127 : kbVel); selectPad (p); } }
        else if (! d && keysDown.count (i)) { keysDown.erase (i); used = true; if (p >= 0 && p < kPads) stopPad (p); }
    }
    return used;
}
}
