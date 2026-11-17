#include <map>
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
// schermo incassato (come Dust e Studio Comp)
static void inset (juce::Graphics& g, juce::Rectangle<float> r, float rad)
{
    rad = juce::jmin (rad, 7.0f);
    g.setColour (juce::Colours::black.withAlpha (0.65f)); g.fillRoundedRectangle (r.expanded (1.5f), rad + 1.5f);
    juce::ColourGradient cg (BC::lcd1, r.getCentreX(), r.getCentreY(), BC::lcd2, r.getX(), r.getY(), true); g.setGradientFill (cg); g.fillRoundedRectangle (r, rad);
    g.setColour (juce::Colours::white.withAlpha (0.05f)); g.drawRoundedRectangle (r.reduced (0.5f), rad, 1.0f);
}
// frontali: ogni modulo e' una macchina diversa (pannello, scritte, manopole)
struct Face { juce::uint32 p1, p2, ink; int knob; const char* sub; };
static const Face& face (int m)
{
    static const Face F[10] = {
        { 0xff3a2418, 0xff1d120c, 0xfff1dcc4, 0, "sub, boom, rumore" },
        { 0xff2a2340, 0xff15111f, 0xffe6e0f4, 2, "pitch e formante" },
        { 0xff3a3d44, 0xff1d1f24, 0xffe9e2cf, 1, "6 caratteri + OTT" },
        { 0xff4d1716, 0xff2a0c0b, 0xfff1dcc4, 0, "9 macchine, a 8x" },
        { 0xff15363e, 0xff0b1e23, 0xffdff4f4, 1, "room, hall, plate, molla" },
        { 0xff202228, 0xff0f1013, 0xffe6e8ec, 1, "a stadi, a 8x" },
        { 0xff3b1838, 0xff1e0b1c, 0xfff4dff0, 2, "delay, bitcrush, wobble" },
        { 0xff1c1e24, 0xff0d0e11, 0xffe6e8ec, 1, "" },     // EQ
        { 0xff4b402c, 0xff2c251a, 0xfff0e4c8, 0, "" },     // Pultec
        { 0xff17262c, 0xff0b1418, 0xffdff4f4, 1, "" } };   // Imager
    return F[juce::jlimit (0, 9, m)];
}
static void panel (juce::Graphics& g, juce::Rectangle<float> b, int f, juce::Colour acc, bool on, int seed)
{
    const auto& F = face (f);
    hw::faceplate (g, b, juce::Colour (F.p1), juce::Colour (F.p2), on ? acc : BC::faint, 34, seed, 12);
}
static void scan (juce::Graphics&, juce::Rectangle<float>) {}

// ======================= manopola hardware (la stessa di Studio Comp e Dust) =======================
bool PKnob::changed()
{
    const float v = param->getValue(); if (dynLabel) label = dynLabel();
    const auto t = (fmt ? fmt (param->convertFrom0to1 (v)) : param->getCurrentValueAsText()) + label; const bool l = lit ? lit() : true;
    if (v == lastV && t == lastTxt && l == lastLit) return false;
    lastV = v; lastTxt = t; lastLit = l; return true;
}
void PKnob::paint (juce::Graphics& g)
{
    if (dynLabel) label = dynLabel();
    hw::KnobSpec k; k.style = style; k.col = col; k.ink = ink; k.cap = col; k.label = label; k.big = isBig; k.bipolar = bipolar; k.hover = hover; k.drag = active;
    k.enabled = lit ? lit() : true;
    k.text = fmt ? fmt (param->convertFrom0to1 (param->getValue())) : param->getCurrentValueAsText();
    hw::knob (g, getLocalBounds().toFloat(), param->getValue(), k);
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
    auto led = b.removeFromLeft (b.getHeight());
    hw::jewel (g, led.getCentreX(), led.getCentreY(), juce::jmin (6.0f, led.getHeight() * 0.3f), on, col);
    if (label.isNotEmpty()) { g.setColour (on ? BC::text : BC::dim); g.setFont (bf (10.0f, true)); g.drawText (label, b.withTrimmedLeft (5), juce::Justification::centredLeft); }
}

// ======================= curve per i display =======================
static juce::String hzFmt (float v) { return v < 21 ? juce::String ("OFF") : v > 19900 ? juce::String ("OFF") : v >= 1000 ? juce::String (v / 1000, 1) + "k" : juce::String (juce::roundToInt (v)); }
// curva del saturatore per il display: la parte non lineare dei modelli di Alter Drive (senza i filtri), a volume pari
static float satRaw (int type, float u, float d)
{
    switch (type)
    {
        case 1: case 2: return std::tanh (u * 0.8f + 0.08f) - 0.0798f;                                   // 4 TRACK, TAPE
        case 3: return std::tanh (u + 0.35f) - std::tanh (0.35f);                                         // TUBE (valvola)
        case 4: { const float q = std::pow (2.0f, 15.0f - 12.0f * d); return std::round (std::tanh (u) * q) / q; }   // LO-FI
        case 5: return std::sin (u * 1.5707963f);                                                           // FLUB (ripiegatore)
        case 6: { const float v = u * 0.35f * (1.0f + 118.0f * d * d); return v / std::sqrt (1.0f + v * v) + 0.15f * u * 0.35f; }   // OVERDRIVE
        case 7: { const float gn = 1.0f + 1500.0f * d * d * d, v = u * 0.25f * gn / (1.0f + 0.02f * gn); return juce::jlimit (-1.0f, 1.0f, v); }   // DISTORSORE
        case 8: { const float v = u * 0.7f * (1.0f + 6.0f * d), w = v + 0.25f; return (w >= 0 ? std::tanh (2.2f * w) : 0.75f * std::tanh (0.7f * w)) - std::tanh (0.55f); }   // FUZZ
        default: { const float u2 = u * u; return u / std::sqrt (std::sqrt (1.0f + u2 * u2)); }           // CONSOLE
    }
}
static float satCurve (int type, float x, float a, float st)
{
    juce::ignoreUnused (st);
    static const float maxDb[9] = { 30, 30, 30, 30, 24, 22, 18, 18, 24 };
    const float ae = a * a, g = juce::Decibels::decibelsToGain (maxDb[juce::jlimit (0, 8, type)] * 0.8f * ae);
    float pk = 1e-3f; for (float t : { -1.0f, -0.5f, 0.5f, 1.0f }) pk = juce::jmax (pk, std::abs (satRaw (type, t * g, ae)) / std::abs (t));
    const float y = satRaw (type, x * g, ae) / pk;   // stesso volume (come il guadagno automatico del modulo)
    return x + (y - x) * juce::jmin (1.0f, a * 3.0f);
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
    {   auto setK = [this] (PKnob* k) { k->style = face (mod).knob; k->ink = juce::Colour (face (mod).ink); k->lit = [this] { return power->param->getValue() > 0.5f; }; };
        setK (big.get()); for (auto* k : smalls) setK (k); }
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
    power->setBounds (w - 32, 8, 22, 22);   // spia a destra, come Dust e Alter
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
        const int t = juce::roundToInt (ed.proc.apvts.getRawParameterValue ("u_xType")->load());
        smalls[0]->label = t == 0 ? "TIME" : t == 1 ? "BITS" : "RATE";
        smalls[1]->label = t == 0 ? "FEEDBACK" : t == 1 ? "RATE" : "DEPTH";
    }
    const bool lk = ed.proc.locked[mod].load();
    // frontale in cache: si rifa' solo se cambia acceso/spento, trascinamento, lucchetto o tema
    const int sl = dragging ? 9 : slot();
    base.draw (g, getLocalBounds().toFloat(), ((on ? 1 : 0) + (dragging ? 2 : 0) + (lk ? 4 : 0)) + 8 * sl + 1000 * neo::K::theme, [&] (juce::Graphics& bg)
    {
        panel (bg, b, mod, c, on, 11 + mod);
        bg.setColour (BC::text.withAlpha (on ? 1.0f : 0.6f)); bg.setFont (bf (11.0f, true));
        // numero = posto nella catena (i moduli si riordinano trascinando la testata)
        bg.drawFittedText ((sl < 9 ? juce::String (sl + 1) + " " : juce::String()) + kModNames[mod], juce::Rectangle<int> (14, 6, (int) b.getWidth() - 74, 24), juce::Justification::centredLeft, 1, 0.8f);
        if (dragging) hw::cyberFrame (bg, b.reduced (2), c, 16);
        // lucchetto
        auto lr = lockRect().toFloat();
        bg.setColour (lk ? BC::amber : juce::Colour (face (mod).ink).withAlpha (0.45f));
        if (lk) bg.fillRoundedRectangle (lr.getX() + 3, lr.getY() + 8, 12, 9, 2); else bg.drawRoundedRectangle (lr.getX() + 3, lr.getY() + 8, 12, 9, 2, 1.5f);
        juce::Path arc; arc.addCentredArc (lr.getCentreX(), lr.getY() + 8, 4, 5, 0, -juce::MathConstants<float>::halfPi, lk ? juce::MathConstants<float>::halfPi : 0.3f, true);
        bg.strokePath (arc, juce::PathStrokeType (1.5f));
        // maniglia per spostare
        hw::bezel (bg, displayRect().expanded (3), 8);
        if (! on) { bg.setColour (juce::Colours::black.withAlpha (0.32f)); bg.fillRoundedRectangle (b.reduced (4).withTrimmedTop (34), 8); }
    });
    drawDisplay (g, displayRect());
    if (! on) { g.setColour (juce::Colours::black.withAlpha (0.45f)); g.fillRoundedRectangle (displayRect(), 6); }
}

