#include "ButterEditor.h"

namespace jb
{
juce::Font bf (float size, bool bold, bool mono)
{
    return mono ? jlook::mono (size) : jlook::ui (size, bold ? 2 : 1);
}
static const char* kModNames[7] = { "BOOM / NOISE", "SHIFT", "COMPRESS", "SATURATE", "SPACE", "CLIP", "EXTRA" };
static const char* kDlyNames[7] = { "1/16", "1/8T", "1/8", "3/16", "1/4", "3/8", "1/2" };
static juce::String pct (float v) { return juce::String (juce::roundToInt (v * 100)) + "%"; }
static juce::String bip (float v) { return std::abs (v) < 0.01f ? juce::String ("0") : (v > 0 ? "+" : "") + juce::String (juce::roundToInt (v * 100)); }

static void glow (juce::Graphics& g, const juce::String& t, juce::Rectangle<float> r, juce::Colour c, juce::Font f, juce::Justification j = juce::Justification::centredLeft)
{
    g.setFont (f); g.setColour (c); g.drawFittedText (t, r.toNearestInt(), j, 1, 0.8f);
}
static void inset (juce::Graphics& g, juce::Rectangle<float> r, float rad)
{
    auto p = neo::chamfer (r, juce::jmin (7.0f, rad + 1.0f), 2.0f);
    g.setColour (BC::lcd2); g.fillPath (p);
    g.setColour (BC::phos.withAlpha (0.12f)); g.strokePath (p, juce::PathStrokeType (1.0f));
}
static void scan (juce::Graphics& g, juce::Rectangle<float> r)
{
    g.setColour (juce::Colours::black.withAlpha (0.18f)); for (float y = r.getY(); y < r.getBottom(); y += 3) g.fillRect (r.getX(), y, r.getWidth(), 1.0f);
}

// ======================= manopola 3D con anello LED =======================
void PKnob::paint (juce::Graphics& g)
{
    const float lh = isBig ? 30.0f : 27.0f;
    const float size = juce::jmin ((float) getWidth(), getHeight() - lh);
    const float v = param->getValue();
    actSm += ((activity ? juce::jlimit (0.0f, 1.0f, activity()) : 0.0f) - actSm) * 0.35f;
    neo::setTheme (3);
    // manopola CYBER: anello di LED a segmenti (si accende col segnale), disco nero, lancetta colorata
    const float cx = getWidth() * 0.5f, cy = size * 0.5f, R = size * 0.5f - 3.0f, kr = R * (isBig ? 0.72f : 0.7f), av = -2.356f + 4.712f * v;
    if (actSm > 0.02f) { g.setColour (col.withAlpha (0.12f * actSm)); g.fillEllipse (cx - R - 3, cy - R - 3, R * 2 + 6, R * 2 + 6); }
    neo::ledRing (g, cx, cy, R, v, bipolar, hover || active ? col.brighter (0.2f) : col, isBig ? 3.4f : 2.4f, isBig ? 33 : 23);
    g.setColour (juce::Colours::black.withAlpha (0.45f)); g.fillEllipse (cx - kr, cy - kr + 2, kr * 2, kr * 2);
    juce::ColourGradient bgc (juce::Colour (0xff2a2e3a), cx, cy - kr, juce::Colour (0xff111319), cx, cy + kr, false); g.setGradientFill (bgc); g.fillEllipse (cx - kr, cy - kr, kr * 2, kr * 2);
    g.setColour (juce::Colours::white.withAlpha (0.08f)); g.drawEllipse (cx - kr + 0.5f, cy - kr + 0.5f, kr * 2 - 1, kr * 2 - 1, 1.0f);
    { const float r0 = kr * 0.3f, r1 = kr * 0.88f, sn = std::sin (av), cs = std::cos (av);
      juce::Path pl; pl.startNewSubPath (cx + sn * r0, cy - cs * r0); pl.lineTo (cx + sn * r1, cy - cs * r1);
      g.setColour (col); g.strokePath (pl, juce::PathStrokeType (isBig ? 3.0f : 2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded)); }
    if (dynLabel) label = dynLabel();
    g.setColour (hover ? BC::text : BC::dim); g.setFont (neo::cap (isBig ? 9.5f : 8.0f)); g.drawFittedText (label, juce::Rectangle<int> (0, (int) size + 2, getWidth(), 12), juce::Justification::centred, 1, 0.7f);
    g.setFont (neo::num (isBig ? 11.0f : 9.5f)); g.setColour (active ? col : BC::text.withAlpha (0.9f));
    g.drawFittedText (fmt ? fmt (param->convertFrom0to1 (v)) : param->getCurrentValueAsText(), juce::Rectangle<int> (0, (int) size + 14, getWidth(), 13), juce::Justification::centred, 1, 0.7f);
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
        if (i == cur) { g.setColour (col.withAlpha (0.25f)); g.fillRect (r); g.setColour (col); g.fillRect (r.getX(), r.getBottom() - 2, r.getWidth(), 2.0f); }
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
    g.setColour (juce::Colours::black.withAlpha (0.7f)); g.fillRect (led.expanded (1.5f));
    if (on) { g.setColour (col.withAlpha (0.3f)); g.fillRect (led.expanded (3)); }
    g.setColour (on ? col : juce::Colour (0xff30333b)); g.fillRect (led);
    if (label.isNotEmpty()) { g.setColour (on ? BC::text : BC::dim); g.setFont (bf (10.0f, true)); g.drawText (label, b.withTrimmedLeft (5), juce::Justification::centredLeft); }
}

// ======================= curve per i display =======================
static juce::String hzFmt (float v) { return v < 21 ? juce::String ("OFF") : v > 19900 ? juce::String ("OFF") : v >= 1000 ? juce::String (v / 1000, 1) + "k" : juce::String (juce::roundToInt (v)); }
static float satCurve (int type, float x, float a, float st)
{
    const float ae = a * a, d = 1.0f + ae * 12.0f, X = x * d; float y;
    switch (type)
    {
        case 1: y = std::tanh (X * 0.8f) + 0.08f * st * X * X / (1 + X * X); break;
        case 2: y = std::atan (X * (1 + st)) / std::atan (1.0f + st) * 0.8f; break;
        case 3: { const float k = 0.55f + 0.45f * (1 - st); y = X > 0 ? std::tanh (X) : std::tanh (X * k) / k * 0.8f; y += 0.12f * st * y * y; break; }
        case 4: { const float q = std::pow (2.0f, 4.0f - st * 2.5f); y = std::round (juce::jlimit (-1.0f, 1.0f, X) * q) / q; break; }
        case 5: y = std::sin (X * (1.0f + st * 2.0f)) * 0.9f; break;
        case 6: { const float xx = X * 1.4f; y = xx > 0 ? std::tanh (xx) : std::tanh (xx * 0.75f) / 0.75f * 0.9f; break; }
        case 7: { const float xx = X * (2.0f + st * 4.0f); y = juce::jlimit (-1.0f, 1.0f, xx); y = (y - y * y * y / 3.0f) * 1.3f; break; }
        case 8: { const float xx = X * (3.0f + st * 6.0f); y = std::copysign (1.0f - std::exp (-std::abs (xx)), xx) * 0.85f; break; }
        default: y = std::tanh (X + st * 0.3f * X * X / (1 + std::abs (X))); break;
    }
    const float w = juce::jmin (1.0f, a * 3.0f);
    return x + (y / (1.0f + ae * 1.2f) * (1.0f + ae * 0.6f) - x) * w;
}
static float clipCurve (float x, float a, int stages)
{
    const float ae = a * a; float y = x * (1.0f + ae * 10.0f);
    for (int s = 0; s < stages; ++s) y = (s % 2 == 0) ? std::tanh (y * 1.1f) : juce::jlimit (-0.9f, 0.9f, y * 1.25f);
    return x + (y / (1.0f + ae * 0.8f) * (1.0f + ae * 0.5f) - x) * juce::jmin (1.0f, a * 3.0f);
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
            addSmall (P ("u_bDecay"), "DECAY", [] (float v) { return juce::String (juce::roundToInt (40 + v * 450)) + " ms"; });
            addSmall (P ("u_bPunch"), "PUNCH", pct);
            extra = std::make_unique<Switch> (P ("u_acid"), "ACID", c);
            extraPill = std::make_unique<Pill> (C ("u_nType"), c);
            break;
        case 1:
            power = std::make_unique<Switch> (P ("u_shOn"), "", c);
            big = std::make_unique<PKnob> (P ("u_shAmt"), "MIX", c, true, pct);
            addSmall (P ("u_shPitch"), "PITCH", [] (float v) { return (v > 0 ? "+" : "") + juce::String (v, 1) + " st"; }, true);
            addSmall (P ("u_shForm"), "FORMANTE", [] (float v) { return std::abs (v) < 0.05f ? juce::String ("SEGUE") : (v > 0 ? "+" : "") + juce::String (v, 1) + " st"; }, true);
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
            addSmall (P ("u_spLo"), "LO CUT", hzFmt); addSmall (P ("u_spHi"), "HI CUT", hzFmt);
            break;
        case 6:
        {
            power = std::make_unique<Switch> (P ("u_xOn"), "", c);
            type = std::make_unique<Pill> (C ("u_xType"), c);
            big = std::make_unique<PKnob> (P ("u_xAmt"), "AMOUNT", c, true, pct);
            auto* tp = P ("u_xType");
            addSmall (P ("u_xP1"), "TIME", [tp] (float v) { const int t = juce::roundToInt (tp->convertFrom0to1 (tp->getValue()));
                if (t == 0) return juce::String (kDlyNames[juce::jlimit (0, 6, (int) (v * 6.99f))]);
                if (t == 1) return juce::String (juce::roundToInt (2.0f + (1.0f - v) * 13.0f)) + " bit";
                return juce::String (0.1f + v * v * 8.0f, 2) + " Hz"; });
            addSmall (P ("u_xP2"), "FEEDBACK", pct);
            addSmall (P ("u_xLo"), "LO CUT", hzFmt); addSmall (P ("u_xHi"), "HI CUT", hzFmt);
            target = std::make_unique<Target> (P ("u_xTgt"), c);
            break;
        }
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
    // la manopola principale "respira" con il lavoro che il modulo sta facendo
    big->activity = [this]
    {
        auto& D = ed.proc.dsp; if (power->param->getValue() < 0.5f || big->param->getValue() < 0.005f) return 0.0f;
        const float lev = juce::jmin (1.0f, D.scOut.at (Scope::N - 1) * 2.5f);
        return mod == 2 ? juce::jmin (1.0f, D.grMeter.load() / 10.0f) : lev * (0.35f + 0.65f * big->param->getValue());
    };
}

void Module::resized()
{
    const int w = getWidth();
    power->setBounds (8, 8, 20, 20);
    if (type) type->setBounds (10, 128, w - 20, 22);
    big->setBounds (14, type ? 154 : 136, w - 28, 128);
    const int sw = (w - 12) / 2, y0 = type ? 288 : 272;
    if (mod == 4 || mod == 6)
    {   // tre colonne per far stare i filtri
        const int s3 = (w - 8) / 3; const int order6[] = { 0, 1, 2, 3 };
        for (int i = 0; i < smalls.size(); ++i) { const int k = mod == 6 ? order6[i] : i; smalls[i]->setBounds (4 + (k % 3) * s3, y0 + (k / 3) * 70, s3, 68); }
    }
    else for (int i = 0; i < smalls.size(); ++i) smalls[i]->setBounds (6 + (i % 2) * sw, y0 + (i / 2) * 78, sw, 76);
    if (mod == 0) { extra->setBounds (12, 432, sw - 4, 22); extraPill->setBounds (6 + sw + 2, 432, sw - 6, 22); }
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
        case 6:
        {
            const int t = (int) v ("u_xType"); const float amt = v ("u_xAmt"), p1 = v ("u_xP1"), p2 = v ("u_xP2");
            if (t == 0)
            {
                // eco: impulsi che decadono col feedback, distanziati a tempo
                static const float bt[] = { 0.25f, 1.0f / 3.0f, 0.5f, 0.75f, 1.0f, 1.5f, 2.0f };
                const float step = bt[juce::jlimit (0, 6, (int) (p1 * 6.99f))] / 4.0f;
                g.setColour (BC::phos.withAlpha (0.25f)); g.strokePath (envPath (D.scOut, 1.2f), juce::PathStrokeType (1.0f));
                float a2 = 1.0f; int k = 0;
                for (float x = 0; x <= 1.0f && k < 24; x += step, ++k)
                {
                    const float h = (k == 0 ? 1.0f : amt * a2) * in.getHeight() * 0.85f; const float xx = in.getX() + x * in.getWidth();
                    g.setColour ((k == 0 ? BC::text : c).withAlpha (k == 0 ? 0.6f : 0.95f)); g.fillRect (xx, in.getBottom() - h, 3.0f, h);
                    g.setColour (c.withAlpha (0.25f)); g.fillRect (xx - 2, in.getBottom() - h, 7.0f, h);
                    if (k > 0) a2 *= p2 * 0.9f + 0.05f;
                }
                glow (g, kDlyNames[juce::jlimit (0, 6, (int) (p1 * 6.99f))], in.withHeight (12), c, bf (9.5f, true, true), juce::Justification::centredRight);
            }
            else if (t == 1)
            {
                const float q = std::pow (2.0f, 2.0f + (1.0f - p1) * 13.0f); const int hold = 1 + (int) (p2 * 18);
                juce::Path p;
                for (int i = 0; i <= 120; ++i)
                {
                    const int hi = (i / hold) * hold; const float sx = std::sin (hi / 120.0f * 12.566f) * 0.8f;
                    const float y0 = std::sin (i / 120.0f * 12.566f) * 0.8f, yq = std::round (sx * q) / q, y = y0 + (yq - y0) * amt;
                    const float x = in.getX() + in.getWidth() * i / 120.0f, yy = in.getCentreY() - y * in.getHeight() * 0.45f;
                    if (i == 0) p.startNewSubPath (x, yy); else p.lineTo (x, yy);
                }
                g.setColour (c.withAlpha (0.3f)); g.strokePath (p, juce::PathStrokeType (4.0f)); g.setColour (c); g.strokePath (p, juce::PathStrokeType (1.4f));
                glow (g, juce::String (juce::roundToInt (2.0f + (1.0f - p1) * 13.0f)) + " BIT", in.withHeight (12), c, bf (9.5f, true, true), juce::Justification::centredRight);
            }
            else
            {
                const float ph = (float) (juce::Time::getMillisecondCounterHiRes() * 0.001 * (0.1 + p1 * p1 * 8.0));
                juce::Path p;
                for (int i = 0; i <= 80; ++i)
                {
                    const float t2 = i / 80.0f, y = std::sin ((t2 * 2.0f + ph) * juce::MathConstants<float>::twoPi) * (0.15f + p2 * 0.7f) * (0.3f + amt * 0.7f);
                    const float x = in.getX() + in.getWidth() * t2, yy = in.getCentreY() - y * in.getHeight() * 0.45f;
                    if (i == 0) p.startNewSubPath (x, yy); else p.lineTo (x, yy);
                }
                g.setColour (BC::phos.withAlpha (0.25f)); g.strokePath (envPath (D.scOut, 1.2f), juce::PathStrokeType (1.0f));
                g.setColour (c.withAlpha (0.3f)); g.strokePath (p, juce::PathStrokeType (4.0f)); g.setColour (c); g.strokePath (p, juce::PathStrokeType (1.6f));
                glow (g, "WOBBLE", in.withHeight (12), c, bf (9.5f, true, true), juce::Justification::centredRight);
            }
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
    if (mod == 6)
    {
        const int t = (int) ed.proc.apvts.getRawParameterValue ("u_xType")->load();
        smalls[0]->label = t == 0 ? "TIME" : t == 1 ? "BITS" : "RATE";
        smalls[1]->label = t == 0 ? "FEEDBACK" : t == 1 ? "RATE" : "DEPTH";
    }
    // scheda: ombra morbida, bordo che sfuma dall'accento
    // scheda in cache (prima: grana e gradienti rifatti 30 volte al secondo per ogni modulo)
    base.draw (g, getLocalBounds().toFloat(), (on ? 1 : 0) + (dragging ? 2 : 0), [&] (juce::Graphics& bg)
    {
        neo::setTheme (3); neo::plate (bg, b, on ? c : BC::faint, 12.0f);
        bg.setColour (c.withAlpha (on ? 0.85f : 0.25f)); bg.fillRect (b.getX() + 10, b.getY() + 35, b.getWidth() - 20, 1.0f);
        if (dragging) neo::brackets (bg, b.reduced (2), c, 12.0f, 1.6f);
    });
    // barra di segnale nella testata
    { const float act = big ? big->actSm : 0.0f; if (on) { g.setColour (c.withAlpha (0.18f + 0.5f * act)); g.fillRect (b.getX() + 10, b.getY() + 33, (b.getWidth() - 20) * (0.15f + 0.85f * act), 2.0f); } }
    glow (g, kModNames[mod], juce::Rectangle<float> (32, 6, b.getWidth() - 62, 24), on ? BC::text : BC::dim, bf (11.5f, true), juce::Justification::centredLeft);
    // lucchetto
    const bool lk = ed.proc.locked[mod].load(); auto lr = lockRect().toFloat();
    g.setColour (lk ? BC::amber : BC::dim.withAlpha (0.6f));
    if (lk) g.fillRoundedRectangle (lr.getX() + 3, lr.getY() + 8, 12, 9, 2); else g.drawRoundedRectangle (lr.getX() + 3, lr.getY() + 8, 12, 9, 2, 1.5f);
    juce::Path arc; arc.addCentredArc (lr.getCentreX(), lr.getY() + 8, 4, 5, 0, -juce::MathConstants<float>::halfPi, lk ? juce::MathConstants<float>::halfPi : 0.3f, true);
    g.strokePath (arc, juce::PathStrokeType (1.5f));
    // maniglia per spostare
    if ((mod >= 1 && mod <= 4) || mod == 6) { g.setColour (BC::dim.withAlpha (0.5f)); for (int k = 0; k < 3; ++k) g.fillRoundedRectangle (b.getRight() - 46, b.getY() + 12 + k * 4.0f, 10, 2, 1); }
    drawDisplay (g, displayRect());
    if (! on) { g.setColour (BC::panel2.withAlpha (0.55f)); g.fillRect (b.withTrimmedTop (38).reduced (2, 0).withTrimmedBottom (14)); }
}

void Module::mouseDown (const juce::MouseEvent& e)
{
    if (lockRect().contains (e.getPosition())) { ed.proc.locked[mod] = ! ed.proc.locked[mod].load(); repaint(); return; }
    dragging = ((mod >= 1 && mod <= 4) || mod == 6) && e.position.y < 36; dragX = getX();
    if (dragging) toFront (false);
}
void Module::mouseDrag (const juce::MouseEvent& e) { if (dragging) { setTopLeftPosition (dragX + e.getDistanceFromDragStartX(), getY()); ed.repaint(); } }
void Module::mouseUp (const juce::MouseEvent&) { if (dragging) { dragging = false; ed.moduleDropped (*this); } }

// ======================= EQ analogico + analizzatore =======================
static float f2x (float f, juce::Rectangle<float> r) { return r.getX() + r.getWidth() * std::log (f / 20.0f) / std::log (1000.0f); }
static float x2f (float x, juce::Rectangle<float> r) { return 20.0f * std::pow (1000.0f, juce::jlimit (0.0f, 1.0f, (x - r.getX()) / r.getWidth())); }
static const juce::Colour kEqC[4] = { juce::Colour (0xffff6a3d), juce::Colour (0xffffd23f), juce::Colour (0xff2de2e6), juce::Colour (0xffa66bff) };
static juce::String hzTxt (float f) { return f <= 1 ? juce::String ("OFF") : f >= 1000 ? juce::String (f / 1000, f >= 10000 ? 0 : 1) + "k" : juce::String (juce::roundToInt (f)); }
static AeSettings aeNow (juce::AudioProcessorValueTreeState& A)
{
    auto v = [&A] (const juce::String& id) { return A.getRawParameterValue (id)->load(); };
    AeSettings S; S.model = (int) v ("u_aeModel"); S.q = v ("u_aeQ"); S.drive = v ("u_aeDrive");
    for (int k = 0; k < 4; ++k) { S.g[k] = v ("u_ae" + juce::String (k + 1) + "g"); S.f[k] = v ("u_ae" + juce::String (k + 1) + "f"); }
    return S;
}

EqPanel::EqPanel (Editor& e) : ed (e)
{
    fftBuf.assign (4096, 0.0f); specIn.assign (1024, -100.0f); specOut.assign (1024, -100.0f);
    auto& A = ed.proc.apvts;
    on = std::make_unique<Switch> (A.getParameter ("u_eqOn"), "EQ", BC::phos); addAndMakeVisible (*on);
    for (int k = 0; k < 4; ++k)
    {
        const juce::String id = "u_ae" + juce::String (k + 1);
        auto* g = knobs.add (new PKnob (A.getParameter (id + "g"), "", kEqC[k], false, {})); g->bipolar = true;
        g->dynLabel = [this, k] { return juce::String (aeInfo ((int) ed.proc.apvts.getRawParameterValue ("u_aeModel")->load()).b[k].name); };
        g->fmt = [this, k] (float v) { const auto& B = aeInfo ((int) ed.proc.apvts.getRawParameterValue ("u_aeModel")->load()).b[k];
            if (B.type == Bq::HP) return juce::String ("-"); const float gg = B.boostOnly ? juce::jmax (0.0f, v) : B.cutOnly ? juce::jmin (0.0f, v) : v; return (gg > 0 ? "+" : "") + juce::String (gg, 1); };
        auto* f = knobs.add (new PKnob (A.getParameter (id + "f"), "FREQ", kEqC[k], false, {}));
        f->fmt = [this, k] (float v) { const int m = (int) ed.proc.apvts.getRawParameterValue ("u_aeModel")->load(); const auto& I = aeInfo (m);
            if (m == AE_PE1C && k == 1) return juce::String ("= LF"); const auto& B = I.b[k]; return hzTxt (B.freqs[(size_t) aeIndex (B, v)]); };
    }
    auto* dr = knobs.add (new PKnob (A.getParameter ("u_aeDrive"), "DRIVE", BC::amber, false, pct));
    juce::ignoreUnused (dr);
    auto* q = knobs.add (new PKnob (A.getParameter ("u_aeQ"), "Q", BC::amber, false, {}));
    q->dynLabel = [this] { const int m = (int) ed.proc.apvts.getRawParameterValue ("u_aeModel")->load(); return juce::String (m == AE_SSL || m == AE_LIND81 ? "Q MEDI" : m == AE_NEVE84 ? "HI-Q" : m == AE_PE1C ? "BANDA" : "-"); };
    q->fmt = [this] (float v) { const int m = (int) ed.proc.apvts.getRawParameterValue ("u_aeModel")->load();
        if (m == AE_NEVE84) return juce::String (v > 0.5f ? "ON" : "OFF"); if (m == AE_PE1C) return juce::String (v < 0.33f ? "LARGA" : v < 0.66f ? "MEDIA" : "STRETTA");
        if (m == AE_SSL) return juce::String (0.5f + v * 2.5f, 1); if (m == AE_LIND81) return juce::String (0.6f + v * 1.6f, 1); return juce::String ("-"); };
    for (auto* k : knobs) addAndMakeVisible (k);
}
void EqPanel::resized()
{
    on->setBounds (14, 6, 60, 20);
    const int y = 126, kw = 52, x0 = 14;
    for (int k = 0; k < 4; ++k) { knobs[k * 2]->setBounds (x0 + k * 112, y, kw, 80); knobs[k * 2 + 1]->setBounds (x0 + k * 112 + kw + 2, y, kw, 80); }
    knobs[8]->setBounds (x0 + 4 * 112 + 4, y, kw, 80); knobs[9]->setBounds (x0 + 4 * 112 + 4 + kw + 4, y, kw, 80);
}
void EqPanel::mouseDown (const juce::MouseEvent& e)
{
    if (! modelRect().contains (e.getPosition())) return;
    auto* pr = dynamic_cast<juce::AudioParameterChoice*> (ed.proc.apvts.getParameter ("u_aeModel"));
    juce::PopupMenu m; for (int i = 0; i < kAeModels; ++i) m.addItem (i + 1, juce::String (aeInfo (i).name) + "   -   " + aeInfo (i).desc, true, i == pr->getIndex());
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [this, pr] (int r) { if (r > 0) { pr->setValueNotifyingHost (pr->convertTo0to1 ((float) (r - 1))); repaint(); } });
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
    base.draw (g, b, 1, [&] (juce::Graphics& bg) { neo::setTheme (3); neo::plate (bg, b.reduced (1), BC::phos, 12.0f); });
    const auto S = aeNow (ed.proc.apvts); const auto& I = aeInfo (S.model);
    glow (g, "EQ ANALOGICO", juce::Rectangle<float> (84, 6, 140, 20), BC::text, bf (11.0f, true));
    g.setFont (bf (9.0f, false, true)); g.setColour (BC::dim);
    g.drawFittedText (I.desc, juce::Rectangle<int> (190, 6, getWidth() - 390, 20), juce::Justification::centredLeft, 1, 0.8f);
    { auto mr = modelRect().toFloat(); neo::setTheme (3); neo::key (g, mr, true, BC::amber); g.setColour (BC::text); g.setFont (neo::cap (9.5f)); g.drawFittedText (juce::String (I.name) + juce::String::fromUTF8 ("  \xe2\x96\xbe"), mr.toNearestInt().reduced (10, 0), juce::Justification::centred, 1, 0.7f); }
    auto r = plot(); inset (g, r, 8); auto p = r.reduced (4);
    const double sr = ed.proc.getSampleRate() > 0 ? ed.proc.getSampleRate() : 44100.0;
    for (float f : { 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f })
    {
        const float x = f2x (f, p); g.setColour (BC::phos.withAlpha (0.07f)); g.drawVerticalLine ((int) x, p.getY(), p.getBottom());
        g.setColour (BC::phos.withAlpha (0.35f)); g.setFont (bf (8.0f, false, true)); g.drawText (f >= 1000 ? juce::String ((int) (f / 1000)) + "k" : juce::String ((int) f), juce::Rectangle<float> (x + 2, p.getBottom() - 10, 30, 10), juce::Justification::centredLeft);
    }
    g.setColour (BC::phos.withAlpha (0.2f)); g.drawHorizontalLine ((int) p.getCentreY(), p.getX(), p.getRight());
    auto specPath = [&] (const std::vector<float>& sp, bool close)
    {
        juce::Path path; bool started = false;
        for (int x = 0; x < (int) p.getWidth(); x += 3)
        {
            const float f = x2f (p.getX() + x, p); const int bin = juce::jlimit (1, 1023, (int) (f / sr * 2048));
            const float db = sp[(size_t) bin] + 3.0f * std::log2 (f / 1000.0f + 1e-3f);
            const float y = p.getBottom() - juce::jlimit (0.0f, 1.0f, (db + 90.0f) / 84.0f) * p.getHeight();
            if (! started) { path.startNewSubPath (p.getX() + x, close ? p.getBottom() : y); if (close) path.lineTo (p.getX() + x, y); started = true; } else path.lineTo (p.getX() + x, y);
        }
        if (close) { path.lineTo (p.getRight(), p.getBottom()); path.closeSubPath(); }
        return path;
    };
    g.setColour (juce::Colours::white.withAlpha (0.08f)); g.fillPath (specPath (specIn, true));
    g.setColour (BC::phos.withAlpha (0.6f)); g.strokePath (specPath (specOut, false), juce::PathStrokeType (1.1f));
    // curva del modello (PE 1C: compreso il taglio dei bassi)
    const bool eqOn = ed.proc.apvts.getRawParameterValue ("u_eqOn")->load() > 0.5f;
    Bq f[4]; bool act[4]; for (int k = 0; k < 4; ++k) aeBandFilter (S, k, sr, f[k], act[k]);
    juce::Path cv, fill;
    for (int x = 0; x <= (int) p.getWidth(); x += 2)
    {
        const float hz = x2f (p.getX() + x, p); double db = 0;
        if (eqOn) for (int k = 0; k < 4; ++k) if (act[k]) db += f[k].magDb (hz, sr);
        const float y = p.getCentreY() - juce::jlimit (-18.0f, 18.0f, (float) db) / 18.0f * p.getHeight() * 0.5f;
        if (x == 0) { cv.startNewSubPath (p.getX(), y); fill.startNewSubPath (p.getX(), p.getCentreY()); fill.lineTo (p.getX(), y); } else { cv.lineTo (p.getX() + x, y); fill.lineTo (p.getX() + x, y); }
    }
    fill.lineTo (p.getRight(), p.getCentreY()); fill.closeSubPath();
    g.setColour (BC::magenta.withAlpha (eqOn ? 0.15f : 0.04f)); g.fillPath (fill);
    g.setColour (BC::magenta.withAlpha (0.3f)); g.strokePath (cv, juce::PathStrokeType (5.0f)); g.setColour (eqOn ? BC::magenta : BC::magenta.withAlpha (0.3f)); g.strokePath (cv, juce::PathStrokeType (2.0f));
    for (int k = 0; k < 4; ++k)
    {
        const auto& B = I.b[k]; int idx = aeIndex (B, S.f[k]); if (S.model == AE_PE1C && k == 1) idx = aeIndex (I.b[0], S.f[0]);
        const float fr = B.freqs[(size_t) idx]; if (fr <= 1) continue;
        const float x = f2x (juce::jlimit (20.0f, 20000.0f, fr), p);
        g.setColour (kEqC[k].withAlpha (act[k] ? 0.9f : 0.35f)); g.fillEllipse (x - 3.5f, p.getY() + 3, 7, 7);
    }
    scan (g, p);
}

// ======================= Pultec =======================
PultecPanel::PultecPanel (Editor& e) : ed (e)
{
    auto& A = ed.proc.apvts; const auto c = BC::amber;
    on = std::make_unique<Switch> (A.getParameter ("u_pOn"), "", c); addAndMakeVisible (*on);
    auto ten = [] (float v) { return juce::String (v, 1); };
    struct K { const char* id; const char* l; std::function<juce::String (float)> f; };
    const K ks[8] = { { "u_pLF", "LF FREQ", [] (float v) { static const char* n[] = { "20", "30", "60", "100" }; return juce::String (n[juce::jlimit (0, 3, juce::roundToInt (v))]) + " Hz"; } },
                      { "u_pLB", "BOOST", ten }, { "u_pLA", "ATTEN", ten },
                      { "u_pAS", "ATT SEL", [] (float v) { static const char* n[] = { "5k", "10k", "20k" }; return juce::String (n[juce::jlimit (0, 2, juce::roundToInt (v))]); } },
                      { "u_pHF", "HF FREQ", [] (float v) { static const char* n[] = { "3k", "4k", "5k", "8k", "10k", "12k", "16k" }; return juce::String (n[juce::jlimit (0, 6, juce::roundToInt (v))]); } },
                      { "u_pHB", "BOOST", ten }, { "u_pBW", "BANDA", [] (float v) { return v < 3.3f ? juce::String ("STRETTA") : v < 6.6f ? juce::String ("MEDIA") : juce::String ("LARGA"); } }, { "u_pHA", "ATTEN", ten } };
    for (auto& k : ks) { auto* pk = knobs.add (new PKnob (A.getParameter (k.id), k.l, c, false, k.f)); addAndMakeVisible (pk); }
}
void PultecPanel::resized()
{
    on->setBounds (12, 8, 20, 20);
    const int w = (getWidth() - 12) / 4;
    for (int i = 0; i < 8; ++i) knobs[i]->setBounds (6 + (i % 4) * w, 106 + (i / 4) * 68, w, 66);
}
void PultecPanel::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (1); const bool o = on->param->getValue() > 0.5f;
    base.draw (g, getLocalBounds().toFloat(), o ? 1 : 0, [&] (juce::Graphics& bg) { neo::setTheme (3); neo::plate (bg, b, o ? BC::amber : BC::faint, 12.0f); });
    glow (g, "PULTEC EQP-1A", { 38, 6, 160, 24 }, o ? BC::text : BC::dim, bf (11.5f, true));
    g.setColour (BC::dim); g.setFont (bf (8.5f, false, true)); g.drawText ("boost e atten insieme = il trucco", juce::Rectangle<float> (b.getRight() - 190, 8, 180, 20), juce::Justification::centredRight);
    auto d = juce::Rectangle<float> (12, 34, b.getWidth() - 22, 66); inset (g, d, 6);
    auto v = [this] (const char* id) { return ed.proc.apvts.getRawParameterValue (id)->load(); };
    PultecSettings P; P.lb = v ("u_pLB"); P.la = v ("u_pLA"); P.lf = (int) v ("u_pLF"); P.hb = v ("u_pHB"); P.bw = v ("u_pBW"); P.hf = (int) v ("u_pHF"); P.ha = v ("u_pHA"); P.as = (int) v ("u_pAS");
    Bq f[4]; pultecFilters (P, 48000, f);
    auto in = d.reduced (6, 6);
    juce::Path p, fill; fill.startNewSubPath (in.getX(), in.getCentreY());
    for (int k = 0; k <= 90; ++k)
    {
        const float hz = 20.0f * std::pow (1000.0f, k / 90.0f); double db = 0; for (auto& q : f) db += q.magDb (hz, 48000);
        const float x = in.getX() + in.getWidth() * k / 90.0f, y = in.getCentreY() - juce::jlimit (-18.0f, 18.0f, (float) db) / 18.0f * in.getHeight() * 0.5f;
        if (k == 0) p.startNewSubPath (x, y); else p.lineTo (x, y); fill.lineTo (x, y);
    }
    fill.lineTo (in.getRight(), in.getCentreY()); fill.closeSubPath();
    const auto c = o ? BC::amber : BC::dim;
    g.setColour (BC::phos.withAlpha (0.1f)); g.drawHorizontalLine ((int) in.getCentreY(), in.getX(), in.getRight());
    g.setColour (c.withAlpha (0.18f)); g.fillPath (fill);
    g.setColour (c.withAlpha (0.35f)); g.strokePath (p, juce::PathStrokeType (4.0f)); g.setColour (c); g.strokePath (p, juce::PathStrokeType (1.5f));
}

