#pragma once

#include <juce_graphics/juce_graphics.h>

// Запасные нарисованные фоны (по одному на каждый пункт Character).
// Используются, если в Resources нет своих картинок.
namespace clannad_scenes
{
    // Процедурные фоны (запасной вариант): по одному на каждый пункт Character
    struct SceneDef
    {
        juce::uint32 top, mid, bottom, hillFar, hillNear, sun;
        float sunX, sunY;   // доли окна, -1 = нет
        int kind;           // 0 облака, 1 звёзды, 2 лепестки, 3 снег, 4 листья, 5 дождь, 6 крона дерева
    };

    const SceneDef sceneDefs[] =
    {
        { 0xff5fb0ea, 0xffa6d8f5, 0xffe9f6fd, 0xff9cd49a, 0xff6fba78, 0xfffff4c2, 0.78f, 0.17f, 0 },  // ясный день
        { 0xff5aa9e6, 0xff9fd3f3, 0xffe3f3fb, 0xff86c48a, 0xff5da86a, 0xfffff2b8, 0.30f, 0.12f, 6 },  // солнечная крона
        { 0xff5a4a9a, 0xffe9769a, 0xffffc78a, 0xff6b4a7a, 0xff3f2c5a, 0xffffe3a0, 0.50f, 0.33f, 0 },  // закат
        { 0xff0f1740, 0xff2b3a7a, 0xff6a78c0, 0xff1b2350, 0xff10163a, 0xfffff6d6, 0.76f, 0.16f, 1 },  // ночь
        { 0xff55606f, 0xff7d8a99, 0xffb5c0cc, 0xff5f7380, 0xff465866, 0x00000000, -1.f,  -1.f,  5 },  // дождь
        { 0xff9fd2f2, 0xffe8d4ee, 0xfffbe6ef, 0xffd5e8c2, 0xffa8d09a, 0xffffffff, 0.20f, 0.15f, 2 },  // сакура
        { 0xff3a5d9a, 0xff8f9fd0, 0xfff3c1a4, 0xff55608a, 0xff3b4470, 0xffffd9a8, 0.28f, 0.34f, 0 },  // вечер
        { 0xffe8b76a, 0xfff2d3a0, 0xfffbeed8, 0xffc98a3c, 0xff9c5f26, 0xfffff0c0, 0.80f, 0.14f, 4 },  // осень
        { 0xffb9d6ee, 0xffdcebf6, 0xfff8fbfe, 0xffeef5fb, 0xffd3e4f3, 0xffffffff, -1.f,  -1.f,  3 },  // снег
    };

    constexpr int numScenes = (int) (sizeof (sceneDefs) / sizeof (sceneDefs[0]));

    inline void drawCloud (juce::Graphics& g, float x, float y, float s, juce::Colour c)
    {
        g.setColour (c);
        g.fillEllipse (x,            y + 14.f * s, 90.f * s, 34.f * s);
        g.fillEllipse (x + 14.f * s, y,            44.f * s, 40.f * s);
        g.fillEllipse (x + 40.f * s, y + 6.f * s,  40.f * s, 34.f * s);
        g.fillRoundedRectangle (x + 8.f * s, y + 26.f * s, 80.f * s, 22.f * s, 11.f * s);
    }

    inline void fillHills (juce::Graphics& g, juce::Colour c, float w, float h,
                    float baseY, float amp, float freq, float phase)
    {
        juce::Path p;
        p.startNewSubPath (0.f, h);
        for (float x = 0.f; x <= w; x += 8.f)
            p.lineTo (x, baseY + std::sin (x * freq + phase) * amp
                              + std::sin (x * freq * 2.3f + phase * 1.7f) * amp * 0.4f);
        p.lineTo (w, h);
        p.closeSubPath();
        g.setColour (c);
        g.fillPath (p);
    }

