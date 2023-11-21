#version 330 core

in vec2 texCoords;
out vec4 FragColor;
uniform vec4 aColor;

void main() {
  FragColor = vec4(aColor);
}
