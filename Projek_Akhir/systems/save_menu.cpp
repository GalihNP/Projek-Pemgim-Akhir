// systems/save_menu.cpp
#include "save_menu.h"
#include "ui.h"
#include "core/config.h"
#include "data/weapons.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>

enum { CONFIRM_NONE = 0, CONFIRM_DELETE, CONFIRM_OVERWRITE, CONFIRM_REPLACE_CORRUPT };

// ---------------------------------------------------------------------------
// Layout
// ---------------------------------------------------------------------------
static const float PANEL_W = 940.0f, PANEL_H = 650.0f;
static const float ROW_H = 82.0f, ROW_STEP = 92.0f;
static const int   BACK_INDEX = SAVE_SLOT_COUNT;

static Rectangle Panel(void) { return Rectangle{ (SCREEN_W - PANEL_W) * 0.5f, (SCREEN_H - PANEL_H) * 0.5f, PANEL_W, PANEL_H }; }

static Rectangle RowRect(int i)
{
    Rectangle p = Panel();
    return Rectangle{ p.x + 40.0f, p.y + 104.0f + i * ROW_STEP, PANEL_W - 80.0f, ROW_H };
}

static Rectangle DeleteRect(int i)
{
    Rectangle r = RowRect(i);
    return Rectangle{ r.x + r.width - 140.0f, r.y + (ROW_H - 44.0f) * 0.5f, 120.0f, 44.0f };
}

static Rectangle BackRect(void)
{
    Rectangle p = Panel();
    return Rectangle{ p.x + (PANEL_W - 240.0f) * 0.5f, p.y + PANEL_H - 70.0f, 240.0f, 48.0f };
}

