#pragma once

#include "cudoku.h"
#include "shader.h"


int init_fonts(const char *font_path);
void prepare_font(unsigned int *vao, unsigned int *vbo);
Size calculate_text_size(const char *text, float scale);
void draw_number(Shader shader, Cell cell, int row, int column, float scale,unsigned int vao, unsigned int vbo, float* transform);
void draw_text_center(Shader shader, const char *text, Size text_size, float scale, unsigned int vao, unsigned int vbo, float *transform, float offset_x, float offset_y);
void draw_text_at(Shader shader, const char *text, Vec2 pos, float scale, unsigned int vao, unsigned int vbo, float *projection, Color *color);
