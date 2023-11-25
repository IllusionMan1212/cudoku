#pragma once

#include <stdbool.h>
#include <stdlib.h>

#include "z_math.h"

typedef struct Character {
  Size size;
  Size bearing;
  unsigned int advance;
  Vec2f tex_coords[4]; // the 4 corners of the character quad
} Character;

typedef struct ZephFont {
  Character characters[128];
  unsigned int atlas_texture_id;
} ZephFont;

typedef struct TextInstance {
  Vec4f position;
  int tex_coords_index;
  float model[4][4];
} TextInstance;

typedef struct Text {
  TextInstance *instance_data;
  int instance_count;
  int instance_capacity;
} Text;

typedef struct Context {
  bool should_quit;
  Color clear_color;
  Size screen_size;
  Size window_size;
  ZephFont font;
  Text texts;

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
  float rotation;
} UIConstraints;

int init_zephr(const char* font_path, const char* window_title, Size window_size, Color *clear_color);
void deinit_zephr();
void make_window_non_resizable();
bool zephr_should_quit();
void zephr_swap_buffers();
Size zephr_get_window_size();
void zephr_batch_text_draw();
void set_x_constraint(UIConstraints *constraints, float value, UIConstraint type);
void set_y_constraint(UIConstraints *constraints, float value, UIConstraint type);
void set_width_constraint(UIConstraints *constraints, float value, UIConstraint type);
void set_height_constraint(UIConstraints *constraints, float value, UIConstraint type);
void set_rotation_constraint(UIConstraints *constraints, float angle_d);
void draw_quad(UIConstraints constraints, Color *color, float border_radius, Alignment align);
void draw_circle(UIConstraints constraints, Color *color, Alignment align);
void draw_triangle(UIConstraints constraints, Color *color, Alignment align);
void draw_text(const char* text, int font_size, UIConstraints constraints, Color *color, Alignment align);
Sizef calculate_text_size(const char *text, int font_size);
