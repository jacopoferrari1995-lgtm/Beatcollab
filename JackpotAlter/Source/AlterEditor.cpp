#include "AlterEditor.h"

namespace ja
{
juce::Font uf (float size, bool bold) { return jlook::ui (size, bold ? 2 : 1); }
juce::Font mf (float size) { return jlook::mono (size); }
void glowText (juce::Graphics& g, const juce::String& t, juce::Rectangle<float> r, juce::Colour c, juce::Font f, juce::Justification j)
{
    g.setFont (f);
    g.setColour (c.withAlpha (0.2f)); g.drawText (t, r.translated (-0.7f, 0.3f), j); g.drawText (t, r.translated (0.7f, 0.3f), j);
    g.setColour (c); g.drawText (t, r, j);
}
void lcd (juce::Graphics& g, juce::Rectangle<float> r, float rad)
{
    if (jlook::skinHooks().on && jlook::skinHooks().screen) { jlook::skinHooks().screen (g, r, rad, C::phos); return; }
    g.setColour (juce::Colours::black.withAlpha (0.65f)); g.fillRoundedRectangle (r.expanded (1.5f), rad + 1.5f);
    juce::ColourGradient cg (C::lcd1, r.getCentreX(), r.getCentreY(), C::lcd2, r.getX(), r.getY(), true); g.setGradientFill (cg); g.fillRoundedRectangle (r, rad);
    g.setColour (C::phos.withAlpha (0.035f));
    for (float x = r.getX() + 10; x < r.getRight(); x += 12) g.drawVerticalLine ((int) x, r.getY() + 2, r.getBottom() - 2);
    for (float y = r.getY() + 10; y < r.getBottom(); y += 12) g.drawHorizontalLine ((int) y, r.getX() + 2, r.getRight() - 2);
}
static juce::String hzS (float f) { return f >= 1000 ? juce::String (f / 1000.0f, f >= 10000 ? 0 : 1) + "k" : juce::String (juce::roundToInt (f)); }
static juce::String semi (float v) { return std::abs (v) < 0.005f ? juce::String ("0") : (v > 0 ? "+" : "") + juce::String (v, std::abs (v - std::round (v)) < 0.005f ? 0 : 2) + " st"; }

// frontali: ogni unita' e' una macchina diversa
struct Face { juce::uint32 p1, p2, ink, acc; int knob; };
static const Face& face (int u)
{
    static const Face F[4] = {
        { 0xff2a2340, 0xff14101f, 0xffe6e0f4, 0xffa66bff, 2 },     // pitch
        { 0xff4d1716, 0xff2a0c0b, 0xfff1dcc4, 0xffff2e88, 0 },     // drive
        { 0xff202228, 0xff0f1013, 0xffe6e8ec, 0xff5ef2ff, 1 },     // multibanda
        { 0xff2a2a2e, 0xff121214, 0xffe9e2cf, 0xffffb000, 1 } };   // uscita
    return F[juce::jlimit (0, 3, u)];
}
static juce::Colour acc (int u) { return juce::Colour (face (u).acc); }

// ======================================================================= controlli
void Knob::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isAltDown() || e.mods.isCommandDown()) { param->setValueNotifyingHost (param->getDefaultValue()); return; }
    start = param->getValue(); param->beginChangeGesture(); drag = true; repaint();
}
void Knob::mouseDrag (const juce::MouseEvent& e)
{
    if (! drag) return;
    float v = juce::jlimit (0.0f, 1.0f, start - e.getDistanceFromDragStartY() / (e.mods.isShiftDown() ? 900.0f : 220.0f));
    if (snap) v = param->convertTo0to1 (snap (param->convertFrom0to1 (v), e.mods.isShiftDown()));
    param->setValueNotifyingHost (v);
}
void Knob::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    float v = juce::jlimit (0.0f, 1.0f, param->getValue() + (w.deltaY > 0 ? 0.01f : -0.01f));
    if (snap) { const float cur = param->convertFrom0to1 (param->getValue()); v = param->convertTo0to1 (snap (cur + (w.deltaY > 0 ? 1.0f : -1.0f) * (e.mods.isShiftDown() ? 0.05f : 1.0f), e.mods.isShiftDown())); }
    param->setValueNotifyingHost (v);
}
void Knob::paint (juce::Graphics& g)
{
    auto s = spec; s.enabled = lit; s.hover = hover; s.drag = drag; s.text = text ? text() : param->getCurrentValueAsText();
    hw::knob (g, getLocalBounds().toFloat(), param->getValue(), s);
}
void Segs::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat(); lcd (g, b, 5);
    const int n = juce::jmax (1, items.size()), cur = juce::roundToInt (param->convertFrom0to1 (param->getValue()));
    const float w = (b.getWidth() - 6) / n; const auto c = lit ? col : col.withSaturation (0.15f).withMultipliedBrightness (0.55f);
    for (int i = 0; i < n; ++i)
    {
        auto r = juce::Rectangle<float> (3 + i * w, 3, w, b.getHeight() - 6).reduced (1, 0);
        if (i == cur) { g.setColour (c.withAlpha (0.2f)); g.fillRoundedRectangle (r, 4); g.setColour (c.withAlpha (0.85f)); g.drawRoundedRectangle (r, 4, 1.0f); glowText (g, items[i], r, c, uf (font), juce::Justification::centred); }
        else { g.setColour (C::dim.withAlpha (lit ? 1.0f : 0.5f)); g.setFont (uf (font)); g.drawText (items[i], r, juce::Justification::centred); }
    }
}
void Segs::mouseDown (const juce::MouseEvent& e)
{
    const int n = juce::jmax (1, items.size()); const int k = juce::jlimit (0, n - 1, (int) ((e.position.x - 3) / ((getWidth() - 6) / (float) n)));
    param->setValueNotifyingHost (param->convertTo0to1 ((float) k)); repaint();
}
void Switch::paint (juce::Graphics& g)
{
    const bool on = param->getValue() > 0.5f; const float h = (float) juce::jmin (getHeight(), 18), w = h * 1.8f;
    auto t = juce::Rectangle<float> (0, (getHeight() - h) / 2, w, h); lcd (g, t, h / 2);
    const float d = h - 6, x = on ? t.getRight() - d - 3 : 3.0f;
    if (on) { g.setColour (col.withAlpha (0.35f)); g.fillEllipse (x - 3, t.getY(), d + 6, d + 6); }
    juce::ColourGradient lg (on ? col.brighter (0.6f) : juce::Colour (0xff4a4f5a), x, t.getY() + 3, on ? col : juce::Colour (0xff15171c), x, t.getY() + 3 + d, false); g.setGradientFill (lg); g.fillEllipse (x, t.getY() + 3, d, d);
    auto lr = juce::Rectangle<float> (w + 7, 0, getWidth() - w - 7, (float) getHeight());
    g.setColour (on ? ink : ink.withAlpha (0.55f)); g.setFont (uf (9.5f)); g.drawText (label, lr, juce::Justification::centredLeft);
}

