#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include <glad/gl.h>
#include "3rdparty/stb/stb_image.h"

#include "cudoku.h"
#include "font.h"
#include "shader.h"
#include "audio.h"

#define HELP_TEXT_SIZE 13
#define HELP_OVERLAY_WIDTH 700

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

void set_scale_factor(int width, int height, float *x, float *y) {
  if (width > height) {
    float ratio = (float)height / (float)width;
    *x = ratio;
    *y = 1.f;
  } else {
    float ratio = (float)width / (float)height;
    *x = 1.f;
    *y = ratio;
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
        draw_number(shader, board[i][j], i, j, 5.0f, vao, vbo, transform);
      }
    }
  }

  glBindVertexArray(0);
}

unsigned int prepare_win_overlay() {
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

  glBindVertexArray(0);

  return vao;
}

void draw_win_overlay(Shader win_shader, Shader font_shader, unsigned int vao, unsigned int font_vao, unsigned int font_vbo, float *transform, Cudoku *game) {
  use_shader(win_shader);

  set_mat4f(win_shader, "transform", transform);
  set_vec4f(win_shader, "aColor", 0.0, 0.0, 0.0, 0.8);

  glBindVertexArray(vao);

  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

  glBindVertexArray(0);

  char time_text[64];
  float time_elapsed = game->win_time - game->timer.time;
  snprintf(time_text, 64, "Solved in: %02dm%02ds", (int)time_elapsed / 60, (int)time_elapsed % 60);

  Size win_text_size = calculate_text_size(win_text, 1.0f);
  Size time_text_size = calculate_text_size(time_text, 1.0f);
  draw_text_center(font_shader, win_text, win_text_size, 1.5f, font_vao, font_vbo, transform, 0.0, 75.0);
  draw_text_center(font_shader, time_text, time_text_size, 0.8f, font_vao, font_vbo, transform, 0.0, -75.0);
}

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

  timer_reset(&game->timer);
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

void draw_ui_element_at(Shader shader, unsigned int vao, unsigned int vbo, Matrix4x4 *projection, Vec2 pos, Size size, Color color) {
  use_shader(shader);

  set_mat4f(shader, "transform", (float *)projection->m);
  set_vec4f(shader, "aColor", color.r, color.g, color.b, color.a);

  glBindVertexArray(vao);

  float vertices[6][2] = {
     {pos.x,              pos.y + size.height},
     {pos.x,              pos.y},
     {pos.x + size.width, pos.y},

     {pos.x,              pos.y + size.height},
     {pos.x + size.width, pos.y},
     {pos.x + size.width, pos.y + size.height},
  };

  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glDrawArrays(GL_TRIANGLES, 0, 6);

  glBindVertexArray(0);
}

void draw_help_overlay(Shader overlay_shader, Shader font_shader, unsigned int overlay_vao, unsigned int overlay_vbo, unsigned int font_vao, unsigned int font_vbo, Matrix4x4 *projection, int window_height, Timer *timer) {
  int total_help_texts_height = 20;

  int const text_padding = 10;
  float const text_scale = 0.3f;
  int overlay_height = 0;

  for (int i = 0; i < HELP_TEXT_SIZE; i++) {
    Size text_size = calculate_text_size(help_texts[i], text_scale);
    overlay_height += text_size.height + text_padding;
  }
Size size = {.width = HELP_OVERLAY_WIDTH, .height = overlay_height + 20};
  Vec2 pos = {
    .x = 0,
    .y = window_height - size.height
  };
  Color color = {
    .r = 0.1f,
    .g = 0.1f,
    .b = 0.1f,
    .a = 0.95f
  };
  draw_ui_element_at(overlay_shader, overlay_vao, overlay_vbo, projection, pos, size, color);

  for (int i = 0; i < HELP_TEXT_SIZE; i++) {
    Size text_size = calculate_text_size(help_texts[i], text_scale);
    Vec2 text_pos = {
      .x = text_padding,
      .y = window_height - total_help_texts_height - (text_padding * 2)
    };
    total_help_texts_height += text_size.height + text_padding;
    draw_text_at(font_shader, help_texts[i], text_pos, text_scale, font_vao, font_vbo, (float *)projection->m, NULL);
  }

  if (timer->is_running) {
    char closing_in_text[128];
    snprintf(closing_in_text, sizeof(closing_in_text), "Hiding in %ds", (int)timer_remaining(timer) + 1);
    Size closing_in_text_size = calculate_text_size(closing_in_text, text_scale / 2.0);
    Vec2 closing_in_text_pos = {
      .x = HELP_OVERLAY_WIDTH - closing_in_text_size.width,
      .y = window_height - closing_in_text_size.height - text_padding,
    };
    Color closing_in_text_color = {
      .r = 255.0f,
      .g = 150.0f,
      .b = 0.0f,
      .a = 1.0f,
    };
    draw_text_at(font_shader, closing_in_text, closing_in_text_pos, text_scale / 2.0, font_vao, font_vbo, (float *)projection->m, &closing_in_text_color);
  }
}

void draw_timer(Shader shader, unsigned int vao, unsigned int vbo, float *projection, Timer *timer, int win_width) {
  if (!timer->is_running) return;

  char timer_text[64];

  float elapsed = timer_elapsed(timer);
  int minutes = elapsed / 60;
  int seconds = (int)elapsed % 60;

  snprintf(timer_text, sizeof(timer_text), "Time: %02d:%02d", minutes, seconds);
  float text_scale = 0.35f;
  Size text_size = calculate_text_size(timer_text, text_scale);
  Vec2 text_pos = {
    .x = win_width - text_size.width - 10,
    .y = 10,
  };
  Color text_color = {
    .r = 66.0f,
    .g = 92.0f,
    .b = 124.0f,
    .a = 1.0f,
  };

  draw_text_at(shader, timer_text, text_pos, text_scale, vao, vbo, projection, &text_color);
}
