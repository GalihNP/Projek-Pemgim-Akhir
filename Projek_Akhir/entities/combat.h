#pragma once
#include "core/types.h"

bool InArc(Vector2 origin, Vector2 dir, float range, float arc,
    Vector2 target, float targetR);

void DoAttack(Player* p, Enemy* enemies, int enemyCount);
