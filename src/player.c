/*
 * player.c -- Player movement, aiming, and dash
 */

#include "player.h"
#include <math.h>

/* ── Helpers ───────────────────────────────────────────────────────────────── */

/*
 * player_calc_move_dir -- pure logic, no Raylib input calls.
 * Converts WASD booleans into a world-space direction relative to aim_dir.
 */
Vector2 player_calc_move_dir(Vector2 aim_dir, bool forward, bool back, bool left, bool right) {
    /* Perpendicular vectors used for strafing relative to aim_dir
     * in Raylib screen coordinates (Y+ is down): perp_left is strafe-left
     * relative to aim_dir, and perp_right is strafe-right.
     */
    Vector2 perp_left = {aim_dir.y, -aim_dir.x};
    Vector2 perp_right = {-aim_dir.y, aim_dir.x};

    Vector2 dir = {0};
    if (forward)
        dir = Vector2Add(dir, aim_dir);
    if (back)
        dir = Vector2Add(dir, Vector2Scale(aim_dir, -1.0f));
    if (left)
        dir = Vector2Add(dir, perp_left);
    if (right)
        dir = Vector2Add(dir, perp_right);

    /* Normalize so diagonals aren't faster */
    float len = Vector2Length(dir);
    if (len > 0.0f) {
        dir = Vector2Scale(dir, 1.0f / len);
    }
    return dir;
}

static Vector2 get_movement_input(Vector2 aim_dir) {
    bool fwd = IsKeyDown(KEY_W) || IsKeyDown(KEY_UP);
    bool back = IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN);
    bool left = IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT);
    bool right = IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT);
    return player_calc_move_dir(aim_dir, fwd, back, left, right);
}

static void clamp_to_arena(Player *p, Rectangle arena) {
    float r = PLAYER_RADIUS;
    if (p->position.x - r < arena.x)
        p->position.x = arena.x + r;
    if (p->position.x + r > arena.x + arena.width)
        p->position.x = arena.x + arena.width - r;
    if (p->position.y - r < arena.y)
        p->position.y = arena.y + r;
    if (p->position.y + r > arena.y + arena.height)
        p->position.y = arena.y + arena.height - r;
}

/* ── Public ────────────────────────────────────────────────────────────────── */

void player_init(Player *p, Vector2 start_pos) {
    p->position = start_pos;
    p->aim_direction = (Vector2){0.0f, -1.0f};
    p->hp = PLAYER_MAX_HP;
    p->max_hp = PLAYER_MAX_HP;

    p->is_dashing = false;
    p->dash_direction = (Vector2){0};
    p->dash_timer = 0.0f;
    p->dash_cooldown = 0.0f;
}

void player_update(Player *p, float dt, Rectangle arena) {
    /* ── Aim toward mouse cursor ──────────────────────────────────────── */
    Vector2 mouse = GetMousePosition();
    Vector2 to_mouse = Vector2Subtract(mouse, p->position);
    float dist = Vector2Length(to_mouse);
    if (dist > 1.0f) { /* dead zone: ignore cursor within 1px of player center */
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

        p->position =
            Vector2Add(p->position, Vector2Scale(p->dash_direction, DASH_SPEED * dash_dt));
        clamp_to_arena(p, arena);

        p->dash_timer -= dash_dt;
        if (p->dash_timer > 0.0f) {
            return; /* Still dashing -- no normal movement */
        }

        p->is_dashing = false;
        p->dash_timer = 0.0f;
        dt -= dash_dt;
    }

    /* ── Initiate dash ────────────────────────────────────────────────── */
    if (IsKeyPressed(KEY_SPACE) && p->dash_cooldown <= 0.0f) {
        Vector2 move_dir = get_movement_input(p->aim_direction);
        /* If no movement keys held, dash toward aim direction */
        if (Vector2Length(move_dir) < 0.01f) {
            move_dir = p->aim_direction;
        }
        p->is_dashing = true;
        p->dash_direction = move_dir;
        p->dash_timer = DASH_DURATION;
        p->dash_cooldown = DASH_COOLDOWN;
        return;
    }

    /* ── Normal movement ──────────────────────────────────────────────── */
    Vector2 move_dir = get_movement_input(p->aim_direction);
    p->position = Vector2Add(p->position, Vector2Scale(move_dir, PLAYER_SPEED * dt));
    clamp_to_arena(p, arena);
}

void player_draw(const Player *p) {
    Vector2 pos = p->position;
    Vector2 aim = p->aim_direction;
    float r = PLAYER_RADIUS;

    /* Perpendicular to aim (for left/right offsets) */
    Vector2 perp = {-aim.y, aim.x};

    /* ── Color palette ────────────────────────────────────────────────── */
    /* Normal: cyan/teal sci-fi.  Dashing: electric blue shift. */
    Color body_fill = p->is_dashing ? (Color){20, 60, 160, 255} : (Color){10, 60, 80, 255};
    Color body_outline = p->is_dashing ? (Color){80, 160, 255, 255} : (Color){0, 220, 200, 255};
    Color glow = p->is_dashing ? (Color){80, 160, 255, 60} : (Color){0, 220, 200, 40};
    Color core_color = p->is_dashing ? (Color){180, 220, 255, 255} : (Color){200, 255, 240, 255};
    Color aim_color = (Color){255, 50, 120, 200};

    /* ── Outer glow ───────────────────────────────────────────────────── */
    DrawCircleV(pos, r + 4.0f, glow);

    /* ── Body fill ────────────────────────────────────────────────────── */
    DrawCircleV(pos, r, body_fill);

    /* ── Directional nose (triangle pointing in aim direction) ─────── */
    Vector2 nose_tip = Vector2Add(pos, Vector2Scale(aim, r + 3.0f));
    Vector2 nose_l =
        Vector2Add(pos, Vector2Add(Vector2Scale(aim, r * 0.3f), Vector2Scale(perp, r * 0.5f)));
    Vector2 nose_r =
        Vector2Add(pos, Vector2Add(Vector2Scale(aim, r * 0.3f), Vector2Scale(perp, -r * 0.5f)));
    DrawTriangle(nose_tip, nose_l, nose_r, body_outline);

    /* ── Body outline (neon ring) ─────────────────────────────────────── */
    DrawCircleLinesV(pos, r, body_outline);

    /* ── Reactor core (bright center dot) ─────────────────────────────── */
    DrawCircleV(pos, 3.0f, core_color);

    /* ── Aim line (hot magenta, extends past body) ────────────────────── */
    Vector2 aim_start = Vector2Add(pos, Vector2Scale(aim, r + 4.0f));
    Vector2 aim_end = Vector2Add(pos, Vector2Scale(aim, r + 18.0f));
    DrawLineEx(aim_start, aim_end, 2.0f, aim_color);

    /* Small crosshair dot at the tip */
    DrawCircleV(aim_end, 2.0f, aim_color);
}
