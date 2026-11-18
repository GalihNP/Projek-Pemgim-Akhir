#include "render.h"
#include "core/config.h"
#include "data/weapons.h"
#include "world/collision.h"
#include "world/background.h"
#include "entities/npc.h"
#include "entities/enemy.h"
#include "systems/pause_menu.h"
#include "raymath.h"
#include <math.h>

// Dipanggil di antara DisplayBeginFrame() / DisplayEndFrame() (lihat systems/display.h).
void RenderGame(const GameState* s) {
    ClearBackground(Color{ 24, 24, 32, 255 });

    BeginMode2D(s->cam);

    // Ground lingkungan (tileset seamless) + tembok batu + bayangan objek
    BackgroundDrawGround(&s->world);

    // Objek lingkungan yang berada DI BELAKANG player (alas di atas posisi player)
    BackgroundDrawDecor(s->player.position.y, false);

    // NPC (prompt [E] hanya saat bermain normal)
    int talkable = (s->mode == MODE_PLAY && s->player.hp > 0)
        ? NpcFindTalkable(s->npcs, s->npcCount, s->player.position) : -1;
    int speaking = (s->mode == MODE_DIALOGUE) ? s->dialogue.npcIndex : -1;
    NpcDrawAll(s->npcs, s->npcCount, talkable, speaking);

    // Enemies
    for (int i = 0; i < MAX_ENEMIES; i++) {
        const Enemy* e = &s->enemies[i];
        if (!e->alive) continue;

        // Animasi muncul: lingkaran sihir mengecil + badan memudar masuk
        if (e->spawnTimer > 0) {
            float t = 1.0f - Clamp(e->spawnTimer / ENEMY_SPAWN_DELAY, 0.0f, 1.0f);
            float ringR = e->radius + 26.0f * (1.0f - t);
            DrawRing(e->position, ringR - 3.0f, ringR, 0, 360, 32, Fade(Color{ 200, 60, 200, 255 }, 0.35f + 0.5f * t));
            DrawCircleV(e->position, e->radius * t, Fade(Color{ 200, 60, 60, 255 }, t));
            continue;
        }

        DrawCircleV(e->position, e->radius, e->hitFlash > 0 ? WHITE : Color{ 200, 60, 60, 255 });

        float ratio = Clamp(e->hp / e->maxHp, 0.0f, 1.0f);
        DrawRectangle((int)(e->position.x - 20), (int)(e->position.y - e->radius - 12), 40, 5, Color{ 60, 0, 0, 255 });
        DrawRectangle((int)(e->position.x - 20), (int)(e->position.y - e->radius - 12), (int)(40 * ratio), 5, Color{ 220, 50, 50, 255 });
    }

    // Swing arc
    const Weapon* wp = &WEAPONS[s->player.weaponIndex];
    if (s->player.swingTime > 0) {
        float a = s->player.swingTime / SWING_DURATION;
        float ang = atan2f(s->player.facing.y, s->player.facing.x) * RAD2DEG;
        float half = wp->arc * RAD2DEG * 0.5f;
        Color c = wp->color;
        c.a = (unsigned char)(170 * a);
        DrawCircleSector(s->player.position, wp->range, ang - half, ang + half, 32, c);
    }

    // Player
    Color pc = (s->player.iframes > 0 && ((int)(s->player.iframes * 20) % 2)) ? Color{ 200, 255, 200, 255 } : GREEN;
    DrawCircleV(s->player.position, PLAYER_RADIUS, pc);

    // Garis Aiming (dari player ke arah mouse)
    Vector2 aimLineEnd = Vector2Add(s->player.position, Vector2Scale(s->player.facing, 60.0f));
    DrawLineEx(s->player.position, aimLineEnd, 2.0f, Fade(LIME, 0.5f));

    // Objek lingkungan DI DEPAN player (pohon/gedung menutupi player dari belakang)
    BackgroundDrawDecor(s->player.position.y, true);

    EndMode2D();

    // --- CROSSHAIR (Digambar setelah EndMode2D agar menempel di layar) ---
    if (s->mode == MODE_PLAY) {
        Vector2 mouseScreen = GetMousePosition();
        DrawCircleLines((int)mouseScreen.x, (int)mouseScreen.y, 8.0f, LIGHTGRAY);
        DrawLine((int)mouseScreen.x - 12, (int)mouseScreen.y, (int)mouseScreen.x - 4, (int)mouseScreen.y, LIGHTGRAY);
        DrawLine((int)mouseScreen.x + 4, (int)mouseScreen.y, (int)mouseScreen.x + 12, (int)mouseScreen.y, LIGHTGRAY);
        DrawLine((int)mouseScreen.x, (int)mouseScreen.y - 12, (int)mouseScreen.x, (int)mouseScreen.y - 4, LIGHTGRAY);
        DrawLine((int)mouseScreen.x, (int)mouseScreen.y + 4, (int)mouseScreen.x, (int)mouseScreen.y + 12, LIGHTGRAY);
    }

    // HUD
    DrawRectangle(10, 10, 280, 112, Color{ 0, 0, 0, 180 });
    DrawText(TextFormat("HP: %.0f / %.0f", s->player.hp, s->player.maxHp), 20, 20, 20, RAYWHITE);
    DrawRectangle(20, 44, 240, 12, Color{ 60, 0, 0, 255 });
    DrawRectangle(20, 44, (int)(240 * Clamp(s->player.hp / s->player.maxHp, 0.0f, 1.0f)), 12, Color{ 220, 50, 50, 255 });
    DrawText(TextFormat("[%d] %s", s->player.weaponIndex + 1, wp->name), 20, 62, 18, wp->color);
    int enemiesHere = EnemyCountInRoom(s->enemies, MAX_ENEMIES, &s->world, s->world.currentRoom);
    DrawText(TextFormat("Musuh di ruangan: %d", enemiesHere), 20, 88, 16,
        enemiesHere > 0 ? Color{ 240, 120, 120, 255 } : Color{ 140, 220, 140, 255 });

    // Room title
    if (s->world.roomTitleTimer > 0 && s->world.currentRoom >= 0) {
        float fadeIn = Clamp((2.0f - s->world.roomTitleTimer) / 0.3f, 0.0f, 1.0f);
        float fadeOut = Clamp(s->world.roomTitleTimer / 0.4f, 0.0f, 1.0f);
        Color c = RAYWHITE;
        c.a = (unsigned char)(255 * fadeIn * fadeOut);
        const char* rn = s->world.rooms[s->world.currentRoom].name;
        int sz = 36;
        DrawText(rn, SCREEN_W / 2 - MeasureText(rn, sz) / 2, 40, sz, c);
    }

    if (s->player.hp <= 0) {
        DrawText("DEAD", SCREEN_W / 2 - 50, SCREEN_H / 2 - 20, 40, RED);
    }

    BackgroundDrawWarning();
    DrawFPS(SCREEN_W - 90, 10);

    // Kotak dialog NPC
    if (s->mode == MODE_DIALOGUE)
        DialogueDraw(&s->dialogue, &s->npcs[s->dialogue.npcIndex]);

    // Menu pause (di atas semuanya)
    if (s->mode == MODE_PAUSE)
        PauseMenuDraw(&s->pause);

    // Toast (pesan singkat, mis. "Game tersimpan")
    if (s->toastTimer > 0) {
        float a = Clamp(s->toastTimer / 0.4f, 0.0f, 1.0f);
        int fs = 20;
        int tw = MeasureText(s->toast, fs);
        int tx = SCREEN_W / 2 - tw / 2;
        int ty = (s->mode == MODE_PAUSE) ? SCREEN_H / 2 + 250 : 100;   // di bawah panel pause
        DrawRectangle(tx - 14, ty - 8, tw + 28, fs + 16, Fade(Color{ 20, 20, 30, 255 }, 0.85f * a));
        DrawText(s->toast, tx, ty, fs, Fade(Color{ 240, 210, 120, 255 }, a));
    }
}