#include "ui_editor.hpp"
#include "raylib.h"
#include "rlgl.h"
#include "pixel_scale.hpp"
#include "editor_controls.hpp"
#include "image_utils.hpp"
#include <tinyfiledialogs.h>
#include <cstring>
#include <cstdio>
#include <cmath>

UIEditor::UIEditor(UIManager& uiManager) : uiManager(uiManager)
{
    auto& screens = uiManager.getScreens();
    if (!screens.empty())
    {
        editingScreenName = screens.begin()->first;
    }
    textBuf[0] = '\0';
    actionBuf[0] = '\0';
    screenNameBuf[0] = '\0';
}

bool UIEditor::isEditorMode() const
{
    return editorMode;
}

void UIEditor::toggle()
{
    editorMode = !editorMode;
    if (editorMode)
    {
        editingScreenName = uiManager.getCurrentScreenName();
        deselectWidget();
    }
    else
    {
        uiManager.setCurrentScreenImmediate(editingScreenName);
    }
}

Screen* UIEditor::getCurrentScreen()
{
    auto& screens = uiManager.getScreens();
    auto it = screens.find(editingScreenName);
    if (it != screens.end()) return it->second.get();
    return nullptr;
}

Widget* UIEditor::getSelectedWidget()
{
    Screen* screen = getCurrentScreen();
    if (!screen || selectedWidgetIndex < 0) return nullptr;
    auto& widgets = screen->getWidgets();
    if (selectedWidgetIndex < static_cast<int>(widgets.size()))
    {
        return widgets[selectedWidgetIndex].get();
    }
    return nullptr;
}

void UIEditor::selectWidget(int index)
{
    selectedWidgetIndex = index;
    syncPropsFromSelected();
}

void UIEditor::deselectWidget()
{
    selectedWidgetIndex = -1;
    textBuf[0] = '\0';
    actionBuf[0] = '\0';
    textBufLen = 0;
    actionBufLen = 0;
    activeField = Field::None;
    editFontSize = 20;
    editFontIndex = 0;
    editX = 0; editY = 0;
    editWidth = 200; editHeight = 50;
    editColorR = 0; editColorG = 0; editColorB = 0; editColorA = 255;
    editHoverR = 0; editHoverG = 0; editHoverB = 0; editHoverA = 255;
    editPressR = 0; editPressG = 0; editPressB = 0; editPressA = 255;
    editTxtR = 255; editTxtG = 255; editTxtB = 255; editTxtA = 255;
    actionTypeIndex = 0;
    imgPathBuf[0] = '\0';
    imgPathBufLen = 0;
    editTintR = 255; editTintG = 255; editTintB = 255; editTintA = 255;

    Screen* screen = getCurrentScreen();
    if (screen)
    {
        strncpy(bgImgPathBuf, screen->getBackgroundImagePath().c_str(), sizeof(bgImgPathBuf) - 1);
        bgImgPathBuf[sizeof(bgImgPathBuf) - 1] = '\0';
        bgImgPathBufLen = static_cast<int>(screen->getBackgroundImagePath().length());
        editBgFitIndex = static_cast<int>(screen->getBackgroundFit());
        Color bc = screen->getBackgroundColor();
        editBgColorR = bc.r;
        editBgColorG = bc.g;
        editBgColorB = bc.b;
        editBgColorA = bc.a;

        editBgPatternIndex = static_cast<int>(screen->getPattern());
        Color pa = screen->getPatternColorA();
        editPatColorAR = pa.r; editPatColorAG = pa.g; editPatColorAB = pa.b; editPatColorAA = pa.a;
        Color pb = screen->getPatternColorB();
        editPatColorBR = pb.r; editPatColorBG = pb.g; editPatColorBB = pb.b; editPatColorBA = pb.a;
        editPatTileSize = screen->getPatternTileSize();

        editScrollDirIndex = static_cast<int>(screen->getScrollDirection());
        editScrollSpeed = static_cast<int>(screen->getScrollSpeed());
    }
    else
    {
        bgImgPathBuf[0] = '\0';
        bgImgPathBufLen = 0;
    }
}

