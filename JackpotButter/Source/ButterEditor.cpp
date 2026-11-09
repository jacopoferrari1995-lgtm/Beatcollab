#include "ButterEditor.h"

namespace jb
{
juce::Font bf (float size, bool bold) { auto o = juce::FontOptions (size); if (bold) o = o.withStyle ("Bold"); return juce::Font (o); }
static const char* kModNames[6] = { "BOOM / NOISE", "SHIFT", "COMPRESS", "SATURATE", "SPACE", "CLIP" };
static juce::String pct (float v) { return juce::String (juce::roundToInt (v * 100)) + "%"; }
static juce::String bip (float v) { return std::abs (v) < 0.01f ? juce::String ("0") : (v > 0 ? "+" : "") + juce::String (juce::roundToInt (v * 100)); }

// ======================= controlli =======================
void PKnob::paint (juce::Graphics& g)
{
    const float lh = isBig ? 30.0f : 26.0f;
    const float size = juce::jmin ((float) getWidth(), getHeight() - lh);
    const float cx = getWidth() * 0.5f, cy = size * 0.5f + 1, R = size * 0.5f - 3, r = R - (isBig ? 9.0f : 6.0f);
    const float v = param->getValue(), a0 = -2.36f, a1 = 2.36f;
    // tacche
    for (int i = 0; i <= (isBig ? 20 : 10); ++i)
    {
        const float t = i / (isBig ? 20.0f : 10.0f), a = a0 + t * (a1 - a0);
        g.setColour (BC::ink.withAlpha (t <= v + 0.001f ? 0.55f : 0.18f));
        g.drawLine (cx + std::sin (a) * (R - 3), cy - std::cos (a) * (R - 3), cx + std::sin (a) * (R + 1), cy - std::cos (a) * (R + 1), isBig ? 2.0f : 1.4f);
    }
    // ombra e corpo colorato
    g.setColour (juce::Colours::black.withAlpha (0.22f)); g.fillEllipse (cx - r + 1, cy - r + 4, r * 2, r * 2);
    juce::ColourGradient body (col.brighter (0.35f), cx - r * 0.5f, cy - r, col.darker (0.25f), cx + r * 0.5f, cy + r, false);
    g.setGradientFill (body); g.fillEllipse (cx - r, cy - r, r * 2, r * 2);
    g.setColour (juce::Colours::white.withAlpha (0.35f)); g.drawEllipse (cx - r + 1, cy - r + 1, r * 2 - 2, r * 2 - 2, 1.2f);
    const float r2 = r * 0.62f;
    juce::ColourGradient cap (col.brighter (0.5f), cx, cy - r2, col, cx, cy + r2, false);
    g.setGradientFill (cap); g.fillEllipse (cx - r2, cy - r2, r2 * 2, r2 * 2);
    const float a = a0 + v * (a1 - a0);
    g.setColour (juce::Colours::white); g.drawLine (cx + std::sin (a) * r2 * 0.2f, cy - std::cos (a) * r2 * 0.2f, cx + std::sin (a) * r * 0.9f, cy - std::cos (a) * r * 0.9f, isBig ? 4.0f : 2.6f);
    // testo
    g.setColour (BC::ink); g.setFont (bf (isBig ? 12.0f : 10.0f, true)); g.drawText (label, juce::Rectangle<float> (0, size + 1, (float) getWidth(), 13), juce::Justification::centred);
    g.setColour (BC::inkSoft); g.setFont (bf (isBig ? 11.0f : 9.5f));
    g.drawText (fmt ? fmt (param->convertFrom0to1 (v)) : param->getCurrentValueAsText(), juce::Rectangle<float> (0, size + 14, (float) getWidth(), 13), juce::Justification::centred);
}

void Pill::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (1);
    g.setColour (BC::ink); g.fillRoundedRectangle (b, b.getHeight() / 2);
    g.setColour (col.brighter (0.3f)); g.setFont (bf (11.0f, true)); g.drawText (param->getCurrentChoiceName(), b, juce::Justification::centred);
    juce::Path t; t.addTriangle (b.getRight() - 14, b.getCentreY() - 2, b.getRight() - 8, b.getCentreY() - 2, b.getRight() - 11, b.getCentreY() + 2); g.fillPath (t);
}
void Pill::mouseDown (const juce::MouseEvent&)
{
    juce::PopupMenu m; for (int i = 0; i < param->choices.size(); ++i) m.addItem (i + 1, param->choices[i], true, i == param->getIndex());
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [this] (int r) { if (r > 0) param->setValueNotifyingHost (param->convertTo0to1 ((float) (r - 1))); });
}

