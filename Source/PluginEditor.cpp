#include "PluginEditor.h"
#include "Phrases.h"
#include "Quiz.h"
#include "Scenes.h"

#ifndef SORA_HAS_ASSETS
 #define SORA_HAS_ASSETS 0
#endif

#if SORA_HAS_ASSETS
 #include <BinaryData.h>
#endif

static_assert (sizeof (phrasesJa) / sizeof (phrasesJa[0]) == (size_t) numPhrases,
               "phrasesJa must match Phrases.h");

namespace
{
    constexpr int windowW = 1000;
    constexpr int windowH = 700;

    // палитра окон новеллы: тёмно-синий, светлые рамки, кремовый текст
    juce::Colour panelTop    (0xc81a2350);
    juce::Colour panelBottom (0xc810173a);
    juce::Colour edgeLight   (0xffb7c9ee);
    juce::Colour textCream   (0xfff3f1ff);
    juce::Colour accentTop   (0xffcfe2ff);
    juce::Colour accentBottom(0xff6f9ad6);
    juce::Colour navyDark    (0xff0b1030);
    juce::Colour bevelDark   (0xff1a2650);

    // остальные цвета кнопок, ползунков и цветков (тоже меняются в секретном режиме)
    juce::Colour btnTopN (0xff2a3770), btnBotN (0xff161e4a), btnTopH (0xff3a4a8c), btnBotH (0xff1f2b66),
                 btnTopD (0xff3b4f93), btnBotD (0xff1c2762), popupHi (0xff3f5fa8),
                 thumbTop (0xfff7faff), thumbBot (0xff9db6df), thumbLine (0xff4a5f94), gripBot (0xff8ea8d6),
                 arcVal (0xffff9fc2), flTip (0xffffe9f1), flBase (0xfff07aa5), flOut (0xffc2547f),
                 flVein (0xffe6769f), flStamen (0xffd24d7f), flCenter (0xffffe28a), plate (0xd8121a44);

    // обычная палитра или секретная, кроваво-красная
    void setPalette (bool secretMode)
    {
        if (secretMode)
        {
            panelTop     = juce::Colour (0xc83a0a10);
            panelBottom  = juce::Colour (0xc8200508);
            edgeLight    = juce::Colour (0xffeaa0a0);
            textCream    = juce::Colour (0xffffe8e8);
            accentTop    = juce::Colour (0xffff9a9a);
            accentBottom = juce::Colour (0xffc01818);
            navyDark     = juce::Colour (0xff1c0408);
            bevelDark    = juce::Colour (0xff3a0a10);
            btnTopN = juce::Colour (0xff7a1a22); btnBotN = juce::Colour (0xff3e0a10);
            btnTopH = juce::Colour (0xff962630); btnBotH = juce::Colour (0xff54121a);
            btnTopD = juce::Colour (0xffa33038); btnBotD = juce::Colour (0xff2a070c);
            popupHi = juce::Colour (0xffa02028);
            thumbTop = juce::Colour (0xffffd6d6); thumbBot = juce::Colour (0xffc46a6a);
            thumbLine = juce::Colour (0xff6a1a1a); gripBot = juce::Colour (0xffc46a6a);
            arcVal = juce::Colour (0xffff5a5a);
            flTip = juce::Colour (0xffff9a9a); flBase = juce::Colour (0xffa01018);
            flOut = juce::Colour (0xff5a0a10); flVein = juce::Colour (0xff7a0f16);
            flStamen = juce::Colour (0xff400408); flCenter = juce::Colour (0xffd8a0a0);
            plate = juce::Colour (0xd8300a10);
        }
        else
        {
            panelTop     = juce::Colour (0xc81a2350);
            panelBottom  = juce::Colour (0xc810173a);
            edgeLight    = juce::Colour (0xffb7c9ee);
            textCream    = juce::Colour (0xfff3f1ff);
            accentTop    = juce::Colour (0xffcfe2ff);
            accentBottom = juce::Colour (0xff6f9ad6);
            navyDark     = juce::Colour (0xff0b1030);
            bevelDark    = juce::Colour (0xff1a2650);
            btnTopN = juce::Colour (0xff2a3770); btnBotN = juce::Colour (0xff161e4a);
            btnTopH = juce::Colour (0xff3a4a8c); btnBotH = juce::Colour (0xff1f2b66);
            btnTopD = juce::Colour (0xff3b4f93); btnBotD = juce::Colour (0xff1c2762);
            popupHi = juce::Colour (0xff3f5fa8);
            thumbTop = juce::Colour (0xfff7faff); thumbBot = juce::Colour (0xff9db6df);
            thumbLine = juce::Colour (0xff4a5f94); gripBot = juce::Colour (0xff8ea8d6);
            arcVal = juce::Colour (0xffff9fc2);
            flTip = juce::Colour (0xffffe9f1); flBase = juce::Colour (0xfff07aa5);
            flOut = juce::Colour (0xffc2547f); flVein = juce::Colour (0xffe6769f);
            flStamen = juce::Colour (0xffd24d7f); flCenter = juce::Colour (0xffffe28a);
            plate = juce::Colour (0xd8121a44);
        }
    }

    //--------------------------------------------------------------------------
    // Необязательные собственные картинки из Resources/*.png
    juce::Image loadAsset (const juce::String& resourceName)
    {
       #if SORA_HAS_ASSETS
        int size = 0;
        if (auto* data = BinaryData::getNamedResource (resourceName.toRawUTF8(), size))
            return juce::ImageFileFormat::loadFrom (data, (size_t) size);
       #else
        juce::ignoreUnused (resourceName);
       #endif
        return {};
    }

    // скошенный прямоугольник: светлые верх и лево, тёмные низ и право
    void drawBevelBox (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour light, juce::Colour dark)
    {
        g.setColour (light);
        g.drawLine (r.getX(), r.getY(), r.getRight(), r.getY(), 1.f);
        g.drawLine (r.getX(), r.getY(), r.getX(), r.getBottom(), 1.f);
        g.setColour (dark);
        g.drawLine (r.getX(), r.getBottom(), r.getRight(), r.getBottom(), 1.f);
        g.drawLine (r.getRight(), r.getY(), r.getRight(), r.getBottom(), 1.f);
    }
}

namespace
{
    // Пиксельный текст: рисуем MS Gothic 12 пт в маленькую картинку (там у шрифта
    // встроенные пиксельные глифы) и увеличиваем без сглаживания. Получается
    // крупный текст в стиле старых новелл, а не гладкий шрифт.
    void drawPixelText (juce::Graphics& g, const juce::String& text, juce::Rectangle<int> area, float scale)
    {
        const int w = juce::jmax (1, (int) ((float) area.getWidth() / scale));
        const int h = juce::jmax (1, (int) ((float) area.getHeight() / scale));

        juce::Image img (juce::Image::ARGB, w, h, true);
        {
            juce::Graphics gi (img);
            gi.setFont (vnFont (12.f));
            gi.setColour (textCream);
            gi.drawFittedText (text, 0, 0, w, h, juce::Justification::topLeft, 2, 1.0f);
        }

        juce::Graphics::ScopedSaveState state (g);
        g.setImageResamplingQuality (juce::Graphics::lowResamplingQuality);
        g.drawImageTransformed (img, juce::AffineTransform::scale (scale)
                                         .translated ((float) area.getX(), (float) area.getY()));
    }

    // лепесток сакуры с вырезом на конце, направлен вверх из начала координат
    juce::Path makePetal (float r)
    {
        juce::Path p;
        p.startNewSubPath (0.f, 0.f);
        p.cubicTo (-0.55f * r, -0.12f * r, -0.66f * r, -0.80f * r, -0.24f * r, -1.00f * r);
        p.lineTo (0.f, -0.84f * r);
        p.lineTo (0.24f * r, -1.00f * r);
        p.cubicTo (0.66f * r, -0.80f * r, 0.55f * r, -0.12f * r, 0.f, 0.f);
        p.closeSubPath();
        return p;
    }
}

//==============================================================================
SoraLookAndFeel::SoraLookAndFeel()
{
    applyPalette();
}

void SoraLookAndFeel::applyPalette()
{
    setColour (juce::Slider::textBoxTextColourId, textCream);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Label::textColourId, textCream);

    setColour (juce::ComboBox::backgroundColourId, navyDark.withAlpha (0.8f));
    setColour (juce::ComboBox::textColourId, textCream);
    setColour (juce::ComboBox::outlineColourId, edgeLight.withAlpha (0.7f));
    setColour (juce::ComboBox::arrowColourId, accentTop);

    setColour (juce::PopupMenu::backgroundColourId, navyDark.withAlpha (0.94f));
    setColour (juce::PopupMenu::textColourId, textCream);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, popupHi);
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);

    setColour (juce::TextButton::textColourOffId, textCream);
    setColour (juce::TextButton::textColourOnId, textCream);
}

juce::Font SoraLookAndFeel::getLabelFont (juce::Label& l)          { return vnFont (l.getFont().getHeight()); }
juce::Font SoraLookAndFeel::getComboBoxFont (juce::ComboBox&)      { return vnFont (14.f); }
juce::Font SoraLookAndFeel::getPopupMenuFont()                      { return vnFont (14.f); }
juce::Font SoraLookAndFeel::getTextButtonFont (juce::TextButton&, int) { return vnFont (12.f); }

int SoraLookAndFeel::getSliderThumbRadius (juce::Slider& slider)
{
    return slider.isHorizontal() ? 5 : 9;
}

void SoraLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour&,
                                            bool highlighted, bool down)
{
    const auto b = button.getLocalBounds().toFloat().reduced (0.5f);
    down = down || button.getToggleState();

    const auto top    = down ? btnTopD : highlighted ? btnTopH : btnTopN;
    const auto bottom = down ? btnBotD : highlighted ? btnBotH : btnBotN;

    g.setGradientFill (juce::ColourGradient (top, 0.f, b.getY(), bottom, 0.f, b.getBottom(), false));
    g.fillRect (b);

    if (down)
        drawBevelBox (g, b, bevelDark, edgeLight.withAlpha (0.8f));
    else
        drawBevelBox (g, b, edgeLight.withAlpha (0.8f), bevelDark);
}

void SoraLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                                        float sliderPos, float, float, juce::Slider::SliderStyle style,
                                        juce::Slider& slider)
{
    if (style == juce::Slider::LinearVertical)
    {
        drawLever (g, x, y, width, height, sliderPos, slider);
        return;
    }

    const float cy = (float) y + (float) height * 0.5f;
    const float left = (float) x, right = (float) (x + width);
    const juce::Rectangle<float> track (left, cy - 3.f, right - left, 6.f);

    // риски над полосой
    g.setColour (edgeLight.withAlpha (0.35f));
    for (int k = 0; k <= 4; ++k)
    {
        const float tx = left + (float) k * 0.25f * (right - left);
        g.drawLine (tx, cy - 10.f, tx, cy - 6.f, 1.f);
    }

    // жёлоб
    g.setColour (navyDark.withAlpha (0.9f));
    g.fillRect (track);

    // заливка
    const juce::Rectangle<float> fill (track.getX(), track.getY(), juce::jmax (0.f, sliderPos - track.getX()), track.getHeight());
    g.setGradientFill (juce::ColourGradient (accentTop, 0.f, fill.getY(), accentBottom, 0.f, fill.getBottom(), false));
    g.fillRect (fill);

    g.setColour (edgeLight.withAlpha (0.7f));
    g.drawRect (track.expanded (1.f), 1.f);

    // бегунок
    const float hw = 9.f, hh = 17.f;
    const juce::Rectangle<float> th (sliderPos - hw * 0.5f, cy - hh * 0.5f, hw, hh);

    g.setGradientFill (juce::ColourGradient (thumbTop, 0.f, th.getY(),
                                             thumbBot, 0.f, th.getBottom(), false));
    g.fillRect (th);
    drawBevelBox (g, th, juce::Colours::white.withAlpha (0.9f), bevelDark);

    g.setColour (thumbLine);
    g.drawLine (sliderPos, th.getY() + 4.f, sliderPos, th.getBottom() - 4.f, 1.f);
}

