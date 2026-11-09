#include "ButterEditor.h"

namespace jb
{
juce::Font bf (float size, bool bold, bool mono)
{
    auto o = juce::FontOptions (size);
    if (mono) o = o.withName (juce::Font::getDefaultMonospacedFontName());
    if (bold) o = o.withStyle ("Bold");
    return juce::Font (o);
}
static const char* kModNames[6] = { "BOOM / NOISE", "SHIFT", "COMPRESS", "SATURATE", "SPACE", "CLIP" };
static juce::String pct (float v) { return juce::String (juce::roundToInt (v * 100)) + "%"; }
static juce::String bip (float v) { return std::abs (v) < 0.01f ? juce::String ("0") : (v > 0 ? "+" : "") + juce::String (juce::roundToInt (v * 100)); }

static void glow (juce::Graphics& g, const juce::String& t, juce::Rectangle<float> r, juce::Colour c, juce::Font f, juce::Justification j = juce::Justification::centredLeft)
{
    g.setFont (f);
    for (int k = 2; k >= 1; --k) { g.setColour (c.withAlpha (0.12f)); g.drawText (t, r.translated ((float) -k * 0.6f, 0), j); g.drawText (t, r.translated ((float) k * 0.6f, 0), j); }
    g.setColour (c); g.drawText (t, r, j);
}
static void inset (juce::Graphics& g, juce::Rectangle<float> r, float rad)
{
    g.setColour (juce::Colours::black.withAlpha (0.65f)); g.fillRoundedRectangle (r.expanded (1.5f), rad + 1.5f);
    juce::ColourGradient cg (BC::lcd1, r.getCentreX(), r.getCentreY(), BC::lcd2, r.getX(), r.getY(), true); g.setGradientFill (cg); g.fillRoundedRectangle (r, rad);
    g.setColour (juce::Colours::white.withAlpha (0.05f)); g.drawLine (r.getX() + rad, r.getBottom() + 1.5f, r.getRight() - rad, r.getBottom() + 1.5f);
}
static void scan (juce::Graphics& g, juce::Rectangle<float> r)
{
    g.setColour (juce::Colours::black.withAlpha (0.22f)); for (float y = r.getY(); y < r.getBottom(); y += 3) g.drawHorizontalLine ((int) y, r.getX(), r.getRight());
    juce::ColourGradient gl (juce::Colours::white.withAlpha (0.05f), r.getX(), r.getY(), juce::Colours::transparentWhite, r.getX() + r.getWidth() * 0.4f, r.getY() + r.getHeight() * 0.7f, false);
    g.setGradientFill (gl); g.fillRect (r);
}

// ======================= manopola 3D con anello LED =======================
void PKnob::paint (juce::Graphics& g)
{
    const float lh = isBig ? 30.0f : 27.0f;
    const float size = juce::jmin ((float) getWidth(), getHeight() - lh);
    const float cx = getWidth() * 0.5f, cy = size * 0.5f + 1, R = size * 0.5f - 3, r = R - (isBig ? 11.0f : 8.0f);
    const float v = param->getValue(), a0 = -2.42f, a1 = 2.42f;
    const int nLed = isBig ? 31 : 21;
    for (int i = 0; i < nLed; ++i)
    {
        const float t = i / (float) (nLed - 1), a = a0 + t * (a1 - a0);
        const bool lit = bipolar ? ((v >= 0.5f && t >= 0.5f && t <= v + 0.001f) || (v < 0.5f && t <= 0.5f && t >= v - 0.001f)) : (t <= v + 0.001f && v > 0.002f);
        const float x = cx + std::sin (a) * R, y = cy - std::cos (a) * R, d = isBig ? 4.2f : 3.4f;
        if (lit) { g.setColour (col.withAlpha (0.25f)); g.fillEllipse (x - d * 1.3f, y - d * 1.3f, d * 2.6f, d * 2.6f); g.setColour (col.brighter (0.25f)); }
        else g.setColour (juce::Colour (0xff23262e));
        g.fillEllipse (x - d * 0.5f, y - d * 0.5f, d, d);
    }
    // corpo: gonna metallica + tappo zigrinato
    g.setColour (juce::Colours::black.withAlpha (0.7f)); g.fillEllipse (cx - r - 1, cy - r + 4, r * 2 + 2, r * 2 + 2);
    juce::ColourGradient sk (juce::Colour (0xff4a4f5a), cx, cy - r, juce::Colour (0xff0c0d11), cx, cy + r, false);
    g.setGradientFill (sk); g.fillEllipse (cx - r, cy - r, r * 2, r * 2);
    const float r2 = r * 0.8f;
    juce::ColourGradient cap (juce::Colour (0xff343843), cx - r2 * 0.5f, cy - r2, juce::Colour (0xff111318), cx + r2 * 0.4f, cy + r2, false);
    g.setGradientFill (cap); g.fillEllipse (cx - r2, cy - r2, r2 * 2, r2 * 2);
    for (int i = 0; i < 36; ++i) { const float a = i * juce::MathConstants<float>::twoPi / 36; g.setColour (juce::Colours::black.withAlpha (0.4f)); g.drawLine (cx + std::sin (a) * r2, cy - std::cos (a) * r2, cx + std::sin (a) * r, cy - std::cos (a) * r, 1.0f); }
    juce::ColourGradient spec (juce::Colours::white.withAlpha (0.16f), cx - r2 * 0.4f, cy - r2 * 0.7f, juce::Colours::transparentWhite, cx, cy, true);
    g.setGradientFill (spec); g.fillEllipse (cx - r2, cy - r2, r2 * 2, r2 * 2);
    g.setColour (col.withAlpha (0.55f)); g.drawEllipse (cx - r2, cy - r2, r2 * 2, r2 * 2, 1.0f);
    const float a = a0 + v * (a1 - a0);
    g.setColour (col.withAlpha (0.4f)); g.drawLine (cx + std::sin (a) * r2 * 0.25f, cy - std::cos (a) * r2 * 0.25f, cx + std::sin (a) * r2 * 0.95f, cy - std::cos (a) * r2 * 0.95f, isBig ? 6.0f : 4.0f);
    g.setColour (juce::Colours::white); g.drawLine (cx + std::sin (a) * r2 * 0.3f, cy - std::cos (a) * r2 * 0.3f, cx + std::sin (a) * r2 * 0.92f, cy - std::cos (a) * r2 * 0.92f, isBig ? 2.6f : 1.8f);
    g.setColour (BC::dim); g.setFont (bf (isBig ? 10.5f : 9.0f, true)); g.drawText (label, juce::Rectangle<float> (0, size + 2, (float) getWidth(), 12), juce::Justification::centred);
    glow (g, fmt ? fmt (param->convertFrom0to1 (v)) : param->getCurrentValueAsText(), juce::Rectangle<float> (0, size + 14, (float) getWidth(), 13), active ? col.brighter (0.4f) : col, bf (isBig ? 11.5f : 10.0f, true, true), juce::Justification::centred);
}

void Pill::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (1);
    inset (g, b, 5);
    glow (g, param->getCurrentChoiceName(), b, col, bf (10.5f, true, true), juce::Justification::centred);
    g.setColour (col.withAlpha (0.6f)); juce::Path t; t.addTriangle (b.getRight() - 13, b.getCentreY() - 2, b.getRight() - 7, b.getCentreY() - 2, b.getRight() - 10, b.getCentreY() + 2); g.fillPath (t);
}
void Pill::mouseDown (const juce::MouseEvent&)
{
    juce::PopupMenu m; for (int i = 0; i < param->choices.size(); ++i) m.addItem (i + 1, param->choices[i], true, i == param->getIndex());
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [this] (int r) { if (r > 0) param->setValueNotifyingHost (param->convertTo0to1 ((float) (r - 1))); });
}

