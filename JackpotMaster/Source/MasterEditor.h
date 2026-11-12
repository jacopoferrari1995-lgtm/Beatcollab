#pragma once
#include "MasterProcessor.h"
#include "JackpotScale.h"
#include "MasterNeo.h"

namespace jms
{
namespace C
{
    const juce::Colour bg { 0xff0b0c10 }, text { 0xffdfe3ea }, dim { 0xff7d8494 }, faint { 0xff4f5562 };
    const juce::Colour magenta { 0xffff2e88 }, phos { 0xff5ef2ff }, amber { 0xffffb000 }, green { 0xff7dff5a }, red { 0xffff3b5c }, violet { 0xffa66bff };
    const juce::Colour lcd1 { 0xff05161a }, lcd2 { 0xff020b0d };
}
juce::Font uf (float size, bool bold = true);
juce::Font mf (float size);
void glowText (juce::Graphics&, const juce::String&, juce::Rectangle<float>, juce::Colour, juce::Font, juce::Justification = juce::Justification::centredLeft);
void lcd (juce::Graphics&, juce::Rectangle<float>, float rad);

class Editor;

// analizzatore di spettro (FFT 8192, scala log, inclinazione 4.5 dB/ott)
struct Analyzer
{
    std::unique_ptr<juce::dsp::FFT> fft; std::vector<float> a, b;
    void run (const StereoRing& rg, float* out, int nPts, float f0, float f1, double sr, float smooth = 0.7f);
};

class Knob : public juce::Component
{
public:
    Knob (juce::RangedAudioParameter* p, juce::Colour c) : param (p), col (c) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent&) override { if (drag) param->endChangeGesture(); drag = false; repaint(); }
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseDoubleClick (const juce::MouseEvent&) override { param->setValueNotifyingHost (param->getDefaultValue()); }
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w) override { param->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, param->getValue() + (w.deltaY > 0 ? 0.01f : -0.01f))); }
    void mouseEnter (const juce::MouseEvent&) override { hover = true; setMouseCursor (juce::MouseCursor::UpDownResizeCursor); repaint(); }
    void mouseExit (const juce::MouseEvent&) override { hover = false; repaint(); }
    juce::RangedAudioParameter* param; juce::Colour col; juce::String label;
    std::function<juce::String()> text; std::function<void()> onTouch;
    bool big = false, bipolar = false; float activity = 0;
private:
    float start = 0; bool drag = false, hover = false;
};

class Segs : public juce::Component
{
public:
    Segs (juce::RangedAudioParameter* p, juce::Colour c, juce::StringArray it) : param (p), col (c), items (std::move (it)) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    juce::RangedAudioParameter* param; juce::Colour col; juce::StringArray items; float font = 11.0f; int rows = 1;
};

class Switch : public juce::Component
{
public:
    Switch (juce::RangedAudioParameter* p, juce::Colour c, juce::String l = {}) : param (p), col (c), label (std::move (l)) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { param->setValueNotifyingHost (param->getValue() > 0.5f ? 0.0f : 1.0f); }
    juce::RangedAudioParameter* param; juce::Colour col; juce::String label;
};

