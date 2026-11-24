// systems/save.cpp
#include "save.h"
#include "core/config.h"
#include "data/weapons.h"
#include <fstream>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <ctime>

static const uint32_t SAVE_MAGIC = 0x56415341;   // "ASAV"
static const uint32_t SAVE_VERSION_V1 = 1;       // format lama (savegame.dat), tanpa metadata
static const uint32_t SAVE_VERSION = 2;          // + waktu simpan & waktu bermain

// Isi inti save (layout sama persis dengan format versi 1).
struct SaveDataV1 {
    uint32_t magic;
    uint32_t version;
    uint32_t size;

    Vector2  playerPos;
    float    playerHp, playerMaxHp;
    int      weaponIndex;
    int      skill;

    int      currentRoom;
    bool     roomVisited[MAX_ROOMS];

    Enemy    enemies[MAX_ENEMIES];
    bool     npcTalked[MAX_NPCS];
};

struct SaveMeta {
    int64_t  savedAt;      // time(nullptr) saat menyimpan
    float    playTime;     // detik
    uint32_t reserved[4];
};

struct SaveDataV2 {
    SaveDataV1 core;       // core.version = 2, core.size = sizeof(SaveDataV2)
    SaveMeta   meta;
};

static bool SlotValid(int slot) { return slot >= 0 && slot < SAVE_SLOT_COUNT; }

static void SlotPath(int slot, char* out, size_t outSize)
{
    snprintf(out, outSize, SAVE_SLOT_PATH_FMT, slot + 1);
}

static bool FileExistsPath(const char* path)
{
    std::ifstream f(path, std::ios::binary);
    return f.good();
}

// Membaca file save versi 1 atau 2. Untuk versi 1, metadata diisi dari waktu file.
static bool ReadSaveFile(const char* path, SaveDataV1* core, SaveMeta* meta)
{
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;

    f.read(reinterpret_cast<char*>(core), sizeof(*core));
    if (!f || f.gcount() != (std::streamsize)sizeof(*core)) return false;
    if (core->magic != SAVE_MAGIC) return false;

    memset(meta, 0, sizeof(*meta));
    if (core->version == SAVE_VERSION_V1 && core->size == sizeof(SaveDataV1)) {
        meta->savedAt = (int64_t)GetFileModTime(path);
    }
    else if (core->version == SAVE_VERSION && core->size == sizeof(SaveDataV2)) {
        f.read(reinterpret_cast<char*>(meta), sizeof(*meta));
        if (!f || f.gcount() != (std::streamsize)sizeof(*meta)) return false;
    }
    else return false;

    if (core->currentRoom < 0 || core->currentRoom >= MAX_ROOMS) return false;
    return true;
}

bool SaveSlotExists(int slot)
{
    if (!SlotValid(slot)) return false;
    char path[64];
    SlotPath(slot, path, sizeof(path));
    return FileExistsPath(path);
}

bool SaveSlotWrite(int slot, const GameState* s)
{
    if (!SlotValid(slot)) return false;

    SaveDataV2 d;
    memset(&d, 0, sizeof(d));
    d.core.magic = SAVE_MAGIC;
    d.core.version = SAVE_VERSION;
    d.core.size = (uint32_t)sizeof(SaveDataV2);

    d.core.playerPos = s->player.position;
    d.core.playerHp = s->player.hp;
    d.core.playerMaxHp = s->player.maxHp;
    d.core.weaponIndex = s->player.weaponIndex;
    d.core.skill = s->player.skill;

    d.core.currentRoom = s->world.currentRoom;
    memcpy(d.core.roomVisited, s->world.roomVisited, sizeof(d.core.roomVisited));
    memcpy(d.core.enemies, s->enemies, sizeof(d.core.enemies));
    for (int i = 0; i < s->npcCount && i < MAX_NPCS; i++) d.core.npcTalked[i] = s->npcs[i].talked;

    d.meta.savedAt = (int64_t)time(nullptr);
    d.meta.playTime = s->playTime;

    char path[64];
    SlotPath(slot, path, sizeof(path));
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f) return false;
    f.write(reinterpret_cast<const char*>(&d), sizeof(d));
    return f.good();
}

bool SaveSlotLoad(int slot, GameState* s)
{
    if (!SlotValid(slot)) return false;
    char path[64];
    SlotPath(slot, path, sizeof(path));

    SaveDataV1 d;
    SaveMeta meta;
    if (!ReadSaveFile(path, &d, &meta)) return false;
    if (d.currentRoom >= s->world.roomCount) return false;

    s->player.position = d.playerPos;
    s->player.maxHp = d.playerMaxHp > 0 ? d.playerMaxHp : 100.0f;
    s->player.hp = d.playerHp > 0 ? d.playerHp : s->player.maxHp;
    s->player.weaponIndex = (d.weaponIndex >= 0 && d.weaponIndex < WEAPON_COUNT) ? d.weaponIndex : 0;
    s->player.skill = d.skill;

    s->world.currentRoom = d.currentRoom;
    memcpy(s->world.roomVisited, d.roomVisited, sizeof(d.roomVisited));
    memcpy(s->enemies, d.enemies, sizeof(d.enemies));
    for (int i = 0; i < MAX_ENEMIES; i++) s->enemies[i].hitFlash = 0.0f;
    for (int i = 0; i < s->npcCount && i < MAX_NPCS; i++) s->npcs[i].talked = d.npcTalked[i];

    s->playTime = meta.playTime > 0 ? meta.playTime : 0.0f;
    s->world.transitionCooldown = 0.45f;
    s->world.roomTitleTimer = 2.0f;
    return true;
}

bool SaveSlotDelete(int slot)
{
    if (!SlotValid(slot)) return false;
    char path[64];
    SlotPath(slot, path, sizeof(path));
    if (!FileExistsPath(path)) return true;
    return remove(path) == 0;
}

void SaveSlotPeek(int slot, SaveSlotInfo* out)
{
    memset(out, 0, sizeof(*out));
    if (!SlotValid(slot)) return;
    char path[64];
    SlotPath(slot, path, sizeof(path));
    out->exists = FileExistsPath(path);
    if (!out->exists) return;

    SaveDataV1 d;
    SaveMeta meta;
    if (!ReadSaveFile(path, &d, &meta)) return;

    out->valid = true;
    out->room = d.currentRoom;
    out->hp = d.playerHp;
    out->maxHp = d.playerMaxHp;
    out->weaponIndex = d.weaponIndex;
    out->savedAt = (long long)meta.savedAt;
    out->playTime = meta.playTime;
}

void SaveMigrateLegacy(void)
{
    if (!FileExistsPath(SAVE_FILE_PATH) || SaveSlotExists(0)) return;
    char path[64];
    SlotPath(0, path, sizeof(path));
    if (rename(SAVE_FILE_PATH, path) == 0)
        TraceLog(LOG_INFO, "SAVE: %s dipindahkan ke %s", SAVE_FILE_PATH, path);
}
