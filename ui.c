#include <stdio.h>
#include <math.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include <freetype/ftmm.h>
#include <X11/Xlib.h>
#include <glad/glx.h>

#include "cudoku.h"
#include "shader.h"
#include "audio.h"
#include "ui.h"
#include "x11.h"

#define FONT_PIXEL_SIZE 128

Display *display;
Window window;

Context zephr_context;
Character characters[128];
Shader font_shader;
Shader ui_shader;
unsigned int font_vao;
unsigned int ui_vao;
unsigned int font_vbo;
unsigned int ui_vbo;

// TODO: instanced rendering of ui elements and text??

int init_fonts(const char *font_path) {
  FT_Library ft;
  if (FT_Init_FreeType(&ft)) {
    return -1;
  }

  FT_Face face;
  if (FT_New_Face(ft, font_path, 0, &face)) {
    return -2;
  }

  if ((face->face_flags & FT_FACE_FLAG_MULTIPLE_MASTERS)) {
    printf("[INFO] Got a variable font\n");
    FT_MM_Var *mm;
    FT_Get_MM_Var(face, &mm);

    FT_Set_Var_Design_Coordinates(face, mm->num_namedstyles, mm->namedstyle[mm->num_namedstyles - 4].coords);

    FT_Done_MM_Var(ft, mm);
  }

  FT_Set_Pixel_Sizes(face, 0, FONT_PIXEL_SIZE);

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  for (int c = 0; c < 128; c++) {
    if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
      printf("[ERROR]: failed to load glyph for char '%c'\n", c);
    }

    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, face->glyph->bitmap.width,
        face->glyph->bitmap.rows, 0, GL_RED, GL_UNSIGNED_BYTE,
        face->glyph->bitmap.buffer);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    characters[c].texture_id = texture;
    characters[c].advance = face->glyph->advance.x;
    characters[c].size = (Size){ .width = face->glyph->bitmap.width, .height = face->glyph->bitmap.rows };
    characters[c].bearing = (Size){ .width = face->glyph->bitmap_left, .height = face->glyph->bitmap_top };
  }

  glBindTexture(GL_TEXTURE_2D, 0);

  FT_Done_Face(face);
  FT_Done_FreeType(ft);

  return 0;
}

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

  font_shader = create_shader("shaders/ui.vert", "shaders/font.frag");
  ui_shader = create_shader("shaders/ui.vert", "shaders/ui.frag");

  glGenVertexArrays(1, &font_vao);
  glGenVertexArrays(1, &ui_vao);
  glGenBuffers(1, &font_vbo);
  glGenBuffers(1, &ui_vbo);

  glBindVertexArray(font_vao);
  glBindBuffer(GL_ARRAY_BUFFER, font_vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);

  glBindVertexArray(ui_vao);
  glBindBuffer(GL_ARRAY_BUFFER, ui_vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  return 0;
}

int init_zephr(const char* font_path, const char* window_title, Size window_size, Color *clear_color) {
  int res = audio_init();
  if (res != 0) {
    printf("[ERROR]: failed to initialize audio\n");
    return 1;
  }

  res = init_x11(&display, &window, window_title, window_size.width, window_size.height);
  if (res < 0) return 1;

  res = init_ui(font_path, (Size){ .width = window_size.width, .height = window_size.height });
  if (res != 0) {
    printf("[ERROR]: failed to initialize ui\n");
    return 1;
  }

  if (clear_color) {
    zephr_context.clear_color = *clear_color;
  } else {
    zephr_context.clear_color = (Color){0.0, 0.0, 0.0, 1.0};
  }

  start_internal_timer();

  return 0;
}

void deinit_zephr() {
  close_x11(&display, &window);
  audio_close();
}

bool zephr_should_quit() {
  // TODO: input events
  // TODO: window events
  // TODO: ui update??

  while (XPending(display)) {
    XEvent xev;
    XNextEvent(display, &xev);

    if (xev.type == ConfigureNotify) {
      XConfigureEvent xce = xev.xconfigure;

      if (xce.width != zephr_context.window_size.width || xce.height != zephr_context.window_size.height) {
        zephr_context.window_size = (Size){ .width = xce.width, .height = xce.height };
        zephr_context.projection = orthographic_projection_2d(0.f, xce.width, xce.height, 0.f);
        resize_x11_window(display, window);
        /* set_scale_factor(xce.width, xce.height, &x_scale, &y_scale); */
      }

    } else if (xev.type == KeyPress) {
      if (XLookupKeysym(&xev.xkey, 0) == XK_Escape ||
          (XLookupKeysym(&xev.xkey, 0) == XK_q && xev.xkey.state & ControlMask)) {
        zephr_context.should_quit = true;
        break;
      } else {
        /* handle_keypress(xev, &game); */
      }
    } else if (xev.type == ButtonPress) {
      /* if (xev.xbutton.button == Button1) { */
      /*   int x = xev.xbutton.x; */
      /*   int y = xev.xbutton.y; */
      /*   do_selection(&game, x, y, width, height, x_scale, y_scale); */
      /* } */
    }
  }

  float r = zephr_context.clear_color.r;
  float g = zephr_context.clear_color.g;
  float b = zephr_context.clear_color.b;
  float a = zephr_context.clear_color.a;
  glClearColor(r, g, b, a);
  glClear(GL_COLOR_BUFFER_BIT);

  /* update_ui(); */
  audio_update();

  return zephr_context.should_quit;
}

