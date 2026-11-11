#include "RewindEditor.h"

namespace jrwd
{
static juce::String dotS() { return juce::String::fromUTF8 ("  \xc2\xb7  "); }
juce::String Editor::barsText (juce::int64 a, juce::int64 b) const
{
    double ppq, bpm; int num;
    if (! proc.musicalAt (a, ppq, bpm, num)) return {};
    const double br = (double) (b - a) / proc.sampleRate() * bpm / 60.0 / num;
    return (std::abs (br - std::round (br)) < 0.05 ? juce::String ((int) std::round (br)) : juce::String (br, 1)) + " bat";
}
void Editor::setSel (juce::int64 a, juce::int64 b, bool snap)
{
    if (b < a) std::swap (a, b);
    if (snap && proc.value ("snap") > 0.5f) { a = proc.snapToBar (a, -1); b = proc.snapToBar (b, 1); if (b <= a) b = a + (juce::int64) (proc.sampleRate() * 0.1); }
    selA = juce::jmax (a, proc.oldest()); selB = juce::jmin (b, proc.written());
    if (proc.pvOn.load()) proc.preview (selA, selB, proc.value ("loop") > 0.5f);
    repaint();
}
juce::File Editor::exportSel()
{
    if (! hasSel()) return {};
    juce::String info; auto f = proc.exportRange (selA, selB, proc.value ("norm") > 0.5f, proc.value ("rev") > 0.5f, info);
    if (f.existsAsFile()) lastFile = f.getFileName();
    repaint();
    return f;
}
void Editor::findTakes()
{
    // prese: dove c'era suono, con pause piu corte di 1.5 s unite
    takes.clear();
    const juce::int64 c0 = proc.oldest() / proc.colLen() + 1, c1 = proc.written() / proc.colLen();
    const int gap = (int) (1.5 / Processor::kColSec), minLen = (int) (0.6 / Processor::kColSec);
    juce::int64 start = -1, lastOn = -1; double en = 0;
    for (juce::int64 c = c0; c < c1; ++c)
    {
        const auto& cl = proc.col (c); if (cl.abs.load() != c) continue;
        const bool on = cl.rms.load() > 0.004f;
        if (on) { if (start < 0) { start = c; en = 0; } lastOn = c; en += cl.rms.load(); }
        if (start >= 0 && (c - lastOn > gap || c == c1 - 1))
        {
            if (lastOn - start >= minLen) takes.push_back ({ start * proc.colLen(), (lastOn + 1) * proc.colLen(), (float) (en / (lastOn - start + 1)), false });
            start = -1;
        }
    }
    const juce::ScopedLock l (proc.marksLock);
    for (auto& t : takes) for (auto m : proc.marks) if (m >= t.a && m <= t.b + (juce::int64) (proc.sampleRate() * 2)) t.starred = true;
}

// ======================================================================= panoramica
juce::int64 Overview::sampleAt (float x) const
{
    const float w = (float) getWidth() - 20;
    return viewEnd - viewLen + (juce::int64) ((x - 10) / w * (double) viewLen);
}
float Overview::xOf (juce::int64 s) const { return 10.0f + (float) ((double) (s - (viewEnd - viewLen)) / (double) viewLen) * (getWidth() - 20.0f); }
void Overview::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat(); lcd (g, b, 8, ed.acc);
    auto& P = ed.proc; viewEnd = P.written();
    // la vista cresce con la memoria: da 1 minuto fino a tutta la capacita
    viewLen = juce::jlimit ((juce::int64) (P.sampleRate() * 60), P.capacity(), (juce::int64) (P.written() * 1.05));
    const float top = 22, bot = b.getBottom() - 18, mid = (top + bot) * 0.5f, half = (bot - top) * 0.5f;
    // prese
    for (auto& t : ed.takes)
    {
        const float x0 = xOf (t.a), x1 = xOf (t.b);
        g.setColour ((t.starred ? C::amber : ed.acc).withAlpha (0.08f)); g.fillRect (juce::Rectangle<float>::leftTopRightBottom (x0, top - 6, x1, bot + 4));
        g.setColour ((t.starred ? C::amber : ed.acc).withAlpha (0.5f)); g.fillRect (juce::Rectangle<float>::leftTopRightBottom (x0, top - 6, x1, top - 3));
    }
    // onda (picchi per colonna)
    const int W = getWidth() - 20; const double perPx = (double) viewLen / W / P.colLen();
    for (int x = 0; x < W; ++x)
    {
        const juce::int64 cA = (viewEnd - viewLen) / P.colLen() + (juce::int64) (x * perPx), cB = cA + juce::jmax ((juce::int64) 1, (juce::int64) perPx);
        float pk = 0, rm = 0;
        for (juce::int64 c = cA; c < cB; ++c) { if (c < 0 || c * P.colLen() < P.oldest()) continue; const auto& cl = P.col (c); if (cl.abs.load() != c) continue; pk = juce::jmax (pk, cl.pk.load()); rm = juce::jmax (rm, cl.rms.load()); }
        if (pk <= 0) continue;
        const float hp = juce::jmin (1.0f, std::sqrt (pk)) * half, hr = juce::jmin (1.0f, std::sqrt (rm) * 1.4f) * half;
        g.setColour (ed.acc.withAlpha (0.35f)); g.drawVerticalLine (10 + x, mid - hp, mid + hp);
        g.setColour (ed.acc.withAlpha (0.85f)); g.drawVerticalLine (10 + x, mid - hr, mid + hr);
    }
    // segnalibri
    { const juce::ScopedLock l (P.marksLock); for (auto m : P.marks) { const float x = xOf (m); if (x < 10) continue; g.setColour (C::amber); g.drawVerticalLine ((int) x, top - 6, bot); g.setFont (uf (12.0f)); g.drawText (juce::String::fromUTF8 ("\xe2\x98\x85"), juce::Rectangle<float> (x - 8, 2, 16, 14), juce::Justification::centred); } }
    // selezione
    if (ed.hasSel())
    {
        const float x0 = xOf (ed.selA), x1 = juce::jmax (x0 + 2, xOf (ed.selB));
        g.setColour (ed.acc2.withAlpha (0.18f)); g.fillRect (juce::Rectangle<float>::leftTopRightBottom (x0, top - 6, x1, bot + 4));
        g.setColour (ed.acc2); g.drawRect (juce::Rectangle<float>::leftTopRightBottom (x0, top - 6, x1, bot + 4), 1.5f);
    }
    if (P.pvOn.load()) { const float x = xOf (P.pvPos.load()); g.setColour (C::text); g.drawVerticalLine ((int) x, top - 6, bot + 4); }
    // asse del tempo: da quanto fa
    g.setFont (mf (9.0f));
    const double totSec = (double) viewLen / P.sampleRate(); const int stepS = totSec > 180 ? 30 : 10, labS = totSec > 180 ? 60 : 10;
    for (int s = 0; s <= (int) totSec; s += stepS)
    {
        const float x = xOf (viewEnd - (juce::int64) (s * P.sampleRate()));
        g.setColour (juce::Colours::white.withAlpha (s % labS == 0 ? 0.12f : 0.05f)); g.drawVerticalLine ((int) x, top, bot);
        if (s % labS == 0) { g.setColour (C::dim); g.drawText (s == 0 ? juce::String ("ORA") : "-" + ed.timeText (s), juce::Rectangle<float> (x - 30, bot + 2, 60, 14), juce::Justification::centred); }
    }
    if (P.hold.load()) glowText (g, "BLOCCATO", { b.getRight() - 120, 4, 110, 14 }, C::red, mf (10.0f), juce::Justification::centredRight);
    else glowText (g, juce::String::fromUTF8 ("\xe2\x97\x8f REC"), { b.getRight() - 120, 4, 110, 14 }, C::red.withAlpha (0.5f + 0.5f * (float) ((juce::Time::getMillisecondCounter() / 500) % 2)), mf (10.0f), juce::Justification::centredRight);
}
void Overview::mouseDown (const juce::MouseEvent& e) { dragFrom = sampleAt (e.position.x); }
void Overview::mouseDrag (const juce::MouseEvent& e) { if (dragFrom >= 0 && std::abs (e.getDistanceFromDragStartX()) > 3) ed.setSel (dragFrom, sampleAt (e.position.x)); }
void Overview::mouseUp (const juce::MouseEvent& e)
{
    if (std::abs (e.getDistanceFromDragStartX()) <= 3)
    {   // clic: seleziona la presa sotto il mouse
        const auto s = sampleAt (e.position.x);
        for (auto& t : ed.takes) if (s >= t.a && s <= t.b) { ed.setSel (t.a, t.b); return; }
    }
    dragFrom = -1;
}
void Overview::mouseDoubleClick (const juce::MouseEvent&) { ed.selA = ed.selB = -1; ed.repaint(); }

