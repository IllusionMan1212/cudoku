#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "core.h"
#include "cudoku.h"
#include "audio.h"
#include "ui.h"
#include "text.h"
#include "zephr.h"

#define HELP_TEXT_SIZE 13

static const Color mistake_color = {255.f, 0.f, 0.f, 127};
static const Color selection_color = {102, 102, 255, 255};

static const char *win_text = "You won!";

static const char *help_texts[HELP_TEXT_SIZE] = {
  "F1 - Toggle help",
  "1-9 - Set number",
  "0/Del/Backspace - Remove number",
  "Left/A/H - Move selection left",
  "Right/D/L - Move selection right",
  "Up/W/K - Move selection up",
  "Down/S/J - Move selection down",
  "Space/Enter - Toggle selection",
  "N - New board",
  "C - Check for mistakes",
  "R - Reset board",
  "P - Pause/Resume",
  "Ctrl+Q/Esc - Quit"
};

void draw_win(Cudoku *game) {
  Size window = zephr_get_window_size();
  Color bg_color = {0, 0, 0, 200.f};
  Color text_color = {237.f, 225.f, 215.f, 255.f};

  char time_text[64];

  double time_elapsed = game->win_time - game->timer.start + game->timer.elapsed;
  snprintf(time_text, 64, "Solved in: %02dm%02ds", (int)time_elapsed / 60, (int)time_elapsed % 60);

  UIConstraints constraints = {0};
  set_x_constraint(&constraints, 0, UI_CONSTRAINT_FIXED);
  set_y_constraint(&constraints, 0, UI_CONSTRAINT_FIXED);
  set_width_constraint(&constraints, window.width, UI_CONSTRAINT_FIXED);
  set_height_constraint(&constraints, window.height, UI_CONSTRAINT_FIXED);

  draw_quad(constraints, &bg_color, 0.0, ALIGN_CENTER);

  set_width_constraint(&constraints, 1, UI_CONSTRAINT_FIXED);
  set_height_constraint(&constraints, 1, UI_CONSTRAINT_FIXED);

  set_y_constraint(&constraints, -30, UI_CONSTRAINT_FIXED);
  draw_text(win_text, 72.f, constraints, &text_color, ALIGN_CENTER);
  set_y_constraint(&constraints, 30, UI_CONSTRAINT_FIXED);
  draw_text(time_text, 48.f, constraints, &text_color, ALIGN_CENTER);
}

