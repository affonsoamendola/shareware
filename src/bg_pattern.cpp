#include "bg_pattern.hpp"
#include <cmath>
#include <cstdint>
#include <cstring>

static const char* patternNames[] =
{
    "none",
    "checkerboard",
    "h_stripes",
    "v_stripes",
    "diagonal_stripes",
    "dots",
    "diamonds",
    "waves",
    "zigzag",
    "hexagons",
    "brick",
    "plaid",
    "noise",
    "cross_hatch"
};

const char* bgPatternName(BgPattern p)
{
    int idx = static_cast<int>(p);
    if (idx < 0 || idx >= 14) return "none";
    return patternNames[idx];
}

BgPattern bgPatternFromName(const char* name)
{
    for (int i = 0; i < 14; i++)
    {
        if (strcmp(patternNames[i], name) == 0)
            return static_cast<BgPattern>(i);
    }
    return BgPattern::None;
}

int bgPatternCount() { return 14; }

const char* bgPatternNameByIndex(int i)
{
    if (i < 0 || i >= 14) return "none";
    return patternNames[i];
}

static uint32_t hashCoord(int x, int y)
{
    uint32_t h = static_cast<uint32_t>(x * 374761393 + y * 668265263);
    h = (h ^ (h >> 13)) * 1274126177;
    return h ^ (h >> 16);
}

static void drawCheckerboard(int ts, Color ca, Color cb, int w, int h)
{
    for (int y = 0; y < h; y += ts)
    {
        for (int x = 0; x < w; x += ts)
        {
            bool odd = ((x / ts) + (y / ts)) % 2 != 0;
            DrawRectangle(x, y, ts, ts, odd ? cb : ca);
        }
    }
}

static void drawHStripes(int ts, Color ca, Color cb, int w, int h)
{
    for (int y = 0; y < h; y += ts)
    {
        Color c = ((y / ts) % 2 == 0) ? ca : cb;
        DrawRectangle(0, y, w, ts, c);
    }
}

static void drawVStripes(int ts, Color ca, Color cb, int w, int h)
{
    for (int x = 0; x < w; x += ts)
    {
        Color c = ((x / ts) % 2 == 0) ? ca : cb;
        DrawRectangle(x, 0, ts, h, c);
    }
}

static void drawDiagonalStripes(int ts, Color ca, Color cb, int w, int h)
{
    int thick = ts / 2;
    if (thick < 1) thick = 1;
    for (int y = -h; y < h * 2; y += thick * 2)
    {
        DrawTriangle(
            {(float)(y), 0.0f},
            {(float)(y + thick), 0.0f},
            {(float)(y - h), (float)h},
            ca);
        DrawTriangle(
            {(float)(y + thick), 0.0f},
            {(float)(y - h + thick), (float)h},
            {(float)(y - h), (float)h},
            ca);
    }
    for (int y = -h; y < h * 2; y += thick * 2)
    {
        DrawTriangle(
            {(float)(y + thick), 0.0f},
            {(float)(y + thick * 2), 0.0f},
            {(float)(y - h + thick), (float)h},
            cb);
        DrawTriangle(
            {(float)(y + thick * 2), 0.0f},
            {(float)(y - h + thick * 2), (float)h},
            {(float)(y - h + thick), (float)h},
            cb);
    }
}

static void drawDots(int ts, Color ca, Color cb, int w, int h)
{
    DrawRectangle(0, 0, w, h, cb);
    int radius = ts / 3;
    if (radius < 1) radius = 1;
    for (int y = ts / 2; y < h; y += ts)
    {
        for (int x = ts / 2; x < w; x += ts)
        {
            DrawCircle(x, y, (float)radius, ca);
        }
    }
}

static void drawDiamonds(int ts, Color ca, Color cb, int w, int h)
{
    DrawRectangle(0, 0, w, h, cb);
    float half = (float)ts / 2.0f;
    for (int row = -1; row < h / ts + 2; row++)
    {
        for (int col = -1; col < w / ts + 2; col++)
        {
            float cx = col * ts + ((row % 2 != 0) ? half : 0);
            float cy = row * ts;
            DrawTriangle(
                {cx, cy - half},
                {cx + half, cy},
                {cx - half, cy},
                ca);
            DrawTriangle(
                {cx, cy + half},
                {cx + half, cy},
                {cx - half, cy},
                ca);
        }
    }
}

static void drawWaves(int ts, Color ca, Color cb, int w, int h)
{
    DrawRectangle(0, 0, w, h, cb);
    int amp = ts / 3;
    if (amp < 1) amp = 1;
    int period = ts * 2;
    for (int y = 0; y < h; y++)
    {
        float offset = sinf((float)y * 3.14159f * 2.0f / (float)period) * (float)amp;
        int x0 = (int)offset;
        DrawRectangle(x0, y, w - x0, 1, ca);
    }
}

static void drawZigzag(int ts, Color ca, Color cb, int w, int h)
{
    DrawRectangle(0, 0, w, h, cb);
    int amp = ts / 2;
    if (amp < 1) amp = 1;
    int period = ts * 2;
    for (int y = 0; y < h; y++)
    {
        int phase = y % period;
        int half = period / 2;
        int xOff;
        if (phase < half)
            xOff = (amp * phase) / half;
        else
            xOff = amp - (amp * (phase - half)) / half;
        DrawRectangle(xOff, y, w - xOff, 1, ca);
    }
}

