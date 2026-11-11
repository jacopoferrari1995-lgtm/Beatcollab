#include "MixEditor.h"

namespace jmx
{
using namespace hw;
static const juce::Colour kBg (0xff121211), kTxt (0xffe9e4d8), kDim (0xff8f897c), kLine (0xff34322e), kAmber (0xffffb347), kBlue (0xff4dabf7), kRed (0xffff5a4e), kGreen (0xff7dde5a);
static juce::Colour bandCol (int b) { static const juce::uint32 c[kBands] = { 0xffff6b6b, 0xffffa94d, 0xffffd43b, 0xffa9e34b, 0xff51cf66, 0xff38d9a9, 0xff3bc9db, 0xff4dabf7, 0xff748ffc, 0xffb197fc, 0xfff783ac, 0xffe599f7 }; return juce::Colour (c[b % kBands]); }
static juce::String hzT (float f) { return f >= 1000 ? juce::String (f / 1000.0f, f >= 10000 ? 1 : 2) + " kHz" : juce::String (juce::roundToInt (f)) + " Hz"; }
static void glassBox (juce::Graphics& g, juce::Rectangle<float> r, float rad = 6)
{   // cornice di uno strumento da misura: metallo scuro, vetro nero
    g.setColour (juce::Colours::black.withAlpha (0.6f)); g.fillRoundedRectangle (r.translated (0, 2), rad);
    juce::ColourGradient fr (juce::Colour (0xff2c2b29), r.getX(), r.getY(), juce::Colour (0xff181716), r.getX(), r.getBottom(), false); g.setGradientFill (fr); g.fillRoundedRectangle (r, rad);
    auto in = r.reduced (5);
    g.setColour (juce::Colour (0xff070707)); g.fillRoundedRectangle (in, rad - 2);
    g.setColour (juce::Colours::white.withAlpha (0.07f)); g.drawRoundedRectangle (r.reduced (0.5f), rad, 1.0f);
}
static void pill (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& t, bool on, juce::Colour c, float fs = 10.0f)
{
    juce::ColourGradient bg (on ? c.withAlpha (0.35f) : juce::Colour (0xff2e2d2a), r.getX(), r.getY(), on ? c.withAlpha (0.18f) : juce::Colour (0xff1f1e1c), r.getX(), r.getBottom(), false);
    g.setGradientFill (bg); g.fillRoundedRectangle (r, 4);
    g.setColour (on ? c : juce::Colour (0xff4a4843)); g.drawRoundedRectangle (r.reduced (0.5f), 4, on ? 1.4f : 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.06f)); g.drawHorizontalLine ((int) r.getY() + 1, r.getX() + 3, r.getRight() - 3);
    g.setFont (alook::serif (fs)); g.setColour (on ? c.brighter (0.3f) : kTxt.withAlpha (0.85f)); g.drawText (t, r, juce::Justification::centred);
}

// ======================================================================= aspetto di ogni hardware
Design designOf (int type)
{
    Design D; const auto& m = mod (type); const juce::String n = m.name;
    D.plate = juce::Colour (m.look.plate); D.accent = juce::Colour (m.look.accent); D.meter = m.look.meter;
    auto byLabel = [] (std::function<int (const juce::String&, const PSpec&, int)> f) { return [f] (int i, const PSpec& s) { return f (juce::String (s.label), s, i); }; };
    switch (m.cat)
    {
        case C_PRE:
            D.face = FACE_PAINT; D.meter = 2;
            if (n == "73 PRE" || n == "81 PRE" || n == "1272 PRE")
                D.pick = byLabel ([] (const juce::String& l, const PSpec&, int) { return l == "GAIN" ? KS_NEVE_RED : l == "LOW CUT" ? KS_NEVE_BLUE : KS_NEVE_GREY; });
            else if (n == "SSL E PRE") { D.plate = juce::Colour (0xff3a3d40); D.pick = byLabel ([] (const juce::String& l, const PSpec&, int) { return l == "GAIN" ? KS_SSL_RED : l == "LOW CUT" ? KS_SSL_BLUE : KS_SSL_WHITE; }); }
            else if (n == "610 PRE") { D.face = FACE_BLACK; D.pick = byLabel ([] (const juce::String& l, const PSpec&, int) { return l == "GAIN" ? KS_CHICKEN : KS_DAVIES; }); }
            else if (n == "737 PRE") { D.face = FACE_BLACK; D.knob = KS_SILVER; D.ink = juce::Colour (0xffd9b45c); }
            else if (n == "TG2 PRE") { D.knob = KS_CHICKEN; }
            else if (n == "TRIDENT PRE") D.pick = byLabel ([] (const juce::String& l, const PSpec&, int) { return l == "GAIN" ? KS_SSL_RED : KS_SSL_BLUE; });
            else if (n == "API 312 PRE") { D.plate = juce::Colour (0xff2a2d31); D.knob = KS_FET; }
            else D.knob = KS_DAVIES;
            break;
        case C_EQ:
            D.face = FACE_PAINT; D.meter = 0;
            if (n == "EQP-1A" || n == "MEQ-5") { D.plate = juce::Colour (0xff3f5468); D.pick = [] (int, const PSpec& s) { return s.fmt == F_ITEMS ? KS_CHICKEN : KS_BIGBLACK; }; }
            else if (n == "PE 1C") { D.face = FACE_CREAM; D.plate = juce::Colour (0xffcfc6b0); D.pick = [] (int, const PSpec& s) { return s.fmt == F_ITEMS ? KS_CHICKEN : KS_BIGBLACK; }; }
            else if (n == "SSL E") { D.plate = juce::Colour (0xff3a3d40); D.pick = [] (int i, const PSpec&) { static const int c[4] = { KS_SSL_YELLOW, KS_SSL_BLUE, KS_SSL_GREEN, KS_SSL_RED }; return i < 8 ? c[i % 4] : KS_SSL_WHITE; }; }
            else if (n == "MAAG EQ4") { D.face = FACE_BLACK; D.knob = KS_SILVER; }
            else D.pick = [] (int i, const PSpec&) { return i < 4 ? KS_NEVE_GREY : i < 8 ? KS_NEVE_BLUE : KS_NEVE_RED; };
            break;
        case C_DEESS: D.face = FACE_BLACK; D.knob = n == "DS 902" ? KS_DAVIES : KS_SILVER; D.meter = 3; break;
        case C_SAT:
            if (n == "TUBE BOX") { D.face = FACE_PAINT; D.knob = KS_DAVIES; }
            else if (n == "MB SAT") { D.face = FACE_BLACK; D.pick = byLabel ([] (const juce::String& l, const PSpec&, int) { return l.contains ("BASSI") ? KS_SSL_YELLOW : l.contains ("MEDI") ? KS_SSL_GREEN : l.contains ("ALTI") ? KS_SSL_RED : KS_SSL_WHITE; }); }
            else { D.face = FACE_BLACK; D.knob = KS_SILVER; }
            D.meter = 2; break;
        case C_COMP:
            D.meter = 1;
            if (n == "OPTO 2A")      { D.face = FACE_BRUSHED; D.plate = juce::Colour (0xffbab9b3); D.knob = KS_BIGBLACK; }
            else if (n == "OPTO 3A") { D.face = FACE_BRUSHED; D.plate = juce::Colour (0xff9c9c97); D.knob = KS_BIGBLACK; }
            else if (n == "FET 76")  { D.face = FACE_BLACK; D.plate = juce::Colour (0xff1b1b1c); D.knob = KS_FET; }
            else if (n == "BUS VCA") { D.face = FACE_PAINT; D.plate = juce::Colour (0xff3a3d40); D.pick = byLabel ([] (const juce::String& l, const PSpec&, int i) { return i == 0 ? KS_SSL_RED : l.contains ("RATIO") ? KS_SSL_BLUE : KS_SSL_WHITE; }); }
            else if (n == "VARI-MU") { D.face = FACE_BRUSHED; D.plate = juce::Colour (0xffa4b0bb); D.pick = [] (int, const PSpec& s) { return s.fmt == F_ITEMS ? KS_CHICKEN : KS_SILVER; }; }
            else if (n == "RMS 160") { D.face = FACE_BRUSHED; D.plate = juce::Colour (0xffc4c4c0); D.knob = KS_DAVIES; }
            else if (n == "VARI 670"){ D.face = FACE_PAINT; D.plate = juce::Colour (0xff8a8d88); D.pick = [] (int, const PSpec& s) { return s.fmt == F_ITEMS ? KS_CHICKEN : KS_BIGBLACK; }; }
            else if (n == "OTT")     { D.face = FACE_BLACK; D.pick = [] (int i, const PSpec&) { return i == 0 ? KS_SSL_RED : KS_SSL_WHITE; }; D.meter = 3; }
            else { D.face = FACE_BLACK; D.knob = KS_SILVER; }
            break;
        case C_CLIP: D.face = FACE_BLACK; D.knob = KS_SILVER; D.meter = 3; break;
        case C_REV:
            D.meter = 0;
            if (n == "PLATE 140" || n == "SPRING") { D.face = FACE_CREAM; D.plate = juce::Colour (n == "SPRING" ? 0xffcbd2c0 : 0xffe2d8c0); D.knob = KS_IVORY; }
            else if (n == "HALL 480") { D.face = FACE_CREAM; D.plate = juce::Colour (0xffd8d2c2); D.knob = KS_SILVER; }
            else if (n == "ROOM") { D.face = FACE_WOOD; D.plate = juce::Colour (0xff6a4628); D.knob = KS_DAVIES; }
            else if (n == "CHAMBER") { D.face = FACE_PAINT; D.knob = KS_DAVIES; }
            else if (n == "CONVOLUTION") { D.face = FACE_BRUSHED; D.plate = juce::Colour (0xff9fa4a8); D.knob = KS_SILVER; }
            else { D.face = FACE_BLACK; D.knob = KS_SILVER; }
            break;
        case C_DLY:
            D.meter = 0;
            if (n == "TAPE ECHO") { D.face = FACE_BLACK; D.plate = juce::Colour (0xff1f2a20); D.knob = KS_DAVIES; }
            else if (n == "BBD ECHO") { D.face = FACE_PEDAL; D.pedal = true; D.plate = juce::Colour (0xffb9c0c7); D.knob = KS_PEDAL; }
            else if (n == "DUB DELAY") { D.face = FACE_PAINT; D.knob = KS_CHICKEN; }
            else if (n == "SLAPBACK") { D.face = FACE_CREAM; D.plate = juce::Colour (0xffd8c39c); D.knob = KS_IVORY; }
            else if (n == "PING PONG") { D.face = FACE_BLACK; D.knob = KS_SSL_BLUE; }
            else { D.face = FACE_BLACK; D.knob = KS_SILVER; }
            break;
        case C_DIST:
            D.meter = 0;
            if (n == "AMP CRUNCH") { D.face = FACE_BLACK; D.plate = juce::Colour (0xff1c1a18); D.knob = KS_SSL_YELLOW; D.ink = juce::Colour (0xffd9b45c); }
            else if (n == "WAVEFOLDER" || n == "HARD CLIP") { D.face = FACE_BLACK; D.knob = KS_SILVER; }
            else if (n == "LO-FI") { D.face = FACE_BRUSHED; D.plate = juce::Colour (0xffa8adb5); D.knob = KS_DAVIES; }
            else if (n == "RADIO") { D.face = FACE_CREAM; D.plate = juce::Colour (0xffd9c9a3); D.knob = KS_IVORY; }
            else { D.face = FACE_PEDAL; D.pedal = true; D.knob = KS_PEDAL; }
            break;
        case C_MOD:
            D.meter = 0;
            if (n == "CHORUS CE-1" || n == "FLANGER" || n == "PHASER 90" || n == "VIBRATO") { D.face = FACE_PEDAL; D.pedal = true; D.knob = KS_PEDAL; }
            else if (n == "DIMENSION D") { D.face = FACE_BLACK; D.knob = KS_SSL_WHITE; }
            else { D.face = FACE_BLACK; D.knob = KS_SILVER; }
            break;
        case C_TAPE: D.face = FACE_BRUSHED; D.plate = juce::Colour (0xffb9b6ad); D.knob = KS_DAVIES; D.meter = 2; break;
        case C_DYN: D.face = n == "GATE" ? FACE_PAINT : FACE_BLACK; D.knob = n == "GATE" ? KS_DAVIES : KS_SSL_WHITE; D.meter = 3; break;
        default: D.face = FACE_BLACK; D.knob = KS_SILVER; D.meter = 0; break;
    }
    if (D.ink == juce::Colour (0xfff0ece2) || D.ink.isTransparent())
        D.ink = D.plate.getPerceivedBrightness() > 0.5f ? juce::Colour (0xff1c1a17) : juce::Colour (0xfff0ece2);
    return D;
}

