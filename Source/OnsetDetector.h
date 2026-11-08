#pragma once

#include <algorithm>
#include <cmath>

//==============================================================================
// Детектор ударов. Раньше срабатывание "перезаряжалось" только когда сигнал
// падал ниже порога, поэтому, если хвост прошлого кика ещё звучал, следующий
// кик не распознавался (обрезка Length и питч его теряли). Теперь удар это
// резкий скачок уровня относительно медленной огибающей "фона", плюс короткая пауза
// (50 мс) между срабатываниями. Хвост предыдущего кика больше не мешает.
// Без зависимостей от JUCE.
//==============================================================================
class OnsetDetector
{
public:
    void prepare (double sampleRate)
    {
        relFast    = std::exp (-1.0f / (0.008f * (float) sampleRate));
        aSlow      = 1.0f - std::exp (-1.0f / (0.060f * (float) sampleRate));
        relSlow    = std::exp (-1.0f / (0.200f * (float) sampleRate));
        relMid     = std::exp (-1.0f / (0.030f * (float) sampleRate));
        refractory = (int) (0.05 * sampleRate);
        reset();
    }

    void reset()
    {
        fast = slow = mid = 0.f;
        peakSince = 0.f;
        valley = 1.0e9f;
        cooldown = 0;
        armed = true;
    }

    // peak: модуль текущего отсчёта, trigLin: порог (линейная амплитуда).
    // Возвращает true на отсчёте, где начался удар.
    bool process (float peak, float trigLin)
    {
        const float slowBefore = slow;

        fast = peak > fast ? peak : fast * relFast;
        slow = peak > slow ? slow + (peak - slow) * aSlow : slow * relSlow;
        mid  = peak > mid ? peak : mid * relMid;

        // "впадина": после удара запоминаем самый тихий уровень спада. Новый удар поверх
        // громкого хвоста всё равно заметно выше этой впадины (раньше такой кик терялся).
        peakSince = std::max (peakSince, mid);
        if (mid < 0.5f * peakSince)
            valley = std::min (valley, mid);

        if (cooldown > 0)
        {
            --cooldown;
            return false;
        }

        if (fast < trigLin * 0.3f)
            armed = true;

        // удар: резкий скачок над фоном, либо сигнал был тихим (после паузы)
        if (fast > trigLin && (armed || fast > ratio * slowBefore || (useValley && mid > 3.0f * valley)))
        {
            armed = false;
            cooldown = refractory;
            peakSince = 0.f;
            valley = 1.0e9f;
            return true;
        }

        return false;
    }

    // правило впадины: ловит новый удар поверх громкого хвоста (для Length и питча)
    void enableValley (bool on) { useValley = on; }

    float getFast() const { return fast; }

private:
    float relFast = 0.99f, aSlow = 0.001f, relSlow = 0.9999f;
    float ratio = 2.5f;
    bool useValley = false;
    float relMid = 0.99f;
    float fast = 0.f, slow = 0.f, mid = 0.f, peakSince = 0.f, valley = 1.0e9f;
    int refractory = 2205, cooldown = 0;
    bool armed = true;
};
