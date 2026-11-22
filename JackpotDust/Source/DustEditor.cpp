#include "DustEditor.h"
#include <complex>

namespace jds
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
    g.setColour (C::amber.withAlpha (0.035f));
    for (float x = r.getX() + 10; x < r.getRight(); x += 12) g.drawVerticalLine ((int) x, r.getY() + 2, r.getBottom() - 2);
    for (float y = r.getY() + 10; y < r.getBottom(); y += 12) g.drawHorizontalLine ((int) y, r.getX() + 2, r.getRight() - 2);
}
static juce::String hzS (float f) { return f >= 1000 ? juce::String (f / 1000.0f, f >= 10000 ? 0 : 1) + "k" : juce::String (juce::roundToInt (f)); }
static juce::String hzT (float f) { return f >= 1000 ? juce::String (f / 1000.0f, f >= 10000 ? 1 : 2) + " kHz" : juce::String (juce::roundToInt (f)) + " Hz"; }

// ======================================================================= frontali dei moduli
// ogni modulo ha la sua "macchina": colore del pannello, scritte, manopole
struct Face { juce::uint32 p1, p2, ink, acc; int knob; const char* name; const char* sub; };
static const Face& face (int m)
{
    static const Face F[kMods] = {
        { 0xffbdb6a3, 0xff8f8979, 0xff1e1b16, 0xffffb000, 2, "SAMPLER", "macchine d'epoca" },
        { 0xff4d1716, 0xff2a0c0b, 0xfff1dcc4, 0xffff7a1a, 0, "SATURAZIONE", "nastro, valvola, clip" },
        { 0xff202228, 0xff0f1013, 0xffe6e8ec, 0xff5ef2ff, 1, "FILTRI & EQ", "taglia e scolpisci" },
        { 0xff2f3b48, 0xff19212a, 0xffe9e2cf, 0xffff6f91, 0, "COMPRESSORE", "FET, bus, 670, OTT" },
        { 0xff4b402c, 0xff2c251a, 0xfff0e4c8, 0xfff3d9a4, 1, "NASTRO", "wow, flutter, eta" },
        { 0xff15363e, 0xff0b1e23, 0xffdff4f4, 0xff5ef2ff, 2, "ROOM", "stanza per batterie grandi" },
        { 0xff17171a, 0xff08080a, 0xfff3d9a4, 0xffb5c94a, 0, "POLVERE", "vinile, crackle, hiss" } };
    return F[juce::jlimit (0, kMods - 1, m)];
}
static juce::Colour modCol (int m) { return juce::Colour (face (m).acc); }
static juce::Rectangle<float> dispRect (int m) { auto u = Editor::unit (m); return { u.getX() + 16, u.getY() + 48, u.getWidth() - 32, 64 }; }