// ======================================================================= modulo
static int visibleCount (int t) { int c = 0; for (int i = 0; i < kP; ++i) if (! hidden (mod (t).spec[(size_t) i])) ++c; return c; }
static constexpr float kTop = 58, kMeter = 90, kBottomSend = 100, kBottom = 46, kCellH = 78;
static float bottomOf (int t) { const auto& m = mod (t); return m.send || designOf (t).pedal ? kBottomSend : kBottom; }
int SlotView::unitsFor (int t, int height)
{
    const auto D = designOf (t); const float faceH = (float) height - kRail;
    const float ctrlH = faceH - kTop - (D.meter > 0 ? kMeter : 0) - bottomOf (t);
    const int rows = juce::jmax (1, (int) (ctrlH / kCellH)), n = visibleCount (t) + (mod (t).send ? 0 : 1);
    return juce::jlimit (1, 4, (n + 2 * rows - 1) / (2 * rows));
}
SlotView::SlotView (Editor& e, int s) : ed (e), slot (s)
{
    type = ed.proc.slotType (s); D = designOf (type); const auto& m = mod (type);
    setTooltip (m.name + "  (ispirato a " + m.inspired + ")\n" + m.desc);
    for (int i = 0; i < kP; ++i)
    {
        const auto& sp = m.spec[(size_t) i]; if (hidden (sp)) continue;
        auto* prm = ed.P (Processor::sid (s, ("p" + juce::String (i + 1)).toRawUTF8()));
        if (sp.fmt == F_BOOL)
        {
            auto sw = std::make_unique<hw::Switch> (prm, sp.label, "ON", "OFF"); sw->inkC = D.ink; sw->setComponentID (juce::String (i));
            addAndMakeVisible (*sw); ctl.push_back (std::move (sw));
        }
        else
        {
            auto k = std::make_unique<hw::Knob> (prm, sp.label, D.styleFor (i, sp));
            k->inkC = D.ink; k->hot = D.accent.getPerceivedBrightness() > 0.25f ? D.accent : juce::Colour (0xffff8a3d);
            const int st = type; k->text = [st, i, prm] { return paramText (st, i, prm->getValue()); };
            if (sp.fmt == F_ITEMS) { k->stepsN = sp.items.size(); k->ticks = juce::jmax (1, sp.items.size() - 1); }
            k->bipolar = sp.fmt == F_DB && sp.lo < 0 && sp.hi > 0 && std::abs (sp.lo + sp.hi) < 0.01f;
            k->setComponentID (juce::String (i)); k->setTooltip (juce::String (sp.label));
            addAndMakeVisible (*k); ctl.push_back (std::move (k));
        }
    }
    mixK = std::make_unique<hw::Knob> (ed.P (Processor::sid (s, "mix")), m.send ? "MANDATA" : "MIX", m.send ? D.knob == KS_IVORY ? KS_IVORY : KS_BIGBLACK : D.styleFor (0, m.spec[0]));
    mixK->inkC = D.ink; mixK->hot = kAmber;
    mixK->text = [this] { return juce::String (juce::roundToInt (ed.proc.valueS (Processor::sid (slot, "mix")) * 100)) + "%"; };
    if (m.send) { mixK->labelSize = 11.0f; mixK->valueSize = 10.5f; mixK->labelH = 16; }
    mixK->setTooltip (m.send ? "Quanto effetto aggiungi: il suono originale resta sempre intero (come una mandata)" : "Dry/wet del modulo: 100% = solo effetto");
    addAndMakeVisible (*mixK);
}
void SlotView::resized()
{
    const float w = (float) getWidth(), h = (float) getHeight(), fy = kRail;
    const auto& m = mod (type);
    nameR = { 8, 0, w - 34, (float) kRail }; menuR = { w - 26, 0, 24, (float) kRail };
    meterR = {};
    float y = fy + kTop;
    if (D.meter > 0) { const float mw = juce::jmin (w - 28, 168.0f); meterR = { (w - mw) * 0.5f, y + 2, mw, kMeter - 16 }; y += kMeter; }
    const float bottomY = h - bottomOf (type);
    std::vector<juce::Component*> cells; for (auto& c : ctl) cells.push_back (c.get()); if (! m.send) cells.push_back (mixK.get());
    const int n = (int) cells.size();
    if (n > 0)
    {
        const float ctrlH = bottomY - y;
        const int rowsMax = juce::jmax (1, (int) (ctrlH / kCellH));
        const int colsMax = juce::jmax (1, (int) (w / 66.0f));
        const int cols = juce::jlimit (1, colsMax, juce::jmax ((n + rowsMax - 1) / rowsMax, juce::jmin (n, 2)));
        const int rows = (n + cols - 1) / cols;
        const float cw = (w - 12) / cols, ch = juce::jmin (106.0f, ctrlH / rows);
        const float y0 = y + (ctrlH - ch * rows) * 0.5f;
        for (int k = 0; k < n; ++k)
        {
            const int r = k / cols, c = k % cols;
            const int inRow = juce::jmin (cols, n - r * cols);
            const float x0 = 6 + (w - 12 - cw * inRow) * 0.5f;
            auto cell = juce::Rectangle<float> (x0 + c * cw, y0 + r * ch, cw, ch).reduced (2, 3);
            auto* comp = cells[(size_t) k];
            if (dynamic_cast<hw::Switch*> (comp)) comp->setBounds (cell.withSizeKeepingCentre (juce::jmin (cell.getWidth(), 60.0f), juce::jmin (cell.getHeight(), 70.0f)).toNearestInt());
            else comp->setBounds (cell.withSizeKeepingCentre (juce::jmin (cell.getWidth(), 84.0f), cell.getHeight()).toNearestInt());
        }
    }
    // in basso: tasto IN (o footswitch) e, per riverberi e delay, la MANDATA grande
    footR = {}; irR = {}; inR = {};
    if (m.send)
    {
        mixK->setBounds (juce::Rectangle<float> (w * 0.5f - 48, bottomY - 4, 96, kBottomSend - 14).toNearestInt());
        if (D.pedal) footR = { w - 50, bottomY + 30, 38, 38 }; else inR = { w - 50, bottomY + 44, 40, 22 };
        if (m.kind == K_CONV) irR = { 10, bottomY + 42, 44, 26 };
    }
    else if (D.pedal) footR = { w * 0.5f - 24, bottomY + 24, 48, 48 };
    else inR = { w * 0.5f - 22, h - 38, 44, 22 };
}
void SlotView::paint (juce::Graphics& g)
{
    const float w = (float) getWidth(), h = (float) getHeight();
    const auto& m = mod (type); auto& P = ed.proc;
    const bool byp = P.valueS (Processor::sid (slot, "byp")) > 0.5f;
    // binario superiore con il nome (menu)
    {
        auto rail = juce::Rectangle<float> (0, 0, w, (float) kRail - 3);
        juce::ColourGradient rg (juce::Colour (0xff3a3936), 0, 0, juce::Colour (0xff22211f), 0, rail.getBottom(), false); g.setGradientFill (rg); g.fillRoundedRectangle (rail, 3);
        g.setColour (catColour (m.cat)); g.fillRoundedRectangle (4, 5, 3, rail.getHeight() - 10, 1.5f);
        g.setFont (alook::typed (8.5f)); g.setColour (kDim); g.drawText (juce::String (slot + 1), juce::Rectangle<float> (10, 0, 16, rail.getHeight()), juce::Justification::centredLeft);
        g.setFont (alook::serif (10.5f)); g.setColour (kTxt);
        g.drawFittedText (m.name + juce::String::fromUTF8 ("  \xe2\x96\xbe"), juce::Rectangle<int> (26, 0, (int) w - 56, (int) rail.getHeight()), juce::Justification::centredLeft, 1, 0.7f);
        g.setColour (kTxt.withAlpha (0.7f)); g.setFont (alook::serif (14.0f)); g.drawText (juce::String::fromUTF8 ("\xe2\x8b\xae"), menuR, juce::Justification::centred);
    }
    auto face = juce::Rectangle<float> (0, (float) kRail, w, h - kRail);
    hw::faceplate (g, face, D.face, D.plate, slot * 13 + type * 7);
    // marchio: nome grande, ispirazione piccola, spia
    const auto ink = D.ink;
    hw::silk (g, m.name, { 10, face.getY() + 16, w - 20, 22 }, alook::wide (w < 150 ? 13.5f : 16.0f), ink);
    hw::silk (g, m.inspired, { 6, face.getY() + 38, w - 12, 12 }, alook::serif (8.0f, false), ink.withAlpha (0.75f));
    g.setColour (D.accent.withAlpha (0.8f)); g.fillRect (w * 0.5f - 18, face.getY() + 54, 36.0f, 1.5f);
    if (! D.pedal) hw::led (g, w - 16, face.getY() + 24, 4.0f, D.accent.getPerceivedBrightness() > 0.3f ? D.accent : kRed, ! byp);
    // strumento
    if (D.meter > 0 && ! meterR.isEmpty())
    {
        if (D.meter == 3)
        {   // fila di LED di riduzione
            auto lb = meterR.withSizeKeepingCentre (meterR.getWidth(), 22);
            g.setColour (juce::Colour (0xff0a0a0a)); g.fillRoundedRectangle (lb.expanded (3), 3);
            const int N = 12; const float cw = lb.getWidth() / N;
            for (int i = 0; i < N; ++i) { const bool on = ledV > (i + 0.5f) * 2.0f; g.setColour ((i < 6 ? juce::Colour (0xffffc23d) : kRed).withAlpha (on ? 1.0f : 0.13f)); g.fillRoundedRectangle (lb.getX() + i * cw + 1.5f, lb.getY() + 3, cw - 3, lb.getHeight() - 6, 1.5f); }
            g.setFont (alook::typed (7.5f)); g.setColour (ink.withAlpha (0.8f));
            for (int i = 0; i <= N; i += 3) g.drawText (juce::String (i * 2), juce::Rectangle<float> (lb.getX() + i * cw - 10, lb.getBottom() + 4, 20, 10), juce::Justification::centred);
            hw::silk (g, "RIDUZIONE dB", { lb.getX(), lb.getY() - 18, lb.getWidth(), 12 }, alook::serif (8.0f), ink);
        }
        else hw::vu (g, meterR, vu, D.meter == 2 ? "OUTPUT" : "GAIN REDUCTION", D.meter == 1);
    }
    if (D.pedal)
    {   // pedale: footswitch cromato e LED sopra
        auto f = footR;
        hw::led (g, f.getCentreX(), f.getY() - 10, 4.0f, kRed, ! byp);
        juce::ColourGradient fg (juce::Colour (0xfff2f1ee), f.getX(), f.getY(), juce::Colour (0xff6d6b66), f.getRight(), f.getBottom(), false);
        g.setColour (juce::Colours::black.withAlpha (0.5f)); g.fillEllipse (f.translated (1, 2));
        g.setGradientFill (fg); g.fillEllipse (f);
        g.setColour (juce::Colour (0xff8d8b85)); g.drawEllipse (f.reduced (5), 1.5f);
        juce::ColourGradient fg2 (juce::Colour (0xffd8d6d0), f.getX(), f.getBottom(), juce::Colour (0xffffffff), f.getRight(), f.getY(), false); g.setGradientFill (fg2); g.fillEllipse (f.reduced (8));
    }
    if (! inR.isEmpty())
    {   // tasto IN quadrato con la lampadina dentro (come un 500-series)
        auto r = inR; const bool on = ! byp;
        g.setColour (juce::Colours::black.withAlpha (0.55f)); g.fillRoundedRectangle (r.expanded (2).translated (0, 1), 3);
        juce::ColourGradient bg (on ? D.accent.brighter (0.4f) : juce::Colour (0xff4a4844), r.getX(), r.getY(), on ? D.accent.darker (0.3f) : juce::Colour (0xff22211f), r.getX(), r.getBottom(), false);
        if (on && D.accent.getPerceivedBrightness() < 0.25f) bg = juce::ColourGradient (kAmber.brighter (0.3f), r.getX(), r.getY(), kAmber.darker (0.4f), r.getX(), r.getBottom(), false);
        g.setGradientFill (bg); g.fillRoundedRectangle (r, 2.5f);
        if (on) { g.setColour (D.accent.withAlpha (0.25f)); g.fillRoundedRectangle (r.expanded (5), 6); }
        g.setColour (juce::Colours::white.withAlpha (0.25f)); g.drawHorizontalLine ((int) r.getY() + 1, r.getX() + 2, r.getRight() - 2);
        g.setFont (alook::serif (9.5f)); g.setColour (on ? juce::Colour (0xff1a120a) : juce::Colour (0xffbdb6a6)); g.drawText ("IN", r, juce::Justification::centred);
    }
    if (! irR.isEmpty())
    {
        g.setColour (juce::Colour (0xff1a1a1a)); g.fillRoundedRectangle (irR, 3); g.setColour (D.accent); g.drawRoundedRectangle (irR, 3, 1);
        g.setFont (alook::serif (8.0f)); g.drawFittedText ("CARICA\nIR", irR.toNearestInt(), juce::Justification::centred, 2);
        const auto nm = P.userIrName (slot);
        if (nm.isNotEmpty()) { g.setFont (alook::typed (7.5f)); g.setColour (ink); g.drawFittedText (nm, juce::Rectangle<int> (4, (int) irR.getY() - 14, (int) w / 2, 12), juce::Justification::centredLeft, 1, 0.6f); }
    }
    if (byp)
    {
        g.setColour (juce::Colours::black.withAlpha (0.38f)); g.fillRoundedRectangle (face, 3);
        g.setFont (alook::serif (10.0f)); g.setColour (juce::Colours::white.withAlpha (0.7f)); g.drawText ("BYPASS", face.withHeight (20).translated (0, 2), juce::Justification::centred);
    }
}
void SlotView::tickMeters()
{
    if (auto* e = ed.proc.engine (slot))
    {
        const float gr = e->gr.load(), out = juce::Decibels::gainToDecibels (e->out.load(), -60.0f);
        const float target = D.meter == 2 ? out + 12.0f : 3.0f + gr * 1.15f;
        vu += (target - vu) * 0.3f; ledV += (-gr - ledV) * 0.4f;
    }
    if (D.meter > 0) repaint (meterR.expanded (20).toNearestInt());
}
void SlotView::mouseDown (const juce::MouseEvent& e)
{
    downAt = e.position; dragging = false;
    if (e.position.y < kRail)
    {
        if (e.mods.isPopupMenu() || menuR.contains (e.position)) { ed.slotMenu (slot, this); return; }
        return;   // clic sul nome: menu al rilascio (se non trascini)
    }
    if (e.mods.isPopupMenu()) { ed.slotMenu (slot, this); return; }
    if ((D.pedal && footR.expanded (4).contains (e.position)) || (! inR.isEmpty() && inR.expanded (3).contains (e.position))) { auto* p = ed.P (Processor::sid (slot, "byp")); p->setValueNotifyingHost (p->getValue() > 0.5f ? 0.0f : 1.0f); repaint(); return; }
    if (! irR.isEmpty() && irR.contains (e.position))
    {
        auto fc = std::make_shared<juce::FileChooser> ("Scegli una risposta all'impulso (WAV/AIFF)", juce::File::getSpecialLocation (juce::File::userMusicDirectory), "*.wav;*.aif;*.aiff");
        const int s = slot; auto* edp = &ed;
        fc->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [edp, fc, s] (const juce::FileChooser& c)
        { const auto f = c.getResult(); if (! f.existsAsFile()) return; edp->proc.setUserIr (s, f); edp->proc.setP (Processor::sid (s, "p1"), 9.0f); edp->repaint(); });
    }
}
void SlotView::mouseDrag (const juce::MouseEvent& e)
{
    if (downAt.y >= kRail || e.position.getDistanceFrom (downAt) < 6) return;
    if (! ed.isDragAndDropActive()) { dragging = true; ed.startDragging ("slot:" + juce::String (slot), this); }
}
void SlotView::mouseUp (const juce::MouseEvent& e)
{
    if (! dragging && downAt.y < kRail && nameR.contains (e.position) && ! e.mods.isPopupMenu()) ed.pickModule (slot, this);
    dragging = false;
}

