#include "ui_editor.hpp"
#include "editor_controls.hpp"
#include "image_utils.hpp"
#include "pixel_scale.hpp"
#include <tinyfiledialogs.h>
#include <cstring>
#include <cstdio>

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

    colorPicker("BG Color:", editRtbBgR, editRtbBgG, editRtbBgB, editRtbBgA, 10, positionY);
    positionY += 6;

    DrawText("-- Content (Markdown) --", static_cast<int>(labelX), static_cast<int>(positionY), 10, GRAY);
    positionY += 14;
    Rectangle mdRec = {labelX, positionY, controlW, MD_BOX_H};
    if (textBox(mdRec, mdContentBuf, sizeof(mdContentBuf), mdContentBufLen, activeField == Field::RichTextContent, true))
        activeField = Field::RichTextContent;
    if (activeField == Field::RichTextContent)
    {
        RichTextBox* rtb = static_cast<RichTextBox*>(getSelectedWidget());
        rtb->fromMarkdown(mdContentBuf);
    }
    positionY += MD_BOX_H + 4;
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
    if (textBox(rRec, colorRBuf, sizeof(colorRBuf), colorRBufLen, activeField == Field::ColorR))
        activeField = Field::ColorR;

    float gLabelX = labelX + entryW + 24;
    DrawText("G:", static_cast<int>(gLabelX), static_cast<int>(row1Y + 2), 10, LIGHTGRAY);
    Rectangle gRec = {gLabelX + 16, row1Y - 2, entryW, 16};
    if (textBox(gRec, colorGBuf, sizeof(colorGBuf), colorGBufLen, activeField == Field::ColorG))
        activeField = Field::ColorG;

    DrawText("B:", static_cast<int>(labelX), static_cast<int>(row2Y + 2), 10, LIGHTGRAY);
    Rectangle bRec = {labelX + 16, row2Y - 2, entryW, 16};
    if (textBox(bRec, colorBBuf, sizeof(colorBBuf), colorBBufLen, activeField == Field::ColorB))
        activeField = Field::ColorB;

    DrawText("A:", static_cast<int>(gLabelX), static_cast<int>(row2Y + 2), 10, LIGHTGRAY);
    Rectangle aRec = {gLabelX + 16, row2Y - 2, entryW, 16};
    if (textBox(aRec, colorABuf, sizeof(colorABuf), colorABufLen, activeField == Field::ColorA))
        activeField = Field::ColorA;

    if (activeField == Field::ColorR && IsKeyPressed(KEY_TAB)) { activeField = Field::ColorG; }
    if (activeField == Field::ColorG && IsKeyPressed(KEY_TAB)) { activeField = Field::ColorB; }
    if (activeField == Field::ColorB && IsKeyPressed(KEY_TAB)) { activeField = Field::ColorA; }
    if (activeField == Field::ColorA && IsKeyPressed(KEY_TAB))
    {
        commitActiveField();
        activeField = Field::None;
        applyPropsToSelected();
    }

    positionY = row2Y + 20;
}

float UIEditor::beginPropPanelScroll(float x, float y, float w, float h)
{
    BeginScissorMode(static_cast<int>(x), static_cast<int>(y),
                     static_cast<int>(w), static_cast<int>(h));
    return y - propPanelScroll;
}

