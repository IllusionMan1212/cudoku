#pragma once

typedef struct Shader {
  int program;
} Shader;

void read_shader_file(const char *path, char **buf);
Shader create_shader(const char *vertex_path, const char *fragment_path);
void use_shader(Shader shader);
void set_mat4f(Shader shader, const char *name, float *mat);
void set_float(Shader shader, const char *name, float val);
void set_vec2f(Shader shader, const char *name, float val1, float val2);
void set_vec3f(Shader shader, const char *name, float val1, float val2, float val3);
void set_vec4f(Shader shader, const char *name, float val1, float val2, float val3, float val4);