// Крутилка в виде цветка сакуры: пять лепестков поворачиваются вместе со значением,
// вокруг тонкая шкала, на кончике одного лепестка белая точка-указатель.
void SoraLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                        float pos, float startAngle, float endAngle, juce::Slider&)
{
    const float d = (float) juce::jmin (width, height) - 6.f;
    const float cx = (float) x + (float) width * 0.5f;
    const float cy = (float) y + (float) height * 0.5f;
    const float R = d * 0.5f;
    const float angle = startAngle + pos * (endAngle - startAngle);

    // тёмная плитка с рамкой
    g.setColour (navyDark.withAlpha (0.78f));
    g.fillEllipse (cx - R, cy - R, 2.f * R, 2.f * R);
    g.setColour (edgeLight.withAlpha (0.55f));
    g.drawEllipse (cx - R, cy - R, 2.f * R, 2.f * R, 1.f);

    // шкала
    juce::Path track;
    track.addCentredArc (cx, cy, R - 5.f, R - 5.f, 0.f, startAngle, endAngle, true);
    g.setColour (edgeLight.withAlpha (0.28f));
    g.strokePath (track, juce::PathStrokeType (2.f));

    juce::Path value;
    value.addCentredArc (cx, cy, R - 5.f, R - 5.f, 0.f, startAngle, juce::jmax (startAngle + 0.001f, angle), true);
    g.setColour (arcVal);
    g.strokePath (value, juce::PathStrokeType (3.f));

    // цветок
    const float fr = R * 0.80f;
    const auto petal = makePetal (fr);

    for (int k = 0; k < 5; ++k)
    {
        juce::Graphics::ScopedSaveState state (g);
        g.addTransform (juce::AffineTransform::rotation (angle + (float) k * juce::MathConstants<float>::twoPi / 5.f)
                            .translated (cx, cy));

        g.setGradientFill (juce::ColourGradient (flTip, 0.f, -fr,
                                                 flBase, 0.f, 0.f, false));
        g.fillPath (petal);

        g.setColour (flOut.withAlpha (0.85f));
        g.strokePath (petal, juce::PathStrokeType (0.9f));

        // прожилка
        g.setColour (flVein.withAlpha (0.7f));
        g.drawLine (0.f, -0.12f * fr, 0.f, -0.62f * fr, 0.8f);
    }

    // серединка с тычинками
    g.setColour (flCenter);
    g.fillEllipse (cx - 4.f, cy - 4.f, 8.f, 8.f);
    g.setColour (flStamen);
    for (int k = 0; k < 8; ++k)
    {
        const float a = (float) k * juce::MathConstants<float>::twoPi / 8.f + angle;
        g.fillEllipse (cx + std::sin (a) * fr * 0.22f - 1.2f, cy - std::cos (a) * fr * 0.22f - 1.2f, 2.4f, 2.4f);
    }

    // указатель на кончике первого лепестка
    g.setColour (juce::Colours::white);
    g.fillEllipse (cx + std::sin (angle) * fr * 0.88f - 2.5f, cy - std::cos (angle) * fr * 0.88f - 2.5f, 5.f, 5.f);
}

void SoraLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button, bool highlighted, bool)
{
    const auto r = button.getLocalBounds().toFloat();
    const juce::Rectangle<float> box (r.getX() + 1.f, r.getCentreY() - 8.f, 16.f, 16.f);

    g.setColour (navyDark.withAlpha (0.92f));
    g.fillRect (box);
    drawBevelBox (g, box, bevelDark, edgeLight.withAlpha (0.8f));

    if (button.getToggleState())
    {
        juce::Path tick;
        tick.startNewSubPath (box.getX() + 3.f, box.getCentreY());
        tick.lineTo (box.getX() + 6.5f, box.getBottom() - 4.f);
        tick.lineTo (box.getRight() - 3.f, box.getY() + 4.f);
        g.setColour (arcVal);
        g.strokePath (tick, juce::PathStrokeType (2.f));
    }

    g.setColour (highlighted ? juce::Colours::white : textCream);
    g.setFont (vnFont (12.f));
    g.drawText (button.getButtonText(), r.withTrimmedLeft (24.f).toNearestInt(), juce::Justification::centredLeft);
}

// Рычажок питча: вертикальный паз с рисками каждые 2 полутона, ноль в центре
void SoraLookAndFeel::drawLever (juce::Graphics& g, int x, int y, int width, int height,
                                 float sliderPos, juce::Slider& slider)
{
    const float cx = (float) x + (float) width * 0.5f;
    const float radius = (float) getSliderThumbRadius (slider);
    const float top = (float) y + radius;
    const float bottom = (float) (y + height) - radius;

    auto yFor = [&] (double value)
    {
        return bottom - (float) slider.valueToProportionOfLength (value) * (bottom - top);
    };

    // паз
    const juce::Rectangle<float> groove (cx - 4.f, top - 4.f, 8.f, bottom - top + 8.f);
    g.setColour (navyDark.withAlpha (0.92f));
    g.fillRect (groove);

    const bool generic = (bool) slider.getProperties().getWithDefault ("generic", false);

    // подсветка от нуля до рычажка
    const float zeroY = generic ? bottom : yFor (0.0);
    const float fy1 = juce::jmin (zeroY, sliderPos), fy2 = juce::jmax (zeroY, sliderPos);
    g.setGradientFill (juce::ColourGradient (accentTop, cx, fy1, accentBottom, cx, fy2, false));
    g.fillRect (cx - 3.f, fy1, 6.f, fy2 - fy1);

    g.setColour (edgeLight.withAlpha (0.7f));
    g.drawRect (groove.expanded (1.f), 1.f);

    // риски
    if (generic)
    {
        for (int s = 0; s <= 4; ++s)
        {
            const float ty = bottom - (float) s * 0.25f * (bottom - top);
            const float len = (s == 0 || s == 4) ? 9.f : 5.f;
            g.setColour (edgeLight.withAlpha (0.55f));
            g.drawLine (cx - 7.f - len, ty, cx - 7.f, ty, 1.f);
            g.drawLine (cx + 7.f, ty, cx + 7.f + len, ty, 1.f);
        }
    }
    else
    for (int s = -12; s <= 12; s += 2)
    {
        const float ty = yFor ((double) s);
        const bool major = (s == 0 || s == 12 || s == -12);
        const float len = major ? 11.f : 6.f;

        g.setColour (edgeLight.withAlpha (major ? 0.85f : 0.4f));
        g.drawLine (cx - 7.f - len, ty, cx - 7.f, ty, 1.f);
        g.drawLine (cx + 7.f, ty, cx + 7.f + len, ty, 1.f);
    }

    if (! generic)
    {
        g.setColour (edgeLight.withAlpha (0.9f));
        g.setFont (vnFont (10.f));
        g.drawText ("+12", (int) (cx + 22.f), (int) (yFor (12.0) - 6.f), 26, 12, juce::Justification::centredLeft);
        g.drawText ("0",   (int) (cx + 22.f), (int) (zeroY - 6.f),        26, 12, juce::Justification::centredLeft);
        g.drawText ("-12", (int) (cx + 22.f), (int) (yFor (-12.0) - 6.f), 26, 12, juce::Justification::centredLeft);
    }

    // рукоять
    const float gw = 38.f, gh = 17.f;
    const juce::Rectangle<float> grip (cx - gw * 0.5f, sliderPos - gh * 0.5f, gw, gh);

    g.setGradientFill (juce::ColourGradient (thumbTop, 0.f, grip.getY(),
                                             gripBot, 0.f, grip.getBottom(), false));
    g.fillRect (grip);
    drawBevelBox (g, grip, juce::Colours::white.withAlpha (0.9f), bevelDark);

    g.setColour (thumbLine);
    for (int k = -1; k <= 1; ++k)
        g.drawLine (grip.getX() + 8.f, grip.getCentreY() + (float) k * 4.f,
                    grip.getRight() - 8.f, grip.getCentreY() + (float) k * 4.f, 1.f);
}

//==============================================================================
void WaveEditor::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const float w = b.getWidth(), h = b.getHeight();
    const float amp = h * 0.46f;

    g.setColour (navyDark.withAlpha (0.78f));
    g.fillRect (b);

    // сетка: оси вход/выход
    g.setColour (edgeLight.withAlpha (0.28f));
    g.drawHorizontalLine ((int) (h * 0.5f), 0.f, w);
    g.drawVerticalLine ((int) (w * 0.5f), 0.f, h);
    g.setColour (edgeLight.withAlpha (0.12f));
    for (int k = 1; k < 4; k += 2)
    {
        g.drawVerticalLine ((int) (w * (float) k * 0.25f), 0.f, h);
        g.drawHorizontalLine ((int) (h * (float) k * 0.25f), 0.f, w);
    }

    // прямая "без изменений" для сравнения
    g.setColour (edgeLight.withAlpha (0.3f));
    g.drawLine (0.f, h * 0.5f + amp, w, h * 0.5f - amp, 1.f);

    float pts[WaveTable::numPoints];
    float table[WaveTable::tableSize + 1];
    for (int i = 0; i < WaveTable::numPoints; ++i)
        pts[i] = proc.getWavePoint (i);
    WaveTable::build (pts, table);

    juce::Path curve;
    for (int t = 0; t <= WaveTable::tableSize; t += 4)
    {
        const float px = (float) t / (float) WaveTable::tableSize * w;
        const float py = h * 0.5f - table[t] * amp;

        if (t == 0) curve.startNewSubPath (px, py);
        else        curve.lineTo (px, py);
    }

    g.setColour (accentTop);
    g.strokePath (curve, juce::PathStrokeType (1.8f));

    // точки, за которые можно тянуть
    g.setColour (juce::Colours::white.withAlpha (0.85f));
    for (int i = 0; i < WaveTable::numPoints; ++i)
    {
        const float px = (float) i / (float) (WaveTable::numPoints - 1) * w;
        const float py = h * 0.5f - pts[i] * amp;
        g.fillRect (px - 1.5f, py - 1.5f, 3.f, 3.f);
    }

    if (! proc.isShaperOn())
    {
        g.setColour (navyDark.withAlpha (0.65f));
        g.fillRect (b);
        g.setColour (edgeLight);
        g.setFont (vnFont (12.f));
        g.drawText (tr ("WAVESHAPER OFF"), getLocalBounds(), juce::Justification::centred);
    }

    g.setColour (edgeLight.withAlpha (0.7f));
    g.drawRect (b, 1.f);
}

void WaveEditor::setFromMouse (juce::Point<float> pos, bool firstPoint)
{
    const float w = (float) getWidth(), h = (float) getHeight();

    const int idx = juce::jlimit (0, WaveTable::numPoints - 1,
                                  (int) std::round (pos.x / w * (float) (WaveTable::numPoints - 1)));
    const float val = juce::jlimit (-1.f, 1.f, (h * 0.5f - pos.y) / (h * 0.46f));

    if (! firstPoint && lastIdx >= 0 && lastIdx != idx)
    {
        // заполняем пропущенные точки между двумя положениями мыши
        const int steps = std::abs (idx - lastIdx);
        const int dir = idx > lastIdx ? 1 : -1;

        for (int s = 1; s <= steps; ++s)
        {
            const float v = lastVal + (val - lastVal) * (float) s / (float) steps;
            proc.setWavePoint (lastIdx + dir * s, v);
        }
    }
    else
    {
        proc.setWavePoint (idx, val);
    }

    lastIdx = idx;
    lastVal = val;
    proc.notifyWaveChanged();
    repaint();
}

void WaveEditor::mouseDown (const juce::MouseEvent& e)        { setFromMouse (e.position, true); }
void WaveEditor::mouseDrag (const juce::MouseEvent& e)        { setFromMouse (e.position, false); }

void WaveEditor::mouseDoubleClick (const juce::MouseEvent&)
{
    proc.setWavePreset (0);
    repaint();
}

//==============================================================================
SettingsPanel::SettingsPanel (SoraKickAudioProcessor& p) : proc (p)
{
    auto& st = proc.apvts.state;
    st.setProperty ("bellsOn", false, nullptr);   // при запуске колокольчики выключены

    const char* tabNames[2] = { "MAIN", "BELLS" };
    for (int i = 0; i < 2; ++i)
    {
        tabButtons[i].setButtonText (tabNames[i]);
        tabButtons[i].setClickingTogglesState (true);
        tabButtons[i].setRadioGroupId (2701);
        tabButtons[i].onClick = [this, i] { showTab (i); };
        addAndMakeVisible (tabButtons[i]);
    }
    tabButtons[0].setToggleState (true, juce::dontSendNotification);

    shaperToggle.setButtonText ("WAVESHAPER");
    addAndMakeVisible (shaperToggle);
    shaperAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "shaperOn", shaperToggle);

    secretToggle.setButtonText ("SECRET");
    secretToggle.setToggleState (true, juce::dontSendNotification);
    secretToggle.onClick = [this]
    {
        if (! secretToggle.getToggleState() && onSecretOff)
            onSecretOff();
    };
    addChildComponent (secretToggle);

    bellsToggle.setButtonText ("BELLS");
    bellsToggle.setToggleState ((bool) st.getProperty ("bellsOn", false), juce::dontSendNotification);
    bellsToggle.onClick = [this]
    {
        proc.apvts.state.setProperty ("bellsOn", bellsToggle.getToggleState(), nullptr);
    };
    addAndMakeVisible (bellsToggle);

    const char* modeNames[2] = { "ORIGINAL", "SINGLE" };
    const bool single = (bool) st.getProperty ("bellSingle", false);
    for (int i = 0; i < 2; ++i)
    {
        modeButtons[i].setButtonText (modeNames[i]);
        modeButtons[i].setClickingTogglesState (true);
        modeButtons[i].setRadioGroupId (2702);
        modeButtons[i].onClick = [this, i]
        {
            proc.apvts.state.setProperty ("bellSingle", i == 1, nullptr);
            repaint();
        };
        addAndMakeVisible (modeButtons[i]);
    }
    modeButtons[single ? 1 : 0].setToggleState (true, juce::dontSendNotification);

    const char* fallNames[3] = { "OFF", "PETALS", "SNOW" };
    const int fallMode = (int) st.getProperty ("fallMode", 0);
    for (int i = 0; i < 3; ++i)
    {
        fallButtons[i].setButtonText (fallNames[i]);
        fallButtons[i].setClickingTogglesState (true);
        fallButtons[i].setRadioGroupId (2703);
        fallButtons[i].onClick = [this, i] { proc.apvts.state.setProperty ("fallMode", i, nullptr); };
        addAndMakeVisible (fallButtons[i]);
    }
    fallButtons[juce::jlimit (0, 2, fallMode)].setToggleState (true, juce::dontSendNotification);

    for (auto* s : { &hueSlider, &lightSlider })
    {
        s->setSliderStyle (juce::Slider::LinearHorizontal);
        s->setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        s->setRange (0.0, 1.0, 0.001);
        addAndMakeVisible (*s);
    }

    hueSlider.setValue ((double) st.getProperty ("bellHue", 0.93), juce::dontSendNotification);
    lightSlider.setValue ((double) st.getProperty ("bellLight", 0.7), juce::dontSendNotification);

    hueSlider.onValueChange = [this]
    {
        proc.apvts.state.setProperty ("bellHue", hueSlider.getValue(), nullptr);
        repaint();
    };
    lightSlider.onValueChange = [this]
    {
        proc.apvts.state.setProperty ("bellLight", lightSlider.getValue(), nullptr);
        repaint();
    };

    showTab (0);
}