void UIEditor::endPropPanelScroll()
{
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
            float py = beginPropPanelScroll(x, TOOLBAR_H, PROP_PANEL_W, canvasH) + 80;
            float lx = x + 8;
            float iw = PROP_PANEL_W - 65;

            DrawText("-- Screen --", static_cast<int>(lx), static_cast<int>(py), 11, LIGHTGRAY);
            py += 18;

            colorPicker("BG Color:", editBgColorR, editBgColorG, editBgColorB, editBgColorA, -100, py);
            py += 6;

            DrawText("BG Pattern:", static_cast<int>(lx), static_cast<int>(py), 11, LIGHTGRAY);
            py += 14;
            {
                float controlW = iw + 55;
                int patCount = bgPatternCount();
                for (int i = 0; i < patCount; i++)
                {
                    Color bg = (editBgPatternIndex == i) ? (Color){70, 70, 130, 255} : (Color){50, 50, 60, 255};
                    if (button({lx, py, controlW - 2, 16}, bgPatternNameByIndex(i), bg))
                    {
                        editBgPatternIndex = i;
                        screen->setPattern(static_cast<BgPattern>(editBgPatternIndex));
                    }
                    py += 18.0f;
                }
                py += 4;
            }

            if (editBgPatternIndex != 0)
            {
                colorPicker("Pat Color A:", editPatColorAR, editPatColorAG, editPatColorAB, editPatColorAA, -200, py);
                py += 6;
                colorPicker("Pat Color B:", editPatColorBR, editPatColorBG, editPatColorBB, editPatColorBA, -201, py);
                py += 6;
                if (slider({lx, py, iw + 55, 16}, "Tile Size:", &editPatTileSize, 4, 64))
                {
                    screen->setPatternTileSize(editPatTileSize);
                }
                py += 20;

                DrawText("Scroll Dir:", static_cast<int>(lx), static_cast<int>(py), 11, LIGHTGRAY);
                {
                    float controlW = iw + 55;
                    float bw = controlW / 4;
                    const char* dirLabels[] = {"None", "H", "V", "Diag"};
                    for (int i = 0; i < 4; i++)
                    {
                        Color bg = (editScrollDirIndex == i) ? (Color){70, 70, 130, 255} : (Color){50, 50, 60, 255};
                        if (button({lx + i * bw, py, bw - 2, 16}, dirLabels[i], bg))
                        {
                            editScrollDirIndex = i;
                            screen->setScrollDirection(static_cast<BgScrollDirection>(editScrollDirIndex));
                        }
                    }
                }
                py += 20;

                if (editScrollDirIndex != 0)
                {
                    if (slider({lx, py, iw + 55, 16}, "Speed:", &editScrollSpeed, 10, 500))
                    {
                        screen->setScrollSpeed(static_cast<float>(editScrollSpeed));
                    }
                    py += 20;
                }
            }

            DrawText("BG Image:", static_cast<int>(lx), static_cast<int>(py), 11, LIGHTGRAY);
            Rectangle bgRec = {lx + 55, py - 2, iw, 18};
            if (textBox(bgRec, bgImgPathBuf, sizeof(bgImgPathBuf), bgImgPathBufLen, activeField == Field::BgImagePath))
                activeField = Field::BgImagePath;
            if (activeField == Field::BgImagePath && IsKeyPressed(KEY_TAB))
            {
                commitActiveField();
                activeField = Field::None;
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
            propPanelContentH = py - TOOLBAR_H + propPanelScroll;
        }
        else
        {
            propPanelContentH = 80.0f;
        }

        endPropPanelScroll();
        return;
    }

    DrawText("PROPERTIES", static_cast<int>(x + 10), static_cast<int>(TOOLBAR_H + 10), 12, LIGHTGRAY);

    float py = beginPropPanelScroll(x, TOOLBAR_H, PROP_PANEL_W, canvasH) + 30;
    float lx = x + 8;
    float vx = x + 55;
    float iw = PROP_PANEL_W - 65;

    const char* typeName = "Unknown";
    if (w->getType() == WidgetType::Button) typeName = "Button";
    else if (w->getType() == WidgetType::Label) typeName = "Label";
    else if (w->getType() == WidgetType::Image) typeName = "Image";
    else if (w->getType() == WidgetType::ImageViewer) typeName = "ImageViewer";
    else if (w->getType() == WidgetType::RichTextBox) typeName = "RichTextBox";
    DrawText("Type:", static_cast<int>(lx), static_cast<int>(py), 11, LIGHTGRAY);
    DrawText(typeName, static_cast<int>(vx), static_cast<int>(py), 11, WHITE);
    py += 18;

    if (w->getType() == WidgetType::Image)
    {
        DrawText("Image:", static_cast<int>(lx), static_cast<int>(py), 11, LIGHTGRAY);
        Rectangle imgRec = {vx, py - 2, iw, 18};
        if (textBox(imgRec, imgPathBuf, sizeof(imgPathBuf), imgPathBufLen, activeField == Field::WidgetText))
            activeField = Field::WidgetText;
        if (activeField == Field::WidgetText && IsKeyPressed(KEY_TAB))
        {
            commitActiveField();
            activeField = Field::None;
            applyPropsToSelected();
        }
        py += 22;

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

        if (button({lx + (iw + 55) / 2 + 2, py, (iw + 55) / 2 - 2, 18}, "Load Image", {60, 100, 60, 255}))
        {
            applyPropsToSelected();
        }
        py += 24;

        ImageWidget* imgWidget = static_cast<ImageWidget*>(w);
        if (imgWidget->textureLoaded)
        {
            DrawText("Preview:", static_cast<int>(lx), static_cast<int>(py), 10, GRAY);
            py += 12;
            float previewW = iw + 55;
            float previewH = 60.0f;
            DrawRectangleRec({lx, py, previewW, previewH}, {25, 25, 35, 255});
            DrawRectangleLinesEx({lx, py, previewW, previewH}, 1, DARKGRAY);

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
    else if (w->getType() == WidgetType::ImageViewer)
    {
        drawImageViewerProperties(py, lx, iw);
    }
    else if (w->getType() == WidgetType::RichTextBox)
    {
        drawRichTextBoxProperties(py, lx, iw);
    }
    else
    {
        DrawText("Text:", static_cast<int>(lx), static_cast<int>(py), 11, LIGHTGRAY);
        Rectangle textRec = {vx, py - 2, iw, 18};
        if (textBox(textRec, textBuf, sizeof(textBuf), textBufLen, activeField == Field::WidgetText))
            activeField = Field::WidgetText;
        if (activeField == Field::WidgetText && IsKeyPressed(KEY_TAB))
        {
            commitActiveField();
            activeField = Field::None;
            applyPropsToSelected();
        }
        py += 22;

        {
            Rectangle sr = {lx, py, iw + 55, 16};
            if (slider(sr, "Size:", &editFontSize, 8, 72))
                applyPropsToSelected();
        }
        py += 20;

        {
            FontManager& fonts = uiManager.getFontManager();
            DrawText("Font:", static_cast<int>(lx), static_cast<int>(py), 11, LIGHTGRAY);

            float selectorX = vx;
            float selectorW = iw - 30;

            if (button({selectorX, py - 2, 16, 16}, "<", {60, 60, 80, 255}))
            {
                editFontIndex--;
                if (editFontIndex < 0) editFontIndex = fonts.getCount() - 1;
                applyPropsToSelected();
            }

            const char* fontName = fonts.getFontName(editFontIndex).c_str();
            DrawRectangle(static_cast<int>(selectorX + 18), static_cast<int>(py - 2),
                          static_cast<int>(selectorW - 36), 16, {40, 40, 50, 255});
            DrawText(fontName, static_cast<int>(selectorX + 22), static_cast<int>(py + 1), 10, WHITE);

            if (button({selectorX + selectorW - 14, py - 2, 16, 16}, ">", {60, 60, 80, 255}))
            {
                editFontIndex++;
                if (editFontIndex >= fonts.getCount()) editFontIndex = 0;
                applyPropsToSelected();
            }
        }
        py += 20;
    }

    {
        float fieldW = (iw + 55) / 2 - 20;
        DrawText("X:", static_cast<int>(lx), static_cast<int>(py + 2), 10, LIGHTGRAY);
        Rectangle xRec = {lx + 16, py - 2, fieldW, 16};
        if (textBox(xRec, xBuf, sizeof(xBuf), xBufLen, activeField == Field::PosX))
            activeField = Field::PosX;

        float yLabelX = lx + 16 + fieldW + 8;
        DrawText("Y:", static_cast<int>(yLabelX), static_cast<int>(py + 2), 10, LIGHTGRAY);
        Rectangle yRec = {yLabelX + 16, py - 2, fieldW, 16};
        if (textBox(yRec, yBuf, sizeof(yBuf), yBufLen, activeField == Field::PosY))
            activeField = Field::PosY;

        if (activeField == Field::PosX && IsKeyPressed(KEY_TAB))
        {
            commitActiveField();
            activeField = Field::PosY;
        }
        if (activeField == Field::PosY && IsKeyPressed(KEY_TAB))
        {
            commitActiveField();
            activeField = Field::None;
            applyPropsToSelected();
        }
    }
    py += 22;

    if (w->getType() == WidgetType::Button || w->getType() == WidgetType::Image || w->getType() == WidgetType::ImageViewer || w->getType() == WidgetType::RichTextBox)
    {
        float fieldW = (iw + 55) / 2 - 20;
        DrawText("W:", static_cast<int>(lx), static_cast<int>(py + 2), 10, LIGHTGRAY);
        Rectangle wRec = {lx + 16, py - 2, fieldW, 16};
        if (textBox(wRec, wBuf, sizeof(wBuf), wBufLen, activeField == Field::SizeW))
            activeField = Field::SizeW;

        float hLabelX = lx + 16 + fieldW + 8;
        DrawText("H:", static_cast<int>(hLabelX), static_cast<int>(py + 2), 10, LIGHTGRAY);
        Rectangle hRec = {hLabelX + 16, py - 2, fieldW, 16};
        if (textBox(hRec, hBuf, sizeof(hBuf), hBufLen, activeField == Field::SizeH))
            activeField = Field::SizeH;

        if (activeField == Field::SizeW && IsKeyPressed(KEY_TAB))
        {
            commitActiveField();
            activeField = Field::SizeH;
        }
        if (activeField == Field::SizeH && IsKeyPressed(KEY_TAB))
        {
            commitActiveField();
            activeField = Field::None;
            applyPropsToSelected();
        }
        py += 22;
    }

    py += 4;
    DrawText("-- Colors --", static_cast<int>(lx), static_cast<int>(py), 10, GRAY);
    py += 14;

    if (w->getType() == WidgetType::Image || w->getType() == WidgetType::ImageViewer)
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

        if (w->getType() == WidgetType::Image)
        {
            py += 4;
            auto drawImgAnimSection = [&]()
            {
                DrawText("--- Animation ---", static_cast<int>(lx), static_cast<int>(py), 10, GRAY);
                py += 14;

                float btnW = (iw + 55) / 3 - 2;
                if (button({lx, py, btnW, 16}, "Browse", {60, 80, 120, 255}))
                {
                    const char* filterPatterns[] = { "*.png", "*.jpg", "*.jpeg", "*.bmp", "*.gif", "*.tga" };
                    const char* result = tinyfd_openFileDialog("Select Frames", "", 6, filterPatterns, "Image Files", 1);
                    if (result)
                    {
                        std::string resultStr(result);
                        size_t pos = 0;
                        while ((pos = resultStr.find('|')) != std::string::npos)
                        {
                            imgAnimFramePaths.push_back(resultStr.substr(0, pos));
                            resultStr.erase(0, pos + 1);
                        }
                        if (!resultStr.empty())
                            imgAnimFramePaths.push_back(resultStr);
                        applyPropsToSelected();
                    }
                }
                if (button({lx + btnW + 2, py, btnW, 16}, "Clear", {100, 60, 60, 255}))
                {
                    imgAnimFramePaths.clear();
                    applyPropsToSelected();
                }
                py += 18;

                if (imgAnimFramePaths.empty())
                {
                    DrawText("(no frames)", static_cast<int>(lx), static_cast<int>(py), 8, LIGHTGRAY);
                    py += 14;
                }
                else
                {
                    for (size_t i = 0; i < imgAnimFramePaths.size(); i++)
                    {
                        Rectangle r = {lx, py, iw + 55, 16};
                        Vector2 m = GetVirtualMousePos();
                        bool hover = CheckCollisionPointRec(m, r);
                        Color bg = hover ? (Color){55, 55, 70, 255} : (Color){40, 40, 50, 255};
                        DrawRectangleRec(r, bg);

                        const char* fname = imgAnimFramePaths[i].c_str();
                        const char* slash = strrchr(fname, '/');
                        if (slash) fname = slash + 1;
                        DrawText(fname, static_cast<int>(lx + 4), static_cast<int>(py + 3), 8, LIGHTGRAY);

                        float xBtnW = 16;
                        Rectangle xr = {lx + iw + 55 - xBtnW, py, xBtnW, 16};
                        bool xHover = CheckCollisionPointRec(m, xr);
                        Color xBg = xHover ? (Color){160, 50, 50, 255} : (Color){100, 40, 40, 255};
                        DrawRectangleRec(xr, xBg);
                        DrawText("X", static_cast<int>(xr.x + 4), static_cast<int>(py + 3), 8, WHITE);

                        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && xHover)
                        {
                            imgAnimFramePaths.erase(imgAnimFramePaths.begin() + i);
                            applyPropsToSelected();
                            break;
                        }
                        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && hover && !xHover)
                        {
                            const char* filterPatterns[] = { "*.png", "*.jpg", "*.jpeg", "*.bmp", "*.gif", "*.tga" };
                            const char* result = tinyfd_openFileDialog("Replace Frame", "", 6, filterPatterns, "Image Files", 0);
                            if (result)
                            {
                                imgAnimFramePaths[i] = result;
                                applyPropsToSelected();
                            }
                        }
                        py += 18;
                    }
                }

                int durInt = static_cast<int>(imgAnimFrameDuration * 100);
                if (slider({lx, py, iw + 55, 16}, "Duration:", &durInt, 5, 200))
                {
                    imgAnimFrameDuration = durInt / 100.0f;
                    applyPropsToSelected();
                }
                py += 20;

                const char* loopLabel = imgAnimLoop ? "[x] Loop" : "[ ] Loop";
                if (button({lx, py, btnW + 10, 16}, loopLabel, imgAnimLoop ? (Color){60, 100, 60, 255} : (Color){50, 50, 60, 255}))
                {
                    imgAnimLoop = !imgAnimLoop;
                    applyPropsToSelected();
                }
                py += 22;
            };
            drawImgAnimSection();
        }
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

        if (actionTypeIndex == 0 || actionTypeIndex == 4 || actionTypeIndex == 5)
        {
            const char* actLabel = (actionTypeIndex == 0) ? "Target:" :
                                   (actionTypeIndex == 4) ? "Command:" : "Value:";
            DrawText(actLabel, static_cast<int>(lx), static_cast<int>(py), 11, LIGHTGRAY);
            Rectangle actRec = {vx, py - 2, iw, 18};
            if (textBox(actRec, actionBuf, sizeof(actionBuf), actionBufLen, activeField == Field::ActionValue))
                activeField = Field::ActionValue;
            if (activeField == Field::ActionValue && IsKeyPressed(KEY_TAB))
            {
                commitActiveField();
                activeField = Field::None;
                applyPropsToSelected();
            }
            py += 24;
        }

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

    if (w->getType() == WidgetType::Button)
    {
        auto drawAnimSection = [&](const char* label,
            std::vector<std::string>& paths,
            float& duration, bool& loop)
        {
            DrawText(label, static_cast<int>(lx), static_cast<int>(py), 10, GRAY);
            py += 14;

            float btnW = (iw + 55) / 3 - 2;
            if (button({lx, py, btnW, 16}, "Browse", {60, 80, 120, 255}))
            {
                const char* filterPatterns[] = { "*.png", "*.jpg", "*.jpeg", "*.bmp", "*.gif", "*.tga" };
                const char* result = tinyfd_openFileDialog("Select Frames", "", 6, filterPatterns, "Image Files", 1);
                if (result)
                {
                    std::string resultStr(result);
                    size_t pos = 0;
                    while ((pos = resultStr.find('|')) != std::string::npos)
                    {
                        paths.push_back(resultStr.substr(0, pos));
                        resultStr.erase(0, pos + 1);
                    }
                    if (!resultStr.empty())
                        paths.push_back(resultStr);
                    applyPropsToSelected();
                }
            }
            if (button({lx + btnW + 2, py, btnW, 16}, "Clear", {100, 60, 60, 255}))
            {
                paths.clear();
                applyPropsToSelected();
            }
            py += 18;

            if (paths.empty())
            {
                DrawText("(no frames)", static_cast<int>(lx), static_cast<int>(py), 8, LIGHTGRAY);
                py += 14;
            }
            else
            {
                for (size_t i = 0; i < paths.size(); i++)
                {
                    Rectangle r = {lx, py, iw + 55, 16};
                    Vector2 m = GetVirtualMousePos();
                    bool hover = CheckCollisionPointRec(m, r);
                    Color bg = hover ? (Color){55, 55, 70, 255} : (Color){40, 40, 50, 255};
                    DrawRectangleRec(r, bg);

                    const char* fname = paths[i].c_str();
                    const char* slash = strrchr(fname, '/');
                    if (slash) fname = slash + 1;
                    DrawText(fname, static_cast<int>(lx + 4), static_cast<int>(py + 3), 8, LIGHTGRAY);

                    float xBtnW = 16;
                    Rectangle xr = {lx + iw + 55 - xBtnW, py, xBtnW, 16};
                    bool xHover = CheckCollisionPointRec(m, xr);
                    Color xBg = xHover ? (Color){160, 50, 50, 255} : (Color){100, 40, 40, 255};
                    DrawRectangleRec(xr, xBg);
                    DrawText("X", static_cast<int>(xr.x + 4), static_cast<int>(py + 3), 8, WHITE);

                    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && xHover)
                    {
                        paths.erase(paths.begin() + i);
                        applyPropsToSelected();
                        break;
                    }
                    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && hover && !xHover)
                    {
                        const char* filterPatterns[] = { "*.png", "*.jpg", "*.jpeg", "*.bmp", "*.gif", "*.tga" };
                        const char* result = tinyfd_openFileDialog("Replace Frame", "", 6, filterPatterns, "Image Files", 0);
                        if (result)
                        {
                            paths[i] = result;
                            applyPropsToSelected();
                        }
                    }
                    py += 18;
                }
            }

            int durInt = static_cast<int>(duration * 100);
            if (slider({lx, py, iw + 55, 16}, "Duration:", &durInt, 5, 200))
            {
                duration = durInt / 100.0f;
                applyPropsToSelected();
            }
            py += 20;

            const char* loopLabel = loop ? "[x] Loop" : "[ ] Loop";
            if (button({lx, py, btnW + 10, 16}, loopLabel, loop ? (Color){60, 100, 60, 255} : (Color){50, 50, 60, 255}))
            {
                loop = !loop;
                applyPropsToSelected();
            }
            py += 22;
        };

        drawAnimSection("--- Idle Animation ---",
            idleAnimFramePaths,
            idleAnimFrameDuration, idleAnimLoop);

        drawAnimSection("--- Hover Animation ---",
            hoverAnimFramePaths,
            hoverAnimFrameDuration, hoverAnimLoop);

        drawAnimSection("--- Click Animation ---",
            clickAnimFramePaths,
            clickAnimFrameDuration, clickAnimLoop);
    }

    py += 10;
    if (button({lx, py, PROP_PANEL_W - 16, 28}, "DELETE WIDGET", {140, 50, 50, 255}))
    {
        deleteSelectedWidget();
    }
    py += 28;

    propPanelContentH = py - TOOLBAR_H + propPanelScroll;
    endPropPanelScroll();
}