void Segs::paint (juce::Graphics& g)
{
    const int n = param->choices.size(); const float w = getWidth() / (float) n; const int cur = param->getIndex();
    inset (g, getLocalBounds().toFloat(), 4);
    for (int i = 0; i < n; ++i)
    {
        auto r = juce::Rectangle<float> (i * w, 0, w, (float) getHeight()).reduced (2);
        if (i == cur) { g.setColour (col.withAlpha (0.25f)); g.fillRoundedRectangle (r, 3); g.setColour (col); g.drawRoundedRectangle (r, 3, 1.0f); }
        const auto t = param->choices[i] == "FULL" ? juce::String ("ALL") : param->choices[i].substring (0, 1);
        if (i == cur) glow (g, t, r, col, bf (9.0f, true, true), juce::Justification::centred);
        else { g.setColour (BC::dim); g.setFont (bf (9.0f, true, true)); g.drawText (t, r, juce::Justification::centred); }
    }
}

void Target::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setFont (bf (8.0f, true, true)); g.setColour (BC::dim);
    g.drawText ("SUST", b.withHeight (10), juce::Justification::centredLeft); g.drawText ("TARGET", b.withHeight (10), juce::Justification::centred); g.drawText ("TRANS", b.withHeight (10), juce::Justification::centredRight);
    auto tr = juce::Rectangle<float> (10, b.getHeight() - 10, b.getWidth() - 20, 5);
    inset (g, tr.expanded (2), 3);
    const float v = param->getValue(), x = tr.getX() + v * tr.getWidth();
    g.setColour (col.withAlpha (0.3f)); g.fillRect (juce::jmin (x, tr.getCentreX()), tr.getY() - 1, std::abs (x - tr.getCentreX()), tr.getHeight() + 2);
    g.setColour (col); g.fillRect (juce::jmin (x, tr.getCentreX()), tr.getY() + 1, std::abs (x - tr.getCentreX()), tr.getHeight() - 2);
    g.setColour (BC::dim); g.drawVerticalLine ((int) tr.getCentreX(), tr.getY() - 3, tr.getBottom() + 3);
    juce::ColourGradient kn (juce::Colour (0xff50555f), x, tr.getCentreY() - 7, juce::Colour (0xff15171c), x, tr.getCentreY() + 7, false);
    g.setGradientFill (kn); g.fillRoundedRectangle (x - 5, tr.getCentreY() - 8, 10, 16, 2); g.setColour (col); g.drawHorizontalLine ((int) tr.getCentreY(), x - 4, x + 4);
}

void Switch::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat(); const bool on = param->getValue() > 0.5f;
    auto led = b.removeFromLeft (b.getHeight()).reduced (3);
    g.setColour (juce::Colours::black.withAlpha (0.7f)); g.fillEllipse (led.expanded (2));
    if (on) { g.setColour (col.withAlpha (0.3f)); g.fillEllipse (led.expanded (4)); }
    juce::ColourGradient lg (on ? col.brighter (0.6f) : juce::Colour (0xff30333b), led.getCentreX(), led.getY(), on ? col : juce::Colour (0xff15171c), led.getCentreX(), led.getBottom(), false);
    g.setGradientFill (lg); g.fillEllipse (led);
    if (label.isNotEmpty()) { g.setColour (on ? BC::text : BC::dim); g.setFont (bf (10.0f, true)); g.drawText (label, b.withTrimmedLeft (5), juce::Justification::centredLeft); }
}

// ======================= curve per i display =======================
static float satCurve (int type, float x, float a, float st)
{
    const float d = 1.0f + a * 12.0f, X = x * d; float y;
    switch (type)
    {
        case 1: y = std::tanh (X * 0.8f) + 0.08f * st * X * X / (1 + X * X); break;
        case 2: y = std::atan (X * (1 + st)) / std::atan (1.0f + st) * 0.8f; break;
        case 3: { const float k = 0.55f + 0.45f * (1 - st); y = X > 0 ? std::tanh (X) : std::tanh (X * k) / k * 0.8f; y += 0.12f * st * y * y; break; }
        case 4: { const float q = std::pow (2.0f, 4.0f - st * 2.5f); y = std::round (juce::jlimit (-1.0f, 1.0f, X) * q) / q; break; }
        case 5: y = std::sin (X * (1.0f + st * 2.0f)) * 0.9f; break;
        default: y = std::tanh (X + st * 0.3f * X * X / (1 + std::abs (X))); break;
    }
    return y / (1.0f + a * 1.2f) * (1.0f + a * 0.6f);
}
static float clipCurve (float x, float a, int stages)
{
    float y = x * (1.0f + a * 10.0f);
    for (int s = 0; s < stages; ++s) y = (s % 2 == 0) ? std::tanh (y * 1.1f) : juce::jlimit (-0.9f, 0.9f, y * 1.25f);
    return y / (1.0f + a * 0.8f) * (1.0f + a * 0.5f);
}

