#pragma once
#include "raylib.h"
#include "raymath.h"
#include <math.h>

static inline Vector2 NormalizeOrZero(Vector2 v) {
    float len = Vector2Length(v);
    return (len > 0.0001f) ? Vector2Scale(v, 1.0f / len) : Vector2{ 0, 0 };
}

static inline float LerpExp(float cur, float target, float rate, float dt) {
    return cur + (target - cur) * (1.0f - expf(-rate * dt));
}