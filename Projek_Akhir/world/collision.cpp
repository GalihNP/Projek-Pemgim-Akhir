#include "collision.h"
#include "core/config.h"
#include "raymath.h"
#include <math.h>

static Rectangle s_walls[MAX_WALLS];
static int s_wallCount = 0;

void ClearWalls(void) { s_wallCount = 0; }

void AddWall(float x, float y, float w, float h) {
    if (s_wallCount >= MAX_WALLS) return;
    s_walls[s_wallCount++] = Rectangle{ x, y, w, h };
}

int GetWallCount(void) { return s_wallCount; }
Rectangle GetWallRect(int index) { return s_walls[index]; }

void ResolveCircleWalls(Vector2* pos, float radius) {
    for (int pass = 0; pass < 2; pass++) {
        for (int i = 0; i < s_wallCount; i++) {
            Rectangle w = s_walls[i];
            float cx = Clamp(pos->x, w.x, w.x + w.width);
            float cy = Clamp(pos->y, w.y, w.y + w.height);
            float dx = pos->x - cx;
            float dy = pos->y - cy;
            float d2 = dx * dx + dy * dy;

            if (d2 < radius * radius && d2 > 0.0001f) {
                float d = sqrtf(d2);
                float push = radius - d;
                pos->x += (dx / d) * push;
                pos->y += (dy / d) * push;
            }
        }
    }
}

bool CircleVsAABB(Vector2 c, float r, Rectangle rect, Vector2* outNormal) {
    float cx = Clamp(c.x, rect.x, rect.x + rect.width);
    float cy = Clamp(c.y, rect.y, rect.y + rect.height);
    float dx = c.x - cx;
    float dy = c.y - cy;
    float d2 = dx * dx + dy * dy;

    if (d2 >= r * r) return false;

    float d = sqrtf(d2);
    if (d > 0.0001f && outNormal) *outNormal = Vector2{ dx / d, dy / d };
    else if (outNormal) *outNormal = Vector2{ 1, 0 };
    return true;
}