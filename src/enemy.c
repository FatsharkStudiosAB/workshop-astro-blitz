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
            e->hit_flash = 0.0f;
            e->shoot_cooldown = 0.0f;
            e->ai_timer = 0.0f;
            e->is_charging = false;

            switch (type) {
            case ENEMY_SWARMER:
                e->hp = SWARMER_HP;
                e->radius = SWARMER_RADIUS;
                e->speed = SWARMER_SPEED;
                e->damage = SWARMER_DAMAGE;
                break;
            case ENEMY_GRUNT:
                e->hp = GRUNT_HP;
                e->radius = GRUNT_RADIUS;
                e->speed = GRUNT_SPEED;
                e->damage = GRUNT_DAMAGE;
                e->shoot_cooldown = GRUNT_SHOOT_COOLDOWN * 0.5f; /* first shot sooner */
                break;
            case ENEMY_STALKER:
                e->hp = STALKER_HP;
                e->radius = STALKER_RADIUS;
                e->speed = STALKER_SPEED;
                e->damage = STALKER_DAMAGE;
                e->ai_timer = STALKER_DASH_INTERVAL;
                break;
            case ENEMY_BOMBER:
                e->hp = BOMBER_HP;
                e->radius = BOMBER_RADIUS;
                e->speed = BOMBER_SPEED;
                e->damage = BOMBER_DAMAGE;
                break;
            default:
                break;
            }
            return;
        }
    }
    /* Pool full -- silently drop the spawn */
}

/* Compute a direction toward target using flow field if available, else direct. */
static Vector2 compute_seek_direction(const Enemy *e, Vector2 target, const Tilemap *tm) {
    Vector2 diff = Vector2Subtract(target, e->position);
    float dist = Vector2Length(diff);

    if (dist < 1.0f) {
        return (Vector2){0.0f, 0.0f};
    }

    /* Use flow field when tilemap is available and enemy is far enough */
    if (tm != NULL && dist > TILE_SIZE * 3.0f) {
        int tx, ty;
        tilemap_world_to_tile(tm, e->position.x, e->position.y, &tx, &ty);

        if (tx >= 0 && tx < tm->cols && ty >= 0 && ty < tm->rows &&
            tm->flow_dist[ty][tx] != FLOW_UNREACHABLE) {
            Vector2 flow_dir = tm->flow[ty][tx];
            if (flow_dir.x != 0.0f || flow_dir.y != 0.0f) {
                return flow_dir;
            }
        }
    }

    /* Fallback: direct seek */
    return Vector2Scale(diff, 1.0f / dist);
}

static void update_swarmer(Enemy *e, float dt, Vector2 target, Rectangle arena, const Tilemap *tm) {
    Vector2 dir = compute_seek_direction(e, target, tm);
    e->velocity = Vector2Scale(dir, e->speed);
    e->position = Vector2Add(e->position, Vector2Scale(e->velocity, dt));
    clamp_to_arena(&e->position, e->radius, arena);
    resolve_tile_collision_enemy(&e->position, e->radius, tm);
}

static void update_grunt(Enemy *e, float dt, Vector2 target, Rectangle arena, const Tilemap *tm,
                         EnemyBulletPool *ebp) {
    Vector2 diff = Vector2Subtract(target, e->position);
    float dist = Vector2Length(diff);

    /* Move toward preferred range: approach if far, retreat if too close */
    if (dist > GRUNT_PREFERRED_RANGE + 20.0f) {
        Vector2 dir = compute_seek_direction(e, target, tm);
        e->velocity = Vector2Scale(dir, e->speed);
    } else if (dist < GRUNT_PREFERRED_RANGE - 20.0f && dist > 1.0f) {
        /* Back away */
        Vector2 away = Vector2Scale(diff, -1.0f / dist);
        e->velocity = Vector2Scale(away, e->speed * 0.6f);
    } else {
        e->velocity = (Vector2){0.0f, 0.0f};
    }

    e->position = Vector2Add(e->position, Vector2Scale(e->velocity, dt));
    clamp_to_arena(&e->position, e->radius, arena);
    resolve_tile_collision_enemy(&e->position, e->radius, tm);

    /* Shoot at player */
    e->shoot_cooldown -= dt;
    if (e->shoot_cooldown <= 0.0f && ebp != NULL && dist > 1.0f) {
        Vector2 shoot_dir = Vector2Scale(diff, 1.0f / dist);
        enemy_bullet_pool_fire(ebp, e->position, shoot_dir, GRUNT_BULLET_SPEED,
                               GRUNT_BULLET_DAMAGE);
        e->shoot_cooldown = GRUNT_SHOOT_COOLDOWN;
    }
}