void UIEditor::syncPropsFromSelected()
{
    Widget* widget = getSelectedWidget();
    if (!widget) return;

    std::string text = widget->getText();
    strncpy(textBuf, text.c_str(), sizeof(textBuf) - 1);
    textBuf[sizeof(textBuf) - 1] = '\0';
    textBufLen = static_cast<int>(text.length());

    std::string action = widget->action;
    std::string actionValue;
    if (action.rfind("goto:", 0) == 0) { actionTypeIndex = 0; actionValue = action.substr(5); }
    else if (action == "quit") { actionTypeIndex = 1; }
    else if (action == "toggle_fullscreen") { actionTypeIndex = 2; }
    else if (action == "pause") { actionTypeIndex = 3; }
    else if (action.rfind("run:", 0) == 0) { actionTypeIndex = 4; actionValue = action.substr(4); }
    else { actionTypeIndex = 5; actionValue = action; }

    strncpy(actionBuf, actionValue.c_str(), sizeof(actionBuf) - 1);
    actionBuf[sizeof(actionBuf) - 1] = '\0';
    actionBufLen = static_cast<int>(actionValue.length());

    editFontSize = widget->font_size;
    editFontIndex = widget->font_index;
    editX = widget->getPosition().x;
    editY = widget->getPosition().y;
    snprintf(xBuf, sizeof(xBuf), "%.0f", editX);
    xBufLen = static_cast<int>(strlen(xBuf));
    snprintf(yBuf, sizeof(yBuf), "%.0f", editY);
    yBufLen = static_cast<int>(strlen(yBuf));
    editWidth = widget->getSize().x;
    editHeight = widget->getSize().y;
    snprintf(wBuf, sizeof(wBuf), "%.0f", editWidth);
    wBufLen = static_cast<int>(strlen(wBuf));
    snprintf(hBuf, sizeof(hBuf), "%.0f", editHeight);
    hBufLen = static_cast<int>(strlen(hBuf));

    editTxtR = widget->text_color.r;
    editTxtG = widget->text_color.g;
    editTxtB = widget->text_color.b;
    editTxtA = widget->text_color.a;

    if (widget->getType() == WidgetType::Button)
    {
        editColorR = widget->normal_color.r;
        editColorG = widget->normal_color.g;
        editColorB = widget->normal_color.b;
        editColorA = widget->normal_color.a;
        editHoverR = widget->hover_color.r;
        editHoverG = widget->hover_color.g;
        editHoverB = widget->hover_color.b;
        editHoverA = widget->hover_color.a;
        editPressR = widget->pressed_color.r;
        editPressG = widget->pressed_color.g;
        editPressB = widget->pressed_color.b;
        editPressA = widget->pressed_color.a;

        Button* btn = static_cast<Button*>(widget);
        idleAnimFramePaths = btn->idleAnim.framePaths;
        idleAnimFrameDuration = btn->idleAnim.frameDuration;
        idleAnimLoop = btn->idleAnim.loop;
        hoverAnimFramePaths = btn->hoverAnim.framePaths;
        hoverAnimFrameDuration = btn->hoverAnim.frameDuration;
        hoverAnimLoop = btn->hoverAnim.loop;
        clickAnimFramePaths = btn->clickAnim.framePaths;
        clickAnimFrameDuration = btn->clickAnim.frameDuration;
        clickAnimLoop = btn->clickAnim.loop;
    }

    if (widget->getType() == WidgetType::Image)
    {
        ImageWidget* imgWidget = static_cast<ImageWidget*>(widget);
        strncpy(imgPathBuf, imgWidget->imagePath.c_str(), sizeof(imgPathBuf) - 1);
        imgPathBuf[sizeof(imgPathBuf) - 1] = '\0';
        imgPathBufLen = static_cast<int>(imgWidget->imagePath.length());
        editTintR = imgWidget->tint.r;
        editTintG = imgWidget->tint.g;
        editTintB = imgWidget->tint.b;
        editTintA = imgWidget->tint.a;
        editFitIndex = static_cast<int>(imgWidget->fit);
        imgAnimFramePaths = imgWidget->anim.framePaths;
        imgAnimFrameDuration = imgWidget->anim.frameDuration;
        imgAnimLoop = imgWidget->anim.loop;
    }

    if (widget->getType() == WidgetType::RichTextBox)
    {
        RichTextBox* rtb = static_cast<RichTextBox*>(widget);
        std::string markdownContent = rtb->toMarkdown();
        strncpy(mdContentBuf, markdownContent.c_str(), sizeof(mdContentBuf) - 1);
        mdContentBuf[sizeof(mdContentBuf) - 1] = '\0';
        mdContentBufLen = static_cast<int>(markdownContent.length());
        rtbFontSize = rtb->font_size;
        rtbLineSpacing = rtb->lineHeight;
        rtbPadding = rtb->padding;
        rtbTextAlign = static_cast<int>(rtb->textAlign);
        editRtbBgR = rtb->bgColor.r;
        editRtbBgG = rtb->bgColor.g;
        editRtbBgB = rtb->bgColor.b;
        editRtbBgA = rtb->bgColor.a;
    }

    if (widget->getType() == WidgetType::ImageViewer)
    {
        ImageViewer* iv = static_cast<ImageViewer*>(widget);
        editTintR = iv->tint.r;
        editTintG = iv->tint.g;
        editTintB = iv->tint.b;
        editTintA = iv->tint.a;
        editFitIndex = static_cast<int>(iv->fit);
    }
}

void UIEditor::applyPropsToSelected()
{
    Widget* widget = getSelectedWidget();
    if (!widget)
    {
        Screen* s = getCurrentScreen();
        if (s)
        {
            s->setBackgroundColor(colorToEdit(editBgColorR, editBgColorG, editBgColorB, editBgColorA));
            s->setPatternColorA(colorToEdit(editPatColorAR, editPatColorAG, editPatColorAB, editPatColorAA));
            s->setPatternColorB(colorToEdit(editPatColorBR, editPatColorBG, editPatColorBB, editPatColorBA));
        }
        return;
    }

    widget->setText(textBuf);
    widget->font_size = editFontSize;
    widget->font_index = editFontIndex;
    widget->setPosition(editX, editY);
    if (widget->getType() == WidgetType::Button || widget->getType() == WidgetType::Image || widget->getType() == WidgetType::ImageViewer || widget->getType() == WidgetType::RichTextBox)
        widget->setSize(editWidth, editHeight);

    widget->text_color = colorToEdit(editTxtR, editTxtG, editTxtB, editTxtA);

    if (widget->getType() == WidgetType::Button)
    {
        widget->normal_color = colorToEdit(editColorR, editColorG, editColorB, editColorA);
        widget->hover_color = colorToEdit(editHoverR, editHoverG, editHoverB, editHoverA);
        widget->pressed_color = colorToEdit(editPressR, editPressG, editPressB, editPressA);
        static_cast<Button*>(widget)->current_color = widget->normal_color;

        switch (actionTypeIndex)
        {
            case 0: widget->action = std::string("goto:") + actionBuf; break;
            case 1: widget->action = "quit"; break;
            case 2: widget->action = "toggle_fullscreen"; break;
            case 3: widget->action = "pause"; break;
            case 4: widget->action = std::string("run:") + actionBuf; break;
            case 5: widget->action = actionBuf; break;
            default: widget->action = ""; break;
        }

        Button* btn = static_cast<Button*>(widget);
        if (btn->idleAnim.framePaths != idleAnimFramePaths ||
            btn->idleAnim.frameDuration != idleAnimFrameDuration ||
            btn->idleAnim.loop != idleAnimLoop)
        {
            btn->setIdleAnimation(idleAnimFramePaths, idleAnimFrameDuration, idleAnimLoop);
        }
        if (btn->hoverAnim.framePaths != hoverAnimFramePaths ||
            btn->hoverAnim.frameDuration != hoverAnimFrameDuration ||
            btn->hoverAnim.loop != hoverAnimLoop)
        {
            btn->setHoverAnimation(hoverAnimFramePaths, hoverAnimFrameDuration, hoverAnimLoop);
        }
        if (btn->clickAnim.framePaths != clickAnimFramePaths ||
            btn->clickAnim.frameDuration != clickAnimFrameDuration ||
            btn->clickAnim.loop != clickAnimLoop)
        {
            btn->setClickAnimation(clickAnimFramePaths, clickAnimFrameDuration, clickAnimLoop);
        }
    }

    if (widget->getType() == WidgetType::Image)
    {
        ImageWidget* imgWidget = static_cast<ImageWidget*>(widget);
        if (strcmp(imgPathBuf, imgWidget->imagePath.c_str()) != 0)
            imgWidget->setImagePath(imgPathBuf);
        imgWidget->tint = colorToEdit(editTintR, editTintG, editTintB, editTintA);
        imgWidget->fit = static_cast<ImageFit>(editFitIndex);

        if (imgWidget->anim.framePaths != imgAnimFramePaths ||
            imgWidget->anim.frameDuration != imgAnimFrameDuration ||
            imgWidget->anim.loop != imgAnimLoop)
        {
            imgWidget->setAnimation(imgAnimFramePaths, imgAnimFrameDuration, imgAnimLoop);
        }
    }

    if (widget->getType() == WidgetType::RichTextBox)
    {
        RichTextBox* rtb = static_cast<RichTextBox*>(widget);
        rtb->fromMarkdown(mdContentBuf);
        rtb->font_size = rtbFontSize;
        rtb->lineHeight = rtbLineSpacing;
        rtb->padding = rtbPadding;
        rtb->textAlign = static_cast<TextAlign>(rtbTextAlign);
        rtb->bgColor.r = static_cast<unsigned char>(editRtbBgR);
        rtb->bgColor.g = static_cast<unsigned char>(editRtbBgG);
        rtb->bgColor.b = static_cast<unsigned char>(editRtbBgB);
        rtb->bgColor.a = static_cast<unsigned char>(editRtbBgA);
    }

    if (widget->getType() == WidgetType::ImageViewer)
    {
        ImageViewer* iv = static_cast<ImageViewer*>(widget);
        iv->tint = colorToEdit(editTintR, editTintG, editTintB, editTintA);
        iv->fit = static_cast<ImageFit>(editFitIndex);
    }
}


