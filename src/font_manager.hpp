#pragma once

#include "raylib.h"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

using json = nlohmann::json;

struct FontEntry
{
    std::string name;
    std::string path;
    Font font;
    bool isDefault = false;
};

class FontManager
{
public:
    FontManager();
    ~FontManager();

    void init();
    void unloadAll();

    int loadFont(const std::string& path);
    void removeFont(int index);

    Font* getFont(int index);
    const std::string& getFontName(int index) const;
    const std::string& getFontPath(int index) const;
    int getCount() const;
    int getDefaultIndex() const;

    void setDefaultFont(int index);

    json toJson() const;
    void loadFromJson(const json& data);

    void scanDirectory(const std::string& dir);

private:
    std::vector<FontEntry> fonts;
    int defaultIndex = 0;

    std::string extractName(const std::string& path) const;
    bool fileExists(const std::string& path) const;
};