void UIEditor::drawImageViewerProperties(float& positionY, float labelX, float inputWidth)
{
    float controlW = inputWidth + 55;

    DrawText("Images:", static_cast<int>(labelX), static_cast<int>(positionY), 11, LIGHTGRAY);
    positionY += 14;

    Widget* w = getSelectedWidget();
    if (!w || w->getType() != WidgetType::ImageViewer) return;
    ImageViewer* iv = static_cast<ImageViewer*>(w);

    if (button({labelX, positionY, controlW / 2 - 2, 18}, "Add Images", {60, 80, 120, 255}))
    {
        const char* filterPatterns[] = { "*.png", "*.jpg", "*.jpeg", "*.bmp", "*.gif", "*.tga" };
        const char* result = tinyfd_openFileDialog("Select Images", "", 6, filterPatterns, "Image Files", 1);
        if (result)
        {
            std::string paths(result);
            size_t pos = 0;
            while ((pos = paths.find('|')) != std::string::npos)
            {
                iv->addImage(paths.substr(0, pos));
                paths.erase(0, pos + 1);
            }
            if (!paths.empty())
                iv->addImage(paths);
            applyPropsToSelected();
        }
    }

    if (button({labelX + controlW / 2 + 2, positionY, controlW / 2 - 2, 18}, "Clear All", {140, 60, 60, 255}))
    {
        iv->clearImages();
        applyPropsToSelected();
    }
    positionY += 24;

    DrawText(TextFormat("Count: %d", (int)iv->images.size()),
        static_cast<int>(labelX), static_cast<int>(positionY), 10, LIGHTGRAY);
    positionY += 14;

    for (int i = 0; i < (int)iv->images.size(); i++)
    {
        bool isSelected = (i == iv->currentIndex);
        Color bg = isSelected ? (Color){60, 60, 120, 255} : (Color){40, 40, 50, 255};
        Rectangle r = {labelX, positionY, controlW - 78, 16};

        DrawRectangleRec(r, bg);

        std::string fileName = iv->images[i].path;
        size_t lastSlash = fileName.find_last_of("/\\");
        if (lastSlash != std::string::npos)
            fileName = fileName.substr(lastSlash + 1);
        if (fileName.length() > 24)
            fileName = fileName.substr(0, 21) + "...";

        DrawText(fileName.c_str(), static_cast<int>(labelX + 4), static_cast<int>(positionY + 3), 10, WHITE);

        Vector2 m = GetVirtualMousePos();
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(m, r))
        {
            iv->setCurrentIndex(i);
            applyPropsToSelected();
        }

        if (button({labelX + controlW - 76, positionY, 18, 16}, "~", {60, 80, 100, 255}))
        {
            const char* filterPatterns[] = { "*.png", "*.jpg", "*.jpeg", "*.bmp", "*.gif", "*.tga" };
            const char* result = tinyfd_openFileDialog("Replace Image", "", 6, filterPatterns, "Image Files", 0);
            if (result)
            {
                iv->replaceImage(i, result);
                applyPropsToSelected();
            }
        }

        if (button({labelX + controlW - 56, positionY, 18, 16}, "X", {120, 50, 50, 255}))
        {
            iv->removeImage(i);
            applyPropsToSelected();
            break;
        }

        positionY += 18;
    }

    if (!iv->images.empty())
    {
        positionY += 4;
        DrawText(TextFormat("Current: %d", iv->currentIndex + 1),
            static_cast<int>(labelX), static_cast<int>(positionY), 10, LIGHTGRAY);
        positionY += 16;
    }

    if (iv->images.empty())
    {
        DrawText("Add images above", static_cast<int>(labelX), static_cast<int>(positionY), 10, GRAY);
        positionY += 14;
    }

    if (iv->currentIndex >= 0 && iv->currentIndex < (int)iv->images.size() && iv->images[iv->currentIndex].loaded)
    {
        DrawText("Preview:", static_cast<int>(labelX), static_cast<int>(positionY), 10, GRAY);
        positionY += 12;
        float previewW = controlW;
        float previewH = 60.0f;
        DrawRectangleRec({labelX, positionY, previewW, previewH}, {25, 25, 35, 255});
        DrawRectangleLinesEx({labelX, positionY, previewW, previewH}, 1, DARKGRAY);

        Texture2D& tex = iv->images[iv->currentIndex].texture;
        float texW = (float)tex.width;
        float texH = (float)tex.height;
        float scale = (previewW / texW < previewH / texH) ? previewW / texW : previewH / texH;
        float drawW = texW * scale;
        float drawH = texH * scale;
        float drawX = labelX + (previewW - drawW) / 2;
        float drawY = positionY + (previewH - drawH) / 2;
        DrawTexturePro(tex,
            {0, 0, texW, texH},
            {drawX, drawY, drawW, drawH},
            {0, 0}, 0.0f, iv->tint);
        positionY += previewH + 8;
    }
}

