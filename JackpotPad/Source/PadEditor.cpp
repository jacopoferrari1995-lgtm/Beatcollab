#include "PadEditor.h"

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
    g.setFont (f);
    for (int k = 3; k >= 1; --k) { g.setColour (c.withAlpha (0.10f)); g.drawText (t, r.translated ((float) -k * 0.5f, 0), j); g.drawText (t, r.translated ((float) k * 0.5f, 0), j); g.drawText (t, r.translated (0, (float) k * 0.5f), j); }
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
    if (label.isNotEmpty()) { g.setFont (ft (9.0f, true)); g.setColour (Col::dim); g.drawText (label, b.removeFromTop (13), juce::Justification::centredLeft); }
    insetPanel (g, b, 4);
    juce::ColourGradient cg (Col::lcd1, 0, b.getY(), Col::lcd2, 0, b.getBottom(), false); g.setGradientFill (cg); g.fillRoundedRectangle (b.reduced (1), 4);
    glowText (g, name (get()), b.reduced (6, 0), col, ft (11.5f, true, true), juce::Justification::centred);
    g.setColour (col.withAlpha (0.4f)); g.setFont (ft (8.0f));
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
    juce::Random rr (pad * 97 + 5); g.setColour (juce::Colours::black.withAlpha (0.12f));
    for (int i = 0; i < 140; ++i) g.fillEllipse (b.getX() + rr.nextFloat() * b.getWidth(), b.getY() + rr.nextFloat() * b.getHeight(), 1.3f, 1.3f);
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
    g.setFont (ft (11.0f, true, true)); g.setColour (lum > 0.4f ? juce::Colours::white : Col::text.withAlpha (0.85f));
    g.drawText (juce::String (pad + 1).paddedLeft ('0', 2), tb, juce::Justification::topLeft);
    g.setFont (ft (9.5f, false, true)); g.setColour (c.withAlpha (0.85f)); g.drawText (noteName (root + pad), tb, juce::Justification::topRight);
    auto s = P.getSample (pad);
    g.setFont (ft (10.0f)); g.setColour (lum > 0.4f ? juce::Colours::white : Col::dim);
    const int sl = juce::roundToInt (P.pad[pad].get (SLICE));
    g.drawFittedText (s ? s->name + (sl > 0 ? " #" + juce::String (sl) : juce::String()) : juce::String ("vuoto"), tb.withTrimmedBottom (4).toNearestInt(), juce::Justification::bottomLeft, 1);
    if (flash > 0.05f) { g.setFont (ft (9.0f, true, true)); g.setColour (juce::Colours::white.withAlpha (flash)); g.drawText (juce::String (P.lastVel[pad].load()), tb.withTrimmedBottom (4), juce::Justification::bottomRight); }
}
void PadComp::mouseDown (const juce::MouseEvent& e)
{
    ed.selectPad (pad);
    if (e.mods.isPopupMenu()) { ed.padMenu (pad); return; }
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
static const char* kPageNames[kNumPages] = { "KIT", "SUONI", "MIXER", "SAMPLE", "VELOCITY", "EQ", "FILTRO", "COMP", "FX", "MASTER" };

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
    auto b = getLocalBounds().toFloat();
    insetPanel (g, b, 10);
    auto s = b.reduced (6);
    juce::ColourGradient bg (Col::lcd1, s.getCentreX(), s.getCentreY(), Col::lcd2, s.getX(), s.getY(), true);
    g.setGradientFill (bg); g.fillRoundedRectangle (s, 6);
    g.setColour (Col::phos.withAlpha (0.03f));
    for (float x = s.getX(); x < s.getRight(); x += 12) g.drawVerticalLine ((int) x, s.getY(), s.getBottom());
    for (float y = s.getY(); y < s.getBottom(); y += 12) g.drawHorizontalLine ((int) y, s.getX(), s.getRight());
    // schede
    const float tw = (s.getWidth() - 12) / kNumPages;
    for (int i = 0; i < kNumPages; ++i)
    {
        auto r = juce::Rectangle<float> (s.getX() + 6 + i * tw, s.getY() + 6, tw - 3, 22);
        const bool on = ed.page == i;
        g.setColour (on ? Col::phos : Col::phos.withAlpha (0.06f)); g.fillRoundedRectangle (r, 3);
        if (on) { g.setColour (Col::phos.withAlpha (0.25f)); g.fillRoundedRectangle (r.expanded (2), 4); g.setColour (Col::phos); g.fillRoundedRectangle (r, 3); }
        g.setFont (ft (9.5f, true, true)); g.setColour (on ? Col::lcd2 : Col::phos.withAlpha (0.7f)); g.drawText (kPageNames[i], r, juce::Justification::centred);
        hots.push_back ({ r, [this, i] (const juce::MouseEvent&) { ed.setPage (i); }, nullptr, nullptr, nullptr });
    }
    auto hdr = juce::Rectangle<float> (s.getX() + 10, s.getY() + 34, s.getWidth() - 20, 20);
    auto c = content();
    switch (ed.page)
    {
        case PG_KIT:
        {
            glowText (g, "KIT", hdr, Col::phos.withAlpha (0.6f), ft (11.0f, true, true));
            juce::String rt; rt << juce::String (ed.proc.uiBpm.load(), 1) << " BPM  " << (ed.proc.uiHost.load() ? "HOST" : ed.proc.uiRunning.load() ? "INT" : "STOP");
            glowText (g, rt, hdr, Col::phos.withAlpha (0.8f), ft (10.5f, false, true), juce::Justification::centredRight);
            pageKit (g, c); break;
        }
        case PG_MASTER: glowText (g, juce::String::fromUTF8 ("MASTER  \xc2\xb7  REVERB  \xc2\xb7  DELAY  \xc2\xb7  BUS"), hdr, Col::phos, ft (12.0f, true, true)); pageMaster (g, c); break;
        case PG_BROWSE: padHeader (g, hdr); pageBrowse (g, c); break;
        case PG_MIX: glowText (g, juce::String::fromUTF8 ("MIXER  \xc2\xb7  GAIN  \xc2\xb7  PAN  \xc2\xb7  VOLUME  \xc2\xb7  MUTE / SOLO"), hdr, Col::phos, ft (12.0f, true, true)); pageMix (g, c); break;
        default:
            padHeader (g, hdr);
            if (ed.page == PG_SAMPLE) pageSample (g, c);
            else if (ed.page == PG_VEL) pageVel (g, c);
            else if (ed.page == PG_EQ) pageEq (g, c);
            else if (ed.page == PG_FILTER) pageFilter (g, c);
            else if (ed.page == PG_COMP) pageComp (g, c);
            else if (ed.page == PG_FX) pageFx (g, c);
    }
    g.setColour (juce::Colours::black.withAlpha (0.2f)); for (float y = s.getY(); y < s.getBottom(); y += 3) g.drawHorizontalLine ((int) y, s.getX(), s.getRight());
    juce::ColourGradient gl (juce::Colours::white.withAlpha (0.05f), s.getX(), s.getY(), juce::Colours::transparentWhite, s.getX() + s.getWidth() * 0.35f, s.getY() + s.getHeight() * 0.6f, false);
    g.setGradientFill (gl); g.fillRoundedRectangle (s, 6);
}

// ---------- SUONI (banca per categoria) ----------
void Screen::pageBrowse (juce::Graphics& g, juce::Rectangle<float> c)
{
    auto& P = ed.proc; const auto& cats = libCategories();
    if (browseDirty) { browseItems = libItems (cats[browseCat]); browseDirty = false; }
    auto bottom = c.removeFromBottom (30); c.removeFromBottom (6);
    auto left = c.removeFromLeft (130); c.removeFromLeft (10);
    const float ch = left.getHeight() / cats.size();
    for (int i = 0; i < cats.size(); ++i)
    {
        auto r = juce::Rectangle<float> (left.getX(), left.getY() + i * ch, left.getWidth(), ch - 2);
        const bool on = i == browseCat;
        g.setColour (on ? Col::magenta.withAlpha (0.22f) : Col::phos.withAlpha (0.04f)); g.fillRoundedRectangle (r, 3);
        if (on) { g.setColour (Col::magenta); g.fillRoundedRectangle (r.getX(), r.getY() + 3, 3, r.getHeight() - 6, 1.5f); }
        glowText (g, cats[i], r.reduced (10, 0), on ? Col::magenta : Col::phos.withAlpha (0.8f), ft (10.0f, true, true));
        hots.push_back ({ r, [this, i] (const juce::MouseEvent&) { browseCat = i; browseScroll = 0; browseDirty = true; }, nullptr, nullptr, nullptr });
    }
    // elenco su due colonne
    auto cur = P.getSample (ed.sel); const juce::String curPath = cur ? cur->path : juce::String();
    const float rh = 20; const int rows = (int) (c.getHeight() / rh), cols = 2, perPage = rows * cols;
    browseScroll = juce::jlimit (0, juce::jmax (0, ((int) browseItems.size() - perPage + 1) / 2 * 2), browseScroll);
    const float cw = c.getWidth() / cols;
    hots.push_back ({ c, nullptr, nullptr, [this] (float d) { browseScroll += d > 0 ? -2 : 2; repaint(); }, nullptr });
    for (int k = 0; k < perPage; ++k)
    {
        const int i = browseScroll + k; if (i >= (int) browseItems.size()) break;
        const auto& it = browseItems[(size_t) i];
        auto r = juce::Rectangle<float> (c.getX() + (k / rows) * cw, c.getY() + (k % rows) * rh, cw - 6, rh - 2);
        const bool on = curPath.isNotEmpty() && (it.user ? it.file.getFullPathName() == curPath : descToPath (it.d) == curPath);
        g.setColour (on ? padColour (P.pad[ed.sel].colour.load()).withAlpha (0.25f) : Col::phos.withAlpha (0.03f)); g.fillRoundedRectangle (r, 3);
        glowText (g, (it.user ? juce::String::fromUTF8 ("\xe2\x98\x85 ") : juce::String()) + it.name, r.reduced (8, 0), on ? Col::text : (it.user ? Col::amber : Col::phos), ft (10.0f, on, true));
        hots.push_back ({ r, [this, i] (const juce::MouseEvent&)
                          {
                              if (i >= (int) browseItems.size()) return;
                              loadLibItem (ed.proc, browseItems[(size_t) i], ed.sel); ed.playPad (ed.sel, 100); ed.kitChanged();
                          }, nullptr, [this, perPage] (float d) { browseScroll += d > 0 ? -2 : 2; juce::ignoreUnused (perPage); repaint(); }, nullptr });
    }
    if ((int) browseItems.size() > perPage)
    {
        const float frac = (float) perPage / browseItems.size(), pos = (float) browseScroll / browseItems.size();
        g.setColour (Col::phos.withAlpha (0.1f)); g.fillRect (c.getRight() - 3, c.getY(), 3.0f, c.getHeight());
        g.setColour (Col::phos.withAlpha (0.6f)); g.fillRect (c.getRight() - 3, c.getY() + c.getHeight() * pos, 3.0f, c.getHeight() * frac);
    }
    if (browseItems.empty()) glowText (g, "NESSUN SUONO", c, Col::phos.withAlpha (0.4f), ft (11.0f, true, true), juce::Justification::centred);
    // azioni
    const float bw = (bottom.getWidth() - 16) / 3;
    chip (g, bottom.removeFromLeft (bw), "", "AGGIUNGI I TUOI...", true, [this] (const juce::MouseEvent&) { ed.addToLibraryDialog(); }, Col::amber); bottom.removeFromLeft (8);
    chip (g, bottom.removeFromLeft (bw), "", "SALVA SUONO DEL PAD", false, [this] (const juce::MouseEvent&)
          { savePadToLibrary (ed.proc, ed.sel, libCategories()[browseCat]); browseDirty = true; }, Col::amber); bottom.removeFromLeft (8);
    chip (g, bottom, "", "APRI CARTELLA", false, [this] (const juce::MouseEvent&) { libFolder (libCategories()[browseCat]).revealToUser(); }, Col::phos);
}