// ======================= modulo =======================
Module::Module (Editor& e, int m) : ed (e), mod (m)
{
    auto& A = ed.proc.apvts; const auto c = BC::mod[m];
    auto P = [&A] (const char* id) { return A.getParameter (id); };
    auto C = [&A] (const char* id) { return dynamic_cast<juce::AudioParameterChoice*> (A.getParameter (id)); };
    auto addSmall = [this, c] (juce::RangedAudioParameter* p, const char* l, std::function<juce::String (float)> f, bool bp = false)
    { auto* k = smalls.add (new PKnob (p, l, c, false, std::move (f))); k->bipolar = bp; addAndMakeVisible (k); };
    switch (m)
    {
        case 0:
            power = std::make_unique<Switch> (P ("u_bOn"), "", c);
            big = std::make_unique<PKnob> (P ("u_boom"), "BOOM", c, true, pct);
            addSmall (P ("u_bTone"), "FREQ", [] (float v) { return juce::String (juce::roundToInt (v)) + " Hz"; });
            addSmall (P ("u_noise"), "NOISE", pct);
            extra = std::make_unique<Switch> (P ("u_acid"), "ACID", c);
            extraPill = std::make_unique<Pill> (C ("u_nType"), c);
            break;
        case 1:
            power = std::make_unique<Switch> (P ("u_shOn"), "", c);
            big = std::make_unique<PKnob> (P ("u_shAmt"), "MIX", c, true, pct);
            addSmall (P ("u_shPitch"), "PITCH", [] (float v) { return (v > 0 ? "+" : "") + juce::String (v, 1) + " st"; }, true);
            focus = std::make_unique<Segs> (C ("u_shFoc"), c); target = std::make_unique<Target> (P ("u_shTgt"), c);
            break;
        case 2:
            power = std::make_unique<Switch> (P ("u_cOn"), "", c);
            type = std::make_unique<Pill> (C ("u_cType"), c);
            big = std::make_unique<PKnob> (P ("u_cAmt"), "AMOUNT", c, true, [] (float v) { return std::abs (v) < 0.01f ? juce::String ("0") : (v > 0 ? juce::String ("GIU ") : juce::String ("SU ")) + juce::String (juce::roundToInt (std::abs (v) * 100)); });
            big->bipolar = true;
            addSmall (P ("u_cBands"), "BANDE", pct);
            break;
        case 3:
            power = std::make_unique<Switch> (P ("u_sOn"), "", c);
            type = std::make_unique<Pill> (C ("u_sType"), c);
            big = std::make_unique<PKnob> (P ("u_sAmt"), "AMOUNT", c, true, pct);
            addSmall (P ("u_sStyle"), "STILE", pct);
            focus = std::make_unique<Segs> (C ("u_sFoc"), c); target = std::make_unique<Target> (P ("u_sTgt"), c);
            break;
        case 4:
            power = std::make_unique<Switch> (P ("u_spOn"), "", c);
            type = std::make_unique<Pill> (C ("u_spType"), c);
            big = std::make_unique<PKnob> (P ("u_spAmt"), "AMOUNT", c, true, pct);
            addSmall (P ("u_spDecay"), "DECAY", pct); addSmall (P ("u_spPre"), "PRE", [] (float v) { return juce::String (juce::roundToInt (v)) + " ms"; });
            addSmall (P ("u_spDuck"), "DUCK", pct); addSmall (P ("u_spWidth"), "WIDTH", pct);
            break;
        default:
            power = std::make_unique<Switch> (P ("u_mOn"), "", c);
            big = std::make_unique<PKnob> (P ("u_mAmt"), "DRIVE", c, true, pct);
            addSmall (P ("u_mTone"), "TONO", bip, true);
            extraPill = std::make_unique<Pill> (C ("u_mStages"), c);
            focus = std::make_unique<Segs> (C ("u_mFoc"), c); target = std::make_unique<Target> (P ("u_mTgt"), c);
            break;
    }
    for (juce::Component* comp : std::initializer_list<juce::Component*> { power.get(), big.get(), type.get(), focus.get(), target.get(), extra.get(), extraPill.get() })
        if (comp) addAndMakeVisible (comp);
}

void Module::resized()
{
    const int w = getWidth();
    power->setBounds (8, 8, 20, 20);
    if (type) type->setBounds (10, 128, w - 20, 22);
    big->setBounds (14, type ? 154 : 136, w - 28, 128);
    const int sw = (w - 12) / 2, y0 = type ? 288 : 272;
    for (int i = 0; i < smalls.size(); ++i) smalls[i]->setBounds (6 + (i % 2) * sw, y0 + (i / 2) * 78, sw, 76);
    if (mod == 0) { extra->setBounds (12, 358, sw - 4, 22); extraPill->setBounds (6 + sw + 2, 358, sw - 6, 22); }
    if (mod == 5) extraPill->setBounds (6 + sw + 6, y0 + 22, sw - 12, 22);
    if (focus) focus->setBounds (10, getHeight() - 62, w - 20, 20);
    if (target) target->setBounds (6, getHeight() - 36, w - 12, 28);
}

