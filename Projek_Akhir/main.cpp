// main.cpp
#include "raylib.h"
#include "core/types.h"
#include "core/config.h"
#include "systems/input.h"
#include "systems/camera.h"
#include "systems/render.h"
#include "world/world.h"
#include "entities/player.h"
#include "entities/enemy.h"
#include "entities/combat.h"

int main(void) {
    InitWindow(SCREEN_W, SCREEN_H, "Game");
    SetTargetFPS(60);

    GameState state = { 0 };
    WorldInit(&state.world);

    // Perbaikan: Tambahkan posisi awal
    PlayerInit(&state.player, Vector2{ ROOM_W * 0.5f, ROOM_H * 0.5f });
    EnemyInitAll(state.enemies, MAX_ENEMIES, state.player.position);
    CameraInit(&state.cam, state.player.position);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        InputState input = ReadInput();

        Vector2 mouseWorldPos = GetScreenToWorld2D(GetMousePosition(), state.cam);
        input.aim = mouseWorldPos;

        PlayerUpdate(&state.player, &input, dt);

        // Logika serangan dipindah ke sini
        if (!state.player.swingHitDone && state.player.swingTime > 0 && state.player.swingTime <= SWING_DURATION * 0.5f) {
            DoAttack(&state.player, state.enemies, MAX_ENEMIES);
            state.player.swingHitDone = true;
        }

        EnemyUpdateAll(state.enemies, MAX_ENEMIES, &state.player, dt);
        WorldUpdateRoom(&state.world, state.player.position, dt);
        CameraUpdate(&state.cam, &state.player, &state.world, dt);

        RenderGame(&state);
    }

    WorldUnload(&state.world);
    CloseWindow();
    return 0;
}