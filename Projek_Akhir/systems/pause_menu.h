// systems/pause_menu.h
#pragma once
#include "core/types.h"

typedef enum {
    PAUSE_NONE = 0,
    PAUSE_RESUME,
    PAUSE_SAVE,
    PAUSE_QUIT
} PauseAction;

void        PauseMenuOpen(PauseMenu* m);
// Navigasi: mouse (hover + klik) atau W/S / panah + Enter/Spasi.
PauseAction PauseMenuUpdate(PauseMenu* m);
// Digambar di layar (di luar BeginMode2D), di atas tampilan game.
void        PauseMenuDraw(const PauseMenu* m);
