#include <math.h>

#include "helper.h"

Matrix4x4 identity() {
    Matrix4x4 result = {0};

    result.m[0][0] = 1.0f;
    result.m[1][1] = 1.0f;
    result.m[2][2] = 1.0f;
    result.m[3][3] = 1.0f;

    return result;
}

void mult_4x4(Matrix4x4 *multiplicant, Matrix4x4 *multiplier) {
  Matrix4x4 result = identity();

  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      result.m[i][j] = 0;
      for (int k = 0; k < 4; k++) {
        result.m[i][j] += multiplicant->m[i][k] * multiplier->m[k][j];
      }
    }
  }

  *multiplicant = result;
}

void apply_scale(Matrix4x4 *matrix, Sizef scale) {
  Matrix4x4 temp = identity();

  temp.m[0][0] = scale.width;
  temp.m[1][1] = scale.height;

  mult_4x4(matrix, &temp);
}

void apply_rotation(Matrix4x4 *model, float angle) {
  Matrix4x4 temp = identity();

  temp.m[0][0] = cos(angle);
  temp.m[0][1] = sin(angle);
  temp.m[1][0] = -sin(angle);
  temp.m[1][1] = cos(angle);

  mult_4x4(model, &temp);
}

void apply_translation(Matrix4x4 *matrix, Vec2f pos) {
  Matrix4x4 temp = identity();

  temp.m[3][0] = pos.x;
  temp.m[3][1] = pos.y;

  mult_4x4(matrix, &temp);
}

Matrix4x4 orthographic_projection_2d(float left, float right, float bottom, float top) {
    Matrix4x4 result = identity();

    result.m[0][0] = 2.0f / (right - left);
    result.m[3][0] = -(right + left) / (right - left);
    result.m[1][1] = 2.0f / (top - bottom);
    result.m[3][1] = -(top + bottom) / (top - bottom);
    result.m[2][2] = -1.0f;
    result.m[3][3] = 1.0f;

    return result;
}

int max(int a, int b) {
    return a > b ? a : b;
}

float maxf(float a, float b) {
    return a > b ? a : b;
}

float clampf(float value, float min, float max) {
    if (value < min) {
        return min;
    } else if (value > max) {
        return max;
    }

    return value;
}

float to_radians(float degrees) {
  return degrees * (M_PI / 180.f);
}