// ======================= Imager =======================
ImagerPanel::ImagerPanel (Editor& e) : ed (e)
{
    auto& A = ed.proc.apvts; const auto c = BC::phos;
    on = std::make_unique<Switch> (A.getParameter ("u_iOn"), "", c);
    width = std::make_unique<PKnob> (A.getParameter ("u_iWidth"), "WIDTH", c, false, [] (float v) { return juce::String (juce::roundToInt (v * 100)) + "%"; });
    width->bipolar = true;
    mono = std::make_unique<PKnob> (A.getParameter ("u_iMono"), "MONO BASSI", c, false, [] (float v) { return v < 21 ? juce::String ("OFF") : "< " + juce::String (juce::roundToInt (v)) + " Hz"; });
    for (juce::Component* k : std::initializer_list<juce::Component*> { on.get(), width.get(), mono.get() }) addAndMakeVisible (k);
}
void ImagerPanel::resized()
{
    on->setBounds (12, 8, 20, 20);
    const int w = (getWidth() - 12) / 2;
    width->setBounds (6, 128, w, 74); mono->setBounds (6 + w, 128, w, 74);
}
void ImagerPanel::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (1); const bool o = on->param->getValue() > 0.5f;
    base.draw (g, getLocalBounds().toFloat(), o ? 1 : 0, [&] (juce::Graphics& bg) { neo::setTheme (3); neo::plate (bg, b, o ? BC::phos : BC::faint, 12.0f); });
    glow (g, "IMAGER", { 38, 6, 80, 24 }, o ? BC::text : BC::dim, bf (11.5f, true));
    auto& D = ed.proc.dsp;
    auto d = juce::Rectangle<float> (12, 38, b.getWidth() - 22, 82); inset (g, d, 6);
    auto sq = juce::Rectangle<float> (d.getX() + 4, d.getY() + 4, d.getHeight() - 8, d.getHeight() - 8);
    g.setColour (BC::phos.withAlpha (0.1f)); g.drawLine (sq.getCentreX(), sq.getY(), sq.getCentreX(), sq.getBottom()); g.drawLine (sq.getX(), sq.getY(), sq.getRight(), sq.getBottom()); g.drawLine (sq.getRight(), sq.getY(), sq.getX(), sq.getBottom());
    const int w0 = D.anW.load(); float pk = 0.05f;
    for (int i = 0; i < 1024; ++i) { const int k = (w0 - 1024 + i + Burro::AN) % Burro::AN; pk = juce::jmax (pk, std::abs (D.anL[k]), std::abs (D.anR[k])); }
    const float sc = sq.getWidth() * 0.45f / pk;
    g.setColour (BC::phos.withAlpha (0.6f));
    for (int i = 0; i < 1024; i += 2)
    {
        const int k = (w0 - 1024 + i + Burro::AN) % Burro::AN; const float l = D.anL[k], r = D.anR[k];
        const float x = sq.getCentreX() + (r - l) * 0.707f * sc, y = sq.getCentreY() - (l + r) * 0.707f * sc * 0.5f;
        if (sq.contains (x, y)) g.fillRect (x, y, 1.3f, 1.3f);
    }
    auto cr = juce::Rectangle<float> (sq.getRight() + 8, d.getY() + 10, d.getRight() - sq.getRight() - 16, d.getHeight() - 20);
    g.setColour (BC::dim); g.setFont (bf (8.5f, true)); g.drawText ("CORR.", cr.withHeight (12), juce::Justification::centred);
    auto bar = juce::Rectangle<float> (cr.getCentreX() - 4, cr.getY() + 16, 8, cr.getHeight() - 32);
    g.setColour (juce::Colours::black.withAlpha (0.5f)); g.fillRoundedRectangle (bar, 3);
    const float cv = D.corr.load(), yy = bar.getCentreY() - cv * bar.getHeight() * 0.5f;
    const auto cc = cv < 0 ? juce::Colour (0xffff3b5c) : cv < 0.3f ? BC::amber : juce::Colour (0xff7dff5a);
    g.setColour (cc); g.fillRoundedRectangle (bar.getX(), juce::jmin (yy, bar.getCentreY()), bar.getWidth(), std::abs (yy - bar.getCentreY()) + 1, 2);
    g.setColour (cc); g.setFont (bf (9.0f, true, true)); g.drawText (juce::String (cv, 2), juce::Rectangle<float> (cr.getX(), cr.getBottom() - 12, cr.getWidth(), 12), juce::Justification::centred);
    g.setColour (juce::Colours::black.withAlpha (0.18f)); for (float y = d.getY(); y < d.getBottom(); y += 3) g.drawHorizontalLine ((int) y, d.getX() + 2, d.getRight() - 2);
    g.setColour (BC::dim); g.setFont (bf (8.5f, false, true)); g.drawText ("larghezza e bassi in mono", juce::Rectangle<float> (10, b.getHeight() - 30, b.getWidth() - 20, 24), juce::Justification::centred);
}