// ======================================================================= display
juce::String Tuner::key() const
{
    auto& p = ed.proc; juce::String k; k << skin::cur() << "/" << p.value ("pitch") << "/" << p.value ("formant") << "/" << p.value ("mode") << "/" << p.value ("key") << "/" << p.value ("link");
    k << "/" << noteOf (p.pitch.inHz.load()) << "/" << juce::roundToInt (p.pitch.corrSemi.load() * 20);
    return k;
}
void Tuner::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat(); lcd (g, b, 7);
    auto& p = ed.proc; const bool on = ed.pitchOn(); const auto c = on ? acc (0) : C::faint;
    const int mode = juce::roundToInt (p.value ("mode")); auto r = b.reduced (12, 8);
    const float st = p.value ("pitch"), fo = p.value ("formant");
    // riga dei semitoni da -12 a +12, con il pitch e la formante
    auto line = r.removeFromBottom (24); const float cy = line.getCentreY();
    g.setColour (C::dim.withAlpha (0.4f)); g.drawHorizontalLine ((int) cy, line.getX(), line.getRight());
    for (int s = -12; s <= 12; ++s) { const float x = line.getX() + line.getWidth() * (s + 12) / 24.0f, h = s % 12 == 0 ? 7.0f : (s % 5 == 0 || s % 7 == 0) ? 4.5f : 2.5f; g.setColour (C::dim.withAlpha (s == 0 ? 0.9f : 0.45f)); g.drawVerticalLine ((int) x, cy - h, cy + h); }
    auto mark = [&] (float v, juce::Colour col, bool up)
    {
        const float x = line.getX() + line.getWidth() * (juce::jlimit (-12.0f, 12.0f, v) + 12) / 24.0f;
        juce::Path t; if (up) t.addTriangle (x - 5, cy - 11, x + 5, cy - 11, x, cy - 3); else t.addTriangle (x - 5, cy + 11, x + 5, cy + 11, x, cy + 3);
        g.setColour (col); g.fillPath (t);
    };
    mark (fo, C::amber.withAlpha (on ? 1.0f : 0.4f), false); mark (st, c, true);
    // nota d'ingresso -> nota d'uscita (QUANTIZZA / ROBOT), oppure lo spostamento
    auto top = r;
    if (mode != TRASPONI)
    {
        const float hz = p.pitch.inHz.load(), cs = p.pitch.corrSemi.load();
        glowText (g, hz > 0 ? noteOf (hz) : juce::String ("-"), top.removeFromLeft (top.getWidth() * 0.32f), C::text, mf (22.0f), juce::Justification::centred);
        g.setColour (C::dim); g.setFont (uf (14.0f)); g.drawText (juce::String::fromUTF8 ("\xe2\x86\x92"), top.removeFromLeft (30), juce::Justification::centred);
        glowText (g, hz > 0 ? noteOf (hz * std::pow (2.0f, (cs + st) / 12.0f)) : juce::String ("-"), top.removeFromLeft (top.getWidth() * 0.5f), c, mf (22.0f), juce::Justification::centred);
        g.setColour (C::dim); g.setFont (mf (9.5f));
        g.drawFittedText ((mode == ROBOT ? "ROBOT su " : "SCALA ") + noteNames()[juce::roundToInt (p.value ("key"))] + "\ncorrezione " + semi (cs), top.toNearestInt(), juce::Justification::centred, 2);
    }
    else
    {
        glowText (g, "PITCH " + semi (st), top.removeFromTop (top.getHeight() * 0.55f), c, mf (17.0f), juce::Justification::centred);
        g.setColour (C::amber.withAlpha (on ? 0.95f : 0.45f)); g.setFont (mf (10.5f));
        g.drawText (p.value ("link") > 0.5f ? juce::String ("FORMANTI CHE SEGUONO IL PITCH") : "FORMANTE " + semi (fo), top, juce::Justification::centred);
    }
    jlook::glass (g, b, 7);
}

