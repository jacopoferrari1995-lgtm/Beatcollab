// Controllo delle automazioni, uguale per tutti i plugin della serie.
// 1) ogni parametro e' visibile e automatizzabile da Ableton
// 2) il plugin segue l'automazione mentre suona (il suono cambia) senza NaN
// 3) una rampa di automazione non fa clic (salti improvvisi nell'uscita)
// 4) salva e riapri il progetto: tutti i valori tornano uguali
// 5) a valori di partenza il plugin e' neutro (regola della serie)
#include <juce_audio_processors/juce_audio_processors.h>
#include <cmath>
#include <cstdio>

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();

static void fillSignal (juce::AudioBuffer<float>& b, int64_t start, double sr)
{
    juce::Random r ((juce::int64) start * 7 + 1);
    for (int i = 0; i < b.getNumSamples(); ++i)
    {
        const double t = (start + i) / sr, beat = std::fmod (t * 2.0, 1.0);
        const float v = (float) (0.45 * std::sin (2 * juce::MathConstants<double>::pi * 60 * t) * std::exp (-beat * 7)
                               + 0.15 * (r.nextFloat() * 2 - 1) * std::exp (-std::fmod (t * 4.0, 1.0) * 25)
                               + 0.08 * std::sin (2 * juce::MathConstants<double>::pi * 440 * t) + 0.04 * std::sin (2 * juce::MathConstants<double>::pi * 3300 * t));
        for (int c = 0; c < b.getNumChannels(); ++c) b.setSample (c, i, c ? v * 0.92f : v);
    }
}

struct Run { std::vector<float> out; bool nan = false; };

static Run render (juce::AudioProcessor& p, int blocks, int bs, double sr, std::function<void (int)> perBlock = nullptr)
{
    Run r; const int ch = juce::jmax (2, juce::jmax (p.getTotalNumInputChannels(), p.getTotalNumOutputChannels()));
    juce::AudioBuffer<float> b (ch, bs); juce::MidiBuffer m;
    const bool synth = p.getTotalNumInputChannels() == 0;
    for (int k = 0; k < blocks; ++k)
    {
        if (perBlock) perBlock (k);
        b.clear();
        if (! synth) fillSignal (b, (int64_t) k * bs, sr);
        m.clear(); if (synth && k % 20 == 0) m.addEvent (juce::MidiMessage::noteOn (1, 36 + (k / 20) % 4, (juce::uint8) 110), 0);
        p.processBlock (b, m);
        for (int i = 0; i < bs; ++i) { const float x = b.getSample (0, i); if (! std::isfinite (x)) r.nan = true; r.out.push_back (std::isfinite (x) ? x : 0.0f); }
    }
    return r;
}
static double diffDb (const std::vector<float>& a, const std::vector<float>& b, size_t from = 0)
{
    double d = 0, e = 0; for (size_t i = from; i < juce::jmin (a.size(), b.size()); ++i) { d += (a[i] - b[i]) * (a[i] - b[i]); e += a[i] * a[i]; }
    return 10 * std::log10 (d / (e + 1e-12) + 1e-20);
}
// clic: il salto piu grande fra campioni vicini, rispetto a quello del segnale normale
static float maxJump (const std::vector<float>& a, size_t from) { float m = 0; for (size_t i = from + 1; i < a.size(); ++i) m = juce::jmax (m, std::abs (a[i] - a[i - 1])); return m; }

