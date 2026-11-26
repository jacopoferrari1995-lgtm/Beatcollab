#pragma once
// Scheda preset della serie Cybernet: stessa struttura della scheda Jackpot (gruppi a sinistra, elenco con descrizione,
// preset dell'utente con salva / sovrascrivi / cancella / cartella) ma vestita da terminale cyberpunk:
// vetro scuro, angoli tagliati, staffe HUD, righe di scansione, neon che sfarfalla, numeri di file.
#include <juce_gui_basics/juce_gui_basics.h>

namespace jcyber
{
struct Entry { juce::String group, name, note; int id = 0; bool user = false; };

class Browser : public juce::Component, private juce::Timer
{
public:
    std::function<std::vector<Entry>()> entries;
    std::function<int()> current;
    std::function<void (const Entry&)> load;
    std::function<void()> save, overwrite, folder;
    std::function<void (const Entry&)> remove;
    std::function<bool()> canOverwrite;
    std::function<void()> onClose;
    juce::String title { "PRESET" }, sub { "ARCHIVIO" };
    juce::Colour accent { 0xffff2fa0 }, accent2 { 0xff38f2ff }, ink { 0xffe8ecf7 }, dim { 0xff6f7590 }, bg0 { 0xf2070910 }, bg1 { 0xf20d1020 };
    juce::Font titleFont { juce::FontOptions (28.0f) }, nameFont { juce::FontOptions (18.0f) }, monoFont { juce::FontOptions (11.0f) };

