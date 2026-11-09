#include "Library.h"

namespace jp
{
const juce::StringArray& libCategories()
{
    static const juce::StringArray c { "KICK", "SNARE", "CLAP / SNAP", "HAT", "OPEN HAT", "808", "PERC", "TOM", "CYMBAL", "MELODICI", "TEXTURE" };
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
    return out;
}

bool loadLibItem (Processor& P, const LibItem& it, int p)
{
    auto& pd = P.pad[p];
    pd.v[START] = 0; pd.v[END] = 1; pd.v[SLICE] = 0; pd.v[WARP] = 0; pd.v[SBPM] = 0; pd.v[LOOP] = 0;
    if (it.user) return P.loadFile (it.file, p);
    P.setSample (p, makeDrum (it.d));
    return true;
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
}
