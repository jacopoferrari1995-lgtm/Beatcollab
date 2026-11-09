#include "SampleScan.h"
#include <set>

namespace jp
{
static const char* kWild = "*.wav;*.aif;*.aiff;*.flac;*.mp3;*.ogg";

SampleIndex::SampleIndex() : juce::Thread ("Jackpot sample scan") { load(); }
SampleIndex::~SampleIndex() { stopThread (6000); }

juce::File SampleIndex::file()
{
    auto d = juce::File::getSpecialLocation (juce::File::userMusicDirectory).getChildFile ("Jackpot Sampler");
    d.createDirectory(); return d.getChildFile ("scan-index.xml");
}

void SampleIndex::load()
{
    const juce::ScopedLock sl (lock);
    items.clear(); byPath.clear(); rootList.clear();
    auto xml = juce::XmlDocument::parse (file()); if (xml == nullptr) { recount(); return; }
    for (auto* r : xml->getChildWithTagNameIterator ("ROOT")) rootList.addIfNotAlreadyThere (r->getStringAttribute ("p"));
    for (auto* s : xml->getChildWithTagNameIterator ("S"))
    {
        ScanEntry e; e.path = s->getStringAttribute ("p"); e.cat = s->getStringAttribute ("c");
        e.mtime = (juce::int64) s->getStringAttribute ("t").getLargeIntValue(); e.dur = (float) s->getDoubleAttribute ("d"); e.manual = s->getBoolAttribute ("m");
        if (e.path.isEmpty() || e.cat.isEmpty() || byPath.count (e.path)) continue;
        byPath[e.path] = items.size(); items.push_back (e);
    }
    recount();
}

void SampleIndex::save()
{
    juce::XmlElement x ("JACKPOT_SCAN");
    {
        const juce::ScopedLock sl (lock);
        for (auto& r : rootList) x.createNewChildElement ("ROOT")->setAttribute ("p", r);
        for (auto& e : items)
        {
            auto* s = x.createNewChildElement ("S");
            s->setAttribute ("p", e.path); s->setAttribute ("c", e.cat); s->setAttribute ("t", juce::String (e.mtime));
            s->setAttribute ("d", juce::String (e.dur, 3)); if (e.manual) s->setAttribute ("m", 1);
        }
    }
    auto f = file(), tmp = f.getSiblingFile ("scan-index.tmp");
    if (x.writeTo (tmp)) tmp.moveFileTo (f);
}

void SampleIndex::recount()
{
    counts.clear(); for (auto& e : items) ++counts[e.cat];
    ++generation;
}

juce::StringArray SampleIndex::roots() const { const juce::ScopedLock sl (lock); return rootList; }
void SampleIndex::addRoot (const juce::File& d) { { const juce::ScopedLock sl (lock); rootList.addIfNotAlreadyThere (d.getFullPathName()); } startScan(); }
void SampleIndex::removeRoot (const juce::String& r)
{
    stopThread (6000);
    {
        const juce::ScopedLock sl (lock);
        rootList.removeString (r);
        const juce::File root (r);
        std::vector<ScanEntry> keep; for (auto& e : items) if (! juce::File (e.path).isAChildOf (root)) keep.push_back (e);
        items = std::move (keep); byPath.clear(); for (size_t i = 0; i < items.size(); ++i) byPath[items[i].path] = i;
        recount();
    }
    save();
}
void SampleIndex::clearAll()
{
    stopThread (6000);
    { const juce::ScopedLock sl (lock); items.clear(); byPath.clear(); rootList.clear(); recount(); }
    save();
}
void SampleIndex::startScan() { if (isThreadRunning()) return; done = 0; total = 0; added = 0; startThread (juce::Thread::Priority::low); }

std::vector<ScanEntry> SampleIndex::entries (const juce::String& cat) const
{
    const juce::ScopedLock sl (lock); std::vector<ScanEntry> out;
    for (auto& e : items) if (e.cat == cat) out.push_back (e);
    return out;
}
int SampleIndex::count (const juce::String& cat) const { const juce::ScopedLock sl (lock); auto it = counts.find (cat); return it == counts.end() ? 0 : it->second; }
int SampleIndex::size() const { const juce::ScopedLock sl (lock); return (int) items.size(); }
bool SampleIndex::contains (const juce::String& p) const { const juce::ScopedLock sl (lock); return byPath.count (p) > 0; }
void SampleIndex::setCategory (const juce::String& p, const juce::String& cat)
{
    { const juce::ScopedLock sl (lock); auto it = byPath.find (p); if (it == byPath.end()) return; items[it->second].cat = cat; items[it->second].manual = true; recount(); }
    if (! isThreadRunning()) save();
}
void SampleIndex::remove (const juce::String& p)
{
    {
        const juce::ScopedLock sl (lock); auto it = byPath.find (p); if (it == byPath.end()) return;
        items.erase (items.begin() + (long) it->second); byPath.clear(); for (size_t i = 0; i < items.size(); ++i) byPath[items[i].path] = i; recount();
    }
    if (! isThreadRunning()) save();
}

// ---------------------------------------------------------------- scansione
void SampleIndex::run()
{
    juce::AudioFormatManager fm; fm.registerBasicFormats();
    const auto rs = roots();
    std::vector<juce::File> files;
    for (auto& r : rs)
    {
        const juce::File root (r); if (! root.isDirectory()) continue;
        for (const auto& it : juce::RangedDirectoryIterator (root, true, kWild, juce::File::findFiles, juce::File::FollowSymlinks::no))
        {
            if (threadShouldExit()) return;
            const auto& f = it.getFile();
            if (f.getFileName().startsWithChar ('.')) continue;   // file nascosti / risorse macOS "._"
            files.push_back (f);
            if ((files.size() & 255) == 0) total = (int) files.size();
        }
    }
    total = (int) files.size();
    std::set<juce::String> seen;
    int sinceSave = 0;
    for (size_t i = 0; i < files.size(); ++i)
    {
        if (threadShouldExit()) break;
        const auto& f = files[i]; const auto path = f.getFullPathName(); seen.insert (path);
        const auto mt = f.getLastModificationTime().toMilliseconds();
        bool need = true; bool manual = false; juce::String manualCat;
        {
            const juce::ScopedLock sl (lock);
            auto it = byPath.find (path);
            if (it != byPath.end()) { const auto& e = items[it->second]; need = e.mtime != mt; manual = e.manual; manualCat = e.cat; }
        }
        if (need)
        {
            float dur = 0; auto cat = classify (f, fm, dur);
            if (cat.isNotEmpty())
            {
                const juce::ScopedLock sl (lock);
                ScanEntry e { path, manual ? manualCat : cat, mt, dur, manual };
                auto it = byPath.find (path);
                if (it != byPath.end()) items[it->second] = e; else { byPath[path] = items.size(); items.push_back (e); ++added; }
                if ((++sinceSave & 63) == 0) recount();
            }
        }
        done = (int) i + 1;
    }
    {
        const juce::ScopedLock sl (lock);
        if (! threadShouldExit())
        {   // via i file spariti dalle cartelle scansionate
            std::vector<juce::File> rootFiles; for (auto& r : rs) rootFiles.push_back (juce::File (r));
            std::vector<ScanEntry> keep;
            for (auto& e : items)
            {
                bool under = false; const juce::File ef (e.path);
                for (auto& rf : rootFiles) if (ef.isAChildOf (rf)) under = true;
                if (! under || seen.count (e.path)) keep.push_back (e);
            }
            items = std::move (keep);
        }
        byPath.clear(); for (size_t k = 0; k < items.size(); ++k) byPath[items[k].path] = k;
        recount();
    }
    save();
    ++generation;
}

// ---------------------------------------------------------------- riconoscimento dal nome
static juce::StringArray tokensOf (const juce::String& s0)
{
    // separa anche camelCase e numeri: "OpenHat02" -> open hat 02
    juce::String s;
    for (int i = 0; i < s0.length(); ++i)
    {
        const auto c = s0[i];
        if (i > 0 && juce::CharacterFunctions::isUpperCase (c) && juce::CharacterFunctions::isLowerCase (s0[i - 1])) s << ' ';
        if (i > 0 && juce::CharacterFunctions::isDigit (c) != juce::CharacterFunctions::isDigit (s0[i - 1]) && juce::CharacterFunctions::isLetterOrDigit (s0[i - 1])) s << ' ';
        s << (juce::CharacterFunctions::isLetterOrDigit (c) ? juce::CharacterFunctions::toLowerCase (c) : (juce::juce_wchar) ' ');
    }
    auto t = juce::StringArray::fromTokens (s, " ", ""); t.removeEmptyStrings();
    return t;
}

struct Rule { const char* cat; std::initializer_list<const char*> words; };

// parole intere (anche al plurale): l'ordine conta, la prima regola che trova vince
static const Rule kRules[] = {
    { "VOCI", { "vocal", "vocals", "vox", "voice", "voices", "acapella", "acappella", "accapella", "chant", "adlib", "adlibs", "choir", "speech", "spoken", "shout", "yeah", "hey" } },
    { "FX", { "fx", "sfx", "riser", "risers", "uplifter", "downlifter", "sweep", "sweeps", "impact", "impacts", "whoosh", "swoosh", "transition", "laser", "zap", "glitch", "siren", "gunshot", "gun", "explosion", "scratch", "scratches", "reverse", "reversed", "rev", "downer", "noisefx", "effect", "effects", "foley" } },
    { "KICK", { "kick", "kicks", "kik", "kck", "bd", "bassdrum" } },
    { "808", { "808", "808s", "sub", "subbass", "bass", "basses", "reese", "bs" } },
    { "CLAP / SNAP", { "clap", "claps", "clp", "snap", "snaps", "snp", "fingersnap" } },
    { "SNARE", { "snare", "snares", "snr", "sn", "sd", "rimshot" } },
    { "OPEN HAT", { "openhat", "ohh", "oh", "ohat", "ophat" } },
    { "HAT", { "hihat", "hihats", "hat", "hats", "hh", "chh", "ch", "closedhat", "chat" } },
    { "CYMBAL", { "crash", "crashes", "ride", "rides", "cymbal", "cymbals", "cym", "splash", "china" } },
    { "TOM", { "tom", "toms" } },
    { "PERC", { "perc", "percs", "percussion", "shaker", "shakers", "shk", "conga", "congas", "bongo", "bongos", "rim", "rims", "clave", "cowbell", "tamb", "tambourine", "block", "triangle", "cabasa", "guiro", "tabla", "djembe", "timbale", "agogo", "click", "knock", "wood", "woodblock" } },
    { "MELODICI", { "melody", "melodies", "melodic", "mel", "lead", "leads", "keys", "key", "piano", "synth", "synths", "chord", "chords", "pluck", "plucks", "bell", "bells", "guitar", "gtr", "string", "strings", "flute", "brass", "sax", "horn", "horns", "organ", "rhodes", "ep", "arp", "stab", "stabs", "harp", "marimba", "kalimba", "violin", "cello", "pad", "pads", "mallet", "sitar", "instrument" } },
    { "TEXTURE", { "texture", "textures", "ambience", "ambient", "atmos", "atmosphere", "vinyl", "crackle", "field", "drone", "noise", "roomtone", "tape" } },
};

static juce::String catOfTokens (const juce::StringArray& t)
{
    auto has = [&] (const char* w) { return t.contains (w); };
    // combinazioni prima delle parole singole
    if ((has ("open") || has ("opened")) && (has ("hat") || has ("hh") || has ("hihat") || has ("hats"))) return "OPEN HAT";
    if ((has ("closed") || has ("close")) && (has ("hat") || has ("hh") || has ("hihat") || has ("hats"))) return "HAT";
    if (has ("bass") && has ("drum")) return "KICK";
    if (has ("hi") && has ("hat")) return "HAT";
    if (has ("snare") && has ("roll")) return "SNARE";
    for (auto& r : kRules) for (auto* w : r.words) if (has (w)) return r.cat;
    // parole lunghe anche dentro altre parole ("superkick", "trapsnare")
    static const std::pair<const char*, const char*> sub[] = { { "kick", "KICK" }, { "snare", "SNARE" }, { "clap", "CLAP / SNAP" }, { "hihat", "HAT" }, { "openhat", "OPEN HAT" },
        { "cymbal", "CYMBAL" }, { "crash", "CYMBAL" }, { "shaker", "PERC" }, { "perc", "PERC" }, { "vocal", "VOCI" }, { "riser", "FX" }, { "impact", "FX" }, { "sweep", "FX" },
        { "808", "808" }, { "piano", "MELODICI" }, { "guitar", "MELODICI" }, { "synth", "MELODICI" }, { "melod", "MELODICI" }, { "chord", "MELODICI" } };
    const auto all = t.joinIntoString ("");
    for (auto& [w, c] : sub) if (all.contains (w)) return c;
    return {};
}

juce::String SampleIndex::classifyName (const juce::File& f, bool& oneShot, int& loop)
{
    oneShot = false; loop = 0;
    juce::String cat;
    auto scan = [&] (const juce::String& part, bool isName)
    {
        const auto t = tokensOf (part);
        const auto all = t.joinIntoString ("");
        const bool os = all.contains ("oneshot") || t.contains ("os") || all.contains ("1shot") || t.contains ("hit") || t.contains ("hits") || t.contains ("single") || all.contains ("oneshots");
        const bool strong = t.contains ("break") || t.contains ("breaks") || t.contains ("groove") || t.contains ("grooves") || all.contains ("loop");
        const bool weak = t.contains ("bpm");
        if (os) oneShot = true;
        if (! os && (isName || ! oneShot)) { if (strong) loop = 2; else if (weak && isName) loop = juce::jmax (loop, 1); }
        if (cat.isEmpty()) cat = catOfTokens (t);
    };
    scan (f.getFileNameWithoutExtension(), true);
    auto d = f.getParentDirectory();
    for (int k = 0; k < 3 && d.exists() && d != d.getParentDirectory(); ++k, d = d.getParentDirectory()) scan (d.getFileName(), false);
    if (oneShot) loop = 0;
    return cat;
}

// ---------------------------------------------------------------- riconoscimento dall'ascolto
SoundFeatures SampleIndex::analyse (juce::AudioFormatReader& r)
{
    SoundFeatures F; const double sr = r.sampleRate > 0 ? r.sampleRate : 44100.0;
    F.dur = (float) (r.lengthInSamples / sr);
    const int n = (int) juce::jmin<juce::int64> (r.lengthInSamples, (juce::int64) (sr * 6.0));
    if (n < 64) return F;
    juce::AudioBuffer<float> b ((int) juce::jmax (1u, juce::jmin (2u, r.numChannels)), n);
    r.read (&b, 0, n, 0, true, b.getNumChannels() > 1);
    std::vector<float> x ((size_t) n);
    for (int i = 0; i < n; ++i) { float v = 0; for (int c = 0; c < b.getNumChannels(); ++c) v += b.getSample (c, i); x[(size_t) i] = v / b.getNumChannels(); }
    // inviluppo a 2 ms
    const int hop = juce::jmax (1, (int) (sr * 0.002)); std::vector<float> env;
    for (int i = 0; i + hop <= n; i += hop) { float e = 0; for (int k = 0; k < hop; ++k) e += x[(size_t) (i + k)] * x[(size_t) (i + k)]; env.push_back (std::sqrt (e / hop)); }
    if (env.empty()) return F;
    size_t kp = 0; for (size_t k = 1; k < env.size(); ++k) if (env[k] > env[kp]) kp = k;
    const float pk = env[kp]; if (pk < 1e-5f) return F;
    size_t k0 = 0; while (k0 < kp && env[k0] < pk * 0.1f) ++k0;
    F.attack = (float) ((kp - k0) * hop / sr);
    size_t k30 = kp; while (k30 < env.size() && env[k30] > pk * 0.0316f) ++k30;
    F.t30 = (float) ((k30 - kp) * hop / sr);
    if (k30 >= env.size()) F.t30 = juce::jmax (F.t30, F.dur - (float) (kp * hop / sr));
    // micro-colpi nei primi 90 ms (clap): inviluppo fine sulle alte, conta i picchi separati da una vera valle
    {
        juce::IIRFilter h1, h2; h1.setCoefficients (juce::IIRCoefficients::makeHighPass (sr, 900)); h2.setCoefficients (juce::IIRCoefficients::makeHighPass (sr, 900));
        const int a = (int) (k0 * hop), len = juce::jmin (n - a, (int) (sr * 0.09)), w = juce::jmax (1, (int) (sr * 0.003)), st = juce::jmax (1, (int) (sr * 0.001));
        std::vector<float> hx ((size_t) juce::jmax (0, len)); for (int i = 0; i < len; ++i) hx[(size_t) i] = h2.processSingleSampleRaw (h1.processSingleSampleRaw (x[(size_t) (a + i)]));
        std::vector<float> me; for (int i = 0; i + w <= len; i += st) { float e = 0; for (int k = 0; k < w; ++k) e += hx[(size_t) (i + k)] * hx[(size_t) (i + k)]; me.push_back (std::sqrt (e / w)); }
        float mp = 0; for (auto v : me) mp = juce::jmax (mp, v);
        if (mp > 1e-5f)
        {
            float lastPk = 0, valley = 1e9f; bool armed = true;
            for (size_t k = 1; k + 1 < me.size(); ++k)
            {
                valley = juce::jmin (valley, me[k]);
                if (me[k] > me[k - 1] && me[k] >= me[k + 1] && me[k] > mp * 0.3f)
                {
                    if (F.micro == 0 || (armed && valley < 0.6f * juce::jmin (lastPk, me[k]))) { ++F.micro; lastPk = me[k]; valley = 1e9f; armed = true; }
                    else lastPk = juce::jmax (lastPk, me[k]);
                }
            }
        }
    }
    // colpi su tutto il pezzo analizzato: energia su finestre di 25 ms (niente falsi colpi dalle basse frequenze)
    {
        const int w = (int) (sr * 0.025), st = (int) (sr * 0.005); std::vector<float> le;
        for (int i = 0; i + w <= n; i += st) { float e = 0; for (int k = 0; k < w; k += 2) e += x[(size_t) (i + k)] * x[(size_t) (i + k)]; le.push_back (std::log (1e-9f + e / (w / 2))); }
        float mx = -100; for (auto v : le) mx = juce::jmax (mx, v);
        int last = -1000;
        for (size_t k = 3; k < le.size(); ++k)
            if (le[k] - le[k - 3] > 1.6f && le[k] > mx - 4.0f && (int) k - last > 18) { ++F.onsets; last = (int) k; }
    }
    // energie per banda e passaggi per lo zero sul corpo del suono
    {
        const int a = (int) (k0 * hop), len = juce::jmin (n - a, (int) (sr * 0.3));
        if (len > 32)
        {
            juce::IIRFilter lp, hp1, lp2, hp2;
            lp.setCoefficients (juce::IIRCoefficients::makeLowPass (sr, 150)); hp1.setCoefficients (juce::IIRCoefficients::makeHighPass (sr, 150));
            lp2.setCoefficients (juce::IIRCoefficients::makeLowPass (sr, 5000)); hp2.setCoefficients (juce::IIRCoefficients::makeHighPass (sr, 5000));
            double el = 0, em = 0, eh = 0; int zc = 0;
            for (int i = 0; i < len; ++i)
            {
                const float v = x[(size_t) (a + i)];
                const float l = lp.processSingleSampleRaw (v), m = lp2.processSingleSampleRaw (hp1.processSingleSampleRaw (v)), h = hp2.processSingleSampleRaw (v);
                el += l * l; em += m * m; eh += h * h;
                if (i > 0 && (x[(size_t) (a + i)] >= 0) != (x[(size_t) (a + i - 1)] >= 0)) ++zc;
            }
            const double tot = el + em + eh + 1e-12; F.low = (float) (el / tot); F.mid = (float) (em / tot); F.high = (float) (eh / tot);
            F.zcr = (float) (zc / (len / sr));
        }
    }
    // tonalita: autocorrelazione normalizzata (salta il primo lobo per non sbagliare d'ottava)
    auto acf = [&] (int a, float& f0) -> float
    {
        f0 = 0; const int len = juce::jmin (n - a, 4096);
        const int minLag = juce::jmax (2, (int) (sr / 1000.0)), maxLag = juce::jmin (len / 2, (int) (sr / 30.0));
        if (a < 0 || len <= 1024 || maxLag <= minLag) return 0;
        const int m = len - maxLag; double e0 = 0; for (int i = 0; i < m; i += 2) e0 += (double) x[(size_t) (a + i)] * x[(size_t) (a + i)];
        float best = 0; int bl = 0; bool dipped = false;
        for (int lag = minLag; lag <= maxLag; lag += (lag < 200 ? 1 : 2))
        {
            double s2 = 0, e1 = 0;
            for (int i = 0; i < m; i += 2) { s2 += (double) x[(size_t) (a + i)] * x[(size_t) (a + i + lag)]; e1 += (double) x[(size_t) (a + i + lag)] * x[(size_t) (a + i + lag)]; }
            const float rr = (float) (s2 / std::sqrt (juce::jmax (1e-12, e0 * e1)));
            if (rr < 0.25f) dipped = true;
            if (dipped && rr > best) { best = rr; bl = lag; }
        }
        f0 = bl > 0 ? (float) (sr / bl) : 0; return juce::jlimit (0.0f, 1.0f, best);
    };
    F.tonal = acf ((int) (kp * hop + sr * 0.02), F.f0);
    acf ((int) (kp * hop + sr * 0.12), F.f0b);
    return F;
}

juce::String SampleIndex::classifySound (const SoundFeatures& F, bool oneShot, bool loop)
{
    const bool tonal = F.tonal > 0.8f;
    if (! oneShot && (loop || (F.dur > 2.2f && F.onsets >= 4))) return "LOOP";
    if (F.attack > 0.12f) return "FX";   // si gonfia: riser, reverse, swell
    // basso / 808 contro cassa: la cassa scende di intonazione, l'808 resta ferma
    if (F.tonal > 0.85f && F.f0 > 25 && F.f0 < 130 && F.low > 0.5f)
    {
        const float drift = F.f0b > 0 ? F.f0 / F.f0b : 1.0f;
        if (F.f0 > 90 && F.t30 < 0.5f) return "TOM";
        if (drift > 1.25f && F.t30 < 0.8f) return "KICK";
        if (F.t30 < 0.35f && F.f0 > 48) return "KICK";
        return (F.t30 > 0.2f || F.dur > 0.8f) ? "808" : "KICK";
    }
    if (F.low > 0.5f && F.attack < 0.03f && F.t30 < 1.5f && F.dur < 2.5f) return "KICK";
    if (F.dur > 2.5f)
    {
        if (F.tonal > 0.55f) return "MELODICI";
        if (F.high > 0.5f && F.t30 > 1.0f) return "CYMBAL";
        return F.onsets >= 3 ? "LOOP" : "TEXTURE";
    }
    // suoni intonati: melodici se durano, percussioni se brevi
    if (F.tonal > 0.55f && F.high < 0.25f && F.dur > 0.7f && F.t30 > 0.18f && F.low < 0.5f) return "MELODICI";
    if (F.tonal > 0.4f && F.dur > 1.0f && F.high < 0.1f) return "MELODICI";
    if (tonal && F.high < 0.3f)
    {
        if (F.t30 < 0.35f && F.dur < 0.8f) return (F.low > 0.18f && F.f0 < 300) ? "TOM" : "PERC";
        if (F.low > 0.2f && F.f0 > 55 && F.f0 < 300 && F.t30 < 0.8f) return "TOM";
        return F.dur > 0.5f ? "MELODICI" : "PERC";
    }
    if (F.high > 0.62f) { if (F.attack > 0.01f && F.t30 < 0.2f) return "PERC"; return F.t30 < 0.13f ? "HAT" : F.t30 < 0.75f ? "OPEN HAT" : "CYMBAL"; }
    if (F.micro >= 3 && F.tonal < 0.6f && F.low < 0.15f && F.mid + F.high > 0.6f) return "CLAP / SNAP";
    if (F.low < 0.04f && F.high < 0.3f && F.mid > 0.65f && F.zcr < 10000 && F.t30 < 0.5f && F.tonal < 0.6f) return "CLAP / SNAP";   // rumore a banda stretta senza corpo
    if (F.mid + F.high > 0.6f && F.t30 > 0.04f && F.t30 < 0.7f && F.zcr > 1500) return "SNARE";
    if (F.t30 < 0.15f) return "PERC";
    if (tonal) return "MELODICI";
    return "PERC";
}

juce::String SampleIndex::classify (const juce::File& f, juce::AudioFormatManager& fm, float& dur)
{
    bool os = false; int lp = 0; auto cat = classifyName (f, os, lp);
    std::unique_ptr<juce::AudioFormatReader> r (fm.createReaderFor (f));
    if (r == nullptr || r->sampleRate <= 0 || r->lengthInSamples <= 0) return {};
    dur = (float) (r->lengthInSamples / r->sampleRate);
    if (lp == 2 && dur > 0.8f) return "LOOP";
    if (lp == 1 && dur > 1.2f) { const auto F = analyse (*r); if (F.onsets >= 3 || dur > 4.0f) return "LOOP"; if (cat.isEmpty()) return classifySound (F, os, false); }
    if (cat.isNotEmpty())
    {
        // un nome di batteria su un file lungo e pieno di colpi e un loop (es. "Kick 140bpm")
        if (! os && dur > 3.0f && cat != "MELODICI" && cat != "TEXTURE" && cat != "FX" && cat != "VOCI")
        {
            const auto F = analyse (*r); if (F.onsets >= 4) return "LOOP";
        }
        return cat;
    }
    return classifySound (analyse (*r), os, lp == 2 && dur > 0.8f);
}
}
