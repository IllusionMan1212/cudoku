#include <ft2build.h>
#include FT_FREETYPE_H
#include <freetype/ftmm.h>
#include <glad/gl.h>

#include "font.h"
#include "shader.h"

Character numbers[9];
Character characters[128];

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

  FT_Set_Pixel_Sizes(face, 0, 128);

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  for (int i = 0; i < 9; i++) {
    if (FT_Load_Char(face, '0' + (i + 1), FT_LOAD_RENDER)) {
      printf("[ERROR]: failed to load glyph for number '%c'\n", '0' + i);
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

    numbers[i].texture_id = texture;
    numbers[i].advance = face->glyph->advance.x;
    Size s = { .width = face->glyph->bitmap.width, .height = face->glyph->bitmap.rows };
    numbers[i].size = s;
    Size bearing = { .width = face->glyph->bitmap_left, .height = face->glyph->bitmap_top };
    numbers[i].bearing = bearing;
  }

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
    Size s = { .width = face->glyph->bitmap.width, .height = face->glyph->bitmap.rows };
    characters[c].size = s;
    Size bearing = { .width = face->glyph->bitmap_left, .height = face->glyph->bitmap_top };
    characters[c].bearing = bearing;
  }

  glBindTexture(GL_TEXTURE_2D, 0);

  FT_Done_Face(face);
  FT_Done_FreeType(ft);

  return 0;
}

void prepare_font(unsigned int *vao, unsigned int *vbo) {
  glGenVertexArrays(1, &(*vao));
  glGenBuffers(1, &(*vbo));

  glBindVertexArray(*vao);
  glBindBuffer(GL_ARRAY_BUFFER, *vbo);

  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6*4, NULL, GL_DYNAMIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}

Size calculate_text_size(const char *text, float scale) {
  Size size = { .width = 0, .height = 0 };
  float w = 0;
  float h = 0;
  for (uint i = 0; i < strlen(text); i++) {
    Character ch = characters[(uint)text[i]];
    w += ch.advance / 64.0;
    if (ch.size.height > h) {
      h = ch.size.height;
    }
  }
  size.width = w * scale;
  size.height = h * scale;
  return size;
}

void draw_number(Shader shader, Cell cell, int row, int column, float scale, unsigned int vao, unsigned int vbo, float *transform) {
  use_shader(shader);
  if (cell.is_locked) {
    set_vec3f(shader, "textColor", 44.f / 255.f, 54.f / 255.f, 57.f / 255.f);
  } else {
    set_vec3f(shader, "textColor", 110.f / 255.f, 133.f / 255.f, 183.f / 255.f);
  }
  set_mat4f(shader, "transform", transform);
  glActiveTexture(GL_TEXTURE0);
  glBindVertexArray(vao);

  Character ch = numbers[cell.value - 1];

  float w = ch.size.width;
  float h = ch.size.height;
  float ratio = w / h;

  // draw the glyph over the entire board and then scale it down to the size
  // of a single cell
  float vert_h = 1.0;
  float vert_w = vert_h * ratio;
  // get size of a single cell in opengl coords (-1, 1)
  float single_cell = (1.f/9.f) * 2;
  // we minus 4 since we start at column and row 4 (0-indexed)
  float offset_x = single_cell * (column - 4);
  float offset_y = single_cell * (row - 4);

  // texture coords are y-flipped
  float vertices[6][4] = {
    // top left tri
    { ((-(vert_w) / w * ratio) * scale) + offset_x, (-vert_h / h * scale) - offset_y, 0.0f, 1.0f },
    { ((-(vert_w) / w * ratio) * scale) + offset_x, ( vert_h / h * scale) - offset_y, 0.0f, 0.0f },
    { (( (vert_w) / w * ratio) * scale) + offset_x, ( vert_h / h * scale) - offset_y, 1.0f, 0.0f },

    // bottom right tri
    { ((-(vert_w) / w * ratio) * scale) + offset_x, (-vert_h / h * scale) - offset_y, 0.0f, 1.0f },
    { (( (vert_w) / w * ratio) * scale) + offset_x, ( vert_h / h * scale) - offset_y, 1.0f, 0.0f },
    { (( (vert_w) / w * ratio) * scale) + offset_x, (-vert_h / h * scale) - offset_y, 1.0f, 1.0f }
  };

  glBindTexture(GL_TEXTURE_2D, ch.texture_id);

  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glDrawArrays(GL_TRIANGLES, 0, 6);

  glBindVertexArray(0);
  glBindTexture(GL_TEXTURE_2D, 0);
}

