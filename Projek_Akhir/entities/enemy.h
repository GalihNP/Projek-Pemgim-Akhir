#pragma once
#include "core/types.h"

void EnemyInitAll(Enemy* enemies, int count, Vector2 nearPos);
void EnemyUpdateAll(Enemy* enemies, int count, Player* player, float dt);
