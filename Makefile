RAYLIB_LIB_PATH = out
RAYLIB_SRC_PATH = lib/raylib/src
RAYGUI_SRC_PATH = lib/raygui/src
export RAYLIB_RELEASE_PATH := $(CURDIR)/$(RAYLIB_LIB_PATH)
export RAYLIB_LIBTYPE = SHARED

$(RAYLIB_LIB_PATH)/libraylib.dylib:
	$(MAKE) -C $(RAYLIB_SRC_PATH) raylib


CFLAGS = -O2 -std=c99 -D_DEFAULT_SOURCE
CFLAGS += -Wall -Wextra -Wno-missing-braces -Wunused-result -Wstrict-prototypes
CFLAGS += -Wl,-rpath,$(RAYLIB_RELEASE_PATH)
INCLUDE_PATHS = -I. -I$(RAYLIB_SRC_PATH) -I$(RAYLIB_SRC_PATH)/external -I$(RAYGUI_SRC_PATH)
LDFLAGS = -L. -L$(RAYLIB_RELEASE_PATH) -L$(RAYLIB_SRC_PATH)
LDLIBS = -lraylib -framework OpenGL -framework Cocoa -framework IOKit -framework CoreAudio -framework CoreVideo

SKETCHBOOK_SRC_PATH = .
SKETCHBOOK_BIN_PATH = out

$(SKETCHBOOK_BIN_PATH)/sketchbook: $(SKETCHBOOK_SRC_PATH)/sketchbook.c | $(RAYLIB_LIB_PATH)/libraylib.dylib
	$(CC) -o $@ $< $(CFLAGS) $(INCLUDE_PATHS) $(LDFLAGS) $(LDLIBS)

SKETCHES_SRC_PATH = sketches
SKETCHES_LIB_PATH = out/sketches
# just for autocompletion
SKETCHES = $(patsubst $(SKETCHES_SRC_PATH)/%.c,$(SKETCHES_LIB_PATH)/%.dylib,$(wildcard $(SKETCHES_SRC_PATH)/*.c))

$(SKETCHES): $(SKETCHES_LIB_PATH)/%.dylib: $(SKETCHES_SRC_PATH)/%.c | $(RAYLIB_LIB_PATH)/libraylib.dylib
	$(CC) -dynamiclib -o $@ $< $(CFLAGS) $(INCLUDE_PATHS) $(LDFLAGS) $(LDLIBS)


TAGS: $(RAYLIB_SRC_PATH)/*.[ch] $(RAYGUI_SRC_PATH)/*.[ch] $(SKETCHES_SRC_PATH)/*.[ch] *.c
	etags -l c --declarations -o $@ $^