// ======================= editor =======================
static juce::Rectangle<int> slotRect (int s) { return { 18 + s * 158, 78, 150, 470 }; }

Editor::Editor (Processor& p) : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf); setOpaque (true);
    for (int m = 0; m < 7; ++m) addAndMakeVisible (modules.add (new Module (*this, m)));
    eq = std::make_unique<EqPanel> (*this); addAndMakeVisible (*eq);
    pultec = std::make_unique<PultecPanel> (*this); addAndMakeVisible (*pultec);
    imager = std::make_unique<ImagerPanel> (*this); addAndMakeVisible (*imager);
    auto& A = proc.apvts; const auto mc = BC::amber;
    auto add = [this] (juce::RangedAudioParameter* pr, const char* l, juce::Colour c, bool big, std::function<juce::String (float)> f, bool bp = false) { auto* k = master.add (new PKnob (pr, l, c, big, std::move (f))); k->bipolar = bp; addAndMakeVisible (k); };
    add (A.getParameter ("u_tone"), "TONE", mc, false, bip, true);
    add (A.getParameter ("u_air"), "AIR", mc, false, pct);
    add (A.getParameter ("u_att"), "ATTACK", mc, false, bip, true);
    add (A.getParameter ("u_sus"), "SUSTAIN", mc, false, bip, true);
    add (A.getParameter ("u_mag"), "MAGNITUDE", BC::magenta, true, pct);
    add (A.getParameter ("u_in"), "INPUT", BC::phos, false, [] (float v) { return juce::String (v, 1) + " dB"; }, true);
    add (A.getParameter ("u_out"), "OUTPUT", BC::phos, false, [] (float v) { return juce::String (v, 1) + " dB"; });
    clip = std::make_unique<Switch> (A.getParameter ("u_clip"), "LIMITE", BC::magenta); addAndMakeVisible (*clip);
    limType = std::make_unique<Pill> (dynamic_cast<juce::AudioParameterChoice*> (A.getParameter ("u_limType")), BC::magenta); addAndMakeVisible (*limType);
    addAndMakeVisible (sizeChip);
    scaler.setup();
    startTimerHz (30);
}