void Module::mouseDown (const juce::MouseEvent& e)
{
    if (lockRect().contains (e.getPosition())) { ed.proc.locked[mod] = ! ed.proc.locked[mod].load(); repaint(); return; }
    dragging = ((mod >= 1 && mod <= 4) || mod == 6) && e.position.y < 36; dragX = getX();
    if (dragging) toFront (false);
}
void Module::mouseDrag (const juce::MouseEvent& e) { if (dragging) { setTopLeftPosition (dragX + e.getDistanceFromDragStartX(), getY()); ed.repaint(); } }
void Module::mouseUp (const juce::MouseEvent&) { if (dragging) { dragging = false; ed.moduleDropped (*this); } }

// ======================= EQ (lo stesso del Master: punti sul grafico, scheda della banda, spettro prima/dopo) =======================
static juce::String dbS (float v, int dec = 1) { return (v > 0.05f ? "+" : "") + juce::String (v, dec); }
static juce::String hzT (float f) { return f >= 1000 ? juce::String (f / 1000.0f, f >= 10000 ? 1 : 2) + " kHz" : juce::String (juce::roundToInt (f)) + " Hz"; }
static juce::Colour bandCol (int b)
{
    static const juce::uint32 c[8] = { 0xffffb547, 0xffff8a3d, 0xffff3d9a, 0xff8b5cff, 0xff3ee8ff, 0xff4dffb4, 0xffffe066, 0xffff6b6b };
    return juce::Colour (c[juce::jlimit (0, 7, b)]);
}

EqPanel::EqPanel (Editor& e) : ed (e), view (e)
{
    on = std::make_unique<Switch> (ed.proc.apvts.getParameter ("u_eqOn"), "", BC::magenta); addAndMakeVisible (*on);
    addAndMakeVisible (view);
}
void EqPanel::resized()
{
    on->setBounds (getWidth() - 34, 8, 20, 20);
    view.setBounds (10, 38, getWidth() - 20, getHeight() - 48);
}
void EqPanel::setBig (bool b)
{
    if (b == big) return;
    big = b;
    if (big) { smallBounds = getBounds(); setBounds (14, 70, Editor::W - 28, Editor::H - 84); toFront (false); }
    else setBounds (smallBounds);
    view.sel = view.popBand = -1; view.updatePop(); repaint();
}
void EqPanel::mouseDown (const juce::MouseEvent& e)
{
    if (bigBtn().contains (e.getPosition())) setBig (! big);
}
void EqPanel::mouseMove (const juce::MouseEvent& e)
{
    const bool h = bigBtn().contains (e.getPosition()); if (h != btnHover) { btnHover = h; repaint (bigBtn()); }
    setMouseCursor (h ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
}
void EqPanel::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    const bool eon = ed.proc.apvts.getRawParameterValue ("u_eqOn")->load() > 0.5f;
    base.draw (g, b, (eon ? 1 : 0) + 2 * neo::K::theme + 100 * getWidth(), [&] (juce::Graphics& bg)
    {
        panel (bg, b.reduced (1), 7, BC::magenta, eon, 31);
        bg.setColour (BC::text.withAlpha (eon ? 1.0f : 0.6f)); bg.setFont (bf (12.0f, true)); bg.drawText ("8  EQ", juce::Rectangle<float> (16, 6, 60, 24), juce::Justification::centredLeft);
        bg.setColour (BC::dim); bg.setFont (bf (9.0f, false, true));
        bg.drawFittedText (juce::String::fromUTF8 ("doppio clic = banda  \xc2\xb7  rotella = Q  \xc2\xb7  ogni banda pu\xc3\xb2 essere dinamica"),
                           juce::Rectangle<int> (70, 6, (int) b.getWidth() - 250, 24), juce::Justification::centredLeft, 1, 0.75f);
    });
    jlook::button (g, bigBtn().toFloat(), BC::magenta, big, btnHover, big ? juce::String::fromUTF8 ("\xe2\x86\x99  RIDUCI") : juce::String::fromUTF8 ("\xe2\x86\x97  INGRANDISCI"), bf (9.5f, true));
}