// ======================================================================= tessere EQ e +
void EqTile::paint (juce::Graphics& g)
{
    const float w = (float) getWidth(), h = (float) getHeight(); const bool on = ed.proc.value ("eq_on") > 0.5f;
    auto rail = juce::Rectangle<float> (0, 0, w, (float) SlotView::kRail - 3);
    juce::ColourGradient rg (juce::Colour (0xff253441), 0, 0, juce::Colour (0xff17202a), 0, rail.getBottom(), false); g.setGradientFill (rg); g.fillRoundedRectangle (rail, 3);
    g.setFont (alook::serif (10.5f)); g.setColour (juce::Colour (0xff9fd4ff)); g.drawText ("EQ", rail, juce::Justification::centred);
    auto face = juce::Rectangle<float> (0, (float) SlotView::kRail, w, h - SlotView::kRail);
    hw::faceplate (g, face, FACE_BLACK, juce::Colour (0xff141c24), 99);
    hw::led (g, w * 0.5f, face.getY() + 32, 5, kBlue, on);
    hw::silk (g, on ? "ON" : "OFF", { 0, face.getY() + 42, w, 12 }, alook::serif (8.0f), juce::Colour (0xff9fd4ff));
    g.saveState(); g.addTransform (juce::AffineTransform::rotation (-juce::MathConstants<float>::halfPi, w * 0.5f, face.getCentreY()));
    hw::silk (g, juce::String::fromUTF8 ("EQ CENTRALE  \xc2\xb7  trascina per spostarlo"), { w * 0.5f - 150, face.getCentreY() - 9, 300, 18 }, alook::serif (10.5f), juce::Colour (0xff9fd4ff));
    g.restoreState();
    if (! on) { g.setColour (juce::Colours::black.withAlpha (0.35f)); g.fillRoundedRectangle (face, 3); }
}
void EqTile::mouseDown (const juce::MouseEvent& e)
{
    if (e.position.y > SlotView::kRail + 14 && e.position.y < SlotView::kRail + 60) { auto* p = ed.P ("eq_on"); p->setValueNotifyingHost (p->getValue() > 0.5f ? 0.0f : 1.0f); repaint(); ed.graph.repaint(); }
}
void EqTile::mouseDrag (const juce::MouseEvent& e) { if (e.getDistanceFromDragStart() > 6 && ! ed.isDragAndDropActive()) ed.startDragging ("eq", this); }