void UIEditor::addWidgetToScreen(const std::string& type)
{
    Screen* screen = getCurrentScreen();
    if (!screen) return;
    deselectWidget();
    if (type == "button")
        screen->createWidget<Button>(300, 200, 200, 50, "New Button");
    else if (type == "label")
        screen->createWidget<Label>(300, 200, "New Label", 20);
    else if (type == "image")
        screen->createWidget<ImageWidget>(300, 200, 150, 150, "");
    else if (type == "richtextbox")
        screen->createWidget<RichTextBox>(300, 150, 400, 250);
    else if (type == "imageviewer")
        screen->createWidget<ImageViewer>(300, 200, 300, 250);
    auto& widgets = screen->getWidgets();
    selectWidget(static_cast<int>(widgets.size()) - 1);
}

void UIEditor::deleteSelectedWidget()
{
    Screen* screen = getCurrentScreen();
    if (!screen || selectedWidgetIndex < 0) return;
    screen->removeWidget(selectedWidgetIndex);
    deselectWidget();
}

void UIEditor::duplicateSelectedWidget()
{
    Screen* screen = getCurrentScreen();
    Widget* w = getSelectedWidget();
    if (!screen || !w) return;
    json j = w->toJson();
    auto newWidget = std::make_shared<Button>(0, 0, 200, 50, "copy");
    if (w->getType() == WidgetType::Label)
    {
        auto lbl = std::make_shared<Label>(0, 0, "copy", 20);
        lbl->action = w->action;
        lbl->font_size = w->font_size;
        lbl->font_index = w->font_index;
        lbl->text_color = w->text_color;
        lbl->setPosition(w->getPosition().x + 20, w->getPosition().y + 20);
        screen->addWidget(lbl);
    }
    else if (w->getType() == WidgetType::Image)
    {
        ImageWidget* srcImg = static_cast<ImageWidget*>(w);
        auto img = std::make_shared<ImageWidget>(
            w->getPosition().x + 20, w->getPosition().y + 20,
            w->getSize().x, w->getSize().y, srcImg->imagePath);
        img->tint = srcImg->tint;
        screen->addWidget(img);
    }
    else if (w->getType() == WidgetType::ImageViewer)
    {
        ImageViewer* srcIv = static_cast<ImageViewer*>(w);
        auto iv = std::make_shared<ImageViewer>(
            w->getPosition().x + 20, w->getPosition().y + 20,
            w->getSize().x, w->getSize().y);
        iv->tint = srcIv->tint;
        iv->fit = srcIv->fit;
        for (auto& img : srcIv->images)
        {
            iv->addImage(img.path);
        }
        iv->setCurrentIndex(srcIv->currentIndex);
        screen->addWidget(iv);
    }
    else
    {
        auto btn = std::make_shared<Button>(
            w->getPosition().x + 20, w->getPosition().y + 20,
            w->getSize().x, w->getSize().y, w->getText());
        btn->action = w->action;
        btn->font_size = w->font_size;
        btn->font_index = w->font_index;
        btn->text_color = w->text_color;
        btn->normal_color = w->normal_color;
        btn->hover_color = w->hover_color;
        btn->pressed_color = w->pressed_color;
        screen->addWidget(btn);
    }
    auto& widgets = screen->getWidgets();
    selectWidget(static_cast<int>(widgets.size()) - 1);
}

// ===================== Custom UI Controls =====================

bool UIEditor::button(Rectangle rec, const char* label, Color bg)
{
    Vector2 mousePos = GetVirtualMousePos();
    bool hover = CheckCollisionPointRec(mousePos, rec);
    bool press = hover && IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    bool click = hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    Color color = bg;
    if (press) color = { static_cast<unsigned char>(bg.r * 0.6f), static_cast<unsigned char>(bg.g * 0.6f), static_cast<unsigned char>(bg.b * 0.6f), 255 };
    else if (hover) color = { static_cast<unsigned char>(bg.r * 1.2f > 255 ? 255 : bg.r * 1.2f),
                             static_cast<unsigned char>(bg.g * 1.2f > 255 ? 255 : bg.g * 1.2f),
                             static_cast<unsigned char>(bg.b * 1.2f > 255 ? 255 : bg.b * 1.2f), 255 };

    DrawRectangleRec(rec, color);
    int textWidth = MeasureText(label, 12);
    DrawText(label, static_cast<int>(rec.x + (rec.width - textWidth) / 2),
             static_cast<int>(rec.y + (rec.height - 12) / 2), 12, WHITE);
    return click;
}

bool UIEditor::slider(Rectangle rec, const char* label, int* value, int minVal, int maxVal)
{
    Vector2 mousePos = GetVirtualMousePos();
    bool changed = false;

    DrawText(label, static_cast<int>(rec.x), static_cast<int>(rec.y + 2), 10, LIGHTGRAY);

    Rectangle track = { rec.x + 30, rec.y + 1, rec.width - 60, 12 };
    DrawRectangleRec(track, {50, 50, 60, 255});

    float t = (*value - static_cast<float>(minVal)) / static_cast<float>(maxVal - minVal);
    Rectangle fill = { track.x, track.y, track.width * t, track.height };
    DrawRectangleRec(fill, {80, 120, 180, 255});

    float handleX = track.x + track.width * t - 4;
    DrawRectangle(static_cast<int>(handleX), static_cast<int>(track.y - 1), 8, 14, WHITE);

    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mousePos, {track.x - 4, track.y - 4, track.width + 8, track.height + 8}))
    {
        float normalizedT = (mousePos.x - track.x) / track.width;
        if (normalizedT < 0) normalizedT = 0; if (normalizedT > 1) normalizedT = 1;
        int newVal = minVal + static_cast<int>(normalizedT * (maxVal - minVal));
        if (newVal != *value) { *value = newVal; changed = true; }
    }

    char valueStr[16];
    snprintf(valueStr, sizeof(valueStr), "%d", *value);
    DrawText(valueStr, static_cast<int>(track.x + track.width + 4), static_cast<int>(track.y - 1), 10, WHITE);

    return changed;
}