static const float DLG_W = 580.0f, DLG_H = 230.0f;
static Rectangle DialogRect(void) { return Rectangle{ (SCREEN_W - DLG_W) * 0.5f, (SCREEN_H - DLG_H) * 0.5f, DLG_W, DLG_H }; }
static Rectangle DialogButton(int i)   // 0 = Ya, 1 = Tidak
{
    Rectangle d = DialogRect();
    float bw = 200.0f, bh = 50.0f, gap = 30.0f;
    float x0 = d.x + (DLG_W - (bw * 2 + gap)) * 0.5f;
    return Rectangle{ x0 + i * (bw + gap), d.y + DLG_H - 76.0f, bw, bh };
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static void FormatDate(long long t, char* out, size_t n)
{
    if (t <= 0) { snprintf(out, n, "-"); return; }
    time_t tt = (time_t)t;
    struct tm lt;
#if defined(_MSC_VER)
    if (localtime_s(&lt, &tt) != 0) { snprintf(out, n, "-"); return; }
#else
    if (!localtime_r(&tt, &lt)) { snprintf(out, n, "-"); return; }
#endif
    strftime(out, n, "%d/%m/%Y %H:%M", &lt);
}

static void FormatPlayTime(float seconds, char* out, size_t n)
{
    int s = (int)seconds;
    if (s < 0) s = 0;
    snprintf(out, n, "%02d:%02d:%02d", s / 3600, (s / 60) % 60, s % 60);
}

void SlotMenuSetStatus(SlotMenu* m, const char* msg)
{
    snprintf(m->status, sizeof(m->status), "%s", msg);
    m->statusTimer = 2.5f;
}

void SlotMenuRefresh(SlotMenu* m)
{
    for (int i = 0; i < SAVE_SLOT_COUNT; i++) SaveSlotPeek(i, &m->info[i]);
    if (m->selected < SAVE_SLOT_COUNT && !m->info[m->selected].exists) m->focusDelete = false;
}

void SlotMenuOpen(SlotMenu* m, SlotMenuMode mode, int activeSlot)
{
    memset(m, 0, sizeof(*m));
    m->mode = mode;
    m->activeSlot = activeSlot;
    m->selected = (activeSlot >= 0 && activeSlot < SAVE_SLOT_COUNT) ? activeSlot : 0;
    SlotMenuRefresh(m);
}

static void OpenConfirm(SlotMenu* m, int kind, int slot)
{
    m->confirm = kind;
    m->confirmSlot = slot;
    m->confirmSel = 1;            // default "Tidak" agar aman
}

// Klik / Enter pada baris slot.
static SlotMenuAction ActivateSlot(SlotMenu* m, int slot, int* outSlot)
{
    const SaveSlotInfo* in = &m->info[slot];
    *outSlot = slot;

    if (m->mode == SLOTMENU_LOAD) {
        if (!in->exists) return SLOTACT_NEW_GAME;
        if (in->valid)   return SLOTACT_LOAD;
        OpenConfirm(m, CONFIRM_REPLACE_CORRUPT, slot);
        return SLOTACT_NONE;
    }

    // SLOTMENU_SAVE: slot kosong / slot yang sedang dimainkan langsung disimpan
    if (!in->exists || (slot == m->activeSlot && in->valid)) return SLOTACT_SAVE;
    OpenConfirm(m, CONFIRM_OVERWRITE, slot);
    return SLOTACT_NONE;
}

static void RequestDelete(SlotMenu* m, int slot)
{
    if (slot >= 0 && slot < SAVE_SLOT_COUNT && m->info[slot].exists)
        OpenConfirm(m, CONFIRM_DELETE, slot);
}

// Pemain memilih "Ya" pada dialog konfirmasi.
static SlotMenuAction ConfirmYes(SlotMenu* m, int* outSlot)
{
    int kind = m->confirm, slot = m->confirmSlot;
    m->confirm = CONFIRM_NONE;
    *outSlot = slot;

    switch (kind) {
    case CONFIRM_DELETE: {
        char msg[64];
        if (SaveSlotDelete(slot)) snprintf(msg, sizeof(msg), "Slot %d dihapus", slot + 1);
        else snprintf(msg, sizeof(msg), "Gagal menghapus Slot %d", slot + 1);
        SlotMenuSetStatus(m, msg);
        SlotMenuRefresh(m);
        m->focusDelete = false;
        return SLOTACT_NONE;
    }
    case CONFIRM_OVERWRITE:
        return SLOTACT_SAVE;
    case CONFIRM_REPLACE_CORRUPT:
        SaveSlotDelete(slot);
        SlotMenuRefresh(m);
        return SLOTACT_NEW_GAME;
    }
    return SLOTACT_NONE;
}

// ---------------------------------------------------------------------------
// Update
// ---------------------------------------------------------------------------
static SlotMenuAction UpdateConfirm(SlotMenu* m, int* outSlot)
{
    if (UiPressedBack()) { m->confirm = CONFIRM_NONE; return SLOTACT_NONE; }
    if (UiPressedLeft() || UiPressedRight()) m->confirmSel = 1 - m->confirmSel;
    if (UiPressedConfirm()) {
        if (m->confirmSel == 0) return ConfirmYes(m, outSlot);
        m->confirm = CONFIRM_NONE;
        return SLOTACT_NONE;
    }

    Vector2 mouse = GetMousePosition();
    bool moved = UiMouseMoved();
    for (int i = 0; i < 2; i++) {
        if (!CheckCollisionPointRec(mouse, DialogButton(i))) continue;
        if (moved) m->confirmSel = i;
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (i == 0) return ConfirmYes(m, outSlot);
            m->confirm = CONFIRM_NONE;
            return SLOTACT_NONE;
        }
    }
    return SLOTACT_NONE;
}