void Segs::paint (juce::Graphics& g)
{
    const int n = param->choices.size(); const float w = getWidth() / (float) n; const int cur = param->getIndex();
    auto b = getLocalBounds().toFloat();
    g.setColour (BC::ink.withAlpha (0.12f)); g.fillRoundedRectangle (b, 5);
    for (int i = 0; i < n; ++i)
    {
        auto r = juce::Rectangle<float> (i * w, 0, w, b.getHeight()).reduced (1.5f);
        if (i == cur) { g.setColour (BC::ink); g.fillRoundedRectangle (r, 4); }
        g.setColour (i == cur ? col.brighter (0.3f) : BC::inkSoft); g.setFont (bf (9.0f, true));
        g.drawText (param->choices[i].substring (0, 1) == "F" ? "ALL" : param->choices[i].substring (0, 1), r, juce::Justification::centred);
    }
}

void Target::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setFont (bf (8.5f, true)); g.setColour (BC::inkSoft);
    g.drawText ("SUST", b.withHeight (10), juce::Justification::centredLeft); g.drawText ("TARGET", b.withHeight (10), juce::Justification::centred); g.drawText ("TRANS", b.withHeight (10), juce::Justification::centredRight);
    auto tr = juce::Rectangle<float> (8, b.getHeight() - 9, b.getWidth() - 16, 4);
    g.setColour (BC::ink.withAlpha (0.15f)); g.fillRoundedRectangle (tr, 2);
    const float v = param->getValue(), x = tr.getX() + v * tr.getWidth();
    g.setColour (col); g.fillRoundedRectangle (juce::jmin (x, tr.getCentreX()), tr.getY(), std::abs (x - tr.getCentreX()), tr.getHeight(), 2);
    g.setColour (BC::ink); g.drawVerticalLine ((int) tr.getCentreX(), tr.getY() - 3, tr.getBottom() + 3);
    g.setColour (juce::Colours::white); g.fillEllipse (x - 6, tr.getCentreY() - 6, 12, 12); g.setColour (BC::ink); g.drawEllipse (x - 6, tr.getCentreY() - 6, 12, 12, 1.5f);
}

void Switch::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat(); const bool on = param->getValue() > 0.5f;
    auto led = b.removeFromLeft (b.getHeight()).reduced (3);
    if (on) { g.setColour (col.withAlpha (0.35f)); g.fillEllipse (led.expanded (3)); }
    g.setColour (on ? col.brighter (0.2f) : BC::ink.withAlpha (0.2f)); g.fillEllipse (led);
    g.setColour (BC::ink.withAlpha (0.5f)); g.drawEllipse (led, 1.0f);
    if (label.isNotEmpty()) { g.setColour (BC::ink); g.setFont (bf (10.0f, true)); g.drawText (label, b.withTrimmedLeft (4), juce::Justification::centredLeft); }
}

