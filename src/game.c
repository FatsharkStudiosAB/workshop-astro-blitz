/*
 * game.c -- Top-level game state, update loop, and draw
 */

#include "game.h"
#include <string.h>

/* ── Helpers ───────────────────────────────────────────────────────────────── */

/* Maximum attempts per enemy to find a walkable spawn position */
#define SPAWN_RETRIES 5

/* Spawn a wave of enemies just outside the camera viewport */
static void spawn_wave(GameState *gs) {
    int count = GetRandomValue(SPAWN_MIN_GROUP, SPAWN_MAX_GROUP);

    /* Calculate viewport edges in world space from the active camera */
    float screen_w = (float)GetScreenWidth();
    float screen_h = (float)GetScreenHeight();
    float zoom = (gs->camera.zoom != 0.0f) ? gs->camera.zoom : 1.0f;
    float cam_x = gs->camera.target.x;
    float cam_y = gs->camera.target.y;

    /* Spawn margin: a little outside the visible area */
    float margin = 40.0f;
    float left = cam_x - (gs->camera.offset.x / zoom) - margin;
    float right = cam_x + ((screen_w - gs->camera.offset.x) / zoom) + margin;
    float top = cam_y - (gs->camera.offset.y / zoom) - margin;
    float bottom = cam_y + ((screen_h - gs->camera.offset.y) / zoom) + margin;

    /* Clamp to the walkable interior, not the solid border-wall tiles */
    Rectangle wb = gs->arena;
    float spawn_padding = 10.0f;
    float min_spawn_x = wb.x + TILE_SIZE + spawn_padding;
    float max_spawn_x = wb.x + wb.width - TILE_SIZE - spawn_padding;
    float min_spawn_y = wb.y + TILE_SIZE + spawn_padding;
    float max_spawn_y = wb.y + wb.height - TILE_SIZE - spawn_padding;

    if (left < min_spawn_x) {
        left = min_spawn_x;
    }
    if (right > max_spawn_x) {
        right = max_spawn_x;
    }
    if (top < min_spawn_y) {
        top = min_spawn_y;
    }
    if (bottom > max_spawn_y) {
        bottom = max_spawn_y;
    }

    for (int i = 0; i < count; i++) {
        for (int attempt = 0; attempt < SPAWN_RETRIES; attempt++) {
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

            /* Spawn if the position is walkable; otherwise retry */
            if (!tilemap_is_solid(&gs->tilemap, pos.x, pos.y)) {
                enemy_pool_spawn(&gs->enemies, ENEMY_SWARMER, pos);
                break;
            }
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
                audio_play_enemy_hit(&gs->audio);
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
            audio_play_hit(&gs->audio);
            if (p->hp < 0.0f) {
                p->hp = 0.0f;
            }
        }
    }
}

static void update_camera(GameState *gs) {
    gs->camera.target = gs->player.position;

    /* Clamp camera so it doesn't show outside the world.
     * offset is in screen pixels; convert to world units via zoom. */
    float zoom = (gs->camera.zoom != 0.0f) ? gs->camera.zoom : 1.0f;
    float half_w = gs->camera.offset.x / zoom;
    float half_h = gs->camera.offset.y / zoom;
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

    /* Restart / menu prompts */
    const char *prompt = "Press R to restart  |  ESC for menu";
    int prompt_size = 16;
    int prompt_w = MeasureText(prompt, prompt_size);
    DrawText(prompt, (SCREEN_WIDTH - prompt_w) / 2, stat_y + 3 * line_spacing + 20, prompt_size,
             GRAY);
}

/* ── Menu helpers ──────────────────────────────────────────────────────────── */

/* Navigate a menu cursor with W/S/Up/Down. Returns new cursor value. */
static int menu_navigate(int cursor, int item_count) {
    if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP)) {
        cursor--;
        if (cursor < 0) {
            cursor = item_count - 1;
        }
    }
    if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN)) {
        cursor++;
        if (cursor >= item_count) {
            cursor = 0;
        }
    }
    return cursor;
}

/* Returns true if the user confirmed the current menu selection */
static bool menu_confirm(void) {
    return IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER);
}

/* Draw a centered menu list. Selected item is highlighted. */
static void draw_menu_items(const char *items[], int count, int selected, int start_y,
                            int font_size, int spacing) {
    for (int i = 0; i < count; i++) {
        int w = MeasureText(items[i], font_size);
        int x = (SCREEN_WIDTH - w) / 2;
        int y = start_y + i * spacing;
        Color c = (i == selected) ? (Color){0, 220, 200, 255} : GRAY;
        DrawText(items[i], x, y, font_size, c);

        /* Draw selection arrow */
        if (i == selected) {
            DrawText(">", x - 20, y, font_size, c);
        }
    }
}

