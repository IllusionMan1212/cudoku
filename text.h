#pragma once

#include "ui.h"

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
  Color color;
  float model[4][4];
} TextInstance;

typedef struct GlyphInstanceList {
  TextInstance *data;
  int size;
  int capacity;
} GlyphInstanceList;

int init_fonts(const char *font_path);
Sizef calculate_text_size(const char *text, int font_size);
void draw_text(const char* text, int font_size, UIConstraints constraints, const Color *color, Alignment alignment);