juce::String Curve::key() const { auto& p = ed.proc; juce::String k; k << skin::cur() << "/" << p.value ("model") << "/" << p.value ("drive") << "/" << p.value ("tone"); return k; }
void Curve::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat(); lcd (g, b, 7);
    auto& p = ed.proc; const bool on = ed.driveOn(); const auto c = on ? acc (1) : C::faint;
    const int m = juce::roundToInt (p.value ("model")); const float d = p.value ("drive");
    auto r = b.reduced (10, 8);
    // due periodi di una sinusoide a 200 Hz passati nel modello vero (a 4x), dopo un giro di riscaldamento
    DriveChannel dc; const double fs = 192000; dc.prepare (fs, 4); dc.setModel (m); dc.setDrive (d);
    const float gIn = juce::Decibels::decibelsToGain (d * modelInfo (m).maxDb), w = juce::jlimit (0.0f, 1.0f, d / 0.05f);
    const int per = (int) (fs / 200), N = per * 2; std::vector<float> y ((size_t) N); float pk = 1e-6f;
    for (int i = -per * 6; i < N; ++i) { const float x = 0.5f * (float) std::sin (2 * juce::MathConstants<double>::pi * 200.0 * i / fs); const float v = x + (dc.process (x, gIn, d) - x) * w; if (i >= 0) { y[(size_t) i] = v; pk = juce::jmax (pk, std::abs (v)); } }
    juce::Path ref, out;
    for (int i = 0; i < N; i += 4)
    {
        const float x = r.getX() + r.getWidth() * i / (float) N, s0 = 0.5f * (float) std::sin (2 * juce::MathConstants<double>::pi * 200.0 * i / fs) / 0.5f;
        const float yy = r.getCentreY() - y[(size_t) i] / pk * r.getHeight() * 0.44f, yr = r.getCentreY() - s0 * r.getHeight() * 0.44f;
        if (i == 0) { out.startNewSubPath (x, yy); ref.startNewSubPath (x, yr); } else { out.lineTo (x, yy); ref.lineTo (x, yr); }
    }
    g.setColour (C::dim.withAlpha (0.35f)); g.strokePath (ref, juce::PathStrokeType (1.0f));
    g.setColour (c.withAlpha (0.3f)); g.strokePath (out, juce::PathStrokeType (4.0f)); g.setColour (c); g.strokePath (out, juce::PathStrokeType (1.5f));
    glowText (g, juce::String (modelInfo (m).name) + "  " + modelInfo (m).sub, r.withHeight (12), c, mf (9.5f), juce::Justification::centredLeft);
    glowText (g, juce::String (juce::roundToInt (d * 100)) + "%", r.withHeight (12), c, mf (9.5f), juce::Justification::centredRight);
    jlook::glass (g, b, 7);
}