int main()
{
    juce::ScopedJuceInitialiser_GUI gi;
    const double sr = 48000; const int bs = 512, blocks = 160;
    std::unique_ptr<juce::AudioProcessor> p (createPluginFilter());
    p->setRateAndBufferSizeDetails (sr, bs); p->prepareToPlay (sr, bs);
    auto& params = p->getParameters();
    std::printf ("=== %s: %d parametri\n", p->getName().toRawUTF8(), params.size());
    int notAuto = 0, noName = 0;
    for (auto* q : params) { if (! q->isAutomatable()) ++notAuto; if (q->getName (64).isEmpty()) ++noName; }
    std::printf ("visibili in Ableton: %d/%d automatizzabili, %d senza nome\n", params.size() - notAuto, params.size(), noName);

    // riferimento a valori di partenza
    std::vector<float> defaults; for (auto* q : params) defaults.push_back (q->getDefaultValue());
    auto resetAll = [&] { for (int i = 0; i < params.size(); ++i) params[i]->setValueNotifyingHost (defaults[(size_t) i]); };
    auto fresh = [&] { p->releaseResources(); p->prepareToPlay (sr, bs); };
    resetAll(); fresh();
    const auto ref = render (*p, blocks, bs, sr);
    // neutro: l'uscita e' uguale all'ingresso?
    if (p->getTotalNumInputChannels() > 0)
    {
        std::vector<float> in; juce::AudioBuffer<float> b (2, bs); for (int k = 0; k < blocks; ++k) { fillSignal (b, (int64_t) k * bs, sr); for (int i = 0; i < bs; ++i) in.push_back (b.getSample (0, i)); }
        const int lat = p->getLatencySamples(); std::vector<float> shifted (in.size(), 0.0f); for (size_t i = (size_t) lat; i < in.size(); ++i) shifted[i] = in[i - (size_t) lat];
        const double nd = diffDb (shifted, ref.out, (size_t) (sr * 0.5));
        // stessa misura solo nella banda udibile (sotto i 18 kHz): i filtri del sovracampionamento toccano solo l'ultimissimo acuto
        auto lp = [sr] (std::vector<float> v)
        {   // passa basso ripido a 16 kHz (sinc finestrato, 511 punti)
            const int M = 511, H2 = M / 2; std::vector<float> h ((size_t) M); const double fc = 16000.0 / sr;
            for (int k = 0; k < M; ++k) { const double t = k - H2; const double sn = t == 0 ? 2 * fc : std::sin (juce::MathConstants<double>::twoPi * fc * t) / (juce::MathConstants<double>::pi * t);
                h[(size_t) k] = (float) (sn * (0.42 - 0.5 * std::cos (juce::MathConstants<double>::twoPi * k / (M - 1)) + 0.08 * std::cos (2 * juce::MathConstants<double>::twoPi * k / (M - 1)))); }
            std::vector<float> o (v.size(), 0.0f);
            for (size_t i = (size_t) M; i < v.size(); ++i) { double a = 0; for (int k = 0; k < M; ++k) a += h[(size_t) k] * v[i - (size_t) k]; o[i] = (float) a; }
            return o;
        };
        const double ndA = diffDb (lp (shifted), lp (ref.out), (size_t) (sr * 0.5));
        if (nd > -60)
        {   // ritardo giusto? cerca lo spostamento che allinea meglio
            double best = 0; int bl = 0;
            for (int d = -64; d <= 64; ++d)
            {
                std::vector<float> sh (in.size(), 0.0f); for (size_t i = 0; i < in.size(); ++i) { const long j = (long) i - lat - d; if (j >= 0 && j < (long) in.size()) sh[i] = in[(size_t) j]; }
                const double v = diffDb (sh, ref.out, (size_t) (sr * 0.5)); if (v < best) { best = v; bl = d; }
            }
            std::printf ("   (latenza dichiarata %d, allineamento migliore a %+d campioni: %.1f dB)\n", lat, bl, best);
        }
        std::printf ("partenza neutra: differenza dall'ingresso %.1f dB, nell'udibile %.1f dB %s\n", nd, ndA, nd < -60 ? "(neutro)" : ndA < -60 ? "(neutro nell'udibile)" : "<<< NON NEUTRO");
    }
    int dead = 0, clicks = 0, nans = 0; juce::StringArray deadNames, clickNames;
    const float refJump = maxJump (ref.out, (size_t) (sr * 0.5));
    for (int i = 0; i < params.size(); ++i)
    {
        auto* q = params[i];
        if (! q->isAutomatable()) continue;
        resetAll(); fresh();
        const float d0 = defaults[(size_t) i], target = d0 < 0.5f ? 0.85f : 0.15f;
        const int steps = juce::jmax (2, q->getNumSteps());
        // automazione a gradini durante la riproduzione (da 0.3 s a 1.3 s una rampa)
        const auto r = render (*p, blocks, bs, sr, [&] (int k)
        {
            const float t = juce::jlimit (0.0f, 1.0f, (k * bs / (float) sr - 0.3f) / 1.0f);
            q->setValueNotifyingHost (d0 + (target - d0) * t);
        });
        if (r.nan) { ++nans; continue; }
        const double dd = diffDb (ref.out, r.out, (size_t) (sr * 0.3));
        if (dd < -80) { ++dead; deadNames.add (q->getName (40)); }
        const float jmp = maxJump (r.out, (size_t) (sr * 0.3));
        if (steps > 50 && jmp > juce::jmax (0.25f, refJump * 2.5f))
        {   // confronto: lo stesso valore finale fermo dall'inizio. Se salta uguale e' il suono dell'effetto, non un clic dell'automazione
            resetAll(); fresh(); q->setValueNotifyingHost (target);
            const auto st = render (*p, blocks, bs, sr);
            const float js = maxJump (st.out, (size_t) (sr * 0.3));
            if (jmp > js * 1.5f) { ++clicks; clickNames.add (q->getName (40) + " (" + juce::String (jmp, 2) + " contro " + juce::String (js, 2) + " fermo)"); }
        }
    }
    std::printf ("seguono l'automazione mentre suona: %d senza effetto sul suono, %d con salti/clic, %d con NaN\n", dead, clicks, nans);
    if (! deadNames.isEmpty()) std::printf ("   senza effetto (spesso normali: misure, ascolto, interfaccia): %s\n", deadNames.joinIntoString (", ").substring (0, 900).toRawUTF8());
    if (! clickNames.isEmpty()) std::printf ("   CLIC: %s\n", clickNames.joinIntoString (", ").toRawUTF8());
    // salva e riapri
    juce::Random rr (5); std::vector<float> set; juce::StringArray setT;
    for (auto* q : params) { const float v = rr.nextFloat(); q->setValueNotifyingHost (v); set.push_back (q->getValue()); setT.add (q->getCurrentValueAsText()); }
    juce::MemoryBlock mb; p->getStateInformation (mb);
    std::unique_ptr<juce::AudioProcessor> p2 (createPluginFilter()); p2->setRateAndBufferSizeDetails (sr, bs); p2->prepareToPlay (sr, bs);
    p2->setStateInformation (mb.getData(), (int) mb.getSize());
    int wrong = 0; juce::StringArray wrongNames;
    for (int i = 0; i < p2->getParameters().size(); ++i) if (std::abs (p2->getParameters()[i]->getValue() - set[(size_t) i]) > 0.002f && p2->getParameters()[i]->getCurrentValueAsText() != setT[i]) { ++wrong; wrongNames.add (p2->getParameters()[i]->getName (30)); }
    std::printf ("salva e riapri: %d/%d valori tornano uguali%s%s\n", params.size() - wrong, params.size(), wrong ? "  <<< diversi: " : "", wrongNames.joinIntoString (", ").substring (0, 400).toRawUTF8());
    return 0;
}