void SettingsPanel::refreshTexts()
{
    tabButtons[0].setButtonText (tr ("MAIN"));
    tabButtons[1].setButtonText (tr ("BELLS"));
    shaperToggle.setButtonText (tr ("WAVESHAPER"));
    bellsToggle.setButtonText (tr ("BELLS"));
    secretToggle.setButtonText (tr ("SECRET"));
    modeButtons[0].setButtonText (tr ("ORIGINAL"));
    modeButtons[1].setButtonText (tr ("SINGLE"));
    fallButtons[0].setButtonText (tr ("OFF"));
    fallButtons[1].setButtonText (tr ("PETALS"));
    fallButtons[2].setButtonText (tr ("SNOW"));
    repaint();
}

void SettingsPanel::setSecretMode (bool on)
{
    secretShown = on;
    secretToggle.setToggleState (true, juce::dontSendNotification);
    showTab (tab);
    refreshTexts();
}

void SettingsPanel::showTab (int t)
{
    tab = t;
    shaperToggle.setVisible (t == 0);
    bellsToggle.setVisible (t == 0);
    secretToggle.setVisible (t == 0 && secretShown);

    for (auto& m : modeButtons)
        m.setVisible (t == 1);

    for (auto& f : fallButtons)
        f.setVisible (t == 1);

    hueSlider.setVisible (t == 1);
    lightSlider.setVisible (t == 1);
    repaint();
}

void SettingsPanel::resized()
{
    const int x0 = handleW + 6, w = bodyW - 12;

    tabButtons[0].setBounds (x0, 8, w / 2 - 2, 22);
    tabButtons[1].setBounds (x0 + w / 2 + 2, 8, w / 2 - 2, 22);

    shaperToggle.setBounds (x0, 44, w, 24);
    bellsToggle.setBounds (x0, 74, w, 24);
    secretToggle.setBounds (x0, 104, w, 24);

    modeButtons[0].setBounds (x0, 44, w / 2 - 2, 22);
    modeButtons[1].setBounds (x0 + w / 2 + 2, 44, w / 2 - 2, 22);
    hueSlider.setBounds (x0, 84, w, 20);
    lightSlider.setBounds (x0, 120, w, 20);

    const int fw = (w - 8) / 3;
    for (int i = 0; i < 3; ++i)
        fallButtons[i].setBounds (x0 + i * (fw + 4), 160, fw, 22);
}

void SettingsPanel::mouseDown (const juce::MouseEvent& e)
{
    if (e.x < handleW && onToggle)
        onToggle();
}

void SettingsPanel::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();

    g.setGradientFill (juce::ColourGradient (panelTop.withAlpha (0.85f), 0.f, 0.f, panelBottom.withAlpha (0.85f), 0.f, b.getBottom(), false));
    g.fillRect (b);
    g.setColour (edgeLight.withAlpha (0.75f));
    g.drawRect (b.reduced (0.5f), 1.f);

    // ручка-язычок слева
    const juce::Rectangle<float> handle (0.f, 0.f, (float) handleW, b.getHeight());
    g.setColour (navyDark.withAlpha (0.8f));
    g.fillRect (handle);
    g.setColour (edgeLight.withAlpha (0.5f));
    g.drawVerticalLine (handleW, 0.f, b.getBottom());

    juce::Path arrow;
    const float cy = 48.f;
    if (open) arrow.addTriangle (9.f, cy - 8.f, 9.f, cy + 8.f, 19.f, cy);
    else      arrow.addTriangle (19.f, cy - 8.f, 19.f, cy + 8.f, 9.f, cy);
    g.setColour (arcVal);
    g.fillPath (arrow);

    g.setColour (edgeLight.withAlpha (0.9f));
    g.setFont (vnFont (12.f));
    g.drawText ("S", 0, 80, handleW, 14, juce::Justification::centred);
    g.drawText ("E", 0, 94, handleW, 14, juce::Justification::centred);
    g.drawText ("T", 0, 108, handleW, 14, juce::Justification::centred);

    const int x0 = handleW + 6, w = bodyW - 12;

    if (tab == 0)
    {
        g.setFont (vnFont (12.f));
        g.setColour (edgeLight.withAlpha (0.85f));
        g.drawFittedText (tr ("MAIN_HINT") == "MAIN_HINT"
                              ? juce::String ("Waveshaper off: DRIVE works as a plain soft saturator.\nBells are off at every start.")
                              : tr ("MAIN_HINT"),
                          x0, secretShown ? 136 : 112, w, 70, juce::Justification::topLeft, 6);
    }
    else
    {
        const auto& st = proc.apvts.state;
        const bool single = (bool) st.getProperty ("bellSingle", false);
        const float hue = (float) (double) st.getProperty ("bellHue", 0.93);
        const float light = (float) (double) st.getProperty ("bellLight", 0.7);

        g.setFont (vnFont (12.f));
        g.setColour (edgeLight);
        g.drawText (tr ("HUE"), x0, 70, w, 14, juce::Justification::centredLeft);
        g.drawText (tr ("LIGHT"), x0, 106, w, 14, juce::Justification::centredLeft);
        g.drawText (tr ("FALLING"), x0, 144, w, 14, juce::Justification::centredLeft);

        const juce::Rectangle<float> sw ((float) x0, 190.f, (float) w, 26.f);
        const juce::Colour glass = single ? bellGlassColour (hue, light) : juce::Colour (0xffbfe6ff);
        const juce::Colour paper = single ? bellPaperColour (hue, light) : juce::Colour (0xfff7f1ff);

        g.setColour (navyDark.withAlpha (0.9f));
        g.fillRect (sw);
        g.setColour (glass);
        g.fillEllipse (sw.getX() + 10.f, sw.getY() + 5.f, 22.f, 16.f);
        g.setColour (paper);
        g.fillRect (sw.getX() + 38.f, sw.getY() + 4.f, 10.f, 18.f);
        g.setColour (edgeLight.withAlpha (0.7f));
        g.drawRect (sw, 1.f);
        g.setColour (single ? textCream : edgeLight.withAlpha (0.8f));
        g.drawText (single ? tr ("SINGLE COLOUR") : tr ("ORIGINAL COLOURS"), (int) sw.getX() + 56, (int) sw.getY(), w - 58, 26,
                    juce::Justification::centredLeft);
    }
}

//==============================================================================
FallLayer::FallLayer()
{
    // спрайты лепестков (3 оттенка): рисуем один раз
    const juce::uint32 tips[6]  = { 0xfffff1f5, 0xffffe3ec, 0xfffbd7e3, 0xffe45a5a, 0xffd44848, 0xffe86a6a };
    const juce::uint32 bases[6] = { 0xfff2a2bd, 0xffec8fb0, 0xfff5b5c8, 0xff8a0f14, 0xff7a0b10, 0xff9c1419 };

    for (int i = 0; i < 6; ++i)
    {
        petals[i] = juce::Image (juce::Image::ARGB, 48, 44, true);
        juce::Graphics g (petals[i]);
        g.addTransform (juce::AffineTransform::translation (24.f, 40.f));

        const auto p = makePetal (34.f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (tips[i]), 0.f, -34.f, juce::Colour (bases[i]), 0.f, 0.f, false));
        g.fillPath (p);
        g.setColour (juce::Colour (0xffd9739a).withAlpha (0.55f));
        g.strokePath (p, juce::PathStrokeType (1.2f));
        g.setColour (juce::Colour (0xffe590ad).withAlpha (0.6f));
        g.drawLine (0.f, -4.f, 0.f, -22.f, 1.1f);
    }

    // снежинка-кристалл: шесть лучей с веточками (крупный спрайт)
    flakes[0] = juce::Image (juce::Image::ARGB, 32, 32, true);
    {
        juce::Graphics g (flakes[0]);
        g.setColour (juce::Colours::white.withAlpha (0.95f));
        for (int k = 0; k < 6; ++k)
        {
            juce::Graphics::ScopedSaveState s (g);
            g.addTransform (juce::AffineTransform::rotation ((float) k * juce::MathConstants<float>::pi / 3.f, 16.f, 16.f));
            g.drawLine (16.f, 2.f, 16.f, 30.f, 1.4f);
            g.drawLine (16.f, 8.f, 12.f, 4.f, 1.1f);
            g.drawLine (16.f, 8.f, 20.f, 4.f, 1.1f);
            g.drawLine (16.f, 13.f, 13.f, 10.f, 0.9f);
            g.drawLine (16.f, 13.f, 19.f, 10.f, 0.9f);
        }
        g.fillEllipse (14.f, 14.f, 4.f, 4.f);
    }

    // мягкая круглая снежинка
    flakes[1] = juce::Image (juce::Image::ARGB, 16, 16, true);
    {
        juce::Graphics g (flakes[1]);
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.95f), 8.f, 8.f,
                                                 juce::Colours::white.withAlpha (0.f), 16.f, 8.f, true));
        g.fillEllipse (0.f, 0.f, 16.f, 16.f);
    }
}

void FallLayer::setMode (int newMode)
{
    mode = newMode;
    setCaughtReset();
    count = juce::jmin ((int) ps.size(), mode == 1 ? 34 : mode == 2 ? 17 : 0);

    for (int i = 0; i < count; ++i)
        respawn (ps[(size_t) i], true);

    repaint();
}

void FallLayer::respawn (Particle& p, bool anywhere)
{
    const float h = (float) getHeight();

    p.baseX = rng.nextFloat() * (float) getWidth();
    p.x = p.baseX;
    p.y = anywhere ? rng.nextFloat() * h : -20.f - rng.nextFloat() * 40.f;
    p.phase = rng.nextFloat() * 6.28f;
    p.rot = rng.nextFloat() * 6.28f;
    p.flip = rng.nextFloat() * 6.28f;
    p.kind = rng.nextInt (3);

    if (mode == 1)
    {
        p.vy = 22.f + rng.nextFloat() * 26.f;
        p.swayAmp = 18.f + rng.nextFloat() * 28.f;
        p.swayFreq = 0.5f + rng.nextFloat() * 0.8f;
        p.spin = (rng.nextFloat() - 0.5f) * 2.2f;
        p.flipSpeed = 1.5f + rng.nextFloat() * 2.f;
        p.size = 0.42f + rng.nextFloat() * 0.3f;
    }
    else
    {
        p.vy = 18.f + rng.nextFloat() * 32.f;
        p.swayAmp = 6.f + rng.nextFloat() * 12.f;
        p.swayFreq = 0.6f + rng.nextFloat() * 1.0f;
        p.spin = (rng.nextFloat() - 0.5f) * 1.2f;
        p.flipSpeed = 0.f;
        p.size = 0.45f + rng.nextFloat() * 0.45f;
        p.kind = rng.nextFloat() < 0.6f ? 0 : 1;   // 0 кристалл, 1 мягкая точка
    }
}

juce::Rectangle<int> FallLayer::boundsOf (const Particle& p) const
{
    const int r = mode == 1 ? 32 : 20;
    return { (int) p.x - r, (int) p.y - r, 2 * r, 2 * r };
}

