#include "TestLab.h"
#include <cstdlib>
#include <juce_dsp/juce_dsp.h>
#include "../Source/DustProcessor.h"
#include "../Source/DustEditor.h"
using namespace jds;

static void setP (Processor& p, const char* id, float v) { auto* rp = p.apvts.getParameter (id); rp->setValueNotifyingHost (rp->convertTo0to1 (v)); }
static void run (Processor& p, std::function<void (int, float&, float&)> gen, int len, std::vector<float>& L, std::vector<float>& R, int block = 512)
{
    juce::AudioBuffer<float> b (2, block); juce::MidiBuffer mb;
    for (int off = 0; off < len; off += block)
    {
        const int n = juce::jmin (block, len - off); b.setSize (2, n, false, false, true);
        for (int i = 0; i < n; ++i) { float l, r; gen (off + i, l, r); b.setSample (0, i, l); b.setSample (1, i, r); }
        p.processBlock (b, mb);
        for (int i = 0; i < n; ++i) { L.push_back (b.getSample (0, i)); R.push_back (b.getSample (1, i)); }
    }
}
static float rms (const std::vector<float>& x, int a, int b) { double e = 0; for (int i = a; i < b; ++i) e += x[(size_t) i] * x[(size_t) i]; return (float) std::sqrt (e / juce::jmax (1, b - a)); }
static float bandDb (const std::vector<float>& x, int start, double sr, float f0, float f1)
{
    const int order = 14, N = 1 << order; juce::dsp::FFT f (order); std::vector<float> d ((size_t) (2 * N), 0);
    for (int i = 0; i < N; ++i) d[(size_t) i] = x[(size_t) (start + i)] * (0.5f - 0.5f * std::cos (6.2831853f * i / N));
    f.performFrequencyOnlyForwardTransform (d.data(), true);
    double e = 0; for (int k = (int) (f0 * N / sr); k <= (int) (f1 * N / sr); ++k) e += d[(size_t) k] * d[(size_t) k];
    return 10 * (float) std::log10 (e + 1e-20);
}