void draw_board(Cudoku *game, Size window_size) {
  Color bg_color = {240.0f, 235.0f, 227.0f, 255.f};
  Size cell_size = {window_size.width / 9, window_size.height / 9};
  UIConstraints constraints = {0};
  set_x_constraint(&constraints, 0, UI_CONSTRAINT_FIXED);
  set_y_constraint(&constraints, 0, UI_CONSTRAINT_FIXED);
  set_width_constraint(&constraints, window_size.width, UI_CONSTRAINT_FIXED);
  set_height_constraint(&constraints, window_size.height, UI_CONSTRAINT_FIXED);

  draw_quad(constraints, &bg_color, 0.0, ALIGN_TOP_LEFT);

  for (int i = 1; i < 9; i++) {
    set_x_constraint(&constraints, i * 100, UI_CONSTRAINT_FIXED);
    set_y_constraint(&constraints, 0, UI_CONSTRAINT_FIXED);
    if (i % 3 == 0) {
      set_width_constraint(&constraints, 4, UI_CONSTRAINT_FIXED);
    } else {
      set_width_constraint(&constraints, 2, UI_CONSTRAINT_FIXED);
    }
    set_height_constraint(&constraints, window_size.height, UI_CONSTRAINT_FIXED);
    draw_quad(constraints, NULL, 0.0, ALIGN_TOP_LEFT);

    set_x_constraint(&constraints, 0, UI_CONSTRAINT_FIXED);
    set_y_constraint(&constraints, i * 100, UI_CONSTRAINT_FIXED);
    set_width_constraint(&constraints, window_size.width, UI_CONSTRAINT_FIXED);
    if (i % 3 == 0) {
      set_height_constraint(&constraints, 4, UI_CONSTRAINT_FIXED);
    } else {
      set_height_constraint(&constraints, 2, UI_CONSTRAINT_FIXED);
    }
    draw_quad(constraints, NULL, 0.0, ALIGN_TOP_LEFT);
  }

  if (game->timer.state != TIMER_PAUSED) {
    set_x_constraint(&constraints, 0, UI_CONSTRAINT_FIXED);
    set_y_constraint(&constraints, 0, UI_CONSTRAINT_FIXED);
    set_width_constraint(&constraints, 1, UI_CONSTRAINT_FIXED);
    set_height_constraint(&constraints, 1, UI_CONSTRAINT_FIXED);

    GlyphInstanceList batch;
    new_glyph_instance_list(&batch, 81);

    for (int i = 0; i < 9; i++) {
      for (int j = 0; j < 9; j++) {
        if (!game->board[i][j].value) {
          continue;
        }
        char num[2];
        snprintf(num, sizeof(num), "%d", game->board[i][j].value);
        Sizef text_size = calculate_text_size(num, 72.f);
        set_y_constraint(&constraints, i * 100 + cell_size.height / 2.f - text_size.height / 2.f, UI_CONSTRAINT_FIXED);
        set_x_constraint(&constraints, j * 100 + cell_size.width / 2.f - text_size.width / 2.f, UI_CONSTRAINT_FIXED);
        if (game->board[i][j].is_locked) {
          add_text_instance(&batch, num, 72.f, constraints, NULL, ALIGN_TOP_LEFT);
        } else {
          add_text_instance(&batch, num, 72.f, constraints, &selection_color, ALIGN_TOP_LEFT);
        }
      }
    }

    draw_text_batch(&batch);
  }
}

void draw_pause_overlay() {
  Size window = zephr_get_window_size();
  Color bg_color = {0, 0, 0, 200.f};
  Color text_color = {237.f, 225.f, 215.f, 255.f};

  UIConstraints constraints = {0};
  set_x_constraint(&constraints, 0, UI_CONSTRAINT_FIXED);
  set_y_constraint(&constraints, 0, UI_CONSTRAINT_FIXED);
  set_width_constraint(&constraints, window.width, UI_CONSTRAINT_FIXED);
  set_height_constraint(&constraints, window.height, UI_CONSTRAINT_FIXED);

  draw_quad(constraints, &bg_color, 0.0, ALIGN_CENTER);

  set_width_constraint(&constraints, 1, UI_CONSTRAINT_FIXED);
  set_height_constraint(&constraints, 1, UI_CONSTRAINT_FIXED);

  draw_text("Paused", 100.f, constraints, &text_color, ALIGN_CENTER);
}

void pause_game(Cudoku *game) {
  if (game->has_won) return;

  if (game->timer.state == TIMER_PAUSED) {
    timer_resume(&game->timer);
  } else if (game->timer.state == TIMER_RUNNING) {
    game->should_draw_help = false;
    game->should_draw_selection = false;
    game->should_highlight_mistakes = false;
    timer_pause(&game->timer);
    timer_stop(&game->help_timer);
  }
}

void draw_selection_box(int x, int y, const Color color) {
  UIConstraints constraints = {0};
  set_x_constraint(&constraints, x * 100, UI_CONSTRAINT_FIXED);
  set_y_constraint(&constraints, y * 100, UI_CONSTRAINT_FIXED);
  set_width_constraint(&constraints, 100, UI_CONSTRAINT_FIXED);
  set_height_constraint(&constraints, 100, UI_CONSTRAINT_FIXED);

  draw_quad(constraints, &color, 0.0, ALIGN_TOP_LEFT);
}

