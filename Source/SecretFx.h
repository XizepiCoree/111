#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

#include "OnsetDetector.h"

//==============================================================================
// Эффекты секретного режима. Двенадцать ручек (0..1):
//   кик (8):     Ring, Stutter, Shatter, Wobble, Rectify, Scream, SubVoid, Comb
//   мелодия (4): Demon, Choir, Rewind, Drone
// Все они намеренно "ненормальные": искажают звук как могут. Без зависимостей
// от JUCE (можно проверять отдельно). Нулевые ручки ничего не меняют.
//==============================================================================
class SecretFx
{
public:
    enum { Ring, Stutter, Shatter, Wobble, Rectify, Scream, SubVoid, Comb,
           Demon, Choir, Rewind, Drone, numFx };

    void prepare (double sampleRate)
    {
        sr = (float) sampleRate;

        long long size = 1;
        while (size < (long long) (sampleRate * 1.2)) size <<= 1;
        longMask = size - 1;

        long long ss = 1;
        while (ss < (long long) (sampleRate * 0.08)) ss <<= 1;
        shortMask = ss - 1;

        for (auto& b : lb) b.assign ((size_t) size, 0.f);
        for (auto& row : sbuf) for (auto& b : row) b.assign ((size_t) ss, 0.f);

        onset.prepare (sampleRate);
        envRel = std::exp (-1.0f / (0.05f * sr));
        voidDecay = std::exp (-1.0f / (0.9f * sr));
        smC = 1.0f - std::exp (-1.0f / (0.01f * sr));
        reset();
    }

    void reset()
    {
        for (auto& b : lb) std::fill (b.begin(), b.end(), 0.f);
        for (auto& row : sbuf) for (auto& b : row) std::fill (b.begin(), b.end(), 0.f);

        std::fill (std::begin (sm), std::end (sm), 0.f);
        onset.reset();
        lw = 0; sw = 0;
        envF = 0.f;
        voidAmp = 0.f; voidPhase = 0.f; voidT = 0.f;
        ringPhase = 0.f;
        shHold = 1; shNew = true;
        stCount = 0; stEngaged = false; stG = 0.f; stPos = 0; stLen = 1; stStart = 0;
        for (int ch = 0; ch < 2; ++ch)
        {
            shHeld[ch] = 0.f; dcX[ch] = dcY[ch] = 0.f; ic1[ch] = ic2[ch] = 0.f;
            demLp[ch] = 0.f; demPrev[ch] = 0.f; demSign[ch] = 1.f;
        }
        for (int k = 0; k < 3; ++k) choirPh[k] = 0.3f * (float) k;
        wobPh = 0.f;
        rewT[0] = 0; rewT[1] = 0; rewStart[0] = rewStart[1] = 0;
        droT[0] = 0; droT[1] = 0; droStart[0] = droStart[1] = 0;
        initHeads = true;
    }

