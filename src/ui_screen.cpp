#include "ui_screen.hpp"
#include "font_manager.hpp"
#include "pixel_scale.hpp"
#include "image_utils.hpp"
#include "bg_pattern.hpp"

Screen::Screen(const std::string& name_) : name(name_) {}

void Screen::addWidget(std::shared_ptr<Widget> widget_) 
{
    widgets.push_back(widget_);
}

void Screen::update(float dt) 
{
    if (scroll_direction != BgScrollDirection::None)
    {
        if (scroll_direction == BgScrollDirection::Horizontal ||
            scroll_direction == BgScrollDirection::Diagonal)
            scroll_offset_x += scroll_speed * dt;
        if (scroll_direction == BgScrollDirection::Vertical ||
            scroll_direction == BgScrollDirection::Diagonal)
            scroll_offset_y += scroll_speed * dt;
    }

    for (auto& widget : widgets) 
    {
        widget->update(dt);
    }
}

void Screen::draw(FontManager& fonts)
{
    ClearBackground(background_color);

    if (bg_pattern != BgPattern::None)
    {
        drawBackgroundPattern(bg_pattern, pattern_color_a, pattern_color_b,
            pattern_tile_size, GetVirtualScreenWidth(), GetVirtualScreenHeight(),
            scroll_offset_x, scroll_offset_y);
    }

    if (background_texture_loaded)
    {
        float bw = (float)GetVirtualScreenWidth();
        float bh = (float)GetVirtualScreenHeight();
        auto r = computeImageFit(
            (float)background_texture.width, (float)background_texture.height,
            0, 0, bw, bh, background_fit);
        DrawTexturePro(background_texture, r.src, r.dst, {0, 0}, 0.0f, WHITE);
    }

    for (auto& widget : widgets) 
    {
        widget->draw(fonts);
    }

    for (auto& widget : widgets) 
    {
        widget->drawOverlay(fonts);
    }
}

std::string Screen::getClickedAction() 
{
    for (auto& widget : widgets) 
    {
        if (widget->isClicked() && !widget->action.empty()) 
        {
            return widget->action;
        }
    }
    return "";
}

const std::string& Screen::getName()
{
    return name;
}

void Screen::setBackgroundColor(Color color)
{
    background_color = color;
}

Color Screen::getBackgroundColor() const
{
    return background_color;
}

void Screen::setBackgroundImage(const std::string& path)
{
    if (background_texture_loaded)
    {
        UnloadTexture(background_texture);
        background_texture_loaded = false;
    }

    background_image_path = path;

    if (!path.empty())
    {
        ::Image img = LoadImage(path.c_str());
        if (img.width > 0 && img.height > 0)
        {
            background_texture = LoadTextureFromImage(img);
            UnloadImage(img);
            background_texture_loaded = true;
        }
    }
}

const std::string& Screen::getBackgroundImagePath() const
{
    return background_image_path;
}

bool Screen::hasBackgroundImage() const
{
    return background_texture_loaded;
}

Texture2D Screen::getBackgroundTexture() const
{
    return background_texture;
}

void Screen::setBackgroundFit(ImageFit fit)
{
    background_fit = fit;
}

ImageFit Screen::getBackgroundFit() const
{
    return background_fit;
}

void Screen::setPattern(BgPattern pattern)
{
    bg_pattern = pattern;
}

BgPattern Screen::getPattern() const
{
    return bg_pattern;
}

void Screen::setPatternColorA(Color color)
{
    pattern_color_a = color;
}

Color Screen::getPatternColorA() const
{
    return pattern_color_a;
}

void Screen::setPatternColorB(Color color)
{
    pattern_color_b = color;
}

Color Screen::getPatternColorB() const
{
    return pattern_color_b;
}

void Screen::setPatternTileSize(int size)
{
    pattern_tile_size = size;
}

int Screen::getPatternTileSize() const
{
    return pattern_tile_size;
}

void Screen::setScrollDirection(BgScrollDirection dir)
{
    scroll_direction = dir;
}

BgScrollDirection Screen::getScrollDirection() const
{
    return scroll_direction;
}

void Screen::setScrollSpeed(float speed)
{
    scroll_speed = speed;
}

float Screen::getScrollSpeed() const
{
    return scroll_speed;
}

float Screen::getScrollOffsetX() const
{
    return scroll_offset_x;
}

float Screen::getScrollOffsetY() const
{
    return scroll_offset_y;
}

std::vector<std::shared_ptr<Widget>>& Screen::getWidgets()
{
    return widgets;
}

void Screen::removeWidget(size_t index)
{
    if (index < widgets.size())
    {
        widgets.erase(widgets.begin() + index);
    }
}

json Screen::toJson() const
{
    json j;
    j["background"] = {background_color.r, background_color.g, background_color.b, background_color.a};
    if (!background_image_path.empty())
    {
        j["background_image"] = background_image_path;
        const char* fitNames[] = {"stretch", "contain", "cover"};
        j["background_fit"] = fitNames[static_cast<int>(background_fit)];
    }
    if (bg_pattern != BgPattern::None)
    {
        j["bg_pattern"] = bgPatternName(bg_pattern);
        j["bg_pattern_color_a"] = {pattern_color_a.r, pattern_color_a.g, pattern_color_a.b, pattern_color_a.a};
        j["bg_pattern_color_b"] = {pattern_color_b.r, pattern_color_b.g, pattern_color_b.b, pattern_color_b.a};
        j["bg_pattern_tile_size"] = pattern_tile_size;
    }
    if (scroll_direction != BgScrollDirection::None)
    {
        const char* dirNames[] = {"none", "horizontal", "vertical", "diagonal"};
        j["bg_scroll_direction"] = dirNames[static_cast<int>(scroll_direction)];
        j["bg_scroll_speed"] = scroll_speed;
    }
    json widgetsArr = json::array();
    for (auto& widget : widgets)
    {
        widgetsArr.push_back(widget->toJson());
    }
    j["widgets"] = widgetsArr;
    return j;
}