void highlight_mistakes(Cudoku *game) {
  for (int i = 0; i < 9; i++) {
    for (int j = 0; j < 9; j++) {
      Cell cell = game->board[i][j];
      if (cell.value != 0 && cell.value != game->solution[i][j]) {
        draw_selection_box(j, i, mistake_color);
      }
    }
  }

  UIConstraints constraints = {0};

  const int font_size = 24;
  const char *text = "Mistake highlighter is ON";
  Sizef text_size = calculate_text_size(text, font_size);
  set_width_constraint(&constraints, text_size.width, UI_CONSTRAINT_FIXED);
  set_height_constraint(&constraints, text_size.height, UI_CONSTRAINT_FIXED);
  draw_quad(constraints, &(Color){0, 0, 0, 30}, 0.0, ALIGN_BOTTOM_LEFT);

  set_width_constraint(&constraints, 1, UI_CONSTRAINT_FIXED);
  set_height_constraint(&constraints, 1, UI_CONSTRAINT_FIXED);
  draw_text(text, font_size, constraints, &(Color){255, 0, 0, 255}, ALIGN_BOTTOM_LEFT);
}

void toggle_check(Cudoku *game) {
  if (game->has_won || game->timer.state == TIMER_PAUSED) return;

  game->should_highlight_mistakes = !game->should_highlight_mistakes;
}

void do_selection(Cudoku *game, int x, int y) {
  if (game->has_won || game->timer.state == TIMER_PAUSED) return;

  int cell_x = (int)floor(x / 100.f);
  int cell_y = (int)floor(y / 100.f);

  if (cell_x < 0 || cell_x > 8 || cell_y < 0 || cell_y > 8) {
    game->should_draw_selection = false;
    return;
  }

  game->should_draw_selection = true;
  game->selection.x = cell_y;
  game->selection.y = cell_x;
}

void reset_state(Cudoku *game) {
  for (int i = 0; i < 9; i++) {
    for (int j = 0; j < 9; j++) {
      game->board[i][j].value = 0;
      game->board[i][j].is_locked = false;
    }
  }

  game->should_draw_selection = false;
  game->should_highlight_mistakes = false;
  game->has_won = false;
  timer_reset(&game->timer);
}

bool check_win(Cudoku *game) {
  for (int i = 0; i < 9; i++) {
    for (int j = 0; j < 9; j++) {
      if (game->board[i][j].is_locked) continue;
      if (game->board[i][j].value != game->solution[i][j]) return false;
    }
  }

  game->has_won = true;
  game->win_time = get_time();
  timer_stop(&game->timer);

  audio_play_win();

  return true;
}

void set_selected_number(Cudoku *game, int number) {
  if (game->should_draw_selection && !game->has_won && !game->board[game->selection.x][game->selection.y].is_locked) {
    if (number != 0)
      audio_play_scribble();
    game->board[game->selection.x][game->selection.y].value = number;
    check_win(game);
  }
}

void move_selection(Cudoku *game, int x, int y) {
  if (game->should_draw_selection && !game->has_won) {
    game->selection.x += y;
    game->selection.y += x;

    if (game->selection.x < 0) {
      game->selection.x = 0;
    } else if (game->selection.x > 8) {
      game->selection.x = 8;
    }

    if (game->selection.y < 0) {
      game->selection.y = 0;
    } else if (game->selection.y > 8) {
      game->selection.y = 8;
    }
  }
}

void toggle_selection(Cudoku *game) {
  if (game->has_won || game->timer.state == TIMER_PAUSED) return;

  game->should_draw_selection = !game->should_draw_selection;
}

void remove_arr_element(int *arr, int index, int size) {
  for (int i = index; i < size - 1; i++) {
    arr[i] = arr[i + 1];
  }
}

