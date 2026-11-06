#include "camera.h"
#include "core/config.h"
#include "core/utils.h"
#include "raymath.h"
#include <math.h>

void CameraInit(Camera2D* cam, Vector2 target) {
    cam->offset = Vector2{ SCREEN_W * 0.5f, SCREEN_H * 0.5f };
    cam->target = target;
    cam->rotation = 0;
    cam->zoom = 1.0f;
}

static Vector2 ClampCameraToRoom(Vector2 desired, int roomIdx, const World* w) {
    if (roomIdx < 0) return desired;
    Rectangle b = w->rooms[roomIdx].bounds;
    float hw = SCREEN_W * 0.5f, hh = SCREEN_H * 0.5f;
    Vector2 r;
    if (b.width <= 2 * hw) r.x = b.x + b.width * 0.5f;
    else r.x = Clamp(desired.x, b.x + hw, b.x + b.width - hw);
    if (b.height <= 2 * hh) r.y = b.y + b.height * 0.5f;
    else r.y = Clamp(desired.y, b.y + hh, b.y + b.height - hh);
    return r;
}

void CameraUpdate(Camera2D* cam, const Player* p, const World* w, float dt) {
    Vector2 desired = { p->position.x + p->facing.x * CAM_LOOKAHEAD, p->position.y + p->facing.y * CAM_LOOKAHEAD };
    Vector2 clamped = ClampCameraToRoom(desired, w->currentRoom, w);
    float rate = (w->camTransitionTime > 0) ? CAM_TRANS_LERP : CAM_LERP;
    cam->target.x = LerpExp(cam->target.x, clamped.x, rate, dt);
    cam->target.y = LerpExp(cam->target.y, clamped.y, rate, dt);
}