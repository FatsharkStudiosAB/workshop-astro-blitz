/*
 * player.c -- Player movement, aiming, and dash
 */

#include "player.h"
#include "tilemap.h"
#include <math.h>

/* ── Movement direction helpers (pure logic, no Raylib input calls) ─────── */

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

Vector2 player_calc_move_dir_8dir(bool up, bool down, bool left, bool right) {
    Vector2 dir = {0};
    if (up) {
        dir.y -= 1.0f;
    }
    if (down) {
        dir.y += 1.0f;
    }
    if (left) {
        dir.x -= 1.0f;
    }
    if (right) {
        dir.x += 1.0f;
    }

    /* Normalize so diagonals aren't faster */
    float len = Vector2Length(dir);
    if (len > 0.0f) {
        dir = Vector2Scale(dir, 1.0f / len);
    }
    return dir;
}

static Vector2 get_movement_input(Vector2 aim_dir, MovementLayout layout) {
    bool fwd_or_up = IsKeyDown(KEY_W) || IsKeyDown(KEY_UP);
    bool back_or_down = IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN);
    bool left = IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT);
    bool right = IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT);

    if (layout == MOVEMENT_8DIR) {
        return player_calc_move_dir_8dir(fwd_or_up, back_or_down, left, right);
    }
    return player_calc_move_dir(aim_dir, fwd_or_up, back_or_down, left, right);
}

static void clamp_to_arena(Player *p, Rectangle arena) {
    float r = PLAYER_RADIUS;
    if (p->position.x - r < arena.x) {
        p->position.x = arena.x + r;
    }
    if (p->position.x + r > arena.x + arena.width) {
        p->position.x = arena.x + arena.width - r;
    }
    if (p->position.y - r < arena.y) {
        p->position.y = arena.y + r;
    }
    if (p->position.y + r > arena.y + arena.height) {
        p->position.y = arena.y + arena.height - r;
    }
}

/*
 * resolve_tile_collision -- push the player out of any solid tiles.
 *
 * Checks the four cardinal points on the player's bounding circle. If any
 * point is inside a solid tile, the player is pushed back along that axis.
 * This gives smooth sliding along walls.
 */
static void resolve_tile_collision(Player *p, const Tilemap *tm) {
    if (!tm) {
        return;
    }

    float r = PLAYER_RADIUS;

    /* Check right edge */
    if (tilemap_is_solid(tm, p->position.x + r, p->position.y)) {
        int tx = (int)floorf((p->position.x + r) / (float)tm->tile_size);
        p->position.x = (float)(tx * tm->tile_size) - r - 0.01f;
    }
    /* Check left edge */
    if (tilemap_is_solid(tm, p->position.x - r, p->position.y)) {
        int tx = (int)floorf((p->position.x - r) / (float)tm->tile_size);
        p->position.x = (float)((tx + 1) * tm->tile_size) + r + 0.01f;
    }
    /* Check bottom edge */
    if (tilemap_is_solid(tm, p->position.x, p->position.y + r)) {
        int ty = (int)floorf((p->position.y + r) / (float)tm->tile_size);
        p->position.y = (float)(ty * tm->tile_size) - r - 0.01f;
    }
    /* Check top edge */
    if (tilemap_is_solid(tm, p->position.x, p->position.y - r)) {
        int ty = (int)floorf((p->position.y - r) / (float)tm->tile_size);
        p->position.y = (float)((ty + 1) * tm->tile_size) + r + 0.01f;
    }
}

/* ── Public ────────────────────────────────────────────────────────────────── */

void player_init(Player *p, Vector2 start_pos) {
    p->position = start_pos;
    p->aim_direction = (Vector2){0.0f, -1.0f};
    p->hp = PLAYER_MAX_HP;
    p->max_hp = PLAYER_MAX_HP;
    p->current_weapon = weapon_get_default();

    p->is_dashing = false;
    p->dash_direction = (Vector2){0};
    p->dash_timer = 0.0f;
    p->dash_cooldown = 0.0f;
}

void player_update(Player *p, float dt, Rectangle arena, const Tilemap *tm, Camera2D camera,
                   MovementLayout movement_layout) {
    /* ── Aim toward mouse cursor (world space) ───────────────────────── */
    Vector2 screen_mouse = GetMousePosition();
    Vector2 world_mouse = GetScreenToWorld2D(screen_mouse, camera);
    Vector2 to_mouse = Vector2Subtract(world_mouse, p->position);
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
        resolve_tile_collision(p, tm);

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
        Vector2 move_dir = get_movement_input(p->aim_direction, movement_layout);
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
    Vector2 move_dir = get_movement_input(p->aim_direction, movement_layout);
    p->position = Vector2Add(p->position, Vector2Scale(move_dir, PLAYER_SPEED * dt));
    clamp_to_arena(p, arena);
    resolve_tile_collision(p, tm);
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
