#include "font_manager.hpp"
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

FontManager::FontManager() {}

FontManager::~FontManager()
{
    unloadAll();
}

void FontManager::init()
{
    fonts.clear();
    FontEntry def;
    def.name = "Default";
    def.path = "";
    def.font = GetFontDefault();
    def.isDefault = true;
    fonts.push_back(def);
    defaultIndex = 0;

    scanDirectory("fonts");
}

void FontManager::unloadAll()
{
    for (auto& entry : fonts)
    {
        if (!entry.isDefault)
        {
            UnloadFont(entry.font);
        }
    }
    fonts.clear();
}

int FontManager::loadFont(const std::string& path)
{
    for (int i = 0; i < static_cast<int>(fonts.size()); i++)
    {
        if (fonts[i].path == path)
        {
            return i;
        }
    }

    if (!fileExists(path))
    {
        TraceLog(LOG_WARNING, "Font file not found: %s", path.c_str());
        return -1;
    }

    Font font = LoadFont(path.c_str());
    if (!font.texture.id)
    {
        TraceLog(LOG_WARNING, "Failed to load font: %s", path.c_str());
        return -1;
    }

    FontEntry entry;
    entry.name = extractName(path);
    entry.path = path;
    entry.font = font;
    entry.isDefault = false;
    fonts.push_back(entry);

    TraceLog(LOG_INFO, "Loaded font: %s (index %d)", entry.name.c_str(), (int)fonts.size() - 1);
    return static_cast<int>(fonts.size()) - 1;
}

void FontManager::removeFont(int index)
{
    if (index <= 0 || index >= static_cast<int>(fonts.size())) return;

    if (!fonts[index].isDefault)
    {
        UnloadFont(fonts[index].font);
    }
    fonts.erase(fonts.begin() + index);

    if (defaultIndex == index)
    {
        defaultIndex = 0;
    }
    else if (defaultIndex > index)
    {
        defaultIndex--;
    }
}

Font* FontManager::getFont(int index)
{
    if (index < 0 || index >= static_cast<int>(fonts.size()))
    {
        return &fonts[0].font;
    }
    return &fonts[index].font;
}

const std::string& FontManager::getFontName(int index) const
{
    if (index < 0 || index >= static_cast<int>(fonts.size()))
    {
        return fonts[0].name;
    }
    return fonts[index].name;
}

const std::string& FontManager::getFontPath(int index) const
{
    static const std::string empty;
    if (index < 0 || index >= static_cast<int>(fonts.size()))
    {
        return empty;
    }
    return fonts[index].path;
}

int FontManager::getCount() const
{
    return static_cast<int>(fonts.size());
}

int FontManager::getDefaultIndex() const
{
    return defaultIndex;
}

void FontManager::setDefaultFont(int index)
{
    if (index >= 0 && index < static_cast<int>(fonts.size()))
    {
        defaultIndex = index;
    }
}

json FontManager::toJson() const
{
    json data;
    data["default_font"] = defaultIndex;
    json fontsArr = json::array();
    for (int i = 1; i < static_cast<int>(fonts.size()); i++)
    {
        json f;
        f["name"] = fonts[i].name;
        f["path"] = fonts[i].path;
        fontsArr.push_back(f);
    }
    data["fonts"] = fontsArr;
    return data;
}

void FontManager::loadFromJson(const json& data)
{
    if (data.contains("fonts") && data["fonts"].is_array())
    {
        for (auto& f : data["fonts"])
        {
            if (f.contains("path"))
            {
                std::string path = f["path"].get<std::string>();
                int idx = loadFont(path);
                if (idx >= 0 && f.contains("name"))
                {
                    fonts[idx].name = f["name"].get<std::string>();
                }
            }
        }
    }

    if (data.contains("default_font"))
    {
        int def = data["default_font"].get<int>();
        if (def >= 0 && def < static_cast<int>(fonts.size()))
        {
            defaultIndex = def;
        }
    }
}

void FontManager::scanDirectory(const std::string& dir)
{
    if (!fs::exists(dir) || !fs::is_directory(dir)) return;

    for (auto& entry : fs::directory_iterator(dir))
    {
        if (!entry.is_regular_file()) continue;

        std::string ext = entry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        if (ext == ".ttf" || ext == ".otf")
        {
            loadFont(entry.path().string());
        }
    }
}

std::string FontManager::extractName(const std::string& path) const
{
    fs::path p(path);
    return p.stem().string();
}

bool FontManager::fileExists(const std::string& path) const
{
    return fs::exists(path) && fs::is_regular_file(path);
}
