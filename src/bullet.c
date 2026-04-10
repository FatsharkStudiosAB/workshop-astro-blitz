/*
 * bullet.c -- Projectile pool management
 */

#include "bullet.h"
#include "tilemap.h"
#include <math.h>
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
        DrawCircleV(b->position, BULLET_RADIUS, b->color);
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
            b->damage = 1.0f;
            b->color = ORANGE;
            b->bounces = 0;
            b->active = true;
            pool->fire_cooldown = PISTOL_FIRE_RATE;
            return true;
        }
    }
    /* Pool full -- silently drop the shot */
    return false;
}

int bullet_pool_fire_weapon(BulletPool *pool, Vector2 origin, Vector2 direction, float fire_rate,
                            float damage, float bullet_speed, float spread_angle,
                            int projectile_count, float bullet_lifetime, Color bullet_color) {
    if (pool->fire_cooldown > 0.0f) {
        return 0; /* Rate limited */
    }

    int spawned = 0;
    float half_spread = spread_angle * 0.5f * DEG2RAD;

    for (int i = 0; i < projectile_count; i++) {
        /* Calculate spread offset for this projectile */
        float angle_offset = 0.0f;
        if (projectile_count > 1) {
            /* Evenly distribute across the spread cone */
            float t = (float)i / (float)(projectile_count - 1); /* 0..1 */
            angle_offset = -half_spread + t * 2.0f * half_spread;
        } else if (spread_angle > 0.0f) {
            /* Single projectile with spread = random offset within cone */
            float rand_t = (float)GetRandomValue(-1000, 1000) / 1000.0f;
            angle_offset = rand_t * half_spread;
        }

        /* Rotate direction by angle_offset */
        float cos_a = cosf(angle_offset);
        float sin_a = sinf(angle_offset);
        Vector2 dir = {
            direction.x * cos_a - direction.y * sin_a,
            direction.x * sin_a + direction.y * cos_a,
        };

        /* Find an inactive slot */
        bool placed = false;
        for (int j = 0; j < MAX_BULLETS; j++) {
            Bullet *b = &pool->bullets[j];
            if (!b->active) {
                b->position = origin;
                b->velocity = Vector2Scale(dir, bullet_speed);
                b->lifetime = bullet_lifetime;
                b->damage = damage;
                b->color = bullet_color;
                b->bounces = 0;
                b->active = true;
                spawned++;
                placed = true;
                break;
            }
        }
        if (!placed) {
            break; /* Pool full */
        }
    }

    if (spawned > 0) {
        pool->fire_cooldown = fire_rate;
    }
    return spawned;
}
