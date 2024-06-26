# Sketchbook

I created this simple program to code along [The Nature of
Code](https://natureofcode.com/) but it can be used for any sketches.

It's based on [Raylib](https://www.raylib.com/).

It can

  * make menu list of sketches,
  * execute sketch compiled to dynamic library,
  * reload sketch on file change.

Also you can press F10 to go back to the sketch menu.

### Building Sketchbook

Use `make out/sketchbook` command to compile the app.

### Building Sketch

Use `make out/sketches/<sketch>.dylib` command to compile your sketch.

I like to use [fswatch](https://github.com/emcrisostomo/fswatch) to automatically recompile sketch on source file change to utilize hot reloading.

    fswatch -0 sketches/test.c | xargs -0 -n 1 -I {} make out/sketches/test.dylib

## Notes

Currently only MacOS is supported, but it is not difficult to extend the program to other operating systems.