void draw_text_center(Shader shader, const char *text, Size text_size, float scale, unsigned int vao, unsigned int vbo, float *transform, float offset_x, float offset_y) {
  use_shader(shader);
  set_vec3f(shader, "textColor", 200.f / 255.f, 200.f / 255.f, 200.f / 255.f);
  set_mat4f(shader, "transform", transform);
  glActiveTexture(GL_TEXTURE0);
  glBindVertexArray(vao);

  int c = 0;
  int x = 0;
  while (text[c] != '\0') {
    Character ch = characters[(int)text[c]];

    float xpos = ((x + ch.bearing.width * scale + offset_x) / 900.f) - ((text_size.width / 2.f) * scale / 900.f);
    float ypos = (((ch.bearing.height - ch.size.height + offset_y) * scale) / 900.f) - ((text_size.height / 2.f) * scale / 900.f);

    float w = (ch.size.width * scale / 900.f);
    float h = (ch.size.height * scale / 900.f);

    float vertices[6][4] = {
      // top left tri
      { xpos,     ypos + h, 0.0f, 0.0f },
      { xpos,     ypos,     0.0f, 1.0f },
      { xpos + w, ypos,     1.0f, 1.0f },

      // bottom right tri
      { xpos,     ypos + h, 0.0f, 0.0f },
      { xpos + w, ypos,     1.0f, 1.0f },
      { xpos + w, ypos + h, 1.0f, 0.0f }
    };

    glBindTexture(GL_TEXTURE_2D, ch.texture_id);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    x += (ch.advance >> 6) * scale; 
    c++;
  }

  glBindVertexArray(0);
  glBindTexture(GL_TEXTURE_2D, 0);
}

Color default_text_color = { 200.f / 255.f, 200.f / 255.f, 200.f / 255.f, 1.f };

void draw_text_at(Shader shader, const char *text, Vec2 pos, float scale, unsigned int vao, unsigned int vbo, float *projection, Color *color) {
  use_shader(shader);
  if (color != NULL) {
    set_vec3f(shader, "textColor", color->r / 255.f, color->g / 255.f, color->b / 255.f);
  } else {
    set_vec3f(shader, "textColor", 200.f / 255.f, 200.f / 255.f, 200.f / 255.f);
  }
  set_mat4f(shader, "transform", (float *)projection);
  glActiveTexture(GL_TEXTURE0);
  glBindVertexArray(vao);

  int c = 0;
  while (text[c] != '\0') {
    Character ch = characters[(int)text[c]];

    float xpos = pos.x + ch.bearing.width * scale;
    float ypos = pos.y - (ch.size.height - ch.bearing.height) * scale;

    float w = (ch.size.width * scale);
    float h = (ch.size.height * scale);

    float vertices[6][4] = {
      // top left tri
      { xpos,     ypos + h, 0.0f, 0.0f },
      { xpos,     ypos,     0.0f, 1.0f },
      { xpos + w, ypos,     1.0f, 1.0f },

      // bottom right tri
      { xpos,     ypos + h, 0.0f, 0.0f },
      { xpos + w, ypos,     1.0f, 1.0f },
      { xpos + w, ypos + h, 1.0f, 0.0f }
    };

    glBindTexture(GL_TEXTURE_2D, ch.texture_id);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    pos.x += (ch.advance >> 6) * scale; 
    c++;
  }

  glBindVertexArray(0);
  glBindTexture(GL_TEXTURE_2D, 0);
}
