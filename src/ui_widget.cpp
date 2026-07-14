#include "ui_widget.hpp"
#include "font_manager.hpp"
#include "pixel_scale.hpp"
#include "image_utils.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cctype>

// Widget base
void Widget::update(float dt) 
{
    if (!visible || !enabled) return;

    Vector2 mouse = GetVirtualMousePos();
    hovered = CheckCollisionPointRec(mouse, bounds);
    pressed = hovered && IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    clicked = hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

void Widget::draw(FontManager& fonts) 
{
    if (!visible) return;
}

bool Widget::isClicked()
{
    return clicked && enabled;
}

bool Widget::isHovered()
{
    return hovered;
}

void Widget::setPosition(float x, float y)
{
    bounds.x = x;
    bounds.y = y;
}

void Widget::setSize(float w, float h)
{
    bounds.width = w;
    bounds.height = h;
}

void Widget::setText(const std::string& text_)
{
    text = text_;
}

void Widget::setVisible(bool visible_)
{
    visible = visible_;
}

void Widget::setEnabled(bool enabled_)
{
    enabled = enabled_;
}

Vector2 Widget::getPosition()
{
    return {bounds.x, bounds.y};
}

Vector2 Widget::getSize()
{
    return {bounds.width, bounds.height};
}

const std::string& Widget::getText()
{
    return text;
}

bool Widget::isVisible()
{
    return visible;
}

bool Widget::isEnabled()
{
    return enabled;
}

WidgetType Widget::getType()
{
    return type;
}

json Widget::toJson() const
{
    json j;
    j["x"] = bounds.x;
    j["y"] = bounds.y;
    j["text"] = text;
    j["font_size"] = font_size;
    j["font_index"] = font_index;
    j["text_color"] = colorToJson(text_color);
    return j;
}

json Widget::colorToJson(Color color)
{
    return {color.r, color.g, color.b, color.a};
}

// FrameAnimation
void FrameAnimation::load()
{
    unload();
    if (framePaths.empty()) return;

    for (const auto& path : framePaths)
    {
        if (path.empty()) continue;
        ::Image img = LoadImage(path.c_str());
        if (img.width > 0 && img.height > 0)
        {
            textures.push_back(LoadTextureFromImage(img));
            UnloadImage(img);
        }
    }
    loaded = !textures.empty();
}

void FrameAnimation::unload()
{
    for (auto& tex : textures)
    {
        UnloadTexture(tex);
    }
    textures.clear();
    loaded = false;
}

// Button
static const FrameAnimation* buttonGetAnim(const Button& btn, Button::AnimState state)
{
    switch (state)
    {
        case Button::AnimState::Idle:   return &btn.idleAnim;
        case Button::AnimState::Hover:  return &btn.hoverAnim;
        case Button::AnimState::Click:  return &btn.clickAnim;
    }
    return nullptr;
}
Button::Button() 
{
    type = WidgetType::Button;
    current_color = normal_color;
}

Button::Button(float x, float y, float w, float h, const std::string& text_) 
{
    type = WidgetType::Button;
    bounds = {x, y, w, h};
    text = text_;
    current_color = normal_color;
}

void Button::update(float dt) 
{
    Widget::update(dt);
    if (!visible || !enabled) return;

    AnimState desiredState;
    if (pressed) desiredState = AnimState::Click;
    else if (hovered) desiredState = AnimState::Hover;
    else desiredState = AnimState::Idle;

    if (desiredState != animState)
    {
        animState = desiredState;
        animFrame = 0;
        animTimer = 0.0f;
    }

    const FrameAnimation* anim = buttonGetAnim(*this, animState);
    if (anim && anim->loaded && anim->textures.size() > 1)
    {
        animTimer += dt;
        if (animTimer >= anim->frameDuration)
        {
            animTimer -= anim->frameDuration;
            animFrame++;
            int total = static_cast<int>(anim->textures.size());
            if (animFrame >= total)
                animFrame = anim->loop ? 0 : total - 1;
        }
    }
    else
    {
        animFrame = 0;
    }

    if (pressed)      current_color = pressed_color;
    else if (hovered) current_color = hover_color;
    else              current_color = normal_color;
}

void Button::draw(FontManager& fonts)
{
    if (!visible) return;

    const FrameAnimation* anim = buttonGetAnim(*this, animState);
    bool drewImage = false;

    if (anim && anim->loaded && !anim->textures.empty())
    {
        int frameIdx = animFrame;
        if (frameIdx >= static_cast<int>(anim->textures.size()))
            frameIdx = 0;
        const Texture2D& tex = anim->textures[frameIdx];
        Rectangle src = {0, 0, (float)tex.width, (float)tex.height};
        Rectangle dst = {bounds.x, bounds.y, bounds.width, bounds.height};
        DrawTexturePro(tex, src, dst, {0, 0}, 0.0f, WHITE);
        drewImage = true;
    }

    if (!drewImage)
    {
        DrawRectangleRounded(bounds, 0.3f, 8, current_color);

        Font* font = fonts.getFont(font_index);
        int text_width = MeasureTextEx(*font, text.c_str(), (float)font_size, 1).x;
        float textX = bounds.x + (bounds.width - text_width) / 2;
        float textY = bounds.y + (bounds.height - font_size) / 2;
        DrawTextEx(*font, text.c_str(), {textX, textY}, (float)font_size, 1, text_color);
    }
}

void Button::setIdleAnimation(const std::vector<std::string>& paths, float duration, bool loop)
{
    idleAnim.framePaths = paths;
    idleAnim.frameDuration = duration;
    idleAnim.loop = loop;
    idleAnim.load();
}

void Button::setHoverAnimation(const std::vector<std::string>& paths, float duration, bool loop)
{
    hoverAnim.framePaths = paths;
    hoverAnim.frameDuration = duration;
    hoverAnim.loop = loop;
    hoverAnim.load();
}

void Button::setClickAnimation(const std::vector<std::string>& paths, float duration, bool loop)
{
    clickAnim.framePaths = paths;
    clickAnim.frameDuration = duration;
    clickAnim.loop = loop;
    clickAnim.load();
}

void Button::unloadAnimations()
{
    idleAnim.unload();
    hoverAnim.unload();
    clickAnim.unload();
}

json Button::toJson() const
{
    json j = Widget::toJson();
    j["type"] = "button";
    j["width"] = bounds.width;
    j["height"] = bounds.height;
    if (!action.empty()) j["action"] = action;
    j["color"] = colorToJson(normal_color);
    j["hover_color"] = colorToJson(hover_color);
    j["pressed_color"] = colorToJson(pressed_color);

    if (idleAnim.hasContent())
    {
        json a;
        a["frames"] = idleAnim.framePaths;
        a["frame_duration"] = idleAnim.frameDuration;
        a["loop"] = idleAnim.loop;
        j["idle_animation"] = a;
    }
    if (hoverAnim.hasContent())
    {
        json a;
        a["frames"] = hoverAnim.framePaths;
        a["frame_duration"] = hoverAnim.frameDuration;
        a["loop"] = hoverAnim.loop;
        j["hover_animation"] = a;
    }
    if (clickAnim.hasContent())
    {
        json a;
        a["frames"] = clickAnim.framePaths;
        a["frame_duration"] = clickAnim.frameDuration;
        a["loop"] = clickAnim.loop;
        j["click_animation"] = a;
    }
    return j;
}

// Label
Label::Label() 
{
    type = WidgetType::Label;
}

Label::Label(float x, float y, const std::string& text_, int size) 
{
    type = WidgetType::Label;
    bounds = {x, y, 0, 0};
    text = text_;
    font_size = size;
}

void Label::update(float dt) 
{
    Widget::update(dt);
}

void Label::draw(FontManager& fonts)
{
    if (!visible) return;
    Font* font = fonts.getFont(font_index);
    DrawTextEx( *font,
                text.c_str(), 
                {bounds.x, bounds.y}, 
                (float)font_size, 
                1,
                text_color);
}

json Label::toJson() const
{
    json j = Widget::toJson();
    j["type"] = "label";
    return j;
}

// ImageWidget
ImageWidget::ImageWidget()
{
    type = WidgetType::Image;
}

ImageWidget::ImageWidget(float x, float y, float w, float h, const std::string& path)
{
    type = WidgetType::Image;
    bounds = {x, y, w, h};
    setImagePath(path);
}

ImageWidget::~ImageWidget()
{
    if (textureLoaded)
    {
        UnloadTexture(texture);
        textureLoaded = false;
    }
    anim.unload();
}

void ImageWidget::update(float dt)
{
    Widget::update(dt);

    if (anim.loaded && anim.textures.size() > 1)
    {
        animTimer += dt;
        if (animTimer >= anim.frameDuration)
        {
            animTimer -= anim.frameDuration;
            animFrame++;
            int total = static_cast<int>(anim.textures.size());
            if (animFrame >= total)
                animFrame = anim.loop ? 0 : total - 1;
        }
    }
}

void ImageWidget::draw(FontManager& fonts)
{
    if (!visible) return;

    if (anim.loaded && !anim.textures.empty())
    {
        int frameIdx = animFrame;
        if (frameIdx >= static_cast<int>(anim.textures.size()))
            frameIdx = 0;
        const Texture2D& tex = anim.textures[frameIdx];
        auto r = computeImageFit(
            (float)tex.width, (float)tex.height,
            bounds.x, bounds.y, bounds.width, bounds.height, fit);
        DrawTexturePro(tex, r.src, r.dst, {0, 0}, 0.0f, tint);
    }
    else if (textureLoaded)
    {
        auto r = computeImageFit(
            (float)texture.width, (float)texture.height,
            bounds.x, bounds.y, bounds.width, bounds.height, fit);
        DrawTexturePro(texture, r.src, r.dst, {0, 0}, 0.0f, tint);
    }
    else
    {
        DrawRectangleRec(bounds, {60, 60, 60, 255});
        DrawRectangleLinesEx(bounds, 1, DARKGRAY);
        DrawText("No Image",
            static_cast<int>(bounds.x + bounds.width / 2 - 30),
            static_cast<int>(bounds.y + bounds.height / 2 - 6),
            10, LIGHTGRAY);
    }
}

void ImageWidget::setImagePath(const std::string& path)
{
    if (textureLoaded)
    {
        UnloadTexture(texture);
        textureLoaded = false;
    }

    imagePath = path;

    if (!path.empty())
    {
        ::Image img = LoadImage(path.c_str());
        if (img.width > 0 && img.height > 0)
        {
            texture = LoadTextureFromImage(img);
            UnloadImage(img);
            textureLoaded = true;
        }
    }
}

void ImageWidget::setAnimation(const std::vector<std::string>& paths, float duration, bool loop)
{
    anim.framePaths = paths;
    anim.frameDuration = duration;
    anim.loop = loop;
    anim.load();
    animFrame = 0;
    animTimer = 0.0f;
}

json ImageWidget::toJson() const
{
    json j = Widget::toJson();
    j["type"] = "image";
    j["width"] = bounds.width;
    j["height"] = bounds.height;
    j["image_path"] = imagePath;
    j["tint"] = colorToJson(tint);
    const char* fitNames[] = {"stretch", "contain", "cover"};
    j["fit"] = fitNames[static_cast<int>(fit)];
    if (anim.hasContent())
    {
        json a;
        a["frames"] = anim.framePaths;
        a["frame_duration"] = anim.frameDuration;
        a["loop"] = anim.loop;
        j["animation"] = a;
    }
    return j;
}

// ImageViewer
ImageViewer::ImageViewer()
{
    type = WidgetType::ImageViewer;
}

ImageViewer::ImageViewer(float x, float y, float w, float h)
{
    type = WidgetType::ImageViewer;
    bounds = {x, y, w, h};
}

ImageViewer::~ImageViewer()
{
    for (auto& img : images)
    {
        if (img.loaded)
        {
            UnloadTexture(img.texture);
            img.loaded = false;
        }
    }
}

void ImageViewer::update(float dt)
{
    Widget::update(dt);

    if (!visible || !enabled) return;
    if (fullscreen)
    {
        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER))
        {
            fullscreen = false;
        }
        if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D))
        {
            if (!images.empty())
                currentIndex = (currentIndex + 1) % (int)images.size();
        }
        if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A))
        {
            if (!images.empty())
                currentIndex = (currentIndex - 1 + (int)images.size()) % (int)images.size();
        }
        return;
    }

    hoveredButton = -1;
    Vector2 mouse = GetVirtualMousePos();

    float btnSize = 28.0f;
    float btnY = bounds.y + bounds.height - btnSize - 4.0f;
    float totalBtnW = btnSize * 2 + 8.0f;
    float btnStartX = bounds.x + (bounds.width - totalBtnW) / 2.0f;

    Rectangle prevBtn = {btnStartX, btnY, btnSize, btnSize};
    Rectangle nextBtn = {btnStartX + btnSize + 8.0f, btnY, btnSize, btnSize};

    bool prevHover = CheckCollisionPointRec(mouse, prevBtn);
    bool nextHover = CheckCollisionPointRec(mouse, nextBtn);

    if (prevHover) hoveredButton = 0;
    if (nextHover) hoveredButton = 1;

    if (prevHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !images.empty())
    {
        currentIndex = (currentIndex - 1 + (int)images.size()) % (int)images.size();
    }
    if (nextHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !images.empty())
    {
        currentIndex = (currentIndex + 1) % (int)images.size();
    }

    Rectangle imageArea = {bounds.x, bounds.y, bounds.width, bounds.height - btnSize - 8.0f};
    if (CheckCollisionPointRec(mouse, imageArea) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !images.empty())
    {
        fullscreen = true;
    }
}

