#include "render.h"
#include "core/config.h"
#include "data/weapons.h"
#include "world/collision.h"
#include "world/background.h"
#include "raymath.h"
#include <math.h>

void RenderGame(const GameState* s) {
    BeginDrawing();
    ClearBackground(Color{ 24, 24, 32, 255 });

    BeginMode2D(s->cam);

    // Ground lingkungan (tileset seamless) + tembok batu + bayangan objek
    BackgroundDrawGround(&s->world);

    // Objek lingkungan yang berada DI BELAKANG player (alas di atas posisi player)
    BackgroundDrawDecor(s->player.position.y, false);

    // Enemies
    for (int i = 0; i < MAX_ENEMIES; i++) {
        const Enemy* e = &s->enemies[i];
        if (!e->alive) continue;

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
    Vector2 mouseScreen = GetMousePosition();
    DrawCircleLines((int)mouseScreen.x, (int)mouseScreen.y, 8.0f, LIGHTGRAY);
    DrawLine((int)mouseScreen.x - 12, (int)mouseScreen.y, (int)mouseScreen.x - 4, (int)mouseScreen.y, LIGHTGRAY);
    DrawLine((int)mouseScreen.x + 4, (int)mouseScreen.y, (int)mouseScreen.x + 12, (int)mouseScreen.y, LIGHTGRAY);
    DrawLine((int)mouseScreen.x, (int)mouseScreen.y - 12, (int)mouseScreen.x, (int)mouseScreen.y - 4, LIGHTGRAY);
    DrawLine((int)mouseScreen.x, (int)mouseScreen.y + 4, (int)mouseScreen.x, (int)mouseScreen.y + 12, LIGHTGRAY);

    // HUD
    DrawRectangle(10, 10, 280, 90, Color{ 0, 0, 0, 180 });
    DrawText(TextFormat("HP: %.0f / %.0f", s->player.hp, s->player.maxHp), 20, 20, 20, RAYWHITE);
    DrawRectangle(20, 44, 240, 12, Color{ 60, 0, 0, 255 });
    DrawRectangle(20, 44, (int)(240 * Clamp(s->player.hp / s->player.maxHp, 0.0f, 1.0f)), 12, Color{ 220, 50, 50, 255 });
    DrawText(TextFormat("[%d] %s", s->player.weaponIndex + 1, wp->name), 20, 62, 18, wp->color);

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

    DrawText("WASD move | J/LMB attack | 1-4 weapon", 10, SCREEN_H - 24, 16, LIGHTGRAY);

    if (s->player.hp <= 0) {
        DrawText("DEAD", SCREEN_W / 2 - 50, SCREEN_H / 2 - 20, 40, RED);
    }

    BackgroundDrawWarning();
    DrawFPS(SCREEN_W - 90, 10);
    EndDrawing();
}