void Editor::layoutModules()
{
    modules[0]->setBounds (slotRect (0)); modules[5]->setBounds (slotRect (6));
    for (int s = 0; s < 5; ++s) modules[proc.dsp.order[s].load()]->setBounds (slotRect (s + 1));
}
void Editor::resized()
{
    const float sc = scaler.update();
    layoutModules();
    eq->setBounds (18, 560, 574, 244);
    pultec->setBounds (600, 560, 322, 244); imager->setBounds (930, 560, 188, 244);
    const int mx = MX;
    for (int i = 0; i < 4; ++i) master[i]->setBounds (mx + 18 + (i % 2) * 104, 120 + (i / 2) * 98, 96, 94);
    clip->setBounds (mx + 18, 318, 100, 22); limType->setBounds (mx + 122, 317, 96, 24);
    master[4]->setBounds (mx + 39, 352, 160, 186);
    master[5]->setBounds (mx + 18, 560, 96, 94); master[6]->setBounds (mx + 122, 560, 96, 94);
    presetRect = { 500, 15, 340, 36 }; prevRect = { 462, 15, 36, 36 }; nextRect = { 842, 15, 36, 36 }; randRect = { 896, 15, 120, 36 }; abRect = { 1026, 15, 100, 36 };
    sizeChip.setBounds (W - 76, H - 18, 58, 15);
    jlook::scaleChildren (*this, sc);
}

