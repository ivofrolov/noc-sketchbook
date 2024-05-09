#include "raylib.h"

// this function is called in texture rendering context
// BeginTextureMode();
void init(void* storage) {}
// EndTextureMode();

// this function is called in texture rendering context
// BeginTextureMode();
void loop(void* storage) {
  ClearBackground(RAYWHITE);
  DrawText("Congrats! You created your first window!", 190, 200, 20, LIGHTGRAY);
}
// EndTextureMode();

void deinit(void* storage) {}