void FallLayer::step (float dt)
{
    if (mode == 0)
        return;

    time += dt;
    const float wind = 10.f * std::sin (time * 0.35f);
    const float h = (float) getHeight();

    for (int i = 0; i < count; ++i)
    {
        auto& p = ps[(size_t) i];
        const auto oldBounds = boundsOf (p);

        p.y += p.vy * dt;
        p.baseX += (wind + p.swayAmp * 0.25f) * dt * 0.5f;
        p.x = p.baseX + std::sin (time * p.swayFreq + p.phase) * p.swayAmp;
        p.rot += p.spin * dt;
        p.flip += p.flipSpeed * dt;

        if (p.y > h + 4.f || p.x < -30.f || p.x > (float) getWidth() + 30.f)
            respawn (p, false);

        repaint (oldBounds.getUnion (boundsOf (p)));
    }

    // полёт пойманных лепестков в ячейки
    for (size_t i = 0; i < flies.size();)
    {
        auto& f = flies[i];
        const auto before = juce::Rectangle<int> ((int) f.sx - 40, (int) f.sy - 80, 80, 120).getUnion (juce::Rectangle<int> ((int) f.tx - 40, (int) f.ty - 80, 80, 120));
        f.t += dt / 0.6f;
        f.rot += dt * 6.f;

        if (f.t >= 1.f)
        {
            ++landed;
            flies.erase (flies.begin() + (long) i);
            repaint (before);

            if (landed >= 4)
            {
                const auto cb = onFull;
                setCaughtReset();
                repaint();
                if (cb)
                    cb();
                return;
            }
            continue;
        }

        repaint (before);
        ++i;
    }

    for (size_t i = 0; i < bursts.size();)
    {
        auto& b = bursts[i];
        repaint ((int) b.x - 50, (int) b.y - 50, 100, 100);
        b.t += dt / 0.45f;
        if (b.t >= 1.f)
            bursts.erase (bursts.begin() + (long) i);
        else
            ++i;
    }
}

int FallLayer::findPetal (int x, int y) const
{
    if (mode != 1 || caught >= 4)
        return -1;

    const float h = (float) getHeight();

    for (int i = count - 1; i >= 0; --i)
    {
        const auto& p = ps[(size_t) i];
        if (p.y < 0.f || p.y > h - 30.f)
            continue;

        const float r = 34.f * p.size * 0.6f + 6.f;
        const float dx = p.x - (float) x, dy = p.y - (float) y;
        if (dx * dx + dy * dy < r * r)
            return i;
    }

    return -1;
}

void FallLayer::mouseDown (const juce::MouseEvent& ev)
{
    const int i = findPetal (ev.x, ev.y);
    if (i < 0)
        return;

    auto& p = ps[(size_t) i];
    const auto old = boundsOf (p);

    // вспышка на месте и полёт в свободную ячейку
    bursts.push_back ({ p.x, p.y, 0.f });
    const auto target = slotCentre (caught);
    flies.push_back ({ p.x, p.y, p.x, p.y, 0.f, p.rot, p.size, p.kind });
    flies.back().tx = target.x;
    flies.back().ty = target.y;
    ++caught;

    respawn (p, false);
    repaint (old);
}

void FallLayer::setCaughtReset()
{
    caught = landed = 0;
    flies.clear();
    bursts.clear();
}

void FallLayer::paint (juce::Graphics& g)
{
    if (mode == 0)
        return;

    const float h = (float) getHeight();

    for (int i = 0; i < count; ++i)
    {
        const auto& p = ps[(size_t) i];

        // у границы диалогового окна частицы тают
        const float alpha = juce::jlimit (0.f, 1.f, (h - p.y) / 50.f) * juce::jlimit (0.f, 1.f, (p.y + 10.f) / 30.f);
        if (alpha <= 0.01f)
            continue;

        g.setOpacity (alpha);

        if (mode == 1)
        {
            const auto& img = petals[p.kind + (blood ? 3 : 0)];
            const float sx = p.size * (0.35f + 0.65f * std::abs (std::cos (p.flip)));
            g.drawImageTransformed (img, juce::AffineTransform::translation (-24.f, -22.f)
                                              .scaled (sx, p.size)
                                              .rotated (p.rot)
                                              .translated (p.x, p.y));
        }
        else if (p.kind == 0)
        {
            g.drawImageTransformed (flakes[0], juce::AffineTransform::translation (-16.f, -16.f)
                                                   .scaled (p.size)
                                                   .rotated (p.rot)
                                                   .translated (p.x, p.y));
        }
        else
        {
            g.drawImageTransformed (flakes[1], juce::AffineTransform::translation (-8.f, -8.f)
                                                   .scaled (p.size)
                                                   .translated (p.x, p.y));
        }
    }

    // ячейки пойманных лепестков и эффекты подбора (только режим лепестков)
    if (mode == 1 && (caught > 0 || ! flies.empty()))
    {
        for (int i = 0; i < 4; ++i)
        {
            const auto c = slotCentre (i);
            g.setOpacity (1.f);

            if (i < landed)
            {
                g.drawImageTransformed (petals[0], juce::AffineTransform::translation (-24.f, -22.f)
                                                       .scaled (0.42f).translated (c.x, c.y));
            }
            else
            {
                g.setColour (juce::Colours::white.withAlpha (0.35f));
                g.drawEllipse (c.x - 9.f, c.y - 9.f, 18.f, 18.f, 1.2f);
            }
        }
    }

    for (const auto& f : flies)
    {
        const float t = f.t, e = t * t * (3.f - 2.f * t);
        const float x = f.sx + (f.tx - f.sx) * e;
        const float y = f.sy + (f.ty - f.sy) * e - std::sin (3.14159265f * t) * 50.f;
        g.setOpacity (1.f);
        g.drawImageTransformed (petals[f.kind], juce::AffineTransform::translation (-24.f, -22.f)
                                                    .scaled (f.size * (1.f - 0.35f * t))
                                                    .rotated (f.rot)
                                                    .translated (x, y));
    }

    for (const auto& b : bursts)
    {
        const float r = 8.f + b.t * 34.f, a = 1.f - b.t;
        g.setColour (juce::Colour (0xffffd6e4).withAlpha (a * 0.8f));
        g.drawEllipse (b.x - r, b.y - r, 2.f * r, 2.f * r, 1.6f);

        for (int k = 0; k < 6; ++k)
        {
            const float ang = (float) k * juce::MathConstants<float>::pi / 3.f + 0.4f;
            const float rr = r * 0.9f;
            g.setColour (juce::Colours::white.withAlpha (a));
            g.fillEllipse (b.x + std::cos (ang) * rr - 2.f, b.y + std::sin (ang) * rr - 2.f, 4.f * a + 1.f, 4.f * a + 1.f);
        }
    }
}

//==============================================================================
SecretQuiz::SecretQuiz()
{
    for (int i = 0; i < 6; ++i)
    {
        buttons[i].onClick = [this, i] { answer (i); };
        addChildComponent (buttons[i]);
    }
}

void SecretQuiz::start()
{
    stage = 0;
    girl = -1;
    showStage();
    setVisible (true);
    toFront (false);
}

void SecretQuiz::showStage()
{
    for (auto& b : buttons)
        b.setVisible (false);

    if (stage == 2)
        setBounds ((windowW - 340) / 2, (windowH - 170) / 2, 340, 170);
    else
        setBounds (14, 150, 340, 236);

    if (stage == 2)
    {
        buttons[0].setButtonText ("Yes");
        buttons[0].setBounds (30, 100, 130, 36);
        buttons[1].setButtonText ("No");
        buttons[1].setBounds (180, 100, 130, 36);
        buttons[0].setVisible (true);
        buttons[1].setVisible (true);
    }
    else if (stage == 0)
    {
        for (int i = 0; i < 6; ++i)
        {
            buttons[i].setButtonText (quizGirls[i].name);
            buttons[i].setBounds (12 + (i % 2) * 164, 84 + (i / 2) * 44, 152, 34);
            buttons[i].setVisible (true);
        }
    }
    else
    {
        const auto& q = quizGirls[girl].q[question];
        for (int i = 0; i < 3; ++i)
        {
            buttons[i].setButtonText (q.a[i]);
            buttons[i].setBounds (12, 90 + i * 44, 316, 34);
            buttons[i].setVisible (true);
        }
    }

    repaint();
}

void SecretQuiz::answer (int index)
{
    if (stage == 2)
    {
        if (index == 0) { if (onYes) onYes(); }
        else            { if (onNo)  onNo(); }
        return;
    }

    if (stage == 0)
    {
        girl = juce::jlimit (0, 5, index);
        question = rng.nextInt (2);   // случайный из двух вопросов
        stage = 1;
        showStage();
        return;
    }

    const bool right = (index == quizGirls[girl].q[question].correct);

    if (right) { stage = 2; showStage(); }   // подтверждение: "Вы уверены?"
    else       { if (onNo) onNo(); }
}

void SecretQuiz::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();

    g.setGradientFill (juce::ColourGradient (panelTop.withAlpha (0.94f), 0.f, 0.f, panelBottom.withAlpha (0.94f), 0.f, b.getBottom(), false));
    g.fillRect (b);
    g.setColour (edgeLight.withAlpha (0.8f));
    g.drawRect (b.reduced (0.5f), 1.f);
    g.setColour (edgeLight.withAlpha (0.2f));
    g.drawRect (b.reduced (4.5f), 1.f);

    g.setColour (textCream);
    g.setFont (vnFont (stage == 2 ? 18.f : 14.f));

    if (stage == 2)
    {
        g.drawFittedText (juce::String::fromUTF8 ("本当によろしいですか？"), 12, 20, getWidth() - 24, 60,
                          juce::Justification::centred, 2);
        return;
    }

    const char* q = stage == 0 ? quizFirstQuestion : quizGirls[girl].q[question].q;
    g.drawFittedText (juce::String (q), 16, 14, getWidth() - 32, 62, juce::Justification::topLeft, 3);
}

//==============================================================================
MiniScope::MiniScope (SoraKickAudioProcessor& p) : proc (p)
{
    setInterceptsMouseClicks (false, false);
    setOpaque (true);
}

void MiniScope::setRunning (bool on)
{
    running = on;
    updateTimer();
    repaint();
}

void MiniScope::updateTimer()
{
    if (running && isShowing())
    {
        if (! isTimerRunning())
            startTimerHz (30);
    }
    else
        stopTimer();
}

void MiniScope::timerCallback()
{
    proc.readScope (wave.data(), 1024);

    // спектр: последние 512 отсчётов с окном Ханна
    std::fill (fftData.begin(), fftData.end(), 0.f);
    for (int i = 0; i < 512; ++i)
    {
        const float wnd = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) i / 511.f);
        fftData[(size_t) i] = wave[(size_t) (512 + i)] * wnd;
    }
    fft.performFrequencyOnlyForwardTransform (fftData.data());

    for (int i = 0; i < 32; ++i)
    {
        const int lo = juce::jmax (1, (int) std::pow (150.f, (float) i / 32.f));
        const int hi = juce::jmax (lo + 1, (int) std::pow (150.f, (float) (i + 1) / 32.f) + 1);
        float m = 0.f;
        for (int k = lo; k < hi && k < 256; ++k)
            m = juce::jmax (m, fftData[(size_t) k]);

        const float db = 20.f * std::log10 (m / 128.f + 1.0e-5f);
        const float v = juce::jlimit (0.f, 1.f, (db + 60.f) / 60.f);

        bars[(size_t) i] = juce::jmax (v, bars[(size_t) i] * 0.86f);
        peaks[(size_t) i] = juce::jmax (bars[(size_t) i], peaks[(size_t) i] - 0.012f);
    }

    repaint();
}

