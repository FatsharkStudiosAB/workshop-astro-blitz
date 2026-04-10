/*
 * player.c -- Player movement, aiming, and dash
 */

#include "player.h"
#include <math.h>

/* ── Helpers ───────────────────────────────────────────────────────────────── */

static Vector2 get_movement_input(void)
{
    Vector2 dir = {0};
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))    dir.y -= 1.0f;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))   dir.y += 1.0f;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))   dir.x -= 1.0f;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT))  dir.x += 1.0f;

    /* Normalize so diagonals aren't faster */
    float len = Vector2Length(dir);
    if (len > 0.0f) {
        dir = Vector2Scale(dir, 1.0f / len);
    }
    return dir;
}

static void clamp_to_arena(Player *p, Rectangle arena)
{
    float r = PLAYER_RADIUS;
    if (p->position.x - r < arena.x)                   p->position.x = arena.x + r;
    if (p->position.x + r > arena.x + arena.width)     p->position.x = arena.x + arena.width - r;
    if (p->position.y - r < arena.y)                   p->position.y = arena.y + r;
    if (p->position.y + r > arena.y + arena.height)    p->position.y = arena.y + arena.height - r;
}

/* ── Public ────────────────────────────────────────────────────────────────── */

void player_init(Player *p, Vector2 start_pos)
{
    p->position      = start_pos;
    p->aim_direction  = (Vector2){0.0f, -1.0f};
    p->hp            = PLAYER_MAX_HP;
    p->max_hp        = PLAYER_MAX_HP;

    p->is_dashing    = false;
    p->dash_direction = (Vector2){0};
    p->dash_timer    = 0.0f;
    p->dash_cooldown = 0.0f;
}

void player_update(Player *p, float dt, Rectangle arena)
{
    /* ── Aim toward mouse cursor ──────────────────────────────────────── */
    Vector2 mouse = GetMousePosition();
    Vector2 to_mouse = Vector2Subtract(mouse, p->position);
    float dist = Vector2Length(to_mouse);
    if (dist > 0.0f) {
        p->aim_direction = Vector2Scale(to_mouse, 1.0f / dist);
    }

    /* ── Dash cooldown ────────────────────────────────────────────────── */
    if (p->dash_cooldown > 0.0f) {
        p->dash_cooldown -= dt;
        if (p->dash_cooldown < 0.0f) {
            p->dash_cooldown = 0.0f;
        }
    }

    /* ── Active dash ──────────────────────────────────────────────────── */
    if (p->is_dashing) {
        float dash_dt = dt;
        if (dash_dt > p->dash_timer) {
            dash_dt = p->dash_timer;
        }

        p->position = Vector2Add(p->position, Vector2Scale(p->dash_direction, DASH_SPEED * dash_dt));
        clamp_to_arena(p, arena);

        p->dash_timer -= dash_dt;
        if (p->dash_timer > 0.0f) {
            return;  /* Still dashing -- no normal movement */
        }

        p->is_dashing = false;
        p->dash_timer = 0.0f;
        dt -= dash_dt;
    }

    /* ── Initiate dash ────────────────────────────────────────────────── */
    if (IsKeyPressed(KEY_SPACE) && p->dash_cooldown <= 0.0f) {
        Vector2 move_dir = get_movement_input();
        /* If no movement keys held, dash toward aim direction */
        if (Vector2Length(move_dir) < 0.01f) {
            move_dir = p->aim_direction;
        }
        p->is_dashing     = true;
        p->dash_direction  = move_dir;
        p->dash_timer      = DASH_DURATION;
        p->dash_cooldown   = DASH_COOLDOWN;
        return;
    }

    /* ── Normal movement ──────────────────────────────────────────────── */
    Vector2 move_dir = get_movement_input();
    p->position = Vector2Add(p->position, Vector2Scale(move_dir, PLAYER_SPEED * dt));
    clamp_to_arena(p, arena);
}

void player_draw(const Player *p)
{
    /* Body */
    Color body_color = p->is_dashing ? SKYBLUE : GREEN;
    DrawCircleV(p->position, PLAYER_RADIUS, body_color);

    /* Aim direction indicator (line from center outward) */
    Vector2 aim_end = Vector2Add(p->position, Vector2Scale(p->aim_direction, PLAYER_RADIUS + 10.0f));
    DrawLineEx(p->position, aim_end, 2.0f, YELLOW);
}