void AddTile::paint (juce::Graphics& g)
{
    const float w = (float) getWidth(), h = (float) getHeight(); const bool hov = isMouseOver();
    auto rail = juce::Rectangle<float> (0, 0, w, (float) SlotView::kRail - 3);
    g.setColour (juce::Colour (0xff2a2927)); g.fillRoundedRectangle (rail, 3);
    g.setFont (alook::serif (10.0f)); g.setColour (hov ? kAmber : kTxt.withAlpha (0.8f)); g.drawText (juce::String::fromUTF8 ("SELEZIONA PLUGIN \xe2\x96\xbe"), rail, juce::Justification::centred);
    auto face = juce::Rectangle<float> (2, (float) SlotView::kRail + 2, w - 4, h - SlotView::kRail - 4);
    g.setColour (juce::Colour (0xff0d0d0c).withAlpha (0.6f)); g.fillRoundedRectangle (face, 4);
    juce::Path d; d.addRoundedRectangle (face, 4); juce::Path dd; const float dash[] = { 6.0f, 5.0f };
    juce::PathStrokeType (1.3f).createDashedStroke (dd, d, dash, 2); g.setColour ((hov ? kAmber : kTxt).withAlpha (hov ? 0.8f : 0.3f)); g.fillPath (dd);
    g.setFont (alook::serif (44.0f, false)); g.drawText ("+", face.withTrimmedBottom (40), juce::Justification::centred);
    g.setFont (alook::serif (9.0f)); g.drawFittedText ("clicca o trascina qui\nun modulo", face.withTrimmedTop (face.getHeight() * 0.55f).withHeight (30).toNearestInt(), juce::Justification::centred, 2);
}
void AddTile::mouseDown (const juce::MouseEvent&) { ed.pickModule (-1, this); }

// ======================================================================= telaio
void RackContent::rebuild()
{
    auto& P = ed.proc;
    juce::String sig; sig << P.chainLen() << "/" << (int) P.value ("eq_pos") << "/" << getHeight();
    for (int s = 0; s < P.chainLen(); ++s) sig << "," << P.slotType (s);
    if (sig == signature) return;
    signature = sig;
    slots.clear(); eqTile.reset(); addTile.reset(); removeAllChildren();
    for (int s = 0; s < P.chainLen(); ++s) addAndMakeVisible (slots.add (new SlotView (ed, s)));
    eqTile = std::make_unique<EqTile> (ed); addAndMakeVisible (*eqTile);
    if (P.chainLen() < kSlots) { addTile = std::make_unique<AddTile> (ed); addAndMakeVisible (*addTile); }
    layoutTiles (getHeight());
}
void RackContent::layoutTiles (int height)
{
    auto& P = ed.proc; const int n = P.chainLen(), ep = juce::jlimit (0, n, (int) P.value ("eq_pos"));
    int x = 12; const int y = 8, h = height - 16;
    for (int s = 0; s <= n; ++s)
    {
        if (s == ep && eqTile) { eqTile->setBounds (x, y, 58, h); x += 58 + 8; }
        if (s < n && s < slots.size()) { const int w = SlotView::unitsFor (P.slotType (s), h) * SlotView::kUnit; slots[s]->setBounds (x, y, w, h); x += w + 8; }
    }
    if (addTile) { addTile->setBounds (x, y, SlotView::kUnit, h); x += SlotView::kUnit + 8; }
    setSize (juce::jmax (x + 4, ed.rackView.getWidth()), height);
}
void RackContent::paint (juce::Graphics& g)
{   // telaio lunchbox: fondo scuro, guide in alto e in basso con i fori
    auto b = getLocalBounds().toFloat();
    juce::ColourGradient bg (juce::Colour (0xff1b1a19), 0, 0, juce::Colour (0xff0e0e0d), 0, b.getBottom(), false); g.setGradientFill (bg); g.fillRect (b);
    for (float y : { 2.0f, b.getBottom() - 8 })
    {
        juce::ColourGradient rg (juce::Colour (0xff5a5853), 0, y, juce::Colour (0xff2a2926), 0, y + 6, false); g.setGradientFill (rg); g.fillRect (0.0f, y, b.getWidth(), 6.0f);
        g.setColour (juce::Colours::black.withAlpha (0.7f)); for (float x = 10; x < b.getWidth(); x += 18) g.fillEllipse (x, y + 1.5f, 3, 3);
    }
}
void RackContent::paintOverChildren (juce::Graphics& g)
{
    if (dropAt < 0) return;
    float x = 6;
    for (auto* s : slots) if (s->slot < dropAt) x = (float) s->getRight() + 4;
    if (dropAt == 0 && eqTile && (int) ed.proc.value ("eq_pos") == 0) x = 6;
    g.setColour (kAmber); g.fillRoundedRectangle (x - 2, 6, 4, getHeight() - 12.0f, 2);
}
int RackContent::insertIndex (juce::Point<float> p) const
{
    int idx = 0; for (auto* s : slots) if (p.x > s->getBounds().toFloat().getCentreX()) idx = s->slot + 1;
    return juce::jlimit (0, ed.proc.chainLen(), idx);
}
void RackContent::itemDropped (const SourceDetails& d)
{
    const juce::String s = d.description.toString(); const int at = insertIndex (d.localPosition.toFloat()); dropAt = -1;
    auto& P = ed.proc;
    if (s.startsWith ("slot:"))
    {
        const int from = s.fromFirstOccurrenceOf (":", false, false).getIntValue(); int to = at > from ? at - 1 : at;
        to = juce::jlimit (0, P.chainLen() - 1, to); if (to != from) P.moveSlot (from, to);
    }
    else if (s == "eq") P.setP ("eq_pos", (float) at);
    signature.clear(); juce::MessageManager::callAsync ([this] { rebuild(); });
}

