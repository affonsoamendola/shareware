#include "ui_editor.hpp"
#include "raylib.h"
#include "rlgl.h"
#include "pixel_scale.hpp"
#include <tinyfiledialogs.h>
#include <cstring>
#include <cstdio>
#include <cmath>

static Color hsvToRgb(float h, float s, float v)
{
    float c = v * s;
    float x = c * (1.0f - fabsf(fmodf(h / 60.0f, 2.0f) - 1.0f));
    float m = v - c;
    float r, g, b;
    if      (h < 60)  { r = c; g = x; b = 0; }
    else if (h < 120) { r = x; g = c; b = 0; }
    else if (h < 180) { r = 0; g = c; b = x; }
    else if (h < 240) { r = 0; g = x; b = c; }
    else if (h < 300) { r = x; g = 0; b = c; }
    else              { r = c; g = 0; b = x; }
    return {
        static_cast<unsigned char>((r + m) * 255),
        static_cast<unsigned char>((g + m) * 255),
        static_cast<unsigned char>((b + m) * 255),
        255
    };
}

static void rgbToHsv(int ri, int gi, int bi, float& h, float& s, float& v)
{
    float r = ri / 255.0f;
    float g = gi / 255.0f;
    float b = bi / 255.0f;
    float cmax = fmaxf(r, fmaxf(g, b));
    float cmin = fminf(r, fminf(g, b));
    float d = cmax - cmin;
    v = cmax;
    if (cmax == 0) { s = 0; h = 0; return; }
    s = d / cmax;
    if (d == 0) { h = 0; return; }
    if (cmax == r)      h = 60.0f * fmodf((g - b) / d, 6.0f);
    else if (cmax == g) h = 60.0f * ((b - r) / d + 2.0f);
    else                h = 60.0f * ((r - g) / d + 4.0f);
    if (h < 0) h += 360.0f;
}

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
        deselectWidget();
    }
    else
    {
        // Reset all RichTextBox interactivity when leaving editor
        Screen* screen = getCurrentScreen();
        if (screen)
        {
            for (auto& w : screen->getWidgets())
            {
                if (w->getType() == WidgetType::RichTextBox)
                    static_cast<RichTextBox*>(w.get())->interactive = false;
            }
        }
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
    // Disable interactive on previously selected RichTextBox
    Widget* old = getSelectedWidget();
    if (old && old->getType() == WidgetType::RichTextBox)
        static_cast<RichTextBox*>(old)->interactive = false;

    selectedWidgetIndex = index;

    // Enable interactive on newly selected RichTextBox
    Widget* now = getSelectedWidget();
    if (now && now->getType() == WidgetType::RichTextBox)
        static_cast<RichTextBox*>(now)->interactive = true;

    syncPropsFromSelected();
}

void UIEditor::deselectWidget()
{
    Widget* w = getSelectedWidget();
    if (w && w->getType() == WidgetType::RichTextBox)
        static_cast<RichTextBox*>(w)->interactive = false;

    selectedWidgetIndex = -1;
    textBuf[0] = '\0';
    actionBuf[0] = '\0';
    textBufLen = 0;
    actionBufLen = 0;
    activeField = -1;
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
    }
}

void UIEditor::applyPropsToSelected()
{
    Widget* widget = getSelectedWidget();
    if (!widget) return;

    widget->setText(textBuf);
    widget->font_size = editFontSize;
    widget->font_index = editFontIndex;
    widget->setPosition(editX, editY);
    if (widget->getType() == WidgetType::Button || widget->getType() == WidgetType::Image || widget->getType() == WidgetType::RichTextBox)
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
    }

    if (widget->getType() == WidgetType::Image)
    {
        ImageWidget* imgWidget = static_cast<ImageWidget*>(widget);
        if (strcmp(imgPathBuf, imgWidget->imagePath.c_str()) != 0)
            imgWidget->setImagePath(imgPathBuf);
        imgWidget->tint = colorToEdit(editTintR, editTintG, editTintB, editTintA);
        imgWidget->fit = static_cast<ImageFit>(editFitIndex);
    }

    if (widget->getType() == WidgetType::RichTextBox)
    {
        RichTextBox* rtb = static_cast<RichTextBox*>(widget);
        rtb->fromMarkdown(mdContentBuf);
        rtb->font_size = rtbFontSize;
        rtb->lineHeight = rtbLineSpacing;
        rtb->padding = rtbPadding;
        rtb->textAlign = static_cast<TextAlign>(rtbTextAlign);
    }
}