// ======================= modulo =======================
Module::Module (Editor& e, int m) : ed (e), mod (m)
{
    auto& A = ed.proc.apvts; const auto c = BC::mod[m];
    auto P = [&A] (const char* id) { return A.getParameter (id); };
    auto C = [&A] (const char* id) { return dynamic_cast<juce::AudioParameterChoice*> (A.getParameter (id)); };
    auto addSmall = [this, c] (juce::RangedAudioParameter* p, const char* l, std::function<juce::String (float)> f) { addAndMakeVisible (smalls.add (new PKnob (p, l, c, false, std::move (f)))); };
    switch (m)
    {
        case 0:
            power = std::make_unique<Switch> (P ("u_bOn"), "", juce::Colour (0xfffffbe8));
            big = std::make_unique<PKnob> (P ("u_boom"), "BOOM", c, true, pct);
            addSmall (P ("u_bTone"), "FREQ", [] (float v) { return juce::String (juce::roundToInt (v)) + " Hz"; });
            addSmall (P ("u_noise"), "NOISE", pct);
            extra = std::make_unique<Switch> (P ("u_acid"), "ACID", c);
            extraPill = std::make_unique<Pill> (C ("u_nType"), c);
            break;
        case 1:
            power = std::make_unique<Switch> (P ("u_shOn"), "", juce::Colour (0xfffffbe8));
            big = std::make_unique<PKnob> (P ("u_shAmt"), "MIX", c, true, pct);
            addSmall (P ("u_shPitch"), "PITCH", [] (float v) { return (v > 0 ? "+" : "") + juce::String (v, 1) + " st"; });
            focus = std::make_unique<Segs> (C ("u_shFoc"), c); target = std::make_unique<Target> (P ("u_shTgt"), c);
            break;
        case 2:
            power = std::make_unique<Switch> (P ("u_cOn"), "", juce::Colour (0xfffffbe8));
            type = std::make_unique<Pill> (C ("u_cType"), c);
            big = std::make_unique<PKnob> (P ("u_cAmt"), "AMOUNT", c, true, [] (float v) { return std::abs (v) < 0.01f ? juce::String ("0") : (v > 0 ? juce::String ("GIU ") : juce::String ("SU ")) + juce::String (juce::roundToInt (std::abs (v) * 100)); });
            addSmall (P ("u_cBands"), "BANDE", pct);
            break;
        case 3:
            power = std::make_unique<Switch> (P ("u_sOn"), "", juce::Colour (0xfffffbe8));
            type = std::make_unique<Pill> (C ("u_sType"), c);
            big = std::make_unique<PKnob> (P ("u_sAmt"), "AMOUNT", c, true, pct);
            addSmall (P ("u_sStyle"), "STILE", pct);
            focus = std::make_unique<Segs> (C ("u_sFoc"), c); target = std::make_unique<Target> (P ("u_sTgt"), c);
            break;
        case 4:
            power = std::make_unique<Switch> (P ("u_spOn"), "", juce::Colour (0xfffffbe8));
            type = std::make_unique<Pill> (C ("u_spType"), c);
            big = std::make_unique<PKnob> (P ("u_spAmt"), "AMOUNT", c, true, pct);
            addSmall (P ("u_spDecay"), "DECAY", pct); addSmall (P ("u_spPre"), "PRE", [] (float v) { return juce::String (juce::roundToInt (v)) + " ms"; });
            addSmall (P ("u_spDuck"), "DUCK", pct); addSmall (P ("u_spWidth"), "WIDTH", pct);
            break;
        default:
            power = std::make_unique<Switch> (P ("u_mOn"), "", juce::Colour (0xfffffbe8));
            big = std::make_unique<PKnob> (P ("u_mAmt"), "DRIVE", c, true, pct);
            addSmall (P ("u_mTone"), "TONO", bip);
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
    power->setBounds (8, 10, 18, 18);
    if (type) type->setBounds (12, 42, w - 24, 22);
    big->setBounds (10, 70, w - 20, 128);
    const int sw = (w - 16) / 2;
    for (int i = 0; i < smalls.size(); ++i) smalls[i]->setBounds (8 + (i % 2) * sw, 206 + (i / 2) * 76, sw, 74);
    if (extra) extra->setBounds (14, 292, sw - 10, 22);
    if (extraPill) extraPill->setBounds (mod == 0 ? 8 + sw + 2 : 8 + sw + 4, mod == 0 ? 292 : 224, sw - 8, 22);
    if (focus) focus->setBounds (12, getHeight() - 74, w - 24, 20);
    if (target) target->setBounds (8, getHeight() - 46, w - 16, 26);
}

void Module::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (1);
    const auto c = BC::mod[mod]; const bool on = power->param->getValue() > 0.5f;
    g.setColour (juce::Colours::black.withAlpha (dragging ? 0.3f : 0.12f)); g.fillRoundedRectangle (b.translated (0, dragging ? 6.0f : 3.0f), 14);
    juce::ColourGradient bg (c.interpolatedWith (BC::cream, 0.78f), 0, b.getY(), c.interpolatedWith (BC::cream2, 0.7f), 0, b.getBottom(), false);
    g.setGradientFill (bg); g.fillRoundedRectangle (b, 14);
    g.setColour (c.withAlpha (0.55f)); g.drawRoundedRectangle (b, 14, 1.4f);
    // banda superiore
    juce::Path top; top.addRoundedRectangle (b.getX(), b.getY(), b.getWidth(), 36, 14, 14, true, true, false, false);
    g.setColour (c); g.fillPath (top);
    g.setColour (BC::ink); g.setFont (bf (12.5f, true)); g.drawText (kModNames[mod], juce::Rectangle<float> (30, 0, b.getWidth() - 62, 36), juce::Justification::centredLeft);
    // lucchetto
    const bool lk = ed.proc.locked[mod].load(); auto lr = lockRect().toFloat();
    g.setColour (BC::ink.withAlpha (lk ? 1.0f : 0.35f));
    g.drawRoundedRectangle (lr.getX() + 3, lr.getY() + 8, 12, 9, 2, 1.6f); if (lk) g.fillRoundedRectangle (lr.getX() + 3, lr.getY() + 8, 12, 9, 2);
    juce::Path arc; arc.addCentredArc (lr.getCentreX(), lr.getY() + 8, 4, 5, 0, -juce::MathConstants<float>::halfPi, lk ? juce::MathConstants<float>::halfPi : 0.3f, true);
    g.strokePath (arc, juce::PathStrokeType (1.6f));
    if (mod == 2)
    {
        const float gr = juce::jlimit (0.0f, 18.0f, ed.proc.dsp.grMeter.load()); auto mr = juce::Rectangle<float> (14, 300, b.getWidth() - 28, 6);
        g.setColour (BC::ink.withAlpha (0.12f)); g.fillRoundedRectangle (mr, 3); g.setColour (BC::ink); g.fillRoundedRectangle (mr.withLeft (mr.getRight() - mr.getWidth() * gr / 18), 3);
        g.setFont (bf (9.0f, true)); g.setColour (BC::inkSoft); g.drawText ("GR -" + juce::String (gr, 1) + " dB", juce::Rectangle<float> (14, 310, b.getWidth() - 28, 14), juce::Justification::centred);
    }
    if (mod >= 1 && mod <= 4) { g.setColour (BC::ink.withAlpha (0.3f)); g.setFont (bf (8.5f)); g.drawText ("trascina per spostare", juce::Rectangle<float> (0, b.getBottom() - 18, b.getWidth(), 14), juce::Justification::centred); }
    if (! on) { g.setColour (BC::cream.withAlpha (0.55f)); g.fillRoundedRectangle (b.withTrimmedTop (36), 14); }
}

