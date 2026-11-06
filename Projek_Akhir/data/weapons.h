#pragma once
#include "raylib.h"
#include "core/config.h"

typedef struct {
    const char* name;
    float range;
    float arc;
    float damage;
    float cooldown;
    float knockback;
    Color color;
} Weapon;

extern const Weapon WEAPONS[];
extern const int    WEAPON_COUNT;

float SkillDamageMul(int skill);
float SkillCooldownMul(int skill);