bool UIEditor::sliderF(Rectangle rec, const char* label, float* value, float minVal, float maxVal)
{
    Vector2 mousePos = GetVirtualMousePos();
    bool changed = false;

    DrawText(label, static_cast<int>(rec.x), static_cast<int>(rec.y + 2), 10, LIGHTGRAY);

    Rectangle track = { rec.x + 30, rec.y + 1, rec.width - 60, 12 };
    DrawRectangleRec(track, {50, 50, 60, 255});

    float t = (*value - minVal) / (maxVal - minVal);
    Rectangle fill = { track.x, track.y, track.width * t, track.height };
    DrawRectangleRec(fill, {80, 120, 180, 255});

    float handleX = track.x + track.width * t - 4;
    DrawRectangle(static_cast<int>(handleX), static_cast<int>(track.y - 1), 8, 14, WHITE);

    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mousePos, {track.x - 4, track.y - 4, track.width + 8, track.height + 8}))
    {
        float normalizedT = (mousePos.x - track.x) / track.width;
        if (normalizedT < 0) normalizedT = 0; if (normalizedT > 1) normalizedT = 1;
        float newVal = minVal + normalizedT * (maxVal - minVal);
        if (fabs(newVal - *value) > 0.5f) { *value = newVal; changed = true; }
    }

    char valueStr[16];
    snprintf(valueStr, sizeof(valueStr), "%.0f", *value);
    DrawText(valueStr, static_cast<int>(track.x + track.width + 4), static_cast<int>(track.y - 1), 10, WHITE);

    return changed;
}

bool UIEditor::textBox(Rectangle rec, char* buf, int bufSize, int& len, bool active, bool multiline)
{
    Vector2 mousePos = GetVirtualMousePos();
    bool clicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mousePos, rec);

    if (clicked)
        textCursorPos = textBoxClickPos(rec, buf, len, mousePos, multiline);

    DrawRectangleRec(rec, {30, 30, 40, 255});
    DrawRectangleLinesEx(rec, 1, active ? BLUE : GRAY);

    if (active)
    {
        if (textCursorPos > len) textCursorPos = len;
        if (textCursorPos < 0) textCursorPos = 0;

        int pressedKey = GetCharPressed();
        while (pressedKey > 0)
        {
            if (pressedKey >= 32 && pressedKey < 127 && len < bufSize - 1)
            {
                for (int i = len; i >= textCursorPos; i--)
                    buf[i + 1] = buf[i];
                buf[textCursorPos] = static_cast<char>(pressedKey);
                len++;
                textCursorPos++;
            }
            pressedKey = GetCharPressed();
        }

        if (IsKeyPressed(KEY_ENTER) && multiline && len < bufSize - 1)
        {
            for (int i = len; i >= textCursorPos; i--)
                buf[i + 1] = buf[i];
            buf[textCursorPos] = '\n';
            len++;
            textCursorPos++;
        }

        if (IsKeyPressed(KEY_BACKSPACE) && textCursorPos > 0)
        {
            for (int i = textCursorPos - 1; i < len; i++)
                buf[i] = buf[i + 1];
            len--;
            textCursorPos--;
        }

        if (IsKeyPressed(KEY_DELETE) && textCursorPos < len)
        {
            for (int i = textCursorPos; i < len; i++)
                buf[i] = buf[i + 1];
            len--;
        }

        if (IsKeyPressed(KEY_LEFT) && textCursorPos > 0) textCursorPos--;
        if (IsKeyPressed(KEY_RIGHT) && textCursorPos < len) textCursorPos++;
        if (IsKeyPressed(KEY_HOME)) textCursorPos = 0;
        if (IsKeyPressed(KEY_END)) textCursorPos = len;

        textBoxDrawCursor(rec, buf, len, textCursorPos, multiline);
    }

    textBoxDrawText(rec, buf, len, multiline);
    return clicked;
}

int UIEditor::textBoxClickPos(Rectangle rec, const char* buf, int len, Vector2 mouse, bool multiline)
{
    if (!multiline)
    {
        for (int i = 0; i <= len; i++)
        {
            std::string sub(buf, i);
            if (mouse.x < rec.x + TB_PAD + MeasureText(sub.c_str(), TB_FONT))
                return i;
        }
        return len;
    }

    int clickedLine = (int)((mouse.y - rec.y - TB_PAD) / TB_LINE_H);
    int lineStart = 0;
    int currentLine = 0;

    for (int i = 0; i <= len; i++)
    {
        if (i == len || buf[i] == '\n')
        {
            if (currentLine == clickedLine)
            {
                for (int j = lineStart; j < i; j++)
                {
                    std::string sub(buf + lineStart, j - lineStart + 1);
                    if (mouse.x < rec.x + TB_PAD + MeasureText(sub.c_str(), TB_FONT))
                        return j;
                }
                return i;
            }
            lineStart = i + 1;
            currentLine++;
        }
    }
    return len;
}

int UIEditor::textBoxCursorLine(const char* buf, int pos)
{
    int line = 0;
    for (int i = 0; i < pos; i++)
        if (buf[i] == '\n') line++;
    return line;
}

int UIEditor::textBoxCursorX(const char* buf, int lineStart, int pos)
{
    std::string lineText(buf + lineStart, pos - lineStart);
    return TB_PAD + MeasureText(lineText.c_str(), TB_FONT);
}

void UIEditor::textBoxDrawCursor(Rectangle rec, const char* buf, int len, int pos, bool multiline)
{
    if ((GetTime() * 2.0) - (int)(GetTime() * 2.0) <= 0.5f) return;

    int cursorX, cursorY;
    if (multiline)
    {
        int line = textBoxCursorLine(buf, pos);
        int lineStart = 0;
        for (int i = 0; i < pos; i++)
            if (buf[i] == '\n') lineStart = i + 1;
        cursorX = static_cast<int>(rec.x + textBoxCursorX(buf, lineStart, pos));
        cursorY = static_cast<int>(rec.y + TB_PAD + line * TB_LINE_H);
    }
    else
    {
        cursorX = static_cast<int>(rec.x + textBoxCursorX(buf, 0, pos));
        cursorY = static_cast<int>(rec.y + TB_PAD - 1);
    }
    DrawRectangle(cursorX, cursorY, 1, TB_LINE_H, WHITE);
}

