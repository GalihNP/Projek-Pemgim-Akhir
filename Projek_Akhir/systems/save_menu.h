// systems/save_menu.h
// Layar pemilihan 5 slot save.
//  - Mode LOAD (dari Main Menu -> Play): pilih slot berisi untuk melanjutkan,
//    atau slot kosong untuk memulai game baru di slot itu.
//  - Mode SAVE (dari menu pause -> Save): pilih slot tujuan penyimpanan.
// Di kedua mode, slot yang berisi bisa dihapus (dengan konfirmasi).
#pragma once
#include "core/types.h"
#include "systems/save.h"

typedef enum {
    SLOTMENU_LOAD = 0,
    SLOTMENU_SAVE
} SlotMenuMode;

typedef enum {
    SLOTACT_NONE = 0,
    SLOTACT_BACK,             // tutup layar
    SLOTACT_NEW_GAME,         // mulai game baru di slot *outSlot
    SLOTACT_LOAD,             // muat slot *outSlot
    SLOTACT_SAVE              // simpan ke slot *outSlot
} SlotMenuAction;

typedef struct {
    SlotMenuMode mode;
    int          selected;     // 0..SAVE_SLOT_COUNT-1 = slot, SAVE_SLOT_COUNT = tombol Kembali
    bool         focusDelete;  // navigasi keyboard: fokus pada tombol Hapus di baris terpilih
    int          activeSlot;   // slot yang sedang dimainkan (-1 jika belum ada)
    SaveSlotInfo info[SAVE_SLOT_COUNT];

    int          confirm;      // jenis dialog konfirmasi yang terbuka (0 = tidak ada)
    int          confirmSlot;
    int          confirmSel;   // 0 = Ya, 1 = Tidak

    char         status[64];
    float        statusTimer;
} SlotMenu;

void           SlotMenuOpen(SlotMenu* m, SlotMenuMode mode, int activeSlot);
void           SlotMenuRefresh(SlotMenu* m);                 // baca ulang info semua slot
void           SlotMenuSetStatus(SlotMenu* m, const char* msg);
SlotMenuAction SlotMenuUpdate(SlotMenu* m, float dt, int* outSlot);
// overlay = true saat dibuka dari dalam game (latar game digelapkan).
void           SlotMenuDraw(const SlotMenu* m, const World* w, bool overlay);