void MiniScope::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2a0407), 0.f, 0.f, juce::Colour (0xff0c0102), 0.f, b.getBottom(), false));
    g.fillRect (b);

    const auto area = b.reduced (6.f);
    const juce::Colour hot (0xffff3a3a), dark (0xff7a0a12);

    if (! running)
    {
        drawPixelText (g, "made by xizepi for his friends<3",
                       { (getWidth() - 300) / 2, (getHeight() - 36) / 2, 300, 40 }, 1.5f);
    }
    else if (style == 0)   // BARS
    {
        const float bw = area.getWidth() / 32.f;
        for (int i = 0; i < 32; ++i)
        {
            const float h = bars[(size_t) i] * area.getHeight();
            const juce::Rectangle<float> r (area.getX() + (float) i * bw + 1.f, area.getBottom() - h, bw - 2.f, h);
            g.setGradientFill (juce::ColourGradient (hot, 0.f, area.getY(), dark, 0.f, area.getBottom(), false));
            g.fillRect (r);
            g.setColour (juce::Colours::white.withAlpha (0.8f));
            g.fillRect (area.getX() + (float) i * bw + 1.f, area.getBottom() - peaks[(size_t) i] * area.getHeight() - 2.f, bw - 2.f, 2.f);
        }
    }
    else if (style == 1)   // WAVE
    {
        int start = 0;
        for (int i = 1; i < 480; ++i)
            if (wave[(size_t) i - 1] < 0.f && wave[(size_t) i] >= 0.f) { start = i; break; }

        juce::Path p;
        for (int i = 0; i < 512; ++i)
        {
            const float x = area.getX() + area.getWidth() * (float) i / 511.f;
            const float y = area.getCentreY() - juce::jlimit (-1.f, 1.f, wave[(size_t) (start + i)]) * area.getHeight() * 0.48f;
            if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
        }

        g.setColour (hot.withAlpha (0.25f));
        g.strokePath (p, juce::PathStrokeType (4.f));
        g.setColour (hot);
        g.strokePath (p, juce::PathStrokeType (1.4f));
        g.setColour (edgeLight.withAlpha (0.25f));
        g.drawHorizontalLine ((int) area.getCentreY(), area.getX(), area.getRight());
    }
    else if (style == 2)   // MIRR: зеркальные столбики от центра в стороны и вверх-вниз
    {
        const float bw = area.getWidth() * 0.5f / 32.f;
        const float cx = area.getCentreX(), cy = area.getCentreY();
        for (int i = 0; i < 32; ++i)
        {
            const float v = bars[(size_t) i];
            const float h = juce::jmax (2.f, v * area.getHeight());
            g.setColour (dark.interpolatedWith (hot, v));
            g.fillRect (cx + (float) i * bw + 1.f, cy - h * 0.5f, bw - 2.f, h);
            g.fillRect (cx - (float) (i + 1) * bw + 1.f, cy - h * 0.5f, bw - 2.f, h);
        }
        g.setColour (edgeLight.withAlpha (0.3f));
        g.drawHorizontalLine ((int) cy, area.getX(), area.getRight());
    }
    else                   // LED
    {
        const int cols = 16, rowsN = 10;
        const float cw = area.getWidth() / (float) cols, rh = area.getHeight() / (float) rowsN;
        for (int i = 0; i < cols; ++i)
        {
            const float v = juce::jmax (bars[(size_t) i * 2], bars[(size_t) i * 2 + 1]);
            for (int r = 0; r < rowsN; ++r)
            {
                const bool lit = v > (float) r / (float) rowsN;
                const float t = (float) r / (float) (rowsN - 1);
                g.setColour (lit ? dark.interpolatedWith (juce::Colour (0xffffb0a0), t) : juce::Colour (0xff1c0306));
                g.fillRect (area.getX() + (float) i * cw + 1.5f, area.getBottom() - (float) (r + 1) * rh + 1.5f, cw - 3.f, rh - 3.f);
            }
        }
    }

    g.setColour (edgeLight.withAlpha (0.85f));
    g.drawRect (b.reduced (0.5f), 1.f);
    g.setColour (juce::Colour (0xffb01018).withAlpha (0.6f));
    g.drawRect (b.reduced (3.5f), 1.f);
}

//==============================================================================
SoraKickAudioProcessorEditor::SoraKickAudioProcessorEditor (SoraKickAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p), waveEditor (p), scope (p), panel (p)
{
    setLookAndFeel (&laf);

    buildScenes();

    // 7 ползунков + рычажок питча = 8 основных регуляторов
    addRow ("drive",     "DRIVE");
    addRow ("layerDrop", "PITCH DROP");
    addRow ("layer",     "SUB LAYER");
    addRow ("rumble",    "RUMBLE");
    addRow ("length",    "LENGTH");
    addRow ("attack",    "PUNCH");
    addRow ("screech",   "SCREECH");

    // рычажок питча (0 = исходный тон)
    leverSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 18);
    leverSlider.setDoubleClickReturnValue (true, 0.0);
    addAndMakeVisible (leverSlider);
    leverAtt = std::make_unique<APVTS::SliderAttachment> (proc.apvts, "pitch", leverSlider);
    leverSlider.onContextMenu = [this] { showHostMenu ("pitch"); };

    // у нуля рычажок "защёлкивается", пока его тянут мышью
    leverSlider.onValueChange = [this]
    {
        if (leverSnapGuard || ! leverSlider.isMouseButtonDown())
            return;

        const double v = leverSlider.getValue();
        if (v != 0.0 && std::abs (v) < 0.3)
        {
            leverSnapGuard = true;
            leverSlider.setValue (0.0, juce::sendNotificationSync);
            leverSnapGuard = false;
        }
    };

    leverLabel.setText ("PITCH", juce::dontSendNotification);
    leverLabel.setFont (vnFont (13.f));
    leverLabel.setJustificationType (juce::Justification::centred);
    leverLabel.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (leverLabel);

    // стиль кика
    characterLabel.setText ("CHARACTER", juce::dontSendNotification);
    characterLabel.setFont (vnFont (13.f));
    characterLabel.setJustificationType (juce::Justification::centredLeft);
    characterLabel.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (characterLabel);

    characterBox.addItemList (SoraKickAudioProcessor::getCharacterNames(), 1);
    addAndMakeVisible (characterBox);
    characterAtt = std::make_unique<APVTS::ComboBoxAttachment> (proc.apvts, "character", characterBox);

    // секретный режим: тот же список, но он меняет только задний фон
    bgBox.addItemList (SoraKickAudioProcessor::getCharacterNames(), 1);
    bgBox.setSelectedId (juce::jlimit (1, bgBox.getNumItems(), (int) proc.apvts.state.getProperty ("secretScene", 0) + 1),
                         juce::dontSendNotification);
    bgBox.onChange = [this] { proc.apvts.state.setProperty ("secretScene", bgBox.getSelectedId() - 1, nullptr); };
    addChildComponent (bgBox);

    // waveshaper
    addAndMakeVisible (waveEditor);

    const char* waveNames[4] = { "LIN", "SOFT", "HARD", "FOLD" };
    for (int i = 0; i < 4; ++i)
    {
        waveButtons[i].setButtonText (waveNames[i]);
        waveButtons[i].onClick = [this, i]
        {
            proc.setWavePreset (i);
            waveEditor.repaint();
        };
        addAndMakeVisible (waveButtons[i]);
    }

    // страница 2: Sakura FX
    addKnob ("delayTime", "DELAY TIME");
    addKnob ("delayFb",   "FEEDBACK");
    addKnob ("delayMix",  "DELAY MIX");
    addKnob ("overdrive", "OVERDRIVE");
    addKnob ("fold",      "FOLD");
    addKnob ("tone",      "TONE");
    addKnob ("reverb",    "REVERB");
    addKnob ("width",     "WIDTH");

    // секретная первая страница: 8 ручек для кика и 4 для мелодии
    addKnob ("xRing",    "X_RING",    true);
    addKnob ("xStutter", "X_STUTTER", true);
    addKnob ("xShatter", "X_SHATTER", true);
    addKnob ("xWobble",  "X_WOBBLE",  true);
    addKnob ("xRectify", "X_RECTIFY", true);
    addKnob ("xScream",  "X_SCREAM",  true);
    addKnob ("xVoid",    "X_VOID",    true);
    addKnob ("xComb",    "X_COMB",    true);
    addKnob ("xDemon",   "X_DEMON",   true);
    addKnob ("xChoir",   "X_CHOIR",   true);
    addKnob ("xRewind",  "X_REWIND",  true);
    addKnob ("xDrone",   "X_DRONE",   true);

    // секретная страница 2: восемь рычажков (по 4 с каждой стороны от визуалайзера)
    addLever ("delayTime", "DELAY TIME");
    addLever ("delayFb",   "FEEDBACK");
    addLever ("delayMix",  "DELAY MIX");
    addLever ("overdrive", "OVERDRIVE");
    addLever ("fold",      "FOLD");
    addLever ("tone",      "TONE");
    addLever ("reverb",    "REVERB");
    addLever ("width",     "WIDTH");

    addChildComponent (scope);
    {
        const char* vn[4] = { "BARS", "WAVE", "MIRR", "LED" };
        for (int i = 0; i < 4; ++i)
        {
            visButtons[i].setButtonText (vn[i]);
            visButtons[i].setClickingTogglesState (true);
            visButtons[i].setRadioGroupId (2800);
            visButtons[i].onClick = [this, i] { setVisStyle (i); };
            addChildComponent (visButtons[i]);
        }

        visOnButton.setButtonText ("VIS");
        visOnButton.setClickingTogglesState (true);
        visOnButton.onClick = [this] { setVisOn (visOnButton.getToggleState()); };
        addChildComponent (visOnButton);

        visStyle = juce::jlimit (0, 3, (int) proc.apvts.state.getProperty ("visStyle", 0));
        visOn = (bool) proc.apvts.state.getProperty ("visOn", true);
        visButtons[visStyle].setToggleState (true, juce::dontSendNotification);
        visOnButton.setToggleState (visOn, juce::dontSendNotification);
        scope.setStyle (visStyle);
        scope.setRunning (visOn);
    }

    secResetButton.setButtonText ("RESET");
    secResetButton.onClick = [this]
    {
        auto reset = [this] (const char* id, bool toZero)
        {
            if (auto* prm = proc.apvts.getParameter (id))
            {
                prm->beginChangeGesture();
                prm->setValueNotifyingHost (toZero ? 0.f : prm->getDefaultValue());
                prm->endChangeGesture();
            }
        };

        if (page == 0)   // секретная страница 1: все 12 ручек в 0
        {
            for (auto* id : { "xRing", "xStutter", "xShatter", "xWobble", "xRectify", "xScream",
                              "xVoid", "xComb", "xDemon", "xChoir", "xRewind", "xDrone" })
                reset (id, true);
        }
        else             // секретная страница 2: рычажки Sakura FX в исходное положение
        {
            for (auto* id : { "delayTime", "delayFb", "delayMix", "overdrive", "fold", "tone", "reverb", "width" })
                reset (id, false);
        }
    };
    addChildComponent (secResetButton);

    secretEnterButton.setButtonText ("SECRET");
    secretEnterButton.onClick = [this] { quiz.setVisible (false); setSecret (true); };
    addChildComponent (secretEnterButton);

    syncBox.addItemList (SoraKickAudioProcessor::getSyncNames(), 1);
    addAndMakeVisible (syncBox);
    syncAtt = std::make_unique<APVTS::ComboBoxAttachment> (proc.apvts, "delaySync", syncBox);

    resetButton.setButtonText ("RESET");
    resetButton.onClick = [this]
    {
        for (auto* id : { "delayTime", "delayFb", "delayMix", "delaySync", "overdrive", "fold", "tone", "reverb", "width" })
            if (auto* prm = proc.apvts.getParameter (id))
            {
                prm->beginChangeGesture();
                prm->setValueNotifyingHost (prm->getDefaultValue());
                prm->endChangeGesture();
            }
    };
    addAndMakeVisible (resetButton);

    // переключатель страниц
    const char* pageNames[2] = { "PAGE 1", "PAGE 2" };
    for (int i = 0; i < 2; ++i)
    {
        pageButtons[i].setButtonText (pageNames[i]);
        pageButtons[i].setClickingTogglesState (true);
        pageButtons[i].setRadioGroupId (2700);
        pageButtons[i].onClick = [this, i] { setPage (i); };
        addAndMakeVisible (pageButtons[i]);
    }
    pageButtons[0].setToggleState (true, juce::dontSendNotification);

    fall.setInterceptsMouseClicks (true, false);
    fall.setBounds (0, 0, windowW, 424);
    fall.onFull = [this] { if (! secret && ! quiz.isVisible()) quiz.start(); };
    addAndMakeVisible (fall);

    // выдвижное окошко настроек (поверх всего)
    panel.onToggle = [this] { panel.open = ! panel.open; panel.repaint(); };
    addAndMakeVisible (panel);
    panel.onSecretOff = [this] { setSecret (false); };

    quiz.onNo  = [this] { quiz.setVisible (false); };   // "No": остаёмся в обычном режиме
    quiz.onYes = [this] { quiz.setVisible (false); setSecret (true); };
    addChildComponent (quiz);

    hideButton.setButtonText ("HIDE");
    hideButton.onClick = [this] { setUiHidden (true); };
    addAndMakeVisible (hideButton);

    exitButton.setButtonText ("EXIT");
    exitButton.onClick = [this] { setSecret (false); };
    addChildComponent (exitButton);


    setSize (windowW, windowH);
    setPage (0);

    if (proc.apvts.getRawParameterValue ("secretMode")->load() > 0.5f)
        setSecret (true);
    else
        updateEnterButton();

    // сразу показываем фон текущего Character без плавного перехода
    shownScene = prevScene = juce::jmax (0, characterBox.getSelectedId() - 1) % (int) scenes.size();
    fade = 1.f;

    initBells();
    startTimerHz (30);
}

