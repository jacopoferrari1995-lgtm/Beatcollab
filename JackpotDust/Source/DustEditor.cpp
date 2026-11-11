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
    g.setColour (juce::Colours::black.withAlpha (0.65f)); g.fillRoundedRectangle (r.expanded (1.5f), rad + 1.5f);
    juce::ColourGradient cg (C::lcd1, r.getCentreX(), r.getCentreY(), C::lcd2, r.getX(), r.getY(), true); g.setGradientFill (cg); g.fillRoundedRectangle (r, rad);
    g.setColour (C::amber.withAlpha (0.035f));
    for (float x = r.getX() + 10; x < r.getRight(); x += 12) g.drawVerticalLine ((int) x, r.getY() + 2, r.getBottom() - 2);
    for (float y = r.getY() + 10; y < r.getBottom(); y += 12) g.drawHorizontalLine ((int) y, r.getX() + 2, r.getRight() - 2);
}
static juce::String hzT (float f) { return f >= 1000 ? juce::String (f / 1000.0f, f >= 10000 ? 1 : 2) + " kHz" : juce::String (juce::roundToInt (f)) + " Hz"; }
static const juce::Colour modCol[kMods] = { C::amber, C::rust, C::rose, C::phos, C::cream, C::olive };
static const char* modName[kMods] = { "SAMPLER", "SATURAZIONE", "COMPRESSORE", "FILTRI & EQ", "NASTRO", "POLVERE" };
static const char* modSub[kMods] = { "macchine d'epoca", "4 caratteri", "punch e colla", "taglia e scolpisci", "wow, flutter, eta", "vinile, crackle, hiss" };

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
void Knob::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w)
{
    if (onTouch) onTouch();
    param->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, param->getValue() + (w.deltaY > 0 ? 0.01f : -0.01f)));
}
void Knob::paint (juce::Graphics& g)
{
    const float lh = big ? 32.0f : 27.0f, size = juce::jmin ((float) getWidth(), getHeight() - lh);
    const auto c = lit ? col : col.withSaturation (0.15f).withMultipliedBrightness (0.55f);
    jlook::KnobLook kl; kl.big = big; kl.bipolar = bipolar; kl.enabled = true; kl.hover = hover ? 1.0f : 0.0f; kl.active = drag ? 1.0f : 0.0f;
    jlook::drawKnob (g, { (getWidth() - size) * 0.5f, 0, size, size }, param->getValue(), c, kl);
    g.setColour (lit ? C::dim : C::faint); g.setFont (uf (big ? 11.5f : 9.0f)); g.drawText (label, juce::Rectangle<float> (-6, size + 1, getWidth() + 12.0f, 14), juce::Justification::centred);
    glowText (g, text ? text() : param->getCurrentValueAsText(), juce::Rectangle<float> (-6, size + 15, getWidth() + 12.0f, 14), drag ? c.brighter (0.4f) : c, mf (big ? 12.5f : 10.0f), juce::Justification::centred);
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
    const bool on = param->getValue() > 0.5f; const float h = (float) juce::jmin (getHeight(), 20), w = h * 1.8f;
    auto t = juce::Rectangle<float> (0, (getHeight() - h) / 2, w, h); lcd (g, t, h / 2);
    const float d = h - 6, x = on ? t.getRight() - d - 3 : 3.0f;
    if (on) { g.setColour (col.withAlpha (0.35f)); g.fillEllipse (x - 3, t.getY(), d + 6, d + 6); }
    juce::ColourGradient lg (on ? col.brighter (0.6f) : juce::Colour (0xff4a4f5a), x, t.getY() + 3, on ? col : juce::Colour (0xff15171c), x, t.getY() + 3 + d, false); g.setGradientFill (lg); g.fillEllipse (x, t.getY() + 3, d, d);
    auto lr = juce::Rectangle<float> (w + 8, 0, getWidth() - w - 8, (float) getHeight());
    if (on) glowText (g, label, lr, col, uf (10.5f)); else { g.setColour (C::dim); g.setFont (uf (10.5f)); g.drawText (label, lr, juce::Justification::centredLeft); }
}

