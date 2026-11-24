// systems/ui.h
// Komponen UI kecil yang dipakai bersama oleh main menu, options, controls,
// dan layar save slot. Gayanya mengikuti menu pause.
#pragma once
#include "raylib.h"

#define UI_GOLD        Color{ 240, 210, 120, 255 }
#define UI_GOLD_DIM    Color{ 200, 170,  90, 255 }
#define UI_PANEL       Color{  22,  22,  34, 245 }
#define UI_BTN         Color{  45,  45,  62, 255 }
#define UI_BTN_SEL     Color{  70,  90, 150, 255 }
#define UI_BTN_DANGER  Color{ 150,  45,  45, 255 }
#define UI_EDGE        Color{  90,  90, 115, 255 }
#define UI_TEXT_DIM    Color{ 150, 150, 170, 255 }

typedef enum {
    UI_BUTTON_NORMAL = 0,
    UI_BUTTON_DANGER,     // merah saat dipilih (Exit, Hapus, dll.)
    UI_BUTTON_DISABLED
} UiButtonStyle;

void UiDrawPanel(Rectangle r);
void UiDrawButton(Rectangle r, const char* label, bool selected, UiButtonStyle style, int fontSize);
void UiDrawTextCentered(const char* text, float centerX, float y, int fontSize, Color c);
// Gambar tombol keyboard ("W", "ESC", ...). Mengembalikan lebarnya.
float UiDrawKeyCap(const char* label, float x, float y, float h);
float UiKeyCapWidth(const char* label, float h);

// true jika mouse bergerak frame ini (hover hanya mengubah pilihan saat mouse bergerak,
// supaya tidak bentrok dengan navigasi keyboard).
bool UiMouseMoved(void);
bool UiPressedUp(void);       // W / panah atas
bool UiPressedDown(void);     // S / panah bawah
bool UiPressedLeft(void);     // A / panah kiri
bool UiPressedRight(void);    // D / panah kanan
bool UiPressedConfirm(void);  // Enter / Spasi
bool UiPressedBack(void);     // ESC / Backspace