void ImageViewer::draw(FontManager& fonts)
{
    if (!visible) return;

    if (fullscreen)
    {
        float screenW = (float)GetScreenWidth();
        float screenH = (float)GetScreenHeight();
        DrawRectangle(0, 0, (int)screenW, (int)screenH, {0, 0, 0, 230});

        if (!images.empty() && images[currentIndex].loaded)
        {
            auto& tex = images[currentIndex].texture;
            auto r = computeImageFit(
                (float)tex.width, (float)tex.height,
                20.0f, 20.0f, screenW - 40.0f, screenH - 40.0f, fit);
            DrawTexturePro(tex, r.src, r.dst, {0, 0}, 0.0f, tint);
        }

        float fsBtnSize = 36.0f;
        float fsBtnY = screenH - fsBtnSize - 16.0f;
        float fsTotalBtnW = fsBtnSize * 2 + 12.0f;
        float fsBtnStartX = (screenW - fsTotalBtnW) / 2.0f;

        Rectangle fsPrevBtn = {fsBtnStartX, fsBtnY, fsBtnSize, fsBtnSize};
        Rectangle fsNextBtn = {fsBtnStartX + fsBtnSize + 12.0f, fsBtnY, fsBtnSize, fsBtnSize};
        Vector2 mouse = GetVirtualMousePos();

        for (int i = 0; i < 2; i++)
        {
            Rectangle& btn = (i == 0) ? fsPrevBtn : fsNextBtn;
            bool hover = CheckCollisionPointRec(mouse, btn);
            Color bg = hover ? Color{100, 100, 120, 200} : Color{60, 60, 80, 180};
            DrawRectangleRounded(btn, 0.3f, 4, bg);

            const char* label = (i == 0) ? "<" : ">";
            int labelW = MeasureText(label, 16);
            DrawText(label,
                (int)(btn.x + (btn.width - labelW) / 2),
                (int)(btn.y + (btn.height - 16) / 2),
                16, WHITE);
        }

        if (!images.empty())
        {
            std::string counter = std::to_string(currentIndex + 1) + " / " + std::to_string((int)images.size());
            int counterW = MeasureText(counter.c_str(), 14);
            DrawText(counter.c_str(), (int)(screenW - counterW - 20), (int)(screenH - 30), 14, WHITE);
        }

        const char* closeText = "[ESC] Close";
        int closeW = MeasureText(closeText, 12);
        DrawText(closeText, (int)(screenW - closeW - 20), 20, 12, {200, 200, 200, 180});
        return;
    }

    if (!images.empty() && currentIndex >= 0 && currentIndex < (int)images.size() &&
        images[currentIndex].loaded)
    {
        auto& tex = images[currentIndex].texture;
        float imageH = bounds.height - 36.0f;
        auto r = computeImageFit(
            (float)tex.width, (float)tex.height,
            bounds.x, bounds.y, bounds.width, imageH, fit);
        DrawTexturePro(tex, r.src, r.dst, {0, 0}, 0.0f, tint);
    }
    else
    {
        DrawRectangleRec(bounds, {60, 60, 60, 255});
        DrawRectangleLinesEx(bounds, 1, DARKGRAY);
        DrawText("No Images",
            (int)(bounds.x + bounds.width / 2 - 35),
            (int)(bounds.y + bounds.height / 2 - 6),
            10, LIGHTGRAY);
    }

    float btnSize = 28.0f;
    float btnY = bounds.y + bounds.height - btnSize - 4.0f;
    float totalBtnW = btnSize * 2 + 8.0f;
    float btnStartX = bounds.x + (bounds.width - totalBtnW) / 2.0f;

    Vector2 mouse = GetVirtualMousePos();

    for (int i = 0; i < 2; i++)
    {
        float bx = btnStartX + i * (btnSize + 8.0f);
        Rectangle btn = {bx, btnY, btnSize, btnSize};
        bool hover = (hoveredButton == i);
        Color bg = hover ? Color{100, 100, 120, 255} : Color{60, 60, 80, 255};
        DrawRectangleRounded(btn, 0.3f, 4, bg);

        const char* label = (i == 0) ? "<" : ">";
        int labelW = MeasureText(label, 14);
        DrawText(label,
            (int)(bx + (btnSize - labelW) / 2),
            (int)(btnY + (btnSize - 14) / 2),
            14, WHITE);
    }

    if (!images.empty())
    {
        std::string counter = std::to_string(currentIndex + 1) + " / " + std::to_string((int)images.size());
        int counterW = MeasureText(counter.c_str(), 10);
        DrawText(counter.c_str(),
            (int)(bounds.x + (bounds.width - counterW) / 2),
            (int)(btnY + btnSize + 2),
            10, LIGHTGRAY);
    }
}