void UIEditor::textBoxDrawText(Rectangle rec, const char* buf, int len, bool multiline)
{
    if (!multiline)
    {
        DrawText(buf, static_cast<int>(rec.x + TB_PAD), static_cast<int>(rec.y + TB_PAD), TB_FONT, WHITE);
        return;
    }

    int y = static_cast<int>(rec.y + TB_PAD);
    int lineStart = 0;
    for (int i = 0; i <= len; i++)
    {
        if (i == len || buf[i] == '\n')
        {
            DrawText(std::string(buf + lineStart, i - lineStart).c_str(),
                     static_cast<int>(rec.x + TB_PAD), y, TB_FONT, WHITE);
            y += TB_LINE_H;
            lineStart = i + 1;
        }
    }
}


void UIEditor::commitActiveField()
{
    switch (activeField)
    {
        case Field::WidgetText: break;
        case Field::ActionValue: break;
        case Field::ScreenName: break;
        case Field::BgImagePath:
        {
            Screen* currentScreen = getCurrentScreen();
            if (currentScreen) currentScreen->setBackgroundImage(bgImgPathBuf);
            break;
        }
        case Field::PosX:
            editX = strtof(xBuf, nullptr);
            break;
        case Field::PosY:
            editY = strtof(yBuf, nullptr);
            break;
        case Field::SizeW:
            editWidth = strtof(wBuf, nullptr);
            break;
        case Field::SizeH:
            editHeight = strtof(hBuf, nullptr);
            break;
        case Field::ColorR: case Field::ColorG: case Field::ColorB: case Field::ColorA:
            if (activeColorR)
            {
                int* target = (activeField == Field::ColorR) ? activeColorR :
                              (activeField == Field::ColorG) ? activeColorG :
                              (activeField == Field::ColorB) ? activeColorB : activeColorA;
                const char* buf = (activeField == Field::ColorR) ? colorRBuf :
                                  (activeField == Field::ColorG) ? colorGBuf :
                                  (activeField == Field::ColorB) ? colorBBuf : colorABuf;
                *target = static_cast<int>(strtof(buf, nullptr));
                if (*target < 0) *target = 0; if (*target > 255) *target = 255;
                rgbToHsv(*activeColorR, *activeColorG, *activeColorB, pickerHue, pickerSat, pickerVal);
                snprintf(colorRBuf, sizeof(colorRBuf), "%d", *activeColorR);
                snprintf(colorGBuf, sizeof(colorGBuf), "%d", *activeColorG);
                snprintf(colorBBuf, sizeof(colorBBuf), "%d", *activeColorB);
                snprintf(colorABuf, sizeof(colorABuf), "%d", *activeColorA);
                colorRBufLen = static_cast<int>(strlen(colorRBuf));
                colorGBufLen = static_cast<int>(strlen(colorGBuf));
                colorBBufLen = static_cast<int>(strlen(colorBBuf));
                colorABufLen = static_cast<int>(strlen(colorABuf));
            }
            break;
        case Field::RichTextContent:
        {
            Widget* selectedWidget = getSelectedWidget();
            if (selectedWidget && selectedWidget->getType() == WidgetType::RichTextBox)
            {
                RichTextBox* rtb = static_cast<RichTextBox*>(selectedWidget);
                rtb->fromMarkdown(mdContentBuf);
                rtb->font_size = rtbFontSize;
            }
            break;
        }
        default: break;
    }
}

// ===================== Editor Logic =====================