SoraKickAudioProcessorEditor::~SoraKickAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void SoraKickAudioProcessorEditor::buildScenes()
{
    scenes.clear();

    // порядок поиска: sceneN.png -> background.png -> нарисованный фон
    const auto common = loadAsset ("background_png");

    for (int i = 0; i < clannad_scenes::numScenes; ++i)
    {
        auto img = loadAsset ("scene" + juce::String (i + 1) + "_png");
        if (! img.isValid())
            img = common;
        if (! img.isValid())
            img = clannad_scenes::renderScene (i, windowW, windowH);

        scenes.push_back (img);
    }

    // секретные фоны: secretN.png -> secret.png -> тёмный кроваво-красный фон
    secretScenes.clear();
    const auto secretCommon = loadAsset ("secret_png");

    juce::Image blood (juce::Image::ARGB, windowW, windowH, false);
    {
        juce::Graphics g (blood);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3a0508), 0.f, 0.f, juce::Colour (0xff0a0102), 0.f, (float) windowH, false));
        g.fillAll();
        g.setGradientFill (juce::ColourGradient (juce::Colour (0x66aa1010), windowW * 0.5f, windowH * 0.3f,
                                                 juce::Colour (0x00000000), windowW * 0.95f, windowH * 0.95f, true));
        g.fillAll();

        juce::Random rnd (7);
        for (int i = 0; i < 40; ++i)
        {
            g.setColour (juce::Colour (0x14ff4040));
            g.fillRect (rnd.nextFloat() * (float) windowW, 0.f, 1.f + rnd.nextFloat() * 3.f, (float) windowH * 0.8f);
        }
    }

    for (int i = 0; i < clannad_scenes::numScenes; ++i)
    {
        auto img = loadAsset ("secret" + juce::String (i + 1) + "_png");
        if (! img.isValid())
            img = secretCommon;
        if (! img.isValid())
            img = blood;

        secretScenes.push_back (img);
    }

    // заранее подгоняем фоны под размер окна, чтобы paint() не масштабировал большую картинку каждый раз
    auto fit = [] (const juce::Image& src)
    {
        if (src.getWidth() == windowW && src.getHeight() == windowH)
            return src;

        juce::Image out (juce::Image::RGB, windowW, windowH, true);
        juce::Graphics g (out);
        g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
        g.drawImage (src, juce::Rectangle<float> (0.f, 0.f, (float) windowW, (float) windowH),
                     juce::RectanglePlacement::centred | juce::RectanglePlacement::fillDestination);
        return out;
    };

    for (auto& s : scenes)       s = fit (s);
    for (auto& s : secretScenes) s = fit (s);
}

void SoraKickAudioProcessorEditor::addRow (const char* paramID, const char* title)
{
    auto r = std::make_unique<Row>();
    r->en = title;

    r->label.setText (title, juce::dontSendNotification);
    r->label.setFont (vnFont (13.f));
    r->label.setJustificationType (juce::Justification::centredLeft);
    r->label.setInterceptsMouseClicks (false, false);

    r->slider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 58, 20);
    r->attachment = std::make_unique<APVTS::SliderAttachment> (proc.apvts, paramID, r->slider);
    r->slider.onContextMenu = [this, id = juce::String (paramID)] { showHostMenu (id); };

    addAndMakeVisible (r->label);
    addAndMakeVisible (r->slider);
    rows.push_back (std::move (r));
}

void SoraKickAudioProcessorEditor::addKnob (const char* paramID, const char* title, bool secretSet)
{
    auto k = std::make_unique<Knob>();
    k->en = title;

    k->label.setText (title, juce::dontSendNotification);
    k->label.setFont (vnFont (13.f));
    k->label.setJustificationType (juce::Justification::centred);
    k->label.setInterceptsMouseClicks (false, false);

    k->slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    k->slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, secretSet ? 70 : 96, 18);
    k->attachment = std::make_unique<APVTS::SliderAttachment> (proc.apvts, paramID, k->slider);
    k->slider.onContextMenu = [this, id = juce::String (paramID)] { showHostMenu (id); };

    addAndMakeVisible (k->label);
    addAndMakeVisible (k->slider);
    (secretSet ? secKnobs : knobs).push_back (std::move (k));
}

void SoraKickAudioProcessorEditor::addLever (const char* paramID, const char* title)
{
    auto k = std::make_unique<Lever>();
    k->en = title;

    k->label.setText (title, juce::dontSendNotification);
    k->label.setFont (vnFont (12.f));
    k->label.setJustificationType (juce::Justification::centred);
    k->label.setInterceptsMouseClicks (false, false);

    k->slider.getProperties().set ("generic", true);
    k->slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 68, 18);
    k->attachment = std::make_unique<APVTS::SliderAttachment> (proc.apvts, paramID, k->slider);
    k->slider.onContextMenu = [this, id = juce::String (paramID)] { showHostMenu (id); };

    addChildComponent (k->label);
    addChildComponent (k->slider);
    secLevers.push_back (std::move (k));
}

namespace
{
    juce::File unlockMarker()
    {
        return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                   .getChildFile ("ClannadKI").getChildFile ("secret.unlocked");
    }
}

bool SoraKickAudioProcessorEditor::wasSecretBefore() const
{
    return (bool) proc.apvts.state.getProperty ("secretUnlocked", false) || unlockMarker().existsAsFile();
}

void SoraKickAudioProcessorEditor::rememberSecret()
{
    proc.apvts.state.setProperty ("secretUnlocked", true, nullptr);

    const auto f = unlockMarker();
    if (! f.existsAsFile())
    {
        f.getParentDirectory().createDirectory();
        f.create();
    }
}

void SoraKickAudioProcessorEditor::updateEnterButton()
{
    secretEnterButton.setVisible (! secret && wasSecretBefore());
    secretEnterButton.toFront (false);
}

void SoraKickAudioProcessorEditor::setVisStyle (int s)
{
    visStyle = s;
    proc.apvts.state.setProperty ("visStyle", s, nullptr);
    scope.setStyle (s);
}

void SoraKickAudioProcessorEditor::setVisOn (bool on)
{
    visOn = on;
    proc.apvts.state.setProperty ("visOn", on, nullptr);
    scope.setRunning (on);
}

void SoraKickAudioProcessorEditor::setPage (int newPage)
{
    page = newPage;
    const bool p1 = (page == 0);
    const bool normal1 = p1 && ! secret;   // обычная первая страница
    const bool secret1 = p1 && secret;     // секретная первая страница

    for (auto& r : rows)
    {
        r->label.setVisible (normal1);
        r->slider.setVisible (normal1);
    }

    leverLabel.setVisible (normal1);
    leverSlider.setVisible (normal1);
    characterLabel.setVisible (normal1 || secret1);
    characterBox.setVisible (normal1);
    bgBox.setVisible (secret1);
    waveEditor.setVisible (normal1);
    for (auto& b : waveButtons)
        b.setVisible (normal1);

    for (auto& k : secKnobs)
    {
        k->label.setVisible (secret1);
        k->slider.setVisible (secret1);
    }

    const bool secret2 = ! p1 && secret;

    for (auto& k : knobs)
    {
        k->label.setVisible (! p1 && ! secret);
        k->slider.setVisible (! p1 && ! secret);
    }
    syncBox.setVisible (! p1 && ! secret);
    resetButton.setVisible (! p1 && ! secret);

    for (auto& l : secLevers)
    {
        l->label.setVisible (secret2);
        l->slider.setVisible (secret2);
    }

    scope.setVisible (secret2);
    for (auto& b : visButtons)
        b.setVisible (secret2);
    visOnButton.setVisible (secret2);
    secResetButton.setVisible (secret);
    if (secret)
        secResetButton.setBounds (secret1 ? juce::Rectangle<int> (40, 650, 92, 24) : juce::Rectangle<int> (24, 664, 92, 22));
    pageButtons[page].setToggleState (true, juce::dontSendNotification);

    repaint();
}

// меню хоста для параметра (в FL Studio там есть "Create automation clip")
void SoraKickAudioProcessorEditor::showHostMenu (const juce::String& paramID)
{
    if (auto* ctx = getHostContext())
        if (auto* prm = proc.apvts.getParameter (paramID))
            if (auto menu = ctx->getContextMenuForParameter (prm))
                menu->showNativeMenu (getMouseXYRelative());
}

void SoraKickAudioProcessorEditor::setUiHidden (bool hide)
{
    if (hide == uiHidden)
        return;

    uiHidden = hide;

    if (hide)
    {
        hiddenState.clear();

        for (auto* c : getChildren())
        {
            if (c == &fall)
                continue;

            bool clicks = true, children = true;
            c->getInterceptsMouseClicks (clicks, children);
            hiddenState.emplace_back (c, clicks, children);
            c->setAlpha (0.f);
            c->setInterceptsMouseClicks (false, false);
        }
    }
    else
    {
        for (auto& s : hiddenState)
        {
            std::get<0> (s)->setAlpha (1.f);
            std::get<0> (s)->setInterceptsMouseClicks (std::get<1> (s), std::get<2> (s));
        }

        hiddenState.clear();
    }

    repaint();
}

void SoraKickAudioProcessorEditor::setSecret (bool on)
{
    // каждый режим помнит свою страницу
    (secret ? pageSecret : pageNormal) = page;
    page = on ? pageSecret : pageNormal;

    secret = on;
    secretFlag() = on;

    if (auto* prm = proc.apvts.getParameter ("secretMode"))
        prm->setValueNotifyingHost (on ? 1.f : 0.f);

    // при переходе в секретный режим встроенный waveshaper выключается, а при выходе возвращается, если был включён
    if (auto* prm = proc.apvts.getParameter ("shaperOn"))
    {
        if (on)
        {
            if (prm->getValue() > 0.5f)
            {
                proc.apvts.state.setProperty ("shaperWasOn", true, nullptr);
                prm->setValueNotifyingHost (0.f);
            }
        }
        else if ((bool) proc.apvts.state.getProperty ("shaperWasOn", false))
        {
            proc.apvts.state.setProperty ("shaperWasOn", false, nullptr);
            prm->setValueNotifyingHost (1.f);
        }
    }

    setPalette (on);
    laf.applyPalette();
    sendLookAndFeelChange();

    // в секретном режиме боковая панель пропадает, а выход всегда справа сверху
    panel.open = false;
    panelPos = 0.f;
    placePanel();
    panel.setVisible (! on);
    exitButton.setVisible (on);
    exitButton.toFront (false);

    if (on)
        rememberSecret();
    updateEnterButton();

    fall.setBlood (on);
    quiz.setVisible (false);

    // текущая реплика на другом языке
    if (lastPhrase >= 0)
    {
        phraseSpeaker = tr (phrases[lastPhrase].speaker);
        phraseText = on ? juce::String::fromUTF8 (phrasesJa[lastPhrase]) : juce::String (phrases[lastPhrase].text);
        shownChars = phraseText.length();
    }

    refreshTexts();
    resized();
    setPage (page);

    // фон нового режима без плавного перехода
    shownScene = prevScene = juce::jmax (0, (on ? bgBox : characterBox).getSelectedId() - 1) % (int) scenes.size();
    fade = 1.f;
    proc.updateHostDisplay (juce::AudioProcessor::ChangeDetails().withParameterInfoChanged (true));
    repaint();
}

void SoraKickAudioProcessorEditor::refreshTexts()
{
    for (auto& r : rows)
        r->label.setText (tr (r->en), juce::dontSendNotification);

    for (auto& k : knobs)
        k->label.setText (tr (k->en), juce::dontSendNotification);

    for (auto& k : secKnobs)
        k->label.setText (tr (k->en), juce::dontSendNotification);

    for (auto& l : secLevers)
        l->label.setText (tr (l->en), juce::dontSendNotification);

    {
        const char* vn[4] = { "BARS", "WAVE", "MIRR", "LED" };
        for (int i = 0; i < 4; ++i)
            visButtons[i].setButtonText (tr (vn[i]));
        visOnButton.setButtonText (tr ("VIS"));
        secResetButton.setButtonText (tr ("RESET"));
        secretEnterButton.setButtonText (tr ("SECRET"));
    }

    exitButton.setButtonText (tr ("EXIT"));
    hideButton.setButtonText (tr ("HIDE"));

    leverLabel.setText (tr ("PITCH"), juce::dontSendNotification);
    characterLabel.setText (tr ("CHARACTER"), juce::dontSendNotification);

    const char* waveNames[4] = { "LIN", "SOFT", "HARD", "FOLD" };
    for (int i = 0; i < 4; ++i)
        waveButtons[i].setButtonText (tr (waveNames[i]));

    pageButtons[0].setButtonText (tr ("PAGE 1"));
    pageButtons[1].setButtonText (tr ("PAGE 2"));
    resetButton.setButtonText (tr ("RESET"));

    // списки: меняем подписи пунктов и обновляем показанный текст
    const auto chars = SoraKickAudioProcessor::getCharacterNames();
    for (int i = 0; i < chars.size(); ++i)
        characterBox.changeItemText (i + 1, tr (chars[i].toRawUTF8()));

    const auto syncs = SoraKickAudioProcessor::getSyncNames();
    for (int i = 0; i < syncs.size(); ++i)
        syncBox.changeItemText (i + 1, tr (syncs[i].toRawUTF8()));

    for (int i = 0; i < chars.size(); ++i)
        bgBox.changeItemText (i + 1, tr (chars[i].toRawUTF8()));

    for (auto* box : { &characterBox, &bgBox, &syncBox })
    {
        const int id = box->getSelectedId();
        box->setSelectedId (0, juce::dontSendNotification);
        box->setSelectedId (id, juce::dontSendNotification);
    }

    panel.refreshTexts();
}

