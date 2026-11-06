// world/world.h
#pragma once
#include "core/types.h"

void  WorldInit(World* w);
void  WorldUnload(World* w);
int   WorldRoomAt(World* w, Vector2 p);
void  WorldUpdateRoom(World* w, Vector2 playerPos, float dt);