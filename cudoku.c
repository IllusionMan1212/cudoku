#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

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
}

void draw_bg_grid_texture(Shader shader, unsigned int vao, unsigned int texture, float* transform) {
  use_shader(shader);
  set_mat4f(shader, "transform", transform);

  glBindVertexArray(vao);
  glBindTexture(GL_TEXTURE_2D, texture);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void draw_numbers(Shader shader, unsigned int vao, unsigned int vbo, float *transform, int board[9][9]) {
  // TODO: go thru the board state and call draw_text() with the appropriate number
  // and position

  for (int i = 0; i < 9; i++) {
    for (int j = 0; j < 9; j++) {
      draw_number(shader, board[i][j], i, j, 5.0f, vao, vbo, transform);
    }
  }
}

void draw_win_overlay(Shader shader, unsigned int vao, unsigned int vbo, float *transform) {
  const char *text = "You won!";
  Size text_size = calculate_text_size(text, 1.0f);
  draw_text(shader, text, text_size, 1.5f, vao, vbo, transform);
}
