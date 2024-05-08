#include <assert.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syslimits.h>

#include "raylib.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"


typedef struct SketchFileList {
  size_t count;
  char** names;
  char** paths;
} SketchFileList;

static SketchFileList loadSketchFiles() {
  FilePathList files = LoadDirectoryFilesEx("./sketches", ".dylib", false);

  SketchFileList sketch_files = { .count = files.count };
  sketch_files.names = calloc(files.count, sizeof sketch_files.names);
  char* namesp = calloc(files.count, PATH_MAX);
  sketch_files.paths = calloc(files.count, sizeof sketch_files.paths);
  char* pathsp = calloc(files.count, PATH_MAX);

  for (size_t i = 0; i < files.count; i++) {
    sketch_files.names[i] = strncpy(namesp + PATH_MAX * i,
                                    GetFileNameWithoutExt(files.paths[i]),
                                    PATH_MAX);
    sketch_files.paths[i] = strncpy(pathsp + PATH_MAX * i,
                                    files.paths[i],
                                    PATH_MAX);
  }

  UnloadDirectoryFiles(files);
  TraceLog(LOG_INFO, "HOTRELOAD: loaded %d sketch files", sketch_files.count);
  return sketch_files;
}

static void unloadSketchFiles(SketchFileList files) {
  free(files.names[0]);
  free(files.names);
  free(files.paths[0]);
  free(files.paths);
}


typedef enum {
  UNLOADED,
  SELECTED,
  RUNNING,
} SketchState;

typedef void sketch_init_function(void);
typedef void sketch_loop_function(void);

typedef struct Sketch {
  char* path;
  void* handle;
  sketch_init_function* init;
  sketch_loop_function* loop;
  SketchState state;
} Sketch;

static void selectSketch(Sketch* sketch, const char* path) {
  assert(sketch->state == UNLOADED);
  strncpy(sketch->path, path, PATH_MAX);
  sketch->state = SELECTED;
  TraceLog(LOG_INFO, "HOTRELOAD: selected %s", path);
};

static bool loadSketch(Sketch* sketch) {
  assert(sketch->state == SELECTED);

  void* libp = dlopen(sketch->path, RTLD_NOW);
  if (libp == NULL) {
    TraceLog(LOG_ERROR, "HOTRELOAD: could not load %s: %s",
             sketch->path, dlerror());
    goto UNDO;
  }

  sketch_init_function* init_symp = dlsym(libp, "init");
  if (init_symp == NULL) {
    TraceLog(LOG_ERROR, "HOTRELOAD: could not find symbol \"init\" in %s: %s",
             sketch->path, dlerror());
    goto UNDO;
  }

  sketch_loop_function* loop_symp = dlsym(libp, "loop");
  if (loop_symp == NULL) {
    TraceLog(LOG_ERROR, "HOTRELOAD: could not find symbol \"loop\" in %s: %s",
             sketch->path, dlerror());
    goto UNDO;
  }

  sketch->handle = libp;
  sketch->init = init_symp;
  sketch->loop = loop_symp;
  sketch->state = RUNNING;
  TraceLog(LOG_INFO, "HOTRELOAD: loaded %s", sketch->path);
  return true;

UNDO:
  if (sketch->handle)
    dlclose(libp);
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


static void drawSketchMenu(Sketch* current_sketch, SketchFileList* sketches) {
  BeginDrawing();
  ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));
  for (size_t i = 0; i < sketches->count; i++){
    if (GuiButton((Rectangle){ 20 + 100 * i, 20 + 40 * (i % 10), 80, 20 }, sketches->names[i])) {
      selectSketch(current_sketch, sketches->paths[i]);
    }
  }
  EndDrawing();
}

static bool sketchFileChanged(Sketch* current_sketch) {
  static long last_mod_time = 0;
  long mod_time = GetFileModTime(current_sketch->path);
  bool result = (last_mod_time > 0) && (mod_time > last_mod_time);
  last_mod_time = mod_time;
  return result;
}


char path[PATH_MAX];
Sketch current_sketch = { .state = UNLOADED, .path = path };

int main(void)
{
  InitWindow(820, 620, "Sketchbook");
  SetTargetFPS(30);

  // we'll use this as a canvas for sketches
  RenderTexture2D target = LoadRenderTexture(820, 620);

  unsigned char frame_counter = 0;
  SketchFileList sketches = loadSketchFiles();
  while (!WindowShouldClose()) {
    if (++frame_counter >= 60)
      frame_counter = 0;
    if (frame_counter % 12 == 0) {
      if (current_sketch.state == RUNNING && sketchFileChanged(&current_sketch)) {
        reloadSketch(&current_sketch);
      }
    }

    if (IsKeyPressed(KEY_R)) {
      reloadSketch(&current_sketch);
    }

    switch (current_sketch.state) {
    case UNLOADED:
      drawSketchMenu(&current_sketch, &sketches);
      break;
    case SELECTED:
      if (!loadSketch(&current_sketch))
        break;
      BeginTextureMode(target);
      ClearBackground(WHITE);
      initSketch(&current_sketch);
      EndTextureMode();
    case RUNNING:
      BeginTextureMode(target);
      loopSketch(&current_sketch);
      EndTextureMode();
      BeginDrawing();
      ClearBackground(WHITE);
      DrawTextureRec(target.texture, (Rectangle) { 0, 0, (float)target.texture.width, (float)-target.texture.height }, (Vector2) { 0, 0 }, WHITE);
      EndDrawing();
    }
  }

  unloadSketchFiles(sketches);
  UnloadRenderTexture(target);
  CloseWindow();
  return EXIT_SUCCESS;
}
