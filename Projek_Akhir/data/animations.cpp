#include "animations.h"
#include "data/weapons.h"

// Koordinat frame diukur dari sprite sheet asli (lihat README bagian Animasi).
// anchorX = titik tengah kepala di dalam frame, supaya posisi karakter tidak
// "melompat" ketika berganti frame walau lebar frame berbeda-beda.
// Urutan mengikuti WEAPONS[]: 0 = Sword, 1 = Pistol, 2 = Keris.
const WeaponAnim WEAPON_ANIMS[] = {
    // ---------------- Sword (Pemgim1.png) ----------------
    {
        "../Asset/Pemgim1.png",
        // walk: baris atas, 3 frame (pedang diturunkan)
        { {
            { { 257.0f, 1433.0f, 153.0f, 207.0f }, 74.0f, 0.0f },
            { { 439.0f, 1433.0f, 154.0f, 208.0f }, 74.0f, 0.0f },
            { { 622.0f, 1433.0f, 154.0f, 207.0f }, 74.0f, 0.0f },
        }, 3 },
        // attack: kuda-kuda -> ayunan ke atas -> tebasan penuh (efek biru)
        { {
            { { 250.0f, 1742.0f, 148.0f, 206.0f }, 75.0f, 0.07f },
            { { 439.0f, 1741.0f, 164.0f, 207.0f }, 75.0f, 0.07f },
            { { 634.0f, 1737.0f, 187.0f, 212.0f }, 73.0f, 0.12f },
        }, 3 },
        2,                       // damage saat tebasan penuh tampil
        { -1.0f, -1.0f },
    },
    // ---------------- Pistol (Pemgim2.png) ----------------
    {
        "../Asset/Pemgim2.png",
        // walk: baris atas, 4 frame (pistol dipegang di depan dada)
        { {
            { { 270.0f, 346.0f, 139.0f, 206.0f }, 75.0f, 0.0f },
            { { 444.0f, 350.0f, 139.0f, 208.0f }, 76.0f, 0.0f },
            { { 612.0f, 351.0f, 139.0f, 206.0f }, 75.0f, 0.0f },
            { { 773.0f, 352.0f, 139.0f, 206.0f }, 76.0f, 0.0f },
        }, 4 },
        // attack: bidik -> tembak (muzzle flash) -> kembali membidik (recoil)
        { {
            { { 425.0f, 617.0f, 147.0f, 208.0f }, 83.0f, 0.06f },
            { { 601.0f, 620.0f, 207.0f, 206.0f }, 73.0f, 0.08f },
            { { 425.0f, 617.0f, 147.0f, 208.0f }, 83.0f, 0.10f },
        }, 3 },
        1,                       // damage tepat saat muzzle flash
        { 122.0f, 100.0f },      // pangkal kilatan moncong pada frame tembak
    },
    // ---------------- Keris (Pemgim3.png) ----------------
    {
        "../Asset/Pemgim3.png",
        // walk: baris atas, 4 frame (keris digenggam terbalik)
        { {
            { { 316.0f, 903.0f, 137.0f, 205.0f }, 75.0f, 0.0f },
            { { 470.0f, 904.0f, 139.0f, 205.0f }, 76.0f, 0.0f },
            { { 620.0f, 907.0f, 138.0f, 206.0f }, 75.0f, 0.0f },
            { { 773.0f, 907.0f, 139.0f, 205.0f }, 76.0f, 0.0f },
        }, 4 },
        // attack: genggam -> tarik ke belakang -> menerjang menusuk (efek sabetan)
        { {
            { { 248.0f, 1149.0f, 138.0f, 207.0f }, 75.0f, 0.06f },
            { { 432.0f, 1157.0f, 138.0f, 207.0f }, 75.0f, 0.08f },
            // Frame terjangan: badan condong ke depan, poros diambil di tengah
            // antara kepala dan kaki belakang agar terjangan terlihat maju.
            { { 600.0f, 1162.0f, 224.0f, 196.0f }, 92.0f, 0.13f },
        }, 3 },
        2,                       // damage saat tusukan menjangkau ke depan
        { -1.0f, -1.0f },
    },
};

const int WEAPON_ANIM_COUNT = sizeof(WEAPON_ANIMS) / sizeof(WeaponAnim);

static const AnimClip* AttackClip(int weapon) {
    if (weapon < 0 || weapon >= WEAPON_ANIM_COUNT) return 0;
    return &WEAPON_ANIMS[weapon].attack;
}

float AnimAttackDuration(int weapon) {
    const AnimClip* c = AttackClip(weapon);
    if (!c) return SWING_DURATION;
    float t = 0.0f;
    for (int i = 0; i < c->count; i++) t += c->frames[i].duration;
    return t;
}

float AnimAttackHitTime(int weapon) {
    const AnimClip* c = AttackClip(weapon);
    if (!c) return SWING_DURATION * 0.5f;
    float t = 0.0f;
    for (int i = 0; i < WEAPON_ANIMS[weapon].hitFrame && i < c->count; i++)
        t += c->frames[i].duration;
    return t;
}

int AnimAttackFrameAt(int weapon, float elapsed) {
    const AnimClip* c = AttackClip(weapon);
    if (!c || c->count <= 0) return 0;
    float t = 0.0f;
    for (int i = 0; i < c->count; i++) {
        t += c->frames[i].duration;
        if (elapsed < t) return i;
    }
    return c->count - 1;
}