// ======================================================================= controlli
void Knob::mouseDown (const juce::MouseEvent& e)
{
    if (onTouch) onTouch();
    if (e.mods.isAltDown() || e.mods.isCommandDown()) { param->setValueNotifyingHost (param->getDefaultValue()); return; }
    start = param->getValue(); param->beginChangeGesture(); drag = true; repaint();
}
void Knob::mouseDrag (const juce::MouseEvent& e)
{
    if (! drag) return;
    param->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, start - e.getDistanceFromDragStartY() / (e.mods.isShiftDown() ? 900.0f : 220.0f)));
}
void Knob::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w)
{
    if (onTouch) onTouch();
    param->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, param->getValue() + (w.deltaY > 0 ? 0.01f : -0.01f)));
}
void Knob::paint (juce::Graphics& g)
{
    auto s = spec; s.enabled = lit; s.hover = hover; s.drag = drag; s.text = text ? text() : param->getCurrentValueAsText();
    hw::knob (g, getLocalBounds().toFloat(), param->getValue(), s);
}
void Segs::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat(); lcd (g, b, 6);
    const int n = juce::jmax (1, items.size()), cur = juce::roundToInt (param->convertFrom0to1 (param->getValue()));
    const float w = (b.getWidth() - 6) / n; const auto c = lit ? col : col.withSaturation (0.15f).withMultipliedBrightness (0.55f);
    for (int i = 0; i < n; ++i)
    {
        auto r = juce::Rectangle<float> (3 + i * w, 3, w, b.getHeight() - 6).reduced (1, 0);
        if (i == cur) { g.setColour (c.withAlpha (0.2f)); g.fillRoundedRectangle (r, 4); g.setColour (c.withAlpha (0.85f)); g.drawRoundedRectangle (r, 4, 1.0f); glowText (g, items[i], r, c, uf (font), juce::Justification::centred); }
        else { g.setColour (C::dim); g.setFont (uf (font)); g.drawText (items[i], r, juce::Justification::centred); }
    }
}
void Segs::mouseDown (const juce::MouseEvent& e)
{
    const int n = juce::jmax (1, items.size()); const int k = juce::jlimit (0, n - 1, (int) ((e.position.x - 3) / ((getWidth() - 6) / (float) n)));
    param->setValueNotifyingHost (param->convertTo0to1 ((float) k)); if (onPick) onPick (k); repaint();
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

// ======================================================================= display dei moduli
static double biqMag (const Biquad& b, double f, double sr)
{
    const std::complex<double> z = std::polar (1.0, -2 * kPi * f / sr);
    return std::abs ((b.b0 + b.b1 * z + b.b2 * z * z) / (1.0 + b.a1 * z + b.a2 * z * z));
}
juce::String ModView::key() const
{
    auto& P = ed.proc; juce::String k; k << (int) ed.isOn (mod) << skin::cur();
    switch (mod)
    {
        case M_SAMP: k << P.value ("s_bits") << "/" << P.value ("s_rate") << "/" << P.value ("s_model"); break;
        case M_SAT:  k << P.value ("d_drive") << "/" << P.value ("d_model") << "/" << P.value ("amount"); break;
        case M_FILT: for (auto* id : { "f_hp", "f_lp", "f_res", "f_24", "f_boom", "f_crack", "f_warm", "f_air" }) k << "/" << P.value (id); break;
        case M_ROOM: for (auto* id : { "r_size", "r_decay", "r_tone", "r_gate" }) k << "/" << P.value (id); break;
        default: break;
    }
    return k;
}
void ModView::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat(); lcd (g, b, 7);
    auto& P = ed.proc; const bool on = ed.isOn (mod); const auto c = on ? modCol (mod) : C::faint;
    auto r = b.reduced (9, 7);
    g.setFont (mf (8.5f));
    switch (mod)
    {
        case M_SAMP:
        {
            const float bits = P.value ("s_bits"), rate = P.value ("s_rate");
            const float lv = std::pow (2.0f, juce::jmax (2.0f, bits - 7.0f));
            juce::Path sine, st; const int N = 160; const double span = 0.0022, f = 900;
            float held = 0; double hp = 1;
            for (int i = 0; i <= N; ++i)
            {
                const double t = span * i / N; const float v = (float) std::sin (2 * kPi * f * t) * 0.85f;
                hp += span / N * rate; if (hp >= 1) { hp -= std::floor (hp); held = std::round (v * lv) / lv; }
                const float x = r.getX() + r.getWidth() * i / N;
                if (i == 0) { sine.startNewSubPath (x, r.getCentreY() - v * r.getHeight() * 0.42f); st.startNewSubPath (x, r.getCentreY() - held * r.getHeight() * 0.42f); }
                else { sine.lineTo (x, r.getCentreY() - v * r.getHeight() * 0.42f); st.lineTo (x, st.getCurrentPosition().y); st.lineTo (x, r.getCentreY() - held * r.getHeight() * 0.42f); }
            }
            g.setColour (C::dim.withAlpha (0.5f)); g.strokePath (sine, juce::PathStrokeType (1.0f));
            g.setColour (c.withAlpha (0.3f)); g.strokePath (st, juce::PathStrokeType (3.5f)); g.setColour (c); g.strokePath (st, juce::PathStrokeType (1.4f));
            glowText (g, juce::String (juce::roundToInt (bits)) + " BIT  " + hzT (rate), r.withHeight (11), c, mf (9.5f), juce::Justification::centredRight);
            break;
        }
        case M_SAT:
        {
            const int m = juce::roundToInt (P.value ("d_model")); const float gd = dbToG (P.value ("d_drive") * P.value ("amount"));
            double e = 0; for (int k = 0; k < 64; ++k) { const float y = shapeFn (m, gd * 0.5f * std::sin (2 * (float) kPi * k / 64)); e += y * y; }
            const float mk = 0.5f / (float) std::sqrt (e / 64 + 1e-12) * 0.7071f;
            g.setColour (C::dim.withAlpha (0.3f)); g.drawLine (r.getX(), r.getBottom(), r.getRight(), r.getY(), 1.0f);
            juce::Path p;
            for (int i = 0; i <= 100; ++i) { const float x = -1 + 2 * i / 100.0f, y = juce::jlimit (-1.2f, 1.2f, shapeFn (m, x * gd) * mk * 1.4f); const float px = r.getX() + r.getWidth() * i / 100.0f, py = r.getCentreY() - y * r.getHeight() * 0.42f; if (i == 0) p.startNewSubPath (px, py); else p.lineTo (px, py); }
            g.setColour (c.withAlpha (0.3f)); g.strokePath (p, juce::PathStrokeType (3.5f)); g.setColour (c); g.strokePath (p, juce::PathStrokeType (1.5f));
            glowText (g, "+" + juce::String (P.value ("d_drive"), 1) + " dB", r.withHeight (11), c, mf (9.5f), juce::Justification::centredRight);
            break;
        }
        case M_COMP:
        {
            juce::Path p; const int n = (int) std::size (trace);
            for (int i = 0; i < n; ++i) { const float v = trace[(tw + i) % n]; const float x = r.getX() + r.getWidth() * i / (n - 1), y = r.getY() + 12 + juce::jlimit (0.0f, 1.0f, v / 18.0f) * (r.getHeight() - 24); if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y); }
            juce::Path f2 = p; f2.lineTo (r.getRight(), r.getY() + 12); f2.lineTo (r.getX(), r.getY() + 12); f2.closeSubPath();
            g.setColour (c.withAlpha (0.18f)); g.fillPath (f2); g.setColour (c); g.strokePath (p, juce::PathStrokeType (1.4f));
            const float gr = P.eng.gr.load();
            glowText (g, "-" + juce::String (gr, 1) + " dB", r.withHeight (11), c, mf (9.5f), juce::Justification::centredRight);
            g.setColour (C::dim); g.drawText ("riduzione", r.withHeight (11), juce::Justification::centredLeft);
            break;
        }
        case M_FILT:
        {
            const double sr = 48000; Biquad hp, lp, bo, cr, hp2, lp2;
            const float res = P.value ("f_res"), fh = P.value ("f_hp"), fl = P.value ("f_lp"); const bool s24 = P.value ("f_24") > 0.5f;
            hp.hp (fh, 1.0 / (1.4142 - 1.33 * res * 0.6), sr); lp.lp (fl, 1.0 / (1.4142 - 1.33 * res), sr); hp2.hp (fh, 0.7071, sr); lp2.lp (fl, 0.7071, sr);
            bo.bell (70, 0.9, P.value ("f_boom"), sr); cr.bell (2600, 1.0, P.value ("f_crack"), sr);
            Biquad w1, w2, w3, ai; const float wm = P.value ("f_warm"), am = P.value ("f_air");
            w1.lowShelf (85, wm * 4.5, sr); w2.bell (320, 0.8, -wm * 2.5, sr); w3.highShelf (9000, -wm * 3.0, sr); ai.highShelf (11000, am * 7.0, sr);
            juce::Path p;
            for (int i = 0; i <= 100; ++i)
            {
                const double f = 20 * std::pow (1000.0, i / 100.0);
                double m = biqMag (bo, f, sr) * biqMag (cr, f, sr) * biqMag (w1, f, sr) * biqMag (w2, f, sr) * biqMag (w3, f, sr) * biqMag (ai, f, sr);
                if (fh > 21) m *= biqMag (hp, f, sr) * (s24 ? biqMag (hp2, f, sr) : 1.0);
                if (fl < 19900) m *= biqMag (lp, f, sr) * (s24 ? biqMag (lp2, f, sr) : 1.0);
                const float db = (float) juce::jlimit (-30.0, 18.0, 20 * std::log10 (m + 1e-9));
                const float x = r.getX() + r.getWidth() * i / 100.0f, y = r.getY() + r.getHeight() * (18 - db) / 48;
                if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
            }
            g.setColour (C::dim.withAlpha (0.3f)); g.drawHorizontalLine ((int) (r.getY() + r.getHeight() * 18 / 48), r.getX(), r.getRight());
            for (float hz : { 100.0f, 1000.0f, 10000.0f }) { const float x = r.getX() + r.getWidth() * std::log (hz / 20) / std::log (1000.0f); g.drawVerticalLine ((int) x, r.getY(), r.getBottom()); }
            g.setColour (c.withAlpha (0.3f)); g.strokePath (p, juce::PathStrokeType (3.5f)); g.setColour (c); g.strokePath (p, juce::PathStrokeType (1.5f));
            break;
        }
        case M_TAPE:
        {
            const float rad = r.getHeight() * 0.46f; const juce::Point<float> a (r.getX() + rad + 14, r.getCentreY()), bpt (r.getRight() - rad - 14, r.getCentreY());
            for (auto ctr : { a, bpt })
            {
                g.setColour (c.withAlpha (0.15f)); g.fillEllipse (juce::Rectangle<float> (rad * 2, rad * 2).withCentre (ctr));
                g.setColour (c.withAlpha (0.7f)); g.drawEllipse (juce::Rectangle<float> (rad * 2, rad * 2).withCentre (ctr), 1.4f);
                g.setColour (c.withAlpha (0.5f)); g.drawEllipse (juce::Rectangle<float> (rad * 0.5f, rad * 0.5f).withCentre (ctr), 1.1f);
                for (int k = 0; k < 3; ++k) { const float an = spin + k * 2.094f; g.drawLine (ctr.x + std::cos (an) * rad * 0.3f, ctr.y + std::sin (an) * rad * 0.3f, ctr.x + std::cos (an) * rad * 0.85f, ctr.y + std::sin (an) * rad * 0.85f, 1.8f); }
            }
            juce::Path tape; tape.startNewSubPath (a.x, a.y + rad);
            const float wob = std::sin (spin * 0.37f) * 4.0f * P.value ("t_wow");
            tape.quadraticTo ((a.x + bpt.x) / 2, a.y + rad + 6 + wob, bpt.x, bpt.y + rad);
            g.setColour (c); g.strokePath (tape, juce::PathStrokeType (1.4f));
            break;
        }
        case M_ROOM:
        {   // pianta della stanza vista dall'alto con le riflessioni, e la coda nel tempo
            const float size = 0.35f + 1.15f * P.value ("r_size"), dec = P.value ("r_decay"); const bool gate = P.value ("r_gate") > 0.5f;
            auto room = juce::Rectangle<float> (r.getX(), r.getY(), r.getHeight() * 1.25f, r.getHeight()).withSizeKeepingCentre (r.getHeight() * 0.7f * size + 20, r.getHeight() * 0.55f * size + 14).withX (r.getX() + 4);
            g.setColour (c.withAlpha (0.6f)); g.drawRect (room, 1.2f);
            const auto src = room.getCentre().translated (-room.getWidth() * 0.2f, 0);
            for (int k = 0; k < 6; ++k) { const float an = k * 1.05f + 0.3f; g.setColour (c.withAlpha (0.35f)); g.drawLine (src.x, src.y, src.x + std::cos (an) * room.getWidth() * 0.6f, src.y + std::sin (an) * room.getHeight() * 0.6f, 0.8f); }
            g.setColour (c); g.fillEllipse (src.x - 3, src.y - 3, 6, 6);
            auto tl = r.withTrimmedLeft (r.getHeight() * 1.25f + 10);
            juce::Path p; const float rt = 0.15f + 2.35f * dec * dec;
            for (int i = 0; i <= 60; ++i)
            {
                const float t = i / 60.0f * 1.5f; float a = std::exp (-6.9f * t / rt);
                if (gate && t > 0.06f + 0.16f * P.value ("r_size")) a *= std::exp (-(t - 0.06f - 0.16f * P.value ("r_size")) * 60.0f);
                const float x = tl.getX() + tl.getWidth() * i / 60.0f, y = tl.getBottom() - a * tl.getHeight() * 0.9f;
                if (i == 0) p.startNewSubPath (x, tl.getBottom()); p.lineTo (x, y);
            }
            g.setColour (c.withAlpha (0.3f)); g.strokePath (p, juce::PathStrokeType (3.0f)); g.setColour (c); g.strokePath (p, juce::PathStrokeType (1.3f));
            glowText (g, (gate ? juce::String ("GATE  ") : juce::String()) + juce::String (rt, 2) + " s", tl.withHeight (11), c, mf (9.0f), juce::Justification::centredRight);
            break;
        }
        case M_DUST:
        default:
        {
            const float rad = r.getHeight() * 0.5f; const auto ctr = juce::Point<float> (r.getCentreX(), r.getCentreY());
            g.setColour (juce::Colour (0xff050505)); g.fillEllipse (juce::Rectangle<float> (rad * 2, rad * 2).withCentre (ctr));
            for (int k = 1; k < 8; ++k) { g.setColour (juce::Colours::white.withAlpha (0.04f + 0.01f * (k % 2))); g.drawEllipse (juce::Rectangle<float> (rad * 2 * k / 8, rad * 2 * k / 8).withCentre (ctr), 0.8f); }
            g.setColour (c.withAlpha (0.8f)); g.fillEllipse (juce::Rectangle<float> (rad * 0.5f, rad * 0.5f).withCentre (ctr));
            { const float an = spin; g.setColour (juce::Colours::black); g.drawLine (ctr.x, ctr.y, ctr.x + std::cos (an) * rad * 0.25f, ctr.y + std::sin (an) * rad * 0.25f, 1.5f); }
            juce::Random rr ((juce::int64) (spin * 10));
            const int dots = on ? (int) (P.value ("n_vinyl") * 30 + P.value ("n_pops") * 6) : 0;
            for (int k = 0; k < dots; ++k) { const float an = rr.nextFloat() * 6.283f, rd = rad * (0.35f + 0.6f * rr.nextFloat()), s = 1 + rr.nextFloat() * (k < P.value ("n_pops") * 6 ? 4.0f : 1.5f); g.setColour (C::cream.withAlpha (0.4f + 0.6f * rr.nextFloat())); g.fillEllipse (ctr.x + std::cos (an) * rd - s / 2, ctr.y + std::sin (an) * rd - s / 2, s, s); }
            break;
        }
    }
    jlook::glass (g, b, 7);
}
void Scope::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat(); lcd (g, b, 7);
    float d[2048]; ed.proc.ringOut.read (d, 2048);
    juce::Path p; auto r = b.reduced (6, 6);
    for (int i = 0; i < 2048; i += 4) { const float x = r.getX() + r.getWidth() * i / 2048.0f, y = r.getCentreY() - juce::jlimit (-1.0f, 1.0f, d[i]) * r.getHeight() * 0.48f; if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y); }
    g.setColour (C::amber.withAlpha (0.25f)); g.strokePath (p, juce::PathStrokeType (3.0f)); g.setColour (C::amber); g.strokePath (p, juce::PathStrokeType (1.2f));
    jlook::glass (g, b, 7);
}

