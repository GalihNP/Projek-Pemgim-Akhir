// world/background.h
// Background lingkungan pixel art "Adventure World: Arunika".
// Ground dibangun per piksel dari patch tileset (seamless), objek
// lingkungan diekstrak dari panel atlas lalu disusun natural per ruangan.
#pragma once
#include "core/types.h"

// Panggil SETELAH WorldInit() (butuh daftar dinding ruangan).
// Mengembalikan false jika tileset.png tidak ditemukan.
bool BackgroundInit(const World* w, Vector2 playerSpawn);
void BackgroundUnload(void);

// Di dalam BeginMode2D:
//   1. BackgroundDrawGround   -> ground + tembok + bayangan objek
//   2. BackgroundDrawDecor(y, false) -> objek di belakang entitas (alas <= y)
//   3. ... musuh, player ...
//   4. BackgroundDrawDecor(y, true)  -> objek di depan entitas (alas > y)
void BackgroundDrawGround(const World* w);
void BackgroundDrawDecor(float splitY, bool front);

// Di luar BeginMode2D (layar): pesan jika aset gagal dimuat.
void BackgroundDrawWarning(void);

// Jenis terrain di posisi dunia (untuk logika gameplay lanjutan).
enum TerrainType { GRASS, GRASS_ACCENT, FOREST, ROAD, SAND, WATER, DIRT, CITY, RUIN, ASPHALT, WALLSTONE };
int BackgroundTerrainAt(Vector2 worldPos);
