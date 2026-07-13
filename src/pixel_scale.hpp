#pragma once

#include "raylib.h"

extern int VIRTUAL_W;
extern int VIRTUAL_H;
extern float PIXEL_SCALE;

Vector2 GetVirtualMousePos();
int GetVirtualScreenWidth();
int GetVirtualScreenHeight();
