#include "Library.h"
#include "KeyDetect.h"
#include "Generator.h"
#include <map>

namespace jp
{
const juce::StringArray& libCategories()
{
    static const juce::StringArray c { "KICK", "SNARE", "CLAP / SNAP", "HAT", "OPEN HAT", "808", "PERC", "TOM", "CYMBAL", "MELODICI", "TEXTURE", "FX", "VOCI", "LOOP" };
    return c;
}

juce::File libFolder (const juce::String& category)
{
    auto d = Processor::baseFolder().getChildFile ("Samples")
                 .getChildFile (juce::File::createLegalFileName (category.replace (" / ", " e ")));
    d.createDirectory(); return d;
}

juce::String categoryOf (const DrumDesc& d)
{
    const auto n = d.name.toLowerCase();
    switch (d.type)
    {
        case S_KICK: return "KICK";
        case S_808: return n.contains ("kick") ? "KICK" : "808";
        case S_SNARE: return "SNARE";
        case S_CLAP: case S_SNAP: return "CLAP / SNAP";
        case S_HAT: return (n.contains ("open") || n.contains ("apert")) ? "OPEN HAT" : "HAT";
        case S_TOM: return "TOM";
        case S_CYMBAL: return "CYMBAL";
        case S_STAB: case S_PLUCK: return "MELODICI";
        case S_NOISE: return "TEXTURE";
        default: return "PERC";
    }
}

static const std::vector<DrumDesc>& factorySounds()
{
    static std::vector<DrumDesc> all = []
    {
        std::vector<DrumDesc> v; juce::StringArray seen;
        auto add = [&] (const DrumDesc& d) { const auto key = descToPath (d); if (! seen.contains (key)) { seen.add (key); v.push_back (d); } };
        for (auto& k : factoryKits()) for (auto& p : k.pads) add (p.d);
        // varianti in più per ogni categoria
        const float kicks[][5] = { { 110, 45, 0.6f, 0.3f, 0.2f }, { 180, 55, 0.25f, 0.9f, 0.6f }, { 140, 50, 0.4f, 0.5f, 0.0f }, { 200, 60, 0.2f, 1.0f, 0.8f }, { 90, 42, 0.7f, 0.1f, 0.4f }, { 160, 48, 0.32f, 0.7f, 0.9f } };
        const char* kn[] = { "Kick Rotondo", "Kick Click", "Kick Pulito", "Kick Tagliente", "Kick Profondo", "Kick Saturo" };
        for (int i = 0; i < 6; ++i) add ({ S_KICK, kicks[i][0], kicks[i][1], kicks[i][2], kicks[i][3], kicks[i][4], kn[i] });
        const float sn[][5] = { { 230, 0.2f, 0.1f, 1.0f, 0.4f }, { 170, 0.35f, 0.15f, 0.3f, 0.7f }, { 280, 0.12f, 0.05f, 0.9f, 0.2f }, { 200, 0.28f, 0.12f, 0.5f, 0.9f } };
        const char* snn[] = { "Snare Secco", "Snare Fat", "Snare Piccolo", "Snare Legno" };
        for (int i = 0; i < 4; ++i) add ({ S_SNARE, sn[i][0], sn[i][1], sn[i][2], sn[i][3], sn[i][4], snn[i] });
        add ({ S_CLAP, 14, 0.3f, 900, 0.7f, 0, "Clap Stanza" }); add ({ S_CLAP, 7, 0.12f, 1800, 0.0f, 0, "Clap Secco" }); add ({ S_SNAP, 0.15f, 2600, 0, 0, 0, "Snap Lungo" });
        add ({ S_HAT, 0.025f, 11, 0.8f, 0, 0, "Hat Tick" }); add ({ S_HAT, 0.08f, 5.5f, 0.2f, 0, 0, "Hat Caldo" }); add ({ S_HAT, 0.06f, 8.5f, 1.0f, 0, 0, "Hat Metallico" });
        add ({ S_HAT, 0.7f, 8, 0.5f, 0, 0, "Open Hat Lungo" }); add ({ S_HAT, 0.25f, 10, 0.8f, 0, 0, "Open Hat Corto" });
        static const char* notes[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        for (int n = 0; n < 12; ++n) add ({ S_808, (float) (32.703 * std::pow (2.0, n / 12.0)), 1.3f, 0.35f, 0.3f, 0.4f, juce::String ("808 ") + notes[n] + "1" });
        add ({ S_TOM, 75, 0.5f, 0.5f, 0, 0, "Tom Floor" }); add ({ S_TOM, 240, 0.25f, 0.3f, 0, 0, "Tom Rack" });
        add ({ S_CYMBAL, 3.0f, 0.3f, 0, 0, 0, "Crash Lungo" }); add ({ S_CYMBAL, 1.4f, 0.1f, 0.9f, 0, 0, "Ride Bell" }); add ({ S_CYMBAL, 0.8f, 0.6f, 0, 0, 0, "Splash" });
        add ({ S_RIM, 2400, 0.03f, 0, 0, 0, "Rim Alto" }); add ({ S_CLAVE, 3000, 0.03f, 0, 0, 0, "Clave Alta" }); add ({ S_CONGA, 450, 0.18f, 0, 0, 0, "Bongo" });
        add ({ S_BELL, 820, 1220, 0.15f, 0, 0, "Bell Alta" }); add ({ S_SHAKER, 0.25f, 4, 0, 0, 0, "Shaker Lungo" });
        add ({ S_NOISE, 4.0f, 8, 0.1f, 0, 0, "Nastro Hiss" }); add ({ S_NOISE, 4.0f, 2.5f, 1.0f, 0, 0, "Vinile Crackle" });
        return v;
    }();
    return all;
}

std::vector<LibItem> libItems (const juce::String& category)
{
    std::vector<LibItem> out;
    for (auto& d : factorySounds()) if (categoryOf (d) == category) out.push_back ({ d.name, category, false, d, {} });
    std::sort (out.begin(), out.end(), [] (const LibItem& a, const LibItem& b) { return a.name.compareNatural (b.name) < 0; });
    for (size_t i = 1, k = 2; i < out.size(); ++i) { if (out[i].d.name == out[i - 1].d.name) out[i].name = out[i].d.name + " " + juce::String ((int) k++); else k = 2; }
    auto files = libFolder (category).findChildFiles (juce::File::findFiles, true, "*.wav;*.aif;*.aiff;*.flac;*.mp3;*.ogg");
    std::sort (files.begin(), files.end(), [] (const juce::File& a, const juce::File& b) { return a.getFileName().compareNatural (b.getFileName()) < 0; });
    for (auto& f : files) { LibItem it; it.name = f.getFileNameWithoutExtension(); it.category = category; it.user = true; it.file = f; out.push_back (it); }
    // campioni trovati dallo SCAN (restano dove sono)
    juce::SharedResourcePointer<SampleIndex> idx;
    auto sc = idx->entries (category);
    std::sort (sc.begin(), sc.end(), [] (const ScanEntry& a, const ScanEntry& b) { return juce::File (a.path).getFileName().compareNatural (juce::File (b.path).getFileName()) < 0; });
    for (auto& e : sc) { LibItem it; it.file = juce::File (e.path); it.name = it.file.getFileNameWithoutExtension(); it.category = category; it.user = true; it.scanned = true; it.dur = e.dur; out.push_back (it); }
    return out;
}

std::vector<LibItem> searchLibrary (const juce::String& query, size_t maxItems)
{
    std::vector<LibItem> out; const auto words = juce::StringArray::fromTokens (query.toLowerCase(), " ", "");
    for (auto& c : libCategories())
        for (auto& it : libItems (c))
        {
            const auto hay = (it.name + " " + (it.scanned ? it.file.getParentDirectory().getFileName() : juce::String())).toLowerCase();
            bool ok = true; for (auto& w : words) if (w.isNotEmpty() && ! hay.contains (w)) ok = false;
            if (ok) { out.push_back (it); if (out.size() >= maxItems) return out; }
        }
    return out;
}

bool moveLibItem (const LibItem& it, const juce::String& category)
{
    if (! it.user || it.category == category) return false;
    if (it.scanned) { juce::SharedResourcePointer<SampleIndex> idx; idx->setCategory (it.file.getFullPathName(), category); return true; }
    if (! it.file.existsAsFile()) return false;
    return it.file.moveFileTo (libFolder (category).getNonexistentChildFile (it.file.getFileNameWithoutExtension(), it.file.getFileExtension(), false));
}

bool loadLibItem (Processor& P, const LibItem& it, int p)
{
    auto& pd = P.pad[p];
    pd.v[START] = 0; pd.v[END] = 1; pd.v[SLICE] = 0; pd.v[WARP] = 0; pd.v[SBPM] = 0; pd.v[LOOP] = 0;
    pd.v[SKEY] = -1;
    if (it.user) { const bool ok = P.loadFile (it.file, p); if (ok) { P.padCat[p] = it.category; P.applyKeyLock (p); } return ok; }
    P.setSample (p, makeDrum (it.d)); P.padCat[p] = it.category;
    P.detectKey (p); P.applyKeyLock (p);
    return true;
}

bool loadLibLayer (Processor& P, const LibItem& it, int p)
{
    if (it.user) return P.loadLayerFile (it.file, p);
    P.setLayer (p, makeDrum (it.d)); return true;
}

SamplePtr libItemSample (Processor& P, const LibItem& it)
{
    if (! it.user) return makeDrum (it.d);
    std::unique_ptr<juce::AudioFormatReader> rd (P.formats.createReaderFor (it.file));
    if (rd == nullptr || rd->lengthInSamples < 8) return nullptr;
    const int len = (int) std::min<long long> ((long long) rd->lengthInSamples, (long long) (rd->sampleRate * 12));
    auto s = std::make_shared<SampleData>(); s->buf.setSize (2, len); rd->read (&s->buf, 0, len, 0, true, true);
    if (rd->numChannels == 1) s->buf.copyFrom (1, 0, s->buf, 0, 0, len);
    s->sr = rd->sampleRate; s->name = it.name; s->path = it.file.getFullPathName();
    return s;
}

bool addFileToLibrary (const juce::File& src, const juce::String& category)
{
    if (! src.existsAsFile()) return false;
    auto dst = libFolder (category).getNonexistentChildFile (src.getFileNameWithoutExtension(), src.getFileExtension(), false);
    return src.copyFileTo (dst);
}

bool savePadToLibrary (Processor& P, int p, const juce::String& category)
{
    auto s = P.getSample (p); if (! s) return false;
    const int n = s->buf.getNumSamples();
    const int a = juce::jlimit (0, n - 1, (int) (P.pad[p].get (START) * n)), b = juce::jlimit (a + 1, n, (int) (P.pad[p].get (END) * n));
    auto f = libFolder (category).getNonexistentChildFile (juce::File::createLegalFileName (s->name), ".wav", false);
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::OutputStream> os (f.createOutputStream()); if (os == nullptr) return false;
    std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (os.get(), s->sr, 2, 24, {}, 0)); if (w == nullptr) return false;
    os.release();
    juce::AudioBuffer<float> part (2, b - a); part.copyFrom (0, 0, s->buf, 0, a, b - a); part.copyFrom (1, 0, s->buf, juce::jmin (1, s->buf.getNumChannels() - 1), a, b - a);
    return w->writeFromAudioSampleBuffer (part, 0, b - a);
}

const char* kRandModes[3] = { "FABBRICA", "LIBRERIA", "SOLO MIEI" };

static int catColour (const juce::String& c)
{
    static const std::map<juce::String, int> m { { "KICK", 0 }, { "SNARE", 1 }, { "CLAP / SNAP", 2 }, { "HAT", 3 }, { "OPEN HAT", 4 }, { "808", 14 }, { "PERC", 5 }, { "TOM", 11 },
                                                 { "CYMBAL", 12 }, { "MELODICI", 8 }, { "TEXTURE", 15 }, { "FX", 6 }, { "VOCI", 10 }, { "LOOP", 7 } };
    auto it = m.find (c); return it == m.end() ? 6 : it->second;
}
static float maxDur (const juce::String& c)
{
    if (c == "KICK" || c == "SNARE" || c == "CLAP / SNAP" || c == "HAT" || c == "PERC" || c == "TOM") return 1.6f;
    if (c == "OPEN HAT") return 2.5f; if (c == "808") return 4.0f; if (c == "CYMBAL") return 6.0f;
    if (c == "MELODICI" || c == "VOCI" || c == "FX") return 12.0f;
    return 40.0f;
}

// Kit casuale con criterio: ruoli fissi per pad (1-8 batteria, 9-16 fx/voci/melodie/loop), suoni pescati per
// categoria dalla banca scelta, parti tonali portate tutte nella stessa tonalita. 1 volta su 5: kit sperimentale.
void randomKit (Processor& P, int mode)
{
    juce::Random r;
    // pad 1-8: 70% classico, 15% variato (tom/perc e piatto diventano altri one shot), 15% sperimentale
    // pad 9-16: 70% almeno 4 campioni (uno e sempre 808/basso) + altri one shot, 30% inventato
    const float r0 = r.nextFloat();
    const int first = r0 < 0.70f ? 0 : r0 < 0.85f ? 1 : 2;
    const bool invent = r.nextFloat() < 0.30f;
    const bool weird = first == 2;
    std::map<juce::String, std::vector<LibItem>> pools;
    auto pool = [&] (const juce::String& cat) -> std::vector<LibItem>&
    {
        auto f = pools.find (cat); if (f != pools.end()) return f->second;
        std::vector<LibItem> mine, fac;
        for (auto& it : libItems (cat)) { if (it.scanned && it.dur > maxDur (cat)) continue; (it.user ? mine : fac).push_back (it); }
        std::vector<LibItem> v;
        if (mode == 0) v = fac;
        else if (mode == 2) v = mine;
        else { v = (! mine.empty() && (fac.empty() || r.nextFloat() < 0.65f)) ? mine : fac; }
        return pools[cat] = v;
    };
    // ruoli: categorie possibili in ordine di preferenza
    std::vector<juce::StringArray> roles = {
        { "KICK" }, { "SNARE" }, { "HAT" }, { "OPEN HAT", "HAT" }, { "CLAP / SNAP", "PERC", "SNARE" }, { "TOM", "PERC" }, { "PERC", "TOM" }, { "CYMBAL", "OPEN HAT" },
        { "808", "KICK" }, { "MELODICI" }, { "MELODICI" }, { "MELODICI" }, { "PERC" }, { "FX" }, { "PERC" }, { "FX" } };
    auto pick = [&r] (std::initializer_list<const char*> l) { std::vector<const char*> v (l); return juce::String (v[(size_t) r.nextInt ((int) v.size())]); };
    if (first == 1)
        for (int q = 5; q < 8; ++q) roles[(size_t) q] = { pick ({ "VOCI", "FX", "PERC", "TEXTURE", "CLAP / SNAP", "VOCI" }), "PERC", "FX" };
    if (first == 2)
    {
        roles[0] = { pick ({ "KICK", "808", "KICK", "FX" }), "KICK" }; roles[1] = { pick ({ "SNARE", "CLAP / SNAP", "FX", "PERC" }), "SNARE" };
        roles[2] = { pick ({ "HAT", "PERC", "TEXTURE" }), "HAT" };
        for (int q = 3; q < 8; ++q) roles[(size_t) q] = { pick ({ "FX", "TEXTURE", "VOCI", "PERC", "TOM", "CYMBAL", "808", "MELODICI", "CLAP / SNAP" }), "PERC" };
    }
    // pad 9-16: niente colpi di batteria. Pad 9 808/basso, poi quasi tutti campioni lunghi (melodie, loop, voci, texture)
    std::vector<bool> wantLong (kPads, false);
    static const juce::StringArray longCats { "MELODICI", "LOOP", "VOCI", "TEXTURE", "MELODICI", "LOOP" };
    if (! invent)
    {
        const int oneShots = r.nextFloat() < 0.6f ? 0 : 1;
        roles[8] = { "808", "MELODICI" };
        for (int q = 9; q < 16 - oneShots; ++q) { roles[(size_t) q] = { longCats[r.nextInt (longCats.size())], "MELODICI", "LOOP", "TEXTURE" }; wantLong[(size_t) q] = true; }
        for (int q = 16 - oneShots; q < 16; ++q) roles[(size_t) q] = { pick ({ "FX", "VOCI", "TEXTURE" }), "FX", "TEXTURE" };
    }
    else
        for (int q = 8; q < 16; ++q)
        {
            roles[(size_t) q] = { pick ({ "FX", "TEXTURE", "VOCI", "MELODICI", "LOOP", "MELODICI", "808" }), "MELODICI", "TEXTURE", "FX" };
            wantLong[(size_t) q] = roles[(size_t) q][0] != "808" && r.nextFloat() < 0.8f;
        }
    juce::StringArray used;
    // tonalita del kit: la decide la prima parte tonale con tonalita nota (melodie per prime)
    int kitKey = P.keyLock();
    const int order[] = { 9, 10, 11, 8, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7 };
    for (int p : order)
    {
        auto& pd = P.pad[p]; pd.reset(); P.setLayer (p, nullptr);
        bool done = false; juce::String cat;
        // pad 9-16 dei ruoli "campione": serve un suono lungo (loop, frase, melodia), non un altro one shot
        const bool longOnly = wantLong[(size_t) p];
        auto lenOf = [&P, p] { auto s = P.getSample (p); return s ? s->buf.getNumSamples() / s->sr : 0.0; };
        // passaggi: 0 solo lunghi (1.5 s o piu), 1 almeno 0.8 s, 2 qualsiasi
        for (int pass = longOnly ? 0 : 2; pass < 3 && ! done; ++pass)
            for (auto& c : roles[(size_t) p])
            {
                auto& v = pool (c); if (v.empty()) continue;
                const float minLen = pass == 0 ? 1.5f : pass == 1 ? 0.8f : 0.0f;
                for (int t = 0; t < (pass == 0 ? 30 : 10) && ! done; ++t)
                {
                    const auto& it = v[(size_t) r.nextInt ((int) v.size())];
                    if (minLen > 0 && it.scanned && it.dur > 0 && it.dur < minLen) continue;   // durata gia nota: scarta subito
                    const auto key = it.user ? it.file.getFullPathName() : descToPath (it.d);
                    if (used.contains (key) && t < 7) continue;
                    if (loadLibItem (P, it, p))
                    {
                        if (lenOf() < minLen) continue;   // troppo corto: prova un altro
                        used.add (key); done = true; cat = c;
                    }
                }
                if (done) break;
            }
        if (! done && mode == 2)
        {   // niente dei tuoi per questo ruolo: suono di fabbrica dello stesso ruolo
            for (auto& c : roles[(size_t) p])
            {
                std::vector<LibItem> fac; for (auto& it : libItems (c)) if (! it.user) fac.push_back (it);
                if (! fac.empty()) { done = loadLibItem (P, fac[(size_t) r.nextInt ((int) fac.size())], p); cat = c; }
                if (done) break;
            }
        }
        if (! done) { P.setSample (p, nullptr); continue; }
        pd.colour = catColour (cat);
        if ((cat == "HAT" || cat == "OPEN HAT") && p < 8) pd.v[CHOKE] = 1;
        if (cat == "OPEN HAT" || cat == "CYMBAL") pd.v[DECAY] = 4000;
        const bool tonal = cat == "MELODICI" || cat == "808" || cat == "LOOP" || cat == "VOCI";
        if (tonal)
        {
            const int k = juce::roundToInt (pd.get (SKEY));
            if (k >= 0)
            {
                if (kitKey < 0) kitKey = k >= 24 ? 12 + k % 12 : k;
                else P.transposeTo (p, kitKey);
            }
        }
        // piccola variazione umana
        pd.v[LEVEL] = juce::jlimit (0.0f, 1.41f, pd.get (LEVEL) * (0.9f + r.nextFloat() * 0.12f));
        if (! tonal && cat != "KICK" && r.nextFloat() < 0.35f) pd.v[TUNE] = (float) (r.nextInt (3) - 1);
        if (weird)
        {
            if (! tonal && r.nextFloat() < 0.3f) pd.v[TUNE] = (float) (r.nextInt (15) - 7);
            if (p >= 8 && r.nextFloat() < 0.25f) pd.v[REVERSE] = 1;
            if (r.nextFloat() < 0.2f) { pd.v[BITS] = 0.3f + r.nextFloat() * 0.4f; pd.v[RATE] = r.nextFloat() * 0.5f; }
            if (r.nextFloat() < 0.3f) { pd.v[DRIVE] = 0.2f + r.nextFloat() * 0.5f; pd.v[DTYPE] = (float) (4 + r.nextInt (3)); }   // overdrive, distorsione, fuzz
            if (p >= 8 && r.nextFloat() < 0.3f) pd.v[SREV] = 0.2f + r.nextFloat() * 0.4f;
        }
    }
    P.kitName = juce::String ("Random ") + (first == 0 ? "Classico" : first == 1 ? "Variato" : "Sperimentale") + (invent ? "+" : "") + " / " + kRandModes[juce::jlimit (0, 2, mode)] + (kitKey >= 0 ? " in " + keyName (kitKey) : juce::String());
    P.kitFile = juce::File();
}

// Random su un solo pad. 1-8: un altro suono della stessa categoria del pad. 9-16: casuale, quasi sempre lungo.
// Le parti tonali seguono la tonalita fissata o quella del resto del kit.
void randomPad (Processor& P, int p, int mode)
{
    if (! juce::isPositiveAndBelow (p, kPads)) return;
    juce::Random r;
    static const char* drumRole[8] = { "KICK", "SNARE", "HAT", "OPEN HAT", "CLAP / SNAP", "TOM", "PERC", "CYMBAL" };
    juce::StringArray cats; bool longOnly = false;
    if (p < 8)
    {
        auto c = P.padCat[p];
        if (c.isEmpty())
        {   // categoria non nota: deducila dal ruolo del suono che c'e, altrimenti dal ruolo del pad
            switch (padRole (P, p))
            {
                case R_KICK: c = "KICK"; break; case R_SNARE: c = "SNARE"; break; case R_CLAP: c = "CLAP / SNAP"; break; case R_HAT: c = "HAT"; break;
                case R_OPEN: c = "OPEN HAT"; break; case R_TOM: c = "TOM"; break; case R_CYM: c = "CYMBAL"; break; case R_808: c = "808"; break;
                case R_PERC: c = "PERC"; break; case R_FX: c = "FX"; break; case R_VOX: c = "VOCI"; break; case R_TEX: c = "TEXTURE"; break;
                case R_MEL: c = "MELODICI"; break; case R_LOOP: c = "LOOP"; break; default: c = drumRole[p]; break;
            }
        }
        cats.add (c);
    }
    else
    {
        const float x = r.nextFloat();
        if (x < 0.85f) { static const char* l[] = { "MELODICI", "LOOP", "VOCI", "TEXTURE", "MELODICI", "LOOP" }; cats.add (l[r.nextInt (6)]); cats.addArray ({ "MELODICI", "LOOP", "TEXTURE" }); longOnly = true; }
        else if (x < 0.93f) cats.addArray ({ "808", "MELODICI" });
        else cats.addArray ({ r.nextBool() ? "FX" : "VOCI", "FX", "TEXTURE" });
    }
    const juce::String curPath = P.getSample (p) ? P.getSample (p)->path : juce::String();
    const int col = P.pad[p].colour.load(); const float choke = P.pad[p].get (CHOKE);
    auto lenOf = [&P, p] { auto s = P.getSample (p); return s ? s->buf.getNumSamples() / s->sr : 0.0; };
    bool done = false; juce::String cat;
    for (int pass = longOnly ? 0 : 2; pass < 3 && ! done; ++pass)
        for (auto& c : cats)
        {
            std::vector<LibItem> mine, fac, v;
            for (auto& it : libItems (c)) { if (it.scanned && it.dur > maxDur (c)) continue; (it.user ? mine : fac).push_back (it); }
            v = mode == 0 ? fac : mode == 2 ? mine : (! mine.empty() && (fac.empty() || r.nextFloat() < 0.65f)) ? mine : fac;
            if (v.empty() && mode == 2) v = fac;
            if (v.empty()) continue;
            const float minLen = pass == 0 ? 1.5f : pass == 1 ? 0.8f : 0.0f;
            for (int t = 0; t < 30 && ! done; ++t)
            {
                const auto& it = v[(size_t) r.nextInt ((int) v.size())];
                if (minLen > 0 && it.scanned && it.dur > 0 && it.dur < minLen) continue;
                const auto key = it.user ? it.file.getFullPathName() : descToPath (it.d);
                if (key == curPath && v.size() > 1) continue;   // un suono diverso da quello di prima
                P.pad[p].reset(); P.setLayer (p, nullptr);
                if (loadLibItem (P, it, p) && lenOf() >= minLen) { done = true; cat = c; }
            }
            if (done) break;
        }
    if (! done) return;
    auto& pd = P.pad[p]; pd.colour = col;
    if (p < 8) pd.v[CHOKE] = choke;
    if (cat == "OPEN HAT" || cat == "CYMBAL") pd.v[DECAY] = 4000;
    if (P.keyLock() < 0 && P.isTonalPad (p) && pd.get (SKEY) >= 0)
    {
        const int k = P.kitEffectiveKey (p);
        if (k >= 0) P.transposeTo (p, k);
    }
}
}
