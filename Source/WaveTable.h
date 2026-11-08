#pragma once

#include <algorithm>
#include <cmath>

//==============================================================================
// Waveshaper: 32 точки кривой передачи (вход по горизонтали от -1 до +1,
// выход по вертикали). Звук кика проходит через эту кривую, поэтому форма
// напрямую меняет тембр: прямая линия = звук не меняется, пологая S-кривая =
// мягкий перегруз, "ступенька" = жёсткий клиппинг, волна = фолдинг.
// Кривая проходит через ноль (иначе появилась бы постоянная составляющая).
//==============================================================================
namespace WaveTable
{
    constexpr int numPoints = 32;
    constexpr int tableSize = 1024;

    // x в точке i: от -1 до +1
    inline float pointX (int i) { return -1.0f + 2.0f * (float) i / (float) (numPoints - 1); }

    // table должен вмещать tableSize + 1 значений (x от -1 до +1)
    inline void build (const float* pts, float* table)
    {
        for (int t = 0; t <= tableSize; ++t)
        {
            const float u = (float) t * (float) (numPoints - 1) / (float) tableSize;
            const int i1 = std::min ((int) u, numPoints - 2);
            const float f = u - (float) i1;

            const float p0 = pts[std::max (i1 - 1, 0)];
            const float p1 = pts[i1];
            const float p2 = pts[i1 + 1];
            const float p3 = pts[std::min (i1 + 2, numPoints - 1)];

            table[t] = 0.5f * ((2.f * p1)
                               + (-p0 + p2) * f
                               + (2.f * p0 - 5.f * p1 + 4.f * p2 - p3) * f * f
                               + (-p0 + 3.f * p1 - 3.f * p2 + p3) * f * f * f);
        }

        // кривая должна проходить через ноль
        const float zero = table[tableSize / 2];
        for (int t = 0; t <= tableSize; ++t)
            table[t] = std::max (-1.f, std::min (1.f, table[t] - zero));
    }

    // применить кривую к значению x (за пределами [-1, 1] остаётся крайнее значение)
    inline float lookup (const float* table, float x)
    {
        x = std::max (-1.0f, std::min (1.0f, x));
        const float u = (x + 1.0f) * 0.5f * (float) tableSize;
        const int i0 = std::min ((int) u, tableSize - 1);
        const float f = u - (float) i0;
        return table[i0] + f * (table[i0 + 1] - table[i0]);
    }

    // 0 LIN (прямая), 1 SOFT, 2 HARD, 3 FOLD
    inline void fillPreset (float* pts, int type)
    {
        for (int i = 0; i < numPoints; ++i)
        {
            const float x = pointX (i);

            switch (type)
            {
                case 0:  pts[i] = x; break;
                case 1:  pts[i] = std::tanh (2.5f * x) / std::tanh (2.5f); break;
                case 2:  pts[i] = std::max (-1.f, std::min (1.f, 2.5f * x)); break;
                default: pts[i] = std::sin (1.5f * 3.14159265f * x); break;
            }
        }
    }
}