// ======================================================================= misure
void LoudPanel::paint (juce::Graphics& g)
{
    auto& P = ed.proc; const auto& T = targets()[(size_t) juce::jlimit (0, (int) targets().size() - 1, (int) P.value ("target"))];
    auto b = getLocalBounds().toFloat(); glassBox (g, b);
    auto in = b.reduced (12);
    g.setFont (alook::serif (9.5f)); g.setColour (kDim); g.drawText (juce::String::fromUTF8 ("LOUDNESS  \xc2\xb7  USCITA"), in.removeFromTop (14), juce::Justification::centredLeft);
    resetR = { b.getRight() - 58, b.getY() + 10, 44, 18 }; pill (g, resetR, "RESET", false, kAmber, 8.5f);
    in.removeFromTop (4);
    targetR = in.removeFromTop (24);
    g.setColour (juce::Colour (0xff17160f)); g.fillRoundedRectangle (targetR, 4); g.setColour (kAmber.withAlpha (0.6f)); g.drawRoundedRectangle (targetR, 4, 1);
    g.setFont (alook::serif (10.0f)); g.setColour (kAmber); g.drawFittedText (juce::String (T.name) + juce::String::fromUTF8 ("  \xe2\x96\xbe"), targetR.reduced (8, 0).toNearestInt(), juce::Justification::centredLeft, 1, 0.7f);
    in.removeFromTop (8);
    const float S = P.lufsS.load(), M = P.lufsM.load(), I = P.lufsI.load(), tp = P.tp.load(), tpH = P.tpHold.load();
    juce::Colour sc = kGreen; juce::String status = "nella zona giusta";
    if (S > -69) { if (S > T.lufsHi) { sc = kRed; status = "troppo forte: -" + juce::String (S - T.lufsHi, 1) + " dB"; } else if (S < T.lufsLo) { sc = juce::Colour (0xff7fb8ff); status = "troppo piano: +" + juce::String (T.lufsLo - S, 1) + " dB"; } }
    else { status = "nessun segnale"; sc = kDim; }
    // numero grande
    auto big = in.removeFromTop (46);
    g.setFont (alook::typed (36.0f)); g.setColour (sc); g.drawText (S <= -69 ? juce::String ("--.-") : juce::String (S, 1), big.removeFromLeft (big.getWidth() * 0.66f), juce::Justification::centredLeft);
    g.setFont (alook::serif (9.0f)); g.setColour (kDim); g.drawFittedText ("LUFS\nSHORT", big.toNearestInt(), juce::Justification::centredLeft, 2);
    g.setFont (alook::serif (9.5f)); g.setColour (sc); g.drawText (status, in.removeFromTop (14), juce::Justification::centredLeft);
    in.removeFromTop (8);
    // barre verticali: M S I e TRUE PEAK, con la zona del target
    auto bars = in.removeFromTop (in.getHeight() - 58);
    const float lo = -40, hi = 0; auto yL = [&] (float db, juce::Rectangle<float> r) { return r.getBottom() - r.getHeight() * juce::jlimit (0.0f, 1.0f, (db - lo) / (hi - lo)); };
    auto scaleR = bars.removeFromLeft (26);
    g.setFont (alook::typed (7.5f)); g.setColour (kDim);
    for (int d = -40; d <= 0; d += 5) { const float y = yL ((float) d, bars.withTrimmedBottom (14)); g.drawText (juce::String (d), juce::Rectangle<float> (scaleR.getX(), y - 5, 22, 10), juce::Justification::centredRight); }
    const float bw = (bars.getWidth() - 12) / 4;
    struct B { const char* n; float v; bool lufs; } bs[] = { { "M", M, true }, { "S", S, true }, { "I", I, true }, { "TP", tp, false } };
    for (int k = 0; k < 4; ++k)
    {
        auto r = juce::Rectangle<float> (bars.getX() + k * (bw + 4), bars.getY(), bw, bars.getHeight() - 14);
        g.setColour (juce::Colour (0xff111110)); g.fillRoundedRectangle (r, 2);
        if (bs[k].lufs) { g.setColour (kGreen.withAlpha (0.12f)); g.fillRect (juce::Rectangle<float>::leftTopRightBottom (r.getX(), yL (T.lufsHi, r), r.getRight(), yL (T.lufsLo, r))); }
        const int nSeg = 40; const float sh = r.getHeight() / nSeg;
        for (int i = 0; i < nSeg; ++i)
        {
            const float db = lo + (hi - lo) * (i + 0.5f) / nSeg; const bool on = bs[k].v > -69 && db <= bs[k].v;
            juce::Colour c = bs[k].lufs ? (db > T.lufsHi ? kRed : db >= T.lufsLo ? kGreen : juce::Colour (0xff4f8fd6)) : (db > T.peak ? kRed : juce::Colour (0xffffc23d));
            g.setColour (c.withAlpha (on ? 0.95f : 0.07f)); g.fillRect (r.getX() + 2, r.getBottom() - (i + 1) * sh + 1, r.getWidth() - 4, sh - 1.5f);
        }
        if (! bs[k].lufs) { g.setColour (juce::Colours::white); g.fillRect (r.getX() - 2, yL (T.peak, r) - 1, r.getWidth() + 4, 2.0f); g.setColour (kRed.withAlpha (0.9f)); g.fillRect (r.getX(), yL (tpH, r) - 1, r.getWidth(), 2.0f); }
        g.setFont (alook::serif (9.0f)); g.setColour (kTxt); g.drawText (bs[k].n, juce::Rectangle<float> (r.getX(), r.getBottom() + 2, r.getWidth(), 12), juce::Justification::centred);
    }
    // righe in basso
    auto row = [&] (const juce::String& l, const juce::String& v, juce::Colour c)
    { auto r = in.removeFromTop (16); g.setFont (alook::serif (9.0f)); g.setColour (kDim); g.drawText (l, r, juce::Justification::centredLeft); g.setFont (alook::typed (10.0f)); g.setColour (c); g.drawText (v, r, juce::Justification::centredRight); };
    in.removeFromTop (4);
    row ("PICCO CONSIGLIATO", juce::String (T.peak, 1) + " dBTP", kTxt);
    row ("PICCO MASSIMO", tpH <= -69 ? juce::String ("--") : juce::String (tpH, 1) + " dBTP", tpH > T.peak ? kRed : kTxt);
    row ("TARGET", juce::String (T.lufsLo, 0) + " / " + juce::String (T.lufsHi, 0) + " LUFS", kTxt);
}
void LoudPanel::mouseDown (const juce::MouseEvent& e)
{
    if (resetR.contains (e.position)) { ed.proc.resetMeter = true; return; }
    if (targetR.contains (e.position))
    {
        juce::PopupMenu m; const int cur = (int) ed.proc.value ("target");
        for (int i = 0; i < (int) targets().size(); ++i) m.addItem (i + 1, targets()[(size_t) i].name, true, i == cur);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [this] (int r) { if (r > 0) { ed.proc.setP ("target", (float) (r - 1)); repaint(); } });
    }
}
void ImagerPanel::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat(); glassBox (g, b);
    auto in = b.reduced (12);
    g.setFont (alook::serif (9.5f)); g.setColour (kDim); g.drawText ("IMMAGINE STEREO", in.removeFromTop (14), juce::Justification::centredLeft);
    auto bottom = in.removeFromBottom (40);
    // semicerchio polare: al centro il mono, ai lati L e R, fuori fase sotto l'orizzonte
    const float R = juce::jmin (in.getWidth() * 0.5f, in.getHeight() - 6), cx = in.getCentreX(), cy = in.getBottom() - 4;
    { juce::Path arc; arc.addCentredArc (cx, cy, R, R, 0, -juce::MathConstants<float>::halfPi, juce::MathConstants<float>::halfPi, true); g.setColour (kLine); g.strokePath (arc, juce::PathStrokeType (1.0f)); }
    for (float a : { -0.785f, 0.0f, 0.785f }) { g.setColour (kLine.withAlpha (0.8f)); g.drawLine (cx, cy, cx + std::sin (a) * R, cy - std::cos (a) * R, 0.7f); }
    g.drawLine (cx - R, cy, cx + R, cy, 0.7f);
    g.setFont (alook::typed (8.0f)); g.setColour (kDim);
    g.drawText ("L", juce::Rectangle<float> (cx - R * 0.72f - 14, cy - R * 0.72f - 12, 12, 12), juce::Justification::centred);
    g.drawText ("R", juce::Rectangle<float> (cx + R * 0.72f + 2, cy - R * 0.72f - 12, 12, 12), juce::Justification::centred);
    g.drawText ("M", juce::Rectangle<float> (cx - 6, cy - R - 12, 12, 10), juce::Justification::centred);
    ed.proc.post.read (bl, br, 2048);
    float pk = 1e-4f; for (int i = 0; i < 2048; ++i) pk = juce::jmax (pk, std::abs (bl[i]), std::abs (br[i]));
    for (int i = 0; i < 2048; i += 2)
    {
        const float l = bl[i] / pk, r = br[i] / pk; const float amp = std::sqrt (l * l + r * r) * 0.7071f;
        if (amp < 0.01f) continue;
        float M = (l + r) * 0.7071f, Sd = (l - r) * 0.7071f;
        if (M < 0) { M = -M; Sd = -Sd; }                  // il semicerchio mostra solo la meta positiva
        const float a = std::atan2 (Sd, M);                // 0 = mono, +-pi/4 = solo L o R, oltre = fuori fase
        const bool neg = std::abs (Sd) > M;
        const float rr = juce::jmin (1.0f, amp) * R;
        const float x = cx - std::sin (a) * rr, y = cy - std::cos (a) * rr;
        g.setColour ((neg ? kRed : juce::Colour (0xff9cf2a8)).withAlpha (0.4f)); g.fillRect (x, y, 1.6f, 1.6f);
    }
    const float c = ed.proc.corr.load(), w = ed.proc.width.load();
    auto cb = bottom.removeFromTop (12).reduced (4, 2);
    g.setColour (juce::Colour (0xff111110)); g.fillRoundedRectangle (cb, 2);
    const float xc = cb.getCentreX() + c * cb.getWidth() * 0.5f;
    g.setColour (c < 0 ? kRed : kGreen); g.fillRect (juce::jmin (cb.getCentreX(), xc), cb.getY() + 1, std::abs (xc - cb.getCentreX()), cb.getHeight() - 2);
    g.setColour (kTxt.withAlpha (0.5f)); g.fillRect (cb.getCentreX() - 0.5f, cb.getY() - 2, 1.0f, cb.getHeight() + 4);
    bottom.removeFromTop (6);
    g.setFont (alook::serif (9.0f)); g.setColour (kDim);
    g.drawText ("CORRELAZIONE  " + juce::String (c, 2), bottom, juce::Justification::topLeft);
    g.drawText ("LARGHEZZA  " + juce::String (juce::roundToInt (w * 200)) + "%", bottom, juce::Justification::topRight);
}

