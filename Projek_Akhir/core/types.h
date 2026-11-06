// core/types.h
#pragma once
#include "raylib.h"
#include "config.h"

typedef struct {
    Rectangle bounds;
    Color     floor;
    const char* name;
} Room;

typedef struct {
    Room rooms[MAX_ROOMS];
    int  roomCount;
    int  currentRoom;
    float transitionCooldown;
    float camTransitionTime;
    float roomTitleTimer;
} World;

typedef struct {
    Vector2 position, facing, velocity;
    float   hp, maxHp;
    int     weaponIndex;
    int     skill;            // <--- TAMBAHKAN INI
    float   attackCd, swingTime, iframes;
    bool    swingHitDone;
} Player;

typedef struct {
    Vector2 position, velocity;
    float   radius, hp, maxHp;
    bool    alive;
    float   hitFlash, attackCd;
} Enemy;

typedef struct {
    Vector2  move;
    Vector2  aim;
    bool     attackPressed;
    bool     attackHeld;
    int      weaponSwitch;
} InputState;

typedef struct {
    World    world;
    Player   player;
    Enemy    enemies[MAX_ENEMIES];
    Camera2D cam;
} GameState;