void SoraKickAudioProcessorEditor::placePanel()
{
    panel.setBounds (windowW - SettingsPanel::handleW - (int) std::round (panelPos * (float) SettingsPanel::bodyW),
                     126, SettingsPanel::panelW, SettingsPanel::panelH);
}

void SoraKickAudioProcessorEditor::applyBellColours()
{
    const auto& st = proc.apvts.state;
    bellsOn = ! secret && (bool) st.getProperty ("bellsOn", false);
    const bool single = (bool) st.getProperty ("bellSingle", false);
    const float hue = (float) (double) st.getProperty ("bellHue", 0.93);
    const float light = (float) (double) st.getProperty ("bellLight", 0.7);

    for (auto& b : bells)
    {
        b.glass = single ? bellGlassColour (hue, light) : b.glass0;
        b.paper = single ? bellPaperColour (hue, light) : b.paper0;
    }
}

//==============================================================================
// Японские колокольчики (фурин) на верхней границе окна. Каждый это маятник
// (угол + угловая скорость) с лёгким ветром; под ним бумажная полоска на втором
// "отстающем" маятнике. Курсор толкает колокольчик при проходе и слегка
// отодвигает его, когда стоит рядом; клик даёт сильный толчок.
namespace
{
    constexpr float bellAnchorY = 4.f;
}

juce::Point<float> SoraKickAudioProcessorEditor::bellCentre (const Bell& b, float distance) const
{
    // положительный угол в juce::AffineTransform::rotation поворачивает по часовой стрелке
    return { b.anchorX - std::sin (b.angle) * distance, bellAnchorY + std::cos (b.angle) * distance };
}

void SoraKickAudioProcessorEditor::initBells()
{
    const float xs[7]   = { 250.f, 370.f, 490.f, 610.f, 730.f, 850.f, 950.f };
    const float lens[7] = {  55.f,  85.f,  48.f,  95.f,  62.f,  78.f,  52.f };
    const juce::uint32 glass[7] = { 0xffcfe9ee, 0xffe8d4dc, 0xffd9e8d0, 0xffdcd5ea, 0xffcfe4f0, 0xffecdfc0, 0xffd2ecee };
    const juce::uint32 paper[7] = { 0xfff4efe4, 0xfff0cfd9, 0xfff2e8c8, 0xffd6e2ef, 0xffe2eed8, 0xfff4efe4, 0xfff0cfd9 };

    for (size_t i = 0; i < bells.size(); ++i)
    {
        auto& b = bells[i];
        b.anchorX = xs[i];
        b.len = lens[i];
        b.omega = juce::MathConstants<float>::twoPi * 0.55f * std::sqrt (70.f / b.len);
        b.phase = (float) i * 1.7f;
        b.angle = 0.05f * std::sin (b.phase);
        b.tail = b.angle;
        b.glass0 = b.glass = juce::Colour (glass[i]);
        b.paper0 = b.paper = juce::Colour (paper[i]);
    }
}

void SoraKickAudioProcessorEditor::updateBells()
{
    const float tick = 1.f / 30.f;
    const float dt = tick * 0.5f;
    windTime += tick;

    const bool inside = isMouseOver (true);
    const auto mouse = inside ? getMouseXYRelative().toFloat() : juce::Point<float>();
    juce::Point<float> mv;

    if (inside && haveLastMouse)
        mv = (mouse - lastMouse) * 30.f;   // пикселей в секунду

    haveLastMouse = inside;
    lastMouse = mouse;

    for (auto& b : bells)
    {
        if (inside)
        {
            const auto c = bellCentre (b, b.len + 12.f);
            const float dx = c.x - mouse.x;
            const float dist = c.getDistanceFrom (mouse);
            const float reach = 38.f;

            if (dist < reach)
            {
                const float s = 1.f - dist / reach;
                b.vel += -mv.x * s * 0.004f * (60.f / b.len);   // проход курсора раскачивает
                b.vel += (dx > 0.f ? -1.f : 1.f) * s * 0.15f;     // и слегка отодвигает
                b.ring = juce::jmax (b.ring, juce::jmin (1.f, std::abs (mv.x) / 500.f * s * 3.f));
            }
        }

        b.vel = juce::jlimit (-7.f, 7.f, b.vel);

        for (int step = 0; step < 2; ++step)
        {
            const float wind = 1.2f * std::sin (windTime * 0.8f + b.phase) * std::sin (windTime * 0.31f + b.phase * 1.7f);
            const float acc = -b.omega * b.omega * std::sin (b.angle) - 0.5f * b.vel + wind;
            b.vel += acc * dt;
            b.angle = juce::jlimit (-1.3f, 1.3f, b.angle + b.vel * dt);

            const float wt = b.omega * 1.6f;
            const float tacc = -wt * wt * (b.tail - b.angle) - 2.5f * b.tailVel;
            b.tailVel += tacc * dt;
            b.tail += b.tailVel * dt;
        }

        b.ring *= 0.93f;
    }
}

void SoraKickAudioProcessorEditor::renderBellSprite (juce::Graphics& g, const Bell& b, int part) const
{
    const float ax = b.anchorX, ay = bellAnchorY, by = ay + b.len;

    if (part == 0)
    {
        juce::Graphics::ScopedSaveState state (g);
        g.addTransform (juce::AffineTransform::rotation (b.angle, ax, ay));

        // нить: скрученный хлопок (две пряди) с узелком
        g.setColour (juce::Colour (0xffe9e1cf));
        g.drawLine (ax - 0.4f, ay, ax - 0.4f, by, 1.1f);
        g.setColour (juce::Colour (0xffa99d86));
        g.drawLine (ax + 0.5f, ay, ax + 0.5f, by, 0.9f);
        g.setColour (juce::Colour (0xffcfc3aa));
        for (float ty = ay + 6.f; ty < by - 4.f; ty += 7.f)
            g.drawLine (ax - 1.f, ty, ax + 1.f, ty + 2.f, 0.8f);

        // латунный колпачок
        {
            juce::Path cap;
            cap.addRoundedRectangle (ax - 5.f, by - 1.f, 10.f, 5.f, 1.8f);
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xffe6d39a), ax - 5.f, 0.f,
                                                     juce::Colour (0xff7d6a3c), ax + 5.f, 0.f, false));
            g.fillPath (cap);
            g.setColour (juce::Colour (0xff4d4226).withAlpha (0.8f));
            g.strokePath (cap, juce::PathStrokeType (0.7f));
        }

        // стеклянный колокол: узкое плечо, расширяющийся нижний край
        juce::Path body;
        const float t0 = by + 3.f;
        body.startNewSubPath (ax - 4.f, t0);
        body.cubicTo (ax - 11.f, t0 + 2.f, ax - 12.5f, t0 + 12.f, ax - 16.f, t0 + 22.f);
        body.quadraticTo (ax, t0 + 27.f, ax + 16.f, t0 + 22.f);
        body.cubicTo (ax + 12.5f, t0 + 12.f, ax + 11.f, t0 + 2.f, ax + 4.f, t0);
        body.quadraticTo (ax, t0 - 1.f, ax - 4.f, t0);
        body.closeSubPath();

        // тень внутри (задняя стенка видна сквозь стекло)
        g.setColour (b.glass.darker (0.55f).withAlpha (0.35f));
        g.fillEllipse (ax - 15.f, t0 + 18.f, 30.f, 7.f);

        // стекло: прозрачнее в центре, плотнее по краям
        juce::ColourGradient gl (b.glass.withAlpha (0.80f), ax - 16.f, 0.f, b.glass.darker (0.35f).withAlpha (0.72f), ax + 16.f, 0.f, false);
        gl.addColour (0.35, b.glass.brighter (0.2f).withAlpha (0.30f));
        gl.addColour (0.65, b.glass.withAlpha (0.28f));
        g.setGradientFill (gl);
        g.fillPath (body);

        // роспись по стеклу: тонкая волнистая линия и точки
        g.setColour (b.glass.darker (0.7f).withAlpha (0.55f));
        {
            juce::Path wave;
            wave.startNewSubPath (ax - 12.5f, t0 + 14.f);
            wave.quadraticTo (ax - 6.f, t0 + 10.f, ax, t0 + 14.f);
            wave.quadraticTo (ax + 6.f, t0 + 18.f, ax + 12.5f, t0 + 14.f);
            g.strokePath (wave, juce::PathStrokeType (0.9f));
        }
        g.fillEllipse (ax - 5.f, t0 + 7.f, 1.6f, 1.6f);
        g.fillEllipse (ax + 3.f, t0 + 8.f, 1.6f, 1.6f);

        // контур и толстый нижний край
        g.setColour (juce::Colours::white.withAlpha (0.55f));
        g.strokePath (body, juce::PathStrokeType (0.8f));
        {
            juce::Path lip;
            lip.startNewSubPath (ax - 16.f, t0 + 22.f);
            lip.quadraticTo (ax, t0 + 27.f, ax + 16.f, t0 + 22.f);
            g.setColour (juce::Colours::white.withAlpha (0.75f));
            g.strokePath (lip, juce::PathStrokeType (2.0f));
            g.setColour (b.glass.darker (0.6f).withAlpha (0.6f));
            g.strokePath (lip, juce::PathStrokeType (0.8f));
        }

        // блики
        g.setColour (juce::Colours::white.withAlpha (0.85f));
        {
            juce::Path hl;
            hl.startNewSubPath (ax - 8.5f, t0 + 3.f);
            hl.quadraticTo (ax - 11.f, t0 + 10.f, ax - 12.f, t0 + 17.f);
            g.strokePath (hl, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        g.setColour (juce::Colours::white.withAlpha (0.3f));
        g.drawLine (ax + 8.f, t0 + 5.f, ax + 10.f, t0 + 15.f, 1.3f);

        // язычок внутри: металлический стержень и грузик, от него нить к бумажке
        g.setColour (juce::Colour (0xff6f7480).withAlpha (0.9f));
        g.drawLine (ax, t0 + 1.f, ax, t0 + 24.f, 0.9f);
        {
            juce::Path disc;
            disc.addEllipse (ax - 3.2f, t0 + 22.f, 6.4f, 3.2f);
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xffd8dce4), ax - 3.f, 0.f, juce::Colour (0xff6a707c), ax + 3.f, 0.f, false));
            g.fillPath (disc);
        }
        g.setColour (juce::Colour (0xffd9d0bb));
        g.drawLine (ax, t0 + 25.f, ax, t0 + 31.f, 0.9f);
    }

    // бумажная полоска (тандзаку) на отстающем маятнике
    if (part == 1)
    {
        const auto pivot = bellCentre (b, b.len + 3.f + 31.f);
        juce::Graphics::ScopedSaveState state (g);
        g.addTransform (juce::AffineTransform::rotation (b.tail, pivot.x, pivot.y));

        const juce::Rectangle<float> strip (pivot.x - 6.5f, pivot.y, 13.f, 40.f);

        g.setColour (juce::Colours::black.withAlpha (0.14f));
        g.fillRect (strip.translated (1.5f, 1.5f));

        g.setGradientFill (juce::ColourGradient (b.paper, 0.f, strip.getY(), b.paper.darker (0.10f), 0.f, strip.getBottom(), false));
        g.fillRect (strip);

        // волокна бумаги
        g.setColour (juce::Colours::white.withAlpha (0.35f));
        g.drawLine (strip.getX() + 3.f, strip.getY() + 2.f, strip.getX() + 3.4f, strip.getBottom() - 3.f, 0.6f);
        g.drawLine (strip.getRight() - 3.f, strip.getY() + 4.f, strip.getRight() - 3.6f, strip.getBottom() - 2.f, 0.6f);

        // тушь: вертикальные мазки, как надпись кистью
        g.setColour (juce::Colour (0xff2a2a36).withAlpha (0.7f));
        g.drawLine (pivot.x, strip.getY() + 6.f, pivot.x + 0.3f, strip.getY() + 15.f, 1.8f);
        g.drawLine (pivot.x - 1.f, strip.getY() + 19.f, pivot.x + 0.6f, strip.getY() + 23.f, 1.3f);
        g.drawLine (pivot.x + 0.2f, strip.getY() + 26.f, pivot.x - 0.2f, strip.getY() + 34.f, 1.5f);

        g.setColour (juce::Colours::black.withAlpha (0.25f));
        g.drawRect (strip, 0.6f);
        g.setColour (juce::Colour (0xffd9d0bb));
        g.fillEllipse (pivot.x - 1.2f, pivot.y + 1.5f, 2.4f, 2.4f);
    }
}

