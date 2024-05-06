#include <assert.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

#include "raylib.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

typedef enum {
  UNLOADED,
  SELECTED,
  RUNNING,
} SketchState;

typedef void sketch_init_function(void);
typedef void sketch_loop_function(void);

typedef struct Sketch {
  const char* path;
  void* handle;
  sketch_init_function* init;
  sketch_loop_function* loop;
  SketchState state;
} Sketch;

Sketch sketch = { .state = UNLOADED };

static void selectSketch(Sketch* sketch, const char* path) {
  assert(sketch->state == UNLOADED);
  sketch->path = path;
  sketch->state = SELECTED;
  TraceLog(LOG_INFO, "HOTRELOAD: selected %s", path);
};

static bool loadSketch(Sketch* sketch) {
  assert(sketch->state == SELECTED);

  void* lib_ptr = dlopen(sketch->path, RTLD_NOW);
  if (lib_ptr == NULL) {
    TraceLog(LOG_ERROR, "HOTRELOAD: could not load %s: %s",
             sketch->path, dlerror());
    goto UNDO;
  }

  sketch_init_function* sym_init_ptr = dlsym(lib_ptr, "init");
  if (sym_init_ptr == NULL) {
    TraceLog(LOG_ERROR, "HOTRELOAD: could not find symbol \"init\" in %s: %s",
             sketch->path, dlerror());
    goto UNDO;
  }

  sketch_loop_function* sym_loop_ptr = dlsym(lib_ptr, "loop");
  if (sym_loop_ptr == NULL) {
    TraceLog(LOG_ERROR, "HOTRELOAD: could not find symbol \"loop\" in %s: %s",
             sketch->path, dlerror());
    goto UNDO;
  }

  sketch->handle = lib_ptr;
  sketch->init = sym_init_ptr;
  sketch->loop = sym_loop_ptr;
  sketch->state = RUNNING;
  TraceLog(LOG_INFO, "HOTRELOAD: loaded %s", sketch->path);
  return true;

UNDO:
  if (sketch->handle)
    dlclose(lib_ptr);
  sketch->state = UNLOADED;
  return false;
}

static void reloadSketch(Sketch* sketch) {
  if (sketch->state == UNLOADED)
    return;
  assert(sketch->state == RUNNING);
  dlclose(sketch->handle);
  sketch->handle = NULL;
  sketch->init = NULL;
  sketch->loop = NULL;
  sketch->state = SELECTED;
  loadSketch(sketch);
}

static void initSketch(Sketch* sketch) {
  assert(sketch->state == RUNNING);
  sketch->init();
  TraceLog(LOG_INFO, "HOTRELOAD: initialized %s", sketch->path);
}

static void loopSketch(Sketch* sketch) {
  sketch->loop();
}

static void drawSketchMenu() {
  BeginDrawing();
  ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));
  if (GuiButton((Rectangle){ 20, 20, 80, 20 }, "Test Sketch")) {
    selectSketch(&sketch, "sketches/test.dylib");
  }
  EndDrawing();
}

int main(void)
{
  InitWindow(800, 600, "Sketchbook");
  SetTargetFPS(60);
  while (!WindowShouldClose()) {
    if (IsKeyPressed(KEY_R)) {
      reloadSketch(&sketch);
    }
    switch (sketch.state) {
    case UNLOADED:
      drawSketchMenu();
      break;
    case SELECTED:
      if (!loadSketch(&sketch))
        break;
      initSketch(&sketch);
    case RUNNING:
      loopSketch(&sketch);
    }
  }
  CloseWindow();
  return EXIT_SUCCESS;
}
