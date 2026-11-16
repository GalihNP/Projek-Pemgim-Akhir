#include "enemy.h"
#include "core/config.h"
#include "world/collision.h"
#include "world/world.h"
#include "world/background.h"
#include "raymath.h"
#include <math.h>
#include <string.h>

// Jumlah musuh yang muncul di tiap ruangan (index = nomor ruangan).
// Ruang 0 (Desa Pesisir) = desa aman tempat para NPC, jadi 0 musuh.
static const int ENEMIES_PER_ROOM[] = { 0, 6, 8, 10 };
static const int ENEMIES_PER_ROOM_DEFAULT = 8;   // untuk ruangan di luar tabel

static int EnemyWaveSize(int room)
{
    int n = (int)(sizeof(ENEMIES_PER_ROOM) / sizeof(ENEMIES_PER_ROOM[0]));
    return (room >= 0 && room < n) ? ENEMIES_PER_ROOM[room] : ENEMIES_PER_ROOM_DEFAULT;
}

void EnemyClearAll(Enemy* enemies, int count) {
    for (int i = 0; i < count; i++) {
        memset(&enemies[i], 0, sizeof(Enemy));
        enemies[i].alive = false;
    }
}

static bool EnemySpotFree(Vector2 p, float r, const Enemy* enemies, int count)
{
    for (int i = 0; i < GetWallCount(); i++)
        if (CircleVsAABB(p, r + 6.0f, GetWallRect(i), nullptr)) return false;

    int t = BackgroundTerrainAt(p);
    if (t == WATER || t == WALLSTONE) return false;

    for (int i = 0; i < count; i++)
        if (enemies[i].alive && Vector2Distance(p, enemies[i].position) < r * 2.0f + 20.0f) return false;
    return true;
}

int EnemySpawnInRoom(Enemy* enemies, int count, const World* w, int room, Vector2 playerPos) {
    if (room < 0 || room >= w->roomCount) return 0;

    Rectangle b = w->rooms[room].bounds;
    const float inset = WALL_T + 50.0f;
    const float radius = 16.0f;
    int want = EnemyWaveSize(room);
    int spawned = 0;

    for (int k = 0; k < want; k++) {
        // cari slot kosong
        int slot = -1;
        for (int i = 0; i < count; i++) if (!enemies[i].alive) { slot = i; break; }
        if (slot < 0) break;   // MAX_ENEMIES penuh

        // cari posisi acak yang valid; jika sulit, longgarkan jarak minimal dari player
        Vector2 pos = { 0, 0 };
        bool found = false;
        for (int attempt = 0; attempt < 120 && !found; attempt++) {
            float minDist = (attempt < 80) ? ENEMY_SPAWN_MIN_DIST : ENEMY_SPAWN_MIN_DIST * 0.5f;
            pos.x = (float)GetRandomValue((int)(b.x + inset), (int)(b.x + b.width - inset));
            pos.y = (float)GetRandomValue((int)(b.y + inset), (int)(b.y + b.height - inset));
            if (Vector2Distance(pos, playerPos) < minDist) continue;
            if (!EnemySpotFree(pos, radius, enemies, count)) continue;
            found = true;
        }
        if (!found) continue;

        Enemy* e = &enemies[slot];
        memset(e, 0, sizeof(*e));
        e->position = pos;
        e->velocity = Vector2{ 0, 0 };
        e->radius = radius;
        e->hp = 60; e->maxHp = 60;
        e->alive = true;
        e->hitFlash = 0; e->attackCd = 0;
        e->spawnTimer = ENEMY_SPAWN_DELAY + k * 0.12f;   // muncul bergiliran
        spawned++;
    }
    return spawned;
}

void EnemyUpdateAll(Enemy* enemies, int count, Player* p, const World* w, float dt) {
    for (int i = 0; i < count; i++) {
        Enemy* e = &enemies[i];
        if (!e->alive) continue;

        // animasi muncul: belum bisa bergerak / menyerang
        if (e->spawnTimer > 0) {
            e->spawnTimer -= dt;
            continue;
        }

        // musuh di ruangan lain diam (tidak mengejar menembus dinding)
        if (WorldRoomAt(w, e->position) != w->currentRoom) {
            e->velocity = Vector2{ 0, 0 };
            continue;
        }

        Vector2 to = Vector2Subtract(p->position, e->position);
        float dist = Vector2Length(to);
        Vector2 nd = (dist > 0.001f) ? Vector2Scale(to, 1.0f / dist) : Vector2{ 0, 0 };

        e->velocity = Vector2Add(e->velocity, Vector2Scale(nd, 800.0f * dt));
        e->velocity = Vector2Scale(e->velocity, 1.0f - Clamp(4.0f * dt, 0.0f, 0.9f));
        e->position = Vector2Add(e->position, Vector2Scale(e->velocity, dt));
        ResolveCircleWalls(&e->position, e->radius);

        if (e->hitFlash > 0) e->hitFlash -= dt;
        if (e->attackCd > 0) e->attackCd -= dt;

        if (dist < e->radius + PLAYER_RADIUS + 4 && e->attackCd <= 0 && p->iframes <= 0 && p->hp > 0) {
            p->hp -= 8;
            p->iframes = PLAYER_IFRAMES;
            e->attackCd = 0.8f;
            p->position = Vector2Add(p->position, Vector2Scale(nd, -30));
        }
    }
}

int EnemyCountInRoom(const Enemy* enemies, int count, const World* w, int room) {
    int n = 0;
    for (int i = 0; i < count; i++)
        if (enemies[i].alive && WorldRoomAt(w, enemies[i].position) == room) n++;
    return n;
}