float MbView::f2x (float f) const { auto r = getLocalBounds().toFloat().reduced (8); return r.getX() + r.getWidth() * std::log (juce::jlimit (20.0f, 20000.0f, f) / 20.0f) / std::log (1000.0f); }
float MbView::x2f (float x) const { auto r = getLocalBounds().toFloat().reduced (8); return 20.0f * std::pow (1000.0f, juce::jlimit (0.0f, 1.0f, (x - r.getX()) / r.getWidth())); }
void MbView::update()
{
    float tmp[2048]; ed.proc.ringOut.read (tmp, 2048);
    for (int i = 0; i < 2048; ++i) buf[(size_t) i] = tmp[i] * (0.5f - 0.5f * std::cos (6.2831853f * i / 2047.0f));
    std::fill (buf.begin() + 2048, buf.end(), 0.0f);
    fft.performFrequencyOnlyForwardTransform (buf.data());
    for (int i = 0; i < 1024; ++i) { const float db = juce::Decibels::gainToDecibels (buf[(size_t) i] / 512.0f, -100.0f); spec[(size_t) i] = db > spec[(size_t) i] ? db : spec[(size_t) i] * 0.85f + db * 0.15f; }
}
void MbView::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat(); lcd (g, b, 7);
    auto& p = ed.proc; const bool multi = p.value ("multi") > 0.5f, on = ed.driveOn();
    auto r = b.reduced (8); const double sr = p.sampleRate() > 0 ? p.sampleRate() : 48000.0;
    const float x1 = f2x (p.value ("x1")), x2 = f2x (p.value ("x2"));
    const float w[3] = { p.value ("wLo"), p.value ("wMid"), p.value ("wHi") };
    const juce::Colour bc[3] = { juce::Colour (0xffff6a3d), juce::Colour (0xffffd23d), juce::Colour (0xff5ef2ff) };
    const float edges[4] = { r.getX(), x1, x2, r.getRight() };
    if (multi)
        for (int k = 0; k < 3; ++k)
        {   // altezza della banda = quanto drive ci arriva
            auto br = juce::Rectangle<float> (edges[k], r.getY(), edges[k + 1] - edges[k], r.getHeight());
            const float h = br.getHeight() * juce::jlimit (0.0f, 1.0f, w[k] / 2.0f);
            g.setColour (bc[k].withAlpha (on ? 0.16f : 0.06f)); g.fillRect (br.withTop (br.getBottom() - h));
            g.setColour (bc[k].withAlpha (on ? 0.8f : 0.3f)); g.drawHorizontalLine ((int) (br.getBottom() - h), br.getX() + 2, br.getRight() - 2);
            static const char* nm[3] = { "BASSI", "MEDI", "ALTI" };
            g.setFont (mf (8.5f)); g.drawText (juce::String (nm[k]) + " " + juce::String (juce::roundToInt (w[k] * 100)) + "%", br.withHeight (14).reduced (4, 0), juce::Justification::centred);
        }
    for (float f : { 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f })
    { const float x = f2x (f); g.setColour (C::phos.withAlpha (0.06f)); g.drawVerticalLine ((int) x, r.getY(), r.getBottom()); g.setColour (C::dim.withAlpha (0.6f)); g.setFont (mf (7.5f)); g.drawText (hzS (f), juce::Rectangle<float> (x + 2, r.getBottom() - 10, 30, 10), juce::Justification::centredLeft); }
    juce::Path sp; bool st = false;
    for (int x = 0; x < (int) r.getWidth(); x += 3)
    {
        const float f = x2f (r.getX() + x); const int bin = juce::jlimit (1, 1023, (int) (f / sr * 2048));
        const float db = spec[(size_t) bin] + 3.0f * std::log2 (f / 1000.0f + 1e-3f), y = r.getBottom() - juce::jlimit (0.0f, 1.0f, (db + 90.0f) / 84.0f) * r.getHeight();
        if (! st) { sp.startNewSubPath (r.getX() + x, y); st = true; } else sp.lineTo (r.getX() + x, y);
    }
    g.setColour (C::text.withAlpha (0.55f)); g.strokePath (sp, juce::PathStrokeType (1.1f));
    if (multi) for (int k = 0; k < 2; ++k)
    {
        const float x = k == 0 ? x1 : x2; g.setColour ((dragK == k ? C::text : C::phos).withAlpha (0.9f)); g.drawLine (x, r.getY(), x, r.getBottom(), dragK == k ? 2.0f : 1.4f);
        g.fillEllipse (x - 4, r.getCentreY() - 4, 8, 8);
        g.setFont (mf (8.5f)); g.drawText (hzS (p.value (k == 0 ? "x1" : "x2")), juce::Rectangle<float> (x - 30, r.getY() + 16, 60, 12), juce::Justification::centred);
    }
    else { g.setColour (C::dim); g.setFont (uf (11.0f)); g.drawText ("UNA BANDA: IL DRIVE PRENDE TUTTO", r, juce::Justification::centred); }
    jlook::glass (g, b, 7);
}
void MbView::mouseDown (const juce::MouseEvent& e)
{
    if (ed.proc.value ("multi") < 0.5f) return;
    const float a = std::abs (e.position.x - f2x (ed.proc.value ("x1"))), b2 = std::abs (e.position.x - f2x (ed.proc.value ("x2")));
    dragK = a < b2 ? 0 : 1; ed.P (dragK == 0 ? "x1" : "x2")->beginChangeGesture(); mouseDrag (e);
}
void MbView::mouseDrag (const juce::MouseEvent& e)
{
    if (dragK < 0) return; auto* pr = ed.P (dragK == 0 ? "x1" : "x2");
    pr->setValueNotifyingHost (pr->convertTo0to1 (pr->getNormalisableRange().snapToLegalValue (x2f (e.position.x)))); repaint();
}
void MbView::mouseUp (const juce::MouseEvent&) { if (dragK >= 0) ed.P (dragK == 0 ? "x1" : "x2")->endChangeGesture(); dragK = -1; repaint(); }
void MbView::mouseMove (const juce::MouseEvent&) { setMouseCursor (ed.proc.value ("multi") > 0.5f ? juce::MouseCursor::LeftRightResizeCursor : juce::MouseCursor::NormalCursor); }