void UIEditor::update(float dt)
{
    if (!editorMode) return;

    Screen* screen = getCurrentScreen();
    g_view.offset = canvasPan;
    g_view.scale = canvasZoom;
    if (screen)
    {
        screen->update(dt);
    }

    float scrollWheel = GetMouseWheelMove();
    if (scrollWheel != 0.0f)
    {
        float scrollSpeed = 30.0f;
        float delta = -scrollWheel * scrollSpeed;
        Vector2 mousePos = GetVirtualMousePos();
        float screenW = static_cast<float>(GetVirtualScreenWidth());
        float screenH = static_cast<float>(GetVirtualScreenHeight());

        if (showFontPanel)
        {
            float panelW = 320.0f;
            float panelH = 340.0f;
            float panelX = (screenW - panelW) / 2;
            float panelY = (screenH - panelH) / 2;
            if (mousePos.x >= panelX && mousePos.x <= panelX + panelW && mousePos.y >= panelY && mousePos.y <= panelY + panelH)
            {
                FontManager& fonts = uiManager.getFontManager();
                float contentH = fonts.getCount() * 18.0f;
                float listH = 80.0f;
                float maxScroll = (contentH > listH) ? (contentH - listH) : 0.0f;
                fontListScroll += delta;
                if (fontListScroll < 0.0f) fontListScroll = 0.0f;
                if (fontListScroll > maxScroll) fontListScroll = maxScroll;
                return;
            }
        }

        if (mousePos.x >= SIDEBAR_W && mousePos.x <= screenW - PROP_PANEL_W && mousePos.y > TOOLBAR_H)
        {
            float mouseCanvasX = (mousePos.x - canvasPan.x) / canvasZoom;
            float mouseCanvasY = (mousePos.y - canvasPan.y) / canvasZoom;
            float newZoom = canvasZoom * (1.0f + scrollWheel * 0.1f);
            if (newZoom < 0.25f) newZoom = 0.25f;
            if (newZoom > 4.0f) newZoom = 4.0f;
            canvasPan.x = mousePos.x - mouseCanvasX * newZoom;
            canvasPan.y = mousePos.y - mouseCanvasY * newZoom;
            canvasZoom = newZoom;
            return;
        }

        if (mousePos.x < SIDEBAR_W && mousePos.y > TOOLBAR_H)
        {
            float contentH = screenListContentH;
            if (contentH < 30.0f) contentH = 30.0f;
            float visibleH = screenH - TOOLBAR_H;
            float maxScroll = (contentH > visibleH) ? (contentH - visibleH) : 0.0f;
            screenListScroll += delta;
            if (screenListScroll < 0.0f) screenListScroll = 0.0f;
            if (screenListScroll > maxScroll) screenListScroll = maxScroll;
        }
        else if (mousePos.x > screenW - PROP_PANEL_W && mousePos.y > TOOLBAR_H)
        {
            float contentH = propPanelContentH;
            if (contentH < 80.0f) contentH = 80.0f;
            float visibleH = screenH - TOOLBAR_H;
            float maxScroll = (contentH > visibleH) ? (contentH - visibleH) : 0.0f;
            propPanelScroll += delta;
            if (propPanelScroll < 0.0f) propPanelScroll = 0.0f;
            if (propPanelScroll > maxScroll) propPanelScroll = maxScroll;
        }
    }

    if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_E)) { toggle(); return; }
    if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_D)) duplicateSelectedWidget();
    if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_I)) uiManager.getInitialScreenRef() = editingScreenName;
    if (IsKeyPressed(KEY_G)) snapToGrid = !snapToGrid;

    if (showFontPanel && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        Vector2 mousePos = GetVirtualMousePos();
        float panelW = 320.0f;
        float panelH = 340.0f;
        float panelX = (static_cast<float>(GetVirtualScreenWidth()) - panelW) / 2;
        float panelY = (static_cast<float>(GetVirtualScreenHeight()) - panelH) / 2;

        if (mousePos.x < panelX || mousePos.x > panelX + panelW || mousePos.y < panelY || mousePos.y > panelY + panelH)
        {
            if (activeField == Field::FontPath) { fontPathActive = false; activeField = Field::None; }
            if (activeField == Field::FontPreview) { fontPreviewActive = false; activeField = Field::None; }
        }
    }

    Screen* currentScreen = getCurrentScreen();
    if (!currentScreen) return;

    Vector2 mouse = GetVirtualMousePos();
    bool mouseInCanvas = (mouse.x >= SIDEBAR_W && mouse.x <= GetVirtualScreenWidth() - PROP_PANEL_W &&
                          mouse.y >= TOOLBAR_H && mouse.y <= GetVirtualScreenHeight());

    if (canvasPanning)
    {
        if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE))
        {
            canvasPan.x += mouse.x - canvasPanLast.x;
            canvasPan.y += mouse.y - canvasPanLast.y;
            canvasPanLast = mouse;
        }
        else
        {
            canvasPanning = false;
        }
        return;
    }

    if (mouseInCanvas && IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE))
    {
        canvasPanning = true;
        canvasPanLast = mouse;
        return;
    }

    float mouseCanvasX = (mouse.x - canvasPan.x) / canvasZoom;
    float mouseCanvasY = (mouse.y - canvasPan.y) / canvasZoom;

    if (resizing && selectedWidgetIndex >= 0)
    {
        Widget* selectedWidget = getSelectedWidget();
        if (selectedWidget && IsMouseButtonDown(MOUSE_BUTTON_LEFT))
        {
            float newW = mouseCanvasX - selectedWidget->getPosition().x;
            float newH = mouseCanvasY - selectedWidget->getPosition().y;
            if (newW < 30) newW = 30;
            if (newH < 15) newH = 15;
            if (snapToGrid)
            {
                newW = roundf(newW / GRID_SIZE) * GRID_SIZE;
                newH = roundf(newH / GRID_SIZE) * GRID_SIZE;
            }
            editWidth = newW;
            editHeight = newH;
            snprintf(wBuf, sizeof(wBuf), "%.0f", editWidth);
            wBufLen = static_cast<int>(strlen(wBuf));
            snprintf(hBuf, sizeof(hBuf), "%.0f", editHeight);
            hBufLen = static_cast<int>(strlen(hBuf));
            applyPropsToSelected();
        }
        else if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT))
        {
            resizing = false;
        }
        return;
    }

    if (dragging && selectedWidgetIndex >= 0)
    {
        Widget* selectedWidget = getSelectedWidget();
        if (selectedWidget && IsMouseButtonDown(MOUSE_BUTTON_LEFT))
        {
            float newX = mouseCanvasX - dragOffset.x;
            float newY = mouseCanvasY - dragOffset.y;
            if (snapToGrid)
            {
                newX = roundf(newX / GRID_SIZE) * GRID_SIZE;
                newY = roundf(newY / GRID_SIZE) * GRID_SIZE;
            }
            if (newX < 0) newX = 0;
            if (newY < 0) newY = 0;
            editX = newX;
            editY = newY;
            snprintf(xBuf, sizeof(xBuf), "%.0f", editX);
            xBufLen = static_cast<int>(strlen(xBuf));
            snprintf(yBuf, sizeof(yBuf), "%.0f", editY);
            yBufLen = static_cast<int>(strlen(yBuf));
            applyPropsToSelected();
        }
        else if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT))
        {
            dragging = false;
        }
        return;
    }

    if (mouseInCanvas && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && selectedWidgetIndex >= 0)
    {
        Widget* selectedWidget = getSelectedWidget();
        if (selectedWidget && (selectedWidget->getType() == WidgetType::Button || selectedWidget->getType() == WidgetType::Image || selectedWidget->getType() == WidgetType::ImageViewer || selectedWidget->getType() == WidgetType::RichTextBox))
        {
            float localX = mouseCanvasX;
            float localY = mouseCanvasY;
            Vector2 pos = selectedWidget->getPosition();
            Vector2 size = selectedWidget->getSize();
            float handleX = pos.x + size.x - 8;
            float handleY = pos.y + size.y - 8;

            if (localX >= handleX && localX <= handleX + 16 && localY >= handleY && localY <= handleY + 16)
            {
                resizing = true;
                return;
            }
        }
    }

    if (mouseInCanvas && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && activeField == Field::None)
    {
        float localX = mouseCanvasX;
        float localY = mouseCanvasY;

        auto& widgets = currentScreen->getWidgets();
        bool hitWidget = false;

        for (int i = static_cast<int>(widgets.size()) - 1; i >= 0; i--)
        {
            Widget* widget = widgets[i].get();
            Vector2 pos = widget->getPosition();
            Vector2 size = widget->getSize();

            float widgetW = (widget->getType() == WidgetType::Label) ?
                        MeasureTextEx(*uiManager.getFontManager().getFont(widget->font_index), widget->getText().c_str(), static_cast<float>(widget->font_size), 1).x : size.x;
            float widgetH = (widget->getType() == WidgetType::Label) ?
                        static_cast<float>(widget->font_size) : size.y;

            if (localX >= pos.x && localX <= pos.x + widgetW &&
                localY >= pos.y && localY <= pos.y + widgetH)
            {
                selectWidget(i);
                dragging = true;
                dragOffset = {localX - pos.x, localY - pos.y};
                hitWidget = true;
                break;
            }
        }

        if (!hitWidget) deselectWidget();
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && activeField != Field::None)
    {
        bool hitField = false;
        if (mouse.x >= GetVirtualScreenWidth() - PROP_PANEL_W)
        {
            hitField = true;
        }
        if (!hitField)
        {
            commitActiveField();
            activeField = Field::None;
            applyPropsToSelected();
        }
    }

    if (IsKeyPressed(KEY_ENTER) && activeField != Field::None && activeField != Field::RichTextContent)
    {
        commitActiveField();
        activeField = Field::None;
        applyPropsToSelected();
    }
}

void UIEditor::draw()
{
    if (!editorMode) return;
    drawCanvas();
    drawToolbar();
    drawScreenList();
    drawPropertyPanel();
    if (showFontPanel) drawFontPanel();
}

