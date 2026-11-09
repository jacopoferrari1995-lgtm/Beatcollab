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
    auto o = juce::FontOptions (size);
    if (mono) o = o.withName (juce::Font::getDefaultMonospacedFontName());
    if (bold) o = o.withStyle ("Bold");
    return juce::Font (o);
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
    const float cx = getWidth() * 0.5f, cy = size * 0.5f + 2, R = size * 0.5f - 3, r = R - 9;
    const float v = get(), a0 = -2.42f, a1 = 2.42f;
    // anello di LED
    const int nLed = 23;
    for (int i = 0; i < nLed; ++i)
    {
        const float t = i / (float) (nLed - 1), a = a0 + t * (a1 - a0);
        const bool on = bipolar ? ((v >= 0.5f && t >= 0.5f && t <= v + 0.001f) || (v < 0.5f && t <= 0.5f && t >= v - 0.001f)) : t <= v + 0.001f && v > 0.002f;
        const float x = cx + std::sin (a) * R, y = cy - std::cos (a) * R;
        if (on) { g.setColour (col.withAlpha (0.25f)); g.fillEllipse (x - 4, y - 4, 8, 8); g.setColour (col.brighter (0.3f)); }
        else g.setColour (juce::Colour (0xff23262e));
        g.fillEllipse (x - 1.8f, y - 1.8f, 3.6f, 3.6f);
    }
    // corpo 3D
    g.setColour (juce::Colours::black.withAlpha (0.65f)); g.fillEllipse (cx - r - 1, cy - r + 3, r * 2 + 2, r * 2 + 2);
    juce::ColourGradient sk (juce::Colour (0xff3b3f49), cx, cy - r, juce::Colour (0xff0d0e12), cx, cy + r, false);
    g.setGradientFill (sk); g.fillEllipse (cx - r, cy - r, r * 2, r * 2);
    const float r2 = r * 0.8f;
    juce::ColourGradient cap (juce::Colour (0xff2c3038), cx - r2 * 0.5f, cy - r2, juce::Colour (0xff121419), cx + r2 * 0.4f, cy + r2, false);
    g.setGradientFill (cap); g.fillEllipse (cx - r2, cy - r2, r2 * 2, r2 * 2);
    g.setColour (juce::Colours::white.withAlpha (0.08f)); g.drawEllipse (cx - r2, cy - r2, r2 * 2, r2 * 2, 1.0f);
    for (int i = 0; i < 28; ++i) { const float a = i * juce::MathConstants<float>::twoPi / 28; g.setColour (juce::Colours::black.withAlpha (0.35f)); g.drawLine (cx + std::sin (a) * r2, cy - std::cos (a) * r2, cx + std::sin (a) * r, cy - std::cos (a) * r, 1.0f); }
    const float a = a0 + v * (a1 - a0);
    g.setColour (col.withAlpha (0.35f)); g.drawLine (cx + std::sin (a) * r2 * 0.25f, cy - std::cos (a) * r2 * 0.25f, cx + std::sin (a) * r2 * 0.95f, cy - std::cos (a) * r2 * 0.95f, 4.0f);
    g.setColour (juce::Colours::white); g.drawLine (cx + std::sin (a) * r2 * 0.3f, cy - std::cos (a) * r2 * 0.3f, cx + std::sin (a) * r2 * 0.92f, cy - std::cos (a) * r2 * 0.92f, 1.8f);
    // etichetta e valore
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
    g.drawFittedText (s ? s->name : juce::String ("vuoto"), tb.withTrimmedBottom (4).toNearestInt(), juce::Justification::bottomLeft, 1);
    if (flash > 0.05f) { g.setFont (ft (9.0f, true, true)); g.setColour (juce::Colours::white.withAlpha (flash)); g.drawText (juce::String (P.lastVel[pad].load()), tb.withTrimmedBottom (4), juce::Justification::bottomRight); }
}
void PadComp::mouseDown (const juce::MouseEvent& e)
{
    ed.selectPad (pad);
    if (e.mods.isPopupMenu()) { ed.padMenu (pad); return; }
    if (ed.proc.erase.load()) { ed.proc.erasePad (pad); return; }
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

// ======================= LCD =======================
juce::Rectangle<float> Lcd::wave() const { return getLocalBounds().toFloat().reduced (18, 0).withTrimmedTop (48).withTrimmedBottom (44); }
void Lcd::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    insetPanel (g, b, 10);
    auto s = b.reduced (6);
    juce::ColourGradient bg (Col::lcd1, s.getCentreX(), s.getCentreY(), Col::lcd2, s.getX(), s.getY(), true);
    g.setGradientFill (bg); g.fillRoundedRectangle (s, 6);
    auto& P = ed.proc; const int p = ed.sel; auto smp = P.getSample (p); const auto pc = padColour (P.pad[p].colour.load());
    // griglia del display
    g.setColour (Col::phos.withAlpha (0.035f));
    for (float x = s.getX(); x < s.getRight(); x += 12) g.drawVerticalLine ((int) x, s.getY(), s.getBottom());
    for (float y = s.getY(); y < s.getBottom(); y += 12) g.drawHorizontalLine ((int) y, s.getX(), s.getRight());
    // intestazione
    auto hdr = juce::Rectangle<float> (s.getX() + 12, s.getY() + 8, s.getWidth() - 24, 22);
    g.setColour (pc); g.fillRoundedRectangle (hdr.getX(), hdr.getY() + 3, 4, 16, 2);
    glowText (g, "PAD " + juce::String (p + 1).paddedLeft ('0', 2) + "  " + noteName (P.rootNote() + p) + "  " + (smp ? smp->name.toUpperCase() : juce::String ("VUOTO")), hdr.withTrimmedLeft (12), Col::phos, ft (14.0f, true, true));
    juce::String rt; rt << juce::String (P.uiBpm.load(), 1) << " BPM  " << (P.uiHost.load() ? "HOST" : P.uiRunning.load() ? "INT" : "STOP");
    glowText (g, rt, hdr, P.recording.load() ? Col::red : Col::phos.withAlpha (0.8f), ft (11.0f, false, true), juce::Justification::centredRight);
    // forma d'onda
    auto w = wave();
    g.setColour (Col::phos.withAlpha (0.12f)); g.drawHorizontalLine ((int) w.getCentreY(), w.getX(), w.getRight());
    if (smp)
    {
        const int n = smp->buf.getNumSamples(); const float* d = smp->buf.getReadPointer (0); const int Wd = (int) w.getWidth();
        const float s0 = P.pad[p].get (START), s1 = P.pad[p].get (END);
        juce::Path top, fill;
        for (int x = 0; x < Wd; ++x)
        {
            const int a = (int) ((double) x / Wd * n), e = juce::jmin (n, (int) ((double) (x + 1) / Wd * n) + 1);
            float mx = 0; for (int i = a; i < e; i += 3) mx = juce::jmax (mx, std::abs (d[i]));
            const float fx = (float) x / Wd; const bool in = fx >= s0 && fx <= s1;
            const float h = mx * w.getHeight() * 0.47f;
            g.setColour (in ? pc.withAlpha (0.85f) : Col::phos.withAlpha (0.15f));
            g.drawVerticalLine ((int) w.getX() + x, w.getCentreY() - h, w.getCentreY() + h + 1);
        }
        // regione scura fuori da start/end
        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.fillRect (w.withWidth (w.getWidth() * s0)); g.fillRect (w.withTrimmedLeft (w.getWidth() * s1));
        for (int k = 0; k < 2; ++k)
        {
            const float x = w.getX() + (k ? s1 : s0) * w.getWidth();
            g.setColour (Col::phos); g.drawLine (x, w.getY() - 4, x, w.getBottom() + 4, 1.5f);
            juce::Path t; t.addTriangle (x - 6, w.getY() - 6, x + 6, w.getY() - 6, x, w.getY() + 3); g.fillPath (t);
            glowText (g, k ? "END" : "START", juce::Rectangle<float> (k ? x - 44 : x + 4, w.getBottom() - 14, 40, 12), Col::phos, ft (9.0f, true, true), k ? juce::Justification::centredRight : juce::Justification::centredLeft);
        }
        // inviluppo sovrapposto
        const float atk = P.pad[p].get (ATTACK), dec = P.pad[p].get (DECAY), len = (float) (n / smp->sr * 1000.0) * (s1 - s0);
        juce::Path env; const float x0 = w.getX() + s0 * w.getWidth(), xw = (s1 - s0) * w.getWidth();
        env.startNewSubPath (x0, w.getBottom());
        const float ax = x0 + xw * juce::jmin (1.0f, atk / juce::jmax (1.0f, len));
        env.lineTo (ax, w.getY() + 4);
        for (int i = 1; i <= 40; ++i)
        {
            const float t = i / 40.0f, xx = ax + (x0 + xw - ax) * t, ms = (len - atk) * t;
            const float lv = dec >= 3990 ? 1.0f : std::exp (-ms / (dec / 4.6f));
            env.lineTo (xx, w.getBottom() - (w.getHeight() - 4) * lv);
        }
        g.setColour (Col::amber.withAlpha (0.18f)); g.strokePath (env, juce::PathStrokeType (5.0f));
        g.setColour (Col::amber.withAlpha (0.85f)); g.strokePath (env, juce::PathStrokeType (1.4f));
    }
    else glowText (g, "TRASCINA UN CAMPIONE SUL PAD", w, Col::phos.withAlpha (0.6f), ft (13.0f, true, true), juce::Justification::centred);
    // riga inferiore: velocity e info
    auto foot = juce::Rectangle<float> (s.getX() + 12, s.getBottom() - 30, s.getWidth() - 24, 20);
    const int lv = P.lastVel[p].load(); const float vv = P.velCurve (lv);
    glowText (g, "VEL", foot, Col::phos.withAlpha (0.7f), ft (10.0f, true, true));
    auto bar = juce::Rectangle<float> (foot.getX() + 34, foot.getCentreY() - 4, 200, 8);
    for (int i = 0; i < 32; ++i)
    {
        const float t = i / 31.0f; auto seg = juce::Rectangle<float> (bar.getX() + i * 6.25f, bar.getY(), 4.5f, 8);
        g.setColour (t <= lv / 127.0f ? (t > 0.85f ? Col::magenta : Col::phos) : Col::phos.withAlpha (0.1f)); g.fillRect (seg);
    }
    glowText (g, juce::String (lv).paddedLeft (' ', 3) + "  >  " + juce::String (juce::roundToInt (vv * 100)) + "%", foot.withTrimmedLeft (244), Col::phos, ft (10.5f, true, true));
    if (smp) glowText (g, juce::String (smp->buf.getNumSamples() / smp->sr, 2) + " s  " + juce::String (smp->sr / 1000.0, 1) + " kHz", foot, Col::phos.withAlpha (0.6f), ft (10.0f, false, true), juce::Justification::centredRight);
    // righe di scansione e vetro
    g.setColour (juce::Colours::black.withAlpha (0.22f)); for (float y = s.getY(); y < s.getBottom(); y += 3) g.drawHorizontalLine ((int) y, s.getX(), s.getRight());
    juce::ColourGradient gl (juce::Colours::white.withAlpha (0.06f), s.getX(), s.getY(), juce::Colours::transparentWhite, s.getX() + s.getWidth() * 0.35f, s.getY() + s.getHeight() * 0.6f, false);
    g.setGradientFill (gl); g.fillRoundedRectangle (s, 6);
}
void Lcd::mouseDown (const juce::MouseEvent& e)
{
    auto w = wave(); if (! w.expanded (6, 10).contains (e.position)) { marker = -1; return; }
    auto& P = ed.proc.pad[ed.sel]; const float fx = (e.position.x - w.getX()) / w.getWidth();
    marker = std::abs (fx - P.get (START)) < std::abs (fx - P.get (END)) ? 0 : 1;
    mouseDrag (e);
}
void Lcd::mouseDrag (const juce::MouseEvent& e)
{
    if (marker < 0) return;
    auto w = wave(); auto& P = ed.proc.pad[ed.sel]; const float fx = juce::jlimit (0.0f, 1.0f, (e.position.x - w.getX()) / w.getWidth());
    if (marker == 0) P.v[START] = juce::jmin (fx, P.get (END) - 0.002f); else P.v[END] = juce::jmax (fx, P.get (START) + 0.002f);
    repaint(); ed.repaint();
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
          const double q = P.quantize (e.tick); const float x = a.getX() + a.getWidth() * (float) (q / L), y = a.getBottom() - (e.pad + 1) * rh;
          const auto c = padColour (P.pad[e.pad].colour.load());
          g.setColour (c.withAlpha (0.3f + 0.7f * e.vel / 127.0f)); g.fillRoundedRectangle (x - 1, y + 0.5f, 4, juce::jmax (1.5f, rh - 1), 1);
      } }
    if (P.uiRunning.load())
    {
        const double t = std::fmod (P.uiTick.load(), L); const float x = a.getX() + a.getWidth() * (float) (t / L);
        g.setColour (Col::magenta.withAlpha (0.3f)); g.fillRect (x - 3, a.getY(), 6.0f, a.getHeight());
        g.setColour (Col::magenta); g.drawVerticalLine ((int) x, a.getY(), a.getBottom());
    }
    if (P.events.empty()) glowText (g, "PREMI REC E SUONA: IL PATTERN SI QUANTIZZA DA SOLO", a, Col::phos.withAlpha (0.35f), ft (10.0f, true, true), juce::Justification::centred);
}