void Editor::moduleDropped (Module& m)
{
    const int cx = m.getBounds().getCentreX();
    const int slot = juce::jlimit (1, 5, (int) std::round ((cx - 18 - 75) / 158.0));
    std::vector<int> v; for (auto& o : proc.dsp.order) v.push_back (o.load());
    const auto it = std::find (v.begin(), v.end(), m.mod); if (it != v.end()) v.erase (it);
    v.insert (v.begin() + (slot - 1), m.mod);
    for (int i = 0; i < 5; ++i) proc.dsp.order[i] = v[(size_t) i];
    layoutModules(); repaint();
}

void Editor::paint (juce::Graphics& g)
{
    g.addTransform (juce::AffineTransform::scale (scaler.s));
    // fondo, testata, piastre e scritte fisse in cache: prima tutto veniva rifatto dietro a ogni display che si muoveva
    base.draw (g, { 0, 0, (float) W, (float) H }, 3, [this] (juce::Graphics& bg) { paintStatic (bg); });
    neo::setTheme (3);
    // preset
    inset (g, presetRect.toFloat(), 8);
    g.setFont (neo::num (12.0f)); g.setColour (BC::amber); g.drawFittedText (proc.getProgramName (proc.preset).toUpperCase(), presetRect.reduced (8, 0), juce::Justification::centred, 1, 0.7f);
    neo::key (g, randRect.toFloat(), true, BC::magenta, randHover); g.setColour (BC::text); g.setFont (neo::cap (10.0f)); g.drawText ("RANDOM", randRect.toFloat(), juce::Justification::centred);
    neo::key (g, abRect.toFloat(), proc.ab.cur == 1, BC::phos, abHover); g.setColour (BC::text); g.setFont (neo::cap (9.5f)); g.drawFittedText ("MEMORIA " + proc.ab.label(), abRect.reduced (8, 0), juce::Justification::centred, 1, 0.7f);
    g.setColour (BC::amber); g.setFont (bf (20.0f, true));
    g.drawText (juce::String::fromUTF8 ("\xe2\x80\xb9"), prevRect, juce::Justification::centred); g.drawText (juce::String::fromUTF8 ("\xe2\x80\xba"), nextRect, juce::Justification::centred);
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
    meter ({ MX + 18.0f, 668, 40, 128 }, proc.inL.load(), proc.inR.load(), "IN");
    meter ({ MX + 176.0f, 668, 40, 128 }, proc.outL.load(), proc.outR.load(), "OUT");
    g.setColour (BC::dim); g.setFont (bf (9.0f));
    g.drawFittedText ("lucchetto = blocca il modulo nei preset\ntrascina la testata per riordinare", juce::Rectangle<int> (MX + 66, 680, 104, 90), juce::Justification::centred, 6);
}
void Editor::paintStatic (juce::Graphics& g)
{
    const float Wf = (float) W, Hf = (float) H;
    neo::setTheme (3);
    neo::cyberBackdrop (g, Wf, Hf, 0.5f);
    neo::cyberHeader (g, Wf, 64, "BTR-7", "BUTTER RACK // DRUM BUS");
    for (int s = 0; s < 6; ++s) { const float x = (float) slotRect (s).getRight() + 4; juce::Path ar; ar.addArrow ({ x - 2, 96.0f, x + 6, 96.0f }, 1.5f, 6, 6); g.setColour (BC::phos.withAlpha (0.6f)); g.fillPath (ar); }
    auto mr = juce::Rectangle<float> ((float) MX, 78, 238, 726);
    neo::plate (g, mr, BC::magenta, 14.0f);
    g.setFont (neo::cap (10.0f)); g.setColour (BC::text); g.drawText ("MASTER", mr.withHeight (34), juce::Justification::centred);
    g.setColour (BC::magenta); g.fillRect (mr.getX() + 14, mr.getY() + 33, mr.getWidth() - 28, 1.0f);
}