// ======================================================================= editor
Editor::Editor (Processor& p) : AudioProcessorEditor (&p), proc (p)
{
    skin::init ("dust", { 0xffe8cf86, 0xffd9814f, 0xffb5bf86, 0xffd8cdb8 }, 0xffffa020); C::sync(); themeRect = { 1072, 19, 100, 28 };
    setLookAndFeel (&lnf);
    auto turnOn = [this] (int m) { return [this, m] { if (! isOn (m)) P (onId (m))->setValueNotifyingHost (1.0f); }; };
    auto knob = [&] (int m, const char* id, const char* label, std::function<juce::String()> txt, bool bip = false) -> Knob*
    {
        auto k = std::make_unique<Knob> (P (id));
        k->spec.label = label; k->spec.bipolar = bip;
        if (m >= 0) { const auto& F = face (m); k->spec.style = F.knob; k->spec.ink = juce::Colour (F.ink); k->spec.col = modCol (m); k->onTouch = turnOn (m); }
        else { k->spec.style = 1; k->spec.ink = juce::Colour (0xffe9e2cf); k->spec.col = C::amber; }
        k->text = std::move (txt);
        addAndMakeVisible (*k); knobMod.push_back ({ k.get(), m }); knobs.push_back (std::move (k)); return knobs.back().get();
    };
    auto pct = [this] (const char* id) { return [this, id] { return juce::String (juce::roundToInt (proc.value (id) * 100)) + "%"; }; };
    auto db = [this] (const char* id) { return [this, id] { const float v = proc.value (id); return (v > 0.05f ? "+" : "") + juce::String (v, 1) + " dB"; }; };
    // sampler
    knob (M_SAMP, "s_bits", "BIT", [this] { return juce::String (juce::roundToInt (proc.value ("s_bits"))) + " bit"; })->spec.cap = juce::Colour (0xffd23b3b);
    knob (M_SAMP, "s_rate", "FREQUENZA", [this] { return hzT (proc.value ("s_rate")); })->spec.cap = juce::Colour (0xff3f7fd6);
    knob (M_SAMP, "s_mix", "MIX", pct ("s_mix"))->spec.cap = juce::Colour (0xffe8e8e8);
    // saturazione
    knob (M_SAT, "d_drive", "DRIVE", [this] { return "+" + juce::String (proc.value ("d_drive"), 1) + " dB"; });
    knob (M_SAT, "d_tone", "COLORE", [this] { const float v = proc.value ("d_tone"); return std::abs (v) < 0.05f ? juce::String ("NEUTRO") : v < 0 ? "CORPO " + juce::String (juce::roundToInt (-v * 100)) : "MORSO " + juce::String (juce::roundToInt (v * 100)); }, true);
    knob (M_SAT, "d_mix", "MIX", pct ("d_mix"));
    // filtri
    knob (M_FILT, "f_hp", "HP", [this] { return proc.value ("f_hp") <= 21 ? juce::String ("OFF") : hzS (proc.value ("f_hp")); });
    knob (M_FILT, "f_lp", "LP", [this] { return proc.value ("f_lp") >= 19900 ? juce::String ("OFF") : hzS (proc.value ("f_lp")); });
    knob (M_FILT, "f_res", "RES", pct ("f_res"));
    knob (M_FILT, "f_boom", "BOOM", db ("f_boom"), true);
    knob (M_FILT, "f_crack", "CRACK", db ("f_crack"), true);
    knob (M_FILT, "f_warm", "CALDO", pct ("f_warm"));
    knob (M_FILT, "f_air", "ARIA", pct ("f_air"));
    // compressore
    knob (M_COMP, "c_amt", "QUANTITA", pct ("c_amt"));
    knob (M_COMP, "c_att", "ATTACCO", [this] { return juce::String (proc.value ("c_att"), proc.value ("c_att") < 10 ? 1 : 0) + " ms"; });
    knob (M_COMP, "c_rel", "RILASCIO", [this] { return juce::String (juce::roundToInt (proc.value ("c_rel"))) + " ms"; });
    knob (M_COMP, "c_punch", "PUNCH", [this] { const float v = proc.value ("c_punch"); return (v > 0.01f ? "+" : "") + juce::String (juce::roundToInt (v * 100)); }, true);
    knob (M_COMP, "c_mix", "MIX", pct ("c_mix"));
    // nastro
    knob (M_TAPE, "t_age", "ETA", pct ("t_age"));
    knob (M_TAPE, "t_wow", "WOW", pct ("t_wow"));
    knob (M_TAPE, "t_flut", "FLUTTER", pct ("t_flut"));
    knob (M_TAPE, "t_drop", "DROPOUT", pct ("t_drop"));
    // room
    knob (M_ROOM, "r_size", "GRANDEZZA", pct ("r_size"))->spec.cap = juce::Colour (0xff3f7fd6);
    knob (M_ROOM, "r_decay", "CODA", [this] { const float d = proc.value ("r_decay"); return juce::String (0.15f + 2.35f * d * d, 2) + " s"; })->spec.cap = juce::Colour (0xff3f7fd6);
    knob (M_ROOM, "r_tone", "TONO", [this] { const float t = proc.value ("r_tone"); return t < 0.35f ? juce::String ("SCURA") : t > 0.65f ? juce::String ("CHIARA") : juce::String ("NEUTRA"); })->spec.cap = juce::Colour (0xffe0a020);
    knob (M_ROOM, "r_mix", "MIX", pct ("r_mix"))->spec.cap = juce::Colour (0xffe8e8e8);
    // polvere
    knob (M_DUST, "n_vinyl", "VINILE", pct ("n_vinyl"));
    knob (M_DUST, "n_pops", "POP", pct ("n_pops"));
    knob (M_DUST, "n_hiss", "HISS", pct ("n_hiss"));
    knob (M_DUST, "n_hum", "RONZIO", pct ("n_hum"));
    // uscita
    kAmount = knob (-1, "amount", "CARATTERE", pct ("amount")); kAmount->spec.big = true; kAmount->spec.style = 0;
    knob (-1, "in", "INPUT", db ("in"), true);
    knob (-1, "mix", "MIX", pct ("mix"));
    knob (-1, "out", "OUTPUT", db ("out"), true);
    // selettori
    auto seg = [&] (int m, const char* id, juce::StringArray it, float font) -> Segs*
    { auto s = std::make_unique<Segs> (P (id), modCol (m), it); s->font = font; addAndMakeVisible (*s); segMod.push_back ({ s.get(), m }); segs.push_back (std::move (s)); return segs.back().get(); };
    juce::StringArray sm; for (int i = 0; i < kSModels; ++i) sm.add (sInfo (i).name);
    seg (M_SAMP, "s_model", sm, 8.5f)->onPick = [this] (int m)
    {   // la macchina porta i suoi bit e la sua frequenza
        auto* b = P ("s_bits"); b->setValueNotifyingHost (b->convertTo0to1 (sInfo (m).bits));
        auto* r = P ("s_rate"); r->setValueNotifyingHost (r->convertTo0to1 (sInfo (m).rate));
        if (! isOn (M_SAMP)) P ("s_on")->setValueNotifyingHost (1.0f);
    };
    juce::StringArray dm; for (int i = 0; i < kDModels; ++i) dm.add (dName (i));
    seg (M_SAT, "d_model", dm, 8.0f)->onPick = [this] (int) { if (! isOn (M_SAT)) P ("d_on")->setValueNotifyingHost (1.0f); };
    juce::StringArray cm; for (int i = 0; i < kCStyles; ++i) cm.add (cName (i));
    seg (M_COMP, "c_style", cm, 9.0f)->onPick = [this] (int) { if (! isOn (M_COMP)) P ("c_on")->setValueNotifyingHost (1.0f); };
    auto sw = [&] (const char* id, juce::Colour c, juce::Colour ink, const char* l) { switches.push_back (std::make_unique<Switch> (P (id), c, l)); switches.back()->ink = ink; addAndMakeVisible (*switches.back()); return switches.back().get(); };
    sw ("f_24", modCol (M_FILT), juce::Colour (face (M_FILT).ink), "24 dB/OTT");
    sw ("r_gate", modCol (M_ROOM), juce::Colour (face (M_ROOM).ink), "GATE ANNI 80");
    sw ("n_follow", modCol (M_DUST), juce::Colour (face (M_DUST).ink), "SOLO MENTRE SUONA");
    sw ("mono", C::amber, juce::Colour (0xffe9e2cf), "MONO");
    sw ("auto", C::amber, juce::Colour (0xffe9e2cf), "VOLUME AUTOMATICO");
    sw ("cal", C::amber, juce::Colour (0xffe9e2cf), "LIVELLO AUTO");
    for (int m = 0; m < kMods; ++m) { views.push_back (std::make_unique<ModView> (*this, m)); addAndMakeVisible (*views.back()); }
    addAndMakeVisible (scope);
    addAndMakeVisible (sizeChip);
    setupBrowser();
    scaler.setup();
    startTimerHz (30);
}

