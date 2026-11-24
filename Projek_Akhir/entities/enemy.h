#pragma once
#include "core/types.h"

// Kosongkan semua slot musuh (tidak ada musuh di awal permainan).
void EnemyClearAll(Enemy* enemies, int count);

// Munculkan gelombang musuh di dalam ruangan `room`. Dipanggil sekali saat
// player PERTAMA KALI masuk ruangan. Mengembalikan jumlah musuh yang muncul.
int  EnemySpawnInRoom(Enemy* enemies, int count, const World* w, int room, Vector2 playerPos);

// Hanya musuh yang berada di ruangan player yang bergerak & menyerang.
void EnemyUpdateAll(Enemy* enemies, int count, Player* player, const World* w, float dt);

// Jumlah musuh hidup di dalam ruangan tertentu (untuk HUD).
int  EnemyCountInRoom(const Enemy* enemies, int count, const World* w, int room);

// State animasi visual musuh (disimpan terpisah agar format save tetap kompatibel).
bool EnemyIsAttacking(int index);
float EnemyAnimTime(int index);
float EnemyAttackAnimTime(int index);
float EnemyFacingX(int index);
