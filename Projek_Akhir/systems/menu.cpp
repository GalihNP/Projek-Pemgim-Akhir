// systems/menu.cpp
#include "menu.h"
#include "ui.h"
#include "display.h"
#include "core/config.h"
#include "data/weapons.h"
#include "world/background.h"
#include <stdio.h>
#include <math.h>

// ============================================================================
// Latar belakang menu
// ============================================================================
void MenuBackgroundDraw(const World* w, float t)
{
    ClearBackground(Color{ 24, 24, 32, 255 });

    const float worldW = (float)(ROOM_COLS * ROOM_W);
    const float worldH = (float)(ROOM_ROWS * ROOM_H);
    Camera2D cam = {};
    cam.offset = Vector2{ SCREEN_W * 0.5f, SCREEN_H * 0.5f };
    cam.target = Vector2{ worldW * 0.5f + cosf(t * 0.035f) * (worldW * 0.5f - SCREEN_W * 0.5f - 40.0f),
                          worldH * 0.5f + sinf(t * 0.060f) * (worldH * 0.5f - SCREEN_H * 0.5f - 40.0f) };
    cam.zoom = 1.0f;

    BeginMode2D(cam);
    BackgroundDrawGround(w);
    BackgroundDrawDecor(1e9f, false);
    EndMode2D();

    DrawRectangleGradientV(0, 0, SCREEN_W, SCREEN_H, Color{ 5, 5, 12, 150 }, Color{ 5, 5, 12, 225 });
}

// ============================================================================
// Main menu
// ============================================================================
static const char* MAIN_LABELS[] = { "Play", "Options", "Controls", "Exit" };
static const MainMenuAction MAIN_ACTIONS[] = { MAINMENU_PLAY, MAINMENU_OPTIONS, MAINMENU_CONTROLS, MAINMENU_EXIT };
static const int MAIN_COUNT = 4;
static const float MAIN_BTN_W = 300.0f, MAIN_BTN_H = 56.0f, MAIN_BTN_GAP = 16.0f, MAIN_BTN_Y = 330.0f;

static Rectangle MainButtonRect(int i)
{
    return Rectangle{ (SCREEN_W - MAIN_BTN_W) * 0.5f, MAIN_BTN_Y + i * (MAIN_BTN_H + MAIN_BTN_GAP), MAIN_BTN_W, MAIN_BTN_H };
}

void MainMenuOpen(MainMenu* m) { m->selected = 0; }

MainMenuAction MainMenuUpdate(MainMenu* m)
{
    if (UiPressedUp())   m->selected = (m->selected + MAIN_COUNT - 1) % MAIN_COUNT;
    if (UiPressedDown()) m->selected = (m->selected + 1) % MAIN_COUNT;
    if (UiPressedConfirm()) return MAIN_ACTIONS[m->selected];

    Vector2 mouse = GetMousePosition();
    bool moved = UiMouseMoved();
    for (int i = 0; i < MAIN_COUNT; i++) {
        if (!CheckCollisionPointRec(mouse, MainButtonRect(i))) continue;
        if (moved) m->selected = i;
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { m->selected = i; return MAIN_ACTIONS[i]; }
    }
    return MAINMENU_NONE;
}

void MainMenuDraw(const MainMenu* m)
{
    // Judul
    const char* title = "ARUNIKA";
    int tfs = 96;
    int tw = MeasureText(title, tfs);
    DrawText(title, SCREEN_W / 2 - tw / 2 + 4, 124, tfs, Color{ 0, 0, 0, 160 });   // bayangan
    DrawText(title, SCREEN_W / 2 - tw / 2, 120, tfs, UI_GOLD);
    UiDrawTextCentered("Adventure World", SCREEN_W * 0.5f, 226, 28, RAYWHITE);
    DrawRectangle(SCREEN_W / 2 - 160, 266, 320, 2, Fade(UI_GOLD_DIM, 0.7f));

    for (int i = 0; i < MAIN_COUNT; i++) {
        UiButtonStyle st = (MAIN_ACTIONS[i] == MAINMENU_EXIT) ? UI_BUTTON_DANGER : UI_BUTTON_NORMAL;
        UiDrawButton(MainButtonRect(i), MAIN_LABELS[i], i == m->selected, st, 28);
    }

    UiDrawTextCentered("W/S atau mouse untuk memilih  |  Enter / klik untuk konfirmasi",
                       SCREEN_W * 0.5f, SCREEN_H - 40.0f, 16, UI_TEXT_DIM);
}

