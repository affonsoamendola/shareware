#pragma once

#include "raylib.h"
#include "sprite.hpp"
#include <string>
#include <vector>
#include <unordered_map>

class SpriteSheet 
{
public:
    SpriteSheet() = default;
    SpriteSheet(Texture2D texture, int frameWidth, int frameHeight);

    Sprite getFrame(int index);
    Sprite getFrame(int row, int col);
    Sprite getNamedFrame(const std::string& name);

    void addRegion(const std::string& name, Rectangle rect);
    int getFrameCount();
    int getColumns();
    int getRows();

    bool isLoaded();

private:
    Texture2D texture = {};
    int frame_width = 0;
    int frame_height = 0;
    int columns = 0;
    int rows = 0;
    int frame_count = 0;
    bool loaded = false;

    std::unordered_map<std::string, Rectangle> regions;
};