void UIEditor::drawToolbar()
{
    float w = static_cast<float>(GetVirtualScreenWidth());
    DrawRectangle(0, 0, static_cast<int>(w), static_cast<int>(TOOLBAR_H), {40, 40, 50, 255});
    DrawLine(0, static_cast<int>(TOOLBAR_H), static_cast<int>(w), static_cast<int>(TOOLBAR_H), GRAY);

    float x = 8;

    // Add widgets
    if (button({x, 4, 65, 32}, "+ Button", {50, 100, 140, 255}))
        addWidgetToScreen("button");
    x += 70;

    if (button({x, 4, 65, 32}, "+ Label", {50, 100, 140, 255}))
        addWidgetToScreen("label");
    x += 70;

    if (button({x, 4, 65, 32}, "+ Image", {50, 100, 140, 255}))
        addWidgetToScreen("image");
    x += 70;

    if (button({x, 4, 65, 32}, "+ RichText", {50, 120, 100, 255}))
        addWidgetToScreen("richtextbox");
    x += 70;

    if (button({x, 4, 65, 32}, "+ Viewer", {120, 80, 120, 255}))
        addWidgetToScreen("imageviewer");
    x += 70;

    if (button({x, 4, 55, 32}, "Fonts", {100, 80, 120, 255}))
        showFontPanel = !showFontPanel;
    x += 60;

    const char* snapLabel = snapToGrid ? "Snap: ON" : "Snap: OFF";
    Color snapColor = snapToGrid ? (Color){60, 130, 70, 255} : (Color){130, 60, 60, 255};
    if (button({x, 4, 75, 32}, snapLabel, snapColor))
        snapToGrid = !snapToGrid;
    x += 80;

    // Save
    if (button({w - 80, 4, 72, 32}, "SAVE", {140, 100, 40, 255}))
        uiManager.saveToFile("ui.json");
}

