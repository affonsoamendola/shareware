#include "spritesheet.hpp"

SpriteSheet::SpriteSheet(Texture2D texture, int frame_width_, int frame_height_)
    :   texture(texture), 
        frame_width(frame_width_), 
        frame_height(frame_height_), 
        loaded(true) 
{
    columns = texture.width / frame_width;
    rows = texture.height / frame_width;
    frame_count = columns * rows;
}

Sprite SpriteSheet::getFrame(int index)
{
    if (!loaded || index < 0 || index >= frame_count) return {};

    int col = index % columns;
    int row = index / columns;

    Rectangle src = 
    {
        static_cast<float>(col * frame_width),
        static_cast<float>(row * frame_height),
        static_cast<float>(frame_width),
        static_cast<float>(frame_height)
    };

    return Sprite(texture, src);
}

Sprite SpriteSheet::getFrame(int row, int col) 
{
    if (!loaded || row < 0 || row >= rows || col < 0 || col >= columns) return {};

    Rectangle src = 
    {
        static_cast<float>(col * frame_width),
        static_cast<float>(row * frame_height),
        static_cast<float>(frame_width),
        static_cast<float>(frame_height)
    };

    return Sprite(texture, src);
}

Sprite SpriteSheet::getNamedFrame(const std::string& name)
{
    auto it = regions.find(name);
    if (it == regions.end()) return {};
    return Sprite(texture, it->second);
}

void SpriteSheet::addRegion(const std::string& name, Rectangle rect) 
{
    regions[name] = rect;
}

int SpriteSheet::getFrameCount() { return frame_count; }
int SpriteSheet::getColumns() { return columns; }
int SpriteSheet::getRows() { return rows; }
bool SpriteSheet::isLoaded() { return loaded; }
