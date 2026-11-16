// systems/save.cpp
#include "save.h"
#include "core/config.h"
#include "data/weapons.h"
#include <fstream>
#include <cstdint>
#include <cstring>

static const uint32_t SAVE_MAGIC = 0x56415341;   // "ASAV"
static const uint32_t SAVE_VERSION = 1;

struct SaveData {
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

bool SaveGameExists(void)
{
    std::ifstream f(SAVE_FILE_PATH, std::ios::binary);
    return f.good();
}

bool SaveGameWrite(const GameState* s)
{
    SaveData d;
    memset(&d, 0, sizeof(d));
    d.magic = SAVE_MAGIC;
    d.version = SAVE_VERSION;
    d.size = (uint32_t)sizeof(SaveData);

    d.playerPos = s->player.position;
    d.playerHp = s->player.hp;
    d.playerMaxHp = s->player.maxHp;
    d.weaponIndex = s->player.weaponIndex;
    d.skill = s->player.skill;

    d.currentRoom = s->world.currentRoom;
    memcpy(d.roomVisited, s->world.roomVisited, sizeof(d.roomVisited));
    memcpy(d.enemies, s->enemies, sizeof(d.enemies));
    for (int i = 0; i < s->npcCount && i < MAX_NPCS; i++) d.npcTalked[i] = s->npcs[i].talked;

    std::ofstream f(SAVE_FILE_PATH, std::ios::binary | std::ios::trunc);
    if (!f) return false;
    f.write(reinterpret_cast<const char*>(&d), sizeof(d));
    return f.good();
}

bool SaveGameLoad(GameState* s)
{
    std::ifstream f(SAVE_FILE_PATH, std::ios::binary);
    if (!f) return false;

    SaveData d;
    f.read(reinterpret_cast<char*>(&d), sizeof(d));
    if (!f || f.gcount() != (std::streamsize)sizeof(d)) return false;
    if (d.magic != SAVE_MAGIC || d.version != SAVE_VERSION || d.size != sizeof(SaveData)) return false;
    if (d.currentRoom < 0 || d.currentRoom >= s->world.roomCount) return false;

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

    s->world.transitionCooldown = 0.45f;
    s->world.roomTitleTimer = 2.0f;
    return true;
}