EqView::EqView (Editor& e) : ed (e)
{
    win.resize (4096); buf.resize (8192); specPre.assign (2048, -100.0f); specPost.assign (2048, -100.0f);
    for (int i = 0; i < 4096; ++i) win[(size_t) i] = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * i / 4095.0f);
    addChildComponent (pop);
    // ridisegno agganciato al refresh dello schermo (come l'onda del FACILE): le bande dinamiche scorrono senza scatti
    vb = std::make_unique<juce::VBlankAttachment> (this, [this]
    {
        if (! isShowing()) return;
        const double now = juce::Time::getMillisecondCounterHiRes(); const float dt = (float) juce::jlimit (1.0, 50.0, now - lastFrame); lastFrame = now;
        const float k = 1.0f - std::exp (-dt / 45.0f);
        for (int b = 0; b < kQBands; ++b) dynShown[b] += (ed.proc.dsp.eqDyn (b) - dynShown[b]) * k;
        specFrame (dt);
        repaint (plot().toNearestInt().expanded (14));
    });
}
float EqView::val (int b, const char* k) const { return ed.proc.apvts.getRawParameterValue (qId (b, k))->load(); }
void EqView::setV (int b, const char* k, float v)
{
    if (auto* p = ed.proc.apvts.getParameter (qId (b, k))) p->setValueNotifyingHost (p->convertTo0to1 (v));
    if (ed.proc.apvts.getRawParameterValue ("u_eqOn")->load() < 0.5f) if (auto* o = ed.proc.apvts.getParameter ("u_eqOn")) o->setValueNotifyingHost (1.0f);   // toccare l'EQ lo accende
}
float EqView::xOf (float f) const { auto p = plot(); return p.getX() + p.getWidth() * std::log (juce::jmax (10.0f, f) / 20.0f) / std::log (1000.0f); }
float EqView::fOf (float x) const { auto p = plot(); return 20.0f * std::pow (1000.0f, (x - p.getX()) / p.getWidth()); }
float EqView::yOf (float db) const { auto p = plot(); return p.getY() + p.getHeight() * (0.5f - db / 36.0f); }
float EqView::dbOf (float y) const { auto p = plot(); return (0.5f - (y - p.getY()) / p.getHeight()) * 36.0f; }
juce::Point<float> EqView::nodeOf (int b) const
{
    const int t = (int) val (b, "type");
    return { xOf (val (b, "f")), jmx::hasGain (t) ? yOf (val (b, "g")) : yOf (0) };
}
int EqView::hit (juce::Point<float> p) const
{
    int best = -1; float bd = 13;
    for (int b = 0; b < kQBands; ++b) if (val (b, "on") > 0.5f) { const float d = p.getDistanceFrom (nodeOf (b)); if (d < bd) { bd = d; best = b; } }
    return best;
}
void EqView::tick()
{
    if (popHold > 0 && --popHold == 0 && ! pop.isMouseOver (true) && ! isMouseOver (false)) { popBand = sel; updatePop(); }
}
// spettro a ogni fotogramma: per ogni pixel il massimo delle frequenze che ci cadono (niente denti in alto),
// sale subito e scende morbido nel tempo vero (non a scatti del timer)
void EqView::specFrame (float dtMs)
{
    auto p = plot(); const int W = juce::jmax (2, (int) (p.getWidth() / 2)); const double sr = (ed.proc.getSampleRate() > 1000 ? ed.proc.getSampleRate() : 48000.0);
    if ((int) pxPre.size() != W) { pxPre.assign ((size_t) W, -120.0f); pxPost.assign ((size_t) W, -120.0f); }
    const float fall = 1.0f - std::exp (-dtMs / 260.0f);
    auto& D = ed.proc.dsp; const int w0 = D.anW.load();
    auto run = [&] (const float* src, std::vector<float>& px)
    {
        for (int i = 0; i < 4096; ++i) buf[(size_t) i] = src[(w0 + i) % Burro::AN] * win[(size_t) i];
        std::fill (buf.begin() + 4096, buf.end(), 0.0f); fft.performFrequencyOnlyForwardTransform (buf.data());
        for (int x = 0; x < W; ++x)
        {
            const float f0 = fOf (p.getX() + x * 2.0f), f1 = fOf (p.getX() + x * 2.0f + 2.0f);
            const int b0 = juce::jlimit (1, 2047, (int) (f0 / sr * 4096)), b1 = juce::jlimit (b0, 2047, (int) (f1 / sr * 4096));
            float m = 0; for (int b = b0; b <= b1; ++b) m = juce::jmax (m, buf[(size_t) b]);
            const float fc = 0.5f * (f0 + f1);
            const float db = juce::Decibels::gainToDecibels (m / 1024.0f, -120.0f) + 4.5f * std::log2 (juce::jmax (0.05f, fc / 1000.0f));
            auto& v = px[(size_t) x]; v = db > v ? db : v + (db - v) * fall;
        }
    };
    run (D.anIn, pxPre); run (D.anOut, pxPost);
}
void EqView::updatePop()
{
    const int b = popBand >= 0 ? popBand : sel;
    if (b < 0 || val (b, "on") < 0.5f) { pop.setVisible (false); pop.band = -1; return; }
    pop.show (b);
    const auto n = nodeOf (b); const int pw = pop.getWidth(), ph = pop.getHeight();
    const int x = juce::jlimit (4, getWidth() - pw - 4, (int) n.x - pw / 2);
    int y = (int) n.y + 18; if (y + ph > getHeight() - 4) y = (int) n.y - 18 - ph;
    pop.setTopLeftPosition (x, juce::jlimit (4, getHeight() - ph - 4, y));
    pop.setVisible (true); pop.repaint();
}
void EqView::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    auto p = plot();
    const double sr = (ed.proc.getSampleRate() > 1000 ? ed.proc.getSampleRate() : 48000.0);
    // vetro, griglia e scritte: fermi, in cache
    grid.draw (g, b, neo::K::theme + 16 * getWidth() + 77777 * getHeight(), [&] (juce::Graphics& gg)
    {
        inset (gg, b, 7);
        gg.setFont (neo::num (8.0f));
        for (float f : { 20.f, 30.f, 40.f, 50.f, 60.f, 80.f, 100.f, 200.f, 300.f, 400.f, 500.f, 600.f, 800.f, 1000.f, 2000.f, 3000.f, 4000.f, 5000.f, 6000.f, 8000.f, 10000.f, 20000.f })
        {
            const bool major = f == 20 || f == 50 || f == 100 || f == 200 || f == 500 || f == 1000 || f == 2000 || f == 5000 || f == 10000 || f == 20000;
            gg.setColour ( juce::Colours::white.withAlpha (major ? 0.07f : 0.03f)); gg.drawVerticalLine ((int) xOf (f), p.getY(), p.getBottom());
            if (major) { gg.setColour (BC::dim); gg.drawText (f >= 1000 ? juce::String ((int) (f / 1000)) + "k" : juce::String ((int) f), juce::Rectangle<float> (xOf (f) - 14, p.getBottom() + 2, 28, 12), juce::Justification::centred); }
        }
        for (int db = -12; db <= 12; db += 6)
        { gg.setColour ( juce::Colours::white.withAlpha (db == 0 ? 0.13f : 0.04f)); gg.drawHorizontalLine ((int) yOf ((float) db), p.getX(), p.getRight()); gg.setColour (BC::dim); gg.drawText ((db > 0 ? "+" : "") + juce::String (db), juce::Rectangle<float> (b.getX() + 4, yOf ((float) db) - 6, 26, 12), juce::Justification::centredRight); }
    });
    auto specPath = [&] (const std::vector<float>& px, bool fill) -> juce::Path
    {
        juce::Path path; const int W = (int) px.size(); if (W < 2) return path;
        auto yOfDb = [&] (float db) { return juce::jlimit (p.getY(), p.getBottom(), p.getBottom() - (db + 90.0f) / 90.0f * p.getHeight()); };
        path.startNewSubPath (p.getX(), fill ? p.getBottom() : yOfDb (px[0])); if (fill) path.lineTo (p.getX(), yOfDb (px[0]));
        for (int x = 1; x < W; ++x) path.lineTo (p.getX() + x * 2.0f + 1.0f, yOfDb (px[(size_t) x]));
        if (fill) { path.lineTo (p.getRight(), p.getBottom()); path.closeSubPath(); }
        return path.createPathWithRoundedCorners (3.0f);
    };
    g.setColour ( juce::Colours::white.withAlpha (0.18f)); g.strokePath (specPath (pxPre, false), juce::PathStrokeType (1.0f));
    { juce::ColourGradient sg (BC::phos.withAlpha (0.24f), 0, p.getY(), BC::phos.withAlpha (0.03f), 0, p.getBottom(), false); g.setGradientFill (sg); g.fillPath (specPath (pxPost, true));
      g.setColour (BC::phos.withAlpha (0.55f)); g.strokePath (specPath (pxPost, false), juce::PathStrokeType (1.0f)); }
    const bool eqOn = ed.proc.apvts.getRawParameterValue ("u_eqOn")->load() > 0.5f;
    juce::Path tot; const int NX = (int) (p.getWidth() / 2) + 2; std::vector<double> sum ((size_t) NX, 0.0);
    for (int k = 0; k < kQBands; ++k)
    {
        if (val (k, "on") < 0.5f) continue;
        jmx::BandSet B; B.on = true; B.type = (int) val (k, "type"); B.slope = (int) val (k, "slope"); B.ch = (int) val (k, "ch"); B.f = val (k, "f"); B.g = val (k, "g"); B.q = val (k, "q");
        B.dyn = val (k, "dyn") > 0.5f; B.range = val (k, "rng");
        jmx::BQ f[8]; const float gNow = B.g + (B.dyn ? dynShown[k] : 0.0f); const int n = jmx::designBand (B, sr, gNow, f);
        juce::Path path; int i = 0; bool st = false;
        for (float x = p.getX(); x < p.getRight() && i < NX; x += 2, ++i)
        {
            double d = 0; const float fr = fOf (x); for (int q = 0; q < n; ++q) d += f[q].magDb (fr, sr);
            if (B.ch <= 1) sum[(size_t) i] += d;
            const float y = yOf ((float) juce::jlimit (-24.0, 24.0, d));
            if (! st) { path.startNewSubPath (x, y); st = true; } else path.lineTo (x, y);
        }
        const auto c = bandCol (k); const bool focus = k == sel || k == hover || k == popBand;
        if (focus) { juce::Path fp = path; fp.lineTo (p.getRight(), yOf (0)); fp.lineTo (p.getX(), yOf (0)); fp.closeSubPath(); g.setColour (c.withAlpha (0.14f)); g.fillPath (fp); }
        g.setColour (c.withAlpha (focus ? 0.95f : 0.4f)); g.strokePath (path, juce::PathStrokeType (focus ? 1.6f : 1.0f));
    }
    { int i = 0; bool st = false; for (float x = p.getX(); x < p.getRight() && i < NX; x += 2, ++i) { const float y = yOf ((float) juce::jlimit (-24.0, 24.0, sum[(size_t) i])); if (! st) { tot.startNewSubPath (x, y); st = true; } else tot.lineTo (x, y); } }
    g.setColour (BC::phos.withAlpha (eqOn ? 0.25f : 0.08f)); g.strokePath (tot, juce::PathStrokeType (5.0f));
    g.setColour ( juce::Colours::white.withAlpha (eqOn ? 0.95f : 0.3f)); g.strokePath (tot, juce::PathStrokeType (1.8f));
    const int so = ed.proc.dsp.eqSolo.load();
    for (int k = 0; k < kQBands; ++k)
    {
        if (val (k, "on") < 0.5f) continue;
        const auto n = nodeOf (k); const auto c = bandCol (k); const bool focus = k == sel || k == hover || k == popBand; const int t = (int) val (k, "type");
        if (val (k, "dyn") > 0.5f && jmx::hasGain (t)) { const float y1 = yOf (val (k, "g") + val (k, "rng")); g.setColour (c.withAlpha (0.55f)); g.drawLine (n.x, n.y, n.x, y1, 2.0f); g.fillEllipse (n.x - 3, y1 - 3, 6, 6); }
        g.setColour (c.withAlpha (focus ? 0.45f : 0.2f)); g.fillEllipse (n.x - 11, n.y - 11, 22, 22);
        g.setColour (c); g.drawEllipse (n.x - 9, n.y - 9, 18, 18, focus ? 2.2f : 1.3f);
        g.setFont (neo::num (9.0f)); g.setColour (juce::Colours::white); g.drawText (juce::String (k + 1), juce::Rectangle<float> (n.x - 9, n.y - 8, 18, 16), juce::Justification::centred);
        juce::String tags; const int ch = (int) val (k, "ch"); if (ch > 0) tags << juce::String ("MSLR")[ch - 1]; if (val (k, "dyn") > 0.5f) tags << " D"; if (so == k) tags << " SOLO";
        if (tags.isNotEmpty()) { g.setFont (neo::cap (7.5f)); g.setColour (c); g.drawText (tags.trim(), juce::Rectangle<float> (n.x + 11, n.y - 20, 60, 11), juce::Justification::centredLeft); }
    }
    if (! eqOn) { g.setFont (neo::f (12.0f)); g.setColour (BC::amber); g.drawText ("EQ SPENTO: tocca un punto o fai doppio clic per accenderlo", p, juce::Justification::centredTop); }
}
void EqView::mouseMove (const juce::MouseEvent& e)
{
    const int h = hit (e.position);
    if (h != hover) { hover = h; repaint(); }
    if (h >= 0 && h != popBand) { popBand = h; updatePop(); }
    if (h >= 0) popHold = 90;
}
void EqView::mouseDown (const juce::MouseEvent& e)
{
    drag = hit (e.position);
    if (drag >= 0)
    {
        sel = drag; popBand = drag; updatePop();
        if (e.mods.isPopupMenu()) { bandMenu (drag); drag = -1; return; }
        ed.proc.apvts.getParameter (qId (drag, "f"))->beginChangeGesture(); ed.proc.apvts.getParameter (qId (drag, "g"))->beginChangeGesture();
    }
    else if (! e.mods.isPopupMenu()) { sel = -1; popBand = -1; updatePop(); }
}
void EqView::mouseDrag (const juce::MouseEvent& e)
{
    if (drag < 0) return;
    const int t = (int) val (drag, "type");
    setV (drag, "f", juce::jlimit (20.0f, 20000.0f, fOf (e.position.x)));
    if (jmx::hasGain (t)) setV (drag, "g", juce::jlimit (-18.0f, 18.0f, dbOf (e.position.y)));
    updatePop();
}
void EqView::mouseUp (const juce::MouseEvent&)
{
    if (drag >= 0) { ed.proc.apvts.getParameter (qId (drag, "f"))->endChangeGesture(); ed.proc.apvts.getParameter (qId (drag, "g"))->endChangeGesture(); }
    drag = -1;
}
void EqView::mouseDoubleClick (const juce::MouseEvent& e)
{
    const int h = hit (e.position);
    if (h >= 0) { setV (h, "on", 0); if (ed.proc.dsp.eqSolo.load() == h) ed.proc.dsp.eqSolo = -1; sel = popBand = -1; updatePop(); return; }
    for (int b = 0; b < kQBands; ++b)
        if (val (b, "on") < 0.5f)
        {
            const float f = fOf (e.position.x); const int t = f < 35 ? jmx::BT_LCUT : f > 17000 ? jmx::BT_HCUT : jmx::BT_BELL;
            setV (b, "dyn", 0); setV (b, "type", (float) t); setV (b, "f", juce::jlimit (20.0f, 20000.0f, f)); setV (b, "q", t == jmx::BT_BELL ? 1.0f : 0.71f);
            setV (b, "g", t == jmx::BT_BELL ? juce::jlimit (-18.0f, 18.0f, dbOf (e.position.y)) : 0.0f); setV (b, "ch", 0);
            setV (b, "on", 1); sel = popBand = b; updatePop(); return;
        }
}
void EqView::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    const int h = hit (e.position); if (h < 0) return;
    auto* p = ed.proc.apvts.getParameter (qId (h, "q")); p->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, p->getValue() + (w.deltaY > 0 ? 0.02f : -0.02f))); updatePop();
}
void EqView::bandMenu (int b)
{
    juce::PopupMenu m, tm, cm, sm; const int t = (int) val (b, "type"), ch = (int) val (b, "ch"), sl = (int) val (b, "slope");
    for (int i = 0; i < qTypeNames().size(); ++i) tm.addItem (100 + i, qTypeNames()[i], true, i == t);
    for (int i = 0; i < qChNames().size(); ++i) cm.addItem (200 + i, qChNames()[i], true, i == ch);
    static const char* sn[4] = { "12 dB/ott", "24 dB/ott", "48 dB/ott", "96 dB/ott" };
    for (int i = 0; i < 4; ++i) sm.addItem (300 + i, sn[i], true, i == sl);
    m.addSectionHeader ("BANDA " + juce::String (b + 1));
    m.addSubMenu ("Tipo", tm); m.addSubMenu ("Canale", cm); m.addSubMenu ("Pendenza (tagli)", sm, t == jmx::BT_LCUT || t == jmx::BT_HCUT);
    m.addItem (1, "Dinamica", jmx::hasGain (t), val (b, "dyn") > 0.5f);
    m.addItem (3, "Solo (ascolta la zona)", true, ed.proc.dsp.eqSolo.load() == b);
    m.addSeparator(); m.addItem (4, "Spegni la banda");
    juce::Component::SafePointer<EqView> sp (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [sp, b] (int r)
    {
        if (sp == nullptr || r <= 0) return;
        auto& v = *sp;
        if (r >= 300) v.setV (b, "slope", (float) (r - 300));
        else if (r >= 200) v.setV (b, "ch", (float) (r - 200));
        else if (r >= 100) v.setV (b, "type", (float) (r - 100));
        else if (r == 1) v.setV (b, "dyn", v.val (b, "dyn") > 0.5f ? 0.0f : 1.0f);
        else if (r == 3) v.ed.proc.dsp.eqSolo = v.ed.proc.dsp.eqSolo.load() == b ? -1 : b;
        else if (r == 4) { v.setV (b, "on", 0); v.sel = v.popBand = -1; }
        v.updatePop(); v.repaint();
    });
}

