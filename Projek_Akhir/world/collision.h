// world/collision.h
#pragma once
#include "raylib.h"

// --- Manajemen dinding dunia ---
void ClearWalls(void);
void AddWall(float x, float y, float w, float h);
int  GetWallCount(void);
Rectangle GetWallRect(int index); // <--- TAMBAHKAN BARIS INI

// --- Resolusi tabrakan ---
void ResolveCircleWalls(Vector2* pos, float radius);

// --- Helper geometri murni ---
bool CircleVsAABB(Vector2 center, float radius, Rectangle rect, Vector2* outNormal);