// ======================================================================= EQ centrale
EqGraph::EqGraph (Editor& e) : ed (e)
{
    win.resize (4096); buf.resize (8192); specPre.assign (2048, -100.0f); specPost.assign (2048, -100.0f);
    for (int i = 0; i < 4096; ++i) win[(size_t) i] = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * i / 4095.0f);
    addChildComponent (pop);
}
void EqGraph::resized() { updatePop(); }
float EqGraph::xOf (float f) const { auto p = plot(); return p.getX() + p.getWidth() * std::log (juce::jmax (10.0f, f) / 20.0f) / std::log (1000.0f); }
float EqGraph::fOf (float x) const { auto p = plot(); return 20.0f * std::pow (1000.0f, (x - p.getX()) / p.getWidth()); }
float EqGraph::yOf (float db) const { auto p = plot(); return p.getY() + p.getHeight() * (0.5f - db / 48.0f); }
float EqGraph::dbOf (float y) const { auto p = plot(); return (0.5f - (y - p.getY()) / p.getHeight()) * 48.0f; }
juce::Point<float> EqGraph::nodeOf (int b) const
{
    auto& P = ed.proc; const int t = (int) P.valueS (Processor::bid (b, "type"));
    return { xOf (P.valueS (Processor::bid (b, "f"))), hasGain (t) ? yOf (P.valueS (Processor::bid (b, "g"))) : yOf (0) };
}
int EqGraph::hit (juce::Point<float> p) const
{
    int best = -1; float bd = 13;
    for (int b = 0; b < kBands; ++b) if (ed.proc.valueS (Processor::bid (b, "on")) > 0.5f) { const float d = p.getDistanceFrom (nodeOf (b)); if (d < bd) { bd = d; best = b; } }
    return best;
}
void EqGraph::tick()
{
    auto spec = [this] (const Ring& r, std::vector<float>& out)
    {
        std::vector<float> l (4096), rr (4096); r.read (l.data(), rr.data(), 4096);
        for (int i = 0; i < 4096; ++i) buf[(size_t) i] = 0.5f * (l[(size_t) i] + rr[(size_t) i]) * win[(size_t) i];
        std::fill (buf.begin() + 4096, buf.end(), 0.0f); fft.performFrequencyOnlyForwardTransform (buf.data());
        for (int i = 0; i < 2048; ++i) { const float db = juce::Decibels::gainToDecibels (buf[(size_t) i] / 1024.0f, -100.0f); out[(size_t) i] = db > out[(size_t) i] ? db : out[(size_t) i] * 0.85f + db * 0.15f; }
    };
    spec (ed.proc.pre, specPre); spec (ed.proc.post, specPost);
    // la finestrella della banda segue il mouse e resta un attimo
    if (popHold > 0 && --popHold == 0 && ! pop.isMouseOver (true) && ! isMouseOver (false)) { popBand = sel; updatePop(); }
    repaint();
}
void EqGraph::updatePop()
{
    const int b = popBand >= 0 ? popBand : sel;
    if (b < 0 || ed.proc.valueS (Processor::bid (b, "on")) < 0.5f) { pop.setVisible (false); pop.band = -1; return; }
    pop.show (b);
    const auto n = nodeOf (b); const int pw = pop.getWidth(), ph = pop.getHeight();
    int x = juce::jlimit (4, getWidth() - pw - 4, (int) n.x - pw / 2);
    int y = (int) n.y + 18; if (y + ph > getHeight() - 4) y = (int) n.y - 18 - ph;
    pop.setTopLeftPosition (x, juce::jlimit (4, getHeight() - ph - 4, y));
    pop.setVisible (true); pop.repaint();
}
void EqGraph::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat(); glassBox (g, b);
    auto p = plot();
    const double sr = juce::jmax (8000.0, ed.proc.sampleRate());
    g.setFont (alook::typed (8.0f));
    for (float f : { 20.f, 30.f, 40.f, 50.f, 60.f, 80.f, 100.f, 200.f, 300.f, 400.f, 500.f, 600.f, 800.f, 1000.f, 2000.f, 3000.f, 4000.f, 5000.f, 6000.f, 8000.f, 10000.f, 20000.f })
    {
        const bool major = f == 20 || f == 50 || f == 100 || f == 200 || f == 500 || f == 1000 || f == 2000 || f == 5000 || f == 10000 || f == 20000;
        g.setColour (juce::Colours::white.withAlpha (major ? 0.07f : 0.03f)); g.drawVerticalLine ((int) xOf (f), p.getY(), p.getBottom());
        if (major) { g.setColour (kDim); g.drawText (f >= 1000 ? juce::String ((int) (f / 1000)) + "k" : juce::String ((int) f), juce::Rectangle<float> (xOf (f) - 14, p.getBottom() + 2, 28, 12), juce::Justification::centred); }
    }
    for (int db = -18; db <= 18; db += 6)
    { g.setColour (juce::Colours::white.withAlpha (db == 0 ? 0.13f : 0.04f)); g.drawHorizontalLine ((int) yOf ((float) db), p.getX(), p.getRight()); g.setColour (kDim); g.drawText ((db > 0 ? "+" : "") + juce::String (db), juce::Rectangle<float> (b.getX() + 6, yOf ((float) db) - 6, 24, 12), juce::Justification::centredRight); }
    auto specPath = [&] (const std::vector<float>& sp, bool fill) -> juce::Path
    {
        juce::Path path; bool st = false;
        for (float x = p.getX(); x < p.getRight(); x += 2)
        {
            const float f = fOf (x); const int bin = juce::jlimit (1, 2047, (int) (f / sr * 4096));
            const float db = sp[(size_t) bin] + 4.5f * std::log2 (juce::jmax (0.05f, f / 1000.0f));
            const float y = juce::jlimit (p.getY(), p.getBottom(), p.getBottom() - (db + 90.0f) / 90.0f * p.getHeight());
            if (! st) { path.startNewSubPath (x, fill ? p.getBottom() : y); st = true; if (fill) path.lineTo (x, y); } else path.lineTo (x, y);
        }
        if (fill) { path.lineTo (p.getRight(), p.getBottom()); path.closeSubPath(); }
        return path;
    };
    g.setColour (juce::Colours::white.withAlpha (0.13f)); g.strokePath (specPath (specPre, false), juce::PathStrokeType (1.0f));
    { juce::ColourGradient sg (kBlue.withAlpha (0.25f), 0, p.getY(), kBlue.withAlpha (0.05f), 0, p.getBottom(), false); g.setGradientFill (sg); g.fillPath (specPath (specPost, true)); }
    const auto E = ed.proc.eqSettings(); const bool eqOn = ed.proc.value ("eq_on") > 0.5f;
    if (ed.proc.eq.spectralOn (E))
    {
        juce::Path rp; rp.startNewSubPath (xOf (20), yOf (0));
        for (int v = 0; v < ProEq::kView; ++v) { const float f = 20.0f * std::pow (1000.0f, (float) v / (ProEq::kView - 1)); rp.lineTo (xOf (f), yOf (juce::jlimit (-24.0f, 18.0f, ed.proc.eq.specView[v].load()))); }
        rp.lineTo (xOf (20000), yOf (0)); rp.closeSubPath(); g.setColour (kRed.withAlpha (0.3f)); g.fillPath (rp);
    }
    juce::Path tot; const int NX = (int) (p.getWidth() / 2) + 2; std::vector<double> sum ((size_t) NX, 0.0);
    for (int k = 0; k < kBands; ++k)
    {
        const auto& B = E.b[k]; if (! B.on) continue;
        BQ f[8]; const float gNow = B.g + (B.dyn ? ed.proc.eq.dynUi[k].load() : 0.0f); const int n = designBand (B, sr, gNow, f);
        juce::Path path; int i = 0; bool st = false;
        for (float x = p.getX(); x < p.getRight() && i < NX; x += 2, ++i)
        {
            double d = 0; const float fr = fOf (x); for (int q = 0; q < n; ++q) d += f[q].magDb (fr, sr);
            if (B.ch == 0 || B.ch == 1) sum[(size_t) i] += d;
            const float y = yOf ((float) juce::jlimit (-30.0, 30.0, d));
            if (! st) { path.startNewSubPath (x, y); st = true; } else path.lineTo (x, y);
        }
        const auto c = bandCol (k); const bool focus = k == sel || k == hover || k == popBand;
        if (focus) { juce::Path fp = path; fp.lineTo (p.getRight(), yOf (0)); fp.lineTo (p.getX(), yOf (0)); fp.closeSubPath(); g.setColour (c.withAlpha (0.14f)); g.fillPath (fp); }
        g.setColour (c.withAlpha (focus ? 0.95f : 0.4f)); g.strokePath (path, juce::PathStrokeType (focus ? 1.6f : 1.0f));
    }
    { int i = 0; bool st = false; for (float x = p.getX(); x < p.getRight() && i < NX; x += 2, ++i) { const float y = yOf ((float) juce::jlimit (-30.0, 30.0, sum[(size_t) i])); if (! st) { tot.startNewSubPath (x, y); st = true; } else tot.lineTo (x, y); } }
    g.setColour (juce::Colours::white.withAlpha (eqOn ? 0.95f : 0.3f)); g.strokePath (tot, juce::PathStrokeType (2.0f));
    for (int k = 0; k < kBands; ++k)
    {
        const auto& B = E.b[k]; if (! B.on) continue;
        const auto n = nodeOf (k); const auto c = bandCol (k); const bool focus = k == sel || k == hover || k == popBand;
        if (B.dyn && hasGain (B.type)) { const float y1 = yOf (B.g + B.range); g.setColour (c.withAlpha (0.55f)); g.drawLine (n.x, n.y, n.x, y1, 2.0f); g.fillEllipse (n.x - 3, y1 - 3, 6, 6); }
        g.setColour (c.withAlpha (focus ? 0.45f : 0.2f)); g.fillEllipse (n.x - 11, n.y - 11, 22, 22);
        g.setColour (c); g.drawEllipse (n.x - 9, n.y - 9, 18, 18, focus ? 2.2f : 1.3f);
        g.setFont (alook::typed (9.0f)); g.setColour (juce::Colours::white); g.drawText (juce::String (k + 1), juce::Rectangle<float> (n.x - 9, n.y - 8, 18, 16), juce::Justification::centred);
        juce::String tags; if (B.ch > 0) tags << juce::String ("MSLR")[B.ch - 1]; if (B.dyn) tags << " D"; if (B.spec) tags << " S"; if (B.solo) tags << " SOLO";
        if (tags.isNotEmpty()) { g.setFont (alook::typed (8.0f)); g.setColour (c); g.drawText (tags.trim(), juce::Rectangle<float> (n.x + 11, n.y - 20, 60, 11), juce::Justification::centredLeft); }
    }
    g.setFont (alook::serif (9.5f)); g.setColour (kTxt.withAlpha (0.8f));
    g.drawText ("EQ CENTRALE", juce::Rectangle<float> (b.getX() + 14, b.getY() + 8, 120, 14), juce::Justification::centredLeft);
    g.setFont (alook::serif (8.5f, false)); g.setColour (kDim);
    g.drawText (juce::String::fromUTF8 ("doppio clic = nuova banda  \xc2\xb7  trascina = frequenza e gain  \xc2\xb7  rotella = Q  \xc2\xb7  passa sopra un punto per SOLO, DINAMICA, SPETTRALE  \xc2\xb7  doppio clic sul punto = elimina"),
                juce::Rectangle<float> (b.getX() + 120, b.getY() + 8, b.getWidth() - 140, 14), juce::Justification::centredLeft);
    if (! eqOn) { g.setFont (alook::serif (13.0f)); g.setColour (kAmber); g.drawText ("EQ SPENTO  (accendilo dalla spia del modulo EQ nel rack)", p, juce::Justification::centred); }
}
void EqGraph::mouseMove (const juce::MouseEvent& e)
{
    const int h = hit (e.position);
    if (h != hover) { hover = h; repaint(); }
    if (h >= 0 && h != popBand) { popBand = h; updatePop(); }
    if (h >= 0) popHold = 45;   // ~3 s alla frequenza del timer del grafico
}
void EqGraph::mouseExit (const juce::MouseEvent&) { hover = -1; repaint(); }
void EqGraph::mouseDown (const juce::MouseEvent& e)
{
    drag = hit (e.position);
    if (drag >= 0) { sel = drag; popBand = drag; updatePop(); if (e.mods.isPopupMenu()) { bandMenu (drag); drag = -1; } }
    else if (! e.mods.isPopupMenu()) { sel = -1; popBand = -1; updatePop(); }
}
void EqGraph::mouseDrag (const juce::MouseEvent& e)
{
    if (drag < 0) return;
    auto& P = ed.proc; const int t = (int) P.valueS (Processor::bid (drag, "type"));
    P.setP (Processor::bid (drag, "f"), juce::jlimit (10.0f, 22000.0f, fOf (e.position.x)));
    if (hasGain (t)) P.setP (Processor::bid (drag, "g"), juce::jlimit (-30.0f, 30.0f, dbOf (e.position.y)));
    updatePop();
}
void EqGraph::mouseUp (const juce::MouseEvent&) { drag = -1; }
void EqGraph::mouseDoubleClick (const juce::MouseEvent& e)
{
    auto& P = ed.proc; const int h = hit (e.position);
    if (h >= 0) { P.setP (Processor::bid (h, "on"), 0); P.setP (Processor::bid (h, "solo"), 0); sel = popBand = -1; updatePop(); return; }
    for (int b = 0; b < kBands; ++b)
        if (P.valueS (Processor::bid (b, "on")) < 0.5f)
        {
            const float f = fOf (e.position.x); const int t = f < 40 ? BT_LCUT : f > 16000 ? BT_HCUT : BT_BELL;
            for (auto* id : { "dyn", "spec", "solo" }) P.setP (Processor::bid (b, id), 0);
            P.setP (Processor::bid (b, "type"), (float) t); P.setP (Processor::bid (b, "f"), f); P.setP (Processor::bid (b, "q"), 1);
            P.setP (Processor::bid (b, "g"), t == BT_BELL ? juce::jlimit (-30.0f, 30.0f, dbOf (e.position.y)) : 0.0f); P.setP (Processor::bid (b, "ch"), 0);
            P.setP (Processor::bid (b, "on"), 1); sel = popBand = b; updatePop(); return;
        }
}
void EqGraph::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    const int h = hit (e.position); if (h < 0) return;
    auto* p = ed.P (Processor::bid (h, "q")); p->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, p->getValue() + (w.deltaY > 0 ? 0.025f : -0.025f))); updatePop();
}
void EqGraph::bandMenu (int b)
{
    auto& P = ed.proc; juce::PopupMenu m, tm, cm, sm; const int t = (int) P.valueS (Processor::bid (b, "type")), ch = (int) P.valueS (Processor::bid (b, "ch")), sl = (int) P.valueS (Processor::bid (b, "slope"));
    for (int i = 0; i < kBandTypes; ++i) tm.addItem (100 + i, bandTypeNames()[i], true, i == t);
    for (int i = 0; i < 5; ++i) cm.addItem (200 + i, bandChNames()[i], true, i == ch);
    for (int i = 0; i < 4; ++i) sm.addItem (300 + i, slopeNames()[i] + " dB/ott", true, i == sl);
    m.addSectionHeader ("BANDA " + juce::String (b + 1));
    m.addSubMenu ("Tipo", tm); m.addSubMenu ("Canale", cm); m.addSubMenu ("Pendenza (tagli)", sm, t == BT_LCUT || t == BT_HCUT);
    m.addItem (1, "Dinamica", hasGain (t), P.valueS (Processor::bid (b, "dyn")) > 0.5f);
    m.addItem (2, "Spettrale", hasGain (t), P.valueS (Processor::bid (b, "spec")) > 0.5f);
    m.addItem (3, "Solo", true, P.valueS (Processor::bid (b, "solo")) > 0.5f);
    m.addSeparator(); m.addItem (4, "Elimina");
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [this, b] (int r)
    {
        auto& P = ed.proc; auto tog = [&] (const char* id) { P.setP (Processor::bid (b, id), P.valueS (Processor::bid (b, id)) > 0.5f ? 0.0f : 1.0f); };
        if (r >= 300) P.setP (Processor::bid (b, "slope"), (float) (r - 300));
        else if (r >= 200) P.setP (Processor::bid (b, "ch"), (float) (r - 200));
        else if (r >= 100) P.setP (Processor::bid (b, "type"), (float) (r - 100));
        else if (r == 1) tog ("dyn"); else if (r == 2) tog ("spec");
        else if (r == 3) { const bool on = P.valueS (Processor::bid (b, "solo")) < 0.5f; for (int k = 0; k < kBands; ++k) P.setP (Processor::bid (k, "solo"), 0); P.setP (Processor::bid (b, "solo"), on ? 1.0f : 0.0f); }
        else if (r == 4) { P.setP (Processor::bid (b, "on"), 0); sel = popBand = -1; }
        updatePop(); repaint();
    });
}