// ======================================================================= editor
Editor::Editor (Processor& p) : AudioProcessorEditor (&p), proc (p)
{
    skin::init ("alter", { 0xffe58aa0, 0xffb9a6e0, 0xffe9a260, 0xffcfcfca }, 0xffff2e88); C::sync(); themeRect = { 950, 18, 100, 28 };
    setLookAndFeel (&lnf); setOpaque (true);
    auto knob = [&] (int u, const char* id, const char* label, std::function<juce::String()> txt, bool bip = false, bool big = false) -> Knob*
    {
        auto k = std::make_unique<Knob> (P (id));
        const auto& F = face (u < 0 ? 3 : u);
        k->spec.label = label; k->spec.bipolar = bip; k->spec.big = big; k->spec.style = big && u != 0 ? 0 : F.knob; k->spec.ink = juce::Colour (F.ink); k->spec.col = acc (u < 0 ? 3 : u); k->spec.cap = acc (u < 0 ? 3 : u);
        k->text = std::move (txt);
        addAndMakeVisible (*k); knobUnit.push_back ({ k.get(), u }); knobs.push_back (std::move (k)); return knobs.back().get();
    };
    auto pct = [this] (const char* id) { return [this, id] { return juce::String (juce::roundToInt (proc.value (id) * 100)) + "%"; }; };
    auto stSnap = [] (float v, bool fine) { return fine ? v : std::round (v); };
    knob (0, "pitch", "PITCH", [this] { return semi (proc.value ("pitch")); }, true, true)->snap = stSnap;
    knob (0, "formant", "FORMANTE", [this] { return proc.value ("link") > 0.5f ? juce::String ("SEGUE") : semi (proc.value ("formant")); }, true, true)->snap = stSnap;
    knob (1, "drive", "DRIVE", pct ("drive"), false, true);
    knob (1, "tone", "TONO", [this] { const float v = proc.value ("tone"); return std::abs (v) < 0.01f ? juce::String ("NEUTRO") : v < 0 ? "SCURO " + juce::String (juce::roundToInt (-v * 100)) : "CHIARO " + juce::String (juce::roundToInt (v * 100)); }, true);
    knob (2, "wLo", "BASSI", pct ("wLo"))->spec.cap = juce::Colour (0xffff6a3d);
    knob (2, "wMid", "MEDI", pct ("wMid"))->spec.cap = juce::Colour (0xffffd23d);
    knob (2, "wHi", "ALTI", pct ("wHi"))->spec.cap = juce::Colour (0xff5ef2ff);
    knob (2, "x1", "TAGLIO 1", [this] { return hzS (proc.value ("x1")) + " Hz"; });
    knob (2, "x2", "TAGLIO 2", [this] { return hzS (proc.value ("x2")) + " Hz"; });
    knob (-1, "mix", "MIX", pct ("mix"), false, true);
    knob (-1, "out", "OUTPUT", [this] { const float v = proc.value ("out"); return (v > 0.05f ? "+" : "") + juce::String (v, 1) + " dB"; }, true);
    for (int i = 2; i <= 6; ++i) knobs[(size_t) i]->spec.style = 2;
    mode = std::make_unique<Segs> (P ("mode"), acc (0), juce::StringArray { "TRASPONI", "QUANTIZZA", "ROBOT" });
    keySeg = std::make_unique<Segs> (P ("key"), acc (0), noteNames()); keySeg->font = 9.0f;
    scaleSeg = std::make_unique<Segs> (P ("scale"), acc (0), juce::StringArray { "CROM", "MAGG", "MIN", "ARM", "PENTA", "FRIG" }); scaleSeg->font = 9.0f;
    link = std::make_unique<Switch> (P ("link"), acc (0), "LINK FORMANTI"); link->ink = juce::Colour (face (0).ink);
    multi = std::make_unique<Switch> (P ("multi"), acc (2), "3 BANDE"); multi->ink = juce::Colour (face (2).ink);
    autoG = std::make_unique<Switch> (P ("auto"), acc (3), "AUTO GAIN"); autoG->ink = juce::Colour (face (3).ink);
    for (juce::Component* c : std::initializer_list<juce::Component*> { mode.get(), keySeg.get(), scaleSeg.get(), link.get(), multi.get(), autoG.get(), &tuner, &curve, &mb }) addAndMakeVisible (c);
    addAndMakeVisible (sizeChip);
    setupBrowser();
    scaler.setup();
    startTimerHz (30);
}

void Editor::resized()
{
    const float sc = scaler.update();
    auto set = [] (juce::Component& c, float x, float y, float w, float h) { c.setBounds (juce::Rectangle<float> (x, y, w, h).toNearestInt()); };
    const auto pu = pitchUnit(), du = driveUnit(), mu = mbUnit(), oc = outCard();
    set (*mode, pu.getX() + 16, pu.getY() + 44, 250, 26); set (*link, pu.getX() + 278, pu.getY() + 46, 112, 22);
    set (tuner, pu.getX() + 16, pu.getY() + 80, 368, 84);
    set (*knobs[0], pu.getX() + 30, pu.getY() + 170, 156, 178); set (*knobs[1], pu.getX() + 214, pu.getY() + 170, 156, 178);
    set (*keySeg, pu.getX() + 24, pu.getY() + 354, 352, 20); set (*scaleSeg, pu.getX() + 24, pu.getY() + 378, 352, 20);
    set (curve, du.getX() + 16, du.getY() + 106, 368, 76);
    set (*knobs[2], du.getX() + 18, du.getY() + 190, 196, 210); set (*knobs[3], du.getX() + 240, du.getY() + 192, 130, 132);
    set (*multi, mu.getX() + 150, mu.getY() + 7, 120, 20);
    set (mb, mu.getX() + 16, mu.getY() + 44, 420, 124);
    for (int i = 0; i < 5; ++i) set (*knobs[(size_t) (4 + i)], mu.getX() + 450 + i * 70.0f, mu.getY() + 42, 68, 128);
    set (*knobs[9], oc.getX() + 62, oc.getY() + 48, 200, 222);
    set (*knobs[10], oc.getX() + 26, oc.getY() + 288, 120, 130); set (*autoG, oc.getX() + 168, oc.getY() + 330, 140, 22);
    sizeChip.setBounds (W - 76, H - 14, 58, 12);
    presetRect = { 418, 14, 344, 36 }; prevRect = { 382, 14, 34, 36 }; nextRect = { 764, 14, 34, 36 }; abRect = { 812, 14, 128, 36 };
    browser.setBounds (0, 0, W, H);
    jlook::scaleChildren (*this, sc);
}

