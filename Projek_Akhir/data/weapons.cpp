#include "weapons.h"
#include "raymath.h"

// 1. Tambahkan 'const' di sini agar cocok dengan weapons.h
const Weapon WEAPONS[] = {
    { "Dagger",      70.0f,  100.0f * DEG2RAD, 12.0f, 0.22f,  80.0f, SKYBLUE },
    { "Sword",      110.0f,   90.0f * DEG2RAD, 25.0f, 0.42f, 180.0f, RAYWHITE },
    { "Spear",      170.0f,   50.0f * DEG2RAD, 22.0f, 0.55f, 140.0f, GOLD },
    { "Greatsword", 140.0f,  140.0f * DEG2RAD, 45.0f, 0.85f, 320.0f, ORANGE },
};

// 2. Pastikan pakai sizeof(Weapon), bukan sizeof(WEAPONS[0])
const int WEAPON_COUNT = sizeof(WEAPONS) / sizeof(Weapon);

float SkillDamageMul(int s) { return 1.0f + (s / 100.0f) * 0.5f; }
float SkillCooldownMul(int s) { return 1.0f - (s / 100.0f) * 0.35f; }