#include "RewindProcessor.h"
#include "RewindEditor.h"

namespace jrwd
{
juce::AudioProcessorValueTreeState::ParameterLayout Processor::layout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "snap", 1 }, "Aggancia alle battute", true));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "norm", 1 }, "Normalizza", false));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "rev", 1 }, "Al contrario", false));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "loop", 1 }, "Ascolto in loop", true));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "pvGain", 1 }, "Volume ascolto", NormalisableRange<float> (-24, 6), 0));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "pvSolo", 1 }, "Ascolto da solo", true));
    return l;
}
Processor::Processor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true).withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", layout()) {}
void Processor::prepareToPlay (double s, int)
{
    const bool same = std::abs (s - sr) < 1 && cap > 0;
    sr = s;
    if (same) return;   // non perdere la memoria se l'host richiama prepare
    const double minutes = sr <= 50000 ? 8.0 : 4.0;
    cap = (juce::int64) (sr * 60.0 * minutes);
    ring.assign ((size_t) (cap * 2), 0);
    colN = juce::jmax (64, (int) (sr * kColSec));
    nCols = (int) (cap / colN) + 4;
    cols = std::vector<Col> ((size_t) nCols);
    wr = 0; colAt = 0; cPk = 0; cSq = 0;
}
void Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals nd;
    const int n = buffer.getNumSamples(); if (n == 0 || cap == 0) return;
    if (getTotalNumInputChannels() == 1) buffer.copyFrom (1, 0, buffer, 0, 0, n);
    const float* L = buffer.getReadPointer (0); const float* R = buffer.getReadPointer (1);
    double ppq = -1, bpm = 0; int num = 4; bool playing = false;
    if (auto* ph = getPlayHead()) if (auto pos = ph->getPosition())
    {
        playing = pos->getIsPlaying();
        if (auto b = pos->getBpm()) bpm = *b;
        if (auto p = pos->getPpqPosition()) ppq = *p;
        if (auto ts = pos->getTimeSignature()) num = ts->numerator;
    }
    float pk = 0;
    if (! hold.load())
    {
        juce::int64 w = wr.load();
        for (int i = 0; i < n; ++i)
        {
            const size_t k = (size_t) ((w % cap) * 2);
            ring[k] = (juce::int16) juce::jlimit (-32767.0f, 32767.0f, L[i] * 32767.0f);
            ring[k + 1] = (juce::int16) juce::jlimit (-32767.0f, 32767.0f, R[i] * 32767.0f);
            const float a = juce::jmax (std::abs (L[i]), std::abs (R[i])); cPk = juce::jmax (cPk, a); cSq += 0.5 * (L[i] * L[i] + R[i] * R[i]); pk = juce::jmax (pk, a);
            if (w % colN == 0)
            {   // inizio di una colonna: annota dove eravamo nella canzone
                auto& c = cols[(size_t) ((w / colN) % nCols)];
                c.ppq = (playing && ppq >= 0) ? ppq + i / sr * bpm / 60.0 : -1.0; c.bpm = bpm; c.num = num;
            }
            ++w;
            if (w % colN == 0)
            {
                auto& c = cols[(size_t) (((w - 1) / colN) % nCols)];
                c.pk = cPk; c.rms = (float) std::sqrt (cSq / colN); c.abs = (w - 1) / colN; cPk = 0; cSq = 0;
            }
        }
        wr = w;
    }
    else for (int i = 0; i < n; ++i) pk = juce::jmax (pk, std::abs (L[i]), std::abs (R[i]));
    inPk = juce::jmax (pk, inPk.load() * 0.9f);
    // ascolto della selezione
    if (pvOn.load())
    {
        const float g = juce::Decibels::decibelsToGain (value ("pvGain")); const bool solo = value ("pvSolo") > 0.5f;
        float* oL = buffer.getWritePointer (0); float* oR = buffer.getWritePointer (1);
        juce::int64 p = pvPos.load();
        for (int i = 0; i < n; ++i)
        {
            float l = 0, r = 0;
            if (pvOn.load() && p < pvB && p >= oldest())
            {
                const size_t k = (size_t) ((p % cap) * 2); l = ring[k] / 32767.0f * g; r = ring[k + 1] / 32767.0f * g;
                if (++p >= pvB) { if (pvLoop) p = pvA; else pvOn = false; }
            }
            if (solo) { oL[i] = l; oR[i] = r; } else { oL[i] += l; oR[i] += r; }
        }
        pvPos = p;
    }
}
void Processor::readRange (juce::int64 a, juce::int64 b, juce::AudioBuffer<float>& out) const
{
    a = juce::jmax (a, oldest()); b = juce::jmin (b, wr.load());
    const int n = (int) juce::jmax ((juce::int64) 0, b - a); out.setSize (2, n);
    for (int i = 0; i < n; ++i) { const size_t k = (size_t) (((a + i) % cap) * 2); out.setSample (0, i, ring[k] / 32767.0f); out.setSample (1, i, ring[k + 1] / 32767.0f); }
}
bool Processor::musicalAt (juce::int64 s, double& ppq, double& bpm, int& num) const
{
    if (s < oldest() || s >= wr.load()) return false;
    const auto& c = col (s / colN);
    if (c.ppq.load() < 0 || c.bpm.load() <= 0) return false;
    bpm = c.bpm; num = juce::jmax (1, c.num.load());
    ppq = c.ppq + (double) (s - (s / colN) * colN) / sr * bpm / 60.0;
    return true;
}
juce::int64 Processor::snapToBar (juce::int64 s, int mode) const
{
    double ppq, bpm; int num;
    if (! musicalAt (s, ppq, bpm, num)) return s;
    const double q = ppq / num, bar = (mode < 0 ? std::floor (q + 0.02) : mode > 0 ? std::ceil (q - 0.02) : std::round (q)) * num;
    return s + (juce::int64) std::llround ((bar - ppq) * 60.0 / bpm * sr);
}
juce::File Processor::folder()
{
    auto d = juce::File::getSpecialLocation (juce::File::userMusicDirectory).getChildFile ("Jackpot Rewind"); d.createDirectory(); return d;
}
// tonalita: cromagramma e profili di Krumhansl
juce::String Processor::keyOf (const juce::AudioBuffer<float>& b) const
{
    const int n = b.getNumSamples(); if (n < 8192) return {};
    juce::dsp::FFT fft (13); std::vector<float> buf (16384); double chroma[12] {};
    const int step = juce::jmax (4096, n / 120);
    for (int pos = 0; pos + 8192 <= n; pos += step)
    {
        for (int i = 0; i < 8192; ++i) { const float w = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * i / 8191.0f); buf[(size_t) i] = 0.5f * (b.getSample (0, pos + i) + b.getSample (1, pos + i)) * w; }
        std::fill (buf.begin() + 8192, buf.end(), 0.0f);
        fft.performFrequencyOnlyForwardTransform (buf.data());
        for (int k = 4; k < 4096; ++k)
        {
            const double f = k * sr / 8192.0; if (f < 55 || f > 2000) continue;
            const int pc = ((int) std::lround (12.0 * std::log2 (f / 440.0)) % 12 + 12 + 9) % 12;   // 0 = Do
            chroma[pc] += buf[(size_t) k];
        }
    }
    static const double maj[12] = { 6.35, 2.23, 3.48, 2.33, 4.38, 4.09, 2.52, 5.19, 2.39, 3.66, 2.29, 2.88 }, mnr[12] = { 6.33, 2.68, 3.52, 5.38, 2.60, 3.53, 2.54, 4.75, 3.98, 2.69, 3.34, 3.17 };
    static const char* nm[12] = { "C", "Db", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B" };
    double best = -1e9; juce::String key; double tot = 0; for (double c : chroma) tot += c; if (tot <= 0) return {};
    for (int r = 0; r < 12; ++r) for (int m = 0; m < 2; ++m)
    {
        double sxy = 0, sx = 0, sy = 0, sxx = 0, syy = 0;
        for (int i = 0; i < 12; ++i) { const double x = chroma[(i + r) % 12], y = (m ? mnr : maj)[i]; sxy += x * y; sx += x; sy += y; sxx += x * x; syy += y * y; }
        const double cr = (12 * sxy - sx * sy) / std::sqrt ((12 * sxx - sx * sx) * (12 * syy - sy * sy) + 1e-12);
        if (cr > best) { best = cr; key = juce::String (nm[r]) + (m ? "m" : ""); }
    }
    return best > 0.5 ? key : juce::String();
}
juce::File Processor::exportRange (juce::int64 a, juce::int64 b, bool normalize, bool reverse, juce::String& info)
{
    juce::AudioBuffer<float> buf; readRange (a, b, buf);
    const int n = buf.getNumSamples(); if (n < 64) return {};
    if (reverse) buf.reverse (0, n);
    // dissolvenze cortissime ai bordi: niente click
    const int fd = juce::jmin (n / 4, (int) (sr * 0.004));
    for (int c = 0; c < 2; ++c) { buf.applyGainRamp (c, 0, fd, 0, 1); buf.applyGainRamp (c, n - fd, fd, 1, 0); }
    if (normalize) { const float m = buf.getMagnitude (0, n); if (m > 1e-5f) buf.applyGain (juce::Decibels::decibelsToGain (-0.3f) / m); }
    double ppq, bpm = 0; int num = 4; juce::String bars;
    if (musicalAt (a, ppq, bpm, num)) { const double beats = (double) n / sr * bpm / 60.0, br = beats / num; bars = std::abs (br - std::round (br)) < 0.05 ? juce::String ((int) std::round (br)) + "bar" : juce::String (br, 1) + "bar"; }
    const auto key = keyOf (buf);
    juce::String name = "Rewind " + juce::Time::getCurrentTime().formatted ("%H%M%S");
    if (bpm > 0) name << " " << juce::String (bpm, std::abs (bpm - std::round (bpm)) < 0.01 ? 0 : 1) << "bpm";
    if (key.isNotEmpty()) name << " " << key;
    if (bars.isNotEmpty()) name << " " << bars;
    if (reverse) name << " rev";
    auto f = folder().getNonexistentChildFile (name, ".wav", false);
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (new juce::FileOutputStream (f), sr, 2, 24, {}, 0));
    if (w == nullptr) return {};
    w->writeFromAudioSampleBuffer (buf, 0, n); w.reset();
    info = name;
    return f;
}
void Processor::getStateInformation (juce::MemoryBlock& d) { if (auto x = apvts.copyState().createXml()) copyXmlToBinary (*x, d); }
void Processor::setStateInformation (const void* data, int size) { if (auto x = getXmlFromBinary (data, size)) { auto t = juce::ValueTree::fromXml (*x); if (t.isValid()) apvts.replaceState (t); } }
juce::AudioProcessorEditor* Processor::createEditor() { return new Editor (*this); }
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new jrwd::Processor(); }
