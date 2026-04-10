/*
 * game.c -- Top-level game state, update loop, and draw
 */

#include "game.h"

/* ── Helpers ───────────────────────────────────────────────────────────────── */

static void spawn_wave(GameState *gs) {
    int count = GetRandomValue(SPAWN_MIN_GROUP, SPAWN_MAX_GROUP);

    for (int i = 0; i < count; i++) {
        Vector2 pos;
        int edge = GetRandomValue(0, 3);

        switch (edge) {
        case 0: /* top */
            pos.x = gs->arena.x + (float)GetRandomValue(0, (int)gs->arena.width);
            pos.y = gs->arena.y;
            break;
        case 1: /* bottom */
            pos.x = gs->arena.x + (float)GetRandomValue(0, (int)gs->arena.width);
            pos.y = gs->arena.y + gs->arena.height;
            break;
        case 2: /* left */
            pos.x = gs->arena.x;
            pos.y = gs->arena.y + (float)GetRandomValue(0, (int)gs->arena.height);
            break;
        default: /* right */
            pos.x = gs->arena.x + gs->arena.width;
            pos.y = gs->arena.y + (float)GetRandomValue(0, (int)gs->arena.height);
            break;
        }

        enemy_pool_spawn(&gs->enemies, ENEMY_SWARMER, pos);
    }
}

static void resolve_bullet_enemy_collisions(GameState *gs) {
    BulletPool *bp = &gs->bullets;
    EnemyPool *ep = &gs->enemies;

    for (int b = 0; b < MAX_BULLETS; b++) {
        Bullet *bullet = &bp->bullets[b];
        if (!bullet->active) {
            continue;
        }

        for (int e = 0; e < MAX_ENEMIES; e++) {
            Enemy *enemy = &ep->enemies[e];
            if (!enemy->active) {
                continue;
            }

            if (check_circle_collision(bullet->position, BULLET_RADIUS, enemy->position,
                                       enemy->radius)) {
                bullet->active = false;
                enemy->hp -= 1.0f;
                if (enemy->hp <= 0.0f) {
                    enemy->active = false;
                }
                break; /* bullet consumed -- stop checking enemies */
            }
        }
    }
}

static void resolve_enemy_player_collisions(GameState *gs) {
    Player *p = &gs->player;
    EnemyPool *ep = &gs->enemies;

    /* Skip damage if player is dashing and invincible */
    if (DASH_INVINCIBLE && p->is_dashing) {
        return;
    }

    for (int e = 0; e < MAX_ENEMIES; e++) {
        Enemy *enemy = &ep->enemies[e];
        if (!enemy->active) {
            continue;
        }

        if (check_circle_collision(p->position, PLAYER_RADIUS, enemy->position, enemy->radius)) {
            p->hp -= enemy->damage;
            enemy->active = false; /* swarmer dies on contact */
            if (p->hp < 0.0f) {
                p->hp = 0.0f;
            }
        }
    }
}

static void draw_hud(const GameState *gs) {
    const Player *p = &gs->player;

    /* ── Health bar (bottom-left) ───────────────────────────────────────── */
    float bar_x = 10.0f;
    float bar_y = SCREEN_HEIGHT - 30.0f;
    float bar_w = 150.0f;
    float bar_h = 16.0f;
    float hp_ratio = p->hp / p->max_hp;
    if (hp_ratio < 0.0f) {
        hp_ratio = 0.0f;
    }

    DrawRectangle((int)bar_x, (int)bar_y, (int)bar_w, (int)bar_h, DARKGRAY);
    DrawRectangle((int)bar_x, (int)bar_y, (int)(bar_w * hp_ratio), (int)bar_h, RED);
    DrawRectangleLines((int)bar_x, (int)bar_y, (int)bar_w, (int)bar_h, RAYWHITE);
    DrawText("HP", (int)bar_x + 4, (int)bar_y + 2, 12, RAYWHITE);

    /* ── Dash cooldown indicator (next to health bar) ─────────────────── */
    float dash_x = bar_x + bar_w + 20.0f;
    float dash_w = 80.0f;
    float cd_ratio = 1.0f;
    if (p->dash_cooldown > 0.0f) {
        cd_ratio = 1.0f - (p->dash_cooldown / DASH_COOLDOWN);
    }

    DrawRectangle((int)dash_x, (int)bar_y, (int)dash_w, (int)bar_h, DARKGRAY);
    DrawRectangle((int)dash_x, (int)bar_y, (int)(dash_w * cd_ratio), (int)bar_h, SKYBLUE);
    DrawRectangleLines((int)dash_x, (int)bar_y, (int)dash_w, (int)bar_h, RAYWHITE);
    DrawText("DASH", (int)dash_x + 4, (int)bar_y + 2, 12, RAYWHITE);
}

static void draw_arena(const GameState *gs) {
    DrawRectangleLinesEx(gs->arena, 2.0f, DARKGRAY);
}

/* ── Public ────────────────────────────────────────────────────────────────── */

void game_init(GameState *gs) {
    gs->arena = (Rectangle){ARENA_MARGIN, ARENA_MARGIN, SCREEN_WIDTH - 2.0f * ARENA_MARGIN,
                            SCREEN_HEIGHT - 2.0f * ARENA_MARGIN};

    Vector2 center = {gs->arena.x + gs->arena.width / 2.0f, gs->arena.y + gs->arena.height / 2.0f};

    player_init(&gs->player, center);
    bullet_pool_init(&gs->bullets);
    enemy_pool_init(&gs->enemies);
    gs->spawn_timer = SPAWN_INTERVAL;
}

void game_update(GameState *gs) {
    float dt = GetFrameTime();

    player_update(&gs->player, dt, gs->arena);

    /* ── Shooting ─────────────────────────────────────────────────────── */
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        Vector2 muzzle = Vector2Add(gs->player.position,
                                    Vector2Scale(gs->player.aim_direction, PLAYER_RADIUS + 2.0f));
        bullet_pool_fire(&gs->bullets, muzzle, gs->player.aim_direction);
    }

    bullet_pool_update(&gs->bullets, dt, gs->arena);

    /* ── Enemy spawning ───────────────────────────────────────────────── */
    gs->spawn_timer -= dt;
    if (gs->spawn_timer <= 0.0f) {
        spawn_wave(gs);
        gs->spawn_timer = SPAWN_INTERVAL;
    }

    /* ── Enemy update ─────────────────────────────────────────────────── */
    enemy_pool_update(&gs->enemies, dt, gs->player.position, gs->arena);

    /* ── Collisions ───────────────────────────────────────────────────── */
    resolve_bullet_enemy_collisions(gs);
    resolve_enemy_player_collisions(gs);
}

void game_draw(const GameState *gs) {
    BeginDrawing();
    ClearBackground(BLACK);
    draw_arena(gs);
    bullet_pool_draw(&gs->bullets);
    enemy_pool_draw(&gs->enemies);
    player_draw(&gs->player);
    draw_hud(gs);
    EndDrawing();
}
