/*
 * game.c -- Top-level game state, update loop, and draw
 */

#include "game.h"

/* ── Helpers ───────────────────────────────────────────────────────────────── */

/* Spawn a wave of enemies just outside the camera viewport */
static void spawn_wave(GameState *gs) {
    int count = GetRandomValue(SPAWN_MIN_GROUP, SPAWN_MAX_GROUP);

    /* Calculate viewport edges in world space */
    float half_w = (float)SCREEN_WIDTH / 2.0f;
    float half_h = (float)SCREEN_HEIGHT / 2.0f;
    float cam_x = gs->camera.target.x;
    float cam_y = gs->camera.target.y;

    /* Spawn margin: a little outside the visible area */
    float margin = 40.0f;
    float left = cam_x - half_w - margin;
    float right = cam_x + half_w + margin;
    float top = cam_y - half_h - margin;
    float bottom = cam_y + half_h + margin;

    /* Clamp to world bounds */
    Rectangle wb = gs->arena;
    if (left < wb.x) {
        left = wb.x + 10.0f;
    }
    if (right > wb.x + wb.width) {
        right = wb.x + wb.width - 10.0f;
    }
    if (top < wb.y) {
        top = wb.y + 10.0f;
    }
    if (bottom > wb.y + wb.height) {
        bottom = wb.y + wb.height - 10.0f;
    }

    for (int i = 0; i < count; i++) {
        Vector2 pos;
        int edge = GetRandomValue(0, 3);

        switch (edge) {
        case 0: /* top */
            pos.x = (float)GetRandomValue((int)left, (int)right);
            pos.y = top;
            break;
        case 1: /* bottom */
            pos.x = (float)GetRandomValue((int)left, (int)right);
            pos.y = bottom;
            break;
        case 2: /* left */
            pos.x = left;
            pos.y = (float)GetRandomValue((int)top, (int)bottom);
            break;
        default: /* right */
            pos.x = right;
            pos.y = (float)GetRandomValue((int)top, (int)bottom);
            break;
        }

        /* Only spawn in walkable tiles */
        if (!tilemap_is_solid(&gs->tilemap, pos.x, pos.y)) {
            enemy_pool_spawn(&gs->enemies, ENEMY_SWARMER, pos);
        }
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
                    gs->stats.kills++;
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

static void update_camera(GameState *gs) {
    gs->camera.target = gs->player.position;

    /* Clamp camera so it doesn't show outside the world */
    float half_w = gs->camera.offset.x;
    float half_h = gs->camera.offset.y;
    Rectangle wb = gs->arena;

    if (gs->camera.target.x - half_w < wb.x) {
        gs->camera.target.x = wb.x + half_w;
    }
    if (gs->camera.target.x + half_w > wb.x + wb.width) {
        gs->camera.target.x = wb.x + wb.width - half_w;
    }
    if (gs->camera.target.y - half_h < wb.y) {
        gs->camera.target.y = wb.y + half_h;
    }
    if (gs->camera.target.y + half_h > wb.y + wb.height) {
        gs->camera.target.y = wb.y + wb.height - half_h;
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

static void draw_game_over(const GameState *gs) {
    /* Semi-transparent dark overlay */
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){0, 0, 0, 180});

    /* "GAME OVER" title */
    const char *title = "GAME OVER";
    int title_size = 60;
    int title_w = MeasureText(title, title_size);
    int title_x = (SCREEN_WIDTH - title_w) / 2;
    int title_y = SCREEN_HEIGHT / 2 - 100;
    DrawText(title, title_x, title_y, title_size, RED);

    /* Stats */
    int stat_size = 20;
    int stat_x_center = SCREEN_WIDTH / 2;
    int stat_y = title_y + title_size + 30;
    int line_spacing = 28;

    /* Kills */
    const char *kills_text = TextFormat("Kills: %d", gs->stats.kills);
    int kills_w = MeasureText(kills_text, stat_size);
    DrawText(kills_text, stat_x_center - kills_w / 2, stat_y, stat_size, RAYWHITE);

    /* Survival time (MM:SS) */
    int total_seconds = (int)gs->stats.survival_time;
    int minutes = total_seconds / 60;
    int seconds = total_seconds % 60;
    const char *time_text = TextFormat("Time: %02d:%02d", minutes, seconds);
    int time_w = MeasureText(time_text, stat_size);
    DrawText(time_text, stat_x_center - time_w / 2, stat_y + line_spacing, stat_size, RAYWHITE);

    /* Waves spawned */
    const char *waves_text = TextFormat("Waves: %d", gs->stats.waves_spawned);
    int waves_w = MeasureText(waves_text, stat_size);
    DrawText(waves_text, stat_x_center - waves_w / 2, stat_y + 2 * line_spacing, stat_size,
             RAYWHITE);

    /* Restart prompt */
    const char *prompt = "Press R to restart";
    int prompt_size = 16;
    int prompt_w = MeasureText(prompt, prompt_size);
    DrawText(prompt, (SCREEN_WIDTH - prompt_w) / 2, stat_y + 3 * line_spacing + 20, prompt_size,
             GRAY);
}

/* ── Public ────────────────────────────────────────────────────────────────── */

void game_init(GameState *gs) {
    /* Generate world */
    int spawn_tx = WORLD_COLS / 2;
    int spawn_ty = WORLD_ROWS / 2;
    tilemap_generate(&gs->tilemap, spawn_tx, spawn_ty, 0);
    gs->arena = tilemap_get_world_bounds(&gs->tilemap);

    /* Place player at center of spawn tile */
    Vector2 center = tilemap_tile_to_world(&gs->tilemap, spawn_tx, spawn_ty);
    center.x += TILE_SIZE / 2.0f;
    center.y += TILE_SIZE / 2.0f;

    player_init(&gs->player, center);
    bullet_pool_init(&gs->bullets);
    enemy_pool_init(&gs->enemies);
    gs->spawn_timer = SPAWN_INTERVAL;
    gs->phase = PHASE_PLAYING;
    gs->stats = (GameStats){.kills = 0, .survival_time = 0.0f, .waves_spawned = 0};

    /* Camera centered on player */
    gs->camera.target = center;
    gs->camera.offset = (Vector2){SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f};
    gs->camera.rotation = 0.0f;
    gs->camera.zoom = 1.0f;
}

void game_update(GameState *gs) {
    /* ── Game-over phase: wait for restart ────────────────────────────── */
    if (gs->phase == PHASE_GAME_OVER) {
        if (IsKeyPressed(KEY_R)) {
            game_init(gs);
        }
        return;
    }

    /* ── Playing phase ────────────────────────────────────────────────── */
    float dt = GetFrameTime();
    gs->stats.survival_time += dt;

    player_update(&gs->player, dt, gs->arena, &gs->tilemap, gs->camera);

    /* ── Shooting ─────────────────────────────────────────────────────── */
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        Vector2 muzzle = Vector2Add(gs->player.position,
                                    Vector2Scale(gs->player.aim_direction, PLAYER_RADIUS + 2.0f));
        bullet_pool_fire(&gs->bullets, muzzle, gs->player.aim_direction);
    }

    bullet_pool_update(&gs->bullets, dt, gs->arena, &gs->tilemap);

    /* ── Enemy spawning ───────────────────────────────────────────────── */
    gs->spawn_timer -= dt;
    if (gs->spawn_timer <= 0.0f) {
        spawn_wave(gs);
        gs->spawn_timer = SPAWN_INTERVAL;
        gs->stats.waves_spawned++;
    }

    /* ── Enemy update ─────────────────────────────────────────────────── */
    enemy_pool_update(&gs->enemies, dt, gs->player.position, gs->arena, &gs->tilemap);

    /* ── Collisions ───────────────────────────────────────────────────── */
    resolve_bullet_enemy_collisions(gs);
    resolve_enemy_player_collisions(gs);

    /* ── Death check ──────────────────────────────────────────────────── */
    game_check_death(gs);

    /* ── Camera ───────────────────────────────────────────────────────── */
    update_camera(gs);
}

void game_check_death(GameState *gs) {
    if (gs->phase == PHASE_PLAYING && gs->player.hp <= 0.0f) {
        gs->phase = PHASE_GAME_OVER;
    }
}

void game_draw(const GameState *gs) {
    BeginDrawing();
    ClearBackground(BLACK);

    /* ── World-space drawing (camera-relative) ────────────────────────── */
    BeginMode2D(gs->camera);

    tilemap_draw(&gs->tilemap, gs->camera);
    bullet_pool_draw(&gs->bullets);
    enemy_pool_draw(&gs->enemies);
    player_draw(&gs->player);

    EndMode2D();

    /* ── Screen-space drawing (HUD, overlays) ─────────────────────────── */
    draw_hud(gs);
    if (gs->phase == PHASE_GAME_OVER) {
        draw_game_over(gs);
    }

    EndDrawing();
}
