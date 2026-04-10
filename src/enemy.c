/*
 * enemy.c -- Enemy pool management and behavior
 */

#include "enemy.h"
#include <string.h>
#include <math.h>

/* ── Helpers ───────────────────────────────────────────────────────────────── */

static void clamp_to_arena(Vector2 *pos, float radius, Rectangle arena)
{
    float min_x = arena.x + radius;
    float max_x = arena.x + arena.width - radius;
    float min_y = arena.y + radius;
    float max_y = arena.y + arena.height - radius;

    if (pos->x < min_x) {
        pos->x = min_x;
    }
    if (pos->x > max_x) {
        pos->x = max_x;
    }
    if (pos->y < min_y) {
        pos->y = min_y;
    }
    if (pos->y > max_y) {
        pos->y = max_y;
    }
}

/* ── Public ────────────────────────────────────────────────────────────────── */

void enemy_pool_init(EnemyPool *pool)
{
    memset(pool->enemies, 0, sizeof(pool->enemies));
}

void enemy_pool_spawn(EnemyPool *pool, EnemyType type, Vector2 position)
{
    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy *e = &pool->enemies[i];
        if (!e->active) {
            e->position = position;
            e->velocity = (Vector2){0.0f, 0.0f};
            e->type     = type;
            e->active   = true;

            switch (type) {
            case ENEMY_SWARMER:
                e->hp     = SWARMER_HP;
                e->radius = SWARMER_RADIUS;
                e->speed  = SWARMER_SPEED;
                e->damage = SWARMER_DAMAGE;
                break;
            }
            return;
        }
    }
    /* Pool full -- silently drop the spawn */
}

void enemy_pool_update(EnemyPool *pool, float dt, Vector2 target,
                       Rectangle arena)
{
    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy *e = &pool->enemies[i];
        if (!e->active) {
            continue;
        }

        /* Seek toward target */
        Vector2 diff = Vector2Subtract(target, e->position);
        float dist = Vector2Length(diff);

        if (dist > 1.0f) {
            Vector2 dir = Vector2Scale(diff, 1.0f / dist);
            e->velocity = Vector2Scale(dir, e->speed);
        } else {
            e->velocity = (Vector2){0.0f, 0.0f};
        }

        e->position = Vector2Add(e->position, Vector2Scale(e->velocity, dt));
        clamp_to_arena(&e->position, e->radius, arena);
    }
}

void enemy_pool_draw(const EnemyPool *pool)
{
    for (int i = 0; i < MAX_ENEMIES; i++) {
        const Enemy *e = &pool->enemies[i];
        if (!e->active) {
            continue;
        }

        switch (e->type) {
        case ENEMY_SWARMER:
            DrawCircleV(e->position, e->radius, RED);
            break;
        }
    }
}

int enemy_pool_active_count(const EnemyPool *pool)
{
    int count = 0;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (pool->enemies[i].active) {
            count++;
        }
    }
    return count;
}

/* ── Collision helpers ─────────────────────────────────────────────────────── */

bool check_circle_collision(Vector2 pos_a, float radius_a,
                            Vector2 pos_b, float radius_b)
{
    float dx = pos_a.x - pos_b.x;
    float dy = pos_a.y - pos_b.y;
    float dist_sq = dx * dx + dy * dy;
    float radii = radius_a + radius_b;
    return dist_sq <= radii * radii;
}
