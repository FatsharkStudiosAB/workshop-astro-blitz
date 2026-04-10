/*
 * player.h -- Player state and interface
 */

#pragma once

#include "raylib.h"
#include "raymath.h"

/* ── Constants ─────────────────────────────────────────────────────────────── */

#define PLAYER_RADIUS       12.0f
#define PLAYER_SPEED        200.0f
#define PLAYER_MAX_HP       100.0f

#define DASH_SPEED          600.0f
#define DASH_DURATION       0.15f
#define DASH_COOLDOWN       1.0f
#define DASH_INVINCIBLE     1  /* 1 = invincible during dash */

/* ── Types ─────────────────────────────────────────────────────────────────── */

typedef struct {
    Vector2 position;
    Vector2 aim_direction;     /* normalized, toward mouse cursor */
    float   hp;
    float   max_hp;

    /* Dash state */
    bool    is_dashing;
    Vector2 dash_direction;
    float   dash_timer;        /* time remaining in current dash */
    float   dash_cooldown;     /* time remaining before dash is available */
} Player;

/* ── Public API ────────────────────────────────────────────────────────────── */

void player_init(Player *p, Vector2 start_pos);
void player_update(Player *p, float dt, Rectangle arena);
void player_draw(const Player *p);
