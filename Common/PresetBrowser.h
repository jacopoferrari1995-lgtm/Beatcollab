#pragma once
// Scheda dei preset condivisa (stessa vista di Jackpot Master): gruppi a sinistra, elenco grande con la descrizione,
// in piu' i preset dell'utente con salva / sovrascrivi / cancella / cartella.
#include "Neo.h"

namespace jbrowse
{
struct Entry { juce::String group, name, note; int id = 0; bool user = false; };

class Browser : public juce::Component
{
public:
    std::function<std::vector<Entry>()> entries;
    std::function<int()> current;              // id del preset caricato (-1 nessuno)
    std::function<void (const Entry&)> load;
    std::function<void()> save, overwrite, folder;
    std::function<void (const Entry&)> remove;
    std::function<bool()> canOverwrite;
    std::function<void()> onClose;
    juce::Colour accent { 0xffa66bff };
    juce::String title { "Preset" };

    void open()
    {
        list = entries ? entries() : std::vector<Entry>();
        const auto gs = groups(); group = 0;
        if (current) for (auto& e : list) if (e.id == current()) group = juce::jmax (0, gs.indexOf (e.group));
        scroll = 0; hov = hovG = hovB = -1;
        setVisible (true); toFront (true); repaint();
    }
    void refresh() { list = entries ? entries() : std::vector<Entry>(); repaint(); }
    juce::Rectangle<float> box() const { return { 120, 64, (float) getWidth() - 240, (float) getHeight() - 96 }; }

    void paint (juce::Graphics& g) override
    {
        using namespace neo;
        g.fillAll (K::lo.withAlpha (light() ? 0.25f : 0.62f));
        auto bx = box();
        g.setColour (K::lo.withAlpha (0.4f)); g.fillRoundedRectangle (bx.translated (0, 6), 20);
        juce::ColourGradient bg (K::panel0, 0, bx.getY(), K::panel1, 0, bx.getBottom(), false); g.setGradientFill (bg); g.fillRoundedRectangle (bx, 20);
        g.setColour (K::hi.withAlpha (0.08f)); g.drawRoundedRectangle (bx.reduced (0.5f), 20, 1.0f);
        g.setFont (big (20.0f)); g.setColour (K::ink); g.drawText (title, juce::Rectangle<float> (bx.getX() + 22, bx.getY() + 14, 200, 32), juce::Justification::centredLeft);
        g.setFont (f (10.5f)); g.setColour (K::sub); g.drawText (juce::String::fromUTF8 ("clic per caricare  \xc2\xb7  clic fuori per chiudere"), juce::Rectangle<float> (bx.getX() + 130, bx.getY() + 14, 300, 32), juce::Justification::centredLeft);
        const char* bl[3] = { "SALVA NUOVO", "SOVRASCRIVI", "CARTELLA" };
        for (int i = 0; i < 3; ++i) { if (i == 1 && ! (canOverwrite && canOverwrite())) continue; chip (g, btn (i), bl[i], false, accent, 9.5f, hovB == i); }
        const auto gs = groups();
        for (int i = 0; i < gs.size(); ++i) chip (g, grpCell (bx, i), gs[i].toUpperCase(), i == group, accent, 9.5f, hovG == i);
        auto lr = listRect();
        g.saveState(); g.reduceClipRegion (lr.toNearestInt());
        const auto it = items (group); const int cur = current ? current() : -1;
        if (it.empty()) { g.setFont (f (12.0f)); g.setColour (K::sub); g.drawText ("Nessun preset qui. SALVA NUOVO salva le impostazioni di adesso.", lr.withHeight (60), juce::Justification::centred); }
        for (int k = 0; k < (int) it.size(); ++k)
        {
            const auto& e = list[(size_t) it[(size_t) k]]; auto r = juce::Rectangle<float> (lr.getX(), lr.getY() + k * kRowH - scroll, lr.getWidth() - 10, kRowH - 6);
            if (r.getBottom() < lr.getY() || r.getY() > lr.getBottom()) continue;
            const bool isCur = e.id == cur, h = hov == it[(size_t) k];
            g.setColour (isCur ? accent.withAlpha (0.18f) : K::hi.withAlpha (h ? 0.07f : 0.03f)); g.fillRoundedRectangle (r, 10);
            if (isCur) { g.setColour (accent.withAlpha (0.7f)); g.drawRoundedRectangle (r.reduced (0.5f), 10, 1.0f); }
            g.setFont (f (14.0f, 1)); g.setColour (K::ink); g.drawText ((e.user ? juce::String::fromUTF8 ("\xe2\x98\x85 ") : juce::String()) + e.name, r.withTrimmedLeft (16).withTrimmedRight (110).withHeight (26).translated (0, 4), juce::Justification::centredLeft);
            if (group == 0 && ! e.user) { g.setFont (cap (7.5f)); g.setColour (K::sub); g.drawText (e.group.toUpperCase(), r.withTrimmedRight (14).withHeight (26).translated (0, 4), juce::Justification::centredRight); }
            if (e.user) chip (g, delCell (r), "CANCELLA", false, K::red, 8.0f, h);
            g.setFont (f (9.8f)); g.setColour (K::sub); g.drawFittedText (e.note, r.withTrimmedLeft (16).withTrimmedRight (12).withTrimmedTop (30).toNearestInt(), juce::Justification::topLeft, 2, 0.9f);
        }
        g.restoreState();
        const float total = it.size() * kRowH;
        if (total > lr.getHeight()) { const float h = lr.getHeight() * lr.getHeight() / total, y = lr.getY() + (lr.getHeight() - h) * scroll / (total - lr.getHeight()); g.setColour (K::hi.withAlpha (0.15f)); g.fillRoundedRectangle (lr.getRight() - 4, y, 4, h, 2); }
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
                auto r = juce::Rectangle<float> (lr.getX(), lr.getY() + k * kRowH - scroll, lr.getWidth() - 10, kRowH - 6);
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
private:
    static constexpr float kRowH = 70, kGrpH = 31;
    std::vector<Entry> list; int group = 0, hov = -1, hovG = -1, hovB = -1; float scroll = 0;
    void close() { setVisible (false); if (onClose) onClose(); }
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
    juce::Rectangle<float> grpCell (juce::Rectangle<float> bx, int i) const { return { bx.getX() + 16, bx.getY() + 58 + i * kGrpH, 190, kGrpH - 5 }; }
    juce::Rectangle<float> listRect() const { auto bx = box(); return { bx.getX() + 222, bx.getY() + 58, bx.getWidth() - 238, bx.getHeight() - 72 }; }
    juce::Rectangle<float> btn (int i) const { auto bx = box(); return { bx.getRight() - 16 - (3 - i) * 122.0f, bx.getY() + 16, 114, 28 }; }
    static juce::Rectangle<float> delCell (juce::Rectangle<float> r) { return { r.getRight() - 96, r.getY() + 8, 84, 22 }; }
};
}
