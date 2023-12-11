#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include <glad/glx.h>

#include "shader.h"
#include "text.h"
#include "zephr.h"

#define FONT_PIXEL_SIZE 64

Shader font_shader;
unsigned int font_vao;
unsigned int font_instance_vbo;

void extend_glyph_instance_list(GlyphInstanceList *dest, GlyphInstanceList *src) {
  if (dest->size + src->size >= dest->capacity) {
    dest->capacity = dest->size + src->size;
    GlyphInstance* temp = realloc(dest->data, dest->capacity * sizeof(GlyphInstance));
    if (!temp) {
      printf("[FATAL] Failed to reallocate memory for glyph instance list\n");
      exit(1);
    }
    dest->data = temp;
  }

  memcpy(dest->data + dest->size, src->data, src->size * sizeof(GlyphInstance));
  dest->size += src->size;
}

void new_glyph_instance_list(GlyphInstanceList *list, u32 capacity) {
  list->size = 0;
  list->capacity = capacity;
  list->data = malloc(list->capacity * sizeof(GlyphInstance));
}

void clear_glyph_instance_list(GlyphInstanceList *list) {
  list->size = 0;
  memset(list->data, 0, list->size * sizeof(GlyphInstance));
}

void add_glyph_instance(GlyphInstanceList *list, GlyphInstance instance) {
  if (list->size >= list->capacity) {
    list->capacity *= 2;
    GlyphInstance* temp = realloc(list->data, list->capacity * sizeof(GlyphInstance));
    if (!temp) {
      printf("[FATAL] Failed to reallocate memory for glyph instance list\n");
      exit(1);
    }
    list->data = temp;
  }

  list->data[list->size++] = instance;
}

int init_freetype(const char* font_path) {
  FT_Library ft;
  if (FT_Init_FreeType(&ft)) {
    return -1;
  }

  FT_Face face;
  if (FT_New_Face(ft, font_path, 0, &face)) {
    return -2;
  }

  // sets the variable font to be bold
  /* if ((face->face_flags & FT_FACE_FLAG_MULTIPLE_MASTERS)) { */
  /*   printf("[INFO] Got a variable font\n"); */
  /*   FT_MM_Var *mm; */
  /*   FT_Get_MM_Var(face, &mm); */

  /*   FT_Set_Var_Design_Coordinates(face, mm->num_namedstyles, mm->namedstyle[mm->num_namedstyles - 4].coords); */

  /*   FT_Done_MM_Var(ft, mm); */
  /* } */

  FT_Set_Pixel_Sizes(face, 0, FONT_PIXEL_SIZE);

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  int pen_x = 0, pen_y = 0;

  /* FT_UInt glyph_idx; */
  /* FT_ULong c = FT_Get_First_Char(face, &glyph_idx); */

  u32 tex_width = 0, tex_height = 0;
  for (u32 i = 32; i < 128; i++) {
    if (FT_Load_Char(face, i, FT_LOAD_RENDER)) {
      printf("[ERROR]: failed to load glyph for char '0x%x'\n", i);
    }

    /* FT_Render_Glyph(face->glyph, FT_RENDER_MODE_SDF); */

    tex_width += face->glyph->bitmap.width + 1;
    tex_height = CORE_MAX(tex_height, face->glyph->bitmap.rows);
  }

  char *pixels = (char *)calloc(tex_width * tex_height, 1);

  for (u32 i = 32; i < 128; i++) {
  /* while (glyph_idx) { */
    if (FT_Load_Char(face, i, FT_LOAD_RENDER)) {
      printf("[ERROR]: failed to load glyph for char '0x%x'\n", i);
    }

    /* FT_Render_Glyph(face->glyph, FT_RENDER_MODE_SDF); */

    FT_Bitmap *bmp = &face->glyph->bitmap;

    if (pen_x + bmp->width >= tex_width) {
      pen_x = 0;
      pen_y += (1 + (face->size->metrics.height >> 6));
    }

    for (u32 row = 0; row < bmp->rows; row++) {
      for (u32 col = 0; col < bmp->width; col++) {
        int x = pen_x + col;
        int y = pen_y + row;
        pixels[y * tex_width + x] = bmp->buffer[row * bmp->pitch + col];
      }
    }

    float atlas_x0 = (float)pen_x / (float)tex_width;
    float atlas_y0 = (float)pen_y / (float)tex_height;
    float atlas_x1 = (float)(pen_x + bmp->width) / (float)tex_width;
    float atlas_y1 = (float)(pen_y + bmp->rows) / (float)tex_height;

    Vec2f top_left = (Vec2f){ .x = atlas_x0, .y = atlas_y1 };
    Vec2f top_right = (Vec2f){ .x = atlas_x1, .y = atlas_y1 };
    Vec2f bottom_right = (Vec2f){ .x = atlas_x1, .y = atlas_y0 };
    Vec2f bottom_left = (Vec2f){ .x = atlas_x0, .y = atlas_y0 };

    Character character;

    character.tex_coords[0] = top_left;
    character.tex_coords[1] = top_right;
    character.tex_coords[2] = bottom_right;
    character.tex_coords[3] = bottom_left;
    character.advance = face->glyph->advance.x;
    character.size = (Size){ .width = face->glyph->bitmap.width, .height = face->glyph->bitmap.rows };
    character.bearing = (Size){ .width = face->glyph->bitmap_left, .height = face->glyph->bitmap_top };

    zephr_ctx.font.characters[i] = character;

    pen_x += bmp->width + 1;

    /* c = FT_Get_Next_Char(face, c, &glyph_idx); */
  }

  unsigned int texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, tex_width,
      tex_height, 0, GL_RED, GL_UNSIGNED_BYTE,
      pixels);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  zephr_ctx.font.atlas_texture_id = texture;

  glBindTexture(GL_TEXTURE_2D, 0);

  FT_Done_Face(face);
  FT_Done_FreeType(ft);

  free(pixels);

  return 0;
}

