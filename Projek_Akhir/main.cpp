#include "raylib.h"
#include "core/types.h"
#include "core/config.h"
#include "systems/input.h"
#include "systems/camera.h"
#include "systems/render.h"
#include "systems/pause_menu.h"
#include "systems/save.h"
#include "systems/save_menu.h"
#include "systems/menu.h"
#include "systems/display.h"
#include "world/world.h"
#include "world/background.h"
#include "entities/player.h"
#include "entities/enemy.h"
#include "entities/npc.h"
#include "entities/combat.h"
#include "data/animations.h"
#include "intro.h"
#include <stdio.h>
#include <string.h>

// Layar tingkat aplikasi (di luar gameplay).
typedef enum {
    APP_MAIN_MENU = 0,
    APP_OPTIONS,
    APP_CONTROLS,
    APP_SLOTS,                // pilih save file (Play)
    APP_INTRO,                // percakapan pembuka untuk game baru
    APP_GAME
} AppScreen;

static void ShowToast(GameState* s, const char* msg) {
    snprintf(s->toast, sizeof(s->toast), "%s", msg);
    s->toastTimer = 2.2f;
}

// Kembalikan semua data permainan ke kondisi awal. Dunia (ruangan, dinding,
// background) dibangun sekali saat program mulai dan tidak ikut di-reset.
static void GameReset(GameState* s, Vector2 spawn, int slot) {
    PlayerInit(&s->player, spawn);
    EnemyClearAll(s->enemies, MAX_ENEMIES);
    NpcInitAll(s->npcs, &s->npcCount, &s->world, spawn);

    s->world.currentRoom = 0;
    memset(s->world.roomVisited, 0, sizeof(s->world.roomVisited));
    s->world.transitionCooldown = 0.0f;
    s->world.camTransitionTime = 0.0f;
    s->world.roomTitleTimer = 0.0f;

    s->mode = MODE_PLAY;
    memset(&s->dialogue, 0, sizeof(s->dialogue));
    memset(&s->pause, 0, sizeof(s->pause));
    s->toast[0] = '\0';
    s->toastTimer = 0.0f;
    s->quitRequested = false;
    s->activeSlot = slot;
    s->playTime = 0.0f;

    CameraInit(&s->cam, s->player.position);
}

// Update gameplay normal (hanya saat MODE_PLAY).
static void UpdateGameplay(GameState* s, const InputState* input, float dt) {
    PlayerUpdate(&s->player, input, dt);
    NpcResolveCircle(s->npcs, s->npcCount, &s->player.position, PLAYER_RADIUS);

    // Damage diberikan tepat saat frame "hit" animasi serangan tampil
    // (tebasan penuh pedang, muzzle flash pistol, terjangan keris).
    Player* pl = &s->player;
    if (!pl->swingHitDone && (pl->swingDuration - pl->swingTime) >= pl->hitTime) {
        DoAttack(pl, s->enemies, MAX_ENEMIES);
        pl->swingHitDone = true;
        // Senjata api (punya titik moncong) -> tampilkan jejak peluru
        if (pl->attackWeapon >= 0 && pl->attackWeapon < WEAPON_ANIM_COUNT &&
            WEAPON_ANIMS[pl->attackWeapon].muzzle.x >= 0.0f)
            pl->shotFx = SHOT_FX_DURATION;
    }

    EnemyUpdateAll(s->enemies, MAX_ENEMIES, &s->player, &s->world, dt);
    for (int i = 0; i < MAX_ENEMIES; i++)
        if (s->enemies[i].alive)
            NpcResolveCircle(s->npcs, s->npcCount, &s->enemies[i].position, s->enemies[i].radius);

    WorldUpdateRoom(&s->world, s->player.position, dt);

    // Musuh hanya muncul saat player masuk ke sebuah ruangan untuk pertama kali.
    int room = s->world.currentRoom;
    if (room >= 0 && !s->world.roomVisited[room]) {
        s->world.roomVisited[room] = true;
        EnemySpawnInRoom(s->enemies, MAX_ENEMIES, &s->world, room, s->player.position);
    }

    CameraUpdate(&s->cam, &s->player, &s->world, dt);
}

