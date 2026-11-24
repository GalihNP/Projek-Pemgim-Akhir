#include "combat.h"
#include "data/weapons.h"
#include "raymath.h"
#include <math.h>

bool InArc(Vector2 origin, Vector2 dir, float range, float arc,
    Vector2 target, float targetR) {
    Vector2 to = Vector2Subtract(target, origin);
    float d = Vector2Length(to);
    if (d > range + targetR) return false;
    if (d < 0.001f) return true;
    Vector2 nd = Vector2Scale(to, 1.0f / d);
    float dot = Clamp(Vector2DotProduct(nd, dir), -1.0f, 1.0f);
    return acosf(dot) <= arc * 0.5f;
}

void DoAttack(Player* p, Enemy* enemies, int enemyCount) {
    // Pakai senjata & arah yang dikunci saat animasi serangan dimulai, sehingga
    // damage selalu cocok dengan animasi yang sedang tampil.
    if (p->attackWeapon < 0 || p->attackWeapon >= WEAPON_COUNT) return;
    const Weapon* w = &WEAPONS[p->attackWeapon];
    float mul = SkillDamageMul(p->skill);

    for (int i = 0; i < enemyCount; i++) {
        Enemy* e = &enemies[i];
        if (!e->alive || e->spawnTimer > 0) continue;   // musuh yang sedang muncul kebal
        if (!InArc(p->position, p->attackDir, w->range, w->arc, e->position, e->radius))
            continue;

        Vector2 to = Vector2Subtract(e->position, p->position);
        float d = Vector2Length(to);
        if (d < 0.001f) d = 0.001f;
        Vector2 nd = Vector2Scale(to, 1.0f / d);

        e->hp -= w->damage * mul;
        e->hitFlash = 0.15f;
        e->velocity = Vector2Add(e->velocity, Vector2Scale(nd, w->knockback));
        if (e->hp <= 0.0f) e->alive = false;
    }
}