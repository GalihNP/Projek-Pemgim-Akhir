// core/config.h
#pragma once
#include <math.h>

#define SCREEN_W        1280
#define SCREEN_H        720

#define ROOM_COLS       2
#define ROOM_ROWS       2
#define ROOM_W          2000
#define ROOM_H          1200
#define WALL_T          24
#define DOOR_SIZE       140
#define MAX_WALLS       128
#define MAX_ROOMS       32
#define MAX_ENEMIES     64

#define PLAYER_SPEED    260.0f
#define PLAYER_RADIUS   18.0f
#define PLAYER_IFRAMES  0.6f

#define CAM_LERP        6.0f
#define CAM_TRANS_LERP  3.5f
#define CAM_LOOKAHEAD   50.0f

#define SWING_DURATION  0.18f