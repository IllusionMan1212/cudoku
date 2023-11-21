#pragma once

typedef struct {
    float m[4][4];
} Matrix4x4;

Matrix4x4 identity();
void mult_4x4(Matrix4x4 *multiplicant, Matrix4x4 *multiplier);
void apply_rotation(Matrix4x4 *model, float angle);
Matrix4x4 orthographic_projection_2d(float left, float right, float bottom, float top);
int max(int a, int b);
float maxf(float a, float b);
float clampf(float value, float min, float max);
float to_radians(float degrees);