/* ── Phase-specific update helpers ─────────────────────────────────────────── */

static void update_first_run(GameState *gs) {
    enum { PICK_8DIR, PICK_TANK, PICK_COUNT };

    gs->menu_cursor = menu_navigate(gs->menu_cursor, PICK_COUNT);

    if (menu_confirm()) {
        switch (gs->menu_cursor) {
        case PICK_8DIR:
            gs->settings.movement_layout = MOVEMENT_8DIR;
            break;
        case PICK_TANK:
            gs->settings.movement_layout = MOVEMENT_TANK;
            break;
        }
        settings_save(&gs->settings);
        gs->phase = PHASE_MAIN_MENU;
        gs->menu_cursor = 0;
    }
}

static void update_main_menu(GameState *gs) {
    enum { MENU_PLAY, MENU_SETTINGS, MENU_QUIT, MENU_COUNT };

    gs->menu_cursor = menu_navigate(gs->menu_cursor, MENU_COUNT);

    if (menu_confirm()) {
        switch (gs->menu_cursor) {
        case MENU_PLAY:
            game_init(gs);
            gs->phase = PHASE_PLAYING;
            gs->menu_cursor = 0;
            break;
        case MENU_SETTINGS:
            gs->settings_return_phase = PHASE_MAIN_MENU;
            gs->phase = PHASE_SETTINGS;
            gs->menu_cursor = 0;
            break;
        case MENU_QUIT:
            gs->should_quit = true;
            break;
        }
    }
}

static void update_paused(GameState *gs) {
    enum { PAUSE_RESUME, PAUSE_SETTINGS, PAUSE_MAIN_MENU, PAUSE_QUIT, PAUSE_COUNT };

    /* ESC to resume */
    if (IsKeyPressed(KEY_ESCAPE)) {
        gs->phase = PHASE_PLAYING;
        gs->menu_cursor = 0;
        return;
    }

    gs->menu_cursor = menu_navigate(gs->menu_cursor, PAUSE_COUNT);

    if (menu_confirm()) {
        switch (gs->menu_cursor) {
        case PAUSE_RESUME:
            gs->phase = PHASE_PLAYING;
            gs->menu_cursor = 0;
            break;
        case PAUSE_SETTINGS:
            gs->settings_return_phase = PHASE_PAUSED;
            gs->phase = PHASE_SETTINGS;
            gs->menu_cursor = 0;
            break;
        case PAUSE_MAIN_MENU:
            audio_stop_death_music(&gs->audio);
            gs->phase = PHASE_MAIN_MENU;
            gs->menu_cursor = 0;
            break;
        case PAUSE_QUIT:
            gs->should_quit = true;
            break;
        }
    }
}

static void update_settings(GameState *gs) {
    /* ESC to go back */
    if (IsKeyPressed(KEY_ESCAPE)) {
        gs->phase = gs->settings_return_phase;
        gs->menu_cursor = 0;
        return;
    }

    /* Left/Right to toggle movement layout */
    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT)) {
        if (gs->settings.movement_layout == MOVEMENT_TANK) {
            gs->settings.movement_layout = MOVEMENT_8DIR;
        } else {
            gs->settings.movement_layout = MOVEMENT_TANK;
        }
        settings_save(&gs->settings);
    }
}

static void update_playing(GameState *gs) {
    /* ESC to pause */
    if (IsKeyPressed(KEY_ESCAPE)) {
        gs->phase = PHASE_PAUSED;
        gs->menu_cursor = 0;
        return;
    }

    float dt = GetFrameTime();
    gs->stats.survival_time += dt;

    player_update(&gs->player, dt, gs->arena, &gs->tilemap, gs->camera,
                  gs->settings.movement_layout);

    /* ── Shooting ─────────────────────────────────────────────────────── */
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        Vector2 muzzle = Vector2Add(gs->player.position,
                                    Vector2Scale(gs->player.aim_direction, PLAYER_RADIUS + 2.0f));
        if (bullet_pool_fire(&gs->bullets, muzzle, gs->player.aim_direction)) {
            audio_play_shoot(&gs->audio);
        }
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
    tilemap_compute_flow_field(&gs->tilemap, gs->player.position.x, gs->player.position.y);
    enemy_pool_update(&gs->enemies, dt, gs->player.position, gs->arena, &gs->tilemap);

    /* ── Collisions ───────────────────────────────────────────────────── */
    resolve_bullet_enemy_collisions(gs);
    resolve_enemy_player_collisions(gs);

    /* ── Death check ──────────────────────────────────────────────────── */
    game_check_death(gs);

    /* ── Camera ───────────────────────────────────────────────────────── */
    update_camera(gs);
}

