#include "ui_manager.hpp"
#include "pixel_scale.hpp"
#include <fstream>
#include <iostream>

UIManager::UIManager() {}

bool UIManager::loadFromFile(const std::string& path) 
{
    fontManager.init();

    std::ifstream file(path);
    if (!file.is_open()) 
    {
        TraceLog(   LOG_WARNING, 
                    "Failed to open UI file: %s", 
                    path.c_str());
        return false;
    }

    json data;
    try 
    {
        data = json::parse(file);
    } 
    catch (const json::parse_error& e) 
    {
        TraceLog(LOG_WARNING, "JSON parse error: %s", e.what());
        return false;
    }

    if (data.contains("initial_screen")) 
    {
        initial_screen = data["initial_screen"].get<std::string>();
    }

    if (data.contains("fonts"))
    {
        fontManager.loadFromJson(data);
    }

    if (data.contains("screens")) 
    {
        for (   auto& [name, 
                screenData] : data["screens"].items()) 
        {
            parseScreen(name, screenData);
        }
    }

    if (!initial_screen.empty() && screens.count(initial_screen)) 
    {
        current_screen = initial_screen;
    } else if (screens.empty()) {
        current_screen = screens.begin()->first;
    }

    return true;
}

bool UIManager::saveToFile(const std::string& path)
{
    json data = toJson();
    std::ofstream file(path);
    if (!file.is_open())
    {
        TraceLog(LOG_WARNING, "Failed to save UI file: %s", path.c_str());
        return false;
    }
    file << data.dump(4);
    file.close();
    TraceLog(LOG_INFO, "UI saved to %s", path.c_str());
    return true;
}

json UIManager::toJson() const
{
    json data;
    data["initial_screen"] = initial_screen;
    data["fonts"] = fontManager.toJson();
    json screensObj = json::object();
    for (auto& [name, screen] : screens)
    {
        screensObj[name] = screen->toJson();
    }
    data["screens"] = screensObj;
    return data;
}

std::unordered_map<std::string, std::shared_ptr<Screen>>& UIManager::getScreens()
{
    return screens;
}

std::string& UIManager::getInitialScreenRef()
{
    return initial_screen;
}

void UIManager::addScreen(const std::string& name)
{
    if (!screens.count(name))
    {
        screens[name] = std::make_shared<Screen>(name);
        if (initial_screen.empty())
        {
            initial_screen = name;
        }
    }
}

void UIManager::removeScreen(const std::string& name)
{
    screens.erase(name);
    if (current_screen == name && !screens.empty())
    {
        current_screen = screens.begin()->first;
    }
    if (initial_screen == name && !screens.empty())
    {
        initial_screen = screens.begin()->first;
    }
}

void UIManager::renameScreen(const std::string& oldName, const std::string& newName)
{
    if (screens.count(oldName) && !screens.count(newName))
    {
        auto screen = screens[oldName];
        screens.erase(oldName);
        screens[newName] = screen;
        if (current_screen == oldName) current_screen = newName;
        if (initial_screen == oldName) initial_screen = newName;
    }
}

void UIManager::parseScreen(const std::string& name, const json& data) {
    auto screen = std::make_shared<Screen>(name);

    if (data.contains("background")) 
    {
        screen->setBackgroundColor(parseColor(data["background"]));
    }

    if (data.contains("background_image"))
    {
        screen->setBackgroundImage(data["background_image"].get<std::string>());
    }

    if (data.contains("background_fit"))
    {
        std::string fitStr = data["background_fit"].get<std::string>();
        if (fitStr == "contain") screen->setBackgroundFit(ImageFit::Contain);
        else if (fitStr == "cover") screen->setBackgroundFit(ImageFit::Cover);
        else screen->setBackgroundFit(ImageFit::Stretch);
    }

    if (data.contains("widgets")) 
    {
        for (auto& widgetData : data["widgets"]) 
        {
            parseWidget(*screen, widgetData);
        }
    }

    screens[name] = screen;
}