    // data: каналы (на месте), amt: 12 значений 0..1, trigLin: порог удара
    void process (float* const* data, int numCh, int n, const float* amt, float trigLin)
    {
        numCh = std::min (numCh, 2);

        float top = 0.f;
        for (int k = 0; k < numFx; ++k) top = std::max (top, std::max (amt[k], sm[k]));
        if (top < 0.0005f)
            return;

        const int W  = std::max (64, (int) (sr * 0.32f));
        const int G  = std::max (64, (int) (sr * 0.18f));
        const int combD = std::max (2, (int) (sr * 0.0037f));

        if (initHeads)
        {
            rewT[0] = 0; rewT[1] = W / 2;
            droT[0] = 0; droT[1] = G / 2;
            rewStart[0] = rewStart[1] = lw;
            droStart[0] = droStart[1] = lw - G;
            initHeads = false;
        }

        for (int i = 0; i < n; ++i)
        {
            for (int k = 0; k < numFx; ++k)
                sm[k] += (amt[k] - sm[k]) * smC;

            // --- общие вещи для всех каналов ---
            float peak = 0.f;
            for (int ch = 0; ch < numCh; ++ch)
                peak = std::max (peak, std::abs (data[ch][i]));

            envF = peak > envF ? peak : envF * envRel;

            if (onset.process (peak, trigLin))
            {
                voidAmp = 1.f; voidPhase = 0.f; voidT = 0.f;
            }

            float voidS = 0.f;
            if (voidAmp > 0.0001f)
            {
                const float f = 26.f + 34.f * std::exp (-voidT * 18.f);
                voidPhase += f / sr;
                if (voidPhase >= 1.f) voidPhase -= 1.f;
                voidS = std::tanh (3.f * std::sin (6.2831853f * voidPhase)) * voidAmp;
                voidAmp *= voidDecay;
                voidT += 1.f / sr;
            }

            // кольцевой модулятор
            const float aRing = sm[Ring];
            ringPhase += 40.f * std::exp2 (aRing * 5.f) / sr;
            if (ringPhase >= 1.f) ringPhase -= 1.f;
            const float ringS = std::sin (6.2831853f * ringPhase);

            // осколки: случайное удержание отсчётов
            const float aSh = sm[Shatter];
            if (--shHold <= 0)
            {
                shHold = 1 + (int) (aSh * aSh * 40.f * rnd01());
                shNew = true;
            }

            // заикание
            const float aSt = sm[Stutter];
            if (--stCount <= 0)
            {
                stCount = std::max (1, (int) (sr * 0.11f));
                stEngaged = aSt > 0.001f && rnd01() < aSt * 1.2f;
                stLen = std::max (16, (int) (sr * (0.012f + 0.04f * rnd01())));
                stStart = lw - stLen;
                stPos = 0;
            }
            stG += ((stEngaged ? 1.f : 0.f) - stG) * (1.f / 64.f);

            // качание задержки
            wobPh += (0.4f + sm[Wobble] * 7.f) / sr;
            if (wobPh >= 1.f) wobPh -= 1.f;

            // хор: три медленных модулированных задержки
            const float chRate[3] = { 0.31f, 0.47f, 0.83f };
            for (int k = 0; k < 3; ++k)
            {
                choirPh[k] += chRate[k] / sr;
                if (choirPh[k] >= 1.f) choirPh[k] -= 1.f;
            }

            // визг: полосовой фильтр, частота следует за громкостью
            float a1 = 0.f, a2 = 0.f, a3 = 0.f, kq = 1.f;
            {
                const float fc = std::min (0.45f * sr, 300.f + 3800.f * std::min (1.f, envF * 1.5f));
                const float g = std::tan (3.14159265f * fc / sr);
                const float q = 3.f + sm[Scream] * 18.f;
                kq = 1.f / q;
                a1 = 1.f / (1.f + g * (g + kq));
                a2 = g * a1;
                a3 = g * a2;
            }

            // перемотка и дрон: головки с окном Ханна
            for (int h = 0; h < 2; ++h)
            {
                if (rewT[h] >= W) { rewT[h] = 0; rewStart[h] = lw; }
                if (droT[h] >= G) { droT[h] = 0; droStart[h] = lw - G; }
            }
            float rewWin[2], droWin[2];
            for (int h = 0; h < 2; ++h)
            {
                const float pr = (float) rewT[h] / (float) W;
                const float pd = (float) droT[h] / (float) G;
                const float sr_ = std::sin (3.14159265f * pr);
                const float sd_ = std::sin (3.14159265f * pd);
                rewWin[h] = sr_ * sr_;
                droWin[h] = sd_ * sd_;
            }

            for (int ch = 0; ch < numCh; ++ch)
            {
                float x = data[ch][i];

                // 1) выпрямление (октава вверх, толстый кик) + срез постоянной составляющей
                const float aRe = sm[Rectify];
                if (aRe > 0.0005f)
                {
                    const float y = x * (1.f - aRe) + std::abs (x) * aRe;
                    dcY[ch] = y - dcX[ch] + 0.995f * dcY[ch];
                    dcX[ch] = y;
                    x = dcY[ch];
                }

                // 2) кольцевая модуляция
                if (aRing > 0.0005f)
                {
                    const float m = std::min (1.f, aRing * 3.f);
                    x = x * (1.f - m) + x * ringS * m;
                }

                // 3) осколки
                if (aSh > 0.0005f)
                {
                    if (shNew) shHeld[ch] = x;
                    const float levels = std::exp2 (15.f - aSh * 11.f);
                    const float y = std::round (shHeld[ch] * levels) / levels;
                    const float m = std::min (1.f, aSh * 4.f);
                    x = x * (1.f - m) + y * m;
                }

                // 4) гребёнка
                {
                    auto& cb = sbuf[0][ch];
                    const float v = cb[(size_t) ((sw - combD) & shortMask)];
                    const float aC = sm[Comb];
                    cb[(size_t) (sw & shortMask)] = x + v * aC * 0.92f;
                    x = x + std::min (1.f, aC * 3.f) * v * 0.8f;
                }

                // 5) качание тона
                {
                    auto& wb = sbuf[1][ch];
                    wb[(size_t) (sw & shortMask)] = x;
                    const float aW = sm[Wobble];
                    if (aW > 0.0005f)
                    {
                        const float depth = aW * 0.007f * sr;
                        const float d = depth + 2.f + depth * std::sin (6.2831853f * wobPh)
                                        + 0.35f * depth * std::sin (6.2831853f * wobPh * 2.7f + 1.f);
                        const float y = readShort (wb, d);
                        const float m = std::min (1.f, aW * 8.f);
                        x = x * (1.f - m) + y * m;
                    }
                }

                // 6) визг
                {
                    const float v3 = x - ic2[ch];
                    const float v1 = a1 * ic1[ch] + a2 * v3;
                    const float v2 = ic2[ch] + a2 * ic1[ch] + a3 * v3;
                    ic1[ch] = 2.f * v1 - ic1[ch];
                    ic2[ch] = 2.f * v2 - ic2[ch];
                    x += sm[Scream] * v1 * kq * 30.f;
                }

                // длинный буфер для заикания, перемотки и дрона
                lb[ch][(size_t) (lw & longMask)] = x;

                // 7) заикание: подменяем звук зацикленным куском
                if (stG > 0.001f)
                {
                    const long long idx = stStart + (stPos % stLen);
                    x = x * (1.f - stG) + lb[ch][(size_t) (idx & longMask)] * stG;
                }

                // 8) демон: октава вниз (делитель частоты)
                {
                    demLp[ch] += 0.12f * (x - demLp[ch]);
                    if (demPrev[ch] <= 0.f && demLp[ch] > 0.f)
                        demSign[ch] = -demSign[ch];
                    demPrev[ch] = demLp[ch];
                    x += sm[Demon] * 1.5f * demLp[ch] * demSign[ch];
                }

                // 9) хор
                {
                    auto& hb = sbuf[2][ch];
                    hb[(size_t) (sw & shortMask)] = x;
                    const float aH = sm[Choir];
                    if (aH > 0.0005f)
                    {
                        const float cc[3] = { 0.016f, 0.023f, 0.012f };
                        const float dd[3] = { 0.006f, 0.008f, 0.005f };
                        float sum = 0.f;
                        for (int k = 0; k < 3; ++k)
                            sum += readShort (hb, sr * (cc[k] + dd[k] * std::sin (6.2831853f * choirPh[k])));
                        x = x * (1.f - 0.3f * aH) + aH * sum * 0.7f;
                    }
                }

                // 10) перемотка (обратные зёрна) и дрон (зависающие зёрна)
                {
                    float rev = 0.f, dro = 0.f;
                    for (int h = 0; h < 2; ++h)
                    {
                        rev += rewWin[h] * lb[ch][(size_t) ((rewStart[h] - rewT[h]) & longMask)];
                        dro += droWin[h] * lb[ch][(size_t) ((droStart[h] + droT[h]) & longMask)];
                    }
                    x += sm[Rewind] * 1.3f * rev;
                    x = x * (1.f - 0.4f * sm[Drone]) + sm[Drone] * 1.4f * dro;
                }

                // 11) бездна: гигантский саб от каждого удара
                x += sm[SubVoid] * 0.9f * voidS;

                // страховка от перегруза
                data[ch][i] = std::tanh (x * 0.9f) * (1.f / 0.9f);
            }

            shNew = false;
            ++stPos;
            for (int h = 0; h < 2; ++h) { ++rewT[h]; ++droT[h]; }
            ++lw;
            ++sw;
        }
    }

private:
    float rnd01()
    {
        seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5;
        return (float) (seed & 0xffffff) / 16777216.f;
    }

