// entities/player.h
#pragma once
#include "core/types.h"

void PlayerInit(Player* p, Vector2 startPos);
void PlayerUpdate(Player* p, const InputState* in, float dt);