static void update_game_over(GameState *gs) {
    /* ESC opens pause menu (with main menu / quit options) */
    if (IsKeyPressed(KEY_ESCAPE)) {
        gs->phase = PHASE_PAUSED;
        gs->menu_cursor = 0;
        return;
    }

    /* R to restart */
    if (IsKeyPressed(KEY_R)) {
        audio_stop_death_music(&gs->audio);
        game_init(gs);
        gs->phase = PHASE_PLAYING;
    }
}

/* ── Phase-specific draw helpers ───────────────────────────────────────────── */

static void draw_first_run(const GameState *gs) {
    /* Title */
    const char *title = "ASTRO BLITZ";
    int title_size = 50;
    int title_w = MeasureText(title, title_size);
    DrawText(title, (SCREEN_WIDTH - title_w) / 2, 80, title_size, (Color){0, 220, 200, 255});

    /* Prompt */
    const char *prompt = "Choose your movement style:";
    int prompt_size = 20;
    int prompt_w = MeasureText(prompt, prompt_size);
    DrawText(prompt, (SCREEN_WIDTH - prompt_w) / 2, 170, prompt_size, RAYWHITE);

    /* Options */
    const char *items[] = {"8-Directional", "Tank Controls"};
    draw_menu_items(items, 2, gs->menu_cursor, 240, 24, 50);

    /* Descriptions for the currently highlighted option */
    const char *desc = NULL;
    if (gs->menu_cursor == 0) {
        desc = "W = up, S = down, A = left, D = right";
    } else {
        desc = "W/S = forward/back toward aim, A/D = strafe";
    }
    int desc_size = 16;
    int desc_w = MeasureText(desc, desc_size);
    DrawText(desc, (SCREEN_WIDTH - desc_w) / 2, 360, desc_size, GRAY);

    const char *note = "(You can change this later in Settings)";
    int note_size = 14;
    int note_w = MeasureText(note, note_size);
    DrawText(note, (SCREEN_WIDTH - note_w) / 2, 390, note_size, DARKGRAY);

    /* Hint */
    const char *hint = "W/S to select  |  Enter to confirm";
    int hint_size = 14;
    int hint_w = MeasureText(hint, hint_size);
    DrawText(hint, (SCREEN_WIDTH - hint_w) / 2, SCREEN_HEIGHT - 50, hint_size, DARKGRAY);
}

static void draw_main_menu(const GameState *gs) {
    /* Title */
    const char *title = "ASTRO BLITZ";
    int title_size = 50;
    int title_w = MeasureText(title, title_size);
    DrawText(title, (SCREEN_WIDTH - title_w) / 2, 120, title_size, (Color){0, 220, 200, 255});

    /* Subtitle */
    const char *sub = "Top-down sci-fi roguelike shooter";
    int sub_size = 16;
    int sub_w = MeasureText(sub, sub_size);
    DrawText(sub, (SCREEN_WIDTH - sub_w) / 2, 180, sub_size, GRAY);

    /* Menu items */
    const char *items[] = {"Play", "Settings", "Quit"};
    draw_menu_items(items, 3, gs->menu_cursor, 280, 24, 40);
}

static void draw_paused(const GameState *gs) {
    /* Dark overlay */
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){0, 0, 0, 180});

    const char *title = "PAUSED";
    int title_size = 40;
    int title_w = MeasureText(title, title_size);
    DrawText(title, (SCREEN_WIDTH - title_w) / 2, 150, title_size, RAYWHITE);

    const char *items[] = {"Resume", "Settings", "Main Menu", "Quit"};
    draw_menu_items(items, 4, gs->menu_cursor, 240, 22, 36);
}

