#include "raylib.h"
#include "ui_manager.hpp"
#include "ui_editor.hpp"
#include "pixel_scale.hpp"
#include <string>

RenderContext g_render;

Vector2 GetVirtualMousePos()
{
    Vector2 m = GetMousePosition();
    return { m.x / g_render.pixel_scale, m.y / g_render.pixel_scale };
}

int GetVirtualScreenWidth()
{
    return g_render.virtual_w;
}

int GetVirtualScreenHeight()
{
    return g_render.virtual_h;
}

int main(int argc, char** argv)
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1920, 1080, "UI System");
    ToggleFullscreen();
    SetTargetFPS(60);

    g_render.virtual_w = 960;
    g_render.virtual_h = 540;

    RenderTexture2D target = LoadRenderTexture(g_render.virtual_w, g_render.virtual_h);
    SetTextureFilter(target.texture, TEXTURE_FILTER_POINT);

    UIManager ui;
    if (!ui.loadFromFile("ui.json"))
    {
        TraceLog(LOG_WARNING, "Failed to load ui.json, using fallback");
    }

    UIEditor editor(ui);

    ui.onAction = [](const std::string& action) {
        if (action == "quit")
        {
            CloseWindow();
        }
        else if (action == "toggle_fullscreen")
        {
            ToggleFullscreen();
        }
        else if (action == "pause")
        {
            TraceLog(LOG_INFO, "Pause pressed");
        }
        else if (action.rfind("run:", 0) == 0)
        {
            std::string command = action.substr(4);
            TraceLog(LOG_INFO, "Executing command: %s", command.c_str());
#ifdef _WIN32
            ShellExecuteA(NULL, "open", command.c_str(), NULL, NULL, SW_SHOW);
#elif defined(__APPLE__)
            std::string cmd = "open " + command;
            system(cmd.c_str());
#else
            std::string cmd = "xdg-open " + command + " &";
            system(cmd.c_str());
#endif
        }
    };

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();

        if (IsKeyPressed(KEY_F11))
        {
            ToggleFullscreen();
        }

        if (editor.isEditorMode())
        {
            editor.update(dt);
        }
        else
        {
            if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_E))
            {
                editor.toggle();
            }
            else
            {
                ui.update(dt);
            }
        }

        BeginTextureMode(target);
            ClearBackground(BLACK);
            if (editor.isEditorMode())
            {
                editor.draw();
            }
            else
            {
                ui.draw();

                DrawText("Press E for editor", 10, g_render.virtual_h - 20, 10, DARKGRAY);
            }
        EndTextureMode();

        BeginDrawing();
            ClearBackground(BLACK);
            {
                float screenW = static_cast<float>(GetScreenWidth());
                float screenH = static_cast<float>(GetScreenHeight());
                float scale = screenH / static_cast<float>(g_render.virtual_h);
                float renderW = static_cast<float>(g_render.virtual_w) * scale;
                float offsetX = (screenW - renderW) / 2.0f;
                DrawTexturePro(target.texture,
                    { 0, 0, static_cast<float>(g_render.virtual_w), static_cast<float>(-g_render.virtual_h) },
                    { offsetX, 0, renderW, screenH },
                    { 0, 0 }, 0.0f, WHITE);
            }
        EndDrawing();
    }

    UnloadRenderTexture(target);
    CloseWindow();
    return 0;
}
