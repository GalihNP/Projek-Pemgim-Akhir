#include "player.h"
#include "core/config.h"
#include "core/utils.h"
#include "world/collision.h"
#include "data/weapons.h"
#include "data/animations.h"
#include "raymath.h"
#include <math.h>

void PlayerInit(Player* p, Vector2 startPos) {
    p->position = startPos;
    p->facing = Vector2{ 1.0f, 0.0f };
    p->velocity = Vector2{ 0.0f, 0.0f };
    p->hp = 100.0f;
    p->maxHp = 100.0f;
    p->weaponIndex = 0;
    p->skill = 50;
    p->attackCd = 0.0f;
    p->swingTime = 0.0f;
    p->iframes = 0.0f;
    p->swingHitDone = true;
    p->swingDuration = SWING_DURATION;
    p->hitTime = SWING_DURATION * 0.5f;
    p->attackWeapon = 0;
    p->attackDir = p->facing;
    p->shotFx = 0.0f;
    p->moving = false;
    p->walkTime = 0.0f;
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

    /* --- Animasi jalan: hanya saat benar-benar bergerak --- */
    p->moving = (len > 0) && p->hp > 0;
    if (p->moving) p->walkTime += dt;
    else           p->walkTime = 0.0f;   // kembali ke pose idle

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
        // Animasi serangan sesuai senjata; damage diberikan di frame "hit"
        // (lihat UpdateGameplay di main.cpp), sehingga visual & damage sinkron.
        p->attackWeapon = p->weaponIndex;
        p->attackDir = p->facing;
        p->swingDuration = AnimAttackDuration(p->weaponIndex);
        p->hitTime = AnimAttackHitTime(p->weaponIndex);
        p->swingTime = p->swingDuration;
        p->swingHitDone = false;
    }

    if (p->attackCd > 0) p->attackCd -= dt;
    if (p->swingTime > 0) {
        p->swingTime -= dt;
        if (p->swingTime < 0) p->swingTime = 0;   // selesai -> kembali idle / jalan
    }
    if (p->shotFx > 0) p->shotFx -= dt;
    if (p->iframes > 0) p->iframes -= dt;
	if (p->hp < 0) p->hp = 0;
}