#pragma once

#include "shader.h"

unsigned int prepare_bg_rect(Shader shader);
void draw_bg_rect(Shader shader, unsigned int vao, float* transform);
void draw_bg_lines(Shader shader, float* transform, float resolution);
