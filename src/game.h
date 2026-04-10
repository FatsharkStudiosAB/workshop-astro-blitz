/*
 * game.h -- Top-level game state and interface
 */

#pragma once

#include "bullet.h"
#include "enemy.h"
#include "player.h"
#include "raylib.h"
#include "tilemap.h"

/* ── Constants ─────────────────────────────────────────────────────────────── */

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

/* ── Types ─────────────────────────────────────────────────────────────────── */

/* Game phase -- controls which branch of update/draw runs */
typedef enum { PHASE_PLAYING, PHASE_GAME_OVER } GamePhase;

/* Run statistics shown on the game-over screen */
typedef struct {
    int kills;           /* total enemies killed this run */
    float survival_time; /* seconds survived */
    int waves_spawned;   /* number of enemy waves spawned */
} GameStats;

typedef struct {
    Player player;
    BulletPool bullets;
    EnemyPool enemies;
    Tilemap tilemap;
    Camera2D camera;
    Rectangle arena;   /* world bounds (derived from tilemap) */
    float spawn_timer; /* time until next enemy wave */
    GamePhase phase;
    GameStats stats;
} GameState;

/* ── Public API ────────────────────────────────────────────────────────────── */

void game_init(GameState *gs);
void game_update(GameState *gs);
void game_draw(const GameState *gs);

/* Transition to PHASE_GAME_OVER if the player is dead. Separated from
 * game_update so it can be unit-tested without a Raylib window context. */
void game_check_death(GameState *gs);