static void drawHexagons(int ts, Color ca, Color cb, int w, int h)
{
    DrawRectangle(0, 0, w, h, cb);
    float hexR = (float)ts * 0.45f;
    float hexW = hexR * 2.0f;
    float hexH = hexR * 1.732f;
    float xStep = hexW * 0.75f;
    float yStep = hexH;
    for (int row = -2; row < (int)(h / yStep) + 3; row++)
    {
        for (int col = -2; col < (int)(w / xStep) + 3; col++)
        {
            float cx = col * xStep;
            float cy = row * yStep + (col % 2 != 0 ? hexH * 0.5f : 0.0f);
            for (int i = 0; i < 6; i++)
            {
                float a1 = (float)i * 60.0f * 3.14159f / 180.0f;
                float a2 = (float)(i + 1) * 60.0f * 3.14159f / 180.0f;
                float x1 = cx + hexR * cosf(a1);
                float y1 = cy + hexR * sinf(a1);
                float x2 = cx + hexR * cosf(a2);
                float y2 = cy + hexR * sinf(a2);
                DrawTriangle(
                    {cx, cy},
                    {x1, y1},
                    {x2, y2},
                    ca);
            }
        }
    }
}

static void drawBrick(int ts, Color ca, Color cb, int w, int h)
{
    DrawRectangle(0, 0, w, h, cb);
    int brickH = ts;
    int brickW = ts * 2;
    int mortar = 2;
    for (int row = 0; row < h; row += brickH)
    {
        int offset = ((row / brickH) % 2 == 0) ? 0 : brickW / 2;
        for (int col = -brickW; col < w + brickW; col += brickW)
        {
            DrawRectangle(col + offset + mortar, row + mortar,
                          brickW - mortar * 2, brickH - mortar * 2, ca);
        }
    }
}

static void drawPlaid(int ts, Color ca, Color cb, int w, int h)
{
    DrawRectangle(0, 0, w, h, ca);
    int thick = ts / 4;
    if (thick < 1) thick = 1;
    Color halfA = ca;
    halfA.a = 128;
    Color halfB = cb;
    halfB.a = 128;
    for (int x = 0; x < w; x += ts)
    {
        DrawRectangle(x, 0, thick, h, cb);
        DrawRectangle(x + thick / 2, 0, thick / 2, h, halfB);
    }
    for (int y = 0; y < h; y += ts)
    {
        DrawRectangle(0, y, w, thick, cb);
        DrawRectangle(0, y + thick / 2, w, thick / 2, halfB);
    }
}

static void drawNoise(int ts, Color ca, Color cb, int w, int h)
{
    DrawRectangle(0, 0, w, h, cb);
    for (int y = 0; y < h; y += ts)
    {
        for (int x = 0; x < w; x += ts)
        {
            uint32_t h = hashCoord(x / ts, y / ts);
            if ((h & 0xFF) < 128)
                DrawRectangle(x, y, ts, ts, ca);
        }
    }
}

static void drawCrossHatch(int ts, Color ca, Color cb, int w, int h)
{
    DrawRectangle(0, 0, w, h, cb);
    int thick = ts / 5;
    if (thick < 1) thick = 1;
    for (int d = -h; d < w + h; d += ts)
    {
        DrawRectangle(d, 0, thick, h, ca);
        DrawRectangle(0, d, w, thick, ca);
    }
}

void drawBackgroundPattern(
    BgPattern pattern,
    Color colorA,
    Color colorB,
    int tileSize,
    int screenW,
    int screenH)
{
    if (pattern == BgPattern::None) return;
    if (tileSize < 2) tileSize = 2;

    switch (pattern)
    {
        case BgPattern::Checkerboard:      drawCheckerboard(tileSize, colorA, colorB, screenW, screenH); break;
        case BgPattern::HorizontalStripes:  drawHStripes(tileSize, colorA, colorB, screenW, screenH); break;
        case BgPattern::VerticalStripes:    drawVStripes(tileSize, colorA, colorB, screenW, screenH); break;
        case BgPattern::DiagonalStripes:    drawDiagonalStripes(tileSize, colorA, colorB, screenW, screenH); break;
        case BgPattern::Dots:               drawDots(tileSize, colorA, colorB, screenW, screenH); break;
        case BgPattern::Diamonds:           drawDiamonds(tileSize, colorA, colorB, screenW, screenH); break;
        case BgPattern::Waves:              drawWaves(tileSize, colorA, colorB, screenW, screenH); break;
        case BgPattern::Zigzag:             drawZigzag(tileSize, colorA, colorB, screenW, screenH); break;
        case BgPattern::Hexagons:           drawHexagons(tileSize, colorA, colorB, screenW, screenH); break;
        case BgPattern::Brick:              drawBrick(tileSize, colorA, colorB, screenW, screenH); break;
        case BgPattern::Plaid:              drawPlaid(tileSize, colorA, colorB, screenW, screenH); break;
        case BgPattern::Noise:              drawNoise(tileSize, colorA, colorB, screenW, screenH); break;
        case BgPattern::CrossHatch:         drawCrossHatch(tileSize, colorA, colorB, screenW, screenH); break;
        default: break;
    }
}