// ======================================================================= dettaglio
void Detail::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat(); lcd (g, b, 8, ed.acc2);
    auto& P = ed.proc;
    if (! ed.hasSel())
    {
        g.setColour (C::dim); g.setFont (uf (13.0f));
        g.drawFittedText (juce::String::fromUTF8 ("Trascina sulla memoria qui sopra per scegliere un pezzo, oppure clicca una presa.\nTutto quello che passa su questa traccia resta qui per 8 minuti, anche se non stavi registrando."), b.reduced (20).toNearestInt(), juce::Justification::centred, 3, 1.0f);
        return;
    }
    const juce::int64 len = ed.selB - ed.selA, pad = len / 12;
    v0 = ed.selA - pad; v1 = ed.selB + pad;
    if (cA != v0 || cB != v1) { P.readRange (juce::jmax (v0, P.oldest()), juce::jmin (v1, P.written()), cache); cA = v0; cB = v1; }
    const float top = 18, bot = b.getBottom() - 16, mid = (top + bot) * 0.5f, half = (bot - top) * 0.5f;
    auto X = [&] (juce::int64 s) { return 10.0f + (float) ((double) (s - v0) / (double) (v1 - v0)) * (getWidth() - 20.0f); };
    // griglia delle battute
    double ppq, bpm; int num;
    if (P.musicalAt (ed.selA, ppq, bpm, num))
    {
        const double spb = 60.0 / bpm * P.sampleRate(); const double firstBeat = std::ceil ((ppq - (double) (ed.selA - v0) / spb));
        for (double bt = firstBeat; ; bt += 1)
        {
            const juce::int64 s = ed.selA + (juce::int64) ((bt - ppq) * spb); if (s > v1) break;
            const bool bar = std::fmod (bt, (double) num) == 0; g.setColour (juce::Colours::white.withAlpha (bar ? 0.16f : 0.05f)); g.drawVerticalLine ((int) X (s), top, bot);
            if (bar) { g.setColour (C::dim); g.setFont (mf (8.5f)); g.drawText (juce::String ((int) (bt / num) + 1), juce::Rectangle<float> (X (s) + 2, 2, 30, 12), juce::Justification::centredLeft); }
        }
    }
    const int n = cache.getNumSamples(); const juce::int64 off = juce::jmax (v0, P.oldest()) - v0;
    const int Wd = getWidth() - 20;
    for (int x = 0; x < Wd; ++x)
    {
        const juce::int64 s0 = v0 + (juce::int64) ((double) x / Wd * (v1 - v0)) - v0 - off, s1 = v0 + (juce::int64) ((double) (x + 1) / Wd * (v1 - v0)) - v0 - off;
        float mn = 0, mx = 0;
        for (juce::int64 s = juce::jmax ((juce::int64) 0, s0); s < juce::jmin ((juce::int64) n, s1); ++s) { const float v = 0.5f * (cache.getSample (0, (int) s) + cache.getSample (1, (int) s)); mn = juce::jmin (mn, v); mx = juce::jmax (mx, v); }
        const juce::int64 sa = v0 + (juce::int64) ((double) x / Wd * (v1 - v0)); const bool in = sa >= ed.selA && sa < ed.selB;
        g.setColour ((in ? ed.acc2 : C::dim).withAlpha (in ? 0.9f : 0.35f)); g.drawVerticalLine (10 + x, mid - mx * half, mid - mn * half + 1);
    }
    for (int k = 0; k < 2; ++k) { const float x = X (k ? ed.selB : ed.selA); g.setColour (ed.acc2); g.drawLine (x, top - 4, x, bot + 4, 2.0f); juce::Path t; t.addTriangle (x - 6, top - 10, x + 6, top - 10, x, top - 2); g.fillPath (t); }
    if (P.pvOn.load()) { const float x = X (P.pvPos.load()); g.setColour (C::text); g.drawVerticalLine ((int) x, top, bot); }
}
void Detail::mouseMove (const juce::MouseEvent& e)
{
    if (! ed.hasSel()) return;
    auto X = [&] (juce::int64 s) { return 10.0f + (float) ((double) (s - v0) / (double) (v1 - v0)) * (getWidth() - 20.0f); };
    const bool near = std::abs (e.position.x - X (ed.selA)) < 8 || std::abs (e.position.x - X (ed.selB)) < 8;
    setMouseCursor (near ? juce::MouseCursor::LeftRightResizeCursor : juce::MouseCursor::NormalCursor);
}
void Detail::mouseDown (const juce::MouseEvent& e)
{
    edge = 0; if (! ed.hasSel()) return;
    auto X = [&] (juce::int64 s) { return 10.0f + (float) ((double) (s - v0) / (double) (v1 - v0)) * (getWidth() - 20.0f); };
    if (std::abs (e.position.x - X (ed.selA)) < 10) edge = 1; else if (std::abs (e.position.x - X (ed.selB)) < 10) edge = 2;
}
void Detail::mouseDrag (const juce::MouseEvent& e)
{
    if (edge == 0) return;
    const juce::int64 s = v0 + (juce::int64) ((e.position.x - 10) / (getWidth() - 20.0f) * (double) (v1 - v0));
    const bool snap = ! e.mods.isShiftDown();
    if (edge == 1) ed.setSel (s, ed.selB, snap); else ed.setSel (ed.selA, s, snap);
    if (edge == 1 && ed.selA > ed.selB) edge = 2;
}

