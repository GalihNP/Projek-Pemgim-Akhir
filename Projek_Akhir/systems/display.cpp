// systems/display.cpp
#include "display.h"
#include "core/config.h"
#include <fstream>
#include <string>
#include <cstdlib>

static RenderTexture2D s_canvas = { 0 };
static DisplaySettings s_settings = { DISPLAY_WINDOWED, SCREEN_W, SCREEN_H };
static bool s_inFrame = false;

static Resolution s_res[16];
static int s_resCount = 0;

static const char* MODE_NAMES[DISPLAY_MODE_COUNT] = { "Windowed", "Borderless", "Fullscreen" };

const char* DisplayModeName(int mode)
{
    return (mode >= 0 && mode < DISPLAY_MODE_COUNT) ? MODE_NAMES[mode] : "?";
}

Resolution DisplayMonitorSize(void)
{
    int m = GetCurrentMonitor();
    Resolution r = { GetMonitorWidth(m), GetMonitorHeight(m) };
    if (r.width <= 0 || r.height <= 0) r = Resolution{ SCREEN_W, SCREEN_H };
    return r;
}

// Resolusi umum 16:9 (+ resolusi asli monitor) yang muat di monitor.
static void BuildResolutionList(void)
{
    static const Resolution CANDIDATES[] = {
        {  960,  540 }, { 1280,  720 }, { 1366,  768 }, { 1600,  900 },
        { 1920, 1080 }, { 2560, 1440 }, { 3200, 1800 }, { 3840, 2160 },
    };
    Resolution mon = DisplayMonitorSize();

    s_resCount = 0;
    for (const Resolution& r : CANDIDATES)
        if (r.width <= mon.width && r.height <= mon.height) s_res[s_resCount++] = r;

    // resolusi asli monitor (mis. 1920x1200, 1440x900) jika belum ada
    bool hasNative = false;
    for (int i = 0; i < s_resCount; i++)
        if (s_res[i].width == mon.width && s_res[i].height == mon.height) hasNative = true;
    if (!hasNative && s_resCount < 16) {
        int at = s_resCount;
        while (at > 0 && (s_res[at - 1].width > mon.width ||
              (s_res[at - 1].width == mon.width && s_res[at - 1].height > mon.height))) {
            s_res[at] = s_res[at - 1];
            at--;
        }
        s_res[at] = mon;
        s_resCount++;
    }
    if (s_resCount == 0) s_res[s_resCount++] = Resolution{ SCREEN_W, SCREEN_H };
}

int DisplayResolutionCount(void) { return s_resCount; }

Resolution DisplayResolutionAt(int index)
{
    if (index < 0 || index >= s_resCount) return Resolution{ SCREEN_W, SCREEN_H };
    return s_res[index];
}

int DisplayFindResolution(int width, int height)
{
    for (int i = 0; i < s_resCount; i++)
        if (s_res[i].width == width && s_res[i].height == height) return i;
    return -1;
}

const DisplaySettings* DisplayGetSettings(void) { return &s_settings; }

// ---------------------------------------------------------------------------
// settings.cfg (teks sederhana: key=value)
// ---------------------------------------------------------------------------
static void LoadSettingsFile(DisplaySettings* out)
{
    std::ifstream f(SETTINGS_FILE_PATH);
    if (!f) return;
    std::string line;
    while (std::getline(f, line)) {
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = line.substr(0, eq);
        int value = atoi(line.c_str() + eq + 1);
        if (key == "display_mode") out->mode = value;
        else if (key == "width")   out->width = value;
        else if (key == "height")  out->height = value;
    }
}

static void SaveSettingsFile(const DisplaySettings* s)
{
    std::ofstream f(SETTINGS_FILE_PATH, std::ios::trunc);
    if (!f) return;
    f << "display_mode=" << s->mode << "\n";
    f << "width=" << s->width << "\n";
    f << "height=" << s->height << "\n";
}

// Pastikan pengaturan valid untuk monitor ini.
static DisplaySettings Sanitize(DisplaySettings s)
{
    if (s.mode < 0 || s.mode >= DISPLAY_MODE_COUNT) s.mode = DISPLAY_WINDOWED;
    if (DisplayFindResolution(s.width, s.height) < 0) {
        // pakai resolusi terbesar yang tidak melebihi permintaan (fallback: 1280x720 / terkecil)
        int pick = DisplayFindResolution(SCREEN_W, SCREEN_H);
        for (int i = 0; i < s_resCount; i++)
            if (s_res[i].width <= s.width && s_res[i].height <= s.height) pick = i;
        if (pick < 0) pick = 0;
        s.width = s_res[pick].width;
        s.height = s_res[pick].height;
    }
    return s;
}