// ======================================================================= display dei moduli
static double biqMag (const Biquad& b, double f, double sr)
{
    const std::complex<double> z = std::polar (1.0, -2 * kPi * f / sr);
    return std::abs ((b.b0 + b.b1 * z + b.b2 * z * z) / (1.0 + b.a1 * z + b.a2 * z * z));
}
void ModView::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat(); lcd (g, b, 8);
    auto& P = ed.proc; const bool on = ed.isOn (mod); const auto c = on ? modCol[mod] : C::faint;
    auto r = b.reduced (10, 8);
    g.setFont (mf (9.0f));
    switch (mod)
    {
        case M_SAMP:
        {
            const float bits = P.value ("s_bits"), rate = P.value ("s_rate");
            const float lv = std::pow (2.0f, juce::jmax (2.0f, bits - 7.0f));   // gradini esagerati per vederli
            juce::Path sine, st; const int N = 220; const double span = 0.0022, f = 900;
            float held = 0; double hp = 1;
            for (int i = 0; i <= N; ++i)
            {
                const double t = span * i / N; const float v = (float) std::sin (2 * kPi * f * t) * 0.85f;
                hp += span / N * rate; if (hp >= 1) { hp -= std::floor (hp); held = std::round (v * lv) / lv; }
                const float x = r.getX() + r.getWidth() * i / N;
                if (i == 0) { sine.startNewSubPath (x, r.getCentreY() - v * r.getHeight() * 0.45f); st.startNewSubPath (x, r.getCentreY() - held * r.getHeight() * 0.45f); }
                else { sine.lineTo (x, r.getCentreY() - v * r.getHeight() * 0.45f); st.lineTo (x, st.getCurrentPosition().y); st.lineTo (x, r.getCentreY() - held * r.getHeight() * 0.45f); }
            }
            g.setColour (C::dim.withAlpha (0.5f)); g.strokePath (sine, juce::PathStrokeType (1.0f));
            g.setColour (c.withAlpha (0.3f)); g.strokePath (st, juce::PathStrokeType (4.0f)); g.setColour (c); g.strokePath (st, juce::PathStrokeType (1.5f));
            glowText (g, juce::String (juce::roundToInt (bits)) + " BIT  " + hzT (rate), r.withHeight (12), c, mf (10.0f), juce::Justification::centredRight);
            g.setColour (C::dim); g.drawText (sInfo ((int) P.value ("s_model")).aaOrder == 0 ? "senza anti-alias" : "con filtro anti-alias", r.withHeight (12), juce::Justification::centredLeft);
            break;
        }
        case M_SAT:
        {
            const int m = (int) P.value ("d_model"); const float gd = dbToG (P.value ("d_drive") * P.value ("amount"));
            double e = 0; for (int k = 0; k < 64; ++k) { const float y = shapeFn (m, gd * 0.5f * std::sin (2 * (float) kPi * k / 64)); e += y * y; }
            const float mk = 0.5f / (float) std::sqrt (e / 64 + 1e-12) * 0.7071f;
            g.setColour (C::dim.withAlpha (0.3f)); g.drawLine (r.getX(), r.getBottom(), r.getRight(), r.getY(), 1.0f);
            g.drawHorizontalLine ((int) r.getCentreY(), r.getX(), r.getRight()); g.drawVerticalLine ((int) r.getCentreX(), r.getY(), r.getBottom());
            juce::Path p;
            for (int i = 0; i <= 120; ++i) { const float x = -1 + 2 * i / 120.0f, y = juce::jlimit (-1.2f, 1.2f, shapeFn (m, x * gd) * mk * 1.4f); const float px = r.getX() + r.getWidth() * i / 120.0f, py = r.getCentreY() - y * r.getHeight() * 0.42f; if (i == 0) p.startNewSubPath (px, py); else p.lineTo (px, py); }
            g.setColour (c.withAlpha (0.3f)); g.strokePath (p, juce::PathStrokeType (4.0f)); g.setColour (c); g.strokePath (p, juce::PathStrokeType (1.6f));
            glowText (g, "+" + juce::String (P.value ("d_drive"), 1) + " dB", r.withHeight (12), c, mf (10.0f), juce::Justification::centredRight);
            g.setColour (C::dim); g.drawText ("ingresso -> uscita", r.withHeight (12), juce::Justification::centredLeft);
            break;
        }
        case M_COMP:
        {
            juce::Path p; const int n = (int) std::size (trace);
            for (int i = 0; i < n; ++i) { const float v = trace[(tw + i) % n]; const float x = r.getX() + r.getWidth() * i / (n - 1), y = r.getY() + 14 + juce::jlimit (0.0f, 1.0f, v / 18.0f) * (r.getHeight() - 30); if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y); }
            juce::Path f2 = p; f2.lineTo (r.getRight(), r.getY() + 14); f2.lineTo (r.getX(), r.getY() + 14); f2.closeSubPath();
            g.setColour (c.withAlpha (0.18f)); g.fillPath (f2); g.setColour (c); g.strokePath (p, juce::PathStrokeType (1.5f));
            const float gr = P.eng.gr.load();
            auto bar = juce::Rectangle<float> (r.getX(), r.getBottom() - 10, r.getWidth(), 8);
            g.setColour (juce::Colours::white.withAlpha (0.05f)); g.fillRoundedRectangle (bar, 3);
            g.setColour (c); g.fillRoundedRectangle (bar.withWidth (bar.getWidth() * juce::jlimit (0.0f, 1.0f, gr / 18.0f)), 3);
            glowText (g, "-" + juce::String (gr, 1) + " dB", r.withHeight (12), c, mf (10.0f), juce::Justification::centredRight);
            g.setColour (C::dim); g.drawText ("riduzione", r.withHeight (12), juce::Justification::centredLeft);
            break;
        }
        case M_FILT:
        {
            const double sr = 48000; Biquad hp, lp, bo, cr;
            const float res = P.value ("f_res"), fh = P.value ("f_hp"), fl = P.value ("f_lp"); const bool s24 = P.value ("f_24") > 0.5f;
            Biquad hp2, lp2;
            hp.hp (fh, 1.0 / (1.4142 - 1.33 * res * 0.6), sr); lp.lp (fl, 1.0 / (1.4142 - 1.33 * res), sr); hp2.hp (fh, 0.7071, sr); lp2.lp (fl, 0.7071, sr);
            bo.bell (70, 0.9, P.value ("f_boom"), sr); cr.bell (2600, 1.0, P.value ("f_crack"), sr);
            Biquad w1, w2, w3, ai; const float wm = P.value ("f_warm"), am = P.value ("f_air");
            w1.lowShelf (85, wm * 4.5, sr); w2.bell (320, 0.8, -wm * 2.5, sr); w3.highShelf (9000, -wm * 3.0, sr); ai.highShelf (11000, am * 7.0, sr);
            juce::Path p;
            for (int i = 0; i <= 140; ++i)
            {
                const double f = 20 * std::pow (1000.0, i / 140.0);
                double m = biqMag (bo, f, sr) * biqMag (cr, f, sr) * biqMag (w1, f, sr) * biqMag (w2, f, sr) * biqMag (w3, f, sr) * biqMag (ai, f, sr);
                if (fh > 21) m *= biqMag (hp, f, sr) * (s24 ? biqMag (hp2, f, sr) : 1.0);
                if (fl < 19900) m *= biqMag (lp, f, sr) * (s24 ? biqMag (lp2, f, sr) : 1.0);
                const float db = (float) juce::jlimit (-30.0, 18.0, 20 * std::log10 (m + 1e-9));
                const float x = r.getX() + r.getWidth() * i / 140.0f, y = r.getY() + r.getHeight() * (18 - db) / 48;
                if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
            }
            const float y0 = r.getY() + r.getHeight() * 18 / 48;
            g.setColour (C::dim.withAlpha (0.3f)); g.drawHorizontalLine ((int) y0, r.getX(), r.getRight());
            for (float hz : { 100.0f, 1000.0f, 10000.0f }) { const float x = r.getX() + r.getWidth() * std::log (hz / 20) / std::log (1000.0f); g.drawVerticalLine ((int) x, r.getY(), r.getBottom()); }
            g.setColour (c.withAlpha (0.3f)); g.strokePath (p, juce::PathStrokeType (4.0f)); g.setColour (c); g.strokePath (p, juce::PathStrokeType (1.6f));
            g.setColour (C::dim); g.drawText ("100", juce::Rectangle<float> (r.getX() + r.getWidth() * std::log (5.0f) / std::log (1000.0f) + 2, r.getBottom() - 11, 30, 10), juce::Justification::centredLeft);
            g.drawText ("1k", juce::Rectangle<float> (r.getX() + r.getWidth() * std::log (50.0f) / std::log (1000.0f) + 2, r.getBottom() - 11, 30, 10), juce::Justification::centredLeft);
            g.drawText ("10k", juce::Rectangle<float> (r.getX() + r.getWidth() * std::log (500.0f) / std::log (1000.0f) + 2, r.getBottom() - 11, 30, 10), juce::Justification::centredLeft);
            break;
        }
        case M_TAPE:
        {
            // due bobine che girano e oscillano col wow
            const float rad = r.getHeight() * 0.42f; const juce::Point<float> a (r.getX() + rad + 10, r.getCentreY()), bpt (r.getRight() - rad - 10, r.getCentreY());
            for (auto ctr : { a, bpt })
            {
                g.setColour (c.withAlpha (0.15f)); g.fillEllipse (juce::Rectangle<float> (rad * 2, rad * 2).withCentre (ctr));
                g.setColour (c.withAlpha (0.7f)); g.drawEllipse (juce::Rectangle<float> (rad * 2, rad * 2).withCentre (ctr), 1.5f);
                g.setColour (c.withAlpha (0.5f)); g.drawEllipse (juce::Rectangle<float> (rad * 0.5f, rad * 0.5f).withCentre (ctr), 1.2f);
                for (int k = 0; k < 3; ++k) { const float an = spin + k * 2.094f; g.drawLine (ctr.x + std::cos (an) * rad * 0.3f, ctr.y + std::sin (an) * rad * 0.3f, ctr.x + std::cos (an) * rad * 0.85f, ctr.y + std::sin (an) * rad * 0.85f, 2.0f); }
            }
            juce::Path tape; tape.startNewSubPath (a.x, a.y + rad);
            const float wob = std::sin (spin * 0.37f) * 4.0f * P.value ("t_wow");
            tape.quadraticTo ((a.x + bpt.x) / 2, a.y + rad + 8 + wob, bpt.x, bpt.y + rad);
            g.setColour (c); g.strokePath (tape, juce::PathStrokeType (1.5f));
            glowText (g, "ETA " + juce::String (juce::roundToInt (P.value ("t_age") * 100)) + "%", r.withHeight (12), c, mf (10.0f), juce::Justification::centred);
            break;
        }
        case M_DUST:
        default:
        {
            // disco che gira con i crackle che compaiono
            const float rad = r.getHeight() * 0.48f; const auto ctr = juce::Point<float> (r.getCentreX(), r.getCentreY());
            g.setColour (juce::Colour (0xff050505)); g.fillEllipse (juce::Rectangle<float> (rad * 2, rad * 2).withCentre (ctr));
            for (int k = 1; k < 8; ++k) { g.setColour (juce::Colours::white.withAlpha (0.04f + 0.01f * (k % 2))); g.drawEllipse (juce::Rectangle<float> (rad * 2 * k / 8, rad * 2 * k / 8).withCentre (ctr), 0.8f); }
            g.setColour (c.withAlpha (0.8f)); g.fillEllipse (juce::Rectangle<float> (rad * 0.5f, rad * 0.5f).withCentre (ctr));
            juce::Random rr ((juce::int64) (spin * 10));
            const int dots = on ? (int) (P.value ("n_vinyl") * 30 + P.value ("n_pops") * 6) : 0;
            for (int k = 0; k < dots; ++k) { const float an = rr.nextFloat() * 6.283f, rd = rad * (0.35f + 0.6f * rr.nextFloat()), s = 1 + rr.nextFloat() * (k < P.value ("n_pops") * 6 ? 4.0f : 1.5f); g.setColour (C::cream.withAlpha (0.4f + 0.6f * rr.nextFloat())); g.fillEllipse (ctr.x + std::cos (an) * rd - s / 2, ctr.y + std::sin (an) * rd - s / 2, s, s); }
            g.setColour (C::dim);
            g.drawText (P.value ("n_follow") > 0.5f ? "solo mentre suona" : "sempre", r.withHeight (12), juce::Justification::centredRight);
            break;
        }
    }
    if (! on) glowText (g, "SPENTO  -  clicca il nome o gira una manopola", b, C::dim, uf (10.0f), juce::Justification::centredBottom);
}
void Scope::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat(); lcd (g, b, 8);
    float d[2048]; ed.proc.ringOut.read (d, 2048);
    juce::Path p; auto r = b.reduced (6, 6);
    for (int i = 0; i < 2048; i += 4) { const float x = r.getX() + r.getWidth() * i / 2048.0f, y = r.getCentreY() - juce::jlimit (-1.0f, 1.0f, d[i]) * r.getHeight() * 0.48f; if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y); }
    g.setColour (C::amber.withAlpha (0.25f)); g.strokePath (p, juce::PathStrokeType (3.0f)); g.setColour (C::amber); g.strokePath (p, juce::PathStrokeType (1.2f));
}

