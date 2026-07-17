#pragma once

#include "ui_manager.hpp"
#include "font_manager.hpp"
#include "bg_pattern.hpp"
#include <string>
#include <vector>

class UIEditor
{
public:
    UIEditor(UIManager& uiManager);

    void update(float dt);
    void draw();
    void toggle();
    bool isEditorMode() const;

private:
    UIManager& uiManager;
    bool editorMode = false;

    // Selection
    int selectedWidgetIndex = -1;
    std::string editingScreenName;

    // Drag
    bool dragging = false;
    Vector2 dragOffset = {0, 0};

    // Canvas pan & zoom
    bool canvasPanning = false;
    Vector2 canvasPan = {0, 0};
    Vector2 canvasPanLast = {0, 0};
    float canvasZoom = 1.0f;

    // Resize
    bool resizing = false;

    // Property editing buffers
    char textBuf[256] = "";
    char actionBuf[256] = "";
    char screenNameBuf[128] = "";
    int textBufLen = 0;
    int actionBufLen = 0;
    int screenNameBufLen = 0;
    enum class Field
    {
        None = -1,
        WidgetText = 0,
        ActionValue = 1,
        FontPath = 2,
        FontPreview = 3,
        BgImagePath = 4,
        ScreenName = 5,
        PosX = 10,
        PosY = 11,
        SizeW = 12,
        SizeH = 13,
        ColorR = 20,
        ColorG = 21,
        ColorB = 22,
        ColorA = 23,
        RichTextContent = 30,
    };
    Field activeField = Field::None;
    int textCursorPos = 0; // cursor position within active text field

    int editFontSize = 20;
    int editFontIndex = 0;
    float editX = 0, editY = 0;
    char xBuf[16] = "";
    int xBufLen = 0;
    char yBuf[16] = "";
    int yBufLen = 0;
    float editWidth = 200, editHeight = 50;
    char wBuf[16] = "";
    int wBufLen = 0;
    char hBuf[16] = "";
    int hBufLen = 0;
    int editColorR = 0, editColorG = 0, editColorB = 0, editColorA = 255;
    int editHoverR = 0, editHoverG = 0, editHoverB = 0, editHoverA = 255;
    int editPressR = 0, editPressG = 0, editPressB = 0, editPressA = 255;
    int editTxtR = 255, editTxtG = 255, editTxtB = 255, editTxtA = 255;
    int actionTypeIndex = 0;

    // Button animation editing
    std::vector<std::string> idleAnimFramePaths;
    float idleAnimFrameDuration = 0.1f;
    bool idleAnimLoop = true;
    std::vector<std::string> hoverAnimFramePaths;
    float hoverAnimFrameDuration = 0.1f;
    bool hoverAnimLoop = true;
    std::vector<std::string> clickAnimFramePaths;
    float clickAnimFrameDuration = 0.1f;
    bool clickAnimLoop = true;

    // Image widget fields
    char imgPathBuf[512] = "";
    int imgPathBufLen = 0;
    int editTintR = 255, editTintG = 255, editTintB = 255, editTintA = 255;
    int editFitIndex = 0;
    std::vector<std::string> imgAnimFramePaths;
    float imgAnimFrameDuration = 0.1f;
    bool imgAnimLoop = true;

    // Screen background image fields
    char bgImgPathBuf[512] = "";
    int bgImgPathBufLen = 0;
    int editBgFitIndex = 0;

    // Screen background color fields
    int editBgColorR = 245, editBgColorG = 245, editBgColorB = 245, editBgColorA = 255;

    // Screen background pattern fields
    int editBgPatternIndex = 0;
    int editPatColorAR = 255, editPatColorAG = 165, editPatColorAB = 0, editPatColorAA = 255;
    int editPatColorBR = 180, editPatColorBG = 100, editPatColorBB = 0, editPatColorBA = 255;
    int editPatTileSize = 64;

    // Screen background scroll fields
    int editScrollDirIndex = 0;
    int editScrollSpeed = 60;

    // RichTextBox editing
    char mdContentBuf[8192] = "";
    int mdContentBufLen = 0;
    int rtbFontSize = 16;
    float rtbLineSpacing = 1.4f;
    int rtbPadding = 8;
    int rtbTextAlign = 0;
    int editRtbBgR = 30, editRtbBgG = 30, editRtbBgB = 40, editRtbBgA = 255;

    // ImageViewer editing

    // Color picker state
    int expandedColorId = -1;
    float pickerHue = 0.0f;
    float pickerSat = 0.0f;
    float pickerVal = 0.0f;
    char colorRBuf[8] = "";
    char colorGBuf[8] = "";
    char colorBBuf[8] = "";
    char colorABuf[8] = "";
    int colorRBufLen = 0;
    int colorGBufLen = 0;
    int colorBBufLen = 0;
    int colorABufLen = 0;
    int* activeColorR = nullptr;
    int* activeColorG = nullptr;
    int* activeColorB = nullptr;
    int* activeColorA = nullptr;

    // Font settings panel
    bool showFontPanel = false;
    int selectedFontIndex = -1;
    char fontPathBuf[512] = "";
    int fontPathBufLen = 0;
    bool fontPathActive = false;
    char fontPreviewBuf[256] = "Sample Text 123";
    int fontPreviewBufLen = 15;
    bool fontPreviewActive = false;
    int fontPreviewSize = 32;

    // Layout constants
    static constexpr float TOOLBAR_H = 40.0f;
    static constexpr float SIDEBAR_W = 180.0f;
    static constexpr float PROP_PANEL_W = 230.0f;
    static constexpr float GRID_SIZE = 10.0f;

    // TextBox constants
    static constexpr int TB_LINE_H = 14;
    static constexpr int TB_PAD = 4;
    static constexpr int TB_FONT = 12;
    static constexpr int MD_BOX_H = 80;

    // Scroll offsets for panels
    float screenListScroll = 0.0f;
    float screenListContentH = 0.0f;
    float propPanelScroll = 0.0f;
    float propPanelContentH = 0.0f;
    float fontListScroll = 0.0f;

    // Methods
    void drawToolbar();
    void drawScreenList();
    void drawPropertyPanel();
    void drawCanvas();
    void drawFontPanel();

    void selectWidget(int index);
    void deselectWidget();
    void syncPropsFromSelected();
    void applyPropsToSelected();
    void commitActiveField();

    Screen* getCurrentScreen();
    Widget* getSelectedWidget();

    void addWidgetToScreen(const std::string& type);
    void deleteSelectedWidget();
    void duplicateSelectedWidget();

    // Custom UI helpers
    bool button(Rectangle rec, const char* label, Color bg);
    bool slider(Rectangle rec, const char* label, int* value, int minVal, int maxVal);
    bool sliderF(Rectangle rec, const char* label, float* value, float minVal, float maxVal);
    bool textBox(Rectangle rec, char* buf, int bufSize, int& len, bool active, bool multiline = false);
    void colorPicker(const char* label, int& r, int& g, int& b, int& a, int id, float& py);

    float beginPropPanelScroll(float x, float y, float w, float h);
    void endPropPanelScroll();

    int textBoxClickPos(Rectangle rec, const char* buf, int len, Vector2 mouse, bool multiline);
    int textBoxCursorLine(const char* buf, int pos);
    int textBoxCursorX(const char* buf, int lineStart, int pos);
    void textBoxDrawCursor(Rectangle rec, const char* buf, int len, int pos, bool multiline);
    void textBoxDrawText(Rectangle rec, const char* buf, int len, bool multiline);
    void drawRichTextBoxProperties(float& py, float lx, float iw);
    void drawImageViewerProperties(float& py, float lx, float iw);
};