static void update_stalker(Enemy *e, float dt, Vector2 target, Rectangle arena, const Tilemap *tm) {
    Vector2 diff = Vector2Subtract(target, e->position);
    float dist = Vector2Length(diff);

    e->ai_timer -= dt;

    if (e->is_charging) {
        /* Dashing: move in current velocity direction */
        e->position = Vector2Add(e->position, Vector2Scale(e->velocity, dt));
        clamp_to_arena(&e->position, e->radius, arena);
        resolve_tile_collision_enemy(&e->position, e->radius, tm);

        if (e->ai_timer <= 0.0f) {
            e->is_charging = false;
            e->ai_timer = STALKER_DASH_INTERVAL;
        }
        return;
    }

    /* Circle the player: move perpendicular to player direction */
    if (dist > 1.0f) {
        Vector2 to_player = Vector2Scale(diff, 1.0f / dist);
        /* Perpendicular (circle clockwise) */
        Vector2 perp = {-to_player.y, to_player.x};

        /* Also approach/retreat to maintain circle range */
        float range_diff = dist - STALKER_CIRCLE_RANGE;
        Vector2 radial = Vector2Scale(to_player, range_diff * 0.02f);

        Vector2 dir = Vector2Add(perp, radial);
        float dir_len = Vector2Length(dir);
        if (dir_len > 0.0f) {
            dir = Vector2Scale(dir, 1.0f / dir_len);
        }

        e->velocity = Vector2Scale(dir, e->speed);
    } else {
        e->velocity = (Vector2){0.0f, 0.0f};
    }

    e->position = Vector2Add(e->position, Vector2Scale(e->velocity, dt));
    clamp_to_arena(&e->position, e->radius, arena);
    resolve_tile_collision_enemy(&e->position, e->radius, tm);

    /* Initiate dash toward player */
    if (e->ai_timer <= 0.0f && dist > 1.0f) {
        e->is_charging = true;
        Vector2 dash_dir = Vector2Scale(diff, 1.0f / dist);
        e->velocity = Vector2Scale(dash_dir, STALKER_DASH_SPEED);
        e->ai_timer = STALKER_DASH_DURATION;
    }
}

static void update_bomber(Enemy *e, float dt, Vector2 target, Rectangle arena, const Tilemap *tm) {
    Vector2 diff = Vector2Subtract(target, e->position);
    float dist = Vector2Length(diff);

    if (e->is_charging) {
        /* Charging: rush toward player at high speed */
        e->position = Vector2Add(e->position, Vector2Scale(e->velocity, dt));
        clamp_to_arena(&e->position, e->radius, arena);
        resolve_tile_collision_enemy(&e->position, e->radius, tm);
        return;
    }

    /* Normal movement: approach player */
    Vector2 dir = compute_seek_direction(e, target, tm);
    e->velocity = Vector2Scale(dir, e->speed);
    e->position = Vector2Add(e->position, Vector2Scale(e->velocity, dt));
    clamp_to_arena(&e->position, e->radius, arena);
    resolve_tile_collision_enemy(&e->position, e->radius, tm);

    /* Start charge when close enough */
    if (dist < BOMBER_CHARGE_RANGE && dist > 1.0f) {
        e->is_charging = true;
        Vector2 charge_dir = Vector2Scale(diff, 1.0f / dist);
        e->velocity = Vector2Scale(charge_dir, BOMBER_CHARGE_SPEED);
    }
}

void enemy_pool_update(EnemyPool *pool, float dt, Vector2 target, Rectangle arena,
                       const Tilemap *tm, EnemyBulletPool *ebp) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy *e = &pool->enemies[i];
        if (!e->active) {
            continue;
        }

        /* Tick hit flash timer */
        if (e->hit_flash > 0.0f) {
            e->hit_flash -= dt;
            if (e->hit_flash < 0.0f) {
                e->hit_flash = 0.0f;
            }
        }

        switch (e->type) {
        case ENEMY_SWARMER:
            update_swarmer(e, dt, target, arena, tm);
            break;
        case ENEMY_GRUNT:
            update_grunt(e, dt, target, arena, tm, ebp);
            break;
        case ENEMY_STALKER:
            update_stalker(e, dt, target, arena, tm);
            break;
        case ENEMY_BOMBER:
            update_bomber(e, dt, target, arena, tm);
            break;
        default:
            break;
        }
    }
}

