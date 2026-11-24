// data/animations.h
// Data animasi karakter yang diambil dari sprite sheet di folder Asset:
//   Pemgim1.png = Sword  (3 frame jalan + 3 frame serangan tebasan)
//   Pemgim2.png = Pistol (4 frame jalan + 2 frame bidik/tembak dengan muzzle flash)
//   Pemgim3.png = Keris  (4 frame jalan + 3 frame serangan tusukan)
// Pose diam (idle) tetap memakai player_weapon_1..3.png seperti sebelumnya.
#pragma once
#include "raylib.h"

#define ANIM_MAX_FRAMES  8

// Satu frame di dalam sprite sheet (koordinat piksel pada file PNG asli).
typedef struct {
    Rectangle src;       // potongan frame pada sheet
    float     anchorX;   // titik tengah kepala, relatif terhadap src.x (dipakai sbg poros)
    float     duration;  // durasi frame (detik) - hanya dipakai animasi serangan
} AnimFrame;

typedef struct {
    AnimFrame frames[ANIM_MAX_FRAMES];
    int       count;
} AnimClip;

// Satu set animasi untuk satu senjata.
typedef struct {
    const char* sheetPath;   // path sprite sheet (relatif ke working directory)
    AnimClip    walk;        // diputar berulang saat bergerak
    AnimClip    attack;      // diputar sekali saat menyerang
    int         hitFrame;    // indeks frame serangan saat damage diberikan
    Vector2     muzzle;      // posisi moncong senjata pada frame hitFrame (lokal src); (-1,-1) = tidak ada
} WeaponAnim;

extern const WeaponAnim WEAPON_ANIMS[];
extern const int        WEAPON_ANIM_COUNT;

#define WALK_ANIM_FPS     8.0f
// Skala gambar: sama dengan sprite idle lama (tinggi 219 px -> 72 px di dunia).
#define CHAR_SPRITE_SCALE (72.0f / 219.0f)

// Total durasi klip serangan (detik).
float AnimAttackDuration(int weapon);
// Waktu (detik sejak serangan dimulai) ketika frame hitFrame mulai tampil.
float AnimAttackHitTime(int weapon);
// Indeks frame serangan untuk waktu `elapsed` (detik sejak serangan dimulai).
int   AnimAttackFrameAt(int weapon, float elapsed);