void Module::drawDisplay (juce::Graphics& g, juce::Rectangle<float> r)
{
    auto& P = ed.proc; auto& D = P.dsp; const auto c = BC::mod[mod];
    auto v = [&P] (const char* id) { return P.apvts.getRawParameterValue (id)->load(); };
    inset (g, r, 6);
    auto in = r.reduced (5);
    g.setColour (BC::phos.withAlpha (0.04f));
    for (float x = in.getX(); x < in.getRight(); x += 10) g.drawVerticalLine ((int) x, in.getY(), in.getBottom());
    for (float y = in.getY(); y < in.getBottom(); y += 10) g.drawHorizontalLine ((int) y, in.getX(), in.getRight());
    // inviluppo d'uscita come base comune
    auto envPath = [&] (const Scope& sc, float scale) { juce::Path p; for (int k = 0; k < Scope::N; ++k) { const float x = in.getX() + in.getWidth() * k / (Scope::N - 1), y = in.getBottom() - juce::jmin (1.0f, sc.at (k) * scale) * in.getHeight(); if (k == 0) p.startNewSubPath (x, y); else p.lineTo (x, y); } return p; };
    auto transfer = [&] (std::function<float (float)> f, float level)
    {
        g.setColour (BC::phos.withAlpha (0.15f)); g.drawLine (in.getX(), in.getBottom(), in.getRight(), in.getY(), 1.0f);
        juce::Path p; const float cx = in.getCentreX(), cy = in.getCentreY(), hw = in.getWidth() * 0.5f, hh = in.getHeight() * 0.5f;
        for (int i = 0; i <= 60; ++i) { const float x = -1 + i / 30.0f, y = juce::jlimit (-1.1f, 1.1f, f (x)); if (i == 0) p.startNewSubPath (cx + x * hw, cy - y * hh); else p.lineTo (cx + x * hw, cy - y * hh); }
        g.setColour (c.withAlpha (0.3f)); g.strokePath (p, juce::PathStrokeType (4.0f)); g.setColour (c); g.strokePath (p, juce::PathStrokeType (1.6f));
        const float lx = juce::jmin (1.0f, level), ly = juce::jlimit (-1.1f, 1.1f, f (lx));
        g.setColour (juce::Colours::white); g.fillEllipse (cx + lx * hw - 3, cy - ly * hh - 3, 6, 6);
        g.setColour (c.withAlpha (0.4f)); g.fillEllipse (cx + lx * hw - 6, cy - ly * hh - 6, 12, 12);
    };
    const float level = D.scIn.at (Scope::N - 1);
    switch (mod)
    {
        case 0:
        {
            const float f0 = v ("u_bTone"), q = v ("u_acid") > 0.5f ? 9.0f : 1.6f, amt = v ("u_boom");
            juce::Path p;
            for (int i = 0; i <= 80; ++i)
            {
                const float f = 20.0f * std::pow (25.0f, i / 80.0f), w = f / f0, mag = (w / q) / std::sqrt ((1 - w * w) * (1 - w * w) + (w / q) * (w / q));
                const float y = in.getBottom() - in.getHeight() * 0.9f * juce::jlimit (0.0f, 1.0f, mag * amt * 0.8f + 0.02f);
                const float x = in.getX() + in.getWidth() * i / 80.0f; if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
            }
            g.setColour (BC::phos.withAlpha (0.25f)); g.strokePath (envPath (D.scIn, 1.2f), juce::PathStrokeType (1.0f));
            g.setColour (c.withAlpha (0.3f)); g.strokePath (p, juce::PathStrokeType (4.0f)); g.setColour (c); g.strokePath (p, juce::PathStrokeType (1.6f));
            glow (g, juce::String (juce::roundToInt (f0)) + " Hz", in.withHeight (12), c, bf (9.5f, true, true), juce::Justification::centredRight);
            break;
        }
        case 1:
        {
            const float tgt = v ("u_shTgt");
            for (int k = 0; k < Scope::N; ++k)
            {
                const float tr = D.scTr.at (k), w = tgt >= 0 ? (1 - tgt) + tgt * tr : (1 + tgt) + (-tgt) * (1 - tr);
                const float x = in.getX() + in.getWidth() * k / Scope::N;
                g.setColour (c.withAlpha (0.08f + 0.5f * w * v ("u_shAmt"))); g.fillRect (x, in.getY(), in.getWidth() / Scope::N + 0.5f, in.getHeight());
            }
            g.setColour (BC::phos.withAlpha (0.7f)); g.strokePath (envPath (D.scOut, 1.2f), juce::PathStrokeType (1.2f));
            const float st = v ("u_shPitch");
            glow (g, (st > 0 ? "+" : "") + juce::String (st, 1) + " st", in.reduced (2), c, bf (16.0f, true, true), juce::Justification::centred);
            break;
        }
        case 2:
        {
            g.setColour (BC::phos.withAlpha (0.25f)); g.strokePath (envPath (D.scIn, 1.2f), juce::PathStrokeType (1.0f));
            for (int k = 0; k < Scope::N; ++k)
            {
                const float gr = juce::jlimit (0.0f, 18.0f, D.scGr.at (k)), x = in.getX() + in.getWidth() * k / Scope::N;
                g.setColour (c.withAlpha (0.85f)); g.fillRect (x, in.getY(), in.getWidth() / Scope::N + 0.6f, in.getHeight() * gr / 18.0f);
            }
            g.setColour (BC::phos.withAlpha (0.8f)); g.strokePath (envPath (D.scOut, 1.2f), juce::PathStrokeType (1.2f));
            glow (g, "GR -" + juce::String (D.grMeter.load(), 1) + " dB", in.withTrimmedTop (in.getHeight() - 12), c, bf (9.5f, true, true), juce::Justification::centredRight);
            break;
        }
        case 3:
        {
            const int t = (int) v ("u_sType"); const float a = v ("u_sAmt"), st = v ("u_sStyle");
            transfer ([=] (float x) { return satCurve (t, x, a, st); }, level);
            break;
        }
        case 4:
        {
            const float dec = v ("u_spDecay"), pre = v ("u_spPre") / 120.0f * 0.25f, amt = v ("u_spAmt");
            g.setColour (BC::phos.withAlpha (0.25f)); g.strokePath (envPath (D.scOut, 1.2f), juce::PathStrokeType (1.0f));
            juce::Path p; p.startNewSubPath (in.getX(), in.getBottom());
            for (int i = 0; i <= 60; ++i)
            {
                const float t = i / 60.0f, x = in.getX() + in.getWidth() * t;
                const float y = t < pre ? 0.0f : std::exp (-(t - pre) * (7.0f - dec * 5.5f)) * (0.3f + 0.7f * amt);
                p.lineTo (x, in.getBottom() - y * in.getHeight() * 0.9f);
            }
            p.lineTo (in.getRight(), in.getBottom()); p.closeSubPath();
            g.setColour (c.withAlpha (0.25f)); g.fillPath (p); g.setColour (c); g.strokePath (p, juce::PathStrokeType (1.4f));
            break;
        }
        default:
        {
            const float a = v ("u_mAmt"); const int stg = (int) v ("u_mStages") + 1;
            transfer ([=] (float x) { return clipCurve (x, a, stg); }, level);
            break;
        }
    }
    scan (g, in);
}