// finestrella della banda (stile Pro-Q): tipo, canale, SOLO, DINAMICA, SPETTRALE e i valori trascinabili
enum PopId { PI_TYPE = 1, PI_CH, PI_SLOPE, PI_SOLO, PI_DYN, PI_SPEC, PI_DEL, PI_F = 20, PI_G, PI_Q, PI_THR, PI_RANGE };
void BandPop::show (int b)
{
    band = b; auto& P = graph.ed.proc;
    const bool dynLike = P.valueS (Processor::bid (b, "dyn")) > 0.5f || P.valueS (Processor::bid (b, "spec")) > 0.5f;
    setSize (dynLike ? 486 : 356, 64);
}
void BandPop::paint (juce::Graphics& g)
{
    if (band < 0) return;
    auto& P = graph.ed.proc; const int b = band; auto v = [&P, b] (const char* id) { return P.valueS (Processor::bid (b, id)); };
    const auto c = bandCol (b); auto r = getLocalBounds().toFloat();
    g.setColour (juce::Colours::black.withAlpha (0.5f)); g.fillRoundedRectangle (r.translated (0, 3), 7);
    g.setColour (juce::Colour (0xf01b1c1f)); g.fillRoundedRectangle (r, 7); g.setColour (c.withAlpha (0.8f)); g.drawRoundedRectangle (r.reduced (0.5f), 7, 1.2f);
    hots.clear();
    const int t = (int) v ("type");
    auto btn = [&] (juce::Rectangle<float> rr, const juce::String& s, bool on, juce::Colour col, int id) { pill (g, rr, s, on, col, 9.0f); hots.push_back ({ rr, id }); };
    float x = 8;
    g.setFont (alook::typed (11.0f)); g.setColour (c); g.drawText (juce::String (b + 1), juce::Rectangle<float> (x, 6, 16, 22), juce::Justification::centred); x += 20;
    btn ({ x, 6, 92, 22 }, bandTypeNames()[t] + juce::String::fromUTF8 (" \xe2\x96\xbe"), true, c, PI_TYPE); x += 96;
    btn ({ x, 6, 70, 22 }, bandChNames()[(int) v ("ch")] + juce::String::fromUTF8 (" \xe2\x96\xbe"), v ("ch") > 0.5f, c, PI_CH); x += 74;
    btn ({ x, 6, 46, 22 }, "SOLO", v ("solo") > 0.5f, juce::Colour (0xffffd43b), PI_SOLO); x += 50;
    btn ({ x, 6, 54, 22 }, "DINAMICA", v ("dyn") > 0.5f, kAmber, PI_DYN); x += 58;
    if (r.getWidth() > 400) { btn ({ x, 6, 62, 22 }, "SPETTRALE", v ("spec") > 0.5f, kRed, PI_SPEC); x += 66; }
    else { btn ({ x, 6, 26, 22 }, "SP", v ("spec") > 0.5f, kRed, PI_SPEC); x += 30; }
    btn ({ r.getRight() - 30, 6, 22, 22 }, juce::String::fromUTF8 ("\xc3\x97"), false, kRed, PI_DEL);
    // valori: trascina in su/giu, rotella, doppio clic = default
    x = 8; const float y = 34;
    auto val = [&] (float w, const juce::String& lab, const juce::String& s, bool en, int id)
    {
        auto rr = juce::Rectangle<float> (x, y, w, 24); x += w + 4;
        g.setColour (juce::Colour (0xff0d0d0e)); g.fillRoundedRectangle (rr, 4); g.setColour (en ? c.withAlpha (0.4f) : kLine); g.drawRoundedRectangle (rr, 4, 1);
        g.setFont (alook::serif (7.5f)); g.setColour (kDim); g.drawText (lab, rr.withTrimmedLeft (6).withHeight (10).translated (0, 1), juce::Justification::centredLeft);
        g.setFont (alook::typed (10.0f)); g.setColour (en ? kTxt : kDim); g.drawText (s, rr.withTrimmedTop (9).withTrimmedRight (6), juce::Justification::centredRight);
        if (en) hots.push_back ({ rr, id });
    };
    val (92, "FREQ", hzT (v ("f")), true, PI_F);
    val (80, "GAIN", hasGain (t) ? (v ("g") > 0 ? "+" : "") + juce::String (v ("g"), 1) + " dB" : juce::String ("--"), hasGain (t), PI_G);
    val (64, "Q", juce::String (v ("q"), 2), true, PI_Q);
    if (t == BT_LCUT || t == BT_HCUT) btn ({ x, y, 76, 24 }, slopeNames()[(int) v ("slope")] + " dB/OTT", false, c, PI_SLOPE);
    if (r.getWidth() > 400)
    {
        x = 270;
        val (100, "SOGLIA", juce::String (v ("thr"), 1) + " dB", true, PI_THR);
        val (100, "RANGE", (v ("range") > 0 ? "+" : "") + juce::String (v ("range"), 1) + " dB", true, PI_RANGE);
    }
}
int BandPop::hit (juce::Point<float> p) const { for (auto& h : hots) if (h.r.contains (p)) return h.id; return 0; }
static const char* popParam (int id) { switch (id) { case PI_F: return "f"; case PI_G: return "g"; case PI_Q: return "q"; case PI_THR: return "thr"; case PI_RANGE: return "range"; default: return nullptr; } }
void BandPop::mouseDown (const juce::MouseEvent& e)
{
    if (band < 0) return;
    auto& ed = graph.ed; auto& P = ed.proc; const int b = band; const int id = hit (e.position);
    auto tog = [&] (const char* p) { P.setP (Processor::bid (b, p), P.valueS (Processor::bid (b, p)) > 0.5f ? 0.0f : 1.0f); };
    auto menu = [&] (const juce::StringArray& items, const char* p)
    {
        juce::PopupMenu m; const int cur = (int) P.valueS (Processor::bid (b, p)); for (int i = 0; i < items.size(); ++i) m.addItem (i + 1, items[i], true, i == cur);
        juce::Component::SafePointer<EqGraph> sg (&graph);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [sg, b, p] (int x) { if (x > 0 && sg) { sg->ed.proc.setP (Processor::bid (b, p), (float) (x - 1)); sg->updatePop(); } });
    };
    graph.popHold = 60;
    switch (id)
    {
        case PI_TYPE: menu (bandTypeNames(), "type"); break;
        case PI_CH: menu (bandChNames(), "ch"); break;
        case PI_SLOPE: menu (slopeNames(), "slope"); break;
        case PI_SOLO: { const bool on = P.valueS (Processor::bid (b, "solo")) < 0.5f; for (int k = 0; k < kBands; ++k) P.setP (Processor::bid (k, "solo"), 0); P.setP (Processor::bid (b, "solo"), on ? 1.0f : 0.0f); break; }
        case PI_DYN: tog ("dyn"); break;
        case PI_SPEC: tog ("spec"); break;
        case PI_DEL: P.setP (Processor::bid (b, "on"), 0); P.setP (Processor::bid (b, "solo"), 0); graph.sel = graph.popBand = -1; break;
        default:
            if (auto* pid = popParam (id)) { dragK = id; auto* prm = ed.P (Processor::bid (b, pid)); dragStart = prm->getValue(); if (e.getNumberOfClicks() > 1) prm->setValueNotifyingHost (prm->getDefaultValue()); }
            break;
    }
    graph.updatePop(); graph.repaint();
}
void BandPop::mouseDrag (const juce::MouseEvent& e)
{
    if (band < 0 || dragK < 0) return;
    if (auto* pid = popParam (dragK)) { auto* prm = graph.ed.P (Processor::bid (band, pid)); prm->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, dragStart - e.getDistanceFromDragStartY() / (e.mods.isShiftDown() ? 800.0f : 200.0f))); }
    graph.popHold = 60; repaint(); graph.repaint();
}
void BandPop::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    if (band < 0) return;
    if (auto* pid = popParam (hit (e.position))) { auto* prm = graph.ed.P (Processor::bid (band, pid)); prm->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, prm->getValue() + (w.deltaY > 0 ? 0.01f : -0.01f))); }
    graph.popHold = 60; repaint(); graph.repaint();
}

void GainChip::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff0f0e0d)); g.fillRoundedRectangle (b, 4); g.setColour (kLine.brighter (0.3f)); g.drawRoundedRectangle (b.reduced (0.5f), 4, 1);
    const float v = param->convertFrom0to1 (param->getValue());
    g.setFont (alook::serif (8.0f)); g.setColour (kDim); g.drawText (label, b.withHeight (13).translated (0, 2), juce::Justification::centred);
    g.setFont (alook::typed (11.0f)); g.setColour (std::abs (v) > 0.05f ? kAmber : kTxt); g.drawText ((v > 0 ? "+" : "") + juce::String (v, 1) + " dB", b.withTrimmedTop (13), juce::Justification::centred);
}

