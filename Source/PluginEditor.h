#pragma once

#include "PluginProcessor.h"
#include "Japanese.h"
#include <tuple>
#include <functional>
#include <vector>

// Шрифт интерфейса: MS Gothic (есть в Windows). Чтобы сменить, замените имя ниже.
inline juce::Font vnFont (float height)
{
    return juce::Font (juce::FontOptions ("MS Gothic", height, juce::Font::plain));
}

//==============================================================================
// Оформление в духе системных окон новеллы середины 2000-х: тёмно-синие
// полупрозрачные панели, тонкие светлые рамки, скошенные "кнопки" и полосы.
class SoraLookAndFeel : public juce::LookAndFeel_V4
{
public:
    SoraLookAndFeel();
    void applyPalette();   // цвета обычного и секретного режима

    void drawLinearSlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           juce::Slider::SliderStyle, juce::Slider&) override;

    int getSliderThumbRadius (juce::Slider&) override;

    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&,
                               bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height, float sliderPosProportional,
                           float rotaryStartAngle, float rotaryEndAngle, juce::Slider&) override;

    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool shouldDrawButtonAsHighlighted,
                           bool shouldDrawButtonAsDown) override;

    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
    juce::Font getLabelFont (juce::Label&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getPopupMenuFont() override;

private:
    void drawLever (juce::Graphics&, int x, int y, int width, int height, float sliderPos, juce::Slider&);
};

//==============================================================================
// Слайдер, который по правой кнопке мыши вызывает меню хоста (там FL Studio даёт
// "Create automation clip"), а по левой работает как обычно.
class HostSlider : public juce::Slider
{
public:
    using juce::Slider::Slider;

    std::function<void()> onContextMenu;

    void mouseDown (const juce::MouseEvent& e) override
    {
        blocked = e.mods.isPopupMenu();
        if (blocked) { if (onContextMenu) onContextMenu(); return; }
        juce::Slider::mouseDown (e);
    }

    void mouseDrag (const juce::MouseEvent& e) override { if (! blocked) juce::Slider::mouseDrag (e); }

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (! blocked) juce::Slider::mouseUp (e);
        blocked = false;
    }

private:
    bool blocked = false;
};

//==============================================================================
// Падающие лепестки сакуры или снег. Отдельный прозрачный слой над сценой:
// ограниченное число частиц, готовые маленькие спрайты, перерисовываются только
// небольшие области вокруг частиц, поэтому плагин не тормозит.
class FallLayer : public juce::Component
{
public:
    FallLayer();

    void setMode (int newMode);          // 0 выкл, 1 лепестки, 2 снег
    void setBlood (bool on) { blood = on; repaint(); }
    std::function<void()> onFull;        // поймано 4 лепестка

    bool hitTest (int x, int y) override { return findPetal (x, y) >= 0; }
    void mouseDown (const juce::MouseEvent&) override;
    void step (float dt);
    void paint (juce::Graphics&) override;

private:
    struct Particle
    {
        float x = 0, y = 0, vy = 30, baseX = 0, swayAmp = 10, swayFreq = 1, phase = 0;
        float rot = 0, spin = 0, flip = 0, flipSpeed = 1, size = 1, drift = 0;
        int kind = 0;
    };

    void respawn (Particle&, bool anywhere);

    // эффект подбора: лепесток летит в ячейку, на месте клика вспышка
    struct Fly   { float sx, sy, tx, ty, t, rot, size; int kind; };
    struct Burst { float x, y, t; };
    std::vector<Fly> flies;
    std::vector<Burst> bursts;
    int caught = 0, landed = 0;
    static juce::Point<float> slotCentre (int i) { return { 34.f + (float) i * 26.f, 70.f }; }
    int findPetal (int x, int y) const;
    juce::Rectangle<int> boundsOf (const Particle&) const;

    static constexpr int maxParticles = 96;
    std::array<Particle, maxParticles> ps;
    int count = 0, mode = 0;
    bool blood = false;
    float time = 0.f;
    juce::Image petals[6], flakes[2];
    void setCaughtReset();
    juce::Random rng;
};

