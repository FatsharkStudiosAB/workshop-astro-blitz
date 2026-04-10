/*
 * vec2.h -- 2D vector math utilities.
 *
 * Provides a Vec2 type and common operations needed by all game systems:
 * movement, physics, collision detection, AI targeting, etc.
 *
 * All functions are pure (no side effects, no global state).
 */

#pragma once

/* A 2D vector with float components. */
typedef struct {
    float x;
    float y;
} Vec2;

/* Add two vectors: (a.x + b.x, a.y + b.y). */
Vec2 vec2_add(Vec2 a, Vec2 b);

/* Subtract b from a: (a.x - b.x, a.y - b.y). */
Vec2 vec2_sub(Vec2 a, Vec2 b);

/* Scale a vector by a scalar: (v.x * s, v.y * s). */
Vec2 vec2_scale(Vec2 v, float s);

/* Return the length (magnitude) of a vector. */
float vec2_length(Vec2 v);

/* Return a unit vector in the same direction. Returns (0, 0) when the input length is smaller than 1e-8. */
Vec2 vec2_normalize(Vec2 v);

/* Return the distance between two points. */
float vec2_distance(Vec2 a, Vec2 b);

/* Return the dot product of two vectors. */
float vec2_dot(Vec2 a, Vec2 b);

/* Linearly interpolate between a and b by t (0.0 = a, 1.0 = b). */
Vec2 vec2_lerp(Vec2 a, Vec2 b, float t);
