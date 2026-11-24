// systems/pause_menu.cpp
// Menu jeda yang dibuka dengan ESC: Resume, Save, Controls, Main Menu, Quit.
#include "pause_menu.h"
#include "core/config.h"
#include "raymath.h"
#include <math.h>

static const char* BUTTON_LABELS[] = { "Resume", "Save", "Controls", "Main Menu", "Quit" };
static const PauseAction BUTTON_ACTIONS[] = { PAUSE_RESUME, PAUSE_SAVE, PAUSE_CONTROLS, PAUSE_MAIN_MENU, PAUSE_QUIT };
static const int BUTTON_COUNT = 5;

static const float PANEL_W = 380.0f;
static const float PANEL_H = 470.0f;
static const float BTN_W = 260.0f;
static const float BTN_H = 54.0f;
static const float BTN_GAP = 14.0f;

static Rectangle PanelRect(void)
{
    return Rectangle{ (SCREEN_W - PANEL_W) * 0.5f, (SCREEN_H - PANEL_H) * 0.5f, PANEL_W, PANEL_H };
}

static Rectangle ButtonRect(int i)
{
    Rectangle p = PanelRect();
    return Rectangle{ p.x + (PANEL_W - BTN_W) * 0.5f, p.y + 96.0f + i * (BTN_H + BTN_GAP), BTN_W, BTN_H };
}

void PauseMenuOpen(PauseMenu* m)
{
    m->selected = 0;
}

PauseAction PauseMenuUpdate(PauseMenu* m)
{
    // keyboard
    if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP))
        m->selected = (m->selected + BUTTON_COUNT - 1) % BUTTON_COUNT;
    if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN))
        m->selected = (m->selected + 1) % BUTTON_COUNT;
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_SPACE))
        return BUTTON_ACTIONS[m->selected];

    // mouse (hover hanya mengubah pilihan saat mouse bergerak, agar tidak bentrok dengan keyboard)
    Vector2 mouse = GetMousePosition();
    Vector2 delta = GetMouseDelta();
    bool moved = (fabsf(delta.x) + fabsf(delta.y)) > 0.0f;
    for (int i = 0; i < BUTTON_COUNT; i++) {
        if (!CheckCollisionPointRec(mouse, ButtonRect(i))) continue;
        if (moved) m->selected = i;
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            m->selected = i;
            return BUTTON_ACTIONS[i];
        }
    }
    return PAUSE_NONE;
}

void PauseMenuDraw(const PauseMenu* m)
{
    // gelapkan layar game di belakang menu
    DrawRectangle(0, 0, SCREEN_W, SCREEN_H, Color{ 0, 0, 0, 160 });

    Rectangle p = PanelRect();
    DrawRectangleRounded(p, 0.06f, 8, Color{ 22, 22, 34, 245 });
    DrawRectangleRoundedLinesEx(p, 0.06f, 8, 2.0f, Color{ 200, 170, 90, 255 });

    const char* title = "PAUSED";
    int tfs = 40;
    DrawText(title, (int)(p.x + (PANEL_W - MeasureText(title, tfs)) * 0.5f), (int)p.y + 30, tfs, RAYWHITE);

    for (int i = 0; i < BUTTON_COUNT; i++) {
        Rectangle b = ButtonRect(i);
        bool sel = (i == m->selected);
        bool isQuit = (BUTTON_ACTIONS[i] == PAUSE_QUIT || BUTTON_ACTIONS[i] == PAUSE_MAIN_MENU);

        Color fill = sel ? (isQuit ? Color{ 150, 45, 45, 255 } : Color{ 70, 90, 150, 255 }) : Color{ 45, 45, 62, 255 };
        Color edge = sel ? Color{ 240, 210, 120, 255 } : Color{ 90, 90, 115, 255 };
        DrawRectangleRounded(b, 0.25f, 8, fill);
        DrawRectangleRoundedLinesEx(b, 0.25f, 8, 2.0f, edge);

        int fs = 26;
        const char* label = BUTTON_LABELS[i];
        DrawText(label, (int)(b.x + (b.width - MeasureText(label, fs)) * 0.5f), (int)(b.y + (b.height - fs) * 0.5f), fs, RAYWHITE);
    }

    const char* hint = "ESC: kembali ke game";
    int hfs = 14;
    DrawText(hint, (int)(p.x + (PANEL_W - MeasureText(hint, hfs)) * 0.5f), (int)(p.y + PANEL_H - 28), hfs, GRAY);
}
