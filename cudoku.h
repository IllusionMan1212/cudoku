#pragma once

#include <stdbool.h>

#include "shader.h"

void set_scale_factor(int width, int height, float *x, float *y);
unsigned int prepare_bg(bool use_texture, unsigned int *texture);
void draw_bg_grid_shader(Shader shader, unsigned int vao, float* transform, float resolution);
void draw_bg_grid_texture(Shader shader, unsigned int vao, unsigned int texture, float* transform);
void draw_numbers(Shader shader, unsigned int vao, unsigned int vbo, float *transform, int board[9][9]);
void draw_win_overlay(Shader shader, unsigned int vao, unsigned int vbo, float *transform);
