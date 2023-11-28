#pragma once

#include <stdbool.h>

#include "text.h"

typedef struct Context {
  bool should_quit;
  Color clear_color;
  Size screen_size;
  Size window_size;
  ZephFont font;

  Matrix4x4 projection;
} Context;

int init_zephr(const char* font_path, const char* window_title, Size window_size, Color *clear_color);
void deinit_zephr();
bool zephr_should_quit();
void zephr_swap_buffers();
Size zephr_get_window_size();
void zephr_make_window_non_resizable();
