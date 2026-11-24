// systems/camera.h
#pragma once
#include "core/types.h"

void CameraInit(Camera2D* cam, Vector2 target);
void CameraUpdate(Camera2D* cam, const Player* p, const World* w, float dt);
