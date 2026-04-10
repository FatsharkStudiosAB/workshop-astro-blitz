/*
 * vec2.c -- 2D vector math implementation.
 */

#include "vec2.h"

Vec2 vec2_add(Vec2 a, Vec2 b) {
    return (Vec2){a.x + b.x, a.y + b.y};
}

Vec2 vec2_sub(Vec2 a, Vec2 b) {
    return (Vec2){a.x - b.x, a.y - b.y};
}

Vec2 vec2_scale(Vec2 v, float s) {
    return (Vec2){v.x * s, v.y * s};
}

float vec2_length(Vec2 v) {
    return sqrtf(v.x * v.x + v.y * v.y);
}

Vec2 vec2_normalize(Vec2 v) {
    float len = vec2_length(v);
    if (len < 1e-8f) {
        return (Vec2){0.0f, 0.0f};
    }
    return (Vec2){v.x / len, v.y / len};
}

float vec2_distance(Vec2 a, Vec2 b) {
    return vec2_length(vec2_sub(a, b));
}

float vec2_dot(Vec2 a, Vec2 b) {
    return a.x * b.x + a.y * b.y;
}

Vec2 vec2_lerp(Vec2 a, Vec2 b, float t) {
    return (Vec2){a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t};
}