void UIEditor::drawScreenList()
{
    float screenH = static_cast<float>(GetVirtualScreenHeight());
    float canvasH = screenH - TOOLBAR_H;

    DrawRectangle(0, static_cast<int>(TOOLBAR_H), static_cast<int>(SIDEBAR_W),
                  static_cast<int>(canvasH), {30, 30, 40, 255});
    DrawLine(static_cast<int>(SIDEBAR_W), static_cast<int>(TOOLBAR_H),
             static_cast<int>(SIDEBAR_W), static_cast<int>(screenH), GRAY);

    DrawText("SCREENS", 10, static_cast<int>(TOOLBAR_H + 10), 12, LIGHTGRAY);

    float maxScroll = (screenListContentH > canvasH) ? (screenListContentH - canvasH) : 0.0f;
    if (screenListScroll > maxScroll) screenListScroll = maxScroll;
    if (screenListScroll < 0.0f) screenListScroll = 0.0f;

    BeginScissorMode(0, static_cast<int>(TOOLBAR_H), static_cast<int>(SIDEBAR_W), static_cast<int>(canvasH));

    float y = TOOLBAR_H + 30 - screenListScroll;
    auto& screens = uiManager.getScreens();
    for (auto& [name, screen] : screens)
    {
        bool isInit = (name == uiManager.getInitialScreenRef());
        bool isCurrent = (name == editingScreenName);
        Color bg = isCurrent ? (Color){60, 60, 90, 255} : (Color){40, 40, 50, 255};
        Rectangle r = {5, y, SIDEBAR_W - 10, 24};

        Vector2 m = GetVirtualMousePos();
        bool hover = CheckCollisionPointRec(m, r);
        if (hover) bg = { (unsigned char)(bg.r + 15), (unsigned char)(bg.g + 15), (unsigned char)(bg.b + 15), 255 };

        DrawRectangleRec(r, bg);

        const char* label = isInit ? TextFormat("* %s", name.c_str()) : name.c_str();
        DrawText(label, 12, static_cast<int>(y + 6), 11, WHITE);

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && hover)
        {
            editingScreenName = name;
            deselectWidget();
        }
        y += 28;
    }

    Screen* screen = getCurrentScreen();
    if (screen)
    {
        y += 8;
        DrawText(TextFormat("Widgets: %d", (int)screen->getWidgets().size()),
                 10, static_cast<int>(y), 11, LIGHTGRAY);

        y += 20;
        DrawText("[CTRL+D] dup", 10, static_cast<int>(y), 10, GRAY);
        y += 14;
        DrawText("[E] toggle", 10, static_cast<int>(y), 10, GRAY);
        y += 14;
        DrawText("[G] snap toggle", 10, static_cast<int>(y), 10, GRAY);
        y += 14;
        DrawText("[CTRL+I] set init", 10, static_cast<int>(y), 10, GRAY);
        y += 16;

        DrawText("-- Screens --", 10, static_cast<int>(y), 10, GRAY);
        y += 14;

        Rectangle nameRec = {10, y, SIDEBAR_W - 46, 16};
        if (textBox(nameRec, screenNameBuf, sizeof(screenNameBuf), screenNameBufLen, activeField == Field::ScreenName))
            activeField = Field::ScreenName;

        if (button({SIDEBAR_W - 34, y - 2, 28, 20}, "+", {60, 120, 60, 255}))
        {
            if (screenNameBufLen > 0)
            {
                std::string newName(screenNameBuf);
                uiManager.addScreen(newName);
                editingScreenName = newName;
                deselectWidget();
                screenNameBuf[0] = '\0';
                screenNameBufLen = 0;
            }
        }

        y += 22;

        if (uiManager.getScreens().size() > 1)
        {
            if (button({10, y, SIDEBAR_W - 20, 18}, "Delete Current", {140, 50, 50, 255}))
            {
                std::string toDelete = editingScreenName;
                uiManager.removeScreen(toDelete);
                auto& remaining = uiManager.getScreens();
                if (!remaining.empty() && !remaining.count(editingScreenName))
                {
                    editingScreenName = remaining.begin()->first;
                }
                deselectWidget();
            }
            y += 24;
        }
    }

    screenListContentH = y - TOOLBAR_H + screenListScroll;

    EndScissorMode();
}
void UIEditor::drawCanvas()
{
    float canvasW = static_cast<float>(GetVirtualScreenWidth());
    float canvasH = static_cast<float>(GetVirtualScreenHeight());

    rlPushMatrix();
    rlTranslatef(canvasPan.x, canvasPan.y, 0);
    rlScalef(canvasZoom, canvasZoom, 1);

    DrawRectangle(0, 0, static_cast<int>(canvasW), static_cast<int>(canvasH), {20, 20, 25, 255});

    if (snapToGrid)
    {
        for (float gx = 0; gx < canvasW; gx += GRID_SIZE)
        {
            DrawLine(static_cast<int>(gx), 0, static_cast<int>(gx), static_cast<int>(canvasH),
                     {35, 35, 40, 255});
        }
        for (float gy = 0; gy < canvasH; gy += GRID_SIZE)
        {
            DrawLine(0, static_cast<int>(gy), static_cast<int>(canvasW), static_cast<int>(gy),
                     {35, 35, 40, 255});
        }
    }

    Screen* currentScreen = getCurrentScreen();
    if (!currentScreen) { rlPopMatrix(); return; }

    Color bgColor = currentScreen->getBackgroundColor();
    DrawRectangle(1, 1, static_cast<int>(canvasW - 2), static_cast<int>(canvasH - 2), bgColor);

    if (currentScreen->getPattern() != BgPattern::None)
    {
        drawBackgroundPattern(
            currentScreen->getPattern(),
            currentScreen->getPatternColorA(),
            currentScreen->getPatternColorB(),
            currentScreen->getPatternTileSize(),
            static_cast<int>(canvasW - 2), static_cast<int>(canvasH - 2),
            currentScreen->getScrollOffsetX(), currentScreen->getScrollOffsetY());
    }

    if (currentScreen->hasBackgroundImage())
    {
        Texture2D bgTex = currentScreen->getBackgroundTexture();
        ImageFit bgFit = currentScreen->getBackgroundFit();
        auto r = computeImageFit(
            (float)bgTex.width, (float)bgTex.height,
            1, 1, canvasW - 2, canvasH - 2, bgFit);
        DrawTexturePro(bgTex, r.src, r.dst, {0, 0}, 0.0f, WHITE);
    }

    FontManager& fonts = uiManager.getFontManager();
    auto& widgets = currentScreen->getWidgets();

    Rectangle selRect = {};
    bool haveSel = false;
    Rectangle handleRect = {};
    bool haveHandle = false;

    for (int i = 0; i < static_cast<int>(widgets.size()); i++)
    {
        Vector2 pos = widgets[i]->getPosition();

        if (widgets[i]->getType() == WidgetType::Button)
        {
            Button* btn = static_cast<Button*>(widgets[i].get());
            Vector2 mousePos = GetVirtualMousePos();
            float mouseCanvasX = (mousePos.x - canvasPan.x) / canvasZoom;
            float mouseCanvasY = (mousePos.y - canvasPan.y) / canvasZoom;
            Vector2 btnPos = btn->getPosition();
            Vector2 btnSize = btn->getSize();
            bool hover = (mouseCanvasX >= btnPos.x && mouseCanvasX <= btnPos.x + btnSize.x &&
                          mouseCanvasY >= btnPos.y && mouseCanvasY <= btnPos.y + btnSize.y);
            bool press = hover && IsMouseButtonDown(MOUSE_BUTTON_LEFT);
            if (press) btn->current_color = btn->pressed_color;
            else if (hover) btn->current_color = btn->hover_color;
            else btn->current_color = btn->normal_color;
        }

        widgets[i]->draw(fonts);

        if (i == selectedWidgetIndex)
        {
            Font* widgetFont = fonts.getFont(widgets[i]->font_index);
            float widgetW = (widgets[i]->getType() == WidgetType::Label) ?
                        MeasureTextEx(*widgetFont, widgets[i]->getText().c_str(), static_cast<float>(widgets[i]->font_size), 1).x :
                        widgets[i]->getSize().x;
            float widgetH = (widgets[i]->getType() == WidgetType::Label) ?
                        static_cast<float>(widgets[i]->font_size) :
                        widgets[i]->getSize().y;

            float cx = pos.x - 3, cy = pos.y - 3;
            float cw = widgetW + 6, ch = widgetH + 6;
            selRect = {cx, cy, cw, ch};
            haveSel = true;

            if (widgets[i]->getType() == WidgetType::Button || widgets[i]->getType() == WidgetType::Image || widgets[i]->getType() == WidgetType::ImageViewer || widgets[i]->getType() == WidgetType::RichTextBox)
            {
                float hx = pos.x + widgets[i]->getSize().x - 6;
                float hy = pos.y + widgets[i]->getSize().y - 6;
                handleRect = {hx, hy, 12, 12};
                haveHandle = true;
            }
        }
    }

    rlPopMatrix();

    if (haveSel)
    {
        float sx = selRect.x * canvasZoom + canvasPan.x;
        float sy = selRect.y * canvasZoom + canvasPan.y;
        float sw = selRect.width * canvasZoom;
        float sh = selRect.height * canvasZoom;
        DrawRectangleLinesEx({sx, sy, sw, sh}, 1, YELLOW);

        if (haveHandle)
        {
            float hsx = handleRect.x * canvasZoom + canvasPan.x;
            float hsy = handleRect.y * canvasZoom + canvasPan.y;
            float hsw = handleRect.width * canvasZoom;
            float hsh = handleRect.height * canvasZoom;
            DrawRectangle(static_cast<int>(hsx), static_cast<int>(hsy), static_cast<int>(hsw), static_cast<int>(hsh), YELLOW);
            int hFontSize = static_cast<int>(10 * canvasZoom);
            if (hFontSize < 6) hFontSize = 6;
            DrawText(">", static_cast<int>(hsx + 3), static_cast<int>(hsy + 1), hFontSize, BLACK);
        }
    }

    float hue = fmodf(static_cast<float>(GetTime()) * 60.0f, 360.0f);
    DrawRectangleLinesEx({canvasPan.x, canvasPan.y, canvasW * canvasZoom, canvasH * canvasZoom},
                         2, ColorFromHSV(hue, 1.0f, 1.0f));

    Vector2 mousePos = GetVirtualMousePos();
    float screenLeft = canvasPan.x;
    float screenTop = canvasPan.y;
    float screenRight = canvasPan.x + canvasW * canvasZoom;
    float screenBottom = canvasPan.y + canvasH * canvasZoom;
    if (mousePos.x >= screenLeft && mousePos.x <= screenRight &&
        mousePos.y >= screenTop && mousePos.y <= screenBottom)
    {
        float canvasMouseX = (mousePos.x - canvasPan.x) / canvasZoom;
        float canvasMouseY = (mousePos.y - canvasPan.y) / canvasZoom;
        int fontSize = (canvasZoom >= 1.0f) ? 11 : static_cast<int>(11 * canvasZoom);
        if (fontSize < 6) fontSize = 6;
        DrawText(TextFormat("(%d, %d)", static_cast<int>(canvasMouseX), static_cast<int>(canvasMouseY)),
                 static_cast<int>(screenLeft + 5),
                 static_cast<int>(screenBottom - fontSize - 4), fontSize, LIGHTGRAY);
    }
}
