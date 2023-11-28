#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include <glad/gl.h>
#include "3rdparty/stb/stb_image.h"

#include "cudoku.h"
#include "shader.h"
#include "audio.h"
#include "ui.h"

#define HELP_TEXT_SIZE 13

static const float quad_vertices[] = {
  // positions     // texture coords
  -1.0f, 1.0f, 0.0f, 0.0f, 1.0f,  // top left
   1.0f, 1.0f, 0.0f, 1.0f, 1.0f, // top right 
  -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, // bottom left
   1.0f, -1.0f, 0.0f, 1.0f, 0.0f // bottom right
};

static const int quad_indices[] = {
  0, 1, 2,
  1, 2, 3
};

static const Color mistake_color = {1.f, 0.f, 0.f, 0.5f};

static const char *win_text = "You won!";

static const char *help_texts[HELP_TEXT_SIZE] = {
  "F1 - Toggle help",
  "F11 - Toggle fullscreen",
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
  "Ctrl+Q/Esc - Quit"
};

void draw_win(Cudoku *game) {
  Size window = zephr_get_window_size();
  Color bg_color = {0, 0, 0, 180.f};
  Color text_color = {237.f, 225.f, 215.f, 255.f};

  char time_text[64];

  float time_elapsed = game->win_time - game->timer.time;
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
  Color color = {0.0f, 0.0f, 0.0f, 255.f};
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
    draw_quad(constraints, &color, 0.0, ALIGN_TOP_LEFT);

    set_x_constraint(&constraints, 0, UI_CONSTRAINT_FIXED);
    set_y_constraint(&constraints, i * 100, UI_CONSTRAINT_FIXED);
    set_width_constraint(&constraints, window_size.width, UI_CONSTRAINT_FIXED);
    if (i % 3 == 0) {
      set_height_constraint(&constraints, 4, UI_CONSTRAINT_FIXED);
    } else {
      set_height_constraint(&constraints, 2, UI_CONSTRAINT_FIXED);
    }
    draw_quad(constraints, &color, 0.0, ALIGN_TOP_LEFT);
  }

  set_x_constraint(&constraints, 0, UI_CONSTRAINT_FIXED);
  set_y_constraint(&constraints, 0, UI_CONSTRAINT_FIXED);
  set_width_constraint(&constraints, 1, UI_CONSTRAINT_FIXED);
  set_height_constraint(&constraints, 1, UI_CONSTRAINT_FIXED);

  for (int i = 0; i < 9; i++) {
    for (int j = 0; j < 9; j++) {
      if (!game->board[i][j].value) {
        continue;
      }
      Sizef text_size = calculate_text_size("7", 72.f);
      char num[2];
      snprintf(num, sizeof(num), "%d", game->board[i][j].value);
      set_y_constraint(&constraints, i * 100 + cell_size.height / 2.f - text_size.height / 2.f, UI_CONSTRAINT_FIXED);
      set_x_constraint(&constraints, j * 100 + cell_size.width / 2.f - text_size.width / 2.f, UI_CONSTRAINT_FIXED);
      draw_text(num, 72.f, constraints, &color, ALIGN_TOP_LEFT);
    }
  }
}

unsigned int prepare_bg(bool use_texture, unsigned int *texture) {
  unsigned int vbo, vao, ebo;
  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);
  glGenBuffers(1, &ebo);

  glBindVertexArray(vao);

  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quad_indices), quad_indices, GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);

  if (use_texture) {
    int tex_width = 0, tex_height = 0;
    unsigned char *tex_data = NULL;

    glGenTextures(1, &(*texture));
    glBindTexture(GL_TEXTURE_2D, *texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    tex_data = stbi_load("assets/sudoku-grid.png", &tex_width, &tex_height, NULL, 0);
    if (tex_data) {
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tex_width, tex_height, 0, GL_RGB, GL_UNSIGNED_BYTE, tex_data);
      glGenerateMipmap(GL_TEXTURE_2D);

      stbi_image_free(tex_data);
    } else {
      printf("[ERROR] could not load texture image\n");
    }

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(3 * sizeof(float)));
  }

  glBindVertexArray(0);

  return vao;
}

