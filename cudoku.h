#pragma once

#include <stdbool.h>

#include "shader.h"

void set_scale_factor(int width, int height, float *x, float *y);
unsigned int prepare_bg(bool use_texture);
void draw_bg_grid_shader(Shader shader, int vao, float* transform, float resolution);
void draw_bg_grid_texture(Shader shader, int vao, float* transform);