// ======================================================================= editor
Editor::Editor (Processor& p) : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);
    auto turnOn = [this] (int m) { return [this, m] { if (! isOn (m)) P (onId (m))->setValueNotifyingHost (1.0f); }; };
    auto knob = [&] (int m, const char* id, const char* label, std::function<juce::String()> txt, bool bip = false) -> Knob*
    {
        auto k = std::make_unique<Knob> (P (id), m >= 0 ? modCol[m] : C::amber); k->label = label; k->text = std::move (txt); k->bipolar = bip;
        if (m >= 0) k->onTouch = turnOn (m);
        addAndMakeVisible (*k); knobMod.push_back ({ k.get(), m }); knobs.push_back (std::move (k)); return knobs.back().get();
    };
    auto pct = [this] (const char* id) { return [this, id] { return juce::String (juce::roundToInt (proc.value (id) * 100)) + "%"; }; };
    auto db = [this] (const char* id) { return [this, id] { const float v = proc.value (id); return (v > 0.05f ? "+" : "") + juce::String (v, 1) + " dB"; }; };
    // sampler
    knob (M_SAMP, "s_bits", "BIT", [this] { return juce::String (juce::roundToInt (proc.value ("s_bits"))) + " bit"; });
    knob (M_SAMP, "s_rate", "FREQUENZA", [this] { return hzT (proc.value ("s_rate")); });
    knob (M_SAMP, "s_mix", "MIX", pct ("s_mix"));
    // saturazione
    knob (M_SAT, "d_drive", "DRIVE", [this] { return "+" + juce::String (proc.value ("d_drive"), 1) + " dB"; });
    knob (M_SAT, "d_tone", "COLORE", [this] { const float v = proc.value ("d_tone"); return std::abs (v) < 0.05f ? juce::String ("NEUTRO") : v < 0 ? "CORPO " + juce::String (juce::roundToInt (-v * 100)) : "MORSO " + juce::String (juce::roundToInt (v * 100)); }, true);
    knob (M_SAT, "d_mix", "MIX", pct ("d_mix"));
    // compressore
    knob (M_COMP, "c_amt", "QUANTITA", pct ("c_amt"));
    knob (M_COMP, "c_att", "ATTACCO", [this] { return juce::String (proc.value ("c_att"), proc.value ("c_att") < 10 ? 1 : 0) + " ms"; });
    knob (M_COMP, "c_rel", "RILASCIO", [this] { return juce::String (juce::roundToInt (proc.value ("c_rel"))) + " ms"; });
    knob (M_COMP, "c_punch", "PUNCH", [this] { const float v = proc.value ("c_punch"); return (v > 0.01f ? "+" : "") + juce::String (juce::roundToInt (v * 100)); }, true);
    knob (M_COMP, "c_mix", "MIX", pct ("c_mix"));
    // filtri
    knob (M_FILT, "f_hp", "PASSA ALTO", [this] { return proc.value ("f_hp") <= 21 ? juce::String ("OFF") : hzT (proc.value ("f_hp")); });
    knob (M_FILT, "f_lp", "PASSA BASSO", [this] { return proc.value ("f_lp") >= 19900 ? juce::String ("OFF") : hzT (proc.value ("f_lp")); });
    knob (M_FILT, "f_res", "RISONANZA", pct ("f_res"));
    knob (M_FILT, "f_boom", "BOOM 70", db ("f_boom"), true);
    knob (M_FILT, "f_crack", "CRACK 2.6k", db ("f_crack"), true);
    knob (M_FILT, "f_warm", "CALDO", pct ("f_warm"));
    knob (M_FILT, "f_air", "ARIA", pct ("f_air"));
    // nastro
    knob (M_TAPE, "t_age", "ETA", pct ("t_age"));
    knob (M_TAPE, "t_wow", "WOW", pct ("t_wow"));
    knob (M_TAPE, "t_flut", "FLUTTER", pct ("t_flut"));
    knob (M_TAPE, "t_drop", "DROPOUT", pct ("t_drop"));
    // polvere
    knob (M_DUST, "n_vinyl", "VINILE", pct ("n_vinyl"));
    knob (M_DUST, "n_pops", "POP", pct ("n_pops"));
    knob (M_DUST, "n_hiss", "HISS", pct ("n_hiss"));
    knob (M_DUST, "n_hum", "RONZIO", pct ("n_hum"));
    // uscita
    kAmount = knob (-1, "amount", "INTENSITA", pct ("amount")); kAmount->big = true;
    knob (-1, "in", "INPUT", db ("in"), true);
    knob (-1, "mix", "MIX", pct ("mix"));
    knob (-1, "out", "OUTPUT", db ("out"), true);
    // selettori
    auto seg = [&] (int m, const char* id, juce::StringArray it, float font) -> Segs*
    { auto s = std::make_unique<Segs> (P (id), modCol[m], it); s->font = font; addAndMakeVisible (*s); segMod.push_back ({ s.get(), m }); segs.push_back (std::move (s)); return segs.back().get(); };
    juce::StringArray sm; for (int i = 0; i < kSModels; ++i) sm.add (sInfo (i).name);
    seg (M_SAMP, "s_model", sm, 9.5f)->onPick = [this] (int m)
    {   // la macchina porta i suoi bit e la sua frequenza
        auto* b = P ("s_bits"); b->setValueNotifyingHost (b->convertTo0to1 (sInfo (m).bits));
        auto* r = P ("s_rate"); r->setValueNotifyingHost (r->convertTo0to1 (sInfo (m).rate));
        if (! isOn (M_SAMP)) P ("s_on")->setValueNotifyingHost (1.0f);
    };
    juce::StringArray dm; for (int i = 0; i < kDModels; ++i) dm.add (dName (i));
    seg (M_SAT, "d_model", dm, 10.0f)->onPick = [this] (int) { if (! isOn (M_SAT)) P ("d_on")->setValueNotifyingHost (1.0f); };
    juce::StringArray cm; for (int i = 0; i < kCStyles; ++i) cm.add (cName (i));
    seg (M_COMP, "c_style", cm, 10.5f)->onPick = [this] (int) { if (! isOn (M_COMP)) P ("c_on")->setValueNotifyingHost (1.0f); };
    auto sw = [&] (const char* id, juce::Colour c, const char* l) { switches.push_back (std::make_unique<Switch> (P (id), c, l)); addAndMakeVisible (*switches.back()); };
    sw ("f_24", C::phos, "PENDENZA 24 dB/OTT"); sw ("n_follow", C::olive, "SOLO QUANDO SUONA LA BATTERIA");
    sw ("mono", C::amber, "MONO"); sw ("auto", C::amber, "VOLUME AUTOMATICO");
    for (int m = 0; m < kMods; ++m) { views.push_back (std::make_unique<ModView> (*this, m)); addAndMakeVisible (*views.back()); }
    addAndMakeVisible (scope);
    addAndMakeVisible (sizeChip);
    scaler.setup();
    startTimerHz (30);
}