void draw_bg_grid_shader(Shader shader, unsigned int vao, float* transform, float resolution) {
  use_shader(shader);

  set_mat4f(shader, "transform", transform);
  set_float(shader, "resolution", resolution);

  glBindVertexArray(vao);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

  glBindVertexArray(0);
}

void draw_bg_grid_texture(Shader shader, unsigned int vao, unsigned int texture, float* transform) {
  use_shader(shader);
  set_mat4f(shader, "transform", transform);

  glBindVertexArray(vao);
  glBindTexture(GL_TEXTURE_2D, texture);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

  glBindVertexArray(0);
}

void draw_numbers(Shader shader, unsigned int vao, unsigned int vbo, float *transform, Cell board[9][9]) {
  for (int i = 0; i < 9; i++) {
    for (int j = 0; j < 9; j++) {
      if (board[i][j].value != 0) {
        /* draw_number(shader, board[i][j], i, j, 5.0f, vao, vbo, transform); */
      }
    }
  }

  glBindVertexArray(0);
}

/* void draw_pause_overlay(Cudoku *game) { */
/*   timer_pause(&game->timer); */

/*   // */
/*   // this is my basic idea of drawing ui elements */
/*   // */
/*   // examples of drawing shapes */
/*   draw_quad(beginning_x, beginning_y, win_width, win_height); */
/*   draw_circle(); */ 
/*   draw_triangle(); */

/*   // example of drawing text */
/*   draw_text_at(pause_text, font_scale, posx, posy, color); */

/*   // TODO: rotations, translation, scaling */
/*   // TODO: buttons and other elements */
/* } */

void prepare_selection_box(unsigned int *vao, unsigned int *vbo) {
  glGenVertexArrays(1, &(*vao));
  glGenBuffers(1, &(*vbo));

  glBindVertexArray(*vao);
  glBindBuffer(GL_ARRAY_BUFFER, *vbo);

  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6*2, NULL, GL_DYNAMIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), 0);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}


void draw_selection_box(Shader shader, unsigned int vao, unsigned int vbo, int x, int y, float *transform, Color color) {
  use_shader(shader);

  set_mat4f(shader, "transform", transform);
  set_vec4f(shader, "aColor", color.r, color.g, color.b, color.a);

  glBindVertexArray(vao);

  float vert = 0.11f;
  // get size of a single cell in opengl coords (-1, 1)
  float single_cell = (1.f/9.f) * 2;
  // we minus 4 since we start at column and row 4 (0-indexed)
  float offset_x = single_cell * (y - 4);
  float offset_y = single_cell * (x - 4);

  float vertices[6][2] = {
    // top left tri
    { -vert + offset_x, -vert - offset_y },
    { -vert + offset_x,  vert - offset_y },
    {  vert + offset_x,  vert - offset_y },

    // bottom right tri
    { -vert + offset_x, -vert - offset_y },
    {  vert + offset_x,  vert - offset_y },
    {  vert + offset_x, -vert - offset_y }
  };

  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glDrawArrays(GL_TRIANGLES, 0, 6);

  glBindVertexArray(0);
}

void highlight_mistakes(Shader shader, unsigned int vao, unsigned vbo, float *transform, Cudoku *game) {
  for (int i = 0; i < 9; i++) {
    for (int j = 0; j < 9; j++) {
      Cell cell = game->board[i][j];
      if (cell.value != 0 && cell.value != game->solution[i][j]) {
        draw_selection_box(shader, vao, vbo, i, j, transform, mistake_color);
      }
    }
  }
}

void toggle_check(Cudoku *game) {
  game->should_highlight_mistakes = !game->should_highlight_mistakes;
}

