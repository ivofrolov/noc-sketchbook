#include "raylib.h"

void init() {};

void loop() {
  BeginDrawing();
  ClearBackground(RAYWHITE);
  DrawText("Congrats! You created your second window!", 190, 200, 20, LIGHTGRAY);
  EndDrawing();
};
