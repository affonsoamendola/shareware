#pragma once

#include "ui_screen.hpp"
#include "font_manager.hpp"
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <memory>
#include <functional>

using json = nlohmann::json;

enum class TransitionType {
    None,
    FadeOut,
    FadeIn
};

class UIManager {
public:
    UIManager();

    bool loadFromFile(const std::string& path);
    bool saveToFile(const std::string& path);
    json toJson() const;

    void transitionTo(const std::string& screenName);
    void update(float dt);
    void draw();

    FontManager& getFontManager();

    const std::string& getCurrentScreenName();
    bool isTransitioning();

    std::unordered_map<std::string, std::shared_ptr<Screen>>& getScreens();
    std::string& getInitialScreenRef();
    void addScreen(const std::string& name);
    void removeScreen(const std::string& name);
    void renameScreen(const std::string& oldName, const std::string& newName);

    std::function<void(const std::string&)> onAction;

private:
    void parseScreen(const std::string& name, const json& data);
    void parseWidget(Screen& screen, const json& data);

    Color parseColor(const json& data);

    FontManager fontManager;
    std::unordered_map<std::string, std::shared_ptr<Screen>> screens;
    std::string current_screen;
    std::string initial_screen;

    TransitionType transition = TransitionType::None;
    float transition_alpha = 0.0f;
    float transition_speed = 2.0f;
    std::string next_screen;
};
