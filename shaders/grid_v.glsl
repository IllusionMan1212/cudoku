#version 330 core
uniform mat4 transform;

vec3 gridPlane[6] = vec3[](
    vec3(1, 1, 0), vec3(-1, -1, 0), vec3(-1, 1, 0),
    vec3(-1, -1, 0), vec3(1, 1, 0), vec3(1, -1, 0)
);

void main() {
  vec3 p = gridPlane[gl_VertexID].xyz;
  gl_Position = vec4(p, 1.0f);
}