/*
 * player.h -- Player state and interface
 */

#pragma once

#include "raylib.h"
#include "raymath.h"
#include "settings.h"
#include "weapon.h"
#include <stdbool.h>

/* Forward declaration to avoid circular include */
typedef struct Tilemap Tilemap;

/* ── Constants ─────────────────────────────────────────────────────────────── */

#define PLAYER_RADIUS 12.0f
#define PLAYER_SPEED 200.0f
#define PLAYER_MAX_HP 100.0f

#define DASH_SPEED 600.0f
#define DASH_DURATION 0.15f
#define DASH_COOLDOWN 1.0f
#define DASH_INVINCIBLE 1 /* 1 = invincible during dash */

/* Melee attack */
#define MELEE_DAMAGE 3.0f
#define MELEE_ARC_DEGREES 120.0f /* total arc width in degrees */
#define MELEE_RANGE 40.0f        /* reach from player center */
#define MELEE_COOLDOWN 0.5f      /* seconds between swings */
#define MELEE_DURATION 0.15f     /* active swing time */
#define MELEE_KNOCKBACK 300.0f   /* knockback speed applied to enemies */

/* ── Types ─────────────────────────────────────────────────────────────────── */

typedef struct {
    Vector2 position;
    Vector2 aim_direction; /* normalized, toward mouse cursor */
    float hp;
    float max_hp;

    /* Weapon */
    Weapon current_weapon;

    /* Dash state */
    bool is_dashing;
    Vector2 dash_direction;
    float dash_timer;    /* time remaining in current dash */
    float dash_cooldown; /* time remaining before dash is available */

    /* Melee state */
    float melee_cooldown;    /* time remaining before next melee */
    float melee_timer;       /* time remaining in current swing (0 = not swinging) */
    Vector2 melee_direction; /* direction of current swing */
} Player;

/* ── Public API ────────────────────────────────────────────────────────────── */

/* Initialize a player at the given world position with full HP and no dash. */
void player_init(Player *p, Vector2 start_pos);

/*
 * player_update -- Update player movement, aiming, and dash.
 *
 * Uses the camera to convert screen-space mouse position to world coordinates
 * for aiming. Uses the tilemap for wall collision (NULL tilemap skips wall
 * checks). The movement_layout controls how WASD maps to directions.
 */
void player_update(Player *p, float dt, Rectangle arena, const Tilemap *tm, Camera2D camera,
                   MovementLayout movement_layout);

/* Draw the player's layered sci-fi visual (body, nose, glow, aim line). */
void player_draw(const Player *p);

/*
 * player_calc_move_dir -- Compute world-space movement direction from raw input.
 *
 * Converts raw WASD/arrow key input (forward/back/left/right) into a
 * world-space direction vector relative to the player's aim direction
 * (tank controls). The result is normalized so diagonals aren't faster.
 *
 * Parameters:
 *   aim_dir -- Normalized aim direction (where the player is pointing).
 *   forward -- True if W or Up is held.
 *   back    -- True if S or Down is held.
 *   left    -- True if A or Left is held.
 *   right   -- True if D or Right is held.
 *
 * Returns a normalized Vector2 (or zero vector if no input).
 */
Vector2 player_calc_move_dir(Vector2 aim_dir, bool forward, bool back, bool left, bool right);

/*
 * player_calc_move_dir_8dir -- Compute world-space movement from screen-relative input.
 *
 * 8-directional movement: WASD maps to fixed world directions regardless of
 * where the player is aiming (twin-stick style). W = up (-Y), S = down (+Y),
 * A = left (-X), D = right (+X). Diagonals are normalized.
 *
 * Parameters:
 *   up    -- True if W or Up is held.
 *   down  -- True if S or Down is held.
 *   left  -- True if A or Left is held.
 *   right -- True if D or Right is held.
 *
 * Returns a normalized Vector2 (or zero vector if no input).
 */
Vector2 player_calc_move_dir_8dir(bool up, bool down, bool left, bool right);