/* Helper to get facing direction + perpendicular from velocity */
static void get_facing(const Enemy *e, Vector2 *facing, Vector2 *perp) {
    *facing = (Vector2){0.0f, 1.0f};
    float spd = Vector2Length(e->velocity);
    if (spd > 1.0f) {
        *facing = Vector2Scale(e->velocity, 1.0f / spd);
    }
    *perp = (Vector2){-facing->y, facing->x};
}

static void draw_swarmer(const Enemy *e) {
    Vector2 pos = e->position;
    float r = e->radius;
    Vector2 facing, perp;
    get_facing(e, &facing, &perp);

    bool flashing = e->hit_flash > 0.0f;
    Color body_fill = flashing ? (Color){255, 255, 255, 255} : (Color){80, 10, 10, 255};
    Color body_outline = flashing ? (Color){255, 255, 255, 255} : (Color){255, 60, 30, 255};
    Color glow = flashing ? (Color){255, 255, 255, 80} : (Color){255, 40, 20, 40};
    Color eye_color = (Color){255, 220, 50, 255};

    DrawCircleV(pos, r + 3.0f, glow);
    DrawCircleV(pos, r, body_fill);
    DrawCircleLinesV(pos, r, body_outline);

    /* Mandible lines */
    Vector2 jaw_base_l =
        Vector2Add(pos, Vector2Add(Vector2Scale(facing, r * 0.3f), Vector2Scale(perp, r * 0.4f)));
    Vector2 jaw_base_r =
        Vector2Add(pos, Vector2Add(Vector2Scale(facing, r * 0.3f), Vector2Scale(perp, -r * 0.4f)));
    Vector2 jaw_tip = Vector2Add(pos, Vector2Scale(facing, r + 2.0f));
    DrawLineEx(jaw_base_l, jaw_tip, 1.5f, body_outline);
    DrawLineEx(jaw_base_r, jaw_tip, 1.5f, body_outline);

    /* Eyes */
    Vector2 eye_l =
        Vector2Add(pos, Vector2Add(Vector2Scale(facing, r * 0.35f), Vector2Scale(perp, r * 0.35f)));
    Vector2 eye_r = Vector2Add(
        pos, Vector2Add(Vector2Scale(facing, r * 0.35f), Vector2Scale(perp, -r * 0.35f)));
    DrawCircleV(eye_l, 1.5f, eye_color);
    DrawCircleV(eye_r, 1.5f, eye_color);
}

static void draw_grunt(const Enemy *e) {
    Vector2 pos = e->position;
    float r = e->radius;
    Vector2 facing, perp;
    get_facing(e, &facing, &perp);

    bool flashing = e->hit_flash > 0.0f;
    Color body_fill = flashing ? (Color){255, 255, 255, 255} : (Color){40, 40, 80, 255};
    Color body_outline = flashing ? (Color){255, 255, 255, 255} : (Color){100, 100, 255, 255};
    Color glow = flashing ? (Color){255, 255, 255, 80} : (Color){60, 60, 255, 40};
    Color eye_color = (Color){255, 100, 100, 255};

    DrawCircleV(pos, r + 3.0f, glow);
    DrawCircleV(pos, r, body_fill);
    DrawCircleLinesV(pos, r, body_outline);

    /* Gun barrel line */
    Vector2 barrel_start = Vector2Add(pos, Vector2Scale(facing, r * 0.5f));
    Vector2 barrel_end = Vector2Add(pos, Vector2Scale(facing, r + 4.0f));
    DrawLineEx(barrel_start, barrel_end, 2.0f, body_outline);

    /* Single eye */
    Vector2 eye = Vector2Add(pos, Vector2Scale(facing, r * 0.2f));
    DrawCircleV(eye, 2.5f, eye_color);
}