juce::String Editor::staticKey()
{
    juce::String k; k << skin::cur() << "/" << proc.currentName() << "/" << proc.ab.cur << "/" << (int) pitchOn() << (int) driveOn() << "/" << proc.value ("model") << "/" << proc.value ("multi") << "/" << hovModel << "/" << proc.getLatencySamples();
    return k;
}
void Editor::paintStatic (juce::Graphics& g)
{
    const float Wf = (float) W, Hf = (float) H;
    jlook::backdrop (g, { 0, 0, Wf, Hf }, C::magenta, C::violet);
    jlook::header (g, { 0, 0, Wf, 64 }, "ALTER DRIVE", C::magenta, "JA-2  PITCH / FORMANTE / DRIVE MULTIBANDA", C::violet, C::magenta);
    lcd (g, presetRect.toFloat(), 8); jlook::glass (g, presetRect.toFloat(), 8);
    glowText (g, (proc.userName.isNotEmpty() ? juce::String::fromUTF8 ("\xe2\x98\x85 ") : juce::String()) + proc.currentName().replace (" | ", "  /  ").toUpperCase(), presetRect.toFloat().reduced (8, 0), C::magenta, mf (12.5f), juce::Justification::centred);
    g.setColour (C::magenta); g.setFont (uf (20.0f));
    g.drawText (juce::String::fromUTF8 ("\xe2\x80\xb9"), prevRect, juce::Justification::centred); g.drawText (juce::String::fromUTF8 ("\xe2\x80\xba"), nextRect, juce::Justification::centred);
    skin::chipDraw (g, themeRect.toFloat());
    auto unit = [&] (juce::Rectangle<float> u, int f, const char* name, bool on, int seed)
    {
        const auto& F = face (f);
        hw::faceplate (g, u, juce::Colour (F.p1), juce::Colour (F.p2), on ? acc (f) : C::faint, 34, seed, 12);
        g.setColour (juce::Colour (F.ink).withAlpha (on ? 1.0f : 0.7f)); g.setFont (uf (12.0f)); g.drawText (name, juce::Rectangle<float> (u.getX() + 14, u.getY() + 6, 200, 24), juce::Justification::centredLeft);
        hw::jewel (g, u.getRight() - 22, u.getY() + 18, 5.0f, on, acc (f));
    };
    unit (pitchUnit(), 0, "PITCH", pitchOn(), 3);
    hw::bezel (g, juce::Rectangle<float> (pitchUnit().getX() + 16, pitchUnit().getY() + 80, 368, 84).expanded (3), 9);
    unit (driveUnit(), 1, "DRIVE", driveOn(), 7);
    hw::bezel (g, juce::Rectangle<float> (driveUnit().getX() + 16, driveUnit().getY() + 106, 368, 76).expanded (3), 9);
    {   // i 13 modelli: tasti incisi
        const int cur = juce::roundToInt (proc.value ("model"));
        for (int m = 0; m < kNumModels; ++m) jlook::button (g, modelCell (m), juce::Colour (modelInfo (m).colour), m == cur, m == hovModel, m == TRASFORMATORE ? juce::String ("TRASF.") : m == DISTORSIONE ? juce::String ("DISTORS.") : juce::String (modelInfo (m).name), uf (8.5f));
        const auto& I = modelInfo (cur); const auto du = driveUnit();
        g.setColour (juce::Colour (face (1).ink).withAlpha (0.75f)); g.setFont (uf (9.5f, false));
        g.drawFittedText (juce::String ("Per: ") + I.use, juce::Rectangle<float> (du.getX() + 232, du.getY() + 332, 150, 76).toNearestInt(), juce::Justification::topLeft, 5, 0.9f);
    }
    unit (mbUnit(), 2, "MULTIBANDA", driveOn() && proc.value ("multi") > 0.5f, 11);
    hw::bezel (g, juce::Rectangle<float> (mbUnit().getX() + 16, mbUnit().getY() + 44, 420, 124).expanded (3), 9);
    {
        const auto oc = outCard(); const auto& F = face (3);
        hw::faceplate (g, oc, juce::Colour (F.p1), juce::Colour (F.p2), C::amber, 34, 23, 14);
        g.setColour (juce::Colour (F.ink)); g.setFont (uf (12.0f)); g.drawText ("USCITA", juce::Rectangle<float> (oc.getX() + 16, oc.getY() + 6, 150, 24), juce::Justification::centredLeft);
        for (int i = 0; i < 2; ++i) { auto mr = juce::Rectangle<float> (oc.getX() + 22, oc.getY() + 458 + i * 34.0f, oc.getWidth() - 44, 20); lcd (g, mr, 4); g.setColour (C::dim); g.setFont (mf (8.5f)); g.drawText (i ? "OUT" : "IN", mr.translated (0, -11).withHeight (10), juce::Justification::centredLeft); }
        g.setColour (juce::Colour (F.ink).withAlpha (0.6f)); g.setFont (uf (9.5f, false));
        const double ms = proc.getLatencySamples() * 1000.0 / juce::jmax (1.0, proc.sampleRate());
        g.drawFittedText ("PITCH e FORMANTE: trascina per semitoni, SHIFT per i centesimi.\nDRIVE: scegli il modello, poi dosalo per banda.\nLatenza " + juce::String (juce::roundToInt (ms)) + " ms, compensata dalla DAW.",
                          juce::Rectangle<int> ((int) oc.getX() + 20, (int) oc.getY() + 532, (int) oc.getWidth() - 40, 64), juce::Justification::topLeft, 5);
    }
}

