#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

#include "OnsetDetector.h"

//==============================================================================
// Питч кика без смазывания во времени ("re-trigger resampling").
//
// Плагин всегда задерживает звук на lookahead (100 мс), поэтому, когда на входе
// обнаружен удар, он уже "знает будущее": голос проигрывает записанный кик с
// нужной скоростью (ratio), начиная ровно с момента удара. Так тон кика меняется
// как при изменении скорости воспроизведения сэмпла, а атака остаётся на месте.
//
// Если ratio == 1, кик просто проходит с задержкой lookahead (задержка постоянна).
// Для питча вверх голос догоняет запись, поэтому длинные хвосты при больших
// значениях (около +12) укорачиваются.
//
// Без зависимостей от JUCE, чтобы можно было проверять отдельно.
//==============================================================================
class KickPitcher
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        lookahead   = (int) std::lround (0.1 * sr);
        lookback    = std::max (16, (int) std::lround (0.002 * sr));
        maxVoiceLen = (int) std::lround (2.0 * sr);
        idleLimit   = (int) std::lround (3.0 * sr);
        onset.enableValley (true);
        onset.prepare (sampleRate);

        long long size = 1;
        while (size < (long long) (sr * 3.0))
            size <<= 1;

        for (auto& h : hist)
            h.assign ((size_t) size, 0.f);

        mask = size - 1;
        reset();
    }

    void reset()
    {
        for (auto& h : hist)
            std::fill (h.begin(), h.end(), 0.f);

        for (auto& v : voices)
            v = Voice();

        onset.reset();
        w = 0;
        holdMute = false;
        idle = 0;
        vg = 0.f;
    }

    int getLatency() const { return lookahead; }

    // data: указатели на каналы (обрабатывается на месте), ratio: скорость воспроизведения
    // (1 = без изменений), trigLin: порог срабатывания (линейная амплитуда)
    void process (float* const* data, int numCh, int n, float ratio, float trigLin)
    {
        numCh = std::min (numCh, 2);
        const bool shifting = std::abs (ratio - 1.0f) > 0.0005f;
        const float vgStep = 1.0f / 64.0f;

        for (int i = 0; i < n; ++i)
        {
            // 1) запись входа
            float peak = 0.f;
            for (int ch = 0; ch < numCh; ++ch)
            {
                const float x = data[ch][i];
                hist[ch][(size_t) (w & mask)] = x;
                peak = std::max (peak, std::abs (x));
            }

            // 2) поиск удара
            if (onset.process (peak, trigLin) && shifting)
                startVoice (ratio);

            // 3) голоса
            float sum[2] = { 0.f, 0.f };
            bool any = false;

            for (auto& v : voices)
            {
                if (! v.active)
                    continue;

                if (v.waiting)
                {
                    if (--v.waitLeft <= 0)
                    {
                        v.waiting = false;
                        holdMute = true;

                        for (auto& u : voices)
                            if (&u != &v && u.active && ! u.waiting)
                                u.retiring = true;
                    }
                    continue;
                }

                const double lag = (double) w - v.pos;
                if (lag < 4.0 || v.age >= maxVoiceLen)
                {
                    v.active = false;
                    continue;
                }

                float g = std::min (1.0f, (float) (v.age + 1) / 64.0f);

                if (lag < 600.0)
                    g *= (float) ((lag - 4.0) / 596.0);

                const int remaining = maxVoiceLen - v.age;
                if (remaining < 4096)
                    g *= (float) remaining / 4096.0f;

                if (v.retiring)
                {
                    v.retireGain -= 1.0f / 256.0f;
                    if (v.retireGain <= 0.f)
                    {
                        v.active = false;
                        continue;
                    }
                    g *= v.retireGain;
                }

                for (int ch = 0; ch < numCh; ++ch)
                    sum[ch] += g * hermite (hist[ch], mask, v.pos);

                v.pos += (double) v.ratio;
                ++v.age;
                any = true;
            }

            // оригинал приглушён, пока играет голос (и после его конца, пока не закончится
            // кик на входе). Страховка: если голоса нет уже 300 мс, оригинал возвращается.
            if (any)
                idle = 0;
            else if (idle < 1000000000)
                ++idle;

            if (holdMute && ! any && (onset.getFast() < trigLin * 0.3f || idle > idleLimit))
                holdMute = false;

            const float target = (any || holdMute) ? 1.0f : 0.0f;
            if (vg < target)      vg = std::min (target, vg + vgStep);
            else if (vg > target) vg = std::max (target, vg - vgStep);

            // 4) выход: задержанный оригинал или голос
            for (int ch = 0; ch < numCh; ++ch)
            {
                const float dry = hist[ch][(size_t) ((w - lookahead) & mask)];
                data[ch][i] = dry * (1.0f - vg) + sum[ch] * vg;
            }

            ++w;
        }
    }

private:
    struct Voice
    {
        bool active = false;
        bool waiting = false;
        bool retiring = false;
        int waitLeft = 0;
        int age = 0;
        double pos = 0.0;
        float ratio = 1.0f;
        float retireGain = 1.0f;
    };

    void startVoice (float ratio)
    {
        Voice* slot = nullptr;
        for (auto& v : voices)
            if (! v.active) { slot = &v; break; }

        if (slot == nullptr)
            slot = &voices[0];

        *slot = Voice();
        slot->active = true;
        slot->waiting = true;
        slot->waitLeft = std::max (1, lookahead - lookback);
        slot->pos = (double) (w - lookback);
        slot->ratio = ratio;
    }

    static float hermite (const std::vector<float>& b, long long mask, double pos)
    {
        const long long i = (long long) std::floor (pos);
        const float f = (float) (pos - (double) i);

        const float y0 = b[(size_t) ((i - 1) & mask)];
        const float y1 = b[(size_t) (i & mask)];
        const float y2 = b[(size_t) ((i + 1) & mask)];
        const float y3 = b[(size_t) ((i + 2) & mask)];

        const float c1 = 0.5f * (y2 - y0);
        const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
        const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
        return ((c3 * f + c2) * f + c1) * f + y1;
    }

    static constexpr int numVoices = 3;

    double sr = 44100.0;
    int lookahead = 4410, lookback = 88, maxVoiceLen = 44100, idleLimit = 13230;
    std::vector<float> hist[2];
    long long mask = 0;
    long long w = 0;

    OnsetDetector onset;
    Voice voices[numVoices];
    bool holdMute = false;
    int idle = 0;
    float vg = 0.f;
};
