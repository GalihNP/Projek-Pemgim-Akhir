#include "raylib.h"
#include "core/types.h"
#include "core/config.h"
#include "systems/input.h"
#include "systems/camera.h"
#include "systems/render.h"
#include "systems/pause_menu.h"
#include "systems/save.h"
#include "world/world.h"
#include "world/background.h"
#include "entities/player.h"
#include "entities/enemy.h"
#include "entities/npc.h"
#include "entities/combat.h"
#include "intro.h"
#include <stdio.h>

static void ShowToast(GameState* s, const char* msg) {
    snprintf(s->toast, sizeof(s->toast), "%s", msg);
    s->toastTimer = 2.2f;
}

// Update gameplay normal (hanya saat MODE_PLAY).
static void UpdateGameplay(GameState* s, const InputState* input, float dt) {
    PlayerUpdate(&s->player, input, dt);
    NpcResolveCircle(s->npcs, s->npcCount, &s->player.position, PLAYER_RADIUS);

    if (!s->player.swingHitDone && s->player.swingTime > 0 && s->player.swingTime <= SWING_DURATION * 0.5f) {
        DoAttack(&s->player, s->enemies, MAX_ENEMIES);
        s->player.swingHitDone = true;
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
    SetExitKey(KEY_NULL);   // ESC dipakai untuk menu pause, bukan menutup game
    SetTargetFPS(60);

    // 1. Jalankan Intro Percakapan Terlebih Dahulu
    bool introSuccess = RunIntroDialogue();
    if (!introSuccess) {
        CloseWindow();
        return 0;
    }

    // 2. Masuk ke Inti Permainan Utama
    static GameState state = {};
    WorldInit(&state.world);

    Vector2 spawn = Vector2{ ROOM_W * 0.5f, ROOM_H * 0.5f };
    BackgroundInit(&state.world, spawn);   // ground tileset + objek lingkungan

    PlayerInit(&state.player, spawn);
    EnemyClearAll(state.enemies, MAX_ENEMIES);
    NpcInitAll(state.npcs, &state.npcCount, &state.world, spawn);
    state.mode = MODE_PLAY;

    // Lanjutkan dari save jika ada
    if (SaveGameExists()) {
        if (SaveGameLoad(&state)) ShowToast(&state, "Save dimuat");
        else ShowToast(&state, "Save tidak valid - mulai baru");
    }
    CameraInit(&state.cam, state.player.position);

    while (!WindowShouldClose() && !state.quitRequested) {
        float dt = GetFrameTime();
        InputState input = ReadInput();

        Vector2 mouseWorldPos = GetScreenToWorld2D(GetMousePosition(), state.cam);
        input.aim = mouseWorldPos;

        // Satu handler per frame, supaya satu tombol tidak terpakai dua kali
        // (mis. E yang membuka dialog tidak langsung melanjutkannya).
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
                else if (SaveGameWrite(&state)) ShowToast(&state, "Game tersimpan");
                else ShowToast(&state, "Gagal menyimpan game");
            }
            else if (action == PAUSE_QUIT) {
                state.quitRequested = true;
            }
        } break;
        }

        // Gameplay hanya berjalan jika tidak sedang dialog / pause
        if (frameMode == MODE_PLAY && state.mode == MODE_PLAY)
            UpdateGameplay(&state, &input, dt);

        if (state.toastTimer > 0) state.toastTimer -= dt;

        RenderGame(&state);
    }

    BackgroundUnload();
    WorldUnload(&state.world);
    CloseWindow();
    return 0;
}