void ImageViewer::addImage(const std::string& path)
{
    ImageViewerImage img;
    img.path = path;

    if (!path.empty())
    {
        ::Image rawImg = LoadImage(path.c_str());
        if (rawImg.width > 0 && rawImg.height > 0)
        {
            img.texture = LoadTextureFromImage(rawImg);
            UnloadImage(rawImg);
            img.loaded = true;
        }
    }

    images.push_back(img);
}

void ImageViewer::removeImage(int index)
{
    if (index >= 0 && index < (int)images.size())
    {
        if (images[index].loaded)
        {
            UnloadTexture(images[index].texture);
        }
        images.erase(images.begin() + index);
        if (currentIndex >= (int)images.size())
            currentIndex = (int)images.size() - 1;
        if (currentIndex < 0) currentIndex = 0;
    }
}

void ImageViewer::replaceImage(int index, const std::string& newPath)
{
    if (index >= 0 && index < (int)images.size())
    {
        if (images[index].loaded)
            UnloadTexture(images[index].texture);

        images[index].path = newPath;
        Texture2D tex = LoadTexture(newPath.c_str());
        if (tex.id > 0)
        {
            images[index].texture = tex;
            images[index].loaded = true;
        }
        else
        {
            images[index].loaded = false;
        }
    }
}

