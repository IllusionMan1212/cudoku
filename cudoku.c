#include <glad/gl.h>

#include "shader.h"

static const float quad_vertices[] = {
  -0.5f, 1.0f, 0.0f, // top left
   0.5f, 1.0f, 0.0f, // top right 
  -0.5f, -1.0f, 0.0f, // bottom left
   0.5f, -1.0f, 0.0f, // bottom right
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

void draw_bg_rect(Shader shader, unsigned int vao) {
  use_shader(shader);

  glBindVertexArray(vao);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

  glBindVertexArray(0);
}
