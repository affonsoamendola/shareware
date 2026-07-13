#include "sprite.hpp"

Sprite::Sprite(Texture2D texture_)
    :   texture(texture_), 
        loaded(true) 
{
    source ={  
                0, 
                0, 
                static_cast<float>(texture.width), 
                static_cast<float>(texture.height)
            };
    UpdateDest();
}

Sprite::Sprite(Texture2D texture_, Rectangle source_)
    :   texture(texture_), 
        source(source_), 
        loaded(true) 
{
    UpdateDest();
}

void Sprite::draw()
{
    if (!loaded) return;

    Rectangle src = source;
    if (flip_h) src.width = -src.width;
    if (flip_v) src.height = -src.height;

    DrawTexturePro(texture, src, dest, origin, rotation, tint);
}

void Sprite::setScale(float sx, float sy) 
{
    scale = {sx, sy};
    UpdateDest();
}

void Sprite::setSourceRect(Rectangle source_) 
{
    source = source_;
    UpdateDest();
}

void Sprite::UpdateDest() {
    dest = {
        dest.x,
        dest.y,
        source.width * scale.x,
        source.height * scale.y
    };
}