void Module::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (1);
    const auto c = BC::mod[mod]; const bool on = power->param->getValue() > 0.5f;
    // ombra e corpo in metallo
    g.setColour (juce::Colours::black.withAlpha (dragging ? 0.7f : 0.5f)); g.fillRoundedRectangle (b.translated (0, dragging ? 8.0f : 4.0f), 12);
    juce::ColourGradient bg (BC::panel1, 0, b.getY(), BC::panel2, 0, b.getBottom(), false); g.setGradientFill (bg); g.fillRoundedRectangle (b, 12);
    juce::Random rr (mod * 17 + 1); for (int y = 0; y < (int) b.getHeight(); y += 2) { g.setColour (juce::Colours::white.withAlpha (rr.nextFloat() * 0.015f)); g.drawHorizontalLine (y, b.getX() + 4, b.getRight() - 4); }
    // bordo luminoso
    if (on) { g.setColour (c.withAlpha (0.12f)); g.drawRoundedRectangle (b.expanded (1.5f), 13, 3.0f); }
    g.setColour (on ? c.withAlpha (0.65f) : juce::Colours::white.withAlpha (0.08f)); g.drawRoundedRectangle (b, 12, 1.2f);
    juce::ColourGradient bev (juce::Colours::white.withAlpha (0.08f), 0, b.getY(), juce::Colours::transparentWhite, 0, b.getY() + 30, false); g.setGradientFill (bev); g.fillRoundedRectangle (b.withHeight (30), 12);
    // striscia LED in testa
    g.setColour (on ? c : c.withAlpha (0.2f)); g.fillRoundedRectangle (b.getX() + 12, b.getY() + 33, b.getWidth() - 24, 2.5f, 1.2f);
    if (on) { g.setColour (c.withAlpha (0.25f)); g.fillRoundedRectangle (b.getX() + 10, b.getY() + 31, b.getWidth() - 20, 6.5f, 3); }
    glow (g, kModNames[mod], juce::Rectangle<float> (32, 6, b.getWidth() - 62, 24), on ? BC::text : BC::dim, bf (11.5f, true), juce::Justification::centredLeft);
    // lucchetto
    const bool lk = ed.proc.locked[mod].load(); auto lr = lockRect().toFloat();
    g.setColour (lk ? BC::amber : BC::dim.withAlpha (0.6f));
    if (lk) g.fillRoundedRectangle (lr.getX() + 3, lr.getY() + 8, 12, 9, 2); else g.drawRoundedRectangle (lr.getX() + 3, lr.getY() + 8, 12, 9, 2, 1.5f);
    juce::Path arc; arc.addCentredArc (lr.getCentreX(), lr.getY() + 8, 4, 5, 0, -juce::MathConstants<float>::halfPi, lk ? juce::MathConstants<float>::halfPi : 0.3f, true);
    g.strokePath (arc, juce::PathStrokeType (1.5f));
    // maniglia per spostare
    if (mod >= 1 && mod <= 4) { g.setColour (BC::dim.withAlpha (0.5f)); for (int k = 0; k < 3; ++k) g.fillRoundedRectangle (b.getRight() - 46, b.getY() + 12 + k * 4.0f, 10, 2, 1); }
    drawDisplay (g, displayRect());
    if (! on) { g.setColour (BC::panel2.withAlpha (0.6f)); g.fillRoundedRectangle (b.withTrimmedTop (38), 12); }
}

void Module::mouseDown (const juce::MouseEvent& e)
{
    if (lockRect().contains (e.getPosition())) { ed.proc.locked[mod] = ! ed.proc.locked[mod].load(); repaint(); return; }
    dragging = mod >= 1 && mod <= 4 && e.position.y < 36; dragX = getX();
    if (dragging) toFront (false);
}
void Module::mouseDrag (const juce::MouseEvent& e) { if (dragging) setTopLeftPosition (dragX + e.getDistanceFromDragStartX(), getY()); }
void Module::mouseUp (const juce::MouseEvent&) { if (dragging) { dragging = false; ed.moduleDropped (*this); } }

// ======================= EQ + analizzatore =======================
static float f2x (float f, juce::Rectangle<float> r) { return r.getX() + r.getWidth() * std::log (f / 20.0f) / std::log (1000.0f); }
static float x2f (float x, juce::Rectangle<float> r) { return 20.0f * std::pow (1000.0f, juce::jlimit (0.0f, 1.0f, (x - r.getX()) / r.getWidth())); }
static const char* kEqF[4] = { "u_e1f", "u_e2f", "u_e3f", "u_e4f" };
static const char* kEqG[4] = { "u_e1g", "u_e2g", "u_e3g", "u_e4g" };
static const juce::Colour kEqC[4] = { juce::Colour (0xffff6a3d), juce::Colour (0xffffd23f), juce::Colour (0xff2de2e6), juce::Colour (0xffa66bff) };