int main (int argc, char** argv)
{
    if (argc > 1 && juce::String (argv[1]) == "lab")
    {
        juce::ScopedJuceInitialiser_GUI gui2;
        const int pr = juce::SystemStats::getEnvironmentVariable ("JPRESET", "0").getIntValue();
        { Processor q; std::printf ("preset: %s\n", q.getProgramName (pr).toRawUTF8()); }
        testlab::run ([&] (double rate, int blk) -> std::unique_ptr<juce::AudioProcessor>
        {
            auto p = std::make_unique<Processor>(); p->setPlayConfigDetails (2, 2, rate, blk); p->prepareToPlay (rate, blk); p->loadPreset (pr);
            if (auto* js = std::getenv ("JSET")) for (auto kv : juce::StringArray::fromTokens (js, ",", "")) { auto* r = p->apvts.getParameter (kv.upToFirstOccurrenceOf ("=", false, false)); r->setValueNotifyingHost (r->convertTo0to1 (kv.fromFirstOccurrenceOf ("=", false, false).getFloatValue())); }
            return p;
        }, std::getenv ("JNOCLICK") == nullptr, [] (juce::AudioProcessorParameter* q)
        {
            auto* r = dynamic_cast<juce::RangedAudioParameter*> (q); if (r == nullptr) return true;
            if (auto* only = std::getenv ("JONLY")) return ! r->paramID.contains (only);
            return false;
        });
        return 0;
    }

    if (argc > 1 && juce::String (argv[1]) == "drum")
    {   // loop di batteria sintetico (cassa, rullante, hat) a 90 BPM; per ogni preset: quanto cambia il suono
        juce::ScopedJuceInitialiser_GUI gi; const double sr = 48000; const int B = 512;
        const int len = (int) (sr * 60.0 / 90.0 * 8);   // 2 battute
        std::vector<float> in ((size_t) len, 0.0f); juce::Random rnd (7);
        const int beat = (int) (sr * 60.0 / 90.0), e8 = beat / 2;
        auto addKick = [&] (int at, float g) { double ph = 0; for (int i = 0; i < (int) (sr * 0.45) && at + i < len; ++i) { const double t = i / sr, f = 45 + 110 * std::exp (-t * 28); ph += 2 * juce::MathConstants<double>::pi * f / sr; in[(size_t) (at + i)] += g * (float) (std::sin (ph) * std::exp (-t * 6.5) + 0.3 * std::exp (-t * 300) * (rnd.nextFloat() - 0.5)); } };
        auto addSnare = [&] (int at, float g) { float lp = 0; for (int i = 0; i < (int) (sr * 0.3) && at + i < len; ++i) { const double t = i / sr; const float nz = rnd.nextFloat() * 2 - 1; lp += 0.5f * (nz - lp); in[(size_t) (at + i)] += g * (float) (0.55 * (nz - lp) * std::exp (-t * 14) + 0.5 * std::sin (2 * juce::MathConstants<double>::pi * 190 * t) * std::exp (-t * 25)); } };
        auto addHat = [&] (int at, float g) { float hp = 0; for (int i = 0; i < (int) (sr * 0.06) && at + i < len; ++i) { const double t = i / sr; const float nz = rnd.nextFloat() * 2 - 1; hp += 0.7f * (nz - hp); in[(size_t) (at + i)] += g * (nz - hp) * (float) std::exp (-t * 70); } };
        for (int b = 0; b < 8; ++b) { if (b % 4 == 0 || b % 4 == 2) addKick (b * beat, 0.8f); if (b % 4 == 3) addKick (b * beat + e8, 0.5f); if (b % 2 == 1) addSnare (b * beat, 0.6f); for (int h = 0; h < 2; ++h) addHat (b * beat + h * e8, h ? 0.18f : 0.28f); }
        { float pk = 0; for (float v : in) pk = std::max (pk, std::abs (v)); for (float& v : in) v *= 0.89f / pk; }   // picco a -1 dBFS
        auto crest = [&] (const std::vector<float>& x, size_t a) { double e = 0; float pk = 0; for (size_t i = a; i < x.size(); ++i) { e += x[i] * x[i]; pk = std::max (pk, std::abs (x[i])); } const double rms = std::sqrt (e / (x.size() - a)); return std::make_pair (20 * std::log10 (pk + 1e-9), 20 * std::log10 (rms + 1e-9)); };
        auto bands = [&] (const std::vector<float>& x, size_t a, double out[4]) {   // energia sotto 120, 120-1k, 1k-6k, sopra 6k
            const double fc[3] = { 120, 1000, 6000 }; double lp[3] = { 0, 0, 0 }, e[4] = { 0, 0, 0, 0 };
            for (size_t i = 0; i < x.size(); ++i) { double prev = 0, v = x[i]; double parts[4]; double rest = v;
                for (int k = 0; k < 3; ++k) { const double al = 1 - std::exp (-2 * juce::MathConstants<double>::pi * fc[k] / sr); lp[k] += al * (v - lp[k]); }
                parts[0] = lp[0]; parts[1] = lp[1] - lp[0]; parts[2] = lp[2] - lp[1]; parts[3] = v - lp[2]; (void) prev; (void) rest;
                if (i >= a) for (int k = 0; k < 4; ++k) e[k] += parts[k] * parts[k]; }
            for (int k = 0; k < 4; ++k) out[k] = 10 * std::log10 (e[k] / (x.size() - a) + 1e-20); };
        const auto ci = crest (in, (size_t) beat); double bi[4]; bands (in, (size_t) beat, bi);
        std::printf ("ingresso: picco %.1f  rms %.1f  crest %.1f   bande %.1f %.1f %.1f %.1f\n", ci.first, ci.second, ci.first - ci.second, bi[0], bi[1], bi[2], bi[3]);
        const int np = Processor().getNumPrograms();
        for (int pr = 1; pr < np; ++pr)
        {
            if (auto* o = std::getenv ("JPRESET")) if (std::atoi (o) != pr) continue;
            Processor p; p.setPlayConfigDetails (2, 2, sr, B); p.prepareToPlay (sr, B); p.loadPreset (pr);
            if (auto* js = std::getenv ("JSET")) for (auto kv : juce::StringArray::fromTokens (js, ",", "")) { auto* r = p.apvts.getParameter (kv.upToFirstOccurrenceOf ("=", false, false)); r->setValueNotifyingHost (r->convertTo0to1 (kv.fromFirstOccurrenceOf ("=", false, false).getFloatValue())); }
            const int lat = p.getLatencySamples(); std::vector<float> out; juce::AudioBuffer<float> b (2, B); juce::MidiBuffer m; float maxGr = 0;
            for (int rep = 0; rep < 2; ++rep) for (int o = 0; o < len; o += B) { const int n = std::min (B, len - o); b.setSize (2, n, false, false, true); for (int i = 0; i < n; ++i) { b.setSample (0, i, in[(size_t) (o + i)]); b.setSample (1, i, in[(size_t) (o + i)]); } p.processBlock (b, m); if (rep == 1) for (int i = 0; i < n; ++i) out.push_back (b.getSample (0, i)); maxGr = std::max (maxGr, p.eng.gr.load()); }
            // allinea la latenza: confronta l'uscita del secondo giro con l'ingresso
            std::vector<float> al ((size_t) len, 0.0f); for (int i = 0; i < len; ++i) { const int k = i + lat; al[(size_t) i] = k < len ? out[(size_t) k] : 0.0f; }
            const auto co = crest (al, (size_t) beat); double bo[4]; bands (al, (size_t) beat, bo);
            // quanto e' diverso: residuo dopo il miglior guadagno
            double xy = 0, xx = 0, yy = 0; for (int i = beat; i < len - lat; ++i) { xy += in[(size_t) i] * al[(size_t) i]; xx += in[(size_t) i] * in[(size_t) i]; yy += al[(size_t) i] * al[(size_t) i]; }
            const double g = xy / (xx + 1e-20), res = yy - 2 * g * xy + g * g * xx;
            std::printf ("%2d %-24s livello %+5.1f  crest %+5.1f  GR %4.1f  bande %+5.1f %+5.1f %+5.1f %+5.1f  cambia %+6.1f dB\n", pr, p.getProgramName (pr).toRawUTF8(), co.second - ci.second, (co.first - co.second) - (ci.first - ci.second), maxGr,
                         bo[0] - bi[0] - (co.second - ci.second), bo[1] - bi[1] - (co.second - ci.second), bo[2] - bi[2] - (co.second - ci.second), bo[3] - bi[3] - (co.second - ci.second), 10 * std::log10 (res / (yy + 1e-20) + 1e-20));
        }
        return 0;
    }

    if (auto* t = std::getenv ("JTHEME")) skin::cur() = std::atoi (t);
    juce::ScopedJuceInitialiser_GUI gui;
    const double sr = 48000;
    auto fresh = [&] { auto p = std::make_unique<Processor>(); p->setPlayConfigDetails (2, 2, sr, 512); p->prepareToPlay (sr, 512); return p; };
    // batteria finta: cassa ogni 0.5 s, rullante in levare, hat in ottavi
    juce::Random rnd (5);
    if (argc > 1 && juce::String (argv[1]) == "lat")
    {   // latenza vera (correlazione col rumore d'ingresso) contro quella dichiarata, modulo per modulo
        const char* cases[] = { "", "s_on=1", "d_on=1,d_drive=0.5", "f_on=1", "c_on=1,c_amt=0.5", "t_on=1", "t_on=1,t_wow=1,t_flut=1,amount=1.5", "r_on=1,r_mix=0.3", "n_on=1", "s_on=1,d_on=1,f_on=1,c_on=1,t_on=1,r_on=1,n_on=1" };
        for (auto* cs : cases)
        {
            auto p = fresh();
            for (auto kv : juce::StringArray::fromTokens (cs, ",", "")) setP (*p, kv.upToFirstOccurrenceOf ("=", false, false).toRawUTF8(), kv.fromFirstOccurrenceOf ("=", false, false).getFloatValue());
            juce::Random rn (9); std::vector<float> in, L, R;
            run (*p, [&] (int, float& l, float& r) { const float x = (rn.nextFloat() - 0.5f) * 0.3f; in.push_back (x); l = r = x; }, (int) sr * 12, L, R);
            int best = 0; double bc = -1;
            for (int d = 0; d < 600; ++d) { double c = 0; for (int i = (int) sr * 2; i < (int) sr * 12; ++i) c += (double) L[(size_t) i] * in[(size_t) (i - d)]; if (c > bc) { bc = c; best = d; } }
            double e = 0, ee = 0; const int Lt = p->getLatencySamples(); for (int i = (int) sr * 2; i < (int) sr * 12; ++i) { e += std::pow (L[(size_t) i] - in[(size_t) (i - Lt)], 2.0); ee += in[(size_t) i] * in[(size_t) i]; }
            std::printf ("%-60s dichiarata %4d  vera %4d %s  differenza dal pulito %.1f dB\n", cs[0] ? cs : "(tutto spento)", Lt, best, std::abs (best - Lt) > 2 ? "<<< SBAGLIATA" : "", 10 * std::log10 (e / ee + 1e-30));
        }
        return 0;
    }
    auto drums = [&] (int i, float& l, float& r)
    {
        const double t = i / sr, b = std::fmod (t, 0.5), h = std::fmod (t, 0.125), s2 = std::fmod (t + 0.25, 0.5);
        float v = (float) (0.8 * std::sin (2 * 3.14159 * (50 + 120 * std::exp (-b * 30)) * b) * std::exp (-b * 8));
        if (std::fmod (t, 1.0) >= 0.5 && s2 < 0.2) v += (float) ((rnd.nextFloat() - 0.5) * 0.7 * std::exp (-s2 * 25) + 0.3 * std::sin (2 * 3.14159 * 190 * s2) * std::exp (-s2 * 30));
        v += (float) ((rnd.nextFloat() - 0.5) * 0.25 * std::exp (-h * 90));
        l = v; r = v * 0.95f;
    };
    if (argc > 2 && juce::String (argv[1]) == "snap")
    {
        auto p = fresh(); if (auto* pr = std::getenv ("JPRESET")) p->loadPreset (std::atoi (pr));
        std::vector<float> L, R; run (*p, drums, (int) sr * 2, L, R);
        Editor ed (*p); if (auto* jsc = std::getenv ("JSCALE")) ed.scaler.set ((float) std::atof (jsc));
        ed.tickForTest();
        auto img = ed.createComponentSnapshot (ed.getLocalBounds(), true, 1.0f);
        juce::File f (argv[2]); f.deleteFile(); juce::FileOutputStream os (f); juce::PNGImageFormat().writeImageToStream (img, os);
        return 0;
    }
    {   // appena caricato: neutro (a parte la latenza)
        auto p = fresh(); const int lat = p->getLatencySamples(); std::vector<float> L, R, in;
        run (*p, [&] (int i, float& l, float& r) { drums (i, l, r); in.push_back (l); }, 48000, L, R);
        double e = 0; for (size_t i = (size_t) lat; i < L.size(); ++i) e += std::pow (L[i] - in[i - (size_t) lat], 2.0);
        std::printf ("neutro: latenza %d, differenza %.1f dB\n", lat, 10 * std::log10 (e / L.size() + 1e-30));
    }
    {   // SP-12 senza anti-alias: un 15 kHz a 26 kHz ritorna a 11 kHz; S950 lo filtra
        for (int m : { S_SP12, S_S950 })
        {
            auto p = fresh(); setP (*p, "s_on", 1); setP (*p, "s_model", (float) m); setP (*p, "s_bits", 12); setP (*p, "s_rate", m == S_SP12 ? 26040.0f : 32000.0f);
            std::vector<float> L, R; run (*p, [&] (int i, float& l, float& r) { l = r = 0.5f * (float) std::sin (2 * 3.14159 * 15000 * i / sr); }, 48000, L, R);
            const float rate = m == S_SP12 ? 26040.0f : 32000.0f, alias = rate - 15000;
            std::printf ("%s: 15 kHz -> immagine a %.0f Hz: %.1f dB rispetto al tono\n", sInfo (m).name, alias, bandDb (L, 20000, sr, alias - 100, alias + 100) - bandDb (L, 20000, sr, 14900, 15100));
        }
    }
    {   // saturazione: volume compensato
        for (int m = 0; m < kDModels; ++m)
        {
            auto p = fresh(); setP (*p, "d_on", 1); setP (*p, "d_model", (float) m); setP (*p, "d_drive", 18);
            std::vector<float> L, R, in; run (*p, [&] (int i, float& l, float& r) { drums (i, l, r); in.push_back (l); }, 96000, L, R);
            std::printf ("saturazione %s +18 dB: volume %+.1f dB\n", dName (m), juce::Decibels::gainToDecibels (rms (L, 48000, 96000) / rms (in, 48000, 96000)));
        }
    }
    {   // compressore
        for (int st = 0; st < kCStyles; ++st)
        {
            auto p = fresh(); setP (*p, "c_on", 1); setP (*p, "c_style", (float) st); setP (*p, "c_amt", 0.6f);
            std::vector<float> L, R, in; run (*p, [&] (int i, float& l, float& r) { drums (i, l, r); in.push_back (l); }, 96000, L, R);
            float pi = 0, po = 0; for (int i = 48000; i < 96000; ++i) { pi = std::max (pi, std::abs (in[(size_t) i])); po = std::max (po, std::abs (L[(size_t) i])); }
            std::printf ("comp %s: picchi %+.1f dB, rms %+.1f dB (crest ridotto di %.1f dB)\n", cName (st), juce::Decibels::gainToDecibels (po / pi), juce::Decibels::gainToDecibels (rms (L, 48000, 96000) / rms (in, 48000, 96000)),
                         juce::Decibels::gainToDecibels ((pi / rms (in, 48000, 96000)) / (po / rms (L, 48000, 96000))));
        }
        auto p = fresh(); setP (*p, "c_on", 1); setP (*p, "c_amt", 0); setP (*p, "c_punch", 1);
        std::vector<float> L, R, in; run (*p, [&] (int i, float& l, float& r) { drums (i, l, r); in.push_back (l); }, 96000, L, R);
        float pi = 0, po = 0; for (int i = 48000; i < 96000; ++i) { pi = std::max (pi, std::abs (in[(size_t) i])); po = std::max (po, std::abs (L[(size_t) i])); }
        std::printf ("punch +100: picchi %+.1f dB, rms %+.1f dB\n", juce::Decibels::gainToDecibels (po / pi), juce::Decibels::gainToDecibels (rms (L, 48000, 96000) / rms (in, 48000, 96000)));
    }
    {   // polvere: niente rumore nel silenzio se segue la batteria
        auto p = fresh(); setP (*p, "n_on", 1); setP (*p, "n_vinyl", 0.6f); setP (*p, "n_hiss", 0.5f);
        std::vector<float> L, R; run (*p, [&] (int i, float& l, float& r) { if (i < 48000) drums (i, l, r); else l = r = 0; }, 48000 * 3, L, R);
        std::printf ("polvere: con batteria %.1f dBFS, nel silenzio dopo 2 s %.1f dBFS\n", juce::Decibels::gainToDecibels (rms (L, 20000, 46000)), juce::Decibels::gainToDecibels (rms (L, 130000, 144000) + 1e-9f));
    }
    {   // mix al 50% senza filtro a pettine: con tutti i moduli spenti il segnale torna uguale
        auto p = fresh(); setP (*p, "mix", 0.5f); setP (*p, "d_on", 1); setP (*p, "d_drive", 0.0f); const int lat = p->getLatencySamples();
        std::vector<float> L, R, in; run (*p, [&] (int i, float& l, float& r) { drums (i, l, r); in.push_back (l); }, 48000, L, R);
        double e = 0; for (size_t i = (size_t) lat + 1000; i < L.size(); ++i) e += std::pow (L[i] - in[i - (size_t) lat], 2.0);
        std::printf ("mix 50%% allineato: differenza %.1f dB\n", 10 * std::log10 (e / L.size() + 1e-30));
    }
    if (std::getenv ("JPROF"))
    {
        auto bench = [&] (const char* nm, std::function<void (Processor&)> f)
        {
            auto p = fresh(); p->loadPreset (1); f (*p);
            juce::AudioBuffer<float> b (2, 512); juce::MidiBuffer mb; for (int c = 0; c < 2; ++c) for (int i = 0; i < 512; ++i) b.setSample (c, i, 0.3f * std::sin (i * 0.05f));
            const auto t0 = juce::Time::getMillisecondCounterHiRes(); const int nb = 375;
            for (int k = 0; k < nb; ++k) p->processBlock (b, mb);
            std::printf ("%-28s %.2f%%\n", nm, (juce::Time::getMillisecondCounterHiRes() - t0) / 40.0);
        };
        bench ("ricetta intera", [] (Processor&) {});
        for (auto id : { "s_on", "d_on", "f_on", "c_on", "t_on", "r_on", "n_on" })
            bench ((juce::String ("senza ") + id).toRawUTF8(), [id] (Processor& p) { setP (p, id, 0.0f); });
        {   // aliasing: tono a 9999 Hz nel CLIP spinto
            auto p = fresh(); setP (*p, "d_on", 1); setP (*p, "d_model", 4); setP (*p, "d_drive", 18); if (std::getenv ("JCOMP")) { setP (*p, "c_on", 1); setP (*p, "c_style", 0); setP (*p, "c_amt", 0.8f); }
            std::vector<float> L, R; run (*p, [] (int i, float& l, float& r) { l = r = 0.5f * (float) std::sin (2 * 3.14159265 * 9999.27 * i / 48000.0); }, 48000 + 32768, L, R);
            const int NF = 32768; std::vector<float> fb ((size_t) NF * 2);
            for (int i = 0; i < NF; ++i) fb[(size_t) i] = L[(size_t) (48000 + i)] * (0.5f - 0.5f * std::cos (2 * 3.14159f * i / NF));
            juce::dsp::FFT fft (15); fft.performFrequencyOnlyForwardTransform (fb.data());
            const double bin = 48000.0 / NF; float ref = 0, worst = 0; double wf = 0;
            for (int k = 1; k < NF / 2; ++k) { const double f = k * bin; bool harm = false; for (int h = 1; h < 6; ++h) if (std::abs (f - h * 9999.27) < 25 * bin) harm = true; if (std::abs (f - 9999.27) < 3 * bin) ref = std::max (ref, fb[(size_t) k]); if (! harm && f > 50 && fb[(size_t) k] > worst) { worst = fb[(size_t) k]; wf = f; } }
            std::printf ("aliasing CLIP +18 dB su 10 kHz: %.1f dB a %.0f Hz\n", 20 * std::log10 (worst / ref), wf);
        }
        bench ("tutto spento", [] (Processor& p) { for (auto id : { "s_on", "d_on", "f_on", "c_on", "t_on", "r_on", "n_on" }) setP (p, id, 0.0f); });
        return 0;
    }
    for (int i = 0; i < (int) presets().size(); ++i)
    {
        auto p = fresh(); p->loadPreset (i); std::vector<float> L, R, in;
        const auto t0 = juce::Time::getMillisecondCounterHiRes();
        run (*p, [&] (int k, float& l, float& r) { drums (k, l, r); in.push_back (l); }, (int) sr * 4, L, R);
        const double ms = juce::Time::getMillisecondCounterHiRes() - t0;
        bool nan = false; float pk = 0; for (size_t k = 0; k < L.size(); ++k) { if (! std::isfinite (L[k]) || ! std::isfinite (R[k])) nan = true; pk = std::max (pk, std::abs (L[k])); }
        std::printf ("%-34s picco %.2f  volume %+.1f dB  nan %d  CPU %.2f%%\n", presets()[(size_t) i].name, pk, juce::Decibels::gainToDecibels (rms (L, 48000, (int) L.size()) / rms (in, 48000, (int) in.size())), (int) nan, ms / 40.0);
    }
    return 0;
}
