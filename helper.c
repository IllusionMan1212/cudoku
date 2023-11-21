#include "helper.h"

Matrix4x4 identity() {
    Matrix4x4 result = {0};

    result.m[0][0] = 1.0f;
    result.m[1][1] = 1.0f;
    result.m[2][2] = 1.0f;
    result.m[3][3] = 1.0f;

    return result;
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