// ======================= Tastiera =======================
static bool isBlack (int n) { const int k = ((n % 12) + 12) % 12; return k == 1 || k == 3 || k == 6 || k == 8 || k == 10; }
int Keys::noteAt (juce::Point<float> p) const
{
    const int first = 12 * (ed.octave + 2); const int nW = 21; const float ww = getWidth() / (float) nW;
    // prima i tasti neri
    int wi = 0;
    for (int n = first; wi < nW; ++n)
    {
        if (isBlack (n)) { auto r = juce::Rectangle<float> (wi * ww - ww * 0.32f, 0, ww * 0.64f, getHeight() * 0.6f); if (r.contains (p)) return n; }
        else ++wi;
    }
    wi = 0;
    for (int n = first; wi < nW; ++n) if (! isBlack (n)) { if (juce::Rectangle<float> (wi * ww, 0, ww, (float) getHeight()).contains (p)) return n; ++wi; }
    return -1;
}
void Keys::paint (juce::Graphics& g)
{
    const int first = 12 * (ed.octave + 2), root = ed.proc.rootNote(); const int nW = 21; const float ww = getWidth() / (float) nW, H = (float) getHeight();
    static const char* letters = "AWSEDFTGYHUJKOLP;";
    auto lit = [this, root] (int n) { const int p = n - root; return ed.heldNotes.count (n) > 0 || (p >= 0 && p < kPads && ed.proc.padGlow[p].load() > 0.08f); };
    auto letter = [first] (int n) { const int k = n - first; return k >= 0 && k < 17 ? juce::String::charToString ((juce::juce_wchar) letters[k]) : juce::String(); };
    int wi = 0;
    for (int n = first; wi < nW; ++n)
    {
        if (isBlack (n)) continue;
        auto r = juce::Rectangle<float> (wi * ww + 1, 0, ww - 2, H);
        const int p = n - root; const bool inPads = p >= 0 && p < kPads, on = lit (n);
        juce::ColourGradient cg (on ? juce::Colour (0xffe9f6ff) : juce::Colour (0xffd5d8de), 0, 0, on ? juce::Colour (0xffb8c8d6) : juce::Colour (0xff9da2ab), 0, H, false);
        g.setGradientFill (cg); g.fillRoundedRectangle (r, 3);
        if (inPads) { const auto c = padColour (ed.proc.pad[p].colour.load()); g.setColour (on ? c : c.withAlpha (0.55f)); g.fillRoundedRectangle (r.getX() + 3, H - 8, r.getWidth() - 6, 4, 2); if (on) { g.setColour (c.withAlpha (0.35f)); g.fillRoundedRectangle (r, 3); } }
        if (n % 12 == 0) { g.setColour (juce::Colour (0xff3a3f4a)); g.setFont (ft (8.5f, true, true)); g.drawText (noteName (n), r.withTrimmedBottom (10).toNearestInt(), juce::Justification::centredBottom); }
        const auto L = letter (n); if (L.isNotEmpty()) { g.setColour (juce::Colour (0xff23262e)); g.setFont (ft (9.0f, true, true)); g.drawText (L, r.withTrimmedBottom (24).toNearestInt(), juce::Justification::centredBottom); }
        ++wi;
    }
    wi = 0;
    for (int n = first; wi < nW; ++n)
    {
        if (! isBlack (n)) { ++wi; continue; }
        auto r = juce::Rectangle<float> (wi * ww - ww * 0.32f, 0, ww * 0.64f, H * 0.6f);
        const int p = n - root; const bool inPads = p >= 0 && p < kPads, on = lit (n);
        juce::ColourGradient cg (on ? juce::Colour (0xff4a5060) : juce::Colour (0xff2a2d35), 0, 0, juce::Colour (0xff0b0c0f), 0, r.getBottom(), false);
        g.setGradientFill (cg); g.fillRoundedRectangle (r, 2.5f);
        if (inPads) { const auto c = padColour (ed.proc.pad[p].colour.load()); g.setColour (on ? c : c.withAlpha (0.6f)); g.fillRoundedRectangle (r.getX() + 3, r.getBottom() - 7, r.getWidth() - 6, 3, 1.5f); }
        const auto L = letter (n); if (L.isNotEmpty()) { g.setColour (juce::Colours::white.withAlpha (0.7f)); g.setFont (ft (8.5f, true, true)); g.drawText (L, r.withTrimmedBottom (10).toNearestInt(), juce::Justification::centredBottom); }
    }
}
void Keys::mouseDown (const juce::MouseEvent& e)
{
    mouseNote = noteAt (e.position); if (mouseNote < 0) return;
    const int p = mouseNote - ed.proc.rootNote(); ed.heldNotes.insert (mouseNote);
    if (p >= 0 && p < kPads) { ed.selectPad (p); ed.playPad (p, ed.fullLevel ? 127 : ed.kbVel); }
    repaint();
}
void Keys::mouseUp (const juce::MouseEvent&)
{
    if (mouseNote < 0) return; ed.heldNotes.erase (mouseNote);
    const int p = mouseNote - ed.proc.rootNote(); if (p >= 0 && p < kPads) ed.stopPad (p);
    mouseNote = -1; repaint();
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
    const auto f = ed.proc.writeMidi();
    juce::DragAndDropContainer::performExternalDragDropOfFiles ({ f.getFullPathName() }, false, this, [this] { dragging = false; repaint(); });
}