void ImageViewer::clearImages()
{
    for (auto& img : images)
    {
        if (img.loaded)
        {
            UnloadTexture(img.texture);
        }
    }
    images.clear();
    currentIndex = 0;
}

void ImageViewer::setCurrentIndex(int index)
{
    if (index >= 0 && index < (int)images.size())
        currentIndex = index;
}

bool ImageViewer::isFullscreen() const
{
    return fullscreen;
}

void ImageViewer::setFullscreen(bool fs)
{
    fullscreen = fs;
}

json ImageViewer::toJson() const
{
    json j = Widget::toJson();
    j["type"] = "imageviewer";
    j["width"] = bounds.width;
    j["height"] = bounds.height;
    j["tint"] = colorToJson(tint);
    const char* fitNames[] = {"stretch", "contain", "cover"};
    j["fit"] = fitNames[static_cast<int>(fit)];

    json paths = json::array();
    for (auto& img : images)
    {
        paths.push_back(img.path);
    }
    j["images"] = paths;
    j["current_index"] = currentIndex;

    return j;
}

// ====================== RichTextBox ======================

RichTextBox::RichTextBox()
{
    type = WidgetType::RichTextBox;
    bounds = {0, 0, 400, 300};
    RichTextBlock block;
    block.type = BlockType::Paragraph;
    RichTextRun run;
    run.text = "";
    block.runs.push_back(run);
    blocks.push_back(block);
}

RichTextBox::RichTextBox(float x, float y, float w, float h)
{
    type = WidgetType::RichTextBox;
    bounds = {x, y, w, h};
    RichTextBlock block;
    block.type = BlockType::Paragraph;
    RichTextRun run;
    run.text = "";
    block.runs.push_back(run);
    blocks.push_back(block);
}

bool RichTextBox::isFocused() const
{
    return focused;
}

void RichTextBox::setFocused(bool isFocused)
{
    focused = isFocused;
    toolbarVisible = isFocused;
}

bool RichTextBox::isToolbarVisible() const
{
    return toolbarVisible;
}

void RichTextBox::clampCursor() const
{
    if (blocks.empty()) return;
    if (cursorBlock < 0) cursorBlock = 0;
    if (cursorBlock >= (int)blocks.size()) cursorBlock = (int)blocks.size() - 1;

    auto& runs = blocks[cursorBlock].runs;
    if (runs.empty()) return;
    if (cursorRun < 0) cursorRun = 0;
    if (cursorRun >= (int)runs.size()) cursorRun = (int)runs.size() - 1;

    int maxPos = (int)runs[cursorRun].text.size();
    if (cursorPos < 0) cursorPos = 0;
    if (cursorPos > maxPos) cursorPos = maxPos;
}

// Split current run at cursor, returns the run index after split
void RichTextBox::splitRunAtCursor()
{
    clampCursor();
    auto& runs = blocks[cursorBlock].runs;
    auto& run = runs[cursorRun];
    int pos = cursorPos;

    if (pos > 0 && pos < (int)run.text.size())
    {
        RichTextRun newRun = run;
        run.text = run.text.substr(0, pos);
        newRun.text = newRun.text.substr(pos);
        runs.insert(runs.begin() + cursorRun + 1, newRun);
        cursorRun++;
        cursorPos = 0;
    }
    else if (pos == 0 && cursorRun > 0)
    {
        cursorRun--;
        cursorPos = (int)runs[cursorRun].text.size();
    }
}