void UIManager::parseWidget(Screen& screen, const json& data) 
{
    std::string type = data.value("type", "button");

    if (type == "button") 
    {
        auto btn = screen.createWidget<Button>
        (
            data.value("x", 0.0f),
            data.value("y", 0.0f),
            data.value("width", 200.0f),
            data.value("height", 50.0f),
            data.value("text", "Button")
        );

        btn->action = data.value("action", "");
        btn->font_size = data.value("font_size", 20);
        btn->font_index = data.value("font_index", 0);

        if (data.contains("color")) btn->normal_color = parseColor(data["color"]);
        if (data.contains("hover_color")) btn->hover_color = parseColor(data["hover_color"]);
        if (data.contains("pressed_color")) btn->pressed_color = parseColor(data["pressed_color"]);
        if (data.contains("text_color")) btn->text_color = parseColor(data["text_color"]);

    } 
    else if (type == "label") 
    {
        auto lbl = screen.createWidget<Label>
        (
            data.value("x", 0.0f),
            data.value("y", 0.0f),
            data.value("text", "Label"),
            data.value("font_size", 20)
        );

        lbl->font_index = data.value("font_index", 0);
        if (data.contains("text_color")) lbl->text_color = parseColor(data["text_color"]);
    }
    else if (type == "image")
    {
        auto img = screen.createWidget<ImageWidget>(
            data.value("x", 0.0f),
            data.value("y", 0.0f),
            data.value("width", 100.0f),
            data.value("height", 100.0f),
            data.value("image_path", "")
        );

        if (data.contains("tint")) img->tint = parseColor(data["tint"]);

        if (data.contains("fit"))
        {
            std::string fitStr = data["fit"].get<std::string>();
            if (fitStr == "contain") img->fit = ImageFit::Contain;
            else if (fitStr == "cover") img->fit = ImageFit::Cover;
            else img->fit = ImageFit::Stretch;
        }
    }
    else if (type == "richtextbox")
    {
        auto rtb = screen.createWidget<RichTextBox>(
            data.value("x", 0.0f),
            data.value("y", 0.0f),
            data.value("width", 400.0f),
            data.value("height", 300.0f)
        );

        rtb->font_size = data.value("font_size", 16);
        rtb->font_index = data.value("font_index", 0);
        if (data.contains("text_color")) rtb->text_color = parseColor(data["text_color"]);
        if (data.contains("bg_color")) rtb->bgColor = parseColor(data["bg_color"]);
        if (data.contains("border_color")) rtb->borderColor = parseColor(data["border_color"]);
        rtb->lineHeight = data.value("line_spacing", 1.4f);
        rtb->padding = data.value("padding", 8);
        rtb->textAlign = (TextAlign)data.value("text_align", 0);

        if (data.contains("markdown_content"))
        {
            rtb->fromMarkdown(data["markdown_content"].get<std::string>());
        }
    }
}

Color UIManager::parseColor(const json& data) 
{
    if (data.is_array() && data.size() >= 4) {
        return {
            static_cast<unsigned char>(data[0].get<int>()),
            static_cast<unsigned char>(data[1].get<int>()),
            static_cast<unsigned char>(data[2].get<int>()),
            static_cast<unsigned char>(data[3].get<int>())
        };
    }
    return WHITE;
}

void UIManager::transitionTo(const std::string& screenName) 
{
    if (!screens.count(screenName)) return;
    if (transition != TransitionType::None) return;

    next_screen = screenName;
    transition = TransitionType::FadeOut;
    transition_alpha = 0.0f;
}

void UIManager::update(float dt) 
{
    if (transition != TransitionType::None) 
    {
        if(transition == TransitionType::FadeOut)
        {
            transition_alpha += transition_speed * dt;
        }
        else if(transition == TransitionType::FadeIn)
        {
            transition_alpha -= transition_speed * dt;
        }
            
        if (transition == TransitionType::FadeOut && 
            transition_alpha >= 1.0f) 
        {
            current_screen = next_screen;
            transition = TransitionType::FadeIn;
            transition_alpha = 1.0f;
        } 
        else if (   transition == TransitionType::FadeIn && 
                    transition_alpha <= 0.0f) 
        {
            transition = TransitionType::None;
            transition_alpha = 0.0f;
        }

        return;
    }

    if (screens.count(current_screen)) 
    {
        screens[current_screen]->update(dt);

        std::string action = screens[current_screen]->getClickedAction();
        if (!action.empty()) 
        {
            if (action.rfind("goto:", 0) == 0) 
            {
                std::string target = action.substr(5);
                transitionTo(target);
            }
            if (onAction) 
            {
                onAction(action);
            }
        }
    }
}

void UIManager::draw() {
    if (screens.count(current_screen)) 
    {
        screens[current_screen]->draw(fontManager);
    }

    if (transition != TransitionType::None) 
    {
        unsigned char alpha = static_cast<unsigned char>(transition_alpha * 255);
        DrawRectangle(0, 0, GetVirtualScreenWidth(), GetVirtualScreenHeight(), {0, 0, 0, alpha});
    }
}

const std::string& UIManager::getCurrentScreenName()
{
    return current_screen;
}

bool UIManager::isTransitioning()
{
    return transition != TransitionType::None;
}

FontManager& UIManager::getFontManager()
{
    return fontManager;
}