void Editor::resized()
{
    const float sc = scaler.update();
    presetRect = { 450, 15, 330, 36 }; prevRect = { 412, 15, 36, 36 }; nextRect = { 782, 15, 36, 36 }; abRect = { 830, 15, 110, 36 };
    for (int m = 0; m < kMods; ++m)
    {
        const auto u = unit (m);
        views[(size_t) m]->setBounds (dispRect (m).toNearestInt());
        std::vector<Knob*> ks; for (auto& [k, mm] : knobMod) if (mm == m) ks.push_back (k);
        bool hasSeg = false; for (auto& [s, mm] : segMod) if (mm == m) { s->setBounds (juce::Rectangle<float> (u.getX() + 12, u.getY() + 126, u.getWidth() - 24, 24).toNearestInt()); hasSeg = true; }
        const int n = (int) ks.size();
        // righe di manopole: 3 in fila (sampler, saturazione), 3+2 (compressore), 4+3 (filtri), 2+2 (nastro, room, polvere)
        const int perRow = n <= 3 ? 3 : n == 5 ? 3 : n == 7 ? 4 : 2;
        const float top = u.getY() + (hasSeg ? 156.0f : 150.0f), cw = (u.getWidth() - 16) / perRow;
        const float kh = n <= 3 ? 104.0f : n == 7 ? 74.0f : 77.0f;
        for (int i = 0; i < n; ++i)
        {
            const int row = i / perRow, col = i % perRow, inRow = juce::jmin (perRow, n - row * perRow);
            const float off = (perRow - inRow) * cw * 0.5f;
            ks[(size_t) i]->setBounds (juce::Rectangle<float> (u.getX() + 8 + off + col * cw, top + row * (kh + 2), cw, kh).toNearestInt());
        }
    }
    switches[0]->setBounds (juce::Rectangle<float> (unit (M_FILT).getX() + 16, unit (M_FILT).getY() + 126, 150, 20).toNearestInt());
    switches[1]->setBounds (juce::Rectangle<float> (unit (M_ROOM).getX() + 16, unit (M_ROOM).getY() + 126, 170, 20).toNearestInt());
    switches[2]->setBounds (juce::Rectangle<float> (unit (M_DUST).getX() + 16, unit (M_DUST).getY() + 126, 180, 20).toNearestInt());
    // uscita
    const auto oc = outCard();
    kAmount->setBounds (juce::Rectangle<float> (oc.getX() + 46, oc.getY() + 82, 180, 180 + 33).toNearestInt());
    std::vector<Knob*> outs; for (auto& [k, mm] : knobMod) if (mm < 0 && k != kAmount) outs.push_back (k);
    for (size_t i = 0; i < outs.size(); ++i) outs[i]->setBounds (juce::Rectangle<float> (oc.getX() + 10 + i * 84.0f, oc.getY() + 330, 84, 96).toNearestInt());
    switches[3]->setBounds (juce::Rectangle<float> (oc.getX() + 20, oc.getY() + 437, 100, 20).toNearestInt());
    switches[4]->setBounds (juce::Rectangle<float> (oc.getX() + 120, oc.getY() + 437, 150, 20).toNearestInt());
    switches[5]->setBounds (juce::Rectangle<float> (oc.getX() + 20, oc.getY() + 461, 130, 20).toNearestInt());
    scope.setBounds (juce::Rectangle<float> (oc.getX() + 22, oc.getY() + 566, oc.getWidth() - 44, 96).toNearestInt());
    sizeChip.setBounds (W - 76, H - 16, 58, 14);
    browser.setBounds (0, 0, W, H);
    jlook::scaleChildren (*this, sc);
}

