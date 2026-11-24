// systems/menu.h
// Main menu (Play / Options / Controls / Exit), layar Options (mode layar &
// resolusi), dan layar Controls (petunjuk tombol).
#pragma once
#include "core/types.h"

// ---------------- Main menu ----------------
typedef enum {
    MAINMENU_NONE = 0,
    MAINMENU_PLAY,
    MAINMENU_OPTIONS,
    MAINMENU_CONTROLS,
    MAINMENU_EXIT
} MainMenuAction;

typedef struct {
    int selected;
} MainMenu;

void           MainMenuOpen(MainMenu* m);
MainMenuAction MainMenuUpdate(MainMenu* m);
void           MainMenuDraw(const MainMenu* m);

// Latar belakang menu: dunia game yang bergeser pelan + lapisan gelap.
void MenuBackgroundDraw(const World* w, float time);

// ---------------- Options ----------------
typedef struct {
    int   selected;           // baris: 0 mode, 1 resolusi, 2 Terapkan, 3 Kembali
    int   mode;               // pilihan (belum tentu diterapkan)
    int   resIndex;
    char  status[64];
    float statusTimer;
} OptionsMenu;

void OptionsMenuOpen(OptionsMenu* m);
// Mengembalikan true jika pemain menutup layar Options.
bool OptionsMenuUpdate(OptionsMenu* m, float dt);
void OptionsMenuDraw(const OptionsMenu* m);

// ---------------- Controls ----------------
// Mengembalikan true jika pemain menutup layar Controls.
bool ControlsScreenUpdate(void);
// overlay = true saat dibuka dari menu pause (latar game digelapkan).
void ControlsScreenDraw(bool overlay);