int main(void) {
    InitWindow(SCREEN_W, SCREEN_H, "Game");
    SetExitKey(KEY_NULL);   // ESC dipakai untuk menu, bukan menutup game
    SetTargetFPS(60);

    // Canvas 1280x720 + mode layar/resolusi dari settings.cfg
    DisplayInit();
    RenderInit(); // Memuat sprite karakter dari folder Asset

    // Dunia dibangun sekali (butuh beberapa detik, ada layar "memuat")
    static GameState state = {};
    WorldInit(&state.world);
    Vector2 spawn = Vector2{ ROOM_W * 0.5f, ROOM_H * 0.5f };
    BackgroundInit(&state.world, spawn);   // ground tileset + objek lingkungan
    GameReset(&state, spawn, -1);

    SaveMigrateLegacy();                    // savegame.dat lama -> Slot 1

    AppScreen   screen = APP_MAIN_MENU;
    MainMenu    mainMenu;
    OptionsMenu options;
    SlotMenu    slots;
    MainMenuOpen(&mainMenu);
    memset(&options, 0, sizeof(options));
    memset(&slots, 0, sizeof(slots));

    float menuTime = 0.0f;
    bool  quit = false;

    while (!WindowShouldClose() && !quit) {
        float dt = GetFrameTime();
        menuTime += dt;

        // ------------------------------------------------------------------
        // UPDATE — satu layar per frame, supaya satu klik/tombol tidak
        // terpakai dua kali (mis. klik "Play" langsung memilih slot).
        // ------------------------------------------------------------------
        switch (screen) {
        case APP_MAIN_MENU:
            switch (MainMenuUpdate(&mainMenu)) {
            case MAINMENU_PLAY:     SlotMenuOpen(&slots, SLOTMENU_LOAD, -1); screen = APP_SLOTS; break;
            case MAINMENU_OPTIONS:  OptionsMenuOpen(&options); screen = APP_OPTIONS; break;
            case MAINMENU_CONTROLS: screen = APP_CONTROLS; break;
            case MAINMENU_EXIT:     quit = true; break;
            default: break;
            }
            break;

        case APP_OPTIONS:
            if (OptionsMenuUpdate(&options, dt)) screen = APP_MAIN_MENU;
            break;

        case APP_CONTROLS:
            if (ControlsScreenUpdate()) screen = APP_MAIN_MENU;
            break;

        case APP_SLOTS: {
            int slot = -1;
            SlotMenuAction act = SlotMenuUpdate(&slots, dt, &slot);
            if (act == SLOTACT_BACK) {
                screen = APP_MAIN_MENU;
            }
            else if (act == SLOTACT_NEW_GAME) {
                GameReset(&state, spawn, slot);
                screen = APP_INTRO;             // intro dijalankan frame berikutnya
            }
            else if (act == SLOTACT_LOAD) {
                GameReset(&state, spawn, slot);
                if (SaveSlotLoad(slot, &state)) {
                    CameraInit(&state.cam, state.player.position);
                    ShowToast(&state, TextFormat("Slot %d dimuat", slot + 1));
                    screen = APP_GAME;
                }
                else {
                    SlotMenuSetStatus(&slots, TextFormat("Gagal memuat Slot %d", slot + 1));
                    SlotMenuRefresh(&slots);
                }
            }
        } break;

        case APP_INTRO:
            // Percakapan pembuka (punya loop sendiri). False = window ditutup.
            if (!RunIntroDialogue()) quit = true;
            else {
                ShowToast(&state, TextFormat("Game baru - Slot %d", state.activeSlot + 1));
                screen = APP_GAME;
            }
            break;

        case APP_GAME: {
            InputState input = ReadInput();
            input.aim = GetScreenToWorld2D(GetMousePosition(), state.cam);

            GameMode frameMode = state.mode;
            switch (frameMode) {
            case MODE_PLAY:
                if (input.pausePressed) {
                    PauseMenuOpen(&state.pause);
                    state.mode = MODE_PAUSE;
                }
                else if (input.interactPressed && state.player.hp > 0) {
                    int n = NpcFindTalkable(state.npcs, state.npcCount, state.player.position);
                    if (n >= 0) {
                        DialogueStart(&state.dialogue, &state.npcs[n], n);
                        state.mode = MODE_DIALOGUE;
                    }
                }
                break;

            case MODE_DIALOGUE:
                if (input.pausePressed ||
                    !DialogueUpdate(&state.dialogue, &state.npcs[state.dialogue.npcIndex], input.advancePressed, dt)) {
                    state.mode = MODE_PLAY;
                }
                break;

            case MODE_PAUSE: {
                PauseAction action = input.pausePressed ? PAUSE_RESUME : PauseMenuUpdate(&state.pause);
                if (action == PAUSE_RESUME) {
                    state.mode = MODE_PLAY;
                }
                else if (action == PAUSE_SAVE) {
                    if (state.player.hp <= 0) ShowToast(&state, "Tidak bisa menyimpan saat mati");
                    else {
                        SlotMenuOpen(&slots, SLOTMENU_SAVE, state.activeSlot);
                        state.mode = MODE_SAVE_SLOTS;
                    }
                }
                else if (action == PAUSE_CONTROLS) {
                    state.mode = MODE_CONTROLS;
                }
                else if (action == PAUSE_MAIN_MENU) {
                    MainMenuOpen(&mainMenu);
                    screen = APP_MAIN_MENU;
                }
                else if (action == PAUSE_QUIT) {
                    quit = true;
                }
            } break;

            case MODE_SAVE_SLOTS: {
                int slot = -1;
                SlotMenuAction act = SlotMenuUpdate(&slots, dt, &slot);
                if (act == SLOTACT_BACK) {
                    state.mode = MODE_PAUSE;
                }
                else if (act == SLOTACT_SAVE) {
                    if (SaveSlotWrite(slot, &state)) {
                        state.activeSlot = slot;
                        ShowToast(&state, TextFormat("Game tersimpan di Slot %d", slot + 1));
                        state.mode = MODE_PAUSE;
                    }
                    else {
                        SlotMenuSetStatus(&slots, TextFormat("Gagal menyimpan ke Slot %d", slot + 1));
                    }
                }
            } break;

            case MODE_CONTROLS:
                if (ControlsScreenUpdate()) state.mode = MODE_PAUSE;
                break;
            }

            // Gameplay hanya berjalan jika tidak sedang dialog / menu
            if (screen == APP_GAME && frameMode == MODE_PLAY && state.mode == MODE_PLAY)
                UpdateGameplay(&state, &input, dt);

            // Waktu bermain dihitung saat bermain / berdialog (bukan saat di menu)
            if (frameMode == MODE_PLAY || frameMode == MODE_DIALOGUE) state.playTime += dt;

            if (state.toastTimer > 0) state.toastTimer -= dt;
        } break;
        }

        // ------------------------------------------------------------------
        // DRAW (ke canvas 1280x720, lalu di-skala ke window)
        // ------------------------------------------------------------------
        if (quit) break;
        // Catatan: frame tetap digambar walau baru pindah ke APP_INTRO, karena
        // EndDrawing() juga mem-poll input (agar klik pemilihan slot tidak ikut
        // melewati baris pertama intro).

        DisplayBeginFrame();
        switch (screen) {
        case APP_MAIN_MENU:
            MenuBackgroundDraw(&state.world, menuTime);
            MainMenuDraw(&mainMenu);
            break;
        case APP_OPTIONS:
            MenuBackgroundDraw(&state.world, menuTime);
            OptionsMenuDraw(&options);
            break;
        case APP_CONTROLS:
            MenuBackgroundDraw(&state.world, menuTime);
            ControlsScreenDraw(false);
            break;
        case APP_SLOTS:
            MenuBackgroundDraw(&state.world, menuTime);
            SlotMenuDraw(&slots, &state.world, false);
            break;
        case APP_INTRO:
            ClearBackground(RAYWHITE);   // satu frame transisi sebelum intro mulai
            break;
        case APP_GAME:
            RenderGame(&state);
            if (state.mode == MODE_SAVE_SLOTS) SlotMenuDraw(&slots, &state.world, true);
            if (state.mode == MODE_CONTROLS)   ControlsScreenDraw(true);
            break;
        }
        DisplayEndFrame();
    }

    RenderUnload();
    DisplayShutdown();
    BackgroundUnload();
    WorldUnload(&state.world);
    CloseWindow();
    return 0;
}
