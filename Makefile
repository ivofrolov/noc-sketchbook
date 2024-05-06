SKETCHES_PATH := sketches

# see lib/raylib/examples/Makefile
RAYLIB_PATH = lib/raylib
RAYGUI_PATH = lib/raygui
export RAYLIB_RELEASE_PATH := $(CURDIR)/dist
export RAYLIB_LIBTYPE = SHARED
CFLAGS = -O2 -std=c99 -D_DEFAULT_SOURCE
CFLAGS += -Wall -Wextra -Wno-missing-braces -Wunused-result -Wmissing-prototypes -Wstrict-prototypes
CFLAGS += -Wl,-rpath,$(RAYLIB_RELEASE_PATH)
INCLUDE_PATHS = -I. -I$(RAYLIB_PATH)/src -I$(RAYLIB_PATH)/src/external -I$(RAYGUI_PATH)/src
LDFLAGS = -L. -L$(RAYLIB_RELEASE_PATH) -L$(RAYLIB_PATH)/src
LDLIBS = -lraylib -framework OpenGL -framework Cocoa -framework IOKit -framework CoreAudio -framework CoreVideo

dist/libraylib.dylib:
	$(MAKE) -C $(RAYLIB_PATH)/src raylib

# sketches: sketches/test

# https://www.gnu.org/software/make/manual/html_node/Static-Usage.html
sketches/%: sketches/%.c | dist/libraylib.dylib
	$(CC) -dynamiclib -o $@.dylib $< $(CFLAGS) $(INCLUDE_PATHS) $(LDFLAGS) $(LDLIBS)

sketchbook: sketchbook.c | dist/libraylib.dylib
	$(CC) -o $@ $< $(CFLAGS) $(INCLUDE_PATHS) $(LDFLAGS) $(LDLIBS)

TAGS: $(RAYLIB_PATH)/src/*.[ch] $(RAYGUI_PATH)/src/*.[ch] $(SKETCHES_PATH)/*.[ch] *.c
	etags -l c --declarations -o $@ $?