// ======================================================================= trascina fuori
void DragZone::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (2); const bool ok = ed.hasSel();
    const auto c = ok ? ed.acc2 : C::faint;
    g.setColour (c.withAlpha (hover && ok ? 0.22f : 0.1f)); g.fillRoundedRectangle (b, 10);
    const float dash[] = { 6, 4 }; juce::Path p; p.addRoundedRectangle (b, 10); juce::Path d; juce::PathStrokeType (1.5f).createDashedStroke (d, p, dash, 2); g.setColour (c); g.fillPath (d);
    glowText (g, ok ? "TRASCINA IN ABLETON" : "SELEZIONA UN PEZZO", b.withTrimmedBottom (b.getHeight() * 0.42f).withTrimmedTop (8), c, uf (15.0f), juce::Justification::centredBottom);
    g.setColour (C::dim); g.setFont (mf (9.0f));
    g.drawText (ok ? "prendi da qui e lascia su una traccia audio" + dotS() + "clic = salva" : juce::String(), b.withTrimmedTop (b.getHeight() * 0.62f), juce::Justification::centredTop);
}
void DragZone::mouseDrag (const juce::MouseEvent& e)
{
    if (dragging || e.getDistanceFromDragStart() < 6 || ! ed.hasSel()) return;
    dragging = true;
    auto f = ed.exportSel();
    if (f.existsAsFile()) juce::DragAndDropContainer::performExternalDragDropOfFiles ({ f.getFullPathName() }, false, this, [this] { dragging = false; });
    else dragging = false;
}
void DragZone::mouseUp (const juce::MouseEvent& e) { if (! dragging && e.getDistanceFromDragStart() < 6 && ed.hasSel()) ed.exportSel(); dragging = false; }

