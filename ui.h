#pragma once

#include <stdbool.h>

#include "helper.h"

typedef struct Size {
  int width;
  int height;
} Size;

typedef struct Sizef {
  float width;
  float height;
} Sizef;

typedef struct Vec2 {
  int x;
  int y;
} Vec2;

typedef struct Vec2f {
  float x;
  float y;
} Vec2f;

typedef struct Color {
  float r;
  float g;
  float b;
  float a;
} Color;

typedef struct Character {
  unsigned int texture_id;
  Size size;
  Size bearing;
  unsigned int advance;
} Character;

typedef struct {
  bool should_quit;
  Color clear_color;
  Size window_size;

  Matrix4x4 projection;
} Context;

typedef enum Alignment {
  ALIGN_TOP_LEFT,
  ALIGN_TOP_CENTER,
  ALIGN_TOP_RIGHT,
  ALIGN_LEFT_CENTER,
  ALIGN_CENTER,
  ALIGN_RIGHT_CENTER,
  ALIGN_BOTTOM_LEFT,
  ALIGN_BOTTOM_CENTER,
  ALIGN_BOTTOM_RIGHT,
} Alignment;

typedef enum UIConstraint {
  UI_CONSTRAINT_FIXED,
  UI_CONSTRAINT_RELATIVE,
  UI_CONSTRAINT_RELATIVE_PIXELS,
  UI_CONSTRAINT_ASPECT_RATIO,
} UIConstraint;

typedef struct UIConstraints {
  float x;
  float y;
  float width;
  float height;
} UIConstraints;

int init_zephr(const char* font_path, const char* window_title, Size window_size, Color *clear_color);
void deinit_zephr();
bool zephr_should_quit();
void zephr_swap_buffers();
Size zephr_get_window_size();
void set_x_constraint(UIConstraints *constraints, float value, UIConstraint type);
void set_y_constraint(UIConstraints *constraints, float value, UIConstraint type);
void set_width_constraint(UIConstraints *constraints, float value, UIConstraint type);
void set_height_constraint(UIConstraints *constraints, float value, UIConstraint type);
void draw_quad(UIConstraints constraints, Color *color, float border_radius, Alignment align);
void draw_circle(UIConstraints constraints, Color *color, Alignment align);
void draw_triangle(UIConstraints constraints, Color *color, Alignment align);
void draw_text(const char* text, int font_size, Vec2f pos, Color *color, Alignment align);
Sizef calculate_text_size(const char *text, int font_size);