void UIEditor::drawFontPanel()
{
    FontManager& fonts = uiManager.getFontManager();

    float panelW = 320.0f;
    float panelH = 340.0f;
    float px = (static_cast<float>(GetVirtualScreenWidth()) - panelW) / 2;
    float py = (static_cast<float>(GetVirtualScreenHeight()) - panelH) / 2;

    DrawRectangleRec({px, py, panelW, panelH}, {30, 30, 40, 245});
    DrawRectangleLinesEx({px, py, panelW, panelH}, 2, GRAY);

    DrawText("FONT MANAGER", static_cast<int>(px + 10), static_cast<int>(py + 10), 14, LIGHTGRAY);

    if (button({px + panelW - 28, py + 6, 22, 20}, "X", {120, 50, 50, 255}))
    {
        showFontPanel = false;
    }

    float fy = py + 34;
    float flx = px + 10;
    float fvx = px + 60;
    float fiw = panelW - 70;

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

    DrawText("Load Font:", static_cast<int>(flx), static_cast<int>(fy), 11, LIGHTGRAY);
    fy += 14;

    Rectangle pathRec = {flx, fy - 2, fiw - 50, 18};
    if (textBox(pathRec, fontPathBuf, sizeof(fontPathBuf), fontPathBufLen, fontPathActive))
    {
        fontPathActive = true;
        fontPreviewActive = false;
        activeField = Field::FontPath;
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

    if (selectedFontIndex > 0)
    {
        if (button({flx, fy, fiw, 18}, "Remove Selected Font", {120, 50, 50, 255}))
        {
            fonts.removeFont(selectedFontIndex);
            selectedFontIndex = -1;
        }
        fy += 24;
    }

    if (selectedFontIndex > 0 && selectedFontIndex != fonts.getDefaultIndex())
    {
        if (button({flx, fy, fiw, 18}, "Set as Default", {60, 80, 120, 255}))
        {
            fonts.setDefaultFont(selectedFontIndex);
        }
        fy += 24;
    }

    fy += 6;

    DrawText("-- Preview --", static_cast<int>(flx), static_cast<int>(fy), 10, GRAY);
    fy += 14;

    DrawText("Text:", static_cast<int>(flx), static_cast<int>(fy), 10, LIGHTGRAY);
    Rectangle prevRec = {flx + 35, fy - 2, fiw - 35, 18};
    if (textBox(prevRec, fontPreviewBuf, sizeof(fontPreviewBuf), fontPreviewBufLen, fontPreviewActive))
    {
        fontPreviewActive = true;
        fontPathActive = false;
        activeField = Field::FontPreview;
    }
    fy += 22;

    {
        Rectangle sr = {flx, fy, fiw, 16};
        if (slider(sr, "Size:", &fontPreviewSize, 8, 72)) {}
    }
    fy += 22;

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