void Editor::paint (juce::Graphics& g)
{
    skin::apply(); C::sync();
    g.addTransform (juce::AffineTransform::scale (scaler.s));
    bg.draw (g, { 0, 0, W, H }, staticKey(), [this] (juce::Graphics& g2) { paintStatic (g2); });
    jlook::button (g, abRect.toFloat(), C::magenta, proc.ab.cur == 1, abHover, "MEMORIA " + proc.ab.label(), uf (11.0f));
    const auto oc = outCard();
    for (int i = 0; i < 2; ++i)
    {
        auto mr = juce::Rectangle<float> (oc.getX() + 22, oc.getY() + 458 + i * 34.0f, oc.getWidth() - 44, 20).reduced (4, 4);
        const float l = i ? proc.outPk[0].load() : proc.inPk[0].load(), r = i ? proc.outPk[1].load() : proc.inPk[1].load();
        for (int c = 0; c < 2; ++c)
        {
            auto m = mr.withHeight (mr.getHeight() / 2 - 1).translated (0, c * mr.getHeight() / 2);
            const float dbv = juce::Decibels::gainToDecibels (c ? r : l, -60.0f), n = juce::jlimit (0.0f, 1.0f, (dbv + 60) / 63);
            for (int k = 0; k < 36; ++k) { const float t2 = k / 36.0f; g.setColour (t2 <= n ? (t2 > 0.93f ? C::red : t2 > 0.8f ? C::amber : C::green) : juce::Colours::white.withAlpha (0.05f)); g.fillRect (m.getX() + k * m.getWidth() / 36, m.getY(), m.getWidth() / 36 - 1.2f, m.getHeight()); }
        }
    }
    if (driveOn() && proc.value ("auto") > 0.5f)
    { g.setColour (C::dim); g.setFont (mf (9.0f)); const float a = proc.drive.autoDb.load(); g.drawText ("auto " + juce::String (a > 0 ? "+" : "") + juce::String (a, 1) + " dB", juce::Rectangle<float> (oc.getX() + 168, oc.getY() + 356, 140, 14), juce::Justification::centredLeft); }
}