EqPanel::EqPanel (Editor& e) : ed (e)
{
    fftBuf.assign (4096, 0.0f); specIn.assign (1024, -100.0f); specOut.assign (1024, -100.0f);
    on = std::make_unique<Switch> (ed.proc.apvts.getParameter ("u_eqOn"), "EQ", BC::phos); addAndMakeVisible (*on);
    on->setBounds (14, 6, 60, 20);
}
void EqPanel::updateSpectrum()
{
    auto& D = ed.proc.dsp; const int w = D.anW.load();
    for (int pass = 0; pass < 2; ++pass)
    {
        const float* src = pass == 0 ? D.anIn : D.anOut; auto& dst = pass == 0 ? specIn : specOut;
        for (int i = 0; i < 2048; ++i) { const float win = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * i / 2047.0f); fftBuf[(size_t) i] = src[(w - 2048 + i + Burro::AN) % Burro::AN] * win; }
        std::fill (fftBuf.begin() + 2048, fftBuf.end(), 0.0f);
        fft.performFrequencyOnlyForwardTransform (fftBuf.data());
        for (int i = 0; i < 1024; ++i)
        {
            const float db = juce::Decibels::gainToDecibels (fftBuf[(size_t) i] / 512.0f, -100.0f);
            dst[(size_t) i] = db > dst[(size_t) i] ? db : dst[(size_t) i] * 0.88f + db * 0.12f - 0.4f;
        }
    }
}
void EqPanel::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (juce::Colours::black.withAlpha (0.5f)); g.fillRoundedRectangle (b.translated (0, 4), 12);
    juce::ColourGradient bg (BC::panel1, 0, 0, BC::panel2, 0, b.getBottom(), false); g.setGradientFill (bg); g.fillRoundedRectangle (b, 12);
    g.setColour (juce::Colours::white.withAlpha (0.08f)); g.drawRoundedRectangle (b.reduced (0.5f), 12, 1.0f);
    glow (g, "EQ MASTER  +  ANALIZZATORE", juce::Rectangle<float> (84, 6, 300, 20), BC::text, bf (11.0f, true));
    g.setFont (bf (9.5f, false, true)); g.setColour (BC::dim);
    g.drawText ("grigio = entrata   ciano = uscita   trascina i punti, rotella = Q, doppio clic = 0 dB", juce::Rectangle<float> (b.getRight() - 470, 6, 456, 20), juce::Justification::centredRight);
    auto r = plot(); inset (g, r, 8); auto p = r.reduced (4);
    const double sr = ed.proc.getSampleRate() > 0 ? ed.proc.getSampleRate() : 44100.0;
    // griglia
    for (float f : { 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f })
    {
        const float x = f2x (f, p); g.setColour (BC::phos.withAlpha (0.07f)); g.drawVerticalLine ((int) x, p.getY(), p.getBottom());
        g.setColour (BC::phos.withAlpha (0.35f)); g.setFont (bf (8.5f, false, true)); g.drawText (f >= 1000 ? juce::String ((int) (f / 1000)) + "k" : juce::String ((int) f), juce::Rectangle<float> (x + 2, p.getBottom() - 11, 30, 10), juce::Justification::centredLeft);
    }
    for (int db = -12; db <= 12; db += 6) { const float y = p.getCentreY() - db / 15.0f * p.getHeight() * 0.5f; g.setColour (BC::phos.withAlpha (db == 0 ? 0.2f : 0.06f)); g.drawHorizontalLine ((int) y, p.getX(), p.getRight()); }
    // spettri
    auto specPath = [&] (const std::vector<float>& sp, bool close)
    {
        juce::Path path; bool started = false;
        for (int x = 0; x < (int) p.getWidth(); x += 2)
        {
            const float f = x2f (p.getX() + x, p); const int bin = juce::jlimit (1, 1023, (int) (f / sr * 2048));
            const float db = sp[(size_t) bin] + 3.0f * std::log2 (f / 1000.0f + 1e-3f);   // pendenza per leggere meglio
            const float y = p.getBottom() - juce::jlimit (0.0f, 1.0f, (db + 90.0f) / 84.0f) * p.getHeight();
            if (! started) { path.startNewSubPath (p.getX() + x, close ? p.getBottom() : y); if (close) path.lineTo (p.getX() + x, y); started = true; } else path.lineTo (p.getX() + x, y);
        }
        if (close) { path.lineTo (p.getRight(), p.getBottom()); path.closeSubPath(); }
        return path;
    };
    g.setColour (juce::Colours::white.withAlpha (0.10f)); g.fillPath (specPath (specIn, true));
    auto so = specPath (specOut, false);
    g.setColour (BC::phos.withAlpha (0.18f)); g.strokePath (so, juce::PathStrokeType (4.0f)); g.setColour (BC::phos.withAlpha (0.85f)); g.strokePath (so, juce::PathStrokeType (1.3f));
    // curva EQ
    auto v = [this] (const char* id) { return ed.proc.apvts.getRawParameterValue (id)->load(); };
    const bool eqOn = v ("u_eqOn") > 0.5f;
    juce::Path cv, fill;
    for (int x = 0; x <= (int) p.getWidth(); x += 2)
    {
        const float f = x2f (p.getX() + x, p);
        float db = Biq::magDb (0, sr, v ("u_e1f"), v ("u_e1g"), 0.707, f) + Biq::magDb (1, sr, v ("u_e2f"), v ("u_e2g"), v ("u_e2q"), f) + Biq::magDb (1, sr, v ("u_e3f"), v ("u_e3g"), v ("u_e3q"), f) + Biq::magDb (2, sr, v ("u_e4f"), v ("u_e4g"), 0.707, f);
        if (! eqOn) db = 0;
        const float y = p.getCentreY() - juce::jlimit (-15.0f, 15.0f, db) / 15.0f * p.getHeight() * 0.5f;
        if (x == 0) { cv.startNewSubPath (p.getX(), y); fill.startNewSubPath (p.getX(), p.getCentreY()); fill.lineTo (p.getX(), y); } else { cv.lineTo (p.getX() + x, y); fill.lineTo (p.getX() + x, y); }
    }
    fill.lineTo (p.getRight(), p.getCentreY()); fill.closeSubPath();
    g.setColour (BC::magenta.withAlpha (eqOn ? 0.15f : 0.04f)); g.fillPath (fill);
    g.setColour (BC::magenta.withAlpha (0.3f)); g.strokePath (cv, juce::PathStrokeType (5.0f)); g.setColour (eqOn ? BC::magenta : BC::magenta.withAlpha (0.3f)); g.strokePath (cv, juce::PathStrokeType (2.0f));
    for (int k = 0; k < 4; ++k)
    {
        const float x = f2x (v (kEqF[k]), p), y = p.getCentreY() - v (kEqG[k]) / 15.0f * p.getHeight() * 0.5f;
        g.setColour (kEqC[k].withAlpha (0.25f)); g.fillEllipse (x - 11, y - 11, 22, 22);
        g.setColour (BC::lcd2); g.fillEllipse (x - 7, y - 7, 14, 14); g.setColour (kEqC[k]); g.drawEllipse (x - 7, y - 7, 14, 14, 2.0f);
        g.setFont (bf (8.5f, true, true)); g.drawText (juce::String (k + 1), juce::Rectangle<float> (x - 7, y - 7, 14, 14), juce::Justification::centred);
        if (k == dragNode)
        {
            const float f = v (kEqF[k]);
            glow (g, (f >= 1000 ? juce::String (f / 1000, 2) + "k" : juce::String (juce::roundToInt (f))) + "  " + juce::String (v (kEqG[k]), 1) + " dB", juce::Rectangle<float> (x - 60, y - 26, 120, 12), kEqC[k], bf (9.5f, true, true), juce::Justification::centred);
        }
    }
    scan (g, p);
}
int EqPanel::nodeAt (juce::Point<float> pt) const
{
    auto p = plot().reduced (4); auto v = [this] (const char* id) { return ed.proc.apvts.getRawParameterValue (id)->load(); };
    int best = -1; float bd = 18;
    for (int k = 0; k < 4; ++k) { const juce::Point<float> c (f2x (v (kEqF[k]), p), p.getCentreY() - v (kEqG[k]) / 15.0f * p.getHeight() * 0.5f); if (c.getDistanceFrom (pt) < bd) { bd = c.getDistanceFrom (pt); best = k; } }
    return best;
}
void EqPanel::mouseDown (const juce::MouseEvent& e) { dragNode = nodeAt (e.position); if (dragNode >= 0) { auto* on2 = ed.proc.apvts.getParameter ("u_eqOn"); if (on2->getValue() < 0.5f) on2->setValueNotifyingHost (1); } }
void EqPanel::mouseDrag (const juce::MouseEvent& e)
{
    if (dragNode < 0) return;
    auto p = plot().reduced (4); auto* pf = ed.proc.apvts.getParameter (kEqF[dragNode]); auto* pg = ed.proc.apvts.getParameter (kEqG[dragNode]);
    pf->setValueNotifyingHost (pf->convertTo0to1 (pf->getNormalisableRange().snapToLegalValue (x2f (e.position.x, p))));
    pg->setValueNotifyingHost (pg->convertTo0to1 (juce::jlimit (-15.0f, 15.0f, std::round ((p.getCentreY() - e.position.y) / (p.getHeight() * 0.5f) * 15.0f * 2) / 2)));
}
void EqPanel::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    const int n = nodeAt (e.position); if (n != 1 && n != 2) return;
    auto* q = ed.proc.apvts.getParameter (n == 1 ? "u_e2q" : "u_e3q");
    q->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, q->getValue() + (w.deltaY > 0 ? 0.04f : -0.04f)));
}
void EqPanel::mouseDoubleClick (const juce::MouseEvent& e) { const int n = nodeAt (e.position); if (n >= 0) { auto* pg = ed.proc.apvts.getParameter (kEqG[n]); pg->setValueNotifyingHost (pg->convertTo0to1 (0)); } }

