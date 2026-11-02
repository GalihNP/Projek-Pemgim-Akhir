#include "player.h"
#include "core/config.h"
#include "core/utils.h"
#include "world/collision.h"
#include "data/weapons.h"
#include "raymath.h"
#include <math.h>

void PlayerInit(Player* p, Vector2 startPos) {
    p->position = startPos;
    p->facing = Vector2{ 1.0f, 0.0f };
    p->velocity = Vector2{ 0.0f, 0.0f };
    p->hp = 100.0f;
    p->maxHp = 100.0f;
    p->weaponIndex = 1;
    p->skill = 50;
    p->attackCd = 0.0f;
    p->swingTime = 0.0f;
    p->iframes = 0.0f;
    p->swingHitDone = true;
}

void PlayerUpdate(Player* p, const InputState* in, float dt) {
    /* --- Movement (Hanya untuk geser posisi) --- */
    Vector2 dir = in->move;
    float len = Vector2Length(dir);
    if (len > 0) {
        dir = Vector2Scale(dir, 1.0f / len);
    }

    p->position.x += dir.x * PLAYER_SPEED * dt;
    p->position.y += dir.y * PLAYER_SPEED * dt;
    ResolveCircleWalls(&p->position, PLAYER_RADIUS);

    /* --- Aiming (Arah hadap ke Mouse) --- */
    // Hitung vektor dari player ke posisi mouse
    Vector2 aimDir = Vector2Subtract(in->aim, p->position);

    // Jika mouse agak jauh dari player, perbarui arah hadap (mencegah jitter)
    if (Vector2Length(aimDir) > 5.0f) {
        p->facing = NormalizeOrZero(aimDir);
    }

    /* --- Weapon Switch --- */
    if (in->weaponSwitch >= 0 && in->weaponSwitch < WEAPON_COUNT)
        p->weaponIndex = in->weaponSwitch;

    /* --- Attack Trigger --- */
    const Weapon* wp = &WEAPONS[p->weaponIndex];
    if (in->attackPressed && p->attackCd <= 0.0f && p->hp > 0) {
        p->attackCd = wp->cooldown * SkillCooldownMul(p->skill);
        p->swingTime = SWING_DURATION;
        p->swingHitDone = false;
    }

    if (p->attackCd > 0) p->attackCd -= dt;
    if (p->swingTime > 0) p->swingTime -= dt;
    if (p->iframes > 0) p->iframes -= dt;
}