void get_candidates(Cell board[9][9], int row, int col, int candidates[], int *size) {
  // use this as a set to check if a number is already in the row, col, or box
  // 0 is no clash, 1 is clash
  int clashingDigits[9] = {0};
  // check row
  for (int i = 0; i < 9; i++) {
    if (board[row][i].value != 0) {
      clashingDigits[board[row][i].value - 1] = 1;
    }
  }

  // check col
  for (int i = 0; i < 9; i++) {
    if (board[i][col].value != 0) {
      clashingDigits[board[i][col].value - 1] = 1;
    }
  }

  // check box
  int box_row = (int)floor(row / 3.f) * 3;
  int box_col = (int)floor(col / 3.f) * 3;
  for (int i = box_row; i < box_row + 3; i++) {
    for (int j = box_col; j < box_col + 3; j++) {
      if (board[i][j].value != 0) {
        clashingDigits[board[i][j].value - 1] = 1;
      }
    }
  }

  for (int i = 0; i < 9; i++) {
    if (clashingDigits[i] != 1) {
      candidates[(*size)++] = i + 1;
    }
  }
}

bool backtracker(Cudoku *game, int start_row, int start_col) {
  // if we went thru all the cols but not all the rows
  if (start_col >= 9 && start_row < 9 - 1) {
    start_row++;
    start_col = 0;
  }

  // if we went thru the entire board
  if (start_row >= 8 && start_col >= 9) {
    return true;
  }

  // if we're at diagonal box 1, 5 or 9, skip it
  if (start_row < 3) {
    if (start_col < 3) {
      start_col = 3;
    }
  } else if (start_row < 6) {
    if (start_col == (floor(start_row / 3.f) * 3)) {
      start_col += 3;
    }
  } else {
    if (start_col == 6) {
      start_row++;
      start_col = 0;
      if (start_row >= 9) {
        return true;
      }
    }
  }

  int num_candidates = 0;
  int candidates[9] = {0};
  get_candidates(game->board, start_row, start_col, candidates, &num_candidates);

  // try every candidate until we find one that uniquely solves the board.
  while (num_candidates) {
    u32 rand_idx = rand() % num_candidates;
    u32 candidate = candidates[rand_idx];
    remove_arr_element(candidates, rand_idx, num_candidates);
    num_candidates--;
    game->board[start_row][start_col].value = candidate;
    game->board[start_row][start_col].is_locked = true;
    if (backtracker(game, start_row, start_col + 1)) {
      return true;
    }
    game->board[start_row][start_col].value = 0;
    game->board[start_row][start_col].is_locked = false;
  }

  // if no candidates, we backtrack and try something else.
  return false;
}

bool solver(int *solutions, Vec2 empty_cells[81], int empty_cells_size, Cell board[9][9]) {
  if (empty_cells_size) {
    (*solutions)++;
    if (*solutions > 1) {
      return false;
    }
    return true;
  }

  int rand_idx = rand() % empty_cells_size;
  Vec2 rand_empty_cell = empty_cells[rand_idx];

  int candidates_size = 0;
  int candidates[9];
  get_candidates(board, rand_empty_cell.x, rand_empty_cell.y, candidates, &candidates_size);

  while (candidates_size) {
    rand_idx = rand() % candidates_size;
    int candidate = candidates[rand_idx];
    remove_arr_element(candidates, rand_idx, candidates_size--);

    Cell cell = { .is_locked = true, .value = candidate };
    board[rand_empty_cell.x][rand_empty_cell.y] = cell;
    if (solver(solutions, empty_cells, empty_cells_size, board)) {
      return true;
    }

    // reset the cell
    cell.value = 0;
    cell.is_locked = false;
    board[rand_empty_cell.x][rand_empty_cell.y] = cell;
  }

  empty_cells_size++;
  empty_cells[empty_cells_size] = rand_empty_cell;

  return false;
}

