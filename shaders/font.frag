#version 330 core
in vec2 v_TexCoords;
out vec4 FragColor;

uniform sampler2D text;
uniform vec4 textColor;

void main() {
  vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, v_TexCoords).r);
  FragColor = textColor * sampled;
}
