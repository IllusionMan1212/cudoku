#pragma once

#include <stdbool.h>

#include "shader.h"

typedef struct Vec2 {
  int x;
  int y;
} Vec2;

typedef struct Cudoku {
  int board[9][9];
  bool has_won;
  Vec2 selection;
  bool should_draw_selection;
} Cudoku;

void set_scale_factor(int width, int height, float *x, float *y);
unsigned int prepare_bg(bool use_texture, unsigned int *texture);
void draw_bg_grid_shader(Shader shader, unsigned int vao, float* transform, float resolution);
void draw_bg_grid_texture(Shader shader, unsigned int vao, unsigned int texture, float* transform);
void draw_numbers(Shader shader, unsigned int vao, unsigned int vbo, float *transform, int board[9][9]);
unsigned int prepare_win_overlay();
void draw_win_overlay(Shader win_shader, Shader font_shader, unsigned int vao, unsigned int font_vao, unsigned int font_vbo, float *transform);
void prepare_selection_box(unsigned int *vao, unsigned int *vbo);
void draw_selection_box(Shader shader, unsigned int vao, unsigned int vbo, int x, int y, float *transform);
void do_selection(Cudoku *game, int x, int y, int width, int height, float x_scale, float y_scale);
void set_selected_number(Cudoku *game, int number);
void move_selection(Cudoku *game, int x, int y);
void toggle_selection(Cudoku *game);