juce::String Editor::staticKey()
{
    juce::String k; k << skin::cur() << "/" << proc.currentName() << "/" << proc.genre << "/" << proc.target << "/" << proc.ab.cur;
    for (int m = 0; m < kMods; ++m) k << (int) isOn (m);
    k << "/" << juce::roundToInt (proc.value ("s_model")) << juce::roundToInt (proc.value ("d_model")) << juce::roundToInt (proc.value ("c_style"));
    return k;
}

void Editor::paintStatic (juce::Graphics& g)
{
    const float Wf = (float) W, Hf = (float) H;
    jlook::backdrop (g, { 0, 0, Wf, Hf }, C::amber, C::rust);
    jlook::header (g, { 0, 0, Wf, 64 }, "DUST", C::amber, "JD-12  DRUM CHARACTER / BOOM BAP / LO-FI / TRAP", C::rust, C::amber);
    lcd (g, presetRect.toFloat(), 8); jlook::glass (g, presetRect.toFloat(), 8);
    glowText (g, (proc.userName.isNotEmpty() ? juce::String::fromUTF8 ("\xe2\x98\x85 ") + proc.userName : proc.currentName()).toUpperCase(), presetRect.toFloat(), C::amber, mf (12.5f), juce::Justification::centred);
    g.setColour (C::amber); g.setFont (uf (20.0f));
    g.drawText (juce::String::fromUTF8 ("\xe2\x80\xb9"), prevRect, juce::Justification::centred); g.drawText (juce::String::fromUTF8 ("\xe2\x80\xba"), nextRect, juce::Justification::centred);
    skin::chipDraw (g, themeRect.toFloat());
    // barra dei generi: una ricetta completa per genere e per parte del kit
    {
        auto gc = genreCard();
        hw::faceplate (g, gc, juce::Colour (0xff24262c), juce::Colour (0xff121317), C::amber, 0, 3, 12);
        g.setColour (C::text); g.setFont (uf (12.0f)); g.drawText ("GENERE", juce::Rectangle<float> (gc.getX() + 18, 84, 100, 28), juce::Justification::centredLeft);
        g.drawText ("PARTE DEL KIT", juce::Rectangle<float> (gc.getX() + 18, 124, 110, 28), juce::Justification::centredLeft);
        for (int i = 0; i < kGenresN; ++i) jlook::button (g, genreChip (i), C::amber, proc.genre == i, false, genreName (i), uf (10.0f));
        for (int i = 0; i < kTargetsN; ++i) jlook::button (g, targetChip (i), C::rust, proc.genre >= 0 && proc.target == i, false, targetName (i), uf (10.0f));
        g.setColour (C::dim); g.setFont (uf (9.5f, false));
        g.drawFittedText ("Clic: carica la catena completa per quel suono.\nCARATTERE la dosa tutta insieme.", juce::Rectangle<int> ((int) gc.getRight() - 300, 122, 284, 32), juce::Justification::centredRight, 2);
    }
    // moduli
    for (int m = 0; m < kMods; ++m)
    {
        const auto u = unit (m); const auto& F = face (m); const bool on = isOn (m); const auto acc = modCol (m), ink = juce::Colour (F.ink);
        hw::faceplate (g, u, juce::Colour (F.p1), juce::Colour (F.p2), on ? acc : C::faint, 30, 11 + m, 12);
        g.setColour (C::text.withAlpha (on ? 1.0f : 0.6f)); g.setFont (uf (12.0f)); g.drawText (juce::String (m + 1) + "  " + F.name, juce::Rectangle<float> (u.getX() + 14, u.getY() + 7, 150, 24), juce::Justification::centredLeft);
        hw::jewel (g, u.getRight() - 22, u.getY() + 19, 5.0f, on, acc);
        hw::bezel (g, dispRect (m), 7);
        // nota sotto le manopole dove c'e' spazio
        juce::String d;
        if (m == M_SAMP) d = sInfo (juce::roundToInt (proc.value ("s_model"))).desc;
        else if (m == M_SAT) d = dDesc (juce::roundToInt (proc.value ("d_model")));
        if (d.isNotEmpty()) { g.setColour (ink.withAlpha (0.75f)); g.setFont (uf (9.0f, false)); g.drawFittedText (d, juce::Rectangle<float> (u.getX() + 16, u.getY() + 262, u.getWidth() - 32, 36).toNearestInt(), juce::Justification::topLeft, 3, 0.9f); }
        if (! on) { g.setColour (juce::Colours::black.withAlpha (0.32f)); g.fillRoundedRectangle (u.reduced (5).withTrimmedTop (30), 8); }
    }
    // l'ottavo posto: cosa sta facendo la ricetta
    {
        const auto u = unit (7);
        hw::faceplate (g, u, juce::Colour (0xff1c1e24), juce::Colour (0xff0d0e11), C::amber, 30, 19, 12);
        g.setColour (C::text); g.setFont (uf (12.0f)); g.drawText ("8  GUIDA", juce::Rectangle<float> (u.getX() + 14, u.getY() + 7, 150, 24), juce::Justification::centredLeft);
        auto sr = u.reduced (16).withTrimmedTop (30); lcd (g, sr, 7);
        juce::AttributedString as; as.setWordWrap (juce::AttributedString::byWord); as.setLineSpacing (1.5f);
        if (proc.genre >= 0)
        {
            as.append (juce::String (genreName (proc.genre)) + "  /  " + targetName (proc.target) + "\n", uf (11.0f), C::amber);
            as.append (juce::String (presets()[(size_t) juce::jlimit (0, (int) presets().size() - 1, proc.preset)].note) + "\n\n", uf (10.0f, false), C::text.withAlpha (0.9f));
            as.append ("CARATTERE sotto il 100% per un tocco, sopra per spingere. Poi ritocca il modulo che senti di piu'.", uf (9.5f, false), C::dim);
        }
        else
        {
            as.append ("COME SI USA\n", uf (11.0f), C::amber);
            as.append ("1. Scegli il GENERE e la PARTE (bus, cassa, rullante, hat o il break intero).\n2. Gira CARATTERE finche' suona.\n3. Ritocca i moduli: il nome sul binario lo accende o spegne.\n\n", uf (10.0f, false), C::text.withAlpha (0.9f));
            as.append ("Tutto parte spento: senza ricetta il plugin non cambia il suono.", uf (9.5f, false), C::dim);
        }
        as.draw (g, sr.reduced (10, 8));
    }
    // uscita
    {
        const auto oc = outCard();
        hw::faceplate (g, oc, juce::Colour (0xff2a2a2e), juce::Colour (0xff121214), C::amber, 30, 23, 14);
        g.setColour (C::text); g.setFont (uf (12.0f)); g.drawText ("USCITA", juce::Rectangle<float> (oc.getX() + 16, oc.getY() + 7, 150, 24), juce::Justification::centredLeft);
        g.setColour (juce::Colour (0xffe9e2cf).withAlpha (0.7f)); g.setFont (uf (9.5f, false));
        g.drawFittedText ("CARATTERE alza o abbassa tutto insieme: macchina, drive, compressione, nastro e polvere.", juce::Rectangle<int> ((int) oc.getX() + 18, (int) oc.getY() + 40, (int) oc.getWidth() - 36, 34), juce::Justification::centredLeft, 2);
        hw::bezel (g, juce::Rectangle<float> (oc.getX() + 22, oc.getY() + 566, oc.getWidth() - 44, 96), 7);
        for (int i = 0; i < 2; ++i) { auto mr = juce::Rectangle<float> (oc.getX() + 22, oc.getY() + 502 + i * 30.0f, oc.getWidth() - 44, 20); lcd (g, mr, 4); g.setColour (C::dim); g.setFont (mf (8.5f)); g.drawText (i ? "OUT" : "IN", mr.translated (0, -11).withHeight (10), juce::Justification::centredLeft); }
        g.setColour (C::dim); g.setFont (uf (9.0f, false));
        g.drawFittedText ("Ordine: sampler, filtri, saturazione, compressore, nastro, room, polvere. LIVELLO AUTO: spinge uguale a ogni volume d'ingresso. Latenza " + juce::String (proc.getLatencySamples()) + " campioni, compensata.",
                          juce::Rectangle<int> ((int) oc.getX() + 18, (int) oc.getY() + 672, (int) oc.getWidth() - 36, 48), juce::Justification::topLeft, 4);
    }
}