// ============================================================================
// Options
// ============================================================================
enum { OPT_ROW_MODE = 0, OPT_ROW_RES, OPT_ROW_APPLY, OPT_ROW_BACK, OPT_ROW_COUNT };

static const float OPT_W = 700.0f, OPT_H = 470.0f;
static Rectangle OptPanel(void) { return Rectangle{ (SCREEN_W - OPT_W) * 0.5f, (SCREEN_H - OPT_H) * 0.5f, OPT_W, OPT_H }; }

static Rectangle OptValueRect(int row)
{
    Rectangle p = OptPanel();
    return Rectangle{ p.x + 290.0f, p.y + 110.0f + row * 76.0f, 360.0f, 50.0f };
}
static Rectangle OptArrowLeft(int row)  { Rectangle v = OptValueRect(row); return Rectangle{ v.x, v.y, 46.0f, v.height }; }
static Rectangle OptArrowRight(int row) { Rectangle v = OptValueRect(row); return Rectangle{ v.x + v.width - 46.0f, v.y, 46.0f, v.height }; }
static Rectangle OptRowRect(int row)
{
    Rectangle p = OptPanel();
    return Rectangle{ p.x + 30.0f, p.y + 104.0f + row * 76.0f, OPT_W - 60.0f, 62.0f };
}
static Rectangle OptButtonRect(int row)   // row = OPT_ROW_APPLY / OPT_ROW_BACK
{
    Rectangle p = OptPanel();
    float bw = 260.0f, bh = 54.0f;
    float y = p.y + 290.0f + (row - OPT_ROW_APPLY) * (bh + 14.0f);
    return Rectangle{ p.x + (OPT_W - bw) * 0.5f, y, bw, bh };
}

static bool OptRowEnabled(const OptionsMenu* m, int row)
{
    return !(row == OPT_ROW_RES && m->mode == DISPLAY_BORDERLESS);
}

static bool OptPending(const OptionsMenu* m)
{
    const DisplaySettings* cur = DisplayGetSettings();
    Resolution r = DisplayResolutionAt(m->resIndex);
    if (m->mode != cur->mode) return true;
    if (m->mode == DISPLAY_BORDERLESS) return false;   // resolusi tidak berpengaruh di borderless
    return r.width != cur->width || r.height != cur->height;
}

static void OptChange(OptionsMenu* m, int row, int dir)
{
    if (row == OPT_ROW_MODE) {
        m->mode = (m->mode + DISPLAY_MODE_COUNT + dir) % DISPLAY_MODE_COUNT;
    }
    else if (row == OPT_ROW_RES && OptRowEnabled(m, row)) {
        int n = DisplayResolutionCount();
        m->resIndex = (m->resIndex + n + dir) % n;
    }
}

static void OptApply(OptionsMenu* m)
{
    DisplaySettings s = *DisplayGetSettings();
    Resolution r = DisplayResolutionAt(m->resIndex);
    s.mode = m->mode;
    s.width = r.width;
    s.height = r.height;
    DisplayApply(&s);
    snprintf(m->status, sizeof(m->status), "Pengaturan diterapkan");
    m->statusTimer = 2.0f;
}

void OptionsMenuOpen(OptionsMenu* m)
{
    const DisplaySettings* cur = DisplayGetSettings();
    m->selected = 0;
    m->mode = cur->mode;
    m->resIndex = DisplayFindResolution(cur->width, cur->height);
    if (m->resIndex < 0) m->resIndex = 0;
    m->status[0] = '\0';
    m->statusTimer = 0.0f;
}

static void OptMoveSelection(OptionsMenu* m, int dir)
{
    for (int k = 0; k < OPT_ROW_COUNT; k++) {
        m->selected = (m->selected + OPT_ROW_COUNT + dir) % OPT_ROW_COUNT;
        if (OptRowEnabled(m, m->selected)) break;
    }
}

