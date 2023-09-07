#include <glad/gl.h>
#include <stdio.h>

#include "shader.h"

static const float quad_vertices[] = {
  -1.0f, 1.0f, 0.0f, // top left
   1.0f, 1.0f, 0.0f, // top right 
  -1.0f, -1.0f, 0.0f, // bottom left
   1.0f, -1.0f, 0.0f, // bottom right
};

static const int quad_indices[] = {
  0, 1, 2,
  1, 2, 3
};

unsigned int prepare_bg_rect() {
  unsigned int vbo, vao, ebo;
  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);
  glGenBuffers(1, &ebo);

  glBindVertexArray(vao);

  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quad_indices), quad_indices, GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);

  return vao;
}

void draw_bg_rect(Shader shader, unsigned int vao, float* transform) {
  use_shader(shader);

  set_mat4(shader, "transform", transform);

  glBindVertexArray(vao);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

  // TODO: no idea why binding to default (0) breaks the grid shader
  /* glBindVertexArray(0); */
}

void draw_bg_lines(Shader shader, float* transform, float resolution) {
  use_shader(shader);

  set_mat4(shader, "transform", transform);
  set_float(shader, "resolution", resolution);

  glDrawArrays(GL_TRIANGLES, 0, 6);
}