    void finishBoot() { boot = 6; stopTimer(); repaint(); }
    void open()
    {
        list = entries ? entries() : std::vector<Entry>();
        const auto gs = groups(); group = 0;
        if (current) for (auto& e : list) if (e.id == current()) group = juce::jmax (0, gs.indexOf (e.group));
        scroll = 0; hov = hovG = hovB = -1; boot = 0;
        setVisible (true); toFront (true); repaint(); startTimerHz (30);
    }
    void refresh() { list = entries ? entries() : std::vector<Entry>(); repaint(); }
    juce::Rectangle<float> box() const { return { 70, 44, (float) getWidth() - 140, (float) getHeight() - 70 }; }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colours::black.withAlpha (0.72f));
        auto bx = box();
        const float on = juce::jmin (1.0f, boot / 6.0f);   // accensione: la scheda "si accende" in pochi fotogrammi
        auto shape = chamfer (bx, 22);
        g.setColour (accent.withAlpha (0.25f * on)); g.fillPath (shape, juce::AffineTransform::translation (6, 6));
        { juce::ColourGradient cg (bg1, bx.getX(), bx.getY(), bg0, bx.getX(), bx.getBottom(), false); g.setGradientFill (cg); g.fillPath (shape); }
        // griglia e righe di scansione
        g.saveState(); g.reduceClipRegion (shape);
        g.setColour (accent2.withAlpha (0.05f)); for (float x = bx.getX(); x < bx.getRight(); x += 28) g.drawVerticalLine ((int) x, bx.getY(), bx.getBottom());
        for (float y = bx.getY(); y < bx.getBottom(); y += 28) g.drawHorizontalLine ((int) y, bx.getX(), bx.getRight());
        g.setColour (juce::Colours::black.withAlpha (0.18f)); for (float y = bx.getY(); y < bx.getBottom(); y += 3) g.drawHorizontalLine ((int) y, bx.getX(), bx.getRight());
        g.restoreState();
        g.setColour (accent.withAlpha (0.9f * on)); g.strokePath (shape, juce::PathStrokeType (1.6f));
        bracket (g, bx.reduced (8), accent2.withAlpha (0.9f * on), 18);
        // testata
        glow (g, title, { bx.getX() + 30, bx.getY() + 16, 400, 36 }, titleFont, accent, on, juce::Justification::centredLeft);
        g.setFont (monoFont); g.setColour (accent2.withAlpha (0.85f));
        g.drawText (sub + "  //  " + juce::String ((int) list.size()) + " FILE  //  CLIC PER CARICARE, FUORI PER CHIUDERE", juce::Rectangle<float> (bx.getX() + 32, bx.getY() + 50, 640, 16), juce::Justification::centredLeft);
        const char* bl[3] = { "SALVA NUOVO", "SOVRASCRIVI", "CARTELLA" };
        for (int i = 0; i < 3; ++i) { if (i == 1 && ! (canOverwrite && canOverwrite())) continue; button (g, btn (i), bl[i], hovB == i, i == 0 ? accent : accent2); }
        // gruppi come etichette
        const auto gs = groups();
        for (int i = 0; i < gs.size(); ++i)
        {
            auto r = grpCell (bx, i); const bool sel = i == group, h = hovG == i;
            auto p = chamfer (r, 8);
            g.setColour (sel ? accent.withAlpha (0.22f) : juce::Colours::white.withAlpha (h ? 0.06f : 0.02f)); g.fillPath (p);
            g.setColour (sel ? accent : dim.withAlpha (0.6f)); g.strokePath (p, juce::PathStrokeType (sel ? 1.4f : 1.0f));
            if (sel) { g.setColour (accent); g.fillRect (r.getX(), r.getY() + 5, 3.0f, r.getHeight() - 10); }
            g.setFont (monoFont.withHeight (12.0f)); g.setColour (sel ? ink : (h ? ink : dim));
            g.drawText (gs[i].toUpperCase(), r.withTrimmedLeft (14), juce::Justification::centredLeft);
            g.setColour ((sel ? accent2 : dim).withAlpha (0.7f)); g.setFont (monoFont.withHeight (9.5f));
            g.drawText (juce::String ((int) items (i).size()).paddedLeft ('0', 2), r.withTrimmedRight (10), juce::Justification::centredRight);
        }
        // elenco
        auto lr = listRect();
        g.saveState(); g.reduceClipRegion (lr.toNearestInt());
        const auto it = items (group); const int cur = current ? current() : -1;
        if (it.empty()) { g.setFont (monoFont.withHeight (12.0f)); g.setColour (dim); g.drawText ("NESSUN FILE QUI  //  SALVA NUOVO salva le impostazioni di adesso", lr.withHeight (60), juce::Justification::centred); }
        for (int k = 0; k < (int) it.size(); ++k)
        {
            const auto& e = list[(size_t) it[(size_t) k]]; auto r = juce::Rectangle<float> (lr.getX(), lr.getY() + k * kRowH - scroll, lr.getWidth() - 12, kRowH - 8);
            if (r.getBottom() < lr.getY() || r.getY() > lr.getBottom()) continue;
            const bool isCur = e.id == cur, h = hov == it[(size_t) k];
            auto p = chamfer (r, 12);
            g.setColour (isCur ? accent.withAlpha (0.16f) : juce::Colours::white.withAlpha (h ? 0.07f : 0.025f)); g.fillPath (p);
            g.setColour (isCur ? accent : (h ? accent2.withAlpha (0.8f) : dim.withAlpha (0.35f))); g.strokePath (p, juce::PathStrokeType (isCur ? 1.5f : 1.0f));
            g.setFont (monoFont.withHeight (10.0f)); g.setColour ((isCur ? accent : accent2).withAlpha (0.8f));
            g.drawText ((e.user ? juce::String ("USR") : juce::String ("#") + juce::String (k + 1).paddedLeft ('0', 2)), juce::Rectangle<float> (r.getX() + 12, r.getY() + 8, 44, 18), juce::Justification::centredLeft);
            if (isCur || h) glow (g, e.name.toUpperCase(), juce::Rectangle<float> (r.getX() + 58, r.getY() + 4, r.getWidth() - 200, 26), nameFont, isCur ? accent : ink, 1.0f, juce::Justification::centredLeft);
            else { g.setFont (nameFont); g.setColour (ink.withAlpha (0.9f)); g.drawText (e.name.toUpperCase(), juce::Rectangle<float> (r.getX() + 58, r.getY() + 4, r.getWidth() - 200, 26), juce::Justification::centredLeft); }
            if (group == 0 && ! e.user) { g.setFont (monoFont.withHeight (9.0f)); g.setColour (dim); g.drawText (e.group.toUpperCase(), r.withTrimmedRight (16).withHeight (30), juce::Justification::centredRight); }
            if (e.user) button (g, delCell (r), "CANCELLA", h, juce::Colour (0xffff3a3a));
            g.setFont (monoFont.withHeight (10.5f)); g.setColour (dim.brighter (0.3f));
            g.drawFittedText (e.note, r.withTrimmedLeft (58).withTrimmedRight (16).withTrimmedTop (30).toNearestInt(), juce::Justification::topLeft, 2, 0.9f);
            if (h) { g.setFont (monoFont.withHeight (9.5f)); g.setColour (accent2); g.drawText ("> CARICA", r.withTrimmedRight (16).withTrimmedTop (r.getHeight() - 22), juce::Justification::centredRight); }
        }
        g.restoreState();
        const float total = it.size() * kRowH;
        if (total > lr.getHeight()) { const float h = lr.getHeight() * lr.getHeight() / total, y = lr.getY() + (lr.getHeight() - h) * scroll / (total - lr.getHeight()); g.setColour (accent.withAlpha (0.6f)); g.fillRect (lr.getRight() - 4, y, 3.0f, h); }
        if (boot < 6) { g.setColour (accent2.withAlpha (0.25f * (1.0f - on))); g.fillRect (bx.withHeight (bx.getHeight() * (1.0f - on)).withY (bx.getY() + bx.getHeight() * on * 0.5f)); }
    }
    void mouseMove (const juce::MouseEvent& e) override
    {
        auto bx = box(); int h = -1, hg = -1, hb = -1;
        const auto gs = groups(); for (int i = 0; i < gs.size(); ++i) if (grpCell (bx, i).contains (e.position)) hg = i;
        for (int i = 0; i < 3; ++i) if (btn (i).contains (e.position)) hb = i;
        auto lr = listRect();
        if (lr.contains (e.position)) { const auto it = items (group); const int k = (int) ((e.position.y - lr.getY() + scroll) / kRowH); if (k >= 0 && k < (int) it.size()) h = it[(size_t) k]; }
        if (h != hov || hg != hovG || hb != hovB) { hov = h; hovG = hg; hovB = hb; repaint(); }
        setMouseCursor (h >= 0 || hg >= 0 || hb >= 0 ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
    }
    void mouseDown (const juce::MouseEvent& e) override
    {
        auto bx = box();
        if (! bx.contains (e.position)) { close(); return; }
        for (int i = 0; i < 3; ++i) if (btn (i).contains (e.position))
        {
            if (i == 0 && save) save();
            if (i == 1 && canOverwrite && canOverwrite() && overwrite) { overwrite(); refresh(); }
            if (i == 2 && folder) folder();
            return;
        }
        const auto gs = groups(); for (int i = 0; i < gs.size(); ++i) if (grpCell (bx, i).contains (e.position)) { group = i; scroll = 0; repaint(); return; }
        if (hov >= 0)
        {
            const auto en = list[(size_t) hov];
            auto lr = listRect(); const auto it = items (group);
            for (int k = 0; k < (int) it.size(); ++k) if (it[(size_t) k] == hov)
            {
                auto r = juce::Rectangle<float> (lr.getX(), lr.getY() + k * kRowH - scroll, lr.getWidth() - 12, kRowH - 8);
                if (en.user && delCell (r).contains (e.position)) { if (remove) remove (en); refresh(); return; }
            }
            if (load) load (en);
            close();
        }
    }
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w) override
    {
        const float lh = listRect().getHeight(), total = items (group).size() * kRowH;
        scroll = juce::jlimit (0.0f, juce::jmax (0.0f, total - lh), scroll - w.deltaY * 220.0f); repaint();
    }
    bool keyPressed (const juce::KeyPress& k) override { if (k == juce::KeyPress::escapeKey) { close(); return true; } return false; }