bool OptionsMenuUpdate(OptionsMenu* m, float dt)
{
    if (m->statusTimer > 0) m->statusTimer -= dt;
    if (!OptRowEnabled(m, m->selected)) OptMoveSelection(m, 1);

    if (UiPressedBack()) return true;
    if (UiPressedUp())   OptMoveSelection(m, -1);
    if (UiPressedDown()) OptMoveSelection(m, 1);
    if (UiPressedLeft())  OptChange(m, m->selected, -1);
    if (UiPressedRight()) OptChange(m, m->selected, 1);
    if (UiPressedConfirm()) {
        if (m->selected == OPT_ROW_APPLY) OptApply(m);
        else if (m->selected == OPT_ROW_BACK) return true;
        else OptChange(m, m->selected, 1);
    }

    // mouse
    Vector2 mouse = GetMousePosition();
    bool moved = UiMouseMoved();
    bool click = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    for (int row = OPT_ROW_MODE; row <= OPT_ROW_RES; row++) {
        if (!OptRowEnabled(m, row) || !CheckCollisionPointRec(mouse, OptRowRect(row))) continue;
        if (moved) m->selected = row;
        if (click) {
            m->selected = row;
            OptChange(m, row, CheckCollisionPointRec(mouse, OptArrowLeft(row)) ? -1 : 1);
        }
    }
    for (int row = OPT_ROW_APPLY; row <= OPT_ROW_BACK; row++) {
        if (!CheckCollisionPointRec(mouse, OptButtonRect(row))) continue;
        if (moved) m->selected = row;
        if (click) {
            m->selected = row;
            if (row == OPT_ROW_APPLY) OptApply(m);
            else return true;
        }
    }
    return false;
}

static void DrawOptRow(const OptionsMenu* m, int row, const char* label, const char* value)
{
    bool enabled = OptRowEnabled(m, row);
    bool sel = (m->selected == row) && enabled;
    Rectangle rr = OptRowRect(row);
    if (sel) DrawRectangleRounded(rr, 0.2f, 8, Color{ 40, 48, 78, 255 });

    Rectangle p = OptPanel();
    DrawText(label, (int)(p.x + 56), (int)(rr.y + (rr.height - 24) * 0.5f), 24, enabled ? RAYWHITE : UI_TEXT_DIM);

    Rectangle v = OptValueRect(row);
    DrawRectangleRounded(v, 0.25f, 8, enabled ? Color{ 32, 32, 46, 255 } : Color{ 30, 30, 38, 255 });
    DrawRectangleRoundedLinesEx(v, 0.25f, 8, 2.0f, sel ? UI_GOLD : UI_EDGE);

    Color vc = enabled ? RAYWHITE : UI_TEXT_DIM;
    int fs = 22;
    DrawText(value, (int)(v.x + (v.width - MeasureText(value, fs)) * 0.5f), (int)(v.y + (v.height - fs) * 0.5f), fs, vc);
    if (enabled) {
        Color ac = sel ? UI_GOLD : UI_TEXT_DIM;
        Rectangle l = OptArrowLeft(row), r = OptArrowRight(row);
        DrawText("<", (int)(l.x + 16), (int)(l.y + 12), 26, ac);
        DrawText(">", (int)(r.x + 16), (int)(r.y + 12), 26, ac);
    }
}

void OptionsMenuDraw(const OptionsMenu* m)
{
    Rectangle p = OptPanel();
    UiDrawPanel(p);
    UiDrawTextCentered("OPTIONS", p.x + OPT_W * 0.5f, p.y + 28, 40, RAYWHITE);

    DrawOptRow(m, OPT_ROW_MODE, "Mode Layar", DisplayModeName(m->mode));

    char resText[64];
    if (m->mode == DISPLAY_BORDERLESS) {
        Resolution mon = DisplayMonitorSize();
        snprintf(resText, sizeof(resText), "Ikut monitor (%d x %d)", mon.width, mon.height);
    }
    else {
        Resolution r = DisplayResolutionAt(m->resIndex);
        snprintf(resText, sizeof(resText), "%d x %d", r.width, r.height);
    }
    DrawOptRow(m, OPT_ROW_RES, "Resolusi", resText);

    UiDrawButton(OptButtonRect(OPT_ROW_APPLY), "Terapkan", m->selected == OPT_ROW_APPLY, UI_BUTTON_NORMAL, 24);
    UiDrawButton(OptButtonRect(OPT_ROW_BACK), "Kembali", m->selected == OPT_ROW_BACK, UI_BUTTON_NORMAL, 24);

    float infoY = p.y + OPT_H - 34;
    if (m->statusTimer > 0) {
        Color c = Fade(UI_GOLD, fminf(1.0f, m->statusTimer / 0.4f));
        UiDrawTextCentered(m->status, p.x + OPT_W * 0.5f, infoY, 18, c);
    }
    else if (OptPending(m)) {
        UiDrawTextCentered("Ada perubahan yang belum diterapkan", p.x + OPT_W * 0.5f, infoY, 18, Color{ 240, 150, 110, 255 });
    }
    else {
        UiDrawTextCentered("A/D atau klik panah: ubah nilai  |  ESC: kembali", p.x + OPT_W * 0.5f, infoY, 16, UI_TEXT_DIM);
    }
}