void UIEditor::drawRichTextBoxProperties(float& positionY, float labelX, float inputWidth)
{
    float valueX = labelX + 47;
    float controlW = inputWidth + 55;

    if (slider({labelX, positionY, controlW, 16}, "Size:", &rtbFontSize, 8, 72))
        applyPropsToSelected();
    positionY += 20;

    {
        FontManager& fonts = uiManager.getFontManager();
        DrawText("Font:", static_cast<int>(labelX), static_cast<int>(positionY), 11, LIGHTGRAY);

        float selectorW = inputWidth - 30;
        if (button({valueX, positionY - 2, 16, 16}, "<", {60, 60, 80, 255}))
        {
            editFontIndex--;
            if (editFontIndex < 0) editFontIndex = fonts.getCount() - 1;
            applyPropsToSelected();
        }

        const char* fontName = fonts.getFontName(editFontIndex).c_str();
        DrawRectangle(static_cast<int>(valueX + 18), static_cast<int>(positionY - 2),
                      static_cast<int>(selectorW - 36), 16, {40, 40, 50, 255});
        DrawText(fontName, static_cast<int>(valueX + 22), static_cast<int>(positionY + 1), 10, WHITE);

        if (button({valueX + selectorW - 14, positionY - 2, 16, 16}, ">", {60, 60, 80, 255}))
        {
            editFontIndex++;
            if (editFontIndex >= fonts.getCount()) editFontIndex = 0;
            applyPropsToSelected();
        }
    }
    positionY += 20;

    {
        int lineSpacingInt = static_cast<int>(rtbLineSpacing * 100);
        if (slider({labelX, positionY, controlW, 16}, "Line Sp:", &lineSpacingInt, 100, 300))
        {
            rtbLineSpacing = lineSpacingInt / 100.0f;
            applyPropsToSelected();
        }
    }
    positionY += 20;

    if (slider({labelX, positionY, controlW, 16}, "Pad:", &rtbPadding, 0, 30))
        applyPropsToSelected();
    positionY += 20;

    DrawText("Align:", static_cast<int>(labelX), static_cast<int>(positionY), 11, LIGHTGRAY);
    {
        float bw = controlW / 4;
        const char* labels[] = {"Left", "Center", "Right", "Justify"};
        for (int i = 0; i < 4; i++)
        {
            Color bg = (rtbTextAlign == i) ? (Color){70, 70, 130, 255} : (Color){50, 50, 60, 255};
            if (button({labelX + i * bw, positionY, bw - 2, 16}, labels[i], bg))
            {
                rtbTextAlign = i;
                applyPropsToSelected();
            }
        }
    }
    positionY += 20;

    DrawText("-- Content (Markdown) --", static_cast<int>(labelX), static_cast<int>(positionY), 10, GRAY);
    positionY += 14;
    Rectangle mdRec = {labelX, positionY, controlW, MD_BOX_H};
    if (textBox(mdRec, mdContentBuf, sizeof(mdContentBuf), mdContentBufLen, activeField == 30, true))
        activeField = 30;
    if (activeField == 30)
    {
        RichTextBox* rtb = static_cast<RichTextBox*>(getSelectedWidget());
        rtb->fromMarkdown(mdContentBuf);
    }
    positionY += MD_BOX_H + 4;
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

Color UIEditor::colorToEdit(int r, int g, int b, int a)
{
    return {(unsigned char)r, (unsigned char)g, (unsigned char)b, (unsigned char)a};
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

void UIEditor::colorPicker(const char* label, int& r, int& g, int& b, int& a, int id, float& positionY)
{
    float labelX = GetVirtualScreenWidth() - PROP_PANEL_W + 8;
    float inputWidth = PROP_PANEL_W - 65;
    Vector2 mousePos = GetVirtualMousePos();
    bool isExpanded = (expandedColorId == id);

    DrawText(label, static_cast<int>(labelX), static_cast<int>(positionY + 2), 10, LIGHTGRAY);

    Color currentColor = {static_cast<unsigned char>(r), static_cast<unsigned char>(g), static_cast<unsigned char>(b), static_cast<unsigned char>(a)};
    float swatchX = labelX + 50;
    DrawRectangle(static_cast<int>(swatchX), static_cast<int>(positionY), 14, 12, currentColor);
    DrawRectangleLines(static_cast<int>(swatchX), static_cast<int>(positionY), 14, 12, WHITE);

    Rectangle clickArea = {labelX, positionY - 2, 70, 16};
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mousePos, clickArea))
    {
        if (isExpanded)
        {
            expandedColorId = -1;
        }
        else
        {
            expandedColorId = id;
            activeColorR = &r;
            activeColorG = &g;
            activeColorB = &b;
            activeColorA = &a;
            rgbToHsv(r, g, b, pickerHue, pickerSat, pickerVal);
            snprintf(colorRBuf, sizeof(colorRBuf), "%d", r);
            snprintf(colorGBuf, sizeof(colorGBuf), "%d", g);
            snprintf(colorBBuf, sizeof(colorBBuf), "%d", b);
            snprintf(colorABuf, sizeof(colorABuf), "%d", a);
            colorRBufLen = static_cast<int>(strlen(colorRBuf));
            colorGBufLen = static_cast<int>(strlen(colorGBuf));
            colorBBufLen = static_cast<int>(strlen(colorBBuf));
            colorABufLen = static_cast<int>(strlen(colorABuf));
        }
    }

    positionY += 14;
    if (!isExpanded) return;

    float gridW = inputWidth + 55;
    if (gridW > 190) gridW = 190;
    float gridH = 100;
    float gridX = labelX;
    float gridY = positionY;
    float blockSize = 4.0f;

    for (float gy = 0; gy < gridH; gy += blockSize)
    {
        float val = 1.0f - gy / gridH;
        for (float gx = 0; gx < gridW; gx += blockSize)
        {
            float sat = gx / gridW;
            Color c = hsvToRgb(pickerHue, sat, val);
            DrawRectangle(
                static_cast<int>(gridX + gx),
                static_cast<int>(gridY + gy),
                static_cast<int>(blockSize),
                static_cast<int>(blockSize),
                c
            );
        }
    }

    float cursorX = gridX + pickerSat * gridW;
    float cursorY = gridY + (1.0f - pickerVal) * gridH;
    DrawRectangle(static_cast<int>(cursorX - 3), static_cast<int>(cursorY - 3), 6, 6, WHITE);
    DrawRectangle(static_cast<int>(cursorX - 2), static_cast<int>(cursorY - 2), 4, 4, BLACK);

    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mousePos, {gridX, gridY, gridW, gridH}))
    {
        pickerSat = (mousePos.x - gridX) / gridW;
        pickerVal = 1.0f - (mousePos.y - gridY) / gridH;
        if (pickerSat < 0) pickerSat = 0; if (pickerSat > 1) pickerSat = 1;
        if (pickerVal < 0) pickerVal = 0; if (pickerVal > 1) pickerVal = 1;
        Color c = hsvToRgb(pickerHue, pickerSat, pickerVal);
        r = c.r; g = c.g; b = c.b;
        snprintf(colorRBuf, sizeof(colorRBuf), "%d", r);
        snprintf(colorGBuf, sizeof(colorGBuf), "%d", g);
        snprintf(colorBBuf, sizeof(colorBBuf), "%d", b);
        colorRBufLen = static_cast<int>(strlen(colorRBuf));
        colorGBufLen = static_cast<int>(strlen(colorGBuf));
        colorBBufLen = static_cast<int>(strlen(colorBBuf));
        applyPropsToSelected();
    }

    float hueX = gridX + gridW + 6;
    float hueW = 14;
    int hueSegments = 36;
    float segmentH = gridH / hueSegments;
    for (int i = 0; i < hueSegments; i++)
    {
        float h = static_cast<float>(i) / hueSegments * 360.0f;
        Color c = hsvToRgb(h, 1.0f, 1.0f);
        DrawRectangle(
            static_cast<int>(hueX),
            static_cast<int>(gridY + i * segmentH),
            static_cast<int>(hueW),
            static_cast<int>(segmentH + 1),
            c
        );
    }

    float hueCursorY = gridY + (pickerHue / 360.0f) * gridH;
    DrawRectangle(static_cast<int>(hueX - 1), static_cast<int>(hueCursorY - 2), static_cast<int>(hueW + 2), 4, WHITE);

    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mousePos, {hueX - 2, gridY, hueW + 4, gridH}))
    {
        pickerHue = (mousePos.y - gridY) / gridH * 360.0f;
        if (pickerHue < 0) pickerHue = 0; if (pickerHue > 360) pickerHue = 360;
        Color c = hsvToRgb(pickerHue, pickerSat, pickerVal);
        r = c.r; g = c.g; b = c.b;
        snprintf(colorRBuf, sizeof(colorRBuf), "%d", r);
        snprintf(colorGBuf, sizeof(colorGBuf), "%d", g);
        snprintf(colorBBuf, sizeof(colorBBuf), "%d", b);
        colorRBufLen = static_cast<int>(strlen(colorRBuf));
        colorGBufLen = static_cast<int>(strlen(colorGBuf));
        colorBBufLen = static_cast<int>(strlen(colorBBuf));
        applyPropsToSelected();
    }

    positionY += gridH + 6;

    float entryW = (gridW + hueW + 6) / 2 - 4;
    float row1Y = positionY;
    float row2Y = positionY + 20;

    DrawText("R:", static_cast<int>(labelX), static_cast<int>(row1Y + 2), 10, LIGHTGRAY);
    Rectangle rRec = {labelX + 16, row1Y - 2, entryW, 16};
    if (textBox(rRec, colorRBuf, sizeof(colorRBuf), colorRBufLen, activeField == 20))
        activeField = 20;

    float gLabelX = labelX + entryW + 24;
    DrawText("G:", static_cast<int>(gLabelX), static_cast<int>(row1Y + 2), 10, LIGHTGRAY);
    Rectangle gRec = {gLabelX + 16, row1Y - 2, entryW, 16};
    if (textBox(gRec, colorGBuf, sizeof(colorGBuf), colorGBufLen, activeField == 21))
        activeField = 21;

    DrawText("B:", static_cast<int>(labelX), static_cast<int>(row2Y + 2), 10, LIGHTGRAY);
    Rectangle bRec = {labelX + 16, row2Y - 2, entryW, 16};
    if (textBox(bRec, colorBBuf, sizeof(colorBBuf), colorBBufLen, activeField == 22))
        activeField = 22;

    DrawText("A:", static_cast<int>(gLabelX), static_cast<int>(row2Y + 2), 10, LIGHTGRAY);
    Rectangle aRec = {gLabelX + 16, row2Y - 2, entryW, 16};
    if (textBox(aRec, colorABuf, sizeof(colorABuf), colorABufLen, activeField == 23))
        activeField = 23;

    if (activeField == 20 && IsKeyPressed(KEY_TAB)) { activeField = 21; }
    if (activeField == 21 && IsKeyPressed(KEY_TAB)) { activeField = 22; }
    if (activeField == 22 && IsKeyPressed(KEY_TAB)) { activeField = 23; }
    if (activeField == 23 && IsKeyPressed(KEY_TAB))
    {
        commitActiveField();
        activeField = -1;
        applyPropsToSelected();
    }

    positionY = row2Y + 20;
}

