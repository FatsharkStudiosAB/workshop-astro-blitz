/*
 * game.c -- Top-level game state, update loop, and draw
 */

#include "game.h"

/* ── Helpers ───────────────────────────────────────────────────────────────── */

static void draw_hud(const GameState *gs)
{
    const Player *p = &gs->player;

    /* ── Health bar (top-left) ────────────────────────────────────────── */
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

static void draw_arena(const GameState *gs)
{
    DrawRectangleLinesEx(gs->arena, 2.0f, DARKGRAY);
}

/* ── Public ────────────────────────────────────────────────────────────────── */

void game_init(GameState *gs)
{
    gs->arena = (Rectangle){
        ARENA_MARGIN,
        ARENA_MARGIN,
        SCREEN_WIDTH - 2.0f * ARENA_MARGIN,
        SCREEN_HEIGHT - 2.0f * ARENA_MARGIN
    };

    Vector2 center = {
        gs->arena.x + gs->arena.width / 2.0f,
        gs->arena.y + gs->arena.height / 2.0f
    };

    player_init(&gs->player, center);
    bullet_pool_init(&gs->bullets);
}

void game_update(GameState *gs)
{
    float dt = GetFrameTime();

    player_update(&gs->player, dt, gs->arena);

    /* ── Shooting ─────────────────────────────────────────────────────── */
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        Vector2 muzzle = Vector2Add(
            gs->player.position,
            Vector2Scale(gs->player.aim_direction, PLAYER_RADIUS + 2.0f)
        );
        bullet_pool_fire(&gs->bullets, muzzle, gs->player.aim_direction);
    }

    bullet_pool_update(&gs->bullets, dt, gs->arena);
}

void game_draw(const GameState *gs)
{
    BeginDrawing();
        ClearBackground(BLACK);
        draw_arena(gs);
        bullet_pool_draw(&gs->bullets);
        player_draw(&gs->player);
        draw_hud(gs);
    EndDrawing();
}
