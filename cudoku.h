#pragma once

#include <stdbool.h>

#include "timer.h"
#include "zephr_math.h"

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
  double win_time;
} Cudoku;

void draw_selection_box(int x, int y, const Color color);
void highlight_mistakes(Cudoku *game);
void toggle_check(Cudoku *game);
void do_selection(Cudoku *game, int x, int y);
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
void pause_game(Cudoku *game);
void draw_pause_overlay();
