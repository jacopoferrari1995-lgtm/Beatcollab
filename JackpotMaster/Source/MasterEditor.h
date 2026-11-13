#pragma once
#include "MasterProcessor.h"
#include "JackpotScale.h"
#include "MasterNeo.h"

namespace jms
{
namespace C
{
    // stessa tavolozza della modalita FACILE (neo)
    inline juce::Colour bg { 0xff0a0b16 }, text { 0xfff2f3f9 }, dim { 0xff9096ad }, faint { 0xff545a70 };
    inline juce::Colour magenta { 0xffff3d9a }, phos { 0xff3ee8ff }, amber { 0xffffb547 }, green { 0xff4dffb4 }, red { 0xffff4d6a }, violet { 0xff8b5cff };
    const juce::Colour lcd1 { 0xff05161a }, lcd2 { 0xff020b0d };
    // l'AVANZATO segue il tema scelto (scuro / chiaro): stessi colori della modalita FACILE
    inline void sync() { using namespace neo; bg = K::bg1; text = K::ink; dim = K::sub; faint = K::faint; magenta = K::pink; phos = K::cyan; amber = K::amber; green = K::mint; red = K::red; violet = K::violet; }
}
// veli e luci che funzionano su tutti e due i temi
inline juce::Colour veil (float a) { return neo::K::lo.withAlpha (neo::light() ? a * 0.22f : a); }
inline juce::Colour glass (float a) { return neo::K::hi.withAlpha (neo::light() ? juce::jmin (1.0f, a * 1.3f) : a); }
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
    juce::Image cache; juce::String cacheKey; bool noBars = false;   // tessere ferme in cache, ogni frame solo le barrette
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
    void paint (juce::Graphics& g) override { neo::chip (g, getLocalBounds().toFloat().reduced (2, 1), text, down, col, 9.0f, hover); }
    void mouseDown (const juce::MouseEvent&) override { down = true; repaint(); if (fn) fn(); }
    void mouseUp (const juce::MouseEvent&) override { down = false; repaint(); }
    void mouseEnter (const juce::MouseEvent&) override { hover = true; setMouseCursor (juce::MouseCursor::PointingHandCursor); repaint(); }
    void mouseExit (const juce::MouseEvent&) override { hover = false; repaint(); }
    juce::String text; juce::Colour col; std::function<void()> fn; bool down = false, hover = false;
};

