#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>

#include <glad/gl.h>
#include "3rdparty/stb/stb_image.h"

#include "cudoku.h"
#include "font.h"
#include "shader.h"

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

void draw_numbers(Shader shader, unsigned int vao, unsigned int vbo, float *transform, int board[9][9]) {
  for (int i = 0; i < 9; i++) {
    for (int j = 0; j < 9; j++) {
      if (board[i][j] != 0) {
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

void draw_win_overlay(Shader win_shader, Shader font_shader, unsigned int vao, unsigned int font_vao, unsigned int font_vbo, float *transform) {
  use_shader(win_shader);

  set_mat4f(win_shader, "transform", transform);
  set_vec4f(win_shader, "aColor", 0.0, 0.0, 0.0, 0.8);

  glBindVertexArray(vao);

  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

  glBindVertexArray(0);

  const char *text = "You won!";
  Size text_size = calculate_text_size(text, 1.0f);
  draw_text(font_shader, text, text_size, 1.5f, font_vao, font_vbo, transform);
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

void draw_selection_box(Shader shader, unsigned int vao, unsigned int vbo, int x, int y, float *transform) {
  use_shader(shader);

  set_mat4f(shader, "transform", transform);
  set_vec4f(shader, "aColor", 0.4, 0.4, 1.0, 0.5);

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

void do_selection(Cudoku *game, int x, int y, int width, int height, float x_scale, float y_scale) {
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

void set_selected_number(Cudoku *game, int number) {
  if (game->should_draw_selection && !game->has_won) {
    game->board[game->selection.x][game->selection.y] = number;
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