void Editor::mouseMove (const juce::MouseEvent& e0)
{
    const auto e = e0.withNewPosition (e0.position / scaler.s);
    const bool h = abRect.contains (e.getPosition());
    if (h != abHover) { abHover = h; repaint (scaler.map (abRect.expanded (8))); }
    int hm = -1; for (int m = 0; m < kNumModels; ++m) if (modelCell (m).contains (e.position)) hm = m;
    if (hm != hovModel) { hovModel = hm; repaint (scaler.map (driveUnit().toNearestInt())); }
    const bool hand = h || hm >= 0 || presetRect.contains (e.getPosition()) || prevRect.contains (e.getPosition()) || nextRect.contains (e.getPosition()) || themeRect.contains (e.getPosition());
    setMouseCursor (hand ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
}
void Editor::mouseDown (const juce::MouseEvent& e0)
{
    const auto e = e0.withNewPosition (e0.position / scaler.s);
    if (themeRect.contains (e.getPosition())) { skin::cycle(); C::sync(); for (auto* ch : getChildren()) ch->repaint(); repaint(); return; }
    for (int m = 0; m < kNumModels; ++m) if (modelCell (m).contains (e.position)) { auto* pr = P ("model"); pr->setValueNotifyingHost (pr->convertTo0to1 ((float) m)); repaint(); return; }
    const int n = (int) presets().size();
    if (abRect.contains (e.getPosition())) { proc.ab.toggle (proc); repaint(); }
    else if (prevRect.contains (e.getPosition())) { proc.loadPreset ((juce::jmax (0, proc.preset) + n - 1) % n); repaint(); }
    else if (nextRect.contains (e.getPosition())) { proc.loadPreset ((juce::jmax (0, proc.preset) + 1) % n); repaint(); }
    else if (presetRect.contains (e.getPosition())) browser.open();
}

void Editor::setupBrowser()
{
    addChildComponent (browser);
    browser.accent = C::magenta;
    auto userFiles = [] { auto f = Processor::presetFolder().findChildFiles (juce::File::findFiles, false, "*.jalter"); f.sort(); return f; };
    browser.entries = [userFiles]
    {
        std::vector<jbrowse::Entry> v;
        for (int i = 0; i < (int) presets().size(); ++i)
        {
            const juce::String nm (presets()[(size_t) i].name);
            jbrowse::Entry e; e.id = i; e.note = juce::String::fromUTF8 (presets()[(size_t) i].note);
            e.group = nm.contains (" | ") ? nm.upToFirstOccurrenceOf (" | ", false, false) : juce::String ("BASE");
            e.name = nm.contains (" | ") ? nm.fromFirstOccurrenceOf (" | ", false, false) : nm;
            v.push_back (e);
        }
        const auto fs = userFiles();
        for (int i = 0; i < fs.size(); ++i) { jbrowse::Entry e; e.id = 100000 + i; e.user = true; e.group = "I MIEI"; e.name = fs[i].getFileNameWithoutExtension(); e.note = "Preset tuo, salvato in Musica / Jackpot / Alter Drive Preset."; v.push_back (e); }
        return v;
    };
    browser.current = [this, userFiles]
    {
        if (proc.userName.isEmpty()) return proc.preset;
        const auto fs = userFiles(); for (int i = 0; i < fs.size(); ++i) if (fs[i].getFileNameWithoutExtension() == proc.userName) return 100000 + i;
        return -1;
    };
    browser.load = [this] (const jbrowse::Entry& e) { if (e.user) proc.loadUserPreset (Processor::presetFolder().getChildFile (e.name + ".jalter")); else proc.loadPreset (e.id); repaint(); };
    browser.save = [this] { askPresetName(); };
    browser.canOverwrite = [this] { return proc.userName.isNotEmpty(); };
    browser.overwrite = [this] { proc.saveUserPreset (proc.userName); repaint(); };
    browser.folder = [] { Processor::presetFolder().startAsProcess(); };
    browser.remove = [this] (const jbrowse::Entry& e) { Processor::presetFolder().getChildFile (e.name + ".jalter").moveToTrash(); if (proc.userName == e.name) proc.userName = {}; repaint(); };
    browser.onClose = [this] { repaint(); };
}
void Editor::askPresetName()
{
    auto* w = new juce::AlertWindow ("Salva preset", "Nome del preset (finisce in Musica / Jackpot / Alter Drive Preset):", juce::MessageBoxIconType::NoIcon, this);
    w->addTextEditor ("name", proc.userName.isNotEmpty() ? proc.userName : proc.currentName().replace (" | ", " - "));
    w->addButton ("SALVA", 1, juce::KeyPress (juce::KeyPress::returnKey)); w->addButton ("ANNULLA", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    juce::Component::SafePointer<Editor> sp (this);
    w->enterModalState (true, juce::ModalCallbackFunction::create ([sp, w] (int r)
    {
        if (sp != nullptr && r == 1) { const auto n = w->getTextEditorContents ("name").trim(); if (n.isNotEmpty()) { sp->proc.saveUserPreset (n); sp->browser.refresh(); sp->repaint(); } }
    }), true);
}

void Editor::timerCallback()
{
    // si ridisegna solo cio' che cambia
    ++tick;
    if (browser.isVisible()) return;
    const bool pOn = pitchOn(), dOn = driveOn(), mOn = proc.value ("multi") > 0.5f, qOn = juce::roundToInt (proc.value ("mode")) != TRASPONI;
    for (auto& [k, u] : knobUnit) k->refresh (u == 0 ? true : u == 1 ? true : u == 2 ? (dOn && mOn) : true);
    mode->refresh (true); keySeg->refresh (qOn); scaleSeg->refresh (qOn && juce::roundToInt (proc.value ("mode")) == QUANTIZZA);
    link->refresh(); multi->refresh(); autoG->refresh();
    { const auto k = tuner.key(); if (k != tuner.lastKey) { tuner.lastKey = k; tuner.repaint(); } }
    { const auto k = curve.key(); if (k != curve.lastKey) { curve.lastKey = k; curve.repaint(); } }
    if (tick % 2 == 0) { mb.update(); mb.repaint(); }
    if (bg.key != staticKey()) repaint();
    else
    {
        const auto oc = outCard();
        repaint (scaler.map (juce::Rectangle<float> (oc.getX() + 20, oc.getY() + 444, oc.getWidth() - 40, 72).toNearestInt()));
        if (dOn) repaint (scaler.map (juce::Rectangle<float> (oc.getX() + 168, oc.getY() + 356, 140, 14).toNearestInt()));
    }
    juce::ignoreUnused (pOn);
}
}