void do_selection(Cudoku *game, int x, int y, int width, int height, float x_scale, float y_scale) {
  if (game->has_won) return;

  float board_x_start = (width / 2.f) - ((width * x_scale) / 2);
  float board_y_start = (height / 2.f) - ((height * y_scale) / 2);

  float cell_width = (width * x_scale) / 9.f;
  float cell_height = (height * y_scale) / 9.f;


  int cell_x = floor((x - board_x_start) / cell_width);
  int cell_y = floor((y - board_y_start) / cell_height);

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
  game->should_draw_selection = false;
  game->should_highlight_mistakes = false;
  game->should_draw_help = false;
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
  if (!game->has_won)
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
  int box_row = floor(row / 3.f) * 3;
  int box_col = floor(col / 3.f) * 3;
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
    uint rand_idx = rand() % num_candidates;
    uint candidate = candidates[rand_idx];
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
    int removed_row = floor(cell / 9.f);
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

    const int k = floor(row / 3.f) * 3;
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
  if (game->has_won) return;

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
  if (!game->has_won) {
    game->should_draw_help = !game->should_draw_help;
    return game->should_draw_help;
  }

  return false;
}

void draw_help(Timer *timer) {
  int const text_padding = 10;
  float const help_font_size = 24.f;
  float const closing_in_font_size = 18.f;
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

  int overlay_height = 0;
  int overlay_width = 0;
  int total_help_texts_height = 10;

  for (int i = 0; i < HELP_TEXT_SIZE; i++) {
    Sizef text_size = calculate_text_size(help_texts[i], help_font_size);
    overlay_height += text_size.height + text_padding;
    overlay_width = max(overlay_width, text_size.width + text_padding * 2);
  }
  Size size = {.width = overlay_width, .height = overlay_height + text_padding * 2};
  UIConstraints constraints = {0};
  set_width_constraint(&constraints, size.width, UI_CONSTRAINT_FIXED);
  set_height_constraint(&constraints, size.height, UI_CONSTRAINT_FIXED);
  draw_quad(constraints, &bg_color, 0.0, ALIGN_TOP_LEFT);

  set_x_constraint(&constraints, text_padding, UI_CONSTRAINT_FIXED);
  for (int i = 0; i < HELP_TEXT_SIZE; i++) {
    Sizef text_size = calculate_text_size(help_texts[i], help_font_size);
    set_y_constraint(&constraints, total_help_texts_height, UI_CONSTRAINT_FIXED);
    set_width_constraint(&constraints, 1, UI_CONSTRAINT_FIXED);
    set_height_constraint(&constraints, 1, UI_CONSTRAINT_FIXED);
    draw_text(help_texts[i], help_font_size, constraints, &text_color, ALIGN_TOP_LEFT);

    total_help_texts_height += text_size.height + text_padding;
  }

  if (timer->is_running) {
    char closing_in_text[128];
    snprintf(closing_in_text, sizeof(closing_in_text), "Hiding in %ds", (int)timer_remaining(timer) + 1);
    Sizef closing_in_text_size = calculate_text_size(closing_in_text, closing_in_font_size);
    set_x_constraint(&constraints, overlay_width - closing_in_text_size.width - text_padding, UI_CONSTRAINT_FIXED);
    set_y_constraint(&constraints, text_padding, UI_CONSTRAINT_FIXED);
    draw_text(closing_in_text, closing_in_font_size, constraints, &closing_in_text_color, ALIGN_TOP_LEFT);
  }
}

void draw_timer(Timer *timer) {
  if (!timer->is_running) return;

  char timer_text[64];
  Color text_color = {
    .r = 66.0f,
    .g = 92.0f,
    .b = 124.0f,
    .a = 255.0f,
  };
    Color bg_color = {
    .r = 0.0f,
    .g = 0.0f,
    .b = 0.0f,
    .a = 25.0f,
  };
  float font_size = 32.f;
  Alignment alignment = ALIGN_BOTTOM_RIGHT;

  float elapsed = timer_elapsed(timer);
  int minutes = elapsed / 60;
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