void Module::mouseDown (const juce::MouseEvent& e)
{
    if (lockRect().contains (e.getPosition())) { ed.proc.locked[mod] = ! ed.proc.locked[mod].load(); repaint(); return; }
    dragging = mod >= 1 && mod <= 4 && e.position.y < 36; dragX = getX();
    if (dragging) toFront (false);
}
void Module::mouseDrag (const juce::MouseEvent& e) { if (dragging) { setTopLeftPosition (dragX + e.getDistanceFromDragStartX(), getY()); ed.moduleDragged (*this, getBounds().getCentreX()); } }
void Module::mouseUp (const juce::MouseEvent&) { if (dragging) { dragging = false; ed.moduleDropped (*this); } }

// ======================= editor =======================
static juce::Rectangle<int> slotRect (int s) { return { 20 + s * 158, 84, 150, 430 }; }

Editor::Editor (Processor& p) : AudioProcessorEditor (&p), proc (p)
{
    for (int m = 0; m < 6; ++m) addAndMakeVisible (modules.add (new Module (*this, m)));
    auto& A = proc.apvts; const auto mc = juce::Colour (0xffd9a441);
    master.add (new PKnob (A.getParameter ("u_tone"), "TONE", mc, false, bip));
    master.add (new PKnob (A.getParameter ("u_air"), "AIR", mc, false, pct));
    master.add (new PKnob (A.getParameter ("u_att"), "ATTACK", mc, false, bip));
    master.add (new PKnob (A.getParameter ("u_sus"), "SUSTAIN", mc, false, bip));
    master.add (new PKnob (A.getParameter ("u_mag"), "MAGNITUDE", BC::ink.brighter (0.25f), true, pct));
    master.add (new PKnob (A.getParameter ("u_in"), "INPUT", mc, false, [] (float v) { return juce::String (v, 1) + " dB"; }));
    master.add (new PKnob (A.getParameter ("u_out"), "OUTPUT", mc, false, [] (float v) { return juce::String (v, 1) + " dB"; }));
    for (auto* k : master) addAndMakeVisible (k);
    clip = std::make_unique<Switch> (A.getParameter ("u_clip"), "CLIP", juce::Colour (0xffe05a5a)); addAndMakeVisible (*clip);
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
    const int mx = 978;
    for (int i = 0; i < 4; ++i) master[i]->setBounds (mx + 6 + (i % 2) * 92, 116 + (i / 2) * 92, 86, 88);
    clip->setBounds (mx + 20, 304, 120, 22);
    master[4]->setBounds (mx + 24, 334, 136, 160);
    master[5]->setBounds (mx + 6, 520, 86, 88); master[6]->setBounds (mx + 98, 520, 86, 88);
    presetRect = { 420, 18, 340, 34 }; prevRect = { 384, 18, 34, 34 }; nextRect = { 762, 18, 34, 34 };
}