//==============================================================================
// Секретная викторина: вопрос про любимую героиню, затем один случайный из двух
// вопросов по ней. Верный ответ включает секретный режим, неверный закрывает окно.
class SecretQuiz : public juce::Component
{
public:
    SecretQuiz();

    void start();
    void paint (juce::Graphics&) override;

    std::function<void()> onYes, onNo;   // правильный ответ / неправильный

private:
    void showStage();
    void answer (int index);

    int stage = 0;    // 0 выбор героини, 1 случайный вопрос по ней
    int girl = -1, question = 0;
    juce::Random rng;
    juce::TextButton buttons[6];
};

//==============================================================================
// Мини-визуалайзер секретной страницы 2: четыре стиля, можно выключить
// (тогда на экране надпись). Таймер работает только пока он виден и включён.
class MiniScope : public juce::Component, private juce::Timer
{
public:
    explicit MiniScope (SoraKickAudioProcessor&);

    void setStyle (int s) { style = s; repaint(); }
    void setRunning (bool on);
    void paint (juce::Graphics&) override;
    void visibilityChanged() override { updateTimer(); }

private:
    void timerCallback() override;
    void updateTimer();

    SoraKickAudioProcessor& proc;
    juce::dsp::FFT fft { 9 };
    std::array<float, 1024> wave {};
    std::array<float, 1024> fftData {};
    std::array<float, 32> bars {};
    std::array<float, 32> peaks {};
    int style = 0;
    bool running = true;
};

// цвета колокольчиков из выбранного оттенка (режим SINGLE)
inline juce::Colour bellGlassColour (float hue, float light)
{
    return juce::Colour::fromHSV (hue, 0.24f, 0.60f + 0.40f * light, 1.f);
}

inline juce::Colour bellPaperColour (float hue, float light)
{
    return juce::Colour::fromHSV (hue, 0.10f, 0.90f + 0.10f * light, 1.f);
}

//==============================================================================
// Маленькое выдвижное окошко справа: вкладка MAIN (waveshaper и колокольчики
// включить/выключить) и вкладка BELLS (цвет колокольчиков).
// Настройки колокольчиков хранятся в состоянии плагина вместе с проектом.
class SettingsPanel : public juce::Component
{
public:
    static constexpr int handleW = 26, bodyW = 210, panelW = handleW + bodyW, panelH = 224;

    explicit SettingsPanel (SoraKickAudioProcessor&);

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;

    std::function<void()> onToggle, onSecretOff;
    bool open = false;

    void refreshTexts();
    void setSecretMode (bool on);

private:
    void showTab (int t);

    SoraKickAudioProcessor& proc;
    int tab = 0;

    juce::TextButton tabButtons[2], modeButtons[2], fallButtons[3];
    juce::ToggleButton shaperToggle, bellsToggle, secretToggle;
    bool secretShown = false;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> shaperAtt;
    juce::Slider hueSlider, lightSlider;
};

//==============================================================================
// Редактор кривой waveshaper: рисуйте мышью кривую передачи (вход -> выход),
// двойной клик возвращает прямую линию (звук без изменений).
class WaveEditor : public juce::Component
{
public:
    explicit WaveEditor (SoraKickAudioProcessor& p) : proc (p) {}

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

private:
    void setFromMouse (juce::Point<float> pos, bool firstPoint);

    SoraKickAudioProcessor& proc;
    int lastIdx = -1;
    float lastVal = 0.f;
};

