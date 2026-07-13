#pragma once

#include "ui_widget.hpp"
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
};
