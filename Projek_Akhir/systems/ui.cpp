// systems/ui.cpp
#include "ui.h"
#include <math.h>

void UiDrawPanel(Rectangle r)
{
    DrawRectangleRounded(r, 0.05f, 8, UI_PANEL);
    DrawRectangleRoundedLinesEx(r, 0.05f, 8, 2.0f, UI_GOLD_DIM);
}

void UiDrawButton(Rectangle b, const char* label, bool sel, UiButtonStyle style, int fs)
{
    Color fill = UI_BTN, edge = UI_EDGE, text = RAYWHITE;
    if (style == UI_BUTTON_DISABLED) {
        fill = Color{ 35, 35, 46, 255 };
        edge = Color{ 60, 60, 75, 255 };
        text = Color{ 100, 100, 115, 255 };
    }
    else if (sel) {
        fill = (style == UI_BUTTON_DANGER) ? UI_BTN_DANGER : UI_BTN_SEL;
        edge = UI_GOLD;
    }
    DrawRectangleRounded(b, 0.25f, 8, fill);
    DrawRectangleRoundedLinesEx(b, 0.25f, 8, 2.0f, edge);
    DrawText(label, (int)(b.x + (b.width - MeasureText(label, fs)) * 0.5f),
             (int)(b.y + (b.height - fs) * 0.5f), fs, text);
}

void UiDrawTextCentered(const char* text, float cx, float y, int fs, Color c)
{
    DrawText(text, (int)(cx - MeasureText(text, fs) * 0.5f), (int)y, fs, c);
}

static int KeyCapFont(float h) { return (int)(h * 0.5f); }

float UiKeyCapWidth(const char* label, float h)
{
    float w = (float)MeasureText(label, KeyCapFont(h)) + h * 0.6f;
    return (w < h) ? h : w;
}

float UiDrawKeyCap(const char* label, float x, float y, float h)
{
    int fs = KeyCapFont(h);
    float w = UiKeyCapWidth(label, h);
    Rectangle body = { x, y, w, h };
    Rectangle top = { x, y, w, h - 4 };
    DrawRectangleRounded(body, 0.3f, 6, Color{ 20, 20, 28, 255 });         // sisi bawah (efek 3D)
    DrawRectangleRounded(top, 0.3f, 6, Color{ 235, 232, 220, 255 });
    DrawRectangleRoundedLinesEx(top, 0.3f, 6, 1.5f, Color{ 120, 115, 100, 255 });
    DrawText(label, (int)(x + (w - MeasureText(label, fs)) * 0.5f), (int)(y + (h - 4 - fs) * 0.5f), fs, Color{ 30, 30, 40, 255 });
    return w;
}

bool UiMouseMoved(void)
{
    Vector2 d = GetMouseDelta();
    return (fabsf(d.x) + fabsf(d.y)) > 0.0f;
}

bool UiPressedUp(void)      { return IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP); }
bool UiPressedDown(void)    { return IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN); }
bool UiPressedLeft(void)    { return IsKeyPressed(KEY_A) || IsKeyPressed(KEY_LEFT); }
bool UiPressedRight(void)   { return IsKeyPressed(KEY_D) || IsKeyPressed(KEY_RIGHT); }
bool UiPressedConfirm(void) { return IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_SPACE); }
bool UiPressedBack(void)    { return IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE); }
