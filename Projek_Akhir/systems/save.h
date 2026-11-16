// systems/save.h
#pragma once
#include "core/types.h"

// File disimpan di folder kerja program (SAVE_FILE_PATH di core/config.h).
bool SaveGameExists(void);
bool SaveGameWrite(const GameState* s);
// Memuat posisi/HP/senjata player, ruangan aktif, ruangan yang sudah dikunjungi,
// dan semua musuh. Mengembalikan false jika file tidak ada / rusak / versi lama.
bool SaveGameLoad(GameState* s);
