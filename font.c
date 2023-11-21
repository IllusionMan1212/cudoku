/* #include <glad/gl.h> */

/* #include "font.h" */
/* #include "shader.h" */

/* void draw_number(Shader shader, Cell cell, int row, int column, float scale, unsigned int vao, unsigned int vbo, float *transform) { */
/*   use_shader(shader); */
/*   if (cell.is_locked) { */
/*     set_vec3f(shader, "textColor", 44.f / 255.f, 54.f / 255.f, 57.f / 255.f); */
/*   } else { */
/*     set_vec3f(shader, "textColor", 110.f / 255.f, 133.f / 255.f, 183.f / 255.f); */
/*   } */
/*   set_mat4f(shader, "transform", transform); */
/*   glActiveTexture(GL_TEXTURE0); */
/*   glBindVertexArray(vao); */

/*   // get ascii value of cell value and use it to index into characters */
/*   Character ch = characters['0' + cell.value]; */

/*   float w = ch.size.width; */
/*   float h = ch.size.height; */
/*   float ratio = w / h; */

/*   // draw the glyph over the entire board and then scale it down to the size */
/*   // of a single cell */
/*   float vert_h = 1.0; */
/*   float vert_w = vert_h * ratio; */
/*   // get size of a single cell in opengl coords (-1, 1) */
/*   float single_cell = (1.f/9.f) * 2; */
/*   // we minus 4 since we start at column and row 4 (0-indexed) */
/*   float offset_x = single_cell * (column - 4); */
/*   float offset_y = single_cell * (row - 4); */

/*   // texture coords are y-flipped */
/*   float vertices[6][4] = { */
/*     // top left tri */
/*     { ((-(vert_w) / w * ratio) * scale) + offset_x, (-vert_h / h * scale) - offset_y, 0.0f, 1.0f }, */
/*     { ((-(vert_w) / w * ratio) * scale) + offset_x, ( vert_h / h * scale) - offset_y, 0.0f, 0.0f }, */
/*     { (( (vert_w) / w * ratio) * scale) + offset_x, ( vert_h / h * scale) - offset_y, 1.0f, 0.0f }, */

/*     // bottom right tri */
/*     { ((-(vert_w) / w * ratio) * scale) + offset_x, (-vert_h / h * scale) - offset_y, 0.0f, 1.0f }, */
/*     { (( (vert_w) / w * ratio) * scale) + offset_x, ( vert_h / h * scale) - offset_y, 1.0f, 0.0f }, */
/*     { (( (vert_w) / w * ratio) * scale) + offset_x, (-vert_h / h * scale) - offset_y, 1.0f, 1.0f } */
/*   }; */

/*   glBindTexture(GL_TEXTURE_2D, ch.texture_id); */

/*   glBindBuffer(GL_ARRAY_BUFFER, vbo); */
/*   glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); */

/*   glBindBuffer(GL_ARRAY_BUFFER, 0); */

/*   glDrawArrays(GL_TRIANGLES, 0, 6); */

/*   glBindVertexArray(0); */
/*   glBindTexture(GL_TEXTURE_2D, 0); */
/* } */

/* void draw_text_center(Shader shader, const char *text, Size text_size, float scale, unsigned int vao, unsigned int vbo, float *transform, float offset_x, float offset_y) { */
/*   use_shader(shader); */
/*   set_vec3f(shader, "textColor", 200.f / 255.f, 200.f / 255.f, 200.f / 255.f); */
/*   set_mat4f(shader, "transform", transform); */
/*   glActiveTexture(GL_TEXTURE0); */
/*   glBindVertexArray(vao); */

/*   int c = 0; */
/*   int x = 0; */
/*   while (text[c] != '\0') { */
/*     Character ch = characters[(int)text[c]]; */

/*     float xpos = ((x + ch.bearing.width * scale + offset_x) / 900.f) - ((text_size.width / 2.f) * scale / 900.f); */
/*     float ypos = (((ch.bearing.height - ch.size.height + offset_y) * scale) / 900.f) - ((text_size.height / 2.f) * scale / 900.f); */

/*     float w = (ch.size.width * scale / 900.f); */
/*     float h = (ch.size.height * scale / 900.f); */

/*     float vertices[6][4] = { */
/*       // top left tri */
/*       { xpos,     ypos + h, 0.0f, 0.0f }, */
/*       { xpos,     ypos,     0.0f, 1.0f }, */
/*       { xpos + w, ypos,     1.0f, 1.0f }, */

/*       // bottom right tri */
/*       { xpos,     ypos + h, 0.0f, 0.0f }, */
/*       { xpos + w, ypos,     1.0f, 1.0f }, */
/*       { xpos + w, ypos + h, 1.0f, 0.0f } */
/*     }; */

/*     glBindTexture(GL_TEXTURE_2D, ch.texture_id); */

/*     glBindBuffer(GL_ARRAY_BUFFER, vbo); */
/*     glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); */
/*     glBindBuffer(GL_ARRAY_BUFFER, 0); */

/*     glDrawArrays(GL_TRIANGLES, 0, 6); */

/*     x += (ch.advance >> 6) * scale; */ 
/*     c++; */
/*   } */

/*   glBindVertexArray(0); */
/*   glBindTexture(GL_TEXTURE_2D, 0); */
/* } */

/* Color default_text_color = { 200.f / 255.f, 200.f / 255.f, 200.f / 255.f, 1.f }; */

/* void draw_text_at(Shader shader, const char *text, Vec2 pos, float scale, unsigned int vao, unsigned int vbo, float *projection, Color *color) { */
/*   use_shader(shader); */
/*   if (color != NULL) { */
/*     set_vec3f(shader, "textColor", color->r / 255.f, color->g / 255.f, color->b / 255.f); */
/*   } else { */
/*     set_vec3f(shader, "textColor", 200.f / 255.f, 200.f / 255.f, 200.f / 255.f); */
/*   } */
/*   set_mat4f(shader, "transform", (float *)projection); */
/*   glActiveTexture(GL_TEXTURE0); */
/*   glBindVertexArray(vao); */

/*   int c = 0; */
/*   while (text[c] != '\0') { */
/*     Character ch = characters[(int)text[c]]; */

/*     float xpos = pos.x + ch.bearing.width * scale; */
/*     float ypos = pos.y - (ch.size.height - ch.bearing.height) * scale; */

/*     float w = (ch.size.width * scale); */
/*     float h = (ch.size.height * scale); */

/*     float vertices[6][4] = { */
/*       // top left tri */
/*       { xpos,     ypos + h, 0.0f, 0.0f }, */
/*       { xpos,     ypos,     0.0f, 1.0f }, */
/*       { xpos + w, ypos,     1.0f, 1.0f }, */

/*       // bottom right tri */
/*       { xpos,     ypos + h, 0.0f, 0.0f }, */
/*       { xpos + w, ypos,     1.0f, 1.0f }, */
/*       { xpos + w, ypos + h, 1.0f, 0.0f } */
/*     }; */

/*     glBindTexture(GL_TEXTURE_2D, ch.texture_id); */

/*     glBindBuffer(GL_ARRAY_BUFFER, vbo); */
/*     glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); */
/*     glBindBuffer(GL_ARRAY_BUFFER, 0); */

/*     glDrawArrays(GL_TRIANGLES, 0, 6); */

/*     pos.x += (ch.advance >> 6) * scale; */ 
/*     c++; */
/*   } */

/*   glBindVertexArray(0); */
/*   glBindTexture(GL_TEXTURE_2D, 0); */
/* } */