void zephr_swap_buffers() {
  glXSwapBuffers(display, window);
}

Size zephr_get_window_size() {
  return zephr_context.window_size;
}

Sizef calculate_text_size(const char *text, int font_size) {
  float scale = (float)font_size / FONT_PIXEL_SIZE;
  Sizef size = { .width = 0, .height = 0 };
  int w = 0;
  int h = 0;
  int max_bearing_h = 0;

  // NOTE: I don't like looping through the characters twice, but it's fine for now
  for (uint i = 0; i < strlen(text); i++) {
    Character ch = characters[(uint)text[i]];
    max_bearing_h = max(max_bearing_h, ch.bearing.height);
  }

  for (uint i = 0; i < strlen(text); i++) {
    Character ch = characters[(uint)text[i]];
    w += ch.advance / 64;

    // remove bearing of first character
    if (i == 0 && strlen(text) > 1) {
      w -= ch.bearing.width;
    }

    // remove the trailing width of the last character
    if (i == strlen(text) - 1) {
      w -= ((ch.advance / 64) - (ch.bearing.width + ch.size.width));
    }

    // if we only have one character in the text, then remove the bearing width
    if (strlen(text) == 1) {
      w -= (ch.bearing.width);
    }

    h = max(h, max_bearing_h - ch.bearing.height + ch.size.height);
  }
  size.width = (float)w * scale;
  size.height = (float)h * scale;
  return size;
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
      constraints->width = zephr_context.window_size.width / 1920.f * value;
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
      constraints->height = zephr_context.window_size.height / 1080.f * value;
      break;
    case UI_CONSTRAINT_ASPECT_RATIO:
      constraints->height = constraints->width * value;
      break;
  }
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

void draw_quad(UIConstraints constraints, Color *color, float border_radius, Alignment align) {
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
  apply_rotation(&model, to_radians(0));
  model.m[3][0] = pos.x;
  model.m[3][1] = pos.y;

  set_mat4f(font_shader, "model", (float *)model.m);

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

void draw_circle(UIConstraints constraints, Color *color, Alignment align) {
  float radius = 0.f;

  if (constraints.width > constraints.height) {
    radius = constraints.height / 2.f;
  } else {
    radius = constraints.width / 2.f;
  }
  draw_quad(constraints, color, radius, align);
}

void draw_triangle(UIConstraints constraints, Color *color, Alignment align) {
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
  apply_rotation(&model, to_radians(0));
  model.m[3][0] = pos.x;
  model.m[3][1] = pos.y;

  set_mat4f(font_shader, "model", (float *)model.m);

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

void draw_text(const char* text, int font_size, Vec2f pos, Color *color, Alignment alignment) {
  use_shader(font_shader);
  if (color) {
    set_vec4f(font_shader, "textColor", color->r / 255.f, color->g / 255.f, color->b / 255.f, color->a / 255.f);
  } else {
    set_vec4f(font_shader, "textColor", 0.f, 0.f, 0.f, 1.f);
  }
  set_mat4f(font_shader, "projection", (float *)zephr_context.projection.m);
  Matrix4x4 model = identity();
  model.m[3][0] = pos.x;
  model.m[3][1] = pos.y;

  set_mat4f(font_shader, "model", (float *)model.m);

  Sizef text_size = calculate_text_size(text, FONT_PIXEL_SIZE);
  float font_scale = (float)font_size / FONT_PIXEL_SIZE;

  int max_bearing_h = 0;
  for (uint i = 0; i < strlen(text); i++) {
    Character ch = characters[(int)text[i]];
    max_bearing_h = max(max_bearing_h, ch.bearing.height);
  }

  float first_char_bearing_w = characters[(int)text[0]].bearing.width;

  glActiveTexture(GL_TEXTURE0);
  glBindVertexArray(font_vao);

  int c = 0;
  int x = 0;
  while (text[c] != '\0') {
    Character ch = characters[(int)text[c]];

    // subtract the bearing width of the first character to remove the extra space
    // at the start of the text and move every char to the left by that width
    float xpos = (x + (ch.bearing.width - first_char_bearing_w)) * font_scale;

    float ypos = (text_size.height - ch.bearing.height - (text_size.height - max_bearing_h)) * font_scale;
    Vec2f final_pos = { xpos, ypos };

    float w = (ch.size.width * font_scale);
    float h = (ch.size.height * font_scale);

    apply_alignment(alignment, &final_pos, (Sizef){ text_size.width * font_scale, text_size.height * font_scale });

    float vertices[6][4] = {
      // bottom left tri
      {final_pos.x,     final_pos.y + h, 0.0, 1.0},
      {final_pos.x,     final_pos.y,     0.0, 0.0},
      {final_pos.x + w, final_pos.y,     1.0, 0.0},

      // top right tri
      {final_pos.x,     final_pos.y + h, 0.0, 1.0},
      {final_pos.x + w, final_pos.y,     1.0, 0.0},
      {final_pos.x + w, final_pos.y + h, 1.0, 1.0},
    };

    glBindTexture(GL_TEXTURE_2D, ch.texture_id);

    glBindBuffer(GL_ARRAY_BUFFER, font_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    x += (ch.advance >> 6); 
    c++;
  }

  glBindVertexArray(0);
  glBindTexture(GL_TEXTURE_2D, 0);
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

