#pragma once

#include "raylib.h"

enum class BgPattern
{
    None = 0,
    Checkerboard,
    HorizontalStripes,
    VerticalStripes,
    Dots,
    Diamonds,
    Plaid,
    CrossHatch
};

enum class BgScrollDirection
{
    None = 0,
    Horizontal,
    Vertical,
    Diagonal
};

const char* bgPatternName(BgPattern p);
BgPattern bgPatternFromName(const char* name);
int bgPatternCount();
const char* bgPatternNameByIndex(int i);

void drawBackgroundPattern(
    BgPattern pattern,
    Color colorA,
    Color colorB,
    int tileSize,
    int screenW,
    int screenH,
    float offsetX = 0.0f,
    float offsetY = 0.0f);
