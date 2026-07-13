#include "sprite_loader.hpp"
#include "raylib.h"

SpriteLoader& SpriteLoader::Instance() 
{
    static SpriteLoader instance;
    return instance;
}

SpriteLoader::~SpriteLoader() 
{
    unloadAll();
}

Sprite SpriteLoader::loadSprite(const std::string& path) 
{
    Texture2D tex = loadOrGetTexture(path);
    return Sprite(tex);
}

Sprite SpriteLoader::loadSprite(const std::string& path, 
                                Rectangle source) 
{
    Texture2D tex = loadOrGetTexture(path);
    return Sprite(tex, source);
}

SpriteSheet 
SpriteLoader::loadSpriteSheet(  const std::string& path, 
                                int frameWidth, 
                                int frameHeight) 
{
    Texture2D tex = loadOrGetTexture(path);
    return SpriteSheet(tex, frameWidth, frameHeight);
}

void SpriteLoader::unloadTexture(const std::string& path) 
{
    auto it = textures.find(path);
    if (it != textures.end()) {
        ::UnloadTexture(it->second);
        textures.erase(it);
    }
}

void SpriteLoader::unloadAll() 
{
    for (auto& [path, tex] : textures) 
    {
        ::UnloadTexture(tex);
    }
    textures.clear();
}

Texture2D SpriteLoader::loadOrGetTexture(const std::string& path) 
{
    auto it = textures.find(path);
    if (it != textures.end()) 
    {
        return it->second;
    }

    Texture2D tex = LoadTexture(path.c_str());
    if (tex.id == 0) 
    {
        TraceLog(LOG_WARNING, "Failed to load texture: %s", path.c_str());
        return {};
    }

    textures[path] = tex;
    return tex;
}