private:
    static constexpr float kRowH = 72, kGrpH = 36;
    std::vector<Entry> list; int group = 0, hov = -1, hovG = -1, hovB = -1, boot = 0; float scroll = 0;
    void timerCallback() override { if (++boot >= 6) stopTimer(); repaint(); }
    void close() { stopTimer(); setVisible (false); if (onClose) onClose(); }
    static juce::Path chamfer (juce::Rectangle<float> r, float c)
    {
        juce::Path p; p.startNewSubPath (r.getX() + c, r.getY()); p.lineTo (r.getRight(), r.getY()); p.lineTo (r.getRight(), r.getBottom() - c);
        p.lineTo (r.getRight() - c, r.getBottom()); p.lineTo (r.getX(), r.getBottom()); p.lineTo (r.getX(), r.getY() + c); p.closeSubPath(); return p;
    }
    static void bracket (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c, float len)
    {
        g.setColour (c); const float w = 2;
        g.fillRect (r.getX(), r.getY(), len, w); g.fillRect (r.getX(), r.getY(), w, len); g.fillRect (r.getRight() - len, r.getBottom() - w, len, w); g.fillRect (r.getRight() - w, r.getBottom() - len, w, len);
    }
    void glow (juce::Graphics& g, const juce::String& t, juce::Rectangle<float> r, juce::Font f, juce::Colour c, float amt, juce::Justification j) const
    {
        g.setFont (f);
        for (int i = 3; i >= 1; --i) { g.setColour (c.withAlpha (0.08f * amt)); for (auto o : { juce::Point<float> (-1, 0), juce::Point<float> (1, 0), juce::Point<float> (0, 1) }) g.drawText (t, r.translated (o.x * i * 1.5f, o.y * i * 1.5f), j, false); }
        g.setColour (c.withAlpha (amt)); g.drawText (t, r, j, false);
    }
    void button (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& t, bool h, juce::Colour c) const
    {
        auto p = chamfer (r, 7);
        g.setColour (c.withAlpha (h ? 0.3f : 0.1f)); g.fillPath (p); g.setColour (c.withAlpha (h ? 1.0f : 0.7f)); g.strokePath (p, juce::PathStrokeType (1.2f));
        g.setFont (monoFont.withHeight (10.5f)); g.setColour (h ? ink : c); g.drawText (t, r, juce::Justification::centred);
    }
    juce::StringArray groups() const
    {
        juce::StringArray g; g.add ("TUTTI");
        for (auto& e : list) if (! e.user && ! g.contains (e.group)) g.add (e.group);
        g.add ("I MIEI");
        return g;
    }
    std::vector<int> items (int gi) const
    {
        const auto gs = groups(); std::vector<int> v;
        for (int i = 0; i < (int) list.size(); ++i)
        {
            const auto& e = list[(size_t) i];
            if (gs[gi] == "I MIEI" ? e.user : (gi == 0 ? ! e.user : (! e.user && e.group == gs[gi]))) v.push_back (i);
        }
        return v;
    }
    juce::Rectangle<float> grpCell (juce::Rectangle<float> bx, int i) const { return { bx.getX() + 24, bx.getY() + 82 + i * kGrpH, 200, kGrpH - 6 }; }
    juce::Rectangle<float> listRect() const { auto bx = box(); return { bx.getX() + 244, bx.getY() + 82, bx.getWidth() - 266, bx.getHeight() - 100 }; }
    juce::Rectangle<float> btn (int i) const { auto bx = box(); return { bx.getRight() - 26 - (3 - i) * 128.0f, bx.getY() + 22, 120, 30 }; }
    static juce::Rectangle<float> delCell (juce::Rectangle<float> r) { return { r.getRight() - 100, r.getY() + 8, 86, 22 }; }
};
}
