/*
 * bullet.c -- Projectile pool management
 */

#include "bullet.h"
#include "tilemap.h"
#include <string.h>

/* ── Public ────────────────────────────────────────────────────────────────── */

void bullet_pool_init(BulletPool *pool) {
    memset(pool->bullets, 0, sizeof(pool->bullets));
    pool->fire_cooldown = 0.0f;
}

void bullet_pool_update(BulletPool *pool, float dt, Rectangle arena, const Tilemap *tm) {
    /* Tick fire cooldown */
    if (pool->fire_cooldown > 0.0f) {
        pool->fire_cooldown -= dt;
        if (pool->fire_cooldown < 0.0f) {
            pool->fire_cooldown = 0.0f;
        }
    }

    for (int i = 0; i < MAX_BULLETS; i++) {
        Bullet *b = &pool->bullets[i];
        if (!b->active) {
            continue;
        }

        /* Move */
        b->position = Vector2Add(b->position, Vector2Scale(b->velocity, dt));

        /* Age */
        b->lifetime -= dt;

        /* Deactivate if expired or out of arena */
        if (b->lifetime <= 0.0f || b->position.x < arena.x ||
            b->position.x > arena.x + arena.width || b->position.y < arena.y ||
            b->position.y > arena.y + arena.height) {
            b->active = false;
            continue;
        }

        /* Bounce off walls (or deactivate if max bounces reached).
         * Check using BULLET_RADIUS so the circle never visually overlaps a wall. */
        if (tm) {
            float r = BULLET_RADIUS;
            bool hit_right = tilemap_is_solid(tm, b->position.x + r, b->position.y);
            bool hit_left = tilemap_is_solid(tm, b->position.x - r, b->position.y);
            bool hit_bottom = tilemap_is_solid(tm, b->position.x, b->position.y + r);
            bool hit_top = tilemap_is_solid(tm, b->position.x, b->position.y - r);

            if (hit_right || hit_left || hit_bottom || hit_top) {
                if (b->bounces >= BULLET_MAX_BOUNCES) {
                    b->active = false;
                    continue;
                }

                /* Reflect velocity on the axis that hit a wall */
                if (hit_right || hit_left) {
                    b->velocity.x = -b->velocity.x;
                }
                if (hit_bottom || hit_top) {
                    b->velocity.y = -b->velocity.y;
                }

                /* Snap the bullet edge out of the wall */
                if (hit_right) {
                    int tx = (int)(b->position.x + r) / tm->tile_size;
                    b->position.x = (float)(tx * tm->tile_size) - r - 0.01f;
                }
                if (hit_left) {
                    int tx = (int)(b->position.x - r) / tm->tile_size;
                    b->position.x = (float)((tx + 1) * tm->tile_size) + r + 0.01f;
                }
                if (hit_bottom) {
                    int ty = (int)(b->position.y + r) / tm->tile_size;
                    b->position.y = (float)(ty * tm->tile_size) - r - 0.01f;
                }
                if (hit_top) {
                    int ty = (int)(b->position.y - r) / tm->tile_size;
                    b->position.y = (float)((ty + 1) * tm->tile_size) + r + 0.01f;
                }

                b->bounces++;
            }
        }
    }
}

void bullet_pool_draw(const BulletPool *pool) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        const Bullet *b = &pool->bullets[i];
        if (!b->active) {
            continue;
        }
        DrawCircleV(b->position, BULLET_RADIUS, ORANGE);
    }
}

bool bullet_pool_fire(BulletPool *pool, Vector2 origin, Vector2 direction) {
    if (pool->fire_cooldown > 0.0f) {
        return false; /* Rate limited */
    }

    /* Find an inactive slot */
    for (int i = 0; i < MAX_BULLETS; i++) {
        Bullet *b = &pool->bullets[i];
        if (!b->active) {
            b->position = origin;
            b->velocity = Vector2Scale(direction, BULLET_SPEED);
            b->lifetime = BULLET_LIFETIME;
            b->bounces = 0;
            b->active = true;
            pool->fire_cooldown = PISTOL_FIRE_RATE;
            return true;
        }
    }
    /* Pool full -- silently drop the shot */
    return false;
}