static void draw_settings(const GameState *gs) {
    /* Dark overlay if coming from pause (game is behind) */
    if (gs->settings_return_phase != PHASE_MAIN_MENU) {
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){0, 0, 0, 200});
    }

    const char *title = "SETTINGS";
    int title_size = 40;
    int title_w = MeasureText(title, title_size);
    DrawText(title, (SCREEN_WIDTH - title_w) / 2, 120, title_size, (Color){0, 220, 200, 255});

    /* Movement layout option */
    const char *label = "Movement:";
    int label_size = 22;
    int label_w = MeasureText(label, label_size);
    int label_x = SCREEN_WIDTH / 2 - label_w - 10;
    int y = 240;
    DrawText(label, label_x, y, label_size, RAYWHITE);

    const char *value =
        (gs->settings.movement_layout == MOVEMENT_TANK) ? "< Tank Controls >" : "< 8-Directional >";
    int value_size = 22;
    DrawText(value, SCREEN_WIDTH / 2 + 10, y, value_size, (Color){0, 220, 200, 255});

    /* Description */
    const char *desc = NULL;
    if (gs->settings.movement_layout == MOVEMENT_TANK) {
        desc = "W/S = forward/back relative to aim, A/D = strafe";
    } else {
        desc = "W = up, S = down, A = left, D = right (screen-relative)";
    }
    int desc_size = 14;
    int desc_w = MeasureText(desc, desc_size);
    DrawText(desc, (SCREEN_WIDTH - desc_w) / 2, y + 40, desc_size, GRAY);

    /* Instructions */
    const char *hint = "Left/Right to change  |  ESC to go back";
    int hint_size = 14;
    int hint_w = MeasureText(hint, hint_size);
    DrawText(hint, (SCREEN_WIDTH - hint_w) / 2, SCREEN_HEIGHT - 60, hint_size, DARKGRAY);
}

/* ── Public ────────────────────────────────────────────────────────────────── */

void game_init(GameState *gs) {
    /* Preserve settings and audio across restarts */
    Settings saved_settings = gs->settings;
    GameAudio saved_audio = gs->audio;

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
    gs->settings_return_phase = PHASE_MAIN_MENU;
    gs->menu_cursor = 0;
    gs->stats = (GameStats){.kills = 0, .survival_time = 0.0f, .waves_spawned = 0};

    /* Camera centered on player */
    gs->camera.target = center;
    gs->camera.offset = (Vector2){SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f};
    gs->camera.rotation = 0.0f;
    gs->camera.zoom = 1.0f;

    /* Restore preserved state */
    gs->settings = saved_settings;
    gs->audio = saved_audio;
}

void game_update(GameState *gs) {
    switch (gs->phase) {
    case PHASE_FIRST_RUN:
        update_first_run(gs);
        break;
    case PHASE_MAIN_MENU:
        update_main_menu(gs);
        break;
    case PHASE_PLAYING:
        update_playing(gs);
        break;
    case PHASE_PAUSED:
        update_paused(gs);
        break;
    case PHASE_SETTINGS:
        update_settings(gs);
        break;
    case PHASE_GAME_OVER:
        update_game_over(gs);
        break;
    }
}

void game_check_death(GameState *gs) {
    if (gs->phase == PHASE_PLAYING && gs->player.hp <= 0.0f) {
        gs->phase = PHASE_GAME_OVER;
        audio_play_death_music(&gs->audio);
    }
}

bool game_should_quit(const GameState *gs) {
    return gs->should_quit;
}

void game_draw(const GameState *gs) {
    BeginDrawing();
    ClearBackground(BLACK);

    switch (gs->phase) {
    case PHASE_FIRST_RUN:
        draw_first_run(gs);
        break;

    case PHASE_MAIN_MENU:
        draw_main_menu(gs);
        break;

    case PHASE_PLAYING:
    case PHASE_PAUSED:
    case PHASE_GAME_OVER:
        /* Draw the world behind any overlay */
        BeginMode2D(gs->camera);
        tilemap_draw(&gs->tilemap, gs->camera);
        bullet_pool_draw(&gs->bullets);
        enemy_pool_draw(&gs->enemies);
        player_draw(&gs->player);
        EndMode2D();

        draw_hud(gs);

        if (gs->phase == PHASE_GAME_OVER) {
            draw_game_over(gs);
        } else if (gs->phase == PHASE_PAUSED) {
            draw_paused(gs);
        }
        break;

    case PHASE_SETTINGS:
        /* Draw world behind if returning to pause, black bg if from main menu */
        if (gs->settings_return_phase != PHASE_MAIN_MENU) {
            BeginMode2D(gs->camera);
            tilemap_draw(&gs->tilemap, gs->camera);
            bullet_pool_draw(&gs->bullets);
            enemy_pool_draw(&gs->enemies);
            player_draw(&gs->player);
            EndMode2D();
            draw_hud(gs);
        }
        draw_settings(gs);
        break;
    }

    EndDrawing();
}
