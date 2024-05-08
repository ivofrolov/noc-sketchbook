#include "raylib.h"

// this function is called in texture rendering context
// BeginTextureMode();
void init(void) {}
// EndTextureMode();

// this function is called in texture rendering context
// BeginTextureMode();
void loop(void) {
  ClearBackground(RAYWHITE);
  DrawText("Congrats! You created your first window!", 190, 200, 20, LIGHTGRAY);
}
// EndTextureMode();