// ======================= Root =======================
void Root::paint (juce::Graphics& g)
{
    const float W = (float) Editor::W, H = (float) Editor::H;
    juce::ColourGradient body (Col::chassis1, 0, 0, Col::chassis2, 0, H, false); g.setGradientFill (body); g.fillAll();
    // metallo spazzolato
    juce::Random rr (7);
    for (int y = 0; y < (int) H; y += 2) { g.setColour (juce::Colours::white.withAlpha (rr.nextFloat() * 0.018f)); g.drawHorizontalLine (y, 0, W); }
    // luce ambientale al neon
    juce::ColourGradient amb (Col::magenta.withAlpha (0.10f), W * 0.78f, 300, juce::Colours::transparentBlack, W * 0.78f + 520, 300, true); g.setGradientFill (amb); g.fillRect (0.0f, 0.0f, W, H);
    juce::ColourGradient amb2 (Col::cyan.withAlpha (0.06f), 260, 200, juce::Colours::transparentBlack, 760, 200, true); g.setGradientFill (amb2); g.fillRect (0.0f, 0.0f, W, H);
    // intestazione
    auto hdr = juce::Rectangle<float> (0, 0, W, 66);
    juce::ColourGradient hg (juce::Colour (0xff22252d), 0, 0, juce::Colour (0xff121419), 0, 66, false); g.setGradientFill (hg); g.fillRect (hdr);
    g.setColour (juce::Colours::black); g.drawHorizontalLine (66, 0, W);
    g.setColour (Col::magenta.withAlpha (0.8f)); g.fillRect (0.0f, 64.0f, W * 0.42f, 2.0f);
    g.setColour (Col::cyan.withAlpha (0.8f)); g.fillRect (W * 0.42f, 64.0f, W * 0.58f, 2.0f);
    glowText (g, "JACKPOT", { 28, 12, 200, 40 }, Col::text, ft (30.0f, true));
    glowText (g, "PAD", { 168, 12, 100, 40 }, Col::magenta, ft (30.0f, true));
    g.setFont (ft (10.0f, true, true)); g.setColour (Col::dim);
    g.drawText (juce::String::fromUTF8 ("JP-16  \xc2\xb7  DRUM SAMPLER  \xc2\xb7  VELOCITY ENGINE  \xc2\xb7  SWING QUANTIZE"), juce::Rectangle<float> (250, 12, 520, 40), juce::Justification::centredLeft);
    // griglia di aerazione
    for (int i = 0; i < 14; ++i) { auto r = juce::Rectangle<float> (W - 380 + i * 11.0f, 20, 5, 26); g.setColour (juce::Colours::black.withAlpha (0.8f)); g.fillRoundedRectangle (r, 2.5f); g.setColour (juce::Colours::white.withAlpha (0.06f)); g.drawLine (r.getX(), r.getBottom() + 1, r.getRight(), r.getBottom() + 1); }
    // striscia di pericolo
    g.saveState(); auto hz = juce::Rectangle<float> (W - 210, 22, 70, 22); g.reduceClipRegion (hz.toNearestInt());
    g.setColour (Col::amber.withAlpha (0.85f)); g.fillRect (hz);
    g.setColour (juce::Colours::black.withAlpha (0.85f)); for (float x = hz.getX() - 30; x < hz.getRight(); x += 12) { juce::Path p; p.addQuadrilateral (x, hz.getBottom(), x + 6, hz.getBottom(), x + 28, hz.getY(), x + 22, hz.getY()); g.fillPath (p); }
    g.restoreState();
    for (auto pt : { juce::Point<float> (12, 80), juce::Point<float> (W - 12, 80), juce::Point<float> (12, H - 12), juce::Point<float> (W - 12, H - 12) }) screw (g, pt.x, pt.y);
    // pannelli
    insetPanel (g, { 724, 82, 496, 496 }, 14);
    g.setFont (ft (9.5f, true)); g.setColour (Col::faint);
    const char* rows[] = { "SAMPLE", "ENV / FILTRO", "VELOCITY" };
    for (int i = 0; i < 3; ++i)
    {
        auto r = juce::Rectangle<float> (24, 346.0f + i * 88, 680, 82);
        g.setColour (juce::Colours::black.withAlpha (0.25f)); g.fillRoundedRectangle (r, 8);
        g.setColour (juce::Colours::white.withAlpha (0.03f)); g.drawRoundedRectangle (r, 8, 1.0f);
        g.saveState(); g.addTransform (juce::AffineTransform::rotation (-juce::MathConstants<float>::halfPi, r.getX() + 14, r.getCentreY()));
        g.setColour (i == 2 ? Col::magenta : Col::cyan); g.setFont (ft (9.5f, true)); g.drawText (rows[i], juce::Rectangle<float> (r.getX() + 14 - 40, r.getCentreY() - 6, 80, 12), juce::Justification::centred);
        g.restoreState();
    }
    // barra inferiore
    g.setColour (juce::Colours::black.withAlpha (0.35f)); g.fillRoundedRectangle (24, 616, 680, 206, 10);
    g.setColour (juce::Colours::white.withAlpha (0.03f)); g.drawRoundedRectangle (24, 616, 680, 206, 10, 1.0f);
    glowText (g, "SEQUENCER", { 36, 620, 160, 18 }, Col::cyan, ft (10.0f, true));
    glowText (g, "TASTIERA", { 726, 646, 120, 18 }, Col::cyan, ft (10.0f, true));
    g.setFont (ft (10.0f, true, true)); g.setColour (Col::text);
    g.drawText ("OCT " + noteName (12 * (ed.octave + 2)) + "   VEL " + juce::String (ed.fullLevel ? 127 : ed.kbVel), juce::Rectangle<float> (810, 646, 200, 18), juce::Justification::centredLeft);
    g.setColour (Col::dim); g.setFont (ft (9.5f));
    g.drawText ("Z / X ottava    C / V velocity", juce::Rectangle<float> (1010, 646, 210, 18), juce::Justification::centredRight);
    g.setFont (ft (9.0f)); g.setColour (Col::faint);
    g.drawText (juce::String::fromUTF8 ("JACKPOT.WAV \xc2\xb7 JP-16 \xc2\xb7 AU"), juce::Rectangle<float> (726, H - 22, 300, 14), juce::Justification::centredLeft);
}

