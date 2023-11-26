#pragma once

typedef struct {
    float m[4][4];
} Matrix4x4;

typedef struct Size {
  int width;
  int height;
} Size;

typedef struct Sizef {
  float width;
  float height;
} Sizef;

typedef struct Vec2 {
  int x;
  int y;
} Vec2;

typedef struct Vec2f {
  float x;
  float y;
} Vec2f;

typedef struct Vec4f {
  float x;
  float y;
  float z;
  float w;
} Vec4f;

typedef struct Color {
  float r;
  float g;
  float b;
  float a;
} Color;
