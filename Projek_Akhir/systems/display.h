// systems/display.h
// Pengaturan layar: mode (windowed / borderless / fullscreen) + resolusi.
//
// Seluruh game tetap digambar di "canvas" virtual berukuran SCREEN_W x SCREEN_H
// (1280x720). Canvas itu lalu di-skala ke ukuran window sebenarnya (dengan
// letterbox hitam bila rasio berbeda). Posisi mouse otomatis dikonversi ke
// koordinat canvas, jadi kode UI/gameplay lama tidak perlu diubah.
#pragma once
#include "raylib.h"

typedef enum {
    DISPLAY_WINDOWED = 0,
    DISPLAY_BORDERLESS,       // borderless windowed: selalu seukuran monitor
    DISPLAY_FULLSCREEN,       // fullscreen eksklusif pada resolusi yang dipilih
    DISPLAY_MODE_COUNT
} DisplayMode;

typedef struct {
    int width, height;
} Resolution;

typedef struct {
    int mode;                 // DisplayMode
    int width, height;        // resolusi window / fullscreen
} DisplaySettings;

// Panggil sekali setelah InitWindow(): membuat canvas, membaca settings.cfg,
// lalu menerapkan mode layar yang tersimpan.
void DisplayInit(void);
void DisplayShutdown(void);

const DisplaySettings* DisplayGetSettings(void);
// Terapkan pengaturan baru dan simpan ke settings.cfg.
void DisplayApply(const DisplaySettings* s);

// Daftar resolusi yang didukung monitor saat ini (urut kecil -> besar).
int         DisplayResolutionCount(void);
Resolution  DisplayResolutionAt(int index);
int         DisplayFindResolution(int width, int height);   // -1 jika tidak ada
const char* DisplayModeName(int mode);
Resolution  DisplayMonitorSize(void);

// Ganti BeginDrawing()/EndDrawing() dengan pasangan ini.
void DisplayBeginFrame(void);
void DisplayEndFrame(void);