// ---------- KIT ----------
void Screen::pageKit (juce::Graphics& g, juce::Rectangle<float> c)
{
    auto& P = ed.proc;
    auto left = c.removeFromLeft (250); c.removeFromLeft (12);
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
    auto rndRow = left.removeFromBottom (26); left.removeFromBottom (6);
    {
        const float rw = (rndRow.getWidth() - 4) * 0.5f;
        chip (g, rndRow.removeFromLeft (rw), "", "RANDOM KIT", true, [this] (const juce::MouseEvent&) { randomKit (ed.proc, randMode); ed.kitChanged(); ed.playPad (0, 100); }, Col::amber);
        rndRow.removeFromLeft (4);
        chip (g, rndRow, "", kRandModes[randMode], false, [this] (const juce::MouseEvent&) { randMode = (randMode + 1) % 3; repaint(); }, Col::amber);
    }
    const float bw = (btnRow.getWidth() - 8) / 3;
    chip (g, btnRow.removeFromLeft (bw), "", "SALVA", true, [this] (const juce::MouseEvent&) { ed.saveKit (false); }, Col::magenta); btnRow.removeFromLeft (4);
    chip (g, btnRow.removeFromLeft (bw), "", "SALVA COME", false, [this] (const juce::MouseEvent&) { ed.saveKit (true); }, Col::magenta); btnRow.removeFromLeft (4);
    chip (g, btnRow, "", "APRI...", false, [this] (const juce::MouseEvent&) { ed.loadKitDialog(); }, Col::magenta);
    // lista
    const float lh = 19; const int vis = (int) (left.getHeight() / lh);
    kitScroll = juce::jlimit (0, juce::jmax (0, (int) items.size() - vis), kitScroll);
    g.setColour (Col::phos.withAlpha (0.05f)); g.fillRoundedRectangle (left, 4);
    for (int k = 0; k < vis && k + kitScroll < (int) items.size(); ++k)
    {
        const int i = k + kitScroll; auto r = juce::Rectangle<float> (left.getX(), left.getY() + k * lh, left.getWidth(), lh);
        if (i == cur) { g.setColour (Col::magenta.withAlpha (0.2f)); g.fillRect (r); }
        if (i > 0 && items[(size_t) i].factory < 0 && items[(size_t) i - 1].factory >= 0) { g.setColour (Col::phos.withAlpha (0.3f)); g.drawHorizontalLine ((int) r.getY(), r.getX(), r.getRight()); }
        g.setFont (ft (10.5f, i == cur, true)); g.setColour (i == cur ? Col::magenta : Col::phos.withAlpha (0.85f));
        g.drawText (items[(size_t) i].name, r.reduced (6, 0), juce::Justification::centredLeft);
        g.setFont (ft (8.5f, false, true)); g.setColour (Col::phos.withAlpha (0.4f));
        g.drawText (items[(size_t) i].style, r.reduced (6, 0), juce::Justification::centredRight);
        hots.push_back ({ r, [loadItem, i] (const juce::MouseEvent&) { loadItem (i); }, nullptr, [this] (float d) { kitScroll += d > 0 ? -1 : 1; repaint(); }, nullptr });
    }
    // ---- vista dinamica: transienti in uscita + pad colpiti (ultimi 3 secondi) ----
    const int NS = Processor::kScope, w0 = P.scW.load();
    auto at = [&P, w0, NS] (int k) { return (w0 + k) % NS; };     // k = 0 piu vecchio, NS-1 piu recente
    g.setColour (Col::phos.withAlpha (0.05f)); g.fillRoundedRectangle (c, 4);
    auto lanes = c.removeFromBottom (c.getHeight() * 0.42f); c.removeFromBottom (4);
    auto wave = c.reduced (4, 2);
    auto lab = wave.removeFromTop (14);
    glowText (g, "TRANSIENTI  OUT", lab, Col::phos.withAlpha (0.7f), ft (9.0f, true, true));
    float pk = 0; int nHits = 0; for (int k = NS - 100; k < NS; ++k) { pk = juce::jmax (pk, P.scPk[at (k)].load()); nHits += juce::countNumberOfBits ((juce::uint32) P.scHit[at (k)].load()); }
    glowText (g, "PICCO " + juce::String (juce::Decibels::gainToDecibels (pk, -60.0f), 1) + " dB   COLPI " + juce::String (nHits), lab, pk > 0.95f ? Col::magenta : Col::phos.withAlpha (0.8f), ft (9.0f, true, true), juce::Justification::centredRight);
    // griglia ogni mezzo secondo
    for (int q = 1; q < 6; ++q) { const float x = wave.getX() + wave.getWidth() * q / 6.0f; g.setColour (Col::phos.withAlpha (0.07f)); g.drawVerticalLine ((int) x, wave.getY(), lanes.getBottom()); }
    const float cy = wave.getCentreY(), hh = wave.getHeight() * 0.48f, dx = wave.getWidth() / NS;
    for (int lvl : { -6, -12, -24 }) { const float y = hh * juce::Decibels::decibelsToGain ((float) lvl); g.setColour (Col::phos.withAlpha (0.06f)); g.drawHorizontalLine ((int) (cy - y), wave.getX(), wave.getRight()); g.drawHorizontalLine ((int) (cy + y), wave.getX(), wave.getRight()); }
    juce::Path env; env.startNewSubPath (wave.getX(), cy);
    for (int k = 0; k < NS; ++k) { const float v = juce::jmin (1.0f, std::sqrt (P.scPk[at (k)].load())); env.lineTo (wave.getX() + k * dx, cy - v * hh); }
    for (int k = NS - 1; k >= 0; --k) { const float v = juce::jmin (1.0f, std::sqrt (P.scPk[at (k)].load())); env.lineTo (wave.getX() + k * dx, cy + v * hh); }
    env.closeSubPath();
    juce::ColourGradient eg (Col::phos.withAlpha (0.1f), wave.getX(), 0, Col::phos.withAlpha (0.55f), wave.getRight(), 0, false); g.setGradientFill (eg); g.fillPath (env);
    g.setColour (Col::phos.withAlpha (0.8f)); g.strokePath (env, juce::PathStrokeType (1.0f));
    // marcatori di transiente: linea colorata del pad, altezza = velocity
    for (int k = 0; k < NS; ++k)
    {
        const int m = P.scHit[at (k)].load(); if (m == 0) continue;
        const float x = wave.getX() + k * dx, age = (float) k / NS, vel = P.scVel[at (k)].load() / 127.0f;
        int first = 0; while (first < kPads && ! ((m >> first) & 1)) ++first;
        const auto pc = padColour (P.pad[juce::jlimit (0, kPads - 1, first)].colour.load());
        g.setColour (pc.withAlpha (0.15f + 0.5f * age)); g.fillRect (x - 1.5f, wave.getY(), 3.0f, wave.getHeight());
        g.setColour (pc.withAlpha (0.4f + 0.6f * age)); g.drawVerticalLine ((int) x, cy - hh * vel, cy + hh * vel);
        g.fillEllipse (x - 2.5f, cy - hh * vel - 2.5f, 5, 5);
        if (k > NS - 220) { g.setFont (ft (8.0f, true, true)); g.drawText (juce::String (first + 1), juce::Rectangle<float> (x - 10, wave.getY(), 20, 10), juce::Justification::centred); }
    }
    // testina "adesso"
    g.setColour (Col::magenta.withAlpha (0.25f)); g.fillRect (wave.getRight() - 6, wave.getY(), 6.0f, lanes.getBottom() - wave.getY());
    g.setColour (Col::magenta); g.drawVerticalLine ((int) wave.getRight(), wave.getY(), lanes.getBottom());
    // corsie dei 16 pad
    auto names = lanes.removeFromLeft (34); const float lh2 = lanes.getHeight() / kPads;
    for (int p = 0; p < kPads; ++p)
    {
        const int row = kPads - 1 - p; const float y = lanes.getY() + row * lh2;
        const auto pc = padColour (P.pad[p].colour.load()); const float gl = juce::jlimit (0.0f, 1.0f, ed.pads[p]->glow);
        if (p == ed.sel) { g.setColour (pc.withAlpha (0.12f)); g.fillRect (lanes.getX(), y, lanes.getWidth(), lh2); }
        g.setColour (pc.withAlpha (0.5f + 0.5f * gl)); g.setFont (ft (7.5f, true, true));
        g.drawText (juce::String (p + 1).paddedLeft ('0', 2), juce::Rectangle<float> (names.getX() + 2, y, 16, lh2), juce::Justification::centredLeft);
        g.setColour (pc.withAlpha (0.15f + 0.85f * gl)); g.fillRect (names.getX() + 18, y + 1, 14 * juce::jmin (1.0f, P.padMeter[p].load() * 1.5f) + 1, lh2 - 2);
        for (int k = 0; k < NS; ++k)
            if ((P.scHit[at (k)].load() >> p) & 1)
            {
                const float x = lanes.getX() + k * (lanes.getWidth() / NS), age = (float) k / NS, vel = P.scVel[at (k)].load() / 127.0f;
                g.setColour (pc.withAlpha (0.25f + 0.75f * age)); g.fillRoundedRectangle (x - 1, y + 0.5f, 3.0f + vel * 3.0f, lh2 - 1, 1);
            }
        hots.push_back ({ juce::Rectangle<float> (names.getX(), y, names.getWidth() + lanes.getWidth(), lh2), [this, p] (const juce::MouseEvent& e) { ed.selectPad (p); if (e.mods.isPopupMenu()) ed.openBrowser (p); else ed.playPad (p, 100); }, nullptr, nullptr, nullptr });
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
    auto w = c.reduced (6, 10);
    g.setColour (Col::phos.withAlpha (0.12f)); g.drawHorizontalLine ((int) w.getCentreY(), w.getX(), w.getRight());
    if (smp)
    {
        const int n = smp->buf.getNumSamples(); const float* d = smp->buf.getReadPointer (0); const int Wd = (int) w.getWidth();
        const float s0 = pd.get (START), s1 = pd.get (END);
        for (int x = 0; x < Wd; ++x)
        {
            const int a = (int) ((double) x / Wd * n), e = juce::jmin (n, (int) ((double) (x + 1) / Wd * n) + 1);
            float mx = 0; for (int i = a; i < e; i += 3) mx = juce::jmax (mx, std::abs (d[i]));
            const float fx = (float) x / Wd, h = mx * w.getHeight() * 0.47f;
            g.setColour (fx >= s0 && fx <= s1 ? pc.withAlpha (0.85f) : Col::phos.withAlpha (0.15f));
            g.drawVerticalLine ((int) w.getX() + x, w.getCentreY() - h, w.getCentreY() + h + 1);
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
            const float x = w.getX() + (k ? s1 : s0) * w.getWidth();
            g.setColour (Col::phos); g.drawLine (x, w.getY() - 4, x, w.getBottom() + 4, 1.5f);
            juce::Path t; t.addTriangle (x - 6, w.getY() - 6, x + 6, w.getY() - 6, x, w.getY() + 3); g.fillPath (t);
            glowText (g, k ? "END" : "START", juce::Rectangle<float> (k ? x - 44 : x + 4, w.getBottom() - 14, 40, 12), Col::phos, ft (9.0f, true, true), k ? juce::Justification::centredRight : juce::Justification::centredLeft);
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
            auto markerDrag = [this, w] (const juce::MouseEvent& e)
            {
                auto& PP = ed.proc.pad[ed.sel]; const float fx = juce::jlimit (0.0f, 1.0f, (e.position.x - w.getX()) / w.getWidth());
                const bool endM = e.mods.isRightButtonDown() || std::abs (fx - PP.get (END)) < std::abs (fx - PP.get (START));
                if (! endM) PP.v[START] = juce::jmin (fx, PP.get (END) - 0.002f); else PP.v[END] = juce::jmax (fx, PP.get (START) + 0.002f);
            };
            hots.push_back ({ w.expanded (6, 10), markerDrag, markerDrag, nullptr, nullptr });
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
                g.setColour ((int) k == dragMark ? Col::amber : Col::magenta); g.drawLine (x, w.getY(), x, w.getBottom(), 1.6f);
                juce::Path t; t.addTriangle (x - 5, w.getBottom() + 6, x + 5, w.getBottom() + 6, x, w.getBottom() - 2); g.fillPath (t);
            }
            auto toFx = [w] (const juce::MouseEvent& e) { return juce::jlimit (0.0f, 1.0f, (e.position.x - w.getX()) / w.getWidth()); };
            hots.push_back ({ w.expanded (6, 10),
                [this, w, toFx] (const juce::MouseEvent& e)
                {
                    dragMark = -1; const float fx = toFx (e); lastFx = fx;
                    for (size_t k = 0; k < marks.size(); ++k) if (std::abs (marks[k] - fx) * w.getWidth() < 7) dragMark = (int) k;
                    if (dragMark >= 0 && e.mods.isPopupMenu()) { marks.erase (marks.begin() + dragMark); dragMark = -1; }
                },
                [this, toFx] (const juce::MouseEvent& e)
                {
                    if (dragMark < 0 || dragMark >= (int) marks.size()) return;
                    auto& PP = ed.proc.pad[ed.sel];
                    marks[(size_t) dragMark] = juce::jlimit (PP.get (START) + 0.001f, PP.get (END) - 0.001f, snapPos (toFx (e)));
                    std::sort (marks.begin(), marks.end());
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
    const float cw = (chips.getWidth() - 30) / 7;
    auto cycle = [this] (int i) { return [this, i] (const juce::MouseEvent& e) { auto& PP = ed.proc.pad[ed.sel]; const int n = choiceCount (i);
                                                                               PP.v[i] = (float) ((juce::roundToInt (PP.get (i)) + (e.mods.isPopupMenu() ? n - 1 : 1)) % n); }; };
    chip (g, chips.removeFromLeft (cw), "MODE", formatPadParam (MODE, pd.get (MODE)), pd.get (MODE) > 0.5f, cycle (MODE), Col::amber); chips.removeFromLeft (5);
    chip (g, chips.removeFromLeft (cw), "REVERSE", formatPadParam (REVERSE, pd.get (REVERSE)), pd.get (REVERSE) > 0.5f, cycle (REVERSE), Col::amber); chips.removeFromLeft (5);
    chip (g, chips.removeFromLeft (cw), "CHOKE", formatPadParam (CHOKE, pd.get (CHOKE)), pd.get (CHOKE) > 0.5f, cycle (CHOKE), Col::amber); chips.removeFromLeft (5);
    chip (g, chips.removeFromLeft (cw), "LOOP", formatPadParam (LOOP, pd.get (LOOP)), pd.get (LOOP) > 0.5f, cycle (LOOP), Col::amber); chips.removeFromLeft (5);
    chip (g, chips.removeFromLeft (cw), "SYNC BPM", formatPadParam (WARP, pd.get (WARP)), pd.get (WARP) > 0.5f, cycle (WARP), Col::magenta); chips.removeFromLeft (5);
    auto bpmR = chips.removeFromLeft (cw * 1.25f); chips.removeFromLeft (5);
    juce::String bv = formatPadParam (SBPM, pd.get (SBPM));
    if (pd.get (SBPM) > 40 && smp) bv << juce::String::fromUTF8 (" â ") << juce::String (ed.proc.hostBpm(), 0);
    chip (g, bpmR, "BPM CAMPIONE", bv, pd.get (WARP) > 0.5f, nullptr, Col::magenta);
    hots.push_back ({ bpmR, [] (const juce::MouseEvent&) {}, [this, start = pd.get (SBPM)] (const juce::MouseEvent& e) mutable
                      { ed.proc.pad[ed.sel].v[SBPM] = juce::jlimit (0.0f, 240.0f, std::round ((juce::jmax (start, 60.0f) - e.getDistanceFromDragStartY() * (e.mods.isShiftDown() ? 0.05f : 0.5f)) * 10) / 10); },
                      [this] (float d) { auto& v = ed.proc.pad[ed.sel].v[SBPM]; v = juce::jlimit (0.0f, 240.0f, juce::jmax (v.load(), 59.0f) + (d > 0 ? 1.0f : -1.0f)); },
                      [this] { ed.proc.detectBpm (ed.sel); } });
    if (smp) chip (g, chips, "", juce::String::fromUTF8 ("EDITOR GRANDE / CHOP â¶"), true, [this] (const juce::MouseEvent&) { ed.openChop (ed.sel); }, Col::green);
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
void Screen::pageEq (juce::Graphics& g, juce::Rectangle<float> c)
{
    auto& pd = ed.proc.pad[ed.sel]; const auto pc = padColour (pd.colour.load()); const bool on = pd.get (EQON) > 0.5f;
    auto top = c.removeFromTop (26); c.removeFromTop (4);
    chip (g, top.removeFromLeft (90), "", on ? "EQ ON" : "EQ OFF", on, [this] (const juce::MouseEvent&) { auto& v = ed.proc.pad[ed.sel].v[EQON]; v = v.load() > 0.5f ? 0.0f : 1.0f; }, pc);
    glowText (g, juce::String::fromUTF8 ("TRASCINA I PUNTI  \xc2\xb7  ROTELLA SUL MID = Q"), top.withTrimmedLeft (12), Col::phos.withAlpha (0.45f), ft (9.0f, false, true));
    auto r = c.reduced (4, 2); const float range = 18;
    freqGrid (g, r, range);
    const double sr = 48000;
    juce::Path curve, fill;
    for (int x = 0; x <= (int) r.getWidth(); x += 2)
    {
        const float f = xToFreq (r.getX() + x, r);
        float db = Biquad::magDb (0, sr, pd.get (LOWF), pd.get (LOWG), 0.707, f) + Biquad::magDb (1, sr, pd.get (MIDF), pd.get (MIDG), pd.get (MIDQ), f) + Biquad::magDb (2, sr, pd.get (HIGHF), pd.get (HIGHG), 0.707, f);
        if (! on) db = 0;
        const float y = r.getCentreY() - juce::jlimit (-range, range, db) / range * r.getHeight() * 0.5f;
        if (x == 0) { curve.startNewSubPath (r.getX() + x, y); fill.startNewSubPath (r.getX() + x, r.getCentreY()); fill.lineTo (r.getX() + x, y); }
        else { curve.lineTo (r.getX() + x, y); fill.lineTo (r.getX() + x, y); }
    }
    fill.lineTo (r.getRight(), r.getCentreY()); fill.closeSubPath();
    g.setColour (pc.withAlpha (on ? 0.18f : 0.05f)); g.fillPath (fill);
    g.setColour (pc.withAlpha (on ? 0.3f : 0.1f)); g.strokePath (curve, juce::PathStrokeType (6.0f)); g.setColour (on ? pc : pc.withAlpha (0.3f)); g.strokePath (curve, juce::PathStrokeType (2.0f));
    const int fP[3] = { LOWF, MIDF, HIGHF }, gP[3] = { LOWG, MIDG, HIGHG }; const char* nm[3] = { "LOW", "MID", "HIGH" };
    for (int k = 0; k < 3; ++k)
    {
        const float x = freqToX (pd.get (fP[k]), r), y = r.getCentreY() - pd.get (gP[k]) / range * r.getHeight() * 0.5f;
        g.setColour (Col::lcd2); g.fillEllipse (x - 8, y - 8, 16, 16); g.setColour (pc); g.drawEllipse (x - 8, y - 8, 16, 16, 2.0f);
        g.setFont (ft (8.5f, true, true)); g.drawText (nm[k], juce::Rectangle<float> (x - 20, y - 22, 40, 12), juce::Justification::centred);
        auto dragNode = [this, r, k, fP, gP] (const juce::MouseEvent& e)
        {
            const float f = juce::jlimit (kPInfo[fP[k]].min, kPInfo[fP[k]].max, xToFreq (e.position.x, r));
            const float db = juce::jlimit (-18.0f, 18.0f, (r.getCentreY() - e.position.y) / (r.getHeight() * 0.5f) * 18.0f);
            ed.proc.pad[ed.sel].v[fP[k]] = f; ed.proc.pad[ed.sel].v[gP[k]] = std::round (db * 2) / 2; ed.proc.pad[ed.sel].v[EQON] = 1;
        };
        hots.push_back ({ juce::Rectangle<float> (x - 12, y - 12, 24, 24), [] (const juce::MouseEvent&) {}, dragNode,
                          k == 1 ? std::function<void (float)> ([this] (float d) { auto& q = ed.proc.pad[ed.sel].v[MIDQ]; q = juce::jlimit (0.3f, 6.0f, q.load() * (d > 0 ? 1.15f : 0.87f)); }) : nullptr,
                          [this, gP, k] { ed.proc.pad[ed.sel].v[gP[k]] = 0; } });
    }
}

// ---------- FILTRO ----------
void Screen::pageFilter (juce::Graphics& g, juce::Rectangle<float> c)
{
    auto& pd = ed.proc.pad[ed.sel]; const auto pc = padColour (pd.colour.load());
    auto top = c.removeFromTop (26); c.removeFromTop (4);
    const char* types[] = { "LOW PASS", "HIGH PASS", "BAND PASS" }; const int ft0 = juce::roundToInt (pd.get (FTYPE));
    for (int i = 0; i < 3; ++i) { chip (g, top.removeFromLeft (96), "", types[i], ft0 == i, [this, i] (const juce::MouseEvent&) { ed.proc.pad[ed.sel].v[FTYPE] = (float) i; }, pc); top.removeFromLeft (6); }
    glowText (g, "TRASCINA: X = CUTOFF  Y = RESO", top.withTrimmedLeft (8), Col::phos.withAlpha (0.45f), ft (9.0f, false, true));
    auto r = c.reduced (4, 2); const float range = 24;
    freqGrid (g, r, range);
    const float fc = pd.get (CUTOFF), k = 2.0f - 1.94f * pd.get (RESO);
    juce::Path curve; juce::Path env;
    for (int x = 0; x <= (int) r.getWidth(); x += 2)
    {
        const float f = xToFreq (r.getX() + x, r), w = f / fc, w2 = w * w;
        const float den = std::sqrt ((1 - w2) * (1 - w2) + (k * w) * (k * w));
        float mag = ft0 == 0 ? 1 / den : ft0 == 1 ? w2 / den : (k * w) / den;
        if (ft0 == 0 && fc >= 19500) mag = 1;
        const float db = 20 * std::log10 (mag + 1e-6f), y = r.getCentreY() - juce::jlimit (-range, range, db) / range * r.getHeight() * 0.5f;
        if (x == 0) curve.startNewSubPath (r.getX() + x, y); else curve.lineTo (r.getX() + x, y);
    }
    g.setColour (pc.withAlpha (0.3f)); g.strokePath (curve, juce::PathStrokeType (6.0f)); g.setColour (pc); g.strokePath (curve, juce::PathStrokeType (2.0f));
    const float nx = freqToX (juce::jmin (fc, 19999.0f), r), ny = r.getCentreY() - pd.get (RESO) * r.getHeight() * 0.45f;
    // inviluppo del filtro: freccia verso dove si sposta il cutoff
    const float fe = pd.get (FENV);
    if (std::abs (fe) > 0.01f) { const float tx = freqToX (juce::jlimit (20.0f, 20000.0f, fc * std::pow (2.0f, fe * 7)), r);
        g.setColour (Col::amber.withAlpha (0.8f)); juce::Path a; a.addArrow ({ tx, ny, nx, ny }, 1.5f, 8, 8); g.fillPath (a);
        glowText (g, "ENV", juce::Rectangle<float> (juce::jmin (tx, nx), ny - 16, std::abs (tx - nx), 12), Col::amber, ft (8.5f, true, true), juce::Justification::centred); }
    g.setColour (Col::lcd2); g.fillEllipse (nx - 8, ny - 8, 16, 16); g.setColour (pc); g.drawEllipse (nx - 8, ny - 8, 16, 16, 2.0f);
    glowText (g, formatPadParam (CUTOFF, fc) + "  Q " + juce::String (juce::roundToInt (pd.get (RESO) * 100)) + "%", juce::Rectangle<float> (r.getRight() - 160, r.getY(), 156, 14), Col::phos, ft (10.0f, true, true), juce::Justification::centredRight);
    auto dragNode = [this, r] (const juce::MouseEvent& e)
    {
        ed.proc.pad[ed.sel].v[CUTOFF] = juce::jlimit (40.0f, 20000.0f, xToFreq (e.position.x, r));
        ed.proc.pad[ed.sel].v[RESO] = juce::jlimit (0.0f, 1.0f, (r.getCentreY() - e.position.y) / (r.getHeight() * 0.45f));
    };
    hots.push_back ({ r, dragNode, dragNode, nullptr, [this] { ed.proc.pad[ed.sel].v[CUTOFF] = 20000; ed.proc.pad[ed.sel].v[RESO] = 0.1f; } });
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

// ---------- FX (catena del pad) ----------
void Screen::pageFx (juce::Graphics& g, juce::Rectangle<float> c)
{
    auto& pd = ed.proc.pad[ed.sel]; const auto pc = padColour (pd.colour.load());
    struct Blk { const char* name; juce::String val; bool on; int page; };
    const bool filt = ! (juce::roundToInt (pd.get (FTYPE)) == 0 && pd.get (CUTOFF) >= 19500);
    const bool eq = pd.get (EQON) > 0.5f && (std::abs (pd.get (LOWG)) + std::abs (pd.get (MIDG)) + std::abs (pd.get (HIGHG))) > 0.1f;
    const Blk blocks[] = {
        { "GAIN", formatPadParam (GAIN, pd.get (GAIN)), std::abs (pd.get (GAIN)) > 0.05f, PG_MIX },
        { "FILTRO", filt ? formatPadParam (CUTOFF, pd.get (CUTOFF)) : "OFF", filt, PG_FILTER },
        { "EQ", eq ? "ATTIVO" : "FLAT", eq, PG_EQ },
        { "COMP", formatPadParam (CMODE, pd.get (CMODE)), pd.get (CMODE) > 0.5f, PG_COMP },
        { "DRIVE", formatPadParam (DRIVE, pd.get (DRIVE)), pd.get (DRIVE) > 0.005f, -1 },
        { "LO-FI", juce::String (juce::roundToInt (15 - pd.get (BITS) * 11)) + " BIT", pd.get (BITS) > 0.005f || pd.get (RATE) > 0.005f, -1 },
        { "VOLUME", formatPadParam (LEVEL, pd.get (LEVEL)), true, PG_MIX } };
    auto row = c.removeFromTop (70); const float bw = (row.getWidth() - 6 * 14) / 7;
    for (int i = 0; i < 7; ++i)
    {
        auto r = row.removeFromLeft (bw); if (i < 6) { auto a = row.removeFromLeft (14); g.setColour (Col::phos.withAlpha (0.4f)); juce::Path ar; ar.addArrow ({ a.getX() + 1, a.getCentreY(), a.getRight() - 1, a.getCentreY() }, 1.2f, 6, 5); g.fillPath (ar); }
        const auto& B = blocks[i];
        g.setColour (B.on ? pc.withAlpha (0.2f) : Col::phos.withAlpha (0.04f)); g.fillRoundedRectangle (r, 5);
        g.setColour (B.on ? pc : Col::phos.withAlpha (0.25f)); g.drawRoundedRectangle (r, 5, B.on ? 1.6f : 1.0f);
        glowText (g, B.name, r.withHeight (26).reduced (6, 0), B.on ? pc : Col::phos.withAlpha (0.6f), ft (10.0f, true, true), juce::Justification::centred);
        glowText (g, B.val, r.withTrimmedTop (30).reduced (4, 0).withHeight (20), Col::phos, ft (10.0f, false, true), juce::Justification::centred);
        if (B.page >= 0) hots.push_back ({ r, [this, pgx = B.page] (const juce::MouseEvent&) { ed.setPage (pgx); }, nullptr, nullptr, nullptr });
    }
    c.removeFromTop (14);
    // mandate
    auto sends = c.removeFromTop (60);
    auto sendBox = [&] (juce::Rectangle<float> r, const char* nm, int param, juce::Colour col)
    {
        const float v = pd.get (param);
        g.setColour (Col::phos.withAlpha (0.05f)); g.fillRoundedRectangle (r, 5); g.setColour (v > 0.005f ? col : Col::phos.withAlpha (0.25f)); g.drawRoundedRectangle (r, 5, 1.0f);
        glowText (g, juce::String ("MANDATA ") + nm, r.reduced (10, 6).withHeight (14), v > 0.005f ? col : Col::phos.withAlpha (0.6f), ft (10.0f, true, true));
        auto t = r.reduced (10, 0).withTrimmedTop (28).withHeight (10);
        g.setColour (Col::phos.withAlpha (0.12f)); g.fillRoundedRectangle (t, 3); g.setColour (col); g.fillRoundedRectangle (t.withWidth (t.getWidth() * v), 3);
        glowText (g, formatPadParam (param, v), r.reduced (10, 6).withHeight (14), Col::phos, ft (10.0f, true, true), juce::Justification::centredRight);
        auto set = [this, param, t] (const juce::MouseEvent& e) { ed.proc.pad[ed.sel].v[param] = juce::jlimit (0.0f, 1.0f, (e.position.x - t.getX()) / t.getWidth()); };
        hots.push_back ({ r, set, set, nullptr, [this, param] { ed.proc.pad[ed.sel].v[param] = 0; } });
    };
    sendBox (sends.removeFromLeft (sends.getWidth() / 2 - 6), "REVERB", SREV, Col::cyan); sends.removeFromLeft (12);
    sendBox (sends, "DELAY", SDLY, Col::magenta);
    c.removeFromTop (12);
    // anteprima drive e lo-fi
    auto prev = c; auto d1 = prev.removeFromLeft (prev.getWidth() / 2 - 6); prev.removeFromLeft (12);
    g.setColour (Col::phos.withAlpha (0.04f)); g.fillRoundedRectangle (d1, 5); g.fillRoundedRectangle (prev, 5);
    juce::Path dp, lp; const float k = 1 + pd.get (DRIVE) * 14, nrm = (1.0f + (k - 1.0f) * 0.15f) / k * 2.0f;
    const float q = std::pow (2.0f, 15.0f - pd.get (BITS) * 11.0f), step = 1.0f - pd.get (RATE) * 0.88f;
    float hold = 0, ph = 1;
    for (int i = 0; i <= 100; ++i)
    {
        const float t = i / 100.0f, s = std::sin (t * juce::MathConstants<float>::twoPi) * 0.9f;
        const float yd = pd.get (DRIVE) > 0.005f ? std::tanh (s * k) * nrm : s;
        ph += step * 0.25f; if (ph >= 1) { ph -= 1; hold = s; }
        const float yl = std::round ((pd.get (RATE) > 0.005f ? hold : s) * juce::jmin (q, 64.0f)) / juce::jmin (q, 64.0f);
        auto pt = [] (juce::Rectangle<float> r, float t2, float y) { return juce::Point<float> (r.getX() + 8 + (r.getWidth() - 16) * t2, r.getCentreY() - y * (r.getHeight() * 0.38f)); };
        if (i == 0) { dp.startNewSubPath (pt (d1, t, yd)); lp.startNewSubPath (pt (prev, t, yl)); } else { dp.lineTo (pt (d1, t, yd)); lp.lineTo (pt (prev, t, yl)); }
    }
    g.setColour (pc); g.strokePath (dp, juce::PathStrokeType (1.6f)); g.strokePath (lp, juce::PathStrokeType (1.6f));
    glowText (g, "DRIVE", d1.reduced (8, 4).withHeight (12), Col::phos.withAlpha (0.6f), ft (9.0f, true, true));
    glowText (g, "LO-FI  BIT / RATE", prev.reduced (8, 4).withHeight (12), Col::phos.withAlpha (0.6f), ft (9.0f, true, true));
}

// ---------- MASTER ----------
void Screen::pageMaster (juce::Graphics& g, juce::Rectangle<float> c)
{
    auto& P = ed.proc; auto par = [&P] (const char* id) { return P.apvts.getParameter (id); };
    auto val = [&P] (const char* id) { return P.apvts.getRawParameterValue (id)->load(); };
    const float bw = (c.getWidth() - 24) / 3;
    auto box = [&] (juce::Rectangle<float> r, const char* nm, juce::Colour col, bool on)
    {
        g.setColour (Col::phos.withAlpha (0.04f)); g.fillRoundedRectangle (r, 6); g.setColour (on ? col : Col::phos.withAlpha (0.25f)); g.drawRoundedRectangle (r, 6, 1.2f);
        glowText (g, nm, r.reduced (10, 6).withHeight (16), on ? col : Col::phos.withAlpha (0.6f), ft (12.0f, true, true));
    };
    auto line = [&] (juce::Rectangle<float>& r, const juce::String& l, const juce::String& v)
    {
        auto rr = r.removeFromTop (18); g.setFont (ft (9.5f, false, true)); g.setColour (Col::phos.withAlpha (0.55f)); g.drawText (l, rr, juce::Justification::centredLeft);
        glowText (g, v, rr, Col::phos, ft (10.5f, true, true), juce::Justification::centredRight);
    };
    auto pct = [] (float v) { return juce::String (juce::roundToInt (v * 100)) + "%"; };
    // reverb
    auto r1 = c.removeFromLeft (bw); c.removeFromLeft (12);
    box (r1, "REVERB", Col::cyan, true); auto in1 = r1.reduced (12, 0).withTrimmedTop (30);
    line (in1, "SIZE", pct (val ("revSize"))); line (in1, "DAMP", pct (val ("revDamp"))); line (in1, "RETURN", pct (val ("revRet")));
    { juce::Path dec; auto gr = in1.removeFromTop (in1.getHeight() - 8).reduced (0, 8);
      for (int i = 0; i <= 60; ++i) { const float t = i / 60.0f, y = std::exp (-t * (5.5f - val ("revSize") * 4.5f)); const juce::Point<float> pt (gr.getX() + gr.getWidth() * t, gr.getBottom() - gr.getHeight() * y); if (i == 0) dec.startNewSubPath (pt); else dec.lineTo (pt); }
      g.setColour (Col::cyan.withAlpha (0.8f)); g.strokePath (dec, juce::PathStrokeType (1.5f)); }
    // delay
    auto r2 = c.removeFromLeft (bw); c.removeFromLeft (12);
    box (r2, "DELAY", Col::magenta, true); auto in2 = r2.reduced (12, 0).withTrimmedTop (30);
    auto* dt = dynamic_cast<juce::AudioParameterChoice*> (par ("dlyTime"));
    chip (g, in2.removeFromTop (24), "", "TEMPO " + dt->getCurrentChoiceName(), true, [dt] (const juce::MouseEvent& e) { const int n = dt->choices.size(); dt->setValueNotifyingHost (dt->convertTo0to1 ((float) ((dt->getIndex() + (e.mods.isPopupMenu() ? n - 1 : 1)) % n))); }, Col::magenta);
    in2.removeFromTop (6);
    line (in2, "FEEDBACK", pct (val ("dlyFb"))); line (in2, "TONE", pct (val ("dlyTone"))); line (in2, "RETURN", pct (val ("dlyRet")));
    { auto gr = in2.reduced (0, 8); float a = 1; for (int k = 0; k < 8 && a > 0.03f; ++k) { const float x = gr.getX() + gr.getWidth() * k / 8.0f; g.setColour ((k % 2 ? Col::cyan : Col::magenta).withAlpha (0.85f)); g.fillRect (x, gr.getBottom() - gr.getHeight() * a, 5.0f, gr.getHeight() * a); a *= val ("dlyFb"); } }
    // bus
    auto r3 = c; box (r3, "BUS", Col::amber, val ("busMode") > 0.5f || val ("clip") > 0.5f);
    auto in3 = r3.reduced (12, 0).withTrimmedTop (30);
    auto* bm = dynamic_cast<juce::AudioParameterChoice*> (par ("busMode"));
    auto row = in3.removeFromTop (24); const float cw = (row.getWidth() - 9) / 4;
    for (int i = 0; i < 4; ++i) { chip (g, row.removeFromLeft (cw), "", bm->choices[i], bm->getIndex() == i, [bm, i] (const juce::MouseEvent&) { bm->setValueNotifyingHost (bm->convertTo0to1 ((float) i)); }, Col::amber); row.removeFromLeft (3); }
    in3.removeFromTop (6);
    line (in3, "AMOUNT", pct (val ("busAmt")));
    auto grb = in3.removeFromTop (14).reduced (0, 3); const float gr = juce::jlimit (0.0f, 18.0f, P.busGr.load());
    g.setColour (Col::phos.withAlpha (0.1f)); g.fillRoundedRectangle (grb, 2); g.setColour (Col::amber); g.fillRoundedRectangle (grb.withLeft (grb.getRight() - grb.getWidth() * gr / 18), 2);
    in3.removeFromTop (6);
    auto* cl = par ("clip");
    chip (g, in3.removeFromTop (24), "", val ("clip") > 0.5f ? "CLIP ON" : "CLIP OFF", val ("clip") > 0.5f, [cl] (const juce::MouseEvent&) { cl->setValueNotifyingHost (cl->getValue() > 0.5f ? 0.0f : 1.0f); }, Col::red);
    in3.removeFromTop (8);
    // meter di uscita
    const float m[2] = { P.meterL.load(), P.meterR.load() };
    for (int ch = 0; ch < 2; ++ch)
    {
        auto mb = in3.removeFromTop (9).reduced (0, 1); in3.removeFromTop (3);
        const float db = juce::Decibels::gainToDecibels (m[ch], -48.0f), n = juce::jlimit (0.0f, 1.0f, (db + 48) / 51);
        for (int k = 0; k < 30; ++k) { const float t = k / 30.0f; g.setColour (t <= n ? (t > 0.9f ? Col::red : t > 0.75f ? Col::amber : Col::green) : Col::phos.withAlpha (0.08f)); g.fillRect (mb.getX() + k * mb.getWidth() / 30, mb.getY(), mb.getWidth() / 30 - 1.5f, mb.getHeight()); }
    }
    line (in3, "OUTPUT", juce::String (val ("out"), 1) + " dB");
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
    if (ed.page == PG_KIT || ed.page == PG_MIX || ed.page == PG_MASTER) ed.setPage (PG_SAMPLE);
    ed.root.repaint();
}
// ======================= Pattern =======================
void PatternView::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat(); insetPanel (g, b, 8);
    auto s = b.reduced (5); g.setColour (Col::lcd2); g.fillRoundedRectangle (s, 5);
    auto& P = ed.proc; const double L = P.loopTicks(); const int bars = (int) (L / kBar);
    auto a = s.reduced (8, 6).withTrimmedTop (12);
    for (int q = 0; q <= bars * 16; ++q)
    {
        const float x = a.getX() + a.getWidth() * (float) (q / (bars * 16.0));
        g.setColour (Col::phos.withAlpha (q % 16 == 0 ? 0.3f : q % 4 == 0 ? 0.12f : 0.04f)); g.drawVerticalLine ((int) x, a.getY(), a.getBottom());
    }
    for (int k = 0; k < bars; ++k) glowText (g, juce::String (k + 1), juce::Rectangle<float> (a.getX() + a.getWidth() * k / bars + 3, s.getY() + 2, 20, 11), Col::phos.withAlpha (0.6f), ft (8.5f, true, true));
    const float rh = a.getHeight() / kPads;
    { const juce::SpinLock::ScopedLockType l (P.recLock);
      for (auto& e : P.events)
      {
          const double q = P.swingTick (e.tick); const float x = a.getX() + a.getWidth() * (float) (q / L), y = a.getBottom() - (e.pad + 1) * rh;
          const auto c = padColour (P.pad[e.pad].colour.load());
          g.setColour (c.withAlpha (0.3f + 0.7f * e.vel / 127.0f)); g.fillRoundedRectangle (x - 1, y + 0.5f, 4, juce::jmax (1.5f, rh - 1), 1);
      } }
    if (P.uiRunning.load())
    {
        const double t = std::fmod (P.uiTick.load(), L); const float x = a.getX() + a.getWidth() * (float) (t / L);
        g.setColour (Col::magenta.withAlpha (0.3f)); g.fillRect (x - 3, a.getY(), 6.0f, a.getHeight());
        g.setColour (Col::magenta); g.drawVerticalLine ((int) x, a.getY(), a.getBottom());
    }
    const auto& pb = patternBank()[(size_t) juce::jlimit (0, (int) patternBank().size() - 1, P.patternIndex)];
    glowText (g, juce::String (pb.genre).toUpperCase() + " / " + juce::String (pb.name).toUpperCase(), juce::Rectangle<float> (s.getRight() - 230, s.getY() + 2, 222, 11), P.loopOn.load() ? Col::magenta : Col::phos.withAlpha (0.4f), ft (8.5f, true, true), juce::Justification::centredRight);
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
    const auto f = ed.proc.writeMidi (ed.exportSwing);
    juce::DragAndDropContainer::performExternalDragDropOfFiles ({ f.getFullPathName() }, false, this, [this] { dragging = false; repaint(); });
}

// ======================= Root =======================
void Root::paint (juce::Graphics& g)
{
    const float W = (float) Editor::W, H = (float) Editor::H;
    jlook::backdrop (g, { 0, 0, W, H }, Col::magenta, Col::cyan);
    jlook::header (g, { 0, 0, W, 66 }, "SAMPLER", Col::magenta, "JP-16  DRUM SAMPLER", Col::magenta, Col::cyan);
    g.setFont (ft (10.0f, true, true)); g.setColour (Col::dim); g.drawText ("KIT", juce::Rectangle<float> (430, 12, 40, 40), juce::Justification::centredLeft);
    glowText (g, ed.proc.kitName.toUpperCase(), { 462, 12, 380, 40 }, Col::amber, ft (11.0f, true, true));
    for (int i = 0; i < 14; ++i) { auto r = juce::Rectangle<float> (W - 380 + i * 11.0f, 20, 5, 26); g.setColour (juce::Colours::black.withAlpha (0.8f)); g.fillRoundedRectangle (r, 2.5f); g.setColour (juce::Colours::white.withAlpha (0.06f)); g.drawLine (r.getX(), r.getBottom() + 1, r.getRight(), r.getBottom() + 1); }
    g.saveState(); auto hz = juce::Rectangle<float> (W - 210, 22, 70, 22); g.reduceClipRegion (hz.toNearestInt());
    g.setColour (Col::amber.withAlpha (0.85f)); g.fillRect (hz);
    g.setColour (juce::Colours::black.withAlpha (0.85f)); for (float x = hz.getX() - 30; x < hz.getRight(); x += 12) { juce::Path p; p.addQuadrilateral (x, hz.getBottom(), x + 6, hz.getBottom(), x + 28, hz.getY(), x + 22, hz.getY()); g.fillPath (p); }
    g.restoreState();
    for (auto pt : { juce::Point<float> (12, 80), juce::Point<float> (W - 12, 80), juce::Point<float> (12, H - 12), juce::Point<float> (W - 12, H - 12) }) screw (g, pt.x, pt.y);
    jlook::softShadow (g, { 724, 80, 496, 496 }, 14); insetPanel (g, { 724, 80, 496, 496 }, 14);
    // fascia Q-link
    jlook::card (g, { 24, 416, 680, 98 }, 10, Col::cyan, true);
    g.saveState(); g.addTransform (juce::AffineTransform::rotation (-juce::MathConstants<float>::halfPi, 38, 465));
    g.setColour (Col::cyan); g.setFont (ft (9.5f, true)); g.drawText ("Q-LINK", juce::Rectangle<float> (38 - 40, 459, 80, 12), juce::Justification::centred);
    g.restoreState();
    // sequencer
    jlook::card (g, { 24, 524, 680, 198 }, 10, Col::magenta, true);
    glowText (g, juce::String::fromUTF8 ("LOOP & SWING  \xc2\xb7  sincronizzati con Ableton"), { 36, 528, 400, 18 }, Col::cyan, ft (10.0f, true));
    // info tastiera
    g.setFont (ft (10.0f, true, true)); g.setColour (Col::text);
    if (ed.kbOn) g.drawText ("OCT " + noteName (12 * (ed.octave + 2)) + "  VEL " + juce::String (ed.fullLevel ? 127 : ed.kbVel) + "   Z/X  C/V", juce::Rectangle<float> (862, 686, 200, 14), juce::Justification::centredLeft);
    g.setColour (Col::dim); g.setFont (ft (9.0f));
    g.drawText (ed.kbOn ? "la tastiera suona il plugin (Ableton non registra)" : "tastiera ad Ableton: premi M e registra", juce::Rectangle<float> (862, 700, 250, 14), juce::Justification::centredLeft);
    // meter master
    const float m[2] = { ed.proc.meterL.load(), ed.proc.meterR.load() };
    for (int c = 0; c < 2; ++c)
    {
        auto mb = juce::Rectangle<float> (1100, 690.0f + c * 9, 118, 6);
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

void ChopView::paint (juce::Graphics& g)
{
    hots.clear();
    g.fillAll (juce::Colours::black.withAlpha (0.72f));
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
    {   // onda
        const float cy = w.getCentreY(), hh = w.getHeight() * 0.45f; float mxAll = 0.01f;
        for (size_t i = 0; i < pkMax.size(); ++i) mxAll = juce::jmax (mxAll, pkMax[i], -pkMin[i]);
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
        const float xx = xOf (marks[k]); const bool dr = drag == (int) k + 1;
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
    g.restoreState();
    g.setColour (Col::dim); g.setFont (ft (9.5f, false, true));
    g.drawText (juce::String::fromUTF8 ("rotella = zoom \xc2\xb7 maiusc+rotella o barra sotto = scorri \xc2\xb7 doppio clic = nuovo taglio \xc2\xb7 tasto destro = togli \xc2\xb7 clic su una fetta = ascolta e scegli \xc2\xb7 trascina INIZIO/FINE (in alto) per la regione"),
                juce::Rectangle<float> (w.getX(), w.getY() - 14, w.getWidth(), 12), juce::Justification::centredLeft);
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
    // fette e pad
    const float yA = 476;
    glowText (g, "ASSEGNA AI PAD", { 34, yA, 200, 22 }, Col::text, ft (13.0f, true));
    g.setColour (Col::dim); g.setFont (ft (10.5f));
    g.drawText (juce::String::fromUTF8 ("1. clicca una fetta sull'onda   2. clicca il pad dove metterla   oppure SPARGI per riempire i pad in ordine"), juce::Rectangle<float> (200, yA, 700, 22), juce::Justification::centredLeft);
    const int nSl = (int) pts.size() - 1;
    const float pw = 82, ph = 50;
    for (int p = 0; p < kPads; ++p)
    {
        const int row = p / 8, col = p % 8;
        auto r = juce::Rectangle<float> (34 + col * (pw + 6), yA + 30 + row * (ph + 6), pw, ph);
        auto ps = ed.proc.getSample (p); const auto c2 = padColour (ed.proc.pad[p].colour.load());
        const bool fromHere = assigned.count (p) > 0;
        jlook::softShadow (g, r, 6, 6, 0.5f, 2);
        juce::ColourGradient pg (fromHere ? c2.withAlpha (0.5f) : juce::Colour (0xff2a2d35), 0, r.getY(), fromHere ? c2.withAlpha (0.2f) : juce::Colour (0xff17191e), 0, r.getBottom(), false);
        g.setGradientFill (pg); g.fillRoundedRectangle (r, 6);
        g.setColour (p == src ? Col::green : (selSlice >= 0 ? Col::amber.withAlpha (0.6f) : juce::Colours::white.withAlpha (0.12f))); g.drawRoundedRectangle (r, 6, p == src ? 1.6f : 1.0f);
        g.setColour (Col::text); g.setFont (ft (10.0f, true, true)); g.drawText (juce::String (p + 1).paddedLeft ('0', 2), r.reduced (6, 4), juce::Justification::topLeft);
        g.setColour (Col::dim); g.setFont (ft (9.0f)); g.drawText (ps ? ps->name : juce::String ("vuoto"), r.reduced (6, 4), juce::Justification::bottomLeft);
        if (fromHere) glowText (g, "F" + juce::String (assigned[p] + 1), r.reduced (6, 4), Col::amber, ft (11.0f, true, true), juce::Justification::topRight);
        hots.push_back ({ r, [this, p] (const juce::MouseEvent&)
        {
            const auto pts2 = points();
            if (selSlice >= 0 && selSlice + 1 < (int) pts2.size())
            {
                auto& D = ed.proc.pad[p]; D.reset(); D.colour = colour; D.v[GAIN] = gain; D.v[SBPM] = bpm; D.v[WARP] = (float) warp;
                D.v[START] = pts2[(size_t) selSlice]; D.v[END] = pts2[(size_t) selSlice + 1]; D.v[SLICE] = (float) (selSlice + 1); D.v[DECAY] = 4000;
                ed.proc.setSample (p, smp); assigned[p] = selSlice;
                selSlice = juce::jmin (selSlice + 1, (int) pts2.size() - 2);   // passa alla fetta dopo
            }
            ed.playPad (p, 110);
        }, nullptr });
    }
    // azioni a destra
    auto ax = juce::Rectangle<float> (34 + 8 * (pw + 6) + 14, yA + 30, card.getRight() - (34 + 8 * (pw + 6) + 14) - 20, ph * 2 + 6);
    g.setColour (juce::Colours::black.withAlpha (0.35f)); g.fillRoundedRectangle (ax, 8);
    glowText (g, juce::String (nSl) + (nSl == 1 ? " FETTA" : " FETTE"), { ax.getX() + 12, ax.getY() + 6, 120, 20 }, Col::phos, ft (12.0f, true, true));
    g.setColour (Col::dim); g.setFont (ft (10.0f)); g.drawText ("dal pad", juce::Rectangle<float> (ax.getX() + 130, ax.getY() + 6, 50, 20), juce::Justification::centredLeft);
    btn ({ ax.getX() + 182, ax.getY() + 4, 26, 24 }, "-", Col::cyan, false, [this] (const juce::MouseEvent&) { firstPad = juce::jmax (0, firstPad - 1); });
    glowText (g, juce::String (firstPad + 1), { ax.getX() + 210, ax.getY() + 4, 30, 24 }, Col::amber, ft (13.0f, true, true), juce::Justification::centred);
    btn ({ ax.getX() + 242, ax.getY() + 4, 26, 24 }, "+", Col::cyan, false, [this] (const juce::MouseEvent&) { firstPad = juce::jmin (kPads - 1, firstPad + 1); });
    btn ({ ax.getX() + 12, ax.getY() + 38, ax.getWidth() - 24, 34 }, juce::String::fromUTF8 ("SPARGI LE FETTE SUI PAD \xe2\x96\xb6"), Col::green, true, [this] (const juce::MouseEvent&)
    {
        const auto pts2 = points();
        for (size_t k = 0; k + 1 < pts2.size(); ++k)
        {
            const int p = firstPad + (int) k; if (p >= kPads) break;
            auto& D = ed.proc.pad[p]; D.reset(); D.colour = colour; D.v[GAIN] = gain; D.v[SBPM] = bpm; D.v[WARP] = (float) warp;
            D.v[START] = pts2[k]; D.v[END] = pts2[k + 1]; D.v[SLICE] = (float) (k + 1); D.v[DECAY] = 4000;
            ed.proc.setSample (p, smp); assigned[p] = (int) k;
        }
    });
    btn ({ ax.getX() + 12, ax.getY() + 78, (ax.getWidth() - 30) / 2, 26 }, "SALVA REGIONE", Col::cyan, false, [this] (const juce::MouseEvent&)
    {   // aggiorna solo il pad sorgente con regione, BPM e sync
        auto& D = ed.proc.pad[src]; D.v[START] = r0; D.v[END] = r1; D.v[SBPM] = bpm; D.v[WARP] = (float) warp;
    });
    btn ({ ax.getX() + 18 + (ax.getWidth() - 30) / 2, ax.getY() + 78, (ax.getWidth() - 30) / 2, 26 }, "ANNULLA ZOOM", Col::cyan, false, [this] (const juce::MouseEvent&) { v0 = 0; v1 = 1; });
    if (nSl > kPads - firstPad) { g.setColour (Col::red); g.setFont (ft (9.5f, true)); g.drawText ("fette oltre il pad 16 non verranno assegnate", juce::Rectangle<float> (ax.getX() + 12, ax.getBottom() + 2, ax.getWidth(), 14), juce::Justification::centredLeft); }
    // la sorgente salva sempre BPM e sync scelti qui
    { auto& D = ed.proc.pad[src]; if (ed.proc.getSample (src) == smp) { D.v[SBPM] = bpm; D.v[WARP] = (float) warp; } }
}

void ChopView::mouseMove (const juce::MouseEvent& e)
{
    int h = -1;
    if (wave().contains (e.position)) { const auto pts = points(); const float fx = fxOf (e.position.x); for (size_t k = 0; k + 1 < pts.size(); ++k) if (fx >= pts[k] && fx < pts[k + 1]) h = (int) k; }
    if (h != hovSlice) { hovSlice = h; repaint(); }
    bool hand = false; for (auto& h2 : hots) if (h2.r.contains (e.position)) hand = true;
    setMouseCursor (hand ? juce::MouseCursor::PointingHandCursor : wave().contains (e.position) ? juce::MouseCursor::IBeamCursor : juce::MouseCursor::NormalCursor);
}
void ChopView::mouseDown (const juce::MouseEvent& e)
{
    drag = 0;
    for (auto it = hots.rbegin(); it != hots.rend(); ++it) if (it->r.contains (e.position) && it->fn) { it->fn (e); repaint(); return; }
    auto w = wave(), ov = overview();
    if (ov.expanded (0, 4).contains (e.position)) { drag = -3; dragOff = (e.position.x - ov.getX()) / ov.getWidth() - v0; mouseDrag (e); return; }
    if (! w.contains (e.position)) return;
    const float fx = fxOf (e.position.x); lastFx = fx;
    // inizio / fine (maniglie in alto)
    if (e.position.y < w.getY() + 34)
    {
        if (std::abs (xOf (r0) - e.position.x) < 24) { drag = -1; return; }
        if (std::abs (xOf (r1) - e.position.x) < 24) { drag = -2; return; }
    }
    for (size_t k = 0; k < marks.size(); ++k) if (std::abs (xOf (marks[k]) - e.position.x) < 7)
    {
        if (e.mods.isPopupMenu()) { marks.erase (marks.begin() + (long) k); selSlice = -1; repaint(); return; }
        drag = (int) k + 1; return;
    }
    // clic su una fetta: la sceglie e la fa sentire
    const auto pts = points();
    for (size_t k = 0; k + 1 < pts.size(); ++k) if (fx >= pts[k] && fx < pts[k + 1]) { selSlice = (int) k; ed.proc.audition (src, pts[k], pts[k + 1]); }
    repaint();
}
void ChopView::mouseDrag (const juce::MouseEvent& e)
{
    if (drag == 0) return;
    if (drag == -3) { auto ov = overview(); const float span = v1 - v0; v0 = juce::jlimit (0.0f, 1.0f - span, (e.position.x - ov.getX()) / ov.getWidth() - dragOff); v1 = v0 + span; repaint(); return; }
    const float fx = juce::jlimit (0.0f, 1.0f, snapTo (fxOf (e.position.x)));
    if (drag == -1) r0 = juce::jmin (fx, r1 - 0.001f);
    else if (drag == -2) r1 = juce::jmax (fx, r0 + 0.001f);
    else if (drag - 1 < (int) marks.size()) marks[(size_t) drag - 1] = juce::jlimit (r0 + 0.0005f, r1 - 0.0005f, fx);
    repaint();
}
void ChopView::mouseUp (const juce::MouseEvent&)
{
    if (drag > 0) std::sort (marks.begin(), marks.end());
    drag = 0; repaint();
}
void ChopView::mouseDoubleClick (const juce::MouseEvent& e)
{
    if (! wave().contains (e.position)) return;
    const float fx = snapTo (fxOf (e.position.x));
    if (fx > r0 + 0.0005f && fx < r1 - 0.0005f && marks.size() < 63) { marks.push_back (fx); std::sort (marks.begin(), marks.end()); if (mode != 3) mode = 3; }
    repaint();
}
void ChopView::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wd)
{
    for (auto& h : hots) if (h.r.contains (e.position) && h.wheel) { h.wheel (wd.deltaY); repaint(); return; }
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
    P.v[i] = juce::jlimit (kPInfo[i].min, kPInfo[i].max, v);
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
        auto* s = selectors.add (new Selector ("GENERE")); s->setName ("genre"); s->col = Col::magenta;
        s->get = [this] { return genre; };
        s->set = [this] (int v) { genre = v; const auto g = patternGenres()[v]; for (int i = 0; i < (int) patternBank().size(); ++i) if (g == patternBank()[(size_t) i].genre) { proc.loadPattern (i); break; } };
        s->count = [] { return patternGenres().size(); };
        s->name = [] (int v) { return patternGenres()[v].toUpperCase(); };
        root.addAndMakeVisible (s);
    }
    {
        auto* s = selectors.add (new Selector ("LOOP")); s->setName ("pattern"); s->col = Col::cyan;
        auto list = [this] { juce::Array<int> a; const auto g = patternGenres()[genre]; for (int i = 0; i < (int) patternBank().size(); ++i) if (g == patternBank()[(size_t) i].genre) a.add (i); return a; };
        s->get = [this, list] { return juce::jmax (0, list().indexOf (proc.patternIndex)); };
        s->set = [this, list] (int v) { proc.loadPattern (list()[v]); };
        s->count = [list] { return juce::jmax (1, list().size()); };
        s->name = [list] (int v) { const int i = list()[v]; return juce::String (patternBank()[(size_t) i].name).toUpperCase(); };
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
    btn ("RANDOM", Col::amber, [] { return false; }, [this] { proc.randomPattern (patternGenres()[genre]); proc.loopOn = true; })->setName ("random");
    btn ("VARIA", Col::amber, [] { return false; }, [this] { proc.varyPattern(); })->setName ("vary");
    btn ("PLAY", Col::green, [this] { return proc.uiRunning.load(); }, [this] { proc.requestPlay (! proc.intPlay.load()); })->setName ("play");
    btn ("SWING MIDI", Col::magenta, [this] { return proc.apvts.getRawParameterValue ("swMidi")->load() > 0.5f; },
         [this] { auto* pr = proc.apvts.getParameter ("swMidi"); pr->setValueNotifyingHost (pr->getValue() > 0.5f ? 0.0f : 1.0f); })->setName ("swmidi");
    btn ("+SWING", Col::cyan, [this] { return exportSwing; }, [this] { exportSwing = ! exportSwing; })->setName ("expsw");
    btn ("TASTIERA PLUGIN", Col::cyan, [this] { return kbOn; }, [this] { kbOn = ! kbOn; setWantsKeyboardFocus (kbOn); if (kbOn) grabKeyboardFocus(); else giveAwayKeyboardFocus(); root.repaint(); })->setName ("kb");
    btn ("NOTE REPEAT", Col::magenta, [this] { return proc.apvts.getRawParameterValue ("nrOn")->load() > 0.5f; },
         [this] { auto* pr = proc.apvts.getParameter ("nrOn"); pr->setValueNotifyingHost (pr->getValue() > 0.5f ? 0.0f : 1.0f); })->setName ("nr");
    btn ("FULL LEVEL", Col::magenta, [this] { return fullLevel; }, [this] { fullLevel = ! fullLevel; root.repaint(); })->setName ("full");
    btn ("LOAD", Col::cyan, [] { return false; }, [this] { loadDialog (sel); })->setName ("load");

    btn ("STOP", Col::red, [] { return false; }, [this] { proc.stopAll(); })->setName ("stop");
    root.addChildComponent (chop);
    bindQlinks();
    setWantsKeyboardFocus (false);
    setResizable (true, true);
    setResizeLimits (W / 2, H / 2, W * 3 / 2, H * 3 / 2);
    if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) W / H);
    setSize (W, H);
    startTimerHz (40);
}

static juce::Component* named (juce::Component& parent, const char* n) { for (auto* c : parent.getChildren()) if (c->getName() == n) return c; return nullptr; }

void Editor::resized()
{
    const float s = getWidth() / (float) W;
    root.setBounds (0, 0, W, H); root.setTransform (juce::AffineTransform::scale (s));
    screen.setBounds (24, 78, 680, 330);
    chop.setBounds (0, 0, W, H);
    named (root, "stop")->setBounds (W - 128, 18, 100, 30);
    for (int i = 0; i < 8; ++i) qlinks[i]->setBounds (58 + i * 80, 422, 76, 88);
    for (int i = 0; i < kPads; ++i) { const int r = 3 - i / 4, c = i % 4; pads[i]->setBounds (726 + c * 123, 82 + r * 123, 123, 123); }
    named (root, "curve")->setBounds (726, 588, 150, 40);
    named (root, "full")->setBounds (884, 600, 100, 30);
    named (root, "load")->setBounds (990, 600, 70, 30);
    globalKnobs[3]->setBounds (1066, 580, 70, 68);
    named (root, "root")->setBounds (1140, 588, 80, 40);
    named (root, "genre")->setBounds (36, 548, 120, 44);
    named (root, "pattern")->setBounds (162, 548, 150, 44);
    const char* tb[] = { "loop", "random", "vary", "play" };
    for (int i = 0; i < 4; ++i) named (root, tb[i])->setBounds (318 + i * 64, 552, 61, 40);
    named (root, "swGrid")->setBounds (578, 548, 54, 44);
    named (root, "swmidi")->setBounds (636, 552, 64, 40);
    pattern.setBounds (36, 600, 400, 114);
    globalKnobs[0]->setBounds (442, 598, 64, 80); globalKnobs[1]->setBounds (508, 598, 64, 80); globalKnobs[2]->setBounds (574, 598, 64, 80);
    named (root, "expsw")->setBounds (640, 608, 62, 34);
    drag.setBounds (442, 682, 260, 32);
    named (root, "kb")->setBounds (726, 686, 130, 30);
    named (root, "nr")->setBounds (726, 642, 130, 34);
    named (root, "nrRate")->setBounds (862, 634, 90, 44);
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
        { { LOWG }, { LOWF }, { MIDG }, { MIDF }, { MIDQ }, { HIGHG }, { HIGHF }, { GAIN } },
        { { CUTOFF }, { RESO }, { FENV }, { FDECAY }, { VFILTER }, { GAIN }, { LEVEL }, { -1 } },
        { { CAMT }, { CMAKEUP }, { CMIX }, { GAIN }, { LEVEL }, { -1 }, { -1 }, { -1 } },
        { { DRIVE }, { BITS }, { RATE }, { SREV }, { SDLY }, { PAN }, { LEVEL }, { GAIN } },
        { { -1, "revSize", "REV SIZE" }, { -1, "revDamp", "REV DAMP" }, { -1, "revRet", "REV RET" }, { -1, "dlyFb", "DLY FB" },
          { -1, "dlyTone", "DLY TONE" }, { -1, "dlyRet", "DLY RET" }, { -1, "busAmt", "BUS AMT" }, { -1, "out", "OUTPUT" } } };
    const auto& m = map[(size_t) page];
    for (int k = 0; k < 8; ++k)
    {
        auto* q = qlinks[k]; const auto& e = m[(size_t) k];
        if (e.pad >= 0)
        {
            const int i = e.pad;
            q->label = kPInfo[i].label; q->bipolar = kPInfo[i].min < 0;
            q->col = (i == VLEVEL || i == VFILTER || i == VSTART || i == VPITCH || i == SREV || i == SDLY) ? Col::magenta : (i == GAIN || i == LEVEL ? Col::amber : Col::cyan);
            q->get = [this, i] { return toNorm (i, proc.pad[sel].get (i)); };
            q->set = [this, i] (float n) { setPadParam (i, fromNorm (i, n)); screen.repaint(); };
            q->reset = [this, i] { setPadParam (i, kPInfo[i].def); screen.repaint(); };
            q->text = [this, i] { return formatPadParam (i, proc.pad[sel].get (i)); };
            q->setVisible (true);
        }
        else if (e.global != nullptr)
        {
            auto* pr = proc.apvts.getParameter (e.global);
            q->label = e.label; q->bipolar = false; q->col = Col::cyan;
            q->get = [pr] { return pr->getValue(); };
            q->set = [this, pr] (float v) { pr->setValueNotifyingHost (v); screen.repaint(); };
            q->reset = [pr] { pr->setValueNotifyingHost (pr->getDefaultValue()); };
            const juce::String id (e.global);
            q->text = [pr, id] { const float v = pr->convertFrom0to1 (pr->getValue()); return id == "out" ? juce::String (v, 1) + " dB" : juce::String (juce::roundToInt (pr->getValue() * 100)) + "%"; };
            q->setVisible (true);
        }
        else q->setVisible (false);
        q->repaint();
    }
}

void Editor::setPage (int pg) { page = juce::jlimit (0, kNumPages - 1, pg); bindQlinks(); screen.repaint(); }
void Editor::selectPad (int p) { sel = juce::jlimit (0, kPads - 1, p); root.repaint(); }
void Editor::kitChanged() { for (auto* pd : pads) pd->repaint(); root.repaint(); screen.repaint(); }
void Editor::openBrowser (int p)
{
    selectPad (p);
    if (auto s = proc.getSample (p)) { DrumDesc d; if (pathToDesc (s->path, d)) screen.browseCat = juce::jmax (0, libCategories().indexOf (categoryOf (d))); }
    screen.browseScroll = 0; screen.browseDirty = true; setPage (PG_BROWSE);
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
        else if (r >= 100) proc.pad[p].colour = r - 100;
        root.repaint();
    });
}

void Editor::timerCallback()
{
    for (int i = 0; i < kPads; ++i)
    {
        auto* pc = pads[i]; const auto h = proc.hits[i].load();
        if (h != pc->lastHit) { pc->lastHit = h; pc->flash = 0.35f + 0.65f * proc.lastVel[i].load() / 127.0f; }
        const float lev = proc.padGlow[i].exchange (0.0f);
        const float target = juce::jlimit (0.0f, 1.0f, std::sqrt (lev) * 1.4f);
        pc->glow = juce::jmax (target, pc->glow * 0.86f); pc->flash *= 0.8f;
        if (pc->glow > 0.003f || pc->flash > 0.003f || pc->held) pc->repaint(); else if (pc->glow > 0) { pc->glow = 0; pc->repaint(); }
    }
    screen.repaint(); pattern.repaint();
    for (auto* b : btns) b->repaint();
    for (auto* k : qlinks) k->repaint();
    for (auto* k : globalKnobs) k->repaint();
    for (auto* s : selectors) s->repaint();
    root.repaint (1096, 686, 124, 22);
}

static int keyNoteOffset (juce::juce_wchar c)
{
    static const char* letters = "awsedftgyhujkolp;";
    for (int i = 0; letters[i]; ++i) if (letters[i] == c) return i;
    return -1;
}
bool Editor::keyPressed (const juce::KeyPress& k)
{
    if (! kbOn) return false;
    const auto c = juce::CharacterFunctions::toLowerCase ((juce::juce_wchar) k.getKeyCode());
    static const int vels[] = { 1, 20, 40, 60, 80, 100, 120, 127 };
    auto velStep = [this] (int d) { int i = 0; while (i < 7 && vels[i] < kbVel) ++i; kbVel = vels[juce::jlimit (0, 7, i + d)]; root.repaint(); };
    if (c == 'z') { octave = juce::jmax (-2, octave - 1); root.repaint(); return true; }
    if (c == 'x') { octave = juce::jmin (7, octave + 1); root.repaint(); return true; }
    if (c == 'c') { velStep (-1); return true; }
    if (c == 'v') { velStep (1); return true; }
    if (k == juce::KeyPress::spaceKey) { proc.requestPlay (! proc.uiRunning.load()); return true; }
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