int init_fonts(const char *font_path) {
  u32 font_vbo;
  u32 font_ebo;

  int res = init_freetype(font_path);
  if (res != 0) {
    return res;
  }

  font_shader = create_shader("shaders/font.vert", "shaders/font.frag");

  glGenVertexArrays(1, &font_vao);
  glGenBuffers(1, &font_vbo);
  glGenBuffers(1, &font_instance_vbo);
  glGenBuffers(1, &font_ebo);

  float quad_vertices[4][2] = {
    {0.0, 1.0}, // top left
    {1.0, 1.0}, // top right
    {1.0, 0.0}, // bottom right
    {0.0, 0.0}, // bottom left
  };

  int quad_indices[6] = {
    0, 1, 2,
    2, 3, 0
  };

  glBindVertexArray(font_vao);

  // font quad vbo
  glBindBuffer(GL_ARRAY_BUFFER, font_vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);

  // font instance vbo
  glBindBuffer(GL_ARRAY_BUFFER, font_instance_vbo);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(GlyphInstance), (void *)0);
  glVertexAttribDivisor(1, 1);
  glEnableVertexAttribArray(2);
  glVertexAttribIPointer(2, 1, GL_INT, sizeof(GlyphInstance), (void *)sizeof(Vec4f));
  glVertexAttribDivisor(2, 1);
  glEnableVertexAttribArray(3);
  glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(GlyphInstance), (void *)(sizeof(Vec4f) + sizeof(int)));
  glVertexAttribDivisor(3, 1);
  glEnableVertexAttribArray(4);
  glEnableVertexAttribArray(5);
  glEnableVertexAttribArray(6);
  glEnableVertexAttribArray(7);
  glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(GlyphInstance), (void *)(sizeof(Vec4f) * 2 + sizeof(int)));
  glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(GlyphInstance), (void *)(sizeof(Vec4f) * 3 + sizeof(int)));
  glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(GlyphInstance), (void *)(sizeof(Vec4f) * 4 + sizeof(int)));
  glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, sizeof(GlyphInstance), (void *)(sizeof(Vec4f) * 5 + sizeof(int)));
  glVertexAttribDivisor(4, 1);
  glVertexAttribDivisor(5, 1);
  glVertexAttribDivisor(6, 1);
  glVertexAttribDivisor(7, 1);

  // font ebo
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, font_ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quad_indices), quad_indices, GL_STATIC_DRAW);

  use_shader(font_shader);

  for (int i = 32; i < 128; i++) {
    for (int j = 0; j < 4; j++) {
      char text[32];
      snprintf(text, 32, "texcoords[%d]", (i - 32) * 4 + j);
      set_vec2f(font_shader, text, zephr_ctx.font.characters[i].tex_coords[j].x, zephr_ctx.font.characters[i].tex_coords[j].y);
    }
  }

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  return 0;
}

Sizef calculate_text_size(const char *text, int font_size) {
  float scale = (float)font_size / FONT_PIXEL_SIZE;
  Sizef size = { .width = 0, .height = 0 };
  int w = 0;
  int h = 0;
  int max_bearing_h = 0;

  // NOTE: I don't like looping through the characters twice, but it's fine for now
  for (int i = 0; text[i] != '\0'; i++) {
    Character ch = zephr_ctx.font.characters[(u32)text[i]];
    max_bearing_h = CORE_MAX(max_bearing_h, ch.bearing.height);
  }

  for (u64 i = 0; text[i] != '\0'; i++) {
    Character ch = zephr_ctx.font.characters[(u32)text[i]];
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

    h = CORE_MAX(h, max_bearing_h - ch.bearing.height + ch.size.height);
  }
  size.width = (float)w * scale;
  size.height = (float)h * scale;
  return size;
}