void UIEditor::commitActiveField()
{
    switch (activeField)
    {
        case 0: break;
        case 1: break;
        case 4:
        {
            Screen* currentScreen = getCurrentScreen();
            if (currentScreen) currentScreen->setBackgroundImage(bgImgPathBuf);
            break;
        }
        case 10:
            editX = strtof(xBuf, nullptr);
            break;
        case 11:
            editY = strtof(yBuf, nullptr);
            break;
        case 20: case 21: case 22: case 23:
            if (activeColorR)
            {
                int* target = (activeField == 20) ? activeColorR :
                              (activeField == 21) ? activeColorG :
                              (activeField == 22) ? activeColorB : activeColorA;
                const char* buf = (activeField == 20) ? colorRBuf :
                                  (activeField == 21) ? colorGBuf :
                                  (activeField == 22) ? colorBBuf : colorABuf;
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
        case 30:
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
    }
}

// ===================== Editor Logic =====================

void UIEditor::update(float dt)
{
    if (!editorMode) return;

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
            float contentH = 30.0f + uiManager.getScreens().size() * 28.0f + 66.0f;
            float visibleH = screenH - TOOLBAR_H;
            float maxScroll = (contentH > visibleH) ? (contentH - visibleH) : 0.0f;
            screenListScroll += delta;
            if (screenListScroll < 0.0f) screenListScroll = 0.0f;
            if (screenListScroll > maxScroll) screenListScroll = maxScroll;
        }
        else if (mousePos.x > screenW - PROP_PANEL_W && mousePos.y > TOOLBAR_H)
        {
            Widget* selectedWidget = getSelectedWidget();
            float contentH = 120.0f;
            if (selectedWidget)
            {
                if (selectedWidget->getType() == WidgetType::Button)
                {
                    contentH += 38.0f + 14.0f + 74.0f * 4;
                    if (actionTypeIndex == 0 || actionTypeIndex == 4 || actionTypeIndex == 5)
                        contentH += 24.0f;
                    if (actionTypeIndex == 0)
                        contentH += 12.0f + uiManager.getScreens().size() * 18.0f;
                    contentH += 64.0f;
                }
                else if (selectedWidget->getType() == WidgetType::Image)
                {
                    contentH += 60.0f + 20.0f + 74.0f + 80.0f;
                }
                else if (selectedWidget->getType() == WidgetType::RichTextBox)
                {
                    contentH += 74.0f + 80.0f + 84.0f;
                }
                else
                {
                    contentH += 74.0f;
                }
                contentH += 38.0f;
            }
            float visibleH = screenH - TOOLBAR_H;
            float maxScroll = (contentH > visibleH) ? (contentH - visibleH) : 0.0f;
            propPanelScroll += delta;
            if (propPanelScroll < 0.0f) propPanelScroll = 0.0f;
            if (propPanelScroll > maxScroll) propPanelScroll = maxScroll;
        }
    }

    if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_E)) { toggle(); return; }
    if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_D)) duplicateSelectedWidget();
    if (IsKeyPressed(KEY_DELETE)) deleteSelectedWidget();

    if (showFontPanel && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        Vector2 mousePos = GetVirtualMousePos();
        float panelW = 320.0f;
        float panelH = 340.0f;
        float panelX = (static_cast<float>(GetVirtualScreenWidth()) - panelW) / 2;
        float panelY = (static_cast<float>(GetVirtualScreenHeight()) - panelH) / 2;

        if (mousePos.x < panelX || mousePos.x > panelX + panelW || mousePos.y < panelY || mousePos.y > panelY + panelH)
        {
            if (activeField == 2) { fontPathActive = false; activeField = -1; }
            if (activeField == 3) { fontPreviewActive = false; activeField = -1; }
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
            newW = roundf(newW / GRID_SIZE) * GRID_SIZE;
            newH = roundf(newH / GRID_SIZE) * GRID_SIZE;
            editWidth = newW;
            editHeight = newH;
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
            newX = roundf(newX / GRID_SIZE) * GRID_SIZE;
            newY = roundf(newY / GRID_SIZE) * GRID_SIZE;
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
        if (selectedWidget && (selectedWidget->getType() == WidgetType::Button || selectedWidget->getType() == WidgetType::Image || selectedWidget->getType() == WidgetType::RichTextBox))
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

    if (mouseInCanvas && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && activeField == -1)
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

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && activeField != -1)
    {
        bool hitField = false;
        if (mouse.x >= GetVirtualScreenWidth() - PROP_PANEL_W)
        {
            hitField = true;
        }
        if (!hitField)
        {
            commitActiveField();
            activeField = -1;
            applyPropsToSelected();
        }
    }

    if (IsKeyPressed(KEY_ENTER) && activeField != -1 && activeField != 30)
    {
        commitActiveField();
        activeField = -1;
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

    if (button({x, 4, 55, 32}, "Fonts", {100, 80, 120, 255}))
        showFontPanel = !showFontPanel;

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

    float contentH = 30.0f + uiManager.getScreens().size() * 28.0f + 66.0f;
    float maxScroll = (contentH > canvasH) ? (contentH - canvasH) : 0.0f;
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
        DrawText("[DEL] delete", 10, static_cast<int>(y), 10, GRAY);
        y += 14;
        DrawText("[CTRL+D] dup", 10, static_cast<int>(y), 10, GRAY);
        y += 14;
        DrawText("[E] toggle", 10, static_cast<int>(y), 10, GRAY);
    }

    EndScissorMode();
}

