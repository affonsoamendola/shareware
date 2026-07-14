#include "ui_screen.hpp"
#include "font_manager.hpp"
#include "pixel_scale.hpp"
#include "image_utils.hpp"

Screen::Screen(const std::string& name_) : name(name_) {}

void Screen::addWidget(std::shared_ptr<Widget> widget_) 
{
    widgets.push_back(widget_);
}

void Screen::update(float dt) 
{
    for (auto& widget : widgets) 
    {
        widget->update(dt);
    }
}

void Screen::draw(FontManager& fonts)
{
    ClearBackground(background_color);

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
    json widgetsArr = json::array();
    for (auto& widget : widgets)
    {
        widgetsArr.push_back(widget->toJson());
    }
    j["widgets"] = widgetsArr;
    return j;
}
