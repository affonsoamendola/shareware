#pragma once

#include "raylib.h"
#include <cmath>
#include <cstring>

inline Color hsvToRgb(float h, float s, float v)
{
    float c = v * s;
    float x = c * (1.0f - fabsf(fmodf(h / 60.0f, 2.0f) - 1.0f));
    float m = v - c;
    float r, g, b;
    if      (h < 60)  { r = c; g = x; b = 0; }
    else if (h < 120) { r = x; g = c; b = 0; }
    else if (h < 180) { r = 0; g = c; b = x; }
    else if (h < 240) { r = 0; g = x; b = c; }
    else if (h < 300) { r = x; g = 0; b = c; }
    else              { r = c; g = 0; b = x; }
    return {
        static_cast<unsigned char>((r + m) * 255),
        static_cast<unsigned char>((g + m) * 255),
        static_cast<unsigned char>((b + m) * 255),
        255
    };
}

inline void rgbToHsv(int ri, int gi, int bi, float& h, float& s, float& v)
{
    float r = ri / 255.0f;
    float g = gi / 255.0f;
    float b = bi / 255.0f;
    float cmax = fmaxf(r, fmaxf(g, b));
    float cmin = fminf(r, fminf(g, b));
    float d = cmax - cmin;
    v = cmax;
    if (cmax == 0) { s = 0; h = 0; return; }
    s = d / cmax;
    if (d == 0) { h = 0; return; }
    if (cmax == r)      h = 60.0f * fmodf((g - b) / d, 6.0f);
    else if (cmax == g) h = 60.0f * ((b - r) / d + 2.0f);
    else                h = 60.0f * ((r - g) / d + 4.0f);
    if (h < 0) h += 360.0f;
}

inline Color colorToEdit(int r, int g, int b, int a)
{
    return {(unsigned char)r, (unsigned char)g, (unsigned char)b, (unsigned char)a};
}