// ======================= editor =======================
static juce::Rectangle<int> slotRect (int s) { return { 18 + s * 160, 78, 152, 470 }; }

Editor::Editor (Processor& p) : AudioProcessorEditor (&p), proc (p)
{
    for (int m = 0; m < 6; ++m) addAndMakeVisible (modules.add (new Module (*this, m)));
    eq = std::make_unique<EqPanel> (*this); addAndMakeVisible (*eq);
    auto& A = proc.apvts; const auto mc = BC::amber;
    auto add = [this] (juce::RangedAudioParameter* pr, const char* l, juce::Colour c, bool big, std::function<juce::String (float)> f, bool bp = false) { auto* k = master.add (new PKnob (pr, l, c, big, std::move (f))); k->bipolar = bp; addAndMakeVisible (k); };
    add (A.getParameter ("u_tone"), "TONE", mc, false, bip, true);
    add (A.getParameter ("u_air"), "AIR", mc, false, pct);
    add (A.getParameter ("u_att"), "ATTACK", mc, false, bip, true);
    add (A.getParameter ("u_sus"), "SUSTAIN", mc, false, bip, true);
    add (A.getParameter ("u_mag"), "MAGNITUDE", BC::magenta, true, pct);
    add (A.getParameter ("u_in"), "INPUT", BC::phos, false, [] (float v) { return juce::String (v, 1) + " dB"; }, true);
    add (A.getParameter ("u_out"), "OUTPUT", BC::phos, false, [] (float v) { return juce::String (v, 1) + " dB"; });
    clip = std::make_unique<Switch> (A.getParameter ("u_clip"), "CLIP", BC::magenta); addAndMakeVisible (*clip);
    setSize (W, H);
    startTimerHz (30);
}

void Editor::layoutModules()
{
    modules[0]->setBounds (slotRect (0)); modules[5]->setBounds (slotRect (5));
    for (int s = 0; s < 4; ++s) modules[proc.dsp.order[s].load()]->setBounds (slotRect (s + 1));
}
void Editor::resized()
{
    layoutModules();
    eq->setBounds (18, 560, 952, 244);
    const int mx = 984;
    for (int i = 0; i < 4; ++i) master[i]->setBounds (mx + 18 + (i % 2) * 104, 120 + (i / 2) * 98, 96, 94);
    clip->setBounds (mx + 30, 318, 120, 22);
    master[4]->setBounds (mx + 39, 352, 160, 186);
    master[5]->setBounds (mx + 18, 560, 96, 94); master[6]->setBounds (mx + 122, 560, 96, 94);
    presetRect = { 470, 15, 330, 36 }; prevRect = { 432, 15, 36, 36 }; nextRect = { 802, 15, 36, 36 };
}

void Editor::moduleDropped (Module& m)
{
    const int cx = m.getBounds().getCentreX();
    const int slot = juce::jlimit (1, 4, (int) std::round ((cx - 18 - 76) / 160.0));
    std::vector<int> v; for (auto& o : proc.dsp.order) v.push_back (o.load());
    const auto it = std::find (v.begin(), v.end(), m.mod); if (it != v.end()) v.erase (it);
    v.insert (v.begin() + (slot - 1), m.mod);
    for (int i = 0; i < 4; ++i) proc.dsp.order[i] = v[(size_t) i];
    layoutModules(); repaint();
}