void Editor::resized()
{
    const float sc = scaler.update();
    presetRect = { 450, 15, 330, 36 }; prevRect = { 412, 15, 36, 36 }; nextRect = { 782, 15, 36, 36 }; abRect = { 830, 15, 110, 36 };
    for (int m = 0; m < kMods; ++m)
    {
        const auto c = card (m);
        views[(size_t) m]->setBounds (juce::Rectangle<float> (c.getX() + 12, c.getY() + 38, 276, 112).toNearestInt());
        std::vector<Knob*> ks; for (auto& [k, mm] : knobMod) if (mm == m) ks.push_back (k);
        const float cw = 276.0f / (float) ks.size(), kw = juce::jmin (64.0f, cw - 4);
        for (size_t i = 0; i < ks.size(); ++i) ks[i]->setBounds (juce::Rectangle<float> (c.getX() + 12 + i * cw + (cw - kw) / 2, c.getY() + 198, kw, kw + 27).toNearestInt());
        for (auto& [s, mm] : segMod) if (mm == m) s->setBounds (juce::Rectangle<float> (c.getX() + 12, c.getY() + 160, 276, 28).toNearestInt());
    }
    switches[0]->setBounds (juce::Rectangle<float> (card (M_FILT).getX() + 14, card (M_FILT).getY() + 164, 260, 22).toNearestInt());
    switches[1]->setBounds (juce::Rectangle<float> (card (M_DUST).getX() + 14, card (M_DUST).getY() + 164, 270, 22).toNearestInt());
    // uscita
    kAmount->setBounds (994, 142, 140, 140 + 32);
    std::vector<Knob*> outs; for (auto& [k, mm] : knobMod) if (mm < 0 && k != kAmount) outs.push_back (k);
    for (size_t i = 0; i < outs.size(); ++i) outs[i]->setBounds (962 + (int) i * 72, 334, 62, 62 + 27);
    switches[2]->setBounds (964, 436, 200, 22); switches[3]->setBounds (964, 464, 200, 22);
    scope.setBounds (962, 500, 206, 116);
    sizeChip.setBounds (W - 76, H - 18, 58, 15);
    jlook::scaleChildren (*this, sc);
}

