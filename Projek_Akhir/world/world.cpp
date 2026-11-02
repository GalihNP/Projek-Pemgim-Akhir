#include "world.h"
#include "collision.h"
#include <string.h>

void WorldInit(World* w) {
    memset(w, 0, sizeof(*w));
    ClearWalls();

    static const char* names[] = { "Hall", "Library", "Crypt", "Sanctum" };
    static const Color floors[] = {
        { 40, 45, 58, 255 }, { 38, 46, 50, 255 },
        { 44, 40, 52, 255 }, { 36, 44, 54, 255 },
    };

    w->roomCount = ROOM_COLS * ROOM_ROWS;
    for (int i = 0; i < w->roomCount; i++) {
        int rx = i % ROOM_COLS, ry = i / ROOM_COLS;
        w->rooms[i].bounds = Rectangle{ (float)(rx * ROOM_W), (float)(ry * ROOM_H), (float)ROOM_W, (float)ROOM_H };
        w->rooms[i].floor = floors[i];
        w->rooms[i].name = names[i];

        Rectangle b = w->rooms[i].bounds;
        float cx = b.x + b.width * 0.5f;
        float cy = b.y + b.height * 0.5f;

        // Utara
        if (ry == 0) AddWall(b.x, b.y, b.width, WALL_T);
        else {
            AddWall(b.x, b.y, cx - DOOR_SIZE / 2 - b.x, WALL_T);
            AddWall(cx + DOOR_SIZE / 2, b.y, b.x + b.width - (cx + DOOR_SIZE / 2), WALL_T);
        }
        // Selatan
        if (ry == ROOM_ROWS - 1) AddWall(b.x, b.y + b.height - WALL_T, b.width, WALL_T);
        else {
            AddWall(b.x, b.y + b.height - WALL_T, cx - DOOR_SIZE / 2 - b.x, WALL_T);
            AddWall(cx + DOOR_SIZE / 2, b.y + b.height - WALL_T, b.x + b.width - (cx + DOOR_SIZE / 2), WALL_T);
        }
        // Barat
        if (rx == 0) AddWall(b.x, b.y, WALL_T, b.height);
        else {
            AddWall(b.x, b.y, WALL_T, cy - DOOR_SIZE / 2 - b.y);
            AddWall(b.x, cy + DOOR_SIZE / 2, WALL_T, b.y + b.height - (cy + DOOR_SIZE / 2));
        }
        // Timur
        if (rx == ROOM_COLS - 1) AddWall(b.x + b.width - WALL_T, b.y, WALL_T, b.height);
        else {
            AddWall(b.x + b.width - WALL_T, b.y, WALL_T, cy - DOOR_SIZE / 2 - b.y);
            AddWall(b.x + b.width - WALL_T, cy + DOOR_SIZE / 2, WALL_T, b.y + b.height - (cy + DOOR_SIZE / 2));
        }
    }
    w->currentRoom = 0;
}

int WorldRoomAt(World* w, Vector2 p) {
    for (int i = 0; i < w->roomCount; i++)
        if (CheckCollisionPointRec(p, w->rooms[i].bounds)) return i;
    return -1;
}

void WorldUpdateRoom(World* w, Vector2 playerPos, float dt) {
    if (w->transitionCooldown > 0) w->transitionCooldown -= dt;
    if (w->camTransitionTime > 0) w->camTransitionTime -= dt;
    if (w->roomTitleTimer > 0) w->roomTitleTimer -= dt;

    int detected = WorldRoomAt(w, playerPos);
    if (detected >= 0 && detected != w->currentRoom && w->transitionCooldown <= 0) {
        w->currentRoom = detected;
        w->transitionCooldown = 0.45f;
        w->camTransitionTime = 0.75f;
        w->roomTitleTimer = 2.0f;
    }
}

void WorldUnload(World* w) {}