void UIEditor::drawPropertyPanel()
{
    float screenH = static_cast<float>(GetVirtualScreenHeight());
    float x = static_cast<float>(GetVirtualScreenWidth()) - PROP_PANEL_W;
    float canvasH = screenH - TOOLBAR_H;

    DrawRectangle(static_cast<int>(x), static_cast<int>(TOOLBAR_H),
                  static_cast<int>(PROP_PANEL_W), static_cast<int>(canvasH),
                  {35, 35, 45, 255});
    DrawLine(static_cast<int>(x), static_cast<int>(TOOLBAR_H),
             static_cast<int>(x), static_cast<int>(screenH), GRAY);

    Widget* w = getSelectedWidget();
    if (!w)
    {
        DrawText("No selection", static_cast<int>(x + 10), static_cast<int>(TOOLBAR_H + 30), 11, GRAY);
        DrawText("Click a widget", static_cast<int>(x + 10), static_cast<int>(TOOLBAR_H + 46), 10, DARKGRAY);

        Screen* screen = getCurrentScreen();
        if (screen)
        {
            float py = TOOLBAR_H + 80;
            float lx = x + 8;
            float iw = PROP_PANEL_W - 65;

            DrawText("-- Screen --", static_cast<int>(lx), static_cast<int>(py), 11, LIGHTGRAY);
            py += 18;

            DrawText("BG Image:", static_cast<int>(lx), static_cast<int>(py), 11, LIGHTGRAY);
            Rectangle bgRec = {lx + 55, py - 2, iw, 18};
            if (textBox(bgRec, bgImgPathBuf, sizeof(bgImgPathBuf), bgImgPathBufLen, activeField == 4))
                activeField = 4;
            if (activeField == 4 && IsKeyPressed(KEY_TAB))
            {
                commitActiveField();
                activeField = -1;
            }
            py += 22;

            if (button({lx + 55, py, 70, 18}, "Select File", {60, 80, 120, 255}))
            {
                const char* filterPatterns[] = { "*.png", "*.jpg", "*.jpeg", "*.bmp", "*.gif", "*.tga" };
                const char* result = tinyfd_openFileDialog("Select Background Image", "", 6, filterPatterns, "Image Files", 0);
                if (result)
                {
                    strncpy(bgImgPathBuf, result, sizeof(bgImgPathBuf) - 1);
                    bgImgPathBuf[sizeof(bgImgPathBuf) - 1] = '\0';
                    bgImgPathBufLen = static_cast<int>(strlen(bgImgPathBuf));
                    screen->setBackgroundImage(bgImgPathBuf);
                }
            }

            if (button({lx + 130, py, 50, 18}, "Load", {60, 100, 60, 255}))
            {
                screen->setBackgroundImage(bgImgPathBuf);
            }

            if (screen->hasBackgroundImage())
            {
                if (button({lx + 185, py, 50, 18}, "Clear", {120, 60, 60, 255}))
                {
                    screen->setBackgroundImage("");
                    bgImgPathBuf[0] = '\0';
                    bgImgPathBufLen = 0;
                }
            }
            py += 28;

            if (screen->hasBackgroundImage())
            {
                DrawText("BG Preview:", static_cast<int>(lx), static_cast<int>(py), 10, GRAY);
                py += 12;
                float previewW = iw + 55;
                float previewH = 60.0f;
                DrawRectangleRec({lx, py, previewW, previewH}, {25, 25, 35, 255});
                DrawRectangleLinesEx({lx, py, previewW, previewH}, 1, DARKGRAY);
                // Just draw the background texture scaled into the preview box
                // The screen background texture is internal, so we re-load for preview
                // Actually, we can reference the screen's background texture via its method
                // For simplicity, just show the path
                DrawText(screen->getBackgroundImagePath().c_str(),
                    static_cast<int>(lx + 4), static_cast<int>(py + 4), 9, LIGHTGRAY);
                py += previewH + 8;

                DrawText("BG Fit:", static_cast<int>(lx), static_cast<int>(py + 2), 11, LIGHTGRAY);
                {
                    float controlW = iw + 55;
                    float bw = controlW / 3;
                    const char* fitLabels[] = {"Stretch", "Contain", "Cover"};
                    for (int i = 0; i < 3; i++)
                    {
                        Color bg = (editBgFitIndex == i) ? (Color){70, 70, 130, 255} : (Color){50, 50, 60, 255};
                        if (button({lx + i * bw, py, bw - 2, 16}, fitLabels[i], bg))
                        {
                            editBgFitIndex = i;
                            screen->setBackgroundFit(static_cast<ImageFit>(editBgFitIndex));
                        }
                    }
                }
                py += 20;
            }
        }

        return;
    }

    BeginScissorMode(static_cast<int>(x), static_cast<int>(TOOLBAR_H),
                     static_cast<int>(PROP_PANEL_W), static_cast<int>(canvasH));

    float py = TOOLBAR_H + 30 - propPanelScroll;
    DrawText("PROPERTIES", static_cast<int>(x + 10), static_cast<int>(py), 12, LIGHTGRAY);
    py += 20;
    float lx = x + 8;
    float vx = x + 55;
    float iw = PROP_PANEL_W - 65;

    // Type
    const char* typeName = "Unknown";
    if (w->getType() == WidgetType::Button) typeName = "Button";
    else if (w->getType() == WidgetType::Label) typeName = "Label";
    else if (w->getType() == WidgetType::Image) typeName = "Image";
    else if (w->getType() == WidgetType::RichTextBox) typeName = "RichTextBox";
    DrawText("Type:", static_cast<int>(lx), static_cast<int>(py), 11, LIGHTGRAY);
    DrawText(typeName, static_cast<int>(vx), static_cast<int>(py), 11, WHITE);
    py += 18;

    // Text / Image path
    if (w->getType() == WidgetType::Image)
    {
        DrawText("Image:", static_cast<int>(lx), static_cast<int>(py), 11, LIGHTGRAY);
        Rectangle imgRec = {vx, py - 2, iw, 18};
        if (textBox(imgRec, imgPathBuf, sizeof(imgPathBuf), imgPathBufLen, activeField == 0))
            activeField = 0;
        if (activeField == 0 && IsKeyPressed(KEY_TAB))
        {
            commitActiveField();
            activeField = -1;
            applyPropsToSelected();
        }
        py += 22;

        // Select File button
        if (button({lx, py, (iw + 55) / 2 - 2, 18}, "Select File", {60, 80, 120, 255}))
        {
            const char* filterPatterns[] = { "*.png", "*.jpg", "*.jpeg", "*.bmp", "*.gif", "*.tga" };
            const char* result = tinyfd_openFileDialog("Select Image", "", 6, filterPatterns, "Image Files", 0);
            if (result)
            {
                strncpy(imgPathBuf, result, sizeof(imgPathBuf) - 1);
                imgPathBuf[sizeof(imgPathBuf) - 1] = '\0';
                imgPathBufLen = static_cast<int>(strlen(imgPathBuf));
                applyPropsToSelected();
            }
        }

        // Load button
        if (button({lx + (iw + 55) / 2 + 2, py, (iw + 55) / 2 - 2, 18}, "Load Image", {60, 100, 60, 255}))
        {
            applyPropsToSelected();
        }
        py += 24;

        // Texture preview
            ImageWidget* imgWidget = static_cast<ImageWidget*>(w);
        if (imgWidget->textureLoaded)
        {
            DrawText("Preview:", static_cast<int>(lx), static_cast<int>(py), 10, GRAY);
            py += 12;
            float previewW = iw + 55;
            float previewH = 60.0f;
            DrawRectangleRec({lx, py, previewW, previewH}, {25, 25, 35, 255});
            DrawRectangleLinesEx({lx, py, previewW, previewH}, 1, DARKGRAY);

            // Scale texture to fit preview
            float texW = (float)imgWidget->texture.width;
            float texH = (float)imgWidget->texture.height;
            float scale = (previewW / texW < previewH / texH) ? previewW / texW : previewH / texH;
            float drawW = texW * scale;
            float drawH = texH * scale;
            float drawX = lx + (previewW - drawW) / 2;
            float drawY = py + (previewH - drawH) / 2;
            DrawTexturePro(imgWidget->texture,
                {0, 0, (float)imgWidget->texture.width, (float)imgWidget->texture.height},
                {drawX, drawY, drawW, drawH},
                {0, 0}, 0.0f, imgWidget->tint);
            py += previewH + 8;
        }
    }
    else if (w->getType() == WidgetType::RichTextBox)
    {
        drawRichTextBoxProperties(py, lx, iw);
    }
    else
    {
        DrawText("Text:", static_cast<int>(lx), static_cast<int>(py), 11, LIGHTGRAY);
        Rectangle textRec = {vx, py - 2, iw, 18};
        if (textBox(textRec, textBuf, sizeof(textBuf), textBufLen, activeField == 0))
            activeField = 0;
        if (activeField == 0 && IsKeyPressed(KEY_TAB))
        {
            commitActiveField();
            activeField = -1;
            applyPropsToSelected();
        }
        py += 22;

        // Font size
        {
            Rectangle sr = {lx, py, iw + 55, 16};
            if (slider(sr, "Size:", &editFontSize, 8, 72))
                applyPropsToSelected();
        }
        py += 20;

        // Font selector
        {
            FontManager& fonts = uiManager.getFontManager();
            DrawText("Font:", static_cast<int>(lx), static_cast<int>(py), 11, LIGHTGRAY);

            float selectorX = vx;
            float selectorW = iw - 30;

            // Previous button
            if (button({selectorX, py - 2, 16, 16}, "<", {60, 60, 80, 255}))
            {
                editFontIndex--;
                if (editFontIndex < 0) editFontIndex = fonts.getCount() - 1;
                applyPropsToSelected();
            }

            // Font name display
            const char* fontName = fonts.getFontName(editFontIndex).c_str();
            DrawRectangle(static_cast<int>(selectorX + 18), static_cast<int>(py - 2),
                          static_cast<int>(selectorW - 36), 16, {40, 40, 50, 255});
            DrawText(fontName, static_cast<int>(selectorX + 22), static_cast<int>(py + 1), 10, WHITE);

            // Next button
            if (button({selectorX + selectorW - 14, py - 2, 16, 16}, ">", {60, 60, 80, 255}))
            {
                editFontIndex++;
                if (editFontIndex >= fonts.getCount()) editFontIndex = 0;
                applyPropsToSelected();
            }
        }
        py += 20;
    }

    // Position (X and Y side by side)
    {
        float fieldW = (iw + 55) / 2 - 20;
        DrawText("X:", static_cast<int>(lx), static_cast<int>(py + 2), 10, LIGHTGRAY);
        Rectangle xRec = {lx + 16, py - 2, fieldW, 16};
        if (textBox(xRec, xBuf, sizeof(xBuf), xBufLen, activeField == 10))
            activeField = 10;

        float yLabelX = lx + 16 + fieldW + 8;
        DrawText("Y:", static_cast<int>(yLabelX), static_cast<int>(py + 2), 10, LIGHTGRAY);
        Rectangle yRec = {yLabelX + 16, py - 2, fieldW, 16};
        if (textBox(yRec, yBuf, sizeof(yBuf), yBufLen, activeField == 11))
            activeField = 11;

        if (activeField == 10 && IsKeyPressed(KEY_TAB))
        {
            commitActiveField();
            activeField = 11;
        }
        if (activeField == 11 && IsKeyPressed(KEY_TAB))
        {
            commitActiveField();
            activeField = -1;
            applyPropsToSelected();
        }
    }
    py += 22;

    // Size (buttons, images, richtext)
    if (w->getType() == WidgetType::Button || w->getType() == WidgetType::Image || w->getType() == WidgetType::RichTextBox)
    {
        Rectangle sr = {lx, py, iw + 55, 16};
        if (sliderF(sr, "W:", &editWidth, 20, 600))
            applyPropsToSelected();
        py += 18;
        sr = {lx, py, iw + 55, 16};
        if (sliderF(sr, "H:", &editHeight, 10, 300))
            applyPropsToSelected();
        py += 20;
    }

    py += 4;
    DrawText("-- Colors --", static_cast<int>(lx), static_cast<int>(py), 10, GRAY);
    py += 14;

    if (w->getType() == WidgetType::Image)
    {
        colorPicker("Tint:", editTintR, editTintG, editTintB, editTintA, 0, py);

        DrawText("Fit:", static_cast<int>(lx), static_cast<int>(py + 2), 11, LIGHTGRAY);
        {
            float controlW = iw + 55;
            float bw = controlW / 3;
            const char* fitLabels[] = {"Stretch", "Contain", "Cover"};
            for (int i = 0; i < 3; i++)
            {
                Color bg = (editFitIndex == i) ? (Color){70, 70, 130, 255} : (Color){50, 50, 60, 255};
                if (button({lx + i * bw, py, bw - 2, 16}, fitLabels[i], bg))
                {
                    editFitIndex = i;
                    applyPropsToSelected();
                }
            }
        }
        py += 20;
    }
    else
    {
        colorPicker("Text Color:", editTxtR, editTxtG, editTxtB, editTxtA, 0, py);

        if (w->getType() == WidgetType::Button)
        {
            colorPicker("Normal:", editColorR, editColorG, editColorB, editColorA, 1, py);
            colorPicker("Hover:", editHoverR, editHoverG, editHoverB, editHoverA, 2, py);
            colorPicker("Pressed:", editPressR, editPressG, editPressB, editPressA, 3, py);
        }
    }

    // Action (buttons only)
    if (w->getType() == WidgetType::Button)
    {
        py += 4;
        DrawText("-- Action --", static_cast<int>(lx), static_cast<int>(py), 10, GRAY);
        py += 14;

        const char* actionTypes[] = {"goto:", "quit", "fullscr", "pause", "run:", "custom"};
        float bw = (PROP_PANEL_W - 24) / 2;
        for (int i = 0; i < 6; i++)
        {
            Color bg = (actionTypeIndex == i) ? (Color){70, 70, 130, 255} : (Color){50, 50, 60, 255};
            float bx = lx + (i % 2) * (bw + 4);
            float by = py + (i / 2) * 20;
            if (button({bx, by, bw, 16}, actionTypes[i], bg))
            {
                actionTypeIndex = i;
                applyPropsToSelected();
            }
        }
        py += 64;

        // Action value input
        if (actionTypeIndex == 0 || actionTypeIndex == 4 || actionTypeIndex == 5)
        {
            const char* actLabel = (actionTypeIndex == 0) ? "Target:" :
                                   (actionTypeIndex == 4) ? "Command:" : "Value:";
            DrawText(actLabel, static_cast<int>(lx), static_cast<int>(py), 11, LIGHTGRAY);
            Rectangle actRec = {vx, py - 2, iw, 18};
            if (textBox(actRec, actionBuf, sizeof(actionBuf), actionBufLen, activeField == 1))
                activeField = 1;
            if (activeField == 1 && IsKeyPressed(KEY_TAB))
            {
                commitActiveField();
                activeField = -1;
                applyPropsToSelected();
            }
            py += 24;
        }

        // Screen target for goto
        if (actionTypeIndex == 0)
        {
            DrawText("Screens:", static_cast<int>(lx), static_cast<int>(py), 10, GRAY);
            py += 12;
            auto& screens = uiManager.getScreens();
            for (auto& [sname, s] : screens)
            {
                Color bg = (strcmp(actionBuf, sname.c_str()) == 0) ?
                           (Color){60, 60, 100, 255} : (Color){45, 45, 55, 255};
                Rectangle r = {lx, py, iw, 16};
                if (button(r, sname.c_str(), bg))
                {
                    strncpy(actionBuf, sname.c_str(), sizeof(actionBuf) - 1);
                    actionBuf[sizeof(actionBuf) - 1] = '\0';
                    actionBufLen = static_cast<int>(strlen(actionBuf));
                    applyPropsToSelected();
                }
                py += 18;
            }
        }
    }

    // Delete button (scrolls with content)
    py += 10;
    if (button({lx, py, PROP_PANEL_W - 16, 28}, "DELETE WIDGET", {140, 50, 50, 255}))
    {
        deleteSelectedWidget();
    }

    EndScissorMode();
}