void RichTextBox::mergeAdjacentRuns()
{
    auto& runs = blocks[cursorBlock].runs;
    for (int i = 0; i + 1 < (int)runs.size(); i++)
    {
        auto& a = runs[i];
        auto& b = runs[i + 1];
        if (a.bold == b.bold && a.italic == b.italic &&
            a.underline == b.underline && a.strikethrough == b.strikethrough &&
            a.textColor.r == b.textColor.r && a.textColor.g == b.textColor.g &&
            a.textColor.b == b.textColor.b && a.textColor.a == b.textColor.a &&
            a.highlightColor.r == b.highlightColor.r &&
            a.highlightColor.g == b.highlightColor.g &&
            a.highlightColor.b == b.highlightColor.b &&
            a.highlightColor.a == b.highlightColor.a)
        {
            a.text += b.text;
            runs.erase(runs.begin() + i + 1);
            i--;
        }
    }
}

void RichTextBox::insertChar(char c)
{
    clampCursor();
    splitRunAtCursor();
    auto& runs = blocks[cursorBlock].runs;
    auto& run = runs[cursorRun];
    run.text.insert(run.text.begin() + cursorPos, c);
    cursorPos++;
    mergeAdjacentRuns();
}

void RichTextBox::deleteChar(bool forward)
{
    clampCursor();
    auto& runs = blocks[cursorBlock].runs;

    if (forward)
    {
        auto& text = runs[cursorRun].text;
        if (cursorPos < (int)text.size())
        {
            text.erase(cursorPos, 1);
        }
        else if (cursorRun + 1 < (int)runs.size())
        {
            runs[cursorRun].text += runs[cursorRun + 1].text;
            runs.erase(runs.begin() + cursorRun + 1);
        }
        else if (cursorBlock + 1 < (int)blocks.size())
        {
            auto& nextRuns = blocks[cursorBlock + 1].runs;
            runs[cursorRun].text += nextRuns[0].text;
            blocks.erase(blocks.begin() + cursorBlock + 1);
        }
    }
    else
    {
        if (cursorPos > 0)
        {
            runs[cursorRun].text.erase(cursorPos - 1, 1);
            cursorPos--;
        }
        else if (cursorRun > 0)
        {
            cursorPos = (int)runs[cursorRun - 1].text.size();
            runs[cursorRun - 1].text += runs[cursorRun].text;
            runs.erase(runs.begin() + cursorRun);
            cursorRun--;
        }
        else if (cursorBlock > 0)
        {
            cursorBlock--;
            auto& prevRuns = blocks[cursorBlock].runs;
            cursorRun = (int)prevRuns.size() - 1;
            cursorPos = (int)prevRuns[cursorRun].text.size();
            prevRuns[cursorRun].text += runs[0].text;
            blocks.erase(blocks.begin() + cursorBlock + 1);
        }
    }
    mergeAdjacentRuns();
    clampCursor();
}

void RichTextBox::backspace()
{
    clampCursor();
    auto& runs = blocks[cursorBlock].runs;

    if (cursorPos > 0)
    {
        runs[cursorRun].text.erase(cursorPos - 1, 1);
        cursorPos--;
    }
    else if (cursorRun > 0)
    {
        cursorPos = (int)runs[cursorRun - 1].text.size();
        runs[cursorRun - 1].text += runs[cursorRun].text;
        runs.erase(runs.begin() + cursorRun);
        cursorRun--;
    }
    else if (cursorBlock > 0)
    {
        int prevBlock = cursorBlock - 1;
        cursorBlock = prevBlock;
        auto& prevRuns = blocks[prevBlock].runs;
        cursorRun = (int)prevRuns.size() - 1;
        cursorPos = (int)prevRuns[cursorRun].text.size();
        prevRuns[cursorRun].text += runs[0].text;
        blocks.erase(blocks.begin() + prevBlock + 1);
    }
    else
    {
        return;
    }
    mergeAdjacentRuns();
    clampCursor();
}

void RichTextBox::insertNewline()
{
    clampCursor();
    splitRunAtCursor();

    RichTextBlock newBlock;
    newBlock.type = BlockType::Paragraph;

    auto& runs = blocks[cursorBlock].runs;
    if (cursorRun + 1 < (int)runs.size())
    {
        newBlock.runs.insert(newBlock.runs.begin(),
            runs.begin() + cursorRun + 1, runs.end());
        runs.resize(cursorRun + 1);
    }
    else if (cursorPos < (int)runs[cursorRun].text.size())
    {
        RichTextRun newRun = runs[cursorRun];
        newRun.text = newRun.text.substr(cursorPos);
        runs[cursorRun].text = runs[cursorRun].text.substr(0, cursorPos);
        newBlock.runs.push_back(newRun);
    }
    else
    {
        RichTextRun emptyRun;
        emptyRun.text = "";
        newBlock.runs.push_back(emptyRun);
    }

    blocks.insert(blocks.begin() + cursorBlock + 1, newBlock);
    cursorBlock++;
    cursorRun = 0;
    cursorPos = 0;
    clampCursor();
}

// Formatting toggles

void RichTextBox::toggleBold()
{
    clampCursor();
    if (selBlock < 0)
    {
        auto& run = blocks[cursorBlock].runs[cursorRun];
        splitRunAtCursor();
        run.bold = !run.bold;
        mergeAdjacentRuns();
    }
}

void RichTextBox::toggleItalic()
{
    clampCursor();
    if (selBlock < 0)
    {
        auto& run = blocks[cursorBlock].runs[cursorRun];
        splitRunAtCursor();
        run.italic = !run.italic;
        mergeAdjacentRuns();
    }
}

void RichTextBox::toggleUnderline()
{
    clampCursor();
    if (selBlock < 0)
    {
        auto& run = blocks[cursorBlock].runs[cursorRun];
        splitRunAtCursor();
        run.underline = !run.underline;
        mergeAdjacentRuns();
    }
}

void RichTextBox::toggleStrikethrough()
{
    clampCursor();
    if (selBlock < 0)
    {
        auto& run = blocks[cursorBlock].runs[cursorRun];
        splitRunAtCursor();
        run.strikethrough = !run.strikethrough;
        mergeAdjacentRuns();
    }
}

void RichTextBox::setBlockType(BlockType bt)
{
    clampCursor();
    blocks[cursorBlock].type = bt;
}

