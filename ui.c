#include <stdio.h>
#include <math.h>

#include <X11/Xlib.h>
#include <glad/glx.h>

#include "cudoku.h"
#include "shader.h"
#include "ui.h"
#include "x11.h"
#include "zephr.h"

extern Context zephr_context;
Shader ui_shader;
unsigned int ui_vao;
unsigned int ui_vbo;

int init_ui(const char* font_path, Size window_size) {
  zephr_context.window_size = window_size;
  zephr_context.projection = orthographic_projection_2d(0.f, window_size.width, window_size.height, 0.f);

  int res = init_fonts(font_path);
  if (res == -1) {
    printf("[ERROR]: could not initialize freetype library\n");
    return 1;
  } else if (res == -2) {
    printf("[ERROR]: could not load font file: \"%s\"\n", font_path);
    return 1;
  }

  ui_shader = create_shader("shaders/ui.vert", "shaders/ui.frag");

  glGenVertexArrays(1, &ui_vao);
  glGenBuffers(1, &ui_vbo);

  glBindVertexArray(ui_vao);
  glBindBuffer(GL_ARRAY_BUFFER, ui_vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  return 0;
}

void set_x_constraint(UIConstraints *constraints, float value, UIConstraint type) {
  switch (type) {
    case UI_CONSTRAINT_RELATIVE_PIXELS:
    case UI_CONSTRAINT_ASPECT_RATIO:
      // no-op. fallback to FIXED
    case UI_CONSTRAINT_FIXED:
      constraints->x = value;
      break;
    case UI_CONSTRAINT_RELATIVE:
      constraints->x = (value * zephr_context.window_size.width);
      break;
  }
}

void set_y_constraint(UIConstraints *constraints, float value, UIConstraint type) {
  switch (type) {
    case UI_CONSTRAINT_RELATIVE_PIXELS:
    case UI_CONSTRAINT_ASPECT_RATIO:
      // no-op. fallback to FIXED
    case UI_CONSTRAINT_FIXED:
      constraints->y = value;
      break;
    case UI_CONSTRAINT_RELATIVE:
      constraints->y = (value * zephr_context.window_size.height);
      break;
  }
}

void set_width_constraint(UIConstraints *constraints, float value, UIConstraint type) {
  switch (type) {
    case UI_CONSTRAINT_FIXED:
      constraints->width = value;
      break;
    case UI_CONSTRAINT_RELATIVE:
      constraints->width = (value * zephr_context.window_size.width);
      break;
    case UI_CONSTRAINT_RELATIVE_PIXELS:
      constraints->width = zephr_context.window_size.width / (float)zephr_context.screen_size.width * value;
      break;
    case UI_CONSTRAINT_ASPECT_RATIO:
      constraints->width = constraints->height * value;
      break;
  }
}

void set_height_constraint(UIConstraints *constraints, float value, UIConstraint type) {
  switch (type) {
    case UI_CONSTRAINT_FIXED:
      constraints->height = value;
      break;
    case UI_CONSTRAINT_RELATIVE:
      constraints->height = (value * zephr_context.window_size.height);
      break;
    case UI_CONSTRAINT_RELATIVE_PIXELS:
      constraints->height = zephr_context.window_size.height / (float)zephr_context.screen_size.height * value;
      break;
    case UI_CONSTRAINT_ASPECT_RATIO:
      constraints->height = constraints->width * value;
      break;
  }
}

void set_rotation_constraint(UIConstraints *constraints, float angle_d) {
  constraints->rotation = angle_d;
}

void apply_constraints(UIConstraints constraints, Vec2f *pos, Sizef *size) {
  *pos = (Vec2f){constraints.x, constraints.y};
  *size = (Sizef){constraints.width, constraints.height};
}

void apply_alignment(Alignment align, Vec2f *pos, Sizef size) {
  switch (align) {
    case ALIGN_TOP_LEFT:
      pos->x = pos->x;
      pos->y = pos->y;
      break;
    case ALIGN_TOP_CENTER:
      pos->x += zephr_context.window_size.width / 2.f - size.width / 2.f;
      pos->y = pos->y;
      break;
    case ALIGN_TOP_RIGHT:
      pos->x += zephr_context.window_size.width - size.width;
      pos->y = pos->y;
      break;
    case ALIGN_BOTTOM_LEFT:
      pos->x = pos->x;
      pos->y += zephr_context.window_size.height - size.height;
      break;
    case ALIGN_BOTTOM_CENTER:
      pos->x += zephr_context.window_size.width / 2.f - size.width / 2.f;
      pos->y += zephr_context.window_size.height - size.height;
      break;
    case ALIGN_BOTTOM_RIGHT:
      pos->x += zephr_context.window_size.width - size.width;
      pos->y += zephr_context.window_size.height - size.height;
      break;
    case ALIGN_LEFT_CENTER:
      pos->x = pos->x;
      pos->y += zephr_context.window_size.height / 2.f - size.height / 2.f;
      break;
    case ALIGN_RIGHT_CENTER:
      pos->x += zephr_context.window_size.width - size.width;
      pos->y += zephr_context.window_size.height / 2.f - size.height / 2.f;
      break;
    case ALIGN_CENTER:
      pos->x += zephr_context.window_size.width / 2.f - size.width / 2.f;
      pos->y += zephr_context.window_size.height / 2.f - size.height / 2.f;
      break;
  }
}

void draw_quad(UIConstraints constraints, const Color *color, float border_radius, Alignment align) {
  use_shader(ui_shader);

  if (color) {
    set_vec4f(ui_shader, "aColor", color->r / 255.f, color->g / 255.f, color->b / 255.f, color->a / 255.f);
  } else {
    set_vec4f(ui_shader, "aColor", 0.f, 0.f, 0.f, 1.f);
  }

  set_float(ui_shader, "uiWidth", constraints.width);
  set_float(ui_shader, "uiHeight", constraints.height);
  set_float(ui_shader, "borderRadius", border_radius);
  set_mat4f(ui_shader, "projection", (float *)zephr_context.projection.m);

  Vec2f pos = { 0.f, 0.f };
  Sizef size = { 0.f, 0.f };

  apply_constraints(constraints, &pos, &size);
  apply_alignment(align, &pos, size);

  Matrix4x4 model = identity();
  apply_translation(&model, (Vec2f){-size.width / 2.f, -size.height / 2.f});
  apply_rotation(&model, to_radians(constraints.rotation));
  apply_translation(&model, (Vec2f){size.width / 2.f, size.height / 2.f});

  apply_translation(&model, pos);

  set_mat4f(ui_shader, "model", (float *)model.m);

  glBindVertexArray(ui_vao);

  float vertices[6][4] = {
    // bottom left tri
    {0,              0 + size.height, 0.0, 1.0},
    {0,              0,               0.0, 0.0},
    {0 + size.width, 0,               1.0, 0.0},

    // top right tri
    {0,              0 + size.height, 0.0, 1.0},
    {0 + size.width, 0,               1.0, 0.0},
    {0 + size.width, 0 + size.height, 1.0, 1.0},
  };

  glBindBuffer(GL_ARRAY_BUFFER, ui_vbo);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glDrawArrays(GL_TRIANGLES, 0, 6);

  glBindVertexArray(0);
}

void draw_circle(UIConstraints constraints, const Color *color, Alignment align) {
  float radius = 0.f;

  if (constraints.width > constraints.height) {
    radius = constraints.height / 2.f;
  } else {
    radius = constraints.width / 2.f;
  }
  draw_quad(constraints, color, radius, align);
}

void draw_triangle(UIConstraints constraints, const Color *color, Alignment align) {
  use_shader(ui_shader);

  if (color) {
    set_vec4f(ui_shader, "aColor", color->r / 255.f, color->g / 255.f, color->b / 255.f, color->a / 255.f);
  } else {
    set_vec4f(ui_shader, "aColor", 0.f, 0.f, 0.f, 1.f);
  }
  set_mat4f(ui_shader, "projection", (float *)zephr_context.projection.m);

  Vec2f pos = { 0.f, 0.f };
  Sizef size = { 0.f, 0.f };

  apply_constraints(constraints, &pos, &size);
  apply_alignment(align, &pos, size);

  Matrix4x4 model = identity();
  apply_translation(&model, (Vec2f){-size.width / 2.f, -size.height / 2.f});
  apply_rotation(&model, to_radians(constraints.rotation));
  apply_translation(&model, (Vec2f){size.width / 2.f, size.height / 2.f});

  apply_translation(&model, pos);

  set_mat4f(ui_shader, "model", (float *)model.m);

  glBindVertexArray(ui_vao);

  float vertices[3][4] = {
    {0 + size.width,     0,               0.0, 0.0},
    {0,                  0,               0.0, 0.0},
    {0 + size.width / 2, 0 + size.height, 0.0, 0.0},
  };

  glBindBuffer(GL_ARRAY_BUFFER, ui_vbo);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glDrawArrays(GL_TRIANGLES, 0, 3);

  glBindVertexArray(0);
}

/* enum Element { */
/*   Rect, */
/*   Circle, */
/* }; */

/* struct UIElement { */
/*   Vec2 pos; */
/*   Size size; */
/*   Color *color; */
/*   enum Element type; */
/* }; */

/* struct UIText { */
/*   Vec2 pos; */
/*   float scale; */
/*   Color *color; */
/*   const char* text; */
/* }; */

/* struct UIElement *create_element(Vec2 pos, Size size, Color *color, enum Element type) { */
/*   struct UIElement *element = malloc(sizeof(struct UIElement)); */
/*   element->pos = pos; */
/*   element->size = size; */
/*   element->color = color; */
/*   element->type = type; */
/*   return element; */
/* } */