void Editor::paint (juce::Graphics& g)
{
    skin::apply(); C::sync();
    g.addTransform (juce::AffineTransform::scale (scaler.s));
    bg.draw (g, { 0, 0, W, H }, staticKey(), [this] (juce::Graphics& g2) { paintStatic (g2); });
    jlook::button (g, abRect.toFloat(), C::amber, proc.ab.cur == 1, abHover, "MEMORIA " + proc.ab.label(), uf (11.5f));
    const auto oc = outCard();
    for (int i = 0; i < 2; ++i)
    {
        auto mr = juce::Rectangle<float> (oc.getX() + 22, oc.getY() + 502 + i * 30.0f, oc.getWidth() - 44, 20).reduced (4, 4);
        const float l = i ? proc.outPk[0].load() : proc.inPk[0].load(), r = i ? proc.outPk[1].load() : proc.inPk[1].load();
        for (int c = 0; c < 2; ++c)
        {
            auto m = mr.withHeight (mr.getHeight() / 2 - 1).translated (0, c * mr.getHeight() / 2);
            const float dbv = gToDb (c ? r : l), n = juce::jlimit (0.0f, 1.0f, (dbv + 60) / 63);
            for (int k = 0; k < 36; ++k) { const float t2 = k / 36.0f; g.setColour (t2 <= n ? (t2 > 0.93f ? C::red : t2 > 0.8f ? C::amber : C::olive) : juce::Colours::white.withAlpha (0.05f)); g.fillRect (m.getX() + k * m.getWidth() / 36, m.getY(), m.getWidth() / 36 - 1.2f, m.getHeight()); }
        }
    }
}

