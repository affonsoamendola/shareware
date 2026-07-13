#pragma once

#include "raylib.h"
#include <string>

class Sprite 
{
public:
    Sprite() = default;
    Sprite(Texture2D texture);
    Sprite(Texture2D texture, Rectangle source);

    void draw();
    void setScale(float sx, float sy);
    void setSourceRect(Rectangle source);

    Texture2D texture = {};
    Rectangle dest = {0, 0, 0, 0};
    Vector2 origin = {0, 0};

    float rotation = 0.0f;
    Color tint = WHITE;
    bool flip_h = false;
    bool flip_v = false;
    bool loaded = false;

    void UpdateDest();
private:
    Vector2 scale = {1.0f, 1.0f};
    Rectangle source = {0, 0, 0, 0};
};
