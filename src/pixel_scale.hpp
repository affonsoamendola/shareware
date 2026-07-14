#pragma once

#include "raylib.h"

struct RenderContext
{
    int virtual_w = 960;
    int virtual_h = 540;
    float pixel_scale = 2.0f;
};

extern RenderContext g_render;

Vector2 GetVirtualMousePos();
int GetVirtualScreenWidth();
int GetVirtualScreenHeight();