SlotMenuAction SlotMenuUpdate(SlotMenu* m, float dt, int* outSlot)
{
    if (m->statusTimer > 0) m->statusTimer -= dt;
    if (m->confirm != CONFIRM_NONE) return UpdateConfirm(m, outSlot);

    // --- keyboard ---
    if (UiPressedBack()) return SLOTACT_BACK;
    if (UiPressedUp())   { m->selected = (m->selected + BACK_INDEX) % (BACK_INDEX + 1); m->focusDelete = false; }
    if (UiPressedDown()) { m->selected = (m->selected + 1) % (BACK_INDEX + 1); m->focusDelete = false; }
    bool onSlot = m->selected < BACK_INDEX;
    if (onSlot && (UiPressedLeft() || UiPressedRight()) && m->info[m->selected].exists)
        m->focusDelete = !m->focusDelete;
    if (onSlot && (IsKeyPressed(KEY_DELETE) || IsKeyPressed(KEY_X)))
        RequestDelete(m, m->selected);
    if (UiPressedConfirm()) {
        if (!onSlot) return SLOTACT_BACK;
        if (m->focusDelete) { RequestDelete(m, m->selected); return SLOTACT_NONE; }
        return ActivateSlot(m, m->selected, outSlot);
    }

    // --- mouse ---
    Vector2 mouse = GetMousePosition();
    bool moved = UiMouseMoved();
    bool click = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    for (int i = 0; i < SAVE_SLOT_COUNT; i++) {
        if (!CheckCollisionPointRec(mouse, RowRect(i))) continue;
        bool onDelete = m->info[i].exists && CheckCollisionPointRec(mouse, DeleteRect(i));
        if (moved) { m->selected = i; m->focusDelete = onDelete; }
        if (click) {
            m->selected = i;
            m->focusDelete = onDelete;
            if (onDelete) { RequestDelete(m, i); return SLOTACT_NONE; }
            return ActivateSlot(m, i, outSlot);
        }
    }
    if (CheckCollisionPointRec(mouse, BackRect())) {
        if (moved) { m->selected = BACK_INDEX; m->focusDelete = false; }
        if (click) return SLOTACT_BACK;
    }
    return SLOTACT_NONE;
}

// ---------------------------------------------------------------------------
// Draw
// ---------------------------------------------------------------------------
static void DrawSlotRow(const SlotMenu* m, int i, const World* w)
{
    const SaveSlotInfo* in = &m->info[i];
    Rectangle r = RowRect(i);
    bool sel = (m->selected == i) && m->confirm == CONFIRM_NONE;
    bool rowFocus = sel && !m->focusDelete;

    DrawRectangleRounded(r, 0.18f, 8, rowFocus ? Color{ 48, 60, 102, 255 } : Color{ 32, 32, 48, 255 });
    DrawRectangleRoundedLinesEx(r, 0.18f, 8, 2.0f, sel ? UI_GOLD : UI_EDGE);

    // Kolom kiri: nomor slot
    DrawText(TextFormat("SLOT %d", i + 1), (int)(r.x + 22), (int)(r.y + 16), 26, in->exists ? UI_GOLD : UI_TEXT_DIM);
    if (i == m->activeSlot) {
        Rectangle badge = { r.x + 22, r.y + 50, 66, 20 };
        DrawRectangleRounded(badge, 0.5f, 6, Color{ 60, 110, 70, 255 });
        DrawText("AKTIF", (int)(badge.x + (badge.width - MeasureText("AKTIF", 14)) * 0.5f), (int)(badge.y + 3), 14, RAYWHITE);
    }

    // Kolom tengah: isi slot
    float ix = r.x + 160.0f;
    DrawRectangle((int)(ix - 20), (int)(r.y + 14), 2, (int)(ROW_H - 28), Color{ 60, 60, 80, 255 });

    if (!in->exists) {
        DrawText("Kosong", (int)ix, (int)(r.y + 16), 24, UI_TEXT_DIM);
        const char* hint = (m->mode == SLOTMENU_LOAD) ? "Pilih untuk memulai game baru" : "Pilih untuk menyimpan di sini";
        DrawText(hint, (int)ix, (int)(r.y + 48), 16, Color{ 120, 120, 140, 255 });
    }
    else if (!in->valid) {
        DrawText("Data rusak / tidak dikenali", (int)ix, (int)(r.y + 16), 22, Color{ 230, 110, 110, 255 });
        const char* hint = (m->mode == SLOTMENU_LOAD) ? "Pilih untuk menimpa dengan game baru, atau hapus" : "Pilih untuk menimpa slot ini";
        DrawText(hint, (int)ix, (int)(r.y + 48), 16, Color{ 120, 120, 140, 255 });
    }
    else {
        const char* roomName = (in->room >= 0 && in->room < w->roomCount) ? w->rooms[in->room].name : "?";
        int fs = 24;
        DrawText(roomName, (int)ix, (int)(r.y + 14), fs, RAYWHITE);
        float x = ix + MeasureText(roomName, fs) + 24.0f;

        const char* hpText = TextFormat("HP %.0f/%.0f", in->hp, in->maxHp);
        DrawText(hpText, (int)x, (int)(r.y + 18), 18, Color{ 230, 110, 110, 255 });
        x += MeasureText(hpText, 18) + 20.0f;
        if (in->weaponIndex >= 0 && in->weaponIndex < WEAPON_COUNT)
            DrawText(WEAPONS[in->weaponIndex].name, (int)x, (int)(r.y + 18), 18, WEAPONS[in->weaponIndex].color);

        char date[40], play[24];
        FormatDate(in->savedAt, date, sizeof(date));
        FormatPlayTime(in->playTime, play, sizeof(play));
        DrawText(TextFormat("Disimpan: %s     Waktu main: %s", date, play), (int)ix, (int)(r.y + 50), 16, UI_TEXT_DIM);
    }

    // Kolom kanan: tombol hapus
    if (in->exists)
        UiDrawButton(DeleteRect(i), "Hapus", sel && m->focusDelete, UI_BUTTON_DANGER, 20);
}

