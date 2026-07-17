#include "bg_pattern.hpp"
#include <cmath>
#include <cstdint>
#include <cstring>
#include <cstdio>

static const char* patternNames[] =
{
    "none",
    "checkerboard",
    "h_stripes",
    "v_stripes",
    "dots",
    "diamonds",
    "plaid",
    "cross_hatch"
};

const char* bgPatternName(BgPattern p)
{
    int idx = static_cast<int>(p);
    if (idx < 0 || idx >= 8) return "none";
    return patternNames[idx];
}

BgPattern bgPatternFromName(const char* name)
{
    for (int i = 0; i < 8; i++)
    {
        if (strcmp(patternNames[i], name) == 0)
            return static_cast<BgPattern>(i);
    }
    return BgPattern::None;
}

int bgPatternCount() { return 8; }

const char* bgPatternNameByIndex(int i)
{
    if (i < 0 || i >= 8) return "none";
    return patternNames[i];
}

static float fposmod(float a, float b)
{
    float r = fmodf(a, b);
    return r < 0.0f ? r + b : r;
}

static void drawCheckerboard(int ts, Color ca, Color cb, int w, int h, float ox, float oy)
{
    float period = (float)(ts * 2);
    float oxW = fposmod(ox, period);
    float oyW = fposmod(oy, period);
    float fts = (float)ts;
    int startX = (int)floorf(-oxW / fts) * ts;
    int startY = (int)floorf(-oyW / fts) * ts;
    int endX = w + ts;
    int endY = h + ts;
    for (int y = startY; y < endY; y += ts)
    {
        for (int x = startX; x < endX; x += ts)
        {
            float drawXf = (float)x + oxW;
            float drawYf = (float)y + oyW;
            int tx = x / ts;
            int ty = y / ts;
            bool odd = (tx + ty) % 2 != 0;
            DrawRectangle((int)drawXf, (int)drawYf, ts, ts, odd ? cb : ca);
        }
    }
}

static void drawHStripes(int ts, Color ca, Color cb, int w, int h, float ox, float oy)
{
    float period = (float)(ts * 2);
    float oyW = fposmod(oy, period);
    float fts = (float)ts;
    int startY = (int)floorf(-oyW / fts) * ts;
    int endY = h + ts;
    for (int y = startY; y < endY; y += ts)
    {
        float drawYf = (float)y + oyW;
        int ty = y / ts;
        Color c = ((ty & 1) == 0) ? ca : cb;
        DrawRectangle(0, (int)drawYf, w, ts, c);
    }
}

static void drawVStripes(int ts, Color ca, Color cb, int w, int h, float ox, float oy)
{
    float period = (float)(ts * 2);
    float oxW = fposmod(ox, period);
    float fts = (float)ts;
    int startX = (int)floorf(-oxW / fts) * ts;
    int endX = w + ts;
    for (int x = startX; x < endX; x += ts)
    {
        float drawXf = (float)x + oxW;
        int tx = x / ts;
        Color c = ((tx & 1) == 0) ? ca : cb;
        DrawRectangle((int)drawXf, 0, ts, h, c);
    }
}

static void drawDots(int ts, Color ca, Color cb, int w, int h, float ox, float oy)
{
    DrawRectangle(0, 0, w, h, cb);
    int radius = ts / 3;
    if (radius < 1) radius = 1;
    float oxW = fposmod(ox, (float)ts);
    float oyW = fposmod(oy, (float)ts);
    float fts = (float)ts;
    int startX = (int)floorf(-oxW / fts) * ts;
    int startY = (int)floorf(-oyW / fts) * ts;
    int endX = w + ts;
    int endY = h + ts;
    float halfTs = fts * 0.5f;
    for (int y = startY; y < endY; y += ts)
    {
        for (int x = startX; x < endX; x += ts)
        {
            float cx = (float)x + oxW + halfTs;
            float cy = (float)y + oyW + halfTs;
            DrawCircle(cx, cy, (float)radius, ca);
        }
    }
}

