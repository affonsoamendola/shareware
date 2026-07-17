#pragma once

#include "ui_widget.hpp"
#include "bg_pattern.hpp"
#include <nlohmann/json.hpp>
#include <vector>
#include <memory>
#include <string>

using json = nlohmann::json;

class FontManager;

class Screen 
{
public:
    Screen() = default;
    Screen(const std::string& name);

    void addWidget(std::shared_ptr<Widget> widget);
    void update(float dt);
    void draw(FontManager& fonts);

    std::string getClickedAction();
    const std::string& getName();

    void setBackgroundColor(Color color);
    Color getBackgroundColor() const;

    void setBackgroundImage(const std::string& path);
    const std::string& getBackgroundImagePath() const;
    bool hasBackgroundImage() const;
    Texture2D getBackgroundTexture() const;

    void setBackgroundFit(ImageFit fit);
    ImageFit getBackgroundFit() const;

    void setPattern(BgPattern pattern);
    BgPattern getPattern() const;
    void setPatternColorA(Color color);
    Color getPatternColorA() const;
    void setPatternColorB(Color color);
    Color getPatternColorB() const;
    void setPatternTileSize(int size);
    int getPatternTileSize() const;

    void setScrollDirection(BgScrollDirection dir);
    BgScrollDirection getScrollDirection() const;
    void setScrollSpeed(float speed);
    float getScrollSpeed() const;
    float getScrollOffsetX() const;
    float getScrollOffsetY() const;

    std::vector<std::shared_ptr<Widget>>& getWidgets();
    void removeWidget(size_t index);

    json toJson() const;

    template<typename T, typename... Args>
    std::shared_ptr<T> createWidget(Args&&... args) 
    {
        auto widget = std::make_shared<T>(std::forward<Args>(args)...);
        widgets.push_back(widget);
        return widget;
    }

private:
    std::string name;
    std::vector<std::shared_ptr<Widget>> widgets;
    Color background_color = RAYWHITE;
    std::string background_image_path;
    Texture2D background_texture = {};
    bool background_texture_loaded = false;
    ImageFit background_fit = ImageFit::Stretch;
    BgPattern bg_pattern = BgPattern::None;
    Color pattern_color_a = {255, 165, 0, 255};
    Color pattern_color_b = {180, 100, 0, 255};
    int pattern_tile_size = 64;
    BgScrollDirection scroll_direction = BgScrollDirection::None;
    float scroll_speed = 60.0f;
    float scroll_offset_x = 0.0f;
    float scroll_offset_y = 0.0f;
};
