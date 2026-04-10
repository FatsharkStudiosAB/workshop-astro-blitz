/*
 * bullet.h -- Projectile pool and interface
 */

#pragma once

#include "raylib.h"
#include "raymath.h"
#include <stdbool.h>

/* ── Constants ─────────────────────────────────────────────────────────────── */

#define MAX_BULLETS 128
#define BULLET_SPEED 500.0f
#define BULLET_RADIUS 3.0f
#define BULLET_LIFETIME 2.0f   /* seconds before despawn */
#define PISTOL_FIRE_RATE 0.25f /* seconds between shots (Basic Pistol) */

/* ── Types ─────────────────────────────────────────────────────────────────── */

typedef struct {
    Vector2 position;
    Vector2 velocity;
    float lifetime;
    bool active;
} Bullet;

typedef struct {
    Bullet bullets[MAX_BULLETS];
    float fire_cooldown; /* time until next shot allowed */
} BulletPool;

/* ── Public API ────────────────────────────────────────────────────────────── */

void bullet_pool_init(BulletPool *pool);
void bullet_pool_update(BulletPool *pool, float dt, Rectangle arena);
void bullet_pool_draw(const BulletPool *pool);

/* Spawn a bullet from `origin` in `direction` (must be normalized). */
void bullet_pool_fire(BulletPool *pool, Vector2 origin, Vector2 direction);