void UIEditor::drawFontPanel()
{
    FontManager& fonts = uiManager.getFontManager();

    float panelW = 320.0f;
    float panelH = 340.0f;
    float px = (static_cast<float>(GetVirtualScreenWidth()) - panelW) / 2;
    float py = (static_cast<float>(GetVirtualScreenHeight()) - panelH) / 2;

    // Panel background
    DrawRectangleRec({px, py, panelW, panelH}, {30, 30, 40, 245});
    DrawRectangleLinesEx({px, py, panelW, panelH}, 2, GRAY);

    // Title
    DrawText("FONT MANAGER", static_cast<int>(px + 10), static_cast<int>(py + 10), 14, LIGHTGRAY);

    // Close button
    if (button({px + panelW - 28, py + 6, 22, 20}, "X", {120, 50, 50, 255}))
    {
        showFontPanel = false;
    }

    float fy = py + 34;
    float flx = px + 10;
    float fvx = px + 60;
    float fiw = panelW - 70;

    // Font list
    DrawText("Loaded Fonts:", static_cast<int>(flx), static_cast<int>(fy), 11, LIGHTGRAY);
    fy += 16;

    float listH = 80.0f;
    DrawRectangleRec({flx, fy, fiw, listH}, {20, 20, 30, 255});
    DrawRectangleLinesEx({flx, fy, fiw, listH}, 1, DARKGRAY);

    float contentH = fonts.getCount() * 18.0f;
    float maxScroll = (contentH > listH) ? (contentH - listH) : 0.0f;
    if (fontListScroll > maxScroll) fontListScroll = maxScroll;
    if (fontListScroll < 0.0f) fontListScroll = 0.0f;

    BeginScissorMode(static_cast<int>(flx), static_cast<int>(fy),
                     static_cast<int>(fiw), static_cast<int>(listH));

    for (int i = 0; i < fonts.getCount(); i++)
    {
        float itemY = fy + i * 18 - fontListScroll;

        bool isSelected = (i == selectedFontIndex);
        bool isDefault = (i == fonts.getDefaultIndex());
        Color bg = isSelected ? (Color){60, 60, 120, 255} : (Color){35, 35, 45, 255};

        DrawRectangleRec({flx + 1, itemY + 1, fiw - 2, 16}, bg);

        const char* label = isDefault ?
            TextFormat("* %s (default)", fonts.getFontName(i).c_str()) :
            fonts.getFontName(i).c_str();
        DrawText(label, static_cast<int>(flx + 5), static_cast<int>(itemY + 3), 10, WHITE);

        Vector2 m = GetVirtualMousePos();
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
            CheckCollisionPointRec(m, {flx, itemY, fiw, 18}))
        {
            selectedFontIndex = i;
        }
    }

    EndScissorMode();

    fy += listH + 8;

    // Load font from path
    DrawText("Load Font:", static_cast<int>(flx), static_cast<int>(fy), 11, LIGHTGRAY);
    fy += 14;

    Rectangle pathRec = {flx, fy - 2, fiw - 50, 18};
    if (textBox(pathRec, fontPathBuf, sizeof(fontPathBuf), fontPathBufLen, fontPathActive))
    {
        fontPathActive = true;
        fontPreviewActive = false;
        activeField = 2;
    }

    if (button({flx + fiw - 46, fy - 2, 44, 18}, "Load", {60, 100, 60, 255}))
    {
        if (fontPathBufLen > 0)
        {
            int loaded = fonts.loadFont(fontPathBuf);
            if (loaded >= 0)
            {
                selectedFontIndex = loaded;
                fontPathBuf[0] = '\0';
                fontPathBufLen = 0;
            }
        }
    }
    fy += 24;

    // Remove font
    if (selectedFontIndex > 0)
    {
        if (button({flx, fy, fiw, 18}, "Remove Selected Font", {120, 50, 50, 255}))
        {
            fonts.removeFont(selectedFontIndex);
            selectedFontIndex = -1;
        }
        fy += 24;
    }

    // Set default
    if (selectedFontIndex > 0 && selectedFontIndex != fonts.getDefaultIndex())
    {
        if (button({flx, fy, fiw, 18}, "Set as Default", {60, 80, 120, 255}))
        {
            fonts.setDefaultFont(selectedFontIndex);
        }
        fy += 24;
    }

    fy += 6;

    // Visualizer
    DrawText("-- Preview --", static_cast<int>(flx), static_cast<int>(fy), 10, GRAY);
    fy += 14;

    DrawText("Text:", static_cast<int>(flx), static_cast<int>(fy), 10, LIGHTGRAY);
    Rectangle prevRec = {flx + 35, fy - 2, fiw - 35, 18};
    if (textBox(prevRec, fontPreviewBuf, sizeof(fontPreviewBuf), fontPreviewBufLen, fontPreviewActive))
    {
        fontPreviewActive = true;
        fontPathActive = false;
        activeField = 3;
    }
    fy += 22;

    // Preview size slider
    {
        Rectangle sr = {flx, fy, fiw, 16};
        if (slider(sr, "Size:", &fontPreviewSize, 8, 72)) {}
    }
    fy += 22;

    // Render preview
    DrawRectangleRec({flx, fy, fiw, 50}, {25, 25, 35, 255});
    DrawRectangleLinesEx({flx, fy, fiw, 50}, 1, DARKGRAY);

    if (selectedFontIndex >= 0 && fontPreviewBufLen > 0)
    {
        Font* font = fonts.getFont(selectedFontIndex);
        DrawTextEx(*font, fontPreviewBuf,
                   {flx + 6, fy + 6},
                   (float)fontPreviewSize, 1, WHITE);
    }
}