// ======================= Editor =======================
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

static float toNorm (int i, float v)
{
    const auto& p = kPInfo[i];
    if (p.kind == 4 || p.kind == 5) { const float mn = juce::jmax (1.0f, p.min); return std::log (juce::jmax (mn, v) / mn) / std::log (p.max / mn); }
    return (v - p.min) / (p.max - p.min);
}
static float fromNorm (int i, float n)
{
    const auto& p = kPInfo[i];
    if (p.kind == 4 || p.kind == 5) { const float mn = juce::jmax (1.0f, p.min); float v = mn * std::pow (p.max / mn, n); if (i == ATTACK && n < 0.02f) v = 0; return v; }
    float v = p.min + n * (p.max - p.min);
    if (p.kind == 2 || p.kind == 3 || p.kind == 6 || p.kind == 7 || p.kind == 8) v = std::round (v);
    return v;
}

Editor::Editor (Processor& p) : AudioProcessorEditor (&p), proc (p)
{
    addAndMakeVisible (root);
    root.addAndMakeVisible (lcd); root.addAndMakeVisible (pattern); root.addAndMakeVisible (keys); root.addAndMakeVisible (drag);
    for (int i = 0; i < kPads; ++i) root.addAndMakeVisible (pads.add (new PadComp (*this, i)));

    // controlli del pad selezionato
    auto padKnob = [this] (int i, juce::Colour c)
    {
        auto* k = padKnobs.add (new Knob (kPInfo[i].label, c));
        k->bipolar = kPInfo[i].min < 0;
        k->get = [this, i] { return toNorm (i, proc.pad[sel].get (i)); };
        k->set = [this, i] (float n) { auto& P = proc.pad[sel]; float v = fromNorm (i, n);
                                       if (i == START) v = juce::jmin (v, P.get (END) - 0.002f); if (i == END) v = juce::jmax (v, P.get (START) + 0.002f);
                                       P.v[i] = v; lcd.repaint(); };
        k->reset = [this, i] { proc.pad[sel].v[i] = kPInfo[i].def; lcd.repaint(); };
        k->text = [this, i] { return formatPadParam (i, proc.pad[sel].get (i)); };
        root.addAndMakeVisible (k); padCtl[i] = k;
    };
    auto padSel = [this] (int i, int n)
    {
        auto* s = selectors.add (new Selector (kPInfo[i].label));
        s->get = [this, i] { return juce::roundToInt (proc.pad[sel].get (i)); };
        s->set = [this, i] (int v) { proc.pad[sel].v[i] = (float) v; };
        s->count = [n] { return n; };
        s->name = [i] (int v) { return formatPadParam (i, (float) v); };
        root.addAndMakeVisible (s); padCtl[i] = s;
    };
    for (int i : { LEVEL, PAN, TUNE, FINE, START, END }) padKnob (i, Col::cyan);
    for (int i : { ATTACK, DECAY, RELEASE, CUTOFF, RESO }) padKnob (i, Col::cyan);
    for (int i : { VLEVEL, VFILTER, VSTART, VPITCH }) padKnob (i, Col::magenta);
    padSel (FTYPE, 3); padSel (MODE, 2); padSel (CHOKE, 9);
    {
        auto* b = btns.add (new Btn ("REVERSE", Col::amber)); root.addAndMakeVisible (b); padCtl[REVERSE] = b;
        b->lit = [this] { return proc.pad[sel].get (REVERSE) > 0.5f; };
        b->click = [this] { auto& v = proc.pad[sel].v[REVERSE]; v = v.load() > 0.5f ? 0.0f : 1.0f; lcd.repaint(); };
    }
    // globali
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
    choice ("len", "LUNGHEZZA", Col::cyan);
    choice ("grid", "QUANTIZZA", Col::cyan);
    {
        auto* pr = proc.apvts.getParameter ("root");
        auto* s = selectors.add (new Selector ("NOTA PAD 1")); s->setName ("root");
        s->get = [pr] { return juce::roundToInt (pr->convertFrom0to1 (pr->getValue())); };
        s->set = [this, pr] (int v) { pr->setValueNotifyingHost (pr->convertTo0to1 ((float) v)); for (auto* pd : pads) pd->repaint(); keys.repaint(); };
        s->count = [] { return 113; };
        s->name = [] (int v) { return noteName (v); };
        root.addAndMakeVisible (s);
    }
    paramKnob ("swing", "SWING", Col::magenta, [] (float v) { return juce::String (v, 1) + "%"; });
    paramKnob ("strength", "STRENGTH", Col::cyan, [] (float v) { return juce::String (juce::roundToInt (v)) + "%"; });
    paramKnob ("bpm", "BPM INT", Col::amber, [] (float v) { return juce::String (v, 1); });
    paramKnob ("out", "OUTPUT", Col::cyan, [] (float v) { return juce::String (v, 1) + " dB"; });

    auto btn = [this] (const juce::String& t, juce::Colour c, std::function<bool()> lit, std::function<void()> click)
    { auto* b = btns.add (new Btn (t, c)); b->lit = std::move (lit); b->click = std::move (click); root.addAndMakeVisible (b); return b; };
    btn ("REC", Col::red, [this] { return proc.recording.load() && ((juce::Time::getMillisecondCounter() / 300) % 2 == 0 || proc.uiRunning.load()); },
         [this] { const bool r = ! proc.recording.load(); proc.recording = r; if (r && ! proc.uiRunning.load()) proc.requestPlay (true); })->setName ("rec");
    btn ("PLAY", Col::green, [this] { return proc.uiRunning.load(); }, [this] { proc.requestPlay (true); })->setName ("play");
    btn ("STOP", Col::text, [] { return false; }, [this] { proc.requestPlay (false); })->setName ("stop");
    btn ("CLEAR", Col::amber, [] { return false; }, [this] { proc.clearPattern(); })->setName ("clear");
    btn ("ERASE", Col::red, [this] { return proc.erase.load(); }, [this] { proc.erase = ! proc.erase.load(); })->setName ("erase");
    btn ("CLICK", Col::cyan, [this] { return proc.apvts.getRawParameterValue ("click")->load() > 0.5f; },
         [this] { auto* pr = proc.apvts.getParameter ("click"); pr->setValueNotifyingHost (pr->getValue() > 0.5f ? 0.0f : 1.0f); })->setName ("click");
    btn ("FULL LEVEL", Col::magenta, [this] { return fullLevel; }, [this] { fullLevel = ! fullLevel; root.repaint(); })->setName ("full");
    btn ("LOAD", Col::cyan, [] { return false; }, [this] { loadDialog (sel); })->setName ("load");

    setWantsKeyboardFocus (true);
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
    lcd.setBounds (24, 82, 680, 252);
    // righe di controlli del pad
    auto cell = [] (int row, int col) { return juce::Rectangle<int> (52 + col * 93, 350 + row * 88, 84, 76); };
    const int r0[] = { LEVEL, PAN, TUNE, FINE, START, END };
    for (int c = 0; c < 6; ++c) padCtl[r0[c]]->setBounds (cell (0, c));
    padCtl[REVERSE]->setBounds (cell (0, 6).reduced (2, 20));
    const int r1[] = { ATTACK, DECAY, RELEASE };
    for (int c = 0; c < 3; ++c) padCtl[r1[c]]->setBounds (cell (1, c));
    padCtl[FTYPE]->setBounds (cell (1, 3).withSizeKeepingCentre (96, 40).translated (0, -2));
    padCtl[CUTOFF]->setBounds (cell (1, 4).translated (14, 0)); padCtl[RESO]->setBounds (cell (1, 5).translated (14, 0));
    const int r2[] = { VLEVEL, VFILTER, VSTART, VPITCH };
    for (int c = 0; c < 4; ++c) padCtl[r2[c]]->setBounds (cell (2, c));
    padCtl[MODE]->setBounds (cell (2, 4).withSizeKeepingCentre (90, 40).translated (0, -2));
    padCtl[CHOKE]->setBounds (cell (2, 5).withSizeKeepingCentre (70, 40).translated (14, -2));
    // pad: pad 1 in basso a sinistra
    for (int i = 0; i < kPads; ++i) { const int r = 3 - i / 4, c = i % 4; pads[i]->setBounds (726 + c * 123, 84 + r * 123, 123, 123); }
    // sotto i pad
    named (root, "curve")->setBounds (726, 586, 150, 40);
    named (root, "full")->setBounds (884, 598, 100, 30);
    named (root, "load")->setBounds (990, 598, 70, 30);
    globalKnobs[3]->setBounds (1066, 576, 70, 64);
    named (root, "root")->setBounds (1140, 586, 80, 40);
    // sequencer
    const char* tb[] = { "rec", "play", "stop", "clear", "erase", "click" };
    for (int i = 0; i < 6; ++i) named (root, tb[i])->setBounds (36 + i * 70, 644, 66, 40);
    named (root, "len")->setBounds (460, 640, 100, 44);
    named (root, "grid")->setBounds (568, 640, 100, 44);
    pattern.setBounds (36, 694, 470, 118);
    globalKnobs[0]->setBounds (512, 694, 64, 78); globalKnobs[1]->setBounds (578, 694, 64, 78); globalKnobs[2]->setBounds (638, 694, 64, 78);
    drag.setBounds (514, 778, 186, 34);
    keys.setBounds (726, 668, 494, 140);
}

