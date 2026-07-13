#pragma once

#include "raylib.h"
#include "sprite.hpp"
#include "spritesheet.hpp"
#include <string>
#include <unordered_map>
#include <memory>

class SpriteLoader 
{
public:
    static SpriteLoader& Instance();

    Sprite loadSprite(const std::string& path);
    Sprite loadSprite(const std::string& path, Rectangle source);
    SpriteSheet loadSpriteSheet(const std::string& path, int frameWidth, int frameHeight);

    void unloadTexture(const std::string& path);
    void unloadAll();

private:
    SpriteLoader() = default;
    ~SpriteLoader();
    SpriteLoader(const SpriteLoader&) = delete;
    SpriteLoader& operator=(const SpriteLoader&) = delete;

    Texture2D loadOrGetTexture(const std::string& path);

    std::unordered_map<std::string, Texture2D> textures;
};