void RichTextBox::setTextAlign(TextAlign align)
{
    textAlign = align;
}

// Markdown conversion

std::string RichTextBox::toMarkdown() const
{
    return richTextToMarkdown(blocks);
}

void RichTextBox::fromMarkdown(const std::string& markdownContent)
{
    blocks = markdownToRichText(markdownContent);
    cursorBlock = 0;
    cursorRun = 0;
    cursorPos = 0;
    selBlock = -1;
    scrollY = 0;
}

// Content height calculation

int RichTextBox::getBlockY(int blockIndex, Font* font) const
{
    int y = padding;
    float fontSize = (float)font_size;
    float lineH = fontSize * lineHeight;

    for (int i = 0; i < blockIndex; i++)
    {
        int blockLines = 1;
        if (!blocks[i].empty())
        {
            std::string fullText = blocks[i].getText();
            float textW = MeasureTextEx(*font, fullText.c_str(), fontSize, 1).x;
            float contentW = bounds.width - padding * 2;
            if (textW > contentW && contentW > 0)
            {
                blockLines = (int)ceilf(textW / contentW);
            }
        }
        y += (int)(blockLines * lineH) + 4;
    }
    return y;
}

void RichTextBox::updateContentHeight(Font* font)
{
    if (blocks.empty())
    {
        contentHeight = bounds.height;
        return;
    }
    int lastY = getBlockY((int)blocks.size(), font);
    contentHeight = (float)lastY;
}

void RichTextBox::getCursorScreenPos(int& outX, int& outY, Font* font) const
{
    clampCursor();
    float fontSize = (float)font_size;
    float lineH = fontSize * lineHeight;

    int blockY = getBlockY(cursorBlock, font);
    int x = bounds.x + padding;
    int y = bounds.y + blockY - (int)scrollY;

    auto& runs = blocks[cursorBlock].runs;
    for (int r = 0; r < cursorRun; r++)
    {
        x += (int)MeasureTextEx(*font, runs[r].text.c_str(), fontSize, 1).x;
    }
    if (cursorRun < (int)runs.size())
    {
        std::string before = runs[cursorRun].text.substr(0, cursorPos);
        x += (int)MeasureTextEx(*font, before.c_str(), fontSize, 1).x;
    }

    outX = x;
    outY = y;
}

// Toolbar

void RichTextBox::drawToolbarInternal(FontManager& fonts)
{
    if (!toolbarVisible) return;

    float tx = bounds.x;
    float ty = bounds.y - toolbarHeight;
    float tw = bounds.width;
    float th = toolbarHeight;

    DrawRectangleRec({tx, ty, tw, th}, toolbarBg);
    DrawRectangleLinesEx({tx, ty, tw, th}, 1, borderColor);

    float bx = tx + 4;
    float bw = 22;
    float bh = 20;
    float by = ty + 4;

    auto& run = blocks[cursorBlock].runs[cursorRun];

    auto drawBtn = [&](const char* label, bool active, std::function<void()> action)
    {
        Color bg = active ? Color{80, 120, 200, 255} : Color{55, 55, 70, 255};
        Vector2 m = GetVirtualMousePos();
        Rectangle rec = {bx, by, bw, bh};
        bool hover = CheckCollisionPointRec(m, rec);
        if (hover) bg = { (unsigned char)(bg.r + 20), (unsigned char)(bg.g + 20),
                          (unsigned char)(bg.b + 20), 255 };

        DrawRectangleRounded(rec, 0.2f, 4, bg);
        int tw2 = MeasureText(label, 10);
        DrawText(label, (int)(bx + (bw - tw2) / 2), (int)(by + 4), 10, WHITE);

        if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            action();
        }
        bx += bw + 2;
    };

    drawBtn("B", run.bold, [this]() { toggleBold(); });
    drawBtn("I", run.italic, [this]() { toggleItalic(); });
    drawBtn("U", run.underline, [this]() { toggleUnderline(); });
    drawBtn("S", run.strikethrough, [this]() { toggleStrikethrough(); });

    bx += 6;
    drawBtn("H1", blocks[cursorBlock].type == BlockType::Heading1,
            [this]() { setBlockType(BlockType::Heading1); });
    drawBtn("H2", blocks[cursorBlock].type == BlockType::Heading2,
            [this]() { setBlockType(BlockType::Heading2); });
    drawBtn("H3", blocks[cursorBlock].type == BlockType::Heading3,
            [this]() { setBlockType(BlockType::Heading3); });

    bx += 6;
    drawBtn("P", blocks[cursorBlock].type == BlockType::Paragraph,
            [this]() { setBlockType(BlockType::Paragraph); });
    drawBtn("- ", blocks[cursorBlock].type == BlockType::BulletList,
            [this]() { setBlockType(BlockType::BulletList); });
    drawBtn("1.", blocks[cursorBlock].type == BlockType::NumberedList,
            [this]() { setBlockType(BlockType::NumberedList); });

    bx += 6;
    drawBtn("L", textAlign == TextAlign::Left,
            [this]() { setTextAlign(TextAlign::Left); });
    drawBtn("C", textAlign == TextAlign::Center,
            [this]() { setTextAlign(TextAlign::Center); });
    drawBtn("R", textAlign == TextAlign::Right,
            [this]() { setTextAlign(TextAlign::Right); });
}

void RichTextBox::handleToolbarClick()
{
}

// Click to position mapping

bool RichTextBox::isInContentArea(Vector2 m) const
{
    return CheckCollisionPointRec(m, {bounds.x, bounds.y, bounds.width, bounds.height});
}

bool RichTextBox::isInToolbarArea(Vector2 m) const
{
    if (!toolbarVisible) return false;
    return CheckCollisionPointRec(m,
        {bounds.x, bounds.y - toolbarHeight, bounds.width, toolbarHeight});
}