static void draw_stalker(const Enemy *e) {
    Vector2 pos = e->position;
    float r = e->radius;
    Vector2 facing, perp;
    get_facing(e, &facing, &perp);

    bool flashing = e->hit_flash > 0.0f;
    Color body_fill = flashing ? (Color){255, 255, 255, 255} : (Color){30, 60, 30, 255};
    Color body_outline = flashing ? (Color){255, 255, 255, 255} : (Color){50, 255, 50, 255};
    Color glow = flashing ? (Color){255, 255, 255, 80} : (Color){50, 255, 50, 40};
    Color eye_color = (Color){200, 255, 50, 255};

    /* Stalker is drawn as a diamond/triangle shape for speed feel */
    DrawCircleV(pos, r + 2.0f, glow);
    DrawCircleV(pos, r, body_fill);
    DrawCircleLinesV(pos, r, body_outline);

    /* Speed lines when dashing */
    if (e->is_charging) {
        Vector2 trail1 = Vector2Add(pos, Vector2Scale(facing, -(r + 3.0f)));
        Vector2 trail2 = Vector2Add(pos, Vector2Scale(facing, -(r + 8.0f)));
        DrawLineEx(trail1, trail2, 1.5f, (Color){50, 255, 50, 100});
    }

    /* Two narrow eyes */
    Vector2 eye_l =
        Vector2Add(pos, Vector2Add(Vector2Scale(facing, r * 0.3f), Vector2Scale(perp, r * 0.25f)));
    Vector2 eye_r =
        Vector2Add(pos, Vector2Add(Vector2Scale(facing, r * 0.3f), Vector2Scale(perp, -r * 0.25f)));
    DrawCircleV(eye_l, 1.5f, eye_color);
    DrawCircleV(eye_r, 1.5f, eye_color);
}

static void draw_bomber(const Enemy *e) {
    Vector2 pos = e->position;
    float r = e->radius;

    bool flashing = e->hit_flash > 0.0f;
    Color body_fill = flashing ? (Color){255, 255, 255, 255} : (Color){80, 40, 10, 255};
    Color body_outline = flashing ? (Color){255, 255, 255, 255} : (Color){255, 150, 30, 255};
    Color glow = flashing ? (Color){255, 255, 255, 80} : (Color){255, 100, 20, 50};
    Color core_color = e->is_charging ? (Color){255, 60, 30, 255} : (Color){255, 200, 50, 255};

    DrawCircleV(pos, r + 4.0f, glow);
    DrawCircleV(pos, r, body_fill);
    DrawCircleLinesV(pos, r, body_outline);

    /* Pulsing core (brighter when charging) */
    float core_r = e->is_charging ? 5.0f : 3.0f;
    DrawCircleV(pos, core_r, core_color);

    /* Warning lines when charging */
    if (e->is_charging) {
        DrawCircleLinesV(pos, r + 6.0f, (Color){255, 60, 30, 120});
    }
}

void enemy_pool_draw(const EnemyPool *pool) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        const Enemy *e = &pool->enemies[i];
        if (!e->active) {
            continue;
        }

        switch (e->type) {
        case ENEMY_SWARMER:
            draw_swarmer(e);
            break;
        case ENEMY_GRUNT:
            draw_grunt(e);
            break;
        case ENEMY_STALKER:
            draw_stalker(e);
            break;
        case ENEMY_BOMBER:
            draw_bomber(e);
            break;
        default:
            break;
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

/* ── Enemy bullet pool ─────────────────────────────────────────────────────── */

void enemy_bullet_pool_init(EnemyBulletPool *pool) {
    memset(pool->bullets, 0, sizeof(pool->bullets));
}

void enemy_bullet_pool_fire(EnemyBulletPool *pool, Vector2 origin, Vector2 direction, float speed,
                            float damage) {
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        EnemyBullet *b = &pool->bullets[i];
        if (!b->active) {
            b->position = origin;
            b->velocity = Vector2Scale(direction, speed);
            b->lifetime = 3.0f;
            b->damage = damage;
            b->active = true;
            return;
        }
    }
}

void enemy_bullet_pool_update(EnemyBulletPool *pool, float dt, Rectangle arena) {
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        EnemyBullet *b = &pool->bullets[i];
        if (!b->active) {
            continue;
        }

        b->position = Vector2Add(b->position, Vector2Scale(b->velocity, dt));
        b->lifetime -= dt;

        if (b->lifetime <= 0.0f || b->position.x < arena.x ||
            b->position.x > arena.x + arena.width || b->position.y < arena.y ||
            b->position.y > arena.y + arena.height) {
            b->active = false;
        }
    }
}

void enemy_bullet_pool_draw(const EnemyBulletPool *pool) {
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        const EnemyBullet *b = &pool->bullets[i];
        if (!b->active) {
            continue;
        }
        DrawCircleV(b->position, 4.0f, (Color){255, 80, 80, 255});
    }
}

/* ── Collision helpers ─────────────────────────────────────────────────────── */

bool check_circle_collision(Vector2 pos_a, float radius_a, Vector2 pos_b, float radius_b) {
    float dx = pos_a.x - pos_b.x;
    float dy = pos_a.y - pos_b.y;
    float dist_sq = dx * dx + dy * dy;
    float radii = radius_a + radius_b;
    return dist_sq <= radii * radii;
}
