#pragma once

#include "raylib.h"

enum class BgPattern
{
    None = 0,
    Checkerboard,
    HorizontalStripes,
    VerticalStripes,
    DiagonalStripes,
    Dots,
    Diamonds,
    Waves,
    Zigzag,
    Hexagons,
    Brick,
    Plaid,
    Noise,
    CrossHatch
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
    int screenH);