//==============================================================================
class SoraKickAudioProcessorEditor : public juce::AudioProcessorEditor,
                                     private juce::Timer
{
public:
    explicit SoraKickAudioProcessorEditor (SoraKickAudioProcessor&);
    ~SoraKickAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;   // клик по фону = реплика героини, по колокольчику = звон

private:
    // японский колокольчик (фурин): маятник + бумажная полоска, реагирует на курсор
    struct Bell
    {
        float anchorX = 0.f, len = 60.f;
        float angle = 0.f, vel = 0.f;       // угол (рад) и угловая скорость
        float tail = 0.f, tailVel = 0.f;    // отстающая бумажная полоска
        float omega = 4.f, phase = 0.f;
        float ring = 0.f;                   // подсветка звона
        juce::Image pend, strip;            // готовые картинки колокольчика и бумажки
        juce::Colour spriteGlass, spritePaper;
        juce::Rectangle<int> lastRect;      // область, где колокольчик рисовался в прошлый раз
        juce::Colour glass, paper, glass0, paper0;
    };

    juce::Point<float> bellCentre (const Bell&, float distance) const;
    void initBells();
    void updateBells();
    void drawBell (juce::Graphics&, Bell&);
    void renderBellSprite (juce::Graphics&, const Bell&, int part) const;
    void ensureBellSprites (Bell&);
    static constexpr int bellSpriteScale = 2;
    void applyBellColours();

    struct Knob
    {
        const char* en = "";
        juce::Label label;
        HostSlider slider { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow };
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    void addKnob (const char* paramID, const char* title, bool secretSet = false);
    void addLever (const char* paramID, const char* title);
    bool wasSecretBefore() const;
    void rememberSecret();
    void setVisStyle (int s);
    void setVisOn (bool on);
    void setPage (int newPage);
    void placePanel();
    void showHostMenu (const juce::String& paramID);
    void setSecret (bool on);
    void refreshTexts();
    void setUiHidden (bool hide);

    using APVTS = juce::AudioProcessorValueTreeState;

    struct Row
    {
        const char* en = "";
        juce::Label label;
        HostSlider slider { juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight };
        std::unique_ptr<APVTS::SliderAttachment> attachment;
    };

    void timerCallback() override;
    void buildScenes();
    void addRow (const char* paramID, const char* title);
    void updateBellRects();
    void showRandomPhrase();

    SoraKickAudioProcessor& proc;
    SoraLookAndFeel laf;

    // фоны (по одному на каждый пункт Character) и плавная смена между ними
    std::vector<juce::Image> scenes;
    int shownScene = 0, prevScene = 0;
    float fade = 1.f;
    int blinkTicks = 0;

    // реплика героини
    juce::String phraseSpeaker, phraseText;
    int shownChars = 0;
    int lastPhrase = -1;
    juce::Random rng;

    std::vector<std::unique_ptr<Row>> rows;

    juce::Label leverLabel, characterLabel;
    HostSlider leverSlider { juce::Slider::LinearVertical, juce::Slider::TextBoxBelow };
    std::unique_ptr<APVTS::SliderAttachment> leverAtt;
    bool leverSnapGuard = false;

    juce::ComboBox characterBox, bgBox;   // bgBox: выбор фона в секретном режиме (на звук не влияет)
    int pageNormal = 0, pageSecret = 0;
    std::unique_ptr<APVTS::ComboBoxAttachment> characterAtt;

    std::array<Bell, 7> bells;
    juce::Point<float> lastMouse;
    bool haveLastMouse = false;
    float windTime = 0.f;

    WaveEditor waveEditor;

    // страница 2 (Sakura FX) и выдвижное окошко
    int page = 0;
    juce::TextButton pageButtons[2], resetButton;
    FallLayer fall;
    int fallMode = 0;
    int caught = 0;
    bool secret = false;
    SecretQuiz quiz;
    juce::TextButton exitButton, hideButton;
    bool uiHidden = false;
    std::vector<std::tuple<juce::Component*, bool, bool>> hiddenState;
    std::vector<std::unique_ptr<Knob>> secKnobs;

    struct Lever
    {
        const char* en = "";
        juce::Label label;
        HostSlider slider { juce::Slider::LinearVertical, juce::Slider::TextBoxBelow };
        std::unique_ptr<APVTS::SliderAttachment> attachment;
    };
    std::vector<std::unique_ptr<Lever>> secLevers;
    MiniScope scope;
    juce::TextButton visButtons[4], visOnButton, secResetButton, secretEnterButton;
    int visStyle = 0;
    bool visOn = true;
    bool bellsDrawn = false;
    void updateEnterButton();
    std::vector<juce::Image> secretScenes;
    std::vector<std::unique_ptr<Knob>> knobs;
    juce::ComboBox syncBox;
    std::unique_ptr<APVTS::ComboBoxAttachment> syncAtt;

    SettingsPanel panel;
    float panelPos = 0.f;
    bool bellsOn = false;
    bool lastShaperOn = true;
    juce::TextButton waveButtons[4];
    int shownWaveVersion = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SoraKickAudioProcessorEditor)
};
