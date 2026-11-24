// core/types.h
#pragma once
#include "raylib.h"
#include "config.h"

typedef struct {
    Rectangle bounds;
    Color     floor;
    const char* name;
} Room;

typedef struct {
    Room rooms[MAX_ROOMS];
    int  roomCount;
    int  currentRoom;
    float transitionCooldown;
    float camTransitionTime;
    float roomTitleTimer;
    bool  roomVisited[MAX_ROOMS];   // true = musuh ruangan ini sudah di-spawn
} World;

typedef struct {
    Vector2 position, facing, velocity;
    float   hp, maxHp;
    int     weaponIndex;
    int     skill;            // <--- TAMBAHKAN INI
    float   attackCd, swingTime, iframes;
    bool    swingHitDone;

    // --- Animasi serangan & gerak (lihat data/animations.h) ---
    float   swingDuration;    // total durasi animasi serangan yang sedang berjalan
    float   hitTime;          // detik sejak serangan dimulai saat damage diberikan
    int     attackWeapon;     // senjata yang dipakai serangan aktif (dikunci saat mulai)
    Vector2 attackDir;        // arah serangan (dikunci saat mulai, sinkron dgn visual)
    float   shotFx;           // > 0 = efek jejak peluru pistol sedang tampil
    bool    moving;           // true = sedang berjalan (animasi jalan)
    float   walkTime;         // akumulasi waktu animasi jalan
} Player;

typedef struct {
    Vector2 position, velocity;
    float   radius, hp, maxHp;
    bool    alive;
    float   hitFlash, attackCd;
    float   spawnTimer;       // > 0 = sedang animasi muncul
} Enemy;

typedef struct {
    Vector2     position;
    float       radius;
    int         room;
    const char* name;
    Color       color;
    const char* lines[NPC_MAX_LINES];
    int         lineCount;
    bool        talked;       // sudah pernah diajak bicara (untuk tanda "!")
    int         spriteIndex;  // 0..2: NPC 1, NPC 2, NPC 3
} Npc;

typedef struct {
    int   npcIndex;           // NPC yang sedang bicara
    int   line;               // baris dialog aktif
    float revealTime;         // efek mesin ketik
} DialogueState;

typedef struct {
    int selected;             // 0 = Resume, 1 = Save, 2 = Controls, 3 = Main Menu, 4 = Quit
} PauseMenu;

typedef enum {
    MODE_PLAY = 0,
    MODE_DIALOGUE,
    MODE_PAUSE,
    MODE_SAVE_SLOTS,          // dari pause -> Save: pilih slot tujuan
    MODE_CONTROLS             // dari pause -> Controls: petunjuk tombol
} GameMode;

typedef struct {
    Vector2  move;
    Vector2  aim;
    bool     attackPressed;
    bool     attackHeld;
    int      weaponSwitch;     // -1 = tidak ganti senjata
    bool     interactPressed;  // E
    bool     advancePressed;   // E / Spasi / Enter / klik kiri (lanjut dialog)
    bool     pausePressed;     // ESC
} InputState;

typedef struct {
    World    world;
    Player   player;
    Enemy    enemies[MAX_ENEMIES];
    Npc      npcs[MAX_NPCS];
    int      npcCount;
    Camera2D cam;

    GameMode      mode;
    DialogueState dialogue;
    PauseMenu     pause;

    char     toast[64];        // pesan singkat di layar ("Game tersimpan", dst.)
    float    toastTimer;
    bool     quitRequested;

    int      activeSlot;       // slot save yang sedang dimainkan (0..SAVE_SLOT_COUNT-1)
    float    playTime;         // total waktu bermain (detik), ikut disimpan
} GameState;