// ============================================================================
// Controls
// ============================================================================
// Kunci yang diawali '~' digambar sebagai teks biasa (pemisah), bukan tombol.
typedef struct {
    const char* keys[6];
    const char* desc;
} ControlRow;

static const ControlRow CONTROL_ROWS[] = {
    { { "W", "A", "S", "D", "~atau", "Tombol Panah" }, "Bergerak" },
    { { "Mouse" },                                     "Membidik / arah hadap" },
    { { "Klik Kiri", "~atau", "J" },                   "Menyerang" },
    { { "1", "2", "3", "4" },                          "Ganti senjata" },     // + daftar senjata
    { { "E" },                                         "Bicara dengan NPC" },
    { { "E", "Spasi", "Enter", "Klik Kiri" },          "Lanjutkan dialog" },
    { { "ESC" },                                       "Menu pause (Save, Controls, ...)" },
};
static const int CONTROL_ROW_COUNT = (int)(sizeof(CONTROL_ROWS) / sizeof(CONTROL_ROWS[0]));

static const float CTRL_W = 880.0f, CTRL_H = 600.0f;
static Rectangle CtrlPanel(void) { return Rectangle{ (SCREEN_W - CTRL_W) * 0.5f, (SCREEN_H - CTRL_H) * 0.5f, CTRL_W, CTRL_H }; }
static Rectangle CtrlBackRect(void)
{
    Rectangle p = CtrlPanel();
    return Rectangle{ p.x + (CTRL_W - 260.0f) * 0.5f, p.y + CTRL_H - 92.0f, 260.0f, 52.0f };
}

bool ControlsScreenUpdate(void)
{
    if (UiPressedBack() || UiPressedConfirm()) return true;
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), CtrlBackRect())) return true;
    return false;
}

void ControlsScreenDraw(bool overlay)
{
    if (overlay) DrawRectangle(0, 0, SCREEN_W, SCREEN_H, Color{ 0, 0, 0, 170 });

    Rectangle p = CtrlPanel();
    UiDrawPanel(p);
    UiDrawTextCentered("CONTROLS", p.x + CTRL_W * 0.5f, p.y + 26, 40, RAYWHITE);

    const float keyH = 34.0f;
    const float rowStep = 56.0f;
    const float keysX = p.x + 50.0f;
    const float descX = p.x + 470.0f;
    float y = p.y + 92.0f;

    for (int i = 0; i < CONTROL_ROW_COUNT; i++, y += rowStep) {
        const ControlRow* r = &CONTROL_ROWS[i];
        if (i % 2 == 0) DrawRectangleRounded(Rectangle{ p.x + 26, y - 9, CTRL_W - 52, rowStep - 4 }, 0.2f, 6, Color{ 30, 30, 46, 255 });

        float x = keysX;
        for (int k = 0; k < 6 && r->keys[k]; k++) {
            const char* key = r->keys[k];
            if (key[0] == '~') {
                DrawText(key + 1, (int)x, (int)(y + 8), 18, UI_TEXT_DIM);
                x += MeasureText(key + 1, 18) + 10.0f;
            }
            else {
                x += UiDrawKeyCap(key, x, y, keyH) + 8.0f;
            }
        }

        if (i == 3) {
            // daftar senjata diambil dari data/weapons.cpp supaya selalu sesuai
            DrawText(r->desc, (int)descX, (int)(y - 2), 22, RAYWHITE);
            char list[128] = { 0 };
            int len = 0;
            for (int w = 0; w < WEAPON_COUNT && len < (int)sizeof(list) - 1; w++)
                len += snprintf(list + len, sizeof(list) - len, "%s%d %s", w ? "   " : "", w + 1, WEAPONS[w].name);
            DrawText(list, (int)descX, (int)(y + 22), 16, UI_GOLD_DIM);
        }
        else {
            DrawText(r->desc, (int)descX, (int)(y + 6), 22, RAYWHITE);
        }
    }

    UiDrawButton(CtrlBackRect(), "Kembali", true, UI_BUTTON_NORMAL, 24);
    UiDrawTextCentered("ESC / Enter: kembali", p.x + CTRL_W * 0.5f, p.y + CTRL_H - 30, 16, UI_TEXT_DIM);
}
