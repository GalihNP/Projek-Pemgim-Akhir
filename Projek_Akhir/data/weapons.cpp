#include "weapons.h"
#include "raymath.h"

// Urutan senjata mengikuti tombol keyboard: 1 = Sword, 2 = Pistol, 3 = Keris.
const Weapon WEAPONS[] = {
    { "Sword",  115.0f,  90.0f * DEG2RAD, 25.0f, 0.42f, 180.0f, RAYWHITE },
    { "Pistol", 280.0f,  14.0f * DEG2RAD, 22.0f, 0.55f, 140.0f, GOLD },
    { "Keris",   85.0f,  65.0f * DEG2RAD, 30.0f, 0.48f, 210.0f, SKYBLUE },
};

const int WEAPON_COUNT = sizeof(WEAPONS) / sizeof(Weapon);

float SkillDamageMul(int s) { return 1.0f + (s / 100.0f) * 0.5f; }
float SkillCooldownMul(int s) { return 1.0f - (s / 100.0f) * 0.35f; }
