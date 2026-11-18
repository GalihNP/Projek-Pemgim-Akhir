// systems/save.h
#pragma once
#include "core/types.h"

// Ada SAVE_SLOT_COUNT (5) slot save. Index slot = 0..4, ditampilkan sebagai "Slot 1..5".
// File disimpan di folder kerja program: save_slot1.dat ... save_slot5.dat
// (lihat SAVE_SLOT_PATH_FMT di core/config.h).

typedef struct {
    bool      exists;         // file ada
    bool      valid;          // file bisa dibaca (bukan rusak / versi tak dikenal)
    int       room;           // ruangan saat disimpan
    float     hp, maxHp;
    int       weaponIndex;
    long long savedAt;        // waktu simpan (detik sejak epoch, waktu lokal saat ditampilkan)
    float     playTime;       // total waktu bermain (detik)
} SaveSlotInfo;

bool SaveSlotExists(int slot);
bool SaveSlotWrite(int slot, const GameState* s);
// Memuat posisi/HP/senjata player, ruangan aktif, ruangan yang sudah dikunjungi,
// musuh, status NPC, dan waktu bermain. False jika file tidak ada / rusak.
bool SaveSlotLoad(int slot, GameState* s);
bool SaveSlotDelete(int slot);
// Baca ringkasan slot untuk ditampilkan di layar pemilihan save.
void SaveSlotPeek(int slot, SaveSlotInfo* out);

// Jika masih ada savegame.dat (format lama, 1 file) dan Slot 1 kosong,
// pindahkan ke Slot 1 supaya progres lama tidak hilang.
void SaveMigrateLegacy(void);
