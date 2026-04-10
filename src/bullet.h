/*
 * bullet.h -- Projectile pool and interface
 */

#pragma once

#include "raylib.h"
#include "raymath.h"
#include <stdbool.h>

/* Forward declaration to avoid circular include */
typedef struct Tilemap Tilemap;

/* ── Constants ─────────────────────────────────────────────────────────────── */

#define MAX_BULLETS 128
#define BULLET_SPEED 500.0f
#define BULLET_RADIUS 3.0f
#define BULLET_LIFETIME 2.0f   /* seconds before despawn */
#define PISTOL_FIRE_RATE 0.25f /* seconds between shots (Basic Pistol) */
#define BULLET_MAX_BOUNCES 3   /* max wall bounces before despawn */

/* ── Types ─────────────────────────────────────────────────────────────────── */

typedef struct {
    Vector2 position;
    Vector2 velocity;
    float lifetime;
    int bounces;
    bool active;
} Bullet;

typedef struct {
    Bullet bullets[MAX_BULLETS];
    float fire_cooldown; /* time until next shot allowed */
} BulletPool;

/* ── Public API ────────────────────────────────────────────────────────────── */

void bullet_pool_init(BulletPool *pool);

/*
 * bullet_pool_update -- Move bullets, expire old ones, bounce off walls.
 *
 * Bullets reflect off solid tiles up to BULLET_MAX_BOUNCES times before
 * being deactivated. Pass NULL for tm to skip wall collision checks
 * (e.g. in tests).
 */
void bullet_pool_update(BulletPool *pool, float dt, Rectangle arena, const Tilemap *tm);

void bullet_pool_draw(const BulletPool *pool);

/* Spawn a bullet from `origin` in `direction` (must be normalized).
 * Returns true if a bullet was actually fired, false if rate-limited or
 * the pool is full. */
bool bullet_pool_fire(BulletPool *pool, Vector2 origin, Vector2 direction);