// finestrella della banda: tipo, canale, SOLO, DINAMICA e i valori trascinabili
enum PopId { PI_TYPE = 1, PI_CH, PI_SLOPE, PI_SOLO, PI_DYN, PI_DEL, PI_F = 20, PI_G, PI_Q, PI_THR, PI_RANGE };
void EqPop::show (int b) { band = b; setSize (296, jmx::hasGain ((int) view.val (b, "type")) ? 172 : 112); }
void EqPop::paint (juce::Graphics& g)
{   // scheda compatta: in alto tipo / canale / solo, sotto tre manopoline (frequenza, guadagno, Q), poi la dinamica sempre pronta
    if (band < 0) return;
    const int b = band; auto v = [this, b] (const char* id) { return view.val (b, id); };
    const auto c = bandCol (b); auto r = getLocalBounds().toFloat();
    g.setColour ( juce::Colours::black.withAlpha (0.5f)); g.fillRoundedRectangle (r.translated (0, 4), 12);
    g.setColour ((neo::light() ? juce::Colour (0xfaf6f4f9) : juce::Colour (0xf40e1019))); g.fillRoundedRectangle (r, 12); g.setColour (c.withAlpha (0.7f)); g.drawRoundedRectangle (r.reduced (0.5f), 12, 1.2f);
    hots.clear();
    const int t = (int) v ("type"); const bool hg = jmx::hasGain (t);
    auto btn = [&] (juce::Rectangle<float> rr, const juce::String& s, bool on, juce::Colour col, int id) { neo::chip (g, rr, s, on, col, 7.5f); hots.push_back ({ rr, id }); };
    g.setFont (neo::num (13.0f)); g.setColour (c); g.drawText (juce::String (b + 1), juce::Rectangle<float> (8, 8, 18, 22), juce::Justification::centred);
    btn ({ 30, 8, 92, 22 }, qTypeNames()[t] + juce::String::fromUTF8 (" \xe2\x96\xbe"), true, c, PI_TYPE);
    btn ({ 126, 8, 74, 22 }, qChNames()[(int) v ("ch")] + juce::String::fromUTF8 (" \xe2\x96\xbe"), v ("ch") > 0.5f, c, PI_CH);
    btn ({ 204, 8, 54, 22 }, "SOLO", view.ed.proc.dsp.eqSolo.load() == b, BC::amber, PI_SOLO);
    btn ({ r.getRight() - 30, 8, 22, 22 }, juce::String::fromUTF8 ("\xc3\x97"), false, juce::Colour (0xffff3b5c), PI_DEL);
    // manopolina: arco del valore, nome sopra, numero sotto
    auto dial = [&] (float cx, float cy, const juce::String& lab, const juce::String& txt, float norm, bool bip, bool en, juce::Colour col, int id)
    {
        const float R = 15, a0 = -2.36f, a1 = 2.36f; auto rr = juce::Rectangle<float> (cx - 34, cy - 30, 68, 62);
        g.setFont (neo::cap (6.5f)); g.setColour (BC::dim); g.drawText (lab, juce::Rectangle<float> (cx - 34, cy - 30, 68, 10), juce::Justification::centred);
        juce::Path tr; tr.addCentredArc (cx, cy, R, R, 0, a0, a1, true);
        g.setColour ( juce::Colours::white.withAlpha (0.08f)); g.strokePath (tr, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        if (en)
        {
            const float av = a0 + (a1 - a0) * juce::jlimit (0.0f, 1.0f, norm), as = bip ? 0.0f : a0;
            juce::Path va; va.addCentredArc (cx, cy, R, R, 0, juce::jmin (as, av), juce::jmax (as, av), true);
            g.setColour (col.withAlpha (0.3f)); g.strokePath (va, juce::PathStrokeType (6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setColour (col); g.strokePath (va, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setColour ((neo::light() ? neo::K::panel1 : juce::Colour (0xff1a1d28))); g.fillEllipse (cx - R + 5, cy - R + 5, (R - 5) * 2, (R - 5) * 2);
            g.setColour (BC::text); g.drawLine (cx + std::sin (av) * (R - 9), cy - std::cos (av) * (R - 9), cx + std::sin (av) * (R - 4), cy - std::cos (av) * (R - 4), 1.6f);
            hots.push_back ({ rr, id });
        }
        g.setFont (neo::num (9.5f)); g.setColour (en ? BC::text : BC::faint); g.drawText (txt, juce::Rectangle<float> (cx - 34, cy + R + 2, 68, 12), juce::Justification::centred);
    };
    auto nv = [&] (const char* p) { return view.ed.proc.apvts.getParameter (qId (b, p))->getValue(); };
    const float y1 = 66;
    dial (52, y1, "FREQUENZA", hzT (v ("f")), nv ("f"), false, true, c, PI_F);
    dial (148, y1, "GUADAGNO", hg ? dbS (v ("g")) + " dB" : juce::String ("--"), nv ("g"), true, hg, c, PI_G);
    dial (244, y1, "Q", juce::String (v ("q"), 2), nv ("q"), false, true, c, PI_Q);
    if (! hg)
    {
        if (t == jmx::BT_LCUT || t == jmx::BT_HCUT) { static const char* sn[4] = { "12 dB/OTT", "24 dB/OTT", "48 dB/OTT", "96 dB/OTT" }; btn ({ 98, 98, 100, 22 }, sn[juce::jlimit (0, 3, (int) v ("slope"))], false, c, PI_SLOPE); }
        return;
    }
    // dinamica: sempre li'. Basta girare SOGLIA o RANGE e la banda diventa dinamica (RANGE a zero = statica)
    const bool dy = v ("dyn") > 0.5f; const auto dc = juce::Colour (0xff7dff5a);
    g.setColour ( juce::Colours::white.withAlpha (0.06f)); g.fillRect (12.0f, 100.0f, r.getWidth() - 24, 1.0f);
    dial (52, 136, "SOGLIA", juce::String (v ("thr"), 1) + " dB", nv ("thr"), false, true, dy ? dc : BC::dim, PI_THR);
    dial (148, 136, "RANGE", dbS (v ("rng")) + " dB", nv ("rng"), true, true, dy ? dc : BC::dim, PI_RANGE);
    // quanto si sta muovendo adesso
    auto mr = juce::Rectangle<float> (204, 108, 82, 46);
    g.setColour ( juce::Colours::black.withAlpha (0.4f)); g.fillRoundedRectangle (mr, 6);
    g.setFont (neo::cap (6.5f)); g.setColour (dy ? dc : BC::faint); g.drawText ("DINAMICA", mr.withHeight (12).translated (0, 3), juce::Justification::centred);
    const float mv = dy ? view.ed.proc.dsp.eqDyn (b) : 0.0f, rg = std::abs (v ("rng")) + 0.01f;
    auto bar = mr.reduced (8, 0).withHeight (5).withY (mr.getY() + 34);
    g.setColour ( juce::Colours::white.withAlpha (0.08f)); g.fillRoundedRectangle (bar, 3);
    const float fr = juce::jlimit (0.0f, 1.0f, std::abs (mv) / rg);
    g.setColour (dc); g.fillRoundedRectangle (bar.withWidth (bar.getWidth() * fr), 3);
    g.setFont (neo::num (9.0f)); g.setColour (dy ? BC::text : BC::faint); g.drawText (dy ? dbS (mv) + " dB" : juce::String ("statica"), mr.withHeight (14).translated (0, 16), juce::Justification::centred);
}
int EqPop::hit (juce::Point<float> p) const { for (auto& h : hots) if (h.r.contains (p)) return h.id; return 0; }
static const char* popParam (int id) { switch (id) { case PI_F: return "f"; case PI_G: return "g"; case PI_Q: return "q"; case PI_THR: return "thr"; case PI_RANGE: return "rng"; default: return nullptr; } }
void EqPop::mouseDown (const juce::MouseEvent& e)
{
    if (band < 0) return;
    const int b = band; const int id = hit (e.position);
    auto menu = [&] (const juce::StringArray& items, const char* p)
    {
        juce::PopupMenu m; const int cur = (int) view.val (b, p); for (int i = 0; i < items.size(); ++i) m.addItem (i + 1, items[i], true, i == cur);
        juce::Component::SafePointer<EqView> sv (&view); const juce::String pid (p);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [sv, b, pid] (int x) { if (x > 0 && sv) { sv->setV (b, pid.toRawUTF8(), (float) (x - 1)); sv->updatePop(); } });
    };
    switch (id)
    {
        case PI_TYPE: menu (qTypeNames(), "type"); break;
        case PI_CH: menu (qChNames(), "ch"); break;
        case PI_SLOPE: menu ({ "12 dB/ott", "24 dB/ott", "48 dB/ott", "96 dB/ott" }, "slope"); break;
        case PI_SOLO: view.ed.proc.dsp.eqSolo = view.ed.proc.dsp.eqSolo.load() == b ? -1 : b; break;
        case PI_DYN: view.setV (b, "dyn", view.val (b, "dyn") > 0.5f ? 0.0f : 1.0f); break;
        case PI_DEL: view.setV (b, "on", 0); if (view.ed.proc.dsp.eqSolo.load() == b) view.ed.proc.dsp.eqSolo = -1; view.sel = view.popBand = -1; break;
        default:
            if (auto* p = popParam (id)) { dragK = id; dragStart = view.ed.proc.apvts.getParameter (qId (b, p))->getValue(); view.ed.proc.apvts.getParameter (qId (b, p))->beginChangeGesture(); }
    }
    view.updatePop(); repaint(); view.repaint();
}
void EqPop::mouseDrag (const juce::MouseEvent& e)
{
    if (dragK < 0 || band < 0) return;
    if (auto* p = popParam (dragK))
    {
        auto* pr = view.ed.proc.apvts.getParameter (qId (band, p)); pr->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, dragStart - e.getDistanceFromDragStartY() / (e.mods.isShiftDown() ? 800.0f : 200.0f)));
        if (dragK == PI_THR || dragK == PI_RANGE) view.setV (band, "dyn", std::abs (view.val (band, "rng")) > 0.05f ? 1.0f : 0.0f);   // girare la dinamica la accende, RANGE a zero la spegne
        repaint(); view.repaint();
    }
}
void EqPop::mouseDoubleClick (const juce::MouseEvent& e)
{
    if (band < 0) return;
    if (auto* p = popParam (hit (e.position))) { auto* pr = view.ed.proc.apvts.getParameter (qId (band, p)); pr->setValueNotifyingHost (pr->getDefaultValue()); repaint(); view.repaint(); }
}
void EqPop::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    if (band < 0) return;
    const int id = hit (e.position);
    if (auto* p = popParam (id))
    {
        auto* pr = view.ed.proc.apvts.getParameter (qId (band, p)); pr->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, pr->getValue() + (w.deltaY > 0 ? 0.01f : -0.01f)));
        if (id == PI_THR || id == PI_RANGE) view.setV (band, "dyn", std::abs (view.val (band, "rng")) > 0.05f ? 1.0f : 0.0f);
        repaint(); view.repaint();
    }
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
    for (auto& k : ks) { auto* pk = knobs.add (new PKnob (A.getParameter (k.id), k.l, c, false, k.f)); pk->style = 0; pk->ink = juce::Colour (face (8).ink); pk->lit = [this] { return on->param->getValue() > 0.5f; }; addAndMakeVisible (pk); }
}
void PultecPanel::resized()
{
    on->setBounds (getWidth() - 32, 8, 22, 22);
    const int w = (getWidth() - 12) / 4;
    for (int i = 0; i < 8; ++i) knobs[i]->setBounds (6 + (i % 4) * w, 106 + (i / 4) * 68, w, 66);
}
void PultecPanel::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (1); const bool o = on->param->getValue() > 0.5f;
    base.draw (g, getLocalBounds().toFloat(), (o ? 1 : 0) + 1000 * neo::K::theme, [&] (juce::Graphics& bg) { panel (bg, b, 8, BC::amber, o, 32); hw::bezel (bg, juce::Rectangle<float> (12, 34, b.getWidth() - 22, 66).expanded (3), 9); });
    glow (g, "9  PULTEC EQP-1A", { 16, 6, 160, 24 }, o ? BC::text : BC::dim, bf (12.0f, true));
    g.setColour (BC::dim); g.setFont (bf (8.5f, false, true)); g.drawText ("boost e atten insieme = il trucco", juce::Rectangle<float> (b.getRight() - 230, 8, 190, 20), juce::Justification::centredRight);
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
    for (auto* k : { width.get(), mono.get() }) { k->style = 1; k->ink = juce::Colour (face (9).ink); k->lit = [this] { return on->param->getValue() > 0.5f; }; }
    for (juce::Component* k : std::initializer_list<juce::Component*> { on.get(), width.get(), mono.get() }) addAndMakeVisible (k);
}
void ImagerPanel::resized()
{
    on->setBounds (getWidth() - 32, 8, 22, 22);
    const int w = (getWidth() - 12) / 2;
    width->setBounds (6, 128, w, 74); mono->setBounds (6 + w, 128, w, 74);
}
void ImagerPanel::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (1); const bool o = on->param->getValue() > 0.5f;
    base.draw (g, getLocalBounds().toFloat(), (o ? 1 : 0) + 1000 * neo::K::theme, [&] (juce::Graphics& bg) { panel (bg, b, 9, BC::phos, o, 33); hw::bezel (bg, juce::Rectangle<float> (12, 38, b.getWidth() - 22, 82).expanded (3), 9); });
    glow (g, "10  IMAGER", { 16, 6, 110, 24 }, o ? BC::text : BC::dim, bf (12.0f, true));
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
    skin::init ("butter", { 0xffe8cf86, 0xffe9a260, 0xffa8bea0, 0xffd8cdb8 }, 0xffffc23d);   // la stessa pelle di Dust e Alter
    setLookAndFeel (&lnf); setOpaque (true);
    for (int m = 0; m < 7; ++m) addAndMakeVisible (modules.add (new Module (*this, m)));
    eq = std::make_unique<EqPanel> (*this); addAndMakeVisible (*eq);
    pultec = std::make_unique<PultecPanel> (*this); addAndMakeVisible (*pultec);
    imager = std::make_unique<ImagerPanel> (*this); addAndMakeVisible (*imager);
    auto& A = proc.apvts; const auto mc = BC::amber;
    auto add = [this] (juce::RangedAudioParameter* pr, const char* l, juce::Colour c, bool big, std::function<juce::String (float)> f, bool bp = false) { auto* k = master.add (new PKnob (pr, l, c, big, std::move (f))); k->bipolar = bp; k->style = big ? 0 : 1; k->ink = juce::Colour (0xffe9e2cf); addAndMakeVisible (k); };
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
    setupBrowser();
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
    presetRect = { 470, 14, 380, 36 }; prevRect = { 432, 14, 36, 36 }; nextRect = { 852, 14, 36, 36 }; randRect = { 900, 14, 110, 36 }; abRect = { 1018, 14, 112, 36 };
    sizeChip.setBounds (W - 76, H - 18, 58, 15);
    browser.setBounds (0, 0, W, H);
    themeRect = { 1142, 18, 100, 28 };
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
    skin::apply();
    g.addTransform (juce::AffineTransform::scale (scaler.s));
    // fondo, testata, piastre e scritte fisse in cache
    base.draw (g, { 0, 0, (float) W, (float) H }, (4) + 1000 * neo::K::theme, [this] (juce::Graphics& bg) { paintStatic (bg); });
    skin::chipDraw (g, themeRect.toFloat(), themeHover);
    // preset
    inset (g, presetRect.toFloat(), 8); jlook::glass (g, presetRect.toFloat(), 8);
    g.setFont (bf (12.5f, false, true)); g.setColour (BC::amber);
    g.drawFittedText ((proc.userName.isNotEmpty() ? juce::String::fromUTF8 ("\xe2\x98\x85 ") : juce::String()) + proc.currentName().replace (" | ", "  /  ").toUpperCase(), presetRect.reduced (10, 0), juce::Justification::centred, 1, 0.7f);
    g.setFont (bf (20.0f, true));
    g.drawText (juce::String::fromUTF8 ("\xe2\x80\xb9"), prevRect, juce::Justification::centred); g.drawText (juce::String::fromUTF8 ("\xe2\x80\xba"), nextRect, juce::Justification::centred);
    jlook::button (g, randRect.toFloat(), BC::magenta, false, randHover, "RANDOM", bf (11.0f, true));
    jlook::button (g, abRect.toFloat(), BC::amber, proc.ab.cur == 1, abHover, "MEMORIA " + proc.ab.label(), bf (11.0f, true));
    // meter verticali in/out
    auto meter = [&] (juce::Rectangle<float> r, float l, float rr2, const char* t)
    {
        g.setColour (juce::Colour (0xffe9e2cf).withAlpha (0.7f)); g.setFont (bf (8.5f, true, true)); g.drawText (t, r.removeFromBottom (12), juce::Justification::centred);
        inset (g, r.expanded (3), 4);
        for (int c = 0; c < 2; ++c)
        {
            auto b = r.removeFromLeft (r.getWidth() / (2 - c)).reduced (1.5f, 0);
            const float db = juce::Decibels::gainToDecibels (c ? rr2 : l, -48.0f), n = juce::jlimit (0.0f, 1.0f, (db + 48) / 51);
            for (int k = 0; k < 24; ++k) { const float t2 = k / 24.0f; g.setColour (t2 <= n ? (t2 > 0.9f ? juce::Colour (0xffff3b30) : t2 > 0.75f ? BC::amber : juce::Colour (0xffb5c94a)) : juce::Colours::white.withAlpha (0.05f)); g.fillRect (b.getX(), b.getBottom() - (k + 1) * b.getHeight() / 24 + 1, b.getWidth(), b.getHeight() / 24 - 1.5f); }
        }
    };
    meter ({ MX + 22.0f, 672, 36, 120 }, proc.inL.load(), proc.inR.load(), "IN");
    meter ({ MX + 180.0f, 672, 36, 120 }, proc.outL.load(), proc.outR.load(), "OUT");
}
void Editor::paintStatic (juce::Graphics& g)
{
    const float Wf = (float) W, Hf = (float) H;
    jlook::backdrop (g, { 0, 0, Wf, Hf }, BC::amber, BC::magenta);
    jlook::header (g, { 0, 0, Wf, 64 }, "BUTTER", BC::amber, "BTR-7  FORMA / PESO / SPAZIO", BC::magenta, BC::amber);
    for (int s = 0; s < 6; ++s) { const float x = (float) slotRect (s).getRight() + 4; juce::Path ar; ar.addArrow ({ x - 2, 96.0f, x + 6, 96.0f }, 1.5f, 6, 6); g.setColour (BC::amber.withAlpha (0.6f)); g.fillPath (ar); }
    auto mr = juce::Rectangle<float> ((float) MX, 78, 238, 726);
    hw::faceplate (g, mr, juce::Colour (0xff2a2a2e), juce::Colour (0xff121214), BC::amber, 34, 23, 14);
    g.setFont (bf (12.0f, true)); g.setColour (juce::Colour (0xffe9e2cf)); g.drawText ("MASTER", juce::Rectangle<float> (mr.getX() + 16, mr.getY() + 6, 150, 24), juce::Justification::centredLeft);
    g.setColour (juce::Colour (0xffe9e2cf).withAlpha (0.6f)); g.setFont (bf (9.0f));
    g.drawFittedText ("lucchetto = il modulo resta com'e' quando cambi preset\ntrascina la testata per riordinare\nlatenza " + juce::String (proc.getLatencySamples()) + " campioni, compensata",
                      juce::Rectangle<int> (MX + 64, 680, 110, 104), juce::Justification::centred, 8);
}