void RichTextBox::clickToPosition(Vector2 m, Font* font)
{
    float fontSize = (float)font_size;
    float lineH = fontSize * lineHeight;

    int localY = (int)(m.y - bounds.y + scrollY);
    int localX = (int)(m.x - bounds.x);

    // Find which block was clicked
    int clickedBlock = -1;
    for (int i = 0; i < (int)blocks.size(); i++)
    {
        int blockY = getBlockY(i, font);
        int blockH = (int)lineH + 4;
        if (localY >= blockY && localY < blockY + blockH)
        {
            clickedBlock = i;
            break;
        }
    }

    if (clickedBlock < 0) clickedBlock = (int)blocks.size() - 1;
    cursorBlock = clickedBlock;

    // Find which run/position within the block
    auto& runs = blocks[cursorBlock].runs;
    int x = padding;
    cursorRun = 0;
    cursorPos = 0;

    for (int r = 0; r < (int)runs.size(); r++)
    {
        float runW = MeasureTextEx(*font, runs[r].text.c_str(), fontSize, 1).x;
        if (localX < x + (int)runW || r == (int)runs.size() - 1)
        {
            cursorRun = r;
            // Find character position
            for (int c = 0; c <= (int)runs[r].text.size(); c++)
            {
                std::string sub = runs[r].text.substr(0, c);
                float subW = MeasureTextEx(*font, sub.c_str(), fontSize, 1).x;
                if (localX < x + (int)subW || c == (int)runs[r].text.size())
                {
                    cursorPos = c;
                    break;
                }
            }
            break;
        }
        x += (int)runW;
    }
}

// Selection highlight

void RichTextBox::drawSelectionHighlight(Font* font)
{
    if (selBlock < 0) return;

    float fontSize = (float)font_size;
    float lineH = fontSize * lineHeight;

    int startBlock = selBlock;
    int startRun = selRun;
    int startPos = selPos;
    int endBlock = cursorBlock;
    int endRun = cursorRun;
    int endPos = cursorPos;

    if (startBlock > endBlock || (startBlock == endBlock && startRun > endRun) ||
        (startBlock == endBlock && startRun == endRun && startPos > endPos))
    {
        std::swap(startBlock, endBlock);
        std::swap(startRun, endRun);
        std::swap(startPos, endPos);
    }

    for (int b = startBlock; b <= endBlock; b++)
    {
        int blockY = getBlockY(b, font);
        int y = bounds.y + blockY - (int)scrollY;
        int x1 = bounds.x + padding;
        int x2 = bounds.x + (int)bounds.width - padding;

        DrawRectangleRec(
            {(float)bounds.x, (float)y, bounds.width, lineH},
            selectionColor);
    }
}

// Cursor line

void RichTextBox::drawCursorLine(Font* font)
{
    int cx, cy;
    getCursorScreenPos(cx, cy, font);

    if ((GetTime() * 2.0) - (int)(GetTime() * 2.0) > 0.5f)
    {
        DrawRectangle(cx, cy, 1, (int)(font_size * lineHeight), cursorColor);
    }
}

// Draw all text blocks

void RichTextBox::drawBlocks(FontManager& fonts)
{
    Font* font = fonts.getFont(font_index);
    if (!font) return;
    cachedFont = font;

    float fontSize = (float)font_size;
    float lineH = fontSize * lineHeight;
    float contentW = bounds.width - padding * 2;

    updateContentHeight(font);

    for (int b = 0; b < (int)blocks.size(); b++)
    {
        auto& block = blocks[b];
        int blockY = getBlockY(b, font);
        int drawY = bounds.y + blockY - (int)scrollY;

        if (drawY + (int)lineH < bounds.y - 10) continue;
        if (drawY > bounds.y + (int)bounds.height + 10) continue;

        // Heading overrides
        float actualFontSize = fontSize;
        if (block.type == BlockType::Heading1) actualFontSize = fontSize * 1.6f;
        else if (block.type == BlockType::Heading2) actualFontSize = fontSize * 1.3f;
        else if (block.type == BlockType::Heading3) actualFontSize = fontSize * 1.1f;

        // Calculate total text width for alignment
        float totalTextW = 0;
        for (auto& run : block.runs)
        {
            totalTextW += MeasureTextEx(*font, run.text.c_str(), actualFontSize, 1).x;
        }

        // Calculate starting X based on alignment
        int baseX = bounds.x + padding;
        int drawX = baseX;

        if (textAlign == TextAlign::Center)
        {
            drawX = baseX + (int)((contentW - totalTextW) / 2);
        }
        else if (textAlign == TextAlign::Right)
        {
            drawX = baseX + (int)(contentW - totalTextW);
        }
        // Left and Justify use baseX

        // Block prefix
        if (block.type == BlockType::BulletList)
        {
            DrawText("\xe2\x80\xa2", drawX, drawY, (int)fontSize, text_color);
            drawX += (int)fontSize + 4;
        }
        else if (block.type == BlockType::NumberedList)
        {
            char numBuf[8];
            snprintf(numBuf, sizeof(numBuf), "%d.", b + 1);
            DrawText(numBuf, drawX, drawY, (int)fontSize, text_color);
            drawX += (int)fontSize * 2 + 4;
        }

        // Draw runs
        for (auto& run : block.runs)
        {
            if (run.text.empty()) continue;

            // Highlight background
            if (run.highlightColor.a != 0)
            {
                float tw = MeasureTextEx(*font, run.text.c_str(), actualFontSize, 1).x;
                DrawRectangleRec(
                    {(float)(drawX - 1), (float)(drawY - 1),
                     tw + 2, actualFontSize + 2},
                    run.highlightColor);
            }

            // Strikethrough
            if (run.strikethrough)
            {
                float tw = MeasureTextEx(*font, run.text.c_str(), actualFontSize, 1).x;
                int midY = drawY + (int)(actualFontSize / 2);
                DrawRectangle(drawX, midY, (int)tw, 1, run.textColor);
            }

            // Text
            Color tc = run.textColor;
            if (run.bold && run.italic)
            {
                // Faux bold: draw slightly offset
                DrawTextEx(*font, run.text.c_str(),
                    {(float)(drawX + 1), (float)drawY},
                    actualFontSize, 1, tc);
            }
            DrawTextEx(*font, run.text.c_str(),
                {(float)drawX, (float)drawY},
                actualFontSize, 1, tc);

            // Underline
            if (run.underline)
            {
                float tw = MeasureTextEx(*font, run.text.c_str(), actualFontSize, 1).x;
                int underY = drawY + (int)actualFontSize + 1;
                DrawRectangle(drawX, underY, (int)tw, 1, tc);
            }

            drawX += (int)MeasureTextEx(*font, run.text.c_str(), actualFontSize, 1).x;
        }
    }

    if (focused)
    {
        drawSelectionHighlight(font);
        drawCursorLine(font);
    }
}

