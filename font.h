#pragma once

#include "cudoku.h"
#include "shader.h"

typedef struct Size {
  int width;
  int height;
} Size;

typedef struct Character {
  unsigned int texture_id;
  Size size;
  Size bearing;
  unsigned int advance;
} Character;

int init_fonts(const char *font_path);
void prepare_font(unsigned int *vao, unsigned int *vbo);
Size calculate_text_size(const char *text, float scale);
void draw_number(Shader shader, Cell cell, int row, int column, float scale,unsigned int vao, unsigned int vbo, float* transform);
void draw_text(Shader shader, const char *text, Size text_size, float scale, unsigned int vao, unsigned int vbo, float *transform);