void UIEditor::drawCanvas()
{
    float canvasW = static_cast<float>(GetVirtualScreenWidth());
    float canvasH = static_cast<float>(GetVirtualScreenHeight());

    rlPushMatrix();
    rlTranslatef(canvasPan.x, canvasPan.y, 0);
    rlScalef(canvasZoom, canvasZoom, 1);

    DrawRectangle(0, 0, static_cast<int>(canvasW), static_cast<int>(canvasH), {20, 20, 25, 255});

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

    Screen* currentScreen = getCurrentScreen();
    if (!currentScreen) { rlPopMatrix(); return; }

    Color bgColor = currentScreen->getBackgroundColor();
    DrawRectangle(1, 1, static_cast<int>(canvasW - 2), static_cast<int>(canvasH - 2), bgColor);

    if (currentScreen->hasBackgroundImage())
    {
        Texture2D bgTex = currentScreen->getBackgroundTexture();
        ImageFit bgFit = currentScreen->getBackgroundFit();
        float texW = (float)bgTex.width;
        float texH = (float)bgTex.height;
        float dstX = 1;
        float dstY = 1;
        float dstW = canvasW - 2;
        float dstH = canvasH - 2;
        float srcX = 0, srcY = 0, srcW = texW, srcH = texH;

        if (bgFit == ImageFit::Contain)
        {
            float scale = (dstW / texW < dstH / texH) ? dstW / texW : dstH / texH;
            float drawW = texW * scale;
            float drawH = texH * scale;
            dstX = 1.0f + (dstW - drawW) / 2.0f;
            dstY = 1.0f + (dstH - drawH) / 2.0f;
            dstW = drawW;
            dstH = drawH;
        }
        else if (bgFit == ImageFit::Cover)
        {
            float scale = (dstW / texW > dstH / texH) ? dstW / texW : dstH / texH;
            srcX = texW / 2.0f - dstW / (2.0f * scale);
            srcY = texH / 2.0f - dstH / (2.0f * scale);
            srcW = dstW / scale;
            srcH = dstH / scale;
        }

        DrawTexturePro(bgTex,
            {srcX, srcY, srcW, srcH},
            {dstX, dstY, dstW, dstH},
            {0, 0}, 0.0f, WHITE);
    }

    FontManager& fonts = uiManager.getFontManager();
    auto& widgets = currentScreen->getWidgets();
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

            Rectangle selRect = {pos.x - 3, pos.y - 3, widgetW + 6, widgetH + 6};
            DrawRectangleLinesEx(selRect, 2, YELLOW);

            if (widgets[i]->getType() == WidgetType::Button || widgets[i]->getType() == WidgetType::Image || widgets[i]->getType() == WidgetType::RichTextBox)
            {
                float handleX = pos.x + widgets[i]->getSize().x - 6;
                float handleY = pos.y + widgets[i]->getSize().y - 6;
                DrawRectangle(static_cast<int>(handleX), static_cast<int>(handleY), 12, 12, YELLOW);
                DrawText(">", static_cast<int>(handleX + 3), static_cast<int>(handleY + 1), 10, BLACK);
            }
        }
    }

    rlPopMatrix();

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