// Main draw

void RichTextBox::draw(FontManager& fonts)
{
    if (!visible) return;

    // Background
    DrawRectangleRounded(bounds, 0.05f, 6, bgColor);
    DrawRectangleLinesEx(bounds, 1, interactive && focused ? cursorColor : borderColor);

    drawBlocks(fonts);
    if (interactive) drawToolbarInternal(fonts);
}

// Update (input handling)

void RichTextBox::update(float dt)
{
    if (!visible || !enabled) return;
    if (!interactive) return;

    Vector2 mousePos = GetVirtualMousePos();
    bool wasFocused = focused;

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        if (isInToolbarArea(mousePos))
        {
            handleToolbarClick();
            return;
        }
        else if (isInContentArea(mousePos))
        {
            focused = true;
            toolbarVisible = true;

            Font defaultFont = GetFontDefault();
            Font* currentFont = cachedFont ? cachedFont : &defaultFont;
            clickToPosition(mousePos, currentFont);
        }
        else
        {
            focused = false;
            toolbarVisible = false;
            selBlock = -1;
        }
    }

    if (!focused) return;

    if (IsKeyPressed(KEY_ESCAPE))
    {
        focused = false;
        toolbarVisible = false;
        selBlock = -1;
        return;
    }

    if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL))
    {
        if (IsKeyPressed(KEY_B)) { toggleBold(); return; }
        if (IsKeyPressed(KEY_I)) { toggleItalic(); return; }
        if (IsKeyPressed(KEY_U)) { toggleUnderline(); return; }
    }

    if (IsKeyPressed(KEY_LEFT))
    {
        if (cursorPos > 0)
        {
            cursorPos--;
        }
        else if (cursorRun > 0)
        {
            cursorRun--;
            cursorPos = static_cast<int>(blocks[cursorBlock].runs[cursorRun].text.size());
        }
        else if (cursorBlock > 0)
        {
            cursorBlock--;
            auto& previousRuns = blocks[cursorBlock].runs;
            cursorRun = static_cast<int>(previousRuns.size()) - 1;
            cursorPos = static_cast<int>(previousRuns[cursorRun].text.size());
        }
        selBlock = -1;
        return;
    }

    if (IsKeyPressed(KEY_RIGHT))
    {
        auto& currentRuns = blocks[cursorBlock].runs;
        if (cursorPos < static_cast<int>(currentRuns[cursorRun].text.size()))
        {
            cursorPos++;
        }
        else if (cursorRun + 1 < static_cast<int>(currentRuns.size()))
        {
            cursorRun++;
            cursorPos = 0;
        }
        else if (cursorBlock + 1 < static_cast<int>(blocks.size()))
        {
            cursorBlock++;
            cursorRun = 0;
            cursorPos = 0;
        }
        selBlock = -1;
        return;
    }

    if (IsKeyPressed(KEY_UP))
    {
        if (cursorBlock > 0)
        {
            cursorBlock--;
            clampCursor();
        }
        selBlock = -1;
        return;
    }

    if (IsKeyPressed(KEY_DOWN))
    {
        if (cursorBlock + 1 < static_cast<int>(blocks.size()))
        {
            cursorBlock++;
            clampCursor();
        }
        selBlock = -1;
        return;
    }

    if (IsKeyPressed(KEY_ENTER))
    {
        insertNewline();
        return;
    }

    if (IsKeyPressed(KEY_BACKSPACE))
    {
        backspace();
        return;
    }

    if (IsKeyPressed(KEY_DELETE))
    {
        deleteChar(true);
        return;
    }

    if (IsKeyPressed(KEY_HOME))
    {
        cursorRun = 0;
        cursorPos = 0;
        selBlock = -1;
        return;
    }

    if (IsKeyPressed(KEY_END))
    {
        auto& currentRuns = blocks[cursorBlock].runs;
        cursorRun = static_cast<int>(currentRuns.size()) - 1;
        cursorPos = static_cast<int>(currentRuns[cursorRun].text.size());
        selBlock = -1;
        return;
    }

    int pressedKey = GetCharPressed();
    while (pressedKey > 0)
    {
        if (pressedKey >= 32 && pressedKey < 127)
        {
            insertChar(static_cast<char>(pressedKey));
        }
        pressedKey = GetCharPressed();
    }

    float scrollWheel = GetMouseWheelMove();
    if (scrollWheel != 0.0f)
    {
        scrollY -= scrollWheel * 30.0f;
        if (scrollY < 0) scrollY = 0;
    }
}

// JSON serialization

json RichTextBox::toJson() const
{
    json j = Widget::toJson();
    j["type"] = "richtextbox";
    j["width"] = bounds.width;
    j["height"] = bounds.height;
    j["markdown_content"] = toMarkdown();
    j["bg_color"] = colorToJson(bgColor);
    j["border_color"] = colorToJson(borderColor);
    j["line_spacing"] = lineHeight;
    j["padding"] = padding;
    j["text_align"] = static_cast<int>(textAlign);
    return j;
}