// EQ stile Pro-Q: grafico con i punti, finestrella della banda, spettro prima/dopo
class EqView;
class EqPop : public juce::Component
{
public:
    explicit EqPop (EqView& v) : view (v) {}
    void show (int band);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override { dragK = -1; }
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    EqView& view; int band = -1;
    struct Hot { juce::Rectangle<float> r; int id; };
    std::vector<Hot> hots;
private:
    int dragK = -1; float dragStart = 0; int hit (juce::Point<float>) const;
};
class EqView : public juce::Component
{
public:
    explicit EqView (Editor&);
    void paint (juce::Graphics&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override { hover = -1; repaint(); }
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void tick();
    void bandMenu (int b);
    void updatePop();
    float val (int b, const char* k) const;
    void setV (int b, const char* k, float v);
    float xOf (float f) const; float fOf (float x) const; float yOf (float db) const; float dbOf (float y) const;
    juce::Point<float> nodeOf (int b) const; int hit (juce::Point<float>) const;
    juce::Rectangle<float> plot() const { return getLocalBounds().toFloat().reduced (8).withTrimmedTop (6).withTrimmedBottom (16).withTrimmedLeft (26); }
    Editor& ed; EqPop pop { *this };
    int hover = -1, drag = -1, sel = -1, popBand = -1, popHold = 0;
private:
    juce::dsp::FFT fft { 12 }; std::vector<float> win, buf, specPre, specPost;
};
class EqPage : public ModPage
{
public:
    EqPage (Editor&);
    void paint (juce::Graphics& g) override;
    void resized() override;
    EqView view;
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
    Editor& ed; juce::Image trail; float scale = 1.0f, crS = 1.0f; double lastMs = 0;
};
class ImagerPage : public ModPage
{
public:
    ImagerPage (Editor&);
    void paint (juce::Graphics& g) override;
    void resized() override;
    ImView view; Gonio gonio; std::unique_ptr<Switch> mono, safe, solo[kImBands]; std::unique_ptr<Segs> smode;
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

// EQ 432: curva in alto, 5 colonne di manopole (frequenza, guadagno, larghezza)
class E4View : public juce::Component
{
public:
    E4View (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    Editor& ed;
};
class Eq432Page : public ModPage
{
public:
    Eq432Page (Editor&);
    void paint (juce::Graphics& g) override;
    void resized() override;
    E4View view; std::unique_ptr<Segs> range; std::unique_ptr<Switch> sh1, sh5;
};

// PULTEC: curva passiva a valvole, sezione bassi e sezione alti come sull'hardware
class PqView : public juce::Component
{
public:
    PqView (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    Editor& ed;
};
class PultecPage : public ModPage
{
public:
    PultecPage (Editor&);
    void paint (juce::Graphics& g) override;
    void resized() override;
    PqView view; std::unique_ptr<Segs> lf, hf, as;
};

// VARI-MU: ago a valvole + storia della riduzione
class VmView : public juce::Component
{
public:
    VmView (Editor& e) : ed (e) {}
    void paint (juce::Graphics&) override;
    Editor& ed; float needle = 0; float hist[240] {}; int hw = 0;
};
class VariMuPage : public ModPage
{
public:
    VariMuPage (Editor&);
    void paint (juce::Graphics& g) override;
    void resized() override;
    VmView view; std::unique_ptr<Segs> mode, att, rel, hpf; std::unique_ptr<Switch> autoM, ms;
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
// viste del display: onda (bipolare), gain (unipolare in dB), spettro, immagine stereo, metering completo, ideale, reference
enum EasyView { V_WAVE, V_GAIN, V_SPEC, V_IMAGE, V_METER, V_IDEAL, V_REF, kEasyViews };
// colonne della storia ridotte a pixel (il massimo di quelle che ci cadono), in modo SCROLL o SCAN
struct PxEnv
{
    std::vector<float> pk, rm, gr; std::vector<int> beat; int n = 0; float cursor = -1;
    void size (int w) { n = w; pk.assign ((size_t) w, -200.0f); rm.assign ((size_t) w, -200.0f); gr.assign ((size_t) w, 0.0f); beat.assign ((size_t) w, -1); }
};
class EasyStage : public juce::Component
{
public:
    explicit EasyStage (EasyPage& p);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override { hov = -1; }
    EasyPage& page; Analyzer anIn, anOut; float specIn[300] {}, specOut[300] {}, specHold[300] {}; juce::Rectangle<float> loadR, listenR;
    int fixedView = -1;   // usata nell'AVANZATO: vista fissa, indipendente dalla pagina FACILE
    int curView() const;
    bool easy() const { return fixedView < 0; }
    int genreSel = -1; juce::Rectangle<float> genreR;
    float mtPk[2] { -100, -100 }, mtHold[2] {}; float bandCr[4] { 1, 1, 1, 1 };
    float pkHold[2] { -100, -100 }, holdS[2] {}, mPk[2] { -100, -100 }, mLu = -100, mRms[2] { -100, -100 }, barHold[40] {}, normOff = 0, crS = 1, crushS = 0.5f, wS = 0;
    double mLast = 0, disp = -1, lastT = 0, lastFrame = 0;
    double clock();   // posizione mostrata (in colonne), scorre col tempo vero e insegue i dati senza scatti
    void envelope (PxEnv&, int npx, double winSec);
    struct Hot { juce::Rectangle<float> r; int id; }; std::vector<Hot> hots; int hov = -1;
    juce::Image scope; std::unique_ptr<juce::VBlankAttachment> vb;
private:
    juce::Rectangle<float> visual();
    juce::Image panelImg, veilImg; juce::String panelKey; void panelDraw (juce::Graphics&);
    void panel (juce::Graphics&); void options (juce::Graphics&); void numbers (juce::Graphics&);
    void wave (juce::Graphics&, juce::Rectangle<float>, bool bipolar, double win, bool small = false); void spectrum (juce::Graphics&, juce::Rectangle<float>, int mode, bool small = false);
    void image (juce::Graphics&, juce::Rectangle<float>, bool small = false); void metering (juce::Graphics&, juce::Rectangle<float>);
    void ref (juce::Graphics&, juce::Rectangle<float>); void ideal (juce::Graphics&, juce::Rectangle<float>); void meterCol (juce::Graphics&, juce::Rectangle<float>);
    void opt (juce::Graphics&, juce::Rectangle<float>, const juce::String&, bool on, int id);
};
// ingresso in orizzontale nella testata del display: livello, zona ottimale, AUTO
class EasyInput : public juce::Component { public: explicit EasyInput (EasyPage& p) : page (p) { setRepaintsOnMouseActivity (true); } void paint (juce::Graphics&) override; void mouseDown (const juce::MouseEvent&) override; EasyPage& page; juce::Rectangle<float> autoR; float sm = -100; };

// guide rapide: cosa sono LUFS e dB, cosa fa Spotify, true peak, dinamica, correlazione, come si fa un master
class GuidePanel : public juce::Component
{
public:
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    int topic = 0, hover = -1; juce::Rectangle<float> closeR, topicR[8];
};

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
    EasyStage stage { *this }; EasyInput input { *this };
    GuidePanel guide; juce::Rectangle<float> guideR;
    neo::Segments tabs, styles; juce::Rectangle<float> targetR, cmpR, deltaR, themeR, bandR[3];
    juce::Image bg; float bgScale = 0; int bgTheme = -1;
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
    ExciterPage exPage { *this }; Eq432Page e4Page { *this }; PultecPage pqPage { *this }; VariMuPage vmPage { *this };
    BalancePage balPage { *this };
    RefPage refPage { *this };
    ThirdAnalyzer live;   // spettro medio dell'uscita (mobile, ~3 s)
    float liveThirds[kThirds] {};
    LimPage limPage { *this };
    SoonPage soon { *this };
    LoudPanel loud { *this };
    EasyPage easyPage { *this };
    EasyStage advStage { easyPage }; int advView = 0; neo::Segments advTabs; void applyAdvView();
    // le linguette MODULO / ONDA / SPETTRO / GAIN / IDEALE stanno in un componente sopra le pagine (prima le copriva la pagina del modulo)
    struct AdvTabsComp : juce::Component
    {
        explicit AdvTabsComp (Editor& e) : ed (e) { setInterceptsMouseClicks (true, false); }
        void paint (juce::Graphics&) override; void mouseDown (const juce::MouseEvent&) override;
        Editor& ed;
    } advTabsComp { *this };
    void applyMode();
    juce::Rectangle<int> modeR[2]; bool lastEasy = false;
    juce::Image neoBg; float neoScale = 0; int neoTheme = -1; neo::Segments modeSeg;
    void paintOverChildren (juce::Graphics&) override { neo::setTheme (0); }   // gli altri plugin Jackpot nella stessa DAW tornano al loro tema
    void paintNeo (juce::Graphics&);
private:
    void timerCallback() override;
    std::unique_ptr<Segs> dither; std::unique_ptr<Switch> compare, norm; std::unique_ptr<Knob> trim;
    juce::Rectangle<int> presetRect, prevRect, nextRect, abRect, balRect, refRect; bool abHover = false; int tick = 0;
};
}
