// core/config.h
#pragma once
#include <math.h>

#define SCREEN_W        1280
#define SCREEN_H        720

#define ROOM_COLS       2
#define ROOM_ROWS       2
#define ROOM_W          2000
#define ROOM_H          1200
#define WALL_T          24
#define DOOR_SIZE       140
#define MAX_WALLS       384   // dinding ruangan + collision alas objek lingkungan
#define MAX_ROOMS       32
#define MAX_ENEMIES     64

#define PLAYER_SPEED    260.0f
#define PLAYER_RADIUS   18.0f
#define PLAYER_IFRAMES  0.6f

#define CAM_LERP        6.0f
#define CAM_TRANS_LERP  3.5f
#define CAM_LOOKAHEAD   50.0f

#define SWING_DURATION  0.18f

// --- NPC & dialog ---
#define MAX_NPCS            16
#define NPC_MAX_LINES       6
#define NPC_RADIUS          16.0f
#define NPC_TALK_RANGE      45.0f    // jarak tepi player <-> tepi NPC agar bisa diajak bicara
#define DIALOGUE_CHARS_PER_SEC 45.0f

// --- Spawn musuh per ruangan ---
#define ENEMY_SPAWN_DELAY     0.7f   // durasi animasi muncul (musuh belum bisa bergerak/diserang)
#define ENEMY_SPAWN_MIN_DIST  320.0f // jarak minimal spawn dari player

// --- Save game ---
#define SAVE_SLOT_COUNT     5
#define SAVE_SLOT_PATH_FMT  "save_slot%d.dat"   // %d = nomor slot 1..5
#define SAVE_FILE_PATH      "savegame.dat"      // format lama (1 file); otomatis dipindah ke slot 1

// --- Pengaturan tampilan ---
#define SETTINGS_FILE_PATH  "settings.cfg"