// ======================================================================= editor
Editor::Editor (Processor& p) : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);
    lnf.setColour (juce::ScrollBar::thumbColourId, juce::Colour (0xff5d5b56));
    rackView.setViewedComponent (&rack, false); rackView.setScrollBarsShown (false, true); rackView.setScrollBarThickness (9);
    addAndMakeVisible (rackView);
    for (auto* c : std::initializer_list<juce::Component*> { &graph, &loud, &imager }) addAndMakeVisible (c);
    inChip = std::make_unique<GainChip> (P ("in"), "INPUT"); outChip = std::make_unique<GainChip> (P ("out"), "OUTPUT");
    addAndMakeVisible (*inChip); addAndMakeVisible (*outChip);
    addAndMakeVisible (sizeChip);
    scaler.setup();
    startTimerHz (30);
}
void Editor::resized()
{
    scaler.update();
    rackView.setBounds (10, 60, 1130, 480);
    rack.signature.clear(); rack.setSize (rack.getWidth(), 480 - 10); rack.rebuild();
    graph.setBounds (10, 548, 1130, 302);
    loud.setBounds (1148, 60, 242, 480); imager.setBounds (1148, 548, 242, 302);
    inChip->setBounds (1072, 12, 66, 34); outChip->setBounds (1144, 12, 66, 34);
    sizeChip.setBounds (1330, 22, 58, 16);
    jlook::scaleChildren (*this, scaler.s);
}
void Editor::paint (juce::Graphics& g)
{
    g.addTransform (juce::AffineTransform::scale (scaler.s));
    juce::ColourGradient bg (juce::Colour (0xff1f1e1c), 0, 0, kBg, 0, (float) H, false); g.setGradientFill (bg); g.fillRect (0, 0, W, H);
    jlook::grain (g, { 0, 0, W, H }, 0.03f);
    // testata
    { juce::ColourGradient hg (juce::Colour (0xff2b2a27), 0, 0, juce::Colour (0xff1a1918), 0, 54, false); g.setGradientFill (hg); g.fillRect (0, 0, W, 54); g.setColour (juce::Colours::black.withAlpha (0.6f)); g.drawHorizontalLine (54, 0, (float) W); }
    alook::engrave (g, "JACKPOT", { 18, 7, 160, 20 }, alook::wide (15.0f), juce::Justification::centredLeft, kAmber);
    alook::engrave (g, "MIXOLOGY", { 18, 26, 300, 24 }, alook::wide (21.0f), juce::Justification::centredLeft, kTxt);
    {
        auto r = presetR; g.setColour (juce::Colour (0xff0b0b0a)); g.fillRoundedRectangle (r, 5); g.setColour (kLine.brighter (0.3f)); g.drawRoundedRectangle (r, 5, 1);
        g.setFont (alook::typed (12.5f)); g.setColour (kTxt); g.drawFittedText (proc.presetName.toUpperCase() + juce::String::fromUTF8 ("  \xe2\x96\xbe"), r.reduced (10, 0).toNearestInt(), juce::Justification::centred, 1, 0.7f);
        for (auto bb : { prevR, nextR }) { pill (g, bb, bb == prevR ? juce::String::fromUTF8 ("\xe2\x80\xb9") : juce::String::fromUTF8 ("\xe2\x80\xba"), false, kTxt, 16.0f); }
        pill (g, abA, "A", proc.ab.cur == 0, kAmber, 11.0f); pill (g, abB, "B", proc.ab.cur == 1, kAmber, 11.0f);
    }
    pill (g, liveR, "LIVE", proc.value ("live") > 0.5f, kRed);
    pill (g, cmpR, "CONFRONTA", proc.value ("cmp") > 0.5f, kBlue);
    {
        const bool sc = proc.scConnected.load();
        g.setColour (juce::Colour (0xff0f0e0d)); g.fillRoundedRectangle (scR, 4); g.setColour (kLine); g.drawRoundedRectangle (scR, 4, 1);
        hw::led (g, scR.getX() + 12, scR.getCentreY(), 4, kGreen, sc);
        g.setFont (alook::serif (8.5f)); g.setColour (sc ? kTxt : kDim); g.drawFittedText (sc ? "SIDECHAIN\nCOLLEGATO" : "SIDECHAIN\nNON COLLEGATO", scR.withTrimmedLeft (22).toNearestInt(), juce::Justification::centredLeft, 2, 0.8f);
    }
}
void Editor::presetMenu()
{
    juce::PopupMenu m; std::map<juce::String, juce::PopupMenu> cats; juce::StringArray order;
    const auto& F = factoryPresets();
    for (int i = 0; i < (int) F.size(); ++i) { if (! order.contains (F[(size_t) i].cat)) order.add (F[(size_t) i].cat); cats[F[(size_t) i].cat].addItem (1 + i, F[(size_t) i].name, true, i == proc.preset); }
    for (auto& c : order) m.addSubMenu (c, cats[c]);
    m.addSeparator();
    juce::PopupMenu mine; auto files = Processor::userPresetDir().findChildFiles (juce::File::findFiles, false, "*.xml"); files.sort();
    for (int i = 0; i < files.size(); ++i) mine.addItem (5000 + i, files[i].getFileNameWithoutExtension());
    if (files.isEmpty()) mine.addItem (4999, "(nessuno)", false);
    m.addSubMenu ("I MIEI PRESET", mine);
    m.addItem (4000, "Salva preset...");
    m.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea (localAreaToGlobal (scaler.map (presetR.toNearestInt()))), [this, files] (int r)
    {
        if (r <= 0) return;
        if (r == 4000)
        {
            auto* aw = new juce::AlertWindow ("Salva preset", "Nome del preset:", juce::MessageBoxIconType::NoIcon);
            aw->addTextEditor ("n", proc.presetName.fromFirstOccurrenceOf ("| ", false, false)); aw->addButton ("Salva", 1, juce::KeyPress (juce::KeyPress::returnKey)); aw->addButton ("Annulla", 0);
            aw->enterModalState (true, juce::ModalCallbackFunction::create ([this, aw] (int res) { if (res == 1) { proc.saveUserPreset (aw->getTextEditorContents ("n")); repaint(); } }), true);
            return;
        }
        if (r >= 5000) proc.loadUserPreset (files[r - 5000]); else proc.loadPreset (r - 1);
        graph.sel = graph.popBand = -1; graph.updatePop(); chainChanged();
    });
}
juce::PopupMenu Editor::moduleMenu (int base, int cur)
{
    juce::PopupMenu m;
    for (int c = 0; c < kCats; ++c)
    {
        juce::PopupMenu sub; bool any = false;
        for (int t = 1; t < (int) mods().size(); ++t) if (mod (t).cat == c) { sub.addItem (base + t, mod (t).name + "   -   " + mod (t).inspired, true, t == cur); any = true; }
        if (any) m.addSubMenu (catName (c), sub);
    }
    return m;
}
void Editor::pickModule (int s, juce::Component* target)
{
    auto m = moduleMenu (1, s >= 0 ? proc.slotType (s) : -1);
    juce::Component::SafePointer<Editor> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (target), [safe, s] (int r)
    {
        if (r <= 0 || ! safe) return;
        if (s < 0) { if (safe->proc.chainLen() < kSlots) safe->proc.insertModule (safe->proc.chainLen(), r - 1); }
        else safe->proc.setModule (s, r - 1);
        juce::MessageManager::callAsync ([safe] { if (safe) { safe->rack.signature.clear(); safe->rack.rebuild(); safe->rackView.setViewPosition (safe->rack.getWidth(), 0); } });
    });
}
void Editor::slotMenu (int s, juce::Component* target)
{
    juce::PopupMenu m; const bool byp = proc.valueS (Processor::sid (s, "byp")) > 0.5f;
    m.addSectionHeader (juce::String (s + 1) + "  " + mod (proc.slotType (s)).name);
    m.addSubMenu ("Sostituisci con...", moduleMenu (1000, proc.slotType (s)));
    m.addItem (1, byp ? "Riattiva" : "Bypass"); m.addItem (2, "Copia modulo"); m.addItem (3, "Incolla qui", proc.hasClip()); m.addItem (4, "Duplica", proc.chainLen() < kSlots);
    m.addItem (6, "Sposta a sinistra", s > 0); m.addItem (7, "Sposta a destra", s < proc.chainLen() - 1);
    m.addSeparator(); m.addItem (5, "Rimuovi");
    juce::Component::SafePointer<Editor> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (target), [safe, s] (int r)
    {
        if (r <= 0 || ! safe) return;
        auto& P = safe->proc;
        if (r == 1) { auto* p = safe->P (Processor::sid (s, "byp")); p->setValueNotifyingHost (p->getValue() > 0.5f ? 0.0f : 1.0f); }
        else if (r == 2) P.copySlot (s);
        else if (r == 3) P.pasteSlot (s);
        else if (r == 4) { P.copySlot (s); P.insertModule (s + 1, P.slotType (s)); P.pasteSlot (s + 1); }
        else if (r == 5) P.removeSlot (s);
        else if (r == 6) P.moveSlot (s, s - 1);
        else if (r == 7) P.moveSlot (s, s + 1);
        else if (r >= 1000) P.setModule (s, r - 1000);
        juce::MessageManager::callAsync ([safe] { if (safe) { safe->rack.signature.clear(); safe->rack.rebuild(); safe->repaint(); } });
    });
}
void Editor::mouseDown (const juce::MouseEvent& e0)
{
    const auto e = e0.withNewPosition (e0.position / scaler.s);
    const int n = (int) factoryPresets().size();
    auto after = [this] { graph.sel = graph.popBand = -1; graph.updatePop(); chainChanged(); };
    if (prevR.contains (e.position)) { proc.loadPreset ((proc.preset + n - 1) % n); after(); return; }
    if (nextR.contains (e.position)) { proc.loadPreset ((proc.preset + 1) % n); after(); return; }
    if (presetR.contains (e.position)) { presetMenu(); return; }
    if (abA.contains (e.position)) { if (proc.ab.cur != 0) proc.ab.toggle (proc); after(); return; }
    if (abB.contains (e.position)) { if (proc.ab.cur != 1) proc.ab.toggle (proc); after(); return; }
    if (liveR.contains (e.position)) { auto* p = P ("live"); p->setValueNotifyingHost (p->getValue() > 0.5f ? 0.0f : 1.0f); repaint(); return; }
    if (cmpR.contains (e.position)) { auto* p = P ("cmp"); p->setValueNotifyingHost (p->getValue() > 0.5f ? 0.0f : 1.0f); repaint(); return; }
}
void Editor::timerCallback()
{
    ++tick;
    loud.repaint(); imager.repaint();
    if (tick % 2 == 0) graph.tick();
    rack.rebuild();   // ricostruisce solo se la catena e cambiata
    for (auto* s : rack.slots) s->tickMeters();
    if (tick % 4 == 0) { for (auto* s : rack.slots) { for (auto& c : s->ctl) c->repaint(); if (s->mixK) s->mixK->repaint(); } if (rack.eqTile) rack.eqTile->repaint(); inChip->repaint(); outChip->repaint(); }
    if (tick % 15 == 0) repaint (scaler.map ({ 330, 0, 1070, 54 }));
}
}
