#pragma once

#include "raylib.h"

struct RenderContext
{
    int virtual_w = 960;
    int virtual_h = 540;
    float pixel_scale = 2.0f;
};

struct ViewTransform
{
    Vector2 offset = {0, 0};
    float scale = 1.0f;
};

extern RenderContext g_render;
extern ViewTransform g_view;

Vector2 GetVirtualMousePos();
Vector2 GetCanvasMousePos();
int GetVirtualScreenWidth();
int GetVirtualScreenHeight();
