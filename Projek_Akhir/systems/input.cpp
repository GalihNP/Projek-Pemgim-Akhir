// systems/input.c
#include "input.h"

InputState ReadInput(void) {
    InputState s = { 0 };
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))    s.move.y -= 1;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))  s.move.y += 1;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))  s.move.x -= 1;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) s.move.x += 1;

    s.attackPressed = IsKeyPressed(KEY_J) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    s.attackHeld = IsKeyDown(KEY_J) || IsMouseButtonDown(MOUSE_BUTTON_LEFT);

    if (IsKeyPressed(KEY_ONE))   s.weaponSwitch = 0;
    if (IsKeyPressed(KEY_TWO))   s.weaponSwitch = 1;
    if (IsKeyPressed(KEY_THREE)) s.weaponSwitch = 2;
    if (IsKeyPressed(KEY_FOUR))  s.weaponSwitch = 3;
    return s;
}