// Колокольчик рисуется быстро: готовые картинки (в 2 раза крупнее, чтобы было чётко)
// просто поворачиваются. Тяжёлая векторная отрисовка выполняется только при смене цвета.
void SoraKickAudioProcessorEditor::ensureBellSprites (Bell& b)
{
    if (b.pend.isValid() && b.spriteGlass == b.glass && b.spritePaper == b.paper)
        return;

    b.spriteGlass = b.glass;
    b.spritePaper = b.paper;

    Bell n = b;
    n.anchorX = 24.f;
    n.angle = n.tail = 0.f;
    n.ring = 0.f;

    const int S = bellSpriteScale;

    b.pend = juce::Image (juce::Image::ARGB, 48 * S, ((int) b.len + 44) * S, true);
    {
        juce::Graphics g (b.pend);
        g.addTransform (juce::AffineTransform::scale ((float) S));
        renderBellSprite (g, n, 0);
    }

    b.strip = juce::Image (juce::Image::ARGB, 20 * S, 50 * S, true);
    {
        juce::Graphics g (b.strip);
        g.addTransform (juce::AffineTransform::translation (-14.f, -(n.len + 34.f)).followedBy (juce::AffineTransform::scale ((float) S)));
        renderBellSprite (g, n, 1);
    }
}

void SoraKickAudioProcessorEditor::drawBell (juce::Graphics& g, Bell& b)
{
    ensureBellSprites (b);

    const float ax = b.anchorX, ay = bellAnchorY;
    const float inv = 1.f / (float) bellSpriteScale;

    g.drawImageTransformed (b.pend, juce::AffineTransform::scale (inv).translated (ax - 24.f, 0.f)
                                        .followedBy (juce::AffineTransform::rotation (b.angle, ax, ay)));

    const auto pivot = bellCentre (b, b.len + 34.f);
    g.drawImageTransformed (b.strip, juce::AffineTransform::scale (inv).translated (pivot.x - 10.f, pivot.y - 4.f)
                                         .followedBy (juce::AffineTransform::rotation (b.tail, pivot.x, pivot.y)));

    // крючок на границе
    g.setColour (juce::Colour (0xff9aa3b5));
    g.fillEllipse (ax - 3.f, ay - 3.f, 6.f, 6.f);
    g.setColour (juce::Colour (0xff3b4258));
    g.drawEllipse (ax - 3.f, ay - 3.f, 6.f, 6.f, 0.8f);

    // круги звона после толчка
    if (b.ring > 0.03f)
    {
        const auto c = bellCentre (b, b.len + 14.f);
        const float r = 18.f + (1.f - b.ring) * 16.f;
        g.setColour (juce::Colours::white.withAlpha (b.ring * 0.45f));
        g.drawEllipse (c.x - r, c.y - r, 2.f * r, 2.f * r, 1.f);
    }
}

//==============================================================================
void SoraKickAudioProcessorEditor::showRandomPhrase()
{
    int idx = rng.nextInt (numPhrases);
    if (numPhrases > 1)
        while (idx == lastPhrase)
            idx = rng.nextInt (numPhrases);

    lastPhrase = idx;
    phraseSpeaker = tr (phrases[idx].speaker);
    phraseText = secretFlag() ? juce::String::fromUTF8 (phrasesJa[idx]) : juce::String (phrases[idx].text);
    shownChars = 0;
    repaint();
}

// клик по фону: если реплика ещё печатается, дописываем её сразу, иначе берём новую
void SoraKickAudioProcessorEditor::mouseDown (const juce::MouseEvent& e)
{
    // клик по колокольчику: сильный толчок вместо реплики
    for (auto& bell : bells)
    {
        if (bellsOn && bellCentre (bell, bell.len + 14.f).getDistanceFrom (e.position) < 26.f)
        {
            const float side = bellCentre (bell, bell.len + 14.f).x < e.position.x ? 1.f : -1.f;
            bell.vel += side * 5.f;
            bell.ring = 1.f;
            return;
        }
    }

    // клик по картинке возвращает скрытое окно
    if (uiHidden)
    {
        setUiHidden (false);
        return;
    }

    if (shownChars < phraseText.length())
    {
        shownChars = phraseText.length();
        repaint();
        return;
    }

    showRandomPhrase();
}

// перерисовываем только небольшие области вокруг каждого колокольчика (раньше целиком полосу 800x175)
void SoraKickAudioProcessorEditor::updateBellRects()
{
    if (! bellsOn)
    {
        if (bellsDrawn)
        {
            bellsDrawn = false;
            for (auto& b : bells)
                b.lastRect = {};
            repaint (200, 0, 800, 200);
        }
        return;
    }

    bellsDrawn = true;

    for (auto& b : bells)
    {
        const float s = juce::jmax (std::abs (std::sin (b.angle)), std::abs (std::sin (b.tail)));
        const int half = (int) ((b.len + 70.f) * s) + 34;
        const juce::Rectangle<int> r ((int) b.anchorX - half, 0, half * 2, (int) b.len + 96);
        repaint (b.lastRect.isEmpty() ? r : r.getUnion (b.lastRect));
        b.lastRect = r;
    }
}

void SoraKickAudioProcessorEditor::timerCallback()
{
    // выбранный Character -> фон; при смене начинаем медленный переход (~3 секунды)
    const int idx = juce::jmax (0, (secret ? bgBox : characterBox).getSelectedId() - 1);
    const int scene = idx % (int) scenes.size();

    if (scene != shownScene)
    {
        prevScene = shownScene;
        shownScene = scene;
        fade = 0.f;
    }

    ++blinkTicks;

    applyBellColours();
    if (bellsOn)
        updateBells();

    {
        const int fm = secret ? 0 : (int) proc.apvts.state.getProperty ("fallMode", 0);
        if (fm != fallMode)
        {
            fallMode = fm;
            fall.setMode (fm);
            fall.setVisible (fm != 0);
        }
        if (fallMode != 0)
            fall.step (1.f / 30.f);
    }
    updateBellRects();

    // выдвижное окошко: плавно выезжает / задвигается
    {
        const float target = panel.open ? 1.f : 0.f;
        if (panelPos != target)
        {
            panelPos += (target - panelPos) * 0.3f;
            if (std::abs (target - panelPos) < 0.01f)
                panelPos = target;
            placePanel();
        }
    }

    // waveshaper включили или выключили (в том числе автоматизацией)
    if (proc.isShaperOn() != lastShaperOn)
    {
        lastShaperOn = proc.isShaperOn();
        waveEditor.repaint();
    }

    // волна могла измениться пресетом
    const int wv = proc.getWaveVersion();
    if (wv != shownWaveVersion)
    {
        shownWaveVersion = wv;
        waveEditor.repaint();
    }

    const bool typing = shownChars < phraseText.length();
    if (typing)
        shownChars = juce::jmin (phraseText.length(), shownChars + 2);

    if (fade < 1.f)
    {
        fade = juce::jmin (1.f, fade + 1.f / 90.f);
        repaint();
    }
    else if (typing)
    {
        repaint (16, 390, 968, 110);
    }
    else
    {
        repaint (940, 640, 40, 30);   // только мигающий треугольник
    }
}

//==============================================================================
void SoraKickAudioProcessorEditor::paint (juce::Graphics& g)
{
    // фон с плавной сменой (картинки уже подогнаны под окно)
    g.setOpacity (1.f);
    const auto& sceneSet = secret ? secretScenes : scenes;
    g.drawImageAt (sceneSet[(size_t) prevScene], 0, 0);

    if (fade > 0.f && shownScene != prevScene)
    {
        g.setOpacity (fade);
        g.drawImageAt (sceneSet[(size_t) shownScene], 0, 0);
        g.setOpacity (1.f);
    }

    // заголовок
    if (! uiHidden)
        drawPixelText (g, tr ("ClannadKI"), { 22, 14, 330, 32 }, 2.f);

    // колокольчики на верхней границе
    if (bellsOn)
        for (auto& bell : bells)
            drawBell (g, bell);

    // кнопка HIDE прячет всё, кроме картинки (как в визуальных новеллах)
    if (uiHidden)
        return;

    // диалоговое окно
    const juce::Rectangle<float> box (16.f, 424.f, 968.f, 264.f);
    g.setGradientFill (juce::ColourGradient (panelTop, 0.f, box.getY(), panelBottom, 0.f, box.getBottom(), false));
    g.fillRect (box);
    g.setColour (edgeLight.withAlpha (0.75f));
    g.drawRect (box.reduced (0.5f), 1.f);
    g.setColour (edgeLight.withAlpha (0.18f));
    g.drawRect (box.reduced (4.5f), 1.f);

    // табличка с именем говорящего
    const juce::Rectangle<float> nameplate (32.f, 394.f, 220.f, 28.f);
    g.setColour (plate);
    g.fillRect (nameplate);
    drawBevelBox (g, nameplate, edgeLight.withAlpha (0.8f), bevelDark);

    g.setColour (juce::Colours::white);
    g.setFont (vnFont (16.f));
    g.drawText (phraseSpeaker.isEmpty() ? tr ("ClannadKI") : phraseSpeaker,
                nameplate.toNearestInt(), juce::Justification::centred);

    // реплика (печатается по буквам, как в новелле)
    const juce::String shown = phraseText.isEmpty()
                                 ? tr ("( Click the scenery to hear from the girls... )")
                                 : phraseText.substring (0, shownChars);

    drawPixelText (g, shown, { 40, 437, 920, 50 }, 1.5f);

    // подпись волны
    g.setFont (vnFont (12.f));
    g.setColour (edgeLight);
    if (page == 0 && ! secret)
        g.drawText (tr ("WAVESHAPER  (draw the curve)"), 756, 492, 230, 16, juce::Justification::centredLeft);

    // мигающий треугольник "дальше" в углу окна
    juce::Path tri;
    tri.addTriangle (952.f, 648.f, 968.f, 648.f, 960.f, 660.f);
    const float blink = 0.35f + 0.5f * (0.5f + 0.5f * std::sin ((float) blinkTicks * 0.2f));
    g.setColour (juce::Colours::white.withAlpha (blink));
    g.fillPath (tri);
}

void SoraKickAudioProcessorEditor::resized()
{
    // ползунки в два столбца: 4 + 3 (+ выбор Character)
    const int rowH = 34;
    for (size_t i = 0; i < rows.size(); ++i)
    {
        const int col = i < 4 ? 0 : 1;
        const int rowInCol = (int) (i < 4 ? i : i - 4);
        const int x0 = col == 0 ? 40 : 360;
        const int y0 = 500 + rowInCol * rowH;

        rows[i]->label.setBounds (x0, y0, 92, 24);
        rows[i]->slider.setBounds (x0 + 92, y0, 210, 24);
    }

    if (secret)
    {
        characterLabel.setBounds (150, 650, 92, 24);
        characterBox.setBounds (244, 650, 200, 24);
        bgBox.setBounds (244, 650, 200, 24);
    }
    else
    {
        characterLabel.setBounds (360, 500 + 3 * rowH, 92, 24);
        characterBox.setBounds (452, 500 + 3 * rowH, 200, 24);
    }

    // рычажок питча
    leverLabel.setBounds (664, 494, 84, 18);
    leverSlider.setBounds (664, 514, 84, 150);

    // волна и кнопки форм
    waveEditor.setBounds (756, 512, 212, 108);
    for (int i = 0; i < 4; ++i)
        waveButtons[i].setBounds (756 + i * 54, 628, 50, 22);

    // страница 2: восемь крутилок-сакур в ряд
    for (size_t i = 0; i < knobs.size(); ++i)
    {
        const int kx = 40 + (int) i * 115;
        knobs[i]->label.setBounds (kx, 498, 108, 16);
        knobs[i]->slider.setBounds (kx, 516, 108, 128);
    }
    syncBox.setBounds (40, 652, 108, 22);
    resetButton.setBounds (160, 652, 88, 22);

    // секретная первая страница: 12 ручек в ряд (8 кик + 4 мелодия)
    for (size_t i = 0; i < secKnobs.size(); ++i)
    {
        const int kx = 40 + (int) i * 74 + (i >= 8 ? 16 : 0);
        secKnobs[i]->label.setBounds (kx - 2, 498, 78, 16);
        secKnobs[i]->slider.setBounds (kx, 516, 74, 128);
    }

    // секретная страница 2: слева 4 рычажка, в центре визуалайзер, справа 4 рычажка
    for (size_t i = 0; i < secLevers.size(); ++i)
    {
        const int lx = i < 4 ? 24 + (int) i * 74 : 686 + ((int) i - 4) * 74;
        secLevers[i]->label.setBounds (lx - 2, 494, 72, 16);
        secLevers[i]->slider.setBounds (lx, 512, 68, 148);
    }
    scope.setBounds (326, 496, 348, 134);
    for (int i = 0; i < 4; ++i)
        visButtons[i].setBounds (326 + i * 64, 638, 60, 24);
    visOnButton.setBounds (590, 638, 84, 24);

    secretEnterButton.setBounds (windowW - 112, 10, 100, 26);
    exitButton.setBounds (windowW - 112, 10, 100, 26);
    hideButton.setBounds (652, 398, 100, 24);

    // переключатель страниц над диалоговым окном
    pageButtons[0].setBounds (760, 398, 100, 24);
    pageButtons[1].setBounds (868, 398, 100, 24);

    placePanel();
}