void Editor::mouseMove (const juce::MouseEvent& e0)
{
    const auto e = e0.withNewPosition (e0.position / scaler.s);
    const bool h = randRect.contains (e.getPosition()), h2 = abRect.contains (e.getPosition()), h3 = themeRect.contains (e.getPosition());
    if (h != randHover || h2 != abHover || h3 != themeHover) { randHover = h; abHover = h2; themeHover = h3; repaint(); }
    setMouseCursor (h || h2 || h3 || presetRect.contains (e.getPosition()) || prevRect.contains (e.getPosition()) || nextRect.contains (e.getPosition()) ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
}
void Editor::mouseDown (const juce::MouseEvent& e0)
{
    const auto e = e0.withNewPosition (e0.position / scaler.s);
    const int n = proc.getNumPrograms();
    if (themeRect.contains (e.getPosition())) { skin::cycle(); std::function<void (juce::Component*)> all = [&all] (juce::Component* c) { c->repaint(); for (auto* ch : c->getChildren()) all (ch); }; all (this); return; }
    if (abRect.contains (e.getPosition())) { proc.ab.toggle (proc); resized(); repaint(); }
    else if (randRect.contains (e.getPosition())) proc.randomize();
    else if (prevRect.contains (e.getPosition())) proc.loadPreset ((juce::jmax (0, proc.preset) - 1 + n) % n);
    else if (nextRect.contains (e.getPosition())) proc.loadPreset ((juce::jmax (0, proc.preset) + 1) % n);
    else if (presetRect.contains (e.getPosition())) browser.open();
    else if (false)
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

static juce::String presetNote (const juce::String& full)
{
    static const std::map<juce::String, juce::String> N = {
        { juce::String::fromUTF8 ("Init"), juce::String::fromUTF8 ("Tutto spento: il suono passa com'e'.") },
        { juce::String::fromUTF8 ("808 | Sub pieno e mono"), juce::String::fromUTF8 ("Nastro solo sui bassi e Pultec sui 60 Hz: sub piu' pieno, tenuto al centro.") },
        { juce::String::fromUTF8 ("808 | Si sente sul telefono"), juce::String::fromUTF8 ("Valvola sui medi e 73 che spinge le armoniche: l'808 si sente anche dalle casse piccole.") },
        { juce::String::fromUTF8 ("808 | Distorto trap"), juce::String::fromUTF8 ("Distorsore e clip a due stadi, poi MAXX: l'808 sporco e forte della trap.") },
        { juce::String::fromUTF8 ("808 | Drill scivolato"), juce::String::fromUTF8 ("Overdrive e Neve sui bassi, compressione SMACK: per gli 808 che scivolano.") },
        { juce::String::fromUTF8 ("808 | Boom rinforzato"), juce::String::fromUTF8 ("Aggiunge un sub sintonizzato a 45 Hz sotto l'808 e lo incolla.") },
        { juce::String::fromUTF8 ("808 | Punch sull'attacco"), juce::String::fromUTF8 ("Piu' botta sul click iniziale, saturazione solo sui transienti.") },
        { juce::String::fromUTF8 ("808 | Fuzz phonk"), juce::String::fromUTF8 ("Fuzz, clip e 73 spinto: l'808 rotto e saturo del phonk.") },
        { juce::String::fromUTF8 ("808 | Pulito per la cassa sopra"), juce::String::fromUTF8 ("Taglia un po' di coda e alleggerisce i 30 Hz: lascia spazio alla cassa.") },
        { juce::String::fromUTF8 ("808 | CHICCA Acid che urla"), juce::String::fromUTF8 ("Il boom diventa un filtro risonante: l'808 fischia come una 303.") },
        { juce::String::fromUTF8 ("808 | CHICCA Ottava sopra per i telefoni"), juce::String::fromUTF8 ("Aggiunge un'ottava sopra solo sui medi: l'808 resta sotto ma si sente ovunque.") },
        { juce::String::fromUTF8 ("Basso analog | Moog rotondo"), juce::String::fromUTF8 ("Nastro, compressione calda e Neve sui bassi: il basso Moog morbido e grosso.") },
        { juce::String::fromUTF8 ("Basso analog | Mini con morso"), juce::String::fromUTF8 ("Valvola e Pultec sotto e sopra: corpo e presenza da Minimoog.") },
        { juce::String::fromUTF8 ("Basso analog | Reese largo"), juce::String::fromUTF8 ("Overdrive, colla e un filo di stanza sopra i 400 Hz: Reese largo ma mono sotto.") },
        { juce::String::fromUTF8 ("Basso analog | Funk compresso"), juce::String::fromUTF8 ("SMACK con attacco in avanti e SSL sui medi: il basso funk che schiocca.") },
        { juce::String::fromUTF8 ("Basso analog | Sub sinusoide caldo"), juce::String::fromUTF8 ("Per i bassi sinusoidali: nastro sui bassi e Pultec, nient'altro.") },
        { juce::String::fromUTF8 ("Basso analog | CHICCA Formante che parla"), juce::String::fromUTF8 ("Formante giu' di 4 semitoni sui medi: il basso sembra dire 'uo'.") },
        { juce::String::fromUTF8 ("Basso digitale | FM pulito e deciso"), juce::String::fromUTF8 ("Compressione PUNCH e aria del Maag: FM chiaro senza saturare.") },
        { juce::String::fromUTF8 ("Basso digitale | Wavetable aggressivo"), juce::String::fromUTF8 ("Distorsore, OTT pieno e clip: il wavetable che sfonda.") },
        { juce::String::fromUTF8 ("Basso digitale | Growl"), juce::String::fromUTF8 ("Fuzz sui medi e OTT: growl largo sopra, sub pulito sotto.") },
        { juce::String::fromUTF8 ("Basso digitale | Bitcrush ma il sub resta"), juce::String::fromUTF8 ("Bitcrush solo sopra i 200 Hz: il sub non si rompe.") },
        { juce::String::fromUTF8 ("Basso digitale | Pluck con delay"), juce::String::fromUTF8 ("Delay a 1/8 sopra i 300 Hz e compressione PUNCH: pluck che rimbalza.") },
        { juce::String::fromUTF8 ("Basso digitale | CHICCA Wobble cassetta"), juce::String::fromUTF8 ("Il pitch ondeggia lento come una cassetta storta, sopra i 150 Hz.") },
        { juce::String::fromUTF8 ("Voce | Rap davanti"), juce::String::fromUTF8 ("PUNCH, console, aria e un plate corto: la voce rap in faccia.") },
        { juce::String::fromUTF8 ("Voce | Trap melodica"), juce::String::fromUTF8 ("OTT leggero, nastro, aria, delay a 1/8T e plate: voce trap con autotune.") },
        { juce::String::fromUTF8 ("Voce | Drill scura"), juce::String::fromUTF8 ("SMACK, valvola sui medi e tono scuro: la voce drill cupa.") },
        { juce::String::fromUTF8 ("Voce | Ritornello largo"), juce::String::fromUTF8 ("Colla, delay a 1/4, hall e allargamento: il ritornello che si apre.") },
        { juce::String::fromUTF8 ("Voce | Intima asciutta"), juce::String::fromUTF8 ("Compressione morbida e una stanza piccolissima: voce vicina, quasi parlata.") },
        { juce::String::fromUTF8 ("Voce | Pultec aria e corpo"), juce::String::fromUTF8 ("Il trucco del Pultec: 100 Hz e 12k insieme, corpo e aria senza durezza.") },
        { juce::String::fromUTF8 ("Voce | Adlib filtrati"), juce::String::fromUTF8 ("Voce da radio con delay e allargamento: per adlib e doppie.") },
        { juce::String::fromUTF8 ("Voce | Plate anni 80"), juce::String::fromUTF8 ("Plate lungo con predelay di 40 ms: la voce resta davanti, la coda dietro.") },
        { juce::String::fromUTF8 ("Voce | Slap delay"), juce::String::fromUTF8 ("Eco corto unico (slap) e un plate: voce rock'n'roll o rap anni 90.") },
        { juce::String::fromUTF8 ("Voce | CHICCA Coro all'ottava sulle code"), juce::String::fromUTF8 ("Un'ottava sopra solo sulle code delle frasi, con hall: coro fantasma.") },
        { juce::String::fromUTF8 ("Voce | CHICCA Mostro all'ottava sotto"), juce::String::fromUTF8 ("Ottava sotto con formante spostata: la voce del mostro.") },
        { juce::String::fromUTF8 ("Voce | CHICCA Telefono dal futuro"), juce::String::fromUTF8 ("Mono, digitale, filtrata e con eco: una chiamata da un altro pianeta.") },
        { juce::String::fromUTF8 ("Voce | CHICCA Molla dub"), juce::String::fromUTF8 ("Molla e delay lunghi con taglio dei bassi: voce dub da sound system.") },
        { juce::String::fromUTF8 ("Master | Bus pronto (MAXX)"), juce::String::fromUTF8 ("Colla, console, Pultec, SSL e MAXX: il bus pronto da far sentire.") },
        { juce::String::fromUTF8 ("Master | Trap moderna"), juce::String::fromUTF8 ("Boom e punch sotto, aria del Maag, mono sui bassi, MAXX.") },
        { juce::String::fromUTF8 ("Master | Boom bap pieno"), juce::String::fromUTF8 ("Compressione calda, 4 track e 73: il beat boom bap grosso.") },
        { juce::String::fromUTF8 ("Master | Pultec e colla"), juce::String::fromUTF8 ("Solo Pultec e un filo di colla: il master classico da studio.") },
        { juce::String::fromUTF8 ("Master | Neve calda"), juce::String::fromUTF8 ("Neve sotto e sui medi, colla leggera: calore senza perdere aria.") },
        { juce::String::fromUTF8 ("Master | Loud e pulito"), juce::String::fromUTF8 ("Colla e clip leggero prima del MAXX: forte senza sporcare.") },
        { juce::String::fromUTF8 ("Master | Drill scuro"), juce::String::fromUTF8 ("Tono scuro, SMACK leggero, valvola: il master drill cupo.") },
        { juce::String::fromUTF8 ("Master | CHICCA Largo sopra, mono sotto"), juce::String::fromUTF8 ("Allarga i medi e gli alti, mono sotto i 140 Hz: grande ma sicuro in club.") },
        { juce::String::fromUTF8 ("Master | CHICCA Solo i transienti in avanti"), juce::String::fromUTF8 ("Piu' attacco, meno coda, saturazione solo sui colpi: il beat respira.") },
        { juce::String::fromUTF8 ("Batteria | Trap knock"), juce::String::fromUTF8 ("Boom a 50 Hz, PUNCH e clip: la cassa trap che bussa.") },
        { juce::String::fromUTF8 ("Batteria | Drill crunch"), juce::String::fromUTF8 ("SMACK, digitale e clip a tre stadi: la batteria drill croccante.") },
        { juce::String::fromUTF8 ("Batteria | Glue leggera"), juce::String::fromUTF8 ("Colla, nastro e un po' d'aria: incolla il bus senza cambiarlo.") },
        { juce::String::fromUTF8 ("Batteria | OTT sparkle"), juce::String::fromUTF8 ("OTT su tutte le bande con aria: hat e rullante che brillano.") },
        { juce::String::fromUTF8 ("Batteria | Parallel smack"), juce::String::fromUTF8 ("SMACK a fondo, mescolato a meta' col dritto: il parallelo classico.") },
        { juce::String::fromUTF8 ("Batteria | Punch moderno"), juce::String::fromUTF8 ("PUNCH, valvola, attacco in avanti e un filo di boom.") },
        { juce::String::fromUTF8 ("Batteria | Pultec completa"), juce::String::fromUTF8 ("Pultec sotto e sopra, colla e nastro: batteria piena e aperta.") },
        { juce::String::fromUTF8 ("Batteria | Cassa che sfonda"), juce::String::fromUTF8 ("Boom a 48 Hz corto con punch, PUNCH e MAXX: cassa singola.") },
        { juce::String::fromUTF8 ("Batteria | Rullante crack"), juce::String::fromUTF8 ("SMACK, console solo sui transienti e aria: lo schiocco del rullante.") },
        { juce::String::fromUTF8 ("Batteria | Rullante grosso (formante giu)"), juce::String::fromUTF8 ("Formante giu' di 5 semitoni sui medi: il rullante sembra piu' grande.") },
        { juce::String::fromUTF8 ("Batteria | Hat corti e secchi"), juce::String::fromUTF8 ("Toglie coda, aggiunge attacco e brillantezza: hat stretti.") },
        { juce::String::fromUTF8 ("Batteria | CHICCA Riverbero solo sulle code"), juce::String::fromUTF8 ("Il plate si abbassa mentre suona la batteria e torna nelle pause.") },
        { juce::String::fromUTF8 ("EQ | Maag aria sul bus"), juce::String::fromUTF8 ("Solo il Maag: aria a 20k e un filo di sotto.") },
        { juce::String::fromUTF8 ("EQ | 73 in faccia"), juce::String::fromUTF8 ("Il 73 spinto: bassi, medi e presenza col colore del trasformatore.") },
        { juce::String::fromUTF8 ("EQ | Lindell 81 scolpito"), juce::String::fromUTF8 ("Scava i medi bassi e apre sopra, stile 81.") },
        { juce::String::fromUTF8 ("Chitarra | Elettrica crunch"), juce::String::fromUTF8 ("Valvola spinta, compressione calda e stanza.") },
        { juce::String::fromUTF8 ("Chitarra | Clean con delay"), juce::String::fromUTF8 ("Delay a 1/4, hall e compressione morbida per chitarre pulite.") },
    };
    const auto it = N.find (full); return it != N.end() ? it->second : juce::String();
}
// note dei gruppi nella scheda preset
static juce::String groupNote (const juce::String& g, const juce::String& name)
{
    juce::String n;
    if (g == "808") n = "Per 808 e sub: il mono sotto tiene i bassi al centro, la saturazione li fa sentire anche sulle casse piccole.";
    else if (g == "Basso analog") n = "Bassi da synth analogico (Moog, Mini, Reese): calore, morso e corpo senza perdere il sub.";
    else if (g == "Basso digitale") n = "Bassi FM e wavetable: OTT, distorsione e crush lasciando il sub pulito.";
    else if (g == "Voce") n = "Catene per voce rap, trap e drill: compressione, aria, plate e delay a tempo.";
    else if (g == "Master") n = "Sul bus master o sul beat intero: colla, Pultec, EQ analogico e limitatore.";
    else if (g == "Batteria") n = "Bus batteria e singoli pezzi: knock, crack, transienti e code.";
    else if (g == "EQ") n = "Solo l'EQ analogico, per partire da un colore.";
    else if (g == "Chitarra") n = "Per chitarre e strumenti suonati.";
    else n = "Tutto spento: il suono passa com'e'.";
    if (name.startsWith ("CHICCA")) n = "Chicca: un trucco da studio. " + n;
    return n;
}
void Editor::setupBrowser()
{
    addChildComponent (browser);
    browser.accent = BC::amber;
    auto userFiles = [] { auto f = Processor::presetFolder().findChildFiles (juce::File::findFiles, false, "*.jbutter"); f.sort(); return f; };
    browser.entries = [userFiles]
    {
        std::vector<jbrowse::Entry> v; const auto& P = burroPresets();
        for (int i = 0; i < (int) P.size(); ++i)
        {
            const juce::String n (P[(size_t) i].name);
            jbrowse::Entry e; e.id = i;
            e.group = n.contains (" | ") ? n.upToFirstOccurrenceOf (" | ", false, false) : juce::String ("BASE");
            e.name = n.contains (" | ") ? n.fromFirstOccurrenceOf (" | ", false, false) : n;
            e.note = presetNote (n); if (e.note.isEmpty()) e.note = groupNote (e.group, e.name); else if (e.name.startsWith ("CHICCA")) e.note = "Chicca. " + e.note;
            v.push_back (e);
        }
        const auto fs = userFiles();
        for (int i = 0; i < fs.size(); ++i) { jbrowse::Entry e; e.id = 100000 + i; e.user = true; e.group = "I MIEI"; e.name = fs[i].getFileNameWithoutExtension(); e.note = "Preset tuo, salvato in Musica / Jackpot / Butter Preset."; v.push_back (e); }
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
        if (e.user) proc.loadUserPreset (Processor::presetFolder().getChildFile (e.name + ".jbutter")); else proc.loadPreset (e.id);
        layoutModules(); repaint();
    };
    browser.save = [this] { askPresetName(); };
    browser.canOverwrite = [this] { return proc.userName.isNotEmpty(); };
    browser.overwrite = [this] { proc.saveUserPreset (proc.userName); repaint(); };
    browser.folder = [] { Processor::presetFolder().startAsProcess(); };
    browser.remove = [this] (const jbrowse::Entry& e) { Processor::presetFolder().getChildFile (e.name + ".jbutter").moveToTrash(); if (proc.userName == e.name) proc.userName = {}; repaint(); };
    browser.onClose = [this] { repaint(); };
}
void Editor::askPresetName()
{
    auto* w = new juce::AlertWindow ("Salva preset", "Nome del preset (finisce in Musica / Jackpot / Butter Preset):", juce::MessageBoxIconType::NoIcon, this);
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
    // si ridisegna solo cio' che cambia: display dei moduli accesi, manopole mosse, analizzatori, meter
    static int tick = 0; ++tick;
    if (browser.isVisible()) return;
    auto knobDirty = [] (PKnob* k) { if (k->changed()) k->repaint(); };
    if (eq->big) { eq->view.tick(); return; }   // EQ a tutto schermo: sotto non si ridisegna niente
    if (tick % 3 == 0 && imager->on->param->getValue() > 0.5f) imager->repaint (juce::Rectangle<int> (12, 38, imager->getWidth() - 22, 82));
    for (auto* m : modules)
    {
        const bool on = m->power->param->getValue() > 0.5f;
        if (on) m->repaint (m->displayRect().toNearestInt().expanded (2));
        if (on != m->lastOn) { m->lastOn = on; m->repaint(); }
        if (m->big) knobDirty (m->big.get()); for (auto* k : m->smalls) knobDirty (k);
        for (juce::Component* c : std::initializer_list<juce::Component*> { m->type.get(), m->extraPill.get(), m->focus.get(), m->target.get(), m->extra.get(), m->power.get() }) if (c) c->repaint();
    }
    for (auto* k : master) knobDirty (k);
    { eq->view.tick(); const float eo = proc.apvts.getRawParameterValue ("u_eqOn")->load(); if (eo != eq->lastOn) { eq->lastOn = eo; eq->repaint(); eq->on->repaint(); } }
    { bool any = false; for (auto* k : pultec->knobs) if (k->changed()) { k->repaint(); any = true; } if (any || pultec->on->param->getValue() != pultec->lastOn) { pultec->lastOn = pultec->on->param->getValue(); pultec->repaint(); } }
    for (auto* k : { imager->width.get(), imager->mono.get() }) knobDirty (k);
    if (imager->on->param->getValue() != imager->lastOn) { imager->lastOn = imager->on->param->getValue(); imager->repaint(); }
    { const float c = clip->param->getValue(), l = (float) limType->param->getIndex(); if (c != lastClip || l != lastLim) { lastClip = c; lastLim = l; clip->repaint(); limType->repaint(); } }
    const auto pt = proc.currentName() + juce::String (proc.ab.cur);
    if (pt != lastPresetTxt) { lastPresetTxt = pt; repaint (scaler.map (presetRect.getUnion (abRect).expanded (4))); layoutModules(); }
    repaint (scaler.map ({ MX + 16, 664, 48, 136 })); repaint (scaler.map ({ MX + 174, 664, 48, 136 }));
}
}