GlyphInstanceList get_glyph_instance_list_from_text(const char *text, int font_size, UIConstraints constraints, const Color *color, Alignment alignment) {
  use_shader(font_shader);
  Color text_color = { 0, 0, 0, 1.f };
  if (color) {
    text_color = (Color){ color->r / 255.f, color->g / 255.f, color->b / 255.f, color->a / 255.f };
  }
  set_mat4f(font_shader, "projection", (float *)zephr_ctx.projection.m);

  Vec2f pos = { 0.f, 0.f };
  Sizef size = {0};

  apply_constraints(constraints, &pos, &size);

  Sizef text_size = calculate_text_size(text, FONT_PIXEL_SIZE);
  float font_scale = (float)font_size / FONT_PIXEL_SIZE * size.width;

  apply_alignment(alignment, &pos, (Sizef){ text_size.width * font_scale, text_size.height * font_scale });

  Matrix4x4 model = identity();
  apply_scale(&model, (Sizef){font_scale, font_scale});

  // rotate around the center point of the text
  apply_translation(&model, (Vec2f){-text_size.width * font_scale / 2.f, -text_size.height * font_scale / 2.f});
  apply_rotation(&model, to_radians(constraints.rotation));
  apply_translation(&model, (Vec2f){text_size.width *font_scale / 2.f, text_size.height * font_scale / 2.f});

  apply_translation(&model, pos);

  int max_bearing_h = 0;
  for (int i = 0; (i < text[i]) != '\0'; i++) {
    Character ch = zephr_ctx.font.characters[(int)text[i]];
    max_bearing_h = CORE_MAX(max_bearing_h, ch.bearing.height);
  }

  float first_char_bearing_w = zephr_ctx.font.characters[(int)text[0]].bearing.width;

  GlyphInstanceList glyph_instance_list;
  new_glyph_instance_list(&glyph_instance_list, 16);

  // we use the original text and character sizes in the loop and then we just
  // scale up or down the model matrix to get the desired font size.
  // this way everything works out fine and we get to transform the text using the
  // model matrix
  int c = 0;
  int x = 0;
  while (text[c] != '\0') {
    Character ch = zephr_ctx.font.characters[(int)text[c]];
    // subtract the bearing width of the first character to remove the extra space
    // at the start of the text and move every char to the left by that width
    float xpos = (x + (ch.bearing.width - first_char_bearing_w));
    float ypos = (text_size.height - ch.bearing.height - (text_size.height - max_bearing_h));

    GlyphInstance instance = {
      .position = (Vec4f){xpos, ypos, ch.size.width, ch.size.height},
      .tex_coords_index = (int)text[c] - 32,
      .color = text_color,
      .model = {{0}},
    };

    memcpy(instance.model, model.m, sizeof(float[4][4]));
    
    add_glyph_instance(&glyph_instance_list, instance);

    x += (ch.advance >> 6); 
    c++;
  }

  return glyph_instance_list;
}

void draw_text(const char* text, int font_size, UIConstraints constraints, const Color *color, Alignment alignment) {
  GlyphInstanceList glyph_instance_list = get_glyph_instance_list_from_text(text, font_size, constraints, color, alignment);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, zephr_ctx.font.atlas_texture_id);
  glBindVertexArray(font_vao);

  glBindBuffer(GL_ARRAY_BUFFER, font_instance_vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GlyphInstance) * glyph_instance_list.size, glyph_instance_list.data, GL_DYNAMIC_DRAW);

  glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, NULL, glyph_instance_list.size);

  glBindVertexArray(0);
  glBindTexture(GL_TEXTURE_2D, 0);

  free(glyph_instance_list.data);
}

void draw_text_batch(GlyphInstanceList *batch) {
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, zephr_ctx.font.atlas_texture_id);
  glBindVertexArray(font_vao);

  glBindBuffer(GL_ARRAY_BUFFER, font_instance_vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GlyphInstance) * batch->size, batch->data, GL_DYNAMIC_DRAW);

  glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, NULL, batch->size);

  glBindVertexArray(0);
  glBindTexture(GL_TEXTURE_2D, 0);

  free(batch->data);
}

void add_text_instance(GlyphInstanceList *batch, const char* text, int font_size, UIConstraints constraints, const Color *color, Alignment alignment) {
  GlyphInstanceList glyph_instance_list = get_glyph_instance_list_from_text(text, font_size, constraints, color, alignment);

  extend_glyph_instance_list(batch, &glyph_instance_list);

  free(glyph_instance_list.data);
}
