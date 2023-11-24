#version 330 core
layout (location = 0) in vec2 vertex;

// per instance
layout (location = 1) in vec4 offset;
layout (location = 2) in int tex_index;

out vec2 v_TexCoords;
uniform mat4 projection;
uniform mat4 model;
// buffer for all the 96 ascii characters' texcoords
uniform vec2 texcoords[96 * 4];

void main() {
  vec2 pos = vec2((vertex.x) * offset.z, (vertex.y) * offset.w);
  gl_Position = projection * model * vec4(pos + offset.xy, 0.0, 1.0);
  v_TexCoords = texcoords[tex_index * 4 + gl_VertexID];
}