void remove_numbers(Cudoku *game) {
  int solutions = 0;
  int removed_cells_size = 0;
  Vec2 removed_cells[81];

  int filled_cells_size = 81;
  int filled_cells[81];
  for (int i = 0; i < filled_cells_size; i++) {
    filled_cells[i] = i;
  }

  while (filled_cells_size) {
    solutions = 0;

    int rand_idx = rand() % filled_cells_size;
    int cell = filled_cells[rand_idx];
    remove_arr_element(filled_cells, rand_idx, filled_cells_size--);
    int removed_row = (int)floor(cell / 9.f);
    int removed_col = cell % 9;

    Cell removed_cell = game->board[removed_row][removed_col];
    Cell empty = {0};
    game->board[removed_row][removed_col] = empty;

    Vec2 removed_cell_coords = {.x = removed_row, .y = removed_col};
    removed_cells[removed_cells_size] = removed_cell_coords;
    removed_cells_size++;

    int candidates_size = 0;
    int candidates[9];
    get_candidates(game->board, removed_row, removed_col, candidates, &candidates_size);

    Vec2 removed_cells_cpy[81];
    Cell board_cpy[9][9];

    while (candidates_size) {
      candidates_size--;

      memcpy(removed_cells_cpy, removed_cells, sizeof(Cell) * removed_cells_size);
      memcpy(board_cpy, game->board, sizeof(Cell) * 81);
      solver(&solutions, removed_cells_cpy, removed_cells_size, board_cpy);

      if (solutions > 1) {
        game->board[removed_row][removed_col] = removed_cell;
        removed_cells_size--;
        break;
      }
    }
  }
}

void generate_random_board(Cudoku *game) {
  if (game->timer.state == TIMER_PAUSED) return;
  reset_state(game);
  int size = 9;
  // fill diagonal boxes 1, 5, 9
  int digits[] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
  for (int row = 0; row < 9; row++) {
    if (row % 3 == 0) {
      for (int l = 0; l < 9; l++) {
        digits[l] = l + 1;
      }
      size = 9;
    }

    const int k = (int)floor(row / 3.f) * 3;
    for (int j = k; j < k + 3; j++) {
      int rand_idx = rand() % size;
      int num = digits[rand_idx];
      remove_arr_element(digits, rand_idx, size--);
      game->board[row][j].value = num;
      game->board[row][j].is_locked = true;
    }
  }

  // fill the rest of the boxes
  backtracker(game, 0, 3);

  // copy the board to the solution
  for (int i = 0; i < 9; i++) {
    for (int j = 0; j < 9; j++) {
      game->solution[i][j] = game->board[i][j].value;
    }
  }

  remove_numbers(game);
}

void reset_board(Cudoku *game) {
  if (game->has_won || game->timer.state == TIMER_PAUSED) return;

  for (int i = 0; i < 9; i++) {
    for (int j = 0; j < 9; j++) {
      if (game->board[i][j].is_locked == false) {
        game->board[i][j].value = 0;
      }
    }
  }

  timer_reset(&game->timer);
}

bool toggle_help(Cudoku *game) {
  if (game->has_won || game->timer.state == TIMER_PAUSED) return false;

  game->should_draw_help = !game->should_draw_help;
  return game->should_draw_help;
}

