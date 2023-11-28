#pragma once

#include <stdbool.h>

#include "shader.h"
#include "timer.h"
#include "ui.h"

typedef struct Cell {
  int value;
  bool is_locked;
} Cell;

typedef struct Cudoku {
  Cell board[9][9];
  int solution[9][9];
  bool has_won;
  Vec2 selection;
  bool should_draw_selection;
  bool should_highlight_mistakes;
  bool should_draw_help;
  Timer help_timer;
  Timer timer;
  float win_time;
} Cudoku;

void draw_bg_grid_texture(Shader shader, unsigned int vao, unsigned int texture, float* transform);
void prepare_selection_box(unsigned int *vao, unsigned int *vbo);
void draw_selection_box(Shader shader, unsigned int vao, unsigned int vbo, int x, int y, float *transform, Color color);
void highlight_mistakes(Shader shader, unsigned int vao, unsigned vbo, float *transform, Cudoku *game);
void toggle_check(Cudoku *game);
void do_selection(Cudoku *game, int x, int y, int width, int height, float x_scale, float y_scale);
void set_selected_number(Cudoku *game, int number);
void move_selection(Cudoku *game, int x, int y);
void toggle_selection(Cudoku *game);
void generate_random_board(Cudoku *game);
void reset_board(Cudoku *game);
bool toggle_help(Cudoku *game);
void draw_help(Timer *timer);
void draw_timer(Timer *timer);
void draw_win(Cudoku *game);
void draw_board(Cudoku *game, Size window_size);
