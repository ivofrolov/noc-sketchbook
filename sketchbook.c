#include <assert.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syslimits.h>

#include "raylib.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include "sketches/defs.h"


typedef struct SketchFileList {
  unsigned int count;
  char** names;
  char** paths;
} SketchFileList;

SketchFileList loadSketchFiles(const char* directory) {
  FilePathList files = LoadDirectoryFilesEx(directory, ".dylib", false);

  SketchFileList sketch_files = { .count = files.count };
  sketch_files.names = calloc(files.count, sizeof sketch_files.names);
  char* namesp = calloc(files.count, PATH_MAX);
  sketch_files.paths = calloc(files.count, sizeof sketch_files.paths);
  char* pathsp = calloc(files.count, PATH_MAX);

  for (unsigned int i = 0; i < files.count; i++) {
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

void unloadSketchFiles(SketchFileList files) {
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

typedef void sketch_callback(void*);

typedef struct Sketch {
  char* path;
  void* handle;
  sketch_callback* init;
  sketch_callback* loop;
  void* arena;
  SketchState state;
} Sketch;

void selectSketch(Sketch* sketch, const char* path) {
  assert(sketch->state == UNLOADED);
  strncpy(sketch->path, path, PATH_MAX);
  sketch->state = SELECTED;
};

bool loadSketch(Sketch* sketch) {
  assert(sketch->state == SELECTED);

  void* libp = dlopen(sketch->path, RTLD_NOW);
  if (libp == NULL) {
    TraceLog(LOG_ERROR, "HOTRELOAD: could not load %s: %s",
             sketch->path, dlerror());
    goto UNDO;
  }

  sketch_callback* init_symp = dlsym(libp, "init");
  if (init_symp == NULL) {
    TraceLog(LOG_ERROR, "HOTRELOAD: could not find symbol \"init\" in %s: %s",
             sketch->path, dlerror());
    goto UNDO;
  }

  sketch_callback* loop_symp = dlsym(libp, "loop");
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

void unloadSketch(Sketch* sketch) {
  assert(sketch->state == RUNNING);
  dlclose(sketch->handle);
  sketch->handle = NULL;
  sketch->init = NULL;
  sketch->loop = NULL;
  sketch->state = UNLOADED;
  TraceLog(LOG_INFO, "HOTRELOAD: unloaded %s", sketch->path);
}

void reloadSketch(Sketch* sketch) {
  assert(sketch->state == RUNNING);
  unloadSketch(sketch);
  sketch->state = SELECTED;
  loadSketch(sketch);
}

void initSketch(Sketch* sketch) {
  assert(sketch->state == RUNNING);
  sketch->init(sketch->arena);
  TraceLog(LOG_INFO, "HOTRELOAD: initialized %s", sketch->path);
}

void loopSketch(Sketch* sketch) {
  sketch->loop(sketch->arena);
}


void drawSketchMenu(Sketch* current_sketch,
                           int screen_width, int screen_height,
                           SketchFileList* sketches) {
  int button_width = 80;
  int button_height = 20;
  int gap = 20;
  int cols;
  int window_offset;

  div_t qr = div(screen_width, button_width + gap);
  cols = qr.quot;
  if (qr.rem >= gap) {
    window_offset = gap + (qr.rem - gap) / 2;
  } else {
    window_offset = gap - (qr.rem - gap) / 2;
  }

  for (unsigned int i = 0; i < sketches->count; i++) {
    int x = window_offset + (gap + button_width) * (i % cols);
    int y = gap + (gap + button_height) * (i / cols);
    Rectangle btn_bounds = { x, y, button_width, button_height };
    if (GuiButton(btn_bounds, sketches->names[i])) {
      selectSketch(current_sketch, sketches->paths[i]);
    }
  }
}

bool sketchFileChanged(Sketch* current_sketch) {
  static long last_mod_time = 0;
  long mod_time = GetFileModTime(current_sketch->path);
  bool result = (last_mod_time > 0) && (mod_time > last_mod_time);
  last_mod_time = mod_time;
  return result;
}


int main(void) {
  int screen_width = 820;
  int screen_height = 620;
  int fps = 30;
  InitWindow(screen_width, screen_height, "Sketchbook");
  SetTargetFPS(fps);

  // we'll use this as a canvas for sketches
  RenderTexture2D canvas = LoadRenderTexture(screen_width, screen_height);

  SketchFileList sketches = loadSketchFiles("./out/sketches");
  Sketch current_sketch = {
    .state = UNLOADED,
    .path = calloc(PATH_MAX, sizeof(char)),
    .arena = malloc(ARENA_SIZE),
  };

  unsigned char sketch_check_counter = 0;
  while (!WindowShouldClose()) {
    if (++sketch_check_counter % (fps / 4) == 0) {
      sketch_check_counter = 0;
      if (current_sketch.state == RUNNING && sketchFileChanged(&current_sketch)) {
        reloadSketch(&current_sketch);
      }
    }

    if (IsKeyPressed(KEY_F10)) {
      unloadSketch(&current_sketch);
    }

    switch (current_sketch.state) {
    case UNLOADED:
      BeginDrawing();
      ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));
      drawSketchMenu(&current_sketch, screen_width, screen_height, &sketches);
      EndDrawing();
      break;
    case SELECTED:
      if (!loadSketch(&current_sketch))
        break;
      BeginTextureMode(canvas);
      ClearBackground(WHITE);
      initSketch(&current_sketch);
      EndTextureMode();
    case RUNNING:
      BeginTextureMode(canvas);
      loopSketch(&current_sketch);
      EndTextureMode();
      BeginDrawing();
      ClearBackground(WHITE);
      // rendered texture must be y-flipped due to default OpenGL coordinates (left-bottom)
      Rectangle texture_src = { 0, 0, (float)canvas.texture.width, (float)-canvas.texture.height };
      Vector2 texture_pos = { 0, 0 };
      DrawTextureRec(canvas.texture, texture_src, texture_pos, WHITE);
      EndDrawing();
    }
  }

  unloadSketchFiles(sketches);
  UnloadRenderTexture(canvas);
  CloseWindow();
  return EXIT_SUCCESS;
}
