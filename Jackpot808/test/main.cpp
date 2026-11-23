#include "../Source/S8Processor.h"
#include "../Source/S8Editor.h"
#include "TestLab.h"
using namespace s8;

static void snap (Processor& p, const char* path)
{
    if (auto* t = std::getenv ("JTHEME")) skin::cur() = std::atoi (t);
    Editor ed (p); ed.diagnose();
    if (std::getenv ("JBROWSE")) ed.browser.open();
    for (int i = 0; i < 12; ++i) ed.spec.compute();
    if (! kSynth) { ed.tuner.hzS = p.ch.uiHz.load() > 20 ? p.ch.uiHz.load() : 49.5f; ed.tuner.centS = 20; }
    auto img = ed.createComponentSnapshot (ed.getLocalBounds(), true, 1.0f);
    juce::File f (path); f.deleteFile(); juce::FileOutputStream os (f); juce::PNGImageFormat().writeImageToStream (img, os);
}
// render del synth con una lista di note (inizio s, durata s, nota, velocity)
struct Nt { double t, d; int n; float v; };
static std::vector<float> play (Processor& p, const std::vector<Nt>& notes, double len, int blk = 512, bool uneven = false, std::vector<float>* right = nullptr)
{
    const double sr = 48000; const int N = (int) (len * sr); std::vector<float> o ((size_t) N), r2 ((size_t) N);
    juce::AudioBuffer<float> b (2, 4096); juce::Random rb (3); int pos = 0;
    while (pos < N)
    {
        const int n = juce::jmin (N - pos, uneven ? 1 + rb.nextInt (blk) : blk);
        juce::MidiBuffer m;
        for (auto& x : notes)
        {
            const int on = (int) (x.t * sr), off = (int) ((x.t + x.d) * sr);
            if (on >= pos && on < pos + n) m.addEvent (juce::MidiMessage::noteOn (1, x.n, x.v), on - pos);
            if (off >= pos && off < pos + n) m.addEvent (juce::MidiMessage::noteOff (1, x.n), off - pos);
        }
        juce::AudioBuffer<float> v (b.getArrayOfWritePointers(), 2, n); v.clear();
        p.processBlock (v, m);
        for (int i = 0; i < n; ++i) { o[(size_t) (pos + i)] = v.getSample (0, i); r2[(size_t) (pos + i)] = v.getSample (1, i); }
        pos += n;
    }
    if (right) *right = r2;
    return o;
}
static float pitchOf (const std::vector<float>& x, int a, int b)
{   // frequenza dai passaggi per lo zero (sinusoide bassa)
    int n = 0; double first = -1, last = -1;
    for (int i = a + 1; i < b; ++i) if (x[(size_t) i - 1] < 0 && x[(size_t) i] >= 0) { const double fr = x[(size_t) i - 1] / (x[(size_t) i - 1] - x[(size_t) i]); const double t = i - 1 + fr; if (first < 0) first = t; last = t; ++n; }
    return n > 2 ? (float) (48000.0 * (n - 1) / (last - first)) : 0.0f;
}
static float rmsDb (const std::vector<float>& x, int a, int b) { double e = 0; for (int i = a; i < b; ++i) e += (double) x[(size_t) i] * x[(size_t) i]; return (float) (10 * std::log10 (e / juce::jmax (1, b - a) + 1e-20)); }
static float clickDb (const std::vector<float>& x, int a, int b)
{   // energia sopra i 5 kHz (differenza seconda) rispetto al picco
    float pk = 1e-9f, hf = 0; for (int i = a + 2; i < b; ++i) { pk = std::max (pk, std::abs (x[(size_t) i])); hf = std::max (hf, std::abs (x[(size_t) i] - 2 * x[(size_t) i - 1] + x[(size_t) i - 2])); }
    return 20 * std::log10 (hf / pk + 1e-12f);
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gi;
    const double sr = 48000; const int B = 512;
    const juce::String mode = argc > 1 ? juce::String (argv[1]) : juce::String();
    if (mode == "lab" && ! kSynth)
    {
        const int pr = std::getenv ("JPRESET") ? std::atoi (std::getenv ("JPRESET")) : 0;
        testlab::run ([&] (double rate, int blk) -> std::unique_ptr<juce::AudioProcessor> { auto p = std::make_unique<Processor>(); p->setPlayConfigDetails (2, 2, rate, blk); p->prepareToPlay (rate, blk); if (pr > 0) p->loadPreset (pr); return p; }, std::getenv ("JNOCLICK") == nullptr);
        return 0;
    }
    if (kSynth)
    {
        auto mk = [&] (int pr) { auto p = std::make_unique<Processor>(); p->setPlayConfigDetails (0, 2, sr, B); p->prepareToPlay (sr, B); if (pr > 0) p->loadPreset (pr); return p; };
        if (mode == "snap") { auto p = mk (std::getenv ("JPRESET") ? std::atoi (std::getenv ("JPRESET")) : 2); play (*p, { { 0.1, 1.0, 36, 0.9f } }, 0.6); snap (*p, argc > 2 ? argv[2] : "/tmp/s8s.png"); return 0; }
        if (mode == "clicks")
        {   // ogni comando salta del 30% durante una nota tenuta (sinusoide pura: qualunque graffio si vede); energia sopra 5 kHz re picco
            const int pr = std::getenv ("JPRESET") ? std::atoi (std::getenv ("JPRESET")) : 0;
            auto hpPeak = [] (const std::vector<float>& x, int a, int b)
            {
                const double w0 = 2 * 3.14159265 * 5000 / 48000.0, c0 = std::cos (w0); static const double Qs[4] = { 0.5098, 0.6013, 0.9000, 2.5629 }; double st[4][4] {}; float pk = 0;
                for (int i = a - 2000; i < b; ++i) { double v = x[(size_t) i]; for (int q = 0; q < 4; ++q) { const double al = std::sin (w0) / (2 * Qs[q]), b0 = (1 + c0) / 2 / (1 + al), b1 = -(1 + c0) / (1 + al), a1 = -2 * c0 / (1 + al), a2 = (1 - al) / (1 + al);
                    const double y = b0 * v + b1 * st[q][0] + b0 * st[q][1] - a1 * st[q][2] - a2 * st[q][3]; st[q][1] = st[q][0]; st[q][0] = v; st[q][3] = st[q][2]; st[q][2] = y; v = y; } if (i >= a) pk = std::max (pk, (float) std::abs (v)); }
                return pk;
            };
            int bad = 0;
            auto refP = mk (pr);
            for (auto* prm : refP->getParameters())
            {
                auto* rp = dynamic_cast<juce::RangedAudioParameter*> (prm); if (rp == nullptr || dynamic_cast<juce::AudioParameterFloat*> (rp) == nullptr) continue;
                const auto id = rp->getParameterID();
                auto p = mk (pr); p->setV ("s_sus", 0.8f); p->setV ("s_dec", 8.0f);
                auto* q = p->apvts.getParameter (id); const float v0 = q->getValue(), v1 = v0 + 0.3f <= 1 ? v0 + 0.3f : v0 - 0.3f;
                const int J = (int) (1.0 * sr);
                std::vector<float> o; { const std::vector<Nt> n = { { 0.1, 2.5, 33, 0.9f } }; juce::AudioBuffer<float> b (2, 512); int pos = 0; const int N = (int) (2.0 * sr);
                    while (pos < N) { juce::MidiBuffer m; for (auto& x : n) { const int on = (int) (x.t * sr); if (on >= pos && on < pos + 512) m.addEvent (juce::MidiMessage::noteOn (1, x.n, x.v), on - pos); }
                        if (J >= pos && J < pos + 512) q->setValueNotifyingHost (v1);
                        b.clear(); p->processBlock (b, m); for (int i = 0; i < 512; ++i) o.push_back (b.getSample (0, i)); pos += 512; } }
                float pk = 1e-9f; for (int i = (int) (0.5 * sr); i < J; ++i) pk = std::max (pk, std::abs (o[(size_t) i]));
                const float before = hpPeak (o, J - 4800, J - 2400), after = hpPeak (o, J, J + 2400);
                const float db = juce::Decibels::gainToDecibels (after / pk, -200.0f), base = juce::Decibels::gainToDecibels (before / pk, -200.0f);
                const bool c = db > -35 && db > base + 6;
                if (c || std::getenv ("JV")) std::printf ("  %-18s %6.1f dB (prima %6.1f)%s\n", rp->getName (30).toRawUTF8(), db, base, c ? " CLICK" : "");
                bad += c;
            }
            std::printf ("click: %d\n", bad); return 0;
        }
        // 1) accordatura: A1 = 55 Hz dopo che il colpo di pitch e' finito
        {
            auto p = mk (0); const int L = p->getLatencySamples();
            auto o = play (*p, { { 0.1, 1.5, 33, 1.0f } }, 2.0);
            const float f = pitchOf (o, (int) (0.6 * sr) + L, (int) (1.2 * sr) + L);
            std::printf ("Init A1: %.2f Hz (atteso 55.00), picco %.2f, latenza %d\n", f, *std::max_element (o.begin(), o.end()), L);
        }
        // 2) ogni preset: livello, picco, NaN, attacco senza click, coda che si spegne
        for (int pr = 0; pr < (int) presets().size(); ++pr)
        {
            auto p = mk (pr); const int L = p->getLatencySamples();
            auto o = play (*p, { { 0.2, 0.8, 36, 0.9f }, { 1.2, 0.5, 43, 0.9f }, { 1.6, 0.6, 31, 0.9f } }, 12.0);
            float pk = 0; bool nan = false; for (auto x : o) { if (! std::isfinite (x)) nan = true; pk = std::max (pk, std::abs (x)); }
            const float before = rmsDb (o, 0, (int) (0.19 * sr) + L), tail = rmsDb (o, (int) (11.0 * sr), (int) (12.0 * sr));
            std::printf ("%-28s picco %.2f  rms nota %.1f dB  coda finale %.1f dB  silenzio prima %.1f%s\n", presets()[(size_t) pr].name, pk, rmsDb (o, (int) (0.25 * sr) + L, (int) (0.9 * sr) + L), tail, before, nan ? "  NaN!" : "");
        }
        // 3) glide legato: C1 tenuta, poi G1 sovrapposta -> scivola, nessun riattacco
        {
            auto p = mk (0); p->setV ("s_glide", 120); p->setV ("s_gmode", 1); p->setV ("s_dec", 6);
            const int L = p->getLatencySamples();
            auto o = play (*p, { { 0.1, 1.2, 24, 1.0f }, { 0.8, 0.8, 31, 1.0f } }, 2.0);
            std::printf ("glide legato: prima %.1f Hz (32.7), dopo 0.3 s %.1f Hz (49.0); salto di livello all'arrivo %.1f dB\n", pitchOf (o, (int) (0.4 * sr) + L, (int) (0.78 * sr) + L), pitchOf (o, (int) (1.1 * sr) + L, (int) (1.5 * sr) + L),
                         rmsDb (o, (int) (0.81 * sr) + L, (int) (0.85 * sr) + L) - rmsDb (o, (int) (0.75 * sr) + L, (int) (0.79 * sr) + L));
        }
        // 4) click: note ribattute veloci con coda lunga (il riattacco non deve saltare)
        {
            auto p = mk (0); p->setV ("s_dec", 4); const int L = p->getLatencySamples();
            std::vector<Nt> n; for (int k = 0; k < 8; ++k) n.push_back ({ 0.1 + k * 0.137, 0.1, 36 + (k % 3) * 2, 0.9f });
            auto o = play (*p, n, 2.0); float worst = -200;
            for (int k = 1; k < 8; ++k) { const int at = (int) ((0.1 + k * 0.137) * sr) + L; worst = std::max (worst, clickDb (o, at - 64, at + 256)); }
            std::printf ("riattacco: bordo piu' acuto %.1f dB (sinusoide continua < -60)\n", worst);
        }
        // 5) blocchi irregolari = blocchi da 512
        {
            auto a = mk (4), b = mk (4);
            const std::vector<Nt> n = { { 0.1, 0.5, 36, 0.9f }, { 0.5, 0.6, 41, 0.7f }, { 1.0, 0.4, 29, 1.0f } };
            auto o1 = play (*a, n, 2.5, 512, false), o2 = play (*b, n, 2.5, 512, true);
            double e = 0, d = 0; for (size_t i = 0; i < o1.size(); ++i) { e += (double) o1[i] * o1[i]; d += std::pow ((double) o1[i] - o2[i], 2.0); }
            std::printf ("blocchi irregolari vs 512: %.1f dB (gli eventi MIDI cadono a campione, resta il passo del sovracampionamento)\n", 10 * std::log10 (d / (e + 1e-30) + 1e-30));
        }
        // 6) sample: una sinusoide a 55 Hz scritta in WAV, caricata, nota trovata, suonata in C2
        {
            juce::MemoryBlock mb; { juce::WavAudioFormat wf; std::unique_ptr<juce::AudioFormatWriter> w (wf.createWriterFor (new juce::MemoryOutputStream (mb, false), 44100, 1, 16, {}, 0));
                juce::AudioBuffer<float> sb (1, 44100 * 2); for (int i = 0; i < sb.getNumSamples(); ++i) sb.setSample (0, i, 0.6f * std::exp (-i / 60000.0f) * (float) std::sin (2 * juce::MathConstants<double>::pi * 55.0 * i / 44100.0));
                w->writeFromAudioSampleBuffer (sb, 0, sb.getNumSamples()); }
            auto p = mk (0); juce::String err;
            const bool ok = p->loadSampleData (mb.getData(), mb.getSize(), "sine55", err);
            const int L = p->getLatencySamples();
            auto o = play (*p, { { 0.1, 1.0, 36, 1.0f } }, 1.4);
            std::printf ("sample: caricato %d, nota trovata %.2f (A1 = 33), suonato in C2: %.2f Hz (65.41), mix %.2f %s\n", (int) ok, p->sampleRef ? p->sampleRef->root : -1.0f, pitchOf (o, (int) (0.4 * sr) + L, (int) (0.9 * sr) + L), p->v ("s_smix"), err.toRawUTF8());
            juce::MemoryBlock st; p->getStateInformation (st); auto q = mk (0); q->setStateInformation (st.getData(), (int) st.getSize());
            std::printf ("sample salvato nella sessione: %s (%d byte di stato)\n", q->sampleRef != nullptr ? "ritorna" : "PERSO", (int) st.getSize());
        }
        // 7) CPU
        {
            auto p = mk (7); const auto t0 = juce::Time::getMillisecondCounterHiRes();
            play (*p, { { 0.0, 9.0, 36, 1.0f } }, 10.0);
            std::printf ("CPU (preset Rage, una nota): %.1f%% di un core\n", (juce::Time::getMillisecondCounterHiRes() - t0) / 10000.0 * 100.0);
        }
        return 0;
    }
    Processor p;
    auto lay = p.getBusesLayout(); lay.inputBuses.getReference (1) = juce::AudioChannelSet::stereo(); p.setBusesLayout (lay);
    p.prepareToPlay (sr, B);
    if (mode == "preset") p.loadPreset (std::getenv ("JPRESET") ? std::atoi (std::getenv ("JPRESET")) : 3);
    juce::AudioBuffer<float> buf (4, B); juce::MidiBuffer m;
    const float notes[4] = { 49.0f * 1.0116f, 49.0f * 1.0116f, 58.27f * 1.0116f, 43.65f * 1.0116f };
    double ph = 0; float pk = 0; bool nan = false;
    for (int k = 0; k < (int) (10 * sr / B); ++k)
    {
        for (int i = 0; i < B; ++i)
        {
            const double t = (k * B + i) / sr; const double beat = std::fmod (t, 0.5); const int note = (int) (t / 1.0) % 4;
            ph += 2 * 3.14159265 * notes[note] / sr;
            const float env = (float) std::exp (-beat * 2.5), x = 0.5f * env * (float) std::sin (ph) + (beat < 0.004 ? 0.3f * (float) std::sin (ph * 6) : 0.0f);
            buf.setSample (0, i, x); buf.setSample (1, i, x * 0.98f + 0.02f * (float) std::sin (ph * 1.01));
            const float kick = beat < 0.08 ? -0.8f * (float) std::exp (-beat * 40) * (float) std::sin (2 * 3.14159 * 55 * beat) : 0.0f;
            buf.setSample (2, i, kick); buf.setSample (3, i, kick);
        }
        p.processBlock (buf, m);
        for (int i = 0; i < B; ++i) { const float x = buf.getSample (0, i); if (! std::isfinite (x)) nan = true; pk = std::max (pk, std::abs (x)); }
    }
    auto& C = p.ch;
    std::printf ("latenza %d, picco %.2f nan %d\n", p.getLatencySamples(), pk, (int) nan);
    std::printf ("nota ora %.1f Hz conf %.2f, media %.1f Hz, scarto medio %.1f cent, misure %d, SUB+ su %.1f Hz\n", C.uiHz.load(), C.uiConf.load(), C.f0Avg.load(), C.centsAvg.load(), C.pitchN.load(), C.uiSubPlus.load());
    std::printf ("cassa: colpi %d corr %.2f sovrapposizione %.1f dB; sub %.1f corpo %.1f alti %.1f; side %.1f dB; crest %.1f\n", C.kickHits.load(), C.kickCorr.load(), C.overlapDb.load(), C.eSub.load(), C.eBody.load(), C.eHigh.load(), C.sideRatio.load(), C.crest.load());
    if (mode == "snap" || mode == "preset") snap (p, argc > 2 ? argv[2] : "/tmp/s8e.png");
    else { Editor ed (p); ed.diagnose(); for (auto& d : ed.diags) std::printf ("  [%d] %s: %s\n", d.sev, d.title.toRawUTF8(), d.text.toRawUTF8()); }
    return 0;
}