void Editor::moduleDragged (Module&, int) {}
void Editor::moduleDropped (Module& m)
{
    const int cx = m.getBounds().getCentreX();
    int slot = juce::jlimit (1, 4, (int) std::round ((cx - 20 - 75) / 158.0));
    int cur[4]; for (int i = 0; i < 4; ++i) cur[i] = proc.dsp.order[i].load();
    int from = 0; for (int i = 0; i < 4; ++i) if (cur[i] == m.mod) from = i;
    std::vector<int> v (cur, cur + 4); v.erase (v.begin() + from); v.insert (v.begin() + (slot - 1), m.mod);
    for (int i = 0; i < 4; ++i) proc.dsp.order[i] = v[(size_t) i];
    layoutModules(); repaint();
}

void Editor::paint (juce::Graphics& g)
{
    // chassis color burro
    juce::ColourGradient bg (BC::cream, 0, 0, BC::cream2, 0, (float) H, false); g.setGradientFill (bg); g.fillAll();
    juce::Random rr (3); g.setColour (BC::ink.withAlpha (0.03f)); for (int i = 0; i < 3000; ++i) g.fillRect (rr.nextFloat() * W, rr.nextFloat() * H, 1.0f, 1.0f);
    // intestazione
    g.setColour (BC::ink); g.fillRoundedRectangle (20, 16, 44, 38, 9);
    g.setColour (juce::Colour (0xffe2b93b)); g.setFont (bf (17.0f, true)); g.drawText ("JB", juce::Rectangle<int> (20, 16, 44, 38), juce::Justification::centred);
    g.setColour (BC::ink); g.setFont (bf (22.0f, true)); g.drawText ("JACKPOT", 76, 12, 120, 30, juce::Justification::centredLeft);
    g.setFont (bf (22.0f)); g.drawText ("BUTTER RACK", 170, 12, 180, 30, juce::Justification::centredLeft);
    g.setColour (BC::inkSoft); g.setFont (bf (10.0f)); g.drawText ("drum bus a 6 moduli  -  JB-6", 76, 38, 260, 16, juce::Justification::centredLeft);
    // preset
    g.setColour (BC::ink); g.fillRoundedRectangle (presetRect.toFloat(), 17);
    g.setColour (juce::Colour (0xffe2b93b)); g.setFont (bf (13.0f, true)); g.drawText (proc.getProgramName (proc.preset), presetRect, juce::Justification::centred);
    g.setColour (BC::ink); g.setFont (bf (18.0f, true));
    g.drawText (juce::String::fromUTF8 ("\xe2\x80\xb9"), prevRect, juce::Justification::centred); g.drawText (juce::String::fromUTF8 ("\xe2\x80\xba"), nextRect, juce::Justification::centred);
    // linea di catena tra i moduli
    g.setColour (BC::ink.withAlpha (0.25f));
    for (int s = 0; s < 5; ++s) { const float x = slotRect (s).getRight() + 4.0f; juce::Path ar; ar.addArrow ({ x - 3, 300.0f, x + 5, 300.0f }, 1.5f, 7, 6); g.fillPath (ar); }
    // master
    auto mr = juce::Rectangle<float> (978, 84, 182, 536);
    g.setColour (BC::ink.withAlpha (0.08f)); g.fillRoundedRectangle (mr.translated (0, 3), 14);
    g.setColour (juce::Colour (0xfffbf5e6)); g.fillRoundedRectangle (mr, 14); g.setColour (BC::line); g.drawRoundedRectangle (mr, 14, 1.4f);
    g.setColour (BC::ink); g.setFont (bf (12.5f, true)); g.drawText ("MASTER", mr.withHeight (30).toNearestInt(), juce::Justification::centred);
    g.setColour (BC::line); g.drawHorizontalLine (114, mr.getX() + 12, mr.getRight() - 12); g.drawHorizontalLine (510, mr.getX() + 12, mr.getRight() - 12);
    // meter in/out in basso
    auto meter = [&] (juce::Rectangle<float> r, float l, float rr2, const char* t)
    {
        g.setColour (BC::inkSoft); g.setFont (bf (9.0f, true)); g.drawText (t, r.removeFromLeft (30).toNearestInt(), juce::Justification::centredLeft);
        for (int c = 0; c < 2; ++c)
        {
            auto b = r.removeFromTop (r.getHeight() / (2 - c)).reduced (0, 1.5f);
            const float db = juce::Decibels::gainToDecibels (c ? rr2 : l, -48.0f), n = juce::jlimit (0.0f, 1.0f, (db + 48) / 51);
            g.setColour (BC::ink.withAlpha (0.1f)); g.fillRoundedRectangle (b, 2);
            g.setColour (n > 0.94f ? juce::Colour (0xffe05a5a) : BC::ink); g.fillRoundedRectangle (b.withWidth (b.getWidth() * n), 2);
        }
    };
    meter ({ 20, 530, 440, 16 }, proc.inL.load(), proc.inR.load(), "IN");
    meter ({ 20, 556, 440, 16 }, proc.outL.load(), proc.outR.load(), "OUT");
    g.setColour (BC::inkSoft); g.setFont (bf (10.0f));
    g.drawFittedText ("Ogni modulo ha un AMOUNT grande. Il lucchetto blocca il modulo quando cambi preset. I 4 moduli centrali si spostano trascinando la testata: l'ordine cambia il suono. TARGET sceglie se lavorare sui transienti o sulla coda, FOCUS su quale banda.",
                      juce::Rectangle<int> (480, 524, 480, 52), juce::Justification::topLeft, 3);
    g.setColour (BC::inkSoft.withAlpha (0.6f)); g.setFont (bf (9.0f)); g.drawText ("JACKPOT.WAV", juce::Rectangle<int> (20, 600, 200, 16), juce::Justification::centredLeft);
}

