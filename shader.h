#pragma once

typedef struct Shader {
  int program;
} Shader;

void read_shader_file(const char *path, char **buf);
Shader create_shader(const char *vertex_path, const char *fragment_path);
void use_shader(Shader shader);