// catena dei moduli
class Chain : public juce::Component
{
public:
    Chain (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override { hov = -1; repaint(); }
    juce::Rectangle<float> tile (int m) const { const float w = (getWidth() - 8.0f * (kNumMods - 1)) / kNumMods; return { m * (w + 8), 0, w, (float) getHeight() }; }
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void paintTile (juce::Graphics&, int m, juce::Rectangle<float>, bool ghost);
    int insertAt (float x) const;
    Editor& ed; int hov = -1, dragMod = -1; bool dragging = false; float dragX = 0;
};

// storia: ingresso, uscita e riduzione di guadagno
class History : public juce::Component
{
public:
    History (Editor& e, bool lim) : ed (e), limiter (lim) {}
    void paint (juce::Graphics&) override;
    Editor& ed; bool limiter;
};

class MaxPage : public juce::Component
{
public:
    MaxPage (Editor&);
    void paint (juce::Graphics&) override;
    void resized() override;
    Editor& ed;
    std::unique_ptr<Knob> gain, clip, chr, link; std::unique_ptr<Segs> style; std::unique_ptr<Switch> on, delta, learn;
    History hist;
};

class LimPage : public juce::Component
{
public:
    LimPage (Editor&);
    void paint (juce::Graphics&) override;
    void resized() override;
    Editor& ed;
    std::unique_ptr<Knob> gain, ceil, rel; std::unique_ptr<Switch> on, delta, autoR, tp;
    History hist;
};

// pagina base per i moduli a latenza zero
class ModPage : public juce::Component
{
public:
    ModPage (Editor& e, int id, const char* onP, const char* deltaP);
    void paintHeader (juce::Graphics&, const juce::String& help);
    Knob* knob (const char* pid, juce::Colour c, const juce::String& label, std::function<juce::String()> txt, bool bip = false);
    void touch();
    Editor& ed; int mod; const char* onId; std::unique_ptr<Switch> on, delta; juce::OwnedArray<Knob> knobs;
};

class BassView : public juce::Component
{
public:
    BassView (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    Editor& ed; float spIn[200] {}, spOut[200] {}; float hist[160] {}; int hw = 0; Analyzer anIn, anOut;
};
class BassPage : public ModPage
{
public:
    BassPage (Editor&);
    void paint (juce::Graphics& g) override;
    void resized() override;
    BassView view;
};

class ActBtn : public juce::Component
{
public:
    ActBtn (juce::String t, juce::Colour c, std::function<void()> f) : text (std::move (t)), col (c), fn (std::move (f)) {}
    void paint (juce::Graphics& g) override { jlook::button (g, getLocalBounds().toFloat(), col, down, hover, text, jlook::ui (11.0f, 2)); }
    void mouseDown (const juce::MouseEvent&) override { down = true; repaint(); if (fn) fn(); }
    void mouseUp (const juce::MouseEvent&) override { down = false; repaint(); }
    void mouseEnter (const juce::MouseEvent&) override { hover = true; setMouseCursor (juce::MouseCursor::PointingHandCursor); repaint(); }
    void mouseExit (const juce::MouseEvent&) override { hover = false; repaint(); }
    juce::String text; juce::Colour col; std::function<void()> fn; bool down = false, hover = false;
};

class EqView : public juce::Component
{
public:
    EqView (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    float xOf (float f) const { return (float) (std::log (f / 20.0) / std::log (1000.0) * getWidth()); }
    float fOf (float x) const { return 20.0f * std::pow (1000.0f, juce::jlimit (0.0f, 1.0f, x / (float) getWidth())); }
    float yOf (float db) const { return getHeight() * 0.5f - db / 20.0f * getHeight() * 0.5f; }
    float dbOf (float y) const { return (getHeight() * 0.5f - y) / (getHeight() * 0.5f) * 20.0f; }
    juce::Point<float> node (int b) const;
    Editor& ed; int drag = -1; float sp[240] {}; Analyzer an;
};
class EqPage : public ModPage
{
public:
    EqPage (Editor&);
    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void selectBand (int b);
    juce::Rectangle<float> chip (int b) const { return { 16 + b * 138.0f, 356, 130, 28 }; }
    EqView view; int band = 0; std::unique_ptr<ActBtn> solo;
    juce::Rectangle<float> soloBox (int b) const { return chip (b).removeFromRight (28).reduced (3, 4); }
    void toggleSolo (int b);
    std::unique_ptr<Segs> type[kEqBands], ch[kEqBands]; std::unique_ptr<Switch> bon[kEqBands], dyn[kEqBands];
    Knob* fk[kEqBands] {}; Knob* gk[kEqBands] {}; Knob* qk[kEqBands] {}; Knob* tk[kEqBands] {}; Knob* rk[kEqBands] {};
};

class ImView : public juce::Component
{
public:
    ImView (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    float xOf (float f) const { return (float) (std::log (f / 20.0) / std::log (1000.0) * getWidth()); }
    float fOf (float x) const { return 20.0f * std::pow (1000.0f, juce::jlimit (0.0f, 1.0f, x / (float) getWidth())); }
    float xb (int k) const;
    Editor& ed; int drag = -1;   // 0..2 tagli, 10..13 larghezze
    float sp[200] {}; Analyzer an;
};
class Gonio : public juce::Component
{
public:
    Gonio (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    Editor& ed;
};
class ImagerPage : public ModPage
{
public:
    ImagerPage (Editor&);
    void paint (juce::Graphics& g) override;
    void resized() override;
    ImView view; Gonio gonio; std::unique_ptr<Switch> mono, solo[kImBands]; std::unique_ptr<Segs> smode;
};

// pulsante d'azione

class DsView : public juce::Component
{
public:
    DsView (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent& e) override { mouseDrag (e); }
    void mouseDrag (const juce::MouseEvent&) override;
    Editor& ed; float sp[200] {}; float hist[160] {}; int hw = 0; Analyzer an;
};
class DeEssPage : public ModPage
{
public:
    DeEssPage (Editor&);
    void paint (juce::Graphics& g) override;
    void resized() override;
    DsView view; std::unique_ptr<Segs> ch;
};

class BusView : public juce::Component
{
public:
    BusView (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    Editor& ed; float needle = 0; float hist[200] {}; int hw = 0;
};
class BusPage : public ModPage
{
public:
    BusPage (Editor&);
    void paint (juce::Graphics& g) override;
    void resized() override;
    BusView view; std::unique_ptr<Segs> ratio, att, rel; std::unique_ptr<Switch> autoM;
};

class MbView : public juce::Component
{
public:
    MbView (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override { drag = -1; }
    float xOf (float f) const { return (float) (std::log (f / 20.0) / std::log (1000.0) * getWidth()); }
    float fOf (float x) const { return 20.0f * std::pow (1000.0f, juce::jlimit (0.0f, 1.0f, x / (float) getWidth())); }
    Editor& ed; int drag = -1;
};
class MultiPage : public ModPage
{
public:
    MultiPage (Editor&);
    void paint (juce::Graphics& g) override;
    void resized() override;
    void ott();
    MbView view; std::unique_ptr<Switch> solo[kMbBands]; std::unique_ptr<ActBtn> ottBtn, resetBtn;
};

class TapeView : public juce::Component
{
public:
    TapeView (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    void update();
    Editor& ed; float angle = 0, harm[8] {}; int lm = -1; float ld = -1, lb = -9;
};
class TapePage : public ModPage
{
public:
    TapePage (Editor&);
    void paint (juce::Graphics& g) override;
    void resized() override;
    TapeView view; std::unique_ptr<Segs> model;
};

class ExView : public juce::Component
{
public:
    ExView (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override { drag = -1; }
    float xOf (float f) const { return (float) (std::log (f / 20.0) / std::log (1000.0) * getWidth()); }
    float fOf (float x) const { return 20.0f * std::pow (1000.0f, juce::jlimit (0.0f, 1.0f, x / (float) getWidth())); }
    Editor& ed; int drag = -1; float sp[200] {}; Analyzer an;
};
class ExciterPage : public ModPage
{
public:
    ExciterPage (Editor&);
    void paint (juce::Graphics& g) override;
    void resized() override;
    ExView view; std::unique_ptr<Segs> mode[kExBands]; std::unique_ptr<Switch> solo[kExBands];
};

constexpr int TOOL_BAL = 100, TOOL_REF = 101;

// bilanciamento tonale rispetto al genere
class BalancePage : public juce::Component
{
public:
    BalancePage (Editor&);
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void applyEq (const float* devRegions);
    juce::Rectangle<float> genreCell (int g) const { return { 16 + (g % 4) * 206.0f, 52.0f + (g / 4) * 30.0f, 200, 26 }; }
    Editor& ed; int genreSel = 0; std::unique_ptr<ActBtn> fix, reset;
};
class RefPage : public juce::Component
{
public:
    RefPage (Editor&);
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    juce::Rectangle<float> waveR() const { return { 16, 100, 822, 90 }; }
    Editor& ed; std::unique_ptr<ActBtn> load, listen, match, matchEq; std::unique_ptr<juce::FileChooser> chooser;
};

class SoonPage : public juce::Component
{
public:
    SoonPage (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    Editor& ed; int mod = 0;
};

class LoudPanel : public juce::Component
{
public:
    LoudPanel (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    juce::Rectangle<float> tgtCell (int t) const { const float w = (getWidth() - 32 - 8) / 3.0f; return { 16 + (t % 3) * (w + 4), 268.0f + (t / 3) * 26.0f, w, 22 }; }
    juce::Rectangle<float> resetR() const { return { (float) getWidth() - 86, 10, 70, 22 }; }
    Editor& ed;
};

// modalita FACILE (stile Master Plan 2): parti vive separate, sfondo in cache
class EasyPage;
class EasyStage : public juce::Component
{
public:
    explicit EasyStage (EasyPage& p) : page (p) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    EasyPage& page; Analyzer anIn, anOut; float specIn[300] {}, specOut[300] {}; juce::Rectangle<float> loadR, listenR;
    neo::Bloom bloom;
private:
    void wave (juce::Graphics&, juce::Rectangle<float>); void spectrum (juce::Graphics&, juce::Rectangle<float>);
    void gain (juce::Graphics&, juce::Rectangle<float>); void ref (juce::Graphics&, juce::Rectangle<float>);
};
class EasyInput : public juce::Component { public: explicit EasyInput (EasyPage& p) : page (p) { setRepaintsOnMouseActivity (true); } void paint (juce::Graphics&) override; void mouseDown (const juce::MouseEvent&) override; EasyPage& page; juce::Rectangle<float> autoR; float sm = -100; };
class EasyResult : public juce::Component { public: explicit EasyResult (EasyPage& p) : page (p) {} void paint (juce::Graphics&) override; EasyPage& page; float sm = 0; neo::Bloom bloom; };
class EasyMeters : public juce::Component { public: explicit EasyMeters (EasyPage& p) : page (p) {} void paint (juce::Graphics&) override; EasyPage& page; float pk[2] {}; float big = -100; };

class EasyPage : public juce::Component
{
public:
    explicit EasyPage (Editor&);
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override { hover = -1; repaint(); }
    void paintStatic (juce::Graphics&);
    void tick();
    Editor& ed; int view = 0, hover = -1;
    juce::OwnedArray<neo::Knob> knobs; neo::Knob *low, *mid, *high, *thick, *warm, *silk, *pres, *loud, *speed, *glue, *calm, *wide, *ceil, *trim;
    std::unique_ptr<neo::Toggle> monoB; std::unique_ptr<juce::FileChooser> chooser;
    EasyStage stage { *this }; EasyInput input { *this }; EasyResult result { *this }; EasyMeters meters { *this };
    neo::Segments tabs, styles; juce::Rectangle<float> targetR, cmpR, deltaR, bandR[3];
    juce::Image bg; float bgScale = 0;
};

class Editor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit Editor (Processor&);
    ~Editor() override { setLookAndFeel (nullptr); }
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void tickForTest() { for (int i = 0; i < 5; ++i) timerCallback(); }
    void select (int m);
    static constexpr int W = 1280, H = 800;
    jlook::LnF lnf;
    Processor& proc;
    jlook::Scaler scaler { *this, "master", W, H };
    jlook::SizeChip sizeChip { scaler };
    juce::RangedAudioParameter* P (const juce::String& id) { return proc.apvts.getParameter (id); }
    int sel = MAXIM;
    Chain chain { *this };
    MaxPage maxPage { *this };
    BassPage bassPage { *this };
    EqPage eqPage { *this };
    ImagerPage imPage { *this };
    DeEssPage dsPage { *this };
    BusPage busPage { *this };
    MultiPage mbPage { *this };
    TapePage tpPage { *this };
    ExciterPage exPage { *this };
    BalancePage balPage { *this };
    RefPage refPage { *this };
    ThirdAnalyzer live;   // spettro medio dell'uscita (mobile, ~3 s)
    float liveThirds[kThirds] {};
    LimPage limPage { *this };
    SoonPage soon { *this };
    LoudPanel loud { *this };
    EasyPage easyPage { *this };
    void applyMode();
    juce::Rectangle<int> modeR[2]; bool lastEasy = false;
    juce::Image neoBg; float neoScale = 0; neo::Segments modeSeg;
    void paintNeo (juce::Graphics&);
private:
    void timerCallback() override;
    std::unique_ptr<Segs> dither; std::unique_ptr<Switch> compare, norm; std::unique_ptr<Knob> trim;
    juce::Rectangle<int> presetRect, prevRect, nextRect, abRect, balRect, refRect; bool abHover = false; int tick = 0;
};
}