void Editor::paint (juce::Graphics& g)
{
    g.addTransform (juce::AffineTransform::scale (scaler.s));
    const float Wf = (float) W, Hf = (float) H;
    jlook::backdrop (g, { 0, 0, Wf, Hf }, C::amber, C::rust);
    jlook::header (g, { 0, 0, Wf, 64 }, "DUST", C::amber, "JD-12  DRUM VINTAGE / LO-FI / BOOM BAP", C::rust, C::amber);
    lcd (g, presetRect.toFloat(), 8); jlook::glass (g, presetRect.toFloat(), 8);
    glowText (g, proc.getProgramName (proc.preset).toUpperCase(), presetRect.toFloat(), C::amber, mf (12.5f), juce::Justification::centred);
    g.setColour (C::amber); g.setFont (uf (20.0f));
    g.drawText (juce::String::fromUTF8 ("\xe2\x80\xb9"), prevRect, juce::Justification::centred); g.drawText (juce::String::fromUTF8 ("\xe2\x80\xba"), nextRect, juce::Justification::centred);
    jlook::button (g, abRect.toFloat(), C::amber, proc.ab.cur == 1, abHover, "MEMORIA " + proc.ab.label(), uf (11.5f));
    for (int m = 0; m < kMods; ++m)
    {
        const auto c = card (m); const bool on = isOn (m); const auto col = modCol[m];
        jlook::softShadow (g, c, 14); jlook::card (g, c, 14, on ? col : C::faint, true, false, false);
        auto led = juce::Rectangle<float> (c.getX() + 16, c.getY() + 13, 10, 10);
        if (on) { g.setColour (col.withAlpha (0.35f)); g.fillEllipse (led.expanded (4)); }
        g.setColour (on ? col : juce::Colour (0xff3a3d45)); g.fillEllipse (led);
        glowText (g, juce::String (m + 1) + "  " + modName[m], { c.getX() + 34, c.getY() + 7, 200, 22 }, on ? C::text : C::dim, uf (13.0f));
        g.setColour (C::dim); g.setFont (mf (9.0f)); g.drawText (modSub[m], juce::Rectangle<float> (c.getX() + 150, c.getY() + 9, c.getWidth() - 166, 18), juce::Justification::centredRight);
        // freccia del flusso
        if (m % 3 != 2) { g.setColour (C::amber.withAlpha (0.35f)); juce::Path a; a.addTriangle (c.getRight() + 2, c.getCentreY() - 6, c.getRight() + 9, c.getCentreY(), c.getRight() + 2, c.getCentreY() + 6); g.fillPath (a); }
        // descrizione sotto le manopole
        juce::String d;
        if (m == M_SAMP) d = sInfo ((int) proc.value ("s_model")).desc;
        else if (m == M_SAT) d = dDesc ((int) proc.value ("d_model"));
        else if (m == M_COMP) { const int s = (int) proc.value ("c_style"); d = s == 0 ? "Veloce sui picchi, morde i transienti; oltre meta aggiunge lo sporco del FET a tutti i bottoni." : s == 1 ? "Lento e morbido, tiene insieme il kit: colla da bus." : "Respira e pompa: le code salgono tra un colpo e l'altro."; }
        else if (m == M_FILT) d = "BOOM spinge il peso della cassa, CRACK lo schiocco del rullante. Risonanza = filtro da campionatore.";
        else if (m == M_TAPE) d = "ETA scurisce, satura e alza il bump in basso. WOW lento, FLUTTER veloce, DROPOUT cali casuali.";
        else d = "Fruscio e crackle del disco, pop rari, hiss del nastro e ronzio a 50 Hz. Non passa nel compressore.";
        g.setColour (on ? C::text.withAlpha (0.7f) : C::faint); g.setFont (uf (9.5f, false));
        g.drawFittedText (d, juce::Rectangle<float> (c.getX() + 14, c.getY() + 290, c.getWidth() - 28, 36).toNearestInt(), juce::Justification::topLeft, 3);
    }
    // uscita
    const juce::Rectangle<float> oc { 948, 76, 234, 674 };
    jlook::softShadow (g, oc, 14); jlook::card (g, oc, 14, C::amber, true, false, false);
    glowText (g, "USCITA", { oc.getX() + 16, oc.getY() + 7, 200, 22 }, C::text, uf (13.0f));
    g.setColour (C::dim); g.setFont (uf (9.5f, false));
    g.drawFittedText ("INTENSITA alza o abbassa tutto insieme: macchina, drive, compressione, nastro e polvere.", juce::Rectangle<int> (962, 106, 206, 28), juce::Justification::topLeft, 2);
    auto meter = [&] (float y, const char* t, float l, float r)
    {
        g.setColour (C::dim); g.setFont (mf (9.0f)); g.drawText (t, juce::Rectangle<float> (962, y, 40, 10), juce::Justification::centredLeft);
        for (int c = 0; c < 2; ++c)
        {
            auto m = juce::Rectangle<float> (996, y + c * 7.0f, 172, 5);
            const float dbv = gToDb (c ? r : l), n = juce::jlimit (0.0f, 1.0f, (dbv + 60) / 63);
            for (int k = 0; k < 30; ++k) { const float t2 = k / 30.0f; g.setColour (t2 <= n ? (t2 > 0.93f ? C::red : t2 > 0.8f ? C::amber : C::olive) : juce::Colours::white.withAlpha (0.05f)); g.fillRect (m.getX() + k * m.getWidth() / 30, m.getY(), m.getWidth() / 30 - 1.2f, m.getHeight()); }
        }
    };
    meter (630, "IN", proc.inPk[0].load(), proc.inPk[1].load());
    meter (652, "OUT", proc.outPk[0].load(), proc.outPk[1].load());
    g.setColour (C::dim); g.setFont (uf (9.5f, false));
    g.drawFittedText ("Ordine: sampler, saturazione, filtri, compressore, nastro, polvere. Latenza " + juce::String (proc.getLatencySamples()) + " campioni, compensata.",
                      juce::Rectangle<int> (962, 680, 206, 56), juce::Justification::topLeft, 4);
}