// ======================================================================= editor
Editor::Editor (Processor& p) : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);
    auto pill = [&] (std::function<juce::String()> t, std::function<void()> click, std::function<bool()> lit, juce::Colour c)
    { auto pl = std::make_unique<Pill> (P ("snap"), c, std::move (t)); pl->click = std::move (click); pl->lit = std::move (lit); addAndMakeVisible (*pl); pills.push_back (std::move (pl)); };
    pill ([this] { return proc.pvOn.load() ? juce::String::fromUTF8 ("\xe2\x96\xa0  STOP") : juce::String::fromUTF8 ("\xe2\x96\xb6  ASCOLTA"); },
          [this] { if (proc.pvOn.load()) proc.stopPreview(); else if (hasSel()) proc.preview (selA, selB, proc.value ("loop") > 0.5f); }, [this] { return proc.pvOn.load(); }, acc2);
    pill ([this] { return proc.hold.load() ? juce::String ("SBLOCCA") : juce::String ("BLOCCA"); }, [this] { proc.hold = ! proc.hold.load(); }, [this] { return proc.hold.load(); }, C::red);
    pill ([] { return juce::String::fromUTF8 ("\xe2\x98\x85  SEGNA ORA"); }, [this] { proc.addMark(); findTakes(); }, [] { return false; }, C::amber);
    pill ([] { return juce::String ("ULTIMA PRESA"); }, [this] { if (! takes.empty()) setSel (takes.back().a, takes.back().b); }, [] { return false; }, acc);
    pill ([] { return juce::String ("ULTIME 4 BATTUTE"); }, [this]
          {   // le ultime 4 battute intere suonate
              double ppq, bpm; int num; const auto now = proc.written() - 1;
              if (proc.musicalAt (now, ppq, bpm, num)) { const double bar = std::floor (ppq / num) * num; const double spb = 60.0 / bpm * proc.sampleRate();
                  const juce::int64 end = now - (juce::int64) ((ppq - bar) * spb); setSel (end - (juce::int64) (4 * num * spb), end, false); }
              else setSel (proc.written() - (juce::int64) (proc.sampleRate() * 8), proc.written(), false);
          }, [] { return false; }, acc);
    pill ([] { return juce::String ("APRI CARTELLA"); }, [] { Processor::folder().revealToUser(); }, [] { return false; }, C::dim);
    auto sw = [&] (const char* id, juce::Colour c, const char* l) { switches.push_back (std::make_unique<Switch> (P (id), c, l)); addAndMakeVisible (*switches.back()); };
    sw ("snap", acc, "AGGANCIA ALLE BATTUTE"); sw ("loop", acc2, "ASCOLTO IN LOOP"); sw ("pvSolo", acc2, "ASCOLTO DA SOLO"); sw ("norm", acc, "NORMALIZZA"); sw ("rev", acc, "AL CONTRARIO");
    kGain = std::make_unique<Knob> (P ("pvGain"), acc2, "VOLUME ASCOLTO"); kGain->text = [this] { return dbT (proc.value ("pvGain")); }; addAndMakeVisible (*kGain);
    addAndMakeVisible (over); addAndMakeVisible (detail); addAndMakeVisible (dz); addAndMakeVisible (sizeChip);
    scaler.setup(); findTakes(); startTimerHz (30);
}
void Editor::resized()
{
    const float sc = scaler.update();
    over.setBounds (30, 108, 1060, 150);
    detail.setBounds (30, 346, 700, 186);
    dz.setBounds (750, 346, 340, 96);
    for (int i = 0; i < 2; ++i) pills[(size_t) i]->setBounds (750 + i * 172, 452, 166, 36);
    pills[2]->setBounds (750, 496, 166, 34);
    pills[5]->setBounds (922, 496, 166, 34);
    pills[3]->setBounds (30, 580, 160, 34); pills[4]->setBounds (196, 580, 170, 34);
    for (size_t i = 0; i < switches.size(); ++i) switches[i]->setBounds (390 + (int) (i % 3) * 190, 576 + (int) (i / 3) * 30, 186, 22);
    kGain->setBounds (1000, 570, 64, 64 + 27);
    sizeChip.setBounds (W - 76, H - 18, 58, 15);
    jlook::scaleChildren (*this, sc);
}
void Editor::paint (juce::Graphics& g)
{
    g.addTransform (juce::AffineTransform::scale (scaler.s));
    jlook::backdrop (g, { 0, 0, (float) W, (float) H }, acc, acc2);
    jlook::header (g, { 0, 0, (float) W, 64 }, "REWIND", acc, "JR-8  REGISTRATORE RETROATTIVO", acc, acc2);
    titledCard (g, { 18, 76, 1084, 222 }, "MEMORIA", "clic su una presa = selezionala" + dotS() + "trascina = scegli" + dotS() + "doppio clic = togli", acc);
    titledCard (g, { 18, 308, 1084, 374 }, "SELEZIONE", "", acc2);
    // prese come chip
    takeChips.clear(); float x = 30;
    const int first = juce::jmax (0, (int) takes.size() - 7);
    for (int i = (int) takes.size() - 1; i >= first; --i)
    {
        const auto& t = takes[(size_t) i]; const double ago = (double) (proc.written() - t.b) / proc.sampleRate(), len = (double) (t.b - t.a) / proc.sampleRate();
        const juce::String s = (t.starred ? juce::String::fromUTF8 ("\xe2\x98\x85 ") : juce::String()) + timeText (ago) + " fa" + dotS() + timeText (len) + (barsText (t.a, t.b).isNotEmpty() ? dotS() + barsText (t.a, t.b) : juce::String());
        const float w = juce::GlyphArrangement::getStringWidth (mf (9.5f), s) + 22;
        if (x + w > 1090) break;
        auto r = juce::Rectangle<float> (x, 266, w, 24); takeChips.push_back (r);
        const bool sel = selA == t.a && selB == t.b;
        lcd (g, r, 6, t.starred ? C::amber : acc); g.setColour ((t.starred ? C::amber : acc).withAlpha (sel ? 0.9f : 0.4f)); g.drawRoundedRectangle (r, 6, sel ? 1.5f : 1.0f);
        g.setColour (sel ? C::text : C::dim); g.setFont (mf (9.5f)); g.drawText (s, r, juce::Justification::centred);
        x += w + 6;
    }
    if (takes.empty()) { g.setColour (C::dim); g.setFont (mf (9.5f)); g.drawText ("le prese appaiono qui da sole quando suoni qualcosa", juce::Rectangle<float> (30, 266, 600, 24), juce::Justification::centredLeft); }
    // informazioni della selezione
    auto info = juce::Rectangle<float> (30, 538, 700, 30);
    if (hasSel())
    {
        double ppq, bpm; int num; const bool mus = proc.musicalAt (selA, ppq, bpm, num);
        juce::String t = "DURATA " + juce::String ((double) (selB - selA) / proc.sampleRate(), 2) + " s";
        if (mus) t << dotS() << barsText (selA, selB) << dotS() << juce::String (bpm, 1) << " BPM";
        if (selKey.isNotEmpty()) t << dotS() << "TONALITA " << selKey;
        glowText (g, t, info, acc2, mf (12.0f));
    }
    if (lastFile.isNotEmpty()) { g.setColour (C::dim); g.setFont (mf (9.0f)); g.drawText ("salvato: " + lastFile, juce::Rectangle<float> (30, 650, 960, 14), juce::Justification::centredLeft); }
}
void Editor::mouseDown (const juce::MouseEvent& e0)
{
    const auto e = e0.withNewPosition (e0.position / scaler.s);
    const int first = juce::jmax (0, (int) takes.size() - 7);
    for (size_t k = 0; k < takeChips.size(); ++k) if (takeChips[k].contains (e.position)) { const auto& t = takes[(size_t) ((int) takes.size() - 1 - (int) k)]; juce::ignoreUnused (first); setSel (t.a, t.b); return; }
}
void Editor::timerCallback()
{
    over.repaint();
    if (proc.pvOn.load() || tick % 10 == 0) detail.repaint();
    if (++tick % 15 == 0) { findTakes(); repaint(); }
    if (tick % 3 == 0) { for (auto& p : pills) p->repaint(); for (auto& s : switches) s->repaint(); dz.repaint(); }
    // tonalita della selezione (solo quando cambia, massimo 60 s)
    if (hasSel() && (selA != keyA || selB != keyB) && tick % 8 == 0)
    {
        keyA = selA; keyB = selB; juce::AudioBuffer<float> b; proc.readRange (selA, juce::jmin (selB, selA + (juce::int64) (proc.sampleRate() * 60)), b);
        selKey = proc.keyOf (b); repaint();
    }
}
}
