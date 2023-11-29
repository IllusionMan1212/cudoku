#include <stdio.h>

#include <X11/Xlib.h>
#include <glad/glx.h>

#include "zephr.h"
#include "audio.h"
#include "x11.h"
#include "ui.h"
#include "timer.h"

Display *display;
Window window;

Context zephr_context = {0};

int init_zephr(const char* font_path, const char* window_title, Size window_size, Color *clear_color) {
  int res = audio_init();
  if (res != 0) {
    printf("[ERROR]: failed to initialize audio\n");
    return 1;
  }

  res = x11_init(&display, &window, window_title, window_size.width, window_size.height);
  if (res < 0) return 1;

  res = init_ui(font_path, (Size){ .width = window_size.width, .height = window_size.height });
  if (res != 0) {
    printf("[ERROR]: failed to initialize ui\n");
    return 1;
  }

  if (clear_color) {
    zephr_context.clear_color = *clear_color;
  } else {
    zephr_context.clear_color = (Color){0.0, 0.0, 0.0, 1.0};
  }

  x11_get_screen_size(display, &zephr_context.screen_size.width, &zephr_context.screen_size.height);
  start_internal_timer();

  return 0;
}

void deinit_zephr() {
  x11_close(&display, &window);
  audio_close();
}

bool zephr_should_quit() {
  // TODO: input events
  // TODO: window events

  while (XPending(display)) {
    XEvent xev;
    XNextEvent(display, &xev);

    if (xev.type == ConfigureNotify) {
      XConfigureEvent xce = xev.xconfigure;

      if (xce.width != zephr_context.window.size.width || xce.height != zephr_context.window.size.height) {
        zephr_context.window.size = (Size){ .width = xce.width, .height = xce.height };
        zephr_context.projection = orthographic_projection_2d(0.f, xce.width, xce.height, 0.f);
        x11_resize_window(display, window);
      }
    }
  }

  float r = zephr_context.clear_color.r;
  float g = zephr_context.clear_color.g;
  float b = zephr_context.clear_color.b;
  float a = zephr_context.clear_color.a;
  glClearColor(r, g, b, a);
  glClear(GL_COLOR_BUFFER_BIT);

  audio_update();

  return zephr_context.should_quit;
}

// This MUST be called at the end of the frame
void zephr_swap_buffers() {
  glXSwapBuffers(display, window);
}

Size zephr_get_window_size() {
  return zephr_context.window.size;
}

// This MUST be called after calling init_zephr()
void zephr_make_window_non_resizable() {
  x11_make_window_non_resizable(display, window, zephr_context.window.size.width, zephr_context.window.size.height);
}

void zephr_toggle_fullscreen() {
  x11_toggle_fullscreen(zephr_context.window.is_fullscreen, display, window);

  zephr_context.window.is_fullscreen = !zephr_context.window.is_fullscreen;
}

void zephr_quit() {
  zephr_context.should_quit = true;
}