void Editor::mouseMove (const juce::MouseEvent& e0)
{
    const auto e = e0.withNewPosition (e0.position / scaler.s);
    const bool h = abRect.contains (e.getPosition());
    if (h != abHover) { abHover = h; repaint (scaler.map (abRect.expanded (8))); }
    bool hand = h || presetRect.contains (e.getPosition()) || prevRect.contains (e.getPosition()) || nextRect.contains (e.getPosition());
    for (int m = 0; m < kMods; ++m) if (card (m).withHeight (32).contains (e.position)) hand = true;
    setMouseCursor (hand ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
}
void Editor::mouseDown (const juce::MouseEvent& e0)
{
    const auto e = e0.withNewPosition (e0.position / scaler.s);
    for (int m = 0; m < kMods; ++m) if (card (m).withHeight (32).contains (e.position)) { auto* p = P (onId (m)); p->setValueNotifyingHost (p->getValue() > 0.5f ? 0.0f : 1.0f); repaint(); return; }
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
    for (auto& [k, m] : knobMod) { const bool lit = m < 0 || isOn (m); const float v = k->param->getValue() + (lit ? 0.0f : 9.0f); if (lit != k->lit || v != k->shown) { k->lit = lit; k->shown = v; k->repaint(); } }
    for (auto& [s, m] : segMod) { const bool lit = isOn (m); if (lit != s->lit) { s->lit = lit; } s->repaint(); }
    auto& cv = *views[M_COMP]; cv.trace[cv.tw] = proc.eng.gr.load(); cv.tw = (cv.tw + 1) % (int) std::size (cv.trace);
    for (auto& v : views) { v->spin += 0.06f * (v->mod == M_TAPE ? 1.0f + 0.3f * std::sin (v->spin * 0.37f) * proc.value ("t_wow") : 1.0f); if (v->mod == M_DUST) v->spin += 0.02f; }
    for (auto& v : views) v->repaint();
    scope.repaint();
    if (++tick % 3 == 0) { for (auto& s : switches) s->repaint(); repaint (scaler.map ({ 948, 620, 234, 50 })); }
    if (tick % 10 == 0) repaint();
}
}