    inline juce::Image renderScene (int index, int w, int h)
    {
        const auto& s = sceneDefs[index % numScenes];
        juce::Image img (juce::Image::ARGB, w, h, true);
        juce::Graphics g (img);

        const float fw = (float) w, fh = (float) h;
        juce::Random rng (1234 + index * 77);

        juce::ColourGradient sky (juce::Colour (s.top), 0.f, 0.f, juce::Colour (s.bottom), 0.f, fh * 0.55f, false);
        sky.addColour (0.5, juce::Colour (s.mid));
        g.setGradientFill (sky);
        g.fillAll();

        if (s.kind == 1)
        {
            for (int i = 0; i < 160; ++i)
            {
                g.setColour (juce::Colours::white.withAlpha (0.3f + 0.7f * rng.nextFloat()));
                const float r = 0.8f + rng.nextFloat() * 1.8f;
                g.fillEllipse (rng.nextFloat() * fw, rng.nextFloat() * fh * 0.5f, r, r);
            }
        }

        if (s.sunX >= 0.f)
        {
            const float sx = s.sunX * fw, sy = s.sunY * fh;
            juce::ColourGradient glow (juce::Colour (s.sun).withAlpha (0.55f), sx, sy,
                                       juce::Colour (s.sun).withAlpha (0.f), sx + 170.f, sy, true);
            g.setGradientFill (glow);
            g.fillEllipse (sx - 170.f, sy - 170.f, 340.f, 340.f);
            g.setColour (juce::Colour (s.sun));
            g.fillEllipse (sx - 34.f, sy - 34.f, 68.f, 68.f);
        }

        if (s.kind == 0 || s.kind == 2 || s.kind == 3)
        {
            const auto c = juce::Colours::white.withAlpha (0.5f);
            drawCloud (g, fw * 0.05f, 40.f,  1.4f, c);
            drawCloud (g, fw * 0.38f, 110.f, 1.0f, c);
            drawCloud (g, fw * 0.62f, 30.f,  1.2f, c);
            drawCloud (g, fw * 0.88f, 120.f, 0.9f, c);
        }
        if (s.kind == 5)
        {
            const auto c = juce::Colour (0xff48515f).withAlpha (0.65f);
            for (int i = 0; i < 6; ++i)
                drawCloud (g, -20.f + (float) i * fw * 0.2f, 10.f + rng.nextFloat() * 50.f, 1.8f, c);
        }

        if (s.kind == 6)
        {
            g.setColour (juce::Colour (0xff5b4030));
            g.fillRect (fw * 0.86f, 40.f, 36.f, fh);

            for (int layer = 0; layer < 2; ++layer)
            {
                for (int i = 0; i < 70; ++i)
                {
                    const float k = layer == 0 ? 1.f : 0.6f;
                    const float cx = rng.nextFloat() * fw;
                    const float cy = -20.f + rng.nextFloat() * 150.f + (layer == 1 ? 10.f : 0.f);
                    const float r  = (34.f + rng.nextFloat() * 56.f) * k;
                    g.setColour (layer == 0 ? juce::Colour (0xff2f6b3a).withAlpha (0.95f)
                                            : juce::Colour (0xff58a85a).withAlpha (0.8f));
                    g.fillEllipse (cx - r, cy - r, r * 2.f, r * 1.7f);
                }
            }
            for (int i = 0; i < 70; ++i)
            {
                g.setColour (juce::Colour (0xfffff7c2).withAlpha (0.25f + 0.2f * rng.nextFloat()));
                const float r = 8.f + rng.nextFloat() * 14.f;
                g.fillEllipse (rng.nextFloat() * fw, rng.nextFloat() * 150.f, r, r * 0.8f);
            }
        }

        fillHills (g, juce::Colour (s.hillFar),  fw, fh, fh * 0.40f, 18.f, 0.006f, (float) index);
        fillHills (g, juce::Colour (s.hillNear), fw, fh, fh * 0.47f, 14.f, 0.009f, (float) index * 1.7f + 1.f);

        if (s.kind == 2)
        {
            for (int i = 0; i < 90; ++i)
            {
                const float x = rng.nextFloat() * fw, y = rng.nextFloat() * fh * 0.52f;
                g.saveState();
                g.addTransform (juce::AffineTransform::rotation (rng.nextFloat() * 6.28f, x, y));
                g.setColour (juce::Colour (0xfff8b9cc).withAlpha (0.5f + 0.5f * rng.nextFloat()));
                g.fillEllipse (x, y, 8.f, 4.f);
                g.restoreState();
            }
        }
        else if (s.kind == 3)
        {
            for (int i = 0; i < 180; ++i)
            {
                g.setColour (juce::Colours::white.withAlpha (0.5f + 0.5f * rng.nextFloat()));
                const float r = 1.5f + rng.nextFloat() * 2.5f;
                g.fillEllipse (rng.nextFloat() * fw, rng.nextFloat() * fh * 0.52f, r, r);
            }
        }
        else if (s.kind == 4)
        {
            const juce::uint32 cols[] = { 0xffd9632b, 0xffe8a33a, 0xffb5472a };
            for (int i = 0; i < 80; ++i)
            {
                const float x = rng.nextFloat() * fw, y = rng.nextFloat() * fh * 0.52f;
                g.saveState();
                g.addTransform (juce::AffineTransform::rotation (rng.nextFloat() * 6.28f, x, y));
                g.setColour (juce::Colour (cols[rng.nextInt (3)]).withAlpha (0.85f));
                g.fillEllipse (x, y, 10.f, 5.f);
                g.restoreState();
            }
        }
        else if (s.kind == 5)
        {
            g.setColour (juce::Colours::white.withAlpha (0.25f));
            for (int i = 0; i < 220; ++i)
            {
                const float x = rng.nextFloat() * fw, y = rng.nextFloat() * fh * 0.55f;
                g.drawLine (x, y, x - 6.f, y + 22.f, 1.2f);
            }
        }

        return img;
    }
}