// ---------------------------------------------------------------------------
// Penerapan mode layar
// ---------------------------------------------------------------------------
static void LeaveSpecialModes(void)
{
    if (IsWindowFullscreen()) ToggleFullscreen();
    if (IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE)) ToggleBorderlessWindowed();
    if (IsWindowState(FLAG_WINDOW_UNDECORATED)) ClearWindowState(FLAG_WINDOW_UNDECORATED);
}

static void CenterWindow(int w, int h)
{
    int m = GetCurrentMonitor();
    Vector2 mp = GetMonitorPosition(m);
    int mw = GetMonitorWidth(m), mh = GetMonitorHeight(m);
    int x = (int)mp.x + (mw - w) / 2;
    int y = (int)mp.y + (mh - h) / 2;
    if (y < (int)mp.y + 30) y = (int)mp.y + 30;   // jangan sampai title bar keluar layar
    if (x < (int)mp.x) x = (int)mp.x;
    SetWindowPosition(x, y);
}

void DisplayApply(const DisplaySettings* req)
{
    s_settings = Sanitize(*req);
    LeaveSpecialModes();

    switch (s_settings.mode) {
    case DISPLAY_WINDOWED:
        SetWindowSize(s_settings.width, s_settings.height);
        CenterWindow(s_settings.width, s_settings.height);
        break;

    case DISPLAY_BORDERLESS:
        // ukuran window mengikuti monitor; resolusi pilihan dipakai lagi saat kembali ke windowed
        ToggleBorderlessWindowed();
        break;

    case DISPLAY_FULLSCREEN: {
        SetWindowSize(s_settings.width, s_settings.height);
        ToggleFullscreen();                      // masuk fullscreen (ukuran desktop)
        Resolution mon = DisplayMonitorSize();
        if (mon.width != s_settings.width || mon.height != s_settings.height)
            SetWindowSize(s_settings.width, s_settings.height);   // ganti video mode monitor
    } break;
    }

    SaveSettingsFile(&s_settings);
}

void DisplayInit(void)
{
    s_canvas = LoadRenderTexture(SCREEN_W, SCREEN_H);
    SetTextureFilter(s_canvas.texture, TEXTURE_FILTER_BILINEAR);

    BuildResolutionList();

    DisplaySettings saved = s_settings;
    LoadSettingsFile(&saved);
    DisplayApply(&saved);
}

void DisplayShutdown(void)
{
    if (s_canvas.id) UnloadRenderTexture(s_canvas);
    s_canvas = RenderTexture2D{ 0 };
}

// ---------------------------------------------------------------------------
// Frame
// ---------------------------------------------------------------------------
static float s_scale = 1.0f;
static float s_offX = 0.0f, s_offY = 0.0f;

static void UpdateViewport(void)
{
    float sw = (float)GetScreenWidth();
    float sh = (float)GetScreenHeight();
    if (sw <= 0 || sh <= 0) return;   // window di-minimize

    float sx = sw / SCREEN_W, sy = sh / SCREEN_H;
    s_scale = (sx < sy) ? sx : sy;
    s_offX = (sw - SCREEN_W * s_scale) * 0.5f;
    s_offY = (sh - SCREEN_H * s_scale) * 0.5f;

    // GetMousePosition() = (posisi asli + offset) * scale -> koordinat canvas
    SetMouseOffset((int)-s_offX, (int)-s_offY);
    SetMouseScale(1.0f / s_scale, 1.0f / s_scale);
}

void DisplayBeginFrame(void)
{
    if (s_inFrame) return;
    s_inFrame = true;
    UpdateViewport();
    BeginTextureMode(s_canvas);
}

void DisplayEndFrame(void)
{
    if (!s_inFrame) return;
    s_inFrame = false;
    EndTextureMode();

    BeginDrawing();
    ClearBackground(BLACK);
    Rectangle src = { 0, 0, (float)s_canvas.texture.width, -(float)s_canvas.texture.height };  // render texture terbalik
    Rectangle dst = { s_offX, s_offY, SCREEN_W * s_scale, SCREEN_H * s_scale };
    DrawTexturePro(s_canvas.texture, src, dst, Vector2{ 0, 0 }, 0.0f, WHITE);
    EndDrawing();
}
