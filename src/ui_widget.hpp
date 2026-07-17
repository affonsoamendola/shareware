#pragma once

#include "raylib.h"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <functional>
#include "rich_text.hpp"

using json = nlohmann::json;

class FontManager;

enum class ImageFit { Stretch, Contain, Cover };

struct FrameAnimation
{
    std::vector<std::string> framePaths;
    std::vector<Texture2D> textures;
    float frameDuration = 0.1f;
    bool loop = true;
    bool loaded = false;

    void load();
    void unload();
    bool hasContent() const { return !framePaths.empty(); }
};

enum class WidgetType 
{
    Button,
    Label,
    Panel,
    Image,
    ImageViewer,
    RichTextBox
};

class Widget 
{
public:
    Widget() = default;
    virtual ~Widget() = default;

    virtual void update(float dt);
    virtual void draw(FontManager& fonts);
    virtual void drawOverlay(FontManager& fonts) {}

    virtual bool isClicked();
    virtual bool isHovered();

    void setPosition(float x, float y);
    void setSize(float w, float h);
    void setText(const std::string& text);
    void setVisible(bool visible);
    void setEnabled(bool enabled);

    Vector2 getPosition();
    Vector2 getSize();
    const std::string& getText();
    bool isVisible();
    bool isEnabled();
    WidgetType getType();

    virtual json toJson() const;

    static json colorToJson(Color c);

    std::string action;
    Color normal_color = DARKBLUE;
    Color hover_color = BLUE;
    Color pressed_color = LIGHTGRAY;
    Color text_color = WHITE;
    int font_size = 24;
    int font_index = 0;

protected:
    WidgetType type = WidgetType::Button;
    Rectangle bounds = {0, 0, 100, 40};
    std::string text;
    bool visible = true;
    bool enabled = true;
    bool hovered = false;
    bool pressed = false;
    bool clicked = false;
};

class Button : public Widget 
{
public:
    Button();
    Button(float x, float y, float w, float h, const std::string& text);

    void update(float dt) override;
    void draw(FontManager& fonts) override;
    json toJson() const override;

    void setIdleAnimation(const std::vector<std::string>& paths, float duration, bool loop);
    void setHoverAnimation(const std::vector<std::string>& paths, float duration, bool loop);
    void setClickAnimation(const std::vector<std::string>& paths, float duration, bool loop);
    void unloadAnimations();

    Color current_color = DARKBLUE;

    FrameAnimation idleAnim;
    FrameAnimation hoverAnim;
    FrameAnimation clickAnim;

    int animFrame = 0;
    float animTimer = 0.0f;
    enum class AnimState { Idle, Hover, Click };
    AnimState animState = AnimState::Idle;
};

class Label : public Widget 
{
public:
    Label();
    Label(float x, float y, const std::string& text, int size = 20);

    void update(float dt) override;
    void draw(FontManager& fonts) override;
    json toJson() const override;
};

class ImageWidget : public Widget
{
public:
    ImageWidget();
    ImageWidget(float x, float y, float w, float h, const std::string& path);
    ~ImageWidget();

    void update(float dt) override;
    void draw(FontManager& fonts) override;
    json toJson() const override;

    void setImagePath(const std::string& path);
    void setAnimation(const std::vector<std::string>& paths, float duration, bool loop);

    std::string imagePath;
    Color tint = WHITE;
    ImageFit fit = ImageFit::Stretch;
    Texture2D texture = {};
    bool textureLoaded = false;

    FrameAnimation anim;
    int animFrame = 0;
    float animTimer = 0.0f;
};

struct ImageViewerImage
{
    std::string path;
    Texture2D texture = {};
    bool loaded = false;
};

class ImageViewer : public Widget
{
public:
    ImageViewer();
    ImageViewer(float x, float y, float w, float h);
    ~ImageViewer();

    void update(float dt) override;
    void draw(FontManager& fonts) override;
    json toJson() const override;

    void addImage(const std::string& path);
    void removeImage(int index);
    void replaceImage(int index, const std::string& newPath);
    void clearImages();
    void setCurrentIndex(int index);

    std::vector<ImageViewerImage> images;
    int currentIndex = 0;
    Color tint = WHITE;
    ImageFit fit = ImageFit::Contain;

    bool isFullscreen() const;
    void setFullscreen(bool fs);

private:
    bool fullscreen = false;
    int hoveredButton = -1;
};

enum class TextAlign { Left, Center, Right, Justify };

class RichTextBox : public Widget
{
public:
    RichTextBox();
    RichTextBox(float x, float y, float w, float h);

    void update(float dt) override;
    void draw(FontManager& fonts) override;
    json toJson() const override;

    std::string toMarkdown() const;
    void fromMarkdown(const std::string& md);

    void toggleBold();
    void toggleItalic();
    void toggleUnderline();
    void toggleStrikethrough();
    void setBlockType(BlockType type);
    void setTextAlign(TextAlign align);

    std::vector<RichTextBlock> blocks;
    Color bgColor = {30, 30, 40, 255};
    Color borderColor = {80, 80, 100, 255};
    Color cursorColor = WHITE;
    Color selectionColor = {60, 100, 180, 100};
    Color toolbarBg = {40, 40, 55, 240};
    float lineHeight = 1.4f;
    int padding = 8;
    TextAlign textAlign = TextAlign::Left;

private:
    mutable int cursorBlock = 0;
    mutable int cursorRun = 0;
    mutable int cursorPos = 0;

    mutable int selBlock = -1;
    mutable int selRun = -1;
    mutable int selPos = -1;

    float scrollY = 0.0f;
    float contentHeight = 0.0f;
    Font* cachedFont = nullptr;

    void handleInput();
    void clampCursor() const;

    void insertChar(char c);
    void deleteChar(bool forward);
    void insertNewline();
    void backspace();

    void applyToSelection(bool bold, bool italic, bool underline, bool strikethrough);
    void toggleOnSelection(int flag);
    void splitRunAtCursor();
    void mergeAdjacentRuns();

    int getBlockY(int blockIndex, Font* font) const;
    void getCursorScreenPos(int& outX, int& outY, Font* font) const;
    void updateContentHeight(Font* font);

    void drawBlocks(FontManager& fonts);
};
