#include <stdio.h>
#include <stdlib.h>

#include <glad/gl.h>

#include "shader.h"

void read_shader_file(const char *path, char **buf) {
  FILE *fp = fopen(path, "rb");

  if (!fp) {
    printf("Could not open file %s", path);
    exit(1);
  }

  fseek(fp, 0, SEEK_END);
  long filesize = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  *buf = malloc(filesize + 1);
  (*buf)[filesize] = '\0';


  size_t bytes_read = fread(*buf, filesize, 1, fp);
  if (bytes_read != sizeof(**buf)) {
    printf("Failed to fully read file %s", path);
    exit(1);
  }

  fclose(fp);
}

Shader create_shader(const char *vertex_path, const char *fragment_path) {
  Shader shader;

  char *v_shader_source = NULL;
  char *f_shader_source = NULL;
  read_shader_file(vertex_path, &v_shader_source);
  read_shader_file(fragment_path, &f_shader_source);

  int v_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(v_shader, 1, (const char **)&v_shader_source, NULL);
  glCompileShader(v_shader);
  int success0 = 0;
  glGetShaderiv(v_shader, GL_COMPILE_STATUS, &success0);
  printf("vertex shader %s compiled with status: %d\n", vertex_path, success0);
  if (!success0) {
    char info_buf[512];
    glGetShaderInfoLog(v_shader, 512, NULL, info_buf);
    printf("vertex shader info: %s\n", info_buf);
  }

  int f_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(f_shader, 1, (const char **)&f_shader_source, NULL);
  glCompileShader(f_shader);
  int success1 = 0;
  glGetShaderiv(f_shader, GL_COMPILE_STATUS, &success1);
  printf("frag shader %s compiled with status: %d\n", fragment_path, success1);
  if (!success1) {
    char info_buf[512];
    glGetShaderInfoLog(f_shader, 512, NULL, info_buf);
    printf("frag shader info: %s\n", info_buf);
  }

  int program = glCreateProgram();
  glAttachShader(program, v_shader);
  glAttachShader(program, f_shader);
  glLinkProgram(program);

  glDeleteShader(v_shader);
  glDeleteShader(f_shader);

  free(v_shader_source);
  free(f_shader_source);

  shader.program = program;

  return shader;
}

void use_shader(Shader shader) {
  glUseProgram(shader.program);
}

void set_mat4(Shader shader, const char *name, float *mat4) {
  int loc = glGetUniformLocation(shader.program, name);
  glUniformMatrix4fv(loc, 1, GL_FALSE, mat4);
}

void set_float(Shader shader, const char *name, float val) {
  int loc = glGetUniformLocation(shader.program, name);
  glUniform1f(loc, val);
}
