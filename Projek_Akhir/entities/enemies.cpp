#include "enemy.h"
#include "core/config.h"
#include "world/collision.h"
#include "raymath.h"
#include <math.h>

void EnemyInitAll(Enemy* enemies, int count, Vector2 nearPos) {
    for (int i = 0; i < count; i++) {
        // Mengganti (Enemy){ .position = ... } menjadi assignment biasa
        enemies[i].position.x = nearPos.x + (float)GetRandomValue(-500, 500);
        enemies[i].position.y = nearPos.y + (float)GetRandomValue(-500, 500);
        enemies[i].velocity = Vector2{ 0, 0 };
        enemies[i].radius = 16;
        enemies[i].hp = 60; enemies[i].maxHp = 60;
        enemies[i].alive = true;
        enemies[i].hitFlash = 0; enemies[i].attackCd = 0;
    }
}

void EnemyUpdateAll(Enemy* enemies, int count, Player* p, float dt) {
    for (int i = 0; i < count; i++) {
        Enemy* e = &enemies[i];
        if (!e->alive) continue;

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