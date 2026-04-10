/*
 * enemy.c -- Enemy pool management and behavior
 */

#include "enemy.h"
#include "tilemap.h"
#include <math.h>
#include <string.h>

/* ── Helpers ───────────────────────────────────────────────────────────────── */

static void clamp_to_arena(Vector2 *pos, float radius, Rectangle arena) {
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

/*
 * resolve_tile_collision_enemy -- push enemy out of solid tiles.
 * Same axis-separation approach as the player version.
 */
static void resolve_tile_collision_enemy(Vector2 *pos, float radius, const Tilemap *tm) {
    if (!tm) {
        return;
    }

    /* Check right edge */
    if (tilemap_is_solid(tm, pos->x + radius, pos->y)) {
        int tx = (int)floorf((pos->x + radius) / (float)tm->tile_size);
        pos->x = (float)(tx * tm->tile_size) - radius - 0.01f;
    }
    /* Check left edge */
    if (tilemap_is_solid(tm, pos->x - radius, pos->y)) {
        int tx = (int)floorf((pos->x - radius) / (float)tm->tile_size);
        pos->x = (float)((tx + 1) * tm->tile_size) + radius + 0.01f;
    }
    /* Check bottom edge */
    if (tilemap_is_solid(tm, pos->x, pos->y + radius)) {
        int ty = (int)floorf((pos->y + radius) / (float)tm->tile_size);
        pos->y = (float)(ty * tm->tile_size) - radius - 0.01f;
    }
    /* Check top edge */
    if (tilemap_is_solid(tm, pos->x, pos->y - radius)) {
        int ty = (int)floorf((pos->y - radius) / (float)tm->tile_size);
        pos->y = (float)((ty + 1) * tm->tile_size) + radius + 0.01f;
    }
}

/* ── Public ────────────────────────────────────────────────────────────────── */

void enemy_pool_init(EnemyPool *pool) {
    memset(pool->enemies, 0, sizeof(pool->enemies));
}

void enemy_pool_spawn(EnemyPool *pool, EnemyType type, Vector2 position) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy *e = &pool->enemies[i];
        if (!e->active) {
            e->position = position;
            e->velocity = (Vector2){0.0f, 0.0f};
            e->type = type;
            e->active = true;

            switch (type) {
            case ENEMY_SWARMER:
                e->hp = SWARMER_HP;
                e->radius = SWARMER_RADIUS;
                e->speed = SWARMER_SPEED;
                e->damage = SWARMER_DAMAGE;
                break;
            }
            return;
        }
    }
    /* Pool full -- silently drop the spawn */
}

void enemy_pool_update(EnemyPool *pool, float dt, Vector2 target, Rectangle arena,
                       const Tilemap *tm) {
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
        resolve_tile_collision_enemy(&e->position, e->radius, tm);
    }
}

void enemy_pool_draw(const EnemyPool *pool) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        const Enemy *e = &pool->enemies[i];
        if (!e->active) {
            continue;
        }

        switch (e->type) {
        case ENEMY_SWARMER: {
            Vector2 pos = e->position;
            float r = e->radius;

            /* Facing direction from velocity (fall back to down if stationary) */
            Vector2 facing = {0.0f, 1.0f};
            float spd = Vector2Length(e->velocity);
            if (spd > 1.0f) {
                facing = Vector2Scale(e->velocity, 1.0f / spd);
            }
            Vector2 perp = {-facing.y, facing.x};

            /* Colors */
            Color body_fill = (Color){80, 10, 10, 255};
            Color body_outline = (Color){255, 60, 30, 255};
            Color glow = (Color){255, 40, 20, 40};
            Color eye_color = (Color){255, 220, 50, 255};

            /* Outer glow */
            DrawCircleV(pos, r + 3.0f, glow);

            /* Body fill */
            DrawCircleV(pos, r, body_fill);

            /* Neon outline */
            DrawCircleLinesV(pos, r, body_outline);

            /* Mandible lines (V shape pointing in facing direction) */
            Vector2 jaw_base_l = Vector2Add(
                pos, Vector2Add(Vector2Scale(facing, r * 0.3f), Vector2Scale(perp, r * 0.4f)));
            Vector2 jaw_base_r = Vector2Add(
                pos, Vector2Add(Vector2Scale(facing, r * 0.3f), Vector2Scale(perp, -r * 0.4f)));
            Vector2 jaw_tip = Vector2Add(pos, Vector2Scale(facing, r + 2.0f));
            DrawLineEx(jaw_base_l, jaw_tip, 1.5f, body_outline);
            DrawLineEx(jaw_base_r, jaw_tip, 1.5f, body_outline);

            /* Eyes (two small bright dots) */
            Vector2 eye_l = Vector2Add(
                pos, Vector2Add(Vector2Scale(facing, r * 0.35f), Vector2Scale(perp, r * 0.35f)));
            Vector2 eye_r = Vector2Add(
                pos, Vector2Add(Vector2Scale(facing, r * 0.35f), Vector2Scale(perp, -r * 0.35f)));
            DrawCircleV(eye_l, 1.5f, eye_color);
            DrawCircleV(eye_r, 1.5f, eye_color);
            break;
        }
        }
    }
}

int enemy_pool_active_count(const EnemyPool *pool) {
    int count = 0;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (pool->enemies[i].active) {
            count++;
        }
    }
    return count;
}

/* ── Collision helpers ─────────────────────────────────────────────────────── */

bool check_circle_collision(Vector2 pos_a, float radius_a, Vector2 pos_b, float radius_b) {
    float dx = pos_a.x - pos_b.x;
    float dy = pos_a.y - pos_b.y;
    float dist_sq = dx * dx + dy * dy;
    float radii = radius_a + radius_b;
    return dist_sq <= radii * radii;
}