void Editor::mouseDown (const juce::MouseEvent& e)
{
    const int n = proc.getNumPrograms();
    if (prevRect.contains (e.getPosition())) proc.loadPreset ((proc.preset - 1 + n) % n);
    else if (nextRect.contains (e.getPosition())) proc.loadPreset ((proc.preset + 1) % n);
    else if (presetRect.contains (e.getPosition()))
    {
        juce::PopupMenu m; juce::String group; juce::PopupMenu sub; std::vector<std::pair<juce::String, juce::PopupMenu>> groups;
        for (int i = 0; i < n; ++i)
        {
            const auto name = proc.getProgramName (i); const auto g = name.upToFirstOccurrenceOf (" | ", false, false);
            if (! name.contains (" | ")) { m.addItem (i + 1, name, true, i == proc.preset); continue; }
            bool found = false; for (auto& gr : groups) if (gr.first == g) { gr.second.addItem (i + 1, name.fromFirstOccurrenceOf (" | ", false, false), true, i == proc.preset); found = true; }
            if (! found) { juce::PopupMenu pm; pm.addItem (i + 1, name.fromFirstOccurrenceOf (" | ", false, false), true, i == proc.preset); groups.push_back ({ g, pm }); }
        }
        for (auto& gr : groups) m.addSubMenu (gr.first, gr.second);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea (localAreaToGlobal (presetRect)), [this] (int r) { if (r > 0) { proc.loadPreset (r - 1); repaint(); } });
    }
    repaint();
}

void Editor::timerCallback()
{
    for (auto* m : modules) m->repaint();
    for (auto* k : master) k->repaint();
    clip->repaint();
    repaint (0, 520, 470, 60); repaint (presetRect);
}
}