void Editor::selectPad (int p) { sel = juce::jlimit (0, kPads - 1, p); root.repaint(); }
void Editor::playPad (int p, int vel) { proc.uiNoteOn (p, vel); }
void Editor::stopPad (int p) { proc.uiNoteOff (p); }

void Editor::loadDialog (int p)
{
    chooser = std::make_unique<juce::FileChooser> ("Campione per il pad " + juce::String (p + 1), juce::File(), proc.formats.getWildcardForAllFormats());
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [this, p] (const juce::FileChooser& fc) { const auto f = fc.getResult(); if (f.existsAsFile()) proc.loadFile (f, p); root.repaint(); });
}
void Editor::padMenu (int p)
{
    juce::PopupMenu m, cm;
    m.addItem (1, "Carica campione...");
    m.addItem (2, "Svuota pad");
    m.addItem (3, "Cancella le note di questo pad dal pattern");
    m.addItem (4, "Ripristina parametri");
    static const char* names[] = { "Magenta", "Arancio", "Ambra", "Giallo", "Lime", "Menta", "Ciano", "Blu", "Viola", "Fucsia", "Rosa", "Mandarino", "Acqua", "Acido", "Rosso", "Lavanda" };
    for (int i = 0; i < 16; ++i) cm.addColouredItem (100 + i, names[i], padColour (i), true, proc.pad[p].colour.load() == i);
    m.addSubMenu ("Colore", cm);
    m.addSeparator(); m.addItem (5, "Ricarica il kit di fabbrica (tutti i pad)");
    m.showMenuAsync (juce::PopupMenu::Options(), [this, p] (int r)
    {
        if (r == 1) loadDialog (p);
        else if (r == 2) proc.clearPad (p);
        else if (r == 3) proc.erasePad (p);
        else if (r == 4) { const int c = proc.pad[p].colour.load(); proc.pad[p].reset(); proc.pad[p].colour = c; }
        else if (r == 5) proc.loadDefaultKit();
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
    lcd.repaint(); pattern.repaint(); keys.repaint();
    for (auto* b : btns) b->repaint();
    for (auto* k : padKnobs) k->repaint();
    for (auto* k : globalKnobs) k->repaint();
    for (auto* s : selectors) s->repaint();
}

// tastiera del computer come la tastiera MIDI di Ableton
int Editor::keyNote (int code) const
{
    static const char* letters = "awsedftgyhujkolp;";
    const auto c = juce::CharacterFunctions::toLowerCase ((juce::juce_wchar) code);
    for (int i = 0; letters[i]; ++i) if (letters[i] == c) return 12 * (octave + 2) + i;
    return -1;
}
bool Editor::keyPressed (const juce::KeyPress& k)
{
    const auto c = juce::CharacterFunctions::toLowerCase ((juce::juce_wchar) k.getKeyCode());
    static const int vels[] = { 1, 20, 40, 60, 80, 100, 120, 127 };
    auto velStep = [this] (int d) { int i = 0; while (i < 7 && vels[i] < kbVel) ++i; kbVel = vels[juce::jlimit (0, 7, i + d)]; root.repaint(); };
    if (c == 'z') { octave = juce::jmax (-2, octave - 1); root.repaint(); return true; }
    if (c == 'x') { octave = juce::jmin (7, octave + 1); root.repaint(); return true; }
    if (c == 'c') { velStep (-1); return true; }
    if (c == 'v') { velStep (1); return true; }
    if (k == juce::KeyPress::spaceKey) { proc.requestPlay (! proc.uiRunning.load()); return true; }
    return keyNote (k.getKeyCode()) >= 0;
}
bool Editor::keyStateChanged (bool)
{
    static const char* letters = "awsedftgyhujkolp;";
    bool used = false; const int root = proc.rootNote();
    for (int i = 0; letters[i]; ++i)
    {
        const int code = letters[i];
        const bool d = juce::KeyPress::isKeyCurrentlyDown (code) || juce::KeyPress::isKeyCurrentlyDown (juce::CharacterFunctions::toUpperCase ((juce::juce_wchar) code));
        const int note = 12 * (octave + 2) + i, p = note - root;
        if (d && ! keysDown.count (i))
        {
            keysDown.insert (i); heldNotes.insert (note); used = true;
            if (p >= 0 && p < kPads) { playPad (p, fullLevel ? 127 : kbVel); sel = p; }
        }
        else if (! d && keysDown.count (i))
        {
            keysDown.erase (i); used = true;
            for (int o = -3; o <= 9; ++o) heldNotes.erase (12 * (o + 2) + i);
            if (p >= 0 && p < kPads) stopPad (p);
        }
    }
    return used;
}
}