static void drawDiamonds(int ts, Color ca, Color cb, int w, int h, float ox, float oy)
{
    DrawRectangle(0, 0, w, h, cb);
    float half = (float)ts / 2.0f;
    float oxW = fposmod(ox, (float)ts);
    float oyW = fposmod(oy, (float)ts);
    int startX = (int)floorf(-oxW / (float)ts) - 2;
    int startY = (int)floorf(-oyW / (float)ts) - 2;
    int endX = w / ts + 3;
    int endY = h / ts + 3;
    for (int row = startY; row < endY; row++)
    {
        for (int col = startX; col < endX; col++)
        {
            float cx = (float)(col * ts) + oxW;
            float cy = (float)(row * ts) + oyW;
            
            DrawTriangle(
                {cx, cy - half},
                {cx - half, cy},
                {cx + half, cy},
                ca);
            
            DrawTriangle(
                {cx, cy + half},
                {cx + half, cy},
                {cx - half, cy},
                ca);
        }
    }
}

static void drawPlaid(int ts, Color ca, Color cb, int w, int h, float ox, float oy)
{
    DrawRectangle(0, 0, w, h, ca);
    int thick = ts / 4;
    if (thick < 1) thick = 1;
    Color halfA = ca;
    halfA.a = 128;
    Color halfB = cb;
    halfB.a = 128;
    float period = (float)(ts * 2);
    float oxW = fposmod(ox, period);
    float oyW = fposmod(oy, period);
    float fts = (float)ts;
    int startX = (int)floorf(-oxW / fts) - 1;
    int endX = (int)((float)w / fts) + 2;
    for (int x = startX; x < endX; x++)
    {
        float drawX = (float)x * fts + oxW;
        DrawRectangle((int)drawX, 0, thick, h, cb);
        DrawRectangle((int)drawX + thick / 2, 0, thick / 2, h, halfB);
    }
    int startY = (int)floorf(-oyW / fts) - 1;
    int endY = (int)((float)h / fts) + 2;
    for (int y = startY; y < endY; y++)
    {
        float drawY = (float)y * fts + oyW;
        DrawRectangle(0, (int)drawY, w, thick, cb);
        DrawRectangle(0, (int)drawY + thick / 2, w, thick / 2, halfB);
    }
}

static void drawCrossHatch(int ts, Color ca, Color cb, int w, int h, float ox, float oy)
{
    DrawRectangle(0, 0, w, h, cb);
    int thick = ts / 5;
    if (thick < 1) thick = 1;
    float oxW = fposmod(ox, (float)ts);
    float oyW = fposmod(oy, (float)ts);
    float fts = (float)ts;
    int total = (w + h) / ts + 4;
    int startD = -total / 2;
    int endD = total;
    for (int d = startD; d < endD; d++)
    {
        float drawX = (float)d * fts + oxW;
        float drawY = (float)d * fts + oyW;
        DrawRectangle((int)drawX, 0, thick, h, ca);
        DrawRectangle(0, (int)drawY, w, thick, ca);
    }
}

void drawBackgroundPattern(
    BgPattern pattern,
    Color colorA,
    Color colorB,
    int tileSize,
    int screenW,
    int screenH,
    float offsetX,
    float offsetY)
{
    if (pattern == BgPattern::None) return;
    if (tileSize < 2) tileSize = 2;

    switch (pattern)
    {
        case BgPattern::Checkerboard:      drawCheckerboard(tileSize, colorA, colorB, screenW, screenH, offsetX, offsetY); break;
        case BgPattern::HorizontalStripes:  drawHStripes(tileSize, colorA, colorB, screenW, screenH, offsetX, offsetY); break;
        case BgPattern::VerticalStripes:    drawVStripes(tileSize, colorA, colorB, screenW, screenH, offsetX, offsetY); break;
        case BgPattern::Dots:               drawDots(tileSize, colorA, colorB, screenW, screenH, offsetX, offsetY); break;
        case BgPattern::Diamonds:           drawDiamonds(tileSize, colorA, colorB, screenW, screenH, offsetX, offsetY); break;
        case BgPattern::Plaid:              drawPlaid(tileSize, colorA, colorB, screenW, screenH, offsetX, offsetY); break;
        case BgPattern::CrossHatch:         drawCrossHatch(tileSize, colorA, colorB, screenW, screenH, offsetX, offsetY); break;
        default: break;
    }
}