void draw_help(Timer *timer) {
  int const text_padding = 10;
  int const help_font_size = 24;
  int const closing_in_font_size = 18;
  Color const bg_color = {
    .r = 0.f,
    .g = 0.f,
    .b = 0.f,
    .a = 185.f
  };
  Color const text_color = {
    237.f,
    255.f,
    255.f,
    255.f,
  };
  Color const closing_in_text_color = {
    .r = 255.0f,
    .g = 150.0f,
    .b = 0.0f,
    .a = 255.0f,
  };

  float overlay_height = 0;
  float overlay_width = 0;
  float total_help_texts_height = 10;

  for (int i = 0; i < HELP_TEXT_SIZE; i++) {
    Sizef text_size = calculate_text_size(help_texts[i], help_font_size);
    overlay_height += text_size.height + text_padding;
    overlay_width = CORE_MAX(overlay_width, text_size.width + text_padding * 2);
  }
  Sizef size = {.width = overlay_width, .height = overlay_height + text_padding * 2};
  UIConstraints constraints = {0};
  set_width_constraint(&constraints, size.width, UI_CONSTRAINT_FIXED);
  set_height_constraint(&constraints, size.height, UI_CONSTRAINT_FIXED);
  draw_quad(constraints, &bg_color, 0.0, ALIGN_TOP_LEFT);

  GlyphInstanceList batch;
  new_glyph_instance_list(&batch, 100);

  set_x_constraint(&constraints, text_padding, UI_CONSTRAINT_FIXED);
  for (int i = 0; i < HELP_TEXT_SIZE; i++) {
    Sizef text_size = calculate_text_size(help_texts[i], help_font_size);
    set_y_constraint(&constraints, total_help_texts_height, UI_CONSTRAINT_FIXED);
    set_width_constraint(&constraints, 1, UI_CONSTRAINT_FIXED);
    set_height_constraint(&constraints, 1, UI_CONSTRAINT_FIXED);
    add_text_instance(&batch, help_texts[i], help_font_size, constraints, &text_color, ALIGN_TOP_LEFT);

    total_help_texts_height += text_size.height + text_padding;
  }

  if (timer->state == TIMER_RUNNING) {
    char closing_in_text[128];
    snprintf(closing_in_text, sizeof(closing_in_text), "Hiding in %ds", (int)timer_remaining(timer) + 1);
    Sizef closing_in_text_size = calculate_text_size(closing_in_text, closing_in_font_size);
    set_x_constraint(&constraints, overlay_width - closing_in_text_size.width - text_padding, UI_CONSTRAINT_FIXED);
    set_y_constraint(&constraints, text_padding, UI_CONSTRAINT_FIXED);
    add_text_instance(&batch, closing_in_text, closing_in_font_size, constraints, &closing_in_text_color, ALIGN_TOP_LEFT);
  }

  draw_text_batch(&batch);
}

void draw_timer(Timer *timer) {
  if (timer->state == TIMER_STOPPED) return;

  const int font_size = 32;
  const Color text_color = {
    .r = 66.0f,
    .g = 92.0f,
    .b = 124.0f,
    .a = 255.0f,
  };
  const Color bg_color = {
    .r = 0.0f,
    .g = 0.0f,
    .b = 0.0f,
    .a = 25.0f,
  };
  const Alignment alignment = ALIGN_BOTTOM_RIGHT;

  char timer_text[64];

  double elapsed = timer_elapsed(timer);

  if (timer->state == TIMER_PAUSED) {
    // if the timer is paused, we want to show the time at which it was paused
    elapsed = timer->elapsed;
  }

  int minutes = (int)elapsed / 60;
  int seconds = (int)elapsed % 60;

  snprintf(timer_text, sizeof(timer_text), "Time: %02d:%02d", minutes, seconds);

  Sizef text_size = calculate_text_size(timer_text, font_size);

  UIConstraints constraints = {0};
  set_x_constraint(&constraints, 0, UI_CONSTRAINT_FIXED);
  set_y_constraint(&constraints, 0, UI_CONSTRAINT_FIXED);
  set_width_constraint(&constraints, text_size.width + 4, UI_CONSTRAINT_FIXED);
  set_height_constraint(&constraints, text_size.height + 4, UI_CONSTRAINT_FIXED);

  draw_quad(constraints, &bg_color, 0.0, alignment);

  set_width_constraint(&constraints, 1, UI_CONSTRAINT_FIXED);
  set_height_constraint(&constraints, 1, UI_CONSTRAINT_FIXED);
  draw_text(timer_text, font_size, constraints, &text_color, alignment);
}
