/*
 * bullet.c -- Projectile pool management
 */

#include "bullet.h"
#include <string.h>

/* ── Public ────────────────────────────────────────────────────────────────── */

void bullet_pool_init(BulletPool *pool) {
    memset(pool->bullets, 0, sizeof(pool->bullets));
    pool->fire_cooldown = 0.0f;
}

void bullet_pool_update(BulletPool *pool, float dt, Rectangle arena) {
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

void bullet_pool_fire(BulletPool *pool, Vector2 origin, Vector2 direction) {
    if (pool->fire_cooldown > 0.0f) {
        return; /* Rate limited */
    }

    /* Find an inactive slot */
    for (int i = 0; i < MAX_BULLETS; i++) {
        Bullet *b = &pool->bullets[i];
        if (!b->active) {
            b->position = origin;
            b->velocity = Vector2Scale(direction, BULLET_SPEED);
            b->lifetime = BULLET_LIFETIME;
            b->active = true;
            pool->fire_cooldown = PISTOL_FIRE_RATE;
            return;
        }
    }
    /* Pool full -- silently drop the shot */
}