    float readShort (const std::vector<float>& b, float delay) const
    {
        const float rp = (float) sw - delay;
        const long long i0 = (long long) std::floor (rp);
        const float f = rp - (float) i0;
        return b[(size_t) (i0 & shortMask)] * (1.f - f) + b[(size_t) ((i0 + 1) & shortMask)] * f;
    }

    float sr = 44100.f, envRel = 0.99f, voidDecay = 0.9999f, smC = 0.002f;
    long long longMask = 0, shortMask = 0, lw = 0, sw = 0;
    std::vector<float> lb[2];
    std::vector<float> sbuf[3][2];

    OnsetDetector onset;
    float sm[numFx] {};
    float envF = 0.f;
    float voidAmp = 0.f, voidPhase = 0.f, voidT = 0.f;
    float ringPhase = 0.f;
    int shHold = 1; bool shNew = true;
    float shHeld[2] {}, dcX[2] {}, dcY[2] {}, ic1[2] {}, ic2[2] {};
    int stCount = 0, stPos = 0, stLen = 1; bool stEngaged = false; float stG = 0.f; long long stStart = 0;
    float wobPh = 0.f, choirPh[3] {};
    float demLp[2] {}, demPrev[2] {}, demSign[2] { 1.f, 1.f };
    int rewT[2] {}, droT[2] {}; long long rewStart[2] {}, droStart[2] {};
    bool initHeads = true;
    unsigned int seed = 2463534242u;
};
