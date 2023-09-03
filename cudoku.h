#pragma once

#include "shader.h"

unsigned int prepare_bg_rect(Shader shader);
void draw_bg_rect(Shader shader, unsigned int vao, float x_scale, float y_scale);
