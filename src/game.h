/*
 * game.h -- Top-level game state and interface
 */

#pragma once

#include "bullet.h"
#include "enemy.h"
#include "player.h"
#include "raylib.h"

/* ── Constants ─────────────────────────────────────────────────────────────── */

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

/* Arena is inset slightly from the screen edges; the HUD overlays the play area */
#define ARENA_MARGIN 10.0f

/* ── Types ─────────────────────────────────────────────────────────────────── */

typedef struct {
    Player player;
    BulletPool bullets;
    EnemyPool enemies;
    Rectangle arena;
    float spawn_timer; /* time until next enemy wave */
} GameState;

/* ── Public API ────────────────────────────────────────────────────────────── */

void game_init(GameState *gs);
void game_update(GameState *gs);
void game_draw(const GameState *gs);