void Editor::mouseMove (const juce::MouseEvent& e0)
{
    const auto e = e0.withNewPosition (e0.position / scaler.s);
    const bool h = randRect.contains (e.getPosition()), h2 = abRect.contains (e.getPosition());
    if (h != randHover || h2 != abHover) { randHover = h; abHover = h2; repaint(); }
    setMouseCursor (h || h2 || presetRect.contains (e.getPosition()) || prevRect.contains (e.getPosition()) || nextRect.contains (e.getPosition()) ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
}
void Editor::mouseDown (const juce::MouseEvent& e0)
{
    const auto e = e0.withNewPosition (e0.position / scaler.s);
    const int n = proc.getNumPrograms();
    if (abRect.contains (e.getPosition())) { proc.ab.toggle (proc); resized(); repaint(); }
    else if (randRect.contains (e.getPosition())) proc.randomize();
    else if (prevRect.contains (e.getPosition())) proc.loadPreset ((proc.preset - 1 + n) % n);
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
        m.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea (localAreaToGlobal (scaler.map (presetRect))), [this] (int r) { if (r > 0) { proc.loadPreset (r - 1); repaint(); } });
    }
    repaint();
}

void Editor::timerCallback()
{
    // si ridisegna solo cio che si muove: display dei moduli, manopole cambiate, analizzatori
    static int tick = 0; ++tick;
    auto knobDirty = [] (PKnob* k) { const float v = k->param->getValue(); if (v != k->lastV || k->activity) { k->lastV = v; k->repaint(); } };
    if (tick % 2 == 0) { eq->updateSpectrum(); eq->repaint (eq->plot().toNearestInt().expanded (2)); }
    if (tick % 3 == 0) imager->repaint();
    for (auto* m : modules)
    {
        m->repaint (m->displayRect().toNearestInt().expanded (2)); m->repaint (0, 0, m->getWidth(), 40);
        if (m->big) knobDirty (m->big.get()); for (auto* k : m->smalls) knobDirty (k);
    }
    for (auto* k : master) knobDirty (k);
    for (auto* k : eq->knobs) { const float v = k->param->getValue(); if (v != k->lastV) { k->lastV = v; k->repaint(); eq->repaint(); } }
    for (auto* k : pultec->knobs) { const float v = k->param->getValue(); if (v != k->lastV) { k->lastV = v; k->repaint(); pultec->repaint(); } }
    clip->repaint(); limType->repaint();
    repaint (scaler.map ({ MX, 660, 238, 140 })); repaint (scaler.map (presetRect));
}
}