void Editor::mouseMove (const juce::MouseEvent& e0)
{
    const auto e = e0.withNewPosition (e0.position / scaler.s);
    const bool h = abRect.contains (e.getPosition());
    if (h != abHover) { abHover = h; repaint (scaler.map (abRect.expanded (8))); }
    bool hand = h || presetRect.contains (e.getPosition()) || prevRect.contains (e.getPosition()) || nextRect.contains (e.getPosition()) || themeRect.contains (e.getPosition());
    for (int m = 0; m < kMods; ++m) if (unit (m).withHeight (34).contains (e.position)) hand = true;
    for (int i = 0; i < kGenresN; ++i) if (genreChip (i).contains (e.position)) hand = true;
    for (int i = 0; i < kTargetsN; ++i) if (targetChip (i).contains (e.position)) hand = true;
    setMouseCursor (hand ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
}
void Editor::mouseDown (const juce::MouseEvent& e0)
{
    const auto e = e0.withNewPosition (e0.position / scaler.s);
    if (themeRect.contains (e.getPosition())) { skin::cycle(); C::sync(); for (auto* ch : getChildren()) ch->repaint(); repaint(); return; }
    for (int m = 0; m < kMods; ++m) if (unit (m).withHeight (34).contains (e.position)) { auto* p = P (onId (m)); p->setValueNotifyingHost (p->getValue() > 0.5f ? 0.0f : 1.0f); repaint(); return; }
    for (int i = 0; i < kGenresN; ++i) if (genreChip (i).contains (e.position)) { proc.loadPreset (recipeIndex (i, juce::jmax (0, proc.target))); repaint(); return; }
    for (int i = 0; i < kTargetsN; ++i) if (targetChip (i).contains (e.position)) { proc.loadPreset (recipeIndex (proc.genre >= 0 ? proc.genre : 0, i)); repaint(); return; }
    const int n = (int) presets().size();
    if (abRect.contains (e.getPosition())) { proc.ab.toggle (proc); repaint(); }
    else if (prevRect.contains (e.getPosition())) { proc.loadPreset ((juce::jmax (0, proc.preset) + n - 1) % n); repaint(); }
    else if (nextRect.contains (e.getPosition())) { proc.loadPreset ((juce::jmax (0, proc.preset) + 1) % n); repaint(); }
    else if (presetRect.contains (e.getPosition())) browser.open();
}

void Editor::setupBrowser()
{
    addChildComponent (browser);
    browser.accent = C::amber;
    auto userFiles = [] { auto f = Processor::presetFolder().findChildFiles (juce::File::findFiles, false, "*.jdust"); f.sort(); return f; };
    browser.entries = [userFiles]
    {
        std::vector<jbrowse::Entry> v;
        for (int i = 0; i < (int) presets().size(); ++i)
        {
            const juce::String n (presets()[(size_t) i].name);
            jbrowse::Entry e; e.id = i; e.note = presets()[(size_t) i].note;
            e.group = n.contains (" | ") ? n.upToFirstOccurrenceOf (" | ", false, false) : juce::String ("BASE");
            e.name = n.contains (" | ") ? n.fromFirstOccurrenceOf (" | ", false, false) : n;
            v.push_back (e);
        }
        const auto fs = userFiles();
        for (int i = 0; i < fs.size(); ++i) { jbrowse::Entry e; e.id = 100000 + i; e.user = true; e.group = "I MIEI"; e.name = fs[i].getFileNameWithoutExtension(); e.note = "Preset tuo, salvato in Musica / Jackpot / Dust Preset."; v.push_back (e); }
        return v;
    };
    browser.current = [this, userFiles]
    {
        if (proc.userName.isEmpty()) return proc.preset;
        const auto fs = userFiles(); for (int i = 0; i < fs.size(); ++i) if (fs[i].getFileNameWithoutExtension() == proc.userName) return 100000 + i;
        return -1;
    };
    browser.load = [this] (const jbrowse::Entry& e)
    {
        if (e.user) proc.loadUserPreset (Processor::presetFolder().getChildFile (e.name + ".jdust")); else proc.loadPreset (e.id);
        repaint();
    };
    browser.save = [this] { askPresetName(); };
    browser.canOverwrite = [this] { return proc.userName.isNotEmpty(); };
    browser.overwrite = [this] { proc.saveUserPreset (proc.userName); repaint(); };
    browser.folder = [] { Processor::presetFolder().startAsProcess(); };
    browser.remove = [this] (const jbrowse::Entry& e) { Processor::presetFolder().getChildFile (e.name + ".jdust").moveToTrash(); if (proc.userName == e.name) proc.userName = {}; repaint(); };
    browser.onClose = [this] { repaint(); };
}
void Editor::askPresetName()
{
    auto* w = new juce::AlertWindow ("Salva preset", "Nome del preset (finisce in Musica / Jackpot / Dust Preset):", juce::MessageBoxIconType::NoIcon, this);
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
    // si ridisegna solo cio' che cambia: manopole mosse, display animati dei moduli accesi, misuratori
    for (auto& [k, m] : knobMod) k->refresh (m < 0 || isOn (m));
    for (auto& [s, m] : segMod) s->refresh (isOn (m));
    for (auto& s : switches) s->refresh();
    auto& cv = *views[M_COMP]; cv.trace[cv.tw] = proc.eng.gr.load(); cv.tw = (cv.tw + 1) % (int) std::size (cv.trace);
    for (auto& v : views)
    {
        const bool on = isOn (v->mod);
        const bool anim = on && (v->mod == M_COMP || v->mod == M_TAPE || v->mod == M_DUST);
        if (anim)
        {
            v->spin += 0.06f * (v->mod == M_TAPE ? 1.0f + 0.3f * std::sin (v->spin * 0.37f) * proc.value ("t_wow") : 1.0f) + (v->mod == M_DUST ? 0.02f : 0.0f);
            v->repaint(); continue;
        }
        const auto k = v->key(); if (k != v->lastKey) { v->lastKey = k; v->repaint(); }
    }
    {
        const float pk[4] = { proc.inPk[0].load(), proc.inPk[1].load(), proc.outPk[0].load(), proc.outPk[1].load() };
        bool ch = false; for (int i = 0; i < 4; ++i) if (std::abs (pk[i] - mShown[i]) > 0.003f) { mShown[i] = pk[i]; ch = true; }
        if (ch) { const auto oc = outCard(); repaint (scaler.map (juce::Rectangle<float> (oc.getX() + 20, oc.getY() + 494, oc.getWidth() - 40, 56).toNearestInt())); }
        if (pk[2] > 0.0005f || pk[3] > 0.0005f) scope.repaint();
    }
    if (++tick % 8 == 0) { const auto k = staticKey(); if (k != lastKey) { lastKey = k; repaint(); } }
}
}
