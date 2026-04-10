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

    p->melee_cooldown = 0.0f;
    p->melee_timer = 0.0f;
    p->melee_direction = (Vector2){0};

    p->speed_bonus = 0.0f;
    p->dash_cd_mult = 0.0f; /* 0 means "use 1.0" in player_update */
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
        float cd_mult = (p->dash_cd_mult > 0.0f) ? p->dash_cd_mult : 1.0f;
        p->dash_cooldown = DASH_COOLDOWN * cd_mult;
        return;
    }

    /* ── Normal movement ──────────────────────────────────────────────── */
    Vector2 move_dir = get_movement_input(p->aim_direction, movement_layout);
    float eff_speed = PLAYER_SPEED + p->speed_bonus;
    p->position = Vector2Add(p->position, Vector2Scale(move_dir, eff_speed * dt));
    clamp_to_arena(p, arena);
    resolve_tile_collision(p, tm);
}

void player_draw(const Player *p) {
    Vector2 pos = p->position;
    Vector2 aim = p->aim_direction;
    float r = PLAYER_RADIUS;

    /* Directional basis vectors */
    Vector2 perp = {-aim.y, aim.x};
    Vector2 back = {-aim.x, -aim.y};

    /* ── Color palette ────────────────────────────────────────────────── */
    bool d = p->is_dashing;
    Color neon = d ? (Color){80, 180, 255, 255} : (Color){0, 230, 210, 255};
    Color dark = d ? (Color){12, 30, 80, 255} : (Color){6, 35, 50, 255};
    Color mid = d ? (Color){20, 55, 140, 255} : (Color){10, 60, 80, 255};
    Color visor = d ? (Color){160, 220, 255, 255} : (Color){180, 255, 245, 255};
    Color glow_col = d ? (Color){80, 160, 255, 45} : (Color){0, 220, 200, 30};
    Color aim_col = (Color){255, 50, 120, 180};

    /* ── Ambient glow ─────────────────────────────────────────────────── */
    DrawCircleV(pos, r + 5.0f, glow_col);

    /* ── Dash thruster flare (only while dashing) ─────────────────────── */
    if (d) {
        Vector2 exhaust = Vector2Add(pos, Vector2Scale(back, r + 6.0f));
        Vector2 fl =
            Vector2Add(pos, Vector2Add(Vector2Scale(back, r * 0.6f), Vector2Scale(perp, 4.0f)));
        Vector2 fr =
            Vector2Add(pos, Vector2Add(Vector2Scale(back, r * 0.6f), Vector2Scale(perp, -4.0f)));
        DrawTriangle(fl, exhaust, fr, (Color){120, 200, 255, 160});
    }

    /* ── Body: tapered hull (wide at shoulders, narrow at back) ───────── */
    /*    Built from two triangles forming a kite/shield shape            */
    Vector2 nose = Vector2Add(pos, Vector2Scale(aim, r * 1.1f));
    Vector2 shoulder_l =
        Vector2Add(pos, Vector2Add(Vector2Scale(aim, -r * 0.15f), Vector2Scale(perp, r * 0.85f)));
    Vector2 shoulder_r =
        Vector2Add(pos, Vector2Add(Vector2Scale(aim, -r * 0.15f), Vector2Scale(perp, -r * 0.85f)));
    Vector2 tail = Vector2Add(pos, Vector2Scale(back, r * 0.9f));

    /* Dark fill: front half */
    DrawTriangle(nose, shoulder_l, shoulder_r, dark);
    /* Dark fill: rear half */
    DrawTriangle(shoulder_l, tail, shoulder_r, dark);
    /* Mid-tone armor plate on front */
    Vector2 plate_l =
        Vector2Add(pos, Vector2Add(Vector2Scale(aim, r * 0.15f), Vector2Scale(perp, r * 0.55f)));
    Vector2 plate_r =
        Vector2Add(pos, Vector2Add(Vector2Scale(aim, r * 0.15f), Vector2Scale(perp, -r * 0.55f)));
    DrawTriangle(nose, plate_l, plate_r, mid);

    /* ── Hull outline (neon edges) ────────────────────────────────────── */
    DrawLineEx(nose, shoulder_l, 1.5f, neon);
    DrawLineEx(nose, shoulder_r, 1.5f, neon);
    DrawLineEx(shoulder_l, tail, 1.5f, neon);
    DrawLineEx(shoulder_r, tail, 1.5f, neon);

    /* ── Shoulder pauldrons (bright accent pips) ──────────────────────── */
    DrawCircleV(shoulder_l, 2.5f, neon);
    DrawCircleV(shoulder_r, 2.5f, neon);

    /* ── Visor (bright slit across the front -- the "face") ───────────── */
    Vector2 visor_l =
        Vector2Add(pos, Vector2Add(Vector2Scale(aim, r * 0.4f), Vector2Scale(perp, r * 0.35f)));
    Vector2 visor_r =
        Vector2Add(pos, Vector2Add(Vector2Scale(aim, r * 0.4f), Vector2Scale(perp, -r * 0.35f)));
    DrawLineEx(visor_l, visor_r, 2.5f, visor);

    /* ── Weapon barrel (extends from right shoulder forward) ──────────── */
    Vector2 gun_base =
        Vector2Add(pos, Vector2Add(Vector2Scale(aim, r * 0.1f), Vector2Scale(perp, -r * 0.5f)));
    Vector2 gun_tip =
        Vector2Add(pos, Vector2Add(Vector2Scale(aim, r + 5.0f), Vector2Scale(perp, -r * 0.15f)));
    DrawLineEx(gun_base, gun_tip, 2.0f, neon);

    /* ── Reactor core (center glow) ───────────────────────────────────── */
    DrawCircleV(pos, 3.0f, (Color){neon.r, neon.g, neon.b, 80});
    DrawCircleV(pos, 1.5f, visor);

    /* ── Aim laser (thin magenta line with crosshair) ─────────────────── */
    Vector2 aim_start = Vector2Add(pos, Vector2Scale(aim, r + 6.0f));
    Vector2 aim_end = Vector2Add(pos, Vector2Scale(aim, r + 20.0f));
    DrawLineEx(aim_start, aim_end, 1.0f, aim_col);
    DrawCircleV(aim_end, 2.0f, aim_col);
}