static void DrawConfirm(const SlotMenu* m)
{
    DrawRectangle(0, 0, SCREEN_W, SCREEN_H, Color{ 0, 0, 0, 140 });
    Rectangle d = DialogRect();
    UiDrawPanel(d);

    const char* title = "";
    const char* line = "";
    switch (m->confirm) {
    case CONFIRM_DELETE:
        title = TextFormat("Hapus Slot %d?", m->confirmSlot + 1);
        line = "Data save di slot ini akan hilang permanen.";
        break;
    case CONFIRM_OVERWRITE:
        title = TextFormat("Timpa Slot %d?", m->confirmSlot + 1);
        line = "Save lama di slot ini akan diganti progres sekarang.";
        break;
    case CONFIRM_REPLACE_CORRUPT:
        title = TextFormat("Slot %d tidak bisa dibaca", m->confirmSlot + 1);
        line = "Hapus data ini dan mulai game baru di slot ini?";
        break;
    }
    // TextFormat memakai buffer bergilir, jadi aman dipakai untuk dua string di sini
    UiDrawTextCentered(title, d.x + DLG_W * 0.5f, d.y + 32, 30, RAYWHITE);
    UiDrawTextCentered(line, d.x + DLG_W * 0.5f, d.y + 84, 18, UI_TEXT_DIM);

    bool danger = (m->confirm == CONFIRM_DELETE || m->confirm == CONFIRM_REPLACE_CORRUPT);
    UiDrawButton(DialogButton(0), "Ya", m->confirmSel == 0, danger ? UI_BUTTON_DANGER : UI_BUTTON_NORMAL, 24);
    UiDrawButton(DialogButton(1), "Tidak", m->confirmSel == 1, UI_BUTTON_NORMAL, 24);
}

void SlotMenuDraw(const SlotMenu* m, const World* w, bool overlay)
{
    if (overlay) DrawRectangle(0, 0, SCREEN_W, SCREEN_H, Color{ 0, 0, 0, 170 });

    Rectangle p = Panel();
    UiDrawPanel(p);

    const char* title = (m->mode == SLOTMENU_LOAD) ? "PILIH SAVE FILE" : "SIMPAN KE SLOT";
    UiDrawTextCentered(title, p.x + PANEL_W * 0.5f, p.y + 24, 36, RAYWHITE);

    if (m->statusTimer > 0) {
        Color c = Fade(UI_GOLD, fminf(1.0f, m->statusTimer / 0.4f));
        UiDrawTextCentered(m->status, p.x + PANEL_W * 0.5f, p.y + 70, 18, c);
    }
    else {
        const char* sub = (m->mode == SLOTMENU_LOAD)
            ? "Pilih slot untuk melanjutkan, atau slot kosong untuk game baru"
            : "Pilih slot tujuan penyimpanan";
        UiDrawTextCentered(sub, p.x + PANEL_W * 0.5f, p.y + 70, 16, UI_TEXT_DIM);
    }

    for (int i = 0; i < SAVE_SLOT_COUNT; i++) DrawSlotRow(m, i, w);

    UiDrawButton(BackRect(), "Kembali", m->selected == BACK_INDEX && m->confirm == CONFIRM_NONE, UI_BUTTON_NORMAL, 24);
    DrawText("W/S: pilih   A/D: ke tombol Hapus   Del/X: hapus   ESC: kembali",
             (int)(p.x + 40), (int)(p.y + PANEL_H - 18), 12, Color{ 110, 110, 130, 255 });

    if (m->confirm != CONFIRM_NONE) DrawConfirm(m);
}