void Editor::paint (juce::Graphics& g)
{
    const float Wf = (float) W, Hf = (float) H;
    juce::ColourGradient bg (juce::Colour (0xff181a20), 0, 0, BC::bg, 0, Hf, false); g.setGradientFill (bg); g.fillAll();
    juce::Random rr (7); for (int y = 0; y < H; y += 2) { g.setColour (juce::Colours::white.withAlpha (rr.nextFloat() * 0.014f)); g.drawHorizontalLine (y, 0, Wf); }
    juce::ColourGradient amb (BC::magenta.withAlpha (0.08f), Wf * 0.85f, 200, juce::Colours::transparentBlack, Wf * 0.85f + 500, 200, true); g.setGradientFill (amb); g.fillRect (0.0f, 0.0f, Wf, Hf);
    juce::ColourGradient amb2 (BC::phos.withAlpha (0.06f), 300, 700, juce::Colours::transparentBlack, 900, 700, true); g.setGradientFill (amb2); g.fillRect (0.0f, 0.0f, Wf, Hf);
    // intestazione
    juce::ColourGradient hg (juce::Colour (0xff23262e), 0, 0, juce::Colour (0xff121419), 0, 64, false); g.setGradientFill (hg); g.fillRect (0.0f, 0.0f, Wf, 64.0f);
    g.setColour (BC::magenta.withAlpha (0.8f)); g.fillRect (0.0f, 62.0f, Wf * 0.45f, 2.0f); g.setColour (BC::phos.withAlpha (0.8f)); g.fillRect (Wf * 0.45f, 62.0f, Wf * 0.55f, 2.0f);
    glow (g, "JACKPOT", { 22, 12, 140, 38 }, BC::text, bf (27.0f, true));
    glow (g, "BUTTER RACK", { 158, 12, 240, 38 }, BC::amber, bf (27.0f, true));
    g.setFont (bf (9.5f, true, true)); g.setColour (BC::dim); g.drawText ("JB-6  DRUM BUS", juce::Rectangle<float> (22, 44, 200, 14), juce::Justification::centredLeft);
    // preset
    inset (g, presetRect.toFloat(), 8);
    glow (g, proc.getProgramName (proc.preset).toUpperCase(), presetRect.toFloat(), BC::amber, bf (12.5f, true, true), juce::Justification::centred);
    g.setColour (BC::phos); g.setFont (bf (20.0f, true));
    g.drawText (juce::String::fromUTF8 ("\xe2\x80\xb9"), prevRect, juce::Justification::centred); g.drawText (juce::String::fromUTF8 ("\xe2\x80\xba"), nextRect, juce::Justification::centred);
    // griglia di aerazione
    for (int i = 0; i < 12; ++i) { auto r = juce::Rectangle<float> (Wf - 250 + i * 11.0f, 18, 5, 26); g.setColour (juce::Colours::black.withAlpha (0.8f)); g.fillRoundedRectangle (r, 2.5f); }
    // frecce della catena
    for (int s = 0; s < 5; ++s) { const float x = (float) slotRect (s).getRight() + 4; juce::Path ar; ar.addArrow ({ x - 2, 96.0f, x + 6, 96.0f }, 1.5f, 6, 6); g.setColour (BC::phos.withAlpha (0.6f)); g.fillPath (ar); }
    // master
    auto mr = juce::Rectangle<float> (984, 78, 238, 726);
    g.setColour (juce::Colours::black.withAlpha (0.5f)); g.fillRoundedRectangle (mr.translated (0, 4), 12);
    juce::ColourGradient mg (BC::panel1, 0, mr.getY(), BC::panel2, 0, mr.getBottom(), false); g.setGradientFill (mg); g.fillRoundedRectangle (mr, 12);
    g.setColour (BC::magenta.withAlpha (0.5f)); g.drawRoundedRectangle (mr, 12, 1.2f);
    glow (g, "MASTER", mr.withHeight (34), BC::text, bf (12.0f, true), juce::Justification::centred);
    g.setColour (BC::magenta); g.fillRoundedRectangle (mr.getX() + 14, mr.getY() + 33, mr.getWidth() - 28, 2.5f, 1.2f);
    // meter verticali in/out
    auto meter = [&] (juce::Rectangle<float> r, float l, float rr2, const char* t)
    {
        g.setColour (BC::dim); g.setFont (bf (8.5f, true, true)); g.drawText (t, r.removeFromBottom (12), juce::Justification::centred);
        for (int c = 0; c < 2; ++c)
        {
            auto b = r.removeFromLeft (r.getWidth() / (2 - c)).reduced (1.5f, 0);
            const float db = juce::Decibels::gainToDecibels (c ? rr2 : l, -48.0f), n = juce::jlimit (0.0f, 1.0f, (db + 48) / 51);
            for (int k = 0; k < 24; ++k) { const float t2 = k / 24.0f; g.setColour (t2 <= n ? (t2 > 0.9f ? BC::magenta : t2 > 0.75f ? BC::amber : BC::phos) : juce::Colours::white.withAlpha (0.05f)); g.fillRect (b.getX(), b.getBottom() - (k + 1) * b.getHeight() / 24 + 1, b.getWidth(), b.getHeight() / 24 - 1.5f); }
        }
    };
    meter ({ 1002, 668, 40, 128 }, proc.inL.load(), proc.inR.load(), "IN");
    meter ({ 1160, 668, 40, 128 }, proc.outL.load(), proc.outR.load(), "OUT");
    g.setColour (BC::dim); g.setFont (bf (9.0f));
    g.drawFittedText ("lucchetto = blocca il modulo nei preset\ntrascina la testata per riordinare", juce::Rectangle<int> (1050, 680, 104, 90), juce::Justification::centred, 6);
}

void Editor::mouseDown (const juce::MouseEvent& e)
{
    const int n = proc.getNumPrograms();
    if (prevRect.contains (e.getPosition())) proc.loadPreset ((proc.preset - 1 + n) % n);
    else if (nextRect.contains (e.getPosition())) proc.loadPreset ((proc.preset + 1) % n);
    else if (presetRect.contains (e.getPosition()))
    {
        juce::PopupMenu m; std::vector<std::pair<juce::String, juce::PopupMenu>> groups;
        for (int i = 0; i < n; ++i)
        {
            const auto name = proc.getProgramName (i); const auto gname = name.upToFirstOccurrenceOf (" | ", false, false);
            if (! name.contains (" | ")) { m.addItem (i + 1, name, true, i == proc.preset); continue; }
            bool found = false; for (auto& gr : groups) if (gr.first == gname) { gr.second.addItem (i + 1, name.fromFirstOccurrenceOf (" | ", false, false), true, i == proc.preset); found = true; }
            if (! found) { juce::PopupMenu pm; pm.addItem (i + 1, name.fromFirstOccurrenceOf (" | ", false, false), true, i == proc.preset); groups.push_back ({ gname, pm }); }
        }
        for (auto& gr : groups) m.addSubMenu (gr.first, gr.second);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea (localAreaToGlobal (presetRect)), [this] (int r) { if (r > 0) { proc.loadPreset (r - 1); repaint(); } });
    }
    repaint();
}

void Editor::timerCallback()
{
    eq->updateSpectrum(); eq->repaint();
    for (auto* m : modules) m->repaint();
    for (auto* k : master) k->repaint();
    clip->repaint();
    repaint (984, 660, 238, 140); repaint (presetRect);
}
}
